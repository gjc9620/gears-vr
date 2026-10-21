"""Autonomous, zero-image playtests with owned-session cleanup and JSON evidence."""
import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import traceback
import uuid

from core import Evidence, Explorer, NavigationStalled, atomic_json, controllable, gameplay, approach_target

ROOT = Path(__file__).resolve().parents[2]


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def inherit_observed_context(report, observer, source_path):
    source_path = Path(source_path).resolve()
    if not source_path.is_relative_to(ROOT / 'analysis'):
        raise ValueError('Observed source report must be retained under analysis/')
    raw = source_path.read_bytes()
    source = json.loads(raw)
    provenance = source.get('provenance') or {}
    if source.get('mode') != 'run' or any(provenance.get(k) != observer.identities[k]
                                         for k in ('game_identity', 'owner_identity')):
        raise ValueError('Observed source report belongs to another game/owner')
    artifacts = provenance.get('manifest') or {}
    if any(not artifacts.get(k) or artifacts.get(k) != observer.session['diagnostics_artifacts'].get(k)
           for k in ('game_sha256', 'native_sha256', 'xr_sha256')):
        raise ValueError('Observed source report binary versions differ')
    with (report.directory / 'source-run-at-attach.json').open('xb') as stream:
        stream.write(raw)
    report.assisted = source.get('assisted') is True
    report.provenance.update(observed_source_report=str(source_path), observed_source_sha256=digest(raw),
        observed_health_assistance=((provenance.get('health_assistance') or {}).get('enabled') is True
                                    or (provenance.get('fixture') or {}).get('source_health_assistance') is True))


class ProtectedFiles:
    def __init__(self, paths, directory, require_stopped):
        self.directory = Path(directory)
        self.directory.mkdir()
        self.require_stopped = require_stopped
        self.rows = []
        self.expected = {}
        require_stopped()
        for i, value in enumerate(paths):
            path = Path(value).resolve()
            raw = path.read_bytes()
            if len(raw) > 8 * 1024 * 1024 or path.read_bytes() != raw:
                raise RuntimeError('Protected file changed or exceeded bound: ' + str(path))
            backup = self.directory / str(i)
            with backup.open('xb') as stream:
                stream.write(raw)
            self.rows.append(dict(source=str(path), backup=str(backup), sha256=digest(raw)))
            self.expected[str(path)] = digest(raw)
        atomic_json(self.directory / 'manifest.json', dict(files=self.rows))

    def replace(self, path, raw):
        self.require_stopped()
        target = next((r for r in self.rows if Path(r['source']) == Path(path).resolve()), None)
        if target is None:
            raise RuntimeError('Target is outside protected file set')
        if digest(Path(path).read_bytes()) != self.expected[target['source']]:
            raise RuntimeError('Protected file changed outside this test before replacement')
        write_bytes(Path(path), raw)
        self.expected[target['source']] = digest(raw)

    def restore(self):
        self.require_stopped()
        receipts = []
        for i, row in enumerate(self.rows):
            source, backup = Path(row['source']), Path(row['backup'])
            raw = backup.read_bytes()
            if digest(raw) != row['sha256']:
                raise RuntimeError('Backup hash changed')
            current = source.read_bytes()
            after = self.directory / (str(i) + '.after')
            if after.exists() and after.read_bytes() != current:
                after = self.directory / (str(i) + '.recovery-' + uuid.uuid4().hex + '.after')
            if not after.exists():
                with after.open('xb') as stream:
                    stream.write(current)
            self.require_stopped()
            if source.read_bytes() != current:
                raise RuntimeError('Protected file changed during restoration')
            write_bytes(source, raw)
            receipts.append(dict(source=str(source), test_sha256=digest(current),
                                 restored_sha256=digest(source.read_bytes())))
        atomic_json(self.directory / 'restored.json', dict(files=receipts, restored=True))
        return receipts


def write_bytes(path, raw):
    temporary = path.with_name(path.name + '.gearsvr-playtest-tmp')
    with temporary.open('xb') as stream:
        stream.write(raw)
    try:
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    if path.read_bytes() != raw:
        raise RuntimeError('Protected write readback failed')


def behind_preflight():
    from live import launcher
    from background_launch import verify_test_behind_build
    import instance_policy
    quiet = (ROOT / 'tools/quiet-startup/build-instance4') if instance_policy.context() else ROOT / 'tools/quiet-startup/build-test-behind2'
    verify_test_behind_build(quiet)
    provenance = json.loads((ROOT / 'tools/openxr-probe/simulator-quiet/test-behind-atomic-provenance.json').read_text(encoding='utf8'))
    manifest = ROOT / provenance['candidate_manifest']
    dll = ROOT / provenance['compiled_dll']
    if (digest(manifest.read_bytes()) != provenance['candidate_manifest_sha256'] or
            digest(dll.read_bytes()) != provenance['compiled_dll_sha256']):
        raise RuntimeError('Behind-windows simulator manifest/DLL identity changed')
    runtime = json.loads(manifest.read_text(encoding='utf8'))
    selected = Path(runtime['runtime']['library_path'])
    if not selected.is_absolute():
        selected = manifest.parent / selected
    if selected.resolve() != dll.resolve():
        raise RuntimeError('Simulator manifest selects a different DLL')
    return quiet, manifest


def open_lifecycle_lock(backend, *, create=False):
    import ctypes
    from ctypes import wintypes
    kernel = backend.control.k
    from instance_policy import owner_lock_name
    name = owner_lock_name()
    if create:
        ctypes.set_last_error(0)
        handle = kernel.CreateMutexW(None, False, name)
        if not handle:
            raise ctypes.WinError(ctypes.get_last_error())
        if ctypes.get_last_error() == 183:
            kernel.CloseHandle(handle)
            return None
        return handle
    kernel.OpenMutexW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    kernel.OpenMutexW.restype = wintypes.HANDLE
    handle = kernel.OpenMutexW(0x100000, False, name)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    return handle


def wait_until(seconds, predicate, stop=lambda: False):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return True
        if stop():
            return False
        time.sleep(.1)
    return bool(predicate())


