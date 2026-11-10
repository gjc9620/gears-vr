"""Real optional action adapter with simulated receipts; no game acceptance."""
from pathlib import Path
from types import SimpleNamespace
import copy,json,tempfile,unittest
from check_restore_observations import fixture
from hammerburst_restore_chain import run
from hammerburst_isolated_operator import reload_source


class Tests(unittest.TestCase):
    def exercise(self,*,missing_stow=False,changed_mount=False,changed_counts=False,foreign_mount=False):
        temp=tempfile.TemporaryDirectory();self.addCleanup(temp.cleanup)
        rows,before,after=fixture();rows[-1]['stage']='3';before['observed_ns']=10;after['observed_ns']=20
        after['game_time']=11.
        if changed_counts:after['loaded']+=1
        data={'hammerburst_restore':[dict(rows[0],serial='999')],'native_equipment':[]}
        presses=[];left=[];now=[0.];mount_calls=[];count_calls=[]
        g=SimpleNamespace(game=before['game'],directory=Path(temp.name),head_rest=dict(x=0,y=1.6,z=0),
            poses={1:dict(x=.18,y=1.4,z=-.48,yaw=0,pitch=0,roll=0)},
            rec=SimpleNamespace(event=lambda *a,**k:None))
        def pad(hand,**controls):
            if not controls.get('squeeze'):return
            presses.append((hand,controls))
            if len(presses)==1:
                if missing_stow:return
                data['native_equipment'].append(dict(status='stowed',slot='3',target='123'))
                data['hammerburst_restore'].append(rows[0])
            else:
                data['native_equipment'].append(dict(status='held',slot='3',target='123'))
                data['hammerburst_restore'].extend(rows[1:4])
        g.pad=pad
        def grip():left.append('new grip');data['hammerburst_restore'].extend(rows[4:])
        def reach(pose,squeeze=0):g.poses[1]=dict(pose)
        c=SimpleNamespace(g=g,live=lambda:True,records=lambda prefix:list(data.get(prefix,[])),reach=reach,
            current=lambda label:dict(weapon='0x123',cls='Weap_LocustAssaultRifle'))
        def counts(context,label):
            count_calls.append(label);return copy.deepcopy(before if len(count_calls)==1 else after)
        def mount():
            index=len(mount_calls);mount_calls.append(index)
            value=dict(game=copy.deepcopy(g.game),weapon='0x123',mesh='0x88',observed_ns=11 if index==0 else 19,
                       mesh_component_to_world=[0.]*16)
            if index and changed_mount:value['mesh_component_to_world'][12]=1.
            if index and foreign_mount:value['game']['creation_time']+=1
            return value
        def sleep(seconds):now[0]+=seconds
        try:
            result=run(c,count_reader=counts,release_left=lambda:left.append('release'),grip_left=grip,
                       mount_reader=mount,clock=lambda:now[0],sleep=sleep)
        except BaseException as error:
            return error,presses,left,json.loads((g.directory/'hammerburst-park-restore.json').read_text())
        return result,presses,left,json.loads((g.directory/'hammerburst-park-restore.json').read_text())

    def test_fresh_actual_rows_after_offset_and_real_up_then_grip(self):
        result,presses,left,saved=self.exercise()
        self.assertIsInstance(result,dict)
        self.assertEqual(len(presses),2)
        self.assertEqual(left,['release','new grip'])
        self.assertTrue(saved['checks']['park_resume_counts_preserved'])
        self.assertEqual(len(saved['restore_rows']),7)
        self.assertNotIn('999',[r['serial'] for r in saved['restore_rows']])

    def test_missing_stow_stops_without_another_grip(self):
        error,presses,left,saved=self.exercise(missing_stow=True)
        self.assertIsInstance(error,TimeoutError)
        self.assertEqual(len(presses),1);self.assertEqual(left,['release'])
        self.assertIn('no input retry',saved['error'])

    def test_changed_or_foreign_mount_blocks_left_resume_press(self):
        for options in ({'changed_mount':True},{'foreign_mount':True}):
            with self.subTest(options=options):
                error,presses,left,saved=self.exercise(**options)
                self.assertIsInstance(error,AssertionError)
                self.assertEqual(len(presses),2);self.assertEqual(left,['release'])
                self.assertFalse(saved['checks'])

    def test_independent_inventory_change_is_not_accepted(self):
        error,presses,left,saved=self.exercise(changed_counts=True)
        self.assertIsInstance(error,AssertionError)
        self.assertIn('Inventory changed',saved['error']);self.assertFalse(saved['checks'])

    def test_optional_hook_compiles_at_opened_magazine_without_enabling_default(self):
        source=reload_source()
        self.assertIn("if '--hammerburst-park-restore' in sys.argv:",source)
        self.assertLess(source.index('restore_run(c,count_reader=counts'),source.index("phase('extract_original_for_replacement'"))
        self.assertIn("wait('hammerburst_reload_transition',resumed_transitions",source)
        compile(source,'HB reload with optional restore hook','exec')


if __name__=='__main__':unittest.main(verbosity=2)
