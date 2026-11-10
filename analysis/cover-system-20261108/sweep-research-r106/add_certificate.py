"""Bind four traced callee bodies to the original PE file; no DLL execution."""
from pathlib import Path
import hashlib
import json
import struct

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
IMAGE=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
EXPECTED='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
sha=lambda raw:hashlib.sha256(raw).hexdigest()
raw=IMAGE.read_bytes()
assert sha(raw)==EXPECTED and len(raw)==2711552
u16=lambda at:struct.unpack_from('<H',raw,at)[0]
u32=lambda at:struct.unpack_from('<I',raw,at)[0]
pe=u32(0x3c)
assert raw[pe:pe+4]==b'PE\0\0' and u16(pe+4)==0x8664
table=pe+24+u16(pe+20)
sections=[struct.unpack_from('<IIII',raw,table+i*40+8) for i in range(u16(pe+6))]
header=ROOT/'tools/native-capture/cover_physx_query_code.h'
text=header.read_text(encoding='utf-8-sig')
assert text.count('std::array<Chunk,22>')==1
add=[];records=[]
for filename,rva,size in (('box-inner.json',0xd5a70,516),('triangles-inner.json',0xd9490,1354),
                          ('ray-capsule.json',0xd0180,1633),('sphere-triangles.json',0xd7570,2232)):
    evidence=json.loads((HERE/filename).read_text())
    assert evidence['subject']['digest']['sha256']==EXPECTED
    body=evidence['raw_result']['procedure']['body']
    assert body['available'] and body['contains_entry'] and not body['non_contiguous']
    assert body['total_bytes']==size and body['ranges']==[{'start':hex(0x180000000+rva),'end':hex(0x180000000+rva+size-1)}]
    ranges=[(off+rva-va,size) for vs,va,n,off in sections if va<=rva and rva+size<=va+n and off+n<=len(raw)]
    assert len(ranges)==1
    off,n=ranges[0];digest=sha(raw[off:off+n])
    add.append('    {0x%x,%d,{{%s}}},'%(rva,size,','.join('0x'+digest[i:i+2] for i in range(0,64,2))))
    records.append(dict(evidence=filename,evidence_id=evidence['evidence_id'],evidence_sha256=sha((HERE/filename).read_bytes()),
                        rva=hex(rva),file_offset=off,size=size,sha256=digest))
before=sha(header.read_bytes())
text=text.replace('std::array<Chunk,22>','std::array<Chunk,26>')
at=text.index('\n}};')
text=text[:at]+'\n    // Traced target-specific sweep consumers; not a transitive coverage claim.\n'+'\n'.join(add)+text[at:]
header.write_text(text,encoding='utf-8')
(HERE/'certificate-ranges.json').write_text(json.dumps(dict(image=str(IMAGE),image_sha256=EXPECTED,
    before_header_sha256=before,after_header_sha256=sha(header.read_bytes()),added_bytes=sum(r['size'] for r in records),
    total_chunks=26,total_bytes=16705,records=records,complete_call_graph_certified=False,
    sphere_sweep_enabled=False,movement_authorized=False),indent=2),encoding='utf-8')
print('Added four original callee ranges, 5735 bytes; no native execution')
