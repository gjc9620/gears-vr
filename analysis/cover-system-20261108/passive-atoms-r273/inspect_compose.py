from pathlib import Path
import hashlib,json,traceback
import ida_auto,ida_bytes,ida_funcs,ida_hexrays,ida_lines,ida_pro,idautils,idc
out=Path(r"J:/codexWorktree/gearsVr/analysis/cover-system-20261108/passive-atoms-r273")
try:
 ida_auto.auto_wait()
 checks=[]
 for ea,size,expected in [(0x140128920,5018,'4b65c4f0bbd5f12af73bde81a459dc222efd1c554e52ff1754ea7b2ae04c82a1'),(0x1408a9500,567,'d4a5080e659dc461365cdc0320a04b0d765571e0b2d6095595a05c60e7e3088a')]:
  raw=ida_bytes.get_bytes(ea,size);actual=hashlib.sha256(raw).hexdigest() if raw else None
  checks.append(dict(ea=hex(ea),size=size,sha256=actual,expected=expected));assert actual==expected,checks
 assert ida_hexrays.init_hexrays_plugin();function=ida_funcs.get_func(0x140128920);assert function
 pseudo=ida_hexrays.decompile(function);assert pseudo
 (out/'compose-pseudocode.txt').write_text(str(pseudo),encoding='utf-8')
 lines=[];callees=set()
 for ea in idautils.FuncItems(function.start_ea):
  lines.append(hex(ea)+' '+ida_lines.tag_remove(idc.generate_disasm_line(ea,0)))
  if idc.print_insn_mnem(ea)=='call':
   for target in idautils.CodeRefsFrom(ea,False):callees.add(target)
 (out/'compose-assembly.txt').write_text('\n'.join(lines),encoding='utf-8')
 (out/'compose-result.json').write_text(json.dumps(dict(okay=True,chunks=checks,start=hex(function.start_ea),callees=[hex(x) for x in sorted(callees)]),indent=2)+'\n')
except BaseException:
 (out/'compose-error.txt').write_text(traceback.format_exc(),encoding='utf-8');ida_pro.qexit(1)
ida_pro.qexit(0)
