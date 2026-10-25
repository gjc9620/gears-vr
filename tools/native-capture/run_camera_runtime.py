"""Session lifecycle for the existing guarded CameraBone policy.

The factory returns a fresh LiveRegistryAnchorSampler and transfers ownership
of sampler.r.p to this wrapper. A reader stays open while its policy certificate
may still restore fields. This module adds no memory writes or weaker guards.
tick() is rate limited; restore()/close() stop further application and may be
retried when their report explicitly says released=False/closed=False.
An independently verified wait handle proves original-process exit; a failed
identity lookup or a non-signaled/failed wait never supplies that authority.
"""
import copy
import ctypes as c
from ctypes import wintypes as w
import json
from pathlib import Path
import struct
import time

from run_camera_policy import Policy, PolicyRejected

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PROFILE = ROOT / 'tools/package-inspector/reloaded-profile.json'
ERRORS = (RuntimeError, OSError, ValueError, struct.error)
WAIT_OBJECT_0, WAIT_TIMEOUT, WAIT_FAILED = 0, 0x102, 0xffffffff
LIFETIME_ACCESS = 0x00100000 | 0x1000  # SYNCHRONIZE | QUERY_LIMITED_INFORMATION


class WindowsLifetimeProof:
    """Bind one read-only wait handle to the original process, never its PID alone."""
    def __init__(self, pid, identity, *, kernel=None):
        if (not isinstance(identity, dict) or identity.get('pid') != pid or type(identity.get('creation_time')) is not int
                or identity['creation_time'] <= 0 or not isinstance(identity.get('image'), str)
                or not identity['image']):
            raise PolicyRejected('Run-camera lifetime needs original creation time and image')
        self.k = kernel if kernel is not None else c.WinDLL('kernel32', use_last_error=True)
        for name, args, result in (
                ('OpenProcess', [w.DWORD, w.BOOL, w.DWORD], w.HANDLE),
                ('GetProcessTimes', [w.HANDLE, *([c.POINTER(w.FILETIME)] * 4)], w.BOOL),
                ('QueryFullProcessImageNameW', [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)], w.BOOL),
                ('WaitForSingleObject', [w.HANDLE, w.DWORD], w.DWORD),
                ('CloseHandle', [w.HANDLE], w.BOOL)):
            function = getattr(self.k, name)
            function.argtypes, function.restype = args, result
        self.handle = self.k.OpenProcess(LIFETIME_ACCESS, False, pid)
        if not self.handle:
            raise c.WinError(c.get_last_error())
        try:
            times = [w.FILETIME() for _ in range(4)]
            if not self.k.GetProcessTimes(self.handle, *(c.byref(value) for value in times)):
                raise c.WinError(c.get_last_error())
            image = c.create_unicode_buffer(32768)
            length = w.DWORD(len(image))
            if not self.k.QueryFullProcessImageNameW(self.handle, 0, image, c.byref(length)):
                raise c.WinError(c.get_last_error())
            if not 0 < length.value < len(image) or not image.value:
                raise PolicyRejected('Run-camera lifetime image query has invalid length')
            creation = (times[0].dwHighDateTime << 32) | times[0].dwLowDateTime
            if (creation != identity['creation_time'] or
                    str(Path(image.value).resolve()).casefold() != str(Path(identity['image']).resolve()).casefold()):
                raise PolicyRejected('Run-camera lifetime handle belongs to a different process identity')
        except BaseException:
            self.close()
            raise

    def wait(self):
        if not self.handle:
            raise PolicyRejected('Run-camera lifetime handle is closed')
        result = self.k.WaitForSingleObject(self.handle, 0)
        if result == WAIT_FAILED:
            raise c.WinError(c.get_last_error())
        return result

    def close(self):
        if self.handle:
            if not self.k.CloseHandle(self.handle):
                raise c.WinError(c.get_last_error())
            self.handle = None


def _sampler(pid, profile_path):
    # Policy imports add package-inspector to sys.path. Keep native/process
    # dependencies lazy so synthetic lifecycle tests need no process handles.
    from resolve_camera import Resolver
    from anchor_config import LiveRegistryAnchorSampler
    profile = json.loads(Path(profile_path).read_text(encoding='utf-8'))
    resolver = Resolver(pid, profile)
    try:
        return LiveRegistryAnchorSampler(resolver)
    except BaseException:
        resolver.p.close()
        raise


