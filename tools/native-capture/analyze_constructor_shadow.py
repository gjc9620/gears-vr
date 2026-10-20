"""Validate one unsubmitted native full-constructor reconstruction."""
import argparse
import json
import math
import pathlib
import re
from analyze_shadow_view import analyze as analyze_shadow


def analyze(status, restored, independent=False):
    report = analyze_shadow(status, restored)
    lines = [s for s in status.splitlines() if s.startswith('constructor_shadow_enabled=')]
    if len(lines) != 1:
        raise ValueError('Exactly one full-constructor summary required')
    match = re.fullmatch(r'constructor_shadow_enabled=(\d+) derived_finite=(\d+) derived_max_error=(\S+)', lines[0])
    if not match:
        raise ValueError('Malformed full-constructor summary')
    enabled, finite = map(int, match.group(1, 2))
    error = float(match.group(3))
    report['checks'].update({
        'unchanged_eye_mode': 'constructor_eye_enabled=1' not in status,
        'expected_state_mode': ('constructor_state_enabled=1' in status) == independent,
        'full_constructor_enabled': enabled == 1,
        'seven_derived_blocks_finite': finite == 1,
        'derived_blocks_match': math.isfinite(error) and 0 <= error <= 1e-5,
    })
    report['passed'] = all(report['checks'].values())
    report['constructor'] = {'enabled': enabled, 'finite': finite, 'max_error': error,
                             'blocks': ['0x160', '0x1a0', '0x1e0', '0x220', '0x260', '0x2b0', '0x2f0']}
    report['limits'] = [
        'One unsubmitted reconstruction using unchanged matrices and dimensions.',
        'Static single-player scope; no head motion or independent eye history validated here.',
        'Only seven derived matrix blocks and four known arrays compared; not complete object equivalence.',
        'No stereo rendering, GPU completion or OpenXR eye projection demonstrated.',
    ]
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('capture', 'restored', 'output'):
        p.add_argument('--' + name, type=pathlib.Path, required=True)
    a = p.parse_args()
    report = analyze(a.capture.with_suffix('.status.txt').read_text(), json.loads(a.restored.read_text()))
    a.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
