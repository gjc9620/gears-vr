"""Owned-game input-axis driver; writes one reflected float, then neutralizes it."""
import ctypes as c
from ctypes import wintypes as w
import math
from pathlib import Path
import struct
import time

from core import controllable, yaw_delta
from world_state import Query


class AxisNeutralizationFailed(RuntimeError):
    """The owned input slot could not be certified neutral after a write."""


class AxisReadInterrupted(RuntimeError):
    def __init__(self, reason, receipt):
        super().__init__(reason)
        self.receipt = receipt


class PrivateWriter:
    def __init__(self, identity):
        self.identity = identity
        self.k = c.WinDLL('kernel32', use_last_error=True)
        for name, args, result in (
            ('OpenProcess', [w.DWORD, w.BOOL, w.DWORD], w.HANDLE),
            ('CloseHandle', [w.HANDLE], w.BOOL),
            ('GetExitCodeProcess', [w.HANDLE, c.POINTER(w.DWORD)], w.BOOL),
            ('GetProcessTimes', [w.HANDLE, *([c.POINTER(w.FILETIME)] * 4)], w.BOOL),
            ('QueryFullProcessImageNameW', [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)], w.BOOL),
            ('WriteProcessMemory', [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)], w.BOOL)):
            function = getattr(self.k, name)
            function.argtypes, function.restype = args, result
        self.handle = self.k.OpenProcess(0x1438, False, identity['pid'])
        if not self.handle:
            raise c.WinError(c.get_last_error())
        try:
            self.verify()
        except BaseException:
            self.close()
            raise

    def verify(self):
        times = [w.FILETIME() for _ in range(4)]
        code = w.DWORD()
        path, length = c.create_unicode_buffer(32768), w.DWORD(32768)
        if (not self.k.GetExitCodeProcess(self.handle, c.byref(code)) or code.value != 259 or
                not self.k.GetProcessTimes(self.handle, *(c.byref(item) for item in times)) or
                not self.k.QueryFullProcessImageNameW(self.handle, 0, path, c.byref(length))):
            raise RuntimeError('Axis writer process is no longer live/queryable')
        created = times[0].dwLowDateTime | times[0].dwHighDateTime << 32
        if created != self.identity['creation_time'] or str(Path(path.value).resolve()).casefold() != self.identity['image']:
            raise RuntimeError('Axis writer handle identity mismatch')

    def write(self, address, raw):
        from runtime_probe import MBI
        self.verify()
        if address % 4 or len(raw) != 4:
            raise ValueError('Private data write must be aligned and four bytes')
        self.k.VirtualQueryEx.argtypes = [w.HANDLE, c.c_void_p, c.POINTER(MBI), c.c_size_t]
        self.k.VirtualQueryEx.restype = c.c_size_t
        region = MBI()
        if (not self.k.VirtualQueryEx(self.handle, address, c.byref(region), c.sizeof(region)) or
                region.State != 0x1000 or region.Type != 0x20000 or region.Protect not in (4, 8) or
                not (region.BaseAddress <= address <= region.BaseAddress + region.RegionSize - 4)):
            raise RuntimeError('Axis slot is not committed writable private data')
        count = c.c_size_t()
        data = c.create_string_buffer(raw, 4)
        if not self.k.WriteProcessMemory(self.handle, address, data, 4, c.byref(count)) or count.value != 4:
            raise c.WinError(c.get_last_error())

    def close(self):
        if self.handle:
            if not self.k.CloseHandle(self.handle):
                raise c.WinError(c.get_last_error())
            self.handle = None


class FloatWriter(PrivateWriter):
    def write(self, address, raw):
        if len(raw) != 4 or not math.isfinite(struct.unpack('<f', raw)[0]):
            raise ValueError('Axis write must be a finite float32')
        super().write(address, raw)


