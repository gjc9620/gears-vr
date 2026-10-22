"""Candidate calibration gate before activating Native or entering combat."""
import json
import os
from pathlib import Path
import subprocess
import time
import uuid

from armspan_calibration import Session, default_path
from armspan_wire import Reader, Unavailable


def event(phase, progress=0):
    print(json.dumps({'armspan_calibration': {'phase': phase, 'progress': progress}}), flush=True)


def prepare(args, game, owner, control, runner, cancel, root, *, path=None,
            reader_factory=Reader, popen=subprocess.Popen, clock=time.monotonic, sleep=time.sleep, timeout=90):
    path = Path(path) if path is not None else default_path()
    session = Session(path, bool(getattr(args, 'remeasure_armspan', False)))
    if not session.required:
        event('reused', 1)
        return dict(path=str(path), mode='reused', saved=False, record_state=session.previous.state)
    if os.environ.get('GEARSVR_TEST_WINDOWS_BEHIND') == '1':
        raise ValueError('Personal armspan measurement is disabled in autonomous background tests')
    if not getattr(args, 'startup_menu_window', 0):
        raise ValueError('Armspan startup calibration requires the owned startup menu')
    if control.identity(game['pid']) != game or control.identity(owner['pid']) != owner:
        raise RuntimeError('Calibration game/owner identity changed')
    prefix = Path(root) / 'analysis/runtime' / ('armspan-' + str(game['pid']) + '-' + uuid.uuid4().hex[:10])
    prefix.parent.mkdir(parents=True, exist_ok=True)
    command = [str(args.xr_build_dir.resolve() / 'OpenXRPoseProbe.exe'), '0', str(prefix.with_suffix('.csv')),
               '--stereo-game-pid', str(game['pid']), '--continuous-session', '--controller-input',
               '--owner-pid', str(owner['pid']), '--owner-creation-time', str(owner['creation_time']),
               '--startup-menu-window', str(args.startup_menu_window), str(game['pid'])]
    child = reader = None
    cleanup = {}
    cancelled = False
    event('waiting')
    try:
        with prefix.with_suffix('.out.jsonl').open('w', encoding='utf-8') as stdout, \
                prefix.with_suffix('.err.txt').open('w', encoding='utf-8') as stderr:
            child = popen(command, stdout=stdout, stderr=stderr, env=runner.xr_environment(os.environ),
                          creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            publisher = control.identity(child.pid)
            if (publisher is None or Path(publisher['image']).resolve() != Path(command[0]).resolve()):
                raise RuntimeError('Owned calibration XR identity unavailable')
            reader = reader_factory(game, publisher, control.identity)
            deadline, last_percent = clock() + timeout, -1
            while clock() < deadline:
                if cancel():
                    cancelled = True
                    break
                if child.poll() is not None:
                    raise RuntimeError('Calibration XR exited before measurement')
                if control.identity(owner['pid']) != owner:
                    raise RuntimeError('Calibration owner changed')
                try:
                    sample = reader.read()
                except FileNotFoundError:
                    sample = None  # Publisher startup is allowed, never an old sample.
                except Unavailable:
                    session.measurement.reset_window()
                    sample = None
                if sample is not None:
                    # Commit a completed measurement immediately. A later XR
                    # cleanup failure blocks Native without rolling back this record.
                    session.observe(sample, reader.now())
                percent = int(session.measurement.progress * 10)
                if percent != last_percent:
                    event('holding' if percent else 'waiting', percent / 10)
                    last_percent = percent
                if session.saved:
                    event('saved', 1)
                    break
                sleep(.01)
            else:
                raise TimeoutError('Armspan measurement timed out; prior record preserved')
    finally:
        try:
            if reader is not None:
                reader.close()
        finally:
            if child is not None:
                runner.cleanup_owned_xr(child, child.pid, game['pid'], args.cleanup_timeout, control, cleanup)
                if (cleanup.get('xr_exit_code') != 0 or cleanup.get('xr_cleanup_error') or
                        cleanup.get('xr_forced_cleanup_reason')):
                    raise RuntimeError('Calibration XR did not close cleanly; Native remains inactive')
    if cancelled:
        event('cancelled')
        return None
    return dict(path=str(path), mode='measured', saved=True, record_state=session.previous.state,
                xr_cleanup=cleanup, log=str(prefix.with_suffix('.out.jsonl')))
