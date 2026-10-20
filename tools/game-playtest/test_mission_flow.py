import copy
import math
import unittest

from mission_flow import checkpoint_history_key, observed_checkpoint_signals
from mission_flow import nearby_streaming_touch


class StreamingTouchTests(unittest.TestCase):
    def test_only_nearby_loaded_compatible_player_touches_are_navigation_hints(self):
        row = dict(originator=dict(outer=4), bounds=dict(center=[500, 0, 100]),
                   same_world_verified=True, enabled=True, trigger_count=0,
                   player_only=True, player_proximity_allowed=True)
        self.assertTrue(nearby_streaming_touch(row, [0, 0, 0], {4}))
        for change in (dict(player_only=False), dict(player_proximity_allowed=False), dict(enabled=False),
                       dict(trigger_count=1), dict(bounds=dict(center=[2000, 0, 0])),
                       dict(bounds=dict(center=[500, 0, 500]))):
            self.assertFalse(nearby_streaming_touch(dict(row, **change), [0, 0, 0], {4}))
        self.assertFalse(nearby_streaming_touch(row, [0, 0, 0], {5}))


class CheckpointHistoryTests(unittest.TestCase):
    def fixture(self):
        return dict(root_path=[['sp_hos_p', 0, 'Package'], ['Main_Sequence', 0, 'Sequence']],
                    event_name=dict(name='Fire', number=11), game_sha256='version', observed_count=1)

    def test_history_requires_positive_observation_or_a_current_checkpoint_binding(self):
        row = self.fixture()
        self.assertIsNotNone(checkpoint_history_key(row))
        for count in (0, -1, True, None):
            self.assertIsNone(checkpoint_history_key(dict(row, observed_count=count)))
        self.assertIsNotNone(checkpoint_history_key(dict(row, observed_count=0, current_checkpoint_verified=True)))

    def test_history_identity_keeps_package_version_and_complete_fname(self):
        row = self.fixture()
        key = checkpoint_history_key(row)
        self.assertNotEqual(key, checkpoint_history_key(dict(row, game_sha256='other')))
        self.assertNotEqual(key, checkpoint_history_key(dict(row, root_path=[['other_map', 0, 'Package']])))
        self.assertNotEqual(key, checkpoint_history_key(dict(row, event_name=dict(name='Fire', number=12))))

    def test_only_observed_signals_authored_upstream_of_checkpoint_actions_are_exported(self):
        row = self.fixture()
        graph = dict(parent_path=row['root_path'], disk_game_sha256='version',
                     nodes=dict(check=dict(kind='SeqAct_WarCheckPoint'),
                                used=dict(kind='SeqEvent_RemoteEvent', trigger_count=1, labels=dict(EventName=row['event_name'])),
                                other=dict(kind='SeqEvent_RemoteEvent', trigger_count=3, labels=dict(EventName=dict(name='Unrelated', number=0)))),
                     edges=[dict(source='used', target='check')])
        records = observed_checkpoint_signals(graph)
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0]['event_name'], row['event_name'])
from unittest.mock import Mock, patch

from mission_flow import choose_choice_goal, parent_graph, checkpoint_touch_rows, capture_checkpoint_goals, CheckpointCaptureBudget
from test_core import sample


