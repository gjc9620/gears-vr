from pathlib import Path
import struct,hashlib,json
p=Path('C:/Windows/System32/VCRUNTIME140.dll');b=p.read_bytes()
def u16(o):return struct.unpack_from('<H',b,o)[0]
def u32(o):return struct.unpack_from('<I',b,o)[0]
pe=u32(0x3c);opt=pe+24;assert b[pe:pe+4]==b'PE\0\0' and u16(opt)==0x20b
sections=[];st=opt+u16(pe+20)
for i in range(u16(pe+6)):
 o=st+i*40;sections.append((u32(o+12),u32(o+8),u32(o+20),u32(o+16)))
def at(r):
 for va,vs,raw,size in sections:
  if va<=r<va+max(vs,size):assert r-va<size;return raw+r-va
 raise ValueError(hex(r))
def string(r):
 o=at(r);return b[o:b.index(b'\0',o,o+256)].decode('ascii')
e=at(u32(opt+112));functions,names,ordinals=u32(e+28),u32(e+32),u32(e+36);found=[]
for i in range(u32(e+24)):
 name=string(u32(at(names)+i*4))
 if name in ('memcpy','memmove'):
  rva=u32(at(functions)+u16(at(ordinals)+i*2)*4)
  found.append(dict(name=name,rva=hex(rva),prefix=b[at(rva):at(rva)+32].hex()))
result=dict(path=str(p),sha256=hashlib.sha256(b).hexdigest(),exports=found,recorded_r283_target_rva='0x1f020',recorded_prefix='e91bdbffffcccccccccccccccccccccce94bf5ffffcccccccccccccccccccccc',execution=False)
result['matches_recorded_memcpy']=any(v['name']=='memcpy' and v['rva']==result['recorded_r283_target_rva'] and v['prefix']==result['recorded_prefix'] for v in found)
Path(__file__).with_name('copy-export-proof.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
