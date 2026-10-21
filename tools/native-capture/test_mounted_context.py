"""Synthetic memory only: mounted possession, identity and race rejection."""
import copy
import struct
import unittest
from unittest import mock

from test_cinematic_anchor import CinematicFixture
from anchor_config import LiveRegistryAnchorSampler, AnchorRejected
from mounted_context import MountedContextCertificate, MountedTimestampChanged
from mounted_read_pass import MountedReadPass


class MountedFixture(CinematicFixture):
    TURRET = 0x80000
    INVENTORY = 0x81000

    def __init__(self):
        super().__init__()
        self.profile['property_offset_internal'] = 0x94
        self.add(0x70000, 'Vehicle', 0x1000, parent=0x3000)
        self.add(0x71000, 'Turret', 0x1000, parent=0x70000)
        self.add(0x72000, 'PlayerController', 0x1000, parent=0x6000)
        self.add(0x73000, 'Camera', 0x1000, parent=0x4000)
        self.add(0x74000, 'WarInventoryManager', 0x1000, parent=0x4000)
        self.ptr(0x5000 + 96, 0x72000)
        self.ptr(0x9000 + 96, 0x73000)
        self.add(self.TURRET, 'Turret_0', 0x71000, outer=0x50000)
        self.add(self.INVENTORY, 'Inventory_0', 0x74000, outer=0x50000)
        for actor in (0x10000, 0x30000):self.ptr(actor + 64, 0x50000)
        for index, (owner, name, offset) in enumerate((
                (0x6000, 'Pawn', 0x224), (0x3000, 'Controller', 0x250),
                (0x70000, 'Driver', 0x800), (0x3000, 'DrivenVehicle', 0x810),
                (0x72000, 'PlayerCamera', 0x37c), (0x73000, 'PCOwner', 0x224),
                (0x72000, 'Player', 0x374), (0x3000, 'InvManager', 0x820),
                (0x4000, 'Owner', 0x8c))):
            address = 0x90000 + index * 0x1000
            self.add(address, name, 0x69000, outer=owner)
            self.properties[owner, name] = dict(address=address, offset=offset, type='ObjectProperty')
            self.word(address + 0x70, 1)
            self.word(address + 0x74, 8)
            self.word(address + 0x94, offset)
        for address, value in ((0x10224, self.TURRET), (self.TURRET + 0x250, 0x10000),
                               (self.TURRET + 0x800, 0x20000), (0x20810, self.TURRET),
                               (0x20250, 0), (0x20820, self.INVENTORY),
                               (self.INVENTORY + 0x8c, 0x20000),
                               (self.TURRET + self.WORLD_OFFSET, self.WORLD_INFO),
                               (0x30000 + self.WORLD_OFFSET, self.WORLD_INFO),
                               (0x30500, self.TURRET)):
            self.ptr(address, value)
        self.sampler = LiveRegistryAnchorSampler.__new__(LiveRegistryAnchorSampler)
        self.sampler.r = self
        for key, value in dict(controller=0x10000, camera=0x30000, local=0x40000,
                               stamp=0x30600, pov=0x30604, view_target=0x30500).items():
            setattr(self.sampler, key, value)

    def stream_levels(self):
        self.levels = {}
        for index, (label, actor) in enumerate((('controlled', self.TURRET), ('driver', 0x20000),
                ('controller', 0x10000), ('camera', 0x30000), ('world_info', self.WORLD_INFO))):
            level = 0xa0000 + index * 0x1000
            self.add(level, 'StreamLevel_' + str(index), 0x7000)
            self.ptr(actor + 64, level)
            self.levels[label] = level
        self.ptr(self.INVENTORY + 64, self.levels['driver'])
        return self


