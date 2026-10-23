"""Bounded owner coordinator; poll never builds certificates or waits for a worker.

Dependencies are explicit: observe performs read-only full-identity discovery;
prepare builds and exclusively publishes an immutable world_rebind Candidate using
HookEntryAttestation and a DLL digest from trusted launch-manifest configuration.
It must NOT derive that expected digest from the currently loaded/on-disk DLL.
Both callbacks must call check_live between bounded operations and close their
own process/attestation leases. Neither callback may signal native control events. No process,
input, focus, injection, or XR lifecycle operations belong in these callbacks.
Optional prepare_changed acquires a full candidate once after a pointer hint or
an exact-session native receipt requiring recovery of a quiesced old world.
It must apply the supplied snapshot predicate before exclusive publication;
a rejected/unchanged snapshot returns (snapshot, None) without creating a file.
prepare must check_live immediately before exclusive publication. After a
successful publication it must return the receipt, even if cancellation arrives;
exceptions mean no file was published. This prevents orphaned exclusive paths.

The owner calls close() on shutdown (or uses a context manager). Cancellation is
cooperative: close joins the one worker, so dependencies must honor check_live.
No executor/thread is created for a disabled or unsupported native session.
"""
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
import threading
import time
import math


class Cancelled(RuntimeError):
    pass


class RetryablePreparation(RuntimeError):
    """Adapter-only marker for a rejected, unpublished read-only acquisition."""
    def __init__(self,error,*,scope='timestamp_sample'):
        if not isinstance(error,Exception):raise TypeError('Original preparation error required')
        if type(scope)is not str or scope not in ('timestamp_sample','acquisition_churn'):
            raise ValueError('Invalid read-only retry scope')
        super().__init__(str(error));self.error=error;self.scope=scope


def _guard_failure(error):
    """Reduce existing failure evidence to public enums; never sample or retry."""
    try:
        evidence = getattr(error, 'evidence', None)
        if not isinstance(evidence, dict):
            return None
        guards = evidence.get('guard_acquisitions')
        guard = guards[-1] if isinstance(guards, list) and guards else None
        has_guard = isinstance(guard, dict)
        guard = guard if has_guard else evidence
        # Top-level sample attempts have a different budget/meaning. Do not
        # mislabel them as strict guard attempts during discovery failure.
        attempts = guard.get('attempts') if has_guard else ([] if guards == [] else None)
        attempt = attempts[-1] if isinstance(attempts, list) and attempts else {}
        if not isinstance(attempt, dict):
            attempt = {}
        observation = attempt.get('camera_observation')
        observation = observation if isinstance(observation, dict) else {}
        def boolean(value):
            return value if type(value) is bool else None
        def state(record):
            value = record.get('consistent') if isinstance(record, dict) else None
            return 'unchanged' if value is True else 'changed' if value is False and record.get('observed_raw') is not None else 'unavailable'
        stages = {'identity', 'memory', 'header', 'target', 'timestamp', 'target_identity', 'target_field'}
        mismatches = observation.get('mismatches')
        mismatches = mismatches[:8] if isinstance(mismatches, list) else []
        mismatch_stages = [item.get('stage') if item.get('stage') in stages else 'unknown'
                           for item in mismatches if isinstance(item, dict)]
        read_error = observation.get('read_error')
        read_stage = (read_error.get('stage') if read_error.get('stage') in stages else 'unknown') if isinstance(read_error, dict) else None
        reason = attempt.get('reason') if attempt else evidence.get('reason')
        exception_type = attempt.get('exception_type') if attempt else evidence.get('exception_type')
        cause = 'unknown'
        if reason == 'Mounted camera target is not controlled turret':
            cause = 'mounted_target_mismatch'
        elif exception_type == 'MountedTimestampChanged' or reason in (
                'Mounted camera timestamp changed at bracket boundary',
                'Mounted camera frame changed during anchor bracket',
                'Cross-frame camera timestamp during identity checks'):
            cause = 'timestamp_changed'
        elif exception_type in ('RegistryHeaderChanged', 'AnchorRegistryChanged') or isinstance(guard.get('registry_change'), dict) or isinstance(evidence.get('registry_change'), dict):
            cause = 'registry_changed'
        elif read_stage is not None:
            cause = 'read_failed'
        elif guard.get('failure_reason') in ('Anchor guard acquisition deadline exceeded', 'Anchor sample acquisition deadline exceeded'):
            cause = 'deadline'
        relation = 'unknown'
        field = observation.get('target_field')
        if isinstance(field, dict) and 'value' in field:
            target = field['value']
            roles = observation.get('roles')
            controlled = roles.get('controlled') if isinstance(roles, dict) else None
            if target == 0:
                relation = 'null'
            elif observation.get('target_is_driver') is True:
                relation = 'driver'
            elif isinstance(controlled, dict) and target is not None and target == controlled.get('address'):
                relation = 'controlled'
            elif type(target) is int and target > 0:
                relation = 'other'
        elapsed = attempt.get('elapsed_ms')
        elapsed = elapsed if type(elapsed) in (int, float) and math.isfinite(elapsed) and 0 <= elapsed <= 60000 else None
        headers = observation.get('headers')
        headers = headers if isinstance(headers, dict) else {}
        stage = guard.get('stage')
        return dict(schema=1, stage=stage if stage in ('initial', 'startup_validation', 'sample',
            'sample_before', 'sample_after', 'certificate_assembly', 'assembly_after',
            'final_identity_validation', 'final_boundary') else 'unknown', cause=cause,
            attempts=min(len(attempts), 5) if isinstance(attempts, list) else None,
            retryable=boolean(attempt.get('retryable')),
            strict_guard_accepted=boolean(attempt.get('strict_guard_accepted')), elapsed_ms=elapsed,
            target_relation=relation, consistent_read=boolean(observation.get('consistent_read')),
            timestamp_state=state(observation.get('timestamp')),
            objects_registry_state=state(headers.get('objects')), names_registry_state=state(headers.get('names')),
            mismatch_stages=mismatch_stages, read_error_stage=read_stage)
    except Exception:
        return None  # Diagnostics cannot replace the original failure or affect lifecycle.


