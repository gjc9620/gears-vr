"""Archive only this chat's three independently verified closed PSO caches."""
from pathlib import Path
import hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl();sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
archive=Path('J:/gearsVr/analysis/cover-cache-archive-20261111-r175').resolve()
assert archive.is_relative_to(Path('J:/gearsVr/analysis').resolve()) and not archive.exists()
rows=[]
for folder,name in (('source-age-detail-r169','cover-1111-r170-cloak-age'),('owner-link-detail-r171','cover-1111-r172-cloak-links'),('actor-backlink-r173','cover-1111-r174-cloak-backlink')):
    number=name.split('-')[2]
    receipt=load(H.parent/folder/('runtime-'+number)/'closeout.json')
    assert receipt['normal_cleanup_verified'] and receipt['exact_owned_processes_absent'] and not receipt['forced_termination']
    for identity in receipt['identities'].values():assert control.identity(identity['pid'])!=identity
    instance=(R/'analysis/instances'/name).resolve()
    assert instance.is_relative_to((R/'analysis/instances').resolve())
    source=(instance/'LocalAppData/Coalition/WarGame/PSOCache').resolve()
    assert source.is_relative_to(instance) and source.name=='PSOCache'
    for path in source.rglob('*'):
        assert not path.is_symlink()
        if path.is_file():
            assert path.resolve().is_relative_to(source)
            dest=archive/name/path.relative_to(source)
            assert dest.resolve().is_relative_to(archive)
            rows.append(dict(source=str(path),destination=str(dest),size=path.stat().st_size,sha256=sha(path)))
for row in rows:
    dest=Path(row['destination']);dest.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(row['source'],dest)
for row in rows:
    assert sha(Path(row['destination']))==row['sha256']==sha(Path(row['source']))
for folder,name in (('source-age-detail-r169','cover-1111-r170-cloak-age'),('owner-link-detail-r171','cover-1111-r172-cloak-links'),('actor-backlink-r173','cover-1111-r174-cloak-backlink')):
    receipt=load(H.parent/folder/('runtime-'+name.split('-')[2])/'closeout.json')
    for identity in receipt['identities'].values():assert control.identity(identity['pid'])!=identity
for row in rows:
    path=Path(row['source']);assert sha(path)==row['sha256'];path.unlink()
result=dict(files=rows,bytes=sum(x['size'] for x in rows),files_deleted=len(rows),archive=str(archive),
    scope='Only PSOCache files in the three exact closed cover instances; no source/evidence/media/save/config/user profile removed',
    exact_owned_processes_absent=True,archive_hashes_verified=True)
(H/'cache-archive.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(files=len(rows),bytes=result['bytes'],archive=str(archive))))
