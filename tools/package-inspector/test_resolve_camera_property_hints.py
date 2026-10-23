"""Attempt-local property discovery; synthetic identities and links only."""
import unittest
from unittest import mock

from resolve_camera import Resolver,DiscoveryRegistryChanged


def fixture(groups):
    r=Resolver.__new__(Resolver)
    r.profile=dict(struct_children=0x80,field_next=0x68,field_super=0x60,property_offset_internal=0x94)
    r._field_hint_links=None;r._field_hint_properties=None
    kinds={name:0x900000+i*0x100 for i,name in enumerate(('IntProperty','FloatProperty','Function'))}
    objects={address:dict(address=address,name=name)for name,address in kinds.items()}
    memory={};offsets={};selected={};next_address=0x100000
    for owner,parent,values in groups:
        addresses=[];memory[owner+0x60]=parent
        for name,kind,offset in values:
            address=next_address;next_address+=0x100;addresses.append(address)
            objects[address]=dict(address=address,outer=owner,name=name,cls=kinds[kind])
            offsets[address+0x94]=offset;selected.setdefault((owner,name),[]).append(address)
        memory[owner+0x80]=addresses[0]if addresses else 0
        for index,address in enumerate(addresses):memory[address+0x68]=addresses[index+1]if index+1<len(addresses)else 0
    r.u64=mock.Mock(side_effect=lambda address:memory[address])
    r.u32=mock.Mock(side_effect=lambda address:offsets[address])
    r.identity=mock.Mock(side_effect=lambda address,**kwargs:objects[address])
    return r,memory,offsets,selected


