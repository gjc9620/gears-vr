import json
from pathlib import Path
import re
from types import SimpleNamespace
import unittest
from unittest import mock

import hand_animation_probe_run as probe


def event(phase, *, kind=17, call=1, source='call_function', **changes):
    fields = {
        'seq': 1 if phase == 'pre' else 2, 'call': call, 'parent': 0,
        'depth': 0, 'kind': kind, 'phase': phase, 'source': source,
        'function': 'PlayCustomAnim', 'outer': 'AnimNodeSlot',
        'object': '0000000100000000', 'function_ptr': '0000000100001000',
        'native_index': 0, 'ms': 100 if phase == 'pre' else 110,
        'pickup_request': 1, 'melee_request': 0, 'pickup_episode': 1,
        'melee_episode': 0, 'ownership_proved': 0, 'writes': 0,
        'caller_frame': '0000000200000000', 'caller_known': 1,
        'caller_node': '0000000100002000', 'caller_object': '0000000100003000',
        'caller_code': '0000000100004001', 'caller_locals': '0000000200001000',
        'caller_previous': '0000000000000000', 'caller_script': '0000000100004000',
        'caller_script_size': 16, 'param_transport': 'caller_frame_unconsumed',
    }
    fields.update(changes)
    return 'hand_animation_event ' + ' '.join(f'{key}={value}' for key, value in fields.items())


def log_for(events=(), *, call_summary=None, restored=None, **terminal_changes):
    terminal = {
        'status': 4, 'reason': 'explicit_stop', 'samples': 1, 'snapshots': 1,
        'calls': len({probe.fields_of(line, 'test')['call'] for line in events}),
        'event_lines': len(events), 'hook_callbacks': 10, 'hook_mask': '0',
        'bridge_mask': '0', 'special_changes': 0, 'melee_changes': 0,
        'started_active': 0, 'overflow': 0, 'field_limit': 0, 'function_limit': 0,
        'ownership_proved': 0, 'writes': 0,
        'gap': 'vm_direct_calls_uninstrumented;idle_is_not_release',
    }
    terminal.update(terminal_changes)
    lines = [*events, 'hand_animation_frame state_known=256 slots=19']
    if call_summary is not None:
        lines.append('hand_animation_call_probe_end ' + call_summary)
    lines.append('hand_animation_probe_end ' +
                 ' '.join(f'{key}={value}' for key, value in terminal.items()))
    if restored is not None:
        lines.append('hand_animation_call_hook_restored ' + restored)
    return '\n'.join(lines)


SUMMARY = 'callbacks=8 accepted=1 completed=1 frame_rejected=2 mask=20000 writes=0'
RESTORED = 'bytes=32 matched=1'


def call_log(events=None, **changes):
    if events is None:
        events = [event('pre'), event('post')]
    options = {'call_summary': SUMMARY, 'restored': RESTORED}
    options.update(changes)
    return log_for(events, **options)


