"""Synthetic real reflection + fake GT transport. No process or engine calls."""
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import cover_surface_getters as getters
from cover_state import CoverQuery
from cover_bone_query import SubmitFailure
from test_cover_bone_query import FakeBridge
from test_cover_surface_query import SurfaceMemory


class GetterQueue(FakeBridge):
    def __init__(self, fixture):
        super().__init__(fixture)
        self.location, self.rotation = [-326., -3118., -56.], [0, -31860, 0]

    def submit(self, address, timeout_ms):
        assert 0 < timeout_ms <= 1000
        self.submissions.append(address)
        if self.error_at == 'create_thread':
            raise SubmitFailure('Synthetic no thread', pending_possible=False)
        if self.error_at == 'submit_timeout':
            raise SubmitFailure('Synthetic remote completion unknown', pending_possible=True)
        if self.on_submit:
            self.on_submit(self, address)
        if self.submit_result:
            return self.submit_result
        raw = self.allocations[address]
        state, size, obj, fn = struct.unpack_from('<iIQQ', raw)
        assert state == 0 and size == 16 and obj == self.f.LINK
        assert struct.unpack_from('<i', raw, 24)[0] == 1
        assert fn in (self.f.LOCATION_FN, self.f.ROTATION_FN)
        value = self.location if fn == self.f.LOCATION_FN else self.rotation
        struct.pack_into('<3f' if fn == self.f.LOCATION_FN else '<3i', raw, 28, *value)
        struct.pack_into('<i', raw, 0, self.queue_state)
        return 0

    def read(self, address, size):
        if self.error_at == 'read':
            raise OSError('Synthetic queue read failed')
        return super().read(address, size)

    def free(self, address):
        if self.error_at == 'free':
            raise OSError('Synthetic free failed')
        return super().free(address)

    def close(self):
        self.closed = True
        if self.error_at == 'close':
            raise OSError('Synthetic close failed')


class GetterMemory(SurfaceMemory):
    def __init__(self):
        super().__init__()
        self.profile['sha256'] = self.module['sha256'] = getters.surface.GAME_SHA256
        for fn, name in ((self.LOCATION_FN, 'GetSlotLocation'), (self.ROTATION_FN, 'GetSlotRotation')):
            self.put(fn + 0x106, struct.pack('<HH', 16, 4))
            self.put(fn + 0x114, struct.pack('<Q', self.BASE + getters.PROFILE[name][0]))
            for field, offset in (('SlotIdx',0), ('ReturnValue',4)):
                prop = self.properties[fn, field]
                prop['offset'] = offset
                self.put(prop['address'] + 0x94, struct.pack('<I', offset))
        self.getter_chunks = []
        for i, (name, (_, chunks)) in enumerate(getters.PROFILE.items()):
            for j, (rva, size, _) in enumerate(chunks):
                data = bytes([0x61 + i * 2 + j]) * size
                self.put(self.BASE + rva, data)
                self.getter_chunks.append((name, rva, data, hashlib.sha256(data).hexdigest()))

    def factory(self, identity, dll_path, *, expected_sha256, attach_only):
        if identity != self.observer.identities['game_identity'] or expected_sha256 != self.DLL_SHA or attach_only is not True:
            raise RuntimeError('Fake attach/hash rejected')
        q = GetterQueue(self)
        self.bridges.append(q)
        if self.configure_bridge:
            self.configure_bridge(q)
        return q

    def capture_getters(self, **kwargs):
        # Only the external private-code loader is synthetic. Reflection,
        # current module/code equality, closing identities and transport run.
        with patch.object(getters, '_certificate', return_value=(self.getter_chunks, dict(path='synthetic', hard_pinned_profile=True, chunk_count=7))):
            return getters.capture(self.observer, 'synthetic/Playground.dll', self.DLL_SHA,
                                   certificate_path='synthetic', bridge_factory=self.factory, **kwargs)


