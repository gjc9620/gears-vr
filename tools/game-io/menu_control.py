"""Confirm precisely reviewed menu choices using bounded, read-only UI snapshots."""
import argparse
from dataclasses import dataclass, field
import json
from pathlib import Path
import time


RULES = {
    ('DeltaMainMenu', 'WarUIScene_MainMenu', 'CampaignButton', 'UIButtonIgnoreStart'): 'confirm',
    ('DeltaCampaignType', 'WarUIScene_CampaignType', 'butSinglePlayer', 'UILabelButton'): 'confirm',
    ('DeltaCampaignMenu_ContinueGame', 'WarUIScene_Campaign', 'butContinue', 'UILabelButton'): 'confirm',
    ('AttractLauncher', 'UIScene_AttractLauncher', 'AttractLauncher', 'UIScene_AttractLauncher'): 'confirm',
    ('WarfareConfirmation', 'UIScene_Confirmation', 'OkButton', 'WarLabelButtonOnlyClickable'): 'confirm',
}
TITLE_CHOICE = ('AttractLauncher', 'UIScene_AttractLauncher', 'AttractLauncher', 'UIScene_AttractLauncher')
ONLINE_CHOICE = ('WarfareConfirmation', 'UIScene_Confirmation', 'OkButton', 'WarLabelButtonOnlyClickable')
TITLE_STACK = (('UIScene', 'UIScene', 0),
               ('DeltaWarTitleScreen', 'WarUIScene_TitleScreen', 4),
               ('AttractLauncher', 'UIScene_AttractLauncher', 0))
ONLINE_TITLE = 'Online Service Error'
ONLINE_MESSAGE = ('Gears of War: Reloaded services login failed, access to online multiplayer is not available.'
                  '\n\nYou are not connected. Please check your connection and try again.')
INITIALIZING_ERRORS = frozenset((
    'Expected one registered GameUISceneClient; found 0',
    'Invalid name array header', 'Invalid object array header',
))
REGISTRY_IDENTITY_CHANGED = 'Object registry identity changed'


def reader_process_identity(reader):
    """Bind the reader's existing process handle to a PID and creation FILETIME."""
    import ctypes as c
    from ctypes import wintypes as w
    kernel = c.WinDLL('kernel32', use_last_error=True)
    kernel.GetProcessId.argtypes, kernel.GetProcessId.restype = [w.HANDLE], w.DWORD
    kernel.GetProcessTimes.argtypes = [w.HANDLE, *([c.POINTER(w.FILETIME)] * 4)]
    kernel.GetProcessTimes.restype = w.BOOL
    handle, stamps = reader.r.p.handle, [w.FILETIME() for _ in range(4)]
    pid = kernel.GetProcessId(handle)
    if not pid or not kernel.GetProcessTimes(handle, *(c.byref(stamp) for stamp in stamps)):
        raise c.WinError(c.get_last_error())
    creation = (stamps[0].dwHighDateTime << 32) | stamps[0].dwLowDateTime
    if not creation:
        raise RuntimeError('Invalid game creation identity')
    return pid, creation


def wait_for_replacement(factory, identity, window, interrupted, *, expected_identity, **kwargs):
    """Revalidate each fresh handle before accepting a rediscovered registry/client."""
    def matching_factory():
        reader = factory()
        try:
            if identity(reader) != expected_identity:
                raise RuntimeError('Game process creation identity changed')
        except Exception:
            reader.close()
            raise
        return reader
    return wait_for_reader(matching_factory, window, interrupted, **kwargs)


def wait_for_reader(factory, window, interrupted, *, target, deadline,
                    emit=lambda event: None, now=time.monotonic, sleep=time.sleep):
    """Retry only known early initialization errors; factory must close failed construction."""
    previous_error = None
    while now() < deadline:
        if interrupted():
            return None, 'interrupted'
        if window() != target:
            return None, 'window_changed'
        try:
            reader = factory()
        except RuntimeError as error:
            if str(error) not in INITIALIZING_ERRORS:
                raise
            if str(error) != previous_error:
                emit(dict(event='initializing', reason=str(error)))
            previous_error = str(error)
            sleep(min(.25, max(0, deadline - now())))
            continue
        try:
            stopped = interrupted()
            changed = window() != target
        except Exception:
            reader.close()
            raise
        if stopped or changed or now() >= deadline:
            reader.close()
            return None, 'interrupted' if stopped else 'window_changed' if changed else 'timeout'
        emit(dict(event='reader_ready'))
        return reader, 'ready'
    return None, 'timeout'


def choice(state):
    top, leaf = state.get('top') or {}, state.get('focused_leaf') or {}
    return top.get('tag'), top.get('cls'), leaf.get('name'), leaf.get('cls')


