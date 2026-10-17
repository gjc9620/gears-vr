"""Read-only Turret possession certificate used by mounted anchor admission.

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
from sample_player_anchor import AnchorRejected, _check_registry_header


class MountedTimestampChanged(AnchorRejected):
    """Only camera time changed after all other capture boundaries validated."""


class MountedContextCertificate:
    def __init__(self, sampler):
        self.sampler = sampler
        self._initial = self._capture()

    def capture(self):
        value = self._capture()
        # Possession changes require a new immutable certificate/rebind. Camera
        # timestamp and POV may advance between separate successful captures.
        for key in ('roles', 'levels', 'identities', 'supers', 'properties', 'links'):
            if value[key] != self._initial[key]:
                raise AnchorRejected('Mounted certificate changed: ' + key)
        return copy.deepcopy(value)

    def certifies_world(self, identity):
        # Cinematic target validation runs inside the sampler's mounted capture
        # bracket. It may share this exact world while the actor Levels differ.
        return identity == self._initial['roles']['world_info']

    def _capture(self):
        s, r = self.sampler, self.sampler.r
        identities, supers, properties, links, reads = {}, {}, {}, {}, {}
        chains = {}

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
            # Reuse only within this capture. Every visited identity is freshly
            # revalidated below, including class and property identities.
            if address in identities:
                return identities[address]
            value = s.fresh(address)
            if not value['name'] or value['name'].startswith('Default__'):
                raise AnchorRejected('Default/empty mounted identity')
            identities[address] = copy.deepcopy(value)
            return value

        def chain(cls):
            if cls in chains:
                return chains[cls]
            start = cls
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
            chains[start] = result
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
        # Streamed actors can have different immediate Levels while sharing the
        # reflected Actor.WorldInfo above. Freeze every actual Level identity;
        # a name or an arbitrary outer pointer is never sufficient evidence.
        levels = {}
        for label, obj in (('controlled', controlled), ('driver', driver),
                           ('controller', pc), ('camera', camera), ('world_info', world)):
            owner = identity(obj['outer'])
            derives(owner, 'Level')
            if owner['cls'] != level['cls']:
                raise AnchorRejected('Mounted Level class mismatch')
            levels[label] = owner
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
        def changed_timestamp(observed):
            if len(observed) != 4 or not math.isfinite(struct.unpack('<f', observed)[0]):
                raise AnchorRejected('Invalid mounted camera timestamp at bracket boundary')
            return observed != stamp

        timestamp_changed = False
        for (address, size), expected in reads.items():
            observed = r.read(address, size)
            if address in (r.objects_header, r.names_header) and size == 16:
                registry = 'objects' if address == r.objects_header else 'names'
                _check_registry_header(registry, observed, struct.unpack('<Qii', expected))
            elif (address, size) == (s.stamp, 4):
                timestamp_changed |= changed_timestamp(observed)
            elif observed != expected:
                raise AnchorRejected('Mounted memory changed at bracket boundary')
        # Close after all identity/layout/link/POV checks. A timestamp mismatch
        # is retryable only when the other boundaries, including both exact
        # registry headers, passed. No successful proof survives a retry.
        timestamp_changed |= changed_timestamp(r.read(s.stamp, 4))
        for header, registry in ((r.objects_header, 'objects'), (r.names_header, 'names')):
            _check_registry_header(registry, r.read(header, 16),
                                   struct.unpack('<Qii', reads[header, 16]))
        if timestamp_changed:
            raise MountedTimestampChanged('Mounted camera timestamp changed at bracket boundary')
        roles = dict(controller=pc, camera=camera, local_player=local,
                     controlled=controlled, driver=driver, inventory_pawn=driver,
                     inventory=inventory, camera_target=controlled, level=level, world_info=world)
        return dict(schema=1, kind='turret_driver', roles=roles, levels=levels, identities=identities,
                    supers=supers, properties=properties, links=links,
                    camera_timestamp_raw=stamp.hex(), camera_pov_raw=pov.hex(),
                    atomic_snapshot=False, runtime_verified=False)


MOUNTED_PROPERTIES = ('Pawn', 'Controller', 'Driver', 'DrivenVehicle', 'PlayerCamera',
                      'PCOwner', 'Player', 'WorldInfo', 'InvManager')
MOUNTED_OWNERS = ('Controller', 'Pawn', 'Vehicle', 'Pawn', 'PlayerController',
                  'Camera', 'PlayerController', 'Actor', 'Pawn')


def validate_mounted_config(config):
    """Check serialized semantics before native independently validates live bytes."""
    m=config['mounted'];ids=config['identities'];roles=config['roles']
    if set(m)!={'role_driver','role_inventory','role_world_info','property_offset_internal','properties'}:
        raise AnchorRejected('Incomplete mounted certificate')
    for key in ('role_driver','role_inventory','role_world_info'):
        if type(m[key]) is not int or not 0<=m[key]<len(ids):raise AnchorRejected('Invalid mounted role')
    if type(m['property_offset_internal']) is not int or not 0<=m['property_offset_internal']<=0xfffc:
        raise AnchorRejected('Invalid mounted property offset')
    if len(set([*roles.values(),m['role_driver'],m['role_inventory'],m['role_world_info']]))!=8:
        raise AnchorRejected('Aliased mounted roles')
    by_address={i['address']:i for i in ids}
    def ancestry(cls,name):
        seen=set()
        while cls:
            item=by_address.get(cls)
            if (cls in seen or len(seen)>=64 or not item or not item['check_super'] or item['number'] or
                    by_address.get(item['cls'],{}).get('name')!='Class'):
                raise AnchorRejected('Invalid mounted ancestry')
            if item['name']==name:return True
            seen.add(cls);cls=item['super']
        return False
    pawn=ids[roles['pawn']];driver=ids[m['role_driver']];inventory=ids[m['role_inventory']]
    world=ids[m['role_world_info']];level=ids[roles['level']]
    for item,name in ((pawn,'Turret'),(pawn,'Vehicle'),(driver,'WarPawn'),(inventory,'WarInventoryManager'),(world,'WorldInfo')):
        if item['name'].startswith('Default__') or not ancestry(item['cls'],name):raise AnchorRejected('Mounted role class mismatch')
    if pawn['outer']!=level['address'] or inventory['outer']!=driver['address']:
        raise AnchorRejected('Mounted role owner mismatch')
    for obj in (pawn,driver,world,ids[roles['controller']],ids[roles['camera']]):
        owner=by_address.get(obj['outer'])
        if (not owner or not owner['name'] or owner['name'].startswith('Default__') or
                owner['cls']!=level['cls'] or not ancestry(owner['cls'],'Level')):
            raise AnchorRejected('Missing or invalid mounted Level identity')
    if set(m['properties'])!=set(MOUNTED_PROPERTIES):raise AnchorRejected('Missing mounted property')
    for name,owner in zip(MOUNTED_PROPERTIES,MOUNTED_OWNERS):
        value=m['properties'][name]
        if set(value)!={'role','offset'} or type(value['role']) is not int or not 0<=value['role']<len(ids) or type(value['offset']) is not int or not 0<=value['offset']<=0xfff8:
            raise AnchorRejected('Invalid mounted property')
        field=ids[value['role']];kind=by_address.get(field['cls'],{});owner_id=by_address.get(field['outer'],{})
        if field['name']!=name or field['number'] or kind.get('name')!='ObjectProperty' or owner_id.get('name')!=owner or owner_id.get('number'):
            raise AnchorRejected('Mounted property identity mismatch')
    for field,name in (('controller_pawn','Pawn'),('pawn_controller','Controller'),('player_camera','PlayerCamera'),('pc_owner','PCOwner'),('player','Player')):
        if config['fields'][field]!=m['properties'][name]['offset']:raise AnchorRejected('Mounted field mismatch')
    return m



def mounted_crosshair_suspension(anchor,pid,base,sha256):
    validate_mounted_config(anchor)
    m=anchor['mounted'];ids=anchor['identities']
    return dict(schema=2,native_contract=2,pid=pid,base=base,sha256=sha256,
        controller=ids[anchor['roles']['controller']]['address'],
        controlled=ids[anchor['roles']['pawn']]['address'],driver=ids[m['role_driver']]['address'],
        camera=ids[anchor['roles']['camera']]['address'])


def mounted_crosshair_ini(proof):
    if set(proof)!={'schema','native_contract','pid','base','sha256','controller','controlled','driver','camera'} or proof['schema']!=2 or proof['native_contract']!=2:
        raise AnchorRejected('Invalid mounted crosshair suspension')
    for key,value in proof.items():
        if key=='sha256':
            if value!='bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4':raise AnchorRejected('Suspension build mismatch')
        elif type(value) is not int or not 0<value<1<<64:raise AnchorRejected('Invalid suspension integer')
    return '[CrosshairAssignment]\n'+'\n'.join(f'{key}={value}' for key,value in proof.items())+'\n'
