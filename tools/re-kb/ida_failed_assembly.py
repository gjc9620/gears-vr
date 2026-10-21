"""IDAPython fallback: preserve addressed assembly for failed decompilations."""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3


def run():
    import idautils,idc,ida_bytes,ida_lines,idaapi
    root=Path('J:/gearsVr/analysis/re-kb/full-native/failed-assembly')
    root.mkdir(exist_ok=True)
    with closing(sqlite3.connect('J:/gearsVr/analysis/re-kb/knowledge.sqlite')) as db:
        rows=db.execute("SELECT artifact_id,va,error,metadata FROM functions WHERE status='failed'").fetchall()
        saved=0; items=0
        for aid,va,error,metadata in rows:
            lines=[]
            for ea in idautils.FuncItems(int(va,16)):
                size=ida_bytes.get_item_size(ea)
                raw=ida_bytes.get_bytes(ea,size)
                disasm=idc.generate_disasm_line(ea,0)
                lines.append({'address':hex(ea),'size':size,'bytes':raw.hex() if raw is not None else None,
                    'is_code':bool(ida_bytes.is_code(ida_bytes.get_flags(ea))),
                    'assembly':ida_lines.tag_remove(disasm) if disasm else None})
            path=root/(va+'.json')
            document={'artifact_id':aid,'function_va':va,'decompilation_error':error,'ida':idaapi.get_kernel_version(),
                'scope':'IDA-defined function items; not proof of complete control flow or runtime execution','items':lines}
            encoded=json.dumps(document,ensure_ascii=False,indent=2).encode('utf-8')
            temp=path.with_suffix('.tmp');temp.write_bytes(encoded);temp.replace(path)
            m=json.loads(metadata);m.update(assembly_path=str(path),assembly_sha256=hashlib.sha256(encoded).hexdigest())
            with db:
                db.execute('UPDATE functions SET metadata=? WHERE artifact_id=? AND va=?',(json.dumps(m),aid,va))
            saved+=1;items+=len(lines)
        return {'failed_functions_with_assembly':saved,'items':items}
