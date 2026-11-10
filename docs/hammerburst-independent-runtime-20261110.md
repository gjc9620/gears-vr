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

## source-loss-r1 实际47948：再次core6，旧排弹第二次DOWN只作baseline

新实例hb-cloak-1110-215931再次actual_fire=true，首轮6发；第一轮排弹也6发，typed66/78。下一次旧固定0.35秒输入frame266 pressed3/released2/value1，current/source ready/active/preparation_budget1，但Input结果baseline1/owned0，frame267 blocked8，没有新的原first event，typed仍66/78，原脚本按零耗弹停止。没有复现36568末轮partial3，pending-only source_loss为空，不能说观察器已找出原null原因或修复了它。

适配器现将core已验证的UP消费→一次DOWN→原首条真实trajectory→UP用于每次排弹和返射；所有步骤仍受原18秒排弹/8秒返射总期限约束，typed读取超原总期限也失败。5项真实press-helper检查、4项实际适配drain控制测试、15项身份/适配器检查通过；覆盖多轮同一绝对截止、不重试失败press、晚到typed不能通过及原trace一致性保护。引擎和输入为夹具，不是游戏通过。

source-loss-r2只扩大观察覆盖：idle故障独立最多6条，已承诺burst故障仍12条；保留各自copy/withdrawal frame，不重读、不授帧、不改时钟与释放策略。消费观察768条有界覆盖完整reload时段，避免原192上限先耗尽。完整PG编译成功，SHA 82335ee87e8351e2ecbd43d498df9ede888298eeea828741d603dfde54d760f8，Native/XR及公共v71不变。

47948 continuous/session passed=true、launcher0/operator1/stop0、Native清理/入口恢复/XR exit0，本方game/私有子进程消失，无强杀；1484前台/2601CLOAK检查零违规。媒体复核另存原recording。source-loss-r2和新统一输入时序尚待实际验证，全部原需求保留。

47948媒体：410帧、recording errors空，离线编码exit0；亲看frame00375/00394实际原图，近脸大遮挡仍在，未通过完整贴合。新输入帮助器补充只观察同一原burst的晚到尾发，保持调用者原绝对截止、不再按键。最终6项press-helper、4项实际drain适配、15项独立身份/装配检查通过。排弹/返射新时序和source-loss-r2尚未实测；首发在两个实际构成中各通过6发，不等于完整排空/换弹稳定。

## service-loss-r1准备与磁盘恢复

11644 actual core6并typed72/78；下一确认手势只4/6，总10条有效一致射线。帮助器在typed再次读取前停止，最终typed缺失；不得补写推算68/78。闲置copy观察未与该4发取消对齐，取消先于R2读；新service gate观察按原序短路记录首因，最多8条，不增clock/Frame读、不改寿命/释放。完整PG SHA 8c798dcc10b9e296313b1e85387fbb8bdf2536202e818a9b6da81ce83a689526，冻结事件适配器280检查通过（队列/引擎/证书为夹具）；未实测该观察点。

11644 continuous/session passed=true，launcher0/operator1/stop0、Native清理/PG入口恢复/XR exit0，game/子进程消失、无强杀、2131前台/3461CLOAK零违规。新实例曾因C盘free0在清单创建阶段失败，未生game。原归档复制同样WinError112中断；随后只清理本枪11个终态build目录77个可重建obj/69,574,112bytes，源码/DLL/PDB/原证据均保留。复查free1,883,758,592bytes，未把全部空闲变化归因于本方。已逐文件SHA核验补齐11644文本归档，并在本方prepare前增加1GiB空闲下限；外部命令失败后停止后续步骤，不再继续未完成的checkpoint/run。先前37907ce6只保存观察头与patch，不代表完整归档已提交；本次补齐。

## 11644录像实查与动作录制边界修正

已亲看11644全时间跨度48个双眼样本、三页概览；样本全部是启动等待卡。306张原JPEG逐文件SHA核对只有一个不同哈希，证明本次保存的全部帧字节相同。recording.json明确保留一次“No complete fresh projection frame”异常，终帧42.881s；不将其补写成errors空，也不把continuous/session通过当作动作画面。未编码这份失败录像，没有手枪接触或贴合证据。审阅索引及原图哈希在`analysis/hammerburst-20261108/runtime-11644-media-review/prepared.json`；原图、原异常和原游戏日志保留。

本枪私有动作适配器现保留启动交接事件，但将GPU录制开始移到Native手/装备交接就绪后、首个动作之前；首次新完整帧仍限原5秒，失败或失去本方身份直接拒绝进入动作。共享录制器、Native、XR和武器机制均未修改。5项针对性检查与15项既有实例身份/接线检查通过；这只是脚本离线结果，不是实际画面已修复。

223959在启动资源检查时因已有两个自动实例拒绝，未启动game/operator。224419在start时磁盘free3,924,176,896bytes低于现共享4GiB下限拒绝，同样未启动game/operator；本方prepare前也已统一4GiB，先前1GiB只是历史步骤。保留两次拒绝，不计游戏尝试或通过。随后free14,749,691,904bytes，按原入口启动新唯一实例224756；实际结果待原进程终态后记录，不重启旧名、不碰其他实例。

224756进程已终态exit1，start明确因两个其他自动worker的实际身份仍存活而拒绝，未产生launcher-session/game/operator；不是service-loss候选的游戏失败。本枪runner现prepare前调用同一共享require_disk/require_memory作只读预检，避免已知资源不足时先复制约1GiB实例；start仍在原共享短事务中复查并登记，预检不占名额/锁/内存，不代替最终准入。`--resources`只读模式同样在mkdir之前返回；当前读到两个实际存活worker，继续离线工作，不将资源等待标blocked，不停止其他会话。

