"""Read-only, bracket-checked local Pawn anchor measurements.

Equal camera timestamps and stable identities are consistency checks, not an
atomic engine/render snapshot. Candidate eye location is an unverified hypothesis.
"""
import argparse
import csv
import datetime
import json
import math
import pathlib
import struct
import time


class AnchorRejected(RuntimeError):
    pass


class RegistryHeaderChanged(AnchorRejected):
    """Exact valid header mismatch observed by the strict registry comparison."""
    def __init__(self,reason,registry,expected,observed):
        super().__init__(reason)
        self.registry_change={'registry':registry,'expected_header':list(expected),
                              'observed_header':list(observed)}


def _check_registry_header(registry,raw,expected):
    observed=struct.unpack('<Qii',raw)
    if observed==expected:return
    data,count,capacity=observed
    reason=('Object' if registry=='objects' else 'Name')+' registry changed since resolution'
    if not data or not 0<count<=capacity<4000000:raise AnchorRejected(reason)
    raise RegistryHeaderChanged(reason,registry,expected,observed)


class AnchorSampler:
    def __init__(self,resolver,*,prepared_camera=None,target_validator=None):
        self.r=resolver
        self.target_validator=target_validator
        # Explicit discovery hint from this Resolver's same preparation attempt.
        # It replaces discovery only; fresh identities/links, guard and sampling
        # below still run. It is not an expected-camera acceptance certificate.
        self.resolved=resolver.resolve() if prepared_camera is None else dict(prepared_camera)
        if prepared_camera is not None:self.resolved['offsets']=dict(prepared_camera['offsets'])
        self.camera=int(self.resolved['camera_address'],16)
        self.controller=int(self.resolved['controller_address'],16)
        self.local=int(self.resolved['local_player_address'],16)
        self.stamp=int(self.resolved['timestamp_address'],16)
        self.pov=int(self.resolved['pov_address'],16)
        self.layout={}
        pc=self.fresh(self.controller)
        self._property('controller_pawn',pc['cls'],'Pawn','ObjectProperty')
        self.pawn=self.pointer(self.controller,'controller_pawn')
        pawn=self.fresh(self.pawn)
        self.validate_controlled(pawn)
        self._property('pawn_controller',pawn['cls'],'Controller','ObjectProperty')
        self._property('pawn_location',pawn['cls'],'Location','StructProperty','Vector')
        self._property('pawn_rotation',pawn['cls'],'Rotation','StructProperty','Rotator')
        self._property('controller_rotation',pc['cls'],'Rotation','StructProperty','Rotator')
        self._property('eye_height',pawn['cls'],'EyeHeight','FloatProperty')
        self._property('base_eye_height',pawn['cls'],'BaseEyeHeight','FloatProperty')
        cam=self.fresh(self.camera)
        self.view_target=None
        try:
            view=resolver.property(cam['cls'],'ViewTarget',True)
        except RuntimeError as error:
            if 'not found' not in str(error):raise
            self.view_target_unavailable=str(error)
        else:
            if view['type']!='StructProperty':raise AnchorRejected('Camera.ViewTarget is not a struct')
            view_type=resolver.struct_type(view)
            if resolver.identity(view_type)['name']!='TViewTarget':raise AnchorRejected('Unexpected ViewTarget struct')
            target=resolver.property(view_type,'Target')
            if target['type']!='ObjectProperty':raise AnchorRejected('ViewTarget.Target is not an object')
            self.view_target=self.camera+view['offset']+target['offset']
            self.layout['camera_view_target']={'container':view,'field':target,'address':hex(self.view_target)}
        self.initial=self.guard()
        self.metadata={'resolver':self.resolved,'layout':self.layout,
            'initial_guard':self.initial,'view_target_readable':self.view_target is not None,
            'view_target_unavailable':getattr(self,'view_target_unavailable',None),
            'candidate_eye_formula':'Pawn.Location + (0,0,EyeHeight); hypothesis only',
            'renderer_alignment_verified':False,'standing_verified':False,
            'limits':'Timestamp/identity bracketing is not an atomic engine snapshot or renderer alignment proof.'}

    def validate_controlled(self,pawn):
        if not self.derives(pawn['cls'],'WarPawn'):
            raise AnchorRejected('Controlled object does not inherit WarPawn')

    def fresh(self,address):
        r=self.r;p=r.profile
        if not address:raise AnchorRejected('Null object pointer')
        raw=r.read(address,0x60)
        index=struct.unpack_from('<I',raw,p['object_internal_index'])[0]
        if index>=r.object_count or r.u64(r.objects_data+index*8)!=address:
            raise AnchorRejected('Object registry identity mismatch')
        name,number=struct.unpack_from('<II',raw,p['object_name'])
        return {'address':address,'internal_index':index,'name':r.name(name),'number':number,
                'cls':struct.unpack_from('<Q',raw,p['object_class'])[0],
                'outer':struct.unpack_from('<Q',raw,p['object_outer'])[0]}

    def derives(self,cls,expected):
        seen=set()
        while cls:
            if cls in seen or len(seen)>=64:raise AnchorRejected('Invalid superclass chain')
            seen.add(cls);identity=self.fresh(cls)
            if identity['name']==expected:return True
            cls=self.r.u64(cls+self.r.profile['field_super'])
        return False

    def _property(self,label,cls,name,kind,structure=None):
        prop=self.r.property(cls,name,True)
        if prop['type']!=kind:raise AnchorRejected('Unexpected property type: '+label)
        record=dict(prop)
        if structure:
            owner=self.r.struct_type(prop)
            if self.r.identity(owner)['name']!=structure:raise AnchorRejected('Unexpected struct: '+label)
            components=('X','Y','Z') if structure=='Vector' else ('Pitch','Yaw','Roll')
            fields=[self.r.property(owner,key) for key in components]
            expected='FloatProperty' if structure=='Vector' else 'IntProperty'
            if [p['offset'] for p in fields]!=[0,4,8] or any(p['type']!=expected for p in fields):
                raise AnchorRejected('Unsupported component layout: '+label)
            record['struct_name']=structure;record['components']=dict(zip(components,fields))
        self.layout[label]=record

    def pointer(self,owner,label):return self.r.u64(owner+self.layout[label]['offset'])

    def guard(self):
        r=self.r;p=r.profile
        stamp_raw=r.read(self.stamp,4)
        objects=r.read(r.objects_header,16);names=r.read(r.names_header,16)
        _check_registry_header('objects',objects,(r.objects_data,r.object_count,r.object_capacity))
        _check_registry_header('names',names,(r.names_data,r.name_count,r.name_capacity))
        pawn_pointer=self.pointer(self.controller,'controller_pawn')
        if pawn_pointer!=self.pawn:raise AnchorRejected('Controlled Pawn changed; re-resolve required')
        pawn=self.fresh(self.pawn);controller=self.fresh(self.controller)
        camera=self.fresh(self.camera);local=self.fresh(self.local)
        self.validate_controlled(pawn)
        if pawn['name'].startswith('Default__'):
            raise AnchorRejected('Pawn is default object or class changed')
        outer=pawn['outer'];outer_chain=[];seen=set();level=None
        while outer:
            if outer in seen or len(seen)>=16:raise AnchorRejected('Invalid Pawn outer chain')
            seen.add(outer);identity=self.fresh(outer);outer_chain.append(identity)
            if self.derives(identity['cls'],'Level'):
                level=identity;break
            outer=identity['outer']
        if level is None:raise AnchorRejected('Pawn outer chain has no Level')
        if self.pointer(self.pawn,'pawn_controller')!=self.controller:
            raise AnchorRejected('Pawn.Controller reciprocal link mismatch')
        offsets=self.resolved['offsets']
        if (r.u64(self.controller+int(offsets['PlayerCamera'],16))!=self.camera or
            r.u64(self.camera+int(offsets['PCOwner'],16))!=self.controller or
            r.u64(self.controller+int(offsets['Player'],16))!=self.local):
            raise AnchorRejected('Camera/controller/local-player relationship changed')
        target=r.u64(self.view_target) if self.view_target is not None else None
        target_proof=None
        if self.target_validator is not None:
            target_proof=self.target_validator(self,target,pawn,level)
        if self.target_validator is None and target is not None and target!=self.pawn:
            error=AnchorRejected('Camera view target is not controlled Pawn')
            # Bounded failure-only evidence; it never admits a non-Pawn camera.
            observation={'diagnostic_only':True,'target':target,'pawn':self.pawn,
                         'camera':self.camera,'controller':self.controller,
                         'timestamp_raw':stamp_raw.hex(),'consistent_read':False}
            try:
                if target:
                    identity=self.fresh(target);cls=self.fresh(identity['cls'])
                    observation.update(target_identity=identity,target_class=cls)
                    identity_stable=self.fresh(target)==identity and self.fresh(identity['cls'])==cls
                else:identity_stable=True
                observation['consistent_read']=bool(identity_stable and r.u64(self.view_target)==target
                    and self.pointer(self.controller,'controller_pawn')==self.pawn
                    and r.read(self.stamp,4)==stamp_raw and r.read(r.objects_header,16)==objects
                    and r.read(r.names_header,16)==names)
            except (RuntimeError,OSError,ValueError,struct.error) as diagnostic_error:
                observation['read_error']=str(diagnostic_error)
            error.camera_observation=observation
            raise error
        if not math.isfinite(struct.unpack('<f',stamp_raw)[0]):raise AnchorRejected('Nonfinite camera timestamp')
        if r.read(self.stamp,4)!=stamp_raw:raise AnchorRejected('Cross-frame camera timestamp during identity checks')
        # Recheck links after the ending timestamp read as well: a possession
        # switch can race even this final boundary. This remains non-atomic.
        if (self.pointer(self.controller,'controller_pawn')!=self.pawn or
            self.pointer(self.pawn,'pawn_controller')!=self.controller):
            raise AnchorRejected('Pawn/controller pointer changed at bracket boundary')
        if (r.u64(self.controller+int(offsets['PlayerCamera'],16))!=self.camera or
            r.u64(self.camera+int(offsets['PCOwner'],16))!=self.controller or
            r.u64(self.controller+int(offsets['Player'],16))!=self.local):
            raise AnchorRejected('Camera/controller/player pointer changed at bracket boundary')
        if self.view_target is not None and r.u64(self.view_target)!=target:
            raise AnchorRejected('View target changed at bracket boundary')
        if self.target_validator is not None:
            if self.target_validator(self,target,pawn,level)!=target_proof:
                raise AnchorRejected('Cinematic target changed during identity checks')
            if r.read(self.stamp,4)!=stamp_raw:
                raise AnchorRejected('Cross-frame camera timestamp during identity checks')
        result={'pawn':pawn,'controller':controller,'camera':camera,'local_player':local,'level':level,
                'outer_chain':outer_chain,'view_target':target,'timestamp_raw':stamp_raw.hex(),
                'objects_header':objects.hex(),'names_header':names.hex()}
        if self.target_validator is not None:result['target_proof']=target_proof
        return result

    def sample(self):
        record={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'started_monotonic':time.monotonic(),'accepted':False}
        try:
            before=self.guard()
            for key in ('pawn','controller','camera','local_player','level'):
                if before[key]!=self.initial[key]:raise AnchorRejected('Resolved object identity changed: '+key)
            values={};raw={}
            for label,owner,fmt in (('pawn_location',self.pawn,'<3f'),('pawn_rotation',self.pawn,'<3i'),
                    ('controller_rotation',self.controller,'<3i'),('eye_height',self.pawn,'<f'),
                    ('base_eye_height',self.pawn,'<f')):
                if getattr(self,'mounted_contract',None) is not None and owner==self.pawn:continue
                data=self.r.read(owner+self.layout[label]['offset'],struct.calcsize(fmt))
                value=struct.unpack(fmt,data);values[label]=list(value) if len(value)>1 else value[0];raw[label]=data.hex()
            data=self.r.read(self.pov,28);pov=struct.unpack('<3f3if',data)
            values['camera_cache_pov']={'location':list(pov[:3]),'rotation_ru':list(pov[3:6]),'fov':pov[6]};raw['camera_cache_pov']=data.hex()
            if getattr(self,'mounted_contract',None) is not None:
                values.update(pawn_location=list(pov[:3]),pawn_rotation=list(pov[3:6]),eye_height=0.,base_eye_height=0.)
            record.update(values=values,raw_values=raw,guard_before=before)
            after=self.guard();record['guard_after']=after
            if before!=after:raise AnchorRejected('Cross-frame or identity/pointer change during measurement')
            if not all(math.isfinite(v) for v in (*values['pawn_location'],values['eye_height'],values['base_eye_height'],*pov[:3],pov[-1])):
                raise AnchorRejected('Nonfinite anchor values')
            if not 1<pov[-1]<179:raise AnchorRejected('Invalid CameraCache FOV')
            location=values['pawn_location'];height=values['eye_height']
            record.update(accepted=True,camera_timestamp=struct.unpack('<f',bytes.fromhex(before['timestamp_raw']))[0],
                          candidate_eye_hypothesis=[location[0],location[1],location[2]+height])
        except (RuntimeError,OSError,ValueError,struct.error) as error:
            record['rejection']=str(error)
        record['finished_monotonic']=time.monotonic()
        return record


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--seconds',type=float,default=10)
    parser.add_argument('--interval',type=float,default=.02)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args(argv)
    if args.pid<=0 or not 0<args.seconds<=60 or not .005<=args.interval<=1:parser.error('Invalid bounded sampling parameters')
    report_path=args.output.with_suffix('.json')
    if args.output.exists() or report_path.exists():raise RuntimeError('Preserve existing measurement')
    from resolve_camera import Resolver
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    resolver=Resolver(args.pid,profile);records=[];report={'pid':args.pid,'renderer_alignment_verified':False,'atomic_snapshot_verified':False}
    try:
        sampler=AnchorSampler(resolver);report.update(sampler.metadata)
        start=time.monotonic()
        while time.monotonic()-start<args.seconds:
            records.append(sampler.sample());time.sleep(args.interval)
        accepted=[r for r in records if r['accepted']]
        reasons={}
        for row in records:
            if not row['accepted']:reasons[row['rejection']]=reasons.get(row['rejection'],0)+1
        report.update(samples=len(records),accepted=len(accepted),rejected=len(records)-len(accepted),rejection_counts=reasons,
                      distinct_accepted_camera_timestamps=len({r['camera_timestamp'] for r in accepted}),
                      elapsed_seconds=time.monotonic()-start,records=records,passed=bool(accepted),
                      passed_policy='At least one bracket-consistent read; not an atomic/render/standing acceptance gate')
    except Exception as error:report.update(error=repr(error),passed=False)
    finally:resolver.p.close()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open('w',newline='',encoding='utf-8') as stream:
        writer=csv.writer(stream)
        keys=('started_utc','started_monotonic','finished_monotonic','accepted','camera_timestamp','rejection')
        columns=['pawn_'+a for a in 'xyz']+['pawn_'+a+'_ru' for a in ('pitch','yaw','roll')]+[
            'controller_'+a+'_ru' for a in ('pitch','yaw','roll')]+['eye_height','base_eye_height']+[
            'camera_'+a for a in 'xyz']+['camera_'+a+'_ru' for a in ('pitch','yaw','roll')]+['camera_fov']+[
            'candidate_eye_'+a+'_hypothesis' for a in 'xyz']
        writer.writerow([*keys,*columns,'raw_record_json'])
        for row in records:
            values=row.get('values',{});pov=values.get('camera_cache_pov',{})
            flatten=[*values.get('pawn_location',['']*3),*values.get('pawn_rotation',['']*3),
                *values.get('controller_rotation',['']*3),values.get('eye_height',''),values.get('base_eye_height',''),
                *pov.get('location',['']*3),*pov.get('rotation_ru',['']*3),pov.get('fov',''),
                *row.get('candidate_eye_hypothesis',['']*3)]
            writer.writerow([row.get(k,'') for k in keys]+flatten+[json.dumps(row,separators=(',',':'))])
    report_path.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({k:report.get(k) for k in ('pid','passed','samples','accepted','rejected','rejection_counts','error','renderer_alignment_verified')}))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
