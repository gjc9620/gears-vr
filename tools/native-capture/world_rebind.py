"""Owner-side read-only certificate preparation. No injection or live publication.

Existing startup builders still require their original code proofs. Installed
native patches can make those proofs fail; this module never bypasses them.
"""
from dataclasses import dataclass
from collections.abc import Mapping
import copy
import hashlib
import os
from pathlib import Path
import tempfile
from types import MappingProxyType

from start_capture import acquire_observer_preparation, resolve_camera_once
from anchor_config import anchor_ini
from ui_observer_config import ui_observer_ini
from crosshair_assignment_config import validate_crosshair_assignment_config, crosshair_assignment_ini


def process_identity(pid):
    from run_stereo_continuous import WindowsControl
    return WindowsControl().identity(pid)


def uint(value, label, bits=64, positive=False):
    if isinstance(value, str):
        try:value = int(value, 0)
        except ValueError:raise ValueError('Invalid '+label) from None
    if type(value) is not int or not int(positive) <= value < 1 << bits:
        raise ValueError('Invalid '+label)
    return value


def identity(value):
    if not isinstance(value, Mapping):raise ValueError('Missing full local identity')
    result = {key:uint(value[key], 'identity '+key, 32 if key in ('number','internal_index') else 64,
                       key in ('address','cls')) for key in ('address','cls','outer','number','internal_index')}
    name = value['name']
    if not isinstance(name,str) or not name or any(c in name for c in '\r\n\0'):
        raise ValueError('Invalid identity name')
    result['name'] = name
    return result


def process(value):
    if not isinstance(value,Mapping):raise ValueError('Missing process identity')
    result = dict(pid=uint(value['pid'],'process PID',32,True),
                  creation_time=uint(value['creation_time'],'process creation',positive=True))
    if not isinstance(value.get('image'),str) or not value['image']:
        raise ValueError('Missing process image')
    result['image'] = value['image'].casefold()
    return result


def freeze(value):
    if isinstance(value,Mapping):return MappingProxyType({key:freeze(item) for key,item in value.items()})
    if isinstance(value,(list,tuple)):return tuple(freeze(item) for item in value)
    if value is None or type(value) in (str,int,float,bool,bytes):return value
    raise ValueError('Unsupported candidate data')


def thaw(value):
    if isinstance(value,Mapping):return {key:thaw(item) for key,item in value.items()}
    if isinstance(value,tuple):return [thaw(item) for item in value]
    return value


@dataclass(frozen=True)
class Candidate:
    schema: int
    game_identity: object
    owner_identity: object
    previous_generation: int
    generation: int
    current_camera: object
    local_identity: object
    module_base: int
    disk_sha256: str
    anchor_metadata: object
    ui_metadata: object
    anchor_ini: str
    ui_ini: str


def role(config, name):
    index = uint(config['roles'][name], 'role '+name, 32)
    ids = config['identities']
    if index >= len(ids):raise ValueError('Role outside identities: '+name)
    return identity(ids[index])


def ui_bundle_ini(ui):
    section = ui_observer_ini(ui['config'])
    if 'crosshair_assignment_suspended' in ui:
        from mounted_context import mounted_crosshair_ini
        section+=mounted_crosshair_ini(ui['crosshair_assignment_suspended'])
    if 'crosshair_assignment' in ui:
        section += crosshair_assignment_ini(ui['crosshair_assignment'])
    return section


