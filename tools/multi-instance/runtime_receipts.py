"""Bounded reads of concurrently published private runtime receipts."""
import json
import shutil
import time
import math
from pathlib import Path


def confirmed_prelaunch_rejection(index, owner, selected):
    """Only a typed no-start proof from this exact owned launcher qualifies."""
    if not owner or not index or index.get('owner_identity')!=owner:return False
    cleanup=index.get('reservation_failure_cleanup',{})
    proof=cleanup.get('proof',{})
    if (cleanup.get('status')!='prelaunch_rejected' or cleanup.get('no_game_started') is not True or
            proof.get('kind')!='startup_prelaunch_rejection' or proof.get('schema')!=1 or
            proof.get('stage')!='python_discovery' or
            not all(proof.get(k) is False for k in ('game_creation_attempted','injection_attempted','prime_attempted'))):return False
    receipt=Path(proof.get('launch_receipt','')).resolve()
    return (receipt.parent==selected.resolve() and receipt.name.endswith('.birth.json') and
            not receipt.exists() and not any(selected.glob('*.birth.json')))


class ResourceUnavailable(RuntimeError):
    """No slot/memory admitted; no worker has been started."""


def first_admission(guard, check_fresh, probe, commit, *, busy_errors=(),
                    wait_seconds=60., clock=time.monotonic, sleep=time.sleep):
    """Retry only pre-admission contention, sleeping outside the short mutex."""
    if (isinstance(wait_seconds,bool) or not isinstance(wait_seconds,(int,float)) or
            not math.isfinite(wait_seconds) or not 0<=wait_seconds<=600):
        raise ValueError('First admission wait must be finite, within 0..600 seconds')
    deadline=clock()+wait_seconds
    committing=False
    while True:
        try:
            with guard():
                check_fresh()
                probe_result=probe()
                committing=True
                return commit(probe_result)
        except (ResourceUnavailable,*busy_errors):
            if committing:raise  # Never repeat spawn/registration, even on failure.
            remaining=deadline-clock()
            if remaining<=0:raise
        sleep(min(1.,remaining))


def require_disk(path, *, minimum_bytes=4*1024**3, usage=shutil.disk_usage):
    """Admission only; never stops a live instance or deletes files."""
    value=usage(path)
    receipt={'check':'disk_admission','path':str(path),'free_bytes':value.free,
             'minimum_free_bytes':minimum_bytes,'admitted':value.free>=minimum_bytes}
    if not receipt['admitted']:
        # Keep failure details in the caller's existing stdout/stderr. A full
        # evidence volume cannot be relied on to publish another disk receipt.
        raise RuntimeError(json.dumps(receipt)+'; insufficient evidence-volume space; no new game started; continue independent work and retry after space is released')
    return receipt


def read_json(path, *, attempts=5, sleep=time.sleep):
    for attempt in range(attempts):
        try:
            value = json.loads(path.read_text(encoding='utf-8'))
            if isinstance(value, dict):
                return value
        except (OSError, ValueError):
            pass
        if attempt + 1 < attempts:
            sleep(.01)
    return None  # Unknown, never evidence that an owned process exited.


class GameReceipt:
    def __init__(self):
        self.game = None
        self.index = None

    def observe(self, selected, control):
        value = read_json(selected / 'launcher-session.json')
        if value is not None:
            self.index = value
        candidates = []
        if value:
            candidates.append(value.get('game_identity') or
                              value.get('reservation_failure_cleanup', {}).get('game_identity'))
        if self.game is None:
            for birth in sorted(selected.glob('*.birth.json')):
                receipt = read_json(birth)
                if receipt:
                    candidates.append(receipt.get('game_identity'))
        for game in candidates:
            if (not isinstance(game, dict) or
                    any(type(game.get(k)) is not int or game[k] <= 0 for k in ('pid','creation_time')) or
                    not isinstance(game.get('image'),str) or not game['image']):
                continue
            if self.game is not None:
                if game != self.game:
                    raise RuntimeError('Private game receipt changed identity')
            else:
                # A durable birth may first be read after normal process exit.
                # Window mutation still requires an exact live match at use.
                self.game = game.copy()
        return self.game


def wait_launcher(child, control, identity, *, sleep=time.sleep):
    """Request normal restoration and retain ownership until launcher exits."""
    while child.poll() is None:
        try:
            if identity and control.identity(child.pid) == identity:
                control.signal('Local\\GearsVR.RepositoryLauncher.Stop.' + str(child.pid))
        except OSError:
            pass  # Retain ownership while the normal stop event is unavailable.
        sleep(.25)
