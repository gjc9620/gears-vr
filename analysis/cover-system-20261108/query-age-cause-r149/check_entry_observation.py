"""Exercise the actual transformed A block, including its release-on-failure."""
from pathlib import Path
import ast,json,textwrap
from entry_observation import instrument,NEEDLE,ADDED
H=Path(__file__).resolve().parent;R=H.parents[2]
source=(R/'analysis/solo-m1-20261028/cover_boundary_chain.py').read_text(encoding='utf-8')
changed=instrument(source);ast.parse(changed)
assert changed.replace(ADDED,'')==source
cases=1
for fail in (False,True):
    calls=[];report={}
    class Pad:
        ROOT=R
        def tap(self,key,duration):
            calls.append(('tap',key,duration))
            if fail:raise RuntimeError('original tap failure')
        def pad(self,value,wait):calls.append(('pad',value,wait))
    class Clock:
        def perf_counter_ns(self):calls.append(('clock',));return len(calls)
    class Context:pc=1;pawn=2
    def sample(*args):calls.append(('sample',));return {'cover_state_raw':{'CoverType':0}}
    def save():calls.append(('save',))
    env=dict(g=Pad(),report=report,time=Clock(),sample_playground=sample,observer=object(),ctx=Context(),save=save)
    try:exec(textwrap.dedent(ADDED+NEEDLE),env)
    except RuntimeError:
        assert fail
    assert calls==[('clock',),('sample',),('clock',),('save',),('tap','buttonA',1),('pad',0,.8)]
    assert report['before_original_A']['cover_state_raw']['CoverType']==0
    assert report['before_original_A_observation_end_ns']>report['before_original_A_observation_begin_ns']
    cases+=1
for bad in ('',source+NEEDLE,changed):
    try:instrument(bad);raise AssertionError('Unmatched/repeated source accepted')
    except ValueError:pass
    cases+=1
result=dict(cases=cases,failures=0,scope='Actual source transform and original A/finally block; no game, input or navigation acceptance')
(H/'entry-observation-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
