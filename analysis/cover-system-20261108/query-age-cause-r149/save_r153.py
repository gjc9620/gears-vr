"""Preserve failed startup and separate the source-only bounded retry repair."""
from pathlib import Path
import datetime,hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];root=R/'analysis/instances/cover-1111-r153-cloak-pulse'
out=H/'runtime-r153-startup';out.mkdir(exist_ok=True)
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];index=load(root/'launcher-session.json')
assert v.get('operator_exit') is None and not v['normal_cleanup_verified'] and v['normal_cleanup_required']
assert not v['remaining_owned'] and not v['forced_termination'] and s['native_started'] is False and s['xr_exit_code']==1
trace=s['exception_traceback'];assert 'start_gamepad' in trace and 'FrozenThreads' in trace and 'OpenThread' in trace
assert '"failed_tid_still_present": false' in trace and '"tid": 36712' in trace
assert not(root/'cover-output/projection').exists()
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for i in identities.values():assert c.identity(i['pid'])!=i
for source,name in ((root/'cover-output/run-result.json','run-result-original.json'),
                    (Path(index['session_report']),'session-original.json'),
                    (Path(index['session_report']).with_suffix('.first-failure.json'),'first-failure-original.json'),
                    (root/'isolated-done.json','isolated-done-original.json'),
                    (root/'isolated-worker.json','worker-original.json'),
                    (root/'isolated-start-request.json','first-start-request-original.json')):shutil.copyfile(source,out/name)
(out/'exception-original.txt').write_text(trace,encoding='utf-8')
summary=dict(scope='Actual startup failure before Native continuous activation or operator',identities=identities,
    exact_owned_processes_absent=True,normal_cleanup_verified=False,launcher_exit=1,xr_exit=1,
    failed_thread=36712,failed_thread_still_present=False,already_suspended_threads=15,
    startup_stage='before_activation/start_gamepad/background_input.change/FrozenThreads.OpenThread',
    operator_started=False,projection_frames=0,pulse_runtime_verified=False,query_cause_observed=False,
    source_repair='At most 3 whole enable transactions after witnessed absent-thread OpenThread87; unchanged strict freeze; same identity and certified original code required; first failure retained.',
    checks=dict(startup_loader=26,background_input=9,failures=0),
    source_sha256={p:sha(R/p) for p in ('tools/vr-launcher/playground_loader.py','tools/game-io/background_input.py','analysis/cover-system-20261108/isolated-r105/prepare.py')},
    background_input_modified_by_this_turn=False,startup_retry_runtime_verified=False,
    world_coverage_verified=False,movement_authorized=False,gameplay_accepted=False,public_changed=False,headset_accepted=False)
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r153：启动线程消失87拒绝，原冻结不变、完整事务有界重试修正（2026-11-11）

基于bf57aec6准备的r153等待活句柄观察原两名额占用，资源释放后单次启动。实际game39264/creation134388146522999539、owner53828/creation134388146430178398、XR39540/creation134388146609309585。XR preflight6.324秒后，before_activation/start_gamepad/background_input.change/FrozenThreads.OpenThread出现WinError87：枚举到tid36712，已挂起15条；新的同进程快照明确failed_tid_still_present false，失败线程已消失。实际异常堆栈及原诊断已保留，不归咎掩体、Native查询或新步行脉冲。

Native continuous native_started false，operator未启动、projection0、未输入A/步行/pose_request，步行修正和查询年龄分项均未取得运行证据。launcher1/XR1，normal_cleanup_verified false；后续精确game/owner/XR/worker身份均已不存在、无强杀，这一进程事实不替代正常清理。原始结果、首次失败和堆栈在query-age-cause-r149/runtime-r153-startup。

未改当前已有未提交的background_input.py，也未跳过任何线程。playground_loader增加_enable_gamepad_gate，仅当原FrozenThreads已完整展开并抛出带唯一见证的OpenThread87、失败tid来自旧枚举且在新快照消失时，允许最多3次完整enable事务；每次保留check_live、精确创建身份、完整status原代码认证，不接受enabled/pending/未知代码，不重复Native Start或GateLease。其他87、仍存活线程、观察失败、权限/恢复错误均直接拒绝；第三次不再重试。成功报告保留原异常和见证，失败保留重试历史。原Freeze的异常恢复失败会变RuntimeError，不能进入此重试。源码与离线检查不证明实际恢复或入口已修好。

26项startup/loader检查及9项background_input检查通过，包含真实start_gamepad函数的模拟依赖调用、首败留存、身份变化/代码未知拒绝、次数界及原回滚/释放；没有调用游戏/真实冻结。prepare现在绑定原8项来源以及loader和background_input两项启动依赖SHA，已有准备记录不改哈希，新运行须重新准备。下一步保存后使用新私有实例定向验证启动、定时步行及原CT2入口，再继续原查询原因定位。完整CV01–24/AC01–22、工程25%、publicv71/固定入口及实体头显状态不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=False,processes_absent=True,startup_repair_runtime_verified=False)))
