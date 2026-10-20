"""Pure offline control-policy tests; no reader/process/input is imported."""
import copy
import unittest
import sys
from types import SimpleNamespace
from unittest import mock
from menu_control import (control, allowed, fingerprint, ONLINE_TITLE, ONLINE_MESSAGE,
                          wait_for_reader, wait_for_replacement, REGISTRY_IDENTITY_CHANGED, ControlProgress)


class DispatchInstallationTests(unittest.TestCase):
    def test_selected_window_dispatch_and_legacy_fallback(self):
        from menu_control import send_confirm
        post=mock.Mock(return_value=True);fixed=mock.Mock(side_effect=RuntimeError('Game window not found'))
        stopped=[False]
        module=SimpleNamespace(game_window=fixed,interrupted=lambda:stopped[0],user=SimpleNamespace(PostMessageW=post))
        with mock.patch.dict(sys.modules,{'game_io':module}),mock.patch('menu_control.time.sleep'):
            send_confirm('confirm',(123,456),window_lookup=lambda:(123,456))
            self.assertEqual([call.args[1] for call in post.call_args_list],[0x100,0x101])
            self.assertTrue(all(call.args[0]==456 for call in post.call_args_list));fixed.assert_not_called()
            post.reset_mock()
            for target in ((123,999),(999,456)):
                with self.assertRaisesRegex(RuntimeError,'target changed'):send_confirm('confirm',(123,456),window_lookup=lambda:target)
            stopped[0]=True
            with self.assertRaisesRegex(RuntimeError,'Stop/F8'):send_confirm('confirm',(123,456),window_lookup=lambda:(123,456))
            stopped[0]=False;post.assert_not_called()
            with self.assertRaisesRegex(RuntimeError,'Game window not found'):send_confirm('confirm',(123,456))
            fixed.assert_called_once()


def state(continuing=False):
    top = dict(tag='DeltaCampaignMenu_ContinueGame' if continuing else 'DeltaCampaignType',
               cls='WarUIScene_Campaign' if continuing else 'WarUIScene_CampaignType', address='0x100')
    chain = [dict(name='scene', cls=top['cls'], address='0x100',
                  states=['UIState_Enabled', 'UIState_Focused']),
             dict(name='butContinue' if continuing else 'butSinglePlayer', cls='UILabelButton',
                  address='0x200', states=['UIState_Enabled', 'UIState_Focused'])]
    return dict(pid=123, top=top, focus_chain=chain, focused_leaf=chain[-1],
                consistent_reread=True, player_index=0, input_block_count=0)


def title_state():
    current = state()
    top = dict(name='AttractLauncher', tag='AttractLauncher', cls='UIScene_AttractLauncher', address='0x300')
    leaf = dict(name=top['name'], cls=top['cls'], address=top['address'], states=['UIState_Enabled'])
    current.update(top=top, focus_chain=[leaf], focused_leaf=leaf,
                   active_scenes=[dict(name='UIScene', cls='UIScene', input_mode=0),
                                  dict(name='DeltaWarTitleScreen', cls='WarUIScene_TitleScreen', input_mode=4),
                                  dict(name='AttractLauncher', cls='UIScene_AttractLauncher', input_mode=0)])
    return current


def online_state():
    current = state()
    current['top'].update(tag='WarfareConfirmation', cls='UIScene_Confirmation')
    current['focus_chain'][-1].update(name='OkButton', cls='WarLabelButtonOnlyClickable')
    current['dialog_text'] = dict(Title=ONLINE_TITLE, MessageText=ONLINE_MESSAGE)
    return current


