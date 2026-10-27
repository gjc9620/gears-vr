"""Record a bounded synthetic dual-hand IK stream in an owned retail arena.

The external owner launches and closes the behind-windows game. This runner
does not launch a game, enable physical input or certify original animation,
XR hands, visual skeletal effects, headset visibility or animation ownership.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import traceback

from hand_animation_probe_run import (
    decline_owned_private_settings_prompt, fields_of, shutdown_bridge,
    verify_owned, verify_preload_call_function_entry, wait_for,
)


ROOT = Path(__file__).resolve().parents[2]
LEASES = 32
PHASES = ('before', 'written', 'pre_restore')
END_EXPECTED = {
    'status': 5, 'leases': 32, 'restored_leases': 32, 'action_yields': 1,
    'immediate_action_test': 1, 'original_melee_seen': 1,
    'original_slot_play_seen': 1, 'resumed': 1, 'setter_calls': 256,
    'target_writes': 256, 'trial_apply_mask': 0, 'dirty': 0, 'faulted': 0,
    'safe_to_unload': 1, 'ownership_proved': 0,
    'reason': 'bounded_dual_hand_recording_complete',
}
SAMPLE_REQUIRED = {
    'phase', 'lease', 'segment', 'trial_apply_mask', 'mesh_tick_known',
    'mesh_tick', 'sequence', 'left', 'right', 'mesh',
}
RESTORE_FIELDS = {'lease', 'dirty_before', 'dirty_after', 'restored', 'reason'}
ACTION_EXPECTED = {'dirty_before': 1, 'dirty_after': 0, 'yielded': 1, 'invoked': 1}
STATUS_EXPORT = 'PlaygroundHandIkStreamTrialStatus'
STOP_EXPORT = 'PlaygroundHandIkStreamTrialStop'


def integer(value, label, *, signed=False):
    if not re.fullmatch(r'-?[0-9]+' if signed else r'[0-9]+', value):
        raise RuntimeError(f'Invalid stream trial {label}')
    return int(value)


def prefixed_rows(log, prefix):
    return [(index, fields_of(line, prefix.rstrip()))
            for index, line in enumerate(log.splitlines()) if line.startswith(prefix)]


def parse_end(log):
    rows = prefixed_rows(log, 'hand_ik_stream_trial_end ')
    if len(rows) != 1:
        raise RuntimeError('Expected exactly one hand IK stream terminal row')
    _, row = rows[0]
    if set(row) != set(END_EXPECTED):
        raise RuntimeError('Hand IK stream terminal schema changed')
    parsed = {name: value if name == 'reason' else integer(value, name)
              for name, value in row.items()}
    if parsed != END_EXPECTED:
        raise RuntimeError('Hand IK stream terminal did not confirm 32 complete clean leases')
    return parsed


def matrix(value, label):
    try:
        values = [float(item) for item in value.split(',')]
    except ValueError as error:
        raise RuntimeError(f'Invalid stream trial {label} matrix') from error
    if len(values) != 16 or not all(math.isfinite(item) for item in values):
        raise RuntimeError(f'Incomplete or nonfinite stream trial {label} matrix')
    return values


def parse_sample(row):
    if not SAMPLE_REQUIRED <= set(row):
        raise RuntimeError('Hand IK stream sample fields are incomplete')
    value = dict(row)
    for name in ('lease', 'segment', 'trial_apply_mask', 'mesh_tick_known', 'sequence'):
        value[name] = integer(row[name], name)
    value['mesh_tick'] = integer(row['mesh_tick'], 'mesh_tick', signed=True)
    phase, lease = value['phase'], value['lease']
    if (phase not in PHASES or not 1 <= lease <= LEASES or
            value['segment'] != (lease - 1) // 8 or value['mesh_tick_known'] != 1 or
            value['sequence'] <= 0 or
            value['trial_apply_mask'] != (0 if phase == 'before' else 3) or
            row.get('ownership_proved', '0') != '0'):
        raise RuntimeError('Hand IK stream sample lease, phase or application mask changed')
    for name in ('left', 'right', 'mesh'):
        value[name] = matrix(row[name], name)
    if phase == 'written':
        for index in range(4):
            name = f'target{index}'
            encoded = row.get(name, '')
            if not re.fullmatch(r'[0-9a-fA-F]{24}', encoded):
                raise RuntimeError('Hand IK stream written targets must each contain exactly 12 bytes')
    return value


def validate_log(log):
    terminal = parse_end(log)
    samples = [(index, parse_sample(row)) for index, row in
               prefixed_rows(log, 'hand_ik_stream_trial_sample ')]
    keys = [(row['lease'], row['phase']) for _, row in samples]
    if keys != [(lease, phase) for lease in range(1, LEASES + 1) for phase in PHASES]:
        raise RuntimeError('Expected all 96 ordered hand IK stream sample phases')

    restores = prefixed_rows(log, 'hand_ik_stream_trial_restore ')
    if len(restores) != LEASES:
        raise RuntimeError('Expected exactly 32 hand IK stream restoration rows')
    restore_by_lease = {}
    for expected_lease, (position, row) in enumerate(restores, 1):
        if set(row) != RESTORE_FIELDS:
            raise RuntimeError('Hand IK stream restoration schema changed')
        parsed = {name: item if name == 'reason' else integer(item, name)
                  for name, item in row.items()}
        reason = 'action_boundary' if expected_lease == 16 else 'pre_tick'
        if parsed != {'lease': expected_lease, 'dirty_before': 1,
                      'dirty_after': 0, 'restored': 1, 'reason': reason}:
            raise RuntimeError('Hand IK stream restoration did not release its exact lease')
        restore_by_lease[expected_lease] = position

    actions = prefixed_rows(log, 'hand_ik_stream_trial_action_dispatch_test ')
    if len(actions) != 1 or set(actions[0][1]) != set(ACTION_EXPECTED):
        raise RuntimeError('Expected exactly one hand IK stream immediate action test')
    action_position, action = actions[0]
    action = {name: integer(item, name) for name, item in action.items()}
    if action != ACTION_EXPECTED:
        raise RuntimeError('Hand IK stream action was not restored before original dispatch')

    previous_sequence = 0
    lease_rows = []
    for lease in range(1, LEASES + 1):
        (before_position, before), (_, written), (pre_position, pre_restore) = samples[(lease - 1) * 3:lease * 3]
        if (before['sequence'] <= previous_sequence or
                any(row['sequence'] != before['sequence'] for row in (written, pre_restore)) or
                before['mesh_tick'] != written['mesh_tick']):
            raise RuntimeError('Hand IK stream sequence or before/written mesh tick changed')
        previous_sequence = before['sequence']
        restore_position = restore_by_lease[lease]
        next_before = samples[lease * 3][0] if lease < LEASES else len(log.splitlines())
        if not before_position < pre_position < restore_position < next_before:
            raise RuntimeError('Hand IK stream restoration was outside its recorded lease')
        lease_rows.append({'lease': lease, 'segment': before['segment'],
                           'sequence': before['sequence'],
                           'before_mesh_tick': before['mesh_tick'],
                           'pre_restore_mesh_tick': pre_restore['mesh_tick']})
    if not restore_by_lease[16] < action_position < samples[16 * 3][0]:
        raise RuntimeError('Immediate action dispatch did not follow lease16 restoration before resume')
    terminal_position = prefixed_rows(log, 'hand_ik_stream_trial_end ')[0][0]
    if terminal_position <= max(action_position, restore_by_lease[LEASES], samples[-1][0]):
        raise RuntimeError('Hand IK stream terminal preceded its complete evidence')
    return {'terminal': terminal, 'sample_count': len(samples),
            'restore_count': len(restores), 'action_dispatch_test': action,
            'leases': lease_rows}


def prepare_owned_lancer(bridge, arena, receipt):
    resolver = bridge.r
    resolver.prepare()  # Include the arena's newly spawned original pickups.

    def property_of(obj, name):
        return resolver.property(resolver.identity(obj)['cls'], name, True)

    def pointer(obj, name):
        return resolver.u64(obj + property_of(obj, name)['offset'])

    def class_name(obj):
        return resolver.class_name(resolver.identity(obj))

    pawn = int(arena['pawn'], 0)
    pc = pointer(pawn, 'Controller')
    inventory = pointer(pawn, 'InvManager')
    world = pointer(pc, 'WorldInfo')
    if not pawn or not pc or not inventory or not world:
        raise RuntimeError('Current retail pawn ownership incomplete')
    pickup = 0
    for obj in resolver.object_pointers:
        if not obj:
            continue
        try:
            item = resolver.identity(obj)
            if (item['name'].startswith('Default__') or class_name(obj) != 'WarDroppedPickup' or
                    pointer(obj, 'WorldInfo') != world):
                continue
            cls = pointer(obj, 'InventoryClass')
            if cls and resolver.identity(cls)['name'] == 'Weap_AssaultRifle':
                pickup = obj
                break
        except (RuntimeError, TypeError, KeyError):
            continue
    if not pickup:
        raise RuntimeError('Original Lancer pickup missing in owned arena')
    bridge.call(pickup, 'GiveTo', P=pawn)

    def owned_lancer():
        item = pointer(inventory, 'InventoryChain')
        for _ in range(32):
            if not item:
                break
            if class_name(item) == 'Weap_AssaultRifle':
                return item
            item = pointer(item, 'Inventory')
        return 0

    weapon = wait_for(owned_lancer, 5, 'GiveTo did not add original Lancer')
    bridge.call(inventory, 'SetCurrentWeapon', DesiredWeapon=weapon)
    wait_for(lambda: pointer(pawn, 'Weapon') == weapon, 8,
             'Original Lancer equip did not finish')
    wait_for(lambda: bridge.call(pawn, 'ShouldLeftHandIKBeOn'), 10,
             'Original held Lancer pose did not settle')
    receipt['original_lancer_prepared'] = True


def trial_status(bridge, receipt, key='trial_status'):
    status = bridge.export(STATUS_EXPORT)
    if not isinstance(status, int) or status < 0:
        raise RuntimeError('Invalid hand IK stream status export')
    phase, leases = status & 0xffff, status >> 16
    receipt[key] = {'raw': status, 'phase': phase, 'leases': leases}
    if phase not in range(9) or not 0 <= leases <= LEASES:
        raise RuntimeError('Hand IK stream status phase or successful lease count changed')
    return phase, leases


def wait_trial(bridge, receipt):
    def finished():
        phase, leases = trial_status(bridge, receipt)
        if phase in (6, 7, 8):
            raise RuntimeError(f'Hand IK stream trial cancelled or failed: phase={phase}, leases={leases}')
        if phase == 5:
            if leases != LEASES:
                raise RuntimeError('Hand IK stream completed without exactly 32 successful leases')
            return phase, leases
        return False

    return wait_for(finished, 90, 'Hand IK stream trial did not reach a terminal state')


def cleanup_trial_bridge(bridge, receipt):
    """Do not remove hooks while the stream journal is unsafe or unconfirmed."""
    closed_by_shutdown = False
    try:
        if (receipt.get('trial_armed') or
                (receipt.get('trial_start_attempted') and not receipt.get('trial_start_rejected'))):
            phase, _ = trial_status(bridge, receipt, 'cleanup_status')
            if phase not in (5, 6, 7):
                receipt['trial_stop_result'] = bridge.export(STOP_EXPORT)
                if phase == 8 or receipt['trial_stop_result'] == 40:
                    raise RuntimeError('Unsafe hand IK stream journal retained; owner must close its game')
                if receipt['trial_stop_result'] != 0:
                    raise RuntimeError('Hand IK stream stop request rejected')

                def stopped():
                    current, count = trial_status(bridge, receipt, 'cleanup_status')
                    if current == 8:
                        raise RuntimeError('Unsafe hand IK stream journal retained after stop')
                    return (current, count) if current in (5, 6, 7) else False

                wait_for(stopped, 8, 'Hand IK stream stop did not confirm a clean journal')
        closed_by_shutdown = True
        shutdown_bridge(bridge, receipt, require_call_boundary=True)
    except BaseException:
        receipt['cleanup_error'] = traceback.format_exc()
    finally:
        if not closed_by_shutdown:
            receipt['shutdown_skipped'] = 'stream_journal_not_confirmed_clean'
            bridge.close()


def run(args, receipt):
    identity = verify_owned(args)
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    bridge = None
    try:
        def check_before_load():
            try:
                return verify_preload_call_function_entry(identity, args.runtime_root, receipt)
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header', 'Invalid object array header'):
                    raise
                return False

        wait_for(check_before_load, 90, 'Original registries unavailable before CallFunction check')

        def attach():
            nonlocal bridge
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256.lower())
                return True
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header', 'Invalid object array header'):
                    raise
                return False

        wait_for(attach, 90, 'Original object registries did not become ready')

        def enable():
            bridge.verify()
            if decline_owned_private_settings_prompt(identity['pid']):
                receipt['private_settings_restore_declined'] = True
            return bridge.export('PlaygroundEnable') == 0

        wait_for(enable, 60, 'Playground menu viewport unavailable')
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
            return value if value.get('ready') and value.get('pickups', 0) else False

        arena = wait_for(ready, 120, 'Private arena did not become ready')
        prepare_owned_lancer(bridge, arena, receipt)
        receipt['trial_start_attempted'] = True
        receipt['trial_start_result'] = bridge.export('PlaygroundHandIkStreamTrialStart')
        if receipt['trial_start_result'] != 0:
            receipt['trial_start_rejected'] = True
            raise RuntimeError('Private synthetic dual-hand stream trial did not arm')
        receipt['trial_armed'] = True
        receipt['phase'], receipt['lease_count'] = wait_trial(bridge, receipt)
    finally:
        if bridge:
            cleanup_trial_bridge(bridge, receipt)

    if receipt.get('cleanup_error') or receipt.get('shutdown') != 0:
        raise RuntimeError('Private hand IK stream shutdown was not confirmed clean')
    for name in ('call_function', 'process_event'):
        if (receipt.get(f'{name}_restore_error') or
                not receipt.get(f'{name}_entry', {}).get('after_shutdown', {}).get('matched')):
            raise RuntimeError(f'{name} live entry was not restored after PlaygroundShutdown')
    log_path = args.dll.with_name(f'playground-{identity["pid"]}.log')
    raw = log_path.read_bytes()
    evidence = validate_log(raw.decode('utf-8-sig'))
    if (evidence['terminal']['status'] != receipt['phase'] or
            evidence['terminal']['leases'] != receipt['lease_count']):
        raise RuntimeError('Hand IK stream export and terminal row disagree')
    receipt.update(game_pid=identity['pid'], log=str(log_path),
                   log_sha256=hashlib.sha256(raw).hexdigest(), **evidence,
                   effect_proved=False, ownership_proved=False,
                   source_limit=('32 bounded synthetic dual-hand leases in an owned background '
                                 'retail arena; raw recording and restoration protocol only; '
                                 'no original animation, skeletal effect, XR hand, headset '
                                 'visibility or animation ownership proof'))
    receipt['passed'] = True


def preserve_log(args, receipt):
    try:
        identity = json.loads(args.owned_json.read_text(encoding='utf-8'))['game']
        log_path = args.dll.with_name(f'playground-{identity["pid"]}.log')
        raw = log_path.read_bytes()
        receipt['log'] = str(log_path)
        receipt['log_sha256'] = hashlib.sha256(raw).hexdigest()
        terminal_lines = [line for line in raw.decode('utf-8-sig').splitlines()
                          if line.startswith('hand_ik_stream_trial_end ')]
        if terminal_lines:
            receipt['terminal_line'] = terminal_lines[-1]
    except (FileNotFoundError, KeyError, ValueError, UnicodeError):
        pass


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
        raise ValueError('Hand IK stream receipt must remain in worktree analysis/')
    receipt = {'schema': 1, 'passed': False,
               'source': 'owned retail, default-off bounded synthetic dual-hand stream trial',
               'runner_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               'dll_sha256': args.dll_sha256.lower()}
    try:
        run(args, receipt)
    except BaseException:
        receipt['error'] = traceback.format_exc()
        raise
    finally:
        try:
            preserve_log(args, receipt)
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        finally:
            (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