def allowed(state, pid):
    chain = state.get('focus_chain')
    key = choice(state)
    common = (state.get('pid') == pid and state.get('consistent_reread') is True
            and state.get('player_index') == 0 and state.get('input_block_count') == 0
            and bool(chain) and state.get('focused_leaf') == chain[-1]
            and key in RULES)
    if not common:
        return False
    if key == TITLE_CHOICE:
        stack = tuple((x.get('name'), x.get('cls'), x.get('input_mode'))
                      for x in state.get('active_scenes', []))
        top = state.get('top') or {}
        return (stack == TITLE_STACK and len(chain) == 1
                and top.get('name') == 'AttractLauncher'
                and chain[0].get('name') == top.get('name')
                and chain[0].get('cls') == top.get('cls')
                and chain[0].get('address') == top.get('address')
                and 'UIState_Enabled' in chain[0].get('states', [])
                and 'UIState_Disabled' not in chain[0].get('states', []))
    focused = all({'UIState_Enabled', 'UIState_Focused'} <= set(x.get('states', []))
                  and 'UIState_Disabled' not in x.get('states', []) for x in chain)
    if key == ONLINE_CHOICE:
        text = state.get('dialog_text') or {}
        return focused and text.get('Title') == ONLINE_TITLE and text.get('MessageText') == ONLINE_MESSAGE
    return focused


def fingerprint(state):
    # Include addresses, ownership, focus states, scene stack and gates, not timings.
    return json.dumps({k: state.get(k) for k in (
        'pid', 'client', 'active_scenes', 'top', 'focus_chain', 'focused_leaf',
        'player_index', 'input_block_count', 'consistent_reread', 'dialog_text')}, sort_keys=True)


@dataclass
class ControlProgress:
    """One navigation attempt's dispatch ledger, retained across window rebinding."""
    attempted: set = field(default_factory=set)
    actions: int = 0
    last_success: tuple | None = None
    retry_choice: tuple | None = None
    retries: int = 0

    def window_lost(self):
        # Only the most recent fully completed navigation pair can qualify.
        # One extra dispatch globally; loading confirmations are never replayed.
        if self.retries == 0 and self.last_success and self.last_success[2] != 'butContinue':
            self.retry_choice = self.last_success


def control(read_state, send_action, window, interrupted, *, pid, seconds=60,
            max_actions=5, emit=lambda event: None, now=time.monotonic, sleep=time.sleep,
            target=None, deadline=None, recover_reader=None, progress=None):
    """Injectable polling controller. Successful confirmation is not level readiness.

    Optional recovery replaces the source used by read_state and returns a reader
    wait reason. It runs only for the recognized stale registry identity error;
    its failures propagate, and recovery never preserves snapshot stability.
    """
    if seconds <= 0 or max_actions < 1:
        raise ValueError('seconds and max-actions must be positive')
    target = window() if target is None else target
    if target[0] != pid:
        raise RuntimeError('Initial game window PID mismatch')
    deadline, stable_since = now() + seconds if deadline is None else deadline, None
    last, last_error = None, None
    progress = ControlProgress() if progress is None else progress

    def finish(reason):
        result = dict(reason=reason, actions=progress.actions, loading_requested=reason == 'loading_requested',
                      level_ready=False, pid=pid)
        emit(dict(event='finished', **result))
        return result

    while now() < deadline:
        if interrupted():
            return finish('interrupted')
        if progress.actions >= max_actions + progress.retries:
            return finish('max_actions')
        if window() != target:
            return finish('window_changed')
        try:
            state = read_state()
            current = fingerprint(state)
            last_error = None
        except Exception as error:
            message = type(error).__name__ + ': ' + str(error)
            if message != last_error:
                emit(dict(event='read_error', error=message))
            last_error, last, stable_since = message, None, None
            if (recover_reader is not None and type(error) is RuntimeError
                    and str(error) == REGISTRY_IDENTITY_CHANGED):
                reason = recover_reader()
                if reason != 'ready':
                    return finish(reason)
            sleep(min(.15, max(0, deadline - now())))
            continue
        if current != last:
            emit(dict(event='state', state=state))
            last, stable_since = current, now()
        key = choice(state)
        retry = key == progress.retry_choice and progress.retries == 0 and key[2] != 'butContinue'
        if allowed(state, pid) and (key not in progress.attempted or retry) and now() - stable_since >= .6:
            try:
                before = read_state()
            except Exception as error:
                emit(dict(event='read_error', error=type(error).__name__ + ': ' + str(error)))
                last, stable_since = None, None
                if (recover_reader is not None and type(error) is RuntimeError
                        and str(error) == REGISTRY_IDENTITY_CHANGED):
                    reason = recover_reader()
                    if reason != 'ready':
                        return finish(reason)
                sleep(min(.15, max(0, deadline - now())))
                continue
            if fingerprint(before) != current or not allowed(before, pid):
                emit(dict(event='action_withheld', reason='snapshot_changed'))
                last, stable_since = None, None
                sleep(min(.15, max(0, deadline - now())))
                continue
            if interrupted():
                return finish('interrupted')
            if window() != target:
                return finish('window_changed')
            if now() >= deadline:
                break
            # Reserve before dispatch: even a partially failed send is never retried.
            progress.attempted.add(key)
            progress.last_success = progress.retry_choice = None
            if retry:
                progress.retries += 1
            emit(dict(event='action_attempt', action=RULES[key], choice=key, target=target, window_retry=retry))
            try:
                send_action(RULES[key], target)
            except Exception as error:
                if interrupted():
                    return finish('interrupted')
                emit(dict(event='action_error', error=type(error).__name__ + ': ' + str(error)))
                return finish('action_error')
            progress.actions += 1
            progress.last_success = key
            emit(dict(event='action_sent', action=RULES[key], choice=key, window_retry=retry))
            if key[2] == 'butContinue':
                return finish('loading_requested')
            if progress.actions >= max_actions + progress.retries:
                return finish('max_actions')
        sleep(min(.15, max(0, deadline - now())))
    return finish('timeout')


