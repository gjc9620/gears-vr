import math
import unittest
from right_stick_policy import Policy,percent,zero_axis_deadzone,legacy_look_edit

class PolicyTests(unittest.TestCase):
    def test_retail_action_alias_precedes_legacy_axis(self):
        bindings=[('XboxTypeS_RightX','Game_LookX'),('XboxTypeS_RightY','Game_LookY'),
                  ('XboxTypeS_RightX','Axis aTurn Speed=1.0 DeadZone=0.2'),
                  ('XboxTypeS_RightY','Axis aLookup Speed=1.0 DeadZone=0.3')]
        edits=[(name,legacy_look_edit(name,text)) for name,text in bindings]
        self.assertEqual([name for name,edit in edits if edit is not None],['XboxTypeS_RightX','XboxTypeS_RightY'])
        self.assertTrue(all('DeadZone=0' in edit for name,edit in edits if edit is not None))
        with self.assertRaises(ValueError):legacy_look_edit('XboxTypeS_RightX','Game_LookY')
        with self.assertRaises(ValueError):legacy_look_edit('XboxTypeS_RightX','Game_Reload')
    def test_only_look_commands_and_preserve_allocation(self):
        for original in ['Axis aTurn Speed=1.0 DeadZone=0.2','Axis aLookup Speed=0.8 DeadZone=0.200',
                         'Axis aTurn DeadZone=.25 Invert=1','Axis aLookup Speed=1']:
            changed=zero_axis_deadzone(original)
            self.assertEqual(len(changed),len(original))
            self.assertEqual(zero_axis_deadzone(changed),changed)
            self.assertIn('Axis',changed)
        with self.assertRaises(ValueError):zero_axis_deadzone('Axis aStrafe DeadZone=0.3')
    def test_default_and_bad_percent(self):
        self.assertEqual(percent(2),2.)
        for value in [True,'2',-.01,50.1,math.nan,math.inf]:
            with self.assertRaises(ValueError):percent(value)

    def transaction(self,fail_address=None):
        # The policy must recover a partial write and respect later game edits.
        memory={100:b'old-x',200:b'old-y',300:b'.300'}
        class Reader:
            def read(self,address,size):return memory[address][:size]
        class Writer:
            def write(self,address,value):
                if address==fail_address and value==b'new-y':raise OSError('write failed')
                memory[address]=value
        policy=object.__new__(Policy)
        policy.r=Reader();policy.writer=Writer();policy.entries=[];policy.current=None
        policy.report=dict(applied=False,applications=0,restored=0,stale_restore_skipped=0)
        valid=[True]
        policy.plan=lambda:(lambda:valid[0],[(100,b'old-x',b'new-x'),(200,b'old-y',b'new-y'),(300,b'.300',b'.020')],{})
        return policy,memory,valid

    def test_partial_transaction_rolls_back(self):
        policy,memory,_=self.transaction(200)
        with self.assertRaises(OSError):policy.apply()
        self.assertEqual(memory,{100:b'old-x',200:b'old-y',300:b'.300'})
        self.assertFalse(policy.report['applied'])
        self.assertEqual(policy.report['restored'],1)

    def test_restore_keeps_external_changes(self):
        policy,memory,_=self.transaction();policy.apply()
        memory[200]=b'game!'
        policy.restore()
        self.assertEqual(memory,{100:b'old-x',200:b'game!',300:b'.300'})
        self.assertEqual(policy.report['restored'],2)
        self.assertEqual(policy.report['stale_restore_skipped'],1)

    def test_restore_does_not_write_reused_object(self):
        policy,memory,valid=self.transaction();policy.apply();installed=dict(memory)
        valid[0]=False;policy.restore()
        self.assertEqual(memory,installed)
        self.assertEqual(policy.report['restored'],0)
        self.assertEqual(policy.report['stale_restore_skipped'],3)

if __name__=='__main__':unittest.main()