class MountedContextTests(unittest.TestCase):
    def test_batched_pass_reduces_actual_resolver_reads_with_equal_proof(self):
        counts, proofs = {}, {}
        for label, span in (('unbatched', 8), ('batched', 256)):
            f = MountedFixture();cert = MountedContextCertificate(f.sampler)
            with mock.patch.object(MountedReadPass, 'MAX_SPAN', span), mock.patch.object(f, 'read', wraps=f.read) as reading:
                proofs[label] = cert.capture();counts[label] = reading.call_count
            self.assertFalse(hasattr(f.sampler, '_mounted_read_pass'))
        self.assertEqual(proofs['unbatched'], proofs['batched'])
        self.assertLess(counts['batched'], counts['unbatched'] * .8, counts)

    def test_slot_batches_are_bounded_and_each_pass_rereads(self):
        f = MountedFixture();cert = MountedContextCertificate(f.sampler)
        with mock.patch.object(f, 'read', wraps=f.read) as reading:
            cert.capture()
        calls = [(x.args[0], x.args[1]) for x in reading.call_args_list]
        batches = [(a, n) for a, n in calls if n > 8 and (
            f.objects_data <= a < f.objects_data + f.object_count * 8 or
            f.names_data <= a < f.names_data + f.name_count * 8)]
        self.assertTrue(batches)
        for a, n in batches:
            self.assertLessEqual(n, 256);self.assertEqual(a // 4096, (a + n - 1) // 4096)
            self.assertIn(calls.count((a, n)), (2, 4))
        # Both separate FName reads and fresh entry bodies survive the batching.
        for address, identity in cert._initial['identities'].items():
            expected = 4 if address in cert._initial['validation']['inner_before']['roles'] else 2
            self.assertEqual(calls.count((address + f.profile['object_name'], 4)), expected)
            entry = f.u64(f.names_data + identity['name_index'] * 8)
            self.assertGreaterEqual(calls.count((entry, 148)), 2)

    def test_slot_batch_short_read_is_terminal_and_binding_is_removed(self):
        for registry in ('objects', 'names'):
            f = MountedFixture();cert = MountedContextCertificate(f.sampler);read = f.read
            start = getattr(f, registry + '_data');count = getattr(f, 'object_count' if registry == 'objects' else 'name_count')
            def short(address, size):
                data = read(address, size)
                return data[:-1] if start <= address < start + count * 8 and size > 8 else data
            with mock.patch.object(f, 'read', side_effect=short):
                with self.assertRaisesRegex(AnchorRejected, 'Short mounted identity memory read'):cert.capture()
            self.assertFalse(hasattr(f.sampler, '_mounted_read_pass'))

    def test_slot_change_after_opening_batch_rejected_by_independent_closing_pass(self):
        for registry in ('objects', 'names'):
            f = MountedFixture();cert = MountedContextCertificate(f.sampler);read = f.read
            identity = cert._initial['roles']['controlled']
            slot = getattr(f, registry + '_data') + identity['internal_index' if registry == 'objects' else 'name_index'] * 8
            changed = False
            def changing(address, size):
                nonlocal changed
                value = read(address, size)
                if not changed and size > 8 and address <= slot < address + size:
                    changed = True;f.ptr(slot, 0)
                return value
            with mock.patch.object(f, 'read', side_effect=changing):
                with self.assertRaises(AnchorRejected):cert.capture()
            self.assertTrue(changed);self.assertFalse(hasattr(f.sampler, '_mounted_read_pass'))

    def test_repeated_selected_slot_request_forces_new_read(self):
        f = MountedFixture();cert = MountedContextCertificate(f.sampler)
        ids = cert._initial['identities'];names = {a: x['name_index'] for a, x in ids.items()}
        reader = MountedReadPass(f, ids, names);address = f.TURRET
        self.assertEqual(reader.fresh(address), f.sampler.fresh(address))
        f.ptr(f.objects_data + ids[address]['internal_index'] * 8, 0)
        with self.assertRaises(AnchorRejected):reader.fresh(address)

    def test_mounted_pass_retains_fresh_name_index_entry_and_text_checks(self):
        for mutation in ('object_name', 'entry_index', 'entry_text', 'entry_short'):
            with self.subTest(mutation=mutation):
                f = MountedFixture();cert = MountedContextCertificate(f.sampler)
                ids = cert._initial['identities'];identity = ids[f.TURRET]
                reader = MountedReadPass(f, ids, {a: x['name_index'] for a, x in ids.items()})
                entry = f.u64(f.names_data + identity['name_index'] * 8);read = f.read
                if mutation == 'entry_index':f.word(entry, identity['name_index'] + 1)
                elif mutation == 'entry_text':f.put(entry + 20, 'Changed\0'.encode('utf-16-le'))
                def changing(address, size):
                    value = read(address, size)
                    if mutation == 'object_name' and (address, size) == (f.TURRET, 96):
                        f.word(f.TURRET + f.profile['object_name'], 0)
                    if mutation == 'entry_short' and (address, size) == (entry, 148):return value[:-1]
                    return value
                with mock.patch.object(f, 'read', side_effect=changing):
                    with self.assertRaises(AnchorRejected):reader.fresh(f.TURRET)

    def test_failed_capture_buffers_never_survive_into_retry(self):
        f = MountedFixture();cert = MountedContextCertificate(f.sampler)
        identity = cert._initial['roles']['controlled'];slot = f.objects_data + identity['internal_index'] * 8
        original = f.u64(slot);f.ptr(slot, 0)
        with self.assertRaises(AnchorRejected):cert.capture()
        self.assertFalse(hasattr(f.sampler, '_mounted_read_pass'))
        f.ptr(slot, original)
        self.assertEqual(cert.capture(), cert._initial)

    def test_inventory_actor_owner_and_level_are_distinct(self):
        f = MountedFixture().stream_levels()
        proof = MountedContextCertificate(f.sampler).capture()
        self.assertEqual(proof['roles']['inventory']['outer'], f.levels['driver'])
        self.assertEqual(proof['links']['inventory_owner']['value'], 0x20000)
        for offset, value in ((0x8c, 0), (0x8c, f.TURRET), (64, 0x20000),
                              (64, f.levels['controlled']), (64, 0)):
            with self.subTest(offset=offset, value=value):
                f = MountedFixture().stream_levels()
                f.ptr(f.INVENTORY + offset, value)
                with self.assertRaises(AnchorRejected):MountedContextCertificate(f.sampler)

    def test_inventory_owner_changes_during_read_or_between_captures_reject(self):
        f = MountedFixture()
        original = f.read
        def read(address, size):
            data = original(address, size)
            if address == f.INVENTORY + 0x8c and size == 8:
                f.ptr(address, 0)
            return data
        f.read = read
        with self.assertRaises(AnchorRejected):MountedContextCertificate(f.sampler)
        f = MountedFixture();certificate = MountedContextCertificate(f.sampler)
        f.ptr(f.INVENTORY + 0x8c, f.TURRET)
        with self.assertRaises(AnchorRejected):certificate.capture()

    def test_inventory_owner_reflected_layout_and_identity_rejected(self):
        for offset, value, width in ((0x40, 0x3000, 8), (0x74, 4, 4),
                                     (0x94, 0x90, 4), (0x08, 0xffffffff, 4)):
            f=MountedFixture()
            prop=f.property(0x74000, 'Owner', True)['address']
            (f.ptr if width==8 else f.word)(prop+offset, value)
            with self.assertRaises((AnchorRejected, RuntimeError)):
                MountedContextCertificate(f.sampler)

    def test_rejected_camera_targets_have_failure_only_consistency_evidence(self):
        for label, target in (('driver', 0x20000), ('null', 0), ('foreign', 0xb0000)):
            with self.subTest(target=label):
                f = MountedFixture()
                if label == 'foreign':f.add(target, 'ForeignTarget_0', 0x3000, outer=0x50000)
                f.ptr(f.sampler.view_target, target)
                with self.assertRaisesRegex(AnchorRejected, '^Mounted camera target is not controlled turret$') as failure:
                    MountedContextCertificate(f.sampler)
                self.assertNotIsInstance(failure.exception, MountedTimestampChanged)
                observation = failure.exception.camera_observation
                self.assertTrue(observation['diagnostic_only']); self.assertFalse(observation['accepted'])
                self.assertTrue(observation['consistent_read']); self.assertFalse(observation['atomic_snapshot'])
                self.assertEqual(observation['target_field'], dict(address=f.sampler.view_target, value=target, observed_value=target))
                self.assertEqual(observation['target_is_driver'], label == 'driver')
                self.assertEqual(observation['roles']['controlled']['address'], f.TURRET)
                self.assertEqual(observation['roles']['driver']['address'], 0x20000)
                self.assertTrue(observation['timestamp']['consistent'])
                self.assertTrue(all(header['consistent'] for header in observation['headers'].values()))
                if target:
                    self.assertEqual(observation['target_identity']['address'], target)
                    self.assertEqual(observation['target_class']['address'], observation['target_identity']['cls'])
                else:
                    self.assertIsNone(observation['target_identity']); self.assertIsNone(observation['target_class'])

    def test_camera_observation_churn_and_read_errors_cannot_change_rejection(self):
        for kind in ('target', 'identity', 'timestamp', 'header', 'property', 'link', 'read_error', 'target_unreadable'):
            with self.subTest(kind=kind):
                f = MountedFixture(); f.ptr(f.sampler.view_target, 0xdeadbeef if kind == 'target_unreadable' else 0x20000)
                original_read = f.read; original_fresh = f.sampler.fresh; hits = {}; bracket_started = False
                def read(address, size):
                    nonlocal bracket_started
                    # Discovery may read possession links. This fixture tests
                    # changes during the failure diagnostic, after its capture
                    # has opened the camera bracket, not discovery rejection.
                    if (address, size) == (f.sampler.stamp, 4):bracket_started = True
                    if not bracket_started:return original_read(address, size)
                    key = address, size; hits[key] = hits.get(key, 0) + 1
                    data = original_read(address, size)
                    if hits[key] >= (1 if kind in ('property', 'link', 'read_error') else 2):
                        if kind == 'target' and address == f.sampler.view_target:return struct.pack('<Q', f.TURRET)
                        if kind == 'timestamp' and address == f.sampler.stamp:return struct.pack('<f', 42)
                        if kind == 'header' and address == f.names_header:
                            data_ptr, count, cap = struct.unpack('<Qii', data);return struct.pack('<Qii', data_ptr, count + 1, cap + 1)
                        if kind == 'property' and address == 0x92094:return struct.pack('<I', 0x808)
                        if kind == 'link' and address == f.TURRET + 0x800:return struct.pack('<Q', 0)
                        if kind == 'read_error' and address == f.TURRET + 0x800:raise RuntimeError('Diagnostic link unreadable')
                    return data
                fresh_hits = {}
                def fresh(address):
                    value = original_fresh(address);fresh_hits[address] = fresh_hits.get(address, 0) + 1
                    if kind == 'identity' and address == f.TURRET and fresh_hits[address] >= 2:
                        value = dict(value, number=value['number'] + 1)
                    return value
                with mock.patch.object(f, 'read', side_effect=read), mock.patch.object(f.sampler, 'fresh', side_effect=fresh):
                    with self.assertRaisesRegex(AnchorRejected, '^Mounted camera target is not controlled turret$') as failure:
                        MountedContextCertificate(f.sampler)
                self.assertNotIsInstance(failure.exception, MountedTimestampChanged)
                observation = failure.exception.camera_observation
                self.assertFalse(observation['consistent_read']);self.assertFalse(observation['accepted'])
                if kind in ('read_error', 'target_unreadable'):
                    self.assertIn('read_error', observation)
                    self.assertIn('exception_type', observation['read_error'])
                    self.assertIn('stage', observation['read_error'])
                else:self.assertTrue(observation['mismatches'])

    def test_accepted_turret_does_not_run_failure_diagnostic(self):
        f = MountedFixture()
        proof = MountedContextCertificate(f.sampler).capture()
        self.assertEqual(proof['roles']['camera_target']['address'], f.TURRET)
        self.assertNotIn('camera_observation', proof)

    def test_guard_propagates_camera_diagnostic_without_retry_authority(self):
        from anchor_config import AnchorSamplingRejected
        f = MountedFixture();f.ptr(f.sampler.view_target, 0x20000)
        with self.assertRaises(AnchorSamplingRejected) as failure:
            LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
        attempts = failure.exception.evidence['guard_acquisitions'][0]['attempts']
        self.assertEqual(len(attempts), 1);self.assertFalse(attempts[0]['retryable'])
        self.assertTrue(attempts[0]['camera_observation']['diagnostic_only'])
        self.assertTrue(attempts[0]['camera_observation']['target_is_driver'])

    def test_capture_local_reuse_still_revalidates_every_identity(self):
        f = MountedFixture()
        with mock.patch.object(f.sampler, 'fresh', wraps=f.sampler.fresh) as fresh:
            cert = MountedContextCertificate(f.sampler)
        addresses = [call.args[0] for call in fresh.call_args_list]
        self.assertEqual(set(addresses), set(cert._initial['identities']))
        role_ids = cert._initial['validation']['inner_before']['roles']
        self.assertTrue(all(addresses.count(address) == (4 if address in role_ids else 2)
                            for address in set(addresses)))
        with mock.patch.object(f.sampler, 'fresh', wraps=f.sampler.fresh) as fresh:
            self.assertEqual(cert.capture(), cert._initial)
        self.assertEqual(fresh.call_count, len(addresses))

    def test_reflection_discovery_precedes_frame_bracket_and_never_repeats_in_capture(self):
        f = MountedFixture();events = [];read = f.read;prop = f.property
        def reading(address, size):
            if (address, size) == (f.sampler.stamp, 4):events.append('stamp')
            return read(address, size)
        def discovering(cls, name, inherited=False):
            events.append(('property', cls, name));return prop(cls, name, inherited)
        with mock.patch.object(f, 'read', side_effect=reading), mock.patch.object(f, 'property', side_effect=discovering):
            cert = MountedContextCertificate(f.sampler)
            calls = [event for event in events if isinstance(event, tuple)]
            self.assertEqual(len(calls), len(set(calls)))
            self.assertEqual(len(calls), len(cert._property_hints))
            self.assertTrue(calls)
            self.assertFalse(any(isinstance(event, tuple) for event in events[events.index('stamp'):]))
            events.clear();cert.capture();cert.capture()
            self.assertTrue(events);self.assertEqual(set(events), {'stamp'})
        # No alternative resolver call silently replaces the cached hints.
        with mock.patch.object(f, 'property', side_effect=AssertionError('reflection inside capture')):
            self.assertEqual(cert.capture(), cert._initial)

    def test_prepared_discovery_rejects_stale_offset_identity_super_and_possession(self):
        for mutation in ('offset', 'field_identity', 'role_identity', 'super', 'possession', 'registry'):
            with self.subTest(mutation=mutation):
                f = MountedFixture();cert = MountedContextCertificate(f.sampler, defer_capture=True)
                self.assertIsNone(cert._initial)
                self.assertFalse(cert.certifies_world({}))
                if mutation == 'offset':f.word(0x92094, 0x808)
                elif mutation == 'field_identity':f.word(0x92000 + 76, 1)
                elif mutation == 'role_identity':f.word(f.TURRET + 76, 1)
                elif mutation == 'super':f.ptr(0x71000 + 96, 0x3000)
                elif mutation == 'possession':f.ptr(0x10224, 0x20000)
                else:f.ptr(f.objects_data + f.u32(0x92000 + 8) * 8, 0)
                with mock.patch.object(f, 'property', side_effect=AssertionError('unexpected rediscovery')):
                    with self.assertRaises(AnchorRejected):cert.capture()
                self.assertIsNone(cert._initial)

    def test_guard_timestamp_retry_retains_discovery_but_not_failed_certificate(self):
        f = MountedFixture();discover = MountedContextCertificate._discover_properties
        capture = MountedContextCertificate._capture;discoveries = [];captures = []
        def discovering(cert):
            discoveries.append(cert);return discover(cert)
        def capturing(cert):
            captures.append(cert)
            if len(captures) == 1:
                self.assertIsNone(cert._initial)
                raise MountedTimestampChanged('synthetic first bracket rollover')
            if len(captures) == 2:self.assertIsNone(cert._initial)
            return capture(cert)
        with mock.patch.object(MountedContextCertificate, '_discover_properties', discovering),\
             mock.patch.object(MountedContextCertificate, '_capture', capturing):
            sampler = LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
        self.assertEqual(len(discoveries), 1)
        self.assertEqual(len({id(cert) for cert in captures}), 1)
        self.assertIs(sampler.mounted_contract, discoveries[0])
        self.assertIsNotNone(sampler.mounted_contract._initial)
        attempts = sampler.guard_acquisitions[0]['attempts']
        self.assertEqual(len(attempts), 2)
        self.assertTrue(attempts[0]['retryable']);self.assertTrue(attempts[1]['strict_guard_accepted'])

    def test_discovery_is_within_existing_guard_budget(self):
        from anchor_config import AnchorSamplingRejected
        f = MountedFixture();ticks = [0];discover = MountedContextCertificate._discover_properties
        def discovering(cert):
            result = discover(cert);ticks[0] = 201000000;return result
        with mock.patch.object(MountedContextCertificate, '_discover_properties', discovering):
            with self.assertRaises(AnchorSamplingRejected) as failure:
                LiveRegistryAnchorSampler(f, clock_ns=lambda: ticks[0])
        evidence = failure.exception.evidence['guard_acquisitions'][0]
        self.assertEqual(evidence['deadline_perf_counter_ns'], 200000000)
        self.assertEqual(evidence['strict_guard_attempts'], 1)
        self.assertFalse(evidence['accepted'])
        self.assertEqual(evidence['failure_reason'], 'Anchor guard acquisition deadline exceeded')

    def test_timestamp_only_transient_retries_once_or_exhausts_existing_cap(self):
        from anchor_config import AnchorSamplingRejected
        original_capture = MountedContextCertificate._capture
        for continuous in (False, True):
            with self.subTest(continuous=continuous):
                f = MountedFixture(); captures = 0
                def capture(cert):
                    nonlocal captures
                    captures += 1
                    if captures > 1 and not continuous:
                        return original_capture(cert)
                    read = f.read; stamps = 0
                    def changing(address, size):
                        nonlocal stamps
                        if (address, size) == (cert.sampler.stamp, 4):
                            stamps += 1
                            if stamps == 2:
                                old = struct.unpack('<f', read(address, size))[0]
                                f.put(address, struct.pack('<f', old + 1))
                        return read(address, size)
                    with mock.patch.object(f, 'read', side_effect=changing):
                        return original_capture(cert)
                with mock.patch.object(MountedContextCertificate, '_capture', capture):
                    if continuous:
                        with self.assertRaises(AnchorSamplingRejected) as failure:
                            LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
                        evidence = failure.exception.evidence['guard_acquisitions'][0]
                        self.assertEqual(captures, 5)
                        self.assertEqual(len(evidence['attempts']), 5)
                        self.assertTrue(all(x['retryable'] for x in evidence['attempts']))
                    else:
                        sampler = LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
                        evidence = sampler.guard_acquisitions[0]
                        self.assertTrue(evidence['accepted'])
                        self.assertEqual(len(evidence['attempts']), 2)
                    self.assertEqual(evidence['attempts'][0]['exception_type'], 'MountedTimestampChanged')

    def test_camera_advance_outside_inner_window_is_allowed_without_calling_base_guard(self):
        from anchor_config import AnchorSampler
        f = MountedFixture();sampler = LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
        original = f.read; stamps = 0
        def advancing(address, size):
            nonlocal stamps
            if (address, size) == (sampler.stamp, 4):stamps += 1
            # A camera frame starts during the outer opening, and another
            # after the inner window, before closing metadata validation.
            if (address, size) == (f.TURRET, 96) and stamps in (0, 3):
                f.put(sampler.stamp, struct.pack('<f', 20 + stamps))
                f.put(sampler.pov, struct.pack('<3f3if', 400 + stamps, 500, 600, 7, 8, 9, 90))
            return original(address, size)
        with mock.patch.object(f,'read',side_effect=advancing), \
             mock.patch.object(AnchorSampler,'guard',side_effect=AssertionError('old long mounted bracket')):
            result = sampler.guard()
        proof = result['mounted_context'];self.assertEqual(stamps, 3)
        self.assertEqual(proof['camera_timestamp_raw'],struct.pack('<f',20).hex())
        self.assertEqual(f.read(sampler.stamp,4),struct.pack('<f',23))
        self.assertEqual(proof['validation']['inner_before'],proof['validation']['inner_after'])

    def test_two_layer_inner_and_outer_changes_reject_without_retry(self):
        from anchor_config import AnchorSamplingRejected
        for phase in ('inner', 'outer_closing'):
            for kind in ('identity', 'super', 'property', 'link', 'level', 'slot'):
                with self.subTest(phase=phase,kind=kind):
                    f = MountedFixture();sampler = LiveRegistryAnchorSampler(f,clock_ns=lambda: 0)
                    original = f.read;stamps = 0;changed = False
                    def changing(address,size):
                        nonlocal stamps, changed
                        if (address,size)==(sampler.stamp,4):stamps += 1
                        trigger = stamps == 1 if phase=='inner' else stamps == 3 and size==96
                        if trigger and not changed:
                            changed = True
                            if kind=='identity':f.word(f.TURRET+76,1)
                            elif kind=='super':f.ptr(0x71060,0)
                            elif kind=='property':f.word(0x92094,0x808)
                            elif kind=='link':f.ptr(f.TURRET+0x800,0)
                            elif kind=='level':f.ptr(f.TURRET+64,0)
                            else:f.ptr(f.objects_data+f.u32(f.TURRET+8)*8,0)
                        return original(address,size)
                    with mock.patch.object(f,'read',side_effect=changing):
                        with self.assertRaises(AnchorSamplingRejected) as failure:sampler.guard()
                    self.assertTrue(changed)
                    attempts=failure.exception.evidence['guard_acquisitions'][-1]['attempts']
                    self.assertEqual(len(attempts),1);self.assertFalse(attempts[0]['retryable'])

    def test_outer_comparison_requires_all_static_values_and_keys_equal(self):
        from anchor_config import _check_mounted_bracket
        f = MountedFixture(); before = MountedContextCertificate(f.sampler).capture()
        _check_mounted_bracket(before, copy.deepcopy(before))
        for key in before:
            if key in ('camera_timestamp_raw', 'camera_pov_raw'):continue
            with self.subTest(key=key):
                after = copy.deepcopy(before)
                after['camera_timestamp_raw'] = struct.pack('<f', 11).hex()
                after[key] = None
                with self.assertRaises(AnchorRejected) as failure:_check_mounted_bracket(before, after)
                self.assertNotIsInstance(failure.exception, MountedTimestampChanged)
        for change in ('add', 'remove'):
            after = copy.deepcopy(before);after['camera_timestamp_raw'] = struct.pack('<f', 11).hex()
            if change == 'add':after['new_field'] = True
            else:del after['atomic_snapshot']
            with self.assertRaises(AnchorRejected) as failure:_check_mounted_bracket(before, after)
            self.assertNotIsInstance(failure.exception, MountedTimestampChanged)

    def test_timestamp_transient_uses_existing_guard_deadline(self):
        from anchor_config import AnchorSamplingRejected
        sampler = LiveRegistryAnchorSampler(MountedFixture())
        ticks = iter(range(0, 1000000000, 50000000))
        sampler.guard_clock_ns = lambda: next(ticks)
        with mock.patch.object(sampler, '_strict_guard', side_effect=MountedTimestampChanged('rollover')) as strict:
            with self.assertRaises(AnchorSamplingRejected) as failure:
                sampler.guard()
        self.assertEqual(strict.call_count, 2)
        evidence = failure.exception.evidence['guard_acquisitions'][-1]
        self.assertEqual(evidence['deadline_perf_counter_ns'] - evidence['started_perf_counter_ns'], 200000000)
        self.assertEqual(len(evidence['attempts']), 2)
        self.assertTrue(all(x['retryable'] for x in evidence['attempts']))

    def test_timestamp_change_does_not_hide_other_boundary_mutation(self):
        for target in (0x80800, 0x92094, 0x71060, 0x80000):
            with self.subTest(target=hex(target)):
                f = MountedFixture(); original = f.read; counts = {}
                def changing(address, size):
                    counts[address] = counts.get(address, 0) + 1
                    data = original(address, size)
                    if address in (target, 0x30600) and counts[address] >= 2:
                        index = 76 if address == 0x80000 else 0
                        return data[:index] + bytes([data[index] ^ 1]) + data[index + 1:]
                    return data
                with mock.patch.object(f, 'read', side_effect=changing):
                    with self.assertRaises((AnchorRejected, RuntimeError)) as failure:
                        MountedContextCertificate(f.sampler)
                self.assertNotIsInstance(failure.exception, MountedTimestampChanged)

    def test_pov_with_timestamp_rollover_retries_and_uses_new_frame(self):
        f = MountedFixture();sampler = LiveRegistryAnchorSampler(f, clock_ns=lambda: 0)
        original = f.read;hits = 0
        new_pov = struct.pack('<3f3if', 400, 500, 600, 7, 8, 9, 90)
        def changing(address, size):
            nonlocal hits
            if (address, size) == (sampler.pov, 28):
                hits += 1
                if hits == 2:
                    f.put(sampler.pov, new_pov)
                    f.put(sampler.stamp, struct.pack('<f', 11))
            return original(address, size)
        with mock.patch.object(f, 'read', side_effect=changing):
            result = sampler.guard()
        attempts = sampler.guard_acquisitions[-1]['attempts']
        self.assertEqual(len(attempts), 2)
        self.assertTrue(attempts[0]['retryable'])
        self.assertFalse(attempts[0]['strict_guard_accepted'])
        self.assertEqual(result['mounted_context']['camera_pov_raw'], new_pov.hex())

    def test_valid_header_change_keeps_registry_classification(self):
        from sample_player_anchor import RegistryHeaderChanged
        for registry in ('objects', 'names'):
            f = MountedFixture(); header = getattr(f, registry + '_header')
            original = f.read; hits = 0
            expected = struct.unpack('<Qii', original(header, 16))
            observed = (expected[0], expected[1] + 1, expected[2] + 1)
            def changing(address, size):
                nonlocal hits
                value = original(address, size)
                if (address, size) == (header, 16):
                    hits += 1
                    if hits >= 2:
                        return struct.pack('<Qii', *observed)
                return value
            with mock.patch.object(f, 'read', side_effect=changing):
                with self.assertRaises(RegistryHeaderChanged) as failure:
                    MountedContextCertificate(f.sampler)
            self.assertEqual(failure.exception.registry_change, dict(registry=registry,
                expected_header=list(expected), observed_header=list(observed)))

    def test_distinct_registered_levels_same_world(self):
        f=MountedFixture().stream_levels()
        proof=MountedContextCertificate(f.sampler).capture()
        self.assertEqual({key:value['address'] for key,value in proof['levels'].items()},f.levels)
        self.assertEqual(len(set(f.levels.values())),5)

    def test_streamed_level_class_default_registry_and_world_rejected(self):
        for kind in ('class','default','unregistered','world','missing'):
            with self.subTest(kind=kind):
                f=MountedFixture().stream_levels();level=f.levels['driver']
                if kind=='class':f.ptr(level+80,0x8000)
                elif kind=='default':f.add(level,'Default__Level',0x7000)
                elif kind=='unregistered':f.ptr(f.objects_data+f.u32(level+8)*8,0)
                elif kind=='world':f.ptr(0x20000+f.WORLD_OFFSET,f.TURRET)
                else:f.ptr(0x20000+64,0xdeadbeef)
                with self.assertRaises((AnchorRejected,RuntimeError)):
                    MountedContextCertificate(f.sampler)

    def test_streamed_level_identity_churn_and_boundary_race_rejected(self):
        for label in ('controlled','driver','controller','camera','world_info'):
            with self.subTest(label=label):
                f=MountedFixture().stream_levels();cert=MountedContextCertificate(f.sampler)
                f.word(f.levels[label]+76,1)
                with self.assertRaises(AnchorRejected):cert.capture()
                f=MountedFixture().stream_levels();fresh=f.sampler.fresh;hits=0
                def changing(address):
                    nonlocal hits
                    result=fresh(address)
                    if address==f.levels[label]:
                        hits+=1
                        if hits>=2:result['number']+=1
                    return result
                with mock.patch.object(f.sampler,'fresh',side_effect=changing):
                    with self.assertRaises(AnchorRejected):MountedContextCertificate(f.sampler)

    def test_inner_window_read_count_and_independent_evidence(self):
        f=MountedFixture();cert=MountedContextCertificate(f.sampler)
        with mock.patch.object(f,'read',wraps=f.read) as reader:proof=cert.capture()
        calls=[(x.args[0],x.args[1]) for x in reader.call_args_list]
        stamps=[i for i,x in enumerate(calls) if x==(f.sampler.stamp,4)]
        window=calls[stamps[0]:stamps[-1]+1]
        # Same fixture previously required 341 reads between camera timestamps.
        self.assertLess(len(window),120);self.assertLess(len(window),len(calls)//3)
        role_ids=proof['validation']['inner_before']['roles']
        self.assertTrue(all(a in role_ids for a,n in window if n==96))
        self.assertEqual(proof['validation']['inner_before'],proof['validation']['inner_after'])
        self.assertIsNot(proof['validation']['inner_before'],proof['validation']['inner_after'])
        self.assertIsNot(proof['validation']['inner_before']['roles'],proof['validation']['inner_after']['roles'])
        self.assertEqual(proof['validation']['outer_before_sha256'],proof['validation']['outer_after_sha256'])
        self.assertFalse(proof['atomic_snapshot'])

    def test_inner_endpoint_and_outer_digest_tampering_rejected(self):
        from mounted_context import mounted_guard_result
        original=MountedContextCertificate(MountedFixture().sampler).capture()
        for kind in ('role','slot_index','link','pov','stamp','outer'):
            with self.subTest(kind=kind):
                proof=copy.deepcopy(original);end=proof['validation']['inner_after']
                if kind=='role':end['roles'][proof['roles']['controller']['address']]['cls']+=8
                elif kind=='slot_index':end['roles'][proof['roles']['controlled']['address']]['internal_index']+=1
                elif kind=='link':end['links']['controlled']['raw']='00'*8
                elif kind=='pov':end['pov_raw']='00'*28
                elif kind=='stamp':end['timestamp_raw']=struct.pack('<f',42).hex()
                else:proof['validation']['outer_after_sha256']='0'*64
                with self.assertRaises(AnchorRejected):mounted_guard_result(proof)

    def test_mounted_sample_uses_actual_inner_pov_and_rotation_not_later_camera(self):
        f=MountedFixture();sampler=LiveRegistryAnchorSampler(f,clock_ns=lambda:0)
        original=f.read;stamps=0;advanced=False
        def reading(address,size):
            nonlocal stamps,advanced
            if (address,size)==(sampler.stamp,4):stamps+=1
            if stamps==3 and size==96 and not advanced:
                advanced=True
                f.put(sampler.stamp,struct.pack('<f',42))
                f.put(sampler.pov,struct.pack('<3f3if',400,500,600,7,8,9,90))
            return original(address,size)
        with mock.patch.object(f,'read',side_effect=reading):sample=sampler.sample()
        self.assertTrue(sample['accepted'],sample.get('rejection'));self.assertTrue(advanced)
        self.assertEqual(sample['sampling_contract'],'single_guard_two_layer_mounted_inner_endpoints')
        self.assertEqual(sample['raw_values']['camera_cache_pov'],sample['guard_before']['mounted_context']['validation']['inner_before']['pov_raw'])
        self.assertNotEqual(sample['raw_values']['camera_cache_pov'],f.read(sampler.pov,28).hex())
        self.assertEqual(len(sample['values']['controller_rotation']),3)
        self.assertEqual(len(sampler.guard_acquisitions[-1]['attempts']),1)
        self.assertEqual(sampler.guard_acquisitions[-1]['stage'],'sample_two_layer')
        self.assertIsNot(sample['guard_before']['pawn'],sample['guard_after']['pawn'])

    def test_inner_pov_and_controller_rotation_changes_reject(self):
        from anchor_config import AnchorSamplingRejected
        for kind in ('pov','rotation'):
            with self.subTest(kind=kind):
                f=MountedFixture();sampler=LiveRegistryAnchorSampler(f,clock_ns=lambda:0)
                original=f.read;stamps=0;changed=False
                def reading(address,size):
                    nonlocal stamps,changed
                    if (address,size)==(sampler.stamp,4):stamps+=1
                    if stamps==2 and not changed:
                        changed=True
                        if kind=='pov':f.put(sampler.pov,struct.pack('<3f3if',400,500,600,7,8,9,90))
                        else:f.put(sampler.controller+sampler.layout['controller_rotation']['offset'],struct.pack('<3i',77,88,99))
                    return original(address,size)
                with mock.patch.object(f,'read',side_effect=reading):
                    with self.assertRaises(AnchorSamplingRejected):sampler.guard()

    def test_two_layer_config_keeps_native_wire4_and_bounded_metadata(self):
        import json
        from anchor_config import build_anchor_config,validate_anchor_config,anchor_ini
        config,metadata=build_anchor_config(MountedFixture())
        validate_anchor_config(config);text=anchor_ini(config)
        self.assertEqual(config['schema'],3);self.assertIn('schema=4\n',text)
        self.assertIn('mounted_enabled=1\n',text)
        self.assertEqual(set(config['mounted']['properties']),
                         {'Pawn','Controller','Driver','DrivenVehicle','PlayerCamera','PCOwner','Player','WorldInfo','InvManager','Owner'})
        self.assertLess(len(json.dumps(metadata).encode()),256*1024)
        self.assertEqual(metadata['sample']['sampling_contract'],'single_guard_two_layer_mounted_inner_endpoints')

    def test_roles_camera_and_independent_results(self):
        f = MountedFixture()
        certificate = MountedContextCertificate(f.sampler)
        result = certificate.capture()
        self.assertEqual(result['roles']['controlled']['address'], f.TURRET)
        self.assertEqual(result['roles']['inventory_pawn']['address'], 0x20000)
        self.assertEqual(result['roles']['camera_target'], result['roles']['controlled'])
        self.assertEqual(bytes.fromhex(result['camera_pov_raw']), f.read(0x30604, 28))
        result['roles']['driver']['address'] = 1
        self.assertEqual(certificate.capture()['roles']['driver']['address'], 0x20000)
        f.put(0x30600, struct.pack('<f', 11))
        f.put(0x30604, struct.pack('<3f3if', 400, 500, 600, 7, 8, 9, 90))
        self.assertEqual(bytes.fromhex(certificate.capture()['camera_pov_raw']), f.read(0x30604, 28))

    def test_reciprocal_links_world_target_and_inventory_rejected(self):
        for address, value in ((0x10224, 0x20000), (0x80800, 0), (0x80250, 0),
                               (0x20810, 0), (0x20250, 0x10000), (0x30500, 0x20000),
                               (0x800d0, 0), (0x200d0, 0), (0x300d0, 0),
                               (0x80040, 0), (0x81040, 0x80000), (0x20820, 0),
                               (0x1037c, 0), (0x30224, 0), (0x10374, 0)):
            with self.subTest(address=hex(address)):
                f = MountedFixture(); f.ptr(address, value)
                with self.assertRaises((AnchorRejected, RuntimeError)):
                    MountedContextCertificate(f.sampler)

    def test_class_property_and_registry_rejected(self):
        for address, value, width in ((0x80050, 0x70000, 8), # Vehicle alone is insufficient.
                                      (0x71060, 0x71000, 8), # Class cycle.
                                      (0x93040, 0x4000, 8), # DrivenVehicle wrong owner.
                                      (0x92074, 4, 4), (0x92094, 0x808, 4),
                                      (0x80008, 0xffffffff, 4)):
            with self.subTest(address=hex(address)):
                f = MountedFixture()
                (f.ptr if width == 8 else f.word)(address, value)
                with self.assertRaises((AnchorRejected, RuntimeError)):
                    MountedContextCertificate(f.sampler)

    def test_mutation_at_final_read_bracket_rejected(self):
        # Link, reflected offset, superclass, target, camera timestamp and POV
        # mutations after discovery must all reject, never use earlier values.
        for target in (0x80800, 0x92094, 0x71060, 0x30500, 0x30600, 0x30604):
            with self.subTest(target=hex(target)):
                f = MountedFixture(); original = f.read; counts = {}
                def changing(address, size):
                    counts[address] = counts.get(address, 0) + 1
                    data = original(address, size)
                    if address == target and counts[address] >= 2:
                        return bytes([data[0] ^ 1]) + data[1:]
                    return data
                with mock.patch.object(f, 'read', side_effect=changing):
                    with self.assertRaises((AnchorRejected, RuntimeError)):
                        MountedContextCertificate(f.sampler)

    def test_role_change_requires_new_certificate(self):
        f = MountedFixture(); certificate = MountedContextCertificate(f.sampler)
        # Consistent remount onto another registered Turret is valid only for a
        # newly prepared certificate, not an in-place change to the first one.
        other = 0xb0000
        f.add(other, 'Turret_1', 0x71000, outer=0x50000)
        for address, value in ((0x10224, other), (other + 0x250, 0x10000),
                               (other + 0x800, 0x20000), (other + f.WORLD_OFFSET, f.WORLD_INFO),
                               (0x20810, other), (0x30500, other)):
            f.ptr(address, value)
        with self.assertRaises(AnchorRejected):certificate.capture()
        self.assertEqual(MountedContextCertificate(f.sampler).capture()['roles']['controlled']['address'], other)

    def test_identity_and_registry_races_rejected(self):
        for kind in ('identity', 'objects', 'names', 'late_timestamp'):
            with self.subTest(kind=kind):
                f = MountedFixture()
                if kind == 'identity':
                    original = f.sampler.fresh; count = 0
                    def fresh(address):
                        nonlocal count
                        value = original(address)
                        if address == f.TURRET:
                            count += 1
                            if count >= 2:value['number'] += 1
                        return value
                    context = mock.patch.object(f.sampler, 'fresh', side_effect=fresh)
                else:
                    target = {'objects': f.objects_header, 'names': f.names_header,
                              'late_timestamp': 0x30600}[kind]
                    original = f.read; count = 0
                    def read(address, size):
                        nonlocal count
                        value = original(address, size)
                        if address == target:
                            count += 1
                            if count >= (3 if kind == 'late_timestamp' else 2):
                                return bytes([value[0] ^ 1]) + value[1:]
                        return value
                    context = mock.patch.object(f, 'read', side_effect=read)
                with context:
                    with self.assertRaises(AnchorRejected):MountedContextCertificate(f.sampler)


if __name__ == '__main__':
    unittest.main()
