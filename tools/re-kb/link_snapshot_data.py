"""Join existing IDA data references to same-capture, hash-verified sections/literals."""
import argparse
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3


def link(database,manifest_path):
    manifest_path=Path(manifest_path).resolve()
    raw=manifest_path.read_bytes();capture=hashlib.sha256(raw).hexdigest()
    manifest=json.loads(raw)
    for s in manifest['sections']:
        data=(manifest_path.parent/s['file']).read_bytes()
        if len(data)!=s['size'] or hashlib.sha256(data).hexdigest()!=s['sha256']:
            raise ValueError('capture section hash mismatch')
    with closing(sqlite3.connect(database)) as db:
        db.execute('PRAGMA synchronous=NORMAL')
        code_ids=[]
        for s in manifest['sections']:
            if s['executable']:
                row=db.execute("SELECT id FROM artifacts WHERE sha256=? AND kind='runtime_snapshot' AND module_base=?",
                    (s['sha256'],manifest['base'])).fetchone()
                if row:code_ids.append(row[0])
        if not code_ids:raise ValueError('no code artifact matches this capture')
        db.executescript('''
          CREATE TABLE IF NOT EXISTS snapshot_data_refs(
            capture_sha256 TEXT,artifact_id INTEGER,function_va TEXT,source TEXT,target TEXT,
            section_sha256 TEXT,edge_type TEXT,
            PRIMARY KEY(capture_sha256,artifact_id,function_va,source,target,edge_type));
          CREATE TABLE IF NOT EXISTS snapshot_literal_links(
            capture_sha256 TEXT,artifact_id INTEGER,function_va TEXT,source TEXT,target TEXT,
            section_sha256 TEXT,encoding TEXT,
            PRIMARY KEY(capture_sha256,artifact_id,function_va,source,target,encoding));
          CREATE INDEX IF NOT EXISTS snapshot_literal_address ON snapshot_literals(manifest_sha256,address);
          CREATE INDEX IF NOT EXISTS snapshot_links_target ON snapshot_literal_links(artifact_id,target);
        ''')
        with db:
            db.execute('DELETE FROM snapshot_data_refs WHERE capture_sha256=?',(capture,))
            db.execute('DELETE FROM snapshot_literal_links WHERE capture_sha256=?',(capture,))
            for s in manifest['sections']:
                if s['executable']:continue
                start=hex(int(s['address'],0));end=hex(int(s['address'],0)+s['size'])
                if len(start)!=len(end):raise ValueError('hex address width changes across section')
                for aid in code_ids:
                    db.execute('''INSERT OR IGNORE INTO snapshot_data_refs
                        SELECT ?,artifact_id,function_va,source,target,?,edge_type FROM edges
                        WHERE artifact_id=? AND category='xrefs' AND json_extract(evidence,'$.iscode')=0
                        AND length(target)=? AND target>=? AND target<?''',
                        (capture,s['sha256'],aid,len(start),start,end))
            db.execute('''INSERT OR IGNORE INTO snapshot_literal_links
                SELECT r.capture_sha256,r.artifact_id,r.function_va,r.source,r.target,r.section_sha256,l.encoding
                FROM snapshot_data_refs r JOIN snapshot_literals l
                  ON l.manifest_sha256=r.capture_sha256 AND l.source_sha256=r.section_sha256 AND l.address=r.target
                WHERE r.capture_sha256=?''',(capture,))
        return {'capture_sha256':capture,'code_artifacts':code_ids,
            'data_references':db.execute('SELECT count(*) FROM snapshot_data_refs WHERE capture_sha256=?',(capture,)).fetchone()[0],
            'literal_start_links':db.execute('SELECT count(*) FROM snapshot_literal_links WHERE capture_sha256=?',(capture,)).fetchone()[0],
            'limits':'Exact-start links to scanned candidates, not proof of string type/execution; interior references and pointer chains are not inferred.'}


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--db',required=True);p.add_argument('--manifest',required=True);p.add_argument('--output',required=True)
    a=p.parse_args();r=link(a.db,a.manifest);Path(a.output).write_text(json.dumps(r,indent=2),encoding='utf-8');print(json.dumps(r))
