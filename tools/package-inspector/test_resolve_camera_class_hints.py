"""Mapped-memory discovery comparisons; no live process is opened."""
import struct
import unittest

from resolve_camera import DiscoveryRegistryChanged, Resolver


def fixture():
    r=Resolver.__new__(Resolver)
    r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80)
    r.ident_cache={};r.names_cache={}
    names={0:'Class',1:'Pawn',2:'Canvas',3:'Instance'}
    r.name=names.__getitem__
    memory=bytearray(0x70000);calls=[]
    def read(address,size):
        calls.append((address,size))
        if address<0 or address+size>len(memory):raise RuntimeError('Unmapped')
        return bytes(memory[address:address+size])
    r.read=read
    def obj(address,name,cls,index=0):
        struct.pack_into('<I',memory,address+8,index)
        struct.pack_into('<IIQ',memory,address+72,name,0,cls)
    obj(0x580,0,0x580);obj(0x500,1,0x580);obj(0x600,2,0x580)
    pointers=[]
    for slot in range(8):
        for page in range(1,97):
            address=page*4096+slot*0x100+0x100
            obj(address,3,0x500,len(pointers));pointers.append(address)
    pointers.extend((0x600,0x580,0x500))
    return r,memory,calls,pointers,obj


def ordinary(r,pointers):
    discover=r.discovery_identity_reader()
    return [(address,discover(address)) for address in pointers
            if address and discover.class_identity(address)['name']=='Class']


class ClassHintTests(unittest.TestCase):
    def test_mapped_blocks_preserve_candidates_and_reduce_reads(self):
        r,_,calls,pointers,_=fixture();expected=ordinary(r,pointers);before=len(calls)
        r,_,calls,pointers,_=fixture();actual=list(r.discovery_class_candidates(pointers))
        self.assertEqual(sorted(expected),actual)
        self.assertLess(len(calls),before//4)
        self.assertLessEqual(max(size for _,size in calls),65536)

    def test_unreadable_or_short_block_falls_back(self):
        for short in (False,True):
            with self.subTest(short=short):
                r,_,_,pointers,_=fixture();read=r.read
                def restricted(address,size):
                    if size==r.CLASS_HINT_BLOCK_SIZE:
                        if short:return b'\0'
                        raise OSError('Unmapped neighbor')
                    return read(address,size)
                r.read=restricted
                self.assertEqual([a for a,_ in r.discovery_class_candidates(pointers)],[0x500,0x580,0x600])

    def test_header_crossing_block_boundary_uses_exact_read(self):
        r,_,calls,_,obj=fixture();address=0x1ffd0;obj(address,2,0x580)
        self.assertEqual([a for a,_ in r.discovery_class_candidates([address])],[address])
        self.assertIn((address,0x60),calls)

    def test_null_and_bad_class_are_not_candidates(self):
        r,_,_,_,obj=fixture();obj(0x1000,3,0);obj(0x2000,3,0xfffffff)
        self.assertEqual(list(r.discovery_class_candidates([0,0x1000,0x2000])),[])

    def test_registry_change_is_never_swallowed(self):
        r,_,_,pointers,_=fixture()
        def changed(*_):raise DiscoveryRegistryChanged('Changed')
        r.read=changed
        with self.assertRaises(DiscoveryRegistryChanged):list(r.discovery_class_candidates(pointers))

    def test_class_identity_read_error_does_not_poison_later_hint(self):
        r,_,_,_,obj=fixture();obj(0x1000,2,0x580);obj(0x2000,2,0x580)
        identity=r.identity;failed=False
        def flaky(address,**kwargs):
            nonlocal failed
            if address==0x580 and not failed:
                failed=True;raise OSError('Temporary read failure')
            return identity(address,**kwargs)
        r.identity=flaky
        self.assertEqual([a for a,_ in r.discovery_class_candidates([0x1000,0x2000])],[0x2000])


if __name__=='__main__':unittest.main()
