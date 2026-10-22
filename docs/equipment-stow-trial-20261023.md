# 身体槽暂收与原按键交还试验

2026-10-23，独立开发分支 `codex/weapon-6dof`。本记录只覆盖默认关闭的固定测试场候选；尚未替换普通启动器，也不代表完整身体挂枪、Quest 或头显验收。

## 已有原版依据

购买版 PID 18068 的已保存观察显示，人物的 `SetWeaponTemporarilyHolstered(true)` 会把当前 Lancer 的 Weapon.Mesh 从人物 Mesh 脱离；传入 false 后，同一组件恢复到右手 `b_MF_Weapon_R_End`。当前 Weapon、库存链、四槽、Active 状态保持不变，武器 `bTemporaryHolster` 始终为 false。不能用这个旗标判断人物助手已经完成收枪，也不能用它标记谁拥有当前暂收。

旧 runner 曾错误等待该旗标变成 true，所以原收据保持失败。另存的机械 audit 只总结已记录的附件和控制链数据，没有重写运行结论。证据位于忽略的 `analysis/hand-stow-observe-1023d/`。

随后检查旧自由手／动画记录共 1,003 帧，并在未 GiveTo、未清空库存、未启动 IK/XR writer 的初始 arena 另观察一次购买版 PID 28452。初始人物已经持有 Hammerburst，3 组外部读数和 20 帧探针均为同一非零 Weapon。因此**没有取得真正 `Pawn.Weapon == 0` 的基线**，不会据此开放零武器的手骨写入。该局原观察／清理成功：Shutdown=0，两入口还原匹配，精确游戏进程退出，静音守护器直到游戏消失后才结束。回执位于忽略的 `analysis/hand-unarmed-baseline-1023a/`，其中 `complete_natural_weapon_zero_baseline=false` 保留。

当前暂收候选采用已观察的原版助手，保留当前武器对象；它与真正把库存当前武器清空是不同的状态。

## 候选行为

`GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST` 默认关闭，要求原库存 Switch／Intent、R2／L2 与现有 CallFunction 动作观察器同时存在。候选拒绝其他私有 IK、枪体、弹匣、装填和电锯 writer，以便单独验证附件和输入交还。它只允许固定自有 arena 的 Lancer／Hammerburst；Hammerburst 仍须取得自己当前实例的往返证据，不能沿用 Lancer 的结果。

身体槽点按先在 `WeaponEquipmentModel` 的副本中提议。只有原调用及读回满足要求，才提交持枪／收枪状态。实际反射失败不能留下“模型已经持有”的乐观状态。

暂收前要求武器原本具有唯一的右手 attachment，记录其骨名及相对位置、旋转、缩放的原字节；记录八个完整对象身份，包括 WorldInfo、Level、PC、Pawn、InvManager、人物 Mesh、Weapon 和 Weapon.Mesh，以及四槽和动作代次。原助手调用前先记 dirty，调用返回后重新验证同实例及 detached。函数报失败也可能已改动附件，不能仅凭返回值丢弃 journal。

收枪及切换前同步交还本桥拥有的 R2／L2，并确认原游戏按键真的已经抬起。没有归属于本桥的原按键不会被代发 Release；仍被外部按住时拒绝本次身体槽操作。当前 R2 安全交还只支持已认证的 Hammerburst 路径，不能把 Torque Bow 等“松开即发射”的武器当成普通自动枪释放。

干净 lease 下取回当前武器时，要求组件已经在原手挂点。如果原游戏已把它卸下，VR 模型为 neutral 也不能擅自调用 false 拔回。

## 动作接管和恢复权

同一武器、同一库存且仍 detached，不能证明暂收仍由本桥拥有。原版再调用一次 true 时，这些字段可能完全不变。

候选具有独立的 `before_action`，分别接入反射请求、ProcessEvent 和 CallFunction。它不借用手骨试验的 active 状态。已知相关原动作到来时，顺序为：按进入动作前仍有效的 journal 调 false、确认原 attachment、清 journal／重置模型、递增代次，随后转发原动作。若原动作本身是 true，最终仍由这次原动作保持收枪；旧 journal 不会之后再补 false。

观察中失去身份认证、出现 foreign／unknown attachment，或恢复后无法确认原 attachment，都会永久撤销本次 lease 的写权限。即使稍后又看到同实例 detached，也不能重新取得恢复权。撤销后仅允许只读确认已经回到完整原 attachment 并清 dirty。测试覆盖 foreign → detached 和 unknown → detached，不再重试原助手。无法确认恢复时保留 dirty／失败，拒绝正常卸钩。

本试验只支持原地、短时、无剧情／拾取／换武器并发的往返，以及显式注入的一次外部 true。动作名表不是所有 native 直调的完整覆盖证明；不能把这个有限记录提升成全剧情的长期暂收保证。

## R2／L2 输入交还