class CheckpointLinkTests(unittest.TestCase):
    def test_budgeted_discovery_resumes_at_the_unvisited_operation_and_rejects_old_context(self):
        world = dict(address=3, outer=4)
        observer = Mock(identities=dict(game_identity=dict(pid=10, creation_time=100)))
        observer.sampler.pawn = 1000
        observer.sample.return_value = sample(0)
        query = Mock()
        query.pointer.return_value = world
        operations = [dict(identity=dict(address=a), parent=dict(address=p),
                           labels=dict(EventName=dict(name=n, number=0)))
                      for a, p, n in [(201, 200, 'A'), (301, 300, 'B')]]
        root = dict(world_identity=world, parent=dict(address=100), nodes={
            'cp':dict(kind='SeqAct_WarCheckPoint'),
            'a':dict(kind='SeqEvent_RemoteEvent', enabled=True, trigger_count=0, labels=operations[0]['labels']),
            'b':dict(kind='SeqEvent_RemoteEvent', enabled=True, trigger_count=0, labels=operations[1]['labels'])},
            edges=[dict(source='a', target='cp'), dict(source='b', target='cp')])

        def operation_graph(address, count):
            operation = operations[0 if address == 200 else 1]
            key = hex(operation['identity']['address'])
            return dict(world_identity=world, parent=dict(address=address), nodes={
                key:dict(kind='SeqAct_ActivateRemoteEvent'),
                'touch':dict(kind='SeqEvent_Touch', identity=dict(address=6), originator=dict(address=7, outer=4),
                             same_world_verified=True, enabled=True, trigger_count=count,
                             bounds=dict(center=[0, 0, 0], extent=[10, 10, 10]))},
                edges=[dict(source='touch', target=key, description='Touched')])

        budget = [True]
        calls = []

        def graph_reader(_observer, address, **kwargs):
            calls.append(address)
            if address == 100:
                return root
            if address == 300 and budget:
                budget.pop()
                raise CheckpointCaptureBudget('bounded pass full')
            return operation_graph(address, 1 if address == 200 else 0)

        def discover(_observer, kind):
            return dict(matches=[dict(parent=dict(address=100))]) if kind == 'SeqAct_WarCheckPoint' else dict(matches=operations)

        with (patch('mission_flow.Query', return_value=query),
              patch('world_state.streaming_levels', return_value=dict(loaded=[])),
              patch('mission_flow.find_operations', side_effect=discover),
              patch('mission_flow.parent_graph', side_effect=graph_reader)):
            first = capture_checkpoint_goals(observer)
            self.assertEqual(first['rows'], [])
            self.assertFalse(first['operation_scan_complete'])
            self.assertEqual(first['next_cursor']['operation'], dict(address=301))
            calls.clear()
            second = capture_checkpoint_goals(observer, resume_cursor=first['next_cursor'])
            self.assertEqual([row['identity']['address'] for row in second['rows']], [6])
            self.assertTrue(second['operation_scan_complete'])
            self.assertNotIn(200, calls)
            calls.clear()
            observer.sample.return_value = sample(0, world=2)
            capture_checkpoint_goals(observer, resume_cursor=first['next_cursor'])
            self.assertIn(200, calls)

    def fixture(self):
        parent = dict(parent=dict(address=1), nodes={
            'sub':dict(kind='Sequence', identity=dict(address=2)),
            'goal':dict(kind='SeqAct_ActivateRemoteEvent')},
            edges=[dict(source='sub', target='goal', description='Done')])
        child = dict(parent=dict(address=2), nodes={
            'touch':dict(kind='SeqEvent_Touch', identity=dict(address=3)),
            'finish':dict(kind='SeqAct_FinishSequence', labels=dict(OutputLabel='Done'))},
            edges=[dict(source='touch', target='finish', description='Touched')])
        return parent, child

    def test_checkpoint_link_follows_the_matching_child_output_port(self):
        parent, child = self.fixture()
        reader = Mock(return_value=child)
        rows = checkpoint_touch_rows(parent, {'goal'}, reader)
        self.assertEqual([r['identity']['address'] for r in rows], [3])
        self.assertEqual(rows[0]['parent_sequence'], dict(address=2))
        reader.assert_called_once_with(2)

    def test_an_unrelated_finish_port_does_not_supply_a_touch_goal(self):
        parent, child = self.fixture()
        child['nodes']['finish']['labels']['OutputLabel'] = 'Other'
        self.assertEqual(checkpoint_touch_rows(parent, {'goal'}, Mock(return_value=child)), [])

    def test_recursive_sequence_links_stop_at_the_bounded_depth(self):
        parent, child = self.fixture()
        child['nodes']['sub'] = dict(kind='Sequence', identity=dict(address=2))
        child['edges'] = [dict(source='sub', target='finish', description='Done')]
        reader = Mock(return_value=child)
        self.assertEqual(checkpoint_touch_rows(parent, {'goal'}, reader), [])
        self.assertEqual(reader.call_count, 2)


