import struct
import unittest
from prime_owner import owner_argument,pending_prime,graphics_prime_count


class PrimeOwnerTests(unittest.TestCase):
    def test_menu_lock_inventory_is_explicit(self):
        status='capturing pid=12 lifetime=owner native_hooks_enabled=4 graphics_hooks_enabled=4 scene_hooks_enabled=0'
        self.assertEqual(graphics_prime_count(status,True,True),4)
        with self.assertRaises(RuntimeError):graphics_prime_count(status,True,False)
        with self.assertRaises(RuntimeError):graphics_prime_count(status.replace('=4','=3'),True,True)
        with self.assertRaises(RuntimeError):graphics_prime_count(status,False,True)

    def test_graphics_policy_requires_exact_owned_inventory(self):
        status='capturing pid=12 lifetime=owner native_hooks_enabled=3 graphics_hooks_enabled=3 scene_hooks_enabled=0'
        self.assertEqual(graphics_prime_count(status,True),3)
        with self.assertRaises(RuntimeError):graphics_prime_count(status,False)
        for changed in (status.replace('=3','=2'),status.replace('scene_hooks_enabled=0','scene_hooks_enabled=1'),
                        status.replace('lifetime=owner','seconds=180'),status+'\n'+status):
            with self.assertRaises(RuntimeError):graphics_prime_count(changed,True)
        legacy='capturing pid=12 seconds=180 native_hooks_enabled=0'
        self.assertEqual(graphics_prime_count(legacy),0)
        with self.assertRaises(RuntimeError):graphics_prime_count(legacy,True)

    def test_argument_layout_and_limits(self):
        self.assertEqual(struct.unpack('<IIIIQ',owner_argument(7,2**64-1)),(24,1,7,0,2**64-1))
        for pid,created in ((0,1),(2**32,1),(1,0),(1,2**64)):
            with self.assertRaises(ValueError):owner_argument(pid,created)

    def test_pending_requires_exact_identity(self):
        status='capturing pid=12 lifetime=owner owner_pid=7 owner_creation_time=1000 native_hooks_enabled=0\n'
        self.assertTrue(pending_prime(status,12,7,1000,require_owned=True))
        for pid,owner,created in ((1,7,1000),(12,7,100),(12,70,1000),(12,None,None)):
            self.assertFalse(pending_prime(status,pid,owner,created))
        for suffix in ('stopped reason=cancel','handed_off creates=1','prime_setup_failed'):
            self.assertFalse(pending_prime(status+suffix,12,7,1000))

    def test_legacy_remains_bounded(self):
        status='capturing pid=12 seconds=180 native_hooks_enabled=0\n'
        self.assertTrue(pending_prime(status,12))
        self.assertFalse(pending_prime(status,12,7,1000,require_owned=True))
        self.assertFalse(pending_prime(status.replace('180','1800'),12))


if __name__=='__main__':unittest.main()
