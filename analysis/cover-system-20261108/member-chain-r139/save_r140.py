"""Preserve pre-activation startup rejection separately from candidate behavior."""
from pathlib import Path
import datetime,hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r140'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def save(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
root=R/'analysis/instances/cover-1111-r140-cloak-chain'
result=load(root/'cover-output/run-result.json');index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['start_exit']==0 and not result['normal_cleanup_verified'] and not result['remaining_owned']
assert result['isolated_done']['launcher_exit']==1 and result['isolated_done']['owned_game_absent']
assert not result['forced_termination'] and not result['isolated_done']['forced_termination']
assert session['native_started'] is False and session['native_cleanup_verified'] is False and session['xr_exit_code']==1
assert session['vr_gamepad_cleanup']['restored'] and not session.get('entry_restore_verified')
assert not result.get('operator_identity') and not result.get('projection_directory') and not result.get('projection_frames')
assert result['cloak_observations']==dict(samples=405,failures=[])
source=Path(index['session_report']);saved=out/'session-original.json'
if saved.exists():assert sha(saved)==sha(source)
else:shutil.copyfile(source,saved)
assert load(saved)['native_status'] is None
failed=next((root/'native').glob('anchor-observer-config-failed-*.json'));config=load(failed)
assert config['accepted'] is False and config['native_activation_attempted'] is False and config['failure_stage']=='initial'
attempt=config['guard_acquisitions'][0]['attempts'][0]
assert config['strict_guard_attempts']==1 and attempt['strict_guard_accepted'] and attempt['elapsed_ms']==3536.1894
scope=config['guard_acquisitions'][0]
assert scope['deadline_perf_counter_ns']-scope['started_perf_counter_ns']==200000000
assert scope['failure_reason']=='Anchor guard acquisition deadline exceeded'
shutil.copyfile(failed,out/'config-failure-original.json')
native=load(H/'native-build-result.json')['dll_sha256']
assert sha(root/'native/GearsCapture.dll')==native
mute=index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations']==367 and mute['initially_muted_sessions']==1 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
save('closeout.json',dict(classification='progress: preserved exact startup rejection before Native activation; optimization game behavior untested',
    identities=identities,exact_owned_processes_absent_at_closeout=True,native_sha256=native,
    original_report=str(source),original_report_sha256=sha(saved),config_failure=str(failed),config_failure_sha256=sha(failed),
    native_activation_attempted=False,native_started=False,candidate_code_executed=False,operator_executed=False,
    first_failure='initial read-only anchor guard acquisition exceeded 200ms; first strict sample took3536.1894ms',
    strict_guard_accepted=True,acquisition_accepted=False,original_query_calls=0,gameplay_projection_recorded=False,
    normal_cleanup_verified=False,native_cleanup_verified=False,native_cleanup_error=session['native_cleanup_error'],
    entry_restore_verified=session.get('entry_restore_verified'),pad_restored=True,xr_exit_code=1,launcher_exit_code=1,
    forced_termination=False,stale_cleanup_pending_receipt_preserved=True,final_done_owned_game_absent=True,
    cloak=result['cloak_observations'],mute=mute,disk_before=result['disk_before'],
    collision_clear=None,world_coverage_verified=False,movement_authorized=False,public_changed=False,headset_accepted=False,
    remaining=['reduce or explain actual startup acquisition cost without extending freshness/deadline or reusing old data',
        'actual r139 original path/endpoint and unchanged source-age checks',
        'complete body geometry, current world coverage, same-pose C0 body/view/hit/muzzle',
        'head/shoulder obstruction, CV01-24 AC01-22 headset']))
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r140：激活前严格anchor采集超时，优化Native未执行（2026-11-11）

r139源码及离线读量先保存b7a9f8cd；新票cover-1111-r140-cloak-chain资源准入后启动，game34692/creation134388043499140447、owner39240/creation134388043409774082、XR28276/creation134388043703130168、worker13816/creation134388043408916129。启动disk_before5821665280≥4GiB。候选8814ea68…cc32b按SHA复制正确，但激活前只读anchor配置采集在initial获取拒绝，native_activation_attempted=false、native_started=false、native_status=null，未执行operator或记录私有玩法projection，原查询0。不能将此说成成员链缓存或路径逻辑运行失败，也不能声称优化已在游戏通过。

原始config-failed回执显示单次严格采集strict_guard_accepted=true、无语义异常，但耗时3536.1894ms；本次获取的原deadline为200ms，故acquisition accepted=false，理由Anchor guard acquisition deadline exceeded。计时115625775453000→115629311651100ns，拒绝发生在注入候选Native之前。未延长门限、未改旧样本时间或自动重试，初始原因和完整raw guard保留；慢读的进一步原因尚未确定。

launcher1、Native cleanup false/error incomplete or deadline、entry restore无证明、XR1（terminal exiting without end、对象销毁和shutdown完成），input pad恢复true。本轮normal_cleanup_verified=false完整保留，不补签；isolated-cleanup-pending旧回执owned_game_absent=false也不修改。其后独立isolated-done为owned_game_absent=true，保存时精确game/owner/XR/worker全不在，remaining_owned空、无强杀；进程退出与正常恢复证明分开。CLOAK405次失败0，静音验证367/initially_muted1/verify_failed0/target_shared_rejected0，不虚构玩法画面或编码。

证据member-chain-r139/runtime-r140/{session-original,config-failure-original,closeout}.json及prepare/operator日志。下一步检查本次激活前配置获取的慢路径，保留严格获取/身份/新鲜度条件；r139实际查询效果仍待新候选复查。已有离线1404与读量减少28.3%不替代此项实际结果。完整身体/世界覆盖、唯一C0同版视图/命中/枪口、自身遮挡及CV01–24/AC01–22继续未完成，工程25%、公共v71与固定入口不变，目标active。
'''
assert '## r140：' not in doc.read_text(encoding='utf8')
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode();doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native_activation_attempted=False,normal_cleanup_verified=False,exact_processes_absent=True)))
