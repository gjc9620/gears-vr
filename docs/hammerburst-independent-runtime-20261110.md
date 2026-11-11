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

## pickup-baseline-r1：以实际释放基线和held回执驱动取枪

原core固定0.6秒Grip后即移手，未证明肩部fresh released Native baseline或matching held先到。现私有driver在肩部真实UP后，最多5秒只等与新命令对应的Source1/current source<=100ms、真实right UP/p==r、Input baseline或quiet的原观测，再发一次Grip；最多2秒等新frame/sequence/source_sync/press+1/event/target/input_control匹配的真实held回执，成功前不移手。源间隙记录优先于held，保留第一次失败并发真实UP，不重按/重放/续期。生产Input/100ms/库存/lease/动画/取枪代码均未改。

原库存observer增加独立最多16条Source1肩部UP观察，限肩部位置只选择观测，不授予动作/改变几何。只有原库存current_after成功phase4且实际Input evidence同sequence/sync并为baseline/quiet才输出hammerburst_pickup_up，记录原Frame/input/epochs/p/r/clock。已有idle4/down12限额保留。11项driver输入/回执fixtures与9项精确observer C++ fixtures通过：旧UP/错误source/未释放/错误target/frame/press/断源不发送或不再重复Grip，计数上限、Input序号/时间/status不符不发布基线。最初矛盾fixture同时出现source-gap与held时会先返回held，已修正为先保留source-gap；生产取枪门未改。

完整PG SHA04f39bad7cd10271978084abe8b3009e4eb2493edff33c0108cd11494334a812，Native/XR未重建；仅扩展观测及测试驱动，不能称取枪/换弹已修复。原43项history与9项priority结果覆盖的代码未变，不重复扩大测试。下一步新唯一实例实际验证一次，保持全部13项及首发/尾部/空仓/贴合/头显要求。

## 实际13492：取枪驱动通过，原首发事件超时

pickup-baseline-r1/hb-cloak-1111-030749/game13492/creation134388115233198588。实际新UP命令122794824617000→Native肩部UP Frame110/seq4473 sync122794858045100/admitted122794915882200；随后唯一Grip命令122794930125300→真实matching held Frame112/seq4482/event1/press1/current HB，source sync122794971318900，回执后才移手。没有source-gap重试，该新driver分支已实测到，不把11项synthetic fixture当作游戏替代。

之后Frame180原ButtonPress调用成功；真实首发event captured122804597859400，仍无射线。Frame181/source4905 sync122804660699000，read-game之后observed122804740625600（79.9266ms），input admission122804743183300（82.4843ms），原deliver gate8在122804744635700（83.9367ms）不能满足20ms reserve；下一service122804852207200距原event254.3478ms，原250ms门取消。最晚withdrawal177是更早的记录，不能借它解释当前181；当前失败证据来自同Frame source与现有delivery gate。原UP/edge/source/expiry门保持，helper停止后真实释放，没有重按。

下一步只对已经实际owned、service validated、同一原press/counters/cohort且仍真实Down的HB FIRST pending，在当前muzzle准备后、read_game/aim/input冗余等待前调用原deliver；所有原native/current/20ms/source100ms/event250ms/claim/engine证据仍跑。不得早发新按下、尾部、UP/新边沿或其他枪事件；同Frame最多一次attempt，避免后面默认deliver再试。此调度修正尚未实现/编译/运行，不能先称首发修复。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0、本方game/子进程退出、无强杀，1626前台/2710CLOAK零违规。168帧errors空、encode0，亲看48双眼全跨度三页，取枪/持枪可见，近脸/白面重叠/间歇近枪可见性仍未接受，非全部角度或头显证明。7份原文本SHA归档；详见runtime-13492-first-failure-analysis.json。无reload transition/priority/history分支，13项需求与30%保持，本方所有运行/请求终态。

## 已编译 first-priority-r1：保留完整Input，首发优先于L2

复核context只要求r2_known，不要求r2_held；完整Input还检查continuity、producer gap、actor/cohort、UP与新边沿。因此撤回上节“read_game/input之前直接deliver”的实现计划，不能绕开这些检查。实际窄改为：只有本轮进入前已经owns_press、精确HB targets、已有initial debt时，将原L2 aim.advance块延后到原Input::advance→after_tick→deliver_ready之后；新按下、其他枪、无source/无债保持原顺序。所有Input/射击语句与20ms/100ms/250ms门保持；L2延后执行使用实际较晚的admission clock，不改任何原输入source或event时间。

完整PG编译SHA787a60f424a948d260ccde981a48316dfa106464b9c25521ae49015276b6e8ba；Native/XR未重建。10项实际C++调度块call-order检查通过，覆盖四个资格条件、前后aim fault、Input拒绝与abort；Input/engine为spies，不能称游戏、anti-replay或首发修复通过。变换逆向还原与原源码逐字一致，完整Input/delivery块未删。构建中因原日志仍使用aim_result/aim_allowed/part_input_reserved而发生两轮变量作用域编译错误，已移到外层，最终完整编译0。

下一步使用全新本方CLOAK实例验证本候选，保留原取枪UP确认及单次Grip，不重按失败射击。候选未运行，完整13项/30%/头显false均保持；不改publicv71或稳定手动入口。

## 实际22896：原首发1发通过，松手尾段仍失败

