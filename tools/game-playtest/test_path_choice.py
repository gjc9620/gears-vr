import unittest

from path_choice import pick_key


class ChoiceTests(unittest.TestCase):
    def test_current_unique_key_selects_side(self):
        keys = dict(LeftTriggerKeys=['XboxTypeS_LeftTrigger', 'LeftMouseButton'],
                    RightTriggerKeys=['RightMouseButton'], StartKeys=['Enter'])
        self.assertEqual(pick_key(keys, 'left', {'LeftMouseButton', 'RightMouseButton'}), 'LeftMouseButton')
        self.assertEqual(pick_key(keys, 'right', {'LeftMouseButton', 'RightMouseButton'}), 'RightMouseButton')

    def test_ambiguous_or_unsupported_input_is_never_guessed(self):
        keys = dict(LeftTriggerKeys=['Enter', 'Unsupported'], RightTriggerKeys=['Enter'], StartKeys=[])
        with self.assertRaisesRegex(RuntimeError, 'unambiguous'):
            pick_key(keys, 'left', {})


if __name__ == '__main__':
    unittest.main()
