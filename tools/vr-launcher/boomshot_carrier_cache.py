"""Pinned original MagazineMesh0 LOD0 conversion; no fabricated ammo geometry.

Consumes only a verified private export. Runtime package extraction and asset
ownership are separate. No game launch, native call or public asset payload.
"""
import hashlib
import math
import struct
import os
import stat
import tempfile
from pathlib import Path
from weapon_part_cache import read_export

EXPORT_BYTES = 8276
EXPORT_SHA256 = '3f0f49cba8a4522df276112b85842cc039055a31ae88a4ad32602fcb44170480'
SOURCE_PATH = 'Locust_Boomshot.Mesh.Locust_Boomshot_Magazine'
VERTICES = 108
INDICES = 372
POSITION_DATA = 1376
TANGENT_UV_DATA = 3992
INDEX_DATA = 7468
VERTEX_SHA256 = '5b6bded6b860a21ca28156775839e0dcf3404f64db2c5628e65eb8f69e6cc88c'
INDEX_SHA256 = 'eed7c47ea467c3eb3e3b689e437480038decc17b38b4e9ddf6b2f2efcf850e89'
PACKAGE_BYTES = 121876625
PACKAGE_SHA256 = '43502a95a151a7412e52bacd1db628fd2e8d726d98f6785d4ebd0fa52182dc9c'
EXPORT_OFFSET = 299205280
CACHE_VERSION = 'boomshot-carrier-3f0f49cb-p1'


def _unit(values):
    size = math.sqrt(sum(v*v for v in values))
    if not math.isfinite(size) or not .75 <= size <= 1.25:
        raise ValueError('Invalid original Boomshot tangent basis')
    return tuple(v/size for v in values)


def _packed(values):
    return bytes(max(0, min(255, round((v+1)*127.5))) for v in values) + b'\x80'


def build_input(raw):
    if len(raw) != EXPORT_BYTES or hashlib.sha256(raw).hexdigest() != EXPORT_SHA256:
        raise ValueError('Unverified original Boomshot MagazineMesh0 export')
    if struct.unpack_from('<2I', raw, POSITION_DATA-8) != (12, 216) or \
       struct.unpack_from('<2I', raw, TANGENT_UV_DATA-8) != (16, 216) or \
       struct.unpack_from('<2I', raw, INDEX_DATA-8) != (2, INDICES):
        raise ValueError('Original Boomshot bulk layout changed')
    positions = raw[POSITION_DATA:POSITION_DATA+216*12]
    if positions[:VERTICES*12] != positions[VERTICES*12:]:
        raise ValueError('Original doubled position storage changed')
    indices = struct.unpack_from('<372H', raw, INDEX_DATA)
    if set(indices) != set(range(VERTICES)):
        raise ValueError('Original referenced vertex set changed')
    vertices = bytearray()
    for i in range(VERTICES):
        position = struct.unpack_from('<3f', positions, i*12)
        if not all(math.isfinite(v) and abs(v) < 100 for v in position):
            raise ValueError('Invalid original Boomshot position')
        at = TANGENT_UV_DATA+i*16
        tangent = _unit(tuple(v/127.5-1 for v in raw[at:at+3]))
        normal = _unit(tuple(v/127.5-1 for v in raw[at+4:at+7]))
        sign = 1 if raw[at+7] >= 128 else -1
        bitangent = _unit((normal[1]*tangent[2]-normal[2]*tangent[1],
                           normal[2]*tangent[0]-normal[0]*tangent[2],
                           normal[0]*tangent[1]-normal[1]*tangent[0]))
        uv = struct.unpack_from('<2e', raw, at+12)
        if not all(math.isfinite(v) and abs(v) < 16 for v in uv):
            raise ValueError('Invalid original Boomshot UV')
        vertices.extend(struct.pack('<3f', *(v/100 for v in position)))
        vertices.extend(_packed(tangent))
        vertices.extend(_packed(tuple(sign*v for v in bitangent)))
        vertices.extend(_packed(normal))
        vertices.extend(struct.pack('<8f', *(uv*4)))
        vertices.extend(raw[at+8:at+12])
    if len(vertices) != VERTICES*60:
        raise ValueError('Boomshot input stride changed')
    result = bytes(vertices), struct.pack('<372H', *indices)
    if any(hashlib.sha256(data).hexdigest() != wanted
           for data, wanted in zip(result, (VERTEX_SHA256, INDEX_SHA256))):
        raise ValueError('Boomshot carrier codec fingerprint changed')
    return result


def prepare(game_exe, cache_root):
    game = Path(game_exe).resolve().parent.parent
    requested = Path(cache_root).absolute()/CACHE_VERSION

    def no_links(path):
        for item in (path, *path.parents):
            try:
                info = item.lstat()
            except FileNotFoundError:
                continue
            if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
                raise ValueError('Reparse Boomshot carrier cache')

    no_links(requested)
    cache = requested.resolve()
    if cache == game or cache.is_relative_to(game):
        raise ValueError('Cache must be outside game')
    package = game/'WarGame/CookedPC/Maps/MP_Maps/MP_WarMachine.war'
    raw = read_export(package, export_offset=EXPORT_OFFSET, export_bytes=EXPORT_BYTES,
                      export_sha256=EXPORT_SHA256, package_bytes=PACKAGE_BYTES,
                      package_sha256=PACKAGE_SHA256)
    vertices, indices = build_input(raw)
    cache.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for name, data in (('boomshot-carrier.vtx60', vertices), ('boomshot-carrier.idx16', indices)):
        path = cache/name;no_links(path)
        hashes[name] = hashlib.sha256(data).hexdigest()
        if path.exists():
            if not path.is_file() or path.read_bytes() != data:
                raise ValueError('Different existing Boomshot carrier cache')
            continue
        fd, temporary = tempfile.mkstemp(prefix=name+'.', suffix='.tmp', dir=cache)
        try:
            with os.fdopen(fd, 'wb') as stream:
                stream.write(data)
            no_links(path)
            os.replace(temporary, path)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)
    metadata = dict(schema=1, private_game_derived=True, distribution_allowed=False,
                    source_path=SOURCE_PATH, source_package_sha256=PACKAGE_SHA256,
                    source_export_sha256=EXPORT_SHA256, vertices=VERTICES,
                    triangles=INDICES//3, hashes=hashes, runtime_constructed=False, rendered=False)
    path = cache/'source.json';no_links(path)
    import json
    path.write_text(json.dumps(metadata, indent=2)+'\n', encoding='utf-8')
    return dict(directory=str(cache), source_sha256=EXPORT_SHA256, hashes=hashes)