first-priority-r1/hb-cloak-1111-033308/game22896/creation134388128429694897。32.312秒/25次纯只读资源等待后同一次首次start准入。fresh UP129→唯一Grip→matching held131成功；204首按因最终reserve未执行而由原Input defer，205同一原press实际ButtonPress调用。206原first outer实际交付、count1，唯一射线trace/write/current来源一致。当前已有owned/first debt符合L2后移条件，206 input admission与observed同刻；单次未配对运行不证明调度因果或稳定首发修复。

测试见首发后真实UP，207原Input status4/owned0/p1r1，保留burst尾段。208 source原sync124117990271100，read-game observed124118072713100，源年龄82.442ms，已不满足20ms reserve。随后service gate4/count1明确原事件过期，withdrawal208年龄101.3219ms同时记录，但日志缺尾事件原capture/serial及207验证状态，不能断言该withdrawal是唯一根因。无重按、无尾段补造、无reload触达。

检查真实源码发现service-before-Input→真实UP release_completed清validated→after_tick→deliver路径未重新service。check_release_handoff_order.py用原实际adapter复现：已pending并service validated的refire，在真实UP成功后validation变false，本帧deliver gate1不派发；原事件未改serial/capture，重新完整service后同事件派发且第二次deliver不重复；超过250ms仍取消无射线。新增14加原308=322通过，common queue/engine/contract为fake；它证明存在该处理顺序缺口，尚不能证明游戏207当时已pending或该缺口是唯一原因。下一步只为成功真实UP、已有同一refire事件补完整验证，保留原全部source/20ms/100ms/250ms/identity/cancel门，并记录原pending capture以辨别实际时间。

本方launcher0/operator1/stop0，Native清理/PG入口恢复/XR0、game及子进程退出，无强杀或workererror；1491前台/2534CLOAK零违规。161帧errors空、encode0，亲看3页48双眼全跨度样本，实际场景/取枪/持枪可见，前段手臂枪重叠、后段枪贴近右眼仍不接受；非全部帧/全部角度/头显证明。7原文本SHA保留，runtime-22896-first-failure-analysis.json与13项audit已更新。完整六连发、换弹、四态、贴合、交付仍未完成；分数30%/头显false保持。全部本方运行终态，不重复启动同实例。

## release-handoff-r1：实际UP交接重新验证已编译

上一目标轮为progress：源改/完整编译/实际22896及322项真adapter局部复现均有证据。本轮在成功release_completed全部16个原gate之后，仅当已有refire pending时保存原serial/captured及本次frame/sync。原Input::advance完成实际owned→released后才调用after_release，一次消费handoff，要求原事件/本帧/sync/原cohort/press及唯一release匹配、跟踪可用、无cancel/firstdebt，再重新执行完整service。service只验证；原after_tick/deliver仍唯一发射点，原100ms源/20msreserve/250ms事件及identity/state/metadata检查不变。无新队列、重捕获、更新时间、松手补按或新许可。日志最多8行记录原captured/serial与service结果，不能借此授权。

完整PG SHAe75af846db1529aaeaa2576f7df52380436e67720339f93ff48c7be299609beb，Native/XR未重建。430实际adapter fake检查通过（原308+新增21情形122项），覆盖同一pending尾段、new press/release、取消、reference/sourceframe/sync/hold变动、仍owned、event串/时间变化、元数据/registration/state失败、reserve失败、250ms过期、handoff缺失/一次消费、UP变DOWN、stop、tracking与pendingFIRST取消。queue/retailcontract/engine/source仍fake，不证明实际game或头显。准备脚本一次边界匹配断言失败在编译前终态，改用完整行边界后生成，最终编译0；未复用游戏名字。下一步单次新隔离实例，保留单Grip与首失败停止。全13项/30%/publicv71保持。

## 实际24648：首发准备过期，新增UP交接未触达

release-handoff-r1/hb-cloak-1111-034331/game24648/creation134388136103748532。178.515秒149次资源只读等待后同一次首次start；未停止其他实例。fresh UP130→单Grip→matching held132成功，无source-gap重试。224同一次扳机DOWN入Input后status10 defer；真实execute entry source年龄76.302ms，entry checks2.317ms/asset6.3101ms/contract3.5035ms/schema.0796ms，最终20msreserve拒绝，ButtonPress invoked0。225 source read年龄91.5502ms、preparation_budget0，原edge sync124868388918600+250ms=124868638918600，Input admission124868664114200已经晚25.1956ms，原Input status8封掉该press；226更新鲜的source也不能重放已过期按下。零first capture/零射线，不是同一次按钮多次重按。

没有hammerburst_release_handoff，新增交接/430fake checks的产品分支本次未触达，不能称修复通过或失败。没有reload/priority/history触达。下一步检查重复首发资产反射：当前muzzle准备已有MuzzleSample.asset与两次asset read，ready_for/live又读对象/指针；execute另做全asset read。需逐项对比type/selector/hidden/attachment/identity/结构变化检查并证明等价或建立同Frame完整准备，不能简单删除新鲜检查或延长100/20/250ms。

launcher0/operator1/stop0，Native清理/PG入口恢复/XR0、本方game及子进程退出，无强杀/workererror，1264前台/2216CLOAK零违规。149帧errors空encode0，亲看3页48双眼全跨度，真实场景/取枪/持枪可见，反复贴近右眼的大枪与间歇枪可见性/手臂枪重叠仍未接受；非所有帧/五处贴合/全部角度/头显验收。7原文本SHA归档，runtime-24648-first-failure-analysis.json与完整13项audit已更新。所有本方运行终态，不重复同实例，不再跑未改候选；完整目标active，30%/publicv71不变。

