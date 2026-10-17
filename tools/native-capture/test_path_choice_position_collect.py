"""Offline acquisition orchestration with synthetic memory and real serializers."""
import copy
from dataclasses import FrozenInstanceError
import json
from pathlib import Path
import struct
from types import SimpleNamespace
import unittest
from unittest import mock

import path_choice_position_collect as collector
import test_path_choice_close_config as fixtures


def fixture():
    sample, proof, metadata = fixtures.fixture()
    panel = sample['tree'][1]
    rows = []
    for i, (name, kind, raw, units) in enumerate(collector.SUPPORTED_LAYOUT):
        if name == 'UIWarSafeRegionPanel': row = copy.deepcopy(panel)
        else:
            row = dict(address=hex(0xf00000+i*0x100), parent=sample['top']['address'],
                       outer=sample['top']['address'], number=0, depth=3, layout_details=dict(position=dict(members={})))
        row.update(name=name, cls=kind)
        members = row['layout_details']['position']['members']
        members.setdefault('Value', {}).update(raw=raw, values=list(struct.unpack('<4f', bytes.fromhex(raw))))
        members.setdefault('ScaleType', {}).update(raw=units, values=list(bytes.fromhex(units)))
        if kind == 'UIScene_PathChoice': row.update(address=sample['top']['address'], parent=None)
        rows.append(row)
    sample['tree'] = rows
    memory = {}
    for check in proof['checks']:
        memory.update((check['address']+i,b) for i,b in enumerate(bytes.fromhex(check['raw'])))
    panel_address = int(panel['address'], 0)
    panel_raw = next(s[2] for s in collector.SUPPORTED_LAYOUT if s[0] == 'UIWarSafeRegionPanel')
    memory.update((panel_address+0x60+i,b) for i,b in enumerate(bytes.fromhex(panel_raw)))
    proof['checks'] = []
    base = sample['scene_owner']['module_base']
    r = SimpleNamespace(pid=123, base=base, ident_cache={}, names_cache={})
    def read(address, size): return bytes(memory[address+i] for i in range(size))
    r.read = read
    for prefix, singular, rva in (('objects','object',0x1c24730), ('names','name',0x1c34800)):
        header=base+rva; data,count,capacity=struct.unpack('<Qii',read(header,16))
        setattr(r,prefix+'_header',header);setattr(r,prefix+'_data',data)
        setattr(r,singular+'_count',count);setattr(r,singular+'_capacity',capacity)
    reader = SimpleNamespace(r=r, client=sample['scene_owner']['roles']['client']['address'],
        client_identity=copy.deepcopy(sample['scene_owner']['roles']['client']), properties={})
    reader.force_registry_refresh = mock.Mock()
    def acquire(**options):
        # Model the fresh MenuReader acquisition, including its observed sources.
        for address in sorted(memory): r.read(address,1)
        return copy.deepcopy(sample)
    reader._sample_once = mock.Mock(side_effect=acquire)
    session = dict(phase='running', native_started_qpc=789,
        game_identity=dict(pid=123,creation_time=456,image=str(Path('synthetic-game.exe').resolve())),
        owner_identity=dict(pid=42,creation_time=101,image=str(Path('synthetic-owner.exe').resolve())))
    status = dict(phase='running',started_qpc='789',world_generation='2',
        **{'path_position_'+key:'0' for key in ('identity_lost','ownership_lost','setter_failed','pending_restore','unresolved')})
    metadata['session_binding'] = collector._binding(session)
    return reader,sample,proof,metadata,session,status,memory


