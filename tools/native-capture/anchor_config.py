"""Read-only reflected anchor certificate for the native observer (default off).

A stable bracket is evidence of consistency, not an atomic render snapshot or
proof that Pawn.Location + EyeHeight is the rendered eye.
"""
import struct
import time
import datetime
import math
from sample_player_anchor import AnchorSampler, AnchorRejected, RegistryHeaderChanged
from mounted_context import MountedTimestampChanged

LAYOUT = ('object_internal_index','object_name','object_class','object_outer',
          'field_super','name_entry_index','name_entry_text')
ROLES = ('controller','local_player','camera','pawn','level')
FIELDS = ('controller_pawn','pawn_controller','pawn_location','pawn_rotation',
          'controller_rotation','eye_height','base_eye_height')


def _header(resolver, address):
    data, count, capacity = struct.unpack('<Qii', resolver.read(address,16))
    if not data or not 0 < count <= capacity < 4000000:
        raise AnchorRejected('Invalid live registry header')
    return data,count,capacity


def _check_mounted_bracket(before, after):
    if before == after:return
    camera_fields = {'camera_timestamp_raw', 'camera_pov_raw'}
    # Both complete captures already passed their own live validation. A newer
    # camera frame rejects this attempt too, but can use the existing retry
    # budget only when every non-camera value and the entire key set agree.
    if (set(before) == set(after) and camera_fields <= set(before) and
            before['camera_timestamp_raw'] != after['camera_timestamp_raw'] and
            {key:value for key,value in before.items() if key not in camera_fields} ==
            {key:value for key,value in after.items() if key not in camera_fields}):
        raise MountedTimestampChanged('Mounted camera frame changed during anchor bracket')
    raise AnchorRejected('Mounted context changed during anchor bracket')


