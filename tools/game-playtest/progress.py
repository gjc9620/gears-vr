"""Export an owned run's hashed post-test profiles as a separate continuation fixture."""
import argparse
import hashlib
import json
from pathlib import Path


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def export_fixture(directory, output, *, allow_incomplete=False):
    directory, output = Path(directory).resolve(), Path(output).resolve()
    root = Path(__file__).resolve().parents[2] / 'analysis'
    if not directory.is_relative_to(root) or not output.is_relative_to(root):
        raise ValueError('Progress derivatives must remain under ignored analysis/')
    report_raw = (directory / 'report.json').read_bytes()
    report = json.loads(report_raw)
    life = report.get('provenance', {}).get('lifecycle') or {}
    phase = report.get('phase')
    closed_phase = phase == 'finished' or (allow_incomplete and phase == 'incomplete')
    if (report.get('mode') != 'run' or not closed_phase or
            not all(life.get(key) is True for key in ('native_cleanup_verified', 'entry_restore_verified',
                                                      'game_exit_confirmed', 'protected_files_restored'))):
        raise RuntimeError('Continuation fixture requires verified owned cleanup and restoration')
    backup_dir = directory / 'backup'
    manifest = json.loads((backup_dir / 'manifest.json').read_text(encoding='utf8'))
    restored = json.loads((backup_dir / 'restored.json').read_text(encoding='utf8'))
    if restored.get('restored') is not True:
        raise RuntimeError('Protected restoration receipt is incomplete')
    receipts = {row['source']: row for row in restored['files']}
    files = []
    for row in manifest['files']:
        name = Path(row['source']).name
        if '.ue3profile' not in name:
            continue
        original = Path(row['backup']).resolve()
        if original.parent != backup_dir or digest(original.read_bytes()) != row['sha256']:
            raise RuntimeError('Original backup identity changed')
        receipt = receipts.get(row['source']) or {}
        if receipt.get('restored_sha256') != row['sha256']:
            raise RuntimeError('Original profile restoration was not verified')
        candidates = [original.with_name(original.name + '.after'),
                      *sorted(backup_dir.glob(original.name + '.recovery-*.after'))]
        matches = [path for path in candidates if path.is_file() and
                   digest(path.read_bytes()) == receipt.get('test_sha256')]
        if not matches:
            raise RuntimeError('Post-test profile bytes do not match the restoration receipt')
        files.append(dict(name=name, path=str(matches[0]), sha256=receipt['test_sha256']))
    if not files:
        raise RuntimeError('No verified post-test profiles found')
    history = list((report.get('provenance', {}).get('fixture') or {}).get('completed_checkpoint_signals', []))
    for path in sorted(directory.glob('checkpoint-flow-*.json')):
        value = json.loads(path.read_text(encoding='utf8'))
        for row in value.get('completed_checkpoint_signals_observed', []):
            history.append(dict(row, source=str(path), source_sha256=digest(path.read_bytes())))
    unique_history = {}
    for row in history:
        if isinstance(row, dict):
            key = json.dumps([row.get('game_sha256'), row.get('root_path'), row.get('event_name')], sort_keys=True)
            unique_history[key] = row
    fixture = dict(schema=1, kind='owned_test_progress', files=files,
                   completed_checkpoint_signals=list(unique_history.values()),
                   basis='Normal game post-test profile bytes; actual resumed checkpoint must be observed',
                   source_report=str(directory / 'report.json'), source_report_sha256=digest(report_raw),
                   source_run_phase=phase, source_run_closed_normally=phase == 'finished',
                   source_scenario_completed=report.get('scenario_completed') is True,
                   source_health_assistance=((report['provenance'].get('health_assistance') or {}).get('enabled') is True
                       or (report['provenance'].get('fixture') or {}).get('source_health_assistance') is True),
                   game_sha256=report['provenance']['manifest']['game_sha256'],
                   assisted=report.get('assisted', False), generator_sha256=digest(Path(__file__).read_bytes()),
                   chapter_completion_verified=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('x', encoding='utf8') as stream:
        json.dump(fixture, stream, ensure_ascii=False, indent=2)
    return fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-incomplete', action='store_true',
                        help='Keep a failed owned scenario explicit while exporting only after verified cleanup/restoration')
    args = parser.parse_args()
    fixture = export_fixture(args.run, args.output, allow_incomplete=args.allow_incomplete)
    print(json.dumps(dict(output=str(args.output), files=len(fixture['files']),
                          game_sha256=fixture['game_sha256'], checkpoint_resume_verified=False)))


if __name__ == '__main__':
    main()
