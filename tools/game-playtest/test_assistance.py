from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import Mock, patch

from assistance import maintain_health
from live import InputNotReady
from test_core import sample
import test_live_input


class HealthAssistanceTests(unittest.TestCase):
    def fixture(self):
        observer = test_live_input.InputTests().fixture()
        row = sample(0)
        row['pawn_identity']['cls'] = 2
        observer.sample.return_value = row
        observer.sampler = Mock(controller=10, pawn=1000)
        q = Mock()
        pawn, controller, world = row['pawn_identity'], dict(address=10), dict(address=9)
        q.obj.side_effect = lambda address: {10: controller, 1000: pawn}[address]
        q.pointer.side_effect = lambda obj, name: {
            (1000, 'Controller'): controller, (10, 'Pawn'): pawn,
            (1000, 'WorldInfo'): world, (10, 'WorldInfo'): world}[(obj['address'], name)]
        q.prop.return_value = dict(size=4, offset=128, type='IntProperty', name='Health')
        q.scalar.side_effect = [100, 9999]
        q.s.derives.return_value = True
        report = Mock(directory=Path(tempfile.gettempdir()) / 'gearsvr-unittest-no-health-stop', provenance={})
        writers = Mock(return_value=Mock())
        return observer, report, row, q, writers

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_health_write_is_owned_typed_verified_closed_and_separately_marked(self, background):
        observer, report, row, q, writers = self.fixture()
        with patch('assistance.Query', return_value=q):
            receipt = maintain_health(observer, report, row, writer_factory=writers)
        writers.return_value.write.assert_called_once_with(1128, struct.pack('<i', 9999))
        writers.return_value.close.assert_called_once()
        self.assertEqual(receipt['health_after'], 9999)
        self.assertIs(receipt['survival_acceptance'], False)
        self.assertEqual(report.provenance['health_assistance']['writes'], 1)
        self.assertIs(report.assisted, True)

    def test_passive_or_dead_context_never_opens_writer(self):
        observer, report, row, q, writers = self.fixture()
        observer.action_owner = None
        with self.assertRaisesRegex(RuntimeError, 'owned launcher'):
            maintain_health(observer, report, row, writer_factory=writers)
        observer.action_owner = observer.identities['owner_identity']
        row['health'] = 0
        with self.assertRaises(InputNotReady):
            maintain_health(observer, report, row, writer_factory=writers)
        writers.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_foreground_context_never_opens_writer(self, background):
        observer, report, row, q, writers = self.fixture()
        observer.desktop.return_value = dict(occlusion=[], foreground=2)
        with self.assertRaises(InputNotReady):
            maintain_health(observer, report, row, writer_factory=writers)
        writers.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_readback_failure_still_closes_handle_and_preserves_attempt_marker(self, background):
        observer, report, row, q, writers = self.fixture()
        q.scalar.side_effect = [100, RuntimeError('readback changed')]
        with patch('assistance.Query', return_value=q):
            with self.assertRaisesRegex(RuntimeError, 'readback changed'):
                maintain_health(observer, report, row, writer_factory=writers)
        writers.return_value.close.assert_called_once()
        self.assertIs(report.assisted, True)
        self.assertEqual(report.provenance['health_assistance']['attempts'], 1)
        self.assertEqual(report.provenance['health_assistance']['writes'], 0)
