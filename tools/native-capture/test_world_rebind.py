"""Owner candidate preparation only: mocked readers, no game/process/tools."""
import copy
import configparser
import contextlib
import dataclasses
import hashlib
import io
import pathlib
import tempfile
import unittest
import types
from collections.abc import Mapping
from unittest import mock

import world_rebind as rebind
import start_capture as start


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
            return self.camera,self.anchor,self.anchor_text(self.anchor_config),self.ui,rebind.ui_bundle_ini(self.ui)
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

    def enable_crosshair(self):
        from test_crosshair_assignment_config import Fixture
        from crosshair_assignment_config import build_crosshair_assignment_config
        fixture=Fixture()
        cc=build_crosshair_assignment_config(fixture,fixture.camera,include_tick_roots=True,include_writer_view=True)
        cc['pid']=7;self.digest=cc['sha256'];self.camera['sha256']=self.digest
        self.ui['sha256']=self.ui_config['sha256']=self.digest
        self.controller=copy.deepcopy(cc['identities'][cc['roles']['controller']])
        self.pawn=copy.deepcopy(cc['identities'][cc['roles']['pawn']])
        self.camera['controller_address']=hex(self.controller['address'])
        self.anchor_config['identities'][1]=self.ui_config['identities'][0]=self.controller
        self.anchor_config['identities'][3]=self.ui_config['identities'][2]=self.pawn
        self.anchor['certificate_final_guard']['controller']=self.controller
        self.ui['crosshair_assignment']=cc
        return cc

    def test_crosshair_bundle_uses_fresh_acquisition_and_canonical_combined_ini(self):
        self.enable_crosshair();candidate=self.build(crosshair_assignment_observer=True)
        self.assertTrue(self.acquire.call_args.kwargs['crosshair_assignment_observer'])
        self.assertIn('[CrosshairAssignment]\n',candidate.ui_ini)
        self.assertIn('[CrosshairAssignment]\n',rebind.serialize_candidate(candidate).decode('utf-16'))
        self.assertEqual(candidate.ui_ini,rebind.ui_bundle_ini(rebind.thaw(candidate.ui_metadata)))
        invalid=dataclasses.replace(candidate,ui_ini=self.ui_text(self.ui_config))
        with self.assertRaisesRegex(ValueError,'Candidate section mismatch'):rebind.serialize_candidate(invalid)
        missing=rebind.thaw(candidate.ui_metadata);missing.pop('crosshair_assignment')
        with self.assertRaisesRegex(ValueError,'Missing requested crosshair'):
            rebind.serialize_candidate(dataclasses.replace(candidate,ui_metadata=rebind.freeze(missing),
                ui_ini=self.ui_text(self.ui_config)))

    def test_crosshair_missing_or_different_world_roles_reject(self):
        from crosshair_assignment_config import CrosshairConfigRejected
        with self.assertRaisesRegex(ValueError,'Missing requested crosshair'):
            self.build(crosshair_assignment_observer=True)
        cc=self.enable_crosshair();original=copy.deepcopy(cc)
        mutations=[lambda c:c.update(pid=8),lambda c:c.update(base=self.base+1),
                   lambda c:c.update(sha256='b'*64),lambda c:c.update(tick_roots=[]),
                   lambda c:c.update(writer_view=dict(enabled=False))]
        for role in ('controller','pawn'):
            for field,value in (('address',0xdead),('cls',0xbeef),('outer',0xabcdf),('number',99),
                                ('internal_index',3999999),('name','Different'),('name_index',3999999)):
                mutations.append(lambda c,role=role,field=field,value=value:c['identities'][c['roles'][role]].update({field:value}))
        for mutate in mutations:
            cc=copy.deepcopy(original);mutate(cc);self.ui['crosshair_assignment']=cc
            with self.assertRaises((ValueError,CrosshairConfigRejected)):
                self.build(crosshair_assignment_observer=True)

    def test_crosshair_attestation_is_forwarded_and_postproof_failure_rejects(self):
        self.enable_crosshair()
        proof=types.SimpleNamespace(pid=7,game_creation=70,owner=dict(self.owner),dll=pathlib.Path('unused.dll').resolve(),
            module_base=self.base,reverify=mock.Mock(),read_entry=mock.Mock())
        self.build(crosshair_assignment_observer=True,attestation=proof)
        self.assertIs(self.acquire.call_args.kwargs['entry_reader'],proof.read_entry)
        self.assertTrue(self.acquire.call_args.kwargs['crosshair_assignment_observer'])
        proof.reverify.side_effect=[None,RuntimeError('post crosshair proof failed')]
        with self.assertRaisesRegex(RuntimeError,'post crosshair proof failed'):
            self.build(crosshair_assignment_observer=True,attestation=proof)

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

    def test_serialization_keeps_diagnostics_without_traversing_their_graphs(self):
        bundle=self.build();expected=rebind.serialize_candidate(bundle);walked=[]
        class Tracked(Mapping):
            def __init__(self,label):self.label=label;self.value=rebind.freeze({'attempts':[{'records':list(range(100))}]})
            def __getitem__(self,key):return self.value[key]
            def __iter__(self):walked.append(self.label);return iter(self.value)
            def __len__(self):return len(self.value)
        anchor_diag=Tracked('anchor');ui_diag=Tracked('ui')
        anchor=types.MappingProxyType(dict(bundle.anchor_metadata,diagnostic_records=anchor_diag))
        ui=types.MappingProxyType(dict(bundle.ui_metadata,diagnostic_records=ui_diag))
        changed=dataclasses.replace(bundle,anchor_metadata=anchor,ui_metadata=ui)
        self.assertEqual(rebind.serialize_candidate(changed),expected)
        self.assertEqual(walked,[])
        self.assertIs(changed.anchor_metadata['diagnostic_records'],anchor_diag)
        self.assertIs(changed.ui_metadata['diagnostic_records'],ui_diag)

    def test_serialization_rejects_non_mapping_metadata_even_with_valid_pairs(self):
        bundle=self.build()
        for field in ('anchor_metadata','ui_metadata'):
            for value in (None,list(getattr(bundle,field).items())):
                with self.subTest(field=field,value_type=type(value).__name__),self.assertRaises((ValueError,TypeError)):
                    rebind.serialize_candidate(dataclasses.replace(bundle,**{field:value}))

    def test_serialization_still_rejects_changed_source_flags_and_observer_requirement(self):
        bundle=self.build()
        for source,key,value in (('anchor','pid',8),('anchor','read_only',False),
                ('ui','pid',8),('ui','read_only',False),('ui','accepted',False),
                ('ui','sha256','b'*64),('ui','crosshair_assignment_observer_enabled',True),
                ('ui','crosshair_assignment_observer_enabled',1)):
            field=source+'_metadata';metadata=rebind.thaw(getattr(bundle,field));metadata[key]=value
            changed=dataclasses.replace(bundle,**{field:rebind.freeze(metadata)})
            with self.subTest(source=source,key=key),self.assertRaises(ValueError):rebind.serialize_candidate(changed)

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

    def proof(self,**changes):
        values=dict(pid=7,game_creation=70,owner=dict(self.owner),dll=pathlib.Path('unused.dll').resolve(),
                    module_base=self.base,reverify=mock.Mock(),read_entry=mock.Mock(return_value=next(iter(self.signatures.values()))))
        values.update(changes)
        return types.SimpleNamespace(**values)

    def test_attested_acquisition_uses_only_bound_expectations_without_discovery(self):
        proof=self.proof()
        candidate=self.build(attestation=proof)
        self.resolve.assert_not_called()
        self.assertEqual(self.acquire.call_args.args[2],
                         dict(local_player_address=hex(self.local['address']),module_base=hex(self.base)))
        self.assertEqual(rebind.thaw(candidate.current_camera),self.camera)
        self.assertEqual(proof.reverify.call_count,2)
        self.acquire.reset_mock()
        self.build()
        self.resolve.assert_called_once_with(7,{'sha256':self.digest})
        self.assertEqual(self.acquire.call_args.args[2],self.resolve.return_value)

    def test_attested_binding_and_generation_reject_before_acquisition(self):
        for changes in (dict(pid=9),dict(game_creation=71),dict(owner=dict(self.owner,creation_time=81)),
                        dict(dll=pathlib.Path('other.dll').resolve()),dict(module_base=0)):
            with self.subTest(changes=changes):
                proof=self.proof(**changes);self.acquire.reset_mock()
                with self.assertRaises(ValueError):self.build(attestation=proof)
                self.acquire.assert_not_called()
        for generation in (0,True,(1<<64)-1):
            proof=self.proof();self.acquire.reset_mock()
            with self.assertRaises(ValueError):self.build(attestation=proof,expected_generation=generation)
            proof.reverify.assert_not_called();self.acquire.assert_not_called()
        self.resolve.assert_not_called()

    def test_attested_stale_full_local_identity_and_module_reject_after_acquisition(self):
        for field,value in (('cls',0x9001),('outer',0x8001),('number',1),('internal_index',2),('name','ReusedLocal')):
            with self.subTest(field=field):
                with self.assertRaisesRegex(ValueError,'Stable local identity changed'):
                    self.build(attestation=self.proof(),previous_local_identity=dict(self.local,**{field:value}))
        with self.assertRaisesRegex(ValueError,'Attestation camera module mismatch'):
            self.build(attestation=self.proof(module_base=self.base+0x1000))
        self.resolve.assert_not_called()

    @contextlib.contextmanager
    def fresh_attested_acquisition(self,*,retry=False,change=None):
        import anchor_config as anchor
        import ui_observer_config as ui
        readers=[];built=[]
        def resolver(pid,profile):
            camera=copy.deepcopy(self.camera)
            address=0x5000+len(readers)*0x1000
            camera.update(camera_address=hex(address),timestamp_address=hex(address+0x10),pov_address=hex(address+0x20))
            if change:camera.update(change)
            reader=types.SimpleNamespace(base=int(camera['module_base'],0),p=mock.Mock(),
                                         resolve=mock.Mock(return_value=camera),read=mock.Mock())
            readers.append(reader);return reader
        def build_anchor(reader,camera,*,prepared_camera,cinematic_camera):
            self.assertIs(camera,prepared_camera);self.assertTrue(cinematic_camera)
            metadata=copy.deepcopy(self.anchor)
            camera_id=metadata['config']['identities'][2]
            camera_id['address']=int(camera['camera_address'],0)
            metadata['certificate_final_guard']['camera']=copy.deepcopy(camera_id)
            metadata['sampler']['resolver']=copy.deepcopy(camera)
            built.append((reader,metadata))
            return metadata['config'],metadata
        def build_ui(reader,camera,**kwargs):
            self.assertIs(reader,built[-1][0]);self.assertIs(camera,reader.resolve.return_value)
            if retry and len(readers)==1:raise ui.UIRegistryChanged('Registry header changed')
            metadata=copy.deepcopy(self.ui)
            metadata['config']['identities'][1]=copy.deepcopy(built[-1][1]['config']['identities'][2])
            return metadata['config'],metadata
        with contextlib.ExitStack() as stack:
            self.acquire.side_effect=start.acquire_observer_preparation
            stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
            stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=build_anchor))
            stack.enter_context(mock.patch.object(anchor,'anchor_ini',side_effect=self.anchor_text))
            stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',side_effect=build_ui))
            stack.enter_context(mock.patch.object(ui,'ui_observer_ini',side_effect=self.ui_text))
            stack.enter_context(mock.patch.object(start.time,'sleep'))
            stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
            yield readers,built

    def test_attested_retry_resolves_again_and_returns_only_current_camera_certificates(self):
        proof=self.proof()
        with self.fresh_attested_acquisition(retry=True) as (readers,built):
            candidate=self.build(attestation=proof)
        self.resolve.assert_not_called();self.assertEqual(len(readers),2)
        for reader in readers:reader.resolve.assert_called_once();reader.p.close.assert_called_once()
        self.assertEqual(len(built),2);self.assertEqual(proof.read_entry.call_count,2)
        self.assertEqual(proof.reverify.call_count,2)
        self.assertEqual(candidate.current_camera['camera_address'],'0x6000')
        self.assertEqual(candidate.anchor_metadata['sampler']['resolver']['camera_address'],'0x6000')
        self.assertEqual(candidate.ui_metadata['config']['identities'][1]['address'],0x6000)

    def test_attested_fresh_address_and_module_failures_are_hard(self):
        for change,message in ((dict(local_player_address='0x999'),'Local player changed'),
                               (dict(module_base=hex(self.base+0x1000)),'Module base changed')):
            with self.subTest(change=change):
                proof=self.proof()
                with self.fresh_attested_acquisition(change=change) as (readers,built):
                    with self.assertRaisesRegex(RuntimeError,message):self.build(attestation=proof)
                self.assertEqual(len(readers),1);self.assertFalse(built)
                readers[0].resolve.assert_called_once();readers[0].p.close.assert_called_once()
                proof.read_entry.assert_not_called();self.assertEqual(proof.reverify.call_count,1)
        self.resolve.assert_not_called()

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