class LiveRegistryAnchorSampler(AnchorSampler):
    """Refresh live registry bounds; count growth alone does not reject identity."""
    def __init__(self,resolver,clock_ns=None,*,prepared_camera=None,cinematic_camera=False):
        # Base construction invokes self.guard before it creates metadata.
        self.guard_acquisitions=[];self.guard_stage='initial'
        self.guard_clock_ns=clock_ns or time.perf_counter_ns
        self.sample_guard_index=None
        self.cinematic_contract=None
        self.mounted_contract=None
        def validate_target(sampler,target,pawn,level):
            if self.cinematic_contract is None:
                from cinematic_anchor import CinematicTargetCertificate
                self.cinematic_contract=CinematicTargetCertificate(sampler)
            return self.cinematic_contract.target(sampler,target,pawn,level)
        started=self.guard_clock_ns()
        try:super().__init__(resolver,prepared_camera=prepared_camera,
                             target_validator=validate_target if cinematic_camera else None)
        except AnchorSamplingRejected:raise
        except Exception as error:
            raise AnchorSamplingRejected(str(error),self.failure_evidence('initial',error,started)) from error

    def validate_controlled(self,pawn):
        if self.derives(pawn['cls'],'WarPawn'):return
        if not self.derives(pawn['cls'],'Turret'):
            raise AnchorRejected('Controlled object is neither WarPawn nor certified Turret')

    def controlled_identity(self,controller):
        if self.pawn:return super().controlled_identity(controller)
        r=self.r;p=r.profile;started=self.guard_clock_ns()
        def headers():
            for name,prefix in (('objects','object'),('names','name')):
                expected=(getattr(r,name+'_data'),getattr(r,prefix+'_count'),getattr(r,prefix+'_capacity'))
                observed=_header(r,getattr(r,name+'_header'))
                if observed!=expected:
                    reason='Registry changed during anchor unpossession check'
                    evidence=self.failure_evidence('initial',AnchorRejected(reason),started)
                    evidence['registry_change']=dict(registry=name,expected_header=list(expected),observed_header=list(observed))
                    raise AnchorRegistryChanged(reason,evidence)
        headers()
        prop=self.layout['controller_pawn'];offset=_offset(prop['offset'],'controller_pawn')
        field=self.fresh(prop['address']);kind=self.fresh(field['cls'])
        metaclass=self.fresh(kind['cls'])
        if (controller['name'].startswith('Default__') or field['name']!='Pawn' or field['number'] or
                prop['type']!='ObjectProperty' or kind['name']!='ObjectProperty' or kind['number'] or
                metaclass['name']!='Class' or metaclass['number']):
            raise AnchorRejected('Invalid reflected controller Pawn during unpossession')
        supers=[];seen=set();current=controller['cls']
        while current and current not in seen and len(seen)<64:
            seen.add(current);item=self.fresh(current)
            if item['number']or self.fresh(item['cls'])!=metaclass:raise AnchorRejected('Invalid controller class during unpossession')
            parent=r.u64(current+p['field_super']);supers.append((item,parent))
            if current==field['outer']:break
            current=parent
        else:raise AnchorRejected('Controller Pawn property outer mismatch during unpossession')
        def layout():return (struct.unpack('<II',r.read(field['address']+0x70,8)),r.u32(field['address']+p['property_offset_internal']))
        if layout()!=((1,8),offset):raise AnchorRejected('Controller Pawn property layout mismatch during unpossession')
        after=self.pointer(self.controller,'controller_pawn')
        if (self.fresh(self.controller)!=controller or self.fresh(field['address'])!=field or
                self.fresh(kind['address'])!=kind or self.fresh(metaclass['address'])!=metaclass or
                layout()!=((1,8),offset) or any(self.fresh(item['address'])!=item or
                    r.u64(item['address']+p['field_super'])!=parent for item,parent in supers)):
            raise AnchorRejected('Controller Pawn identity/field changed during unpossession')
        headers()
        if after:self.validate_controlled(self.fresh(after))
        reason='Controller Pawn changed after initial null'if after else'Controller Pawn unavailable during anchor construction'
        evidence=self.failure_evidence('initial',AnchorRejected(reason),started)
        evidence['controlled_link']=dict(controller=controller,field=field,offset=offset,before=0,after=after)
        rejection=AnchorControlledLinkChanged if after else AnchorControlledPawnUnavailable
        raise rejection(reason,evidence)

    def failure_evidence(self,stage,error,started):
        return {'accepted':False,'stage':stage,'reason':str(error),
                'exception_type':type(error).__name__,'exception_repr':repr(error),
                'raw_rejection':None,'clock':'perf_counter_ns',
                'started_perf_counter_ns':started,'finished_perf_counter_ns':self.guard_clock_ns(),
                'guard_acquisitions':list(self.guard_acquisitions),
                'strict_guard_attempts':sum(len(x['attempts']) for x in self.guard_acquisitions)}

    def _strict_guard(self):
        self.r.objects_data,self.r.object_count,self.r.object_capacity = _header(self.r,self.r.objects_header)
        self.r.names_data,self.r.name_count,self.r.name_capacity = _header(self.r,self.r.names_header)
        # Reflection discovery belongs to this guard's existing budget, outside
        # the strict timestamp bracket. Both target validations below stay fresh.
        if self.target_validator is not None and self.cinematic_contract is None:
            from cinematic_anchor import CinematicTargetCertificate
            self.cinematic_contract=CinematicTargetCertificate(self)
        if not self.derives(self.fresh(self.pawn)['cls'],'WarPawn'):
            from mounted_context import MountedContextCertificate,mounted_guard_result
            if self.mounted_contract is None:
                self.mounted_contract=MountedContextCertificate(self, defer_capture=True)
            return mounted_guard_result(self.mounted_contract.capture())
        return super().guard()

    def guard(self):
        stage=self.guard_stage
        if self.sample_guard_index is not None:
            stage='sample_before' if self.sample_guard_index==0 else 'sample_after'
            self.sample_guard_index+=1
        clock=self.guard_clock_ns;started=clock();deadline=started+200000000
        evidence={'accepted':False,'stage':stage,'clock':'perf_counter_ns',
                  'started_perf_counter_ns':started,'deadline_perf_counter_ns':deadline,
                  'maximum_attempts':5,'budget_scope':'this guard acquisition only', 'attempts':[]}
        self.guard_acquisitions.append(evidence)
        reason='Anchor guard acquisition deadline exceeded';last_error=None
        for index in range(5):
            begin=clock()
            if begin>=deadline:break
            value=None;error=None
            try:value=self._strict_guard()
            except Exception as failure:error=failure
            finished=clock()
            temporary=isinstance(error,MountedTimestampChanged) or (isinstance(error,AnchorRejected) and
                str(error)=='Cross-frame camera timestamp during identity checks')
            reason=str(error) if error is not None else None
            evidence['attempts'].append({'attempt':index+1,'started_perf_counter_ns':begin,
                'finished_perf_counter_ns':finished,'elapsed_ms':(finished-begin)/1e6,
                'strict_guard_accepted':error is None,'reason':reason,'retryable':temporary,
                'exception_type':type(error).__name__ if error is not None else None,
                'exception_repr':repr(error) if error is not None else None,
                'raw_guard':value if error is None else None})
            if error is not None and hasattr(error,'camera_observation'):
                evidence['attempts'][-1]['camera_observation']=error.camera_observation
            if isinstance(error,RegistryHeaderChanged):evidence['registry_change']=error.registry_change
            last_error=error
            # Never admit even a successful strict bracket after the deadline.
            if finished>deadline:reason='Anchor guard acquisition deadline exceeded';break
            if error is None:
                evidence.update(accepted=True,finished_perf_counter_ns=finished,
                                strict_guard_attempts=len(evidence['attempts']))
                return value
            if not temporary:break
        evidence.update(finished_perf_counter_ns=clock(),failure_reason=reason,
                        strict_guard_attempts=len(evidence['attempts']))
        failure=self.failure_evidence(stage,last_error or AnchorRejected(reason),started)
        failure['failure_reason']=reason
        if isinstance(last_error,RegistryHeaderChanged):
            failure['registry_change']=last_error.registry_change
            raise AnchorRegistryChanged(reason,failure) from last_error
        raise AnchorSamplingRejected(reason,failure) from last_error

    def sample(self):
        # Mounted uses one explicitly two-layer acquisition, not two expensive
        # full guards forced to share a camera frame. Its inner endpoints are
        # independently read, including roles, slots, links and actual POV.
        if self.mounted_contract is not None:
            from mounted_context import mounted_guard_result
            record={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    'started_monotonic':time.monotonic(),'accepted':False,
                    'sampling_contract':'single_guard_two_layer_mounted_inner_endpoints'}
            previous_stage=self.guard_stage
            self.guard_stage='sample_two_layer';self.sample_guard_index=None
            try:
                guarded=self.guard();proof=guarded['mounted_context']
                before=mounted_guard_result(proof,'inner_before')
                record['guard_before']=before
                after=mounted_guard_result(proof,'inner_after');record['guard_after']=after
                for key in ROLES:
                    if before[key]!=self.initial[key]:raise AnchorRejected('Resolved object identity changed: '+key)
                pov=struct.unpack('<3f3if',bytes.fromhex(proof['validation']['inner_before']['pov_raw']))
                if not all(math.isfinite(x) for x in (*pov[:3],pov[-1])) or not 1<pov[-1]<179:
                    raise AnchorRejected('Invalid CameraCache FOV or values')
                values=dict(camera_cache_pov=dict(location=list(pov[:3]),rotation_ru=list(pov[3:6]),fov=pov[6]),
                            pawn_location=list(pov[:3]),pawn_rotation=list(pov[3:6]),eye_height=0.,base_eye_height=0.)
                rotation_raw=proof['validation']['inner_before']['controller_rotation_raw']
                if rotation_raw is None:raise AnchorRejected('Missing mounted controller rotation evidence')
                values['controller_rotation']=list(struct.unpack('<3i',bytes.fromhex(rotation_raw)))
                record.update(accepted=True,values=values,raw_values={'camera_cache_pov':proof['camera_pov_raw'],
                              'controller_rotation':rotation_raw},
                              camera_timestamp=struct.unpack('<f',bytes.fromhex(before['timestamp_raw']))[0],
                              candidate_eye_hypothesis=list(pov[:3]),geometry_source='CameraCache.POV')
            except (RuntimeError,OSError,ValueError,struct.error) as error:
                record['rejection']=str(error)
            finally:
                self.sample_guard_index=None;self.guard_stage=previous_stage
            record['finished_monotonic']=time.monotonic()
            return record
        self.sample_guard_index=0
        try:
            result=super().sample()
            if self.mounted_contract is not None and result.get('accepted'):
                result['candidate_eye_hypothesis']=list(result['values']['camera_cache_pov']['location'])
                result['geometry_source']='CameraCache.POV'
            return result
        finally:self.sample_guard_index=None

    def fresh(self,address):
        mounted_pass = getattr(self, "_mounted_read_pass", None)
        if mounted_pass is not None:
            return mounted_pass.fresh(address)
        result = super().fresh(address)
        r=self.r;p=r.profile
        name_index = r.u32(address+p['object_name'])
        if not 0 <= name_index < r.name_count:
            raise AnchorRejected('Live name registry index out of range')
        entry=r.u64(r.names_data+name_index*8)
        if not entry:raise AnchorRejected('Live name entry identity mismatch')
        entry_raw=None
        if (p['name_entry_index'],p['name_entry_text'])==(0,20):
            # This known layout spans exactly 148 bytes. Unknown layouts and
            # failed/short combined reads keep the original separate reads.
            try:entry_raw=r.read(entry,148)
            except (RuntimeError,OSError):pass
            if entry_raw is not None and len(entry_raw)!=148:entry_raw=None
        entry_index=struct.unpack_from('<I',entry_raw)[0] if entry_raw is not None else r.u32(entry+p['name_entry_index'])
        if entry_index != name_index:
            raise AnchorRejected('Live name entry identity mismatch')
        raw=entry_raw[20:148] if entry_raw is not None else r.read(entry+p['name_entry_text'],128)
        end=next((i for i in range(0,128,2) if raw[i:i+2]==b'\0\0'),None)
        if end is None:raise AnchorRejected('Observer name exceeds 63 UTF16 units')
        name=raw[:end].decode('utf-16-le',errors='strict')
        if not name or any(c in name for c in '\r\n\0') or name!=result['name']:
            raise AnchorRejected('Name registry identity changed')
        result['name_index']=name_index
        return result