class CollectorTests(unittest.TestCase):
    def invoke(self, values, verify=None):
        reader,sample,proof,metadata,session,status,memory=values
        with mock.patch.object(collector,'_panel_proof',side_effect=lambda *a:copy.deepcopy(proof)), \
             mock.patch.object(collector,'_close_metadata',side_effect=lambda *a:copy.deepcopy(metadata)):
            return collector.collect(reader,sample,status,session,1000,verify or (lambda r:copy.deepcopy(session)))

    def test_single_fresh_admission_immutable_and_no_retained_baseline(self):
        values=fixture();reader=values[0];original=reader.r.read
        receipt=self.invoke(values)
        self.assertIn('vertical_offset_reference_pixels=374',receipt.position_ini)
        self.assertIn('duration_ms=6000',receipt.position_ini)
        self.assertIn('duration_ms=8000',receipt.close_ini)
        self.assertEqual(json.loads(receipt.baseline_ids_json)['nonce'],1000)
        self.assertTrue(json.loads(receipt.evidence_json)['consistent_final_reread'])
        reader.force_registry_refresh.assert_called_once();reader._sample_once.assert_called_once()
        self.assertIs(reader.r.read,original)
        with self.assertRaises(FrozenInstanceError):receipt.close_ini='changed'
        # A second activation must independently reacquire rather than reuse.
        self.invoke(values)
        self.assertEqual(reader.force_registry_refresh.call_count,2)
        self.assertEqual(reader._sample_once.call_count,2)

    def test_bad_session_owner_and_native_start_stop_before_refresh(self):
        for field in ('owner_identity','game_identity'):
            values=fixture();other=copy.deepcopy(values[4]);other[field]['creation_time']+=1
            with self.assertRaisesRegex(ValueError,'binding changed'):self.invoke(values,lambda r:other)
            values[0].force_registry_refresh.assert_not_called()
        values=fixture();values[5]['started_qpc']='790'
        with self.assertRaisesRegex(ValueError,'start mismatch'):self.invoke(values)

    def test_session_change_after_capture_yields_no_receipt(self):
        values=fixture();calls=[]
        def verify(reader):
            calls.append(1);current=copy.deepcopy(values[4])
            if len(calls)==3:current['owner_identity']['pid']+=1
            return current
        with self.assertRaisesRegex(ValueError,'binding changed'):self.invoke(values,verify)
        self.assertEqual(len(calls),3)

    def test_changed_final_read_restores_reader_and_stops(self):
        values=fixture();reader=values[0];base_read=reader.r.read
        # During final verification the original function sees itself restored
        # only after finally; the third repeated complete header is final reread.
        header=reader.r.objects_header;counts={}
        def changing(address,size):
            key=(address,size);counts[key]=counts.get(key,0)+1
            raw=base_read(address,size)
            if key==(header,16) and counts[key]>=3:return bytes([raw[0]^1])+raw[1:]
            return raw
        reader.r.read=changing
        with self.assertRaises(ValueError):self.invoke(values)
        self.assertIs(reader.r.read,changing)

    def test_owner_reflection_and_geometry_proof_rejections(self):
        for mutation in ('reflection','owner','geometry','pending'):
            values=fixture();sample=values[1]
            if mutation=='reflection':
                row=next(r for r in sample['tree'] if r['name']=='UIWarSafeRegionPanel')
                row['layout_details']['position']['members']['ScaleType']['reflected']['offset']=15
            elif mutation=='owner':sample['scene_owner']['links']['scene_player_owner']['value']+=8
            elif mutation=='geometry':sample['tree'][0]['layout_details']['position']['members']['Value']['raw']='00'*16
            else:values[5]['path_position_pending_restore']='1'
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):self.invoke(values)

    def test_fresh_original_identity_and_inventory_races_fail_once(self):
        values=fixture();reader=values[0]
        original=reader._sample_once.side_effect
        def changed(**options):
            fresh=original(**options);fresh['scene_owner']['roles']['scene']['address']+=8;return fresh
        reader._sample_once.side_effect=changed
        with self.assertRaisesRegex(ValueError,'baseline changed'):self.invoke(values)
        reader._sample_once.assert_called_once()

    def test_close_metadata_requires_exact_ancestry_owned_function_and_native_pointer(self):
        values=fixture();reader,sample,proof,meta,session,status,memory=values
        client=sample['scene_owner']['roles']['client'];declaring=meta['owner_class'];function=meta['function']
        package=dict(address=declaring['outer'],name='Engine',number=0,outer=0,cls=0x111100)
        items={row['identity']['address']:row['identity'] for row in sample['scene_owner']['classes']}
        items.update({client['address']:client,declaring['address']:declaring,function['address']:function,package['address']:package})
        items[function['cls']]=dict(name='Function');items[package['cls']]=dict(name='Package')
        cap=SimpleNamespace(identities=items,identity=lambda address:items[address],
            class_identity=lambda address:items[address],chain=lambda address:[client['cls'],declaring['address']],
            class_fields=lambda address:[function],raw=lambda address,size:bytes.fromhex(meta['header_hex']),finish=mock.Mock())
        with mock.patch.object(collector,'_Capture',return_value=cap):
            receipt=collector._close_metadata(reader.r,sample,collector._binding(session))
            self.assertEqual(receipt['function'],function);cap.finish.assert_called_once()
            package['name']='Foreign'
            with self.assertRaisesRegex(ValueError,'package'):collector._close_metadata(reader.r,sample,collector._binding(session))
            package['name']='Engine';function['outer']+=8
            with self.assertRaisesRegex(ValueError,'Function required'):collector._close_metadata(reader.r,sample,collector._binding(session))
            function['outer']-=8
            raw=bytearray.fromhex(meta['header_hex']);raw[0x114]^=1;meta['header_hex']=raw.hex()
            with self.assertRaisesRegex(ValueError,'native pointer'):collector._close_metadata(reader.r,sample,collector._binding(session))


if __name__=='__main__':unittest.main()
