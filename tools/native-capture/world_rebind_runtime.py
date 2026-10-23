"""Read-only live certificate adapter for the continuous owner.

Only the monitor's owner thread signals Prepare/Commit. Worker functions own
and close each RPM/attestation lease and never write game memory.
"""
import configparser
import copy
import json
import struct
from pathlib import Path
from types import SimpleNamespace
from functools import partial

from start_capture import ROOT, Resolver, observer_signatures,observer_preparation_churn
from resolve_camera import DiscoveryRegistryChanged,DiscoveryHintChanged
from anchor_config import LiveRegistryAnchorSampler,AnchorSamplingRejected,timestamp_guard_retryable
from hook_entry_attestation import HookEntryAttestation
from world_rebind import build_candidate, publish_candidate, role, identity, thaw, uint

MOUNTED_LEVEL_ROLES = ('controlled','driver','controller','camera','world_info')


class WorldObservationChanged(RuntimeError):
    """Two valid full snapshots differ before any candidate was published."""
    def __init__(self,observed,current):
        super().__init__('World changed during automatic certificate preparation')
        self.observed,self.current=observed,current


def _read_only_acquisition(function,*args,**kwargs):
    try:return function(*args,**kwargs)
    except Exception as error:
        if timestamp_guard_retryable(error):
            from world_rebind_monitor import RetryablePreparation
            raise RetryablePreparation(error) from error
        if observer_preparation_churn(error):
            from world_rebind_monitor import RetryablePreparation
            raise RetryablePreparation(error,scope='acquisition_churn') from error
        raise


def mounted_levels(config):
    if 'mounted' not in config:return ()
    from mounted_context import validate_mounted_config
    validate_mounted_config(config)
    ids=config['identities'];m=config['mounted'];roles=config['roles']
    actors=(roles['pawn'],m['role_driver'],roles['controller'],roles['camera'],m['role_world_info'])
    by_address={item['address']:item for item in ids}
    return tuple(identity(by_address[ids[index]['outer']]) for index in actors)


def snapshot(config):
    from world_rebind_monitor import WorldSnapshot
    cine = config.get('mounted') or config.get('cinematic') or {}
    world_index = cine.get('role_world_info')
    if type(world_index) is not int or not 0 <= world_index < len(config['identities']):
        raise ValueError('World recovery requires certified WorldInfo')
    return WorldSnapshot(**{name:role(config,name) for name in
        ('local_player','controller','camera','level','pawn')},
        world_info=identity(config['identities'][world_index]),
        mounted_context=tuple(identity(config['identities'][index]) for index in (config['roles']['pawn'],config['mounted']['role_driver'],config['mounted']['role_inventory'])) if 'mounted' in config else (),
        mounted_levels=mounted_levels(config))