纯模块新增 `yield(context, execute)`，不依赖下一张 XR 帧。`allow_press=false` 时，已拥有的按键先同步交还，再考虑重复／失效输入。R2 同时检查按下时锁存的和当前的安全释放许可；L2 拒绝原版 toggle-to-aim。身份变化或执行失败锁定 abort，保留尚未交还的 ownership，不自动重发。

交还会封存旧边沿。重新允许输入后的第一张新鲜匹配帧只建立 baseline，不补发 tap 或 press。如果该帧的扳机 active、值不大于 0.35 且原游戏按键已抬起，就已证明当前松开，后续一次新按压即可工作；持续按住、中段值、inactive 的默认零值、原按键仍 down 都不能解除门禁。也可以用之后单独的新 Release 边沿确认松开。这样既避免取回后持续按压自动开火，也不要求原本已经松开的玩家多按一次。

冷启动尚无任何输入／实例绑定时，合法 ordinary context 只能封存门禁，不发 Release。内部输入重置保留已绑定过的实例身份，不能借失去 XR 输入重回冷启动许可。

当前 Context 无法识别外部输入在本桥按下以后又加入同一原按键；独占输入源仍是有限试验的调用方条件。后台物理输入保持禁用，不据此宣称生产输入归属已经完全解决。

## 有界停止与验证

单次暂收最长 10 秒。Stop 或 Shutdown 请求游戏线程处理原 journal；Shutdown 在 hooks 仍启用时等待 dirty 清除及在途 callbacks 返回，随后才卸钩。游戏线程进入 Shutdown 会先排队停止并返回 51，避免在生命周期锁上阻塞外部正在等待的调用。

候选导出 `PlaygroundEquipmentStowStatus` 和 `PlaygroundEquipmentStowStop`。状态低位 5 为暂收、6 为恢复、8 为失败，`0x10000` 表示 dirty。原槽请求状态新增 10 为验证暂收、11 为验证持在原手、12 为本次暂收／取回被拒绝。普通关闭构建不导出这两个 Stow 接口。

首局候选 DLL SHA256 为 `016b587cd95e4e01f99bd7e1b7c08b7592b33fe617f6fd8cb572a883a1a95ef7`，保留当时 156 份源码。修复确认接口竞态后的当前候选为 `81601d4b1b4cf70540c90cbe28aa4c28476303f5075eb3fe039cb0b73280cdec`，封存 158 份源码。两版各自位于忽略的 `analysis/weapon-stow-candidates/<SHA256>/`，原清单、纯检查输出和默认构建导出检查一起保存，后版不覆盖前版证据。

已通过 VS18 x64 Release `/W4 /WX` 候选构建、默认关闭构建和导出隔离；缺少 CallFunction 依赖的配置按预期拒绝。六个启用 assertions 的检查程序通过：Stow journal、请求 mailbox、原 R2、原 L2、CallFunction 转发／还原及动画观察。Mailbox 回归实际暂停 consumer 于 observe 后、complete 前，确认 Status 保持 0，且第二 producer 不能加入；另验证两个 producer 竞争只准一个。Astra Max 复核发现的恢复权重取和 Shutdown 锁顺序问题已修复；本轮 subagent 均未启用 Fast。

首局购买版 PID 47872 已取得 Hammerburst 往返及外部交还证据：实际槽位日志依次为 `11 → 10 → 11 → 10 → 11`，暂收后组件从人物附件数组消失，恢复后原 44 字节完全一致。外部 true 的调用窗口内，内部 false、called=1、原附件读回、clean journal 和 action 转发顺序均记录。随后 35 份读取持续 11.698669 秒保持 detached／dirty=0，跨过原 10 秒 deadline，期间没有内部 helper 调用；runner 最后为自己注入的外部 true 调 false，恢复原附件。49 份外部读取中的武器／库存／Active 及八控件身份和强度位模式保持。

该局原收据仍为失败：私有状态 API 在 `queued_slot.exchange(4)` 取走请求后立即显示队列空，但结果尚未更新，runner 读到上次遗留的 10，提前终止；本次真实完成日志随后正确为 11。这是确认接口的竞态，不能把日志中正确取回追认为整局通过。清理时已经是 clean，因此没有覆盖计划中的 dirty Shutdown。实际 Shutdown=0、之后原 attachment 44 字节一致，两入口原字节独立匹配，精确游戏进程退出，指定静音 Guard 到游戏消失后才关闭，九个原已静音会话无错误／无待恢复。原回执与机械 audit 保留在忽略的 `analysis/equipment-stow-runtime-1023a/`。

后续修复使外部请求在 reserved／queued／processing 全阶段持续返回进行中，直至本次调用、读回、模型更新及完成日志结束才发布结果；处理中也拒绝第二外部请求。测试同时要求本请求之后新增的唯一完成日志作独立确认。Enqueue 与 Shutdown 共用生命周期锁，游戏线程在拿锁前返回，确保只在 producer／consumer 均退出后重置 mailbox。

