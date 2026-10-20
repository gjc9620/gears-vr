"""Offline test-only audio lifecycle fixtures; no real audio volume mutation."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock
import game_audio_guard as audio


class GuardTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name); self.helper = self.root/'fixture.exe'; self.helper.write_bytes(b'fixture')
        self.log = self.root/'audio.jsonl'; self.events = mock.Mock(); self.events.create.return_value = 7
        self.game = dict(pid=12,creation_time=34); self.owner = dict(pid=56,creation_time=78)

    def guard(self, reason='stop', *, cleanup=True, timeout=False, ready=True):
        def spawn(command, **kwargs):
            self.command = command
            process = mock.Mock(); process.poll.return_value = None
            if ready:
                kwargs['stdout'].write((json.dumps(dict(event='guard_ready',**self.game))+'\n').encode())
                kwargs['stdout'].flush()
            def finish(timeout):
                if timeout_flag: raise subprocess.TimeoutExpired(command, timeout)
                if cleanup:
                    with self.log.open('a') as output:
                        output.write(json.dumps(dict(event='guard_cleanup',reason=reason,
                            matched=1,muted=1,restore_pending=0,restored=1))+'\n')
                return 0
            process.wait.side_effect = finish
            self.process = process
            return process
        timeout_flag = timeout
        ticks = [0.]
        return audio.Guard(self.game,self.owner,self.log,helper=self.helper,events=self.events,popen=spawn,
                           clock=lambda:ticks[0],sleep=lambda seconds:ticks.__setitem__(0,ticks[0]+seconds))

    def test_manual_opt_in_never_overrides_test_mute(self):
        with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME='1'),mock.patch.object(audio,'Guard') as guard:
            self.assertIsNone(audio.begin_manual(self.game,self.owner,self.log));guard.assert_not_called()
        with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME=''),mock.patch.object(audio,'verify_manual_helper') as verified,mock.patch.object(audio,'Guard') as guard:
            audio.begin_manual(self.game,self.owner,self.log)
            verified.assert_called_once_with(audio.MANUAL_HELPER)
            self.assertTrue(guard.call_args.kwargs['manual'])
            self.assertEqual(guard.call_args.kwargs['helper'],audio.MANUAL_HELPER)

    def test_packaged_test_mute_reuses_verified_helper_without_manual_mode(self):
        import hashlib
        receipt=self.helper.with_name('audio-helper-provenance.json')
        receipt.write_text(json.dumps(dict(schema=1,policy='manual_game_session_v1',helper=self.helper.name,
            helper_sha256=hashlib.sha256(self.helper.read_bytes()).hexdigest())))
        with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME='1'),mock.patch.object(audio,'MANUAL_HELPER',self.helper),mock.patch.object(audio,'Guard') as guard:
            audio.begin(self.game,self.owner,self.log)
            self.assertEqual(guard.call_args.kwargs,{'helper':self.helper})
            guard.reset_mock();self.helper.write_bytes(b'wrong')
            with self.assertRaisesRegex(ValueError,'provenance/hash'):audio.begin(self.game,self.owner,self.log)
            guard.assert_not_called()

    def test_manual_helper_missing_wrong_hash_and_old_policy_rejected(self):
        import hashlib
        receipt=self.helper.with_name('audio-helper-provenance.json')
        with self.assertRaises(FileNotFoundError):audio.verify_manual_helper(self.helper)
        value=dict(schema=1,policy='manual_game_session_v1',helper=self.helper.name,
                   helper_sha256=hashlib.sha256(self.helper.read_bytes()).hexdigest())
        receipt.write_text(json.dumps(value));self.assertEqual(audio.verify_manual_helper(self.helper),value)
        self.helper.write_bytes(b'old-helper')
        with self.assertRaisesRegex(ValueError,'provenance/hash'):audio.verify_manual_helper(self.helper)
        value['helper_sha256']=hashlib.sha256(self.helper.read_bytes()).hexdigest();value['policy']='mute_only'
        receipt.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError,'provenance/hash'):audio.verify_manual_helper(self.helper)
        self.helper.unlink()
        with self.assertRaisesRegex(ValueError,'provenance/hash'):audio.verify_manual_helper(self.helper)

    def test_manual_exact_identity_command_and_late_session_not_claimed(self):
        def spawn(command,**kwargs):
            self.command=command
            kwargs['stdout'].write((json.dumps(dict(event='guard_ready',**self.game))+'\n').encode());kwargs['stdout'].flush()
            process=mock.Mock();process.poll.return_value=None
            def finish(timeout):
                with self.log.open('a') as stream:
                    stream.write(json.dumps(dict(event='manual_audio_cleanup',sessions_configured=0,configure_failed=0))+'\n')
                return 0
            process.wait.side_effect=finish;return process
        with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME=''):
            guard=audio.Guard(self.game,self.owner,self.log,helper=self.helper,manual=True,events=self.events,popen=spawn)
        self.assertEqual(self.command[1],'--manual-volume')
        self.assertEqual(self.command[self.command.index('--creation-time')+1],'34')
        self.assertFalse(guard.close()['configured'])
        self.assertNotIn('restored',guard.close())

    def test_only_exact_opt_in_and_valid_identity(self):
        for value in ('','0','true','yes'):
            with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME=value),mock.patch.object(audio,'Guard') as guard:
                self.assertIsNone(audio.begin(self.game,self.owner,self.log)); guard.assert_not_called()
        for bad in ({},{'pid':True,'creation_time':34},{'pid':12,'creation_time':0}):
            with self.assertRaises(ValueError): audio.identity(bad)

    def test_normal_cancel_game_exit_and_owner_exit_share_restore(self):
        for reason in ('stop','game_exit','owner_exit','deadline'):
            with self.subTest(reason=reason):
                if self.log.exists(): self.log.unlink()
                guard = self.guard(reason)
                result = guard.close()
                self.assertTrue(result['restored']); self.assertEqual(result['sessions_restored'],1)
                self.assertEqual(result['reason'],reason)
                self.assertEqual(guard.close(),result)
                self.assertEqual(self.process.wait.call_count,1)
                for option, expected in (('--pid','12'),('--creation-time','34'),('--owner-pid','56'),('--owner-creation-time','78')):
                    self.assertEqual(self.command[self.command.index(option)+1],expected)

    def test_missing_session_is_not_claimed_as_muted(self):
        guard = self.guard()
        self.assertFalse(any(r.get('event')=='audio_muted' for r in audio.records(self.log)))
        guard.close()

    def test_no_receipt_or_timeout_never_kills_guard_or_claims_restored(self):
        for timeout in (True,False):
            if self.log.exists(): self.log.unlink()
            guard = self.guard(cleanup=False,timeout=timeout)
            self.assertFalse(guard.close()['restored'])
            self.process.kill.assert_not_called(); self.process.terminate.assert_not_called()

    def test_startup_error_signals_restore(self):
        with self.assertRaisesRegex(RuntimeError,'did not acknowledge'):
            self.guard(ready=False)
        self.events.signal.assert_called_once_with(7)
        self.events.close.assert_called_once_with(7)
        self.process.wait.assert_called_once()


if __name__=='__main__': unittest.main()
