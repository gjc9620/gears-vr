"""Choose a current PathChoice using its reflected trigger-key arrays."""
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import struct
import time

from core import atomic_json


KEYS = {'One': (49, 2, False), 'Two': (50, 3, False), 'Q': (81, 0x10, False),
        'E': (69, 0x12, False), 'A': (65, 0x1e, False), 'D': (68, 0x20, False),
        'Left': (37, 0x4b, True), 'Right': (39, 0x4d, True), 'Enter': (13, 0x1c, False)}


def pick_key(tables, side, mouse_keys):
    if side not in ('left', 'right'):
        raise ValueError('Unsupported path side')
    own = tables['LeftTriggerKeys' if side == 'left' else 'RightTriggerKeys']
    other = set(tables['RightTriggerKeys' if side == 'left' else 'LeftTriggerKeys']) | set(tables['StartKeys'])
    for key in own:
        if key not in other and (key in mouse_keys or key in KEYS):
            return key
    raise RuntimeError('No supported, unambiguous scoped key for this path choice')


def choose(observer, report, side):
    from live import MOUSE, fingerprint, registered
    if observer.action_owner != observer.identities['owner_identity'] or not observer.sampler:
        raise RuntimeError('Path selection requires an owned gameplay session')
    def state():
        observer.verify()
        value = observer.reader.sample(include_owner=True, expected_controller=observer.sampler.controller,
                                       expected_local_player=observer.sampler.local)
        top = value.get('top') or {}
        if (top.get('tag'), top.get('cls'), top.get('input_mode')) != ('DeltaChoosePath', 'UIScene_PathChoice', 4):
            raise RuntimeError('Exact PathChoice scene is no longer active')
        if not value.get('consistent_reread') or not value.get('read_only') or value.get('input_block_count') != 0:
            raise RuntimeError('PathChoice input is blocked')
        r, reader = observer.reader.r, observer.reader
        scene = registered(r, int(top['address'], 16))
        tables, sources = {}, []
        for name in ('LeftTriggerKeys', 'RightTriggerKeys', 'StartKeys'):
            prop = reader.prop(scene, name, 'ArrayProperty')
            data, count, inner = reader.array(scene, name, 'NameProperty', 32)
            if struct.unpack('<II', r.read(inner['address'] + 0x70, 8)) != (1, 8):
                raise RuntimeError('PathChoice key element layout mismatch')
            address = scene['address'] + prop['offset']
            header = r.read(address, 16)
            raw = r.read(data, count * 8) if count else b''
            names = []
            for index, number in struct.iter_unpack('<II', raw):
                if number:
                    raise RuntimeError('Numbered PathChoice input key is unsupported')
                names.append(r.name(index))
            if len(names) != len(set(names)):
                raise RuntimeError('Duplicate PathChoice input keys')
            tables[name] = names
            sources.extend([(address, header)] + ([(data, raw)] if count else []))
        if any(r.read(address, len(raw)) != raw for address, raw in sources):
            raise RuntimeError('PathChoice trigger arrays changed')
        return value, tables, hashlib.sha256(b''.join(raw for _, raw in sources)).hexdigest()
    before, tables, digest = state()
    proof_path = report.directory / ('path-choice-' + str(report.samples) + '.json')
    atomic_json(proof_path, dict(state=before, tables=tables, source_bytes_sha256=digest, side=side))
    key = pick_key(tables, side, MOUSE)
    time.sleep(.2)
    again, current_tables, current_digest = state()
    if fingerprint(before) != fingerprint(again) or tables != current_tables or digest != current_digest:
        raise RuntimeError('PathChoice changed before dispatch; no key sent')
    identity = observer.identities['game_identity']
    window = observer.backend.game_window(identity['pid'])
    desktop = observer.desktop()
    if desktop['occlusion'] or desktop['foreground'] == window:
        raise RuntimeError('PathChoice test window is foreground or occludes another application')
    if key in MOUSE:
        down, up, down_flags, up_flags = MOUSE[key]
        rect = w.RECT()
        if not observer.user.GetClientRect(window, c.byref(rect)):
            raise c.WinError(c.get_last_error())
        lp = rect.right // 2 | ((rect.bottom // 2) << 16)
        release_lp = lp
    else:
        vk, scan, extended = KEYS[key]
        down, up, down_flags, up_flags = 0x100, 0x101, vk, vk
        lp = 1 | (scan << 16) | (int(extended) << 24)
        release_lp = lp | 0xc0000000
    observer.verify()
    if (report.directory / 'stop').exists() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
        raise RuntimeError('PathChoice cancelled before dispatch')
    if not observer.user.PostMessageW(window, down, down_flags, lp):
        raise c.WinError(c.get_last_error())
    try:
        report.event('path_choice_key_down', side=side, key=key, hwnd=window, proof=str(proof_path))
        time.sleep(.08)
    finally:
        if (observer.backend.control.identity(identity['pid']) != identity or
                observer.backend.game_window(identity['pid']) != window):
            raise RuntimeError('PathChoice release target identity changed')
        if not observer.user.PostMessageW(window, up, up_flags, release_lp):
            raise c.WinError(c.get_last_error())
        report.event('path_choice_key_up', side=side, key=key, hwnd=window)
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        row = report.observe(observer.sample())
        if ((row.get('menu') or {}).get('top') or {}).get('tag') != 'DeltaChoosePath' and not row['errors']:
            report.cover('branch_choice', 'passed', side=side, key=key, sample=row['sample_id'],
                         scope='Reviewed trigger dispatched once and PathChoice scene closed; destination progression is separate')
            return
        time.sleep(.2)
    raise RuntimeError('PathChoice did not acknowledge the single input; no blind retry')
