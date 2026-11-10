from pathlib import Path
import json,hashlib,struct,re,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
path=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()=='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
pe=struct.unpack_from('<I',raw,0x3c)[0];count=struct.unpack_from('<H',raw,pe+6)[0];opt=struct.unpack_from('<H',raw,pe+20)[0];table=pe+24+opt
sections=[struct.unpack_from('<IIII',raw,table+i*40+8) for i in range(count)]
def bytes_at(rva,size):
    for virtual_size,virtual_start,file_size,file_start in sections:
        if virtual_start<=rva and rva-virtual_start+size<=file_size:return raw[file_start+rva-virtual_start:file_start+rva-virtual_start+size]
    raise ValueError((rva,size))
chunks=[]
for name in ('public','internal','bridge','fallback','broad'):
    data=json.loads((H/(name+'-info.json')).read_text())['normalized_result'];body=data['body'];assert body['non_contiguous'] is False
    ranges=body['ranges'];assert len(ranges)==1
    start=int(ranges[0]['start'],16)-0x180000000;size=int(ranges[0]['end'],16)-0x180000000-start+1
    assert size==body['total_bytes'];chunks.append((start,size,hashlib.sha256(bytes_at(start,size)).digest()))
assert sum(x[1] for x in chunks)==1897
rows=['    {0x%x,%d,{{%s}}},'%(a,b,','.join('0x%02x'%x for x in d)) for a,b,d in chunks]
text='''#pragma once
#include "cover_physx_query_source.h"
// Selected complete capsule enumeration procedure ranges, including the public
// wrapper, full internal body, fallback bridge/iterator and broadphase entry.
// This does NOT certify all shape narrowphase/scale/bone coverage or world state.
namespace gears_cover_physx_overlap_code {
using Chunk=gears_cover_physx_query_code::Chunk;
inline constexpr std::array<Chunk,5> chunks{{
'''+ '\n'.join(rows)+'''
}};
template<class Read,class Hash> bool current(std::uintptr_t base,std::uint32_t image_size,Read& read,Hash hash){
    namespace s=gears_cover_physx_query_source;
    if(!base||(base&0xffff)||image_size>UINTPTR_MAX-base||image_size<0x23cd90)return false;
    std::uintptr_t public_method{},internal_method{};
    if(!s::pose::field(read,public_method,base+0x23ca98,0x2f0)||public_method!=base+0x172160||
       !s::pose::field(read,internal_method,base+0x22ccb8,0x248)||internal_method!=base+0xa9f10)return false;
    std::array<unsigned char,1233> buffer{};
    for(auto const& row:chunks){s::Digest digest{};
        if(!row.size||row.size>buffer.size()||row.rva>=image_size||row.size>image_size-row.rva||
           !s::pose::bytes(read,buffer.data(),base+row.rva,row.size)||!hash(buffer.data(),row.size,digest)||digest!=row.sha256)return false;}
    return true;
}
}
'''
for before,after in (('base','image_base'),('buffer','code_buffer'),('row','code_row'),('digest','observed_digest')):
    text=re.sub(r'\b'+before+r'\b',after,text)
destination=R/'tools/native-capture/cover_physx_overlap_code.h'
if sys.argv[1:]==['--check']:
    assert destination.read_text()==text
    print('Generated certificate matches current header, 5 complete ranges / 1897 bytes');raise SystemExit(0)
destination.write_text(text,encoding='utf8')
(H/'certificate.json').write_text(json.dumps(dict(chunks=[dict(rva=hex(a),bytes=b,sha256=d.hex()) for a,b,d in chunks],total_bytes=1897),indent=2)+'\n')
print('Certified selected enumeration ranges: 5 / 1897 bytes; original read as data only')