def _offset(value,label):
    if isinstance(value,bool) or not isinstance(value,int) or not 0 <= value < 0x10000:
        raise AnchorRejected('Unsupported observer offset: '+label)
    return value


class AnchorSamplingRejected(AnchorRejected):
    def __init__(self,reason,evidence):
        super().__init__(reason);self.evidence=evidence


class AnchorRegistryChanged(AnchorSamplingRejected):
    """Strict registry header mismatch; requires full Resolver reacquisition."""


class AnchorControlledPawnUnavailable(AnchorSamplingRejected):
    """A recertified controller/property link remains null; no anchor accepted."""


class AnchorControlledLinkChanged(AnchorSamplingRejected):
    """The recertified null link now selects a valid Pawn; reacquire in full."""


def timestamp_guard_retryable(error):
    """Authorize a fresh read-only acquisition, never a rejected certificate.

    Require the real sampler exception and every failed strict attempt. Public
    diagnostic summaries and a matching message alone grant no retry policy.
    """
    if type(error)is not AnchorSamplingRejected:return False
    evidence=getattr(error,'evidence',None)
    if not isinstance(evidence,dict):return False
    guards=evidence.get('guard_acquisitions')
    if not isinstance(guards,list) or not guards or not isinstance(guards[-1],dict):return False
    guard=guards[-1];attempts=guard.get('attempts')
    if guard.get('accepted')is not False or not isinstance(attempts,list) or not 1<=len(attempts)<=5:return False
    allowed={('AnchorRejected','Cross-frame camera timestamp during identity checks'),
             ('MountedTimestampChanged','Mounted camera timestamp changed at bracket boundary'),
             ('MountedTimestampChanged','Mounted camera frame changed during anchor bracket')}
    reason=error.args[0] if len(error.args)==1 else None
    reasons={item[1]for item in allowed}|{'Anchor guard acquisition deadline exceeded'}
    if type(reason)is not str or reason not in reasons or guard.get('failure_reason')!=reason:return False
    return all(isinstance(item,dict) and item.get('strict_guard_accepted')is False and
        item.get('retryable')is True and type(item.get('exception_type'))is str and type(item.get('reason'))is str and
        (item['exception_type'],item['reason'])in allowed for item in attempts)


