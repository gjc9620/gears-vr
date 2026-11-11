"""Save actual failed r162 validation, reviewed images and exact cleanup facts."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r162-cloak-schema';out=H/'runtime-r162'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session']
assert v['normal_cleanup_verified'] and v['operator_exit']==0 and not v.get('error')
assert v['projection_frames']==274 and not v['projection_errors'] and v['projection_thread_joined']
assert v['anchor_sources_unchanged'] and len(v['anchor_source_sha256'])==13
assert not v['forced_termination'] and not v['remaining_owned']
native='58c2fe1d9fa2a108010f33008278060f9ad3a2ccde53d3d87f3e27c829680c7f'
assert sha(root/'native/GearsCapture.dll')==native
q=load(out/'path-probe.json');raw=q['raw'];cause=q['source_age_cause']
assert cause['observed'] and cause['causes']==1 and q['original_started']==q['original_returned']==0
assert q['first_rejection_layer']=='engine/source_begin/guard'
assert q['session_guard']['name']=='physics_read' and q['session_guard']['failed_call']==1
assert q['owner_guard']['failed_call']==9 and q['owner_guard']['context_detail']==6
elapsed=int(raw['end_ns'])-int(raw['begin_ns']);assert elapsed==22273500
spec=importlib.util.spec_from_file_location('r162_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,native);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(r for r in pose['rows'] if r['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert int(raw[k])==complete[k]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));action=load(boundary)
assert action['passed'] and action['after_original_A']['cover_state_raw']['CoverType']==2 and len(action['eye_observation'])==5
review=load(out/'media-review.json')
assert len(review['pages'])==14 and len(review['contacts'])==5 and review['encode_exit']==0
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(full_span_personally_reviewed=True,reviewed_pages=14,reviewed_original_contacts=5,
    verdict='All five actual contact images retain severe self head/shoulder/torso obstruction; FAIL',
    cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl();identities={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert control.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json'),
    (root/'isolated-done.json','isolated-done-original.json'),(root/'launcher-session.json','launcher-index-original.json')):
    shutil.copyfile(source,out/name)
summary=load(out/'closeout.json')
summary.update(identities=identities,exact_owned_processes_absent=True,native_sha256=native,
    original_low_cover_entered=True,source_age_cause=cause,owner_guard=q['owner_guard'],session_guard=q['session_guard'],
    query_elapsed_ns=elapsed,same_C0_complete_query_key=True,original_query_started=0,original_query_returned=0,
    source_age_repaired=False,latency_improvement_proven=False,cover_view_accepted=False,world_coverage_verified=False,
    movement_authorized=False,reviewed_pages=14,reviewed_contacts=5,public_changed=False,
    next='Inspect the source-to-query prefix: initial typed-role samples precede the engine SourceGuard full resample; preserve all freshness and closing proofs.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r162：结构复核去重候选完成实跑，原查询前仍拒绝身体来源年龄（2026-11-11）

本轮首先核对当前工作树、启动源及磁盘，再以新cover-1111-r162-cloak-schema验证Native58c2fe1d…0c7f。13项准备来源前后相同，准备与操作导入均0，既有共享600秒准入接受；运行前磁盘73000828928字节。原版A进入CT2、五段eye动作与operator0；274投影帧、errors0、编码0，正常清理true、无强杀，game/owner/XR/worker精确身份独立确认均不在。CLOAK749/失败0，前台456/owned_activation false。原始会话、动作、来源和退出记录保存在schema-repeat-r158/runtime-r162。

本path首键Scene437/Compose26594/seq4372/world2137293056/pawn95255003392与complete首键逐项相同。path code4、engine/source_begin/guard，Owner第9次context/source_age_expired/detail6，Session第1次physics_read/guard；原因见证observed1/entry14674320/Compose26594/thread54328/stage0/verdict3/causes1，身体来源年龄拒绝、装备来源本次未过期。原版碰撞started/returned仍0/0；query156032411584500→156032433858000共22.2735ms。与r157拒绝层内部位置、入口年龄及调度不匹配，不能据此声称实际全query加速或年龄修复。读取优化未打通本次原调用，不刷新旧样本、不放宽时限。

14页全跨度和5张实际阶段contact均亲看，候选7/7/7/8/7；五阶段仍有严重自身头肩/躯干遮挡，画面FAIL，非完整掩体或头显通过。阶段区间不冒充同C0/GPU证明。历史媒体按主人清理指令可能已删除，历史观察仍作为原记录，不重新打开或为补图重跑。

下一窄点是来源到查询的前缀：Session先完成physics/collider/component初始样本，engine入口SourceGuard又进入完整physics采样；本次在该首复核拒绝。后续须定位各层成本与入口实际年龄，再选择保留完整身份/动态读取/关闭证明的修复；不原样重跑本候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71和固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,query_elapsed_ns=elapsed,body_age_repaired=False,normal_cleanup_verified=True,reviewed_pages=14,contacts=5)))
