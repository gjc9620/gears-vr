# 掩体系统开发与验证记录

目标：完成掩体系统开发与验证。状态：进行中。

## 2026年11月8日 首批接续

使用统一工作树 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。本批开始时 HEAD 为 `f30862b`。保留现有武器工作，不修改武器分派、共享骨骼写入端、默认候选或用户配置。

### 当前实现缺口

- `tools/native-capture/cover_input_live.inl` 和 `cover_shared_input_live.inl` 均将抓边范围设为零、边缘设为空。现有抓边状态机没有因此形成实际玩法。
- `cover_geometry_live.inl` 能核对槽位及原生位置、旋转 getter，但显式不认证碰撞或抓取许可。
- `body_c0_goal_policy_live.inl` 的 `no_cover` 检查排除掩体，共同身体写入策略仍须接入掩体上下文；不能简单取消该检查就宣布完成。
- `cover_full_goal_live.inl` 只保存完整身体目标，明确标记姿态尚未应用、碰撞尚未验证。
- 专项规格中保留的历史地面和低墙命中仅能帮助确定下一次测量位置，不能代替当前场景的可抓边、可达性或身体扫掠证据。

### 本批修改

在原 `tools/game-playtest/cover_trace_query.py` 增加 `capture_edge_interval`，直接复用现有、已认证的原版 GT Trace 调用。输入为已测得的命中射线、向边缘外侧的平移和所需区间精度；不从槽位高度猜测真实顶沿。

搜索先要求起点命中实体挡面、外侧端点为空，再有界二分。单次搜索最多八条射线；规划精度无法在预算内完成时，提交前即拒绝。查询失败、排队状态未定、资源归属未完成、场景身份或槽位变化时立即停止，不重试。命中必须处于线段内部，法线为迎向来射方向的近水平单位向量；拒绝自身、控制器、CoverLink 标记、地面和不同碰撞 Actor。

输出只表示一个截面的已测命中与空返回区间，不授予 LGrip，不放大合法拉拽范围，不宣称连续碰撞或整段边缘已认证。调用的三十秒预算在每条射线前后检查；单条已经开始的查询仍按原 `capture` 自身截止时间和资源归属规则结束，不取消或重发结果未定的请求。

### 本批实际验证

| 检查 | 结果与范围 |
| --- | --- |
| `python -m unittest discover -s tools/game-playtest -p test_cover_trace_query.py -v` | 18 项通过，其中 11 个新增测试方法，包含顶沿、双侧与旋转墙面、错误命中、绑定变化、未定请求、预算和证据缺失等分支 |
| `python -m unittest discover -s tools/game-playtest -p test_cover_surface_query.py -v` | 原 13 项通过，原版 Trace 函数头、参数、代码及关闭证书保持 |
| MSVC `/std:c++17 /W4 /WX /EHsc /UNDEBUG /O2` | 6 个原掩体核心程序编译并实际执行 exit 0：Interaction、View、BodyTarget、ControllerAdapter、InputOwner、InputTransaction |
| `git diff --check` | 本批两个源码文件无空白错误 |

MSVC 原始结果保存在 `analysis/cover-system-20261108/focused-cover-results.json` 及相邻编译、运行日志。Python 使用 `C:/Program Files/Python314/python.exe`。这些为离线证据；本批没有启动游戏、XR 或模拟器，没有构建或交付新 Native／Playground 候选。

### 后续完成所需证据

| 需求范围 | 当前结论 | 下一项实际工作 |
| --- | --- | --- |
| CV-01–05 进入、基准与移动 | 有旧原型及局部证据，整套未验收 | 当前场景认证高低掩体与中立眼点，接沿边和退出 |
| CV-06–09 真实追踪与位移叠加 | 核心离线逻辑通过，完整姿态未接通 | 使用共同身体基准接实际掩体姿态，不重复应用 HMD |
| CV-10–16 抓边、归属与合法位移 | 核心状态机通过；实际抓取仍关闭 | 先取得当前实体边缘截面，再接手部可达和连续身体扫掠，最后提供合法范围和边缘 |
| CV-17–18 越墙、暴露与受击 | 未完成 | 接实际头部受阻遮蔽及身体暴露／受击链 |
| CV-19–21 瞄准和联合战斗 | 必须保留既有武器链，掩体整体验证未完成 | 验证单手抓边开火、松手双手支撑、换弹、缩回及枪口遮挡 |
| CV-22–24 稳定、过渡与双眼 | 有稳定相机候选，未达最终标准 | 跑动、冲刺、地形、吸附退出及真实头动联合验证 |
| 全部状态中断与 AC-01–19 | 仍待当前玩法证据 | 在实际集成候选记录暂停、回中、丢追踪、死亡和重载，并复核双眼图像与头显结果 |