def validate_sources(camera, anchor, ui, local, pid, digest, *, crosshair_assignment_observer=False):
    if uint(camera['pid'],'camera PID',32,True) != pid or camera['sha256'] != digest:
        raise ValueError('Camera build/PID mismatch')
    base = uint(camera['module_base'],'module base',positive=True)
    ac,uc = anchor['config'],ui['config']
    if (anchor.get('read_only') is not True or anchor.get('pid') != pid or ac['schema'] not in (2,3) or
            uint(ac['cinematic']['module_base'],'cinematic module',positive=True) != base):
        raise ValueError('Anchor certificate mismatch')
    if (ui.get('accepted') is not True or ui.get('read_only') is not True or ui.get('pid') != pid or
            ui.get('sha256') != digest or uc['pid'] != pid or uc['sha256'] != digest or uc['base'] != base):
        raise ValueError('UI certificate mismatch')
    if role(ac,'local_player') != local or identity(anchor['certificate_final_guard']['local_player']) != local:
        raise ValueError('Stable local identity changed')
    resolved = anchor['sampler']['resolver']
    for key in ('camera_address','controller_address','local_player_address','timestamp_address','pov_address','module_base'):
        if uint(resolved[key],key) != uint(camera[key],key):raise ValueError('Anchor camera source mismatch: '+key)
    if resolved['pid'] != pid or resolved['sha256'] != digest:raise ValueError('Anchor resolver build/PID mismatch')
    for name,key in (('local_player','local_player_address'),('controller','controller_address'),('camera','camera_address')):
        item = role(ac,name)
        if item['address'] != uint(camera[key],key,positive=True):raise ValueError('Camera role mismatch: '+name)
        if identity(anchor['certificate_final_guard'][name]) != item:raise ValueError('Final anchor role mismatch: '+name)
        if name != 'local_player' and role(uc,name) != item:raise ValueError('UI camera source mismatch: '+name)
    mounted=ac['schema']==3
    inventory_pawn=identity(ac['identities'][ac['mounted']['role_driver']]) if mounted else role(ac,'pawn')
    if inventory_pawn != role(uc,'pawn'):raise ValueError('UI Pawn source mismatch')
    if mounted and (uc['schema']!=2 or role(ac,'pawn')!=role(uc,'controlled')):raise ValueError('Mounted UI source mismatch')
    required_crosshair = ui.get('crosshair_assignment_observer_enabled',False)
    if type(required_crosshair) is not bool:raise ValueError('Invalid crosshair observer requirement')
    if (crosshair_assignment_observer or required_crosshair) and 'crosshair_assignment' not in ui and 'crosshair_assignment_suspended' not in ui:
        raise ValueError('Missing requested crosshair assignment certificate')
    if 'crosshair_assignment_suspended' in ui:
        from mounted_context import mounted_crosshair_suspension,mounted_crosshair_ini
        suspended=ui['crosshair_assignment_suspended']
        if not mounted or 'crosshair_assignment' in ui or suspended!=mounted_crosshair_suspension(ac,pid,base,digest):
            raise ValueError('Mounted crosshair suspension mismatch')
        mounted_crosshair_ini(suspended)
    if 'crosshair_assignment' in ui:
        if mounted:raise ValueError('Mounted context cannot use human finite aim')
        cc = validate_crosshair_assignment_config(ui['crosshair_assignment'])
        if cc['pid'] != pid or cc['base'] != base or cc['sha256'] != digest:
            raise ValueError('Crosshair certificate build/PID/base mismatch')
        if len(cc['tick_roots']) != 2 or not cc.get('writer_view',{}).get('enabled'):
            raise ValueError('Crosshair world recovery requires tick roots and writer view')
        def full_role(config, name):
            item = config['identities'][uint(config['roles'][name],'role '+name,32)]
            result = identity(item)
            if 'name_index' not in item:raise ValueError('Missing full role name index: '+name)
            result['name_index'] = uint(item['name_index'],'role name index',32)
            return result
        for name in ('controller','pawn'):
            crosshair_role = full_role(cc,name)
            if crosshair_role != full_role(ac,name) or crosshair_role != full_role(uc,name):
                raise ValueError('Crosshair full role mismatch: '+name)
    cam = uint(camera['camera_address'],'camera',positive=True)
    for field,key in (('camera_timestamp','timestamp_address'),('camera_pov','pov_address')):
        if cam+uint(ac['fields'][field],field,32) != uint(camera[key],key,positive=True):
            raise ValueError('Camera field mismatch: '+field)
    yaw = uint(camera['pov_address'],'POV',positive=True)+uint(camera['offsets']['Rotation'],'rotation',32)+4
    uint(yaw,'camera yaw',positive=True)
    return base,yaw


def build_candidate(pid,profile,previous_local_identity,expected_generation,owner_identity,dll,*,signatures,check_live=None,attestation=None,
                    crosshair_assignment_observer=False,camera_discovery=None):
    if camera_discovery is not None and (attestation is None or not callable(camera_discovery)):
        raise ValueError('Camera discovery hint requires live entry attestation')
    pid = uint(pid,'game PID',32,True)
    previous = uint(expected_generation,'previous generation',positive=True)
    if previous == (1 << 64)-1:raise ValueError('Generation exhausted')
    owner = process(owner_identity)
    if owner['pid'] == pid:raise ValueError('Owner is game process')
    local = identity(previous_local_identity)
    profile = copy.deepcopy(profile)
    digest = profile['sha256']
    if not isinstance(digest,str) or len(digest)!=64 or any(c not in '0123456789abcdef' for c in digest):
        raise ValueError('Invalid disk hash')
    if not isinstance(signatures,dict) or not signatures:raise ValueError('Caller signatures required')
    signatures = dict(signatures)
    for rva,raw in signatures.items():
        if type(rva) is not int:raise ValueError('Invalid signature RVA')
        uint(rva,'signature RVA',positive=True)
        if not isinstance(raw,bytes) or not raw:raise ValueError('Invalid signature bytes')
    game = process(process_identity(pid))
    if game['pid'] != pid:raise RuntimeError('Game process identity mismatch')
    def live():
        if check_live:check_live()
        if process(process_identity(pid)) != game:raise RuntimeError('Game process identity changed')
        if process(process_identity(owner['pid'])) != owner:raise RuntimeError('Owner process identity changed')
    live()
    if attestation is not None:
        if (attestation.pid,attestation.game_creation,attestation.owner,attestation.dll)!=(
                pid,game['creation_time'],owner,Path(dll).resolve()):
            raise ValueError('Attestation process/owner/DLL mismatch')
        attestation.reverify()
        # Acquisition always resolves a fresh camera on every attempt. Only its
        # two comparison inputs are needed here: the retained local identity and
        # the module base authenticated by the bound, live attestation.
        camera = dict(local_player_address=hex(local['address']),
                      module_base=hex(uint(attestation.module_base,'attested module base',positive=True)))
    else:
        camera = resolve_camera_once(pid,profile)
        if uint(camera['local_player_address'],'local address',positive=True) != local['address']:
            raise ValueError('Stable local address changed')
    live()
    camera,anchor,anchor_section,ui,ui_section = acquire_observer_preparation(
        pid,profile,camera,signatures,Path(dll),anchor_observer=True,ui_observer=True,cinematic_camera=True,
        record_anchor_failure=False,check_live=live,
        **({'camera_discovery':camera_discovery} if camera_discovery is not None else {}),
        **({'crosshair_assignment_observer':True} if crosshair_assignment_observer else {}),
        **({'entry_reader':attestation.read_entry} if attestation is not None else {}))
    live()
    if crosshair_assignment_observer:
        # Preserve the request in the existing immutable metadata so subsequent
        # serialization cannot silently turn a required section into absence.
        ui = dict(ui,crosshair_assignment_observer_enabled=True)
    base,_ = validate_sources(camera,anchor,ui,local,pid,digest,crosshair_assignment_observer=crosshair_assignment_observer)
    if attestation is not None and attestation.module_base != base:
        raise ValueError('Attestation camera module mismatch')
    if str(Path(camera['executable']).resolve()).casefold() != game['image']:
        raise ValueError('Camera executable identity mismatch')
    if anchor_ini(anchor['config']) != anchor_section or ui_bundle_ini(ui) != ui_section:
        raise ValueError('Certificate INI mismatch')
    live()
    if attestation is not None:
        attestation.reverify()
        live()
    return Candidate(1,freeze(game),freeze(owner),previous,previous+1,freeze(camera),freeze(local),base,digest,
                     freeze(anchor),freeze(ui),anchor_section,ui_section)


