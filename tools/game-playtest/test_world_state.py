import unittest
import struct
from unittest.mock import Mock, patch

from world_state import Query, objective_fingerprint, select_hostile, registry_append_only, combat_control, squad_hostility, hostile, render_hint


class CombatControlTests(unittest.TestCase):
    def mounted(self):
        pawn, vehicle, controller, world = [dict(address=i, cls=i) for i in range(1, 5)]
        links = {(1, 'DrivenVehicle'): vehicle, (2, 'Driver'): pawn, (2, 'Controller'): controller,
                 (2, 'WorldInfo'): world, (3, 'Pawn'): vehicle, (3, 'WorldInfo'): world}
        q = Mock()
        q.pointer.side_effect = lambda obj, name: links.get((obj['address'], name))
        q.s.derives.side_effect = lambda cls, name: (cls, name) in ((2, 'Vehicle'), (3, 'AIController'))
        return q, pawn, vehicle, controller, world, links

    def test_mounted_ai_requires_driver_and_controlled_vehicle_backlinks(self):
        q, pawn, vehicle, controller, world, _ = self.mounted()
        result = combat_control(q, pawn, world)
        self.assertIsNone(result['direct_controller'])
        self.assertEqual(result['controller'], controller)
        self.assertEqual(result['controlled_pawn'], vehicle)
        self.assertTrue(result['ai_controller'])

    def test_wrong_driver_world_or_controller_pawn_is_rejected(self):
        for key in ((2, 'Driver'), (2, 'WorldInfo'), (3, 'Pawn')):
            q, pawn, vehicle, controller, world, links = self.mounted()
            links[key] = dict(address=999, cls=1)
            with self.assertRaises(RuntimeError):
                combat_control(q, pawn, world)

    def test_squad_hostility_requires_exact_ally_team_and_live_enemy(self):
        q = Mock();q.s.derives.return_value = True;q.scalar.return_value = 100
        local, enemy = dict(address=1), dict(address=2, cls=3)
        own = dict(team=dict(address=10), index=0)
        opponent = dict(team=dict(address=20), index=1)
        control = dict(ai_controller=True, enemy=enemy, controlled_pawn=dict(address=4))
        def check(teams):
            with patch('world_state.team_membership', side_effect=teams):
                return squad_hostility(q, local, control, dict(address=5))
        self.assertTrue(check([own, opponent, own])['verified'])
        self.assertIsNone(check([own, own]))
        self.assertIsNone(check([own, dict(team=dict(address=20), index=0)]))
        self.assertIsNone(check([own, opponent, dict(team=dict(address=30), index=0)]))
        q.scalar.return_value = 0
        self.assertIsNone(check([own, opponent, own]))
        control['ai_controller'] = False
        self.assertIsNone(squad_hostility(q, local, control, dict(address=5)))


