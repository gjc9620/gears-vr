from pathlib import Path
import hashlib,json,traceback
import ida_auto,ida_bytes,ida_funcs,ida_hexrays,ida_lines,ida_pro,idautils,idc
out=Path('J:/codexWorktree/gearsVr/analysis/cover-system-20261108/cache-overwrite-r288')
try:
 ida_auto.auto_wait();assert ida_hexrays.init_hexrays_plugin();ea=0x140577e5f;f=ida_funcs.get_func(ea);assert f
 lines=[hex(at)+' '+ida_lines.tag_remove(idc.generate_disasm_line(at,0)) for at in idautils.FuncItems(f.start_ea)]
 (out/'copy-assembly.txt').write_text('\n'.join(lines),encoding='utf-8')
 pseudo=ida_hexrays.decompile(f)
 (out/'copy-pseudocode.txt').write_text(str(pseudo),encoding='utf-8')
 refs=sorted(set(target for at in idautils.FuncItems(f.start_ea) for target in idautils.CodeRefsFrom(at,False) if not(f.start_ea<=target<f.end_ea)))
 raw=ida_bytes.get_bytes(f.start_ea,f.end_ea-f.start_ea)
 (out/'copy-result.json').write_text(json.dumps(dict(start=hex(f.start_ea),end=hex(f.end_ea),size=len(raw),sha256=hashlib.sha256(raw).hexdigest(),references=[hex(x) for x in refs]),indent=2)+'\n')
except BaseException:
 (out/'copy-error.txt').write_text(traceback.format_exc(),encoding='utf-8');ida_pro.qexit(1)
ida_pro.qexit(0)
