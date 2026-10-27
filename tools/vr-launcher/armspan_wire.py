"""Read-only x64 GVC3/Stereo-v4 adapter for candidate startup calibration.

Both mappings are copied under their existing mutex. Exact ticket, process
creation, publisher identity, QPC frequency, version, bounds and freshness are
checked before a physical-source sample reaches the measurement core.
"""
import ctypes as C
import math
from armspan_calibration import Sample


U32, U64, I64 = C.c_uint32, C.c_uint64, C.c_int64
class Vector(C.Structure):
    _fields_ = [('x', C.c_double), ('y', C.c_double), ('z', C.c_double)]
class Quaternion(C.Structure):
    _fields_ = [('x', C.c_double), ('y', C.c_double), ('z', C.c_double), ('w', C.c_double)]
class Pose(C.Structure):
    _fields_ = [('position', Vector), ('orientation', Quaternion)]
class Fov(C.Structure):
    _fields_ = [(key, C.c_double) for key in ('left', 'right', 'up', 'down')]
class Eye(C.Structure):
    _fields_ = [('pose', Pose), ('fov', Fov)]
class Stereo(C.Structure):
    _fields_ = [('source', U32), ('session', U64), ('sequence', U64), ('received_ns', I64),
                ('predicted_time', I64), ('head_flags', U64), ('view_flags', U64),
                ('focused', U32), ('should_render', U32), ('head', Pose), ('eyes', Eye * 2),
                ('space_epoch', U64), ('visible', U32)]
class StereoPacket(C.Structure):
    _fields_ = [('magic', U32), ('version', U32), ('size', U32), ('pid', U32), ('frequency', I64), ('sample', Stereo)]
class Hand(C.Structure):
    _fields_ = [('grip', Pose), ('aim', Pose), ('grip_flags', U64), ('aim_flags', U64),
                ('grip_active', U32), ('aim_active', U32), ('buttons', U32), ('buttons_active', U32),
                ('trigger', C.c_float), ('squeeze', C.c_float), ('stick_x', C.c_float), ('stick_y', C.c_float),
                ('trigger_active', U32), ('squeeze_active', U32), ('stick_active', U32),
                ('pressed', U32 * 7), ('released', U32 * 7), ('last_squeeze_press_grip', Pose),
                ('last_squeeze_press_flags', U64), ('last_squeeze_press_sequence', U64)]
class Input(C.Structure):
    _fields_ = [('source', U32), ('session', U64), ('sequence', U64), ('space_epoch', U64),
                ('profile', U64), ('input_epoch', U64), ('predicted_time', I64), ('received_ns', I64),
                ('sync_ns', I64), ('focused', U32), ('hands', Hand * 2)]
class InputPacket(C.Structure):
    _fields_ = [('magic', U32), ('version', U32), ('size', U32), ('pid', U32), ('game_start', U64),
                ('publisher_pid', U64), ('publisher_start', U64), ('frequency', I64),
                ('latest_sequence', U64), ('frames', Input * 8)]


class ContractError(ValueError):
    pass
class Unavailable(ValueError):
    pass


def xyz(value):
    return (value.x, value.y, value.z)


def pose_valid(pose):
    q = pose.orientation
    return (all(math.isfinite(v) for v in xyz(pose.position) + (q.x, q.y, q.z, q.w)) and
            abs(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w - 1) <= .001)


