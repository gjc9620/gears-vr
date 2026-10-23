"""Production adapter ownership and manifest trust tests; no live process."""
import copy
from collections.abc import Mapping
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import world_rebind_runtime as runtime
from world_rebind import freeze
from world_rebind_monitor import PreparedRebind


def config():
    def obj(i):
        return dict(address=i*100,cls=9000,outer=8000,number=0,internal_index=i,name='object'+str(i))
    return dict(roles={name:i for i,name in enumerate(('local_player','controller','camera','level','pawn'))},
        identities=[obj(i+1) for i in range(6)],cinematic=dict(role_world_info=5))


def initial_guard():
    value=config()
    return dict(**{name:value['identities'][index] for name,index in value['roles'].items()},
                target_proof=dict(world_info=value['identities'][5]))


class Tests(unittest.TestCase):
    def test_preparation_reads_only_operational_metadata_and_keeps_original_publication(self):
        fixture,value,candidate=self._mounted_candidate();original=freeze(candidate.anchor_metadata);reads=[]
        class Tracked(Mapping):
            def __getitem__(self,key):reads.append(key);return original[key]
            def __iter__(self):return iter(original)
            def __len__(self):return len(original)
        candidate.anchor_metadata=Tracked();candidate.current_camera=freeze(candidate.current_camera)
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid);adapter.profile=fixture.profile
            with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate') as publish,mock.patch.object(runtime,'snapshot',wraps=runtime.snapshot) as snapshot:
                actual,receipt=adapter.prepare_changed(1,3,lambda _:True,lambda:None)
            self.assertEqual(actual,receipt.snapshot);self.assertEqual(reads,['config','certificate_final_guard'])
            publish.assert_called_once_with(candidate,receipt.path)
            copied=snapshot.call_args.args[0];selected=value['roles']['controller'];before=original['config']['identities'][selected]['number']
            roles=copy.deepcopy(adapter._camera_roles);copied['identities'][selected]['number']+=100
            self.assertEqual(original['config']['identities'][selected]['number'],before)
            self.assertEqual(adapter._camera_roles,roles)
            self.assertIn('guard_acquisitions',original)

    def test_rejected_snapshot_does_not_copy_diagnostics_or_read_final_hint_guard(self):
        fixture,value,candidate=self._mounted_candidate();original=freeze(candidate.anchor_metadata);reads=[]
        class Tracked(Mapping):
            def __getitem__(self,key):reads.append(key);return original[key]
            def __iter__(self):return iter(original)
            def __len__(self):return len(original)
        candidate.anchor_metadata=Tracked()
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid);adapter.profile=fixture.profile
            with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                actual,receipt=adapter.prepare_changed(1,3,lambda _:False,lambda:None)
            self.assertEqual(actual,runtime.snapshot(value));self.assertIsNone(receipt);self.assertEqual(reads,['config'])
            publish.assert_not_called();self.assertIsNone(adapter._camera_hint)

    def _mounted_candidate(self):
        from test_mounted_integration import MountedUIFixture
        from anchor_config import build_anchor_config
        import cinematic_anchor
        fixture=MountedUIFixture();fixture.p=SimpleNamespace(close=mock.Mock())
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(fixture.code)):
            config,metadata=build_anchor_config(fixture,cinematic_camera=True)
        return fixture,config,SimpleNamespace(current_camera=fixture.resolve(),anchor_metadata=metadata)

    def test_prepared_candidate_seeds_next_fresh_discovery_and_keeps_dismount_live(self):
        import cinematic_anchor
        fixture,value,candidate=self._mounted_candidate()
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid)
            adapter.profile=fixture.profile;adapter._native_discovery=None
            with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                first,receipt=adapter.prepare_changed(1,3,lambda _:True,lambda:None)
            self.assertEqual(receipt.snapshot,first);publish.assert_called_once()
            hint=copy.deepcopy(adapter._camera_hint);roles=copy.deepcopy(adapter._camera_roles)
            self.assertIsNotNone(hint);self.assertEqual(len(roles),3)
            candidate.current_camera['camera_address']='0xdead'
            candidate.anchor_metadata['certificate_final_guard']['camera']['number']+=7
            self.assertEqual(adapter._camera_hint,hint);self.assertEqual(adapter._camera_roles,roles)
            with mock.patch.object(fixture,'prepare',create=True) as prepare,\
                 mock.patch.object(fixture,'resolve',wraps=fixture.resolve) as resolve,\
                 mock.patch.object(runtime,'Resolver',return_value=fixture),\
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(fixture.code)):
                self.assertEqual(adapter._preparation_camera(fixture),hint)
                fixture.dismount();normal=adapter.observe(lambda:None)
            self.assertFalse(normal.mounted_context);self.assertNotEqual(normal.pawn,first.pawn)
            self.assertEqual(prepare.call_count,2);resolve.assert_not_called();fixture.p.close.assert_called_once()

    def test_prepared_hint_rechecks_same_address_role_generations_before_reuse(self):
        for name in ('controller','camera','local_player'):
            fixture,value,candidate=self._mounted_candidate()
            with tempfile.TemporaryDirectory() as directory:
                adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid)
                adapter.profile=fixture.profile;adapter._native_discovery=None
                self.assertTrue(adapter._remember_prepared_camera(candidate,value))
                selected=value['identities'][value['roles'][name]]
                fixture.word(selected['address']+76,selected['number']+1)
                with mock.patch.object(fixture,'prepare',create=True),mock.patch.object(fixture,'resolve',wraps=fixture.resolve) as resolve:
                    with self.assertRaises(runtime.DiscoveryRegistryChanged):adapter._preparation_camera(fixture)
                    self.assertIsNone(adapter._camera_hint);self.assertIsNone(adapter._camera_roles)
                    adapter._preparation_camera(fixture);resolve.assert_called_once()

    def test_optional_prepared_hint_rejects_missing_or_inconsistent_metadata(self):
        fixture,value,original=self._mounted_candidate()
        for fault in ('missing_camera','missing_guard','pid','module','hash','name_index','role'):
            candidate=copy.deepcopy(original)
            if fault=='missing_camera':del candidate.current_camera
            if fault=='missing_guard':del candidate.anchor_metadata['certificate_final_guard']
            if fault=='pid':candidate.current_camera['pid']+=1
            if fault=='module':candidate.current_camera['module_base']=hex(fixture.base+4096)
            if fault=='hash':candidate.current_camera['sha256']='b'*64
            if fault=='name_index':candidate.anchor_metadata['certificate_final_guard']['camera']['name_index']+=1
            if fault=='role':candidate.anchor_metadata['certificate_final_guard']['camera']['address']+=8
            with tempfile.TemporaryDirectory() as directory:
                adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid);adapter.profile=fixture.profile
                adapter._camera_hint={'old':True};adapter._camera_roles={'old':True}
                self.assertFalse(adapter._remember_prepared_camera(candidate,value),fault)
                self.assertIsNone(adapter._camera_hint,fault);self.assertIsNone(adapter._camera_roles,fault)

    def test_rejected_snapshot_does_not_seed_new_discovery_hints(self):
        fixture,value,candidate=self._mounted_candidate()
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory);adapter.game=dict(adapter.game,pid=fixture.pid);adapter.profile=fixture.profile
            with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate') as publish,\
                 mock.patch.object(adapter,'_remember_prepared_camera',wraps=adapter._remember_prepared_camera) as remember:
                actual,receipt=adapter.prepare_changed(1,3,lambda _:False,lambda:None)
            self.assertEqual(actual,runtime.snapshot(value));self.assertIsNone(receipt)
            publish.assert_not_called();remember.assert_not_called();self.assertIsNone(adapter._camera_hint)

    def test_topology_hint_only_reads_two_links_and_always_closes(self):
        import struct
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory);adapter._topology_fields={'controller_pawn':548,'player_camera':892}
            baseline=adapter.baseline;check=mock.Mock()
            for pawn,camera,changed in ((baseline.pawn[0],baseline.camera[0],False),
                    (999,baseline.camera[0],True),(baseline.pawn[0],999,True),(0,baseline.camera[0],False)):
                reader=SimpleNamespace(read=mock.Mock(side_effect=[struct.pack('<Q',pawn),struct.pack('<Q',camera)]),
                    p=SimpleNamespace(close=mock.Mock()))
                with mock.patch.object(runtime,'Resolver',return_value=reader):
                    self.assertIs(adapter.changed_hint(baseline,check),changed)
                self.assertEqual(reader.read.call_count,2);reader.p.close.assert_called_once()
            reader.read.side_effect=OSError('gone');reader.p.close.reset_mock()
            with mock.patch.object(runtime,'Resolver',return_value=reader),self.assertRaises(OSError):adapter.changed_hint(baseline,check)
            reader.p.close.assert_called_once()

    def test_preparation_hint_discards_generation_changes_before_fresh_retry(self):
        from test_mounted_integration import MountedUIFixture
        import cinematic_anchor
        f=MountedUIFixture();f.p=SimpleNamespace(close=mock.Mock())
        adapter=runtime.RuntimePreparation.__new__(runtime.RuntimePreparation)
        adapter.game={'pid':f.pid};adapter.profile=f.profile
        with mock.patch.object(runtime,'Resolver',return_value=f),mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)),\
                mock.patch.object(f,'prepare',create=True),mock.patch.object(f,'resolve',wraps=f.resolve) as resolve:
            adapter.observe(lambda:None)
            adapter._preparation_camera(f);self.assertEqual(resolve.call_count,1)
            f.word(f.sampler.camera+76,1)
            with self.assertRaises(runtime.DiscoveryRegistryChanged):adapter._preparation_camera(f)
            self.assertIsNone(adapter._camera_hint)
            adapter._preparation_camera(f);self.assertEqual(resolve.call_count,2)

    def test_cached_discovery_keeps_live_mount_guard_and_skips_global_scan(self):
        from test_mounted_integration import MountedUIFixture
        import cinematic_anchor
        f=MountedUIFixture();f.p=SimpleNamespace(close=mock.Mock())
        adapter=runtime.RuntimePreparation.__new__(runtime.RuntimePreparation)
        adapter.game={'pid':f.pid};adapter.profile=f.profile
        with mock.patch.object(runtime,'Resolver',return_value=f), \
             mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)), \
             mock.patch.object(f,'resolve',wraps=f.resolve) as resolve, \
             mock.patch.object(f,'prepare',create=True) as prepare:
            first=adapter.observe(lambda:None)
            self.assertEqual(adapter.observe(lambda:None),first)
            self.assertEqual(resolve.call_count,1);prepare.assert_called_once()
            # Camera/controller/local roles are unchanged while mounting state
            # changes: the fresh guard must still observe the new human Pawn.
            f.dismount();human=adapter.observe(lambda:None)
            self.assertNotEqual(human.pawn,first.pawn)
            self.assertFalse(human.mounted_context)
            self.assertEqual(resolve.call_count,1)

    def test_changed_cached_role_discards_reader_and_rediscovers(self):
        from test_mounted_integration import MountedUIFixture
        import cinematic_anchor
        f=MountedUIFixture();f.p=SimpleNamespace(close=mock.Mock())
        adapter=runtime.RuntimePreparation.__new__(runtime.RuntimePreparation)
        adapter.game={'pid':f.pid};adapter.profile=f.profile
        with mock.patch.object(runtime,'Resolver',return_value=f), \
             mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)), \
             mock.patch.object(f,'resolve',wraps=f.resolve) as resolve, \
             mock.patch.object(f,'prepare',create=True):
            first=adapter.observe(lambda:None)
            f.word(f.sampler.camera+76,1)
            second=adapter.observe(lambda:None)
            self.assertNotEqual(first.camera,second.camera)
            self.assertEqual(resolve.call_count,2)
            self.assertEqual(f.p.close.call_count,3)

    def adapter(self, directory, crosshair=None):
        path=Path(directory)
        (path/'anchor-observer-config.json').write_text(json.dumps(dict(pid=7,read_only=True,config=config())))
        (path/'capture.ini').write_text('[Capture]\npid=7\nfamily_diagnostics=1\nviewport_diagnostics=1\nview_union_mask_fix=1\nshadow_view=1\nshadow_state=1\nshadow_constructor=1\n',encoding='utf-16')
        if crosshair is not None:
            with (path/'capture.ini').open('a',encoding='utf-16') as output:
                output.write('crosshair_assignment_observer='+str(int(crosshair))+'\n')
        return runtime.RuntimePreparation(path,'capture.dll','a'*64,
            dict(pid=7,creation_time=70,image='game.exe'),dict(pid=8,creation_time=80,image='owner.exe'))

    def test_rejects_missing_cinematic_worldinfo_and_invalid_manifest_digest(self):
        invalid=config();invalid['cinematic']={}
        with self.assertRaises(ValueError):runtime.snapshot(invalid)
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError,'manifest SHA256'):
                runtime.RuntimePreparation(directory,'capture.dll','',{}, {})

    def test_startup_crosshair_flag_is_forwarded_only_when_enabled(self):
        for enabled in (None,False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as directory:
                adapter=self.adapter(directory,crosshair=enabled)
                self.assertEqual(adapter.crosshair_assignment_observer,bool(enabled))
                proof=mock.MagicMock();candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
                with mock.patch.object(runtime,'HookEntryAttestation',return_value=proof),\
                     mock.patch.object(runtime,'build_candidate',return_value=candidate) as build,\
                     mock.patch.object(runtime,'publish_candidate'):
                    adapter.prepare(adapter.baseline,1,3,lambda:None)
                self.assertIs(build.call_args.kwargs['attestation'],proof.__enter__.return_value)
                if enabled:self.assertIs(build.call_args.kwargs['crosshair_assignment_observer'],True)
                else:self.assertNotIn('crosshair_assignment_observer',build.call_args.kwargs)

    def test_one_acquisition_applies_fresh_snapshot_predicate_before_publication(self):
        for accepted in (False,True):
            with self.subTest(accepted=accepted),tempfile.TemporaryDirectory() as directory:
                adapter=self.adapter(directory)
                value=config();value['identities'][value['roles']['pawn']]['number']=7
                candidate=SimpleNamespace(anchor_metadata=dict(config=value))
                accept=mock.Mock(return_value=accepted)
                with mock.patch.object(runtime,'HookEntryAttestation') as proof, \
                     mock.patch.object(runtime,'build_candidate',return_value=candidate) as build, \
                     mock.patch.object(runtime,'publish_candidate') as publish, \
                     mock.patch.object(adapter,'observe',side_effect=AssertionError('No separate discovery')):
                    actual,receipt=adapter.prepare_changed(1,3,accept,lambda:None)
                self.assertEqual(actual,runtime.snapshot(value));accept.assert_called_once_with(actual)
                build.assert_called_once()
                self.assertIs(build.call_args.kwargs['attestation'],proof.return_value.__enter__.return_value)
                if accepted:
                    self.assertEqual(receipt.snapshot,actual)
                    self.assertEqual((receipt.previous_generation,receipt.request),(1,3))
                    publish.assert_called_once_with(candidate,receipt.path)
                else:
                    self.assertIsNone(receipt);publish.assert_not_called()

    def test_one_acquisition_identity_or_candidate_failure_cannot_publish(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            with mock.patch.object(runtime,'HookEntryAttestation') as proof, \
                 mock.patch.object(runtime,'build_candidate') as build, \
                 mock.patch.object(runtime,'publish_candidate') as publish:
                proof.return_value.__enter__.side_effect=RuntimeError('Wrong owner')
                with self.assertRaisesRegex(RuntimeError,'Wrong owner'):
                    adapter.prepare_changed(1,3,lambda _:True,lambda:None)
                build.assert_not_called();publish.assert_not_called()
                proof.return_value.__enter__.side_effect=None
                build.side_effect=ValueError('Invalid anchor')
                with self.assertRaisesRegex(ValueError,'Invalid anchor'):
                    adapter.prepare_changed(1,3,lambda _:True,lambda:None)
                publish.assert_not_called()

    def test_normal_prepare_still_rejects_missing_observation(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
            with mock.patch.object(runtime,'HookEntryAttestation'), \
                 mock.patch.object(runtime,'build_candidate',return_value=candidate), \
                 mock.patch.object(runtime,'publish_candidate') as publish:
                with self.assertRaisesRegex(RuntimeError,'World changed'):
                    adapter.prepare(None,1,3,lambda:None)
                publish.assert_not_called()

    def test_observation_closes_rpm_lease_on_success_and_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            for error in (False,True):
                adapter._camera_hint=None
                reader=SimpleNamespace(resolve=mock.Mock(return_value={}),p=SimpleNamespace(close=mock.Mock()))
                with mock.patch.object(runtime,'Resolver',return_value=reader),mock.patch.object(runtime,'LiveRegistryAnchorSampler',
                    side_effect=RuntimeError('load changed') if error else None,return_value=SimpleNamespace(initial=initial_guard())):
                    if error:
                        with self.assertRaises(RuntimeError):adapter.observe(lambda:None)
                    else:self.assertEqual(adapter.observe(lambda:None),adapter.baseline)
                reader.p.close.assert_called_once()

    def test_observe_uses_strict_initial_guard_without_measurement(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            camera={'camera_address':'0x1234'}
            reader=SimpleNamespace(resolve=mock.Mock(return_value=camera),p=SimpleNamespace(close=mock.Mock()))
            sampler=SimpleNamespace(initial=initial_guard(),sample=mock.Mock(side_effect=AssertionError('measurement not needed')))
            check=mock.Mock()
            with mock.patch.object(runtime,'Resolver',return_value=reader),\
                 mock.patch.object(runtime,'LiveRegistryAnchorSampler',return_value=sampler) as construct:
                observed=adapter.observe(check)
            construct.assert_called_once_with(reader,prepared_camera=camera,cinematic_camera=True)
            sampler.sample.assert_not_called();self.assertEqual(check.call_count,3)
            self.assertEqual(observed,adapter.baseline)
            sampler.initial['pawn']['number']=9
            self.assertEqual(observed,adapter.baseline) # Snapshot owns immutable identity copies.
            reader.p.close.assert_called_once()

    def test_observe_never_substitutes_missing_world_proof(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            reader=SimpleNamespace(resolve=mock.Mock(return_value={}),p=SimpleNamespace(close=mock.Mock()))
            guard=initial_guard();guard['target_proof']={}
            with mock.patch.object(runtime,'Resolver',return_value=reader),\
                 mock.patch.object(runtime,'LiveRegistryAnchorSampler',return_value=SimpleNamespace(initial=guard)):
                with self.assertRaises(KeyError):adapter.observe(lambda:None)
            reader.p.close.assert_called_once()

    def test_observe_closes_reader_when_cancelled_after_guard(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            reader=SimpleNamespace(resolve=mock.Mock(return_value={}),p=SimpleNamespace(close=mock.Mock()))
            check=mock.Mock(side_effect=[None,None,RuntimeError('cancelled')])
            with mock.patch.object(runtime,'Resolver',return_value=reader),\
                 mock.patch.object(runtime,'LiveRegistryAnchorSampler',return_value=SimpleNamespace(initial=initial_guard())):
                with self.assertRaisesRegex(RuntimeError,'cancelled'):adapter.observe(check)
            reader.p.close.assert_called_once()

    def test_prepare_uses_manifest_digest_and_closes_proof(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
            proof=mock.MagicMock()
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=proof) as attest,\
                 mock.patch.object(runtime,'build_candidate',return_value=candidate)as build,\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                result=adapter.prepare(adapter.baseline,1,3,lambda:None)
                self.assertIsInstance(result,PreparedRebind)
                self.assertEqual(result.path.name,'world-rebind-70-2-3.ini')
                self.assertEqual(attest.call_args.kwargs['expected_dll_sha256'],'a'*64)
                self.assertIs(build.call_args.kwargs['sample_pose'],False)
                proof.__exit__.assert_called_once()
                publish.assert_called_once_with(candidate,result.path)

    def test_changed_certificate_is_never_published(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            changed=copy.deepcopy(config());changed['identities'][1]['number']=1
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=mock.MagicMock()),\
                 mock.patch.object(runtime,'build_candidate',return_value=SimpleNamespace(anchor_metadata=dict(config=changed))),\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                with self.assertRaisesRegex(RuntimeError,'World changed'):
                    adapter.prepare(adapter.baseline,1,1,lambda:None)
                publish.assert_not_called()


if __name__=='__main__':unittest.main()
