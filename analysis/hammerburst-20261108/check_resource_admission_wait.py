"""Protect the one prepared attempt; no launcher/game/input/slot mutation."""
import unittest
from run_isolated_empty import wait_for_resource_admission

class Tests(unittest.TestCase):
    def setUp(self):self.now=0.;self.checks=0
    def clock(self):return self.now
    def sleep(self,seconds):self.now+=seconds
    def call(self,check,**kwargs):
        return wait_for_resource_admission(check=check,clock=self.clock,sleep=self.sleep,**kwargs)
    def test_busy_then_free_returns_one_admission_without_reservation(self):
        def check():
            self.checks+=1
            if self.checks<3:raise RuntimeError('Two automatic instances already own this machine')
            return {'disk':'admitted','memory':'admitted'}
        value=self.call(check,paused=lambda:False)
        self.assertEqual(self.checks,3);self.assertEqual(value['waited_seconds'],2)
        self.assertFalse(value['runtime_slot_reserved'])
    def test_owner_pause_during_wait_cannot_admit(self):
        def check():raise RuntimeError('Two automatic instances already own this machine')
        with self.assertRaisesRegex(RuntimeError,'Owner paused'):
            self.call(check,paused=lambda:self.now>=1)
    def test_unexpected_error_is_not_retried(self):
        def check():self.checks+=1;raise RuntimeError('Identity mismatch')
        with self.assertRaisesRegex(RuntimeError,'Identity mismatch'):
            self.call(check,paused=lambda:False)
        self.assertEqual(self.checks,1)
    def test_deadline_is_not_renewed_by_rejections(self):
        def check():raise RuntimeError('Insufficient currently available memory')
        with self.assertRaisesRegex(TimeoutError,'no start attempted'):
            self.call(check,paused=lambda:False,timeout=3)
        self.assertEqual(self.now,3)
    def test_late_or_paused_success_cannot_admit(self):
        for late in (False,True):
            self.now=0
            def check():self.now=4;return {}
            with self.subTest(late=late),self.assertRaises((TimeoutError,RuntimeError)):
                self.call(check,paused=lambda:False if late else self.now>=1,timeout=3)

if __name__=='__main__':unittest.main(verbosity=2)
