# 双手 GVE6 到原版 IK 的有限试验候选

本记录属于 `codex/weapon-6dof` 独立开发分支。前一阶段的小幅合成手骨目标、原版近战交接和精确恢复已有购买版记录，见 [逐帧 IK 试验](hand-ik-stream-trial-20261023.md)。这里新增实际 GVE6 grip 世界姿态的消费路径。购买版后台两局已用模拟器、真实 OpenXR 客户端与 Native 串通双手数据和手骨写入／恢复；整局图像链未通过，不作为物理手柄或头显验收。

## source 0购买版联合结果h（2026-10-23）

新的GHW1来源0候选已完成一次后台购买版测试：owner39432／game23528／XR44324，PG `fcaeca76…5e44e6d`，manifest `19af0b18…4b900ebd`。只在独立h目录准备和运行，没有改写f／g原始证据。运行前等待另一游戏／XR测试自然退出，实际入口重新核对空闲状态；Mute1／Behind1／ObserveOnly1、实体后台输入关闭，全程没有移动、放大或激活窗口。准备阶段Popen0，正式运行使用已核对的prep SHA `1564f4488db546e3893c74e459e339765802ae5c0b63ab368bb0428f30bd7a44`。独立预检不是缓存通行证：本局start_capture未传旧proof参数，仍走同次run_suite后才激活Native。

组件24项检查及运行后独立复算全部通过：25个written lease均有跨网格Tick样本和对应恢复，50个双手样本的位置残差最大 `3.910064697265625e-5` 原始单位，完整旋转角差最大 `0.0213450333°`。未放宽0.1原始单位／0.1°阈值。收枪阶段在绑定后改变两手位置及旋转，lease8–15提供8个新响应样本，左右手的位置和完整旋转运动门均通过；取回武器后lease16建立新绑定，lease16–24有9个新样本。四次唯一装备回执为11→10→11→10，Shutdown0；25次恢复、原8控制器全部字段与44字节附件、CallFunction32／ProcessEvent64两原入口的独立字节核对通过。末次Shutdown前证明收枪journal dirty；仍没有原子证据证明两个journal在Shutdown入口同时dirty。

25张pose证书关联本局Native activation `3130411681690` 和实际XR PID44324／creation `134371865867405158`；Native新通道matched104／missing0／published98／rejected6，XR新通道source_samples1126／published1126／skipped0／tracker_rejected0。6次Native rejection包含去重语义，不能归因到具体lease。stream／reference／双手loss epoch本局均为1，没有运行失追或参考变更边界的覆盖。written处GHW1 receipt年龄为47–79ms；该年龄不是两个源NS在最终提交瞬间的年龄，未独立记录commit_ns，相关coverage仍false。source0只表示wire profile，实际命令仍是模拟器姿态输入，不能称真实手柄跟踪。

静音守护核对9个专属会话；它们已经静音，所以muted写入0、restore_attempted0、read_failed0、mute_failed0、pending0。清理顺序为PG恢复和关闭→Native drain／XR退出→确切游戏退出→Guard关闭，Guard exit0。运行后独立CIM没有发现这三个已知owner／game／XR PID。整局原始结论继续为 `passed=false / whole_session_passed=false`：实际输出仍为8×8，抽样的后处理源纹理为3840×2160，frame-link未关联、GPU capture_attempts=0、XR没有图像layer／copy并exit1；Native自身cleanup通过，但其五项DXGI入口磁盘verifier false原样保留。PG两原入口恢复与Native verifier为不同证据，不能相互代替。没有进行黑输出窗口恢复或把数据层结果写成可见双手／头显验收。

私有不可改写证据位于 `analysis/hand-equipment-1023h/`：receipt SHA256 `c5a2d0ec184e22338301a247baeae2180fb5b35178f099df9d19f4ec1792c141`，PG log `365b552ee9b8f967040a70ea62d672a419472c968ac8c4b57be31f00cdc79b2f`，component audit `38524b14a23ad1f35f7ab2168d85c79b4ca0b16139fdd656b69ceddd1689d9c3`；独立复算和进程退出摘要为 `verification-summary.json`，SHA256 `eea5888148020b2df77d905188382c8c88d80c1d6f03568a349a7c620661e546`。Astra对新增GHW1身份、统计及工件来源门的只读复核未发现确定误判问题。此局支持固定测试场15秒／256上限内的连续双手与装备往返组件；永久运行、跨世界、正式动画所有权、掌心标定及最终渲染仍待实现或验收。以下保留运行前设计、预检与历史记录。

## 接线与临时绑定

### 固定实例持续会话源码候选（2026-10-23，CPU验证）

`GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL` 默认OFF，须certified＋combined。它不新增骨骼writer、不改304-byte Start或原导出，也不允许同一次模块生命周期重复Start；仅把原15秒／256次的录制寿命与固定实例会话寿命分开。持续循环仍在每次原Tick前恢复，原动作先撤Plan、清绑定再恢复，冻结对象／武器／世界／Native身份变化即结束，不自动跨实例接回。公开FreeHandControl的ownership／apply门保持关闭。

每次授权仍有有限的 `write_until = min(ticket.created_ms + 100, plan.context.observed_ms + 100, current_context.observed_ms + 100)`；任一加法溢出拒绝。它不来自now续期，有限模式仍传原trial.deadline。helper授权前后两次截止检查、MS／NS倒退、三份Context／ticket年龄、双source NS、最后Stop／Shutdown均无条件保留；首Tick arm_deadline仍有效。停止完整诊断后，快照、原装备三次认证、最后authorize、journal获取及恢复照常执行。lease／动作epoch耗尽会终止；统计饱和；持续模式的16-bit Status lease字段钳到65535，不回绕。

完整pose／bone只在前15秒／256次诊断预留内输出；`recorded_leases`含可能的planned-only预留，不能当成实际写入数。slot／action／equipment／restore等热事件另限1024；stow Backend.read只加同一日志门，原读取、expected认证和返回值不变。终态及固定大小timing块保留一次，日志FILE和ferror处理未被伪造或关闭。

连续ON、联合OFF和普通默认版构建均通过；root独立核对7份实际源码、三份DLL及PE导出，ON／OFF导出相同，默认版没有手部／stow私有导出。机械提取真实提交helper的旧有限31场景、持续21场景，以及真实yield／phase资格块4场景通过；这些调度检查stub掉Policy授权，不能冒充Policy或实际游戏验收。原Policy25／certified12及Start／Lease／刚体绑定检查另行通过；新Session CPU及缺certified／combined两拒配通过。Astra复核发现装备phase变化路径中，epoch耗尽的clean-but-ended可被后续资格失败写回等待状态；现已在yield返回后立即检查ended并返回，4场景覆盖终态7、dirty故障8及正常交还，Astra再次复核闭合。

私有验证报告为 `analysis/hand-continuous-session-20261023/report.json`，SHA256 `27be235f1b603dee79f8139367fa87649cae37c065396e3d107bd1198a361e6c`；ON DLL `172f215325a916a007d01635b1592a64648c38de57c3f7e73ca681dee6937ec3`，联合OFF `31076d2e02251850f567712ecf3082015ed146f83ea68d6c079465e5e3438f34`，默认 `059746ddf5200fc9d2bf35a52fdb9b1edefd6b131b92b47d0193cfe1f344af7e`。报告自身SHA只在外部记录，不放入自身hash表；14份记录hash和源码／产物独立回读通过。旧h及旧CPU报告不改写。本候选尚未运行新游戏局；h的有限组件通过不能代替越过录制预算后的实际持续运行、可见手部或头显验收。

### 后续 GHW1 连续性候选（2026-10-23，默认关闭）

