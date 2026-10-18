import unittest
from window_occlusion import find_occlusion


def window(hwnd, pid, rect, **kw):
    return dict(hwnd=hwnd, pid=pid, rect=rect, visible=kw.get('visible', True),
                minimized=kw.get('minimized', False), cloaked=kw.get('cloaked', False))


class OcclusionTests(unittest.TestCase):
    def test_secondary_monitor_below_foreground(self):
        rows = [window(1, 1, (0, 0, 100, 100)), window(2, 9, (-100, 0, 0, 100)),
                window(3, 2, (-100, 0, 0, 100))]
        self.assertEqual(find_occlusion(rows, {9})[0]['covered_hwnd'], 3)

    def test_test_window_fully_covered(self):
        rows = [window(1, 1, (0, 0, 100, 100)), window(2, 9, (0, 0, 100, 100)),
                window(3, 2, (0, 0, 100, 100))]
        self.assertEqual(find_occlusion(rows, {9}), [])

    def test_partial_foreground_cover(self):
        rows = [window(1, 1, (0, 0, 50, 100)), window(2, 9, (0, 0, 100, 100)),
                window(3, 2, (0, 0, 100, 100))]
        self.assertEqual(find_occlusion(rows, {9})[0]['overlap_rects'], [(50, 0, 100, 100)])

    def test_hidden_minimized_cloaked(self):
        for flag in ('visible', 'minimized', 'cloaked'):
            rows = [window(2, 9, (0, 0, 100, 100), **{flag: flag != 'visible'}),
                    window(3, 2, (0, 0, 100, 100))]
            self.assertEqual(find_occlusion(rows, {9}), [])

    def test_behind_all_or_nonoverlapping(self):
        rows = [window(3, 2, (0, 0, 100, 100)), window(2, 9, (0, 0, 100, 100))]
        self.assertEqual(find_occlusion(rows, {9}), [])
        self.assertEqual(find_occlusion(list(reversed(rows)), {9}, {3}), [])
        rows[0]['rect'] = (100, 0, 200, 100)
        self.assertEqual(find_occlusion(list(reversed(rows)), {9}), [])


if __name__ == '__main__':
    unittest.main()
