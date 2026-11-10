from pathlib import Path
import datetime,hashlib,importlib.util,json,sys
from inspect_guard_probe import inspect
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r138'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
root=R/'analysis/instances/cover-1111-r138-cloak-guard'
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and result['normal_cleanup_required'] and not result['prebirth_resource_refusal']
assert not result['remaining_owned'] and not result.get('error') and not result['forced_termination']
assert result['operator_exit']==0 and result['probe_mode']=='path'
assert result['projection_frames']==267 and not result['projection_errors'] and result['projection_thread_joined']
assert result['cloak_observations']==dict(samples=660,failures=[])
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
mute=index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations']==274 and mute['initially_muted_sessions']==1
assert not mute['verify_failed'] and not mute['target_shared_rejected']
assert result['disk_before']['admitted'] and result['disk_before']['free_bytes']==5136269312
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
original=Path(index['session_report']);saved=out/'session-original.json';assert sha(original)==sha(saved)
report=load(saved);probe=inspect(report)
assert probe['first_rejection_layer']=='engine/source_repeat/guard'
assert probe['original_started']==probe['original_returned']==0 and not probe['queried_and_returned']
assert probe['owner_guard']==dict(calls=21,failed_call=21,stage=8,name='context',detail=3,extra=6,context_verdict='source_age_expired',context_detail=6)
assert probe['session_guard']==dict(calls=4,failed_call=4,stage=2,name='physics_read',detail=3,extra=0)
assert probe['raw']['layout_step']=='40' and probe['raw']['layout_member_identities']=='225' and probe['raw']['layout_member_capacity']=='0'
assert report['native_status']['cover_sphere_probe_requested']=='0' and report['native_status']['cover_sphere_probe_attempted']=='0'
save('guard-probe.json',probe)
spec=importlib.util.spec_from_file_location('cover_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
pose=p.inspect(report,probe['native_sha256']);assert not pose['missing']
complete=next(x for x in pose['rows'] if x['name']=='complete');measurement=next(x for x in pose['rows'] if x['name']=='measurement')
assert complete['count']==2 and complete['scene']==395 and complete['compose']==24378 and complete['sequence']==3992
assert measurement['count']==328 and measurement['measurement_status']=='core' and measurement['scene']==399
save('pose-probe.json',pose)
media=load(out/'media-review.json')
assert media['frames']==267 and len(media['pages'])==14 and len(media['contacts'])==5 and media['encode_exit']==0
for row in media['pages']+media['contacts']:assert sha(Path(row['file']))==row['sha256']
media.update(full_span_personally_reviewed=True,all_contacts_personally_reviewed=True,
    visual_result='FAIL: own head/shoulder/torso blocks substantial portions of both eyes in all five cover phases; static opening and intermittent hand visibility retained',
    same_C0_GPU_proven=False,headset_accepted=False)
save('media-review.json',media)
boundary=load(next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json')))
assert len(boundary['eye_observation'])==5 and all(x['observation']['cover_state_raw']['CoverType']==2 for x in boundary['eye_observation'])
reacquisitions=[x['registry_reacquisition'] for x in boundary['steps'] if x.get('registry_reacquisition')]
save('closeout.json',dict(classification='progress: actual capsule path rejected by repeated-source guard before any original call',
    identities=identities,exact_owned_processes_absent_at_closeout=True,native_sha256=probe['native_sha256'],
    original_report=str(original),original_report_sha256=sha(saved),operator_exit=0,input_and_five_phases_completed=True,
    first_failure=probe['first_rejection_layer'],original_started=0,original_returned=0,
    failure_cause='ActualContext source_age_expired/detail6; body-source or equipment-source age not separately witnessed',
    owner_guard=probe['owner_guard'],session_guard=probe['session_guard'],
    diagnostic_capsule=probe['raw']['capsule'],diagnostic_delta=probe['raw']['delta'],
    body_geometry_verified=False,world_coverage_verified=False,collision_clear=None,movement_authorized=False,
    same_C0_across_rows=False,probe_admission=complete,separate_measurement=measurement,
    registry_reacquisition_branch_observed=bool(reacquisitions),normal_cleanup_verified=True,
    unified_weapons_cleanup=session['unified_weapons_cleanup'],cloak=result['cloak_observations'],mute=mute,
    disk_before=result['disk_before'],visual_result='FAIL',public_changed=False,headset_accepted=False,
    remaining=['reduce repeated reflection parsing with complete closing proofs and unchanged original source ages; distinguish body/equipment age if needed',
        'actual original path and endpoint execution', 'complete body geometry and complete current collision-world coverage',
        'same-pose C0 body/view/hit/muzzle', 'own head/shoulder obstruction', 'CV01-24 AC01-22 headset']))
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='\n## r138：真实首拒绝为ActualContext来源过期，未放宽时限（2026-11-11）\n\nr137源码先保存b0a73357后，新票cover-1111-r138-cloak-guard在两实例资源释放后只启动一次。Native10880b1c…6ff5、冻结PG/XR、Source1、0.4、独立path模式；game58172/creation134388022483876946、owner54612/creation134388022438460371、XR51104/creation134388022549958139、worker58904/creation134388022437680932。原版接近/A进入CT2及五段观察完成，operator0；无抓边或虚拟身体移动。\n\npath requested/attempted/finished均1，code6；Session engine5/detail6，Engine source_repeat3/source2=guard，original_started/returned前后均0。LayoutStep40、225成员见证、capacity0。新增外层见证21,21,8,3,6明确第21次Owner guard在ActualContext.current返回false，原Verdict3=source_age_expired、detail6；内层4,4,2,3,0表示第4次SourceGuard在physics_read得到typed Code3=guard。当前冻结ActualContext的detail6来自身体来源/装备来源时效判断，hard依赖先检查；它尚不区分其中哪项age失败，未观测起始age，不将本次37.2984ms自身耗时直接等同完整source age。不称身份失败，也不改原时间、延长时限或删除二次复核。\n\n本次Scene395/Compose24378/sequence3992/thread36648，World2319417600/gen1、Pawn95208479360、Link2314748672/slot0；诊断capsule(-3920.83569,-190.812454,621.348755)→(-3920.83569,-190.812454,623.348755),r1，delta(1,0,0)，括号113513466828200→113513504126600ns。姿态complete2首份同该C0；measurement/core328首份Scene399/Compose24623/sequence4000，是另一C0。零segments/hits/endpoint是未成功输出，不是空碰撞证明，原版路径/终点均未调用。\n\n267张实际双眼投影、录制错误0/线程joined、编码0；14页全跨度overview及5张原始phase图全部亲自查看。各段仍严重自身头肩/躯干遮挡，visualFAIL；长静止开头及间歇手部可见性保留。phase完整请求/观察区间内各7可选帧，不称同C0/GPU证明。正常stop，Native/入口/pad恢复、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0；保存时精确game/owner/XR/worker全不在，无强杀。CLOAK660次失败0，静音验证274/initially_muted1/verify_failed0/target_shared_rejected0，启动disk_before5136269312≥4GiB。\n\n证据guard-witness-r137/runtime-r138/{session-original,guard-probe,pose-probe,media-review,closeout}.json。下一步减少重复全反射解析的读取成本，保留完整兄弟链/身份/声明/字段闭合及原时效，先定向证明等价拒绝与读量，再编译复查；若须进一步拆分年龄源，沿现有拒绝码记录，绝不刷新旧数据时间。完整身体/世界覆盖、实际路径与终点调用、唯一C0同版视图/命中/枪口、头肩遮挡及CV01–24/AC01–22仍未完成。工程25%、公共v71和固定入口保持，目标active。\n'
assert '## r138：' not in doc.read_text(encoding='utf8')
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,first_failure=probe['first_rejection_layer'],original_calls=0,visual_result='FAIL')))
