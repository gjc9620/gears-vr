from pathlib import Path
import hashlib,json,traceback
import ida_auto,ida_bytes,ida_funcs,ida_hexrays,ida_lines,ida_pro,idautils,idc
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/cache-overwrite-r288')
try:
 ida_auto.auto_wait();assert ida_hexrays.init_hexrays_plugin();results=[]
 original=Path('J:/gearsVr/analysis/ida/camera-110878-20261014/section_1000.bin').read_bytes()
 for rva,size,expected in [(0xf7940,2569,'418cdc8a350ce1a4b176126345e5c890aef09d4ed9b9599e7f1d7d5a3d6e91b0'),(0xfacb0,4055,'660fe88e7f2237e6aec7f288e2c34ffb69212cf863ce89ce783551ce4e27f7a0'),(0xf5b78,None,None)]:
  ea=0x140000000+rva;f=ida_funcs.get_func(ea);assert f
  count=size or f.end_ea-f.start_ea;start=ea if size else f.start_ea
  raw=ida_bytes.get_bytes(start,count);assert raw==original[start-0x140001000:start-0x140001000+count]
  digest=hashlib.sha256(raw).hexdigest();assert not expected or digest==expected
  pseudo=ida_hexrays.decompile(f);assert pseudo
  (out/(hex(rva)+'-pseudocode.txt')).write_text(str(pseudo),encoding='utf-8')
  lines=[hex(at)+' '+ida_lines.tag_remove(idc.generate_disasm_line(at,0)) for at in idautils.FuncItems(f.start_ea)]
  (out/(hex(rva)+'-assembly.txt')).write_text('\n'.join(lines),encoding='utf-8')
  results.append(dict(requested=hex(ea),function_start=hex(f.start_ea),end=hex(f.end_ea),checked_start=hex(start),checked_bytes=count,sha256=digest,raw_file_matched=True))
 (out/'original-functions.json').write_text(json.dumps(dict(okay=True,functions=results,raw_section_sha256=hashlib.sha256(original).hexdigest(),execution='offline IDA CLI private DB; no game target'),indent=2)+'\n')
except BaseException:
 (out/'inspect-error.txt').write_text(traceback.format_exc(),encoding='utf-8');ida_pro.qexit(1)
ida_pro.qexit(0)
