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

from start_capture import ROOT, Resolver, observer_signatures
from resolve_camera import DiscoveryRegistryChanged
from anchor_config import LiveRegistryAnchorSampler
from hook_entry_attestation import HookEntryAttestation
from world_rebind import build_candidate, publish_candidate, role, identity, thaw

MOUNTED_LEVEL_ROLES = ('controlled','driver','controller','camera','world_info')


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
        self.game, self.owner = dict(game), dict(owner)
        self.profile = json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
        anchor = json.loads(self.dll.with_name('anchor-observer-config.json').read_text())
        if anchor.get('pid') != game['pid'] or anchor.get('read_only') is not True:
            raise ValueError('Startup anchor certificate process mismatch')
        self.baseline = snapshot(anchor['config'])
        self.local = role(anchor['config'],'local_player')
        self._camera_hint = None
        self._camera_roles = None
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

    def _preparation_camera(self, resolver):
        """Reuse discovery locations, never a guard or certificate.

        Acquisition owns this new Resolver and closes it on any rejection.
        Invalid hints force the existing bounded retry to create another reader
        and perform full discovery, including same-address generation changes.
        """
        if self._camera_hint is None:return resolver.resolve()
        try:
            resolver.prepare()
            if resolver.base!=int(self._camera_hint['module_base'],0) or resolver.pid!=self._camera_hint['pid']:
                raise RuntimeError('Cached camera process/module changed')
            reader=LiveRegistryAnchorSampler.__new__(LiveRegistryAnchorSampler);reader.r=resolver
            for expected in self._camera_roles.values():
                if reader.fresh(expected['address'])!=expected:raise RuntimeError('Cached camera role changed')
            return copy.deepcopy(self._camera_hint)
        except (RuntimeError,OSError,ValueError):
            self._camera_hint=self._camera_roles=None
            raise DiscoveryRegistryChanged('Camera discovery hint changed; reacquire with a fresh reader')

    def observe(self, check_live):
        check_live()
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
            if camera is None:camera = resolver.resolve()
            check_live()
            # Discovery needs identities, not a cross-frame measurement of POV
            # and Pawn values. Construction already acquires a strict timestamp
            # bracket with fresh identities, reciprocal links and world proof.
            # Prepare still builds the complete certificate before publication.
            sampler = LiveRegistryAnchorSampler(resolver,prepared_camera=camera,cinematic_camera=True)
            guard = sampler.initial
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
        from world_rebind_monitor import PreparedRebind
        check_live()
        with HookEntryAttestation(self.dll.with_name('hook-entry-manifest.json'),pid=self.game['pid'],
                game_creation=self.game['creation_time'],owner_identity=self.owner,dll=self.dll,
                expected_dll_sha256=self.expected_hash) as proof:
            candidate = build_candidate(self.game['pid'],self.profile,self.local,previous_generation,
                self.owner,self.dll,signatures=self.signatures,attestation=proof,check_live=check_live,
                camera_discovery=self._preparation_camera,
                **({'crosshair_assignment_observer':True} if self.crosshair_assignment_observer else {}))
            actual = snapshot(thaw(candidate.anchor_metadata)['config'])
            if actual != observed:
                raise RuntimeError('World changed during automatic certificate preparation')
            check_live()
            path = self.build_dir / f'world-rebind-{self.game["creation_time"]}-{previous_generation+1}-{request}.ini'
            publish_candidate(candidate,path)
            # After publication always return the receipt, including when shutdown
            # arrives now. Only the owner thread may decide whether to signal it.
            return PreparedRebind(actual,previous_generation,request,path)
