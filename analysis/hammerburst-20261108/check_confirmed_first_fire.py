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
        if prefix=='native_lancer_shot_trajectory':return [{'trace_valid':'1'}]*self.shots if self.fired else []
        row=dict(frame='100',current='1',source_status='1',trigger_active='1',active_at_admission='1',
                 input_sync_ns=str(self.now),admitted_ns=str(self.now),status='1',owned='0',game_known='1',game_r2='0',
                 trigger_value='1' if self.state=='down' else '0',trigger_pressed='1',trigger_released='0')
        if self.fault=='stale':row['input_sync_ns']='1'
        if self.fault=='inactive':row['active_at_admission']='0'
        if self.fault=='owned':row['owned']='1'
        if self.fault=='down_missing' and self.state=='down':row['trigger_value']='0';row['trigger_released']='1'
        return [row]
    def run(self):
        data={'checks':{}};first_fire(self,data,clock=lambda:self.now,sleep=self.sleep);return data

class Tests(unittest.TestCase):
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

if __name__=='__main__':unittest.main(verbosity=2)
