import unittest

from combat import engagement_window
from test_core import sample


class EngagementBudgetTests(unittest.TestCase):
    def test_continuous_combat_yields_a_navigation_interval(self):
        attempts, world = {}, dict(source_sample=sample(0))
        for now in (0, 5, 10):
            self.assertEqual(engagement_window(attempts, world, now), (True, False))
        self.assertEqual(engagement_window(attempts, world, 12), (False, True))
        self.assertEqual(engagement_window(attempts, world, 15), (False, False))
        self.assertEqual(engagement_window(attempts, world, 20), (True, False))

    def test_walking_gap_or_new_possession_does_not_inherit_old_combat_time(self):
        attempts, world = {}, dict(source_sample=sample(0))
        engagement_window(attempts, world, 0)
        self.assertEqual(engagement_window(attempts, world, 10), (True, False))
        for now in (15, 20):
            self.assertEqual(engagement_window(attempts, world, now), (True, False))
        self.assertEqual(engagement_window(attempts, world, 22), (False, True))
        world['source_sample']['pawn_identity'] = dict(address=2000, internal_index=5)
        self.assertEqual(engagement_window(attempts, world, 23), (True, False))