运行前更正：当前连续host的wire profile固定为runtime source 0，并拒绝simulator-operator模式；`GEARS_PLAYGROUND_VR_R2_SIM_TEST` 是另一路可选调试入口，不能据此选择本通道来源。提交 `911c172` 将PG初始化固定为 `StereoSource::RuntimeTracked`；真实初始化块提取回归在该宏为1时验证source 0 baseline→binding和source 1拒绝。来源0只描述wire profile，不能据此称为物理跟踪。新的PG三种构建、11项组件检查、4项配置拒绝、31项真实提交helper场景与两项删guard变异检查通过。737份源码与新PG已配套封存为 `analysis/hand-continuity-candidates/222a9b2f5538a42777003485c6c6d9289be7bd4a74e2efdd69d011204467bae2/`，基线 `911c172`，manifest SHA256 `19af0b187e5cdcf4984e87f144aee5d0293d2ae71a43b5215e5953ed4b900ebd`；PG SHA256 `fcaeca76e1e680d0620ed7398936f5627f7a473f4419188b79a60eecc5e44e6d`，Native／XR分别保持 `ecd30c01…`／`886362d5…`。旧b28封存不改写，后续h必须使用新source 0封存。

独立启动链预检在 `analysis/hand-certified-startup-preflight-20261023/` 完成22/22项：17项CPU／ABI／状态，5项独立DXGI／D3D12检查；44份stdout／stderr已记录hash，没有启动游戏或XR。FrameLink测试窗口不带WS_VISIBLE，采用隐藏进程和Mute1／Behind1／ObserveOnly1。该记录只验证给定Native helpers；实际连续owner仍须执行同次22项检查。新source 0封存本身仍标记runtime_tested=false，h尚待准备审阅和一次性实际执行，不能先宣称连续手部运动或整局图像通过。私有构建／回归回执分别为 `analysis/hand-certified-source0-build-checks-20261023.json`、`analysis/hand-certified-commit-guard-source0-20261023/report.json`、`source0-init-report.json` 和 `analysis/hand-certified-source0-frozen-20261023.json`。

f／g 实测表明旧绑定有时在最早入口已经超过100ms，单纯前移读取无法解决重绑。本候选把 grip 到手骨的刚体校准与单次写入许可分开：历史采样过龄或本次没有新包，可以保留**不可写**的几何 mount；只有新的、完整认证的票据和当前游戏状态，才可能生成本次 Plan。每次观察仍撤销旧 Plan，原 PlayerTick 前仍先恢复全部 dirty journal。旧 GVE6 路径、原25场景和现有100ms写入门继续保留。

新增 `ControllerContinuity.v1`（GHC1）是与 GVC3 并存的独立 ring 8 通道。每个 XR publisher 生命周期只有一个 SourceTracker，对每张采样先更新再尝试发布；两手 loss epoch 独立累计。因此即使中间的3→0→3未被 Native 消费，恢复帧仍能暴露那次失追。完整相同的最后合法帧可幂等重读，改内容的重复帧、倒退或计数溢出均拒绝；mapping 重开和发布失败不能重置来源历史。XR 仍按新通道→原 GVC3→eyes 的顺序发布，不改原 CSV。

Native 的 StereoGate 另保存单调 reference continuity epoch。reset、回正、LOCAL origin 变化、实际参考建立，以及已观察到的非过龄失败都会留下边界；重复失败只锁存一次中断。纯过龄保留参考证书，过龄同时伴随坏几何或不同原点则不能隐藏该边界，原 StereoStatus 返回顺序保持。Native 在原 stereo_mutex 内、同票据 L3 处理之后冻结实际 gate 状态与 `result.reference/reference_generation`；仅 stable、无 pending、未耗尽的参考可发布候选。

`WorldHandContinuity.v1`（GHW1）独立提交世界姿态与完整证书，自行分配 serial／created_ms；不把旧 GVE6 调用方尚未赋值的局部 serial 拼回来。证书包含固定 Native activation QPC、实际 XR PID／creation、stream／reference／两手 tracking epoch、profile、source，以及原 Stereo 和 Controller 两个 received_ns。Native 必须将 GHC1 输入与本次 GVC3 输入逐字段比对，并核对两通道的真实 publisher 身份。发布持锁、读取复制后各自取得实时源时钟，两份源时间都必须有效且不超过100ms。同 publisher／source／session 的重复或倒退 sequence 不推进 GHW1 serial；Native 重复使用同一眼部采样、包括同票据 recenter，都等下一张真实源帧才发布，避免把合法重复渲染变成 PG 的重放失败。读取明确区分无新包、首次丢弃历史、有效新包和拒绝；已消费坏包不能等时钟追上后复活。证书只证明这张姿态使用的参考，不是 PG 写入瞬间的 Native 当前参考 getter。

PG 的 `GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL` 默认 OFF，必须显式启用联合试验，并排斥旧 transport 续龄候选。它固定 Start 的 Native activation，首次合法 XR publisher 后不换源；global／reference 边界清双方，tracking epoch 增加只清对应手。新包的认证失败、重放、epoch／源时间倒退、future／过龄会永久封闭本次 Policy，再沿原交还流程结束。最终 authorize 再验两份源时间，返回 mask 前重新检查 Stop、Shutdown、有限试验 deadline 及当前 Context／票据年龄。原三次装备认证、最多15秒／256次 Lease、逐帧恢复和原动画交还顺序保留。新模式不读取或配对旧 GVE6 shadow，日志明确标为 GHW1并记录证书，旧模式的诊断格式保留。公开 FreeHandControl 的 ownership／apply 条件没有因此放开。

离线验证已通过：XR ON／OFF构建和5项检查；Native ON构建和7项检查，OFF构建和7项检查、缺 observer 拒配；PG 新候选、关闭候选的联合版和普通默认版三种构建，11项组件检查和4种拒配。旧25场景测试源码未改，新12组覆盖过龄保留、无票据暂停、隐藏单手失追、各类边界、重放／坏证书不能复活、两路源时间最终过龄、Plan逐字段篡改和模式隔离。忽略目录回执为 `analysis/controller-continuity-20261023/verification.json`、`analysis/hand-certified-policy-20261023/verification.json`、`analysis/build-xr-hand-continuity-trial/build-validation.json`、`analysis/build-native-hand-continuity-default-off/validation-summary.json` 和 `analysis/hand-certified-build-checks-20261023.json`。机械提取实际提交helper另有31例CPU回归全部通过；删除最终guard的变异出现17个反例，仅删NS回退检查的变异出现1个反例，这些是测试变异，不是历史生产版本。对应回执为 `analysis/hand-certified-commit-guard-20261023/report.json`。Native 原有 shadow／窄化告警另列，没有把它们写成新代码无告警。

对应代码分两项提交保存：`7ddb0f3` 为来源／参考证书通道，`c5262b3` 为认证校准保留与PG接线。737份项目源码、PG／Native／XR三份候选、loader、所需Native helpers及检查回执已封存在忽略目录 `analysis/hand-continuity-candidates/b28d12a80495ef15683199c6fd6c52b5627902ce2b9f726c2d968e95e4d3ffe0/`；源码基点为 `c5262b3`，manifest SHA256 `7d1714efd92ceebec200ee9f4a0ca45670609452c6f4958e586ef441818b3e70`。PG DLL为 `6004449e…8fdcfe2`、Native为 `ecd30c01…79c262a`、XR为 `886362d5…9c9032c`，三者均为新私有产物。8个 reserve／continuous 启动helpers只完成构建与哈希记录，尚未执行完整启动自检。

此候选尚未运行购买版游戏，也尚未替换冻结默认 Native／XR／loader。f／g 的持续运动失败、独立 Native entry verifier 与图像链失败回执保持原样；下一步使用这套配套产物和新的一次性运行目录，先完成启动链预检，再进行静音、后台、只读窗口观察的实际联合测试。

`GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL` 默认关闭，须同时启用原逐帧试验基础和两个原动画观察边界。它替换合成轨迹循环，以 `PlaygroundHandIkXrStreamTrialStart` 接收一次性启动参数；该构建不导出合成轨迹 Start。Stop／Status 和原有恢复、关闭协议复用。

