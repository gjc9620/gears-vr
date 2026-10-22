# 双手四通道连续 IK 与原动画交接试验

2026-10-23，独立分支 `codex/weapon-6dof`。默认关闭的 `GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL` 已在购买版自有测试场完成五轮各 32 次双手位置／旋转驱动、32 次还原及一次立即近战交接，其中三轮覆盖三个位置轴和三个旋转轴。输入为有身份和新鲜度校验的合成骨世界姿态；这还不是实际手柄、全关卡或头显验收，也不改变公开的 `ownership_proved=0`。

## 写入范围与运行顺序

复用人物原八个手部控制器。右侧链尾 index 2／3 是世界位置／旋转，左侧链尾 index 6／7 是同类通道；其余四个 `IKBone*` 头控制器保持原值。动画树链、部件所属网格、坐标空间、混合配置、渲染开关和骨数组不修改，不请求强制动画求值。

`hand_ik_stream_lease.h` 为四个通道维护独立 journal：

1. 写入前先完成四组原目标和三强度值快照，强度必须逐位等于正零三元组。所有 journal 都在第一次可能的修改前建立。
2. 每个目标是原样保存的 12 字节。位置使用三个 float；旋转先将弧度转换为三个整数 UE Rotator，并核对反射结构的字段类型、大小和 0／4／8 偏移。SingleBone 链尾还要求 `bApplyTranslation=false`。
3. 目标写入和原版 `SetSkelControlStrength(1,0)` 后，重读四组目标及强度，全部匹配才记录本次私有 `trial_apply_mask=3`。
4. 下一次 `PlayerTick` 最开始、桥接请求分发前，逐组比较仍拥有的字段，调用原 Setter 归零并验证，再恢复原目标。原 `PlayerTick` 在恢复后运行；新的目标在其返回后写入。
5. 在相关原动作的 bridge、ProcessEvent 或 CallFunction 入口执行前，如果 journal 尚未恢复，则先恢复再原样转发。回调独立于只读观察器的活动、日志期限和容量，在访问非原子试验状态前先检查游戏线程。

恢复只依赖该 journal 的当前对象、固定配置及拥有的动态字段。新的动作状态或当前武器变化可以阻止继续写入，但不会阻止恢复仍然属于本次试验的链尾。未知值、外部改写、部分强度或无法确认的恢复保持 dirty；不以旧值覆盖，状态 8 拒绝卸载，由外部 owner 关闭其自有游戏。helper 返回失败但重读确认全部恢复时，journal 可以 clean，但 sticky fault 仍终止试验。

## 何时允许这一帧写入

首次和近战结束后都需要至少八个不同网格 Tick、持续至少 250 ms 的合格观察。检查购买版自有测试场身份、人物存活、当前武器 Active、已知固定动作标志、切换／装填／近战请求／过渡状态、八控制器身份和原强度模式。

所有身体槽元素都必须成功读取。认证的空指针可以跳过；读取失败不能当空槽。有效 slot 要求当前网格匹配、父混合时间为零、不在播放 custom、目标回到 base。每个 custom 子的实际权重、总权重和目标权重都必须严格为零，并完整读到子节点状态；非零最小浮点值同样拒绝，不用 epsilon 放宽。权重／剩余混合的零检查按 binary32 位模式允许正／负零，避免游戏 MXCSR 的 DAZ／FTZ 把非零次正规数当零；离线测试主动启用该模式、读 volatile 次正规数验证拒绝，然后恢复测试进程原模式。候选不改变游戏浮点模式。

第二局发现 Lancer 的实际退出特征：`Slot_Std_Idle_Lower` 已 `playing=0,target_index=0,blend=0`，`AR_Melee_Saw_Idle` 子的三种权重均为零，但子 `bPlaying=1` 保留到 30 秒超时。退出混合期间的 `Weight=TotalWeight=0.134134` 仍会被拒绝。保存脚本的循环 `BS_MeleeIdle` 与 `StopBodyStance → StopCustomAnim` 路径与这个现象一致；Engine 导出没有 StopCustomAnim 正文，不能要求它必然清该标志。

