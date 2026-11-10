"""Exercise actual pulse timing and the existing publisher, without a game."""
import math
from types import SimpleNamespace
import unittest
from cover_ordinary_approach import timed_pulse

class Clock:
    def __init__(self):self.now=0.;self.sleeps=[]
    def read(self):return self.now
    def sleep(self,seconds):self.sleeps.append(seconds);self.now+=seconds

def fixture(clock,check_delay=0.,put_delay=0.,fail_at=None):
    calls=[];events=[];checks=[]
    def live():checks.append(clock.now);clock.now+=check_delay
    def put(hand,pose,**controls):
        calls.append((clock.now,hand,pose,controls));clock.now+=put_delay
        if len(calls)==fail_at:raise RuntimeError('existing publisher failed')
    ctx=SimpleNamespace(live=live,g=SimpleNamespace(put=put,poses={0:object()},
        rec=SimpleNamespace(event=lambda label,**fields:events.append((label,fields)))))
    return ctx,calls,events,checks

class PulseTest(unittest.TestCase):
    def test_regular_ramp_releases_at_planned_end(self):
        c=Clock();ctx,calls,events,checks=fixture(c)
        timed_pulse(ctx,.62,0.,.04,clock=c.read,sleep=c.sleep)
        self.assertEqual(calls[-1][3],{})
        self.assertAlmostEqual(calls[-1][0],.32)
        self.assertTrue(all(row[1]==0 and row[2] is ctx.g.poses[0] for row in calls))
        values=[row[3].get('stickX',0.) for row in calls]
        self.assertEqual(values[0],0.)
        self.assertLessEqual(max(values),.62)
        self.assertGreater(max(values),.60)
        self.assertGreater(values[3],values[1]);self.assertLess(values[-2],values[7])
        self.assertEqual(events[0][1]['scheduling_overrun_seconds'],0.)

    def test_slow_identity_checks_do_not_finish_all_old_fixed_steps(self):
        c=Clock();ctx,calls,events,checks=fixture(c,check_delay=.11)
        timed_pulse(ctx,-.5,-.3,.04,clock=c.read,sleep=c.sleep)
        self.assertEqual(len(calls),3) # Two active samples, then release.
        self.assertEqual(calls[-1][3],{})
        self.assertLess(calls[-1][0],.4)
        self.assertGreater(events[0][1]['scheduling_overrun_seconds'],0.)

    def test_slow_publication_is_reported_and_then_released(self):
        c=Clock();ctx,calls,events,checks=fixture(c,put_delay=.5)
        timed_pulse(ctx,.62,0.,.04,clock=c.read,sleep=c.sleep)
        self.assertEqual(len(calls),2);self.assertEqual(calls[-1][3],{})
        self.assertAlmostEqual(events[0][1]['scheduling_overrun_seconds'],.68)

    def test_original_publisher_exception_still_attempts_release(self):
        c=Clock();ctx,calls,events,checks=fixture(c,fail_at=2)
        with self.assertRaisesRegex(RuntimeError,'existing publisher failed'):
            timed_pulse(ctx,.62,0.,.04,clock=c.read,sleep=c.sleep)
        self.assertEqual(calls[-1][3],{})

    def test_invalid_pulse_never_publishes(self):
        for args in ((math.nan,0.,.04),(.63,0.,.04),(.62,0.,1.)):
            c=Clock();ctx,calls,events,checks=fixture(c)
            with self.assertRaises(ValueError):timed_pulse(ctx,*args,clock=c.read,sleep=c.sleep)
            self.assertFalse(calls)

    def test_stalled_clock_remains_bounded_and_releases(self):
        c=Clock();ctx,calls,events,checks=fixture(c)
        with self.assertRaisesRegex(RuntimeError,'bounded publication count'):
            timed_pulse(ctx,.62,0.,.04,clock=c.read,sleep=lambda _:None)
        self.assertEqual(len(calls),65);self.assertEqual(calls[-1][3],{})

if __name__=='__main__':unittest.main()
