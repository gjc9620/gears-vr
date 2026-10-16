"""Synthetic memory only: mounted possession, identity and race rejection."""
import struct
import unittest
from unittest import mock

from anchor_config import LiveRegistryAnchorSampler, AnchorRejected
from test_cinematic_anchor import CinematicFixture
from mounted_context import MountedContextCertificate


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


class MountedContextTests(unittest.TestCase):
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
