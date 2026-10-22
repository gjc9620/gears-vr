"""Summarize one original retail Lancer reload's bounded visual handoff.

Raw per-frame component evidence remains under ignored analysis/. Each event
is reported as the interval between the last old-state sample and the first
new-state sample, not as an exact animation-notify timestamp.
"""
import argparse
import json
import math
from pathlib import Path


def change(rows, key, before, after):
    matches = [index for index in range(1, len(rows))
               if rows[index - 1][key] == before and rows[index][key] == after]
    if len(matches) != 1:
        raise ValueError(f'Expected exactly one {key} {before}->{after} transition')
    return matches[0]


def interval(rows, index, started):
    previous, current = rows[index - 1], rows[index]
    if started and (not math.isclose(previous['reload_start'], started, abs_tol=.01) or
                    not math.isclose(current['reload_start'], started, abs_tol=.01)):
        raise ValueError('Original reload start changed during visual handoff')
    return {'sample_before': index - 1, 'sample_after': index,
            'wall_ms': [previous['ms'], current['ms']],
            'elapsed_s': ([previous['world'] - started, current['world'] - started]
                          if started else None)}


def summarize(capture):
    rows = capture.get('reload_visual_timeline')
    if (capture.get('passed') is not True or capture.get('shutdown') != 0 or
            not isinstance(rows, list) or len(rows) < 10 or
            [row.get('sample') for row in rows] != list(range(len(rows))) or
            any(rows[index]['qpc'] <= rows[index - 1]['qpc'] or
                rows[index]['ms'] < rows[index - 1]['ms']
                for index in range(1, len(rows)))):
        raise ValueError('Expected a complete owned original reload timeline')
    for row in rows:
        if any(not math.isfinite(row[key]) for key in
               ('world', 'reload_start', 'control', 'target', 'scale')):
            raise ValueError('Nonfinite original visual state')
    entered = change(rows, 'reloading', False, True)
    control_on = change(rows, 'control', 0.0, 1.0)
    attached = change(rows, 'mag_pawn', False, True)
    detached = change(rows, 'mag_pawn', True, False)
    control_off = change(rows, 'control', 1.0, 0.0)
    finished = change(rows, 'reloading', True, False)
    if not (entered < control_on < attached < detached <= finished and
            attached < control_off <= finished):
        raise ValueError('Original magazine handoff order changed')
    started = rows[entered]['reload_start']
    if (started <= 0 or rows[entered]['ammo_used'] <= 0 or
            rows[finished]['ammo_used'] != 0 or
            any(row['ammo_used'] != rows[entered]['ammo_used']
                for row in rows[entered:finished])):
        raise ValueError('Original ammo accounting did not finish with reload')
    events = {'reload_entered': interval(rows, entered, 0),
              'gun_magazine_control_on': interval(rows, control_on, started),
              'hand_magazine_attached': interval(rows, attached, started),
              'hand_magazine_detached': interval(rows, detached, started),
              'gun_magazine_control_off': interval(rows, control_off, started),
              'reload_finished': interval(rows, finished, started)}
    return {'schema': 1, 'source': 'owned retail original Lancer reload',
            'capture_pid': capture.get('game_pid'), 'samples': len(rows),
            'test_seeded_ammo_used': capture.get('seeded_ammo_used'),
            'magazine_hidden_game_ever_true': any(row['mag_hidden'] for row in rows),
            'events': events}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    value = json.dumps(summarize(json.loads(args.capture.read_text(encoding='utf-8'))),
                       indent=2) + '\n'
    if args.output:
        args.output.write_text(value, encoding='utf-8')
    else:
        print(value, end='')


if __name__ == '__main__':
    main()
