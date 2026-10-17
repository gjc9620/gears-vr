"""Offline position config contracts; synthetic identities and raw memory only."""
import copy
import configparser
import hashlib
import struct
import unittest
from unittest import mock

import path_choice_position_config as position
import path_choice_trial_config as owner
from test_path_choice_trial_config import fixture as owner_fixture

SESSION=dict(pid=123,creation_time=456,started_qpc=789,world_generation=2,nonce=1000,duration_ms=3000)


def fixture():
    snapshot=owner_fixture();extra=dict(identities=[],classes=[],name_entries=[])
    all_ids=snapshot['identities'];all_names=snapshot['name_entries']
    cls_ids={c['identity']['name']:c['identity'] for c in snapshot['classes']}
    def identity(name,cls,outer=0,number=0):
        index=len(all_ids)+len(extra['identities']);address=0x900000+index*0x200
        names={n['text']:n for n in all_names+extra['name_entries']}
        if name not in names:
            ni=len(all_names)+len(extra['name_entries']);entry=dict(index=ni,address=0xa00000+ni*0x100,text=name);extra['name_entries'].append(entry)
        else:ni=names[name]['index']
        obj=dict(address=address,internal_index=index,name_index=ni,name=name,number=number,cls=cls,outer=outer)
        extra['identities'].append(obj);return obj
    def klass(name,parent):
        obj=identity(name,cls_ids['Class']['address']);extra['classes'].append(dict(identity=obj,super=cls_ids[parent]['address']))
        cls_ids[name]=obj;return obj
    for name,parent in [('UIRoot','Object'),('UIScreenObject','UIRoot'),('UIObject','UIScreenObject'),
                        ('UIWarSafeRegionPanel','UIObject'),('ScriptStruct','Object'),('StructProperty','Object'),
                        ('FloatProperty','Object'),('ByteProperty','Object')]:klass(name,parent)
    scene=snapshot['roles']['scene'];panel=identity('UIWarSafeRegionPanel',cls_ids['UIWarSafeRegionPanel']['address'],scene['address'],1)
    structure=identity('UIScreenValue_Bounds',cls_ids['ScriptStruct']['address'],cls_ids['UIRoot']['address'])
    def reflected(name,kind,defining,offset,dim,size):
        prop=identity(name,cls_ids[kind]['address'],defining)
        return dict(property_address=hex(prop['address']),name=name,type=kind,owner=hex(defining),offset=offset,array_dim=dim,element_size=size)
    pos=reflected('Position','StructProperty',cls_ids['UIScreenObject']['address'],0x60,1,28)
    val=reflected('Value','FloatProperty',structure['address'],0,4,4)
    scale=reflected('ScaleType','ByteProperty',structure['address'],16,4,1)
    children_meta=reflected('Children','ArrayProperty',cls_ids['UIScreenObject']['address'],0x84,1,16)
    raw=struct.pack('<4f',.15305,.0374,.84695,.9626);values=list(struct.unpack('<4f',raw))
    row=dict(address=hex(panel['address']),name=panel['name'],number=1,cls='UIWarSafeRegionPanel',outer=hex(scene['address']),parent=hex(scene['address']),depth=2,
             render_bounds={'raw':'mutable'},layout_details=dict(face_order=['left','top','right','bottom'],position=dict(
                 reflected=pos,structure={k:v for k,v in structure.items() if k!='name_index'},members=dict(Value=dict(reflected=val,raw=raw.hex(),values=values),
                 ScaleType=dict(reflected=scale,raw='04040404',values=[4]*4)))))
    sample=dict(pid=123,read_only=True,consistent_reread=True,scene_owner=snapshot,client=hex(snapshot['roles']['client']['address']),
        top=dict(address=hex(scene['address']),cls='UIScene_PathChoice'),tree=[dict(address=hex(scene['address']),parent=None),row,
            dict(address='0xb00000',name='extraUIImage',cls='UIImage',parent=hex(scene['address']))])
    base=snapshot['module_base'];proof=dict(schema=1,kind='path_choice_position_panel_proof',pid=123,module_base=base,read_only=True,
        consistent_reread=True,owner_snapshot_sha256=position.owner_digest(snapshot),**extra,vtable=base+0x138a558,
        vslots={'0x3d8':base+0x2988d8,'0x3b8':base+0x297338},checks=[],
        children=dict(reflected=children_meta,data=0xb01000,count=2,capacity=4,members=[panel['address'],0xb00000]))
    merged=copy.deepcopy(snapshot)
    for key in extra:merged[key].extend(extra[key])
    ini=owner.build(merged,**SESSION);fields=dict(line.split('=',1) for line in ini.splitlines()[1:]);memory={}
    def put(address,raw):memory.update((address+i,b) for i,b in enumerate(raw))
    for i in range(int(fields['check_count'])):
        address,value=fields['check_'+str(i)].split(':');put(int(address,16),bytes.fromhex(value))
    for meta in (pos,val,scale,children_meta):
        address=int(meta['property_address'],16);put(address+112,struct.pack('<II',meta['array_dim'],meta['element_size']));put(address+148,struct.pack('<I',meta['offset']))
    put(int(pos['property_address'],16)+208,struct.pack('<Q',structure['address']))
    put(scene['address']+0x84,struct.pack('<Qii',0xb01000,2,4));put(0xb01000,struct.pack('<QQ',panel['address'],0xb00000))
    put(panel['address'],struct.pack('<Q',proof['vtable']));put(panel['address']+0x60,raw);put(panel['address']+0x70,b'\x04'*4)
    for slot,target in proof['vslots'].items():put(proof['vtable']+int(slot,16),struct.pack('<Q',target))
    reads=position.required_reads(sample,proof,**SESSION)
    proof['checks']=[dict(address=r['address'],raw=bytes(memory[r['address']+i] for i in range(r['size'])).hex()) for r in reads]
    return sample,proof


