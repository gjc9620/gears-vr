from pathlib import Path
import ida_auto,ida_funcs,ida_hexrays,ida_lines,ida_bytes,ida_nalt,ida_pro,idautils,idc,json,hashlib,traceback
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/cache-overwrite-r288')
try:
 ida_auto.auto_wait();assert ida_hexrays.init_hexrays_plugin();base=ida_nalt.get_imagebase();results=[]
 for rva in (0x1cb40,0x1db00):
  f=ida_funcs.get_func(base+rva);assert f
  lines=[];refs=set()
  for ea in idautils.FuncItems(f.start_ea):
   lines.append(hex(ea-base)+' '+ida_lines.tag_remove(idc.generate_disasm_line(ea,0)))
   refs.update(x-base for x in idautils.CodeRefsFrom(ea,False) if not(f.start_ea<=x<f.end_ea))
  raw=ida_bytes.get_bytes(f.start_ea,f.end_ea-f.start_ea)
  (out/(hex(rva)+'-crt-assembly.txt')).write_text('\n'.join(lines),encoding='utf-8')
  (out/(hex(rva)+'-crt-pseudocode.txt')).write_text(str(ida_hexrays.decompile(f)),encoding='utf-8')
  results.append(dict(start=hex(f.start_ea-base),end=hex(f.end_ea-base),sha256=hashlib.sha256(raw).hexdigest(),external_code_refs=[hex(x) for x in sorted(refs)]))
 (out/'crt-code-result.json').write_text(json.dumps(dict(base=hex(base),functions=results),indent=2)+'\n')
except BaseException:
 (out/'crt-error.txt').write_text(traceback.format_exc(),encoding='utf-8');ida_pro.qexit(1)
ida_pro.qexit(0)
