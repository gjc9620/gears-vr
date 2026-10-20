"""Read-only game observations and identity-scoped ordinary HWND input."""
import ctypes as c
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import struct
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
for folder in ('tools/vr-launcher', 'tools/native-capture', 'tools/game-io', 'tools/debug-launch',
               'tools/package-inspector', 'tools/quiet-startup'):
    sys.path.insert(0, str(ROOT / folder))
import launcher
import background_input
from anchor_config import LiveRegistryAnchorSampler, acquire_anchor_sample
from inspect_ui_state import registered
from menu_control import reader_process_identity, fingerprint
from menu_state import (MenuReader, RegistrySnapshotChanged, SnapshotChanged,
                        InventoryChanged, DialogStorageChanged, ForcedRegistryRefreshChanged)
from resolve_camera import Resolver
from window_snapshot import WindowSnapshot
from window_occlusion import find_occlusion
from core import gameplay, controllable as controllable_state


class SessionChanged(RuntimeError):
    pass


class InputNotReady(RuntimeError):
    """No down event was sent; a fresh observation may safely precede retry."""


class InputInterrupted(RuntimeError):
    """A pulse was released after an observation boundary; outcome is unknown."""


class MouseReleaseFailed(RuntimeError):
    """The run must stop because a posted mouse-down lacks a verified release."""


class InteractionReleaseFailed(MouseReleaseFailed):
    """The owned game must close after an unverified interaction key release."""


