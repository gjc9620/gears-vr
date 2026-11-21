import unittest
from startup_status import confirmation


class StartupStatusTest(unittest.TestCase):
    def setUp(self):
        self.owner=dict(pid=1,creation_time=10,image='python.exe')
        self.game=dict(pid=2,creation_time=20,image='game.exe')
        self.xr=dict(pid=3,creation_time=30,image='xr.exe')
        self.live={x['pid']:x for x in (self.owner,self.game,self.xr)}
        self.state=dict(owner_identity=self.owner,game_identity=self.game)
        self.report=dict(game_identity=self.game,xr_identity=self.xr,
                         native_started=True,phase='running',
                         startup_timings=dict(first_stereo_submission=dict(elapsed_ms=100)))

    def confirmed(self):
        return confirmation(self.state,self.report,self.live.get)['startup_confirmed']

    def test_actual_startup(self):
        self.assertTrue(self.confirmed())

    def test_processes_without_stereo_are_not_ready(self):
        self.report['startup_timings']={}
        self.assertFalse(self.confirmed())

    def test_waiting_scene_is_not_ready(self):
        self.report['phase']='awaiting_level'
        self.assertFalse(self.confirmed())

    def test_reused_pid_is_not_ready(self):
        self.live[3]=dict(self.xr,creation_time=31)
        self.assertFalse(self.confirmed())

    def test_other_game_report_is_not_ready(self):
        self.report['game_identity']=dict(self.game,creation_time=21)
        self.assertFalse(self.confirmed())

    def test_exited_owner_is_not_ready(self):
        self.live.pop(1)
        self.assertFalse(self.confirmed())

    def test_missing_report_is_not_ready(self):
        self.assertFalse(confirmation(self.state,None,self.live.get)['startup_confirmed'])


if __name__=='__main__':
    unittest.main()