def _error_detail(error):
    """Keep diagnostics single-line and bounded without retaining tracebacks."""
    try:
        message = str(error)[:512]
    except Exception:
        return ''
    # Sampling deadlines otherwise hide the inner rejection that triggered
    # repeated acquisition. Reuse the last recorded attempt, never its raw
    # identity/POV graph; this adds no reads, retries or lifecycle decisions.
    try:
        evidence = getattr(error, 'evidence', None)
        guards = evidence.get('guard_acquisitions') if isinstance(evidence, dict) else None
        guard = guards[-1] if isinstance(guards, list) and guards else None
        attempts = guard.get('attempts') if isinstance(guard, dict) else None
        attempt = attempts[-1] if isinstance(attempts, list) and attempts else None
        if isinstance(attempt, dict):
            stage, reason = guard.get('stage'), attempt.get('reason')
            if (attempt.get('strict_guard_accepted') is True and reason is None and
                    guard.get('accepted') is False and
                    guard.get('failure_reason') == 'Anchor guard acquisition deadline exceeded'):
                reason = 'strict guard accepted after deadline'
            if isinstance(stage, str) and isinstance(reason, str):
                message = (message[:256] + ' [guard=' + stage[:32] + '; last=' +
                           reason[:160] + '; attempts=' + str(min(len(attempts), 5)) + ']')[:512]
    except Exception:
        pass # Evidence formatting must not replace the original failure.
    return ' '.join(''.join(c if c.isprintable() else ' ' for c in message).split())


def _identity(value):
    fields = ('address', 'cls', 'outer', 'number', 'internal_index')
    values = []
    for key in fields:
        item = value[key]
        if isinstance(item, str):
            item = int(item, 0)
        if type(item) is not int or not 0 <= item < 1 << (32 if key in fields[3:] else 64):
            raise ValueError('Invalid full identity: ' + key)
        if key in ('address', 'cls') and item == 0:
            raise ValueError('Null full identity: ' + key)
        values.append(item)
    name = value['name']
    if not isinstance(name, str) or not name or any(c in name for c in '\r\n\0'):
        raise ValueError('Invalid full identity name')
    return tuple(values) + (name,)


