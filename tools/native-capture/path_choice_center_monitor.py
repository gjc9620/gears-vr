"""Renew bounded, authenticated PathChoice transform leases from the owner.

Only the owner thread publishes sidecars. The single worker owns its read-only
process reader; it never sends input, changes windows or writes game memory.
"""
from concurrent.futures import Future
from pathlib import Path
import os
import stat
import queue
import threading
import time

MAX_SAMPLE_SECONDS = 2.0


class StaleLayoutRead(ValueError):
    """A completed read cannot obtain a new lease after its capture budget."""


class _ReadWorker:
    """One bounded daemon reader; it has no publication/control capability.

    An uninterruptible OS read must not hold up native/XR shutdown or Python's
    executor atexit join. The worker closes its own reader when it returns;
    process exit also releases any remaining read-only OS handles.
    """
    def __init__(self, probe):
        self.probe, self.jobs = probe, queue.Queue(maxsize=1)
        self.stop = threading.Event()
        self.thread = None
        self.error = None

    def submit(self, function):
        if self.stop.is_set():raise RuntimeError('PathChoice worker stopped')
        future = Future()
        self.jobs.put_nowait((future,function))
        if self.thread is None:
            self.thread = threading.Thread(target=self.run,name='PathChoice read-only',daemon=True)
            self.thread.start()
        return future

    def run(self):
        try:
            while not self.stop.is_set():
                try:future,function = self.jobs.get(timeout=.05)
                except queue.Empty:continue
                if self.stop.is_set():future.cancel();break
                if future.set_running_or_notify_cancel():
                    try:future.set_result(function())
                    except BaseException as error:future.set_exception(error)
        finally:
            try:self.probe.close()
            except Exception as error:self.error=type(error).__name__

    def close(self, timeout=.25):
        self.stop.set()
        if self.thread is None:
            self.probe.close()
            return True
        self.thread.join(timeout)
        return not self.thread.is_alive()


class LivePathChoiceProbe:
    def __init__(self, game, check):
        self.game, self.check, self.reader = dict(game), check, None

    def capture(self):
        self.check()
        try:
            if self.reader is None:
                from menu_state import MenuReader
                self.reader = MenuReader(self.game['pid'])
            from menu_control import reader_process_identity
            if reader_process_identity(self.reader) != (self.game['pid'], self.game['creation_time']):
                raise RuntimeError('PathChoice reader process changed')
            self.check()
            basic = self.reader.sample()
            if (basic.get('top') or {}).get('cls') != 'UIScene_PathChoice':
                return None
            camera = self.reader.r.resolve()
            self.check()
            sample = self.reader.sample(include_layout=True, include_owner=True,
                expected_controller=int(camera['controller_address'],16),
                expected_local_player=int(camera['local_player_address'],16))
            self.check()
            return sample
        except Exception:
            self.close()
            raise

    def close(self):
        if self.reader:
            self.reader.close()
            self.reader = None


