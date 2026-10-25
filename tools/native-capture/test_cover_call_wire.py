"""Synthetic packet tests. Optional native parser path enables full round-trip."""
import copy
import hashlib
import json
import os
import pathlib
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zlib

import cover_call_config
import cover_call_wire as wire
from test_cover_call_config import Fixture


class WireTests(unittest.TestCase):
    def setUp(self):
        self.fixture = Fixture()
        self.config, self.provenance = cover_call_config.prepare(self.fixture.sampler)

    def test_pure_encoding_layout_crc_provenance_and_complete_sources(self):
        original = copy.deepcopy(self.config)
        with patch('builtins.open', side_effect=AssertionError('wire opened file')):
            packet = wire.encode(self.config)
        self.assertEqual(self.config, original)
        self.assertEqual(wire.HEADER.size, 96)
        self.assertEqual(wire.FUNCTION.size, 56)
        header = wire.HEADER.unpack_from(packet)
        self.assertEqual(header[:8], (wire.MAGIC, 1, 0, len(packet), self.fixture.pid, 6, len(self.config['source_checks']), 0))
        self.assertEqual(header[8], self.fixture.base)
        self.assertEqual(header[9:12], (self.fixture.pc, self.fixture.pawn, self.fixture.cam))
        self.assertEqual(header[12].hex(), self.provenance['config_sha256'])
        self.assertEqual(struct.unpack_from('<I', packet, len(packet) - 4)[0], zlib.crc32(packet[:-4]) & 0xffffffff)
        position = wire.HEADER.size
        for index, function in enumerate(self.config['functions']):
            record = wire.FUNCTION.unpack_from(packet, position)
            self.assertEqual(record[:2], (index, 0))
            self.assertEqual(record[2:6], (self.config['identities'][function['identity']]['address'],
                                         function['script']['address'], function['script']['count'], function['locals_size']))
            if index in (0, 2):
                names = {f['name']: f for f in function['fields']}
                self.assertEqual(record[6:], tuple(value for name in ('PawnCT', 'PawnCA', 'PawnCD')
                                                   for value in (names[name]['offset'], 1)))
            else:
                self.assertEqual(record[6:], (0,) * 6)
            position += wire.FUNCTION.size
        for expected in self.config['source_checks']:
            address, size, flags = wire.CHECK.unpack_from(packet, position)
            position += wire.CHECK.size
            self.assertEqual((address, size, flags), (expected['address'], expected['size'], 0))
            self.assertEqual(packet[position:position + size].hex(), expected['bytes'])
            position += size
        self.assertEqual(position, len(packet) - 4)

    def test_python_dispatcher_certificate_matches_existing_header(self):
        certificate = cover_call_config._boundary_certificate()
        self.assertEqual(wire.DISPATCHER_RVA, certificate['rva'])
        self.assertEqual(wire.DISPATCHER_BYTES, certificate['bytes'])

    def test_rejects_function_role_local_and_authority_shape(self):
        cases = ('schema_bool', 'mutation', 'aliasing', 'runtime', 'pid_bool', 'base_overflow',
                 'role_alias', 'identity_duplicate', 'function_missing', 'function_duplicate', 'function_order',
                 'function_owner', 'numbered_function', 'script_hash', 'script_header', 'script_capacity',
                 'locals_size', 'entry', 'local_missing', 'local_duplicate', 'local_width', 'local_dimension',
                 'local_type', 'local_owner', 'local_containment', 'interpreted_flags')
        for case in cases:
            with self.subTest(case=case):
                c = copy.deepcopy(self.config)
                f = c['functions'][0]
                local = next(x for x in f['fields'] if x['name'] == 'PawnCT')
                if case == 'schema_bool': c['schema'] = True
                elif case == 'mutation': c['mutation_authorized'] = True
                elif case == 'aliasing': c['callsite_aliasing_verified'] = True
                elif case == 'runtime': c['runtime_observation'] = True
                elif case == 'pid_bool': c['pid'] = True
                elif case == 'base_overflow': c['base'] = (1 << 64) - 8
                elif case == 'role_alias': c['roles']['pawn'] = c['roles']['controller']
                elif case == 'identity_duplicate': c['identities'].append(copy.deepcopy(c['identities'][0]))
                elif case == 'function_missing': c['functions'].pop()
                elif case == 'function_duplicate': c['functions'][1] = copy.deepcopy(f)
                elif case == 'function_order': c['functions'][0], c['functions'][1] = c['functions'][1], c['functions'][0]
                elif case == 'function_owner': f['owner'] = c['roles']['war_pc']
                elif case == 'numbered_function': c['identities'][f['identity']]['number'] = 1
                elif case == 'script_hash': f['script']['sha256'] = '00' * 32
                elif case == 'script_header': f['script']['header_raw'] = '00' * 16
                elif case == 'script_capacity': f['script']['capacity'] = 0
                elif case == 'locals_size': f['locals_size'] = wire.MAX_LOCALS + 1
                elif case == 'entry': f['entry'] += 8
                elif case == 'local_missing': f['fields'].remove(local)
                elif case == 'local_duplicate': f['fields'].append(copy.deepcopy(local))
                elif case == 'local_width': local['width'] = 2; local['storage_width'] = 2
                elif case == 'local_dimension': local['dimension'] = 2; local['storage_width'] = 2
                elif case == 'local_type': local['type'] = 'IntProperty'
                elif case == 'local_owner': local['owner'] = c['roles']['war_pc']
                elif case == 'local_containment': local['offset'] = f['locals_size']
                elif case == 'interpreted_flags': local['flags_interpreted'] = True
                with self.assertRaises(wire.WireRejected):
                    wire.encode(c)

    def test_requires_verified_dispatcher_and_exact_original_check(self):
        for case in ('unverified', 'wrong_entry', 'missing', 'check_bytes', 'larger_span'):
            with self.subTest(case=case):
                c = copy.deepcopy(self.config)
                selected = next(x for x in c['source_checks'] if x['address'] == c['base'] + wire.DISPATCHER_RVA and x['size'] == 32)
                if case == 'unverified': c['dispatcher']['compared_in_current_process'] = False
                elif case == 'wrong_entry': c['dispatcher']['entry'] = '00' * 32
                elif case == 'missing': c['source_checks'].remove(selected)
                elif case == 'check_bytes': selected['bytes'] = '00' * 32
                elif case == 'larger_span': selected['size'] = 33; selected['bytes'] += '00'
                with self.assertRaises(wire.WireRejected):
                    wire.encode(c)

    def test_rejects_invalid_every_source_span_and_duplicate_or_oversized_packet(self):
        for case in ('zero_address', 'overflow', 'zero_size', 'oversize', 'length', 'hex', 'duplicate', 'check_count', 'packet_size'):
            with self.subTest(case=case):
                c = copy.deepcopy(self.config)
                selected = c['source_checks'][0]
                if case == 'zero_address': selected['address'] = 0
                elif case == 'overflow': selected['address'] = (1 << 64) - 1
                elif case == 'zero_size': selected['size'] = 0; selected['bytes'] = ''
                elif case == 'oversize': selected['size'] = wire.MAX_SOURCE + 1; selected['bytes'] = '00' * selected['size']
                elif case == 'length': selected['bytes'] += '00'
                elif case == 'hex': selected['bytes'] = 'z' * len(selected['bytes'])
                elif case == 'duplicate': c['source_checks'].append(copy.deepcopy(selected))
                elif case == 'check_count': c['source_checks'] = [selected] * (wire.MAX_CHECKS + 1)
                elif case == 'packet_size':
                    for index in range(16):
                        c['source_checks'].append(dict(address=0x100000000 + index * wire.MAX_SOURCE,
                                                       size=wire.MAX_SOURCE, bytes='00' * wire.MAX_SOURCE))
                with self.assertRaises(wire.WireRejected):
                    wire.encode(c)

    def test_function_metadata_must_be_bound_to_the_retained_source_checks(self):
        f = self.config['functions'][0]
        address = self.config['identities'][f['identity']]['address']
        for span in ((address + 0x88, 4), (address + 0x8c, 16), (address + 0x114, 8),
                     (f['script']['address'], f['script']['count'])):
            with self.subTest(span=span):
                c = copy.deepcopy(self.config)
                c['source_checks'] = [x for x in c['source_checks'] if (x['address'], x['size']) != span]
                with self.assertRaises(wire.WireRejected):
                    wire.encode(c)

    @unittest.skipUnless(os.environ.get('GEARS_COVER_CALL_WIRE_TEST_EXE'), 'native parser executable not supplied')
    def test_cross_language_packet_round_trip(self):
        packet = wire.encode(self.config)
        with tempfile.TemporaryDirectory(prefix='gears-cover-wire-') as folder:
            path = pathlib.Path(folder) / 'synthetic.bin'
            path.write_bytes(packet)
            result = subprocess.run([os.environ['GEARS_COVER_CALL_WIRE_TEST_EXE'], '--packet', str(path)],
                                    check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
        decoded = json.loads(result.stdout.decode('utf-8'))
        self.assertEqual(decoded['pid'], self.config['pid'])
        self.assertEqual(decoded['base'], self.config['base'])
        self.assertEqual(decoded['controller'], self.fixture.pc)
        self.assertEqual(decoded['pawn'], self.fixture.pawn)
        self.assertEqual(decoded['camera'], self.fixture.cam)
        self.assertEqual(decoded['config_sha256'], self.provenance['config_sha256'])
        self.assertFalse(decoded['mutation_authorized'])
        self.assertEqual(decoded['checks'], [dict(address=x['address'], bytes=x['bytes']) for x in self.config['source_checks']])
        for index, (actual, expected) in enumerate(zip(decoded['functions'], self.config['functions'])):
            self.assertEqual(actual['address'], self.config['identities'][expected['identity']]['address'])
            self.assertEqual(actual['script'], expected['script']['address'])
            self.assertEqual(actual['size'], expected['script']['count'])
            self.assertEqual(actual['locals_size'], expected['locals_size'])
            pairs = actual['locals']
            if index in (0, 2):
                fields = {x['name']: x for x in expected['fields']}
                self.assertEqual(pairs, [[fields[name]['offset'], 1] for name in ('PawnCT', 'PawnCA', 'PawnCD')])
            else:
                self.assertEqual(pairs, [[0, 0]] * 3)


if __name__ == '__main__':
    unittest.main()
