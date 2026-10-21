from pathlib import Path
import tempfile
import unittest
from unittest.mock import Mock, patch

from core import Evidence
from run import observe
from live import SessionChanged
from test_core import sample


class NestedObservationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.report = Evidence(Path(self.temp.name) / 'run', mode='test', provenance={})
        self.observer = Mock()
        self.observer.identities = dict(game_identity=dict(pid=10, creation_time=100),
                                        owner_identity=dict(pid=20, creation_time=200))
        self.observer.action_owner = self.observer.identities['owner_identity']
        self.observer.sample.return_value = sample(0)
        self.observer.desktop.return_value = dict(occlusion=[], foreground=1)
        self.observer.user.GetAsyncKeyState.return_value = 0

    def tearDown(self):
        self.temp.cleanup()

    def stop_and_reject(self, *args, **kwargs):
        (self.report.directory / 'stop').touch()
        raise RuntimeError('Nested observation invalidated its readers')

    @patch('world_state.capture')
    def test_failed_nested_sample_cannot_reuse_outer_input_admission(self, capture):
        capture.side_effect = self.stop_and_reject
        observe(self.report, self.observer, 1, explore=True, world_state=True)
        self.observer.bindings.assert_not_called()
        self.observer.pulse.assert_not_called()

    def test_binding_acquisition_failure_discards_readers_without_input(self):
        self.observer.bindings.side_effect = self.stop_and_reject
        observe(self.report, self.observer, 1, explore=True)
        self.observer.close.assert_called_once()
        self.observer.pulse.assert_not_called()

    def test_navigation_rejection_waits_without_falling_back_to_blind_movement(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []

        def state():
            calls.append(1)
            if len(calls) == 3:
                (self.report.directory / 'stop').touch()
            return sample(len(calls))

        self.observer.sample.side_effect = state
        with patch('navigation.capture', side_effect=RuntimeError('Navigation graph changed')) as capture:
            observe(self.report, self.observer, 3, explore=True, navigation=True)
        capture.assert_called_once()
        self.observer.pulse.assert_not_called()

    def test_session_identity_failure_is_terminal(self):
        self.observer.bindings.side_effect = SessionChanged('Owned identity changed')
        with self.assertRaises(SessionChanged):
            observe(self.report, self.observer, 1, explore=True)
        self.observer.pulse.assert_not_called()

    def test_failed_checkpoint_capture_cannot_reuse_outer_input_admission(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(0))
        hints = dict(source_sample=sample(0))
        flow = dict(rows=[], end_sample=sample(0))
        with (patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=hints),
              patch('mission_hints.choose_goal', return_value=None),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', return_value=None),
              patch('mission_flow.capture_checkpoint_goals', side_effect=self.stop_and_reject)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        self.observer.pulse.assert_not_called()

    def test_partial_checkpoint_discovery_waits_before_falling_back_to_an_old_door(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(0))
        flow = dict(rows=[], end_sample=sample(0), errors=[], next_cursor=dict(operation=dict(address=30)))
        door = dict(position=[100, 0, 0], event=dict(address=10), arrival_radius=100, forced_hints_required=False)

        def partial(_observer, **kwargs):
            (self.report.directory / 'stop').touch()
            return flow

        with (patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(0))),
              patch('mission_hints.choose_goal', return_value=door),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', return_value=None),
              patch('mission_flow.capture_checkpoint_goals', side_effect=partial)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        self.assertEqual(self.report.counts['checkpoint_flow_waiting'], 1)
        self.assertEqual(self.report.counts['interaction_goal_selected'], 0)
        self.observer.pulse.assert_not_called()

    def test_reachable_checkpoint_progression_precedes_a_reset_old_door_event(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []

        def state():
            calls.append(1)
            if len(calls) == 2:
                (self.report.directory / 'stop').touch()
            return sample(len(calls))

        self.observer.sample.side_effect = state
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(1))
        door = dict(position=[100, 0, 0], event=dict(address=10), arrival_radius=100,
                    forced_hints_required=False)
        checkpoint = dict(position=[0, 0, 0], event=dict(address=20), arrival_radius=20,
                          goal_kind='touch', forced_hints_required=False)
        flow = dict(rows=[], end_sample=sample(1), errors=[])
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        with (patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(1))),
              patch('mission_hints.choose_goal', return_value=door),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', side_effect=[None, checkpoint]),
              patch('mission_flow.capture_checkpoint_goals', return_value=flow) as capture,
              patch('mission_flow.touch_count', return_value=0),
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        capture.assert_called_once()
        import json
        rows = [json.loads(line) for line in (self.report.directory / 'events.jsonl').read_text().splitlines()]
        selected = next(row for row in rows if row['kind'] == 'interaction_goal_selected')
        self.assertEqual(selected['goal']['event'], dict(address=20))
        self.observer.pulse.assert_not_called()

    def test_touch_goal_is_discarded_before_reading_an_event_from_the_old_world(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []

        def state():
            calls.append(1)
            if len(calls) == 2:
                (self.report.directory / 'stop').touch()
            return sample(len(calls), world=len(calls))

        self.observer.sample.side_effect = state
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(1))
        goal = dict(position=[0, 0, 0], event=dict(address=20), arrival_radius=20, goal_kind='touch')
        flow = dict(rows=[], end_sample=sample(1))
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        with (patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(1))),
              patch('mission_hints.choose_goal', return_value=goal),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', return_value=None),
              patch('mission_flow.touch_count') as count,
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        count.assert_not_called()
        self.observer.pulse.assert_not_called()
        self.assertEqual(self.report.counts['touch_goal_context_changed'], 1)

    def test_active_touch_goal_survives_a_navigation_snapshot_refresh(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []
        clock = [0]

        def state():
            calls.append(1)
            if len(calls) == 3:
                clock[0] = 11
                (self.report.directory / 'stop').touch()
            return sample(len(calls), position=((len(calls) - 1) * 650, 0, 0))

        self.observer.sample.side_effect = state
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(0))
        goal = dict(position=[0, 0, 0], event=dict(address=20), arrival_radius=20, goal_kind='touch')
        flow = dict(rows=[], end_sample=sample(0))
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        explorer.next.return_value = None
        with (patch('run.time.monotonic', side_effect=lambda: clock[0]),
              patch('navigation.capture', return_value=graph) as capture,
              patch('mission_hints.capture', return_value=dict(source_sample=sample(0))),
              patch('mission_hints.choose_goal', return_value=goal) as choose,
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', return_value=None),
              patch('mission_flow.touch_count', return_value=0),
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        self.assertEqual(capture.call_count, 2)
        choose.assert_called_once()
        self.observer.pulse.assert_not_called()

    def test_unfinished_exploration_destination_survives_a_spatial_graph_refresh(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls, clock = [], [0]

        def state():
            calls.append(1)
            if len(calls) == 3:
                clock[0] = 11
                (self.report.directory / 'stop').touch()
            return sample(len(calls), position=((len(calls) - 1) * 650, 0, 0))

        self.observer.sample.side_effect = state
        nodes = {key: dict(identity=dict(address=i), position=[x, 0, 0]) for i, (key, x) in
                 enumerate([('a', 0), ('middle', 1300), ('end', 2800), ('back', -1000)])}
        edges = []
        for a, b in [('a', 'middle'), ('middle', 'end'), ('a', 'back')]:
            for start, end in [(a, b), (b, a)]:
                edges.append(dict(start=start, end=end, distance=abs(nodes[start]['position'][0]-nodes[end]['position'][0]),
                                  walking_hint_eligible=True, **{'class': 'ReachSpec'}))
        graph = dict(nodes=nodes, edges=edges, world_identity=dict(address=3), consistent_reread=True,
                     truncated=False, source_sample=sample(0))
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        explorer.next.return_value = None
        with (patch('run.time.monotonic', side_effect=lambda: clock[0]),
              patch('navigation.capture', return_value=graph) as capture,
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True)
        import json
        rows = [json.loads(line) for line in (self.report.directory / 'events.jsonl').read_text().splitlines()]
        routes = [row for row in rows if row['kind'] == 'navigation_route_selected']
        self.assertEqual(capture.call_count, 2)
        self.assertEqual([row['goal'] for row in routes], ['end', 'end'])
        self.assertEqual(self.report.counts['exploration_destination_selected'], 1)
        self.assertEqual(self.report.counts['exploration_destination_reached'], 0)
        self.observer.pulse.assert_not_called()

    def test_streaming_touch_returns_to_the_suspended_exploration_destination(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls, clock = [], [0]

        def state():
            calls.append(1)
            n = len(calls)
            if n >= 3:
                clock[0] = 11
            if n == 5:
                (self.report.directory / 'stop').touch()
            return sample(n, position=(min(n-1, 2)*650, 0, 0))

        self.observer.sample.side_effect = state
        nodes = {key: dict(identity=dict(address=i), position=[x, 0, 0]) for i, (key, x) in
                 enumerate([('a', 0), ('middle', 1300), ('end', 2800), ('back', -1000)])}
        edges = []
        for a, b in [('a', 'middle'), ('middle', 'end'), ('a', 'back')]:
            for start, end in [(a, b), (b, a)]:
                edges.append(dict(start=start, end=end, distance=abs(nodes[start]['position'][0]-nodes[end]['position'][0]),
                                  walking_hint_eligible=True, **{'class': 'ReachSpec'}))
        graph = dict(nodes=nodes, edges=edges, world_identity=dict(address=3), consistent_reread=True,
                     truncated=False, source_sample=sample(0))
        flow = dict(rows=[], end_sample=sample(0), source_sample=sample(0), errors=[], next_cursor=None)
        empty = dict(flow, tag='streaming')
        active = dict(empty, rows=[dict(identity=dict(address=20))])
        streaming_goal = dict(position=[1300, 0, 0], event=dict(address=20), arrival_radius=20,
                              goal_kind='touch', forced_hints_required=False)
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        explorer.next.return_value = None

        def choose(_graph, hints, _position, **kwargs):
            return streaming_goal if hints.get('tag') == 'streaming' and hints['rows'] else None

        with (patch('run.time.monotonic', side_effect=lambda: clock[0]),
              patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(0))),
              patch('mission_hints.choose_goal', return_value=None),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.capture_checkpoint_goals', return_value=flow),
              patch('mission_flow.capture_streaming_goals', side_effect=[empty, active, empty]),
              patch('mission_flow.choose_choice_goal', side_effect=choose),
              patch('mission_flow.touch_count', return_value=1),
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 100, explore=True, navigation=True, interaction_goals=True)
        import json
        rows = [json.loads(line) for line in (self.report.directory / 'events.jsonl').read_text().splitlines()]
        routes = [row for row in rows if row['kind'] == 'navigation_route_selected']
        self.assertEqual(routes[0]['goal'], 'end')
        self.assertEqual(routes[-1]['goal'], 'end')
        self.assertEqual(self.report.counts['exploration_destination_selected'], 1)
        self.assertEqual(self.report.counts['touch_goal_activated'], 1)
        self.observer.pulse.assert_not_called()

    def test_same_world_reposition_discards_the_old_touch_before_count_read(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []

        def state():
            calls.append(1)
            if len(calls) == 2:
                (self.report.directory / 'stop').touch()
            return sample(len(calls), position=(4000, 0, 0) if len(calls) == 2 else (0, 0, 0))

        self.observer.sample.side_effect = state
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(0))
        old_goal = dict(position=[0, 0, 0], event=dict(address=20), arrival_radius=20, goal_kind='touch')
        new_goal = dict(position=[4000, 0, 0], event=dict(address=30), arrival_radius=20, goal_kind='touch')
        flow = dict(rows=[], end_sample=sample(0))
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        explorer.next.return_value = None
        with (patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(0))),
              patch('mission_hints.choose_goal', side_effect=[old_goal, new_goal]) as choose,
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', return_value=None),
              patch('mission_flow.touch_count') as count,
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        count.assert_not_called()
        self.assertEqual(choose.call_count, 2)
        self.assertEqual(self.report.counts['control_position_discontinuity'], 1)
        self.observer.pulse.assert_not_called()

    def test_prerequisite_door_does_not_skip_a_pending_earlier_checkpoint_goal(self):
        self.observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
        calls = []
        clock = [0]

        def state():
            calls.append(1)
            if len(calls) == 3:
                clock[0] = 11
                (self.report.directory / 'stop').touch()
            return sample(len(calls), position=((len(calls) - 1) * 650, 0, 0))

        self.observer.sample.side_effect = state
        graph = dict(nodes={}, edges=[], truncated=False, source_sample=sample(0))
        door = dict(position=[100, 0, 0], event=dict(address=10), arrival_radius=100, forced_hints_required=False)
        checkpoint = dict(position=[300, 0, 0], event=dict(address=20), arrival_radius=20,
                          goal_kind='touch', forced_hints_required=True)
        flow = dict(rows=[], end_sample=sample(0), errors=[], next_cursor=dict(operation=dict(address=30)))
        explorer = Mock(waypoints=[], turn_rate=-120, route_complete=False, heading=0)
        explorer.next.return_value = None
        with (patch('run.time.monotonic', side_effect=lambda: clock[0]),
              patch('navigation.capture', return_value=graph),
              patch('mission_hints.capture', return_value=dict(source_sample=sample(0))),
              patch('mission_hints.choose_goal', return_value=door),
              patch('mission_flow.capture_choice_goals', return_value=flow),
              patch('mission_flow.choose_choice_goal', side_effect=[None, checkpoint, None, checkpoint]),
              patch('mission_flow.capture_checkpoint_goals', return_value=flow) as capture,
              patch('mission_flow.capture_streaming_goals', return_value=dict(flow, next_cursor=None)),
              patch('run.Explorer', return_value=explorer)):
            observe(self.report, self.observer, 3, explore=True, navigation=True, interaction_goals=True)
        self.assertEqual([call.kwargs['resume_cursor'] for call in capture.call_args_list], [None, None])
        self.assertEqual(self.report.counts['checkpoint_flow_waiting'], 0)
        self.observer.pulse.assert_not_called()


if __name__ == '__main__':
    unittest.main()
