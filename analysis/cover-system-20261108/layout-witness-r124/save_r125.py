from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r125'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
root=R/'analysis/instances/cover-1110-r125-cloak-layout'
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and not result['remaining_owned'] and result['projection_thread_joined']
assert result['operator_exit']==0 and not result.get('error') and not result['forced_termination']
assert not result['projection_errors'] and result['projection_frames']==286
assert result['cloak_observations']['samples']==617 and not result['cloak_observations']['failures']
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
mute=index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations']>0 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl()
for key in ('game_identity','owner_identity','xr_identity'):assert control.identity(session[key]['pid'])!=session[key]
report=Path(index['session_report']);saved=out/'session-original.json';assert not saved.exists();shutil.copyfile(report,saved)
spec=importlib.util.spec_from_file_location('cover_probe_interpret',H.parent/'source-stage-r104/inspect_probe.py')
parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
native=load(H/'native-build-result.json')['dll_sha256'];probe=parser.inspect(load(saved),native)
assert probe['raw']['attempted']=='0' and probe['raw']['finished']=='0' and probe['collision_clear'] is None
(out/'probe.json').write_text(json.dumps(probe,indent=2)+'\n')
media=load(out/'media-review.json');assert media['encode_exit']==0 and len(media['pages'])==15 and len(media['contacts'])==5
media.update(full_span_personally_reviewed=True,all_contacts_personally_reviewed=True,
    visual_result='FAIL: self head/shoulder/torso obstructs both eyes in all five cover phases; hands/arms visible before entry',headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
status=session['native_status']
record=dict(classification='progress: new production witness compiled/tested; actual run exposes earlier unobserved gate',
    game=session['game_identity'],owner=session['owner_identity'],xr=session['xr_identity'],native_sha256=native,
    source_commit='b40db9d7',original_report=str(report),original_report_sha256=sha(saved),
    run_result_sha256=sha(root/'cover-output/run-result.json'),input_and_phase_sequence=True,
    query_attempted=False,query_success=False,layout_rejection_reproduced=False,layout_field='unobserved',
    original_query_started=False,collision_clear=None,movement_authorized=False,new_capsule_enumeration_executed=False,
    pre_probe_gate='unknown: captured C0 cover-pose admission/measurement before observe_sphere_once; infer path from callsite, exact failed predicate unproven',
    body_native_captures=status['body_native_captures'],body_native_commits=status['body_native_commits'],
    normal_cleanup_verified=True,unified_weapons_cleanup=session['unified_weapons_cleanup'],
    cloak=result['cloak_observations'],mute_verification=mute,media_review_sha256=sha(out/'media-review.json'),
    remaining=['bounded pre-probe admission/cover measurement rejection','specific physics layout rejection','actual original query execution',
               'head shoulder obstruction','endpoint/world coverage/body commit','CV01-24 AC01-22 headset'])
(out/'closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=H.parent/'requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry=f'''
## r125：实际进入CT2与五段观察，布局诊断未到达（2026-11-10）

保存b40db9d7后，当前CLOAK/静音入口新建cover-1110-r125-cloak-layout；Native f5d3cf69…9850，PG/XR沿用匹配冻结版本，0.4。game54104/creation134387912043325145，owner59216/creation134387911939008697，XR55772/creation134387912123551617。Source1输入、原版接近及A进入CT2、neutral/turned/restored/raised/restored五段均实际执行，operator0；未做抓边或虚拟身体移动。

关键负结果：Sphere requested1但attempted0/finished0/code1，outer/Session/Engine/layout均idle0、pass0，原版started/returned0。此运行未到r124新布局检查，既不证明布局通过，也不复现r123具体layout5，更不能把raw_hit0当无碰撞。实际C0 captures693/commits376与原CT2已读到；固定源码唯一调用位于body_c0_goal_policy_live::sample_current_cover_pose在current/key/marker/measure/recheck之后。可定位下一观察范围为probe之前的C0准入/掩体姿态测量，哪个条件失败尚未证实，不删这些条件、不盲目重跑同候选。

286张实际双眼投影、录制错误0、线程joined，VFR编码exit0；15张全跨度overview及5张原始phase图全部亲自查看。进入前可见双手/手臂，进入后五段均有自身头肩/躯干严重遮挡，visualFAIL保持。phase图严格选事件区间内完整请求/观察括号（5/8/7/7/7帧可选），不称同C0或GPU消费证明；静止开场持续约16秒也保留。实际probe没有发生，不把“动作脚本passed”当掩体功能或原查询通过。

正常stop、Native/入口/手柄恢复true、fire_disable0/hand_stop5/drainedtrue，XR0/launcher0，本方精确game/owner/XR均已不存在，无强杀。CLOAK617次失败0；目标静音回读{mute['verified_muted_observations']}次、verify_failed0/target_shared_rejected0，保留其他会话及系统音量。原报告按launcher-session索引原样复制，SHA与实际身份绑定；失败路径不改写此前早停记录。

证据layout-witness-r124/runtime-r125/{{probe,closeout,media-review,session-original}}.json。下一步先给现有sample_current_cover_pose的早退条件增加有界失败见证，再定向修复实际拒绝；r124布局见证保持可用。公共v71、完整CV01–24/AC01–22、工程25%及物理头显未完成状态不变。
'''
assert '## r125：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup=True,probe_attempted=False,mute_verified=mute['verified_muted_observations'])))
