import struct
import unittest
from unittest.mock import Mock

from world_state import streaming_levels


class StreamingEvidenceTests(unittest.TestCase):
    def test_actual_loaded_visible_level_is_retained_when_request_flag_is_false(self):
        q = Mock()
        a, b, level = dict(address=1, cls=11), dict(address=2, cls=12), dict(address=3, cls=13)
        q.array.return_value = (dict(address=9), 8, struct.pack('<2Q', 1, 2), 2)
        q.r.class_name.return_value = 'ObjectProperty'
        q.obj.side_effect = lambda address: {1: a, 2: b}[address]
        q.s.derives.return_value = True
        q.pointer.side_effect = lambda obj, name: None if obj == a else level
        q.prop.side_effect = lambda obj, name, kind: dict(name=name, type=kind, address=100)
        q.value.side_effect = lambda obj, prop: prop['name']
        q.decode.side_effect = lambda kind, raw, address: (
            dict(name='sp_embry_p', number=0) if raw == 'PackageName' else raw != 'bShouldBeLoaded')
        evidence = streaming_levels(q, dict(address=4))
        self.assertEqual(evidence['count'], 2)
        self.assertEqual(len(evidence['loaded']), 1)
        self.assertTrue(evidence['loaded'][0]['bIsVisible'])
        self.assertFalse(evidence['loaded'][0]['bShouldBeLoaded'])
