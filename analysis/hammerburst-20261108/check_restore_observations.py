import copy,unittest
from verify_restore_observations import verify

def fixture(native=True):
    before=dict(game=dict(pid=7,creation_time=11),weapon='0x123',capacity=78,loaded=0 if native else 78,reserve=78 if native else 0,infinite_reserve=False,active_reload_bonus_shots=0,game_time=10.)
    phases=['parked','rebound','display_ready','up_baseline','intent']+(['native_reacquired'] if native else [])+['resumed']
    rows=[]
    for i,phase in enumerate(phases):
        row=dict(phase=phase,weapon='123',world='456',serial='1',old_item='1',accepted_begin_ms='100',perfect_end_ms='4000',reward_start='10',
            uncertain='0',loaded=str(before['loaded']),reserve=str(before['reserve']),capacity='78',angle='.6',hold='1' if i==0 else '2',
            left_known='1',left_tracked='1',left_down='0' if phase=='up_baseline' else '1',frame=str(20+i),press='2',source_sync_ns=str(1000+i),
            native_owned='1' if phase in ('native_reacquired','resumed') and native else '0',feed='0' if native else '1',stage='5' if native else '7')
        rows.append(row)
    return rows,before,copy.deepcopy(before)

class Tests(unittest.TestCase):
    def test_both_real_resume_branches(self):
        for native in (False,True):
            rows,before,after=fixture(native);self.assertTrue(verify(rows,before,after,native_reacquired=native)['original_clock_preserved'])
    def test_reject_fabricated_or_replayed_outcomes(self):
        mutations=[lambda r,b,a:r.pop(3),lambda r,b,a:r.append(copy.deepcopy(r[-1])),
            lambda r,b,a:r[1].update(hold='1'),lambda r,b,a:r[4].update(press='1'),
            lambda r,b,a:r[3].update(left_down='1'),lambda r,b,a:r[4].update(source_sync_ns=r[3]['source_sync_ns']),
            lambda r,b,a:r[-1].update(accepted_begin_ms='200'),lambda r,b,a:r[-1].update(uncertain='1'),
            lambda r,b,a:a.update(loaded=a['loaded']+1),lambda r,b,a:a['game'].update(creation_time=12),
            lambda r,b,a:r[-1].update(angle='.7'),lambda r,b,a:r[-1].update(native_owned='0'),
            lambda r,b,a:r[-1].update(stage='13'),lambda r,b,a:a.update(active_reload_bonus_shots=1),
            lambda r,b,a:a.update(game_time=9.)]
        for change in mutations:
            with self.subTest(change=mutations.index(change)):
                rows,before,after=fixture();change(rows,before,after)
                with self.assertRaises(AssertionError):verify(rows,before,after,native_reacquired=True)

if __name__=='__main__':unittest.main(verbosity=2)
