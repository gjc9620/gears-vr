# 掩体系统开发与验证记录

目标：完成掩体系统开发与验证。状态：进行中。

最近结果（2026-11-09）：r24/game33420完成当前严格实体边缘截面的8次原Trace，但暂停B恢复、unified武器清理、右身体遮挡与启动捕获缺口仍失败或未闭合。r28的Native真实骨架读回已接生产C0收尾；r30在当前停机资源修复上验证47个Host场景／961检查通过，未清除来源覆盖债务。r29新增准确静止正Extent参数与最多五次原版体积查询对照，73个Python测试方法通过，冻结候选prepare-only通过，已保存7d741ee8，尚未stage或启动。已获Boltok105222接受其后的唯一后继登记，未获最终交窗，当前不占运行窗；沿用准确旧r13三制品，不包含r25–28或后续Native源码修改。完整CV-01–24／AC-01–22与实体头显仍未完成，具体边界见本文末两节。

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

### 独立视图下的原始抓边轴

再补 `cover_tracking_basis(reference, view, wall, out)` 纯计算重载，将墙面右轴先投到独立连续视图的右／前轴，再转入固定reference的原始OpenXR坐标。当前HMD／控制器旋转不是输入。旧两参数接口不变，同轴情况新旧结果一致；点到世界的映射继续调用既有 `cover_tracking_point`，但独立基准消费者必须显式传view anchor，不得误传wall anchor。

4种墙向与3种连续视向共12组，真实手沿新原始轴平移5cm映到世界后始终等于墙面右移5cm，上移3cm仍按世界重力，非法基准拒绝且不改输出。现有 `cover_view_test` 883检查及原 `cover_controller_adapter_test` 629检查通过，MSVC/W4/WX，回执在 `cover-independent-axes.json`。没有改输入仲裁、真实按下／释放、100ms等时限或任何身体写入。

此重载在r8构建之后添加，只完成独立内核编译与检查，尚未选入live抓边消费者，也不属于r8二进制的新运行证据；旧两参数路径行为保持。后续真正使用时须与当前身份／reference、独立view heading、边缘／可达／碰撞及同一提交事务一起接通并构建验证。没有据此启动游戏或重建未使用该重载的Native。

## 2026年11月9日 只读归属诊断49584／65700与字段缓存修正

收到Frag69684完整收尾后的明确唯一交接，精确使用r7三制品与21个冻结驱动，运行只读符号Actor归属项。49584／creation134386384867694833的首故障是无关UObject发现提示的名称超出原有UTF16读取上限；尚未到目标Actor字段，没有A进入、Trace或身体写入。只将该发现循环接到普通掩体发现已使用的有界异常处理：注册表变化继续立即失败，无关不可读提示可跳过，所选Actor仍须原始身份、世界、字段与closing核验。执行过的原r3／修正r4源码与计划分别保存，未延长预算或改产品。

49584的37.20秒完整双路共2页及目标观察附近game00515／sim00717原图亲看：启动第三人称／等待后实际第一人称自由手，未发生掩体动作。编码原句柄38965已缺失，不能补造原进程退出码；成品与最终日志已核对，ffprobe实际exit0、视频37.233333秒。Native cleanup／entry restore TRUE、XR0、launcher0、game退出，wrapper与config／settings／原缺head精确恢复，驱动结束哈希一致。retention85554实际terminal0，删除1830文件／334978883字节、errors0，保留[49584,69684,26152]。

具体发现故障修正后在同一窗口只复查一次，实际65700／creation134386392141155216跳过2个无关提示，读到目标类名StaticMeshCollectionActor，随后在严格WorldInfo声明归属检查停止。没有完成组件／Owner观测和最终closing，不能以类名读到推断当前碰撞组件关系。没有A、Trace、抓边或身体写入，也没有将r8数学混入r7运行。

65700的33.47秒完整双路2页及game00505／sim00651原图亲看，实际第一人称自由手可见；564游戏／736模拟器采样帧、双路末端覆盖合格。游戏最大采集间隔2599.9861ms和模拟器500.0106ms保留，不作为游戏停顿／渲染FPS证明。wrapper／编码均实际exit0，Native cleanup／entry TRUE、XR0／launcher0／game退出；config／settings／原缺head精确恢复，21驱动结束时一致。retention35013实际terminal0，删除1919文件／326355451字节、errors0，保留[65700,49584,69684]。重新核对Game／XR／录制／编码／retention均空后，已最终明确唯一交Longshot021930，后续HB023513→Lancer023554；本窗口不再stage／run。

离线按65700真实调用顺序复现：先以可继承查询读取集合Actor的WorldInfo，再由_surface_state逐层认证实际声明类，旧Query.prop用(owner,name)缓存两种查询，错误复用父类字段并在子类声明检查抛出相同的Bone field identity/class/owner rejected: WorldInfo。新增回归先得到同错误RED，再仅把缓存键改为(owner,name,structure)。当前完整链可认证实际父类声明，子类自身查询仍明确not found；改变已认证的继承链仍使closing拒绝。没有取消归属检查、放宽时间或把未知值当有效结果。

world／trace／bone／surface四个相关模块共98项实际离线检查通过，原失败日志保留在inherited-property-cache-red.log、inherited-property-cache-runtime-sequence-red.log，GREEN在inherited-property-cache-green.log。此为读取器修正，r7／r8制品未改，修正后尚未游戏复查。执行过的65700计划另存，下一入口的21依赖与源码在drivers-owner-only-r5，prepare-only实际exit0，未stage／run。后续仍需取得真实组件关系，才能继续实体边缘与完整身体碰撞证据；不能据此宣布掩体可玩、画面或实体头显通过。两局完整证据见cover-49584-review.json、cover-65700-review.json及对应不可变session／动作／双路目录。

## 2026年11月9日 集合命中组件的严格读取路径

从主人指定游戏副本的Engine.u重新离线解压，55块长度检查通过；解压SHA256为95b0c8da…e90510，与保留的Engine.u逐字节身份相同。现有UELib解析器定向导出StaticMeshCollectionActor及其父类；对象表包含StaticMeshComponents的ArrayProperty／ComponentProperty，集合类继承StaticMeshActorBase，与普通StaticMeshActor的单组件默认配置不同。此处只是静态结构提示，不能据此断言65700的CollisionComponent为空、成员Owner为何值或实际命中哪一个组件。

原始部分输出保留在collection-static与collection-parent-static：集合数组反编译为array<@NULL>；父类／默认对象有RenderOnlyDrawScale3D及Components解析错误，不能称完整反编译。REA的Windows PTY捕获明确不支持验证后代清理，本次未取得REA Evidence；按技能恢复路径改用已有只读包解析器，未安装或修改REA、未附加其他会话的游戏。来源身份、解析限制及原工具拒绝另存collection-surface-review.json和rea-process-provider-unavailable.json。

补入_collection_member：只有调用方明确提供实际TraceHitInfo.HitComponent，且当前Actor为精确StaticMeshCollectionActor时，才可在原根CollisionComponent不同的情况下检查集合路径。须认证实际声明的ArrayProperty、ComponentProperty内层及其StaticMeshComponent声明类，命中组件须属于该类、在当前完整成员数组中恰好出现一次、Owner仍为同一Actor，之后仍核对原有类型化LocalToWorld。数组头、全部成员字节、内层链接、声明类链接、继承链、Owner与变换均进入原closing；不是从数组中挑一个组件冒充射线结果。

capture从已认证TraceHitInfo布局读取实际命中组件，并在后续射线前后传递同一组件身份；空返回也须保持先前表面证据，换组件／换Actor或元数据变化立即停止。普通Actor仍须命中其原根CollisionComponent，无显式组件时也不因集合类名或成员列表放行。调用预算、单次提交、未知请求不重发、分配释放及原身份／生命周期保护未变。该路径仍只认证诊断射线的表面观察，不证明整条可抓边、手部可达或连续身体体积扫掠，collision_verified／grab_authorized／body_modified继续FALSE。

world／trace／bone／surface共101个离线测试方法实际exit0。新增覆盖明确组件的正例、普通根组件等价、11种错误身份／类型／成员／Owner反例，以及成员顺序、内层链接、声明类、Owner和变换变化后的closing拒绝；既有区间夹具确认第二条及以后继续绑定首次实际观察的组件。测试为合成反射内存和队列夹具，不是本次游戏调用／实际集合归属通过。初次26项、随后100项及最终101项日志均保留；最终为collection-surface-final.log。

下一次已排队项仍仅按符号重认证当前Actor并读取元数据，无A、无Trace、无抓边／身体写入；诊断额外记录数组实际声明类和所抽样组件类，并将数组内层链接加入closing。21依赖冻结在drivers-owner-only-r7，prepare-only通过；没有stage／run，没有改r7／r8 Native或公开v71。HB已更正唯一前驱为Boltok030739，既有尾序为Snub024436→Frag030808→Boltok030739→Cover新只读；运行时以实际最终交接为准，不据idle占窗。C0旧宿主的当前依赖接线缺口仍在，未重编或伪报通过；身体／视图／碰撞完整闭环继续未完成。

## 2026年11月9日 共同抓边层接入独立视图数学，r9隔离构建

继续检查发现，尽管身体目标转换器已经分开墙面与连续视向，CoverInputOwner仍直接用墙面anchor构造view，并把追踪参考中的physical.right当墙面右探量。新增端到端回归：墙面转90度，视向保持，真实头部同时向原视向右移15cm、前移10cm；旧逻辑实际出现5处失败，包括视图旋转和期望墙面右探-10cm。RED在cover_input_owner-view-red.log，未据此启动另一局。

cover_view增加显式wall／view两个anchor的prepare_cover_view重载，要求相同中立眼点：虚拟偏移沿墙面右轴与世界重力，视图旋转使用独立view，HMD不在此处应用。同轴路径保留原单次组合；身体Goal转换与共同抓边层复用这一个实现。CoverInputOwnerFrame新增必须显式提供的view_anchor，冻结／重试核对也包含其完整基准；各现有调用方已显式填写，不能因字段默认空而悄悄使用墙面视向。

共同抓边层从固定追踪reference及同一对wall／view计算原始手部拉拽轴，不取实时头朝向，也不使用变换后的虚拟手反推身体。真实位移仍保留原追踪分量，requested_right另外投影到墙面右轴；上下仍按重力。缺失／非法掩体视图只撤回Cover提案与视图，不阻塞同次已认证输入中的合法武器请求。释放、保持、重抓、反拉、token及原100ms／身份保护未放宽。

针对检查实际通过：CoverView1087项、CoverInputOwner120项、CoverInputTransaction766项；身体转换designer／Marcus各326项及各12组全身规划，普通转换159项。现有共享消费者宿主outside25、cover18、source1-cover18、held-fallback54、retry14项也通过，使用真实公共owner／仲裁／broker代码与明确合成Native端点，不是游戏或头显。覆盖分离视向后的抓取正反方向、松手保持、墙面投影、未知视图下武器请求，以及view基准在同一事务中被修改后的拒绝。

第一次联查另有13个旧事务夹具失败：夹具搬了墙面中立眼点而未搬新增的独立视图眼点，因此新同点保护正确拒绝。已保留原失败JSON／日志，仅在该“新场景同时搬基准”的正例中显式移动两者；同帧单独修改view眼点／forward／right的3个负例继续拒绝，没有放宽同点或缓存规则。最终事务日志为cover_input_transaction-separate-view-final-test.log，其他结果在cover-owner-separate-view-checks.json与owner-view-final-consumer-checks.json。

r9从完整保留r8源码树复制，冻结10项明确overlay；除本批9个实际修改文件外，包含先前已检查但r8构建后才加入的独立抓边轴／Marcus测试CLI。其余Native源码及所有GEARS开关沿用r8，不引入共享工作树的其他武器实验。Native实际编译链接exit0、661依赖，DLL SHA256为b20cd94d464d13ce9f65ddaf705825d9c5f2413e75b62a77c7892127402c4b94；快照overlay与当前源逐字节一致。记录见context-native-r9-source.json、context-native-r9-build-result.json及相邻隔离目录。

r9未stage、未游戏／XR运行，下一组件只读项仍使用原r7。旧预览调用方目前显式传原先同轴基准，维持旧路径的已有证据；它们不是连续C0基准的新提供方。实际身体面墙、连续视图、合法眼点／脚位、连续体积碰撞及同版受击／枪口消费仍须在现有唯一写入端闭合，72608画面失败与旧C0Goal完整宿主缺口均未据本批关闭。HB后续确认本只读项全收尾后的唯一后继为Longshot034956，前驱Boltok030739保持；无并行runner或新Hook，公开v71不变。

## 2026年11月9日 身体目标继续携带冻结视向与追踪，r10隔离构建

上轮为progress：2c4e6470／4943386f改变了实际读取器及公共抓边数学，r9编译记录可复核。本轮继续沿数据链检查，发现CoverBodyTarget仍只接受一个anchor，即使输入层已经分离视向，也会因用墙面重建committed view而拒绝。按90度墙面、固定观察方向及真实头部组合平移增加回归，旧接口实际出现1个拒绝失败，保存在cover_body_target-separate-view-red.log；未据此启动游戏。

cover_body_target现在显式接受并保存wall／view两个基准，按两者核对原committed view，真实眼点通过view基准映射一次。新增公共cover_physical_in_wall，仅将原始真实位移从固定view参照换算为墙面right／up／forward；CoverInputOwner和CoverBodyTarget共同使用，避免输入与身体目标各算一套符号。原始physical分量继续保留，desired_right／forward为墙面分量；committed_view只含虚拟偏移，desired_eye_world已含一次真实平移，不可再当中立眼点重复叠加HMD。

