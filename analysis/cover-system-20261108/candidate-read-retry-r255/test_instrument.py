from pathlib import Path
import ast,copy,hashlib,importlib.util,json,math,struct
from types import SimpleNamespace
from instrument import instrument,BEFORE
checks=0
def check(value):
 global checks
 checks+=1
 assert value,checks
marker='import json, math, struct, sys, time\n'
source=marker+'''def run(observer, hints, report, ctx, origin):
        candidates = []
'''+BEFORE+'''
                candidates.append((link['serial'], raw['serial']))
        return candidates
'''
error_text='World-state metadata/registry changed at closing boundary'
def diff(data=0x20000,count=4,capacity=10,new_data=0x20000,new_count=5,new_capacity=10,address=0xABC0):
 return dict(kind='registry',address=address,size=16,before=struct.pack('<Qii',data,count,capacity).hex(),after=struct.pack('<Qii',new_data,new_count,new_capacity).hex())
def execute(plans,hints=(1,),verify_failure=0,initial=None):
 events=[];instances=[];saved=[];report={} if initial is None else copy.deepcopy(initial)
 class Observer:
  checks=0
  def verify(self):
   self.checks+=1;events.append(('verify',self.checks))
   if self.checks==verify_failure:raise RuntimeError('owner identity changed')
 observer=Observer()
 class Query:
  def __init__(self,owner):
   self.serial=len(instances)+1;self.observer=owner;self.r=SimpleNamespace(objects_header=0xABC0);self.closing_rejection=None
   self.plan=plans[self.serial-1] if self.serial<=len(plans) else None;instances.append(self);events.append(('new',self.serial))
  def obj(self,address):return dict(address=address,name='Cover',serial=self.serial)
  def pointer(self,link,name):return dict(address=7)
  def array(self,*args):return (0,0,0,1)
  def finish(self,*args,**kwargs):
   check(not args and not kwargs);events.append(('finish',self.serial));self.observer.verify()
   if self.plan is not None:
    self.closing_rejection=copy.deepcopy(self.plan[0]);raise self.plan[1]
 def save():saved.append(copy.deepcopy(report));events.append(('save',len(saved)))
 namespace=dict(CoverQuery=Query,save=save,
   _geometry=lambda q,*args:dict(value=[q.serial,0,0]),
   slot_snapshot=lambda q,*args:dict(rows=[dict(serial=q.serial)]),
   slot_world_candidates=lambda p,r,rows:dict(rows=rows))
 exec(compile(instrument(source),'<candidate-read-test>','exec'),namespace)
 result=None;failure=None
 try:result=namespace['run'](observer,list(hints),report,SimpleNamespace(world=7),[0,0,0])
 except BaseException as e:failure=e
 return SimpleNamespace(result=result,failure=failure,instances=instances,report=report,events=events,saved=saved,namespace=namespace)
valid=(diff(),RuntimeError(error_text))
case=execute([None]);check(case.result==[(1,1)] and not case.saved and len(case.instances)==1)
case=execute([valid,None]);check(case.result==[(2,2)] and len(case.instances)==2 and case.report['candidate_read_retry_used'])
check(case.saved[0]['candidate_read_rejections'][0]['retry_next_read'] is True)
check(case.events.index(('save',1))<case.events.index(('new',2)))
check(case.events.index(('verify',2))<case.events.index(('new',2)))
check(case.report['candidate_closing_rejection']['difference']==valid[0])
case=execute([valid,valid]);check(case.failure is not None and case.result is None and len(case.instances)==2)
check([x['retry_next_read'] for x in case.report['candidate_read_rejections']]==[True,False])
case=execute([valid,None,valid,None],hints=(1,2));check(case.failure is not None and case.result is None and len(case.instances)==3)
check([x['retry_next_read'] for x in case.report['candidate_read_rejections']]==[True,False])
case=execute([valid,None],verify_failure=2);check(case.failure is not None and len(case.instances)==1 and case.result is None)
case=execute([valid,None],initial=dict(candidate_read_retry_used=True));check(case.failure is not None and len(case.instances)==1)
bad=[]
for kwargs in [dict(new_data=0x20008),dict(new_count=6),dict(new_count=3),dict(new_capacity=11),dict(count=0,new_count=1),dict(capacity=4,new_capacity=4),dict(capacity=4000001,new_capacity=4000001),dict(address=0xABD0),dict(data=0,new_data=0),dict(data=0x20001,new_data=0x20001)]:bad.append(diff(**kwargs))
for key,value in [('kind','metadata'),('kind','identity'),('size',8),('before','zz'),('after','00')]:
 row=diff();row[key]=value;bad.append(row)
bad.extend([None,{},dict(kind='registry',size=16,address=0xABC0)])
for rejected in bad:
 case=execute([(rejected,RuntimeError(error_text)),None]);check(case.failure is not None and len(case.instances)==1 and case.result is None and not case.report.get('candidate_read_retry_used',False))
for error in [RuntimeError('Cover value changed at closing boundary'),ValueError(error_text),KeyboardInterrupt()]:
 case=execute([(diff(),error),None]);check(case.failure is error and len(case.instances)==1 and case.result is None)
# Decoded convenience fields do not override the actual retained header bytes.
row=diff();row['before_header']=dict(data=0,count=0,capacity=0)
case=execute([(row,RuntimeError(error_text)),None]);check(case.result==[(2,2)])
try:instrument(instrument(source));raise AssertionError('double instrument accepted')
except ValueError:checks+=1
# Freeze and parse the ACTUAL existing source, after its existing observers.
r=Path(__file__).resolve().parents[3];actual=r/'analysis/solo-m1-20261028/cover_boundary_chain.py';text=actual.read_text(encoding='utf-8')
for name,path in [('entry',r/'analysis/cover-system-20261108/query-age-cause-r149/entry_observation.py'),('closing',r/'analysis/cover-system-20261108/query-close-detail-r180/instrument.py')]:
 spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);text=module.instrument(text)
transformed=instrument(text);ast.parse(transformed);check('allow_registry_growth=True' not in transformed)
result=dict(checks=checks,passed=True,actual_source_sha256=hashlib.sha256(actual.read_bytes()).hexdigest(),instrument_sha256=hashlib.sha256(Path(__file__).with_name('instrument.py').read_bytes()).hexdigest(),transformed_source_sha256=hashlib.sha256(transformed.encode()).hexdigest(),scope='Actual candidate-source transform, synthetic fresh queries/strict-close failures only; no game/input/original dispatch')
Path(__file__).with_name('test-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
