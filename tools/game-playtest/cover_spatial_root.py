"""Read-only world/hash/root metadata within an existing authenticated query.

This is not a scene traversal or a collision query. It never calls the engine,
reads component-array payloads, synchronizes the world or authorizes movement.
The caller supplies the existing current module certificate and WorldInfo role,
and must close the SAME CoverQuery. Not connected to an action/runtime driver.
"""
import hashlib
import math
from pathlib import Path
import struct

import cover_surface_query as surface
import cover_world_query_code as world_code
from cover_bone_query import _chain


GLOBAL_WORLD_RVA = 0x1c20890
WORLD_HASH_OFFSET = 0x178
VTABLE_RVA = 0x133d140
VTABLE_TARGET_RVAS = (0x1efa20, 0x3fb4e4, 0x136918, 0xcaec0,
                      0xcfef0, 0x18e6c0, 0x3b9b78, 0xcc018)
ROOT_CELL_RVA = 0x1afac20
HALF_RVA, MINIMUM_RVA = 0x17070e4, 0x1707150
NODE_BYTES, CHILDREN, COOKIE_BYTES = 24, 8, 8
# Observation bound, not a proved engine maximum. No array payload is read.
MAX_NODE_CAPACITY = 1 << 20
CHUNKS = (
    (0xcfef0, 1686, 'd066c0679f667c6ae7d7ca009282c551faacd7e6f5a85e988e7246eb6cf4b0e8'),
    (0x89b2d0, 97, 'd4e36fc72b8a25f883a5fd3f724f7c8581254c351c43dc070776b4eedc74ef51'),
    (0xd0650, 748, '7cb44908458ff9ca5c433987390c7d2d6a813df679dc3ac5b961f87b7b6b84a5'),
    (0xd093c, 116, '9060bb6a0fd0a1515a1629a28c3c8b126f2a63f63dfb41ef8f8d0763c61cfa33'),
    (0xd0b54, 282, 'ab4b69b2576e22b79cab0c63bf4c8231fb76cdab83d8ae504e5ffef2d933f8dc'),
    (0x18e6c0, 219, '5d87dff66a2a49e3f2bb84af1aabd1aea42cfa1c802b8ce4aaa19f92cf9893a3'),
    (0xd10f8, 654, '335d4912ca6ae7fb614bdccf389677dfd939c780deb04b14d447f96ff9fc80df'),
    (0x89b3a4, 41, '798d2b754d571baff4828dd3886ed3e7ab6ee700e6b7c434d5b9012ae38bf466'),
    (0x3d0e20, 17, '1018b23bfc6803d1e50586a1e9b0151b14d97a7ba9267a595518de56a7704eab'),
    (0x1efc5c, 105, '099f4732b22dddb32007b6bb8855c9d77db8991b5ea12dbd3e5697c1d83ddb4b'),
    # Contiguous original includes the seven-byte gap omitted by old metadata.
    (0x53eb20, 112, 'ee3416fba3b985c0ca1dd44c85b0630390eef7b3141fc15e7be2acf11e9f2dab'),
)


def _historical(path=surface.DEFAULT_BLOB):
    raw = Path(path).read_bytes()
    if len(raw) != surface.HISTORICAL_SIZE or hashlib.sha256(raw).hexdigest() != surface.HISTORICAL_SHA256:
        raise RuntimeError('Spatial root historical code source differs')
    result = []
    for rva, size, digest in CHUNKS:
        offset = rva - surface.HISTORICAL_RVA
        body = raw[offset:offset + size] if offset >= 0 else b''
        if len(body) != size or hashlib.sha256(body).hexdigest() != digest:
            raise RuntimeError('Spatial root original code range differs: ' + hex(rva))
        result.append((rva, body, digest))
    return result


def _pointer(address, size, *, alignment=8, label='native'):
    if (type(address) is not int or type(size) is not int or size <= 0 or
            not 0x10000 <= address < (1 << 47) - size or address % alignment):
        raise RuntimeError('Spatial root pointer/range rejected: ' + label)
    return address


def _data_range(module, rva, size):
    section = surface._range(module, rva, size)
    if section['characteristics'] & 0x20000000:
        raise RuntimeError('Spatial root data overlaps executable section')


def _instance(q, address, expected):
    _pointer(address, 1, label=expected)
    obj = q.obj(address)
    if obj['name'].startswith('Default__'):
        raise RuntimeError('Spatial root default object rejected: ' + expected)
    classes = list(_chain(q, obj['cls']))
    if not classes or expected not in [c['name'] for c in classes]:
        raise RuntimeError('Spatial root instance class rejected: ' + expected)
    for cls in classes:
        if cls['number'] or q.obj(cls['cls'])['name'] != 'Class':
            raise RuntimeError('Spatial root class identity rejected: ' + expected)
    return obj


def _node(raw, address):
    if len(raw) != NODE_BYTES:
        raise RuntimeError('Spatial root node header size rejected')
    data, count, capacity, children = struct.unpack('<QiiQ', raw)
    if not 0 <= count <= capacity <= MAX_NODE_CAPACITY:
        raise RuntimeError('Spatial root node count/capacity rejected')
    if capacity:
        _pointer(data, capacity * 8, label='component array')
    elif data:
        raise RuntimeError('Spatial root zero-capacity node has data')
    if children:
        _pointer(children, CHILDREN * NODE_BYTES, label='child records')
        _pointer(children - COOKIE_BYTES, COOKIE_BYTES, label='child cookie')
    return dict(address=address, data=data, count=count, capacity=capacity,
                children=children, header_bytes=raw.hex())


