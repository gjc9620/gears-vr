import unittest
from window_snapshot import WindowSnapshot


class FakeDesktop:
    def __init__(self):
        self.orders = [[10, 20], [10, 20]]
        self.order_index = -1
        self.fail_attribute = None
        self.pid_reads = 0
        self.change_identity = False

    def GetTopWindow(self, _):
        self.order_index += 1
        return self.orders[min(self.order_index, len(self.orders) - 1)][0]

    def GetWindow(self, hwnd, direction):
        assert direction == 2
        order = self.orders[min(self.order_index, len(self.orders) - 1)]
        index = order.index(hwnd) + 1
        return order[index] if index < len(order) else 0

    def GetWindowThreadProcessId(self, hwnd, pid):
        self.pid_reads += 1
        pid._obj.value = hwnd + (1 if self.change_identity and self.pid_reads == 2 else 0)
        return 3

    def IsWindowVisible(self, hwnd): return True
    def IsIconic(self, hwnd): return False
    def GetShellWindow(self): return 0
    def GetDesktopWindow(self): return 0

    def DwmGetWindowAttribute(self, hwnd, attribute, out, size):
        if attribute == self.fail_attribute:
            return -1
        if attribute == 14:
            out._obj.value = 0
        else:
            out._obj.left, out._obj.top = -100, 0
            out._obj.right, out._obj.bottom = 0, 100
        return 0


class SnapshotTests(unittest.TestCase):
    def make(self):
        sampler = WindowSnapshot.__new__(WindowSnapshot)
        sampler.u = sampler.d = FakeDesktop()
        return sampler

    def test_reads_physical_bounds_without_window_mutation(self):
        sampler = self.make()
        result = sampler.capture()
        self.assertEqual([x['hwnd'] for x in result['windows']], [10, 20])
        self.assertEqual(result['windows'][0]['rect'], (-100, 0, 0, 100))
        self.assertFalse(result['atomic'])

    def test_reorder_is_not_a_pass(self):
        sampler = self.make()
        sampler.u.orders[1] = [20, 10]
        with self.assertRaisesRegex(RuntimeError, 'order changed'):
            sampler.capture()

    def test_dwm_failure_is_not_a_pass(self):
        for attribute in (9, 14):
            sampler = self.make()
            sampler.u.fail_attribute = attribute
            with self.assertRaises(RuntimeError):
                sampler.capture()

    def test_identity_race_is_not_a_pass(self):
        sampler = self.make()
        sampler.u.change_identity = True
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            sampler.capture()

    def test_enumeration_bounded(self):
        with self.assertRaisesRegex(RuntimeError, 'snapshot limit'):
            self.make().capture(limit=1)

    def test_many_hidden_windows_do_not_exhaust_visible_budget(self):
        sampler = self.make()
        sampler.u.orders = [list(range(10, 647))] * 2
        sampler.u.IsWindowVisible = lambda hwnd: hwnd < 50
        result = sampler.capture()
        self.assertEqual(result['traversed_windows'], 637)
        self.assertEqual(len(result['windows']), 40)

    def test_visible_budget_independently_bounded(self):
        with self.assertRaisesRegex(RuntimeError, 'Visible windows'):
            self.make().capture(visible_limit=1)

    def test_cycle_has_distinct_error(self):
        sampler = self.make()
        sampler.u.GetWindow = lambda hwnd, direction: hwnd
        with self.assertRaisesRegex(RuntimeError, 'cycle'):
            sampler.capture()


if __name__ == '__main__':
    unittest.main()
