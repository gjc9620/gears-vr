from contextlib import closing
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import kb
from snapshot_strings import index
from link_snapshot_data import link
import pseudocode_search as ps


class DataLinkTests(unittest.TestCase):
    def test_capture_identity_idempotence_and_tampering(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);database=root/'index.sqlite'
            code=b'\x90'*8; data=b'HUDLiteral\0'
            (root/'code.bin').write_bytes(code);(root/'data.bin').write_bytes(data)
            sections=[{'file':name,'size':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),
                'address':address,'executable':executable} for name,raw,address,executable in
                [('code.bin',code,'0x1000',True),('data.bin',data,'0x2000',False)]]
            manifest=root/'manifest.json';manifest.write_text(json.dumps({'base':'0x1000','pid':1,
                'captured_utc':'historical','executable_sha256':'f'*64,'sections':sections}))
            with closing(kb.connect(database)) as db:
                with db:
                    aid=kb.artifact(db,{'sha256':sections[0]['sha256'],'kind':'runtime_snapshot','module_base':'0x1000'})
                    kb.import_function(db,aid,{'va':'0x1000','name':'f','status':'skipped',
                        'xrefs':[{'from':'0x1000','to':'0x2000','type':'3','iscode':False}]},root)
            index(database,manifest)
            first=link(database,manifest)
            self.assertEqual(first['literal_start_links'],1)
            self.assertEqual(first,link(database,manifest))
            (root/'data.bin').write_bytes(b'corrupt')
            with self.assertRaises(ValueError):link(database,manifest)

    def test_script_fulltext_excludes_stale_records(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);database=root/'index.sqlite';source=root/'f.uc'
            source.write_text('function UniqueCameraTerm() {}')
            with closing(kb.connect(database)) as db:
                with db:
                    aid=kb.artifact(db,{'sha256':'b'*64,'kind':'unrealscript_package'})
                    db.execute('INSERT INTO script_entries VALUES(?,?,?,?,?,?,?)',
                        (aid,1,'Function\'Pkg.C.f\'',str(source),hashlib.sha256(source.read_bytes()).hexdigest(),0,
                         json.dumps({'exported':True})))
            with closing(ps.connect(database)) as db:
                self.assertEqual(ps.build_scripts(db)['updated'],1)
                self.assertEqual(ps.build_scripts(db)['unchanged'],1)
                self.assertEqual(len(ps.search_scripts(db,'UniqueCameraTerm')),1)
                with db:db.execute('UPDATE script_entries SET sha256=?',('c'*64,))
                self.assertEqual(ps.search_scripts(db,'UniqueCameraTerm'),[])


if __name__=='__main__':unittest.main()
