"""Index literal candidates from hash-verified historical snapshot sections.

No executable analysis or target execution. Candidates are not IDA-defined
strings and do not establish references, types, ownership or runtime use.
"""
import argparse
from contextlib import closing
import hashlib
import json
from pathlib import Path
import re
import sqlite3


def index(db_path, manifest_path):
    manifest_path = Path(manifest_path).resolve()
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    manifest_hash = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
    # Validate all sources before accepting any part of this capture.
    sources = []
    for section in manifest['sections']:
        source = manifest_path.parent / section['file']
        data = source.read_bytes()
        if len(data) != section['size'] or hashlib.sha256(data).hexdigest() != section['sha256']:
            raise ValueError('snapshot integrity mismatch: '+str(source))
        sources.append((section,source,data))
    count = 0
    with closing(sqlite3.connect(db_path)) as db, db:
        db.execute('''CREATE TABLE IF NOT EXISTS snapshot_literals(
          manifest_sha256 TEXT, source_sha256 TEXT, address TEXT, rva TEXT,
          encoding TEXT, value TEXT, source_path TEXT, metadata TEXT,
          PRIMARY KEY(manifest_sha256,source_sha256,address,encoding))''')
        for section,source,data in sources:
            origin = int(section['address'],0)
            metadata = json.dumps({'authority':'historical snapshot literal candidate',
                'pid':manifest['pid'],'capture':manifest['captured_utc'],
                'manifest_path':str(manifest_path),'executable_sha256':manifest['executable_sha256'],
                'limitations':'Printable ASCII and UTF-16LE ASCII subset, length >=4; may include code/data false positives; no execution/xref claim'})
            db.execute('DELETE FROM snapshot_literals WHERE manifest_sha256=? AND source_sha256=?',
                       (manifest_hash,section['sha256']))
            for encoding,pattern in [('ascii',rb'[\x20-\x7e]{4,}'),('utf-16-le',rb'(?:[\x20-\x7e]\x00){4,}')]:
                for match in re.finditer(pattern,data):
                    address = origin+match.start()
                    db.execute('INSERT INTO snapshot_literals VALUES(?,?,?,?,?,?,?,?)',
                        (manifest_hash,section['sha256'],hex(address),hex(address-int(manifest['base'],0)),
                         encoding,match.group().decode(encoding),str(source),metadata))
                    count += 1
        return {'literal_candidates':count,'sections_verified':len(sources),'manifest_sha256':manifest_hash}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db',required=True)
    parser.add_argument('--manifest',required=True)
    args = parser.parse_args()
    print(json.dumps(index(args.db,args.manifest),indent=2))
