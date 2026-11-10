"""Actual Windows temporary-file sharing regression, no game or input."""
from pathlib import Path
import json,tempfile,unittest
from shared_receipt_reader import open_shared_text,read_shared_json

class Tests(unittest.TestCase):
    def test_old_reader_reproduces_replace_denial_new_reader_allows_it(self):
        with tempfile.TemporaryDirectory() as root:
            p=Path(root)/'birth.json';tmp=p.with_suffix('.json.tmp')
            p.write_text('{"stage":0}');tmp.write_text('{"stage":1}')
            with p.open() as old:
                with self.assertRaises(PermissionError):tmp.replace(p)
                self.assertEqual(json.load(old),{'stage':0})
            with open_shared_text(p) as current:
                tmp.replace(p)
                self.assertEqual(json.load(current),{'stage':0})
                self.assertEqual(read_shared_json(p),{'stage':1})

    def test_held_reader_keeps_old_payload_through_multiple_atomic_publications(self):
        with tempfile.TemporaryDirectory() as root:
            p=Path(root)/'birth.json';tmp=p.with_suffix('.json.tmp');p.write_text('{"stage":0}')
            with open_shared_text(p) as old:
                for stage in range(1,11):
                    tmp.write_text(json.dumps({'stage':stage}));tmp.replace(p)
                    self.assertEqual(read_shared_json(p),{'stage':stage})
                self.assertEqual(json.load(old),{'stage':0})

    def test_missing_and_malformed_are_not_retried_or_fabricated(self):
        with tempfile.TemporaryDirectory() as root:
            p=Path(root)/'birth.json'
            with self.assertRaises(OSError):read_shared_json(p)
            p.write_text('{')
            with self.assertRaises(json.JSONDecodeError):read_shared_json(p)
            self.assertEqual(p.read_text(),'{')

    def test_bom_unicode_and_read_only_wrapper(self):
        with tempfile.TemporaryDirectory() as root:
            p=Path(root)/'出生回执.json';p.write_text('{"stage":1}',encoding='utf-8-sig')
            with open_shared_text(p) as f:
                with self.assertRaises(OSError):f.write('changed')
                self.assertEqual(json.load(f),{'stage':1})
            self.assertEqual(read_shared_json(p),{'stage':1})
            p.unlink() # Closed helper handles leave no delete-blocking lease.

if __name__=='__main__':unittest.main(verbosity=2)
