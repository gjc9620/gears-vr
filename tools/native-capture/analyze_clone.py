"""Check observed renderer view copies without treating them as stereo proof."""
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
    copies=[r for r in rows if r['kind']=='8']
    checks={
        'stopped':'stopped disable_status=0 active=0' in status,
        'family_mode':'family_diagnostics=1 ' in status,
        'entries_restored':restore.get('all_match') is True,
        'sufficient_copies':len(copies)>=100,
        'reads_valid':bool(copies) and all(int(r['valid'])&(128|256|512|1024)==(128|256|512|1024) for r in copies),
        'expected_caller':bool(copies) and all(r['caller_rva']=='0x116444' for r in copies),
        'return_is_destination':bool(copies) and all(r['object']==r['result'] and r['result']!='0x0' for r in copies),
        'distinct_storage':bool(copies) and all(r['clone_source']!=r['object'] for r in copies),
        'source_in_submission':bool(copies) and all(r['clone_source'] in [r[f'family_view{i}'] for i in range(max(0,min(4,int(r['family_count']))))] for r in copies),
        'initial_owner_is_input_family':bool(copies) and all(r['clone_owner']==r['family_address'] for r in copies),
        'matrix_bytes_represented_equal':bool(copies) and all(all(r[f'matrix{i}']==r[f'source_matrix{i}'] for i in range(32)) for r in copies),
        'matrices_finite':bool(copies) and all(all(math.isfinite(float(r[f'matrix{i}'])) and math.isfinite(float(r[f'source_matrix{i}'])) for i in range(32)) for r in copies),
        'no_view_edits':not any(int(r['valid'])&64 for r in rows),
        'successful_presents':sum(r['kind']=='3' for r in rows)>=100 and not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ('3','4')),
    }
    report={'passed':all(checks.values()),'checks':checks,'copies':len(copies),
        'callers':dict(collections.Counter(r['caller_rva'] for r in copies)),
        'threads':dict(collections.Counter(r['thread'] for r in copies)),
        'unique_sources':len({r['clone_source'] for r in copies}),
        'unique_destinations':len({r['object'] for r in copies}),
        'matrix_variants':len({tuple(r[f'matrix{i}'] for i in range(32)) for r in copies}),
        'limitations':'Observed constructor return only. Later owner reassignment, render-thread consumption, destruction, second-eye state, projection conventions and stereo remain unverified. Equal serialized floats cover only the recorded 128-byte matrix region.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
