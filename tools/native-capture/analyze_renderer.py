"""Correlate completed renderer construction with its nested view copies."""
import argparse
import collections
import csv
import json
import math
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=pathlib.Path,required=True)
    parser.add_argument('--restored',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    status=args.capture.with_suffix('.status.txt').read_text()
    restore=json.loads(args.restored.read_text())
    with args.capture.with_suffix('.csv').open(newline='') as stream:rows=list(csv.DictReader(stream))
    constructors=[r for r in rows if r['kind']=='9']
    nested=collections.defaultdict(list)
    for row in rows:
        if row['kind']=='8':nested[(row['thread'],row['object'])].append(row)
    matches=0
    for r in constructors:
        start=int(r['qpc']);end=start+int(r['duration_qpc'])
        if any(start<=int(c['qpc'])<=end and c['clone_source']==r['renderer_source0'] and c['family_address']==r['renderer_input_family'] and c['clone_owner']==r['renderer_input_family'] for c in nested[(r['thread'],r['family_view0'])]):matches+=1
    checks={
        'stopped':'stopped disable_status=0 active=0' in status,
        'family_mode':'family_diagnostics=1 ' in status,
        'entries_restored':restore.get('all_match') is True,
        'sufficient_constructions':len(constructors)>=100,
        'reads_valid':bool(constructors) and all(int(r['valid'])&8064==8064 for r in constructors),
        'caller_matches':bool(constructors) and all(r['caller_rva']=='0x3a1595' for r in constructors),
        'return_is_destination':bool(constructors) and all(r['object']==r['result'] and r['result']!='0x0' for r in constructors),
        'family_embedded_at_plus8':bool(constructors) and all(int(r['family_address'],16)==int(r['object'],16)+8 for r in constructors),
        'owner_reassigned':bool(constructors) and all(r['clone_owner']==r['family_address'] and r['clone_owner']!=r['renderer_input_family'] for r in constructors),
        'single_view_scope':bool(constructors) and all(r['family_count']=='1' and r['renderer_count']=='1' for r in constructors),
        'view_points_to_owned_storage':bool(constructors) and all(r['family_view0']==r['renderer_storage'] for r in constructors),
        'view_is_distinct_from_source':bool(constructors) and all(r['family_view0']!=r['renderer_source0'] for r in constructors),
        'recorded_matrices_equal':bool(constructors) and all(all(r[f'matrix{i}']==r[f'source_matrix{i}'] for i in range(32)) for r in constructors),
        'matrices_finite':bool(constructors) and all(all(math.isfinite(float(r[f'matrix{i}'])) for i in range(32)) for r in constructors),
        'nested_copies_correlated':bool(constructors) and matches/len(constructors)>.99,
        'no_view_edits':not any(int(r['valid'])&64 for r in rows),
        'presents_successful':sum(r['kind']=='3' for r in rows)>=100 and not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ('3','4')),
    }
    report={'passed':all(checks.values()),'checks':checks,'constructions':len(constructors),'nested_copy_matches':matches,
        'callers':dict(collections.Counter(r['caller_rva'] for r in constructors)),
        'threads':dict(collections.Counter(r['thread'] for r in constructors)),
        'view_counts':dict(collections.Counter(r['family_count'] for r in constructors)),
        'storage_capacities':dict(collections.Counter(r['renderer_capacity'] for r in constructors)),
        'matrix_variants':len({tuple(r[f'matrix{i}'] for i in range(32)) for r in constructors}),
        'limitations':'Constructor return within observed single-player single-view path. Does not prove downstream rendering, lifetime after submission, safe second-eye creation, multi-view behavior, matrix conventions or stereo.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
