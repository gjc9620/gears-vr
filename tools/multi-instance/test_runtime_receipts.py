"""Offline sharing-lock and exact-identity cleanup regression fixtures."""
import unittest
from contextlib import contextmanager
from unittest.mock import Mock
from runtime_receipts import read_json,GameReceipt,wait_launcher,require_disk,first_admission,ResourceUnavailable,confirmed_prelaunch_rejection
import tempfile
from pathlib import Path


class Receipts(unittest.TestCase):
    def test_no_birth_alone_is_unknown_but_owned_typed_rejection_can_finish(self):
        with tempfile.TemporaryDirectory() as folder:
            selected=Path(folder);owner={'pid':1,'creation_time':2,'image':'python.exe'}
            state={'owner_identity':owner,'reservation_failure_cleanup':{'status':'no_birth_receipt'}}
            self.assertFalse(confirmed_prelaunch_rejection(state,owner,selected))
            proof={'schema':1,'kind':'startup_prelaunch_rejection','stage':'python_discovery',
                   'game_creation_attempted':False,'injection_attempted':False,'prime_attempted':False,
                   'launch_receipt':str(selected/'case.birth.json')}
            state['reservation_failure_cleanup']={'status':'prelaunch_rejected','no_game_started':True,'proof':proof}
            self.assertTrue(confirmed_prelaunch_rejection(state,owner,selected))
            self.assertFalse(confirmed_prelaunch_rejection(state,dict(owner,creation_time=3),selected))
            (selected/'case.birth.json').write_text('{}')
            self.assertFalse(confirmed_prelaunch_rejection(state,owner,selected))

    def admission_fixture(self):
        ticks=[0.];locked=[False]
        @contextmanager
        def guard():
            self.assertFalse(locked[0]);locked[0]=True
            try:yield
            finally:locked[0]=False
        def sleep(seconds):
            self.assertFalse(locked[0],'Wait held the shared admission lock')
            ticks[0]+=seconds
        return guard,ticks,sleep

    def test_first_admission_capacity_race_waits_unlocked_and_commits_once(self):
        guard,ticks,sleep=self.admission_fixture();fresh=Mock();commit=Mock(return_value='worker')
        probe=Mock(side_effect=[ResourceUnavailable('full'),ResourceUnavailable('full'),{'memory':'fresh'}])
        result=first_admission(guard,fresh,probe,commit,clock=lambda:ticks[0],sleep=sleep)
        self.assertEqual(result,'worker');self.assertEqual(fresh.call_count,3)
        commit.assert_called_once_with({'memory':'fresh'})

    def test_first_admission_timeout_has_no_registration(self):
        guard,ticks,sleep=self.admission_fixture();commit=Mock()
        with self.assertRaises(ResourceUnavailable):
            first_admission(guard,Mock(),Mock(side_effect=ResourceUnavailable('full')),commit,
                            wait_seconds=2,clock=lambda:ticks[0],sleep=sleep)
        self.assertEqual(ticks[0],2);commit.assert_not_called()

    def test_registration_failure_never_retries_even_if_resource_error(self):
        guard,ticks,sleep=self.admission_fixture();commit=Mock(side_effect=ResourceUnavailable('after spawn'))
        with self.assertRaises(ResourceUnavailable):
            first_admission(guard,Mock(),Mock(return_value='ok'),commit,clock=lambda:ticks[0],sleep=sleep)
        commit.assert_called_once();self.assertEqual(ticks[0],0)

    def test_already_attempted_or_real_probe_failure_never_retries(self):
        for stage in ('fresh','probe'):
            guard,ticks,sleep=self.admission_fixture();fresh=Mock();probe=Mock();commit=Mock()
            (fresh if stage=='fresh' else probe).side_effect=RuntimeError('reject')
            with self.assertRaisesRegex(RuntimeError,'reject'):
                first_admission(guard,fresh,probe,commit,clock=lambda:ticks[0],sleep=sleep)
            commit.assert_not_called();self.assertEqual(ticks[0],0)

    def test_lock_contention_retries_only_before_admission(self):
        class Busy(RuntimeError):pass
        real,ticks,sleep=self.admission_fixture();guard=Mock(side_effect=[Busy('occupied'),real()]);commit=Mock(return_value='ok')
        self.assertEqual(first_admission(guard,Mock(),Mock(return_value=1),commit,busy_errors=(Busy,),clock=lambda:ticks[0],sleep=sleep),'ok')
        commit.assert_called_once_with(1)

    def test_admission_wait_bounds_and_zero_budget(self):
        for bad in (-1,601,float('nan'),True):
            with self.assertRaises(ValueError):first_admission(Mock(),Mock(),Mock(),Mock(),wait_seconds=bad)
        guard,ticks,sleep=self.admission_fixture();commit=Mock()
        with self.assertRaises(ResourceUnavailable):
            first_admission(guard,Mock(),Mock(side_effect=ResourceUnavailable('full')),commit,
                            wait_seconds=0,clock=lambda:ticks[0],sleep=sleep)
        commit.assert_not_called();self.assertEqual(ticks[0],0)

    def test_disk_full_or_below_reserve_rejected(self):
        for free in (0,1883758592,4*1024**3-1):
            with self.subTest(free=free),self.assertRaisesRegex(RuntimeError,'no new game started'):
                require_disk('private-volume',usage=lambda _:Mock(free=free))

    def test_disk_exact_reserve_admitted_and_volume_error_propagates(self):
        value=require_disk('private-volume',usage=lambda _:Mock(free=4*1024**3))
        self.assertTrue(value['admitted']);self.assertEqual(value['free_bytes'],4*1024**3)
        with self.assertRaises(OSError):
            require_disk('private-volume',usage=Mock(side_effect=OSError('unavailable')))

    def test_transient_sharing_lock_and_partial_json_retry(self):
        path=Mock();path.read_text.side_effect=[PermissionError(),'{', '{"ok":true}']
        sleep=Mock()
        self.assertEqual(read_json(path,sleep=sleep),{'ok':True})
        self.assertEqual(sleep.call_count,2)

    def test_unreadable_is_unknown_with_bounded_retries(self):
        path=Mock();path.read_text.side_effect=PermissionError()
        self.assertIsNone(read_json(path,sleep=Mock()))
        self.assertEqual(path.read_text.call_count,5)

    def fixture(self):
        selected=Mock();index=Mock();birth=Mock()
        selected.__truediv__=Mock(return_value=index)
        selected.glob.return_value=[birth]
        self.game={'pid':12,'creation_time':34,'image':'game.exe'}
        index.read_text.side_effect=PermissionError()
        birth.read_text.return_value='{"game_identity":{"pid":12,"creation_time":34,"image":"game.exe"}}'
        return selected,index,birth

    def test_birth_retry_and_cache_survive_later_unreadable_files(self):
        selected,index,birth=self.fixture();control=Mock();control.identity.return_value=self.game
        birth.read_text.side_effect=[PermissionError(),birth.read_text.return_value]
        receipt=GameReceipt()
        self.assertEqual(receipt.observe(selected,control),self.game)
        birth.read_text.side_effect=PermissionError()
        self.assertEqual(receipt.observe(selected,control),self.game)

    def test_first_read_after_game_exit_retains_exact_birth(self):
        selected,index,birth=self.fixture();control=Mock();control.identity.return_value=None
        self.assertEqual(GameReceipt().observe(selected,control),self.game)

    def test_missing_receipts_do_not_prove_exit(self):
        selected,index,birth=self.fixture();birth.read_text.side_effect=FileNotFoundError()
        self.assertIsNone(GameReceipt().observe(selected,Mock()))

    def test_normal_stop_waits_and_never_signals_reused_pid(self):
        child=Mock(pid=12);child.poll.side_effect=[None,None,0]
        control=Mock();identity={'pid':12,'creation_time':34}
        control.identity.side_effect=[identity,{'pid':12,'creation_time':35}]
        wait_launcher(child,control,identity,sleep=Mock())
        control.signal.assert_called_once_with('Local\\GearsVR.RepositoryLauncher.Stop.12')
        child.kill.assert_not_called();child.terminate.assert_not_called()


if __name__=='__main__':unittest.main()
