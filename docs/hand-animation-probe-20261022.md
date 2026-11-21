# Original pick-up/near-war animation observation

`GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` Default closes only for the Playground test field. It does not call engine functions, does not send input, does not change game properties, animations, controllers or bones. It only observes original action initiated by a different test driver; all records are fixed `ownership_proved=0 writes=0`. This candidate does not access the default starter.

Enable in the quarantine output directory:

```powershell
cmake -S tools/playground -B analysis/build-hand-animation-probe -G "Visual Studio 18 2026" -A x64 -DGEARS_PLAYGROUND_HAND_ANIMATION_PROBE=ON -DGEARS_PLAYGROUND_OUTPUT_NAME=GearsPlaygroundHandAnimationProbe
cmake --build analysis/build-hand-animation-probe --config Release --target GearsPlayground HandAnimationObservationTest PlaygroundHookTest --parallel
```

The entrance must satisfy both initialization, automatic testing mode, test field ready and test subprocess. `GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`、`GEARSVR_TEST_WINDOWS_BEHIND=1`、`GEARSVR_TEST_MUTE_GAME=1`. The probe does not support replacing role grids or animated trees and continues to record them once each time the game thread is sampled.

| Export | Return value |
| --- | --- |
| `PlaygroundHandAnimationProbeStart` | 0 Queued;1 Environment/testing field/initialization rejection;2 Queued or Rund |
| `PlaygroundHandAnimationProbeStop` | 0 Request submitted for shutdown, next original PlayerTick processing |
| `PlaygroundHandAnimationProbeStatus` | Low 16 bits;high 16 Bit-cycle frame |

Status as 0 idle、1 queued、2 sampling、3 bounded complete、4 explicit stop、5 Invalidated. 3 Only means that the sample reaches the boundary; it does not mean that the full action life cycle or animate the intersection has passed. The upper limit is 30 seconds,1,500 * Periodic frame,256 Match call or call 16 Layer matching call nesting; minimum interval of cycle 20 milliseconds. Extra limit is 32 No actual body slots, no physical cell sets to reserve capacity 256, per slot 8 A node, a bone chain. 16 Sections,32 A chain of control,512 Cache Properties Layout and 512 function name. Actual slots are separated from array reserved capacity: current person measurements for purchase editions 19 Slots, capacity 42;in the early stages of limiting capacity to 32 Reads the entire slot, corrected and re-detected. Overwatching is a state 5 end;normal arrival time/frame/call maximum state 3。

Existing Auto Mode `ProcessEvent` Hook handles only the view/menu/PlayerTick. This candidate adds an independent read-only observation field to the filter and backside, with the original forwarding still being performed as usual. `invoke` Top Level Direct Call Original `ProcessEvent`So there's something else. `source=bridge_invoke` Record; only real detour calls are recorded as `source=hook`. The VM direct call or index native may still not go through the Hook, and lack of logs does not allow for the original version to be unexecuted.

Log is still next to the DLL. `playground-<PID>.log`. The space separated in each row `key=value`：

| Prefix | Links and content |
| --- | --- |
| `hand_animation_probe_begin` | schema=1 5 standing case identities, ceilings, observational positions and bridge bypass statements |
| `hand_animation_event` | seq is log boundary number; call numbers are paired; parent/depth describes matching call nesting; case is pre/post; source distinguishes Hook and bridge; function/outer identification function and declaration status;kind is shown in the table below; naming parameters certified by real-time parameter type to `param_*` Records |
| `hand_animation_frame` | snapshot is the only serial number for all snapshots; sample is the periodic sample number; trigger_call=0 Call as cycle, otherwise associated;case is periodic/pre/post; contains state known location, original instance identity, world time and QPC, pick-up/near war request and round call |
| `hand_animation_control` | snapshot, index, known, object registration index/Name/class/outer, grid, NextControl, current/target strength, residual mix, list_mask of current tree chain |
| `hand_animation_slot` | snapshot, index, ktown, current slot identity/NodeName, grid, total weight, remaining mixing, waiting to be mixed, custom/target sub-index and all with boundary subnodes |
| `hand_animation_snapshot_gap` | No snapshot of the same set of examples was obtained for a call border; no nearby periodic frame could be used to replace the boundary |
| `hand_animation_probe_end` | Status/cause, frame/scatter/call number, Hook/bridge kind mask, observed state change, capacity failure and clarity Gap |