动作录制边界另补充中途失效处理：每次既有live检查在原实例身份通过后检查本方recorder.errors，若捕获已经失败，后续动作立即拒绝；不会因曾拿到第一帧就继续整套无画面动作。新增两项回归验证“首帧后捕获失败”和“原身份失败仍拒绝”，现7项动作录制检查及15项既有身份/适配检查通过。仍须实际场景录像和机械流程验证，不能据此宣称连发或贴合修复。

## 实际3112：取消首因与已松键事务等待候选

hb-cloak-1110-225401，game3112/creation134387960636960786，service-loss-r1实际验证已终态。首轮6发typed72/78；两次排弹各6发，typed66/78、60/78；下一轮4/6被明确取消，总22条有效且write/trace来源一致射线。最后独立typed60/78在第三轮之前，第三轮后缺失，不能补写推算56/78。

唯一service_loss实际记录：gate3（client.armed=false），count4、first_debt0、firing_debt1、cancel_before0、client_phase2、registration_alive1、metadata_epoch11902；同期最近撤回记录stale_before5、frame303/sequence3820、age102,130,700ns，leftUP、flags13、lease0。其前实际第22条射线frame302；同次R2后续copy失败与撤回记录对应。证明本次首个service取消条件是姿态publication撤回；没有证明每次历史部分连发均同因，也不能将102ms过期姿态用来射击。详见`runtime-3112-first-failure-analysis.json`，7份文本原件归档逐SHA核对。

continuous/session passed=true，launcher0/operator1/stop0、Native正常清理、XR0、game/子进程消失、无强杀；1394前台/2395CLOAK零违规。动作录制174帧errors空、离线encode exit0；亲看全跨度48双眼样本三页概览及原图00140/00158/00173，实际场景、取枪及射击可见。近脸大遮挡与叠影仍明显，完整贴合/连续性不接受；不把录制修复提升成整枪验收。

released-debt-r1为本次首因的私有修正，默认未启用公共功能：仅在现有release_completed全部16项检查通过的真实UP处捕获既有ExistingActionLifetime。只有原六连发已commit、count1..5、firing_debt、first_debt无、cancel无、实际输入无owned press、同持枪、明确stale_before且同原action epoch/父关联仍有效，才保留债务等待。service失去publication与after_tick无Frame时不立刻取消这一已松键事务；不会返回Frame/枪口/输入/新事件许可。任意射线仍经过既有新Frame、右手跟踪、native current、20ms余量和100ms source检查；原pending event250ms期限、serial及captured不续期。显式撤销、失去注册、非stale撤回、未松键、first debt或换持枪不进入该等待。

候选完整PG编译成功，SHA df22727d9bc35578f36e39bcc111679054eacf59a2b6f346de08f27aea5547f6；Native/XR未重建。实际事件适配器308项检查通过，其中新增28项覆盖过期时不发射/不续event、新鲜源恢复后仅下一发、8种拒绝、原250ms到期正常取消；队列、引擎、证书和Client lifetime调用为夹具，不冒充实际Native epoch验证。源补丁`docs/hammerburst-released-debt-wait.patch`；实际该修正尚未运行。

另用同冻结树的实际HandClient头执行12项原action lifetime身份/epoch案例，并补2项102.1307ms过期publication案例（single/support分别覆盖）：Frame copy拒绝、armed false、snapshot无权限，原action身份可仅供既有债务保留，显式invalidate后立即拒绝。14项通过；这是实际Client代码加本地主机/观察器，不是Native运行或游戏身份验证。结果`released-debt-r1/client-test-result.json`，编译未改PG候选DLL。

## 可选实际换弹park/redraw/resume动作脚本接线

资源检查持续拒绝新游戏，当前C盘free3,347,656,704bytes（观察值随其他实例变化）低于4GiB。没有重启旧实例或创建本枪新游戏；继续补齐既有恢复需求的独立动作脚本，不将等待标blocked。

`hammerburst_restore_chain.py`通过私有适配器的显式`--hammerburst-park-restore`开关，在原弹匣已打开、尚未抽出时接入。释放左手后独立typed before；按现有右肩slot3路径各一次真实Grip完成stow/redraw，要求新equipment回执及原HB对象；等待本次parked→rebound/display_ready→真实UP；同一实例重新读取枪体组件和矩阵，与原规划比较通过后才发出一次新左手magazine Grip。要求原stage3 opened恢复，再用现有独立typed after和七段实际restore记录验证库存、serial、accepted_begin/perfect_end/reward_start、不确定dispatch、原angle、新hold/UP/press身份。调用方还必须等新的to4原弹匣阶段后才继续原抽出/插入/关锁扣/返射。任何缺失、不同实例、不同mesh/矩阵或库存变化均停止，不输入重试。

5项脚本回归通过（成功新记录、收枪缺失不再按键、两类mount错误拦截左手、独立库存改变拒绝、可选实际适配位置编译）；输入/引擎/回执为模拟，不是实际park/resume完成。默认当前连发复测不传该开关，未更改PG候选DLL或生产恢复策略。完整恢复动作、原UI、奖励/中断及画面仍待实际游戏验证。

## released-debt-r1首次启动49732：动作前失败