def _temporary_sample_rejection(sample):
    reason=sample.get('rejection','Unknown anchor sampling failure')
    if reason=='Cross-frame camera timestamp during identity checks':return 'cross_frame_timestamp'
    if reason!='Cross-frame or identity/pointer change during measurement':return None
    before,after=sample.get('guard_before'),sample.get('guard_after')
    if not isinstance(before,dict) or not isinstance(after,dict):return None
    stable=(*ROLES,'outer_chain','view_target')
    if any(key not in before or key not in after or before[key]!=after[key] for key in stable):return None
    allowed={'timestamp_raw','objects_header','names_header'}
    changed={key for key in set(before)|set(after) if before.get(key)!=after.get(key)}
    if not changed or not changed<=allowed:return None
    for key in ('objects_header','names_header'):
        if key not in changed:continue
        try:
            old=struct.unpack('<Qii',bytes.fromhex(before[key]));new=struct.unpack('<Qii',bytes.fromhex(after[key]))
        except (KeyError,ValueError,struct.error):return None
        if not old[0] or not new[0] or not (0<old[1]<=old[2]<4000000 and 0<new[1]<=new[2]<4000000) or new[1]<=old[1]:return None
    return 'cross_frame_timestamp_or_valid_registry_growth'


def acquire_anchor_sample(sampler,clock_ns=time.perf_counter_ns):
    """Up to32 independent strict attempts within200ms; failed values never reused.

    The deadline prevents subsequent calls, not preemption of an OS memory read.
    A result finishing outside the budget is rejected even when its bracket held.
    """
    started=clock_ns();deadline=started+200000000
    evidence={'accepted':False,'stage':'sample','clock':'perf_counter_ns','started_perf_counter_ns':started,
              'deadline_perf_counter_ns':deadline,'maximum_attempts':32,'attempts':[],
              'reason':None,'exception_type':None,'exception_repr':None,'raw_rejection':None}
    last_reason='Anchor sample acquisition deadline exceeded'
    for index in range(32):
        begin=clock_ns()
        if begin>=deadline:break
        receipts=getattr(sampler,'guard_acquisitions',None)
        guard_start=len(receipts) if isinstance(receipts,list) else 0
        try:sample=sampler.sample()
        except (RuntimeError,OSError,ValueError,struct.error) as error:
            sample={'accepted':False,'rejection':str(error)}
            caught_error=error
        else:caught_error=None
        finished=clock_ns();accepted=sample.get('accepted') is True
        temporary=None if accepted else _temporary_sample_rejection(sample)
        last_reason=sample.get('rejection','Unknown anchor sampling failure')
        evidence['attempts'].append({'attempt':index+1,'started_perf_counter_ns':begin,
            'finished_perf_counter_ns':finished,'elapsed_ms':(finished-begin)/1e6,
            'strict_sample_accepted':accepted,'reason':None if accepted else last_reason,
            'temporary_reason':temporary,
            'exception_type':type(caught_error).__name__ if caught_error is not None else None,
            'exception_repr':repr(caught_error) if caught_error is not None else None})
        if isinstance(receipts,list):
            fresh_receipts=receipts[guard_start:]
            evidence['attempts'][-1].update(guard_acquisition_indices=list(range(guard_start,len(receipts))),
                strict_guard_attempts=sum(len(x['attempts']) for x in fresh_receipts))
            evidence.update(guard_acquisitions=list(receipts),
                strict_guard_attempts=sum(len(x['attempts']) for x in receipts),
                budget_scope='sample acquisition32/200ms; each nested guard independently5/200ms; not total32reads')
            # Base sample catches AnchorRejected. Preserve a terminal guard-layer
            # rejection instead of treating its text as a new sample retry cue.
            failed_guard=next((x for x in fresh_receipts if not x['accepted']),None)
            if failed_guard is not None:
                evidence.update(stage=failed_guard['stage'],finished_perf_counter_ns=finished,
                                failure_reason=failed_guard['failure_reason'])
                last=failed_guard['attempts'][-1] if failed_guard['attempts'] else {}
                partial={key:sample.get(key) for key in
                    ('rejection','guard_before','guard_after','raw_values')}
                evidence['attempts'][-1]['raw_rejection']=partial
                evidence.update(reason=last.get('reason'),exception_type=last.get('exception_type'),
                    exception_repr=last.get('exception_repr'),raw_rejection=partial)
                if 'registry_change' in failed_guard:
                    evidence['registry_change']=failed_guard['registry_change']
                    raise AnchorRegistryChanged(failed_guard['failure_reason'],evidence)
                # An exhausted strict guard supplies no certificate for this
                # attempt. A timestamp-only rejection can, however, be tried
                # again from scratch within the outer 32/200 ms budget. Keep
                # every failed guard in the receipt; never reuse its values.
                timestamp_only=(failed_guard.get('failure_reason')==
                                'Cross-frame camera timestamp during identity checks'
                                and bool(failed_guard.get('attempts'))
                                and all(row.get('retryable') is True
                                        for row in failed_guard['attempts']))
                evidence['attempts'][-1]['exhausted_guard_retryable']=bool(timestamp_only and not accepted)
                if accepted or not timestamp_only:
                    raise AnchorSamplingRejected(failed_guard['failure_reason'],evidence)
        if not accepted:
            # Guard identities/header bytes are finite bounded diagnostic data.
            # Keep their originals for independent retry-classification review.
            evidence['attempts'][-1]['raw_rejection']={key:sample.get(key) for key in
                ('rejection','guard_before','guard_after','raw_values')}
            evidence.update(reason=last_reason,
                exception_type=evidence['attempts'][-1]['exception_type'],
                exception_repr=evidence['attempts'][-1]['exception_repr'],
                raw_rejection=evidence['attempts'][-1]['raw_rejection'])
        if finished>deadline:
            last_reason='Anchor sample acquisition deadline exceeded';break
        if accepted:
            evidence.update(accepted=True,stage='sample',reason=None,
                            exception_type=None,exception_repr=None,
                            raw_rejection=None,finished_perf_counter_ns=finished)
            return sample,evidence
        if temporary is None:
            evidence.update(finished_perf_counter_ns=finished,failure_reason=last_reason)
            raise AnchorSamplingRejected(last_reason,evidence)
    evidence.update(finished_perf_counter_ns=clock_ns(),failure_reason=last_reason)
    raise AnchorSamplingRejected(last_reason,evidence)


