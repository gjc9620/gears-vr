"""Read-only live certificate adapter for the continuous owner.

Only the monitor's owner thread signals Prepare/Commit. Worker functions own
and close each RPM/attestation lease and never write game memory.
"""
import configparser
import json
from pathlib import Path
from types import SimpleNamespace

from start_capture import ROOT, Resolver, observer_signatures
from anchor_config import build_anchor_config
from hook_entry_attestation import HookEntryAttestation
from world_rebind import build_candidate, publish_candidate, role, identity, thaw


def snapshot(config):
    from world_rebind_monitor import WorldSnapshot
    cine = config.get('cinematic') or {}
    world_index = cine.get('role_world_info')
    if type(world_index) is not int or not 0 <= world_index < len(config['identities']):
        raise ValueError('World recovery requires certified WorldInfo')
    return WorldSnapshot(**{name:role(config,name) for name in
        ('local_player','controller','camera','level','pawn')},
        world_info=identity(config['identities'][world_index]))


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
        settings = configparser.ConfigParser()
        settings.read(self.dll.with_name('capture.ini'),encoding='utf-16')
        if settings.getint('Capture','pid') != game['pid']:
            raise ValueError('Startup configuration process mismatch')
        self.crosshair_assignment_observer = settings.getboolean('Capture','crosshair_assignment_observer',fallback=False)
        flags = SimpleNamespace(**{key:settings.getboolean('Capture',key,fallback=False) for key in (
            'family_diagnostics','viewport_diagnostics','first_view_mask_control','view_union_mask_fix',
            'shadow_view','shadow_state','shadow_constructor')})
        self.signatures = observer_signatures(flags)

    def observe(self, check_live):
        check_live()
        resolver = Resolver(self.game['pid'],self.profile)
        try:
            camera = resolver.resolve()
            check_live()
            config, _ = build_anchor_config(resolver,camera,prepared_camera=camera,cinematic_camera=True)
            result = snapshot(config)
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
