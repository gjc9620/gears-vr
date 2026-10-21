"""Aggregate playtest evidence without promoting observations into confirmed bugs."""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

from core import COVERAGE, atomic_json


def summarize(paths):
    runs, issues = [], {}
    coverage = {key: Counter() for key in COVERAGE}
    for path in paths:
        path = Path(path).resolve()
        report = json.loads(path.read_text(encoding='utf8'))
        provenance = report.get('provenance') or {}
        artifacts = provenance.get('artifacts') or provenance.get('manifest') or {}
        native = artifacts.get('native_sha256')
        xr = artifacts.get('xr_sha256')
        game = artifacts.get('game_sha256')
        health_assistance = ((provenance.get('health_assistance') or {}).get('enabled') is True
                             or (provenance.get('fixture') or {}).get('source_health_assistance') is True
                             or provenance.get('observed_health_assistance') is True)
        runs.append(dict(path=str(path), phase=report['phase'], mode=report['mode'],
                         samples=report['samples'], actions=report['actions'],
                         assisted=report['assisted'], game_sha256=game, native_sha256=native, xr_sha256=xr,
                         lifecycle=provenance.get('lifecycle'), health_assistance=health_assistance))
        assisted = bool(report.get('assisted'))
        for key in COVERAGE:
            coverage[key][(report.get('coverage', {}).get(key) or {}).get('status', 'not_run')] += 1
        for issue in report['issues']:
            # Ignore changing addresses and PIDs only in the grouping title;
            # each original report and complete evidence are retained.
            title = re.sub(r'0x[0-9a-fA-F]+', '<address>', issue['title'])
            identity = (game, native, xr, assisted, health_assistance, issue['classification'], issue['category'], title)
            key = hashlib.sha256(json.dumps(identity).encode()).hexdigest()[:16]
            if key not in issues:
                issues[key] = dict(id=key, category=issue['category'], title=issue['title'],
                    classification=issue['classification'], severity=issue['severity'],
                    confirmed=issue['confirmed'], occurrences=0, affected_runs=[],
                    game_sha256=game, native_sha256=native, xr_sha256=xr, assisted=assisted, health_assistance=health_assistance)
            row = issues[key]
            row['occurrences'] += issue['occurrences']
            row['affected_runs'].append(str(path))
    return dict(schema=1, runs=runs, total_samples=sum(r['samples'] for r in runs),
                total_actions=sum(r['actions'] for r in runs),
                issue_groups=len(issues), issues=list(issues.values()),
                by_classification=dict(Counter(x['classification'] for x in issues.values())),
                confirmed_game_bugs=sum(1 for x in issues.values()
                                        if x['confirmed'] and x['classification'] == 'product_candidate'),
                confirmed_mod_bugs=sum(1 for x in issues.values()
                                       if x['confirmed'] and x['classification'] == 'mod_candidate'),
                coverage_by_run={k: dict(v) for k, v in coverage.items()},
                full_campaign_verified=False,
                limits='Counts describe recorded runs. Untested scenarios and unseen rendering defects remain unverified.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2] / 'analysis/playtest')
    args = parser.parse_args()
    value = summarize(sorted(args.root.glob('*/report.json')))
    atomic_json(args.root / 'summary.json', value)
    with (args.root / 'issues.csv').open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=('id', 'category', 'title', 'classification',
            'severity', 'confirmed', 'occurrences', 'game_sha256', 'native_sha256', 'xr_sha256', 'assisted', 'health_assistance', 'affected_runs'))
        writer.writeheader()
        for issue in value['issues']:
            writer.writerow(dict(issue, affected_runs='; '.join(issue['affected_runs'])))
    print(json.dumps({k: value[k] for k in ('total_samples', 'total_actions', 'issue_groups', 'by_classification')}))


if __name__ == '__main__':
    main()
