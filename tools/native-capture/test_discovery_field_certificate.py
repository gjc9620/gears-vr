"""Full independent UI certificate comparison across ordinary and hinted chains."""
import struct
import types
import unittest

from test_ui_observer_config import Fixture
from resolve_camera import Resolver,_DiscoveryFieldHintPages
from ui_observer_config import build_ui_observer_config
from unittest import mock
from ui_observer_config import UIConfigRejected


def fixture():
    r=Fixture();r._field_hint_links=None;r.FIELD_HINT_LIMIT=Resolver.FIELD_HINT_LIMIT
    for owner,children in r.children.items():
        r.put(owner+r.profile['struct_children'],struct.pack('<Q',children[0] if children else 0))
        for i,address in enumerate(children):
            r.put(address+r.profile['field_next'],struct.pack('<Q',children[i+1] if i+1<len(children) else 0))
    for name in ('fields','class_name','property','begin_discovery_field_hints',
                 'finish_discovery_field_hints','_discovery_field_link','_discovery_property_fields'):
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
        self.assertIsNone(r._field_hint_properties)
        self.assertIsNone(r._field_hint_pages)

    def test_selected_property_identity_is_rechecked_after_discovery_hints(self):
        r=fixture();r.begin_discovery_field_hints();read=r.read;headers=0
        selected=r.props[r.classes['Canvas'],'ClipX']['address']
        def changing(address,size):
            nonlocal headers
            if address==r.names_header and size==16:
                headers+=1
                if headers==3:r.put(selected+76,struct.pack('<I',1))
            return read(address,size)
        with mock.patch.object(r,'read',side_effect=changing):
            with self.assertRaisesRegex(UIConfigRejected,'UI final identity mismatch'):
                build_ui_observer_config(r,r.camera)

    def test_full_certificate_equal_with_actual_page_hints_and_selected_mutation_rejects(self):
        for mutate in (False,True):
            r=fixture();selected=r.props[r.classes['Canvas'],'ClipX']['address']
            # Fill synthetic allocation padding so page reads can actually
            # succeed. Original fixture leaves unrelated padding unmapped.
            for obj in r.objects:
                page=obj-obj%4096
                for address in range(page,page+4096):r.memory.setdefault(address,0)
            expected,_=build_ui_observer_config(r,r.camera)
            read=r.read;headers=0
            def changing(address,size):
                nonlocal headers
                if address==r.names_header and size==16:
                    headers+=1
                    if mutate and headers==3:r.put(selected+76,struct.pack('<I',1))
                return read(address,size)
            with mock.patch.object(r,'read',side_effect=changing),\
                 mock.patch.object(_DiscoveryFieldHintPages,'WARMUP_READS',0):
                r.begin_discovery_field_hints()
                if mutate:
                    with self.assertRaisesRegex(UIConfigRejected,'UI final identity mismatch'):
                        build_ui_observer_config(r,r.camera)
                else:
                    actual,_=build_ui_observer_config(r,r.camera);self.assertEqual(actual,expected)
                self.assertIn(selected-selected%4096,r._field_hint_pages.cache.pages)
                r.finish_discovery_field_hints()
                self.assertIsNone(r._field_hint_pages)


if __name__=='__main__':unittest.main()
