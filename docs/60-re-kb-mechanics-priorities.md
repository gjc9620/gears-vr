# Game mechanism knowledge base: available evidence and minimum follow-up

2026-10-15. This round only retrieves reports, IDA exports and historical running records; no IDA connection, no re-decompilation, no changes to database or game. See structured entries `analysis/re-kb/mechanics-findings.json`. This is a combo that does not represent the full coverage of the game; currently UI public drawing links and VR mainlines are preferred.

## The source must be separated.

Disk EXE SHA 256：`bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`Camera IDA evidence from PID 16812 Here `2026-10-12T17:09:44.809609Z` RUN-TIMER CODES, CATEGORY SHA 256 Yes `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`,File Offset 0 Map EA `0x140001000`, module base `0x140000000`. The disk Hash identification file cannot replace the running time-period Hash. Source multidisc/ run-time bytes are inconsistent; two functions cannot be combined unconditionally with RVA.

History `analysis/ghidra/` Path is the source of evidence and this wheel tool remains IDA + REA, without migration. UnrealScript object index, structure field offset, function RVA, process object pointer separate fields; native declaration in package is not considered as native.

## Directly reusable conclusion

| Entry | Evidence and credible scope | Current gap |
| --- | --- | --- |
| Camera Count | `camera-110878-20261014/README.md`and address-based compilations and `byte-verification.json`Packaging RVA `0x3b3af4` Call `0x110878`;CurrentCamMode in camera+0x558;calculating contains Pawn, mode, rotation and plug-in values. Static layout evidence is highly credible | Hex-Rays prototype and AVX pseudocode are unreliable;`0x3d4fbc` The assignment target is unknown and cannot be named GetPawnViewLocation |
| Native View | `docs/04-native-capture-validation.md`、`analysis/runtime/native-capture-1.*`：PID 32532 Medium RVA `0x15cf08` It's... 4115 Sub-calls match local players, matrix readable; historical dynamic validation | Logs are lost. 1 bar, view does not match Present; no full state is established on this basis |
| First person anchor | `player-anchor-21208-dynamic.audit.json`：481 in sample 437 Accept,290 The Pawn/Controller Pitch is different.23 A Controller/POV rotated differently | Small angle static position test; Pawn.Location+World ZxEyeHeight remains hypothetical and not a lens or full lens formula |
| Cross-render threads | `anchor-scene-family-20261014/association-review.md`：`0x2c78f4 → 0x15cf08 → 0x3a148c → 0x116250` Same family data stream; recent history homogeneity join has 1/1740 Not Match | Old CSV No Pawn/SceneInvolvimentID; global lateest, address and proximity cannot authenticate association |
| Weapons/animations | `docs/19-first-person-motion-and-recoil.md`、`motion-recoil-audit.json`Add bIgnoreControllers, four recoil-based animations and 21 Number of name-based filters remains unchanged | The fact that the package properties do not prove a primary branch, ballistics, final mixing weight or recoil cancellation while running |
| Player Status and Star-Signature | `docs/57-ui-position-and-subtitle-route.md`: UpdateHDactions cache may be obsolete due to early return of HUD, ActiveAction, smoke etc. | LastCrosshairHitLocation+0x136c Not function RVA; not final impact point or universal player status sign |

The above path does not contain the list relative to `analysis/ida/`、`analysis/runtime/` or `analysis/first-person/`;JSON saves the complete repository relative path.

## Minimum Follow-up Queue

1. P 1 Camera/first person: First re-export with an existing function to complete the CurrentCamMode assigned object and actual return vector source. Follow-up runs only read links Controller, Pawn, Camera, LocalPlayer and single Scene token; records non-atom reading, refusal reasons, render ticket and crossline transfer. Pawn is not shown as a headward or body roll.
2. P 1 Aim at weapons: Reuse the scene script survey to determine the real grant branch and the current scene ticket. Do not call GetAdjustedaim, which could change random dispersal status, to “validate” caches. Hands must aim for a different source of the gun's mouth, weapon bones and actual direction of launch, and no RVA has been certified.
3. P 2 Animated: Dismantling package data, lens vibrations, bone control and ballistics. For bIgnoreControllers, find a physical primary consumer branch, then compare the single variable with the gun/FOV/shock; complete animated curve is uncoded without first examining all sequences in bulk.
4. P 2 State: Establish a common, aim-oriented, bunker-sprinting, roll-over, cross-field, paused state evidence form. Current camera mode and HUD ActiveAction can only be evidence of their own subsystems without impersonating a general status machine; each one identifies the reflector identity and consumption function, then arranges for a state conversion sample.

Reversing the temporary variable after changing the original view does not equal the withdrawal of all effects:`position-consumers-note.md` Recorded `0x2718c8` The LocalPlayer status is updated by location. The geometry of the freeze view only preserves the relevant settings selected from the old point of view, which must be kept at this experimental boundary.

Current round AI in-depth back-translation 0;reuses 6 Type of evidence, structured registration 7 Item. Real Token measures are currently not available, without file size or estimate of counterfeit billing data.

---

## 中文

# 游戏机制知识库：已有证据与最小后续任务