## press-assets-r1：拆分既有首发准备耗时，未删校验

上一目标轮progress为release-handoff源/编译/430fake/实际24648首发失败证据。当前逐项比对纠正上一段猜测：ready_for/live_sample本身只检查Frame/持枪/epoch/registration/源时效，**没有**资产selector/hidden/挂接metadata校验；完整指针检查在typed_result等独立consumer，不能混同。现有MuzzleSample.asset不能直接替换execute全资产read，故不实施该删减。

原press_timing的asset_ns实际包含lancermain_asset::read、nativeequipment::attached_frame及末次current三项，24648的6.3101ms无法指认其中哪项。新私有press-assets-r1仅在原三个短路布尔结果上加原样返回的计时wrapper，每个原调用仍一次、原顺序和失败短路不变；只有原最多12行active HB timing新增3次时钟读，既有行输出细分asset_read_ns/attachment_ns/post_asset_current_ns。无source/许可/事件时效/输入动作改动；非active零时钟读。

完整PG SHA4174354ac6c4d7243560379c0f85b7027ed1a0d288ac458b230371c3571d0d33、Native/XR未重建；10个精确observer fixture覆盖disabled/无log/null/完整与3个失败短路/缺失阶段/无效index/12行封顶，全部通过。源变换逆向恢复原condition逐字一致；无新产品优化或修复通过声明。下一步用单个新隔离实例取得三项实际耗时及已有交接路径情况，随后针对实测最慢部分改实现；不重复未改候选，不放宽100/20/250ms。13项/30%/publicv71保持。

## 实际57960：肩部UP日志封顶先于驱动命令，测试驱动已修正

press-assets-r1/hb-cloak-1111-035727/game57960/creation134388143847200248，纯只读资源等待30.297秒20次后准入；actual noGrip。16个shoulderUP marker全部在驱动UP125635619882900之前；末行194/seq3194 sync125635519400700、admitted125635548221200，admitted先于UP71.6617ms，producer先100.4822ms。原驱动先完成reach加250mssettle，再发送UP，首批marker预算已耗尽。没有输入source-gap、没有Grip、取枪/首发计时/新增handoff/reload均未触达；不能拿这次归咎于首发或产品交接。

修复仅在本方Python测试adapter：acquire先发真实UP；原有限reach可选ready回调在每个原gesture sample之后读取真实NativeUP，首次fresh且post-own-command marker成立就停止剩余UP movement，更新测试当前手pose，再发送唯一Grip并等待原matching held。未改变原轨迹生成、姿态/年龄容差、原Native16行预算或产品代码；其他reach默认仍完成原全路径与250ms停顿。失败不重按。16项驱动fixture（含原11+移动期间有效/旧/缺失marker3+实际改写reach函数两条路径）通过，8adapters解析通过；非game证明。

launcher0/operator1/stop0、Native/PG入口/XR0与本方进程正常清理，无强杀，1177前台/2040CLOAK零违规。106帧errors空encode0，亲看三页48双眼全跨度，真实场景/肩部手臂可见，初段重影/手臂重叠仍未接受，无持枪/射击/换弹或全角度/头显证明。7原文本SHA归档，runtime-57960-first-failure-analysis.json与13项audit更新。下一步新实例使用修正driver和**同一已编译**press-assets-r1，测试调用方已变，非无改重跑。全部本方运行终态；13项/30%/publicv71保持。

## 实际20284：同一准入请求完成；单Grip无held，修正提前结束路径

上轮progress含press-assets源/编译/57960失败/driver修正。本轮先多次精确poll原exec43764确认live，资源只读两实例占用并保留原首次start37620，不另建/重启；该请求后来正常准入，hb-cloak-1111-040600/game20284/creation134388153672707145。advisory资源等待131.969秒121次，canonical首request04:08:15仍在原600秒内获准，不能将中间观察超时当作terminal。现43764实际terminal1。

真实UP126622248619400→NativeUP499/seq3169 sync126623766887900/admitted126623789975900→唯一Grip126623794522500（距原sync27.6346ms），2秒未收到matching held；failureUP126625799517600。无native_equipment_input_gap，零射线/零press timing/无新增handoff/reload。源码复核确认上轮ready回调一旦40mm宽观察区有UP就return并停止剩余gesture，不能保证到达原请求终点；真实提前停手的pose未记录，不能断言这是本次唯一原因。此前16fixture竟将早停当预期，现明确纠正，保留原失败证据。

本轮driver取消callback早停：每个原gesture sample继续完成，仅观察；最终g.poses必须与原shoulder请求字典完全一致并记录shoulder_endpoint，否则不发Grip；只去掉本次取枪的后置250mssettle，其他reach原样。新private endpoint-baseline-r1仅把observer区域从40mm缩到5mm，防粗观察区提前消耗16行预算，**实际取枪Volume/动作几何/时效均未变**。17driver fixtures（含未到endpoint不得Grip和原真实改写reach必须执行全部sample）/10真observerfixture（额外三轴6mm不记行）及8adapter解析通过。完整PG SHA63b915a914b08e9d2a834c1946b454a0d642b9cb3e00945bec5e68a681b3d93b，Native/XR未重建；该新版本尚未game验证。