资源释放后free8,362,479,616bytes、仅一个其他自动实例，按现入口准备新hb-cloak-1110-232025；worker准入free7,461,855,232bytes。game49732/creation134387976355816944确实启动，operator未启动、Native未激活。原trace明确在before_native_activation→start_private_gamepad→background_input.change→FrozenThreads.__enter__→OpenThread处WinError87。不能把它算作连发修正通过/失败；候选仍未执行动作。线程枚举与OpenThread之间线程终止只是可能原因，当前没有失败TID/复核快照，不宣称已证实。

launcher1/XR1/stop1实际失败保留，PG reservation-only清理shutdown0、入口恢复通过，prime取消/入口恢复；本方game/子进程已正常退出、无强杀，worker_error null；732前台/734CLOAK零违规。Native未启动，不能声称Native运行清理通过。音频目标初始muted1/verified54/verify_failed0/target_shared_rejected0，helper清理exit0。无动作录像。6份文本原件逐SHA归档，详见`runtime-49732-startup-failure.json`。

已按主人“模拟器测试自己协调”的授权，将准确故障点与原证据发给既有统筹/共享工具维护会话，未新建聊天、未启用helper、未修改其共享文件或启动别枪测试。接下来先获得此处有证据的窄修复/处理，再用新唯一实例验证本枪候选；不绕过线程冻结/恢复、输入隔离或代码hash保护。

共享维护方已在同一OpenThread失败点补被动证据，source SHA b27ba12d1f3c703babd5ec09ee731edc95742595da7d9887ad513568feab2bf2，本方已阅读实际hunk及其报告。保留原WinError，并附TID、原枚举、已挂起TID和一次新枚举；任何OpenThread失败仍中止，原逆序恢复保留，不跳过线程、不重试patch。报告9项离线检查通过，维护方未启动游戏；这是观测补齐，不是该启动故障已修复或证实。当前free3,568,406,528bytes仍低于4GiB，因此没有借此额外启动取证游戏。下一实际候选验证使用当前入口新名字并保留这次失败；连发修正仍未执行到动作。

## 本枪从未启动的准备副本归档

为保留证据并减少C盘空间反复不足，仅处理本枪三份已知start准入拒绝、从未创建worker/game/operator的准备副本223959、224419、224756。逐一检查result.game_started/operator_started/worker_started均false、无runtime回执、instance root/name一致、无原目录或嵌套reparse。将各自123文件/893,930,002bytes复制至`J:/gearsVr/analysis/hammerburst-archive-20261110/never-started/<原名>`，369文件逐一SHA/大小核对，保留完整原件与配置；原C路径建立junction并再次逐文件读回核验，才删除已核验的本方同卷handoff。原路径可读，旧名仍禁止重启。未迁出源码、当前候选、实际运行证据或其他会话目录。

三份合计2,681,790,006bytes；记录中的before_free9,424,424,960/after_free13,041,836,032bytes也包含同时发生的外部空间变化，不能全部归因于本方。索引为`rejected-prepare-archive-20261110.json`及三份`hb-cloak-*-archive.json`；操作脚本限定上述三个确切名字，遇既存归档/映射即拒绝再次处理。实际归档不启动游戏、不丢弃文件。当前资源准入仍因两个实际存活自动实例拒绝，本方prepare前停止，无新游戏。

## 实际45736：完整有限排空通过，新弹匣就位失败

hb-cloak-1110-233210，game45736/creation134387983499765929，released-debt-r1实际动作已结束。core6后typed72/78；随后12次各六发，typed72→0、reserve78不变；总78条有效/write+trace来源一致射线，空仓再扣扳机保持L0/R78无射线/无自动补弹。没有stale_debt_wait或service_loss记录，故证明这一次普通排空通过，不能声称过期等待分支已实际触发或因果修复。

普通换弹原begin serial1、锁扣打开、旧弹匣完整抽出/释放、新弹匣shoulder token2/78rounds拿取到stage6；后续8秒未见实际就位to7，operator1保留，没有关锁扣、返射或最终L78/R0证据。Frame710第一次supply后相同Frame读回reason7/game_difference5（request_ammo与incumbent变化，candidate_flags77）；Frame712又发生101.0417ms过期，随后main/mount撤回并恢复。两者分别保留，不把所有后续现象归因于首个difference5，库存不得按计划填入。

launcher0/stop0、Native清理/PGshutdown0入口恢复/XR0、game及子进程正常退出，无强杀；2479前台/4332CLOAK零违规。677实际帧errors空、离线encode exit0；亲看48双眼样本全跨度三页概览和原图00331/00489/00518/00532，实际取枪、射击、锁扣/旧新弹匣与黄色引导可见。近脸遮挡、叠影/白色几何及00518碎块画面仍在，逐件手型/全部角度未接受。9份文本原件逐SHA归档；详见`runtime-45736-first-failure-analysis.json`及媒体审阅索引。

## supply-frame-r1准备

只修本枪新supply首次grant后的同Frame提案变化：原Frame已严格prepare/commit并允许take_replacement后，记录该FrameKey、sequence、sync、press、input/profile/reference及原ammo lease。仅stage6、同source、同held/item时，HB propose保持原肩部request_ammo并延迟新增incumbent到下一Frame。实际incoming game/其他incumbent保持原样；新Frame、源/对象/控制身份变化、松手或跟踪丢失不匹配此记录。marker不是Frame、lease或动作许可；共享Client的prepare/commit/read比较、首次grant、输入/几何/真实库存检查全部未改。源码接线只在私有冻结树，公共v71未变。