class Observer:
    def __init__(self, session_path, *, action_owner=None):
        self.path = Path(session_path).resolve()
        self.session = json.loads(self.path.read_text(encoding='utf8'))
        if self.session.get('kind') != 'continuous_strict_stereo':
            raise ValueError('An exact continuous stereo session report is required')
        self.backend = launcher.Backend()
        self.identities = {k: self.session[k] for k in ('game_identity', 'owner_identity', 'xr_identity')}
        self.started = time.monotonic()
        self.reader = self.resolver = self.sampler = None
        self.action_owner = action_owner
        self.user = c.WinDLL('user32', use_last_error=True)
        self.user.GetForegroundWindow.restype = w.HWND
        self.user.GetAsyncKeyState.argtypes = [c.c_int]
        self.user.GetAsyncKeyState.restype = c.c_short
        self.user.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
        self.user.PostMessageW.restype = w.BOOL
        self.user.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
        self.user.GetClientRect.restype = w.BOOL
        self.user.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
        self.user.GetClassNameW.restype = c.c_int
        self.allow_manual_preview_activation = os.environ.get('GEARSVR_TEST_ALLOW_MANUAL_PREVIEW_ACTIVATION') == '1'
        self.manual_preview_windows = set()
        self.windows = WindowSnapshot()
        self.verify()

    def verify(self):
        current = json.loads(self.path.read_text(encoding='utf8'))
        if (current.get('session_id') != self.session['session_id'] or
                any(current.get(k) != v for k, v in self.identities.items())):
            raise SessionChanged('Session identities were replaced')
        for identity in self.identities.values():
            self.backend.runner.checked_identity(self.backend.control, identity)
        if current.get('phase') != 'running':
            raise SessionChanged('Session is no longer running: ' + str(current.get('phase')))
        return current

    def close_pose(self):
        if self.resolver:
            self.resolver.p.close()
        self.resolver = self.sampler = None

    def closed_normally(self):
        current = json.loads(self.path.read_text(encoding='utf8'))
        return (current.get('session_id') == self.session['session_id']
                and all(current.get(k) == v for k, v in self.identities.items())
                and current.get('phase') == 'stopped'
                and current.get('native_cleanup_verified') is True
                and current.get('entry_restore_verified') is True
                and current.get('xr_exit_code') == 0)

    def close(self):
        self.close_pose()
        if self.reader:
            self.reader.close()
        self.reader = None

    def menu(self):
        if self.reader is None:
            self.reader = MenuReader(self.identities['game_identity']['pid'])
        identity = self.identities['game_identity']
        if reader_process_identity(self.reader) != (identity['pid'], identity['creation_time']):
            raise SessionChanged('Menu reader game identity mismatch')
        return self.reader.sample()

    def _int_property(self, obj, name):
        r = self.resolver
        before = self.sampler.fresh(obj['address'])
        if before != obj:
            raise RuntimeError('Object identity changed before ' + name)
        prop = r.property(obj['cls'], name, True)
        if prop['type'] != 'IntProperty' or not 0 <= prop['offset'] < 1024 * 1024:
            raise RuntimeError('Unsupported integer property ' + name)
        metadata = r.read(prop['address'] + 0x70, 0x28)
        if struct.unpack_from('<II', metadata) != (1, 4):
            raise RuntimeError('Integer property dimension/size mismatch')
        if struct.unpack_from('<I', metadata, 0x24)[0] != prop['offset']:
            raise RuntimeError('Integer property offset changed')
        address = obj['address'] + prop['offset']
        raw = r.read(address, 4)
        if (r.read(address, 4) != raw or r.read(prop['address'] + 0x70, 0x28) != metadata
                or self.sampler.fresh(obj['address']) != obj):
            raise RuntimeError('Property sample changed: ' + name)
        return struct.unpack('<i', raw)[0]

    def _byte_property(self, obj, name):
        r = self.resolver
        if self.sampler.fresh(obj['address']) != obj:
            raise RuntimeError('Object identity changed before ' + name)
        prop = r.property(obj['cls'], name, True)
        if prop['type'] != 'ByteProperty' or not 0 <= prop['offset'] < 1024 * 1024:
            raise RuntimeError('Unsupported byte property ' + name)
        metadata = r.read(prop['address'] + 0x70, 0x28)
        if (struct.unpack_from('<II', metadata) != (1, 1) or
                struct.unpack_from('<I', metadata, 0x24)[0] != prop['offset']):
            raise RuntimeError('Byte property layout mismatch')
        address = obj['address'] + prop['offset']
        raw = r.read(address, 1)
        if (r.read(address, 1) != raw or r.read(prop['address'] + 0x70, 0x28) != metadata
                or self.sampler.fresh(obj['address']) != obj):
            raise RuntimeError('Byte property changed: ' + name)
        return raw[0]

    def sample(self):
        session = self.verify()
        row = dict(elapsed=time.monotonic() - self.started,
                   game_identity=self.identities['game_identity'], errors=[])
        def failed(component, error):
            row['errors'].append(dict(component=component, type=type(error).__name__, message=str(error)))
        try:
            row['menu'] = self.menu()
        except SessionChanged:
            raise
        except (OSError, RuntimeError, ValueError) as error:
            failed('menu', error)
            if self.reader:
                self.reader.close()
            self.reader = None
        try:
            fields = launcher.current_native_status(self.backend, session)
            native = {}
            for key in ('published', 'epoch_submit_total', 'world_generation',
                        'world_epoch', 'resource_generation', 'present_calls', 'started_qpc',
                        'reference_generation', 'owned_ui_faults', 'owned_material_faults',
                        'owned_ui_semantic_incomplete', 'mirror_inventory_rejected',
                        'anchor_submission_rejected', 'telemetry_present_failures',
                        'owned_ui_first_fault', 'owned_material_first_fault'):
                value = fields.get(key)
                if value is not None:
                    native[key] = int(value)
            # Current candidates expose one of these names. Preserve the source.
            if 'world_generation' not in native:
                for key in ('world_epoch',):
                    if key in native:
                        native['world_generation'] = native[key]
                        native['world_generation_source'] = key
                        break
            row['native'] = native
            if fields.get('owned_ui_first_fault_name') is not None:
                native['owned_ui_first_fault_name'] = fields['owned_ui_first_fault_name']
        except (OSError, RuntimeError, ValueError) as error:
            failed('native_status', error)
        if gameplay(row.get('menu') or {}):
            try:
                if self.sampler is None:
                    profile = json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text(encoding='utf8'))
                    self.resolver = Resolver(self.identities['game_identity']['pid'], profile)
                    self.sampler = LiveRegistryAnchorSampler(self.resolver, cinematic_camera=True)
                anchor, evidence = acquire_anchor_sample(self.sampler)
                proof = (anchor.get('guard_before') or {}).get('target_proof') or {}
                if type(proof.get('cinematic')) is not bool:
                    raise RuntimeError('Current certified cinematic state is unavailable')
                row['cinematic'] = proof['cinematic']
                row['view_target_identity'] = proof.get('identity')
                pawn = self.sampler.fresh(self.sampler.pawn)
                values = anchor['values']
                row['pawn_identity'] = pawn
                row['pose'] = dict(position=values['pawn_location'],
                                   yaw_ru=values['controller_rotation'][1],
                                   controller_rotation_ru=values['controller_rotation'],
                                   camera_position=values['camera_cache_pov']['location'],
                                   fov=values['camera_cache_pov']['fov'],
                                   pawn_rotation_ru=values['pawn_rotation'],
                                   camera_rotation_ru=values['camera_cache_pov']['rotation_ru'],
                                   camera_timestamp=anchor['camera_timestamp'])
                row['health'] = self._int_property(pawn, 'Health')
                row['special_move_raw'] = self._byte_property(pawn, 'SpecialMove')
                for name in ('CoverType', 'CoverAction'):
                    try:
                        row.setdefault('cover_state_raw', {})[name] = self._byte_property(pawn, name)
                    except (OSError, RuntimeError, ValueError) as error:
                        row.setdefault('optional_state_unavailable', {})[name] = str(error)
                row['anchor_acquisition_ms'] = (evidence['finished_perf_counter_ns'] -
                                                evidence['started_perf_counter_ns']) / 1e6
                weapon_prop = self.resolver.property(pawn['cls'], 'Weapon', True)
                if weapon_prop['type'] != 'ObjectProperty':
                    raise RuntimeError('Unsupported Pawn.Weapon type')
                weapon_ptr = self.resolver.u64(pawn['address'] + weapon_prop['offset'])
                if weapon_ptr:
                    weapon = self.sampler.fresh(weapon_ptr)
                    if self.sampler.derives(weapon['cls'], 'WarWeapon'):
                        row['weapon_identity'] = weapon
                        row['ammo_used'] = self._int_property(weapon, 'AmmoUsedCount')
                        row['spare_ammo'] = self._int_property(weapon, 'SpareAmmoCount')
                if self.sampler.fresh(pawn['address']) != pawn:
                    raise RuntimeError('Pawn changed after weapon observation')
            except (OSError, RuntimeError, ValueError) as error:
                failed('player_state', error)
                # Never use a partial player/weapon observation for input.
                for key in ('pose', 'pawn_identity', 'health', 'weapon_identity', 'ammo_used', 'spare_ammo', 'special_move_raw'):
                    row.pop(key, None)
                self.close_pose()
        self.verify()
        return row

    def bindings(self, *, extra_keys=()):
        self.verify()
        if not self.sampler or not self.reader:
            raise RuntimeError('Fresh gameplay observation is required before bindings')
        reader, r = self.reader, self.reader.r
        pc = registered(r, self.sampler.controller)
        inp = registered(r, reader.pointer(pc, 'PlayerInput'))
        data, count, inner = reader.array(inp, 'Bindings', 'StructProperty', 1024)
        prop = reader.prop(inp, 'Bindings', 'ArrayProperty')
        header_address = inp['address'] + prop['offset']
        header = r.read(header_address, 16)
        dim, stride = struct.unpack('<II', r.read(inner['address'] + 0x70, 8))
        if dim != 1 or not 16 <= stride <= 128:
            raise RuntimeError('Unsupported input binding stride')
        typ = r.struct_type(dict(address=inner['address'], type='StructProperty'))
        name, command = r.property(typ, 'Name'), r.property(typ, 'Command')
        if name['type'] != 'NameProperty' or command['type'] != 'StrProperty':
            raise RuntimeError('Unsupported binding fields')
        raw = r.read(data, count * stride)
        result, reads = {}, []
        keys = ('LeftMouseButton', 'RightMouseButton', 'MiddleMouseButton',
                'ThumbMouseButton', 'ThumbMouseButton1', 'ThumbMouseButton2', *extra_keys)
        for i in range(count):
            index, number = struct.unpack_from('<II', raw, i * stride + name['offset'])
            key = r.name(index)
            if key not in keys:
                continue
            pointer, size, capacity = struct.unpack_from('<Qii', raw, i * stride + command['offset'])
            if key in result or number or not pointer or not 0 < size <= capacity <= 2048:
                raise RuntimeError('Invalid or duplicate mouse binding')
            text = r.read(pointer, size * 2)
            reads.append((pointer, text))
            if not text.endswith(b'\0\0'):
                raise RuntimeError('Unterminated mouse binding')
            result[key] = text.decode('utf-16-le')[:-1]
        if (r.read(header_address, 16) != header or r.read(data, count * stride) != raw
                or any(r.read(p, len(v)) != v for p, v in reads)
                or registered(r, inp['address']) != inp or registered(r, pc['address']) != pc):
            raise RuntimeError('Mouse bindings changed during acquisition')
        return result

    def desktop(self):
        snapshot = self.windows.capture()
        pids = {v['pid'] for v in self.identities.values()}
        pids.update(launcher.instance_policy.background_process_pids(identity=self.backend.control.identity))
        hits = find_occlusion(snapshot['windows'], pids, snapshot['excluded_hwnds'])
        foreground = int(self.user.GetForegroundWindow() or 0)
        approved = []
        if self.allow_manual_preview_activation:
            xr = self.identities['xr_identity']
            self.backend.runner.checked_identity(self.backend.control, xr)
            previews = set()
            for row in snapshot['windows']:
                if row['pid'] != xr['pid']:
                    continue
                name = c.create_unicode_buffer(256)
                if self.user.GetClassNameW(row['hwnd'], name, len(name)) and name.value == 'OpenXR Simulator':
                    previews.add(row['hwnd'])
            self.manual_preview_windows.intersection_update(previews)
            # Startup still uses the tested nonactivating behind-window policy.
            # Explicit human authorization permits observing this preview in
            # foreground, including its retained Z order after switching away.
            if foreground in previews:
                self.manual_preview_windows.add(foreground)
            approved = [hit for hit in hits if hit['test_hwnd'] in self.manual_preview_windows]
            hits = [hit for hit in hits if hit['test_hwnd'] not in self.manual_preview_windows]
        return dict(occlusion=hits, foreground=foreground, traversed=snapshot['traversed_windows'],
                    automatic_test_pids=sorted(pids), user_activated_preview_occlusion=approved,
                    user_activated_preview_hwnds=sorted(self.manual_preview_windows))

    def pulse(self, key, seconds, *, expected_bindings, stop_requested=lambda: False,
              emit=lambda kind, **value: None, during=None, held_keys=()):
        if self.action_owner != self.identities['owner_identity']:
            raise RuntimeError('Input requires the launcher identity created by this test run')
        maximum = 2 if key == 'RightMouseButton' and during is not None and expected_bindings.get(key) == 'Game_Target' else 1
        if key not in MOUSE or not .02 <= seconds <= maximum:
            raise ValueError('Unsupported bounded mouse pulse')
        holds = getattr(self, '_mouse_holds', {})
        self._mouse_holds = holds
        if held_keys:
            if (key != 'LeftMouseButton' or tuple(held_keys) != ('RightMouseButton',)
                    or set(holds) != {'RightMouseButton'}
                    or expected_bindings.get('LeftMouseButton') != 'Game_Shoot'
                    or expected_bindings.get('RightMouseButton') != 'Game_Target'):
                raise InputNotReady('Aimed fire requires this observer\'s current owned aim hold')
        elif holds:
            raise InputNotReady('An unrelated mouse hold is already active')
        before = self.sample()
        if before['errors'] or not controllable_state(before):
            raise InputNotReady('Current state does not admit gameplay input')
        background = background_input.change(self.identities['game_identity']['pid'], 'status',
                      expected_creation=self.identities['game_identity']['creation_time'])
        if background['state'] != 'original':
            raise RuntimeError('Physical background controller input must remain disabled')
        if self.bindings() != expected_bindings:
            raise RuntimeError('Mouse bindings changed before dispatch')
        window = self.backend.game_window(self.identities['game_identity']['pid'])
        if held_keys and holds['RightMouseButton'] != window:
            raise InputNotReady('Aim hold belongs to a different window')
        desk = self.desktop()
        if desk['occlusion'] or desk['foreground'] == window:
            emit('input_desktop_guard_rejected', desktop=desk, game_window=int(window),
                 game_identity=self.identities['game_identity'],
                 scope='No input dispatched; observation only, no window recovery')
            raise RuntimeError('Owned game is foreground or covers another application')
        rect = w.RECT()
        if not self.user.GetClientRect(window, c.byref(rect)):
            raise c.WinError(c.get_last_error())
        pos = rect.right // 2 | ((rect.bottom // 2) << 16)
        down, up, down_flags, up_flags = MOUSE[key]
        if held_keys:
            down_flags |= MOUSE['RightMouseButton'][2]
            up_flags |= MOUSE['RightMouseButton'][2]
        held = False
        held_observation = None
        down_at = None
        try:
            self.verify()
            if stop_requested() or self.user.GetAsyncKeyState(0x77) & 0x8000:
                raise RuntimeError('Test cancelled before input')
            if not self.user.PostMessageW(window, down, down_flags, pos):
                raise c.WinError(c.get_last_error())
            held = True
            holds[key] = window
            down_at = time.monotonic()
            emit('input_down', key=key, hwnd=window, message=down, flags=down_flags,
                 elapsed=time.monotonic() - self.started)
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                self.verify()
                if (stop_requested() or self.user.GetAsyncKeyState(0x77) & 0x8000
                        or int(self.user.GetForegroundWindow() or 0) == window):
                    raise RuntimeError('Test cancelled during input')
                try:
                    current_menu = self.menu()
                except SessionChanged:
                    raise
                except (OSError, RuntimeError, ValueError) as error:
                    self.reader.close()
                    self.reader = None
                    raise InputInterrupted('Menu observation changed during pulse: ' + str(error)) from error
                if not gameplay(current_menu):
                    raise InputInterrupted('Gameplay state changed during input')
                if during is not None and held_observation is None and time.monotonic() - down_at >= .1:
                    held_observation = during(self.sample())
                time.sleep(min(.03, max(0, deadline - time.monotonic())))
        finally:
            if held:
                # Release only the same process/window; PID reuse cannot receive it.
                identity = self.identities['game_identity']
                if (self.backend.control.identity(identity['pid']) != identity or
                        self.backend.game_window(identity['pid']) != window):
                    raise MouseReleaseFailed('Identity changed; release target is no longer certifiable')
                if not self.user.PostMessageW(window, up, up_flags, pos):
                    raise MouseReleaseFailed('Mouse release post failed: ' + str(c.get_last_error()))
                holds.pop(key, None)
                emit('input_up', key=key, hwnd=window, message=up, flags=up_flags,
                     elapsed=time.monotonic() - self.started)
        return dict(key=key, seconds=seconds, release_posted=True,
                    actual_hold_seconds=time.monotonic() - down_at,
                    held_observation=held_observation)

    def menu_key(self, action, state, *, emit=lambda kind, **value: None):
        if self.action_owner != self.identities['owner_identity']:
            raise RuntimeError('Menu input requires this run\'s owned launcher')
        if action not in ('escape', 'down', 'confirm'):
            raise ValueError('Unsupported reviewed menu key')
        top, leaf = state.get('top') or {}, state.get('focused_leaf') or {}
        tag = top.get('tag')
        allowed = action == 'escape' and gameplay(state)
        if tag == 'DeltaPause-Campaign-Host' and top.get('input_mode') == 1:
            allowed = ((action == 'escape' and leaf.get('name') in ('LBtnResumeSingle', 'LBtnChapterSingle'))
                       or (action == 'down' and leaf.get('name') == 'LBtnResumeSingle')
                       or (action == 'confirm' and leaf.get('name') == 'LBtnChapterSingle'))
        if tag == 'DeltaGameover':
            allowed = (action == 'confirm' and top.get('cls') == 'WarUIScene' and top.get('input_mode') == 1
                       and state.get('player_index') == 0 and leaf.get('name') == 'btnCheckpoint'
                       and leaf.get('cls') == 'UILabelButton')
        if tag == 'WarfareConfirmation' and top.get('cls') == 'UIScene_Confirmation':
            text = state.get('dialog_text') or {}
            allowed = (action in ('escape', 'confirm') and
                       text.get('Title') == '<Strings:WarfareGame.Generic.ConfirmCheckPointLoad_Title>' and
                       text.get('MessageText') == '<Strings:WarfareGame.Generic.ConfirmCheckPointLoad>')
        if not allowed or not state.get('consistent_reread') or not state.get('read_only') or state.get('input_block_count') != 0:
            raise RuntimeError('Menu action is outside reviewed semantic choices')
        original = fingerprint(state)
        time.sleep(.2)
        if fingerprint(self.menu()) != original:
            raise RuntimeError('Menu state changed before key dispatch')
        self.verify()
        if self.user.GetAsyncKeyState(0x77) & 0x8000:
            raise RuntimeError('Scenario cancelled before menu key')
        window = self.backend.game_window(self.identities['game_identity']['pid'])
        desktop = self.desktop()
        if desktop['occlusion'] or desktop['foreground'] == window:
            raise RuntimeError('Menu test is foreground or occludes another application')
        vk, scan, extended = {'escape': (27, 1, False), 'down': (40, 0x50, True),
                              'confirm': (13, 0x1c, False)}[action]
        lp = 1 | (scan << 16) | (int(extended) << 24)
        if not self.user.PostMessageW(window, 0x100, vk, lp):
            raise c.WinError(c.get_last_error())
        try:
            emit('menu_key_down', action=action, hwnd=window, choice=(tag, leaf.get('name')))
            time.sleep(.02)
        finally:
            identity = self.identities['game_identity']
            if self.backend.control.identity(identity['pid']) != identity:
                raise SessionChanged('Game identity changed before menu key release')
            if not self.user.PostMessageW(window, 0x101, vk, lp | 0xc0000000):
                raise c.WinError(c.get_last_error())
            emit('menu_key_up', action=action, hwnd=window)

    def interaction_key(self, key, expected_event, *, emit=lambda kind, **value: None):
        """Single edge action bound to the current controller's interaction event."""
        if self.action_owner != self.identities['owner_identity']:
            raise RuntimeError('Interaction requires this run\'s owned launcher')
        if key not in ('E', 'F'):
            raise ValueError('Unsupported reviewed interaction key')
        before = self.sample()
        if (before['errors'] or not controllable_state(before)):
            raise InputNotReady('Interaction requires current controllable state')
        if self.bindings(extra_keys=(key,)).get(key) != 'Game_Pickup':
            raise RuntimeError('Interaction key is not currently bound to Game_Pickup')
        from world_state import Query
        query = Query(self)
        pc = query.obj(self.sampler.controller)
        if query.pointer(pc, 'InteractEvent') != expected_event:
            raise InputNotReady('Current interaction event changed; no key sent')
        from interaction_flow import door_context
        door = door_context(query, pc, expected_event, include_geometry=True)
        if door and (not door['inside'] or door['door'] is None):
            raise InputNotReady('Door event requires the same current inside-door trigger')
        query.finish()
        identity = self.identities['game_identity']
        background = background_input.change(identity['pid'], 'status', expected_creation=identity['creation_time'])
        if background['state'] != 'original':
            raise RuntimeError('Physical background controller input must remain disabled')
        if any(self.user.GetAsyncKeyState(vk) & 0x8000 for vk in (0x10, 0x11, 0x12, 0x77)):
            raise InputNotReady('Modifier/stop key held; no scoped interaction sent')
        window = self.backend.game_window(identity['pid'])
        desktop = self.desktop()
        if desktop['occlusion'] or desktop['foreground'] == window:
            raise RuntimeError('Interaction window is foreground or occludes another application')
        vk, scan = {'E': (69, 0x12), 'F': (70, 0x21)}[key]
        lp = 1 | (scan << 16)
        self.verify()
        if not self.user.PostMessageW(window, 0x100, vk, lp):
            raise c.WinError(c.get_last_error())
        started = time.monotonic()
        duration = .45 if door else .06
        seen_move = None
        try:
            emit('interaction_key_down', key=key, hwnd=window, event=expected_event, seconds=duration, door=door)
            while time.monotonic() - started < duration:
                self.verify()
                if (self.user.GetAsyncKeyState(0x77) & 0x8000 or
                        int(self.user.GetForegroundWindow() or 0) == window):
                    raise InputInterrupted('Interaction cancelled during hold')
                if door and time.monotonic() - started >= .1:
                    current = self.sample()
                    if (current.get('errors') or not gameplay(current.get('menu') or {}) or
                            current.get('health', 0) <= 0 or current.get('cinematic') is not False or
                            current.get('pawn_identity') != before.get('pawn_identity') or
                            current.get('native', {}).get('world_generation') != before.get('native', {}).get('world_generation')):
                        raise InputInterrupted('Door hold control context changed')
                    if current.get('special_move_raw'):
                        seen_move = current['special_move_raw']
                        break
                time.sleep(.03)
        finally:
            if (self.backend.control.identity(identity['pid']) != identity or
                    self.backend.game_window(identity['pid']) != window):
                raise InteractionReleaseFailed('Interaction release target identity changed')
            if not self.user.PostMessageW(window, 0x101, vk, lp | 0xc0000000):
                raise InteractionReleaseFailed('Interaction release post failed: ' + str(c.get_last_error()))
            emit('interaction_key_up', key=key, hwnd=window)
        return dict(key=key, release_posted=True, event=expected_event, door=door, seconds=duration,
                    actual_hold_seconds=time.monotonic() - started, special_move_observed=seen_move,
                    before_sample=before if door else None)


MOUSE = {
    'LeftMouseButton': (0x201, 0x202, 1, 0),
    'RightMouseButton': (0x204, 0x205, 2, 0),
    'MiddleMouseButton': (0x207, 0x208, 0x10, 0),
    'ThumbMouseButton': (0x20b, 0x20c, 0x10020, 0x10000),
    'ThumbMouseButton1': (0x20b, 0x20c, 0x10020, 0x10000),
    'ThumbMouseButton2': (0x20b, 0x20c, 0x20040, 0x20000),
}


def capabilities(bindings):
    commands = dict(Game_MoveForward='move_forward', Game_MoveBackward='move_backward',
                    Game_TurnLeft='turn_left', Game_TurnRight='turn_right')
    result = {}
    for key, command in bindings.items():
        if key in MOUSE and command in commands:
            result[commands[command]] = key
    return result