下一切片是将本批边界查询用于现有受控游戏路径，取得当前同一槽位的实体边缘截面。接入抓取之前还须完成可达性和身体扫掠，不能将单截面二分成功改写成完整抓边许可。游戏窗口、冻结候选与启动清理继续遵循既有保护；不抢占其他人的游戏或构建过程，不启动助手。

## 2026年11月8日 碰撞组件连续性与正版隔离实例

本轮属于实质进展：新增碰撞表面连续性校验，并完成独立测试实例的准备。完整开发与验证目标仍在进行，不提升历史成熟度或改变公开 v71。

### 碰撞组件与变换

边界搜索最初只比较碰撞 Actor 身份。相同 Actor 的组件替换或变换变化也会使多条射线失去共同几何参照，因此现增加以下检查：

- 通过现有反射读取器核对命中 Actor 的 WorldInfo、CollisionComponent 和组件 Owner。
- 核对 LocalToWorld 的 Matrix、Plane 结构及实际声明字段，支持 Plane 从 Vector 继承 X、Y、Z；结构大小、偏移、类型和继承链接均参与关闭校验。
- 后续每条查询在提交前和返回后核对同一表面。即使射线返回空，也必须观察先前选中的碰撞组件及其变换，不能把墙面移动或组件替换当作发现边缘。
- 非空命中的原始 HitInfo.HitComponent 须与所观察的 CollisionComponent 一致。该偏移来自原有完整 TraceHitInfo 反射证书。

普通单次 Trace 默认路径不启用新表面观察；边界搜索显式启用。边界结果继续只证明单个截面，仍不授予抓边或身体移动权限。

实际离线结果：`test_cover_trace_query.py` 22 项通过；原 `test_cover_surface_query.py` 13 项通过。新增测试使用原有反射内存夹具核对完整 Matrix 和继承 Plane，并验证关闭时的变换、结构链接和继承链接变化拒绝。两次夹具构造失败分别为矩阵打包方式和遗漏结构大小字段，已修正；它们不作为产品运行失败或通过证据。最新日志为 `analysis/cover-system-20261108/surface-transform-tests.log`。

### 当前游戏前置条件

原 `solo-m1-1028d` 测试配置仍指向历史 P2P 路径；约定的 `J:/gearsVr/Gears of War Reloaded` 工作副本不存在。本轮未使用这些路径启动游戏。

通过既有 `game_install.validate_game` 核对 Steam 库内正版 EXE 和 Coalesced 配置后，复用 `tools/multi-instance/start.py::prepare` 创建 `analysis/instances/cover-20261108a`。实例有独立设置、存档副本、日志、Native 目录及 0.4 渲染配置。准备脚本与记录在 `analysis/cover-system-20261108/prepare-instance.py`、`private-instance.json`，`game_started=false`。原武器实例、公开候选和 Steam 游戏文件没有被本轮改写。

当前 Steam 进程和 ActiveUser 存在，但其他现有正版试验在 Steam 已登录后仍得到原版“需要执行 Steam 才能游玩”提示，并在 Native/XR 激活前退出。该结果不能归因为掩体。为避免重复相同前置失败，本轮没有启动游戏；已准备实例也不能写成运行通过。需要确认普通正版启动条件恢复后，再安排当前实体边缘查询。

### 身体执行链接续点

现有 `body_goal_view_adapter.h` 已支持 `Scope::fixed_cover`，可复用其同源头身手目标计算。实际 Native 策略仍以 `upright_posture` 和 `no_cover` 排除掩体；旧 `cover_camera_live`／`cover_neutral_eye_live` 又依赖旧 CoverBody 原型的布局与入口。后续须把只读掩体基准接到现有唯一 C0 写入端，不能为了取眼点而重新开放第二套身体写入器，也不能只删准入检查。实体几何、连续身体碰撞、暴露与受击等完整要求继续保留。

## 2026年11月8日 独立掩体身份读取与统一追踪接线

