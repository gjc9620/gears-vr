"""Run the default-off, read-only hand probe in an already owned retail arena.

The background game owner creates --owned-json and waits for its sibling stop
file. This runner never launches a process or enables physical controller input.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import time
import traceback

from free_hands_probe_report import summarize

ROOT = Path(__file__).resolve().parents[2]


def wait_for(check, seconds, reason):
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        value = check()
        if value:
            return value
        time.sleep(.08)
    raise TimeoutError(reason)


def run(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
    identity = owned['game']
    launch = owned.get('launch', {})
    retail = (args.runtime_root.resolve() / 'Gears of War Reloaded' /
              'Binaries_x64' / 'GOWDE-Steam.exe').resolve()
    if not owned.get('private_root') or not owned.get('owner') or \
            not launch.get('test_windows_behind_requested') or \
            launch.get('background_input_requested') is not False or \
            launch.get('steam_app_id') != '2523720' or \
            Path(launch.get('game_path', '')).resolve() != retail or \
            Path(identity.get('image', '')).resolve() != retail:
        raise ValueError('Expected an owned, behind-windows retail test')
    bridge = None
    try:
        def attach():
            nonlocal bridge
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256)
                return True
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header',
                                      'Invalid object array header'):
                    raise
                return False

        wait_for(attach, 90, 'Original object registries did not become ready')
        wait_for(lambda: bridge.export('PlaygroundEnable') == 0, 60,
                 'Playground menu viewport unavailable')
        if bridge.export('PlaygroundTestEnter') != 0:
            raise RuntimeError('Private arena entry rejected')
        status_path = args.dll.with_name(f'playground-status-{identity["pid"]}.json')

        def ready():
            try:
                value = json.loads(status_path.read_text(encoding='utf-8'))
            except (FileNotFoundError, json.JSONDecodeError):
                return False
            if value.get('failures'):
                raise RuntimeError('Private arena setup failed')
            return bool(value.get('ready') and value.get('pickups', 0) > 0)

        wait_for(ready, 120, 'Private arena did not become ready')
        if bridge.export('PlaygroundFreeHandsProbeStart') != 0:
            raise RuntimeError('Free-hands read-only probe did not arm')

        def complete():
            status = bridge.export('PlaygroundFreeHandsProbeStatus')
            phase, count = status & 0xffff, status >> 16
            if phase in (4, 5):
                raise RuntimeError(f'Free-hands probe stopped early: {phase}, {count}')
            return count if phase == 3 and count == 160 else False

        receipt['sample_count'] = wait_for(complete, 16, 'Hand sample limit not reached')
    finally:
        if bridge:
            try:
                receipt['shutdown'] = bridge.export('PlaygroundShutdown')
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
            bridge.close()
    if receipt.get('shutdown') != 0:
        raise RuntimeError('Private Playground shutdown was not clean')
    log = args.dll.with_name(f'playground-{identity["pid"]}.log')
    raw = log.read_bytes()
    report = summarize(raw.decode('utf-8-sig'))
    if not report['completed'] or report['samples'] != 160 or report['writes'] != 0:
        raise RuntimeError('Original hand read-only evidence incomplete')
    receipt.update(game_pid=identity['pid'], log=str(log),
                   log_sha256=hashlib.sha256(raw).hexdigest(), report=report,
                   source_limit='read-only game-thread hand bones; no XR hand or visible 6DoF proof')
    receipt['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owned-json', required=True, type=Path)
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Hand probe receipt must remain in worktree analysis/')
    receipt = {'schema': 1, 'passed': False,
               'source': 'owned retail, default-off private hand probe',
               'probe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    try:
        run(args, receipt)
    except BaseException:
        receipt['error'] = traceback.format_exc()
        raise
    finally:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
