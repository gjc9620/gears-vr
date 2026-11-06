# 武器与掩体离线代码预实现

目标：提前调研技术方案、排障、代码定位并落实可复用代码，让后续主线接入更顺利。沿用[前期总交接](../offline-tech-prep-20261028/README.md)，本目标尚未整体完成。

所有候选代码位于私有 `analysis/offline-preimplementation-20261028/staging/`，不是新的 Git checkout；生产 `tools/vr-common` 和 `tools/playground` 未应用这些改动。没有改主线 CMake、启动器、候选包、Git 索引或提交，没有运行游戏、XR、模拟器或测试可执行程序。

## 模型输入准备增量（尚未打入第六版）

哈希增量：新增严格 64 位十六进制 SHA256 解析和摘要比较，缺失值与合法全零摘要分开，拒绝错误长度／非法字符并保留原输出。Windows 适配器复用项目已有 BCrypt SHA256 路线，按块计算不可变输入快照，区分算法／创建／更新／完成／不匹配错误，正常及失败路径均释放自有 CNG 句柄。之后必须解析同一个快照，不得哈希后重新打开文件当作同一内容。解析器复合编译期检查通过，Windows API 调用代码编译通过，未链接或执行 BCrypt；这不等于真实文件 SHA 验证已经由 C++ 跑过。JSON 清单读取、文件快照加载及标定身份绑定仍待完成。

最新增量 `weapon_asset_emit.h` 已把数值转换串为实际临时输出流程：检查输入→转换位置并写 binary32→旋转并归一化／重新量化三组切线→镜像时交换每个三角的后两索引→从最终舍入后的顶点重算目标单位 bounds→重验字段／分组及零面积三角→一次替换输出。UV、颜色、切线第四字节占位保留，原 initializer 负责最终手性；没有二次翻转 Y。分配异常或验证失败保留原输出。完整函数只做源码编译，尚未执行或验证真实转换结果；5 个实际使用的绕序／小端写入／量化辅助检查已由编译器求值通过。还需清单／哈希读取、标定身份、原生 bounds 向外舍入编码及材质绑定。

继续增量：`weapon_asset_transform.h` 已实现正统一尺度、正交轴（允许反射）、点／方向分离、原生 float 范围保护和绕序反转判定，8 个编译期场景通过。默认全零映射无效；这是数值变换，不是标定证书，实际使用还须绑定准确模型／游戏／组件身份及测量证据。尚未发射转换后顶点记录、重打包切线或实际交换索引绕序。11 份真实 idx16 与全部 section 也已通过实际 C++ 校验；fixture 按真实 source_materials 字段生成，清单读取由生成器承担，不冒充 C++ parser。首次生成器因误用 materials 字段失败，修正为现有 schema 后成功；原输入未改动。

后续全量顶点检查已完成：对清单哈希匹配的 11 份输入、全部 29,616 条顶点记录生成有界分块 C++ 常量检查，全部编译通过。它验证实际候选的字段读取与校验，不再只抽样首尾。分块包围盒不等于整件模型包围盒验证；整件转换后 bounds、真实索引／section 全量 C++ 检查、Native 加载仍待完成。逐文件覆盖记录在私有 reports/asset-full-vertex-coverage.json；重现脚本只为 asset_full_* 检查增加编译器常量求值步数上限，不链接或运行程序。

已按现有 offline-engine-input.v2 的 60 字节顶点／小端 16 位索引格式，新增私有 `weapon_asset_input.h`。当前实现字节长度、2×N 原存储上限、材质分组连续覆盖、索引范围、准确 section min/max、禁用几何碰撞检查，并返回错误类型及 section/index 位置。8 个编译期检查通过；不执行程序。后续增量已加入无对齐假设的小端 binary32 解码、位置与全部 UV 的有限值检查、量化切线正交／长度检查、未用 UV／全白颜色规则，以及源米制空间 AABB 与最远顶点半径平方。新增 8 个合成编译期检查，另取 11 份真实输入的首尾共 22 个顶点生成检查素材；这不是所有真实顶点的完整验证。源空间 bounds 不能在缩放后直接写入引擎；还需完成已认证变换并重算原生 bounds。manifest／SHA256 解析、坐标转换和材质绑定仍未实现，不能称为完整加载器。该增量未加入第六版 ZIP，历史包保持原样。

## 第六批实际交付 2026年10月29日

[第六版累计补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v6.patch)覆盖同一基线的 27 个候选文件，包含前五版；只选一个累计版本。[本批候选与原调用端位置表](../../analysis/offline-preimplementation-20261028/reports/v6-api-map.json)记录当前符号、行号和哈希。未改生产源码、Native 构建、启动器、运行包或 Git 索引。

### 现有输入 owner 已迁移到实际提交账本

`CoverInputOwner` 现在是 `CoverInputOwner<Authority>`，构造时绑定同一 `CoverMotionLedger<Authority>&`，不再隐含拥有第二套身体位置。三个交互更新入口都使用 `update_candidate`。`CoverInputOwnerCandidate` 返回按值保存的候选、手样本、轴、上下文、已提交版本和抓取代数；它不授权写身体或发布 view。

每个新 Scene 在处理输入之前同步已消费的提交。确认的松手、失追或输入屏障会在 begin 中立即撤销未派发请求；不会等武器回调完成才响应。较新的手样本取消旧的未派发运动，已派发操作继续保留回执责任。body 结果或新几何在 prepare 与 complete 之间到达时，旧候选变为 waiting；同一合法基准下保留租约，不能用旧输入覆盖刚发生的提交。

`complete(ticket, CoverInputCompletion, owner, input_grant)` 取代旧 bool 接口。ready／waiting／unavailable 都可以结束本次输入事务并保留合法抓取；fault 才走终止和真实松手重武装。当前动作许可通过原 Authority 的凭据引用核准，初次抓取、后续移动、当前 Tick 执行仍分别认证。完成回执中的位置来自 ledger，候选目标不会作为实际位置返回。相同 ticket 与原调用参数的重读没有新副作用；即使 ready 的运动因迟到结果退化为 waiting，也不会误判同一次重读。

旧数据中的 slot／world／Pawn／basis 改变不能凭观察直接清零 offset。账本先完成真正的恢复或旧 owner retirement／新绑定；输入内核再执行 `handoff_committed`，保留单调请求号并结束旧抓取。几何版本变化取自 ledger 当前上下文，不误用历史消费记录中的旧几何版本。

### view 发布与输入成功分开

`complete` 不再调用 `build_view`，也不会在失败后凭当前观察自行“重新认证”位置。`publish_view(ticket, tick, view_mapping)` 每次先复核 ledger publication，再调用真实 Authority 的 `verify_view`，核对冻结的眼点基准、单位、头部／reference 样本与同一身体／命中消费。没有新的视图凭据时，已有 body 数字不能替代证明。

发布失败不会覆盖调用者原 publication；缓存的输入结果始终不带 view 权限，重复读取不会把旧视图重新标新。普通武器参与者失败后，仍可在本 Scene 凭独立的当前身体／视图证据发布真实位置和头动；变更快照、旧 ticket、显式 invalidate 或过期证据不能走该路径。

实际使用的基底构造仍只加入已提交虚拟 offset；真实 HMD 位移保持在原 stereo 合成中一次加入。身体高度的只读计算与 view 同属该认证位置，不读取尚未发生的拖拽目标。

### 已迁移的检查与明确剩余调用端

`cover_input_owner_test.cpp`、`cover_input_transaction_test.cpp` 已迁移到新接口；后者保留了 broker prepare/commit/drain、身份、快照、失败恢复、输入代数和跨生命周期检查源码。新增 `cover_input_test_support.h` 只给检查源码提供显式合成碰撞／身体／视图回执，不是 Native Authority，不得用于游戏。原“范围缩小直接裁剪、换上下文直接归零”的检查已改为要求真实交接；空间/session 边界检查显式提供新的合成交接记录，不能依靠输入模块自己冒充身体恢复。

另补了等待中松手、每次发布重新检查视图凭据，以及身体结果在 prepare/complete 之间迟到的完整输入流程源码。**这些完整流程只编译，没有执行**。14 个新增编译期场景直接求值实际 ledger 的撤销／视图凭据路径和输入结果策略，累计 **150 个编译期场景、11 份源码兼容性编译**。

主线 Native 调用端仍未改动，不能将累计补丁直接当作可构建的游戏接入完成件：

1. `cover_input_live.inl` 的 `participant` 声明须改为绑定真正的 `CoverNativeAuthority` 与唯一 ledger；旧独立 preview 执行路径继续禁用，不为它增加第二个身体 writer。
2. `cover_shared_input_live.inl::prepare` 维持原 weapon prepare → 一次仲裁 → weapon commit → complete 的同 Scene 顺序。将原 bool 回调结果转换为明确的输入 completion，提供原 owner 签发的 input_grant；不能复制旧 ticket 成跨 Tick 许可。
3. 旧 `result.view_valid` 分支改成在正常 owner 准备阶段取得当前证据后调用 `publish_view`，不得在异常回调或原函数返回通知中补 Trace／getter。实际身体提交仍在 I4 的 C0 owner，而不在 Scene 内。
4. `cover_input_live_test.cpp`、`cover_shared_input_live_test.cpp` 中的 Native 组合 fixture 也需迁移真实 adapter 接口；本批未编译这些 Native 组合测试，不把 common 层 11 份编译算作它们通过。
5. Native Authority、context/collision、C0 与真实 hit/damage/muzzle 对齐、slot/退出/回中安全路径仍未实现。本批固定了所需接口和失败结果，尚不能认证实际引擎布局或调用。

## 第五批实际交付 2026年10月29日

[第五版累计补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v5.patch)在第四版上增加掩体提交内核和原交互模块的延迟输入接口，共 23 个候选变更文件。仍相对于同一基线，只选一个累计版本。所有代码都在私有 staging，未应用到主线。[第五批接口与当前接线点](../../analysis/offline-preimplementation-20261028/reports/v5-api-map.json)记录候选符号和调查时主线 SHA256。

### 已落实的掩体代码

`cover_motion.h::CoverMotionLedger<Authority>` 实现一个待移动请求、一次派发和真实提交账本：

- `acquire → propose` 关联既有左手逻辑租约、抓取 generation、原手样本、上下文／基准／几何版本和当前实际 pose version。请求只记录目标，不移动身体，也不把该目标用于画面。
- `wait / unavailable` 保留合法抓取。`release` 撤销尚未派发的查询／提交，已派发操作仍必须收回执；已经发生的身体提交晚于松手返回时照实记录位置，但不会重新抓取。
- `begin_consume → take_request` 要求当前 GT Tick、物理快照、真实物理姿态、当前重新核准的左手许可，以及同一 intent 的路径扫掠和终点证据。实际派发时再次核对这些证据，不能用旧 Scene ticket 取代新资格。同一请求只能取出一次。
- accepted 必须等于请求位置；clipped 必须位于起点到目标的锁定轴线段上，不能更改另一轴、越过目标或向相反方向扩张。新输入可替换尚未派发的准备结果，旧查询无法提交。
- `finish` 只接收真实原消费者回执，身体、view、hit 的 pose version 必须一致。明确的无写入回执保持原位置；含糊结果、错误消费版本或原回调缺失进入恢复，保留原请求与恢复责任，不能假设骨架已经回滚。
- `refresh_occupancy` 不移动位置。新几何使旧查询失效；明确占用冲突要求真实恢复，空间后来变空也不会自行恢复操作。unknown 只表示不可发布；若后来确认证明原位置仍空，可恢复其可见资格，但不自动抓取。
- `recover` 必须提供实际恢复／路径／消费和占用证据，产生新 pose version。换世界／Pawn 不能复用旧上下文；先由旧 owner 确认 drain／retire，再绑定新上下文，计数不倒退。`publication` 每次按当前 Tick 复核，没有“拿旧位置刷新时间戳”的快捷路径。

原 `cover_interaction.h` 增加 `synchronize_committed`、`update_candidate` 和共同裁剪重基准函数。先同步已完成的身体事实，再读移动／松手；候选计算完成后恢复内部已提交位置，不把等待中的目标累加成真实位移。裁剪时以该次真实手位和实际 offset 重设起点，因此下一次反拉立即响应；旧抓取或另一轴的裁剪不能改新抓取起点。`synchronize_cover_input` 直接把 ledger 中的事实交给现有交互内核；它不认证 view，输入同步失败也不能阻止真实 release。

### 真实证据的责任边界

所有记录中的 `CoverProofRef` 都只是原 owner 证书注册表的引用。`Authority` 必须解析并验证真实实例、当前逻辑租约、调用深度、碰撞快照／查询、C0 消费和命中消费者。不存在 `collision_verified=true` 就能发布的接口。候选没有提供假生产 Authority；编译期 fixture 的合成注册表仅用于验证顺序和版本规则，不能接到游戏。

特别是 `body_version/view_version/hit_version` 相等只是必要条件。实际 Authority 仍须证明真实命中消费者使用同一姿态；现有 `full_body_c0_native_backend` 的消费者成功不自动等于 physics／伤害已验证。所有 native 证书与 recovery／retirement 收尾仍由唯一身体 owner 实现，不新增骨架或相机写入者。

### 已定位、尚未改动的下一段接线

1. `cover_input_owner.h::begin/update_with_decoded/complete`：在每轮输入前同步 ledger；将现有两处交互更新切为 `update_candidate`；候选只暴露不可变 intent 数据。完成接口须区分输入成功、运动等待、硬失败；wait 应结束本次 ticket 并保留合法租约，不能再调用 `complete(false)` 模拟等待。
2. 同文件 `build_view(pending_result_)` 与 `build_view`：删除把尚未提交目标生成可发布 view 的语义。改为只消费已通过 `publication` 核对的身体结果；真实 HMD 位移仍在原基准层合成一次。
3. `cover_shared_input_live.inl` 调查时 227／260／271 行的 begin、仲裁和 complete：保留一次 weapon prepare/commit/complete 和现有 broker 生命周期；Scene 只保存不可变 intent，不能将瞬时 ticket 或注册借用带到下一 Tick。
4. `body_c0_goal_policy_live.inl::pose_begin` 调查时 217 行仍为 `CoverOffset committed{}`，245 行调用 `apply_with_view_marker`；在实际掩体策略分支取得当 Tick 查询及当前租约后，消费 ledger 请求，并在 `pose_end` 的真实正常返回／恢复路径生成回执。普通主线分支保持原语义。
5. `read_view`、权威枪口和真实命中消费使用同一 publication；slot／退出／回中交接仍按掩体 I4 设计，以已认证原点和安全路径处理，不能直接把旧 offset 置零。上下文切换的输入内核重绑定仍需在该协调层完成。

以上接线未编写、未接入；本批 P4 状态为 partial。现有 bool 完成接口和旧 view 发布仍存在于生产文件，不能因为新的独立内核通过检查就声称掩体功能完成。

### 第五批检查

33 个新编译期场景直接求值实际候选 ledger 和实际使用的裁剪重基准函数：等待／松手、派发前后取消、迟到事实、替换查询、左右／顶部组合、有限值、版本／时相／当前租约、裁剪边界、一次派发、无写入／含糊回执、占用变化、恢复与旧 owner 退役、证书 issuer 和陈旧输入。累计 **136 个编译期场景**。

新增 3 份现有 cover 源码兼容性编译，累计 **10 份**。候选 `cover_interaction_test.cpp` 补入延迟输入、裁剪立即反拉、等待中松手和旧确认拒绝两个流程；只做源码编译，未运行完整手势流程。全部检查仍为 C++17 `/Zs`，没有链接、测试程序执行、游戏／XR／模拟器运行。

## 第四批实际交付 2026年10月29日

[第四版累计补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v4.patch)覆盖同一基线上的 20 个文件，含前三版；只选一个累计版本应用。已在私有副本应用并逐文件核对，生产对应路径仍与基线相同。[新增接口位置](../../analysis/offline-preimplementation-20261028/reports/v4-api-map.json)记录本批候选符号、行号和 SHA256。

### Snub 记账与固定滑套分支

`weapon_snub_reload.h` 已使用 `ReloadTransaction<SnubReloadIdentity>`。拔匣不转移原枪内剩弹；旧匣必须明确放回才能肩部取新匣，或带完整插入证据原样重插。新匣插入先请求，再以原库存回执提交 q。原匣重插走无补弹／无奖励退出；满仓、无备用检查使用 serial=0，不伪造原换弹接受。

计划现在执行冻结规格：开始 L>0 只能是拔匣、插匣两个步骤；开始 L=0 必须追加一次包含滑套的完整拉回／释放，侧翼可以随同联动，不能拆成独立抓取步骤。插入后 L 增大不会改变该分支。旧匣原样恢复属于取消检查，恢复原空枪状态，不额外模拟装入新弹。

`acknowledge` 接受带库存 revision 的公共回执，并独立接收原版 readiness；奖励已确认但原版未就绪时仍禁射。`observe_original_ready` 可在原版就绪后更新许可。`take_completion` 只在原版回执之后提供观察记录，旧的裸计数 completion 回调已移除，避免第二次写库存。中断保留已提交数、原目标和时钟；未知结果先对账，再用新意图恢复。

候选原 `weapon_snub_reload_test.cpp` 已迁移成 18 个编译期场景：半仓免拉套、空仓拉套、计划锁定、旧匣检查、满／无备用、手部占用、认证门槛、插入／滑套证据、坐标换代、未知窗口、迟到及拒绝回执、恢复和原 readiness，全部通过。输入仍是合成证据，不认证实际 Snub 滑套轴或原实例。

### Frag 和 Smoke 使用同一坐标消费规则

两个现有交互候选都改用 `GrenadeWorldMapping`。原版类、引信、重力、反弹、伤害和烟雾效果保持各自所有权；本批只修改出手映射和执行凭据。发现并修正原候选的不一致：Frag 未比较身体方向变化，Smoke 则把每次原点平移当成基准变化。

`grenade_same_basis` 比较 motion/reference/basis 代数、冻结参考姿态、单位和身体旋转，不比较正常原点平移。`grenade_origin_progress` 要求原点版本和掩体提交版本单调，同版本不得偷偷改位置；因此小步掩体平移可保留原始 XR 手部采样，但转向、回中、未声明的方向／单位变化仍使采样失效。传送等离散位移必须由现有拥有者推进 motion epoch，不能伪装成普通连续平移。

初速合成实际使用 `grenade_compose_world_launch`：已转换的手部速度 × 单位 + 一次原游戏移动基底速度；出手原点单独相加，接口没有掩体人工速度参数。新增基底速度来源、版本和观察时间，未知来源不当作静止；经过确认的静止允许零速，未认证载具速度不支持。位置／方向转换仍使用原冻结参考和身体基准，HMD 朝向不作为投掷方向。

### I07 原发射适配器必须消费的字段

