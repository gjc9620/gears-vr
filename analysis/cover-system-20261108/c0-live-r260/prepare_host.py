"""Reproduce the bounded C0 Host snapshot; no game or tests are launched."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--output',default='host-source-repro');args=parser.parse_args()
if not args.output.replace('-','').isalnum():raise ValueError('Local output folder name required')
out=folder/args.output
if out.exists():raise FileExistsError('Keep existing snapshots; choose a new output name')
base=folder.parent/'c0-committed-r259/native-source'
expected=json.loads((folder.parent/'c0-committed-r259/native-source-manifest.json').read_text())['files']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for rel,digest in expected.items():assert sha(base/rel)==digest,rel
shutil.copytree(base,out)
owned=['cover_c0_policy_live.inl','body_c0_goal_policy_live.inl','host_native_goal_seams.inl','body_c0_goal_policy_live_test.cpp','cover_c0_policy_live_test.cpp']
final=json.loads((folder/'host-source-final-manifest.json').read_text())['files']
for name in owned:
 rel='tools/native-capture/'+name;assert sha(root/rel)==final[rel],rel;shutil.copyfile(root/rel,out/rel)
head=['full_body_c0_native_live.inl','full_body_c0_native_live.h','full_body_c0_native_backend.h','full_body_c0_transaction.h','full_body_c0_native_backend_test.cpp','cover_engine_bone_map.h','cover_physics_bone_map.h','host_native_live_prefix.inl']
for name in head:
 rel='tools/native-capture/'+name;data=subprocess.check_output(['git','show','f9d54ff6:'+rel]);(out/rel).write_bytes(data)
p=out/'tools/native-capture/full_body_c0_native_live.inl';s=p.read_text();old='        auto report=independent_standing?';assert s.count(old)==1
p.write_text(s.replace(old,'        auto report=goal.scope==body::Scope::fixed_cover&&goal.target_mode==body::BodyTargetMode::world_absolute?\n            body::plan_cover(state.model,state.source,goal,state.workspace,state.output):independent_standing?'),encoding='utf-8')
p=out/'tools/native-capture/host_native_live_prefix.inl';s=p.read_text()
s=s.replace('#include "weapon_hand_broker.h"','#include "weapon_hand_broker.h"\n#include "cover_query_source_cause.h"')
s=s.replace('#include "../vr-common/hand_part_grasp.h"\n','');p.write_text(s,encoding='utf-8')
for name in owned+head:
 rel='tools/native-capture/'+name;assert sha(out/rel)==final[rel],rel
receipt={'source':str(out),'owned_files':owned,'committed_proof_files':head,'no_game':True,'no_tests_launched':True,'files':{p.relative_to(out).as_posix():sha(p) for p in out.rglob('*') if p.is_file()}}
(folder/(args.output+'-manifest.json')).write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print('Reproduced bounded Host source; compiled-file hashes matched.')
