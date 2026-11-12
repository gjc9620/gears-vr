from pathlib import Path
import json,hashlib,re,shutil,sys
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=folder.parent/'recorded-readback-r277';original=baseline/'native-source-v2'
old=json.loads((baseline/'native-source-v2-manifest.json').read_text())['files']
for p,d in old.items():assert sha(original/p)==d,p
source=folder/'native-source-v4';build=folder/'native-build-v4';assert not source.exists() and not build.exists()
shutil.copytree(original,source)
names=['cover_input_live.inl','cover_l2_grab_live.inl','body_c0_goal_policy_live.inl']
for name in names:shutil.copyfile(root/'tools/native-capture'/name,source/'tools/native-capture'/name)
new='tools/vr-common/cover_view_heading.h';shutil.copyfile(root/new,source/new)
def replace(p,a,b):
 text=p.read_text(encoding='utf-8');assert text.count(a)==1,a
 p.write_text(text.replace(a,b),encoding='utf-8')
replace(source/'tools/native-capture/capture.cpp','#include "../vr-common/cover_view.h"','#include "../vr-common/cover_view.h"\n#include "../vr-common/cover_view_heading.h"')
p=source/'tools/native-capture/cover_shared_input_live.inl'
replace(p,'        neutral::EyeSample eye{};','        neutral::EyeSample eye{};const auto incoming_view=view;')
replace(p,'frame.view_anchor=frame.anchor; // View remains independent of the C0 wall/body basis.','frame.view_anchor=input::view_anchor(scene,eye,stereo,incoming_view);')
replace(p,'cover_tracking_point(frame.anchor,result.view,stereo.reference,hands.hands[0].grip.position','cover_tracking_point(frame.view_anchor,result.view,stereo.reference,hands.hands[0].grip.position')
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if old.get(p)!=d]
assert set(changed)==set(['tools/native-capture/'+p for p in names]+[new,'tools/native-capture/capture.cpp','tools/native-capture/cover_shared_input_live.inl'])
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build-v2/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M));assert flags['GEARS_COVER_PHYSX_QUERIES']=='OFF'
(folder/'native-source-v4-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,flags=flags,baseline='r277-v2'),indent=2)+'\n')
receipt=build_verified(source/'tools/native-capture',build,Path('E:/CMake/bin/cmake.exe'),Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'),Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),[k+'='+v for k,v in flags.items()],['GearsCapture'],'CMakeFiles/GearsCapture.dir/capture.cpp.obj',names)
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
(folder/'native-build-v4-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