`pickup_request` Every time I see you, `ServerUse` Add one, on that basis the gun cannot be claimed to have been picked up.`melee_request` Every time I see you, `StartMeleeAttack` Add one.`pickup_episode` I saw new special actions in the round of questioning. 42／53 Add one at a time;42→53 A new generation is also being created.`melee_episode` Adds a new one when you see Near War Bull changing from known false or unknown to true. They are log links, and they cannot replace the original boundary.`transition` bits 1 Read changes, bits for special actions 2 (a) Changes in the reading of close-to-war;`previous_known` bits 1／2 Each of the last two readings is known, and initially the final line will be marked in action `started_active=1`。

| kind | Call Category |
| --- | --- |
| 0／1／2／3／4 | ServerUse／GrabPickup／ServerDictateSpecialMove／DoSpecialMove／EndSpecialMove |
| 5／6／7／8 | StartSpecialMove／StopSpecialMove／BodyStanceAnimEndNotification／OnAnimEnd |
| 9／10 | Body slot play/stop |
| 11／12／13／14 | Close-off/stop request/MeleeAttackStard/MeleAttackEnded |
| 15／16 | Beginstate/EndState, precisely in MeleeAttacking state |
| 17／18 | PlayCustomAnim/StopCustomAnim of the current body slot |

`frame.state_known` Bits:1 weapon、2 SpecialMove、4 bDoingMeleeAttack、8 bWantsToMelee、16 bSwitchingWeapons、32 bDoingSpecialAttackMove、64 TimeSeconds、128 Health、256 BodyStanceNodes array. The zero value of the unknown item is not a idle certificate.

controller index 0–3 It's right. `IKBoneCtrl`、`IKBoneRotCtrl`、`IKCtrl`、`IKRotCtrl`；4–7 To the left of the same sequence.`control.known` Bits:1 Current validity SkelControllBase,2 SkelComponent、4 NextControl、8 ControlStrength、16 StrengthTarget、32 BlendTimeToGo、64 ControlName、128 Object Name. Only this frame `control_lists_known=1` The blog also highlights the situation:`list_mask` To be recorded as full-fledged tree chain members; part of the failure is only partially observed.

`slot.known` Bits:1 It's working. Anim NodeSlot,2 SkelComponent、4 NodeName、8 NodeTotalWeight、16 BlendTimeToGo、32 PendingBlendOutTime、64 CustomChildIndex、128 TargetChildIndex、256 bIsPlayingCustomAnim、512 Children layout/number,1024 TargetWeight layout/number.`child_data` Comma-separated. Each sub-item is `object:known:Weight:TotalWeight:TargetWeight:AnimSeqNameIndex:AnimSeqNameNumber:AnimSeqNameText:CurrentTime:bPlaying`. Sub-point known bits 1 The main objective is to be readable/both of the actual weights.2 The goal is readable.4／8／16 Reads the serial name/current time/play mark. The current slot and subpoint identity is read again on each frame, and the new animation cannot be used when the slot is reused as the end of the original action.

