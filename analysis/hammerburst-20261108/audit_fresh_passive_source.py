import json,hashlib
import prepare_fresh_passive_socket as p
groups=[(p.PG,p.PGBASE,{
    'tools/vr-common/body_main_mount_provider.h','tools/vr-common/hand_object_grasp.h',
    'tools/playground/weapon_native_lancer_follow.inl','tools/playground/body_main_mount_sender.h',
    'tools/playground/body_main_mount_provider.inl','tools/playground/body_main_mount_sender_test.cpp'}),
    (p.N,p.NBASE,{'tools/vr-common/body_main_mount_provider.h','tools/vr-common/hand_object_grasp.h',
    'tools/native-capture/weapon_hand_broker.h','tools/native-capture/full_body_c0_native_live.inl',
    'tools/native-capture/body_c0_hands.h','tools/native-capture/body_main_mount_broker_test.inl'})]
results=[]
for target,base,allowed in groups:
    changed=[];count=0
    for source in (base/'source/tools').rglob('*'):
        if not source.is_file():continue
        relative=source.relative_to(base/'source').as_posix();current=target/'source'/relative
        assert current.is_file(),relative
        if source.read_bytes()!=current.read_bytes():changed.append(relative)
        count+=1
    assert set(changed)==allowed,(target,changed)
    results.append(dict(candidate=target.name,compared_original_files=count,changed=changed))
proof=dict(passed=True,groups=results,source_and_input_policy_files_unchanged=True,actual_old_world_request100ms_implementation_unchanged=True,
           writer_unchanged='Existing sole C0 transaction; only its cosmetic target source/assembly changes',
           native_original_compile_definitions=json.loads((p.N/'prepared.json').read_text())['original_compile_definitions'],
           tests=json.loads((p.N/'fresh-socket-test-result.json').read_text()),runtime_proved=False)
(p.A/'fresh-passive-source-audit.json').write_text(json.dumps(proof,indent=2));print(json.dumps(results))
