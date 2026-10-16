"""Bounded in-process launcher menu navigation; loading is not player readiness."""
import json
from pathlib import Path
import sys
import time


class NavigationCancelled(RuntimeError):
    reason = 'cancelled'


class WindowUnavailable(RuntimeError):
    """The verified process is alive but temporarily has no visible game window."""


def navigate_core(pid, game_identity, deadline, log_path, *, reader_factory,
                  window, send, interrupted, identity, now=time.monotonic, sleep=time.sleep):
    # The caller supplies an absolute time on this same monotonic clock.
    began = now()
    deadline = min(deadline, began + 60)
    path = Path(log_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
    from menu_control import wait_for_reader, control, ControlProgress
    reader = None
    progress = ControlProgress()
    with path.open('w', encoding='utf-8') as log:
        def emit(event):
            log.write(json.dumps(dict(elapsed=now() - began, **event)) + '\n')
            log.flush()

        def stopped():
            if identity(pid) != game_identity:
                raise RuntimeError('Reserved game exited or identity changed')
            return interrupted()

        def checked_window():
            if stopped():
                raise NavigationCancelled('Menu navigation cancelled')
            try:
                current = window()
            except RuntimeError as error:
                if str(error) == 'Game window not found':
                    raise WindowUnavailable(str(error)) from error
                raise
            if current[0] != pid:
                raise RuntimeError('Menu window belongs to another PID')
            return current

        def checked_read():
            if stopped():
                raise NavigationCancelled('Menu navigation cancelled')
            return reader.sample()

        def checked_send(action, target):
            if stopped():
                raise NavigationCancelled('Menu navigation cancelled')
            if now() >= deadline or checked_window() != target:
                raise RuntimeError('Menu dispatch deadline or window changed')
            return send(action, target)

        try:
            while now() < deadline:
                if stopped():
                    raise NavigationCancelled('Menu navigation cancelled')
                try:
                    target = checked_window()
                except WindowUnavailable:
                    sleep(min(.25, max(0, deadline - now())))
                    continue
                emit(dict(event='window_bound', target=target))
                try:
                    reader, reason = wait_for_reader(reader_factory, checked_window, stopped,
                                                    target=target, deadline=deadline, emit=emit, now=now, sleep=sleep)
                    if reason == 'interrupted':
                        raise NavigationCancelled('Menu navigation cancelled')
                    if reader is None:
                        if reason != 'window_changed':
                            raise RuntimeError('Menu initialization failed: ' + reason)
                    else:
                        result = control(checked_read, checked_send, checked_window, stopped, pid=pid,
                                         target=target, deadline=deadline, max_actions=5, progress=progress,
                                         emit=emit, now=now, sleep=sleep)
                        reason = result['reason']
                        if reason == 'interrupted':
                            raise NavigationCancelled('Menu navigation cancelled')
                        if reason == 'loading_requested':
                            return result
                        # A dispatch error can mean a partial key pair. Never replay it.
                        if reason != 'window_changed':
                            raise RuntimeError('Menu navigation failed: ' + reason)
                except WindowUnavailable:
                    reason = 'window_unavailable'
                finally:
                    if reader is not None:
                        reader.close()
                        reader = None
                progress.window_lost()
                emit(dict(event='window_rebinding', reason=reason, actions=progress.actions,
                          retry_choice=progress.retry_choice))
                sleep(min(.15, max(0, deadline - now())))
            raise RuntimeError('Menu window wait timeout')
        except Exception as error:
            emit(dict(event='error', error=type(error).__name__ + ': ' + str(error)))
            raise
        finally:
            if reader is not None:
                reader.close()


def navigate(pid, game_identity, backend, cancel, deadline, log_path, check_live=None):
    """Live adapters are imported only on explicit launcher invocation."""
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
    from menu_state import MenuReader
    import game_io
    from menu_control import send_confirm
    def interrupted():
        if check_live:check_live()
        return backend.cancelled(cancel) or game_io.interrupted()
    def window():
        # The selected executable can be outside this repository/package. The
        # backend binds the HWND to this reserved PID; navigate_core repeatedly
        # verifies its full creation/image identity before any menu action.
        hwnd=backend.game_window(pid)
        if not hwnd:raise RuntimeError('Game window not found')
        return pid,hwnd
    return navigate_core(pid, game_identity, deadline, log_path,
                         reader_factory=lambda: MenuReader(pid), window=window,
                         send=lambda action,target:send_confirm(action,target,window_lookup=window),
                         interrupted=interrupted,
                         identity=backend.control.identity)
