import contextlib
import json
import pathlib
import tempfile
import unittest
from unittest import mock
import background_input as b

class PatchTests(unittest.TestCase):
    def setUp(self):
        self.original = bytearray(b'\xcc' * b.FUNCTION_SIZE)
        for rva, before, _ in b.PATCHES:
            offset = rva - b.FUNCTION_RVA
            self.original[offset:offset + len(before)] = before
        self.hash_patch = mock.patch.object(b, 'FUNCTION_SHA256', b.digest(self.original))
        self.hash_patch.start(); self.addCleanup(self.hash_patch.stop)
        self.code = bytearray(self.original)

    def test_original_enabled_and_foreign_or_partial_code(self):
        self.assertEqual(b.classify(self.code), 'original')
        for rva, _, after in b.PATCHES:
            off = rva - b.FUNCTION_RVA; self.code[off:off + len(after)] = after
        self.assertEqual(b.classify(self.code), 'enabled')
        self.code[0] ^= 1
        with self.assertRaises(b.CodeNotReady): b.classify(self.code)
        self.code[:] = self.original
        rva, _, after = b.PATCHES[0]; self.code[rva-b.FUNCTION_RVA] = after[0]
        with self.assertRaises(b.CodeNotReady): b.classify(self.code)
        with self.assertRaises(b.CodeNotReady): b.classify(b'')

    @contextlib.contextmanager
    def target(self):
        with tempfile.TemporaryDirectory() as tmp, contextlib.ExitStack() as stack:
            root = pathlib.Path(tmp); exe = root / 'GOWDE-Steam.exe'; exe.write_bytes(b'fixture')
            profile = root/'tools/package-inspector/reloaded-profile.json'; profile.parent.mkdir(parents=True)
            profile.write_text(json.dumps({'sha256':b.digest(exe.read_bytes())}))
            identity = {'pid':42,'creation_time':123,'image':str(exe)}
            control = mock.Mock(); control.identity.return_value = identity
            process = mock.Mock(); process.handle = 7
            process.read.side_effect = lambda *_: bytes(self.code)
            frozen = mock.MagicMock(); frozen.__enter__.return_value.handles = [11,12,13]
            def write(handle, address, raw):
                off = address - 0x140000000 - b.FUNCTION_RVA
                self.code[off:off+len(raw)] = raw
            for name,value in [('ROOT',root),('WindowsControl',mock.Mock(return_value=control)),
                               ('Process',mock.Mock(return_value=process)),
                               ('modules',mock.Mock(return_value={'gowde-steam.exe':(0x140000000,exe)})),
                               ('FrozenThreads',mock.Mock(return_value=frozen))]:
                stack.enter_context(mock.patch.object(b,name,value))
            native = stack.enter_context(mock.patch.object(b,'k'))
            native.OpenProcess.return_value = 8
            writes = stack.enter_context(mock.patch.object(b,'write_code',side_effect=write))
            yield root, exe, control, writes, write
            self.assertEqual(exe.read_bytes(),b'fixture')

    def test_enable_idempotent_restore_and_ledger(self):
        with self.target() as (root,exe,control,writes,write):
            enabled = b.change(42,'enable')
            self.assertEqual(enabled['state'],'enabled'); self.assertEqual(enabled['suspended_threads'],3)
            b.change(42,'enable'); self.assertEqual(writes.call_count,2)
            restored = b.change(42,'disable'); self.assertEqual(restored['state'],'original')
            self.assertEqual(self.code,self.original)

    def test_partial_write_error_rolls_back(self):
        with self.target() as (root,exe,control,writes,write):
            calls = 0
            def fail(handle,address,raw):
                nonlocal calls
                calls += 1
                write(handle,address,raw)
                if calls == 2: raise RuntimeError('simulated failed protection restore')
            writes.side_effect = fail
            with self.assertRaisesRegex(RuntimeError,'simulated'): b.change(42,'enable')
            self.assertEqual(self.code,self.original)
            saved = json.loads(next((root/'analysis/runtime').glob('*.json')).read_text())
            self.assertTrue(saved['rolled_back']); self.assertEqual(saved['state'],'original')

    def test_changed_identity_rejected_before_writes(self):
        with self.target() as (_,exe,control,writes,write):
            first = control.identity.return_value
            control.identity.side_effect = [first,dict(first,creation_time=999)]
            with self.assertRaisesRegex(RuntimeError,'identity'): b.change(42,'enable')
            writes.assert_not_called()

    def test_startup_pid_reuse_is_rejected(self):
        with self.target() as (_,exe,control,writes,write):
            with self.assertRaisesRegex(RuntimeError,'reused PID'): b.change(42,'enable',expected_creation=999)
            writes.assert_not_called()

    def test_unowned_enabled_bytes_are_not_adopted_or_restored(self):
        with self.target() as (_,exe,control,writes,write):
            for rva,_,after in b.PATCHES: write(8,0x140000000+rva,after)
            for action in ('enable','disable'):
                with self.assertRaisesRegex(RuntimeError,'ownership'): b.change(42,action)
            writes.assert_not_called()

    def test_suspend_failure_resumes_already_suspended_threads(self):
        with mock.patch.object(b,'thread_ids',return_value={1,2}),mock.patch.object(b,'k') as k:
            k.OpenThread.side_effect=[10,0];k.SuspendThread.return_value=0;k.GetThreadContext.return_value=True
            with self.assertRaises(OSError):
                with b.FrozenThreads(42,0x140000000): pass
            k.ResumeThread.assert_called_once_with(10); k.CloseHandle.assert_called_once_with(10)

if __name__=='__main__': unittest.main()
