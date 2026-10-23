"""Bounded reflection hints in synthetic memory; no process is opened."""
import struct
import unittest
from unittest import mock

from resolve_camera import Resolver,DiscoveryRegistryChanged


def fixture():
    r=Resolver.__new__(Resolver)
    r.profile=dict(struct_children=0x80,field_next=0x68)
    r._field_hint_links=None
    memory={0x180:0x200,0x268:0x300,0x368:0}
    r.u64=mock.Mock(side_effect=lambda a:memory[a])
    r.identity=mock.Mock(side_effect=lambda a,**kwargs:dict(address=a,outer=0x100,name=str(a)))
    return r,memory


class FieldHintTests(unittest.TestCase):
    def test_repeated_traversals_have_same_results_and_one_final_recheck(self):
        ordinary,_=fixture();expected=[list(ordinary.fields(0x100)) for _ in range(4)]
        r,_=fixture();r.begin_discovery_field_hints()
        self.assertEqual([list(r.fields(0x100)) for _ in range(4)],expected)
        self.assertEqual(r.u64.call_count,3)
        self.assertEqual(r.finish_discovery_field_hints(),3)
        self.assertEqual(r.u64.call_count,6)
        self.assertEqual(ordinary.u64.call_count,12)
        self.assertIsNone(r._field_hint_links)

    def test_changed_head_next_or_tail_rejects_and_clears_scope(self):
        for address in (0x180,0x268,0x368):
            with self.subTest(address=address):
                r,memory=fixture();r.begin_discovery_field_hints();list(r.fields(0x100))
                memory[address]^=0x800
                with self.assertRaises(DiscoveryRegistryChanged):r.finish_discovery_field_hints()
                self.assertIsNone(r._field_hint_links)

    def test_read_error_during_final_check_clears_scope(self):
        r,_=fixture();r.begin_discovery_field_hints();list(r.fields(0x100))
        r.u64.side_effect=OSError('unreadable')
        with self.assertRaises(OSError):r.finish_discovery_field_hints()
        self.assertIsNone(r._field_hint_links)

    def test_next_acquisition_and_ordinary_reads_do_not_reuse_links(self):
        r,memory=fixture();r.begin_discovery_field_hints();list(r.fields(0x100))
        r.finish_discovery_field_hints();memory[0x268]=0
        self.assertEqual(len(list(r.fields(0x100))),1)
        r.begin_discovery_field_hints()
        self.assertEqual(len(list(r.fields(0x100))),1)
        self.assertEqual(r.finish_discovery_field_hints(),2)

    def test_partial_iteration_only_records_links_actually_visited(self):
        r,_=fixture();r.begin_discovery_field_hints()
        fields=r.fields(0x100);self.assertEqual(next(fields)['address'],0x200);fields.close()
        self.assertEqual(r.finish_discovery_field_hints(),1)
        self.assertEqual(r.u64.call_args_list,[mock.call(0x180),mock.call(0x180)])

    def test_capacity_overflow_retains_ordinary_reads_and_checks_saved_prefix(self):
        r,_=fixture();r.FIELD_HINT_LIMIT=1;r.begin_discovery_field_hints()
        list(r.fields(0x100));list(r.fields(0x100))
        self.assertEqual(len(r._field_hint_links),1)
        self.assertEqual(r.u64.call_count,5)
        self.assertEqual(r.finish_discovery_field_hints(),1)

    def test_iterator_resumed_after_scope_uses_ordinary_fresh_links(self):
        r,memory=fixture();r.begin_discovery_field_hints();fields=r.fields(0x100)
        self.assertEqual(next(fields)['address'],0x200)
        r.finish_discovery_field_hints();memory[0x268]=0
        self.assertEqual(list(fields),[])

    def test_cycle_and_owner_guards_remain_active(self):
        for reason in ('cycle','owner'):
            r,memory=fixture();r.begin_discovery_field_hints()
            if reason=='cycle':memory[0x368]=0x200
            else:r.identity.side_effect=lambda a,**kwargs:dict(address=a,outer=0x900)
            with self.subTest(reason=reason),self.assertRaises(RuntimeError):list(r.fields(0x100))

    def test_nested_scope_rejected_without_losing_outer_guard(self):
        r,memory=fixture();r.begin_discovery_field_hints();list(r.fields(0x100))
        with self.assertRaises(RuntimeError):r.begin_discovery_field_hints()
        memory[0x268]=0
        with self.assertRaises(DiscoveryRegistryChanged):r.finish_discovery_field_hints()
        self.assertEqual(r.finish_discovery_field_hints(),0)


if __name__=='__main__':unittest.main()
