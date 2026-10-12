"""Correlate preparation before/after snapshots with enclosing consume lifetime."""
import argparse
import collections
import csv
import json
import math
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','restored','output'):parser.add_argument('--'+name,type=pathlib.Path,required=True)
    args=parser.parse_args()
    status=args.capture.with_suffix('.status.txt').read_text()
    restore=json.loads(args.restored.read_text())
    with args.capture.with_suffix('.csv').open(newline='') as stream:rows=sorted(csv.DictReader(stream),key=lambda r:int(r['qpc']))
    uses={};prepared={};before_pairs=[];after_pairs=[]
    for r in rows:
        key=(r['thread'],r['object'])
        if r['kind']=='10':uses[key]=r;prepared.pop(key,None)
        elif r['kind']=='12':
            u=uses.get(key)
            if u and int(u['qpc'])<=int(r['qpc']) and int(r['qpc'])+int(r['duration_qpc'])<=int(u['qpc'])+int(u['duration_qpc']):
                before_pairs.append((u,r));prepared[key]=r
        elif r['kind']=='11':
            p=prepared.get(key);u=uses.get(key)
            if p and u and int(p['qpc'])+int(p['duration_qpc'])<=int(r['qpc'])<=int(u['qpc'])+int(u['duration_qpc']):after_pairs.append((p,r))
            prepared.pop(key,None);uses.pop(key,None)
    items=[r for r in rows if r['kind']=='12']
    def same(a,prefix_a,b,prefix_b):return all(a[f'{prefix_a}{i}']==b[f'{prefix_b}{i}'] for i in range(32))
    checks={
        'clean_stop':'stopped disable_status=0 active=0' in status,
        'entries_restored':restore.get('all_match') is True,
        'sufficient_observations':len(items)>=100,
        'complete_reads':bool(items) and all(int(r['valid'])&30592==30592 for r in items),
        'single_owned_view':bool(items) and all(r['family_count']=='1' and r['renderer_count']=='1' and r['family_view0']==r['renderer_storage'] and r['clone_source']==r['family_view0'] and r['clone_owner']==r['family_address'] and int(r['family_address'],16)==int(r['object'],16)+8 for r in items),
        'finite_matrices':bool(items) and all(math.isfinite(float(r[f'{prefix}{i}'])) for r in items for prefix in ('matrix','source_matrix') for i in range(32)),
        'nested_consume':bool(items) and len(before_pairs)/len(items)>.99,
        'subsequent_destroy':bool(items) and len(after_pairs)/len(items)>.99,
        'no_view_edits':not any(int(r['valid'])&64 for r in rows),
        'present_success':sum(r['kind']=='3' for r in rows)>=100 and not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ('3','4')),
    }
    report={'passed':all(checks.values()),'checks':checks,'prepare_calls':len(items),'consume_prepare_pairs':len(before_pairs),'prepare_destroy_pairs':len(after_pairs),
        'callers':dict(collections.Counter(r['caller_rva'] for r in items)),
        'threads':dict(collections.Counter(r['thread'] for r in items)),
        'before_equals_consume':sum(same(u,'matrix',p,'source_matrix') for u,p in before_pairs),
        'after_equals_pre_destroy':sum(same(p,'matrix',d,'matrix') for p,d in after_pairs),
        'before_after_changed_indices':{str(i):sum(r[f'source_matrix{i}']!=r[f'matrix{i}'] for r in items) for i in range(32) if any(r[f'source_matrix{i}']!=r[f'matrix{i}'] for r in items)},
        'associated_pointer_pairs':dict(collections.Counter(r['renderer_source0']+' -> '+r['renderer_source1'] for r in items)),
        'focus':dict(collections.Counter(r['focused'] for r in items)),
        'limitations':'One observed view and bounded nested lifetime. Associated pointer type and field meanings are unproved; does not prove specific matrix writer, GPU completion or stereo.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
