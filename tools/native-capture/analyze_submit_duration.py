"""Validate bounded sustained pairs without confusing reused heap addresses."""
import argparse
import csv
import json
import pathlib
import re
from analyze_submit_pair import fields


def analyze(status,restored,csv_path):
    lifetime=fields(status,'submit_lifetime seconds=')
    trial=fields(status,'submit_pair_enabled=');state=fields(status,'shadow_state_enabled=')
    mode=fields(status,'submit_view_count=') if 'submit_view_count=' in status else {'submit_view_count':2}
    view_count=mode.get('submit_view_count');marker=mode.get('submit_event_marker')
    total=lifetime.get('total',0)
    seconds=lifetime.get('seconds',0)
    bounded_policy=1<=seconds<=60 or (seconds==120 and lifetime.get('trial_seconds')==120 and lifetime.get('validated_union')==1)
    counts={kind:0 for kind in ('9','10','11','12')}
    first_constructor=None;last_constructor=0;last_pair=0;last_single=0
    focused=0;present_count=0;present_failures=0
    # Iterate records directly: long trials must not materialize the CSV.
    with csv_path.open(newline='') as stream:
        for row in csv.DictReader(stream):
            kind=row['kind'];time=int(row['qpc'])
            if kind in ('3','4'):
                present_count+=1;present_failures+=bool(int(row['result'],16)&0x80000000)
            marked=marker is None or row.get('test_state')==str(marker)
            if row['family_count']==str(view_count) and kind in counts and marked:
                counts[kind]+=1;last_pair=max(last_pair,time)
                if kind=='9':
                    first_constructor=time if first_constructor is None else min(first_constructor,time)
                    last_constructor=max(last_constructor,time);focused+=row.get('focused')=='1'
            if kind=='9' and row['family_count']=='1' and (marker is None or not marked):last_single=max(last_single,time)
    frequency=int(re.search(r'^qpc_frequency=(\d+)$',status,re.M)[1])
    span=(last_constructor-first_constructor)/frequency if first_constructor is not None else 0.
    pid=re.search(r'^capturing pid=(\d+) ',status,re.M)
    checks={'same_process':bool(pid and int(pid[1])==restored.get('pid')),
        'clean_stop':'stopped disable_status=0 active=0' in status,
        'entries_restored':restored.get('all_match') is True,'enabled':trial.get('submit_pair_enabled')==1,
        'state_released':state.get('created')==1 and state.get('release_returned')==1 and state.get('destroyed')==1 and state.get('flags',0)&1==1,
        'original_source_preserved':state.get('original_preserved')==1,
        'present_success':present_count>=100 and present_failures==0}
    checks['recognized_trial_mode']=view_count in (1,2) and (view_count==2 or marker==77)
    checks.update({'completed_submission':trial.get('attempted')==2 and all(trial.get(key)==1 for key in ('owned','sources_released','original_preserved')),
        'bounded_duration':bounded_policy and seconds-.5<=span<=seconds+.2,
        'multiple_pairs':total>=10,
        'all_submitted_objects_retired':all(trial.get(key)==total for key in ('consumed','prepared','destroyed')),
        'production_closed_and_pool_empty':lifetime.get('closed')==1 and lifetime.get('pending')==0,
        'no_retained_or_skipped_objects':lifetime.get('retained')==0 and lifetime.get('skipped')==0,
        'csv_lifecycle_coverage':total>0 and all(.95*total<=count<=total for count in counts.values()),
        'single_view_resumed':last_pair>0 and last_single>last_pair})
    return {'passed':all(checks.values()),'checks':checks,'lifetime':lifetime,'trial':trial,
        'mode':mode,
        'csv_pair_events':counts,'pair_constructor_span_seconds':span,'state':state,
        'focused_pair_constructors':focused,
        'present_count':present_count,
        'limits':['Counters cover tracked native objects; CSV coverage tolerates at most five percent missing events.',
            'Visible pixels and headset stereo require separate observation; synthetic offsets and original projection.']}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('capture','restored','output'):parser.add_argument('--'+key,type=pathlib.Path,required=True)
    a=parser.parse_args();report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()),a.capture.with_suffix('.csv'))
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
