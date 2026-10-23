"""Certified early unpossession with synthetic reflection memory only."""
import contextlib
import io
from pathlib import Path
import struct
import unittest
from unittest import mock

import start_capture as start
import anchor_config as anchor
import cinematic_anchor
import world_rebind_runtime as runtime
from world_rebind_monitor import RetryablePreparation
from test_mounted_integration import MountedUIFixture
from test_world_rebind_monitor import Harness

LINK=0x10224

def unpossessed():
    f=MountedUIFixture();f.ptr(LINK,0);return f

def reject(f):
    try:anchor.LiveRegistryAnchorSampler(f,cinematic_camera=True)
    except anchor.AnchorSamplingRejected as error:return error
    raise AssertionError('Unpossessed sampler accepted')


class UnpossessedRecoveryTests(unittest.TestCase):
    def test_stable_null_has_exact_type_and_live_source_property_evidence(self):
        f=unpossessed()
        with mock.patch.object(f,'read',wraps=f.read)as read:error=reject(f)
        self.assertIs(type(error),anchor.AnchorControlledPawnUnavailable)
        link=error.evidence['controlled_link']
        self.assertEqual((link['before'],link['after'],link['offset']),(0,0,548))
        self.assertEqual(link['controller']['address'],0x10000);self.assertEqual(link['field']['name'],'Pawn')
        self.assertEqual(error.evidence['stage'],'initial');self.assertFalse(error.evidence['accepted'])
        calls=[call.args for call in read.call_args_list]
        self.assertEqual(calls.count((LINK,8)),2)
        for address in (f.objects_header,f.names_header):self.assertEqual(calls.count((address,16)),2)
        self.assertTrue(start.observer_preparation_churn(error))
        with self.assertRaises(RetryablePreparation)as marked:runtime._read_only_acquisition(mock.Mock(side_effect=error))
        self.assertIs(marked.exception.error,error);self.assertEqual(marked.exception.scope,'acquisition_churn')

    def test_nonnull_wire_and_entire_read_schedule_keep_original_behavior(self):
        results=[]
        for reference in (True,False):
            f=MountedUIFixture()
            with contextlib.ExitStack()as stack:
                stack.enter_context(mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)))
                if reference:stack.enter_context(mock.patch.object(anchor.LiveRegistryAnchorSampler,'controlled_identity',lambda self,controller:self.fresh(self.pawn)))
                read=stack.enter_context(mock.patch.object(f,'read',wraps=f.read))
                config,metadata=anchor.build_anchor_config(f,sample_pose=False,cinematic_camera=True)
            results.append((config,anchor.anchor_ini(config),metadata['certificate_final_guard'],[call.args for call in read.call_args_list]))
        self.assertEqual(results[0],results[1])

    def test_zero_to_valid_turret_rejects_partial_acquisition_then_full_build_succeeds(self):
        f=MountedUIFixture();read=f.read;hits=0
        def transient(address,size):
            nonlocal hits
            if (address,size)==(LINK,8):
                hits+=1
                if hits==1:return bytes(8)
            return read(address,size)
        with mock.patch.object(f,'read',side_effect=transient):error=reject(f)
        self.assertIs(type(error),anchor.AnchorControlledLinkChanged)
        self.assertEqual(error.evidence['controlled_link']['after'],f.TURRET)
        self.assertTrue(start.observer_preparation_churn(error))
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
            config,metadata=anchor.build_anchor_config(f,sample_pose=False,cinematic_camera=True)
        self.assertEqual(config['schema'],3);self.assertTrue(metadata['read_only'])

    def test_wrong_property_type_owner_layout_offset_and_slot_stay_hard(self):
        for fault in ('number','kind','outer','array','size','offset','slot'):
            f=unpossessed();prop=f.properties[0x6000,'Pawn'];field=prop['address']
            if fault=='number':f.word(field+76,1)
            if fault=='kind':f.ptr(field+80,f.classes['FloatProperty'])
            if fault=='outer':f.ptr(field+64,f.classes['WarHUD'])
            if fault=='array':f.word(field+0x70,2)
            if fault=='size':f.word(field+0x74,4)
            if fault=='offset':f.word(field+0x94,prop['offset']+4)
            if fault=='slot':f.ptr(f.objects_data+f.u32(field+8)*8,0)
            with self.subTest(fault=fault):self.assertFalse(start.observer_preparation_churn(reject(f)))

    def test_identity_and_super_changes_during_null_verification_stay_hard(self):
        for fault in ('controller','field','kind','parent','super'):
            f=unpossessed();field=f.properties[0x6000,'Pawn']['address'];read=f.read;changed=False
            def changing(address,size):
                nonlocal changed
                result=read(address,size)
                trigger=(address,size)==((LINK,8)if fault=='controller'else(field+0x70,8))
                if trigger and not changed:
                    changed=True
                    if fault=='super':f.ptr(0x5000+96,0)
                    else:f.word(({'controller':0x10000,'field':field,'kind':0x69000,'parent':0x5000}[fault])+76,1)
                return result
            with self.subTest(fault=fault),mock.patch.object(f,'read',side_effect=changing):
                error=reject(f);self.assertTrue(changed);self.assertFalse(start.observer_preparation_churn(error))

    def test_header_churn_is_typed_but_invalid_headers_and_unreadable_layout_stay_hard(self):
        f=unpossessed();f.put(f.objects_header,struct.pack('<Qii',f.objects_data,f.object_count+1,f.object_capacity+1))
        self.assertIs(type(reject(f)),anchor.AnchorRegistryChanged)
        f=unpossessed();f.put(f.objects_header,struct.pack('<Qii',0,0,0))
        self.assertFalse(start.observer_preparation_churn(reject(f)))
        f=unpossessed();read=f.read;field=f.properties[0x6000,'Pawn']['address']
        def unreadable(address,size):
            if (address,size)==(field+0x70,8):raise OSError('unreadable layout')
            return read(address,size)
        with mock.patch.object(f,'read',side_effect=unreadable):self.assertFalse(start.observer_preparation_churn(reject(f)))

    def test_null_elsewhere_unknown_subclass_and_message_only_do_not_qualify(self):
        f=MountedUIFixture();camera=dict(f.resolve(),controller_address='0x0')
        with self.assertRaises(anchor.AnchorSamplingRejected)as generic:anchor.LiveRegistryAnchorSampler(f,prepared_camera=camera)
        self.assertFalse(start.observer_preparation_churn(generic.exception))
        typed=reject(unpossessed())
        class Unknown(anchor.AnchorControlledPawnUnavailable):pass
        for error in (RuntimeError(str(typed)),anchor.AnchorSamplingRejected(str(typed),typed.evidence),Unknown(str(typed),typed.evidence)):
            self.assertFalse(start.observer_preparation_churn(error))

    def test_owner_retries_real_unpossession_then_commits_fresh_mounted_state(self):
        f=MountedUIFixture();links={address:f.read(address,8)for address in (LINK,0x20250,0x20810,0x30500)}
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
            mounted,_=anchor.build_anchor_config(f,sample_pose=False,cinematic_camera=True)
            f.dismount();normal,_=anchor.build_anchor_config(f,sample_pose=False,cinematic_camera=True)
            for address,value in links.items():f.put(address,value)
            f.ptr(LINK,0);h=Harness();h.current=runtime.snapshot(mounted);calls=[0]
            def prepare(*args):
                calls[0]+=1
                if calls[0]>5:f.ptr(LINK,f.TURRET)
                runtime._read_only_acquisition(anchor.LiveRegistryAnchorSampler,f,cinematic_camera=True)
                return h.prepare(*args)
            with h.monitor(prepare=prepare,baseline=runtime.snapshot(normal))as monitor:
                for delay in (.05,.1,.15,1.7,.3):
                    monitor.poll(h.status);events=h.finish_worker(monitor)
                    self.assertEqual(events[0]['retry_scope'],'acquisition_churn')
                    self.assertAlmostEqual(monitor.next_poll-h.now,delay);self.assertEqual(monitor.attempts,0)
                    self.assertFalse(h.signals);h.now=monitor.next_poll
                monitor.poll(h.status);h.finish_worker(monitor);self.assertEqual(len(h.builds),1)
                h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
                h.status.update(world_generation='2',world_candidate_generation='0')
                self.assertEqual(monitor.poll(h.status)[0]['event'],'world_rebind_committed')
                self.assertEqual(monitor.baseline,h.current);self.assertEqual(len(h.signals),2)

    def test_inner_full_acquisitions_close_all_readers_before_outer_retry(self):
        readers=[]
        def reader(*args):
            f=unpossessed();f.p=mock.Mock();f.put(f.base+0x1234,b'ok');readers.append(f);return f
        first=unpossessed();camera=first.resolve()
        with mock.patch.object(start,'Resolver',side_effect=reader),mock.patch.object(start.time,'sleep'),\
             mock.patch.object(start,'remote_call')as activate,contextlib.redirect_stderr(io.StringIO()),\
             self.assertRaises(RetryablePreparation)as raised:
            runtime._read_only_acquisition(start.acquire_observer_preparation,first.pid,first.profile,camera,
                {0x1234:b'ok'},Path('Unused.dll'),anchor_observer=True,ui_observer=True,cinematic_camera=True,
                record_anchor_failure=False,entry_reader=lambda address,size,read:read(address,size))
        self.assertIs(type(raised.exception.error),anchor.AnchorControlledPawnUnavailable)
        self.assertEqual(len(readers),3)
        for r in readers:r.p.close.assert_called_once()
        activate.assert_not_called()


if __name__=='__main__':unittest.main()