def build_anchor_config(resolver, expected_camera=None, *, prepared_camera=None,cinematic_camera=False,sample_pose=True,
                        reuse_assembly_identities=True):
    if type(sample_pose)is not bool:raise ValueError('sample_pose must be boolean')
    if type(reuse_assembly_identities)is not bool:raise ValueError('reuse_assembly_identities must be boolean')
    sampler=LiveRegistryAnchorSampler(resolver,prepared_camera=prepared_camera,cinematic_camera=cinematic_camera)
    started=time.perf_counter_ns()
    try:return _assemble_anchor_config(sampler,resolver,expected_camera,sample_pose=sample_pose,
                                      reuse_assembly_identities=reuse_assembly_identities)
    except AnchorSamplingRejected:raise
    except Exception as error:
        raise AnchorSamplingRejected(str(error),sampler.failure_evidence(sampler.guard_stage,error,started)) from error


def _assemble_anchor_config(sampler,resolver,expected_camera,*,sample_pose=True,reuse_assembly_identities=True):
    sampler.guard_stage='startup_validation'
    if sampler.view_target is None:
        raise AnchorRejected('Anchor observer requires reflected Camera.ViewTarget.Target')
    if expected_camera is not None:
        for key in ('camera_address','controller_address','local_player_address','timestamp_address','pov_address'):
            if sampler.resolved[key] != expected_camera[key]:
                raise AnchorRejected('Anchor camera changed during startup: '+key)
    if sample_pose:
        sampler.guard_stage='sample'
        sample,acquisition=acquire_anchor_sample(sampler)
        before=sample['guard_after']
    else:
        # Static identity/offset assembly uses no sampled position or rotation.
        # Preserve the complete short identity/link/timestamp guard. Native
        # admission and every rendered observation still validate live pose.
        sampler.guard_stage='certificate_guard'
        before=sampler.guard();sample=None
        acquisition=dict(kind='static_certificate_identity_guards',pose_sampled=False,
                         accepted=None,pose_observation_authority=False)
    sampler.guard_stage='certificate_assembly'
    identities=[];indices={}
    def add(address,check_super=False):
        # The full guards surrounding assembly and final per-identity live
        # reads below remain mandatory. Avoid repeatedly assembling the same
        # class identity/superchain; no hint survives this one acquisition.
        if reuse_assembly_identities and address in indices:
            stored=identities[indices[address]]
            if check_super and not stored['check_super']:
                stored['check_super']=1;stored['super']=resolver.u64(address+resolver.profile['field_super'])
            return indices[address]
        identity=sampler.fresh(address)
        if address not in indices:
            if len(identities)>=128:raise AnchorRejected('Observer identity certificate exceeds128')
            indices[address]=len(identities)
            identities.append(dict(identity,check_super=0,super=0))
        index=indices[address];stored=identities[index]
        if any(stored[key]!=identity[key] for key in identity):
            raise AnchorRejected('Identity changed during certificate assembly')
        if check_super:
            stored['check_super']=1;stored['super']=resolver.u64(address+resolver.profile['field_super'])
        return index
    roles={role:add(before[role]['address']) for role in ROLES}
    for identity in before['outer_chain']:add(identity['address'])
    mounted=None
    if sampler.mounted_contract is not None:
        proof=before['mounted_context']
        for address in proof['identities']:add(address,address in proof['supers'])
        mounted=dict(role_driver=add(proof['roles']['driver']['address']),
            role_inventory=add(proof['roles']['inventory']['address']),
            role_world_info=add(proof['roles']['world_info']['address']),
            property_offset_internal=resolver.profile['property_offset_internal'],
            properties={name:dict(role=add(value['identity']['address']),offset=value['offset'])
                        for name,value in proof['properties'].items()})
    contract=getattr(sampler,'cinematic_contract',None)
    cinematic=None
    if contract is not None:
        cinematic=dict(module_base=resolver.base,
            role_cinematic_property=add(contract.property['address']),
            role_camera_actor_class=add(contract.camera_class['address']),
            cinematic_offset=contract.offset,cinematic_mask=contract.mask,
            role_world_info_property=add(contract.world_property['address']),
            role_world_info=add(contract.world_info['address']),world_info_offset=contract.world_offset)
        # CameraActor is a class role without a static instance in this table;
        # class-of-object closure alone would leave its own Super uncertified.
        for address in contract.identities:add(address,address in contract.supers)
    # Certify every object's actual class and bounded superclass chain, including
    # metaclass identities. Native rechecks the slots, FNames, class/outer/super.
    cursor=0
    while cursor<len(identities):
        cls=identities[cursor]['cls'];seen=set()
        while cls:
            if cls in seen or len(seen)>=64:raise AnchorRejected('Invalid certificate superclass chain')
            seen.add(cls);index=add(cls,True);cls=identities[index]['super']
        cursor+=1
    fields={label:_offset(sampler.layout[label]['offset'],label) for label in FIELDS}
    for name,key in (('player_camera','PlayerCamera'),('pc_owner','PCOwner'),('player','Player')):
        fields[name]=_offset(int(sampler.resolved['offsets'][key],16),name)
    fields['camera_timestamp']=_offset(sampler.stamp-sampler.camera,'camera_timestamp')
    fields['camera_pov']=_offset(sampler.pov-sampler.camera,'camera_pov')
    fields['camera_view_target']=_offset(sampler.view_target-sampler.camera,'camera_view_target')
    layout={key:_offset(resolver.profile[key],key) for key in LAYOUT}
    sampler.guard_stage='assembly_after'
    after=sampler.guard()
    # MountedContextCertificate.capture independently freezes all mounted role,
    # identity, superclass, property and link records across every guard. Its
    # camera timestamp/POV are allowed to advance between assembly guards.
    stable_keys=(*ROLES,'outer_chain','view_target')
    if cinematic is not None:stable_keys+=('target_proof',)
    if any(before[key]!=after[key] for key in stable_keys):
        raise AnchorRejected('Possession/identity changed during certificate assembly')
    sampler.guard_stage='final_identity_validation'
    for identity in identities:
        fresh=sampler.fresh(identity['address'])
        if any(identity[key]!=fresh[key] for key in fresh):
            raise AnchorRejected('Certificate identity changed at final boundary')
        if identity['check_super'] and resolver.u64(identity['address']+layout['field_super'])!=identity['super']:
            raise AnchorRejected('Superclass changed at final boundary')
    sampler.guard_stage='final_boundary'
    final=sampler.guard() # This guard has its own short timestamp/link bracket.
    if any(after[key]!=final[key] for key in stable_keys):
        raise AnchorRejected('Possession/identity changed at final certificate boundary')
    # Offsets are reflected and type/component checked by AnchorSampler, not
    # copied from guessed executable-relative structures.
    config={'schema':1,'objects_header':resolver.objects_header,'names_header':resolver.names_header,
            'layout':layout,'roles':roles,'fields':fields,'identities':identities}
    if cinematic is not None:config.update(schema=2,cinematic=cinematic)
    if mounted is not None:config.update(schema=3,mounted=mounted)
    metadata={'schema':1,'config':config,'sampler':sampler.metadata,'sample':sample,
              'pose_sampled':sample_pose,
              'sample_acquisition':acquisition,
              'guard_acquisitions':sampler.guard_acquisitions,
              'strict_guard_attempts':sum(len(x['attempts']) for x in sampler.guard_acquisitions),
              'acquisition_budget_semantics':('sample5/200ms; each guard independently5/200ms; not total5reads; late success rejected' if sample_pose else
                  'No diagnostic pose sample; each complete identity guard independently5/200ms; native live pose guards remain required'),
              'registry_policy':'Refresh current data/count/capacity and validate selected slots; no startup count equality requirement',
              'read_only':True,'renderer_alignment_verified':False,
              'candidate_eye_formula':'CameraCache.POV; no Driver location substitution' if mounted else 'Pawn.Location + world-up EyeHeight; unverified hypothesis only',
              'certificate_final_guard':final,
              'assembly_semantics':'Static reflected identity/offset certificate; timestamp advance and registry growth are allowed across assembly. Native observations independently revalidate slots, links and strict short timestamp brackets.'}
    return config,metadata


