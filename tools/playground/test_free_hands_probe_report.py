import unittest

from free_hands_probe_report import pose_row, summarize


def sample(index=0, **replace):
    row = dict(sample=str(index), qpc=str(1000 + index), ms=str(1000 + index),
               generation='2', session='7', sequence=str(index + 1),
               space_epoch='3', input_epoch='4', world_generation='6',
               pose_ms=str(1000 + index), units_per_metre='100', weapon='000003000',
               phase='2', tracked_mask='3',
               eligible_mask='0', apply_mask='0', state_known='15', fixed_mask='0',
               special_move='0', bones_known='1', left='1,2,3',
               left_rotation='.1,.2,.3', right='4,5,6', right_rotation='.4,.5,.6',
               mesh=','.join(['1'] * 16), hand_left=','.join(['2'] * 16),
               hand_right=','.join(['3'] * 16))
    row.update(replace)
    return 'free_hand_pose ' + ' '.join(f'{key}={value}' for key, value in row.items())


def log(rows, status=4):
    return '\n'.join(['free_hand_probe_begin ownership_proved=0 apply_mask=0 writes=0',
                      *rows, f'free_hand_probe_end status={status} reason=fixture '
                      f'samples={len(rows)} writes=0'])


class FreeHandsProbeReportTest(unittest.TestCase):
    def test_bounded_complete_probe_has_no_visible_hand_claim(self):
        result = summarize(log([sample(i) for i in range(160)], 3))
        self.assertTrue(result['completed'])
        self.assertEqual((result['xr_samples'], result['original_bone_samples']), (160, 160))
        self.assertFalse(result['ownership_proved'])
        self.assertFalse(result['visible_hands_proved'])
        self.assertFalse(result['physical_controller_source_proved'])

    def test_missing_xr_and_original_animation_remain_distinct(self):
        result = summarize(log([sample(tracked_mask='0', session='0', sequence='0',
                                        space_epoch='0', input_epoch='0', phase='3', fixed_mask='2')]))
        self.assertEqual(result['xr_samples'], 0)
        self.assertEqual(result['phases'], {'original_animation': 1})

    def test_unsafe_or_malformed_hand_frame_is_rejected(self):
        for replace in ({'apply_mask': '1'}, {'eligible_mask': '3'}, {'phase': '6'},
                        {'tracked_mask': '4'}, {'left': 'nan,0,0'},
                        {'hand_left': '1,2'}, {'session': '0'}, {'bones_known': '2'},
                        {'pose_ms': '899'}, {'pose_ms': '1001'}, {'units_per_metre': 'nan'}):
            with self.subTest(replace=replace), self.assertRaises(ValueError):
                pose_row(sample(**replace))

    def test_incomplete_noncontiguous_or_count_mismatch_rejected(self):
        for text in (sample(), log([sample(1)]), log([sample()], 3),
                     log([sample()]).replace('samples=1', 'samples=2'),
                     log([sample()]).replace('writes=0', 'writes=1')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                summarize(text)

    def test_latest_probe_requires_its_own_complete_envelope(self):
        valid = log([sample()])
        self.assertEqual(summarize(valid + '\n' + valid)['samples'], 1)
        with self.assertRaises(ValueError):
            summarize(valid + '\nfree_hand_probe_begin ownership_proved=0 apply_mask=0 writes=0')


if __name__ == '__main__':
    unittest.main()
