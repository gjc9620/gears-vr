"""Startup discovery locations with synthetic memory; all live proofs stay fresh."""
import copy
import contextlib
import io
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest import mock

import start_capture
import cinematic_anchor
import world_rebind as rebind
import world_rebind_runtime as runtime
from anchor_config import build_anchor_config,AnchorSamplingRejected,LiveRegistryAnchorSampler
from test_mounted_integration import MountedUIFixture


def startup_fixture():
    f=MountedUIFixture();f.p=SimpleNamespace(close=mock.Mock())
    links={address:f.read(address,8)for address in (0x10224,0x20250,0x20810,0x30500)}
    f.dismount()
    camera=dict(f.resolve(),executable=str(Path('synthetic-startup-game.exe').resolve()))
    camera['offsets']=dict(camera['offsets'],CameraCache='0x600',TimeStamp='0x0',POV='0x4',Location='0x0',Rotation='0xc',FOV='0x18')
    with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
        config,metadata=build_anchor_config(f,prepared_camera=camera,cinematic_camera=True,sample_pose=False)
    metadata['pid']=f.pid
    return f,camera,config,metadata,links


def startup_adapter(module,directory,f,camera,metadata):
    path=Path(directory)
    (path/'anchor-observer-config.json').write_text(json.dumps(metadata))
    (path/'capture.ini').write_text('[Capture]\npid='+str(f.pid)+'\ncrosshair_assignment_observer=1\n',encoding='utf-16')
    game=dict(pid=f.pid,creation_time=12345,image=camera['executable'].casefold())
    owner=dict(pid=f.pid+1,creation_time=54321,image=str(Path('synthetic-owner.exe').resolve()).casefold())
    return module.RuntimePreparation(path,'capture.dll','a'*64,game,owner)


