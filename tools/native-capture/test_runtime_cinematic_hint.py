"""Cinematic discovery locations retain full fresh proofs; no game opened."""
import copy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from test_cinematic_anchor import CinematicFixture
from test_mounted_integration import MountedUIFixture
import cinematic_anchor
from anchor_config import build_anchor_config,anchor_ini,AnchorRejected,LiveRegistryAnchorSampler
from world_rebind_runtime import RuntimePreparation,DiscoveryRegistryChanged


def cold_fixture(factory=CinematicFixture, noise=256):
    f=factory();f.pid=123;prefix=[]
    resolve=f.resolve
    f.resolve=lambda:dict(resolve(),pid=f.pid,module_base=hex(f.base))
    for index in range(noise):
        address=0x10000000+index*0x1000
        f.add(address,'UnselectedActor'+str(index),0x4000);prefix.append(address)
    prefix_set=set(prefix)
    f.object_pointers=prefix+[a for a in f.object_pointers if a not in prefix_set]
    f.ident_cache.clear()
    return f


def adapter(f):
    result=RuntimePreparation.__new__(RuntimePreparation)
    result.game={'pid':f.pid};result.profile=f.profile
    result._camera_hint=None;result._camera_roles=None
    result._cinematic_class_hint=f.CAMERA_CLASS
    return result


def certify(f, prepared):
    with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
        return build_anchor_config(f,prepared,prepared_camera=prepared,sample_pose=False,cinematic_camera=True)[0]


