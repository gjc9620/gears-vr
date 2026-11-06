# 交互复核与离线准备 v2

2026-10-28。结论：取消“全武器先按 R3 才能换弹”的默认前置条件。物理交互从玩家对本枪部件的明确动作开始；代码仍可调用原 ForceReload 建立计时与库存事务，但这不要求控制器上存在对应按键。R3 保留为可选辅助／兼容映射，本轮只修订设计。

## 主流 VR 交互参考

| 一手来源 | 能支持的事实 | 本项目采用的判断 |
| --- | --- | --- |
| [HL2VR 官方手册](https://halflife2vr.com/manual/#reloading)及[功能说明](https://halflife2vr.com/features/) | 手动模式使用按钮弹匣释放、肩部取弹、物理插入；部分枪空仓需额外上膛；项目明确参考 Alyx；另提供简化换弹 | 按钮释放本身合理，但不能把它当所有机械的统一启动步骤。保留辅助方案，手势路径独立可用 |
| [Pavlov 开发者更新](https://store.steampowered.com/news/posts/?appgroupname=Pavlov+VR&appids=555160&enddate=1643058578&feed=steam_community_announcements) | 官方介绍无可拆弹匣猎枪逐发装填；更新中修正弹匣握持释放缓冲、枪机抓取和不同枪的交互区域 | 保留逐枪差异；插入要有适当吸附和完成反馈，不能要求亚毫米对准或靠固定动画等待 |
| [H3VR 开发者控制方案说明](https://steamcommunity.com/app/450540/discussions/4/3002172678253269313/)与[开发者更新](https://store.steampowered.com/news/posts/?appids=450540&enddate=1617414216&feed=steam_community_announcements) | 原模拟控制与简化控制并存，简化模式明确有功能取舍；逐枪仍分别处理弹巢释放／摆出和枪机等动作 | 不把“物理操作”理解成按钮越少越真实。需要本枪的明确意图、合适容错和可选辅助 |

这些来源不是“所有 VR 游戏都允许直接拔匣”的证据。Lancer 默认直接抓取拔出，是针对本项目战斗节奏、既定手部操作与原联动结构的设计选择；隐含的锁扣释放随抓取／拉出完成，不是假设原枪没有锁扣。

## Lancer 新流程

1. 左手离开支撑点，在弹匣／释放区域重新按住 Grip。只接触、从支撑点扫过或追踪跳变不开始换弹。
2. 左手沿取出方向作明确连续移动，超过本枪的脱离阈值后触发一次“解锁并取匣”。锁扣／机匣随原联动表现开放；在本次原换弹被接受时开始完美期限计时。玩家不需要先按 R3。
3. 旧匣在左手。松手按既定规则回收其显示与剩弹保留；左肩取得新匣，完成导向插入后提交本轮定额。旧匣重新插回仅恢复原状态，不补弹、不给奖励。
4. 新匣到位后默认把必要的卡扣／机匣锁定作为一次自然推入／合盖动作完成。若实机原结构需要明确的独立抓点，再提示一次闭合；不先要求玩家分别操作两个同步运动件。
5. 满匣或无备用时允许检查／取回自己的匣和闭合机构，但不创建有奖励的补弹事务。枪内计数与“装填件在位”分开门控，弹匣离枪时不能因为原计数仍有弹而射击。

R3 辅助路径可以触发同一“释放弹匣”事件，不额外奖励、不重复计时、不跳过装填和闭锁；不能同时把一次抓取和一次 R3 计作两轮换弹。最终辅助按键沿用主线输入方案，不在本轮占用新按钮。

## 全武器复核结果

| 武器 | 默认开始动作 | 复核后的其余要求 |
| --- | --- | --- |
| Lancer | 抓匣并明确拉出，联动释放锁扣 | 合并同步机构，保留剩弹和原匣插回取消 |
| Hammerburst | 抓锁扣并打开 | 保留真实锁扣步骤，不能为了统一“直接拔”越过锁定 |
| Gnasher | 缺弹时左肩取一发，或恢复未结束的装填 | 单轮上限 3 保留；允许装 1／2 发后闭锁提前结束，已装数量保留，未完成本轮目标不授完美奖励 |
| Longshot | 空仓抓枪栓并开膛 | 单发实弹，闭锁后可射，不重复要求射后循环 |
| Snub | 抓匣拉出，抓取隐含释放；也可用按钮释放辅助 | 空仓拉放滑套、半仓免拉；不要求抓两片联动侧翼 |
| Boltok | 抓侧面装填支架并打开 | 支架带弹巢摇出；逐发装填，允许部分补弹后关闭。退空壳为开仓后的原件／视觉联动，暂不加证据不足的独立退壳杆 |
| Boomshot | 抓枪管装填机构并开放 | 使用原独立装填件，一次一发，保留必要关闭步骤 |
| Torque Bow | 缺箭时左肩取箭 | 沿槽装箭到位，保留原扳机蓄力。取消与松键发射分开 |
| Hammer of Dawn | 原目标指向／扳机 | 无普通换弹，完整停止目标与光束 |
| Troika | 原炮位领取与双握把 | 无普通换匣，保留自动恢复及 Loader 差异 |
| Frag / Smoke | 取出后扳机准备，主动松扳机投掷 | 手部轨迹决定初速；准备不新增“捏雷读秒”，两种弹体触发规则独立 |

左肩弹药来源和身体槽位沿用既定设计，不改主线。左 Grip 已握支撑点时必须先释放再重新抓机构，防止移手就误拆。旧匣与新匣不能在同一手凭空叠加；需要双持弹匣的熟练操作另行设计，本轮不加入。

完美换弹仍保留既定的“原窗口终点以前完成可奖励”规则，只把起点从 R3 输入改为原游戏接受的首个有效换弹动作。原窗口可能过短，尤其六发逐装与单发取弹；这仍是明确的舒适度风险，本轮不偷偷延长时间或自动授奖。Gnasher／Boltok 提前闭锁的普通完成是本次新增设计，现有固定步骤计划未来需要小幅适配，本次未改其源码。

## Boltok 机构核对

基于原骨层级、每骨独占顶点和原装配位置，`Object03` 是侧面承载支架，`Object02` 是柱形弹巢，`Object08` 是支架子件；后二者都是 Object03 的子骨。`Object01` 是弹巢后方固定端盘，父骨是枪体；`Object09` 在枪管下方，不在弹巢轴线上，不能作为已经确认的退壳杆。私有[装配图](../../analysis/weapon-specs-20261028/review-v2/boltok-assembly-private.png)和[参考骨／设计关节数据](../../analysis/weapon-specs-20261028/review-v2/boltok-rig-authoring.json)保留核对结果。

摇出方案由支架带 Object02/08 运动，Object01 留在枪体。参考姿态支持沿枪身轴的支架旋转设计，但原始开合动画没有被证明：保存默认对象确实绑定 `COG_Pistol.COG_Pistol` AnimSet，其骨名是 Snub 的 b_HG 系列；本次在两个指定包的 21 个 AnimSet 中没有找到匹配 Object 骨组的轨道。因此 **80° 仅为离线可视化初值，不能称为原版角度或已认证实机机构**；后续绑定时保留这个差别。

原 Object02 两端封闭，不能直接表现六个可插入孔。已补做原创六孔弹巢替换件，六个入口独立命名，完整 Boltok 实弹能作离线配合检查。不需要重做枪体。新增件替换封闭部分，不能叠加在原封面上；原支架、轴向与单位转换留在装配适配层。

## Torque Bow 取消链核对

已经定位到最小非发射组合，仍属于静态语义设计，不是已经上线的 API：

```text
仅在当前武器仍处于 Charge、该输入属于本地 VR 时：
    停止继续注入开火，并消费本次取消输入
    ClearPendingFire(0)，以及本事务拥有的其他 pending fire mode
    清理本轮 bPendingEndFire
    ChargeEnded()：清 TensionIntroEnded，停止蓄力音
    正常状态转移到 Active；切枪则沿正常 PutDownWeapon 转到 WeaponPuttingDown
        -> Charge.EndState 自动执行：清 PlayMaxCharge，StopSimulation，
           退蓄力音、关 ToggleCharging、清震动／扳机反馈、恢复 fire mode
    读回非 Charge/Fire、无 pending input、原弹药未被消费
    等待扳机实际松开后再接受下一次按下
```

不能只调用 ChargeEnded，它只处理一部分音效／timer。不能调用 Charge.EndFire 或普通 StopFire：前者进入 CheckFireShot，后者会调用当前状态 EndFire。不能直接跳 Active 而保留 PendingFire，WarWeapon.Active.BeginState 会遍历 pending 并重新 BeginFire。不能在状态已经进入 Fire／已经生成箭后回滚成“取消未发射”。

另修正上一版描述：CanReleaseFire 检查的是 Pawn 的延迟开火条件，不能写成“未满足最低蓄力就不发”。Charge.BeginState 从 MinChargeTime 开始计数；松键可能立即进 Fire，或置 bPendingEndFire 等待动画允许。Charge.Tick 的疲劳上限是 MaxChargeTime+HoldTimeUntilTired，且完美换弹奖励分支会以 2 倍推进 ChargeTimeCount，因此不能固定说现实时间总是 6.5 秒。

## Frag 与 Smoke 的引信／触发核对

| 场景 | Frag 破片 | Smoke 烟雾 |
| --- | --- | --- |
| 在手准备／旋转 | 查到的 StartGrenadeSpin/PreparedNotify/StartGrenadeThrow 没有启动弹体爆炸计时；不加入额外 cooking | 同样不把准备扳机当烟雾倒计时 |
| 正常投出 | Spawn/Init 建立原弹体和运动，未在这些保存函数里启动 DoExplosion timer | 同左 |
| 正常 Bounce 停止 | 继承 Proj_Grenade.StopBounce：停运动，安排 PlayBeepSound，再 BeginExplosionSequence | **覆盖 StopBounce：bBounce=false、SetPhysics(0)、直接 DoExplosion** |
| 默认停弹后的时长 | 默认 TimeBeforeBeep=.1 s、TimeAfterBeep=1 s，即该普通路径约 1.1 s；若 Base 是 Pawn 会改 1.5+1 s | 普通 StopBounce 不等这一套 .1+1 s 延时，直接进入烟雾效果 |
| 贴附目标 | Proj_Grenade.AttachInit 直接安排 2.5 s 的 DoExplosion，并保存受害者骨等原逻辑 | 专用 AttachInit 直接生成 Emit_SmokeGrenade 并附到目标骨；不是 Frag 的 2.5 s |
| 特殊卡住／重复碰墙 | HitWall 的特定条件会调用 BeginExplosionSequence(0,TimeAfterBeep)，必须保留 | 也不能删除继承的特殊碰撞路径；不能把“普通停止即烟雾”夸大成所有路径都即时 |

StopBounce 可能由低于停止速度阈值或达到反弹计数条件触发，并非“一碰地就算落定”。BeginExplosionSequence 对模拟弹体直接返回，并对已有 DoExplosion timer 有防重复处理；DoExplosion 另有 bHasExploded 门控。LifeSpan=10 是生存期属性，不是投出后十秒引信。后续手势适配只改变初始位置／方向／速度，保留这些原事件。

以上已经把两种触发分支查清到保存脚本控制流；当前进程 ABI、实际时序和物理头显验收仍未执行。不要据此绕过原网权、重复调用效果或重新计时。相关 44 条函数证据、3 个 Bow 状态导出、默认值及来源见 `analysis/weapon-specs-20261028/review-v2/`。

## 模型与交付状态

已完成[离线原创资产包](../../assets/weapon-6dof/offline-v2/README.md)：三款完整实弹、六孔弹巢和复用的原创 Lancer 启动件，共 5 个 GLB。新几何各有两级 LOD、UV、PBR 材质与参考节点；已做闭合边、法线方向、退化面、正体积和确定性生成检查。Snub 原弹匣 314 三角已在私有 analysis 中拆出，保留原 UV／法线／材质索引，未复制到发行资产。

本次没有修改 Native、Playground、逐枪运行状态机、全局 CMake、启动器或候选包，也没有启动游戏／模拟器或占用 IDA 数据库。最终比例、运行时加载、真实手位和原动画接管留待授权的接入阶段；本次模型交付不冒充已接入游戏。