Purchase of the own version of the test site PID 57480 One regular pick-up and ordinary near-war has been observed: Longshot `ServerUse` After entering special action 53 Original `OnAnimEnd` Hit. `Custom_FullBody` It's... `AR_Idle_Ready_Kickup` subpoint, returns with zero action and zero actual/target weight; then two ordinary `MeleeAttacking` Beginstate/EndState `AR_Melee_Smack_A`／`C`, the person concerned shall be given zero weight at each end.189 A quick look at the same. 19 Private reply. `analysis/hand-animation-probe-runtime-1022e.json`;playout, Playground off and 11 The preservation of scripts suggests that the inventory can be changed before moving; normal close-to-war postures may continue to blend. This probe only authenticates the observational boundary of the normal path and does not certify a full ownership agreement. VM internal calls, cancellation/injury, Lancer chainsaws, other fixed actions, XR hand drives and headlight images must still be verified separately; keep the original version in hand when any necessary evidence is missing.

## Purchaser authentication for internal animated slots

Also with Default Closed `GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE`. It is active already. [Four Parameters CallFunction Evidence](91-reticle-inline-call-boundary.md), only installed silent environment after ownership and RVA required `0x1d6f30` Current access and existing 32 The byte certificate is identical; the entrance has been rewritten by another Native owner and no superimposes. Caller FFrame is an unconsumpted parameter expression that cannot be used as a buffer for parameters; candidate only reads its nonde/object/code/locals/previous space, four parameters and returns the Rax position to keep the original passover. `source=call_function` Separated from Hook/bridge source, still fixed `ownership_proved=0 writes=0`。

Add when Build `-DGEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE=ON`;test driver increases `--require-call-boundary`. This mode independently cross-checks CallFunction bytes before loading, and independently reread CallFunction after the stoppage 32 Bytes and ProcessEvent 64 bytes; full pairing is also required `kind=17` slot playing call.`hand_animation_call_probe_end` Recording of callsbacks/accepted/completed/frame_projected/ hexadecimal; `param_transport=caller_frame_unconsumed` Expressly uncoded parameters.

Purchase of PID 59828 By the original version `ServerUse` Take the previously unknown Longshot and do two more regular close combats. **44 Rejects, calls to complete, zero caller frame 11 Subtraction Play**Total, original monitor 53 Against Call,274 A snapshot, no spill. 3／10／14 Three. `PlayCustomAnimByDuration` Replace the same sub-point with the following: `AR_*Holster_Rt`, three subsequent pairs of calls (18／19／20) On the same object, replace `AR_*Equip_Lt`The target weights are both 1 This result does not mean that all Native calls, cancellations or executions are covered and the events have yet to be accessed into the remains.

DLL SHA 256 `cb74b6d3c03aab59b07217362eacd013a141c979ce4c71de952e14670aa8781f`;private returns `analysis/hand-animation-call-runtime-1023a.json`Summary of changes in the same frame `analysis/hand-animation-call-transitions-1023a.json`Original log SHA 256 `433605e0aed03bfa33df34c04663c7930b97fb1c4fac892da93139b16bdd0a47`。`PlaygroundShutdown=0`The two entrances are independently compared.11 Recovers audio sessions, exits with your own game. Private/default disables construction, calls forwarding/occupancy entrance/restoration failure offline testing, monitors and Hook tests,23 Item Python certifier check passed; default DLL not-observer export. This is the game thread call evidence and does not verify the final image or headline.

Follow-up [IK test with consecutive hands](hand-ik-stream-trial-20261023.md) Reverts the separate game thread that is connected to these three action entry points. It restores the original print type of line, independent of whether this monitor is active, log limit and end time; this read-only probe and its `writes=0` Still, it describes the observers themselves and cannot replace a private-driven writing count. The new limited four-channel candidate and the original observer candidate are to be interpreted separately by DLL Hashi and his reply, without raising global animated ownership certificates.

---

## 中文

# 原版拾取／近战只读动画观察器

`GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` 默认关闭，仅用于自有 Playground 测试场。它不调用引擎函数，不发送输入，不改游戏属性、动画图、控制器或骨骼。它只观察另由测试驱动发起的原版动作；全部记录固定 `ownership_proved=0 writes=0`。此候选不接入默认启动器。

