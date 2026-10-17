"""Synthetic name memory; no process is opened."""
import struct
import unittest
from resolve_camera import Resolver


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


if __name__=='__main__':unittest.main()
