"""State-based playtest evidence and navigation; no OS or image dependencies."""
from collections import Counter
import hashlib
import json
import math
from pathlib import Path


COVERAGE = ('startup', 'turn', 'walk', 'vertical_aim', 'horizontal_aim', 'aim_fire', 'enemy_engagement', 'interact', 'door_traversal', 'objective_tracking', 'pause_resume', 'cinematic_transition', 'branch_choice', 'gameover_reload',
            'death_reload', 'checkpoint_transition', 'chapter_completion')


def atomic_json(path, value):
    path = Path(path)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf8')
    temporary.replace(path)


def yaw_delta(before, after):
    return (after - before + 32768) % 65536 - 32768


def gameplay(menu):
    top = menu.get('top') or {}
    return (menu.get('consistent_reread') is True and menu.get('read_only') is True
            and menu.get('input_block_count') == 0
            and top.get('tag') == 'PenderCrossplayInvitePopUp'
            and top.get('input_mode') == 2)


def controllable(sample):
    return (gameplay(sample.get('menu') or {}) and sample.get('cinematic') is False
            and sample.get('special_move_raw', 0) == 0
            and sample.get('health', 0) > 0 and bool(sample.get('pose')))


def approach_target(sample, target, movement, turn, turn_rate, *, arrival_radius=50, face_when_arrived=True,
                    max_distance=220, minimum_walk_seconds=.03, walk_speed=220):
    """Close a short walking gap, then face the trigger while waiting for admission."""
    if not controllable(sample) or turn_rate is None or abs(turn_rate) < 1:
        return None
    pose = sample['pose']
    position = pose['position']
    distance = math.hypot(target[0] - position[0], target[1] - position[1])
    if distance > max_distance or abs(target[2] - position[2]) > 180:
        return None
    walking = distance > arrival_radius
    if not walking and not face_when_arrived:
        return dict(phase='face', action=None, planar_distance=distance)
    desired = math.degrees(math.atan2(target[1] - position[1], target[0] - position[0]))
    actual = pose['yaw_ru'] * 360 / 65536 + (180 if walking and movement == 'move_backward' else 0)
    error = (desired - actual + 180) % 360 - 180
    if abs(error) > 10:
        angle = error % 360 if turn_rate > 0 else (-error) % 360
        action = (turn, max(.03, min(.20, angle / abs(turn_rate))))
    elif walking:
        action = (movement, max(minimum_walk_seconds, min(.18, (distance - arrival_radius) / walk_speed)))
    else:
        action = None
    return dict(phase='approach' if walking else 'face', action=action, planar_distance=distance)


