import copy
import unittest

from mission_hints import choose_goal


class MissionHintTests(unittest.TestCase):
    def fixture(self):
        context = dict(world_identity=dict(address=10), game_identity=dict(pid=1, creation_time=2),
                       source_sample=dict(native=dict(world_generation=2)))
        graph = dict(context, consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[600, 0, 0]),
                                isolated=dict(position=[100, 0, 0])),
                     edges=[dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        def goal(address, position):
            return dict(position=position, same_world_verified=True, collision=dict(radius=40),
                        interaction_details=dict(event=dict(address=address, internal_index=address, number=0),
                                                 TriggerCount=0, MaxTriggerCount=1, bEnabled=True))
        hints = dict(context, consistent_metadata_reread=True, player_collision=dict(radius=34),
                     rows=[goal(1000, [100, 0, 0]), goal(2000, [630, 0, 40])])
        return graph, hints

    def test_goal_requires_nearby_reachable_node(self):
        graph, hints = self.fixture()
        # The closer disconnected trigger is more than 180 from every reachable node.
        hints['rows'][0]['position'] = [300, 300, 0]
        goal = choose_goal(graph, hints, [0, 0, 0])
        self.assertEqual(goal['event']['address'], 2000)
        self.assertEqual(goal['arrival_radius'], 54)
        self.assertEqual(goal['reachable_nodes'], 2)

    def test_consumed_or_previously_attempted_event_is_excluded(self):
        graph, hints = self.fixture()
        hints['rows'][0]['interaction_details']['TriggerCount'] = 1
        goal = choose_goal(graph, hints, [0, 0, 0])
        self.assertEqual(goal['event']['address'], 2000)
        self.assertIsNone(choose_goal(graph, hints, [0, 0, 0], avoid={(2, 2000, 2000, 0)}))

    def test_goal_reachability_excludes_unadmitted_traversal_edges(self):
        graph, hints = self.fixture()
        hints['rows'][0]['interaction_details']['TriggerCount'] = 1
        graph['edges'][0]['walking_hint_eligible'] = False
        self.assertIsNone(choose_goal(graph, hints, [0, 0, 0]))
        del graph['edges'][0]['walking_hint_eligible']
        self.assertIsNone(choose_goal(graph, hints, [0, 0, 0]))

    def test_different_world_or_generation_cannot_supply_goals(self):
        graph, hints = self.fixture()
        changed = copy.deepcopy(hints)
        changed['world_identity']['address'] = 11
        self.assertIsNone(choose_goal(graph, changed, [0, 0, 0]))
        changed = copy.deepcopy(hints)
        changed['source_sample']['native']['world_generation'] = 3
        self.assertIsNone(choose_goal(graph, changed, [0, 0, 0]))

    def test_unused_ordinary_goal_precedes_closer_forced_goal(self):
        graph, hints = self.fixture()
        graph['edges'].append(dict(start='a', end='isolated', ground_hint_eligible=True, **{'class': 'ForcedReachSpec', 'walking_hint_eligible': True}))
        graph['nodes']['isolated']['position'] = [200, 200, 0]
        hints['rows'][0]['position'] = [200, 200, 0]
        goal = choose_goal(graph, hints, [0, 0, 0])
        self.assertEqual(goal['event']['address'], 2000)
        self.assertFalse(goal['forced_hints_required'])
        hints['rows'][1]['interaction_details']['TriggerCount'] = 1
        goal = choose_goal(graph, hints, [0, 0, 0])
        self.assertEqual(goal['event']['address'], 1000)
        self.assertTrue(goal['forced_hints_required'])


if __name__ == '__main__':
    unittest.main()