@dataclass(frozen=True)
class WorldSnapshot:
    local_player: object
    controller: object
    camera: object
    level: object
    world_info: object
    pawn: object
    mounted_context: tuple=()
    mounted_levels: tuple=()

    def __post_init__(self):
        # Copy into immutable tuples: later adapter mutation cannot alter proof.
        for field in self.__dataclass_fields__:
            value = getattr(self, field)
            if field in ('mounted_context','mounted_levels'):
                if len(value) not in (0,3 if field=='mounted_context' else 5):raise ValueError('Invalid mounted role/Level snapshot')
                object.__setattr__(self,field,tuple(_identity(item if not isinstance(item,tuple) else dict(zip(('address','cls','outer','number','internal_index','name'),item))) for item in value))
                continue
            if not isinstance(value, tuple):
                value = _identity(value)
            else:
                value = _identity(dict(zip(('address', 'cls', 'outer', 'number', 'internal_index', 'name'), value)))
            object.__setattr__(self, field, value)

    def changed_from(self, baseline):
        return (self.local_player == baseline.local_player and any(
            getattr(self, key) != getattr(baseline, key)
            for key in ('controller', 'camera', 'level', 'world_info','mounted_context','mounted_levels')))


@dataclass(frozen=True)
class PreparedRebind:
    snapshot: WorldSnapshot
    previous_generation: int
    request: int
    path: object


