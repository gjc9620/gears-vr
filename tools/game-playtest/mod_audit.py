"""Review owned Mod runtime evidence without treating navigation failures as Mod bugs."""
import argparse
from bisect import bisect_right
import hashlib
import json
import math
from pathlib import Path

from core import atomic_json


def json_rows(path):
    with Path(path).open(encoding='utf-8-sig') as stream:
        for line in stream:
            try:
                value = json.loads(line)
            except ValueError:
                continue  # XR stdout also contains ordinary text.
            if isinstance(value, dict):
                yield value


def publication_stalls(samples, menu_rows):
    """Require fresh producer progress in a stable, explicitly gameplay interval.

    Use the native measurement clock, rather than the later telemetry receipt.
    The separate menu ledger is authoritative: cached telemetry.menu_mode can
    still say gameplay while the movie display lease is active.
    """
    modes = sorted((r['observed_ns'], r['mode']) for r in menu_rows
                   if type(r.get('observed_ns')) is int and type(r.get('mode')) is int)
    times = [row[0] for row in modes]
    previous, start, last_stall = None, None, None
    stalls = []
    for row in sorted(samples, key=lambda r: r.get('observed_ns', 0)):
        native = row.get('native') or {}
        qpc, frequency = native.get('telemetry_qpc'), native.get('telemetry_qpc_frequency')
        valid_clock = (type(qpc) is int and type(frequency) is int and frequency > 0)
        time_ns = qpc * 1_000_000_000 // frequency if valid_clock else None
        period = (row.get('periods') or {}).get('native_seconds')
        valid_period = type(period) in (int, float) and math.isfinite(period) and 0 < period <= 3
        begin = time_ns - round(period * 1e9) if valid_clock and valid_period else None
        index = bisect_right(times, begin) - 1 if begin is not None else -1
        eligible = (index >= 0 and modes[index][1] == 1
                    and bisect_right(times, time_ns) == index + 1
                    and (row.get('source_states') or {}).get('native') == 'fresh'
                    and not row.get('source_errors')
                    and all(type(native.get(k)) is int for k in
                            ('published', 'telemetry_present_calls', 'world_generation', 'reference_generation')))
        current = dict(time=time_ns, native=native) if eligible else None
        continuous = (current is not None and previous is not None
                      and 0 < time_ns - previous['time'] <= 3_000_000_000
                      and all(native[k] == previous['native'][k]
                              for k in ('world_generation', 'reference_generation')))
        frozen = (continuous and native['published'] == previous['native']['published']
                  and native['telemetry_present_calls'] > previous['native']['telemetry_present_calls'])
        if frozen:
            if start is None:
                start = previous['time']
            if time_ns - start >= 3_000_000_000:
                if last_stall is None:
                    last_stall = dict(first_ns=start, last_ns=time_ns,
                                      world_generation=native['world_generation'],
                                      published=native['published'], confirmed=False,
                                      classification='mod_candidate', category='stereo_publication_stalled')
                    stalls.append(last_stall)
                last_stall.update(last_ns=time_ns, seconds=(time_ns - start) / 1e9)
        else:
            start = last_stall = None
        previous = current
    return stalls


def audit(directory):
    directory = Path(directory).resolve()
    report_path = directory / 'report.json'
    report = json.loads(report_path.read_text(encoding='utf-8-sig'))
    provenance = report.get('provenance') or {}
    session_path = Path(provenance['session_path'])
    session = json.loads(session_path.read_text(encoding='utf-8-sig'))
    if report.get('phase') not in ('finished', 'incomplete') or session.get('phase') != 'stopped':
        raise ValueError('Final Mod review requires a closed run and a stopped session')
    for key in ('game_identity', 'owner_identity'):
        if not provenance.get(key) or provenance[key] != session.get(key):
            raise ValueError('Run/session identity mismatch: ' + key)
    expected = provenance.get('manifest') or provenance.get('artifacts') or {}
    actual = session.get('diagnostics_artifacts') or {}
    for key in ('game_sha256', 'native_sha256', 'xr_sha256'):
        if not expected.get(key) or expected[key] != actual.get(key):
            raise ValueError('Run/session binary mismatch: ' + key)
    stem = session_path.with_suffix('')
    menu_path = Path(str(stem) + '.menu.jsonl')
    stdout_path = Path(str(stem) + '.stdout.txt')
    telemetry_paths = sorted(stem.parent.glob(stem.name + '.telemetry*.jsonl'))
    menus = list(json_rows(menu_path))
    samples = [r for p in telemetry_paths for r in json_rows(p) if r.get('event') == 'sample']
    wanted = {'outage_diagnostics', 'movie_fallback_diagnostics', 'image_ticket_diagnostics',
              'pause_menu_evidence', 'game_window_fallback_first_frame', 'game_window_fallback_recovered'}
    markers = {}
    for row in json_rows(stdout_path):
        kind = next(iter(row), '')
        if kind in wanted:
            markers[kind] = row
    findings = publication_stalls(samples, menus)
    native = session.get('native_status') or {}
    for key in ('owned_ui_faults', 'owned_material_faults', 'cleanup_timeout'):
        value = native.get(key)
        if str(value or '0').isdigit() and int(value or 0) > 0:
            findings.append(dict(category=key, count=int(value), confirmed=False, classification='mod_candidate'))
    ticket = markers.get('image_ticket_diagnostics') or {}
    if (ticket.get('invalid_submitted_ticket_rows') or 0) > 0:
        findings.append(dict(category='invalid_submitted_ticket',
                             count=ticket['invalid_submitted_ticket_rows'],
                             confirmed=False, classification='mod_candidate'))
    life = provenance.get('lifecycle') or {}
    inputs = [report_path, session_path, menu_path, stdout_path, *telemetry_paths]
    return dict(schema=1, focus='vr_mod', run=str(directory),
                source_hashes={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                binary_identity=actual, game_identity=provenance['game_identity'],
                telemetry_samples=len(samples), findings=findings, confirmed_mod_bugs=0,
                menu_transitions=[{k: r[k] for k in ('observed_ns', 'mode', 'reason') if k in r}
                                  for r in menus if 'mode' in r],
                xr_markers=markers, projection_summary=session.get('continuous_session_evidence'),
                coverage=report.get('coverage'), lifecycle=life,
                navigation_and_unattributed_input_candidates_are_not_mod_bugs=True,
                limits=['Candidates require reproduction and causal attribution.',
                        'Movie common-panel frames are intentional; projection gaps alone do not prove black pixels.',
                        'No pixel, GPU correctness, physical controller or headset acceptance.',
                        'Zero findings only covers the recorded diagnostic signals, not all Mod behavior.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    value = audit(args.run)
    output = args.run / 'mod-report.json'
    atomic_json(output, value)
    print(json.dumps(dict(output=str(output), telemetry_samples=value['telemetry_samples'],
                          findings=len(value['findings']), confirmed_mod_bugs=0)))


if __name__ == '__main__':
    main()
