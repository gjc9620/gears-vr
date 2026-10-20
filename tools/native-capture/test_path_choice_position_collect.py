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
from test_path_choice_group_fit import viewport_fixture


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
        # Actual MenuReader retained registered()/Resolver.identity() shape:
        # name_index exists only in the independent scene-owner role.
        client_identity={key:copy.deepcopy(value) for key,value in sample['scene_owner']['roles']['client'].items()
                         if key != 'name_index'}, properties={})
    reader.force_registry_refresh = mock.Mock()
    def acquire(**options):
        # Model the fresh MenuReader acquisition, including its observed sources.
        for header in (r.objects_header,r.names_header): r.read(header,16)
        for address in sorted(memory):
            if not any(header <= address < header+16 for header in (r.objects_header,r.names_header)):
                r.read(address,1)
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
    def invoke(self, values, verify=None, policy='exact', target=None, close_hook=None):
        reader,sample,proof,metadata,session,status,memory=values
        def panel_proof(r,fresh):
            result=copy.deepcopy(proof)
            result['owner_snapshot_sha256']=collector.position.owner_digest(fresh['scene_owner'])
            return result
        def viewport(r,fresh):
            domain=copy.deepcopy(fresh['viewport_domain'])
            for span in domain['source_checks']:
                expected=bytes.fromhex(span['raw'])
                collector.require(r.read(span['address'],len(expected))==expected,'synthetic viewport source drift')
            return domain
        def close_metadata(*args):
            if close_hook:close_hook()
            return copy.deepcopy(metadata)
        with mock.patch.object(collector,'_panel_proof',side_effect=panel_proof), \
             mock.patch.object(collector,'_viewport_domain',side_effect=viewport), \
             mock.patch.object(collector,'_close_metadata',side_effect=close_metadata):
            return collector.collect(reader,sample,status,session,1000,verify or (lambda r:copy.deepcopy(session)),
                                     registry_header_policy=policy, target_vertical_region=target)

    def shared_fit_values(self, dimensions=(1920.,1080.)):
        values=fixture();sample=values[1];memory=values[6]
        root=sample['top']['address']
        panel=next(row for row in sample['tree'] if row['cls']=='UIWarSafeRegionPanel')
        content=[row for row in sample['tree'] if row['address'] not in (root,panel['address'])]
        self.assertEqual(len(content),11)
        def bounds(row,coordinates):
            raw=struct.pack('<4f',*coordinates);numbers=list(struct.unpack('<4f',raw))
            row['render_bounds']=dict(order=['left','top','right','bottom'],values=numbers,
                ordered_values=numbers,raw=raw.hex(),reflected=dict(type='FloatProperty',
                offset=676,array_dim=4,element_size=4))
            memory.update((int(row['address'],0)+676+i,b) for i,b in enumerate(raw))
        panel.update(parent=root,depth=2)
        fractions=panel['layout_details']['position']['members']['Value']['values']
        bounds(panel,[v*dimensions[i%2] for i,v in enumerate(fractions)])
        for i,row in enumerate(content[:10]):
            row.update(parent=panel['address'],outer=panel['address'],depth=3)
            bounds(row,[(550+i)*dimensions[0]/1920,(70+i)*dimensions[1]/1080,
                (1370-i)*dimensions[0]/1920,(260-i)*dimensions[1]/1080])
        content[-1].update(parent=root,outer=root,depth=2)
        sample['viewport_domain']=viewport_fixture(sample,dimensions)
        for span in sample['viewport_domain']['source_checks']:
            memory.update((span['address']+i,b) for i,b in enumerate(bytes.fromhex(span['raw'])))
        return values

    def test_shared_fit_acquisition_builds_matching_close_certificate(self):
        values=self.shared_fit_values()
        panel=next(row for row in values[1]['tree'] if row['cls']=='UIWarSafeRegionPanel')
        certificate=self.invoke(values,policy='fixed_prefix_v1',target=(.25,.75))
        self.assertIn('group_fit=vertical_v1',certificate.position_ini)
        # Close authorizes the same lease; its restoration uses the baseline
        # already owned by native, not a second independent translation.
        for field in ('nonce=1000', 'panel='+panel['address'], 'world_generation=2'):
            self.assertIn(field,certificate.close_ini)
        self.assertNotIn('vertical_offset_reference_pixels=374',certificate.position_ini)
        self.assertEqual(len(json.loads(certificate.baseline_ids_json)['nodes']),13)

    def test_shared_fit_uses_live_domain_including_4k_and_records_immutable_checks(self):
        for dimensions in ((1920.,1080.),(3840.,2160.),(1280.,1400.)):
            values=self.shared_fit_values(dimensions)
            certificate=self.invoke(values,target=(.25,.75))
            domain=json.loads(certificate.evidence_json)['sample']['viewport_domain']
            self.assertEqual(domain['values_xy'],list(dimensions))
            self.assertIn('0x%X:%s'%(domain['value_address'],domain['raw']),certificate.position_ini)
            self.assertIn('group_fit=vertical_v1',certificate.position_ini)

    def test_shared_fit_final_viewport_value_or_metadata_drift_yields_no_certificate(self):
        for field in ('value','metadata'):
            values=self.shared_fit_values((3840.,2160.));domain=values[1]['viewport_domain']
            address=domain['value_address'] if field=='value' else domain['property']['identity']['address']+0x70
            def mutate():values[6][address]^=1
            with self.subTest(field=field),self.assertRaises(collector.AcquisitionSourceChanged):
                self.invoke(values,target=(.25,.75),close_hook=mutate)

    def test_shared_fit_accepts_current_child_coordinates_legacy_does_not(self):
        values=self.shared_fit_values()
        child=next(row for row in values[1]['tree'] if row['name']=='LblLeftButton')
        member=child['layout_details']['position']['members']['Value']
        changed=list(member['values']);changed[0]+=.01
        member['raw']=struct.pack('<4f',*changed).hex()
        member['values']=list(struct.unpack('<4f',bytes.fromhex(member['raw'])))
        receipt=self.invoke(values,target=(.25,.75))
        evidence=json.loads(receipt.evidence_json)
        actual=next(row for row in evidence['sample']['tree'] if row['name']=='LblLeftButton')
        self.assertEqual(actual['layout_details']['position']['members']['Value'],member)
        self.assertIn('group_fit=vertical_v1',receipt.position_ini)
        with self.assertRaisesRegex(ValueError,'unsupported PathChoice baseline geometry'):
            self.invoke(values)

    def test_shared_fit_still_requires_initial_fresh_full_layout_equality(self):
        for field in ('position','docking','bounds','identity'):
            values=self.shared_fit_values();reader=values[0];original=reader._sample_once.side_effect
            def changed(**options):
                fresh=original(**options)
                child=next(row for row in fresh['tree'] if row['name']=='LblLeftButton')
                if field=='position':
                    member=child['layout_details']['position']['members']['Value']
                    numbers=list(member['values']);numbers[0]+=.01
                    member['raw']=struct.pack('<4f',*numbers).hex()
                    member['values']=list(struct.unpack('<4f',bytes.fromhex(member['raw'])))
                elif field=='docking':child['layout_details']['docking']={'changed':'metadata or target'}
                elif field=='bounds':child['render_bounds']['reflected']['offset']+=4
                else:child['outer']=fresh['top']['address']
                return fresh
            reader._sample_once.side_effect=changed
            with self.subTest(field=field),self.assertRaisesRegex(ValueError,'original baseline changed'):
                self.invoke(values,target=(.25,.75))

    def test_shared_fit_rejects_unknown_inventory_and_invalid_storage(self):
        for field in ('name','class','nan','raw','units'):
            values=self.shared_fit_values();child=next(row for row in values[1]['tree'] if row['name']=='LblLeftButton')
            members=child['layout_details']['position']['members']
            if field=='name':child['name']='UnknownContent'
            elif field=='class':child['cls']='UnknownWidget'
            elif field=='nan':
                members['Value']['values'][0]=float('nan')
                members['Value']['raw']=struct.pack('<4f',*members['Value']['values']).hex()
            elif field=='raw':members['Value']['raw']='00'*16
            else:members['ScaleType']['values'][0]^=1
            with self.subTest(field=field),self.assertRaises(ValueError):self.invoke(values,target=(.25,.75))
            values[0].force_registry_refresh.assert_not_called()

    def live_header_fixture(self):
        values=fixture();r=values[0].r
        # Native99e bytes: GObjects Num 0x3b07c -> 0x3b088, stable Data/Max.
        r.object_count=0x3b07c;r.object_capacity=0x4ba06
        header=values[1]['scene_owner']['evidence']['registry_headers']['objects']
        header.update(count=r.object_count,capacity=r.object_capacity)
        initial=struct.pack('<Qii',r.objects_data,r.object_count,r.object_capacity)
        values[6].update((r.objects_header+i,b) for i,b in enumerate(initial))
        return values,initial

    def test_presample_fixed_prefix_actual11_plus4_and_finally_restore(self):
        values,initial=self.live_header_fixture();reader=values[0];r=reader.r;source=r.read
        r.object_count=241927
        initial=struct.pack('<Qii',r.objects_data,r.object_count,r.object_capacity)
        values[6].update((r.objects_header+i,b) for i,b in enumerate(initial))
        calls=[]
        def grow(address,size):
            raw=source(address,size)
            if (address,size)==(r.objects_header,16):
                calls.append(1)
                return struct.pack('<Qii',r.objects_data,r.object_count+4,r.object_capacity)
            return raw
        r.read=grow
        def sample(**options):
            reader.force_registry_refresh.assert_called_once_with(registry_header_policy="fixed_prefix_v1")
            self.assertEqual(options,dict(include_owner=True))
            self.assertEqual(r.read(r.objects_header,16),initial)
            self.assertEqual(r.read(r.objects_header,16),initial)
            return copy.deepcopy(values[1])
        reader.sample=mock.Mock(side_effect=sample)
        result=collector.sample_fixed_prefix(reader,include_owner=True)
        self.assertEqual(result['registry_header_policy'],'fixed_prefix_v1')
        self.assertEqual(result['scene_owner']['evidence']['source_bytes_semantics'],
            'digest_of_logical_fixed_prefix_read_ledger')
        self.assertEqual(result['registry_header_observations']['objects']['accepted_suffix_reads'],2)
        self.assertEqual(len(calls),2)
        self.assertIs(r.read,grow)

    def test_presample_policy_is_built_after_fresh_registry_refresh(self):
        values=fixture();reader=values[0];r=reader.r;source=r.read
        def refresh(**options):
            self.assertEqual(options,dict(registry_header_policy="fixed_prefix_v1"))
            r.object_count+=1
            raw=struct.pack('<Qii',r.objects_data,r.object_count,r.object_capacity)
            values[6].update((r.objects_header+i,b) for i,b in enumerate(raw))
        reader.force_registry_refresh.side_effect=refresh
        reader.sample=lambda **options:dict(header=r.read(r.objects_header,16))
        result=collector.sample_fixed_prefix(reader)
        self.assertEqual(struct.unpack('<Qii',result['header'])[1],r.object_count)
        self.assertIs(r.read,source)
        reader.force_registry_refresh.side_effect=RuntimeError('fresh inventory rejected')
        reader.sample=mock.Mock()
        with self.assertRaisesRegex(RuntimeError,'fresh inventory rejected'):collector.sample_fixed_prefix(reader)
        reader.sample.assert_not_called()
        self.assertIs(r.read,source)

    def test_presample_fixed_prefix_rejects_invalid_header_and_restores(self):
        for change in ('shrink','data','capacity','excess','partial'):
            values,initial=self.live_header_fixture();reader=values[0];r=reader.r;source=r.read
            data,num,capacity=struct.unpack('<Qii',initial)
            if change=='shrink':num-=1
            if change=='data':data+=8
            if change=='capacity':capacity+=1
            if change=='excess':num=capacity+1
            def changed(address,size):
                return struct.pack('<Qii',data,num,capacity) if (address,size)==(r.objects_header,16) else source(address,size)
            r.read=changed
            reader.sample=lambda **options:r.read(r.objects_header,8 if change=='partial' else 16)
            with self.subTest(change=change),self.assertRaises(ValueError):collector.sample_fixed_prefix(reader)
            self.assertIs(r.read,changed)

    def test_presample_keeps_nonheader_checks_exact_and_restores_on_error(self):
        from menu_state import SnapshotChanged
        for target_kind in ('slot','layout'):
            values=fixture();reader=values[0];r=reader.r;source=r.read
            target=r.objects_data if target_kind=='slot' else 0x987600
            values[6][target]=42
            def sample(**options):
                first=r.read(target,1)
                values[6][target]^=1
                if r.read(target,1)!=first:raise SnapshotChanged('changed ordinary sampled bytes')
            reader.sample=sample
            with self.subTest(target_kind=target_kind),self.assertRaises(SnapshotChanged):
                collector.sample_fixed_prefix(reader)
            self.assertIs(r.read,source)

    def test_fixed_prefix_actual_plus12_uses_initial_serialized_baseline(self):
        values,initial=self.live_header_fixture();reader=values[0];r=reader.r;source=r.read
        calls=0
        def grow(address,size):
            nonlocal calls
            raw=source(address,size)
            if address==r.objects_header and size==16:
                calls+=1
                # A decrease back to INITIAL is legal; there is no highwater.
                num=r.object_count+(12 if calls%2 else 0)
                return struct.pack('<Qii',r.objects_data,num,r.object_capacity)
            return raw
        r.read=grow
        certificate=self.invoke(values,policy='fixed_prefix_v1')
        evidence=json.loads(certificate.evidence_json)
        self.assertTrue(evidence['normalized_fixed_prefix_headers'])
        self.assertEqual(evidence['consistency_semantics'],'logical_fixed_prefix_consistency')
        self.assertFalse(evidence['atomic_snapshot'])
        observation=evidence['registry_header_observations']['objects']
        self.assertEqual(observation['initial_raw'],initial.hex())
        self.assertGreater(observation['accepted_suffix_reads'],0)
        self.assertLessEqual(len(observation['first_live_headers']),8)
        self.assertIn('registry_header_policy=fixed_prefix_v1',certificate.position_ini)
        checks=evidence['panel_proof']['checks']
        self.assertEqual([c['raw'] for c in checks if c['address']==r.objects_header],[initial.hex()])
        self.assertIn(initial.hex(),certificate.position_ini)
        self.assertEqual(evidence['sample']['scene_owner']['evidence']['registry_headers']['objects']['count'],0x3b07c)
        self.assertIs(r.read,grow)

    def test_exact_default_still_rejects_actual_plus12(self):
        values,initial=self.live_header_fixture();r=values[0].r;source=r.read
        def grow(address,size):
            if (address,size)==(r.objects_header,16):
                return struct.pack('<Qii',r.objects_data,r.object_count+12,r.object_capacity)
            return source(address,size)
        r.read=grow
        with self.assertRaisesRegex(ValueError,'registry changed after refresh'):self.invoke(values)
        self.assertIs(r.read,grow)

    def test_fixed_prefix_rejects_shrink_data_capacity_and_excess_count(self):
        for change in ('shrink','data','capacity','excess'):
            values,initial=self.live_header_fixture();r=values[0].r;source=r.read
            data,num,capacity=struct.unpack('<Qii',initial)
            if change=='shrink':num-=1
            if change=='data':data+=8
            if change=='capacity':capacity+=1
            if change=='excess':num=capacity+1
            changed=struct.pack('<Qii',data,num,capacity)
            def read(address,size):
                return changed if (address,size)==(r.objects_header,16) else source(address,size)
            r.read=read
            with self.subTest(change=change),self.assertRaises(collector.AcquisitionSourceChanged):
                self.invoke(values,policy='fixed_prefix_v1')
            self.assertIs(r.read,read)

    def test_fixed_prefix_does_not_mask_old_slot_or_unexpected_ui_bytes(self):
        for kind in ('slot','ui'):
            values,initial=self.live_header_fixture();reader=values[0];r=reader.r
            target=r.objects_data if kind=='slot' else 0x987600
            values[6][target]=42
            original=reader._sample_once.side_effect
            def acquire(**options):
                fresh=original(**options)
                r.read(target,1)
                values[6][target]^=1
                r.read(target,1)
                return fresh
            reader._sample_once.side_effect=acquire
            with self.subTest(kind=kind),self.assertRaises(collector.AcquisitionSourceChanged):
                self.invoke(values,policy='fixed_prefix_v1')

    def test_single_fresh_admission_immutable_and_no_retained_baseline(self):
        values=fixture();reader=values[0];original=reader.r.read
        self.assertNotIn('name_index', reader.client_identity)
        self.assertIn('name_index', values[1]['scene_owner']['roles']['client'])
        original_client=copy.deepcopy(reader.client_identity)
        receipt=self.invoke(values)
        self.assertIn('vertical_offset_reference_pixels=374',receipt.position_ini)
        self.assertIn('duration_ms=6000',receipt.position_ini)
        self.assertIn('duration_ms=8000',receipt.close_ini)
        self.assertEqual(json.loads(receipt.baseline_ids_json)['nonce'],1000)
        self.assertTrue(json.loads(receipt.evidence_json)['consistent_final_reread'])
        reader.force_registry_refresh.assert_called_once();reader._sample_once.assert_called_once()
        self.assertEqual(reader.client_identity, original_client)
        self.assertEqual(json.loads(receipt.baseline_ids_json)['roles']['client'], values[1]['scene_owner']['roles']['client'])
        self.assertIs(reader.r.read,original)
        with self.assertRaises(FrozenInstanceError):receipt.close_ini='changed'
        # A second activation must independently reacquire rather than reuse.
        self.invoke(values)
        self.assertEqual(reader.force_registry_refresh.call_count,2)
        self.assertEqual(reader._sample_once.call_count,2)

    def test_live_shaped_client_boundary_rejects_each_changed_identity_field(self):
        for field in ('address', 'name', 'number', 'cls', 'outer', 'internal_index'):
            values=fixture();reader=values[0]
            value=reader.client_identity[field]
            reader.client_identity[field]=value+'changed' if isinstance(value,str) else value+1
            with self.subTest(field=field), self.assertRaisesRegex(ValueError,'original client mismatch'):
                self.invoke(values)
            reader.force_registry_refresh.assert_not_called()
            reader._sample_once.assert_not_called()

    def test_full_refresh_cannot_adopt_a_changed_selected_client(self):
        for field in ('address', 'name', 'number', 'cls', 'outer', 'internal_index'):
            values=fixture();reader=values[0]
            def replace_client(**options):
                value=reader.client_identity[field]
                reader.client_identity[field]=value+'changed' if isinstance(value,str) else value+1
            reader.force_registry_refresh.side_effect=replace_client
            with self.subTest(field=field), self.assertRaisesRegex(ValueError,'refresh replaced client'):
                self.invoke(values)
            reader.force_registry_refresh.assert_called_once()
            reader._sample_once.assert_not_called()

    def test_owner_only_name_index_still_requires_exact_fresh_role(self):
        values=fixture();reader=values[0];original=reader._sample_once.side_effect
        def changed(**options):
            fresh=original(**options)
            fresh['scene_owner']['roles']['client']['name_index']+=1
            return fresh
        reader._sample_once.side_effect=changed
        with self.assertRaisesRegex(ValueError,'original baseline changed'):
            self.invoke(values)
        reader.force_registry_refresh.assert_called_once()
        reader._sample_once.assert_called_once()

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
        # only after finally; the fourth repeated complete header is final reread.
        header=reader.r.objects_header;counts={}
        def changing(address,size):
            key=(address,size);counts[key]=counts.get(key,0)+1
            raw=base_read(address,size)
            if key==(header,16) and counts[key]>=4:return bytes([raw[0]^1])+raw[1:]
            return raw
        reader.r.read=changing
        with self.assertRaises(collector.AcquisitionSourceChanged) as caught:self.invoke(values)
        detail = caught.exception.read_conflict
        self.assertEqual(detail['phase'], 'final_reread')
        self.assertEqual(detail['changed_address'], hex(header))
        self.assertTrue(detail['first_read'])
        self.assertLessEqual(len(detail['expected_window']),16)
        self.assertIs(reader.r.read,changing)

    def test_repeated_read_conflict_records_first_field_and_exact_byte(self):
        values=fixture();reader=values[0];original_read=reader.r.read
        address=reader.r.objects_header+8
        def acquire(**options):
            name='SyntheticMutableField'
            reader.r.read(address,4)
            values[6][address+2]^=1
            reader.r.read(address,4)
        reader._sample_once.side_effect=acquire
        with self.assertRaises(collector.AcquisitionSourceChanged) as caught:self.invoke(values)
        detail=caught.exception.read_conflict
        self.assertEqual(detail['phase'],'repeated_read')
        self.assertEqual(detail['address'],hex(address))
        self.assertEqual(detail['size'],4)
        self.assertEqual(detail['changed_offset'],2)
        self.assertEqual(detail['changed_address'],hex(address+2))
        self.assertEqual(detail['first_read'][0]['field'],'SyntheticMutableField')
        self.assertLessEqual(len(detail['first_read']),6)
        self.assertIs(reader.r.read,original_read)
        reader._sample_once.assert_called_once()

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