class WorldStateTests(unittest.TestCase):
    def test_reader_loss_during_transition_is_a_recoverable_non_admission(self):
        observer = Mock(resolver=None, sampler=None)
        with self.assertRaisesRegex(RuntimeError, 'fresh live readers'):
            Query(observer)

    def test_registry_append_does_not_equate_reallocation_or_shrinkage(self):
        old = struct.pack('<Qii', 0x1000, 10, 100)
        self.assertTrue(registry_append_only(old, struct.pack('<Qii', 0x1000, 11, 100)))
        self.assertFalse(registry_append_only(old, struct.pack('<Qii', 0x2000, 11, 100)))
        self.assertFalse(registry_append_only(old, struct.pack('<Qii', 0x1000, 9, 100)))
        self.assertFalse(registry_append_only(old, struct.pack('<Qii', 0x1000, 11, 200)))

    def test_append_permission_still_rejects_selected_identity_change(self):
        q = Query.__new__(Query)
        q.observer, q.s, q.r = Mock(), Mock(), Mock()
        q.headers = [(100, struct.pack('<Qii', 0x1000, 10, 100))]
        q.metadata, q.identities = {}, {200: dict(address=200, internal_index=1)}
        q.r.read.return_value = struct.pack('<Qii', 0x1000, 11, 100)
        q.s.fresh.return_value = q.identities[200]
        q.finish(allow_registry_growth=True)
        with self.assertRaises(RuntimeError):
            q.finish()
        q.s.fresh.return_value = dict(address=200, internal_index=2)
        with self.assertRaises(RuntimeError):
            q.finish(allow_registry_growth=True)

    def test_only_semantic_objective_values_affect_progress(self):
        a = dict(kind='StructProperty', interpreted=True, rows=[dict(ID=1, Title='Open door')],
                 fields=[dict(address=1000)])
        b = dict(a, fields=[dict(address=2000)])
        self.assertEqual(objective_fingerprint(a), objective_fingerprint(b))
        b['rows'] = [dict(ID=2, Title='Find exit')]
        self.assertNotEqual(objective_fingerprint(a), objective_fingerprint(b))
        b['interpreted'] = False
        self.assertIsNone(objective_fingerprint(b))

    def test_hostile_requires_current_ai_targeting_player_and_positive_health(self):
        def actor(address, **overrides):
            return dict(identity=dict(address=address), targets_player=True, ai_controller=True,
                        health=100, position=[100, 0, 0], **overrides)
        hostile = actor(3)
        friendly = actor(1);friendly['targets_player'] = False
        dead = actor(2);dead['health'] = 0
        value = dict(source_sample=dict(pose=dict(position=[0, 0, 0])), pawns=[friendly, dead, hostile])
        self.assertEqual(select_hostile(value), hostile)
        hostile['position'] = [100, 0, 500]
        self.assertIsNone(select_hostile(value))

    def test_objective_notification_does_not_count_as_progress(self):
        a = dict(kind='StructProperty', struct='ObjectiveInfo', interpreted=True,
                 rows=[dict(ObjectiveName='4', bCompleted=False, bFailed=False,
                            UpdatedTime=1.0, bUpdated=True, bNotifyPlayer=False)])
        b = dict(a, rows=[dict(a['rows'][0], UpdatedTime=50.0, bUpdated=False, bNotifyPlayer=True)])
        self.assertEqual(objective_fingerprint(a), objective_fingerprint(b))
        b['rows'][0]['bCompleted'] = True
        self.assertNotEqual(objective_fingerprint(a), objective_fingerprint(b))

    def test_squad_target_requires_verified_hostility_and_ai(self):
        actor = dict(identity=dict(address=2), targets_player=False, ai_controller=True,
                     health=100, position=[100, 0, 0], squad_hostility=dict(verified=True))
        value = dict(source_sample=dict(pose=dict(position=[0, 0, 0])), pawns=[actor])
        self.assertEqual(select_hostile(value), actor)
        actor['squad_hostility']['verified'] = False
        self.assertIsNone(select_hostile(value))
        actor['squad_hostility']['verified'] = True;actor['ai_controller'] = False
        self.assertIsNone(select_hostile(value))

    def test_null_squad_proof_is_a_false_state_and_can_be_counted(self):
        pawns = [dict(targets_player=False, squad_hostility=None, ai_controller=True, health=100),
                 dict(targets_player=True, squad_hostility=None, ai_controller=True, health=100),
                 dict(targets_player=False, squad_hostility=dict(verified=True), ai_controller=True, health=100)]
        self.assertIs(hostile(pawns[0]), False)
        self.assertEqual(sum(hostile(p) for p in pawns), 2)

    def test_mounted_threat_priority_preserves_health_and_range_admission(self):
        ordinary = dict(identity=dict(address=1), targets_player=True, ai_controller=True,
                        health=100, position=[500, 0, 0])
        gunner = dict(identity=dict(address=2), targets_player=True, ai_controller=True,
                      health=200, position=[2200, 0, 256], driven_vehicle=dict(address=3))
        value = dict(source_sample=dict(pose=dict(position=[0, 0, 0])), pawns=[ordinary, gunner])
        self.assertEqual(select_hostile(value, max_height=2500), ordinary)
        self.assertEqual(select_hostile(value, max_height=2500, prefer_mounted=True), gunner)
        gunner['health'] = 0
        self.assertEqual(select_hostile(value, max_height=2500, prefer_mounted=True), ordinary)

    def test_render_policy_defers_never_drawn_or_unknown_targets(self):
        ordinary = dict(identity=dict(address=1), targets_player=True, ai_controller=True,
                        health=100, position=[500, 0, 0], render_hint=dict(recently_rendered=True))
        gunner = dict(identity=dict(address=2), targets_player=True, ai_controller=True,
                      health=200, position=[2000, 0, 0], driven_vehicle=dict(address=3),
                      render_hint=dict(recently_rendered=False))
        value = dict(source_sample=dict(pose=dict(position=[0, 0, 0])), pawns=[ordinary, gunner])
        self.assertEqual(select_hostile(value, prefer_mounted=True, require_recent_render=True), ordinary)
        gunner.pop('render_hint')
        self.assertEqual(select_hostile(value, prefer_mounted=True, require_recent_render=True), ordinary)
        gunner['render_hint'] = dict(recently_rendered=True, line_of_sight_verified=False)
        self.assertEqual(select_hostile(value, prefer_mounted=True, require_recent_render=True), gunner)

    def test_render_hint_rejects_sentinels_stale_times_and_hidden_body(self):
        for actor_time, mesh_time, hidden, expected in (
                (0, -1000, False, False), (19.9, -1000, False, False), (10, 10, False, False),
                (19.9, 19.9, False, True), (19.9, 19.9, True, False),
                (25, 25, False, False)):
            q = Mock()
            pawn, mesh, world = dict(address=1), dict(address=2, outer=1, cls=3), dict(address=4)
            q.scalar.side_effect = [actor_time, mesh_time, 20]
            q.prop.side_effect = lambda obj, name, kind: dict(type=kind, size=8 if name == 'Mesh' else 4, address=100)
            q.value.side_effect = [b'\0' * 4, struct.pack('<Q', 2), b'\0' * 4]
            q.decode.side_effect = [False, hidden]
            q.obj.return_value = mesh
            q.s.derives.return_value = True
            result = render_hint(q, pawn, world)
            self.assertIs(result['recently_rendered'], expected)
            self.assertIs(result['line_of_sight_verified'], False)


if __name__ == '__main__':
    unittest.main()
