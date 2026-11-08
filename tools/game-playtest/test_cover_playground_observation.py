"""Empty-UI admission only; no game process, engine call or input authority."""
import unittest
from cover_playground_observation import empty_playground


class EmptyPlaygroundAdmission(unittest.TestCase):
    def setUp(self):
        self.good = dict(consistent_reread=True, read_only=True, input_block_count=0,
                         top=None, active_scenes=[])

    def test_certified_empty_menu(self):
        self.assertTrue(empty_playground(self.good))

    def test_unknown_or_incomplete_menu_rejected(self):
        for key in self.good:
            with self.subTest(missing=key):
                candidate = dict(self.good)
                del candidate[key]
                self.assertFalse(empty_playground(candidate))

    def test_active_or_untrusted_menu_rejected(self):
        for changed in (dict(consistent_reread=False), dict(read_only=False),
                        dict(input_block_count=1), dict(top={}), dict(active_scenes=[{}])):
            with self.subTest(changed=changed):
                self.assertFalse(empty_playground(dict(self.good, **changed)))


if __name__ == '__main__':
    unittest.main()