class PositionConfigTests(unittest.TestCase):
    def test_optional_vertical_offset_and_default_byte_compatibility(self):
        sample,proof=fixture();default=position.build(sample,proof,**SESSION)
        self.assertEqual(hashlib.sha256(default.encode('ascii')).hexdigest(),
                         '97e6da2238a281271c14c453f9310e4907736b47f3309e4fcb3031b116567652')
        self.assertEqual(default,position.build(sample,proof,**SESSION,vertical_offset_reference_pixels=16))
        self.assertNotIn('vertical_offset_reference_pixels=',default)
        original_reads=position.required_reads(sample,proof,**SESSION)
        for offset in (1,374,400):
            ini=position.build(sample,proof,**SESSION,vertical_offset_reference_pixels=offset)
            self.assertEqual(ini.replace('vertical_offset_reference_pixels=%d\n'%offset,''),default)
            parser=configparser.RawConfigParser();parser.read_string(ini);fields=dict(parser['PathChoicePositionTrial'])
            self.assertEqual(len(fields),17+int(fields['check_count']))
            self.assertEqual(position.required_reads(sample,proof,**SESSION,vertical_offset_reference_pixels=offset),original_reads)
        for invalid in (0,401,-1,16.0,True,'374'):
            with self.subTest(offset=invalid):
                with self.assertRaises(ValueError):position.build(sample,proof,**SESSION,vertical_offset_reference_pixels=invalid)
                with self.assertRaises(ValueError):position.required_reads(sample,proof,**SESSION,vertical_offset_reference_pixels=invalid)
    def test_serialization_exact_schema_minimal_scope_and_readonly(self):
        sample,proof=fixture();before=copy.deepcopy((sample,proof));ini=position.build(sample,proof,**SESSION)
        self.assertEqual((sample,proof),before);parser=configparser.RawConfigParser();parser.read_string(ini)
        fields=dict(parser['PathChoicePositionTrial']);self.assertEqual(len(fields),16+int(fields['check_count']))
        self.assertEqual(fields['schema'],'2');self.assertEqual(fields['registry_header_policy'],'fixed_prefix_v1')
        self.assertNotIn('shift_x',fields);self.assertEqual(fields['top_bits'],str(struct.unpack('<I',bytes.fromhex(sample['tree'][1]['layout_details']['position']['members']['Value']['raw'])[4:8])[0]))
        panel=int(fields['panel'],16);checks=[]
        for i in range(int(fields['check_count'])):
            address,raw=fields['check_'+str(i)].split(':');checks.append((int(address,16),bytes.fromhex(raw)))
        for address,raw in checks:
            self.assertFalse(any(address<at+size and at<address+len(raw) for at,size in ((panel+0x64,4),(panel+0x6c,4),(panel+0x74,4))))
            self.assertNotEqual(address,0xb00000)
        self.assertEqual(ini,position.build(sample,proof,**SESSION))

    def test_registry_headers_remain_exact_at_acquisition_boundary(self):
        sample,proof=fixture();base=sample['scene_owner']['module_base'];header=base+0x1c24730
        for entry in proof['checks']:
            raw=bytearray.fromhex(entry['raw'])
            if entry['address']<=header+8 and header+12<=entry['address']+len(raw):
                offset=header+8-entry['address'];num=struct.unpack_from('<i',raw,offset)[0]
                struct.pack_into('<i',raw,offset,num+1);entry['raw']=raw.hex();break
        else:self.fail('missing synthetic registry read')
        with self.assertRaisesRegex(ValueError,'fresh identity/owner/reflection check mismatch'):
            position.build(sample,proof,**SESSION)

    def test_only_whole_known_headers_receive_runtime_policy(self):
        base=0x140000000;headers=(base+0x1c24730,base+0x1c34800)
        raw=struct.pack('<Qii',0x700000,100,200)
        for bad in ('partial','cross','missing','invalid','alias'):
            with self.subTest(bad=bad):
                checks=owner._Checks();checks.add(headers[0],raw)
                if bad!='missing':checks.add(headers[1],struct.pack('<Qii',0x800000,100,200))
                if bad=='partial':checks.add(headers[0]+8,raw[8:])
                if bad=='cross':checks.add(headers[0]-8,b'\0'*8+raw)
                if bad=='invalid':checks.spans[headers[0],16]=struct.pack('<Qii',0x700000,0,200)
                if bad=='alias':checks.spans[headers[1],16]=raw
                with self.assertRaises(ValueError):position._fixed_prefix_headers(checks,base)

    def test_reject_bad_selection_units_reflection_and_vslots(self):
        for error in ('missing','multiple','parent','outer','units','position_offset','value_offset','scale_offset','vslot','unknown_key','missing_identity','bad_class','children'):
            with self.subTest(error=error):
                sample,proof=fixture();row=sample['tree'][1]
                if error=='missing':sample['tree'].pop(1)
                elif error=='multiple':sample['tree'].append(copy.deepcopy(row))
                elif error=='parent':row['parent']='0x9999'
                elif error=='outer':row['outer']='0x9999'
                elif error=='units':row['layout_details']['position']['members']['ScaleType']['raw']='05050505'
                elif error=='position_offset':row['layout_details']['position']['reflected']['offset']=100
                elif error=='value_offset':row['layout_details']['position']['members']['Value']['reflected']['offset']=4
                elif error=='scale_offset':row['layout_details']['position']['members']['ScaleType']['reflected']['offset']=17
                elif error=='vslot':proof['vslots']['0x3d8']+=1
                elif error=='unknown_key':proof['unbounded']=True
                elif error=='missing_identity':proof['identities'].pop()
                elif error=='bad_class':proof['identities'][3]['name']='other'
                elif error=='children':proof['children']['members'][0]=0xb00000
                with self.assertRaises(ValueError):position.build(sample,proof,**SESSION)

    def test_fresh_coverage_and_old_owner_checks_reject(self):
        for error in ('digest','owner_graph','bytes','missing','conflict','budget'):
            with self.subTest(error=error):
                sample,proof=fixture()
                if error=='digest':proof['owner_snapshot_sha256']='0'*64
                elif error=='owner_graph':sample['scene_owner']['links']['scene_client']['value']=1
                elif error=='bytes':
                    raw=bytearray.fromhex(proof['checks'][0]['raw']);raw[0]^=1;proof['checks'][0]['raw']=raw.hex()
                elif error=='missing':proof['checks'].pop(0)
                elif error=='conflict':proof['checks'].append(dict(address=proof['checks'][0]['address'],raw='ff'))
                elif error=='budget':proof['checks']*=20
                with self.assertRaises(ValueError):position.build(sample,proof,**SESSION)

    def test_unowned_classification_reuses_owner_contract(self):
        sample,proof=fixture();snapshot=sample['scene_owner'];snapshot['owner_classification']='unowned_single_local_player'
        snapshot['classification_basis']='null_owner_with_exact_single_player_graph';snapshot['links']['scene_player_owner']['value']=0
        proof['owner_snapshot_sha256']=position.owner_digest(snapshot)
        prop=next(p for p in snapshot['properties'] if p['identity']['name']=='PlayerOwner')
        address=snapshot['roles']['scene']['address']+prop['offset']
        for check in proof['checks']:
            raw=bytearray.fromhex(check['raw'])
            for i in range(len(raw)):
                if address<=check['address']+i<address+8:raw[i]=0
            check['raw']=raw.hex()
        self.assertIn('[PathChoicePositionTrial]',position.build(sample,proof,**SESSION))

if __name__=='__main__':unittest.main()
