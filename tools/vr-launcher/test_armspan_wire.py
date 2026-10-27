import ctypes as C
from ctypes import wintypes as W
import os
import unittest
import uuid
from unittest import mock

import armspan_wire as wire


GAME = dict(pid=100, creation_time=200, image='game.exe')
PUBLISHER = dict(pid=300, creation_time=400, image='xr.exe')
NOW = 1_000_000_000


def packets():
    view, hands = wire.StereoPacket(), wire.InputPacket()
    view.magic, view.version, view.size, view.pid, view.frequency = 0x47565331, 4, C.sizeof(view), 100, 10_000_000
    hands.magic, hands.version, hands.size, hands.pid = 0x47564333, 3, C.sizeof(hands), 100
    hands.game_start, hands.publisher_pid, hands.publisher_start, hands.frequency, hands.latest_sequence = 200, 300, 400, 10_000_000, 8
    sample = view.sample
    sample.session, sample.sequence, sample.space_epoch = 1, 8, 1
    sample.received_ns, sample.predicted_time = NOW, 10
    sample.head_flags, sample.view_flags, sample.focused, sample.should_render, sample.visible = 15, 15, 1, 1, 1
    sample.head.position.y, sample.head.orientation.w = 1.7, 1
    for index, eye in enumerate(sample.eyes):
        eye.pose.orientation.w, eye.pose.position.x = 1, -.032 if index == 0 else .032
        eye.fov.left, eye.fov.right, eye.fov.up, eye.fov.down = -.7, .7, .7, -.7
    frame = hands.frames[0]
    frame.session, frame.sequence, frame.space_epoch, frame.profile, frame.input_epoch = 1, 8, 1, 1, 1
    frame.predicted_time, frame.received_ns, frame.sync_ns, frame.focused = 10, NOW, NOW-1, 1
    for index, hand in enumerate(frame.hands):
        hand.grip.orientation.w, hand.grip.position.x, hand.grip.position.y = 1, -.85 if index == 0 else .85, 1.55
        hand.grip_active, hand.grip_flags, hand.squeeze_active, hand.squeeze = 1, 15, 1, 1
    return view, hands


def decode(view, hands, now=NOW):
    return wire.decode(bytes(view), bytes(hands), GAME, PUBLISHER, 10_000_000, now)


class WireTests(unittest.TestCase):
    def test_reader_attempts_both_mapping_closes_even_when_one_fails(self):
        reader = wire.Reader.__new__(wire.Reader)
        first = mock.Mock()
        second = mock.Mock()
        first.close.side_effect = wire.ContractError('close fixture')
        reader.input, reader.stereo = first, second
        with self.assertRaisesRegex(wire.ContractError, 'close fixture'):
            reader.close()
        first.close.assert_called_once()
        second.close.assert_called_once()
        self.assertEqual((reader.input, reader.stereo), (None, None))

    def test_x64_layout_and_matching_physical_ticket(self):
        self.assertEqual([C.sizeof(t) for t in (wire.Vector, wire.Quaternion, wire.Pose,
                         wire.Stereo, wire.StereoPacket, wire.Hand, wire.Input, wire.InputPacket)],
                         [24, 32, 56, 312, 336, 304, 688, 5560])
        self.assertEqual([wire.StereoPacket.sample.offset, wire.Stereo.head.offset, wire.Stereo.eyes.offset,
                          wire.Stereo.space_epoch.offset, wire.Hand.pressed.offset,
                          wire.Hand.last_squeeze_press_grip.offset, wire.Input.hands.offset, wire.InputPacket.frames.offset],
                         [24, 64, 120, 296, 172, 232, 80, 56])
        value = decode(*packets())
        self.assertEqual((value.sequence, value.space_epoch, value.squeeze), (8, 1, (1., 1.)))
        self.assertAlmostEqual(value.left[0], -.85)

    def test_header_contract_mismatch_is_fatal(self):
        for target, name in (('view', 'magic'), ('view', 'version'), ('view', 'size'), ('view', 'pid'),
                             ('view', 'frequency'), ('hands', 'version'), ('hands', 'game_start'),
                             ('hands', 'publisher_pid'), ('hands', 'publisher_start'), ('hands', 'frequency')):
            with self.subTest(target=target, name=name):
                view, hands = packets()
                value = view if target == 'view' else hands
                setattr(value, name, getattr(value, name)+1)
                with self.assertRaises(wire.ContractError):
                    decode(view, hands)
        with self.assertRaises(wire.ContractError):
            wire.decode(b'bad', bytes(packets()[1]), GAME, PUBLISHER, 10_000_000, NOW)

    def test_unfocused_wrong_ticket_simulator_or_stale_data_never_reaches_measurement(self):
        changes = [lambda v, h: setattr(v.sample, 'source', 1),
                   lambda v, h: setattr(h.frames[0], 'source', 1),
                   lambda v, h: setattr(h, 'latest_sequence', 7),
                   lambda v, h: setattr(h, 'latest_sequence', 16),
                   lambda v, h: setattr(h.frames[0], 'sequence', 7),
                   lambda v, h: setattr(h.frames[0], 'predicted_time', 11),
                   lambda v, h: setattr(h.frames[0], 'space_epoch', 2),
                   lambda v, h: setattr(h.frames[0], 'profile', 0),
                   lambda v, h: setattr(h.frames[0], 'focused', 0),
                   lambda v, h: setattr(v.sample, 'should_render', 0),
                   lambda v, h: setattr(v.sample, 'head_flags', 3),
                   lambda v, h: setattr(h.frames[0], 'received_ns', NOW+1),
                   lambda v, h: setattr(v.sample, 'received_ns', NOW-100_000_001)]
        for index, change in enumerate(changes):
            with self.subTest(index=index), self.assertRaises(wire.Unavailable):
                view, hands = packets()
                change(view, hands)
                decode(view, hands)

    def test_geometry_grip_tracking_and_analog_edge_bounds(self):
        changes = [lambda v, h: setattr(v.sample.head.orientation, 'w', .5),
                   lambda v, h: setattr(v.sample.eyes[0].fov, 'left', float('nan')),
                   lambda v, h: setattr(v.sample.eyes[1].pose.position, 'x', 2),
                   lambda v, h: setattr(h.frames[0].hands[0], 'grip_flags', 3),
                   lambda v, h: setattr(h.frames[0].hands[1], 'grip_active', 0),
                   lambda v, h: setattr(h.frames[0].hands[1], 'squeeze_active', 0),
                   lambda v, h: setattr(h.frames[0].hands[1], 'squeeze', float('nan')),
                   lambda v, h: setattr(h.frames[0].hands[0], 'buttons_active', 1),
                   lambda v, h: h.frames[0].hands[0].pressed.__setitem__(6, 3),
                   lambda v, h: h.frames[0].hands[1].pressed.__setitem__(2, 1)]
        for index, change in enumerate(changes):
            with self.subTest(index=index), self.assertRaises(wire.Unavailable):
                view, hands = packets()
                change(view, hands)
                decode(view, hands)


