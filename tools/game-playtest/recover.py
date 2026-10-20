"""Finish an interrupted playtest's exact profile/settings restoration."""
import argparse
import json
from pathlib import Path
import time

from core import atomic_json
from run import ProtectedFiles, open_lifecycle_lock, wait_until


def recover(directory, seconds):
    from live import launcher
    root = Path(directory).resolve()
    report_path = root / 'report.json'
    report = json.loads(report_path.read_text(encoding='utf8'))
    lifecycle = report['provenance'].get('lifecycle') or {}
    if lifecycle.get('protected_files_restored'):
        return dict(already_restored=True)
    if not (lifecycle.get('native_cleanup_verified') is True and
            lifecycle.get('entry_restore_verified') is True and lifecycle.get('owner_exit_code') == 0):
        raise RuntimeError('Original run lacks verified native/entry/owner cleanup')
    backend = launcher.Backend()
    identities = [report['provenance'].get(k) for k in ('game_identity', 'owner_identity')]
    for identity in identities:
        if identity and backend.control.identity(identity['pid']) == identity:
            raise RuntimeError('Original owned process is still alive')
    lock = None
    def available():
        nonlocal lock
        if backend.game_pids():
            return False
        lock = open_lifecycle_lock(backend, create=True)
        if not lock:
            return False
        if backend.game_pids():
            backend.control.k.CloseHandle(lock)
            lock = None
            return False
        return True
    atomic_json(root / 'recovery.json', dict(phase='waiting_for_idle', original_run=str(report_path)))
    if not wait_until(seconds, available):
        raise RuntimeError('Restoration remains pending while another game/test owns the workspace')
    try:
        def stopped():
            if backend.game_pids():
                raise RuntimeError('A game exists during restoration')
        store = ProtectedFiles.__new__(ProtectedFiles)
        store.directory = root / 'backup'
        store.rows = json.loads((store.directory / 'manifest.json').read_text(encoding='utf8'))['files']
        store.require_stopped = stopped
        receipts = store.restore()
        report['provenance']['lifecycle'].update(protected_files_restored=True, recovered=True)
        atomic_json(report_path, report)
        result = dict(phase='restored', original_run=str(report_path), receipts=receipts)
        atomic_json(root / 'recovery.json', result)
        return result
    finally:
        backend.control.k.CloseHandle(lock)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--wait-seconds', type=float, default=600)
    args = parser.parse_args()
    if not 0 <= args.wait_seconds <= 3600:
        parser.error('Invalid wait bound')
    result = recover(args.directory, args.wait_seconds)
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
