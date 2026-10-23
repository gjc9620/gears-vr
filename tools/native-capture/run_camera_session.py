"""Optional session polling and retained cleanup for one CameraBone transaction.

The idle non-daemon guardian starts before any policy can write. The session
thread owns all calls until finish(); pending cleanup then transfers both the
policy reference and continuous-owner lock to that guardian. It never applies.
Only its dedicated sidecar is published, after verifying the original report's
session ID and process identities. The main session report is never rewritten.
"""
import copy
import json
from pathlib import Path
import threading
import time


def _policy(pid, checker):
    from run_camera_runtime import RuntimePolicy
    return RuntimePolicy(pid, checker)


class RunCameraSession:
    def __init__(self, game, checker, *, session_path, session_id, owner, publish,
                 policy_factory=None, clock=time.monotonic, thread_factory=threading.Thread,
                 retry_wait=None):
        self.game, self.owner = copy.deepcopy(game), copy.deepcopy(owner)
        self.checker, self.factory = checker, policy_factory or _policy
        self.session_path, self.session_id = Path(session_path).resolve(), session_id
        self.sidecar = self.session_path.with_suffix('.camera-cleanup.json')
        self.publish, self.clock = publish, clock
        self.policy, self.stopping, self.closed = None, False, False
        self.errors, self.last_error = [], None
        self.next_poll, self.cleanup_attempts = 0., 0
        self._command, self.done = threading.Event(), threading.Event()
        self._retry_wait = retry_wait or threading.Event().wait
        self._release_lock, self.lock_released = None, False
        self._finished, self._transferred = False, False
        self.thread = thread_factory(target=self._worker, name='RunCameraCleanup-' + str(game['pid']), daemon=False)
        self.thread.start()  # Failure leaves no Policy, reader, writer or journal.

    def _error(self, stage, error):
        self.last_error = dict(stage=stage, type=type(error).__name__, reason=str(error))
        if len(self.errors) < 16:
            self.errors.append(self.last_error)

    def snapshot(self):
        report = copy.deepcopy(self.policy.report) if self.policy is not None else dict(
            applied=False, applications=0, writes=0, restored=0, pending_fields=[], released=True)
        report.update(session_errors=copy.deepcopy(self.errors), session_last_error=copy.deepcopy(self.last_error),
                      session_stopping=self.stopping, session_closed=self.closed)
        return report

    @property
    def pending(self):
        return self.policy is not None and not self.closed

    def tick(self):
        if self.stopping or self.clock() < self.next_poll:
            return False
        self.next_poll = self.clock() + 1.
        try:
            if self.policy is None:
                if self.checker(self.game['pid']) != self.game:
                    raise RuntimeError('Run-camera original session identity is unavailable or changed')
                self.policy = self.factory(self.game['pid'], self.checker)
                if self.policy.identity != self.game:
                    self.stopping = True
                    raise RuntimeError('Run-camera policy captured another process identity')
            self.policy.tick()
        except Exception as error:
            self._error('tick', error)  # Optional policy failure never stops stereo.
        return True

    def close(self, attempts=1):
        self.stopping = True
        if self.closed:
            return True
        if self.policy is None:
            self.closed = True
            return True
        for _ in range(max(1, min(int(attempts), 3))):
            self.cleanup_attempts += 1
            try:
                report = self.policy.close()
                if (isinstance(report, dict) and report.get('closed') is True
                        and report.get('released') is True and report.get('pending_fields') == []):
                    self.closed = True
                    return True
                self._error('close_pending', RuntimeError('Run-camera cleanup has not released all responsibility'))
            except Exception as error:
                self._error('close', error)
        return False

    def cleanup_link(self):
        return dict(path=str(self.sidecar), session_id=self.session_id, pending=self.pending,
                    owner_retained=self.pending, guardian_non_daemon=True,
                    scope='this original game/session only; sidecar is current cleanup status')

    def finish(self, release_lock):
        """After the main report's final write, transfer lock only if still pending."""
        if self._finished:
            return self._transferred
        transfer = self.pending
        self._finished, self._transferred = True, transfer
        self._release_lock = release_lock if transfer else None
        self._command.set()
        if not transfer:
            self.thread.join(timeout=1.)
        return transfer

    def _publish(self):
        try:
            with self.session_path.open('r', encoding='utf-8') as stream:
                original = json.loads(stream.read(4 * 1024 * 1024))
            if (original.get('session_id') != self.session_id
                    or original.get('owner_identity') != self.owner or original.get('game_identity') != self.game):
                raise RuntimeError('Run-camera cleanup report session identity changed; sidecar write refused')
            self.publish(self.sidecar, dict(schema=1, kind='run_camera_cleanup', session_id=self.session_id,
                session_report=str(self.session_path), game_identity=self.game, owner_identity=self.owner,
                pending=not self.lock_released, policy_closed=self.closed, owner_lock_retained=not self.lock_released,
                cleanup_attempts=self.cleanup_attempts, policy=self.snapshot(),
                observed_perf_counter_ns=time.perf_counter_ns()))
        except Exception as error:
            self._error('cleanup_publish', error)  # Reporting cannot discard ownership.

    def _worker(self):
        while not self._command.wait(1.):
            pass
        if self._release_lock is None:
            self.done.set()
            return
        next_publish = 0.
        while not self.lock_released:
            if self.close(attempts=1):
                try:
                    self._release_lock()
                    self.lock_released = True
                except Exception as error:
                    self._error('owner_lock_close', error)
            if self.lock_released or self.clock() >= next_publish:
                self._publish()
                next_publish = self.clock() + 5.
            if not self.lock_released:
                self._retry_wait(1.)
        self.done.set()