新增 `tools/native-capture/cover_context_live.inl`，复用已有 Scene 身份与反射读取器，读取当前 CoverType、CoverLink、CurrentSlotIdx、动作和过渡位。它核对当前人物与世界、Link 世界关系、CoverSlot 数组结构及选定元素，并在关闭时重新核对。读取失败保持调用方输出不变。

读取器不使用旧 CoverBody 的启用状态、布局、线程、Hook 或身体写入能力，也不调用引擎、改变相机或取得抓边许可。`GEARS_COVER_BODY_PROTOTYPE` 编译开关仍用于获得仓库现有的 shared Scene binding 实现，不表示需要启用旧运行时写入器。

通过新选项 `GEARS_COVER_CONTEXT_READER` 接入 `rig_tracking_live`：开启时从独立读取器发布掩体键，关闭时保留原路径。统一帧发布前再次核对掩体状态，拒绝前后不同的对象、槽位或动作。选项默认 OFF；本批隔离 Native 明确以 ON 编译，尚未安装或做游戏验证。实际中立眼点、面墙身体目标、碰撞与受击接线仍未完成。

实际检查如下：

- 真实反射内存夹具：108 项通过，旧身体写入与 Hook 始终关闭，并核对没有修改夹具内存。
- 读取器 OFF 路径、原相机 On／Off 路径均通过；原相机 On 为 195 项，Off 为 2 项。
- 原统一追踪 normal、epochs、binding-close、reference-inflight 四种情况通过。
- 新发布接线夹具 74 项通过：覆盖原相机未取得掩体视点时的掩体键发布、退出清除、读取失败、关闭变化及人物错配拒绝。此夹具替换读取端；真实读取端由前述 108 项单独覆盖，不能将两者称为游戏内联合验证。

完整隔离 Native 已编译、链接通过，SHA256 `94eaf122c3290244978e9e355bfcaad0fb65844c3acc674f16568e5ab30464b4`。Ninja 记录 661 个依赖，实际包含新读取器、统一追踪与原 Scene binding。最终源码、构建、9 次检查及回执分别在 `analysis/cover-system-20261108/context-native-r4-source`、`build-context-native-r4`、`context-native-r4-build-result.json`。构建基于保留的统一冻结源，只叠加本批文件及其缺失的构建辅助文件，没有编入其他会话正在编辑的 capture／武器分派。

首轮独立测试因复用夹具 main 宏冲突和局部参数重名编译失败，修正后通过。前两次冻结包配置分别发现旧快照遗漏 `.cmake` 和 `.asm` 文件；保留原失败目录，补录同一仓库未修改的辅助文件及哈希后完成 r3 编译，追加发布接线检查后完成最终 r4。没有把配置失败改记为通过。

主人经统筹补充当前游戏位置为 `J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64`。本轮已用现有 `game_install.validate_game` 核对该路径 EXE 和 Coalesced 配置通过；此前只依据旧路径禁用记录作出的环境判断应按此最新信息更新。本轮未因此启动游戏或模拟器。后续验证遵从主人最新要求：使用现有游戏直控与模拟器脚本，不使用 Computer Use；需要时录制并实际检查游戏和模拟器画面，并保留共享运行窗口与清理保护。

## 2026年11月8日 掩体眼点与统一身体目标转换

在既有 `body_goal_view_adapter.h` 增加 `place_cover` 和 `prepare_cover`，继续输出给原全身规划器使用的 Goal／Render 数据，不增加写入端。转换将独立指定的掩体中立眼点与朝向应用于已校准的眼点到骨盆关系；身体根朝向按墙面基准旋转，真实头部旋转不进入该身体转向计算。

脚位必须由进入掩体的脚位处理方显式提供。转换器保留这些脚位，不随躯干朝墙自动旋转、移动旧脚锚点。输入 `CoverPlacement` 只是当前放置数据，不是碰撞证明、脚位可达许可或 Native 写入资格。其世界、人物、Mesh、CoverLink、槽位、场景、publication、reference 与时间必须匹配本次有效 Core；拒绝时调用方输出保持不变。

真实 HMD 位移与已提交的虚拟抓边偏移仍由现有统一转换各应用一次。完整 C0 运行策略尚未选择该新入口，`no_cover` 等旧准入没有被直接删除；实际合法眼点、脚位及表面／身体碰撞输入仍待接入。

本批 MSVC `/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG` 编译与执行结果：

