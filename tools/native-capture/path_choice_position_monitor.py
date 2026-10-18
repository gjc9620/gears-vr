"""Owner-side bounded PathChoice placement service; worker access is read-only.

One certificate, baseline and nonce per activation. Renewal changes only the
owned files' modification times; close acknowledgement always precedes position.
This module sends no input, writes no process memory and owns no global VR stop.
"""
from dataclasses import dataclass
import copy
import ctypes
import json
from pathlib import Path
import threading
import time

from path_choice_center_monitor import _ReadWorker
from path_choice_owned_sidecar import OwnedSidecar

MAX_RESULT_AGE = 2.0
OBSERVE_INTERVAL = .5
RENEW_INTERVAL = 2.0
MAX_ACQUISITION_ATTEMPTS = 3
# Fit the complete authenticated panel content into the central vertical band.
# The collector certifies its coordinate domain; unsupported layouts reject.
TARGET_VERTICAL_REGION = (.25, .75)


def _acquisition_source_conflict(error):
    """Only an exact, complete-byte acquisition disagreement can be retried.

    This discards the entire unpublished certificate; no byte is masked and
    fixed-prefix identity/header violations keep their existing terminal path.
    """
    from path_choice_position_collect import AcquisitionSourceChanged
    if type(error) is not AcquisitionSourceChanged or error.__cause__ is not None or error.__context__ is not None:
        return False
    try:
        d=error.read_conflict;address=int(d['address'],16);size=d['size'];offset=d['changed_offset']
        expected=bytes.fromhex(d['expected_window']);actual=bytes.fromhex(d['actual_window'])
        return (d['phase'] in ('repeated_read','final_reread') and type(size) is int and 0<size<=1024 and
                d['actual_size']==size and type(offset) is int and 0<=offset<size and
                0<address<=(1<<64)-size and int(d['changed_address'],16)==address+offset and
                0<len(expected)==len(actual)<=8 and expected!=actual)
    except (AttributeError,KeyError,TypeError,ValueError):
        return False


def get_tick_count64():
    fn = ctypes.WinDLL('kernel32', use_last_error=True).GetTickCount64
    fn.restype = ctypes.c_ulonglong
    return int(fn())


@dataclass(frozen=True)
class Observation:
    scene_present: bool
    certificate: object = None
    scene_absent: bool = False
    scene_address: int = 0


class LivePathChoicePositionProbe:
    """Single worker's reader; verifier independently binds all live identities."""
    def __init__(self, game, verify_session):
        self.game, self.verify_session, self.reader = dict(game), verify_session, None

    def capture(self, nonce, acquire, generation):
        if self.reader is None:
            from menu_state import MenuReader
            self.reader = MenuReader(self.game['pid'])
        reader = self.reader
        # Acquisition must start clean; later observations may see this exact
        # activation's active lease (native pending_restore means lease.active).
        def verify(current):
            return self.verify_session(current, generation, observation_nonce=None if acquire else nonce)
        try:
            session, status = verify(reader)
            from path_choice_position_collect import sample_fixed_prefix
            sample = sample_fixed_prefix(reader) if acquire else reader.sample()
            if (sample.get('top') or {}).get('cls') != 'UIScene_PathChoice':
                verify(reader)
                absent = not any(row.get('cls') == 'UIScene_PathChoice' for row in sample['active_scenes'])
                return Observation(False, scene_absent=absent)
            address = int(sample['top']['address'], 0)
            if not acquire:
                verify(reader)
                return Observation(True, scene_address=address)
            camera = reader.r.resolve()
            sample = sample_fixed_prefix(reader, include_tree=True, include_layout_details=True, include_owner=True,
                expected_controller=int(camera['controller_address'], 16),
                expected_local_player=int(camera['local_player_address'], 16))
            verify(reader)
            from path_choice_position_collect import collect
            certificate = collect(reader, sample, status, session, nonce,
                lambda current: verify(current)[0], registry_header_policy='fixed_prefix_v1',
                target_vertical_region=TARGET_VERTICAL_REGION)
            verify(reader)
            return Observation(True, certificate, scene_address=address)
        except Exception:
            self.close()
            raise

    def close(self):
        if self.reader is not None:
            self.reader.close(); self.reader = None


