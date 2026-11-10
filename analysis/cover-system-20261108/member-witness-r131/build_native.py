"""Dedicated bounded cover member witnesses on preserved r126 Native; build only."""
from pathlib import Path
import hashlib,json,re,shutil,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=R/'analysis/cover-system-20261108/pose-probe-r126'
data=json.loads((baseline/'native-source-manifest.json').read_text())
original=baseline/'native-source'
for p,d in data['files'].items():assert sha(original/p)==d
source=H/'native-source';build=H/'native-build'
assert not source.exists() and not build.exists()
shutil.copytree(original,source)
overlays=['cover_member_witness_live.inl','cover_physics_roles_live.inl','cover_collider_roles_live.inl','cover_component_roles_live.inl','cover_physics_c0_live.inl']
for name in overlays:shutil.copyfile(R/'tools/native-capture'/name,source/'tools/native-capture'/name)
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if data['files'].get(p)!=d]
assert set(changed)=={'tools/native-capture/'+p for p in overlays},changed
(H/'native-source-manifest.json').write_text(json.dumps({'files':manifest,'changed':changed},indent=2)+'\n')
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M))
receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],'CMakeFiles/GearsCapture.dir/capture.cpp.obj',overlays)
actual={};outside=[]
for raw in (build/'dependencies.log').read_text().splitlines()[1:]:
    if not raw.strip():continue
    p=Path(raw.strip());p=p.resolve() if p.is_absolute() else (build/p).resolve()
    if p.is_relative_to(source):
        rel=p.relative_to(source).as_posix();assert sha(p)==manifest[rel];actual[rel]=manifest[rel]
    elif p.is_relative_to(R):outside.append(str(p))
assert not outside,outside
after=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(build/'CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M));assert after==flags
for p,d in manifest.items():assert sha(source/p)==d
receipt.update(dll_sha256=sha(build/'GearsCapture.dll'),project_dependencies=actual,external_worktree_dependencies=outside,changed=changed,flags_unchanged=True,build_only=True,staged=False,game_started=False,query_called=False)
(H/'native-build-result.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'dll':receipt['dll_sha256'],'project_dependencies':len(actual),'changed':changed}))