- `BodyCoverGoalAdapterTest` 212 项通过，使用保留的原版 115 骨记录和明确合成的当前 Core／掩体放置。四种墙面朝向与 0／15／30 厘米眼点降低组成 12 个完整身体规划，全部 solved；真实头部位移、抓边位移、脚位保留与过期／错场景输入拒绝检查通过。
- 原 `BodyGoalViewAdapterTest` 在同一合成当前 Core 上 159 项通过。输出已改为报告实际输入结构尺寸，不再固定打印“actual Core1832”。

首次直接读取保留的 `captured-core.bin` 时，精确尺寸检查失败：旧文件为 1832 字节，当前结构为 1864 字节。旧文件未转换、覆盖、补字节或改记为当前样本；失败保存在 `body_goal_view_adapter-old-core-failure.log` 与 `cover-adapter-first-failure.json`。尝试复用旧 Core 单元测试生成器时，其旧 ABI static_assert 同样拒绝；没有修改这些原断言。后续使用独立、明确命名的当前合成夹具生成器，并先通过当前 `core_snapshot_valid`。合成数据不属于游戏或物理追踪证据。

检查脚本和结果在 `analysis/cover-system-20261108/run-cover-adapter.py`、`make-synthetic-core.cpp`、`synthetic-core-current.bin`、`cover-adapter-results.json`。新检查已登记到 CMake，仍要求显式提供对应 Core 与骨骼夹具。没有据本轮生成新的安装候选或运行游戏；上批 r4 DLL 仅证明其冻结源码中的独立掩体读取接线，不代表本批转换已经进入实跑。

## 2026年11月8日 当前 C0 掩体原始姿态读取

在原唯一 C0 执行入口中补接只读测量：当前人物已进入掩体时，从本次已捕获的原始 Local 骨骼及组件变换重建眼点、根、骨盆与双脚。眼点继续使用 Marcus 原有作者标记；测量不叠加 HMD、不读取渲染结果，也不把动作中或过渡中的姿态自动命名为中立姿态。当前掩体姿态仍拒绝身体写入，不调用覆盖回执取得写入许可。

测量复用本次已认证 C0 上下文，前后检查 current、空闲重绑定条件、停止状态与修订号。只在当前 owner 回调范围有效；新的回调、结束、失效或停止都会清除有效位。记录保留原 Core 的 publication、scene、reference 与 published_ns，不以读取时刻刷新旧样本。它尚无实际玩法消费者，不能单独开启抓边、移动眼点或认证脚位碰撞。

本批 `BodyCoverGoalAdapterTest` 为 221 项通过，含原 12 组完整身体规划；新增检查确认原始骨骼未修改、原时间与身份键保留、HMD／world_head 改变不影响此原始测量，以及拒绝时输出保持。原转换器另在同一明确合成的当前 Core 上 159 项通过。骨骼夹具是真实保留记录，Core 和本批掩体状态是合成输入，仍是 CPU 证据。

最终 r6 隔离 Native 编译、链接通过，SHA256 `63fe604d1db8f242cf02437e78af2214eb163ab9d18fa582ed0f4d2995768567`。依赖记录为 661 项，构建回执中的 10 次实际执行均 exit 0。源快照、叠加文件哈希、构建与检查在 `analysis/cover-system-20261108/context-native-r6-source.json`、`context-native-r6-source`、`build-context-native-r6` 和 `context-native-r6-build-result.json`。私有构建包含本批原始姿态测量接线，未安装、未交付为可玩候选，未运行游戏／XR／模拟器。

同时保留 r5 的独立失败：旧 `BodyC0GoalPolicyLiveTest` 宿主前缀尚未跟进现有 weapon/compose 接口，编译报告超过百项缺失类型／成员。本批新增的 mode16 只读生命周期断言已写入源码，因这一宿主编译失败尚未执行。r6 未请求该过时宿主目标，不把 Native 产品编译成功或纯转换检查当作该宿主检查通过；旧失败日志仍位于 `build-context-native-r5`。后续仍须验证实际 C0 回调生命周期，并取得合法中立眼点、当前实体表面、可达和连续身体碰撞输入后，才能选择 `prepare_cover` 进入写入事务。

## 2026年11月8日 动画状态字段修正

接续核对原版动作链发现本会话独立读取器使用了错误字段名 `LastCoverAction`。原有 `cover_neutral_eye_live` 和保留原脚本 `analysis/cover-prep-20261028/revision2/scripts/Function_WarfareGame.WarPawn.DoingAnimationTransition_.uc` 均使用 `LastCoverActionAnim`。先前合成反射夹具复制了同一错误，因此 r4／r6 的离线通过不足以证明该字段与原版兼容；它们没有游戏内通过证据，不应安装。

