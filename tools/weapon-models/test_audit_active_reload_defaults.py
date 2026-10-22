import struct
import unittest

from audit_active_reload_defaults import decode_super_sweet_durations


def tag(values):
    payload = struct.pack('<I', len(values)) + struct.pack('<' + 'f' * len(values), *values)
    return {'type': 'StructProperty', 'value': {'Value': {'hex': payload.hex()}}}


class ReloadDurationArrayTest(unittest.TestCase):
    def test_decodes_three_difficulty_values(self):
        result = decode_super_sweet_durations(tag([.1, .075, .05]))
        self.assertEqual(len(result), 3)
        for actual, expected in zip(result, [.1, .075, .05]):
            self.assertAlmostEqual(actual, expected)

    def test_rejects_count_mismatch_and_nonpositive_duration(self):
        bad_count = tag([.1])
        bad_count['value']['Value']['hex'] = '03000000' + bad_count['value']['Value']['hex'][8:]
        with self.assertRaises(ValueError):
            decode_super_sweet_durations(bad_count)
        with self.assertRaises(ValueError):
            decode_super_sweet_durations(tag([0.0]))


if __name__ == '__main__':
    unittest.main()