class HandAnimationProbeLogTests(unittest.TestCase):
    def test_existing_default_log_needs_no_call_boundary(self):
        terminal = probe.validate_log(log_for(), 4)
        self.assertEqual(terminal['status'], 4)
        self.assertEqual(set(terminal), probe.TERMINAL_FIELDS)

    def test_opt_in_accepts_paired_slot_play_and_rejected_frames(self):
        terminal = probe.validate_log(call_log(hook_callbacks=0), 4, True)
        self.assertEqual(terminal['calls'], 1)
        self.assertEqual(terminal['ownership_proved'], 0)

    def test_call_summary_may_follow_original_terminal(self):
        lines = call_log().splitlines()
        lines[-2], lines[-3] = lines[-3], lines[-2]
        probe.validate_log('\n'.join(lines), 4, True)

    def test_opt_in_requires_exactly_one_companion_summary(self):
        for text in (call_log(call_summary=None),
                     call_log() + '\nhand_animation_call_probe_end ' + SUMMARY):
            with self.subTest(log=text), self.assertRaisesRegex(RuntimeError, 'exactly one CallFunction terminal'):
                probe.validate_log(text, 4, True)

    def test_opt_in_requires_complete_call_pair(self):
        for events in ([event('pre')], [event('post')],
                       [event('post'), event('pre')],
                       [event('pre'), event('post', call=2)],
                       [event('pre'), event('pre'), event('post')]):
            with self.subTest(events=events), self.assertRaisesRegex(RuntimeError, 'pair'):
                probe.validate_log(call_log(events), 4, True)

    def test_pair_keeps_original_callee_and_kind(self):
        for changes in ({'kind': 9}, {'object': '0000000200000000'},
                        {'function_ptr': '0000000200001000'}, {'parent': 4},
                        {'depth': 1}, {'seq': 1}, {'caller_node': '0000000100002008'}):
            with self.subTest(changes=changes), self.assertRaisesRegex(RuntimeError, 'identity or kind'):
                probe.validate_log(call_log([event('pre'), event('post', **changes)]), 4, True)

    def test_vm_code_pointer_may_advance_without_parameter_decode(self):
        probe.validate_log(call_log([event('pre'), event('post', caller_code='0000000100004010')]),
                           4, True)

    def test_caller_frame_must_remain_known_and_inside_valid_script(self):
        for phase in ('pre', 'post'):
            for changes in ({'caller_known': 0}, {'caller_script_size': 0},
                            {'caller_code': '0000000100004011'},
                            {'caller_frame': '0000000200000001'},
                            {'param_transport': 'decoded'}):
                events = [event('pre'), event('post')]
                events[0 if phase == 'pre' else 1] = event(phase, **changes)
                with self.subTest(phase=phase, changes=changes), self.assertRaises(RuntimeError):
                    probe.validate_log(call_log(events), 4, True)

    def test_body_play_alone_cannot_satisfy_slot_play_requirement(self):
        events = [event('pre', kind=9), event('post', kind=9)]
        summary = SUMMARY.replace('mask=20000', 'mask=200')
        with self.assertRaisesRegex(RuntimeError, 'slot_play kind17'):
            probe.validate_log(call_log(events, call_summary=summary), 4, True)

    def test_other_sources_do_not_substitute_for_call_function(self):
        for source in ('hook', 'bridge_invoke', 'call_function_extra'):
            with self.subTest(source=source), self.assertRaises(RuntimeError):
                probe.validate_log(call_log([event('pre', source=source),
                                            event('post', source=source)]), 4, True)

    def test_call_id_cannot_be_shared_across_sources(self):
        events = [event('pre'), event('pre', source='hook'), event('post')]
        with self.assertRaisesRegex(RuntimeError, 'reused across sources'):
            probe.validate_log(call_log(events), 4, True)

    def test_call_function_rows_cannot_decode_parameters_or_claim_ownership(self):
        for changes in ({'param_AnimName': 'Melee:1:0'}, {'writes': 1},
                        {'ownership_proved': 1}, {'depth': 16}, {'kind': 19}):
            with self.subTest(changes=changes), self.assertRaises(RuntimeError):
                probe.validate_log(call_log([event('pre', **changes), event('post')]), 4, True)

    def test_companion_counts_and_mask_match_real_pairs(self):
        for before, after in (('accepted=1', 'accepted=2'),
                              ('completed=1', 'completed=0'),
                              ('mask=20000', 'mask=20001'),
                              ('callbacks=8', 'callbacks=2'),
                              ('writes=0', 'writes=1'),
                              ('frame_rejected=2', 'frame_rejected=-1')):
            with self.subTest(change=after), self.assertRaises(RuntimeError):
                probe.validate_log(call_log(call_summary=SUMMARY.replace(before, after)), 4, True)
        with self.assertRaisesRegex(RuntimeError, 'schema'):
            probe.validate_log(call_log(call_summary=SUMMARY + ' ownership_proved=0'), 4, True)

    def test_hook_restoration_must_be_complete_and_after_terminals(self):
        for restored in (None, 'bytes=32 matched=0', 'bytes=31 matched=1',
                         RESTORED + ' matched=1', RESTORED + ' extra=1'):
            with self.subTest(restored=restored), self.assertRaises(RuntimeError):
                probe.validate_log(call_log(restored=restored), 4, True)
        lines = call_log().splitlines()
        lines.insert(0, lines.pop())
        with self.assertRaisesRegex(RuntimeError, 'restoration'):
            probe.validate_log('\n'.join(lines), 4, True)
        with self.assertRaisesRegex(RuntimeError, 'restoration'):
            probe.validate_log(call_log() + '\nhand_animation_call_hook_restored ' + RESTORED, 4, True)