先改夹具为原字段名并增加错误字段拒绝，实际重现 124 检查中 20 处失败；原 RED 结果保存在 `context-name-before-fix.json` 与 `context-name-before-fix.log`。随后只修正生产读取字段名，124 检查全部通过。额外覆盖 LastCoverActionAnim 数值变化及关闭边界变化拒绝，旧身体仍关闭，夹具无写入。

r7 使用修正后的源快照重新完成隔离 Native 编译、链接和原 10 次离线检查，均 exit 0；DLL SHA256 `10eed9dff50097b86f795e6fcc97ac0b447d4baef5507cee70df69d0a579ec74`，依赖 661 项。证据在 `context-native-r7-source.json`、`context-native-r7-build-result.json` 与相邻源码／构建目录。此处更新当前可用于后续私有验证的构建身份；仍未安装、未运行游戏／XR／模拟器，原 r5 宿主检查缺口仍保留。

## 2026年11月8日 短段原版掩体与实体边界入口准备

已准备 `equipment-controls-prepared-20261108-cover-r7`，使用上述 r7 Native、保留的 Playground `f8e6f3c2…4807` 与模拟器 XR `16840b27…a7fc`。只复制已冻结二进制并记录源码快照和哈希，未修改共享实例 manifest、默认候选或公开 v71，未启动运行。20 个运行依赖哈希、主人指定 EXE 和配置验证通过；`run_cover_check.py --prepare-only` 实际 exit 0，内存选择后的动作模块也已编译检查。

入口复用现有受控 Source1、后台窗口、observe-only、静音、身份、双路录制和正常退出／配置恢复路径，私有渲染比例为 0.4。动作在同一实际世界中只读发现附近启用的矮掩体候选，通过普通走动、转向及一次原版右手 A 进入；不设置人物位置或 CoverAction，不拾取／开火／换弹。发现、步行、槽数和时间均有界，未成功进入或当前反射几何拒绝即保留失败并正常结束。

只有进入同一实际低掩体并取得当前槽位几何后，才计划一组从低处挡面到上方空返回的截面，复用 `capture_edge_interval`，最多八条现有原版 GT Trace。槽位变换仅用于候选射线放置；挡面、空返回和法线均须由实际调用确认。单个截面成功仍不证明整边、手部可达、连续身体扫掠、中立眼点、受击或完整掩体玩法。任何未定提交不重发。

脚本、准备和计划在 `analysis/cover-system-20261108/prepare-cover-runtime.py` 及 `analysis/solo-m1-20261028/{run_cover_check.py,run_cover_operator.py,cover_boundary_chain.py,cover-boundary-runtime-plan.json}`。提前持久化真实游戏身份的失败默认记录保留已有中断录像归属做法。已向现有统筹登记后续唯一窗口，继续等待明确交接，不因排队结束工作，也不抢占正在使用或已约定的窗口。

## 2026年11月9日 等待期间的需求与启动前补全

统筹已明确接受掩体排在 HB → Boltok → Frag → Lancer235922 之后，Lancer 正常退出、双路复查、编码与 retention 全部结束后再向本会话唯一交接。确认在 `analysis/solo-m1-20261028/cover-boundary-window-queue.json`。本会话尚未获得窗口、未 stage、未启动；正常等待不记 blocked 或完成。

需求文档补入 2026-10-28 交互合同／第二轮研究中的提交顺序、连续体积扫掠、当前占用、动画开火等待和原伤害分支约束，保留 CV-01–24，验收追加至 AC-22。主人随后补充“注意模块化，注意解耦”，据此增加模块职责、输入输出、依赖方向和禁止跨模块内部操作的要求；不将文档接口描述为已接通运行接口，也不发起全仓重构。项目目录副本与来源材料按哈希同步，需求编号及来源链接均已检查。

启动前导入检查发现离线 MSVC 脚本所用 Python 3.14 没有 Pillow（原失败保留）；已核对受管解释器 `C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe` 的 Pillow／NumPy 导入 exit 0，并用它执行 `run_cover_check.py --prepare-only` exit 0。运行入口与审片使用该解释器；原启动器子进程仍使用现有打包解释器。记录在 `runtime-interpreter-check.json`，不是游戏结果。