1. 现有身体／手部拥有者提供 `basis_generation`、`cover_commit_version`、`origin_revision` 和当前已提交的原点／冻结旋转。原点版本覆盖普通身体位置更新，掩体版本只描述人工掩体偏移提交；不能用未确认的请求位置替代。
2. 通过原游戏移动读数提供 `base_velocity_generation`、`base_velocity_observed_ns`、`base_velocity_source`，并认证 `original_base_velocity_verified`、`artificial_translation_excluded`。不能对渲染手或相邻帧镜头位置求差作为基底速度。
3. 释放请求冻结坐标基准和掩体提交版本。执行前必须调用新增参数的 `executable_at`，用当时真实版本核对；发射排队期间掩体版本改变会撤销尚未派发的发射，已派发的仍保留回执责任。
4. 执行回执分别记录 `executed_motion_epoch`、`executed_reference_generation`、`executed_basis_generation`、`executed_cover_commit_version`，不能从请求抄回这些字段冒充实际执行环境。Frag 原来把请求版本再传给自身检查的做法已替换。
5. 原 `Init` 消费请求中的 `launch.direction`，同时覆盖原初始位置／速度；只有实际应用后才能报告 `tracked_initial_direction_applied` 和 `tracked_initial_pose_and_velocity_applied`。两类手雷都检查这些结果。保留原 Spawn／ConsumeAmmo／Init 顺序和各自原引信，不能用原抛物线默认 Speed 再盖回去。

原生调用适配器 `weapon_throw_adapter.inl` 尚未编写／接入，本批并不证明游戏已经使用手部初速。两个实际交互候选已调用新的带认证映射 overload；低层数值转换 overload 只用于纯数学检查，不提供原速度来源认证。

12 个新增编译期场景直接检查公共坐标契约、实际数值合成和两种请求的执行条件；累计 **103 项编译期场景、7 份兼容性源码编译**。两份原手雷检查源码已补入掩体平移前后初速相同及版本拒绝场景，但完整 tracker／手势状态流程未运行，不能将源码编译宣称为实际投掷验证。

## 第三批实际交付 2026年10月29日

[第三版累计补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v3.patch)覆盖同一基线上的 16 个文件，包含前两版；应选择一个累计版本应用，不要逐版叠加。生产文件未应用，私有副本的补丁检查与逐文件哈希核对通过。[候选 API 位置表](../../analysis/offline-preimplementation-20261028/reports/v3-api-map.json)给出本批符号、行号和候选文件 SHA256。

### Lancer 所用的通用模型已使用公共事务

`weapon_reload_model.h` 中原 `ReloadModel` 改为 `ReloadModel<Identity>`，由调用者传入已有认证身份类型。原来以武器地址和裸计数启动、插入即改计数的接口已经移除，避免保留两套冲突记账。`bind` 输入原库存快照；`begin` 输入实际物理意图、原接受 serial 和旧匣实例序号。退匣保留枪内 L 与备用 S，原匣必须明确放回或重插；新匣只预留 q=min(C−L,S)。

`insert → take_request → acknowledge` 才更新原库存的确定结果；插入请求、原库存确认、物理闭锁、原版完成确认是四个独立步骤。状态检查同时区分弹匣是否在位和库存数值，不能因为 loaded 仍大于零就让拔匣中的枪开火。`reserve()` 是可取备用，`inventory().reserve` 是实际原库存；未来原写入端只消费请求里的 before/expected，不能将界面预留值写回原内存。

Lancer 最后动作使用 `MechanismAction::linked_closure`，和 Longshot 枪栓、Gnasher 杠杆明确区分。不需要额外手动动作的武器可在已认证插入后直接发完成请求；未知完美窗口仍可普通完成。逐发路径保留每轮上限、提前关闭和射后循环；已有射后循环与本轮最终杠杆只完成一次。

现有 `weapon_reload_model_test.cpp` 已迁移成 15 项编译期场景，直接使用同一候选实现。`weapon_reload_geometry_test.cpp`、`gnasher_lever_test.cpp` 的旧调用也已迁移；保留其手势／几何检查源码，但本轮仅兼容性编译，未执行几何流程。当前树全文搜索发现的三个 `ReloadModel` 测试消费者都已处理。

### Hammerburst 已使用同一公共事务

`weapon_hammerburst_reload.h` 的锁扣路径继续保留。物理开扣意图通过 `begin_reload`，`begin_r3` 仅辅助；起始需同实例原库存 revision、旧匣序号、原接受及自动补弹已受控的证据。`confirm_latch_open` 后进入持旧匣阶段，明确 `stow_original` 才能肩部取新匣，或用连续插入证据 `restore_original`。锁扣关闭之后还要收到原版完成回执，不能先宣布奖励或允许射击。

例如 L=70、S=3：打开仍为 70／3；取新匣只预留 3；插入成功回执更新为 73／0；关扣与完成回执后可射。原匣重插为 70／3，无补弹／无奖励；满仓或无备用可使用 serial=0 的检查流程，不伪造 ForceReload 成功。

原 `take_completion` 现在只在原版完成回执后提供一次观察记录，**不再是原库存／奖励提交请求**。未来适配器必须改用 `take_request`，避免重复补弹或重复奖励。新旧动作、原库存请求、迟到回执和中断恢复都保持相同 identity／token 所有权。16 项编译期场景覆盖计数、锁扣顺序、旧匣恢复、未知期限、输入证据、中断和拒绝恢复。

### 公共恢复规则补齐

- 迟到的完成成功回执可以记录实际结果，但中断中的事务保持禁射，需同实例原状态对齐和新物理意图显式恢复。
- 含糊回执保留请求；随后取得正确回执只完成对账，不自动恢复操作。不能盲目重发原生请求。
- 中断不自动忘记旧匣，恢复后仍需明确放回／重插。新弹预留可以释放，原库存不能退款式重算。
- 已明确无副作用的完成拒绝，恢复后允许新 token 重试；保留第一次实际闭锁时刻及窗口，不因重试重新计算奖励。Hammerburst 已闭合的锁扣不用虚构第二次开关动作。

本批新增 31 项编译期场景，累计 **73 项**；另有 **7 份**兼容性源码编译。全部为 MSVC `/Zs`，没有链接、测试程序执行或游戏运行。公共事务本身有了两个实际候选消费者，但 Snub、Gnasher、Boltok 等尚未全部迁移，不能把 P1 或 12 枪准备标为完成。原生 timer／库存适配器仍未接入。

## 第二批实际交付 2026年10月29日

[第二版补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v2.patch)是相对同一基线的累计补丁，包含第一版及本批，共 9 个文件。不要先应用第一版再叠加第二版。[私有应用核验](../../analysis/offline-preimplementation-20261028/reports/patch-verification-v2.json)记录基线和候选哈希；生产对应文件仍与基线相同。

### Gnasher 提前闭锁

候选 `weapon_gnasher_mechanics.h` 增加 `request_close`。玩家放回手中未插入的弹药后，可以在本轮装入 1／2 发时进入原杠杆完整下掰回位流程；保留最初 target，不把已装入数重新当成目标。共用 `physical_reload_result` 规定部分装填只获普通完成，全部目标完成且存在已认证期限才可能完美；零新增走取消／无奖励结果。

已插入弹药后不能用 `cancel_reload` 跳过闭锁。未知库存回执进入中断状态并保留原请求；`reset` 对尚未收回执的库存／机构请求返回 false。迟到的正确库存回执仍可更新已提交数量；恢复只允许同一身份、已对齐库存、请求全部收回且恢复追踪之后进行。

候选 `weapon_gnasher_mechanics_test.cpp` 已更新部分关闭、未放回手持弹药、零新增和中断／reset 预期。**其完整几何状态流程只做源码兼容编译，未执行**；编译器直接求值的是共用奖励规则 8 项，不将其声称为完整 Gnasher 运行验证。首次取弹与原换弹开始 serial 的接线仍需逐枪公共事务适配。

### Boltok 逐孔装填与关闭

候选 `weapon_boltok_reload.h` 使用 6 孔显示映射，每次仅允许一发。插入请求记录空孔号和孔位标定版本；只有原库存成功回执才占孔，错误孔位、重复孔位、陈旧标定版本和多发提交均拒绝。原生库存仍只有总数，新绑定按总数生成规范孔位分布；同一绑定恢复保留已有分布，不能以相同武器地址证明同一旧实例。

计划必须包含开放、连续的逐发插入和关闭阶段。`closing_entries[k]` 明确从已插入 k 发后的实际状态关闭，并回到公共关闭后续步骤；不能通过修改 cursor 伪造已完成剩余装填。`request_close` 必须重新取得关闭行程，关闭前不能射击。满仓或无备用可以采用 serial=0 的检查计划；零新增产生 `exit_without_fill`，不授予普通或完美换弹奖励。

`begin_reload` 接受机构打开的物理意图，保留 `begin_r3` 作为辅助入口；两个入口和恢复使用同一单调意图 token 空间。失追／失去持有后仍接收同一身份的认证原生回执；迟到成功只更新实际库存／孔位，保持中断，不能自动恢复射击。明确无副作用的完成拒绝允许用新意图重试，保留原闭锁时刻；含糊结果仍保留请求。新绑定不能清掉未决请求，且必须取得机构已关闭的证据。已确认射击才改变本地弹巢显示索引，不增加手动击锤动作。

[13 个编译期场景](../../analysis/offline-preimplementation-20261028/checks/boltok_compile.cpp)直接求值上述 C++ 候选：部分／完整关闭、零新增、满仓／无备用、孔位与版本、非法计划、关闭起点、迟到提交恢复、拒绝提交、含糊原生变化、射击更新、六孔上限及明确无副作用的完成拒绝后重试，全部通过。输入为合成认证值；真实孔位、实例认证和行程仍未测量。

### 第二批接入责任

- I04 原库存适配器：为 `exit_without_fill` 提供无补弹／无奖励退出路径，认证原生回执和一次性副作用；未知部分写入仍需由游戏线程拥有者调查恢复。候选不会猜库存或盲目重试。
- I06 机构／模型适配器：提供同一代标定中的孔位和关闭变体，从连续手势认证关闭行程。Object03 带动 Object02／08、Object01 固定；离线摆动角不是已认证原动画。
- 公共事务适配：本批复用公共奖励计算和六孔记录，**还未把 Gnasher／Boltok 改为完全使用 `ReloadTransaction` 记账**；不能将这批标为 P1 已完成。
- 原 Boltok 测试源码仍采用七发容量、一次批量补弹及 Object01 关闭的旧合成计划，和冻结规格冲突；本批使用独立新编译期场景，不宣称旧测试已迁移。接入前须迁移旧测试并保留其身份、时序和手势防伪覆盖。

第一版的 21 项编译期场景加本批 21 项，共 42 项；另有 5 份兼容性源码编译。MSVC 只做 `/Zs`，未链接或执行测试程序。当前补丁未应用到生产。

## 第一批实际交付

- [待合入补丁](../../analysis/offline-preimplementation-20261028/patches/offline-preimplementation-v1.patch)：6 个文件，包括 1 个新公共头文件。
- [补丁基线与私有应用核验](../../analysis/offline-preimplementation-20261028/reports/patch-verification.json)：逐文件前后 SHA256；已在私有复制件应用，并逐字节核对输出。
- [新共用换弹事务](../../analysis/offline-preimplementation-20261028/staging/tools/vr-common/weapon_reload_transaction.h)。
- [新目标进度与剩余责任](status.json)。

### 共用换弹事务

`ReloadTransaction<Identity>` 复用现有逐枪 identity，不创建第二套手部协议。`begin` 接受真实物理意图 token 与原换弹接受 serial；满仓／无备用检查使用 serial=0，不要求原 ForceReload 接受无效补弹。

`reserve_item` 只占用可用备用，原计数不变；`request_insertion → take_request → acknowledge` 才接纳原库存的确定结果。退旧匣不会清空 L 或把 L 加进 S，旧匣重插走无补弹退出。目标 q=min(C−L,S,本轮上限)，逐发提前关闭保留最初目标，不能因缩小目标获得完美奖励。

已派发的请求在中断时保留，迟到成功回执仍记入实际提交；未派发请求可以撤销。冲突、部分变化或含糊回执进入 `reconciliation_required`，保留请求，禁止盲目重试。当前接口不会自行修补未知的部分原生写入；实际恢复责任仍由游戏线程适配器承担。`resume` 仅能继续同一实例／同一原换弹所有权。

**当前迁移状态：** 第三、四批已将 Lancer 所用通用模型、Hammerburst 和 Snub 候选接入公共事务；其余逐枪模块仍待迁移，生产旧模型行为仍然存在。该头文件自身不认证手势、追踪或原函数权限，也不写游戏内存。I04 原库存／timer 适配器必须在认证执行域中提供这些输入和回执。

### 扭力弓正常取消

候选 `weapon_torque_bow_interaction.h` 增加 `cancel_charge(stamp, TorqueBowCancelEdge)`，使用 R3 的独立新按下沿，和正常 R2 松键发射区分。尚未派发的开始蓄力可以直接取消；已经派发的开始蓄力先收回执，再发非射击退出请求。取消清除缓冲的松键，不使用故障 interrupt 冒充正常操作。

已经进入 EndFire 请求或记录到实际发射后，不接受新的取消。若原版限时发射与既有取消请求竞争，实际弹体回执优先，不能报告取消成功或退款。取消后等待真实松扳机观察再武装新按下。

纯逻辑方法改为 C++17 constexpr，以便编译器直接检查同一实现的状态变化。原游戏安全退出组合的实际调用还未接入、未验证；配置中的认证标记仍默认 false，不能仅因离线检查通过就开启原生蓄力。

### 手雷计算与两类手雷配置

候选 `grenade_throw.h` 将默认最低速度改为 0，兼容字段仅允许 0；删除 4 cm 位移门槛。保留样本数、时间跨度、连续性、有限值及最大速度约束。回归计算先减去首个位置，降低大坐标相减误差，并拒绝会被平均速度掩盖的瞬时跳变。

新增实际出手朝向：`GrenadeThrowSolution.release_forward → GameGrenadeLaunch.direction`。它来自手中物体在 Grip 空间的朝向，静止时也有效；不会归一化零速度或使用 HMD 朝向。Frag、Smoke 配置都补入 `forward_in_right_grip` 并交给同一 tracker。默认 -Z 表示约定的 Grip 前向；具体物体偏转仍属于原 `grip_geometry_verified` 的真实标定责任，不能把默认值当已测量值。

最低速度门槛的旧检查已更新到候选 `grenade_throw_test.cpp`。仅完成源码兼容性编译，没有执行该测试程序。当前补丁未改变原弹体引信、反弹、伤害或 Spawn 调用。

**当前迁移状态：** 第四批已补齐 Frag／Smoke 候选中的 mapping、基准换代和人工平移分离，详见前文。实际身体拥有者的认证数据发布与 `weapon_throw_adapter.inl` 的 Init／初速消费仍未接入，离线检查不证明原游戏调用成功。

## 第一版历史检查范围

MSVC 19.51，C++17，`/W4 /WX /Zs`，只做编译，不链接、不执行程序。

| 检查 | 范围 | 结果 |
| --- | --- | --- |
| 换弹编译期场景 | 9 个场景：原剩弹保留、旧匣检查、满仓／无备用、部分关闭、迟到提交、未派发取消、含糊结果、拒绝回执、完美奖励唯一完成 | 通过 |
| 弓取消编译期场景 | 7 个场景：派发前取消、派发后等待、重武装、普通发射、开始被拒、实际发射竞争、清理缓冲松键 | 通过 |
| 手雷速度编译期场景 | 5 个场景：静止／轻抛、小范围折返、非法采样、瞬移／超速、大原点 | 通过 |
| 现有源码兼容性 | grenade_throw、Torque Bow、Frag、Smoke 四份现有测试源码，使用候选头文件 | 编译通过；未执行 |
| 补丁 | 私有复制件 `git apply --check`、实际应用、逐文件结果哈希核对 | 通过；生产未应用 |

编译期场景执行的是候选 C++ 中的相同纯逻辑，不是另写的 Python 模型；所有 owner／回执值都是合成输入，不证明真实认证、游戏线程行为、双眼、碰撞或实体手感。

## 后续队列，保留完整目标

1. 衔接 Gnasher 取弹意图和原换弹 serial、Boltok 公共事务及旧测试，再对照 12 张规格卡核对其余差额。Lancer 通用模型、Hammerburst、Snub 的迁移已在第三、四批完成。
2. 手雷 mapping／基准版本与一次速度合成已落入两个候选；继续准备 I07 原生调用适配及身体拥有者数据接线，真实执行留待接入授权。
3. 第五、六批已落实掩体 ledger、common 输入仲裁和独立 view 发布。继续 Native Authority／Scene 调用端和唯一身体 owner 的证书／提交／恢复适配，保留实际 hit/damage/muzzle 与过渡责任。
4. 编写模型输入读取、单位／轴转换、索引／材质槽／包围盒验证的可合入代码；继续原材质绑定与 Snub 私有拆件输入的有界调查。现有输入生成器不等于 C++ 加载器。
5. 落实标定数据校验、身份绑定与失败定位材料，保持未测参数为 null。
6. 各批补丁做当前基线漂移检查、独立检查和包装，最后对照上述完整目标验收；不自动接入主线。

资产后端对应武器交接的 **I06 模型／机构**；前期资产说明中对 I05 的引用是编号笔误，I05 实际为射击门控。此处明确接线归属，避免接入时误改位置。

## 模型文件快照补充（尚未打包）

候选 `weapon_asset_snapshot_windows.h` 将文件打开、限定长度读取、前后文件信息检查和同一内存缓冲区的 SHA256 校验串联起来。只有完整成功才替换输出快照，后续解析应直接使用该快照，不能重新打开同名文件。只接受受信目录下的 ASCII 文件名，拒绝叶节点重解析点；目录及祖先的可信性仍由调用方负责。

文件名检查改用完整字符串长度，补齐路径、保留名称、内嵌空字符及 127／128 字符边界，共 10 条编译期断言通过；Windows 快照适配源码通过编译。文件 API、哈希 API 均未执行。清单解析、标定身份绑定和实际加载验证尚待完成，本批未纳入 V6，也未接入生产。证据见私有 `reports/asset-snapshot-progress.json`。

## 模型清单词法读取补充（尚未打包）

候选 `weapon_asset_json_lexer.h` 提供零分配 token 视图、JSON 数字格式检查、严格 UTF-8／转义／代理对检查，以及不会截断或溢出的 uint32 数量读取。输入内存归调用者持有；错误后解析器保持失败。14 条合成编译期断言以及当前完整 manifest 的词法扫描通过。真实清单 fixture 及源文件哈希保存在私有检查目录和 `reports/asset-json-lexer-input.json`。

这只证明词法读取，不证明 JSON 结构和模型协议正确。对象层级、重复字段（包括转义后的同名字段）、必填字段、字段类型及数量／材质／文件交叉核验仍待上层解析实现。未运行测试程序，未改生产代码，未纳入 V6。编译报告读取增加明确 UTF-8 解码及替换策略，避免本地默认字符编码使错误输出丢失；编译退出码仍决定检查结果。

## 模型清单结构解析补充（尚未打包）

