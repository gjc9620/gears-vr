import time,unittest
from types import SimpleNamespace
from unittest.mock import Mock
from pathlib import Path
import steam_game_start as s

class StartTest(unittest.TestCase):
    def setUp(self):
        self.game=Path('J:/game/GOWDE-Steam.exe').resolve()
        self.steam=dict(pid=10,creation_time=20,image=str(Path('J:/Steam/steam.exe').resolve()))
        self.fresh=dict(pid=30,creation_time=100,image=str(self.game))
        self.row=dict(pid=30,parent=10,name='GOWDE-Steam.exe')
        self.ident=lambda pid:{10:self.steam,30:self.fresh}.get(pid)
    def test_exact_fresh_child(self):
        self.assertEqual(s.selected_game([self.row],self.game,self.steam,self.ident,90),self.fresh)
        for rows,minimum in [([self.row],110),([dict(self.row,parent=11)],90),([self.row,self.row],90)]:
            with self.assertRaises(RuntimeError):s.selected_game(rows,self.game,self.steam,self.ident,minimum)
        with self.assertRaises(RuntimeError):s.selected_game([self.row],self.game,self.steam,lambda p:self.fresh if p==30 else dict(self.steam,creation_time=21),90)
    def test_existing_game_never_adopted_or_launched(self):
        run=Mock()
        with self.assertRaises(RuntimeError):s.launch(self.game,[],steam=self.steam,control=SimpleNamespace(identity=self.ident),enumerate_processes=lambda:[self.row],popen=run)
        run.assert_not_called()
    def test_one_steam_request_no_direct_fallback(self):
        self.fresh['creation_time']=time.time_ns()//100+116444736000000000+100000000
        snapshots=iter([[],[],[self.row]]);run=Mock(return_value=SimpleNamespace(pid=25))
        result=s.launch(self.game,['-NoPauseUnfocus'],steam=self.steam,control=SimpleNamespace(identity=self.ident),enumerate_processes=lambda:next(snapshots),popen=run,sleep=lambda _:None)
        self.assertEqual(result['pid'],30);self.assertTrue(result['steam_authenticated_launch'])
        self.assertEqual(run.call_count,1);self.assertEqual(run.call_args.args[0],[self.steam['image'],'-applaunch','2523720','-NoPauseUnfocus'])
    def test_timeout_never_direct_launches(self):
        ticks=iter([0,0,31]);run=Mock(return_value=SimpleNamespace(pid=25))
        with self.assertRaisesRegex(RuntimeError,'no direct-EXE fallback'):
            s.launch(self.game,[],steam=self.steam,control=SimpleNamespace(identity=self.ident),enumerate_processes=lambda:[],popen=run,clock=lambda:next(ticks),sleep=lambda _:None)
        self.assertEqual(run.call_count,1)

if __name__=='__main__':unittest.main()