针对 r5 宿主缺口，仅在 ignored `host-include-probe` 副本补入实际类型头做一次编译定位。缺失声明的一部分被解决，但旧诊断／runtime seams 与严格警告仍使编译失败，程序没有执行；原编译字节和失败记录保留。没有改生产源码、禁用严格警告或伪造宿主通过。此项不影响已经独立通过的 r7 Native 构建，也不关闭尚未完成的实际 C0 生命周期验证。

## 2026年11月9日 r7 首次实际运行 46808

收到 Lancer 完整收尾后的明确唯一交接，重新核对冻结依赖后精确 stage r7，实际游戏 46808／creation134386310077392271，session `continuous-stereo-46808-20261109-010406-50eed86f`。使用主人指定副本、受管解释器、Source1、0.4、后台／observe-only／静音和原输入隔离保护；公开 v71 未改。

首个失败发生在诊断入口，而非掩体动作：`Observer.sample()` 的旧 `gameplay(menu)` 只接受特定战役 popup，当前 Playground 的 top 为 null，故未填 `special_move_raw` 等人物字段；本入口随后触发 KeyError。没有导航、A 进入、抓边、Trace 或身体移动，因此不记掩体进入／碰撞失败或通过。保留原始失败与精确执行源码哈希，后续仅在 Playground 专用诊断适配层接现有认证锚点／人物读取，不改变公共战役菜单判定。另已补失败前保存完整观察行及明确缺失状态错误，尚未重跑。

亲自检查 13.7000835 秒完整跨度的 1 页双路总览，以及末端 `game-00226.jpg`、`simulator-00286.jpg` 原图：标题、等待卡、初始第三人称及随后实际第一人称自由手可见，未发生掩体动作。227 游戏帧／287 模拟器帧，两路末端相差小于 500ms；游戏捕获最大间隔 1933.3566ms，保留该采集空档，不把它推断为游戏停顿或帧率证据。单段双路编码实际 exit 0，不代表头显或掩体完整验收。

wrapper／launcher 均 exit 0，Native cleanup／entry restore TRUE、XR exit 0、owned game 为空；config、settings 精确恢复，原缺失 head 文件仍缺失，结束时 20 个运行依赖未漂移。既有 retention 22786 实际 terminal0，删除 2148 文件／366588094 字节、errors0，保留 [46808,9684,5008]，文字／源码保留。核实 Game／XR／录制／编码均空后，已明确唯一交窗 HB 原匣004159／PG28f0184d，之后按其既有队列继续；本会话不重启或占用该窗口。

完整证据在 `analysis/cover-system-20261108/cover-46808-review.json`、`cover-boundary-chain-46808.py`、对应 session、`menu-firstperson-46808-010410` 和 `menu-firstperson-46808-startup-20261109-010318/dual-output`。这是新的实际首故障与启动／清理证据，完整掩体开发目标继续进行。

### 首故障后的专用只读适配

新增 `tools/game-playtest/cover_playground_observation.py`。仅在已拥有的正常运行 session、原连接已就绪、明确认证 top=null／active_scenes=[]／无 input block 的 Playground 中，复用现有 `LiveRegistryAnchorSampler`／`acquire_anchor_sample` 和原类型化人物字段读取。校验当前 controller／pawn、非剧情、存活，以及前后 UI、对象和 session；失败关闭自己的 reader，不返回部分人物状态。公共 `Observer`／`gameplay` 未改，也不授予输入、查询、掩体或身体写入权限。

实际离线 `unittest` 三个方法通过，包含一个明确空 UI 正例、五种缺字段和五种活动／不可信 UI 反例。初稿将缺失 top 与明确 null 混同，该负例先失败，增加必须存在 top 键后通过。此检查只验证 UI 准入，实际锚点读取与游戏内掩体动作仍未验证。

入口同时补齐失败前保留观察行和明确缺失人物状态错误，执行过的旧动作源码按原 SHA256 保留；旧运行计划另存 `cover-boundary-runtime-plan-executed-46808.json`。新计划只替换诊断动作哈希并加入专用适配，21 依赖 `--prepare-only` 通过，r7 Native／PG／XR 未改，无新 stage／run。已向 HB 登记在既有 Snub／Boltok／Longshot 之后的尾位复查，等待明确唯一交接；当前窗口仍不属于掩体。

## 2026年11月9日 实际 55600 与定向复查 72608