def observe(report, observer, seconds, *, explore=False, navigation=False, branch_side='left', target=None,
            world_state=False, interact=False, combat=False, interaction_goals=False, look_calibration=None,
            forward_input=False, health_assist=False, next_targets=(), hold_on_stall=False):
    from live import capabilities, InputNotReady, InputInterrupted, SessionChanged, MouseReleaseFailed
    from axis_input import AxisNeutralizationFailed, AxisReadInterrupted
    deadline = time.monotonic() + seconds
    explorer = None
    exploration_identity = None
    last_save = 0
    desktop_failures = 0
    navigation_context = None
    graph = None
    visited_goals = set()
    recoveries = 0
    next_world_sample = 0
    objective_context = None
    last_objectives = None
    interaction_attempts = set()
    next_navigation_sample = 0
    combat_attempts = {}
    target_facing_deadline = None
    target_arrival_radius = 50
    active_goal_event = None
    active_goal_kind = None
    active_touch_context = None
    exploration_destination = None
    checkpoint_capture_cursor = None
    streaming_capture_cursor = None
    unavailable_goal_events = set()
    from navigation import RouteProgress
    route_progress = RouteProgress()
    route_stalls = {}
    gameover_recoveries = 0
    forward_calibration = None
    queued_targets = [list(p) for p in next_targets]
    fixed_sequence = target is not None
    sequence_complete = False
    sequence_finished_at = None
    final_state_attempts = 0
    pending_door = None
    from control import RunControl
    control = RunControl(observer, report, persistent=hold_on_stall,
                         automatic=interaction_goals and not fixed_sequence) if explore else None

    def advance_target(reason, sample):
        nonlocal target, explorer, graph, next_navigation_sample, target_facing_deadline
        nonlocal active_goal_event, active_goal_kind, pending_door, sequence_complete, target_arrival_radius, sequence_finished_at
        nonlocal active_touch_context
        report.event('navigation_goal_completed', target=target, reason=reason, sample=sample['sample_id'])
        target = queued_targets.pop(0) if queued_targets else None
        sequence_complete = fixed_sequence and target is None
        if sequence_complete:
            sequence_finished_at = time.monotonic()
            report.provenance['explicit_target_sequence_completed'] = True
        pending_door = explorer = graph = active_goal_event = active_goal_kind = None
        active_touch_context = None
        next_navigation_sample = 0
        target_facing_deadline = None
        target_arrival_radius = 50
        report.event('navigation_target_advanced', target=target, remaining=len(queued_targets))
    while time.monotonic() < deadline and not (report.directory / 'stop').exists():
        if observer.user.GetAsyncKeyState(0x77) & 0x8000:
            report.event('cancelled', source='F8')
            return
        sample = report.observe(observer.sample())
        if time.monotonic() - last_save >= 2:
            report.save(phase='input_held' if control and control.held else 'exploring' if explore else 'observing')
            last_save = time.monotonic()
        if control:
            command = control.poll(sample, target_allowed=pending_door is None)
            if command and command['action'] == 'resume':
                route_progress.reset()
            if command and command['action'] in ('target', 'invalidate', 'auto', 'reposition'):
                target = command.get('target') if command['action'] == 'target' else None
                active_goal_kind = command.get('kind') if command['action'] == 'target' else None
                active_goal_event = pending_door = explorer = None
                active_touch_context = None
                exploration_destination = None
                checkpoint_capture_cursor = None
                streaming_capture_cursor = None
                queued_targets.clear()
                sequence_complete = False
                sequence_finished_at = target_facing_deadline = None
                target_arrival_radius = 35 if active_goal_kind == 'walk' else 50
                interaction_goals = command['action'] in ('auto', 'reposition')
                if command['action'] == 'target':
                    fixed_sequence = True
                elif command['action'] in ('auto', 'reposition'):
                    fixed_sequence = False
                if command['action'] == 'reposition':
                    visited_goals.clear()
                    unavailable_goal_events.clear()
                    navigation_context = None
                    next_world_sample = 0
                graph = None
                next_navigation_sample = 0
                report.provenance['explicit_target_sequence_completed'] = False
                report.event('navigation_target_override', target=target, target_kind=active_goal_kind,
                             request_sequence=command.get('sequence'), automatic_goals=interaction_goals)
            if control.held:
                if health_assist and not sample.get('errors') and controllable(sample) and sample['health'] < 1500:
                    from assistance import maintain_health
                    maintain_health(observer, report, sample)
                time.sleep(.2)
                continue
        if explore and not sample['errors'] and ((sample.get('menu') or {}).get('top') or {}).get('tag') == 'DeltaGameover':
            from gameover import reviewed_menu, reload_checkpoint
            if reviewed_menu(sample['menu']):
                if gameover_recoveries >= 3:
                    report.event('gameover_recovery_limit', attempts=gameover_recoveries,
                                 scope='Bounded policy limit; campaign progression remains unverified')
                    return
                gameover_recoveries += 1
                reload_checkpoint(observer, report)
                graph = explorer = navigation_context = None
                next_world_sample = next_navigation_sample = 0
                if interaction_goals:
                    target = active_goal_event = None
                continue
        if explore and sample.get('cinematic') is True:
            if graph is not None or explorer is not None:
                report.event('navigation_discarded', reason='Certified cinematic may stream or reposition the world')
            graph = explorer = navigation_context = None
            exploration_destination = None
            next_navigation_sample = 0
            if interaction_goals:
                target = active_goal_event = None
        if (explore and branch_side != 'none' and not sample['errors'] and
                ((sample.get('menu') or {}).get('top') or {}).get('tag') == 'DeltaChoosePath'):
            from path_choice import choose
            choose(observer, report, branch_side)
            graph = explorer = navigation_context = None
            exploration_destination = None
            next_navigation_sample = 0
            continue
        try:
            desktop = observer.desktop()
            report.event('desktop', elapsed=sample['elapsed'], **desktop)
            desktop_failures = 0
            if desktop['occlusion']:
                report.issue('window_occlusion', 'Owned test window covers another application',
                             severity='P1', evidence=desktop)
                raise RuntimeError('Owned-window occlusion observed')
        except RuntimeError as error:
            desktop_failures += 1
            report.event('desktop_unavailable', error=str(error))
            if desktop_failures >= 3 or 'occlusion observed' in str(error):
                raise
            # No input until a new complete desktop snapshot succeeds.
            time.sleep(.2)
            continue
        if explore and not sample['errors'] and controllable(sample):
            if pending_door:
                if (sample['pawn_identity'] != pending_door['before']['pawn_identity'] or
                        sample['native']['world_generation'] != pending_door['before']['native']['world_generation']):
                    report.event('door_crossing_context_changed', scope='New world/possession requires a new route certificate')
                    return
                from interaction_flow import crossing_measure
                crossing = crossing_measure(pending_door['geometry'], sample['pose']['position'])
                if crossing['passed']:
                    report.event('door_crossing_observed', event=pending_door['event'], geometry=pending_door['geometry'],
                                 sample=sample['sample_id'], measurement=crossing)
                    report.cover('door_traversal', 'passed', event=pending_door['event'], sample=sample['sample_id'],
                                 measurement=crossing, scope='Ordinary input moved the same pawn beyond the opened trigger cylinder')
                    if pending_door['completes_target']:
                        advance_target('Door event advanced and opened-trigger crossing observed', sample)
                    else:
                        target = pending_door['resume_target']
                        active_goal_kind = pending_door.get('resume_goal_kind')
                        pending_door = explorer = None
                    continue
            if active_goal_kind == 'touch' and active_goal_event:
                current_touch_context = (sample['pawn_identity'], sample['native']['world_generation'])
                if current_touch_context != active_touch_context:
                    report.event('touch_goal_context_changed', scope='Discarded old event after possession/world change')
                    target = active_goal_event = active_goal_kind = active_touch_context = None
                    graph = explorer = None
                    next_navigation_sample = 0
                    continue
                try:
                    from mission_flow import touch_count
                    count = touch_count(observer, active_goal_event)
                    if count > 0:
                        report.event('touch_goal_activated', event=active_goal_event, count=count,
                                     sample=sample['sample_id'], scope='Current event count advanced; triggering actor is not inferred')
                        advance_target('Authored touch event activation count advanced', sample)
                        continue
                except SessionChanged:
                    raise
                except (OSError, RuntimeError, ValueError) as error:
                    report.event('touch_goal_state_unavailable', reason=str(error))
                    continue
            if sequence_complete:
                if health_assist and sample['health'] < 1500:
                    from assistance import maintain_health
                    maintain_health(observer, report, sample)
                if time.monotonic() - sequence_finished_at < 10:
                    time.sleep(.2)
                    continue
                if world_state:
                    from world_state import capture as capture_world, objective_fingerprint
                    try:
                        final_world = capture_world(observer, include_streaming=True)
                        atomic_json(report.directory / 'final-world-state.json', final_world)
                        report.provenance['target_sequence_final_state'] = dict(
                            map_package=final_world.get('map_package'),
                            objectives=objective_fingerprint(final_world.get('objectives')),
                            world_identity=final_world['world_identity'])
                        report.event('target_sequence_final_state', **report.provenance['target_sequence_final_state'])
                    except (OSError, RuntimeError, ValueError) as error:
                        final_state_attempts += 1
                        report.event('target_sequence_outcome_unavailable', reason=str(error), attempt=final_state_attempts)
                        if final_state_attempts < 3:
                            time.sleep(.2)
                            continue
                if control and control.persistent:
                    control.hold(sample, 'Explicit target sequence completed', awaiting_target=True)
                    sequence_complete = False
                    target = None
                    continue
                return
            if health_assist and sample['health'] < 1500:
                from assistance import maintain_health
                try:
                    maintain_health(observer, report, sample)
                except InputNotReady as error:
                    report.event('test_health_assistance_deferred', reason=str(error))
                continue  # Always reacquire after a possible data write.
            if forward_input and (forward_calibration is None or
                    forward_calibration['pawn_identity'] != sample.get('pawn_identity') or
                    forward_calibration['world_generation'] != sample.get('native', {}).get('world_generation')):
                from movement_input import calibrate as calibrate_forward
                forward_calibration = calibrate_forward(observer, report)
                if forward_calibration is None:
                    raise RuntimeError('Forward input did not pass direction and neutral-settling calibration')
                report.provenance['forward_input_calibration'] = forward_calibration
                continue  # Navigation requires a new sample after calibration.
            if world_state and time.monotonic() >= next_world_sample:
                from world_state import capture as capture_world, objective_fingerprint, hostile
                next_world_sample = time.monotonic() + 2
                try:
                    world = capture_world(observer)
                    report.provenance.setdefault('initial_map_package', world.get('map_package'))
                    report.provenance['last_map_package'] = world.get('map_package')
                    sample = report.observe(world['end_sample'])
                    try:
                        world['keyboard_bindings'] = observer.bindings(extra_keys=('E', 'F', 'R', 'SpaceBar'))
                    except (OSError, RuntimeError, ValueError) as error:
                        world['keyboard_bindings_unavailable'] = str(error)
                    destination = report.directory / ('world-state-' + str(report.samples) + '.json')
                    atomic_json(destination, world)
                    current_objectives = objective_fingerprint(world.get('objectives'))
                    context = (sample.get('pawn_identity'), (sample.get('native') or {}).get('world_generation'))
                    if context != objective_context:
                        last_objectives = None
                        objective_context = context
                    if current_objectives:
                        if last_objectives is not None and last_objectives != current_objectives:
                            report.event('objective_set_changed', path=str(destination),
                                         before=last_objectives, after=current_objectives)
                        last_objectives = current_objectives
                        report.cover('objective_tracking', 'passed', path=str(destination),
                                     scope='Current objective schema/value observation; mission completion unproven')
                    report.event('world_state', path=str(destination), pawns=len(world['pawns']),
                                 hostiles=sum(hostile(p) for p in world['pawns']),
                                 interaction=bool(world.get('interaction', {}).get('event')),
                                 objectives=current_objectives, errors=world['errors'],
                                 acquisition_seconds=world['acquisition_seconds'])
                    if combat and not pending_door:
                        from combat import step
                        current_bindings = observer.bindings()
                        try:
                            if step(observer, report, world, explorer, capabilities(current_bindings),
                                    current_bindings, combat_attempts, look_calibration):
                                next_world_sample = 0
                                route_progress.reset()
                                continue
                        except (InputNotReady, InputInterrupted) as error:
                            report.event('combat_input_interrupted', reason=str(error))
                            continue
                    event = (world.get('interaction') or {}).get('event')
                    if interact and event and not pending_door and active_goal_kind != 'walk':
                        key = next((name for name in ('E', 'F')
                                    if world.get('keyboard_bindings', {}).get(name) == 'Game_Pickup'), None)
                        event_key = (context[1], event['address'], event['internal_index'], event['number'])
                        if key and event_key not in interaction_attempts:
                            before_count = world['interaction'].get('TriggerCount')
                            try:
                                receipt = observer.interaction_key(key, event, emit=report.event)
                            except InputNotReady as error:
                                interaction_attempts.discard(event_key)
                                report.event('interaction_not_dispatched', reason=str(error))
                                continue
                            except InputInterrupted as error:
                                interaction_attempts.discard(event_key)
                                report.event('interaction_interrupted', reason=str(error))
                                continue
                            interaction_attempts.add(event_key)
                            door_effect = None
                            if receipt.get('door'):
                                from interaction_flow import wait_effect
                                door_effect = wait_effect(observer, report, event, before_count, receipt)
                            time.sleep(.25)
                            from world_state import Query
                            q = Query(observer)
                            try:
                                current_event = q.obj(event['address'])
                                if current_event != event:
                                    raise RuntimeError('Interaction event identity changed after dispatch')
                                after_count = q.scalar(current_event, 'TriggerCount', 'IntProperty', '<i')
                                q.finish()
                                report.event('interaction_result', receipt=receipt,
                                             before_count=before_count, after_count=after_count, door_effect=door_effect)
                                if before_count is not None and after_count > before_count:
                                    report.cover('interact', 'passed', event=event, key=key,
                                                 before_count=before_count, after_count=after_count,
                                                 scope='Same interaction event activation counter advanced after scoped pickup key')
                                    if receipt.get('door') and receipt['door'].get('geometry'):
                                        pending_door = dict(event=event, geometry=receipt['door']['geometry'],
                                            before=receipt['before_sample'], resume_target=target,
                                            resume_goal_kind=active_goal_kind,
                                            completes_target=bool(target and active_goal_kind != 'touch' and
                                                                  math.dist(sample['pose']['position'], target) < 300))
                                        target = pending_door['geometry']['exit_goal']
                                        active_goal_kind = 'door_exit'
                                        target_arrival_radius = 35
                                        target_facing_deadline = None
                                        explorer = None
                                        report.event('door_crossing_started', target=target, event=event,
                                                     geometry=pending_door['geometry'])
                                    elif target and math.dist(sample['pose']['position'], target) < 300:
                                        report.event('target_interaction_observed', target=target, event=event)
                                        advance_target('Interaction event activation counter advanced', sample)
                                else:
                                    report.issue('interaction_effect_unverified',
                                                 'Interaction key dispatched without observed event-count increase',
                                                 classification='test_infrastructure', evidence=receipt)
                            except (OSError, RuntimeError, ValueError) as error:
                                report.event('interaction_outcome_unavailable', reason=str(error), receipt=receipt)
                            continue
                except (AxisNeutralizationFailed, MouseReleaseFailed):
                    raise
                except (OSError, RuntimeError, ValueError) as error:
                    report.event('world_state_rejected', reason=str(error))
                    # Nested sampling may have invalidated or replaced readers.
                    # The outer sample cannot authorize input after that change.
                    continue
            try:
                bindings = observer.bindings()
            except SessionChanged:
                raise
            except (OSError, RuntimeError, ValueError) as error:
                report.event('bindings_rejected', reason=str(error))
                observer.close()
                continue
            choices = capabilities(bindings)
            if forward_input:
                choices['move_forward'] = forward_calibration['axis']
            report.event('bindings', values=bindings, capabilities=choices)
            movement = next((v for v in ('move_forward', 'move_backward') if v in choices), None)
            turn = next((v for v in ('turn_left', 'turn_right') if v in choices), None)
            if not movement or not turn:
                report.cover('walk', 'blocked', reason='Verified scoped movement/turn bindings missing', bindings=bindings)
                report.issue('input_capability_missing', 'Current profile lacks the scoped movement/turn bindings',
                             evidence=bindings)
                return
            context = (sample.get('pawn_identity'), (sample.get('native') or {}).get('world_generation'))
            if navigation and context != navigation_context:
                next_navigation_sample = 0
            if navigation and not pending_door and ((graph is None and time.monotonic() >= next_navigation_sample) or
                    (graph and math.dist(sample['pose']['position'], graph['source_sample']['pose']['position']) > 1200)):
                explorer = None
            if explorer is None or context != exploration_identity:
                refresh_graph = (context != navigation_context or graph is None or
                    math.dist(sample['pose']['position'], graph['source_sample']['pose']['position']) > 1200)
                if navigation and not pending_door and refresh_graph and time.monotonic() >= next_navigation_sample:
                    from navigation import capture
                    if context != navigation_context:
                        visited_goals.clear()
                    navigation_context = context
                    next_navigation_sample = time.monotonic() + 10
                    graph = None
                    try:
                        graph = capture(observer)
                        destination = report.directory / ('navigation-' + str(report.samples) + '.json')
                        atomic_json(destination, graph)
                        report.event('navigation_graph', path=str(destination), nodes=len(graph['nodes']),
                                     edges=len(graph['edges']), truncated=graph['truncated'])
                        if interaction_goals and not (active_goal_kind == 'touch' and active_goal_event):
                            from mission_hints import capture as capture_hints, choose_goal
                            hints = capture_hints(observer)
                            hints_path = report.directory / ('mission-hints-' + str(report.samples) + '.json')
                            atomic_json(hints_path, hints)
                            goal = choose_goal(graph, hints, hints['source_sample']['pose']['position'],
                                               avoid=interaction_attempts | unavailable_goal_events)
                            try:
                                from mission_flow import capture_choice_goals, choose_choice_goal
                                flow = capture_choice_goals(observer)
                                flow_path = report.directory / ('mission-flow-' + str(report.samples) + '.json')
                                atomic_json(flow_path, flow)
                                choice_goal = choose_choice_goal(graph, flow, flow['end_sample']['pose']['position'],
                                                                 avoid=unavailable_goal_events)
                                if choice_goal:
                                    goal = choice_goal
                                report.event('mission_flow_goals', path=str(flow_path), rows=len(flow['rows']),
                                             selected=choice_goal)
                            except (OSError, RuntimeError, ValueError) as error:
                                report.event('mission_flow_unavailable', reason=str(error))
                            if goal is None or goal.get('goal_kind') != 'touch':
                                try:
                                    from mission_flow import capture_checkpoint_goals
                                    history = (report.provenance.get('fixture') or {}).get('completed_checkpoint_signals', [])
                                    checkpoint_flow = capture_checkpoint_goals(observer, resume_cursor=checkpoint_capture_cursor,
                                                                              completed_signals=history)
                                    checkpoint_capture_cursor = checkpoint_flow.get('next_cursor')
                                    checkpoint_path = report.directory / ('checkpoint-flow-' + str(report.samples) + '.json')
                                    atomic_json(checkpoint_path, checkpoint_flow)
                                    checkpoint_goal = choose_choice_goal(graph, checkpoint_flow,
                                        checkpoint_flow['end_sample']['pose']['position'], avoid=unavailable_goal_events)
                                    if checkpoint_goal and (goal is None or not checkpoint_goal.get('forced_hints_required')):
                                        goal = checkpoint_goal
                                    elif checkpoint_goal:
                                        checkpoint_capture_cursor = None
                                    report.event('checkpoint_flow_goals', path=str(checkpoint_path),
                                                 rows=len(checkpoint_flow['rows']), selected=checkpoint_goal,
                                                 errors=checkpoint_flow['errors'])
                                    if checkpoint_goal is None and checkpoint_capture_cursor:
                                        report.event('checkpoint_flow_waiting', scope='Resume bounded discovery before choosing a fallback door')
                                        graph = explorer = None
                                        next_navigation_sample = time.monotonic() + .5
                                        continue
                                except SessionChanged:
                                    raise
                                except (OSError, RuntimeError, ValueError) as error:
                                    report.event('checkpoint_flow_unavailable', reason=str(error))
                                    graph = explorer = None
                                    next_navigation_sample = time.monotonic() + 2
                                    continue
                            if goal is None or goal.get('goal_kind') != 'touch':
                                try:
                                    from mission_flow import capture_streaming_goals
                                    streaming = capture_streaming_goals(observer, resume_cursor=streaming_capture_cursor)
                                    streaming_capture_cursor = streaming.get('next_cursor')
                                    streaming_path = report.directory / ('streaming-flow-' + str(report.samples) + '.json')
                                    atomic_json(streaming_path, streaming)
                                    streaming_goal = (choose_choice_goal(graph, streaming,
                                        streaming['end_sample']['pose']['position'], avoid=unavailable_goal_events)
                                        if streaming['rows'] else None)
                                    if streaming_goal and (goal is None or not streaming_goal.get('forced_hints_required')):
                                        streaming_goal['source_kind'] = 'streaming'
                                        goal = streaming_goal
                                    elif streaming_goal:
                                        streaming_capture_cursor = None
                                    report.event('streaming_flow_goals', path=str(streaming_path), rows=len(streaming['rows']),
                                                 selected=streaming_goal, errors=streaming['errors'])
                                    if streaming_goal is None and streaming_capture_cursor:
                                        graph = explorer = None
                                        next_navigation_sample = time.monotonic() + .5
                                        continue
                                except SessionChanged:
                                    raise
                                except (OSError, RuntimeError, ValueError) as error:
                                    report.event('streaming_flow_unavailable', reason=str(error))
                                    graph = explorer = None
                                    next_navigation_sample = time.monotonic() + 2
                                    continue
                            target = goal['position'] if goal else None
                            if goal and goal.get('goal_kind') == 'touch' and goal.get('source_kind') != 'streaming':
                                exploration_destination = None
                            active_goal_event = goal['event'] if goal else None
                            active_goal_kind = goal.get('goal_kind', 'interaction') if goal else None
                            active_touch_context = context if active_goal_kind == 'touch' else None
                            target_arrival_radius = goal['arrival_radius'] if goal else 50
                            target_facing_deadline = None
                            report.event('interaction_goal_selected', goal=goal, path=str(hints_path))
                    except (OSError, RuntimeError, ValueError) as error:
                        if interaction_goals:
                            graph = None
                            target = None
                            active_goal_event = None
                        report.issue('navigation_discovery_unavailable', str(error),
                                     scope=type(error).__name__, evidence=report.samples)
                        report.event('navigation_discovery_rejected', error=str(error))
                if navigation and graph is None and not pending_door:
                    # A rejected snapshot cannot authorize a blind fallback
                    # walk. Retry from a fresh sample after streaming settles.
                    time.sleep(.25)
                    continue
                route = None
                if graph:
                    from navigation import choose_route
                    committed_node = None
                    if target is None and exploration_destination:
                        saved = exploration_destination
                        node = graph['nodes'].get(saved['key'])
                        if (saved['context'] == context and saved['world'] == graph['world_identity'] and
                                node == saved['node']):
                            committed_node = saved['key']
                        else:
                            report.event('exploration_destination_invalidated', goal=saved['key'],
                                         reason='Destination identity, position or context changed')
                            exploration_destination = None
                    route = choose_route(graph, sample['pose']['position'], avoid=visited_goals,
                                         target=target, goal_node=committed_node)
                    if committed_node and not route:
                        control.hold(sample, 'Committed exploration destination has no current walking route')
                        continue
                    if route:
                        if target is None and exploration_destination is None:
                            exploration_destination = dict(key=route['goal'], node=graph['nodes'][route['goal']],
                                                           context=context, world=graph['world_identity'])
                            report.event('exploration_destination_selected', goal=route['goal'],
                                         position=graph['nodes'][route['goal']]['position'])
                        report.event('navigation_route_selected', **route)
                yaw_calibration = (look_calibration or {}).get('yaw')
                precision = bool(forward_input and yaw_calibration)
                explorer = Explorer(movement, turn, sample['pose']['position'], sample['pose']['yaw_ru'],
                                    waypoints=route['waypoints'] if route else (),
                                    waypoint_radii=route.get('waypoint_arrival_radii') if route and precision else None,
                                    planar_waypoints=precision, heading_tolerance=4 if precision else 12)
                if forward_input and yaw_calibration:
                    explorer.turn_rate = (-1 if turn == 'turn_left' else 1) * abs(yaw_calibration['rate'] * yaw_calibration['value'])
                    explorer.walk_speed = max(30, min(600, forward_calibration['speed']))
                route_progress.reset()
                if not route:
                    explorer.heading += recoveries * 90
                exploration_identity = context
            if explorer.waypoints:
                if route_progress.stalled(explorer.waypoints, sample['pose']['position'], time.monotonic()):
                    key = (context[1], active_goal_event['address'] if active_goal_event else None)
                    route_stalls[key] = route_stalls.get(key, 0) + 1
                    report.issue('navigation_route_stalled', 'No meaningful approach to the next waypoint during the bounded route interval',
                                 classification='navigation_obstacle', scope=str(key),
                                 evidence=dict(sample=sample['sample_id'], position=sample['pose']['position'],
                                               next_waypoint=explorer.waypoints[0], attempt=route_stalls[key],
                                               best_distance=route_progress.best_distance,
                                               seconds_without_progress=time.monotonic() - route_progress.last_progress))
                    if hold_on_stall:
                        control.hold(sample, 'Navigation route stalled; input held for inspection')
                        continue
                    if route_stalls[key] >= 3 and active_goal_event:
                        unavailable_goal_events.add((context[1], active_goal_event['address'],
                            active_goal_event['internal_index'], active_goal_event['number']))
                    graph = explorer = None
                    next_navigation_sample = 0
                    continue
            approach = approach_target(sample, target, movement, turn, explorer.turn_rate,
                                       arrival_radius=target_arrival_radius,
                                       face_when_arrived=active_goal_kind not in ('touch', 'door_exit', 'walk'),
                                       max_distance=500 if pending_door else 220,
                                       minimum_walk_seconds=.08 if forward_input and target_arrival_radius >= 35 else .03,
                                       walk_speed=explorer.walk_speed) if target else None
            if approach:
                if active_goal_kind == 'walk' and approach['phase'] == 'face':
                    control.hold(sample, 'Requested walk target reached', awaiting_target=True)
                    target = None
                    explorer = None
                    report.event('control_walk_target_reached', sample=sample['sample_id'])
                    continue
                if approach['phase'] == 'face' and target_facing_deadline is None:
                    target_facing_deadline = time.monotonic() + 10
                    report.event('target_facing_started', target=target, sample=sample['sample_id'])
                if target_facing_deadline is not None and time.monotonic() >= target_facing_deadline:
                    report.event('target_prompt_not_admitted', target=target,
                                 scope='No verified event activation during bounded facing wait; continue exploration')
                    target = None
                    if interaction_goals:
                        if active_goal_event:
                            unavailable_goal_events.add((context[1], active_goal_event['address'],
                                active_goal_event['internal_index'], active_goal_event['number']))
                        graph = None
                        next_navigation_sample = 0
                    target_facing_deadline = None
                    explorer = None
                    continue
                action = approach['action']
                if action is None:
                    time.sleep(.15)
                    continue
            else:
                action = explorer.next(sample)
            if action:
                name, duration = action
                action_name = name
                native_turn = None
                if forward_input and name.startswith('turn_'):
                    from combat import axis_decision
                    desired = math.degrees(math.atan2(target[1] - sample['pose']['position'][1],
                        target[0] - sample['pose']['position'][0])) if approach else explorer.heading
                    actual = sample['pose']['yaw_ru'] * 360 / 65536
                    error = (desired - actual + 180) % 360 - 180
                    native_turn = axis_decision(look_calibration, 'yaw', error)
                try:
                    if forward_input and (name == 'move_forward' or native_turn):
                        from axis_input import pulse as axis_pulse
                        duration = native_turn['seconds'] if native_turn else min(.25, duration)
                        action_name = 'look_yaw' if native_turn else name
                        displacement_limit = 64
                        if not native_turn and (approach or not explorer.detour_moves):
                            point = target if approach else explorer.waypoints[0] if explorer.waypoints else None
                            radius = target_arrival_radius if approach else explorer.waypoint_radii[0] if explorer.waypoints else None
                            if point is not None:
                                gap = math.hypot(point[0] - sample['pose']['position'][0],
                                                 point[1] - sample['pose']['position'][1])
                                displacement_limit = max(1, min(64, gap - radius / 2))
                        pulse = axis_pulse(observer,
                            native_turn['axis'] if native_turn else forward_calibration['axis'],
                            native_turn['value'] if native_turn else forward_calibration['value'], duration,
                            target_degrees=native_turn['target_degrees'] if native_turn else None,
                            displacement_limit=displacement_limit,
                            stop_requested=lambda: (report.directory / 'stop').exists(), emit=report.event)
                    else:
                        pulse = observer.pulse(choices[name], duration, expected_bindings=bindings,
                                           stop_requested=lambda: (report.directory / 'stop').exists(),
                                           emit=report.event)
                except AxisReadInterrupted as error:
                    report.event('navigation_axis_interrupted', name=name, reason=str(error), receipt=error.receipt)
                    continue
                except InputNotReady as error:
                    report.event('input_not_dispatched', name=name, reason=str(error), sample=sample['sample_id'])
                    continue
                except InputInterrupted as error:
                    report.event('input_interrupted', name=name, reason=str(error), sample=sample['sample_id'],
                                 effect_verified=False, scope='Released; next action requires a fresh observation')
                    continue
                report.event('input_release', **pulse)
                time.sleep(.25)
                after = report.observe(observer.sample())
                time.sleep(.25)
                neutral = report.observe(observer.sample())
                result = report.action(action_name, sample, after, dispatched=pulse['release_posted'],
                                       neutral=neutral, detail=pulse)
                try:
                    if not native_turn:
                        explorer.feedback(result, duration)
                except NavigationStalled as error:
                    report.issue('navigation_control_stalled', str(error), classification='navigation_obstacle',
                                 scope=str((sample.get('native') or {}).get('world_generation')),
                                 evidence=dict(action_id=result['action_id'], before=sample['sample_id'],
                                               after=after['sample_id'], before_cover=sample.get('cover_state_raw'),
                                               after_cover=after.get('cover_state_raw')))
                    if control and control.persistent:
                        control.hold(neutral, 'Navigation control stalled; input held for inspection')
                        continue
                    if recoveries >= 2:
                        raise
                    recoveries += 1
                    from scenarios import checkpoint
                    report.event('navigation_recovery_started', attempt=recoveries)
                    checkpoint(observer, report)
                    report.event('navigation_recovery_completed', attempt=recoveries)
                    explorer = None
                    continue
                if result['result'] == 'observed':
                    report.cover('turn' if name.startswith('turn_') else 'walk', 'passed',
                                 action_id=result['action_id'], scope='bounded input response only')
            elif explorer.route_complete:
                report.event('navigation_route_reached', sample=sample['sample_id'])
                if target is None and exploration_destination:
                    visited_goals.add(exploration_destination['key'])
                    report.event('exploration_destination_reached', goal=exploration_destination['key'],
                                 position=sample['pose']['position'], sample=sample['sample_id'])
                    exploration_destination = None
                    graph = None
                    next_navigation_sample = 0
                explorer = None
        if time.monotonic() - last_save >= 2:
            report.save(phase='exploring' if explore else 'observing')
            last_save = time.monotonic()
        time.sleep(.25)


