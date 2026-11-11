"""Decode only complete, bounded timing bins from the exact matched query."""
from pathlib import Path
import importlib.util,json
H=Path(__file__).resolve().parent
STAGES=('physics_initial','collider_initial','components_initial','physics_repeat',
        'collider_repeat','components_repeat','engine','closing')
def decode(status,elapsed):
    if elapsed<0:raise ValueError('Negative query duration')
    invalid=int(status['cover_path_probe_timing_invalid']);overflow=int(status['cover_path_probe_timing_overflow'])
    if invalid or overflow:raise ValueError('Timing clock or accumulation invalid')
    rows=[]
    stages=STAGES+('owner_bracket',) if 'cover_path_probe_timing_8' in status else STAGES
    for index,name in enumerate(stages):
        values=[int(n) for n in status['cover_path_probe_timing_'+str(index)].split(',')]
        if len(values)!=4 or any(n<0 for n in values):raise ValueError('Malformed timing bin')
        calls,completed,total,maximum=values
        if calls!=completed or maximum>total or total>elapsed or (not calls and any(values)):
            raise ValueError('Incomplete or unbounded timing span')
        rows.append(dict(stage=name,calls=calls,closed_spans=completed,total_ns=total,max_ns=maximum,
            operation_success_proven=False))
    return rows
def inspect(report,expected_native):
    spec=importlib.util.spec_from_file_location('r163_cause',H.parent/'query-age-cause-r149/inspect_cause.py')
    reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
    query=reader.inspect(report,expected_native);raw=query['raw']
    if raw['attempted']!='1' or raw['finished']!='1':raise ValueError('No terminal query')
    elapsed=int(raw['end_ns'])-int(raw['begin_ns'])
    return dict(query=query,elapsed_ns=elapsed,rows=decode(report['native_status'],elapsed),
        nested_times_overlap=True,totals_must_not_be_added=True,clock_overhead_included=True,
        source_age_repaired=False,world_coverage_verified=False,movement_authorized=False)
def selftest():
    row={'cover_path_probe_timing_invalid':'0','cover_path_probe_timing_overflow':'0'}
    for i in range(8):row['cover_path_probe_timing_'+str(i)]='0,0,0,0'
    assert len(decode(row,100))==8
    good=dict(row,cover_path_probe_timing_0='2,2,70,40');assert decode(good,100)[0]['total_ns']==70
    extended=dict(row,cover_path_probe_timing_8='2,2,50,30')
    assert len(decode(extended,100))==9 and decode(extended,100)[8]['stage']=='owner_bracket'
    bad=[dict(row,cover_path_probe_timing_invalid='1'),dict(row,cover_path_probe_timing_overflow='1')]
    bad += [dict(row,cover_path_probe_timing_0=v) for v in ('1,0,10,10','1,1,101,101','1,1,10,11','0,0,1,1','-1,0,0,0','1,1,1')]
    for data in bad:
        try:decode(data,100);raise AssertionError('Invalid timing accepted')
        except ValueError:pass
    missing=dict(row);del missing['cover_path_probe_timing_7']
    try:decode(missing,100);raise AssertionError('Missing timing accepted')
    except KeyError:pass
    return dict(cases=12,failures=0,scope='Timing decoding only; no game')
if __name__=='__main__':print(json.dumps(selftest()))