候选 `weapon_asset_json_document.h` 在词法层之上建立有界、非拥有的 JSON 树。检查对象／数组结构、逗号／冒号、结尾多余内容，并在每个对象内按解码后的 Unicode 字符比较字段名；转义与原 UTF-8 写法不能绕过重复字段检查。输入上限 1 MiB、默认 4096 节点、递归深度及字段比较工作量均有限制。失败后不暴露部分树；重新解析会使旧结果失效，调用者必须保持原文本不变。

嵌套查找检查曾暴露递归节点编号问题，改为按值返回编号后通过。17 条合成编译期断言通过，覆盖结构错误、重复字段、Unicode 等价写法、嵌套查找、容量／深度／工作量限制和失败后的不可用状态。完整真实清单共 1166 个值节点通过结构解析，并逐项核对 11 个模型的顶点数与三角形数。

此处仍不是完整模型协议验证：必填字段、字段类型、布局常量、哈希／文件名、材质与分段的交叉检查尚待落实。未执行程序，未接入生产，未纳入 V6。证据见私有 `reports/asset-json-document-progress.json`。

## 模型几何清单读取补充（尚未打包）

候选 `weapon_asset_manifest.h` 将当前 v2 清单读成拥有数据的记录：标识／LOD／部件、来源网格索引、文件名、三个 SHA256、顶点／三角形数、材质数量和分段。读取检查版本、离线状态、单位说明、60 字节布局及 UV 偏移，拒绝必填字段缺失／未知字段、非整数或溢出的数量、非法哈希／文件名、重复逻辑模型及不区分大小写的文件冲突。分段必须连续完整覆盖、引用有效材质槽和顶点范围，且禁用碰撞。整份成功才替换输出。

真实清单 11 项记录及其所有分段读取检查通过；20 个合成正确／错误清单通过编译期检查，错误场景同时检查旧输出未被替换。增加 ASCII 字符串复制／容量／嵌入空字符检查后，结构解析检查共 18 条并通过。全部只编译，无测试程序执行。

`source_materials` 仅验证为非空对象数组并保存数量，PBR 内容不转换为游戏材质；这不是完整材质协议验证。读取结果尚未连接文件快照和二进制检查，真实索引的 min/max、文件哈希匹配仍需后续加载流程核验。标定身份、原生材质绑定及资源创建不由此读取器认证。证据见私有 `reports/asset-manifest-progress.json`；未接入生产，未纳入 V6。

## 清单与模型加载连接补充（尚未打包）

候选 `weapon_asset_loader_windows.h` 提供两个拥有数据的对象。Catalog 读取并验证清单文件的预期长度／SHA256，在同一份内存上完成 JSON 和几何协议检查，保留原清单及记录。LoadedPayload 从已通过的 Catalog 选择模型，读取并验证顶点／索引文件，再通过 `weapon_asset_payload.h` 串联真实索引／分段 min/max 与顶点字段／源空间 bounds 检查。成功结果保留实际被检查的两份数据、记录副本及清单哈希，不重新打开文件。

每层均先构造候选，完全成功后才替换输出；错误保留阶段、原 Windows 错误／哈希错误、JSON 偏移、清单字段或二进制错误细节。移动后的原对象显式失效，不可复制这些拥有数据的对象；调用者须串行管理 Catalog 和输出生命周期。内存分配异常仍向调用者传播，原输出保持不变。

7 个组合二进制编译期用例通过，覆盖有效三角形、记录容量、长度、索引越界、真实分段范围、顶点字段错误与缺失数据，失败时保留旧 bounds。Windows 完整适配源码及 4 条类型约束编译通过，文件／哈希 API 未执行，不能称完整加载运行验证通过。单位转换、标定身份、原生材质、退化三角形的转换后检查和资源创建仍在后续阶段。证据见私有 `reports/asset-loader-progress.json`；未接入生产，未纳入 V6。

## 标定身份与测量字段补充（尚未打包）

候选 `weapon_asset_calibration.h` 区分游戏 EXE、原始游戏网格、清单、作者 GLB、顶点与索引六个摘要，并匹配模型 ID／LOD／部件／来源网格索引。缺少摘要与摘要变化均给出对应字段；逻辑标识长度先检查，避免将非法长度送入比较。mesh→component、component→weapon、grip→weapon、单位和轴向均需要显式已测量标记及有效数值，零／默认单位变换不自动成为测量结果。测量方法、时间、版本和证据包摘要也必须存在；证据路径及原始测量保留在外部证据包。

Binding 复用接入方的现有身份类型，候选不新造手部 Frame 或用裸地址当实例认证。完整检查必须调用拥有者的 binding 与 evidence 核验；需要验证真实武器类／实例／组件、world／session／reference／basis 代数和版本，以及测量包、当前原网格层级／绑定比例。模板没有这些方法则不能调用；真实适配器尚未实现。数据一致不等于运行许可。

9 条编译期断言覆盖六种摘要变化及缺失、模型选择变化、未测量字段、非法数值、证据缺失、模拟身份换代和拥有者拒绝。输入全为合成，不能当成实际标定；计划中的 null 原样保留。加载器到标定消费者、变换只合成一次和实际适配仍待实现。证据见私有 `reports/asset-calibration-progress.json`。未接入生产，未纳入 V6。

## 模型放置与变换合成补充（尚未打包）

补齐标定契约中缺失的 `asset_to_mesh`：补做模型局部米制坐标到原始 raw mesh 的实际测量。旧的 mesh→component→weapon 链本身不足以确定作者模型的偏移、比例和方向；此字段默认未测量并拒绝，不推定为单位变换。`weapon_asset_mapping.h` 按顺序合成，分别输出 asset→component 与 asset→weapon，供消费者明确选择目标空间。

`units_per_metre` 与 `axis_mapping` 在此候选中明确为完整 asset→weapon 链的结果声明，用于和合成结果核对，绝不能再乘入。位置包含缩放和平移，方向只受轴向影响；本层不加入 HMD／世界／身体／重新定位偏移。几何和作者局部交互标记必须使用相同目标映射。输出不是可缓存的运行许可，实际消费仍须检查当前身份。

10 条编译期断言通过：命名空间结果、逐步与合成等价、防重复单位转换、轴向不符、缺失 placement、拥有者拒绝、双镜像、缩放溢出、输出别名和方向变换。原 9 条标定断言更新并通过。当前仅支持正的均匀缩放与正交／镜像轴；如真实原网格存在非均匀比例，须调查并扩展处理，不能暗中归一化。真实标定、主线身份适配、加载对象到几何输出的连接仍待完成。证据见私有 `reports/asset-mapping-progress.json`，它包含更新后的标定头文件哈希；旧批次报告保留其历史身份。未接入生产，未纳入 V6。

## 已加载模型到几何输出连接（尚未打包）

候选 `weapon_asset_prepared_windows.h` 只接受已验证的原始 `WeaponAssetLoadedPayload`，不会接受自身已转换输出作为输入。它复制标定后核验，要求明确选择 component-local 或 weapon-local，再调用同一 emitter；提交输出前再次检查拥有者／证据。成功后通过独占、只读状态保存生成数据、原模型记录、标定副本、源内容身份及坐标空间；失败或分配／复制异常保留旧输出。源记录内哈希标识原始文件，不是转换后缓冲区的哈希。

原作者坐标的交互标记使用相同变换，输出带目标空间并使用不同类型。只读几何访问属于准备材料检查，不授予 Native 提交权限；主线实际消费必须在串行拥有域中使用当前内容身份重新验证。生成前后的检查不能取代该要求。

完整准备和 marker 路径以具体合成 Binding／拒绝型 Authority 实例化并编译通过，未执行 emitter 或任何文件／哈希 API。映射编译期断言增加明确空间选择、非法空间及无效映射拒绝，共 13 条通过；准备对象 2 条类型约束通过。实际材质、资源创建、原生 bounds、测量与身份适配仍待完成。证据见私有 `reports/asset-prepared-progress.json`。未接入生产，未纳入 V6。

## 最终顶点的浮点包围盒编码（尚未打包）

候选 `weapon_asset_bounds.h` 从最终输出的 binary32 顶点位置计算 origin／extent／radius。中心取 binary32 最近偶数舍入，再围绕已编码中心重新计算半尺寸和实际最远顶点距离；正半尺寸／半径向上取可表示值并增加一个有限 ULP，覆盖中间舍入误差。半径通过正浮点数格点的平方比较求取，不依赖主机 sqrt 或 float 强制转换的舍入模式。零尺寸保留零；无法向外表示则拒绝，旧输出不变。

`WeaponAssetPrepared` 已将此步骤放入成功提交之前，保留逻辑 binary32 位字段。它不是可直接 memcpy 的引擎结构，也没有按历史字段偏移写游戏内存。原静态入口图记录 origin／extent／radius 位于 256／268／280，但当前 Native ABI 与实际写入仍由接入方认证。

7 条编译期用例覆盖大坐标中心舍入、实际球半径、极小数、单点、非法／缺失输入、范围溢出和舍入规则；完整准备路径重新实例化编译通过。未执行实际 emitter／游戏裁剪，也未向 Native 提交。证据见私有 `reports/asset-bounds-progress.json`。未接入生产，未纳入 V6。

## V7 模型独立交付包

上述“尚未打包”的模型候选现已收录到 [V7 模型准备包](../../analysis/offline-preimplementation-20261028/gearsvr-offline-model-preparation-v7.zip)，SHA256 `2e5dad2f01ed880748c04bd1011f1c55f667d8da9912baf2503d27e31e9f3be0`。包含 16 个新增头文件、33 份编译检查源码及其报告，共 88 个归档条目；大小 431,993 字节。独立补丁为 [offline-model-preparation-v7.patch](../../analysis/offline-preimplementation-20261028/patches/offline-model-preparation-v7.patch)。

该包仅覆盖新增模型准备模块，不是 V6 的累计替代，也不是游戏运行包。补丁在私有复制件应用并逐字节核对后，用补丁产生的源码重新编译全部 33 份检查，均通过；没有链接或执行测试程序。包内逐文件哈希与 ZIP 完整性通过核对，生产目录中这 16 个新增文件仍不存在。

打包核对发现主线已修改 `weapon_reload_geometry.h`、`weapon_reload_model.h`、`weapon_reload_model_test.cpp`，增加 magazine_close 和旧弹匣恢复逻辑。因此旧基线累计补丁不能声称适配当前主线，后续须在私有候选中保留并协调这些变化。V7 模型包没有包含、覆盖或回退这三个文件；V6 及历史证据保留。完整报告见私有 `reports/model-v7-verification.json`。

本轮因发现基线变化，优先完成独立交付与隔离核验；错误到处理动作的映射仍待编写，未作为已完成项。

## 当前主线换弹差异的私有协调补丁

[reload-current-reconciliation.patch](../../analysis/offline-preimplementation-20261028/patches/reload-current-reconciliation.patch) 以本轮读取的当前主线文件为基线，覆盖通用模型、几何引导、模型检查、事务头文件及两个需要随新接口迁移的检查源码，共 6 个文件。SHA256 `aa62671938fb04b04c3b9a9717eb33052e2466b61b8bc036177ddcb5fa5e4ad7`。在私有副本应用、逐字节核对，并确认本轮核验期间生产目标未改变；没有应用到生产。原始冻结基线、V6 与 V7 都保留。

保留主线 `magazine_close`／`magazine_closure` 名称和数值，旧候选 `linked_closure` 保留为同值源码别名。主线新增的独立闭锁、旧匣保持原剩弹、错误身份拒绝、替换匣提交后拒绝旧匣等需求得到覆盖。新候选的行为差异明确保留：手中替换匣必须先归还；旧匣回插后仍须闭锁，原换弹已启动时等待无补弹／无奖励退出回执。不会恢复仅凭武器地址直接改计数、瞬间归还手持物或跳过闭锁的旧接口。

5 个新增编译期场景覆盖主线独立闭锁、预约替换匣后返回旧匣、替换提交后拒绝旧匣、没有旧匣不能虚构，以及保留待完成射后机构要求；同时重检原 15 个场景和 2 个枚举别名断言。几何与 Gnasher 杠杆检查源码兼容编译通过，未执行程序。

此补丁是通用离线代码的当前基线协调，不证明 Native 库存／身份／物品／手势适配完成，也不能只换一个头文件而保留旧调用方式。其他 V6 文件未在本轮重新定基，旧累计补丁仍不可直接宣称适配当前主线。证据见私有 `reports/reload-current-reconciliation.json`。

## 模型错误处理提示增补

候选 `weapon_asset_recovery_windows.h` 为 88 个现有枚举值（含各类成功值）建立显式处理分类，并按 Load／Prepare 的实际失败阶段选择子错误。返回稳定错误码、处理类别和下一步文本，不执行操作，不自动重试。分类覆盖恢复配套文件、选择匹配标定、补测、重新核验拥有者、修正坐标链／清单／几何、检查文件访问及哈希服务等。调用方必须同时保留原始字段、偏移、顶点／分段与已有 Windows 错误；提示不是原诊断的替代。

11 条编译期用例通过，覆盖关键处理区别、成功结果忽略未使用的默认错误、嵌套顶点错误、末次身份检查失败、未知枚举及矛盾失败状态。未知／矛盾结果不显示成功。显示或日志消费者尚未接入。

[model-recovery-v7-addon.patch](../../analysis/offline-preimplementation-20261028/patches/model-recovery-v7-addon.patch) 仅新增该头文件，依赖已交付 V7 模型模块；已核对全部 16 个基础头文件仍与 V7 相同，私有应用与逐字节核对通过。V7 ZIP 保持原哈希不变，该增补尚未纳入 ZIP。证据见私有 `reports/asset-recovery-progress.json`。未修改生产，未运行程序或游戏。

## Gnasher 物理启动与公共记账适配（未完成调用端迁移）

候选 `weapon_gnasher_reload_accounting.h` 复用 `ReloadTransaction<GnasherIdentity>`，启动必须有当前库存、fresh intent、原版接受 serial 和实际开始 tick。物理取弹同时预约首发；辅助按钮启动不虚构手中弹药。启动与首发预约先在本地副本完成，过期物品失败不会半启动事务或消耗本轮 intent／serial。预检 full／no-ammo 在原版请求之前执行，每轮目标仍为 min(3,空位,备用)。

7 个编译期场景覆盖物理首发、辅助启动、无效接受／库存／来源、部分关闭与真实回执、过期首发的原子拒绝、迟到回执保持原 serial，以及目标上限。原 Gnasher 机构检查源码兼容编译通过，未执行。原 Identity／Inventory 定义抽到 `weapon_gnasher_types.h`，没有新增身份协议；只为纯检查增加 constexpr。

现有 `GnasherMechanicalInteraction` 尚未调用该适配层，旧独立计数仍存在，不能将 P1／P2 标成迁移完成。下一步必须迁移 begin／grab／release／insert／ack／close／cancel／resume，移除重复可写库存；原动画 accepted 仍不能冒充库存退出或奖励已应用，需明确回执。逐个符号及本轮行号已记录在私有 `reports/gnasher-accounting-progress.json`。真实手势、原版请求和游戏执行均未验证。本批未打包，未修改生产。

## Gnasher 机构调用端迁移候选

本轮 `GnasherMechanicalInteraction` 已改用公共 accounting：库存、target 与 inserted 只从 ledger 读取，取弹／释放／插入与回执不再写另一份计数。begin 接受完整 `GnasherReloadStart`；物理首发直接进入持弹，辅助按钮仍留在取弹阶段。插入请求携带实际原换弹 serial。部分关闭保留最初目标；零新增取消需要物理关闭和无补弹退出回执。确定拒绝的插入释放预约，重新取弹使用新物品编号。

`GnasherCycleRequest.reload` 承载独立的公共完成请求。`acknowledge_cycle` 只确认原动画接受，`acknowledge_reload_completion` 另外核对实际库存／奖励回执；仅动画 accepted 不开放射击、不证明奖励。失败／迟到提交保留真实记账并要求同一次原换弹恢复。物理 stroke 判定保留。

公共 ledger 增加仅关闭状态可用的 `rebind_closed`，跨 reset 保留请求／物品／意图计数，同身份保留原 serial 下限；未决或未完成换弹不得 reset。Gnasher 重置保留该 ledger，避免旧回执编号撞上新请求。新增编译期用例覆盖这一风险，accounting 共 8 条通过，公共事务及通用模型检查回归通过。

[gnasher-common-ledger-candidate.patch](../../analysis/offline-preimplementation-20261028/patches/gnasher-common-ledger-candidate.patch) 包含 5 个文件，SHA256 `9662dca6f8675bc341a8663cacb16c9eb122dc7d7e4dec641c021419ab80f5ff`。按当前生产目标建立私有副本、应用补丁并核对字节后，从当前生产复制其余依赖（共 14 个源／依赖文件），accounting／完整机构检查源码／公共事务三份检查重新编译通过。没有修改生产、链接或运行程序。

完整手势、动画、完成回执的先后流程只做源码兼容编译，未执行；Native 原版接受、库存与奖励适配仍未实现，不能把这批称为游戏换弹完成。模型 V7 与既有补丁保留原版本；新 Gnasher 补丁携带更新后的公共事务，不能与旧版本该文件盲目覆盖叠加。证据见私有 `reports/gnasher-ledger-migration.json`。


## Boltok 公共弹药适配层候选（机构迁移前置）

新增 `weapon_boltok_reload_accounting.h`，从公共事务读取弹药数量；六弹位只保存视觉分布。身份与库存声明抽到 `weapon_boltok_types.h`，原机构头文件复用相同声明。尚未将原 `BoltokReload` 的调用路径迁移到此适配层，不可声称消除了原机构里的重复计数。

- 插入请求不会立即占用弹位；原版接受且库存读回相符后才占用。拒绝后释放预留，需要新的取弹令牌。
- 已发出的请求在中断后仍可接收回执，确认的弹位保留；恢复必须对应同一次原版换弹。未发出的请求中断后取消。
- 零目标检查不启动原版换弹；已启动但没有插弹则走不补弹退出。
- 原版确认的一发射击才移除一个视觉弹位。同一身份重新绑定保留已知弹位分布，不能把空位重新排列；新身份才使用规范初始分布。
- 弹药类型不匹配不会消费待处理请求；错误库存或奖励回执保留待核对状态。

九项新增 constexpr 场景和既有 Boltok 编译期场景通过；没有链接或执行测试程序。报告：`analysis/offline-preimplementation-20261028/reports/boltok-accounting-foundation.json`。旧生产测试仍有七发容量、批量插弹、Object01 关闭及缺少关闭分支/弹位标定等过时假设，尚待迁移；不能把旧测试能编译当作当前规格已覆盖。后续先迁移机构调用端，再整理基于当前主线的补丁。此候选未打入已有 V6/V7 包。


## Boltok 机构调用端迁移候选

前一节的“尚未迁移机构”状态已由本节更新。`BoltokReload` 现在通过 `BoltokReloadAccounting` 完成开始、预留、释放、插入回执、关闭回执、射击读回和恢复，不再拥有独立可写库存、目标、插入数量、预留数量或六弹位计数。机构仍负责原计划顺序、真实运动证明、时间/身份/弹药类型/事件序列校验；外部请求使用统一的本地请求令牌，并明确映射到公共事务请求，避免与射击令牌碰撞。