每次原 `PlayerTick` 前先恢复上一帧的全部 dirty journal，原 Tick 与桥接请求在恢复后运行。post Tick 只接受本次成功读取的新 GVE6 帧；没有新帧不复用缓存姿态。四个尾控件可按手选择：左手使用通道 2／3，右手使用通道 0／1；选中通道先全部快照，再写入并回读，未选中通道不参与 Lease 的读写。原四个头控件不写，也不强制动画求值。

`GripHandBinding` 用当前手骨世界矩阵建立临时 grip 局部平移及完整旋转。之后按当前 grip 的刚体变换求出手骨目标，不能用欧拉角逐项相减或固定世界偏移代替。绑定拒绝非有限值、非仿射矩阵、镜像、退化、明显缩放与剪切；仅清理通过刚性检查的浮点噪声。它用于本次有限接线试验，不代替控制器到掌心的实际标定。

独立 `hand_stream_ticket_policy.h` 保存当前计划与每手绑定许可。身份、世界代次、XR session／space／input epoch、单位比例或动作 epoch 改变，以及重新变成可接管状态，都先封存当前帧作 baseline，再等更新帧。serial 与 sequence 在同流中分别严格递增，拒绝帧也消耗游标。3→1 只清右手绑定，左手可继续；短缺帧不写但可保留几何绑定，超过 100 ms 则必须重新 baseline。提交绑定与实际 prewrite 许可分开；所有日志刷新后再次检查时间、动作、Stop 和试验期限，日志阻塞不能使旧票据进入写入。

## 固定测试场启动约束

启动参数使用私有 304 字节 POD：游戏 PID／创建时间、外部冻结 Native epoch／启动 QPC／状态采样 QPC、短启动期限、WorldInfo／Level／PC／Pawn／Mesh／Weapon 的完整对象键与原世界时间。远程 Start 只复制、检查格式和新鲜度，首个游戏 Tick 才核验引擎对象。拒绝的第二次 Start 不得覆盖已经排队的第一次参数。

Native 的 `session-status.txt world_generation` 实际来自 `frame_link.world_epoch()`，并非同步的 `committed_world_generation` getter。此候选只允许**新 Native session、从未 prepare／handoff 的固定测试场**：外部 runner 必须在完整对象快照前后读取同一启动身份的新鲜 running／complete 状态，实际读取 epoch 1，并确认 prepare、candidate、handoff phase／commits 均为零。缺失字段不能填默认值，也不能拿 GVE6 自己的 generation 填 expected。

首 Tick 和 prewrite 对照完整对象 header、Level／WorldInfo 关系及不倒退的世界时间；新票据 generation 必须等于冻结值。失败永久停止本次试验，不自动接受新世界。已有 dirty journal 的恢复只依赖原控制链及 owned 值，不会因新的世界校验失败而被跳过。试验最多 256 个 lease／15 秒，写入预算耗尽后原动作不能重新打开预算。

这是有界实验约束，不提供持续跨世界同步屏障；外部 status 通常按秒发布，对全部对象元数据完全复用且 Native 尚未 rebind 的 ABA 也没有证明。生产使用前仍需处理持续生命周期、空手／收枪、逐枪动作及最终可见性。

Native 私有候选保留 `FREE_HAND_POSE_OBSERVER` 和 `CROSSHAIR_ASSIGNMENT_OBSERVER`，关闭 `UI_CALL_BOUNDARY_OBSERVER`，运行时仍启用 UI／anchor 认证。这样 Playground 独占 CallFunction，Native 从其既有 UI execute TickScope 发布 GVE6，无需新增同入口 Hook 所有者。停止顺序为先恢复并关闭 Playground、独立核验两入口，再关闭 Native／XR 并核对退出回执；本轮已按此顺序执行，图像验收失败仍单独保留。

## 购买版模拟器数据记录

私有候选 DLL SHA256 为 `9c3676e3b1cdab0df6663f3d001fc0077078dbe18be73946970862ad6c869f66`。两局都只操作精确自有 PID／创建时间的购买版副本，私有配置、后台输入关闭、静音保护及窗口置后启用；`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1` 保持只读，没有放大、移动或切前台恢复窗口。四条私有模拟器命令改变左右手位置／yaw／pitch，没有 trigger／Grip 按压；该 runtime 没有 tracking-loss 命令，因此真实失追仍没有运行证据。

| 自有 PID／回执 | 手骨与恢复观察 | 数值与限制 |
| --- | --- | --- |
| 40872／`analysis/hand-xr-1023c/` | 43 个完整 lease、129 份矩阵样本、43 次恢复；原近战入口在 dirty 时同步交还一次，动作退出后接到新票据；终态 dirty／fault 为零 | 未启用第一人称，临时 grip 到手骨平移约 134–163 原始单位。86 个跨网格 Tick 手样本中 85 个位置残差小于 `6.87e-5`，一次较大左手目标残差约 `9.76075`；完整旋转角差最大 `0.02537°`。不把该局说成所有位置精确跟随。 |
| 53564／`analysis/hand-xr-1023d/` | 43 个完整 lease、43 次恢复，近战期间交还并在退出后新绑定；本次桥接请求发生于已干净的 Tick 边界，action_yields=0 | 启用并实际切到第一人称，Native 记录 applied=173。86 个跨网格 Tick 手样本的位置残差最大 `6.57e-5`，完整旋转角差最大 `0.02348°`；written 快照的 GVE6 receipt 年龄为 31–47 ms。另有一个仅记录 planned_mask、在实际写前收到 Stop 的尝试，不能计入已应用 lease。 |

第二局与第一局同时改变第一人称／普通相机认证配置，不能用两局直接证明唯一因果关系。临时绑定尚非真实掌心标定；它在第一人称局仍有明显几何偏移，不能直接作为正式武器握点。两局都观察到原近战调用与槽播放，动作后有更新票据驱动，PlaygroundShutdown=0，CallFunction32／ProcessEvent64 原入口独立匹配，自有游戏实际退出。静音守护各核对九个本来已静音的专属会话，修改与待恢复均为零，读写无错，恢复回执为 true。

两份原始整局 receipt 均保留 `passed=false`：源场景为 1280×720，实际绑定／Present backbuffer 为 8×8，frame-link 比较拒绝，GPU capture_attempts=0，XR 没有图像 projection／copy，退出码 1。没有为此违反窗口只读规则。Native 自身关闭协议通过，但原独立 verifier 对五项 DXGI 入口给出磁盘不匹配；对照同 PID／创建时间／owner 的本局 manifest，这五项关闭后独立 hash 均等于安装前 original、不同于 Native patched，原有 E9 跳转归属未知。原 verifier 未接入完整安装前基准，保留原 `all_match=false`，不把离线对照改成整局通过。

原始日志、failed receipt、组件复算和安装前字节对照都只保存在上述 ignored 目录；第二局组件复算为 `component-audit.json`。前两次启动准备分别因私有工作树缺少静音 helper 路径、私有设置恢复提示阻塞图形初始化而停止，未启动手骨 writer，也未作为通过证据；自有进程均已另核实退出。后续运行器在启动前验证静音 helper，并在 Guard 覆盖下处理私有恢复提示。

## 当前检查

VS18／MSVC 19.51 Release 的 XR、原合成与默认禁用 Playground 构建通过。四项新／扩展检查在 `/W4 /WX /EHsc` 下通过，断言保持开启：

- 完整刚体绑定、旋转带动局部偏移、左右独立、非法矩阵与不可用输出。
- 12 组姿态计划边界，包括旧计划、重放、单手失追／恢复、100／101 ms、无中间观察的长缺帧、身份／动作／流／比例变化。
- Lease 按手选择、未选择的原版／foreign 值保持不读不写、部分写入失败和精确恢复；原四通道检查继续通过。
- 私有 Start 的进程身份、ABI、外部 epoch、状态采样／启动时间边界和非法对象字段。

