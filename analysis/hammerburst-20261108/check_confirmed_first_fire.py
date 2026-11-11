import unittest
from types import SimpleNamespace
from confirmed_first_fire import first_fire

class Host:
    def __init__(self,fault=None,shots=6):
        self.now=1_000_000_000;self.state='';self.inputs=[];self.fault=fault;self.shots=shots;self.fired=False
        self.g=SimpleNamespace(live=lambda:True,pad=self.pad)
    def pad(self,hand,wait=0,**controls):
        self.inputs.append(dict(controls));self.state='down' if controls.get('trigger') else 'up'
        if self.state=='down':self.fired=True
    def sleep(self,seconds):self.now+=int(seconds*1e9)
    def records(self,prefix):
        if prefix.startswith('native_hammerburst_'):return []
        if prefix=='native_lancer_shot_trajectory':
            count=self.shots
            if self.fault=='delayed_tail':count=1 if self.state=='down' else 5 if self.now<1_800_000_000 else 6
            return [{'trace_valid':'1'}]*count if self.fired else []
        row=dict(sequence=str(self.now),frame='100',current='1',source_status='1',trigger_active='1',active_at_admission='1',
                 input_sync_ns=str(self.now),admitted_ns=str(self.now),status='1',owned='0',game_known='1',game_r2='0',
                 trigger_value='1' if self.state=='down' else '0',trigger_pressed='1',trigger_released='0',preparation_budget='1')
        if self.fault=='stale':row['input_sync_ns']='1'
        if self.fault=='inactive':row['active_at_admission']='0'
        if self.fault=='owned':row['owned']='1'
        if self.fault=='old_source_recent_receipt' or self.fault=='late_then_fresh' and self.now<1_150_000_000:row['input_sync_ns']=str(self.now-70_000_000)
        if self.fault=='no_budget':row['preparation_budget']='0'
        if self.fault=='blocked_down' and self.state=='down':row['status']='8'
        if self.fault=='down_missing' and self.state=='down':row['trigger_value']='0';row['trigger_released']='1'
        return [row]
    def run(self):
        data={'checks':{}};first_fire(self,data,clock=lambda:self.now,sleep=self.sleep);return data

class Tests(unittest.TestCase):
    def test_failed_baseline_keeps_bounded_original_age_diagnostics(self):
        h=Host('old_source_recent_receipt');d={'checks':{}}
        with self.assertRaises(TimeoutError):first_fire(h,d,clock=lambda:h.now,sleep=h.sleep)
        wait=d['first_fire_input']['baseline_wait']
        self.assertEqual(h.inputs,[{}]);self.assertEqual(len(wait['samples']),8)
        self.assertGreater(wait['distinct_rows'],8)
        self.assertEqual(wait['minimum_source_age']['source_age_ns'],70_000_000)
        self.assertEqual(wait['minimum_source_age']['receipt_age_ns'],0)
        self.assertNotIn('down_sent_ns',d['first_fire_input'])
    def test_diagnostics_do_not_replace_one_press_or_original_producer(self):
        h=Host();d=h.run();r=d['first_fire_input']
        self.assertEqual(r['baseline_wait']['minimum_source_age']['row'],r['baseline'])
        self.assertEqual(h.inputs,[{},dict(trigger=1),{}])
    def test_late_tail_observes_same_press_without_retry(self):
        h=Host('delayed_tail');data=h.run();self.assertTrue(data['checks']['actual_fire'])
        self.assertEqual(h.inputs,[{},dict(trigger=1),{}]);self.assertGreaterEqual(h.now,1_800_000_000)
    def test_outer_deadline_is_not_renewed(self):
        h=Host('down_missing');data={'checks':{}}
        with self.assertRaises(TimeoutError):first_fire(h,data,clock=lambda:h.now,sleep=h.sleep,deadline_ns=h.now+50_000_000)
        self.assertLessEqual(h.now,1_050_000_000);self.assertEqual(h.inputs,[{}])
        self.assertNotIn('down_sent_ns',data['first_fire_input'])
    def test_full_burst_budget_boundary_does_not_extend_deadline(self):
        for budget in (87_620_700,3_499_999_999,3_500_000_000):
            h=Host();data={'checks':{}};deadline=h.now+budget
            if budget<3_500_000_000:
                with self.assertRaisesRegex(TimeoutError,'no DOWN sent'):
                    first_fire(h,data,clock=lambda:h.now,sleep=h.sleep,deadline_ns=deadline)
                self.assertEqual(h.inputs,[{}])
            else:
                first_fire(h,data,clock=lambda:h.now,sleep=h.sleep,deadline_ns=deadline)
                self.assertTrue(data['checks']['actual_fire'])
                self.assertEqual(h.inputs,[{},dict(trigger=1),{}])
            self.assertEqual(data['first_fire_input']['outer_deadline_ns'],deadline)
    def test_one_press_waits_for_consumption(self):
        h=Host();d=h.run();self.assertEqual(h.inputs,[{},dict(trigger=1),{}]);self.assertTrue(d['checks']['actual_fire'])
        self.assertEqual(len(d['shots']),6);self.assertEqual(d['first_fire_input']['down']['trigger_value'],'1')
    def test_no_press_without_fresh_unowned_up(self):
        for fault in ('stale','inactive','owned'):
            with self.subTest(fault=fault):
                h=Host(fault)
                with self.assertRaises(TimeoutError):h.run()
                self.assertEqual(h.inputs,[{}])
    def test_missing_down_releases_once_without_repress(self):
        h=Host('down_missing')
        with self.assertRaises(TimeoutError):h.run()
        self.assertEqual(h.inputs,[{},dict(trigger=1),{}])
    def test_wrong_burst_size_fails_without_retry(self):
        for n in (0,3,7):
            with self.subTest(shots=n):
                h=Host(shots=n)
                with self.assertRaises(TimeoutError if n==0 else AssertionError):h.run()
                self.assertEqual(h.inputs,[{},dict(trigger=1),{}])

    def test_recent_receipt_does_not_hide_near_expired_up_source(self):
        for fault in ('old_source_recent_receipt','no_budget'):
            h=Host(fault)
            with self.assertRaises(TimeoutError):h.run()
            self.assertEqual(h.inputs,[{}])

    def test_wait_for_better_up_before_only_press(self):
        h=Host('late_then_fresh');d=h.run()
        self.assertEqual(h.inputs,[{},dict(trigger=1),{}])
        self.assertGreaterEqual(d['first_fire_input']['down_sent_ns'],1_150_000_000)

    def test_blocked_down_is_failure_not_successful_consumption(self):
        h=Host('blocked_down');data={'checks':{}}
        with self.assertRaisesRegex(AssertionError,'Original DOWN blocked'):
            first_fire(h,data,clock=lambda:h.now,sleep=h.sleep)
        self.assertEqual(h.inputs,[{},dict(trigger=1),{}])
        self.assertEqual(data['first_fire_input']['first_down_rejection']['status'],'8')

if __name__=='__main__':unittest.main(verbosity=2)