def owned_run(args, report):
    from live import Observer, launcher
    import graphics_policy
    quiet, runtime = behind_preflight()
    backend = launcher.Backend()
    report.provenance['runner_sources'] = {name: digest((Path(__file__).parent / name).read_bytes())
                                           for name in ('core.py', 'live.py', 'run.py', 'control.py', 'scenarios.py', 'navigation.py', 'path_choice.py', 'world_state.py', 'combat.py', 'mission_hints.py', 'mission_flow.py', 'interaction_flow.py', 'axis_input.py', 'movement_input.py', 'assistance.py', 'gameover.py')}
    sources = report.directory / 'runner-source'
    sources.mkdir()
    for name, expected in report.provenance['runner_sources'].items():
        raw = (Path(__file__).parent / name).read_bytes()
        if digest(raw) != expected:
            raise RuntimeError('Playtest source changed before source snapshot')
        (sources / name).write_bytes(raw)
    # Deferred scenario/navigation imports use this run's immutable source
    # snapshot even while the coordinator prepares the next revision.
    sys.path.insert(0, str(sources))
    report.provenance['test_options'] = dict(seconds=args.seconds, flows=args.flows,
        navigation=args.navigation, branch_side=args.branch_side, target=args.target)
    report.provenance['test_options']['world_state'] = args.world_state
    report.provenance['test_options']['interact'] = args.interact
    report.provenance['test_options']['combat'] = args.combat
    report.provenance['test_options']['interaction_goals'] = args.interaction_goals
    report.provenance['test_options']['forward_input_check'] = args.forward_input_check
    report.provenance['test_options']['health_assist'] = args.test_health_assist
    report.provenance['test_options']['next_targets'] = args.next_target
    report.provenance['test_options']['hold_on_stall'] = args.hold_on_stall
    report.provenance['test_options']['manual_preview_activation_allowed'] = (
        os.environ.get('GEARSVR_TEST_ALLOW_MANUAL_PREVIEW_ACTIVATION') == '1')
    if args.test_health_assist:
        report.provenance['health_assistance'] = dict(enabled=True, attempts=0, writes=0,
            floor=1500, target=9999, survival_acceptance=False)
    report.provenance['test_options']['pitch_check'] = args.pitch_check
    def stopped():
        if backend.game_pids():
            raise RuntimeError('Existing game prevents protected file changes')
    idle_since = None
    last_busy = None
    def idle():
        nonlocal idle_since, last_busy
        pids = backend.game_pids()
        active = bool(pids)
        active_owner = None
        if launcher.INDEX.exists():
            current = json.loads(launcher.INDEX.read_text(encoding='utf8'))
            identity = current.get('owner_identity')
            if identity and backend.control.identity(identity['pid']) == identity:
                active = True
                active_owner = identity
        busy = (tuple(pids), active_owner['pid'] if active_owner else None)
        if busy != last_busy:
            report.event('idle_wait', game_pids=pids, active_owner=active_owner)
            report.save(phase='waiting_for_idle')
            last_busy = busy
        if active:
            idle_since = None
            return False
        if idle_since is None:
            idle_since = time.monotonic()
        return time.monotonic() - idle_since >= 3
    if not wait_until(args.wait_for_idle, idle,
                      lambda: (report.directory / 'stop').exists()):
        raise RuntimeError('Game is still occupied; no session was adopted or interrupted')
    stopped()
    source_manifest = args.manifest.resolve()
    manifest = json.loads(source_manifest.read_text(encoding='utf8'))
    manifest['gpu_snapshot'] = False
    config = json.loads(args.config.read_text(encoding='utf8'))
    game, _ = launcher.settings(config, manifest)
    launcher.verify_files(manifest, game)
    fixture = json.loads(args.fixture.read_text(encoding='utf8')) if args.fixture else None
    if fixture and fixture.get('game_sha256') and fixture['game_sha256'] != manifest['game_sha256']:
        raise RuntimeError('Continuation fixture was produced by a different game version')
    if fixture:
        report.assisted = report.assisted or fixture.get('assisted', False)
    import instance_policy
    instance = instance_policy.context()
    if instance:report.provenance['instance_root'] = str(instance)
    marker = (instance / 'GameLogs/Launching.txt') if instance else game.parent.parent / 'WarGame/Logs/Launching.txt'
    if marker.exists():
        stopped()
        if not marker.is_file() or marker.read_bytes() != b'1':
            raise RuntimeError('Unknown failed-launch marker contents')
        archive = report.directory / 'preexisting-launching-marker.bin'
        with archive.open('xb') as stream:
            stream.write(marker.read_bytes())
        if marker.read_bytes() != b'1':
            raise RuntimeError('Failed-launch marker changed before archival')
        marker.unlink()
        report.event('failed_launch_marker_archived', source=str(marker), backup=str(archive))
    atomic_json(report.directory / 'manifest.json', manifest)
    atomic_json(report.directory / 'config.json', config)
    report.provenance.update(manifest=manifest, config=config, runtime=str(runtime))
    settings = graphics_policy.locate()
    save_dir = instance_policy.local_appdata() / 'Coalition/WarGame/SaveData'
    profiles = sorted(save_dir.glob('*.ue3profile*'))
    if not profiles:
        raise RuntimeError('No protected game profiles found')
    protected = ProtectedFiles([settings] + profiles, report.directory / 'backup', stopped)
    owner = observer = session_path = lifecycle_lock = None
    game_identity = None
    report.save(phase='preparing')
    try:
        text, encoding = graphics_policy.decode(settings.read_bytes())
        sections = list(re.finditer(r'(?ms)^\[ClientSettings\]\r?\n(.*?)(?=^\[|\Z)', text))
        if len(sections) != 1:
            raise RuntimeError('Expected one ClientSettings section')
        section = sections[0]
        body = section.group(1)
        for key, value in (('StartupResolutionX', '1280'), ('StartupResolutionY', '720'), ('StartupFullscreen', 'False')):
            body, count = re.subn(r'(?m)^' + key + r'=[^\r\n]*', key + '=' + value, body)
            if count != 1:
                raise RuntimeError('Expected one setting ' + key)
        protected.replace(settings, (text[:section.start(1)] + body + text[section.end(1):]).encode(encoding))
        if args.fixture:
            targets = {p.name: p for p in profiles}
            for item in fixture['files']:
                if Path(item['name']).name != item['name'] or item['name'] not in targets:
                    raise RuntimeError('Fixture target is outside protected profiles')
                raw = Path(item['path']).read_bytes()
                if digest(raw) != item['sha256']:
                    raise RuntimeError('Fixture hash mismatch')
                protected.replace(targets[item['name']], raw)
            report.provenance['fixture'] = fixture
        env = os.environ.copy()
        env.update(GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY='1', GEARSVR_TEST_MUTE_GAME='1',
                   GEARSVR_TEST_WINDOWS_BEHIND='1', GEARSVR_QUIET_STARTUP_BUILD=str(quiet))
        with (report.directory / 'launcher.log').open('xb') as stream:
            owner = subprocess.Popen([sys.executable, str(ROOT / 'tools/vr-launcher/launcher.py'),
                'start', '--config', str(report.directory / 'config.json'),
                '--manifest', str(report.directory / 'manifest.json'), '--auto-continue',
                '--runtime', str(runtime)], cwd=ROOT, env=env, stdout=stream,
                stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        owner_identity = backend.control.identity(owner.pid)
        if not owner_identity:
            raise RuntimeError('Launcher creation identity unavailable')
        report.provenance['owner_identity'] = owner_identity
        report.save(phase='starting_game')
        deadline = time.monotonic() + 180
        while time.monotonic() < deadline and owner.poll() is None:
            if (report.directory / 'stop').exists():
                raise RuntimeError('Test cancelled during startup')
            try:
                index = json.loads(launcher.INDEX.read_text(encoding='utf8'))
                if index.get('owner_identity') == owner_identity:
                    if lifecycle_lock is None:
                        # Keep the launcher's existing named object alive until
                        # all protected files are restored, after child exit.
                        lifecycle_lock = open_lifecycle_lock(backend)
                    game_identity = index.get('game_identity') or game_identity
                    report.provenance['game_identity'] = game_identity
                    if index.get('session_report'):
                        session_path = Path(index['session_report'])
                        session = json.loads(session_path.read_text(encoding='utf8'))
                        if session.get('phase') == 'running' and session.get('native_started'):
                            break
                report.save(phase='starting_game')
            except (OSError, ValueError):
                pass
            time.sleep(.2)
        else:
            raise RuntimeError('Fresh VR startup did not reach running state')
        report.provenance['session_path'] = str(session_path)
        observer = Observer(session_path, action_owner=owner_identity)
        report.cover('startup', 'passed', session=str(session_path), scope='running state; first publication evaluated separately')
        if args.flows:
            from scenarios import exercise
            exercise(observer, report)
        if args.test_health_assist:
            from assistance import maintain_health
            from scenarios import wait_state, playable
            maintain_health(observer, report, wait_state(observer, report, playable, seconds=180))
        if args.pitch_check:
            from axis_input import calibrate
            report.save(phase='calibrating_pitch')
            report.provenance['axis_calibration'] = calibrate(observer, report)
        observe(report, observer, args.seconds, explore=True, navigation=args.navigation,
                branch_side=args.branch_side, target=args.target, world_state=args.world_state,
                interact=args.interact, combat=args.combat, interaction_goals=args.interaction_goals,
                look_calibration=report.provenance.get('axis_calibration'), forward_input=args.forward_input_check,
                health_assist=args.test_health_assist, next_targets=args.next_target, hold_on_stall=args.hold_on_stall)
    except Exception as error:
        if any(issue['title'] == str(error) or issue['title'].endswith(': ' + str(error))
               for issue in report.issues.values()):
            report.event('scenario_error', message=str(error), traceback=traceback.format_exc())
        else:
            report.issue('scenario_incomplete', str(error), scope=type(error).__name__ + ':' + str(error),
                         evidence=dict(type=type(error).__name__, traceback=traceback.format_exc()))
        raise
    finally:
        report.save(phase='cleanup')
        if observer:
            observer.close()
        if owner and owner.poll() is None:
            identity = report.provenance.get('owner_identity')
            if identity and backend.control.identity(owner.pid) == identity:
                signalled = backend.control.signal(launcher.cancel_name(owner.pid))
                report.event('stop_requested', owner_identity=identity, accepted=signalled)
            wait_until(110, lambda: owner.poll() is not None)
        if owner and owner.poll() is None:
            report.issue('cleanup_incomplete', 'Owned launcher did not finish cleanup', severity='P1')
            raise RuntimeError('Launcher remains live; protected files retained for later recovery')
        final = json.loads(session_path.read_text(encoding='utf8')) if session_path and session_path.exists() else {}
        report.provenance['lifecycle'] = dict(owner_exit_code=owner.poll() if owner else None,
                    native_cleanup_verified=final.get('native_cleanup_verified'),
                    entry_restore_verified=final.get('entry_restore_verified'),
                    xr_exit_code=final.get('xr_exit_code'))
        if launcher.INDEX.exists():
            try:
                owner_state = json.loads(launcher.INDEX.read_text(encoding='utf8'))
                if owner_state.get('owner_identity') == report.provenance.get('owner_identity'):
                    report.provenance['lifecycle']['test_audio_guard'] = owner_state.get('test_audio_guard')
            except (OSError, ValueError) as error:
                report.event('optional_audio_receipt_unavailable', error=str(error))
        report.event('native_cleanup', session_report=str(session_path),
                     native_cleanup_verified=final.get('native_cleanup_verified'),
                     entry_restore_verified=final.get('entry_restore_verified'))
        if game_identity and backend.control.identity(game_identity['pid']) == game_identity:
            if not final.get('native_cleanup_verified') or not final.get('entry_restore_verified'):
                raise RuntimeError('Native cleanup unverified; owned game and backups retained')
            for hwnd in backend.game_windows(game_identity['pid']):
                backend.post_normal_close(hwnd, game_identity)
            if not wait_until(25, lambda: backend.control.identity(game_identity['pid']) != game_identity):
                raise RuntimeError('Normal owned-game close timed out; backup restoration deferred')
        if not wait_until(10, lambda: not backend.game_pids()):
            raise RuntimeError('Game process enumeration did not clear; file restoration deferred')
        stopped()
        report.provenance['lifecycle']['game_exit_confirmed'] = True
        receipts = protected.restore()
        report.provenance['lifecycle']['protected_files_restored'] = True
        report.event('protected_files_restored', receipts=receipts)
        if lifecycle_lock:
            backend.control.k.CloseHandle(lifecycle_lock)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('run', 'observe'))
    parser.add_argument('--seconds', type=float, default=90)
    parser.add_argument('--session', type=Path)
    parser.add_argument('--source-run', type=Path, help='Same owned run report for passive assistance/version provenance')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--wait-for-idle', type=float, default=300)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'tools/vr-launcher/manifest.json')
    parser.add_argument('--config', type=Path, default=ROOT / 'tools/vr-launcher/config.json')
    parser.add_argument('--fixture', type=Path, help='Explicit hashed test-profile manifest; originals restored after exit')
    parser.add_argument('--flows', action='store_true', help='Exercise aim/fire, pause/resume and checkpoint reload before exploration')
    parser.add_argument('--navigation', action='store_true', help='Collect current navigation hints and select ordinary walking routes')
    parser.add_argument('--branch-side', choices=('left', 'right', 'none'), default='left')
    parser.add_argument('--target', type=float, nargs=3, help='World-coordinate navigation hint; ordinary walking only')
    parser.add_argument('--next-target', type=float, nargs=3, action='append', default=[],
                        help='Next ordered target, admitted only after the prior interaction and door crossing')
    parser.add_argument('--hold-on-stall', action='store_true',
                        help='Hold owned input on a route stall for live inspection and validated target requests')
    parser.add_argument('--world-state', action='store_true', help='Observe current objectives, interaction events and PawnList')
    parser.add_argument('--interact', action='store_true', help='Try one verified pickup-key edge per current interaction event')
    parser.add_argument('--combat', action='store_true', help='Bounded scoped targeting of live AI currently targeting the player')
    parser.add_argument('--interaction-goals', action='store_true', help='Choose enabled interaction hints in the current directed walking component')
    parser.add_argument('--look-input-check', '--pitch-check', dest='pitch_check', action='store_true',
                        help='Calibrate neutralized reflected pitch/yaw input in this owned test game')
    parser.add_argument('--forward-input-check', action='store_true',
                        help='Require verified forward-axis travel and use bounded native input for walking')
    parser.add_argument('--test-health-assist', action='store_true',
                        help='Explicit assisted flow test: raise live owned pawn health to9999 below1500; survival is untested')
    args = parser.parse_args()
    if args.interaction_goals:
        args.world_state = args.interact = args.navigation = True
        if args.target:
            parser.error('Choose either --target or --interaction-goals')
    if args.interact or args.combat:
        args.world_state = True
    if not 1 <= args.seconds <= 21600 or not 0 <= args.wait_for_idle <= 3600:
        parser.error('seconds or idle-wait bound exceeded')
    if args.target and not all(math.isfinite(value) for value in args.target):
        parser.error('Navigation target must contain finite coordinates')
    if args.next_target and (not args.target or args.command != 'run' or
                            not all(math.isfinite(v) for goal in args.next_target for v in goal)):
        parser.error('Ordered targets require an owned run, initial target and finite coordinates')
    if args.command == 'observe' and (not args.session or args.fixture):
        parser.error('observe requires --session and does not apply a fixture')
    if args.source_run and args.command != 'observe':
        parser.error('--source-run is only valid for passive observe')
    if args.command == 'observe' and (args.interact or args.combat or args.interaction_goals or args.pitch_check or args.forward_input_check or args.test_health_assist or args.hold_on_stall):
        parser.error('game input requires a test-owned run')
    folder = args.output or ROOT / 'analysis/playtest' / (datetime.datetime.now().strftime('%Y%m%d-%H%M%S-') + uuid.uuid4().hex[:8])
    report = Evidence(folder.resolve(), mode=args.command, provenance={})
    code = 0
    try:
        if args.command == 'observe':
            from live import Observer, SessionChanged
            observer = Observer(args.session)
            report.provenance.update(session_path=str(observer.path), identities=observer.identities,
                                     artifacts=observer.session.get('diagnostics_artifacts'))
            try:
                if args.source_run:
                    inherit_observed_context(report, observer, args.source_run)
                observe(report, observer, args.seconds)
            except SessionChanged:
                if not observer.closed_normally():
                    raise
                report.event('observed_session_ended', session=str(observer.path),
                             scope='Source capture closed cleanly; no campaign-completion claim')
            finally:
                observer.close()
        else:
            owned_run(args, report)
    except Exception as error:
        code = 1
        report.issue('run_incomplete', str(error), severity='P1',
                     scope=type(error).__name__ + ':' + str(error),
                     evidence=dict(type=type(error).__name__, traceback=traceback.format_exc()))
    report.save(phase='finished' if code == 0 else 'incomplete')
    print(json.dumps(dict(report=str(report.directory / 'report.json'), exit_code=code,
                         samples=report.samples, actions=report.actions, issues=len(report.issues)), ensure_ascii=False), flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