CoverInputOwnerResult在view_valid时一起保存冻结的wall_anchor、view_anchor与完整StereoResult，包含原始接收／预测时间、头／双眼、参考和代次。begin后覆盖调用者存储的负例验证结果仍来自冻结样本。现有cover_full_goal_live存储层消费这份完整追踪及基准，另与本次Scene和传入Stereo逐字段核对；同序号但不同姿态／参考／状态不再可替换目标，读回也不会刷新追踪时间。墙面基准还须与本次原中立读者／facing一致。此处仍为原有目标存储，不是第二个身体或相机写入端。

新增CoverFullGoalLiveTest复用真实shared consumer／公共owner／broker及目标存储代码，人物、中立提供方及独立view提供方明确为合成seam。23项覆盖同源读取、同序号姿态替换拒绝、独立视向传递、墙面基准变化撤回，以及全部C0／碰撞许可仍FALSE；不把它作为旧BodyC0Goal完整生命周期宿主修复。初次编译因测试全局名称遮蔽触发/WX，原日志保留；改头文件包含顺序、明确生产参数角色名后通过，没有关闭警告。

r10从完整r9冻结树复制，只overlay9项本批源码／测试及新增CMake目标，所有GEARS开关沿用r9。Native完整编译链接exit0，661依赖；DLL SHA256为4b1eb8a1eddfeb9bd85fe028f5cce4715dfd1ad138a69fa79f0cc8a2a54d7e9d。冻结源码上实际执行5个定向目标均exit0：CoverView1135、CoverBodyTarget428、CoverInputOwner120、CoverInputTransaction767、CoverFullGoalLive23。428项包含4种墙向×3种视向的目标、世界眼点与保留FALSE权限检查，以及错误基准／陈旧追踪／错误key等拒绝。原始与最终宿主日志、源码与构建见cover-target-view-checks.json、context-native-r10-source.json、context-native-r10-build-result.json及相邻目录。

r10没有stage或游戏／XR运行，仍未接通实际连续C0基准、合法脚位及体积碰撞，不关闭72608画面失败或头显验收。当前下一只读仍为equipment-controls-prepared-20261108-cover-r7／drivers-owner-only-r7，21驱动核对一致，plan SHA256为8763956cdf027673d47a6e55ec01322b30fda9ae79c0aabbf8e401f19ffb35dc。已直接向唯一前驱Boltok030739提供精确制品／reader身份，待其正常恢复、全跨度视觉、编码和retention全部终态后最终交窗；本只读全收尾后唯一交Longshot034956。正常排队未记blocked，未抢当时48572／32408的实际运行窗口，也未向通知／统筹聊天主动发送进度。

### 72608保留姿态记录的定向核对

为减少补数运行，先核对已归档的同局数值记录。prior-private-text-20261109-021447/done.json明确指向72608／creation134386349955377432的session；三份文件SHA均与当时numerical-receipts.json一致。用实际r7冻结声明生成独立C++解码器，/W4／WX编译、解码均exit0；二进制header／record尺寸、长度／数量、PID及native started_qpc9695846001518与原Native状态互核，未读取新游戏进程。

结果不支持把保留数据当掩体内姿态：cover-pose-phases.bin只有56字节头、episode0，与同局cover_pose_episodes0／file_written1一致。fresh-pose-producer.bin有1个66432字节记录，5个stage均记录115 local且read_mask2047，但source为CT0／slot-1，原published_ns比首次认证CT2观察早15.7602183秒；fresh_certified仍0，不补成写入凭证。control-node-phases.bin有128个1944字节row，session均一致，记录的affected bone只有[8,10,11,32,34,35]，最后after_qpc按该局已保存的10MHz频率换算仍早于CT2约14.6569199秒。不能跨越实际进入／动画变化，把这些旧值当当前头、骨盆或脚位。

此结论只覆盖本次按归档身份选定的三份文件，不声称搜索了全部历史记录。原文件未改，完整关联、尺寸、时序、源码SHA和解码结果保存在retained-72608-pose-review.json、retained-72608-producer.json／controls.json及相邻C++／构建日志。记录同时去掉了先前按排队顺序推断32408武器归属的标签，仅保留实际进程身份，不将窗口顺序当所属会话证明。

下一项具体缺失证据是新鲜原版CT2下的原始C0头／骨盆／脚位。已有r7 CoverPosePhaseObserver具备有界捕获，不需新Hook／writer：未来在本次组件只读完整收尾并取得另一明确窗口后，可在原版进入成功、当前owner／进程creation／Native session／事件名称均核实后只触发一次Local\\GearsVR.CoverPoseCapture.<PID>，由原观察器在下一匹配Tick最多记录一个episode。事件名称只有PID，不能凭名称证明会话；不能消费自动重置事件来探测状态，也不能把SetEvent成功当成C0快照已完成或未知超时后重发。当前21驱动及元数据only作用域未改、没有发事件，也未增加A／Trace／身体动作。实际采样计划仍须结合组件只读结果收窄。

## 2026年11月9日 23420完成只读归属检查

恢复工作时先复核实际进程，原运行handle18152已不存在，Game／XR／录制／编码均无活动进程；没有重启本次检查。原operator回执exit0，launcher exit0、game退出；最终wrapper回执为restored及21驱动一致。原wrapper的工具终态没有留存，因此只记其退出码未知，不由launcher正常退出推填wrapper0。

23420／creation134386441643258345的实际读取越过65700的WorldInfo声明缓存失败。当前世界中唯一mp_warmachine_SMC#3的类仍为StaticMeshCollectionActor，根CollisionComponent为空，Owner随之为空；声明StaticMeshComponents的元素为ComponentProperty、stride8、目标类StaticMeshComponent，但当前数组count0。严格closing与第二次当前人物／UI观察均完成，ownership_read_completed为TRUE。结果只认证本次只读字段，不证明该Actor没有碰撞；实际TraceHitInfo中的组件尚未在本局读取，不能从空数组补造成员，也不能因此放宽Owner／membership检查。无A进入、Trace、抓边、身体写入或捕获事件。

23.1001398秒完整双路总览1页及贴近第二次字段观察的game00265／sim00320原图已亲看：启动第三人称与等待卡后进入实际第一人称自由双手，暂停／B正常衔接；本次没有掩体动作。游戏采样最大缺口1866.6803ms、模拟器266.6672ms，双路末端覆盖通过，不把采样缺口或窗口标题当渲染FPS／实体头显证据。配对编码77923实际terminal0。

Native cleanup／entry restore／pad恢复TRUE、hand_stop5／drained、XR0；原config字节、settings SHA及head原缺状态精确恢复，结束21驱动一致。既有retention21421实际terminal0，删除1761媒体／322581166字节、errors0，保留23420／37116／67780；原始数值、源码与文字保留。再次实际核idle后已最终明确唯一交窗Longshot034956，不在离线工作期间占窗。完整记录在analysis/cover-system-20261108/cover-23420-review.json。

下一证据切片收窄为：在另一次明确交接窗口中，用原版进入取得新鲜CT2；复用已有有界C0观察器单次采样，并用一条原版Trace认证实际HitInfo组件，再只读核对它的类、Owner与当前组件容器。当前空root／空声明数组否定了从这些字段直接选组件的方向，尚不能判定运行时真正归属结构。后续诊断仍不签发collision_verified／grab_authorized／body写入许可，r9／r10尚未实跑、72608画面失败及完整目标保持未完成。

为保留下一条真实命中的身份，原capture现在在有Actor及非空HitInfo组件时，即使未请求surface snapshot，也将该组件加入原严格身份／registry closing；没有更改调用ABI、Trace次数、pending／free条件或表面准入。新增内部只读观察按明确命中的Actor／Component读取Owner、Outer、根组件，以及StaticMeshComponents／Components／AllComponents三个具名反射容器。缺失字段明确记未声明；已声明容器须通过原类型、元素Class、8字节stride、256预算及完整closing，不扫描其他组件替代命中。容器成员出现次数和类型关系只作为观察，所有碰撞／抓取／身体权限始终FALSE，旧严格surface membership规则未变。

实际45个针对unittest通过（Trace32、原surface13），新增案例包含空集合、不同Owner、其他具名容器、错误元素类型及identity、Owner／空header／元素链接在closing改变等。正向观察后仍断言旧surface拒绝，不能把可读字段升级为安全证明。日志在explicit-hit-observation-tests.log；此读取器变化尚未运行进游戏，Native／PG／XR无需据此重编。

## 2026年11月9日 r11将既有只读姿态观察移至视图拒绝之前

准备单次C0请求时复核真实r7控制流：capture.cpp先进行view选择，在missing_basis处return，之后才执行cover_pose_phase_live::scene。因而该明确拒绝路径也会跳过只读Owner更新，不能指望仅发事件就得到新鲜掩体姿态。72608有missing_basis163是实际计数，但当时没有发捕获事件；本次源码发现不是“已证明所有零episode的运行根因”，也不是72608画面修复。

仅将同一现有Scene调用前移至已认证StereoResult、同张票据的L3回中及最终reference确定之后、任何view选择之前。仍在原stereo_mutex与同一Scene／线程内，只调用一次；全部Owner、100ms、代码前缀／caller、正常返回和当前世界检查保持，missing_basis拒绝原文保留。没有增加观察器、Hook、骨骼／人物／相机写入端，也没有让缺基准帧获得投影许可。当前共享capture.cpp只施此小移动，其余武器dirty内容与共享Git index未纳入本次冻结。

隔离r11直接复制准确r7冻结树，逐文件比较只有capture.cpp变化；93个GEARS开关沿用r7，没有混入r9／r10身体数学或其他新武器源。Native编译链接exit0、661依赖，SHA256为6a6a1f9a7e3a78f16076da933b43c27a979ed8cccd715b53968b26087617cc13。实际运行原PosePhaseLive149检查与RigTrackingCoverContext74检查均exit0；前者复用8段完整历史代码SHA认证的Host fixture，保留丢失stage、异常、身份变化、重复／并发、错误参数及8episode上限，不是游戏证据。source／build记录在context-native-r11-source.json、context-native-r11-build-result.json。

为避免把同文件的其他dirty编辑一起提交，另以临时index从当前HEAD只加入这一次移动，保存来源检查点cdb7bac2bf6dba4e0615c3cf2c0a07ec57bcb805及refs/codex/checkpoints/cover-observer-before-view-20261109；共享分支HEAD、真实index及工作树其他内容未由该操作改变。此检查点不是发行标签或另一开发工作树；本轮读取器与23420证据已在正常分支提交e9ec4c54。

下一动作仍只原版A一次进入、当前CT2下既有自动重置事件一次、一条原版Trace及明确命中组件只读，不执行区间二分或抓边。事件请求先核game creation／action_owner／Native SHA／started_qpc／原观察器认证、零episode、100ms观察，然后在SetEvent前持久化unknown；失败、未知或之后会话变化均不重发，不客户端Wait／Reset该事件。4个定向测试方法含15类准入拒绝与一次调用／缺失事件／未知异常，通过；SetEvent成功仍不算已保存C0。

旧df787ea1的r7动作计划在运行前明确撤回，保留原档；最终prepared为equipment-controls-prepared-20261109-cover-r11，PG仍f8e6f3c2、XR仍1684，21驱动与完整源码归档drivers-hit-pose-r11，plan SHA256为c6e3e0c67d49d61fce1745234699157a5642628343056e45eb044c62bde08579，prepare-only通过。已经向唯一前驱Frag更新身份，未stage／run；接受的交接为Snub042943→Frag050613→Cover本项→Longshot050328→HB050438，以实际最终交接为准，不因idle启动。

离线解码器按r7真实Header／Row／Episode声明生成，static_assert尺寸56／84936／594736；复用冻结r10已有Marcus全身模型与头部marker计算原始世界姿态，不制造Core或中立校准。MSVC/W4/WX编译0；72608真实空文件读出count0，错误session／snapshot尺寸及尾随字节拒绝；原观察器Host生成的8episode文件可解码，并分别拒绝异常、并发、重复及错误参数的row。该Host为合成骨架，未被解释为Marcus几何。未来实际游戏文件仍需与该局数值归档SHA、原Native session、CT2观察、世界／人物／mesh及时间互核。原始头／骨盆／脚位仅是测量，不直接授予中立基准、扫掠、C0写入或命中／枪口权限。

最终再显式pin事件复用的既有run_stereo_continuous.py及launcher.py，直接依赖由21增至23；没有再改代码、动作或制品。c6e3e0c6的direct21计划另存未运行档，最终plan SHA为4f17b9da85afd2dd540982d19401ecbabd9fff8e446ca8b050b5f4afc1203232，prepare-only0，已向Frag给出准确新身份。另将保留Marcus115骨骼载荷放入明确合成的Host owner／episode，离线解码的世界头／骨盆／双脚及作者眼点分支均成功，非有限值和单位四元数检查通过；此夹具不是游戏内掩体采样，neutral／atomic／body权限仍FALSE，记录在decoder-synthetic-owner-retained-marcus-review.json。

## 2026年11月9日 实际61492取得当前组件与掩体姿态

接收Frag15368最终交接时保留其Native cleanup_timeout1／drain0／entries0／launcher1，不能改称正常退出；独立核实其Game／XR／录制／编码已不存在后才stage本r11。运行前补了一处拒绝回执：若具名容器越过256预算或类型非法，保留此前已读的明确组件及Owner，标明closing_verified／read_completed为FALSE，不丢失原始拒绝值。原限制与surface准入不变，46个Trace／surface方法通过，源码提交e4e2d09f。最终实际运行计划SHA为69b77bd1ef94fe8b3d94d16d99b28c57937e1de6e34ec30e3c230092dedc27f6／23依赖，准确源码另存drivers-hit-pose-r11-owner-rejection；此前4f17计划保留为未运行档。本局没有触发容器拒绝分支，不将它记作该分支的live覆盖。

