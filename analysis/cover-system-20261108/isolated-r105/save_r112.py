from pathlib import Path
import json,hashlib,datetime
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
instance=R/'analysis/instances/cover-1110-r112-cloak-source-stage'
result=json.loads((instance/'cover-output/run-result.json').read_text())
assert result['operator_exit']==1 and not result['remaining_owned']
assert result['terminal_session']['native_cleanup_verified'] and result['terminal_session']['entry_restore_verified']
assert result['terminal_session']['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
assert not result['normal_cleanup_verified'] and result['terminal_session']['unified_weapons_cleanup']['drained'] is False
assert result['terminal_session']['native_status']['cover_sphere_probe_attempted']=='0'
assert result['cloak_observations']['samples']==310 and not result['cloak_observations']['failures']
projection=instance/'cover-output/projection'
manifest=json.loads((projection/'recording.json').read_text())
assert len(manifest['frames'])==3 and not manifest['errors']
record=dict(name=instance.name,result_sha256=sha(instance/'cover-output/run-result.json'),
    result=str(instance/'cover-output/run-result.json'),native_sha256='58bdbf940abb2e087ff845df7715087f452c2a71a581033caa3bb382d023f333',
    actual_query_attempted=False,first_failure='operator.py shadows Python stdlib operator during pathlib import; no cover input',
    native_restore=True,entry_restore=True,pad_restore=True,xr_exit=0,launcher_exit=0,owned_absent=True,
    unified_drained=False,fire_disable_code=2,hand_stop_code=5,normal_cleanup_verified=False,
    forced_termination=False,cloak_samples=310,cloak_failures=0,
    frames=[dict(file=f['file'],sha256=sha(projection/f['file']),review=('startup waiting card' if i<2 else 'actual stereo scene, both hands; no cover action')) for i,f in enumerate(manifest['frames'])],
    fix='fresh output cover_action.py; generator verifies standard imports from actual output directory',
    next_instance='cover-1110-r113-cloak-source-stage',next_claim_consumed=False,
    remaining='source4 not re-exercised; fire stop acknowledgement not passed; full CV01-24/AC01-22 open')
next_root=R/'analysis/instances'/record['next_instance']
assert not (next_root/'cover-output/claim.json').exists() and not (next_root/'launcher-session.json').exists()
(H/'r112-closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json'
guard=json.loads(guard_path.read_text(encoding='utf8'))
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r112：合成隐藏录制接线，动作启动失败保留（2026-11-10）

本方独立 runner 改用现有 ProjectionRecording 读取实例私有 GPU 双眼投影，取消对可见 HWND 录制的依赖；输入前等待两张新鲜帧，但不据此推断场景内容或玩法通过。每轮检查精确 game/XR 身份和 DWM CLOAKED；未隐藏、读取失败或本方进前台即停止动作并正常收尾。录制在 game/XR 停止前结束，避免身份消失造成工具竞态。实际 runner AST 的八项合成 Win32 检查通过，包含隐藏状态、读取／枚举失败、进前台、PID 复用与暂停；不作为游戏画面证据。

资源不足首次在 claim／启动之前拒绝，之后原占用者退出才启动 cover-1110-r112-cloak-source-stage。game55032／creation134387846856524106，XR58312／creation134387846934804956，Native r111 SHA58bdbf94…333。实际310次自有窗口 CLOAKED 检查，失败0；私有双眼投影3帧，无录制错误。两帧同内容启动等待卡＋最后一帧真实场景／双手已亲自查看，原图完整保留。

实际首失败是生成文件 operator.py 与 Python 标准 operator 模块重名，pathlib 导入发生循环，动作进程 exit1，未发掩体动作。probe attempted0／finished0，不能写成 source4 再现或碰撞通过。改名 cover_action.py，并在生成器中从实际输出目录执行标准库导入验证；修正新实例的同目录 bootstrap 与14项实际 live() 身份保护检查通过。

正常 stop0；Native恢复、入口恢复、手柄恢复均 true，XR0、launcher0，所有本方 game/owner/XR 已不存在，未强杀。统一武器 fire_disable2／hand_stop5／drainedfalse，整体 normal_cleanup_verified=false 原样保留，不能称全部清理通过；进程正常终结关闭本次内存对象生命周期，未取得 fire 停止确认这一失败仍开放。未回滚或改写共享源码解决该状态。

新的 cover-1110-r113-cloak-source-stage 使用同 r111 Native、冻结PG/XR及cloak1110准备，动作文件已修正；资源检查仍发现两个活跃自动实例时在 claim／启动前拒绝，不占运行、不操作他人。r113尚未启动／消耗，接下来只在资源允许后运行这一已修正的定向 source-stage 流程。r112不可重跑。证据 isolated-r105/r112-closeout.json、cloak-guard-check.json 与实例原始 run-result／projection，公共v71不变。

本轮不是掩体功能通过。原 source4 与头肩遮挡未解决，工程25%不变，CV-01–24／AC-01–22和物理头显继续开放。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8')
doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,source4_unexercised=True,next_claim_consumed=False)))
