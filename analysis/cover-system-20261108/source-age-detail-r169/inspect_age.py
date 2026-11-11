"""Interpret existing classifier/current clocks; never infer a renewed source."""
from pathlib import Path
import importlib.util,json
H=Path(__file__).resolve().parent
FIELDS=('observed','tick','world','pawn','scene','stereo_sequence','controller_sequence','thread','verdict','latched',
        'now','metadata_begin','metadata_end','tracking_begin','tracking_end','stereo_received','controller_received','sync','capture_begin')
LIMITS={name:40000000 for name in ('tracking_begin','tracking_end','stereo_received','controller_received','sync')}
LIMITS['metadata_begin']=50000000
def decode(status,key,begin,end):
    actual=[int(x) for x in status['cover_path_probe_age_key'].split(',')]
    calls=[int(x) for x in status['cover_path_probe_age_calls'].split(',')]
    if len(actual)!=5 or len(calls)!=3 or any(x<0 for x in actual+calls) or calls[2]:raise ValueError('Malformed/overflow age observer')
    if actual!=key:raise ValueError('Age scope does not match query key')
    rows=[];cohort=None
    for slot in range(4):
        values=[int(x) for x in status['cover_path_probe_age_'+str(slot)].split(',')]
        if len(values)!=len(FIELDS) or any(x<0 for x in values[:10]):raise ValueError('Malformed age row')
        if not values[0]:
            if any(values):raise ValueError('Unobserved age payload')
            continue
        r=dict(zip(FIELDS,values));stage=slot%2
        if r['observed']!=1 or r['verdict'] not in (0,1,2) or r['latched'] not in (0,1) or not r['tick'] or not calls[stage]:raise ValueError('Invalid observed row')
        if [r['world'],r['pawn'],r['scene'],key[3],r['thread']]!=key:raise ValueError('Age source identity mismatch')
        if not begin<=r['now']<=end:raise ValueError('Age clock outside this query')
        if slot>=2 and r['verdict']!=1:raise ValueError('Failure row is not source_age_only')
        current=(r['tick'],r['stereo_sequence'],r['controller_sequence'])
        if cohort is not None and current!=cohort:raise ValueError('Mixed source cohorts')
        cohort=current;r.update(slot=slot,stage='classify' if not stage else 'current',age_verifiable=r['verdict']!=2)
        if r['age_verifiable']:
            if not 0<r['metadata_begin']<=r['metadata_end']<=r['tracking_begin']<=r['tracking_end']<=r['now']:raise ValueError('Invalid acquisition interval')
            if any(not 0<r[n]<=r['now'] for n in LIMITS):raise ValueError('Invalid source timestamp')
            if (not stage and r['capture_begin']) or (stage and r['capture_begin'] and not r['tracking_end']<=r['capture_begin']<=r['now']):raise ValueError('Invalid capture interval')
            r['ages_ns']={n:r['now']-r[n] for n in LIMITS}
            r['remaining_ns']={n:LIMITS[n]-r['ages_ns'][n] for n in LIMITS}
            r['expired_fields']=[n for n in LIMITS if r['ages_ns'][n]>LIMITS[n]]
            r['capture_age_ns']=r['now']-r['capture_begin'] if r['capture_begin'] else None
            r['capture_expired']=r['capture_age_ns']>40000000 if r['capture_age_ns'] is not None else None
            # Classify uses this exact recorded clock. Current has a later
            # clock; it must NOT retrospectively reclassify earlier input.
            if not stage and ((r['verdict']==1)!=bool(r['expired_fields'])):raise ValueError('Classifier/age mismatch')
        rows.append(r)
    by_slot={r['slot']:r for r in rows}
    for stage in range(2):
        if bool(calls[stage])!=(stage in by_slot):raise ValueError('Missing first observation')
        if stage+2 in by_slot and stage not in by_slot:raise ValueError('Failure without first observation')
    if 0 in by_slot and 1 in by_slot and by_slot[0]['now']>by_slot[1]['now']:raise ValueError('Reversed first clocks')
    return dict(observed=bool(rows),calls=calls[:2],rows=rows,scope='First actual check inside query; offset is not zero-time query entry',
        first_check_offset_ns=by_slot[0]['now']-begin if 0 in by_slot else None,
        classified_expired_fields=by_slot[2].get('expired_fields') if 2 in by_slot else None,
        missing_is_unknown=True,tracking_limit_ns=40000000,metadata_limit_ns=50000000)
def inspect(report,expected_native):
    spec=importlib.util.spec_from_file_location('r169_timing',H.parent/'query-timing-r163/inspect_timing.py')
    reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
    timing=reader.inspect(report,expected_native);raw=timing['query']['raw']
    key=[int(raw[k]) for k in ('world','pawn','scene','compose','thread')]
    age=decode(report['native_status'],key,int(raw['begin_ns']),int(raw['end_ns']))
    return dict(timing=timing,source_ages=age,source_age_repaired=False,movement_authorized=False)
def selftest():
    key=[11,22,33,44,55];base={'cover_path_probe_age_key':','.join(map(str,key)),'cover_path_probe_age_calls':'2,2,0'}
    first=[1,99,11,22,33,7,8,55,0,0,100000000,55000000,56000000,80000000,81000000,65000000,64000000,60000000,0]
    current=first.copy();current[-1]=90000000
    failed=first.copy();failed[8]=1;failed[10]+=1
    closed=failed.copy();closed[-1]=90000000;closed[10]+=1
    def packet(rows):return dict(base,**{'cover_path_probe_age_'+str(i):','.join(map(str,r)) for i,r in enumerate(rows)})
    data=packet([first,current,failed,closed]);v=decode(data,key,95000000,125000000)
    assert v['rows'][0]['remaining_ns']['sync']==0 and v['classified_expired_fields']==['sync'];checks=1
    blank=[0]*19;empty=packet([blank]*4);empty['cover_path_probe_age_calls']='0,0,0'
    assert not decode(empty,key,0,1)['observed'];checks+=1
    later=current.copy();later[10]+=1
    late_packet=packet([first,later,blank,blank]);late_packet['cover_path_probe_age_calls']='1,1,0'
    late=decode(late_packet,key,95000000,125000000)
    assert late['rows'][1]['verdict']==0 and late['rows'][1]['expired_fields']==['sync'];checks+=1
    latched=current.copy();latched[8]=1;latched[9]=1
    latch_packet=packet([first,latched,blank,latched]);latch_packet['cover_path_probe_age_calls']='1,1,0'
    latch=decode(latch_packet,key,95000000,125000000)
    assert latch['classified_expired_fields'] is None and latch['rows'][-1]['latched']==1;checks+=1
    invalid=[]
    for slot,index,value in ((0,2,12),(2,1,100),(0,10,94000000),(2,8,0),(0,17,59999999),(1,18,70000000),(0,0,0),(0,9,2)):
        rows=[r.copy() for r in (first,current,failed,closed)];rows[slot][index]=value;invalid.append(packet(rows))
    invalid.extend((dict(data,cover_path_probe_age_calls='2,2,1'),dict(data,cover_path_probe_age_key='11,22,33,45,55')))
    for bad in invalid:
        try:decode(bad,key,95000000,125000000);raise AssertionError('Invalid age evidence accepted')
        except ValueError:pass
        checks+=1
    missing=dict(data);del missing['cover_path_probe_age_3']
    try:decode(missing,key,95000000,125000000);raise AssertionError('Missing evidence accepted')
    except KeyError:pass
    checks+=1
    print(json.dumps(dict(cases=checks,failures=0,scope='Age interpretation only; no game')))
if __name__=='__main__':selftest()
