"""Validate one unsubmitted state factory/release/deleting-wrapper observation."""
import argparse
import csv
import json
import pathlib
import re


def analyze(status, restored, csv_path):
    lines=[s for s in status.splitlines() if s.startswith('shadow_state_enabled=')]
    if len(lines)!=1:raise RuntimeError('One state diagnostic summary required')
    fields={key:int(value,0) for key,value in re.findall(r'(\w+)=(0x[0-9a-fA-F]+|\d+)',lines[0])}
    required={'shadow_state_enabled','attempted','created','release_returned','destroyed','original_preserved','flags','thread','caller','temporary'}
    if set(fields)!=required:raise RuntimeError('Unexpected state diagnostic fields')
    pid=re.search(r'^capturing pid=(\d+) ',status,re.MULTILINE)
    family_threads=set();present_threads=set();present_count=0;present_failures=0
    with csv_path.open(newline='') as stream:
        for row in csv.DictReader(stream):
            if row['kind']=='7':family_threads.add(int(row['thread']))
            if row['kind'] in ('3','4'):
                present_threads.add(int(row['thread']));present_count+=1
                if int(row['result'],16)&0x80000000:present_failures+=1
    checks={'same_process':bool(pid and int(pid.group(1))==restored.get('pid')),
        'clean_stop':'stopped disable_status=0 active=0' in status,
        'entries_restored':restored.get('all_match') is True,
        'enabled':fields['shadow_state_enabled']==1,'attempted_once':fields['attempted']==1,
        'recognized_independent_state_created':fields['created']==1 and fields['temporary']!=0,
        'ordinary_release_returned':fields['release_returned']==1,
        'deleting_wrapper_completed':fields['destroyed']==1 and bool(fields['flags']&1),
        'original_state_pointer_preserved':fields['original_preserved']==1,
        'present_success':present_count>=100 and present_failures==0}
    report={'passed':all(checks.values()),'checks':checks,'diagnostic':fields,
        'family_threads':sorted(family_threads),'present_threads':sorted(present_threads),
        'delete_on_observed_family_thread':fields['thread'] in family_threads,
        'delete_on_observed_present_thread':fields['thread'] in present_threads,
        'present_count':present_count,'present_failures':present_failures,
        'limits':['One state created and released without rendering it.',
            'Deleting wrapper completion and flags are observed; freed storage is not reread.',
            'No per-eye GPU history use, long-session stability, or general GPU completion claim.']}
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('capture','restored','output'):p.add_argument('--'+key,type=pathlib.Path,required=True)
    a=p.parse_args()
    report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()),a.capture.with_suffix('.csv'))
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
