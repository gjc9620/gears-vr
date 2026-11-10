"""Execute the actual adapted drain with fake engine/first-press seams."""
import sys,types,unittest
from unittest.mock import patch
import hammerburst_isolated_operator as adapter
sys.path.insert(0,str(adapter.CODE))
import confirmed_first_fire

class Tests(unittest.TestCase):
    def execute(self,mode='success'):
        module=types.ModuleType('hb_drain_fixture');module.__file__=str(adapter.CODE/'hammerburst_supply_boundary_chain.py')
        exec(compile(adapter.supply_source(),module.__file__,'exec'),module.__dict__)
        now=[1.];loaded=[12];rays=[];deadlines=[];commands=[];persisted=[];evidence={}
        def clock():return now[0]
        def pad(hand,wait=0,**controls):commands.append(controls);now[0]+=wait
        def counts(c,label):
            if mode=='late_read' and deadlines:now[0]=19.01
            return dict(loaded=loaded[0],reserve=78,capacity=78,infinite_reserve=False)
        def observed_press(c,data,*,expected_shots,deadline_ns):
            deadlines.append(deadline_ns);pad(1,trigger=1);pad(1)
            if mode=='rejected':raise TimeoutError('Actual first shot unavailable; no retry')
            now[0]+=1.;loaded[0]-=expected_shots
            rays.extend(dict(trace_valid='1',trace_source_consistent='1',write_source_consistent='0' if mode=='bad_trace' else '1') for _ in range(expected_shots))
            data['checks']['actual_fire']=True
        module.counts=counts;module.time=types.SimpleNamespace(perf_counter_ns=lambda:int(now[0]*1e9))
        c=types.SimpleNamespace(g=types.SimpleNamespace(pad=pad,live=lambda:True),live=lambda:None,
            records=lambda prefix:rays if prefix=='native_lancer_shot_trajectory' else [])
        error=None
        with patch.object(confirmed_first_fire,'first_fire',observed_press):
            try:module.drain(c,'actual_adapted',evidence=evidence,persist=lambda:persisted.append(len(evidence.get('confirmed_inputs',[]))),clock=clock,sleep=lambda s:None)
            except (AssertionError,TimeoutError) as caught:error=caught
        return error,evidence,deadlines,commands,persisted
    def test_real_adapted_two_bursts_share_original_deadline(self):
        error,evidence,deadlines,commands,persisted=self.execute()
        self.assertIsNone(error);self.assertTrue(evidence['complete']);self.assertEqual(evidence['actual_ammo_spent'],12)
        self.assertEqual(deadlines,[19_000_000_000]*2);self.assertEqual(sum(c.get('trigger',0)==1 for c in commands),2)
        self.assertEqual(len(evidence['confirmed_inputs']),2);self.assertTrue(persisted)
    def test_failure_is_saved_and_not_retried(self):
        error,evidence,deadlines,commands,persisted=self.execute('rejected')
        self.assertIsInstance(error,TimeoutError);self.assertEqual(len(deadlines),1)
        self.assertEqual(sum(c.get('trigger',0)==1 for c in commands),1);self.assertFalse(evidence['complete']);self.assertEqual(len(evidence['confirmed_inputs']),1)
    def test_late_typed_count_cannot_pass(self):
        error,evidence,deadlines,commands,_=self.execute('late_read')
        self.assertIsInstance(error,AssertionError);self.assertIn('original whole drain deadline',str(error));self.assertEqual(len(deadlines),1)
    def test_existing_trajectory_consistency_guard_remains(self):
        error,evidence,_,_,_=self.execute('bad_trace')
        self.assertIsInstance(error,AssertionError);self.assertIn('trajectory mismatch',str(error));self.assertFalse(evidence['complete'])

if __name__=='__main__':unittest.main(verbosity=2)
