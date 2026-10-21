"""Offline corpus audit: integrity, cache hashes and explicit incomplete scope."""
import argparse
from collections import Counter
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3
import time


def verify(database):
    with closing(sqlite3.connect(database,timeout=30)) as db:
        db.row_factory = sqlite3.Row
        db.execute('BEGIN')  # Consistent WAL snapshot even while the exporter commits.
        missing = []
        checked = 0
        for row in db.execute("SELECT artifact_id,va,pseudocode_path,pseudocode_sha256 FROM functions WHERE status='success'"):
            path = Path(row['pseudocode_path'])
            try:
                with path.open('rb') as stream:
                    actual = hashlib.file_digest(stream,'sha256').hexdigest()
                valid = actual == row['pseudocode_sha256']
            except OSError:
                valid = False
            if not valid:
                missing.append({'artifact_id':row['artifact_id'],'va':row['va'],'path':str(path)})
            checked += 1
        statuses = dict(db.execute('SELECT status,count(*) FROM functions GROUP BY status'))
        assembly_checked=0
        assembly_errors=[]
        for aid,va,metadata in db.execute("SELECT artifact_id,va,metadata FROM functions WHERE json_extract(metadata,'$.assembly_path') IS NOT NULL"):
            m=json.loads(metadata)
            if m.get('assembly_path'):
                p=Path(m['assembly_path'])
                if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=m.get('assembly_sha256'):
                    assembly_errors.append({'artifact_id':aid,'va':va})
                assembly_checked+=1
        indexing = dict(db.execute('SELECT indexed,count(*) FROM corpus_work GROUP BY indexed'))
        errors = Counter(row[0] for row in db.execute("SELECT error FROM functions WHERE status='failed'"))
        return {'verified_unix':time.time(),'integrity':[r[0] for r in db.execute('PRAGMA quick_check')],
            'foreign_key_violations':[tuple(r) for r in db.execute('PRAGMA foreign_key_check')],
            'pseudocode_files_checked':checked,'cache_errors':missing,'statuses':statuses,
            'assembly_files_checked':assembly_checked,'assembly_errors':assembly_errors,
            'reference_index':indexing,'failed_decompilations':dict(errors),
            'pending_functions':statuses.get('skipped',0),
            'scope':'IDA-defined functions from historical single code section; not whole-game completion',
            'token_usage':None}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db',required=True)
    parser.add_argument('--output',required=True)
    args = parser.parse_args()
    report = verify(args.db)
    Path(args.output).write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='failed_decompilations'}))
