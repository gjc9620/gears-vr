"""Full independent UI certificate comparison across ordinary and hinted chains."""
import struct
import types
import unittest

from test_ui_observer_config import Fixture
from resolve_camera import Resolver
from ui_observer_config import build_ui_observer_config


def fixture():
    r=Fixture();r._field_hint_links=None;r.FIELD_HINT_LIMIT=Resolver.FIELD_HINT_LIMIT
    for owner,children in r.children.items():
        r.put(owner+r.profile['struct_children'],struct.pack('<Q',children[0] if children else 0))
        for i,address in enumerate(children):
            r.put(address+r.profile['field_next'],struct.pack('<Q',children[i+1] if i+1<len(children) else 0))
    for name in ('fields','class_name','property','begin_discovery_field_hints',
                 'finish_discovery_field_hints','_discovery_field_link'):
        setattr(r,name,types.MethodType(getattr(Resolver,name),r))
    return r


class CertificateTests(unittest.TestCase):
    def test_complete_certificate_is_identical_and_all_hint_links_are_rechecked(self):
        ordinary=fixture();expected,_=build_ui_observer_config(ordinary,ordinary.camera)
        r=fixture();r.begin_discovery_field_hints()
        actual,_=build_ui_observer_config(r,r.camera)
        self.assertEqual(actual,expected)
        self.assertGreater(r.finish_discovery_field_hints(),0)
        self.assertIsNone(r._field_hint_links)


if __name__=='__main__':unittest.main()
