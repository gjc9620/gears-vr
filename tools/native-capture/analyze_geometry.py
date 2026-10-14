"""Summarize bounded geometry observations without treating counts as pixels."""
import argparse
import json
import pathlib
import re


def records(text, prefix):
    return [dict(re.findall(r'(\w+)=([^\s]+)', line))
            for line in text.splitlines() if line.startswith(prefix + ' ')]


def analyze(text, restored):
    match = re.search(r'^capturing pid=(\d+) ', text, re.M)
    geometry = records(text, 'geometry_view')
    draws = records(text, 'engine_view_draw')
    summary = records(text, 'geometry_summary')
    scope = records(text, 'engine_view_draw_summary')
    checks = {
        'same_pid': bool(match and int(match[1]) == restored.get('pid')),
        'clean_stop': 'stopped disable_status=0 active=0' in text,
        'entry_restore': restored.get('all_match') is True,
        'both_geometry_views_observed': {r['eye'] for r in geometry} == {'0', '1'},
        'no_geometry_overflow_or_rejection': bool(summary and summary[0]['overflow'] == '0' and summary[0]['rejected'] == '0'),
        'valid_geometry_samples': bool(geometry) and all(r['valid'] == '15' and r['invalid_lists'] == '0' for r in geometry),
    }
    eyes = {}
    for eye in ('0', '1'):
        items = [r for r in geometry if r['eye'] == eye]
        commands = [r for r in draws if r['eye'] == eye]
        eyes[eye] = {
            'geometry_scope_calls': sum(int(r['calls']) for r in items),
            'geometry_scope_direct_draws': sum(int(r['draws']) + int(r['indexed']) for r in items),
            'context_direct_draws': sum(int(r['calls']) for r in commands),
            'context_final_quad_draws': sum(int(r['calls']) for r in commands if r['final_quad'] == '1'),
        }
    return {'observation_checks_passed': all(checks.values()), 'checks': checks,
            'eyes': eyes, 'geometry': geometry, 'engine_view_draws': draws,
            'engine_view_summary': scope, 'pixels_verified': False, 'stereo_verified': False,
            'limits': ['Only same-thread DrawInstanced/DrawIndexedInstanced, not ExecuteIndirect.',
                       'Current engine view may remain bound during shared/postprocessing work.',
                       'First samples and aggregate counts do not identify the first failed GPU event.',
                       'Empty per-stage dynamic lists do not imply no static scene geometry.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ('status', 'restored', 'output'):
        parser.add_argument('--' + field, type=pathlib.Path, required=True)
    args = parser.parse_args()
    report = analyze(args.status.read_text(), json.loads(args.restored.read_text()))
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k: report[k] for k in ('observation_checks_passed', 'eyes', 'engine_view_summary', 'pixels_verified')}, indent=2))
    return int(not report['observation_checks_passed'])


if __name__ == '__main__':
    raise SystemExit(main())