实际61492／creation134386467119153096、Native started_qpc9812988712167，仅普通移动及一次原版A进入CT2／CA0；既有姿态事件只SetEvent一次，原版Trace只提交一次。Trace accepted、queue state2、pending FALSE、请求内存释放；之后独立只读query对同一roles与明确HitInfo组件完成closing。实际组件为StaticMeshComponent#186308，Owner与Outer均等于当前mp_warmachine_SMC#3；根CollisionComponent仍为空。StaticMeshComponents为typed StaticMeshComponent数组、count0；Actor.Components和Actor.AllComponents均为typed ComponentProperty／ActorComponent数组，count各100，明确命中组件在两者中各出现一次。没有从数组另选组件、重复Trace或发出抓边请求。

这次实际字段证据给出下一读取修正方向：旧_collection_member只支持StaticMeshComponents唯一成员，不能覆盖本次真实运行容器。后续按当前继承声明认证Components／AllComponents成员、Owner及Outer，并继续保留world／类型／身份／变换／closing检查；不能把数组存在或本局一次成员读回升级成整个人体扫掠／grabbable证书。当前严格surface路径未据本局自动放行。

CPOSE文件594792字节，SHA256为f62f9bbe66c4a4e1fc60e0de87b47397a3e9269c74bd5f2da1a459554a36a018，header session与原Native完全一致，1episode closed，exception／concurrent／nested／duplicate均0。seen／accepted均0x73：C0、C1、late P0／P1与L1共5个row有效，主physics P0／P1未观察到，不补成7stage完整。5个row均为115 local／reference，按该局已保存的10MHz QPC换算落在请求前后实际CT2观察区间内；World／Pawn与Trace角色吻合。源文件原样保存在61492-numerical及其receipt.json，解码在cover-61492-pose-decoded.json，未拿旧CT0数据替代当前测量。

现有Marcus marker实际解出C0作者眼点约(-3935.878,-138.288,574.440)，骨盆约(-3931.004,-187.414,546.005)，左右脚约(-3907.025,-166.583,520.609)及(-3962.637,-185.897,520.311)。5个有效stage的作者眼点相同；这不是原子消费、渲染／命中或合法中立校准。实际component-to-world三轴长度约1.000000044／1.000000044／1，当前数据不支持将它归因为非单位整体缩放。没有签发C0写入、碰撞、射击或抓边许可。

完整27.5334801秒双路1页和4张贴近原版进入／C0时刻的原图已亲看。game00340／00343是等待帧卡；sim00397／00401仍可见身体与装备近裁剪遮挡。完整掩体画面继续FAIL，不能把只读观察已恢复等同于视图已修复，双窗口样本也不是同一C0／GPU draw的原子对应。启动第三人称／等待、移动及HUD的旧缺口仍保留。双路末端覆盖通过，game最大捕获缺口1616.6839ms／sim316.6693ms，不解释为FPS或游戏停顿。

本局runner69903／operator／launcher与配对编码27561均实际exit0；Native cleanup／entry restore／pad TRUE、hand_stop5／drained、XR0、Game退出。config原字节、settings原SHA及head原缺精确恢复，结束23依赖一致。既有retention57724实际exit0，删除2064媒体／344281505字节、errors0，保留61492／15368／36696；再次核实际idle后已最终明确唯一交Longshot050328，后继HB050438。本会话不追加本窗，转离线修当前容器认证与真实眼／骨盆／脚位提供方。全部结论在cover-61492-review.json；公开v71不变，完整掩体与实体头显仍未完成。

## 2026年11月9日 当前集合容器认证与真实姿态目标准备

上一目标轮为progress：61492实际字段／姿态证据、r11改动与正常收尾改变了下一步。当前分支仍codex/6dof-single，既有共享capture.cpp其他dirty内容不纳入本批。当前_collection_member已按61492的真实关系补充严格路径：只有原StaticMeshComponents声明数组为空且明确命中组件Outer为本Actor，才分别认证继承自Actor的Components及AllComponents。两者均须为ComponentProperty／ActorComponent、8字节stride、当前声明与元素Class及继承链有效，明确组件各唯一一次；原非空声明路径不变，歧义／非成员／错误Owner／World／变换继续拒绝。所有header、member bytes与类型链接仍加入原closing。没有由数组挑替代组件，不增加Trace、放宽16384／3秒读取预算或签发碰撞／抓边／身体许可。

先用真实形状的空声明＋两个100元素容器回归复现旧拒绝（actual-containers-red.log）；修正后49个Trace／surface方法通过。新增12类准入反例及8类closing变化覆盖两个容器、元素类型、声明类、继承、Outer、Owner、World与矩阵。第一次联合检查的错误World反例使用了fixture中不存在的地址而抛KeyError，保留原日志；改为已存在的错误Pawn对象后原World拒绝通过，没有改变生产读取器的异常或准入规则。该新surface路径尚未在游戏运行；61492只验证了原Trace和独立字段观察。

另按61492原done与numerical-receipts.json取回同局fresh-pose-producer.bin（SHA dc272fc7e3707ebd19df81e38605d4470863d0608d60540a4ec832bdd4ecadcc），与已存CPOSE互核session、PID／creation、World／Pawn／Mesh及完整115参考骨字节。FPOSE为旧CT0普通姿态，只作该人物站立校准来源，不重标成当前CT2或刷新Core时钟。离线probe复用真实生产planner、marker和adapter，没有新造当前Core。原C0几何目标原样规划成功；将站立中立整体降到实际掩体眼点时，solver虽成功，但实际骨盆Z约514.38，低于已测脚位约520.31／520.61。反之将骨盆硬固定在实际546.00并改为面墙／水平视向，preserve-C0与reference-neutral两种姿态分别超出脊柱可达范围，未删原spine_unreachable。仅保持骨盆高度、沿XY调整的有界候选可解，但候选尚无碰撞或姿态校准权限。

新增纯目标准备cover_posture_fit.h，在实测骨盆高度不变的平面上求头部最大可达球、两个固定脚位腿长球及明确水平位移上限的交集；随后仍调用原完整planner验证。头／脚目标、骨长、原始Frame和原残差限值均不改变。输入仅允许fixed_cover、world_absolute、零physical／grip的中立目标，禁止把当前HMD位移送入校准；骨盆低于脚位、错误源、超预算、不可达或最终solver失败均保持输出原字节。该模块只返回数学候选，不调用引擎、写内存或授予碰撞许可。

在61492真实骨骼及明确派生的中立Goal fixture上，preserve-C0／reference-neutral分别水平调整约11.69746／13.27997游戏单位，保持骨盆Z546.004803及双脚，原solver验证通过；3个刚性世界旋转／平移、零预算已可达原姿态、错误源／HMD与Grip非零／预算不足／别名／输出不变等共60检查通过。直接MSVC/W4/WX与隔离CMake新目标均编译及执行0，218依赖。新目标只在明确指定私有fixture时注册；未跑旧完整矩阵或游戏。隔离构建第一次字节比较发现旧冻结头文件LF与当前CRLF差异，原失败保留；所选8项真实依赖逐一记录SHA，旧body planner等文本除换行完全相同，未混入其他武器实验逻辑。见cover-posture-fit-cmake-result.json及posture-fit-source-dependencies.json。

当前posture fit尚未接入Native C0消费者，也未重编／stage新的运行Native；本批不能关闭61492等待帧／穿插问题。后续先将当前姿态与中立目标绑定到同一真实owner／Tick和参考，再在已有唯一写入端接入，并补实际连续碰撞、双眼、受击及枪口证据。公开v71及完整功能验收状态不变。


## 2026年11月9日 实测髋高拟合接入中立目标转换

上轮为 progress：实际组件容器认证与实测髋高拟合器已保存。随后完成需求文档核对，补齐24项功能与22项验收的对应、术语及合同内可调初值；本节继续实现，不改变需求完成标准。

新增 cover_neutral_goal.h，复用既有 body_goal_view_adapter、measure_cover_pose 和 cover_posture_fit。原 prepare_cover 的身份／时效／墙面与独立视向头部pivot检查提取为 place_cover_head，原接口继续经同一检查后合成追踪。新中立准备先从当前原始C0测量髋高，以独立placement指定的眼点、身体朝向、连续视向和脚位生成纯数学pre-HMD目标，再用实测髋高及显式水平预算拟合；成功后返回固定Neutral供已有追踪转换使用。没有更改原Core、刷新时间或把拟合结果签成碰撞／写入许可。中立值须由实际owner在合格中立边界建立一次，并随当前掩体基准失效，不能逐帧用动画或上一输出重校准。

同局61492原FPOSE仍是CT0，原字节和原时间未改。probe仅导出同局站立校准及当前C0骨架的owned-buffer结果；新测试明确在CPU内构造掩体键，不把该Core称为实际CT2来源。两种姿态策略以原壁面法向及进入前观察yaw=-4667分别给身体／视向；preserve_c0与reference_neutral的水平拟合分别10.1788092及11.7528148游戏单位，髋高均保持546.004803，原脚位和目标头位不变。它们还没有当前场景可通行证明。

新增CoverNeutralGoalTest实际84检查、0失败：包含保留站立校准直接转换产生低于脚位的原情况；两种姿态下实测髋高／脚位／固定骨长求解；真实HMD位置及旋转不改变中立拟合；真实平移和已提交虚拟偏移在后续目标各加一次；17类身份、代次、时效、单位／骨架／零预算等负例，以及别名、失败输出和输入不变。原BodyCoverGoalAdapter的designer／Marcus各326检查、12个全身目标通过，原BodyGoalViewAdapter159检查通过。

隔离源码由上一posture-fit-source复制，递归核对本次测试实际quoted include依赖，仅相应目标转换／新模块文本改变；CMake只增本次目标及可选fixture登记。MSVC /W4 /WX编译与4次执行均exit0，实际编译依赖240。原始结果为analysis/cover-system-20261108/neutral-goal-build-result.json，源码与依赖为neutral-goal-source／neutral-goal-source-dependencies.json；原始fixture及派生来源说明见neutral-goal-fixtures.json。没有编译新的运行DLL，也未stage／启动游戏。

继续核对到源码上的启动依赖：capture.cpp中missing_basis返回仍早于rig_tracking_live::publish；copy_scene_metadata_owner消费的是同一成功Scene发布的Core和owner。continuous_hand_owner只有取得该owner才能开始Tick／C0，而body_c0_goal_policy_live须在已captured上下文中才能sample_current_cover_pose，随后仍明确拒绝cover。这说明r11前移只读调试观察不等于接通实际中立／身体来源；也不能据此删除视点或Core准入检查。下一项是沿现有owner与Scene元数据通道建立不依赖最终相机输出的当前C0接线，同时保留原始追踪来源与唯一身体writer。合法中立眼点、连续体积碰撞、实际双眼、受击及枪口闭环仍待证。

### 当前已登记运行切片与后继

reader-r12仍冻结plan b5e097b018a98bd1d576d088e60cb80a778ebd8a4a4853ca3035dc370c42373a，23依赖，prepared仍r11 Native6a6a1f9a／PGf8e6f3c2／XR16840b27。本切片仅原版A一次、最多8条原Trace／30秒的一个边缘cross-section，严格当前实际命中组件与Owner／Outer／容器／几何closing，不发C0事件、不抓边、不写身体；上述中立模块没有混入该Native。实际源与plan核对、stage和运行仍须在明确交窗后执行。

Longshot060433已明确接受Cover r12为唯一后继，Cover确认身份与Scope不变；其前链Frag053206→HB055335→Frag053956保留。本段之后没有更早受理后继，已直接登记Snub060545-input-loss为唯一后继，对方确认。必须等待060433最终完整恢复、双路全跨度及原图审看、编码／retention终态的唯一交窗，并独立核实际进程；不据idle抢窗。以上是排队与源码进展，不是新游戏结果。Publicv71、历史计分及实体头显／完整掩体验收状态均不变。


## 2026年11月9日 61492眼点拒绝的生产读取器复现与r13候选

上轮为progress：1b488ea5接入中立目标拟合并完成隔离检查。本轮先复核既有61492原Native日志：first_person_missing_basis_rejected=504，authored_eye_applied=232、authored_eye_rejected=504、uncomposed_view_rejected=1；对应rig_tracking_published及body_goal_attempts均232。它们是整局累计，不能证明504次均为同一原因。

进一步对照同局当前CT2几何读回，Pawn.Location为[-3935.678955,-167.228714,579]；已保存原始CPOSE row0的Mesh component原点Z=505，实际SpaceBases及已认证Marcus眼部挂点给出的眼Z=574.443539997。该值与从Local做FK得到的574.440407314是两份各自取样的结果，不能混称原子同一姿态。旧first_person_authored_eye明确要求eye.z-Pawn.Location.z至少为0.1米，因而会拒绝这处合法低位模型眼点；其下限把Pawn胶囊基点误作眼睛必然应在上方的基准。

新增FirstPersonAuthoredEyeLiveTest直接include真实生产inl。fixture保留同局C0 metadata、SpaceBases、reference与独立Pawn位置；Scene／registry／锁准入是明确合成seam，CT0原Core只取同局mesh身份，不修改时间或宣称当前CT2 Core。实际旧代码RED为6检查1失败，未改输出；仅更改生产下限后GREEN，读出上述原始SpaceBases眼点。修正以当前网格空间中的实际眼部位置p.z检查0.1米下限，原Pawn水平范围与上限、mesh master、asset／reference／骨架签名、SpaceBases／component／head矩阵以及视觉owner closing均保留，没有新读者、hook、身体writer或追踪／输入准入放宽。