class ControllerTests(unittest.TestCase):
    def run_control(self, read, send=None, **kwargs):
        clock, events, actions = [0.0], [], []
        def dispatch(action, target):
            actions.append((clock[0], action, target))
            if send:
                send(action, target)
        def sleep(dt):
            clock[0] += dt
        result = control(read, dispatch, kwargs.pop('window', lambda: (123, 456)),
                         kwargs.pop('interrupted', lambda: False), pid=123,
                         now=lambda: clock[0], sleep=sleep, emit=events.append,
                         seconds=kwargs.pop('seconds', 2), **kwargs)
        return result, events, actions

    def test_two_recognized_steps(self):
        current = [state()]
        result, events, actions = self.run_control(lambda: copy.deepcopy(current[0]),
                                                  lambda *_: current.__setitem__(0, state(True)))
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertFalse(result['level_ready'])
        self.assertEqual(len(actions), 2)
        self.assertGreaterEqual(actions[0][0], .6)
        self.assertGreaterEqual(actions[1][0] - actions[0][0], .6)

    def test_shared_progress_preserves_total_action_limit(self):
        progress = ControlProgress(actions=4)
        result, _, actions = self.run_control(state, progress=progress)
        self.assertEqual(result['reason'], 'max_actions')
        self.assertEqual(len(actions), 1)
        result, _, actions = self.run_control(lambda: state(True), progress=progress)
        self.assertEqual(result['reason'], 'max_actions')
        self.assertEqual(result['actions'], 5)
        self.assertFalse(actions)

    def test_partial_dispatch_stays_reserved_in_shared_progress(self):
        progress = ControlProgress()
        def partial_send(*_):
            raise RuntimeError('key release failed')
        result, _, actions = self.run_control(state, partial_send, progress=progress)
        self.assertEqual(result['reason'], 'action_error')
        self.assertEqual(len(actions), 1)
        progress.window_lost()
        result, _, actions = self.run_control(state, progress=progress)
        self.assertEqual(result['reason'], 'timeout')
        self.assertFalse(actions)

    def test_successful_pair_retry_requires_window_loss_and_is_bounded(self):
        progress = ControlProgress()
        self.assertEqual(len(self.run_control(state, progress=progress)[2]), 1)
        self.assertFalse(self.run_control(state, progress=progress)[2])
        progress.window_lost()
        self.assertEqual(len(self.run_control(state, progress=progress)[2]), 1)
        progress.window_lost()
        self.assertFalse(self.run_control(state, progress=progress)[2])
        self.assertEqual(progress.retries, 1)

    def test_continue_is_never_replayed_after_window_loss(self):
        progress = ControlProgress()
        self.assertEqual(self.run_control(lambda: state(True), progress=progress)[0]['reason'], 'loading_requested')
        progress.window_lost()
        self.assertFalse(self.run_control(lambda: state(True), progress=progress)[2])

    def test_retry_budget_does_not_consume_remaining_five_step_budget(self):
        progress = ControlProgress(actions=3)
        self.run_control(state, progress=progress)
        progress.window_lost()
        self.assertEqual(len(self.run_control(state, progress=progress)[2]), 1)
        result, _, actions = self.run_control(lambda: state(True), progress=progress)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(result['actions'], 6)
        self.assertEqual(len(actions), 1)

    def test_gates_and_unknown_wait_without_input(self):
        for field, value in [('input_block_count', 1), ('player_index', 1),
                             ('consistent_reread', False), ('pid', 999)]:
            with self.subTest(field=field):
                current = state()
                current[field] = value
                result, events, actions = self.run_control(lambda: current)
                self.assertEqual(result['reason'], 'timeout')
                self.assertFalse(actions)
                self.assertEqual(sum(x['event'] == 'state' for x in events), 1)
        current = state()
        current['top']['tag'] = 'NewGame'
        self.assertFalse(self.run_control(lambda: current)[2])

    def test_focus_gates_every_link(self):
        for states in (['UIState_Enabled'], ['UIState_Focused'],
                       ['UIState_Enabled', 'UIState_Focused', 'UIState_Disabled']):
            current = state()
            current['focus_chain'][0]['states'] = states
            self.assertFalse(self.run_control(lambda: current)[2])

    def test_immediate_reread_change_withholds(self):
        count = [0]
        def read():
            count[0] += 1
            current = state()
            if count[0] >= 6:  # first immediate recheck after .6s
                current['top']['tag'] = 'Unreviewed'
            return current
        result, events, actions = self.run_control(read)
        self.assertFalse(actions)
        self.assertTrue(any(e['event'] == 'action_withheld' for e in events))

    def test_duplicate_suppression_even_after_unknown_and_address_change(self):
        count = [0]
        def read():
            count[0] += 1
            current = state()
            if 8 <= count[0] <= 10:
                current['top']['tag'] = 'Unknown'
            if count[0] > 10:
                current['top']['address'] = '0x999'
            return current
        result, events, actions = self.run_control(read, seconds=4)
        self.assertEqual(len(actions), 1)
        self.assertEqual(result['reason'], 'timeout')

    def test_timeout_before_stability(self):
        self.assertFalse(self.run_control(state, seconds=.5)[2])

    def test_preaction_read_error_resets_stability(self):
        calls = [0]
        def read():
            calls[0] += 1
            if calls[0] == 6:
                raise RuntimeError('snapshot changed')
            return state()
        result, _, actions = self.run_control(read)
        self.assertEqual(len(actions), 1)
        self.assertGreaterEqual(actions[0][0], 1.35)

    def test_read_error_is_bounded_and_not_spammed(self):
        def read():
            raise RuntimeError('changed')
        result, events, actions = self.run_control(read)
        self.assertEqual(result['reason'], 'timeout')
        self.assertEqual(sum(e['event'] == 'read_error' for e in events), 1)
        self.assertFalse(actions)

    def test_send_error_never_retried(self):
        def send(*_):
            raise RuntimeError('partial send')
        result, _, actions = self.run_control(state, send)
        self.assertEqual(result['reason'], 'action_error')
        self.assertEqual(len(actions), 1)

    def test_interrupt_and_window_identity(self):
        self.assertEqual(self.run_control(state, interrupted=lambda: True)[0]['reason'], 'interrupted')
        calls = [0]
        def window():
            calls[0] += 1
            return (123, 456 if calls[0] == 1 else 999)
        self.assertEqual(self.run_control(state, window=window)[0]['reason'], 'window_changed')

    def test_max_actions(self):
        result, _, actions = self.run_control(state, max_actions=1)
        self.assertEqual(result['reason'], 'max_actions')
        self.assertEqual(len(actions), 1)

    def test_exact_title_and_online_are_dispatched(self):
        for factory in (title_state, online_state):
            with self.subTest(rule=factory.__name__):
                self.assertTrue(allowed(factory(), 123))
                self.assertEqual(len(self.run_control(factory)[2]), 1)

    def test_new_rules_keep_common_safety_gates(self):
        for factory in (title_state, online_state):
            for field, value in (('player_index', 1), ('input_block_count', 1),
                                 ('consistent_reread', False)):
                current = factory()
                current[field] = value
                self.assertFalse(allowed(current, 123))

    def test_exact_main_campaign_rule_and_wrong_button_class(self):
        current = state()
        current['top'].update(tag='DeltaMainMenu', cls='WarUIScene_MainMenu')
        current['focused_leaf'].update(name='CampaignButton', cls='UIButtonIgnoreStart')
        self.assertTrue(allowed(current, 123))
        self.assertEqual(len(self.run_control(lambda: current)[2]), 1)
        for field, value in (('name', 'VersusButton'), ('cls', 'UILabelButton')):
            wrong = copy.deepcopy(current)
            wrong['focused_leaf'][field] = value
            self.assertFalse(allowed(wrong, 123))

    def test_title_requires_exact_stack_order_modes_and_length(self):
        variants = []
        current = title_state()
        current['active_scenes'].reverse()
        variants.append(current)
        current = title_state()
        current['active_scenes'][1]['input_mode'] = 0
        variants.append(current)
        current = title_state()
        current['active_scenes'].append(dict(name='Popup', cls='UIScene', input_mode=0))
        variants.append(current)
        current = title_state()
        current['active_scenes'][0]['name'] = 'OtherScene'
        variants.append(current)
        for current in variants:
            self.assertFalse(allowed(current, 123))

    def test_title_focus_exception_does_not_generalize(self):
        current = state()
        current['focus_chain'][-1]['states'] = ['UIState_Enabled']
        self.assertFalse(allowed(current, 123))
        for change in ('disabled', 'extra_link', 'wrong_owner_address', 'wrong_top_tag'):
            current = title_state()
            if change == 'disabled':
                current['focus_chain'][0]['states'].append('UIState_Disabled')
            elif change == 'extra_link':
                current['focus_chain'].insert(0, copy.deepcopy(current['focused_leaf']))
            elif change == 'wrong_owner_address':
                current['top']['address'] = '0x999'
            else:
                current['top']['tag'] = 'Other'
            self.assertFalse(allowed(current, 123))

    def test_online_text_exact_and_missing_rejected(self):
        for field in ('Title', 'MessageText'):
            for mutation in ('missing', 'suffix', 'case', 'newline'):
                current = online_state()
                if mutation == 'missing':
                    del current['dialog_text'][field]
                elif mutation == 'suffix':
                    current['dialog_text'][field] += ' '
                elif mutation == 'case':
                    current['dialog_text'][field] = current['dialog_text'][field].lower()
                else:
                    current['dialog_text'][field] += '\n'
                self.assertFalse(allowed(current, 123))
        current = online_state()
        del current['dialog_text']
        self.assertFalse(allowed(current, 123))
        current = online_state()
        current['focused_leaf']['cls'] = 'UILabelButton'
        self.assertFalse(allowed(current, 123))

    def test_online_dialog_part_of_fingerprint_and_immediate_check(self):
        original = online_state()
        changed = copy.deepcopy(original)
        changed['dialog_text']['Title'] = 'Delete Save'
        self.assertNotEqual(fingerprint(original), fingerprint(changed))
        calls = [0]
        def read():
            calls[0] += 1
            return original if calls[0] < 6 else changed
        _, events, actions = self.run_control(read)
        self.assertFalse(actions)
        self.assertTrue(any(e['event'] == 'action_withheld' for e in events))


