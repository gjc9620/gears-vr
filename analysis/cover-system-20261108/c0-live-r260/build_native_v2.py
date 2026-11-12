from pathlib import Path
import json,hashlib,re,shutil,sys,traceback
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import build_verified
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
baseline=folder.parent/'c0-committed-r259';original=baseline/'native-source'
old=json.loads((baseline/'native-source-manifest.json').read_text())['files']
for p,d in old.items():assert sha(original/p)==d,p
owned=['cover_l2_grab_live.inl','body_c0_goal_policy_live.inl','cover_c0_policy_live.inl','cover_input_live.inl','cover_full_goal_live.inl']
proof=['full_body_c0_native_live.inl','full_body_c0_native_live.h','full_body_c0_native_backend.h','full_body_c0_transaction.h','cover_engine_bone_map.h','cover_physics_bone_map.h']
names=owned+proof
source=folder/'native-source-v2';build=folder/'native-build-v2';assert not source.exists() and not build.exists()
shutil.copytree(original,source)
for name in owned:shutil.copyfile(root/'tools/native-capture'/name,source/'tools/native-capture'/name)
host=folder/'host-source';host_manifest=json.loads((folder/'host-source-final-manifest.json').read_text())['files']
for name in proof:
 rel='tools/native-capture/'+name;assert sha(host/rel)==host_manifest[rel];shutil.copyfile(host/rel,source/rel)
p=source/'tools/native-capture/cover_shared_input_live.inl';text=p.read_text(encoding='utf-8');anchor='            view=result.view.base_view;';assert text.count(anchor)==1
p.write_text(text.replace(anchor,'            if(!input::full_body_cover_enabled)view=result.view.base_view;'),encoding='utf-8')
p=source/'tools/native-capture/capture.cpp';text=p.read_text(encoding='utf-8');anchor='        // Require a fresh, actual head mask for this local actor. Hand';assert text.count(anchor)==1
text=text.replace(anchor,"""        // Keep collecting current sources, but never project an unconsumed cover pose.
        if(cover_input_live::full_body_cover_enabled&&cover_view_owned&&!body_view_owned){
            ++cover_input_live::uncomposed_view_rejected;return false;
        }
"""+anchor);p.write_text(text,encoding='utf-8')

manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if old.get(p)!=d];assert set(changed)==set('tools/native-capture/'+p for p in names+['capture.cpp','cover_shared_input_live.inl'])
flags=dict(re.findall(r'^(GEARS_[A-Z0-9_]+):BOOL=(ON|OFF)$',(baseline/'native-build/CMakeCache.txt').read_text(encoding='utf-8-sig'),re.M))
assert flags.get('GEARS_COVER_BODY_PROTOTYPE')=='ON' and flags.get('GEARS_COVER_PHYSX_QUERIES')=='OFF'
(folder/'native-source-v2-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,overlay=names,baseline='r259',proof_source='tested Host source: f9d54ff6 completed-pose module plus absolute plan_cover selection',flags=flags),indent=2)+'\n')
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
 (folder/'native-build-v2-result.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(dict(dll=receipt['dll_sha256'],dependencies=len(actual),changed=changed)))
except BaseException:
 (folder/'native-build-v2-error.json').write_text(json.dumps(dict(error=traceback.format_exc(),build=str(build),source=str(source),game_started=False),indent=2)+'\n');raise
