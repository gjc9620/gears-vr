import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

from axis_input import Slot


class NeutralCertificateTests(unittest.TestCase):
    def fixture(self):
        slot = Slot.__new__(Slot)
        slot.axis = 'aBaseY'
        slot.controller, slot.input, slot.pawn = [dict(address=i) for i in (1, 2, 3)]
        slot.prop, slot.rotation = dict(address=10), dict(address=11)
        slot.rotation_type = dict(address=4)
        slot.components = {name: dict(address=i) for i, name in enumerate(('Pitch', 'Yaw', 'Roll'), 20)}
        slot.q = object()
        o = SimpleNamespace(resolver=None, sampler=None, verify=Mock())
        def reacquire():
            o.resolver, o.sampler = object(), object()
        o.sample = Mock(side_effect=reacquire)
        slot.observer = o
        q = Mock()
        q.obj.side_effect = lambda address: {1: slot.controller, 2: slot.input, 4: slot.rotation_type}[address]
        q.pointer.side_effect = lambda obj, name: slot.input if name == 'PlayerInput' else dict(address=999)
        q.prop.side_effect = lambda obj, name, kind, **kw: (
            slot.prop if name == slot.axis else slot.rotation if name == 'Rotation' else slot.components[name])
        q.r.struct_type.return_value = 4
        return slot, o, q

    def test_release_reacquires_then_certifies_old_slot_without_requiring_old_pawn(self):
        slot, o, q = self.fixture()
        with patch('axis_input.Query', return_value=q):
            slot.validate(neutral=True)
        o.sample.assert_called_once()
        q.finish.assert_called_once_with(allow_registry_growth=True)
        self.assertIs(slot.q, q)

    def test_reacquisition_never_accepts_a_replaced_input_object(self):
        slot, o, q = self.fixture()
        q.pointer.side_effect = lambda obj, name: dict(address=777)
        old = slot.q
        with patch('axis_input.Query', return_value=q):
            with self.assertRaisesRegex(RuntimeError, 'identity or schema changed'):
                slot.validate(neutral=True)
        self.assertIs(slot.q, old)

    def test_new_input_cannot_use_release_only_reacquisition(self):
        slot, o, q = self.fixture()
        with self.assertRaisesRegex(RuntimeError, 'certificate is no longer available'):
            slot.validate()
        o.sample.assert_not_called()
