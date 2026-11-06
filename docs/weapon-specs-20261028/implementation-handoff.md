# 全武器交互实施交接单

2026-10-28，按用户“下令后直接写代码、不再重新设计”补齐。交互决定以 [12 张 v2 规格卡](weapon-cards.md)为准，原行为以[接入口合同](original-entry-map.md)为准。本单冻结修改责任、请求／回执格式、实施次序和具体检查例。**本轮没有实施下面的运行代码修改，也没有启动构建、游戏、XR 或模拟器。**

配套 [33 文件／78 函数位置表](code-locations.md)及[带 SHA256 的索引](code-locations.json)。当前主线仍会改行号与代码；开始实现时按符号重定位、保留其新改动，这是正常源码对齐，不是重做交互设计。不恢复旧 checkout，不另建写入者，不把历史 RVA 当可直接调用的 ABI。

## 1. 已冻结的决定

- W01–08 由各枪首个明确物理动作开始。统一 `ReloadIntent`，不伪造 R3 按键；R3 只是辅助来源。完整流程和原实例接受计时点见规格卡。
- 原枪内计数与部件是否在位分开。旧剩弹保留、未插新件只预留、插入才提交，原匣插回不补弹／不奖励。满匣和无备用仍允许检查，不强制 ForceReload。
- Gnasher 每轮最多 3 发，Boltok 最多补缺额；两枪允许提前闭合，只按实际装入数量普通完成。原计划目标不随提前退出缩小，以免错误发完美奖励。
- Boltok Object03 支架带 Object02/08，Object01 固定；Object02 改用六孔件。80° 是离线设计初值，原动画尚未找到。取消单独退壳杆交互，退空壳联动显示。
- Bow 蓄力中 fresh R3 只分派公开 `cancel_charge(stamp, edge)`，不再换弹，也不用故障入口 interrupt 代替。复用私有 request_abort，`abort_original_charge` 执行清 pending → ChargeEnded → 正常离开 Charge → 自动 EndState 清理。普通松扳机仍发射；两者不共用 StopFire。取消后等待真实松扳机才能重新武装。
- 手雷准备不新增 cooking；Frag/Smoke 保留各自 StopBounce／AttachInit 与特殊 HitWall。原始 XR 手部历史决定出手速度，只做一次空间转换和一次已认证基底速度合成。掩体人工平移只改变出手原点，不注入拟合初速；转腕偏移已计入参考点，不再加一次角速度项。坐标换代和取消规则见[共同规则](../offline-tech-prep-20261028/shared-contracts.md)。
- 原部件走原引擎组件／原骨，原创件使用同一个资产后端。显示、交互检测、游戏库存三者独立；不把手持模型当权威弹药。

## 2. 共用接线位置和新增文件边界

