"""Check synthetic unsubmitted eye geometry and independent state cleanup."""
import argparse
import json
import math
import pathlib
import re
from analyze_shadow_view import analyze as view
from analyze_shadow_state import analyze as state


def analyze(status, restored, csv_path):
    resource=view(status,restored)
    history=state(status,restored,csv_path)
    lines=[s for s in status.splitlines() if s.startswith('constructor_eye_enabled=')]
    if len(lines)!=1:raise ValueError('Exactly one eye summary required')
    match=re.fullmatch(r'constructor_eye_enabled=(\d+) shifted=(\d+) combined_valid=(\d+) combined_relative_error=(\S+)',lines[0])
    if not match:raise ValueError('Malformed eye summary')
    enabled,shifted,valid=map(int,match.group(1,2,3));error=float(match.group(4))
    derived=re.search(r'^constructor_shadow_enabled=(\d+) derived_finite=(\d+) derived_max_error=(\S+)$',status,re.MULTILINE)
    change=float(derived.group(3)) if derived else float('nan')
    checks={'source_and_family_preserved_cleanup_passed':resource['passed'],
            'independent_state_lifecycle_passed':history['passed'],
            'eye_enabled_and_shifted':enabled==1 and shifted==1,
            'independent_full_constructor_mode':'constructor_state_enabled=1' in status and bool(derived and derived.group(1)=='1'),
            'derived_finite':bool(derived and derived.group(2)=='1'),
            'derived_data_changed':math.isfinite(change) and change>0,
            'combined_matrix_matches_view_times_projection':valid==1 and math.isfinite(error) and 0<=error<1e-5}
    return {'passed':all(checks.values()),'checks':checks,'resources':resource['diagnostic'],'state':history,
            'combined_relative_error':error,'derived_max_change':change,
            'limits':['One synthetic one-game-unit right shift, unsubmitted; not physical IPD or stereo rendering.',
                      'In this mode bit 4 compares requested shifted inputs; bit 16 checks finite first-two array coefficients and independent storage, not original coefficient equality.',
                      'Only one combination matrix numerically checked; finite inverse/other blocks do not prove their mathematical accuracy.',
                      'No GPU use of independent history or OpenXR asymmetric projection validated.']}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('capture','restored','output'):p.add_argument('--'+name,type=pathlib.Path,required=True)
    a=p.parse_args();report=analyze(a.capture.with_suffix('.status.txt').read_text(),json.loads(a.restored.read_text()),a.capture.with_suffix('.csv'))
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