最终52项宿主检查通过，包含原低位眼点、仍合法的站立类高度、18类读取／身份seam／header／component／head变化和非法高度拒绝、失败输出保持及恢复后重复读取。最初两次测试编译使用错误ObjectIdentity索引字段，修正为实际registry_index；这些编译失败日志保留，不作为产品运行证据。复现与数据来源见analysis/cover-system-20261108/authored-eye-61492-evidence.json及authored-eye-host-red.log／r13-FirstPersonAuthoredEyeLiveTest.log。

r13完全复制实际r11冻结树，只改变first_person_authored_eye.inl、对应新宿主测试和CMake登记；共享capture.cpp及其余生产源码未overlay。93项GEARS开关与r11逐项相同，Native全编译／链接exit0，661实际依赖，DLL SHA256为5cf4adea9df910fd35d6a9a4ace70a317dfb3c9edfd05fc1c27b249dd7e88380；同冻结源52项宿主及原FirstPersonViewTest均exit0。源码／构建分别为context-native-r13-source.json、context-native-r13-build-result.json及相邻目录。

本项收窄了下一步：先验证既有独立authored-eye入口恢复有效掩体Scene后，原owner／C0链是否随之获得当前上下文；当前证据尚不要求新增Scene元数据来源。body_c0_goal_policy的cover准入仍明确拒绝，合法中立眼点／体积碰撞／当前全身提交及同版view、hit、muzzle接线仍须实现。不能用此离线修正关闭61492实际等待画面、身体穿插或完整掩体验收。

### 同一已登记窗口更新为reader-r12＋eye-r13

在尚未stage／运行前，将原b5e097b018a98bd1d576d088e60cb80a778ebd8a4a4853ca3035dc370c42373a计划逐字归档为cover-boundary-runtime-plan-frozen-reader-r12-native-r11.json，重新冻结当前plan为6838d7b93804c01aac3e101d8d20be602bcead458714d02459067faae1b7e274，prepared为equipment-controls-prepared-20261109-cover-r13。23驱动中只有本段wrapper的精确prepared名称由r11改r13；其余22项SHA不变。驱动副本及plan保存在drivers-reader-r12-eye-r13，所有制品／源清单与prepare-only均核对0，staged／started仍false。

Native换为上述5cf4adea，Playground仍f8e6f3c2715b244e8250976c3eb1279cb306c2a50576e078f64bf8b23daf4807，XR仍16840b27dc40ebb489f93121758cb7bc0daa2700df046e508babbd50c578a7fc。动作仍原版A一次、最多8原Trace／30秒单个截面；不发C0事件、不抓边、不写身体、无新增动作或slot，中立拟合模块没有编入这份Native。已向唯一前驱Longshot060433及唯一后继Snub060545直接通知准确新身份，旧计划不再用来stage；等060433最终完整交窗，再独立核实际进程后执行。

Boltok063023-hidden-prepare的新尾位请求已转当前后继Snub按其实际尾序直接登记；不能越过任何已受理项。此队列协调不代表运行或接窗成功。本轮公开候选、旧失败、历史计分和完整／实体头显验收状态保持原证据范围。


## 2026年11月9日 掩体目标使用当前V3追踪并复核实际C0掩体键

上轮为progress：fda26ad9明确复现并修正了当前眼点下限，r13已冻结待交窗。本轮检查发现普通身体已有BodyTickSource重载，而掩体测量／placement／目标转换及中立拟合仍只接受旧Scene Core。实际V3契约允许旧Scene作为不改时间的身份关联，并独立取得当前Tick元数据与当前C0追踪；若掩体强制按旧Core追踪时效处理，会错误拒绝当前输入或使用旧头位。

body_goal_view_adapter现在为measure_cover_pose、place_cover_head和prepare_cover增加既有BodyTickSource::Input路径。继续使用原classify：当前追踪40ms、Tick元数据50ms，以及原队列／publisher／cohort／reference／身份规则，未放宽任何现有判据。数学阶段仍保留原Scene／publication等来源键，测量的observed_ns取本次原始metadata_end_ns；不改Core.published_ns／Scene QPC或任何追踪时间。V3 placement须处于该次metadata_begin_ns至metadata_end_ns的时段内，不能把旧Scene时间换成调用时刻。真实HMD只在最终转换时取该次独立stereo；中立拟合仍先用reference构建pre-HMD目标。

cover_neutral_goal复用同一Core／V3准入重载，原Core路径继续拒绝过期Core。实际167项检查通过，包含旧Scene保持原字节而当前V3可用、旧接口仍拒绝、当前位置与旋转不改变中立、使用当前真实平移、15类身份／时段／cohort／reference／publisher／超时负例，以及覆盖整个Input的别名拒绝。首轮合成cohort遗漏同步controller.predicted_time，原classify正确以0x400拒绝；修正测试数据后通过，生产合同未改。原body adapter designer／Marcus各326、普通159检查继续通过，结果为context-native-r14-build-result.json及cover-neutral-current-first／diagnose／final.log。r14仅编译，未stage或运行。

进一步复核到V3的parent仍是旧Scene关联，因此实际C0测量还必须重新读当前CoverType／Link／slot，而不能由纯转换替调用方认证。cover_context_live新增同步sample_current，复用现有current_tick_actor及同一typed property、完整slot布局／数组／selected bytes和前后closing；不需要伪造更新Scene，不发布新追踪或writer许可。CurrentKey保留实际Link身份、数组header、槽位原字节、CA／LastCA及transition，供同一次测量前后逐项比较。

body_c0_goal_policy_live::sample_current_cover_pose在原context prepare检查后读当前Key，与parent中完整Link身份／slot／type核对，再按已启用的V3或原Core测量；最后重复当前Key及context／stop／revision检查。当前退出、换槽、表面布局或动作变化不能借旧Scene名义变成有效掩体测量。此处仍是原来的只读测量，随后cover分支仍明确拒绝身体提交；没有删除cover gate或增加C0写入端。

实际CoverContextLiveOnTest206项通过，包含原Scene读者、当前非掩体和高低类型、当前slot内容变化、12类当前身份／权限／字段／数组／读中变化／错误线程／停止拒绝、失败输出与读者不改原内存检查；Off配置也编译执行0。新的r15准确复制r14冻结树，仅overlay当前cover_context reader／对应测试和body goal测量接线。原C0完整生命周期Host旧夹具仍未修复，不能把这些reader和纯目标检查称为实际C0提交／回滚全路径通过。

r15 Native全编译链接0，661依赖，93开关逐项与r14／r13一致；DLL SHA256为b84973cd67859277cccc58443d8c0e6d1042c1c5beb063c4a9ede4e8f669b720。准确冻结源的On206、Off及CoverNeutralGoal167执行均0。最初构建误写不存在的CoverContextLiveTest目标，Ninja明确拒绝；失败目录原样保留，改用原有On／Off目标在全新build-context-native-r15-corrected构建，未绕过build_verified的空构建目录规则。首次手工读者编译缺MinHook include路径也保留，修正为原tools/third-party/minhook/include后通过。证据见context-native-r15-source.json、context-native-r15-build-result.json及相邻原日志。

当前运行计划仍为6838d7b93804c01aac3e101d8d20be602bcead458714d02459067faae1b7e274／prepared cover-r13／Native5cf4adea／PGf8e6／XR1684；本轮没有改23运行驱动或已登记制品，没有将r14／r15混入该段或新增slot。Longshot窗口尚须其全部视觉／编码／retention及恢复终态后的明确交接。完整中立provider、合法脚位与体积碰撞、抓边提交、双眼／受击／枪口及实体头显仍未闭合，Publicv71和历史评分不变。


## 2026年11月9日 61864实际r13：眼点拒绝清零，几何closing首败保留

收到Longshot实际8092全部恢复、全跨度视觉／编码／retention终态后的唯一明确交窗，独立核实际idle及23驱动／制品SHA，再按6838d7b93804c01aac3e101d8d20be602bcead458714d02459067faae1b7e274计划stage r13。该段没有使用r14／r15，源码接线与实跑证据分别保留。stage归档prior-private-text-20261109-064948，执行计划原样保存为cover-boundary-runtime-plan-executed-61864.json。

实际game61864／creation134386518083637581，owner51584／creation134386518002561062，Native session started_qpc9863948299927；Native5cf4adea／PGf8e6／XR1684。原版A一次到CT2／CA0，当前Pawn位置[-3931.990234,-162.364182,579]。随后独立geometry读取在约71.545ms的观察段末端报告“World-state metadata/registry changed at closing boundary”，operator实际exit1，整段passed=false。Trace尚未启动，实际调用0；因此r12新增当前Components／AllComponents成员认证分支仍未取得游戏执行证据，不能把此次A成功当作该读者通过。

原world_state.Query.finish已经区分identity／metadata／registry并保存closing_rejection，但cover_geometry_state.capture只带出了统一异常字符串，实际是哪一字段／对象／header变化在61864回执中缺失。不能据统一报错推断对象表增长、缓存或游戏故障，也不能在进程退出后恢复未保存的读回。下一项应保留该既有结构化closing witness并做定向检查，继续保留严格closing，不能用允许变化或盲重试绕过本次首败。

r13运行日志中authored_eye_applied352、authored_eye_rejected0、missing_basis_rejected0、uncomposed_view_rejected1；rig_tracking_published313、body_goal_attempts312／rejected83。相比旧61492的504次眼点拒绝，本段该拒绝未再出现；这是准确r13候选的限定运行结果，不证明旧504次全部同因、所有掩体或实体头显已修好。原Scene／C0链已有本段新的调用计数，不能将计数当作完整中立身体写入或当前脚位／碰撞通过。

已亲自查看全21.0167866秒的paired-review-00.jpg及四张实际原图game00352／00363、simulator00426／00437；原图按A后实际观察QPC附近选取，各自时间在61864-selected-visual.json。两路当前掩体附近已显示实际游戏视野；右侧身体／装备仍大面积近裁剪遮挡，完整视觉仍FAIL，启动段仍见第三人称。WGC为369游戏／443模拟器采样，末端覆盖过，最大捕获间隔1.6500237／0.2666679秒，不将采样或窗口标题FPS当实际渲染率／原子C0-GPU配对。

runner62662实际terminal0、launcher0，operator1真实失败保留；paired编码90737实际terminal0。Native cleanup／entry restore及pad恢复TRUE，hand_stop5／drained，Native pending0／resources_retained0／cleanup_timeout0，XR0、game退出。XR回执为实际terminal exiting／objects destroyed／shutdown completed，未把其end_session_succeeded=false写成已调用EndSession。原config字节、settings SHA及head原缺状态精确恢复，结束23驱动一致，closure-proof和wrapper回执均保存。

既有retention55213实际terminal0，删除1207媒体／203868906字节，保留61864／8092／68512，最多3局／48小时，源／文字／数值未删。再次独立核idle后已最终唯一交窗Snub060545-input-loss；本窗没有追加case。完整记录analysis/cover-system-20261108/cover-61864-review.json，原session为analysis/runtime/continuous-stereo-61864-20261109-065042-562f7bb5.json。当前身体面墙、中立provider、合法脚位与连续体积碰撞、真实抓边提交、受击／枪口及实体头显仍未闭合，Publicv71和原历史评分不变。


## 2026年11月9日 reader-r16保留几何closing的原始拒绝证据

上轮为progress：035f9d7f接好当前V3／C0掩体键，并将实际61864首败、眼点出图及全收尾保存为5b32174c。本轮依据该首败检查封装链：world_state.Query.finish原本已有identity／metadata／registry分类、地址及前后值／header，但cover_geometry_state.capture捕获异常时仅存字符串。新增代码只在已有真实closing_rejection时deepcopy到结果；query尚未建立或没有该证据时不制造witness。不增加进程读取、循环、retry、append-only容许或Native调用，所有权限仍FALSE，Query／CoverQuery.finish及原closing规则未改。

test_cover_geometry_state新增两个测试方法，使用原实际Query的内存夹具分别触发身份、属性布局metadata及registry变化，核对精确before／after和header、复制后不再受原字典修改影响，并核对早期读者不可用时没有伪造witness。原代码实际RED缺closing_rejection；首轮metadata夹具误改值／struct指针而非Query.metadata所记录的0x70布局块，原失败日志保留。修正夹具目标后，全部13方法实际0，accepted／collision／activation等原断言继续保留。日志geometry-closing-witness-red.log及geometry-closing-witness-confirmed.log。此处修复了拒绝信息丢失，没有证明61864具体变化原因，更不能填补它没有记录的前后值。

冻结下一有限检查plan859bb156623dc81f31762629473bddded77795c3e78c8b88f836ed1683e39b15，23驱动中仅cover_geometry_state.py变化，副本在drivers-closing-witness-r16；prepare-only0。继续准确equipment-controls-prepared-20261109-cover-r13／Native5cf4adea／PGf8e6／XR1684，不重编／混入r15。本段仍原版A一次、最多8ray／30秒单截面，首个严格失败即停，无新C0事件／抓边／身体或碰撞许可；旧61864执行计划保留为6838d7b9，不据新plan改写旧结果。未stage／game，须排当前已受理尾项之后并收到唯一最终交接才能运行。

## 2026年11月9日 原版三角形体积函数的初始重叠语义

上一目标轮仅定位并核对已有需求稿，没有改变开发接线或下一动作，按开发目标记为no progress。本轮重新核实当前工作树、实际进程及既有导出，继续CV-16／17的碰撞认证；没有把文档齐全当作掩体完成。

