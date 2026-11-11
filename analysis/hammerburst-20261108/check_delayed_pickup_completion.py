from pathlib import Path
import json
from check_shoulder_pickup import Fixture
from confirmed_shoulder_pickup import acquire
A=Path(__file__).resolve().parent
class Delayed(Fixture):
    def pad(self,index,wait=0,squeeze=0):
        super().pad(index,wait=wait,squeeze=squeeze)
        if not squeeze:return
        held=dict(self.rows['native_equipment'][0],cookie='1',frame='13',source_sequence='23',input_status='baseline')
        pending=dict(held,status='unholster_pending',frame='11',source_sequence='21',input_status='event')
        if self.variant=='no_pending':self.rows['native_equipment']=[held];return
        if self.variant=='pending_not_event':pending['input_status']='baseline'
        for variant,key,value in (
            ('wrong_cookie','cookie','2'),('missing_cookie','cookie','0'),('wrong_event','event','2'),
            ('wrong_press','press','2'),('wrong_target','target','456'),('old_frame','frame','11'),
            ('old_sequence','source_sequence','20'),('old_source','input_sync_ns','1')):
            if self.variant==variant:held[key]=value
        if self.variant=='rejected':self.rows['native_equipment']=[pending,dict(pending,status='switch_interrupted',frame='12'),held]
        else:self.rows['native_equipment']=[pending,dict(pending,status='unavailable',frame='12'),held]
cases=0
f=Delayed();data={};acquire(f,data,clock=f.clock,sleep=f.sleep)
assert f.sent==[0,1] and data['pickup_input']['completion_path']=='same_dispatched_after_input_baseline';cases+=1
for variant in ('no_pending','pending_not_event','wrong_cookie','missing_cookie','wrong_event','wrong_press','wrong_target','old_frame','old_sequence','old_source','rejected'):
    f=Delayed(variant);data={}
    try:acquire(f,data,clock=f.clock,sleep=f.sleep)
    except (TimeoutError,AssertionError):pass
    else:raise AssertionError(variant)
    assert f.sent==[0,1,0] and f.sent.count(1)==1 and 'held' not in data['pickup_input'],variant;cases+=1
# Replay the ACTUAL44240 native rows against the same reader, with a synthetic
# clock/input host only. No game process, publication or successful action is made.
proof=json.loads((A/'runtime-44240-first-failure-analysis.json').read_text())
baseline=proof['core']['pickup_input']['baseline'];native=proof['native_equipment_rows']
class ActualRows(Fixture):
    def __init__(self):
        super().__init__();self.initial=proof['core']['initial'];self.now=int(baseline['sync_ns'])-1_000_000
    def pad(self,index,wait=0,squeeze=0):
        self.sent.append(squeeze)
        if not squeeze:
            self.rows['hammerburst_pickup_up']=[baseline];self.now=int(baseline['admitted_ns'])+1000
        else:
            self.now+=1000;self.rows['native_equipment']=[r for r in native if int(r.get('frame',0))>=158]
f=ActualRows();data={};held=acquire(f,data,clock=f.clock,sleep=f.sleep)
assert held['frame']=='161' and held['event']=='1' and held['press']=='1' and held['input_status']=='baseline'
assert data['pickup_input']['original_dispatched']['frame']=='158' and f.sent==[0,1];cases+=1
(A/'delayed-pickup-completion-checks.json').write_text(json.dumps(dict(passed=True,added_cases=cases,original_driver_cases=17,actual44240_rows_replayed=True,actual_runtime_result='early-released-refire-r1/pickup-completion-test-result.json',new_input_or_retry=False,limits='Actual Runtime129/1656 with backend fixtures and actual recorded-row reader replay; not a fresh game/held stability proof.'),indent=2))
print(f'{cases} delayed original pickup cases passed, including actual44240 rows; no new input/retry')
