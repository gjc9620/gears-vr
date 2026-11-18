"""Private original-part inputs derived locally from the owner's retail game.

Only a pinned export is decoded. Never modify game files or include generated
vertices in a portable package. No game launch, memory write or native call.
"""
from pathlib import Path
import hashlib
import json
import math
import os
import struct
import stat
import tempfile

PACKAGE_BYTES=209914317
PACKAGE_SHA256='cd6d9a98c809d5520783a732f955916111e20bb6bf3155fb8b15dea86d7d0538'
EXPORT_OFFSET=54529066
EXPORT_BYTES=373458
EXPORT_SHA256='1f1c5a1405d354f89d352c26547a22a1c4cd791ba769755b6e9317e93ceaecb2'
CACHE_VERSION='snub-1f1c5a14-p1'
VERTEX_SHA256='87f757bb536129247b5622c27108898f800bc36db79d986d9a51f56c88b41975'
INDEX_SHA256='dd54ab478f2c64798aa200d36373b3d28283420d4fb6804b879960c16a1f835a'


def lz4_block(packed,expected):
    """Bounded raw LZ4 block, with overlapping matches and no third-party DLL."""
    if not 0<expected<=8*1024*1024 or not 0<len(packed)<=16*1024*1024:
        raise ValueError('Invalid compressed block size')
    cursor=0;out=bytearray()
    def extended(base):
        nonlocal cursor
        length=base
        if base==15:
            while True:
                if cursor>=len(packed):raise ValueError('Truncated LZ4 length')
                part=packed[cursor];cursor+=1;length+=part
                if length>expected:raise ValueError('LZ4 length exceeds output bound')
                if part!=255:break
        return length
    while cursor<len(packed):
        token=packed[cursor];cursor+=1
        literal=extended(token>>4)
        if literal>len(packed)-cursor or literal>expected-len(out):
            raise ValueError('Invalid LZ4 literal')
        out.extend(packed[cursor:cursor+literal]);cursor+=literal
        if cursor==len(packed):break
        if cursor+2>len(packed):raise ValueError('Truncated LZ4 match')
        offset=struct.unpack_from('<H',packed,cursor)[0];cursor+=2
        if not offset or offset>len(out):raise ValueError('Invalid LZ4 back reference')
        count=extended(token&15)+4
        if count>expected-len(out):raise ValueError('LZ4 match exceeds output bound')
        while count:
            length=min(offset,count);start=len(out)-offset
            out.extend(out[start:start+length]);count-=length
    if len(out)!=expected:raise ValueError('Incomplete LZ4 output')
    return bytes(out)


