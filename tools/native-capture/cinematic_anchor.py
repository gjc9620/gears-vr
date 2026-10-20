"""Opt-in CameraActor certificate for this verified Reloaded build.

The controlled Pawn remains the possession/UI anchor. A cinematic target is
accepted separately, with its own identity, only while bCinematicMode is set.
Native BoolProperty semantics were checked against live bytes; see docs/65.
"""
import hashlib
import struct
from sample_player_anchor import AnchorRejected


BOOL_CODE = (
    (0x1d6ea4, 127, 'b71e2f794b38111292c497fa82c685f18a50822c44857853bb46acb6ad88609e'),
    (0x1daba0, 25, 'f4c26fb034bd7529efd10838092da31c9745e7b66f56cd054e793a1957a2915f'),
)


class CinematicTargetCertificate:
    def __init__(self, sampler):
        r = sampler.r
        self.r = r
        self.identities = {}
        self.supers = {}
        def remember(address):
            identity = sampler.fresh(address)
            self.identities[address] = identity
            return identity
        pc = remember(sampler.controller)
        prop = r.property(pc['cls'], 'bCinematicMode', True)
        if prop['type'] != 'BoolProperty' or not 0 <= prop['offset'] <= 0xfffc:
            raise AnchorRejected('Unsupported cinematic BoolProperty')
        self.property = remember(prop['address'])
        bool_class = remember(self.property['cls'])
        if (self.property['name'] != 'bCinematicMode' or self.property['number'] or
                bool_class['name'] != 'BoolProperty' or bool_class['number'] or
                remember(bool_class['cls'])['name'] != 'Class'):
            raise AnchorRejected('Cinematic property identity mismatch')
        owner = self.property['outer']
        cls, seen = pc['cls'], set()
        while cls:
            if cls in seen or len(seen) >= 64:
                raise AnchorRejected('Invalid cinematic owner inheritance')
            seen.add(cls)
            identity = remember(cls)
            if remember(identity['cls'])['name'] != 'Class':
                raise AnchorRejected('Cinematic owner is not a class')
            self.supers[cls] = r.u64(cls + r.profile['field_super'])
            if cls == owner:
                break
            cls = self.supers[cls]
        if cls != owner:
            raise AnchorRejected('Cinematic property is not inherited by controller')
        # Reuse discovery's cache first; verify the selected object against live
        # registry/name slots. No cached identity is an acceptance certificate.
        candidates = [x['address'] for x in r.ident_cache.values()
                      if x['name'] == 'CameraActor' and x['number'] == 0]
        candidate_set = set(candidates)
        def addresses():
            yield from candidates
            yield from (x for x in r.object_pointers if x and x not in candidate_set)
        self.camera_class = None
        for address in addresses():
            hint = r.identity(address)
            if hint['name'] != 'CameraActor' or hint['number']:
                continue
            identity = sampler.fresh(address)
            meta = sampler.fresh(identity['cls'])
            if meta['name'] == 'Class' and not meta['number']:
                self.camera_class = remember(address)
                remember(identity['cls'])
                break
        if self.camera_class is None:
            raise AnchorRejected('CameraActor class not found')
        world_prop = r.property(pc['cls'], 'WorldInfo', True)
        if world_prop['type'] != 'ObjectProperty' or not 0 <= world_prop['offset'] <= 0xfff8:
            raise AnchorRejected('Unsupported Actor.WorldInfo property')
        self.world_property = remember(world_prop['address'])
        world_owner = remember(self.world_property['outer'])
        if (self.world_property['name'] != 'WorldInfo' or self.world_property['number'] or
                remember(self.world_property['cls'])['name'] != 'ObjectProperty' or
                world_owner['name'] != 'Actor' or world_owner['number'] or
                remember(world_owner['cls'])['name'] != 'Class'):
            raise AnchorRejected('Invalid Actor.WorldInfo property identity')
        for actor_class in (pc['cls'], sampler.fresh(sampler.pawn)['cls'], self.camera_class['address']):
            reflected = r.property(actor_class, 'WorldInfo', True)
            if reflected != world_prop:
                raise AnchorRejected('Actor classes do not share WorldInfo property')
            cls, seen = actor_class, set()
            while cls and cls != world_owner['address']:
                if cls in seen or len(seen) >= 64:
                    raise AnchorRejected('Invalid Actor.WorldInfo owner chain')
                seen.add(cls); remember(cls)
                self.supers[cls] = r.u64(cls + r.profile['field_super'])
                cls = self.supers[cls]
            if cls != world_owner['address']:
                raise AnchorRejected('WorldInfo property not inherited from Actor')
        self.world_offset = world_prop['offset']
        self.world_info = remember(r.u64(sampler.controller + self.world_offset))
        if (self.world_info['name'].startswith('Default__') or
                not sampler.derives(self.world_info['cls'], 'WorldInfo')):
            raise AnchorRejected('Invalid current WorldInfo object')
        remember(self.world_info['cls'])
        self.offset = prop['offset']
        self.mask = r.u32(self.property['address'] + 0xd0)
        if not self.mask or self.mask & (self.mask - 1):
            raise AnchorRejected('Unsupported cinematic BoolProperty mask')
        self.verify(sampler)

    def verify(self, sampler):
        r, prop = self.r, self.property['address']
        for address, expected in self.identities.items():
            if sampler.fresh(address) != expected:
                raise AnchorRejected('Cinematic certificate identity changed')
        for address, expected in self.supers.items():
            if r.u64(address + r.profile['field_super']) != expected:
                raise AnchorRejected('Cinematic owner inheritance changed')
        vtable = r.u64(prop)
        if (not vtable or r.u64(r.base + 0x1b86280 + 0x2d * 8) != r.base + 0x1d6ea4 or
                r.u64(vtable + 0x2a0) != r.base + 0x1daba0 or
                struct.unpack('<II',r.read(prop + 0x70,8)) != (1,4) or
                r.u32(prop + 0x94) != self.offset or r.u32(prop + 0xd0) != self.mask):
            raise AnchorRejected('Cinematic BoolProperty route/layout changed')
        for rva, size, digest in BOOL_CODE:
            if hashlib.sha256(r.read(r.base + rva, size)).hexdigest() != digest:
                raise AnchorRejected('Cinematic BoolProperty native code changed')
        prop = self.world_property['address']
        if (struct.unpack('<II',r.read(prop + 0x70,8)) != (1,8) or
                r.u32(prop + 0x94) != self.world_offset):
            raise AnchorRejected('Actor.WorldInfo property layout changed')

    def target(self, sampler, target, pawn, level):
        self.verify(sampler)
        r = self.r
        cinematic = bool(r.u32(sampler.controller + self.offset) & self.mask)
        identity = sampler.fresh(target)
        mounted = getattr(sampler, 'mounted_contract', None)
        mounted_world = mounted is not None and mounted.certifies_world(self.world_info)
        if ((self.world_info['outer'] != level['address'] and not mounted_world) or
                any(r.u64(actor + self.world_offset) != self.world_info['address']
                    for actor in (sampler.controller, pawn['address'], target))):
            raise AnchorRejected('Cinematic target WorldInfo does not match controlled world')
        target_level = level
        if target == pawn['address']:
            if identity != pawn:
                raise AnchorRejected('Cinematic Pawn target identity changed')
        else:
            if not cinematic:
                raise AnchorRejected('Camera view target is not controlled Pawn')
            if (identity['name'].startswith('Default__') or
                    identity['cls'] != self.camera_class['address']):
                raise AnchorRejected('Non-Pawn target is not a verified cinematic CameraActor')
            target_level = sampler.fresh(identity['outer'])
            if target_level['cls'] != level['cls'] or target_level['name'].startswith('Default__'):
                raise AnchorRejected('Cinematic target outer is not a registered Level')
        return dict(identity=identity, cinematic=cinematic, target_level=target_level,
                    world_info=self.world_info)