class InitializationTests(unittest.TestCase):
    def setUp(self):
        self.clock, self.calls, self.closed, self.events = 0.0, 0, 0, []
        self.target = (123, 456)

    def sleep(self, dt):
        self.clock += dt

    def close(self):
        self.closed += 1

    def wait(self, factory, window=None, interrupted=None, deadline=1):
        return wait_for_reader(factory, window or (lambda: self.target), interrupted or (lambda: False),
                               target=self.target, deadline=deadline, now=lambda: self.clock,
                               sleep=self.sleep, emit=self.events.append)

    def test_early_not_ready_then_shared_deadline_and_original_target(self):
        def factory():
            self.calls += 1
            if self.calls <= 2:
                raise RuntimeError('Expected one registered GameUISceneClient; found 0')
            return self
        reader, reason = self.wait(factory, deadline=.9)
        self.assertIs(reader, self)
        self.assertEqual(reason, 'ready')
        self.assertEqual(self.clock, .5)
        sends = []
        result = control(state, lambda *_: sends.append(True), lambda: self.target, lambda: False,
                         pid=123, target=self.target, deadline=.9, now=lambda: self.clock, sleep=self.sleep)
        self.assertEqual(result['reason'], 'timeout')
        self.assertFalse(sends)  # Remaining .4 sec cannot satisfy .6 sec stability.

    def test_known_errors_timeout_bounded_and_logged_once(self):
        def factory():
            self.calls += 1
            raise RuntimeError('Invalid name array header')
        reader, reason = self.wait(factory, deadline=.4)
        self.assertIsNone(reader)
        self.assertEqual(reason, 'timeout')
        self.assertEqual(self.calls, 2)
        self.assertEqual(self.clock, .4)
        self.assertEqual(len(self.events), 1)

    def test_unknown_or_hash_error_is_not_retried(self):
        for message in ('Executable hash differs from validated build; re-discover offsets',
                        'Expected one registered GameUISceneClient; found 2', 'Unreadable address'):
            def factory():
                raise RuntimeError(message)
            with self.assertRaisesRegex(RuntimeError, message):
                self.wait(factory)
            self.assertEqual(self.clock, 0)

    def test_stop_or_window_change_before_constructor(self):
        def factory():
            self.fail('Constructor must not run')
        self.assertEqual(self.wait(factory, interrupted=lambda: True)[1], 'interrupted')
        self.assertEqual(self.wait(factory, window=lambda: (123, 999))[1], 'window_changed')
        def exited():
            raise RuntimeError('No game window')
        with self.assertRaisesRegex(RuntimeError, 'No game window'):
            self.wait(factory, window=exited)

    def test_ready_reader_closed_on_late_or_replaced_window(self):
        def slow_factory():
            self.clock = 1
            return self
        self.assertEqual(self.wait(slow_factory)[1], 'timeout')
        self.assertEqual(self.closed, 1)
        self.clock = 0
        calls = [0]
        def window():
            calls[0] += 1
            return self.target if calls[0] == 1 else (123, 999)
        self.assertEqual(self.wait(lambda: self, window=window)[1], 'window_changed')
        self.assertEqual(self.closed, 2)


