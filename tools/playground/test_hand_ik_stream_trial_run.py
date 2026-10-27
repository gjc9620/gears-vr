import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import hand_animation_probe_run as call_probe
import hand_ik_stream_trial_run as stream


MATRIX = ','.join('1' if index in (0, 5, 10, 15) else '0' for index in range(16))
TARGET = struct.pack('<fff', 1.25, -2.5, 3.75).hex()
ROTATOR = struct.pack('<iii', -65535, 65536, -32768).hex()


def row(prefix, values):
    return prefix + ' ' + ' '.join(f'{key}={value}' for key, value in values.items())


def recording():
    lines = []
    for lease in range(1, 33):
        for phase in stream.PHASES:
            values = {'phase': phase, 'lease': lease, 'segment': (lease - 1) // 8,
                      'trial_apply_mask': 0 if phase == 'before' else 3,
                      'mesh_tick_known': 1, 'mesh_tick': 100 + lease,
                      'sequence': 1000 + lease, 'left': MATRIX, 'right': MATRIX,
                      'mesh': MATRIX, 'qpc': 100000 + lease, 'ms': 1000 + lease,
                      'action_epoch': 0 if lease <= 16 else 1, 'ownership_proved': 0}
            if phase == 'pre_restore' and lease != 16:
                values['mesh_tick'] += 1
            if phase == 'written':
                values.update({f'target{index}': TARGET if index % 2 == 0 else ROTATOR
                               for index in range(4)})
            lines.append(row('hand_ik_stream_trial_sample', values))
        lines.append(row('hand_ik_stream_trial_restore', {
            'lease': lease, 'dirty_before': 1, 'dirty_after': 0, 'restored': 1,
            'reason': 'action_boundary' if lease == 16 else 'pre_tick',
        }))
        if lease == 16:
            lines.append(row('hand_ik_stream_trial_action_dispatch_test', stream.ACTION_EXPECTED))
    lines.append(row('hand_ik_stream_trial_end', stream.END_EXPECTED))
    return lines


def mutate(lines, prefix, *, occurrence=0, **changes):
    result = list(lines)
    positions = [index for index, line in enumerate(result) if line.startswith(prefix + ' ')]
    position = positions[occurrence]
    values = call_probe.fields_of(result[position], 'test fixture')
    for key, value in changes.items():
        if value is None:
            values.pop(key)
        else:
            values[key] = value
    result[position] = row(prefix, values)
    return result