class GetterTests(unittest.TestCase):
    def result(self, f, accepted=True, **kwargs):
        memory = copy.deepcopy(f.memory)
        out = f.capture_getters(**kwargs)
        self.assertIs(out['accepted'], accepted, out.get('rejection'))
        for key in ('activation_allowed','collision_verified','atomic_snapshot_verified',
                    'actor_or_mesh_fields_written','native_impl_semantics_verified','location_matrix_helper_current_verified'):
            self.assertIs(out[key], False)
        self.assertTrue(out['non_atomic'] and out['read_only_getters'])
        for bridge in f.bridges:
            self.assertTrue(bridge.closed)
        if f.before_read is None and f.at_verify is None and not getattr(f,'allow_synthetic_mutation',False) and not any(b.on_submit or b.on_read for b in f.bridges):
            self.assertEqual(f.memory, memory)
        json.dumps(out, allow_nan=False)
        return out

    def test_exact_two_real_typed_signatures_current_code_and_request_outputs(self):
        f, queries = GetterMemory(), []
        def factory(observer):
            q = CoverQuery(observer);queries.append(q);return q
        out = self.result(f, query_factory=factory)
        self.assertEqual(out['values'], {'GetSlotLocation':[-326.,-3118.,-56.], 'GetSlotRotation':[0,-31860,0]})
        self.assertTrue(out['engine_calls'])
        self.assertEqual(out['queue_submissions'], 2)
        self.assertEqual(out['retained_allocations'], [])
        self.assertEqual([c['getter'] for c in out['calls']], ['GetSlotLocation','GetSlotRotation'])
        for c in out['calls']:
            self.assertEqual((c['function']['native_index'],c['function']['parms_size'],c['function']['return_offset']),(0,16,4))
            self.assertEqual(len(c['code_before']),7)
            self.assertEqual(c['code_after'],c['code_before'])
            self.assertTrue(c['completion_observed'])
        self.assertEqual(f.bridges[0].submissions,f.bridges[0].freed)
        self.assertTrue(all(q.reads < 4096 for q in queries))

    def test_current_slots_world_possession_and_full_identity_reject(self):
        for obj,name,value,fmt in ((SurfaceMemory.PAWN,'Controller',SurfaceMemory.OTHER,'<Q'),
                                  (SurfaceMemory.LINK,'WorldInfo',SurfaceMemory.OTHER,'<Q'),
                                  (SurfaceMemory.PAWN,'CurrentSlotIdx',-1,'<i')):
            with self.subTest(name=name):
                f=GetterMemory();f.set_value(obj,name,value,fmt);out=self.result(f,False)
                self.assertEqual(out['queue_submissions'],0)
        for defect in ('slot','world','identity','slots_header','code','metadata'):
            with self.subTest(closing=defect):
                f=GetterMemory()
                def mutate(b,address):
                    if defect=='slot':f.set_value(f.PAWN,'CurrentSlotIdx',0,'<i')
                    if defect=='world':f.set_value(f.LINK,'WorldInfo',f.OTHER,'<Q')
                    if defect=='identity':f.objects[f.LINK]['internal_index']+=1
                    if defect=='slots_header':f.set_value(f.LINK,'Slots',0,'<Q')
                    if defect=='code':f.put(f.BASE+0x92ec82+31,b'\x00')
                    if defect=='metadata':f.put(f.LOCATION_FN+0x106,struct.pack('<H',20))
                f.configure_bridge=lambda b:setattr(b,'on_submit',mutate)
                out=self.result(f,False);self.assertEqual(len(f.bridges[0].submissions),1)
                self.assertTrue(out['calls'][0]['completion_observed'])
                self.assertEqual(out['retained_allocations'],[])

    def test_code_every_main_and_cold_tail_is_checked_before_allocation(self):
        for _,rva,data,_ in GetterMemory().getter_chunks:
            with self.subTest(rva=hex(rva)):
                f=GetterMemory();f.put(f.BASE+rva+len(data)-1,b'\x00');out=self.result(f,False)
                self.assertEqual(f.bridges[0].allocations,{})
                self.assertEqual(f.bridges[0].submissions,[])
                self.assertIn('full chunk',out['rejection'])

    def test_exact_header_layout_and_struct_component_types_reject(self):
        for defect in ('native','parms','return','entry','flags','argwidth','component','d0','dimension'):
            with self.subTest(defect=defect):
                f=GetterMemory();p=f.properties[f.LOCATION_FN,'SlotIdx']['address']
                if defect=='native':f.put(f.LOCATION_FN+0xf8,struct.pack('<H',1))
                if defect=='parms':f.put(f.LOCATION_FN+0x106,struct.pack('<H',20))
                if defect=='return':f.put(f.LOCATION_FN+0x108,struct.pack('<H',8))
                if defect=='entry':f.put(f.LOCATION_FN+0x114,struct.pack('<Q',f.BASE+0x2100))
                if defect=='flags':f.put(f.LOCATION_FN+0xfc,bytes(4))
                if defect=='argwidth':f.put(p+0x74,struct.pack('<I',8))
                if defect=='dimension':f.put(p+0x70,struct.pack('<I',2))
                if defect=='component':f.objects[f.properties[f.VECTOR,'X']['address']]['cls']=f.kind_classes['IntProperty']
                if defect=='d0':f.put(f.properties[f.LOCATION_FN,'ReturnValue']['address']+0xd0,struct.pack('<Q',f.ROTATOR))
                out=self.result(f,False);self.assertEqual(f.bridges[0].submissions,[])

    def test_transport_unknown_and_partial_write_retain_never_retry(self):
        for error in ('write','create_thread','submit_timeout','read'):
            with self.subTest(error=error):
                f=GetterMemory();f.configure_bridge=lambda b:setattr(b,'error_at',error)
                out=self.result(f,False);self.assertEqual(len(out['calls']),1)
                self.assertEqual(len(out['retained_allocations']),1)
                self.assertEqual(f.bridges[0].freed,[])
                if error=='submit_timeout':self.assertTrue(out['retained_allocations'][0]['pending_possible'])
        for result in (1,2,17):
            f=GetterMemory();f.configure_bridge=lambda b:setattr(b,'submit_result',result)
            out=self.result(f,False);self.assertEqual(len(out['retained_allocations']),1)
            self.assertEqual(len(f.bridges[0].submissions),1)

    def test_pending_unknown_states_and_timeout_preserve_only_request(self):
        for state in (0,-1,1,3):
            with self.subTest(state=state):
                f=GetterMemory();f.configure_bridge=lambda b:setattr(b,'queue_state',state)
                with patch.object(getters,'CALL_SECONDS',.04):out=self.result(f,False)
                self.assertEqual(len(out['retained_allocations']),1)
                self.assertFalse(out['calls'][0]['completion_observed'])
                self.assertEqual(len(f.bridges[0].submissions),1)

    def test_completed_bad_return_argument_header_frees_but_rejects(self):
        for defect in ('nan','infinity','huge','slot','receiver','size'):
            with self.subTest(defect=defect):
                f=GetterMemory()
                def setup(b):
                    if defect in ('nan','infinity','huge'):b.location[0]={'nan':math.nan,'infinity':math.inf,'huge':float(2**25)}[defect]
                    def mutate(bridge,address,size):
                        if size!=getters.REQUEST_SIZE:return
                        raw=bridge.allocations[address]
                        if defect=='slot':struct.pack_into('<i',raw,24,0)
                        if defect=='receiver':struct.pack_into('<Q',raw,8,f.PAWN)
                        if defect=='size':struct.pack_into('<I',raw,4,20)
                    b.on_read=mutate
                f.configure_bridge=setup;out=self.result(f,False)
                self.assertTrue(out['calls'][0]['completion_observed']);self.assertEqual(out['retained_allocations'],[])
                self.assertEqual(len(f.bridges[0].submissions),1)

    def test_lifecycle_exception_free_and_close_failures_are_receipted(self):
        for error in ('allocate','free','close'):
            with self.subTest(error=error):
                f=GetterMemory();f.configure_bridge=lambda b:setattr(b,'error_at',error)
                out=self.result(f,False)
                if error=='free':self.assertEqual(len(out['retained_allocations']),1);self.assertEqual(len(out['calls']),1)
        f=GetterMemory();f.observer.action_owner={};out=self.result(f,False);self.assertEqual(f.bridges,[])
        f=GetterMemory();f.configure_bridge=lambda b:setattr(b,'on_submit',lambda *_:(_ for _ in ()).throw(KeyboardInterrupt('synthetic interruption')))
        out=self.result(f,False);self.assertEqual(out['rejection_type'],'KeyboardInterrupt');self.assertEqual(len(out['retained_allocations']),1)

    def test_binding_change_between_getters_stops_before_second_allocation(self):
        f=GetterMemory();f.allow_synthetic_mutation=True
        def setup(b):
            free=b.free
            def changed(address):
                free(address)
                f.set_value(f.PAWN,'CurrentSlotIdx',0,'<i')
            b.free=changed
        f.configure_bridge=setup;out=self.result(f,False)
        self.assertEqual(len(out['calls']),2)
        self.assertTrue(out['calls'][0]['accepted'])
        self.assertFalse(out['calls'][1]['accepted'])
        self.assertEqual(len(f.bridges[0].submissions),1)
        self.assertEqual(out['retained_allocations'],[])

    def test_exact_dll_pin_and_pre_sample_action_owner_required(self):
        for defect in ('sha','path','readers','owner'):
            with self.subTest(defect=defect):
                f=GetterMemory();sha=f.DLL_SHA;path='synthetic/Playground.dll'
                if defect=='sha':sha='A'*64
                if defect=='path':path=''
                if defect=='readers':f.observer.sampler=None
                if defect=='owner':f.observer.action_owner={}
                out=getters.capture(f.observer,path,sha,certificate_path='unused',bridge_factory=f.factory)
                self.assertFalse(out['accepted']);self.assertEqual(f.bridges,[])

    def test_after_write_metadata_guard_and_after_submit_lifecycle_loss_retain(self):
        f=GetterMemory();f.allow_synthetic_mutation=True
        def setup(b):
            write=b.write
            def changed(address,raw):
                write(address,raw)
                f.put(f.LOCATION_FN+0x106,struct.pack('<H',20))
            b.write=changed
        f.configure_bridge=setup;out=self.result(f,False)
        self.assertEqual(out['submit_attempts'],0)
        self.assertEqual(len(out['retained_allocations']),1)
        f=GetterMemory()
        def lost(*args):f.observer.session['session_id']='different-session'
        f.configure_bridge=lambda b:setattr(b,'on_submit',lost)
        out=self.result(f,False)
        self.assertEqual(out['queue_submissions'],1)
        self.assertTrue(out['execution_possible_unobserved'])
        self.assertFalse(out['calls'][0]['completion_observed'])
        self.assertEqual(len(out['retained_allocations']),1)

    def test_total_budget_expires_before_attach_or_new_allocation(self):
        f=GetterMemory()
        with patch.object(getters,'CAPTURE_SECONDS',0):out=self.result(f,False)
        self.assertEqual(f.bridges,[])
        self.assertEqual(out['retained_allocations'],[])


