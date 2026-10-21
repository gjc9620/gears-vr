"""Incremental, hash-verified local full-text index of cached pseudocode."""
import argparse
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3


def connect(path):
    db = sqlite3.connect(path,timeout=30)
    db.row_factory = sqlite3.Row
    db.execute('PRAGMA journal_mode=WAL')
    db.execute('PRAGMA synchronous=NORMAL')
    db.executescript('''
      CREATE TABLE IF NOT EXISTS pseudocode_documents(
        id INTEGER PRIMARY KEY, artifact_id INTEGER, va TEXT, sha256 TEXT,
        path TEXT, UNIQUE(artifact_id,va));
      CREATE VIRTUAL TABLE IF NOT EXISTS pseudocode_fts USING fts5(body);
    ''')
    return db


def build(db):
    updated = 0
    unchanged = 0
    errors = []
    rows = db.execute("SELECT artifact_id,va,pseudocode_path,pseudocode_sha256 FROM functions WHERE status='success'").fetchall()
    for r in rows:
        old = db.execute('SELECT id,sha256 FROM pseudocode_documents WHERE artifact_id=? AND va=?',
                         (r['artifact_id'],r['va'])).fetchone()
        if old and old['sha256'] == r['pseudocode_sha256']:
            unchanged += 1
            continue
        try:
            data = Path(r['pseudocode_path']).read_bytes()
            if hashlib.sha256(data).hexdigest() != r['pseudocode_sha256']:
                raise ValueError('pseudocode SHA256 mismatch')
            body = data.decode('utf-8-sig')
            with db:
                if old:
                    doc_id = old['id']
                    db.execute('DELETE FROM pseudocode_fts WHERE rowid=?',(doc_id,))
                    db.execute('UPDATE pseudocode_documents SET sha256=?,path=? WHERE id=?',
                               (r['pseudocode_sha256'],r['pseudocode_path'],doc_id))
                else:
                    doc_id = db.execute('INSERT INTO pseudocode_documents(artifact_id,va,sha256,path) VALUES(?,?,?,?)',
                        (r['artifact_id'],r['va'],r['pseudocode_sha256'],r['pseudocode_path'])).lastrowid
                db.execute('INSERT INTO pseudocode_fts(rowid,body) VALUES(?,?)',(doc_id,body))
            updated += 1
        except (OSError,UnicodeError,ValueError) as exc:
            errors.append({'artifact_id':r['artifact_id'],'va':r['va'],'error':str(exc)})
    return {'updated':updated,'unchanged':unchanged,'errors':errors,
            'indexed':db.execute('SELECT count(*) FROM pseudocode_documents').fetchone()[0]}


def search(db,query,limit=10):
    if not 1<=limit<=100:
        raise ValueError('limit must be 1..100')
    return [dict(r) for r in db.execute('''SELECT d.artifact_id,d.va,d.path,d.sha256,
        snippet(pseudocode_fts,0,'[',']',' … ',24) AS excerpt
        FROM pseudocode_fts JOIN pseudocode_documents d ON d.id=pseudocode_fts.rowid
        JOIN functions f ON f.artifact_id=d.artifact_id AND f.va=d.va
        WHERE f.status='success' AND f.pseudocode_sha256=d.sha256
        AND pseudocode_fts MATCH ? ORDER BY rank LIMIT ?''',(query,limit))]


def build_scripts(db):
    db.executescript('''CREATE TABLE IF NOT EXISTS script_documents(
        id INTEGER PRIMARY KEY,artifact_id INTEGER,object_index INTEGER,sha256 TEXT,path TEXT,
        UNIQUE(artifact_id,object_index));
        CREATE VIRTUAL TABLE IF NOT EXISTS script_fts USING fts5(body);''')
    updated=0;unchanged=0;errors=[]
    for r in db.execute('SELECT artifact_id,object_index,file,sha256,metadata FROM script_entries').fetchall():
        if not json.loads(r['metadata']).get('exported'):continue
        old=db.execute('SELECT id,sha256 FROM script_documents WHERE artifact_id=? AND object_index=?',
                       (r['artifact_id'],r['object_index'])).fetchone()
        if old and old['sha256']==r['sha256']:
            unchanged+=1;continue
        try:
            data=Path(r['file']).read_bytes()
            if hashlib.sha256(data).hexdigest()!=r['sha256']:raise ValueError('script SHA256 mismatch')
            body=data.decode('utf-8-sig')
            with db:
                if old:
                    doc_id=old['id'];db.execute('DELETE FROM script_fts WHERE rowid=?',(doc_id,))
                    db.execute('UPDATE script_documents SET sha256=?,path=? WHERE id=?',(r['sha256'],r['file'],doc_id))
                else:
                    doc_id=db.execute('INSERT INTO script_documents(artifact_id,object_index,sha256,path) VALUES(?,?,?,?)',
                        (r['artifact_id'],r['object_index'],r['sha256'],r['file'])).lastrowid
                db.execute('INSERT INTO script_fts(rowid,body) VALUES(?,?)',(doc_id,body))
            updated+=1
        except (OSError,ValueError,UnicodeError) as exc:
            errors.append({'artifact_id':r['artifact_id'],'object_index':r['object_index'],'error':str(exc)})
    return {'updated':updated,'unchanged':unchanged,'errors':errors,
            'indexed':db.execute('SELECT count(*) FROM script_documents').fetchone()[0]}


def search_scripts(db,query,limit=10):
    if not 1<=limit<=100:raise ValueError('limit must be 1..100')
    return [dict(r) for r in db.execute('''SELECT d.artifact_id,d.object_index,s.name,d.path,d.sha256,
        snippet(script_fts,0,'[',']',' … ',24) AS excerpt
        FROM script_fts JOIN script_documents d ON d.id=script_fts.rowid
        JOIN script_entries s ON s.artifact_id=d.artifact_id AND s.object_index=d.object_index
        WHERE json_extract(s.metadata,'$.exported')=1 AND s.sha256=d.sha256
        AND script_fts MATCH ? ORDER BY rank LIMIT ?''',(query,limit))]


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db',required=True)
    parser.add_argument('command',choices=['build','search','build-scripts','search-scripts'])
    parser.add_argument('query',nargs='?')
    parser.add_argument('--limit',type=int,default=10)
    args=parser.parse_args()
    with closing(connect(args.db)) as db:
        if args.command=='build':result=build(db)
        elif args.command=='build-scripts':result=build_scripts(db)
        elif args.command=='search-scripts':result=search_scripts(db,args.query,args.limit)
        else:result=search(db,args.query,args.limit)
    print(json.dumps(result,ensure_ascii=False,indent=2))
