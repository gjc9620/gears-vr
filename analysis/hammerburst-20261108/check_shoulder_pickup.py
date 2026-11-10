"""Synthetic input/receipt seams, no Native/game execution."""
from types import SimpleNamespace
from pathlib import Path
import json
from confirmed_shoulder_pickup import acquire

class Fixture:
    def __init__(self,variant='good'):
        self.now=1_000_000_000;self.variant=variant;self.sent=[];self.rows={};self.initial={'weapon':'0x123'};self.g=self
    def clock(self):return self.now
    def sleep(self,seconds):self.now+=int(seconds*1e9)
    def live(self):return self.variant!='ended'
    def records(self,prefix):return self.rows.get(prefix,[])
    def pad(self,index,wait=0,squeeze=0):
        self.sent.append(squeeze);self.now+=10_000_000
        if squeeze==0:
            row=dict(frame='10',sequence='20',sync_ns=str(self.now),admitted_ns=str(self.now+1),source='1',right_down='0',right_pressed='0',right_released='0',input_status='baseline')
            if self.variant=='old_up':row['sync_ns']='1'
            if self.variant=='foreign_source':row['source']='0'
            if self.variant=='down_baseline':row['right_down']='1'
            if self.variant=='unreleased':row['right_pressed']='1'
            self.rows['hammerburst_pickup_up']=[row];self.now+=1000
        else:
            row=dict(status='held',target='123',event='1',press='1',frame='11',source_sequence='21',input_sync_ns=str(self.now),input_status='event',input_control='6')
            if self.variant=='wrong_weapon':row['target']='456'
            if self.variant=='old_hold':row['frame']='10'
            if self.variant=='old_press':row['press']='0'
            self.rows['native_equipment']=[row]
            if self.variant=='gap':self.rows['native_equipment_input_gap']=[dict(right_pressed='1',reason='producer_sync_gap')]

cases=0
f=Fixture();data={};result=acquire(f,data,clock=f.clock,sleep=f.sleep)
assert f.sent==[0,1] and result['target']=='123' and data['pickup_input']['held']==result;cases+=1
for variant in ('old_up','foreign_source','down_baseline','unreleased','ended'):
    f=Fixture(variant)
    try:acquire(f,{},clock=f.clock,sleep=f.sleep)
    except (TimeoutError,AssertionError):pass
    else:raise AssertionError(variant)
    assert f.sent==[0],(variant,f.sent);cases+=1
for variant in ('wrong_weapon','old_hold','old_press','gap'):
    f=Fixture(variant);data={}
    try:acquire(f,data,clock=f.clock,sleep=f.sleep)
    except (TimeoutError,AssertionError):pass
    else:raise AssertionError(variant)
    assert f.sent==[0,1,0] and f.sent.count(1)==1,(variant,f.sent);cases+=1
    if variant=='gap':assert data['pickup_input']['first_source_gap']['reason']=='producer_sync_gap'
f=Fixture()
try:acquire(f,{'pickup_input':{}},clock=f.clock,sleep=f.sleep)
except AssertionError:pass
else:raise AssertionError('Replayed attempt')
assert not f.sent;cases+=1
class ApproachFixture(Fixture):
    def pad(self,*args,**kwargs):
        super().pad(*args,**kwargs)
        if kwargs.get('squeeze',0)==0:self.rows['hammerburst_pickup_up']=[]
    def reach(self,end,*,ready):
        assert self.sent==[0],self.sent
        self.now+=30_000_000
        row=dict(frame='10',sequence='20',sync_ns=str(self.now),admitted_ns=str(self.now+1),source='1',right_down='0',right_pressed='0',right_released='0',input_status='quiet')
        if self.variant=='approach_old':row['sync_ns']='1'
        if self.variant!='approach_missing':self.rows['hammerburst_pickup_up']=[row]
        self.now+=1000;self.observed_during_reach=ready()
for variant in ('approach_good','approach_old','approach_missing'):
    f=ApproachFixture(variant);data={}
    try:acquire(f,data,shoulder={'x':.23},clock=f.clock,sleep=f.sleep)
    except TimeoutError:assert variant!='approach_good' and f.sent==[0]
    else:assert variant=='approach_good' and f.sent==[0,1] and f.observed_during_reach
    cases+=1
# Execute the actual adapted reach function with a finite gesture spy. Ensure
# callback success stops remaining UP samples and the old settling delay;
# ordinary later reaches retain their complete gesture and settling delay.
import ast
from hammerburst_isolated_operator import equipment_source
tree=ast.parse(equipment_source());node=next(n for n in ast.walk(tree) if isinstance(n,ast.FunctionDef) and n.name=='reach')
for observe in (False,True):
    events=[];poses={1:dict(x=0.,y=0.,z=0.)}
    g=SimpleNamespace(poses=poses,head_rest=dict(x=0.,y=0.,z=0.),rec=SimpleNamespace(event=lambda *a,**kw:None),put=lambda *a,**kw:events.append('put'))
    env=dict(g=g,time=SimpleNamespace(sleep=lambda s:events.append(s)),live=lambda:None,profile_for=lambda *a:None,
        gesture=lambda *a,**kw:iter((i,dict(x=float(i),y=0.,z=0.),{},.01) for i in range(1,4)))
    exec(compile(ast.fix_missing_locations(ast.Module(body=[node],type_ignores=[])),'actual-adapted-reach','exec'),env)
    env['reach'](dict(x=3.,y=0.,z=0.),ready=(lambda:True) if observe else None)
    assert events==(['put',.01] if observe else ['put',.01,'put',.01,'put',.01,.25])
    assert poses[1]['x']==(1. if observe else 3.);cases+=1
Path(__file__).with_name('shoulder-pickup-driver-checks.json').write_text(json.dumps(dict(passed=True,cases=cases,
 scope='Synthetic input and Native-receipt seams; exact one Grip/no replay, fresh released baseline and matching held target required',game_execution=False),indent=2))
print(f'{cases} pickup driver checks passed; input/receipt fixtures only')
