"""Exercise the actual new entry's failure logic without a real runtime lock.

All receipts stay in new offline directories. No game/stage/body/input calls.
"""
from pathlib import Path
import importlib.util,json,sys,types,time
H=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('sphere_entry_guard_check',H/'run_cover_eye_independent.py')
entry=importlib.util.module_from_spec(spec);spec.loader.exec_module(entry)
folder=H/('offline-guard-errors-'+str(time.time_ns()));folder.mkdir()
checks=0
class Failure(RuntimeError):pass
class WaitObserved(BaseException):pass
class Busy(RuntimeError):pass
shim=types.ModuleType('runtime_stage_guard')
original=sys.modules.get('runtime_stage_guard');sys.modules['runtime_stage_guard']=shim
old_argv=sys.argv[:];old_sleep=entry.time.sleep
results=[]
try:
    for name,claimed,restored,busy in [('before_claim',False,False,False),('restored_failure',True,True,False),('uncertain_restore',True,False,False),('guard_busy',False,False,True)]:
        root=folder/name;(root/'runtime').mkdir(parents=True)
        if claimed:(root/'runtime/claim.json').write_text('{}')
        if restored:(root/'runtime/result.json').write_text(json.dumps({'baseline_restored':True}))
        state={'entered':False,'released':False,'called':False,'waited':False}
        class Guard:
            def __init__(self,instance):assert instance=='solo-m1-1028d'
            def __enter__(self):
                if busy:raise Busy()
                state['entered']=True;return self
            def __exit__(self,*args):state['released']=True
        def owned(plan):state['called']=True;raise Failure('synthetic restoration/entry error')
        def observe_wait(seconds):
            assert seconds==1 and state['entered'] and not state['released']
            state['waited']=True;raise WaitObserved()
        shim.RuntimeGuard=Guard;entry.HERE=root;entry.static_check=lambda:({},{});entry.run_owned=owned
        entry.time.sleep=observe_wait;sys.argv=[str(H/'run_cover_eye_independent.py'),'--run']
        kind=''
        try:entry.main()
        except WaitObserved:kind='held_at_wait'
        except Failure:kind='raised'
        except Busy:kind='busy'
        if name=='uncertain_restore':
            assert kind=='held_at_wait' and state['called'] and state['waited']
            receipt=json.loads((root/'runtime/guard-held-error.json').read_text())
            assert receipt['guard_held'] and not receipt['restoration_proven'] and not receipt['automatic_restart']
            checks+=2
        elif busy:
            assert kind=='busy' and not any(state.values());checks+=1
        else:
            assert kind=='raised' and state['called'] and state['released'] and not state['waited'];checks+=1
            assert not (root/'runtime/guard-held-error.json').exists();checks+=1
        results.append({'case':name,'observation':kind,**state})
finally:
    entry.time.sleep=old_sleep;sys.argv=old_argv
    if original is None:sys.modules.pop('runtime_stage_guard',None)
    else:sys.modules['runtime_stage_guard']=original
(folder/'result.json').write_text(json.dumps({'checks':checks,'cases':results,'real_runtime_guard_used':False,'game_started':False,'stage_performed':False},indent=2)+'\n')
print(json.dumps({'checks':checks,'failures':0,'result':str(folder/'result.json'),'scope':'Actual entry error logic with explicit fake guard and injected wait boundary; no real lock/game/stage.'}))