class HandIkStreamRecordingTests(unittest.TestCase):
    def test_complete_synthetic_recording_and_immediate_same_tick_restore(self):
        result = stream.validate_log('\n'.join(recording()))
        self.assertEqual(result['terminal'], stream.END_EXPECTED)
        self.assertEqual(result['sample_count'], 96)
        self.assertEqual(result['restore_count'], 32)
        self.assertEqual(result['leases'][15]['before_mesh_tick'],
                         result['leases'][15]['pre_restore_mesh_tick'])
        self.assertEqual(result['leases'][-1]['segment'], 3)
        self.assertEqual(result['terminal']['ownership_proved'], 0)

    def test_terminal_fields_and_every_prescribed_value_are_strict(self):
        for name, expected in stream.END_EXPECTED.items():
            changed = 'different_reason' if isinstance(expected, str) else expected + 1
            with self.subTest(field=name), self.assertRaises(RuntimeError):
                stream.parse_end('\n'.join(mutate(recording(), 'hand_ik_stream_trial_end',
                                                  **{name: changed})))
        for changes in ({'leases': None}, {'unexpected': 0}):
            with self.subTest(changes=changes), self.assertRaisesRegex(RuntimeError, 'schema'):
                stream.parse_end('\n'.join(mutate(recording(), 'hand_ik_stream_trial_end', **changes)))
        lines = recording()
        for malformed in (lines[:-1], [*lines, lines[-1]], [*lines[:-1], lines[-1] + ' dirty=0']):
            with self.subTest(terminal=malformed[-1]), self.assertRaises(RuntimeError):
                stream.parse_end('\n'.join(malformed))

    def test_exactly_96_ordered_sample_phases(self):
        lines = recording()
        reordered = list(lines)
        reordered[0], reordered[1] = reordered[1], reordered[0]
        for changed in (lines[1:], [lines[0], *lines], reordered):
            with self.subTest(count=len(changed)), self.assertRaisesRegex(RuntimeError, '96 ordered'):
                stream.validate_log('\n'.join(changed))

    def test_original_melee_and_related_slot_play_must_both_be_observed(self):
        for name in ('original_melee_seen', 'original_slot_play_seen'):
            for missing_or_false in (None, 0):
                with self.subTest(field=name, value=missing_or_false), self.assertRaises(RuntimeError):
                    stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_end',
                                                          **{name: missing_or_false})))

    def test_sample_required_fields_phase_segments_and_masks(self):
        cases = [(0, {'mesh': None}), (0, {'segment': 1}), (0, {'lease': 0}),
                 (0, {'mesh_tick_known': 0}), (0, {'trial_apply_mask': 3}),
                 (1, {'trial_apply_mask': 0}), (2, {'trial_apply_mask': 0}),
                 (1, {'phase': 'after'}), (1, {'sequence': 0}),
                 (0, {'ownership_proved': 1})]
        for occurrence, changes in cases:
            with self.subTest(changes=changes), self.assertRaises(RuntimeError):
                stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_sample',
                                                      occurrence=occurrence, **changes)))

    def test_complete_finite_matrices_are_required_for_both_hands_and_mesh(self):
        for name in ('left', 'right', 'mesh'):
            for value in (','.join(['0'] * 15), MATRIX.replace('1', 'nan', 1),
                          MATRIX.replace('1', 'inf', 1), 'bad'):
                with self.subTest(name=name, value=value), self.assertRaisesRegex(RuntimeError, 'matrix'):
                    stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_sample',
                                                          **{name: value})))

    def test_written_targets_are_all_exact_12_byte_opaque_payloads(self):
        for name in ('target0', 'target1', 'target2', 'target3'):
            for value in (None, '00' * 11, '00' * 13, 'zz' * 12):
                with self.subTest(name=name, value=value), self.assertRaises(RuntimeError):
                    stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_sample',
                                                          occurrence=1, **{name: value})))

    def test_sequence_is_fixed_within_each_lease_and_increases_between_leases(self):
        for occurrence, changes in ((1, {'sequence': 1002}),
                                    (3, {'sequence': 1001}),
                                    (1, {'mesh_tick': 999})):
            with self.subTest(changes=changes), self.assertRaisesRegex(RuntimeError, 'sequence or before/written'):
                stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_sample',
                                                      occurrence=occurrence, **changes)))

    def test_no_matrix_change_or_next_tick_effect_is_required(self):
        lines = recording()
        for occurrence in range(2, 96, 3):
            lines = mutate(lines, 'hand_ik_stream_trial_sample', occurrence=occurrence,
                           mesh_tick=101 + occurrence // 3)
        stream.validate_log('\n'.join(lines))

    def test_all_exact_lease_restorations_and_only_lease16_action_boundary(self):
        for occurrence, changes in ((0, {'dirty_after': 1}), (0, {'restored': 0}),
                                    (0, {'lease': 2}), (0, {'reason': 'action_boundary'}),
                                    (15, {'reason': 'pre_tick'}), (0, {'extra': 0})):
            with self.subTest(changes=changes), self.assertRaises(RuntimeError):
                stream.validate_log('\n'.join(mutate(recording(), 'hand_ik_stream_trial_restore',
                                                      occurrence=occurrence, **changes)))
        lines = recording()
        restore = next(line for line in lines if line.startswith('hand_ik_stream_trial_restore '))
        for changed in ([line for line in lines if line != restore], [*lines, restore]):
            with self.subTest(count=len(changed)), self.assertRaisesRegex(RuntimeError, '32 hand IK stream restoration'):
                stream.validate_log('\n'.join(changed))

    def test_restore_cannot_precede_pre_restore_sample_or_follow_next_lease(self):
        lines = recording()
        early = list(lines)
        early[2], early[3] = early[3], early[2]
        late = list(lines)
        late[3], late[4] = late[4], late[3]
        for changed in (early, late):
            with self.assertRaisesRegex(RuntimeError, 'outside its recorded lease'):
                stream.validate_log('\n'.join(changed))

    def test_immediate_action_must_yield_before_original_dispatch_once(self):
        lines = recording()
        action = next(line for line in lines if line.startswith('hand_ik_stream_trial_action_dispatch_test '))
        for changed in ([line for line in lines if line != action], [*lines, action],
                        mutate(lines, 'hand_ik_stream_trial_action_dispatch_test', invoked=0),
                        mutate(lines, 'hand_ik_stream_trial_action_dispatch_test', dirty_after=1)):
            with self.assertRaises(RuntimeError):
                stream.validate_log('\n'.join(changed))
        action_index = lines.index(action)
        for offset in (-1, 1):
            changed = list(lines)
            changed[action_index], changed[action_index + offset] = (
                changed[action_index + offset], changed[action_index])
            with self.assertRaisesRegex(RuntimeError, 'lease16 restoration before resume'):
                stream.validate_log('\n'.join(changed))

    def test_terminal_must_follow_complete_protocol_evidence(self):
        lines = recording()
        lines.insert(0, lines.pop())
        with self.assertRaisesRegex(RuntimeError, 'terminal preceded'):
            stream.validate_log('\n'.join(lines))


class FakeBridge:
    def __init__(self, phases=(5,), *, bad_call_entry=False, bad_process_entry=False):
        self.phases = list(phases)
        self.calls = []
        self.bad_call_entry = bad_call_entry
        self.bad_process_entry = bad_process_entry
        self.r = SimpleNamespace(base=0x140000000, read=self.read)

    def read(self, address, size):
        self.calls.append(('read', address, size))
        if address == self.r.base + call_probe.CALL_FUNCTION_RVA:
            return b'\xe9' * 32 if self.bad_call_entry else call_probe.CALL_FUNCTION_ENTRY
        if address == self.r.base + call_probe.PROCESS_EVENT_RVA:
            return b'\xe9' * 64 if self.bad_process_entry else call_probe.PROCESS_EVENT_ENTRY
        raise AssertionError('Unexpected live read')

    def export(self, name):
        self.calls.append(name)
        if name == stream.STATUS_EXPORT:
            phase = self.phases[0]
            if len(self.phases) > 1:
                self.phases.pop(0)
            return phase | (32 << 16)
        if name == stream.STOP_EXPORT and self.phases[0] == 8:
            return 40
        return 0

    def verify(self):
        self.calls.append('verify')

    def close(self):
        self.calls.append('close')


class HandIkStreamLifecycleTests(unittest.TestCase):
    def test_lancer_preparation_uses_original_owned_pickup_inventory_and_equip(self):
        pawn, pc, inventory, world, pickup, weapon_class, weapon = [0x1000 * value for value in range(1, 8)]
        offsets = {name: 8 * (index + 1) for index, name in enumerate((
            'Controller', 'InvManager', 'WorldInfo', 'InventoryClass',
            'InventoryChain', 'Inventory', 'Weapon'))}
        memory = {pawn + offsets['Controller']: pc, pawn + offsets['InvManager']: inventory,
                  pc + offsets['WorldInfo']: world, pickup + offsets['WorldInfo']: world,
                  pickup + offsets['InventoryClass']: weapon_class,
                  inventory + offsets['InventoryChain']: 0, pawn + offsets['Weapon']: 0}
        identities = {
            obj: {'cls': 0x9000, 'name': name, 'kind': kind}
            for obj, name, kind in ((pawn, 'Pawn', 'WarPawn'), (pc, 'Controller', 'WarPC'),
                                    (inventory, 'Inventory', 'WarInventoryManager'),
                                    (pickup, 'Pickup', 'WarDroppedPickup'),
                                    (weapon_class, 'Weap_AssaultRifle', 'Class'),
                                    (weapon, 'Lancer', 'Weap_AssaultRifle'))}
        resolver = SimpleNamespace(prepare=mock.Mock(), object_pointers=[0, pickup],
                                   identity=lambda obj: identities[obj],
                                   property=lambda owner, name, inherited: {'offset': offsets[name]},
                                   u64=lambda address: memory.get(address, 0),
                                   class_name=lambda value: value['kind'])

        def original_call(obj, name, **values):
            if name == 'GiveTo':
                self.assertEqual((obj, values), (pickup, {'P': pawn}))
                memory[inventory + offsets['InventoryChain']] = weapon
            elif name == 'SetCurrentWeapon':
                self.assertEqual((obj, values), (inventory, {'DesiredWeapon': weapon}))
                memory[pawn + offsets['Weapon']] = weapon
            elif name == 'ShouldLeftHandIKBeOn':
                self.assertEqual(obj, pawn)
                return True
            else:
                raise AssertionError('Unexpected game method')

        bridge = SimpleNamespace(r=resolver, call=mock.Mock(side_effect=original_call))
        receipt = {}
        stream.prepare_owned_lancer(bridge, {'pawn': hex(pawn)}, receipt)
        self.assertTrue(receipt['original_lancer_prepared'])
        self.assertEqual([call.args[1] for call in bridge.call.call_args_list],
                         ['GiveTo', 'SetCurrentWeapon', 'ShouldLeftHandIKBeOn'])
        resolver.prepare.assert_called_once_with()

    def test_cleanup_cancels_active_trial_before_shutdown_and_live_byte_checks(self):
        bridge, receipt = FakeBridge((3, 6)), {'trial_armed': True}
        stream.cleanup_trial_bridge(bridge, receipt)
        self.assertEqual(receipt['cleanup_status']['phase'], 6)
        self.assertEqual(receipt['trial_stop_result'], 0)
        self.assertLess(bridge.calls.index(stream.STOP_EXPORT), bridge.calls.index('PlaygroundShutdown'))
        self.assertLess(bridge.calls.index('PlaygroundShutdown'),
                        bridge.calls.index(('read', 0x1401d6f30, 32)))
        self.assertTrue(receipt['call_function_entry']['after_shutdown']['matched'])
        self.assertTrue(receipt['process_event_entry']['after_shutdown']['matched'])
        self.assertEqual(bridge.calls[-1], 'close')

    def test_unsafe_phase_keeps_hooks_until_external_owner_closes_game(self):
        bridge, receipt = FakeBridge((8,)), {'trial_armed': True}
        stream.cleanup_trial_bridge(bridge, receipt)
        self.assertEqual(receipt['trial_stop_result'], 40)
        self.assertIn('cleanup_error', receipt)
        self.assertEqual(receipt['shutdown_skipped'], 'stream_journal_not_confirmed_clean')
        self.assertNotIn('PlaygroundShutdown', bridge.calls)
        self.assertEqual(bridge.calls[-1], 'close')

    def test_stop_timeout_never_removes_hooks(self):
        bridge, receipt = FakeBridge((3,)), {'trial_armed': True}
        with mock.patch.object(stream, 'wait_for', side_effect=TimeoutError('no clean terminal')):
            stream.cleanup_trial_bridge(bridge, receipt)
        self.assertIn('no clean terminal', receipt['cleanup_error'])
        self.assertNotIn('PlaygroundShutdown', bridge.calls)
        self.assertEqual(bridge.calls[-1], 'close')

    def test_clean_failure_phase_allows_shutdown_without_promoting_success(self):
        bridge, receipt = FakeBridge((7,)), {'trial_armed': True, 'passed': False}
        stream.cleanup_trial_bridge(bridge, receipt)
        self.assertEqual(receipt['shutdown'], 0)
        self.assertNotIn(stream.STOP_EXPORT, bridge.calls)
        self.assertFalse(receipt['passed'])

    def test_wait_rejects_cancelled_clean_failure_and_unsafe_terminals(self):
        for phase in (6, 7, 8):
            with self.subTest(phase=phase), self.assertRaisesRegex(RuntimeError, 'cancelled or failed'):
                stream.wait_trial(FakeBridge((phase,)), {})

    def test_completed_status_requires_exactly32_successful_leases(self):
        bridge = SimpleNamespace(export=lambda name: 5 | (31 << 16))
        with self.assertRaisesRegex(RuntimeError, 'exactly 32'):
            stream.wait_trial(bridge, {})

    def test_invalid_status_cannot_be_treated_as_a_clean_journal(self):
        for status in (-1, '5', 9, 5 | (33 << 16)):
            bridge = SimpleNamespace(export=lambda name: status)
            with self.subTest(status=status), self.assertRaises(RuntimeError):
                stream.trial_status(bridge, {})

    def test_owned_quiet_retail_guards_are_preserved(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            args, owned = self.make_args(root)
            self.assertEqual(stream.verify_owned(args), owned['game'])
            for name, changed in (('test_windows_behind_requested', False),
                                  ('background_input_requested', True),
                                  ('steam_app_id', 'wrong'),
                                  ('game_path', str(root / 'other.exe'))):
                altered = json.loads(json.dumps(owned))
                altered['launch'][name] = changed
                args.owned_json.write_text(json.dumps(altered), encoding='utf-8')
                with self.subTest(field=name), self.assertRaises(ValueError):
                    stream.verify_owned(args)
        self.assertIs(stream.decline_owned_private_settings_prompt,
                      call_probe.decline_owned_private_settings_prompt)

    @staticmethod
    def make_args(root):
        dll = root / 'candidate.dll'
        retail = root / 'Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
        owned = {'game': {'pid': 12, 'creation_time': 34, 'image': str(retail.resolve())},
                 'owner': 'test-owner', 'private_root': str(root / 'private'),
                 'launch': {'test_windows_behind_requested': True,
                            'background_input_requested': False, 'steam_app_id': '2523720',
                            'game_path': str(retail.resolve())}}
        owned_json = root / 'owned.json'
        owned_json.write_text(json.dumps(owned), encoding='utf-8')
        (root / 'playground-status-12.json').write_text(
            json.dumps({'ready': True, 'pickups': 1, 'pawn': '0x1000'}), encoding='utf-8')
        (root / 'playground-12.log').write_text('\n'.join(recording()), encoding='utf-8')
        args = SimpleNamespace(runtime_root=root, dll=dll, dll_sha256='ABCDEF', owned_json=owned_json)
        return args, owned

    def run_mocked(self, args, bridge, receipt, *, preflight=None):
        order = []
        constructor = mock.Mock(side_effect=lambda *a, **kw: order.append('construct') or bridge)
        check = mock.Mock(side_effect=preflight or (lambda *a: order.append('preflight') or True))
        with mock.patch.dict('sys.modules', {'runtime': SimpleNamespace(Bridge=constructor)}), \
                mock.patch.object(stream, 'verify_preload_call_function_entry', check), \
                mock.patch.object(stream, 'prepare_owned_lancer'), \
                mock.patch.object(stream, 'decline_owned_private_settings_prompt', return_value=True):
            stream.run(args, receipt)
        return order, constructor

    def test_full_success_requires_preflight_then_clean_shutdown_and_complete_log(self):
        with tempfile.TemporaryDirectory() as folder:
            args, owned = self.make_args(Path(folder))
            bridge, receipt = FakeBridge(), {'passed': False}
            order, constructor = self.run_mocked(args, bridge, receipt)
            self.assertEqual(order, ['preflight', 'construct'])
            constructor.assert_called_once_with(owned['game'], args.dll, expected_sha256='abcdef')
            self.assertTrue(receipt['passed'])
            self.assertTrue(receipt['private_settings_restore_declined'])
            self.assertEqual(receipt['sample_count'], 96)
            self.assertFalse(receipt['effect_proved'])
            self.assertFalse(receipt['ownership_proved'])
            self.assertIn('synthetic dual-hand', receipt['source_limit'])

    def test_preflight_mismatch_prevents_any_injection(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            constructor = mock.Mock()
            with mock.patch.dict('sys.modules', {'runtime': SimpleNamespace(Bridge=constructor)}), \
                    mock.patch.object(stream, 'verify_preload_call_function_entry',
                                      side_effect=RuntimeError('entry differs')):
                with self.assertRaisesRegex(RuntimeError, 'entry differs'):
                    stream.run(args, {'passed': False})
            constructor.assert_not_called()

    def test_completed_log_cannot_replace_live_restoration_check(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            for options, name in (({'bad_call_entry': True}, 'call_function'),
                                   ({'bad_process_entry': True}, 'process_event')):
                bridge, receipt = FakeBridge(**options), {'passed': False}
                with self.subTest(entry=name), self.assertRaisesRegex(RuntimeError, 'live entry was not restored'):
                    self.run_mocked(args, bridge, receipt)
                self.assertFalse(receipt['passed'])
                self.assertIn(f'{name}_restore_error', receipt)

    def test_clean_trial_failure_keeps_receipt_failed_after_cleanup(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            bridge, receipt = FakeBridge((7,)), {'passed': False}
            with self.assertRaisesRegex(RuntimeError, 'cancelled or failed'):
                self.run_mocked(args, bridge, receipt)
            self.assertEqual(receipt['shutdown'], 0)
            self.assertFalse(receipt['passed'])

    def test_start_export_exception_still_stops_a_possibly_armed_trial(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            bridge, receipt = FakeBridge((3, 6)), {'passed': False}
            original_export = bridge.export

            def export(name):
                if name == 'PlaygroundHandIkStreamTrialStart':
                    raise RuntimeError('start transport failed')
                return original_export(name)

            bridge.export = export
            with self.assertRaisesRegex(RuntimeError, 'start transport failed'):
                self.run_mocked(args, bridge, receipt)
            self.assertTrue(receipt['trial_start_attempted'])
            self.assertNotIn('trial_armed', receipt)
            self.assertIn(stream.STOP_EXPORT, bridge.calls)
            self.assertEqual(receipt['cleanup_status']['phase'], 6)
            self.assertEqual(receipt['shutdown'], 0)
            self.assertFalse(receipt['passed'])

    def test_explicit_start_rejection_can_shutdown_without_waiting_for_a_trial(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            bridge, receipt = FakeBridge((0,)), {'passed': False}
            original_export = bridge.export
            bridge.export = lambda name: 1 if name == 'PlaygroundHandIkStreamTrialStart' else original_export(name)
            with self.assertRaisesRegex(RuntimeError, 'did not arm'):
                self.run_mocked(args, bridge, receipt)
            self.assertTrue(receipt['trial_start_rejected'])
            self.assertNotIn(stream.STOP_EXPORT, bridge.calls)
            self.assertNotIn(stream.STATUS_EXPORT, bridge.calls)
            self.assertEqual(receipt['shutdown'], 0)

    def test_main_failure_writes_private_receipt_and_always_signals_owner_stop(self):
        with tempfile.TemporaryDirectory(dir=stream.ROOT / 'analysis') as folder:
            args, _ = self.make_args(Path(folder))
            output = Path(folder) / 'receipt.json'
            argv = ['runner', '--owned-json', str(args.owned_json), '--dll', str(args.dll),
                    '--dll-sha256', 'ABCDEF', '--runtime-root', str(args.runtime_root),
                    '--output', str(output)]
            with mock.patch.object(sys, 'argv', argv), \
                    mock.patch.object(stream, 'run', side_effect=RuntimeError('unsafe journal')):
                with self.assertRaisesRegex(RuntimeError, 'unsafe journal'):
                    stream.main()
            receipt = json.loads(output.read_text(encoding='utf-8'))
            self.assertFalse(receipt['passed'])
            self.assertEqual(receipt['dll_sha256'], 'abcdef')
            self.assertIn('unsafe journal', receipt['error'])
            self.assertEqual((Path(folder) / 'stop').read_text(encoding='ascii'), 'stop')

    def test_main_rejects_receipt_outside_analysis_before_running(self):
        with tempfile.TemporaryDirectory() as folder:
            args, _ = self.make_args(Path(folder))
            argv = ['runner', '--owned-json', str(args.owned_json), '--dll', str(args.dll),
                    '--dll-sha256', 'ABCDEF', '--runtime-root', str(args.runtime_root),
                    '--output', str(Path(folder) / 'receipt.json')]
            with mock.patch.object(sys, 'argv', argv), mock.patch.object(stream, 'run') as run:
                with self.assertRaisesRegex(ValueError, 'worktree analysis'):
                    stream.main()
            run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
