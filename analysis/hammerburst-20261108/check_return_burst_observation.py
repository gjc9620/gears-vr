from pathlib import Path
import sys,json,types
r=Path.cwd();sys.path.insert(0,str(r/'analysis/solo-m1-20261028'));import hammerburst_reload_chain as m
valid=dict(trace_valid='1',write_source_consistent='1',trace_source_consistent='1')
def case(rows,*,reject=False,bad=False,read_delay=0,expect=None):
 now=0.;i=0;observations={};saved=[];reads=[]
 after=dict(loaded=78,reserve=0,capacity=78,infinite_reserve=False)
 def clock():return now
 def sleep(t):
  nonlocal now
  assert 0<t<=.1;now+=t
 def records(prefix):
  rays,loaded=rows[min(i,len(rows)-1)][:2]
  if prefix=='native_lancer_shot_trajectory':return [dict(valid,trace_valid='0' if bad else '1') for _ in range(rays)]
  return [dict(count='5')] if reject else []
 def count(c,label):
  nonlocal i,now
  row=rows[min(i,len(rows)-1)];v=dict(after,loaded=row[1]);i+=1
  if len(row)>2:v.update(row[2])
  reads.append(v);now+=read_delay;return v
 old=m.counts;m.counts=count;error=None
 try:
  c=types.SimpleNamespace(live=lambda:None,records=records)
  result=m.observe_return_burst(c,after,0,.5,{'reject':0},observations,lambda:saved.append(1),clock=clock,sleep=sleep)
 except AssertionError as e:error=str(e)
 finally:m.counts=old
 if expect is None:assert error is None and len(result[0])==6 and result[1]['loaded']==72,(error,observations)
 else:assert error is not None and expect in error,(error,expect)
 assert (now<=.5 or read_delay) and (not observations or saved)
 return dict(error=error,elapsed=now,reads=len(reads))
results=[case([(5,73),(6,72)]),case([(5,72),(6,72)]),case([(6,73),(6,72)]),case([(6,72)]),case([(5,73)],reject=True,expect='explicitly rejected'),case([(5,73)],expect='existing reload deadline'),case([(7,71)],expect='Invalid'),case([(5,73)],bad=True,expect='Invalid'),case([(5,73,{'reserve':1})],expect=''),case([(5,73),(5,74)],expect='inventory progress'),case([(0,78)],expect='Invalid'),case([(6,72)],read_delay=.6,expect='existing reload deadline')]
p=r/'analysis/hammerburst-20261108/return-burst-observation-check.json';p.write_text(json.dumps(dict(cases=len(results),passed=True,no_input_dispatch=True,no_deadline_renewal=True,game_run=False,results=results),indent=2)+'\n');print('Return burst observation:',len(results),'cases passed')
