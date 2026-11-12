from pathlib import Path
import idaapi,ida_funcs,ida_bytes,idautils,idc,ida_hexrays,json,traceback
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/original-l2-points-r257');bias=0x140001000
try:
 entries={};rows=[]
 for rva in (0x10f6cc,0x10eb80):
  refs=[]
  for x in idautils.XrefsTo(rva-0x1000):
   f=ida_funcs.get_func(x.frm)
   refs.append(dict(at=hex(x.frm+bias),function=hex(f.start_ea+bias) if f else None))
   if f:entries.setdefault(f.start_ea,set()).add(rva)
  rows.append(dict(target=hex(0x140000000+rva),refs=refs))
 assert ida_hexrays.init_hexrays_plugin()
 records=[]
 for entry,targets in sorted(entries.items()):
  size=sum(b-a for a,b in idautils.Chunks(entry))
  if size>12000:continue
  pseudo=str(ida_hexrays.decompile(entry));path=out/(hex(entry+bias)+'-caller.c');path.write_text(pseudo,encoding='utf-8')
  records.append(dict(entry=hex(entry+bias),targets=[hex(x) for x in targets],size=size,file=path.name))
 (out/'getter-callers.json').write_text(json.dumps(dict(refs=rows,records=records),indent=2)+'\n');idc.qexit(0)
except BaseException:
 (out/'callers-error.txt').write_text(traceback.format_exc());idc.qexit(1)