在隔离输出目录启用：

```powershell
cmake -S tools/playground -B analysis/build-hand-animation-probe -G "Visual Studio 18 2026" -A x64 -DGEARS_PLAYGROUND_HAND_ANIMATION_PROBE=ON -DGEARS_PLAYGROUND_OUTPUT_NAME=GearsPlaygroundHandAnimationProbe
cmake --build analysis/build-hand-animation-probe --config Release --target GearsPlayground HandAnimationObservationTest PlaygroundHookTest --parallel
```

入口必须同时满足已初始化、自动测试模式、测试场 ready，以及测试子进程的 `GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`、`GEARSVR_TEST_WINDOWS_BEHIND=1`、`GEARSVR_TEST_MUTE_GAME=1`。游戏线程每次采样复核当前 PC／Pawn／World／Mesh／AnimTree 身份。探针不支持替换角色网格或动画树后继续同一次记录。

| 导出 | 返回值 |
| --- | --- |
| `PlaygroundHandAnimationProbeStart` | 0 已排队；1 环境／测试场／初始化拒绝；2 已排队或运行 |
| `PlaygroundHandAnimationProbeStop` | 0 已提交停采请求，下一个原版 PlayerTick 处理 |
| `PlaygroundHandAnimationProbeStatus` | 低 16 位状态；高 16 位周期帧数 |

状态为 0 idle、1 queued、2 sampling、3 bounded complete、4 explicit stop、5 invalidated。状态 3 仅表示采样达到边界；它不表示完整动作生命周期或动画交接已通过。上限是 30 秒、1,500 个周期帧、256 个匹配调用或 16 层匹配调用嵌套；周期最短间隔 20 毫秒。额外限制为 32 个实际身体槽、身体槽数组预留容量不超过 256、每槽 8 个子节点、每条骨骼控制链 16 节、32 条控制链、512 个缓存属性布局及 512 个函数名。实际槽数与数组预留容量分开校验：购买版当前人物实测 19 槽、容量 42；早期把容量也限为 32 会漏读全部槽，已修正并复测。超过观察容量使状态 5 结束；正常到达时间／帧数／调用数上限为状态 3。

现有自动模式的 `ProcessEvent` Hook 只处理视口／菜单／PlayerTick。本候选在该过滤和递归旁路前加独立只读观察域，原有转发行为仍照常执行。桥接 `invoke` 顶层直接调用原 `ProcessEvent`，所以另有 `source=bridge_invoke` 记录；只有真正经过 detour 的调用记为 `source=hook`。VM 内部直接调用或索引 native 可能仍不经过该 Hook，缺少日志不能判定原版没有执行。

日志仍是候选 DLL 旁的 `playground-<PID>.log`。每行采用空格分隔的 `key=value`：

| 前缀 | 关联与内容 |
| --- | --- |
| `hand_animation_probe_begin` | schema=1、五个固定实例身份、上限、观察位置及桥接旁路声明 |
| `hand_animation_event` | seq 是日志边界序号；call 是配对调用号；parent／depth 描述匹配调用嵌套；phase 为 pre／post；source 区分 Hook 和桥接；function／outer 识别函数及声明所属状态；kind 见下表；经实时参数类型认证的命名参数以 `param_*` 记录 |
| `hand_animation_frame` | snapshot 是所有快照的唯一序号；sample 是周期样本号；trigger_call=0 为周期，否则关联调用；phase 为 periodic／pre／post；含状态已知位、原实例身份、世界时间与 QPC、拾取／近战请求号和轮询动作代次 |
| `hand_animation_control` | snapshot、index、known、对象注册索引／Name／class／outer、网格、NextControl、当前／目标强度、剩余混合、当前树链的 list_mask |
| `hand_animation_slot` | snapshot、index、known、当前槽身份／NodeName、网格、总权重、剩余混合、待退混合、自定义／目标子索引和所有有界子节点 |
| `hand_animation_snapshot_gap` | 某调用边界未能取得同一组实例快照；不能用附近周期帧替代该边界 |
| `hand_animation_probe_end` | 状态／原因、帧数／快照数／调用数、Hook／桥接 kind 掩码、已观察状态变化、容量失败和明确 gap |

