"""Exercise the real drain helper with delayed completion and terminal failures.

Synthetic host/input only. No game process, input or inventory is changed.
"""
from pathlib import Path
import importlib.util,json,sys,types

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'analysis/solo-m1-20261028'))
import hammerburst_supply_boundary_chain as module

def check(samples,*,reject=False,expected_error=None):
    now=0.;index=0;presses=[];releases=[];sleeps=[];rays=[];evidence={}
    def clock():return now
    def sleep(seconds):
        nonlocal now
        assert 0<seconds<=.1
        now+=seconds;sleeps.append(seconds)
    def pad(hand,wait=0,**values):
        nonlocal now
        now+=wait
        (presses if values.get('trigger') else releases).append(wait)
    def counts(context,label):
        nonlocal index
        row=dict(loaded=6,reserve=78,capacity=78,infinite_reserve=False)
        if index:
            value=samples[min(index-1,len(samples)-1)]
            row.update(value if isinstance(value,dict) else dict(loaded=value))
        index+=1
        while len(rays)<6-row['loaded']:
            rays.append(dict(trace_valid='1',trace_source_consistent='1',write_source_consistent='1'))
        return row
    def records(prefix):
        if prefix=='native_lancer_shot_trajectory':return rays
        if reject and presses and prefix=='native_hammerburst_continuation_rejected':return [dict(count='5')]
        return []
    context=types.SimpleNamespace(g=types.SimpleNamespace(pad=pad,live=lambda:True),live=lambda:None,records=records)
    original=module.counts;module.counts=counts;error=None
    try:
        module.drain(context,'same_original',evidence=evidence,clock=clock,sleep=sleep)
    except AssertionError as caught:error=str(caught)
    finally:module.counts=original
    if expected_error is None:
        assert error is None,error
        assert evidence['complete'] and evidence['actual_ammo_spent']==6 and len(rays)==6
    else:assert error is not None and expected_error in error,(error,expected_error)
    assert presses==[.35] and releases==[.6,.5],(presses,releases)
    assert now<=18.5+1e-8,now  # Existing drain18s, followed by existing .5s cleanup.
    return dict(error=error,polls=len(sleeps),elapsed=now,evidence=evidence)

results=[]
results.append(check([1,0,0]))
results.append(check([1,1,0,0]))
results.append(check([0,0]))
results.append(check([1],reject=True,expected_error='explicitly rejected'))
assert results[-1]['polls']==0
results.append(check([1],expected_error='unchanged drain deadline'))
results.append(check([6],expected_error='did not spend'))
results.append(check([1,2],expected_error='Inventory increased'))
results.append(check([dict(loaded=1,reserve=77)],expected_error=''))
results.append(check([-1],expected_error='incomplete'))
assert results[0]['polls']==1 and results[1]['polls']==2
out=ROOT/'analysis/hammerburst-20261108/same-burst-completion-check.json'
out.write_text(json.dumps(dict(cases=len(results),passed=True,game_execution=False,
    unchanged_deadline_seconds=18,no_input_retry=True,
    results=[{k:v for k,v in x.items() if k!='evidence'} for x in results]),indent=2)+'\n',encoding='utf-8')
print(f'Same original burst completion: {len(results)} cases passed; no input retry or deadline renewal')
