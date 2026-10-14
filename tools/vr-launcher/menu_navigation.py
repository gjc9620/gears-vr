"""Bounded in-process launcher menu navigation; loading is not player readiness."""
import json
from pathlib import Path
import sys
import time


class NavigationCancelled(RuntimeError):
    reason = 'cancelled'


def navigate_core(pid, game_identity, deadline, log_path, *, reader_factory,
                  window, send, interrupted, identity, now=time.monotonic, sleep=time.sleep):
    # The caller supplies an absolute time on this same monotonic clock.
    began = now()
    deadline = min(deadline, began + 60)
    path = Path(log_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
    from menu_control import wait_for_reader, control
    reader = None
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
            return window()

        def checked_read():
            if stopped():
                raise NavigationCancelled('Menu navigation cancelled')
            return reader.sample()

        def checked_send(action, target):
            if stopped():
                raise NavigationCancelled('Menu navigation cancelled')
            if now() >= deadline or window() != target:
                raise RuntimeError('Menu dispatch deadline or window changed')
            return send(action, target)

        try:
            while now() < deadline:
                if stopped():
                    raise NavigationCancelled('Menu navigation cancelled')
                try:
                    target = checked_window()
                except RuntimeError as error:
                    if str(error) != 'Game window not found':
                        raise
                    sleep(min(.25, max(0, deadline - now())))
                    continue
                if target[0] != pid:
                    raise RuntimeError('Menu window belongs to another PID')
                break
            else:
                raise RuntimeError('Menu window wait timeout')
            emit(dict(event='window_bound', target=target))
            reader, reason = wait_for_reader(reader_factory, checked_window, stopped,
                                            target=target, deadline=deadline, emit=emit, now=now, sleep=sleep)
            if reason == 'interrupted':
                raise NavigationCancelled('Menu navigation cancelled')
            if reader is None:
                raise RuntimeError('Menu initialization failed: ' + reason)
            result = control(checked_read, checked_send, checked_window, stopped, pid=pid,
                             target=target, deadline=deadline, max_actions=5,
                             emit=emit, now=now, sleep=sleep)
            if result['reason'] == 'interrupted':
                raise NavigationCancelled('Menu navigation cancelled')
            if result['reason'] != 'loading_requested':
                raise RuntimeError('Menu navigation failed: ' + result['reason'])
            return result
        except Exception as error:
            emit(dict(event='error', error=type(error).__name__ + ': ' + str(error)))
            raise
        finally:
            if reader is not None:
                reader.close()


def navigate(pid, game_identity, backend, cancel, deadline, log_path):
    """Live adapters are imported only on explicit launcher invocation."""
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
    from menu_state import MenuReader
    import game_io
    from menu_control import send_confirm
    return navigate_core(pid, game_identity, deadline, log_path,
                         reader_factory=lambda: MenuReader(pid), window=game_io.game_window,
                         send=send_confirm,
                         interrupted=lambda: backend.cancelled(cancel) or game_io.interrupted(),
                         identity=backend.control.identity)