完整PG SHA0d48710fff11b256880d8b512054656f80beaff4b45503500e2f20eab6e43e64；Native/XR未重建。38项source/first-grant安全检查通过，包括实际difference5继续被共享Client严格拒绝、14种source变化和9种非法grant清空marker；不是游戏验证。初始测试fixture误保留Decision默认release_required=1，记忆被正确拒绝，已将合法fixture设0后通过；未改变生产拒绝条件。runner现在选择此候选，尚未实际运行。所有13项原需求保留，整体完成和头显均未接受。

追加实际冻结HandClient整合检查4项：使用本地Host模拟HB提案贡献切换，旧即时incumbent/request切换由真实copy_frame_on_gt重现game_difference5并撤回；采用Snapshot后同Frame首次grant读取保持原观察，下一Frame严格prepare/commit产生retained同lease且无new_grant；同Framegame.busy或显式invalidate仍拒绝并撤回。结果`supply-frame-r1/supply-client-test-result.json`，实际Client头SHA记录；不是Native或游戏运行证明，PG候选DLL未改变。新候选仍等待两个实际存活自动worker之一正常释放后验证，不为此修改其他实例。

## 终态本枪私有shader缓存归档

C盘再次跌至启动下限，排查确认旧私有实例各复制的PSOCache约885MB，旧build仅8至15MB，并非本方当前源码/制品占用主体。仅对本枪已终态221519、215931、214502、225401、232025、233210的`LocalAppData/Coalition/WarGame/PSOCache`归档至J盘。每份先核对isolated-done的game/子进程消失、无强杀/worker_error，再用实际进程PID+creation核对原game/birth/worker/owner/XR身份不再存活。随后同一PowerShell路径执行copy→SHA/大小/文件集核对→原C路径junction读回核对→严格校验本方handoff绝对路径后删除副本。任何不确定/既存映射均拒绝重做。

共630文件/5,310,295,908bytes完整保留，源码、当前候选、日志和录像未移动，其他会话目录未动，旧名仍不重启。观测before_free3,100,655,616/after_free7,835,598,848bytes包含同时外部变化，不归因于本方全部增减。原路径可读，原样内容在`J:/gearsVr/analysis/hammerburst-archive-20261110/terminal-shader-caches/`；六份逐文件SHA索引和总表`terminal-shader-archive-20261110.json`保留在本枪分析目录。归档没有启动游戏或给予任何新运行验收。

## 单次准备后等待首次启动（2026-11-11）

过去名额短暂释放后，复制约0.9GB准备文件再start可能又遇其他worker完成登记。为减少此延迟，本枪runner新增显式`--prepare-then-wait`：要求5GiB（共享4GiB+已测私有准备1GiB余量），只准备一个新唯一名字，随后最多180秒只读等待资源，最多一次首次start。等待期间无worker、无游戏、无输入、不占运行名额、不持共享锁；start仍调用原共享入口，在原短事务下重新核对磁盘/实际存活身份/内存，拒绝也不重新启动这个名字。原默认preflight行为不变。

暂停标记、未知错误、等待截止或迟到准入均中止该未启动尝试；同一个绝对截止不续期。5项针对性离线检查通过（忙后空闲、不预留slot，暂停，不重试未知错误，不续截止，迟到/暂停成功不准入）。此调整只减少本次准备与首次启动之间的竞争，不新增排队/票据/平台，不弱化任何游戏/input保护或构成武器运行验收。

## 实际55584：初始锁扣交接失败，supply修正分支未到达

hb-cloak-1111-000647一次准备，等资源58.532秒/55次拒绝读，名额释放后首次start通过原准入。game55584/creation134388004760825295，supply-frame-r1实际core6+12次六连排空72→0/R78、78条有效/write+trace一致射线、空仓trigger不补弹不出射线通过。但最初锁扣Frame734/sequence4808首次grant后，读回reason7/game_difference6（request_magazine与incumbent）撤回；Frame741输入100.330ms过期；新鲜Frame748到来后原identity精确比较不等，cleanup line640在stage2/angle0.00210334807正常退出原动画。具体变动identity字段当时未记录，不声称只改了hold或lifecycle。没有到旧弹匣stage4，故本次未触发新supply-frame修正分支。

launcher0/operator1/stop0、Native清理/PG入口恢复/XR0、game及子进程退出、无强杀；2147前台/4001CLOAK零违规。517帧errors空、离线encode0；亲看48双眼样本三页全跨度概览及原图00329/00384/00395，实际动作可见，近脸/叠影/白面仍在，00384近处枪手消失后00395又可见；不凭此认定渲染或pose单一根因，不接受完整贴合/连续性。9份文本原件SHA归档；详见`runtime-55584-first-failure-analysis.json`及媒体索引。

## part-frame-r1准备与余下终态缓存归档

新候选仅将同一exact-source交接法显式用于最初锁扣magazine grant：默认Snapshot仍只接受ammo，锁扣须明确传magazine；其他LeaseKind拒绝。只有acquiring/opening且旧item、持有lease和原Frame/source全同，HB propose在该Frame保持原at_latch请求，下一Frame照常提交实际incumbent；不放宽共享Client、first grant、身份/跟踪/输入/期限检查。另在原identity不等的同一次POD比较前输出最多8条位掩码与原/现hold、lifecycle/world generation/input/space；仍走原cleanup，不新增读、clock或身份例外。

