"""Private rigid magazine from the SAME original Lancer LOD0, not the PH prop.

Keep source UVs, colors, topology and tangent handedness. The cache is generated
from the owner's pinned local package and is never distributed with the mod.
"""
from pathlib import Path
import hashlib,json,math,os,stat,struct,tempfile
from weapon_part_cache import read_export,_unit,PACKAGE_SHA256
from lancer_handle_cache import EXPORT_OFFSET,EXPORT_BYTES,EXPORT_SHA256,VERTEX_OFFSET,INDEX_OFFSET

CACHE_VERSION='lancer-magazine-df356ff3-p1'
VERTEX_SHA256='c6b5b6728190e7486d845aff43664bde62f3982615254e34895d26ba6079d4e3'
INDEX_SHA256='eff38c46d35f7b646a8bd0e56808b3fa8ac6910f97e351ba1da34db0068b6eab'
ANCHOR=(0.00796632598,28.14602802187,4.39606437790)


def build_input(raw):
    if len(raw)!=EXPORT_BYTES or hashlib.sha256(raw).hexdigest()!=EXPORT_SHA256:
        raise ValueError('Unverified original Lancer export')
    # Audited first chunk: vertices 0..12546, BoneMap[0] == b_AR_Magazine.
    # The second chunk reuses local bone0 for the chainsaw; never include it.
    used=[]
    for i in range(12547):
        at=VERTEX_OFFSET+i*64
        weights=list(zip(raw[at+56:at+60],raw[at+60:at+64]))
        if sum(w for _,w in weights)!=255:raise ValueError('Invalid skin weights')
        total=sum(w for b,w in weights if b==0)
        if total not in (0,255):raise ValueError('Nonrigid magazine vertex')
        if total==255:used.append(i)
    if len(used)!=120:raise ValueError('Original magazine vertex set changed')
    remap={v:i for i,v in enumerate(used)};vertices=bytearray();triangles=[]
    def axes(v):return (v[1],v[0],-v[2])
    def pack(v):return bytes(max(0,min(255,round((a+1)*127.5))) for a in v)+b'\x80'
    for i in used:
        at=VERTEX_OFFSET+i*64;p=struct.unpack_from('<3f',raw,at)
        if not all(math.isfinite(a) and abs(a)<10000 for a in p):raise ValueError('Invalid position')
        x=_unit(tuple(b/127.5-1 for b in raw[at+12:at+15]))
        n=_unit(tuple(b/127.5-1 for b in raw[at+16:at+19]));sign=raw[at+19]
        if sign not in (0,255):raise ValueError('Invalid tangent handedness')
        y=_unit((n[1]*x[2]-n[2]*x[1],n[2]*x[0]-n[0]*x[2],n[0]*x[1]-n[1]*x[0]))
        if sign==0:y=tuple(-v for v in y)
        uv=raw[at+20:at+52]
        if not all(math.isfinite(a) and abs(a)<10000 for a in struct.unpack('<8f',uv)):
            raise ValueError('Invalid source UV')
        # Exact inverse of lancer_seated_magazine's fixed proper basis. Baking
        # this basis once preserves all existing hand/item/insertion transforms.
        vertices.extend(struct.pack('<3f',*(v/100 for v in axes(tuple(p[k]-ANCHOR[k] for k in range(3))))))
        vertices.extend(pack(axes(x))+pack(axes(y))+pack(axes(n))+uv+raw[at+52:at+56])
    original=struct.unpack_from('<39615H',raw,INDEX_OFFSET)
    for first in range(0,len(original),3):
        face=original[first:first+3]
        if any(v in remap for v in face):
            if not all(v in remap for v in face):raise ValueError('Magazine crosses source triangle')
            triangles.extend(remap[v] for v in face)
    if len(triangles)!=468:raise ValueError('Original magazine topology changed')
    result=(bytes(vertices),struct.pack('<468H',*triangles))
    for data,digest in zip(result,(VERTEX_SHA256,INDEX_SHA256)):
        if digest and hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Magazine codec changed')
    return result


def prepare(game_exe,cache_root):
    if not VERTEX_SHA256 or not INDEX_SHA256:raise ValueError('Magazine codec not frozen')
    game=Path(game_exe).resolve().parent.parent
    requested=Path(cache_root).absolute()/CACHE_VERSION
    def no_links(p):
        for item in (p,*p.parents):
            try:info=item.lstat()
            except FileNotFoundError:continue
            if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                raise ValueError('Reparse Lancer magazine cache')
    no_links(requested);cache=requested.resolve()
    if cache==game or cache.is_relative_to(game):raise ValueError('Cache must be outside game')
    raw=read_export(game/'WarGame/CookedPC/WarfareGame.u',export_offset=EXPORT_OFFSET,
        export_bytes=EXPORT_BYTES,export_sha256=EXPORT_SHA256)
    vertices,indices=build_input(raw);cache.mkdir(parents=True,exist_ok=True);hashes={}
    for name,data in (('lancer-magazine.vtx60',vertices),('lancer-magazine.idx16',indices)):
        path=cache/name;no_links(path);hashes[name]=hashlib.sha256(data).hexdigest()
        if path.exists():
            if not path.is_file() or path.read_bytes()!=data:raise ValueError('Different existing magazine cache')
            continue
        fd,tmp=tempfile.mkstemp(prefix=name+'.',suffix='.tmp',dir=cache)
        try:
            with os.fdopen(fd,'wb') as f:f.write(data)
            os.replace(tmp,path)
        finally:
            if os.path.exists(tmp):os.unlink(tmp)
    meta=dict(schema=1,private_game_derived=True,distribution_allowed=False,
        source_package_sha256=PACKAGE_SHA256,source_export_sha256=EXPORT_SHA256,
        vertices=120,triangles=156,source_selection='first-chunk rigid bone8',
        hashes=hashes,runtime_constructed=False,rendered=False)
    path=cache/'source.json';no_links(path);path.write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    return dict(directory=str(cache),source_sha256=EXPORT_SHA256,hashes=hashes)