class PathChoicePositionMonitor:
    def __init__(self, position_path, close_path, session, read_status, identity, *,
                 clock=time.monotonic, tick_clock=get_tick_count64,
                 nonce_clock=time.perf_counter_ns, probe_factory=LivePathChoicePositionProbe,
                 sidecar_factory=OwnedSidecar, worker_factory=_ReadWorker):
        self.session = copy.deepcopy(session)
        self.game = self.session['game_identity']; self.owner = self.session['owner_identity']
        self.started_qpc = int(self.session['native_started_qpc'])
        if self.session['phase'] != 'running' or self.started_qpc <= 0:
            raise ValueError('Fixed running native session required')
        if Path(position_path).absolute() == Path(close_path).absolute():
            raise ValueError('Position and close paths must differ')
        self.read_status, self.identity = read_status, identity
        self.clock, self.tick_clock, self.nonce_clock = clock, tick_clock, nonce_clock
        self.position = sidecar_factory(position_path); self.close_sidecar = sidecar_factory(close_path)
        self.cancel = threading.Event()
        self.probe = probe_factory(self.game, self.verify_session)
        self.executor = worker_factory(self.probe)
        self.future = None; self.request_generation = None
        self.state = 'idle'; self.generation = None; self.nonce = 0
        self.certificate = None; self.baseline_ids = None
        self.certificate_finished = None
        self.acquisition_attempts = 0
        self.position_bytes = self.close_bytes = None
        self.last_restored_nonce = 0; self.leave_seen = False
        self.last_observed = None; self.due = 0.; self.renew_due = 0.
        self.renew_phase = None; self.renew_receipt = None; self.renew_tick = None
        self.ack_deadline = None; self.terminal = False; self.closed = False
        self.cleanup_error = None; self.worker_joined = None
        self.owner_thread = threading.get_ident()

    def _owner(self):
        if threading.get_ident() != self.owner_thread:
            raise RuntimeError('Only the owner thread may publish PathChoice requests')

    def _check_processes(self):
        if self.cancel.is_set():
            raise RuntimeError('PathChoice monitor cancelled')
        if any(self.identity(value['pid']) != value for value in (self.game, self.owner)):
            raise RuntimeError('PathChoice game/owner identity changed')

    def _status(self, status):
        if not isinstance(status, dict):
            raise ValueError('Missing native status')
        for key, value in (('phase', 'running'), ('pid', self.game['pid']),
                ('owner_pid', self.owner['pid']), ('owner_creation_time', self.owner['creation_time']),
                ('started_qpc', self.started_qpc), ('path_position_service_compiled', 1)):
            if str(status.get(key)) != str(value):
                raise ValueError('Native identity, phase or compiled support mismatch: ' + key)
        if int(status['world_generation']) <= 0:
            raise ValueError('Native world is not current')
        return int(status['world_generation'])

    def verify_session(self, reader, generation, *, observation_nonce=None):
        """Require clean acquisition, or the exact certified lease for observation."""
        self._check_processes()
        from menu_control import reader_process_identity
        if reader_process_identity(reader) != (self.game['pid'], self.game['creation_time']):
            raise RuntimeError('Reader process handle identity changed')
        status = copy.deepcopy(self.read_status())
        if self._status(status) != generation:
            raise RuntimeError('Native world changed during read')
        pending = str(status.get('path_position_pending_restore'))
        expected_held = (observation_nonce is not None and observation_nonce == self.nonce and
                         self.nonce > 0 and self.certificate is not None and self.close_sidecar.owned and
                         self._loaded(status, 'path_position_') and self._applied(status))
        if self._failure(status) or not (pending == '0' or (pending == '1' and expected_held)):
            raise RuntimeError('Native position state changed during read')
        return copy.deepcopy(self.session), status

    @staticmethod
    def _receipt(status, prefix):
        return tuple(int(status[prefix + key]) for key in ('renewals', 'accepted_tick', 'expires_tick'))

    def _loaded(self, status, prefix):
        return int(status.get(prefix + 'nonce', 0)) == self.nonce

    def _live_receipt(self, status, prefix, duration):
        renewals, accepted, expires = self._receipt(status, prefix)
        now = self.tick_clock()
        return renewals >= 0 and 0 < accepted <= now < expires and expires - accepted == duration

    def _applied(self, status):
        return (int(status.get('path_position_active_nonce', 0)) == self.nonce and
                int(status.get('path_position_consumed_nonce', 0)) == self.nonce and
                int(status.get('path_position_last_restored_nonce', 0)) < self.nonce)

    def _failure(self, status):
        # Missing safety fields are failures, not implicit clean zeroes.
        for key in ('identity_lost', 'ownership_lost', 'setter_failed', 'unresolved'):
            if str(status.get('path_position_' + key)) != '0':
                return True
        return any(int(status.get(key, 0)) != 0 for key in
                   ('path_position_rejected', 'path_close_load_rejected', 'path_position_service_terminal'))

    def _remove_position(self):
        try:
            self.position.remove()
            return True
        except Exception as error:
            self.cleanup_error = repr(error)
            return False

    def _revoke(self, reason, *, terminal=False):
        self._remove_position()
        self.renew_phase = None
        self.terminal = self.terminal or terminal
        self.state = 'await_restore' if self.close_sidecar.owned else 'latched_closed'
        return dict(event='path_position_revoked', reason=reason, nonce=self.nonce)

    @staticmethod
    def _exception_detail(error):
        # Keep the exact read rejection diagnosable without dumping samples,
        # process memory, source lines, or an unbounded exception traceback.
        frames = []
        trace = error.__traceback__
        while trace is not None:
            code = trace.tb_frame.f_code
            frames.append(dict(file=Path(code.co_filename).name[:128],
                               function=code.co_name[:64], line=trace.tb_lineno))
            frames = frames[-4:]
            trace = trace.tb_next
        causes = []
        current = error
        for _ in range(2):
            current = current.__cause__ or (None if current.__suppress_context__ else current.__context__)
            if current is None or current is error:
                break
            causes.append(dict(type=type(current).__name__[:64], message=str(current)[:256]))
        detail = dict(error_message=str(error)[:512], error_frames=frames, error_causes=causes)
        from path_choice_position_collect import AcquisitionSourceChanged
        import menu_state
        import scene_owner_snapshot
        current = error
        for _ in range(3):
            if (type(current) in (AcquisitionSourceChanged, getattr(menu_state, "SnapshotChanged", None),
                                  getattr(menu_state, "RegistrySnapshotChanged", None),
                                  getattr(menu_state, "ForcedRegistryRefreshChanged", None),
                                  getattr(scene_owner_snapshot, "SceneOwnerRejected", None))
                    and type(getattr(current, "read_conflict", None)) is dict):
                detail['error_read_conflict'] = copy.deepcopy(current.read_conflict)
                break
            current = current.__cause__ or (None if current.__suppress_context__ else current.__context__)
            if current is None or current is error: break
        return detail

    def _restored(self, status):
        restored = int(status.get('path_position_last_restored_nonce', 0))
        if (not self.nonce or self.last_restored_nonce == self.nonce or restored != self.nonce or
                int(status.get('path_position_last_restored_tick', 0)) <= 0 or
                int(status.get('path_position_active_nonce', -1)) != 0 or
                str(status.get('path_position_pending_restore')) != '0' or
                str(status.get('path_position_unresolved')) != '0'):
            return False
        self.last_restored_nonce = self.nonce
        if not self._remove_position():
            self.terminal = True
        try:
            self.close_sidecar.remove()
        except Exception as error:
            self.cleanup_error = repr(error); self.terminal = True
        self.state = 'latched_closed'; self.renew_phase = None
        return True

    def _validate_certificate(self, certificate):
        if certificate is None:
            raise ValueError('Missing activation certificate')
        baseline = json.loads(certificate.baseline_ids_json)
        if int(baseline['nonce']) != self.nonce or len(baseline['nodes']) != 13:
            raise ValueError('Activation baseline nonce/node count mismatch')
        for role in ('scene', 'client', 'controller', 'local_player'):
            obj = baseline['roles'][role]
            if int(obj['address']) <= 0 or int(obj['internal_index']) < 0 or int(obj['cls']) <= 0:
                raise ValueError('Activation requires fresh positive owner identities')
        # Collector returns immutable strings. Never rebuild them from held state.
        position_raw = certificate.position_ini.encode('ascii')
        close_raw = certificate.close_ini.encode('ascii')
        if not position_raw or not close_raw:
            raise ValueError('Empty activation sidecar')
        self.certificate, self.baseline_ids = certificate, baseline
        self.position_bytes, self.close_bytes = position_raw, close_raw
        return position_raw, close_raw

    def _fresh_certificate(self):
        if self.certificate_finished is None or not 0 <= self.clock() - self.certificate_finished <= MAX_RESULT_AGE:
            raise ValueError('Activation certificate exceeded its completed-read budget')

    def _submit(self, generation, acquire):
        nonce = self.nonce
        def capture():
            value = self.probe.capture(nonce, acquire, generation)
            # Capture time is owned by this wrapper, not evidence-provided data.
            return value, self.clock()
        self.request_generation = generation
        self.future = self.executor.submit(capture)
        if acquire:
            self.acquisition_attempts += 1
            self.state = 'acquiring'

    def _retry_unowned_snapshot(self, error, generation):
        if (self.state != 'acquiring' or self.terminal or self.cancel.is_set() or
                self.request_generation != generation or self.generation != generation or
                self.acquisition_attempts >= MAX_ACQUISITION_ATTEMPTS or
                self.position.owned or self.close_sidecar.owned or self.certificate is not None or
                self.baseline_ids is not None or self.position_bytes is not None or self.close_bytes is not None):
            return False
        try:
            from menu_state import SnapshotChanged
            from scene_owner_snapshot import SceneOwnerRejected
        except ImportError:
            return False
        # Discard only demonstrated pre-publication read conflicts. A registry
        # mismatch requires a NEW reader/full certificate, never relaxed bytes.
        try:
            from menu_state import RegistrySnapshotChanged
        except ImportError:
            RegistrySnapshotChanged = None
        registry_conflict = (RegistrySnapshotChanged is not None and
            type(error) is RegistrySnapshotChanged and
            str(error) == 'Registry changed before snapshot verification' and
            error.__cause__ is None and error.__context__ is None)
        ledger_conflict = (type(error) is SceneOwnerRejected and str(error) == 'Owner source read failed' and
                type(error.__cause__) is SnapshotChanged and
                str(error.__cause__) == 'UI changed during snapshot' and
                error.__cause__.__cause__ is None and error.__cause__.__context__ is None)
        if not (registry_conflict or ledger_conflict or _acquisition_source_conflict(error)):
            return False
        self._check_processes()
        status = copy.deepcopy(self.read_status())
        if self._status(status) != generation:
            raise RuntimeError('Native world changed before snapshot retry')
        if (self._failure(status) or str(status.get('path_position_pending_restore')) != '0' or
                str(status.get('path_position_active_nonce')) != '0'):
            raise RuntimeError('Native position state changed before snapshot retry')
        # LivePathChoicePositionProbe closed its failed reader on the worker;
        # the next full capture independently verifies a newly opened reader.
        return True

    def _renew_ack(self, status, prefix, duration):
        current = self._receipt(status, prefix)
        previous = self.renew_receipt
        return (self._loaded(status, prefix) and self._live_receipt(status, prefix, duration) and
                current[0] > previous[0] and current[1] >= self.renew_tick and
                current[2] > previous[2] and current[2] >= self.renew_tick + duration - 200)

    def poll(self, status=None):
        self._owner()
        if self.closed:
            return []
        events = []
        try:
            self._check_processes()
            # Never allow a cached runner argument to authenticate an activation.
            status = copy.deepcopy(self.read_status())
            generation = self._status(status)
            if self.generation is None:
                self.generation = generation
            if generation != self.generation:
                self.generation = generation
                self.last_observed = None
                events.append(self._revoke('world_changed'))
            if self._failure(status):
                events.append(self._revoke('native_rejection', terminal=True))
            if self._restored(status) and self.state == 'latched_closed':
                # Restore alone cannot authorize recapturing the same open scene.
                pass
            now = self.clock()
            if self.future is not None and self.future.done():
                future, self.future = self.future, None
                retry_acquisition = False
                try:
                    observation, finished = future.result()
                except Exception as error:
                    if not self._retry_unowned_snapshot(error, generation):
                        raise
                    event = dict(event='path_position_acquisition_discarded', reason=type(error).__name__,
                                 nonce=self.nonce, error_state=self.state,
                                 acquisition_attempt=self.acquisition_attempts,
                                 retry_count=self.acquisition_attempts)
                    event.update(self._exception_detail(error))
                    events.append(event)
                    retry_acquisition = True
                if retry_acquisition:
                    self._submit(generation, True)
                    return events
                # The worker can finish between the earlier clock sample and
                # done(). Measure age only after obtaining that finished result.
                now = self.clock()
                age = now - finished;cancelled = self.cancel.is_set()
                if (self.request_generation != generation or not 0 <= age <= MAX_RESULT_AGE or cancelled):
                    event = self._revoke('late_or_world_changed_read')
                    event.update(age_seconds=age,request_generation=self.request_generation,
                                 current_generation=generation,cancelled=cancelled)
                    events.append(event)
                else:
                    self.last_observed = finished
                    if not observation.scene_present:
                        self.leave_seen = self.leave_seen or observation.scene_absent
                        if self.state in ('acquiring', 'await_close_ack', 'await_apply', 'holding'):
                            events.append(self._revoke('scene_left_or_overlay'))
                        if not self.close_sidecar.owned and not self.position.owned:
                            self.state = 'idle'
                    elif self.state == 'acquiring' and not self.terminal:
                        position_raw, close_raw = self._validate_certificate(observation.certificate)
                        self.certificate_finished = finished
                        self._check_processes()
                        if self._status(self.read_status()) != generation:
                            raise RuntimeError('Native world changed before publication')
                        self._fresh_certificate()
                        self.close_sidecar.publish(self.close_bytes)
                        self.state = 'await_close_ack'; self.ack_deadline = now + MAX_RESULT_AGE
                        events.append(dict(event='path_position_close_published', nonce=self.nonce))
                    elif self.state == 'latched_closed' and self.leave_seen and not self.terminal:
                        if not self.close_sidecar.owned and not self.position.owned:
                            self.state = 'idle'
                    elif self.state in ('await_close_ack', 'await_apply', 'holding'):
                        if observation.scene_address != int(self.baseline_ids['roles']['scene']['address']):
                            events.append(self._revoke('scene_identity_changed'))
                    self.due = now + OBSERVE_INTERVAL
            if self.state in ('await_close_ack', 'await_apply', 'holding'):
                if self.last_observed is None or now - self.last_observed > MAX_RESULT_AGE:
                    events.append(self._revoke('observer_stale'))
                elif self.state == 'await_close_ack':
                    if self._loaded(status, 'path_close_') and self._live_receipt(status, 'path_close_', 8000):
                        self._fresh_certificate()
                        self.position.publish(self.position_bytes)
                        self.state = 'await_apply'; self.ack_deadline = now + MAX_RESULT_AGE
                        events.append(dict(event='path_position_published', nonce=self.nonce))
                    elif now > self.ack_deadline:
                        events.append(self._revoke('close_ack_timeout', terminal=True))
                elif self.state == 'await_apply':
                    if self._applied(status) and self._live_receipt(status, 'path_position_', 6000):
                        self.state = 'holding'; self.renew_due = now + RENEW_INTERVAL
                        events.append(dict(event='path_position_holding', nonce=self.nonce))
                    elif now > self.ack_deadline:
                        events.append(self._revoke('apply_ack_timeout', terminal=True))
                elif not self._applied(status) or not all((self._live_receipt(status, 'path_close_', 8000),
                                                           self._live_receipt(status, 'path_position_', 6000))):
                    events.append(self._revoke('native_lease_expired_or_inactive', terminal=True))
                elif self.renew_phase == 'close':
                    if self._renew_ack(status, 'path_close_', 8000):
                        self.renew_receipt = self._receipt(status, 'path_position_')
                        self.renew_tick = self.tick_clock(); self.position.renew()
                        self.renew_phase = 'position'; self.ack_deadline = now + MAX_RESULT_AGE
                    elif now > self.ack_deadline:
                        events.append(self._revoke('close_renew_ack_timeout', terminal=True))
                elif self.renew_phase == 'position':
                    if self._renew_ack(status, 'path_position_', 6000):
                        self.renew_phase = None; self.renew_due = now + RENEW_INTERVAL
                        events.append(dict(event='path_position_renewed', nonce=self.nonce))
                    elif now > self.ack_deadline:
                        events.append(self._revoke('position_renew_ack_timeout', terminal=True))
                elif now >= self.renew_due:
                    self.renew_receipt = self._receipt(status, 'path_close_')
                    self.renew_tick = self.tick_clock(); self.close_sidecar.renew()
                    self.renew_phase = 'close'; self.ack_deadline = now + MAX_RESULT_AGE
            if self.future is None and now >= self.due and not self.terminal:
                acquire = self.state == 'idle' and not self.close_sidecar.owned and not self.position.owned
                if acquire:
                    self.nonce = max(self.nonce + 1, self.last_restored_nonce + 1, int(self.nonce_clock()))
                    self.leave_seen = False; self.certificate = self.baseline_ids = None
                    self.position_bytes = self.close_bytes = None
                    self.acquisition_attempts = 0
                self._submit(generation, acquire)
        except Exception as error:
            detail = self._exception_detail(error)
            detail['error_state'] = self.state
            detail['acquisition_attempt'] = self.acquisition_attempts
            detail['retry_count'] = max(0, self.acquisition_attempts - 1)
            event = self._revoke(type(error).__name__, terminal=True)
            event.update(detail)
            events.append(event)
        return events

    def close(self):
        self._owner()
        if self.closed:
            return
        self.closed = True; self.cancel.set()
        self._remove_position()
        self.state = 'await_restore' if self.close_sidecar.owned else 'latched_closed'
        if self.future is not None:
            self.future.cancel()
        self.worker_joined = self.executor.close()
        if self.executor.error:
            self.cleanup_error = self.executor.error

    def finish_cleanup(self, native_cleanup_verified=False, entry_restore_verified=False, game_absent=False):
        """Caller independently verifies these proofs after native/XR teardown."""
        self._owner()
        if not self.closed:
            raise RuntimeError('Close monitor before final cleanup')
        if not (game_absent is True or
                (native_cleanup_verified is True and entry_restore_verified is True)):
            return not (self.position.owned or self.close_sidecar.owned)
        result = self._remove_position()
        try:
            self.close_sidecar.remove()
        except Exception as error:
            self.cleanup_error = repr(error); result = False
        return result
