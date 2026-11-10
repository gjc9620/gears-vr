from pathlib import Path
import sys,types,json,ast
r=Path.cwd();o=r/'analysis/solo-m1-20261028';sys.path.insert(0,str(o));import hammerburst_supply_boundary_chain as m
valid=dict(trace_valid='1',trace_source_consistent='1',write_source_consistent='1')
def test(*,before_rows=0,loaded=3,rows=3,cap=False,marker=False,timeout=False,bad=False,reserve_bad=False,no_progress=False,should_fail=False,multi=False,partial=False):
 vals=[dict(loaded=loaded,reserve=0,capacity=78,infinite_reserve=False)]
 if multi:vals.append(dict(vals[0],loaded=3))
 if partial:vals.append(dict(vals[0],loaded=loaded-4))
 vals.append(dict(vals[0],loaded=loaded if no_progress else 0,reserve=int(reserve_bad)))
 i=0;shots=[dict(valid) for _ in range(before_rows)];released=[];pressed=[]
 def count(c,l):
  nonlocal i
  x=vals[min(i,len(vals)-1)];i+=1;return x
 def pad(h,wait=0,**k):
  if k.get('trigger'):
   n=([6,3][len(pressed)] if multi else rows);pressed.append(wait)
   shots.extend(dict(valid,trace_valid='0' if bad else '1') for _ in range(n))
  else:released.append(wait)
 m.counts=count;c=types.SimpleNamespace(g=types.SimpleNamespace(pad=pad,live=lambda:True),live=lambda:None,records=lambda p:shots if p=='native_lancer_shot_trajectory' else ([dict(count='4')] if partial and pressed and p=='native_hammerburst_continuation_rejected' else ([dict(first_omitted_shot='129',limit='128')] if marker and p=='native_lancer_trajectory_truncated' else [])))
 ticks=iter([0,19] if timeout else [0,.1,.2,.3,.4]);err=None;evidence={};saved=[]
 try:value=m.drain(c,'test',allow_export_cap=cap,clock=lambda:next(ticks),evidence=evidence,persist=lambda:saved.append(1))
 except AssertionError as e:err=e
 assert (err is not None)==should_fail
 assert evidence['before']['loaded']==loaded and saved
 if should_fail and not (timeout or no_progress or reserve_bad or partial):
  assert evidence['after']['loaded']==0 and not evidence['complete'] and 'exported_trajectories' in evidence
 assert len(released)==len(pressed)+1,'Every burst releases; finally also releases on error'
 assert all(t==.35 for t in pressed) and all(t==.6 for t in released[:-1]) and released[-1]==.5
 if partial:assert len(pressed)==1 and evidence['completion_observations'][0][0]['loaded']==loaded-4 and 'after' not in evidence
 if multi and err is None:assert value['genuine_press_release_bursts']==2
for kw in [{},{'timeout':True,'rows':0,'should_fail':True},{'rows':2,'should_fail':True},{'bad':True,'should_fail':True},{'reserve_bad':True,'should_fail':True},{'before_rows':125,'loaded':6,'rows':3,'cap':True,'marker':True},{'before_rows':125,'loaded':6,'rows':3,'cap':True,'should_fail':True},{'rows':0,'no_progress':True,'should_fail':True},{'loaded':9,'rows':9,'multi':True},{'loaded':12,'rows':4,'partial':True,'should_fail':True}]:test(**kw)
for n in ['hammerburst_reload_chain.py','hammerburst_core_chain.py','run_hammerburst_check.py','hammerburst_supply_boundary_chain.py']:ast.parse((o/n).read_text())
p=r/'analysis/hammerburst-20261108/supply-driver-check.json';x=json.loads(p.read_text());x.update(actual_drain_helper_cases=10,genuine_bursts='Same core .35DOWN/.6UP; wait for same partial original burst under unchanged18s whole deadline; explicit rejection or no progress stops without input retry',game_execution=False);p.write_text(json.dumps(x,indent=2));print(json.dumps(x))
