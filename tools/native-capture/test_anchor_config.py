"""Synthetic reflected memory/config only; no live process or GPU opened."""
import copy
import pathlib
import struct
import sys
from unittest import mock
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/package-inspector'))
from test_sample_player_anchor import MemoryResolver
from anchor_config import build_anchor_config,anchor_ini,LiveRegistryAnchorSampler,AnchorRejected,AnchorSamplingRejected,acquire_anchor_sample


class Fixture(MemoryResolver):
    def __init__(self,view_target=True):
        super().__init__(view_target)
        self.profile.update(name_entry_index=0,name_entry_text=20)
        for index,name in self.names.items():
            entry=0x300000+index*0x100
            self.put(self.names_data+index*8,struct.pack('<Q',entry))
            raw=bytearray(148);struct.pack_into('<I',raw,0,index)
            text=(name+'\0').encode('utf-16-le');raw[20:20+len(text)]=text
            self.put(entry,raw)
    def u32(self,address):return struct.unpack('<I',self.read(address,4))[0]


class AnchorConfigTests(unittest.TestCase):
    def test_known_name_layout_combines_entry_and_retains_fresh_index_and_slot(self):
        r=Fixture();sampler=LiveRegistryAnchorSampler(r);address=0x20000
        index=r.u32(address+r.profile['object_name']);entry=r.u64(r.names_data+index*8)
        with mock.patch.object(r,'read',wraps=r.read) as read:
            sampler.fresh(address)
        for call in (mock.call(entry,148),mock.call(address+r.profile['object_name'],4),mock.call(r.objects_data+index*8,8)):
            self.assertIn(call,read.call_args_list)
        self.assertNotIn(mock.call(entry,4),read.call_args_list)
        self.assertNotIn(mock.call(entry+20,128),read.call_args_list)
        for mutation in ('index','text','slot'):
            with self.subTest(mutation=mutation):
                r=Fixture();sampler=LiveRegistryAnchorSampler(r)
                if mutation=='index':r.put(entry,struct.pack('<I',index+1))
                elif mutation=='text':r.put(entry+20,'Changed\0'.encode('utf-16-le'))
                else:r.put(r.objects_data+index*8,struct.pack('<Q',0))
                with self.assertRaises(AnchorRejected):sampler.fresh(address)

    def test_name_combined_short_failed_and_unknown_layout_keep_separate_path(self):
        for mode in ('short','failed','layout'):
            with self.subTest(mode=mode):
                r=Fixture();sampler=LiveRegistryAnchorSampler(r);address=0x20000
                index=r.u32(address+72);entry=r.u64(r.names_data+index*8);original=r.read
                if mode=='layout':
                    r.profile.update(name_entry_index=4,name_entry_text=24)
                    r.put(entry+4,struct.pack('<I',index))
                    r.put(entry+24,original(entry+20,128))
                def reading(a,size):
                    if (a,size)==(entry,148):
                        if mode=='short':return b'\0'*147
                        if mode=='failed':raise RuntimeError('combined unavailable')
                        self.fail('unknown layout must use separate reads')
                    return original(a,size)
                with mock.patch.object(r,'read',side_effect=reading) as read:
                    self.assertEqual(sampler.fresh(address)['name'],r.names[index])
                self.assertIn(mock.call(entry+r.profile['name_entry_index'],4),read.call_args_list)
                self.assertIn(mock.call(entry+r.profile['name_entry_text'],128),read.call_args_list)

    def test_strict_header_mismatch_preserves_typed_evidence(self):
        from anchor_config import AnchorRegistryChanged
        for registry,operation in (('objects','guard'),('names','guard'),('objects','sample')):
            with self.subTest(registry=registry,operation=operation):
                r=Fixture();sampler=LiveRegistryAnchorSampler(r)
                sampler.guard_stage='assembly_after'
                address=getattr(r,registry+'_header');original=r.read;hits=0
                def changing(address_read,size):
                    nonlocal hits
                    if address_read==address and size==16:
                        hits+=1
                        if hits==2:
                            data,count,capacity=struct.unpack('<Qii',original(address,16))
                            r.put(address,struct.pack('<Qii',data,count+1,capacity+1))
                    return original(address_read,size)
                r.read=changing
                with self.assertRaises(AnchorRegistryChanged) as failure:
                    if operation=='sample':acquire_anchor_sample(sampler)
                    else:sampler.guard()
                evidence=failure.exception.evidence
                self.assertEqual(evidence['stage'],'sample_before' if operation=='sample' else 'assembly_after')
                change=evidence['registry_change']
                self.assertEqual(change['registry'],registry)
                self.assertEqual(change['observed_header'][1],change['expected_header'][1]+1)
                self.assertEqual(evidence['guard_acquisitions'][-1]['strict_guard_attempts'],1)

    def test_prepared_camera_change_at_final_boundary_still_rejected(self):
        r=Fixture();prepared=r.resolve();original=LiveRegistryAnchorSampler._strict_guard
        def changing(sampler):
            if sampler.guard_acquisitions[-1]['stage']=='final_boundary':
                r.put(0x30224,struct.pack('<Q',0x40000))
            return original(sampler)
        with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')) as resolve,\
             mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',changing):
            with self.assertRaises(AnchorSamplingRejected) as failure:
                build_anchor_config(r,prepared,prepared_camera=prepared)
            self.assertEqual(failure.exception.evidence['stage'],'final_boundary');resolve.assert_not_called()

    def test_prepared_discovery_keeps_expected_camera_and_final_guards(self):
        r=Fixture();prepared=r.resolve()
        with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')) as resolve:
            config,metadata=build_anchor_config(r,prepared,prepared_camera=prepared)
            self.assertEqual(config['fields']['camera_view_target'],0x500)
            self.assertTrue(metadata['sample']['accepted']);resolve.assert_not_called()
        r=Fixture();prepared=r.resolve();expected=dict(prepared,camera_address='0xdead')
        with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')):
            with self.assertRaisesRegex(AnchorSamplingRejected,'camera changed'):
                build_anchor_config(r,expected,prepared_camera=prepared)
        for address in (0x10224,0x30224,0x30500):
            with self.subTest(address=address):
                r=Fixture();prepared=r.resolve();r.put(address,struct.pack('<Q',0x40000))
                with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')):
                    with self.assertRaises(AnchorSamplingRejected):build_anchor_config(r,prepared_camera=prepared)

    def test_nonpawn_failure_retains_bounded_target_identity_without_accepting(self):
        r=Fixture();r.put(0x30500,struct.pack('<Q',0x10000))
        with self.assertRaisesRegex(AnchorSamplingRejected,'not controlled Pawn') as failure:
            build_anchor_config(r)
        attempts=failure.exception.evidence['guard_acquisitions'][0]['attempts']
        self.assertEqual(len(attempts),1);self.assertFalse(attempts[0]['retryable'])
        observation=attempts[0]['camera_observation']
        self.assertTrue(observation['diagnostic_only']);self.assertTrue(observation['consistent_read'])
        self.assertEqual(observation['target'],0x10000)
        self.assertEqual(observation['target_identity']['address'],0x10000)
        self.assertEqual(observation['target_class']['name'],'WarPC')

    def test_unreadable_nonpawn_diagnostic_preserves_original_rejection(self):
        r=Fixture();r.put(0x30500,struct.pack('<Q',0xdeadbeef))
        with self.assertRaisesRegex(AnchorSamplingRejected,'not controlled Pawn') as failure:
            build_anchor_config(r)
        observation=failure.exception.evidence['guard_acquisitions'][0]['attempts'][0]['camera_observation']
        self.assertFalse(observation['consistent_read']);self.assertIn('read_error',observation)

    def test_first_strict_cross_frame_failure_then_independent_stable_sample(self):
        r=Fixture();sampler=LiveRegistryAnchorSampler(r)
        r.timestamp_reads=0;r.timestamp_changes=True
        sample,evidence=acquire_anchor_sample(sampler)
        self.assertTrue(sample['accepted']);self.assertTrue(evidence['accepted'])
        self.assertEqual(len(evidence['attempts']),1)
        guard=evidence['guard_acquisitions'][1]
        self.assertEqual(len(guard['attempts']),2)
        self.assertEqual(guard['attempts'][0]['reason'],'Cross-frame camera timestamp during identity checks')
        self.assertFalse(guard['attempts'][0]['strict_guard_accepted'])
        self.assertTrue(guard['attempts'][1]['strict_guard_accepted'])
        self.assertEqual(evidence['attempts'][0]['strict_guard_attempts'],3)
        self.assertEqual(sample['guard_before'],sample['guard_after'])

    def test_exhausted_timestamp_guard_then_fresh_strict_sample(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        original=sampler._strict_guard
        attempts=0
        def crossing():
            nonlocal attempts
            attempts+=1
            if attempts<=5:
                raise AnchorRejected('Cross-frame camera timestamp during identity checks')
            return original()
        with mock.patch.object(sampler,'_strict_guard',side_effect=crossing):
            sample,evidence=acquire_anchor_sample(sampler)
        self.assertTrue(sample['accepted'])
        self.assertEqual(len(evidence['attempts']),2)
        self.assertEqual(evidence['attempts'][0]['strict_guard_attempts'],5)
        self.assertEqual(evidence['attempts'][0]['raw_rejection']['rejection'],
                         'Cross-frame camera timestamp during identity checks')
        self.assertTrue(evidence['attempts'][1]['strict_sample_accepted'])
        self.assertEqual(evidence['stage'],'sample')
        self.assertEqual(sample['guard_before'],sample['guard_after'])

    def test_identity_and_unknown_rejection_are_not_retried(self):
        for reason in ('Pawn.Controller reciprocal link mismatch','Camera view target is not controlled Pawn','Unknown read error'):
            sampler=mock.Mock();sampler.sample.return_value={'accepted':False,'rejection':reason}
            with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler)
            self.assertEqual(sampler.sample.call_count,1)
            self.assertEqual(failure.exception.evidence['attempts'][0]['reason'],reason)

    def test_all_cross_frame_attempts_and_deadline_are_bounded(self):
        sampler=mock.Mock();sampler.sample.return_value={'accepted':False,'rejection':'Cross-frame camera timestamp during identity checks'}
        clock=iter(range(0,100000000,1000000))
        with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertEqual(sampler.sample.call_count,32);self.assertEqual(len(failure.exception.evidence['attempts']),32)
        self.assertEqual(failure.exception.evidence['maximum_attempts'],32)
        sampler.reset_mock();clock=iter([0,1000,201000000,202000000])
        with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertEqual(sampler.sample.call_count,1);self.assertIn('deadline',str(failure.exception))
        sampler.sample.return_value={'accepted':True};clock=iter([0,1000,201000000,202000000])
        with self.assertRaises(AnchorSamplingRejected):acquire_anchor_sample(sampler,lambda:next(clock))

    def test_temporal_rejections_can_recover_after_five_attempts(self):
        before={key:1 for key in ('controller','local_player','camera','pawn','level','outer_chain','view_target')}
        before['timestamp_raw']='0000803f'
        after=dict(before,timestamp_raw='00000040')
        failed={'accepted':False,'rejection':'Cross-frame or identity/pointer change during measurement',
                'guard_before':before,'guard_after':after}
        stable={'accepted':True,'guard_before':after,'guard_after':after}
        sampler=mock.Mock();sampler.sample.side_effect=[failed]*8+[stable]
        clock=iter(range(0,100000000,2000000))
        sample,evidence=acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertIs(sample,stable);self.assertEqual(sampler.sample.call_count,9)
        self.assertTrue(evidence['accepted'])
        self.assertTrue(all(not row['strict_sample_accepted'] for row in evidence['attempts'][:-1]))

    def test_retry_deadline_stops_before_attempt_cap(self):
        sampler=mock.Mock();sampler.sample.return_value={'accepted':False,'rejection':'Cross-frame camera timestamp during identity checks'}
        clock=iter(range(0,300000000,10000000))
        with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertEqual(sampler.sample.call_count,10)
        self.assertFalse(failure.exception.evidence['accepted'])

    def test_only_explicit_temporal_composite_mismatch_retries(self):
        before={key:1 for key in ('controller','local_player','camera','pawn','level','outer_chain','view_target')}
        before.update(timestamp_raw='0000803f',objects_header=struct.pack('<Qii',1000,1001,2000).hex(),names_header=struct.pack('<Qii',2000,1001,2000).hex())
        after=copy.deepcopy(before);after['objects_header']=struct.pack('<Qii',3000,1002,2000).hex()
        failed={'accepted':False,'rejection':'Cross-frame or identity/pointer change during measurement','guard_before':before,'guard_after':after}
        sampler=mock.Mock();sampler.sample.side_effect=[failed,{'accepted':True}]
        _,evidence=acquire_anchor_sample(sampler);self.assertEqual(len(evidence['attempts']),2)
        self.assertEqual(evidence['attempts'][0]['raw_rejection']['guard_before'],before)
        self.assertEqual(evidence['attempts'][0]['raw_rejection']['guard_after'],after)
        after['pawn']=2;sampler=mock.Mock();sampler.sample.return_value=failed
        with self.assertRaises(AnchorSamplingRejected):acquire_anchor_sample(sampler)
        self.assertEqual(sampler.sample.call_count,1)

    def test_certificate_relative_offsets_and_hypothesis_only(self):
        config,metadata=build_anchor_config(Fixture())
        text=anchor_ini(config)
        self.assertIn('schema=1',text);self.assertIn('role_local=',text)
        self.assertEqual(config['fields']['camera_timestamp'],0x600)
        self.assertEqual(config['fields']['camera_pov'],0x604)
        self.assertEqual(config['fields']['camera_view_target'],0x500)
        self.assertEqual(metadata['sample']['candidate_eye_hypothesis'],[10,20,90])
        self.assertFalse(metadata['renderer_alignment_verified'])
        self.assertNotIn('object_count=',text);self.assertNotIn('objects_data=',text)
        self.assertTrue(any(x['name']=='WarPawn' and x['check_super'] for x in config['identities']))
        self.assertTrue(any(x['name']=='Level' and x['check_super'] for x in config['identities']))

    def test_registry_growth_and_reallocation_with_same_slots_accepted(self):
        r=Fixture();sampler=LiveRegistryAnchorSampler(r)
        old=r.object_count
        table=r.read(r.objects_data,old*8)
        new_data=0x401000;r.put(new_data,table+struct.pack('<Q',0))
        r.put(r.objects_header,struct.pack('<Qii',new_data,old+1,old+1))
        self.assertTrue(sampler.sample()['accepted'])

    def test_missing_viewtarget_and_type_mismatch_reject(self):
        with self.assertRaisesRegex(AnchorRejected,'requires reflected'):build_anchor_config(Fixture(False))
        r=Fixture();r.properties[0x3000,'EyeHeight']['type']='IntProperty'
        with self.assertRaisesRegex(AnchorRejected,'property type'):build_anchor_config(r)

    def test_registry_slot_name_class_outer_and_pointer_fail_closed(self):
        mutations=[lambda r:r.put(r.objects_data+14*8,struct.pack('<Q',0)),
                   lambda r:r.put(0x20000+80,struct.pack('<Q',0xa000)),
                   lambda r:r.put(0x20000+64,struct.pack('<Q',0x40000)),
                   lambda r:r.put(0x20250,struct.pack('<Q',0x40000)),
                   lambda r:r.put(0x30500,struct.pack('<Q',0x40000)),
                   lambda r:r.put(0x300000+14*0x100,struct.pack('<I',999))]
        for mutation in mutations:
            r=Fixture();mutation(r)
            with self.subTest(mutation=mutation),self.assertRaises((AnchorRejected,RuntimeError)):build_anchor_config(r)

    def test_name_change_after_resolution_rejects_sample(self):
        r=Fixture();sampler=LiveRegistryAnchorSampler(r)
        r.put(0x20000+72,struct.pack('<I',0))
        self.assertFalse(sampler.sample()['accepted'])

    def test_certificate_allows_frame_advance_but_rejects_possession_change(self):
        class AdvancingFixture(Fixture):
            def __init__(self,change_pawn=False):
                super().__init__();self.advance_done=False;self.change_pawn=change_pawn
            def read(self,address,size):
                if address==0x7000 and size==0x60 and self.timestamp_reads>=6 and not self.advance_done:
                    self.advance_done=True;self.put(0x30600,struct.pack('<f',11))
                    self.put(self.objects_header,struct.pack('<Qii',self.objects_data,self.object_count+1,self.object_capacity+1))
                    if self.change_pawn:self.put(0x10224,struct.pack('<Q',0x40000))
                return super().read(address,size)
        r=AdvancingFixture();config,metadata=build_anchor_config(r)
        self.assertTrue(r.advance_done)
        self.assertEqual(struct.unpack('<f',bytes.fromhex(metadata['certificate_final_guard']['timestamp_raw']))[0],11)
        with self.assertRaises(AnchorRejected):build_anchor_config(AdvancingFixture(True))

    def test_timestamp_and_numeric_bracket_fail_closed(self):
        r=Fixture();r.put(0x20260,struct.pack('<f',float('nan')))
        with self.assertRaises(AnchorSamplingRejected):build_anchor_config(r)
        with mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',side_effect=AnchorRejected('Cross-frame camera timestamp during identity checks')):
            with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(Fixture())
        self.assertEqual(len(failure.exception.evidence['guard_acquisitions'][0]['attempts']),5)

    def test_all_guard_call_sites_retry_independently_and_record_success(self):
        original=LiveRegistryAnchorSampler._strict_guard
        seen={}
        def crossing(sampler):
            stage=sampler.guard_acquisitions[-1]['stage']
            seen[stage]=seen.get(stage,0)+1
            if seen[stage]==1:raise AnchorRejected('Cross-frame camera timestamp during identity checks')
            return original(sampler)
        with mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',crossing):
            _,metadata=build_anchor_config(Fixture())
        self.assertEqual(set(seen),{'initial','sample_before','sample_after','assembly_after','final_boundary'})
        self.assertTrue(all(x['accepted'] and len(x['attempts'])==2 for x in metadata['guard_acquisitions']))
        self.assertEqual(metadata['strict_guard_attempts'],10)
        self.assertEqual(metadata['sample_acquisition']['attempts'][0]['strict_guard_attempts'],4)

    def test_initial_real_strict_guard_recovers_from_cross_frame(self):
        r=Fixture();r.timestamp_changes=True
        sampler=LiveRegistryAnchorSampler(r)
        self.assertTrue(sampler.guard_acquisitions[0]['accepted'])
        self.assertEqual(len(sampler.guard_acquisitions[0]['attempts']),2)

    def test_guard_refreshes_both_headers_and_runs_full_base_each_attempt(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        from anchor_config import AnchorSampler
        original=AnchorSampler.guard
        attempts=[]
        def base_guard(value):
            attempts.append(value.r.object_count)
            if len(attempts)==1:raise AnchorRejected('Cross-frame camera timestamp during identity checks')
            return original(value)
        import anchor_config
        with mock.patch.object(anchor_config,'_header',wraps=anchor_config._header) as header,mock.patch.object(AnchorSampler,'guard',base_guard):
            sampler.guard()
        self.assertEqual(len(attempts),2)
        self.assertEqual(header.call_count,4)

    def test_guard_non_temporal_errors_never_retry(self):
        for error in (AnchorRejected('Invalid live registry header'),AnchorRejected('Object registry identity mismatch'),
                      ValueError('Nonfinite anchor values'),RuntimeError('Unknown read error'),
                      RuntimeError('Cross-frame camera timestamp during identity checks')):
            with self.subTest(error=error),mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',side_effect=error) as strict:
                with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(Fixture())
                self.assertEqual(strict.call_count,1)
                self.assertEqual(failure.exception.evidence['reason'],str(error))
                self.assertEqual(failure.exception.evidence['exception_type'],type(error).__name__)

    def test_timestamp_guard_exhaustion_stays_bounded_across_fresh_samples(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        with mock.patch.object(sampler,'_strict_guard',side_effect=AnchorRejected('Cross-frame camera timestamp during identity checks')) as strict:
            clock=iter(range(0,1000000000,1000000))
            with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertEqual(strict.call_count,5*32)
        self.assertEqual(len(failure.exception.evidence['attempts']),32)
        self.assertEqual(failure.exception.evidence['strict_guard_attempts'],1+5*32)
        self.assertEqual(failure.exception.evidence['stage'],'sample_before')

    def test_sample_after_guard_exhaustion_keeps_actual_partial_sample(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        original=sampler._strict_guard
        def after_crossing():
            if sampler.guard_acquisitions[-1]['stage']=='sample_after':
                raise AnchorRejected('Cross-frame camera timestamp during identity checks')
            return original()
        with mock.patch.object(sampler,'_strict_guard',side_effect=after_crossing),mock.patch.object(sampler,'sample',wraps=sampler.sample) as sample:
            with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler)
        evidence=failure.exception.evidence
        self.assertGreater(sample.call_count,1)
        self.assertLessEqual(sample.call_count,32)
        self.assertEqual(len(evidence['attempts']),sample.call_count)
        self.assertEqual(evidence['stage'],'sample_after')
        self.assertEqual(len(evidence['guard_acquisitions'][-1]['attempts']),5)
        raw=evidence['raw_rejection']
        self.assertEqual(raw['rejection'],'Cross-frame camera timestamp during identity checks')
        self.assertEqual(raw['guard_before'],sampler.initial)
        self.assertEqual(raw['raw_values']['pawn_location'],sampler.r.read(sampler.pawn+sampler.layout['pawn_location']['offset'],12).hex())
        self.assertIsNone(raw['guard_after'])
        self.assertEqual(evidence['attempts'][-1]['raw_rejection'],raw)

    def test_guard_deadline_and_late_success_reject(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        for value,error in (({'strict':'good'},None),(None,AnchorRejected('Cross-frame camera timestamp during identity checks'))):
            clock=iter([0,1000,201000000,202000000,203000000])
            sampler.guard_clock_ns=lambda:next(clock)
            with mock.patch.object(sampler,'_strict_guard',return_value=value,side_effect=error) as strict:
                with self.assertRaises(AnchorSamplingRejected) as failure:sampler.guard()
            self.assertEqual(strict.call_count,1)
            self.assertIn('deadline',str(failure.exception))
            self.assertFalse(sampler.guard_acquisitions[-1]['accepted'])

    def test_initial_and_assembly_errors_have_failure_evidence(self):
        r=Fixture();r.properties[0x3000,'EyeHeight']['type']='IntProperty'
        with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(r)
        self.assertEqual(failure.exception.evidence['stage'],'initial')
        self.assertEqual(failure.exception.evidence['guard_acquisitions'],[])
        r=Fixture();expected=r.resolve();expected['camera_address']='bad'
        with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(r,expected)
        self.assertEqual(failure.exception.evidence['stage'],'startup_validation')
        self.assertIn('camera changed',failure.exception.evidence['reason'])

    def test_fatal_at_each_late_guard_has_stage(self):
        original=LiveRegistryAnchorSampler._strict_guard
        for target in ('sample_after','assembly_after','final_boundary'):
            def failing(sampler):
                if sampler.guard_acquisitions[-1]['stage']==target:raise AnchorRejected('Unknown guard failure')
                return original(sampler)
            with self.subTest(target=target),mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',failing):
                with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(Fixture())
            self.assertEqual(failure.exception.evidence['stage'],target)
            self.assertEqual(len(failure.exception.evidence['guard_acquisitions'][-1]['attempts']),1)

    def test_initial_guard_failure_receipt_preserves_all_attempts(self):
        import contextlib,io,json,tempfile
        import start_capture
        with mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',side_effect=AnchorRejected('Cross-frame camera timestamp during identity checks')):
            with self.assertRaises(AnchorSamplingRejected) as failure:build_anchor_config(Fixture())
        with tempfile.TemporaryDirectory() as directory,contextlib.redirect_stderr(io.StringIO()):
            path=start_capture.save_anchor_sampling_failure(pathlib.Path(directory)/'Capture.dll',123,failure.exception)
            report=json.loads(path.read_text())
        self.assertEqual(report['failure_stage'],'initial')
        self.assertEqual(report['strict_guard_attempts'],5)
        self.assertEqual(len(report['guard_acquisitions'][0]['attempts']),5)
        self.assertEqual(report['sample_acquisition']['reason'],'Cross-frame camera timestamp during identity checks')
        self.assertFalse(report['native_activation_attempted'])

    def test_sample_overall_deadline_unchanged_with_nested_guards(self):
        sampler=LiveRegistryAnchorSampler(Fixture())
        clock=iter([0,1000,201000000,202000000])
        with self.assertRaises(AnchorSamplingRejected) as failure:acquire_anchor_sample(sampler,lambda:next(clock))
        self.assertEqual(len(failure.exception.evidence['attempts']),1)
        self.assertTrue(failure.exception.evidence['attempts'][0]['strict_sample_accepted'])
        self.assertEqual(failure.exception.evidence['attempts'][0]['strict_guard_attempts'],2)
        self.assertIn('deadline',str(failure.exception))

    def test_expected_camera_changes_and_schema_mutations_rejected(self):
        r=Fixture();expected=r.resolve();expected['controller_address']='0x999'
        with self.assertRaisesRegex(AnchorRejected,'camera changed'):build_anchor_config(r,expected)
        config,_=build_anchor_config(Fixture())
        mutations=[lambda c:c['fields'].pop('eye_height'),lambda c:c['layout'].pop('object_name'),
                   lambda c:c['roles'].update(pawn=999),lambda c:c['identities'][0].update(name='evil\nkey=1'),
                   lambda c:c['identities'][0].update(cls=0xdead),lambda c:c['fields'].update(eye_height=0x10000)]
        for mutate in mutations:
            bad=copy.deepcopy(config);mutate(bad)
            with self.assertRaises(AnchorRejected):anchor_ini(bad)


if __name__=='__main__':unittest.main()