| 工作项 | 现有准确位置 | 需要写的内容 |
| --- | --- | --- |
| I01 帧内调度 | `weapon_equipment_backend.inl::tick`，其内部已有 `nativeequipment_rig::tick(frame)`；`weapon_equipment_rig.inl::acquire`；`weapon_hand_client.inl::observe` | 在同一已认证帧、装备更新及身份读回后调用一个交互调度器；复用同一左右手／身体空间和 current 检查。每帧只分派当前具体武器，原帧失败则中断动作 |
| I02 输入与左手占用 | `left_hand_interaction_owner.h`，现有装备／支撑输入；各枪 `begin_r3` 和 Gnasher `begin_reload` | fresh Grip + 明确运动生成 intent；左手支撑、旧匣、新弹、机构互斥；沿用输入 epoch，不能从“当前按住”补造 press |
| I03 游戏线程调用 | `weapon_action_entry.inl::prepare/invoke_owned/process_event`、`weapon_call_subscriber.inl::begin/cleanup`、`weapon_equipment_domain.inl::domain_ready` | 复用现有 action/call 所有权、异常展开与恢复。`domain_ready` 只拒绝冲突，不是写许可；不直接从 XR 线程 ProcessEvent |
| I04 公共换弹 | `vr_lancer_reload_hold.inl::lancer_clear_timer/lancer_set_fallback/lancer_finish_now/lancer_refresh`；metadata reader/parse | 从旧试验提取可复用函数发现与 timer 操作，写正式事务适配；删除本流程对 8 秒 fallback 的依赖。按当前类与状态查函数，做参数布局检查、定额读回和退出去重 |
| I05 射击门控 | `weapon_lancer_r2_native.inl::tick/yield_for_native_equipment`；`weapon_unified_fire.inl::prepare_current/begin/normal_return/cleanup` | 新增统一的机械可射击查询，作为既有开火允许条件之一；不替换主线武器姿态、瞄准返回值覆盖和原伤害／散布链。Bow、HOD、Troika 仍用自己的使用状态 |
| I06 模型／机构 | Lancer feed_anchor/insertion/magazine_pose；现有组件读取、原骨唯一写入端；`weapon_lancer_component_probe.inl::tick` 为历史观测依据 | 固定入口挂枪体；原骨集合每帧一次写入，保留恢复快照；手持件与枪上件显隐互斥；新增件共享资产后端和所有者 |
| I07 手雷发射作用域 | `weapon_unified_fire.inl::typed_result/normal_return` 的类型校验方式、现有 call subscriber | 为当前本地 ThrowGrenade 建独立调用作用域，替换出手位置、Init 方向和初速 Getter；保留 Spawn→ConsumeAmmo→Init→缓存／通知的原顺序，不把枪口覆盖直接复用给手雷 |
| I08 主线集成 | `playground.cpp` 的 include／已有 post-Tick 接点、`tools/playground/CMakeLists.txt` 的当前目标 | 最后添加适配 include／按需编译依赖；使用届时主线配置、固定候选路径与回滚。不要重建不同的全局运行配置或建立第二个 runner |

预计新增四个实现单元，名称固定供开工使用；本轮尚未创建这些运行源码：

1. `tools/playground/weapon_interaction_dispatch.inl`：选择当前 W01–12、输入意图、调用顺序、机械可射击状态和中断收尾。
2. `tools/playground/weapon_reload_adapter.inl`：ForceReload 接受、timer／Notify 所有权、原库存读取与定额提交、奖励／退出唯一执行。
3. `tools/playground/weapon_throw_adapter.inl`：准备／释放请求、短调用作用域、Getter 返回值与 Init 方向、原投后通知和弹体读回。
4. `tools/playground/weapon_part_assets.inl`：原资源组件与原创件统一的创建／更新／显隐／销毁；不承担弹药记账。

公共请求类型放 `tools/vr-common/weapon_reload_transaction.h`；逐枪逻辑继续放已有对应头文件。不要新建一套手部协议或把全部机械塞进 Lancer 运行文件。

## 3. 逐枪代码差额

以下函数均已在源码定位；精确行号和源码哈希在 code-locations。旧字段名可以保留兼容包装，但新路径必须使用真实物理 intent，不能向旧字段灌假的 R3 序号。

