# 双手 GVE6 到原版 IK 的有限试验候选

本记录属于 `codex/weapon-6dof` 独立开发分支。前一阶段的小幅合成手骨目标、原版近战交接和精确恢复已有购买版记录，见 [逐帧 IK 试验](hand-ik-stream-trial-20261023.md)。这里新增实际 GVE6 grip 世界姿态的消费路径。购买版后台两局已用模拟器、真实 OpenXR 客户端与 Native 串通双手数据和手骨写入／恢复；整局图像链未通过，不作为物理手柄或头显验收。

## 接线与临时绑定

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
