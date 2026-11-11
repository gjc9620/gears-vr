"""Preserve matched real timings and failed view, with independent cleanup."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r166-cloak-components';out=H/'runtime-r166'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];t=load(out/'timing-probe.json');q=t['query'];raw=q['raw']
assert v['normal_cleanup_verified'] and v['operator_exit']==0 and not v.get('error') and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==290 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['source_age_cause']['causes']==1 and q['original_started']==q['original_returned']==0
assert q['first_rejection_layer']=='engine/source_guard/current' and q['owner_guard']['failed_call']==29
assert q['session_guard']['failed_call']==6 and q['session_guard']['name']=='physics_read'
assert t['elapsed_ns']==31772900 and all(r['calls']==r['closed_spans'] for r in t['rows'])
spec=importlib.util.spec_from_file_location('r166_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(r for r in pose['rows'] if r['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert int(raw[k])==complete[k]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
review=load(out/'media-review.json');assert len(review['pages'])==15 and len(review['contacts'])==5 and review['encode_exit']==0
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(full_span_personally_reviewed=True,reviewed_pages=15,reviewed_original_contacts=5,
    verdict='All five actual phases have severe self-model obstruction; FAIL',cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json')):shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,native_sha256=q['native_sha256'],
    original_low_cover_entered=True,same_C0_complete_query_key=True,query_elapsed_ns=t['elapsed_ns'],timing=t['rows'],
    source_age_cause=q['source_age_cause'],owner_guard=q['owner_guard'],session_guard=q['session_guard'],
    original_query_started=0,original_query_returned=0,source_age_repaired=False,cover_view_accepted=False,
    world_coverage_verified=False,movement_authorized=False,headset_accepted=False,public_changed=False,
    repeated_physics_collider_largest_measured_stage=True,nested_times_must_not_be_added=True,
    next='Inspect repeated SourceGuard physics/collider sample costs and call sites; retain every identity/dynamic/closing/freshness check. No unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')

gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r166' not in old
entry="""
## r166：组件优化候选实际进入通过，六轮复核后仍身体年龄拒绝（2026-11-11）

9564799d保存后，以新cover-1111-r166-cloak-components和Native3f1d5454…be64d做一次原CT2及同query分段验证。源码核对、准备和操作导入通过，共享启动两文件SHA保持7e630461/385f5e5a，13项anchor/启动来源前后相同；原A进入CT2、五段eye观察与operator0。290投影帧/errors0，编码0；15页全跨度及5实际阶段contact均亲看，候选7/7/8/8/7，五阶段自身头肩/躯干遮挡严重FAIL，不声称同C0/GPU或头显验收。正常清理true、无强杀，Native/entries/input恢复，XR和launcher正常退出，game/owner/XR/worker精确身份独立确认不在；CLOAK652/失败0，前台511/owned_activation false。头部遮蔽记录head_section_applied678、palettes678、uncomposed432、shoulder_armor678；这些累计记录不证明实际每Draw消费，不能替代或推翻本次遮挡像素。

本path键Scene372/Compose22860/seq3678/world2309652736/pawn95368151168与complete首键逐项相同。engine/source_guard/current；Owner第29次context/source_age_expired/detail6，Session第6次physics_read/guard。原因见证entry14674320/Compose22860/thread34024/stage0/verdict3/causes1，身体来源年龄拒绝、equipment当次未过期。原碰撞started/returned0/0；diagnostic小胶囊仍无完整身体或安全移动证明。

query159405188618100→159405220391000共31.7729ms，计时invalid0/overflow0、全部span闭合。initial physics0.8903ms、collider2.7614ms、components7.7426ms；physics repeat6次共6.0789ms、collider repeat5次共13.6629ms、components repeat0次；engine19.9980ms、closing0次。engine包含上述复核，不再相加。初始component实测低于r164的15.5895ms，但不同入口年龄、调度/Actor上下文未控制，不能把全部降幅归为源码因果；本次读到更多复核且最终仍拒绝，不是完整查询性能通过。当前最大观察成本为引擎内反复physics/collider复核，不放宽时限或刷新旧样本。

记录在component-root-repeat-r165/runtime-r166。下一步检查这些复核的真实调用位置和重复schema成本，保持每次动态与身份重读、关闭证明和全部年龄/反重放约束，不原样重跑本候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及实体头显状态不变，目标active。
"""
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,initial_components_ns=7742600,source_age_repaired=False)))