class Slot:
    def __init__(self, observer, axis):
        self.observer, self.axis = observer, axis
        self.component = 1 if axis in ('aTurn', 'aMouseX') else 0
        self.q = Query(observer)
        q = self.q
        self.controller = q.obj(observer.sampler.controller)
        self.pawn = q.pointer(self.controller, 'Pawn')
        self.input = q.pointer(self.controller, 'PlayerInput')
        if (self.pawn != q.obj(observer.sampler.pawn) or not self.input or
                self.input['outer'] != self.controller['address'] or not q.s.derives(self.input['cls'], 'PlayerInput')):
            raise RuntimeError('Axis input/controller/pawn ownership mismatch')
        self.prop = q.prop(self.input, axis, 'FloatProperty')
        if self.prop['size'] != 4:
            raise RuntimeError('Axis field is not scalar float32')
        self.address = self.input['address'] + self.prop['offset']
        self.original = q.value(self.input, self.prop)
        if struct.unpack('<f', self.original)[0] != 0:
            raise RuntimeError('Axis input is not neutral before ownership')
        rotation = q.prop(self.controller, 'Rotation', 'StructProperty')
        typ = q.obj(q.r.struct_type(rotation))
        if rotation['size'] != 12 or typ['name'] != 'Rotator':
            raise RuntimeError('Controller rotation layout is unsupported')
        for name, offset in (('Pitch', 0), ('Yaw', 4), ('Roll', 8)):
            field = q.prop(typ, name, 'IntProperty', structure=True)
            if field['size'] != 4 or field['offset'] != offset:
                raise RuntimeError('Controller rotation component layout is unsupported')
        self.rotation = rotation
        self.rotation_type = typ
        self.components = {name: q.prop(typ, name, 'IntProperty', structure=True) for name in ('Pitch', 'Yaw', 'Roll')}
        self.initial_pitch = self.pitch()
        self.initial_angle = self.angle()
        q.finish(allow_registry_growth=True)

    def validate(self, *, neutral=False):
        # Control polls and neutralization have independent registry brackets.
        # Irrelevant registry growth does not invalidate this selected slot.
        if self.observer.resolver is None or self.observer.sampler is None:
            if neutral:
                # An observation can close its readers after registry churn.
                # Reacquire once for release, then require the exact original
                # controller, input object and property schema below.
                self.observer.sample()
            if self.observer.resolver is None or self.observer.sampler is None:
                raise RuntimeError('Axis reader certificate is no longer available')
        q = Query(self.observer)
        self.observer.verify()
        if (q.obj(self.controller['address']) != self.controller or q.obj(self.input['address']) != self.input or
                q.pointer(self.controller, 'PlayerInput') != self.input or
                q.prop(self.input, self.axis, 'FloatProperty') != self.prop or
                q.prop(self.controller, 'Rotation', 'StructProperty') != self.rotation or
                q.obj(q.r.struct_type(self.rotation)) != self.rotation_type or
                any(q.prop(self.rotation_type, name, 'IntProperty', structure=True) != field
                    for name, field in self.components.items()) or
                (not neutral and q.pointer(self.controller, 'Pawn') != self.pawn)):
            raise RuntimeError('Axis slot identity or schema changed')
        q.finish(allow_registry_growth=True)
        self.q = q  # Readback must use the freshly certified live reader too.

    def pitch(self):
        return struct.unpack('<3i', self.q.value(self.controller, self.rotation))[0]

    def angle(self):
        return struct.unpack('<3i', self.q.value(self.controller, self.rotation))[self.component]

    def raw(self):
        return self.q.value(self.input, self.prop)


