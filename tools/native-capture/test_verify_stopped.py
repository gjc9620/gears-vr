"""Independent restoration verifier tests using saved bytes, never a process."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import verify_stopped


class VerifyStoppedTests(unittest.TestCase):
    def test_full_owned_inventory_keeps_every_entry_and_rejects_duplicates(self):
        def status(count):
            return ''.join(f'hook_target={0x140000000+i*32:x} create=MH_OK queue=MH_OK\n' for i in range(count))
        for count in (2, 65, 87, 91, 102):
            self.assertEqual(verify_stopped.hook_targets(status(count)),
                             [0x140000000+i*32 for i in range(count)])
        for raw in (status(1), status(103), status(65)+status(2)):
            with self.assertRaises(RuntimeError):verify_stopped.hook_targets(raw)

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

    def invoke(self, loaded=None, prime_status=None, hook_manifest=None, game_creation_time=None):
        args = ['verify_stopped.py', '--pid', '321', '--status', str(self.status), '--output', str(self.output)]
        if prime_status is not None:args += ['--prime-status', str(prime_status)]
        if hook_manifest is not None:args += ['--hook-manifest', str(hook_manifest)]
        if game_creation_time is not None:args += ['--game-creation-time', str(game_creation_time)]
        with mock.patch.object(sys, 'argv', args), \
                mock.patch.object(verify_stopped, 'Resolver', return_value=self.resolver) as factory, \
                mock.patch.object(verify_stopped, 'modules', return_value={} if loaded is None else loaded) as modules, \
                contextlib.redirect_stdout(io.StringIO()):
            verify_stopped.main()
        factory.assert_called_once()
        self.resolver.prepare.assert_called_once()
        modules.assert_called_once_with(123)

    def fake_pe(self, raw=bytes(range(32)), relocs=(), image_base=0x70000000):
        pe=SimpleNamespace(OPTIONAL_HEADER=SimpleNamespace(SizeOfImage=0x10000,ImageBase=image_base),
                           get_data=mock.Mock(return_value=raw),close=mock.Mock())
        pe.DIRECTORY_ENTRY_BASERELOC=[SimpleNamespace(entries=[SimpleNamespace(rva=rva,type=kind) for rva,kind in relocs])]
        return pe

    def test_system_main_reuses_pe_and_closes_all_with_full_32byte_comparison(self):
        dxgi=self.fake_pe();core=self.fake_pe(raw=b'C'*32,image_base=0x71000000)
        loaded={'dxgi.dll':(0x70000000,'dxgi.dll'),'d3d12core.dll':(0x71000000,'D3D12Core.dll')}
        addresses=(0x70000100,0x71000200,0x71000300)
        for corrupt in (False,True):
            with self.subTest(corrupt=corrupt):
                self.closed.reset_mock();self.resolver.prepare.reset_mock()
                dxgi.close.reset_mock();core.close.reset_mock()
                for address in addresses:self.memory[address]=bytes(range(32)) if address<0x71000000 else b'C'*32
                if corrupt:self.memory[addresses[-1]]=b'C'*31+b'D'
                self.write_status(tuple(a-self.base for a in addresses))
                with mock.patch.object(verify_stopped.pefile,'PE',side_effect=(dxgi,core)) as parse:
                    if corrupt:
                        with self.assertRaises(SystemExit):self.invoke(loaded)
                    else:self.invoke(loaded)
                self.assertEqual(parse.call_count,2)
                report=json.loads(self.output.read_text());self.assertEqual(report['all_match'],not corrupt)
                self.assertTrue(all(row['bytes_checked']==32 for row in report['checks']))
                self.closed.assert_called_once();dxgi.close.assert_called_once();core.close.assert_called_once()

    def test_preexisting_system_detour_requires_matching_preinstall_and_remote_bytes(self):
        address = 0x70000100
        disk = bytes(range(32))
        preinstalled = b'\xe9\x01\x02\x03\x04' + disk[5:]
        changed = preinstalled[:-1] + b'X'
        cases = (
            ('restored', preinstalled, '1', False, True),
            ('different_after_stop', changed, '1', False, False),
            ('native_restore_failed', preinstalled, '0', False, False),
            ('duplicate_snapshot', preinstalled, '1', True, False),
        )
        for name, observed, native_restored, duplicate, accepted in cases:
            with self.subTest(name=name):
                self.closed.reset_mock();self.resolver.prepare.reset_mock()
                self.memory[address] = observed
                self.write_status((0x296178, address - self.base))
                initial = f'prime_entry target={address:016X} before={preinstalled.hex()}\n'
                final = (f'prime_entry target={address:016X} restored={native_restored} '
                         f'before={preinstalled.hex()}\n')
                text = self.status.read_text().replace('stopped disable_status=0 active=0\n',
                                                       initial + (initial if duplicate else '') + final
                                                       + 'stopped disable_status=0 active=0\n')
                self.status.write_text(text)
                with mock.patch.object(verify_stopped.pefile, 'PE', return_value=self.fake_pe(disk)):
                    if accepted:self.invoke({'dxgi.dll':(0x70000000, 'dxgi.dll')})
                    else:
                        with self.assertRaises(SystemExit):
                            self.invoke({'dxgi.dll':(0x70000000, 'dxgi.dll')})
                report = json.loads(self.output.read_text())
                system = report['checks'][-1]
                self.assertEqual(report['all_match'], accepted)
                self.assertFalse(system['matches_disk'])
                self.assertEqual(system['matches_preinstall'], accepted)
                self.assertEqual(system['matches'], accepted)
                self.closed.assert_called_once()

    def test_split_prime_status_requires_matching_process_and_owner(self):
        address=0x70000100
        disk=bytes(range(32))
        before=b'\xe9\x01\x02\x03\x04'+disk[5:]
        prime=Path(self.directory.name)/'target-prime-status.txt'
        self.memory[address]=before
        self.write_status((0x296178,address-self.base))
        self.status.write_text(
            self.status.read_text().replace('capturing pid=321 flags=fixture\n',
                'capturing pid=321 flags=fixture\n'
                'continuous_owner_verified=1 owner_pid=123 owner_creation_time=456\n')
            +f'prime_entry target={address:016X} restored=1 before={before.hex()}\n')
        content=(f'prime_entry target={address:016X} before={before.hex()}\n'
                 'capturing pid=321 lifetime=owner owner_pid=123 owner_creation_time=456 native_hooks_enabled=4\n')
        prime.write_text(content)
        with mock.patch.object(verify_stopped.pefile,'PE',return_value=self.fake_pe(disk)):
            self.invoke({'dxgi.dll':(0x70000000,'dxgi.dll')},prime)
        report=json.loads(self.output.read_text())
        self.assertTrue(report['all_match'])
        self.assertTrue(report['checks'][-1]['matches_preinstall'])
        self.assertEqual(verify_stopped.prime_preinstall_entries(self.status.read_text(),content),
                         {address:before})
        prime.write_text(content.replace('owner_creation_time=456','owner_creation_time=457'))
        with self.assertRaisesRegex(RuntimeError,'identity does not match'):
            self.invoke({'dxgi.dll':(0x70000000,'dxgi.dll')},prime)

    def test_hook_manifest_restores_preexisting_system_detours_with_exact_identity(self):
        address=0x70000100
        disk=bytes(range(32))
        before=b'\xe9\x01\x02\x03\x04'+disk[5:]
        manifest=Path(self.directory.name)/'hook-entry-manifest.json'
        payload={'schema':1,'pid':321,'game_creation':789,'owner_pid':123,
                 'owner_creation':456,'entries':[{'target':address,'original':before.hex()}]}
        self.memory[address]=before
        self.write_status((0x296178,address-self.base))
        self.status.write_text('continuous_owner_verified=1 owner_pid=123 owner_creation_time=456\n'
                               +self.status.read_text())
        manifest.write_text(json.dumps(payload))
        loaded={'dxgi.dll':(0x70000000,'dxgi.dll')}
        with mock.patch.object(verify_stopped.pefile,'PE',return_value=self.fake_pe(disk)):
            self.invoke(loaded,hook_manifest=manifest,game_creation_time=789)
        report=json.loads(self.output.read_text())
        self.assertTrue(report['all_match'])
        self.assertTrue(report['checks'][-1]['matches_preinstall'])
        self.memory[address]=before[:-1]+b'X'
        with mock.patch.object(verify_stopped.pefile,'PE',return_value=self.fake_pe(disk)):
            with self.assertRaises(SystemExit):
                self.invoke(loaded,hook_manifest=manifest,game_creation_time=789)
        self.assertFalse(json.loads(self.output.read_text())['all_match'])
        self.memory[address]=before
        for key,value in [('owner_creation',457),('game_creation',790),('pid',322)]:
            with self.subTest(key=key):
                altered=dict(payload,**{key:value})
                manifest.write_text(json.dumps(altered))
                with self.assertRaisesRegex(RuntimeError,'identity mismatch'):
                    self.invoke(loaded,hook_manifest=manifest,game_creation_time=789)

    def test_relocation_index_preserves_original_rules_and_application_order(self):
        offset=0x100;raw=bytes(range(32))
        cases=[([],0), ([(offset,10),(offset+24,10)],0x1234),
               ([(offset+8,10),(offset+4,10),(offset+8,10)],-0x30),
               ([(offset-8,3),(offset+32,3),(offset-7,0),(offset+31,0)],0x40),
               ([(offset-7,10)],1), ([(offset+25,10)],1),
               ([(offset+31,10)],1), ([(offset+8,3)],1)]
        for relocs,delta in cases:
            with self.subTest(relocs=relocs,delta=delta):
                expected=bytearray(raw);failure=False
                for rva,kind in relocs:
                    if kind==0 or not offset-7<=rva<offset+32:continue
                    if kind!=10 or not offset<=rva<=offset+24:failure=True;break
                    at=rva-offset;value=struct.unpack_from('<Q',expected,at)[0]
                    struct.pack_into('<Q',expected,at,(value+delta)&0xffffffffffffffff)
                pe=self.fake_pe(raw,relocs);cache=verify_stopped.SystemEntryCache()
                with mock.patch.object(verify_stopped.pefile,'PE',return_value=pe):
                    try:
                        if failure:
                            with self.assertRaisesRegex(RuntimeError,'Unsupported or partial'):cache.expected({'dxgi.dll':(0x70000000+delta,'dxgi.dll')},0x70000000+delta+offset)
                        else:
                            actual,_=cache.expected({'dxgi.dll':(0x70000000+delta,'dxgi.dll')},0x70000000+delta+offset)
                            self.assertEqual(actual,bytes(expected))
                    finally:cache.close()
                pe.close.assert_called_once()

    def test_cache_is_per_run_and_relocation_index_built_once(self):
        scans=[]
        class Entries:
            def __iter__(self):
                scans.append(1);return iter([SimpleNamespace(rva=0x100,type=10)])
        images=[]
        def parse(path):
            image=self.fake_pe();image.DIRECTORY_ENTRY_BASERELOC=[SimpleNamespace(entries=Entries())]
            images.append(image);return image
        with mock.patch.object(verify_stopped.pefile,'PE',side_effect=parse) as parser:
            for _ in range(2):
                cache=verify_stopped.SystemEntryCache()
                try:
                    for offset in (0x100,0x200,0x300):cache.expected({'dxgi.dll':(0x70000000,'dxgi.dll')},0x70000000+offset)
                finally:cache.close()
            self.assertEqual(parser.call_count,2);self.assertEqual(len(scans),2)
        for image in images:image.close.assert_called_once()

    def test_main_read_failure_and_close_error_still_close_every_resource(self):
        for close_error in (False,True):
            with self.subTest(close_error=close_error):
                self.closed.reset_mock();self.resolver.prepare.reset_mock()
                dxgi=self.fake_pe();core=self.fake_pe()
                if close_error:core.close.side_effect=RuntimeError('close failed')
                addresses=(0x70000100,0x71000200)
                self.memory[addresses[0]]=bytes(range(32));self.memory.pop(addresses[1],None)
                self.write_status(tuple(a-self.base for a in addresses))
                loaded={'dxgi.dll':(0x70000000,'dxgi.dll'),'d3d12core.dll':(0x71000000,'core.dll')}
                with mock.patch.object(verify_stopped.pefile,'PE',side_effect=(dxgi,core)):
                    with self.assertRaises((KeyError,RuntimeError)):self.invoke(loaded)
                dxgi.close.assert_called_once();core.close.assert_called_once();self.closed.assert_called_once()

    def test_short_disk_bytes_unknown_module_and_parse_failure_fail_closed(self):
        cache=verify_stopped.SystemEntryCache();pe=self.fake_pe(raw=b'x'*31)
        with mock.patch.object(verify_stopped.pefile,'PE',return_value=pe):
            try:
                with self.assertRaisesRegex(RuntimeError,'Missing on-disk'):cache.expected({'dxgi.dll':(0x70000000,'dxgi.dll')},0x70000100)
            finally:cache.close()
        with mock.patch.object(verify_stopped.pefile,'PE') as parse:
            cache=verify_stopped.SystemEntryCache()
            try:
                with self.assertRaisesRegex(RuntimeError,'allowed graphics system module'):cache.expected({'other.dll':(0x70000000,'other.dll')},0x70000100)
            finally:cache.close()
            parse.assert_not_called()
        first=self.fake_pe();cache=verify_stopped.SystemEntryCache()
        with mock.patch.object(verify_stopped.pefile,'PE',side_effect=(first,RuntimeError('invalid PE'))):
            try:
                with self.assertRaisesRegex(RuntimeError,'invalid PE'):
                    cache.expected({'dxgi.dll':(0x70000000,'dxgi.dll'),'d3d12.dll':(0x71000000,'d3d12.dll')},0x71000100)
            finally:cache.close()
        first.close.assert_called_once()

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

    def test_path_layout_complete_restore_and_tail_corruption(self):
        expected = verify_stopped.PATH_LAYOUT_RANGES
        self.assertEqual(expected, ((0x298bdc, 161,
            'b5dae9f89c8182b26eef27e87ca2ae57804899ed313cb9acdc630280a03b6fb5'),))
        raw = bytes(range(161))
        fixture = ((0x298bdc, len(raw), hashlib.sha256(raw).hexdigest()),)
        for corrupt in (False, True):
            with self.subTest(corrupt=corrupt):
                changed = bytearray(raw)
                if corrupt:changed[-1] ^= 1
                self.memory[self.base + 0x298bdc] = bytes(changed)
                self.write_status((0x298bdc, 0x15cf08))
                with mock.patch.object(verify_stopped, 'PATH_LAYOUT_RANGES', fixture):
                    if corrupt:
                        with self.assertRaises(SystemExit):self.invoke()
                    else:self.invoke()
                check = json.loads(self.output.read_text())['checks'][0]
                self.assertEqual(check['bytes_checked'], 161)
                self.assertEqual(check['matches'], not corrupt)
                self.assertEqual(check['source_snapshot_sha256'],
                    '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')

    def test_path_reflow_contract_covers_all_four_chunks(self):
        self.assertEqual(verify_stopped.PATH_REFLOW_CHUNKS, ((0x2983f8, 666), (0x8ead70, 435)))
        self.assertEqual(verify_stopped.PATH_REFLOW_SHA256,
            '9df4472db8e3ebaca59f6c14356c8de8c685a2aac1a1ff060ca0f7603d945d97')
        self.assertEqual(verify_stopped.PATH_REFLOW_CHILD_CHUNKS, ((0x298694, 201), (0x8eaf24, 39)))
        self.assertEqual(verify_stopped.PATH_REFLOW_CHILD_SHA256,
            'c455b118525aa5c957e5eea29cd8a46479aa2a95ef633693d4aa04da0e15ac28')

    def test_path_reflow_restore_rejects_each_warm_or_cold_tail_and_short_read(self):
        families = (verify_stopped.PATH_REFLOW_CHUNKS, verify_stopped.PATH_REFLOW_CHILD_CHUNKS)
        original = {}
        hashes = []
        for family in families:
            data = []
            for rva, size in family:
                raw = bytes((i % 251 for i in range(size)))
                original[self.base + rva] = raw
                data.append(raw)
            hashes.append(hashlib.sha256(b''.join(data)).hexdigest())
        for slot, expected_rva in ((0x528, 0x2983f8), (0x2c8, 0x298694)):
            original[self.base + 0x138a558 + slot] = (self.base + expected_rva).to_bytes(8, 'little')
        cases = [(None, False)] + [(self.base + rva, short) for family in families
                                  for rva, _ in family for short in (False, True)]
        cases += [(self.base + 0x138a558 + slot, short) for slot in (0x528, 0x2c8) for short in (False, True)]
        for hook_rvas, address, short in [(hook_rvas, address, short)
                for hook_rvas in ((0x2983f8, 0x15cf08), (0x298694, 0x15cf08), (0x298694, 0x2983f8))
                for address, short in cases]:
            with self.subTest(hook_rvas=hook_rvas, address=address, short=short):
                self.resolver.prepare.reset_mock()
                self.closed.reset_mock()
                self.memory.update(original)
                if address is not None:
                    changed = bytearray(self.memory[address])
                    if short:del changed[-1]
                    else:changed[-1] ^= 1
                    self.memory[address] = bytes(changed)
                self.write_status(hook_rvas)
                with mock.patch.object(verify_stopped, 'PATH_REFLOW_SHA256', hashes[0]), \
                        mock.patch.object(verify_stopped, 'PATH_REFLOW_CHILD_SHA256', hashes[1]):
                    if address is None:self.invoke()
                    else:
                        with self.assertRaises(SystemExit):self.invoke()
                report = json.loads(self.output.read_text())
                self.assertEqual(report['all_match'], address is None)
                checks = report['checks'][:2]
                self.assertEqual([item['expected_size'] for item in checks], [1101, 240])
                self.assertEqual([len(item['chunks']) for item in checks], [2, 2])
                self.assertEqual(checks[0]['historical_module_base'], '0x140000000')
                self.assertEqual(checks[1]['source_snapshot_sha256'],
                    '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                self.assertEqual([item['slot'] for item in report['checks'][2:4]], ['0x528', '0x2c8'])
                self.assertEqual([item['target'] for item in checks],
                    [hex(self.base + 0x2983f8), hex(self.base + 0x298694)])

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
                 dict(rvas=(0x296178, 0x296178)), dict(rvas=tuple(range(103)))]
        for values in cases:
            with self.subTest(values=values):
                self.write_status(**values)
                with mock.patch.object(verify_stopped, 'Resolver') as factory, \
                        mock.patch.object(sys, 'argv', ['verify_stopped.py', '--pid', '321',
                            '--status', str(self.status), '--output', str(self.output)]):
                    with self.assertRaises(RuntimeError):verify_stopped.main()
                factory.assert_not_called()
                self.assertFalse(self.output.exists())

    def test_dlss_default_ranges_are_fixed_complete_historical_chunks(self):
        self.assertEqual(verify_stopped.DLSS_OWNED_DOF_RANGES, (
            (0xd7fd14, 0xe61, '62ceae022db3ea1d7fd0994520e2cbff6c1b1dca288023d1a56bd957fbca7d5c'),))
        self.assertEqual(verify_stopped.DLSS_OUTPUT_RANGES, (
            (0xd82f58, 0xc78, '1efb10541bf56d87b31aae4f452da0d281afd552da16b4bfd16d653f2cd425ea'),
            (0xd83bd0, 0x1e9, 'e051a2d325a4d5ebc12be670d63cd8eae1eead437f33dfa39946de852e61e724'),
        ))
        self.assertEqual(verify_stopped.DLSS_INPUT_BIND_CHUNKS, ((0x1665dc, 0x1cc), (0x8b492c, 0x2b)))
        self.assertEqual(verify_stopped.DLSS_INPUT_BIND_SHA256, '566929fd4f84a21529de0ecb749d2dc75a1bf2ab3cc04876a65f65517222a339')

    def test_owned_dof_full_body_and_tail_are_verified_independently(self):
        rva, size, _ = verify_stopped.DLSS_OWNED_DOF_RANGES[0]
        original = bytes(i % 251 for i in range(size))
        ranges = ((rva, size, hashlib.sha256(original).hexdigest()),)
        for changed_at in (None, 0, size - 1, 'short'):
            with self.subTest(changed_at=changed_at):
                current = bytearray(original)
                if isinstance(changed_at, int):current[changed_at] ^= 1
                if changed_at == 'short':current = current[:-1]
                self.memory[self.base + rva] = bytes(current)
                self.write_status((rva, 0x15cf08))
                with self.status.open('a') as stream:stream.write('entries_restored=1 dlss_route_identity match=1\n')
                with mock.patch.object(verify_stopped, 'DLSS_OWNED_DOF_RANGES', ranges):
                    if changed_at is None:self.invoke()
                    else:
                        with self.assertRaises(SystemExit):self.invoke()
                report = json.loads(self.output.read_text())
                self.assertEqual(report['all_match'], changed_at is None)
                self.assertEqual(report['checks'][0]['expected_size'], 0xe61)
                self.assertEqual(report['checks'][0]['source_snapshot_sha256'],
                                 '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                self.read.assert_any_call(self.base + rva, size)

    def dlss_fixture(self):
        # Synthetic test bytes and real SHA256; do not check proprietary chunks
        # into the test suite. Only this test's expected hashes are substituted.
        ranges = []
        for rva, size, _ in verify_stopped.DLSS_OUTPUT_RANGES:
            raw = bytes((i % 251 for i in range(size)))
            self.memory[self.base + rva] = raw
            ranges.append((rva, size, hashlib.sha256(raw).hexdigest()))
        self.write_status((0xd82f58, 0x15cf08))
        return tuple(ranges)

    def test_dlss_output_routes_both_full_ranges_through_independent_hash_checks(self):
        ranges = self.dlss_fixture()
        with mock.patch.object(verify_stopped, 'DLSS_OUTPUT_RANGES', ranges):self.invoke()
        report = json.loads(self.output.read_text())
        self.assertTrue(report['all_match'])
        self.assertEqual(len(report['checks']), 3)
        for check, (rva, size, expected) in zip(report['checks'], ranges):
            self.assertEqual(check['range_rva'], hex(rva))
            self.assertEqual(check['bytes_checked'], size)
            self.assertEqual(check['expected_sha256'], expected)
            self.read.assert_any_call(self.base + rva, size)
        self.closed.assert_called_once()

    def test_dlss_main_or_continuation_tail_drift_fails_despite_status_claims(self):
        ranges = self.dlss_fixture()
        originals = {rva:self.memory[self.base + rva] for rva, _, _ in ranges}
        for rva, size, _ in ranges:
            for at in (0, size - 1):
                with self.subTest(rva=hex(rva), byte=at):
                    self.closed.reset_mock()
                    for old_rva, raw in originals.items():self.memory[self.base + old_rva] = raw
                    changed = bytearray(originals[rva]);changed[at] ^= 1
                    self.memory[self.base + rva] = bytes(changed)
                    with self.status.open('a') as stream:stream.write('entries_restored=1 dlss_route_identity match=1\n')
                    with mock.patch.object(verify_stopped, 'DLSS_OUTPUT_RANGES', ranges):
                        with self.assertRaises(SystemExit) as error:self.invoke()
                    self.assertEqual(error.exception.code, 1)
                    self.assertFalse(json.loads(self.output.read_text())['all_match'])
                    self.closed.assert_called_once()

    def bind_fixture(self):
        data = []
        for rva, size in verify_stopped.DLSS_INPUT_BIND_CHUNKS:
            raw = bytes((i % 251 for i in range(size)))
            self.memory[self.base + rva] = raw;data.append(raw)
        self.write_status((0x1665dc, 0x15cf08))
        return hashlib.sha256(b''.join(data)).hexdigest()

    def test_input_bind_complete_chunk_set_matches_independently(self):
        expected = self.bind_fixture()
        with mock.patch.object(verify_stopped, 'DLSS_INPUT_BIND_SHA256', expected):self.invoke()
        report = json.loads(self.output.read_text())
        self.assertTrue(report['all_match'])
        self.assertEqual(report['checks'][0]['bytes_checked'], 503)
        self.assertEqual(report['checks'][0]['expected_size'], 503)
        for rva, size in verify_stopped.DLSS_INPUT_BIND_CHUNKS:self.read.assert_any_call(self.base + rva, size)
        self.closed.assert_called_once()

    def test_input_bind_main_or_tail_drift_fails_despite_status_claim(self):
        expected = self.bind_fixture()
        originals = {rva:self.memory[self.base + rva] for rva, _ in verify_stopped.DLSS_INPUT_BIND_CHUNKS}
        for rva, size in verify_stopped.DLSS_INPUT_BIND_CHUNKS:
            for at in (0, size - 1):
                with self.subTest(rva=hex(rva), byte=at):
                    self.closed.reset_mock()
                    for old_rva, raw in originals.items():self.memory[self.base + old_rva] = raw
                    changed = bytearray(originals[rva]);changed[at] ^= 1;self.memory[self.base + rva] = bytes(changed)
                    with self.status.open('a') as stream:stream.write('entries_restored=1 dlss_input_bind_identity match=1\n')
                    with mock.patch.object(verify_stopped, 'DLSS_INPUT_BIND_SHA256', expected):
                        with self.assertRaises(SystemExit):self.invoke()
                    self.assertFalse(json.loads(self.output.read_text())['all_match'])
                    self.closed.assert_called_once()

    def test_input_bind_short_read_cannot_pass_even_with_its_own_digest(self):
        self.bind_fixture()
        tail_rva, _ = verify_stopped.DLSS_INPUT_BIND_CHUNKS[-1]
        self.memory[self.base + tail_rva] = self.memory[self.base + tail_rva][:-1]
        expected = hashlib.sha256(b''.join(self.memory[self.base + rva] for rva, _ in verify_stopped.DLSS_INPUT_BIND_CHUNKS)).hexdigest()
        with mock.patch.object(verify_stopped, 'DLSS_INPUT_BIND_SHA256', expected):
            with self.assertRaises(SystemExit):self.invoke()
        self.assertFalse(json.loads(self.output.read_text())['all_match'])

    def test_ngx_wrapper_fixed_complete_historical_ranges(self):
        self.assertEqual(verify_stopped.NGX_WRAPPER_RANGES, (
            (0xbe5b5c, 249, '53709f650d84bd1ea4cc1ae5e25eb8ef7f3462102fe51d07b790d96c48734879'),
            (0xbe5c58, 1377, '21868e6f8203578f5c4ef689fc4f66a5c79e13fdb243eebe2eb81cc7d6309441')))

    def ngx_fixture(self, source=None):
        ranges = []
        for rva, size, _ in (verify_stopped.NGX_WRAPPER_RANGES if source is None else source):
            raw = bytes((i % 251 for i in range(size)))
            self.memory[self.base + rva] = raw
            ranges.append((rva, size, hashlib.sha256(raw).hexdigest()))
        self.write_status(tuple(rva for rva, _, _ in ranges))
        return tuple(ranges)

    def test_ngx_dispatch_complete_ranges_and_tail_rejection(self):
        self.assertEqual([(rva, size) for rva, size, _ in verify_stopped.NGX_DISPATCH_RANGES],
                         [(0x7c82f0, 118), (0x7c8440, 53), (0x7c87e0, 69)])
        ranges = self.ngx_fixture(verify_stopped.NGX_DISPATCH_RANGES)
        originals = {rva:self.memory[self.base + rva] for rva, _, _ in ranges}
        with mock.patch.object(verify_stopped, 'NGX_DISPATCH_RANGES', ranges):
            self.invoke()
            checks = json.loads(self.output.read_text())['checks']
            self.assertEqual([row['bytes_checked'] for row in checks], [118, 53, 69])
            self.assertTrue(all(row['matches'] for row in checks))
            for rva, _, _ in ranges:
                for short in (False, True):
                    with self.subTest(rva=hex(rva), short=short):
                        for old_rva, raw in originals.items():self.memory[self.base + old_rva] = raw
                        raw = originals[rva]
                        self.memory[self.base + rva] = raw[:-1] if short else raw[:-1] + bytes([raw[-1] ^ 1])
                        with self.status.open('a') as stream:stream.write('entries_restored=1\n')
                        with self.assertRaises(SystemExit):self.invoke()
                        self.assertFalse(json.loads(self.output.read_text())['all_match'])

    def test_ngx_wrapper_roles_independently_check_whole_chunks(self):
        ranges = self.ngx_fixture()
        with mock.patch.object(verify_stopped, 'NGX_WRAPPER_RANGES', ranges):self.invoke()
        checks = json.loads(self.output.read_text())['checks']
        self.assertEqual([check['bytes_checked'] for check in checks], [249, 1377])
        self.assertTrue(all(check['matches'] for check in checks))
        for check in checks:self.assertEqual(check['source_snapshot_sha256'], '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')

    def test_ngx_wrapper_tail_drift_or_short_reads_reject_status_claims(self):
        ranges = self.ngx_fixture()
        originals = {rva:self.memory[self.base + rva] for rva, _, _ in ranges}
        for rva, _, _ in ranges:
            for short in (False, True):
                with self.subTest(rva=hex(rva), short=short):
                    for old_rva, raw in originals.items():self.memory[self.base + old_rva] = raw
                    self.memory[self.base + rva] = originals[rva][:-1] if short else originals[rva][:-1] + bytes([originals[rva][-1] ^ 1])
                    with self.status.open('a') as stream:stream.write('entries_restored=1 ngx_route_identity match=1\n')
                    with mock.patch.object(verify_stopped, 'NGX_WRAPPER_RANGES', ranges):
                        with self.assertRaises(SystemExit):self.invoke()
                    self.assertFalse(json.loads(self.output.read_text())['all_match'])


if __name__ == '__main__':
    unittest.main()
