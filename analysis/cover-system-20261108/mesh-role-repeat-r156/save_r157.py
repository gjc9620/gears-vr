"""Keep the actual continued body-age failure after the read-count repair."""
from pathlib import Path
import datetime,hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];root=R/'analysis/instances/cover-1111-r157-cloak-meshrole';out=H/'runtime-r157'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];index=load(root/'launcher-session.json')
assert v['operator_exit']==0 and not v.get('error') and v['normal_cleanup_verified'] and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==277 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==10
native=load(H/'native-build-result.json')['dll_sha256'];assert sha(root/'native/GearsCapture.dll')==native
q=load(out/'path-probe.json');pose=load(out/'pose-probe.json');raw=q['raw'];cause=q['source_age_cause']
assert cause['observed'] and cause['causes']==1 and cause['body_source_age'] and not cause['equipment_age']
assert cause['compose']==24437 and cause['thread']==25376
assert q['owner_guard']['failed_call']==15 and q['owner_guard']['context_detail']==6
assert q['session_guard']['failed_call']==2 and q['session_guard']['name']=='collider_read'
assert q['first_rejection_layer']=='engine/source_begin/guard' and raw['code']=='4'
assert q['original_started']==q['original_returned']==0
elapsed=int(raw['end_ns'])-int(raw['begin_ns']);assert elapsed==32449700
complete=next(row for row in pose['rows'] if row['name']=='complete')
for key in ('scene','compose','sequence','world','pawn'):assert int(raw[key])==complete[key]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
review=load(out/'media-review.json');assert len(review['pages'])==14 and len(review['contacts'])==5 and review['encode_exit']==0
for p in review['pages']:assert sha(Path(p['file']))==p['sha256']
for contact in review['contacts']:
    assert sha(Path(contact['file']))==contact['sha256'] and not contact['same_C0_proven']
    target=out/(contact['label']+'.jpg');shutil.copyfile(contact['file'],target);contact['archived_file']=str(target)
review.update(full_span_personally_reviewed=True,reviewed_pages=14,reviewed_original_contacts=5,
    verdict='All five actual phases retain severe self-model obstruction; FAIL',cover_view_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
for source,name in ((root/'cover-output/run-result.json','run-result-original.json'),(Path(index['session_report']),'session-original.json'),
                    (boundary,'cover-boundary-original.json'),(boundary.parent/'equipment-chain.json','equipment-chain-original.json'),
                    (boundary.parent/'recording.json','action-recording-original.json'),(root/'isolated-done.json','isolated-done-original.json'),
                    (root/'isolated-start-request.json','first-start-request-original.json'),(root/'launcher-session.json','launcher-index-original.json')):shutil.copyfile(source,out/name)
dependency=load(H/'r157-startup-dependency-observation.json');dependency['after_sha256']=sha(Path(dependency['source']))
assert dependency['after_sha256']==dependency['sha256']
dependency['observed_before_after_equal']=True;dependency['exact_loaded_source_proven']=False
(out/'startup-dependency-observation.json').write_text(json.dumps(dependency,indent=2)+'\n')
summary=dict(scope='Actual new Native still rejected body source age before original query',native_sha256=native,identities=identities,
    exact_owned_processes_absent=True,normal_cleanup_verified=True,original_low_cover_entered=True,
    source_age_cause=cause,owner_guard=q['owner_guard'],session_guard=q['session_guard'],first_rejection=q['first_rejection_layer'],
    query_elapsed_ns=elapsed,query_code=4,original_query_started=0,original_query_returned=0,same_C0_complete_query_key=True,
    latency_improvement_proven=False,reason='r155 and r157 failed at different stages; source age at entry and scheduling are not matched',
    frames=277,reviewed_pages=14,contacts=5,encode_exit=0,cloak=v['cloak_observations'],foreground=v['foreground_observations'],disk_before=v['disk_before'],
    shared_startup_publication_change_observed=dependency,world_coverage_verified=False,movement_authorized=False,
    source_age_repaired=False,cover_view_accepted=False,headset_accepted=False,public_changed=False,
    next='Inspect collider/source repeated reads and source-to-query scheduling; no unchanged rerun or age relaxation. Check disk before next admission.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r157：新去重候选实际CT2通过、collider复核仍body年龄拒绝（2026-11-11）

eb8c46b5源码/构建保存后，以新cover-1111-r157-cloak-meshrole、Native ababb095…f2d8及冻结PG/XR，既有runner共享600秒准入做一次定向运行。实际game10688/creation134388186580061229、owner3196/creation134388186507862975、XR56032/creation134388186649960118、worker25076/creation134388186506656610。十项准备时anchor/启动来源前后相同。原A进入CT2、五段eye观察与operator0，Native/entry/input/武器正常恢复，XR0、launcher0、正常清理true、无强杀；独立精确进程身份均不在。

complete2首键Scene396/Compose24437/seq3244/world2321973504/pawn95227484800与本path逐项相同；另measurement266/detail4 neutral首键Scene400/Compose24682/seq3252，不能混合。path code4，first engine/source_begin/guard；Owner第15次context/source_age_expired/detail6，Session第2次collider_read/detailguard，engineStage1/sourceCode2。匹配原因entry14674320/Compose24437/thread25376/stage0/verdict3/causes1，仍body_source_age true/equipment_age false，原started/returned0/0。query129914469800400→129914502250100，32.4497ms；与r155的35.9755ms处在不同拒绝阶段且开始年龄/调度未匹配，不能据此称实际全query提速，更不能称年龄故障修复。

277实际投影帧/errors0，编码0；14页全跨度及5原始contact均亲看，阶段候选8/7/7/7/8，五段自身头肩/躯干遮挡仍FAIL，图像哈希与原图副本保留，不声称同C0/GPU或头显证明。CLOAK590/失败0，前台373/owned_activation false；disk_before5430648832，仍过原4GiB门但余量不多，下一运行前须重新检查，不删他人或未清理实例。

等待期间收到共享launch_reserve.py出生回执原子发布窄修正通知：最多6次同payload替换，仅Win5/32/33、重试前整份原回执相等，非启动/注入重试；本方未改该共享文件，未回复/转发或为此新增运行。在本方worker登记前另记source SHA a4c44d9d…99edf及对应只读证据，结束后源文件SHA相同；不改旧prepared哈希，不把源码前后观察当作精确已加载版本或真实Win5修复验证。此后新prepare将launch_reserve作为第11项启动来源绑定，当前r157仍明确是原10项加独立观察。

原始session/launcher/动作/新artifact解释/媒体/关闭结果在mesh-role-repeat-r156/runtime-r157。已证明的合成读取量收益保留，实际来源年龄与画面缺口仍在；下一窄点继续检查collider/source复核成本及来源到query调度，不放宽新鲜度、不刷新旧样本、不原样重跑。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及实体头显状态不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,body_age_repaired=False,reviewed_pages=14,contacts=5)))
