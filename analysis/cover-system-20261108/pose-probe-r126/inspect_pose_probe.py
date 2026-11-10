"""Interpret bounded measurements only against the exact r126 Native artifact."""
from pathlib import Path
import json,sys
NATIVE='d7cb104d3dcf7e77dfa127f28064d1b3cfbb3df2c6a8f16d416fee2c9c88c2e5'
STEPS=('stopping','idle','current_before','key_before','type','slot','link','marker','measurement','key_after','same_key_after','current_after','stopping_after','revision_after','entry','complete','exception')
MEASURE=('okay','alias','core','units','neutral','policy','view','physical','head_marker','numerical','cover')
def inspect(report):
    if report.get('diagnostics_artifacts',{}).get('native_sha256')!=NATIVE:raise ValueError('Exact diagnostic Native required')
    status=report['native_status'];game=report['game_identity']
    if int(status['pid'])!=game['pid']:raise ValueError('Game/Native PID mismatch')
    rows=[];missing=[]
    for n,name in enumerate(STEPS):
        key='cover_pose_probe_count_'+str(n)
        if key not in status:missing.append(key);continue
        count=int(status[key]);first=status.get('cover_pose_probe_first_'+str(n))
        if count<0:raise ValueError('Negative count')
        if first is None:
            if count:missing.append('cover_pose_probe_first_'+str(n))
            continue
        if not count:raise ValueError('Witness with zero occurrences')
        values=[int(x) for x in first.split(',')]
        if len(values)!=6 or any(v<0 for v in values):raise ValueError('Malformed first witness')
        detail,scene,compose,sequence,world,pawn=values
        row=dict(step=n,name=name,count=count,detail=detail,scene=scene,compose=compose,sequence=sequence,world=world,pawn=pawn)
        if name=='measurement':row['measurement_status']=MEASURE[detail] if detail<len(MEASURE) else 'unknown_'+str(detail)
        rows.append(row)
    # Different gates may refer to different C0s. Do not combine them into a
    # fictional single transaction or infer that a count is the first failure.
    return dict(game=game,native_sha256=NATIVE,rows=rows,missing=missing,
        complete_gate_observed=any(r['name']=='complete' for r in rows),
        query_attempted=status.get('cover_sphere_probe_attempted')=='1',
        collision_clear=None,movement_authorized=False,same_C0_across_rows=False)
def selftest():
    import copy
    s={'pid':'42',**{'cover_pose_probe_count_'+str(n):'0' for n in range(len(STEPS))}}
    d=dict(game_identity=dict(pid=42),native_status=s,diagnostics_artifacts=dict(native_sha256=NATIVE))
    assert not inspect(d)['complete_gate_observed'] and not inspect(d)['missing']
    s.update(cover_pose_probe_count_8='7',cover_pose_probe_first_8='2,9,10,11,12,13')
    p=inspect(d);assert p['rows'][0]['measurement_status']=='core' and p['collision_clear'] is None
    for change in ('sha','pid','zero','format'):
        bad=copy.deepcopy(d)
        if change=='sha':bad['diagnostics_artifacts']['native_sha256']='other'
        if change=='pid':bad['native_status']['pid']='43'
        if change=='zero':bad['native_status']['cover_pose_probe_count_8']='0'
        if change=='format':bad['native_status']['cover_pose_probe_first_8']='2,9'
        try:inspect(bad);raise AssertionError('Invalid report accepted')
        except ValueError:pass
    del s['cover_pose_probe_first_8'];assert inspect(d)['missing']
    print(json.dumps(dict(cases=7,failures=0,scope='report interpretation only; no game')))
if __name__=='__main__':
    if sys.argv[1:]==['--selftest']:selftest()
    else:print(json.dumps(inspect(json.loads(Path(sys.argv[1]).read_text(encoding='utf-8-sig'))),indent=2))
