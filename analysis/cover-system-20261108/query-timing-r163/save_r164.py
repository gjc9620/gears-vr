"""Preserve matched real timings and failed view, with independent cleanup."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r164-cloak-timing';out=H/'runtime-r164'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];t=load(out/'timing-probe.json');q=t['query'];raw=q['raw']
assert v['normal_cleanup_verified'] and v['operator_exit']==0 and not v.get('error') and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==265 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['source_age_cause']['causes']==1 and q['original_started']==q['original_returned']==0
assert q['first_rejection_layer']=='engine/source_repeat/guard' and q['owner_guard']['failed_call']==19
assert q['session_guard']['failed_call']==3 and q['session_guard']['name']=='collider_read'
assert t['elapsed_ns']==28497500 and all(r['calls']==r['closed_spans'] for r in t['rows'])
spec=importlib.util.spec_from_file_location('r164_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(r for r in pose['rows'] if r['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert int(raw[k])==complete[k]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
review=load(out/'media-review.json');assert len(review['pages'])==14 and len(review['contacts'])==5 and review['encode_exit']==0
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(full_span_personally_reviewed=True,reviewed_pages=14,reviewed_original_contacts=5,
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
    initial_components_largest_measured_stage=True,nested_times_must_not_be_added=True,
    next='Inspect component-role root/schema repetition; retain all dynamic reads and closing proofs. No unchanged timing rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r164' not in old
entry='''
## r164：同查询分段实测，初始组件目录为最大成本，仍在重复collider复核年龄拒绝（2026-11-11）

4839a96c保存后，以新cover-1111-r164-cloak-timing和Native50ced042…65f79做一次既有后台path观察。准备/操作导入0，13项启动来源前后相同；原A进入CT2、五段eye动作和operator0。265投影帧/errors0，编码0；14页全跨度和5实际阶段contact全部亲看，候选7/7/8/7/7，五阶段严重自身模型遮挡仍FAIL，不声称同C0/GPU或头显通过。正常清理true、无强杀，Native/entries/input恢复、XR及launcher正常退出，独立game/owner/XR/worker精确身份均不在；CLOAK624/失败0，前台393/owned_activation false。

path键Scene396/Compose24443/seq3679/world2310701312/pawn95226069120与complete首键逐项相同；engine/source_repeat/guard，Owner第19次context/source_age_expired/detail6，Session第3次collider_read/guard。实际原因见证entry14674320/Compose24443/thread33292/stage0/verdict3/causes1仍为body年龄拒绝、equipment当次未过期。原碰撞started/returned0/0，diagnostic小胶囊不证明完整身体或安全移动。

本query157934240187800→157934268685300共28.4975ms；计时invalid0/overflow0、所有span闭合。initial physics0.8626ms、collider3.1175ms、components15.5895ms；repeat physics3次共2.2219ms、collider3次共6.1059ms，components repeat0次，engine8.4879ms、closing0次。engine包含内部复核，不能重复相加；闭合不代表成功，计时开销包含在内，不作跨运行提速对照。这次实际测量将最大单段定位为initial component目录，尚未拆分其内部结构查找/动态读取/关闭成本，也不证明它是唯一或全部年龄原因。

源、动作、同query分段、原始退出和画面审阅记录在query-timing-r163/runtime-r164。下一窄点检查component root与schema重复认证，保留两遍动态读取、身份、完整证明及全部年龄门；不原样再跑计时候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口和实体头显状态不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,initial_components_ns=15589500,source_age_repaired=False)))
