from pathlib import Path
import datetime,hashlib,json,shutil,subprocess,sys
from inspect_pose_probe import inspect
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r127'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
root=R/'analysis/instances/cover-1110-r127-cloak-pose-gate';result=load(root/'cover-output/run-result.json')
index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['operator_exit']==1 and result['normal_cleanup_verified'] and not result['remaining_owned']
assert result['projection_thread_joined'] and not result['projection_errors'] and result['projection_frames']==129
assert not result['forced_termination'] and result['cloak_observations']==dict(samples=505,failures=[])
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
mute=index['test_audio_guard']['mute_verification'];assert mute['verified_muted_observations']==216 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl()
for key in ('game_identity','owner_identity','xr_identity'):assert control.identity(session[key]['pid'])!=session[key]
report=Path(index['session_report']);saved=out/'session-original.json';assert not saved.exists();shutil.copyfile(report,saved)
probe=inspect(load(saved));assert not probe['rows'] and not probe['missing'] and not probe['query_attempted']
(out/'pose-probe.json').write_text(json.dumps(probe,indent=2)+'\n')
log=root/'cover-output/operator.log';assert 'AnchorRegistryChanged: Object registry changed since resolution' in log.read_text()
shutil.copyfile(log,out/'operator-original.log')
media=load(out/'media-review.json');assert media['encode_exit']==0 and len(media['pages'])==7 and not media['contacts']
media.update(full_span_personally_reviewed=True,visual_result='Entry not attempted: static Playground start with arms/hands, transient missing hand pixels near 0.9 and 9.8 seconds; no cover phase or contact acceptance',headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
test=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8','-m','unittest','test_cover_playground_observation','-v'],
    cwd=R/'tools/game-playtest',capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
(out/'observer-checks.log').write_bytes(test.stdout+test.stderr);assert test.returncode==0
record=dict(classification='progress: actual observer constructor failure located and bounded fresh discovery repaired',
    game=session['game_identity'],owner=session['owner_identity'],xr=session['xr_identity'],
    source_commit='0d734832',native_sha256=session['diagnostics_artifacts']['native_sha256'],
    original_report=str(report),original_report_sha256=sha(saved),operator_log_sha256=sha(log),
    first_failure='AnchorRegistryChanged during LiveRegistryAnchorSampler construction before first cover current() observation',
    cover_entry_executed=False,probe_executed=False,collision_clear=None,movement_authorized=False,
    normal_cleanup_verified=True,unified_weapons_cleanup=session['unified_weapons_cleanup'],cloak=result['cloak_observations'],mute=mute,
    observer_repair=dict(test_cases=8,test_exit=0,source_sha256=sha(R/'tools/game-playtest/cover_playground_observation.py'),
        tests_sha256=sha(R/'tools/game-playtest/test_cover_playground_observation.py'),game_verified=False),
    remaining=['actual entry with fresh observer','pre-probe failure witness','PhysicsRoles layout and actual query','complete cover gameplay and headset'])
(out/'closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=H.parent/'requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r127：观察器注册表变化首败与一次完整重新解析修正（2026-11-10）

前轮0d734832为源码/定向检查进展。本轮新建cover-1110-r127-cloak-pose-gate，Native d7cb104d…c2e5，匹配PG/XR冻结版，0.4。首次资源检查先后见两组其他worker仍活，均在claim前拒绝；本方没有重绑票据或抢占。只读精确PID/creation/image与进程handle等待后，资源guard允许同一未消费候选实际启动一次；admission-wait.json保留等待，未作为玩法失败。

实际game42268/creation134387936291943740，owner58124/creation134387936211889401。实际epoch1双眼投影后，ordinary operator在第一次current()的sample_playground构造LiveRegistryAnchorSampler时抛AnchorRegistryChanged: Object registry changed since resolution，operator1。原版接近/A进入及五段phase均未执行，前置姿态各gate计数0、Sphere attempted0；本次不复现r125未测量的具体原因，也不把Native session passed或raw0当玩法通过/空碰撞。

129张实际投影、录制错误0/线程joined，VFR编码exit0；7张全跨度overview均亲自查看，只有起点场景与双臂/手部，局部约0.9s/9.8s有手部像素缺失，无掩体phase/contact验收。没有对本次未执行的掩体动作作视觉结论；r123/r125头肩遮挡FAIL保持。

正常stop、Native/entry/pad恢复true，fire_disable0/hand_stop5/drainedtrue、XR0/launcher0，本方game/owner/XR精确身份已不存在，无强杀。CLOAK505次失败0；目标静音读回216、initially_muted1/verify_failed0/target_shared_rejected0。原报告与operator原日志已复制并逐SHA绑定，早败保留。

针对这次真实观察器构造失败，read-only Playground adapter只在AnchorRegistryChanged时丢弃失败括号、关闭旧Resolver，并在同一session/game/owner/XR身份下最多完整重新发现一次；第二次仍失败、其他错误或人物/会话变化均终止。成功保留第一次拒绝原evidence和新样本原始时间，不修改旧证据/100ms/原guard，不发输入。8项实际adapter路径检查（读进程为Host缝）全通过，覆盖重建/关闭、二次失败、非registry错误、换会话、错误人物及原空UI约束。新operator generator将此adapter独立冻结为私有模块，runner校验散列，避免运行时无记录地变化。该修正尚未新游戏验证；Native不需重编。

证据pose-probe-r126/runtime-r127/{closeout,pose-probe,media-review,session-original}.json与operator-original.log/observer-checks.log，解析器7项合成报告检查通过。下一步用新鲜票据和冻结修正观察器定向进入，读取r126原拒绝见证；公共v71、工程25%和完整CV01–24/AC01–22/物理头显未完成状态保持。
'''
assert '## r127：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup=True,probe_attempted=False,observer_tests=8)))