class RuntimePolicy:
    """Lifetime factories must bind the original identity and return wait()/close()."""
    def __init__(self, pid, check_identity, profile_path=DEFAULT_PROFILE, *,
                 clock=time.monotonic, sampler_factory=None, policy_factory=Policy, lifetime_factory=None):
        if isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0 or not callable(check_identity):
            raise ValueError('Run-camera runtime needs a PID and process identity checker')
        self.pid, self._checker, self.profile_path = pid, check_identity, Path(profile_path)
        self.clock, self._factory = clock, sampler_factory or _sampler
        self._lifetime_factory = lifetime_factory or WindowsLifetimeProof
        self._lifetime, self._exit_confirmed = None, False
        self._retired_identity = object()  # Private proof receipt; never a current identity or JSON value.
        self.identity = copy.deepcopy(self._identity(pid))
        self._policy = policy_factory(pid, self._identity)
        if self._policy.identity != self.identity:
            self._policy.close()  # No plan or writes exist during construction.
            raise PolicyRejected('Run-camera process changed during runtime construction')
        self._sampler = None
        self._released, self._owned = True, []
        self.closed = self.stopping = self.suspended = False
        self.next_poll, self._failures = 0., 0
        self.report = dict(applied=False, released=True, closed=False, stopping=False,
                           suspended=False, status='waiting', attempts=0, acquisitions=0,
                           rebinds=0, errors=[], policy_errors=[], pending_fields=[],
                           lifetime=dict(available=False, retained=False, exit_confirmed=False, state='not_acquired'))
        self._sync()

    def _identity(self, pid):
        if self._exit_confirmed:
            return self._retired_identity
        current = self._checker(pid)
        if current is None and self._lifetime is not None:
            proof = self.report['lifetime']
            try:
                result = self._lifetime.wait()
            except ERRORS as error:
                proof.update(state='wait_error', reason=str(error))
                raise
            proof['last_wait'] = result if type(result) is int else repr(result)
            if type(result) is int and result == WAIT_OBJECT_0:
                self._exit_confirmed = True
                proof.pop('reason', None)
                proof.update(state='exited', exit_confirmed=True)
                return self._retired_identity
            proof['state'] = 'alive' if type(result) is int and result == WAIT_TIMEOUT else 'unknown'
            if proof['state'] == 'alive':
                proof.pop('reason', None)
            if proof['state'] == 'unknown':
                raise PolicyRejected('Run-camera lifetime wait did not prove exit: ' + str(result))
        if not isinstance(current, dict) or current.get('pid') != self.pid:
            # None/failed OpenProcess is unknown, not evidence of retirement.
            raise PolicyRejected('Run-camera process identity is unavailable')
        return current

    def _ensure_lifetime(self):
        if self._lifetime is None:
            lifetime = self._lifetime_factory(self.pid, copy.deepcopy(self.identity))
            if not callable(getattr(lifetime, 'wait', None)) or not callable(getattr(lifetime, 'close', None)):
                if callable(getattr(lifetime, 'close', None)):
                    lifetime.close()
                raise PolicyRejected('Run-camera lifetime factory returned an invalid proof')
            self._lifetime = lifetime
            self.report['lifetime'].update(available=True, retained=True, state='retained')
        if self._identity(self.pid) != self.identity:
            raise PolicyRejected('Run-camera process changed before lifetime-protected application')

    def _sync(self):
        source = self._policy.report
        self.report.update(copy.deepcopy({k: v for k, v in source.items() if k != 'errors'}))
        self.report['policy_errors'] = copy.deepcopy(source.get('errors', []))
        reason = source.get('last_check', {}).get('reason', '')
        if source.get('external_changes', 0) or reason.startswith('Run-camera desired field changed:'):
            self.suspended = True
            self.report['suspended_reason'] = 'External field change; session reapplication disabled'
        self.report.update(released=self._released, closed=self.closed,
                           stopping=self.stopping, suspended=self.suspended,
                           pending_fields=[] if self._released else
                           list(source.get('pending_fields') or self._owned))

    def _failure(self, stage, error):
        self.report['applied'] = False
        self._failures += 1
        item = dict(stage=stage, type=type(error).__name__, reason=str(error))
        self.report['last_error'] = item
        if len(self.report['errors']) < 16:
            self.report['errors'].append(item)
        self.next_poll = self.clock() + min(8., 2. ** min(self._failures - 1, 3))
        self.report.update(status='pending' if not self._released else 'unavailable',
                           consecutive_failures=self._failures, next_poll=self.next_poll)
        self._sync()

    def _acquire(self):
        fresh = self._factory(self.pid, self.profile_path)
        if self._sampler is not None and (fresh.r is self._sampler.r or fresh.r.p is self._sampler.r.p):
            raise PolicyRejected('Sampler factory reused the active Resolver/process reader')
        self.report['acquisitions'] += 1
        return fresh

    @staticmethod
    def _close_reader(sampler):
        if sampler is not None:
            sampler.r.p.close()

    def _apply(self, receipt):
        # Promote the reader before apply: partial failures can leave ownership.
        self.report['last'] = copy.deepcopy(receipt)
        self._released = False
        try:
            installed = self._policy.apply()
            self._owned = list(installed.get('owned_fields', []))
            self.report['last'] = installed
            self.report['applied'] = True
        except BaseException:
            self._release()
            raise
        finally:
            self._sync()

    def _release(self):
        result = self._policy.restore()
        self._released = result['released']
        self._owned = list(result['pending_fields'])
        self.report['applied'] = False
        self._sync()
        return self._released

    def _fresh(self, install):
        fresh = self._acquire()
        try:
            if self._identity(self.pid) != self.identity:
                raise PolicyRejected('Run-camera process changed during acquisition')
            if self._sampler is None:
                receipt = self._policy.prepare(fresh).export()
            else:
                # No apply inside rebind. If it fails, the policy still holds
                # its old certificate; the temporary reader can be closed.
                receipt = self._policy.rebind(fresh, apply=False)
                self.report['rebinds'] += 1
            self._sync()
            if receipt['state'] == 'prepared':
                previous, self._sampler = self._sampler, fresh
                fresh = None
                self._released, self._owned = True, []
                self._close_reader(previous)  # rebind proved the old release.
                if install and not self.suspended:
                    self._ensure_lifetime()  # Certified context first; no write without a lifetime receipt.
                    self._apply(receipt)
                else:
                    self._release()
            elif receipt['state'] == 'applied':
                # The old context recovered between check and rebind; its
                # original certificate (and reader) still owns the fields.
                self._released = False
                if install and not self.suspended:
                    self.report['applied'] = True
                else:
                    self._release()
            else:
                raise PolicyRejected('Run-camera rebind returned an unexpected plan state')
        finally:
            self._close_reader(fresh)
            self._sync()

    def _step(self):
        if (self.stopping or self.suspended) and self._released:
            return
        current = self._identity(self.pid)
        if current != self.identity:
            self.stopping = True
            self.report['stop_reason'] = 'Process identity changed; no new application permitted'
        if not (self.stopping or self.suspended) and self._sampler is not None:
            if self._policy.check():
                self.report['applied'] = True
                self._sync()
                return
            self._sync()
        if self._sampler is not None:
            self._release()
        install = not (self.stopping or self.suspended)
        if install or not self._released:
            self._fresh(install)

    def tick(self):
        if self.closed or self.clock() < self.next_poll:
            return
        self.next_poll = self.clock() + 1.
        self.report['attempts'] += 1
        try:
            self._step()
            if not self._released and not self.report['applied']:
                raise PolicyRejected('Run-camera restoration remains pending')
            self._failures = 0
            self.report.update(status='applied' if self.report['applied'] else
                               'pending' if not self._released else
                               'suspended' if self.suspended else
                               'restored' if self.stopping else 'waiting',
                               consecutive_failures=0, next_poll=self.next_poll)
        except ERRORS as error:
            self._failure('tick', error)
        finally:
            self._sync()

    def restore(self):
        """Stop applications and attempt one restore/rebind; pending retains readers."""
        if self.closed:
            return copy.deepcopy(self.report)
        self.stopping = True
        try:
            if not self._released:
                self._step()
            if not self._released:
                raise PolicyRejected('Run-camera restoration remains pending')
            self.report.update(applied=False, status='restored')
        except ERRORS as error:
            self._failure('restore', error)
        finally:
            self._sync()
        return copy.deepcopy(self.report)

    def close(self):
        """Return closed=False on unresolved responsibility; a later call may retry."""
        if not self.closed:
            self.restore()
            if self._released:
                try:
                    self._policy.close()  # Only after explicit successful release.
                    self._close_reader(self._sampler)
                    self._sampler = None
                    if self._lifetime is not None:
                        self._lifetime.close()
                        self._lifetime = None
                        self.report['lifetime'].update(retained=False, state='closed')
                    self.closed = True
                    self.report.update(applied=False, status='closed')
                except ERRORS as error:
                    self._failure('close', error)
            self._sync()
        return copy.deepcopy(self.report)
