"""Two private owner-marked FRONT-handle morph inputs, derived from the purchased package.

No game execution, mesh replacement, shared-asset write or distributed model.
The exact ROOT-weighted vertices retain the original material and topology.
"""
from pathlib import Path
import hashlib,json,math,os,stat,struct,tempfile
from weapon_part_cache import read_export,PACKAGE_SHA256

EXPORT_OFFSET=37766371
EXPORT_BYTES=1646457
EXPORT_SHA256='df356ff3dda45e060f87b5ed94c806d96bbfe36b74b21a0f1d2032596f9dcf4d'
CACHE_VERSION='lancer-front-handle-df356ff3-p3'
VERTEX_COUNT=12721
FIRST_VERTEX=4133
COUNT=64
VERTEX_OFFSET=832177
INDEX_OFFSET=831
INDEX_COUNT=40191
# Authored VR travel matched to the original longitudinal slot. This is not
# a measurement of stock-animation movement. Keep older p2 caches intact.
SLOT_REAR_VERTEX=10401
SLOT_REAR=(2.7596969604492188,34.79851150512695,12.67965316772461)
TRAVEL_RAW=23.041412353515625
DELTA_SHA256='24062bd3de919aa61f2f7110d628d733d5b08db16eaa283b0e23f44ad4c5f6d7'


def build_input(raw):
    if len(raw)!=EXPORT_BYTES or hashlib.sha256(raw).hexdigest()!=EXPORT_SHA256:
        raise ValueError('Unsupported original Lancer mesh')
    indices=struct.unpack_from('<'+str(INDEX_COUNT)+'H',raw,INDEX_OFFSET)
    selected=set(range(FIRST_VERTEX,FIRST_VERTEX+COUNT));faces=[]
    for i in range(0,len(indices),3):
        face=indices[i:i+3]
        if not any(v in selected for v in face):continue
        if not all(v in selected for v in face):raise ValueError('Front handle crosses original topology')
        faces.append(i//3)
    if faces!=list(range(7758,7793))+list(range(7857,7876)):
        raise ValueError('Original front handle triangle set changed')
    positions=[];data=bytearray()
    for i in range(FIRST_VERTEX,FIRST_VERTEX+COUNT):
        at=VERTEX_OFFSET+i*64;p=struct.unpack_from('<3f',raw,at)
        if not all(math.isfinite(a) for a in p):raise ValueError('Nonfinite original front handle')
        # Exact hash plus audited original first-chunk BoneMap[1] == ROOT.
        if [(b,w) for b,w in zip(raw[at+56:at+60],raw[at+60:at+64]) if w]!=[(1,255)]:
            raise ValueError('Front handle ROOT binding changed')
        positions.append(p)
        data.extend(struct.pack('<3fIHH',0.,-TRAVEL_RAW,0.,0x80808080,i,0))
    low=[min(p[k] for p in positions) for k in range(3)];high=[max(p[k] for p in positions) for k in range(3)]
    for actual,wanted in ((low,(2.1331593990,57.8399238586,12.7702350616)),(high,(4.3696632385,58.9116554260,13.7571115494))):
        if any(abs(a-b)>.0001 for a,b in zip(actual,wanted)):raise ValueError('Front bounds changed')
    rear=struct.unpack_from('<3f',raw,VERTEX_OFFSET+SLOT_REAR_VERTEX*64)
    if any(abs(a-b)>.0001 for a,b in zip(rear,SLOT_REAR)) or abs((low[1]-rear[1])-TRAVEL_RAW)>1e-8:
        raise ValueError('Original guide-slot travel changed')
    result=bytes(data)
    if hashlib.sha256(result).hexdigest()!=DELTA_SHA256:
        raise ValueError('Front morph hash mismatch')
    return result,result


def prepare(game_exe,cache_root):
    game=Path(game_exe).resolve().parent.parent
    requested=Path(cache_root).absolute()/CACHE_VERSION
    def no_links(p):
        for item in (p,*p.parents):
            try:info=item.lstat()
            except FileNotFoundError:continue
            if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                raise ValueError('Reparse original-front-handle cache')
    no_links(requested);cache=requested.resolve()
    if cache==game or cache.is_relative_to(game):raise ValueError('Cache must be outside game')
    raw=read_export(game/'WarGame/CookedPC/WarfareGame.u',export_offset=EXPORT_OFFSET,
        export_bytes=EXPORT_BYTES,export_sha256=EXPORT_SHA256)
    pull_a,pull_b=build_input(raw);cache.mkdir(parents=True,exist_ok=True)
    hashes={}
    for name,data in (('pullA.delta20',pull_a),('pullB.delta20',pull_b)):
        p=cache/name;no_links(p);digest=hashlib.sha256(data).hexdigest();hashes[name]=digest
        if p.exists():
            if not p.is_file() or p.read_bytes()!=data:raise ValueError('Different existing original-front-handle cache')
            continue
        fd,temp=tempfile.mkstemp(prefix=name+'.',suffix='.tmp',dir=cache)
        try:
            with os.fdopen(fd,'wb') as f:f.write(data)
            os.replace(temp,p)
        finally:
            if os.path.exists(temp):os.unlink(temp)
    meta=dict(schema=1,private_game_derived=True,distribution_allowed=False,
        source_package_sha256=PACKAGE_SHA256,source_export_sha256=EXPORT_SHA256,
        source_indices=list(range(FIRST_VERTEX,FIRST_VERTEX+COUNT)),original_base_vertices=VERTEX_COUNT,
        delta_stride=20,hashes=hashes,axis=[0,-1,0],travel_raw=TRAVEL_RAW,guide_slot_rear_vertex=SLOT_REAR_VERTEX,
        guide_slot_rear=SLOT_REAR,vr_authored_stroke_not_stock_animation=True,
        runtime_constructed=False,rendered=False)
    p=cache/'source.json';no_links(p)
    p.write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    return dict(directory=str(cache),source_sha256=EXPORT_SHA256,hashes=hashes)