def validate_anchor_config(config):
    expected={'schema','objects_header','names_header','layout','roles','fields','identities'}
    schema=config.get('schema')
    if schema==2 or schema==3 and 'cinematic' in config:expected.add('cinematic')
    if schema==3:expected.add('mounted')
    if set(config)!=expected or type(schema) is not int or schema not in (1,2,3):raise AnchorRejected('Unsupported observer schema')
    def integer(value,label,limit,positive=False):
        if isinstance(value,bool) or not isinstance(value,int) or not (int(positive)<=value<limit):
            raise AnchorRejected('Invalid observer integer: '+label)
    for key in ('objects_header','names_header'):integer(config[key],key,1<<64,True)
    if set(config['layout'])!=set(LAYOUT):raise AnchorRejected('Missing observer layout')
    if set(config['fields'])!=set(FIELDS)|{'player_camera','pc_owner','player','camera_timestamp','camera_pov','camera_view_target'}:
        raise AnchorRejected('Missing observer reflected field')
    for group in ('layout','fields'):
        for key,value in config[group].items():_offset(value,key)
    ids=config['identities']
    if not isinstance(ids,list) or not 1<=len(ids)<=128:raise AnchorRejected('Invalid identity count')
    addresses=set();slots=set()
    for item in ids:
        if set(item)!={'address','internal_index','name_index','number','cls','outer','name','check_super','super'}:
            raise AnchorRejected('Incomplete identity certificate')
        for key in ('address','cls','outer','super'):integer(item[key],key,1<<64,key in ('address','cls'))
        for key in ('internal_index','name_index'):integer(item[key],key,4000000)
        integer(item['number'],'number',1<<32)
        integer(item['check_super'],'check_super',2)
        name=item['name']
        if not isinstance(name,str) or not name or len(name.encode('utf-16-le'))>126 or any(c in name for c in '\r\n\0'):
            raise AnchorRejected('Invalid certificate name')
        if item['address'] in addresses or item['internal_index'] in slots:raise AnchorRejected('Duplicate identity address/slot')
        addresses.add(item['address']);slots.add(item['internal_index'])
    for item in ids:
        if item['cls'] not in addresses or (item['check_super'] and item['super'] and item['super'] not in addresses):
            raise AnchorRejected('Incomplete certified class chain')
    if set(config['roles'])!=set(ROLES):raise AnchorRejected('Missing observer role')
    if len(set(config['roles'].values()))!=len(ROLES):raise AnchorRejected('Aliased observer roles')
    for role,index in config['roles'].items():
        integer(index,role,len(ids));name=ids[index]['name']
        if name.startswith('Default__'):raise AnchorRejected('Default object observer role')
    if schema==3:
        from mounted_context import validate_mounted_config
        validate_mounted_config(config)
    if 'cinematic' in config:
        cine=config['cinematic']
        if not isinstance(cine,dict) or set(cine)!={'module_base','role_cinematic_property','role_camera_actor_class','cinematic_offset','cinematic_mask','role_world_info_property','role_world_info','world_info_offset'}:
            raise AnchorRejected('Missing cinematic certificate')
        integer(cine['module_base'],'module_base',(1<<64)-0x1b863f0,True)
        integer(cine['cinematic_offset'],'cinematic_offset',0xfffd)
        integer(cine['world_info_offset'],'world_info_offset',0xfff9)
        integer(cine['role_world_info'],'role_world_info',len(ids))
        integer(cine['cinematic_mask'],'cinematic_mask',1<<32,True)
        if cine['cinematic_mask']&(cine['cinematic_mask']-1):raise AnchorRejected('Cinematic mask must be one bit')
        for key,name in (('role_cinematic_property','bCinematicMode'),('role_camera_actor_class','CameraActor'),('role_world_info_property','WorldInfo')):
            integer(cine[key],key,len(ids))
            if ids[cine[key]]['name']!=name or ids[cine[key]]['number']:
                raise AnchorRejected('Cinematic role identity mismatch')
        by_address={i['address']:i for i in ids}
        prop=ids[cine['role_cinematic_property']];actor=ids[cine['role_camera_actor_class']]
        if (by_address[prop['cls']]['name']!='BoolProperty' or by_address[actor['cls']]['name']!='Class' or
                prop['outer'] not in by_address):raise AnchorRejected('Cinematic class/owner mismatch')
        cls=ids[config['roles']['controller']]['cls'];seen=set()
        while cls and cls!=prop['outer']:
            if cls in seen or cls not in by_address or not by_address[cls]['check_super']:
                raise AnchorRejected('Invalid cinematic owner chain')
            seen.add(cls);cls=by_address[cls]['super']
        if cls!=prop['outer']:raise AnchorRejected('Cinematic property not inherited by controller')
        world_prop=ids[cine['role_world_info_property']];world=ids[cine['role_world_info']]
        if (by_address[world_prop['cls']]['name']!='ObjectProperty' or
                by_address.get(world_prop['outer'],{}).get('name')!='Actor' or
                world['name'].startswith('Default__') or
                (world['outer']!=ids[config['roles']['level']]['address'] and
                 not (schema==3 and cine['role_world_info']==config['mounted']['role_world_info']))):
            raise AnchorRejected('Cinematic WorldInfo certificate mismatch')
        def certified_ancestry(start,expected):
            seen=set();current=start
            while current:
                item=by_address.get(current)
                if current in seen or len(seen)>=64 or not item or not item['check_super']:
                    raise AnchorRejected('Incomplete cinematic class ancestry')
                if expected(item):return True
                seen.add(current);current=item['super']
            return False
        for cls in (ids[config['roles']['controller']]['cls'],ids[config['roles']['pawn']]['cls'],actor['address']):
            if not certified_ancestry(cls,lambda item:item['address']==world_prop['outer']):
                raise AnchorRejected('Cinematic actor does not inherit WorldInfo owner')
        if not certified_ancestry(world['cls'],lambda item:item['name']=='WorldInfo'):
            raise AnchorRejected('Cinematic world object is not WorldInfo')
    return config


