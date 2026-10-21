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
