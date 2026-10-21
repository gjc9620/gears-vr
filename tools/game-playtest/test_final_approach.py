import unittest

from navigation import choose_route


class FinalApproachTests(unittest.TestCase):
    def graph(self):
        return dict(consistent_reread=True,
                    nodes={'a': dict(position=[0, 0, 0]), 'b': dict(position=[1000, 0, 0])},
                    edges=[dict(start='a', end='b', distance=1000, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])

    def test_sparse_graph_can_close_a_ground_gap_outside_trigger_range(self):
        target = [1275, 0, 80]
        route = choose_route(self.graph(), [0, 0, 0], target=target)
        self.assertTrue(route['approach_target'])
        self.assertEqual(route['waypoints'][-1], target)
        self.assertFalse(route['forced_hints_allowed'])
        self.assertFalse(route['final_approach_walkability_verified'])

    def test_distant_or_other_floor_goal_is_not_appended(self):
        for target in ([1500, 0, 0], [1000, 0, 250]):
            route = choose_route(self.graph(), [0, 0, 0], target=target)
            self.assertFalse(route['approach_target'])
            self.assertEqual(route['waypoints'][-1], [1000, 0, 0])
