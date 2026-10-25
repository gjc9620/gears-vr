"""Synthetic receipt/script diagnostics only; no proprietary script or runtime I/O."""
import copy
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import cover_call_config
import cover_call_decode as decode
from test_cover_call_config import Fixture


class Receipts:
    def __init__(self):
        fixture = Fixture()
        self.config, _ = cover_call_config.prepare(fixture.sampler)
        self.fields = {}
        for role in (0, 2):
            function = self.config['functions'][role]
            self.fields[role] = {item['name']: item for item in function['fields']}
            raw = bytearray(b'\x25' * 192)
            for start, names in ((32, decode.ARGUMENTS), (96, ('PawnCA',)), (128, ('PawnCD',))):
                body = self.parameters(role, names)
                raw[start:start + len(body)] = body
            self.script(role, bytes(raw))

    def address(self, field):
        return self.config['identities'][field['identity']]['address']

    def parameters(self, role, names):
        return b''.join(b'\x00' + struct.pack('<Q', self.address(self.fields[role][name])) for name in names) + b'\x16'

    def script(self, role, raw):
        function = self.config['functions'][role]
        script = function['script']
        pointer, old_count = script['address'], script['count']
        header = struct.pack('<Qii', pointer, len(raw), len(raw))
        script.update(count=len(raw), capacity=len(raw), header_raw=header.hex(), bytes=raw.hex(),
                      sha256=hashlib.sha256(raw).hexdigest())
        checks = self.config['source_checks']
        checks[:] = [item for item in checks if (item['address'], item['size']) != (pointer, old_count)]
        for item in checks:
            if (item['address'], item['size']) == (self.address(function) + 0x8c, 16):
                item['bytes'] = header.hex()
        checks.append(dict(address=pointer, size=len(raw), bytes=raw.hex()))

    def prefix(self, role, ca, cd):
        raw = bytearray(self.config['functions'][role]['locals_size'])
        for name, value in (('PawnCT', 2), ('PawnCA', ca), ('PawnCD', cd)):
            raw[self.fields[role][name]['offset']] = value
        return raw

    def chain(self, *, role=0, invocation=1, arm=1, base_index=0, before=(0, 0), after=(7, 3)):
        controller = self.address({'identity': self.config['roles']['controller']})
        result = []
        for offset, target, code, end in ((0, role + 1, 32, 78), (1, 4, 96, 106), (2, 5, 128, 138)):
            index = base_index + offset
            first = before if offset == 0 else after
            row = dict(index=str(index), invocation=str(invocation), parent='0', world='1', arm=str(arm),
                       role=str(target), caller_role=str(role), thread='1234', receiver=f'{controller:016X}',
                       caller='0000000000400000', callee=f'{self.address(self.config["functions"][target]):016X}',
                       result_address='0000000000600008', opaque_rax='00000000DEADBEEF', begin=str(100 + index * 20),
                       end=str(110 + index * 20), returned='1', before='accepted', after='accepted', same_storage='1',
                       code_before=str(code), code_after=str(end), ct_before='2', ca_before=str(first[0]), cd_before=str(first[1]),
                       ct_after='2', ca_after=str(after[0]), cd_after=str(after[1]), locals_diagnostic='00000000003FF000',
                       mutation_authorized='0')
            result.append(('cover_call_row', row))
            script = bytes.fromhex(self.config['functions'][role]['script']['bytes'])
            for endpoint, pc, values in (('entry', code, first), ('return', end, after)):
                start = min(max(0, pc - 16), len(script) - 32)
                result.append(('cover_call_bytes', dict(index=str(index), endpoint=endpoint, code_window_offset=str(start),
                    script=script[start:start + 32].hex(), locals_prefix=self.prefix(role, *values).hex())))
        return result

    @staticmethod
    def text(records):
        return ('cover_calls compiled=1 enabled=1 observation_only=1 mutation_authorized=0 explicit_arm=1 '
                'arms=2 candidates=12 began=2 sealed=2 rejected=0 dropped=0 last_source_failure=0000000000000000\n' +
                '\n'.join(kind + ' ' + ' '.join(key + '=' + value for key, value in item.items()) for kind, item in records) + '\n')

    def wrapped(self):
        digest = hashlib.sha256(json.dumps(self.config, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        return dict(config=self.config, provenance=dict(config_sha256=digest))


class DecodeTests(unittest.TestCase):
    def setUp(self):
        self.receipts = Receipts()

    def analyze(self, records=None, config=None):
        fixture = self.receipts
        return decode.analyze(fixture.text(records if records is not None else fixture.chain()),
                              config if config is not None else fixture.config)

    def test_change_requires_exact_parameters_raw_values_and_same_storage_setters(self):
        with patch('builtins.open', side_effect=AssertionError('decoder performed I/O')):
            result = self.analyze()
        self.assertTrue(result['accepted'])
        self.assertEqual(result['out_storage_change_groups'], 1)
        group = result['groups'][0]
        self.assertEqual(group['changed_fields'], ['ca', 'cd'])
        self.assertEqual(group['updater_before'], dict(ct=2, ca=0, cd=0))
        self.assertEqual(group['updater_after'], dict(ct=2, ca=7, cd=3))
        self.assertFalse(group['activation_allowed'])
        self.assertTrue(group['return_buffer_semantics_unproven'])
        self.assertFalse(result['result_buffer_read'])
        self.assertFalse(result['raw_property_flags_interpreted'])
        self.assertEqual(group['rows'][0]['argument_names'], list(decode.ARGUMENTS))
        self.assertEqual(group['rows'][0]['result_address'], 0x600008)
        json.dumps(result, allow_nan=False)

    def test_distinct_stable_arms_do_not_manufacture_an_updater_change(self):
        fixture = self.receipts
        records = fixture.chain(before=(0, 0), after=(0, 0)) + fixture.chain(
            invocation=2, arm=2, base_index=3, before=(7, 3), after=(7, 3))
        result = self.analyze(records, fixture.wrapped())
        self.assertEqual(result['accepted_groups'], 2)
        self.assertEqual(result['continuity_only_groups'], 2)
        self.assertEqual(result['out_storage_change_groups'], 0)
        self.assertTrue(all(group['conclusion'] == 'call_and_storage_continuity_only' for group in result['groups']))

    def test_stationary_uses_its_own_current_function_and_property_addresses(self):
        result = self.analyze(self.receipts.chain(role=2))
        self.assertTrue(result['accepted'])
        self.assertEqual(result['groups'][0]['caller_function'], 'PlayerTakingCover_Stationary.PlayerMove')
        self.assertEqual([row['role'] for row in result['groups'][0]['rows']], [3, 4, 5])

    def test_local_bytes_use_reflected_offsets_instead_of_fixed_positions(self):
        fixture = self.receipts
        for name, offset in zip(('PawnCT', 'PawnCA', 'PawnCD'), (12, 13, 14)):
            field = fixture.fields[0][name]
            field['offset'] = offset
            address = fixture.address(field) + 0x94
            next(item for item in fixture.config['source_checks']
                 if (item['address'], item['size']) == (address, 4))['bytes'] = struct.pack('<I', offset).hex()
        result = self.analyze(fixture.chain())
        self.assertTrue(result['accepted'])
        self.assertEqual(result['groups'][0]['updater_after'], dict(ct=2, ca=7, cd=3))

    def test_row_and_endpoint_bytes_are_checked_not_just_native_acceptance_labels(self):
        for damage in ('window', 'prefix', 'window_offset', 'window_short', 'prefix_short', 'prefix_long',
                       'before', 'after', 'returned', 'same_storage', 'callee', 'receiver', 'role', 'raw_value',
                       'code_before', 'code_after', 'caller_alignment', 'locals_overflow', 'missing_arm'):
            with self.subTest(damage=damage):
                records = self.receipts.chain()
                row, endpoint = records[0][1], records[1][1]
                if damage in ('window', 'prefix'):
                    key = 'script' if damage == 'window' else 'locals_prefix'
                    raw = bytearray.fromhex(endpoint[key]);raw[0] ^= 1;endpoint[key] = raw.hex()
                elif damage == 'window_offset':endpoint['code_window_offset'] = '999'
                elif damage == 'window_short':endpoint['script'] = endpoint['script'][:-2]
                elif damage == 'prefix_short':endpoint['locals_prefix'] = endpoint['locals_prefix'][:-2]
                elif damage == 'prefix_long':endpoint['locals_prefix'] += '00'
                elif damage in ('before', 'after'):row[damage] = 'storage_changed'
                elif damage in ('returned', 'same_storage'):row[damage] = '0'
                elif damage in ('callee', 'receiver'):row[damage] = '0000000000000008'
                elif damage == 'role':row['role'] = '3'
                elif damage == 'raw_value':row['ca_before'] = '256'
                elif damage in ('code_before', 'code_after'):row[damage] = '192'
                elif damage == 'caller_alignment':row['caller'] = '0000000000400001'
                elif damage == 'locals_overflow':row['locals_diagnostic'] = 'FFFFFFFFFFFFFFF8'
                elif damage == 'missing_arm':del row['arm']
                self.assertFalse(self.analyze(records)['accepted'])

    def test_parameter_body_rejects_wrong_alias_opcode_endparm_and_length(self):
        for target in (0, 1, 2):
            for damage in ('property', 'opcode', 'endparm', 'extra_argument'):
                with self.subTest(target=target, damage=damage):
                    fixture = Receipts()
                    script = bytearray.fromhex(fixture.config['functions'][0]['script']['bytes'])
                    start, end = ((32, 78), (96, 106), (128, 138))[target]
                    if damage == 'property':script[start + 1] ^= 8
                    elif damage == 'opcode':script[start] = 1
                    elif damage == 'endparm':script[end - 1] = 0
                    else:script[end - 1:end + 8] = b'\x00' + struct.pack('<Q', 0x77778888)
                    fixture.script(0, script)
                    result = decode.analyze(fixture.text(fixture.chain()), fixture.config)
                    self.assertFalse(result['accepted'])
                    self.assertIn('parameter body', result['groups'][0]['reason'])

    def test_metadata_corruption_or_unbound_fields_reject_whole_config(self):
        for damage in ('script_hash', 'source', 'duplicate_function', 'field_width', 'delta_type', 'flags', 'offset', 'provenance'):
            with self.subTest(damage=damage):
                fixture = Receipts()
                cfg = fixture.config
                if damage == 'script_hash':cfg['functions'][0]['script']['sha256'] = '0' * 64
                elif damage == 'source':cfg['source_checks'][0]['bytes'] = '00' * cfg['source_checks'][0]['size']
                elif damage == 'duplicate_function':cfg['functions'][1] = copy.deepcopy(cfg['functions'][0])
                elif damage == 'field_width':fixture.fields[0]['PawnCA']['width'] = 4
                elif damage == 'delta_type':fixture.fields[0]['DeltaTime']['type'] = 'IntProperty'
                elif damage == 'flags':fixture.fields[0]['bNewLeftSided']['flags_raw'] = '00' * 8
                elif damage == 'offset':fixture.fields[0]['PawnCD']['offset'] = 31
                supplied = fixture.wrapped()
                if damage == 'provenance':supplied['provenance']['config_sha256'] = '0' * 64
                result = decode.analyze(fixture.text(fixture.chain()), supplied)
                self.assertFalse(result['accepted'])
                self.assertEqual(result['errors'][0]['stage'], 'config_or_receipt_shape')

    def test_contexts_and_invocations_cannot_be_stitched_into_an_observed_change(self):
        for key, value in (('invocation', '2'), ('arm', '2'), ('world', '2'), ('thread', '5678'),
                           ('caller', '0000000000401000'), ('locals_diagnostic', '00000000003FE000'),
                           ('caller_role', '2'), ('parent', '1')):
            with self.subTest(key=key):
                records = self.receipts.chain()
                records[3][1][key] = value
                result = self.analyze(records)
                self.assertFalse(result['accepted'])
                self.assertEqual(result['out_storage_change_groups'], 0)

    def test_call_order_timing_and_setter_storage_continuity_are_required(self):
        for damage in ('order', 'overlap', 'different_setter_value'):
            with self.subTest(damage=damage):
                records = self.receipts.chain()
                if damage == 'order':records = records[3:6] + records[:3] + records[6:]
                elif damage == 'overlap':records[3][1]['begin'] = '105'
                else:
                    records[3][1]['ca_before'] = '6'
                    raw = bytearray.fromhex(records[4][1]['locals_prefix'])
                    raw[self.receipts.fields[0]['PawnCA']['offset']] = 6
                    records[4][1]['locals_prefix'] = raw.hex()
                self.assertFalse(self.analyze(records)['accepted'])

    def test_duplicate_or_missing_receipts_reject_instead_of_choosing_one(self):
        records = self.receipts.chain()
        for changed in (records + [records[0]], records + [records[1]], records[:-1], records[1:]):
            with self.subTest(length=len(changed)):
                result = self.analyze(changed)
                self.assertFalse(result['accepted'])
                self.assertTrue(result['errors'])
        text = self.receipts.text(records).replace('index=0 invocation=', 'index=0 index=0 invocation=', 1)
        self.assertFalse(decode.analyze(text, self.receipts.config)['accepted'])

    def test_cli_writes_only_explicit_output_and_preserves_evidence_inputs(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            status, config, output = (root / name for name in ('status.txt', 'config.json', 'diagnostic.json'))
            status.write_text(self.receipts.text(self.receipts.chain()), encoding='utf-8')
            config.write_text(json.dumps(self.receipts.wrapped()), encoding='utf-8')
            before = status.read_bytes(), config.read_bytes()
            self.assertEqual(decode.main(['--status', str(status), '--config', str(config), '--output', str(output)]), 0)
            self.assertEqual((status.read_bytes(), config.read_bytes()), before)
            self.assertEqual({path.name for path in root.iterdir()}, {'status.txt', 'config.json', 'diagnostic.json'})
            self.assertFalse(json.loads(output.read_text())['activation_allowed'])
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                decode.main(['--status', str(status), '--config', str(config), '--output', str(status)])
            self.assertEqual(status.read_bytes(), before[0])


if __name__ == '__main__':
    unittest.main()
