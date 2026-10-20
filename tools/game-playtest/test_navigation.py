import unittest

from navigation import RouteProgress, choose_route, shortest_path, walking_requirements


class NavigationTests(unittest.TestCase):
    def test_near_cover_start_continues_outgoing_route_without_turning_back_into_cover(self):
        from core import Explorer
        for cover_x, expected in [(-35, 'move_forward'), (-80, 'turn_left')]:
            with self.subTest(cover_x=cover_x):
                graph = dict(consistent_reread=True,
                    nodes=dict(a=dict(position=[cover_x, 0, 33], identity=dict(name='CoverSlotMarker')),
                               b=dict(position=[100, 0, 0], identity=dict(name='PathNode')),
                               c=dict(position=[300, 0, 0], identity=dict(name='PathNode'))),
                    edges=[dict(start=a, end=b, distance=200, walking_hint_eligible=True,
                                **{'class': 'ReachSpec'}) for a, b in [('a', 'b'), ('b', 'c')]])
                route = choose_route(graph, [0, 0, 0], target=[300, 0, 0])
                explorer = Explorer('move_forward', 'turn_left', [0, 0, 0], 0,
                    waypoints=route['waypoints'], waypoint_radii=route['waypoint_arrival_radii'],
                    planar_waypoints=True)
                explorer.turn_rate = -120
                sample = dict(health=100, cinematic=False, pose=dict(position=[0, 0, 0], yaw_ru=0),
                    menu=dict(consistent_reread=True, read_only=True, input_block_count=0,
                              top=dict(tag='PenderCrossplayInvitePopUp', input_mode=2)))
                self.assertEqual(explorer.next(sample)[0], expected)
                self.assertEqual(explorer.waypoints[0][0], 100 if cover_x == -35 else cover_x)

    def test_committed_exploration_goal_does_not_flip_to_the_farthest_reverse_node(self):
        nodes = {key: dict(position=[x, 0, 0]) for key, x in
                 [('a', 0), ('middle', 1200), ('end', 2800), ('back', -1000)]}
        edges = []
        for a, b in [('a', 'middle'), ('middle', 'end'), ('a', 'back')]:
            for start, end in [(a, b), (b, a)]:
                edges.append(dict(start=start, end=end, distance=abs(nodes[start]['position'][0]-nodes[end]['position'][0]),
                                  walking_hint_eligible=True, **{'class': 'ReachSpec'}))
        graph = dict(nodes=nodes, edges=edges, consistent_reread=True)
        self.assertEqual(choose_route(graph, [0, 0, 0])['goal'], 'end')
        self.assertEqual(choose_route(graph, [1200, 0, 0])['goal'], 'back')
        committed = choose_route(graph, [1200, 0, 0], goal_node='end')
        self.assertEqual(committed['goal'], 'end')
        self.assertTrue(committed['committed_destination'])
        self.assertIsNone(choose_route(graph, [1200, 0, 0], goal_node='missing'))

    def test_committed_goal_keeps_the_final_close_waypoint_instead_of_selecting_another_goal(self):
        graph = dict(consistent_reread=True, nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[600, 0, 0])),
                     edges=[dict(start='a', end='b', distance=600, walking_hint_eligible=True, **{'class': 'ReachSpec'})])
        route = choose_route(graph, [595, 0, 0], goal_node='b')
        self.assertEqual(route['nodes'], ['b'])
        self.assertEqual(route['waypoint_arrival_radii'], [8])

    def test_extra_reach_bits_and_insufficient_clearance_are_not_walk_admitted(self):
        fields = dict(reachFlags=1, CollisionRadius=34, CollisionHeight=80)
        self.assertTrue(walking_requirements(fields, 34, 72))
        for change in (dict(reachFlags=9), dict(reachFlags=0), dict(reachFlags=True),
                       dict(CollisionRadius=33), dict(CollisionHeight=71),
                       dict(CollisionHeight=None)):
            self.assertFalse(walking_requirements(dict(fields, **change), 34, 72))
        self.assertFalse(walking_requirements(fields, float('nan'), 72))
        self.assertFalse(walking_requirements({}, 34, 72))

    def test_missing_admission_and_mixed_traversal_shortcuts_use_the_detour(self):
        nodes = dict(a={}, b={}, c={})
        edges = [dict(start='a', end='c', distance=1, **{'class': 'ReachSpec'}),
                 dict(start='a', end='b', distance=2, walking_hint_eligible=True, **{'class': 'ReachSpec'}),
                 dict(start='b', end='c', distance=2, walking_hint_eligible=True, **{'class': 'ReachSpec'})]
        self.assertEqual(shortest_path(nodes, edges, 'a', 'c'), ['a', 'b', 'c'])
        edges[0]['walking_hint_eligible'] = False
        self.assertEqual(shortest_path(nodes, edges, 'a', 'c'), ['a', 'b', 'c'])

    def test_ground_node_precision_stays_aligned_with_the_appended_goal(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0], identity=dict(name='PathNode')),
                                b=dict(position=[600, 0, 0], identity=dict(name='CoverLink'))),
                     edges=[dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        route = choose_route(graph, [0, 0, 0], target=[700, 0, 0])
        self.assertEqual(route['waypoint_arrival_radii'], [8, 8, 50])
        self.assertEqual(len(route['waypoints']), len(route['waypoint_arrival_radii']))
        self.assertFalse(route['precision_is_clearance_proof'])
    def test_long_edge_keeps_progress_without_reaching_a_waypoint(self):
        progress = RouteProgress()
        for now in range(0, 91, 10):
            self.assertFalse(progress.stalled([[1000, 0, 0]], [now * 5, 0, 0], now))

    def test_oscillation_and_small_jitter_do_not_extend_the_stall_budget(self):
        progress = RouteProgress()
        self.assertFalse(progress.stalled([[1000, 0, 0]], [100, 0, 0], 0))
        self.assertFalse(progress.stalled([[1000, 0, 0]], [109, 0, 0], 10))
        self.assertFalse(progress.stalled([[1000, 0, 0]], [90, 0, 0], 20))
        self.assertTrue(progress.stalled([[1000, 0, 0]], [109, 0, 0], 30))

    def test_waypoint_change_and_explicit_resume_start_a_fresh_interval(self):
        progress = RouteProgress()
        self.assertFalse(progress.stalled([[1000, 0, 0], [1500, 0, 0]], [100, 0, 0], 0))
        self.assertFalse(progress.stalled([[1500, 0, 0]], [950, 0, 0], 31))
        self.assertTrue(progress.stalled([[1500, 0, 0]], [950, 0, 0], 61))
        progress.reset()
        self.assertFalse(progress.stalled([[1500, 0, 0]], [950, 0, 0], 90))

    def test_uses_directed_lower_cost_route(self):
        nodes = {key: {} for key in ('a', 'b', 'c')}
        edges = [dict(start='a', end='c', distance=10, **{'class': 'ReachSpec', 'walking_hint_eligible': True}),
                 dict(start='a', end='b', distance=2, **{'class': 'ReachSpec', 'walking_hint_eligible': True}),
                 dict(start='b', end='c', distance=2, **{'class': 'ReachSpec', 'walking_hint_eligible': True})]
        self.assertEqual(shortest_path(nodes, edges, 'a', 'c'), ['a', 'b', 'c'])
        self.assertIsNone(shortest_path(nodes, edges, 'c', 'a'))

    def test_special_traversal_and_stale_targets_are_not_walk_routes(self):
        nodes = dict(a={}, b={})
        edges = [dict(start='a', end='b', distance=1, **{'class': 'LadderReachSpec'}),
                 dict(start='a', end='stale', distance=1, **{'class': 'ReachSpec', 'walking_hint_eligible': True})]
        self.assertIsNone(shortest_path(nodes, edges, 'a', 'b'))

    def test_route_requires_current_graph_and_nearby_same_height_start(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[600, 0, 0])),
                     edges=[dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        route = choose_route(graph, [10, 0, 0])
        self.assertEqual(route['nodes'], ['a', 'b'])
        self.assertTrue(route['route_is_hint'])
        self.assertIsNone(choose_route(graph, [0, 0, 500]))
        self.assertIsNone(choose_route(graph, [0, 0, 0], avoid={'b'}))
        graph['consistent_reread'] = False
        self.assertIsNone(choose_route(graph, [0, 0, 0]))

    def test_target_approach_is_bounded_to_a_nearby_navigation_endpoint(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[600, 0, 0])),
                     edges=[dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        near = choose_route(graph, [0, 0, 0], target=[700, 0, 0])
        self.assertTrue(near['approach_target'])
        self.assertEqual(near['waypoints'][-1], [700, 0, 0])
        far = choose_route(graph, [0, 0, 0], target=[5000, 0, 0])
        self.assertFalse(far['approach_target'])
        self.assertEqual(far['waypoints'][-1], [600, 0, 0])

    def test_explicit_far_goal_can_use_loaded_global_route(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[5000, 0, 0])),
                     edges=[dict(start='a', end='b', distance=5000, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        self.assertIsNone(choose_route(graph, [0, 0, 0]))
        self.assertEqual(choose_route(graph, [0, 0, 0], target=[5000, 0, 0])['goal'], 'b')

    def test_target_endpoint_survives_graph_refresh_and_exploration_history(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[600, 0, 0])),
                     edges=[dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        self.assertIsNone(choose_route(graph, [0, 0, 0], avoid={'b'}))
        self.assertEqual(choose_route(graph, [0, 0, 0], avoid={'b'}, target=[600, 0, 0])['goal'], 'b')

    def test_only_explicit_ground_forced_hints_join_the_walking_graph(self):
        nodes = dict(a={}, b={})
        edge = dict(start='a', end='b', distance=100, **{'class': 'ForcedReachSpec', 'walking_hint_eligible': True})
        self.assertIsNone(shortest_path(nodes, [edge], 'a', 'b'))
        edge['ground_hint_eligible'] = True
        self.assertEqual(shortest_path(nodes, [edge], 'a', 'b'), ['a', 'b'])
        edge['class'] = 'MantleReachSpec'
        self.assertIsNone(shortest_path(nodes, [edge], 'a', 'b'))

    def test_route_keeps_ordinary_detour_before_a_forced_door_shortcut(self):
        graph = dict(consistent_reread=True,
                     nodes=dict(a=dict(position=[0, 0, 0]), b=dict(position=[0, 600, 0]),
                                c=dict(position=[600, 0, 0])),
                     edges=[dict(start='a', end='c', distance=600, ground_hint_eligible=True, **{'class': 'ForcedReachSpec', 'walking_hint_eligible': True}),
                            dict(start='a', end='b', distance=600, **{'class': 'ReachSpec', 'walking_hint_eligible': True}),
                            dict(start='b', end='c', distance=900, **{'class': 'ReachSpec', 'walking_hint_eligible': True})])
        route = choose_route(graph, [0, 0, 0], target=[600, 0, 0])
        self.assertEqual(route['nodes'], ['a', 'b', 'c'])
        self.assertFalse(route['forced_hints_allowed'])


if __name__ == '__main__':
    unittest.main()
