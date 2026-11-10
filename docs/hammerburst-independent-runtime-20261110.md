# 兽族步枪：独立测试接线与当前失败记录

工作区：`C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。本记录不改变整枪开发目标或验收范围。

## 当前运行限制

2026-11-10 收到主人经统筹转达的新要求：独立启动仍发生抢前台，暂停新的 game/XR 启动，不自行重启测试焦点，继续离线开发。本次没有启动游戏或模拟器；`hb-1110-181318` 仅完成准备和制品校验，不能算启动、实际武器回归或头显验收通过。

后续恢复启动时仍须遵守 `docs/weapon-development-20261109/independent-runtime-20261110.md`，并采用修复且实际验证过的后台/静音入口。禁止重新 stage/restore 共用 solo 实例，也不以环境变量存在代替实际后台行为验证。

## 本次实际修改

新增 `analysis/hammerburst-20261108/hammerburst_isolated_operator.py`。它只适配本枪的既有动作及只读观察脚本，不启动游戏，不修改共享脚本或生产武器机制。

- 输入命令、动作输出、launcher 观察、Playground 日志、停止入口全部指向传入的独立实例。
- 两个原先通过全机进程枚举寻找共享 XR 的读数脚本，改为验证本实例报告中的 XR PID、creation 和冻结 EXE 路径/哈希。
- 拒绝外实例输出路径、共享实例名、任意辅助脚本、Source 非 1、被复用的进程身份和变化的 XR 制品。
- 动作循环只核对已绑定的 owner/game/XR 进程身份，不在每次手部输入时重读 manifest 或整份 EXE，避免给原有输入时效链增加文件开销。
- 复用已有按 game/XR PID+creation 录制准确 HWND 的录像器；未新增桌面截图或全局输入。

新增 `check_isolated_operator.py`，14 项离线测试全部通过，覆盖身份复用、外实例/XR、制品变化、物理输入源、输出越界、辅助脚本越权、准确停止目标和适配代码编译。五份适配后的原脚本编译成功。这些只证明离线隔离路由及拒绝条件，不证明运行时录制、换弹或射击成功。

## 保留的实际游戏失败

旧 return-observation-r1，game34780：起始六发有效且来源一致；下一轮原始按下 event7 被捕获，但 frame231 的输入通过 `frame_active` 判为 inactive，库存 72/78 未变化。记录的复制输入年龄为 97.7204ms，复制耗时 15.3889ms；不能把它写成已观测到超过100ms，也不能把源中 trigger=1 写成实体松键。具体失败的 Frame 时间戳/形状条件尚未定位。没有到达本次新增的换弹后完整回射观察分支，整枪仍未通过。

证据：`analysis/hammerburst-20261108/runtime-34780-first-failure-analysis.json`。原 operator=1、wrapper=1 保留。游戏/Native/XR 正常收尾，但旧 wrapper 的全机 idle 检查将离线编码误当占用，阻止自动恢复。随后在没有其他活动实例且匹配旧候选的条件下，已准确恢复十项原始基线；恢复记录为 `standalone-return-observation-r1-recovery.json`，SHA256 `031468910137e9ca47f270420200eee766b4dce8cf007dc0aae90cbc65188e42`。手工恢复成功不改写原 wrapper 退出码。

更早 game56252 已实际完成空仓、取替换匣、入槽、锁扣关闭及原版 complete，回射六条真实射线；最后库存读数仍缺，不因此宣称完整回射闭环通过。原有四态护木、原匣返座、普通/完美/取消换弹、有限库存、打断退出、完整拟合及交付要求继续保留。下一次实际测试应在后台路径修复后进行，并继续定位当前首发源失效，不能重跑不变代码碰运气或放宽100/20/250ms保护。

## 枪栓结论

原版 Locust_Hammerburst 的待机/换弹/换弹失败动画只有 Root、MagLatch、Mag，未提供独立可操作枪栓/拉机柄。保留真实锁扣与弹匣交互，不虚构拉栓部件。详见 `analysis/hammerburst-20261108/hammerburst-bolt-original-audit.json`。

## 后续离线定位：实际输入检查时刻

检查上一轮冻结源码发现：`native_lancer_r2` 的 `observed_ns` / `copied_input_age_ns` 使用瞄准处理前的 `now`，而 `Input::advance` 使用瞄准处理之后再次取得的 clock。因此日志97.7204ms只能描述较早观察点，不能证明真实输入检查时仍未超过100ms。相反，也不能从总 action_elapsed 反推真实检查一定超时：后续取消/服务/输出也包含在总耗时内。旧记录缺失该检查点，具体失败仍未证实。

已在原 return-observation-r1 冻结源码上准备并完整编译 `input-admission-observation-r1`，只改变 `weapon_lancer_r2_native.inl`：给原本就存在的检查时刻取变量，原样传入 Input，再在现有有界日志中写出该时刻、相对早观察点的间隔、Frame/立体/控制器原始时间戳及纯状态判定。没有额外读 clock，没有改动100ms/20ms/250ms或身份/输入/库存保护，也没有提前输入检查、更新原始按下时刻或重放按下。

- 补丁：`docs/hammerburst-input-admission-clock.patch`。
- 构建记录：`analysis/hammerburst-20261108/input-admission-observation-r1/prepared.json`。
- 完整 Playground 编译/链接成功，DLL SHA256 `e48de17d716fcfbf3fcd03fa050841e492cbd3f2c04fae46e6eeacf3318037c5`。Native/XR未重建，未替换任何运行实例或公共包。
- 新增 `weapon_hammerburst_input_clock_test.cpp`：12个边界用例、106项检查通过，覆盖早观察98ms但实际检查过期、交互标志丢失、立体时间戳过期、控制器时间戳过期、独立sync过期、Frame时刻过期；全部拒绝且无原版按下调用。两种Source均覆盖。测试使用的Input、Frame有效性头和fixture与上一轮冻结源码逐字节相同。
- 原始34780关键图审阅补齐：1张配对总览和6张原始接触图已亲自查看。保留近脸枪体/右眼遮挡、simulator00595右侧大片白色区域；不宣称完整贴合或舒适度通过。

此候选是用于查清首个实际失败的观察改动，尚未进行游戏验证，不是已修复首发问题的证据。当前仍按要求暂停新的game/XR启动，完整逻辑、护木四态、换弹各分支及拟合验收继续未完成。

## 完美换弹资格记录缺失：源码修复

离线核对实际生产调用链发现一个独立于首发时效失败的缺口：`perfect_reward` 明确要求 `RewardReceipt.eligibility_known` 和同实例资格 owner；原兽族步枪开始流程只调用 `begin`，没有任何 `capture_reward_eligibility` 调用，而 reward 每次 reset 都清零。因此在原流程下，即使及时完成真实机械闭合，也无法通过完美奖励前置检查。这是源码层面确定的缺失；尚无该修复的游戏实录。

新增生产适配 `weapon_hammerburst_reload_begin.inl`，在真实装填开始前、库存仍为本轮 opening 数量时调用现有只读资格查询，然后再请求原版开始。资格记录先保存在该次尝试的局部对象中，只有原版已接受或进入需恢复状态后才交给本轮；原版拒绝后的下一次尝试必须重新查询，不能沿用旧资格。满仓或零备用的无补弹检查不调用原版开始或资格查询；没有增加奖励、改写库存、更新期限或放宽身份检查。

- 实际生产 helper 的10个用例、28项检查通过：半仓、空仓、满仓、零备用、无限备用、查询失败、开始拒绝后资格变化、需恢复的原接受、旧回执替换和无效库存。
- 冻结 `reload-eligibility-r1` 在上一观察候选上仅改变HB runtime开始调用并加入helper；完整 Playground 编译链接成功。DLL SHA256 `f0d5b661ac76de5a879ee7953cbd6892535659812ea8048f1b242874d623c0d1`，构建记录位于同目录 `prepared.json`。
- 共享 runtime 已接入同一helper；为保留该共享文件已有差异，单独保存本次集成补丁 `docs/hammerburst-reload-eligibility.patch`，未把其他差异混入本次提交。
- 这解决资格记录缺失，不能证明完美奖励实机成功，也没有解决前述首发输入失效或切枪保存机械状态的要求。

本轮曾收到后台修复可恢复通知，随后又收到主人“又到前台”的反馈及撤回通知。新实例尚未启动。以 `tools/multi-instance/automatic-runtime-pause.json` 为当前停止新自动启动的边界，只有主人明确恢复后才运行。新本枪单次运行脚本检查该标记，运行中若出现标记即停止本方动作并正常退出；不移除或绕过它。独立输入适配另增加本实例 `stop-actions` 检查，正常停止时先阻止后续输入；含此项在内15项离线隔离测试通过。本次没有新游戏、XR或录制进程，不存在待收尾的本方新运行。

## 停止／切枪与待完成奖励的竞态修复

后续核对生产 tick 和 cleanup 发现：`completing` 分支先于 stop/switch 检查，允许在已有停止／切枪请求后继续启动完美奖励；而 cleanup 遇到未调用且尚未 applied 的完美请求时直接返回 false，即使库存早已插入提交、原版已安全退出，也无法结束清理。

已把停止／切枪检查移到 completing 之前，并让新的生产 `weapon_hammerburst_reload_cleanup.inl` 统一核对已有事务回执：

- 尚未调用的完美奖励被明确拒绝，不启动奖励调用；请求的 before 已包含本轮已提交的插入量，因此拒绝奖励不扣回已装弹量。
- 奖励已调用时，只通过原有 `perfect_reward` 的 dispatched 分支完成既有 hand-back／bonus 核对，不重复分数或音效调用；不明确的原调用结果继续保留待核对，不猜成功。
- 插入已写／未写、普通完成、原匣检查、库存不匹配和对象身份变化保持原事务的核对要求。已处理回执再次到达时不重复记账。

10个用例、130项检查通过，使用真实 `ReloadTransaction` 和实际生产 cleanup helper。再次在本次冻结源码／头文件上编译执行相同测试通过；完整 Playground 编译链接成功。第一次完整构建因 helper 参数遮蔽父命名空间变量而被 `/WX` 拒绝，已只改参数名并正常重建；初次失败日志保留，未降低编译要求。新DLL SHA256 `ceff6e61a4175fc28f07e4564a6af4a2e3200b19b7aa0c363cd038a23a390306`，记录 `analysis/hammerburst-20261108/reload-cleanup-r1/prepared.json`，集成补丁 `docs/hammerburst-reload-cleanup.patch`。本枪未来单次测试入口已改为选取此候选，当前暂停标记仍阻止运行。

此修复只解决停止优先级与现有回执收尾。**切枪保存未闭合机构、重新取回恢复机械状态仍未实现完整链路**：现HB switch cleanup最终reset，会丢弃该实例机构进度；不能把本次清理成功写成该要求已完成。真实打断、完美奖励、首发失效及完整拟合仍待游戏验证。本轮没有启动game/XR。

## 切枪恢复：库存事务保存准备

新增 `weapon_hammerburst_reload_park_ledger.inl`，复用真实 `ReloadTransaction` 的既有 retained-original／suspended rebind 能力，没有放宽通用事务的原有检查。该模块目前尚未接到生产切枪调用点，不改变当前候选运行行为。

准备操作只产生一个副本方案，失败不修改原事务，也不提前丢弃当前手持预留。打开锁扣或尚未完整拔出的原匣保留原编号，并要求实际适配层先把其显示恢复到装填口；已完整拔出的原匣收回后恢复点为等取弹；手持新匣释放未提交预留量；已插入时保存已提交数量、闭合欠账和原事务时间。重新绑定严格调用现有同实例跨hold认证，再检查库存和策略未变；此步骤不产生抓取、库存请求或机械完成。

22个用例、191项检查通过，覆盖各准备阶段、未完整拔出／已拔出、未插新匣预留、已插后闭合、重复旧意图／错误serial、原完美期限前后以及武器／世界／session／input／库存变化拒绝。测试直接执行该模块和真实事务。计时测试证明的是库存事务不重置原起点，不代替后续原游戏Lease计时核对。

仍须完成实际动画所有权正常交还、原匣显示恢复、重新取枪后真实原骨角度恢复、原计时Lease与库存observer协调重绑、真实release/new-grab及停止/读档清理。不能仅凭此模块或用例宣称切枪恢复可用，当前完整Playground候选仍为上节的 `reload-cleanup-r1`。自动运行暂停保持有效，本轮没有启动game/XR。

## 切枪恢复：暂停一侧接入开发快照

已把库存保存方案接到实际HB runtime和切枪前准备入口。新增 parking／parked 状态，先持久保存方案与原计时Lease副本，再正常退出本枪原换弹、交还本枪动画／弹匣显示，核对库存后才发布parked并允许收枪。部分恢复失败时保留parking方案，不能重发插入、奖励或新的原开始。外来动画／弹匣owner或同serial但不同身份的原Lease均拒绝处理；对象明确退役则走原有正常退役清理。

parked没有活跃原调用／显示资源，不继续占用共享服务；其他武器仍可被当前消费者选择。其停止或退役只清空本枪私有记录，不误处理随后属于另一把枪的服务，也不清除另一所有者的outstanding标记。已装弹量、原计时副本和锁扣角度保留，但旧手Lease及行程被撤销。

实际parking控制模块及真实库存事务的11个用例、65项检查通过，并已在本次冻结源码上编译执行通过。库存方案22个用例、191项检查再次通过。默认关闭路径另有13项检查，确认不会进入未完成的parking流程或修改现有事务。

**恢复一侧尚未接入，因此共享源码的 `GEARS_PLAYGROUND_HB_PARK_RESTORE` 默认关闭。** 只有 `reload-parking-r2` 私有编译快照显式打开用于后续继续开发；其完整Playground编译链接成功，DLL SHA256 `2d687b230bec41783b4832c8ce66848126c497239dc4dd082ca188cd7c31a08b`，明确 `runtime_ready=false`、`restoration_integrated=false`、未被运行入口选用。首次r1完整构建暴露前向声明、旧冻结接口兼容及参数遮蔽问题，已按确切接口修正，没有放宽编译要求；失败日志和r1成功快照均保留。新集成补丁为 `docs/hammerburst-reload-parking.patch`，它不修改公共CMake默认开关。

后续须恢复重新取枪后的原骨／弹匣显示，协调observer与原计时，再要求真实release/new-grab。完成这条链路前不交付这个开发快照试玩。可供后续实际普通空仓检查选择的候选仍为此前 `reload-cleanup-r1`，当前自动运行暂停继续有效。没有启动game/XR或替换运行实例。

## 恢复一侧：严格限定的原件恢复校验

新增 `weapon_hammerburst_restore_admission.inl`，为后续重新取枪后的原件显示恢复提供一个短作用域校验。它要求已经重绑的有效park方案、恢复阶段、同一库存owner／FrameKey、原有mount几何、主手可用姿态，以及Frame／sync各自不超过100ms；库存必须与park方案一致，且仍有打开、打断或待闭合的机械事务。空闲已完成的事务、未提交手持弹匣、未决请求、另一帧／武器／场景或失效源均不得通过。失效后同一作用域永久撤销，不能恢复几个字段后重新变为有效。

原有HB动画取得、clip开始、SetPosition和弹匣显示开始的校验已接入此受限路径。它不更改原手部Lease、输入、插入、射击或奖励校验。经原seek_impl提交的恢复位置还必须匹配本实例原曲线对已保存角度的时间映射，禁止借恢复入口推进到任意新角度；clip自身原有初始化仍是原来的无通知初始化，并不提交机械完成。

实际作用域模块使用真实Frame时效、真实库存事务和原曲线换算测试，25个用例、266项检查通过，含100ms边界及再多1ns撤销、Source0/1、嵌套拒绝、作用域退出、不可恢复的撤销、错误帧／owner／阶段、主手姿态失效、未提交弹匣以及非保存时间拒绝。Native当前源／实际资产资格由fixture替身提供，这些用例不等于实际游戏源或原函数运行验证。

`restore-admission-r3`完整Playground编译链接成功，同一冻结源码／头文件上的全部上述测试通过，DLL SHA256 `5dcf98b430188351a68b911b49ab84b204f64e20996aff2618b194d09c00438e`。相对parking-r2仅7个源文件变化，集成补丁 `docs/hammerburst-restore-admission.patch` 保留共享文件其他差异；默认开发开关仍关闭。

当前实际恢复调度尚未构造这些作用域，因此不能宣称已经恢复原件显示或可以试玩；`restoration_dispatch_integrated=false`、`runtime_ready=false`，运行入口仍未选用本快照。接下来须协调observer／同实例hold重绑，分别处理装弹前的原状态恢复与已插入后的仅机械闭合，保留原GAME计时而非对满仓再次ForceReload，并在显示恢复后要求真实release/new-grab。当前自动运行暂停保持有效，本轮没有game/XR启动。

## 已建立握持的重新发布修复

实际38240已在frame106从肩部取得本枪；frame148的hold_failed清除了已建立的逻辑握持，早于frame195首个扳机。该扳机Source1在精确Input.advance时active，源年龄82.7828ms，最终blocked且零权威射线。回调为何失败尚未证明；不把它归因于单一计时或库存原因。实例正常停止，launcher0，game消失，无强杀，1304前台与2067CLOAK检查均无违规。

新增republish_hold仅用于已握持且在新Frame重新核对相同对象和挂接的分支。拒绝或Frame失效时suspend撤销实时发布、清输入事件，保留私有既有握持意图；下个合格Frame才允许重建发布。首次获取仍使用原commit_hold，失败清模型；对象、世界、挂接变化及时钟异常仍撤销。没有保存或刷新旧Frame、姿态、按键或时限。

行为红例在修复前exit1（既有握持被清除）；修复后专用8场景102检查通过，冻结原装备128场景1639检查通过，完整Playground编译链接成功。候选held-republish-r1 SHA256 094a283297083020c62d69aadd86267d993fc98bed6050b8d8cf1e6bca9a48df；仍基于reload-cleanup-r1，未并入未完成的parking/restoration。源码补丁hammerburst-held-republish.patch，完整武器要求仍未通过。

旧音频muted=0只表示没有执行mute写入，不能推出未静音；shared_rejected也不是目标专属。后续使用中央增强helper的GetMute明确读回证据，移除本枪新实例的零音量INI预处理；历史实例原样保留。

## held-republish-r1 实测启动未达动作

新实例hb-cloak-1110-203109使用094a2832候选。worker在读取正在更新的出生JSON时PermissionError退出；launcher另报音频helper三秒身份确认超时，音频记录最终含guard_ready后stop、scans0，两者因果尚未证明。未发送武器动作，不记枪械修复通过或失败。原session确认prime cancelled及entry_restore_verified。原停止入口无活动索引，复用isolated_sim.owned_windows对本方出生PID46324/creation134387874800271196正常WM_CLOSE；已核实游戏消失，无强杀，手动补救收据单独保存，不伪造isolated-done成功。中央维护会话已收到具体错误路径，未重复启动。

## 重新取枪后的进度绑定已接入（未完成恢复交互）

新增restore_control::rebind，并在实际parked分支取得新的GT Frame后调用。严格核对同实例、原观察器、原计划身份、原reload serial与身份、相同component/asset、主手新鲜度和实际挂接；库存与台账均先复制，最后重新核对再发布。discover失败或过期会恢复原几何；同帧更换component的竞争也拒绝。保留opening原始弹数作为奖励依据，只更换身份；不会重新计入已插入弹药，不复用旧手lease，不启动原补弹或动画。

该步骤转入restoring，明确拦住原生UI/旧手动作落入；显示恢复与新鲜意图续接仍未接完，不能选作实际候选。共享开关默认OFF，独立开发snapshot显式ON用于编译，当前运行入口仍选择held-republish-r1。

冻结RestoreRebind 22场景213检查、RestoreAdmission 25场景266检查、ParkControl 11场景65检查通过。前者为生产控制+实际台账，库存/挂接等引擎读数为夹具；后者另覆盖真实Frame时效/曲线。完整Playground编译链接成功，开发DLL SHA256 98751248c8329d724b5eb35767715ef62e99a99858f0f3e9913dc61e8b4ac334。首次编译因局部observed遮蔽既有全局被/WX拒绝，改名后通过，失败日志保留。补丁hammerburst-restore-rebind.patch。

## held-republish-r1 实际53012：握持回归出现，首发仍失败

hb-cloak-1110-204446已进入真实场景，session finished/passed=true，原launcher0、operator1、stop0，Native/入口恢复及XR退出走正常流程，owned game消失、无子进程残留/无强杀。1455次前台与2490次CLOAK检查零违规；增强音频helper明确读回initially_muted_sessions1、verified_muted_observations252、verify_failed0、target_shared_rejected0。不是完整武器通过。

frame109肩部取枪成功；frame191首个trigger_pressed=1且trigger_released=1、value0，Input状态baseline1、Source ready1、精确admission active1，源年龄98.5964ms、preparation_budget0，零权威射线。previous_sync到当前sync相隔2225.4604ms，长消费间隔原因尚未证明，不扩大100ms/20ms/250ms约束。随后frame193 hold_failed，frame202 held1重新发布，本区间未发送新的Grip获取；证明本轮在失败后恢复过持枪，不能推广为全部生命周期通过。

488张私有GPU组合帧、录制errors空，离线编码exit0；已亲看frame00478，场景和低位枪口可见，手部完整贴合及遮挡仍未接受。runtime-53012-first-failure-analysis.json与runtime-53012-text保存文本原件。下一步核对真实UP基线、DOWN消费及录制时序，不能用固定等待脚本的一次失败声称武器本体原因已定。

## 恢复显示接入（开发版本，尚缺新意图续接）

restore_control::display现在由实际restoring分支调用。以每帧短作用域限定原始gun-only clip启动/停止、原曲线保存角度seek、缺失弹匣的原显示隐藏；不同步执行多个阶段，不调用ForceReload、不改库存、不授予旧手授权。每步重新读实际库存并核对原台账；未知或过期只等待，部分派发失败保留原拥有者收据，转回parking清理，不自动重放启动。恢复中再收枪可回parked，保留原serial/游戏计时。

新增Display18场景185检查、Rebind22/213、Admission25/266、ParkControl14/86均通过。Display为实际派发控制/真实台账，原动画/显示调用是夹具；Admission另验证真实Frame/曲线。完整Playground编译链接成功：restore-display-r1 SHAcebec05c8a705336ae606f707c423a5a2416ce52ec41848f455176063f0d8248。第一次构建因准备脚本在嵌套switch处截断分支失败，修正严格外层锚点后编译通过，失败日志保留。

新鲜松开/再次按下的续接、原生重新接管与闭锁奖励计时仍待接完，开关默认OFF，未选作游戏候选。当前实际入口仍为held-republish-r1。补丁hammerburst-restore-display.patch，未缩减原十三项需求。

## 恢复后新意图续接与原游戏时间（r3源码/离线，未实测）

实际restoring分支在显示恢复后调用restore_control::resume。先要求新鲜且已知UP，跟踪/Frame缺失会撤销该基线，再要求当前新grant、正确部件、严格递增原始press token；旧DOWN/retained grant不得启动。台账先复制resume，再由同一原serial保存的Lease重新取得原生补弹，资格receipt不重新采集、原accepted_begin/reward_start/perfect_end不重置。原启动是否实际派发有可选布尔回执；仅在新意图上调用一次，未派发拒绝也消耗本意图并恢复显示，不自动重试旧按键。

未插入时恢复原native owner后在不同Frame取得/停止动画、seek保存角度、隐藏已拔原匣，再回到原opening/opened/waiting_ammo；当前仍有效的同一新手grant可衔接实际握持，不凭空另授lease。已插入时直接继续原closing，不再ForceReload或插弹。此时animation_owned仅接受同一武器、同一visual-only动画与新鲜Frame，原生UI分支不读取其他owner；物理闭锁按保存reward_start读取真实WorldInfo.TimeSeconds，保持原accepted坐标和奖励期限，不用墙钟补偿暂停。完成时先归还本方visual clip。

恢复中再收枪覆盖原生已重新取得与已插入visual-only两条路径，保留弹数/serial/计时。原调用若可能已派发但没有可认证Lease，标记restore_native_uncertain，既不重新开始也不假装parking/cleanup成功；只有实际对象退役可丢弃该未决原件。

冻结Resume19场景284检查、Display18/185、Rebind22/213、Admission25/266、ParkControl17/109通过，共101场景1057检查。Resume/park使用实际控制与台账，原生/动画/授权调用是夹具，不是游戏证据；Admission另覆盖真实Frame/曲线。restore-resume-r3完整Playground编译链接成功，SHA 1a320f2c360723baf7f55b75e7b381b88d49a2100550c896714caa4aa0ed0897。初版编译发现冻结API没有较新begin_for_part，未引入其他武器代码，改为原suspended begin并补可选派发回执；此前失败及r1/r2证据保留。

当前本段源码控制链已接入，但真实native重新取得、UI、切枪恢复、闭锁奖励及整体首发仍待实测。开关默认OFF、未换当前游戏入口；runtime_ready保持false，不把编译当玩法验收。原十三项要求保留。源码差量hammerburst-restore-resume.patch；共享较新API的小包装差量另存hammerburst-restore-resume-state-api.patch，未将其他人的共享源改动纳入提交。

## 首发按实际消费确认（新测试候选，尚未实测）

53012首发记录DOWN/UP都已发生而当前UP，不能据固定0.35秒脚本断言实际DOWN被游戏消费。新增本方confirmed_first_fire：先发送UP并等待原PG实际消费到fresh/current/Source1 ready/game_known/unowned/原R2 UP的基线；只发送一次真实DOWN，最多2秒等待PG消费DOWN，再无条件发UP。仅等待输入回执，不放宽游戏100ms/20ms/原press时限；超时不补按。之后原有限连发应恰好6条有效权威射线，零/不足/多发均失败。

新私有观察点在Input.advance后记录原input_admission_ns与原source sync、active、状态、trigger counters/value以及准备预算；不新读时钟、不查游戏、不写输入/姿态/库存。只本方HB绑定、250ms或真实counter变化时输出，最多192条。共享旧脚本不改，仅本方模块装配替换首发段。4项时序测试与15项独立身份/装配检查通过，覆盖无UP不按、DOWN超时只释放一次、错发数不重试。

consumed-input-r2完整Playground编译成功，SHA a8d937ccebef5416bbeea08287e6deb78fb0ad2577c828754fe61e0d1443b323，基于已编译restore-resume-r3；Native/XR未重编、公共v71未变。r1缺少game_known观察字段，未运行，r2补齐后供本方受控验证。恢复控制链显式私有开关ON，仍无游戏恢复/整枪接受结论；接下来同一owned实例先core再普通empty reload，遇首个失败即正常收尾。

## consumed-input-r2 实际57248：启动观察超时，未达武器动作

新实例hb-cloak-1110-211559原相机就绪since_runner_start49.311648秒；本方operator在XR预检后就开始45秒hand-connection等待，原result记录该等待TimeoutError。随后native-start明确报Owner cancelled camera preparation before activation，不是已证明的Native本体失败。录制结束又报No complete fresh projection frame，覆写终端异常，但原result保留首因。

实际native_started=false、native_cleanup_verified=false/cleanup deadline error、XR exit1、launcher1/operator1，全部保留失败；Playground shutdown0/entry_restored=true、prime取消且入口恢复，本方game消失、无子进程残留/无强杀。1081前台与1857CLOAK检查零违规，302张GPU帧但录制失败，不作场景/武器证据，不称完整运行正常。

本方适配器将观察等待对齐既有180秒启动预算（所有真实准备/身份/时效保护不改），并在已有原异常时只存储录制次因，不覆盖首因。此为具体失败后的单次复查准备，不改变100/20/250ms游戏输入边界。

## consumed-input-r2 实际40524：UP/DOWN已分开消费，脚本在首发前松开

本次startup观察修复后进入真实场景，frame734肩部取枪。frame830实际UP baseline，frame831 DOWN为status3/owned1、Source ready/current、active、muzzle_ready1/preparation_budget1，精确源年龄55.6672ms。脚本仅等待逻辑按下，9.1905ms后即发UP（整个命令DOWN约88.0477ms）；原首发event1在101736627257700被捕获，尚未交付。frame832 UP触发release gate3（completion不是committed/count0/first_debt1），按原规则取消待执行首发，无权威射线。该结果保留为脚本释放过早，不能说生产首发已证实损坏；未改释放/反重放保护。

脚本改为消费DOWN后继续同一次真实持按，等首条真实trajectory后才UP；等不到首发则有界超时释放，无补按。仍要求总计恰好6条有效射线，全部游戏时限原样。4项时序测试通过。

本轮continuous/session passed=true、launcher0、operator1、stop0，Native清理/入口恢复/XR退出0，本方game消失、无子进程残留/无强杀；1475前台/2555CLOAK检查零违规。362帧GPU录制errors空，媒体复核与编码另记，不称整枪通过。raw text归档runtime-40524-text，首因记录runtime-40524-first-failure-analysis.json。

40524媒体后处理：离线编码exit0、stereo-demo.mp4存在；亲看frame00352/00361，两张实际首发附近原图均有近脸/右眼大片枪身遮挡，不接受为完整贴合或全角度证据，仅用于本次失败核对。

修正为等待首条真实弹道后的新申请hb-cloak-1110-212609在start前被现有两实例内存保护拒绝，game_started=false、operator_started=false，无本方运行/恢复责任；不复用该已prepare名字，不通知占用/排队，不标目标blocked。下一轮资源允许时使用新唯一实例，保留当前已编译候选及新单次按下时序继续验证。

## 资源拒绝期间准备切枪恢复的结果核对

hb-cloak-1110-212845再次在start前被现有2实例资源保护拒绝，未生game/operator，不复用该实例名；继续离线工作，无排队/占用通知。

恢复路径新增有界hammerburst_restore记录：parked/rebound/display_ready/up_baseline/intent/native_reacquired/resumed，只在已发生的阶段转移后记录原台账、真实source字段、原serial/accepted game clock、hold和原生归属；上限64，不增clock读、不执行动作、默认OFF。这些private ledger数不能代替真实库存读回。

本方verify_restore_observations把阶段序列与既有typed double-read快照的game/weapon/capacity/loaded/reserve/infinite/bonus/game_time对照；检查新hold、已知tracked UP再新press、原钟不变、正确原生/visual-only分支和回到真实机械阶段。2个测试方法覆盖两条正例及15种错误/重放/改弹数/改奖励/倒退时间拒绝；这是验证器夹具证据，尚无实际切枪动作驱动接线或新玩法证据，不称切枪恢复通过。

冻结Resume19/284、Park17/109仍通过，完整Playground编译链接成功。restore-receipts-r1 SHA 94b0b1f21380402ee57352b8fda2754a6d9c77c26c3a24715a6ed1597c02c0fb，只加阶段观察，Native/XR未重编，公共v71未变。新原图/时序检查均未涨工程分；本次统筹一次性采集沿用HB A.5/B.25/C.25/D.25/E.25=30%，头显false。下一步仍优先修正单次持按首发→六连发→普通空仓完整返射，再接入实际中断动作验证器。

## restore-receipts-r1 实际36568：首发与11轮排弹通过，末轮3/6拒绝

实际hb-cloak-1110-214502肩取成功；新脚本等首条原版trajectory后才UP，core得到恰好6条有效射线，typed实际72/78。原first event1与后续原timer均实际交付，首轮3..6在实际UP/input_owned0下完成，不能推广为失去绑定后的全部timer分支通过。之后11次真实按下/松开均恰好6发，typed从72/78逐轮降到6/78。最后第12次排弹只完成3发，typed3/78；共75条trace_valid/write_source_consistent/trace_source_consistent全部1，与实际used75一致，无库存setter。

首个拒绝为native_hammerburst_release_rejected gate1/frame0/source_stage0/count3，随后after_tick null Frame取消；最后成功源Frame404。原通用R2日志64条上限已用完，当前辅助观察也只记录have_source，无法证明null源是过期、observe、publication还是其他状态。保留失败，不猜C0/超时/身份原因，不重跑原样timer。普通空仓0/78与换弹尚未到达。

continuous/session passed=true、launcher0/operator1/stop0，Native清理/PG入口恢复、XR exit0，本方game及私有子进程全无、无强杀。1486前台与2666CLOAK检查零违规；目标音频265次明确mute读回、verify_failed0/target_shared_rejected0。528帧GPU录制无错误、离线编码exit0；3张全时段overview及6张首发/尾失败原图亲看。真实近脸遮挡、低位/近位跳变、白色几何及重叠表面仍在，完整皮肤贴合/全角度未接受。runtime-36568-first-failure-analysis.json与text归档保留。

新增source-loss-r1只将现有copy_frame_on_gt可选失败观察与withdrawal_on_gt快照接到同一次读上；拒绝处最多12条，保留copy与withdrawal各自Frame/原因/difference/原commit年龄，不把旧withdrawal冒认为本次原因。不增时钟、不重读Frame、不改取消/输入规则。完整Playground编译成功，SHA 39bda734f56dce4141a0fd718bb5026b571b75196e64d586d0a05ee12f551b34；Native/XR未重编、公共v71不改。下次受控窗口只定位这个新首败，再按实因修复；整枪目标与13项需求保持，评分不因诊断涨分。
