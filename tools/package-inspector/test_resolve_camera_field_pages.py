"""Bounded discovery reads in byte arrays; no process or game is opened."""
import unittest
import struct

from resolve_camera import Resolver,_DiscoveryFieldHintPages,DiscoveryRegistryChanged


def fixture(pages=8,failed=False,short=False):
    memory=bytearray((i%251 for i in range((pages+1)*4096)));calls=[]
    def read(address,size):
        calls.append((address,size))
        if size==4096:
            if failed:raise OSError('page unavailable')
            if short:return bytes(memory[address:address+4095])
        return bytes(memory[address:address+size])
    return _DiscoveryFieldHintPages(read),memory,calls,read


def fields(stride,count=12):
    r=Resolver.__new__(Resolver);memory=bytearray((count+4)*stride+0x4000);calls=[]
    owner=0x100;addresses=[0x1000+i*stride for i in range(count)]
    struct.pack_into('<Q',memory,owner+0x80,addresses[0])
    for index,address in enumerate(addresses):
        struct.pack_into('<I',memory,address+8,index)
        struct.pack_into('<QIIQ',memory,address+64,owner,index,0,0x200)
        struct.pack_into('<Q',memory,address+0x68,addresses[index+1]if index+1<count else 0)
    def read(address,size):calls.append((address,size));return bytes(memory[address:address+size])
    r.read=read;r.ident_cache={};r.names_cache={};r._field_hint_links=None;r.name=lambda index:'Field'+str(index)
    r.profile=dict(object_internal_index=8,object_name=72,object_outer=64,object_class=80,struct_children=0x80,field_next=0x68)
    return r,addresses,calls,memory


class FieldPageTests(unittest.TestCase):
    def test_dense_hint_page_keeps_identical_bytes(self):
        hint,memory,calls,_=fixture()
        addresses=[0x1000+i*0x100 for i in range(12)]
        self.assertEqual([hint.read(address,0x60)for address in addresses],
                         [bytes(memory[address:address+0x60])for address in addresses])
        self.assertEqual(calls,[(0x1000,4096)])

    def test_fields_use_two_exact_headers_before_a_dense_page_and_finish_clears(self):
        r,addresses,calls,_=fields(0x100);r.begin_discovery_field_hints()
        self.assertEqual([item['address']for item in r.fields(0x100)],addresses)
        self.assertEqual([(address,size)for address,size in calls if size!=8],
                         [(addresses[0],0x60),(addresses[1],0x60),(0x1000,4096)])
        self.assertEqual(r.finish_discovery_field_hints(),13);self.assertIsNone(r._field_hint_pages)

    def test_one_or_two_objects_per_page_keep_exact_reads(self):
        for per_page in (1,2):
            r,addresses,calls,_=fields(4096//per_page);r.begin_discovery_field_hints()
            self.assertEqual([item['address']for item in r.fields(0x100)],addresses)
            self.assertEqual([(address,size)for address,size in calls if size!=8],[(address,0x60)for address in addresses])
            self.assertFalse(r._field_hint_pages.cache.pages);r.finish_discovery_field_hints()

    def test_sparse_prefix_disables_tracking_for_remainder_of_the_walk(self):
        r,original,calls,memory=fields(4096,count=48)
        addresses=original[:32]+[0x32000+i*0x100 for i in range(16)]
        for old,new in zip(original[32:],addresses[32:]):memory[new:new+0x80]=memory[old:old+0x80]
        for index,address in enumerate(addresses):struct.pack_into('<Q',memory,address+0x68,addresses[index+1]if index+1<len(addresses)else 0)
        r.begin_discovery_field_hints()
        self.assertEqual([item['address']for item in r.fields(0x100)],addresses)
        self.assertFalse(any(size==4096 for _,size in calls));r.finish_discovery_field_hints()

    def test_iterator_resumed_after_finish_cannot_use_the_cached_page(self):
        r,addresses,calls,memory=fields(0x100);r.begin_discovery_field_hints();walking=r.fields(0x100)
        for _ in range(3):next(walking)
        self.assertTrue(r._field_hint_pages.cache.pages);r.finish_discovery_field_hints()
        struct.pack_into('<I',memory,addresses[3]+76,99)
        self.assertEqual(next(walking)['number'],99)
        self.assertIn((addresses[3],0x60),calls)

    def test_failed_or_short_page_probe_falls_back_once_per_retained_page(self):
        for fault in ('failed','short'):
            hint,memory,calls,_=fixture(**{fault:True})
            addresses=[0x1000+i*0x100 for i in range(8)]
            for address in addresses:self.assertEqual(hint.read(address,0x60),bytes(memory[address:address+0x60]))
            self.assertEqual(sum(size==4096 for _,size in calls),1)
            self.assertEqual([address for address,size in calls if size==0x60],addresses)
            self.assertFalse(hint.cache.pages)

    def test_other_sizes_and_cross_page_headers_do_not_fill_or_warm_pages(self):
        hint,memory,calls,_=fixture()
        for address,size in ((0x1000,8),(0x1ff0,0x60),(0x2000,128)):
            self.assertEqual(hint.read(address,size),bytes(memory[address:address+size]))
        self.assertEqual(calls,[(0x1000,8),(0x1ff0,0x60),(0x2000,128)])
        self.assertFalse(hint.failed);self.assertFalse(hint.cache.pages)

    def test_page_bytes_are_hints_and_independent_exact_reader_remains_current(self):
        hint,memory,calls,read=fixture()
        for offset in (0,0x100,0x200):hint.read(0x1000+offset,0x60)
        before=hint.read(0x1300,0x60);memory[0x1300]^=1
        self.assertEqual(hint.read(0x1300,0x60),before)
        self.assertNotEqual(read(0x1300,0x60),before)
        next_scope=_DiscoveryFieldHintPages(read)
        self.assertEqual(next_scope.read(0x1300,0x60),read(0x1300,0x60))

    def test_both_page_storage_and_sparse_tracking_are_bounded(self):
        for failed in (False,True):
            hint,_,_,_=fixture(failed=failed);hint.cache.MAX_PAGES=2
            for page in range(1,8):
                for offset in (0,0x100,0x200,0x300):hint.read(page*4096+offset,0x60)
                self.assertLessEqual(len(hint.failed),2);self.assertLessEqual(len(hint.cache.pages),2)

    def test_known_registry_change_is_not_replaced_by_a_successful_exact_fallback(self):
        hint,memory,calls,read=fixture()
        def changed(address,size):
            if size==4096:raise DiscoveryRegistryChanged('registry moved')
            return read(address,size)
        hint.cache.read_exact=changed
        with self.assertRaises(DiscoveryRegistryChanged):hint.read(0x1200,0x60)
        self.assertNotIn((0x1200,0x60),calls)


if __name__=='__main__':unittest.main()