class PathChoiceCenterMonitor:
    def __init__(self, path, game, owner, started_qpc, identity, *,
                 clock=time.monotonic, nonce_clock=time.perf_counter_ns,
                 probe_factory=LivePathChoiceProbe, builder=None, layout=None):
        from path_choice_trial_config import build
        from path_choice_layout import centered_translation
        self.path, self.game, self.owner = Path(path), dict(game), dict(owner)
        self.started_qpc, self.identity = int(started_qpc), identity
        self.clock, self.nonce_clock = clock, nonce_clock
        self.builder, self.layout = builder or build, layout or centered_translation
        self.cancel = threading.Event()
        self.probe = probe_factory(self.game, self.check)
        self.executor = _ReadWorker(self.probe)
        self.future = None
        self.request_generation = None
        self.request_started = None
        self.due, self.nonce = 0., 0
        self.last_bytes = None
        self.disabled = False
        self.baseline = None
        self.loaded = False
        self.last_wait = None
        self.cleanup_error = None
        self.worker_joined = None

    def check(self):
        if self.cancel.is_set():
            raise RuntimeError('PathChoice monitor cancelled')
        if any(self.identity(v['pid']) != v for v in (self.game,self.owner)):
            raise RuntimeError('PathChoice owner/game identity changed')

    def revoke(self):
        # Best-effort preservation of edits observed before deletion. This is
        # not an atomic ownership protocol for concurrent external writers.
        if self.last_bytes is not None:
            try:
                info = self.path.lstat()
                if stat.S_ISREG(info.st_mode) and info.st_size == len(self.last_bytes):
                    with self.path.open('rb') as stream:
                        owned = stream.read(len(self.last_bytes)+1) == self.last_bytes
                    if owned:self.path.unlink()
            except FileNotFoundError:
                pass
            except OSError as error:
                self.cleanup_error = type(error).__name__
                return False  # Retain ownership bytes for a later retry/expiry.
        self.last_bytes = None
        self.loaded = False
        return True

    def publish(self, raw):
        temporary = self.path.with_name(self.path.name+'.'+str(self.nonce)+'.tmp')
        try:
            with temporary.open('xb') as stream:
                stream.write(raw)
            for attempt in range(3):
                self.check()
                self.require_fresh_read()
                try:
                    os.replace(temporary,self.path)
                    break
                except PermissionError:
                    if attempt == 2:raise
                    time.sleep(.01)
            self.last_bytes = raw
        finally:
            try:temporary.unlink(missing_ok=True)
            except OSError:pass

    def require_fresh_read(self):
        elapsed = self.clock() - self.request_started if self.request_started is not None else -1.
        if not 0. <= elapsed <= MAX_SAMPLE_SECONDS:
            raise StaleLayoutRead('PathChoice layout read exceeded its acquisition budget')

    def poll(self, status):
        if self.disabled or self.cancel.is_set():
            return []
        self.check()
        events = []
        if not status or any(str(status.get(key)) != str(value) for key,value in (
                ('pid',self.game['pid']),('owner_pid',self.owner['pid']),
                ('owner_creation_time',self.owner['creation_time']),
                ('started_qpc',self.started_qpc),('phase','running'))):
            self.revoke()
            if self.last_wait == 'native_identity_or_phase':
                return []
            self.last_wait = 'native_identity_or_phase'
            return [dict(event='path_center_wait',reason=self.last_wait)]
        self.last_wait = None
        if status.get('path_trial_compiled') != '1':
            self.disabled = True
            self.revoke()
            return [dict(event='path_center_disabled',reason='native_support_missing')]
        generation = int(status['world_generation'])
        if generation <= 0:
            self.revoke()
            return []
        counters = {key:int(status.get('path_trial_'+key,'0')) for key in ('failed','nested','load_rejected')}
        if self.baseline is None:
            self.baseline = counters
        if any(counters[key] != self.baseline[key] for key in counters):
            self.disabled = True
            self.revoke()
            return [dict(event='path_center_disabled',reason='native_scope_or_load_failure')]
        if self.last_bytes is not None and self.request_generation != generation:
            self.revoke()
            events.append(dict(event='path_center_revoked',reason='world_changed'))
        if self.last_bytes is not None and not self.loaded and str(status.get('path_trial_nonce')) == str(self.nonce):
            self.loaded = True
            events.append(dict(event='path_center_native_loaded',nonce=self.nonce,world_generation=generation))
        if self.future is not None:
            if not self.future.done():
                return events
            pending,self.future = self.future,None
            try:
                sample = pending.result()
                self.check()
                if self.request_generation != generation:
                    self.revoke()
                    self.due = 0.
                    return events+[dict(event='path_center_discarded',reason='world_changed_during_read')]
                if sample is None:
                    was_active = self.last_bytes is not None
                    self.revoke()
                    self.due = self.clock()+1.
                    return events+([dict(event='path_center_revoked',reason='scene_closed')] if was_active else [])
                self.require_fresh_read()
                x,y = self.layout(sample)
                self.nonce = max(self.nonce+1,int(self.nonce_clock()))
                text = self.builder(sample['scene_owner'],pid=self.game['pid'],
                    creation_time=self.game['creation_time'],started_qpc=self.started_qpc,
                    world_generation=generation,nonce=self.nonce,shift_x=x,shift_y=y,duration_ms=8000)
                # Validation/serialization time is part of the same budget.
                self.require_fresh_read()
                self.publish(text.encode('ascii'))
                self.loaded = False
                self.due = self.clock()+3.
                events.append(dict(event='path_center_lease_published',nonce=self.nonce,
                    world_generation=generation,shift_x=x,shift_y=y,duration_ms=8000))
            except Exception as error:
                self.revoke()
                self.due = self.clock()+1.
                events.append(dict(event='path_center_acquisition_rejected',reason=type(error).__name__))
        if self.future is None and self.clock() >= self.due:
            self.request_generation = generation
            self.request_started = self.clock()
            self.future = self.executor.submit(self.probe.capture)
        return events

    def close(self):
        if self.executor is None:
            return
        self.cancel.set()
        try:
            self.revoke()
        finally:
            try:
                if self.future is not None:
                    self.future.cancel()
            finally:
                # A blocked read cannot prevent the owner from removing native
                # hooks and closing XR. Only this owner could publish its result.
                self.worker_joined = self.executor.close()
                if self.executor.error:self.cleanup_error=self.executor.error
                self.executor = None