class WorldRebindMonitor:
    def __init__(self, *, game, owner, started_qpc, baseline, observe, prepare,
                 signal, identity, enabled=False, clock=time.monotonic,
                 probe_interval=1.0, build_timeout=15.0, prepare_timeout=12.0,
                 commit_timeout=25.0, max_attempts=3, backoff=2.0, include_pawn_changes=False,
                 changed_hint=None,hint_interval=0.1,prepare_changed=None):
        if not isinstance(baseline, WorldSnapshot):
            raise ValueError('Full startup world identities required')
        if type(enabled) is not bool or type(include_pawn_changes) is not bool or int(started_qpc) <= 0:
            raise ValueError('Invalid opt-in/session')
        if any(not math.isfinite(v) or v <= 0 for v in (probe_interval, build_timeout, prepare_timeout, commit_timeout, backoff)):
            raise ValueError('Timeouts must be positive')
        if type(max_attempts) is not int or not 1 <= max_attempts <= 10:
            raise ValueError('Invalid attempt bound')
        if (changed_hint is not None and not callable(changed_hint)) or not math.isfinite(hint_interval) or hint_interval<=0:
            raise ValueError('Invalid discovery hint')
        if prepare_changed is not None and not callable(prepare_changed):
            raise ValueError('Invalid changed-world preparation')
        self.game, self.owner = dict(game), dict(owner)
        self.started_qpc, self.baseline = str(started_qpc), baseline
        self.observe, self.prepare, self.signal, self.identity = observe, prepare, signal, identity
        self.enabled, self.clock = enabled, clock
        self.include_pawn_changes = include_pawn_changes
        self.interval, self.build_timeout = probe_interval, build_timeout
        self.prepare_timeout, self.commit_timeout = prepare_timeout, commit_timeout
        self.max_attempts, self.backoff = max_attempts, backoff
        self.state, self.executor, self.future = 'idle', None, None
        self.cancel = threading.Event()
        self.next_poll, self.deadline, self.last_request = 0.0, 0.0, 0
        self.target, self.attempts, self.prepared = None, 0, None
        self.generation, self.request, self.rejections = 0, 0, 0
        self.force_recovery, self.timed_out, self.signal_attempts = False, False, 0
        self.signal_retry_started=0.0
        self.signal_regular_attempts=0
        self.native_retry_attempts=0
        self.native_prepare_retry_attempts=0
        self.sampling_retry_attempts=0
        self.sampling_retry_due=None
        self.sampling_retry_regular=False
        self.changed_hint,self.hint_interval=changed_hint,hint_interval
        self.prepare_changed=prepare_changed
        self.next_hint,self.hint_retry_after=0.0,0.0
        self.work_timing={}
        self.work_requested_ns=0
        self.fast_build_poll=False
        self.hint_ready=False
        self.work_complete=None

    def _check_live(self):
        if self.cancel.is_set():
            raise Cancelled('World rebind cancelled')
        self._check_processes()

    def _changed_from_baseline(self, snapshot):
        # Pawn adaptation needs an explicit opt-in. Compare the complete owned
        # identity tuple, including same-address UObject generation changes;
        # the local-player identity must still remain exactly unchanged.
        return (isinstance(snapshot,WorldSnapshot) and snapshot.local_player == self.baseline.local_player and
                (snapshot.changed_from(self.baseline) or
                 (self.include_pawn_changes and snapshot.pawn != self.baseline.pawn)))

    def _needs_preparation(self, snapshot):
        return (isinstance(snapshot,WorldSnapshot) and snapshot.local_player==self.baseline.local_player and
                (self.force_recovery or self._changed_from_baseline(snapshot)) and
                # A rejected native handoff has already quiesced the old world.
                # It needs fresh proof even after the ordinary attempt budget;
                # _fail supplies bounded backoff, never a relaxed validation.
                (self.force_recovery or snapshot!=self.target or self.attempts<self.max_attempts))

    def poll_interval(self, ordinary_interval):
        """Honor a cheap hint's deadline without accelerating invalid sessions."""
        if not math.isfinite(ordinary_interval) or ordinary_interval<=0:
            raise ValueError('Invalid ordinary poll interval')
        if not self.enabled or self.timed_out:return ordinary_interval
        now=self.clock()
        waiting=(self.state in ('preparing','committing') or
                 (self.state=='building' and self.fast_build_poll)) and now<self.deadline
        sending=self.state in ('send_prepare','send_commit') and now>=self.next_poll
        short_retry=(0<self.native_retry_attempts<=self.max_attempts or
                     0<self.native_prepare_retry_attempts<=self.max_attempts or
                     self.sampling_retry_attempts>0)
        if self.state in ('send_prepare','send_commit') and now<self.next_poll:
            return min(ordinary_interval,max(.01,self.next_poll-now))
        if self.state=='idle' and short_retry and now<self.next_poll:
            return min(ordinary_interval,max(.01,self.next_poll-now))
        if self.state=='idle' and self.hint_ready and self.changed_hint is not None:
            # A due full observation also checks the hint. Backoff remains an
            # absolute lower bound; an expired hint must not create a spin.
            due=max(self.hint_retry_after,min(self.next_hint,self.next_poll))
            return min(ordinary_interval,max(.01,due-now))
        return min(ordinary_interval,.01) if waiting or sending else ordinary_interval

    def _check_processes(self):
        if (self.identity(self.game['pid']) != self.game or
                self.identity(self.owner['pid']) != self.owner):
            raise Cancelled('Process identity changed')

    def wait(self, ordinary_interval):
        """Wake for one completed worker; poll alone may consume or authorize it."""
        interval=self.poll_interval(ordinary_interval)
        complete=self.work_complete
        if self.enabled and self.state=='building' and self.future is not None and complete is not None:
            signalled=complete.wait(interval)
            # A status gap may defer consumption of a completed receipt. Take
            # its notification once so that gap cannot create an immediate loop.
            complete.clear()
            return signalled
        time.sleep(interval)
        return False

    def _check_hint_live(self):
        # A hint runs on the idle owner, before _start creates a new worker
        # cancellation scope. A previous failed worker must remain cancelled;
        # it must not also cancel every subsequent read-only discovery hint.
        if not self.enabled or self.state!='idle':raise Cancelled('Hint observer is no longer idle')
        self._check_processes()

    def _valid_status(self, status):
        expected = dict(pid=str(self.game['pid']), owner_pid=str(self.owner['pid']),
                        owner_creation_time=str(self.owner['creation_time']),
                        started_qpc=self.started_qpc, phase='running', complete='1',
                        continuous_session='1', world_handoff_supported='1')
        return bool(status and all(status.get(k) == v for k, v in expected.items()) and
                    status.get('stop_recorded', '0') == '0' and
                    status.get('publisher_stopped', '0') == '0')

    def _start(self, function):
        if self.executor is None:
            self.executor = ThreadPoolExecutor(max_workers=1, thread_name_prefix='world-rebind')
        self.cancel = threading.Event()
        self.timed_out = False
        self.work_requested_ns=time.perf_counter_ns()
        # A separate notification per worker prevents a late callback from an
        # earlier request waking the current request or shortening backoff.
        complete=threading.Event()
        self.work_complete=complete
        self.future = self.executor.submit(function)
        self.future.add_done_callback(lambda _future:complete.set())
        self.deadline = self.clock() + self.build_timeout

    def _work(self, single_requested=False):
        snapshot = prepared = None
        started=time.perf_counter_ns();observed=None
        single=single_requested and self.prepare_changed is not None
        try:
            self._check_live()
            if single:
                observed=started
                snapshot,prepared=self.prepare_changed(self.generation,self.request,
                    self._needs_preparation,self._check_live)
                if not isinstance(snapshot,WorldSnapshot):raise ValueError('Preparation did not return full identities')
                if prepared is None:return snapshot,None,None
                if not self._needs_preparation(snapshot):raise ValueError('Unexpected changed-world publication')
            else:
                snapshot = self.observe(self._check_live)
                observed=time.perf_counter_ns()
                self._check_live()
                if snapshot is None:return None, None, None
                if not isinstance(snapshot, WorldSnapshot):raise ValueError('Observer did not return full identities')
                if not self._needs_preparation(snapshot):return snapshot, None, None
                self.fast_build_poll=True
                prepared = self.prepare(snapshot, self.generation, self.request, self._check_live)
            if (not isinstance(prepared, PreparedRebind) or prepared.snapshot != snapshot or
                    prepared.previous_generation != self.generation or prepared.request != self.request or
                    Path(prepared.path).name != self.request_filename):
                raise ValueError('Prepared candidate/request mismatch')
            return snapshot, prepared, None
        except Exception as error:
            retryable=isinstance(error,RetryablePreparation)
            read_scope=error.scope if retryable else None
            if retryable:error=error.error
            failure = (type(error).__name__, _error_detail(error))
            summary = _guard_failure(error)
            if retryable:return snapshot,prepared,failure+(summary,True,read_scope)
            return snapshot, prepared, failure + (summary,) if summary is not None else failure
        finally:
            ended=time.perf_counter_ns()
            self.work_timing=dict(observe_ms=((observed or ended)-started)/1e6,
                prepare_ms=(ended-observed)/1e6 if observed is not None else 0,worker_ms=(ended-started)/1e6,
                single_acquisition=single,clock='qpc_monotonic_ns',requested_ns=self.work_requested_ns,
                worker_started_ns=started,worker_finished_ns=ended)

    @property
    def request_filename(self):
        return f'world-rebind-{self.game["creation_time"]}-{self.generation + 1}-{self.request}.ini'

    def _fail(self, reason, events, detail='', guard_failure=None, *, handoff_rejected=False, transient_read=False,native_timestamp=False,read_scope='timestamp_sample'):
        self.cancel.set()
        self.hint_ready=False
        self.prepared = None
        self.state = 'idle'
        self.timed_out=False
        delay=self._retry_delay(self.attempts)
        now=self.clock()
        fast_retry=False
        if handoff_rejected:
            # The exact native transaction cleared its candidate after fully
            # quiescing the old world. Try a fresh package promptly; only this
            # receipt authorizes a short bounded burst, never a blind resend.
            self.native_retry_attempts+=1
            if self.native_retry_attempts<=self.max_attempts:
                delay=min(delay,.1*self.native_retry_attempts);fast_retry=True
        if transient_read:
            self.sampling_retry_attempts+=1
            # Keep the existing bounded probe schedule. If it finishes before
            # the original ordinary deadline, add one earlier full acquisition
            # while retaining the post-burst retry for a just-later recovery.
            if self.sampling_retry_due is None:self.sampling_retry_due=now+delay
            fast_retry=self.sampling_retry_attempts<=self.max_attempts
            if fast_retry:delay=min(delay,.05*self.sampling_retry_attempts)
            else:
                previous_due=now+delay if not self.sampling_retry_regular else None
                if now<self.sampling_retry_due:delay=self.sampling_retry_due-now
                self.sampling_retry_due=previous_due if previous_due is not None else now+delay
                self.sampling_retry_regular=True
        else:
            # A hard failure establishes its own ordinary delay. Keep the
            # spent short-probe budget, but discard the old read deadline.
            self.sampling_retry_due=None
        if native_timestamp:
            # Exactly this publication was rejected for a completed timestamp
            # bracket. Do not spend the structural failure budget on it. Every
            # retry still reacquires and publishes a new full certificate.
            self.attempts=max(0,self.attempts-1)
            self.native_prepare_retry_attempts+=1
            delay=self._retry_delay(self.attempts)
            if self.native_prepare_retry_attempts<=self.max_attempts:
                delay=min(delay,.05*self.native_prepare_retry_attempts);fast_retry=True
        self.next_poll = now + delay
        self.hint_retry_after=self.next_poll
        events.append(dict(event='world_rebind_failed', reason=reason, request=self.request))
        if handoff_rejected:
            events[-1].update(fast_retry=fast_retry,retry_after_ms=round(delay*1000,3))
        if transient_read:
            events[-1].update(fast_retry=fast_retry,retry_after_ms=round(delay*1000,3),retry_scope=read_scope,
                worker_timing=dict(self.work_timing))
        if native_timestamp:
            events[-1].update(fast_retry=fast_retry,retry_after_ms=round(delay*1000,3),retry_scope='native_timestamp_sample',
                worker_timing=dict(self.work_timing))
        if detail:
            events[-1]['detail'] = detail
        if guard_failure is not None:
            events[-1]['guard_failure'] = guard_failure

    def _retry_delay(self, attempts):
        return self.backoff * min(max(1, attempts), self.max_attempts)

    def _send(self, kind, status, events):
        """One event per accepted signal; no blind Prepare resend on timeout."""
        now = self.clock()
        if now < self.next_poll:
            return
        self._check_processes()
        if self.signal_attempts==0:self.signal_retry_started,self.signal_regular_attempts=now,0
        self.signal_attempts += 1
        try:
            signalled=self.signal(f'Local\\GearsVR.Capture.WorldRebind{kind}.{self.game["pid"]}')
        except OSError:
            # Control.signal raises only when SetEvent itself failed. No
            # accepted request exists in this case; retain the same receipt.
            signalled=False
        if signalled:
            self.state = 'preparing' if kind == 'Prepare' else 'committing'
            self.deadline = now + (self.prepare_timeout if kind == 'Prepare' else self.commit_timeout)
            self.timed_out = False
            events.append(dict(event='world_rebind_' + kind.lower(), request=self.request))
            if kind=='Prepare':events[-1]['worker_timing']=dict(self.work_timing)
        else:
            # False means SetEvent was not accepted, unlike a receipt timeout.
            # Retain exactly one immutable package and retry only this signal;
            # transient event unavailability must not permanently strand VR.
            first_regular=self.signal_retry_started+self._retry_delay(1)
            fast_retry=(self.signal_attempts<=self.max_attempts and self.signal_regular_attempts==0 and now<first_regular)
            if fast_retry:
                delay=min(.05*self.signal_attempts,first_regular-now)
            else:
                # Preserve the ordinary retry timeline for every supported
                # attempt bound/backoff, including a burst cut short by it.
                if self.signal_regular_attempts==0 and now>=first_regular:self.signal_regular_attempts=1
                self.signal_regular_attempts+=1
                delay=self._retry_delay(self.signal_regular_attempts)
                if self.signal_regular_attempts==1:delay=max(.01,first_regular-now)
            self.next_poll = now + delay
            # No accepted request exists for this stage. A timeout from an
            # earlier Prepare receipt must not suppress its retry deadline.
            self.timed_out=False
            if self.signal_attempts>=self.max_attempts:
                events.append(dict(event='world_rebind_waiting', reason=kind.lower()+'_signal_retry',
                                   request=self.request, attempts=self.signal_attempts,
                                   fast_retry=fast_retry,retry_after_ms=round(delay*1000,3),retry_scope='control_signal'))

    def _notice_timeout(self, events):
        if not self.timed_out and self.clock() >= self.deadline:
            self.timed_out = True
            events.append(dict(event='world_rebind_waiting', reason='receipt_timeout',
                               stage=self.state, request=self.request))

    def _conflict(self, reason, events):
        self.cancel.set()
        self.hint_ready=False
        self.state = 'blocked'
        events.append(dict(event='world_rebind_blocked', reason=reason, request=self.request))

    def poll(self, status, mode=None):
        """Return bounded diagnostic events; mode never authorizes a rebind."""
        events = []
        if self.state in ('closed', 'blocked') or not self.enabled:
            return events
        try:
            if not self._valid_status(status):
                self.hint_ready=False
                if self.state == 'building':
                    self.cancel.set()
                # Keep a published/signalled transaction for exact-session
                # reconciliation; no action while native status is unavailable.
                return events
            self._check_processes()
            generation = int(status['world_generation'])
            requests = int(status['world_prepare_requests'])
            rejected = int(status['world_prepare_rejected'])
            candidate = int(status['world_candidate_generation'])
            if (min(generation, requests, rejected, candidate) < 0 or generation == 0 or
                    max(generation, requests, rejected, candidate) >= (1 << 64) - 1):
                raise ValueError('Invalid native counters')
            self.hint_ready=candidate==0
            now = self.clock()
            if self.state != 'idle' and generation != self.generation:
                if (self.state == 'committing' and generation == self.generation + 1 and
                        requests == self.request and rejected == self.rejections):
                    self.baseline = self.prepared.snapshot
                    self.prepared, self.target, self.attempts = None, None, 0
                    self.native_retry_attempts=0
                    self.native_prepare_retry_attempts=0
                    self.sampling_retry_attempts=0
                    self.sampling_retry_due=None
                    self.sampling_retry_regular=False
                    self.state, self.next_poll = 'idle', now + self.interval
                    events.append(dict(event='world_rebind_committed', generation=generation, request=self.request))
                else:
                    self.cancel.set()
                    self.state = 'blocked'
                    events.append(dict(event='world_rebind_blocked', reason='unexpected_generation', request=self.request))
                return events
            if self.state == 'building' and now >= self.deadline and not self.future.done():
                self.cancel.set()
                self._notice_timeout(events)
                return events
            if self.state == 'building' and self.future.done():
                snapshot, prepared, error = self.future.result()
                self.future = None
                self.work_complete=None
                transient_read=bool(error and len(error)>3 and error[3]is True)
                if error is None:
                    self.sampling_retry_attempts=0
                    self.sampling_retry_due=None
                    self.sampling_retry_regular=False
                if snapshot is not None and (self.force_recovery or self._changed_from_baseline(snapshot)):
                    if snapshot != self.target:
                        self.target, self.attempts = snapshot, 0
                    if prepared is not None or (error and not transient_read):
                        self.attempts += 1
                if error and prepared is not None:
                    self.prepared, self.state = prepared, 'blocked'
                    events.append(dict(event='world_rebind_blocked', reason=error[0], request=self.request))
                elif error:
                    self._fail(error[0], events, error[1], error[2] if len(error) > 2 else None,transient_read=transient_read,
                        read_scope=error[4]if transient_read and len(error)>4 else'timestamp_sample')
                elif prepared is None:
                    self.state, self.next_poll = 'idle', now + self.interval
                elif requests != self.request - 1 or candidate != 0:
                    self.prepared, self.state = prepared, 'blocked'
                    events.append(dict(event='world_rebind_blocked', reason='prepare_request_race', request=self.request))
                else:
                    self.last_request = self.request
                    self.prepared, self.state = prepared, 'send_prepare'
                    self.signal_attempts, self.next_poll = 0, now
                    self._send('Prepare', status, events)
            elif self.state == 'send_prepare':
                if requests != self.request - 1 or candidate != 0:
                    self.state = 'blocked'
                    events.append(dict(event='world_rebind_blocked', reason='prepare_request_race'))
                else:
                    self._send('Prepare', status, events)
            elif self.state == 'preparing':
                if requests > self.request or rejected != self.rejections:
                    if candidate:
                        self._conflict('prepare_request_conflict', events)
                    else:
                        self._fail('prepare_rejected_or_request_race', events,native_timestamp=(
                            requests==self.request and rejected==self.rejections+1 and
                            status.get('world_prepare_timestamp_request')==str(self.request)))
                elif requests == self.request and candidate == self.generation + 1:
                    # Native revalidates AFTER draining old scopes and GPU work;
                    # an owner-side re-observation cannot replace that guard.
                    self.native_prepare_retry_attempts=0
                    self.state, self.signal_attempts, self.next_poll = 'send_commit', 0, now
                    self._send('Commit', status, events)
                else:
                    self._notice_timeout(events)
            elif self.state == 'send_commit':
                if requests == self.request and candidate == self.generation + 1:
                    self._send('Commit', status, events)
                else:
                    if candidate:
                        self._conflict('commit_request_conflict', events)
                    else:
                        self._fail('commit_candidate_lost', events,handoff_rejected=(
                            status.get('world_handoff_phase')=='4' and requests==self.request and rejected==self.rejections))
            elif self.state == 'committing':
                if requests != self.request or candidate == 0 or rejected != self.rejections:
                    if candidate:
                        self._conflict('commit_request_conflict', events)
                    else:
                        self._fail('commit_rejected_or_request_race', events,handoff_rejected=(
                            status.get('world_handoff_phase')=='4' and requests==self.request and rejected==self.rejections))
                else:
                    self._notice_timeout(events)
            elif self.state == 'idle':
                hinted=False
                probe_due=now>=self.next_poll
                if (self.changed_hint is not None and now>=self.hint_retry_after and
                        (probe_due or now>=self.next_hint)):
                    self.next_hint=now+self.hint_interval
                    try:changed=self.changed_hint(self.baseline,self._check_hint_live)
                    except Cancelled:raise
                    except (RuntimeError,OSError,ValueError):changed=False
                    hinted=changed is True
                # A due periodic observation can use the same single-acquisition
                # path as an early pointer hint. False/failed hints retain full
                # periodic discovery, including same-address identity changes.
                if not probe_due and not hinted:return events
                # Never overlap a cancelled worker, overwrite an existing pending
                # candidate, or recycle a path after a failed native request.
                if self.future is not None:
                    if not self.future.done():
                        return events
                    self.future.result()
                    self.future = None
                if candidate != 0:
                    return events
                self.generation = generation
                self.request = max(self.last_request + 1, requests + 1)
                self.force_recovery = status.get('world_handoff_phase') == '4'
                # Native increments by exactly one; a timeout with no receipt
                # is ambiguous. Do not signal a path native will not consume.
                if self.request != requests + 1:
                    return events
                self.rejections = rejected
                self.state = 'building'
                self.fast_build_poll=hinted or self.force_recovery or self.sampling_retry_attempts>0 or self.native_prepare_retry_attempts>0
                # A quiesced world already requires a full new certificate;
                # another preliminary observation adds no authorization.
                self._start(lambda:self._work(hinted or self.force_recovery))
        except Exception as error:
            if self.prepared is not None or self.future is not None:
                self._conflict(type(error).__name__, events)
            else:
                self._fail(type(error).__name__, events, _error_detail(error))
        return events

    def close(self):
        self.cancel.set()
        self.state = 'closed'
        if self.executor is not None:
            self.executor.shutdown(wait=True, cancel_futures=True)
            self.executor = None
        self.work_complete=None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