完整PG SHA18131f8a91ea7a22463afd3edb515e9c75ec057590e28687c880e9711794f6e6；Native/XR及共享Client未改。45项exact-source检查通过；真实冻结HandClient加本地Host的8项整合覆盖ammo/magazine两种difference5/6重现、首次Frame保留、下一Frame严格retained、busy及invalidate拒绝。不是实际Native/game验证，尚未运行该候选。

为继续实际复测，将本枪余下已终态212059、211559、204446、200135、194156、1111-000647的PSOCache按前述相同身份/完整SHA/junction读回流程归档，又完整保留630文件/5,310,295,908bytes。未动日志/录像/源码/当前制品。首次进程查询遇到进程在取得StartTime前退出，原脚本在任何复制/移动前中止；现最多再查一次，只接受明确NoProcessFound或正读到不同creation，仍未知/同身份存活则拒绝。没有把空StartTime当作退出。总表`terminal-shader-archive-20261111.json`与六份逐文件索引保留，旧总表不覆盖；目录名仍为首次创建的20261110归档根，非伪改实验日期。

002724本次仅prepare后等待资源180秒，原父工具进程已终态exit1，未出现start.log/isolated-worker/launcher-session/isolated-done，game/operator均未启动，不计part-frame-r1游戏验证。原result中prepared_waiting=true是最后中间标记，不能当成进程仍存活；原结果保留，并以`resource-wait-002724.json`补充已终态、未start的证据。后续finally清除等待标记、记录prepared_only_no_start，避免这个状态歧义。按主人临时资源不足继续等待的偏好，未来单份准备后的资源等待改600秒（仅游戏创建前），仍同一绝对截止、不预留slot、不改任何枪内计时；默认资源/首次start的共享检查保持原样。旧名字不重启。

后续C盘虽然仍高于共享4GiB，却低于另一次新prepare所需5GiB余量，因此新调用在mkdir/prepare前拒绝，未产生新游戏。将已确认父工具终态且从未start的002724按原“never-started”归档流程复制/核验/原路径映射：123文件893,930,176bytes完整保留；严格允许列表只加此已知本方名字，新总表`rejected-prepare-archive-20261111.json`不覆盖旧表。未复用此名字启动，也未降低5/4GiB检查。

## 实际54452与插入阶段观测候选（2026-11-11）

part-frame-r1实际hb-cloak-1111-005137/game54452/creation134388031077158286，core6+12次六连排空至0/R78、78条有效且write/trace来源一致射线、空仓禁射通过。锁扣Frame547/sequence5133/token1及新弹匣Frame676/sequence5611/token2两个exact-source交接标记实际出现；全程support withdrawal与identity-lost日志均0。流程到stage6，但原8秒内未到7；finally真实leftUP后6→5，不能倒置为最初失败原因。旧feed_sample共24条在从肩部移动到插入口之前耗尽，最终端点/内核结果/采样间隙/move gate未知，不据猜测放宽几何或期限。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0，game与本方子进程消失、无强杀；2501前台/4625CLOAK零违规。667帧errors空、encode0；亲看48双眼全跨度样本三页概览及原图00326/00468/00524/00595，近脸遮挡、重叠/白色几何仍在，完整手型/连续性/全角度未接受。9份文本原件逐SHA归档，详见runtime-54452-first-failure-analysis.json及媒体索引。released stale-debt wait未出现，不宣称该分支实测修复。

feed-observe-r1只观测已有插入计算结果、已有source时间差/rearm及失败门；不增加source/metadata/clock读取或引擎调用，不改几何、100ms/250ms、真实库存/identity/lease许可。接近区和端点区各24条上限，另早期拒绝12条/move拒绝16条，避免肩部行程耗尽端点记录。完整PG编译SHA c5c04aee6c3e54bb0c9fc3e93cddb1aefcc17a0eb89d4bf4651e6d5a6e3041b9；Native/XR/共享Client不改。真实冻结FeedInsertionStroke加精确提取的修改前后insertion_step，用合成Frame/local_hand完成8场景84成对样本，返回值、arm/observe/reset次数、source游标及原弹匣extraction调用一致；包含连续插入、超100ms间隙、偏轴、丢失local pose、重复source、超速和分区日志限额。不是游戏验收。测试最初因tmpfile弃用警告不编译，改用tmpfile_s；重复source原逻辑保留已有stroke，修正测试预期，未改生产判定。runner选新候选后进行一次新的实际验证。

历史离线媒体runtime-52960/56252/53484共7307文件1,288,578,715bytes已逐文件SHA/大小/文件集验证归档J盘，原C路径junction读回一致；保留原图、录像及文本，没有删掉失败或改变旧验收。索引offline-media-archive-20261111.json。源码仍在C盘工作树，公共v71和整体30%/headsetfalse保持。

## 实际20672：未到插入，原事件期限拒绝

feed-observe-r1/hb-cloak-1111-013055/game20672/creation134388054697893946，实际core6+五次drain6通过，第六次drain首发后service gate4（原pending event release_required）拒绝，总37条有效且write/trace一致射线。最近withdrawal Frame334/sequence4087 stale_before101.0706ms，属于同时记录，尚不足证明整个原事件等待期限的根因；不放宽250ms事件期限或100ms输入。不重按掩盖失败。尚未进入换弹，feed observer分支未到达，不能当新插入实测。

launcher0/operator1/stop0、Native清理/PG入口恢复/XR0，本方game/子进程消失无强杀；1522前台/2530CLOAK零违规。225帧无capture errors，亲看48双眼样本三页全跨度概览，真实动作可见，近脸遮挡/重叠与近处枪体偶发降低/消失外观仍在，未接受完整贴合。8份原文本SHA归档；详见runtime-20672-first-failure-analysis.json。原54452插入失败与本次射击期限失败都保留。