def read_export(package,*,export_offset=EXPORT_OFFSET,export_bytes=EXPORT_BYTES,export_sha256=EXPORT_SHA256,
                package_bytes=PACKAGE_BYTES,package_sha256=PACKAGE_SHA256):
    if type(package_bytes) is not int or not 101<package_bytes<=2*1024*1024*1024 or \
       not isinstance(package_sha256,str) or len(package_sha256)!=64 or \
       any(c not in '0123456789abcdef' for c in package_sha256):
        raise ValueError('Invalid pinned source package identity')
    if not isinstance(export_offset,int) or not isinstance(export_bytes,int) or \
       export_offset<0 or not 0<export_bytes<=8*1024*1024 or \
       not isinstance(export_sha256,str) or len(export_sha256)!=64:
        raise ValueError('Invalid pinned export request')
    package=Path(package)
    with package.open('rb') as stream:
        size=os.fstat(stream.fileno()).st_size
        if size!=package_bytes or hashlib.file_digest(stream,'sha256').hexdigest()!=package_sha256:
            raise ValueError('Unsupported purchased weapon package')
        stream.seek(0);header=stream.read(101)
        if len(header)!=101 or struct.unpack_from('<IHH',header)!=(0x9E2A83C1,835,76):
            raise ValueError('Unexpected weapon package header')
        flags,count=struct.unpack_from('<II',header,93)
        if flags!=32 or not 0<count<10000:raise ValueError('Unsupported package compression')
        table=stream.read(count*16)
        if len(table)!=count*16:raise ValueError('Truncated package chunk table')
        chunks=list(struct.iter_unpack('<IIII',table));previous=0
        end=export_offset+export_bytes;out=bytearray(export_bytes);covered=0
        for uncompressed,usize,compressed,csize in chunks:
            if not usize or usize>64*1024*1024 or uncompressed<previous or uncompressed+usize>1024*1024*1024 or \
               compressed<101+len(table) or not csize or compressed+csize>size:
                raise ValueError('Invalid package chunk bounds')
            previous=uncompressed+usize
            if uncompressed>=end or uncompressed+usize<=export_offset:continue
            stream.seek(compressed);chunk=stream.read(csize)
            if len(chunk)!=csize or len(chunk)<16:raise ValueError('Truncated package chunk')
            tag,block_size,total_packed,total_raw=struct.unpack_from('<IIII',chunk)
            if tag!=0x9E2A83C1 or total_raw!=usize or not 0<block_size<=8*1024*1024:
                raise ValueError('Invalid package block header')
            blocks=(usize+block_size-1)//block_size
            cursor=16+blocks*8
            if cursor>len(chunk):raise ValueError('Truncated block table')
            rows=list(struct.iter_unpack('<II',chunk[16:cursor]));raw_at=uncompressed;packed_sum=0
            for packed_bytes,raw_bytes in rows:
                if not packed_bytes or not 0<raw_bytes<=block_size or packed_bytes>len(chunk)-cursor:
                    raise ValueError('Invalid package block bounds')
                if raw_at+raw_bytes>export_offset and raw_at<end:
                    decoded=lz4_block(chunk[cursor:cursor+packed_bytes],raw_bytes)
                    left=max(raw_at,export_offset);right=min(raw_at+raw_bytes,end)
                    if left!=export_offset+covered:raise ValueError('Noncontiguous selected export')
                    out[left-export_offset:right-export_offset]=decoded[left-raw_at:right-raw_at]
                    covered+=right-left
                cursor+=packed_bytes;raw_at+=raw_bytes;packed_sum+=packed_bytes
            if cursor!=len(chunk) or raw_at!=uncompressed+usize or packed_sum!=total_packed:
                raise ValueError('Package chunk sizes do not reconcile')
        if covered!=export_bytes or hashlib.sha256(out).hexdigest()!=export_sha256:
            raise ValueError('Pinned original export identity mismatch')
        return bytes(out)


def _unit(value):
    length=math.sqrt(sum(v*v for v in value))
    if not math.isfinite(length) or not .75<=length<=1.25:raise ValueError('Invalid original tangent basis')
    return tuple(v/length for v in value)


def build_input(raw):
    if len(raw)!=EXPORT_BYTES or hashlib.sha256(raw).hexdigest()!=EXPORT_SHA256:
        raise ValueError('Unverified original Snub export')
    start=188986;count=2881;target=[]
    # The pinned LOD0 has one chunk, BoneMap[3] == original magazine bone4.
    for index in range(count):
        at=start+index*64;indices=raw[at+56:at+60];weights=raw[at+60:at+64]
        if sum(weights)!=255:raise ValueError('Invalid original skin weights')
        belongs=sum(weight for bone,weight in zip(indices,weights) if bone==3)
        if belongs not in (0,255):raise ValueError('Magazine has nonrigid vertices')
        target.append(belongs==255)
    used=[i for i,value in enumerate(target) if value]
    if len(used)!=279:raise ValueError('Original magazine vertex set changed')
    remap={old:new for new,old in enumerate(used)};out=bytearray()
    def unpack(at):return _unit(tuple(b/127.5-1 for b in raw[at:at+3]))
    def pack(value):return bytes(max(0,min(255,round((v+1)*127.5))) for v in value)+b'\x80'
    for index in used:
        at=start+index*64;position=struct.unpack_from('<3f',raw,at)
        if not all(math.isfinite(v) and abs(v)<10000 for v in position):raise ValueError('Invalid original position')
        x=unpack(at+12);n=unpack(at+16);sign=raw[at+19]
        if sign not in (0,255):raise ValueError('Unsupported original tangent sign')
        y=_unit((n[1]*x[2]-n[2]*x[1],n[2]*x[0]-n[0]*x[2],n[0]*x[1]-n[1]*x[0]))
        if sign==0:y=tuple(-v for v in y)
        uv=raw[at+20:at+52]
        if not all(math.isfinite(v) and abs(v)<100000 for v in struct.unpack('<8f',uv)):
            raise ValueError('Invalid original UV')
        # Native mesh axes, nominal original units/100. The live adapter uses
        # identity AXES for this source; never apply the GLB axis conversion.
        out.extend(struct.pack('<3f',*(v/100 for v in position)))
        out.extend(pack(x)+pack(y)+pack(n)+uv+raw[at+52:at+56])
    original=struct.unpack_from('<9135H',raw,620);triangles=[]
    for first in range(0,len(original),3):
        face=original[first:first+3]
        if any(i>=count for i in face):raise ValueError('Original index outside LOD0')
        members=[target[i] for i in face]
        if any(members):
            if not all(members):raise ValueError('Magazine crosses a skinned triangle')
            triangles.extend(remap[i] for i in face) # Preserve original native winding.
    if len(triangles)!=314*3:raise ValueError('Original magazine topology changed')
    return bytes(out),struct.pack('<942H',*triangles)