导出隔离已核对：默认禁用 DLL 没有手部试验／动画观察导出，XR 候选没有合成 Start，合成候选没有 XR Start；缺少原逐帧基础时，XR 构建在 CMake 配置阶段拒绝。Astra Max 复核了重复 Start、日志后的最后授权、按手通道与恢复顺序，并区分了 GPU 断链和 DXGI 安装前基准；本轮子任务不启用 Fast。公开 `FreeHandControl` 的 ownership／eligible／apply 门及 GVE6 ABI 没有改变；普通启动器、冻结 Native／XR 和发布包未替换。

## 与身体槽暂收的联合候选

新增默认关闭的 `GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL`，必须同时启用 XR handstream 与暂收候选，才允许这两套原本互斥的功能接合，其他私有枪体／弹匣／装填／电锯 writer 仍禁配。原 Tick 前手骨恢复顺序保持；收枪助手执行前再次确认手部 journal 已清，Shutdown 一开始同时封住两方的新写入。yielding、pending 和实际助手调用期间阻止新手部计划，并废弃旧绑定。

稳定的暂收阶段可以作为有限共存验证的目标：枪仍是同一当前 Weapon，暂收 lease 必须 dirty、未 fault／revoked、未停止／过期，完整对象键一致且实时 attachment 仍 detached，再叠加原手链元数据、原强度／目标、Active／quiet 和新鲜 GVE6 绑定。不能凭 detached 或 status5 取得手骨许可，也不应要求已收枪的武器仍在原手挂点。Astra Max 复核认为附件与手部尾控件没有直接写入冲突；先前暂收记录中的八控制器稳定支持开展有界实验，但尚未证明 detached 时实际双手骨输出响应。

联合候选还须在暂收权限丢失／原动作交还时显式通知 handstream，先尝试恢复手部 journal 并清 binding；不能仅依赖 helper(false) 触发动作观察，因为 foreign／unknown 或已回原手的只读清除可能不调用助手，两套动作表也有差异。联合运行使用同一真实 XR 客户端发布手与输入，不与独立按钮测试发布器抢写同一映射。验收需要同一局取得助手前手骨 clean、稳定 owned-detached 时的双手目标／骨矩阵响应、取回后的新帧重新绑定，以及两个 journal 和原入口的独立恢复证据；不能把旧的分项成功拼接为共存通过。

上述显式交还已接入：`handstream::yield_for_equipment` 先递增动作代次、封存旧计划并清双手 binding，再恢复自己的 dirty journal，不重开 trial 或 15 秒／256 次预算；`equipment_ready` 只表示手 journal clean／非 fault，不因 Stop 阻止装备恢复收尾。原 hand／owned-detached 阶段切换会先消耗 blocked baseline，再经过原稳定门和更新票据重新绑定；plan、prewrite 与日志 flush 后均重验装备阶段，资格鉴别结束后才取最新时间作 100ms 授权。

发现 foreign／unknown 时，先永久 revoke 暂收写权、封停止，再做可重入的手部交还。已观察原 hand 恢复时则进入 journal 的新 `relinquish`：先撤销旧写权，仅同 Key／精确原手读回才只读清 dirty；此后重新 detached 不能补 false。只有新的显式 acquire 在原手认证通过、建立新 Key 时才清旧 revoked；失败的 fault 不清。新增检查覆盖 relinquish 读回时重入 restore、正常只读清除后新 acquire 及错误不能复活，避免手部交还的引擎调用重新打开旧暂收权。

联合候选、默认关闭及原独立暂收构建均通过，VS18 x64 Release `/W4 /WX`；九项断言检查通过，缺 XR、缺显式联合开关以及并发枪体 writer 三个配置按预期拒绝。默认构建无手／暂收导出，独立暂收无手部导出，联合仅有 XR Start、没有合成 Start。Astra Max 最终只读复核未发现本次有限试验范围内剩余确定阻断；子任务未启用 Fast。

DLL SHA256 `159947f1f83e30da826cac43e000de829e626b8a1c6e453706fc604a2c97229d`，160 份源码、检查／配置和导出记录封存在忽略的 `analysis/hand-equipment-candidates/<SHA256>/`；source.json SHA256 `181c60fca9221294fc25c387a183749902327abebfca57eeb9329603e7345993`。对应源码提交 `e8b3cf5`，live 与保存源码的 160 项哈希均匹配。首个私有 runner 已准备，使用同一实际 XR 客户端和原已保存私有 Native／模拟器，先验证原手→稳定 owned-detached→取回重新绑定及联合停止，不启动独立按钮发布器、不启用 R2／L2，不延长原预算。既有 8×8 图像失败仍保留，联合组件成功也不能替代整局图像或头显验收。

运行器的 17 项离线检查和独立组件审计的 30 项自检通过；默认 prepare 在禁止所有 Popen 的环境中完成，实际创建进程为零。准备回执 SHA256 `a34c4706f89bca4357043f2fc9a52a4681dc19ff458fc1f15df2b0c807c31d96`，运行器 `c440a7534bdd9b1ed55387953c55cf66f9123ad764cfdf27e887ec6927b7db0d`，审计脚本 `a1784d0506e3402f0cd60c91001d64b4e0a5a7efa268084db5da9f0471294b24`，保存在忽略的 `analysis/hand-equipment-1023a/` 及对应审计脚本。准备固定 182 项依赖与 23 个实际加载模块的来源，旧 d／b／c 回执原字节不变。Astra Max 窄复核确认：结束 FPP 生效值接入硬门、失败 HandStop 不跳过唯一 Shutdown，以及清理异常不能跳过精确游戏退出与静音守护收尾。

后台子进程显式设置 `GEARSVR_TEST_MUTE_GAME=1`、`GEARSVR_TEST_WINDOWS_BEHIND=1` 和 `GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`，使用已核哈希的 J 盘音频 Guard。游戏关闭或诊断输出失败时保留 owner 与 Guard，只有独立证明精确自有游戏已退出后才恢复原静音状态。第一人称在 Start 前只记录切换事件与自然 Tick，结束后的本局 `applied>0` 为组件硬门；累计值不能归因于每一个 lease。Shutdown 前的顺序读数只证明 stow dirty 与已有新手部写入，不证明入口瞬间两个 journal 同时 dirty。关闭后的附件采用原始唯一 raw52／pose44 与身份锚点核对，不伪称再次调用 getter 或新读了完整对象头。

首次 `--run` 在最前只读 idle 检查发现其他会话的游戏／launcher／Guard，已拒绝启动；没有 run-started、私有实例、注入或本局游戏。**联合游戏尚未开始**，不将该前置拒绝当作组件试验失败，也不干预已有实例。待其自然退出后才能开展这唯一实际联合局。

已有实例自然退出后，实际 `hand-equipment-1023a` 购买版 PID 46048 在双手 Start 前停止：运行器错误要求八个控制器强度均为 `(+0,+0,+0)`，而冻结 C++ `handik::baseline_strengths` 明确要求索引 0／1／4／5 为 `(+1,+1,+0)`、2／3／6／7 为 `(+0,+0,+0)`。真实八组位模式完全满足后者，故这次是私有运行器基线错误，尚无手部／暂收联合交互结果；没有发出 Start、槽请求或启动 Native／XR session。原失败 receipt SHA256 `d845dbf907a04d4891d6e7927f96e4ac6e81314602926543bbd3551ed1399fad`、组件 audit `77ec7fdabada2364cd893e231fd24820fe6c95a1841b59a999f1a9659939c268` 保留。

该局 Shutdown=0，八控制器全字段与原 44 字节附件精确恢复，CallFunction32／ProcessEvent64 独立原字节匹配，精确游戏退出；Guard 匹配 9 个会话、mute／restore 写入均为零，read_failed／mute_failed／restore_pending 均为零，关闭顺序在 game_absent 之后。原日志 SHA256 `eae566703e5a677d1bbf7c9e875f26273ccc9cb60249decd414e79e1b461f10c`。仅在新的 `analysis/hand-equipment-1023b/` 准备严格对齐冻结原生位模式的运行器／审计修正，不改候选、旧脚本或旧回执，不降低身份／动作／新鲜度门限。

