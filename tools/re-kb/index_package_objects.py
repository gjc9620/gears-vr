"""Index complete object inventories and verify every selected script export."""
import csv
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3
from export_scripts import ROOT,sha


def main():
    problems=[]; total_objects=0; checked=0; missing=[]
    with closing(sqlite3.connect(ROOT/'analysis/re-kb/knowledge.sqlite')) as db:
        db.execute('''CREATE TABLE IF NOT EXISTS package_objects(
            artifact_id INTEGER,object_index INTEGER,class TEXT,path TEXT,
            inventory_path TEXT,PRIMARY KEY(artifact_id,object_index))''')
        for name in ('scripts','other-packages-v2','classes-v2'):
            manifest=ROOT/f'analysis/re-kb/{name}/manifest.json'
            for package in json.loads(manifest.read_text(encoding='utf-8'))['packages']:
                if sha(Path(package['source']))!=package['source_sha256']:
                    raise ValueError('source package hash mismatch')
                aid=db.execute("SELECT id FROM artifacts WHERE sha256=? AND kind='unrealscript_package'",
                               (package['source_sha256'],)).fetchone()[0]
                inventory=Path(package['errors_file']).parent/'objects.tsv'
                with db, inventory.open(encoding='utf-8-sig',newline='') as stream:
                    for row in csv.DictReader(stream,delimiter='\t'):
                        db.execute('INSERT OR REPLACE INTO package_objects VALUES(?,?,?,?,?)',
                            (aid,int(row['Index']),row['Class'],row['Path'],str(inventory)))
        # Verify the current rows, including recovery overlays, not stale failures.
        for aid,name,file,digest,metadata in db.execute('SELECT artifact_id,name,file,sha256,metadata FROM script_entries'):
            e=json.loads(metadata)
            if not e['exported']:
                missing.append({'artifact_id':aid,'path':name})
                continue
            p=Path(file)
            if not p.is_file() or sha(p)!=digest:problems.append(name)
            checked+=1
        total_objects=db.execute('SELECT count(*) FROM package_objects').fetchone()[0]
    report={'object_inventory_rows':total_objects,'exported_files_checked':checked,
            'hash_errors':problems,'unavailable_class_exports':missing,
            'limitations':'Object names and parser classes are static package metadata, not runtime layout validation.'}
    (ROOT/'analysis/re-kb/script-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({**report,'unavailable_class_exports':len(missing)}))


if __name__=='__main__':main()