class Evidence:
    def __init__(self, directory, *, mode, provenance):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=False)
        self.mode = mode
        self.provenance = provenance
        self.issues = {}
        self.coverage = {name: dict(status='not_run') for name in COVERAGE}
        self.samples = self.actions = 0
        self.last = None
        self.stall_since = None
        self.stall_reported = False
        self.closed = False
        self.counts = Counter()
        self.phase = 'starting'
        self.assisted = False
        self.cinematic_start = None
        self.cinematic_end = None
        self.last_controlled_sample = None
        self.gameover_observed = False
        self.mod_counters = {}

    def observe_mod_counters(self, sample):
        native = sample.get('native') or {}
        started = native.get('started_qpc')
        if (not sample.get('game_identity') or type(started) is not int or started <= 0
                or any(e.get('component') == 'native_status' for e in sample.get('errors', []))):
            return
        context = (json.dumps(sample['game_identity'], sort_keys=True), started)
        faults = ('owned_ui_faults', 'owned_material_faults')
        diagnostics = ('owned_ui_semantic_incomplete', 'mirror_inventory_rejected',
                       'anchor_submission_rejected', 'telemetry_present_failures')
        for key in faults + diagnostics:
            value = native.get(key)
            if type(value) is not int or value < 0:
                continue  # Missing is unavailable, never a synthetic zero.
            old_context, old_value = self.mod_counters.get(key, (None, None))
            previous = old_value if old_context == context else None
            self.mod_counters[key] = (context, value)
            if previous is not None and value < previous:
                self.event('mod_counter_reset', counter=key, previous=previous, current=value,
                           sample_id=self.samples, cause_verified=False)
                continue
            if value == 0 or previous is not None and value == previous:
                continue
            evidence = dict(counter=key, total=value, previous=previous,
                            new_count=value-previous if previous is not None else None,
                            initial_baseline=previous is None, sample_id=self.samples,
                            started_qpc=started, world_generation=native.get('world_generation'),
                            first_ui_fault=native.get('owned_ui_first_fault_name'),
                            first_material_fault=native.get('owned_material_first_fault'),
                            causal_attribution_verified=False, pixels_verified=False)
            if key in faults:
                self.issue('mod_native_fault', 'Native Mod reported ' + key,
                           scope=key, classification='mod_candidate', severity='P1', evidence=evidence)
            else:
                self.event('mod_diagnostic_counter', **evidence)

    def event(self, kind, **value):
        row = dict(kind=kind, **value)
        with (self.directory / 'events.jsonl').open('a', encoding='utf8') as stream:
            stream.write(json.dumps(row, ensure_ascii=False, allow_nan=False) + '\n')
        self.counts[kind] += 1

    def issue(self, category, title, *, scope='', classification='test_infrastructure',
              severity='P2', evidence=None, confirmed=False):
        key = hashlib.sha256((category + '\0' + scope).encode()).hexdigest()[:16]
        if key not in self.issues:
            self.issues[key] = dict(id=key, category=category, title=title, scope=scope,
                                   classification=classification, severity=severity,
                                   confirmed=confirmed, occurrences=0, evidence=[])
        row = self.issues[key]
        row['occurrences'] += 1
        if evidence is not None:
            # Bounded index; complete observations remain in JSONL.
            row['evidence'] = (row['evidence'] + [evidence])[-12:]
        self.event('issue', issue_id=key, category=category, evidence=evidence)
        return key

    def cover(self, name, status, **evidence):
        if name not in self.coverage or status not in ('passed', 'failed', 'blocked', 'not_run'):
            raise ValueError('Unknown coverage requirement or result')
        self.coverage[name] = dict(status=status, **evidence)
        self.event('coverage', requirement=name, **self.coverage[name])

    def observe(self, sample):
        self.samples += 1
        sample = dict(sample, sample_id=self.samples)
        self.event('sample', **sample)
        self.observe_mod_counters(sample)
        for error in sample.get('errors', []):
            transient = error['type'] in ('AnchorRegistryChanged', 'RegistrySnapshotChanged',
                                         'SnapshotChanged', 'InventoryChanged')
            self.issue('observation_unavailable', error['component'] + ': ' + error['message'],
                       scope=error['component'] + ':' + error['type'], evidence=self.samples,
                       classification='observation_transient' if transient else 'test_infrastructure',
                       severity='P3' if transient else 'P2')
        previous = self.last
        menu = sample.get('menu') or {}
        pose = sample.get('pose') or {}
        is_gameover = ((menu.get('top') or {}).get('tag') == 'DeltaGameover'
                       and (menu.get('top') or {}).get('cls') == 'WarUIScene'
                       and menu.get('read_only') is True and menu.get('consistent_reread') is True)
        if is_gameover and not self.gameover_observed:
            self.event('gameover_observed', sample_id=self.samples,
                       last_controlled_sample=(self.last_controlled_sample or {}).get('sample_id'),
                       cause_verified=False)
        self.gameover_observed = is_gameover
        if sample.get('pawn_identity') and sample.get('health') is not None and not sample.get('errors'):
            self.last_controlled_sample = sample
        if previous and previous.get('game_identity') != sample.get('game_identity'):
            self.issue('identity_changed', 'Game process identity changed', severity='P1',
                       evidence=self.samples)
            self.last = sample
            self.stall_since = None
            self.cinematic_start = self.cinematic_end = None
            return sample
        health = sample.get('health')
        last_health = previous.get('health') if previous else None
        if health is not None and health <= 0 and last_health is not None and last_health > 0:
            self.event('death', sample_id=self.samples, health=health)
        native = sample.get('native') or {}
        old_native = (previous or {}).get('native') or {}
        old_pose = (previous or {}).get('pose') or {}
        world = native.get('world_generation')
        old_move, move = (previous or {}).get('special_move_raw'), sample.get('special_move_raw')
        if (previous and old_move is not None and move is not None and old_move != move
                and previous.get('pawn_identity') == sample.get('pawn_identity')
                and old_native.get('world_generation') == world):
            self.event('special_move_transition', before=old_move, after=move, sample_id=self.samples,
                       scope='Reflected special-move byte; authored movement is not ordinary input drift')
        if sample.get('cinematic') is True:
            if self.cinematic_start is None:
                self.cinematic_start = self.samples
                self.event('cinematic_started', sample_id=self.samples)
            self.cinematic_end = None
        elif (self.cinematic_start and self.cinematic_end is None and sample.get('cinematic') is False):
            self.cinematic_end = (self.samples, native.get('published'), world)
            self.event('cinematic_ended', sample_id=self.samples)
        if self.cinematic_end and controllable(sample):
            end_sample, end_published, end_world = self.cinematic_end
            if (end_published is not None and end_world is not None and end_world == world
                    and native.get('published', -1) > end_published):
                self.cover('cinematic_transition', 'passed', start_sample=self.cinematic_start,
                           end_sample=end_sample, resumed_sample=self.samples,
                           scope='Certified cinematic flag transition and subsequent stereo publication')
                self.cinematic_start = self.cinematic_end = None
        if (previous and world is not None and old_native.get('world_generation') is not None
                and world != old_native['world_generation']):
            self.event('world_transition', before=old_native['world_generation'], after=world,
                       sample_id=self.samples)
        # Missing counters, paused UI, changed worlds and stale camera clocks
        # break continuity. They must never become zero-FPS samples.
        continuous = (previous and gameplay(menu) and gameplay(previous.get('menu') or {})
                      and world is not None and world == old_native.get('world_generation')
                      and pose.get('camera_timestamp') is not None
                      and old_pose.get('camera_timestamp') is not None
                      and pose['camera_timestamp'] > old_pose['camera_timestamp']
                      and native.get('published') is not None
                      and old_native.get('published') is not None)
        if continuous and native['published'] == old_native['published']:
            if self.stall_since is None:
                self.stall_since = previous['elapsed']
            if sample['elapsed'] - self.stall_since >= 3 and not self.stall_reported:
                self.issue('stereo_publication_stalled',
                           'Camera advanced while stereo publication did not advance for 3 seconds',
                           scope=str(world), classification='product_candidate', severity='P1',
                           evidence=dict(first_elapsed=self.stall_since, sample_id=self.samples))
                self.stall_reported = True
        else:
            self.stall_since = None
            self.stall_reported = False
        self.last = sample
        return sample

    def action(self, name, before, after, *, dispatched, neutral=None, detail=None):
        self.actions += 1
        row = dict(action_id=self.actions, name=name, dispatched=dispatched,
                   before=before.get('sample_id'), after=after.get('sample_id'), detail=detail)
        same = (before.get('game_identity') == after.get('game_identity')
                and controllable(before) and controllable(after)
                and not before.get('errors') and not after.get('errors')
                and before.get('health', 0) > 0 and after.get('health', 0) > 0
                and before.get('cinematic') is False and after.get('cinematic') is False
                and before.get('pawn_identity') == after.get('pawn_identity')
                and before.get('pawn_identity') is not None
                and (before.get('native') or {}).get('world_generation') is not None
                and (before.get('native') or {}).get('world_generation') ==
                    (after.get('native') or {}).get('world_generation')
                and gameplay(before.get('menu') or {}) and gameplay(after.get('menu') or {}))
        if not dispatched or not same:
            row.update(result='unverified', reason='dispatch/state/possession boundary')
        else:
            a, b = before.get('pose') or {}, after.get('pose') or {}
            if name.startswith('move_') and a.get('position') and b.get('position'):
                row['distance'] = math.dist(a['position'], b['position'])
                seconds = (detail or {}).get('seconds')
                threshold = min(15, max(1, seconds * 50)) if isinstance(seconds, (int, float)) and math.isfinite(seconds) and .02 <= seconds <= 1 else 15
                row['movement_threshold'] = threshold
                row['result'] = 'observed' if row['distance'] >= threshold else 'no_effect'
                if row['result'] == 'no_effect':
                    self.issue('navigation_blocked', 'Movement request did not meet its bounded response threshold',
                               classification='navigation_obstacle', evidence=self.actions)
            elif name.startswith('turn_') and a.get('yaw_ru') is not None and b.get('yaw_ru') is not None:
                row['yaw_degrees'] = yaw_delta(a['yaw_ru'], b['yaw_ru']) * 360 / 65536
                row['result'] = 'observed' if abs(row['yaw_degrees']) >= 3 else 'no_effect'
            elif name in ('look_pitch', 'look_yaw') and a.get('controller_rotation_ru') and b.get('controller_rotation_ru'):
                label, component = ('pitch', 0) if name == 'look_pitch' else ('yaw', 1)
                row[label + '_degrees'] = yaw_delta(a['controller_rotation_ru'][component], b['controller_rotation_ru'][component]) * 360 / 65536
                if a.get('camera_rotation_ru') and b.get('camera_rotation_ru'):
                    row['camera_' + label + '_degrees'] = yaw_delta(a['camera_rotation_ru'][component], b['camera_rotation_ru'][component]) * 360 / 65536
                row['result'] = 'observed' if abs(row[label + '_degrees']) >= .05 and abs(row.get('camera_' + label + '_degrees', 0)) >= .02 else 'unverified'
                if (neutral and not neutral.get('errors') and controllable(neutral) and
                        neutral.get('pawn_identity') == after.get('pawn_identity') and
                        neutral.get('native', {}).get('world_generation') == after['native']['world_generation'] and
                        neutral.get('pose', {}).get('controller_rotation_ru')):
                    row['neutral_' + label + '_drift'] = yaw_delta(b['controller_rotation_ru'][component], neutral['pose']['controller_rotation_ru'][component]) * 360 / 65536
            elif name == 'fire' and before.get('ammo_used') is not None and after.get('ammo_used') is not None:
                row['ammo_used_delta'] = after['ammo_used'] - before['ammo_used']
                row['result'] = 'observed' if row['ammo_used_delta'] > 0 else 'unverified'
            else:
                row['result'] = 'unverified'
            if neutral and controllable(neutral) and neutral.get('pawn_identity') == after.get('pawn_identity'):
                p = (neutral.get('pose') or {}).get('position')
                if p and b.get('position'):
                    row['neutral_drift'] = math.dist(b['position'], p)
                    # Drift is a candidate: slopes, animation and enemy forces
                    # must be distinguished from a held input in a later test.
                    if row['neutral_drift'] > 10:
                        self.issue('post_release_drift', 'Pawn moved after the recorded release',
                                   classification='product_candidate', evidence=self.actions)
        self.event('action', **row)
        return row

    def save(self, *, phase=None, **extra):
        if phase is not None:
            self.phase = phase
        report = dict(schema=1, phase=self.phase, mode=self.mode, assisted=self.assisted,
                      provenance=self.provenance, samples=self.samples, actions=self.actions,
                      event_counts=dict(self.counts), issues=list(self.issues.values()),
                      coverage=self.coverage, image_capture=False,
                      automatic_full_campaign_verified=False,
                      headset_verified=False, **extra)
        atomic_json(self.directory / 'report.json', report)
        return report