class CertificateTests(unittest.TestCase):
    def test_private_real_certificate_exact_profile_when_available(self):
        path=Path(__file__).resolve().parents[2]/'analysis/cover-implementation-20261024/ida-body1/surface-getters2.json'
        if not path.exists():self.skipTest('Private evidence intentionally excluded from source packages')
        chunks,source=getters._certificate(path)
        self.assertEqual(len(chunks),7);self.assertTrue(source['hard_pinned_profile'])
        self.assertEqual(sum(len(c[2]) for c in chunks),726)

    def test_arbitrary_self_consistent_certificate_cannot_change_function_or_bytes(self):
        methods=[]
        for name,(entry,chunks) in getters.PROFILE.items():
            methods.append(dict(name=name,rva=hex(entry),chunks=[dict(rva=hex(rva),size=size,bytes=bytes(size).hex(),sha256=hashlib.sha256(bytes(size)).hexdigest()) for rva,size,_ in chunks]))
        base=dict(complete=True,historical_runtime_only=True,source_sha256=getters.surface.HISTORICAL_SHA256,methods=methods)
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'certificate.json'
            for defect in ('bytes','name','rva','duplicate','source','extra'):
                value=copy.deepcopy(base)
                if defect=='name':value['methods'][0]['name']='Trace'
                if defect=='rva':value['methods'][0]['rva']='0xcdacc'
                if defect=='duplicate':value['methods'][1]=copy.deepcopy(value['methods'][0])
                if defect=='source':value['source_sha256']='a'*64
                if defect=='extra':value['methods'].append(copy.deepcopy(value['methods'][0]))
                path.write_text(json.dumps(value))
                with self.subTest(defect=defect),self.assertRaises(RuntimeError):getters._certificate(path)


if __name__=='__main__':unittest.main()