class FixedPrefixNestedTests(unittest.TestCase):
    def resolver(self):
        from test_scene_owner_snapshot import Fixture
        r=Fixture()
        for prefix,singular,rva,items in (('objects','object',0x1c24730,r.objects),('names','name',0x1c34800,r.names)):
            setattr(r,prefix+'_header',r.base+rva)
            setattr(r,singular+'_count',len(items))
            setattr(r,singular+'_capacity',len(items)+32)
            r.put(r.base+rva,'<Qii',getattr(r,prefix+'_data'),len(items),len(items)+32)
        return r

    def test_capture_finish_uses_same_initial_prefix_and_exact_slots(self):
        for policy_enabled in (False,True):
            r=self.resolver();initial=r.object_count
            def grow(address,size):
                if address==r.objects_header and r.read_counts[address,size]>=2:
                    r.put(r.objects_header,'<Qii',r.objects_data,initial+12,r.object_capacity)
            r.hook=grow
            policy=collector._FixedPrefixReads(r,r.read)
            if policy_enabled:r.read=policy.read
            if policy_enabled:
                result=r.capture()
                self.assertEqual(result['evidence']['registry_headers']['objects']['count'],initial)
                self.assertGreater(policy.observations['objects']['accepted_suffix_reads'],0)
            else:
                from scene_owner_snapshot import SceneOwnerRejected
                with self.assertRaises(SceneOwnerRejected):r.capture()
        r=self.resolver();policy=collector._FixedPrefixReads(r,r.read);r.read=policy.read
        r.hook=r.mutate_on_reread(r.objects_data)
        from scene_owner_snapshot import SceneOwnerRejected
        with self.assertRaises(SceneOwnerRejected):r.capture()

    def test_referenced_new_object_or_name_is_outside_initial_prefix(self):
        from scene_owner_snapshot import _Capture,SceneOwnerRejected
        r=self.resolver();policy=collector._FixedPrefixReads(r,r.read);r.read=policy.read
        cap=_Capture(r)
        new=r.obj('SuffixOnlyObject',r.classes['Object'])
        r.put(r.objects_data+r.object_count*8,'<Q',new)
        r.put(r.objects_header,'<Qii',r.objects_data,r.object_count+1,r.object_capacity)
        with self.assertRaisesRegex(SceneOwnerRejected,'registry slot mismatch'):cap.identity(new)
        with self.assertRaisesRegex(SceneOwnerRejected,'outside registry'):cap.name(r.name_count)

    def test_partial_header_refused_and_merged_final_span_validates_full_header(self):
        r=self.resolver();header=r.objects_header;source=r.read
        initial=source(header,16)
        r.bytes(header-2,b'ab');r.bytes(header+16,b'cd')
        policy=collector._FixedPrefixReads(r,source)
        r.put(header,'<Qii',r.objects_data,r.object_count+12,r.object_capacity)
        for address,size in ((header+8,4),(header-1,16),(header,8)):
            with self.subTest(address=address,size=size),self.assertRaisesRegex(ValueError,'partial'):
                policy.read(address,size)
        self.assertEqual(policy.read(header-2,20),b'ab'+initial+b'cd')
        r.bytes(header+16,b'Xd')
        self.assertEqual(policy.read(header-2,20),b'ab'+initial+b'Xd')
        r.put(header,'<Qii',r.objects_data+8,r.object_count+12,r.object_capacity)
        with self.assertRaises(collector.AcquisitionSourceChanged):policy.read(header-2,20)

    def test_menu_reader_nested_final_ledger_accepts_only_opt_in_header_suffix(self):
        from menu_state import MenuReader,SnapshotChanged
        from test_menu_state import FakeResolver
        for enabled in (False,True):
            r=FakeResolver();r.base=0x140000000
            r.objects_header=r.base+0x1c24730;r.names_header=r.base+0x1c34800
            r.names_data=0x800000
            r.object_count=len(r.object_pointers);r.object_capacity=r.object_count+32
            r.name_count=10;r.name_capacity=42
            r.put(r.objects_header,struct.pack('<Qii',r.objects_data,r.object_count,r.object_capacity))
            r.put(r.names_header,struct.pack('<Qii',r.names_data,r.name_count,r.name_capacity))
            source=r.read;count=0
            def grow(address,size):
                nonlocal count
                raw=source(address,size)
                if (address,size)==(r.objects_header,16):
                    count+=1
                    if count>=2:return struct.pack('<Qii',r.objects_data,r.object_count+12,r.object_capacity)
                return raw
            policy=collector._FixedPrefixReads(r,grow)
            r.read=policy.read if enabled else grow
            reader=MenuReader.__new__(MenuReader)
            reader.r,reader.client,reader.properties=r,0x3000,{}
            reader.refresh=lambda:None
            if enabled:
                result=reader._sample_once(include_tree=True)
                self.assertTrue(result['consistent_reread'])
                self.assertGreater(policy.observations['objects']['accepted_suffix_reads'],0)
            else:
                with self.assertRaises(SnapshotChanged):reader._sample_once(include_tree=True)


if __name__=='__main__':unittest.main()