def pulse(observer, axis, value, seconds, *, emit=lambda kind, **row: None,
          stop_requested=lambda: False, writer_factory=FloatWriter, slot_factory=Slot, target_degrees=None,
          displacement_limit=64):
    from live import InputNotReady, InputInterrupted, SessionChanged, background_input
    if observer.action_owner != observer.identities['owner_identity']:
        raise RuntimeError('Axis input requires this run\'s owned launcher')
    translation = axis == 'aBaseY'
    if axis not in ('aLookUp', 'aMouseY', 'aTurn', 'aMouseX', 'aBaseY') or not math.isfinite(value) or not 0 < abs(value) <= 1 or not .02 <= seconds <= .25:
        raise ValueError('Unsupported bounded input-axis request')
    if translation and target_degrees is not None:
        raise ValueError('Translation input cannot have an angular target')
    if (type(displacement_limit) not in (int, float) or not math.isfinite(displacement_limit)
            or not 1 <= displacement_limit <= 64 or (not translation and displacement_limit != 64)):
        raise ValueError('Translation displacement limit must stay within 1..64 units')
    limit = 45 if axis in ('aTurn', 'aMouseX') else 10
    if target_degrees is not None and (not math.isfinite(target_degrees) or not 0 < abs(target_degrees) <= limit):
        raise ValueError('Requested angular target is outside the bounded look-axis range')
    angular_limit = min(limit, abs(target_degrees)) if target_degrees is not None else limit
    before = observer.sample()
    if before['errors'] or not controllable(before):
        raise InputNotReady('Look-axis input requires current controllable state')
    initial_position = (before.get('pose') or {}).get('position')
    if translation and (not initial_position or len(initial_position) != 3 or
                        not all(math.isfinite(v) for v in initial_position)):
        raise InputNotReady('Translation input requires a finite current position')
    identity = observer.identities['game_identity']
    background = background_input.change(identity['pid'], 'status', expected_creation=identity['creation_time'])
    if background['state'] != 'original':
        raise RuntimeError('Physical background controller input must remain disabled')
    window = observer.backend.game_window(identity['pid'])
    desktop = observer.desktop()
    if desktop['occlusion'] or desktop['foreground'] == window:
        raise InputNotReady('Look-axis window is foreground or occludes another application')
    if stop_requested() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
        raise InputNotReady('Look-axis input cancelled before dispatch')
    try:
        slot = slot_factory(observer, axis)
    except SessionChanged:
        raise
    except (OSError, RuntimeError, ValueError) as error:
        raise InputNotReady('Axis slot not admitted: ' + str(error)) from error
    writer = writer_factory(identity)
    written, writes, neutral, safety_stop = False, 0, False, False
    read_interruption = None
    started = time.monotonic()
    next_control_poll = 0
    try:
        emit('axis_input_started', axis=axis, value=value, seconds=seconds, address=hex(slot.address),
             input=slot.input, controller=slot.controller, property=slot.prop, backend='reflected_input_float')
        while time.monotonic() - started < seconds:
            if not translation or time.monotonic() >= next_control_poll:
                try:
                    slot.validate()
                    current_desktop = observer.desktop()
                    current_sample = observer.sample()
                    angle = slot.angle()
                except SessionChanged:
                    raise
                except (OSError, RuntimeError, ValueError) as error:
                    read_interruption = str(error)
                    break
                # aBaseY is consumed by the input update. Keep a short train of
                # writes between expensive observations; every native write
                # still verifies process identity and the private writable page.
                next_control_poll = time.monotonic() + .04
            if stop_requested() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
                raise InputInterrupted('Look-axis input cancelled during pulse')
            if (current_desktop['occlusion'] or current_desktop['foreground'] == window
                    or current_sample['errors'] or not controllable(current_sample)
                    or current_sample.get('pawn_identity') != before.get('pawn_identity')
                    or current_sample.get('native', {}).get('world_generation') != before.get('native', {}).get('world_generation')):
                raise InputInterrupted('Look-axis control context changed during pulse')
            if translation:
                position = (current_sample.get('pose') or {}).get('position')
                if not position or len(position) != 3 or not all(math.isfinite(v) for v in position):
                    raise InputInterrupted('Translation position became unavailable')
                reached_limit = math.dist(initial_position, position) >= displacement_limit
            else:
                reached_limit = abs(yaw_delta(slot.initial_angle, angle) * 360 / 65536) >= angular_limit
            if reached_limit:
                safety_stop = True
                break
            written = True  # A partial native write also requires neutralization.
            writer.write(slot.address, struct.pack('<f', value))
            writes += 1
            time.sleep(.001 if translation else .01)
    finally:
        try:
            if written:
                try:
                    slot.validate(neutral=True)
                    writer.write(slot.address, slot.original)
                    neutral = struct.unpack('<f', slot.raw())[0] == 0
                    emit('axis_input_neutral', axis=axis, address=hex(slot.address), neutral_verified=neutral,
                         writes=writes, elapsed=time.monotonic() - started)
                    if not neutral:
                        raise RuntimeError('Look-axis neutral readback was not verified')
                except (OSError, RuntimeError, ValueError) as error:
                    emit('axis_neutral_unavailable', axis=axis, reason=str(error), writes=writes)
                    raise AxisNeutralizationFailed(str(error)) from error
        finally:
            writer.close()
            if not written:
                emit('axis_input_no_write', axis=axis, address=hex(slot.address), writes=0,
                     writer_closed=True, elapsed=time.monotonic() - started,
                     scope='Input cancelled or state rejected before the first native write')
    receipt = dict(axis=axis, value=value, seconds=seconds, actual_hold_seconds=time.monotonic() - started,
                   writes=writes, release_posted=neutral, neutral_verified=neutral, safety_stop=safety_stop,
                   backend='reflected_input_float', initial_pitch=slot.initial_pitch,
                   initial_angle=slot.initial_angle, target_degrees=target_degrees, writer_closed=True)
    if translation:
        receipt.update(initial_position=initial_position, displacement_limit_units=displacement_limit)
    if read_interruption:
        emit('axis_read_interrupted', reason=read_interruption, receipt=receipt)
        raise AxisReadInterrupted(read_interruption, receipt)
    return receipt