| 武器 | 现有逻辑和具体函数 | 修改任务 |
| --- | --- | --- |
| W01 Lancer | `weapon_reload_model.h::begin/insert/finish/cycle/cancel`；Lancer feed/insertion/magazine_pose | `begin` 现将旧 L 加到 reserve 并清 loaded，需区分原库存、旧匣外观与新弹预留；新增旧匣重插取消／无补弹检查。首动作改抓匣拉出，最后采用联动关闭。`perfect_end_ticks<=0` 不能再直接使普通换弹无效，增加窗口未知时普通结果 |
| W02 Hammerburst | `weapon_hammerburst_reload.h::begin_r3/confirm_latch_open/insert_magazine/confirm_latch_closed` | 改成通用 start intent；confirm_latch_open 同样有旧弹回池逻辑，接共用记账，不能把投影值写回原库存；保留锁扣先开后关和真正旧匣归位路径 |
| W03 Gnasher | `weapon_gnasher_mechanics.h::begin_reload/acknowledge_inventory/observe/acknowledge_cycle` | 取首发开始；新增 `close_requested`，在无手持件／无待确认提交时允许提前进入 lever。保留 target，只有 inserted==target 才能完美；原 ShotgunCocking timer 与一次杠杆闭锁去重 |
| W04 Longshot | `weapon_longshot_reload.h::begin_r3/complete_step/acknowledge_inventory/acknowledge_completion` | 开枪栓的 intent 开始；按既有逐步 request/receipt 模式接 q=1。退壳回执自动推动 eject_case，不额外抓壳；保留子类退镜清理 |
| W05 Snub | `weapon_snub_reload.h::begin_r3/confirm_ejection/insert_magazine/move_original_parts` | 通用 intent；confirm_ejection 也有旧弹回池，适配成原库存与预留分离；事务开始时固定 empty/partial 分支；原匣插回退出不授奖励；空仓滑套与侧翼联动 |
| W06 Boltok | `weapon_boltok_reload.h::begin_r3/advance/insert_ammunition/acknowledge_inventory` | 原计划内补 `closing_phase` 和六槽 occupancy；唯一 item/slot 提交，不能同孔重加。提前闭合只跳过剩余插入段、执行实际关闭段，保留计划目标／已提交数。绑定 Object03/02/08 与六孔替换件，Object01 固定 |
| W07 Boomshot | `weapon_boomshot_interaction.h::begin_r3/insert_ammunition/acknowledge_inventory/acknowledge_completion` | 首次开机构触发；固定原 MagazineMesh0 carrier，不再要求重新选择弹体资源；一个组件 q=1，原 ProjectileFire 仍决定飞行类与奖励 |
| W08 Torque Bow | `weapon_torque_bow_interaction.h::begin_r3/trigger/request_abort/take_request/acknowledge` | 取箭开始，装箭提交和原完成分开；`abort_original_charge` 接 C3 已定取消组合，`end_original_fire` 专门正常松扳机；pending 调用先读回再中止，失追不得形成松键发箭。反映原 charge 计数与 Pawn 延迟语义 |
| W09 Hammer of Dawn | `weapon_hammer_of_dawn_interaction.h::observe/begin_r3/take_request/acknowledge` | 本已确认具体类不配置普通换弹，begin_r3 保持不适用；BeginFire/EndFire、实际 trace 与 BeamManager 共用武器方向，完整读取停止光束／音效结果 |
| W10 Troika | `weapon_troika_interaction.h::edge/update_aim/take_request/acknowledge` | 绑定炮位 Claim/Leave、双握把、原角限制／双枪口；不启用手动补弹机械计划；保留原自动 Reloading、Loader 差异，失握终止拥有的开火 |
| W11 Frag | `weapon_bolo_grenade_interaction.h::bind/take_request/cancel/acknowledge` | 配置 fuse_owner=original_projectile；prepare 回执不得有 projectile 或活动引信；投掷只接原 Frag。普通停弹、贴附、特殊碰墙原回调自行处理；本地接收实际弹体后不能再保持虚拟 held |
| W12 Smoke | `weapon_smoke_grenade_interaction.h::bind/take_request/cancel/acknowledge` | 同样使用 original_projectile，但生成类／弹耗分开；保留专用 StopBounce/AttachInit/GrenadeExplosion，不套 Frag 引信。类型切换清旧 token／采样窗口 |

`grenade_throw.h::GrenadeThrowTracker::solve` 有两个直接改点：默认 minimum_speed_mps=.5 和位移<.04 的拒绝。支持有效低速／静止放落时移除玩法最小位移，最低速度设 0；保留样本数、时间跨度、有限值、最大样本间隔、异常速度上限与空间代检查。已有 140 ms 窗口作为首版参数，不再重新选择投掷算法。`grenade_world_launch` 只进行一次轴／单位转换与基底速度合成；零速度 Init 方向用手中物件朝向，不对零向量归一化，也不用头部方向。

## 4. 公共接口与状态契约

接口沿用现有 FrameKey、ObjectIdentity、当前世界／装备代；下面是新增字段的实现要求，不是第二套身份系统。