class HandAnimationProbeEntryTests(unittest.TestCase):
    def test_retained_certificate_matches_native_certificate(self):
        source = (probe.ROOT / 'tools/native-capture/ui_call_boundary.h').read_text(encoding='utf-8')
        certificate = source.split('entry_bytes{{', 1)[1].split('}};', 1)[0]
        expected = bytes(int(value, 16) for value in re.findall(r'0x[0-9a-fA-F]+', certificate))
        self.assertEqual(len(expected), 32)
        self.assertEqual(probe.CALL_FUNCTION_ENTRY, expected)
        self.assertIn(f'rva=0x{probe.CALL_FUNCTION_RVA:x}', source)

    def test_process_event_certificate_matches_existing_loader(self):
        source = (probe.ROOT / 'tools/playground/runtime.py').read_text(encoding='utf-8')
        certificate = re.search(r"expected=bytes.fromhex\('([0-9a-f]+)'\)", source).group(1)
        self.assertEqual(len(probe.PROCESS_EVENT_ENTRY), 64)
        self.assertEqual(probe.PROCESS_EVENT_ENTRY, bytes.fromhex(certificate))
        self.assertIn(f'self.r.base+0x{probe.PROCESS_EVENT_RVA:x}', source)

    def test_live_entry_mismatch_is_retained_and_rejected(self):
        for raw in (b'\xe9' + probe.CALL_FUNCTION_ENTRY[1:], probe.CALL_FUNCTION_ENTRY[:-1]):
            receipt = {}
            resolver = SimpleNamespace(base=0x140000000, read=mock.Mock(return_value=raw))
            with self.subTest(raw=raw.hex()), self.assertRaisesRegex(RuntimeError, '32-byte certificate'):
                probe.verify_call_function_entry(resolver, receipt, 'before_load')
            resolver.read.assert_called_once_with(0x1401d6f30, 32)
            self.assertFalse(receipt['call_function_entry']['before_load']['matched'])

    def test_preload_uses_independent_reader_and_closes_it(self):
        identity = {'pid': 12, 'creation_time': 34, 'image': 'retail.exe'}
        resolver = SimpleNamespace(base=0x140000000,
                                   read=mock.Mock(return_value=probe.CALL_FUNCTION_ENTRY),
                                   prepare=mock.Mock(), p=SimpleNamespace(close=mock.Mock()))
        runtime = SimpleNamespace(Resolver=mock.Mock(return_value=resolver),
                                  WindowsControl=mock.Mock(return_value=SimpleNamespace(
                                      identity=mock.Mock(return_value=identity))))
        receipt = {}
        with mock.patch.dict('sys.modules', {'runtime': runtime}), \
                mock.patch.object(Path, 'read_text', return_value=json.dumps({'sha256': 'certified'})):
            self.assertTrue(probe.verify_preload_call_function_entry(identity, probe.ROOT, receipt))
        runtime.Resolver.assert_called_once_with(12, {'sha256': 'certified'})
        resolver.prepare.assert_called_once_with()
        resolver.p.close.assert_called_once_with()
        self.assertTrue(receipt['call_function_entry']['before_load']['matched'])

    def test_preload_mismatch_prevents_bridge_construction(self):
        args = SimpleNamespace(runtime_root=probe.ROOT, require_call_boundary=True)
        constructor = mock.Mock()
        with mock.patch.dict('sys.modules', {'runtime': SimpleNamespace(Bridge=constructor)}), \
                mock.patch.object(probe, 'verify_owned', return_value={'pid': 12}), \
                mock.patch.object(probe, 'verify_preload_call_function_entry',
                                  side_effect=RuntimeError('live entry differs')):
            with self.assertRaisesRegex(RuntimeError, 'live entry differs'):
                probe.run(args, {})
        constructor.assert_not_called()

    def test_default_skips_call_preflight_and_normalizes_dll_hash(self):
        args = SimpleNamespace(runtime_root=probe.ROOT, dll=Path('candidate.dll'),
                               dll_sha256='ABCDEF')
        constructor = mock.Mock(side_effect=RuntimeError('stop before loading'))
        identity = {'pid': 12}
        with mock.patch.dict('sys.modules', {'runtime': SimpleNamespace(Bridge=constructor)}), \
                mock.patch.object(probe, 'verify_owned', return_value=identity), \
                mock.patch.object(probe, 'verify_preload_call_function_entry') as preflight:
            with self.assertRaisesRegex(RuntimeError, 'stop before loading'):
                probe.run(args, {})
        constructor.assert_called_once_with(identity, args.dll, expected_sha256='abcdef')
        preflight.assert_not_called()

    def test_live_restoration_is_read_after_shutdown_and_before_close(self):
        calls = []
        entries = {0x140000000 + probe.CALL_FUNCTION_RVA: probe.CALL_FUNCTION_ENTRY,
                   0x140000000 + probe.PROCESS_EVENT_RVA: probe.PROCESS_EVENT_ENTRY}

        def read(address, size):
            calls.append(('read', size))
            return entries[address]

        reader = SimpleNamespace(base=0x140000000,
                                 read=read)
        bridge = SimpleNamespace(r=reader, verify=lambda: calls.append('verify'),
                                 export=lambda name: calls.append(name) or 0,
                                 close=lambda: calls.append('close'))
        receipt = {}
        probe.shutdown_bridge(bridge, receipt, True)
        self.assertEqual(calls, ['PlaygroundShutdown', 'verify', ('read', 32), 'verify',
                                 'verify', ('read', 64), 'verify', 'close'])
        self.assertTrue(receipt['call_function_entry']['after_shutdown']['matched'])
        self.assertTrue(receipt['process_event_entry']['after_shutdown']['matched'])
        self.assertEqual(receipt['shutdown'], 0)

    def test_false_restoration_log_cannot_replace_live_entry_check(self):
        reader = SimpleNamespace(base=0x140000000,
                                 read=mock.Mock(return_value=b'\xe9' + probe.CALL_FUNCTION_ENTRY[1:]))
        bridge = SimpleNamespace(r=reader, verify=mock.Mock(),
                                 export=mock.Mock(return_value=0), close=mock.Mock())
        receipt = {}
        probe.shutdown_bridge(bridge, receipt, True)
        self.assertIn('call_function_restore_error', receipt)
        self.assertFalse(receipt['call_function_entry']['after_shutdown']['matched'])
        self.assertEqual(reader.read.call_count, 2)
        bridge.close.assert_called_once_with()

    def test_default_shutdown_retains_compatibility_without_entry_read(self):
        bridge = SimpleNamespace(export=mock.Mock(return_value=0), close=mock.Mock())
        receipt = {}
        probe.shutdown_bridge(bridge, receipt)
        self.assertEqual(receipt, {'shutdown': 0})
        bridge.close.assert_called_once_with()


if __name__ == '__main__':
    unittest.main()