变化：拒绝插入释放预留，重新取弹；零目标物理检查不发送原版完成请求，重新读到原版 ready 后才能射击；中断后接收的完成回执需要恢复；明确拒绝的完成请求可经恢复重试且保留物理关闭时刻；跨重新绑定保留 intent/item 单调序列，与公共事务一致。

17 项机构 constexpr 场景和 9 项弹药适配 constexpr 场景通过。补丁在当前生产基线的私有副本应用并核对哈希，依赖使用当前生产头文件，再编译两份检查源通过。没有执行测试程序或游戏。报告：`analysis/offline-preimplementation-20261028/reports/boltok-ledger-migration.json`。

补丁：`analysis/offline-preimplementation-20261028/patches/boltok-common-ledger-candidate.patch`

SHA256：2c655dfe6828bf3fb0a86c83f9c12c934de54716ba94bc29403234c2e3da3cca

剩余：旧生产测试的规格迁移及覆盖对照尚未完成，原版接入和实际运行未完成。本补丁包括公共事务文件，与 Gnasher/早期换弹补丁有重叠，不能盲目叠加；需统一候选版本后接入。已有 V6/V7 包保持不变。


## Boltok 旧测试覆盖迁移：输入与回执校验

新增 `checks/boltok_legacy_guards_compile.cpp`，使用当前六弹位、逐发插入 fixture 迁移旧测试有效校验。11 个 constexpr 组通过，其中 107 个错误输入变体覆盖绑定/启动、弹药身份、连续运动、库存/完成回执、九项身份字段和原版射击回执；另有 4 个关闭时机边界、迟到完成回执，以及释放和 100ms 新鲜度边界。均由编译器计算，没有执行测试程序。

旧预期不能机械复制：七发容量、两发射击消耗、批量插弹、移动 Object01、未结算请求直接重新绑定、拒绝插入后保留预留、无需关闭的仅插入计划都与当前规格冲突。覆盖对照与剩余项记录在 `analysis/offline-preimplementation-20261028/reports/boltok-legacy-guard-coverage.json`。

剩余需要明确补齐追踪断点后的新轨迹、未知完美时限/最大 tick、原版射击节奏与弹药守恒网格，再形成可替换旧测试的完整候选。目前尚未替换生产测试，也未刷新已交付补丁。


## Boltok 独立测试候选与含测试补丁

补齐追踪中断前/缺失样本起点的轨迹拒绝、新连续轨迹恢复；未知完美时限即使填有数值仍按普通处理；最大 int64 游戏 tick；原版 ready/同一 trigger/空弹/拒绝射击后的计数检查。守恒网格覆盖 0..6 已装弹量与 0/1/2/10 备弹共 28 个组合，逐步检查取弹、插入、回执和关闭。

`staging/tools/vr-common/weapon_boltok_reload_test.cpp` 是独立测试替换候选，包含 17 个机构场景与 15 个迁移校验组，均通过 static_assert 在编译时计算。另有 9 个独立弹药适配场景。这里不声称覆盖所有可能输入，也未执行 Native/游戏/头显验证。生产测试尚未修改。

含测试的五文件补丁：`analysis/offline-preimplementation-20261028/patches/boltok-common-ledger-with-tests.patch`

SHA256：60589a07b2f3fb1f6f431ae9021c207a7a5f72425c46022de7f763c1a0dd358c

当前主线私有副本应用、逐文件哈希、使用当前生产依赖的四份编译检查均通过；原补丁保留。详细报告：`analysis/offline-preimplementation-20261028/reports/boltok-ledger-with-tests.json`。没有运行生成的程序。后续转向原版接入口与多补丁共享文件的整合审查，不再反复运行未改变的检查。


## 共享换弹补丁合并审查：保留当前主线能力

发现此前几个独立补丁虽然能应用且编译，但其候选公共事务文件比当前主线少了 `infinite_reserve` 与 `dispatched_request`。直接使用旧候选会删除原版无限备弹支持和只读已发出请求观察。这说明应用/编译通过不能替代语义合并审查。旧补丁保留用于历史，不再作为当前接入入口。

现以当前生产 `weapon_reload_transaction.h` 为基础，只加入已完成关闭后保持令牌历史的 `rebind_closed`；原无限备弹目标、库存策略匹配、插入不减有限计数、已发出请求只读观察全部保留。通用换弹模型同时保留库存策略匹配和零计数无限备弹开始条件。

新的合并候选包括通用换弹、Gnasher、Boltok 及相关测试，共 14 个文件。补丁：`analysis/offline-preimplementation-20261028/patches/reload-composed-mainline-preserved.patch`

SHA256：481b3374de76336a074cef831cee2e1f4cdae0a021b33475d4d4fa1c41057989

当前基线私有应用与逐文件哈希检查通过；11 份编译检查通过，包括当前生产公共事务测试的副本（无限备弹/策略变化/恢复观察）及三类候选检查。未执行程序，未修改生产文件。报告：`analysis/offline-preimplementation-20261028/reports/reload-composed-mainline-preserved.json`。

仍需补齐 Gnasher/Boltok 专用库存结构的无限备弹标记传递及目标/回执计算；目前它们仍表示有限备弹。不能因公共文件保留该能力而宣称专用适配已支持。Native 接口与游戏验证仍待完成。


## Gnasher / Boltok 无限备弹策略传递

专用库存结构增加默认 false 的 `infinite_reserve`，保留真实有限备弹计数；公共库存转换、回执、恢复、射击检查、Gnasher 三发目标、Boltok 六弹位计划和插入请求均传递该策略。无限时插入只增加装入数，不扣减有限备弹计数，不以大整数模拟无限。外部插入请求也明确携带策略。

新增 3 个 constexpr 场景：Boltok 零有限备弹的无限装填至射击完整机构路径、待处理插入回执更换策略时拒绝结算、Gnasher 无限三发目标及部分装填普通关闭。Gnasher 完整机构无限路径仅编译，未执行；原版策略读取和身份认证尚未验证。

新合并候选：`analysis/offline-preimplementation-20261028/patches/reload-composed-infinite-policy.patch`

SHA256：7235d06af5833b0f070b863bb53c1ba129530da57c8c3a98e8a7d10d9562b45a

私有当前基线应用与哈希核对通过，12 份编译检查通过，无程序执行或生产写入。报告：`analysis/offline-preimplementation-20261028/reports/reload-composed-infinite-policy.json`。上一版保留历史；下一步查原版策略读取、换弹所有权、库存提交与完成/退出回执的真实接入口。


## 原版换弹接入口源码审查

已定位当前 Lancer 的库存/策略读取、完整身份、迟到回执观察、单次库存写入、原版换弹所有权、时钟、非补弹退出、恢复、奖励和完成调用端。11 个入口的准确行号与文件哈希保存在 `analysis/offline-preimplementation-20261028/reports/native-reload-seam-audit.json`。这是当前源码证据，不是新增原版逆向或运行证明。

关键接入约束：现有 read 明确限定 Weap_AssaultRifle，不能直接替换类名并宣称跨枪成立；完整 Native 身份含对象身份及启动/注册/生命周期/持握代数，候选身份不能仅由指针强转构成；原版接受时间采用控制器单调毫秒，候选 game_tick 必须使用同一时基；已提交库存后走不补弹退出，再单独结算奖励，避免原版自动补满第二次；recovery_required 表示仍持有未解决所有权，不能当作普通拒绝清零。

剩余原版问题收敛到：两枪状态覆盖/正常模式/Abort 路径、奖励适用性和算术、完整身份映射、统一所有权调用端及视觉清理。下一步查已有知识库和保存证据验证两枪差异，不改当前生产接入。


## 保存原版证据复核：动画通知与奖励时机

知识库先查 `AbortWeaponReload`，随后核对 8 份保存的 UnrealScript 导出并记录各文件哈希。详见 `analysis/offline-preimplementation-20261028/reports/saved-weapon-reload-evidence-audit.json`。没有打开 IDA 或运行游戏。导出有未解析 native token/缺失类型，当前包哈希未重核，因此不提升为当前运行证明。

Boltok 的 CockCloseFinish 导出仅播放音效，不能据此确认库存或机构完成。Gnasher 的 ShotgunCocking.FinishedCockingShotgun 包含 HasAmmo、mode0、EndFire(4)、收枪/继续射击/Active/空弹分支，需要保留原状态职责。奖励 ActiveReloadSuccess 显式切换模式 10/11，支持保留先不补弹退出、再奖励、恢复模式的接入顺序，但两枪原版覆盖仍需核实。

发现奖励接入的具体待解问题：IsApplicableForARDamageBonus 读取 AmmoUsedCount 和游戏规则；现有 perfect_reward 在实际装填后查询，输入已变化。仅修正 ActiveReload_NumBonusShots 算术不足以证明奖励语义保持。下一步解析相关 native 运算符并对照装填前后条件，不擅自改动生产奖励逻辑。类导出存在缺失函数声明，不能以未搜到覆盖作为继承证明。


## 原版奖励谓词已解析：确认装填前后语义差异

保存 Core 声明确认 129=!、130=&&、151=>；原奖励适用条件为 `!(有规则对象 && 禁用伤害奖励 && 多人游戏) && AmmoUsedCount > 0`。所以当前 `perfect_reward` 在满装后查询适用性，会使后面的原版伤害奖励分支不执行。该分支不只有奖励弹数，还含层级、计时器、视觉与声音；仅校正计数不能修复。此结论来自保存脚本与当前源码对照，未作当前游戏复现。

新增纯参考头 `weapon_reload_reward_reference.h` 与 11 条编译期断言。它不发放奖励，也不作为 Native 修复。奖金算术的整数 min 简化仅适用于数值精度边界内；原脚本含 float 转换，累计奖励很大时不可无证据等价替换。

同时扫描两枪及 PistolBase/MeleeComboBase 的保存函数索引并记录父类与匹配结果；索引没有覆盖条目不是当前运行继承证明。详细证据与哈希：`analysis/offline-preimplementation-20261028/reports/reload-reward-predicate-resolved.json`。下一步定位原版底层奖励副作用入口，按装填前条件执行且避免临时改写弹药；不以降级普通换弹代替修复。


## 原版奖励修复实施顺序

保存调用链确认：WarPawn.ActiveReloadSuccess 已包含计分、GUD39、本地连击、controller/HUD，不能与完整 LocalActiveReloadSuccess 重复调用。伤害分支另有 bActiveReloadBonusActive、tier、bonus shots、两条原版关闭计时器、HideMuzzleFlashEmitter、TurnOnActiveReloadBonus_VisualEffects 和 sound23；不适用分支为 sound22。视觉关闭还负责清零 tier，与伤害关闭不是同一职责。

实施方向已收敛为：保留装填前适用条件/计数；物理关闭且原版非补弹退出后，按原顺序执行一次反馈、一次角色奖励、必要伤害分支与原版视觉/声音；始终不改写已提交弹药。不能先调用完整奖励再补效果，否则可能先播错误 sound22 或重复计分。需要分阶段保存 dispatch/known-applied 回执，不确定结果不能重放或降级成普通完成。

九个实施阶段、原版入口、读回与失败恢复要求在 `analysis/offline-preimplementation-20261028/reports/reload-reward-repair-recipe.json`。目前是可审查的修复方案，尚未改生产适配；反射参数、数值边界、规则变化和非幂等副作用回执仍需落实。


## 奖励数值与反馈调用约束

新增 `weapon_reload_reward_math.h`，按保存脚本的 binary32 顺序计算奖励数，先拒绝整数加法溢出和越界 float→int；12 条 constexpr 检查通过。大值反例：16777216+1 经原顺序会得到 16777216，不能恒等替换成整数 min 增量。仍未验证原版运行浮点模式/越界语义，未接入生产或已有补丁包。

保存反馈函数会因 PlayerInput.bUsingGamepad=false 返回；函数成功不证明 XR 震动，不能为此伪造全局手柄状态。声音静态调用需要实际 SoundGroupClass 的默认接收者，按原参数 P/Id/bNoReplicate 和原默认 Sounds 表处理，不能直接用基类。ScoreActiveReload 既增加计数又可能发事件，计数读回不能证明嵌套事件全部成功，未知调用结果不能重放。

参数名来自导出/索引，参数 flags/offsets/类别仍需当前反射核实。报告：`analysis/offline-preimplementation-20261028/reports/reward-call-contract-and-math.json`。


## 奖励分阶段回执候选

新增 `weapon_reload_reward_journal.h`，最多 11 个有序效果：反馈、角色计分/HUD、奖励标记/层级/计数、两条计时器、隐藏枪口/启用视觉、声音、最终读回。原版游戏线程调用端仍未连接。

状态处理先记录发出再交给外部执行；未知结果保留原请求，禁止重试/覆盖；明确无副作用才允许经所有者恢复后用新令牌重试；中断后的回执可结算但不会自动继续下一效果。4 个 constexpr 场景通过，覆盖顺序、未知结果、明确无效果重试、错误身份和迟到回执。

Applied 必须证明所需嵌套效果，不是原函数返回就算成功；NoEffect 不能由超时猜测。调用端需要真实所有权与恢复证据，销毁 journal 不会取消原版副作用。报告：`analysis/offline-preimplementation-20261028/reports/reward-journal-candidate.json`。未打入旧补丁、未运行程序。


## 奖励函数参数布局候选

新增 `weapon_reload_reward_signature.h`，校验 waveform、pawn_success、sound 三种已定位调用的参数布局。区分局部变量与输入参数；拒绝未知输入、输出/返回参数、重复名、类型/宽度/数组维数不符、越界、重叠和非法布尔掩码。三参数声音调用不再套用当前只接受一个 offset0 参数的 input_parameter。

6 条 static_assert 通过，其中包括 9 种声音错误布局；测试布局是合成值，不是已测原版偏移。此校验器只处理稳定快照，真实反射采集、对象类别/静态默认接收者、函数身份和原版副作用回执仍需调用端完成。无程序执行或生产修改。报告：`analysis/offline-preimplementation-20261028/reports/reward-signature-candidate.json`。


## 奖励反射快照的复用边界与实现位置

源码核对确认：weapon_metadata_parse 的 TickScope/Walk 是解析加速和有界遍历，可复用但不构成调用授权；damage metadata 的 function_fields 强制 FloatProperty 返回，不能直接用于 void 奖励；ComponentPreparedCall 固定 12 字节和三个 setter，其完整头读取与 native 地址范围假设不能复制到脚本奖励函数。Args 仅构造/调用参数，不提供完整身份与布局稳定性证明。

六个精确源码位置与哈希、八步采集/重读/调用前复核流程、八类验收用例记录于 `analysis/offline-preimplementation-20261028/reports/reward-reflection-reuse-map.json`。明确要求快照自持参数名，不能让 string_view 指向临时名字缓冲；未知局部变量按函数范围检查，输入字段按参数范围检查。

实际奖励专用 reader 仍未实现，此轮未放宽既有 reader 或修改生产代码。该结果决定采用原底层身份/遍历服务加专用快照，而不是扩大伤害/组件调用许可范围。


## 奖励参数快照采集核心

新增 `weapon_reload_reward_snapshot.h`：通过调用者提供的既有身份类型/Reader，采集最多64字段，自持名字，检查字段链循环、outer、局部变量范围和参数范围；两次语义快照与头部重读一致后，连接已有 signature 校验并发布输出。工作区由调用者提供，不要求在游戏回调栈上分配大临时对象。失败保留旧输出，调用者必须检查返回值，不能把旧输出当作新证据。

稳定读取/名字拷贝和7种负例通过编译期断言，负例含函数身份变化、字段循环、两次读取布尔掩码变化、错误outer、失去所有权、名字越界和缺失输入标记。实际 Native Reader 特化仍未接入，未读取游戏内存或运行程序；这不是完整运行时读取器已完成。报告：`analysis/offline-preimplementation-20261028/reports/reward-snapshot-core.json`。


## 实际元数据 Reader 私有候选

新增 `staging/tools/playground/weapon_reload_reward_parameters.inl`，直接复用当前 gripdamage_metadata 的受限读取、对象表/名字表与 raw_identity/fresh_name，不调用原 damage 查询准入。固定调用者提供的完整 receiver/function/owner 身份，限制目标奖励函数短名与 void 返回头；读取项目现用字段布局、布尔掩码，自持名字并连接双快照/签名校验。对象参数目标类别必须由专用验证提供，并再次核对该类别身份。

编译单元包含当前生产 `weapon_grip_damage_metadata.inl` 并实例化实际 Reader 的 capture 模板，/Zs 通过；没有链接、运行或读取游戏内存。确切包/路径、继承、声音默认接收者、函数 flags、游戏线程/reward lease 的 Authorize 回调，以及 ObjectTarget 反射类别读取仍未实现；缺失回调会拒绝，不能把此候选称为完整授权接入。

报告：`analysis/offline-preimplementation-20261028/reports/reward-native-reader-candidate.json`。下一步落实两个调用端验证，保留现有生命周期/所有权边界。


## 奖励函数所属路径与接收者继承准入

实际 Reader 增加三个明确声明路径：Engine.PlayerController.ClientPlayForceFeedbackWaveform、WarfareGame.WarPawn.ActiveReloadSuccess、WarfareGame.WarSoundGroup.PlaySoundWithNoSpatialization。核对函数 outer、Class 类型、包名/Package 类型/根 outer，接收者类链必须到达该所有者；最多40级，拒绝循环，每个类身份和 parent 链接重读。未验证的声明覆盖不自动允许。

使用现有元数据源码编译通过，未运行实际内存测试。PropertyClass 知识库查询无结果；现有检索未得到对象参数类别、FunctionFlags、静态 CDO 的足够布局依据，因此没有猜常见 UE3 偏移。默认对象名字不能单独放行，原必需回调仍保留。报告：`analysis/offline-preimplementation-20261028/reports/reward-owner-path-admission.json`。


## 奖励离线准备归档 v1

`analysis/offline-preimplementation-20261028/gearsvr-offline-reward-preparation-v1.zip`，SHA256 000eb57eb628258b45850c2bc67a40a8b2acb976aaaf4b7321f97dff22fc9411。包含候选、编译检查、证据和缺口清单，逐条 ZIP 字节核对通过；不是独立构建包或可接入成品。PropertyClass/FunctionFlags/CDO 定向缓存检索未得到布局证明，细节在 reward-layout-lookup-closeout.json，不能以无命中推断功能不存在。暂不重复相同检索；继续其他武器/掩体离线准备，三项原生布局问题保留给有界定向核实。

## Longshot 五步机械合同落实

私有 `weapon_longshot_reload.h` 的计划校验改为固定 open_breech → eject_case → take_round → insert_round → close_breech。退壳由原动作完成，不再要求独立左手操作；开闭使用同一已标定枪栓。原候选允许取弹/插弹两步直接到 final_ready_state，与已定 W04 不符，现已拒绝。实际枪栓复合运动、行程、原动作身份仍需外部认证，没有写死几何常量。

