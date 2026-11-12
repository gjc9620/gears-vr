from pathlib import Path
import ida_funcs,ida_bytes,idautils,idc,json,hashlib
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/original-l2-points-r257');bias=0x140001000
entry=0x3b51b0;chunks=list(idautils.Chunks(entry));rows=[];asm=[]
for a,b in chunks:
 raw=ida_bytes.get_bytes(a,b-a);rows.append(dict(rva=hex(a+0x1000),size=b-a,sha256=hashlib.sha256(raw).hexdigest()))
 for e in idautils.Heads(a,b):asm.append(hex(e+bias)+' '+idc.generate_disasm_line(e,0))
(out/'viewpoint-asm.txt').write_text('\n'.join(asm)+'\n',encoding='utf-8')
(out/'viewpoint-chunks.json').write_text(json.dumps(rows,indent=2)+'\n')
idc.qexit(0)
