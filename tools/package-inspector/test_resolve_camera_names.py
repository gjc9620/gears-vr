"""Synthetic name memory; no process is opened."""
import struct
import unittest
from resolve_camera import Resolver,DiscoveryRegistryChanged


class NameReadTests(unittest.TestCase):
    def resolver(self, address, text, *, readable_end=None, index=0):
        r=Resolver.__new__(Resolver)
        r.names_cache={};r.name_count=1;r.name_pointers=(address-20,)
        r.profile=dict(name_entry_index=0,name_entry_text=20)
        r.calls=[]
        def read(current,size):
            r.calls.append((current,size))
            if current==address-20 and size==4:return struct.pack('<I',index)
            if readable_end is not None and current+size>readable_end:
                raise RuntimeError('Unreadable synthetic name page')
            offset=current-address
            return (text+bytes(128))[offset:offset+size]
        r.read=read
        return r

    def test_terminated_name_before_unreadable_neighbor_and_cache(self):
        r=self.resolver(0x1f4bffb4,'StateStack\0'.encode('utf-16-le'),readable_end=0x1f4c0000)
        self.assertEqual(r.name(0),'StateStack')
        self.assertEqual(r.calls,[(0x1f4bffa0,4),(0x1f4bffb4,76)])
        self.assertEqual(r.name(0),'StateStack');self.assertEqual(len(r.calls),2)

    def test_name_continues_across_readable_page(self):
        r=self.resolver(0x1ffe,'AB\0'.encode('utf-16-le'))
        self.assertEqual(r.name(0),'AB')
        self.assertEqual(r.calls[1:],[(0x1ffe,2),(0x2000,126)])

    def test_surrogate_pair_crosses_page_and_final_bounded_terminator(self):
        r=self.resolver(0x1ffe,'\U0001f600\0'.encode('utf-16-le'))
        self.assertEqual(r.name(0),'\U0001f600')
        r=self.resolver(0x1ffe,('A'*63+'\0').encode('utf-16-le'))
        self.assertEqual(r.name(0),'A'*63)
        self.assertEqual(sum(size for _,size in r.calls[1:]),128)

    def test_required_unreadable_page_propagates_without_cache(self):
        r=self.resolver(0x1ffe,'AB\0'.encode('utf-16-le'),readable_end=0x2000)
        with self.assertRaisesRegex(RuntimeError,'Unreadable synthetic'):r.name(0)
        self.assertEqual(r.names_cache,{})

    def test_unterminated_bound_and_invalid_utf16_remain_rejected(self):
        for text,error in ((b'A\0'*64,RuntimeError),(b'\x00\xd8\0\0',UnicodeDecodeError)):
            with self.subTest(error=error):
                r=self.resolver(0x1ffe,text)
                with self.assertRaises(error):r.name(0)
                self.assertEqual(r.names_cache,{})

    def test_index_validation_precedes_text_read(self):
        r=self.resolver(0x1ffe,b'\0\0',index=1)
        with self.assertRaisesRegex(RuntimeError,'Name entry index mismatch'):r.name(0)
        self.assertEqual(r.calls,[(0x1fea,4)])

    def grown_resolver(self, *, count=1002, entry_index=1001, text='NewName\0'):
        r=self.resolver(0x4014,text.encode('utf-16-le'),index=entry_index)
        r.names_header=0x100;r.name_count=count # Live anchor guard changed it.
        header=struct.pack('<Qii',0x8000,count,count)
        ordinary=r.read
        def read(address,size):
            if address==r.names_header and size==16:return header
            if address==0x8000+1001*8 and size==8:return struct.pack('<Q',0x4000)
            return ordinary(address,size)
        r.read=read
        return r

    def test_grown_live_bounds_do_not_index_original_snapshot(self):
        r=self.grown_resolver()
        self.assertEqual(r.name(1001),'NewName')
        self.assertEqual(len(r.name_pointers),1)
        # Even an unchanged stale live count does not hide a valid new slot.
        r=self.grown_resolver();r.name_count=1
        self.assertEqual(r.name(1001),'NewName')

    def test_grown_name_still_rejects_corrupt_index_utf16_and_bounds(self):
        for kwargs,error in ((dict(count=1001),RuntimeError),(dict(count=4000000),RuntimeError),
                             (dict(entry_index=1000),RuntimeError),(dict(text='\ud800\0'),UnicodeDecodeError)):
            with self.subTest(kwargs=kwargs):
                if 'text' in kwargs:
                    r=self.grown_resolver();ordinary=r.read
                    r.read=lambda address,size: (b'\x00\xd8\0\0'+bytes(124))[:size] if address==0x4014 else ordinary(address,size)
                else:r=self.grown_resolver(**kwargs)
                with self.assertRaises(error):r.name(1001)
                self.assertEqual(r.names_cache,{})

    def test_concurrent_header_change_is_typed_and_never_cached(self):
        for changed_at in (2,3):
            with self.subTest(changed_at=changed_at):
                r=self.grown_resolver();ordinary=r.read;hits=0
                def read(address,size):
                    nonlocal hits
                    value=ordinary(address,size)
                    if address==r.names_header:
                        hits+=1
                        if hits>=changed_at:return struct.pack('<Qii',0x9000,1003,1003)
                    return value
                r.read=read
                with self.assertRaises(DiscoveryRegistryChanged):r.name(1001)
                self.assertEqual(r.names_cache,{})


if __name__=='__main__':unittest.main()
