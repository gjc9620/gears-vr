"""Correlate native family submissions with preceding verified player views.

Missing records are reported, not manufactured into frame matches. Pointer
identity is scoped to this one process/capture; no addresses are reused.
"""
import argparse
import collections
import csv
import json
import pathlib
import statistics


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=pathlib.Path,required=True,help='collect_capture JSON stem')
    parser.add_argument('--restored',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    status=args.capture.with_suffix('.status.txt').read_text()
    frequency=int(next(x.split('=')[1] for x in status.splitlines() if x.startswith('qpc_frequency=')))
    rows=sorted(csv.DictReader(args.capture.with_suffix('.csv').open()),key=lambda r:int(r['qpc']))
    restore=json.loads(args.restored.read_text())
    last={};families=[];matched=[];unmatched=[]
    for row in rows:
        if row['kind']=='1' and int(row['valid'])&16 and row['result']!='0x0':
            last[(row['thread'],row['family_address'])]=row
        elif row['kind']=='7':
            families.append(row)
            previous=last.get((row['thread'],row['family_address']))
            pointers=[row[f'family_view{i}'] for i in range(max(0,min(4,int(row['family_count']))))]
            delay=(int(row['qpc'])-int(previous['qpc']))/frequency if previous else None
            if int(row['valid'])&128 and previous and previous['result'] in pointers and 0<=delay<.1:
                matched.append({'family_qpc':row['qpc'],'scene_qpc':previous['qpc'],'view':previous['result'],'delay_ms':delay*1000})
            else:unmatched.append({'qpc':row['qpc'],'family':row['family_address'],'valid':row['valid'],'count':row['family_count'],'capacity':row['family_capacity'],'preceding_scene':previous['result'] if previous else None})
    valid=sum(bool(int(r['valid'])&128) for r in families)
    checks={
        'family_mode': 'family_diagnostics=1 ' in status,
        'capture_stopped': 'stopped disable_status=0 active=0' in status,
        'entries_restored': restore.get('all_match') is True,
        'sufficient_submissions':len(families)>=100,
        'arrays_valid':bool(families) and valid==len(families),
        'same_view_correlated':bool(families) and len(matched)/len(families)>.99,
        'caller_matches':bool(families) and all(r['caller_rva']=='0x2c8909' for r in families),
        'no_view_edits':not any(int(r['valid'])&64 for r in rows),
        'present_success':not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ('3','4')),
    }
    report={'checks':checks,'passed':all(checks.values()),'family_calls':len(families),'valid_arrays':valid,
        'matched_preceding_player_views':len(matched),'unmatched_samples':unmatched[:20],
        'view_count_distribution':dict(collections.Counter(r['family_count'] for r in families)),
        'capacity_distribution':dict(collections.Counter(r['family_capacity'] for r in families)),
        'family_threads':dict(collections.Counter(r['thread'] for r in families)),
        'present_threads':dict(collections.Counter(r['thread'] for r in rows if r['kind']=='3')),
        'median_scene_to_submission_ms':statistics.median(x['delay_ms'] for x in matched) if matched else None,
        'limitations':'Same-process pointer correlation proves the observed submission received the preceding player view. It does not prove downstream lifetime, duplicate-view safety, stereo rendering or one-to-one Present correspondence.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