| 类型 | 必须包含的字段与语义 |
| --- | --- |
| `ReloadIntent` | 当前 frame/weapon identity；严格递增 intent_id；source=`physical` 或 `assist`；kind=`extract_magazine/open_latch/open_breech/take_round/open_yoke/open_carrier/take_arrow`；样本序号、motion epoch、首次有效动作 game tick；原计划版本 |
| `ReloadSession` | 原接受 serial／start tick、开始 L/S/C、目标 q、reserved、committed、唯一 held item、old magazine id、feed_present、mechanism_closed、原窗口 valid/end、当前 phase；原 timer/Notify 所有权证据 |
| `InventoryCommitRequest` | 身份／serial、唯一 token、item_id／slot_id、before revision、before L/S、delta q、expected L+q/S−q。数值是原库存，不是已减去预留的可用投影 |
| `InventoryReceipt` | 同 token、实际前后计数与 revision、原调用结果、执行序号。未知调用结果先读回，不能直接重试；一半 setter 成功要补偿到明确状态 |
| `PartMotionReceipt` | 计划／phase／part 集合、运动开始和终点样本、连续性、明确 fresh Grip、轴向行程／闭锁端点、当前原件身份；显示到位不能自称库存提交 |
| `ReloadFinishRequest` | serial、结束动作可信 tick、全部必要机械回执、已提交 inventory revision、ordinary/perfect/cancel、是否提前闭合；禁止再次转移库存 |
| `InteractionGate` | 当前实例、feed_present、mechanism_closed、post_shot_cycle_complete、original_ready、owned_action_pending。机械通过仍需既有原游戏允许条件 |
| `ThrowPlan` | 当前 W11/W12、release token、冻结身体／参考空间代、手中弹体 reference offset、位置／方向／初速、基底速度是否已合成、原库存 revision、有效期限和原准备 serial |
| `ThrowReceipt` | token、实际 Spawn 的类／对象、初始 Location/Velocity/InitialVX/Y/Z、生成数、实际前后库存、原通知是否恰好一次；引信由原弹体回调拥有 |
| `WeaponPartAssetBackend` | `create(asset_id,owner_generation)`、`set_pose(handle,world_transform)`、`set_visible`、`destroy`；返回带代数的 handle／原组件身份。单位转换只在后端一次完成，失败不创建可抓实物 |

换弹共用状态顺序：`idle → intent_pending → original_accepted → open/extract → reserve → insert_pending → inventory_confirmed → close → finish_pending → complete`。逐发枪在 reserve 至 inventory_confirmed 之间循环。状态 machine 可复用现有枚举，不要求重写所有类型；关键是原回执边界不能合并掉。

检查／原匣归位另走 `inspection_open → restore_original_item → close → idle`，不触发补弹和奖励。中断时保留原实例已提交量／未闭合机械状态，释放未提交预留；换世界或实例代变更废弃旧句柄，不把旧状态套到新枪。

## 5. 资产装配和加载工作已定到什么程度

交付路径：`assets/weapon-6dof/offline-v2/`；四件新几何双 LOD、三款实弹 + 六孔件，另有复用的 Lancer 启动件。单位、UV、PBR、命名、抓点／插入节点和校验器已齐。六孔件 + 实弹的六个装配 transform 在 `boltok-assembly.json`；底缘退后 3 mm，六孔及弹体配合已由实际 GLB 检查。

- 原件：继续原 StaticMeshComponent／SkeletalMesh 骨控制，使用具体原资源路径；Snub 原匣私有拆件已有 UV／法线／材质归属。原件模型不复制进发行 ZIP。
- 新件：GLB 作为唯一离线制作源，资产后端只接收经验证的顶点／索引／法线／UV／材质／LOD／节点数据。运行目标固定为引擎世界中的三维组件，继承其双眼投影与场景深度，不以 XR Quad 或覆盖层替代。
- **已确定、待编码的底层工作：** 统一采用原 UObject 构造 → 临时 StaticMesh → 原 LOD 和顶点初始化器 → 原组件发布 → 原资源 fence 与 GC。入口、字节证据、根引用租约、数据格式和首件验证见[资产后端](../offline-tech-prep-20261028/asset-backend.md)。配套11个网格输入已经生成并独立检查；尚未写游戏加载器。
- Gnasher完整实弹是唯一首件。输入使用60字节角点格式，保留法线；不调用会重新计算法线的RawTriangles Build。实际版本/ABI和GPU结果由实施阶段验证，失败回滚自己的事务并报告对应入口，不能改成第二套绘制器或回退主线。
- LOD 默认只实例化 LOD0，远距才换 LOD1，不能同时加载两个 scene 叠画。弹巢碰撞按孔位分区，禁止单个凸包堵住六孔；几何只是表现，插入依旧需连续行程和唯一槽位判断。

