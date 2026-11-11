from pathlib import Path
import json,hashlib,re,shutil,sys,traceback
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=folder.parent/'shape-pose-detail-r233';original=baseline/'native-source-v2'
old=json.loads((baseline/'native-source-v2-manifest.json').read_text())['files']
for p,d in old.items():assert sha(original/p)==d,p
names=['cover_physx_shape_world.h','cover_physx_observed_pose.h','cover_physics_c0_live.inl']
source=folder/'native-source-v2';build=folder/'native-build-v2';assert not source.exists() and not build.exists()
shutil.copytree(original,source)
for name in names:shutil.copyfile(root/'tools/native-capture'/name,source/'tools/native-capture'/name)
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if old.get(p)!=d];assert set(changed)<=set('tools/native-capture/'+p for p in names)
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build-v2/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M))
assert flags.get('GEARS_COVER_BODY_PROTOTYPE')=='ON' and flags.get('GEARS_COVER_PHYSX_QUERIES')=='ON'
(folder/'native-source-v2-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,overlay=names,baseline='r233',flags=flags),indent=2)+'\n')
try:
 receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),
  Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],
  'CMakeFiles/GearsCapture.dir/capture.cpp.obj',['cover_physx_shape_world.h','cover_physx_observed_pose.h'])
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
 receipt.update(dll_sha256=sha(build/'GearsCapture.dll'),project_dependencies=actual,external_worktree_dependencies=outside,
  changed=changed,flags_unchanged=True,build_only=True,staged=False,game_started=False,query_called=False)
 (folder/'native-build-v2-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
except BaseException:
 (folder/'native-build-v2-error.json').write_text(json.dumps(dict(error=traceback.format_exc(),build=str(build),source=str(source),game_started=False),indent=2)+'\n');raise

