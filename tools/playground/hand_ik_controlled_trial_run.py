"""Exercise the default-off controlled right-hand IK trial in an owned retail arena.

The external game owner supplies --owned-json and waits for its sibling stop
file. This runner never launches a game or enables physical controller input.
It only reports bounded raw evidence; this trial cannot certify a skeletal
effect or visible hands without a separate drift/sham analysis.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import sys
import time
import traceback

ROOT = Path(__file__).resolve().parents[2]


def decline_owned_private_settings_prompt(pid):
    """Dismiss only the exact prior-launch prompt in this owned game process.

    The test's private profile must retain its prepared settings. BM_CLICK does
    not activate, move, resize or reorder the user's windows.
    """
    user32 = ctypes.WinDLL('user32', use_last_error=True)
    callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
    user32.EnumChildWindows.argtypes = [wintypes.HWND, callback, wintypes.LPARAM]
    user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND,
                                                 ctypes.POINTER(wintypes.DWORD)]
    user32.GetWindowTextLengthW.argtypes = [wintypes.HWND]
    user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user32.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
    user32.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT,
                                            wintypes.WPARAM, wintypes.LPARAM,
                                            wintypes.UINT, wintypes.UINT,
                                            ctypes.POINTER(ctypes.c_size_t)]

    def title(hwnd):
        size = user32.GetWindowTextLengthW(hwnd)
        buffer = ctypes.create_unicode_buffer(size + 1)
        user32.GetWindowTextW(hwnd, buffer, len(buffer))
        return buffer.value

    def class_name(hwnd):
        buffer = ctypes.create_unicode_buffer(128)
        user32.GetClassNameW(hwnd, buffer, len(buffer))
        return buffer.value

    matches = []

    def top(hwnd, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value != pid or class_name(hwnd) != '#32770' or title(hwnd) != 'Message':
            return True
        children = []

        def child(candidate, _):
            children.append((candidate, class_name(candidate), title(candidate),
                             user32.GetDlgCtrlID(candidate)))
            return True

        user32.EnumChildWindows(hwnd, callback(child), 0)
        if any(kind == 'Static' and caption ==
               'Last launch did not complete successfully. Revert to default settings?'
               for _, kind, caption, _ in children):
            matches.extend(candidate for candidate, kind, _, control_id in children
                           if kind == 'Button' and control_id == 7)  # IDNO
        return True

    user32.EnumWindows(callback(top), 0)
    if not matches:
        return False
    if len(matches) != 1:
        raise RuntimeError('Ambiguous owned private-settings prompt')
    result = ctypes.c_size_t()
    if not user32.SendMessageTimeoutW(matches[0], 0xF5, 0, 0, 2, 2000,
                                      ctypes.byref(result)):  # BM_CLICK
        raise ctypes.WinError(ctypes.get_last_error())
    return True


def wait_for(check, seconds, reason):
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        value = check()
        if value:
            return value
        time.sleep(.08)
    raise TimeoutError(reason)


def parse_end(log):
    lines = [line for line in log.splitlines()
             if line.startswith('hand_ik_controlled_trial_end ')]
    if len(lines) != 1:
        raise RuntimeError('Expected exactly one controlled hand IK terminal row')
    row = {}
    for item in lines[0].split()[1:]:
        name, value = item.split('=', 1)
        if name in row:
            raise RuntimeError('Duplicate controlled hand IK terminal field')
        row[name] = value
    expected = {'status', 'completed_windows', 'evidence_complete', 'restored',
                'setter_calls', 'effector_writes', 'effect_evidence',
                'inconclusive', 'safe_to_unload', 'reason'}
    if set(row) != expected:
        raise RuntimeError('Controlled hand IK terminal fields changed')
    for name in expected - {'reason'}:
        if not row[name].isdigit():
            raise RuntimeError(f'Invalid controlled hand IK {name}')
        row[name] = int(row[name])
    for name in ('evidence_complete', 'restored', 'effect_evidence',
                 'inconclusive', 'safe_to_unload'):
        if row[name] not in (0, 1):
            raise RuntimeError(f'Invalid controlled hand IK {name} flag')
    if (row['status'] != 5 or row['completed_windows'] != 6 or
            row['evidence_complete'] != 1 or row['restored'] != 1 or
            row['safe_to_unload'] != 1 or row['setter_calls'] != 12 or
            row['effector_writes'] != 12 or row['effect_evidence'] != 0 or
            row['inconclusive'] != 1):
        raise RuntimeError('Controlled hand IK windows or restoration incomplete')
    return row


def run(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
    identity = owned['game']
    launch = owned.get('launch', {})
    retail = (args.runtime_root.resolve() / 'Gears of War Reloaded' /
              'Binaries_x64' / 'GOWDE-Steam.exe').resolve()
    if (not owned.get('private_root') or not owned.get('owner') or
            not launch.get('test_windows_behind_requested') or
            launch.get('background_input_requested') is not False or
            launch.get('steam_app_id') != '2523720' or
            Path(launch.get('game_path', '')).resolve() != retail or
            Path(identity.get('image', '')).resolve() != retail):
        raise ValueError('Expected an owned, behind-windows retail test')

    bridge = None
    try:
        def attach():
            nonlocal bridge
            try:
                bridge = Bridge(identity, args.dll,
                                expected_sha256=args.dll_sha256.lower())
                return True
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header',
                                      'Invalid object array header'):
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
        pickup = 0
        for obj in resolver.object_pointers:
            if not obj:
                continue
            try:
                item = resolver.identity(obj)
                if (item['name'].startswith('Default__') or
                        class_name(obj) != 'WarDroppedPickup' or
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

        if bridge.export('PlaygroundHandIkControlledTrialStart') != 0:
            raise RuntimeError('Private controlled right-hand IK trial did not arm')

        def finished():
            status = bridge.export('PlaygroundHandIkControlledTrialStatus')
            phase, count = status & 0xffff, status >> 16
            if phase in (6, 7, 8):
                raise RuntimeError(f'Controlled right-hand IK trial stopped or failed: {phase}, {count}')
            return (phase, count) if phase == 5 else False

        receipt['phase'], receipt['sample_count'] = wait_for(
            finished, 90, 'Controlled right-hand IK trial did not reach a terminal state')
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
    terminal = parse_end(raw.decode('utf-8-sig'))
    if terminal['status'] != receipt['phase']:
        raise RuntimeError('Controlled hand IK export and terminal row disagree')
    receipt.update(game_pid=identity['pid'], log=str(log),
                   log_sha256=hashlib.sha256(raw).hexdigest(),
                   terminal=terminal,
                   effect_proved=False,
                   source_limit=('six bounded original right-hand IK setter/sham windows in an '
                                 'owned background retail test; raw evidence only, no XR hands '
                                 'or headset visibility proof'))
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
        raise ValueError('Controlled hand IK receipt must remain in worktree analysis/')
    receipt = {'schema': 1, 'passed': False,
               'source': 'owned retail, default-off controlled right-hand IK trial',
               'runner_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               'dll_sha256': args.dll_sha256.lower()}
    try:
        run(args, receipt)
    except BaseException:
        receipt['error'] = traceback.format_exc()
        raise
    finally:
        # Preserve a rejected-before-write reason in the private receipt too.
        # A failed run must never be promoted to a successful effect result.
        try:
            pid = json.loads(args.owned_json.read_text(encoding='utf-8'))['game']['pid']
            log = args.dll.with_name(f'playground-{pid}.log')
            raw = log.read_bytes()
            receipt['log'] = str(log)
            receipt['log_sha256'] = hashlib.sha256(raw).hexdigest()
            terminal = [line for line in raw.decode('utf-8-sig').splitlines()
                        if line.startswith('hand_ik_controlled_trial_end ')]
            if terminal:
                receipt['terminal_line'] = terminal[-1]
        except (FileNotFoundError, KeyError, ValueError, UnicodeError):
            pass
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