class ReaderRecoveryTests(unittest.TestCase):
    class Reader:
        def __init__(self, value, identity=(123, 1000)):
            self.value, self.identity, self.closed = value, identity, False
            self.stale = False

        def sample(self):
            if self.stale:
                raise RuntimeError(REGISTRY_IDENTITY_CHANGED)
            return copy.deepcopy(self.value)

        def close(self):
            self.closed = True

    def test_relocation_recovers_after_sent_action_and_restarts_stability(self):
        original, replacement = self.Reader(state()), self.Reader(state(True))
        current, recoveries = [original], []
        def send(*_):
            current[0].stale = True
        def recover():
            fresh, reason = wait_for_replacement(
                lambda: replacement, lambda reader: reader.identity,
                lambda: (123, 456), lambda: False, expected_identity=original.identity,
                target=(123, 456), deadline=10, now=lambda: 0)
            current[0].close()
            current[0] = fresh
            recoveries.append(reason)
            return reason
        result, _, actions = ControllerTests().run_control(
            lambda: current[0].sample(), send, recover_reader=recover)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(recoveries, ['ready'])
        self.assertTrue(original.closed)
        self.assertEqual(len(actions), 2)
        self.assertGreaterEqual(actions[1][0] - actions[0][0], .9)

    def test_relocation_does_not_retry_already_sent_choice(self):
        original, replacement = self.Reader(state()), self.Reader(state())
        current = [original]
        def recover():
            original.close()
            current[0] = replacement
            return 'ready'
        result, _, actions = ControllerTests().run_control(
            lambda: current[0].sample(), lambda *_: setattr(original, 'stale', True),
            recover_reader=recover)
        self.assertEqual(result['reason'], 'timeout')
        self.assertEqual(len(actions), 1)

    def test_preaction_relocation_requires_new_stability_period(self):
        calls, recoveries = [0], []
        def read():
            calls[0] += 1
            if calls[0] == 6:
                raise RuntimeError(REGISTRY_IDENTITY_CHANGED)
            return state()
        result, _, actions = ControllerTests().run_control(
            read, recover_reader=lambda: recoveries.append(True) or 'ready')
        self.assertEqual(len(recoveries), 1)
        self.assertEqual(len(actions), 1)
        self.assertGreaterEqual(actions[0][0], 1.35)

    def test_same_pid_new_creation_rejected_and_candidate_closed(self):
        replacement = self.Reader(state(True), identity=(123, 1001))
        def recover():
            return wait_for_replacement(
                lambda: replacement, lambda reader: reader.identity,
                lambda: (123, 456), lambda: False, expected_identity=(123, 1000),
                target=(123, 456), deadline=10, now=lambda: 0)[1]
        def read():
            raise RuntimeError(REGISTRY_IDENTITY_CHANGED)
        with self.assertRaisesRegex(RuntimeError, 'Game process creation identity changed'):
            ControllerTests().run_control(read, recover_reader=recover)
        self.assertTrue(replacement.closed)

    def test_unknown_sample_error_does_not_reacquire(self):
        def read():
            raise RuntimeError('UI interaction backlink mismatch')
        def recover():
            self.fail('Only the exact registry identity error may reacquire')
        result, _, actions = ControllerTests().run_control(read, recover_reader=recover)
        self.assertEqual(result['reason'], 'timeout')
        self.assertFalse(actions)

    def test_replacement_failure_is_not_swallowed_by_polling(self):
        def read():
            raise RuntimeError(REGISTRY_IDENTITY_CHANGED)
        def recover():
            def factory():
                raise RuntimeError('Executable hash differs from validated build; re-discover offsets')
            return wait_for_replacement(
                factory, lambda reader: reader.identity,
                lambda: (123, 456), lambda: False, expected_identity=(123, 1000),
                target=(123, 456), deadline=10, now=lambda: 0)[1]
        with self.assertRaisesRegex(RuntimeError, 'Executable hash differs'):
            ControllerTests().run_control(read, recover_reader=recover)

    def test_recovery_focus_stop_and_deadline_keep_original_guards(self):
        for reason in ('interrupted', 'window_changed', 'timeout'):
            with self.subTest(reason=reason):
                def factory():
                    self.fail('Stopped recovery must not construct another reader')
                def recover():
                    return wait_for_replacement(
                        factory, lambda reader: reader.identity,
                        lambda: (123, 999) if reason == 'window_changed' else (123, 456),
                        lambda: reason == 'interrupted', expected_identity=(123, 1000),
                        target=(123, 456), deadline=0 if reason == 'timeout' else 10,
                        now=lambda: 0)[1]
                def read():
                    raise RuntimeError(REGISTRY_IDENTITY_CHANGED)
                result, _, actions = ControllerTests().run_control(read, recover_reader=recover)
                self.assertEqual(result['reason'], reason)
                self.assertFalse(actions)


if __name__ == '__main__':
    unittest.main()
