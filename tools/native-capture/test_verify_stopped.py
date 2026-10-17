"""Independent restoration verifier tests using saved bytes, never a process."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import verify_stopped


class VerifyStoppedTests(unittest.TestCase):
    # Historical artifact 1 entry bytes, independent of status-file contents.
    ROOT_ENTRY = bytes.fromhex('498b00498bc848ffa0a8040000')
    CAMERA_ENTRY = bytes.fromhex('488bc45356574154415541564157')

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.status = Path(self.directory.name) / 'capture-status.txt'
        self.output = Path(self.directory.name) / 'restored.json'
        self.base = 0x180000000  # Exercise the authenticated resolver's base.
        self.closed = mock.Mock()
        self.read = mock.Mock(side_effect=self.read_saved)
        self.memory = {self.base + 0x296178:self.ROOT_ENTRY,
                       self.base + 0x15cf08:self.CAMERA_ENTRY}
        self.resolver = SimpleNamespace(base=self.base, read=self.read, prepare=mock.Mock(),
                                        p=SimpleNamespace(handle=123, close=self.closed))

    def read_saved(self, address, size):
        return self.memory[address][:size]

    def write_status(self, rvas=(0x296178, 0x15cf08), *, pid=321, stopped=True):
        text = f'capturing pid={pid} flags=fixture\n'
        text += ''.join(f'hook_target={self.base + rva:X} create=MH_OK queue=MH_OK\n' for rva in rvas)
        if stopped:text += 'stopped disable_status=0 active=0\n'
        self.status.write_text(text)

    def invoke(self):
        args = ['verify_stopped.py', '--pid', '321', '--status', str(self.status), '--output', str(self.output)]
        with mock.patch.object(sys, 'argv', args), \
                mock.patch.object(verify_stopped, 'Resolver', return_value=self.resolver) as factory, \
                mock.patch.object(verify_stopped, 'modules', return_value={}) as modules, \
                contextlib.redirect_stdout(io.StringIO()):
            verify_stopped.main()
        factory.assert_called_once()
        self.resolver.prepare.assert_called_once()
        modules.assert_called_once_with(123)

    def test_exact_historical_root_matches_and_closes_reader(self):
        self.write_status()
        self.invoke()
        report = json.loads(self.output.read_text())
        self.assertTrue(report['all_match'])
        self.assertEqual(len(report['checks']), 2)
        root = report['checks'][0]
        self.assertEqual(root['bytes_checked'], 13)
        self.assertEqual(root['source'], 'artifact 1 runtime snapshot UI scene root adapter')
        self.assertEqual(root['expected_sha256'], '29fdfb65bcc522946db8b4bb2b62aa64f56be240b8b8f77d22c951a6fb981466')
        self.assertEqual(root['actual_sha256'], root['expected_sha256'])
        self.read.assert_any_call(self.base + 0x296178, 13)
        self.closed.assert_called_once()

    def test_each_corrupted_root_byte_fails_despite_capture_claim_and_closes(self):
        for at in range(13):
            with self.subTest(byte=at):
                self.closed.reset_mock()
                self.write_status()
                changed = bytearray(self.ROOT_ENTRY)
                changed[at] ^= 1
                self.memory[self.base + 0x296178] = bytes(changed)
                with self.status.open('a') as stream:
                    stream.write('entries_restored=1 original=' + bytes(changed).hex() + '\n')
                with self.assertRaises(SystemExit) as error:self.invoke()
                self.assertEqual(error.exception.code, 1)
                report = json.loads(self.output.read_text())
                self.assertFalse(report['all_match'])
                self.assertFalse(report['checks'][0]['matches'])
                self.closed.assert_called_once()

    def test_adjacent_unrecognized_game_entry_is_rejected_even_with_matching_bytes(self):
        self.write_status((0x296179, 0x15cf08))
        self.memory[self.base + 0x296179] = self.ROOT_ENTRY
        with self.assertRaisesRegex(RuntimeError, 'allowed graphics system module'):self.invoke()
        self.assertFalse(self.output.exists())
        self.read.assert_not_called()
        self.closed.assert_called_once()

    def test_clean_status_and_inventory_preconditions_remain_required(self):
        cases = [dict(pid=322), dict(stopped=False), dict(rvas=(0x296178,)),
                 dict(rvas=(0x296178, 0x296178)), dict(rvas=tuple(range(65)))]
        for values in cases:
            with self.subTest(values=values):
                self.write_status(**values)
                with mock.patch.object(verify_stopped, 'Resolver') as factory, \
                        mock.patch.object(sys, 'argv', ['verify_stopped.py', '--pid', '321',
                            '--status', str(self.status), '--output', str(self.output)]):
                    with self.assertRaises(RuntimeError):verify_stopped.main()
                factory.assert_not_called()
                self.assertFalse(self.output.exists())


if __name__ == '__main__':
    unittest.main()
