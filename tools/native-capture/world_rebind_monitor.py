"""Bounded owner coordinator; poll never builds certificates or waits for a worker.

Dependencies are explicit: observe performs read-only full-identity discovery;
prepare builds and exclusively publishes an immutable world_rebind Candidate using
HookEntryAttestation and a DLL digest from trusted launch-manifest configuration.
It must NOT derive that expected digest from the currently loaded/on-disk DLL.
Both callbacks must call check_live between bounded operations and close their
own process/attestation leases. Neither callback may signal events. No process,
input, focus, injection, or XR lifecycle operations belong in these callbacks.
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


class Cancelled(RuntimeError):
    pass


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

    def __post_init__(self):
        # Copy into immutable tuples: later adapter mutation cannot alter proof.
        for field in self.__dataclass_fields__:
            value = getattr(self, field)
            if not isinstance(value, tuple):
                value = _identity(value)
            else:
                value = _identity(dict(zip(('address', 'cls', 'outer', 'number', 'internal_index', 'name'), value)))
            object.__setattr__(self, field, value)

    def changed_from(self, baseline):
        return (self.local_player == baseline.local_player and any(
            getattr(self, key) != getattr(baseline, key)
            for key in ('controller', 'camera', 'level', 'world_info')))


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
                 commit_timeout=25.0, max_attempts=3, backoff=2.0, include_pawn_changes=False):
        if not isinstance(baseline, WorldSnapshot):
            raise ValueError('Full startup world identities required')
        if type(enabled) is not bool or type(include_pawn_changes) is not bool or int(started_qpc) <= 0:
            raise ValueError('Invalid opt-in/session')
        if any(v <= 0 for v in (probe_interval, build_timeout, prepare_timeout, commit_timeout, backoff)):
            raise ValueError('Timeouts must be positive')
        if type(max_attempts) is not int or not 1 <= max_attempts <= 10:
            raise ValueError('Invalid attempt bound')
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

    def _check_processes(self):
        if (self.identity(self.game['pid']) != self.game or
                self.identity(self.owner['pid']) != self.owner):
            raise Cancelled('Process identity changed')

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
        self.future = self.executor.submit(function)
        self.deadline = self.clock() + self.build_timeout

    def _work(self):
        snapshot = prepared = None
        try:
            self._check_live()
            snapshot = self.observe(self._check_live)
            self._check_live()
            if snapshot is None:
                return None, None, None
            if not isinstance(snapshot, WorldSnapshot):
                raise ValueError('Observer did not return full identities')
            if snapshot.local_player != self.baseline.local_player:
                return snapshot, None, None
            if not self.force_recovery and not self._changed_from_baseline(snapshot):
                return snapshot, None, None
            if snapshot == self.target and self.attempts >= self.max_attempts:
                return snapshot, None, None
            prepared = self.prepare(snapshot, self.generation, self.request, self._check_live)
            if (not isinstance(prepared, PreparedRebind) or prepared.snapshot != snapshot or
                    prepared.previous_generation != self.generation or prepared.request != self.request or
                    Path(prepared.path).name != self.request_filename):
                raise ValueError('Prepared candidate/request mismatch')
            return snapshot, prepared, None
        except Exception as error:
            return snapshot, prepared, type(error).__name__

    @property
    def request_filename(self):
        return f'world-rebind-{self.game["creation_time"]}-{self.generation + 1}-{self.request}.ini'

    def _fail(self, reason, events):
        self.cancel.set()
        self.prepared = None
        self.state = 'idle'
        self.next_poll = self.clock() + self.backoff * max(1, self.attempts)
        events.append(dict(event='world_rebind_failed', reason=reason, request=self.request))

    def _send(self, kind, status, events):
        """One event per accepted signal; no blind Prepare resend on timeout."""
        now = self.clock()
        if now < self.next_poll:
            return
        self._check_processes()
        self.signal_attempts += 1
        if self.signal(f'Local\\GearsVR.Capture.WorldRebind{kind}.{self.game["pid"]}'):
            self.state = 'preparing' if kind == 'Prepare' else 'committing'
            self.deadline = now + (self.prepare_timeout if kind == 'Prepare' else self.commit_timeout)
            self.timed_out = False
            events.append(dict(event='world_rebind_' + kind.lower(), request=self.request))
        elif self.signal_attempts >= self.max_attempts:
            # Keep the receipt: native has no Abort. A failed event must not
            # permit another candidate to overwrite or strand this transaction.
            self.state = 'blocked'
            events.append(dict(event='world_rebind_blocked', reason=kind.lower() + '_signal_failed', request=self.request))
        else:
            self.next_poll = now + self.backoff * self.signal_attempts

    def _notice_timeout(self, events):
        if not self.timed_out and self.clock() >= self.deadline:
            self.timed_out = True
            events.append(dict(event='world_rebind_waiting', reason='receipt_timeout',
                               stage=self.state, request=self.request))

    def _conflict(self, reason, events):
        self.cancel.set()
        self.state = 'blocked'
        events.append(dict(event='world_rebind_blocked', reason=reason, request=self.request))

    def poll(self, status, mode=None):
        """Return bounded diagnostic events; mode never authorizes a rebind."""
        events = []
        if self.state in ('closed', 'blocked') or not self.enabled:
            return events
        try:
            if not self._valid_status(status):
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
            now = self.clock()
            if self.state != 'idle' and generation != self.generation:
                if (self.state == 'committing' and generation == self.generation + 1 and
                        requests == self.request and rejected == self.rejections):
                    self.baseline = self.prepared.snapshot
                    self.prepared, self.target, self.attempts = None, None, 0
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
                if snapshot is not None and (self.force_recovery or self._changed_from_baseline(snapshot)):
                    if snapshot != self.target:
                        self.target, self.attempts = snapshot, 0
                    if prepared is not None or error:
                        self.attempts += 1
                if error and prepared is not None:
                    self.prepared, self.state = prepared, 'blocked'
                    events.append(dict(event='world_rebind_blocked', reason=error, request=self.request))
                elif error:
                    self._fail(error, events)
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
                        self._fail('prepare_rejected_or_request_race', events)
                elif requests == self.request and candidate == self.generation + 1:
                    # Native revalidates AFTER draining old scopes and GPU work;
                    # an owner-side re-observation cannot replace that guard.
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
                        self._fail('commit_candidate_lost', events)
            elif self.state == 'committing':
                if requests != self.request or candidate == 0 or rejected != self.rejections:
                    if candidate:
                        self._conflict('commit_request_conflict', events)
                    else:
                        self._fail('commit_rejected_or_request_race', events)
                else:
                    self._notice_timeout(events)
            elif self.state == 'idle' and now >= self.next_poll:
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
                self._start(self._work)
        except Exception as error:
            if self.prepared is not None or self.future is not None:
                self._conflict(type(error).__name__, events)
            else:
                self._fail(type(error).__name__, events)
        return events

    def close(self):
        self.cancel.set()
        self.state = 'closed'
        if self.executor is not None:
            self.executor.shutdown(wait=True, cancel_futures=True)
            self.executor = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