def decode(stereo_bytes, input_bytes, game, publisher, frequency, now_ns):
    if len(stereo_bytes) != C.sizeof(StereoPacket) or len(input_bytes) != C.sizeof(InputPacket):
        raise ContractError('Unexpected bridge packet length')
    stereo, packet = StereoPacket.from_buffer_copy(stereo_bytes), InputPacket.from_buffer_copy(input_bytes)
    if ((stereo.magic, stereo.version, stereo.size, stereo.pid, stereo.frequency) !=
            (0x47565331, 4, C.sizeof(StereoPacket), game['pid'], frequency) or
            (packet.magic, packet.version, packet.size, packet.pid, packet.game_start,
             packet.publisher_pid, packet.publisher_start, packet.frequency) !=
            (0x47564333, 3, C.sizeof(InputPacket), game['pid'], game['creation_time'],
             publisher['pid'], publisher['creation_time'], frequency)):
        raise ContractError('Bridge version/process/publisher/QPC identity mismatch')
    view = stereo.sample
    if (not view.sequence or view.sequence > packet.latest_sequence or
            packet.latest_sequence - view.sequence >= 8):
        raise Unavailable('Exact hand ticket is not retained')
    frame = packet.frames[view.sequence % 8]
    if ((frame.source, frame.session, frame.sequence, frame.space_epoch, frame.predicted_time) !=
            (0, view.session, view.sequence, view.space_epoch, view.predicted_time) or view.source != 0 or
            not frame.session or not frame.space_epoch or not frame.profile or not frame.input_epoch or
            frame.focused != 1 or view.focused != 1 or view.should_render != 1 or
            view.head_flags & 15 != 15 or view.view_flags & 15 != 15 or
            view.visible > 1 or not pose_valid(view.head) or frame.predicted_time <= 0 or
            not 0 < frame.sync_ns <= frame.received_ns <= now_ns or
            not 0 < view.received_ns <= now_ns or
            now_ns - frame.received_ns > 100_000_000 or now_ns - view.received_ns > 100_000_000):
        raise Unavailable('Unfocused, stale, untracked or mismatched XR ticket')
    for eye in view.eyes:
        f = eye.fov
        if (not pose_valid(eye.pose) or any(not math.isfinite(v) or abs(v) >= 1.55 for v in
                                         (f.left, f.right, f.up, f.down)) or
                f.right - f.left <= .001 or f.up - f.down <= .001):
            raise Unavailable('Invalid stereo geometry')
    if not .04 <= math.dist(xyz(view.eyes[0].pose.position), xyz(view.eyes[1].pose.position)) <= .085:
        raise Unavailable('Invalid stereo eye separation')
    for index, hand in enumerate(frame.hands):
        allowed = 28 if index == 0 else 19
        if (any(v > 1 for v in (hand.grip_active, hand.aim_active, hand.trigger_active, hand.squeeze_active, hand.stick_active)) or
                (hand.buttons | hand.buttons_active) & ~allowed or hand.buttons & ~hand.buttons_active or
                not all(math.isfinite(v) for v in (hand.trigger, hand.squeeze, hand.stick_x, hand.stick_y)) or
                not 0 <= hand.trigger <= 1 or not 0 <= hand.squeeze <= 1 or
                abs(hand.stick_x) > 1 or abs(hand.stick_y) > 1 or
                (not hand.trigger_active and hand.trigger != 0) or
                (not hand.squeeze_active and hand.squeeze != 0) or
                (not hand.stick_active and (hand.stick_x != 0 or hand.stick_y != 0)) or
                hand.grip_active != 1 or hand.grip_flags & 15 != 15 or not pose_valid(hand.grip) or
                (hand.aim_active and hand.aim_flags & 3 == 3 and not pose_valid(hand.aim)) or
                hand.last_squeeze_press_sequence > frame.sequence or
                (hand.last_squeeze_press_sequence and hand.last_squeeze_press_flags & 3 == 3 and
                 not pose_valid(hand.last_squeeze_press_grip)) or hand.squeeze_active != 1):
            raise Unavailable('Invalid controller shape or lost grip tracking')
        for control, (pressed, released) in enumerate(zip(hand.pressed, hand.released)):
            if (pressed > frame.sequence or released > frame.sequence or abs(pressed - released) > 1 or
                    (index == 0 and control in (0, 1) and (pressed or released)) or
                    (index == 1 and control in (2, 3) and (pressed or released))):
                raise Unavailable('Invalid controller edge history')
    q = view.head.orientation
    forward_x, forward_z = -2*(q.x*q.z + q.y*q.w), -(1-2*(q.x*q.x + q.y*q.y))
    if math.hypot(forward_x, forward_z) < .05:
        raise Unavailable('Head heading unavailable')
    return Sample(frame.session, frame.sequence, frame.space_epoch, frame.profile, frame.input_epoch,
                  frame.received_ns, xyz(view.head.position), math.atan2(-forward_x, -forward_z),
                  xyz(frame.hands[0].grip.position), xyz(frame.hands[1].grip.position),
                  (frame.hands[0].squeeze, frame.hands[1].squeeze))


