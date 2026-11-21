# Unified candidate v5

The owner v4 session reached Native/PG and player readiness, then the Python controller raised `Unified equipment connection rejected: 3`. In the hand-participant contract 3 means busy: a PG tick may still be active, or a child attach may have partially connected the parent.

The session now calls the existing exact-cookie stop path and waits for quiescence before reconnecting. Off-state busy stops immediately; partial connections drain before any new start. Retry is bounded to 16 busy attempts; clean exhaustion leaves equipment unavailable without ending first-person/free hands. Drain timeout remains an explicit error. Every attempted start participates in final cleanup, including a failed partial start. Calibration unavailable (1) continues to wait for a real measurement; invalid/ABI/cookie/reentrant errors are not suppressed.

Only `sixdof_unified_runtime.py` changes production behavior. Native/XR/PG are exact v4 artifacts. User Steam path, path history, FOV65 and all config bytes are retained. v4 and its actual failure logs are preserved.

Validation: source/AST and package-file inspection only. No game, XR, simulator, tests, replay or preflight was executed. New owner launch and physical headset acceptance remain pending.

---

## 中文

# 统一候选人 v5

车主诉4 会话到达了本地/ PG 和玩家准备状态, 然后 Python 控制器升起 `Unified equipment connection rejected: 3`. 在手持方合同中 3 表示忙碌: PG 勾选可能仍然在活动,或者孩子的附着可能已经部分连接到父母.

复选会话现在叫着现有的精确cookie停止路径, 等待问答后再重新连接。 州外繁忙立即停止; 任何新开始前部分连接排出。 重试将被限制为 16 忙碌尝试; 干净的耗尽会让设备无法使用, 而不会结束第一人称/ 自由手。 排水超时仍然是明显的错误。 每一次尝试启动都会参与最后清理, 包括失败的部分启动。 校正无法( )1)继续等待真实的测量;无效/ABI/cookie/reentrant错误没有被压制.

Only `sixdof_unified_runtime.py` 改变生产行为。4 文物 用户蒸汽路径 路径历史 FOV65 并保留所有配置字节。 v4 并且保存其实际故障日志。

验证: 仅检查源/ AST 和包文件。 没有执行游戏、 XR、模拟器、 测试、 重播或飞行前操作, 新的所有者启动和物理耳机接收仍然有待进行 。