class PropertyHintTests(unittest.TestCase):
    def test_visited_prefix_avoids_retraversal_but_offsets_are_read_each_time(self):
        r,memory,offsets,selected=fixture([(0x100,0,[('First','IntProperty',4),('Last','FloatProperty',8)])])
        r.begin_discovery_field_hints()
        with mock.patch.object(r,'fields',wraps=r.fields)as fields:
            self.assertEqual(r.property(0x100,'Last')['offset'],8)
            address=selected[0x100,'First'][0];offsets[address+0x94]=12
            self.assertEqual(r.property(0x100,'First')['offset'],12)
            self.assertEqual(r.property(0x100,'First')['address'],address)
            self.assertEqual(fields.call_count,1)
            offsets[address+0x94]=1024*1024+1
            with self.assertRaisesRegex(RuntimeError,'Implausible property offset'):r.property(0x100,'First')
        self.assertEqual(r.u32.call_count,4)
        self.assertEqual(r.finish_discovery_field_hints(),2)
        self.assertIsNone(r._field_hint_properties)

    def test_first_property_and_nonproperty_namesake_keep_ordered_selection(self):
        for first_kind in ('IntProperty','Function'):
            r,_,_,selected=fixture([(0x100,0,[('Dup',first_kind,4),('Dup','FloatProperty',8),('Last','IntProperty',12)])])
            r.begin_discovery_field_hints();r.property(0x100,'Last')
            expected=selected[0x100,'Dup'][int(first_kind=='Function')]
            self.assertEqual(r.property(0x100,'Dup')['address'],expected)
            self.assertEqual(r.property(0x100,'Dup')['address'],expected)
            r.finish_discovery_field_hints()

    def test_owner_scopes_and_inheritance_keep_child_precedence(self):
        r,_,_,_=fixture([(0x100,0x200,[('Shared','IntProperty',4)]),
                        (0x200,0,[('Shared','FloatProperty',8),('ParentOnly','IntProperty',12)])])
        r.begin_discovery_field_hints()
        self.assertEqual(r.property(0x100,'ParentOnly',True)['offset'],12)
        self.assertEqual(r.property(0x100,'Shared',True)['offset'],4)
        self.assertEqual(r.property(0x200,'Shared')['offset'],8)
        with self.assertRaisesRegex(RuntimeError,'not found'):r.property(0x100,'ParentOnly')
        r.finish_discovery_field_hints()

    def test_early_match_does_not_read_unvisited_tail(self):
        r,memory,_,selected=fixture([(0x100,0,[('First','IntProperty',4)])])
        memory[selected[0x100,'First'][0]+0x68]=0xdead
        r.begin_discovery_field_hints();r.property(0x100,'First');r.property(0x100,'First')
        self.assertEqual(r.u64.call_args_list,[mock.call(0x180)])
        self.assertEqual(r.finish_discovery_field_hints(),1)

    def test_changed_saved_links_reject_and_clear_all_hints(self):
        for changed in ('head','next'):
            r,memory,_,selected=fixture([(0x100,0,[('First','IntProperty',4),('Last','FloatProperty',8)])])
            r.begin_discovery_field_hints();r.property(0x100,'Last');r.property(0x100,'First')
            address=0x180 if changed=='head'else selected[0x100,'First'][0]+0x68
            memory[address]=0
            with self.assertRaises(DiscoveryRegistryChanged):r.finish_discovery_field_hints()
            self.assertIsNone(r._field_hint_properties);self.assertIsNone(r._field_hint_links)

    def test_capacity_falls_back_and_next_scope_cannot_reuse_old_locations(self):
        r,memory,_,selected=fixture([(0x100,0,[('First','IntProperty',4),('Last','FloatProperty',8)])])
        r.FIELD_HINT_LIMIT=1;r.begin_discovery_field_hints()
        with mock.patch.object(r,'fields',wraps=r.fields)as fields:
            self.assertEqual(r.property(0x100,'Last')['offset'],8)
            self.assertEqual(r.property(0x100,'First')['offset'],4)
            self.assertGreaterEqual(fields.call_count,2)
        self.assertIsNone(r._field_hint_properties);r.finish_discovery_field_hints()
        r.FIELD_HINT_LIMIT=65536;r.begin_discovery_field_hints();r.property(0x100,'Last');r.finish_discovery_field_hints()
        memory[0x180]=selected[0x100,'Last'][0]
        with self.assertRaisesRegex(RuntimeError,'not found'):r.property(0x100,'First')
        r.begin_discovery_field_hints()
        with self.assertRaisesRegex(RuntimeError,'not found'):r.property(0x100,'First')
        r.finish_discovery_field_hints()

    def test_late_cycle_or_wrong_owner_does_not_become_a_cached_absence(self):
        for fault in ('cycle','owner'):
            r,memory,_,selected=fixture([(0x100,0,[('First','IntProperty',4),('Last','FloatProperty',8)])])
            if fault=='cycle':memory[selected[0x100,'Last'][0]+0x68]=selected[0x100,'First'][0]
            else:r.identity.side_effect(selected[0x100,'Last'][0])['outer']=0x999
            r.begin_discovery_field_hints();r.property(0x100,'First')
            for _ in range(2):
                with self.assertRaisesRegex(RuntimeError,'Invalid reflected field chain'if fault=='cycle'else'Reflected field owner mismatch'):
                    r.property(0x100,'Missing')
                self.assertIsNone(r._field_hint_properties)
            r.finish_discovery_field_hints()

    def test_final_read_error_discards_pending_walk_and_new_scope_starts_fresh(self):
        r,memory,_,_=fixture([(0x100,0,[('First','IntProperty',4),('Last','FloatProperty',8)])])
        r.begin_discovery_field_hints();r.property(0x100,'First');reader=r.u64.side_effect
        r.u64.side_effect=OSError('unreadable final chain')
        with self.assertRaises(OSError):r.finish_discovery_field_hints()
        self.assertIsNone(r._field_hint_properties);self.assertEqual(r._field_hint_property_count,0)
        r.u64.side_effect=reader;r.begin_discovery_field_hints()
        self.assertEqual(r.property(0x100,'Last')['offset'],8);r.finish_discovery_field_hints()


if __name__=='__main__':unittest.main()