修复后的购买版 PID 36000 补测通过，原收据 `passed=true`、无清理错误。同一初始 Hammerburst 槽 3 完成完整往返、外部 true 交还、35 份样本保持 11.5607865 秒不被旧 deadline 拔回、外部 false 后重新取回／暂收；最后在 `0x10005`（暂收且 dirty）时直接 Shutdown。日志确认先恢复原 44 字节挂点、clean／fault=0／revoked=0，随后才还原 CallFunction Hook；Shutdown=0，两个入口原字节独立匹配，精确游戏进程退出、owner=0。九个原已静音会话无静音／读取失败、无待恢复；守护器到游戏消失后结束。启动前私有 ini 原字节快照另存，没有额外调整窗口或配置。证据位于忽略的 `analysis/equipment-stow-runtime-1023b/`，原收据 SHA256 `8c1b57e89ae06553976a615e0553374f23f8444395eb648bd9864398c03ca273`。

这两局均只发外部槽请求，R2／L2 mode 为 off。当时 XR intent 的 pending 检查与读取还不是一个原子 claim；与外部 producer 并发时可能分别执行两个请求。

## 持续开火时的暂收等待

随后购买版 PID 57152 用独立模拟输入发布器实际按下 R2／L2，原字段均为 down，Hammerburst 的 AmmoUsedCount 从 0 增至 6；同局读到 `WeaponFiring`，身体槽请求前为 `BurstDelaying`，其余 idle 条件满足。旧候选返回槽结果 12，未调用暂收助手。原收据在 `analysis/equipment-stow-buttons-runtime-1023a/` 保持 **passed=false**；缺少 yield 分阶段日志，不能把拒绝唯一归因于 Active 条件，也不能用随后 publisher 松键／Shutdown 后的 up 值证明当时 yield 已成功。该局 Shutdown=0、原挂点 44 字节及两入口匹配、发布器和精确游戏进程退出、九个原已静音会话无错误／无待恢复。

修订候选先封住新 press，再分别交还本桥拥有的按键，严格记录每手调用前后的 known／held／ownership、实际 Release 调用与结果。Never-owned down 直接拒绝。成功交还但按钮尚未 up 只能基于该手原 ownership 及成功 Release 回执进入 settling，不能把未知读数当作抬键。

若同一 Hammerburst 仍处于已观察的 `WeaponFiring`／`BurstDelaying`，以同一完整身份和原手挂点等待原版自然返回 Active，最多 1,000 毫秒，不逐 Tick 续期，不改 timer 或强制 state。等待本身没有附件恢复权，不调用 true／false；完成前重新认证八对象、库存、44 字节挂点、idle 条件、Active 和两原键 up，才可 acquire 暂收 journal。等待期间原键的 down 位只能减少，新 down、unknown、未知状态、固定动作或身份丢失均取消。

只有绑定的同一 Hammerburst 在许可状态中的 BeginState／EndState 可以自然完成；Pawn／PC／库存对象上的同名动作没有该豁免。其他已知原动作在转发前取消等待，原动作结束也不会自动复活该请求。Stop／Shutdown 独立等待 pending 可见标记清除，不因 dirty=0 提前卸钩。

原 1 秒期限从本次请求进入时固定，包含最初的交还和查询时间。真正执行 true 前，在参数构造及前置日志 flush 之后再次认证完整 expected Key、原挂点、fresh idle、Active、按键 up，最后检查 Stop／Shutdown／原期限，以该检查作为提交决定；随后到来的停止由 dirty journal 排空恢复。如果这个最终检查拒绝，立即 revoke 预记的 journal，保证“尚未调用 true”时新出现的 detached 不会被误当作自己的暂收而补发 false；原 hand 只能只读清除 dirty，foreign detached 保持撤权并拒绝正常卸钩。新增回归分别覆盖这两个分支。

外部槽请求和 XR intent 现共用 mailbox 的原子 processing claim，先占用才能读 intent，异步等待期间持续保持请求占用；完成读回、模型更新和唯一终行日志后才发布结果。私有 StowStatus 低位 13 表示等待，SwitchStatus 在等待中持续为 0，终值仍为 10／12。纯模块覆盖真实线程竞争、处理期间的排他性、身份复现不复活、deadline／时钟边界和停止优先于 ready。

本修订候选 SHA256 为 `dc870cd148b6dc88b5fbd1942c287de4fa224774c94b6536560491fd761111f2`，160 份源码及七个纯检查结果封存在对应忽略目录；source.json SHA256 为 `986f6e032f55bdbc58b3ca42681f0fbc5b7d0ef67e2a7403b1e9a56b061649ad`。严格候选／默认关闭构建和导出隔离通过；Astra Max 最终源码复核未发现剩余确定阻断。后台持键联调单独使用新 tag，旧失败回执不改写。实际 RGrip 空间命中、肩部代理渲染、与持续双手 IK 共存及头显效果均仍待完成。
