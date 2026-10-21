import json
from pathlib import Path
import tempfile
import unittest
import kb


class KnowledgeBaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.db = kb.connect(self.root/'kb.sqlite')
        (self.root/'f.c').write_text('void HUD_Draw() {}',encoding='utf-8')
        self.header = dict(type='artifact',sha256='a'*64,kind='runtime_snapshot',module_base='0x1000',total_functions=10)
        self.function = dict(type='function',va='0x1100',rva='0x100',name='HUD_Draw',status='success',pseudocode_path='f.c',
            calls=[{'from':'0x1101','to':'0x1200','type':'call'}],
            strings=[{'address':'0x3000','value':'Subtitle'}])

    def tearDown(self):
        self.db.close()
        self.temp.cleanup()

    def export(self,*rows):
        path = self.root/'export.jsonl'
        path.write_text(''.join(json.dumps(r)+'\n' for r in rows),encoding='utf-8')
        return path

    def test_idempotent_incremental_edges_and_artifact_isolation(self):
        path = self.export(self.header,self.function)
        kb.import_jsonl(self.db,path); kb.import_jsonl(self.db,path)
        self.assertEqual(self.db.execute('SELECT count(*) FROM functions').fetchone()[0],1)
        self.assertEqual(self.db.execute('SELECT count(*) FROM edges').fetchone()[0],1)
        changed = dict(self.function,calls=[],strings=[],name='New_Draw')
        kb.import_jsonl(self.db,self.export(self.header,changed))
        self.assertEqual(self.db.execute('SELECT count(*) FROM edges').fetchone()[0],0)
        kb.import_jsonl(self.db,self.export(dict(self.header,kind='disk_exe'),self.function))
        kb.import_jsonl(self.db,self.export(dict(self.header,sha256='b'*64),self.function))
        kb.import_jsonl(self.db,self.export(dict(self.header,module_base='0x800'),dict(self.function,rva='0x900')))
        self.assertEqual(self.db.execute('SELECT count(*) FROM artifacts').fetchone()[0],4)
        self.assertEqual(len(kb.search(self.db,'Subtitle')['strings']),3)
        self.assertEqual(len(kb.search(self.db,'New_Draw',artifact_id=1)['functions']),1)

    def test_resume_failed_record_and_validate_provenance(self):
        bad = dict(self.function,va='0x1200',rva='0x200',pseudocode_sha256='b'*64)
        with self.assertRaisesRegex(ValueError,'SHA-256 mismatch'):
            kb.import_jsonl(self.db,self.export(self.header,self.function,bad))
        self.assertEqual(self.db.execute('SELECT count(*) FROM functions').fetchone()[0],1)
        bad = dict(self.function,rva='0x555')
        with self.assertRaisesRegex(ValueError,'mismatch'):
            kb.import_jsonl(self.db,self.export(self.header,bad))
        broken_edges = dict(self.function,name='should rollback',calls=[{'from':'oops','to':'0x100'}])
        with self.assertRaises(ValueError):
            kb.import_jsonl(self.db,self.export(self.header,broken_edges))
        self.assertEqual(self.db.execute('SELECT name FROM functions').fetchone()[0],'HUD_Draw')

    def test_truncated_tail_is_explicit(self):
        path = self.export(self.header,self.function)
        with path.open('a') as f: f.write('{"type":')
        with self.assertRaises(ValueError): kb.import_jsonl(self.db,path)
        result = kb.import_jsonl(self.db,path,True)
        self.assertEqual(result['truncated_final_line'],3)

    def test_failures_findings_and_unknown_tokens(self):
        failed = dict(type='function',va='0x1200',rva='0x200',name='Fail',status='failed',error='Hex-Rays failure at 0x1203')
        kb.import_jsonl(self.db,self.export(self.header,self.function,failed))
        finding = dict(artifact_id=1,function_va='0x1100',claim='candidate UI draw',confidence='low',
                       evidence=[{'path':'f.c','line':1}],validation={'runtime':'pending'})
        kb.add_finding(self.db,finding); kb.add_finding(self.db,finding)
        stat = kb.stats(self.db)
        self.assertEqual(stat['findings'],1)
        self.assertEqual(stat['artifacts'][0]['function_coverage'],0.2)
        self.assertEqual(stat['artifacts'][0]['decompilation_success_rate'],0.5)
        self.assertEqual(stat['artifacts'][0]['ai_deeply_analyzed_functions'],1)
        self.assertIsNone(stat['ai_tokens_known_sum'])
        with self.assertRaisesRegex(ValueError,'does not belong'):
            kb.add_finding(self.db,dict(finding,function_va='0x9999'))
        kb.add_finding(self.db,dict(finding,id='unbound',artifact_id=None,function_va=None))
        self.assertIsNone(self.db.execute("SELECT artifact_id FROM findings WHERE id='unbound'").fetchone()[0])

    def test_script_declarations_do_not_become_native_functions(self):
        manifest = {'packages':[{'source_sha256':'c'*64,'source':'Engine.u','name':'Engine','entries':[
            {'index':1,'path':'Engine.Canvas.Draw','file':'Canvas.uc','native_declaration':True},
            {'index':2,'path':'Engine.Canvas.Other','file':'Other.uc','native_declaration':False}]}]}
        path = self.root/'manifest.json'; path.write_text(json.dumps(manifest))
        kb.import_manifest(self.db,path); kb.import_manifest(self.db,path)
        s = kb.stats(self.db)['artifacts'][0]
        self.assertEqual(s['script_entries'],2)
        self.assertEqual(s['native_declarations'],1)
        self.assertEqual(s['native_functions_indexed'],0)
        with self.assertRaisesRegex(ValueError,'script declarations'):
            kb.import_function(self.db,1,self.function,self.root)


if __name__ == '__main__': unittest.main()
