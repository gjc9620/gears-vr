"""Validate two owned views in one unsubmitted native renderer."""
import argparse
import json
import pathlib
import re
from analyze_constructor_eye import analyze as eye


def analyze(status,restored,csv_path):
    inputs=eye(status,restored,csv_path)
    lines=[s for s in status.splitlines() if s.startswith('renderer_pair_enabled=')]
    if len(lines)!=1:raise ValueError('Exactly one renderer-pair summary required')
    match=re.fullmatch(r'renderer_pair_enabled=(\d+) checks=(\d+) renderer=(0x[0-9a-fA-F]+) retained=(\d+)',lines[0])
    if not match:raise ValueError('Malformed renderer-pair summary')
    enabled,mask,root,retained=[int(value,0) for value in match.groups()]
    checks={'source_view_and_state_checks':inputs['passed'],'pair_enabled':enabled==1,
            'all_eight_pair_checks':mask==255,'recognized_root_created':root!=0,
            'no_quarantined_inputs':retained==0}
    names=('input_pair_distinct_states','allocated','constructor_returned_destination',
           'two_owned_arrays','owner_state_index_links','both_matrix_inputs_preserved',
           'native_destroy_returned','original_family_preserved')
    return {'passed':all(checks.values()),'checks':checks,'pair':{'root':hex(root),'mask':mask,
            'retained':retained,'details':{name:bool(mask&(1<<i)) for i,name in enumerate(names)}},
            'inputs':inputs,'limits':['One unsubmitted renderer, never queued or consumed.',
                'First view uses original state; only added view has independent state.',
                'Two owned copies and cleanup do not prove two-view rendering, GPU completion or headset stereo.']}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','restored','output'):p.add_argument('--'+name,type=pathlib.Path,required=True)
    a=p.parse_args();report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()),a.capture.with_suffix('.csv'))
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