沿用reverse-engineer-anything流程，复用已保存的skeletal indexed geometry调用链，将问题限定到历史原代码0x1401b697c及其axis helper 0x1401b711c。原始code blob SHA911ab9bc、data blob SHAc5deb081与磁盘游戏EXE是不同证据。私有IDA库导出的主／冷块及helper分别2086／725字节，逐块匹配原code blob；56条相对控制流边全部落在这些原块内，13个直接调用均指向该helper，没有外部／间接调用。实际RIP常量按指令disp32解析后与独立data blob核对，未将IDA显示的cs标签误作真实地址。

REA的IDA provider未配置，直接IDA connector因缺ida_nexus退出；没有安装／修改provider，改用现有官方IDA CLI与既有IDAPython导出方式，关闭自动分析并正常qexit。最初axis优先级检查因ctypes缺64位HANDLE声明而失败，原失败保留；修正声明后实际GetPriorityClass为16384。leaf原导出中的priority_below_normal字面值只是未核实请求，不能引用为已确认优先级。原库复制前后SHA仍698dc321，未修改原库；所有自有IDA进程已退出。

新增ignored离线harness仅在自有进程保留三个精确64KB内存块，逐片校验SHA后先写入再设code为RX、data为只读，无RWX、不覆盖既有映射、不接触游戏。MSVC /W4 /WX编译通过；第一次编译缺MAXULONG宏的日志保留，改用标准numeric_limits后通过。实际回放进程72408确认BelowNormal，七个合成三角形／盒体样本全部符合预期，进程正常exit0并释放自有映射。

原函数实测：静止初始重叠返回1、time0、零法线；静止相切同样返回1／time0；从z2扫到z-2、盒半高0.5时接触time0.375且法线+Z；从重叠向外移动仍time0，法线为运动反向而非穿透深度或已认证恢复方向。静止远离及横向分离返回0。已有更近time0.1时，原接触time0.375的候选返回0并保留原time／normal，所以该detail返回0不能被单独解释为全局空空间；time0或零法线也不能把已发生接触抹掉。

核对当前cover_trace_query：它消费完成的原Actor.Trace对象返回与原hit字段，既有边缘截面仍是Extent0射线，没有直接调用上述leaf，也明确不授予collision／grab／body许可。因此本轮没有凭历史leaf给生产查询加不适用的语义或放宽guard。这七例只认证该历史数学函数及本次参数解释，不能证明当前StaticMesh／世界形状分派、静止初始重叠的broadphase覆盖、动态碰撞、self排除、physics fence或同版hit／muzzle消费。

完整数值、字节身份、失败与范围记录在analysis/cover-system-20261108/ida-volume-leaf-r16/findings.json及replay/replay-result.json；派生原代码片段保持ignored，不进入源码或发行包。r16运行plan及23驱动未改，未stage／启动游戏。当前明确队列保留Frag071258→Longshot070405→HB070119-refire-supply→Cover reader-r16→Snub071103-charging-observer；仍须前驱完整收尾后的唯一交接及独立idle。完整碰撞、抓边身体提交、双眼／受击／枪口和头显验收仍未闭合，Publicv71及历史评分不变。

随后仅在新goal-host-r17输出目录对准确r15冻结源的BodyC0GoalPolicyLiveTest作/W4/WX编译复核，实际exit2。首批缺失是旧host_native_live_prefix未提供当前ComposeObservation、MainMountQueryResult、PendingBranch、safe_copy前置声明及metadata诊断等合同，后续出现连锁编译错误；没有进入执行。原log、命令及四个输入源SHA保存在goal-host-r17/compile-findings.json。此结果明确原Host集成覆盖仍缺，不能由reader／目标数学通过替代；没有删生产分支、加宽松stub或重编DLL。再次逐项核对23驱动与plan859bb156完全一致，实际游戏／XR／自有IDA均无遗留；不将当前idle视作已交接窗口。

## 2026年11月9日 修复身体目标Host接线并验证掩体测量关闭路径

上轮为progress：原版volume leaf的初始重叠回放完成，且准确复现旧Host编译缺口。本轮从r15冻结副本开始修复测试入口，再在当前工作树的正式生产include上完成编译与执行。只改测试前缀、两个测试入口、共用宿主夹具和CMake测试登记，没有改生产Goal／backend／Kernel／掩体reader、运行DLL或开关。

host_native_live_prefix补用正式full_body_c0_native_live、broker、fresh producer、tracking probe及hand-part头，提供真实NativeKey及正确前置声明。原来的简化Property／Proof重复定义移除，使用正式reflection类型和helper；只对原已声明为Host边界的actor／property／metadata读取重命名隔离。两个入口共用新增host_native_goal_seams，Broker明确不可用，视觉样本和未建模数组／矩阵读返回拒绝。没有让测试stub签发覆盖或退休证明；原Registry、Backend、CAS128、Kernel、EpochGate与真实Goal函数继续执行。

编译进入执行后，原synthetic-core-current夹具的published为1999000000ns而Scene只有12100ns，当前40ms准入正确拒绝。只在owned Host副本明确构造与夹具时钟一致的Scene QPC；原文件和生产时间检查不变。另一个原反例将Component矩阵的零值m00乘2而没有改变矩阵，改为将完整第一行乘2，实际形成非法非单位缩放。夹具加载失败现在立即退出，避免继续使用空输入。最初路径错误、字段名称／模板类型错误和各轮编译失败均保留，未将其改写成通过。

BodyC0GoalPolicyLiveTest的0–23模式实际453检查通过：原普通身体提交、原函数异常、drain、错误结束入口、姿态／旧来源拒绝、中立随Component移动、held-view失效继续通过。新增16–23八个掩体模式调用真实sample_current_cover_pose，分别验证合格只读测量，以及closing selected bytes变化、读取拒绝、Link不匹配、slot不匹配、第二次读取停止、来源过期、第二次读取revision变化。拒绝时valid为false且旧测量存储不改，所有掩体模式均无body apply、receipt install或neutral初始化，结束后测量失效。

因共用前缀也影响FullBodyC0FirstFailureLiveTest，同步移除其重复宿主定义并核对受影响入口。仅选择16–20五个首次失败记录场景实际152检查通过，覆盖gate／plan／prepare／commit失败及collector一次写出；没有运行其21–27恢复场景或全工程矩阵。两个最终入口以当前工作树实际include经MSVC /std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译链接0，合计29场景605检查、0失败。编译sourceDependencies各记录90项workspace依赖，其路径／SHA及命令、原输入、日志、产物和结果在analysis/cover-system-20261108/goal-host-r17/verification.json。

这里的actor／property／当前对象准入仍是明确Host seam；CurrentKey类型及比较、Goal测量、数学和提交事务是生产代码。body_tick_source_live关闭，因此本轮没有执行V3掩体测量分支，也未授予／执行掩体身体写入。CMake仅将既有Goal登记范围0–14补全到0–23，不新增运行或验收门禁。本批修复的是CPU集成测试入口及受测关闭路径，不是游戏碰撞、同版hit／muzzle或头显通过。

运行plan仍859bb156，23驱动逐项一致，Native仍已冻结r13；未stage、启动游戏／XR或抢窗口。后续继续当前世界形状／占用及V3中立provider接线，再由已有唯一写入端形成合法掩体提交；r16窄段仍等待前驱完整收尾后的唯一交接。完整CV-01–24／AC-01–22、Publicv71和历史评分的证据范围不变。

## 2026年11月9日 持续中立转换WIP及实际脊柱可达失败

上轮为progress：d43e56c6修复真实Host接线。本轮在既有cover_neutral_goal中增加冻结拟合值的Core／V3持续convert入口，提取原placement身份／时段检查供其复用。当前来源、掩体／世界／人物／Link／slot、独立身体和观察轴、脚位、模型及输入参考域继续核对，Scene／publication可随当前合法来源推进，但旧binding不能冒充新回执。冻结Neutral不重新placement或拟合，真实HMD和已提交Grip各应用一次；该纯数学入口不读写游戏或签发碰撞／身体许可，尚未接入live provider。

中立结果新增最小稳定Basis，保留原始Native／输入域、controller、reference、model／units，不缓存每帧时钟或序列；convert还核对拟合结果与Neutral自身一致。Core／V3目标转换、38类冻结域／结果变化、15类当前来源／时段反例及别名／失败输出检查均保留。早期编译的publisher PID窄化和auto推导错误已改正；一次跨成功调用的整Output字节比较失败明确定位于Goal的236–239填充字节，改为逐一比较全部命名字段，失败目的缓冲区仍按原字节保持检查。原日志未删。

进一步把带小幅真实头动及Grip的目标送进原完整planner，真实发现三处spine_unreachable：preserve_c0／reference_neutral的requested分别48.0171826／48.0252175，而available约46.86003游戏单位。当前310检查仍有3个失败，原断言和result-moving-diagnose.log保留；不能把前一步295检查通过称作持续身体跟随通过。尚须在保持中立基准、真实头目标、固定骨长和脚位的前提下处理当前身体姿态可达性，并将最终身体路径纳入碰撞认证，不得用普通站立的head projection／native-body fallback冒充完整掩体跟随。

本批以WIP源保存，未编译运行Native或交付。证据analysis/cover-system-20261108/neutral-convert-r18/。此时收到HB070119实际72536完整恢复、全跨度视觉、编码／retention终态及独立idle后的最终唯一交窗，后继Snub071103→Boltok072100→Frag074333→Longshot075334保留。先保存本源码与失败，再独立复核plan859bb156及23驱动、准确r13三制品，执行原先已登记r16窄段；本WIP及r14／15均不混入该运行包。

## 2026年11月9日 reader-r16实际32760：首条严格组件路径通过，第二条读预算首败

先将上述WIP与三个脊柱失败保存为516d0d75，再独立核对plan859bb156、23驱动、所选游戏SHA及准确r13三制品，prepare-only0。实际stage备份prior-private-text-20261109-075847；继续Native5cf4adea／PGf8e6／XR1684，r14／15及本轮中立转换源码均未混入。计划原样归档cover-boundary-runtime-plan-executed-32760.json，Publicv71不变。

实际game32760／creation134386559451489025，owner66992／creation134386559379233619，Native session9905337298675。原A一次到CT2／CA0，Pawn[-3929.994140625,-160.1084747314453,579]，本次geometry严格读取通过，实际进入Trace；61864的统一closing拒绝没有复现，也没有凭本次通过反推其原因已解决。

实际两次原Actor.Trace完成。ray0 acceptedTRUE，reads16240／bytes158219，命中StaticMeshComponent#186308，Owner／Outer为mp_warmachine_SMC#3；空StaticMeshComponents声明下，继承自Actor的Components和AllComponents均100元素、各唯一包含该明确命中组件，完整typed成员及closing路径实际走通。这补上r12成员认证的限定游戏执行证据，不证明完整实体边缘、可达抓边、体积扫掠或世界占用。

ray1原查询也完成，queue_state2、trace_calls1、pending_possibleFALSE、allocation_retainedFALSE；原输出Actor0／location0／normal0，但随后读取达到16385次而触发原16384上限，bytes161212，整条回执acceptedFALSE。首败为World-state read budget exceeded，按原规则停止，没有第三条、retry或提升预算；不能把未完成closing的原空输出当成认证clear端点，整个edge_interval和本段玩法结果仍FAILED。下一窄修应减少有明确证据的重复读取，并保留全部原始依赖、当前值及closing检查，不能删成员／矩阵证明或沿用旧查询结果。

已亲自查看全部22.583478秒paired-review-00.jpg及入掩体附近实际game00382／00392、simulator00440／00450原图，两路仍显示右侧身体／装备大面积近裁剪遮挡，视觉FAIL；启动第三人称仍可见。原采样421游戏／480模拟器，最大捕获缺口1.7666948／0.283333秒，末端覆盖一致；这些是WGC样本，不是窗口标题FPS、逐渲染帧或原子C0／GPU配对证明。编码视频22.600秒、5185541字节，编码工具36194实际terminal0。

runner96509实际terminal0、launcher0，operator的Edge ray not accepted失败保留；Native cleanup／entries／pad恢复TRUE，hand_stop5／drained，body等pending0，XR0、实际游戏退出。原config字节、settings SHA及head原缺精确恢复，结束23驱动全部一致；XR记录terminal exiting／objects destroyed／shutdown completed，其end_session_succeeded仍FALSE，不写成EndSession成功。r13本段authored_eye_applied411／rejected0、missing_basis0、uncomposed1；body_goal_attempts368／rejected139，均只作本段计数，不当作完整身体提交或旧504拒绝的统一因果证明。

全部视觉与编码完成后既有retention6097实际terminal0，删2553媒体／451948482字节，保留32760／72536／50728、最多3局／48小时，源码／数值／文字未删。再次独立核idle后已明确最终唯一交窗Snub071103，后链Boltok072100→Frag074333→Longshot075334保留，本窗不追加case。证据cover-32760-review.json、cover-32760-closure-proof.json及原session continuous-stereo-32760-20261109-075940-3e5ba4de.json。

本轮进展为“严格实际命中组件路径首次通过＋新预算首败明确＋完整正常收尾”，完整掩体仍FALSE。持续身体转换310检查中的3个脊柱可达失败也仍保留，后续须处理当前姿态可达性并认证最终身体碰撞、双眼、受击与枪口。CV-01–24／AC-01–22及实体头显验收没有缩减，历史评分不变。

## 2026年11月9日 reader-r19减少重复读取，保留原预算和全部见证

上一文档核对回合没有改变实现。本批继续处理32760第二条原Trace完成后读取达到16385次的实际失败，不增加16384次／2MiB／3秒上限，不重试、删检查或沿用上一条查询的值。TraceQuery关闭复核将同一OS页内选中范围合并读取，然后逐项比较每个原始见证；页间不合并，原跨页范围保持独立，间隙字节不进入见证或输出证据。合并读取失败直接保留失败，无逐项回退。原identity／metadata／name-slot／registry检查及顺序保留。

