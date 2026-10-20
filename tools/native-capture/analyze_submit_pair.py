"""Separate completed single-frame submission from a safely rejected trial."""
import argparse
import csv
import json
import pathlib
import re


def fields(status, prefix):
    lines=[line for line in status.splitlines() if line.startswith(prefix)]
    if len(lines)!=1:raise ValueError('Exactly one '+prefix+' summary required')
    return {key:int(value,0) for key,value in re.findall(r'(\w+)=(0x[0-9a-fA-F]+|\d+)',lines[0])}


def analyze(status, restored, csv_path):
    trial=fields(status,'submit_pair_enabled=')
    state=fields(status,'shadow_state_enabled=')
    pid=re.search(r'^capturing pid=(\d+) ',status,re.M)
    with csv_path.open(newline='') as stream:
        rows=list(csv.DictReader(stream))
    presents=[r for r in rows if r['kind'] in ('3','4')]
    checks={'same_process':bool(pid and int(pid[1])==restored.get('pid')),
        'clean_stop':'stopped disable_status=0 active=0' in status,
        'entries_restored':restored.get('all_match') is True,
        'enabled':trial.get('submit_pair_enabled')==1,
        'state_released':state.get('created')==1 and state.get('release_returned')==1 and
            state.get('destroyed')==1 and state.get('flags',0)&1==1,
        'original_source_preserved':state.get('original_preserved')==1,
        'present_success':len(presents)>=100 and all(int(r['result'],16)&0x80000000==0 for r in presents)}
    root=trial.get('root',0)
    constructors=[r for r in rows if r['kind']=='9' and int(r['object'],16)==root and r['family_count']=='2']
    events=[]
    if len(constructors)==1:
        first=int(constructors[0]['qpc'])
        # Heap addresses may be reused after destruction; stop at the first destroy.
        events=sorted((r for r in rows if int(r['object'],16)==root and int(r['qpc'])>=first and
            r['kind'] in ('10','11','12')),key=lambda r:int(r['qpc']))
        for i,row in enumerate(events):
            if row['kind']=='11':events=events[:i+1];break
    submitted={'completed':trial.get('attempted')==2 and root!=0,
        'owned_copies_and_sources_released':trial.get('owned')==1 and trial.get('sources_released')==1,
        'original_family_preserved':trial.get('original_preserved')==1,
        'native_lifecycle_counters':all(trial.get(name,0)>=1 for name in ('consumed','prepared','destroyed')),
        'one_pair_constructor_record':len(constructors)==1 and constructors[0]['renderer_count']=='2',
        'pair_consume_prepare_destroy_records':all(any(r['kind']==kind and r['family_count']=='2'
            for r in events) for kind in ('10','12','11')),
        'ordinary_frames_resume':bool(events and any(r['kind']=='9' and r['family_count']=='1' and
            int(r['qpc'])>int(events[-1]['qpc']) for r in rows))}
    return {'passed':all(checks.values()) and all(submitted.values()),
        'safe_rejection':all(checks.values()) and trial.get('attempted')==3 and root==0 and
            all(trial.get(name)==0 for name in ('consumed','prepared','destroyed')),
        'checks':checks,'submission_checks':submitted,'trial':trial,'state':state,
        'present_count':len(presents),'limits':['Native lifecycle records do not verify visible pixels or headset stereo.',
            'One frame with synthetic offsets and original projections; no sustained rendering claim.']}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('capture','restored','output'):parser.add_argument('--'+key,type=pathlib.Path,required=True)
    a=parser.parse_args()
    report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()),a.capture.with_suffix('.csv'))
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