为取得已有插入失败的端点证据，runner增加显式--focused-loaded-replacement：新唯一实例仍经原取枪/core6和原replacement/return-fire动作，只跳过十二次排空前置，使用已有普通部分弹量换弹路径，不改生产代码/库存/许可/动作期限。默认完整空仓路径不变；聚焦结果即使通过也不接受空仓流程或关闭当前射击期限失败。4项mode/环境接线离线检查通过，待一次新聚焦运行。

## 实际53560：插入连续性缺样重置已明确

hb-cloak-1111-013604资源等待134.266秒/124次拒绝读后首次start，game53560/creation134388059272218729。聚焦loaded replacement，core6有效/write+trace来源一致，实际72/R78开始原换弹，锁扣/旧匣/肩取新匣到6。接近阶段result1，Frame784 depth44.1288882mm仍approaching；Frame787/seq4539实际gap110.739ms触发原rearm，depth41.9230863mm已低于45mm外侧接近门，result0。后续Frame802等间隙109.8941ms继续重置。最终Frame851端点depth0.0113263217mm/lateral0.00579742196mm/orientation_dot1仍result0，故本次不是最终未推到位的猜测：已抓到原连续性超限清除approach状态的实际分支。全程move/early rejection、support withdrawal、identity-lost均0。finally真实leftUP后6→5仍不是最初原因。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0，本方game/子进程消失、无强杀，2079前台/3855CLOAK零违规。551帧errors空、encode0；亲看48双眼全跨度样本三页与原图00398/00445/00491，白面/重叠/近脸遮挡与偶发近处枪体可见性问题仍在，不接受完整手型/连续性/全角度。8份文本原件SHA归档，详见runtime-53560-first-failure-analysis.json与媒体索引。该次是部分弹量换弹诊断，没有排空，不当空仓或有限连发稳定性验收。

下一步采用已有生产端真实运动history补齐游戏回调之间的中间几何样本：保留当前Frame/grant及精确身份/mount/publisher/control检查，每对原样本仍须100ms内、序号/时间单调、真实持握无释放、最终点与实际当前pose一致，缺样或身份变化原子拒绝，不捏造插值/时间、不新增枪/骨骼writer、不加宽几何/输入/原事件期限。现有latch_history/controller_relative_motion仅是可复用构件，尚未完成feed接线或验收；不再原样重跑feed-observe版本。20672原pending250ms期限失败、空仓完整回射及其余13项要求仍开放。

Git保存说明：c6a37b0c提交时共享index同时出现其他会话的cover camera-discovery-r141、requirements-owner-sync、cinematic_anchor及其测试和cover文档，合并进了同一提交。它们不是本枪实现/测试的证据；内容保留，不重写共享历史或撤销他人工作。后续本枪使用明确pathspec的git commit --only避免吸入同时暂存内容。本方53560源码/证据已保存且运行终态，下一次运行须先完成上述真实history修正。

## feed-history-r1：真实生产端中间样本接线

新增本枪专用geometry helper与runtime consumer，仅在stage6同item/source游标且游戏回调间gap超过100ms时尝试existing producer ring。每帧capture使用原read_input_continuity一次获取相同controllers及history；固定mount/scale/units、publisher、游戏/Native/世界/lifecycle/各对象、reference generation、press token/control须与先前消费点一致；真实raw两手tracking/stream/input/profile/space、逐帧序号与原时间、同一左手控制持续按住均由真实relative::build检查。中间点位置及方向从原左右手相对姿态投影，不插值、不造Frame/边沿/许可；最终完整pose须与当前local_hand实际测量一致。

纯geometry helper先检查整个最多9点段，逐对仍100ms和原速度限制；在stroke副本上消费，缺样/身份/时间/端点/超速失败不修改原stroke，交回原rearm路径。若中间真实点已经座入，其余真实点也必须持续座入，不能靠历史触碰后退出完成。当前严格grant和原transaction/native完成路径未改，旧匣extraction路径未改。第一遍编译局部new_item隐藏原全局对象被/WX拒绝，重命名first_item_sample后完整PG编译通过，SHA a38e8e85c6c3464ef1beb9b4a4738a501c36924d16b982cb0348bef1b41613d0；Native/XR/共享Client未改。

21项真实FeedInsertionStroke helper检查和22项精确提取runtime consumer检查通过：真实raw continuity/projector/kernel，合成local context/producer fixtures；包括53560实际深度/110.739ms差值的合成中间点证明、旧单点rearm失败、合法连续点插入、缺序/倒时/丢失两手tracking、松手、input epoch、game/native/lifecycle/object/publisher/mount/reference/control/token变化、端点不符以及原子拒绝。中间点不是53560录到的历史，不冒充实际游戏修复。runner选择新候选，下一步一次新的focused loaded replacement实测；空仓/射击期限/完整贴合/13项最终要求不缩减。

## 实际56348：首次按下等待余量，未进入换弹

feed-history-r1/hb-cloak-1111-015245/game56348/creation134388067907247032，首次start前资源读等待0.187秒/拒绝0。已正常取枪，第一次core DOWN Frame193/seq4485/status10，真实枚举是deferred而非硬执行失败；sync118063577524800/admission118063660207000。Frame194执行前readiness valid/same_frame/frame_current/held/registration/live/epoch均1，但reserved_frame_current/reserved_live均0，source83.8485ms，不能满足原20ms余量，原ButtonPress未调用。Frame195/status8 blocked，input admission118063894200400已超过原首次defer sync+250ms=118063827524800；后续同一held DOWN只有accepted、无新press，最终真实release，实际射线0。不要把deferred解释成硬拒绝、不要重复新按下掩盖它。前置准备的具体耗时分配尚未证实，不能只凭此归咎某次metadata读取。

