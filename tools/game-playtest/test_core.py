import json
from pathlib import Path
import tempfile
import unittest

from core import Evidence, Explorer, NavigationStalled, gameplay, approach_target


def sample(t, *, published=1, world=1, menu=True, health=100, position=(0, 0, 0), yaw=0):
    return dict(elapsed=t, game_identity={'pid': 10, 'creation_time': 100},
                pawn_identity={'address': 1000, 'internal_index': 4}, health=health, cinematic=False,
                menu=dict(consistent_reread=True, read_only=True, input_block_count=0,
                          top=dict(tag='PenderCrossplayInvitePopUp' if menu else 'WarPause', input_mode=2)),
                pose=dict(position=position, yaw_ru=yaw, camera_timestamp=t),
                native=dict(published=published, world_generation=world), errors=[])


class ApproachTests(unittest.TestCase):
    def test_native_close_approach_has_a_usable_pulse_without_extending_the_hold_limit(self):
        decision = approach_target(sample(0), [43, 0, 0], 'move_forward', 'turn_left', -120,
                                   arrival_radius=35, minimum_walk_seconds=.08, walk_speed=104)
        self.assertEqual(decision['action'], ('move_forward', .08))
        far = approach_target(sample(0), [200, 0, 0], 'move_forward', 'turn_left', -120,
                              arrival_radius=35, minimum_walk_seconds=.08, walk_speed=104)
        self.assertEqual(far['action'], ('move_forward', .18))

    def test_backward_approach_enters_small_trigger_before_facing(self):
        decision = approach_target(sample(0, yaw=32768), [150, 0, 40],
                                   'move_backward', 'turn_left', -120)
        self.assertEqual(decision['phase'], 'approach')
        self.assertEqual(decision['action'][0], 'move_backward')
        self.assertLessEqual(decision['action'][1], .18)

    def test_close_backward_facing_rotates_then_waits_for_current_event(self):
        decision = approach_target(sample(0, yaw=32768), [45, 0, 40],
                                   'move_backward', 'turn_left', -120)
        self.assertEqual(decision['phase'], 'face')
        self.assertEqual(decision['action'][0], 'turn_left')
        decision = approach_target(sample(0), [45, 0, 40],
                                   'move_backward', 'turn_left', -120)
        self.assertIsNone(decision['action'])

    def test_different_floor_is_not_a_direct_approach(self):
        self.assertIsNone(approach_target(sample(0), [45, 0, 500],
                                         'move_backward', 'turn_left', -120))

    def test_touch_arrival_waits_without_turning_back_out_of_volume(self):
        decision = approach_target(sample(0, yaw=32768), [15, 0, 0],
                                   'move_backward', 'turn_left', -120,
                                   arrival_radius=20, face_when_arrived=False)
        self.assertEqual(decision['phase'], 'face')
        self.assertIsNone(decision['action'])


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.report = Evidence(Path(self.temp.name) / 'run', mode='test', provenance={})

    def tearDown(self):
        self.temp.cleanup()

    def test_stall_requires_camera_progress_and_three_seconds(self):
        for t in (0, 1, 2):
            self.report.observe(sample(t))
        self.assertFalse(self.report.issues)
        self.report.observe(sample(3))
        self.report.observe(sample(4))
        self.assertEqual(len(self.report.issues), 1)
        self.assertEqual(next(iter(self.report.issues.values()))['occurrences'], 1)

    def test_pause_missing_counter_and_world_change_break_stall(self):
        for row in (sample(0), sample(1), sample(2, menu=False), sample(3),
                    sample(4, published=None), sample(5), sample(6, world=2), sample(7, world=2)):
            self.report.observe(row)
        self.assertFalse(self.report.issues)

    def test_identity_reuse_invalidates_actions(self):
        a, b = sample(0), sample(1, position=(100, 0, 0))
        b['game_identity'] = dict(pid=10, creation_time=200)
        result = self.report.action('move_forward', a, b, dispatched=True)
        self.assertEqual(result['result'], 'unverified')
        self.assertNotIn('distance', result)

    def test_obstacle_is_not_a_confirmed_game_bug(self):
        a = self.report.observe(sample(0))
        b = self.report.observe(sample(1))
        self.report.action('move_forward', a, b, dispatched=True)
        issue = next(iter(self.report.issues.values()))
        self.assertEqual(issue['classification'], 'navigation_obstacle')
        self.assertFalse(issue['confirmed'])

    def test_no_green_full_coverage_from_passive_samples(self):
        self.report.observe(sample(0))
        report = self.report.save(phase='finished')
        self.assertFalse(report['automatic_full_campaign_verified'])
        self.assertTrue(all(x['status'] == 'not_run' for x in report['coverage'].values()))
        self.assertFalse(report['image_capture'])
        self.assertEqual(json.loads((self.report.directory / 'report.json').read_text())['samples'], 1)

    def test_backward_navigation_calibrates_and_tries_detour(self):
        explorer = Explorer('move_backward', 'turn_left', (0, 0, 0), 0)
        self.assertEqual(explorer.next(sample(0)), ('turn_left', .15))
        explorer.feedback(dict(name='turn_left', result='observed', yaw_degrees=-18), .15)
        explorer.heading = 180
        self.assertEqual(explorer.next(sample(1)), ('move_backward', .45))
        explorer.feedback(dict(name='move_backward', result='no_effect', distance=0), .45)
        self.assertEqual(explorer.next(sample(2))[0], 'turn_left')

    def test_precise_ground_route_reaches_the_corner_before_turning(self):
        explorer = Explorer('move_forward', 'turn_left', (0, -37, 0), 16384,
                            waypoints=[[0, 0, -25], [-122, 39, -25]], waypoint_radii=[8, 8],
                            planar_waypoints=True, heading_tolerance=4)
        explorer.turn_rate = -120
        self.assertEqual(explorer.next(sample(0, position=(0, -37, 0), yaw=16384))[0], 'move_forward')
        self.assertEqual(len(explorer.waypoints), 2)
        explorer.next(sample(1, position=(0, -5, 0), yaw=16384))
        self.assertEqual(explorer.waypoints, [[-122, 39, -25]])
        self.assertEqual(explorer.waypoint_radii, [8])

    def test_ground_arrival_does_not_accept_a_point_on_another_floor(self):
        explorer = Explorer('move_forward', 'turn_left', (0, 0, 0), 0,
                            waypoints=[[0, 0, 400]], waypoint_radii=[8], planar_waypoints=True)
        explorer.turn_rate = -120
        explorer.next(sample(0))
        self.assertFalse(explorer.route_complete)
        self.assertEqual(len(explorer.waypoints), 1)

    def test_verified_short_progress_breaks_the_obstacle_streak(self):
        explorer = Explorer('move_forward', 'turn_left', (0, 0, 0), 0)
        for _ in range(5):
            explorer.feedback(dict(name='move_forward', result='no_effect', distance=0), .08)
        explorer.feedback(dict(name='move_forward', result='observed', distance=5), .08)
        self.assertEqual(explorer.blocked, 0)
        explorer.feedback(dict(name='move_forward', result='no_effect', distance=0), .08)
        self.assertEqual(explorer.blocked, 1)

    def test_unknown_passthrough_ui_is_not_accepted(self):
        row = sample(0)['menu']
        row['top']['tag'] = 'UnknownDialog'
        self.assertFalse(gameplay(row))

    def test_registry_boundary_does_not_become_turn_failure(self):
        explorer = Explorer('move_backward', 'turn_left', (0, 0, 0), 0)
        explorer.feedback(dict(name='turn_left', result='unverified'), .25)
        self.assertIsNone(explorer.turn_rate)
        explorer.feedback(dict(name='turn_left', result='observed', yaw_degrees=-18), .15)
        explorer.feedback(dict(name='turn_left', result='unverified'), .25)
        self.assertEqual(explorer.turn_rate, -120)

    def test_missing_world_or_dead_pawn_cannot_certify_movement(self):
        a, b = sample(0), sample(1, position=(100, 0, 0))
        a['native']['world_generation'] = b['native']['world_generation'] = None
        self.assertEqual(self.report.action('move_forward', a, b, dispatched=True)['result'], 'unverified')
        a, b = sample(0), sample(1, health=0, position=(100, 0, 0))
        self.assertEqual(self.report.action('move_forward', a, b, dispatched=True)['result'], 'unverified')

    def test_collision_detour_is_not_overridden_by_old_waypoint(self):
        explorer = Explorer('move_backward', 'turn_left', (0, 0, 0), 0,
                            waypoints=[(500, 0, 0)])
        explorer.feedback(dict(name='turn_left', result='observed', yaw_degrees=-18), .15)
        explorer.feedback(dict(name='move_backward', result='no_effect', distance=0), .45)
        heading = explorer.heading
        explorer.next(sample(1))
        self.assertEqual(explorer.heading, heading)
        self.assertEqual(explorer.detour_moves, 1)

    def test_short_approach_response_is_not_a_fifteen_unit_obstacle(self):
        a, b = sample(0), sample(1, position=(6, 0, 0))
        result = self.report.action('move_forward', a, b, dispatched=True, detail=dict(seconds=.03))
        self.assertEqual(result['result'], 'observed')
        self.assertFalse(self.report.issues)
        result = self.report.action('move_forward', b, sample(2, position=(6, 0, 0)),
                                    dispatched=True, detail=dict(seconds=.03))
        self.assertEqual(result['result'], 'no_effect')

    def test_camera_bob_alone_does_not_certify_look_input(self):
        before, after = sample(0), sample(1)
        before['pose'].update(controller_rotation_ru=[0, 0, 0], camera_rotation_ru=[0, 0, 0])
        after['pose'].update(controller_rotation_ru=[0, 0, 0], camera_rotation_ru=[100, 0, 0])
        self.assertEqual(self.report.action('look_pitch', before, after, dispatched=True)['result'], 'unverified')
        after['pose']['controller_rotation_ru'][0] = 100
        result = self.report.action('look_pitch', before, after, dispatched=True, neutral=after)
        self.assertEqual(result['result'], 'observed')
        self.assertEqual(result['neutral_pitch_drift'], 0)

    def test_reached_waypoints_are_skipped_and_corner_approach_is_shortened(self):
        explorer = Explorer('move_forward', 'turn_left', (0, 0, 0), 0,
                            waypoints=[(0, 0, 0), (10, 0, 0), (500, 0, 0)])
        explorer.feedback(dict(name='turn_left', result='observed', yaw_degrees=-18), .15)
        self.assertEqual(explorer.next(sample(0)), ('move_forward', .8))
        self.assertEqual(explorer.waypoints, [(500, 0, 0)])
        name, seconds = explorer.next(sample(1, position=(410, 0, 0)))
        self.assertEqual(name, 'move_forward')
        self.assertLess(seconds, .3)
        self.assertLessEqual(410 + seconds * explorer.walk_speed, 500)

    def test_navigation_obstacles_are_local_to_the_stall(self):
        explorer = Explorer('move_forward', 'turn_left', (0, 0, 0), 0)
        for _ in range(8):
            explorer.feedback(dict(name='move_forward', result='no_effect', distance=0), .45)
            explorer.feedback(dict(name='move_forward', result='observed', distance=100), .45)
        self.assertEqual(explorer.blocked, 0)
        for _ in range(5):
            explorer.feedback(dict(name='move_forward', result='no_effect', distance=0), .45)
        with self.assertRaises(NavigationStalled):
            explorer.feedback(dict(name='move_forward', result='no_effect', distance=0), .45)

    def test_lost_turn_response_requests_recovery(self):
        explorer = Explorer('move_backward', 'turn_left', (0, 0, 0), 0)
        explorer.feedback(dict(name='turn_left', result='observed', yaw_degrees=-18), .15)
        with self.assertRaisesRegex(NavigationStalled, 'Previously calibrated'):
            explorer.feedback(dict(name='turn_left', result='no_effect', yaw_degrees=0), .25)

    def test_cinematic_input_is_inhibited_and_recovery_is_observed(self):
        explorer = Explorer('move_backward', 'turn_left', (0, 0, 0), 0)
        a = sample(0)
        a['cinematic'] = True
        self.assertIsNone(explorer.next(a))
        self.report.observe(a)
        b = sample(1, published=2)
        self.report.observe(b)
        self.assertEqual(self.report.coverage['cinematic_transition']['status'], 'not_run')
        self.report.observe(sample(2, published=3))
        self.assertEqual(self.report.coverage['cinematic_transition']['status'], 'passed')
        self.assertEqual(self.report.action('turn_left', a, b, dispatched=True)['result'], 'unverified')

    def test_path_choice_gap_does_not_lose_cinematic_episode(self):
        start = sample(0)
        start['cinematic'] = True
        self.report.observe(start)
        prompt = sample(1, menu=False)
        prompt.pop('cinematic')
        prompt.pop('pose')
        self.report.observe(prompt)
        self.report.observe(sample(2, published=2))
        self.report.observe(sample(3, published=3))
        self.assertEqual(self.report.coverage['cinematic_transition']['status'], 'passed')


if __name__ == '__main__':
    unittest.main()