class Mapping:
    def __init__(self, api, name, lock, size):
        self.api, self.size = api, size
        self.mutex = self.mapping = self.address = None
        try:
            self.mutex = api.OpenMutexW(0x100001, False, lock)
            if not self.mutex:
                self._open_error()
            self.mapping = api.OpenFileMappingW(4, False, name)
            if not self.mapping:
                self._open_error()
            self.address = api.MapViewOfFile(self.mapping, 4, 0, 0, size)
            if not self.address:
                raise ContractError('XR mapping has no compatible readable view')
        except BaseException:
            self.close()
            raise

    @staticmethod
    def _open_error():
        if C.get_last_error() == 2:
            raise FileNotFoundError('XR bridge not published yet')
        raise ContractError('XR mapping/mutex could not be opened read-only')

    def read(self):
        state = self.api.WaitForSingleObject(self.mutex, 0)
        if state == 0x80:
            self.api.ReleaseMutex(self.mutex)
            raise Unavailable('Abandoned XR publisher mutex')
        if state == 0x102:
            return None
        if state != 0:
            raise ContractError('XR mutex wait failed')
        try:
            return C.string_at(self.address, self.size)
        finally:
            self.api.ReleaseMutex(self.mutex)

    def close(self):
        address, mapping, mutex = self.address, self.mapping, self.mutex
        self.address = self.mapping = self.mutex = None
        failed = []
        if address and not self.api.UnmapViewOfFile(address):
            failed.append('view')
        for handle in (mapping, mutex):
            if handle and not self.api.CloseHandle(handle):
                failed.append('handle')
        if failed:
            raise ContractError('XR consumer cleanup failed: ' + ','.join(failed))


class Reader:
    def __init__(self, game, publisher, identity):
        from ctypes import wintypes as W
        self.api = C.WinDLL('kernel32', use_last_error=True)
        for name, restype, args in (
            ('OpenMutexW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
            ('OpenFileMappingW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
            ('MapViewOfFile', C.c_void_p, (W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t)),
            ('WaitForSingleObject', W.DWORD, (W.HANDLE, W.DWORD)),
            ('ReleaseMutex', W.BOOL, (W.HANDLE,)), ('CloseHandle', W.BOOL, (W.HANDLE,)),
            ('UnmapViewOfFile', W.BOOL, (C.c_void_p,)),
            ('QueryPerformanceCounter', W.BOOL, (C.POINTER(I64),)),
            ('QueryPerformanceFrequency', W.BOOL, (C.POINTER(I64),))):
            method = getattr(self.api, name)
            method.restype, method.argtypes = restype, args
        frequency = I64()
        if not self.api.QueryPerformanceFrequency(C.byref(frequency)) or frequency.value <= 0:
            raise ContractError('QPC frequency unavailable')
        self.frequency = frequency.value
        self.game, self.publisher, self.identity = game, publisher, identity
        self.stereo = self.input = None

    def now(self):
        value = I64()
        if not self.api.QueryPerformanceCounter(C.byref(value)):
            raise ContractError('QPC unavailable')
        return value.value * 1_000_000_000 // self.frequency

    def read(self):
        if self.identity(self.game['pid']) != self.game or self.identity(self.publisher['pid']) != self.publisher:
            raise ContractError('Owned game or XR publisher exited/changed')
        pid, start = self.game['pid'], self.game['creation_time']
        if self.stereo is None:
            self.stereo = Mapping(self.api, f'Local\\GearsVR.Stereo.{pid}', f'Local\\GearsVR.StereoLock.{pid}', C.sizeof(StereoPacket))
        if self.input is None:
            self.input = Mapping(self.api, f'Local\\GearsVR.ControllerInput.v3.{pid}.{start}',
                                 f'Local\\GearsVR.ControllerInputLock.v3.{pid}.{start}', C.sizeof(InputPacket))
        view, hands = self.stereo.read(), self.input.read()
        if view is None or hands is None:
            return None
        return decode(view, hands, self.game, self.publisher, self.frequency, self.now())

    def close(self):
        failure = None
        for mapping in (self.input, self.stereo):
            if mapping is not None:
                try:
                    mapping.close()
                except BaseException as error:
                    if failure is None:
                        failure = error
        self.input = self.stereo = None
        if failure is not None:
            raise failure
