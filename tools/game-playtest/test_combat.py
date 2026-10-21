import copy
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from combat import plan, axis_decision, aimed_burst, observed_axis_rate, aim_calibration_state
from test_core import sample


class CombatTests(unittest.TestCase):
    def world(self):
        state = sample(1)
        state['pose'].update(camera_position=[0, 0, 40], camera_rotation_ru=[0, 0, 0])
        target = dict(identity=dict(address=2), controller=dict(address=3), ai_controller=True,
                      targets_player=True, health=200, position=[500, 0, 0],
                      collision=dict(height=80, radius=40))
        return dict(source_sample=state, end_sample=state, pawns=[target])

    def test_aligned_live_hostile_can_receive_bounded_fire(self):
        world = self.world()
        decision = plan(world, turn_rate=-120, turn_name='turn_left')
        self.assertEqual(decision['name'], 'fire')
        self.assertEqual(decision['seconds'], .25)
        self.assertFalse(decision['line_of_sight_verified'])

    def test_unproven_enemy_or_dead_actor_does_not_admit_fire(self):
        world = self.world()
        world['pawns'][0]['targets_player'] = False
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))
        world['pawns'][0].update(targets_player=True, health=0)
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))

    def test_vertical_miss_does_not_count_as_aimed_fire(self):
        world = self.world()
        world['end_sample']['pose']['camera_position'][2] = 200
        world['end_sample']['pose']['camera_rotation_ru'][1] = 16384
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))

    def test_observed_pawn_in_firing_lane_blocks_fire(self):
        world = self.world()
        ally = copy.deepcopy(world['pawns'][0])
        ally.update(identity=dict(address=4), targets_player=False, position=[250, 0, 0])
        world['pawns'].append(ally)
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))

    def test_target_turn_is_bounded_and_requires_calibration(self):
        world = self.world()
        world['pawns'][0]['position'] = [0, -500, 0]
        self.assertIsNone(plan(world, turn_rate=None, turn_name='turn_left'))
        decision = plan(world, turn_rate=-120, turn_name='turn_left')
        self.assertEqual(decision['name'], 'turn_left')
        self.assertEqual(decision['seconds'], .20)

    def test_cinematic_inhibits_combat(self):
        world = self.world()
        world['end_sample']['cinematic'] = True
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))

    def test_vertical_miss_uses_only_a_calibrated_bounded_axis(self):
        world = self.world()
        world['end_sample']['pose']['camera_position'][2] = 200
        calibration = dict(pitch=dict(axis='aLookUp', rate=100, value=.2))
        decision = plan(world, turn_rate=-120, turn_name='turn_left', look_calibration=calibration)
        self.assertEqual(decision['name'], 'look_pitch')
        self.assertLess(decision['value'], 0)
        self.assertLessEqual(decision['target_degrees'], 10)
        self.assertLessEqual(decision['seconds'], .20)

    def test_calibrated_yaw_takes_the_signed_short_direction(self):
        world = self.world()
        world['pawns'][0]['position'] = [0, -500, 0]
        calibration = dict(yaw=dict(axis='aTurn', rate=120, value=.5))
        decision = plan(world, turn_rate=-120, turn_name='turn_left', look_calibration=calibration)
        self.assertEqual(decision['name'], 'look_yaw')
        self.assertLess(decision['value'], 0)
        self.assertEqual(decision['target_degrees'], 45)

    def test_small_correction_reduces_input_magnitude_instead_of_hitting_pulse_floor(self):
        calibration = dict(yaw=dict(axis='aTurn', rate=3000, value=.05))
        step = axis_decision(calibration, 'yaw', 2)
        self.assertLess(abs(step['value']), .01)
        self.assertGreaterEqual(step['seconds'], .1)
        self.assertLessEqual(3000 * abs(step['value']) * (step['seconds'] + .05), 2)
        self.assertIsNone(axis_decision(calibration, 'yaw', 0))

    def test_precision_loop_converges_with_bounded_release_delay(self):
        for rate in (-1200, 3000):
            for initial in (-11, 2, 6):
                error = initial
                calibration = dict(yaw=dict(axis='aTurn', rate=rate, value=.05))
                for _ in range(6):
                    step = axis_decision(calibration, 'yaw', error)
                    if step is None:
                        break
                    error -= rate * step['value'] * (step['seconds'] + .05)
                self.assertLess(abs(error), .5)

    def test_mounted_target_records_upper_body_aim_inside_its_capsule(self):
        world = self.world()
        world['pawns'][0]['driven_vehicle'] = dict(address=50)
        world['end_sample']['pose']['camera_position'][2] = 64
        decision = plan(world, turn_rate=-120, turn_name='turn_left')
        self.assertEqual(decision['aim_position'], [500, 0, 64])
        self.assertEqual(decision['name'], 'fire')

    def test_elevated_mounted_target_requires_calibrated_pitch_and_is_not_filtered_as_another_floor(self):
        world = self.world()
        target = world['pawns'][0]
        target.update(position=[2200, 0, 256], driven_vehicle=dict(address=50))
        self.assertIsNone(plan(world, turn_rate=-120, turn_name='turn_left'))
        calibration = dict(pitch=dict(axis='aLookUp', rate=-1200, value=.05))
        step = plan(world, turn_rate=-120, turn_name='turn_left', look_calibration=calibration)
        self.assertEqual(step['name'], 'look_pitch')
        self.assertLess(step['value'], 0)
        self.assertLessEqual(step['target_degrees'], 10)

    def aimed_fixture(self):
        base, fresh = self.world(), self.world()
        base['end_sample']['pose']['fov'] = 90
        fresh['end_sample']['pose']['fov'] = 60
        decision = dict(name='fire', target=base['pawns'][0], seconds=.25)
        proof = dict(before=fresh['end_sample'])
        observer, report = Mock(), Mock(directory=Path('unused-aimed-test'))
        report.observe.side_effect = lambda row: dict(row, sample_id=12)
        def pulse(key, seconds, **options):
            if key == 'RightMouseButton':
                options['during'](fresh['end_sample'])
            else:
                self.assertEqual(options['held_keys'], ('RightMouseButton',))
            return dict(release_posted=True)
        observer.pulse.side_effect = pulse
        return observer, report, base, fresh, decision, proof

    def test_aimed_fire_requires_fresh_same_target_and_fov_then_uses_owned_chord(self):
        observer, report, base, fresh, decision, proof = self.aimed_fixture()
        with patch('combat.capture_world', return_value=fresh), patch('combat.plan', return_value=decision), patch('combat.certify_target', return_value=proof):
            result = aimed_burst(observer, report, base, decision, {}, None)
        self.assertTrue(result['shot']['aimed'])
        self.assertEqual(result['proof']['before']['sample_id'], 12)
        self.assertEqual([c.args[0] for c in observer.pulse.call_args_list], ['RightMouseButton', 'LeftMouseButton'])

    def test_aimed_view_cannot_fire_after_target_change(self):
        observer, report, base, fresh, decision, proof = self.aimed_fixture()
        changed = dict(decision, target=dict(identity=dict(address=999)))
        with patch('combat.capture_world', return_value=fresh), patch('combat.plan', return_value=changed), patch('combat.certify_target') as certify:
            self.assertIsNone(aimed_burst(observer, report, base, decision, {}, None))
        certify.assert_not_called()
        self.assertEqual([c.args[0] for c in observer.pulse.call_args_list], ['RightMouseButton'])

    def test_unchanged_fov_does_not_admit_an_aimed_shot(self):
        observer, report, base, fresh, decision, proof = self.aimed_fixture()
        fresh['end_sample']['pose']['fov'] = 90
        with patch('combat.capture_world', return_value=fresh), patch('combat.plan') as planning:
            self.assertIsNone(aimed_burst(observer, report, base, decision, {}, None))
        planning.assert_not_called()
        self.assertEqual([c.args[0] for c in observer.pulse.call_args_list], ['RightMouseButton'])

    def test_aimed_view_can_correct_once_and_recertify_before_fire(self):
        observer, report, base, fresh, decision, proof = self.aimed_fixture()
        correction = dict(decision, name='look_pitch', axis='aLookUp', value=.01,
                          seconds=.12, target_degrees=1, yaw_error=.1)
        observer.sample.return_value = fresh['end_sample']
        report.action.return_value = dict(action_id=1)
        with patch('combat.capture_world', return_value=fresh), patch('combat.plan', side_effect=[correction, decision]), patch('combat.certify_target', return_value=proof) as certify, patch('axis_input.pulse', return_value=dict(release_posted=True, neutral_verified=True)) as axis:
            result = aimed_burst(observer, report, base, decision, {}, None)
        self.assertTrue(result['shot']['aimed'])
        self.assertEqual(certify.call_count, 2)
        axis.assert_called_once()

    def test_elevated_mounted_aim_uses_a_narrower_capsule_band(self):
        world = self.world()
        world['pawns'][0].update(position=[2200, 19.2, 0], driven_vehicle=dict(address=50))
        world['end_sample']['pose']['camera_position'][2] = 64
        calibration = dict(pitch=dict(axis='aLookUp', rate=-1200, value=.05),
                           yaw=dict(axis='aTurn', rate=3000, value=.05))
        self.assertEqual(plan(world, turn_rate=-120, turn_name='turn_left', look_calibration=calibration)['name'], 'fire')
        self.assertEqual(plan(world, turn_rate=-120, turn_name='turn_left', look_calibration=calibration, aiming=True)['name'], 'look_yaw')

    def test_aim_response_estimate_requires_real_neutralized_input_and_control(self):
        after = self.world()['end_sample']
        after['pose']['controller_rotation_ru'] = [0, 655, 0]
        receipt = dict(value=.05, actual_hold_seconds=.12, initial_angle=0,
                       writes=1, neutral_verified=True, writer_closed=True)
        self.assertAlmostEqual(observed_axis_rate(receipt, after, 'yaw'), 600, delta=1)
        receipt['neutral_verified'] = False
        self.assertIsNone(observed_axis_rate(receipt, after, 'yaw'))
        receipt['neutral_verified'] = True;after['cinematic'] = True
        self.assertIsNone(observed_axis_rate(receipt, after, 'yaw'))

    def test_aim_gain_cache_is_separate_from_hip_and_survives_only_the_same_pawn_weapon_world(self):
        world = self.world();attempts = {}
        world['source_sample']['weapon_identity'] = dict(address=10, internal_index=11)
        cached = aim_calibration_state(attempts, world);cached['yaw'] = dict(rate=600)
        self.assertIs(aim_calibration_state(attempts, world), cached)
        world['source_sample']['weapon_identity'] = dict(address=20, internal_index=21)
        self.assertEqual(aim_calibration_state(attempts, world), {})
        world['source_sample']['weapon_identity'] = dict(address=10, internal_index=11)
        world['source_sample']['native']['world_generation'] += 1
        self.assertEqual(aim_calibration_state(attempts, world), {})

    def test_cached_aim_gain_does_not_overwrite_the_hip_calibration(self):
        observer, report, base, fresh, decision, proof = self.aimed_fixture()
        hip = dict(yaw=dict(axis='aTurn', rate=3000, value=.05))
        cached = dict(yaw=dict(axis='aTurn', rate=600, value=.05))
        with patch('combat.capture_world', return_value=fresh), patch('combat.plan', return_value=decision) as planning, patch('combat.certify_target', return_value=proof):
            result = aimed_burst(observer, report, base, decision, {}, hip, aimed_cache=cached)
        self.assertTrue(result['shot']['aimed'])
        self.assertEqual(planning.call_args.kwargs['look_calibration']['yaw']['rate'], 600)
        self.assertEqual(hip['yaw']['rate'], 3000)


if __name__ == '__main__':
    unittest.main()
