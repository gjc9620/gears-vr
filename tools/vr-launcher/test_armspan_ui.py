"""Own-process Win32 IPC and bounded stdout feedback tests; no game or XR."""
import contextlib
import ctypes as C
from ctypes import wintypes as W
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import threading
import unittest
import uuid

# -I intentionally excludes the script directory; select this exact source.
SOURCE = Path(__file__).resolve().parent
sys.path.insert(0, str(SOURCE))
import armspan_ui as ui

U64 = (1 << 64) - 1
GAME = {'pid': 11, 'creation_time': 0x1122334455667788}
OWNER = {'pid': 22, 'creation_time': 0x2233445566778899}
PUBLISHER = {'pid': 33, 'creation_time': 0x33445566778899aa}
TOKEN = '0123456789abcdef0123456789abcdef'


class PacketTests(unittest.TestCase):
    def test_exact_112_byte_fields(self):
        self.assertEqual(Path(ui.__file__).resolve(), SOURCE / 'armspan_ui.py')
        raw = ui.packet(GAME, OWNER, PUBLISHER, TOKEN, 0x123456789abcdef0,
                        0x123456789abcdef, 'holding', 999, 'zh-CN')
        expected = (b'GCALUI1\0' + (1).to_bytes(4, 'little') + (112).to_bytes(4, 'little') +
                    b''.join(value.to_bytes(8, 'little') for value in
                             (11, GAME['creation_time'], 22, OWNER['creation_time'],
                              33, PUBLISHER['creation_time'])) + bytes.fromhex(TOKEN) +
                    (0x123456789abcdef0).to_bytes(8, 'little') +
                    (0x123456789abcdef).to_bytes(8, 'little', signed=True) +
                    struct.pack('<4I', 2, 999, 1, 0))
        self.assertEqual(len(raw), 112)
        self.assertEqual(raw, expected)
        self.assertEqual(raw[64:80], bytes.fromhex(TOKEN))
        self.assertEqual(struct.unpack_from('<Qq4I', raw, 80),
                         (0x123456789abcdef0, 0x123456789abcdef, 2, 999, 1, 0))

    def test_valid_content_and_integer_boundaries(self):
        for phase, progress in [('waiting', 0), ('holding', 0), ('holding', 500),
                                ('holding', 999), ('saved', 1000), ('cancelled', 0), ('failed', 0)]:
            for language in ('en', 'zh-CN'):
                with self.subTest(phase=phase, progress=progress, language=language):
                    raw = ui.packet(GAME, OWNER, PUBLISHER, TOKEN, U64, (1 << 63) - 1,
                                    phase, progress, language)
                    self.assertEqual(struct.unpack_from('<4I', raw, 96),
                                     (ui.PHASES[phase], progress, ui.LANGUAGES[language], 0))

    def test_invalid_tokens_names_and_packet_content(self):
        for token in (None, '', '0' * 32, TOKEN.upper(), 'g' * 32, TOKEN + '\n', TOKEN[:-1]):
            with self.subTest(token=token), self.assertRaises(ValueError):
                ui.object_names(token)
        self.assertEqual(ui.object_names(TOKEN),
                         ('Local\\GearsVR.ArmspanUI.v1.' + TOKEN,
                          'Local\\GearsVR.ArmspanUI.v1.' + TOKEN + '.Lock'))
        base = [GAME, OWNER, PUBLISHER, TOKEN, 1, 1, 'waiting', 0, 'en']
        changes = [(4, 0), (4, -1), (4, U64 + 1), (4, True), (5, 0), (5, 1 << 63),
                   (5, 1.0), (6, 'unknown'), (7, True), (7, 1), (8, 'zh')]
        for index, value in changes:
            with self.subTest(index=index, value=value), self.assertRaises(ValueError):
                args = list(base)
                args[index] = value
                ui.packet(*args)
        for phase, progress in [('holding', -1), ('holding', 1000), ('saved', 999),
                                ('saved', 1001), ('cancelled', 1), ('failed', 1000)]:
            with self.subTest(phase=phase, progress=progress), self.assertRaises(ValueError):
                ui.packet(*base[:6], phase, progress, 'en')
        for index in range(3):
            for field, value in [('pid', 0), ('pid', 1 << 32), ('pid', True),
                                 ('creation_time', 0), ('creation_time', U64 + 1)]:
                with self.subTest(identity=index, field=field, value=value), self.assertRaises(ValueError):
                    args = list(base)
                    args[index] = dict(args[index], **{field: value})
                    ui.packet(*args)