class NavigationStalled(RuntimeError):
    """Valid observations show that the current movement policy cannot progress."""


class Explorer:
    """Short state-checked movement with collision detours and coverage limits."""
    def __init__(self, direction, turn, origin, yaw_ru, *, waypoints=(), waypoint_radii=None,
                 planar_waypoints=False, heading_tolerance=12):
        if direction not in ('move_forward', 'move_backward') or turn not in ('turn_left', 'turn_right'):
            raise ValueError('Verified movement and turn capabilities required')
        self.direction, self.turn = direction, turn
        self.origin = list(origin)
        self.heading = yaw_ru * 360 / 65536
        self.turn_rate = None
        self.blocked = 0
        self.distance = 0
        self.waypoints = list(waypoints)
        self.waypoint_radii = list(waypoint_radii) if waypoint_radii is not None else [80] * len(self.waypoints)
        if (len(self.waypoint_radii) != len(self.waypoints) or
                any(type(r) not in (int, float) or not math.isfinite(r) or not 0 < r <= 200
                    for r in self.waypoint_radii)):
            raise ValueError('Waypoint arrival radii must match the route')
        self.planar_waypoints = planar_waypoints
        self.heading_tolerance = heading_tolerance
        self.visited = set()
        self.detour_moves = 0
        self.route_complete = False
        self.walk_speed = 220.0

    def next(self, sample):
        if not controllable(sample):
            return None
        pose = sample.get('pose') or {}
        if not pose.get('position') or pose.get('yaw_ru') is None:
            return None
        if self.turn_rate is None:
            return self.turn, .15
        xyz = pose['position']
        if self.waypoints and not self.detour_moves:
            goal = self.waypoints[0]
            def distance(point):
                return math.hypot(point[0] - xyz[0], point[1] - xyz[1]) if self.planar_waypoints else math.dist(xyz, point)
            while distance(goal) <= self.waypoint_radii[0] and abs(goal[2] - xyz[2]) <= 180:
                self.waypoints.pop(0)
                self.waypoint_radii.pop(0)
                if not self.waypoints:
                    self.route_complete = True
                    return None
                goal = self.waypoints[0]
            self.heading = math.degrees(math.atan2(goal[1] - xyz[1], goal[0] - xyz[0]))
        actual = pose['yaw_ru'] * 360 / 65536 + (180 if self.direction == 'move_backward' else 0)
        error = (self.heading - actual + 180) % 360 - 180
        if abs(error) > self.heading_tolerance:
            signed = self.turn_rate
            rotation = error % 360 if signed > 0 else (-error) % 360
            return self.turn, max(.03, min(.25, rotation / abs(signed)))
        if self.waypoints and not self.detour_moves:
            # Stop before the next corner, with headroom for release latency.
            remaining = math.hypot(self.waypoints[0][0] - xyz[0], self.waypoints[0][1] - xyz[1]) if self.planar_waypoints else math.dist(xyz, self.waypoints[0])
            margin = self.waypoint_radii[0] / 2 if self.planar_waypoints else 45
            return self.direction, max(.08, min(.8, (remaining - margin) / self.walk_speed))
        return self.direction, .45

    def feedback(self, result, seconds):
        if result.get('result') == 'unverified':
            # Registry/scene/possession changes invalidate this measurement;
            # they do not establish an input failure or a new calibration.
            return
        if result['name'].startswith('turn_'):
            delta = result.get('yaw_degrees')
            if self.turn_rate is not None and seconds < .1 and result.get('result') == 'no_effect':
                return
            if result.get('result') != 'observed' or not delta:
                if self.turn_rate is not None:
                    raise NavigationStalled('Previously calibrated turn produced no verified yaw response')
                raise NavigationStalled('Initial turning response could not be calibrated')
            self.turn_rate = delta / seconds
        elif result['name'].startswith('move_'):
            self.distance += result.get('distance', 0)
            if result.get('result') == 'observed':
                self.blocked = 0
            if result.get('result') == 'observed' and result.get('distance', 0) >= 15:
                speed = result['distance'] / seconds
                if 30 <= speed <= 600:
                    self.walk_speed = speed
            if result.get('result') == 'no_effect':
                self.blocked += 1
                self.heading = (self.heading + 90) % 360
                self.detour_moves = 1
                if self.blocked >= 6:
                    raise NavigationStalled('Six movement obstacles exhausted the local detour policy')
            elif result.get('result') == 'observed' and self.detour_moves:
                self.detour_moves -= 1
