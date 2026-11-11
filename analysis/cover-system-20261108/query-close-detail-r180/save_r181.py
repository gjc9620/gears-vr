"""Preserve age refusal before the repaired owner-reader was reached."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r181-cloak-close';out=H/'runtime-r181'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];q=load(out/'path-probe.json')
t=load(out/'timing-probe.json');a=load(out/'age-probe.json');links=load(out/'owner-links.json')
assert v['normal_cleanup_verified'] and v['operator_exit']==v['start_exit']==0 and not v.get('error')
assert not v.get('projection_error') and not v['projection_errors'] and v['projection_thread_joined']
assert not v['remaining_owned'] and not v['forced_termination'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['query_code_name']=='source' and q['original_started']==q['original_returned']==0
assert q['owner_guard']['failed_call']==78 and q['session_guard']['failed_call']==18
assert q['source_age_cause']['body_source_age'] and not q['source_age_cause']['equipment_age'] and not links['observed']
assert q['raw']['engine_stage']=='4' and t['elapsed_ns']==35594000
assert a['calls']==[78,78] and len(a['rows'])==4
assert set(a['classified_expired_fields'])=={'stereo_received','controller_received','sync'}
assert all(not x['expired_fields'] for x in a['rows'][:2])
assert a['rows'][2]['ages_ns']['stereo_received']==40386900 and a['rows'][2]['ages_ns']['metadata_begin']==36963000
spec=importlib.util.spec_from_file_location('r181_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(x for x in pose['rows'] if x['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert complete[k]==int(q['raw'][k])
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));boundary_data=load(boundary)
assert 'candidate_closing_rejection' not in boundary_data
assert load(H.parent/'isolated-r105/cover-1111-r181-cloak-close/operator-prepared.json')['candidate_closing_source_sha256']=='50979a3c824a1a51d87a250fa997de4e417dfc033405454029dc58c006041eca'
assert boundary_data['passed'] and boundary_data['after_original_A']['cover_state_raw']['CoverType']==2
assert len(boundary_data['eye_observation'])==5
review=load(out/'media-review.json')
assert review['frames']==265 and len(review['pages'])==14 and len(review['contacts'])==5 and review['encode_exit']==0
assert not review['recording_errors'] and not review['missing_contacts'] and review['captured_span_complete']
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=True,reviewed_pages=14,
    reviewed_original_contacts=5,verdict='All five actual cover phases have severe self-model obstruction; FAIL',cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json')):shutil.copyfile(source,out/name)
shutil.copyfile(root/'cover-output/projection/recording.json',out/'projection-recording-original.json')
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,runner_exit=0,
    native_sha256=q['native_sha256'],same_C0_complete_query_key=True,query_elapsed_ns=t['elapsed_ns'],
    source_age_cause=q['source_age_cause'],source_age_observation=a,owner_links=links,
    repaired_owner_reader_reached=None,owner_links_game_repaired=False,source_age_repaired=False,
    original_query_started=0,original_query_returned=0,cover_view_accepted=False,world_coverage_verified=False,
    movement_authorized=False,headset_accepted=False,public_changed=False,
    next='Reduce repeated full-proof read cost without changing source timestamps, guards or closing proofs; no unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r181' not in old
entry='\n## r181：组件联合扫描候选实际query仍超40ms；原候选收尾失败未重现（2026-11-11）\n\n带r180具体closing差异记录的新cover-1111-r181-cloak-close使用原r177 Native de07b0c1…01b8f1。prepare/导入/启动0，原A进入CT2、五阶段及operator/runner0；候选严格收尾通过，没有candidate_closing_rejection。r179失败本次未重现，不把记录层当成原因修复。生成器回执绑定50979a3c…41eca。正常cleanup true、remaining_owned空、无强杀；保存时game/owner/XR/worker精确身份独立确认不在。CLOAK607/失败0，前台374/owned_activation false，13启动来源前后相同。启动前另对r179已结束精确身份核验后，将105 PSOCache文件885049318字节校验归档J:/gearsVr/analysis/cover-cache-archive-20261111-r180，只移除该缓存，源码/文本数值/媒体/存档/配置保持。\n\nquery键world2224455936/Pawn95203878528/Scene403/Compose24829/seq3512/thread23612与complete首键一致；171869734261500→171869769855500共35.5940ms。code4 source、engine/source_guard/current，Owner78/context/source_age_expired/detail6，Session18/collider_read/guard，body=true/equipment=false，original started/returned0/0，link witness未观察仍未知。\n\n首次classify比query begin晚0.6801ms，body Tick24828、追踪seq3514与query分别保留；首次tracking1.4236ms/stereo5.4775ms/metadata2.0536ms，current capture1.1743ms。首次age时stereo40.3869ms、controller40.3511ms、sync40.3787ms超原40ms；tracking36.3330ms/metadata36.9630ms/capture36.0818ms未超，latched=false。没有续时间或扩大时限。\n\ninitial physics1.2492ms/collider2.9179ms/components3.3837ms，重复physics18次12.3993ms/collider18次14.3040ms/components0次；engine27.0113ms含内部复核，不重复相加，全部span闭合无invalid/overflow。首次components比r1769.7828ms低，但输入/调度不同，不把时间差全归因于优化；总query仍35.5940ms，真实超时未修复。下一步审查18次重复身份/目录/完整Proof读取的具体成本，在保持所有原始时间、动态字段和closing语义下减少重复工作；不原样重跑挑一轮。\n\n265投影帧/errors0/编码0，14页全跨度概览及五阶段原始00181/00200/00220/00238/00257全部亲看；中立/转头/恢复/上移/恢复仍严重自身模型遮挡，视图FAIL。区间帧不是同C0/GPU原子证明或头显验收。原始session、动作、projection数值清单、年龄、分段及审阅在query-close-detail-r180/runtime-r181。完整world/body几何、movement、CV01–24/AC01–22和实体头显未完成，工程25%、publicv71/固定入口不变，目标active。\n'
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,repaired_owner_reader_reached=None,source_age_repaired=False)))