def prepare(game_exe,cache_root):
    if not VERTEX_SHA256 or not INDEX_SHA256:raise ValueError('Original part codec not frozen')
    game=Path(game_exe).resolve().parent.parent
    package=game/'WarGame/CookedPC/WarfareGame.u'
    requested=Path(cache_root).absolute()/CACHE_VERSION
    def no_links(path):
        for item in (path,*path.parents):
            try:info=item.lstat()
            except FileNotFoundError:continue
            if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                raise ValueError('Reparse private cache')
    no_links(requested)
    cache=requested.resolve()
    if cache==game or cache.is_relative_to(game):raise ValueError('Private cache must be outside the game')
    # Revalidate the purchased source even when the generated cache exists.
    # Only selected LZ4 blocks are decoded; no decompressed package is saved.
    raw=read_export(package)
    vertices,indices=build_input(raw)
    wanted=[('snub-magazine.vtx60',vertices,VERTEX_SHA256),('snub-magazine.idx16',indices,INDEX_SHA256)]
    for _,data,digest in wanted:
        if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Original part codec output changed')
    cache.mkdir(parents=True,exist_ok=True)
    for name,data,digest in wanted:
        path=cache/name
        no_links(path)
        if path.exists():
            if not path.is_file() or path.stat().st_size!=len(data):
                raise ValueError('Unexpected existing private part file')
            previous=path.read_bytes();old_hash=hashlib.sha256(previous).hexdigest()
            if old_hash==digest:continue
            # Preserve a corrupt owned cache before atomic replacement.
            backup=path.with_name(name+'.rejected-'+old_hash[:16]);no_links(backup)
            if not backup.exists():
                with backup.open('xb') as stream:stream.write(previous)
            elif backup.read_bytes()!=previous:raise ValueError('Private cache backup collision')
        fd,temp=tempfile.mkstemp(prefix=name+'.',suffix='.tmp',dir=cache)
        try:
            with os.fdopen(fd,'wb') as stream:stream.write(data)
            os.replace(temp,path)
        finally:
            if os.path.exists(temp):os.unlink(temp)
    metadata={'schema':1,'private_game_derived':True,'distribution_allowed':False,
        'source_package_sha256':PACKAGE_SHA256,'source_export_sha256':EXPORT_SHA256,
        'vertices':279,'triangles':314,'basis':'original_mesh_metres','source_units_per_metre':100,
        'vertex_sha256':VERTEX_SHA256,'index_sha256':INDEX_SHA256}
    metadata_path=cache/'source.json';no_links(metadata_path)
    fd,temp=tempfile.mkstemp(prefix='source.',suffix='.tmp',dir=cache)
    try:
        with os.fdopen(fd,'w',encoding='utf-8') as stream:json.dump(metadata,stream,indent=2);stream.write('\n')
        os.replace(temp,metadata_path)
    finally:
        if os.path.exists(temp):os.unlink(temp)
    return {'directory':str(cache),'source_sha256':EXPORT_SHA256,'vertices':279,'triangles':314}