严格基线修正后的运行器 20 检查／审计 38 自检及 Popen0 prepare 通过。实际 b 局 PID 47248 的 Start=0、槽确认 `11→10`，取得原手阶段 4 次、owned-detached 阶段 4 次新票据写入与跨 Tick 采样，8 次全部精确恢复。16 手样本对写入目标的位置残差最大 `1.33515×10⁻⁵` 原始单位、完整旋转最大 `0.026602°`；本局累计 FPP applied=63，四条模拟器 controller pose 命令已确认。**这些不证明连续跟手成功**：8 次写入的 `bind_mask` 均为 3，移动后的新 grip 被重复绑定到当时自然手骨，未取得同一绑定内至少八次移动响应，也未走到显式取回／最终 dirty Shutdown。原 receipt 仍为失败，SHA256 `95a42eb4ae9b643d7f25c8dffe405bed437f8ccd95ee03e1c8b30108f13e18fa`；audit `00dc6268695c6124be7383bfa22c0c4b8532c633a272280311cace29f83c2665`，日志 `07f8a187560ae65dbce6df746b4667e3b2e1e921dd0d7f6c7112ebaddabc7088`。

b 局的 host 在 phase2 等待中因 Native 新鲜度复合检查停止；失败时的 begin／stamp／now／frequency 未保存，不能唯一归因于超龄或时间顺序。保存的 Native telemetry 两个相邻 stamp 间隔分别约 988.9962／1010.2447 ms；普通源码写出周期为 1000 ms 加循环与原子发布开销，因此存在读到短暂超龄状态的可能，不据此追认失败瞬间。失败 HandStop=0、Shutdown=0，8 controls／44 附件／两原入口独立恢复，Native 干净关闭，game_absent 后 Guard 结束，9 个会话无写入、无 read／mute 错误、无待恢复。本局图像仍无 GPU publication、XR exit1，实际 frame-link 首条比较为预期 3840×2160／实际 8×8；Native 的独立入口 verifier 仍 false，不能用旧局的基准解释改写本局失败。

Astra Max 从 b 的 phase2 时序确认：lease5–7 的旧 receipt 在下一 pre_restore 已分别为 125／125／109 ms，后续 Policy 会按既有 100 ms 长间隔规则撤销绑定；不是每 Tick 的 observed_ms 被当成新身份，也没有最终 authorize 超龄后静默继续。一次 accepted 路径原有六次完整装备认证，其中三次藏在 `xr_context` 构造中。提交 `f8ec02f` 仅去掉这些隐式引擎查询，保留 observed／prewrite／日志 flush 后三个显式完整认证，以及 ready、dirty／fault、真实 snapshot 时间、epoch、Stop／deadline／最后 authorize；不修改 Policy 的 100 ms、15 秒或 256 次预算。严格构建、九检查、三拒配与导出隔离通过，新 DLL `64b965a97524b6d9e2ca798fa5c7a6536f257f1f148baf099a99ff957dca6798`、160 源码 manifest `5e15c2761b86c8e891705fa75916ebb0c6856a2fbb0019ecfe688567a1b2f40e` 已封存，**尚未运行新候选，不预先声称性能或跟手修复**。Native host 另准备详细失败数值和同预算内只对超龄的有界只读等候；每次仍须取得 ≤1 秒的新状态，未改冻结 Native／XR。

随后 c 运行器接入严格的 Native 状态等候：只有完整身份／session／epoch／零 prepare-handoff／时钟顺序与频率均通过、仅 age>1 秒时，才能在原剩余预算内最多 250 ms、每 10 ms 只读重取；成功仍须新 stamp 且 age≤1 秒。Start 的 expected session 取自已认证 continuous report，不从待验 raw 状态自当基准。模块十检查、运行器 23 检查、审计 38 自检及 Popen0 prepare 通过，准备回执 `89da32d30dabf12b58bb4b2232c9b5ba3516c5eda8b254adb8880757121e983c`。

c 实际购买版 PID38884 已走完整槽确认 `11→10→11→10`：原手至少 4 次、收枪后至少 8 次、取回后至少 8 次新跨 Tick 双手采样，最终 stow status `0x10005` 后直接联合 Shutdown=0；总计 23 次写入／23 次恢复，184 次 Setter 与目标操作，dirty／fault=0。四条 controller pose 命令确认，取回后的新票据／新绑定及助手前 hand_clean、phase0 无写入、两个 journal 收尾、八控制器全字段／44 附件／Call32／Event64 独立恢复等门均通过。本局累计 FPP applied=88；46 个手样本对已写目标的位置残差最大 `0.000246161` 原始单位，完整旋转最大 `0.026882°`。Shutdown 入点双方同时 dirty 仍为 unknown，不以顺序读数作证明。

**c 原组件 audit 仍为 false，唯一未过门为 phase2_motion_response。** 收枪阶段 lease6–14 全部再次 bind3，移动后 grip 又被吸收到新自然手骨，未证明同一绑定内连续八次移动响应；原手阶段已有部分 bind0，不能由此推称收枪跟手已修。原 receipt SHA256 `a43360483e8c032a9b17bf5b2e88298dd2db6f967771821aebb9d69ad175ea83`、audit `eec68346226072e8b2f597c10417b52187f4dd7f0a783266f682893e3c96c63c`、日志 `c5d9e217af92abe389df6fbf22fcdc6dd7b3dfa9de884e191153a006b95d66e4` 保留在 `analysis/hand-equipment-1023c/`。这支持完整联合交接／恢复流程，仍非连续运动或完整 6DoF 验收。

两次本局实际 Native 超龄记录分别为 1004.8584／1000.8158 ms，只读等候约 11.64／23.15 ms 后取得新 stamp，接受时年龄为 1.8264／11.7987 ms，未接受超龄状态或延长预算。Native 干净关闭但独立入口 verifier 仍 false，GPU publication=0、XR exit1，整局仍为 false；没有补 resize／前台恢复。精确 game_absent 后才关闭 Guard，9 个匹配会话无 mute／restore 写入、无 read／mute 错误或待恢复，全部自有进程已退出。后续须先测量剩余装备认证与收枪 tick 路径的成本，再决定如何保留完整认证并减少开销；不通过延长票据／绑定寿命绕过当前运动失败。

Astra Max 后续只读核对指出：c 的收枪 lease6–14 在 pre_restore 时旧票据已为 93–94 ms，余量约 6–7 ms。静态路径中的 dirty stow tick 加三次完整 phase 认证含 7 次 read_state／3 次 equipment snapshot，合计 40 次 GetWeaponInSlot；这是调用数量，不是成本测量。前置 stow tick 在原 PlayerTick 之前，phase 认证在其后，不能将前者缓存为后者的资格。下一步应仅增加按 phase 的内存聚合 QPC 计时与撤权原因计数：pre_restore→observe、stow tick、原 PlayerTick、三个 phase 认证及其中 read_state／snapshot 分段，终端统一输出，不在关键路径逐条日志／fflush；测到热点后才做单次认证内部的数据去重，末尾独立读回、idle／Active、epoch 与日志后认证保留。

