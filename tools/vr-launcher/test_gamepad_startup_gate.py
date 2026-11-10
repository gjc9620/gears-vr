"""Exercise witnessed startup transaction retry, without threads or a game."""
import json
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock,patch
import playground_loader as p

class StartupGateTests(unittest.TestCase):
    identity=dict(pid=17,creation_time=29,image='owned-game')
    def race(self,**changes):
        witness=dict(stage='OpenThread',pid=17,tid=45,winerror=87,enumerated=[44,45],
                     already_suspended=[44],fresh_threads=[44,46],failed_tid_still_present=False)
        witness.update(changes);error=OSError('Thread disappeared');error.winerror=87
        error.add_note(json.dumps(witness));return error
    @patch.object(p.time,'sleep')
    def test_success_restarts_whole_change_and_preserves_first_failure(self,sleep):
        change=Mock(side_effect=[self.race(),dict(identity=self.identity,state='original'),dict(state='enabled')])
        result,attempts=p._enable_gamepad_gate(self.identity,change,lambda _:self.identity,lambda:None)
        self.assertEqual((result['state'],attempts),('enabled',2))
        self.assertEqual([c.args[1] for c in change.call_args_list],['enable','status','enable'])
        self.assertEqual(result['startup_thread_exit_retries'][0]['witness']['tid'],45)
        sleep.assert_called_once_with(.05)
    @patch.object(p.time,'sleep')
    def test_third_churn_refused_with_history(self,sleep):
        change=Mock(side_effect=[self.race(),dict(identity=self.identity,state='original'),self.race(),
                                 dict(identity=self.identity,state='original'),self.race()])
        with self.assertRaises(OSError) as caught:p._enable_gamepad_gate(self.identity,change,lambda _:self.identity,lambda:None)
        self.assertEqual(change.call_count,5)
        self.assertTrue(any('retry_history' in n for n in caught.exception.__notes__))
    @patch.object(p.time,'sleep')
    def test_unwitnessed_other_or_still_live_thread_never_retried(self,sleep):
        plain=OSError('unknown');plain.winerror=87
        for error in (plain,OSError('denied'),RuntimeError('Failed to resume game thread'),
                      self.race(stage='GetThreadContext'),self.race(pid=18),self.race(tid=0),
                      self.race(failed_tid_still_present=True),self.race(fresh_threads=[44,45]),
                      self.race(enumerated=[44]),self.race(fresh_snapshot_error='failed')):
            change=Mock(side_effect=error)
            with self.assertRaises(type(error)):p._enable_gamepad_gate(self.identity,change,lambda _:self.identity,lambda:None)
            self.assertEqual(change.call_count,1)
        sleep.assert_not_called()
    @patch.object(p.time,'sleep')
    def test_identity_change_and_invalid_code_state_never_reenable(self,sleep):
        changed=dict(self.identity,creation_time=30)
        for status in (dict(identity=self.identity,state='pending'),dict(identity=self.identity,state='enabled'),
                       dict(identity=changed,state='original')):
            change=Mock(side_effect=[self.race(),status])
            with self.assertRaises(RuntimeError):p._enable_gamepad_gate(self.identity,change,lambda _:self.identity,lambda:None)
            self.assertEqual(change.call_count,2)
        change=Mock(side_effect=self.race());identity=Mock(side_effect=[self.identity,changed])
        with self.assertRaises(RuntimeError):p._enable_gamepad_gate(self.identity,change,identity,lambda:None)
        self.assertEqual(change.call_count,1)
    def test_live_guard_cancellation_prevents_any_change(self):
        change=Mock();live=Mock(side_effect=RuntimeError('startup cancelled'))
        with self.assertRaisesRegex(RuntimeError,'cancelled'):p._enable_gamepad_gate(self.identity,change,lambda _:self.identity,live)
        change.assert_not_called()
    @patch.object(p.time,'sleep')
    def test_start_gamepad_does_not_restart_native_or_retake_lease_on_retry(self,sleep):
        bridge=Mock();bridge.control.identity.return_value=self.identity
        bridge.allocate.return_value=4096;bridge.export.return_value=0
        background=SimpleNamespace(change=Mock(side_effect=[dict(state='original'),self.race(),
            dict(identity=self.identity,state='original'),dict(state='enabled')]))
        runtime=SimpleNamespace(Bridge=Mock(return_value=bridge))
        manifest=dict(native_build='tools/native-capture/private-fixture',native_dll='Native.dll',native_sha256='fixture')
        with patch.dict(sys.modules,{'runtime':runtime,'background_input':background}):
            result=p.start_gamepad(manifest,self.identity,dict(pid=9,creation_time=10),simulator_operator=True)
        self.assertEqual(result['focus_gate_attempts'],2)
        self.assertEqual([c.args[0] for c in bridge.export.call_args_list],
                         ['GearsCaptureStartVrGamepad','GearsCaptureVrGamepadGateLease'])
        bridge.free.assert_called_once_with(4096);bridge.close.assert_called_once()

if __name__=='__main__':unittest.main()