`pickup_request` 每见一次 `ServerUse` 增一，不能据此断言拾到了枪。`melee_request` 每见一次 `StartMeleeAttack` 增一。`pickup_episode` 在轮询见到新的特殊动作 42／53 时增一；42→53 也建立新代次。`melee_episode` 在轮询见到近战布尔从已知 false 或未知变为 true 时增一。它们是日志关联线索，不能代替原版边界。`transition` 位 1 为特殊动作读数变化、位 2 为近战读数变化；`previous_known` 位 1／2 分别表示上一次两项读数可知，初始已在动作中会在终行标 `started_active=1`。

| kind | 调用类别 |
| --- | --- |
| 0／1／2／3／4 | ServerUse／GrabPickup／ServerDictateSpecialMove／DoSpecialMove／EndSpecialMove |
| 5／6／7／8 | StartSpecialMove／StopSpecialMove／BodyStanceAnimEndNotification／OnAnimEnd |
| 9／10 | 身体槽播放／停止 |
| 11／12／13／14 | 近战请求／停止请求／MeleeAttackStarted／MeleeAttackEnded |
| 15／16 | 精确属于 MeleeAttacking 状态的 BeginState／EndState |
| 17／18 | 当前身体槽的 PlayCustomAnim／StopCustomAnim |

`frame.state_known` 位值分别为：1 weapon、2 SpecialMove、4 bDoingMeleeAttack、8 bWantsToMelee、16 bSwitchingWeapons、32 bDoingSpecialAttackMove、64 TimeSeconds、128 Health、256 BodyStanceNodes 数组。未知项的零值不是空闲证明。

控制器 index 0–3 是右侧 `IKBoneCtrl`、`IKBoneRotCtrl`、`IKCtrl`、`IKRotCtrl`；4–7 为同序左侧。`control.known` 位值分别为：1 当前有效 SkelControlBase、2 SkelComponent、4 NextControl、8 ControlStrength、16 StrengthTarget、32 BlendTimeToGo、64 ControlName、128 对象 Name。只有该帧 `control_lists_known=1` 时，`list_mask` 才能作为完整有界树链成员记录；部分失败时已有位仅是部分观察。

`slot.known` 位值分别为：1 当前有效 AnimNodeSlot、2 SkelComponent、4 NodeName、8 NodeTotalWeight、16 BlendTimeToGo、32 PendingBlendOutTime、64 CustomChildIndex、128 TargetChildIndex、256 bIsPlayingCustomAnim、512 Children 布局／数量、1024 TargetWeight 布局／数量。`child_data` 为逗号分隔，每个子项是 `object:known:Weight:TotalWeight:TargetWeight:AnimSeqNameIndex:AnimSeqNameNumber:AnimSeqNameText:CurrentTime:bPlaying`。子项 known 位 1 为对象／两种实际权重可读，2 为目标权重可读，4／8／16 为序列名／当前时间／播放标志可读。每帧重新读取当前槽和子节点身份，槽被复用时不能把新动画的淡出当作原动作结束。