class ChoiceGoalTests(unittest.TestCase):
    def test_cylinder_entry_does_not_stop_in_a_bounding_box_corner(self):
        graph, hints = self.fixture()
        graph['nodes'] = dict(a=dict(position=[90, 90, 0]))
        graph['edges'] = []
        hints['rows'][0]['bounds'] = dict(center=[0, 0, 0], extent=[100, 100, 100])
        hints['rows'][0]['trigger_shape'] = dict(kind='cylinder', radius=100, half_height=100)
        goal = choose_choice_goal(graph, hints, [90, 90, 0])
        self.assertAlmostEqual(math.hypot(*goal['position'][:2]), 80)
        self.assertLess(goal['position'][0], 80)

    def test_focused_parent_budget_cannot_expand_beyond_the_explicit_bound(self):
        observer = Mock()
        for limit in (0, 513, True):
            with self.subTest(limit=limit), self.assertRaises(ValueError):
                parent_graph(observer, 1, max_nodes=limit)
        observer.sample.assert_not_called()

    def test_wide_trigger_uses_a_short_entry_before_a_distant_inside_node(self):
        graph, hints = self.fixture()
        graph['nodes'] = dict(start=dict(position=[0, -200, 0]), near=dict(position=[0, -20, 0]),
                              far=dict(position=[300, 0, 0]))
        graph['edges'] = [dict(start='start', end='near', **{'class': 'ReachSpec', 'walking_hint_eligible': True}),
                          dict(start='near', end='far', **{'class': 'ReachSpec', 'walking_hint_eligible': True})]
        hints['rows'][0]['bounds'] = dict(center=[200, 0, 0], extent=[200, 10, 100])
        goal = choose_choice_goal(graph, hints, [0, -200, 0])
        self.assertEqual(goal['reachable_node'], 'near')
        self.assertEqual(goal['position'], [20, -5, 0])

    def fixture(self):
        context = dict(game_identity=dict(pid=1, creation_time=2), world_identity=dict(address=3),
                       source_sample=dict(native=dict(world_generation=4)))
        graph = dict(context, consistent_reread=True,
                     nodes=dict(a=dict(position=[600, 0, 0]), b=dict(position=[60, 0, 0]),
                                disconnected=dict(position=[0, 0, 0])),
                     edges=[dict(start='a', end='b', **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        hints = dict(context, consistent_metadata_reread=True, rows=[dict(
            identity=dict(address=10, internal_index=11, number=0), same_world_verified=True,
            enabled=True, trigger_count=0, parent_sequence=dict(address=12),
            position=[0, 0, 0], bounds=dict(center=[0, 0, 0], extent=[16, 1000, 100], radius=1010))])
        return graph, hints

    def test_brush_bounds_supply_short_interior_approach(self):
        graph, hints = self.fixture()
        goal = choose_choice_goal(graph, hints, [600, 0, 0])
        self.assertEqual(goal['position'], [8, 0, 0])
        self.assertEqual(goal['reachable_node'], 'b')
        self.assertEqual(goal['arrival_radius'], 20)
        self.assertEqual(goal['goal_kind'], 'touch')
        self.assertFalse(goal['mission_order_verified'])

    def test_consumed_disabled_and_retired_triggers_are_excluded(self):
        graph, hints = self.fixture()
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0], avoid={(4, 10, 11, 0)}))
        hints['rows'][0]['trigger_count'] = 1
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0]))
        hints['rows'][0].update(trigger_count=0, enabled=False)
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0]))

    def test_disconnected_and_vertical_goals_do_not_supply_approach(self):
        graph, hints = self.fixture()
        graph['edges'] = []
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0]))
        graph, hints = self.fixture()
        hints['rows'][0]['bounds']['center'][2] = 200
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0]))

    def test_context_and_fresh_world_are_required(self):
        graph, hints = self.fixture()
        changed = copy.deepcopy(hints)
        changed['source_sample']['native']['world_generation'] = 5
        self.assertIsNone(choose_choice_goal(graph, changed, [600, 0, 0]))
        hints['rows'][0]['same_world_verified'] = False
        self.assertIsNone(choose_choice_goal(graph, hints, [600, 0, 0]))


if __name__ == '__main__':
    unittest.main()
