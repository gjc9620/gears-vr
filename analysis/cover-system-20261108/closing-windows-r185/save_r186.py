"""Preserve age refusal before the repaired owner-reader was reached."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r186-cloak-close';out=H/'runtime-r186'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];q=load(out/'path-probe.json')
t=load(out/'timing-probe.json');a=load(out/'age-probe.json');links=load(out/'owner-links.json')
assert v['cover_reader_sources_unchanged'] and len(v['cover_reader_source_sha256'])==2
assert v['normal_cleanup_verified'] and v['operator_exit']==v['start_exit']==0 and not v.get('error')
assert not v.get('projection_error') and not v['projection_errors'] and v['projection_thread_joined']
assert not v['remaining_owned'] and not v['forced_termination'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['query_code_name']=='source' and q['original_started']==q['original_returned']==0
assert q['owner_guard']['failed_call']==8 and q['session_guard']['failed_call']==1
assert q['source_age_cause']['body_source_age'] and not q['source_age_cause']['equipment_age'] and not links['observed']
assert q['raw']['engine_stage']=='1' and t['elapsed_ns']==14493800
assert a['calls']==[8,8] and len(a['rows'])==4
assert set(a['classified_expired_fields'])=={'stereo_received','controller_received','sync'}
assert all(not x['expired_fields'] for x in a['rows'][:2])
assert a['rows'][2]['ages_ns']['stereo_received']==40255100 and a['rows'][2]['ages_ns']['metadata_begin']==16647400
spec=importlib.util.spec_from_file_location('r186_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(x for x in pose['rows'] if x['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert complete[k]==int(q['raw'][k])
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));boundary_data=load(boundary)
assert 'candidate_closing_rejection' not in boundary_data
assert load(H.parent/'isolated-r105/cover-1111-r186-cloak-close/operator-prepared.json')['candidate_closing_source_sha256']=='50979a3c824a1a51d87a250fa997de4e417dfc033405454029dc58c006041eca'
assert boundary_data['passed'] and boundary_data['after_original_A']['cover_state_raw']['CoverType']==2
assert len(boundary_data['eye_observation'])==5
review=load(out/'media-review.json')
assert review['frames']==311 and len(review['pages'])==16 and len(review['contacts'])==5 and review['encode_exit']==0
assert not review['recording_errors'] and not review['missing_contacts'] and review['captured_span_complete']
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=True,reviewed_pages=16,
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
summary=load(out/'closeout.json');summary.update(cover_reader_source_sha256=v['cover_reader_source_sha256'],cover_reader_sources_after=v['cover_reader_sources_after'],cover_reader_sources_unchanged=v['cover_reader_sources_unchanged'])
summary.update(candidate_closing_failure_reproduced=False,registry_race_game_repaired=False)
summary=summary;summary.update(identities=ids,exact_owned_processes_absent=True,runner_exit=0,
    native_sha256=q['native_sha256'],same_C0_complete_query_key=True,query_elapsed_ns=t['elapsed_ns'],
    source_age_cause=q['source_age_cause'],source_age_observation=a,owner_links=links,
    repaired_owner_reader_reached=None,owner_links_game_repaired=False,source_age_repaired=False,
    original_query_started=0,original_query_returned=0,cover_view_accepted=False,world_coverage_verified=False,
    movement_authorized=False,headset_accepted=False,public_changed=False,
    next='Reduce repeated full-proof read cost without changing source timestamps, guards or closing proofs; no unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r186' not in old
entry='\n## r186：组合候选进入掩体，首次来源已26.4991ms，query仍在原调用前过40ms（2026-11-11）\n\nfeb1f0e4保存后，新cover-1111-r186-cloak-close使用精确r183 Native0d4437fd…cdf6f及r185窗口收尾reader。prepare/导入/启动、原A进入CT2、五阶段与operator/runner全部0。候选严格收尾通过，r184 count增一未重现，不以本轮通过宣称目录race因果修复。两份cover_state/world_state来源SHA在claim前/清理后相同，原13项来源也相同；启动前9291214848字节通过原门槛。CLOAK731/失败0、前台442/owned_activation false。normal_cleanup true、remaining_owned空、无强杀；保存时game/owner/XR/worker精确身份独立确认不在。\n\nquery键world2252701952/Pawn95181176960/Scene318/Compose19580/seq4319/thread54900与complete首键一致；174581640226000→174581654719800共14.4938ms。code4 source、engine/source_begin/guard；Owner第8次context/source_age_expired/detail6，Session第1次physics_read/guard，body=true/equipment=false，original started/returned0/0；link witness未观察仍未知。首次重复physics在其Owner前置检查即拒绝，本次未到后续typed owner组。\n\n身体Tick19579、追踪seq4320与query分别保存。首次classify在query begin后0.7327ms，此时tracking2.1943ms、stereo26.4991ms、controller26.2785ms、sync26.3101ms、metadata2.8914ms；最旧stereo距原40ms仅13.5009ms。按同一原时间相减，query begin时该stereo已25.7664ms，可用14.2336ms，实际query14.4938ms，不能把全部26.5ms算成query新耗时。首次age时stereo40.2551ms、controller40.0345ms、sync40.0661ms超原40ms；tracking15.9503ms、metadata16.6474ms、current capture15.6549ms未超，latched=false。没有重写时间或扩大时限。\n\ninitial physics1.8795ms/collider4.5246ms/components5.4962ms；physics_repeat1次0.2104ms、collider/components repeat0次，engine0.5644ms含内部复核，不重复相加，span完整无invalid/overflow。query比先前短不代表性能通过，因为更早失败且初始输入更旧；r183优化的完整query仍未验证。下一点应同时考虑来源到查询的延迟和嵌套完整Owner复核成本，不能挑较新输入重跑、延长40ms、续期或跳过中断/身份/closing。\n\n311投影帧/errors0/编码0，16页全跨度概览及五阶段原始00221/00241/00262/00284/00304全部亲看，仍严重自身模型遮挡，视图FAIL。区间帧不是同C0/GPU原子证明或头显验收。原始session/动作/projection数值清单、年龄/分段和审阅在closing-windows-r185/runtime-r186。world/body几何、movement、全CV01–24/AC01–22及实体头显未完成，工程25%、publicv71/固定入口不变，目标active。\n'
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,repaired_owner_reader_reached=None,source_age_repaired=False)))