购买版自有测试场 PID 57480 已观察到一局正常拾取与普通近战：Longshot `ServerUse` 后进入特殊动作 53，原版 `OnAnimEnd` 命中 `Custom_FullBody` 的 `AR_Idle_Ready_Kickup` 子节点，返回时动作归零且其实际／目标权重为零；随后两次普通 `MeleeAttacking` BeginState／EndState 分别对应 `AR_Melee_Smack_A`／`C`，每次结束后人物相关槽子节点权重归零。189 个快照读到同 19 个身体槽与八个稳定手部控制器身份，探针写入仍为零。私有回执 `analysis/hand-animation-probe-runtime-1022e.json`；游戏退出、Playground 关闭和 11 个静音会话恢复均完成。保存脚本提示拾取可先改库存再进动作，取消可接下一动作；普通近战姿态也可能继续退混合。此探针只认证该次正常路径的可观察边界，不认证完整所有权协议。VM 内部调用、取消／受伤、Lancer 电锯、其他固定动作、XR 手部驱动与头显画面仍须分别验证；缺任一必要证据时保持原版持有双手。

## 内部动画槽调用的购买版验证

另加默认关闭的 `GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE`，要求同时启用本观察器。它复用已有 [四参数 CallFunction 证据](91-reticle-inline-call-boundary.md)，仅在自有置后静音环境安装，并要求 RVA `0x1d6f30` 当前入口与既有 32 字节证书完全相同；入口已被其他 Native 所有者改写时拒绝叠加。调用者 FFrame 是未消费的参数表达式来源，不能作为参数缓冲；候选只读其 node／object／code／locals／previous 和脚本区间，四个参数及返回 RAX 位保持原样透传。新的 `source=call_function` 与 Hook／桥接来源分别记账，仍固定 `ownership_proved=0 writes=0`。

构建时增加 `-DGEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE=ON`；测试驱动增加 `--require-call-boundary`。该模式在加载前独立核对 CallFunction 原字节，停机后独立重读 CallFunction 的 32 字节及 ProcessEvent 的 64 字节；还要求完整配对的实际 `kind=17` 槽播放调用。`hand_animation_call_probe_end` 记录 callbacks／accepted／completed／frame_rejected／十六进制 mask／writes；事件中的 `param_transport=caller_frame_unconsumed` 明示未解码参数。

购买版 PID 59828 通过原版 `ServerUse` 取得原先没有的 Longshot，再做两次普通近战。新增入口取得 **44 对完整调用、零 caller frame 拒绝、11 次槽播放**，原观察器总计 53 对调用、274 个快照，无溢出。slots 3／10／14 的三个 `PlayCustomAnimByDuration` 先将相同子节点改为 `AR_*Holster_Rt`，后续三个配对调用（18／19／20）在同样对象上改为 `AR_*Equip_Lt`，目标权重均为 1；此前只有周期帧能看到的复用，现在已取得真实同步调用边界。主 Kickup 和 A／C 近战的五次槽播放也已观察。这个结果解决本次正常路径的具体漏钩，不等于所有 Native 直接调用、取消或处决都已覆盖，也尚未把这些事件接入持续写骨。

DLL SHA256 `cb74b6d3c03aab59b07217362eacd013a141c979ce4c71de952e14670aa8781f`；私有回执 `analysis/hand-animation-call-runtime-1023a.json`、同帧变化摘要 `analysis/hand-animation-call-transitions-1023a.json`，原日志 SHA256 `433605e0aed03bfa33df34c04663c7930b97fb1c4fac892da93139b16bdd0a47`。`PlaygroundShutdown=0`，两入口独立还原比较、11 个音频会话恢复、自有游戏退出通过。私有／默认禁用构建、调用转发／占用入口／恢复失败离线测试、观察器和 Hook 测试、23 项 Python 验证器检查通过；默认 DLL 无观察器导出。这是游戏线程调用证据，未验证最终画面或头显。

后续 [双手连续 IK 试验](hand-ik-stream-trial-20261023.md) 已在这三个动作入口接入独立的游戏线程恢复回调。它在原动作执行前恢复 private dirty lease，独立于本观察器是否活动、日志限额和结束时间；本只读探针及其 `writes=0` 仍仅描述观察器自身，不能代替整个私有驱动的写入计数。新的有限四通道候选和原观察候选须分开按 DLL 哈希及回执解释，未提升全局动画所有权证明。
