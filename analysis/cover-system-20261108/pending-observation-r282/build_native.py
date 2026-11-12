from pathlib import Path
import json,hashlib,re,shutil,sys
from overlay import ROOT as root,REL,apply
folder=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=folder.parent/'continuous-heading-r279';original=baseline/'native-source-v4'
old=json.loads((baseline/'native-source-v4-manifest.json').read_text())['files']
for p,d in old.items():assert sha(original/p)==d,p
source=folder/'native-source-v2';build=folder/'native-build-v2';assert not source.exists() and not build.exists()
shutil.copytree(original,source);p=source/REL;p.write_text(apply(p.read_text(encoding='utf-8')),encoding='utf-8')
new='tools/native-capture/cover_pending_observation.inl';shutil.copyfile(root/new,source/new)
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()};changed=[p for p,d in manifest.items() if old.get(p)!=d];assert set(changed)=={REL,new},changed
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build-v4/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M));assert flags['GEARS_COVER_PHYSX_QUERIES']=='OFF'
(folder/'native-source-v2-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,flags=flags,baseline='r279-v4; observer-only overlay; no recovery or write-policy changes'),indent=2)+'\n')
receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],'CMakeFiles/GearsCapture.dir/capture.cpp.obj',['full_body_c0_native_live.inl','cover_pending_observation.inl'])
actual={};outside=[]
for raw in (build/'dependencies.log').read_text().splitlines()[1:]:
 if not raw.strip():continue
 p=Path(raw.strip());p=p.resolve() if p.is_absolute() else (build/p).resolve()
 if p.is_relative_to(source):
  rel=p.relative_to(source).as_posix();assert sha(p)==manifest[rel];actual[rel]=manifest[rel]
 elif p.is_relative_to(root):outside.append(str(p))
assert not outside,outside
after=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(build/'CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M));assert after==flags
for p,d in manifest.items():assert sha(source/p)==d,p
receipt.update(dll_sha256=sha(build/'GearsCapture.dll'),project_dependencies=actual,external_worktree_dependencies=outside,changed=changed,flags_unchanged=True,build_only=True,staged=False,game_started=False,query_called=False)
(folder/'native-build-v2-result.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
