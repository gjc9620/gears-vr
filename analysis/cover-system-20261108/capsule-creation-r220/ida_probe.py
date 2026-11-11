import idaapi,ida_funcs,ida_bytes,ida_nalt,idautils,idc,ida_hexrays
from pathlib import Path
import hashlib,json,traceback
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/capsule-creation-r220')
try:
 source=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin').read_bytes()
 assert hashlib.sha256(source).hexdigest()=='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
 entries=set();refs=[]
 bias=0x140001000
 entries.add(0x1402cf478-bias)
 entries.add(0x1402cfa6c-bias)
 for target in ():
  for ref in idautils.XrefsTo(target):
   f=ida_funcs.get_func(ref.frm)
   refs.append(dict(target=hex(target+bias),at=hex(ref.frm+bias),entry=hex(f.start_ea+bias) if f else None))
   if f:entries.add(f.start_ea)
 records=[]
 assert ida_hexrays.init_hexrays_plugin()
 for entry in sorted(entries):
  chunks=list(idautils.Chunks(entry));size=sum(b-a for a,b in chunks)
  if size>18000:continue
  ranges=[]
  for a,b in chunks:
   raw=ida_bytes.get_bytes(a,b-a);assert raw==source[a:b]
   ranges.append(dict(start=hex(a+bias),end_exclusive=hex(b+bias),bytes=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()))
  try:pseudo=str(ida_hexrays.decompile(entry))
  except Exception as e:pseudo='ERROR: '+repr(e)
  (out/(hex(entry+bias)+'-pseudocode.c')).write_bytes(pseudo.encode('utf-8'))
  instructions=[dict(address=hex(ea+bias),text=idc.generate_disasm_line(ea,0)) for a,b in chunks for ea in idautils.Heads(a,b)]
  records.append(dict(entry=hex(entry+bias),database_entry=hex(entry),ranges=ranges,instructions=instructions,pseudocode_sha256=hashlib.sha256(pseudo.encode()).hexdigest()))
 (out/'probe.json').write_text(json.dumps(dict(refs=refs,records=records,input=ida_nalt.get_input_file_path(),imagebase=hex(ida_nalt.get_imagebase()),segments=[dict(start=hex(s),name=idc.get_segm_name(s)) for s in idautils.Segments()],first_functions=[hex(f) for _,f in zip(range(8),idautils.Functions())]),indent=2),encoding='utf-8')
 idc.qexit(0)
except BaseException:
 (out/'probe-error.txt').write_text(traceback.format_exc(),encoding='utf-8');idc.qexit(1)