def anchor_ini(config):
    """Bounded numeric schema with already-validated, newline-free UTF16 names."""
    validate_anchor_config(config)
    # Python schema3 denotes mounted semantics. Native wire4 adds Actor.Owner;
    # wire3's nine-property contract is incompatible in both directions.
    # In particular Native103 rejects wire4 before reading any role/layout.
    wire_schema=4 if config['schema']==3 else config['schema']
    rows=['[AnchorObserver]',f"schema={wire_schema}"]
    if config['schema']==3:
        rows.append('mounted_enabled=1')
        mounted=config['mounted']
        rows.extend(f'mounted_{key}={value}' for key,value in mounted.items() if key!='properties')
        from mounted_context import MOUNTED_PROPERTIES
        for i,name in enumerate(MOUNTED_PROPERTIES):
            rows.extend(f'mounted_property{i}_{key}={value}' for key,value in mounted['properties'][name].items())
        rows.append(f'cinematic_enabled={int("cinematic" in config)}')
    if 'cinematic' in config:
        if config['schema']!=3:rows.append('cinematic_enabled=1')
        rows.extend(f'{key}={value}' for key,value in config['cinematic'].items())
    for key in ('objects_header','names_header'):rows.append(f'{key}={config[key]:#x}')
    for key,value in config['layout'].items():rows.append(f'{key}={value}')
    rows.append(f"identity_count={len(config['identities'])}")
    for index,item in enumerate(config['identities']):
        for key in ('address','internal_index','name_index','number','cls','outer','name','check_super','super'):
            label='index' if key=='internal_index' else key
            rows.append(f'id{index}_{label}={item[key]}')
    for role,index in config['roles'].items():rows.append(f"role_{'local' if role=='local_player' else role}={index}")
    for key,value in config['fields'].items():rows.append(f'{key}={value}')
    return '\n'.join(rows)+'\n'
