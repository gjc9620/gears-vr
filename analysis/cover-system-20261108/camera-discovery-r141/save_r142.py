"""Preserve actual startup success and unattempted query as separate facts."""
from pathlib import Path
import datetime
import hashlib
import importlib.util
import json
import shutil
import sys

H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r142'
root=R/'analysis/instances/cover-1111-r142-cloak-discovery'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
native=load(root/'manifest.json')['native_sha256']
assert native=='8814ea68079dcb16b6815f3d72dd32ec3d5a7fdf2395d6302a5d19fb4eccc32b'
assert sha(root/'native/GearsCapture.dll')==native
assert result['normal_cleanup_verified'] and not result['remaining_owned'] and not result.get('error')
assert result['operator_exit']==0 and result['projection_frames']==302 and not result['projection_errors']
assert result['projection_thread_joined'] and result['anchor_sources_unchanged']
assert result['cloak_observations']==dict(samples=717,failures=[])
assert result['foreground_observations']==dict(samples=453,owned_activation=False)
assert session['native_started'] and session['native_cleanup_verified'] and session['entry_restore_verified']
assert session['vr_gamepad_cleanup']['restored'] and session['unified_weapons_cleanup']['drained']
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
assert result['isolated_done']['owned_game_absent'] and not result['forced_termination']
mute=index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations']==305 and mute['initially_muted_sessions']==1
assert not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl()
identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert control.identity(identity['pid'])!=identity
for source,name in ((Path(index['session_report']),'session-original.json'),
    (root/'cover-output/run-result.json','run-result-original.json'),
    (root/'native/anchor-observer-config.json','anchor-config-original.json'),
    (root/'isolated-done.json','isolated-done-original.json')):
    if (out/name).exists():assert sha(out/name)==sha(source)
    else:shutil.copyfile(source,out/name)
probe=module('r142_query',H.parent/'guard-witness-r137/inspect_guard_probe.py').inspect(session,native)
assert probe['raw']['requested']=='1' and probe['raw']['attempted']=='0' and probe['raw']['finished']=='0'
assert probe['owner_guard']['calls']==probe['session_guard']['calls']==0
assert all(probe['raw'][k]=='0' for k in ('original_started_before','original_started_after','original_returned_before','original_returned_after'))
save('path-probe.json',probe)
pose=module('r142_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py').inspect(session,native)
assert pose['rows']==[dict(step=8,name='measurement',count=258,detail=2,scene=340,compose=20953,
    sequence=4277,world=1564115200,pawn=95359002240,measurement_status='core')]
assert not pose['complete_gate_observed'] and not pose['missing']
save('pose-probe.json',pose)
anchor=load(out/'anchor-config-original.json')
guards=[dict(stage=g['stage'],accepted=g['accepted'],
    deadline_ns=g['deadline_perf_counter_ns']-g['started_perf_counter_ns'],
    attempts=[dict(elapsed_ms=a['elapsed_ms'],accepted=a['strict_guard_accepted']) for a in g['attempts']])
    for g in anchor['guard_acquisitions']]
assert len(guards)==5 and all(g['accepted'] and g['deadline_ns']==200000000 for g in guards)
assert guards[0]['attempts']==[dict(elapsed_ms=59.6409,accepted=True)]
review=load(out/'media-review.json')
assert review['frames']==302 and len(review['pages'])==16 and len(review['contacts'])==5 and review['encode_exit']==0
for row in review['pages']:assert sha(Path(row['file']))==row['sha256']
for row in review['contacts']:
    assert sha(Path(row['file']))==row['sha256'] and not row['same_C0_proven']
    archived=out/(row['label']+'.jpg');shutil.copyfile(row['file'],archived)
    assert sha(archived)==row['sha256'];row['archived_file']=str(archived)
review.update(full_span_personally_reviewed=True,reviewed_overview_pages=16,reviewed_original_contacts=5,
    cover_view_result='FAIL: head/shoulder/torso mesh obstructs both eyes in all five contact phases',
    same_C0_proven=False,headset_accepted=False)