收到 Longshot 完整正常收尾后的唯一交接，精确 stage 原 r7，实际 55600／creation134386346981712265 的专用 Playground 适配取得当前认证人物状态（Health450）。新首故障在发现阶段：全图 257 个 CoverLink 在筛当前世界／附近范围之前就触发原 128 候选预算。尚未导航、A 进入或 Trace。15.80 秒双路全跨度 1 页及两路末端原图亲看，编码15948 exit0；wrapper48612 exit0、Native／entry TRUE、XR0／launcher0／game退出；config／settings／原缺失head精确恢复。retention47796 terminal0，删除1360文件／218221879字节、errors0，保留[55600,4632,2044]。

只修发现顺序：先用当前 world 和900单位距离形成位置提示，再应用原128候选预算；后续真实反射 world／位置／槽位及 closing 检查仍逐项执行。500000 UObject／30秒、128候选／32槽等既有诊断预算、原版查询证书、输入与时间保护均未放宽，产品三二进制不变。执行过的21依赖全源码归档在 `analysis/cover-system-20261108/drivers-r1`，55600旧计划另存。修正后 `--prepare-only` 通过；依据具体失败允许定向修复／复查的现有授权，在同一独占窗口内仅作一次未达到动作的复查，并提前告知下一接收者HB；没有重复已通过玩法或未知请求。

实际复查 72608／creation134386349955377432，从257个全图提示筛得17个同世界附近Link、62个启用低掩体槽候选，普通VR走动及一次右手原版A实际进入矮掩体：CT2／CA0／CD0、current/left/right slot0、pct0、stationaryTRUE／circularFALSE。同场景几何读取 accepted。随后原版GT Trace实际完成一次，queue_state2、pending_possibleFALSE、调用内存已释放，命中 `mp_warmachine_SMC` 挡面；返回位置约(-3898.058,-200.418,584)，法线约(-0.803208,0.595699,0.0000357)。后续 `_surface_state` 报 `Edge collision component ownership differs`，故区间搜索立即拒绝，没有第二条射线或重发。不可把调用成功、有命中或Actor名称当成组件关系／顶沿／连续碰撞认证；抓边与身体写入未开放。

完整27.07秒双路1页及四张原图已亲看。末端游戏等待卡／模拟器黑帧保留；贴近CT2观察时刻的 `game-00430.jpg`（约+1.03ms）已是等待帧卡，`simulator-00533.jpg`（约-32.30ms）可见自身身体与装备近裁剪遮挡。数值原版进入与实际VR画面分开记：原版进入达到，完整掩体画面FAIL。不能据此直接命名C0或相机根因；须核对现有同次来源撤回与视图／身体消费记录。游戏最大捕获空档1950.022ms、模拟器一次pool resize drop均保留，不推断渲染FPS或游戏停顿。

72608 wrapper35800／编码61815均 terminal0，Native cleanup／entry restore TRUE、XR0／launcher0／game退出；config／settings精确恢复、原缺head仍缺，21运行依赖结束时无漂移。retention18126 terminal0，删除2602文件／458183517字节、errors0，保留[72608,55600,4632]。核实Game／XR／录制／编码均空后，最终明确唯一交给HB012813／PGfe0bd568，之后按其已接受队列Lancer012317→Snub。本会话本窗口不再启动游戏。

证据分别在 `analysis/cover-system-20261108/cover-55600-review.json`、`cover-72608-review.json`，对应不可变session与 `menu-firstperson-*/cover-boundary.json`／双路目录。先前46808媒体已按主人 newest3／48h 规则实际删除，文字、源码和亲看记录保留，不再提供不存在的图片链接。下一源码工作分为集合静态网格的当前Actor／Component归属关系，以及实际CT2时身体／视图来源衔接；不删保护、不把单条射线升级为完整安全范围。完整目标保持未完成。

### 72608 拒绝后的离线定位

旧 `_surface_state` 把 CollisionComponent 为空及 Owner 与 Actor 不同合并为同一错误，72608 回执没有保留这两个已读值，因而不能从集合Actor名称猜定根因。新增 `SurfaceOwnershipRejected` 只把原本已经读取的 actor／component／owner 保存为明确 `accepted=false/collision_verified=false` 的拒绝观察；capture 继续返回失败，所有归属、世界、变换、关闭、预算与重试规则不变，没有新增引擎调用或周期采集。空组件与错误Owner两种反例均确认仍拒绝且能区分。实际 `test_cover_trace_query.py` 23项及原 `test_cover_surface_query.py` 13项通过；未做新游戏运行。

