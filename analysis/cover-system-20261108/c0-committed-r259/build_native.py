from pathlib import Path
import json,hashlib,re,shutil,sys,traceback
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=folder.parent/'l2-live-r258';original=baseline/'native-source-v2'
old=json.loads((baseline/'native-source-v2-manifest.json').read_text())['files']
for p,d in old.items():assert sha(original/p)==d,p
native_names=['cover_committed_drag.h','cover_c0_committed_adapter.h','cover_neutral_goal.h','cover_posture_fit.h','cover_c0_target_adapter.h','full_body_c0_planner.h','body_goal_view_adapter.h','cover_full_goal_live.inl','cover_input_live.inl']
vr_names=['cover_input_owner.h','cover_motion.h','cover_interaction.h','cover_view.h','cover_body_target.h']
names=native_names+vr_names
source=folder/'native-source';build=folder/'native-build';assert not source.exists() and not build.exists()
shutil.copytree(original,source)
for name in native_names:shutil.copyfile(root/'tools/native-capture'/name,source/'tools/native-capture'/name)
for name in vr_names:shutil.copyfile(root/'tools/vr-common'/name,source/'tools/vr-common'/name)
p=source/'tools/native-capture/capture.cpp';text=p.read_text(encoding='utf-8');anchor='#include "body_c0_neutral.h"';assert text.count(anchor)==1
p.write_text(text.replace(anchor,anchor+'\n#include "cover_c0_target_adapter.h"\n#include "cover_c0_committed_adapter.h"'),encoding='utf-8')
p=source/'tools/native-capture/cover_shared_input_live.inl';text=p.read_text(encoding='utf-8');anchor='            frame.height_curve=input::height_curve.data();';assert text.count(anchor)==1
p.write_text(text.replace(anchor,'            frame.view_anchor=frame.anchor;\n'+anchor),encoding='utf-8')

manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if old.get(p)!=d];assert set(changed)<=set('tools/native-capture/'+p for p in native_names+['capture.cpp','cover_shared_input_live.inl'])|set('tools/vr-common/'+p for p in vr_names)
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build-v2/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M))
assert flags.get('GEARS_COVER_BODY_PROTOTYPE')=='ON' and flags.get('GEARS_COVER_PHYSX_QUERIES')=='OFF'
(folder/'native-source-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,overlay=names,baseline='r258-v2',collision_disabled=True,flags=flags),indent=2)+'\n')
try:
 receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),
  Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],
  'CMakeFiles/GearsCapture.dir/capture.cpp.obj',names)
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
 (folder/'native-build-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
except BaseException:
 (folder/'native-build-error.json').write_text(json.dumps(dict(error=traceback.format_exc(),build=str(build),source=str(source),game_started=False),indent=2)+'\n');raise
