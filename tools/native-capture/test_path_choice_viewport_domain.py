"""Real bounded reflection ledger over synthetic memory; no process handles."""
import struct
import unittest
from unittest import mock
import path_choice_position_collect as collector
from scene_owner_snapshot import SceneOwnerRejected
from test_scene_owner_snapshot import Fixture


def fixture(dimensions=(3840.,2160.)):
    f=Fixture()
    for name in ('Package','StructProperty','FloatProperty','ScriptStruct'):
        address=f.obj(name,f.classes['Class']);f.classes[name]=address
        f.put(address+96,'<Q',f.classes['Object']);f.put(address+128,'<Q',0)
    package=f.obj('Engine',f.classes['Package'])
    f.put(f.classes['UIScene']+64,'<Q',package)
    vector=f.obj('ViewportVector',f.classes['ScriptStruct'],f.classes['Object'])
    f.classes['ViewportVector']=vector;f.put(vector+128,'<Q',0)
    prop=f.prop('UIScene','CurrentViewportSize','StructProperty',0x400)
    f.put(prop+208,'<Q',vector)
    for i,name in enumerate(('X','Y')):
        member=f.prop('ViewportVector',name,'FloatProperty',i*4)
        f.put(member+112,'<II',1,4)
    f.put(f.scene+0x400,'<2f',*dimensions);f.sync()
    # Resolver methods supply discovery only; _Capture authenticates every
    # selected identity, declaring field chain, type and actual source bytes.
    def property_hint(owner,name,inherited=False):
        current=owner
        for _ in range(64):
            key=next(key for key,address in f.classes.items() if address==current)
            if (key,name) in f.props:
                address=f.hint_overrides.get((current,name),f.props[key,name])
                cls=struct.unpack('<Q',f.read(address+80,8))[0]
                kind=next(key for key,at in f.classes.items() if at==cls)
                return dict(address=address,type=kind,offset=struct.unpack('<I',f.read(address+148,4))[0])
            if not inherited:break
            current=struct.unpack('<Q',f.read(current+96,8))[0]
            if not current:break
        raise RuntimeError('Missing synthetic property')
    f.property=mock.Mock(side_effect=property_hint)
    f.struct_type=lambda p:struct.unpack('<Q',f.read(p['address']+208,8))[0]
    scene=f.capture()['roles']['scene']
    sample=dict(top=dict(address=hex(f.scene)),scene_owner=dict(roles=dict(scene=scene)))
    return f,sample


class ViewportDomainTests(unittest.TestCase):
    def test_actual_reflection_and_storage_for_multiple_domains(self):
        for dimensions in ((1920.,1080.),(3840.,2160.),(1280.,1400.)):
            r,sample=fixture(dimensions);source=r.read
            result=collector._viewport_domain(r,sample)
            self.assertEqual(result['values_xy'],list(dimensions))
            self.assertEqual(result['raw'],struct.pack('<2f',*dimensions).hex())
            self.assertEqual(result['property']['owner']['name'],'UIScene')
            self.assertEqual(result['members']['Y']['offset'],4)
            self.assertEqual(result['value_address'],r.scene+0x400)
            self.assertTrue(result['consistent_reread']);self.assertEqual(r.read,source)
            self.assertIn(dict(address=r.scene+0x400,raw=result['raw']),result['source_checks'])

    def test_wrong_type_owner_extent_members_and_scene_are_rejected(self):
        for fault in ('type','package','extent','overlap','scene','value','struct'):
            r,sample=fixture();source=r.read;prop=r.props['UIScene','CurrentViewportSize']
            if fault=='type':r.put(prop+80,'<Q',r.classes['FloatProperty'])
            elif fault=='package':r.put(r.classes['UIScene']+64,'<Q',r.engine)
            elif fault=='extent':r.put(prop+112,'<II',2,8)
            elif fault=='overlap':r.put(r.props['ViewportVector','Y']+148,'<I',0)
            elif fault=='scene':sample['scene_owner']['roles']['scene']['address']+=8
            elif fault=='value':r.put(r.scene+0x400,'<2f',0,2160)
            else:r.struct_type=lambda p:r.classes['Object']
            with self.subTest(fault=fault),self.assertRaises((ValueError,SceneOwnerRejected)):
                collector._viewport_domain(r,sample)
            self.assertEqual(r.read,source)

    def test_registered_but_unreachable_discovery_hint_rejects(self):
        r,sample=fixture();original=r.props['UIScene','CurrentViewportSize']
        fake=r.obj('CurrentViewportSize',r.classes['StructProperty'],r.classes['UIScene'])
        r.put(fake+112,'<II',1,8);r.put(fake+148,'<I',0x400);r.put(fake+208,'<Q',r.classes['ViewportVector']);r.sync()
        r.hint_overrides[r.classes['UIScene'],'CurrentViewportSize']=fake
        self.assertNotEqual(fake,original)
        with self.assertRaisesRegex(ValueError,'unreachable'):collector._viewport_domain(r,sample)

    def test_value_and_metadata_changes_during_reread_reject(self):
        for field in ('value','metadata'):
            r,sample=fixture();source=r.read
            target=r.scene+0x400 if field=='value' else r.props['UIScene','CurrentViewportSize']+148
            r.hook=r.mutate_on_reread(target)
            with self.subTest(field=field),self.assertRaises((ValueError,SceneOwnerRejected)):
                collector._viewport_domain(r,sample)
            self.assertEqual(r.read,source)


if __name__=='__main__':unittest.main()