## 6. 开工次序与每阶段可检查结果

| 次序 | 编码内容 | 完成的具体证据 |
| --- | --- | --- |
| P1 | 通用 intent、普通窗口未知分支、权威库存／预留分离、旧匣归位、部分闭合；修正既有纯逻辑及相应窄用例 | 编译及相关离线用例通过；不需要游戏来决定交互方案 |
| P2 | 公共 reload adapter + Lancer 的原接受／timer／插入 q／一次退出 | L=20/S=5 只到25；等待不自动补60；切枪不重加；R3 和抓匣合并一次事务 |
| P3 | 原件组件／机构适配；Snub/Hammerburst；同时首件 Gnasher 原创资产后端 | 原匣在手／枪互斥；空仓与半仓正确；原创件实际世界渲染／深度／生命周期明确 |
| P4 | Gnasher/Longshot/Boltok/Boomshot 逐枪机械与新件装配 | 各自定额装填、提前关闭、射后循环和原效果读回；Boltok 原动画未找到时采用已经明确的设计驱动，并保留恢复 |
| P5 | Bow 取消、HOD trace/stop、Troika 炮位／双握把 | 特殊状态能停止且不漏弹／残留光束／占用，不套通用 StopFire |
| P6 | 共用 throw adapter + Frag，再同接口 Smoke | 原弹体确实消费手部初速与一次弹耗；两套原引信／效果独立验证 |
| P7 | 按届时主线配置编译、标识候选、正常收尾及人工头显检查 | 源码、编译、候选交付、基本运行和头显反馈分别记录 |

P1 至 P7 是开工后的执行顺序，不是现在的构建或运行授权。后续基本自测继续服从唯一协调者和当时有效的保护／范围，不恢复旧全量矩阵；物理手感以用户头显反馈为准。

## 7. 已准备的关键检查例

| 对象 | 输入与期望 |
| --- | --- |
| Lancer | L20/S5，新匣插入 L25/S0；同 token 重放不变；原匣插回 L20/S5、无奖励；满匣拔出期间禁射 |
| Hammerburst | 锁扣未开不能拔／插；L70/S3 完成为73；关闭仅完成一次；Notify_Throw 不销毁已转交新件 |
| Gnasher | L2/S10，一轮最多到5；只装1后闭锁到3、普通；原射后 timer 不能替代手动杠杆 |
| Longshot | L0/S1，插入后 L1/S0、未闭锁禁射；退出原瞄准清理执行；重复完成不加第二发 |
| Snub | L0/S4，插后4但需拉放滑套；L4/S2，插后6免拉；旧匣重插无 refill |
| Boltok | L2/S3，只补1后关为3；补完为5；同孔重复插入不加；支架开启时固定后盘不跟动 |
| Boomshot | q=1 使用原 carrier；重复 Notify 不生成第二件；玩家 projectile 与原奖励保留 |
| Torque Bow | Charge 中取消后不 Fire／不扣弹／不重新 BeginFire；普通松键仍可 Fire；已 Fire 不回滚退款 |
| Hammer of Dawn | 原非法区域仍不能照射；松键／失追同时终止目标、Beam 和循环音；转头不改武器目标 |
| Troika | 领取失败不绑手；任一握把松开停止拥有的开火；退出不遗留占用；Loader 恢复时间沿原分支 |
| Frag | 有效静止／轻抛可生成；重复 release 一枚；普通 StopBounce 使用原默认延时，特殊 HitWall／贴附分别检查 |
| Smoke | 只扣烟雾库存；普通 StopBounce 直接进烟雾；贴附原 emitter；绝不误生成 Frag 或复用 Frag timer |
| 所有物件 | 失追／回中／Snap 转向／旧世界代不补造运动；未提交物件回收，已提交不回滚；句柄销毁与动作调用退出各一次 |

已有 `weapon_*_test.cpp` 与 `grenade_throw_test.cpp` 扩展对应缺口，针对新增语义写窄用例，不为每个字段另建测试工程。文件索引可定位到现有类型和入口，原函数／源码哈希可定位到保存证据；开工不需要重新征询键位、机械步骤或模型名单。
