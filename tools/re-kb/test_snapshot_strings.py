import hashlib
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

import kb
from snapshot_strings import index


class SnapshotStringTests(unittest.TestCase):
    def test_provenance_search_idempotence_and_integrity(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            raw = b'\0HUD_route\0'+ 'Camera_Mode'.encode('utf-16-le')+b'\0\0'
            (root/'section.bin').write_bytes(raw)
            manifest = {'pid':123,'captured_utc':'historical','base':'0x140000000',
                'executable_sha256':'a'*64,'sections':[{'file':'section.bin','size':len(raw),
                'sha256':hashlib.sha256(raw).hexdigest(),'address':'0x140001000'}]}
            path = root/'manifest.json'
            path.write_text(json.dumps(manifest))
            database = root/'test.sqlite'
            first = index(database,path)
            self.assertEqual(first,index(database,path))
            with closing(kb.connect(database)) as db:
                results = kb.search(db,'Camera_Mode')['snapshot_literals']
                self.assertEqual(len(results),1)
                self.assertEqual(results[0]['encoding'],'utf-16-le')
                self.assertEqual(json.loads(results[0]['metadata'])['pid'],123)
                self.assertEqual(db.execute('SELECT count(*) FROM functions').fetchone()[0],0)
                count = db.execute('SELECT count(*) FROM snapshot_literals').fetchone()[0]
            (root/'section.bin').write_bytes(b'wrong')
            with self.assertRaises(ValueError):
                index(database,path)
            with closing(sqlite3.connect(database)) as db:
                self.assertEqual(db.execute('SELECT count(*) FROM snapshot_literals').fetchone()[0],count)


if __name__ == '__main__':
    unittest.main()
