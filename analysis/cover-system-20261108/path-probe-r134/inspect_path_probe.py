"""Interpret exact-artifact path control receipts; never authorize body motion."""
import copy,json,sys
from pathlib import Path
NATIVE='0098be93163f11c4f3007115d8f2154f484c9bf15338d59cbf5f440a048c37cb'
def inspect(report,expected_native=NATIVE):
    if report.get('diagnostics_artifacts',{}).get('native_sha256')!=expected_native:raise ValueError('Exact Native required')
    s=report['native_status'];game=report['game_identity']
    if int(s['pid'])!=game['pid']:raise ValueError('Game/Native mismatch')
    raw={k.removeprefix('cover_path_probe_'):v for k,v in s.items() if k.startswith('cover_path_probe_')}
    required=('requested','attempted','finished','code','segments','hits','reports','endpoint_shapes','endpoint_reports',
        'original_started_before','original_started_after','original_returned_before','original_returned_after',
        'session_entered','outer_stage','session_stage','session_detail','engine_stage','engine_source_code','engine_exception',
        'begin_ns','end_ns','diagnostic_geometry','body_geometry','movement_permission')
    missing=[k for k in required if k not in raw]
    out=dict(game=game,native_sha256=expected_native,raw=raw,missing=missing,queried_and_returned=False,
        collision_clear=None,body_geometry_verified=False,world_coverage_verified=False,movement_authorized=False)
    if missing:return out
    v={k:int(raw[k]) for k in required}
    if any(v[k]!=1 for k in ('requested','attempted','finished')):return out
    started=v['original_started_after']-v['original_started_before']
    returned=v['original_returned_after']-v['original_returned_before']
    if not 0<=returned<=started<=3:raise ValueError('Invalid one-shot counters')
    if v['end_ns']<v['begin_ns'] or v['begin_ns']<=0:raise ValueError('Invalid original read bracket')
    if v['diagnostic_geometry']!=1 or v['body_geometry'] or v['movement_permission']:raise ValueError('Wrong evidence scope')
    out.update(original_started=started,original_returned=returned)
    if v['code']==0:
        if not 1<=v['segments']<=2 or started!=v['segments']+1 or returned!=started:raise ValueError('Incomplete path calls')
        if not 0<=v['hits']+v['endpoint_shapes']<=32 or not 0<=v['reports']+v['endpoint_reports']<=8:raise ValueError('Invalid shared budget')
        for i in range(v['hits']):
            row=raw.get('sweep_'+str(i),'').split(',')
            if len(row)!=8 or len(row[0])!=128:raise ValueError('Missing raw sweep row')
            bytes.fromhex(row[0])
        for i in range(v['endpoint_shapes']):
            row=raw.get('endpoint_'+str(i),'').split(',')
            if len(row)!=8 or int(row[0])!=int(row[2]):raise ValueError('Missing endpoint shape/owner row')
        out['queried_and_returned']=True
    return out
def selftest():
    v=dict(requested=1,attempted=1,finished=1,code=0,segments=1,hits=0,reports=0,endpoint_shapes=0,endpoint_reports=0,
        original_started_before=0,original_started_after=2,original_returned_before=0,original_returned_after=2,
        session_entered=1,outer_stage=6,session_stage=7,session_detail=0,engine_stage=10,engine_source_code=0,engine_exception=0,
        begin_ns=10,end_ns=20,diagnostic_geometry=1,body_geometry=0,movement_permission=0)
    r=dict(game_identity=dict(pid=42),diagnostics_artifacts=dict(native_sha256=NATIVE),native_status={'pid':'42',**{'cover_path_probe_'+k:str(x) for k,x in v.items()}})
    assert inspect(r)['queried_and_returned'] and inspect(r)['collision_clear'] is None
    for key,value in [('pid','43'),('cover_path_probe_original_returned_after','3'),('cover_path_probe_original_started_after','1'),
        ('cover_path_probe_segments','0'),('cover_path_probe_body_geometry','1'),('cover_path_probe_end_ns','9'),('cover_path_probe_hits','1')]:
        bad=copy.deepcopy(r);bad['native_status'][key]=value
        try:inspect(bad);raise AssertionError('Invalid receipt accepted')
        except ValueError:pass
    bad=copy.deepcopy(r);bad['diagnostics_artifacts']['native_sha256']='other'
    try:inspect(bad);raise AssertionError('Wrong Native accepted')
    except ValueError:pass
    print(json.dumps(dict(cases=9,failures=0,scope='report interpretation, no game')))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    else:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig'))),indent=2))
