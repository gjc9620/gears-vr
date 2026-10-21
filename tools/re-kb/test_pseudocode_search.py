from contextlib import closing
import hashlib
import tempfile
from pathlib import Path
import unittest
import kb
import pseudocode_search as ps


class PseudocodeSearchTests(unittest.TestCase):
    def test_incremental_replacement_and_hash_rejection(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); path=root/'body.c'; database=root/'index.sqlite'
            with closing(kb.connect(database)) as db:
                with db:
                    aid=kb.artifact(db,{'sha256':'a'*64,'kind':'runtime_snapshot','module_base':'0x1000'})
                def put(text):
                    path.write_text(text,encoding='utf-8')
                    with db:
                        kb.import_function(db,aid,{'va':'0x1010','name':'function','status':'success',
                            'pseudocode_path':str(path)},root)
                put('void f() { CameraUnique(); }')
                with closing(ps.connect(database)) as fts:
                    self.assertEqual(ps.build(fts)['updated'],1)
                    self.assertEqual(ps.build(fts)['unchanged'],1)
                    self.assertEqual(ps.search(fts,'CameraUnique')[0]['va'],'0x1010')
                    put('void f() { RenderUnique(); }')
                    self.assertEqual(ps.build(fts)['updated'],1)
                    self.assertEqual(ps.search(fts,'CameraUnique'),[])
                    self.assertEqual(len(ps.search(fts,'RenderUnique')),1)
                    put('void f() { InputUnique(); }')
                    path.write_text('tampered',encoding='utf-8')
                    self.assertEqual(len(ps.build(fts)['errors']),1)
                    self.assertEqual(ps.search(fts,'RenderUnique'),[])


if __name__=='__main__':
    unittest.main()