def serialize_candidate(candidate):
    if not isinstance(candidate,Candidate) or type(candidate.schema) is not int or candidate.schema != 1:
        raise ValueError('Invalid candidate schema')
    game,owner = process(thaw(candidate.game_identity)),process(thaw(candidate.owner_identity))
    previous = uint(candidate.previous_generation,'previous generation',positive=True)
    if previous == (1 << 64)-1 or uint(candidate.generation,'generation',positive=True) != previous+1:
        raise ValueError('Invalid candidate generation')
    camera,anchor,ui = (thaw(value) for value in (candidate.current_camera,candidate.anchor_metadata,candidate.ui_metadata))
    base,yaw = validate_sources(camera,anchor,ui,identity(thaw(candidate.local_identity)),game['pid'],candidate.disk_sha256)
    if base != candidate.module_base or owner['pid']==game['pid']:raise ValueError('Candidate identity mismatch')
    if anchor_ini(anchor['config']) != candidate.anchor_ini or ui_bundle_ini(ui) != candidate.ui_ini:
        raise ValueError('Candidate section mismatch')
    rows = ['[WorldRebind]','schema=1',f"game_pid={game['pid']}",f"game_creation={game['creation_time']}",
            f"owner_pid={owner['pid']}",f"owner_creation={owner['creation_time']}",
            f'previous_generation={previous}',f'generation={candidate.generation}',
            f'module_base={base:#x}',f'game_sha256={candidate.disk_sha256}',
            '[Capture]',f"pid={game['pid']}",f"local_player={uint(camera['local_player_address'],'local'):#x}",
            f"controller={uint(camera['controller_address'],'controller'):#x}",
            f"camera={uint(camera['camera_address'],'camera'):#x}",
            f"timestamp={uint(camera['timestamp_address'],'timestamp'):#x}",f'camera_yaw={yaw:#x}']
    return ('\n'.join(rows)+'\n'+candidate.anchor_ini+candidate.ui_ini).encode('utf-16')


def publish_candidate(candidate,path):
    """Publish a new explicit path only. File existence is never a request.

    The owner must signal native only after this returns. A short-lived empty
    exclusive reservation prevents another cooperative publisher overwriting it.
    """
    data = serialize_candidate(candidate)
    path = Path(path)
    temporary = None
    reservation = None
    try:
        fd,name = tempfile.mkstemp(prefix=path.name+'.',suffix='.tmp',dir=path.parent)
        temporary = Path(name)
        with os.fdopen(fd,'wb') as output:
            output.write(data);output.flush();os.fsync(output.fileno())
        with path.open('xb') as output:reservation = os.fstat(output.fileno())
        os.replace(temporary,path)
        temporary = None
        reservation = None
        return hashlib.sha256(data).hexdigest()
    finally:
        try:
            if temporary is not None:temporary.unlink(missing_ok=True)
        finally:
            if reservation is not None:
                try:current = path.stat()
                except FileNotFoundError:current = None
                if current is not None and (current.st_dev,current.st_ino)==(reservation.st_dev,reservation.st_ino):
                    path.unlink()