每次_surface_state的Matrix四个Plane字段、链接、类型和大小仍分别读取。相同精确Plane类型的X／Y／Z／W标量布局在该次快照内验证一次，所有原字段／super／Next／类型及值仍进入关闭复核；不同类型地址重新验证，不跨前后快照或查询缓存。

实际56个Python定向方法通过，日志query-budget-r19/final.log。新增负例覆盖不同Plane类型的错误布局、共享schema各类变化、所选字节突变、间隙变化不成为证据、原跨页范围、合并读取失败不重试及原字节／时间预算。先前仅schema去重仍超预算、以及夹具未映射页内间隙导致读取失败的日志保留；夹具补齐已存在内存页的零字节，不在生产代码增加缺失字节回退。

同一typed cooked100／100组件夹具执行796501ca旧生产实现与当前生产实现，两个表面快照加两次关闭的读取583→359，少224次；78个选中value见证及metadata／identity集合哈希完全一致。合并读取包含间隙，计入预算的字节17616→33712，未隐藏额外字节。再实际消耗16000次读取后运行同样流程，最终16361次／289744字节，在原上限内。该证据是合成夹具，不是原32760内存重放或完整Trace管线，更不证明游戏已修复。结果及源码SHA见query-budget-r19/verification.json。

新r19计划SHA16345fcddd83873882863b29cb4346a52ec6229daa57c63c098f3f2c93814a2c；23个冻结驱动中仅cover_trace_query变a49ea42e96b8539e633f07ebe14a603fda9de18a655784f53d845474782f7331，其余22项不变。准确Native5cf4adea／PGf8e6／XR1684仍用原cover-r13，未重建Native。原r16计划与全部新驱动已归档，prepare-only实际0，未stage／启动。原A一次、最多8ray／30秒和首败停止不变；不授予C0、抓边、身体或碰撞许可。已向原尾端Longshot075334登记／转发尾位请求，不越过已受理顺序，须收到完整最终唯一交窗再独立核idle。

完整CV-01–24／AC-01–22继续开放，既有近裁剪视觉失败和冻结中立转换的3个spine_unreachable仍保留。公开v71、用户配置和历史评分不变。

## 2026年11月9日 r20精确头部目标与固定双脚的当前骨盆联合求解

在等待r19已登记窗口期间，继续处理516d0d75保留的三个持续转换spine_unreachable。原planner只把期望骨盆投影到两只固定脚的最大可达球，随后按精确头部目标检查脊柱；中立拟合接近最大脊柱长度时，真实头部旋转的眼点／头骨偏置及脚部限位会使当前头部超过该骨盆的脊柱可达范围。不能重新拟合每帧中立值，也不能投影头部、伸长骨骼或套用普通站立的回退来声称掩体成功。

full_body_c0_planner新增显式plan_cover入口，共用原输入／模型／别名验证及完整腿、脊柱和最终编码残差求解。它只接受fixed_cover＋world_absolute；本次当前骨盆除满足两脚可达球，还须满足以精确头部目标和当前spine-anchor偏置建立的脊柱可达球。通过有界投影调整本次实际骨盆／root，Goal、冻结neutral、真实物理／Grip输入、头／脚位姿及所有骨长保持原值。最终原腿／脊柱和残差检查均保留，空交集或求解不完整继续拒绝，失败Output原字节不变。

普通plan继续明确使用原先不依赖头部的骨盆策略；公开Goal／Report／Output／Workspace布局不变。新入口目前仅由离线检查调用，尚未接live provider、构建Native DLL或签发碰撞／身体写入许可。动态调整的实际身体路径及终点占用仍须后续认证，不能只认证原始输入偏移。

实际编译采用当前统一源、MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT。先前312检查通过，再增加两种posture下84组真实平移／三轴正负旋转／保持Grip组合、6组90／180度旋转平移等变检查，以及12个不支持策略、不可达头／脚、NaN和alias拒绝，最终20434检查0失败。三个原spine失败场景通过新入口；测试仍明确要求原入口在对应目标返回原spine_unreachable，未删旧失败。所有轨迹检查精确头、固定脚、原translation／scale、冻结结果及输入不变。

相关近伸直脊柱35／35、8097检查，普通站立精确眼点之外的既有fallback1057检查，以及中立fit60检查均0失败。通用planner默认检查在本61492夹具有3条失败，继承它的absolute检查同样3条失败；以提交2dd68827原header加同一批27项实际依赖另行编译，原／新标准输出逐字节相同，40份生成产物SHA全部相同。它们保留为既有断言失败，不写成通用／absolute整套通过，也不为该掩体修复修改其原断言。

全部结果、编译依赖、基线源码／产物、原失败及范围声明在analysis/cover-system-20261108/cover-pelvis-reach-r20/verification.json。r19冻结运行计划16345fcd及其Native／PG／XR不包含本次源码；没有占用或启动游戏／XR窗口。Longshot075334已确认r19为唯一后继，随后Snub082247请求经实际尾端受理并直接确认，保持Boltok072100→Frag074333→Longshot075334→Cover r19→Snub082247，不复用旧位或将登记视为当前窗口。

本批解决了冻结中立转换在这批实际骨架输入上的离线可达失败。实际实体边缘、连续身体扫掠及占用、当前C0接线、近裁剪视觉、同版双眼／受击／枪口、全套交互恢复及实体头显仍未完成，完整掩体FALSE；CV-01–24／AC-01–22和历史计分不变。

## 2026年11月9日 r21裁剪反拉与保持位置范围失效

上轮f28a40d7为实际源码及离线求解进展。本轮检查原生产CoverInteraction发现：抓取越过合法范围后仅clamp当前offset，却保留首次grab_hand／grab_offset，下一小段反向移动仍被越界请求抵消；保持位置落到缩小范围外时，原入口又直接clamp并认证新位置。两者分别违反CV-16／AC-22的裁剪反拉和当前位置失效恢复要求。

新增三方向1cm反拉用例实际RED，原内核返回边界而非立即反拉。修正后，候选裁剪明确返回clipped，并以该次限位offset及真实手位重设抓取起点；原增益、冻结操作轴、总范围和共同另一轴不变。范围不再容纳当前保持offset时返回range_invalidated，保留原数值、撤回认证并解除抓取；不把范围clamp当成真实移动，也不瞬移到零。恢复合法范围后仍须真实release／新按下，不能自动续旧抓取。

实际CoverInteraction全套及新增三方向反拉、长时间不漂移、保持／抓取中范围失效、旋转轴和gain2检查通过；CoverInputOwner120检查0失败；实际CoverInputTransaction808检查0失败。事务负例确认失败complete不安装候选裁剪位置或重设起点，范围失效时view/body输出撤回，恢复范围只复核原保持位置。旧事务夹具原要求范围收缩后clamp并认证view，实际4条失败已保留，预期按本明确需求改为保留原值／撤回view／范围恢复后再认证，未放宽生产判断。最初新检查调用误放原catch外导致测试进程未捕获异常退出，移回原catch后得到明确RED，两个日志均保留。

证据interaction-clipping-r21/verification.json及源码依赖、编译／RED／green日志。此处complete仍只是已有输入／左手事务完成，不是实际身体提交；跨Tick intent、真实commit ledger、碰撞证书和同版消费者仍须接通。没有借本次内核修正开放真实抓边或身体写入，没有构建／stage新Native；r19冻结计划和三制品保持原身份。

## 2026年11月9日 r22原查询分支及访问标记不是物理世界版本

继续沿既有原指令证据核对碰撞查询同步边界，未重复修复不可用provider，也未调用游戏查询。复用本会话旧私有IDA库，禁自动分析、低优先级及隐藏控制台，定向导出原0x1400e7030与0x1400d0600，共1744＋135字节；全部chunks逐字节对照原section_1000.bin SHA911ab9bc校验。原源IDA库SHA698dc321前后不变，私有库正常flush后SHA变化如实记录；idat67088／工具句柄均实际exit0，无游戏／XR执行。

原receiver0x1400cfef0对有限Extent三分量分别做零比较：全零时+0xf8为1，经0xd0251调用E7030；任一非零时为0，经0xd03b9调用E6a10。故既有零extent边缘射线走的是此次补导出的E7030，不能把此前体积遍历E6a10的单独说明当作全部射线路径。E7030还包含关联对象虚表+0x330、R10虚调用、条件+0x2a8和递归等边界，未把未认证callback概括为完整同步保证。

receiver入口无条件递增32位全局值，E7030从同一RIP目标读取，与candidate+0xf4比较、跳过已访问项并写回。按真实disp32计算目标为0x141ad1998，IDA显示的1AD0998h仅作标签，不当真实地址。该值证明的是每次查询访问标记，不能用作碰撞世界变更版本，更不能根据Actor不变、射线成功或未看到锁就签发物理快照／身体提交许可。

证据query-synchronization-r22/findings.json及原始chunks／生命周期记录。当前具体receiver实例、动态对象／非零extent执行、并行更新的读租约或等价时相保证以及实际body/view/hit/muzzle同版消费仍待认证。完整掩体目标和验收范围不变，r19仍只执行原已登记窄段。

## 2026年11月9日 r23掩体位移意图与实际提交账本内核

新增既定模块tools/vr-common/cover_motion.h，并将CoverMotionTest注册到现有CMake。Ledger的唯一位置推进源是Backend认证的真实身体完成记录，不接受输入complete(true)、查询排队或调用者提交的success bool。Backend须核对真实body publication及碰撞／占用证据引用；缺省UnavailableBackend始终拒绝。本批没有Native生产后端，也尚未把现有CoverInputOwner切到此账本，不能称为实际身体提交接线完成。

每次新提案、等待或取消先读取已完成提交，再处理输入。Intent只保留拥有拷贝的注册键、CoverEpochs、单调id、from-event／version、原／请求offset和真实手样本、锁定轴、grab／edge关联；不保留跨Tick broker nonce、瞬时input ticket、可写栈指针或可直接执行的旧租约。真正执行端仍须重新认证当前逻辑左手归属、当前几何／完整路径／终点及from-version。等待不更新offset，取消只使待版本失效，已发生提交不会被晚到release抹除；裁剪记录保留其实际位置与确切手样本供输入重基准。

账本核对连续event／version、context／basis／全部epochs、精确请求和手样本，以及实际body publication递增。旧确认不能覆盖新位置；同event不同内容、未知部分写入、取消后又出现的异常提交、观察通道失效和后端替换均保留原已知位置并进入恢复债务。普通占用重获、重新给baseline或取消动作没有清债入口；真正恢复／fresh-retirement后端及其与原C0恢复账本的连接仍待实现。原始输入时钟／序列、时间上限、单轴与另一轴保持、单调id溢出和同线程重入拒绝都有独立检查。

MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT实际编译，最终208检查0失败。检查使用显式Host seam，先改本进程拥有的位置再发布模拟body回执，执行的是本生产账本类；这不是游戏内存、物理世界或第二份独立参考算法，也不认证真实body/view/hit/muzzle消费。证据motion-ledger-r23/verification.json、实际依赖与原编译／执行日志；未改r19冻结计划／三制品，未构建新Native。

此时Longshot075334/game65324已在全部进程、恢复、全跨度视觉、编码／retention和idle后唯一交窗本r19。其schema／Interval解码通过但closed-bolt旋转gate6拒绝179.965度，原失败保留，不当作开栓或取弹通过。先完成本源码保存，再独立复核plan16345fcd、23驱动、准确cover-r13三制品及空闲，运行原A一次／最多8ray30秒的首败停止窄段；本r20／21／23源码不混入冻结r19。后继仍唯一Snub082247，再其已受理Bol084022，不能提前或重复交窗。

## 2026年11月9日 r19实际60336：读取次数降低但首条关闭超2MiB

先将r23保存4f4a9fb9，独立CIM／idle及plan16345fcd、23驱动、准确r13三制品均核对，再stage，备份prior-private-text-20261109-085601。第一次包装器误用无Pillow的Python314，在启动游戏／XR前import失败；config原字节恢复、idle和驱动一致后，改用计划中SHA已核的Pillow／NumPy解释器。错误日志和独立恢复证据r19-prelaunch-interpreter-failure.json保留，未改环境或用它算作游戏结果。

实际game60336／creation134386594223021086，owner48548／creation134386594139843738。准确Native5cf4adea／PGf8e6／XR1684和reader a49ea42e运行；原A一次到CT2／CA0，严格geometry读取通过。第一条原Trace完成，queue_state2／trace_calls1、pending_possibleFALSE／allocation_retainedFALSE，返回mp_warmachine_SMC#3及StaticMeshComponent#186308真实命中；之后读取到5487次、2097976字节，超过原2097152字节上限，acceptedFALSE。原r16的读次数瓶颈降低，但整页合并造成额外间隙字节膨胀，本r19仍FAIL。按首败停止，没有第二ray、retry或提升任何上限；未完成closing的surface_after成员解析和raw hit不能改写成严格表面／边缘认证。

已亲自查看全部22.3668148秒paired-review-00及实际game00369／00390、simulator00432／00453原图，两路20.48／21.47秒右侧身体／装备大面积近裁剪仍视觉FAIL。启动第三人称和等待卡仍可见。409游戏／473模拟器样本，最大捕获缺口1.8166894／0.3333357秒，两路末端覆盖；不是窗口标题FPS或原子C0／GPU证明。视频ffprobe22.400秒／4998057字节，原脚本ffmpeg wait0断言后的完整metrics已输出，review／ffmpeg进程消失；统一review工具句柄未保留，单独记入closure，未重复编码以补工具记账。

