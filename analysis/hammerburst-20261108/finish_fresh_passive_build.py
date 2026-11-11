from pathlib import Path
import json,hashlib,subprocess,difflib,re
import prepare_fresh_passive_socket as p
patch=''
for target,base,relations in (
    (p.PG,p.PGBASE,['tools/vr-common/body_main_mount_provider.h','tools/vr-common/hand_object_grasp.h','tools/vr-common/fresh_support_socket.h','tools/playground/weapon_native_lancer_follow.inl','tools/playground/body_main_mount_sender.h','tools/playground/body_main_mount_provider.inl','tools/playground/body_main_mount_sender_test.cpp']),
    (p.N,p.NBASE,['tools/vr-common/body_main_mount_provider.h','tools/vr-common/hand_object_grasp.h','tools/vr-common/fresh_support_socket.h','tools/native-capture/weapon_hand_broker.h','tools/native-capture/full_body_c0_native_live.inl','tools/native-capture/body_c0_hands.h','tools/native-capture/body_main_mount_broker_test.inl'])):
    for relative in relations:
        original=base/'source'/relative;current=target/'source'/relative
        before=original.read_text() if original.exists() else '';after=current.read_text()
        patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile=target.name+'/a/'+relative,tofile=target.name+'/b/'+relative))
for name in ('body_main_mount_provider.h','fresh_support_socket.h'):
    assert (p.PG/'source/tools/vr-common'/name).read_bytes()==(p.N/'source/tools/vr-common'/name).read_bytes()
for name in ('Target','Request'):
    declarations=[re.search(r'struct '+name+r' \{[^\n]+', (target/'source/tools/vr-common/hand_object_grasp.h').read_text())[0] for target in (p.PG,p.N)]
    assert declarations[0]==declarations[1],name
assert (p.N/'source/tools/native-capture/lancer_grasp_live.inl').read_bytes()==(p.NBASE/'source/tools/native-capture/lancer_grasp_live.inl').read_bytes(),'Original100ms Request rejection unchanged'
check=json.loads((p.N/'fresh-socket-test-result.json').read_text());assert check['passed']
(p.R/'docs/hammerburst-fresh-passive-socket.patch').write_text(patch)
for target in (p.PG,p.N):
    path=target/'prepared.json';proof=json.loads(path.read_text())
    command=proof['build_command'] if target==p.PG else proof['commands'][1]
    with (target/'final-build.log').open('wb') as log:
        subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    digest=hashlib.sha256(Path(proof['dll']).read_bytes()).hexdigest()
    proof['dll_sha256' if target==p.PG else 'native_sha256']=digest
    proof.update(final_source_rebuilt=True,focused_checks_passed=True,original_request_source_unchanged=True)
    path.write_text(json.dumps(proof,indent=2));print(target.name,digest,flush=True)