20284 launcher0/operator1/stop0、Native/PG入口/XR0、本方game/子进程正常退出，无强杀/workererror，1229前台/2114CLOAK零违规。54帧errors空encode0，亲看3页48双眼全跨度；00000..3等待卡、00016黑帧保留，其余有真实场景与肩部手臂，无成功持枪/射击/换弹或全贴合/全角度/头显证明。7份原文本SHA归档，runtime-20284-first-failure-analysis.json/13项audit/active-runtime-wait已同步终态。下一步仅一个全新endpoint-baseline-r1实例，先actual exact-endpoint+freshUP+单Grip/held再测首发拆分耗时。无本方live运行；30%/publicv71/全部未验收范围不变。

## 实际21288：记录等待卡后超时，动作未触达；首张实际投影startup门已接线

本轮接续endpoint-baseline-r1/hb-cloak-1111-042322/game21288/creation134388160595100885。资源只读228.86秒206次后单次start。Native手部连接后录像启动，只保存2张相同SHA的STARTING VR等待卡，亲看原图确认，第三个请求5秒未得到fresh完整截图。action_live抛错阻止equipment/core，未产生hammerburst-core.json、无Grip/首发/计时/新handoff/reload。没有把截图失败当成武器失败，也不认图像ready或补造video。

首张等待卡可以触发旧HB_first_action_capture_ready，故“Native手部连接+第一张composed image”不够。现有共享continuous报告会在实际XR epoch1投影成功xrEndFrame后独立写入startup_timings.first_stereo_submission，带原QPC；本轮只复用它，不新增共享writer。Binding存本方初始report/sessionid，读取时仍要求本方live/pinned publisher、原game及XR完整identity、原session/source1、真实非未来QPC及非runner观测的明确actual marker。start_action_recording先以15秒有界等待这个startup正证，期间无截图或武器输入，然后启动原recorder；原first capture5秒、每张请求5秒和录像错误停止后续动作保持。这个startup marker只准开始录像，不能充当当前Frame/输入/发射许可。

13项helper测试通过，包含7原例/延迟真实投影/缺失15秒不得start/identity丢失/实际投影之后仍5秒capture门、11种receipt反例及缺失/坏JSON；8adapter解析通过。未新建/修改Native/XR/PG制品，同一endpoint-baseline-r1 SHA63b915a9保持；修复尚未game验证。

本次launcher0/operator1/stop0、Native清理/PG入口恢复/XR0与本方进程正常退出，无强杀/workererror，1264前台/2141CLOAK零违规。2帧及原录像error完整保留，未encode（失败recorder未生成frames.txt），8原文本SHA归档，runtime-21288-first-failure-analysis.json与完整13项audit更新。启动/截图现象的全部根因仍未知，此改消除已知错误就绪条件，不称已修好renderer或全武器。下一步先保存源码，再用新唯一实例验证startup→精确端点取枪→首发分项耗时；原13项/30%/publicv71保持。

## 实际54720：首实际投影门＋完整端点取枪通过，首次DOWN原源过期

原exec5500续跑后terminal1，endpoint-baseline-r1/hb-cloak-1111-043643/game54720/creation134388169642126146。资源只读347.469秒319次后同一次start；无另启/重放旧实例。实际事件依次native_hand_connection_ready→HB_real_projection_ready→HB_first_action_capture_ready；152录像frames/errors空，首幅及亲看48全跨度paired样本均实际场景。startup门此狭窄分支已实测，不等于全部startup/renderer/头显通过。

原完整gesture endpoint实际记录x.23/y1.5850000477/z-.17/yaw0/pitch-.3000000223/roll0；真实UP128217868964300→NativeUP148/seq3440 sync128219832250400→唯一Grip128219893729200→matching held151/seq3449，确认后才移动。无source-gap重试；新完整端点driver/5mm observer已实测到，非单靠17fixtures。

首发UP baseline238原sync128228193001200/admitted128228262132900，source在admission已69.1317ms，随后DOWN命令128228274868300。实际首DOWN239/seq3820原sync128228343425200，muzzle rejection源102.0321ms，Input admission128228448301800时104.8766ms/current0，正确seal；240新鲜source50.6063ms只baseline，241 blocked8不重放。原helper误将后来242/status8叫consumedDOWN并等射线；本轮仍零ButtonPress/first/射线、没有press_assets/handoff/reload触达，不能归咎到未执行的资产耗时或交接代码。旧故障保留，不因后续driver修正抹去。

修正仅测试driver：UP就绪须producer距当前观察<=60ms且原preparation_budget1，admission>=原producer；这是发出唯一DOWN之前更严格的setup选窗，**产品100ms/20ms/250ms限制与source/anti-replay代码均未改**。DOWN若已有明确Input status8则保存first_down_rejection并立即失败/实际释放，不再误报成功消费，也不重按。9 helper tests通过，含旧source/新日志不能当就绪、budget缺失、等待更及时UP才唯一DOWN、blocked立即失败、原whole-flow deadline/尾段与错误发数；fixtures不证明稳定游戏射击。

本次launcher0/operator1/stop0、Native清理/PG入口/XR0与本方game/children正常退出，无强杀/workererror，1329前台/2248CLOAK零违规。152frames/errors空encode0，亲看三页48paired全跨度，有取枪持枪，但右眼近大枪/间歇消失重影/手臂重叠仍未接受，非全帧/全角度/五处贴合/头显。7份原文本SHA归档，runtime-54720-first-failure-analysis.json与13项audit同步。

