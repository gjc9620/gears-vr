"""Save the actual first Core age witness, lifecycle and reviewed images."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r145'
root=R/'analysis/instances/cover-1111-r145-cloak-core'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and result['operator_exit']==0 and not result.get('error')
assert not result['remaining_owned'] and not result['forced_termination'] and result['anchor_sources_unchanged']
assert result['projection_frames']==323 and not result['projection_errors'] and result['projection_thread_joined']
assert result['cloak_observations']==dict(samples=860,failures=[])
assert result['foreground_observations']==dict(samples=524,owned_activation=False)
assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0 and result['isolated_done']['owned_game_absent']
mute=index['test_audio_guard']['mute_verification'];assert mute['verified_muted_observations']==349
assert mute['initially_muted_sessions']==1 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert control.identity(identity['pid'])!=identity
for source,name in ((Path(index['session_report']),'session-original.json'),(root/'cover-output/run-result.json','run-result-original.json'),
    (root/'native/anchor-observer-config.json','anchor-config-original.json'),(root/'isolated-done.json','isolated-done-original.json')):
    shutil.copyfile(source,out/name)
from inspect_core import inspect
pose=inspect(session);w=pose['core_witness'];save('pose-core-probe.json',pose)
assert pose['rows']==[dict(step=8,name='measurement',count=241,detail=2,scene=337,compose=20751,
    sequence=5433,world=2209054976,pawn=95208990336,measurement_status='core')]
assert w['mode']==1 and w['valid']==0 and w['non_age_valid']==1 and w['expired']==2 and w['controllers_match']==1
assert w['legacy_age_only_proven'] and w['limit']==50000000
ages=dict(head_ns=w['now']-w['stereo_received'],publication_ns=w['now']-w['published'],
    controller_ns=w['now']-w['controller_received'])
assert ages==dict(head_ns=58791700,publication_ns=38145700,controller_ns=58760400)
spec=importlib.util.spec_from_file_location('r145_query',H.parent/'guard-witness-r137/inspect_guard_probe.py')
q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
native=load(H/'native-build-result.json')['dll_sha256'];assert sha(root/'native/GearsCapture.dll')==native
query=q.inspect(session,native);save('path-probe.json',query)
assert query['raw']['attempted']=='0' and query['raw']['requested']=='1' and query['raw']['finished']=='0'
assert query['owner_guard']['calls']==query['session_guard']['calls']==0
assert all(query['raw'][k]=='0' for k in ('original_started_before','original_started_after','original_returned_before','original_returned_after'))
review=load(out/'media-review.json');assert len(review['pages'])==17 and len(review['contacts'])==5 and review['encode_exit']==0
for row in review['pages']:assert sha(Path(row['file']))==row['sha256']
for row in review['contacts']:
    assert sha(Path(row['file']))==row['sha256'] and row['eligible_frames']==7 and not row['same_C0_proven']
    target=out/(row['label']+'.jpg');shutil.copyfile(row['file'],target);row['archived_file']=str(target)
review.update(full_span_personally_reviewed=True,reviewed_overview_pages=17,reviewed_original_contacts=5,
    cover_view_result='FAIL: head/shoulder/torso mesh obstructs both eyes in every contact phase',same_C0_proven=False)
save('media-review.json',review)
save('closeout.json',dict(classification='progress: first actual legacy Core measurement rejected specifically on original head-sample age',
    identities=identities,exact_owned_processes_absent=True,native_sha256=native,first_pose=pose['rows'][0],first_core=w,ages=ages,
    first_cause_proven=True,all_241_causes_proven=False,query_requested=True,query_attempted=False,original_query_calls=0,
    normal_cleanup_verified=True,cloak=result['cloak_observations'],foreground=result['foreground_observations'],mute=mute,
    disk_before=result['disk_before'],frames=323,reviewed_pages=17,reviewed_contacts=5,encode_exit=0,
    world_coverage_verified=False,movement_authorized=False,cover_view_accepted=False,public_changed=False,headset_accepted=False,
    remaining='Reuse the already implemented current Tick measurement overload in the frozen candidate; do not renew old Core clocks or relax 50ms/40ms predicates.'))
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
before=doc.read_bytes();assert before==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r145：首个测量拒绝确定为旧头部样本58.7917ms（2026-11-11）

r144票在最终共享准入前被其他worker占用空位，start1、stdout空、明确Two automatic instances拒绝，无worker/index/game；claim保留消耗，normal_cleanup_required=false，不补签正常收尾，不算一次游戏。随后新票r145继续等真实worker资源释放，未放宽2实例/内存/4GiB规则。

r145实际game32320/creation134388087505037769，Native5522ac2a…ec093、同冻结PG/XR；启动anchor各获取最终均在原200ms内接受，8个anchor来源SHA前后一致。A进入CT2、五段eye观察完成，measurement241次core、complete0、path requested1/attempted0/finished0、原查询0。

首个同一测量证据Scene337/Compose20751/seq5433：mode1旧Core，strict0/non_age1/controllers_match1、expired2，仅原头部样本超过50ms。原now120038702219900、stereo_received120038643428200，年龄58.7917ms；publication120038664074200，年龄38.1457ms；controller_received120038643459500，年龄58.7604ms仍在其100ms内。票据338/引用代次6/size1864/ABI1/flags7/status1。证明这个首证拒绝是头部年龄，不将同一结果扩张到全部241次或倒推r142首证。

现主线已有035f9d7f的BodyTickSource测量接口：当前追踪40ms、元数据50ms、原classify及当前CoverKey前后检查；私有冻结测量仍只用旧Core。下一修复应只把既有当前Tick测量重载/接线补入冻结候选，保留旧Core路径原拒绝和原始所有时间，不通过更新旧Core或扩大时限放行；尚未编译/实跑此后续修复。

323实际私有projection帧/errors0、编码0；17页完整跨度总览和5原始contact亲看，五阶段模型头肩/躯干严重遮挡双眼，仍FAIL。每阶段7个完整落入动作时间段的候选，不能当同C0/GPU骨证明。launcher0、Native/entry/input恢复、fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常，独立精确身份全不在、无强杀；CLOAK860/失败0，前台524/owned_activation false，静音349/initial1/失败0/共享拒绝0。disk_before104385679360。原始session/anchor/run-result/done及同测量分项已保存core-measurement-r143/runtime-r145。

本轮实际根因证据是进展；完整身体/世界碰撞覆盖、实际抓边移动、遮挡/暴露/枪口/受击、CV01–24/AC01–22及头显仍未完成。工程25%、publicv71和固定手动入口不变，目标active。
'''
updated=before+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,first_head_age_ms=58.7917,normal_cleanup_verified=True,query_attempted=False)))