2026-10-15。本轮只检索已有报告、IDA 导出和历史运行记录；未连接 IDA、未重新反编译、未修改数据库或运行游戏。结构化条目见 `analysis/re-kb/mechanics-findings.json`。这是有界梳理，不代表游戏机制全覆盖；当前 UI 公共绘制链路和 VR 主线优先。

## 来源必须分离

磁盘 EXE SHA256：`bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。相机 IDA 证据来自 PID 16812 于 `2026-10-12T17:09:44.809609Z` 捕获的运行时代码，代码段 SHA256 为 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，文件偏移 0 映射 EA `0x140001000`，模块基址 `0x140000000`。磁盘哈希标识发行文件，不能替代运行时段哈希。源 manifest 有多处磁盘/运行时字节不一致；不得将两类函数按 RVA 无条件合并。

历史 `analysis/ghidra/` 路径是既存证据出处，本轮工具仍为 IDA + REA，没有迁移环境。UnrealScript 对象索引、结构字段偏移、函数 RVA、进程对象指针分别建字段；包中 native 声明不视作原生实现。

## 可直接复用的结论

| 条目 | 证据与可信范围 | 当前缺口 |
| --- | --- | --- |
| 相机计算 | `camera-110878-20261014/README.md`、地址化汇编及 `byte-verification.json`：包装 RVA `0x3b3af4` 调用 `0x110878`；CurrentCamMode 位于 camera+0x558；计算包含 Pawn、模式、旋转与插值。静态布局证据高可信 | Hex-Rays 原型和 AVX 伪代码不可靠；`0x3d4fbc` 的分派目标身份未知，不能命名为 GetPawnViewLocation |
| 原生场景视图 | `docs/04-native-capture-validation.md`、`analysis/runtime/native-capture-1.*`：PID 32532 中 RVA `0x15cf08` 的 4115 次调用匹配本地玩家，矩阵可读；属于历史动态验证 | 日志丢 1 条，视图与 Present 不保证一一对应；没有据此证明全部状态 |
| 第一人称锚点 | `player-anchor-21208-dynamic.audit.json`：481 样本中 437 接受，290 个 Pawn/Controller pitch 不同，23 个 Controller/POV 旋转不同 | 小角度静止位置试验；Pawn.Location+世界 Z×EyeHeight 仍是假设，不是眼骨或完整镜头公式 |
| 跨渲染线程关联 | `anchor-scene-family-20261014/association-review.md`：`0x2c78f4 → 0x15cf08 → 0x3a148c → 0x116250` 的同 family 数据流；历史最近同线程 join 有 1/1740 不匹配 | 旧 CSV 无 Pawn/SceneInvocationID；全局 latest、地址和时间邻近不能认证关联 |
| 武器/动画 | `docs/19-first-person-motion-and-recoil.md`、`motion-recoil-audit.json`：包中新增 bIgnoreControllers，四种后坐力镜头动画及 21 个按名筛选函数字节不变 | 包属性事实不证明运行时原生分支、弹道、最终混合权重或取消后坐力 |
| 玩家状态与准星 | `docs/57-ui-position-and-subtitle-route.md`：UpdateHUDActions 的缓存可能因 HUD、ActiveAction、烟雾等提前返回而陈旧 | LastCrosshairHitLocation+0x136c 不是函数 RVA；不是最终弹着点或通用玩家状态标志 |

上述路径未带目录者相对于 `analysis/ida/`、`analysis/runtime/` 或 `analysis/first-person/`；JSON 保存完整仓库相对路径。

## 最小后续队列

1. P1，相机/第一人称：先复用已有函数导出，补齐 CurrentCamMode 分派对象和实际返回向量来源。后续运行验证只读关联 Controller、Pawn、Camera、LocalPlayer 与单次 Scene token；记录非原子读取、拒绝理由、renderer ticket 和跨线程传递。不把 Pawn 朝向或身体翻滚直接作为头显朝向。
2. P1，武器瞄准：先复用准星脚本调查，确定真实赋值分支与该次 scene ticket。不要额外调用可能改变随机散布状态的 GetAdjustedAim 来“验证”缓存。双手瞄准必须另找枪口、武器骨骼和实际发射方向的原生来源，目前无已认证 RVA。
3. P2，动画：将包数据、镜头震动、骨骼控制和弹道拆开。针对 bIgnoreControllers 找一个实际原生消费分支，随后在同枪械/FOV/震动设置下做单变量比较；完整动画曲线未解码，不先批量研究全部序列。
4. P2，状态：建立普通、瞄准、掩体、冲刺、翻滚、过场、暂停的状态证据表。当前相机模式及 HUD ActiveAction 只能是各自子系统证据，不冒充通用状态机；每项先确认反射身份和消费函数，再安排状态转换采样。

修改原视点后恢复临时变量不等于撤销所有影响：`position-consumers-note.md` 已记录 `0x2718c8` 会按位置更新 LocalPlayer 状态。仅在冻结视图上改几何也会保留旧视点选出的相关设置，必须保留此实验边界。

本轮 AI 深入反编译数量为 0；复用 6 类证据，结构化登记 7 项。真实 Token 计量当前未取得，不用文件大小或估算冒充账单数据。