C余量实际降至3.47GB，禁止再启动。只归档本方6个已terminal＋逐一identity gone的033308/034331/035727/040600/042322/043643着色器PSOCache，各105文件885049318bytes，共5310295908bytes；先复制J、全SHA/文件集合/长度核验，再保留C原路径junction并核验读回，才移除已验证临时副本。源码/日志/制品/录像未移动，其他owner/活动实例未触碰。archive_terminal_shader_caches_r2.ps1及terminal-shader-archive-20261111-r2.json保留全部证据。下一步保存后仅新唯一实例验证修正setup；PG/Native/XR未重建。全13项/30%/publicv71/头显false保持。

## 实际21596：出生回执重复阶段发布WinError5，非武器结果

续原exec72902后terminal1，hb-cloak-1111-045812/game21596/creation134388183099827305，operator未启动。原reserve.log确证mark_launch_stage更新injection_attempted时birth.json.tmp replace birth.json抛WinError5；launcher1/stop1，不是正常完整runtime清理通过。done仍确证本方game absent/children空/无强杀/workererror，85前台零违规，CLOAK0样本不冒称覆盖。无continuous session/Native/XR/武器/录像证据。

两个最终birth与tmp均stage injection_attempted true；源码在optional Playground reserve之前和capture DLL load之前各标一次，因此不能因本次replace失败声称没有任何此前PG远程操作。保留原injection unknown清理分类与原失败，不复用旧实例。8份原文本SHA归档，runtime-21596-first-failure-analysis.json/13项audit/active-runtime-wait终态同步。

已按主人既有“模拟器测试自行协调”范围向既有协调会话报告具体新共享故障；未创建helper或新chat，未修改共享launch_reserve。本方先做真实Windows临时文件reader实验4例：旧Path.open持有时replace拒绝复现；仅CreateFileW只读且SHARE_READ|WRITE|DELETE也仍拒绝，故2通过/2error，明确失败而非修复。本方runner接线已撤回；shared_receipt_reader.py/其check标FAILED并保留实验，运行不import它。不能宣称原故障唯一原因已证明。

协调者已单独窄修共享publisher：同一payload只写一次、replace最多6次，仅5/32/33、最多5次25ms等待，后来每次前要求完整原receipt仍等于previous。identity/schema/stage不放宽，其他错误立即退出；绝不重跑game/reserve/inject/prime，最终失败保留tmp和原injection未知保护。12项离线检查原记录复用、不重复跑，本方核对两文件完整SHA与实际变更hunk。launch_reserve SHAa4c44d9dc20afb11edb84beffe70c5b968ab0b76be7d3958bd2efe04c9899edf，test6385698f3262c9afd0d86f6de5fc1fa418bcbbd531dd969f43b78d9fed8961f4；birth-publication-integration-check.json保存审阅与共享proof SHA，无game实证或根因修复声明。

下一步保存后新唯一实例，使用原endpoint-baseline-r1/新首发UP-only60ms setup及共享publisher窄改；产品DLL/Native/XR不重建、不改旧实例SHA。54720 startup/完整取枪的局部实证保留，首发9fixture仍未live验证，全部13项/30%/publicv71/头显false保持。

## 首次重检在prepare前因磁盘余量拒绝；r3只归档已退出本方缓存

03378682保存后新请求在run_isolated_empty.py:70 existing4GiB+1GiB margin检查抛错，返回1；代码尚未生成实例名/target/prepare/start，因此没有新game或operator，不复用21596。该磁盘前置拒绝与21596共享发布故障分别保留，不伪造runtime启动/cleanup。

使用同一已审阅archive路径/identity/SHA/读回/junction保护，r3仅扩展到本方已terminal正常退出的005137/013055/013604/015245/020257/021724/024442/030749；各自owner/game/XR精确lifetime退出核对后才移动，活动实例和其他owner未碰。实际8目录736文件6195345229bytes，不把首目录已只剩1文件3bytes虚报为885MB；其余7目录各105文件885049318bytes。所有内容J副本SHA/文件集合/长度及C原路径映射读回通过，原源码/日志/录像/制品不动。terminal-shader-archive-20261111-r3.json保存after_free8543346688，后续启动仍需重新查实际资源。

未新增游戏、未改变任何武器/输入/source门，目标active。下一步同一当前候选的新唯一实例，使用已核共享publisher窄改与本方首发setup改；21596实例不重启。

## 实际22992：loaded replacement＋两组原六连发聚焦通过，不是全枪完成

原exec29637 terminal0，endpoint-baseline-r1/hb-cloak-1111-051514/game22992/creation134388191263534883。资源只读120.016秒110次后单次start；普通新共享publisher启动通过，不证明本次真的触发WinError5 retry，也不声称根因已查明。operator0/launcher0/stop0；Native清理/PG入口恢复/XR0、本方game与children退出，无强杀/workererror，2171前台/4031CLOAK零违规。

actual exact-endpoint pickup与bound first-real-projection gate再次成功。新的UP-only60ms setup在真实首按前使用更及时source，第一次ButtonPress272 sourceentry40.1577ms，第二次767为54.7879ms；原压检/asset/contract/20msreserve全跑，均invoked1。两段各原六发，共12条trace/write/source一致；每段在首发1射线后真实UP，剩余原有限尾段完成。新release_handoff实际274/event2 captured130386035771100和769/event8 captured130427202034200，原serial/captured相同、service0/validated1/no_dispatch1，然后原deliver产生对应第2射线，未重捕获/更新时间/二次派发。此产品分支已实测到，不再只有430fake检查；单次正例不证明所有延迟稳定性/唯一因果。