def calibrate_component(observer, report, role, axes):
    from scenarios import wait_state, playable
    from live import InputNotReady
    component = 0 if role == 'pitch' else 1
    metric, neutral_metric = ('pitch_degrees', 'neutral_pitch_drift') if role == 'pitch' else ('yaw_degrees', 'neutral_yaw_drift')
    requirement = 'vertical_aim' if role == 'pitch' else 'horizontal_aim'
    minimum_speed = 15 if role == 'pitch' else 60
    best = None
    for axis in axes:
        for value in (.05, .2, 1.0):
            baseline = wait_state(observer, report, playable)
            time.sleep(.2)
            before = wait_state(observer, report, playable)
            baseline_delta = yaw_delta(baseline['pose']['controller_rotation_ru'][component],
                                       before['pose']['controller_rotation_ru'][component]) * 360 / 65536
            if abs(baseline_delta) > .1:
                report.event('axis_calibration_baseline_unstable', axis=axis, pitch_degrees=baseline_delta)
                break
            try:
                try:
                    receipt = pulse(observer, axis, value, .15, emit=report.event,
                                    stop_requested=lambda: (report.directory / 'stop').exists())
                except AxisReadInterrupted as error:
                    if error.receipt['writes'] > 0 and not error.receipt['neutral_verified']:
                        raise
                    report.event('axis_calibration_retry', axis=axis, reason=str(error), receipt=error.receipt)
                    time.sleep(.2)
                    before = wait_state(observer, report, playable)
                    receipt = pulse(observer, axis, value, .15, emit=report.event,
                                    stop_requested=lambda: (report.directory / 'stop').exists())
            except InputNotReady as error:
                report.event('axis_calibration_not_dispatched', axis=axis, reason=str(error))
                break
            except AxisReadInterrupted as error:
                report.event('axis_calibration_rejected', axis=axis, reason=str(error), receipt=error.receipt)
                break
            time.sleep(.2)
            after = wait_state(observer, report, playable)
            time.sleep(.2)
            neutral = wait_state(observer, report, playable)
            result = report.action('look_' + role, before, after, dispatched=receipt['release_posted'],
                                   neutral=neutral, detail=receipt)
            delta = result.get(metric, 0)
            rate = delta / (value * receipt['actual_hold_seconds'])
            report.event('axis_calibration_attempt', axis=axis, value=value, receipt=receipt,
                         role=role, angular_degrees=delta, degrees_per_second_per_unit=rate)
            neutral_drift = result.get(neutral_metric)
            if (result['result'] == 'observed' and receipt['neutral_verified'] and neutral_drift is not None
                    and abs(neutral_drift) <= .25 and .1 <= abs(rate) <= 4000):
                best = dict(axis=axis, rate=rate, value=value, role=role,
                            source_action=result['action_id'], backend='reflected_input_float')
                if abs(rate * value) >= minimum_speed or value == 1:
                    break
        if best:
            break
    if best and abs(best['rate'] * best['value']) < 5:
        report.event('axis_calibration_too_slow', **best)
        best = None
    if best:
        report.cover(requirement, 'passed', **best,
                     scope='Owned reflected input float and controller/camera angular response; hardware input untested')
    else:
        report.cover(requirement, 'blocked', reason='Bounded neutralized look-axis pulses produced no accepted angular calibration')
    return best


def calibrate(observer, report):
    report.assisted = True
    return dict(pitch=calibrate_component(observer, report, 'pitch', ('aLookUp', 'aMouseY')),
                yaw=calibrate_component(observer, report, 'yaw', ('aTurn', 'aMouseX')))