class CinematicHintTests(unittest.TestCase):
    def test_entire_human_and_mounted_wire_equal_with_less_discovery(self):
        for factory in (CinematicFixture,MountedUIFixture):
            rows=[]
            for enabled in (False,True):
                f=cold_fixture(factory);a=adapter(f);prepared=f.resolve()
                with mock.patch.object(f,'read',wraps=f.read) as reads:
                    if enabled:a._prime_cinematic_hint(f)
                    c=certify(f,prepared)
                rows.append((c,anchor_ini(c),reads.call_count))
            self.assertEqual(rows[0][:2],rows[1][:2]);self.assertLess(rows[1][2],rows[0][2]-200)

    def test_bad_or_unreadable_hint_falls_back_without_changing_certificate(self):
        for mode in ('wrong_name','unreadable'):
            f=cold_fixture();prepared=f.resolve();expected=certify(f,prepared)
            f=cold_fixture();a=adapter(f);prepared=f.resolve()
            if mode=='wrong_name':a._cinematic_class_hint=f.WORLD_CLASS
            original=f.identity;failed=False
            def identity(address):
                nonlocal failed
                if mode=='unreadable' and address==f.CAMERA_CLASS and not failed:
                    failed=True;raise OSError('Hint temporarily unreadable')
                return original(address)
            with mock.patch.object(f,'identity',side_effect=identity):
                a._prime_cinematic_hint(f);self.assertIsNone(a._cinematic_class_hint)
                self.assertEqual(certify(f,prepared),expected)

    def test_registry_change_is_not_hidden_by_hint_fallback(self):
        f=cold_fixture(noise=0);a=adapter(f)
        with mock.patch.object(f,'identity',side_effect=DiscoveryRegistryChanged('Registry changed')):
            with self.assertRaises(DiscoveryRegistryChanged):a._prime_cinematic_hint(f)

    def test_hint_cannot_authorize_changed_slot_name_metaclass_property_or_code(self):
        for mutation in ('slot','name_index','metaclass','property','code'):
            f=cold_fixture(noise=0);a=adapter(f);prepared=f.resolve();a._prime_cinematic_hint(f)
            index=f.u32(f.CAMERA_CLASS+8)
            if mutation=='slot':f.ptr(f.objects_data+index*8,0)
            if mutation=='name_index':f.word(f.u64(f.names_data+index*8),index+1)
            if mutation=='metaclass':f.ptr(f.CAMERA_CLASS+80,0x2000)
            if mutation=='property':f.word(f.PROPERTY+0x74,8)
            if mutation=='code':f.put(f.base+f.code[0][0],bytes([0]))
            with self.subTest(mutation=mutation),self.assertRaises(AnchorRejected):certify(f,prepared)

    def test_preparation_and_observation_prime_each_new_reader(self):
        f=cold_fixture();a=adapter(f);prepared=f.resolve();f.p=SimpleNamespace(close=mock.Mock())
        a._cinematic_class_hint=None
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)), \
             mock.patch('world_rebind_runtime.Resolver',return_value=f):
            first=a.observe(lambda:None)
            self.assertEqual(a._cinematic_class_hint,f.CAMERA_CLASS)
            f.ident_cache.clear()
            with mock.patch.object(f,'prepare',create=True):
                self.assertEqual(a._preparation_camera(f),prepared)
            self.assertIn(f.CAMERA_CLASS,f.ident_cache)
            f.ident_cache.clear()
            with mock.patch.object(f,'prepare',create=True):second=a.observe(lambda:None)
            self.assertEqual(first,second);self.assertIn(f.CAMERA_CLASS,f.ident_cache)
        self.assertEqual(f.p.close.call_count,2)

    def test_cold_preparation_primes_address_after_full_resolve(self):
        f=cold_fixture();a=adapter(f)
        with mock.patch.object(f,'identity',wraps=f.identity) as lookup:
            self.assertEqual(a._preparation_camera(f),f.resolve())
        self.assertIn(mock.call(f.CAMERA_CLASS),lookup.call_args_list)

    def test_retained_state_is_only_an_address_and_uses_exact_class_role(self):
        f=cold_fixture(noise=0);a=adapter(f)
        valid=dict(cinematic=dict(role_camera_actor_class=0),identities=[dict(
            address=f.CAMERA_CLASS,name='CameraActor',number=0)])
        a._remember_cinematic_hint(valid);self.assertEqual(a._cinematic_class_hint,f.CAMERA_CLASS)
        for change in ('wrong_name','number','negative','bool_role','missing'):
            c=copy.deepcopy(valid)
            if change=='wrong_name':c['identities'][0]['name']='WorldInfo'
            if change=='number':c['identities'][0]['number']=1
            if change=='negative':c['identities'][0]['address']=-1
            if change=='bool_role':c['cinematic']['role_camera_actor_class']=False
            if change=='missing':c['cinematic']={}
            a._remember_cinematic_hint(c);self.assertIsNone(a._cinematic_class_hint)

    def test_startup_constructor_retains_the_certified_class_location(self):
        f=cold_fixture(noise=0);c=certify(f,f.resolve())
        with tempfile.TemporaryDirectory() as directory:
            folder=Path(directory)
            (folder/'anchor-observer-config.json').write_text(json.dumps(dict(pid=f.pid,read_only=True,config=c)))
            (folder/'capture.ini').write_text('[Capture]\npid=123\nfamily_diagnostics=1\n',encoding='utf-16')
            a=RuntimePreparation(folder,'Capture.dll','a'*64,dict(pid=f.pid,creation_time=1),
                                 dict(pid=2,creation_time=3,image='owner.exe'))
            self.assertEqual(a._cinematic_class_hint,f.CAMERA_CLASS)

    def test_lookup_hint_avoids_unrelated_discovery_consuming_guard_budget(self):
        # A deterministic lookup-cost model, not a game timing measurement.
        for enabled in (False,True):
            f=cold_fixture();a=adapter(f);clock=[0];identity=f.identity
            def timed_identity(address):
                clock[0]+=1000000
                return identity(address)
            with mock.patch.object(f,'identity',side_effect=timed_identity), \
                 mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
                if enabled:a._prime_cinematic_hint(f)
                def acquire():return LiveRegistryAnchorSampler(f,lambda:clock[0],
                    prepared_camera=f.resolve(),cinematic_camera=True)
                if enabled:self.assertIsNotNone(acquire().initial)
                else:
                    with self.assertRaisesRegex(AnchorRejected,'deadline exceeded'):acquire()


if __name__=='__main__':unittest.main()
