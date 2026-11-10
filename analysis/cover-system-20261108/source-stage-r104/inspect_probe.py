"""Interpret terminal probe facts; never turn an error/zero into collision safety."""
from pathlib import Path
import json,sys
OUTER=('idle','admission','cover','guard','alias','session','returned')
SESSION=('idle','geometry','physics_roles','collider_roles','component_roles','engine','closing','complete')
ENGINE=('idle','source_begin','code_begin','source_repeat','source_guard','dispatch','returned','source_end','code_end','guard_end','complete')
ROLES=('current','unavailable','request','guard','metadata','layout','read','changed','capacity','exception')
SOURCE=('current','request','guard','read','changed','layout','module')
def name(values,index):return values[index] if 0<=index<len(values) else 'unknown_'+str(index)
def inspect(report,expected_native):
    if report.get('diagnostics_artifacts',{}).get('native_sha256')!=expected_native:
        raise ValueError('Native artifact differs; enum/evidence interpretation refused')
    game=report['game_identity'];status=report['native_status']
    if int(status['pid'])!=game['pid']:raise ValueError('Native and game identity differ')
    raw={k.removeprefix('cover_sphere_probe_'):v for k,v in status.items() if k.startswith('cover_sphere_probe_')}
    out={'game':game,'raw':raw,'original_started':None,'original_returned':None,
         'first_rejection_layer':'unproven','collision_clear':None,'movement_authorized':False}
    required=('outer_stage','session_entered','session_stage','session_detail','engine_observed','engine_stage',
              'engine_source_code','engine_exception','original_started_before','original_started_after',
              'original_returned_before','original_returned_after')
    missing=[k for k in required if k not in raw]
    if missing:out['missing_observation']=missing;return out
    if any(raw.get(k)!='1' for k in ('requested','attempted','finished')):
        out['missing_observation']=['completed opt-in attempt'];return out
    x={k:int(raw[k]) for k in required}
    out['outer_stage']=name(OUTER,x['outer_stage']);out['session_stage']=name(SESSION,x['session_stage'])
    out['engine_stage']=name(ENGINE,x['engine_stage']);out['engine_exception']=bool(x['engine_exception'])
    if x['engine_observed']==1:
        started=x['original_started_after']-x['original_started_before']
        returned=x['original_returned_after']-x['original_returned_before']
        if not 0<=returned<=started<=1:raise ValueError('One-shot counters inconsistent')
        out.update(original_started=started,original_returned=returned)
    code=int(raw['code']);out['code']=code
    if code!=0:
        if not x['session_entered']:out['first_rejection_layer']='c0/'+out['outer_stage']
        elif x['session_stage'] in (2,3,4):
            out['first_rejection_layer']='typed/'+out['session_stage']+'/'+name(ROLES,x['session_detail'])
        elif x['session_stage']==5:
            out['first_rejection_layer']='engine/'+out['engine_stage']+'/'+name(SOURCE,x['engine_source_code'])
        else:out['first_rejection_layer']='session/'+out['session_stage']
    out['queried_and_returned']=(code==0 and out['original_started']==1 and out['original_returned']==1)
    # Raw hits/misses retain their original scope; no full inventory/world or
    # endpoint certificate is manufactured by this interpreter.
    return out
def selftest():
    import copy
    native='test-artifact';s={'pid':'1'}
    fields={'requested':'1','attempted':'1','finished':'1','code':'4','raw_hit':'0',
        'outer_stage':'6','session_entered':'1','session_stage':'5','session_detail':'4',
        'engine_observed':'1','engine_stage':'1','engine_source_code':'6','engine_exception':'0',
        'original_started_before':'0','original_started_after':'0','original_returned_before':'0','original_returned_after':'0'}
    s.update({'cover_sphere_probe_'+k:v for k,v in fields.items()})
    r={'game_identity':{'pid':1},'native_status':s,'diagnostics_artifacts':{'native_sha256':native}}
    a=inspect(r,native);assert a['original_started']==0 and a['first_rejection_layer']=='engine/source_begin/module'
    b=copy.deepcopy(r);b['native_status'].update({'cover_sphere_probe_engine_stage':'7','cover_sphere_probe_engine_exception':'1',
        'cover_sphere_probe_engine_source_code':'0','cover_sphere_probe_original_started_after':'1','cover_sphere_probe_original_returned_after':'1'})
    a=inspect(b,native);assert a['original_returned']==1 and a['code']==4 and a['collision_clear'] is None
    c=copy.deepcopy(r);c['native_status'].update({'cover_sphere_probe_session_stage':'2','cover_sphere_probe_session_detail':'6'})
    assert inspect(c,native)['first_rejection_layer']=='typed/physics_roles/read'
    d=copy.deepcopy(r);del d['native_status']['cover_sphere_probe_engine_observed']
    assert inspect(d,native)['original_started'] is None
    e=copy.deepcopy(r);e['native_status']['cover_sphere_probe_original_returned_after']='1'
    try:inspect(e,native);raise AssertionError('counter inconsistency accepted')
    except ValueError:pass
    try:inspect(r,'different-artifact');raise AssertionError('wrong provenance accepted')
    except ValueError:pass
    print(json.dumps({'cases':6,'failures':0,'game_started':False,'scope':'Synthetic report interpretation only'}))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    elif len(sys.argv)==3:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig')),sys.argv[2]),indent=2))
    else:raise SystemExit('Use --selftest or exact-report.json expected-native-sha256')
