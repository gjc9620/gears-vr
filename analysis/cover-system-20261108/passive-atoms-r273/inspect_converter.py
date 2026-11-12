from pathlib import Path
import ida_auto,ida_bytes,ida_funcs,ida_hexrays,ida_lines,ida_pro,idautils,idc,hashlib,json,traceback
out=Path(r"J:/codexWorktree/gearsVr/analysis/cover-system-20261108/passive-atoms-r273")
try:
 ida_auto.auto_wait();assert ida_hexrays.init_hexrays_plugin()
 ea=0x1400f6100;f=ida_funcs.get_func(ea);assert f and f.start_ea==ea
 rawfile=Path(r"J:/gearsVr/analysis/ida/camera-110878-20261014/section_1000.bin").read_bytes();chunks=[]
 for begin,end in idautils.Chunks(ea):
  data=ida_bytes.get_bytes(begin,end-begin);assert data==rawfile[begin-0x140001000:end-0x140001000]
  chunks.append(dict(begin=hex(begin),size=end-begin,sha256=hashlib.sha256(data).hexdigest()))
 (out/'converter-pseudocode.txt').write_text(str(ida_hexrays.decompile(f)),encoding='utf-8')
 (out/'converter-assembly.txt').write_text('\n'.join(hex(e)+' '+ida_lines.tag_remove(idc.generate_disasm_line(e,0)) for e in idautils.FuncItems(ea)),encoding='utf-8')
 (out/'converter-result.json').write_text(json.dumps(dict(chunks=chunks,raw_code_sha256=hashlib.sha256(rawfile).hexdigest(),saved_artifact_only=True,current_callee_bytes_verified=False),indent=2)+'\n')
except BaseException:
 (out/'converter-error.txt').write_text(traceback.format_exc());ida_pro.qexit(1)
ida_pro.qexit(0)