@unittest.skipUnless(os.name == 'nt', 'Windows named mapping handle lifecycle')
class MappingTests(unittest.TestCase):
    def setUp(self):
        self.api = C.WinDLL('kernel32', use_last_error=True)
        for name, restype, args in (
                ('CreateMutexW', W.HANDLE, (C.c_void_p, W.BOOL, W.LPCWSTR)),
                ('CreateFileMappingW', W.HANDLE, (W.HANDLE, C.c_void_p, W.DWORD, W.DWORD, W.DWORD, W.LPCWSTR)),
                ('OpenMutexW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
                ('OpenFileMappingW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
                ('MapViewOfFile', C.c_void_p, (W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t)),
                ('WaitForSingleObject', W.DWORD, (W.HANDLE, W.DWORD)),
                ('ReleaseMutex', W.BOOL, (W.HANDLE,)), ('CloseHandle', W.BOOL, (W.HANDLE,)),
                ('UnmapViewOfFile', W.BOOL, (C.c_void_p,))):
            method = getattr(self.api, name)
            method.restype, method.argtypes = restype, args

    def test_read_only_consumer_handles_are_closed_and_temporary_mapping_disappears(self):
        name = 'Local\\GearsVR.Armspan.Unit.' + uuid.uuid4().hex
        mutex = self.api.CreateMutexW(None, False, name+'.lock')
        creator = self.api.CreateFileMappingW(W.HANDLE(-1), None, 4, 0, 64, name)
        self.assertTrue(mutex and creator)
        mapping = None
        try:
            mapping = wire.Mapping(self.api, name, name+'.lock', 64)
            self.assertEqual(mapping.read(), bytes(64))
            self.assertTrue(self.api.CloseHandle(creator))
            creator = None
            self.assertTrue(self.api.CloseHandle(mutex))
            mutex = None
            mapping.close()
            self.assertEqual((mapping.address, mapping.mapping, mapping.mutex), (None, None, None))
            mapping.close()  # Idempotent cleanup must not close reused handles.
            leftover = self.api.OpenFileMappingW(4, False, name)
            if leftover:
                self.api.CloseHandle(leftover)
            self.assertFalse(leftover)
            self.assertFalse(self.api.OpenMutexW(0x100001, False, name+'.lock'))
        finally:
            if mapping:
                mapping.close()
            for handle in (creator, mutex):
                if handle:
                    self.api.CloseHandle(handle)

    def test_missing_mapping_does_not_leak_opened_mutex(self):
        name = 'Local\\GearsVR.Armspan.Unit.' + uuid.uuid4().hex
        mutex = self.api.CreateMutexW(None, False, name+'.lock')
        try:
            with self.assertRaises(FileNotFoundError):
                wire.Mapping(self.api, name, name+'.lock', 64)
        finally:
            self.api.CloseHandle(mutex)
        self.assertFalse(self.api.OpenMutexW(0x100001, False, name+'.lock'))


if __name__ == '__main__':
    unittest.main()
