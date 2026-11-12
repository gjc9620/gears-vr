from pathlib import Path
import struct,json,hashlib
out=Path(__file__).resolve().parent;root=out.parents[2]
runtime=json.loads((root/'analysis/runtime/continuous-stereo-53384-20261112-160149-0e2b6dc9.json').read_text())
p=Path(runtime['game_identity']['image']);data=p.read_bytes();digest=hashlib.sha256(data).hexdigest();assert digest==runtime['diagnostics_artifacts']['game_sha256']
def u16(at):return struct.unpack_from('<H',data,at)[0]
def u32(at):return struct.unpack_from('<I',data,at)[0]
def u64(at):return struct.unpack_from('<Q',data,at)[0]
pe=u32(0x3c);assert data[pe:pe+4]==b'PE\0\0';optional=pe+24;assert u16(optional)==0x20b
sections=[];at=optional+u16(pe+20)
for i in range(u16(pe+6)):
 s=at+i*40;sections.append((u32(s+12),u32(s+8),u32(s+20),u32(s+16)))
def offset(rva):
 for va,vs,raw,rs in sections:
  if va<=rva<va+max(vs,rs):
   assert rva-va<rs;return raw+rva-va
 raise ValueError(hex(rva))
def string(rva):
 start=offset(rva);end=data.index(b'\0',start,start+512);return data[start:end].decode('ascii')
import_rva=u32(optional+120);found=[];descriptor=offset(import_rva)
for i in range(512):
 at=descriptor+i*20;ilt,stamp,chain,name,iat=struct.unpack_from('<5I',data,at)
 if not any((ilt,stamp,chain,name,iat)):break
 if not ilt:continue
 for j in range(8192):
  thunk=u64(offset(ilt+j*8))
  if not thunk:break
  if iat+j*8==0x131ca18:
   found.append(dict(dll=string(name),function=string(thunk+2) if not thunk>>63 else 'ordinal:'+str(thunk&0xffff),iat_rva=hex(iat+j*8)))
result=dict(game_file=str(p),sha256=digest,import_directory_rva=hex(import_rva),matches=found,execution=False)
(out/'copy-import.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