class RuntimePreparation:
    def __init__(self, build_dir, dll_name, expected_hash, game, owner):
        self.build_dir = Path(build_dir).resolve()
        self.dll = self.build_dir / dll_name
        # This value must arrive from the already verified launch manifest.
        if (not isinstance(expected_hash,str) or len(expected_hash)!=64 or
                any(ch not in '0123456789abcdef' for ch in expected_hash)):
            raise ValueError('Trusted native manifest SHA256 required for world recovery')
        self.expected_hash = expected_hash
        from native_class_discovery import NativeClassDiscovery
        self._native_discovery=NativeClassDiscovery(self.dll,expected_hash)
        self.game, self.owner = dict(game), dict(owner)
        self.profile = json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
        anchor = json.loads(self.dll.with_name('anchor-observer-config.json').read_text())
        if anchor.get('pid') != game['pid'] or anchor.get('read_only') is not True:
            raise ValueError('Startup anchor certificate process mismatch')
        self.baseline = snapshot(anchor['config'])
        self.local = role(anchor['config'],'local_player')
        self._camera_hint = None
        self._camera_roles = None
        self._remember_cinematic_hint(anchor['config'])
        self._remember_startup_camera(anchor)
        self._topology_fields = {key:anchor['config'].get('fields',{}).get(key)
                                 for key in ('controller_pawn','player_camera')}
        settings = configparser.ConfigParser()
        settings.read(self.dll.with_name('capture.ini'),encoding='utf-16')
        if settings.getint('Capture','pid') != game['pid']:
            raise ValueError('Startup configuration process mismatch')
        self.crosshair_assignment_observer = settings.getboolean('Capture','crosshair_assignment_observer',fallback=False)
        flags = SimpleNamespace(**{key:settings.getboolean('Capture',key,fallback=False) for key in (
            'family_diagnostics','viewport_diagnostics','first_view_mask_control','view_union_mask_fix',
            'shadow_view','shadow_state','shadow_constructor')})
        self.signatures = observer_signatures(flags)

    def _remember_startup_camera(self,anchor):
        """Optional startup discovery locations; no live proof is reused."""
        self._camera_hint=self._camera_roles=None
        try:
            config=anchor['config'];camera=anchor['sampler']['resolver'];offsets=camera['offsets']
            address=uint(camera['camera_address'],'startup camera',positive=True)
            fields={key:uint(config['fields'][key],key,16)for key in
                    ('player_camera','pc_owner','player','camera_timestamp','camera_pov')}
            for field,key in (('player_camera','PlayerCamera'),('pc_owner','PCOwner'),('player','Player')):
                if uint(offsets[key],key,32)!=fields[field]:return False
            cache=uint(offsets['CameraCache'],'CameraCache',32)
            if (cache+uint(offsets['TimeStamp'],'TimeStamp',32)!=fields['camera_timestamp']or
                    cache+uint(offsets['POV'],'POV',32)!=fields['camera_pov']or
                    uint(camera['timestamp_address'],'startup timestamp',positive=True)!=address+fields['camera_timestamp']or
                    uint(camera['pov_address'],'startup POV',positive=True)!=address+fields['camera_pov']or
                    tuple(uint(offsets[key],key,32)for key in ('Location','Rotation','FOV'))!=(0,12,24)):
                return False
            if (not isinstance(camera['executable'],str)or not camera['executable']or
                    str(Path(camera['executable']).resolve()).casefold()!=str(self.game['image']).casefold()):return False
            return self._remember_prepared_camera(SimpleNamespace(current_camera=camera,anchor_metadata=anchor),config)
        except (AttributeError,KeyError,TypeError,ValueError,OSError):return False

    def _remember_cinematic_hint(self, config):
        """Retain a discovery address, never a class identity or certificate."""
        self._cinematic_class_hint=None
        index=config.get('cinematic',{}).get('role_camera_actor_class')
        ids=config.get('identities',())
        if type(index)is int and 0<=index<len(ids):
            item=ids[index]
            address=item.get('address')
            if (item.get('name')=='CameraActor' and item.get('number')==0 and
                    type(address)is int and 0<address<(1<<64)-0x60):
                self._cinematic_class_hint=address

    def _prime_cinematic_hint(self, resolver):
        # Ordinary discovery reads populate only this fresh Resolver's hint
        # cache. CinematicTargetCertificate independently certifies the chosen
        # class, metaclass, live slot/name, properties and superclass links.
        address=getattr(self,'_cinematic_class_hint',None)
        if address is None:return
        try:item=resolver.identity(address)
        except DiscoveryRegistryChanged:raise
        except (RuntimeError,OSError,ValueError):
            self._cinematic_class_hint=None;return
        if not isinstance(item,dict) or item.get('name')!='CameraActor' or item.get('number')!=0:
            self._cinematic_class_hint=None

    def _remember_prepared_camera(self, candidate, config):
        """Seed locations after full acquisition; every later reader rechecks.

        Keep only camera/controller/local identities, with their name indices.
        Certificate assembly and guards remain fresh on every subsequent pass.
        Missing optional hint metadata falls back to ordinary global discovery.
        """
        self._camera_hint=self._camera_roles=None
        try:
            camera=thaw(candidate.current_camera)
            final=candidate.anchor_metadata['certificate_final_guard']
            if (uint(camera['pid'],'hint PID',32,True)!=self.game['pid'] or camera['sha256']!=self.profile['sha256'] or
                    uint(camera['module_base'],'hint module',positive=True)!=uint(config['cinematic']['module_base'],'anchor module',positive=True)):return False
            roles={}
            for name,key in (('local_player','local_player_address'),('controller','controller_address'),('camera','camera_address')):
                selected=final[name];expected=role(config,name)
                if identity(selected)!=expected or expected['address']!=uint(camera[key],'hint role',positive=True):return False
                name_index=uint(selected['name_index'],'hint name index',32)
                if name_index!=config['identities'][config['roles'][name]]['name_index']:return False
                roles[name]=dict(expected,name_index=name_index)
        except (AttributeError,KeyError,TypeError,ValueError):return False
        self._camera_hint,self._camera_roles=copy.deepcopy(camera),roles
        return True

    def changed_hint(self, baseline, check_live):
        """Two pointer reads can schedule discovery; they authorize nothing."""
        if any(type(offset) is not int or not 0 <= offset < 65536 for offset in self._topology_fields.values()):return False
        check_live()
        resolver=Resolver(self.game['pid'],self.profile)
        try:
            controller=baseline.controller[0]
            pawn=struct.unpack('<Q',resolver.read(controller+self._topology_fields['controller_pawn'],8))[0]
            camera=struct.unpack('<Q',resolver.read(controller+self._topology_fields['player_camera'],8))[0]
            check_live()
            # A null pointer is a transition, not a useful discovery hint.
            return bool(pawn and camera and (pawn!=baseline.pawn[0] or camera!=baseline.camera[0]))
        finally:resolver.p.close()

    def _preparation_camera(self, resolver, check_live=None):
        """Reuse discovery locations, never a guard or certificate.

        Acquisition owns this new Resolver and closes it on any rejection.
        Invalid hints force the existing bounded retry to create another reader
        and perform full discovery, including same-address generation changes.
        """
        begin=getattr(resolver,'begin_discovery_field_hints',None)
        if callable(begin):begin()
        native=getattr(self,'_native_discovery',None)
        if native is not None:
            resolver._native_class_discovery=partial(native.discover,check_live=check_live)
            resolver.discovery_pointer_view=native.pointer_view
        if self._camera_hint is None:
            camera=resolver.resolve()
            self._prime_cinematic_hint(resolver)
            return camera
        try:
            resolver.prepare()
            if resolver.base!=int(self._camera_hint['module_base'],0) or resolver.pid!=self._camera_hint['pid']:
                raise RuntimeError('Cached camera process/module changed')
            reader=LiveRegistryAnchorSampler.__new__(LiveRegistryAnchorSampler);reader.r=resolver
            for expected in self._camera_roles.values():
                if reader.fresh(expected['address'])!=expected:raise RuntimeError('Cached camera role changed')
            self._prime_cinematic_hint(resolver)
            return copy.deepcopy(self._camera_hint)
        except (RuntimeError,OSError,ValueError):
            self._camera_hint=self._camera_roles=None
            raise DiscoveryHintChanged('Camera discovery hint changed; reacquire with a fresh reader')

    def observe(self, check_live):
        check_live()
        native=getattr(self,'_native_discovery',None)
        if native is not None:native.warmup()
        resolver = Resolver(self.game['pid'],self.profile)
        try:
            camera = None
            if getattr(self,'_camera_hint',None) is not None:
                try:
                    resolver.prepare()
                    if (resolver.base!=int(self._camera_hint['module_base'],0) or
                            resolver.pid!=self._camera_hint['pid']):
                        raise RuntimeError('Cached camera process/module changed')
                    reader=LiveRegistryAnchorSampler.__new__(LiveRegistryAnchorSampler);reader.r=resolver
                    for expected in self._camera_roles.values():
                        if reader.fresh(expected['address'])!=expected:
                            raise RuntimeError('Cached camera role changed')
                    camera=copy.deepcopy(self._camera_hint)
                except (RuntimeError,OSError,ValueError):
                    # A hint can only remove discovery work. Discard its whole
                    # Resolver on any failure; the fallback starts fresh.
                    resolver.p.close()
                    resolver=Resolver(self.game['pid'],self.profile)
                check_live()
            if camera is None:camera = _read_only_acquisition(resolver.resolve)
            _read_only_acquisition(self._prime_cinematic_hint,resolver)
            check_live()
            # Discovery needs identities, not a cross-frame measurement of POV
            # and Pawn values. Construction already acquires a strict timestamp
            # bracket with fresh identities, reciprocal links and world proof.
            # Prepare still builds the complete certificate before publication.
            sampler = _read_only_acquisition(LiveRegistryAnchorSampler,resolver,prepared_camera=camera,cinematic_camera=True)
            guard = sampler.initial
            contract=getattr(sampler,'cinematic_contract',None)
            if contract is not None:self._cinematic_class_hint=contract.camera_class['address']
            self._camera_hint=copy.deepcopy(camera)
            self._camera_roles={key:copy.deepcopy(guard[key]) for key in ('controller','camera','local_player')}
            from world_rebind_monitor import WorldSnapshot
            result = WorldSnapshot(**{name:guard[name] for name in
                ('local_player','controller','camera','level','pawn')},
                world_info=guard['target_proof']['world_info'],
                mounted_context=tuple(guard['mounted_context']['roles'][key] for key in ('controlled','driver','inventory')) if 'mounted_context' in guard else (),
                mounted_levels=tuple(guard['mounted_context']['levels'][key] for key in MOUNTED_LEVEL_ROLES) if 'mounted_context' in guard else ())
            check_live()
            return result
        finally:
            resolver.p.close()

    def prepare(self, observed, previous_generation, request, check_live):
        return self._prepare(observed,previous_generation,request,check_live)[1]

    def prepare_changed(self, previous_generation, request, accept_snapshot, check_live):
        """Derive observation and certificate from one fresh acquisition.

        A pointer hint or a quiesced recovery selected this path. The full candidate must
        pass the monitor's baseline/change predicate before any publication.
        """
        if not callable(accept_snapshot):raise ValueError('Snapshot acceptance predicate required')
        return self._prepare(None,previous_generation,request,check_live,accept_snapshot)

    def _prepare(self, observed, previous_generation, request, check_live, accept_snapshot=None):
        from world_rebind_monitor import PreparedRebind,WorldSnapshot,RetryablePreparation
        check_live()
        with HookEntryAttestation(self.dll.with_name('hook-entry-manifest.json'),pid=self.game['pid'],
                game_creation=self.game['creation_time'],owner_identity=self.owner,dll=self.dll,
                expected_dll_sha256=self.expected_hash) as proof:
            candidate = _read_only_acquisition(build_candidate,self.game['pid'],self.profile,self.local,previous_generation,
                self.owner,self.dll,signatures=self.signatures,attestation=proof,check_live=check_live,
                sample_pose=False,
                camera_discovery=partial(self._preparation_camera,check_live=check_live),
                **({'crosshair_assignment_observer':True} if self.crosshair_assignment_observer else {}))
            anchor_config=thaw(candidate.anchor_metadata['config'])
            actual = snapshot(anchor_config)
            if accept_snapshot is None and actual != observed:
                # Both reads are full certificates for the same local player.
                # The world may change between them. Reject this unpublished
                # acquisition and require a fresh observation and preparation;
                # invalid/missing observations remain ordinary hard failures.
                if isinstance(observed,WorldSnapshot)and observed.local_player==actual.local_player:
                    error=WorldObservationChanged(observed,actual)
                    raise RetryablePreparation(error,scope='acquisition_churn')from error
                raise RuntimeError('World changed during automatic certificate preparation')
            if accept_snapshot is not None and accept_snapshot(actual) is not True:return actual,None
            self._remember_cinematic_hint(anchor_config)
            self._remember_prepared_camera(candidate,anchor_config)
            check_live()
            path = self.build_dir / f'world-rebind-{self.game["creation_time"]}-{previous_generation+1}-{request}.ini'
            publish_candidate(candidate,path)
            # After publication always return the receipt, including when shutdown
            # arrives now. Only the owner thread may decide whether to signal it.
            return actual,PreparedRebind(actual,previous_generation,request,path)