计时实现已接入默认关闭的联合候选：15 个 inclusive span 按进入时已知 phase 记录 count／sum／max QPC，包括上述路径、手部恢复／snapshot／quiet、写前证据 flush、lease acquire 及写后 snapshot／written flush。嵌套 span 不能相加当作总耗时或 FPS；pre_restore→下一常规 observe 的区间可在 Stop 时保留 pending，不补造完成值。终端由游戏线程在所有计时 scope 退出后只输出一次，输出完才递减外层 callback，保持 Shutdown 等待与原恢复顺序。Policy 新增只读 last_observe 诊断，记录实际经过分支的手别 mask、旧票据／当前及前一 Context／新票据年龄和 future 标识；no_receipt／not_newer 可以只是当前 lease 交还，原因可重叠，不统称为重绑次数。Plan、current／authorize、游标消费、created_ms 续龄、100ms／15s／256 及所有装备认证门均保持原语义。12 组 Policy 边界场景扩展断言、联合／独立 XR／合成／独立暂收／默认构建、九检查与三拒配通过。此时只完成诊断接线，尚无新候选实际成本或运动修复结论。

上述诊断提交 `6467b1d`，封存 DLL `ccda4103695deb1afeff194a644b480dee06f2db9ea1ff24cceace48f5aeaca0`、160 源码 manifest `3ea8c42b84f3e829b4789c94a55d3a2336125e21d05e61bed27377da6cee64ca`。Astra Max 补复核了写后证据计时、游戏线程／重入 dump 与 Policy 原授权语义，未见有限范围内阻断。新 d runner 只替 c 的目录／候选／manifest 常量，audit 只替两个工件常量；23 runner／38 audit 自检及 Popen0 prepare 通过，186 依赖／24 实际模块固定，25 项 a／b／c 历史文件哈希保持。prep SHA256 `3f9b9a89818c95763a64408e4c886f673e3df8bdda7f5b5dc7d5b10dbddfd97d`，runner `a917ad2599618b691de81043d03ab84ba8e7c9ea544b159c9022efbd036a0e11`，独立 timing audit `8b6cb9c9dd61d4f99371eefaa7185797466928cdde1a87c731d7397b42c8846a` 的 36 项自检通过。

d 购买版 PID31544 的完整联合流程仍为 23 写／23 恢复、四个槽确认 `11→10→11→10`、最终 stow dirty 后 Shutdown=0；8 控件／44 附件／Call32／Event64 精确恢复，本局累计 FPP applied=101。组件唯一失败仍是 phase2_motion_response，未以计时成功代替跟手通过。独立计时块结构通过：QPC 10MHz，23 个恢复区间启动／22 个完成／1 个停止尾部 pending，invalid_clock=0，新增诊断热路径文本写入为零。收枪阶段的实际分项如下，均为进入时 phase2 的 inclusive elapsed time，样本数不同且包含嵌套，**不能相加推算整帧、FPS 或 CPU 使用率**。

| 分项 | 样本 | 平均 ms | 最大 ms |
| --- | ---: | ---: | ---: |
| pre_restore→下一常规 observe | 9 | 35.275 | 38.349 |
| 手部 restore | 10 | 9.806 | 10.202 |
| dirty stow tick | 36 | 2.149 | 2.451 |
| 原 PlayerTick | 36 | 8.241 | 9.334 |
| 首次手部 snapshot＋identity | 36 | 4.317 | 4.717 |
| observed 装备认证 | 36 | 7.106 | 7.747 |
| prewrite 装备认证 | 10 | 7.072 | 7.415 |
| postflush 装备认证 | 10 | 7.189 | 7.831 |
| 认证内首次 read_state | 56 | 1.973 | 2.300 |
| 认证内 equipment snapshot | 56 | 2.629 | 3.159 |
| 认证内末尾独立 read_state | 56 | 1.990 | 2.466 |
| 写前 sample／pose／flush | 10 | 0.085 | 0.145 |
| lease acquire | 10 | 6.234 | 6.545 |
| 写后 snapshot／written／flush | 10 | 3.441 | 3.894 |

phase2 的 binding_age 分支实际发生 9 次、mask 均为 both；左右旧票据各平均 126.667ms、最大 140ms，当次新票据平均 48.556ms、最大 62ms，当前 Context 最大 16ms。它直接支持旧 binding 的 source-age 撤销，不能将所有相互重叠的 Policy 原因相加当作重绑次数。写前证据 I/O 在本局不是主耗时；重复认证与恢复检查均有实际成本，尚未根据这些分项证明某一去重方案足以修复运动。

d 原 failed receipt `c867d64b2624ef9cc87014b49e7c323c5e81e511d84a5dac0c29808024b1034c`、组件 audit `54be97d8fabfcc17557da18d1d41ab6d23a141635ba8ee55f0931a2a94369681`、日志 `e05871b9a420aadd99e971bdec9655e86d85bff71c4a9396571dc0f75bb87900`、独立计时报告 `7772c81341a530bd28b503b81d0b071c70829db9eba99bb23aaa40017d3e6306` 保存在 ignored `analysis/hand-equipment-1023d/`。Native clean 但独立 entry verifier false、XR exit1／整局 false 继续保留，没有补窗口调整。精确 game_absent 后才关闭 Guard，9 个匹配会话无 mute／restore 写入、read／mute 错误或待恢复，音频收尾报告 `38853b46108139606bc95d66e62f5d263a7bb3bf057f02bbe2dce469e444024a`；自有游戏、Guard、XR 和 owner 均已退出。

Astra Max 对 d 的后续窄复核：phase2 的旧票据平均／最大超限约 26.7／40ms，单次认证内合法去重预计不足以单独填平；三个独立 equipment snapshot 约合 7.89ms，即便再把整个 dirty stow 检查 2.15ms 视为可省，也仍不足，且实际不能直接删这些资格检查。written→下一 pre_restore 的 QPC 间隔在 phase2 平均31.55ms、phase1平均19.91ms，说明部分时间在下一轮认证之前消耗，但不能唯一归因于渲染／调度／引擎。下一最小步骤是先在 pre_tick 的 restore 之前增加只读 shadow 观测：现有 GVE6 bridge 使用独立 cursor，记录此刻旧 binding／新票据年龄、严格新 serial／sequence、同 stream／owner／scale／action epoch，并与稍后实际 binding_age 撤销配对；不修改 Policy、writer cursor、Context 或几何绑定，不新增热路径文本。此时尚未实现或运行该 shadow。

只有早期 shadow 证明连续性仍在100ms内，才评估独立的 transport-liveness 接口及纯边界测试。它最多用真实早期时刻看到的新票据 created_ms 续已有 binding，不能救回已超过100ms的旧绑定，不能创建 binding、消费 baseline、生成 Plan 或设置 ready；后续 post_tick 必须重新读新票据并完成当前完整身份、三个装备认证和最终 authorize。原动作仍清绑定，禁止回填 observe 的 now、把旧 Context 标为新 clean，或以接收时间代替 created_ms。目前这些是待验证设计，不构成运动修复结论。

早期只读 shadow 已接线，尚未新局运行：Policy 新 const／按值 continuity_snapshot 仅返回历史 Context／ready、已消费游标、stream／scale、bound mask 和真实旧票据年龄，不更改 last_present、Plan 或任何成员。15 组纯场景验证查询不续龄、不消费游标、不撤销现有计划，含100／101ms、future、单手及返回副本修改隔离。pre_tick 在 restore 前用现有 bridge 的独立 shadow cursor 只读一次，writer cursor 不动；历史身份／action epoch、同 owner／stream／scale、新 serial＋sequence、早期旧票据与新票据均≤100ms才记潜在候选。下一常规 observe 后与实际 binding_age 分支配对，并按手排除 stream／scale／tracking 等其他实际撤权；只有中间状态／epoch／phase未变，且该早期新票据到晚期仍≤100ms，才标记 useful 观测，标记从不被 writer 使用。最多1024行保存在独立静态内存缓冲，停止后一次输出，容量／时钟／游标异常显式报告；不在热路径输出。联合／原独立XR／合成／独立stow／默认构建、九检查与三拒配通过，liveness续龄接口仍未实现。