class StartupCameraHintTests(unittest.TestCase):
    def test_constructor_seeds_only_optional_locations_without_opening_a_process(self):
        f,camera,config,metadata,_=startup_fixture()
        with tempfile.TemporaryDirectory()as directory,mock.patch.object(runtime,'Resolver')as reader:
            adapter=startup_adapter(runtime,directory,f,camera,metadata)
        reader.assert_not_called();self.assertEqual(adapter._camera_hint,camera)
        self.assertEqual(set(adapter._camera_roles),{'local_player','controller','camera'})
        for key,selected in adapter._camera_roles.items():self.assertEqual(selected,metadata['certificate_final_guard'][key])

    def test_first_mount_builds_and_publishes_full_candidate_without_global_resolve(self):
        f,camera,config,metadata,links=startup_fixture()
        with tempfile.TemporaryDirectory()as directory:
            adapter=startup_adapter(runtime,directory,f,camera,metadata);adapter._native_discovery=None
            adapter.signatures={0x1234:b'synthetic entry'};f.put(f.base+0x1234,b'synthetic entry')
            for address,value in links.items():f.put(address,value)
            proof=SimpleNamespace(pid=f.pid,game_creation=adapter.game['creation_time'],owner=adapter.owner,
                dll=adapter.dll,module_base=f.base,reverify=mock.Mock(),read_entry=lambda address,size,read:read(address,size))
            context=mock.MagicMock();context.__enter__.return_value=proof
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=context),\
                 mock.patch.object(rebind,'process_identity',side_effect=lambda pid:adapter.game if pid==f.pid else adapter.owner),\
                 mock.patch.object(start_capture,'Resolver',return_value=f),\
                 mock.patch.object(f,'prepare',create=True)as prepare,\
                 mock.patch.object(f,'resolve',return_value=camera)as resolve,\
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
                actual,receipt=adapter.prepare_changed(1,1,lambda value:value.changed_from(adapter.baseline),lambda:None)
            prepare.assert_called_once();resolve.assert_not_called();f.p.close.assert_called_once()
            context.__exit__.assert_called_once();self.assertEqual(proof.reverify.call_count,2)
            self.assertTrue(actual.mounted_context);self.assertEqual(actual,receipt.snapshot)
            payload=receipt.path.read_bytes().decode('utf-16')
            self.assertIn('mounted_enabled=1\n',payload);self.assertIn('[CrosshairAssignment]',payload)
            self.assertEqual(len(list(Path(directory).glob('world-rebind-*.ini'))),1)

    def test_first_observation_keeps_live_mount_state_and_closes_its_reader(self):
        f,camera,config,metadata,links=startup_fixture()
        with tempfile.TemporaryDirectory()as directory:
            adapter=startup_adapter(runtime,directory,f,camera,metadata);adapter._native_discovery=None
            for address,value in links.items():f.put(address,value)
            with mock.patch.object(runtime,'Resolver',return_value=f),mock.patch.object(f,'prepare',create=True)as prepare,\
                 mock.patch.object(f,'resolve',return_value=camera)as resolve,\
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
                actual=adapter.observe(lambda:None)
            prepare.assert_called_once();resolve.assert_not_called();f.p.close.assert_called_once()
            self.assertNotEqual(actual.pawn,adapter.baseline.pawn);self.assertTrue(actual.mounted_context)

    def test_each_selected_generation_is_rechecked_before_first_preparation(self):
        for role in ('local_player','controller','camera'):
            f,camera,config,metadata,_=startup_fixture()
            with self.subTest(role=role),tempfile.TemporaryDirectory()as directory:
                adapter=startup_adapter(runtime,directory,f,camera,metadata);adapter._native_discovery=None
                item=config['identities'][config['roles'][role]];f.word(item['address']+76,item['number']+1)
                with mock.patch.object(f,'prepare',create=True),mock.patch.object(f,'resolve',return_value=camera)as resolve:
                    with self.assertRaises(runtime.DiscoveryRegistryChanged):adapter._preparation_camera(f)
                    self.assertIsNone(adapter._camera_hint);self.assertIsNone(adapter._camera_roles)
                    adapter._preparation_camera(f);resolve.assert_called_once()

    def test_invalid_optional_hint_falls_back_to_global_discovery(self):
        f,camera,config,original,_=startup_fixture()
        for fault in ('sampler','resolver','guard','pid','hash','module','role','name_index','executable',
                      'player_camera','pc_owner','player','cache','timestamp','pov','timestamp_address','pov_address',
                      'rotation','location','fov','field_type'):
            metadata=copy.deepcopy(original);source=metadata['sampler']['resolver']
            if fault=='sampler':metadata['sampler']=None
            elif fault=='resolver':metadata['sampler']['resolver']=None
            elif fault=='guard':metadata.pop('certificate_final_guard')
            elif fault=='pid':source['pid']+=1
            elif fault=='hash':source['sha256']='b'*64
            elif fault=='module':source['module_base']=hex(f.base+4096)
            elif fault=='role':source['controller_address']='0xdead'
            elif fault=='name_index':metadata['certificate_final_guard']['camera']['name_index']+=1
            elif fault=='executable':source['executable']=str(Path('other-fixture.exe').resolve())
            elif fault=='field_type':metadata['config']['fields']['camera_timestamp']=True
            elif fault in ('timestamp_address','pov_address'):source[fault]=hex(int(source[fault],0)+4)
            else:
                key={'player_camera':'PlayerCamera','pc_owner':'PCOwner','player':'Player','cache':'CameraCache',
                     'timestamp':'TimeStamp','pov':'POV','rotation':'Rotation','location':'Location','fov':'FOV'}[fault]
                source['offsets'][key]=hex(int(source['offsets'][key],0)+4)
            with self.subTest(fault=fault),tempfile.TemporaryDirectory()as directory:
                adapter=startup_adapter(runtime,directory,f,camera,metadata);adapter._native_discovery=None
                self.assertIsNone(adapter._camera_hint);self.assertIsNone(adapter._camera_roles)
                with mock.patch.object(f,'resolve',return_value=camera)as resolve:
                    self.assertEqual(adapter._preparation_camera(f),camera);resolve.assert_called_once()

    def test_seeded_locations_and_roles_own_independent_copies(self):
        f,camera,config,metadata,_=startup_fixture()
        with tempfile.TemporaryDirectory()as directory:
            adapter=startup_adapter(runtime,directory,f,camera,metadata)
            self.assertTrue(adapter._remember_startup_camera(metadata))
            saved_camera=copy.deepcopy(adapter._camera_hint);saved_roles=copy.deepcopy(adapter._camera_roles)
            metadata['sampler']['resolver']['offsets']['PlayerCamera']='0x0'
            metadata['certificate_final_guard']['camera']['number']+=9
            self.assertEqual(adapter._camera_hint,saved_camera);self.assertEqual(adapter._camera_roles,saved_roles)
            self.assertFalse(adapter._remember_startup_camera(metadata))
            self.assertIsNone(adapter._camera_hint);self.assertIsNone(adapter._camera_roles)

    def test_startup_hint_cannot_hide_changes_at_final_fresh_guard(self):
        f,camera,config,metadata,_=startup_fixture();guard=LiveRegistryAnchorSampler._strict_guard
        with tempfile.TemporaryDirectory()as directory:
            adapter=startup_adapter(runtime,directory,f,camera,metadata);adapter._native_discovery=None
            def changing(sampler):
                if sampler.guard_acquisitions[-1]['stage']=='final_boundary':f.word(sampler.camera+76,1)
                return guard(sampler)
            with mock.patch.object(f,'prepare',create=True),mock.patch.object(f,'resolve',side_effect=AssertionError('global scan')),\
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)),\
                 mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',changing):
                fresh=adapter._preparation_camera(f)
                with self.assertRaises(AnchorSamplingRejected):build_anchor_config(f,prepared_camera=fresh,cinematic_camera=True,sample_pose=False)

    def test_invalid_initial_hint_closes_reader_then_fully_rediscovers_without_fixed_sleep(self):
        initial,camera,config,metadata,_=startup_fixture();readers=[]
        with tempfile.TemporaryDirectory()as directory:
            adapter=startup_adapter(runtime,directory,initial,camera,metadata);adapter._native_discovery=None
            def reader(*args):
                if readers:readers[-1].p.close.assert_called_once()
                f,_,_,_,_=startup_fixture();f.word(0x30000+76,1);f.put(f.base+0x1234,b'synthetic entry')
                f.prepare=mock.Mock();f.resolve=mock.Mock(return_value=dict(camera,camera_number=1));readers.append(f)
                return f
            with mock.patch.object(start_capture,'Resolver',side_effect=reader),\
                 mock.patch.object(start_capture.time,'sleep')as sleep,\
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(initial.code)),contextlib.redirect_stderr(io.StringIO()):
                result=start_capture.acquire_observer_preparation(initial.pid,initial.profile,camera,{0x1234:b'synthetic entry'},adapter.dll,
                    anchor_observer=True,ui_observer=True,cinematic_camera=True,record_anchor_failure=False,anchor_sample_pose=False,
                    entry_reader=lambda address,size,read:read(address,size),camera_discovery=adapter._preparation_camera)
            self.assertEqual(len(readers),2);sleep.assert_not_called()
            for r in readers:r.p.close.assert_called_once()
            readers[0].resolve.assert_not_called();readers[1].resolve.assert_called_once()
            selected=result[1]['config'];identity=selected['identities'][selected['roles']['camera']]
            self.assertEqual(identity['number'],1);self.assertTrue(result[3]['accepted'])

    def test_immediate_hint_fallback_remains_exact_typed_and_attempt_deadline_bounded(self):
        class Unknown(start_capture.DiscoveryHintChanged):pass
        for kind,attempts,sleeps in ((start_capture.DiscoveryHintChanged,3,0),(Unknown,3,2),
                                    (start_capture.DiscoveryRegistryChanged,3,2),(RuntimeError,1,0)):
            readers=[];error=kind('Camera discovery hint changed; reacquire with a fresh reader')
            def reader(*args):
                r=mock.Mock();readers.append(r);return r
            with self.subTest(kind=kind.__name__),mock.patch.object(start_capture,'Resolver',side_effect=reader),\
                 mock.patch.object(start_capture.time,'sleep')as sleep,mock.patch.object(start_capture.time,'monotonic',return_value=0),\
                 contextlib.redirect_stderr(io.StringIO()),self.assertRaises(kind)as rejected:
                start_capture.acquire_observer_preparation(1,{}, {},{1:b'ok'},Path('Unused.dll'),ui_observer=True,
                    camera_discovery=mock.Mock(side_effect=error))
            self.assertIs(rejected.exception,error);self.assertEqual(len(readers),attempts);self.assertEqual(sleep.call_count,sleeps)
            for r in readers:r.p.close.assert_called_once()
        r=mock.Mock();error=start_capture.DiscoveryHintChanged('Optional source changed')
        with mock.patch.object(start_capture,'Resolver',return_value=r)as create,\
             mock.patch.object(start_capture.time,'sleep')as sleep,mock.patch.object(start_capture.time,'monotonic',side_effect=[0,2]),\
             self.assertRaises(start_capture.DiscoveryHintChanged):
            start_capture.acquire_observer_preparation(1,{}, {},{1:b'ok'},Path('Unused.dll'),ui_observer=True,preparation_deadline=1,
                camera_discovery=mock.Mock(side_effect=error))
        create.assert_called_once();r.p.close.assert_called_once();sleep.assert_not_called()


if __name__=='__main__':unittest.main()
