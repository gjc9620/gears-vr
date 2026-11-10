"""Same-call source rejection observation on exact r146; build only."""
from pathlib import Path
import hashlib,json,re,shutil,sys
from instrument import ready,query
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=H.parent/'current-measurement-r146';data=json.loads((baseline/'native-source-manifest.json').read_text())
original=baseline/'native-source'
for p,d in data['files'].items():assert sha(original/p)==d
source=H/'native-source';build=H/'native-build'
assert not source.exists() and not build.exists()
shutil.copytree(original,source)
for name,fn in (('ready_wrist_entry_live.inl',ready),('cover_physics_c0_live.inl',query)):
    path=source/'tools/native-capture'/name;path.write_text(fn(path.read_text(encoding='utf-8-sig')),encoding='utf-8')
shutil.copyfile(R/'tools/native-capture/cover_query_source_cause.h',source/'tools/native-capture/cover_query_source_cause.h')
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if data['files'].get(p)!=d]
assert set(changed)=={'tools/native-capture/ready_wrist_entry_live.inl','tools/native-capture/cover_physics_c0_live.inl','tools/native-capture/cover_query_source_cause.h'}
(H/'native-source-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed),indent=2)+'\n')
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M))
receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),
    Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],
    'CMakeFiles/GearsCapture.dir/capture.cpp.obj',['ready_wrist_entry_live.inl','cover_physics_c0_live.inl','cover_query_source_cause.h'])
actual={};outside=[]
for raw in (build/'dependencies.log').read_text().splitlines()[1:]:
    if not raw.strip():continue
    p=Path(raw.strip());p=p.resolve() if p.is_absolute() else (build/p).resolve()
    if p.is_relative_to(source):
        rel=p.relative_to(source).as_posix();assert sha(p)==manifest[rel];actual[rel]=manifest[rel]
    elif p.is_relative_to(R):outside.append(str(p))
assert not outside
after=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(build/'CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M));assert after==flags
for p,d in manifest.items():assert sha(source/p)==d
receipt.update(dll_sha256=sha(build/'GearsCapture.dll'),project_dependencies=actual,external_worktree_dependencies=outside,
    changed=changed,flags_unchanged=True,build_only=True,staged=False,game_started=False,query_called=False)
(H/'native-build-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