该只读 shadow 提交63317ca，DLL `ad22fe3d63359a6ebb311aa8b6689c89e6ae800efd171940354360a01cbeff4e`、160源码 manifest `233992ce202bd836c9530463ac46a60b25623c90586970232e9dec1f4ea3fe7c` 封存。Astra Max 发现并修正了诊断假阳性：late observe 可能同时因 binding_age 与 stream_changed 等撤权，故新增逐手 reason bits／other_reset，不能将这种情况算为有用的提前读取。e runner／audit仅替d的目录／工件常量，23／38自检及Popen0 prepare通过，186依赖／24实际模块固定，83项abc／d历史文件不变；prep `ec2963cce789a4ca2642b36d7da19641349b55e0fb4d92649c2bd2bf54892afe`。

e实际购买版 PID35740：74条shadow均完成配对，73条读到新帧，容量／时钟／writer cursor变化均为0，独立shadow审计54自检及实际结构检查通过。phase2共36行，9次实际binding_age均是双方手：最早入口旧票据78–94ms、新票据0–16ms；到常规observe，旧票据125–141ms，而该早期新票据仍为46–63ms。全部9次same_state、candidate/useful=3、other_reset=0；phase1另7次实际超龄同样均有可用的早期新帧。这个证据支持评估独立transport-liveness接口，仍不证明续龄实现或同一绑定的移动响应。只读bridge读取的74段QPC合计0.1659ms，未测入整个shadow函数的成本，也不推算FPS。

本局23写／23恢复、四tap和最终dirty stow Shutdown0、8控件／44附件／两Hook独立恢复均通过，FPP累计99；组件仍仅phase2_motion_response失败。原failed receipt `0464570b41efb84f83a5b91392f946ff3fc88be62fde272e6f7fd07e2023be51`、组件audit `975adc4e8e02f831cc99bebf84594eec5ae0d1882995774e956485036b97c04d`、日志 `602b8e25407afdd3cf25451652c4850782d9fcfccd9eabd3952270bc3a87c115`、shadow报告 `ec76cc1749acfe08d1f62dd0d93297d718d3229544c87cca9cbbbb72818af5db`、timing报告 `11ae0e6ee0f598b8b77e78e837142f88c88da4d017d4114a236e62bb873350f2` 保存于ignored `analysis/hand-equipment-1023e/`。Shadow审计脚本SHA `bc2522f99bf679391793f64856ec2b2af97bbec4f96c3d7ccdfe9a9481b55eca`；旧timing脚本不改。Native clean但entry verifier false／XR exit1／整局false保留。精确game_absent后Guard结束，9会话无写入／错误／待恢复，音频报告 `9833e392bfa5fccffd2b9b42aad3a07c13400023e26651f3f1f31a969b5b4e44`；自有game、XR、Guard和owner39764均确认退出。

e 的证据后新增真正的提前连续性路径，开关 `GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL` 默认 OFF，且必须显式开启 combined。它在 pre_tick 的旧 journal 恢复之前用第三个独立 bridge cursor 读取，仍将实际 GetTickCount64 与票据自身 created_ms 分开使用。`Policy::refresh_bound_transport` 首先清 current Plan revision，并按实际时刻撤销旧年龄 101ms／future 的手、要求新 baseline；新帧只能给剩余 bound 且 tracked 手续存真实 created_ms，不能复活已过期绑定、创建新绑定或消费 baseline。历史 Context／ready／last_valid、正常 writer cursor 与 Policy serial／sequence 均不变。有效 tracking loss 只撤销缺失手；全局身份／动作／owner／world／stream／scale 或票据错误撤销全部。所有结果都继续原 dirty journal 恢复路径，停止／卸载、三次后置装备认证与最终独立 snapshot／authorize 均保留。

transport 与 regular 路径以 8 个不驱逐的 StreamKey fence 协调各自高水位。transport 不改 writer 高水位；已合法检查的完整同一 Ticket 可以由正常 writer 重新经过全部校验一次。身份／action／ready 变化清掉该例外，自然更新 observed_ms 不清例外；拒绝的 future／旧 tracking-good 等不能换路径再次利用，新 stream 被 regular 采用时继承已拒绝的水位，容量耗尽永久拒绝。API 未使用时原有 15 组场景保持，新增 10 组 transport 场景覆盖旧／新年龄独立的 100／101ms、旧 Plan authorize 和 commit 双拒绝、stamp 倒退、clock 回拨、单手 loss／恢复、跨路径／跨 stream 重放及容量边界；25 组全部通过。Astra Max 对核心和拒读终止顺序的最终只读复核无剩余确定阻断；尚待实跑，纯测试不等同于持续跟手验收。

新日志最多 1024 行内存记录，结束时在原 timing／shadow 块之后单独输出，含调用前后每手原始 stamp／年龄、实际 refresh／reset 掩码、三个游标／历史状态未变及 read／refresh 两段 QPC。容量满或计时错误不得跳过实际撤销和旧 journal 恢复。原 shadow 改为比较绝对源 stamp；真实续龄后 same_state 应变 false，shadow candidate／useful 不参与授权。独立 ignored 审计 `analysis/hand-equipment-transport-audit-20261023.py` SHA `76031af0127daecd8fcc5000737667436cf68458b80ac5e1fa257f3ec2fa7e0b` 的 75 项自检通过，并机械核对全部 44 个 row 字段；旧 e 只能报告 missing_measurement，不补造 transport 数据。该审计不证明完整 Ticket 身份、旧 Plan 双拒绝、完整写权限、FPS 或修复验收。严格新路径／关闭新路径联合构建、默认／独立 stow 隔离构建及九项组件检查／四种非法配置拒绝均通过；新游戏试验尚未开始，冻结 Native／XR 和普通启动器未替换。

Astra 的 runtime 复核还发现 bridge 会在推进独立 cursor 后因 future／过期／pose_valid 拒绝且不返回 Ticket；若不处理，late writer 可能重读同一张早期被拒的 future 票据。有限候选现对非首次 `!fresh && cursor_after > cursor_before` 立即置 Stop 和拒读终止标志，不伪造 Ticket 或提前 end；仍沿原 pre_tick 先恢复 dirty journal，成功后通过现有 yield 清 Plan／绑定，再以明确失败原因 end7。恢复失败继续原失败终止。首次 UINT64_MAX 建基、mutex 竞争／无新帧和成功读取不触发，5 个编译期断言覆盖分类；begin 记录 rejected_reads，独立审计必须为零。没有扩展 GVE6 ABI，也没有放宽 late writer 检查。

该实现提交 bf95959，DLL `d45654e35d7e529394497ccfbb1f254af5695edb38af755101d467ba990a5bf0`、160源码 manifest `a6689a34f562b996ee750416439155bf368fe6772faab6407532a642563703ea` 封存。f仅更换e runner／audit的路径和工件常量，23／38自检与显式Popen禁止的prepare通过，186依赖／24实际模块，150份历史文件不变；root独立重验字节变换、依赖、三项后台参数与Guard精确SHA后启动。prep `99635b90348275aa06824eb535a4fece2ead12a4c6c15e311cfc5c26ccb56411`，prepare-proof `162c89d4fed9c9ee7cd84d503e2e2b99082edd22f046c34be03097b076d40694`。原f manifest的runtime_tested=false保持，实际结果另存于本局证据。

f实际购买版PID56488：25次双手写入均跨mesh Tick并精确恢复，无planned-only；完整11→10→11→10、最终dirty stow Shutdown0、8控件／44附件／Call32与Event64恢复和FPP累计86通过。三份独立 timing／shadow／transport 审计有效，transport65行／64fresh／24called、零drop／invalid_clock／rejected_reads／writer或Policy游标变化；21次真实双手续龄使用原created_ms。phase1普通持枪与取回的14次调用全部续龄、无超龄；phase2的10次为7续龄与3次早期过期，row25／28／33旧109／110／109ms，新15／16／16ms。该三次按100ms先撤销绑定正确；新姿态在重绑后作为参考，尚无八个相对同一绑定参考的双手位置加旋转运动响应，原component仍仅phase2_motion_response失败，不能将25次目标命中称为持续跟手通过。phase2原pre_restore→observe平均34.203ms，恢复9.583、三处phase约6.95–7.03、dirty stow2.103ms均为各自inclusive样本，不相加推FPS。

