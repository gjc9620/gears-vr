"""Correlate construction, pre-consume and pre-destroy within one process/run."""
import argparse
import collections
import csv
import json
import math
import pathlib
import statistics


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=pathlib.Path,required=True)
    parser.add_argument('--restored',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    status=args.capture.with_suffix('.status.txt').read_text()
    frequency=int(next(s.split('=')[1] for s in status.splitlines() if s.startswith('qpc_frequency=')))
    restore=json.loads(args.restored.read_text())
    with args.capture.with_suffix('.csv').open(newline='') as stream:rows=sorted(csv.DictReader(stream),key=lambda r:int(r['qpc']))
    constructors={};consumers={};pairs=[];triples=[]
    for r in rows:
        if r['kind']=='9':
            constructors[r['object']]=r;consumers.pop(r['object'],None)
        elif r['kind']=='10':
            c=constructors.get(r['object']);consumers[r['object']]=(r,c)
            if c and 0<=int(r['qpc'])-int(c['qpc'])-int(c['duration_qpc'])<frequency:
                pairs.append((c,r))
        elif r['kind']=='11':
            item=consumers.get(r['object'])
            if item:
                u,c=item
                if c and 0<=int(u['qpc'])-int(c['qpc'])-int(c['duration_qpc'])<frequency and u['thread']==r['thread'] and int(u['qpc'])<=int(r['qpc'])<=int(u['qpc'])+int(u['duration_qpc']):triples.append((c,u,r))
            constructors.pop(r['object'],None);consumers.pop(r['object'],None)
    uses=[r for r in rows if r['kind']=='10'];deletes=[r for r in rows if r['kind']=='11']
    def intact(r):
        return int(r['valid'])&5760==5760 and r['family_count']=='1' and r['renderer_count']=='1' and r['family_view0']==r['renderer_storage'] and r['clone_owner']==r['family_address'] and int(r['family_address'],16)==int(r['object'],16)+8
    checks={
        'clean_stop':'stopped disable_status=0 active=0' in status,
        'entries_restored':restore.get('all_match') is True,
        'sufficient_observations':len(uses)>=100 and len(deletes)>=100,
        'uses_have_valid_owned_view':bool(uses) and all(intact(r) for r in uses),
        'pre_destroy_has_valid_owned_view':bool(deletes) and all(intact(r) for r in deletes),
        'construct_to_consume_correlated':bool(uses) and len(pairs)/len(uses)>.99,
        'consume_to_destroy_correlated':bool(deletes) and len(triples)/len(deletes)>.99,
        'view_identity_preserved':bool(pairs) and all(c['family_view0']==u['family_view0'] for c,u in pairs),
        'matrices_finite':bool(uses) and all(all(math.isfinite(float(r[f'matrix{i}'])) for i in range(32)) for r in uses+deletes),
        'no_view_edits':not any(int(r['valid'])&64 for r in rows),
        'present_success':sum(r['kind']=='3' for r in rows)>=100 and not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ('3','4')),
    }
    report={'passed':all(checks.values()),'checks':checks,'consume_calls':len(uses),'destroy_calls':len(deletes),'constructor_consume_pairs':len(pairs),'complete_triplets':len(triples),
        'constructor_to_consume_median_ms':statistics.median((int(u['qpc'])-int(c['qpc'])-int(c['duration_qpc']))*1000/frequency for c,u in pairs) if pairs else None,
        'consume_callers':dict(collections.Counter(r['caller_rva'] for r in uses)),
        'destroy_callers':dict(collections.Counter(r['caller_rva'] for r in deletes)),
        'construct_threads':dict(collections.Counter(r['thread'] for r in rows if r['kind']=='9')),
        'consume_threads':dict(collections.Counter(r['thread'] for r in uses)),
        'destroy_threads':dict(collections.Counter(r['thread'] for r in deletes)),
        'present_threads':dict(collections.Counter(r['thread'] for r in rows if r['kind']=='3')),
        'matrix_equal_construct_to_consume':sum(all(c[f'matrix{i}']==u[f'matrix{i}'] for i in range(32)) for c,u in pairs),
        'matrix_equal_consume_to_pre_destroy':sum(all(u[f'matrix{i}']==d[f'matrix{i}'] for i in range(32)) for c,u,d in triples),
        'changed_matrix_indices':{str(i):sum(u[f'matrix{i}']!=d[f'matrix{i}'] for c,u,d in triples) for i in range(32) if any(u[f'matrix{i}']!=d[f'matrix{i}'] for c,u,d in triples)},
        'consume_focus':dict(collections.Counter(r['focused'] for r in uses)),
        'constructor_matrix_variants':len({tuple(r[f'matrix{i}'] for i in range(32)) for r in rows if r['kind']=='9'}),
        'limitations':'Pre-call state and observed nesting only. No access after original consumer/destroy returns. Pointer identity is bounded by constructor and destroy events; does not prove all frames, general thread safety, GPU completion or stereo.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