`longshot_plan_compile.cpp` 的5组编译期断言覆盖正确计划、缺少机械动作、错误动作顺序、5种手部角色错配和3种闭锁错误；/Zs 通过，未执行程序。此修改只在私有候选中，未替换生产头文件。旧生产测试中的退壳 needs_left_hand=true 夹具也需要随未来迁移修正，不能宣称旧测试已通过。begin_r3 旧接口、共用库存账本、已派发请求中断处理及无限备用弹仍需后续逐项迁移。

随后已补入 `begin_reload(start,intent,source,now)`：只接受已认证的 mechanism_open 或 auxiliary_button，物理开膛必须左手追踪有效。正常开膛不依赖 r3_press_edge；保留 begin_r3 兼容入口映射到同一事务。两种输入使用同一 owner 分配的意图编号，Native 不应将两套独立按键/手势计数直接混用。原换弹编号、自动补弹保持、库存确认和原游戏时钟仍是必要条件；启动后 step_index=0，完整开膛动作仍由后续行程回执完成。

`longshot_start_compile.cpp` 新增4组断言：无R3物理启动、5种无效启动、活动事务不被按钮重启及中断后同意图拒绝、辅助按钮入口。两份 Longshot 编译单元 /Zs 均通过，未执行完整换弹。剩余重点是旧 interrupt 清空 pending request 的行为；在共用账本迁移前，此候选不得被当作可接入 Native 的完整模块。

## Longshot 共用记账适配层

新增私有 `weapon_longshot_reload_accounting.h`，以 ReloadTransaction 为唯一库存/预留/请求持有者，验证单发容量、五步计划、物理或辅助启动、原换弹编号与完美窗口；提供原实弹预留、插入、闭锁完成、只派发一次、只读已派发请求、未知结果保留、中断和同一原换弹显式恢复。LongshotInventory 增加尾部 infinite_reserve 字段，记账转换保留原无限备用弹规则，零备用数也不被当作空库存。

5组编译期场景通过：插入派发后中断并收到迟到结果、无限备用弹零计数、错误政策回执保留待核对请求、明确拒绝后必须换新实弹编号、完成回执迟到后显式恢复且不重复派发。测试最初把 expected.revision=0 占位误当实际版本，编译期拒绝后已改为显式原库存回读版本；共用账本规则没有放宽。

此层尚未取代 LongshotReload 机械类中的旧 inventory/request 成员；完整机械流程、旧测试夹具、Native 接口与实际恢复仍未迁移。不要把新适配层通过等同于 Longshot 全流程完成。下一步应移除机械类重复可写库存、连接这层并保留闭锁时刻，不能同时保留两套记账。

## Longshot 机械调用端迁移完成（私有候选）

后续已将声明拆到 `weapon_longshot_types.h`、机械流程迁到 `weapon_longshot_mechanics.h`；原 `weapon_longshot_reload.h` 保留为包含入口。旧实现快照保存在私有 `reports/longshot-before-mechanical-migration.h`。机械类不再持有独立库存、库存请求、完成请求或请求编号，全部通过 LongshotReloadAccounting。held_item 只记录抓持对象身份，不作为库存计数。

已派发插入/完成回执在失追或中断后仍可结算；操作保持 interrupted，只有同一原换弹、真实库存和当前机械状态确认后才 resume。插入被明确拒绝回到取弹阶段，需新实弹编号；闭锁完成被明确拒绝可新编号重试，保留原闭锁时刻和奖励窗口。完成回执增加 result_applied，不能仅凭 original_ready 推断完美奖励已经落地。无限备用弹策略随请求/库存保留。

`longshot_mechanics_compile.cpp` 的5组编译期完整状态流通过：有限/无限备用弹正常五步、中断后的迟到插入、迟到完成、拒绝完成后的固定闭锁时刻重试；同时包含前面的计划/启动/账本断言。是编译器对合成状态流的求值，未执行实机行程、游戏或测试程序。旧生产测试迁移、不同绑定的关闭后重绑定、原动作与Native奖励/库存消费者仍待完成；未生成可直接应用主线的成品补丁。

## Longshot 完成后的新实例重绑定

私有机械类和记账层现允许在旧事务已完整闭锁、完成且无待处理请求时，以更高 binding_generation 开始另一个实例的换弹；不能通过换对象清除未完成请求。共用账本 rebind_closed 保留请求、意图和实弹编号历史；新实例允许原换弹编号从自己的序列开始，旧实例回执不能用于新实例。重绑定和 begin 在临时账本上准备，失败不会破坏原已完成状态。

`longshot_rebind_compile.cpp` 新增3组编译期检查：完成后新实例请求编号继续增加、旧实弹及旧回执拒绝；旧意图/旧绑定失败不改变原库存且随后新绑定可成功；中断中的已派发请求不能被新实例覆盖且仍可结算。检查同时包含之前所有 Longshot 编译期流程，/Zs 通过，未运行游戏。旧生产测试语义迁移、Native 接线和实机标定仍未完成。

## Longshot 旧检查迁移与最新共用账本差额

已将生产 Longshot 测试的5个分组迁到私有 `weapon_longshot_reload_test.cpp`，改为 static_assert 编译期求值；仍有90个 check 调用位置（不是90种独立场景证明）。保留旧的正常流程、计时边界、计划/弹药、追踪/重放和原回执防护；更新自动退壳、固定五步、同轮恢复、迟到事实、拒绝后新实弹和 result_applied 等语义。迁移脚本与变更表：`migrate_longshot_legacy_checks.py`、`reports/longshot-legacy-migration.json`。完整5组已通过 /Zs，main 不执行任何检查，本轮也未链接/运行。

本轮发现当前生产共用账本已新增 rebind_suspended；旧离线副本会删掉它。现已在私有副本逐字保留该方法，再保留本任务所需的 rebind_closed；当前差额只有新增10行。复制当前生产账本测试到 `current_suspended_reload_compile.cpp`，与迁移后的Longshot旧检查、完整流程/重绑定检查一同通过。之前归档的补丁仍是历史基线，不能直接覆盖当前主线；下一次生成补丁必须使用此次合并后的头文件并重新核对基线。

## Longshot 当前基线离线包 v2

`analysis/offline-preimplementation-20261028/gearsvr-offline-longshot-v2.zip`，SHA256 `510dc349e20d40db4b5b5e48545bb304789f866dae59d35fa2e43fd764cb4d18`，20个条目。补丁 `patches/longshot-current-ledger-v2.patch`，SHA256 `cedb4b3cb013dce9612f27741f0fa2ac180d541bc268db29a88f79bb144701d9`，涉及6个文件。包中保留当前基线/候选/源码指纹、候选文件、编译期检查、报告和限制说明。

补丁在新私有目录复制的当前基线上实际应用，全部候选字节匹配；使用应用后的文件编译 Longshot 旧检查、完整流程/重绑定检查和当前主线账本检查，三单元通过。前后确认生产基线及当前账本测试未变，ZIP全部条目逐字节验证。未修改生产或Git索引，未链接/执行程序。

首次v1尝试因新文件被Git转换为CRLF而未通过字节核对，保留失败补丁及目录；v2显式将交付文件规范为CRLF，同时保留原staging指纹，不忽略字节差异。此包只覆盖Longshot与新增共用rebind_closed，不是以前多武器补丁的累计替代包；共享头文件必须按当前基线合并。Native库存/奖励消费者、动作识别及实机标定仍待完成。

## Boomshot W07 合同收敛

私有 `weapon_boomshot_interaction.h` 原先允许多发容量、projectile装填、insert-only计划、任意额外after-shot手动流程。现按W07限制capacity=1、original_ammo_cost=1、verified_carrier；空仓计划固定为barrel开放、feed_port插入一个组件、barrel闭合。射后计划只能有零个手动步骤，正常满仓不进入半仓流程。原弹体数量仍由认证配置给出，没有替换原弹体/奖励/散布语义；实际运动方向与行程继续要求外部标定。

`boomshot_plan_compile.cpp` 四组编译期断言通过，含10种计划错误、7种配置错误和半仓拒绝。仅覆盖计划/配置合同，不是完整Boomshot状态流。旧测试使用capacity7/一次两发/飞行弹体/射后循环等泛化夹具，与W07不符，尚未迁移，不能沿用它们的通过结论。物理启动、共用账本、已派发请求中断恢复、无限备用弹、原Notify副作用及实际投射物生成仍待核对和落实。


## Boomshot 物理换弹启动入口

私有候选增加 begin_reload(frame,start,intent,source)，接受 mechanism_open / auxiliary_button。R3 入口保留为同一流程的兼容调用，正常物理启动不依赖 r3_press_edge；两种输入必须由同一 owner 分配单调意图编号，原换弹编号、原游戏接受和自动完成抑制仍须确认。启动后保持 step_index=0，不能把启动意图当作开机构行程已经完成。实际抓持接触认证仍由未来 Native 调用端负责。

保留原 frame 的双手追踪、样本连续性与失追后重建运动基准规则。boomshot_start_compile.cpp 的4组编译期断言通过：无 R3 启动与活动中拒绝按钮重启、10种错误启动、兼容辅助按钮、失追恢复。包含原计划检查，/Zs 编译成功，没有链接或执行程序。报告 reports/boomshot-physical-start.json。

此候选仍使用旧库存/请求成员；invalidate 清空请求和 receipt 拒绝中断后回执的问题尚未修复，不能据启动检查认定完整换弹可接入。下一步迁移共用账本、无限备用弹和迟到回执，再迁移旧测试夹具。生产文件未修改。


## Boomshot 共用换弹账本适配层

新增私有 weapon_boomshot_reload_accounting.h，内部使用 ReloadTransaction<BoomshotIdentity>，提供原换弹启动、认证组件预留、插入、只派发一次、只读已派发请求、回执结算、实际闭锁、中断与同一绑定/原换弹恢复。开始新事务在临时账本校验后提交，不能用失败启动覆盖未完成请求。组件限左肩 verified_carrier、一发；核对绑定、原换弹编号、计划、插入阶段、弹药类型与抓持。BoomshotInventory 增加尾部 infinite_reserve 字段，适配层保留该政策。

boomshot_accounting_compile.cpp 六组编译期检查通过：中断后迟到插入、无限备用弹零计数、错误政策回执保留、拒绝插入后必须新组件、迟到闭锁完成后显式恢复、9种错误组件。同时包含先前计划和物理启动检查。/Zs 仅编译，没有链接/执行或游戏测试。报告 reports/boomshot-accounting-candidate.json。

适配层尚未替换 BoomshotInteraction 的旧库存/请求成员；旧机械实现也尚未使用 infinite_reserve，不能把本报告解释为完整武器已支持无限备用弹或已修复机械中断。下一步迁移实际机械调用端并移除重复状态，保留原射击交接语义，再检查完整流程及旧夹具。Native 接入和实机运动标定仍待完成。


## Boomshot 机械调用端迁移

将声明提取到 weapon_boomshot_types.h，机械类移到 weapon_boomshot_mechanics.h，原头文件作为包含入口保留。迁移前快照 reports/boomshot-before-mechanical-migration.h。机械类不再持有独立 inventory、inventory_request、completion_request 或预留数量；均由 BoomshotReloadAccounting / 共用账本持有。射击请求仍单独交回原版，原弹体数量、后坐和扣弹回执验证保留，不增加射后手动动作。

请求接口改为 take_reload_request 一次派发、dispatched_reload_request 只读查询，旧请求 getter 调用端必须迁移。插入迟到可结算但保持 interrupted；恢复需同绑定、同原换弹、新意图与认证机械状态。闭锁回执新增 result_applied 和 original_ready，缺少任一必要事实不判 ready，保留请求等待核对。拒绝闭锁后的重试使用原物理闭锁时刻。reset 不再清除活动/中断事务，关闭后重置保留账本计数器。

boomshot_mechanics_compile.cpp 的8组机械流程编译期检查通过：有限/无限完整换弹、迟到插入、迟到闭锁、拒绝闭锁的计时保留、换弹后原射击交接、拒绝插入后新组件、3种未确认完成事实。包含先前计划/启动/账本检查。首次编译发现射击请求仍引用删除的库存成员，已改为唯一账本库存，后续完整编译通过。未链接或执行程序；生产头文件与前次记录SHA相同。

仍需迁移旧夹具并检查关闭后新实例绑定、射后再次换弹及射击交接中断恢复。原生调用端、动作认证、Notify/奖励和实机标定未接入；此记录不表示完整武器已可运行。报告 reports/boomshot-mechanical-migration.json。


## Boomshot 射击中断与关闭后重新绑定

发现机械迁移后 after_original_shot 仍只接受 fire_handoff，导致中断后的真实射击结果被拒绝。现加入 take_fire_request 一次派发接口；fire_request 仅可读取，不得用作重复执行凭证。中断取消尚未派发的请求，保留已经派发的请求。迟到的接受/拒绝结果均可核对；接受必须保留扣弹、原弹体数量与后坐证明，拒绝不得同时声称弹体/后坐已发生。错误结果保留请求，不能通过 reset 或恢复绕过。

新增 resume_ready 处理无开放换弹的闲置/射击中断，要求无待处理射击、账本闭合、当前库存一致、原版就绪、机构闭合及有效追踪；换弹恢复仍使用原 resume。迟到射击结算后保持 interrupted，直到明确恢复。没有增加射后手动开闭流程。

boomshot_lifecycle_compile.cpp 的6组编译期检查通过：中断后接受/拒绝结果、未派发取消、错误弹体数量保留及核对、射击后启动换弹、新实例绑定继续请求/组件历史。包含此前完整机械检查，未链接/执行。新实例检查确认旧组件编号被拒绝、新组件可装填且请求编号继续增加。报告 reports/boomshot-lifecycle-candidate.json。旧夹具迁移、重复完整换弹-射击-换弹和当前基线打包仍待完成；Native 调用端必须适配新派发接口，不能直接沿用旧 getter 触发调用。


## Boomshot 重复换弹与旧防护检查迁移

新增 boomshot_repeat_compile.cpp，完整执行编译期换弹→原射击→再次换弹：拒绝旧原换弹编号，使用新组件101，第二轮库存由空仓/备用10变为装填1/备用9，按第二次启动的独立窗口判 ordinary。未靠 reset 重建实例绕过连续使用。

新增 boomshot_legacy_guards_compile.cpp，从生产旧测试迁移8组防护：17种运动证明、4种插入几何、5种库存回执、5种射击回执、6种追踪断点、闭合回程证明、5种计时边界、动画接管/恢复，共44种循环/独立变体。计时保留原游戏暂停时刻、恰好窗口末端、超过末端、长延迟和未知窗口；原动画接管不要求当前双手追踪有效，恢复后仍遵守追踪/原开火许可。/Zs通过，包含之前所有Boomshot检查，未链接/执行。

旧合成容量7/两发组件、insert-only、多数量混合计划、射后手动循环以及重置丢请求等期待与W07或中断安全合同不符，由新拒绝规则/恢复场景替代；不是删掉检查来接受旧行为。旧配置/启动矩阵、引导与守恒检查仍需迁移和核对，尚未生成独立的生产测试替换文件；不得宣称整个旧测试已经通过。报告 reports/boomshot-repeat-and-legacy-guards.json。


## Boomshot 独立检查与当前基线候选包 v1

完成旧配置/启动矩阵迁移（17种配置、18种启动错误），补引导、取放组件、插入前后守恒及满仓/无弹检查。assemble_boomshot_test.py 展开私有检查依赖，生成独立 weapon_boomshot_interaction_test.cpp；不再包含私有cpp文件，仅依赖交付头文件。所有断言在编译时求值，main为空且未执行。reports/boomshot-standalone-test.json 映射旧7个分组到新检查，属于按W07语义替换，不声称每条旧断言逐字保留。

gearsvr-offline-boomshot-v1.zip，SHA256 8ae12dac21031d977f476dd2f158c0952ce8db6a22b1ecccc8fea3aad469ef8b，14条目。六文件补丁 patches/boomshot-current-ledger-v1.patch，SHA256 65450010f471b519e42a5b3763297abecbdacb325419a53d92ae38be990ef106。在当前基线的新私有副本实际应用，候选逐字节一致；应用后的独立Boomshot检查与当前主线共用账本检查均编译通过，生产基线前后哈希一致。ZIP逐条字节核对通过。详细报告 reports/boomshot-current-package.json。

公共账本相对当前主线只增加rebind_closed十行，保留现有rebind_suspended。此包与Longshot/其他武器包共享该文件，不能盲目叠加补丁；必须核对基线合并。包内是离线候选与证据，不是可运行游戏包；Native派发/回执、原Notify和奖励调用、实际机构标定及游戏/头显验证仍未完成。已完成的离线模块不等于全武器准备目标完成。


## Torque Bow W08 取箭启动与单步装填

核对 weapon-cards.md W08 后发现私有旧候选仍允许capacity至10000、任意verified_mechanism步骤，且先按R3才可取箭。现配置限制capacity=1，认证换弹计划只允许一个insert_arrow槽位动作（包含送入到位锁定），不新增手拉弦或额外开闭。未认证换弹计划仍允许仅已有箭的原蓄力功能，不能开始装填。

新增 begin_from_left_shoulder(start,intent,arrow)，先核对箭编号/来源/类型/左手认证，再通过原换弹准入并原子预留同一箭；不要求r3_edge。begin_r3保留兼容入口，和物理取箭、主动取消共用owner意图编号流。错误取箭/原换弹拒绝不会消耗本次时间戳或箭编号，随后正确输入可启动。

bow_physical_reload_compile.cpp 三组新增编译期检查通过：无R3取箭→插入→原完成回执，5种错误配置，6种错误启动后同输入恢复；同时包含既有bow_cancel_compile.cpp检查。首次编译夹具误用Boomshot完成事件名，改为扭力弓实际perfect事件后通过。未运行程序或游戏。旧三步测试夹具尚未迁移，不能沿用其通过结论；共用账本、无限备用弹、中断恢复和Native取消/Notify/奖励接入仍待完成。报告 reports/bow-physical-reload-candidate.json。


## Torque Bow 无限备用弹政策

旧候选启动直接拒绝reserve=0，完成请求无条件--reserve，不保留原无限备用弹语义。私有TorqueBowInventory增加尾部infinite_reserve；启动/再取箭依据该政策放行，结算只在有限政策下减备用数，availability不下溢。原射击、装填完成和其他回执均核对政策一致。total在无限政策下只返回记录的loaded+reserve，不把虚拟手持箭伪计为有限库存守恒证明。

bow_infinite_reload_compile.cpp四组编译期检查通过：零计数无限备用装填、错误政策回执保留、放回再取箭、有限零库存仍拒绝。包含先前W08与取消检查；未链接/运行。报告reports/bow-infinite-policy-candidate.json。当前仍是旧机械类自身持有库存/完成请求，共用账本与中断/拒绝后的显式恢复尚未迁移，不以此局部检查宣称扭力弓完整准备完成。


## Torque Bow 原换弹退出后的恢复入口

当前候选可在中断后结算迟到完成回执，但faulted保持且没有恢复入口。新增recover_exited_reload，仅面向明确认证原换弹已经退出的情形：需同原换弹编号、新意图、原自动补弹抑制已释放、无待派发/已派发请求、无手持/已座未提交箭、库存一致、原武器idle且无pending/R2、当前双手追踪及扳机已松。错误恢复不消耗输入；恢复不重放补弹或奖励，新装填仍需新原换弹编号。

