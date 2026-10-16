"""Read-only reflected anchor certificate for the native observer (default off).

A stable bracket is evidence of consistency, not an atomic render snapshot or
proof that Pawn.Location + EyeHeight is the rendered eye.
"""
import struct
import time
from sample_player_anchor import AnchorSampler, AnchorRejected, RegistryHeaderChanged

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


class LiveRegistryAnchorSampler(AnchorSampler):
    """Refresh live registry bounds; count growth alone does not reject identity."""
    def __init__(self,resolver,clock_ns=None,*,prepared_camera=None,cinematic_camera=False):
        # Base construction invokes self.guard before it creates metadata.
        self.guard_acquisitions=[];self.guard_stage='initial'
        self.guard_clock_ns=clock_ns or time.perf_counter_ns
        self.sample_guard_index=None
        self.cinematic_contract=None
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
            temporary=error is not None and str(error)=='Cross-frame camera timestamp during identity checks' and isinstance(error,AnchorRejected)
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
        self.sample_guard_index=0
        try:return super().sample()
        finally:self.sample_guard_index=None

    def fresh(self,address):
        result = super().fresh(address)
        r=self.r;p=r.profile
        name_index = r.u32(address+p['object_name'])
        if not 0 <= name_index < r.name_count:
            raise AnchorRejected('Live name registry index out of range')
        entry=r.u64(r.names_data+name_index*8)
        if not entry or r.u32(entry+p['name_entry_index']) != name_index:
            raise AnchorRejected('Live name entry identity mismatch')
        raw=r.read(entry+p['name_entry_text'],128)
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
    """Five independent strict attempts within200ms; failed values never reused.

    The deadline prevents subsequent calls, not preemption of an OS memory read.
    A result finishing outside the budget is rejected even when its bracket held.
    """
    started=clock_ns();deadline=started+200000000
    evidence={'accepted':False,'stage':'sample','clock':'perf_counter_ns','started_perf_counter_ns':started,
              'deadline_perf_counter_ns':deadline,'maximum_attempts':5,'attempts':[],
              'reason':None,'exception_type':None,'exception_repr':None,'raw_rejection':None}
    last_reason='Anchor sample acquisition deadline exceeded'
    for index in range(5):
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
                budget_scope='sample acquisition5/200ms; each nested guard independently5/200ms; not total5reads')
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
            evidence.update(accepted=True,finished_perf_counter_ns=finished)
            return sample,evidence
        if temporary is None:
            evidence.update(finished_perf_counter_ns=finished,failure_reason=last_reason)
            raise AnchorSamplingRejected(last_reason,evidence)
    evidence.update(finished_perf_counter_ns=clock_ns(),failure_reason=last_reason)
    raise AnchorSamplingRejected(last_reason,evidence)


def build_anchor_config(resolver, expected_camera=None, *, prepared_camera=None,cinematic_camera=False):
    sampler=LiveRegistryAnchorSampler(resolver,prepared_camera=prepared_camera,cinematic_camera=cinematic_camera)
    started=time.perf_counter_ns()
    try:return _assemble_anchor_config(sampler,resolver,expected_camera)
    except AnchorSamplingRejected:raise
    except Exception as error:
        raise AnchorSamplingRejected(str(error),sampler.failure_evidence(sampler.guard_stage,error,started)) from error


def _assemble_anchor_config(sampler,resolver,expected_camera):
    sampler.guard_stage='startup_validation'
    if sampler.view_target is None:
        raise AnchorRejected('Anchor observer requires reflected Camera.ViewTarget.Target')
    if expected_camera is not None:
        for key in ('camera_address','controller_address','local_player_address','timestamp_address','pov_address'):
            if sampler.resolved[key] != expected_camera[key]:
                raise AnchorRejected('Anchor camera changed during startup: '+key)
    sampler.guard_stage='sample'
    sample,acquisition=acquire_anchor_sample(sampler)
    sampler.guard_stage='certificate_assembly'
    before=sample['guard_after'];identities=[];indices={}
    def add(address,check_super=False):
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
    metadata={'schema':1,'config':config,'sampler':sampler.metadata,'sample':sample,
              'sample_acquisition':acquisition,
              'guard_acquisitions':sampler.guard_acquisitions,
              'strict_guard_attempts':sum(len(x['attempts']) for x in sampler.guard_acquisitions),
              'acquisition_budget_semantics':'sample5/200ms; each guard independently5/200ms; not total5reads; late success rejected',
              'registry_policy':'Refresh current data/count/capacity and validate selected slots; no startup count equality requirement',
              'read_only':True,'renderer_alignment_verified':False,
              'candidate_eye_formula':'Pawn.Location + world-up EyeHeight; unverified hypothesis only',
              'certificate_final_guard':final,
              'assembly_semantics':'Static reflected identity/offset certificate; timestamp advance and registry growth are allowed across assembly. Native observations independently revalidate slots, links and strict short timestamp brackets.'}
    return config,metadata


def validate_anchor_config(config):
    expected={'schema','objects_header','names_header','layout','roles','fields','identities'}
    schema=config.get('schema')
    if schema==2:expected.add('cinematic')
    if set(config)!=expected or type(schema) is not int or schema not in (1,2):raise AnchorRejected('Unsupported observer schema')
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
    if schema==2:
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
                world['name'].startswith('Default__') or world['outer']!=ids[config['roles']['level']]['address']):
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
    rows=['[AnchorObserver]',f"schema={config['schema']}"]
    if config['schema']==2:
        rows.append('cinematic_enabled=1')
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