原failed receipt `560457d544e6c236144560397f9d6b8c31e68a0f00028a0fc14e3f4be330dfc2`、component audit `6346e7ab69224fea219608fd12d41c6555662c00be72587393f216223a7d530c`、log `c78055fab04fc09ce7673572d28a7490a3603f37879b05a49ae99823b640a526`、timing `868902bdef76431bc235f44397dea1ec7206fc5a2a5f07303f03eb6c8299766a`、shadow `f245ea5322c5b0eed8dec2d8b35df2b7c676bf0022a283bfa61d13ac2f43decb`、transport `b33bd5f97eb23e5c29aa6997cd81a692ca0de4d7785ee309369f1209a924b938` 保存在ignored `analysis/hand-equipment-1023f/`。Native clean但entry verifier false／XR exit1／整局false保留，没有窗口调整。9个本来已静音会话没有mute／restore写入或read／mute错误、无待恢复；先确认精确game_absent再结束Guard。owner54684与game的创建身份和最终CIM只读检查另存，自有game／owner／Guard／XR全部退出。

Astra Max后续只读复核：现pre_restore→observe在transport之后才开始，完全未覆盖PE入口至pre_tick之前的action／stow／animation observers、筛选及反射查询，不能用现有聚合断言前移能追回此次余量。下一最小步骤仅增加同顶层ProcessEvent的entry now／QPC与const snapshot，按epoch、phase、bound和raw source stamp配对当前transport；另检查当前票据created_ms是否不晚于entry_now。只有入口旧age仍≤100ms、状态未变、票据当时已存在，才支持评估更早读取。此时该入口测量尚未实现；不提前续龄、不削资格或放宽100ms。

该只读入口测量现已接线：event_hook在Active之后、所有action／animation observers之前构造本地EquipmentEntryScope，TLS depth只选最外层ProcessEvent；先核已知GT，再核inside／enabled／automatic／trial状态、冻结controller与PlayerTick NameIndex，仅读取时钟与const continuity_snapshot，无引擎查询、bridge读取或Policy状态调用。本地const observation仅经同一次event_hook→pre_tick→refresh→entry_record同步传递，不使用跨Tick全局pending。1024条独立entry行与原transport索引一一对应，比较phase／epoch、历史Context／stream／游标／scale／ready／bound与raw stamp；无样本known0保留缺测且不报clock错误。计时错误或容量满仅记诊断，原read／refresh／restore、三认证与最终authorize位置条件不动。

entry块在transport_end之后独立输出，独立ignored审计 `analysis/hand-equipment-entry-audit-20261023.py` SHA `6181062f8cf8e27ff8ae5a83d14ac6c3b2131048e712e0631a4ff40902590ceb` 的65项自检通过；严格复用固定SHA的transport审计，并验证同count／同序join、时钟差、字段与same_state原始stamp。旧f仅返回missing_entry_measurement，不能补造。所谓age-only potential只表示：实际transport超龄手在入口旧年龄≤100ms、状态未变，且late源票据的created_ms不晚于入口、年龄仍合格；考虑GetTickCount64粒度与采样到发布延迟，这**不能证明该票据在入口已发布／可读**，也不排除first reason隐藏的其他拒绝，不授予Plan、写权限或修复验收。Astra最终静态复核无阻断，transport ON／OFF联合、独立XR、默认与独立stow构建通过；尚未运行新候选。

另记录Astra对后续策略的窄判断：GripHandBinding保存grip局部offset／rotation，数学上不因100ms自行失效；当前旧binding销毁与本次新pose写入资格是可分开的概念。若今后验证age-only suspend／retain，仍须在无新帧时零授权、恢复journal并废旧Plan；任何新写入须全新合格票据和全部现有当前门，动作／装备phase／身份／stream／scale／显式loss等继续清校准。当前GVE6只存最新pose，中间loss可能被覆盖，input_epoch未按单手tracking-loss递增，Native reference_generation也未进入现有StreamKey；同流加未见loss不能证明间隔无几何边界。此方向需独立有界策略和来源证据，当前候选不作该改动。

入口诊断提交182e7b0，DLL `c37e1e36d5c3f07109b0946e11f14b7fc2b009f2bd0be1b35e62c808644139f5`、160源码 manifest `69beec2ad02c9e36843e819c53edbec79e6e05ceda1527a2ae28ab04b60849d2` 封存。C++ begin6字段／row13字段与独立entry审计机械匹配。g只更换f的路径／工件常量，23／38自检、Popen0 prepare、186依赖／24实际模块及216份历史文件保留通过；root独立重验后唯一运行，prep `8b44e80c62d0d40138e605a9e59be80ffa099cbb8e9287e55bc721206b5d4bbe`，proof `d743d1aa1ba376992d85f9e16f06c47df8df41a6f2962e7918c0ba752f7d3e23`。

g实际购买版PID31604：25次双手写入全部跨mesh Tick且25次精确恢复，无planned-only；完整11→10→11→10、最终dirty stow Shutdown0、原8控件／44附件／Playground两入口恢复通过，FPP累计80。四份独立timing／shadow／transport／entry审计有效，entry66条全部known／same_state，零丢行／clock异常；入口至transport QPC平均0.0167727ms／最大0.0389ms，所有entry_ms与transport.now_ms相同。phase1的14次transport调用全部续龄，phase2的10次为4续龄与6过期；row23／25／28／31／34／37在入口已是109／110／109／109／109／125ms，新票据15–16ms，age-only potential全部0。**本局实测排除了仅把当前refresh前移到PE入口可补足此次年龄余量的假设。** 原组件仍仅phase2_motion_response失败，没有持续跟手／完整6DoF通过结论；不继续以该入口假设修改授权位置。

原failed receipt `978f271d128bbeda2df91bbe7b7686e734b28744ea66e4e1d3b1a49acb453d66`、component `7965b43f8765d6387af9b01b7001f75c834d03609256c06fb27f6632363cc567`、log `26839e56380051f41d0228dd3046e1852ddd2af97f1193b86a043e102b5406dd`、entry `a3f5a128e269da909092273c6ca2af53591d33b0255f48c695a4804baf834727`、transport `0cb300fadc6b6ae0802b2e2ac89b556679d02d6b5fa7e64250497f1d758fac61`、shadow `fdb0c8252832c3fd274a4d799d6e226c2021554f75fc25a7843c04cf804cb453`、timing `72e86ee19176576a9f2be7066b09aea9d96d635de322f2a85c75c1acf05dc826` 保存于ignored `analysis/hand-equipment-1023g/`。Native clean但独立entry verifier false／XR exit1／整局false保留，无窗口调整。9个本来已静音会话无mute／restore动作、read／mute错误或待恢复；精确game_absent后Guard结束，音频报告 `f6436bb3e275c03b97bf2e864cf03c4725fefb456fb3fbd57eb14bd9ddd0f11f`。game与owner24668创建身份及独立CIM退出检查已保存，自有game／owner／XR／Guard全部退出。

后续来源调查的明确缺口：StereoReferenceGate本体在不reset／不回卷的同一实例内递增，但reset会清零；现status的reference_generation是提交后更新的诊断镜像，既不能排除尚未提交的变化，也不能排除deferred回中，GVE6没有同步reference字段。XR CSV的left/right为左右眼，前240帧之后约每秒采样，tracked_pose_frames也仅为HMD；它没有grip tracking mask。Native的tracked_left/right计数只覆盖成功消费的输入样本，ControllerInputFrame及GVE6均latest-only且无单调grip-loss计数，3→0→3可漏读。因此，现有端点状态与CSV不能提升为age-only保留所需的区间证书；下一独立策略须补实际源边界或严格限定结论，当前Policy和100ms写资格不改。