bow_reload_recovery_compile.cpp三组编译期检查通过：中断后迟到成功、迟到拒绝并退出后新编号重启、8种错误恢复条件；包括待处理请求阻止恢复。包含之前W08/无限政策/取消检查。未执行程序或游戏。此入口明确放弃旧原换弹，不替代待实现的共用账本同轮resume；不能把“恢复可用”缩减为只支持重新启动。报告reports/bow-reload-exit-recovery.json。Native必须实际证明原状态退出与抑制释放，布尔参数本身不是运行证据。


## Torque Bow 共用账本适配层

新增私有weapon_torque_bow_reload_accounting.h，以ReloadTransaction持有库存、箭预留和请求。首次取箭在临时账本完成启动与预留后提交；保留原换弹编号及到位锁定时刻，装填结算后使用同一物理时刻提交完成，不增加额外手部步骤。提供一次派发、只读已派发请求、回执结算、中断及同武器/同原换弹显式resume。

bow_accounting_compile.cpp三组编译期检查通过：有限/无限备用下迟到插入及迟到完成，错误政策回执保留；错误武器或原换弹编号不能恢复。包含此前取消、W08、无限与退出恢复检查，未执行程序。报告reports/bow-common-accounting-candidate.json。

适配层尚未取代机械类旧库存/请求，不能作为完整迁移交付。仍需在排队前拒绝早于本轮启动的装箭时刻、补放回/再取和拒绝插入恢复，然后连接机械调用端。原候选将补弹和完成合在一次请求；共用账本明确分离两个原生副作用，未来Native调用端也必须落实，不能仅改类型即可接入。


## Torque Bow 共用账本拒绝与再取箭路径

适配层新增reserve/release，核对左肩来源、匹配箭类型和左手认证，组件编号由共用账本保持递增。seat_and_lock在排队前拒绝早于启动的时刻，拒绝不消耗箭或创建请求。明确拒绝的插入回执结算后清除本次座入状态，可取新箭重试；错误/未知回执不清除状态。中断尚未派发的插入时取消请求及座入标记，已派发请求保留以接收迟到结果。

bow_accounting_retry_compile.cpp四组编译期检查通过：放回后新箭、拒绝插入直接重试、拒绝插入中断后同轮恢复、未派发中断后再取。各组同时核对旧箭拒绝、早于启动时刻不排队、重试请求换编号和原计时不重置（窗口外重试ordinary）。包含全部先前扭力弓检查，未链接/执行。报告reports/bow-accounting-retry.json。机械类仍未改用账本，不能将适配层通过等同实际调用端完成。


## Torque Bow 类型拆分与空闲库存同步

原interaction头改为包含入口，类型移至weapon_torque_bow_types.h，机械类移至weapon_torque_bow_mechanics.h，账本仅包含types以消除循环。迁移前快照reports/bow-before-mechanical-migration.h保留。机械类现可包含账本，但尚未替换旧inventory成员。

账本新增bind_idle及sync_original_idle，供原版射击调用端核实实际结果后同步唯一库存。核对完整武器身份、版本与无限备用政策；活动换弹/待处理请求不能借空闲同步或重新bind覆盖。bow_idle_accounting_compile.cpp两组新增断言通过：原射击空仓同步后开始换弹、待处理派发保持；同时包含此前扭力弓检查。未执行程序。报告reports/bow-idle-accounting-and-types.json。Native必须先验证射击副作用，sync接口自身不证明射击已发生；下一步落实机械调用端全部库存访问和分离插入/完成请求。


## Torque Bow 机械类唯一库存持有者

机械类删除独立TorqueBowInventory inventory_成员，所有库存读取改用accounting_.inventory()。bind通过账本初始化；原版射击/旧合并装填结果在原证明核对后通过sync_original_idle提交，observe版本和退出恢复也走同一持有者。未放宽身份、数量、政策和回执条件。

bow_inventory_owner_compile.cpp两组新增编译期完整装箭完成→原蓄力→原弹体回执流程通过，分别覆盖有限/无限备用，射后库存为0且备用政策保持。包含之前取消、恢复、账本检查，未链接/执行。报告reports/bow-mechanical-inventory-owner.json。

这是库存所有权迁移，尚非完整换弹事务迁移：机械类仍使用旧complete_original_reload合并请求，账本此时仅作为idle库存持有者。下一步必须将装箭/锁定连接reserve/insert和分离完成派发，届时不能继续用sync_original_idle更新活动换弹。旧合并请求必须移除，不能保留为绕过共用账本的备用路径。


## Torque Bow 分离装箭与完成请求的机械接线

机械begin/取箭/放回/完整槽位动作现连接共用账本，旧finish_step不再创建complete_original_reload合并请求。新增take_reload_request、dispatched_reload_request、acknowledge_reload、resume_reload，与蓄力/取消take_request分离。插入原回执通过后使用已保存到位时刻发出完成请求；中断后插入结果可结算，但需同轮显式恢复后再继续完成，不增加手部动作。

bow_split_mechanics_compile.cpp两组正常/迟到插入完整流程通过，含第一次取箭预留、一次派发、插入与完成分离、同原换弹恢复和最终库存；包含既有蓄力取消检查。未链接/执行。报告reports/bow-split-mechanical-migration.json。旧合并请求检查未迁移，之前通过报告是历史结果，不能视为当前完整兼容。

仍有明确迁移缺口：退出旧换弹的recover_exited_reload此前依赖idle同步，活动账本下会拒绝，需补原退出结算；晚到/拒绝完成、原射击后重复使用、无限完整机械流程要验证；旧acknowledge内不可达合并分支待删除。尚不可打包为可接入成品，Native两种副作用消费者也未实现。


## Torque Bow 完成恢复与未派发中断修复

bow_completion_recovery_compile.cpp新增三组机械编译期检查：迟到完成须原版ready证明后显式resume；拒绝完成在长等待后重试仍使用原到位时刻/奖励；未派发插入取消后同轮恢复可取新箭。发现最后一条中账本已取消但机械step仍记为已座入，现中断时在无派发/未装填条件下清除step与箭状态，要求外部认证实际未座入才恢复。删除旧acknowledge中不可达的合并换弹回执分支，保留蓄力/取消处理。修改后包含新分离流程与既有取消检查的/Zs通过，未执行。报告reports/bow-completion-recovery-migration.json。原退出结算及旧夹具迁移仍待完成，未接入Native。


## Torque Bow 原退出账本结算

私有共用账本新增settle_exited，限interrupted、无pending/预留/旧组件、同身份数量政策与原换弹编号、库存版本不倒退、新意图。仅记录owner已认证的原退出/最终供弹闭合状态，不执行补弹或奖励；保留原结果、请求/组件/换弹编号历史。扭力弓recover_exited_reload改用该入口，不再尝试idle同步活动事务。原状态退出与自动填充释放仍需Native真实认证。

bow_settled_exit_compile.cpp两组机械编译检查通过：未装填取消后退出且新编号再取箭、已发完成请求先核对迟到结果再退出且无新请求。保存的账本暂停恢复检查也编译通过；未运行程序。报告reports/bow-settled-exit.json。公共头新增差额需在后续包核对，不能盲目叠加此前Longshot/Boomshot历史包。旧合并夹具、无限/重复射击流程迁移仍待完成。


## Torque Bow 连续装填蓄力射击与失追修复

bow_split_repeat_compile.cpp三组编译期检查通过：有限/无限各两轮取箭→插入回执→完成回执→蓄力→原弹体回执，有限备用3→1，无限保持0；请求编号递增，箭/原换弹编号独立，第二轮未知完美窗口判ordinary而不继承第一轮perfect。第三组确认失追释放未插入预留并可取新箭，插入排队/派发后pending查询始终可见。

检查发现tracking_lost原来只清视觉箭，未释放共用账本预留，造成再取失败；现同步release。has_pending_request原来仅查看蓄力通道，现包含共用装填通道。修改后上述检查及包含的完成恢复/退出恢复/取消检查通过，未链接或执行。报告reports/bow-split-repeat-and-tracking.json。旧合并夹具迁移、独立测试与当前基线包仍待完成，Native实际认证与游戏验证仍未接入。


## Torque Bow 旧蓄力发射检查迁移

从生产weapon_torque_bow_interaction_test.cpp迁移五组至bow_legacy_charge_compile.cpp：original_charge_release_and_actual_projectile、loss_never_generates_end_fire、quick_tap_and_expiry、sequence_clock_and_dispatch_gates、contradictory_receipts_and_identity_change_fail_closed的蓄力/射击部分。改为constexpr夹具与编译期失败断言，配置采用W08单箭槽位；删除不再使用的旧三步open/seat辅助方法。五组完整编译期求值通过，保留延迟EndFire、原疲劳发射、快速松键缓冲、失追不得产生EndFire、身份/时钟/边沿重放和矛盾结果防护，未执行程序。

最后一组原bad_reload尾段依赖旧合并请求，明确延期至装填回执迁移，不计入此次通过范围。其他四组配置/装填检查仍待迁移。报告reports/bow-legacy-charge-migration.json记录生产源与候选指纹；当前结果不证明全部旧测试或Native行为完成。


## Torque Bow 旧装填检查的W08分离语义迁移

新增bow_legacy_reload_compile.cpp五组编译期检查，覆盖原配置/引导守恒/奖励时间/装填防护和此前延期的bad_reload尾段。9种配置错误、5种原时钟边界（含暂停、末端、超时和未知窗口）、5种错误装填回执、10种槽位动作证明均被拒绝或正确分类；取放箭、旧动作不能装新箭、组件引导和有限库存守恒保留。旧额外开闭/最后机构步骤由W08固定单箭槽位取代，不再接受旧手动机构顺序。

上述五组与包含的分离恢复、连续两轮、取消检查通过/Zs，未链接/执行。报告reports/bow-legacy-reload-migration.json给出原分组到新检查的映射。这是按最终语义迁移的覆盖，不声称旧每条assert逐字保留；尚需汇集独立完整测试文件、核对Native两次调用合同及共享退出差额，再打当前基线包。


## Torque Bow 当前基线离线候选包 v1

assemble_bow_test.py汇集分离正常/迟到插入、完成恢复、退出结算、两轮有限/无限装填蓄力原射击、失追预留，以及迁移后的旧蓄力/装填防护，生成独立weapon_torque_bow_interaction_test.cpp。私有cpp包含全部展开，原头文件作为唯一代码入口；编译通过，main为空且未执行。覆盖是按W08和分离副作用语义映射，不逐字保留不适用的旧三步或合并请求期待。

gearsvr-offline-torque-bow-v1.zip，SHA256 0532a73cad2162a5f35eab7096df009eac834a2471b61f6686ecb57e8025dd21，14条目。补丁patches/torque_bow-current-ledger-v1.patch，SHA256 f0442ebdf900ed52afdbbc3b12e209c8ce3c4372824e70eba32dc174fa2eb5a9，6文件。在当前生产源码的新私有副本实际应用，候选逐字节一致；应用后的独立扭力弓检查和新复制的当前生产共用账本检查均编译通过，生产文件前后哈希一致，ZIP逐条字节验证。报告reports/torque_bow-current-package.json。

公共账本相对当前主线增加rebind_closed与settle_exited，保留rebind_suspended；不能直接叠加Longshot/Boomshot历史补丁。此包仍为离线代码准备：Native必须将插入补弹和完成奖励分成正确原调用、认证原状态退出及最终机构状态，并落实非发射Charge取消组合；实际箭槽位标定、原脚本Notify副作用、游戏/头显验证未完成。不得作为可运行包或直接接入授权。后续继续Hammer of Dawn/Troika及其他原生接入口准备。


## Hammer of Dawn W09 无消耗资源合同

核对武器卡W09，旧私有候选仍允许original_managed资源池、认证数量变动，生产旧测试默认777单位。现拒绝消耗资源配置，bind与后续资源认证均要求units=0，保留原Manager/target/phase/conditions门槛及unverified阻止使用规则。没有新增电池、装填或补充能量动作。

hod_resource_contract_compile.cpp三组编译期检查通过：认证无消耗绑定、三种虚构非零资源配置拒绝、未知政策仅零计数绑定。未运行。旧777单位及消耗回执测试尚未迁移，不能沿用它们的通过结果；瞄准/开火拥有权与EndFire停止链仍待核对、原生实际调用未认证。报告reports/hod-resource-contract.json。


## Hammer of Dawn 右手独立准入

旧observe只有left_tracked才从await_tracking交回VR，导致W09右手握持与合法目标也无法右扳机启动。现有效右手武器姿态即可恢复VR，左手aim入口仍要求左手追踪；没有取消原Manager/target/resource/phase门槛。纯方法改constexpr，射线有限/单位长度校验改为有界比较供编译期流程求值。

hod_right_hand_compile.cpp两组编译期sample→edge流程通过：左手未追踪但右手合法目标可请求原开火，目标无效则拒绝。包含无消耗合同检查，未执行程序。报告reports/hod-right-hand-admission.json。实际停止/InvalidTarget回调链、旧资源夹具迁移和射线边界检查仍待完成，不以启动通过推断停止可用。


## Hammer of Dawn 停止确认所有权修复

发现accepted release_original_fire先清owns_fire，再只核对输入位，忽略原phase仍designating/warming/illuminating。现先确认输入已松且不在活动阶段，否则保留owns_fire、标记interrupted并请求非发射取消；正常cooldown允许释放。不是仅凭EndFire返回成功宣称光束停止。

hod_stop_compile.cpp四组编译期流程通过：正常松键、松键成功但仍照射、目标失效、右手失追。后两者完整核对一次取消派发、回执cooldown后才释放所有权。包含右手准入与零资源合同检查，未执行程序。报告reports/hod-stop-ownership.json。实际EndFire/NotifyInvalidTarget/NotifyStopFire/激光关闭必须由Native实现并认证，纯状态回执不证明游戏实际停止。待开始/更新请求与取消的竞态、拒绝停止回执和旧夹具仍需后续处理。


## Hammer of Dawn 请求竞态与射线边界

hod_races_compile.cpp新增四组编译期检查：已派发开始请求失追后先结算再取消；更新目标尚未回执时松键等待，回执后才派发EndFire；取消拒绝保留owns_fire且新编号重试，确认cooldown后释放；六种射线边界（有效巨大有限起点、零方向、巨大方向、无限起点、NaN方向、非单位方向）。射线先拒绝非有限/不可能的方向分量再算平方和，避免巨大输入溢出。包含此前停止/准入/资源合同检查，/Zs通过，未执行。报告reports/hod-request-races-and-ray.json。尚未迁移旧资源消耗夹具，也未认证游戏Native调用；纯请求/回执场景不证明实际光束/声音已停止。


## Hammer of Dawn 旧八组检查迁移

hod_legacy_compile.cpp将生产旧八组改为constexpr夹具和CHECK失败阻止常量求值；去除运行main。默认original_managed/777改为no_consumable/0，真实效果回执不扣资源；非法754/700等非零变动仍拒绝，认证布尔不能放行虚构能量池。快照变动使用显式1而非对0减一。首次编译仅动画恢复组失败：旧期待要求左手追踪后恢复，现按已确定右手独立合同更新为右手有效立即hands_restored，下一帧observed。修改后八组全部通过。

保留原门槛/姿态、瞄准与原效果、designation票据、失追/迟到、动画/身份、回执/R3/外来输入及取消边界。报告reports/hod-legacy-migration.json记录源码与候选指纹。未链接/执行；检查位置数不等于独立场景数量。仍需汇集新停止/竞态与旧检查为独立测试、核对Native调用合同及打包。


## Hammer of Dawn 当前基线离线包 v1

汇集hod_races_compile及其停止/右手/资源依赖、迁移的八组旧检查为独立weapon_hammer_of_dawn_interaction_test.cpp。未包含私有cpp，main为空，编译通过。gearsvr-offline-hod-v1.zip，SHA256 14d75ef16735938939738cfd79ddca58a1ba1bcfe6eb2a635ba1302c9bdeb363，9条目；两文件补丁patches/hod-current-ledger-v1.patch，SHA256 deee3f177962419fc4c07ad6b61f62d7814e5051c4f479b921207ceb3fff8f80。在当前源码新私有副本实际应用、候选逐字节一致、应用后独立检查编译通过、生产哈希前后不变，ZIP逐条字节核对。报告reports/hod-current-package.json。

包内native-binding-contract.json依据现有W09/B06文档列出BeginFire、GetTraceImpact/Tick/UpdateFX和EndFire停止通知链，明确同武器目标查询/输入所有权/停止phase回执要求。此次未重新认证准确脚本或Native ABI：worktree无脚本缓存，J盘缓存初步文件名检索只找到基类部分导出，未把文档映射当成可调用地址。实际Native调用、目标查询和光束/声音停止仍未完成，包仅离线决策/测试准备，无公共账本改动。下一步继续Troika，不重复将局部包当全目标完成。


## Troika W10 禁止手动换弹/机构计划

核对W10发现旧候选允许manual_mechanical_plan_verified、任意步骤及automatic_completion_suspended。现config要求manual_reload_absent（计划/数量/标志和所有潜在步骤字段为空），begin_mechanical在有效身份下只返回unsupported，不启动或暂停原自动Reloading。保留原炮位/双握把/射击/自动服务观察逻辑，未改原游戏计时或Loader行为。

troika_reload_contract_compile.cpp两组编译期合同检查通过：空计划可接受，6种残留手动配置拒绝。仅覆盖计划合同，未证明实际炮位、自动换弹或原动画。生产旧manual=true夹具尚待迁移，不能沿用通过结果。报告reports/troika-no-manual-reload.json；下一步核对双握把停止和退出所有权，实际原生绑定仍未完成。


## Troika 退出动画与驾驶权合同

源码核对drive_allowed依赖both_handles，fire_allowed依赖drive_allowed；reconcile先处理owned-fire stop再unmount。animation_finished原来在有transition token时只检查phase，遗漏驾驶权释放/获得。新增实际调用的troika_transition_completed：mounted须driver_is_this_pawn，detached须相反，且fire_input_down/animation_owns_hands均false。未完成状态不能清掉transition并恢复手部。

troika_transition_contract_compile.cpp两组编译期合同检查通过，覆盖正常进出与7种未完成状态；包括前次无手动换弹检查。仅为已接调用端谓词的证明，完整几何/炮位流程未求值或执行。报告reports/troika-transition-ownership.json。旧退出夹具若保留driver=true需按原实际回执迁移，不能放宽此条件来让旧测试通过。Native占用/退出、角色观察基准恢复仍需认证。


## Troika 旧夹具源码迁移与调用端缺口

私有weapon_troika_interaction_test.cpp将旧手动机构组改为original_automatic_reloading，删除service/motion辅助；场景覆盖服务期间禁止VR转向/开火、手动入口不支持、带认证原事件的补弹观察、恢复后需新扳机。原animation辅助已按detached设置driver=false，无需虚改。troika_legacy_source_compile.cpp使用/Zs编译通过，未求值或执行运行时CHECK，不可写为场景通过。报告reports/troika-legacy-source-migration.json。

