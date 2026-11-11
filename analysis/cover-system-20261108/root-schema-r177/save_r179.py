from pathlib import Path
import json,hashlib,datetime,sys,shutil
H=Path('analysis/cover-system-20261108/root-schema-r177').resolve();R=H.parents[2];out=H/'runtime-r179';root=R/'analysis/instances/cover-1111-r179-cloak-root'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session']
assert v['normal_cleanup_verified'] and v['operator_exit']==1 and v['start_exit']==0 and not v['remaining_owned'] and not v['forced_termination']
assert v['anchor_sources_unchanged'] and len(v['anchor_source_sha256'])==13 and not v['cloak_observations']['failures'] and not v['foreground_observations']['owned_activation']
assert not v['projection_errors'] and v['projection_frames']==136
q=load(out/'timing-probe.json');assert q['query_proven'] is False and 'No terminal query' in q['interpretation_error']
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));b=load(boundary)
assert not b['passed'] and 'World-state metadata/registry changed' in b['error'] and 'after_original_A' not in b and not b.get('eye_observation')
sys.path.insert(0,str(R/'tools/native-capture'));from run_stereo_continuous import WindowsControl
control=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')};ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert control.identity(identity['pid'])!=identity
review=load(out/'media-review.json');assert review['frames']==136 and len(review['pages'])==7 and not review['contacts'] and review['encode_exit']==0
for item in review['pages']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=False,reviewed_pages=7,reviewed_original_contacts=0,captured_span_complete=False,expected_phase_count=5,completed_phase_count=0,cover_action_span_complete=False,missing_contacts=[dict(label=x,reason='Operator rejected during candidate selection before original A; phase not reached') for x in ('cover_eye_neutral','cover_eye_turned','cover_eye_turn_restored','cover_eye_raised','cover_eye_rise_restored')],verdict='Captured pre-entry view reviewed; no cover phase reached, no cover-view acceptance',cover_view_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),(boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json'),(root/'cover-output/operator.log','operator-original.log')):
 if source.exists():shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,runner_exit=1,original_A_reached=False,query_proven=False,query_missing_reason=q['interpretation_error'],root_schema_game_exercised=False,source_age_repaired=False,world_coverage_verified=False,movement_authorized=False,cover_view_accepted=False,headset_accepted=False,public_changed=False,first_failure='Candidate CoverQuery.finish: World-state metadata/registry changed at closing boundary; detailed existing closing_rejection was not saved',next='Preserve the already-read closing rejection in the isolated cover operator without changing strict finish or retrying stale queries.')
(out/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
refused=R/'analysis/instances/cover-1111-r178-cloak-root'
for name in ('launcher-session.json','isolated-worker.json','isolated-start-request.json'):assert not (refused/name).exists()
assert not (refused/'cover-output/claim.json').exists()
(H/'r178-preflight-refusal.json').write_text(json.dumps(dict(name=refused.name,stage='runner require_disk before claim',free_bytes=3938926592,minimum_free_bytes=4294967296,runner_exit=1,game_started=False,xr_started=False,worker_started=False,claim_consumed=False,first_start_request_created=False,source='Observed tool return and independently checked absence of attempt files; no fabricated terminal session'),indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r179' not in old
entry='''
## r179：新候选在原A前候选读取收尾拒绝，无终态碰撞查询（2026-11-11）

r177接入--native-r177后，仅选择精确一文件delta/原413依赖和原flags，source/build保持。归档正常结束且独立精确身份不在的r176 PSOCache105文件885049318字节到J盘，全部SHA一致后仅移除缓存文件；853791d6保存。r178准备和导入0，但runner前置require_disk实际3938926592字节小于原4294967296，在claim/worker/start请求前拒绝；各attempt文件独立确认不存在，不复用该身份。随后同样核对已结束r166/r168，将210缓存文件1770098636字节逐文件校验归档J:/gearsVr/analysis/cover-cache-archive-20261111-r178；源码/文本数值/媒体/存档/配置及其他实例保持。

新cover-1111-r179-cloak-root、Native de07b0c1…01b8f1 prepare/导入/启动0，启动前4768755712字节通过原磁盘门槛。实际game58696、creation134388601048933864，observer在cover_boundary_chain候选CoverQuery.finish处抛World-state metadata/registry changed at closing boundary，operator/runner1。原A与五阶段未达，严格reader无终态query。没有components耗时或源码修复游戏证明，不将No terminal query伪装0次原调用的已测计数；未观察到的首项身份/字段/registry差异仍未知。

Normal cleanup true，Native/entries/input与XR正常退出、remaining_owned空、无强杀；保存时独立确认game/owner/XR/worker精确身份不在。13项启动来源前后相同，CLOAK499/失败0、前台330/owned_activation false。136投影帧/errors0/编码0，7页实际概览全部亲看，仅覆盖进入掩体前场景；五阶段及contact0，明确标记动作跨度不完整，不作掩体视图验收。

发现Query.finish已记录closing_rejection的before/after，但外层只保存通用异常，具体首差异丢失。下一步仅在本chat的隔离operator生成时保存这份已有差异并重抛原异常；不放松registry变化、不续期、不跳过候选、不使用失败观察或新增重试。先补已存在证据的落盘，再有依据地复查，不原样重跑。r177实际碰撞/40ms、自身模型遮挡、CV01–24/AC01–22与头显未完成，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,query_proven=False,exact_owned_processes_absent=True)))
