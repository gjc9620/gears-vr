"""Standard-library, artifact-isolated reverse engineering evidence index."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sqlite3


def dumps(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True)


def address(value):
    return hex(int(value, 0) if isinstance(value, str) else int(value))


def digest(value):
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", value):
        raise ValueError("SHA-256 must contain exactly 64 hexadecimal characters")
    return value.lower()


def connect(path):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(path)
    db.row_factory = sqlite3.Row
    db.execute("PRAGMA foreign_keys=ON")
    db.executescript('''
    CREATE TABLE IF NOT EXISTS artifacts(
      id INTEGER PRIMARY KEY, sha256 TEXT NOT NULL, kind TEXT NOT NULL,
      module_base TEXT NOT NULL, path TEXT, version TEXT, total_functions INTEGER,
      metadata TEXT NOT NULL, UNIQUE(sha256,kind,module_base));
    CREATE TABLE IF NOT EXISTS functions(
      artifact_id INTEGER REFERENCES artifacts(id), va TEXT, rva TEXT, name TEXT,
      bytes_sha256 TEXT, status TEXT, error TEXT, pseudocode_path TEXT,
      pseudocode_sha256 TEXT, metadata TEXT, PRIMARY KEY(artifact_id,va));
    CREATE TABLE IF NOT EXISTS edges(
      artifact_id INTEGER, function_va TEXT, category TEXT, source TEXT,
      target TEXT, edge_type TEXT, evidence TEXT,
      FOREIGN KEY(artifact_id,function_va) REFERENCES functions(artifact_id,va));
    CREATE INDEX IF NOT EXISTS edges_target ON edges(artifact_id,target);
    CREATE INDEX IF NOT EXISTS edges_owner ON edges(artifact_id,function_va);
    CREATE TABLE IF NOT EXISTS strings(
      artifact_id INTEGER, function_va TEXT, address TEXT, value TEXT,
      FOREIGN KEY(artifact_id,function_va) REFERENCES functions(artifact_id,va));
    CREATE TABLE IF NOT EXISTS script_entries(
      artifact_id INTEGER REFERENCES artifacts(id), object_index INTEGER,
      name TEXT, file TEXT, sha256 TEXT, native_declaration INTEGER,
      metadata TEXT, PRIMARY KEY(artifact_id,object_index));
    CREATE TABLE IF NOT EXISTS findings(
      id TEXT PRIMARY KEY, artifact_id INTEGER REFERENCES artifacts(id),
      function_va TEXT, claim TEXT NOT NULL, confidence TEXT NOT NULL,
      evidence TEXT NOT NULL, validation TEXT NOT NULL,
      ai_tokens INTEGER, metadata TEXT NOT NULL);
    ''')
    return db


def artifact(db, row):
    sha = digest(row['sha256'])
    kind = row['kind']
    if kind not in ('disk_exe', 'runtime_snapshot', 'unrealscript_package'):
        raise ValueError('unsupported artifact kind: ' + kind)
    base = address(row.get('module_base', 0))
    total = row.get('total_functions')
    if total is not None and (not isinstance(total, int) or total < 0):
        raise ValueError('invalid total_functions')
    db.execute('''INSERT INTO artifacts(sha256,kind,module_base,path,version,total_functions,metadata)
      VALUES(?,?,?,?,?,?,?) ON CONFLICT(sha256,kind,module_base) DO UPDATE SET
      path=excluded.path,version=excluded.version,
      total_functions=COALESCE(excluded.total_functions,artifacts.total_functions),metadata=excluded.metadata''',
      (sha,kind,base,row.get('path'),row.get('version'),total,dumps(row.get('metadata',{}))))
    return db.execute('SELECT id FROM artifacts WHERE sha256=? AND kind=? AND module_base=?',
                      (sha,kind,base)).fetchone()[0]


def import_function(db, aid, row, folder):
    a = db.execute('SELECT * FROM artifacts WHERE id=?',(aid,)).fetchone()
    if a['kind'] == 'unrealscript_package':
        raise ValueError('script declarations cannot be imported as native functions')
    va = address(row['va'])
    rva = address(int(va,16)-int(a['module_base'],16))
    if int(rva,16) < 0 or ('rva' in row and address(row['rva']) != rva):
        raise ValueError('VA/RVA/module base mismatch')
    status = row['status']
    if status not in ('success','failed','skipped'):
        raise ValueError('unsupported function status')
    if status == 'failed' and not row.get('error'):
        raise ValueError('failed function requires an error')
    p = row.get('pseudocode_path')
    sha = row.get('pseudocode_sha256')
    if p:
        p = Path(p)
        p = (folder / p).resolve() if not p.is_absolute() else p.resolve()
        actual = hashlib.sha256(p.read_bytes()).hexdigest()
        if sha and digest(sha) != actual:
            raise ValueError('pseudocode SHA-256 mismatch')
        sha = actual
    if status == 'success' and not p:
        raise ValueError('successful decompilation requires a pseudocode file')
    bh = row.get('bytes_sha256')
    if bh:
        bh = digest(bh)
    db.execute('''INSERT INTO functions VALUES(?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(artifact_id,va) DO UPDATE SET rva=excluded.rva,name=excluded.name,
      bytes_sha256=excluded.bytes_sha256,status=excluded.status,error=excluded.error,
      pseudocode_path=excluded.pseudocode_path,pseudocode_sha256=excluded.pseudocode_sha256,
      metadata=excluded.metadata''',
      (aid,va,rva,row['name'],bh,status,row.get('error'),str(p) if p else None,sha,dumps(row.get('metadata',{}))))
    db.execute('DELETE FROM edges WHERE artifact_id=? AND function_va=?',(aid,va))
    db.execute('DELETE FROM strings WHERE artifact_id=? AND function_va=?',(aid,va))
    for category in ('xrefs','calls'):
        seen = set()
        for e in row.get(category,[]):
            key = dumps(e)
            if key in seen:
                continue
            seen.add(key)
            db.execute('INSERT INTO edges VALUES(?,?,?,?,?,?,?)',
                       (aid,va,category,address(e['from']),address(e['to']),str(e.get('type','')),key))
    for s in {dumps(s) for s in row.get('strings',[])}:
        value = json.loads(s)
        db.execute('INSERT INTO strings VALUES(?,?,?,?)',
                   (aid,va,address(value['address']),value['value']))


def import_jsonl(db, path, allow_truncated=False):
    path = Path(path).resolve()
    aid = None
    count = 0
    with path.open(encoding='utf-8-sig') as stream:
        for line_no, line in enumerate(stream,1):
            if not line.strip():
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                if allow_truncated and not line.endswith('\n'):
                    return {'artifact_id':aid,'functions_imported':count,'truncated_final_line':line_no}
                raise ValueError(f'invalid JSON at {path}:{line_no}')
            with db:
                if row['type'] == 'artifact':
                    if aid is not None:
                        raise ValueError('only one artifact header is allowed per export')
                    aid = artifact(db,row)
                elif row['type'] == 'function' and aid is not None:
                    import_function(db,aid,row,path.parent)
                    count += 1
                else:
                    raise ValueError(f'unsupported record/header missing at line {line_no}')
    if aid is None:
        raise ValueError('missing artifact header')
    return {'artifact_id':aid,'functions_imported':count}


def import_manifest(db,path):
    path = Path(path).resolve()
    manifest = json.loads(path.read_text(encoding='utf-8-sig'))
    count = 0
    with db:
        for package in manifest['packages']:
            aid = artifact(db,{'sha256':package['source_sha256'],'kind':'unrealscript_package',
                'path':package['source'],'metadata':{'manifest':str(path),'name':package['name'],
                'authority':manifest.get('authority'),'limitations':manifest.get('limitations',[])}})
            for e in package['entries']:
                db.execute('''INSERT INTO script_entries VALUES(?,?,?,?,?,?,?)
                  ON CONFLICT(artifact_id,object_index) DO UPDATE SET name=excluded.name,
                  file=excluded.file,sha256=excluded.sha256,native_declaration=excluded.native_declaration,
                  metadata=excluded.metadata''',
                  (aid,e['index'],e['path'],e['file'],e.get('sha256'),int(e.get('native_declaration',False)),dumps(e)))
                count += 1
    return {'script_entries_imported':count}


def add_finding(db,row):
    if row['confidence'] not in ('low','medium','high'):
        raise ValueError('confidence must be low, medium, or high')
    if not row.get('evidence') or not row.get('validation'):
        raise ValueError('finding requires original evidence and validation status/limitations')
    tokens = row.get('ai_tokens')
    if tokens is not None and (type(tokens) is not int or tokens < 0):
        raise ValueError('ai_tokens must be measured nonnegative integer or null (unknown)')
    aid = row.get('artifact_id')
    if aid is not None and not db.execute('SELECT id FROM artifacts WHERE id=?',(aid,)).fetchone():
        raise ValueError('unknown artifact')
    va = address(row['function_va']) if row.get('function_va') is not None else None
    if va and not db.execute('SELECT va FROM functions WHERE artifact_id=? AND va=?',(aid,va)).fetchone():
        raise ValueError('function does not belong to artifact')
    fid = row.get('id') or hashlib.sha256(dumps(row).encode()).hexdigest()
    with db:
        db.execute('INSERT OR REPLACE INTO findings VALUES(?,?,?,?,?,?,?,?,?)',
          (fid,aid,va,row['claim'],row['confidence'],dumps(row['evidence']),dumps(row['validation']),tokens,dumps(row.get('metadata',{}))))
    return {'finding_id':fid}


def stats(db):
    results = []
    for a in db.execute('SELECT * FROM artifacts ORDER BY id'):
        aid = a['id']
        counts = dict(db.execute('SELECT status,count(*) FROM functions WHERE artifact_id=? GROUP BY status',(aid,)))
        n = sum(counts.values())
        attempted = counts.get('success',0)+counts.get('failed',0)
        results.append({'artifact_id':aid,'sha256':a['sha256'],'kind':a['kind'],'module_base':a['module_base'],
          'native_functions_indexed':n,'total_functions':a['total_functions'],
          'function_coverage':n/a['total_functions'] if a['total_functions'] else None,
          'pseudocode_coverage':counts.get('success',0)/a['total_functions'] if a['total_functions'] else None,
          'status_counts':counts,'decompilation_success_rate':counts.get('success',0)/attempted if attempted else None,
          'script_entries':db.execute('SELECT count(*) FROM script_entries WHERE artifact_id=?',(aid,)).fetchone()[0],
          'native_declarations':db.execute('SELECT count(*) FROM script_entries WHERE artifact_id=? AND native_declaration=1',(aid,)).fetchone()[0],
          'ai_deeply_analyzed_functions':db.execute('SELECT count(DISTINCT function_va) FROM findings WHERE artifact_id=? AND function_va IS NOT NULL',(aid,)).fetchone()[0]})
    tokens = db.execute('SELECT count(*),count(ai_tokens),sum(ai_tokens) FROM findings').fetchone()
    corpus = None
    if db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='corpus_work'").fetchone():
        corpus = [dict(r) for r in db.execute('SELECT artifact_id,indexed,count(*) AS functions FROM corpus_work GROUP BY artifact_id,indexed')]
    literals = None
    if db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='snapshot_literals'").fetchone():
        literals = db.execute('SELECT count(*) FROM snapshot_literals').fetchone()[0]
    return {'artifacts':results,'reference_index_progress':corpus,'snapshot_literal_candidates':literals,
            'findings':tokens[0], 'ai_tokens_known_sum':tokens[2],
            'findings_with_unknown_tokens':tokens[0]-tokens[1], 'session_total_ai_tokens':None,
            'token_note':'Only externally measured finding token values are recorded; session usage is unknown.'}


def search(db,query,limit=30,artifact_id=None):
    if limit < 1 or limit > 1000:
        raise ValueError('limit must be 1..1000')
    term = '%' + query.replace('\\','\\\\').replace('%','\\%').replace('_','\\_') + '%'
    result = {}
    for table, fields in [('functions',['name','va','rva']),('script_entries',['name']),
                           ('strings',['value']),('findings',['claim','evidence'])]:
        where = '('+' OR '.join(f"{f} LIKE ? ESCAPE '\\'" for f in fields)+')'
        params = [term]*len(fields)
        if artifact_id is not None:
            where += ' AND artifact_id=?'
            params.append(artifact_id)
        result[table] = [dict(r) for r in db.execute(f'SELECT * FROM {table} WHERE {where} LIMIT ?',params+[limit])]
    # Snapshot literals have their own section/manifest identity, not function ownership.
    if artifact_id is None and db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='snapshot_literals'").fetchone():
        result['snapshot_literals'] = [dict(r) for r in db.execute(
            "SELECT * FROM snapshot_literals WHERE value LIKE ? ESCAPE '\\' LIMIT ?",(term,limit))]
    if db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='package_objects'").fetchone():
        sql="SELECT * FROM package_objects WHERE path LIKE ? ESCAPE '\\'"
        params=[term]
        if artifact_id is not None:
            sql+=' AND artifact_id=?'; params.append(artifact_id)
        result['package_objects']=[dict(r) for r in db.execute(sql+' LIMIT ?',params+[limit])]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db',required=True)
    sub = parser.add_subparsers(dest='command',required=True)
    p = sub.add_parser('import-jsonl'); p.add_argument('path'); p.add_argument('--allow-truncated',action='store_true')
    p = sub.add_parser('import-manifest'); p.add_argument('path')
    p = sub.add_parser('add-finding'); p.add_argument('path')
    p = sub.add_parser('search'); p.add_argument('query'); p.add_argument('--limit',type=int,default=30); p.add_argument('--artifact-id',type=int)
    p = sub.add_parser('refs'); p.add_argument('address'); p.add_argument('--artifact-id',type=int,required=True); p.add_argument('--limit',type=int,default=30)
    sub.add_parser('stats')
    args = parser.parse_args()
    with connect(args.db) as db:
        if args.command == 'import-jsonl': result = import_jsonl(db,args.path,args.allow_truncated)
        elif args.command == 'import-manifest': result = import_manifest(db,args.path)
        elif args.command == 'add-finding': result = add_finding(db,json.loads(Path(args.path).read_text(encoding='utf-8-sig')))
        elif args.command == 'search': result = search(db,args.query,args.limit,args.artifact_id)
        elif args.command == 'refs':
            if not 1 <= args.limit <= 1000:
                raise ValueError('limit must be 1..1000')
            va = address(args.address)
            result = {key:[dict(r) for r in db.execute(
                f'SELECT * FROM edges WHERE artifact_id=? AND {column}=? LIMIT ?',
                (args.artifact_id,va,args.limit))] for key,column in [('incoming','target'),('function_records','function_va')]}
            if db.execute("SELECT 1 FROM sqlite_master WHERE name='snapshot_literal_links'").fetchone():
                result['capture_literal_links']=[dict(r) for r in db.execute('''SELECT x.*,substr(l.value,1,240) AS literal_candidate
                    FROM snapshot_literal_links x JOIN snapshot_literals l ON l.manifest_sha256=x.capture_sha256
                    AND l.source_sha256=x.section_sha256 AND l.address=x.target AND l.encoding=x.encoding
                    WHERE x.artifact_id=? AND (x.target=? OR x.function_va=?) LIMIT ?''',
                    (args.artifact_id,va,va,args.limit))]
        else: result = stats(db)
    print(json.dumps(result,ensure_ascii=False,indent=2))


if __name__ == '__main__':
    main()