定向搜索tools/native-capture、openxr-probe、playground的TroikaInteraction/头文件引用仅找到测试CMake目标，未发现实际消费者。现有TroikaPose接受外部yaw_rad/pitch_rad和认证位，仅验证限制并转发，不负责从双手求炮塔角度；不能把该纯类当作已经实现握把驱动。下一步需补已标定枢轴/握把到角度适配，再明确Native claim/drive/stop/unclaim与身体/视角恢复；自动Reloading的Loader时长/粒子仍由原版绑定，不新增计时器。


## Troika 双握把限位角度拟合候选

新增weapon_troika_grip_solver.h：输入已认证固定枢轴坐标系内米制双手位置与局部握把偏移，规范约定+Z yaw后+Y pitch，按原限制范围81点粗搜再20次局部细化，逐手误差满足标定阈值才提交角度/帧票据。绑定/标定/双手追踪与Grip/坐标认证缺失、枢轴相对两握把共线等退化几何拒绝且不改输出。不使用HMD、计时或发射输入；不会把当前枪口角度强行写作自由武器方向。

troika_grip_solver_compile.cpp四组编译期检查通过：零位、非网格角度(0.37,-0.21)、上限(1,0.6)、7种拒绝。合成样本部分由同旋转模型生成，因此只证明内部一致性，不是引擎几何或实机标定；粗搜加局部细化也不是全局唯一解证明。未执行程序。报告reports/troika-grip-solver-candidate.json。下一步独立角度夹具、输出与TroikaPose的绑定/标定/帧连接及Native原轴映射；实际消费者尚未实现。


## Troika 求解到姿态连接与独立角度检查

新增weapon_troika_solver_bridge.h，从同一TroikaPose双手世界位置逆变换至已认证的未转动枢轴基座，核对binding、pose mapping/标定、原限位、局部握把偏移、双手追踪/Grip和原约束认证；成功仅更新yaw/pitch，失败不改输出。固定基座不是当前转动炮塔或HMD姿态，不生成枪口或原认证位。

troika_solver_bridge_compile.cpp三组编译期检查通过，使用直接写出的90° yaw坐标及30° pitch坐标（不调用求解器rotate造样），加平移/90°旋转基座逆变换和映射错配保持旧角度。/Zs未执行。报告reports/troika-solver-bridge.json。此连接函数尚无Native调用者；实际原轴/符号/顺序及固定基座仍需认证。还需扩展失败矩阵、实际瞄准消费接线和完整生命周期检查，不以数学夹具证明游戏可用。


## Troika 私有手部消费入口与失败矩阵

新增observe_troika_hand_drive，先核对本轮identity/binding；mounted且无原动画/自动服务时从双手求解后model.observe，失败调用新增suspend_hand_drive。后者在新鲜stamp下consume并invalidate_tracking/reconcile，撤销驱动、清未派发瞄准，保留已派发开火待回执并优先处理停火；不把几何失败永久标记terminal。进入/退出动画与自动服务绕过求解，继续原状态门槛。

桥接检查新增12种错配/缺失/几何误差，全部编译期拒绝并保留旧角度；此前独立角度检查通过。旧炮位测试源码亦/Zs通过，均未执行。报告reports/troika-hand-drive-consumer.json。实际消费函数已编译，但完整炮位与停火路径尚未运行或常量求值，不能用源码编译冒充流程通过。Native校准基座/原轴、原claim/stop/unclaim及视角恢复仍待认证。


## Troika 完整编译期流程与实际连接入口

纯几何/状态方法改为constexpr，有限值/单位四元数与射线在平方前加界限；握把余弦阈值复用私有solver的有界级数（输入0..pi/2）。旧迁移夹具的sqrt(0.5)改显式常量，abs改纯辅助，七组检查完成常量求值通过。此前source compile证据仍保留，本报告才支持这些合成状态场景通过。

另troika_consumer_flow_compile.cpp实际调用observe_troika_hand_drive：正常已标定姿态→原开火获得所有权→左Grip松开→停止请求→回执释放所有权→禁止继续瞄准，编译期通过。不是伪model接口测试，未执行程序。独立角度/失败矩阵再次通过。报告reports/troika-full-compiler-flows.json。余弦替代的阈值数值边界仍需专项核对，真实原轴/基座标定、原引擎调用和头显行为未验证。


## Troika 数值边界与当前基线候选包 v1

数值检查采用Decimal90位/70项参考余弦，0..pi/2共13点，与C++有界余弦误差不超过3e-15；握把距离阈值两侧、显式60度四元数角阈值两侧及q/-q等价检查通过。参考输入reports/troika-cosine-reference-input.json，检查troika_numeric_edges_compile.cpp。未执行C++程序，参考计算脚本只生成数值。

独立weapon_troika_interaction_test.cpp汇集七组迁移流程、实际消费停火、独立角度、失败矩阵、求解器、驾驶权/无手动换弹和数值断言；无私有cpp包含，main为空。gearsvr-offline-troika-v1.zip，SHA256 b042692db7f6d44f4ecfd3abe9e1faf9e694c1bc4ce4e995f57ceb0e33b0ca5e，11条目；四文件补丁patches/troika-current-ledger-v1.patch，SHA256 ea4c8f1ded30cc0ffb06f7dff203de8b09a8fec7a3d86d7a5ef1a2045f9a608a。在当前源码新私有副本应用并逐字节匹配，应用后独立检查编译通过，生产前后哈希一致，ZIP逐条字节核对。报告reports/troika-current-package.json。

包内Native合同要求真实Claim/DriverEnter、固定基座/原轴/限位、原UpdateTurret及枪口查询、停火后Unclaim/DriverLeave、驾驶权清理后自由手/视角恢复，并保留原Loader/自动Reloading；准确ABI、实际标定与引擎消费者仍未实现，不代表游戏可用。后续回到全范围缺口核对，避免把多个离线包当作全武器/掩体/模型/原生准备完成。


## 全范围覆盖复核与下一实质缺口

重新读取并核对Longshot、Boomshot、Torque Bow、Hammer of Dawn、Troika五份归档SHA，全部与生成报告一致。仅证明离线归档内容未变，不证明累计补丁可叠加或原生接入完成。status总表P1/P2的早期未迁移描述已补充后续证据，保留每枪报告的实际限制。

对当前tools/native-capture的h/cpp/inl定向搜索ThrowGrenade/GetGrenadeTossVelocity/GetPhysicalFireStartLoc无命名命中；搜索不排除间接入口，但不能据现有纯轨迹计算声称投掷已接入。下一优先是Frag/Smoke原ThrowGrenade单次作用域：精确武器/类型/线程/调用票据，出手位置/速度/Init方向只在该作用域生效，一次生成许可，派发后未知结果保留；原动画通知不得制造第二枚，原生成成功与扣弹部分失败不能伪称原子成功。

报告reports/full-scope-coverage-audit.json记录搜索边界与归档逐项证据。全12武器Native调用、掩体query可见碰撞/受击几何、模型实际标定/原生资源生命周期仍未完成；继续原目标，不以归档数量替代。


## 手雷原投掷调用作用域候选

新增grenade_native_launch_scope.h，一次派发请求对应不可复制且不可重置的gate。Native owner须先认证原请求executable_at、冻结身体坐标/基底速度和确切原调用；作用域按world/代际/pawn/weapon/代际/projectile class/binding/request/dispatch/thread/invocation匹配。出生位置/速度只在当前调用提供，Init方向仅给记录的原弹体，一次spawn许可必须在真实生成前拦截，finish后不再覆盖。

finish只核对原观测：一弹体、扣一枚、一次通知才complete；null spawn且无扣弹/通知才spawn_failed；未返回、已生成未扣弹、重复/缺失通知或错误弹体类为unknown并保留弹体身份，不重试或自行退款。grenade_native_scope_compile.cpp四组编译期检查通过，未执行。报告reports/grenade-native-launch-scope.json。

这是私有准入/结算代码，不是已安装hook。每请求gate必须由owner保留，销毁重建不能作为防重依据；原调用结束后迟到的AnimNotify仍需单独的原准备编号/通知映射，不能凭scope内单次许可声称已阻止全部二次生成。Frag/Smoke请求桥接、实际Native观测和部分生成/扣弹回执仍待落实。


## 手雷迟到投掷动画通知接管表

新增grenade_throw_notify_owner.h，owner在手势投掷派发前按真实world/代际/pawn/weapon/代际/projectile class/原准备编号claim；同准备不能换request/dispatch编号再次claim。迟到animation_throw仅命中已接管准备时suppress，spawn_completed始终forward原通知，未接管或其他类型/生命周期维持原逻辑。结果未知/失败也保留tombstone，不由动画自动重试。

grenade_notify_owner_compile.cpp三组编译期检查通过，含Frag/Smoke/武器/代际错配和容量满不覆盖旧记录；包含前次作用域检查，未执行。报告reports/grenade-notify-ownership.json。当前固定容量（默认32）是尚待解决的生命周期约束：满后拒绝新接管，不能称无限连续投掷就绪。仅认证世界销毁可清记录，后续需真实原通知排空/事件fence或安全压缩证明。准确原准备编号来源、Frag/Smoke请求桥接及Native拦截均未实现，不能以表逻辑通过声称游戏已防二次生成。


## Frag/Smoke 原请求到调用作用域桥接

新增grenade_native_request_bridge.h，接受已认证派发的BoloRequest/SmokeRequest及当前owner context，复用原executable_at核对时间/完整身份/绑定/运动和参考基准/cover commit，另核对config/原入口/弹体/准备编号和恰好扣一枚。Smoke另核对自己的ammo/emitter类型。通知claim成功后建立不可复用作用域，原launch直接复制，不再转换坐标或叠加基底速度。

作用域key补齐controller、XR session、space/input/profile generation，避免跨输入会话调用命中。grenade_request_bridge_compile.cpp三组通过：Frag成功、Smoke错误emitter拒绝后正确可启动、7种错误原请求不占用gate/通知表且不改输出。包括此前作用域/通知检查，未执行程序。报告reports/grenade-native-request-bridge.json。

本轮夹具是填好的合成请求，尚非交互类真实take_request端到端；派发认证、原类/入口、launch/body proof仍需Native owner，context布尔不等于证据。原准备编号与通知回收fence、实际生成/扣弹回执和partial unknown映射仍待完成。


## Frag/Smoke 实际派发接口源码消费夹具

新增grenade_frag_dispatch_source_compile.cpp与grenade_smoke_dispatch_source_compile.cpp，复用现有交互Fixture的取用/准备/轨迹/真实take_request方法，将结果送入各自桥接、作用域、单次生成、按实际弹体取Init方向、完成/未知结果路由与迟到通知表。成功才交原交互acknowledge；已生成未确认扣弹不造成功回执，保留原pending且不能take第二次。

两单元/Zs源码/类型编译通过，夹具含原轨迹三角函数拟合，未运行或常量求值，不能声明端到端动作通过。报告reports/grenade-dispatch-consumer-source.json。最关键下一问题是原准备编号与AnimNotify关联：现有receipt中的编号是adapter证书，不证明游戏里有同名字段；需查保存RE/原通知参数与状态，不能虚构偏移或凭动画名归属。通知安全回收、部分结果核对及真实Native回调仍未完成。


## 原投掷通知证据修正：必须保留通知外层

按RE流程先查KB AnimNotifyThrow，读取六份保存脚本并记录SHA；两份通知导出与artifact2 KB SHA一致。基类AnimNotifyThrow无显式准备编号参数，调用顺序为声音→ThrowGrenade→GrenadeThrownNotification；Frag覆盖还super后角色SoundGroup声音43。完成通知含DetachWeapon、震动、声音和弹体自定义动画。故原准备serial不能当游戏字段；只手调ThrowGrenade并压掉后续整个通知会遗漏原副作用。

方案修正为手势释放时执行一次已认证动态AnimNotifyThrow外层（包含Frag override），其嵌套ThrowGrenade内消费位置/速度/Init作用域；后续同次动画通知整体去重。桥接新增original_notify_envelope_verified准入。脚本对null spawn仍调用完成通知，因此scope失败分类改为无弹体/无扣弹/一次完成通知，零通知属unknown。桥接编译期与Frag/Smoke派发源码编译均通过；未执行。

报告reports/grenade-notify-source-evidence.json保存路径、SHA和边界。导出有@NULL/未解析tokens，不能据此直接认证ABI；ThrowingGrenade.BeginState/EndState仅提供状态开始/清理线索，没有队列已排空证明。原notify归属与容量回收仍需动画节点/状态发生身份，禁止凭EndState清表或捏造serial。


## 投掷通知来源新证据：当前武器转发与AnimSeqInstigator线索

KB定位WarAnimNotify_ThrowGrenade.Notify，artifact2 object13366导出SHA已核对。保存脚本从Pawn(Owner).Weapon取得当前手雷，检查GrenadeThrownTimer未激活后调用AnimNotifyThrow，再设置0.5计时；Timer函数体为空。该防重复窗口不是队列排空证明，也不能当引信。上一轮只保留内层武器AnimNotifyThrow的方案仍不足：必须保留外层debounce语义或经认证的等价调用，不能默认手调内层完整覆盖原行为。

新风险是切换手雷后旧通知可能转发给当前武器；不能只凭当前weapon/adapter准备号归属。对象清单另记录Notify.AnimSeqInstigator(object13364)、Owner(13365)，被损坏的脚本签名遗漏，给出有界元数据追踪入口。属性存在不证明其Parm flags、准确类型/偏移或播放代际，仍禁止猜ABI。AnimNotify_Script的NotifyName仅事件名，不足以区分播放实例。

报告reports/grenade-notify-dispatch-origin.json保存五份文件指纹与推断边界。下一步查保留的包属性/函数布局及通知Native调用者，恢复真实来源，再解决安全回收；本轮没有安装hook、运行游戏或重做同一无结果搜索。


## 原通知参数从包元数据恢复

使用现有Eliot.UELib只读解析WarfareGame.u解压副本，先核对SHA256=91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13，定向反序列化13362..13366五对象。恢复Owner(object13365)为Actor、AnimSeqInstigator(13364)为AnimNodeSequence，两者PropertyFlags均0x80:Parm；P和Grenade无Parm，是Pawn/Weap_Grenade局部变量。原脚本文本的空Notify()声明遗漏了这两个参数，不能继续当成无参事件。

原始解析报告reports/grenade-notify-property-reader.json；结论/限制reports/grenade-notify-parameters-resolved.json，记录库指纹。未启动游戏或测试程序；运行的是已有离线包分析库。所有cooked ElementSize=0，不能直接推运行时参数offset/ParmSize或堆布局。AnimNodeSequence提供实际来源对象线索，但可复用节点不等于唯一播放发生；下步追节点生命周期/原通知分派，仍不得据状态退出回收旧防重记录。


## AnimNodeSequence 生命周期元数据与Native注册线索

对SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510的Engine.u用既有UELib只读解析11对象，恢复AnimSeq、PreviousTime、CurrentTime、bIsIssuingNotifies、bNoNotifies、bLooping、bPlaying；SetAnim/PlayAnim/StopAnim/SetPosition为Native/Public，PlayAnim另HasOptionalParms。原始reports/grenade-node-metadata.json。类文本只有空声明，先前没有展示这些已存在字段。

KB原生名搜索未得函数，只得历史快照UTF16注册literal intUAnimNodeSequenceexecPlayAnim，地址0x141726210、来源SHA c5deb08128dea890e2606b7bb6bf30eb3e1045c19b3f477e0bebf6c7fd15231c；refs定向无命中。该地址是数据字符串，绝不能当exec代码入口。报告reports/grenade-node-lifecycle-evidence.json。

未找到已认证播放代际/队列排空依据；采样CurrentTime回绕、bPlaying=false或bIsIssuingNotifies=false都不能证明旧回调不会到达。bNoNotifies还会影响其他通知，不做全关捷径。下步聚焦Native注册与真实通知分派，不重复从无参数脚本文本猜ABI。本轮只读包分析，无游戏/测试执行。


## Native注册定位的数据库覆盖与工具失败证据

复制full-native/snapshot.i64至私有ida-grenade-notify-20261029，前后原库SHA324248ac7cd0388a05907c29436fccf804eb82e5cb8433190806342c1cfcbb26不变。MCP打开失败：现有IDA插件缺ida_nexus，worker退出1；实例列表为空，无持有lease，不安装升级。按恢复说明用已有idat和仅子进程IDA_NEXUS_IDALIB_ONLY=1执行有界IDAPython探针，写出xref_probe.json但进程仍exit1，保留日志，不记为成功完整运行。

探针显示数据库仅包含0x140001000..0x14131a94f代码段；execPlayAnim字符串0x141726210位于未映射数据段，因此空Xrefs不能证明无注册引用。报告reports/grenade-native-registration-probe.json。下一步在新私有分析副本映射已hash认证的历史数据段再定位注册；不混淆原磁盘EXE、历史快照与当前游戏，也不重复同一空搜索。


## 私有历史代码/数据映射探针完成

已有idat+IDAPython路径可用，修正退出为ida_pro.qexit；第一次数据写入把put_bytes无布尔返回当失败，保留failure-first.txt，第二次改为实际读回SHA核对。映射0x14131b000保存数据段，代码全段与原历史快照SHA一致，数据读回SHA亦一致，确认目标UTF16字符串内容；定向探针exit0且未保存数据库。原full-native数据库SHA前后一致，未改插件/安装工具。

对完整字符串精确地址的RIP相对LEA模式、当前数据段绝对指针搜索均无结果。此范围不包含其他相对注册表/短名/动态查找，不能证明无原函数。报告reports/grenade-registration-mapped-probe.json，原输出ida-grenade-registration-data-v1/registration.json。下一步改变方向查PlayAnim短名/AnimNodeSequence间接注册，不重复同一完整字符串扫描或假设字符串附近是函数。


## Native入口直接定位路径排除与绑定方向

新short probe在私有历史映射中正常exit0，完整短名PlayAnim/SetAnim/SetPosition/StopAnim未见独立UTF16词，仅UAnimNodeSequence类名；相关直接LEA及各保存数据段绝对指针未命中。购买版Steam EXE只读PE导出31项，无目标动画符号，磁盘SHA为支持基线bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4，不拿磁盘字节当历史内存字节。

既有UELib对认证Engine.u定向读取6478/6482/6483/6486，四个Native函数NativeToken全部0，无反序列化错误。不能用GNatives[0]或包对象index当代码地址。reports/grenade-node-native-tokens.json与grenade-retail-export-probe.json保存原结果；汇总reports/grenade-native-lookup-route-audit.json。下一方向是UFunction实际Native绑定/查找路径与现有resolver，不重复短名/导出/指令token猜测。尚未得到函数RVA、播放发生或通知回收fence，本轮静态调查不等于原生接入。


## 复用现有UFunction入口读取链

当前analyze_native_camera按真实类字段找到原生函数后读UFunction+0x114并验证可执行段；cover_call_config/wire对脚本读取同字段但要求base+0x1d6ca0及script/locals证明，crosshair_live有入口重读约束，native_metadata_contract有版本/chunks认证。五份源码SHA/位置见reports/grenade-existing-function-entry-route.json。resolve_runtime_function.py实为异常表范围工具，不能因名字当UFunction绑定器。

