"""Check the one-shot native source-copy result against clean capture shutdown."""
import argparse
import json
import pathlib
import re


def analyze(status, restored):
    lines=[line for line in status.splitlines() if line.startswith('shadow_view_enabled=')]
    if len(lines)!=1:raise ValueError('Exactly one shadow diagnostic summary required')
    fields=dict(re.findall(r'(\w+)=(0x[0-9a-fA-F]+|\d+)',lines[0]))
    values={key:int(value,0) for key,value in fields.items()}
    required={'shadow_view_enabled','attempted','stage','checks','nonempty_arrays','source','temporary'}
    if set(values)!=required:raise ValueError('Unexpected diagnostic fields')
    match=re.search(r'^capturing pid=(\d+) ',status,re.MULTILINE)
    checks={
        'clean_shutdown':'stopped disable_status=0 active=0' in status,
        'hooks_restored':restored.get('all_match') is True,
        'same_process':bool(match and int(match.group(1))==restored.get('pid')),
        'enabled':values['shadow_view_enabled']==1,
        'one_attempt_completed':values['attempted']==2,
        'successful_native_copy':values['stage']==6,
        'all_eight_checks':values['checks']==255,
        'distinct_source_and_copy':bool(values['source'] and values['temporary'] and values['source']!=values['temporary']),
    }
    return {'checks':checks,'passed':all(checks.values()),'diagnostic':values,
        'limits':['One unsubmitted copy, not a second rendered eye.',
            'Nonempty linked map rejected; only four known array containers checked.',
            'Shared view-state pointer retained; independent per-eye history not demonstrated.',
            'Zero nonempty arrays does not establish dynamic coverage of populated containers.']}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','restored','output'):p.add_argument('--'+name,type=pathlib.Path,required=True)
    a=p.parse_args()
    report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()))
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
