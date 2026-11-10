from pathlib import Path
import datetime,hashlib,importlib.util,json,sys
from inspect_path_probe import inspect
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r136'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
root=R/'analysis/instances/cover-1111-r136-cloak-path'
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and result['normal_cleanup_required'] and not result['prebirth_resource_refusal']
assert not result['remaining_owned'] and not result.get('error') and not result['forced_termination']
assert result['operator_exit']==0 and result['probe_mode']=='path'
assert result['projection_frames']==251 and not result['projection_errors'] and result['projection_thread_joined']
assert result['cloak_observations']==dict(samples=618,failures=[])
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
mute=index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations']==248 and mute['initially_muted_sessions']==1
assert not mute['verify_failed'] and not mute['target_shared_rejected']
assert result['disk_before']['admitted'] and result['disk_before']['free_bytes']==7454486528
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
original=Path(index['session_report']);saved=out/'session-original.json';assert sha(original)==sha(saved)
report=load(saved);probe=inspect(report)
assert probe['first_rejection_layer']=='engine/source_repeat/guard'
assert probe['original_started']==probe['original_returned']==0 and not probe['queried_and_returned']
assert probe['raw']['layout_step']=='40' and probe['raw']['layout_member_identities']=='225' and probe['raw']['layout_member_capacity']=='0'
assert report['native_status']['cover_sphere_probe_requested']=='0' and report['native_status']['cover_sphere_probe_attempted']=='0'
save('path-probe.json',probe)
spec=importlib.util.spec_from_file_location('cover_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
pose=p.inspect(report,probe['native_sha256']);assert not pose['missing']
complete=next(x for x in pose['rows'] if x['name']=='complete');measurement=next(x for x in pose['rows'] if x['name']=='measurement')
assert complete['count']==316 and complete['scene']==431 and complete['compose']==26633 and complete['sequence']==3588
assert measurement['count']==24 and measurement['measurement_status']=='core' and measurement['scene']==448
save('pose-probe.json',pose)
media=load(out/'media-review.json')
assert media['frames']==251 and len(media['pages'])==13 and len(media['contacts'])==5 and media['encode_exit']==0
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
    failure_cause='guard returned false; exact nested predicate not witnessed; no timer/identity root-cause claim',
    diagnostic_capsule=probe['raw']['capsule'],diagnostic_delta=probe['raw']['delta'],
    body_geometry_verified=False,world_coverage_verified=False,collision_clear=None,movement_authorized=False,
    same_C0_across_rows=False,probe_admission=complete,separate_measurement=measurement,
    registry_reacquisition_branch_observed=bool(reacquisitions),normal_cleanup_verified=True,
    unified_weapons_cleanup=session['unified_weapons_cleanup'],cloak=result['cloak_observations'],mute=mute,
    disk_before=result['disk_before'],visual_result='FAIL',public_changed=False,headset_accepted=False,
    remaining=['first failed predicate in repeated source guard, preserving all current-source checks',
        'actual original path and endpoint execution', 'complete body geometry and complete current collision-world coverage',
        'same-pose C0 body/view/hit/muzzle', 'own head/shoulder obstruction', 'CV01-24 AC01-22 headset']))
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r136：实际路径在source_repeat/guard拒绝，原版调用0；正常关闭与双眼失败已核验（2026-11-11）

r135预创建拒绝及等待分类修复已保存626c6dd0。新票cover-1111-r136-cloak-path在精确worker资源释放后只启动一次，Native0098be93…37cb、冻结PG/XR、Source1、0.4、path模式，sphere requested/attempted均0。game16040/creation134388003952038420、owner21152/creation134388003872202839、XR55760/creation134388004017415947，worker47672/creation134388003871254438。原版接近/A进入CT2及五段观察完成，operator0；没有抓边或虚拟身体移动。

path requested/attempted/finished均1，code6=changed；Session engine5/detail6，Engine source_repeat3/source_code2=guard、exception0，original_started/returned前后均0。LayoutStep40、225独立成员见证、capacity0，已越过physics/collider/component初始读取，未越过引擎第二次来源复核。source::capture返回guard只证明其调用的当前来源guard为false，尚未区分内部typed读闭合、身份、元数据、原时间或owner等具体条件，不猜计时/身份根因、不删除复核。零hit/endpoint/segment字段是未成功输出，不是空碰撞证据。

本次Scene431/Compose26633/sequence3588/thread25192，World2312601856/gen1、Pawn95194022016、Link2309112576/slot0；诊断胶囊(-3921.29443,-148.927185,615.215271)→(-3921.29443,-148.927185,617.215271),r1，delta(1,0,0)，括号111654972425700→111655005645500ns。仍是厘米级ABI控制，非正式身体体积。姿态complete316，首份同该C0；measurement/core24首份Scene448/Compose27715/sequence3615，属于另一C0，分开保留。

251张实际私有双眼投影、录制错误0/线程joined、编码0；13页完整跨度overview和5张原始phase图全部亲自查看。头肩/躯干仍遮挡双眼大块区域，visualFAIL。phase完整请求/观察括号内各7可选帧，不称同C0/GPU消费。正常stop，Native/入口/pad恢复、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0；精确game/owner/XR/worker均不在，无强杀。CLOAK618次失败0，静音验证248/initially_muted1/verify_failed0/target_shared_rejected0；启动disk_before7454486528≥4GiB。新prebirth分类在本次为false，真实启动清理分支保持；不拿此运行冒充跳过等待特殊分支实测。

原报告原样复制并按精确SHA解释，解析器新增本次source_repeat/guard拒绝案例后10项合成检查0。证据path-probe-r134/runtime-r136/{session-original,path-probe,pose-probe,media-review,closeout}.json。下一步记录来源guard首个具体拒绝谓词，定向修复后再查；不重跑未改Sphere或同一失败路径，不放宽原始时效/身份/释放。完整身体/世界覆盖、唯一C0提交及同版视图/命中/枪口、遮挡与全部CV01–24/AC01–22仍未完成。工程25%、公共v71和固定手动入口保持，目标active。
'''
assert '## r136：' not in doc.read_text(encoding='utf8')
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,first_failure=probe['first_rejection_layer'],original_calls=0,visual_result='FAIL')))
