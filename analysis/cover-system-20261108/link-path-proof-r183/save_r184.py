from pathlib import Path
import json,hashlib,datetime,sys,shutil
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r184';root=R/'analysis/instances/cover-1111-r184-cloak-path'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];assert v['normal_cleanup_verified'] and v['operator_exit']==1 and v['start_exit']==0 and not v['remaining_owned'] and not v['forced_termination']
assert v['anchor_sources_unchanged'] and len(v['anchor_source_sha256'])==13 and not v['cloak_observations']['failures'] and not v['foreground_observations']['owned_activation'] and not v['projection_errors'] and v['projection_frames']==145
assert sha(root/'native/GearsCapture.dll')=='0d4437fd5427902f84e5571cb1e3f577c201b114bfaf2843b3e4483f998cdf6f'
q=load(out/'timing-probe.json');assert q['query_proven'] is False and 'No terminal query' in q['interpretation_error']
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));b=load(boundary);assert not b['passed'] and 'after_original_A' not in b and not b.get('eye_observation')
failure=b['candidate_closing_rejection'];d=failure['difference'];assert d['kind']=='registry' and not failure['difference_missing_is_unknown']
before=d['before_header'];after=d['after_header'];assert before['data']==after['data']==372178944 and before['capacity']==after['capacity']==225272 and before['count']==218209 and after['count']==218210
sys.path.insert(0,str(R/'tools/native-capture'));from run_stereo_continuous import WindowsControl
control=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')};ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert control.identity(identity['pid'])!=identity
review=load(out/'media-review.json');assert review['frames']==145 and len(review['pages'])==8 and not review['contacts'] and review['encode_exit']==0
for item in review['pages']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=False,reviewed_pages=8,reviewed_original_contacts=0,captured_span_complete=False,cover_action_span_complete=False,expected_phase_count=5,completed_phase_count=0,cover_view_accepted=False,verdict='Pre-cover captured span reviewed; original A and all five cover phases not reached')
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),(boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json'),(root/'cover-output/operator.log','operator-original.log'),(root/'cover-output/projection/recording.json','projection-recording-original.json')):
 if source.exists():shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,runner_exit=1,native_sha256=sha(root/'native/GearsCapture.dll'),original_A_reached=False,query_proven=False,link_path_proof_game_exercised=False,source_age_repaired=False,world_coverage_verified=False,movement_authorized=False,cover_view_accepted=False,headset_accepted=False,public_changed=False,first_failure=failure,next='Reduce duplicate read-only candidate observation work with unchanged strict registry/identity/metadata/value closing; do not enable append-only permission or rerun unchanged.')
(out/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r184' not in old
entry='''
## r184：新候选在原A前严格拒绝注册表增一，已取得精确前后值（2026-11-11）

c9b8ba8b保存后，新cover-1111-r184-cloak-path使用r183 Native0d4437fd…cdf6f，prepare/导入/启动0。实际game33600、creation134388623588570950，候选CoverQuery.finish拒绝，operator/runner1；原A/五阶段未达，严格reader没有终态query。因此没有r183继承优化的实际query耗时，不能声称40ms已修复或原查询实测0次。

r180记录取得首个差异：candidate link2303863648，stage candidate_selection_before_original_A；registry地址5398218544，16字节before00002f160000000061540300f86f0300→after00002f160000000062540300f86f0300，data372178944/capacity225272保持，count218209→218210，missing-is-unknown=false。只能确认此注册表头的这一差异，未取得新增项的语义，不能证明与候选无关。严格原检查拒绝未变化，未开启append-only放行或复用失败证据。

normal_cleanup true、remaining_owned空、无强杀，保存时独立确认game/owner/XR/worker精确身份不在。CLOAK546/失败0，前台352/owned_activation false，13启动来源前后相同。145投影帧/errors0、编码0，8页实际已有跨度全部亲看，仅进入掩体前场景；contact0/阶段0，明确动作跨度不完整，不作视图或头显验收。原始session/差异/动作/投影清单及review在link-path-proof-r183/runtime-r184。

下一步缩短候选观察中的重复读取，尤其数组头/完整Slots重复采样、逐槽Bool mask重复读取和收尾的重叠数值窗口；必须保持相同身份/字段/目录/slot值的严格收尾和时间/容量限制，不延时等待或跳过变动候选。完整world/body/movement、严重模型遮挡、全CV01–24/AC01–22及头显未完成，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(saved=True,normal_cleanup_verified=True,query_proven=False,exact_owned_processes_absent=True,registry_difference=d)))
