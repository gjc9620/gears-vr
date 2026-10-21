"""Add verified historical data sections to a private copy; retry failures once."""
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3
import time
from ida_export import atomic,digest

ROOT=Path('J:/gearsVr')
OUT=ROOT/'analysis/re-kb/data-recovery'
MANIFEST=ROOT/'analysis/ghidra/render-16812-verified/manifest.json'


def load_data():
    import ida_segment,ida_bytes,ida_loader,ida_auto,idaapi
    idb=Path(ida_loader.get_path(ida_loader.PATH_TYPE_IDB)).resolve()
    if idb.parent!=OUT.resolve():
        raise ValueError('recovery must use its private IDB')
    manifest=json.loads(MANIFEST.read_text())
    sections=[]
    for s in manifest['sections']:
        data=(MANIFEST.parent/s['file']).read_bytes()
        if len(data)!=s['size'] or hashlib.sha256(data).hexdigest()!=s['sha256']:
            raise ValueError('snapshot hash mismatch')
        sections.append((s,data))
    for s,data in sections:
        start=int(s['address'],0); seg=ida_segment.getseg(start)
        if seg is None:
            if s['executable']:
                raise ValueError('expected existing code segment')
            seg=ida_segment.segment_t();seg.start_ea=start;seg.end_ea=start+len(data)
            seg.bitness=2;seg.sel=ida_segment.setup_selector(0)
            seg.perm=ida_segment.SEGPERM_READ;seg.type=ida_segment.SEG_DATA
            if not ida_segment.add_segm_ex(seg,'snapshot_'+s['rva'][2:],'DATA',0):
                raise RuntimeError('segment creation failed')
            ida_bytes.put_bytes(start,data)
        if ida_bytes.get_bytes(start,len(data))!=data:
            raise ValueError('loaded segment bytes mismatch')
    ida_auto.auto_wait()
    for s,data in sections:
        if ida_bytes.get_bytes(int(s['address'],0),len(data))!=data:
            raise ValueError('analysis changed snapshot bytes')
    profile={'manifest_sha256':digest(MANIFEST),'ida':idaapi.get_kernel_version(),
             'sections':manifest['sections'],'operation':'verified-data-load-and-failed-only-retry-v1'}
    profile['profile_id']=hashlib.sha256(json.dumps(profile,sort_keys=True).encode()).hexdigest()
    atomic(OUT/'profile.json',json.dumps(profile,indent=2))
    return {'segments_verified':len(sections),'profile_id':profile['profile_id']}


def retry(seconds=45):
    import ida_hexrays,ida_bytes
    profile=json.loads((OUT/'profile.json').read_text()); pid=profile['profile_id']
    # Always recheck loaded bytes; a receipt is not evidence of current IDB state.
    for s in profile['sections']:
        data=ida_bytes.get_bytes(int(s['address'],0),s['size'])
        if data is None or hashlib.sha256(data).hexdigest()!=s['sha256']:
            raise ValueError('recovery database bytes changed')
    if not ida_hexrays.init_hexrays_plugin(): raise RuntimeError('Hex-Rays unavailable')
    (OUT/'pseudocode').mkdir(exist_ok=True)
    with closing(sqlite3.connect(ROOT/'analysis/re-kb/knowledge.sqlite')) as db:
        db.execute('PRAGMA synchronous=NORMAL')
        db.execute('''CREATE TABLE IF NOT EXISTS recovery_attempts(
          artifact_id INTEGER,va TEXT,profile_id TEXT,old_error TEXT,new_error TEXT,
          status TEXT,pseudocode_path TEXT,pseudocode_sha256 TEXT,
          PRIMARY KEY(artifact_id,va,profile_id))''')
        rows=db.execute('''SELECT f.artifact_id,f.va,f.error,f.metadata FROM functions f
          WHERE f.status='failed' AND NOT EXISTS(SELECT 1 FROM recovery_attempts a
          WHERE a.artifact_id=f.artifact_id AND a.va=f.va AND a.profile_id=?)''',(pid,)).fetchall()
        begun=time.monotonic();processed=0
        for aid,va,old_error,metadata in rows:
            if processed and time.monotonic()-begun>=seconds:break
            error=None;path=None;sha=None;status='failed'
            try:
                failure=ida_hexrays.hexrays_failure_t()
                result=ida_hexrays.decompile(int(va,16),failure,ida_hexrays.DECOMP_NO_CACHE)
                if result is None:raise RuntimeError(str(failure))
                path=OUT/'pseudocode'/(va+'.c');atomic(path,str(result));sha=digest(path);status='success'
            except Exception as exc:error=str(exc)
            with db:
                db.execute('INSERT INTO recovery_attempts VALUES(?,?,?,?,?,?,?,?)',
                    (aid,va,pid,old_error,error,status,str(path) if path else None,sha))
                if status=='success':
                    m=json.loads(metadata);m.update(recovery_profile=str(OUT/'profile.json'),
                        recovery_profile_id=pid,original_decompilation_error=old_error)
                    db.execute("UPDATE functions SET status='success',error=NULL,pseudocode_path=?,pseudocode_sha256=?,metadata=? WHERE artifact_id=? AND va=?",
                        (str(path),sha,json.dumps(m),aid,va))
            processed+=1
        summary={'processed':processed,'remaining_this_profile':len(rows)-processed,
            'attempts':dict(db.execute('SELECT status,count(*) FROM recovery_attempts WHERE profile_id=? GROUP BY status',(pid,)))}
        atomic(OUT/'status.json',json.dumps(summary,indent=2))
        return summary