因此新feed history分支未到达，不能用这轮接受或否定插入修正。下一步先针对56348检查首次按下执行之前的准备延迟，减少重复工作同时保留实际当前source/20ms余量/250ms意图/身份条件；不加宽保护，不原样重跑碰运气。20672尾部原事件期限及全部13项需求仍开放。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0，本方game/子进程消失、无强杀，1512前台/2583CLOAK零违规。174帧errors空、encode0，亲看48双眼全跨度样本三页，实际场景/双手/取枪可见，近脸/白面重叠和近处枪体可见性仍未接受。7份文本原件SHA归档，详见runtime-56348-first-failure-analysis.json与媒体索引。源码checkpoint3c20e7d9完整保留新feed43项离线检查及完整PG身份，没有公共安装/整体完成宣称。

## press-timing-r1：首次执行前分段观测

56348只有最终readiness余量不足，不能据此跳过附件/contract/ButtonName检查。新候选保留feed-history-r1，在HB首次press execute添加最多12条RAII分段时长：函数入口、entry/press state、asset、contract、ButtonName schema、最终reserve、owned+dispatch。仅记录器读取clock，绝不替换source时间；析构输出在返回路径，不额外调用游戏/输入/metadata、不中断或延期操作。剥去新增include/observer语句后整份weapon_lancer_r2_native.inl逐字等于父版本，production_statement_parity通过。最大12次观察后无clock读取，7项exact observer合成clock/FILE检查通过。

完整PG编译SHA49aa43dad38eadfb6e207745e725d04766bb4442a7a41584b2836f09a87e6b1f，Native/XR未重建；没有功能修复或游戏结论。runner选择新候选，下一步一次新的focused loaded replacement，先定位真实执行前成本，若到换弹也核对真实history采用；不改20ms/100ms/250ms或缩减13项最终要求。

## 实际53152与reload-priority-r1

press-timing-r1/hb-cloak-1111-020257/game53152/creation134388074371033031，资源等待45.219秒/39次拒绝后首次start。core6通过；首次执行entry source60.337ms、entry checks0.7143ms、asset2.7674ms、contract3.0047ms、ButtonName0.1751ms、final reserve0.003ms、owned+dispatch0.0745ms，实际调用成功。不是首次射击间歇性问题的修复证明，也不据此取消任何重复身份检查。随后初始锁扣Native magazine grant Frame280/seq5135/token1，但source stale101.0049ms撤回，未进opening、未到feed history。

源码与日志顺序均显示原backend先fire，随后nativelancerfollow tick/apply，再reload；本次follow readback在Frame280撤回前记录。新候选只对exact bound HB weapon/hold、left-down或本枪已有active reload，把原HB tick提前到fire之后、follow视觉之前；包括失败在内同一Frame最多一次，设置现有reload_ticked_early避免后续再调，不重试旧输入。原HB tick仍自行copy/校验当前Frame、身份、lease、几何、库存及原动画；分类predicate只是POD调用顺序选择，不是许可。其他武器路径保持原顺序。旧图形组件的恢复/写入仍由原sole follow服务处理，不增加writer。

新完整PG SHAdae40cdbfe03562bc1ef90e03e558ab87c16346c83dcedb15a5e5b6aee5677e8，Native/XR未重建。9项精确调度块+实际POD predicate的local spy检查覆盖early/已经消费/非HB/unknown/weapon或hold变化/idle-UP/active-UP/拒绝后不重复；不是游戏结果。初版直接访问后定义的stage/bound_key导致完整PG编译拒绝，现通过本枪forward函数与既有blocks_fire接口解决，无暴露共享global或改执行许可；最终完整编译及9项检查通过。候选尚未实测。

53152 launcher0/operator1/stop0、Native清理/PG入口恢复/XR0、本方game/子进程退出、无强杀，1943前台/3453CLOAK零违规。376帧errors空、encode0，亲看48双眼全跨度三页，近脸/白面重叠/间歇近枪可见性仍未接受；8份文本SHA保留。详见runtime-53152-first-failure-analysis.json。下一步一次新候选聚焦换弹，保持13项原需求、空仓和首发/尾部期限问题开放，不原样重跑旧候选。

## 实际56032：未证明初始取枪，priority/history未执行

reload-priority-r1/hb-cloak-1111-021724/game56032/creation134388083969403034，资源等待138.359秒/117次拒绝后首次start。core在取枪回执检查失败，没有匹配本次原HB对象的native_equipment held receipt；实际射线0，无reload transition、early reload或feed history标记。部分库存读取inventory_unavailable/current_after；源码该阶段就是snapshot之后原current(nullptr,frame)校验拒绝，现有记录不足区分输入过期、game/identity变化及库存读取具体成本，不能把它定为单一计时根因。不得算作新priority/history实测成功或否定，也不再原样重跑碰运气/重按旧边沿。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0，本方game/子进程退出无强杀，1786前台/3098CLOAK零违规。163帧errors空、encode0；亲看48双眼全跨度三页，实际场景/手及短暂近右侧枪体可见，这些像素不替代权威取枪回执/稳定持枪证明。7份文本SHA归档，补入完整设备状态/input-gap/frame-read行，详见runtime-56032-first-failure-analysis.json。下一步核对取枪原右肩输入/边沿与current_after检查；不跳过库存/身份/新鲜度门。全部13项需求、完整换弹与回射、护木四态/最终贴合/头显仍开放，公共v71/30%不变。本方运行已全部终态。

