"""Ownership rejection checks; no game, process access, XR or bridge calls."""
import json,sys,tempfile,types,unittest
from pathlib import Path
from unittest.mock import patch
import gnasher_sequence_probe as probe

class ReachedBridge(Exception):pass
class LauncherOwnership(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        self.dll=self.root/'reference.dll';self.session_path=self.root/'session.json';self.calls=0
        self.identity={'pid':123,'creation_time':456,'image':'owned-game.exe'}
        self.args=types.SimpleNamespace(runtime_root=probe.ROOT,launcher_session=self.session_path,owned_json=None,
            dll=self.dll,dll_sha256='a'*64,game_pid=123,game_creation_time=456)
        self.session={'phase':'session_owned','test_audio_guard':{'enabled':True},'owner_identity':{'pid':789,'creation_time':1011},
            'game_identity':self.identity,'playground':{'pid':123,'enabled':True,'dll':str(self.dll),'sha256':'a'*64}}
    def run_case(self,accepted):
        self.session_path.write_text(json.dumps(self.session),encoding='utf-8')
        def bridge(*args,**kwargs):
            self.calls+=1;self.assertTrue(kwargs['attach_only']);self.assertEqual(args[0],self.identity);raise ReachedBridge()
        with patch.dict(sys.modules,{'runtime':types.SimpleNamespace(Bridge=bridge)}):
            with self.assertRaises(ReachedBridge if accepted else ValueError):probe.sample_owned_game(self.args,{})
        self.assertEqual(self.calls,1 if accepted else 0)
    def test_valid_exact_owner(self):self.run_case(True)
    def test_wrong_pid(self):self.args.game_pid=124;self.run_case(False)
    def test_recycled_pid(self):self.args.game_creation_time=457;self.run_case(False)
    def test_wrong_dll(self):self.session['playground']['dll']=str(self.root/'other.dll');self.run_case(False)
    def test_wrong_hash(self):self.session['playground']['sha256']='b'*64;self.run_case(False)
    def test_unmuted(self):self.session['test_audio_guard']['enabled']=False;self.run_case(False)
    def test_unowned_phase(self):self.session['phase']='starting';self.run_case(False)
    def test_wrong_playground_pid(self):self.session['playground']['pid']=124;self.run_case(False)
    def test_disabled_playground(self):self.session['playground']['enabled']=False;self.run_case(False)
    def test_running_report_must_match_game(self):
        p=self.root/'report.json';p.write_text(json.dumps({'phase':'running','game_identity':dict(self.identity,creation_time=457)}))
        self.session.update(phase='waiting_for_level',session_report=str(p));self.run_case(False)
    def test_matching_running_report(self):
        p=self.root/'report.json';p.write_text(json.dumps({'phase':'running','game_identity':self.identity}))
        self.session.update(phase='waiting_for_level',session_report=str(p));self.run_case(True)

class ReferenceBridgeLifetime(unittest.TestCase):
    def bridge(self,code=0,throws=False):
        calls=[]
        def export(name):
            calls.append(name)
            if throws:raise RuntimeError('injected export failure')
            return code
        return types.SimpleNamespace(export=export,close=lambda:calls.append('close')),calls
    def test_launcher_parent_is_not_shutdown(self):
        bridge,calls=self.bridge();receipt={'passed':False}
        probe.close_reference_bridge(bridge,receipt,True)
        self.assertEqual(calls,['close'])
        self.assertTrue(receipt['shutdown_deferred'])
        self.assertFalse(receipt['launcher_cleanup_verified'])
        self.assertFalse(receipt['passed']);self.assertNotIn('shutdown',receipt)
    def test_probe_owned_shutdown_once(self):
        bridge,calls=self.bridge();receipt={};probe.close_reference_bridge(bridge,receipt,False)
        self.assertEqual(calls,['PlaygroundShutdown','close']);self.assertEqual(receipt['shutdown'],0)
    def test_probe_owned_rejection_still_closes_handle(self):
        bridge,calls=self.bridge(4)
        with self.assertRaisesRegex(RuntimeError,'not clean'):probe.close_reference_bridge(bridge,{},False)
        self.assertEqual(calls,['PlaygroundShutdown','close'])
    def test_export_exception_still_closes_handle(self):
        bridge,calls=self.bridge(throws=True)
        with self.assertRaisesRegex(RuntimeError,'injected'):probe.close_reference_bridge(bridge,{},False)
        self.assertEqual(calls,['PlaygroundShutdown','close'])
if __name__=='__main__':unittest.main()
