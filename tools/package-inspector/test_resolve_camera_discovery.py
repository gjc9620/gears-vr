"""Synthetic memory only; constructing these resolvers never opens a process."""
import struct
import unittest
from unittest import mock
from resolve_camera import Resolver,_DiscoveryHintPages


class DiscoveryHintTests(unittest.TestCase):
    def test_class_hint_avoids_decoding_instance_names_and_shares_scan_pages(self):
        raw=bytearray(4096)
        for address,index,cls in ((0x100,0,0x500),(0x180,1,0x580),(0x500,2,0x580),(0x580,3,0x580)):
            struct.pack_into('<I',raw,address+8,index)
            struct.pack_into('<IIQ',raw,address+72,index,0,cls)
        r=Resolver.__new__(Resolver);r.ident_cache={}
        r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80)
        r.name=mock.Mock(side_effect=lambda i:('Instance','Canvas','Pawn','Class')[i])
        r.read=mock.Mock(side_effect=lambda a,n:bytes(raw[a:a+n]))
        discover=r.discovery_identity_reader()
        self.assertEqual(discover.class_identity(0x100)['name'],'Pawn')
        self.assertEqual(discover.class_identity(0x180)['name'],'Class')
        self.assertEqual(discover(0x180)['name'],'Canvas')
        self.assertNotIn(mock.call(0),r.name.call_args_list)
        self.assertEqual(r.read.call_args_list,[mock.call(0,4096)])
        with self.assertRaisesRegex(RuntimeError,'Null discovery class'):discover.class_identity(0x200)

    def test_camera_class_scan_matches_exact_results_and_rejections_with_fewer_reads(self):
        for outcome in ('found','missing','bad_index'):
            with self.subTest(outcome=outcome):
                raw=bytearray(4096)
                names=('Canvas','WarPlayerCamera','Class','Pawn')
                # Skip a null slot, a numbered camera and a camera whose class
                # is not Class before reaching the valid camera class.
                records=((0x100,1,0,0,0x500),(0x180,2,1,1,0x500),
                         (0x200,3,1,0,0x580),(0x280,4,1,0,0x500),
                         (0x500,5,2,0,0x500),(0x580,6,3,0,0x500))
                for address,index,name,number,cls in records:
                    if outcome=='bad_index' and address==0x180:index=99
                    if outcome=='missing' and address==0x280:number=2
                    struct.pack_into('<I',raw,address+8,index)
                    struct.pack_into('<IIQ',raw,address+72,name,number,cls)
                def resolver():
                    r=Resolver.__new__(Resolver);r.ident_cache={};r.calls=[]
                    r.object_pointers=(0,0x100,0x180,0x200,0x280)
                    r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80)
                    r.name=lambda index:names[index]
                    def read(address,size):
                        r.calls.append((address,size));return bytes(raw[address:address+size])
                    r.read=read
                    return r
                ordinary=resolver();batched=resolver()
                # The reference uses the same predicates with ordinary exact
                # identity reads, matching the scan before batching.
                with mock.patch.object(ordinary,'discovery_identity_reader',return_value=ordinary.identity):
                    if outcome=='found':
                        self.assertEqual(ordinary.find_camera_class(),0x280)
                        self.assertEqual(batched.find_camera_class(),0x280)
                    else:
                        reason='Object registry index mismatch' if outcome=='bad_index' else 'WarPlayerCamera class not loaded'
                        for r in (ordinary,batched):
                            with self.assertRaisesRegex(RuntimeError,reason):r.find_camera_class()
                self.assertEqual(batched.ident_cache,ordinary.ident_cache)
                self.assertLess(len(batched.calls),len(ordinary.calls))
                self.assertEqual(batched.calls[0],(0,4096))

    def test_fixed_memory_equivalence_and_original_path(self):
        raw=bytearray(8192)
        for address,index in ((0x100,0),(0x180,1),(0x1100,2)):
            struct.pack_into('<I',raw,address+8,index)
            struct.pack_into('<Q',raw,address+64,0x1230+index)
            struct.pack_into('<IIQ',raw,address+72,index,index+3,0x4560+index)
        def resolver():
            r=Resolver.__new__(Resolver);r.ident_cache={};r.calls=[]
            r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80)
            r.name=lambda index:('Canvas','WarHUD','Pawn')[index]
            def read(address,size):
                r.calls.append((address,size));return bytes(raw[address:address+size])
            r.read=read
            return r
        ordinary=resolver();batched=resolver();discover=batched.discovery_identity_reader()
        for address in (0x100,0x180,0x1100,0x100):
            self.assertEqual(discover(address),ordinary.identity(address))
        self.assertEqual(ordinary.calls,[(0x100,0x60),(0x180,0x60),(0x1100,0x60)])
        self.assertEqual(batched.calls,[(0,4096),(4096,4096)])
        # Cache hits never consult even an explicitly supplied alternate reader.
        self.assertEqual(batched.identity(0x100,reader=lambda *_:self.fail('cached read')),ordinary.identity(0x100))
        # Each scan gets its own page storage; there is no Resolver page cache.
        batched.ident_cache.clear();batched.discovery_identity_reader()(0x100)
        self.assertEqual(batched.calls[-1],(0,4096));self.assertEqual(len(batched.calls),3)

    def test_unreadable_or_short_neighbor_page_falls_back_without_negative_cache(self):
        for failure in ('runtime','oserror','short'):
            with self.subTest(failure=failure):
                calls=[]
                def read(address,size):
                    calls.append((address,size))
                    if size==4096:
                        if failure=='runtime':raise RuntimeError('unreadable neighbors')
                        if failure=='oserror':raise OSError('unreadable neighbors')
                        return bytes(4000)
                    return bytes([address%251])*size
                pages=_DiscoveryHintPages(read)
                for address in (0x1100,0x1180):self.assertEqual(pages.read(address,0x60),bytes([address%251])*0x60)
                self.assertEqual(calls,[(0x1000,4096),(0x1100,0x60),(0x1000,4096),(0x1180,0x60)])
                self.assertFalse(pages.pages)

    def test_cross_page_and_nonidentity_requests_use_exact_reader(self):
        calls=[]
        def read(address,size):calls.append((address,size));return bytes(size)
        pages=_DiscoveryHintPages(read)
        pages.read(4096-0x50,0x60);pages.read(0x100,8)
        self.assertEqual(calls,[(4096-0x50,0x60),(0x100,8)]);self.assertFalse(pages.pages)

    def test_cache_is_bounded_and_evicts_oldest_page(self):
        calls=[]
        def read(address,size):calls.append((address,size));return bytes(size)
        pages=_DiscoveryHintPages(read)
        for index in range(65):
            pages.read(index*4096+0x100,0x60)
            self.assertLessEqual(len(pages.pages),64)
        self.assertEqual(sum(map(len,pages.pages.values())),64*4096)
        self.assertNotIn(0,pages.pages)
        pages.read(0x100,0x60)
        self.assertEqual(len(calls),66);self.assertEqual(calls[-1],(0,4096))

    def test_failed_exact_fallback_keeps_failure(self):
        def read(address,size):raise RuntimeError('unreadable exact request' if size==0x60 else 'unreadable page')
        with self.assertRaisesRegex(RuntimeError,'unreadable exact request'):
            _DiscoveryHintPages(read).read(0x100,0x60)


if __name__=='__main__':unittest.main()
