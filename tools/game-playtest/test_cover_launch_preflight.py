import unittest

import cover_launch_preflight as gate


def row(pid, name='python.exe', command='offline_review.py'):
    return dict(ProcessId=pid, Name=name, CommandLine=command)


class StartupCheckTest(unittest.TestCase):
    def test_offline_media_does_not_hold_game(self):
        rows = [row(10, 'ffmpeg.exe', 'solo-m1-20261028/output.mp4'),
                row(11, command='review_paired_windows.py'),
                row(12, command='retain_test_media.py')]
        proof = gate.check(99, inventory=lambda: rows, mutex=lambda _: False)
        self.assertTrue(proof['runtime_idle_observed'])
        self.assertFalse(proof['runtime_window_owned'])
        self.assertFalse(proof['reservation_created'])
        self.assertFalse(proof['atomic_exclusion_proven'])

    def test_each_runtime_blocks_even_without_command(self):
        for name in gate.RUNTIME_NAMES:
            with self.subTest(name=name), self.assertRaises(gate.Occupied):
                gate.check(99, inventory=lambda: [row(10, name, None)], mutex=lambda _: False)

    def test_each_existing_runner_blocks(self):
        for script in gate.RUNNERS:
            with self.subTest(script=script), self.assertRaises(gate.Occupied):
                gate.check(99, inventory=lambda: [row(10, command='python ' + script.upper())], mutex=lambda _: False)

    def test_own_process_is_only_pid_exception(self):
        rows = [row(99, command='run_basic.py'), row(100, command='run_basic.py')]
        self.assertEqual([r['ProcessId'] for r in gate.blocking_processes(rows, 99)], [100])

    def test_unknown_python_command_blocks(self):
        with self.assertRaises(gate.Occupied):
            gate.check(99, inventory=lambda: [row(10, command=None)], mutex=lambda _: False)

    def test_mutex_before_inventory_rejects_without_reading_processes(self):
        def forbidden():
            self.fail('No need to read processes once an owner is found')
        with self.assertRaises(gate.Occupied):
            gate.check(99, inventory=forbidden, mutex=lambda _: True)

    def test_mutex_appearing_during_inventory_rejects(self):
        calls = []
        def mutex(name):
            calls.append(name)
            return len(calls) > len(gate.OWNER_LOCKS)
        with self.assertRaises(gate.Occupied):
            gate.check(99, inventory=lambda: [], mutex=mutex)

    def test_query_errors_are_not_idle(self):
        def failed(*_):
            raise OSError('Inventory or mutex inaccessible')
        for inventory, mutex in ((failed, lambda _: False), (lambda: [], failed)):
            with self.assertRaises(OSError):
                gate.check(99, inventory=inventory, mutex=mutex)

    def test_malformed_inventory_never_proves_idle(self):
        for rows in (None, {}, [None], [row(True)], [row(0)],
                     [row(10), row(10)], [dict(ProcessId=10)],
                     [row(10, command=42)]):
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                gate.check(99, inventory=lambda: rows, mutex=lambda _: False)


if __name__ == '__main__':
    unittest.main()
