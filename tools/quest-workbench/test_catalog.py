"""Independent serialized-surface, catalog coverage and label checks. No device/game."""
from collections import Counter
from pathlib import Path
import hashlib,json,struct,unittest

ASSETS=Path(__file__).resolve().parent/'assets'
def geometry(path):
    raw=path.read_bytes();size=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+size]);start=28+size
    prim=j['meshes'][0]['primitives'][0]
    def array(index,fmt,width):
        a=j['accessors'][index];view=j['bufferViews'][a['bufferView']];at=start+view.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from(fmt,raw,at+i*width) for i in range(a['count'])]
    p=array(prim['attributes']['POSITION'],'<3f',12);ix=array(prim['indices'],'<H',2)
    return Counter(tuple(p[ix[i+k][0]] for k in range(3)) for i in range(0,len(ix),3))

class CatalogTests(unittest.TestCase):
    def setUp(self):self.rows=json.loads((ASSETS/'weapon-catalog.json').read_text('utf-8'))['weapons']
    def test_twelve_models_and_troika_all_bones(self):
        self.assertEqual(len(self.rows),12);self.assertEqual(len({r['model'] for r in self.rows}),12)
        row=next(r for r in self.rows if r['id']=='Troika_turret');self.assertEqual(len(row['bones']),13)
        for b in ['b_troika_handle_L','b_troika_handle_R','b_troika_spinner_L','b_troika_spinner_R']:
            self.assertIn(b,row['bones'])
        self.assertEqual(hashlib.sha256((ASSETS/'Lancer.glb').read_bytes()).hexdigest(),'8fa26e2b63d920de078011c08ad4da7b77ac4cd3c2a44ce08d8f450e63bbaec6')
    def test_split_models_preserve_every_original_triangle(self):
        for row in self.rows:
            if row['id']=='Lancer':continue
            with self.subTest(model=row['model']):
                full=geometry(ASSETS/row['model']);info=json.loads((ASSETS/(row['id']+'.parts.json')).read_text('utf-8'))
                combined=Counter()
                for group in info['groups']:
                    if group['mesh']:combined.update(geometry(ASSETS/group['mesh']))
                if info['rest_mesh']:combined.update(geometry(ASSETS/info['rest_mesh']))
                self.assertEqual(combined,full);self.assertEqual(sum(full.values()),row['triangles'])
                self.assertEqual([g['bone'] for g in info['groups']],row['bones'])
                self.assertTrue(info['curated']);self.assertEqual(len(info['parts']),row['parts']);self.assertLessEqual(len(info['parts']),4)
                self.assertEqual(info['parts'][0]['role'],'main')
                self.assertEqual(len({p['id'] for p in info['parts']}),len(info['parts']))
                self.assertIn(info['practice']['kind'],['magazine','pistol','lever','bolt','revolver','breech','bow','beam','turret','frag','smoke'])
                if info['practice']['ammo_mesh']:self.assertTrue((ASSETS/info['practice']['ammo_mesh']).is_file())
                for i,g in enumerate(info['groups']):self.assertTrue(g['parent']<i or i==0)
    def test_only_interactive_parts_and_ammo_entry(self):
        for stem in ['Boltock','COG_Pistol']:
            j=json.loads((ASSETS/(stem+'.parts.json')).read_text('utf-8'))
            visible=[p for p in j['parts'] if p.get('editor_visible',True)]
            self.assertEqual([p['id'] for p in visible],['main_grip','support','ammo','mechanism'])
            self.assertEqual(next(p['label'] for p in visible if p['id']=='ammo'),'装填弹巢' if stem=='Boltock' else '子弹')
            self.assertTrue(j['practice']['shared_grip']);self.assertEqual(j['parts'][1]['label'],'副手握持');self.assertNotIn('pose_alias',j['parts'][1])
            self.assertEqual(j['parts'][0]['position'],j['parts'][1]['position'])
        j=json.loads((ASSETS/'Boltock.parts.json').read_text('utf-8'));self.assertFalse(j['practice']['fastloader']);self.assertFalse(j['practice']['group_round_loading']);self.assertEqual(j['practice']['reload_item'],'cylinder');self.assertEqual(j['practice']['ammo_mesh'],'Boltock.reload-cylinder.mesh')
        j=json.loads((ASSETS/'Gnasher.parts.json').read_text('utf-8'));self.assertFalse(j['parts'][3]['editor_visible'])
    def test_ammunition_scale_is_metres(self):
        for name,lo,hi in [('boomshot-carrier.mesh',.10,.50),('bow-arrow-preview.mesh',.20,.50),('boltok-round.mesh',.04,.10),('Boltock.reload-cylinder.mesh',.06,.09),('gnasher-round.mesh',.07,.15),('longshot-round.mesh',.10,.20)]:
            raw=(ASSETS/name).read_bytes();size=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+size]);a=j['accessors'][0];extent=max(b-a for a,b in zip(a['min'],a['max']));self.assertGreater(extent,lo,name);self.assertLess(extent,hi,name)
    def test_reload_cylinder_matches_mounted_cylinder(self):
        def positions(name):
            raw=(ASSETS/name).read_bytes();n=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+n]);a=j['accessors'][0];b=j['bufferViews'][a['bufferView']];start=28+n+b.get('byteOffset',0)+a.get('byteOffset',0)
            return [struct.unpack_from('<3f',raw,start+i*12) for i in range(a['count'])]
        mounted=positions('Boltock.open-cylinder.mesh');item=positions('Boltock.reload-cylinder.mesh');feed=json.loads((ASSETS/'Boltock.parts.json').read_text('utf-8'))['practice']['feed'];self.assertEqual(len(mounted),len(item))
        for a,b in zip(mounted,item):
            for i in range(3):self.assertAlmostEqual(a[i],b[i]+feed[i],places=6)
    def test_catalog_chinese_labels_are_in_actual_atlas(self):
        glyphs=json.loads((ASSETS/'font.json').read_text('utf-8'))['glyphs']
        labels=''.join(r['title'] for r in self.rows)
        for row in self.rows:
            if row['id']!='Lancer':labels+=''.join(p['label'] for p in json.loads((ASSETS/(row['id']+'.parts.json')).read_text('utf-8'))['parts'])
        for c in set(labels):self.assertIn(str(ord(c)),glyphs,c)

if __name__=='__main__':unittest.main()