下一方向为现有身份/反射读取链定向特化：AnimNodeSequence四个原生入口快照，以及WarAnimNotify_ThrowGrenade.Notify的Owner/AnimSeqInstigator参数与脚本派发；核对运行时参数布局、对象生命周期和稳定重读。+0x114仅在当前其他功能源码有依据，本轮未读到手雷实时对象，准确地址/ABI未认证。无游戏/进程读取；不再沿注册名字/token0猜地址。


## 原通知参数快照合同落实

将已有reward快照采集核心提取为capture_parameter_snapshot(validator)，原capture_reward_parameters接口与规则保留。新增grenade_notify_parameter_snapshot.h，精确核对声明owner/function、两个Parm对象参数Owner/AnimSeqInstigator及Actor/AnimNodeSequence类身份，限制8字节标量、不重叠、局部/参数边界和字段链；复用双次完整读取、header重读与owner current检查，失败不发布半结果。

grenade_notify_snapshot_compile.cpp两组（正常及7种错误）常量求值通过；原reload_reward_snapshot及实际reward_native_reader编译也通过。测试0/8偏移与16字节仅合成夹具，不代表运行时ABI。报告reports/grenade-notify-snapshot-candidate.json。Native读取回调尚未实现，不以接口复用当真实元数据认证；现有奖励归档早于此次通用核心提取，未来需按基线重合并而非直接覆盖。


### 2026-10-29：恢复中断后的 Notify 读取与调用帧候选

核对中断前四项编译报告均 exit_code=0。Notify 参数读取已接到私有 reward_parameters::Reader，复用现有对象表、名字表、Function/Class/Package 身份与接收者继承链检查。新增 grenade_notify_call_frame.h：依据经过双次捕获的参数元数据读取 Owner 与 AnimSeqInstigator，双次核对指针与对象身份，并在发布参数前重查元数据和调用租约。输出失败不发布半成品。正常路径与八类失败路径通过 static_assert；原读取器继续通过 /W4 /WX /Zs。未链接、未运行测试程序。

这是离线候选，不是实际 Native 回调已完成。Authorize 与 ObjectTarget 的真实实现、PropertyClass 读取位置、原始回调参数块认证、入口代码认证、动画播放代际和通知排空证据仍待补齐。GrenadeNotifyFrame 是候选回执合同，不能当作原版 FFrame 的布局。没有生产源码、构建配置、启动器或 Git 索引修改，没有游戏/XR/模拟器运行。报告：analysis/offline-preimplementation-20261028/reports/grenade-notify-reader-frame-candidate.json。


### 2026-10-29：具体 FrameReader 候选

补齐私有 FrameReader：在经过所有者认证的参数块范围内，使用现有 meta::read_at 读取指针；复用对象表、名字身份和有界 superclass 遍历校验投掷者/动画节点实例。读取前后都重查认证，回执及 Header 必须与绑定匹配。实际模板组合、调用帧 constexpr 场景和既有 reward 读取器三项 /W4 /WX /Zs 检查通过，未链接或运行程序。仅私有 staging 修改。

限定 KB 与源码搜索未发现支持 ObjectProperty.PropertyClass 目标类偏移的证据；anchor_observer 中的 property_class 是属性对象本身的 UObject 类，不是 ObjectProperty 允许的目标类，不能混用。本轮不猜测此偏移。原版回调入口/线程/调用寿命认证和动画代际归属仍未完成。报告：analysis/offline-preimplementation-20261028/reports/grenade-notify-native-frame-reader.json。


### 2026-10-29：ObjectProperty 字符串引用的实际指令核对

对历史快照六节重验哈希，新增两处 ObjectProperty 文字引用候选，并在已有私有 IDA 副本中核对指令。UTF16 引用 0x14035605e 位于连续文字及固定编号初始化序列，符合预定义名字注册，未证明目标类读取；ASCII 引用 0x140ca31b4 前有属性 offset+0x94 和 virtual+0x290，后有文字比较及条件 virtual+0x378，属于属性值/格式分派线索，不能把任何虚表槽猜成 PropertyClass getter。两处均不是已经定位好的目标类偏移。

第一次相对路径 IDA 调用初始化失败；改绝对数据库、脚本和日志路径后同一已安装 IDA 正常结束 exit0。仅使用现有私有数据库，不保存原始数据库，不启动游戏，不更改工具配置。报告 object-property-route-assessment.json 保留观察、推断和限制，避免后续重复走名字注册线索。


### 2026-10-29：名字注册路线收敛与八包组合审计

核验已有两个 helper 伪代码 SHA，观察到文字条目构造与按条目索引写入名字表，支持前轮名字注册判断。缓存伪代码存在错误 ABI 变量与 raw-loader 地址偏差，不能直接复制 MEMORY 地址或据此补 PropertyClass 偏移。五个历史非执行节精确 RTTI 形式搜索无结果，仅是限定负证据；目标类布局仍未解决。

新增 audit_candidate_composition.py，只读核验八个最新候选 ZIP（CRC 与 SHA256）、candidate/staging 下的 100 个文件，与当前 staging 和生产文件分别计算哈希。跨包冲突涉及 3 个文件；归档与当前 staging 的有意义差异共 12 个文件（已排除换行/BOM 差异），见 candidate-composition-audit.json。完整文本 diff 按版本保存，索引 candidate-composition-differences.json。公共换弹 transaction 当前与 TorqueBow 包一致，相比 Longshot 包增加 settle_exited；旧 v6 缺少更多后续修复。此次仅识别与保存差异，未完成语义合并，未应用任何包或补丁。后续统一候选必须保留各项后续修复，不能按 ZIP 顺序覆盖文件。


### 2026-10-29：统一离线源码快照 v1 与公共接口组合检查

核对公共 model/geometry 差异：保留 magazine_close 的当前名称和值，linked_closure 仅作兼容别名；无限备用弹药策略纳入库存一致性校验，有限 reserve 数值不伪造；公共 transaction 保留 settle_exited 等后续处理。新增 all_weapon_shared_headers_compile.cpp，将十二种武器主要候选、机械模块、手雷请求/通知及 reward 快照放进同一编译单元，发现公共声明/宏/类型冲突。本轮组合检查及 model、无限弹药、Gnasher accounting、Longshot legacy、Bow standalone、Boomshot standalone 共七项 /W4 /WX /Zs 通过。

新增可重现 build_unified_offline_snapshot.py，冻结当前 staging 为 unified-source-v1（183 个源码/检查文件），并从隔离快照路径重新编译全武器头文件组合，exit0。ZIP 内逐文件 SHA256 与 CRC 回读通过。归档 gearsvr-offline-unified-source-v1.zip，SHA256 64ff59e77924cc3d19a620e7426e4d9fcd2f254e176f5ddcebbeeac16df25571。它是当前私有源码快照，不能作为游戏安装包或整体覆盖生产 tools；十二项差异的完整语义复核、真实 Native 回调及运行验证仍未完成。没有链接/运行测试程序或游戏，没有应用补丁。报告 reports/unified-source-v1.json。


### 2026-10-29：Boltok / Gnasher 差异复核

逐项复核 v6→当前 Boltok reload、Gnasher mechanics 和 mechanics test 三文件差异，当前 SHA 与统一快照 v1 一致。保留公共账本、无限备用弹药策略、原版接受序号、迟到回执及不复用 item/intent 历史；Gnasher 机构/动画交接不再直接作为奖励完成凭证，零填装取消仍需机构关闭和原版非补弹退出回执。拒绝插入后释放保留，要求重新抓取，不能留下虚构弹药或弹巢孔。

五项编译检查通过：Boltok 主流程、账本、legacy guards；Gnasher 账本及完整机械 fixture。账本检查含 constexpr 断言；完整 Gnasher fixture 仅语法编译，没有运行其 main 或机械流程。真实 Native/几何/中途原版退出认证仍未完成。本轮确认保留这些后续差异，不是已完成全包语义合并或游戏验收。报告 reports/boltok-gnasher-composition-review.json。


### 2026-10-29：十二项候选版本选择复核完成

补核扭力弓与 reward 三文件：扭力弓入口保留对拆分 mechanics/types/accounting 的引用；当前单箭入槽锁定、独立库存/完成回执、取消及原版退出后恢复都保留。原版 Charge/EndFire/真实弹体事件仍分开处理，非发射 abort 真实入口未证实，生产准入必须保持关闭。reward collector 仅抽取公共双次元数据采集器，旧 reward API 保留；grenade_notify 选择独立 Notify 路径，reward 签名验证仍拒绝此值，不能混作换弹奖励。

本轮八项针对性编译通过，含 Bow cancel/settled-exit/repeat/standalone，reward signature/snapshot/native reader 与 grenade notify native reader。八包审计标记的十二个文件现已逐项记录保留理由，所有当前 SHA 与统一快照 v1 一致，不重建相同源码包。范围是候选版本选择复核完成，不能等同全 Native 集成、完整语义验收或原版功能已实现。统一快照仍非整体覆盖生产源码的补丁。报告 candidate-composition-review-v1.json；所有原版调用/几何/资源与掩体消费者缺口继续保留，目标未完成。


### 2026-10-29：Trace bit1 包装器缺口已用实际指令补齐

限定私有 IDA 导出实际 Actor.Trace 两个 chunks，分别1873/186字节，哈希与既有历史/live比较记录一致。缓存 JUMPOUT 不能直接作为 VA：第一次按缓存推断的范围落入相邻函数，保留但排除其 Trace 语义；以真实函数边界重新导出后，在0x1400cdf29确认 BTS bit17、TEST extra最低位及 CMOVZ。最终 EBX 掩码经0x1400cdf83写调用参数，在0x1400cdf8f进入0x1400cf8f8。因此 extra=1 确实增加内部0x20000，与extra=8的诊断路径不同。

新增 cover_trace_flag_translation.h，仅是指令整数运算的离线模型，11项 static_assert /W4 /WX /Zs 通过，不是原版查询API或ABI。下游0xcf8f8→0xcfa40的保存伪代码仍有参数/尾部缺口，bit17实际组件形状过滤、query-visible物理姿态仍未知，不能签发 hit_geometry。旧统一快照v1不含此新增文件，保持不可变；生产代码/诊断flag白名单未改，未启动游戏。报告 cover-trace-bit1-decoded.json。


### 2026-10-29：枪弹查询掩码下游转交已核实

私有 IDA 实际指令确认0xcf8f8读取输入掩码后，在0xcf952设置bit10（0x400），0xcf96e写出并在0xcf977调用0xcfa40；后者在0xcfa9c取得R14D，经过0xcfd33条件检查，从外层+0x178取得接收者并调用其虚表+0x20，0xcfd6d仍传出完整R14D。因此bit17没有在这两层被清除。

IDA 在0xcfd58处误拆连续函数区段：0xcfd55的mov之后直接继续0xcfd58，使用同一组寄存器与栈帧。缓存伪代码的独立sub调用视图不能作为真实可调用ABI。全chunk已对照历史blob校验，原始导出见 cover-query-helper-chunks.json；评估见 cover-query-mask-forwarding.json。接收者的真实类型、虚表目标、形状过滤与姿态查询可见性仍未知，不把它猜成PhysX或CollisionHash。标志转换候选增加helper bit10模型，14项编译期断言通过。没有生产或诊断白名单改动、没有运行实验，原冻结快照v1不包含新候选。


### 2026-10-29：+0x178 查询接收者创建链及具体槽目标

历史字节候选扫描得到74处寄存器写+0x178，机械核对缓存后选中0x1401f1150：实际指令0x1f117d读取global0x141c20890，与Trace0xcdf60真实RIP地址一致；分配0x120字节，0x1f11b2调用0x1f14a0，0x1f11b8写parent+0x178。构造函数实际LEA在0x1f14ad取vtable0x14133d140，写object+0。保存数据节的槽+0x20（0x14133d160）指向0x1400cfef0。因此该创建路径的查询实现已有具体目标，不再只是未知虚调用。

这是历史构造链，不证明当前实例未被替换或当前ABI；动态对象身份/虚表/代码仍须认证。0xcfef0缓存（SHA已验）出现状态写入和0xe6a10递归线索，不得把它重放成只读探针；bit17具体组件过滤仍待补证。旧私有库打开报初始化error4/empty，保留失败副本，未发现存活IDA进程；重新校验原库SHA后复制新私有v2副本，实际导出成功exit0，原库不操作。报告 cover-query-receiver-resolved.json 与 cover-query-concrete-vtable.json，未接入生产、无游戏运行。


### 2026-10-29：查询掩码抵达候选组件虚检查

实际指令已确认0xcfef0读取incoming mask（0xcff07）并写context+0x3c（0xcffa3），0xd03b9进入0xe6a10。遍历从节点数组取candidate，检查+0xf4去重，取candidate+0x78关联对象，并读取+0x108起的向量做范围预检。随后0xe6c9d调用关联对象虚表+0x330，R9D来自context+0x3c；通过后0xe6ccc载入candidate虚表+0x2e8，0xe6cd8/0xe6cde再次传出同一掩码，0xe6d46调用R10。零返回走0xe6fcb的后续记录路径，不能按一般bool成功解释。具体槽实现仍待身份认证。

实施缺口由此更具体：除骨骼/刚体外，必须确认候选范围边界及查询索引已消费同一姿态，否则可能在窄检查前被排除。这里仍称bounds-like向量，不根据偏移强行命名引擎字段。原查询有context和去重状态写入，不可重放为被动探针。报告 cover-query-component-route.json 和原始 cover-query-component-chunks.json；尚未证明bit17最终几何语义、玩家组件具体槽目标或查询可见刷新时机。没有生产改动或游戏运行。


### 2026-10-29：SkeletalMeshComponent 基础命中检查及 bit17 分支

由类名引用定位工厂0x1404fd1e8，实际LEA0x4fd25a指向UTF16 SkeletalMeshComponent（0x1415bf86a）；工厂登记构造回调0x3d9d90，thunk在0x3d9d99调用实例构造0x3d9da4。后者实际LEA0x3d9daf取得虚表0x14137ac50并写实例首指针。保存数据的+0x2e8槽指向0x140339294，基础类命中检查因此有确定目标。

实际检查指令0x339307取输入mask到R12D，0x339332 BT bit17并条件跳转0x3393b1；该分支对关联数据+0x130做正值检查，正值进入0x14090bdf6，非正值返回0x339339，后者读取RSI+0x248并有条件调用0x131e80。不能根据这些偏移命名多边形/骨骼/PhysX分支；仍须解析具体几何与变换来源。当前玩家是否派生覆盖此槽也未认证。报告 cover-skeletal-hit-route-resolved.json 记录完整身份链、指令及限制；无生产或运行改动。下一步沿0x90bdf6解析数据消费，再与body更新对齐。


### 2026-10-29：命中分支与既有骨骼矩阵 getter 的直接连接

实际冷区0x90be4d先做name→index查询0xf9d80，-1跳过；0x90be76设置RCX=mesh，0x90be88以RDX输出矩阵、R8D索引调用0x1d40b8。返回矩阵随即复制并用于0x137a60变换处理，后续按asset+0x138的条目进入几何辅助函数。这是骨骼变换参与至少一条命中路径的直接证据，不能再将所有hit geometry都笼统归于刚体更新。

本次导出的0x1d40b8两段真实代码SHA，与现有tools/native-capture/cover_bone_getters.h的两段认证值逐项一致，可复用已有当前owner/thread/mesh及SpaceBases读取合同，无需另造骨骼getter。当前publisher明确拒绝MasterPose、无效索引identity fallback，并检查矩阵/数组/代码当前性；这些限制保留。包声明中存在PerPolyCollisionBones/PerPolyBoneKDOPs，仅支持后续假设，不能用其索引补运行时偏移。几何辅助函数、实际玩家资产分支、SpaceBases/world变换及范围索引的同版本同步仍未证明。报告 cover-hit-bone-transform-reuse.json；原库/生产源码未改，无运行验证。


### 2026-10-29：已定位查询链的完整片段身份清单

新增 cover-hit-query-code-spans.json、可重复离线 verifier 和私有 cover_hit_query_code_spans.h：从已有实际指令导出中去重排序，14个无重叠代码片段共7828字节，逐片对照历史代码节SHA验证通过；包含已知查询辅助、遍历、骨骼命中主/冷片段、共享矩阵getter和名字索引。C++清单范围/哈希格式/重叠检查 /W4 /WX /Zs 通过。它供未来接入前确认代码身份，不包含原版函数体，也不授权调用，不是完整递归依赖闭包。

缓存辅助代码出现zero-extent区分及两套几何路径，尚未给它们强行命名。尝试导出辅助函数时私有IDA v2再次初始化error4，未产生新导出；保留日志/副本，不重复修复，继续现有证据的可重复核验。更深几何helper、实际实例/ABI/姿态和bounds/index认证仍缺；没有修改生产或执行游戏/测试程序。报告 cover-hit-code-preparation.json。冻结快照v1不包含新清单头文件。


### 2026-10-29：几何辅助函数实际导出与代码清单 v2

从校验原库新建私有v3副本，一次限定导出五个辅助函数成功exit0，原库/工具配置不改。实际0xdb120对三分量做零比较，有限输入下支持zero-extent分流判断，但不能据此省略非有限输入校验。0xdec0e8按80字节条目从asset+0x138准备查询；0xdebe0c复用该准备后增加extent相关处理。0xded060经0xad8464进入0xdecae8；0xdec81c经范围调整和同一预检进入0xdec154。具体叶级几何算法与资产记录类型仍未证明。

新增五段至不可变v1之外的code-spans-v2清单，现19个无重叠片段共9002字节，全部逐片对历史代码节验SHA；私有C++清单编译通过，离线verifier支持显式v1/v2，默认v2，分别保存报告。没有调用或重写原版几何函数，没有修改诊断白名单。报告 cover-skeletal-geometry-helper-route.json，实际玩家资产、组件派生、同版本姿态/边界索引仍缺，目标未完成。


### 2026-10-29：叶级三索引顶点数据已核实

实际递归查询0xdecae8的叶段0xdecfd9..decfec以8字节记录前进，取+6的16位附加值并调用0xdeba24；后者0xdeba70..debaa4从asset+0x138的80字节条目读取+0x40顶点基址，按记录+0/+2/+4三个16位索引形成12字节顶点地址。体积查询0xdec154在0xdec699..dec72a使用同一顶点来源、三个索引和附加值，将三点传0x1b697c。后续命中结果还要和已有fraction比较，不可把任一叶返回当整个查询结果。

三组3-float顶点与原向量检查支持三角形类几何的推断，但不证明全视觉网格蒙皮或完整树格式。附加16位值不擅自命名材质/骨骼。实施上保留原资产顶点/索引，用当前bone-world变换配合同版本范围/索引可见性；不能通过改共享资产顶点替代姿态更新。普通线诊断不能覆盖体积查询。报告 cover-skeletal-indexed-geometry-route.json 与原始指令导出；当前玩家分支、其他物理路线、范围刷新和实际受击关联仍未完成。未改变生产、未运行游戏。