普通partial loaded replacement：typed72/78→原lock打开/旧mag抽出/新supply→插入→锁扣闭合，typed78/72；再六发后typed72/72、used6，真实after_return_fire与exported6一致。latch369/token1、supply500/token2同Frame路径、early_reload369 before_active0→1/once1实测。history7行，其中4次used1/points6/result1桥接原producer间距>100ms（如562 gap112.4448ms），strict当前源/20ms门和几何未变；最终feed618 result2。此前feed-history分支未实测的限制在这个具体路径已补证，不宣称所有插入轨迹完成。6条support_withdrawal发生在seat/closure后仍原样保留（含difference6/2），identity_lost0，不冒称全局零withdrawal。pause/Bresume实际通过。

首发asset拆分实测：read.2793/.2771ms，attached1.5100/1.5149ms，postcurrent.6263/1.0278ms；总asset2.4156/2.8198ms，contract2.4497/2.9725ms。更及时来源下原检查可完成，不删读asset/attachment也不把此前6.3101ms断言为单一asset rootcause。

582帧errors空、encode0，亲看3页48paired全跨度及6个实际contact185/297/334/420/470/494。实际取枪/锁扣/弹匣手动作/暂停恢复可见，右眼近大枪、重影/穿插/副手张开和分离仍存在，指南细字可读性未验收；不是全部帧/全角度/五处皮肤贴合/头显通过。8份原文本SHA归档。输出runtime-22992-focused-result.json明确focused_actions_passed=true/full_goal_complete=false，旧latest failure21596保留，未把成功批次伪称failure。

这次刻意focused-loaded-replacement，**未**做empty drain/零备弹、retained原匣复插、双手及护木四态、perfect cancel、tracking/switch/stowrestore或完整贴合。全13项audit已按实际局部结果更新，30%/publicv71/头显false不抬升为整项通过。下一步保存后新唯一实例跑原FULL empty路径（不传focused标记），验证78发耗尽/empty veto/empty换78/0/复射6typed72/0；首失败停止，不重复已通过的普通loaded聚焦路径。原其他未验收项继续保留。

## FULL empty052600在Python预检拒绝，原wrapper结束但本方worker仍live

上轮progress为实际22992普通loaded链通过/保存/发起FULL empty。续原exec34409确认terminal1，hb-cloak-1111-052600无game_identity、无birth、无operator/continuous session/录像/武器动作。reserve.log首故障Start-DebugGame.ps1:48明确GEARSVR_PYTHON not usable；原5秒probe失败原因未输出（timeout/exit/stdout/stderr均缺），不能断言解释器坏或是此前birth replace问题。资源只读4.547秒4次后原single start，启动不重试。

wrapper stop1/cleanup_unresolved true，**不是**game absent或正常cleanup通过。读取原worker.json并用WindowsControl逐一核对，worker53716/creation134388198120486950 exact still live。isolated_sim.py finally在known.game=None时children固定unknownTrue，所以保守循环，不生成done并继续占资源；无按进程名杀/无强杀/无补造birth/无释放未知reservation。本方暂停新增runtime，旧139的no_birth未知不借本次日志一起洗白。

只进行一次明确GEARSVR_PYTHON同一路径的Start-DebugGame.ps1 -PythonCheckOnly：exit0/0.4604311秒，返回相同解释器/check_only true；代码在该开关返回前不启动game。此当前正例不修复、不解释原失败，不作为重开旧实例的理由。已向既有协调会话发送原日志、exact live worker与prelaunch源码顺序，没创建helper/新chat，也没并行改共享文件。协调snapshot cursor07910094-1b3c-4665-8069-e920b2d0b42e:4显示其正在增加绑定本次attempt的预启动拒绝回执与Python probe具体诊断；仍不把普通缺birth当作无game，尚未收到完成/收尾结果。

prelaunch-052600-result.json和5份原观察文本保留；cleanup-pending标为mutable observed-byte snapshot，未把它当静态原件或已终态。本次Start-DebugGame源码SHA仅observation-time，不伪称launch-time证明。完整13项audit明确full empty未执行；22992的12射线/ordinary replace/pause局部通过保留，empty/retained/four-state/其他interruptions/fit/headset仍开。下一步跟踪本方exact worker与协调修复，正常收尾确认前不新增测试、不重用052600。目标active，30%/publicv71不变。

## 预启动新协议已核；旧53716不热更新，两个旧worker占满准入

协调已完成future-only修正，python-prelaunch-rejection-20261111.json记录5项真实PythonCheckOnly/81项launcher offline/15项runtime offline与parse通过，无新game。本方逐一核四文件完整SHA并读改变hunk：新PS1保留probePID/status/elapsed/exit/stdout/stderr/exception，只有Python guard在game creation/injection/prime全False时发布绑定exact LaunchReceipt和脚本SHA的.no-start sidecar；launcher验证source/path/flags且无任一birth；新worker在自己的exact launcher退出后才采信typed prelaunch_rejected，done另列no_game_started=True、owned_game_absent仍null。普通no_birth/错身份/有birth继续未知，没被放宽。

本方wrapper原样保留done且passed仍须operator0和真正owned_game_absent，因此已兼容这个新类型，无需额外改变或把no-start刷成gameplay pass。prelaunch-rejection-integration-check.json保存源码SHA与审阅，不重复共享测试、不启动runtime。旧53716和旧139未热更新、未回填新sidecar、未停止；本方53716 exact identity再核仍live。