另核对 r7 冻结 `capture.cpp` 的最终视图选择：已有 `body_view_required` 且 first_person／body／cover 三者均无基准时，明确累加 missing_basis_rejected 并拒绝本帧；共同C0策略本身仍排除cover写入。随后互核保存的 Native 快照 owner8396／creation134386349882977996 与 capturing pid72608，均与本session吻合，读取到 `first_person_missing_basis_rejected=163`；未进入掩体的55600对照为0。这是同次实际拒绝计数，仍不是某个CT2／compose／图像的原子因果回执。合法低位基准、脚位与碰撞仍须补证，不能删除该保护以获得一帧未认证视图。

下一只读入口已冻结：在普通Playground中仅按当前世界和符号 `mp_warmachine_SMC#3` 重认证该集合Actor，复用严格表面字段读取及closing，保存组件／Owner拒绝观察；不再A进入、不提交Trace、不改变身体或产品。发现仍有500000对象／30秒预算，21依赖及精确源码归档在 `analysis/cover-system-20261108/drivers-owner-only`，`--prepare-only`通过。HB已接受在其本段→Lancer012317→Snub013007→Boltok013903→Frag020326之后唯一交本只读项；以实际交接最新顺序为准。本会话未stage／run，目标继续未完成。

## 2026年11月9日 身体面墙与连续视图基准分离

离线按CV-02复核此前转换器，发现 `CoverPlacement.anchor` 同时控制身体根朝向、虚拟拉拽轴、真实位移映射和视图朝向，无法表达身体转向墙面而HMD观察方向连续。先增加“墙面转90／180／-90度，真实头位姿不变，观察方向应保持”的回归，实际245检查中9处失败，原12组整身求解仍通过；这说明原通过包含了错误的视图随墙转向假设。RED保留在 `cover-heading-before-fix.json/log`。

新增必须显式提供的 `CoverPlacement.view_anchor`，其眼点须与墙面anchor相同，朝向独立。`place_cover` 形成身体根／骨盆／明确脚位的参考放置；`prepare_cover` 再用原作者眼点至头骨关系校正中立头骨支点，形成完整目标。墙面仅决定身体根朝向和虚拟拉拽轴；连续view anchor决定视图及真实头动的世界映射。HMD／virtual grip仍各应用一次，身体转向不修改HMD观察，真实转头不回写身体根或骨盆。普通 `prepare` 保留原同轴单次视图组合路径。

分离后原脚位夹具的支点问题也实际暴露：它绕眼点旋转整个地面放置，头向保持后部分固定骨长目标不可达。保留中间失败 `cover-heading-pivot-foot-failure.json`，没有放宽求解器。夹具现在显式围绕中立头骨支点提供入掩体脚位，降低眼点不降低地面；转换器仍逐项保留输入脚位。另加旧眼点支点脚位反例，转换不擅自挪脚，原求解器继续返回spine_unreachable。实际脚位可达／扫掠及中立基准认证仍须由运行接线完成，合成脚位不授予任何许可。

最终MSVC/W4/WX检查：designer与实际live使用的Marcus marker各326项、各12组完整身体规划通过；原普通转换159项通过。覆盖独立墙面朝向、原地真实转头、真实位移与虚拟位移分离、明确脚位保留、错误／缺失view anchor、错身份与旧时钟拒绝。Core与放置仍为明确合成输入，骨骼为保留115骨记录。Marcus选择参数为 `--marcus`，不再把designer marker的结果当实际live profile覆盖。

隔离r8 Native编译、链接及10次既有针对检查均通过，SHA256 `7aaafc5819f04e81a5e741dbaf61407fbacb4699e49708a099ea3604aba14a3c`，依赖661项。产品头文件与冻结r8逐字节一致；构建后仅增加测试CLI的marker选择，再独立编译同一产品头完成Marcus检查，没有重建未变的Native。回执在 `context-native-r8-build-result.json`、`cover-adapter-results.json`、`cover-marcus-adapter.json/log`。旧宿主r5缺口仍未关闭。

r8没有stage、没有游戏／XR运行，也没有开启掩体写入或解决72608画面失败。下一只读窗口继续使用明确冻结的r7产品；本批数学不会混入它。连续view heading的实际同源提供方、合法中立眼点／脚位／当前碰撞、抓边追踪轴与该独立视图的映射、C0事务及同版视图／受击／枪口消费仍待接通，不能把本批编译与CPU规划称为完整掩体通过。