def send_confirm(action, target, *, window_lookup=None):
    """Same HWND Enter pair as confirm_window; bind dispatch to the original HWND."""
    import ctypes as c
    from ctypes import wintypes as w
    from game_io import game_window, interrupted, user
    # Launchers for a selected installation supply their identity-checked HWND
    # lookup. Standalone tools retain their original installation lookup.
    lookup=window_lookup if window_lookup is not None else game_window
    if action != 'confirm' or interrupted() or lookup() != target:
        raise RuntimeError('Confirmation target changed or Stop/F8 active')
    user.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
    user.PostMessageW.restype = w.BOOL
    hwnd = target[1]
    if not user.PostMessageW(hwnd, 0x100, 13, 0x001c0001):
        raise c.WinError(c.get_last_error())
    try:
        time.sleep(.08)
    finally:
        if not user.PostMessageW(hwnd, 0x101, 13, 0xc01c0001):
            raise c.WinError(c.get_last_error())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--seconds', type=float, default=60)
    parser.add_argument('--max-actions', type=int, default=5)
    args = parser.parse_args()
    from menu_state import MenuReader
    from game_io import game_window, interrupted
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open('w', encoding='utf-8') as log:
        def emit(event):
            log.write(json.dumps(dict(elapsed=time.monotonic() - began, **event)) + '\n')
            log.flush()
        began = time.monotonic()
        reader = None
        try:
            if args.seconds <= 0 or args.max_actions < 1:
                raise ValueError('seconds and max-actions must be positive')
            target, deadline = game_window(), began + args.seconds
            if target[0] != args.pid:
                raise RuntimeError('Initial game window PID mismatch')
            reader, reason = wait_for_reader(lambda: MenuReader(args.pid), game_window, interrupted,
                                            target=target, deadline=deadline, emit=emit)
            if reader is None:
                result = dict(reason=reason, actions=0, loading_requested=False,
                              level_ready=False, pid=args.pid)
                emit(dict(event='finished', **result))
            else:
                identity = reader_process_identity(reader)
                if identity[0] != args.pid:
                    raise RuntimeError('Reader process PID mismatch')
                def recover_reader():
                    nonlocal reader
                    replacement, reason = wait_for_replacement(
                        lambda: MenuReader(args.pid), reader_process_identity, game_window, interrupted,
                        expected_identity=identity, target=target, deadline=deadline, emit=emit)
                    if replacement is not None:
                        previous, reader = reader, replacement
                        previous.close()
                        emit(dict(event='reader_reacquired', pid=identity[0], creation_time=identity[1]))
                    return reason
                result = control(lambda: reader.sample(), send_confirm, game_window, interrupted,
                                 pid=args.pid, seconds=args.seconds, max_actions=args.max_actions,
                                 emit=emit, target=target, deadline=deadline, recover_reader=recover_reader)
        except Exception as error:
            emit(dict(event='error', error=type(error).__name__ + ': ' + str(error)))
            result = dict(reason='error', loading_requested=False, level_ready=False)
        finally:
            if reader is not None:
                reader.close()
    print(json.dumps(dict(result, output=str(args.out))))
    return 0 if result['reason'] == 'loading_requested' else 1


if __name__ == '__main__':
    raise SystemExit(main())
