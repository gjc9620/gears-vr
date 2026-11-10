"""Preserve actual current-Tick measurement progress and the next age refusal."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r148';root=R/'analysis/instances/cover-1111-r148-cloak-current'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and result['operator_exit']==0 and not result.get('error')
assert not result['remaining_owned'] and not result['forced_termination'] and result['anchor_sources_unchanged']
assert result['projection_frames']==321 and not result['projection_errors'] and result['projection_thread_joined']
assert result['cloak_observations']==dict(samples=860,failures=[])
assert result['foreground_observations']==dict(samples=534,owned_activation=False)
assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0 and result['isolated_done']['owned_game_absent']
mute=index['test_audio_guard']['mute_verification'];assert mute['verified_muted_observations']==340
assert mute['initially_muted_sessions']==1 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
for source,name in ((Path(index['session_report']),'session-original.json'),(root/'cover-output/run-result.json','run-result-original.json'),
    (root/'native/anchor-observer-config.json','anchor-config-original.json'),(root/'isolated-done.json','isolated-done-original.json'),
    (root/'isolated-start-request.json','first-start-request-original.json')):shutil.copyfile(source,out/name)
native=load(H/'native-build-result.json')['dll_sha256'];assert sha(root/'native/GearsCapture.dll')==native
sys.path[:0]=[str(H.parent/'core-measurement-r143'),str(H.parent/'guard-witness-r137')]
from inspect_core import inspect
from inspect_guard_probe import inspect as inspect_query
pose=inspect(session,native);query=inspect_query(session,native);save('pose-probe.json',pose);save('path-probe.json',query)
assert pose['complete_gate_observed'] and pose['query_attempted'] and not pose['core_witness'] and not pose['missing']
complete=next(r for r in pose['rows'] if r['name']=='complete');measurement=next(r for r in pose['rows'] if r['name']=='measurement')
assert complete==dict(step=15,name='complete',count=1,detail=0,scene=325,compose=20038,sequence=5513,world=2212528384,pawn=95434014848)
assert measurement['count']==257 and measurement['measurement_status']=='neutral'
raw=query['raw'];assert all(raw[k]=='1' for k in ('requested','attempted','finished'))
assert raw['code']=='4' and query['first_rejection_layer']=='session/component_roles'
assert query['owner_guard']==dict(calls=7,failed_call=7,stage=8,name='context',detail=3,extra=6,context_verdict='source_age_expired',context_detail=6)
assert query['session_guard']['calls']==0 and query['original_started']==query['original_returned']==0
assert raw['layout_step']=='40' and raw['layout_member_identities']=='225' and raw['layout_member_capacity']=='0'
for key in ('scene','compose','sequence','world','pawn'):assert int(raw[key])==complete[key]
elapsed=int(raw['end_ns'])-int(raw['begin_ns']);assert elapsed==35286800
assert session['native_status']['body_mount_source_enabled']=='1'
review=load(out/'media-review.json');assert len(review['pages'])==17 and len(review['contacts'])==5 and review['encode_exit']==0
for row in review['pages']:assert sha(Path(row['file']))==row['sha256']
for row in review['contacts']:
    assert sha(Path(row['file']))==row['sha256'] and not row['same_C0_proven']
    target=out/(row['label']+'.jpg');shutil.copyfile(row['file'],target);row['archived_file']=str(target)
review.update(full_span_personally_reviewed=True,reviewed_overview_pages=17,reviewed_original_contacts=5,
    cover_view_result='FAIL: head/shoulder/torso mesh obstructs both eyes in every contact phase',same_C0_proven=False)
save('media-review.json',review)
save('closeout.json',dict(classification='progress: current-Tick measurement completed and reached path attempt; original calls still refused',
    identities=identities,exact_owned_processes_absent=True,native_sha256=native,current_tick_source_enabled=True,
    completed_measurements=1,other_measurement_rejections=257,other_measurement_status='neutral; subpredicate unrecorded',
    same_C0_complete_and_query_key=True,query_requested=True,query_attempted=True,query_finished=True,query_code=4,
    first_rejection=query['first_rejection_layer'],owner_guard=query['owner_guard'],session_guard=query['session_guard'],
    query_elapsed_ns=elapsed,original_query_calls=0,age_cause_bits='not recorded: body/tracking versus equipment unproven',
    normal_cleanup_verified=True,cloak=result['cloak_observations'],foreground=result['foreground_observations'],mute=mute,
    disk_before=result['disk_before'],frames=321,reviewed_pages=17,reviewed_contacts=5,encode_exit=0,
    world_coverage_verified=False,movement_authorized=False,cover_view_accepted=False,public_changed=False,headset_accepted=False,
    remaining='Preserve same-call source_rejected cause bits for the private query prepare failure; keep all original age/identity/input gates. Do not rerun unchanged.'))
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
before=doc.read_bytes();assert before==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r148：当前Tick测量完成并到达path尝试，组件阶段上下文年龄拒绝（2026-11-11）

先保存68b81740及8e00075a：共享首次准入已支持锁外有界等待60秒，本方启动timeout从30兼容为90秒，并绑定isolated_sim/runtime_receipts新SHA；原名额2、内存、4GiB、身份、暂停检查仍由锁内及worker执行。本方advisory只对明确ResourceUnavailable转入共享等待，不吞其他错误；20项只读兼容检查通过。r147外层见空位后内层advisory被抢占，未产生claim/start-request/worker/index/game，原失败另存，使用新名r148。未修改旧prepared哈希或重用失败请求。

r148实际game54628/creation134388103051301203、owner44832/creation134388102957884329、XR9880/creation134388103149022211、worker35196/creation134388102956219331。Native49f76f01…1a472、冻结PG/XR不变；anchor initial58.0816ms，其余2.0393/5.7539/1.9463/2.0571ms均在原200ms内接受，八个anchor来源前后一致。共享工具SHAe6b4a0ae…42c2b/7db0a084…abefb按新准备记录核对，不替换成历史9f3502哈希。

body_mount_source_enabled1。原A进入CT2，测量complete1的Scene325/Compose20038/seq5513/world2212528384/pawn95434014848与path尝试键逐项相同。此前旧Core年龄门本轮没有记录为测量首败；另有measurement257/detail4=neutral，首证Scene327/Compose20163/seq5519，具体中立数学谓词尚未记录，不能混成同一个C0或称所有姿态都可用。

path requested/attempted/finished1，code4，Session stage4 component_roles/detail3；Owner guard第7次拒绝，context/source_age_expired/detail6，Session SourceGuard尚未进入。layout40/member225/未超容量，engine stage0，原始started/returned均0。query时间121595420167000→121595455453800，35.2868ms；不能拿这段自身耗时当原来源总年龄或判断body/equipment哪个过期。诊断capsule仍1raw半径、2raw高、1raw平移，不是完整身体/世界安全证明。V3测量接线这次确实到达后续消费者，但完整path查询仍未执行。

321实际projection帧/errors0，编码0；17页全跨度总览与5原始contact已亲看，五段模型头肩/躯干遮挡仍FAIL，时间段候选7/7/7/7/8，不声称同C0/GPU骨/头显证明。launcher0、Native/entry/input恢复、fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常、独立身份全不在、无强杀。CLOAK860/失败0、前台534/owned_activation false，静音340/initial1/失败0/共享拒绝0，disk_before87473856512。

修正报告reader将path attempted纳入query_attempted（之前只看sphere），9项解释自测通过；不是改运行行为或补造原调用。下一窄点复用现有source_rejected实际cause bits，区分prepare阶段本次body/tracking年龄与equipment年龄；当前commit-only journal不能证明该分项。不放宽时限或重取/续时旧输入，不原样重跑此批。源/原始结果/画面在current-measurement-r146/runtime-r148；完整CV01–24/AC01–22仍开放，工程25%、publicv71及固定入口不变，目标active。
'''
updated=before+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,completed_measurement=True,path_attempted=True,original_calls=0,normal_cleanup_verified=True)))