一次只读resource preflight当前明确active=['hb-cloak-1111-052600','lancer-natural-v139-225928']占满两名额。故不是一个会自动释放的普通等待：后续源码修复不能让旧运行代码正常退出；没有新增实例、没有放宽名额/身份、没有伪造game absence。需要单独解决本方旧控制worker的退出权限/资源占位，其他owner139不在本方操作范围。22992 loaded局部通过继续有效，052600完整空仓未执行，全部13项和full goal仍未完成。


## 2026-11-11 视觉归档修正与资源恢复

22992 finalizer将不存在的fivepartfitting ID改为真实part_fitting，并断言目标需求存在；把已亲审的48组双路概览及6张原始contact证据关联到手部贴合和引导条目。两项继续未接受，不增加运行或头显通过。PG静态复核只确认被动proximity与有效support lease分支边界，不替代Native/当前四态实测。

协调方按用户明确释放指令强制结束旧052600 Python worker，原失败和异常收尾保留；没有强杀游戏或补造birth/done。证据owner-resource-release-20261111.json。新115143经正式入口准备一次并等待准入，继续完整空仓耗弹、换弹和回射，不复用旧实例。完整13项尚未完成。


## 2026-11-11 game15016：四次六连发后UP观察超时

新115143正常启动取枪，核心6发与3轮耗弹各6发，24条实际有效且来源一致射线，typed54/78。下一轮只发UP，未取得严格60ms的setup回执而超时，未再按DOWN，未到空仓/换弹。失败窗口11条回执最小写入源年龄54.9158ms，缺少测试观察时年龄，不能断言根因。补充最多8条不同回执及最小源年龄、原producer/log/observation时间和live/read耗时；不改60ms/setup/deadline/生产保护。11helper测试及8adapter编译检查通过，新诊断尚未进游戏。

原operator1保留，stop/launcher/Native/PG/XR正常结束，无强制清理，无FG/CLOAK违规。226帧无capture错误，编码0；3页48组paired概览及raw120/158/196亲审，近脸右眼遮挡、手分离与重影未接受。证据runtime-15016-first-failure-analysis.json，完整目标未完成。


## 2026-11-11 game22048：完整耗尽/空扣通过，入座前源时效撤销

新115944实际78条有效且来源一致射线，12轮耗弹及typed0/78空扣无射线通过，原begin与换弹1→6、新匣获取通过。插入token2在698源年龄100.1528ms首次撤销，706/707继续过期，708 decision12无lease，709起granted0。最后接受的706深度21.913565mm、lateral约4.9e-6、orientation1；未入座/未关闭/未回射。805 lifecycle difference64为后续现象，不当作首因。保持100ms与anti-replay，下一步追踪Native源选择/发布至support consumer的实际耗时，不能保留过期控制。

新增test-reader记录已实练，12次UP观察45.326～59.517ms，最大读取1.9238ms；本轮未复现15016超时，不声称已经修复。operator1保留，stop/launcher/Native/PG/XR正常0，无FG/CLOAK违规、无强制终止。699帧无capture错误，编码0，3页48组概览亲审；遮挡/重影/白色几何与手贴合仍不接受，密集contact待检查。证据runtime-22048-first-failure-analysis.json及原嵌套ordinary_empty_replacement动作归档。完整13项未完成。


## 2026-11-11 新匣姿态空窗持有者修复（尚未游戏验证）

22048源码追查确认：tick在copy_frame失败时清空HB held，即使只是publication/stale_before。下一帧propose不再申报同一新匣incumbent，Native owner按规则返回lease_lost12。窄修复只对stage=new_magazine、非零new_item、held=ammo且id相同/lifecycle有效，保留逻辑原持有者；失败当帧不提供Frame、不移动、不入座、不写库存。下一个独立新鲜prepare/commit仍证明全部对象/输入/身份/原lease，原history/间隔/anti-replay不变。其他原因及阶段保持原reset。

实际旧reset与新helper经同一真实HandClient/Native owner fixture对比，旧路径复现lease_lost，新路径仅新鲜后继retained、真实UP released；20新增检查和原client套件通过。完整PG编译成功，SHA b9296d6a6859673e461335efc7caded2bbf1375403d5a667f5d77d6ade096f2d，Native/XR及publicv71未改。最多8条分支记录供后续实际验证，不能以离线fixture称空仓完成。


## 2026-11-11 game464：新匣修复未练到，原连发在源失效后按规则取消

pose-gap-incumbent-r1实际core6+6轮耗弹6，42发后typed36/78。下一次first43后真实UP337 handoff validated，339 copy stale_or_changed_after，原release gate1 count1取消；保留43条一致射线，缺失第43发后的最终typed。未到空仓/换弹，新修复分支0条，不能判定通过或失败。正常stop/launcher/Native/PG/XR退出；225帧无capture错误/编码0，3页48组概览亲审，遮挡重影白几何继续未接受。下一步改用短loaded换弹定向练新分支；完整空仓与source339失败仍保留，不缩小13项目标。


## 2026-11-11 game47240：原匣抽出未完成，短路径仍未到新匣修复

新123129/core6一致射线通过，原卡扣1→4，随后真实松手触发cleanup474/stage4，因为extraction.complete为false。最后有移动记录深度67.246405mm，原日志只在位移超过10mm时记录，不能据此断言最终命令端点未到。清理前未捕获support withdrawal，新匣helper0条，不能把此失败归因于新修复。原提取器会在样本gap>100ms重置，但本轮缺少逐次observe接受/complete记录，下一步先窄加只读回执定位；不改阈值或原保护，不做不变重跑。

