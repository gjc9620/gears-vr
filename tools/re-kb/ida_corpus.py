"""Resumable full IDA census, reference index and bounded pseudocode batches.

Call run(stage, seconds) through IDAPython, on the corpus-owned private IDB.
No target execution; no function text is returned through MCP.
"""
import hashlib
import json
import os
from pathlib import Path
import sys
import time

ROOT = Path('J:/gearsVr')
OUT = ROOT / 'analysis/re-kb/full-native'
sys.path.insert(0, str(ROOT / 'tools/re-kb'))
import kb
from ida_export import atomic, digest


def run(stage='census', seconds=45, batch_size=100):
    import ida_bytes, ida_funcs, ida_hexrays, ida_nalt, ida_xref, idautils
    import idaapi
    config = json.loads((ROOT/'analysis/re-kb/pilot-1000/config.json').read_text())
    artifact = config['artifact']
    if digest(ida_nalt.get_input_file_path()) != artifact['sha256']:
        raise ValueError('IDA input identity mismatch')
    source = Path(artifact['path']).read_bytes()
    if hashlib.sha256(source).hexdigest() != artifact['sha256']:
        raise ValueError('source snapshot changed')
    base, start = int(artifact['module_base'],0), int(config['load_address'],0)
    OUT.mkdir(exist_ok=True)
    lock = OUT/'writer.lock'
    with lock.open('x') as stream:
        stream.write(str(os.getpid()))
    connection = None
    begin = time.monotonic()
    processed = 0
    try:
        connection = kb.connect(ROOT/'analysis/re-kb/knowledge.sqlite')
        connection.execute('PRAGMA journal_mode=WAL')
        connection.execute('PRAGMA synchronous=NORMAL')
        connection.executescript('''
          CREATE TABLE IF NOT EXISTS corpus_work(
            artifact_id INTEGER, va TEXT, indexed INTEGER DEFAULT 0,
            index_error TEXT, PRIMARY KEY(artifact_id,va));
        ''')
        aid = connection.execute('SELECT id FROM artifacts WHERE sha256=? AND kind=? AND module_base=?',
                                 (artifact['sha256'],artifact['kind'],artifact['module_base'])).fetchone()[0]
        if stage == 'census':
            functions = list(idautils.Functions())
            with connection:
                for ea in functions:
                    va = hex(ea)
                    f = ida_funcs.get_func(ea)
                    connection.execute('INSERT OR IGNORE INTO corpus_work(artifact_id,va) VALUES(?,?)',(aid,va))
                    connection.execute('''INSERT OR IGNORE INTO functions VALUES(?,?,?,?,?,?,?,?,?,?)''',
                        (aid,va,hex(ea-base),ida_funcs.get_func_name(ea),None,'skipped',None,None,None,
                         json.dumps({'flags':f.flags,'status_reason':'indexed name; pseudocode not attempted',
                                     'end_ea':hex(f.end_ea)})))
                connection.execute('UPDATE artifacts SET total_functions=? WHERE id=?',(len(functions),aid))
            atomic(OUT/'census.json',json.dumps({'functions':len(functions),'artifact':artifact,
                'ida':idaapi.get_kernel_version(),'source_idb':'analysis/re-kb/pilot-1000/snapshot.i64',
                'scope':'IDA-defined functions in one historical runtime code section',
                'created_unix':time.time()},indent=2))
            processed = len(functions)
        elif stage == 'index':
            rows = connection.execute('SELECT va FROM corpus_work WHERE artifact_id=? AND indexed=0 ORDER BY va',(aid,)).fetchall()
            for row in rows:
                if processed and time.monotonic()-begin >= seconds:
                    break
                ea = int(row['va'],16)
                try:
                    chunks = list(idautils.Chunks(ea))
                    parts = []
                    for a,b in chunks:
                        data = ida_bytes.get_bytes(a,b-a)
                        if a < start or data is None or data != source[a-start:b-start]:
                            raise ValueError('function chunk differs from source snapshot')
                        parts.append(data)
                    refs = set()
                    calls = set()
                    for address in idautils.FuncItems(ea):
                        for x in idautils.XrefsFrom(address,0):
                            if x.type != ida_xref.fl_F:
                                refs.add((x.frm,x.to,x.type,bool(x.iscode)))
                            if x.type in (ida_xref.fl_CN,ida_xref.fl_CF):
                                calls.add((x.frm,x.to))
                    for x in idautils.XrefsTo(ea,0):
                        if x.type != ida_xref.fl_F:
                            refs.add((x.frm,x.to,x.type,bool(x.iscode)))
                    with connection:
                        connection.execute('DELETE FROM edges WHERE artifact_id=? AND function_va=?',(aid,hex(ea)))
                        for a,b,t,code in sorted(refs):
                            evidence = json.dumps({'from':hex(a),'to':hex(b),'type':str(t),'iscode':code})
                            connection.execute('INSERT INTO edges VALUES(?,?,?,?,?,?,?)',
                                (aid,hex(ea),'xrefs',hex(a),hex(b),str(t),evidence))
                        # Calls are outgoing only; incoming references are not this function's calls.
                        for address,target in sorted(calls):
                            connection.execute('INSERT INTO edges VALUES(?,?,?,?,?,?,?)',
                                (aid,hex(ea),'calls',hex(address),hex(target),'call',json.dumps({'source':'IDA'})))
                        old = json.loads(connection.execute('SELECT metadata FROM functions WHERE artifact_id=? AND va=?',(aid,hex(ea))).fetchone()[0])
                        old.update(chunks=[[hex(a),hex(b)] for a,b in chunks],snapshot_bytes_verified=True,
                                   references_scope='IDA xrefs; indirect targets unresolved; data sections absent')
                        connection.execute('UPDATE functions SET bytes_sha256=?,metadata=? WHERE artifact_id=? AND va=?',
                            (hashlib.sha256(b''.join(parts)).hexdigest(),json.dumps(old),aid,hex(ea)))
                        connection.execute('UPDATE corpus_work SET indexed=1,index_error=NULL WHERE artifact_id=? AND va=?',(aid,hex(ea)))
                except Exception as exc:
                    with connection:
                        connection.execute('UPDATE corpus_work SET indexed=-1,index_error=? WHERE artifact_id=? AND va=?',(repr(exc),aid,hex(ea)))
                processed += 1
        elif stage == 'decompile':
            if not ida_hexrays.init_hexrays_plugin():
                raise RuntimeError('Hex-Rays unavailable')
            for directory in ('pseudocode','batches','records'):
                (OUT/directory).mkdir(exist_ok=True)
            while time.monotonic()-begin < seconds:
                rows = connection.execute('''SELECT f.va FROM functions f JOIN corpus_work w
                    ON f.artifact_id=w.artifact_id AND f.va=w.va
                    WHERE f.artifact_id=? AND f.status='skipped' AND w.indexed=1 ORDER BY f.va LIMIT ?''',
                    (aid,batch_size)).fetchall()
                addresses = [int(r[0],16) for r in rows]
                if not addresses:
                    break
                # Empty address list would request the entire database.
                batch_error = None
                try:
                    ok = ida_hexrays.decompile_many(str(OUT/'batches'/f'{addresses[0]:x}.c'),addresses,ida_hexrays.VDRUN_SILENT)
                    if not ok:
                        batch_error = 'batch cancelled or internal error; inspect individual results'
                except Exception as exc:
                    batch_error = repr(exc)
                for ea in addresses:
                    record = {'va':hex(ea),'batch_error':batch_error,'status':'failed','error':None}
                    path = None
                    sha = None
                    try:
                        failure = ida_hexrays.hexrays_failure_t()
                        func = ida_hexrays.decompile(ea,failure)
                        if func is None:
                            raise RuntimeError(str(failure))
                        path = OUT/'pseudocode'/f'{ea:x}.c'
                        atomic(path,str(func))
                        sha = digest(path)
                        record.update(status='success',pseudocode_path=str(path),pseudocode_sha256=sha)
                    except Exception as exc:
                        record['error'] = str(exc)
                    atomic(OUT/'records'/f'{ea:x}.json',json.dumps(record))
                    with connection:
                        connection.execute('UPDATE functions SET status=?,error=?,pseudocode_path=?,pseudocode_sha256=? WHERE artifact_id=? AND va=?',
                            (record['status'],record['error'],str(path) if path else None,sha,aid,hex(ea)))
                    processed += 1
        else:
            raise ValueError('unknown stage')
        summary = {'stage':stage,'processed':processed,'elapsed_seconds':time.monotonic()-begin,
                   'index':dict(connection.execute('SELECT indexed,count(*) FROM corpus_work WHERE artifact_id=? GROUP BY indexed',(aid,))),
                   'pseudocode':dict(connection.execute('SELECT status,count(*) FROM functions WHERE artifact_id=? GROUP BY status',(aid,))),
                   'updated_unix':time.time(),'worker_pid':os.getpid(),'ai_tokens':None}
        atomic(OUT/'status.json',json.dumps(summary,indent=2))
        with (OUT/'runs.jsonl').open('a') as stream:
            stream.write(json.dumps(summary)+'\n')
        return summary
    finally:
        if connection is not None:
            connection.close()
        lock.unlink()