因此本候选将子 `bPlaying` 保留为已知遥测，以父槽和三种零权重判断当前没有 custom 姿态贡献。不调用 StopAnim 清原字段。保留的播放标志不代表已证明时间仍在推进；零姿态权重也不单独证明所有原生 notify／根运动逻辑停止。逐帧恢复和同步动作交接仍然必须执行。

## 有限试验与证据

每段八次 lease。前两轮使用右／左世界 X 分别 +1／−1，再反向；近战后使用右／左 yaw 分别 +0.03／−0.03 弧度，再反向。当前源码和后三轮扩大为三轴组合：位置右 `(1,0.5,0.75)`、左 `(-0.75,1,0.5)` 原始单位，然后反向；旋转 pitch／yaw／roll 右 `(0.02,0.03,-0.025)`、左 `(-0.03,-0.02,0.015)` 弧度，然后反向。两个手的目标分别计算。姿态经现有 `FreeHandControl` 的相同身份、新鲜度和完整 6DoF 数据校验；公开 eligibility／apply 仍为零，有限私有 lease 单独授权。每手绑定当前原骨姿态，属于临时试验基准，不称作实际握点或手掌校准。

第 16 次写入后，在同一 post-Tick 内立即调用原 `StartMeleeAttack`。必须记录 dirty→restore→original，实际观察 `bDoingMeleeAttack=true` 和相关 `PlayCustomAnim*`，1.4 秒后调用原 Stop，再等待父槽、子权重、原状态及稳定窗口全部满足，才执行后 16 次。该 dirty 交接由 bridge 入口首先完成；它不冒充“外层 VM 参数尚未消费时执行恢复 Setter”的实测。

| 购买版进程 | 私有回执 | 结果 |
| --- | --- | --- |
| 29424 | `analysis/hand-ik-stream-runtime-1023a.json` | 16 次驱动／16 次恢复和立即交接成功；旧子播放条件导致 30 秒干净超时，不算完整试验通过 |
| 51004 | `analysis/hand-ik-stream-runtime-1023b.json` | 相同干净超时；详细字段定位到零权重 Saw_Idle 保留标志；未改变原动画字段 |
| 15952 | `analysis/hand-ik-stream-runtime-1023c.json` | 32 次驱动／32 次恢复、96 份完整矩阵样本，256 次 Setter、256 次目标写入／恢复，实际近战与重新接管通过；dirty／fault 均为零 |
| 20624 | `analysis/hand-ik-stream-runtime-1023d.json` | 调整非原子状态访问前的线程过滤后重复 X／yaw 试验，全部 32 次恢复与交接通过 |
| 56972 | `analysis/hand-ik-stream-runtime-1023e.json` | 三个位置轴及三个旋转轴组合目标／反向目标，全部 32 次恢复与交接通过 |
| 37196 | `analysis/hand-ik-stream-runtime-1023f.json` | 权重零检查改为独立于 DAZ／FTZ 的位模式后，重复三轴试验，96 份采样、32 次恢复及实际近战／重新接管全部通过 |
| 43348 | `analysis/hand-ik-stream-runtime-1023g.json` | 终态不再授权新目标、部分失败不误报完整 apply 掩码后，三轴试验与全部恢复协议再次通过 |

15952 候选 DLL SHA256 为 `db3477fdcb0a750b900107092ec0f87a30471c6b48e7bf097adb14974ef488f4`，日志 SHA256 为 `fa8f11912a4beee28bf368af4b85029864c0733f4af38b975444a203082d1f8e`。停机返回 0；独立读取的 CallFunction32／ProcessEvent64 与加载前原始证书一致。自有游戏已退出，九个被静音会话均恢复。前两局同样完成字段、入口和九个音频会话恢复，失败回执保留。

私有骨响应复算输出在 `analysis/hand-ik-stream-bone-response-15952/`。31 个有网格 Tick 推进的样本中，两手目标位置残差最大约右 `2.67e-5`／左 `2.57e-5` 原始单位。旋转首次正向段，实际右／左 yaw 约变化 `+0.02977／−0.03027` 弧度；反转段约 `−0.05983／+0.06021`。首次旋转后完整角差约右 `0.01299°`／左 `0.02686°`，反转后约 `0.00831°／0.02328°`。第 16 次是同帧立即恢复，没有下一网格 Tick，单独排除效果统计。