operator1保留，stop/launcher/Native/PG/XR正常0，无强制终止和FG/CLOAK违规。359帧无capture错误/编码0，3页48组paired概览亲审，近脸遮挡/重影/白几何继续不接受。新匣原持有者修复仍仅20新增离线检查、原客户端检查及完整编译通过，尚未实练。全13项目标保留。


## 2026-11-11 原匣抽出检查只读诊断

extraction-observe-r1仅把原条件中的extraction.observe一次调用包装为最多32条原结果记录，包含accepted/complete/gap/depth/lateral/aligned/原sync；不改变几何、100ms间隔、授权、轨迹和库存。45次实际wrapper与原kernel结果一致/调用一次/日志cap检查、原连续抽出和10类失效测试通过，完整PG编译通过，Native/XR未重编。下一次短loaded路径只定位47240的原匣抽出失败，不作为完整空仓验收。


## 2026-11-11 game43944：实际Native input_lost，纠正清理回调归因

extraction-observe-r1实际4条一致射线，随后service gate3/count4；disarm_reason257=Native input_lost，元数据epoch已变，先撤销再进入null-source release gate1。withdrawal299的99.7802ms是较早读回执，不能当作Native实际失效时刻；更早229的142.7236ms在first294之前，明确排除其直接因果。464则是service gate4原事件过期，均不能绕过。

另外离线复现一个回调WAIT接线缺口：只有先前真实UP证书仍有效且未过原期限，null-source清理才可走原debt-only WAIT。旧码复现exit17，新码335原adapter检查及合并handoff457检查、完整编译通过；停止/epoch失效/重新持压/250ms过期均取消。这个released-pose-gap-r1实验无法修复已明确Native输入失效的43944，也不能修复464原过期，因此未选用，runner继续extraction-observe-r1。保持实际失败，不以绿测试替代因果。

43944正常stop/launcher/Native/PG/XR退出，无强制终止和FG/CLOAK违规。148帧无capture错误/编码0，3页48组paired概览亲审，视觉缺陷继续未接受。下一步从真实806dcbd6冻结Native源码定位broker active_now的prepare/commit失效阶段，必要时只加有界只读分类。原匣诊断和新匣修复均未实练，完整13项目标未完成。


## 2026-11-11 冻结Native输入失效阶段诊断（尚未运行）

基于真实806dcbd6冻结源码与原生产flags/minhook构建，只有weapon_hand_broker.h及weapon_hand_runtime.inl两处生产源变化。沿原时钟/原active_now结果记录最近16项：prepare过期、commit过期、commit非正时钟、外部input_lost（无已存在时钟，明确0/未知）；不加输入取样或改变失效/返回/清理。实际完整Native编译，SHA 6ff4104439f4bc6faacd22b97e69503dd3d1065453592215672e816458a79829。恢复7c30b73a四份测试inl；其header与冻结仅差proximity方法，实际接口匹配且原回归及新增共2530检查0失败。早期新测试遗漏原Pending RAII abort已修正，拒绝后不伪装自动收尾。

私有wrapper新增显式--native-input-loss，仅该实例manifest替换并验SHA；不带该flag仍原806dcbd6。PG继续extraction-observe-r1，未选用WAIT实验；publicv71及固定手动入口未改。下一步保存后一次短loaded实测实际失效阶段，完整13项未完成。


## 2026-11-11 game22812：六连发通过，卡扣SetPosition请求失败后取消

Native6ff只读分类与PG extraction-observe-r1实际运行，6条一致射线。卡扣1→2、angle0.67082526时seek_latch_angle返回false，cleanup535；368后续source withdrawal104.1648ms。原cleanup pending是transaction.pending，不是seek_pending，故不冒称已确认未派发。Native总记录40，保留16项皆external/frame0/clock0未知；这是有界窗口，不能推断全部40分类或当作43944复现。原capture/session-status独立SHA归档。正常stop/launcher/Native/PG/XR，无FG/CLOAK违规。353帧无capture错误/编码0，3页48组paired概览亲审，视觉仍未接受。

新latch-seek-gap-r1只对同一active/stopped动画inventory+serial、seek_pending=false且原received时间已过100ms、时钟单调的失败，恢复此前纯stroke/history并等待新鲜采样；不写过期姿态，不重放旧setter，不放宽轨迹/时效/授权，已派发/未知继续cleanup。12项实际helper/caller block边界检查及完整PG编译通过，SHA5bce08627e0a752c2293f6b96c6506ad90ac5d866fb0c569d1714068f6ac114e；尚未游戏验证。原匣/新匣及全13项目标保留，WAIT实验未选用。


## 2026-11-11 game50828：当前候选短loaded换弹与回射通过

PG5bce0862/Native6ff41044，新132753实际肩取/core6、原卡扣打开、原匣抽出、新匣获取/入座、关扣、return6及pause/B通过。typed72/78→78/72→72/72，12条有效且来源一致射线。原抽出10次observe全部接受，最后49.0108508mm/complete1，最后gap72.789ms。seek_unavailable及pose_gap_incumbent均0，不声称新修复分支已实练或先前失效已修好。全空仓/原匣保留/四态/双手/完美与取消/中断/视觉/头显仍待验证。

operator/stop/launcher/Native/PG/XR均0，无FG/CLOAK违规，无强制终止。562帧无capture错误/编码0，3页48组paired概览亲审，近脸遮挡/重影/手分离/引导小字继续未接受；机械通过不代表视觉通过。下一步当前同构成完整空仓，不重复短路径取代全目标。