## inventory-timing-r1：区分库存前后失效

56032现有frame_read记录包含stale_before5及stale_after8，但不能把早期记录套到后面current_after上。完整snapshot仍调用四原GetWeaponInSlot/各ammo/CanSwitch/animation/grenade读；Input已经在库存之前采样，不能以重复移动Input作为修复。新增最多4条idle+12条实际right Grip按住的库存分段观测，记录current_before、原snapshot、绑定、current_after和原Client withdrawal的frame/sequence关联；不重读source/game，不增加库存/输入/engine调用，不改变原语句/返回/许可/期限。原current拒绝若没有同frame+sequence撤回记录，明确withdrawal_same=0，不借旧原因断言。

完整PG SHA9e846150eee799ea90708912ebc1e6df2d10a61a0432a1b334df1813b1c0d224，Native/XR未重建；剥离observer后生产源码逐字等于reload-priority-r1，6项真实observer+合成clock/client/FILE检查通过，限额后clock/withdrawal查询均0。不是功能修复或游戏验收；runner选该候选做一次新聚焦动作，保留priority/history修正和全部13项最终要求。

023254本次仅prepare后只读等资源115.172秒/99次拒绝。advisory看到只有Lancer，正式start短事务时Boltok已先占第二名额，原入口拒绝。工具59437终态exit1，未出现worker/session/done，本方game/operator均未创建；实际进程查询无该实例命令。此为资源准入竞争，不是库存/武器运行失败；记录admission-rejected-023254.json，保留旧名字且不重启。新候选仍未运行，下一次用新唯一prepare等待首次start，不改共享准入/名额或重复旧start。

023618新唯一prepare同样等待158.047秒/132次拒绝，正式start时HOD已登记第二worker，再次被原入口拒绝。工具41155终态exit1、无worker/session/done，无game/operator创建；记录admission-rejected-023618.json，旧名字不重启。已向原协调会话01a1bb5e-5946-7271-af0d-1068759697e0发送具体两次race证据，请其在原共享入口提供“尚未准入的同一首次请求在锁外有界等待→原短事务首度登记”的处理办法，不预留名额、不重启已准入实例、不弱化身份/资源检查。此为实际共享故障协调，不是常规占用通知；没有新线程/助手/共享入口写入。暂不继续重复新prepare竞争，同步做离线检查；inventory-timing-r1仍未游戏运行，库存根因尚未知。

协调会话现已完成原入口修正：isolated_sim创建独占first-start request，runtime_receipts.first_admission只对登记前ResourceUnavailable/短锁忙在锁外等待，再在原锁内重复fresh/pause/disk/实际身份/内存检查；committing在任何spawn/登记开始前设定，之后失败绝不重试。默认60秒，可显式0..600；旧已结束请求拒绝重启。14项共享离线检查报告、三个当前源SHA与first-admission-wait-20261111.json核对一致；本方已读关键hunk，未写共享入口。仅把本枪首次start显式等待设600秒，继续新唯一prepare，不复用两个旧名字；等待全在游戏创建前，不改枪内任何时钟/许可。证据first-admission-integration-check.json；共享实际竞争效果和本枪库存观测仍待新运行。

## 实际57324：库存通过，原按下已过期

inventory-timing-r1/hb-cloak-1111-024442/game57324/creation134388101714967603。原canonical首次请求PID4856/creation134388098990567222曾正读到仍活、无worker/session；之后同一个新实例首次登记运行并正常清理，没有退出换名争抢/重复start。request与worker回执逐SHA保留；这是一次实际同请求等待到准入观察，不宣称所有竞争场景完成。

实际初始Grip Frame331/seq3337：Input producer_sync_gap，previous sync121482017180500→current121482176556000相隔159.3755ms；真实记录的right press seq3333/sync121482085699800，消费observed121482228658600时已142.9588ms，超过原100ms，因此拒绝、重新baseline，held DOWN不重放。相关4条down inventory均phase4/stageok，elapsed4.5663/10.5288/4.5333/8.0841ms，exit input age58.6153/75.6599/54.4694/60.7873ms，均未过100ms。这一轮不能再归因库存snapshot超时，56032 current_after具体原因仍不倒推。无matching held receipt、射线0、priority/history/reload未到。

下一步修正本枪测试驱动的取枪前置：在真实肩部位置先确认fresh released Native baseline，再发一次新的实际Grip并等待真实matching held回执后才把手移开。保持原100ms/source/边沿/身份/库存门，不给过期边沿续命，不加静默重复press；首次/尾部射击、换弹priority/history、空仓/回射与全部13项要求仍开放。

launcher0/operator1/stop0、Native清理/PG入口恢复/XR0，本方game/子进程退出、无强杀，2148前台/3751CLOAK零违规。86帧errors空、encode0；亲看48双眼全跨度三页，初始00000/00002是等待卡、00009黑图，后续实际场景/手/近右枪体可见；不称全部图像为实际动作证明，完整贴合/连续性/头显未接受。7份文本+request/worker原件逐SHA归档，详见runtime-57324-first-failure-analysis.json。公共v71与30%不变，本方所有请求/运行均终态。