这些数据表明当前实例的双手骨位置和方向随试验目标变化。没有自然 sham 的完整因果验收，也没有最终绘制或头显图像验收；采集器和运行器的 `effect_proved`、全局 `ownership_proved` 继续为 false。

三轴组合的 56972／37196 各有 31 个跨 Tick 样本、每次网格 Tick 均 +1，两手完整位置目标残差分别均小于 `6.16e-5`／`3.25e-5` 单位，两个旋转段完整目标角差分别小于 `0.020°`／`0.022°`。37196 首次实际位移为右 `(1,0.5,0.75)`、左约 `(-0.75,1.000006,0.5)`，反向时约右 `(-2,-0.999994,-1.5)`、左 `(1.5,-2.000008,-1)`；首次组合旋转的完整角差由右 `2.4045°→0.01790°`、左 `2.3042°→0.01105°`，反向时由右 `4.7782°→0.02087°`、左 `4.6120°→0.01308°`。复算在相应 `analysis/hand-ik-stream-bone-response-<PID>/`。

37196 该轮 DLL SHA256 为 `29069e7215a44450a6448e2babe6a71974ae12b53c97b69e981e98159bc96f21`，日志 SHA256 `942cecc70715332c53a84cf6143bdf731856273929d1ed8250aad84488697849`；56972 日志 SHA256 `7c2ff3ed8c1b275ddb0c30bb1c8181e8f29054159cf29e739526c1865d4bd147`。追加两局均完成两个入口独立还原、九个静音会话恢复及自有游戏退出。候选二进制、相关源码和哈希清单只存于忽略的 `analysis/hand-ik-stream-candidates/`；原始失败回执没有改为成功。

43348 的最终源码候选 DLL SHA256 为 `87308482f6e60502361f81032e96b28f9b2d2971b02b9f2962d1046643090563`，日志 SHA256 `1ec630c750a0a30f58c40e53152cfc40c686a5cf74338b9dc9eec477050253f9`；该局两手完整位置残差最大 `6.248e-5` 单位、旋转段完整角差最大 `0.01669°`。32 次恢复、两个入口独立还原、九个静音会话恢复及游戏退出均通过。终态的私有 apply 为零表示停止授权新目标；任何未清理 journal 仍由独立 dirty／fault／safe 字段报告，不能把 apply 为零当成恢复成功。

## 构建与自动检查

```powershell
cmake -S tools/playground -B analysis/build-hand-ik-stream-trial -G "Visual Studio 18 2026" -A x64 -DGEARS_PLAYGROUND_HAND_ANIMATION_PROBE=ON -DGEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE=ON -DGEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL=ON -DGEARS_PLAYGROUND_OUTPUT_NAME=GearsPlaygroundHandIkStreamTrial
cmake --build analysis/build-hand-ik-stream-trial --config Release --target GearsPlayground HandIkStreamLeaseTest --parallel
py -3 -m unittest discover -s tools/playground -p test_hand_ik_stream_trial_run.py -v
```

宏要求两种原动画观察入口，且拒绝与旧单脉冲／六窗、自由手探针及其他输入／骨骼写入候选组合。入口已由其他 Native owner 安装时，CallFunction 原字节认证会拒绝叠加。

新运行器严格检查 32 次 lease／三阶段 sequence／96 个有限完整矩阵／32 次有序恢复／唯一第 16 次动作交接／终行所有计数和标志。加载前独立读 CallFunction 32 字节，停机后独立读两个入口；日志声称恢复不能替代活体字节比较。不安全或不能确认 clean 时保留 Hook，不执行正常卸载。30 项 Python 检查覆盖成功、干净失败、部分生命周期、停止失败、字节不符与伪造日志；四通道 C++ 测试覆盖每个部分写入点、失败后已写入、foreign 值不覆盖及精确恢复。旧受控 IK 和 Hook 离线检查、私有与默认关闭构建通过。

默认构建仍关闭全部手部试验宏，导出表不含 HandIk／HandAnimation 测试入口。未替换冻结 Native／XR、默认启动器或发布包。

下一步是实际双手 grip 到手骨的分别标定、与现有 Native 同票据姿态及单一 CallFunction owner 的集成、更多拾取／收枪／受伤／处决交接，以及最终渲染和头显验收。
