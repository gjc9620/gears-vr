"""Cold client discovery over synthetic memory; no process handles or input."""
import struct
import unittest
from unittest import mock
import menu_state
from menu_state import MenuReader, registered
from resolve_camera import Resolver


def resolver(*, duplicate=False, change=None, unreadable_pages=False):
    r=Resolver.__new__(Resolver);r.pid=123;r.ident_cache={};r.names_cache={};r.calls=[]
    r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80)
    r.prepare=mock.Mock();r.p=mock.Mock();r.objects_data=0x10000
    r.client=0x300;r.client_class=0x180
    objects=[0x100,0x180,0x200,0x300,*[0x400+i*0x80 for i in range(64)]]
    if duplicate:objects.append(0x2480)
    r.object_pointers=tuple(objects);r.raw=bytearray(0x12000)
    names={1:'Class',2:'GameUISceneClient',3:'Other',4:'Unrelated'}
    r.name=lambda index:names[index]
    for index,address in enumerate(objects):
        name=1 if address==0x100 else 2 if address in (0x180,0x300,0x2480) else 3 if address==0x200 else 4
        cls=0x180 if address in (0x300,0x2480) else 0x100 if address in (0x100,0x180,0x200) else 0x200
        struct.pack_into('<I',r.raw,address+8,index)
        struct.pack_into('<Q',r.raw,address+64,0x70000 if address in (0x300,0x2480) else 0)
        struct.pack_into('<IIQ',r.raw,address+72,name,0,cls)
        struct.pack_into('<Q',r.raw,r.objects_data+index*8,address)
    mutated=False
    def read(address,size):
        nonlocal mutated
        r.calls.append((address,size))
        if unreadable_pages and size==4096:raise RuntimeError('Unreadable neighbor page')
        if not mutated:
            if change=='identity' and (address,size)==(r.client,0x60):
                struct.pack_into('<Q',r.raw,r.client+64,0x71000);mutated=True
            elif change=='class' and (address,size)==(r.client_class,0x60):
                struct.pack_into('<I',r.raw,r.client_class+72,3);mutated=True
            elif change=='slot' and (address,size)==(r.objects_data+objects.index(r.client)*8,8):
                struct.pack_into('<Q',r.raw,address,0);mutated=True
        return bytes(r.raw[address:address+size])
    r.read=read
    return r


def legacy_client(r):
    """The original constructor's exact per-object registered scan."""
    r.prepare();candidates=[]
    for address in r.object_pointers:
        try:
            obj=registered(r,address)
            if obj['name']=='GameUISceneClient' and r.class_name(obj)=='GameUISceneClient':candidates.append(address)
        except (RuntimeError,OSError,ValueError,UnicodeError):continue
    if len(candidates)!=1:raise RuntimeError('Expected one registered GameUISceneClient')
    return candidates[0],dict(registered(r,candidates[0]))


class ClientDiscoveryTests(unittest.TestCase):
    def construct(self,r):
        with mock.patch.object(menu_state,'Resolver',return_value=r):return MenuReader(r.pid)

    def test_same_client_with_fewer_exact_and_total_reads(self):
        ordinary=resolver();expected=legacy_client(ordinary)
        batched=resolver();before=bytes(batched.raw);reader=self.construct(batched)
        self.assertEqual((reader.client,reader.client_identity),expected)
        self.assertEqual(bytes(batched.raw),before)
        self.assertLess(len(batched.calls)*3,len(ordinary.calls))
        self.assertEqual(sum(size==0x60 for _,size in batched.calls),2)
        self.assertEqual(sum(size==4096 for _,size in batched.calls),3)
        self.assertEqual(sum(size==8 for _,size in batched.calls),3)
        self.assertFalse(any(address in batched.ident_cache for address in batched.object_pointers if address>=0x400))
        reader.close();batched.p.close.assert_called_once()

    def test_candidate_or_class_change_after_hint_rejects(self):
        for change in ('identity','class'):
            r=resolver(change=change)
            with self.subTest(change=change),self.assertRaisesRegex(RuntimeError,'identity changed during discovery'):
                self.construct(r)
            r.p.close.assert_called_once()

    def test_fresh_candidate_registry_slot_drift_rejects(self):
        r=resolver(change='slot')
        with self.assertRaisesRegex(RuntimeError,'Live object registry slot changed'):self.construct(r)
        r.p.close.assert_called_once()

    def test_multiple_fresh_clients_still_rejects(self):
        r=resolver(duplicate=True)
        with self.assertRaisesRegex(RuntimeError,'found 2'):self.construct(r)
        r.p.close.assert_called_once()

    def test_unreadable_hint_pages_use_existing_exact_fallback(self):
        r=resolver(unreadable_pages=True);reader=self.construct(r)
        expected=legacy_client(resolver())
        self.assertEqual((reader.client,reader.client_identity),expected)
        self.assertGreater(sum(size==0x60 for _,size in r.calls),2)
        reader.close()

    def test_separate_readers_have_separate_discovery_pages(self):
        first=resolver();one=self.construct(first)
        second=resolver();two=self.construct(second)
        self.assertEqual((one.client,one.client_identity),(two.client,two.client_identity))
        self.assertEqual(sum(size==4096 for _,size in first.calls),3)
        self.assertEqual(sum(size==4096 for _,size in second.calls),3)
        one.close();two.close()


if __name__=='__main__':unittest.main()