def observe(q, module, world_info):
    """Observe only root/first child headers; caller must finish its query.

    No new observer, thread, session, input or queue is created. Matching code
    and read-bracket closure do not establish a physics epoch or complete tree.
    """
    if (module['base'] != q.r.base or module.get('sha256') != surface.GAME_SHA256 or
            q.r.profile['sha256'] != surface.GAME_SHA256):
        raise RuntimeError('Spatial root module/reader identity differs')
    info = _instance(q, world_info['address'], 'WorldInfo')
    if info != world_info:
        raise RuntimeError('Spatial root WorldInfo role changed')
    level = _instance(q, info['outer'], 'Level')
    world = _instance(q, level['outer'], 'World')
    for rva, size in ((GLOBAL_WORLD_RVA, 8), (VTABLE_RVA, 64),
                      (ROOT_CELL_RVA, 16), (HALF_RVA, 4), (MINIMUM_RVA, 4)):
        _data_range(module, rva, size)
    global_world = struct.unpack('<Q', q.read(module['base'] + GLOBAL_WORLD_RVA, 8))[0]
    if global_world != world['address']:
        raise RuntimeError('Spatial root global World differs from WorldInfo outer World')

    route = world_code.observe(q, module)
    observed_code = []
    for rva, expected, digest in _historical():
        surface._range(module, rva, len(expected), True)
        actual = q.read(module['base'] + rva, len(expected))
        actual_digest = hashlib.sha256(actual).hexdigest()
        if actual != expected or actual_digest != digest:
            raise world_code.CodeRejected(dict(kind='spatial_root_code_mismatch', rva=hex(rva),
                size=len(expected), expected_sha256=digest, observed_sha256=actual_digest,
                observed_size=len(actual), closing_verified=False, activation_allowed=False))
        observed_code.append(dict(rva=hex(rva), size=len(expected), sha256=digest))

    hash_address = struct.unpack('<Q', q.read(world['address'] + WORLD_HASH_OFFSET, 8))[0]
    _pointer(hash_address, 16, label='hash receiver')
    vtable, root_address = struct.unpack('<QQ', q.read(hash_address, 16))
    if vtable != module['base'] + VTABLE_RVA:
        raise RuntimeError('Spatial root receiver vtable address rejected')
    targets = struct.unpack('<8Q', q.read(vtable, 64))
    if targets != tuple(module['base'] + rva for rva in VTABLE_TARGET_RVAS):
        raise RuntimeError('Spatial root receiver vtable targets rejected')
    for rva in VTABLE_TARGET_RVAS:
        surface._range(module, rva, 1, True)
    _pointer(root_address, NODE_BYTES, label='root node')
    root = _node(q.read(root_address, NODE_BYTES), root_address)
    children = []
    if root['children']:
        start = root['children']
        if start - COOKIE_BYTES < root_address + NODE_BYTES and root_address < start + CHILDREN * NODE_BYTES:
            raise RuntimeError('Spatial root child allocation overlaps root node')
        cookie = struct.unpack('<Q', q.read(start - COOKIE_BYTES, COOKIE_BYTES))[0]
        if cookie != CHILDREN:
            raise RuntimeError('Spatial root child allocation cookie rejected')
        raw = q.read(start, CHILDREN * NODE_BYTES)
        for index in range(CHILDREN):
            child = _node(raw[index * NODE_BYTES:(index + 1) * NODE_BYTES], start + index * NODE_BYTES)
            if child['children'] and (child['children'] == root_address or
                    start <= child['children'] < start + CHILDREN * NODE_BYTES):
                raise RuntimeError('Spatial root child header cycle rejected')
            child['index'] = index
            children.append(child)
    cube = struct.unpack('<4f', q.read(module['base'] + ROOT_CELL_RVA, 16))
    if not all(math.isfinite(v) for v in cube) or cube[3] <= 0:
        raise RuntimeError('Spatial root cell center/half rejected')
    factor = q.read(module['base'] + HALF_RVA, 4)
    minimum = q.read(module['base'] + MINIMUM_RVA, 4)
    if factor != struct.pack('<f', .5) or minimum != struct.pack('<f', 100.):
        raise RuntimeError('Spatial root current split constants differ')
    return dict(world_info=info, outer_level=level, global_world=world,
                hash_receiver=hash_address, vtable=vtable,
                vtable_targets=[hex(v) for v in targets], root=root, child_headers=children,
                root_cell=dict(center=list(cube[:3]), half=cube[3]),
                code=dict(wrapper_route=route, spatial_chunks=observed_code),
                read_only=True, root_links_observed=True, closing_required=True,
                first_child_headers_only=True, component_payload_observed=False,
                complete_tree_observed=False, complete_scene_verified=False,
                bsp_or_special_coverage_verified=False, engine_calls=False, virtual_calls=False,
                atomic_snapshot_verified=False, world_synchronization_verified=False,
                path_or_occupancy_verified=False, activation_allowed=False)