runner36635实际terminal0／launcher0；Native cleanup／entries／pad恢复TRUE，body／hand／atom等pending0，XR0、游戏退出。config原字节、settings SHA和head原缺失精确恢复，结束23驱动全部一致；XR terminal exiting／objects destroyed／shutdown completed，但EndSession仍FALSE。authored_eye_applied393／rejected0、body_goal_attempts350／rejected117、uncomposed2只作本段计数。旧目录menu-firstperson-60336-021741属于另一历史Steam creation134379442251489996，其PermissionError不算本次故障，身份已分开。

全部视觉和编码完成后retention76724实际terminal0／errors0，删2049媒体／353026582字节，保留60336／65324／4532、最多3局／48小时；源码／文字／数值保留。独立idle后已最终唯一交Snub082247，Snub确认准确stage并启动其原窄段，本会话不再用该窗。其后同一Bol084022槽已由后继在stage前明确更新为Bol085057-phase-timeline（plan45cdcf2e／PG474638af），不是新增或复用旧slot。

原session continuous-stereo-60336-20261109-085736-c8b1bf49、action menu-firstperson-60336-085741/cover-boundary.json及cover-60336-review／closure-proof保存实际首败与恢复。完整掩体、实体头显及历史计分保持未完成／不变。

## 2026年11月9日 r24关闭复核的额外间隙字节预算

针对实际60336，只修正关闭读取的分组策略。先合并同页重叠／相接原范围，再按间隙字节从小到大选择可合并邻接；每次finish额外padding总量最多64KiB。未选中的块仍独立执行全部原读取和逐项原见证比较，不跳过、回退或重试。跨页、身份／metadata／registry／name-slot及原16384次／2MiB／3秒总上限不变；padding仍由Query.read正常记账，不成为证据。

新增1000稀疏页／2000见证，旧r19实际RED：选中16000字节被读成3492000。当前同一夹具78568字节／1982块，保留全部2000原见证，额外62568字节在65536内；另用2048字节间隙恰好整除预算的夹具验证小间隙不会被早地址稀疏页饿死。最终Python58方法0失败，16000次真实预消耗后的原typed组件用例仍16361次／289744字节；原数据改变拒绝、页边界和预算拒绝分支保留。这不是原60336内存重放或游戏通过。

新r24计划SHA83be29c74b1f75f589ccf16d640d4cf8e4b180edf6a7d4a32e8f6016b40231f9；23驱动仅cover_trace_query变369f12175e4bdb920ed2b9b213cee61ea6291d2120802b96d398c870d4fddafb，其余22不变，Native／PG／XR仍准确cover-r13，未重建Native。新驱动全部归档drivers-padding-budget-r24，prepare-only0，未stage／启动。下一次仍须新尾位登记与最终唯一交窗，不能追加到已结束60336或占用Snub窗口。

证据query-padding-r24/verification.json及RED／green／final日志。当前账本208检查只证明纯模块，真实输入／body消费与恢复、碰撞快照／扫掠／占用、近裁剪、同版双眼／命中／枪口和头显仍开放，CV-01–24／AC-01–22不缩减。

## 2026年11月9日 r25现有输入归属链接入可选延迟身体提交

上轮5f37931d为源码修复与实际60336首败后的进展。本轮将CoverMotion Ledger接到现有CoverInputOwner／CoverInteraction；控制器解码、唯一LeftHandInteractionOwner、weapon prepare／仲裁／commit／complete仍复用原路径，没有新仲裁器、相机或骨骼写入端。默认构造保留当前geometry-disabled Native的旧路径；显式传入认证Backend才使用延迟模式，尚未在真实Native启用，不能据本批开放实际非零抓边范围。

CoverInteraction新增拥有拷贝的propose输出，推进真实输入和抓取状态，但offset只保留已同步身体位置；请求值不再因输入complete(true)就成为身体位置。CoverInputOwner在新输入前先同步完成记录；新release、失追踪、变化的真实手位／边缘／上下文、目标范围失效或原100ms到期，会在任何外部weapon回调之前取消旧待版本。已实际完成的移动则先同步，随后松手／失败事务保留该位置，不能回滚成旧offset。

等待中手位、抓取及目标完全相同时，后续新序列保留同一intent及其原始sequence／observed_ns，不能每帧替换而饿死多Tick查询，也不刷新旧时间；原100ms过期后产生新id。每次真正执行仍须由后端独立复核当前逻辑左手归属、当前C0来源、完整最终身体路径／占用及from-version；motion_current只表示取消／版本状态，不是再次写入许可。Host提交者明确在自身版本已推进后拒绝同一intent二次执行，即使输入尚未ack。

输入限位的clipped标志随原手样本绑定到intent／完成记录。只有实际身体完成后才重设该抓取起点；几何裁剪也按实际安全位置／对应手位重设。另新增已在实际限位处继续向外拉的RED：无需身体新移动的no_motion分支原本不重基准，反拉1cm仍卡住。现只在请求等于身体已确认位置时重设该新手位，不增加body write、commit event或version；等待尚未完成的位移仍不重设。原RED及修正后green保留。

body若在weapon回调期间完成仍然有效的旧请求，complete先复核真实新位置，并重算本次已解码输入的状态；不重新仲裁，也不提交基于旧位置的候选。回调前已读到release或变化手位的请求则立即失效，不能在回调间隙再执行。后端在最终complete失效、admission拒绝、source中断或未知写入时撤回掩体提案／视图，并保留恢复债务；已有独立weapon grant不因掩体后端缺失被全局撤销。

真实控制器适配器、仲裁器、交互内核与账本组成的新检查最终1157项0失败，其中808为原输入事务检查、349为新增组合检查。覆盖等待／释放顺序、保持／重抓反拉、限位及几何clip、no_motion继续拉、源时限／不续时、回调期间完成、失败weapon事务、非零身体baseline、HMD独立叠加、范围收缩、上下文／snapshot变化、后端丢失、退出及武器隔离。初版15条失败中，静止采样替换确为接线缺口；其余一部分是Host用literal0.3替代原精确计算请求，严格相等正确拒绝，夹具改成实际执行该请求，生产未增浮点容差。

当前源MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT六个目标编译0；交互内核原20000帧重复／漂移检查、InputOwner120、Motion Ledger213、Native input62和共享生产消费者六个定向case共158检查均0失败。共享消费者执行的是原outside／cover／source1-cover／held-fallback／pending-magazine／commit-rejected路径，只使用各测试进程自己的Controller映射；不是游戏或真实模块／物理验证。CoverInputMotionTest已注册CMake，完整当前工程Native DLL／CMake构建未据此宣称通过。

证据input-motion-r25/verification.json、编译依赖和原始RED／green／final结果。新模式的BodyBackend仅有显式owned-memory Host和Unavailable实现；真实C0完成／回滚／fresh-retirement认证、跨掩体basis的合法handoff仍未接通。当前实例对退出后的旧域明确拒绝自动复活，不能据本批称反复进出完整通过。输入侧view是已同步逻辑位置与当前追踪的候选；最终绘制、命中与枪口仍必须消费真实身体完成publication的同一版本，不能用本输入结果取代。

r24 plan83be29c7、reader369f1217、23驱动及准确cover-r13三制品保持不变，未stage／启动，也没有把本r25源码混入。新尾申请已由Bol085057经Longshot085615转实际尾端Frag090321，Frag直接确认本r24为其全部终态后的唯一后继；当前队列Snub082247→Bol085057→Longshot085615→Frag090321→Cover r24。对方按prepared.source_snapshot=context-native-r13-source更正了最初source目录假设，实际初核24项声明源／3保存制品／23驱动及AST无差；此为尾位登记，不是当前授窗。完整CV-01–24／AC-01–22、近裁剪、真实碰撞／受击／枪口和实体头显仍未完成，公开v71与历史计分不变。


## 2026年11月9日 r26真实C0消费完成边界与撤回后拒绝发布

上一轮fab0bccf已保存输入与延迟提交账本接线。本轮继续核对实际C0内核与Native Goal发布者，发现一个具体缺口：完整commit之后若在原版消费前成功rollback，随后notify仍可通过，原retained_consumer仅凭commit_finished和consumed将已撤回的候选视为仍被消费。新增生产内核回归在原实现实际24877检查中出现1条失败；原日志保留。修复后要求消费入口真实认证成功，且所有计划修改单元仍为applied。正常撤回、未commit、消费入口拒绝、异常、停止、关闭失败和协议债务均不能获得计划姿态的保留／完成认定。

新增只读completed_consumer查询，检查同一Backend、原Stamp和安装时serial、原版正常返回后的真实finish、关闭的lease与无失败／协议债务。fresh-retired和下一次prepare后旧完成不再有效。查询不做新的游戏读取、CAS、guard调用或债务清除；已有applied字段的恢复责任继续保留。它只证明原消费流程完成，不证明最终SpaceBases、碰撞占用、physics／HitInfo或枪口，因此尚不能独自签发掩体位移完成记录。

现有body_c0_goal_policy_live在apply成功时保存实际kernel serial，在Native真实pose_end之后要求该确切事务completed_consumer才发布视图。新增两条实际Native生产路径检查：原版调用前撤回完整候选，以及正常完成身体却使用错误serial的视图候选；二者均拒绝新视图发布。此处没有新增身体writer，也未删除no_cover或开放掩体非零范围。所有者／Core／actor／receipt准入仍是明确Host seam；实际内存Backend、CAS128、SEH和原生产Kernel／Native apply／finally／Goal路径确实执行。

最终事务25271检查0失败，含逐修改单元正向／反向mismatch和fault-before／after、真实访问异常、时间失效、撤回、消费与fresh-retirement；Native Backend135检查0失败；Goal实际路径26个mode合计483检查0失败，其中原24个mode继续通过，新增24／25已在CMake注册。编译使用MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT，未构建新的完整Native DLL或启动游戏／XR。新查询的跨线程拒绝、错误Backend／stamp／serial、重复只读不改变guard／write／pending及真实retirement失效均覆盖。完整证据在body-completion-r26/verification.json和依赖／原RED／最终日志。

本轮同时核对需求汇总，补充简明行为总览；CV-01–24／AC-01–22和工程默认／实际验收的区分保留。实际Native motion Backend、最终身体几何／连续扫掠／占用与世界同步、basis合法交接及恢复、同版双眼／命中／枪口、近裁剪与实体头显继续未完成。完成边界修复是主线接线的必要准备，不将它表述为掩体全功能或实际物理验证通过。

当前未拥有运行窗。r24 plan83be29c7、reader369f1217、23驱动及准确r13三制品均不变，未stage。本轮已按真实尾序接受Snub093107为Cover r24全部终态后的唯一后继，并直接确认Frag090321→Cover r24→Snub093107；Longshot094103的后续新申请由Snub继续核对真实尾。登记不等于交窗，仍须Frag最终唯一全终交接后独立核对再运行。公开v71、完整掩体FALSE、实体头显未验收及历史计分保持不变。


## 2026年11月9日 r24实际边缘查询通过但暂停恢复与画面仍失败

收到Frag090321/game58316的最终唯一全终交窗后，独立CIM确认game／XR／录制／ffmpeg／本窗脚本空闲；再次核对plan83be29c7、prepared f4bc243e、冻结source_snapshot的24项声明源、3保存制品、23实际driver及AST、所选EXE身份，随后stage准确r13并运行。Native5cf4adea／PGf8e6f3c2／XR1684没有混入r25／26／27源；公开v71未动。运行game33420，creation134386633626394071，owner25508，creation134386633552582872；session6f87d2d5017f4956b4c965cd72a3bc72。

一次原版A取得CT2／CA0，原几何认证通过；8次零Extent原Trace全部strict accepted，分别明确queue2／call1、pending_possible=false和allocation_retained=false，没有重试。当前挡面为mp_warmachine_SMC#3／StaticMeshComponent#186308，边界位于100单位切片的17.1875–18.75之间，宽1.5625。每次读取最大12121次／368691字节，保留原16384次／2MiB／3秒总上限；实际r19的超字节失败未再出现，原失败证据继续保留。这只证明该截面边界，不证明整条可抓边、连续体积路径、终点占用、动态世界同步或任何抓取／身体写入许可。

后续原菜单段先成功暂停，B恢复等待超时，操作器37252明确exit1；menu-firstperson结果保存RuntimeError('VR pause state did not become False')。不能把cover-boundary.json的passed=true或总session的passed=true推广为整段操作通过。Native关闭／drain／入口恢复通过，body／hand／atom pending均0；unified_weapons_cleanup实际fire_disable_code2／hand_stop_code4／drained=false，继续作为未成功的清理子项保留。XR exit0、terminal exiting／objects destroyed／shutdown complete，EndSession=false如实保留，不能写成EndSession成功。

全部3张paired overview与6张原图亲自审看。62.267秒的game726帧、simulator795帧中，启动仍有普通第三人称暴露和等待卡；两路在约12.47–37.18秒有约24.7秒WGC捕获缺口，合成视频在此持有旧帧，不代表新渲染帧或已知游戏卡顿原因。实际8查询位于45.472–51.803秒，窗口内最大捕获间隔game116.6662ms／sim116.6666ms。43.084秒原图可见双手，49.084／51.5秒掩体原图均有严重右身体／肩部／装备遮挡；完整画面、手体贴合和头显均未接受。视频ffprobe为62.300秒／7813858字节；WGC和编码都不是render-FPS／硬件每Draw或命中证明。

runner27002 terminal0／launcher0，encoder37789 terminal0，原操作器terminal1分别记录。唯一retention21342 terminal0／errors0，删除1506项生成媒体246627021字节，保留33420／58316／16872最新三局、48小时；源／文本／数值不删。config原字节、settings原SHA与head原不存在状态已独立复核恢复，结束23驱动哈希一致，最终CIM无自有运行／编码／录制残留。之后最终明确唯一交Snub093107，已不占窗。记录cover-33420-review.json、cover-33420-closure-proof.json、33420-paired-review-build.log和33420-retention-result.json。

