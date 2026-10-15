"""Owner candidate preparation only: mocked readers, no game/process/tools."""
import copy
import configparser
import dataclasses
import hashlib
import pathlib
import tempfile
import unittest
import types
from unittest import mock

import world_rebind as rebind


class Tests(unittest.TestCase):
    def setUp(self):
        self.digest='a'*64;self.base=0x140000000
        self.game=dict(pid=7,creation_time=70,image=str(pathlib.Path('game.exe').resolve()).casefold())
        self.owner=dict(pid=8,creation_time=80,image='owner.exe')
        def ident(address,name,index):
            return dict(address=address,name=name,number=0,cls=0x9000,outer=0x8000,internal_index=index)
        self.local=ident(0x1000,'WarLocalPlayer',1)
        self.controller=ident(0x2000,'Controller',2);self.camera_id=ident(0x3000,'WarPlayerCamera',3)
        self.pawn=ident(0x4000,'Pawn',4)
        self.camera=dict(pid=7,module_base=hex(self.base),sha256=self.digest,executable=self.game['image'],
                         local_player_address='0x1000',controller_address='0x2000',camera_address='0x3000',
                         timestamp_address='0x3010',pov_address='0x3020',offsets={'Rotation':'0xc'})
        self.anchor_config=dict(schema=2,cinematic={'module_base':self.base},
            roles={'local_player':0,'controller':1,'camera':2,'pawn':3},
            identities=[self.local,self.controller,self.camera_id,self.pawn],
            fields={'camera_timestamp':0x10,'camera_pov':0x20})
        self.ui_config=dict(schema=1,pid=7,base=self.base,sha256=self.digest,
            roles={'controller':0,'camera':1,'pawn':2},identities=[self.controller,self.camera_id,self.pawn])
        self.anchor=dict(schema=1,pid=7,read_only=True,config=self.anchor_config,sampler={'resolver':self.camera},
            certificate_final_guard={'local_player':self.local,'controller':self.controller,'camera':self.camera_id})
        self.ui=dict(schema=1,pid=7,accepted=True,read_only=True,sha256=self.digest,config=self.ui_config)
        self.signatures={0x123:b'original verified bytes'}
        # Canonical serializer dependencies are mocked, not validation in the
        # existing builders; those have their own CPU certificate fixtures.
        self.anchor_text=lambda config:'[AnchorObserver]\nschema=2\ncamera='+str(config['identities'][config['roles']['camera']]['address'])+'\n'
        self.ui_text=lambda config:'[UIObserver]\nschema=1\nbase='+str(config['base'])+'\n'
        def acquire(*args,**kwargs):
            kwargs['check_live']()
            return self.camera,self.anchor,self.anchor_text(self.anchor_config),self.ui,self.ui_text(self.ui_config)
        self.patches=[mock.patch.object(rebind,'process_identity',side_effect=lambda pid:self.game if pid==7 else self.owner if pid==8 else None),
                      mock.patch.object(rebind,'resolve_camera_once',return_value=copy.deepcopy(self.camera)),
                      mock.patch.object(rebind,'acquire_observer_preparation',side_effect=acquire),
                      mock.patch.object(rebind,'anchor_ini',side_effect=self.anchor_text),
                      mock.patch.object(rebind,'ui_observer_ini',side_effect=self.ui_text)]
        self.identity,self.resolve,self.acquire,self.ai,self.uii=[patch.start() for patch in self.patches]
        for patch in self.patches:self.addCleanup(patch.stop)

    def build(self,**kwargs):
        args=dict(pid=7,profile={'sha256':self.digest},previous_local_identity=self.local,
                  expected_generation=1,owner_identity=self.owner,dll=pathlib.Path('unused.dll'),signatures=self.signatures)
        args.update(kwargs);return rebind.build_candidate(**args)

    def test_bundle_is_deeply_immutable_and_uses_full_existing_preparation(self):
        live=mock.Mock();bundle=self.build(check_live=live)
        self.assertEqual((bundle.schema,bundle.previous_generation,bundle.generation),(1,1,2))
        self.assertEqual(bundle.module_base,self.base)
        self.assertEqual(dict(bundle.local_identity),self.local)
        args,kw=self.acquire.call_args
        self.assertEqual(args[3],self.signatures)
        self.assertTrue(all(kw[k] for k in ('anchor_observer','ui_observer','cinematic_camera')))
        self.assertFalse(kw['record_anchor_failure']);self.assertGreaterEqual(live.call_count,4)
        with self.assertRaises(dataclasses.FrozenInstanceError):bundle.generation=4
        with self.assertRaises(TypeError):bundle.current_camera['camera_address']='0xdead'
        with self.assertRaises(TypeError):bundle.anchor_metadata['config']['identities'][0]['number']=10
        self.camera['camera_address']='0xdead';self.local['name']='changed'
        self.assertEqual(bundle.current_camera['camera_address'],'0x3000')
        self.assertEqual(bundle.local_identity['name'],'WarLocalPlayer')

    def test_serialized_metadata_and_capture_addresses_match_new_camera(self):
        bundle=self.build();data=rebind.serialize_candidate(bundle)
        self.assertTrue(data.startswith(b'\xff\xfe'))
        parsed=configparser.ConfigParser();parsed.read_string(data.decode('utf-16'))
        self.assertEqual(parsed.sections(),['WorldRebind','Capture','AnchorObserver','UIObserver'])
        meta=parsed['WorldRebind']
        self.assertEqual([meta[k] for k in ('schema','game_pid','game_creation','owner_pid','owner_creation','previous_generation','generation')],
                         ['1','7','70','8','80','1','2'])
        self.assertEqual(meta['game_sha256'],self.digest)
        self.assertEqual(dict(parsed['Capture']),dict(pid='7',local_player='0x1000',controller='0x2000',
                         camera='0x3000',timestamp='0x3010',camera_yaw='0x3030'))
        self.assertIn(bundle.anchor_ini+bundle.ui_ini,data.decode('utf-16'))

    def test_all_certificates_follow_new_camera_not_initial_discovery(self):
        baseline=self.resolve.return_value
        baseline.update(controller_address='0x5000',camera_address='0x6000',timestamp_address='0x6010',pov_address='0x6020')
        bundle=self.build()
        self.assertEqual(self.acquire.call_args.args[2]['camera_address'],'0x6000')
        self.assertEqual(bundle.current_camera['camera_address'],'0x3000')
        self.assertEqual(bundle.anchor_metadata['sampler']['resolver']['camera_address'],'0x3000')
        self.assertEqual(bundle.ui_metadata['config']['identities'][1]['address'],0x3000)

    def test_attestation_is_explicit_bound_and_verified_before_after_build(self):
        attestation=types.SimpleNamespace(pid=7,game_creation=70,owner=dict(self.owner),dll=pathlib.Path('unused.dll').resolve(),
                                          module_base=self.base,reverify=mock.Mock(),read_entry=mock.Mock())
        self.build(attestation=attestation)
        self.assertEqual(attestation.reverify.call_count,2)
        self.assertIs(self.acquire.call_args.kwargs['entry_reader'],attestation.read_entry)
        attestation.reverify.reset_mock();attestation.reverify.side_effect=[None,RuntimeError('sealed manifest changed')]
        with self.assertRaisesRegex(RuntimeError,'sealed manifest changed'):self.build(attestation=attestation)
        attestation.pid=123;self.acquire.reset_mock()
        with self.assertRaisesRegex(ValueError,'Attestation process/owner/DLL mismatch'):self.build(attestation=attestation)
        self.acquire.assert_not_called()

    def test_missing_overflow_and_mismatched_inputs_reject_before_acquisition(self):
        for changes in ({'expected_generation':0},{'expected_generation':(1<<64)-1},{'expected_generation':True},
                        {'pid':1<<32},{'signatures':{}},{'signatures':{1:b''}},
                        {'owner_identity':dict(self.owner,pid=7)},{'owner_identity':dict(self.owner,creation_time=81)},
                        {'previous_local_identity':{'address':0x1000}},
                        {'previous_local_identity':dict(self.local,address=0x999)}):
            with self.subTest(changes=changes):
                self.acquire.reset_mock()
                with self.assertRaises((ValueError,RuntimeError,KeyError)):self.build(**changes)
                self.acquire.assert_not_called()

    def test_metadata_local_role_or_camera_relationship_changes_reject(self):
        mutations=[lambda:self.anchor['certificate_final_guard']['local_player'].update(number=1),
                   lambda:self.anchor_config['identities'][0].update(outer=0x999),
                   lambda:self.ui_config.update(base=self.base+1),
                   lambda:self.ui_config.update(sha256='b'*64),
                   lambda:self.camera.update(timestamp_address='0x999'),
                   lambda:self.anchor['sampler'].update(resolver=dict(self.camera,camera_address='0x888')),
                   lambda:self.ui_config['roles'].update(camera=0)]
        # Each case owns a separate fixture; shared source objects deliberately
        # simulate mutation at acquisition boundaries.
        for mutate in mutations:
            saved=copy.deepcopy((self.camera,self.anchor,self.ui,self.local))
            with self.subTest(mutate=mutate):
                previous=copy.deepcopy(self.local);mutate()
                with self.assertRaises(ValueError):self.build(previous_local_identity=previous)
            self.camera,self.anchor,self.ui,self.local=saved
            self.anchor_config=self.anchor['config'];self.ui_config=self.ui['config']

    def test_owner_change_cancellation_and_builder_failures_never_fallback(self):
        live=mock.Mock(side_effect=RuntimeError('cancelled'))
        with self.assertRaisesRegex(RuntimeError,'cancelled'):self.build(check_live=live)
        self.acquire.assert_not_called()
        error=RuntimeError('UI entry bytes mismatch: vm')
        self.acquire.side_effect=error
        with self.assertRaisesRegex(RuntimeError,'UI entry bytes mismatch'):self.build()
        self.acquire.assert_called_once()
        self.acquire.reset_mock()
        def changed(*args,**kwargs):
            self.owner['creation_time']+=1
            return self.camera,self.anchor,self.anchor_text(self.anchor_config),self.ui,self.ui_text(self.ui_config)
        self.acquire.side_effect=changed
        with self.assertRaisesRegex(RuntimeError,'Owner process identity changed'):self.build()
        self.acquire.assert_called_once()

    def test_publish_new_path_exact_bytes_and_hash_without_overwriting(self):
        bundle=self.build();data=rebind.serialize_candidate(bundle)
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'world-rebind-2.ini'
            digest=rebind.publish_candidate(bundle,path)
            self.assertEqual(digest,hashlib.sha256(data).hexdigest());self.assertEqual(path.read_bytes(),data)
            with self.assertRaises(FileExistsError):rebind.publish_candidate(bundle,path)
            self.assertEqual(path.read_bytes(),data);self.assertEqual(list(path.parent.iterdir()),[path])

    def test_failed_replace_removes_only_owned_files(self):
        bundle=self.build()
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'world-rebind-2.ini'
            with mock.patch.object(rebind.os,'replace',side_effect=PermissionError('busy')):
                with self.assertRaises(PermissionError):rebind.publish_candidate(bundle,path)
            self.assertEqual(list(path.parent.iterdir()),[])

    def test_serialization_rejects_forged_metadata_and_write_failure_cleans_temp(self):
        bundle=self.build()
        for changed in (dataclasses.replace(bundle,schema=True),dataclasses.replace(bundle,generation=4),
                        dataclasses.replace(bundle,anchor_ini='[AnchorObserver]\nschema=1\n')):
            with self.assertRaises(ValueError):rebind.serialize_candidate(changed)
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'world-rebind-2.ini'
            with mock.patch.object(rebind.os,'fsync',side_effect=OSError('write failed')):
                with self.assertRaises(OSError):rebind.publish_candidate(bundle,path)
            self.assertEqual(list(path.parent.iterdir()),[])


if __name__=='__main__':unittest.main()
