"""Exact certificate and adversarial fresh-read checks; no game or XR."""
import hashlib
import struct
import unittest
from unittest import mock

from test_ui_observer_config import Fixture as UIFixture
from test_crosshair_assignment_config import Fixture as CrosshairFixture
from test_mounted_integration import MountedUIFixture
from anchor_config import build_anchor_config
from ui_observer_config import build_ui_observer_config,ui_observer_ini,UIConfigRejected
from crosshair_assignment_config import build_crosshair_assignment_config,crosshair_assignment_ini,CrosshairConfigRejected


def assignment(f,**options):
    with mock.patch('cinematic_anchor.BOOL_CODE',getattr(f,'aim_code',())):
        return build_crosshair_assignment_config(f,f.camera,include_tick_roots=True,
            include_writer_view=True,include_aim_state=hasattr(f,'aim_code'),**options)


class AimFixture(CrosshairFixture):
    def __init__(self):
        super().__init__();self.cls('BoolProperty',self.classes['Object'])
        prop=self.field(self.classes['WarPC'],'bIsTargeting',0x330,4,0x20,'BoolProperty')
        self.put(prop,struct.pack('<Q',0xb10000));self.put(0xb102a0,struct.pack('<Q',self.base+0x1daba0))
        self.put(self.base+0x1b86280+0x2d*8,struct.pack('<Q',self.base+0x1d6ea4));self.aim_code=[]
        for rva,size in ((0x1d6ea4,127),(0x1daba0,25)):
            raw=bytes([0x90])*size;self.put(self.base+rva,raw)
            self.aim_code.append((rva,size,hashlib.sha256(raw).hexdigest()))
        self.finish_registry()


class CoalescedReadTests(unittest.TestCase):
    def test_ui_and_mounted_certificates_equal_reference_with_fewer_reads(self):
        for factory in (UIFixture,MountedUIFixture):
            rows=[]
            for enabled in (False,True):
                f=factory();options={}
                if factory is MountedUIFixture:
                    anchor,_=build_anchor_config(f,sample_pose=False)
                    options['mounted_anchor']=anchor
                with mock.patch.object(f,'read',wraps=f.read) as reads:
                    c,_=build_ui_observer_config(f,f.resolve() if factory is MountedUIFixture else f.camera,
                        coalesce_reads=enabled,**options)
                rows.append((c,reads.call_count))
            self.assertEqual(rows[0][0],rows[1][0]);self.assertEqual(ui_observer_ini(rows[0][0]),ui_observer_ini(rows[1][0]))
            self.assertLess(rows[1][1],rows[0][1])

    def test_crosshair_wire_equal_reference_with_fewer_reads(self):
        for factory in (CrosshairFixture,AimFixture):
            rows=[]
            for enabled in (False,True):
                f=factory()
                with mock.patch.object(f,'read',wraps=f.read) as reads:c=assignment(f,coalesce_reads=enabled)
                rows.append((crosshair_assignment_ini(c),reads.call_count))
            self.assertEqual(rows[0][0],rows[1][0]);self.assertLess(rows[1][1],rows[0][1]*.9)

    def test_each_identity_field_is_compared_between_fresh_snapshots(self):
        for offset in (8,64,72,76,80):
            f=CrosshairFixture();read=f.read;hits=0
            def changing(a,n):
                nonlocal hits
                raw=read(a,n)
                if (a,n)==(f.pc+8,80):
                    hits+=1
                    if hits==1:f.put(f.pc+offset,bytes([read(f.pc+offset,1)[0]^1]))
                return raw
            f.read=changing
            with self.subTest(offset=offset),self.assertRaisesRegex(CrosshairConfigRejected,'double-read changed'):
                assignment(f)
            self.assertEqual(hits,2)

    def test_mutable_object_padding_does_not_become_identity(self):
        f=CrosshairFixture();read=f.read;hits=0
        def changing(a,n):
            nonlocal hits
            raw=read(a,n)
            if (a,n)==(f.pc+8,80):
                hits+=1
                if hits==1:f.put(f.pc+16,bytes([1]))
            return raw
        f.read=changing;c=assignment(f)
        self.assertEqual(c['identities'][c['roles']['controller']]['address'],f.pc);self.assertEqual(hits,2)

    def test_name_index_and_text_stay_double_read_while_padding_is_ignored(self):
        for kind in ('ui','crosshair'):
            for offset in (0,20,4):
                f=UIFixture() if kind=='ui' else CrosshairFixture()
                read=f.read;entry=0x300000+f.names['WarPC']*0x100;hits=0
                def changing(a,n):
                    nonlocal hits
                    raw=read(a,n)
                    if (a,n)==(entry,148):
                        hits+=1
                        if hits==1:f.put(entry+offset,bytes([read(entry+offset,1)[0]^1]))
                    return raw
                f.read=changing
                def build():return build_ui_observer_config(f,f.camera) if kind=='ui' else assignment(f)
                with self.subTest(kind=kind,offset=offset):
                    if offset==4:build()
                    else:
                        with self.assertRaises(UIConfigRejected if kind=='ui' else CrosshairConfigRejected):build()
                    self.assertGreaterEqual(hits,2)

    def test_short_batched_name_and_object_reads_are_rejected(self):
        for kind in ('ui','crosshair_name','crosshair_object'):
            f=UIFixture() if kind=='ui' else CrosshairFixture();read=f.read
            target=(f.pc+8,80) if kind=='crosshair_object' else (0x300000+f.names['WarPC']*0x100,148)
            f.read=lambda a,n:read(a,n)[:-1] if (a,n)==target else read(a,n)
            with self.subTest(kind=kind),self.assertRaises(UIConfigRejected if kind=='ui' else CrosshairConfigRejected):
                build_ui_observer_config(f,f.camera) if kind=='ui' else assignment(f)

    def test_unknown_name_layout_keeps_reference_reads_and_certificate(self):
        rows=[]
        for enabled in (False,True):
            f=UIFixture();f.profile.update(name_entry_index=4,name_entry_text=24)
            for ni in f.names.values():
                entry=0x300000+ni*0x100;name=f.read(entry+20,128)
                f.put(entry+4,struct.pack('<I',ni));f.put(entry+24,name)
            with mock.patch.object(f,'read',wraps=f.read) as reads:
                c,_=build_ui_observer_config(f,f.camera,coalesce_reads=enabled)
            rows.append((c,reads.call_count))
        self.assertEqual(rows[0],rows[1])

    def test_reference_option_requires_boolean(self):
        for value in (1,None,'yes'):
            f=UIFixture()
            with self.assertRaises(ValueError):build_ui_observer_config(f,f.camera,coalesce_reads=value)
            f=CrosshairFixture()
            with self.assertRaises(ValueError):assignment(f,coalesce_reads=value)


if __name__=='__main__':unittest.main()
