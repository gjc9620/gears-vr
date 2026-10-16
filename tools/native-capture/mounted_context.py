"""Read-only Turret possession certificate; no native/runtime admission is enabled.

Consumer supplies a sampler-like object with r, fresh(address), controller,
camera, local, stamp, pov and view_target. These are the existing AnchorSampler
discovery fields; call before its WarPawn-only guard. fresh must validate live
object/name registry slots (LiveRegistryAnchorSampler.fresh does this).
capture() returns independent dictionaries for config serialization. It never
substitutes Driver for Controller.Pawn or computes an eye from pawn location.
Stable repeated reads establish consistency, not an atomic engine snapshot.
"""
import copy
import math
import struct
from sample_player_anchor import AnchorRejected


class MountedContextCertificate:
    def __init__(self, sampler):
        self.sampler = sampler
        self._initial = self._capture()

    def capture(self):
        value = self._capture()
        # Possession changes require a new immutable certificate/rebind. Camera
        # timestamp and POV may advance between separate successful captures.
        for key in ('roles', 'identities', 'supers', 'properties', 'links'):
            if value[key] != self._initial[key]:
                raise AnchorRejected('Mounted certificate changed: ' + key)
        return copy.deepcopy(value)

    def _capture(self):
        s, r = self.sampler, self.sampler.r
        identities, supers, properties, links, reads = {}, {}, {}, {}, {}

        def raw(address, size):
            if not address or address < 0 or address > (1 << 64) - size:
                raise AnchorRejected('Invalid mounted memory address')
            value = r.read(address, size)
            if len(value) != size:
                raise AnchorRejected('Short mounted memory read')
            key = address, size
            if key in reads and reads[key] != value:
                raise AnchorRejected('Mounted memory changed during discovery')
            reads[key] = value
            return value

        def pointer(address):
            return struct.unpack('<Q', raw(address, 8))[0]

        def identity(address):
            value = s.fresh(address)
            if not value['name'] or value['name'].startswith('Default__'):
                raise AnchorRejected('Default/empty mounted identity')
            if address in identities and identities[address] != value:
                raise AnchorRejected('Mounted identity changed during discovery')
            identities[address] = copy.deepcopy(value)
            return value

        def chain(cls):
            result = []
            while cls:
                if cls in result or len(result) >= 64:
                    raise AnchorRejected('Invalid mounted superclass chain')
                value = identity(cls)
                meta = identity(value['cls'])
                if meta['name'] != 'Class' or meta['number'] or value['number']:
                    raise AnchorRejected('Mounted ancestry is not registered classes')
                result.append(cls)
                supers[cls] = pointer(cls + r.profile['field_super'])
                cls = supers[cls]
            return result

        def derives(obj, name):
            if not any(identities[x]['name'] == name for x in chain(obj['cls'])):
                raise AnchorRejected('Mounted class mismatch: ' + name)

        def prop(obj, name, owner_name):
            value = r.property(obj['cls'], name, True)
            offset = value['offset']
            if value['type'] != 'ObjectProperty' or type(offset) is not int or not 0 <= offset <= 0xfff8:
                raise AnchorRejected('Unsupported mounted property: ' + name)
            field = identity(value['address'])
            kind = identity(field['cls'])
            owner = identity(field['outer'])
            if (field['name'] != name or field['number'] or kind['name'] != 'ObjectProperty' or
                    kind['number'] or identity(kind['cls'])['name'] != 'Class' or
                    owner['name'] != owner_name or owner['number'] or
                    owner['address'] not in chain(obj['cls'])):
                raise AnchorRejected('Mounted property owner/type mismatch: ' + name)
            layout = struct.unpack('<II', raw(field['address'] + 0x70, 8))
            live_offset = struct.unpack('<I', raw(field['address'] + r.profile['property_offset_internal'], 4))[0]
            if layout != (1, 8) or live_offset != offset:
                raise AnchorRejected('Mounted property layout mismatch: ' + name)
            properties[name] = dict(identity=field, offset=offset, owner=owner_name)
            return offset

        def link(label, obj, name, owner, expected=None):
            offset = prop(obj, name, owner)
            value = pointer(obj['address'] + offset)
            if expected is not None and value != expected:
                raise AnchorRejected('Mounted reciprocal link mismatch: ' + label)
            links[label] = dict(address=obj['address'] + offset, value=value)
            return value

        # Include exact registry headers and the camera timestamp in the same
        # read bracket as all roles, reflected layouts and pointer relationships.
        for header in (r.objects_header, r.names_header):
            data, count, capacity = struct.unpack('<Qii', raw(header, 16))
            if not data or not 0 < count <= capacity < 4000000:
                raise AnchorRejected('Invalid mounted registry header')
        stamp = raw(s.stamp, 4)
        if not math.isfinite(struct.unpack('<f', stamp)[0]):
            raise AnchorRejected('Invalid mounted camera timestamp')
        pc, camera, local = identity(s.controller), identity(s.camera), identity(s.local)
        controlled = identity(link('controlled', pc, 'Pawn', 'Controller'))
        derives(controlled, 'Turret')
        derives(controlled, 'Vehicle')
        driver = identity(link('driver', controlled, 'Driver', 'Vehicle'))
        derives(driver, 'WarPawn')
        if len({pc['address'], camera['address'], local['address'], controlled['address'], driver['address']}) != 5:
            raise AnchorRejected('Mounted role alias')
        link('possession', controlled, 'Controller', 'Pawn', pc['address'])
        link('driven_vehicle', driver, 'DrivenVehicle', 'Pawn', controlled['address'])
        # Vehicle.DriverEnter calls C.UnPossess before C.Possess(vehicle).
        link('driver_controller', driver, 'Controller', 'Pawn', 0)
        link('camera', pc, 'PlayerCamera', 'PlayerController', camera['address'])
        link('camera_owner', camera, 'PCOwner', 'Camera', pc['address'])
        link('local', pc, 'Player', 'PlayerController', local['address'])
        world = identity(link('world', pc, 'WorldInfo', 'Actor'))
        derives(world, 'WorldInfo')
        for label, obj in (('controlled_world', controlled), ('driver_world', driver), ('camera_world', camera)):
            link(label, obj, 'WorldInfo', 'Actor', world['address'])
        level = identity(controlled['outer'])
        derives(level, 'Level')
        if driver['outer'] != level['address'] or world['outer'] != level['address']:
            raise AnchorRejected('Mounted roles differ in Level')
        inventory = identity(link('inventory', driver, 'InvManager', 'Pawn'))
        derives(inventory, 'WarInventoryManager')
        if inventory['outer'] != driver['address']:
            raise AnchorRejected('Mounted inventory owner mismatch')
        if not s.view_target or pointer(s.view_target) != controlled['address']:
            raise AnchorRejected('Mounted camera target is not controlled turret')
        pov = raw(s.pov, 28)
        values = struct.unpack('<3f3if', pov)
        if not all(math.isfinite(x) for x in (*values[:3], values[-1])) or not 0 < values[-1] < 180:
            raise AnchorRejected('Invalid mounted camera POV')
        for address, expected in identities.items():
            if s.fresh(address) != expected:
                raise AnchorRejected('Mounted identity changed at bracket boundary')
        for (address, size), expected in reads.items():
            if r.read(address, size) != expected:
                raise AnchorRejected('Mounted memory changed at bracket boundary')
        # Camera time and registry bounds close the bracket after the final
        # identity/link reads as well. No retry or stale snapshot substitution.
        for address, size in ((s.stamp, 4), (r.objects_header, 16), (r.names_header, 16)):
            if r.read(address, size) != reads[address, size]:
                raise AnchorRejected('Mounted camera/registry bracket changed')
        roles = dict(controller=pc, camera=camera, local_player=local,
                     controlled=controlled, driver=driver, inventory_pawn=driver,
                     inventory=inventory, camera_target=controlled, level=level, world_info=world)
        return dict(schema=1, kind='turret_driver', roles=roles, identities=identities,
                    supers=supers, properties=properties, links=links,
                    camera_timestamp_raw=stamp.hex(), camera_pov_raw=pov.hex(),
                    atomic_snapshot=False, runtime_verified=False)