save('media-review.json',review)
save('closeout.json',dict(classification='progress: actual bounded anchor acquisition success; query never attempted',
    identities=identities,exact_owned_processes_absent=True,native_sha256=native,
    anchor_guards=guards,anchor_sources_unchanged=True,normal_cleanup_verified=True,
    cloak=result['cloak_observations'],foreground=result['foreground_observations'],mute=mute,disk_before=result['disk_before'],
    query_requested=True,query_attempted=False,original_query_calls=0,pose_measurement_rejections=258,
    first_measurement_status='core',core_predicate_failure_detail='not recorded; source age versus other predicates unproven',
    core_policy_maximum_age_ns=50000000,query_optimization_game_effect='unproven; caller gate not reached',
    frames=302,encode_exit=0,reviewed_pages=16,reviewed_contacts=5,cover_view_accepted=False,
    collision_clear=None,body_geometry_verified=False,world_coverage_verified=False,movement_authorized=False,
    public_changed=False,headset_accepted=False,engineering_percent=25,
    remaining='Observe the exact failing core predicate at the existing cover measurement, without renewing timestamps or weakening the 50ms policy; no unchanged rerun. Full CV01-24/AC01-22 remains open.'))
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r142：严格启动采集通过，姿态 core 门未到查询入口（2026-11-11）

先保存583a74af，准备新票cover-1111-r142-cloak-discovery，并绑定8个实际anchor启动来源SHA；结束后逐项一致。等待真实worker资源释放后准入，disk_before128625233920≥4GiB。game11788/creation134388064387918283、owner21348/creation134388064304674851、XR39640/creation134388064477169381、worker31316/creation134388064303553040。Native仍8814ea68…cc32b，PG/XR保留原冻结版本；公开v71不变。

实际anchor五次guard均在原200ms内accepted：initial59.6409ms、sample_before1.7749ms、sample_after6.3687ms、assembly_after2.0053ms、final_boundary1.547ms。Native启动、实际projection及原A进入CT2与五段eye观察完成。只证明本次采集成功，不以跨进程59.64对3536.19宣称速度倍率或r140唯一因果。

path requested1、attempted0、finished0，owner/session guard调用均0，原查询started/returned均0；r139成员链优化的实际调用收益仍未被执行验证。既有姿态first-failure统计只有measurement258次/detail2=core，首证Scene340/Compose20953/seq4277；complete0。精确冻结body_goal_view_adapter规定50ms且core_snapshot_valid是多条件谓词，当前回执没有失败分项，不能把全部失败定性为年龄、布局或本次优化回归；也不能用最后一次状态拼成首证同C0。

302实际私有projection帧/errors0、编码0；16页全跨度总览与5张原始contact已亲看，五个阶段头／肩／躯干模型严重遮挡双眼，画面验收FAIL。每阶段整段落内7/7/7/7/8张候选；取中间contact只说明时间段，非同C0／GPU骨证明。没有抓边或虚拟身体移动，不宣称路径／占用安全。

launcher0、Native/entry/input恢复、武器fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常，独立精确身份复核全不在，无强杀。CLOAK717/失败0、前台453/owned_activation false；静音305/initial1/失败0/共享目标拒绝0。原始session、anchor、run-result及done保存于camera-discovery-r141/runtime-r142，保留r140原失败。

下一窄点是在现有测量first-failure记录中区分core谓词失败及原始时间，不重读游戏、不刷新来源、不放宽50ms或100ms。不要原样重跑本批。完整身体几何与世界覆盖、实际抓边移动、身体遮挡／暴露／枪口／受击、CV01–24／AC01–22仍未完成；工程25%和目标active保持。
'''
payload=entry.replace('\n','\r\n').encode('utf-8');updated=doc.read_bytes()+payload
doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,query_attempted=False,core_rejections=258)))