class MemoryInfo(C.Structure):
    _fields_ = [('BaseAddress', C.c_void_p), ('AllocationBase', C.c_void_p),
                ('AllocationProtect', W.DWORD)]
    if C.sizeof(C.c_void_p) == 8:
        _fields_ += [('PartitionId', W.WORD)]
    _fields_ += [('RegionSize', C.c_size_t), ('State', W.DWORD), ('Protect', W.DWORD), ('Type', W.DWORD)]


@unittest.skipUnless(os.name == 'nt', 'actual Win32 own-process objects required')
class Win32PublisherTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = C.WinDLL('kernel32', use_last_error=True)
        signatures = (
            ('OpenFileMappingW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
            ('OpenMutexW', W.HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
            ('CreateFileMappingW', W.HANDLE, (W.HANDLE, C.c_void_p, W.DWORD, W.DWORD, W.DWORD, W.LPCWSTR)),
            ('CreateMutexW', W.HANDLE, (C.c_void_p, W.BOOL, W.LPCWSTR)),
            ('MapViewOfFile', C.c_void_p, (W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t)),
            ('UnmapViewOfFile', W.BOOL, (C.c_void_p,)), ('CloseHandle', W.BOOL, (W.HANDLE,)),
            ('WaitForSingleObject', W.DWORD, (W.HANDLE, W.DWORD)), ('ReleaseMutex', W.BOOL, (W.HANDLE,)),
            ('GetCurrentProcess', W.HANDLE, ()), ('GetProcessHandleCount', W.BOOL, (W.HANDLE, C.POINTER(W.DWORD))),
            ('GetProcessTimes', W.BOOL, (W.HANDLE,) + (C.POINTER(W.FILETIME),) * 4),
            ('VirtualQuery', C.c_size_t, (C.c_void_p, C.POINTER(MemoryInfo), C.c_size_t)))
        for name, result, arguments in signatures:
            function = getattr(cls.api, name)
            function.restype, function.argtypes = result, arguments
        created, exited, kernel, user = (W.FILETIME() for _ in range(4))
        if not cls.api.GetProcessTimes(cls.api.GetCurrentProcess(), C.byref(created), C.byref(exited),
                                       C.byref(kernel), C.byref(user)):
            raise OSError('own process identity unavailable')
        cls.identity = dict(pid=os.getpid(), creation_time=(created.dwHighDateTime << 32) | created.dwLowDateTime)

    def publisher(self, **kwargs):
        publisher = ui.Publisher(self.identity, self.identity, **kwargs)
        self.addCleanup(publisher.close)
        return publisher

    @contextlib.contextmanager
    def read_view(self, token):
        name, mutex_name = ui.object_names(token)
        mapping = self.api.OpenFileMappingW(4, False, name)  # FILE_MAP_READ
        mutex = self.api.OpenMutexW(0x100001, False, mutex_name)  # SYNCHRONIZE | MUTEX_MODIFY_STATE
        view = None
        try:
            self.assertTrue(mapping and mutex)
            view = self.api.MapViewOfFile(mapping, 4, 0, 0, 112)
            self.assertTrue(view)
            info = MemoryInfo()
            self.assertEqual(self.api.VirtualQuery(view, C.byref(info), C.sizeof(info)), C.sizeof(info))
            self.assertEqual(info.Protect & 0xff, 2)  # PAGE_READONLY; do not attempt a faulting write.
            yield view, mutex
        finally:
            closed = []
            if view:
                closed.append(bool(self.api.UnmapViewOfFile(view)))
            if mapping:
                closed.append(bool(self.api.CloseHandle(mapping)))
            if mutex:
                closed.append(bool(self.api.CloseHandle(mutex)))
            self.assertTrue(all(closed))

    def read(self, view, mutex):
        self.assertEqual(self.api.WaitForSingleObject(mutex, 1000), 0)
        try:
            return C.string_at(view, 112)
        finally:
            self.assertTrue(self.api.ReleaseMutex(mutex))

    def absent(self, token, *, mapping=True, mutex=True):
        name, mutex_name = ui.object_names(token)
        for needed, function, arguments in (
                (mapping, self.api.OpenFileMappingW, (4, False, name)),
                (mutex, self.api.OpenMutexW, (0x100000, False, mutex_name))):
            if needed:
                handle = function(*arguments)
                if handle:
                    self.api.CloseHandle(handle)
                self.assertFalse(handle)

    def handle_count(self):
        count = W.DWORD()
        self.assertTrue(self.api.GetProcessHandleCount(self.api.GetCurrentProcess(), C.byref(count)))
        return count.value

    def test_actual_mapping_read_only_update_close_and_collision(self):
        publisher = self.publisher(language='zh-CN')
        self.assertEqual(publisher.publisher, self.identity)
        with self.read_view(publisher.token) as (view, mutex):
            first = self.read(view, mutex)
            self.assertEqual(first, ui.packet(self.identity, self.identity, self.identity, publisher.token,
                                             1, publisher.updated_ns, 'waiting', 0, 'zh-CN'))
            with self.assertRaises(OSError):
                ui.Publisher(self.identity, self.identity, token=publisher.token)
            self.assertEqual(self.read(view, mutex), first)
            updated = publisher.updated_ns + 1
            self.assertEqual(publisher.publish('holding', 500, updated), 2)
            self.assertEqual(self.read(view, mutex), ui.packet(self.identity, self.identity, self.identity,
                             publisher.token, 2, updated, 'holding', 500, 'zh-CN'))
            before = self.read(view, mutex)
            with self.assertRaises(ValueError):
                publisher.publish('saved', 999, updated)
            with self.assertRaises(RuntimeError):
                publisher.publish('saved', 1000, updated - 1)
            self.assertEqual(self.read(view, mutex), before)
            self.assertEqual((publisher.sequence, publisher.updated_ns), (2, updated))
            publisher.close()
            self.assertEqual(self.read(view, mutex), before)  # Reader owns the remaining view/handles.
        self.absent(publisher.token)
        publisher.close()
        with self.assertRaises(RuntimeError):
            publisher.publish('waiting', 0)

    def test_existing_mapping_collision_cleans_new_mutex(self):
        token = uuid.uuid4().hex
        name, _ = ui.object_names(token)
        mapping = self.api.CreateFileMappingW(W.HANDLE(-1), None, 4, 0, 112, name)
        self.assertTrue(mapping)
        self.addCleanup(self.api.CloseHandle, mapping)
        self.absent(token, mapping=False)
        with self.assertRaises(OSError):
            ui.Publisher(self.identity, self.identity, token=token)
        self.absent(token, mapping=False)
        view = self.api.MapViewOfFile(mapping, 4, 0, 0, 112)
        self.assertTrue(view)
        try:
            self.assertEqual(C.string_at(view, 112), bytes(112))
        finally:
            self.assertTrue(self.api.UnmapViewOfFile(view))

    def test_mutex_timeout_and_abandonment_do_not_publish(self):
        publisher = self.publisher()
        acquired, release = threading.Event(), threading.Event()
        outcomes = []
        def hold():
            outcomes.append(self.api.WaitForSingleObject(publisher.mutex, 1000))
            acquired.set()
            if release.wait(2):
                outcomes.append(bool(self.api.ReleaseMutex(publisher.mutex)))
        worker = threading.Thread(target=hold)
        worker.start()
        try:
            self.assertTrue(acquired.wait(2))
            self.assertEqual(outcomes, [0])
            with self.assertRaisesRegex(RuntimeError, 'lock unavailable'):
                publisher.publish('holding', 500)
            self.assertEqual(publisher.sequence, 1)
        finally:
            release.set()
            worker.join(2)
        self.assertFalse(worker.is_alive())
        self.assertEqual(outcomes, [0, True])
        outcomes.clear()
        def abandon():
            outcomes.append(self.api.WaitForSingleObject(publisher.mutex, 1000))
        worker = threading.Thread(target=abandon)
        worker.start()
        worker.join(2)
        self.assertFalse(worker.is_alive())
        self.assertEqual(outcomes, [0])
        with self.assertRaisesRegex(RuntimeError, 'lock unavailable'):
            publisher.publish('holding', 500)
        self.assertEqual(publisher.sequence, 1)
        self.assertEqual(publisher.publish('holding', 500), 2)  # Abandoned ownership was released.

    def test_invalid_creation_and_repeated_lifetimes_reclaim_handles(self):
        token = uuid.uuid4().hex
        before = self.handle_count()
        with self.assertRaises(ValueError):
            ui.Publisher(self.identity, self.identity, language='unsupported', token=token)
        self.absent(token)
        for _ in range(8):
            publisher = self.publisher()
            with self.read_view(publisher.token) as (view, mutex):
                self.assertEqual(len(self.read(view, mutex)), 112)
            publisher.close()
            self.absent(publisher.token)
        after = self.handle_count()
        self.assertEqual(after, before)
        print(f'Win32 Publisher eight lifetimes: handles={before}->{after}; read-only views and object removal verified', flush=True)


class Clock:
    def __init__(self, path):
        self.time, self.path, self.chunks, self.sleeps = 0., Path(path), [], 0
    def __call__(self):
        return self.time
    def append(self, data):
        with self.path.open('ab') as stream:
            stream.write(data)
    def sleep(self, interval):
        self.time = round(self.time + interval, 8)
        self.sleeps += 1
        if self.sleeps > 1000:
            raise AssertionError('unbounded wait')
        while self.chunks and self.chunks[0][0] <= self.time:
            _, data = self.chunks.pop(0)
            self.append(data)


class Pulses:
    def __init__(self, clock):
        self.clock, self.sequence, self.calls, self.fail = clock, 10, [], False
    def publish(self, phase, progress):
        if self.fail:
            raise OSError('synthetic heartbeat failure')
        self.sequence += 1
        self.calls.append((self.clock(), phase, progress, self.sequence))
        return self.sequence


def feedback(**changes):
    value = dict(armspan_ui_presented=True, token=TOKEN, phase='saved', sequence=10, end_frame_succeeded=True)
    value.update(changes)
    return json.dumps(value, separators=(',', ':')).encode() + b'\n'


class WaitSavedTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix='gears-armspan-ui-test-')
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name) / 'feedback.stdout'
        self.clock = Clock(self.path)
        self.publisher = Pulses(self.clock)
    def wait(self, *, child=None, owner_alive=lambda: True, cancel=lambda: False, timeout=2.5):
        if child is None:
            child = type('LiveChild', (), {'poll': lambda _: None})()
        return ui.wait_saved(self.path, TOKEN, 10, child, owner_alive, cancel, self.publisher,
                             clock=self.clock, sleep=self.clock.sleep, timeout=timeout)

    def test_chunked_junk_then_success_holds_point_six_and_heartbeats(self):
        self.clock.append(b'ordinary log\n\xff\xfe\n[]\n' + feedback(token='f' * 32) + feedback(sequence=9))
        line = feedback()
        self.clock.chunks = [(.03, line[:19]), (.07, line[19:-1]), (.11, b'\r'), (.12, b'\n')]
        value = self.wait()
        self.assertEqual(value['sequence'], 10)
        self.assertGreaterEqual(self.clock(), .72)
        self.assertLess(self.clock(), .74)
        self.assertGreaterEqual(len(self.publisher.calls), 3)
        for at, phase, progress, _ in self.publisher.calls:
            self.assertEqual((phase, progress), ('saved', 1000))
            self.assertGreaterEqual(at, .2)

    def test_new_heartbeat_sequence_can_confirm_but_unpublished_future_cannot(self):
        self.clock.append(feedback(sequence=11))  # Not yet published; must be discarded.
        self.clock.chunks = [(.25, feedback(sequence=11))]  # First heartbeat published at .2.
        result = self.wait()
        self.assertEqual(result['sequence'], 11)
        self.assertGreaterEqual(self.clock(), .85)
        self.assertLess(self.clock(), .87)

    def test_wrong_receipts_do_not_confirm(self):
        cases = [dict(token='f' * 32), dict(sequence=9), dict(sequence=11), dict(sequence=U64 + 1),
                 dict(sequence=0), dict(sequence=-1), dict(sequence=True), dict(sequence=10.), dict(sequence='10'),
                 dict(armspan_ui_presented=1), dict(armspan_ui_presented=False), dict(phase='holding'),
                 dict(end_frame_succeeded=False), dict(end_frame_succeeded=1)]
        for changes in cases:
            with self.subTest(changes=changes):
                self.path.write_bytes(feedback(**changes))
                self.clock = Clock(self.path)
                self.publisher = Pulses(self.clock)
                with self.assertRaises(TimeoutError):
                    self.wait(timeout=.19)  # No heartbeat can make sequence11 current.
                self.assertEqual(self.publisher.calls, [])

    def test_unterminated_feedback_and_late_ack_do_not_bypass_visibility(self):
        self.clock.append(feedback()[:-1])
        with self.assertRaises(TimeoutError):
            self.wait(timeout=.19)
        self.path.unlink()
        self.clock = Clock(self.path)
        self.publisher = Pulses(self.clock)
        self.clock.chunks = [(2.1, feedback(sequence=10))]
        with self.assertRaises(TimeoutError):
            self.wait()
        self.assertEqual(self.clock(), 2.5)

    def test_missing_file_timeout_and_cancel_during_saved_hold(self):
        with self.assertRaises(TimeoutError):
            self.wait(timeout=.25)
        self.assertFalse(self.path.exists())
        self.assertEqual(len(self.publisher.calls), 1)
        self.clock = Clock(self.path)
        self.publisher = Pulses(self.clock)
        self.clock.append(feedback())
        self.assertIsNone(self.wait(cancel=lambda: self.clock() >= .3))
        self.assertEqual(self.clock(), .3)

    def test_child_and_owner_death_before_or_during_hold(self):
        for dead, when in [('child', 0), ('child', .3), ('owner', 0), ('owner', .3)]:
            with self.subTest(dead=dead, when=when):
                self.path.write_bytes(feedback())
                self.clock = Clock(self.path)
                self.publisher = Pulses(self.clock)
                child = type('Child', (), {'poll': lambda _: 0 if dead == 'child' and self.clock() >= when else None})()
                with self.assertRaisesRegex(RuntimeError, 'child/owner exited'):
                    self.wait(child=child, owner_alive=lambda: not (dead == 'owner' and self.clock() >= when))
                self.assertEqual(self.clock(), when)

    def test_line_bound_exact_and_cross_chunk_overflow(self):
        self.clock.append(b'x' * 65536 + b'\n' + feedback())
        self.assertEqual(self.wait()['sequence'], 10)
        self.assertGreaterEqual(self.clock(), .61)  # Ack is in the second bounded read.
        for suffix in (b'', b'\n' + feedback()):
            with self.subTest(terminated=bool(suffix)):
                self.path.write_bytes(b'x' * 65537 + suffix)
                self.clock = Clock(self.path)
                self.publisher = Pulses(self.clock)
                with self.assertRaisesRegex(RuntimeError, 'line exceeds its bound'):
                    self.wait()
                self.assertLessEqual(self.clock(), .01)

    def test_heartbeat_failure_cannot_finish_saved_hold(self):
        self.clock.append(feedback())
        self.publisher.fail = True
        with self.assertRaisesRegex(OSError, 'heartbeat failure'):
            self.wait()
        self.assertEqual(self.clock(), .2)


if __name__ == '__main__':
    unittest.main(verbosity=2)
