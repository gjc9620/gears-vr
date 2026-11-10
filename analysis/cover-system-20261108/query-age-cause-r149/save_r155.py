"""Preserve matched body-age refusal, actual timed walking and failed view."""
from pathlib import Path
import datetime,hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r155';root=R/'analysis/instances/cover-1111-r155-cloak-start'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];index=load(root/'launcher-session.json')
assert v['operator_exit']==0 and not v.get('error') and v['normal_cleanup_verified'] and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==259 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert v['admission_wait_seconds']==600 and len(v['anchor_source_sha256'])==10
q=load(out/'path-probe.json');pose=load(out/'pose-probe.json');raw=q['raw'];cause=q['source_age_cause']
assert cause['observed'] and cause['causes']==1 and cause['body_source_age'] and not cause['equipment_age']
assert cause['compose']==24726 and cause['thread']==55064 and cause['stage']==0 and cause['verdict']==3
assert q['owner_guard']['failed_call']==21 and q['owner_guard']['context_detail']==6
assert q['session_guard']['failed_call']==4 and q['first_rejection_layer']=='engine/source_repeat/guard'
assert q['original_started']==q['original_returned']==0 and raw['code']=='6'
elapsed=int(raw['end_ns'])-int(raw['begin_ns']);assert elapsed==35975500
complete=next(row for row in pose['rows'] if row['name']=='complete')
for key in ('scene','compose','sequence','world','pawn'):assert int(raw[key])==complete[key]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
events=load(boundary.parent/'recording.json')['events'];pulses=[e for e in events if e['label']=='cover_ordinary_pulse_released']
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
assert len(pulses)==3 and all(p['planned_seconds']==.32 and .327<p['actual_seconds']<.329 for p in pulses)
assert index['vr_gamepad']['focus_gate_attempts']==1 and not index['vr_gamepad']['focus_gate']['startup_thread_exit_retries']
review=load(out/'media-review.json');assert len(review['pages'])==13 and len(review['contacts'])==5 and review['encode_exit']==0
for p in review['pages']:assert sha(Path(p['file']))==p['sha256']
for contact in review['contacts']:
    assert sha(Path(contact['file']))==contact['sha256'] and not contact['same_C0_proven']
    target=out/(contact['label']+'.jpg');shutil.copyfile(contact['file'],target);contact['archived_file']=str(target)
review.update(full_span_personally_reviewed=True,reviewed_pages=13,reviewed_original_contacts=5,
    verdict='All five actual phase contacts retain severe self head/shoulder/torso obstruction; FAIL',cover_view_accepted=False)
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
summary=dict(scope='Actual timed approach and matched prepare body-source-age refusal',identities=identities,
    exact_owned_processes_absent=True,normal_cleanup_verified=True,original_low_cover_entered=True,pulses=pulses,
    startup_gate_attempts=1,startup_retry_branch_exercised=False,source_age_cause=cause,owner_guard=q['owner_guard'],session_guard=q['session_guard'],
    query_elapsed_ns=elapsed,query_code=6,original_query_started=0,original_query_returned=0,same_C0_complete_query_key=True,
    body_subclock='Unknown: body tracking, metadata, capture or latched age not separated by this witness',
    earlier_r138_r148_cause='Unknown; this witness only proves r155',frames=259,reviewed_pages=13,contacts=5,encode_exit=0,
    cloak=v['cloak_observations'],foreground=v['foreground_observations'],disk_before=v['disk_before'],
    world_coverage_verified=False,movement_authorized=False,cover_view_accepted=False,headset_accepted=False,public_changed=False,
    next='Inspect repeated physics/source guard read cost and source-to-query schedule; retain age/identity/closing checks, do not rerun unchanged candidate.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r155：定时步行与原CT2进入通过，同查询明确body年龄拒绝、equipment未过期（2026-11-11）

在d264443c保存后，新r155直接调用既有runner --admission-wait-seconds600；共享锁外等待/锁内原准入最终接受，未复用r154请求或放宽名额、内存/磁盘。Native ac5cf37f…ebda及冻结PG/XR不变，10项anchor/启动来源前后相同。实际game58564/creation134388167581202750、worker47468/creation134388167497036032。后台输入门首次成功、attempts1、startup_thread_exit_retries空，正常启动链通过，但未实测新87重试分支。

三个普通步行脉冲planned0.320秒，actual均约0.328秒、samples12/13/13、overrun约8ms，随后原版A确实进入CT2，五段eye观察完成，operator0。新before_A观察记录(-3963.17554,-139.47527,579)、yaw -5792、CT0，实际读取+保存窗口16.8663ms；这个额外耗时仍明确保留，不宣称与旧运行完全同序。该次限定接近/入口已触达，不代表所有路径或掩体玩法通过。

complete2首键Scene400/Compose24726/seq3953/world2248507648/pawn16826695936与path键逐项相同；另measurement334/detail4=neutral首键Scene404/Compose24971/seq3961，不能混成同C0或判定全部测量通过。path requested/attempted/finished，code6；Owner第21次context/source_age_expired/detail6，Session第4次physics_read/guard，实际第一拒绝层engine/source_repeat/guard；engineStage3/sourceCode2，原started/returned0/0。时间128023511475700→128023547451200，共35.9755ms。

本次匹配prepare原因见证observed1/entry14674320/Compose24726/thread55064/stage0/verdict3/causes1：body_source_age true、equipment_age false。这是同查询实际上下文回调，确定本次body年龄分支拒绝且equipment在该次检查仍新鲜；不能细分tracking40ms、metadata50ms、capture或已烧毁age，也不能倒推r138/r148相同分项。仍不刷新旧输入/时间、不放宽年龄门。下一窄点为复核重复physics/source读取开销及source至query调度，保留完整关闭检查；不原样重跑此候选。

259实际投影帧/errors0，编码0；13页全跨度与5原始contact均亲看，候选7/7/7/8/7，五段自身头肩/躯干遮挡仍明显FAIL，接触图另存哈希，不声称同C0/GPU或头显验收。Native/entries/input恢复、fire_disable0/hand_stop5/drained、XR0、launcher0、normal_cleanup true，无强杀，独立精确身份全部不在；CLOAK660/失败0、前台415/owned_activation false，disk_before14328074240。原始session/launcher/操作/查询/媒体在query-age-cause-r149/runtime-r155。完整CV01–24/AC01–22、工程25%、publicv71及固定入口仍不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,body_age=True,equipment_age=False,normal_cleanup_verified=True,reviewed_pages=13,contacts=5)))
