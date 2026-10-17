"""Synthetic memory only: mounted possession, identity and race rejection."""
import struct
import unittest
from unittest import mock

from test_cinematic_anchor import CinematicFixture
from anchor_config import LiveRegistryAnchorSampler, AnchorRejected
from mounted_context import MountedContextCertificate, MountedTimestampChanged


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
        self.add(0x74000, 'WarInventoryManager', 0x1000, parent=0x8000)
        self.ptr(0x5000 + 96, 0x72000)
        self.ptr(0x9000 + 96, 0x73000)
        self.add(self.TURRET, 'Turret_0', 0x71000, outer=0x50000)
        self.add(self.INVENTORY, 'Inventory_0', 0x74000, outer=0x20000)
        for actor in (0x10000, 0x30000):self.ptr(actor + 64, 0x50000)
        for index, (owner, name, offset) in enumerate((
                (0x6000, 'Pawn', 0x224), (0x3000, 'Controller', 0x250),
                (0x70000, 'Driver', 0x800), (0x3000, 'DrivenVehicle', 0x810),
                (0x72000, 'PlayerCamera', 0x37c), (0x73000, 'PCOwner', 0x224),
                (0x72000, 'Player', 0x374), (0x3000, 'InvManager', 0x820))):
            address = 0x90000 + index * 0x1000
            self.add(address, name, 0x69000, outer=owner)
            self.properties[owner, name] = dict(address=address, offset=offset, type='ObjectProperty')
            self.word(address + 0x70, 1)
            self.word(address + 0x74, 8)
            self.word(address + 0x94, offset)
        for address, value in ((0x10224, self.TURRET), (self.TURRET + 0x250, 0x10000),
                               (self.TURRET + 0x800, 0x20000), (0x20810, self.TURRET),
                               (0x20250, 0), (0x20820, self.INVENTORY),
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
        return self


class MountedContextTests(unittest.TestCase):
    def test_capture_local_reuse_still_revalidates_every_identity(self):
        f = MountedFixture()
        with mock.patch.object(f.sampler, 'fresh', wraps=f.sampler.fresh) as fresh:
            cert = MountedContextCertificate(f.sampler)
        addresses = [call.args[0] for call in fresh.call_args_list]
        self.assertEqual(set(addresses), set(cert._initial['identities']))
        self.assertTrue(all(addresses.count(address) == 2 for address in set(addresses)))
        with mock.patch.object(f.sampler, 'fresh', wraps=f.sampler.fresh) as fresh:
            self.assertEqual(cert.capture(), cert._initial)
        self.assertEqual(fresh.call_count, len(addresses))

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
        for target in (0x80800, 0x92094, 0x71060, 0x30604, 0x80000):
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
