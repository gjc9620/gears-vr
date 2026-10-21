"""Attempt-local assembly hints never replace the final live proof."""
import struct
import unittest
from unittest import mock

from test_ui_observer_config import Fixture as UIFixture
from test_crosshair_assignment_config import Fixture as CrosshairFixture
from test_mounted_integration import MountedUIFixture
from anchor_config import build_anchor_config, anchor_ini, LiveRegistryAnchorSampler, AnchorRejected
from ui_observer_config import build_ui_observer_config, ui_observer_ini, UIConfigRejected
from crosshair_assignment_config import (build_crosshair_assignment_config,
    crosshair_assignment_ini, CrosshairConfigRejected)


def crosshair(f, reuse=True):
    return build_crosshair_assignment_config(f, f.camera, include_tick_roots=True,
        include_writer_view=True, reuse_assembly_identities=reuse)


class AssemblyIdentityTests(unittest.TestCase):
    def crowded_ui(self):
        f=UIFixture()
        # Real owner classes contain many fields that will not be serialized
        # into the <=96 selected-identity certificate. Keep synthetic extras
        # outside the fixture's registry/name-table address ranges.
        for i in range(300):
            name='UnselectedFunction'+str(i)
            address=f.function('WarHUD',name,0x1d6ca0)
            relocated=0x10000000+i*0x1000
            f.put(relocated,f.read(address,0x140));f.objects[-1]=relocated
            f.children[f.classes['WarHUD']][-1]=relocated
            f.function_addresses['WarHUD',name]=relocated
        f.finish_registry()
        return f

    def test_ui_hint_capacity_falls_back_without_changing_certificate(self):
        f=self.crowded_ui()
        reference,_=build_ui_observer_config(f,f.camera,reuse_assembly_identities=False)
        cached,meta=build_ui_observer_config(f,f.camera)
        self.assertEqual(cached,reference)
        self.assertEqual(ui_observer_ini(cached),ui_observer_ini(reference))
        self.assertLessEqual(len(cached['identities']),96)
        self.assertEqual(meta['assembly_identity_hints']['entries'],256)
        self.assertGreater(meta['assembly_identity_hints']['uncached_reads'],0)

    def test_ui_full_hint_cache_still_rejects_final_identity_change(self):
        f=self.crowded_ui();read=f.read;hits=0
        def changing(address,size):
            nonlocal hits
            if address==f.names_header and size==16:
                hits+=1
                if hits==3:f.put(f.hud+76,struct.pack('<I',1))
            return read(address,size)
        f.read=changing
        with self.assertRaisesRegex(UIConfigRejected,'UI final identity mismatch'):
            build_ui_observer_config(f,f.camera)

    def test_anchor_all_guards_retained_and_entire_wire_equal(self):
        from test_anchor_config import Fixture
        for factory in (Fixture,MountedUIFixture):
            rows=[]
            for reuse in (False,True):
                f=factory()
                with mock.patch.object(f,'read',wraps=f.read) as reads:
                    config,meta=build_anchor_config(f,sample_pose=False,reuse_assembly_identities=reuse)
                rows.append((config,meta,reads.call_count))
            self.assertEqual(rows[0][0],rows[1][0])
            self.assertEqual(anchor_ini(rows[0][0]),anchor_ini(rows[1][0]))
            self.assertEqual(len(rows[0][1]['guard_acquisitions']),len(rows[1][1]['guard_acquisitions']))
            self.assertEqual(rows[0][1]['strict_guard_attempts'],rows[1][1]['strict_guard_attempts'])
            self.assertLess(rows[1][2],rows[0][2]*.95)

    def test_anchor_final_boundary_rejects_changed_identity_and_super(self):
        for mutation in ('identity','super'):
            f=MountedUIFixture();read=LiveRegistryAnchorSampler.fresh;changed=False
            def changing(sampler,address):
                nonlocal changed
                if sampler.guard_stage=='final_identity_validation' and not changed:
                    changed=True
                    if mutation=='identity':f.ptr(f.TURRET+80,0x3000)
                    else:f.ptr(0x71000+f.profile['field_super'],0)
                return read(sampler,address)
            with self.subTest(mutation=mutation),mock.patch.object(LiveRegistryAnchorSampler,'fresh',changing):
                with self.assertRaises(AnchorRejected):build_anchor_config(f,sample_pose=False)
            self.assertTrue(changed)

    def test_ui_proof_and_wire_equal_with_fewer_actual_reads(self):
        rows=[]
        for reuse in (False, True):
            f=UIFixture()
            with mock.patch.object(f, 'read', wraps=f.read) as reads:
                config,_=build_ui_observer_config(f, f.camera, reuse_assembly_identities=reuse)
            rows.append((config, reads.call_count))
        self.assertEqual(rows[0][0], rows[1][0])
        self.assertEqual(ui_observer_ini(rows[0][0]), ui_observer_ini(rows[1][0]))
        self.assertLess(rows[1][1], rows[0][1] * .4)

    def test_mounted_ui_retains_driver_controlled_and_entire_wire(self):
        rows=[]
        for reuse in (False, True):
            f=MountedUIFixture();anchor,_=build_anchor_config(f)
            config,_=build_ui_observer_config(f, f.resolve(), mounted_anchor=anchor,
                                             reuse_assembly_identities=reuse)
            rows.append(config)
        self.assertEqual(rows[0], rows[1])
        self.assertEqual(ui_observer_ini(rows[0]), ui_observer_ini(rows[1]))
        self.assertEqual(rows[1]['identities'][rows[1]['roles']['controlled']]['address'], f.TURRET)
        self.assertEqual(rows[1]['identities'][rows[1]['roles']['pawn']]['address'], 0x20000)

    def test_crosshair_proof_and_wire_equal_with_fewer_actual_reads(self):
        rows=[]
        for reuse in (False, True):
            f=CrosshairFixture()
            with mock.patch.object(f, 'read', wraps=f.read) as reads:
                config=crosshair(f, reuse)
            rows.append((config, reads.call_count))
        self.assertEqual(crosshair_assignment_ini(rows[0][0]), crosshair_assignment_ini(rows[1][0]))
        for config,_ in rows:
            config['provenance'].pop('acquired_perf_counter_ns')
            config['provenance'].pop('finished_perf_counter_ns')
        self.assertEqual(rows[0][0], rows[1][0])
        self.assertLess(rows[1][1], rows[0][1] * .25)

    def test_ui_final_boundary_rereads_every_identity_slot_and_live_name(self):
        for mutation in ('header', 'slot', 'name'):
            f=UIFixture();read=f.read;hits=0
            def changing(address,size):
                nonlocal hits
                if address==f.names_header and size==16:
                    hits+=1
                    if hits==3:
                        if mutation=='header':f.put(f.hud+76,struct.pack('<I',1))
                        elif mutation=='slot':f.put(f.od+f.objects.index(f.hud)*8,bytes(8))
                        else:f.put(0x300000+f.names['WarHUD']*0x100+20,'Changed'.encode('utf-16-le')+b'\0\0')
                return read(address,size)
            f.read=changing
            with self.subTest(mutation=mutation), self.assertRaises(UIConfigRejected):
                build_ui_observer_config(f,f.camera)

    def test_crosshair_final_boundary_rereads_every_recorded_span(self):
        for mutation in ('header', 'slot', 'name'):
            f=CrosshairFixture();read=f.read;hits=0
            def changing(address,size):
                nonlocal hits
                # These headers have only their initial two reads before the
                # uncached final check. Alter a selected identity at that gate.
                if address==f.objects_header and size==16:
                    hits+=1
                    if hits==3:
                        if mutation=='header':f.put(f.weapon+76,struct.pack('<I',1))
                        elif mutation=='slot':f.put(f.od+f.objects.index(f.weapon)*8,bytes(8))
                        else:f.put(0x300000+f.names['Rifle']*0x100+20,'Changed'.encode('utf-16-le')+b'\0\0')
                return read(address,size)
            f.read=changing
            with self.subTest(mutation=mutation), self.assertRaises(CrosshairConfigRejected):crosshair(f)

    def test_hints_do_not_survive_acquisition_or_reader(self):
        f=UIFixture();original,_=build_ui_observer_config(f,f.camera)
        f.put(f.hud+76,struct.pack('<I',1))
        changed,_=build_ui_observer_config(f,f.camera)
        self.assertEqual(original['identities'][original['roles']['hud']]['number'],0)
        self.assertEqual(changed['identities'][changed['roles']['hud']]['number'],1)
        other=UIFixture();again,_=build_ui_observer_config(other,other.camera)
        self.assertEqual(original,again)

    def test_optional_reference_mode_requires_boolean(self):
        for value in (1,None,'yes'):
            f=UIFixture()
            with self.assertRaises(ValueError):
                build_ui_observer_config(f,f.camera,reuse_assembly_identities=value)
            f=CrosshairFixture()
            with self.assertRaises(ValueError):
                build_crosshair_assignment_config(f,f.camera,reuse_assembly_identities=value)


if __name__=='__main__':unittest.main()