下一次实际查询应针对仍缺的非零体积／初始重叠与提交时相取得新证据，不重跑已通过的同一零Extent切片。暂停恢复失败、unified清理失败和实际近裁剪需分别定位，不能用边缘数值通过掩盖。当前完整需求与历史计分不变。

## 2026年11月9日 r27消费后真实骨架读回后端

在现有生产Kernel增加completed_pose只读拷贝：由该Kernel自己的正常消费完成状态提供原Stamp／serial和root、pelvis、head、双脚、双肩目标，Native读者不接受调用者自填目标作为证明。现有Native Backend新增read_consumed_pose，必须在同一C0原版正常结束后、backend.invalidate之前持有原callback／模块／身份准入调用；本批没有在真实回调中启用它，也没有新身体writer或扩展身体写入权限。

读者使用Backend拥有的固定缓冲，重复读取全部115项实际Local／SpaceBases，复核原metadata、reference与Desired，逐骨核对实际Local构成的父子矩阵、单位尺度及原骨长。实际world root／pelvis／head／双脚／双肩再与该确切事务的计划目标比较，沿用既有0.002位置／骨长和1e-4角度界限。原版合法改变非选定叶骨旋转可以通过，矩阵自洽但整个人平移或头部旋转偏离目标仍拒绝。观测QPC单独记录实际读回时间，不续Core／手／HMD原样本时间。

任何失败不修改调用者输出，不CAS、不恢复或注销债务。结果仅是当前CPU Local／SpaceBases及目标几何的重复读取观测，不是原子世界快照、硬件上传、每Draw、physics／HitInfo、碰撞占用或下一帧许可。后续真实运动Backend还须绑定有效身体publication、完整路径／终点占用和同版命中／枪口；不能直接用此成功枚举开放抓边。大候选和复读缓冲在Backend所有存储内，未放在游戏回调栈。

新增停止时序RED：收尾Context合法地允许恢复检查，但读回进行中收到stop后，原新增读者仍返回verified并写出候选，实际441检查中3项失败。现于最终输出边界再核stop，保留该RED，修正后同441项0失败。其中原Native Backend135项继续通过，新增24个读回模式及跨线程／停止检查共306项；覆盖真实VirtualAlloc页的CAS128／读回／SEH、损坏或自洽错误矩阵、变化的Local／尺度／reference／Desired／metadata、错误事务、最终owner撤回、不可读Local／SpaceBases和输出不变。原版Compose输出由明确Host数学构造，owner准入亦为Host，不把它记为实际游戏Compose验证。

当前源三个目标MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT编译0；事务25271检查0失败，实际Goal生产路径26个mode／483检查0失败。91项实际编译项目依赖及原夹具SHA保存于consumed-pose-r27/verification.json。没有新完整Native DLL、默认包或本批读回的游戏运行；本批真实game33420仍是准确冻结r13，二者证据分开。实际回调接线、Native cover-motion完成后端、basis交接与恢复、体积扫掠／世界同步、受击／枪口／最终画面和实体头显继续未完成。


## 2026年11月9日 r28读回接入生产C0收尾与只读结果消费

上一轮a4ab6a94的读回方法已保存，r24真实game33420仍是准确旧r13。本轮在现有full_body_c0_native_live接通读回时点：只有已有apply／Kernel守卫成功提交、Goal为fixed_cover且原Core携带掩体键的调用，才在真实Kernel.finish之后、ActualContext.expired和Backend.invalidate之前执行。普通持枪／stationary Goal及没有原掩体键的来源不进入该读回路径，现有body_goal_policy的no_cover保持，未据此开放任何新身体或抓边写入。

新增模块接口read_completed_cover供同一原C0回调同步复制结果。它要求原Entry地址／内容、原未改Core、同一Kernel完成Stamp／serial以及同一GT；输出不得与Native自有状态或输入别名。它不再次读游戏、不重算／刷新观测QPC，不授予占用／写入／跨Tick许可，不能保存回调栈指针作为下一Tick权利。新的同mesh Compose（含未选中／passthrough）在选择门之前撤回旧结果；停止、OFF、错误来源／回调／线程、Kernel失败和无验证结果均拒绝。读回失败不撤销已经消费的动作，也不注销任何实际applied字段／恢复责任。

实际生产apply→before_original→pose_end／Kernel.finish→Native真实内存读回→invalidate→只读消费链已在Host执行；原版Compose的Local／SpaceBases生成仍是明确Host数学构造，actor／Core／receipt准入亦为Host。原26个Goal场景483项继续通过，新增17个场景355项，最终43个mode共838项0失败。覆盖正常完成、没有生成SpaceBases、矩阵自洽但身体偏离目标、原异常、消费前结束、停止、错Core／Entry／线程、未选中新Compose、Kernel停止、非cover Goal／来源、runtime OFF、遗漏消费通知及输出别名。正常结果在backend.invalidate后仍可读取；测试把实际游戏样式的Local与SpaceBases页改为PAGE_NOACCESS，复制仍成功且原QPC不变，证明getter只读自有已完成结果而不重新访问那些页。

新增BodyC0CoverCompletionOffTest在capture式匿名命名空间编译真实OFF分支并执行，确认无结果输出、无新增hook／write／debt。初版Host封装错误地将NativeAtom前置声明放到全局并遗漏Scope声明，产生编译错误；改正测试包含边界后通过，未放宽生产代码。43场景及OFF目标已注册现有CMake，MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT编译0；91项实际项目依赖、夹具SHA与原日志保存于cover-completed-live-r28/verification.json。没有据此宣称完整capture DLL／默认包已编译或这些场景在实际游戏中通过。

另仅检查33420保存证据：B输入指令于54.626184秒写出，54.778906秒释放，约152.721ms；之后四次UI读取直至60.4021秒仍为DeltaPause-Campaign-Host／LBtnResumeSingle。原Native状态保留8978次成功来源读取／5594次game poll，说明虚拟pad在局内确曾工作；结束enabled0属于正常关闭，不是本次B期间的状态。6564次来源拒绝是全程聚合，缺少该B具体Route准入、原game poll实际返回和stow-fence证据，不能据此归因为定时器、身份、新鲜度或fence。冻结r13本就包含display-mode交接保留数字沿的修复，不能重复将其当作本次新修复；没有重发B、放宽时限或改动其他会话的pad／scope源。定向证据见pause-33420-evidence.json。

后续需将同一C0事实接入实际cover-motion完成后端，并取得真正体积扫掠／终点占用／世界同步及同版命中／枪口证据；本接口仅证实CPU骨架与目标，不证实这些条件。暂停／联合清理失败、右身体遮挡与实体头显继续开放。当前不占运行窗，已交Snub093107的窗口不复用；未再stage／启动。公开v71、完整CV-01–24／AC-01–22、完整掩体FALSE及历史计分保持不变。

## 2026年11月9日 r29非零体积与初始重叠查询对照候选

上一开发轮15ad6a4c保存了r28真实C0完成读回接线；随后需求核对没有新增运行证据。本轮改为补足实际碰撞查询语义所缺的非零Extent对照，不重复r24已经通过的八射线边界二分。

原生产cover_trace_query参数检查现允许起终点完全相等、三个float32编码后的Extent分量均不小于0.001的静止诊断形状。零Extent点查询、退化静止形状，以及规划长度或float32编码长度不在0.001–300内的移动查询仍拒绝；正Extent编码成零亦拒绝。不注入epsilon移动，不修改原typed header、116字节布局、native277入口、身份／关闭检查、16384读／2MiB／3秒预算或无重试规则。报告保留原输入，并另记实际float32输入及stationary标志。

新增cover_volume_query复用原capture，每次外部调用前持久化attempted_result_unknown，返回后保留原回执并核对call1、无pending／无保留allocation。同一组必须保持确切session／game／owner／Pawn／slot／位置／module／function；后续每次调用前后核对同一实体Actor／Component及完整表面变换。最多依次调用当前零Extent墙面线、静止分离正Extent、移动正Extent、静止初始重叠、离开初始重叠；30秒只限制新调用准入，已经开始的查询仍使用原截止和正常收尾。首次严格或命中／空对照失败立即停止，不取消或重试原调用。

本次Extent为(10,10,10)游戏单位，仅作语义校准，不是已认证人体尺寸。重叠中心由本局当前实际命中点和法线计算，诊断深度不是穿透恢复距离；初始重叠的零法线如实保留。反射Actor.Trace不提供FCheckResult.Time，不能制造原始命中时间。即使五项对照通过，collision_verified、grab_authorized、body_modified、shape_semantics_verified与实体头显验收仍为false；完整身体扫掠、终点占用、动态世界同步及提交时相继续缺证。

实际离线检查共73个Python方法通过：Trace48、Surface13、Volume12。使用明确合成的反射内存和原查询替身，Game=0。旧参数守卫拒绝准确静止正Extent的RED保留；初版float32打包遗漏元组展开导致8个错误，修正后通过。首次prepare-only因默认GBK不能读取UTF-8破折号退出，保留失败；新wrapper显式UTF-8读取，新prepared／plan按ASCII JSON序列化供未改共享加载器使用，最终prepare-only exit0。注入后的原equipment／menu owner字符串已compile及AST核对，未执行游戏状态。

准确plan为analysis/solo-m1-20261028/cover-volume-runtime-plan.json，SHA256 233b9822925de64e947fcb3a4c5031afeb8bf034e139286d9d36ea41e1d24542；prepared为equipment-controls-prepared-20261109-cover-volume-r29，prepared.json SHA256 5ac624f4fb9481233812454c371acb7fb5c9dc51a588b4e476ad238f3b4a77aa。24项声明冻结源、24个驱动及AST、3保存制品已核对并归档。Native 5cf4adea9df910fd35d6a9a4ace70a317dfb3c9edfd05fc1c27b249dd7e88380、Playground f8e6f3c2715b244e8250976c3eb1279cb306c2a50576e078f64bf8b23daf4807、XR 16840b27dc40ebb489f93121758cb7bc0daa2700df046e508babbd50c578a7fc均为原r13准确制品；不包含r25–28新C++，不重新声称这些源码已在实际游戏执行。

本次几何操作器保留原finally／正常关闭，仅移除原操作器中未取得新诊断依据的暂停B重复动作；game33420该失败仍开放，不因此改记通过。保留0.4、后台／observe-only、测试静音、物理控制器输入隔离、身份新鲜度、存档音频与正常清理保护。登记链为Snub102805→Longshot104905→Boltok105222→Cover r29；Boltok已直接接受此准确候选为其后唯一后继，但尚未最终交窗。登记不授当前stage／启动权，不复用r24旧窗口。完整日志与准确驱动存于analysis/cover-system-20261108/volume-query-r29。

另收到当前共享Native的consumed-output停机资源窄修复：正常验证消费并返回、scope关闭后的对齐Local浮点POD覆盖债务，永久停止时可不再算Native资源存活；原pending及来源重用守卫仍保留，active／异常／部分或未消费债务仍阻止结束。修改保留r28 completed-cover接口。Frag私有Native5e51dfdd的game54784停机证据与当前根源码分别记录，本轮没有重建此完整Native，也未将其混入r29旧r13制品或本次源码提交；它不能自动证明掩体unified清理已修好。原回执见analysis/frag-grip-20261108/20261109-101526-consumed-output-stop/shared-merge-receipt.json。

真实cover-motion后端、合法neutral／basis交接与恢复、体积与占用／世界同步、同版view／hit／muzzle、右身体遮挡、暂停／联合清理与实体头显均继续未完成。公开v71和历史计分不变，本目标保持进行中。

## 2026年11月9日 r30当前停机资源修复与掩体完成结果兼容验证

r29准确源码已先保存为7d741ee8133b9721525fc954c9fe2f477fc65a0c；直接向既有Boltok105222前驱确认同一plan233b9822／prepared5ac624f4及24项最终驱动清单，没有替换候选或占运行窗。

共享Native新增停机资源计数后，原r28结果已不代表当前修改的Kernel／Native。为验证其与本功能实际C0收尾接线的相互作用，在已有BodyC0GoalPolicyLiveTest增加43–46四个模式，并把现有CMake注册扩至0–46；未另建生产writer、reader hook或停止路径。覆盖正常消费返回后停止、原版异常、遗漏消费通知、消费已通知但事务仍active时停止。检查正常完成可在停止前复制原owned结果；停止后getter拒绝且输出不变。正常闭合的POD仅结束资源存活责任，pending／outstanding字段债务保持；异常或未通知保留原屏障；active阶段不能提前结束，必须等待真实scope收尾。

当前生产Native／Kernel、Host原版Compose数学和原Goal路径共47个模式961检查0失败；其中原43模式838检查继续通过，新增4模式123检查通过。真实OFF匿名命名空间目标亦编译与执行0。MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT独立构建，实际编译90项项目依赖纳入91项原依赖集合及CMake的构建前后哈希检查，未出现运行期间源改变。输入仍为保留的真实骨架与明确合成Core，不能改记为实际游戏、物理碰撞或头显证据。准确源快照、编译依赖、日志及verification保存在analysis/cover-system-20261108/cover-stop-integration-r30；没有覆盖r28历史结果。

本批只修改上述测试、CMake的模式上限和本文；共享停机生产修复仍由原提交归属保留，不混入本批Git保存。准确r29 plan／prepared／24实际驱动和旧r13三制品不变，未stage／启动，不包含此当前Native；实际unified清理、暂停恢复、身体遮挡及CV-01–24／AC-01–22仍未通过。下一实际动作仍是完成前序后的单局非零Extent／初始重叠对照，之后根据原回执推进体积、占用及唯一身体提交后端，不以此次CPU兼容检查代替运行闭环。
