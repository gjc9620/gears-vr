"""Mocked pre-activation acquisition: no native hooks, process or game opened."""
import contextlib
import io
import pathlib
import struct
import unittest
from unittest import mock
import start_capture as start
import ui_observer_config as ui
import anchor_config as anchor
from resolve_camera import DiscoveryRegistryChanged
from test_ui_observer_config import Fixture

class ReacquisitionTests(unittest.TestCase):
    def setUp(self):
        self.camera={'local_player_address':'0x100','module_base':'0x140000000','controller_address':'old','camera_address':'old'}
        self.resolvers=[]
        def factory(pid,profile):
            r=mock.Mock();r.base=0x140000000;r.cache={};r.generation=len(self.resolvers)+1
            r.resolve.return_value=dict(self.camera,controller_address=f'pc{r.generation}',camera_address=f'cam{r.generation}')
            r.read.return_value=b'bytes';self.resolvers.append(r);return r
        self.factory=factory
    def collect(self,builder,*,clock=None,anchor_builder=None):
        def prepared_anchor(r,c,*,prepared_camera=None):
            self.assertIs(prepared_camera,c)
            self.assertIs(prepared_camera,r.resolve.return_value)
            if anchor_builder is not None:return anchor_builder(r,c)
            return {'camera':c['camera_address']},{'camera':c['camera_address']}
        with mock.patch.object(start,'Resolver',side_effect=self.factory),mock.patch.object(ui,'build_ui_observer_config',side_effect=builder),\
             mock.patch.object(ui,'ui_observer_ini',side_effect=lambda c:'ui:'+c['camera']),\
             mock.patch.object(anchor,'build_anchor_config',side_effect=prepared_anchor) as anchored,\
             mock.patch.object(anchor,'anchor_ini',side_effect=lambda c:'anchor:'+c['camera']),\
             mock.patch.object(start.time,'monotonic',**({'side_effect':clock} if clock is not None else {'return_value':0})),\
             mock.patch.object(start.time,'sleep') as slept,contextlib.redirect_stderr(io.StringIO()):
            value=start.acquire_observer_preparation(1,{},self.camera,{1:b'bytes'},pathlib.Path('Unused.dll'),anchor_observer=True,ui_observer=True)
            return value,anchored.call_count,slept.call_count
    def test_reacquires_resolver_camera_anchor_and_ui_as_one_preparation(self):
        seen=[]
        def build(r,c):
            seen.append((r,c.copy(),r.cache.copy()));r.cache['old']='must not survive'
            if r.generation==1:raise ui.UIRegistryChanged('UI registry changed during assembly; reacquire')
            return {'camera':c['camera_address']},{'camera':c['camera_address']}
        (camera,am,asection,um,usection),anchors,sleeps=self.collect(build)
        self.assertEqual((camera['camera_address'],am['camera'],um['camera']),('cam2','cam2','cam2'))
        self.assertEqual((asection,usection),('anchor:cam2','ui:cam2'))
        self.assertEqual((anchors,sleeps,len(self.resolvers)),(2,1,2))
        self.assertTrue(all(not cache for _,_,cache in seen))
        for r in self.resolvers:r.resolve.assert_called_once();r.p.close.assert_called_once();r.read.assert_called_once()
    def test_only_typed_final_registry_change_retries(self):
        for error in (ui.UIConfigRejected('identity changed'),ui.UIConfigRejected('UI registry changed during assembly; reacquire'),RuntimeError('signature mismatch'),IndexError('tuple index out of range'),RuntimeError('Name index out of range')):
            with self.subTest(error=error):
                self.resolvers=[]
                with self.assertRaises(type(error)):self.collect(lambda r,c:(_ for _ in ()).throw(error))
                self.assertEqual(len(self.resolvers),1);self.resolvers[0].p.close.assert_called_once()

    def test_discovery_registry_change_reacquires_all_certificates(self):
        def build(r,c):
            if r.generation==1:raise DiscoveryRegistryChanged('Name registry changed during discovery')
            return {'camera':c['camera_address']},{'camera':c['camera_address']}
        (camera,am,_,um,_),anchors,sleeps=self.collect(build)
        self.assertEqual((camera['camera_address'],am['camera'],um['camera']),('cam2','cam2','cam2'))
        self.assertEqual((anchors,sleeps),(2,1))
        self.assertTrue(start.camera_preparation_transient(DiscoveryRegistryChanged('changed')))
        self.assertFalse(start.camera_preparation_transient(IndexError('tuple index out of range')))
        for r in self.resolvers:r.p.close.assert_called_once()

    def test_ui_discovery_propagates_only_typed_registry_change(self):
        r=Fixture()
        r.discovery_identity_reader=lambda:lambda address:(_ for _ in ()).throw(DiscoveryRegistryChanged('changed'))
        with self.assertRaises(DiscoveryRegistryChanged):ui.build_ui_observer_config(r,r.camera)
    def test_three_attempt_limit(self):
        with self.assertRaises(ui.UIRegistryChanged):
            self.collect(lambda r,c:(_ for _ in ()).throw(ui.UIRegistryChanged('changed')))
        self.assertEqual(len(self.resolvers),3)
        for r in self.resolvers:r.p.close.assert_called_once()
    def test_anchor_sampling_failure_never_retries(self):
        error=anchor.AnchorSamplingRejected('anchor rejected',{'accepted':False})
        with mock.patch.object(start,'save_anchor_sampling_failure') as saved:
            with self.assertRaises(anchor.AnchorSamplingRejected):
                self.collect(lambda r,c:self.fail('UI must not run'),
                             anchor_builder=lambda r,c:(_ for _ in ()).throw(error))
            saved.assert_called_once_with(pathlib.Path('Unused.dll'),1,error)
        self.assertEqual(len(self.resolvers),1);self.resolvers[0].p.close.assert_called_once()
    def test_deadline_prevents_retry_and_rejects_late_success(self):
        with self.assertRaises(ui.UIRegistryChanged):
            self.collect(lambda r,c:(_ for _ in ()).throw(ui.UIRegistryChanged('changed')),clock=[0,0,11])
        self.assertEqual(len(self.resolvers),1)
        self.resolvers=[]
        with self.assertRaisesRegex(ui.UIConfigRejected,'deadline'):
            self.collect(lambda r,c:({'camera':c['camera_address']},{}),clock=[0,0,11])
        self.assertEqual(len(self.resolvers),1)
    def test_local_player_or_signature_failure_never_retries(self):
        for changed in ('local','signature'):
            with self.subTest(changed=changed):
                self.resolvers=[];factory=self.factory
                def invalid(pid,profile):
                    r=factory(pid,profile)
                    if changed=='local':r.resolve.return_value['local_player_address']='0x999'
                    else:r.read.return_value=b'wrong'
                    return r
                self.factory=invalid
                try:
                    with self.assertRaises(RuntimeError):self.collect(lambda r,c:self.fail('UI must not run'))
                    self.assertEqual(len(self.resolvers),1);self.resolvers[0].p.close.assert_called_once()
                finally:self.factory=factory
    def test_real_final_registry_header_change_has_specific_type(self):
        r=Fixture();original=r.read;hits=0
        def read(address,size):
            nonlocal hits
            if address==r.objects_header and size==16:
                hits+=1
                if hits==5:r.put(address,struct.pack('<Qii',r.od,1102,1102))
            return original(address,size)
        r.read=read
        with self.assertRaisesRegex(ui.UIRegistryChanged,'registry changed during assembly'):
            ui.build_ui_observer_config(r,r.camera)

if __name__=='__main__':unittest.main()
