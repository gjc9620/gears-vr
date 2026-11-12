# 掩体系统开发与验证记录

目标：完成掩体系统开发与验证。状态：进行中。

最近结果（2026-11-09）：r54实际41460完成原A吸附CT2／CA0，并保留首body SphereElems拒绝见证：空数组、inner stride80、目标KSphereElem为ScriptStruct但extent72，现有等值条件拒绝。源查询未关闭／accepted=false。完整双路总览及6原图仍确认严重肩／身体遮挡FAIL。wrapper／Native／入口／XR／编码／媒体保留全终，10项原始基线精确恢复，唯一交接对象Bol180947。r64–66数学与完整清单消费均已提交，但未据此接通真实场景或身体移动；完整CV／AC及头显要求保持未完成。

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

## 2026年11月9日 r31当前原版人物碰撞尺寸来源与同槽候选补充

上一轮b697d557和7d741ee8是已保存的源码／Host验证进展，不是实际体积碰撞通过。本轮核对保留KB中三个脚本的完整SHA：Engine.Pawn.GetCollisionHeight、GetCollisionRadius和Engine.CylinderComponent。前两个getter实际读取命名CylinderComponent的同名字段；只读取某个CollisionComponent或沿用历史34／80等尺寸不足以认证当前来源。脚本并未证明CollisionHeight的半高含义、缩放、物理场景消费或最终探身形状，本轮没有猜原生调用签名、调用setter或在其他游戏运行时开启IDA。

新增生产Python读者cover_collision_shape.py，复用CoverQuery、已有当前掩体roles、字段声明链和完整Matrix读取。它核对当前受控Pawn的CylinderComponent与CollisionComponent是同一非Default对象、类型继承、Outer和Owner均属于Pawn，再读取严格反射FloatProperty宽度／维度／成员链、两个不别名的正有限尺寸和完整typed LocalToWorld。异常、缺失、非有限值、超过诊断上限、字段与矩阵重叠或关闭变化一律拒绝，不填默认值。使用既有16384读／2MiB／3秒关闭预算，保留原closing_rejection，并再次关闭session／game／owner身份。它不生成Extent，不推断半高或世界缩放，更不把原人物移动圆柱当成已认证的骨骼探身／头臂／受击完整形状。

8个新方法执行通过，使用明确合成的反射内存；覆盖当前原字段与矩阵、错误双指针／Owner／Outer／类、零／负／NaN／无穷／超限尺寸、错误属性类型／宽度／维度／脱链／别名、矩阵schema及关闭时尺寸／组件／变换／身份／registry变化。验证无bridge、queue、engine调用或游戏内存写；成功观察仍保持collision_verified／shape_semantics_verified／final_body_coverage_verified／world_synchronization_verified／activation_allowed／实体头显为false。日志、源码SHA、脚本SHA及候选在analysis/cover-system-20261108/cylinder-source-r31。

候选在原边界操作器增加显式--cover-cylinder-source：原A后、原五项体积对照前执行一次严格原字段观察；失败保存后停止。对照结束还比较其当前掩体roles，不能把相同roles解释为体积／物理世界原子快照。仍为原A一次、最多5次原Trace、首次严格或语义失败停止、无重试、不重复未诊断的pause／B。新wrapper／operator及plan独立命名；原r29 plan233b9822与prepared5ac624f4保留为未执行记录，但共享Cover driver已更新，旧r29不能再直接运行。

准确plan analysis/solo-m1-20261028/cover-cylinder-runtime-plan.json SHA256 9a48e15b9113183780c81ba1e30ad76bd207df6b8408ca3d7548265d77766011；prepared equipment-controls-prepared-20261109-cover-cylinder-r31/prepared.json SHA256 3fdf0a9d424ce06a74adfb08eb0845974eee51a8442c7eed9042bd6ba16e6008。25项声明driver及AST、24项原r13声明源、3保存制品、注入原equipment／menu owner字符串compile／AST和prepare-only exit0。Native仍5cf4adea、Playground f8e6f3c2、XR1684；没有新Native构建或混入r25之后C++。r29原73方法结果保留，不伪称本轮重新执行全部73项。

当前同槽前驱仍Boltok105222，尚未最终交窗；本次准确候选拟直接请求替换原未执行r29登记，不新增slot。已直接接受Snub110649为本槽全运行／双路全跨度和原图审看／精确恢复／编码与retention实际终态／独立idle后的唯一后继。更晚HB110627申请已转真实尾Snub直接核对，未作为第二后继。不能据进程暂时空闲或队列消息启动、stage或交窗。完整身体与physics形状、扫掠／终点占用／世界同步、实际Native运动后端与basis恢复、同版view／hit／muzzle、暂停／联合清理／身体遮挡和全部CV／AC／头显要求继续开放，公开v71及历史计分不变。

## 2026年11月9日 r31实际30188：原圆柱字段通过，非零体积命中对象不同首败

准确源码808b0b7b保存后，Boltok105222确认同槽替换r29为r31，并在实际47536全部审图／编码／恢复／retention终态后最终唯一交窗。独立CIM与idle、plan9a48e15b／prepared3fdf0a9d、25项driver及AST、24项冻结源声明、3保存制品及所选EXE哈希均核对通过，再prepare-only和stage。私有文字备份为prior-private-text-20261109-112126。实际仍是旧r13 Native5cf4adea／PGf8e6f3c2／XR1684，没有混入r25以后新C++；公开v71不变。

实际game30188 creation134386680977958133，owner67820 creation134386680904212657，session8231b492。原A一次取得CT2／CA0，现有geometry关闭通过。新增严格圆柱读者实际accepted：当前CylinderComponent#84同时是Pawn命名及选定碰撞组件，Owner／Outer／World／typed Matrix和关闭一致，CollisionRadius原字段34、CollisionHeight原字段72；4408读／36904字节，读者本体202.4266ms，外围事件括段约206.5ms。不能把72命名成已证明的半高、当作整个探身后的人体范围或直接生成Extent；这只是本局严格当前原字段观察，不是原子物理世界／碰撞消费证明。

实际只执行3次原Trace，均queue2／call1、submit0、pending_possible=false、allocation_retained=false，无重试。当前零Extent线accepted，命中mp_warmachine_SMC#3／StaticMeshComponent#186308；随后正Extent(10,10,10)、起终点完全相等的分离对照accepted，返回Actor0且位置／法线零。第三次正Extent移动调用完成，但原回执记录BlockingVolume#194／BrushComponent#53，与前两次固定的网格表面不同，capture按原different Actor/component守卫拒绝。该新命中尚未走完新表面／关闭证明，只能保留为未通过回执，不能直接当作已认证的替代阻挡面。整体Volume及操作器68448明确exit1；静止初始重叠和离开初始重叠均未执行。

3次Query最大12124读／368451字节，原16384读／2MiB／3秒预算没有放宽。本局不是读取超预算或pending未定失败；也不能仅凭两个对象名称推断全部碰撞差异原因。已证实原计划把零Extent网格命中作为所有非零体积的同一表面准入，在本场景遇到了不同对象并拒绝。下一窄修应独立获取非零体积的实际命中Actor／Component及完整表面关闭，再在该非零组内维持严格同一性；零Extent结果仍单独保留，不能简单允许任意换Actor或把本局首败改绿。不重跑未改的原三查询假设。

Native关闭、入口恢复和pad恢复通过，body／hand／atom等结束pending0；unified fire_disable_code0／hand_stop_code5／drained=true。本局该子项正常不能抹掉33420的drained=false历史失败，也不证明当前新Native停机修复进入了旧r13。XR exit0，terminal exiting／objects destroyed／shutdown completed=true，EndSession=false原样保留。本轮没有重试pause／B，33420恢复失败仍开放。

全部1页paired overview和6张实际原图已亲审，覆盖圆柱观察、line及swept首败附近：game00379／00383／00430、simulator00450／00455／00502。两路均有严重右身体／肩部／装备近裁剪遮挡，完整视觉FAIL；启动仍见第三人称暴露和等待卡。game439帧跨度24.0501595秒，simulator511帧跨度24.0168246秒；最大捕获间隔1883.3474／300.0048ms，game启动1.133–3.017秒捕获缺口不直接证明游戏停顿。动作段最大83.3337／66.6674ms、两路尾覆盖有效，不将WGC或标题FPS当render／GPU／头显证据。ffprobe视频24.066667秒／5601352字节。

wrapper工具77474实际terminal0、encoder81605实际terminal0，与操作器terminal1分别保存。唯一retention20757实际terminal0／errors0，删除4247项生成媒体821152522字节，保留30188／47536／35040最新3局、48小时；源码／构建／资产／存档／文本未删除。config及XR settings原字节、head原不存在、所选游戏settings SHA、结束25driver与3stage制品均独立核对一致；最终CIM及idle无自有游戏／XR／录制／编码残留。详细game-30188-review.json、closure-proof.json、selected-visual.json和retention-result.json在analysis/cover-system-20261108/cylinder-source-r31。

独立核Snub110649准确prepared35fa0494、21helpers和4保存制品后，已最终明确唯一交该后继；HB110627由Snub已直接接受其后，不作第二后继。当前不占运行窗，不补第二局。完整身体形状与连续扫掠／终点占用／世界同步、真正Native掩体提交后端与basis恢复、同版view／hit／muzzle、暂停／身体遮挡与全部CV-01–24／AC-01–22／实体头显继续未完成；目标保持进行中，公开v71及历史计分不变。

## 2026年11月9日 r32独立非零体积表面准入，不沿用零射线网格

上一轮为实质进展：808b0b7b及实际30188确认当前圆柱原字段，同时保留非零查询不同Actor／Component的首败。该局并未认证新的BlockingVolume表面，不能直接把其地址或回执改为成功。本轮改的是对照流程：原capture及其typed schema、原生代码证书、Actor／Component／Owner／矩阵／关闭／身份／预算规则均未放宽。

capture_volume_probe的schema升至2，最多4次、全部使用同一正Extent。第一次swept_extent不给旧表面身份，要求原capture从本局当前实际命中取得完整surface_after并完成原严格关闭；这是独立非零表面准入，不是忽略不同Actor异常。之后stationary_separated、stationary_initial_overlap和leaving_initial_overlap全部提供该确切Actor／Component，核对同一完整表面、session／game／owner／Pawn／slot／位置／module／function及规划和float32编码输入。任何后续更换Actor或组件、变换变化、自身／Controller／CoverLink命中、pending／allocation未清或命中／空对照不符均停止。首扫还必须是线段内部、方向迎向且近水平的单位法线，不能用无命中、零法线或地面建立候选。

零Extent查询完全移出本次流程，不再次用已经通过的线查询认证体积。重叠中心仅从本局已严格关闭的非零扫掠返回位置／法线计算，诊断深度仍非穿透恢复距离；等起终点保持完全相等，初始零法线原样保留，不制造FCheckResult.Time。原30秒新调用准入、各已开始查询的原截止与正常收尾、每次调用前attempted_result_unknown持久化和无重试规则保留。成功对照也不签发body／grab／collision／shape／occupancy／world-sync或头显许可，10单位Extent仍不是已认证人体尺寸。

新正Extent-only合同在旧流程上实际14方法47个断言失败，主要因为旧流程首先提交零Extent；这是离线流程合同RED，不是重放实际30188或新游戏失败。改正后14个Volume方法通过，包含非零新表面、后续表面／Actor／Component更换拒绝、基线接触、静止重叠零法线和未知回执保存等。另在原生产typed surface helper增加BlockingVolume／BrushComponent独立选择及Owner关闭变化拒绝，Trace49方法通过，合计63方法0失败；合成内存／原查询替身仍是CPU证据，不把它推广为实际Brush表面已关闭。日志和准确源在analysis/cover-system-20261108/nonzero-surface-r32。

新准确plan analysis/solo-m1-20261028/cover-nonzero-runtime-plan.json SHA256 5ad2a094e90eb003fba8f9ff2836b740aeed05047fc200c02040a819d0a2dfca；prepared equipment-controls-prepared-20261109-cover-nonzero-r32/prepared.json SHA256 f04ee0c4d178b2836ce4395954a45e242c160638e4eb5bc34c6696e62d94d209。27项声明driver及AST（含原idle／retention和review入口）、24项冻结源声明、3保存制品均核对，注入equipment／menu owner字符串compile／AST、prepare-only exit0。仍原r13 Native5cf4adea／PGf8e6f3c2／XR1684，没有新Native构建或混入后续C++。

运行范围为原A一次、当前严格圆柱观察一次、最多4次原非零Trace／30秒准入，首败停，不重复pause／B；0.4、后台observe-only、测试静音、物理输入隔离、身份新鲜度与存档音频／清理保护保持。旧r31实际执行driver已原样归档且结束哈希在交窗前核对，本轮源码更新不改写其历史结果。当前未拥有窗口、未stage／启动，新候选须沿实际已受理尾申请，不复用已经最终交出的r31槽。完整最终身体／physics形状、连续扫掠／终点占用／世界同步、Native提交与basis恢复、同版view／hit／muzzle、暂停／身体遮挡与所有CV／AC／实体头显仍开放；公开v71及历史计分保持。

## 2026年11月9日 r33原物理变换比较叶函数的准确ABI与有限性边界

r32源码已保存a35289683a80026caa1504ed92c28be8095a077b。经HB真实尾转交后，Longshot112230直接接受准确r32为其全部终态后的唯一后继，plan／prepared／3制品／27driver身份不变；当前没有更早已受理后继，未获最终交窗，不stage／启动，不复用r31。

继续检查实际物理消费接线：既有历史0x1400f26d4主路径在若干条件后调用0x1400f2278，再选择是否进入接收对象vtable操作。P1正常返回并不表示每个body都执行过更新。为缩小这一缺口，本轮只取已留存、代码SHA与KB相符的115字节比较叶函数，不调用物理更新入口、setter或接收对象vtable，不猜更新flags，也不对游戏附加／读写。

函数SHA为94ab2d89f2be090acd82574f9fa5b740edcde2c8aa100ac3104baa0f018a531c，原代码段911ab9bc、数据段c5deb081均完整核对。按入口实际RIP位移取得0x141707090的符号位清除mask和0x1417070ac的float32阈值17b7d138，约9.999999747e-5；不使用反编译旧标签直接充当实际地址。该数值是输入变换分量的原阈值，未证明可直接按游戏单位或米解释。

第一版隔离小程序误信缓存反编译的四参数原型，52次均SEH访问失败且输入未改，原失败源码与JSON保留；不能当成引擎运行失败。实际入口mov r11,rcx／lea r8,[rdx+36]／sub r11,rdx表明前两个寄存器承载输入，R8／R9用于内部计算。按该真实用法改为两个只读指针后，115字节及常量未变，隔离CPU重放52次正常返回。其中50个有限输入对照（相同、12分量分别正负小／大扰动、零值）与分量绝对差阈值一致，输入均保持只读不变；其余两次仅作非有限行为观察：单个平移分量quiet NaN返回1，正无穷返回0。不能由此把NaN姿态当成相同或有效，也不将有限样本对照推广为完整物理语义。

小程序MSVC C++17／O2／W4／WX／EHsc／UNDEBUG／MT编译0、修正后exit0，低于正常优先级；只在自身映射131072字节历史代码／常量及只读输入页，W^X分离，结束正常释放。首次准备脚本相对路径错误、未生成build.cmd而未编译／运行的失败也保留；首次mask字按float显示为NaN的准备记录保留，最终准备记录使用uint32位mask。本批原始文件、准确源码／EXE／依赖SHA、失败及最终结果在analysis/cover-system-20261108/physics-transform-gate-r33。

此结果明确后续逐body观察须使用准确调用边界、显式有限性检查和实际body覆盖；不能把这个宽松比较谓词、普通P1或辅助路调用完成当作HitInfo／枪口／碰撞世界同步。现有Local／SpaceBases／Matrix有限性守卫保持，不新增或放宽生产许可。具体物理接收对象vtable目标、同步／延迟查询可见性和完整消费仍未认证。没有新Native DLL或本批游戏验证；准确r32候选不变，公开v71、全部CV／AC、身体遮挡／暂停及实体头显仍未完成。

## 2026年11月9日 r32实际36040：非零表面严格通过，离开重叠返回空

Longshot112230实际12980全部运行、审图、编码、恢复和retention终态后最终唯一交窗，独立CIM／idle核空闲，重新核plan5ad2a094／preparedf04ee0c4、27driver及AST、24冻结源声明、3保存制品、所选EXE与实际Python解释器SHA和恢复基线。prepare-only0后stage准确旧r13 Native5cf4adea／PGf8e6f3c2／XR1684，备份prior-private-text-20261109-115208；没有把r33重放或其他新C++编入游戏制品，公开v71不变。

game36040 creation134386699398055069，owner15000 creation134386699323010745，session31aabc3e。原A一次CT2／CA0，geometry及当前严格圆柱观察通过，原CollisionRadius34／CollisionHeight72保留原义。四次查询均正Extent(10,10,10)，queue2／call1、submit0、pending_possible=false／allocation_retained=false且完整严格关闭通过：swept_extent独立取得BlockingVolume#194／BrushComponent#53，静止分离返回空，静止初始重叠命中同一组件且法线(0,0,0)，leaving_initial_overlap返回Actor0、位置／法线零。原表面、组件变换、当前角色／掩体／模块／函数及float32输入全程核对。单次最大11881读／353943字节，预算未改，没有重试或第五次调用。

这实际证明r32独立非零表面准入在本场景通过，修正了r31将线网格身份套到体积查询的准入假设；不能把r31旧首败改绿。当前整段预设要求leaving也命中，实际返回空，故Volume和操作器28264明确exit1。四次strict accepted与整体semantic contrast失败分别记录，session自身passed=true也不覆盖操作器失败。静止重叠中心来自本局严格首扫接触，查询位置约(-3906.07884,-194.50493,584)；静止返回该float32位置、零法线，不能推成穿透深度或恢复向量，也没有原始FCheckResult.Time。

旧r16隔离三角叶函数的离开重叠行为不能推广为本次完整Actor.Trace／BlockingVolume行为。本局至少观察到“先静止重叠命中，再由该位置向外移动返回空”，所以moving miss不能证明初始未重叠。当前只有此场景／此诊断盒的严格区间证据；同Actor及变换不证明原子碰撞世界，也不证明终点时相或整个人体安全。后续真实提交端必须分别消费起点占用、连续路径和终点占用；已重叠状态须走经认证的原版恢复／退出链，不能简单把wantedHit改false、把本局改绿或用空返回放行。不能重跑未改的同四项来制造修复结论。

Native关闭、入口恢复、pad恢复通过；unified fire_disable0／hand_stop5／drained=true，XR0／launcher0／game absent。EndSession=false、terminal exiting／objects destroyed／shutdown complete=true分别保留。没有重复pause／B，33420旧暂停失败仍开放。wrapper工具74831与encoder11063均实际terminal0，操作器terminal1保持。全部1页paired overview与6张关键原图亲审：game00375／00428／00444，simulator00443／00498／00515；两路严重右身体／肩／装备近裁剪遮挡仍FAIL，启动第三人称／等待卡仍有。game451帧跨度24.6168345秒，simulator522帧24.5835005秒，最大捕获间隔1933.3558／316.6676ms；动作窗最大99.9992／66.6695ms、尾部有效。启动game1.050–2.983秒捕获缺口不直接证明渲染停顿，WGC／窗口标题FPS不当render或头显证明；ffprobe视频24.666667秒／5668305字节。

唯一retention85297实际terminal0／errors0，删除4342项生成媒体824496767字节，保留36040／12980／65044最新三局48小时，源／构建／资产／存档／文本不删。config和XR settings原字节、head原不存在、所选game settings SHA、ending27driver与3stage制品均独立核对一致，最终CIM／idle无残留。准确game-36040-review.json、closure-proof.json、selected-visual.json、retention-result.json和preflight在analysis/cover-system-20261108/nonzero-surface-r32。

运行期间已直接受理HB115111为本槽唯一后继，末尾独立核其准确preparedebdf0792、source子目录2251声明源及6保存制品匹配后，已最终明确唯一交该后继。首次核对误把source_archive自身当源根，发现实际源在其source子目录；没有源不匹配或运行修改，准确路径记录于successor-check.json。新Frag114500申请已沿真实尾转HB处理，不作第二后继。当前不占窗、不补第二局；完整最终身体／physics形状、路径／终点占用与世界同步、Native掩体提交及basis恢复、同版view／hit／muzzle、身体遮挡／暂停和全部CV-01–24／AC-01–22／实体头显仍未完成，目标保持进行中。


## 2026年11月9日 r34物理来源读取器与只读候选

r32实际已证明moving miss不能排除起点重叠，下一步不重复原四项，而是核对当前人物物理实例与原身体更新接收者，明确后续同姿态物理／命中消费的真实来源。新增tools/game-playtest/cover_physics_state.py，复用原CoverQuery的16384读、2MiB、3秒预算及完整身份、反射字段、数组、注册表和值关闭检查；只读，无引擎调用、排队、分配游戏内存或写入。

读取绑定同一当前PC／Pawn／World／CoverLink／slot、非零CoverType和当前mesh；按反射类型与原消费者偏移核PhysicsAsset／PhysicsAssetInstance、Bodies／BodySetup完整同数量数组、RootBodyIndex、每项BodyIndex／OwnerComponent及BoneName。PAI Owner只接受准确当前Pawn或mesh，Body OwnerComponent必须准确当前mesh。数组1–64项，超容量、空项、重复项、布局不符或关闭变化均拒绝，不截断为成功。BodyData和BoneSpring只按认证的8字节Pointer结构读取；空接收者明确保留，别名只统计唯一数。非空指针只读取vtable的40／104／728数值槽及32字节目标前缀，不调用，不猜方法语义、可执行模块或物理变换。

四段原更新代码全字节比较历史已核artifact；实际已安装的主入口32字节只可来自现有HookEntryAttestation的当前进程／owner／准确Native SHA封印，同时在原Query里保留实际已改写入口，结束时再次核封印。不改Resolver，不跳过入口前缀，不加Hook。源、数组、指针和前缀关闭不证明物理活动场景、骨骼映射、更新覆盖、有限物理姿态、查询同步或同版hit／muzzle；这些输出许可全部保持false。

12项读取器测试方法通过，覆盖完整来源、准确Owner／Index、数组类型／边界／重复／超限、指针结构、空及别名接收者、完整代码与live封印、BoneName及注册表、关闭期间变化、原预算、64项不截断、结束封印失败和当前cover失效；原HookEntryAttestation的10项通过记录另存。三body与64body夹具分别504读／16536字节和2212读／31826字节，均为合成内存，不是实际游戏成本。初次导入路径和夹具构造／类型链失败保留；新增关闭测试最初两个目标值恰等于夹具原值，改为真正变化后12项通过，没有放宽生产读取器。

只读候选equipment-controls-prepared-20261109-cover-physics-r34沿用准确原r13 Native5cf4adea／PGf8e6f3c2／XR1684，仅复制3个原保存制品并复核24项冻结源声明；没有重编或混入当前其他Native／武器源。新plan70ee042c、prepared59770ded，33个声明driver及AST、原owner注入字符串编译、prepare-only均通过。新操作器仅接受--cover-physics-source-only，原A一次后读取一次当前物理来源；禁止旧volume／cylinder／nonzero控制，不重跑已知pause／B失败。保留原关闭、恢复、双路录像和retention路径。

本批尚未申请／取得新窗、未stage、未启动游戏或XR。下一步完成源码保存后，按当前实际队尾申请新槽；仅最终唯一交接及独立idle／准确候选核验后才能部署。读取若遇当前空PAI、偏移／类型或Owner不符，应保留首败，不据函数名强行调用或放松认证。后续仍须完成真正物理姿态消费／查询世界同步、完整身体路径与终点占用、Native提交与合法恢复、同版view／hit／muzzle及全部CV／AC；公开v71与头显状态不变。

需求文档同步补明保持位置的持续占用、被占据后的合法恢复、同姿态命中和未认证骨架／联机边界，保留24项CV和22项AC。证据analysis/cover-system-20261108/physics-source-r34/verification.json及相邻原日志。


## 2026年11月9日 r35保存姿态的原物理元数据补解码

等待r34新尾位期间，复用61492-numerical中保存的原CPOSE001二进制，而非再次启动游戏。原receipt绑定game61492／creation134386467119153096、Native6a6a1f9a、session9812988712167，原文件594792字节SHAf62f9bbe66c4a4e1fc60e0de87b47397a3e9269c74bd5f2da1a459554a36a018。新离线解码器直接包含准确冻结r7 pose结构，静态核Header／Snapshot／Row／Episode尺寸56／84920／84936／594736；按原session、Pawn、mesh、完整文件长度、episode关闭和逐row原witness条件读取，五项有效行与原解码结果完全一致。没有将未见到的main P0／P1两行补为存在，也没有读取已退出进程。

五个有效阶段为C0、C1、late P0、late P1和L1，全部记录同一非空physics_asset2180350880、physics_instance2309769472及master0；这消除了“该历史场景必然没有物理实例”的猜测，但地址仅属于该已退出游戏，不能给当前r34授权。各行tick_epoch原值1055／pose_epoch1054、flag318=0、flag32c=0保持；flag350由C0／C1的44419变为late P0／P1／L1的42371，原差值2048即0x800。原字段名沿保存结构，未把这些数值命名为活动场景或同步栅栏。原body更新返回、字段稳定和指针非空都不能证明每个body已更新，更不能证明Trace已消费同一姿态。

新解码器MSVC /std:c++17 /W4 /WX编译0，低优先级离线执行0，原文件hash、原解码行号和冻结header身份记录于analysis/cover-system-20261108/retained-physics-metadata-r35/verification.json。第一次build脚本在已进入的目录又拼接相对根，未创建脚本或执行构建；错误日志保留，修正路径后正常完成。仅生成私有离线分析制品，不修改r34的33driver、plan／prepared或任何Native／PG。

r34同一请求已沿实际尾直接受理：Bol115420→HB121040→Frag122000→Snub122512→Coverr34。Snub核准确prepared59770ded／plan70ee042c、24原r13声明源、3制品和33driver后明确接受；本会话确认唯一前驱且无已受理后继。此为排队，不是最终交窗，不据无游戏进程接管。下次动作仍是前驱全终后核准确候选和独立idle，再执行一次当前物理来源只读；实际接收者、更新覆盖、命中／枪口和碰撞世界同步继续待证，全部CV／AC／头显范围不缩减。


## 2026年11月9日 r36原物理资产组合形状读取

为完整身体体积检查补齐真实形状来源，新增cover_physics_shapes.py::read_setup_shapes。它接受调用者当前CoverQuery及准确RB_BodySetup，仅在同一查询中读取KMeshProps.AggGeom的四类完整数组：SphereElems、BoxElems、SphylElems和ConvexElems。原包保存的结构导出经SHA复核，仅用于字段／类型候选；导出中的@NULL、缺失声明和反编译错误不能作为内存布局。所有实际字段偏移、宽度、数组stride、目标结构、继承与Boolean掩码均要求当前反射给出并严格核对；KMeshProps、KAggregateGeom及元素声明归属绑定，缺失、重复、额外字段、重叠、错误类型或关闭变化均拒绝。

球／盒／胶囊读取原TM Matrix四个Plane及正的Radius／X／Y／Z、非负Length、bNoRBCollision。Plane继承Vector的XYZ、自己声明W的真实结构形状被覆盖，不能只数直接子字段。保留原始字段数值和有限矩阵，不推断世界坐标、单位、轴向、半高／全高或刚体正交性。凸体保留VertexData、PermutedVertexData、FaceTriData、EdgeDirections、FaceNormalDirections、FacePlaneData和ElemBox的完整有界原值；未认证cook、索引拓扑、实际启用形状或边界权威。

单setup总形状上限64，顶点256，拓扑数组1536及单数组32768字节；超限明确拒绝、不截断成功，原Query的16384读／2MiB／3秒不变。空数组表示没有该原始形状，不制造替代体积。输出明确closing_required=true，调用者必须在同一当前身份查询中完成finish；完整body／mesh关联、原查询非原子性及真实场景同步责任仍在原来源与消费层，不能把此helper单独用作许可证。

8项定向方法通过，涵盖四类混合形状、继承Plane、无操作／只读／所有许可false、空数组、非法／非有限尺寸与矩阵／顶点、数组类型／指针／容量、完整字段／Boolean、声明归属、关闭期间payload／header／metadata／registry变化、64形状全读及组合超限、原预算拒绝。四类混合合成夹具698读／8898字节；不是实际游戏成本。r34原物理来源12项回归通过。首次夹具缺inner完整metadata和D0尾部映射、随后三项变更在最终值扫掠之后才触发的失败均保留；变更测试改为关闭开始时触发，不改原Query，不声称原子快照或修复并发物理窗口。

本批没有Native／PG编译、游戏／XR或新的运行申请，未修改r34 plan70ee042c／prepared59770ded及33driver；未来须在实际r34来源确认后将形状helper接入同一次来源查询，再核原始形状、活动接收者、骨骼变换和真实命中消费者的对应。当前生产抓边仍不能通过空edge／零range或no_cover检查，未放宽任何准入。完整身体扫掠、起点／终点占用、世界同步、Native提交与恢复和same-pose view／hit／muzzle仍是后续必做项。证据analysis/cover-system-20261108/authored-shapes-r36/verification.json与原测试日志。


## 2026年11月9日 r34实际20596物理来源读取通过及完整收尾

Snub122512实际56176全部终态后的最终唯一交窗已收到，独立idle／CIM、plan70ee042c／prepared59770ded、33driver与AST、24原r13声明源、3保存制品、所选EXE与实际Python SHA核对后，仅stage已冻结r34；备份prior-private-text-20261109-130555，公开v71不变。没有把r36形状helper或当前其他Native／PG源码混入本局。

game20596 creation134386743788278499，owner72144 creation134386743712526676。原A一次进入CT2／CA0，当前几何只读及physics来源严格关闭通过；29个Bodies与29个BodySetup逐项Index／OwnerComponent／BoneName关联，RootBodyIndex0，PhysicsAssetInstance.Owner准确当前Pawn；29个不同主接收者，共用一个vtable，BoneSpring辅助指针全部0。骨名涵盖Pelvis、腿／脚、两段Spine、Head、左右锁骨／臂／手、肩甲及背包／挂点；这些是原来源名，不等于115骨映射或实际活动命中体已全部认证。四段原更新代码、唯一入口原32字节live封印、实际已改写前缀及metadata／registry／value关闭均通过。5668读／60516字节，读取341.8213ms；零Trace／getter／virtual调用、零提交或新Hook，无身份／输入时限放宽，也不将观察时间续作输入票。

wrapper工具50167终态0，operator69988退出0，launcher0；Native cleanup／entry restore／pad restore均true，fire_disable0／hand_stop5／drainedtrue，XR0，原game退出。EndSession=false与terminalExiting／objectsDestroyed／shutdownComplete=true分别保留。config／XR settings原bytes、head原不存在、所选game settings SHA、wrapper还原、ending33driver及3stage制品全部独立核对一致。没有重复旧pause／B，33420旧失败仍开放，也没有重跑r32四项。

全部1页完整paired overview及6关键原图亲审：game00339／00355／00364和simulator00423／00439／00448，分别覆盖进入、物理读取和关闭。游戏镜像及XR双眼均有严重右肩／身体／装备遮挡，视觉仍FAIL，启动第三人称及等待卡保留。物理读取相对paired时钟20.7932975–21.1351188秒；game371帧跨度21.4834644秒，simulator455帧21.4667975秒，动作末端两路覆盖通过。最大捕获间隔2200.0231／333.335ms，game大缺口在启动1.1166822–3.3167053秒，不能当作渲染停顿或头显FPS。编码工具82423实际终态0，ffprobe21.5秒／4777357字节。初次挑图脚本误将JSONL resize事件当frame而缺captured_qpc_ns，错误单独保留；随后仅筛实际frame，未改图、重编码或隐藏缺口。

唯一retention工具38495实际终态0，删除769项生成媒体119065870字节，保留20596／56176／28120最新3局及48小时，源／构建／资产／存档／文本数值不删。最终再核Longshot125546准确prepared747365d8／readyc232473a／plan082fe9dd、source子目录1592声明源、6保存制品、18当前driver＋world_state及AST匹配，再次独立idle／CIM无运行或媒体作业后，已最终明确唯一交该后继；对方已确认接管并stage其准确候选，本会话不再占窗或补本局动作。旧Longshot124434未部署，后续Bol125154→HB125528→Snub131050按真实尾维护，不产生第二后继。

本局只关闭“当前身体与物理来源／接收者关联”这一项。所有active_physics／physical_pose／hit_geometry／collision／world_synchronization／atomic_snapshot许可仍为false。r36原始形状尚未在本局读取；完整身体扫掠与起终点占用、当前物理世界可见性、实际Native提交及恢复、同版view／hit／muzzle、严重身体遮挡和全部CV／AC／头显要求继续未完成。记录analysis/cover-system-20261108/physics-source-r34的game-20596-review、closure-proof、selected-visual、retention-result及preflight。

## 2026年11月9日 r37实际虚函数目标的离线模块对应

从20596已严格关闭的三个32字节目标前缀开始，只读取所选原游戏Binaries_x64中的PhysXCore.dll和PhysXLoader.dll。PhysXCore SHA22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48，AMD64；原32字节出现次数分别2／1／1。首次“每项前缀必须全局唯一”的假设因此拒绝，保留失败，没有选择第一个匹配或将它命名为方法。

三个目标位置的共同基址交集只有0x7ffb5f120000且64KiB对齐，对应原PhysXCore三个.text RVA0x13c270／0x1360f0／0x136890。实际29接收者共用vtable的RVA0x2330a8位于该文件.rdata，40／104／728字节槽的原磁盘指针在同一重定位基址下逐项精确匹配实际读值。这是更强的文件内容／地址对应，并不补签本局未保存的完整加载模块清单，也没有执行任何目标。

只用既有vendor PE解析器读取磁盘，未启动IDA、游戏、XR或进程读取；原始2／1／1匹配、相同基址筛选和静态vtable关联记录于analysis/cover-system-20261108/virtual-targets-r37/correspondence.json。第一次在普通Python直接import pefile缺少vendor路径，随后沿现有package-inspector/python-libs引入；未安装依赖或改变环境。下一步在这个准确PhysX制品上解析三入口及实际接收者类型／更新语义，再认证query-visible时相；不能凭槽号、函数返回、原始姿态或DLL内容匹配强行调用未知ABI，不能重跑不变r34来源来代替消费证明。


## 2026年11月9日 r38原PhysX更新入口与锁及读回路线

沿20596实际目标与原DLL的内容／虚表对应，在相同SHA22051aa7…ddd48的PhysXCore.dll上使用已可用的REA Ghidra12.1.4提供者，仅离线分析8个相关函数。未修复或重配不可用的IDA，也未启动GUI／游戏／XR或调用任何目标。三个目标的原诊断字符串和实际调用流分别支持以下身份：

| 实际vtable字节槽 | 原DLL RVA | 静态识别与完成边界 |
| --- | --- | --- |
| 40 | 0x13c270 | 原诊断标明NpActor::setGlobalPose；先比较当前12项姿态，相同可直接返回。不同姿态仍须通过写锁准入，失败时跳过调用；静态／动态附属体分支不同 |
| 104 | 0x1360f0 | 原诊断标明NpActor::moveGlobalPose；先核写锁，再核可用附属体及模式，可能只报告错误并返回；有效分支调用附属体vtable+0x168 |
| 728 | 0x136890 | 原诊断标明Actor::readBodyFlag；附属体存在时读取其vtable+0xf0结果并与输入掩码求交；缺失时走dynamic错误分支 |

写锁准入helper0x1a55d0读取原actor+0x18关联的锁对象，核计数与线程，进入原Windows CriticalSection；不属于当前线程的占用可返回失败。0x1a5650负责原释放。两个更新入口的汇编都在调用准入前显式从actor+0x18取参，避免采信set入口反编译中遗漏的参数。正常原返回不能单独证明这次实际更新，更不能代表所有29项和碰撞查询已同步；本轮未证明这些跳过分支在20596实际发生，不把静态分支当作本次缺陷的因果结论。

vtable+0x48对应的姿态回传入口0x133fe0进入helper0x1209f0。后者在actor+0x40附属体为空时取原缓存，否则分别经附属体vtable+8／+0x10取得位置与旋转，再组成12个浮点输出。set入口的动态分支使用附属体+0x28，move使用+0x168；不能把两者自动视为同一种完成语义。当前附属体的具体目标／存储布局、游戏与物理单位及矩阵方向、活动场景和query-visible时相尚未认证，不直接调用这些getter或猜结构偏移写值。既有有限性检查保留。

另用准确历史section_1000全SHA911ab9bc…ce6ae6，从已保存原函数入口完整线性解码后定位调用：0xf9ae6前R8D=1／R9D=1，0xfac64前R8D=0／R9D=0；更新函数实际读取0x7e和0x80掩码，并依据R9选择set／move路径。该证据是历史原机器码，不是本局新记录的输入flags，不能据此修改flags或强迫所有分支更新。最初局部caller切片始点落在前一条指令内部，已单独标记无边界权威，替换为完整函数入口锚定解码；原helper／updater主块仍由各自准确入口解码。

REA导出8份完整保留记录，5,147,951字节，SHA7f93687ec7e3766b0d844e500a86a8a1317d2c25bfa44a315075482695b70ab6；各记录value-flow未截断，但间接目标及若干trap/no-return依据仍明确未知。原PE将部分函数拆成多个相邻unwind span，最初“单个.pdata必须等于整函数”比较不成立；已逐项核整个Ghidra观察范围被原连续span分区精确覆盖，保留原差异，不将range覆盖升级为运行或同步证明。静态code certificate记录8个完整观察范围的字节SHA，仅用于后续当前代码认证，不包含调用／写入许可。导出完成后正常close_binary，复查open=false，两个分析工具句柄均已完成。

证据目录analysis/cover-system-20261108/physx-entry-r38，包含rea-evidence、static-code-certificate、caller-flags-anchored、lock-call-assembly、unwind-boundary-differences及verification。下一实际接线应独立认证当前加载的PhysX模块、真实actor附属体及其位置／旋转目标，并将r36完整原始形状、实际BoneName映射和同一C0／物理消费／查询时相绑定；不改变现有HookEntryAttestation仅认证game／Native两base的封印合同，不用旧地址、锁计数或“正常返回”签发身体位移。当前无新Native／PG构建或候选，未重跑r34，所有碰撞／实际提交／恢复／命中／枪口及CV／AC／头显缺口继续保留。

## 2026年11月9日 r39当前PhysX内部来源与原始形状组合接线

r38静态分析后，新增cover_physx_source.py，独立使用现有Resolver只读句柄枚举模块，完整上限2048项，缺失／歧义／路径截断／重复base拒绝；不改变HookEntryAttestation仅game／Native两base的合同。只接受所选game同目录PhysXCore.dll，准确2711552字节／SHA22051aa7，PE完整headers与当前读值一致，8段r38原函数全字节认证。原DIR64重定位逐项应用，vtable槽必须落在原文件只读数据、目标在非可写可执行区并比对准确原前缀，不用仅首个相同字节猜模块。结束再次枚举当前加载路径／base、核文件身份与全内容，并关闭同一个原CoverQuery。最初完整stat比较将读取导致的access time改变也当失败，已改为size／mtime／ctime／file ID及全内容核对；不忽略内容变化。

准确actor的40／72／104／728字节槽绑定r38分析过的目标，再只读actor+0x40内部体指针和内部vtable的8／16／40／240／360字节槽。空内部体保留为缺失，不推成动态身体；非空内部目标仅取得准确模块／代码来源，位置／旋转返回布局、调用ABI、活动场景和更新消费时相仍未认证。零virtual／getter／Trace调用、零写入或新Hook，不读猜测的姿态内存，也不赋予物理同步或身体位移许可。

cover_physics_state.py仅在新显式选择下，在同一查询中读取内部来源及各RB_BodySetup完整Sphere／Box／Sphyl／Convex原始形状。ShapeReader只共享本次查询中的反射布局，拒绝另一个Query的reader；每setup的实际header／payload仍逐项读并在统一关闭核对，不能跨样本缓存。原r34默认来源行为保持；新operator显式--cover-physics-source-only加--cover-physics-internal-shapes，不执行旧r32四对照或pause／B。

33项定向方法通过，包括原来源12、原形状8加同Query约束1、新PhysX12。使用准确原DLL磁盘内容配合合成加载内存，核完整code尾、重定位后的表、缺失／错误路径／双同名／容量及重复base、内部指针、当前模块变更、结束时code／pointer／shape变化及原预算拒绝；这些测试不是当前进程证据，合成内部vtable特意复用可认证原表，不声称真实内部类型已定位。29身体混合四形状7465读／96020字节；64身体256形状15305读／174547字节，均完整关闭，不截断，原16384读／2MiB／3秒不变。首轮30项的1错误是负例只预期RuntimeError而实际只读缺页抛OSError；1失败是变更注入在值扫掠后。测试改为接受真实读取错误并在关闭开始注入，原Query未改，atomic_snapshot仍false。首败及最终原日志保留。

原r13三制品按原SHA复制并复核24冻结源声明，没有重编Native／PG或混用当前武器源码。新equipment-controls-prepared-20261109-cover-physics-r39的prepared SHA8917db74a2bf8ca991380d6b701835e9e012f0c8f78163c24fa02fb966235c0a；cover-physics-combined-runtime-plan.json 初始SHA c73d4f85975ef3e03444cf64502f990391c455dfeed024e5733e0eac5ab0c677（旧prepared关联字段有误，未部署）；当前同槽修正SHA 2b96e8d9976d2501050fd2fd4795953a5f317fcf14a052fc76981b4e77fde456；36个当前driver及AST、prepare-only通过，准确driver已归档。已向原已知队尾Snub申请后续唯一槽，尚待真实尾确认，不凭队列消息或空闲文件接管。未stage、未启动game／XR、未操作其他owner的实例。

下一步在最终唯一交窗且独立idle／候选完整复核后，执行一次当前组合来源读取，保留首败，认证实际内部目标再推进原始形状与BoneName／模型映射、同C0物理姿态、查询可见性、Native真实提交与恢复和同版view／hit／muzzle。原始资产形状不是活动场景形状，代码来源不是姿态消费，任何一个源读取通过都不能关闭CV／AC或视觉／头显缺口。公开v71、严重肩／身体遮挡及旧暂停失败状态不变。证据analysis/cover-system-20261108/combined-physics-r39。

## 2026年11月9日 r40完整参考骨名称与物理身体的纯映射

等待r39当前来源运行期间，新增cover_physics_bone_map.h，复用已有full_body_c0_planner的Model／Frame和Native RefBone68拥有缓冲契约。先核准确Marcus115几何签名、全部canonical字节及Model的parent／q／T／scale一致，再逐项比较完整FName index与number，将最多64个RB_BodySetup名称映射到真正参考骨索引，保留RootBodyIndex并报告对应骨。BodyIndex必须与本次完整输入顺序一致；无名称、重复参考名称、重复身体骨、缺失骨、模型或参考变化全部明确拒绝，输出保持原样。不用BodyIndex替代骨索引，不根据115个数推定兼容，不读取Python反射中IntProperty形式的opaque RefSkeleton。

同时报告每个已命名身体最近的已命名骨架祖先；这只是骨架关系，不是PhysX约束父级、活动岛或刚体连接证明。该纯模块不打开进程、不调用getter、不发布身体、不改变原物理源读取或创建第二writer。名称注册表／原生布局及当前World／Pawn／mesh／asset／PhysicsAsset／BodySetup、原始时间和C0身份必须由实际调用者在现有边界独立认证；此模块尚未接入当前Native运行包，不能单独签发当前映射或命中／碰撞许可。

MSVC /std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译0；低于普通优先级的纯离线测试0，178断言／0失败。使用61492保留reference.bin，经真实BCrypt SHA准备模型；身体名称输入为从拥有参考记录中明确选择的合成值，未将20596的跨局FName索引直接拼入61492。覆盖29项、乱序／跳过骨／根骨未参与的祖先关系、非零FName编号、错误索引／名称／重复／缺失、参考及Model改动、空／0／65项／root越界、缓冲别名，以及64项完整反序映射，不截断。全部工作区编译依赖按准确/sourceDependencies清单归档，源和制品SHA在physics-bone-map-r40/verification.json。首次构建从根目录调用，产物落在根而非证据目录，已核依赖Source准确属于本测试后将这三份自产物移至独立目录，并修正后续build.cmd自定位；没有重编Native或改其他产品源。

r39唯一前驱互核发现初始plan c73d4f85仍含旧r34 prepared_sha25659770ded。已保留首个拒绝，仅修这一个plan字段；prepared8917db74、36driver、源提交bec4d12b和3制品均未变。当前plan2b96e8d9976d2501050fd2fd4795953a5f317fcf14a052fc76981b4e77fde456，36driver SHA／AST、24声明源、3制品、所选game／PhysX重新核对及prepare-only0；Longshot133536独立核验0并正式接受唯一后继位置。旧plan明确退役，未stage／run。实际链HB125528→Snub131050→Frag131651→Longshot133536→Coverr39，以最终交接为准；本会话暂无受理后继，不以排队消息当授窗，不读取其他owner游戏内存。

后续仍先执行准确r39只读组合来源，认证当前内部目标及原始形状，再把准确本局BodySetup名称与拥有的同C0全参考记录关联。此后才可继续姿态消费、活动碰撞形状、起点／路径／终点占用与世界同步、实际Native移动／恢复、同版view／hit／muzzle及全部CV／AC。178断言只证明纯值映射，不关闭严重身体遮挡、原暂停失败、完整掩体或实体头显验收。

## 2026年11月9日 r41原引擎骨名缓存与当前C0参考对应

原MatchRefBone并不直接线性遍历RefBone68，而读取SkeletalMesh的骨名缓存。复核知识库中原IDA已保存函数及SHA，并从准确历史section_1000全blob SHA911ab9bc按认证入口0x1d38d4完整解码151字节，逐指令确认mesh+0x218资产、asset+0xb8 pair data／+0xc0 count／+0xc8 hash buckets／+0xd0 hash size，pair stride16、next+0、Name index+4／number+8、骨索引+12；hash按Name index与size-1取桶，完整两字段比较。IDA旧伪代码参数恢复不准确，只沿已有原代码证书与完整汇编确认这些值流，不从伪原型调用。

新增cover_engine_bone_map.h，只读核同一拥有Frame／Model全部Marcus115 canonical及实际asset参考数据，再遍历原hash桶和完整pair链，要求所有115骨名称和索引与参考逐一一致；越界、环、跨桶、重复索引、缺失、非法header／key及读取故障均拒绝。Map+12原函数不消费，保留为opaque四字节并在关闭核对，不猜成容量。原八段getter主／cold代码在两端完整复核，但不调用任何getter；读原参考、缓存header／payload后再复读，身份guard贯穿。重复相等仍不是原子快照或物理栅栏。

在现有full_body_c0_native_backend.h增加显式observe_engine_bone_map方法，绑定已capture的准确asset／source参考、原线程、stop状态、原Context及完整metadata／immutable readback；外部workspace和输出不得与Backend、实际Local／Space／reference／Desired、mesh或asset内存别名。该方法不参与原Kernel prepare／commit／finish，不给写入票，不改变债务账本或刷新来源。仅被明确调用才观察，目前未接入运行候选触发，未编译新Native DLL。

专用读取器MSVC/W4/WX/UNDEBUG编译0／184断言0失败；完整夹具42读／26992字节，使用真实历史code和保留参考配合明确合成缓存，检查42个读取点逐一失败时不重试、三次guard拒绝、header／chain／key／index／coverage、结束变化和code尾改变。实际Backend拥有内存页整套498断言0失败，包含成功缓存关联、未capture／错线程／stop、Context／metadata／reference／代码拒绝、停止发生在末guard、输出别名Local和workspace别名Backend均不写游戏形态缓冲；原CAS／回滚／恢复／已消费姿态检查继续通过。首个Backend编译因新增测试缺<vector>失败，原日志保留，补测试include后编译0；没有放宽生产检查。上述均是离线Host／拥有内存页，不是当前游戏映射或physics消费证明。

证据analysis/cover-system-20261108/engine-bone-map-r41，包括完整原函数汇编／原伪代码及SHA、全部工作区编译依赖、两项原测试日志和首编译失败。r39原36driver和原r13三制品未变。前驱Longshot133536实际61036已按本段所有运行／双路亲审／恢复／encode／retention全终，独立idle并核候选后最终明确唯一交窗；本会话现保存源码后再做自身idle及全冻结复核，只有准确r39可stage／run。HB135655（prepared0d89d185／plan3f1547e8／inputsa9aa206f）已直接受理为r39唯一后继，排队不授窗。

## 2026年11月9日 r39实际59768首个模块头拒绝与全收尾

Longshot133536实际61036全终后的最终唯一交窗收到；本会话先保存r41源码345ecc992b431ad19e0708f9ea6f1c01de8d47ff，再独立idle、prepared8917db74／plan2b96e8d9、36driver及AST、24准确原r13 snapshot声明源、3保存制品、所选game／PhysX及实际Python SHA全核。仅stage准确原r13 Native5cf4adea／PGf8e6f3c2／XR1684，备份prior-private-text-20261109-141802；r40／r41 C++没有混入此二进制，公开v71不变。

game59768 creation134386787092097890，owner72356 creation134386787018858522，session57551c54。原A一次到CT2／CA0，geometry只读接受；随后组合来源在第一次q.read（1024字节完整PhysX PE头）即以PhysX current complete PE headers differ拒绝。该读取调用总146.6023ms含准备，不作为每帧成本；原模块路径枚举／磁盘SHA准备不能覆盖header拒绝。未到原game physics chunks、身体数组、内部接收者或authored shapes，零Trace／getter／virtual调用、零body写入。原operator62668退出1、cover-boundary passed=false及错误保持；session passed=true仅属原生命周期，不能改成本掩体通过。没有追加本局或重跑未改四对照／pause-B。

本版拒绝时未保存当前PE头原字节，因此差异字段和原因均未知；不能推成加载器正常改动、文件身份或可放宽header的证据。实际失败保存在game-59768-review.json及原cover-boundary SHA，不补成已有差异回执。

wrapper工具42455终态0／launcher0／原game gone；Native cleanup／入口恢复／pad均true，fire_disable0／hand_stop5／drainedtrue，XR0。EndSession=false与terminalExiting／objectsDestroyed／shutdownComplete=true分别保留。config和XR settings原bytes、head原不存在、两game settings SHA、wrapper恢复、ending36driver与3stage制品均独立核一致。

全部1页完整paired overview＋6关键原图亲审：game00351／00366／00370和simulator00438／00452／00457。镜像和XR双眼严重右肩／身体／装备遮挡仍FAIL，启动第三人称及等待卡保留。物理来源尝试在paired时钟21.3677183–21.5143206秒；game376帧跨度21.8834717秒、simulator463帧21.8334716秒，两路末端覆盖均通过。最大捕获间隔2250.0304／333.3358ms，game长缺口在启动1.2500199–3.5000503秒；不把捕获间隔或窗口标题当渲染／头显FPS。encoder工具63834终态0，ffprobe21.9秒／4798339字节。

唯一retention工具7503终态0／errors0，删除2029项生成媒体373450560字节，保留59768／61036／53616最新三局及48小时；源／构建／资产／存档／文本数字不删。最后再次独立idle／CIM无本局残留，并核HB135655的准确prepared0d89d185／plan3f1547e8／inputsa9aa206f、其source子目录2251声明源、6保存制品、19当前入口SHA及AST后，已经最终明确唯一交该后继。本会话不再占窗；Longshot142004申请已沿HB真实尾转交，不作第二后继。闭合证据在combined-physics-r39的closure-proof、selected-visual、restore-check、retention-result、ending-driver-check和successor-hb135655-check。

## 2026年11月9日 r42模块头拒绝原字节回执

实际r39已关闭并交窗之后，针对首败补SourceRejection结构化回执：完整保留第一次已读当前PE头、准确原文件头、两侧SHA／长度、当前模块路径／base及全部差异偏移，外层来源读取器在失败输出中保存该witness。没有第二次读取，不改变完整PE头一致要求，不为差异分类添加豁免；回执closing／accepted／activation／cause_verified均false。它不能补回59768没有保存的字节，也不是模块修复。

34项定向Python方法通过。新增负例在合成当前模块头翻转一个字节，核原／现完整字节和准确offset都被保存，整个source仍拒绝，只有原1读／1024B、无body结果或内部／形状准入，原attestation关闭。原33项继续通过。r39运行时36driver的ending证明已在本次编辑前保存，旧driver副本仍在r39/drivers；新源码尚无prepared或新运行申请，不能使用旧plan继续启动。后续按新候选与真实尾取得窗口，先取得真实差异字节再决定是否存在有证据的修复；全部身体／物理／恢复／view-hit-muzzle／CV／AC与头显范围保持。证据header-rejection-r42。

## 2026年11月9日 r42独立模块观察入口与新候选

针对59768在完整PE头首次读取即拒绝，新增cover_physx_source.py::capture_image，复用同一CoverQuery、准确action_owner／session／game身份和当前模块选择。模块名／路径、准确磁盘SHA、完整header及8段已分析原code全部通过并完成原查询及身份关闭，才接受这次有限模块来源；回执明确不认证整个image、身体、碰撞、原子性或同步。若header仍不同，保存原1读的完整expected／observed／diff回执后拒绝，不重读、不屏蔽字段。

原cover_boundary_chain在当前Gameplay身份检查后、reader.prepare／发现掩体／行走／A之前增加显式--cover-physx-image-only分支；它只调用这个模块读取并返回，不取身体／形状，不执行旧完整掩体动作。标准启动及正常关闭继续沿原路径。选择器拒绝image-only与combined同时出现；旧body来源分支保持原语义。此为对已发生阻断的定向检查，不改变最终身体／物理／玩法验收范围；模块检查通过本身也不能宣称原掩体内失败已修复。

39项方法通过，包括原34项和5项image入口检查：CT0仍可只读模块；18读／7720B完整关闭，无接收者读／队列／输入／新权限；header负例仍仅1读／1024B且保存当前身份；结束时模块／header／code改变、owner变更和原预算超限均拒绝。以上均为准确原DLL与合成加载内存，未读取其他owner进程。

新候选equipment-controls-prepared-20261109-cover-physx-image-r42，prepared SHA03486f56dd6a37fb3f6515bd196a5fe51d623b7ac1198212dc7529d113a28bd1；cover-physx-image-runtime-plan.json SHA494fa8c0082eebbb8d10fb72b21c19267f13778665e2725931668112faf7f45a。复用准确原r13三制品，仅复制这3保存制品并核24snapshot声明源；36个当前driver及AST、prepared关联、prepare-only通过。只允许相对r39的四个已拥有诊断文件变化，其他旧driver保持原SHA；完整新driver已归档。wrapper现在执行prepared整体SHA关联检查。未重编Native／PG，未stage或启动Game／XR。

新请求已发当前HB135655，请其沿真实已受理尾直接转达，不作为其第二后继、不复用r39旧窗。尚未获得最终交窗；当前无占窗或第二次本局动作。后续先取得真正差异字节并核对应字段，再据证据修复模块认证，继续原组合身体／形状、同C0命名骨、实际物理／查询世界、Native提交恢复及view／hit／muzzle闭环。证据header-rejection-r42/image-prepared-verification.json、image-verification.json及39项原日志。


## 2026年11月9日 r42b实际首拒绝与加载基址对应修正

### 实际运行结果及清理

Bol144535/game71440全部终结并最终明确交窗后，本会话独立核对进程空闲、24份原r13声明源、3份保存二进制、36个驱动SHA／AST及所选游戏、PhysX和解释器，stage新r42b；旧未运行r42保留且不再使用。r42b的私有启动读取仅在原45秒期限内处理PermissionError／部分JSON，原身份、错误与就绪条件保持；原循环9个检查通过，共享脚本未修改。

实际game33732／creation134386827342691297，运行owner3340。原保护启动后，在掩体发现、移动及A输入之前读取PhysX模块；首次1024字节读取于36.8024ms内拒绝，操作器71228 exit1。没有读取身体／接收者／形状，没有Trace、getter或虚方法调用，没有掩体输入或身体修改。原失败保持为失败。

本次精确保留的差异为偏移322–325，全部属于PE32+的ImageBase字段（起点320、宽8字节）：磁盘优选地址0x180000000，内存完整字段0x7ffb5f120000，与独立枚举模块base相等，头部其他字节全部一致。这证明本次比较被该字段挡住；不证明由哪个加载组件改写，也不补签game59768未保存的原始头。字段定义回查[Microsoft PE格式](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-windows-specific-fields-image-only)。

已亲审1页完整跨度双路总览及6张关键原图。219张游戏、284张模拟器采样覆盖约13.53秒，双端末尾覆盖通过；游戏最大采集缺口2083.3422ms、模拟器350.0011ms，不是渲染FPS或卡顿归因。掩体外空手视图在镜像与两眼可见；startup第三人称和等待卡仍存在。本次未吸附，不能关闭此前59768的严重肩部／身体遮挡。视频13.566667秒、1554629字节。

wrapper18322 exit0、编码69749 exit0、唯一保留23530 exit0；Native、entry、pad、drain、XR0、launcher0及gamegone通过。XR终态Exiting／对象销毁／shutdown完成，EndSession成功标志为false，分别保留。config、settings、head原缺失和两份游戏设置精确恢复；源码修正前36个末尾驱动及3份已stage制品均一致。保留33732／71440／35152最新三局及48小时，实删324媒体／33954810字节、errors0；源、构建、资源、存档和文字数值不删除。独立空闲并核HB144203的2251份保存源／6制品／19当前驱动及AST后，已最终明确唯一交窗，不追加本窗。

证据：`analysis/cover-system-20261108/header-rejection-r42/game-33732-header-analysis.json`、`r42b-closure-proof.json`、`r42b-selected-visual.json`、`r42b-ending-driver-check.json`、`r42b-retention-result.json`和`r42b-successor-hb144203-check.json`。

### 修正范围与离线结果

`tools/game-playtest/cover_physx_source.py`从已经完整SHA认证的原始头构造第二种预期形式，仅将ImageBase替换成同一路径、唯一且独立枚举的当前base。只接受完整原头或完整绑定头的精确相等；不忽略该字段、不从被检头反推许可地址，也不放宽其他头字节、代码、重定位、对象身份或关闭复读。结果记录所选形式、原始／观察摘要和地址，loader原因仍未证明。原始优选地址仍用于代码及虚表重定位，不被内存头覆盖。

新增真实失败条件的回归先在原实现上FAIL；修正后42项PhysX来源、原形状和物理状态相关方法通过。负例包括其他加载地址、部分字段、零值、其他头字段变化，以及观察期间在两种单独有效头之间切换。精确game33732保存头的离线重放由旧1read／1024B拒绝变为新18reads／7720B对应；其中头及base来自实际保存，后续代码内存是原件派生的合成样本，不能称新读取器已游戏验证。最初新增用例的Python关键字语法错误已修正，首次日志保留；不作为旧生产反例。

本修正不调用任何引擎函数、不写进程、不授予物理／碰撞／姿态／世界同步许可。选中8段code并不等于整个DLL的当前代码都被认证，身体映射、实际碰撞消费者及唯一Native写入闭环仍需后续工作。

### 下一候选与历史证据复用

r44 prepared `dab63424ea36cf92a68d3951697019c10666b2d34b7536338a51d32494dfebe2`，plan `6d678d6453f651e7c07b34a30a00b1fbd6c1315eb99820f2546824ba33d40b88`。路径分别为`analysis/solo-m1-20261028/equipment-controls-prepared-20261109-cover-bound-source-r44/prepared.json`和`cover-bound-source-runtime-plan.json`，入口`run_cover_bound_source_check.py`。原r13 Native5cf4adea、PGf8e6f3c2、XR16840b27未重建；24声明源／3保存制品／36当前驱动SHA与AST及prepare-only0已核，仍不将24声明推广为整棵r13源码一致性。

下一运行复用原r39的目标：一次原A吸附后，在同一关闭查询内观察当前身体、内部接收者及原形状；有首个明确失败即停止，不复跑旧暂停B，不增加Trace／getter／虚调用或身体写入。申请已送往当时真实队尾Snub144724，尚无新槽交窗、未stage／run；当前HB144203→Frag145754→Snub144724原队列保持。

r43离线查到的Actor.Trace→0xcf8f8→0xcfa40→World+0x178／slot32→0xcfef0→0xe6a10→组件slot0x2e8，与原2026-10-29实施包已有证据相同，不算新发现。原包还保存基础SkeletalMeshComponent构造表、0x339294、骨骼矩阵getter及叶级索引几何的更深解析，应直接复用。今天的历史区段数组结构检查不能升级成当前注册unwind或ABI证明，伪代码RIP常量仍须以实际指令核对。后续重点是实际玩家分支与同版bounds／索引／物理消费，而非重做已有静态链。

完整CV-01–24／AC-01–22、全身体积路径与占用、实际Native移动及恢复、同姿态命中／枪口、严重吸附遮挡和实体头显仍未完成。公开v71、固定11／64及36.47%保持。


## 2026年11月9日 r45原版身体命中过滤与骨骼来源接线

本轮沿原实施包尚未解析的0x131e80分支继续，复用历史原件911ab9bc及只读知识库，没有游戏／XR／引擎调用。原0x339294可见分支从mesh+0x248取PhysicsAsset，令R8=mesh并调用0x131e80；该函数主区1535字节、冷区361字节，以及下游0x130e7c的4098字节，均按原件逐段验SHA并完整线性解码。保存区段数组与这些边界一致；该数组仍不代表当前注册unwind。0x339294另有已知共享尾未纳入本轮提取，不将三段KB区段宣称完整依赖闭包。

实际指令将PhysicsAsset+0x68／+0x70作为BodySetup数组与数量，读取条目+0xbc的过滤字。零体积判别来自既有0xdb120；对应两条分支检查掩码4和8，再从BodySetup+0xb4读取完整FName。后续使用mesh+0x218的骨架asset，按asset+0xb8／+0xc8／+0xd0缓存和16字节条目匹配FName的index与number；这与r41现有engine骨名缓存读取合同一致，不另造第二套骨名表。

无MasterPose时，查询从mesh+0x26c读取SpaceBases、+0x274检查数量，索引乘64后取矩阵，并与mesh+0xa0至+0xd0的四行共同计算。MasterPose有另外分支，无效索引存在原identity fallback，现有严格读取不能据此接受缺骨。随后原函数会归一化矩阵的轴，并传递单独计算的缩放；不能将未经同版验证的裸SpaceBases直接当成最终查询形状。原调用把BodySetup+0x6c与该变换传入0x130e7c；后者可见四组数组的遍历，前三组步长80、末组124。数组／结构仍以当前反射认证为准，不能把原纯字段fixture的紧凑尺寸当成原生布局通过。

这提供了另一条实际命中数据来源：身体设置、骨名缓存、SpaceBases及组件变换参与原版形状查询。它不允许把所有命中消费者统一归于PhysX刚体pose，也不证明当前玩家选择了该分支、派生组件未覆盖slot、所有下游助手已闭合，或当前broadphase bounds／查询索引已消费同一姿态。物理世界与实际身体占用要求继续保留。

### 新增只读适配器

新增`tools/game-playtest/cover_hit_source.py::read_setup_filters`，供已有当前BodySetup／mesh关联的调用者在同一CoverQuery中使用。它以实时反射核对RB_BodySetup的`bBlockZeroExtent`与`bBlockNonZeroExtent`：必须认证当前继承链中的实际声明者、BoolProperty、偏移0xbc／宽4／掩码4与8，不从脚本默认值猜声明类；读取完整共享字并要求调用者关闭查询。关闭或缺失标志分别处理；不能用形状的`bNoRBCollision`代替这两项查询过滤。输出的当前route代码、实际选择、命中几何、物理／世界同步及activation标志全部保持false。

6个实际读取器测试方法、16组情形通过：四种开关组合和其他位、错误偏移／宽度／掩码、缺项、关闭时共享字或元数据变化、真实默认对象、身份替换及父类声明字段。全部使用生产读取器与既有合成反射内存，无进程、无写入、无引擎调用。代码中两段历史FILTER_CODE哈希重新对原件核验，22处关键指令锚点一致。证据保存于`analysis/cover-system-20261108/hit-physics-route-r45/verification.json`、`consumer-code.json`及`reader-tests-final2.log`。该模块尚未接入或运行，不改变r44的36个冻结驱动与候选。

### 当前运行队列与下一步

r44已由真实尾Bol153322唯一受理，前序为Frag145754→Snub144724→Bol153322→Coverr44；需其最终全终交窗后再独立idle／全SHA核验。按到达顺序唯一后继已登记并获确认：Lancer stock-reference040742，prepared4401a0a3／planca9bf3b6／inputs c78b9182，1354归档声明／855编译源／6保存制品／22当前驱动和AST完整核对。它仍只有排队受理，没有授窗；后到HB有限供弹请求已转Lancer这个实际尾，r44不登记第二后继。

后续仍先执行已冻结r44，取得当前身体／原形状／内部接收者来源。原命中分支的字段核对随后与当前组件slot、骨名映射、同版SpaceBases／缩放和bounds／索引一起接线，不因本轮静态或CPU结果放行抓边、身体移动或受击保护修改。完整CV-01–24／AC-01–22、Native移动与恢复、碰撞与占用、同版命中枪口、吸附遮挡和实体头显保持未完成。


## 2026年11月9日 r44b同一身体查询接入命中过滤来源

在r45只读字段适配器基础上，`cover_physics_state.capture`增加默认关闭的`include_hit_filters`选项。选择后从同一完整历史原件核两段0x131e80主／冷代码，并由已有模块范围／原字节校验在当前游戏中复核；之后在当前关联的每个BodySetup读取过滤字段。代码、元数据和共享过滤字全部纳入原CoverQuery关闭复读，原Native封存入口、人物／世界／掩体关联和关闭检查保持。

`FilterReader`仅在同一查询内按当前setup class缓存已认证字段布局；每个setup的共享字仍独立读取并记入关闭证据。缓存不得跨查询使用，不隐去字段或指针变化。未选择新选项时旧来源路径不增加读取。字段及原消费者代码对应不等于当前玩家已经选择该几何分支，所有hit geometry／物理／姿态／世界同步和activation许可继续为false。

53个相关测试方法通过，包括原PhysX、物理状态、原形状及11个hit来源方法。新增覆盖当前完整filter code尾字节错误、结束复读时code／body word变化、布局缓存的查询归属、多个setup独立字和64体联合读取。64体／四类原形状／PhysX内部来源／hit过滤同一次合成查询15468reads、179291字节，保持原16384reads／2MiB／3秒限制，不放宽预算。实际游戏尚未运行该扩展，合成预算不代表零散射线或完整身体扫掠已通过。日志与预算在`analysis/cover-system-20261108/hit-physics-route-r45/integrated-affected-tests-final.log`和`integrated-budget.json`。

### 同一未运行槽替换

旧r44 prepared dab63424／plan6d678d64未stage／run，现明确撤用保存。新r44b prepared `4b42fb1f1c0c1b46c1a31f6f0f8e2b39e0c1f71fa5222334d7d84a3b6beec7aa`，plan `37ad70f335ab54e43793bcdea2645d278ea5adf60932158c34a03a2f8cae5545`；路径为`analysis/solo-m1-20261028/equipment-controls-prepared-20261109-cover-bound-source-r44b/prepared.json`及`cover-bound-source-filters-runtime-plan.json`，入口`run_cover_bound_filters_check.py`。原3份Native／PG／XR制品不变；24声明源、37当前驱动SHA／AST及prepare-only0通过。原A一次、原source／shapes读取和现有启动／清理／录制保护保持，没有Trace／getter／虚调用、身体写入或额外游戏输入，不复跑暂停B。

已把完整冻结身份向唯一前驱Bol153322申请替换同一槽，并通知唯一后继Lancer stock-reference，均未新增运行位置。前驱旧r44末尾核验恰好捕获boundary_chain和physics_state的两项有意变化，原拒绝保存；已说明只允许重新核验新r44b，不能按旧身份授窗。此时前驱game8996已进入全终收尾，仍须其新候选核验和最终唯一交窗后才独立idle／hash并stage，不把本节当成授窗。后到Longshot160159申请已沿Lancer实际尾转交，不在此处登记第二后继。

下一实际动作仍是同一生产路径：原版吸附一次后取得当前身体／原形状／内部接收者及过滤来源。完整的连续身体体积扫掠、占用、实际Native移动与恢复、同姿态命中／枪口和最终画面／头显不因本轮source接线通过而关闭。


## 2026年11月9日 r44b实际59528及原Aggregate RenderInfo修正

### 实际结果与完整收尾

收到Bol153322/game8996的最终唯一交窗后，独立CIM／idle、24原r13声明源／3保存制品／37驱动SHA与AST、所选游戏／PhysX／解释器全核通过，仅stage新r44b（备份prior-private-text-20261109-161307），旧r44未运行且撤用。源码检查点1035ccff先于本次运行保存。

实际game59528／creation134386856117449624，owner69380。原A完成CT2／CA0，查询继续越过此前PhysX头比较和原code入口，读到第一个BodySetup及两项原命中过滤字段，随后在`KAggregateGeom`结构完整字段集检查处首次拒绝。2741reads／35041字节，267.1848ms；operator2912 exit1，完整查询没有关闭成功，全部hit／physics／pose／world同步许可false。首条目只具本次定位意义，不宣称整个身体来源或当前过滤分支通过；内部接收者及形状尚未完成读取。没有Trace、getter、虚方法或身体写入。

完整1页双路总览及6张关键原图已亲审。364张游戏／464张模拟器约21.83秒，末端双路覆盖通过；最大采集缺口游戏2116.6878ms（启动段0.9000138–3.0167016s）、模拟器316.6649ms，不作FPS证明。吸附后镜像和两眼均仍有严重肩部／身体／装备遮挡，保留FAIL；startup第三人称和等待卡保留。视频21.866667秒／5120234字节。

wrapper45383 exit0、encode11472 exit0、唯一retention35173 exit0均终结；Native／entry／pad／drain／XR0／launcher0／gamegone通过。XR终态Exiting、对象销毁与shutdown完成，EndSession成功为false单列。config、settings、head原缺失、两份游戏设置精确恢复；新源码修正前37个ending驱动与3份stage制品无漂移。保留59528／8996／71348及48小时，实删2384媒体／389587758字节、errors0，源／构建／资源／存档／文字数值保留。再次核Lancer stock-reference的1354归档声明／855编译源／6制品／22当前驱动SHA与AST及独立idle后，已最终唯一交窗，不追加旧窗。证据集中在`analysis/cover-system-20261108/hit-filters-combined-r46/`，包括first-failure、closure-proof、selected-visual、ending-driver-check、retention-result及successor检查。

### 原始声明与严格修正

旧读取器将KAggregateGeom完整字段集限定为四个形状数组。只读原包知识库对象索引明确还包含export7083 StructProperty RenderInfo；重新核原Engine.u完整SHA，并使用现有UELib直接反序列化该属性，结果指向`ScriptStruct'Core.Object.Pointer'`。其他四类形状的直接字段也按原包索引核对。原59528未保存当时完整声明列表，所以RenderInfo缺项是有原件依据的修正方向，真实本次差异原因仍未补签。

读取器现严格要求原五字段；RenderInfo必须为当前反射的8字节Pointer，只记录原字节和数值，绝不解引用或作为几何／活动缓存证明。缺项、未知字段、错误类型仍拒绝。新增ShapeLayoutRejected在失败时携带已经读到的结构身份、size、预期字段名、声明者与完整字段身份列表，不新增查询、不忽略字段；Physics capture复用原异常见证通道保存。另在查询开始保存原lifecycle和已经读到的PhysX image-at-start，明确close尚未完成，后续失败不再丢失该阶段证据。

旧生产读取器对原包五字段fixture复现同样的KAggregateGeom拒绝；新读取器闭合通过、710reads／9050字节，只是合成重放。56项相关检查通过，新增覆盖RenderInfo不解引用、缺／多字段／错误target的精确拒绝见证和关闭时指针变化。64体联合15668reads／180699字节，原16384／2MiB／3秒预算不变。原始声明、解析器源码／结果、字段索引、预算及旧新重放保存于`analysis/cover-system-20261108/shape-layout-r47/`；测试日志仍按实际文件保存在r46的shape-render-info-final-tests.log。

### 下一候选与抓边姿势要求

新r47 prepared `09720d6cd02bd1e467f9b20a819df9d83778974da38ae6c752baa46ef9f66f02`，plan `83e977197a0d5082f823bcea031d30e8ed8ef9cc27a32c205a23f535d578368d`；入口`analysis/solo-m1-20261028/run_cover_shape_layout_check.py`。原Native5cf4adea／PGf8e6f3c2／XR16840b27未重建。24声明源／3保存制品／37当前驱动SHA与AST及prepare-only0通过，仍仅一次原A及原联合只读来源，不增加调用、写入或游戏输入。已向当前实际Lancer尾路由申请，排在全部更早申请后；尚未受理／stage／run，无运行权。

按主人最新公共合同，需求CV-14及AC-05／12补充每个实际抓边点的独立GrabPose、Snap与Grab权限边界、稳定可退出、不抢手，以及掌心／拇指／四指／食指／腕／袖口和接近至释放过渡的实际双眼检查。保留全部CV-01–24／AC-01–22编号；这是新增明确验收范围，未将其标为已实现或画面通过。继续复用唯一C0写入与公共仲裁，不加writer／Hook，不运行无关武器候选。

## 2026年11月9日 r48实际抓边点姿态规划层

新增 `tools/vr-common/cover_grab_pose.h` 和独立检查程序。规划层接收当前实际边缘的世界坐标、逐点 authored 腕部及15关节姿态、原始手部样本与公共左手归属，区分接近、真实抓稳和释放。输出只供既有C0求解器后续接线，不创建新writer、Hook、武器Kind、租约或身体移动。

- 来源键绑定world／pawn／cover／slot／session／space／reference、边缘、组件、形状代次和姿态profile；调用者仍须认证完整原对象、几何与时相，键值相等不替代UObject或碰撞认证。
- 当前接触、掩体和原手部样本均检查原100ms时限，拒绝未来和零时间。真实抓稳须匹配当前非零grant、grabbing阶段及同一边缘；真实释放即撤销本规划，不复活旧目标。
- 被动接近仅限空闲左手、有效范围及未按Grip；已有持物优先。真实抓稳不再依赖新抓取接近标志，但视觉目标仍检查单独的保持范围与腕部转角。当前0.25m配置上界仅是该规划器的防误配限制，不是实际手感标定或身体移动范围。
- 复用Marcus115原绑定及控制器到解剖腕的换算；逐点腕偏移按model scale应用一次，再由当前边缘旋转变换。输出15个local关节旋转，不改变骨长或原输入。失败、none和release均清除上次proposal，释放blend由既有C0消费端承担。

MSVC真实 `/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT` 编译后执行125项检查、0失败，覆盖三类边缘、旋转墙面、scale一次、旧身份／错误grant／释放、接近与保持范围、非法姿态和旧proposal清除。首次缺少显式stereo_frame.h引入导致的编译失败保留，补include后编译0。当前105份编译依赖均未在可执行文件之后修改，全部SHA、原编译／运行日志及两份源码副本保存在 `analysis/cover-system-20261108/grab-pose-r48/verification.json` 及相邻sources目录。

这些检查中的关节数据明确是合成样本。实际每个抓点的authoring、真实表面贴合、接近／释放过渡、C0消费与提交、固定骨长全臂可达、实际镜像／双眼画面和头显均未完成；不能据125项结果宣称GrabPose或掩体玩法通过。没有构建、安装或运行新Native／Playground，公开v71保持。

r47已由Bol162411独立全身份核验后正式单一受理，尚未收到最终交窗。Cover独立核验Bow164457的1965冻结源、219当前及归档驱动、438次Python AST、6保存制品、选中EXE及prepared／plan无错误，登记其为唯一后继；两方均明确受理不是窗口。回执shape-layout-r47/prepared-verification.json及bow-successor-check.json。r47冻结源、3制品和37驱动未被本批新增文件修改，未stage／run，不复用旧59528窗口。

本批为源码与文档进展。完整CV-01–24／AC-01–22、实际全身体积扫掠和占用、当前物理／查询世界同步、Native移动与恢复、同版命中枪口、严重吸附肩／身体遮挡及实体头显仍未完成，目标保持进行中。

## 2026年11月9日 r49抓边姿态接入C0候选缓冲区

新增cover_grab_pose_adapter.h及独立实际编译程序。它直接重新调用r48规划核对本次输入／边缘／归属，检查真实Marcus115完整参考及当前body候选的root／pelvis／head／双脚，再复用原双骨solver将左上臂、前臂、腕及15个逐点local指节旋转追加到已有C0输出缓冲区。目标不可达则拒绝，不像自由手那样把接触点投回臂长球面，也不延长骨骼。只恢复既有左前臂helper旋转；非左臂数据、所有平移及scale保持，任何失败完整输出字节不变。此函数不读游戏、不写Native、不取得新输入或租约；必须由既有唯一C0事务最后统一提交。

MSVC `/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT` 实际编译0、执行1084项检查0失败。使用原Marcus115完整reference、保存的C0-local及component三份几何fixture，以真实SHA构造model；三类边缘分别施加非零世界位移及腕转向，独立四元数／向量FK验证最终腕位置与朝向。逐骨检查平移、scale及非左臂字节保持；错误grant／形状代次／过期手样本／body端点／完整参考／body失败／非法手型及不可达目标均拒绝并保留原输出。第一次1075项亦通过，后续因新增非零位移／转角及独立FK复核才重新编译执行，原日志全部保留。

编译依赖、三份fixture身份、源文件副本、日志及结果保存在analysis/cover-system-20261108/grab-consumer-r49。这里是缓冲区合成接线，不是实际Tick调用、实时对象认证、LOD准入、接近／释放blend、实际逐点手型authoring或画面贴合通过；这些仍需在原C0调用位置接通。未修改共享body_c0_hands、武器姿态发布或冻结r47驱动，没有新Native／Playground构建或游戏运行，未取得前驱最终窗口。完整碰撞／移动恢复／同版命中枪口／严重吸附遮挡及CV-01–24／AC-01–22／实体头显仍未完成。

## 2026年11月9日 r50当前LOD骨骼准入判定

r49缓冲区合成接口现在强制接收调用者本次已认证DesiredBones掩码，不提供默认全骨掩码。完整参考认证及本次抓边规划通过后，逐项检查左上臂／helper／前臂／腕、15指节及其全部骨架祖先；缺任一项、空掩码或超出115骨的置位均拒绝，不将部分手型报告为完整接触。无需的兄弟骨缺失不误拒绝。仍不以骨数、mask或某个LOD编号代替实际皮肤／每Draw或原生对象认证。

严格MSVC编译0，实际执行1198项检查0失败；新增逐骨移除和空／越界掩码案例，保留r49原非零位移／转向、独立FK、固定骨长和失败原输出字节不变检查。所有编译依赖／源码副本／三份原几何fixture身份及日志在analysis/cover-system-20261108/grab-lod-r50。仅修改掩体独立adapter和其检查程序，没有改变共享C0／武器实现或冻结r47源／制品／驱动。

当前Native已从原DesiredBones为既有手部路径生成finger_available；该事实只用于确定本接口应复用的数据来源，不表示掩体已接线。当前LOD快照完整身份和时相、实际C0调用、接近／释放blend、真实逐点authoring及实际双眼仍未完成。前驱Bol尚未最终交窗，本批无游戏／XR／录像／新Native或Playground构建，完整CV／AC及碰撞／移动恢复／命中枪口／吸附遮挡／头显要求保持未完成。

## 2026年11月9日 r51接近与释放过渡接入候选缓冲区

新增cover_grab_transition.h并由cover_grab_pose_adapter的compose_transition复用同一个合成函数。接近／抓稳以当前真实腕为基准渐入逐点目标，释放只保留衰减的腕局部米制偏移／相对旋转及手指local旋转；没有保存或重放旧世界接触点、输入票、租约或身体偏移。身体松手保持仍由原提交账本负责。当前真实腕平移和旋转即时生效，过渡不能改写原追踪；初始渐入／退回时间0.12s仅为工程初值，尚无最终手感验收。

来源键、参考或边缘改变、失追、陈旧样本、时间倒退、超过100ms中断、其他左手归属均清除旧皮肤状态；已有持物不会被释放尾部抢走。释放需要当前真实Grip已松及公共归属空闲；手指尚未退回时即使腕偏移已归零仍继续受限旋转，不能在腕过渡结束时突然张手。调用方提供本次未拟合的C0自由手／手指基准，不允许把上一帧拟合输出反馈为真实输入。只有完整固定骨长／当前LOD／body候选合成成功才接受下一过渡状态；失败清除皮肤历史并保持原body输出字节不变。这仍是待现有唯一Native事务实际调用的buffer层，不是第二写入端。

严格MSVC编译与执行：姿态／过渡182项0失败，候选合成1226项0失败。实际原Marcus115／C0 fixture验证抓稳→松手→退回基准→重抓、非零腕目标、LOD失败不接受过渡状态；独立案例将旧边缘放到1000单位远，同时平移／旋转新真实手，确认释放跟随新真实手而非旧接触点。首次合成有2个释放未结束断言失败，原因是原float参考四元数虽合法但长度略偏1，未归一化点积使完成阈值永远不到；改比较归一化旋转，加入该误差回归后两组均通过，原失败日志保留。

证据analysis/cover-system-20261108/grab-transition-r51，含两程序编译依赖全SHA、源码副本及首败／最终日志。r47的24冻结源／3保存制品／37当前驱动SHA及AST再次一致，仍未交窗、未stage／run；无新Native或Playground构建、无实际双眼或头显通过。接近释放纯计算与候选合成已接通，但真实Tick来源／LOD身份、实际抓点authoring、完整身体碰撞和移动恢复、同版命中枪口、严重吸附遮挡及CV／AC／实体头显范围全部保持未完成。

## 2026年11月9日 r52当前Tick到抓边视觉Frame的数据转换

新增cover_grab_tick_adapter.h及独立程序。接受已获取的原body_tick_source输入、同Tick实际眼点、当前完整CoverPlacement、原CoverControllerResult、掩体身份／归属及原骨架签名，输出r48／r51所需Frame。复用原40ms追踪／50ms元数据分类、原cover_placement_current完整对象／场景／发布／参考及metadata区间核对、原free_hand_eye_targets控制器到腕换算；不添加第二输入读取、按下沿检测或仲裁器，不把转换成功当成Native写入／碰撞许可。

转换精确保留原控制器sequence／received_ns及原始TrackingSpace grip位置；不会将now写成输入时间，也不从渲染手／已拟合腕反算拉拽。Grip状态直接消费既有掩体adapter的滞回／边沿结果，不重新使用武器0.35阈值推断按下。当前世界Grip仅是同源头／手位和本次C0眼点的一次几何转换，后续皮肤修饰不反向改原hand sample。实际参考字段复用原Placement合同：视图reference_generation与Native reference_revision分别管理，不能因数值曾相同而混用。

实际MSVC严格编译0、执行39项检查0失败，包含原时间和原始手位保持、同一眼点换算一致、不同参考域、滞回带中已按下结果保留；完整Link名称、Scene／publication／Tick／参考、原手序号／时间／位置、旧输入／needs_release、错误签名及metadata区间外观察均拒绝并保持原输出字节。首次35项编译／执行亦通过；语义复核发现参考域及阈值重复解释问题后改为复用原predicate，再加入明确不同域／滞回与观察上界案例到39项，原日志保留。

证据analysis/cover-system-20261108/grab-tick-r52。使用保留61492 source-CT0元数据的明确CPU合成CT2／时钟／cohort及注入的既有adapter结果，仅证明转换契约，不是实际掩体Tick或按下沿运行通过。真实owner必须传递同次已认证数据并在失败时中断视觉消费；世界／人物／reference与kernel epoch映射仍由原live owner维护。本批没有接入实际Native调用、构建新Native／Playground或启动游戏；当前r47仍无最终交窗，当前37驱动无漂移。完整身体扫掠／占用／移动恢复、同版命中枪口、实际抓点手型、吸附遮挡及全部CV／AC／头显要求保持未完成。

## 2026年11月9日 r53同Tick抓边候选统一缓冲区入口

cover_grab_tick_adapter新增compose入口，直接串联r52当前输入转换、r51皮肤过渡与r49／50固定骨长／当前DesiredBones候选合成。调用方仍是既有唯一C0 owner，提供本次已解出的自由手／手指body候选及完整当前来源。输入转换失败时清除旧视觉状态并保留body候选字节；后续几何、可达、参考或LOD失败同样不接受新过渡状态。整个入口没有游戏读写、发布、身体提交、输入读取或新broker；alias在任何scratch／状态修改前拒绝。

真实MSVC严格编译0、执行59项检查0失败。集成案例加载61492完整参考／C0几何，以真实SHA准备model及原头部authored眼点；明确Host反算一个可达腕对应的原XR输入，再走完整转换→过渡→求解入口。合法候选通过且骨长误差在0.002单位内，错误完整Link身份、空LOD及错误shape generation分别在不同层拒绝，完整body输出字节保持且旧skin状态清除。保留原39项来源／时间／不同参考域／滞回验证。反算的控制器只是CPU输入生成，不是物理校准、实际按下沿或实际掩体运行证据。

原编译依赖全SHA、fixture身份、两源文件副本、编译／运行日志在analysis/cover-system-20261108/grab-entry-r53。该统一入口已连接缓冲区步骤，但尚未由实际Native Tick调用；实际owner的对象／几何／LOD身份与提交许可仍须在原边界认证。没有新Native／Playground构建、安装或游戏运行。Bol已在其11108动作段运行，但尚未最终交窗，Coverr47未stage／run。完整身体碰撞／占用／恢复、同版命中枪口、实际逐点手型、严重吸附遮挡及全部CV／AC／头显要求继续未完成。

## 2026年11月9日 r47实际26028首body数组声明拒绝及全终交接

Bol162411-v3实际11108全终、精确恢复、审片、编码与保留结束后明确唯一交窗，本轮重新完整核r47 prepared09720d6c／plan83e97719、24冻结源、3saved制品、37当前驱动SHA与AST、选中EXE／PhysX／审图解释器及Pillow／numpy，独立idle通过，并保存5配置／设置／head原缺失基线。首次stage调用错误使用Python -I，导入同目录retain_test_media前即退出，未部署；保持原脚本不变、改回已有-B调用后stage0，三安装制品再次SHA匹配。原失败日志保留。

实际game26028 creation134386910322425789，owner13248 creation134386910235770519，session5943137e。原A一次进入CT2／CA0，原只读几何与来源观察继续。第一个body为b_MF_Pelvis、FName27549／0；原hit过滤字4c000000，bBlockZeroExtent／bBlockNonZeroExtent仍为同0xbc字的mask4／8。这些只是未关闭的局部字段证据，不是受击／碰撞许可。

本次已越过KAggregateGeom完整5字段与RenderInfo原8字节Pointer判定，随后严格拒绝Physics shape struct array target/extent rejected。2778读、35529字节、466.5335ms；整体acceptedfalse，1个body部分结果，不代表29body／完整shapes或内部接收者已完成。新增source_at_start及physx_image_at_start实际保留：独立枚举base140720153100288与头ImageBase相等、完整绑定头匹配，8段当前已分析代码已读取，methods_calledfalse且closing_requiredtrue；不能把at-start代码／头观察当整查询关闭、活动形状／当前物理姿态或世界同步通过。数组失败路径未保存其已读inner／target／stride／extent细节，本局结束后不能补读旧地址或断言究竟哪项不一致。下一修复应保留这些已读拒绝见证，不放宽相等式或猜测对齐补丁，不重复未变候选。

wrapper工具90227实际terminal0；operator7456 exit1、error字段null且实际动作失败保留。Native清理、入口及pad恢复true，fire_disable0／hand_stop5／drainedtrue，XR0／launcher0，gamegone。EndSessionfalse与terminalExiting／objectsDestroyed／shutdownCompletedtrue分别记录。原wrapper finally曾一度idlefalse而保留输入备份，独立game／XR终态后核对5基线：config、head原缺失和两gameSettings均原字节；simulator settings语义相同而字节格式不同，原after-wrapper副本保存后恢复启动前原字节，最终5项精确一致。此收尾时相记录不等同Native未关闭，也不隐去原日志。

完整1页paired overview及6张原图已亲审：game00424／00439／00443、simulator00610／00626／00630，覆盖吸附、形状拒绝与收尾。镜像及两XR眼睛仍严重右肩／身体／装备近裁剪遮挡，视觉FAIL；startup第三人称／等待卡仍在。游戏446帧29.6663362s，sim633帧29.6496693s，双路末端覆盖通过。游戏最大捕获缺口3066.6216ms，位于启动1.7499975–4.8166191s；sim最大366.6595ms，不把WGC间隔或窗口FPS当渲染／头显证明。编码90694实际terminal0，视频29.7秒／5372121字节。

结束37驱动SHA／AST及3安装制品在后续源码改动前匹配。保留工具6347及实际python44892进程曾确认仍活跃，仅轮询同一作业、不重启，最终terminal0；实际删除2308媒体／352804738字节、errors0，保留26028／11108／31420最新3局及48小时，源文／数值记录保留。独立核Bow164457的preparedff9504ac／plan9d86138c、1965冻结源、219当前及归档驱动、438AST、6saved制品和选中EXE无错误，并再次idle后已最终明确唯一交该后继。当前不占窗、不补第二局；后到请求均沿Bow真实尾转达，不在Cover登记第二继任。

证据analysis/cover-system-20261108/shape-layout-r47/runtime，包含before-runtime、两个stage日志、wrapper原日志、game-26028-first-failure、ending-driver-check、settings-restoration、selected-visual、closure-proof、retention-result及runtime-jobs。r48–53独立新C++文件不在本次冻结Native内，不能把本局当它们的运行验证。完整CV-01–24／AC-01–22、真实抓边／身体碰撞与占用／Native移动恢复／同版hit与muzzle、严重吸附遮挡及实体头显仍未完成；公开v71及原历史计分保持。

## 2026年11月9日 r54形状数组拒绝保留已有声明见证

26028已全终交Bow，Bow已直接确认接收并独立完整预检；当前Cover不占窗。独立检查确认cover_physics_shapes.py／cover_physics_state.py及形状检查程序均不在Bow164457的219个冻结Root驱动内，本批只改掩体shape reader与其离线检查，不改变后继当前候选。

针对实际target／extent错误，原array函数现在在同一拒绝处抛现有ShapeLayoutRejected，携带已经读取的ArrayProperty、原header、data／count／capacity、inner完整身份及0x28原metadata、dimension／stride、期望结构名、target完整身份，以及实际已读到的target kind／extent。原名称／number不符时仍不查询class或extent，class不符时仍不查extent；未读取值明确None及unknown，不新增进程读取。原target名称、ScriptStruct类型和stride==extent条件、预算与关闭规则全保持，返回仍拒绝／source_acceptedfalse／activationfalse。已有combined capture原catch会保存source_rejection_witness，无新观察器或Engine调用。

实际Python shape14项通过，其中新增名称／编号／class／extent四分支的见证和短路读取检查，以及同query combined失败保留；shape与physics source组合26项通过。与r47归档旧reader在五个独立合成fixture逐项运行比较：合法4shape均710读／9050字节，stride失败均48／632，名称／编号／class失败均47／628，拒绝文本和决定完全相同；仅新失败额外保留本地已有见证。比较与日志在analysis/cover-system-20261108/array-witness-r54。实际26028没有这些已丢失元数据，不能用合成fixture推断它的具体原因或宣布几何修复；尚未新game验证，完整CV／AC／碰撞与恢复／同版hit与muzzle／吸附遮挡／头显保持未完成。

## 2026年11月9日 r55原包Inner声明与原消费者步长取证

继续复用已有原包及历史原指令，不开启游戏、不新建反汇编数据库。REA本会话binary_session显示无活动目标，限定0x140130e7c的retained证据查询为空；这不否定本地已有历史chunk，随后直接核保存源与chunk完整SHA。没有把未建立的REA记录当作新反编译结果。当前包Engine.u完整SHA95b0c8da…e90510，历史原代码section完整SHA911ab9bc…06ae6，函数0x140130e7c的4098字节chunk SHA5f4786ad…63a77均验证。

只读UELib helper实际编译／运行0，保留CS0618旧Deserialize API告警及原stdout。原包四个数组7087／7086／7085／7084依次指向inner7079／7080／7081／7082，ArrayDim均1，目标分别为ScriptStruct KSphereElem7053／KBoxElem7059／KSphylElem7064／KConvexElem7078，均归属Engine.KMeshProps。解析器在这四个inner上报告ElementSize0；这只是该序列化解析路径的返回值，不能据此给当前native stride或PropertySize填0，更不能将其用作实际数组读取步长。

原机器码在0x1401311ea／0x140131448／0x140131d1c分别add元素指针0x50，在0x1401318fc为add0x7c，紧随的count引用位移分别0x28／0x18／0x08／0x38。记录保留原bytes、上下文及源SHA；静态指针推进量为80／80／80／124。将这些循环对应到具名Aggregate数组仍是依据原源布局的推断，不证明26028当前反射stride、结构extent、对齐、分支执行或物理消费完成。

因此当前只得到两个不同来源的事实：原包目标声明正确，原消费者有固定指针步长；尚没有26028拒绝的原inner stride／target extent，不能把对齐补齐当作已证实原因，不能据此放宽stride==extent或忽略字段。下一实际来源仍由r54失败见证获得。证据analysis/cover-system-20261108/array-layout-static-r55，包括只读helper源码／exe／库身份、原声明／压缩摘要、原指令锚点及verification。没有改r54 prepared6ec774ef／plan915215be、Native／PG／XR或其他候选；当前不占运行窗，完整CV／AC与碰撞／移动恢复／hit／muzzle／吸附遮挡／实体头显仍未完成。

## 2026年11月9日 r56原primitive消费者尺寸与浮点顺序

检查确认当前Pawn圆柱原始字段仍不能替代最终探身body体积；为后续逐body几何接线，复用已有静态原代码进一步提取0x1400ce24c／0x1401fe65c／0x1400cf080三个直接消费者。原KB伪代码SHA与原section SHA验证，完整机器码分别281／1713／1663字节全部线性解码。Sphere消费者在0x1400ce33e以scalar乘shape+0x40；Box在0x1401fe7f3先以scalar乘原常量，再在7ff／804／809乘shape+0x40／44／48；Sphyl在0x1400cf25e乘同常量、266乘shape+0x44，33d以scalar乘shape+0x40，并有两次原0x1400cf700及一次0x1400d7b68调用。

常量必须按原机器码RIP重定位及同一保存数据段读取。原section_131b000.bin完整SHA c5deb081…5231c验证，两个RIP目标均为0x1417070e4，原bytes0000003f即float32 0.5。首次根据伪代码显示猜为0x1417060e4的断言失败，原尝试及正确计算均保存；地址显示差异原因未认定，不全局挪动RVA或把它当当前array拒绝原因。Sphyl伪代码仅1631文本字节且看似return1，原1663机器字节实际有3次调用，不能凭伪代码片段宣称该查询无效或无命中。

新增cover_primitive_metrics.h纯float32计算：调用者首先认证当前kind／反射字段和该原消费者scalar后，Sphere计算Radius×S，Box半尺寸计算(S×0.5)×X/Y/Z，Sphyl计算Radius×S和(S×0.5)×Length。保持原游戏单位及原乘法顺序；不从矩阵猜scalar、不换米、不产生世界形状或许可，Convex明确unsupported，完整body消费者必须另行处理，不能默默遗漏。非法尺寸、非有限值、溢出／有损归零拒绝且旧输出字节保持。

真实MSVC严格编译0、执行39项检查0失败，包括原尺寸例、零Length、不同shape输出清理、非法输入、unsupported Convex及float32极值。特别以原SSE乘法顺序的IEEE754黄金位模式7f3fffff检验最大float scalar与1.5尺寸，避免先乘尺寸造成非原版溢出；极小值归零拒绝。证据analysis/cover-system-20261108/primitive-semantics-r56含完整原chunks、伪代码、原常量／指令锚点、源副本、编译依赖SHA及日志。

该计算还未接入当前native字段／pose／physics消费者；incoming scalar完整语义、非均匀缩放、Convex、连续扫掠／占用及当前场景可见性仍未知。本批无新Native／PG构建、游戏或头显通过；r54 prepared6ec774ef／plan915215be保持不变。Bow已确认r54沿其全终交窗后的唯一后继Lancer165800单次转达，晚到Bol180947排r54之后；当前向Lancer查询实际尾以直接核接同一申请，Cover尚无新窗口／未stage。完整CV／AC、实际移动恢复、hit／muzzle、逐点手型／吸附遮挡及头显继续未完成。
## 2026年11月9日 r57原primitive与当前骨架的世界空间转换

新增cover_primitive_world.h，复用r40完整FName骨骼映射及r56原primitive尺寸计算，将调用者已认证的形状局部刚性位姿合成到当前Marcus115／C0骨架世界位姿。每次重建并逐项核对整个映射，包括完整名称／number、骨架父级和最近命名body祖先；不能凭旧bone_index继续使用。输入、模型、Frame、Mapping、工作区和输出不得重叠，任何失败均保持旧输出字节不变。

当前范围明确为单位缩放。原model与所有C0 atom scale必须为1，component须为无反射、无非均匀缩放的刚性矩阵；有限浮点旋转容差复用现有数学边界。FK仅归一化合法旋转以避免链中数值缩放累积，使用原translation，不拟合或改变骨长。Sphere／Box输出世界形状位姿及原游戏单位尺寸；Sphyl额外输出世界段轴与两端点。调用者须明确认证shape-space轴，代码不猜+X／+Z；原scalar是否影响局部平移也必须在输入转换前认证，不能将尺寸scalar再次乘到已转换局部位置。Convex明确unsupported，整body消费者必须停止而非漏掉该体积。

实际严格MSVC /std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译0，独立程序以BelowNormal运行，760项检查0失败。使用保留Marcus115参考／C0数据、打乱的合成body名称顺序及合成primitive，以独立四元数／向量FK核对8个骨骼、3类形状、原始及旋转平移后的身体；验证中心、朝向、球柱轴／端点、尺寸只缩放一次、零Length和非零FName number。错误映射／参考／缩放／矩阵／轴／姿态／尺寸／溢出／alias均拒绝且原输出保持。证据analysis/cover-system-20261108/primitive-world-r57保存源码副本、原编译与运行日志、221个实际编译依赖SHA、二进制与3个fixture全SHA。

该入口目前只准备一个形状的owned描述，不是活动PhysX姿态、完整body体积、连续扫掠、终点占用或命中证书；实际当前shape字段／scalar／局部TM和轴语义、同源关闭、physics消费及查询世界同步仍须认证。没有新Native／PG构建或游戏／XR运行。r54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品重新全核无误。Bol直接指出Snub174315为无后继的实际尾后，已向Snub递交同一较早Coverr54完整冻结申请，未据此宣称已受理或已交窗。完整CV-01–24／AC-01–22、Native移动恢复、同版hit／muzzle、实际逐点抓握、严重吸附肩／身体／装备遮挡和实体头显继续未完成。

## 2026年11月9日 r58原局部TM缩放与球柱轴接入

沿原aggregate 0x140130e7c到球柱0x1400cf080追踪寄存器和完整指令。REA对该球柱入口没有保留记录，本轮不新开数据库；复用已保存原代码，重新校验原code／data段全SHA，两个chunk逐字节匹配并完整解码，共5761原始字节。球柱伪代码只保留过短片段，不能据其return1否定真实路径。聚合端把形状TM的+0x30／34／38平移分别乘传入scale.xyz，再作形状TM×父变换；传入primitive的xmm3取scale.x。球柱原消费者将查询点减平移后与矩阵三行点乘，局部第三轴与正负半Length比较，两次球查询中心分别为(0,0,+halfLength)和(0,0,-halfLength)。因此该原路径的球柱段轴为形状局部Z，不采用PhysX胶囊常见轴的外部猜测。

原RIP相对地址重新计算并读取同一保留数据段：0x1400cf186取0x141707080的00000080符号位，0x1400cf25e取0x1417070e4的0000003f半值；对应负端与半段长。输入scale向量生产者的完整语义、近似均匀／非均匀路径及当前native字段对应尚未认证，不将该静态消费者结论升级为当前活动物理形状或查询同步证明。

在既有cover_primitive_world.h增加Authored输入及prepare_authored／convert_authored入口，复用同一FName映射、r56尺寸与r57当前C0转换，不新增writer／读取／Hook。仅接受明确正值、三个分量完全相同的receiver scale与刚性TM；非均匀或近似均匀输入明确unsupported，Convex同样不能跳过。按原float32顺序先计算TM平移乘scale，再形成局部pose；仅在该已知原路径解码时设置球柱Z轴。通用显式axis入口继续保留。非法矩阵、尺寸、缩放、溢出／正值归零及alias均保持旧输出字节；未认证current source的调用方仍无运行许可。

实际严格MSVC编译0，以BelowNormal执行1088项检查0失败，保留此前760项。新增原TM旋转、三档scale、三类形状与保留C0的联合转换，独立四元数／向量FK核世界中心／朝向和旋转后的Z轴端点；0.1f×3以原float32位模式3e99999a验证平移先舍入，不把double乘积直接送到世界变换。完整证据analysis/cover-system-20261108/primitive-transform-r58保存原机器字节关系／常量、可重现只读提取器、两源副本、221编译依赖SHA及编译／运行日志。

运行队列已由Snub174315直接完整受理同一Coverr54为其唯一后继，尚未最终交窗；本轮未stage、未运行游戏／XR，未构建新Native／PG。Coverr54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品重新核对一致。后到同一Bol180947已独立全核Native1811+PG1847冻结源、24当前+24归档驱动、74support、122次AST、4saved制品／精确入口与EXE，并正式受理为Cover唯一后继；41未解析import和静态闭包边界明确保留，不授运行窗。首次本地核对错误使用游戏身份path键已记录，改按实际game_path／exe_sha256后全核通过。完整CV／AC、源关闭、连续身体扫掠／终点占用／Native移动恢复、同版hit／muzzle、逐点手型、严重吸附肩／身体／装备遮挡与实体头显均保持未完成。

## 2026年11月9日 r59完整形状清单与同一C0姿态转换

实际接线检查仍见body_c0_goal_policy_live在掩体分支只sample_current_cover_pose后拒绝写入，运动Backend仍UnavailableBackend。本批不删除这些准入检查。r57／58只转换单个形状，不能证明一个完整body清单未漏项；后续身体扫掠必须先具备同一姿态下完整、可核对的几何输入，不能把若干成功单项组合称作完整体积。

新增cover_body_geometry.h，输入为调用者已经关闭并认证的逐body四类数组count，以及按body／kind／原array index排序的Authored记录。body数必须等于当前完整FName映射；逐数组count总和、kind、body index与原序号必须全部吻合，缺项、重复序号、交换、超容量与整份空清单均拒绝，无截断成功。声明含Convex时即使调用者遗漏它，也明确unsupported；不静默移除体积。该有界工作区可容纳256个primitive，超过容量保持拒绝，不声称已覆盖未知当前资产。

抽出r57同一骨架准备步骤供单项与整份转换复用。整份清单只核一次完整模型／引用／FName映射并重建一次当前C0骨架，各形状仍复用r58原float32局部TM缩放与局部Z球柱解码。所有body及形状共用此姿态；第一个到最后一个形状全部成功后才整体替换公开owned输出。最后一项失败同样保持旧结果全部字节不变。新清单比旧结果小时清除尾部旧形状；大工作区由调用者放heap／TLS，不在原游戏栈建立整份临时Geometry。这里只输出几何描述，没有活动物理、占用或执行许可。

首次严格MSVC编译因验证程序新局部before遮蔽旧同名变量而被/WX拒绝，保留失败记录及原验证源码；仅修正变量名后编译0。BelowNormal实际执行1889项检查0失败，保留此前1088项。整份案例采用保留Marcus115参考／C0、8个打乱选择的合成body名称及53个合成形状，以独立四元数／向量FK逐项核位置、朝向、球柱端点和原序号；这些数量不是新游戏观测。另验证最大256项、完整输出alias、漏项／重复／错序、最后一项名称／矩阵／尺寸错误、unsupported Convex、整数超容量以及256项缩至1项后的旧尾清除。证据analysis/cover-system-20261108/body-geometry-r59含三源副本、222编译依赖SHA、fixture与二进制SHA及真实编译／执行日志。

当前清单仍需由实际同源关闭读取消费，未接Native Tick或原版碰撞查询，不以此关闭连续路径扫掠、终点占用、实际移动／恢复、同版hit／muzzle或严重吸附肩／身体遮挡。Coverr54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品再次匹配，尚未收到Snub最终交窗；唯一已受理后继仍Bol180947，未最终交它。本轮无新Native／PG构建、部署、游戏／XR或头显通过。完整CV-01–24／AC-01–22及逐点手型／物理头显验收保持未完成。

## 2026年11月9日 r60世界查询原码后续段与同查询代码观察

沿原Actor.Trace包装器→0x1400cf8f8→0x1400cfa40复核非零体积查询接续点。r43旧记录已明确shared-tail未解，实际只保存0xcfa40–0xcfd58共792字节；其中多个直接分支去0xcfdb3，另有fallthrough进入0xcfd58，均不在旧片段内。本批核原section全SHA后，从既有原始字节补读到0xcfea9，新增337字节、该完整选定段共1129字节；完整线性解码，旧792字节逐项一致，选定段内所有直接跳转均闭合，末端为与原序言对应的恢复及ret。历史结构化range数组只作佐证，不称当前注册unwind或完整调用图证明，不覆盖被调用函数及动态目标。

新增后续段保留原call rsp+0x20的R15、rsp+0x28的R14D及rsp+0x38的来源参数，虚分派字节偏移0x20即十进制32，与原hash表记录的slot32一致；当前receiver／vtable关联尚未认证。返回链表按0x60字节记录复制，存在64项计数上限，随后有array／count／stride／comparator形式的间接调用和最终复制连链返回。“排序”是依据该参数形式的推断，其导入目标未认证。不能从参数转发就宣布初始重叠、自身排除、动态Actor或碰撞世界同步已通过。

新增独立只读模块cover_world_query_code.py，针对已认证当前module与同一CoverQuery检查已知包装器路径5段共3550原字节，包含上述337字节缺口。所有读取走原q.read、执行区间和image身份保护；调用方仍必须关闭同一query。当前码不符时只保留已有read的rva／范围／expected及actual SHA见证，不新增补读。没有原方法／虚函数调用、输入／body写入或第二观察器；返回明确closing_required且dynamic_dispatch／initial_overlap／sweep／worldsync／activation均false。该模块尚未加入任何运行driver，特别未改变待运行r54范围及其原读取数。

实际Python 8项检查通过、0失败、0跳过，以保留原字节和真实CoverQuery逻辑配合合成进程内存验证：每段不符均拒绝；原缺口0xcfd77处变更拒绝；观察后、关闭前改变后续段亦拒绝；正常5段完整保留在q.values并关闭，错误module／profile／执行属性／范围及坏历史源拒绝。首轮夹具继承了占位aaaa EXE hash而未到目标路径，保留1fail／8error日志和旧验证源码；仅修正夹具身份后全通过，产品image检查未放宽。证据analysis/cover-system-20261108/world-query-continuation-r60包含原完整指令／跳转、可复现提取器、两源副本和首轮／最终日志。

Coverr54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品再次匹配。仍等待Snub174315最终唯一交窗，未stage／run；已受理的唯一后继仍Bol180947，未最终交付。本批没有新Native／PG构建或游戏／XR／头显运行。当前完整形状来源、原查询动态目标及重叠语义、完整身体路径扫掠／终点占用／Native移动恢复、同版hit／muzzle、严重吸附肩／身体遮挡、逐点手型及全部CV-01–24／AC-01–22与实体头显继续未完成。

## 2026年11月9日 r61体积重叠候选与自身排除的原路径约束

继续按原代码核对起点重叠入口，不用虚表习惯次序命名。原构造表十进制slot24指0x1400caec0，实际指令遍历成员、删除匹配项并清容器计数，不能当作PointCheck调用；该候选已排除，未执行。slot40的0x14018e6c0将R8三分量存至状态+0x30、R9存+0x60，构造center±extent包围盒于+0xd8，以原第五参数存flags+0x3c并调用0x1400d0650递归访问。这是中心／体积重叠查询候选的静态依据，实际当前receiver与ABI仍须认证。

原0x1400db120比较三float与零；对有效有限extent，全零标记使visitor选择组件+0x150字的bit17，非零选择bit18，不能只核原Pawn自身的blocking字而忽略实际被查询组件。visitor在候选组件写+0xf4访问代次，并经Actor侧虚表+0x330过滤，第三寄存器参数R8明确为零；不能假定沿用线段查询的来源对象。随后取组件虚表+0x2e0，将当前result局部存储、center、extent与flags交给原方法，返回EAX=0时才分配／加入命中记录，非零则继续遍历。所有角色名称、字段当前反射对应与动态target都未因此认证，尚不授原调用许可。

visitor在加入命中后检查flags&0x200并可提前返回；在这种不完整返回上丢弃自身命中后直接当作空占用是不成立的。原查询还会修改自己的计数及组件访问stamp，不是可任意线程调用的被动读取器。未来接线须分别认证完整／早停结果语义、self排除及GT／世界同步，不能把“start==end的Actor.Trace”默认为同一重叠查询，也不能直接调用推测的slot。

完整保存并复核slot24负例、slot40候选、visitor及零extent分类共1263原始字节，26个关键机器指令锚点一致，原section全SHA911ab9bc…06ae6。slot40实际RIP目标0x141afac20不在此次已有六个保存section范围内，因此root-data值未知；从共享插入／查询代码判断它与空间树根数据有关只是推断，不将其猜成已恢复的回调对象或默认世界边界。限定原包伪代码literal搜索实际工具49106已轮询同一作业至exit0，匹配1400cfef0／140136918；不把原伪代码0x1000显示差异用于全局地址平移。提取与核对程序全终0，未新建IDA数据库或调用原方法。证据analysis/cover-system-20261108/world-overlap-route-r61包含原chunks／伪代码／指令锚点／未知值／各日志及可复现脚本。

本轮是原路径取证，未改产品输入、writer、body策略或待运行driver。Coverr54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品再次一致，尚未收到Snub最终交窗；唯一后继仍Bol180947，未最终交它。本轮无新Native／PG构建、部署、游戏／XR／头显运行。完整CV-01–24／AC-01–22、当前来源关闭、完整扫掠／终点占用、实际Native移动恢复、同版hit／muzzle、严重吸附遮挡及逐点手型仍未完成。

## 2026年11月9日 r62构造表定位实际组件重叠入口与缩放来源

继续追踪r61组件虚表+0x2e0。对原只读段中已知0x140339294指针先作有界字面定位，再按原机器码RIP LEA及紧随mov[this],reg确认实际构造表基址；不能凭连续函数指针段或已知函数猜虚表起点。六个构造来源确认表基址分别14137ac50／b098／b4e0／b928／bd70／c1c0。每表+0x2d8为0x14026eca0，+0x2e0为0x1403a6bc4，而已知339294实际是+0x2e8，下一项50c17c是+0x2f0。最初按已知函数减2d8的推导不成立，未据此调用任何方法。数据段完整SHA c5deb081…5231c、代码段911ab9bc…06ae6、6个原构造及每表已检前缀均保留；它们是历史表，不等于当前实例／类身份已认证。

真正+2e0候选3a6bc4读取component+0x248并进入3a6c34，与已有PhysicsAsset反射合同的静态偏移一致。后者按asset+0x68数组／+0x70数量逐个取setup，传完整BoneName的8字节+0xb4给1d38d4，再调用1d40b8取得组件骨矩阵，经f20f4处理后将setup+0x6c聚合几何交1d0a50。该原路径直接使用authored setup与组件骨变换，不是BodyData活动PhysX姿态读取；不能因此取消当前活动物理／hit／muzzle及空间树同步要求。原循环在骨查找返回-1时跳过该body，无命中本身不能证明完整体积检查；r40完整映射的严格准入继续保留。可选fallback、聚合PointCheck、实际查询时相与当前分派仍待认证。

原3a6c34的缩放生产者亦已定位：每轴先float32计算component+1e8乘+1ec／1f0／1f4；owner+78非空时先以owner+198乘+19c／1a0／1a4，再分别乘上述component结果。新增到既有cover_primitive_metrics.h的PointScale／point_scale只计算调用者已认证的这些值，不读native字段，不从矩阵反推scale；保持原分组顺序。正值有限输入、owner实际存在分支分别验证，原中间乘积溢出或归零均拒绝，不能重排乘法把原已损失的结果救回；owner缺失时不消费未用的owner字段。输出与输入重叠拒绝，任何失败保持旧输出。该生产者不授运行许可，非均匀产物仍由后续原有严格转换范围决定支持与否。

严格MSVC编译0，BelowNormal实际执行73项0失败，保留原39项尺寸检查，新增组件／owner分组、owner缺失、IEEE754位模式3e99999a、重排会改变结果的溢出／归零、非有限输入和alias。源副本、实际编译依赖全SHA、二进制与原指令证据在analysis/cover-system-20261108/world-component-point-r62。原route核对共997机器字节、25关键指令锚点及6个构造表一致；此为静态／纯数学证据，没有原方法执行或新Native／PG构建。

Coverr54 prepared6ec774ef／plan915215be及24冻结源／37当前驱动SHA+AST／3saved制品再次匹配，仍未收到Snub最终交窗，未stage／run。唯一已正式受理后继为Bol180947；更晚HB185428已直接指向该真实尾，Boom191450仅单次沿Bol转达，并明确保留HB及全部更早先序，Cover未另收第二slot。完整CV-01–24／AC-01–22、当前完整形状来源、实际扫掠／终点占用／Native移动恢复、同版hit／muzzle、严重吸附遮挡、逐点手型及实体头显仍未完成。

## 2026年11月9日 r63聚合PointCheck对形状类型与缩放的覆盖限制

沿r62进入原1d0a50聚合PointCheck并核完整hot／cold片段，另核其凸体leaf E20F0，共3294原机器字节，原code／data全SHA一致。选定函数自己的数组循环只使用Box count+0x18及Convex count+0x38。存在可直接证明的无调用路径：有效单位scale、Box／Convex均空时，从原ESI／R12D清零，两个count判断直接到1254的test／sete／mov EAX，返回1；即使Sphere或Sphyl数组非空也不读取和检查它们。此为该路径覆盖限制，不把它泛化为所有world查询，也不要求本来不参与该碰撞的组件参与。

Box入口另外比较float32的abs(Sx-Sy)及abs(Sy-Sz)，均小于原阈值才进入Box循环。按原RIP读取0x1417070ac原bytes17b7d138，即float32约0.0001；不按伪代码字面猜地址。非均匀scale可绕过Box循环，再在Convex为空时直接返回1。因此只取得该路径的Native no-hit，仍不足以证明所有应参与的形状和缩放情形都受检查；不能把省略的类型当作无碰撞。当前Authored转换的严格均匀scale限制继续保持，不因发现原近似容差就扩大。

保存4条静态路径推导：Sphere-only、Sphyl-only、Sphere+Sphyl-only以及非均匀Box-only，均为原无调用分支到miss1的证明，不是运行了原代码，也不是游戏／碰撞测试通过。16个关键机器指令锚点及完整字节由analysis/cover-system-20261108/aggregate-point-coverage-r63/verify_coverage_paths.py实际核对exit0，完整指令、原常量、静态分支及未知范围保存在native-point-coverage-evidence.json。原聚合其他分支及当前可参与组件仍须实际绑定／覆盖认证。

需求CV-16补充明确有效空返回的覆盖条件：应参与碰撞的类型、缩放分支、骨骼或结果不能被静默省略，未知覆盖范围不能签安全／占用。没有新增需求编号、动作键或每次全矩阵门禁，仍保留全部CV-01–24／AC-01–22；这只是落实已有完整体积／路径要求。本轮未改产品writer或查询调用，也未把已定位的PointCheck直接接成可用Backend。

Coverr54 prepared6ec774ef／plan915215be与24冻结源／37当前驱动SHA+AST／3saved制品再次一致，仍未收到Snub最终交窗、未stage／run。唯一正式后继仍Bol180947，其已直接确认后继HB185428，较晚Boom191450由HB实际核接，Cover未加第二位。本轮没有新Native／PG构建、部署或游戏／XR／头显运行。完整来源关闭、覆盖缺口处理、连续身体扫掠／终点占用／Native移动恢复、同版hit／muzzle、严重吸附遮挡、逐点手型与完整CV／AC保持未完成。

## 2026年11月9日 r64球与球柱的有界重叠判断

r63已证实原聚合PointCheck有不检查Sphere／Sphyl就返回miss的路径。本批新增cover_point_overlap.h，对调用者提供的已准备世界Sphere／Sphyl与世界轴对齐查询盒计算窄相距离；不把原no-hit改成安全，也不扩大原查询权限。球采用中心到盒的最近距离，球柱采用线段到盒的分段二次距离；包括线段端点、各区域驻点及平坦最小区间的平面交点。Box／Convex明确unsupported，不能从完整清单静默丢弃。

每次基础double运算向外扩一相邻值，输出最小平方距离的保守上下界，与原float半径的平方界比较。只有下界严格大于半径平方上界才报告分离；重叠须由上界证明；边界精度不足保留indeterminate。有限值、有效半径／轴／端点和查询盒先检查；溢出、次正规或下溢运算保留numeric，不签分离。球柱端点由中心／轴／半段长重新构造区间，避免大坐标下将已舍入端点间的差误当真实长度。输入／输出alias拒绝，所有非evaluated状态保持旧输出字节不变。该函数不改变调用者舍入或FTZ／DAZ模式。

严格MSVC主机编译exit0，实际7284项断言0失败，涵盖球、退化与任意方向球柱、端点／切触、包围盒重叠但真实形状分离、无效输入、输出原子性及FTZ／DAZ切换。另以独立Python Fraction按真实盒平面交点划分线段、逐实际区段精确求最小值，不复用产品26区域候选算法；1054个合成案例包含三轴方向、不同尺度、大坐标及一double相邻边界。在4种SSE舍入×2种FTZ／DAZ设置下，分别以/fp:precise和/fp:fast编译调用，合计16864次精确对照，全部距离界包住精确最小值，无错误分离或错误重叠。每种编译配置的8432次结果为7984分离、240重叠、208未确定；未确定未转为通过。前后142项实际编译依赖SHA完全一致。源副本、独立算法、全部输入／逐项输出、制品及真实日志保存在analysis/cover-system-20261108/point-overlap-r64。

这只补充传入形状的数学计算，不证明场景候选完整、当前参与碰撞的组件／姿态／世界已关闭，也不是完整身体扫掠或终点占用证书。尚未接入Native Tick、实际移动／恢复、同版hit／muzzle或双眼发布，严重吸附肩／身体遮挡仍FAIL，逐点GrabPose及实体头显未验收。全部CV-01–24／AC-01–22保持完整未完成；不以本批主机结果提高历史分数或宣布整套掩体可用。

待运行Coverr54 prepared6ec774ef／plan915215be与24原冻结声明、37当前及归档driver SHA＋AST、3saved制品再次匹配；本批没有修改该候选，尚未收到Snub最终交窗，未stage／run，也未交付唯一后继Bol180947运行权限。后到Frag200216明确撤回，新200930申请仅单次沿既有Bol→HB链转至实际尾，保留全部早到先序，Cover未受理第二后继。本轮没有新Native／PG构建、部署或游戏／XR运行。下一步仍须取得当前同源完整形状和查询语义，再完成完整路径／占用及真实身体消费者，不能以单形状判断替代。

## 2026年11月9日 r65旋转盒与查询盒的完整分离轴判断

r59完整身体清单包含Box，但r64只支持Sphere／Sphyl，不能将其直接用于包含Box的完整体积。本批新增独立cover_box_overlap.h，检查调用者提供的世界旋转盒与世界轴对齐查询盒。按[Geometric Tools原算法说明第2节](https://www.geometrictools.com/Documentation/DynamicCollisionDetection.pdf)覆盖双方各3个面法线及9个边叉积轴，共15个潜在分离轴；仅检六个面方向不足。所有非零轴均计算，不在第一个似乎分离的轴提前退出而隐藏后续数值故障。此函数不改变r64 Sphere／Sphyl接口，不将两者单项成功称为整份身体清单已检查。

旋转定义为对输入近单位四元数精确归一化，矩阵各项使用四元数乘积除以模平方的表达式，不把已舍入的轴当作精确正交。各投影、支持半径及间隔继续复用r64向外扩界运算；仅间隔下界严格为正才分离，每个轴的间隔上界均非正才重叠，其余保留indeterminate。只有数学上精确零的叉积轴可跳过，不能按经验epsilon删除近平行轴。输出没有距离或移动／占用权限；Box半长、姿态及未用字段严格检查，numeric／invalid／unsupported／alias均保持旧输出字节不变。当前原生矩阵与此几何定义的对应关系仍须同源认证。

首轮严格MSVC编译在std::array的空花括号元素初始化处失败，编译exit2、未运行程序；原两源及失败日志保留。仅将该零元素改为显式point(0)后编译0，最终主机2096项断言0失败，含1000个独立整数轴对齐判定、点查询、触边、旋转／四元数符号／近单位归一化、无效输入、下溢、别名及浮点模式不变检查。没有放宽判定或删除失败场景。

独立Python Fraction使用Hamilton四元数乘法求精确角点，并将双方各12条边裁剪到另一盒内，未复用产品SAT公式或15轴枚举。686个任意方向、尺度、薄／点查询、大坐标及相邻double边界案例，在4种SSE舍入×2种FTZ／DAZ设置、/fp:precise与/fp:fast两种编译下共10976次对照，无错误分离或错误重叠。每种编译5488次结果为3304分离、1880重叠、304未确定，未确定不放行。另有17个确实六面投影全重叠、只由叉积轴发现分离的案例，精确边裁剪均判不相交，其中一例固化进主机回归。实际140项编译依赖前后SHA一致；源码、首轮失败、精确输入／逐项输出、两种编译制品及日志保存于analysis/cover-system-20261108/box-overlap-r65。

这是传入Box的窄相数学，不发现或关闭当前场景候选，不证明完整身体清单、连续姿态路径、终点占用或Native移动／恢复。Convex仍明确不支持，任何完整消费不得省略它。r54 prepared6ec774ef／plan915215be及24冻结声明／37当前和归档driver SHA＋AST／3saved制品再次匹配；未收到Snub最终交窗，未stage／run，唯一正式后继Bol180947仍无运行授权。本轮无新Native／PG构建、部署、游戏或XR；严重吸附肩／身体遮挡仍FAIL，同版hit／muzzle、逐点GrabPose、CV-01–24／AC-01–22及实体头显均保持未完成。下一步须将已认证的完整形状清单与各类型判断连为不遗漏的消费，再完成真实场景路径、占用和身体事务，不能把单盒检查当作完整掩体验收。

## 2026年11月9日 r66完整形状清单的逐项重叠消费

r64／65分别提供单项Sphere、Sphyl与Box数学，仍不能证明一份身体清单没有漏检。本批新增cover_body_overlap.h，将r59 Geometry、独立逐body四类count和骨骼Mapping交给同一有界入口，逐项分派给既有两种判断。清单总数／容量、body／kind／原数组序号必须全部匹配，逐shape的完整FName与骨索引须对应同一Mapping；Mapping另检查支持签名、root、引用名称关联和重复骨等结构。这里只核传入值的关联，不替代当前Model／C0的bind、资产与数组来源关闭或同姿态认证。

完整消费最多256项，不在首次重叠或未确定时提前成功。后面的无效、数值失败、身份／序号错误或未支持项均使整次检查拒绝，保持旧输出全部字节不变。声明含Convex时即使调用者漏掉相应row也明确unsupported；计数缺项／增加、截短、错序、重复序号、超容量或整份空清单不能得到分离。输出含已检查数、三类数量及各关系计数和首个对应序号；仅全部项明确分离时整份为separated，只有未确定而无明确重叠时保留indeterminate。有已证明重叠且所有项均完成时报告overlapping，未知项不被转成空间自由。容量缩小复用时不保留旧计数或旧首项序号。

新检查使用保留Marcus115 reference／C0数据，实际走prepare_model→完整FName bind→r59 authored转换→r66消费。8body／53shape（21Sphere、16Box、16Sphyl）均为本次合成清单，不是新游戏资产观测；远处查询全部分离，大范围查询及逐shape自身中心查询覆盖全部53项。另有120组混合整数格点清单，每组48项，以独立整数间隔／距离合成并集结果，核三类数量、关系、首项和输入不变。固定检查覆盖首项切触但其余分离、末项新增明确重叠、首项未确定后末项无效，以及最大256项→1项复用；缺项、声明Convex、最后一项NaN／距离计算溢出／错误端点、映射改变和alias均保持旧公开输出。

首轮编译因Windows头把验证程序局部名far展开为空宏而失败exit2，未执行程序；原源与日志保留。仅将该测试局部名改为distant后通过；最终显式/fp:precise的严格MSVC编译0，BelowNormal主机实际1126项断言0失败。225项编译依赖及3份输入fixture前后SHA一致，源副本、首轮失败、最终日志／制品和检查范围保存在analysis/cover-system-20261108/body-overlap-r66。这些检查验证了实际生产转换／分派代码，没有另做副本实现代替被测入口。

完整性只相对于这份已提供的count／Mapping／Geometry；本入口是“整份身体形状对一个AABB”，不是“身体对整张场景”，也不是连续姿态扫掠、终点占用证书或移动许可。当前真实资产清单／活动物理／碰撞世界、路径、Native移动恢复、同版hit／muzzle及严重吸附肩／身体遮挡仍待解决，逐点GrabPose与CV-01–24／AC-01–22及实体头显均未完成。未把新数学加入r54候选或打开cover写入guard。

Coverr54 prepared6ec774ef／plan915215be及24冻结声明／37当前和归档driver SHA＋AST／3saved制品再次匹配，未stage／run。Snub已直接报告接管Troika交出的原174315窗口，须等其实际全终后再最终交本方；该消息不授Cover当前运行权。Bol180947仍是唯一正式后继，更晚GN202146和Troika203000仅单次经Bol沿真实尾路由，全部早到先序保留，Cover未加第二位。本轮无新Native／PG构建、部署、游戏或XR运行，目标继续active。下一步仍需取得真实同源完整形状，再将完整消费与当前场景、完整路径和唯一身体事务接通。

## 2026年11月9日 r67场景候选树的节点与范围参数区分

r66完整清单对一个查询盒仍不等于完整场景碰撞；后续须取得不遗漏候选的真实场景来源。本批沿r61原0x14018e6c0→D0650补核D093C与D0B54两个完整helper。重要区分：18e6c0以LEA把静态0x141afac20传R8，另从query receiver+8取qword放RCX。D0650把RCX保存r14作为节点、R8保存r13作为范围，RDX是本次查询状态。因此静态地址是根cell立方体范围，不是根节点指针；当前根节点仍须从认证receiver取得，不能把静态全局地址当节点或另猜一个静态指针。

原节点visitor读取node+0的组件指针数组、+8的32位数量、+16的children指针，数组组件指针步长8。递归对子序号先乘3再乘8，证明子节点record步长24；此visitor没有读取或证明+12容量字段，不能凭TArray习惯补签完整数组布局。组件+0xf4仍是原查询会写的访问stamp，直接调用原visitor不是被动读取；未来如做自有候选去重，不能写原stamp或复用旧时相的去重状态来省略新查询。

D093C的cell输入是float32中心xyz于0／4／8、半边长于12。原RIP常量实际0x1417070e4为0000003f，即0.5；child half先按float32乘0.5，再按index的bit4／2／1分别选择x／y／z的正负，偏移中心。D0B54完整原分支对有限合法查询选择正侧当split<query.max、负侧当query.min<=split，按7到0输出。点恰好落在三轴分割点只选child0；从分割点向正方向延伸的闭区间同时包含负侧边界child。本批比较对照限普通有限float32与零，未认证原线程DAZ／FTZ及次正规数语义。它只根据分割平面选择子项，不证明查询在父cell范围内，也不证明插入／迁移或整张场景完整性；这些对应和动态来源继续待查。

原code／data全SHA分别911ab9bc…06ae6／c5deb081…5231c；4段共1365原机器字节及33锚点核对通过，其中两个helper新增398字节。有限范围的指令模型解释原D0B54比较／分支，核343组边界组合及1000组随机有限输入，共1343组与独立平面条件完全一致。这是保存原指令的模型对照，不是执行原Native方法或游戏测试。首轮核对因一个call指令的预期字节抄错而拒绝exit1，保留原核对脚本和错误实际／预期值；改成保存原码的准确字节后BelowNormal核对exit0，未放宽任何原码匹配。静态全局当前cell值仍未知，保存段不覆盖它。全部原片段、模型、逐项结果和未知范围保存在analysis/cover-system-20261108/spatial-candidates-r67。

同目录另保存新的r54准备核验：prepared6ec774ef／plan915215be、24冻结声明、37当前及归档driver SHA＋AST、3saved制品、所选EXE／PhysX及审查Python SHA全部匹配；实际Pillow／numpy／native imaging导入及原wrapper --prepare-only均exit0。独立CIM快照当时未见游戏／XR／录制／该路径编码进程，但它仅是当时的进程快照，不替代Snub最终唯一交接。Snub仍在核本方实际清理／双路证据；Cover未stage／run、未保存新运行设置基线，也未交唯一正式后继Bol180947运行权。

本批没有产品写入、原查询调用、新Native／PG构建或部署，也没有将静态树路径偷偷加入r54。当前场景参与组件、原树插入／迁移规则、同源完整资产、连续身体路径／终点占用、Native移动恢复、同版hit／muzzle、严重吸附肩／身体遮挡、逐点GrabPose及全部CV-01–24／AC-01–22和实体头显仍未完成。下一步在收到实际最终交窗后先运行既定r54一次原A及读取见证，取得当前数组失败事实；未来场景候选接线必须按本批区分的节点与cell来源认证，不能把Native no-hit或单盒检查当完整占用。

## 2026年11月9日 r54实跑41460数组范围见证与闭环

Snub11036最终唯一交窗后，先独立核其review9836f6f9、正常清理、8项精确恢复及全部job终态，再fresh核r54 prepared6ec774ef／plan915215be、24原冻结声明、37当前与归档driver SHA＋AST、3saved制品及所选EXE／PhysX／审查Python与Pillow／numpy。独立idle／CIM空，另保存10项当前基线后，只stage原r13 Native5cf4adea／PGf8e6f3c2／XR16840b27。保持0.4、behind-windows、observe-only、test mute及物理输入隔离；r64–68新代码均未加入本次候选，没有新增Trace／方法调用／身体写入或第二case。

实际game41460、creation134387024382995393完成一次原A，CoverType2／CoverAction0。首body仍为完整FName b_MF_Pelvis27549／0；SphereElems header全0（data／count／capacity均0），ArrayProperty维数1、大小16，inner StructProperty维数1、stride80，target确为KSphereElem／ScriptStruct，而target_extent_observed为72。见证明确closing_verified=false、source_accepted=false、activation_allowed=false及additional_process_reads=0。由此定位当前等值准入的72!=80拒绝事实，不能把它叫作资产损坏或直接放宽所有结构。真实原生步长与反射范围能否以特定布局兼容仍须按原消费者／字段边界证明，空数组也不绕过身份与布局认证。首失败后即正常停止，未继续其余body、hit或暂停恢复，不重跑相同行为。

实际2778次读取、35529字节、源读取段314.1502ms；读取消费错误独立保留。完整同源查询没有关闭，不能将开始时的PhysX原码观察、单body字段或空Sphere数组称作完整物理／命中覆盖。原26028同一错误仍保留为先前缺见证的失败；本次新增的是已经读取的target／inner／extent事实，没有新增RPM或改变原短路判定。

实际双路372张game／453张sim帧，span约21.27／21.23秒，最终覆盖差16.67／0ms，配对视频21.3秒且ffprobe有效。1页完整paired总览和6张约18.0／20.0／21.2秒game／sim原图已亲自检查：接近时头手／世界可见，原A吸附后肩部与身体在镜像及双眼严重遮挡，视觉仍FAIL；起始标题／等待卡及第三人称曝光保留，不算第一人称／掩体完整体验通过。未从这组图接受逐点手型、真实身体碰撞或头显舒适度。

wrapper工具85918实际exit0，operator46976实际exit1；Native cleanup／entry restore=true、pad restored、XR0、launcher0、remaining_owned_game=null。session的EndSession成功仍false，与terminal exiting／objects destroyed／shutdown completed分别记录，不能改写。paired编码工具76995实际exit0；唯一媒体保留工具25671实际exit0，最新3=41460／11036／18952、48小时，删除30项既定旧媒体、2712582字节，errors0。运行后37driver SHA＋AST全匹配，安装的3制品在恢复前匹配本候选；manifest、config、原BOM settings、原本不存在的head command、公开candidate-state、3私有制品和2份私有EngineUserSettings共10项恢复原字节／存在状态。公开v71及手动入口未变。全部证据和闭环见analysis/cover-system-20261108/array-witness-r54/runtime/closure-proof.json。

唯一后继保持Bol180947，结束前独立重核其4身份文件、3780保存源／当前及归档驱动／支持文件SHA、24current＋24archive driver、74support、122AST及4bin，原source commit94813f35存在，原主握单段scope与入口不变；41项未解析import仍明确不是完整环境证明。fresh idle／CIM为空，完整核验保存在boltok-ending-successor-check.json；实际最终唯一交接另存final-handoff.json，不向后到申请者并列授窗。

下一步只针对本次72／80布局拒绝核对并修复读取合同，再准备有身份的新候选；不跳过空数组认证、不全局放宽extent、不声称整套掩体已修复。当前完整资产来源、活动物理与世界候选、连续姿态路径／终点占用、Native移动恢复、同版hit／muzzle、严重吸附遮挡、逐点GrabPose、全部CV-01–24／AC-01–22与实体头显均未完成，goal继续active。

## 2026年11月9日 r69原生元素步长与反射范围的读取修正

上一轮41460的SphereElems80／72首拒已在保存的typed-memory夹具中复现，另复现已知字段codec下Sphyl80／76的同类拒绝；先保留旧两源、复现脚本与见证，再修改读取器。原聚合130e7c的三条array循环分别按80递增，count位移8／24／40对应Sphere／Box／Sphyl，原叶函数另核Sphere Radius+64、Box XYZ+64／68／72、Sphyl Radius／Length+64／68。全部原code字节与源SHA911ab9bc…06ae6吻合；这些是保存原码，不是新运行或当前全类布局已通过。

cover_physics_shapes.py不再把本次三类已知native元素的下一项步长当作其反射字段范围。聚合入口显式指定named primitive codec及当前声明owner，严格要求Sphere stride80／extent72、Box80／80、Sphyl80／76。每类还须完整字段表、TM0／64、精确维度偏移与类型、末端4字节Boolean、Matrix四Plane及Plane四float布局全部吻合；即使数组为空仍检查。字段身份、外层、类／number、数组数量／指针、总形状上限、字节／读取预算及同一query关闭规则均保留。Boolean mask仍取当前经过单bit检查的反射值，不猜原码未证明的bit常量。

80字节只用于下一项位置及整段payload读取，Sphere尾8／Sphyl尾4不解释为几何；完整payload仍进入同一query的关闭比对，尾字节变化同样拒绝。其他结构数组继续严格stride==target extent，不按通用16字节对齐或stride>=extent放行；72／72的Sphere、76／76的Sphyl及任意96步长等不符合本native codec的组合也拒绝。当前游戏仅实际观察到Sphere80／72；Box／Sphyl的当前完整字段与范围仍待新游戏逐项验证，不能将本表预期写成实测通过。Convex未据本批扩大支持。

原26项shape／physics检查保持通过，新增8项后共34项0失败、0跳过，使用实际审查Python、BelowNormal执行。新增覆盖空数组完整schema、两个80字节元素步进、NaN样式尾字节不作几何、任意stride／extent拒绝、损坏空schema、关闭时布局／尾字节变化、普通Vector仍严格相等和combined source不产生物理许可。新增直接调用检查首轮误把fixture内部property描述当已解码field，缺size而报1error；保留原日志／源码，仅改为通过实际layout取得field后全通过，未放宽产品guard。7个相关源依赖在最终检查前后SHA一致。证据、原码锚点、旧拒绝、源码副本与真实日志在analysis/cover-system-20261108/array-extent-r69。

核对当前运行后继Bol180947的冻结依赖清单，本批两文件不在其24driver／74support中；本方未读取其当前done／config／perPID helpers，也未stage、运行、改变3制品或重建未改的Native／PG／XR。原r54冻结候选及其37归档driver仍保留，不用其旧plan混跑新读取器。下一次必须准备带新reader／driver身份、明确新增完整schema读取范围的候选，排在所有既有后继之后做一次定向检查；当前修正仅完成源码／离线验证，最新实际游戏结论仍是41460的拒绝和遮挡FAIL。

交接统计另作更正并保留原不可变凭据：Bol支持清单含1项非.py的reloaded-profile.json，实际Python AST数为121，其余JSON只作哈希核对；此前122把该支持项计入。实物SHA、候选身份、作用范围和唯一后继均未改变，独立更正记录为handoff-AST-count-correction.json。完整CV-01–24／AC-01–22、真实完整资产、场景候选／路径／占用、Native移动恢复、同版hit／muzzle、严重吸附肩／身体遮挡、逐点GrabPose与实体头显继续未完成，goal保持active。

r69 prepared578f178e／plan4e1fda1c／readye21af1dd已封存于analysis/solo-m1-20261028/equipment-controls-prepared-20261109-cover-array-extent-r69。复用原r13的3saved制品及24源声明，仅更新明确的reader codec和独立run_cover_array_extent_check.py入口；37current＋37archive driver／74个实际.py AST、6项review-host文件SHA及Pillow13.0.0／numpy2.3.5匹配，原式prepare-only通过。新增范围明确为完整schema元数据读取，不冒用r54零新增RPM身份；不新增方法调用／输入／写入，不放大原预算。新请求仅单次沿既有Bol→HB尾链转达，保留全部早到申请顺序，尚未正式受理、stage或运行，仍须实际尾独立核验及最终唯一交窗。最初未投递的元数据副本保存在initial-preparation，不可作为队列身份使用。

## 2026年11月9日 r69-admission-v2接管约束及完整主机绑定

实际尾Troika核验指出旧r69缺完整host／origin与校验前消耗的排他接管约束。旧prepared578f178e／plan4e1fda1c／readye21af1dd已明确撤回并通知Troika与Bol，不得stage／run；其冻结文件、原只读预检、转达历史及失败原因保留。撤回不改变reader源927ca1c4的34项离线结果，也不把修正说成已游戏验证。

替代equipment-controls-prepared-20261109-cover-array-extent-r69-admission-v2：prepared f10d978e857cfe5cf4ab947f72c262667e5382dd7c0078acdd4a958dea6259a0，plan5e6d566b13fd200192bcf9b2f5db80e6c47015e0e262a637fccce398e42a3dc7，readyccb519fca02046d303c21a2f58737b147e5788f0de419f70dc9edab3bf3f7efa，入口8d3487a5c9bbc9f4f27b0e589ed3c644c46cbbf2cbf5736c9c2b4d0d3160ffc3。仍复用原r13三saved二进制与24原源声明，未混r64–68或重建Native／PG／XR。完整项目Python运行邻域364当前＋364归档逐SHA及真实AST共728；主机依赖9271文件／796019693字节，含两份Python及完整Pillow13／NumPy2.3.5源码／cache／native库、7实际module origin、package-inspector支持库、WGC录像、ffmpeg／ffprobe、private-instance身份、模拟器manifest／bin及quiet-startup依赖。依赖清单SHAeec50fb31abbee784cd799803e3272d98196520f8ecdbaef20cf21b3794ab1a5，OS／GPU／Steam／system DLL仍明确不在此文件绑定内，不冒称完整硬件环境证明。

固定cwd为当前6dof-single checkout；入口使用固定审查Python3c48fbdc、-E -s -B -X utf8且不用-I。实际执行先清继承GEARSVR_／GEARS_／XR_与PYTHONPATH／PYTHONHOME，再设置精确5项outerENV；原driver新增launcherENV逐项记录在plan和tail-packet.md。原0.4、behind-windows、observe-only、test mute、物理输入隔离和原预算保持。一次性接管文件固定于array-extent-r69/admission-v2/claims下，以OS独占新建并fsync，发生在参数、plan／ready、ticket、source／dependency、idle和stage的任何校验之前。任一步失败均保留消耗记录，不得删除或复用；只读prepare-only不消耗。stage仅允许在同一入口消耗＋全核＋最终唯一交窗及10项当前基线ticket＋idle之后执行，不先单独stage。最终ticket及实际claim目前均不存在，未占用运行窗。

真实入口复制到隔离临时根后，7种早失败覆盖坏参数、缺plan、坏ready、缺／坏ticket、身份不符及缺依赖，均先消耗、再次执行FileExists拒绝；8独立进程竞同一临时路径恰1消耗／7拒绝。这只验证准入顺序与OS排他，不是游戏／正常stage成功证明。实际精确入口只读prepare-only再次exit0；未stage／运行，未操作当前owner的done／config／perPID helper。完整命令、ENV、依赖／origin／SHA、claim路径、验证日志及边界在analysis/cover-system-20261108/array-extent-r69/admission-v2。

替代请求已直接提交实际尾Troika独立核验，明确保留Bol212342及全部早到请求在前；尚无受理、最终交窗或运行许可，不倒向前驱重复递交。运行范围仍仅一次原A和source／internal authored shapes／hit filters、首失败停止及完整正常收尾。最新实际仍41460数组拒绝和吸附肩／身体遮挡FAIL；当前完整资产／场景／连续路径／占用、真实Native移动恢复、同姿态hit／muzzle、逐点GrabPose、完整CV-01–24／AC-01–22及实体头显未完成，目标保持active。本轮为已落地的准备／准入验证progress，不是已验证运行句柄的wait。

## 2026年11月9日 r68插入布局与r70根来源区分

在不占当前运行窗的保存原码核查中，r68以完整code911ab9bc／data c5deb081 SHA核对2995字节原函数片段及75锚点，另补核构造helper原函数元数据漏掉的7字节48895c2428ebda。该段实际含stack spill及回跳53eb51；旧chunk记录未改，另存metadata-gap-code.json，不能把旧元数据不连续的loop当完整控制流。原0x14053eb20的0／1／8项整数指令模型分别产生精确构造地址；其8项＋原17字节ctor模型恰覆盖200字节分配中8字节count cookie与192字节records，不执行原函数。

插入原D10F8现直接证明node data+0、count+8、capacity+12、children+16，共24字节record；capacity不是再凭TArray惯例推断。组件反向成员数组为data+0x164、count+0x16c、capacity+0x170、8字节指针步长，不能把data强制改成8字节对齐。既有count至少10、children空且float32(parent half×0.5)严格大于100时走分裂；200字节分配／8对齐、cookie8、8个24字节child、ctor3d0e20与递归dtor1efc5c相互对应。原表byte slot0是deleting destructor，slot8不是插入，slot16才是插入／更新，避免把vtable顺序误标成方法。分裂会清旧count／capacity、移除反向成员再按component+0x160的0x400标记走normal D0CF8或special D0F40重插；因此头尾指针稳定或单次读闭合不等于整世界同步。完整逐字节证据为spatial-insertion-r68/insertion-layout-evidence.json，实际脚本exit0；尚未取得当前树／所有分支或运行许可。

r70另核原Actor.Trace包装到CFA40的5段共3550字节及9个关键锚点：CDF60从RIP实际地址0x141c20890（RVA1c20890）加载global world到RCX，CDF8F调用CF8F8；CF930存RBP、CF966原值转RCX并在CF977调用CFA40；CFAAA保存RDI，CFD3C才从该UWorld+0x178取hash receiver，CFD52取vtable、CFD77调用byte slot0x20。两处load-to-call窗口无RCX改写，静态绑定核验exit0。现有cover_state／world_state中的world／level／world_identity实际来自Actor.WorldInfo，是WorldInfo actor，不能直接对它读+0x178冒充UWorld hash。下一接线须在同一query中认证当前WorldInfo的外层Level／World与global UWorld一致；该外层对应目前只是待观测要求，不是已经实测。

上述只新增保存原码证据和原指令模型，不新增游戏读取、Trace／getter／虚调用、输入、身体writer、构建或部署，也未偷偷放入待验r69-admission-v2。当前global／receiver／root、BSP及special参与范围、完整场景与动态同步继续未认证；这些证据不能给完整身体连续路径／终点占用或Native移动／恢复许可。源事实记录在spatial-root-r70/root-origin-evidence.json。待验包仍仅排队请求；最新游戏41460及其遮挡FAIL、逐点GrabPose、同姿态hit／muzzle、全CV24／AC22与实体头显未完成，目标active。本轮为新的方向性证据progress，无活动运行句柄可称verified wait。

## 2026年11月9日 r70同query只读根观察模块及新包正式受理

新增cover_spatial_root.py，沿已有CoverQuery和当前模块证书观察根来源，不新建observer／session／线程／输入／队列，不调用原函数。它重新认证传入WorldInfo身份和Class链，再要求其outer为当前Level、该Level的outer为World，并与当前module内RVA1c20890的global World指针相等；这是必须实际读到的严格关联，无猜测备用根。只在该UWorld+0x178取hash receiver，核receiver头、原vtable地址及8个精确当前目标；从receiver+8取得root，分开读取静态root cell中心／half，不把静态cell当root指针，也不从WorldInfo+0x178误读。

根观察同时把原wrapper／world-route 3550字节及11段spatial原码4077字节加入同一query的首值／关闭括号，包含旧metadata漏掉的构造loop7字节。数据RVA须落当前模块可读非执行段，所有代码须在允许执行段；原0.5／100常量逐原字节核对。节点只解释已证明的24字节data／count／capacity／children头：root若有children，仅验证count cookie8并读8个child头，拒绝明显自／兄弟／根循环，不追孙节点、不读任何组件数组载荷。容量上限1048576明确只是本观察的保守操作限额，不是原引擎最大值。root／child头、指针、cookie、cell及原码都由同一CoverQuery关闭时重核。没有把这种头尾一致称作原子帧或碰撞世界同步。

真实审查Python3c48、-E -s -B -X utf8、BelowNormal运行新增14项与现有world-code8项，共22项，0失败／0跳过，7源SHA运行前后不变。反例含WorldInfo／Level／World错类／outer／默认／身份／类循环，global错世界且在hash读取前拒绝，空／未对齐／越界指针、vtable及各目标改动、坏容量／data／child cookie／头循环、全部11段原码与先前漏段改动、非有限cell／非正half／原常量不符、模块／section权限及范围；同query关闭会拒绝global／outer身份／hash／root／第8child／cookie／table／cell／常量／代码的变化。有效夹具仍有未映射组件payload和孙节点，模块确实不读取它们，且明确complete_tree／complete_scene／world_sync／path_or_occupancy／activation均false，不把局部空头当完整no-hit。证据和源码副本见spatial-root-r70/last-checks.json所指精确checks目录。此为保存原码＋合成process-memory离线验证，未游戏实读或实体通过，尚未接入runtime driver。

Bol212342+execution779eca在独立9271主机文件、24原声明、364current＋364archive／728AST、3saved、EXE／PhysX／7origin及精确prepare-only核验后，已正式受理Cover r69-admission-v2（f10d978e／5e6d566b／ccb519fc／8d3487a5／eec50fb3）为唯一后继。独立check SHA089d9dc284c19bc089f37c8da8cb61b6ce64c86dbdf7ca0729131338f0e0bf82；正式受理snapshot和额外前序契约另存array-extent-r69/admission-v2/queue-acceptance.json。原independent check内accepted=false保留，不能把当时核验自动改成授权；正式accepted来自后续successor-current和消息。最终ticket前序额外固定为Bol thread01a1bb45-27c8-7870-bf79-0129be8fd5be及该新候选的真实最终证据，不能用旧18152／旧HB／其他前序。当前只是受理，仍无窗／final handoff／ticket／claim／stage／run。

已通知实际前序和Troika避免并列受理，Snub221733仅为待补准入绑定请求，当前未正式受理任何后继。本批两个新文件未加入已经受理的r69包，原364当前driver逐SHA仍匹配；不为新根观察扩大该包scope或换身份。当前完整场景／BSP／特殊分支／动态同步／完整连续路径／占用、Native移动恢复、同姿态hit／muzzle、吸附肩／身体遮挡、逐点GrabPose、全部CV-01–24／AC-01–22与实体头显均未完成。下一源码方向是根之后参与组件／形状的完整性与动态变化证据；游戏仍只待Bol实际全闭环后最终唯一交窗，目标active，本轮为源码和反例证据progress。

## 2026年11月9日 r71可达树清单与玩家动作顺序纠偏

新增cover_spatial_inventory.py及测试，复用r70同一CoverQuery根认证，迭代遍历所有可达child，不按查询位置裁掉任何分支、不写原component访问stamp。每个child分配含cookie的范围不得与root或既有child分配重叠，读取8项后为每个24字节头保留首值，弹出时再读；因此深层别名／循环或group见证后头变化会拒绝。全部node组件指针载荷进入同query首值／关闭括号，同一node内重复拒绝，不同node共享同一PrimitiveComponent则只认证一次对象并保留全部来源node。最后从该组件实际+0x164读取16字节反向成员头及完整payload，严格核集合等于全部正向成员node；不误对齐成+0x168，不丢第8项或最后一个孙节点。

节点4096、深度64、总组件槽16384、唯一组件4096及单组件反向成员4096均为本观察操作限额，不是声称原引擎最大值。越界就拒绝整次观察，不截短后称成功；原Query16384reads／2MiB／3秒预算未放宽。输出仅称all reachable tree records observed，明确树外／overflow storage、BSP、owner world、全部shape／filter、世界同步和身体路径／占用未认证，即使可达树空也valid_empty_collision_result=false。此模块不是替代原引擎连续查询的新碰撞后端，未接runtime或已受理r69。

新增14项及已有根／world-code22项共36项，0失败／0跳过，真实审查Python3c48、-E -s -B -X utf8、BelowNormal实际exit0，9源SHA前后相同。17节点／3组件／5正向槽夹具覆盖共享组件、第8孙节点及双向关联；反例包括节点内重复、坏Primitive类／默认／类循环、空／错对齐／越界成员指针、缺失／额外／重复反向节点、坏cap／storage、深层cookie／头、共享child分配／祖先循环、全部操作限额、读后变化和原预算拒绝。有效观察前后合成内存不变、无bridge或engine调用。日志和源码副本见spatial-inventory-r71/last-checks.json。离线树遍历通过不构成当前完整碰撞世界证据。

主人经统筹要求这一次完整方案与进度review；仅据已有证据写analysis/coordinator-plan-review-20261109-2243/cover.json，没有为填报告新跑测试／构建。7步覆盖进入视图退出、真实探身与跑动稳定、抓边拉拽保持重抓、完整碰撞占用恢复、受击枪口与射击、逐点手姿及全部中断交付。按指定A20%／B30%／C20%／D15%／E15%的离散工程评分，各块均只够部分已编译实现0.25，合计25%；这是开发／可交付粗粒度评估，不是可玩完成率、工时、头显接受或更改历史固定分数。当前真正缺口仍是零range／空edge、UnavailableBackend、C0 cover拒绝、严重吸附遮挡以及完整姿态的碰撞／命中／枪口闭环。

实施顺序已纠偏：保存本轮既成树模块后停止继续扩大独立观察器／封套／手工世界重建作为主交付。优先基于41460已保存来源修复可见遮挡，并优先认证复用原引擎已有连续查询和起终点占用路径；只有具体入口／覆盖缺口被证明不可用时，才补对应窄缺口。原r63 PointCheck会跳Sphere／Sphyl的事实保留，不泛化为所有原LineCheck／Trace路径均不可用；现有Actor.Trace参数入口支持有界非零extent和stationary诊断输入，仍须认证完整形状、初始重叠、self排除、早停、当前动态target和GT／世界同步后才可作安全证据，不能因它能返回就放行。下一可玩交付按一个低掩体整段动作：清晰进入→真实小探→LGrip拉出／松手保持／重抓缩回→合法半探射击和枪口墙挡→原版退出，物理／受击／枪口绑定同一有效姿态；通过后扩高墙／转角／动态及全部恢复，逐点GrabPose仍必交。

本轮重新核41460保存Native状态及r13源：旧cover_camera enabled0／attempts0，不能把开启旧preview当已证修复；authored-eye350次采用／0拒绝，head palette362／肩甲mask362／head-section362及write_failure0，不能再笼统归因为眼点来源缺失或mask未启用。计数不是每Draw／硬件像素证明，不据此盲扩身体隐藏。standing view reader自身要求当前CoverType0，因此aggregate held标记也不能直接证明掩体用了旧站姿眼点。fallback读取当前原动画head的作者眼点，而当前C0仍因cover拒绝；准确眼点／身体／视线关系还需同场景几何。原6张关键图当前已不在原路径（正常滚动媒体范围），当时亲审与FAIL记录保留，本轮不冒称重新看过图。来源排查及未知范围在spatial-inventory-r71/41460-view-source-review.json，尚无因果修复或Native改动。

已受理r69-admission-v2仍只做原一次A/source/internal shapes/hit filters，未混r70／r71、未stage/run。Cover没有最终交窗、ticket或claim。Snub221733现明确因整手交线拒绝撤回整个申请与排队意向；LS222840在先、Boom225349在后均仅待补静态准入身份，Cover尚无正式后继，不并列授窗。正常排队不记产品失败，也不把它当全局blocked。完整CV-01–24／AC-01–22、真实身体执行／恢复、同姿态hit／muzzle、逐点GrabPose与实体头显均未完成，goal active，本轮为源码／检查和实施方向证据progress。

## 2026年11月9日 r72优先复用原连续查询的具体覆盖缺口

按主人方案review优先评估原连续入口，未再新增世界观察器或重建PhysX。沿原CFEF0核到非零extent走E6A10、零extent走E7030，两visitor均从实际组件虚表byte+0x2e8取LineCheck，而r63的PointCheck是+0x2e0。六个已有构造定位表再次核到2e0=3a6bc4、2e8=339294，不能因PointCheck缺口便推断所有LineCheck同样不可用。339294有经asset+0x248→131e80→130e7c聚合的已见分支，但该组件旧metadata仍漏3个直接cold目标，明确未闭合，不签完整当前分派或调用权限。

继续核原leaf发现一个独立于PointCheck的新具体限制：Sphere CE24C从原第7参数位置取向量交DB120，若不全零则在CE277直接置EAX=1并返回，不进入后续Sphere几何；Sphyl CF080在同一第7参数位置比较X非零时直接走CF102置1及恢复返回。原正float32向量(1,1,1)在两leaf都是这条miss1路径，且不调用实际几何。这是保存机器码的具体有限输入分支证明，不是执行原查询，也不证明所有当前组件必到此分支。它说明即便复用原非零体积LineCheck，也不能从这类Sphere/Sphyl的无命中签发完整身体／占用安全。

本批code／data整SHA911ab9bc／c5deb081、33关键锚点和6构造表匹配；BelowNormal核验exit0。证据native-query-reuse-r72/reuse-evidence.json保留visitor、组件／asset片段、正体积反例、3个cold未知及所需的当前GT／self排除／起始重叠／早停／shape/scale/bone／世界同步条件。实施方向保持优先复用可认证原查询，对上述确证的窄覆盖缺口选择补充或另一已有原生查询，不默认重建整个世界，也不以假空结果或删guard放行。本轮没有方法执行、新hook／writer、Native构建、stage或游戏；最新实际仍41460拒绝及吸附遮挡FAIL，全部CV24／AC22和头显未完成，goal active。

## 2026年11月9日 r73原PhysX场景扫掠与占用入口定位

本轮将原引擎查询复用从候选名推进到具体公开／内部接收者和原始调用转发。没有新增世界观察器、手工重建PhysX、调用游戏方法、修改Native或启动掩体录制；它是下一步碰撞后端接线所需的静态证据，不是可玩闭环或遮挡修复。

精确PhysXCore SHA22051aa7在REA／Ghidra12.1.4中核到：内部Scene vtable22ccb8的byte288→A6E30为盒体扫掠、byte290→A6AA0为胶囊扫掠，原函数分别引用linearOBBSweep／linearCapsuleSweep的明确诊断字符串。公开NpScene是另一张表23ca98；公开byte308→171770、byte310→1716F0读取本对象+6b0，再转入上述内部槽，不能将两层slot混用。两原x64包装保留RDX／R8／R9输入、按8/4/8/8/4/8字节复制公开栈参数5–10，并给内部参数11–13置零；反编译得到的单参数void签名不完整，不能照抄为调用ABI。

从既有NpActor表2330a8的byte8→135B00核到actor+38及其虚表byte8；内部Scene getter A0300明确返回Scene+270的公开包装指针。NpActor构造1328B0及其Actor构造1204C0对应这一关系。它提供从已有实际物理Actor通向查询场景的具体接线方向；当前接收者、两向Scene／NpScene关联和同一世界／代次仍未实测认证。

两个扫掠都在全零movement时直接返回0，尚未进入Scene锁或实际查询，因此零位移扫掠不能当静止占用证明。内部重叠候选byte268→A6290、byte270→A60B0会遍历所选静态／动态宽相位并测试形状及过滤；公开byte338→1682D0在内部命中时返回true，否则继续查询本对象+98的额外状态。布尔overlap不能在自身形状尚未排除时直接用作身体占用；后续必须核完整枚举／过滤、回调与截短语义，不能将未认证的空返回放行。原查询明确进入／退出Scene+238关联临界区；这不单独证明多个查询和C0／物理／命中／枪口消费之间具有共同同步保证。

选定13个方法的9,693字节及9个表槽均与精确原DLL重新对应，完整线性解码、原异常表条目及不可确定项保存在physx-scene-query-r73/scene-route-certificate.json（SHA024f4af3983288b56d85273c1024d5ca4c5c405d6bd898af95d771893ea9866f），BelowNormal核验实际exit0。原公开表136方法的有界筛选只用于找出这两项转发；完整REA记录保留，临时库已正常close且open=false。大Scene构造的high-pcode存在截短，未据此宣称完整数据流；仅引用已保存的原汇编vtable写入。游戏内按旧内部slot搜索得到874条／664owner模式匹配，不能证明NxScene类型；公开slot不同，这批匹配没有被当作实际场景调用者或权限。

此前r72组件LineCheck还缺三个cold目标，本轮按原SHA911ab9bc将热／冷联合范围闭合到1,989字节，补入798字节，每个直接分支落到已解码边界。它只关闭结构范围，不是异常展开、所有callee／virtual或当前组件分派认证。新见1D1550经原伪代码及机器码核为矩阵余子式运算，没有将它误报成已有场景查询。原Sphere／Sphyl非零extent的具体miss缺口仍保留。

视觉方面另核41460原保存三组pose：camera pitch与roll均为0，不能据这些样本把原掩体俯仰当遮挡原因并盲做pitch归零。原头部／肩甲CPU计数仍不能代替实际绘制，缺少同一时刻的头、皮肤和眼点几何；严重肩部／身体遮挡FAIL保留，没有扩大隐藏或移动眼点。证据41460-camera-pitch-exclusion.json（SHA16438e1f46ef45be29b42c2b59031c5690b430465bc2f3f6535fda4a5beff644）。下一步仍以低掩体完整动作和清晰视野为目标：在现有唯一Native消费者中接认证过的实际Scene查询／占用及完成账本，处理同姿态身体与视图，而不是删除cover拒绝、签发假范围或继续扩独立观察器。

运行队列的新事实单独保存：Bol212342旧execution779eca因继承PYTHONSAFEPATH风险在未消费时撤回；同一到序的新execution851ea67e已获其前序定向受理并实际执行，尚在自身收尾。本Cover f10五身份及原一次A／形状来源检查范围未改，尚无新最终交窗、ticket、claim或stage/run。LS231059-v2因assert flags门可被优化移除而主动撤回；其同一后继到序的235126-v3已由本方独核并正式受理，旧合同按原字节保留，当前指针为admission-v2/successor-active.json。V3反向源码仅非assert flags拒绝、清继承PYTHON*和身份变化；1596源／6bin／18动作驱动／旧1067快照不变，1068current/archive匹配，固定Jvenv实际prepare-only0。两份完整入口副本的-O／-I均在校验前先消费后拒绝、重试FileExists，十基线前后相同；不重跑未变race。它不是本Cover当前运行权或第二并列槽，后续请求继续沿实际唯一尾传递。

上一需求查找回合只核对并交付已有文档，未推动玩法；本轮按新原始入口和结构闭合证据记progress。全部CV-01–24／AC-01–22、逐点GrabPose、真实身体执行／恢复、同姿态物理／受击／枪口与实体头显验收仍未完成，目标继续active，公开v71不变。

## 2026年11月10日 r69入口优化模式修正 未运行

启动前实际源码核对发现本Cover旧admission-v2也仅用assert拒绝Python优化模式，-O可移除后续校验。真实旧claim／ticket仍不存在，本方立即撤回f10 v2及交叉到达的Bol9764最终收据728a259b；Bol明确确认作废并保留同一唯一到序。未执行旧入口，未将旧最终授权套给替代包。

窄替代equipment-controls-prepared-20261110-cover-array-extent-r69-admission-v3已冻结：prepared b14404ca／plan dae50212／ready279b1e6c／entry7cca15a7／deps原eec50fb3。反向字节对应只增加独占claim之后、其他校验之前的非assert exact-flags拒绝，并在子进程前清除所有继承PYTHON*（包括PYTHONSAFEPATH），另改本次身份。原claim／校验／stage／执行／正常结束代码其余字节保持；不接受-O／-OO／-I，无新重试路径。旧364当前／归档快照全部不刷新，仅新入口增为365；24原declared源、3保存二进制、9271宿主文件／796019693B和7module origins保持原哈希。

精确3c48解释器-E -s -B -X utf8的真实prepare-only实际0。四种完整入口副本（-O、-OO、-I、无-E的环境优化）均先消费后拒绝，重试FileExists；优化prepare拒绝且不消费。真实新增环境循环AST在独立子进程清掉PYTHON／GEARS／XR哨兵并保留普通环境，十基线前后相同。未重跑不变race和旧失败套件。完整绝对argv／ENV／五身份及证据见array-extent-r69/admission-v3/tail-packet.md，verification SHAcd8005c7705754947a7dd110ada13bc8bcc5f33be2913e3b2e38fce5e9fd47d8。

新旧真实claim／ticket仍不存在，尚未stage／运行。已把替代包送Bol独立定向核验，只有绑定新tuple的新最终交接有效；已受理LS235126-v3仍是同一唯一后继，前序绑定须随本替代身份同步。实际动作仍仅一次原A及来源／形状／hit-filter读取，不扩为未接通的抓边录像；严重遮挡、碰撞／身体执行和完整CV24／AC22继续未完成。

## 2026年11月10日 已消费的启动前失败 与r74原物理变换实现

Bol已对b144 admission-v3完成独立检查并发新最终收据66b2397f，绑定其实际9764／execution851ea67e。Root独立核全部终态证据、fresh prepare-only、idle／CIM及十项原始基线后建立本次唯一ticket。实际调用时Root把ticket实参手写为正斜杠，而冻结plan.argv／str(TICKET)为反斜杠字符串，入口第39行精确参数检查拒绝，Python PID34472／wrapper终态1。该消费是Root调用错误，不是游戏或产品失败；原prepare-only没有覆盖真实实参字符串。

b144 claim永久保留，不删除、不以换prefix在本窗重试。失败发生在plan读取与stage前，无attempt-result／stage.log，无新游戏、Native、输入、XR、录制、编码或保留作业。独立idle／CIM空、十项当前与保存原始字节或不存在全部一致，无恢复写入；不伪造未开始模块的cleanup通过，也不复用旧41460图。证据admission-v3/pre-game-failure-closure.json SHA82f956ae1d79c1daabebae0eedbbbe29321d36dd6c80b46967677d8d31da2fc9；后续实际启动须直接使用已冻结plan.invocation.argv，避免手抄路径，不放宽原入口。

LS独立接受这次pregame全终态c1b68a04。Root最终fresh检查又发现其保守闭包内共享hammerburst_reload_chain.py从c7c210b0变d4c106a6，停止旧235126-v3交接且不回滚他人文件；LS按同到序提供002143-v4，变化确为普通空仓首latch拒绝即记录并停止的3行及自身入口身份。Root独核旧其他1067快照保持，fresh1596saved源／6bin／18动作驱动、1069current／archive哈希及AST、固定Jvenv真实prepare-only0、宿主13435文件及7origins、十基线与idle／CIM后，最终唯一交LS002143-v4。最终收据admission-v3/final-handoff-ls002143-v4.json SHA58df4f7c47906ef91eb2d59c7a5ba89364a58a6a4355fd0fff2cc63fd13d993b，检查14ac7933。Root此后不持窗、不写共享runtime或媒体，不stage／run。此交接没有新的游戏身份，未把源检查扩为其他武器开发。

源实现新增tools/native-capture/cover_physx_transform.h，将已是刚体的共同身体世界Matrix64转换成原PhysX使用的48字节Matrix34。原F2610的194字节与SHA911ab9bc对应，3×3转置，平移分别执行一次float32乘法；实际RIP常量位于1417070c4、字节0ad7a33c（0x3ca3d70a，约0.02）。采用原固定游戏→物理换算，不从HMD米／游戏单位校准推导。原F26D4调用者在F2610之前规范化三个轴；本适配不擅自修输入，明确要求已验证刚体矩阵，拒绝非有限、非仿射、缩放、剪切、反射、别名及依赖FTZ／DAZ的下溢值，拒绝时输出保持。没有内存读取、PhysX调用、碰撞许可或新身体写入端。

新cover_physx_transform_test.cpp以保留的两份实际64字节组件矩阵（6a0656bc／ceb2c34e）和明确合成的非对称单位旋转／平移与identity检查UE行向量点变换和物理列向量点变换的一致性，并验证拒绝和输出保持。8个实际MSVC项目依赖已原字节冻结，/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译0；冻结源测试在两份矩阵上各63检查、0失败，源SHA前后相同。初次测试把非单位合成四元数交给只验证单位长度的函数而失败，原fixture-0.log保留，改为显式归一化夹具后通过，生产convert未据此修改。原机器码只做静态解析，没有执行原函数；CPU测试不等于实际物理查询。证据physx-transform-r74/original-transform-certificate.json SHA324e8f5eadf756031ba7831710268fdc94d2b39e599b96588d046dc3a65cb662及verification.json。

该变换现为源／离线实现，尚未接到实际Native Scene查询或构建Native DLL，没有安装或头显验证。当前形状、查询完整性／自身排除、起终点占用和跨查询／身体／物理／命中／枪口同步仍需闭合；本轮未游戏验证r69 reader修复，最新实际掩体仍41460拒绝与严重遮挡FAIL。完整CV-01–24／AC-01–22、逐点GrabPose、身体执行与恢复及头显继续未完成，goal active，工程25%和公开v71不变。

## 2026年11月10日 r75原场景查询适配源码与冻结离线验证

本轮为进展：从r73已保存的原公开／内部场景查询证据，实现cover_physx_query.h、cover_physx_query_source.h及生成的cover_physx_query_code.h，并增加cover_physx_query_test.cpp。代码尚未包含进Native C0生产入口，没有启动游戏、XR、模拟器、录像或新运行窗口；没有改变公开v71、原r13制品或已消费b144启动前失败的终态。上一轮单纯需求核对按目标进展计为无进展，本轮实际源码和固定源编译／反例结果构成进展。

源码适配提供盒体／胶囊的公开overlap与非零linear sweep调用，保留原公开10参数的8/4/8/8/4/8栈宽度，flags3仅请求原static＋dynamic closest模式，单个64字节raw hit，不声称完整命中枚举。启动prime只增加已加载、与本进程EXE同目录的PhysXCore引用，保留文件读锁并校验原2711552字节／SHA22051aa7；不调用LoadLibrary。当前BodyData主actor经actor+38内部Scene、Scene+270公开NpScene、NpScene+6b0逆关联及10个虚表槽检查，调用方仍负责认证实际C0、类型／字段角色及F26D4原布局；非零地址不授予这些事实。

本次代码校验覆盖10个完整保留函数区间共3544字节。最初只取首个PE unwind片段的2065字节方案已拒绝，code-source.json作为失败选取记录保留，正确依据是code-source-complete.json；公开168350完整125字节通过与保留1682D0逐指令对照、仅268/270槽及126770/126760调用两处差异核定。原DLL8768项非空重定位均未与这10段相交。校验这些函数不代表所有传递被调函数、形状过滤、世界或物理同步均已认证。

查询前后核同源对象及原代码，调用前另重复当前关系读取；请求使用固定副本，拒绝读回调修改Source及输入／输出别名。重复相等读取不能当作原子物理围栏。拒绝零扫掠、非有限／非正常浮点、非刚体盒旋转、退化胶囊及导致原float归一化溢出／下溢或边界溢出的输入；零位移不当作占用查询。原调用发生SEH或返回超出单hit容量时进入fault_unknown，保留uncertain待处理及原模块／文件引用；不能自动解锁、重试或确认正常关闭。此异常停机语义只有源码和合成异常反例，尚未原游戏故障验证。

实际MSVC /std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译及固定5个项目依赖编译均exit0，冻结程序119检查／0失败。检查含选定body及双向场景关系、每个读取失败保持输出、数组／指针／重复body／虚表变化、source回调修改和别名拒绝、完整原DLL数据段映射及每段末字节／截短／hash失败拒绝、两种形状的合成5参数overlap和10参数sweep／64字节结果、合成SEH拒绝、未prime不读目标、不加载缺失模块与零／非有限／溢出位移拒绝。首次/WX因Sweep隐式对齐填充拒绝已保留build-0.log，改显式reserved字段后通过。原DLL仅作为数据读取，未加载或执行；合成ABI函数不能替代原方法运行通过。

证据位于analysis/cover-system-20261108/physx-query-r75/，含source-manifest.json、冻结源码、build/test日志和verification.json。下一步将当前C0的typed PhysicsAssetInstance／RootBodyIndex／BodyData角色完整认证接至此适配，并闭合自身排除、实际形状覆盖与查询／物理消费时相；这些之前不发布掩体移动许可。Native实际移动与合法恢复、同版物理／命中／枪口、严重肩部／身体遮挡、每个实际边缘GrabPose、CV-01–24／AC-01–22及实体头显均仍未完成，不提高25%工程review或其他历史评分。

## 2026年11月10日 r76类型化物理来源与原查询来源对应

上一轮r75适配源码、冻结编译与119项检查已经提交c910a987，属于进展。本轮继续完成缺失的类型化读取层：新增cover_physics_roles_live.inl及实际公共反射夹具测试，复用weapon_hand_scene_binding的对象注册表、identity、property、继承与当前元数据关闭读取，不建立另一套反射器、输入或骨骼写入端。此层仍未包含进Native生产C0或自动执行；调用方必须提供ActualContext当前性guard和当前Backend metadata，普通非零地址不能取得C0许可。

读取严格核对当前Mesh的PhysicsAsset+248及PhysicsAssetInstance+250、各自声明引用类型与实际继承关系、实例Owner为当前Pawn／Mesh并兼容声明类型、Bodies数组+6c的ObjectProperty inner／宽度／所属数组／RB_BodyInstance目标类、动态RootBodyIndex，以及每个body的BodyIndex／OwnerComponent／BodyData+88／BoneSpring+90。OwnerComponent采用实际声明引用类和当前Mesh继承匹配，没有猜成固定Primitive／Skeletal类型。根body必须有主actor；非根BodyData空值或auxiliary缺失只作为来源事实，不能据此跳过全身物理覆盖。最多64个body，最多8种不同派生布局，既有Proof身份／源容量均保持严格拒绝，不截短成功。

字段来源额外保存声明owner的完整兄弟链、名称和身份，包括选中字段之后的节点；保留所有已用类型的完整继承链及super值。两次来源读取后再次核这些布局，防止同地址字段追加／换名／链变和缓存body类继承变化漏过。整个操作仅写自有Workspace／输出，guard在binding mutex之外运行；失败时不改调用方输出，关闭构建不读对象或调用guard。重复读取相等仍非原子快照、查询世界围栏或物理同步。

matches_query_source把本层完整body指针数组、array头、RootBodyIndex来源及选中BodyData与r75原始场景路径逐项对应；合成注册表正例已接通至r75 source::capture，原DLL没有加载或执行，没有调用引擎方法。该对应也不授予抓边／身体移动。生产调用还须在原查询前后持续重取并比较此类型化来源，不能只在启动时校验一次，也不能把这份只读Snapshot当作不再失效的能力。

实际MSVC /std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译和冻结15个实际项目依赖的ON／OFF构建均exit0。ON 241检查／0失败，OFF 2检查／0失败；涵盖Owner两种合法归属、根索引与完整64-body边界、非根换主、重复／错误类成员、数组布局、Pointer结构、字段声明类型、同地址源／继承／元数据／线程变化、异常及guard释放锁、9派生布局容量拒绝、raw场景来源对应与错误actor／root／成员／array头拒绝。首次编译因本地property名称ADL歧义及测试全局名遮蔽分别失败，已窄修命名，原build-0／1.log保留，没有关闭/WX或削弱条件。全部程序使用合成已注册内存，没有C0／原查询运行证据。

证据在analysis/cover-system-20261108/physics-roles-r76/，包括冻结源码、MSVC依赖与SHA、启用／关闭编译和执行退出记录及verification.json。本轮未修改现有脏Native／武器文件、公共v71、已消费b144入口或共享运行配置，没有stage／游戏／XR／录像／窗口交接。下一步把这份可失效来源接入现有C0查询guard与启动／关闭生命周期，再闭合自身排除、实际查询形状及消费时相；保留原no_cover写入保护。完整身体移动／占用／恢复、同版命中／枪口、严重肩／身体遮挡、逐边缘GrabPose、CV-01–24／AC-01–22和实体头显仍未完成，25%工程review保持。

## 2026年11月10日 r77实际C0查询入口与Native收尾接线编译

上一轮118c3d21的类型化来源及冻结验证属于进展。本轮将r75原查询和r76类型化来源组合为cover_physics_query_live::Session，并增加cover_physics_c0_live的四个原始观察入口：盒／胶囊overlap及sweep。入口使用现有实际captured C0、同一Entry地址及未变Core、当前owner线程、未prepare／无Kernel pending、ActualContext prepare当前性、当前Backend metadata及当前Link／slot字节关闭检查；原查询前后重取全部类型化物理来源，与原查询实际使用的raw身体／场景关系对应。普通no-argument guard仍兼容，typed guard额外收到实际raw Snapshot，错误actor不能只凭外层Owner当前而通过。

服务在原Native启动／收尾工作线程prime／释放已加载PhysX模块引用与文件pin，原查询跨越期间不持binding／stereo／session私有锁。新GEARS_COVER_PHYSX_QUERIES编译开关默认OFF；运行FullBodyC0Native.cover_queries同样默认0并严格解析，只有已有Native C0／typed continuous owner及continuous session才可开启。未开启或未编入时不启动查询。没有增加Hook或身体写入端，也未在Goal policy自动调用这些入口；现有no_cover拒绝继续保留，原始空返回不是移动／占用许可。

capture的现有stop请求、cleanup-ready、retained判定、hook-restored与日志摘要均加入此服务的pending。工作线程在无私有锁的位置poll_stop；在途／错误线程不能释放。query的fault_unknown立即关闭后续调用，实际C0入口请求原proof-failure停止流程；uncertain保留原资源及清理屏障，到现有截止仍pending时记录cover_query_stop_incomplete并返回ERROR_BUSY，不能打印正常卸载或强制解锁。此故障／线程／重入行为仅有Host调度与装载seam验证，没有原游戏故障证据。

完整Native在独立冻结r13源码基底构建，原有GEARS开关保持，仅新增查询编译开关ON；8项记录overlay和capture／CMake两项有限替换，未混共享工作树的其他脏Native／武器实现。最终编译链接exit0，DLL SHA3fb600768d0b31a0c8c24c0d237fc90a7e402d0d1453d1fd5676637ece391aa6。Ninja实际669依赖，其中389项目依赖全部位于1086文件冻结树并逐SHA核对，没有外部工作树依赖或缺失项；8个新接线依赖均入编译。第一版46b167a2也编译成功，但未包含随后“fault后立即禁止下一次调用并请求原停止”的新增修正，保留独立记录，不作为最终制品。Native保持原/W4／EHsc及既有警告，不能称全Native /WX无警告；新模块未产生警告。

同一冻结Native／反射源码加当前测试，实际/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT三程序均编译执行0：Session50检查／0失败，Raw122／0，C0关闭构建16／0；实际项目依赖并集50项。Session保留真实反射／身份／来源读取，只将模块装载与原方法dispatch替成Host seam；覆盖双查询、私锁释放、重入拒绝、错误raw接收者、根变化、查询中stop、错误线程关闭、fault后不再dispatch及永不假关闭。Raw增加typed／legacy guard分派和错误接收者拒绝；C0 OFF覆盖四入口不调用原方法／输出保持、配置0／1／invalid与关闭无pending。原DLL只读数据，未加载或执行。首个Session编译因继承测试main宏及局部hit遮蔽拒绝，改测试入口宏与局部名后通过；保留失败日志，没有关闭/WX。

共享capture／CMake原本已有其他未提交工作；本轮用精确替换分别生成HEAD与工作树改动，只暂存自己的HEAD补丁，不整文件提交那些既有改动。首次生成因HEAD仍用pending_count而工作树用resource_pending_count触发断言，未写文件；分别按原接口保留后接线。依赖核对首轮误把Ninja相对路径按工作区解析，按真实build目录解析后逐SHA通过，未修改构建源码或依赖记录来通过。

证据在analysis/cover-system-20261108/query-c0-r77/，含两次不可变Native源／构建、真实依赖关闭、冻结单元源／运行退出、有限集成patch及verification.json。本轮没有运行安装／游戏／XR／录制，没有申请或借用新窗口，没有重试已消费b144入口，也没有替换公共v71或候选。下一步是让完整身体形状／当前占用与扫掠消费者使用此C0入口，并解决自身排除及物理／命中／枪口共同消费时相，不能用closest单hit或bool overlap跳过完整覆盖。实际原查询、Native身体移动／恢复、严重肩／身体遮挡、逐点GrabPose、CV-01–24／AC-01–22及实体头显继续未完成，25%工程review保持。

提交核对补充：生成补丁在Windows写出CRLF而HEAD为LF，首次git apply --cached --check因此拒绝；当时暂存区为空，未产生部分暂存。保留owned-integration-crlf-rejected.patch，改为LF原字节写出后，两个共享文件的精确HEAD补丁检查通过。此修正只改保存补丁的换行方式，不修改已编译的Native／单元源码或旧运行事实。

## 2026年11月10日 r78有界原版全命中流，截断不放行

上一轮204308c9的C0／收尾接线、隔离Native构建及检查属于进展。本轮补上原sweep最近命中模式不能绕过自身hit继续检查的前置缺口：新增cover_physx_stream.h及原sweep_all入口，仍返回原始记录，不在缺少shape→actor证据时猜测自身归属。复用已保存REA原PhysX SHA22051aa7及worker ev_0a905f6cc36ae824d2c8db2804b18233edade37a5a7792ed983e8ddb652702fd，未新开反编译目标／观察器或运行游戏。

原A71F0完整Ghidra body为A71F0–A8949、5978字节，本轮逐字节对应原DLL、完整解码1232指令并确认该区间没有重定位项。13个原指令见证包含A734D test bl,8、容量／buffer／callback的RBP+338／340／348读取、A7CE1按64字节计算条数、A7CF9 vtable[0]调用的RCX=this／EDX=count／R8=batch，以及AL=false退出分支。据此沿原公开10参数ABI使用flags11（static1＋dynamic2＋all-hit8），明确提供64-record自有batch，避免原动态栈／分配器fallback。新worker加入当前字节校验，11个完整区间共9522字节，scratch扩至8192以容纳5978字节整段；并不据此声称所有传递被调函数已认证。

收集器是手工明确vtable[0] ABI的同步自有descriptor，回调只拷原64字节hit，不读游戏对象、不持binding／stereo／session私锁、不筛Actor。TLS限定当前同线程descriptor；非法self／数据起点／数量／线程、嵌套或晚到回调拒绝。零命中批次消耗报告预算但不是结束标记；最大1024记录／256报告可由调用方降低，容量耗尽返回false并显式truncated。原返回数量必须与已复制条数一致，malformed／计数不符为callback_rejected；截断、源变化或失败均不发布结果。原调用SEH仍走已有fault_unknown及资源保留，正常已返回的截断不伪造成异常锁债。

该入口已接过现有Session与当前C0六个观察API（原overlap／sweep四个加盒／胶囊sweep_all两个），保留原来源／当前metadata／Link-slot／Entry／100ms时限、停止及别名拒绝。运行与编译开关仍默认关闭；Goal policy仍不自动调用查询，no_cover身体写入拒绝仍保留。sweep_all成功只表示本次原选择流未截断，不是无碰撞、完整世界、可移动或占用许可。

最终冻结Native沿r77不可变源仅overlay本批7个实现文件，所有GEARS flags不变，完整生产目标编译链接exit0。DLL SHA31ea732dd259d4c5aa44c1b5a98e87bc7453da1a2ff15578ebbf131d9be18c78；670真实依赖／390冻结项目依赖逐SHA，无外部工作树依赖。Native仍保留旧/W4／EHsc设置及旧警告，不称全Native /WX；最终新模块无警告。首版1825c082保留，局部active名在真实公共fixture下触发C4459／WX拒绝后改current_reporter，另建不可变final源重编，没有覆盖首版或关闭警告。

同一冻结Native／反射源的四个程序均/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Stream40检查／0，Session58／0，Raw125／0，C0 OFF18／0；实际项目依赖并集52项。覆盖64条批次、零批次后继续、顺序与末记录、hit／report预算、错误／过期／跨线程descriptor、合成原10参数ABI、截断Native计数与复制数量不同、Session截断不发布／仍可正常关闭、两种形状、11段原字节变更拒绝及关闭构建六入口保持输出。原方法dispatch／装载仍为合成或Host seam，原DLL只作数据，不能当原运行通过。final单元Driver因预期57而实际58的汇总marker停止；原58检查已exit0且源码未变，保留记录，仅续编执行尚未进行的Raw／C0Off，没有重复Stream／Session绿色程序。

证据在analysis/cover-system-20261108/physx-stream-r78/，含原指令／合同证书、11段来源、首／最终Native与单元冻结树、运行退出和verification.json。本轮没有stage运行制品／游戏／XR／录制或重试b144，没有新增窗口、助手或其他武器工作，没有替换公共v71。下一步须认证公开hit shape→actor和完整自身集合，再结合当前完整形状／组合形变路径、终点占用及世界／物理／命中／枪口同步；不得把过滤掉首个自身hit、单个bool或本批完整流当作这些证明。严重肩／身体遮挡继续FAIL；应同步从保留同帧头／身体／原生view方向和实际蒙皮关系定位，不靠盲移眼点或隐藏身体绕过。逐点GrabPose、Native实际移动／恢复、CV-01–24／AC-01–22及实体头显仍未完成，25%工程review保持。

## 2026年11月10日 r79眼点同调用诊断与61492跨Scene证据纠正

上一目标轮仅核对已存在需求文档，未推进功能，按no progress重新核对当前状态。本轮回到严重肩／身体遮挡，不把原PhysX未闭合当作等待全部视图工作的理由。保留61492的实际SpaceBases与Local重算头／眼点接近，但原probe-61492-cover-placement.cpp明确把standing->source写成source-CT0.bin，同时从较后CPOSE episode row0取掩体metadata／local／space。前者Scene2／CT0，后者Scene292／scene_qpc9813103981965；同Pawn／mesh／reference不是同一时相。旧算出的41.3436度差值不能归因为视角与身体失配，也不说明41460原因。用新冻结依赖重新编译原数值probe，26项目依赖关闭核验、执行0，数值与旧输出完全一致；更正单独保存，不覆盖原证据。

沿现有first_person_live::prepare和authored_eye读取路径增加first_person_eye_trace有界内存记录，不增加Hook／世界观察器／骨骼写入或相机控制链。GEARSVR_AUTHORED_EYE_TRACE默认未启用；只接受空／0／1的精确值，1还要求simulator_operator、continuous session及body_basis。默认空指针路径不新增游戏读取。显式启用时最多10Hz／512行，先到先保存、满额明确overflow，不静默覆盖。每行把同次prepare的当前Scene／family／renderer QPC、来源对象键、人物与mesh／asset身份、component、完整115 SpaceBases、完整reference、实际head局部眼点、输入视图及prepare输出放在一起；不是拿旧Core或最近时间记录配对。prepare输出仍只是后续共同body view接管前的本层结果，不称最终双眼／GPU图像。

可选几何读取按原SpaceBases／reference完整复制与重复比对，关闭检查tick／pose epoch、数组头、component和用于正常眼点的head55，reference须与当前缓存所用完整记录一致。read_failed／reference_changed／payload_changed／nonfinite分别保留，最终visual owner关闭成功单独标记。诊断失败不改变正常眼点接受及视图结果；反之原眼点失败不会留下旧owner_closed。此为CPU重复读取括号，只能证明所读字节前后相同，不能证明引擎原子快照、ABA不存在、每Draw硬件buffer或真实物理／命中／枪口。

仅现有日志端在session draining／stopped、recording=false及active=0时写authored-eye-geometry.bin；其他阶段只写摘要。格式记录magic/version/row_bytes/count/capacity/due/overflow/QPC频率，写入和关闭结果检查，失败不标记exported。产品prepare、期满及诊断不授权身体位移、不改视点方向／偏移、不放宽no_cover或现有输入／身份／新鲜度／恢复约束。

三个聚焦程序/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Journal542检查／0，production prepare与drained导出41／0，原眼点读取81／0；共30实际项目依赖。覆盖默认不读诊断、精确配置、节流／倒退／容量不覆盖、格式读回、同调用输入输出与失败标记、诊断读取／epoch／payload／reference／非有限失败不改原眼点、原owner失败不留旧closure，以及运行中／录制中／回调未退出不导出。绑定与eye部分使用明确CPU seam，61492姿态用于正常读取反例，不当游戏身份或原子姿态证明。第一版prepare测试缺真实weapon类型include及已有initializer shadow警告，在独立最终fixture修正include并仅包住旧C4456；未关闭其他/WX。新产品模块没有通过关警告掩盖问题。

完整生产Native复用r78不可变源，仅3文件overlay，所有GEARS编译开关不变。最终DLL SHA8035bf35049efe954f80c6947ce582281d5f576d62a812a1c1bd12964404cfb4，671真实依赖／391冻结项目依赖全部逐SHA，无外部工作树依赖，编译链接exit0。首版因first_person include时尚无后续phase命名空间而编译拒绝，保留原日志／冻结源，改用已有SessionPhase生命周期，再在独立v2树完整编译；最终Native保留原/W4／EHsc及旧C4456等警告，不称全Native /WX。未变的542／81绿色程序经完整实际依赖SHA一致性复用，只重编有变化的prepare程序。

证据analysis/cover-system-20261108/view-alignment-r79/verification.json及61492-provenance-correction.json。没有stage运行制品、游戏／XR／录制、新运行窗口、助手、CUA或其他武器工作；Publicv71不变，b144消费失败不重试。下一步在实际唯一交窗后使用这项材料变化的新候选，短时复现严重遮挡，取得同调用几何与实际双眼关联，定位后修复；当前不按跨Scene的41度旋转相机、不盲移眼点或隐藏身体。原PhysX完整覆盖、自身集合／shape归属、组合形变路径／终点占用、同版physics-hit-muzzle、Native移动恢复、逐点GrabPose、完整CV-01–24／AC-01–22及实体头显仍未完成；严重肩／身体遮挡继续FAIL，25%工程review保持，目标active。

## 2026年11月10日 r80单次遮挡观察候选已封存／仅后序意向

上一轮900d2264的同调用眼点诊断已编译；本轮继续把它接成可执行、可复核的短切片，不重跑b144原array/filter套件。新equipment-controls-prepared-20261110-cover-eye-r80保留r79 Native8035bf35049efe954f80c6947ce582281d5f576d62a812a1c1bd12964404cfb4，PG／模拟XR原封复用，不新增Native编译或改publicv71。完整1088份Native源冻结，363项现有driver逐SHA／AST关闭，9271项解释器／包／录制依赖及7个真实module origin通过实际prepare-only exit0；该准备路径未stage、未发输入、未创建claim／ticket。

动作只复用原菜单／控制器／导航路径、一次原A进入矮掩体，随后中立、固定位置转头返回、真实追踪升高10cm返回。五段停留各记录当前Pawn／world-generation与原始QPC区间，原有普通掩体读取完成后直接return，不到后面的原物理查询、抓边或武器操作。已知pause/B失败不在此切片重跑。原基本runner仅此执行体的期限由3600收窄180秒，仍走原正常stop和清理，不强杀其他会话。离线实际operator变换全部compile通过，三个变换体未执行动作；另核对plan的结构化argv与入口TICKET原生路径精确一致，避免旧b144手写斜杠失配。

新增离线eye_trace_inspect读取与生产者完全同一冻结header／MSVC ABI（Header48字节、Row16120字节、容量512）。39项实际CPU检查0，验证版本／容量／计数／时间、截短／尾随字节及输出不被失败覆盖，再用显式synthetic CPU文件完成真实工具读回；不把合成Scene292当61492或新游戏事实。工具列出原Scene／线程／身份／epoch、头／眼点／本层输入输出方向与位置，并检查重算眼点及prepare原点残差；只给CPU括号比较资格，GPU／实际双眼／头显接受始终false。

运行收尾先恢复config，随后将本次新眼点bin原字节复制／SHA核对到本候选独立runtime目录，写回执后才移除已证明是本次产生的共享原文件；格式／语义解析失败不能跳过config恢复，也不丢原始字节。必须五段各有当前Pawn／world-generation、begin/end完全落在相应QPC区间内的合格行，不能用最近时刻或同Actor旧记录配对；满额或漏段不当通过。以上入口组合及顺序已作离线检查，真实正常／失败收尾尚未运行。原10项raw基线外，明确增加eye-bin原始存在／不存在的恢复责任，共11项。

本次五身份：prepared0905884d8c7145528598b404e4d31e2f49d0b84e1a5e9cdf1f20e9c587b071b9；planb4f0155935a958e555520ec3a6a72992b8a619557487e1eb017b206c1e35aa83；ready62b5e59027c852c8d22c21dbd64d68b0b53f46fea7eeac9b514c1e7875e0b4c0；entry6709dc5681294dd34c2a26c2e5fa549838be0be0238d4f519e980ed80f91239e；depsff5bd4b7a22a165acafa171de358a18a19bda7fb9e01854e49878e043169c32b。唯一tail-intent-packet SHAa7df6d25b516f875c442efdbc4b08322ab0cee7bc2f22245c7ebe7ecd2f4e3a0已按必要直接交接送Beam8队尾，回执68f872f9347be7778dbf168af8b930ac7fd6385d8e20d8c30214c74dd3fd3029仅确认登记。HB→LS→GN→Frag→Bol→Beam8→Cover顺序保留；对方未独核此source／entry／deps，Beam8实际outer尚未固定，未正式受理／无当前窗／无最终票据，不借旧HOD49912取消例外或旧final授窗。没有可确认本方live运行handle，不称verified wait。

证据analysis/cover-system-20261108/view-alignment-r80/verification.json，六份本轮代码在source-checkpoint保留原字节；原b144已消费失败、所有历史失败均保留。按本次主人汇总请求另写analysis/coordinator-progress-20261110-0305/cover.json，不为汇报新增测试或构建。本轮新检查与准备均服务此目标，没有游戏／XR／录制或新增玩家动作通过。仍在完整任务步骤1，严重肩／身体遮挡FAIL，核心抓边／碰撞／占用／受击枪口／恢复／逐点GrabPose和完整CV24／AC22、头显均未完成；A20/B30/C20/D15/E15各档.25、工程25%保持。下一玩家检查仍是实际单次入掩体的同调用几何与双眼关联，定位后修复，不将新封套或CPU数据当修复完成。

## 2026年11月10日 r81原命中形状归属只读模块／未接Native

上一轮r80候选、离线读取工具及队尾意向属于进展；本轮没有重封r80或重复其准备检查，在未取得正式交窗时推进CV16必需的形状／刚体归属。REA直接复用指定PhysXCore.dll，完整SHA22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48，Ghidra12.1.4只做静态分析，关闭会话后保存612909字节／77条完整证据。没有执行DLL或目标游戏。

原八类NpBox/Capsule/Convex/HeightField/Plane/Sphere/TriangleMesh/Wheel主表的virtual+8都指向13E730，完整5字节488b4138c3确实读取公开shape+38。RTTI／CompleteObjectLocator关闭到每类主表，offset均0；getActor地址的12项数据交叉引用中，只采用8项实际匹配RTTI的主表，其他代码／调度数据位置未误当形状表。NpActor主表2330A8的实际RTTI也已确认。NpBoxShape构造明确含+18的NpShape副基，不把副基地址直接解释成查询返回的公开shape。内部BoxShape／CapsuleShape表virtual+40共享C4E0，完整8字节488b81a0000000c3读取internal shape+A0。原r73 A71F0已保存指令A7C8A调用此虚槽、A7C8D将RAX写Hit+8，和当前相同原文件对应；没有根据别版SDK猜getActor槽位或把Hit首8字节当shape。

新增cover_physx_shape_owner.h提供observe纯读取：调用方须保有原精确Image pin和当前C0 owner，接口只收当前source Snapshot及原64字节Hit，不产生许可。关闭重取原BodyData／Actor／Scene来源；按已认证8主表及两个完整getter字节读公开shape→actor，要求同当前Scene，并核对shape+30→internal shape、internal shape+A0→公开shape、internal shape+D0与公开actor+40相同，以及internal actor+10→公开actor。前后独立读取的身份／指针／类别一致，最终guard后还比较调用方source和原Hit未变。未知表、未认证getter、任何读失败／链接不符／来源改变均返回明确失败，输出保持；不会将未知或其他同Scene actor自动过滤，也没有“只排除首个self hit”逻辑。

这只是当前字段和关系的CPU读取括号，不是引擎原子快照、shape生命周期、ABA排除、物理世界fence或完整碰撞几何。内部shape虚表只校验模块内地址及实际+40 getter，未据此宣称所有内部几何类型／世界覆盖通过。既有mesh BodyData来源只认证当前选中身体及其数组，尚不是包含人物胶囊、所有骨身体、其他组件与装备的完整自身集合。本模块暂未接入Native查询／决策，不授过滤、无碰撞、终点占用或身体移动许可。

单个聚焦程序在独立冻结r79源加本模块／测试和原PE fixture，/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0，202检查／0失败，8实际项目依赖全部位于冻结树并逐SHA。真实原PE仅映射为数据；对象／Scene／双向关系是明确合成fixture。覆盖八个真实主表、其他同Scene actor仍完整返回、15种表／代码／链接／来源错误、全部70个读点失败、6处owner guard失败、两次各自一致却已换actor、最后读／最后guard修改输入及非法Hit指针，所有失败保持输出；进程确认PhysXCore.dll仍未装载。首版因在旧fixture全局base／size之后include新header触发C4459/WX拒绝，保留原冻结与日志，改为新header首include并独立final树重编，没有关警告。未重复原125项旧suite，也未启动新Native／PG编译；未接线的头文件不称生产Native集成编译通过。

证据analysis/cover-system-20261108/physx-shape-owner-r81/shape-owner-certificate.json、rea-evidence.json及verification.json；证书明确分观察、推论和未知。r80原5身份／Native8035与Beam8后序意向不变，当前未正式受理或获窗，本轮无游戏／XR／录制／新玩家动作通过，也没有助手／CUA／其他武器实现或publicv71变更。下一步完整自身集合及同原查询事务的归属消费仍须实现并认证，随后与组合身体路径、终点占用和physics-hit-muzzle同姿态接线；严重肩／身体遮挡仍待r80取得同调用几何与实际双眼后定位修复。完整CV-01–24／AC-01–22、逐点GrabPose／恢复与实体头显仍未完成，工程25%保持，目标active。

## 2026年11月10日 r82命中归属接入原查询事务／完整Native编译

上一轮47ed1125的形状归属读取属于进展；本轮将它接进现有原查询事务及当前C0观察入口，不另起一次查询追认旧结果。新增AllSweepOwners保留完整原AllSweep及按相同索引排列的actor归属。原方法正常返回、callback关闭、数量／截断与原来源关闭检查通过后，在同一个active query及原Image pin期间逐条读取归属，最后再关闭原来源和输入快照才发布。任何归属未知／读取失败／链接不符均整包失败，原结果不被部分写入；未删除自己或外部actor，不能把包名或零命中解释为完整自身排除、无碰撞或终点占用。

QueryEngine在生产中固定别名Queries=原Image+OriginalCalls，配置不能选Host替身；原方法的SEH、flags及参数由原适配函数继续负责。仅为执行同一事务的离线验证，测试明确代换Image和原dispatch，来源、完整code byte检查、真实流收集、r81归属读取、最终closing均执行实际生产事务代码。新owner_read／owner_unsupported／owner_links明确区分归属失败；原fault_unknown仍保留原模块／资源和uncertain，禁止自动关闭／复位。普通只读拒绝不伪造成原方法未返回的锁债。旧裸流接口继续只返回原始流，不隐式过滤或追加归属成本。

Session／C0增加Box与Capsule各一个带归属的sweep_all重载，现有八个观察API均维持同Entry地址、当前captured／未prepared C0、原Core和100ms来源、metadata、Link／slot、owner线程、无Kernel pending及原stop屏障。工作区留在既有串行对象：Engine持raw流和归属数组，Session持较大的组合结果，不在游戏线程额外建立约139KB结果临时量；也去掉裸流外层的一份重复结果。busy／active在归属读取期间仍有效，嵌套调用和worker提前close被拒绝，原调用期间没有新增私锁。GEARS编译／运行开关的默认关闭策略未改；Goal policy仍未调用查询，原no_cover身体写入拒绝保留，没有新Hook／相机／骨骼写入或输入链。

完整独立v3 Native复用r79不可变树，仅本批5个产品文件overlay，1089源文件及全部原GEARS flags不变。实际编译链接0，DLL SHA8eb0bc2a8cb9277ea466c322ae2d2cbb0dec28c329bb0d311da2a3e3731d0bd1；672实际依赖／392冻结项目依赖逐SHA，无外部工作树依赖。两种C0带归属重载在真实ON构建内实例化；这证明生产接线可编译，不是游戏中的原查询通过。Native仍沿原/W4／EHsc及既有警告设置，不称全Native /WX；本批模块未出现新警告。

同一v3冻结源的六个聚焦程序均/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Owned事务74检查／0，Stream40／0，Session67／0，Raw130／0，Owner202／0，C0 OFF20／0；实际项目依赖并集55项。Owned调用真实QueryEngine，只将模块装载和方法dispatch改为Host：一次原查询保留self／foreign两actor、全部1024最后一条归属、Box／Capsule、空流清旧数据、逐归属读取期间pending／拒绝close和重入、未知表／坏链接／读失败／源变／代码变／计数不符／截断／原fault及错误线程均有检查；失败整包不发布、不重跑原方法。Session使用独立已说明Host engine，覆盖大包接线、源变／截断保留输出与drain；Raw／Stream进一步执行生产OriginalCalls转发到合成x64函数，确认原5／10参数和SEH未变。原DLL仅作实际PE数据，测试进程未装载或执行原DLL，所有输入对象关系仍是合成，不能升级为原游戏／当前物理证明。

首轮单元编译因测试guard只给typed而缺原capture需要的plain接口拒绝，补成与实际SourceGuard相同的两接口。下一轮组合fixture在新header的base局部名触发C4459/WX拒绝；修名时我误将三个std::array::size调用一起替换，第三份冻结也被编译拒绝。已恢复这三个调用，只保留参数明确命名，另建v3完整源后六程序全绿。前三版源／日志独立保留，没有关警告、覆盖失败或在失败输入上反复启动游戏。

证据analysis/cover-system-20261108/physx-owner-stream-r82/verification.json及v3-native／unit源、构建与实际依赖回执。本轮未stage／游戏／XR／录制，无新增玩家动作通过。r80五身份及Native8035保持不变，仍仅有Beam8后序意向、未正式受理／获窗；公共v71、旧b144已消费失败均保留。此包的每条归属仍只是读括号，不能提供shape生命周期／ABA排除、共同物理世界fence、完整自身集合或所有形状覆盖；人物胶囊与所有身体／组件／装备自身集合、组合／形变路径及终点占用、physics-hit-muzzle同姿态、Native实际移动恢复、严重肩／身体遮挡、逐点GrabPose、完整CV24／AC22与实体头显继续未完成。工程25%不变，目标active。

## 2026年11月10日 r83完整声明Bodies清单接线与Native编译

上一轮按主人请求核对需求、补充分级与原文索引属于文档进展；本轮继续CV16所需的自身来源读取，将PhysicsAssetInstance声明的Bodies数组中全部BodyData纳入同一次原查询。范围严格限定为该数组：每行保留当前Body身份、空BodyData或实际actor、原NpActor主表、Scene和内部actor反向关系；共享actor只在独立actor列表按首次出现去重，不合并或丢弃Body行。BoneSpring仅有不透明Pointer字段证据，没有把它解释成另一个刚体。空BodyData只表示这个命名字段为空，不等同组件没有其他物理来源。

新增cover_physx_asset_actors.h在当前线程、当前原Image pin和现有C0 owner内，前后关闭原来源、两次完整读取清单并核对typed guard，最终检查输入未变才发布。现有Session SourceGuard通过真实反射角色样本关闭全部Body的类、OwnerComponent、BodyIndex、BodyData及原来源对应；匹配器核对空行、去重顺序和干净尾部。未知actor表、错误Scene或内部反向关系、读取失败、来源改变和所有owner拒绝均保持输出。工作区保留在串行Engine成员，不在游戏栈创建整份大结果。

AllSweepOwners新增asset_bodies；带归属查询在原方法之前取得清单，方法正常返回并完成每条命中归属读取后重新取得并要求一致，最终再关闭原来源和输入后整包发布。缺少typed清单guard或初始非根BodyData未知时，在原调用之前拒绝；原调用后非根项变更则整包拒绝，不重跑原方法，不过滤任何命中。裸AllSweep路径仍不做清单读取。原fault_unknown保持原pin与uncertain，普通只读拒绝不制造锁债。原C0八个观察入口、运行cover_queries默认0及身体Goal拒绝掩体写入保持，没有新增身体／相机writer、Hook或输入。

独立v1冻结r82 v3源，只有4个产品overlay，共1090文件；原GEARS flags逐项不变，GEARS_COVER_BODY_PROTOTYPE和GEARS_COVER_PHYSX_QUERIES均ON。完整Native实际编译链接0，DLL SHA26a97c95270bef68561cb9808c317a2780b2a49a4a71b953a28eb8e891c9833a；673实际依赖／393冻结项目依赖逐SHA，无冻结树外的工作树源码依赖。Native沿原/W4及既有设置，日志37条原有编译警告、本批模块0条；不称整Native /WX。

同一冻结源的8个聚焦程序全部/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译执行0：Owned84／0、Stream40／0、Session69／0、Raw130／0、Owner202／0、C0 OFF20／0、Roles257／0、Asset86／0，实际项目依赖并集57项。新增检查覆盖64行和末项根、共享actor、空字段、非根actor更换、未知主表／错误Scene／反链、每个读取及guard边界失败、过期成员、最后guard修改输入、错误线程、输入／输出别名和超容量拒绝。Owned检查一次查询中的清单关联、typed拒绝发生于原调用前和方法期间变化整包失败；Session实际执行反射SourceGuard与清单读取器，但仍用明确Host替代原engine dispatch。原DLL仅作PE数据、没有装载或执行；对象关系均为合成，没有新增游戏物理证据。

证据analysis/cover-system-20261108/physx-asset-actors-r83/verification.json及v1源、单元／Native构建和实际依赖回执。上述读括号仍不是完整玩家自身集合、actor生命周期、ABA排除或共同物理世界fence。已核对原人物CylinderComponent读取仅证明它等于CollisionComponent以及尺寸／变换；尚无它的物理BodyInstance／actor或确实无该物理来源的证据，不能把本数组当作胶囊、所有组件、附件及装备的全集。下一步继续从已保存字段及原消费者关系核对这部分来源，再与组合身体体积路径、终点占用、physics-hit-muzzle同版和实际移动恢复合流。

本轮未stage／游戏／XR／录制，无新玩家动作通过。r80原五身份／Native8035未改，仍无本方正式受理或当前运行窗，不把队尾意向称verified wait；公共v71及b144已消费失败保留。严重肩／身体遮挡仍FAIL，三方向实际LGrip闭环、完整世界／形状覆盖、身体移动／受击枪口／恢复／逐点GrabPose及CV01–24／AC01–22与头显尚未完成。A20/B30/C20/D15/E15各档.25，工程25%不变；目标active，本轮progress。

## 2026年11月10日 r84独立人物胶囊BodyInstance字段来源

上一轮8bf7fe3a将完整声明Bodies清单接入同一原查询并完成Native编译，属于进展。本轮推进其仍缺的人物胶囊来源，先核对现存原包与字段消费者：原Engine.u SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510；原导出库存中PrimitiveComponent.BodyInstance为ObjectProperty，原48字节导出SHA80467a6b381e530e6e288893d1fcaeab873157cfa7e5188848bb3e91aa7de395。尾部序列化引用值23193对应同包RB_BodyInstance类导出；该引用解释保留为包格式推论，不当作当前进程指针或内存偏移。原CylinderComponent类导出及已存解包声明对应PrimitiveComponent继承关系；Actor.CollisionComponent及RB_BodyInstance类导出也逐字节SHA核对。四份原字节与路径保存在package-source-witness.json，未执行原包／脚本或读取游戏进程。

新增cover_collider_roles_live.inl复用现有binding与r76完整字段成员检查，在调用方当前C0 owner guard内读取实际Pawn的命名CylinderComponent与选中CollisionComponent，要求二者相同、实际Cylinder派生类型、外属及Owner均对应当前Pawn。独立读取继承的BodyInstance ObjectProperty及其声明RB_BodyInstance目标；存在时再认证实际body类型、OwnerComponent确属所选Cylinder，记录BodyIndex与命名BodyData／BoneSpring Pointer字段。BodyInstance及Owner字段位置从当前反射布局解析，不套用历史固定偏移；BodyData88／BoneSpring90沿既有原消费者证据精确关闭。

输出显式区分未采样、BodyInstance字段空、BodyData字段空、BodyData非空四种状态。空字段只描述该字段，不认证组件没有物理体；非空BodyData也只记指针，不冒充合格NpActor。BodyIndex原样观察，未假设独立组件必须是某个PAA行号；BoneSpring保持不透明，不加入actor列表。实际对象／字段读取两遍、所有反射与注册身份关闭通过，最终owner及输入键一致后才发布；任何布局／归属／读取变化均不部分更新输出。元数据锁使用try-lock，并在再次调用owner guard前释放，没有引擎方法或身体写入。

本模块当前为可编译的字段来源模块，尚未接入Session／C0查询或AllSweepOwners清单，未改原8观察API、运行默认或Goal拒绝掩体写入。需要下一步在同一原Image／查询Scene下认证非空BodyData的实际actor关系并合入同一查询事务；不能将分离观察的字段与另一时相命中合并签发自身排除或移动许可。本轮没有为未接线模块重建整Native／PG／XR；上一已编译r83 Native26a97c95仍只是其原有范围。

独立final冻结r83单元源，仅增加本模块及聚焦程序，18实际项目依赖全部来自冻结树并逐SHA。两个程序/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：启用212项／0失败，关闭1项／0失败。执行的是实际反射、注册表及当前guard代码，内存关系为合成fixture。覆盖三种已采样字段状态、保留-1 BodyIndex及不透明BoneSpring、动态BodyInstance偏移、24种类型／Owner／声明引用／字段宽度／指针／兄弟循环／注册失效／Default对象错误，以及读取中BodyInstance消失、字段／归属／元数据代次改变和读取异常。拒绝guard、最终owner拒绝、最后guard改输入、错误线程、锁竞争、无效context与工作区别名均保持输出；关闭时没有读取／guard／原调用。它不是实际游戏C0、原物理消费者或动作通过。

首版合成fixture在physics_setup已绑定旧schema后追加Pawn属性，复用了旧base proof；205检查中38失败首先指向layout_changed。没有放宽产品guard，改为在扩展夹具完成后重新建立测试自己的binding，v2为205／0；随后新增动态偏移与无效context边界，final为212／0。三份冻结输入及首败日志保留，不覆盖失败。REA会话查询为closed，本轮未打开原二进制会话；复用既有包声明和保存字段证据，不新增分析进程、助手、CUA或游戏观察器。

直接运行尾状态的一次compact核对仍只见Beam8→Cover-r80登记意向，无正式受理／授窗；该同一完成消息和idle状态不算本方verified wait，也不消费r80。证据analysis/cover-system-20261108/collider-body-source-r84/verification.json。本轮无stage／游戏／XR／录像／新玩家动作通过，公共v71及r80五身份保持，旧b144已消费失败保留。完整自身集合、全部形状／世界覆盖、共同物理时相、组合身体路径及终点占用、实际身体移动与恢复、physics-hit-muzzle同版、严重肩身遮挡、三方向LGrip与逐点GrabPose及CV01–24／AC01–22、实体头显均未完成。工程25%不变，目标active，本轮progress。

## 2026年11月10日 r85胶囊BodyData同Scene认证与查询事务接线

上一轮00d041b1完成声明胶囊BodyInstance字段来源及合成检查，属于进展。本轮将它接入现有带归属的原查询事务及真实C0 SourceGuard。新增cover_physx_collider.h关闭当前原查询来源、组件的命名BodyInstance字段、BodyData88／BoneSpring90，再对非空BodyData核原NpActor主表2330A8、与本次查询相同的Scene、内部actor指针及反向公开actor关系；两次读取一致、typed owner和最后输入关闭后才发布。该原表及关联沿r73／r81已有精确PhysX证据，未新增调用方法或用别版SDK推断ABI。

胶囊来源携带Pawn／Cylinder／RB body地址、实际反射BodyInstance字段偏移、原BodyIndex和BodyData／BoneSpring；组件字段偏移从r84当前反射读取带入，没有另猜固定位置。结果分别表达BodyInstance为空、BodyData为空和已观察到同Scene actor，BoneSpring仍不透明。空字段不证明组件没有其他物理来源；非空actor可以与PAA body共享，也可以是独立同Scene actor，不因为两者不同就丢弃。字段／表／场景／反向关系不符、读取失败或输入变化均整包拒绝。

QueryEngine现有带归属sweep_all必须收到显式胶囊来源及对应typed guard；缺少来源／guard在原调用前拒绝，不降级为裸流。原查询前分别关闭完整声明Bodies清单及胶囊来源，方法正常返回、原callback与全部命中归属完成后重新读取两者并要求一致，再做最后原来源／输入关闭才发布AllSweepOwners。原输入在前置新增读取期间变化，也会在原调用之前拒绝。新增collider_source保留独立来源，raw hit顺序和数量不变，没有过滤self／foreign。裸AllSweep继续不读取胶囊；原fault_unknown仍保留pin／uncertain，普通读取拒绝不伪造原方法锁债。

Session在既有busy／drain屏障内预读当前类型化胶囊；仅带归属输出的SourceGuard追加r84全身份、Owner／OwnerComponent、继承属性和字段前后核对，并把compact来源交给同一个Engine。接线复用原八个C0观察入口，没有新增Hook、输入、骨骼／相机写入或另一次原查询。大结果和反射工作区仍为串行Session／Engine成员；最终SourceGuard及原Image pin在当前事务生命周期内保持。编译宏按原私有配置ON，但运行cover_queries默认0及Goal的no_cover拒绝保持；任何结果都没有授予身体移动。

完整v1 Native从r83不可变源仅4份产品overlay冻结，共1092源文件；所有原GEARS flags逐项不变。实际编译链接0，DLL SHA fdece8a589ae69512aac0250db52f7781b2d4df48cb135bd0dd8f466ee4660e0；675实际依赖／395冻结项目依赖逐SHA，无冻结树外的工作树源码依赖。本批模块没有新增编译警告，原37条警告保留；Native沿原/W4／EHsc等设置，不称全Native /WX。未重建或替换PG／XR／公开包，r80原五身份／Native8035不变。

聚焦程序均/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Owned93／0、Stream40／0、最终Session76／0、Raw130／0、Owner202／0、C0 OFF20／0、Roles257／0、Asset86／0、新Collider PhysX88／0、Collider Roles219／0、Collider Roles OFF1／0。首批10程序的项目依赖并集61项；增加Session两种空字段贯通后仅重编该程序和本批受影响的Collider OFF，其他未变绿色结果复用，原Session74／0记录保留。原DLL仅作为实际PE数据；模块装载／原dispatch明确为Host或合成函数，Source、code字节、流收集、物理关系和反射读逻辑执行实际实现；关系内存仍是合成，未调用原DLL或游戏。

新检查覆盖同Scene共享／独立actor、两种不同空字段、非根字段／胶囊BodyInstance在查询期间变化、当前类／字段映射匹配、每个读取／guard边界失败、错误Scene／内部反链、来源最后guard被修改、错误线程和别名拒绝；Session使用实际新增SourceGuard与两套读取器，仍明确替代原engine dispatch。关闭状态没有读取／guard／原调用。所有失败输出保持，未知源不变为空结果。编译和合成输入不能替代真实C0时限／查询时相、actor生命周期、ABA排除、世界fence或最终像素。

证据analysis/cover-system-20261108/collider-query-r85/verification.json及v1 Native／单元、final单元的源码／实际依赖回执。本轮未stage／游戏／XR／录像，无新玩家动作通过，没有新运行授窗；沿用r84最后定向核对的仅后序意向状态，不称verified wait，也不消费旧b144或r80。当前清单只覆盖声明的PAA Bodies与当前所选Cylinder.BodyInstance，其他PrimitiveComponent独立BodyInstance、附件与装备、全部原形状／世界覆盖仍未认证；尚不能据此构造完整自身过滤或把空流当占用安全。

下一步继续闭合其余组件／装备的自身来源及原查询完整覆盖，并与组合身体路径、终点占用和同版physics-hit-muzzle合流；实际移动恢复、严重肩身遮挡、三方向真实LGrip、逐点GrabPose、CV01–24／AC01–22与实体头显仍未完成。A20/B30/C20/D15/E15各档.25，工程25%不变，目标active，本轮progress。

## 2026年11月10日 r86人物与当前武器组件字段清单

上一轮17e0733f将胶囊actor同Scene关系接入同一原查询并完成Native编译，属于进展。本轮继续其仍缺的其他组件来源。原Engine.u保持SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510，按原导出库存逐SHA核对Actor.Components／AllComponents及两份ComponentProperty inner，还有Actor.Attached与SkeletalMeshComponent.Attachments，共6导出。两inner的末端序列化引用均对应971 ActorComponent类；该包格式对应关系不作为当前内存偏移或完整附件语义。原字节及库存身份保存package-witness.json，没有重新执行原包、脚本或游戏。

新增cover_component_roles_live.inl，在调用方当前C0 owner guard和既有binding元数据环境中，读取当前Pawn及已知当前Weapon两个根的WorldInfo、Components与AllComponents。无当前武器时只保留Pawn根；未知武器状态拒绝。各数组依据当前反射的ArrayProperty／ComponentProperty inner／ActorComponent声明类及完整字段成员关系读取，不猜历史字段偏移。每张数组最多64行、capacity最多256，合计最多128个唯一组件，组件派生布局最多8类；超过边界整份拒绝，没有截断成功。

原数组的顺序、空项及重复指针逐行保留；独立组件表只按首次出现去重，另记录Pawn的已知Mesh是否在声明列表中。空清单和未列出Mesh均作为范围事实，不当成没有自身物理。逐组件关闭实际身份、ActorComponent继承链及Owner；记录Owner究竟匹配Pawn、当前Weapon或两个根之外的Actor，不能因为出现在某根数组里就自动当self。Primitive／Skeletal分类来自关闭的完整类链；非Primitive标签只表示该类族，不证明无其他物理来源。

每个Primitive组件继续读取其当前BodyInstance声明及实际RB_BodyInstance、原BodyIndex、OwnerComponent、BodyData和不透明BoneSpring，区分空BodyInstance／空BodyData／非空指针。实际身体OwnerComponent及其Owner Actor单独保存；允许它与列表组件的Owner不同，避免把共享RB身体强行改写归属或误排外部障碍。两条Owner的根归属分别记录，尚无过滤。身体字段解析复用r76的同一body_layout，仅把其Workspace参数泛化为同字段的调用方工作区；原Bodies读取仍保留自己的行号及Mesh归属要求。

根WorldInfo及两数组字段、Primitive Owner与BodyInstance、四个身体字段均检查存储区不重叠。全部根、列表、组件、身体及对应元数据前后关闭，再检查最终guard与输入键后才发布；失败保持调用方原输出。大目录候选留在既有模式的成员工作区，values直接填候选，逐行清理尾部，没有在游戏栈构造整份目录。工作区重置只清有效计数、键和小字段，每次重新读取选中源，未激活的scratch不被消费或发布。

本模块尚未接入Session／C0查询或AllSweepOwners，也尚未将这些BodyData逐项认证为当前Scene的物理actor。Actor.Attached及骨骼Attachments、除已认证主Mesh之外的额外PAA Bodies、其他原生物理来源、actor生命周期／ABA与共同物理时相继续单列；不能将两张声明数组当完整玩家自身集合，不能把空清单当路径或占用安全。没有新增Hook／输入／写入者／观察进程或新运行入口，原八C0观察API和运行cover_queries默认0、Goal no_cover拒绝保持。

独立final-v5源的两个聚焦程序/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Components207项／0失败、OFF1／0；21实际项目依赖全部在冻结树并逐SHA。覆盖空项、重复、两根与已知无武器、外部Owner、跨组件共享身体、128唯一组件及末项、129拒绝、9种派生布局拒绝、动态BodyInstance偏移、空清单清旧结果、21类错误布局／身份／Owner／字段／数组／别名，以及读取中数组／Owner／代次改变、读取异常、guard失败、错误线程和锁竞争。全为合成注册表和内存上的实际读取实现，没有游戏或原actor执行。

首版在旧fixture全局inner／actor之后include新header，触发C4459／WX拒绝，改用明确局部名而未关闭警告。V2编译通过后退出3221225725（0xC00000FD），没有测试文本；新代码含一次清空8份大Proof的聚合重置；改为上述有效范围重置后，同一默认栈运行通过，未加大栈。V3为185／0；增加字段重定位及空清单得到V4的189／0，再补字段存储重叠拒绝得到最终207／0。各冻结版本、编译／退出回执与失败保留，未用失败版本运行游戏。

共享helper变化另以实际依赖回归编译执行：Roles257／0、ColliderRoles219／0、Session76／0。完整Native只覆盖r85不可变源中的共享cover_physics_roles_live.inl一份产品文件，1092源文件及原GEARS flags不变，编译链接0；DLL SHA03aa8d693ac262cb9d67de3be12f328549d0370906b159e08eebc821fa6c708b，675实际依赖／395冻结项目依赖逐SHA，无冻结树外工作树源码依赖。本批共享helper无新增警告，原37条保持。新目录模块不是此次Native的实际依赖，不把这个Native构建说成新清单已接入游戏；其实现只由上述聚焦程序实例化验证。PG／XR未重建，公开v71与r80原五身份／Native8035保持。

证据analysis/cover-system-20261108/component-sources-r86/verification.json及原包见证、final-v5源、回归与Native实际依赖回执。本轮无stage／游戏／XR／录制／新增玩家动作通过，也未取得或使用新运行窗，不称verified wait。下一步将组件清单的实际身体来源与现有同Scene查询事务合流，并继续额外骨身体及附件证据；完整自身过滤、世界／形状覆盖、组合路径／终点占用、实际移动恢复、同姿态physics-hit-muzzle、严重肩身遮挡、真实三方向LGrip／逐点GrabPose、CV01–24／AC01–22与实体头显未完成。工程25%保持，目标active，本轮progress。

## 2026年11月10日 r87声明组件身体同Scene认证接入查询

上一轮2b7e48fd完成组件／独立身体字段清单及聚焦检查，属于进展。本轮将其接入现有C0带归属查询，仍不授完整自身过滤或身体移动。新增cover_physx_components.h用紧凑来源保存两根实际Actor、两张数组的反射字段偏移／完整header／成员顺序，以及每个唯一组件、组件Owner、实际身体OwnerComponent及Owner Actor、两套根归属、身体字段状态、BodyIndex、BodyData和不透明BoneSpring。原字段身份与类型继续由r86完整反射读取认证，紧凑包不重解释Owner或抹掉外部／共享关系。

紧凑来源要求成员首次出现顺序与唯一组件表完全对应，空项／重复保留，未用根、数组尾和组件尾干净；两套Owner根掩码必须对应实际根地址，非Primitive／空BodyInstance／空BodyData的状态字段相互一致。数组字段位置不重叠，数组计数／容量有界，已知Pawn Mesh是否列出也与本次查询来源复核。超过2根／每表64行／128唯一组件、未知状态、隐藏尾或不完整清单均拒绝，没有截断成功或静默跳过。

在同一已primed原Image pin、当前C0 owner与原查询来源中，读取所有声明数组header及完整成员，复核各Primitive的BodyInstance／BodyData88／BoneSpring90。每个非空BodyData必须是当前原NpActor主表2330A8，属于与本次查询相同的Scene，内部actor与反向公开actor对应；前后两次均一致、typed guard及最后输入关闭才发布。每个组件的结果状态保留；独立actor副表仅按首次出现去重，不把外部Owner或共享actor过滤成self。非Primitive标签和空字段只描述此次声明，不证明无其他物理来源。

QueryEngine的AllSweepOwners新增component_sources。带归属查询必须传入显式组件目录及对应typed guard；缺少或无效来源在原调用前拒绝。原方法前分别认证原PAA Bodies、所选胶囊和声明组件身体，原方法正常返回、callback与全部hit归属读取完成后再认证三者并要求前后相同，最后原来源与输入关闭才整包发布。目录key、读取候选及大结果保留在串行Engine成员；新目录Workspace另有重入屏障，nested读取不能改写外层key。原fault_unknown保留pin／uncertain；普通读拒绝不制造原方法锁债。裸流不读目录，也不隐式过滤。

Session在既有busy／stop／worker-close范围内预读r86完整类型化目录并生成紧凑源；C0现有八个观察API复用原Entry／Core／metadata／Link-slot／线程／100ms／无prepared或Kernel pending检查。完整目录在类型化物理目录检查点及最终发布前复核，不为每个hit重新遍历全部组件；原PAA／胶囊／C0 plain guard不弱化，不刷新输入原时间。最后原方法返回之后即使目录Owner才改变，Session也拒绝对外发布。没有第二次原查询、输入／Hook／身体或相机writer；cover_queries运行默认0和Goal no_cover写入拒绝保持。

首批12个聚焦程序在同一v1冻结树/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译执行0：Owned102／0、Stream40／0、Session81／0、Raw130／0、Owner202／0、C0 OFF20／0、Roles257／0、Asset86／0、Collider PhysX88／0、Collider Roles219／0、Component Roles207／0、新Components PhysX100／0；实际项目依赖并集65项。另在同一源码补编Component OFF1／0、Collider OFF1／0，均无读取／guard／原调用。本轮首版即通过，没有覆盖既有r86编译／栈溢出等历史失败。

新程序覆盖所有读取及owner边界失败、两种空身体状态、非Primitive状态、空目录清旧actor行、128行末项、共享／独立同Scene actor、外部Owner、未知表／错Scene／反链、目录成员变化、伪造根掩码／隐藏尾／不完整顺序、最后guard改输入、同Workspace重入、错误线程和别名。Owned实际执行同一QueryEngine事务，检查一次原query保留完整raw顺序与外部actor、目录在方法中改变整包拒绝，以及缺目录／typed拒绝发生在原调用前；只将Image装载和原dispatch替成明确Host。Session使用实际r86读取、紧凑转换及新增SourceGuard，原engine dispatch仍为Host，覆盖成员／Owner改变、晚于目录认证的Owner变化和原drain；并非游戏原查询。原DLL只作实际PE数据，所有对象关系为合成，没有装载或执行原DLL。

完整Native从r86不可变源仅4个产品overlay冻结，1094源文件、原GEARS flags逐项保持。实际编译链接0，DLL SHAffe20d7d70bd11f9f7579de89e527a7a493fbc1f641c18a1ec8c8c9ab7cdc233；677实际依赖／397冻结项目依赖逐SHA，无冻结树外工作树源码依赖。新组件目录及物理认证头均为此Native的实际依赖，Box／Capsule的C0带归属重载已实例化；这证明启用路径编译，仍不是原游戏调用通过。原37条编译警告保留，本批模块0新增；不称全Native /WX。PG／XR未重建，公开v71／手动入口和r80原五身份／Native8035均未替换。

证据analysis/cover-system-20261108/component-query-r87/verification.json及v1源、单元／OFF／Native实际依赖回执。本轮无stage／游戏／XR／录像／新增玩家动作验收，未取得或使用新的运行授权，不称verified wait。当前仅关闭声明组件的独立BodyInstance；Actor.Attached、骨骼Attachments、其他SkeletalMeshComponent的额外PAA Bodies及其他原生来源仍未覆盖。重复读不是actor生命周期／ABA排除或共同物理world fence；每个actor同Scene不证明完整世界／形状覆盖或同一物理时相。没有把actor副表当完整self集合，也没有把空流签成占用安全。

下一步继续额外骨身体与附件来源、完整查询／同步和真正自身过滤，再接组合身体体积路径、终点占用及实际移动恢复。严重肩身遮挡、同版physics-hit-muzzle、真实三方向LGrip／逐点GrabPose、CV01–24／AC01–22与实体头显仍未完成。工程25%不变，目标active，本轮progress。

## 2026年11月10日 r88其他骨骼组件PAA来源与同查询Scene认证

上一目标轮核对需求并保存阅读导航，属于文档进展；导航单独提交3cd36260。本轮继续r87尚未覆盖的其他SkeletalMeshComponent.PhysicsAssetInstance.Bodies。扩展现有cover_component_roles_live目录，不新增查询、输入、Hook、身体或相机写入端；当前Pawn及已知当前Weapon的声明组件仍是本轮来源范围，数组成员或Owner本身均不授予self身份。

每个实际骨骼组件分别认证命名PhysicsAsset／PhysicsAssetInstance字段及其声明目标、实际实例类型；两字段独立可空。实例存在时认证Owner、Bodies、RootBodyIndex及ObjectProperty inner，记录实际完整Owner身份、字段偏移、数组头、原始行次和根索引。共享实例不强制Owner等于当前组件；可选／禁用组件允许空数组和-1根索引，原主Pawn严格非空／有效根／根BodyData检查不变。各身体保留完整RB_BodyInstance身份、实际OwnerComponent及其Owner Actor、原有符号BodyIndex、BodyData和不透明BoneSpring；不假设BodyIndex等于数组行号。空身体行与重复／共享身体均保留，未采样、非骨骼、实例字段空和实例存在分别表达。

单数组最多64行，组件最多128，额外身体池最多256；容量不足或布局／来源不一致整笔拒绝，不能截断成功。候选、双读目录和反射Proof均为原Workspace／Session成员，没有新增大目录栈临时或扩大线程栈。所有实际字段与注册身份两次读取，反射兄弟成员、声明引用及来源关闭后才发布；每次空目录均清除旧池行。

紧凑来源同时携带各组件PAA及其完整身体行，cover_physx_components在现有原查询Scene范围内重新读取组件资产字段、实例Owner／数组／根、每个成员及BodyData88／BoneSpring90，再核非空BodyData的原NpActor主表2330A8、同Scene、内部actor和反向公开actor关系。独立BodyInstance和额外PAA的全部来源行分别保留，actor副表仅按首次出现去重，最大384；外部Owner也不静默丢弃。沿用r87同一QueryEngine前后来源核对、原Image pin、一次原stream及最终Session目录关闭；PAA来源／成员在方法中或最终发布前改变均整包拒绝。没有把副表解释为完整self集合，也未过滤任何raw命中。

最终v2冻结r87 Native源，仅2个产品overlay，共1094文件。实际编译链接0，Native SHA256为956fb5f292b22d0c31d2dd0806f22a312b457bfb75f9f3bad388bb8bb532c09d；677实际依赖／397冻结项目依赖逐SHA，原GEARS flags逐项保持，无冻结树外工作树源码依赖。原37条警告保留，两个改动模块0新增；不称全Native /WX。cover_queries运行默认0及Goal no_cover写入拒绝保持，未重建或替换PG／XR、公开v71、手动入口及r80原五身份／Native8035。

最终14个程序均以/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译执行0：Owned109／0、Stream40／0、Session84／0、Raw130／0、Owner202／0、C0 OFF20／0、Roles257／0、Asset86／0、Collider PhysX88／0、Collider Roles219／0、Component Roles207／0、Components PhysX100／0、新Component Assets178／0、新Component Assets PhysX151／0；实际项目依赖并集67项，全部来自冻结树。另3个同源关闭程序各1／0，无读取、guard或原调用。第一阶段仅字段读取的13程序绿色回执保留；最终版增加同Scene来源及实际事务回归，不复用旧结果冒充最终验证。

新检查覆盖共享实例／身体和不同实际Owner、独立可空资产与实例、空数组及预留容量、空身体槽／空BodyData、可选-1根、原样负BodyIndex、全部256行及溢出拒绝、错误布局／声明目标／Owner／表／Scene／反链、每次读取和guard失败、数组及来源两次读取间改变、最后guard修改输入，以及查询中和原方法后目录变化的整包拒绝。Owned实际QueryEngine收到raw命中以外的额外PAA actor，仍只执行一次Host原调用并保持原命中顺序；Session实际SourceGuard关闭新增字段。对象／物理关系均为合成fixture，原DLL仅作PE数据，加载和dispatch为明确Host seam，没有执行原DLL或游戏方法。

Native首次构建启动在编译前被来源哈希检查拒绝：派生脚本误将v2清单配到v1源码目录。两份冻结源均逐SHA未变，没有启动编译或运行；保留rejected脚本和native-input-rejection.json，仅更正构建路径后v2成功，未放宽哈希保护。证据为analysis/cover-system-20261108/component-assets-r88/verification.json及对应冻结源码、单元／OFF／Native依赖回执。

本轮无stage／游戏／XR／录像／新增玩家动作通过，未收到或消费新的正式运行窗口，不称verified wait。Actor.Attached、骨骼Attachments及其他原生物理来源仍待闭合；重复读与各actor同Scene不证明生命周期／ABA排除、完整形状世界覆盖或共同物理时相。下一步继续附件来源与完整自身集合／查询同步，再接组合身体体积路径、终点占用及实际移动恢复。严重肩身遮挡、同版physics-hit-muzzle、真实三方向LGrip／逐点GrabPose、CV01–24／AC01–22与实体头显仍未完成。A20/B30/C20/D15/E15各档.25，工程25%不变，目标active，本轮progress。

## 2026年11月10日 r89递归Actor附件来源与同查询认证

上一轮096fab26完成其他骨骼组件PAA来源与同Scene认证，属于进展。本轮继续其未覆盖的Actor.Attached。原Engine.u仍为SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510，逐原导出SHA核对Actor.Base、Attached及ObjectProperty inner，以及骨骼Attachments、其StructProperty inner、Attachment结构和Component成员，共7导出。Actor附件内层为Actor引用；骨骼附件则是含Component及相对变换的结构，不能混按指针数组读取。原字节／候选序列化引用保存package-witness.json，不将原包导出索引或序列化字节当当前运行偏移。

扩展现有cover_component_roles_live目录，以当前Pawn及已知当前Weapon为1或2个种子，对实际命名Actor.Attached做广度优先、首次出现顺序遍历，最多16个唯一Actor。每节点认证完整注册身份、Actor派生类型、当前WorldInfo、Components／AllComponents、Base和Attached的声明及不重叠字段布局；Base允许为空，非空保留其实际完整身份及声明Actor类型，不强制等于遍历父节点。Attached inner必须是8字节Actor ObjectProperty，各数组最多64行、capacity最多256。空项、重复项和循环边原样保留，遍历去重只用于终止循环；新节点超容量或来源不合法整笔拒绝，不丢失后半链再报告成功。

完成全部Actor节点后，沿原组件数组顺序去重采样组件，继续读取独立BodyInstance和可选骨骼PAA。新增节点的身体走同一类型化Owner／OwnerComponent及既有同Scene认证，不新增身体或相机写入。owner_roots／body_owner_roots现在是一位对应一个实际目录节点的地址相等记录；它不是self权限，也不能仅据Attached成员或Base关系排除碰撞。已知空武器只保留Pawn种子；通过Attached另见旧武器也只作为普通Actor来源，不制造当前持物资格。

紧凑来源追加种子数、所有递归Actor节点、实际Base及原附属数组。cover_physx_components在同一个原查询Scene范围内重新读取每节点组件表、Base、Attached头和全部成员，原有BodyData表／Scene／内部actor反链核对继续保留。QueryEngine现有一次stream的前后目录核对及最终Session SourceGuard覆盖新增字段；查询中附件边改变或原方法后Base才变化均整包拒绝。原raw命中不筛除，独立BodyInstance和PAA行继续分别保留，actor副表只按首次出现去重。没有第二原查询、Hook、输入仲裁器或另一套自身观察平台。

16个聚焦程序均/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT实际编译执行0：Owned116／0、Stream40／0、Session86／0、Raw130／0、Owner202／0、C0 OFF20／0、Roles257／0、Asset86／0、Collider PhysX88／0、Collider Roles219／0、Component Roles207／0、Components PhysX104／0、Component Assets178／0、Component Assets PhysX155／0、新Actor Attachments133／0、新Actor Attachments PhysX142／0；实际项目依赖并集69项。另4个同源关闭程序各1／0，无读取、guard或原调用。首版冻结即通过，没有覆盖r88及更早失败记录。

新检查覆盖深层Actor组件及其PAA、重复／空成员、跨种子和回祖先的循环、实际Base与遍历父节点不同、已知空武器时的重新发现、错误World／类型／inner／宽度／布局／数组／Default对象、所有读取和guard失败、节点及组件表在读取中改变、查询中及发布前变更、全部16节点末项及第17节点溢出拒绝、孤立／错序／隐藏节点和清空旧目录。Owned实际QueryEngine关闭递归边和Base，Session实际SourceGuard拒绝晚变更；仅原模块装载／dispatch为Host seam，原DLL只作PE数据，所有UObject及物理关系仍为合成fixture，不是游戏附件或原查询运行通过。

完整Native从r88 v2不可变源仅2个产品overlay冻结，共1094文件；实际编译链接0，DLL SHA38b012fc366250f2dc94306cfc2de8c9b29344848fb707e315aa10f7cd662d5d。677实际依赖／397冻结项目依赖逐SHA，无冻结树外工作树源码依赖；原GEARS flags逐项保持。原37条警告保留，两份改动模块0新增，不称全Native /WX。反射Proof、双读目录和大结果仍为原Workspace／Session成员，递归队列只是16个地址；未扩大线程栈。运行cover_queries默认0、Goal no_cover拒绝及原C0身份／时间／单写入／清理保护保持，PG／XR、公开v71、手动入口及封存r80原五身份／Native8035未替换。

证据analysis/cover-system-20261108/actor-attachments-r89/verification.json及v1冻结源、单元／OFF／Native实际依赖回执。本轮无stage／游戏／XR／录像／新增玩家动作通过，没有新的正式运行窗口，不称verified wait。当前只是递归声明Actor来源闭合；骨骼Attachments结构、其他原生物理来源、完整自身集合、actor生命周期／ABA排除、共同物理时相和完整形状世界覆盖仍未认证。同Scene与双读不能替代这些条件，也不能把空流签成身体占用安全。

下一步按实际Attachment结构及Component声明继续骨骼附件来源，再闭合自身过滤／查询同步、组合身体路径、终点占用与移动恢复。严重肩身遮挡、同版physics-hit-muzzle、真实三方向LGrip／逐点GrabPose、CV01–24／AC01–22及实体头显均未完成。A20/B30/C20/D15/E15各档.25，工程25%不变，目标active，本轮progress。

## 2026年11月10日 r90骨骼Attachment结构与递归组件来源

上一轮582a13dd完成递归Actor.Attached及同查询来源关闭，属于进展。本轮在同一目录内接入SkeletalMeshComponent.Attachments，不将其误读为组件指针数组。原Engine.u保持SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510，8个相关原导出逐SHA核对，保存在package-witness.json；原声明只确定结构与字段用途，不提供当前内存偏移或行大小。

当前反射路径要求Attachments为ArrayProperty，其inner为一维StructProperty，指向声明类拥有的Attachment ScriptStruct；实际行大小由inner和结构extent一致认证，最多128字节。单独遍历ScriptStruct成员并关闭完整链，不把它冒作Class交给只支持UClass的property helper。Component须为8字节ActorComponent引用，BoneName为8字节NameProperty，RelativeLocation／RelativeRotation／RelativeScale分别为12字节Vector／Rotator／Vector；五字段尺寸、范围、不重叠、引用类型及完整身份均认证。每行完整原字节保留，包括骨名、相对变换及填充；不将这些源字节解释为已认证世界几何。

先保持r89 Actor图和原声明组件顺序，再补当前Pawn Mesh显式种子，随后按组件顺序递归发现结构行引用的新组件。主Mesh即使未列在Actor组件数组中仍读取其独立身体、PAA及Attachments，pawn_mesh_listed继续只记录实际数组成员事实。组件队列去重以终止自引用和循环，但原结构行中的空引用、重复引用及不同相对变换均保留。新增组件沿原Owner／OwnerComponent、独立BodyInstance及PAA同Scene核对，不凭附件成员或Owner给予self权限。组件上限128、结构行池上限256、单数组上限64；任一耗尽或来源不符整笔拒绝，不截断成功。

紧凑来源追加显式主Mesh、每组件结构身份／实际行布局／数组头／原始行；原查询事务在同一Scene内重读数组和每行全部字节，前后及最终Session目录关闭覆盖新增字段。Component指针未变而仅骨名、相对变换或填充改变也会拒绝。原raw hit及其归属保持，不新增查询、Hook、输入、骨骼或相机writer。大目录、Proof及结果仍为原Workspace／Session成员，递归队列只含128个地址，没有扩大线程栈。

第一版17个聚焦程序全部编译执行0；补充原事务行内容变更及原始结构程序后，仅对受影响的Owned、Component Assets PhysX和新Skeletal Attachments PhysX做最终冻结重编，其他程序按实际依赖逐SHA复用。最终18个不同程序全部/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT通过：新结构来源193／0、新原始结构150／0、Owned122／0；其余回归结果和冻结依赖见verification.json。五个同源关闭程序各1／0，无读取、guard或原调用。

新检查覆盖递归骨骼与非Primitive组件、循环／重复／空引用行、外部Owner、主Mesh不在数组时的显式入口、56与64字节动态行及0／8字节Component位置、类型／字段引用／继承／重叠／范围／结构大小错误、读取中引用／骨名／相对变换／填充／布局／数组地址改变、全部256行与第257行拒绝，以及所有原始行读取／guard边界失败、孤立或伪造组件、隐藏尾和主Mesh来源不匹配。清空声明数组时仍保留实际主Mesh，清空骨骼附件则清除旧递归节点和行尾。所有对象与物理关系为合成fixture，原DLL仅作PE数据、加载与dispatch为Host seam，未执行原游戏方法。

Native从r89不可变源仅2个产品overlay冻结，1094文件；实际编译链接0，SHA为945e7b3b90fc3aa364563b838ad3e4eb0f797af86c0aeadebe0ea5828bc839f0。677实际依赖／397冻结项目依赖逐SHA，无冻结树外工作树源码依赖；原GEARS flags逐项保持，原37条警告保留，改动模块0新增，不称全Native /WX。PG／XR、公开v71、手动入口及封存r80五身份／Native8035未替换；运行cover_queries默认0、Goal no_cover拒绝及既有C0身份／原始时间／输入与单写入保护保持。

证据analysis/cover-system-20261108/skeletal-attachments-r90/verification.json及v1 Native／首批单元、final单元／OFF实际依赖回执。当前闭合的是声明Actor图、显式主Mesh、递归骨骼结构组件及其字段／同Scene来源；其他原生物理来源、完整自身与形状集合、生命周期／ABA排除和共同物理world fence仍未认证，不能据此签发自身排除或身体占用安全。双读和同Scene不等于运动及原消费者同步。

本轮无stage／游戏／XR／录像／新增玩家动作通过，也未收到或使用新的正式运行窗口，不称verified wait。下一步核对剩余原生物理来源、完整自身范围及查询同步，再接组合身体路径、终点占用、实际移动与恢复。严重肩身遮挡、physics-hit-muzzle同版、真实三方向LGrip／逐点GrabPose、CV01–24／AC01–22及实体头显均未完成。工程25%不变，目标active，本轮progress。

## 2026年11月10日 r91静态Actor误拒绝与Shape集合原路径

上一条排队确认没有源码或验证进展；本轮复核704da34b当前源码，继续原生物理来源与同步缺口。readonly SQLite先查NpActor／getNbShapes无匹配；BoneSpring仅见相关native声明及历史文字候选，不能将不透明BoneSpring认作第二Actor。未改公共包、封存r80或其他武器文件。

REA绑定同一原PhysXCore.dll，SHA22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48，只做静态读取／反编译，没有装载执行原DLL或游戏。原NpActor表2330A8、字段和虚调用入口按实际字节核对。135C00直接判断Actor+40是否非空；13C730的同一字段为空分支明示Static actor moved并处理静态姿态，非空则调用动态Body。现cover_physx_shape_owner.h第55行却强制该字段满足非空pointer并读Body+10反链；因此现有归属读取不能接受该合法静态路径。这是源码与原静态分支已证实的覆盖缺陷，尚未游戏复现或修复。

Shape集合原路径也已收窄：Actor表+98的135810转到嵌入Actor+28，再由121330读取+20，即公开Actor+48的根Shape。根为空返回0；非空根类型getter+8返回9时走Compound计数+398，否则返回1。Actor表+A0的135B10经121380走根Shape虚表+38取公开Shape列表。单Shape的C4F0返回internal+ A0这个槽位地址，与旧C4E0读取该槽中的公开Shape指针不同，不能把两者混为一条getter。

Compound RTTI类型描述277470、COL2491F0与主表227950交叉核对。表+398的28A60返回(begin@1B0到end@1B8之差)/8；表+38的28A90返回公开Shape数组指针1D8。28B00插入子Shape时写child+8为Compound；28 080原插入流程分别向内部Shape向量1B0／1B8和公开Shape向量1D8／1E0追加，后者取实际子Shape虚表+40的公开指针。静态Compound创建分支存在明确原日志与调用流，故不能仅补单Shape或假设多Shape一定动态。

下一步修复已有归属事务时，应同时关闭空／单／Compound表示、内部与公开成员的对应关系、同一Actor／Scene及当前代码／表身份；动态Body非空时保留原反链，静态路径以实际Shape成员关系提供归属证明，不能简单删除非空检查后宣布通过。原数组换址、成员／计数／代码改变及超容量均须整笔拒绝，原读锁／重入／未知fault保护不弱化。Actor+40既不是通用非空Actor身份，也不能替代完整Shape集合或物理同步。

证据analysis/cover-system-20261108/actor-shapes-r91/rea-evidence.json，68条、10411201字节；verification.json保留12个关键函数完整伪代码／汇编、原读取器SHA与明确下一修复。REA会话已正常关闭。本轮有证据改变下一行动，属于progress；没有产品代码修改、编译或重复测试，无stage／游戏／XR／录像／新增玩家动作验收，未使用新的运行窗口。最新Native仍为r90的945e7b3b，公开v71和r80五身份保持。

完整自身与世界Shape覆盖、生命周期／ABA排除和共同物理时相仍未完成；实际身体路径／终点占用／移动恢复、严重肩身遮挡、physics-hit-muzzle同版、真实LGrip／GrabPose及实体头显也未完成。保留CV01–24／AC01–22完整目标，工程25%不变，目标active。

## 2026年11月10日 r92原Actor Shape成员读取模块

上一轮da8d33bf的原静态分支取证改变修复方向，属于进展。本轮新增cover_physx_actor_shapes.h，按r91原字段建立空／单Shape／Compound只读清单。Actor主表与同Scene必须匹配；动态Body非空时保持Body+10反链，静态Body为空则不伪造动态对象。根从Actor+48读取；单Shape关闭根公开槽、公开Shape主表／Actor及internal反链；Compound要求原227950表与对应getter槽，内部1B0／1B8／1C0及公开1D8／1E0／1E8两套向量头均合法、等计数且最多64行，再逐行核对应指针、子parent为根、公开／内部／Actor关系及Body一致。重复内部或公开成员拒绝，不截断超容量。

清单双读一致且最后guard通过后发布，读取失败、变更与错误关系均保持调用方完整输出。Workspace具有重入拒绝，成员清空可清除旧行；无引擎调用或身体写入。当前CodeGuard仍是显式调用方合同：接线前必须认证r91原getter／count／dispatch代码及Image、当前C0 owner／线程／身份。此模块尚未接入QueryEngine或旧命中归属读取器，因此旧静态误拒绝尚未修复，不能把这份单独清单与另一时相hit直接合并，也不授权self过滤、身体移动或world fence。

独立v2冻结r90 final单元源及3份本轮文件，严格/std:c++17 /O2 /W4 /WX /EHsc /UNDEBUG /MT编译执行0，66项／0失败。覆盖动态单体反链、静态单Shape、静态Compound平行成员、每次读取失败、错误成员／父关系／Body／Actor／Scene／计数／表、双读间根变化及空根清旧成员。原DLL只作PE数据，CodeGuard与owner为明确测试seam，无实际C0、原调用或游戏。首版测试嵌套include将main宏重定义，/WX拒绝并报告重复main；新增可重命名测试入口后v2通过。首版日志与冻结源保留，不扩大栈或放宽产品检查。

证据analysis/cover-system-20261108/actor-shapes-r92/verification.json及source／source-v2、build／build-v2。新模块尚非Native依赖，故本轮未重建或替换Native／PG／XR，最新编译仍r90的945e7b3b；公开v71及r80封存身份保持，无stage／游戏／XR／录像或新玩家动作通过。下一步将实际原代码证书和该清单接入同一hit归属事务，以成员关系补静态路径并保持动态反链，而后完成原查询前后关闭。完整自身与形状世界覆盖、物理同步、身体路径／占用／移动恢复、严重肩身遮挡、physics-hit-muzzle、真实LGrip／GrabPose、CV01–24／AC01–22及实体头显仍未完成。工程25%不变，目标active。

## 2026年11月10日 r93静态Shape归属事务接线

上一轮98013c4f的清单模块及66项检查属于进展。本轮将r91精确getter／count／dispatch字节证书和r92空／单／Compound清单接入原cover_physx_shape_owner::observe。拆出共享公开Shape主表，避免循环include；Actor原98／A0分派槽、Compound表和计数／列表方法槽均关闭，code header逐字节匹配r91原函数。原Actor归属、同Scene、公开／内部Shape反链保持；动态Body非空仍读原反链，静态Body为空只有命中Shape在实际Actor成员清单中恰好出现一次才能通过，不以空值相等代替归属。

命中归属前后各关闭完整成员清单，并要求整个空／单／Compound表示、数组头及逐成员相同；没有第二次原查询或任何写入。已有QueryEngine同一次stream自动使用新归属器；raw命中顺序、输入、callback和fault_unknown pin／清理保护保持。新工作区只是单次归属的有界局部清单，未将64成员数组复制到每个Hit结果。此修复解决源码的静态误拒绝路径，不证明原游戏静态墙体、完整自身集合或物理world fence。

v2冻结r90 Native源，仅4个产品overlay，共1097文件。实际编译链接0，Native SHA为9f375dc7c453b7c0692f1c1b5e223023e66a092ac963e17f64173b88514fec2b；680实际依赖／400冻结项目依赖逐SHA，原flags保持。19个聚焦程序严格/W4 /WX编译执行0，Owner499／0、Owned122／0；最终ActorShapes程序补查完整静态Compound归属及新计数代码字节破坏，73／0。其余结果见verification.json。原37条警告保留，改动模块0新增；未替换PG／XR、publicv71、手动入口或r80封存包，运行cover_queries默认0及Goal no_cover拒绝保持。

首版/WX发现新增参数base遮蔽旧fixture全局，重命名为image_base后v2通过；最终新检查初期待module，但原代码证书通过CodeGuard返回guard拒绝，实际拒绝且输出保持，修正预期后通过，未放宽产品条件。两份失败及冻结输入保留。测试仍是原PE数据、合成对象和Host加载／dispatch，独立清单的owner/code准入为显式seam；新增完整owner调用执行实际代码证书与成员关闭。无原DLL执行／游戏／XR／录像或新玩家动作通过。

本轮收到HOD41008／ee031实际取消前驱预告：窗口仍属HOD，retention及固定完整closeout尚待交付，不是FINAL。后续只准备准确取消分支，保留正常分支，不复用49912例外、不重复产品prepare或消费r80。其余完整Shape／自身覆盖、生命周期／ABA和共同物理同步、身体路径／占用／移动恢复、严重肩身遮挡、physics-hit-muzzle、LGrip／GrabPose及实体头显仍未完成。保留CV01–24／AC01–22，工程25%不变，目标active。

## 2026年11月10日 r94取消证据独核与最新独立执行约定

上一轮aa1d4c31完成静态Shape归属接线、19程序与Native编译，属于进展。本轮按HOD必要接收请求窄核41008／ee031取消事实：首claim ce5fd8、bound bfbe55、baseline647e91及独立terminal2d39一致；Windows FILE_ID_INFO前后确认原首claim文件身份[12451548302678751898,2814749770231531]，未改写原件。完整review SHA8875d0b086c5c128c328e10883aaf197eabca36ad3b0e52b47a85a46635a250c及其取消／done／原配置恢复／1overview+4raw／encode0／retention终态引用逐SHA核对，原10当前与保存字节或缺失状态一致，private9与文件集按固定恢复独核记录保留。取消事实仍是normal_cleanup_verified=false、startup_cancelled_cleanup_verified=true、Native未启动、XR1且EndSession false，未冒称Native／XR正常或玩法通过。

审计文件analysis/cover-system-20261108/view-alignment-r80/hod41008_receiver.py及hod41008-cancellation-audit.json只读，不启动或授窗、不消费r80。核对期间收到主人最新独立执行规则：不再以远期前后继、逐级受理、整包互审或一次性票据重绑作为日常工作和启动的先决条件；仅实际准备启动共享游戏／模拟器时，使用现有启动互斥及真实占用检查简短协调。自有动作结束或首失败后立即正常关闭自有游戏／Native／XR／输入／录制，恢复原值、核对释放；已隔离的离线编码／审片／retention不占游戏使用权。身份、新鲜度、库存、存档、单writer及他人源码保护不变。

因此停止继续制作HOD接收ready／新FINAL门禁；原pending取消接收分支明确标记superseded_as_runtime_gate，正常分支与历史证据仍保留。收到Bow准备启动的实际占用询问，仅回复本会话没有游戏／模拟器／恢复／录制或共享写入作业，要求其自行按互斥与实际占用检查启动，未代替全局空闲核验或保留下一窗口。规则保存在independent-execution-owner-policy.json，后续无需等待HOD离线收尾或逐级签收。

原r80五身份、Native8035和封存入口本轮均逐SHA保持，未重新prepare／stage／启动；旧入口仍带过时FINAL检查，下一步须单独适配实际启动互斥与当前占用协议，不能伪造旧accepted／FINAL布尔值绕过。最新已编译源码Native仍r93的9f375dc7；公共v71、实际掩体动作／严重遮挡／物理同步／身体移动／受击枪口／LGrip／头显尚无新通过，CV01–24／AC01–22完整保留，工程25%不变，目标active。此轮进展为准确取消事实独核及当前执行规则落地，不称游戏或verified wait。


## 2026年11月10日 r95实际启动前只读占用检查

上一目标轮仅核对需求，未推进实现，按no progress处理；本轮重新核实1bf88a09已保存r94，开发文档两份副本与guard一致。r80原入口--prepare-only通过：363驱动、9271依赖文件和7模块来源均匹配，未消耗claim、stage或启动。按主人最新顺序，先完成可见视野／探身抓边／保持缩回／退出完整流程，其后吸附与Volume，最后逐点GrabPose；最终CV01–24／AC01–22范围保持。

新增tools/game-playtest/cover_launch_preflight.py，作为既有观察入口的只读接线模块，不创建新的调度或锁。前后检查原launcher两个owner mutex并读取现场进程；实际game／XR／paired recorder及已知共享python runner阻止启动。离线ffmpeg／review／retention不占游戏窗口；查询失败、错误进程记录及python.exe命令未知保持拒绝。只排除自身精确PID，不按名称排除其他runner。该检查不授予窗口或原子排他，实际启动器仍必须创建现有owner mutex；只能在stage与start现场使用。

9项定向检查通过，覆盖录制／runner阻止、离线media不占用、互斥在进程读取中出现、查询失败和畸形记录拒绝。Windows真实测试专用互斥完成不存在→创建后存在→关闭后不存在，检查句柄正常关闭。首轮实际枚举将Windows PID0空闲统计项作为坏进程拒绝；读取端明确过滤非执行进程PID0后再查，保留正PID严格验证。后续真实查询确认solo-m1-1028d现有launcher mutex已存在，仅记录当前占用，没有启动、stage、恢复他人文件或结束任何进程。此快照不是未来授窗或verified wait。

证据analysis/cover-system-20261108/launch-preflight-r95/verification.json、unit.log及冻结source。模块尚未接入封存r80入口；旧FINAL门禁未被伪造或删除，下一步在独立标识的新入口接线，取得本次fresh baseline及完整恢复责任后才能运行。未重编未变化Native／PG／XR，最新Native仍r93的9f375dc7；publicv71、r80原五身份与原claim保持。实际视野严重遮挡、身体路径／占用／移动恢复、物理同步／受击枪口、LGrip与头显未新增通过，工程25%不变，目标active。


## 2026年11月10日 r96独立观察入口与完整恢复接线

r95已保存177988c8，属于进展。本轮将只读实际占用检查接入独立run_cover_eye_independent.py，去掉对远期前驱／FINAL票据的依赖，不修改或消费封存r80五原件及原claim。独立父入口／派生inner／stage／helper分别逐SHA；运行前静态核9271依赖、363驱动、7模块来源和原二进制。使用统一tools/debug-launch/runtime_stage_guard.py的RuntimeGuard包住即时备份→stage／config→原launcher→正常关闭／恢复；内部仍用原RepositoryLauncher.Owner，不新增另一套锁名或调度。离线编码和审片放共享保护之外。

新增cover_observation_baseline.py：即时记录原11路径及缺失状态、原用户Config／SaveData只读文件、私有Config／SaveData完整平面文件集。所有备份和只读文件在首次恢复写入前统一复核；只恢复workspace中明确拥有的文件，单文件移除新生成私有存档，不递归删除，不写public或原用户存档。4项定向临时目录检查通过，含二进制字节与缺失恢复、私有新文件删除、后置备份损坏阻止前置写入、只读变化拒绝和意外子目录保留。

正常Native／entry／pad／drain／XR清理、Native前启动取消和无game birth的失败分别判定，不把XR1或未激活Native写成正常通过。所有分支还要求本次owner／game身份、终态时相、进程退出和现场占用检查。数值解码前先保留原眼点记录并恢复完整基线；缺失或异常原始记录不据此冒称可见视野通过。未完成恢复则明确保留错误，不能签发玩法通过。

首次新入口预检查因共享run_v63_startup_repro.py已由其他工作改为a3773503而拒绝，未stage。单独核实其余362驱动仍匹配；后续明确采用封存r80内3f297b3d启动脚本副本并保持其余当前／归档逐SHA，不混新共享修改。最终独立prepare-only实际0，未consume／stage／start；原r80 Native8035及五相位观察动作保持，没有换入r93查询Native或新增抓边／武器动作。当前文件中的2026-11-05启动兼容覆盖明确选定既有本地副本，身份仍bdc6705d；不强制改变认证路线。

证据analysis/cover-system-20261108/view-alignment-r96/offline-verification.json、prepare-only.log、baseline-tests.log及独立源码／plan／ready。本保存点只表示入口接线和离线检查完成，尚未实际运行；下一步按统一共享互斥及现场检查执行一次当前视野观察，结束立即恢复释放再离线分析。完整进入使用退出仍为第一阶段目标；吸附／Volume其次、GrabPose最后。publicv71及最新产品编译r93 Native9f375dc7不变，实际严重肩身遮挡、移动碰撞／受击枪口／真实抓边／头显未新增通过，CV01–24／AC01–22、工程25%和active保留。


### r96实际56276首败与正常撤回

保存6ac463c5后实际尝试一次，取得统一RuntimeGuard、现场进程／原launcher互斥检查通过，即时捕获29项字节／缺失和私有文件集，stage0。此时本会话已收到“先等GN原件恢复”的协调信息，但错误地把可取得guard及无game当作可以继续的充分条件；后续GN未完恢复通知到达时已stage／启动。立即按本次owner精确39660／creation134387685048022998发正常Stop并写自有stop-basic，不杀进程、不继续动作、不重试。该操作失误保留，不把mutex可用解释成前场原件已恢复。后续再次stage须先确认这一具体未完恢复已终结；不恢复远期前驱链。

实际game56276／creation134387685123838574、XR12644，Native确实启动。wrapper／launcher0、operator44724退出1。真正动作首败在原版入掩体A之前：选最近Link98 slot0后，普通导航从[-3900,0,583.9485]移动至[-3928.9395,-50.9543,583.9485]，约58.60units；距目标由161.303降至102.704，随后三轮未移动，按既有guard报告Controlled walking blocked before pickup; no teleport fallback。未到after_original_A，无五相位eye_observation，不能称掩体认证／探身或本次视野修复通过。记录显示首次0.62摇杆后移动、0.22刹停输入后静止，但死区／碰撞／其他原因仍未证明，不把时序相关当根因。

本次Native／entry／pad／weapons drain／XR0正常，owner及game／XR退出身份核对通过；父入口在数值解码前恢复即时29项及私有文件集，无public或原用户文件写入。已现场确认outer53828／inner55548／launcher39660／game56276／XR12644退出、无自有录制，统一guard释放并向Bow发一次释放通知。此baseline可能含GN此前尚未恢复部署，只证明撤回掩体本次更改，绝不签发GN原件完成或完整共享原件恢复通过。

隔离眼点bin983368bytes已保存SHA57ebd97320dc5b69205363140d66fd680f3a7828b41fc635c2bbc4d682662e2b，原inspector实际0：61行／61eligible／0numeric_mismatch／0overflow；仅CPU callback读取，未进入掩体五相位，不是GPU／双眼／碰撞／肩身遮挡验收。双路recorder0、encode0，完整1overview及4实际raw亲看：启动waiting card、非掩体自由手双眼场景及正常停止末帧保留；不以最后黑帧判断新渲染根因。game285帧17.216s，simulator343帧16.150s；game有2.25s捕获空隙，simulator尾覆盖差1.05s，both_lanes_cover_end_within_500ms=false，不能称完整行动双路覆盖。

retention实际调用被随后另一活动game55436／XR34068阻止，结果deferred且未删媒体；本会话所有运行及编码均终态，不持有共享使用权，不监控或停止他人游戏。证据runtime/closeout-review.json、原失败动作／cover-boundary、eye-inspection、paired视频和retention记录。r96已consume，不再运行或重新prepare；r80原五件与claim保持。下一动作聚焦普通掩体接近停滞及有效入掩体，确认未完共享恢复后才另建有实质修复的新尝试；先完整逻辑，后体验，再GrabPose。publicv71、r93产品Native、CV01–24／AC01–22、25%和目标active不变。


## 2026年11月10日 r97原版接近停滞定向调整

上一轮71bd7c54保存56276真实首败及正常撤回，属于进展。本轮以原motion及输入timeline核实：0.62幅度产生58.60units位移，随后0.22输入对应位置连续不变。尚无游戏死区读取或完整碰撞证据，根因仍未证明，不宣称已修复deadzone／墙体。新增掩体专用cover_ordinary_approach.py，使用实际曾有响应的0.62幅度，近处将平台保持缩短到0.04s作刹停；仍走原ctx.stick平滑Source1发布与live／Pawn／Controller／World身份，不修改其他武器共用navigate、任何原版输入消费或碰撞。

35units到达标准、连续3次停滞拒绝、42步总上限保持；实际位置才决定到达，不能靠请求或时间宣布成功。5项离线检查实际0：重放56276停滞位置仍拒绝，所有发出输入均0.62／近处0.04s；覆盖实际到达、原Player yaw轴映射、慢进展不能绕过总步数、NaN／Inf及放宽到达距离拒绝。该检查不运行游戏，不证明新输入将通过真实碰撞或进入掩体。

建立独立view-alignment-r97入口、cover专用operator及元数据；r96已消费源码／claim／原失败保持，不重跑。实际新operator只将cover_boundary中一次ctx.navigate换成本次模块，其余候选选择、原A、身份认证及五相位观察保持。原r80 Native8035、PG／XR、9271依赖与363原驱动仍绑定；共享启动脚本继续采用已封存3f297b3d副本，不混其他会话新改动。完整派生函数离线构建／字节码检查确认只导入并调用一次本次接近模块，未执行菜单或游戏部分。首次生成漏复制relocated runtime helper而拒绝；首次模块构建发现旧driver目录未入新operator路径，加入显式O后通过，均未stage或consume。

另修新入口的结果报告：r96外层child0不能覆盖实际operator1。新入口在共享恢复后分别记录operator退出／错误、当前同game结果、boundary与完整5个QPC区间；观察序列未完成则返回非零，仍不签发完整玩法／GPU／头显通过。用保留56276实际文件离线重放，明确得到operator1／0phase／原walking错误；当前指针已属于另一game时同样拒绝，不将不同局拼接。重放仅将可变current指针重定向到原保存结果，不改原文件。

已读GN具体未完恢复的recovery-restoration.json，时间15:16:45、SHA26b2b78c078e061d6fdf166d7172c3ffdcdcaf9abe1aafd74f2ba01f419e8784：与原claim8f91eb及baseline e224c8身份对应，10项checks全true、restored／released true、public_written false。新入口在统一guard内明确核对此已知未完恢复已完成，避免再次把空闲当恢复；这不是远期队列／前驱FINAL或互审前置。每次实际启动仍用共同RuntimeGuard及现场占用，从新即时备份到正常恢复后释放，离线不占用。

最后独立prepare-only0，5项检查0，r97未consume／stage／start。实际看到他人game5212（15:33:48创建）后不启动；狙击枪询本会话是否持锁时只确认自身仅离线准备、未取得guard且无预留窗口，不追查或要求对方签收。r96媒体retention此前因他人运行deferred，不以此占用共享资源。证据view-alignment-r97/verification.json、offline-transform-verification.json、replayed-56276-action-result.json及两份实际日志。新接近逻辑仍待一次游戏定向验证，后续先完成可见视野／入掩体／探身抓边／保持缩回／退出，再体验与GrabPose。publicv71、最新产品Native9f375dc7、完整CV01–24／AC01–22及工程25%不变，目标active。


### r97真实互斥拒绝，未消费候选

保存f5fb1f46后，实际现场暂时无game／XR／paired recorder、launcher index为finished；已确认GN原件恢复记录，向狙击枪仅发实际启动通知，随后调用r97。静态检查通过后，统一RuntimeGuard的WaitForSingleObject(0)返回busy，入口实际exit1，未进入run_owned：无prestage目录、inner.log或claim，无备份／stage／输入／游戏启动，未获得或持有任何共享窗口。guard-refusal.json记录实际入口SHA及拒绝；已简短通知询问占用的狙击枪本会话不持锁，无须等待本会话释放，不追查／停止他人进程。该拒绝不是已确认进程handle上的verified wait，也不是本目标无进展：本轮接近定向修改、实际函数接线和离线证据已完成并保存；尚缺游戏检查，r97保持未消费，不重新prepare或创建替代票据。

随后仅独立调用一次r96未完媒体retention。工具执行到inventory关闭检查时检测新game／XR启动，正常返回deferred='A game/XR started during inventory; no media removed'，未删除媒体；同一工具handle65396已terminal0，无本会话运行／编码／retention作业留存，不占共享guard。结果保存在view-alignment-r97/retention-after-r96.json。下次实际共享释放后方可做r97一次定向运行；原完整目标、公开v71和25%不变，目标active。


## 2026年11月10日 r97实际53976接近通过与r98同局头身视向证据

上一轮c02d6139／f5fb1f46已保存定向接近修改和互斥拒绝，属于进展。本轮先确认他人game55152／XR44844／recorder40280真实live，等待并复核同组进程实际消失；现场无game／XR／录制后发一次启动通知，按原已封存r97静态检查取得共同RuntimeGuard。已知GN具体未完恢复仍按原claim／baseline及10项恢复收据核对，然后即时29项备份／stage0。未重prepare或重跑已消费r96。

实际game53976／creation134387701469138129、owner39756／creation134387701394358574，Native仍8035bf35、PGf8e6f3c2、XR16840b27。4条实际位置反馈后距离30.3885units，满足原35标准；原A进入CT2并认证当前Link／slot，完成中立、固定位置转头／返回、真实样本抬高10cm／返回5相位。operator0、outer0，动作和正常清理分开判断。该实跑支持本次接近驱动修正，仍不证明旧0.22停滞的唯一根因是deadzone。没有抓边／身体拉拽／松手保持／缩回／退出或战斗动作，不称完整掩体流程通过。

Native／entry／pad／weapons drain正常、XR exit0，owned进程退出核对后恢复29项字节／缺失和私有文件集，public／原用户文件未写，立即释放统一guard并发一次实际释放通知，之后只离线。XR保留原terminal_exiting／objects_destroyed／shutdown_completed，EndSession false，不改写成API EndSession通过。session原始报告已复制并逐SHA保存在r97/runtime/continuous-session-original.json，防止后续媒体保留标记影响原报告身份。

原inspector实际0：眼点bin3030608bytes／188rows／188eligible／0numeric_mismatch／0overflow。五相位按本人物／world_generation及各begin≤row.begin≤row.end≤end精确选取7／6／7／7／7条，共34条，不用近时拼接；CPU读取／准备结果不能当GPU原子pose。双路recorder0、encode0，1张完整span overview及7张实际相位raw亲看，五相位均严重头／肩自身遮挡，实际视野FAIL。game531帧29.266s、simulator624帧29.216s，尾覆盖均0ms；game启动段有1.817s捕获gap，simulator最大266.7ms。保留gap及启动第三人称／waiting card，不能用编码0称完整可玩。

新增仅离线eye_face_inspect.cpp，直接复用r80原ABI reader及r79实际冻结的canonical115／Marker／head-to-eye计算；逐row真实SHA准备模型，确认捕获mount与同版marker相同，再将author面向通过同一head55及component矩阵转到world。另直接读取当前57／60眼骨中点，不用另一局或渲染结果反馈校准。/W4 /WX v3编译／执行0，19项项目依赖逐SHA；188／188同版basis与mount匹配。原eye等于当前双眼中点沿当前face方向前3units，最大向量残差0.0002553units；这支持当前authored3cm点确在实际眼骨前方，不是任意头中心偏移。

同局入A前两个CT0观察之间44条同pawn／world CPU bracket，基础view与authored头面向夹角最大0.01647°；入掩体五时间窗61.44–66.69°，水平58.24–63.25°。比较对象是HMD合成前基础view，不冒称最终GPU／头显姿态或唯一根因。实际Native状态cover_camera disabled、cover_full_goal inactive／attempts0，body_goal_view_available0；与该Native实际编译依赖SHA核对的body_c0_goal_policy_live.inl在cover分支只sample_current_cover_pose，随后按cover拒绝普通body目标。当前源码同样保留这条保护。下一修复范围因此收窄到当前cover头／身目标与观察基准的实际consumer接线；沿现有唯一C0和已有fitted adapter实现，保留普通no_cover、身份／时效／输入／路径占用边界，不靠盲改yaw、挪眼或隐藏全身。尚未修改该产品代码或放行任何身体位移。

新离线工具首版缺Windows类型，第二版Windows small宏与冻结纯数学变量冲突；保留两份failed-source／build.log，仅在新离线translation unit补Windows头并undef small，未改冻结或产品头，v3严格通过。证据r97/runtime/closeout-review.json、phase-coverage.json、实际视频，以及r98/verification.json／pre-A-face-control.json／phase-face-comparison.json／evidence-index.json。r98未启动额外game／XR／Native。

本次媒体retention实际terminal0、errors空、删除0，最新3=54632／38416／53976、48h；未占游戏窗口，所有自有任务终态。r97已consume，后续不重跑同一观察；publicv71、最新产品查询Native r93的9f375dc7及未验收完整CV01–24／AC01–22、工程25%保留，目标active。源码、离线、游戏动作、像素和头显证据继续分开。


## r99：Scene 掩体目标到冻结 C0 中立姿态的转换入口（2026-11-10）

新增 `cover_c0_target_adapter.h`，把已有 Scene 掩体目标转换为已拟合、冻结的 C0 身体目标。转换核对当前世界／人物／网格／Link／槽位、Scene、原始追踪空间与序列、独立墙面／观察基准及显式脚位；拒绝提案自行标记已应用、碰撞通过或同版姿态完成。原请求及追踪时间保持不变，不因重新计算刷新证据。失败不修改输出，输入与冻结校准保持不变。

`cover_full_goal_live::prepare_c0` 提供实际存储读取到转换的入口，读取锁在转换之前释放。当前主 C0 `pose_begin` 尚未调用此入口，原有 cover／standing 保护保持；因此这不是实际写入接通或头肩遮挡修复。

严格 `/W4 /WX` 编译与运行：目标转换159项通过，实际存储25项通过。使用保留的61492参考／C0几何及明确构造的CPU掩体身份与时钟，覆盖普通Core和独立当前Tick追踪；旧Scene样本不可替代新Tick，过期与身份／目标篡改均拒绝。v1/v2编译错误、v4错误阶段预期及其日志保留；v5和store-v2最终退出均0。证据：`analysis/cover-system-20261108/c0-target-r99/verification.json`。

本轮未构建Native DLL、未修改运行包、未启动游戏／XR，public v71与工程25%不变。完整CV-01–24／AC-01–22、实际双眼及实体头显验收仍未完成。下一步补齐当前独立掩体放置及占用证据、唯一C0写入事务和提交后视图发布；不能删除no_cover／standing检查或启用旧相机原型代替。


## r100：原版胶囊查询中点溢出修复及转换代码核验（2026-11-10）

继续检查完整身体路径查询时，重新绑定原版 PhysXCore.dll（SHA256 `22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48`）并核对静态调用：胶囊扫掠 A6AA0 经 18A9C0 跳板调用 1A5740，把端点转为宽阶段盒体。该转换先按float把两个端点相加，再乘0.5；端点各自有限、轴长有效仍可产生Inf中点。新增正／负大坐标案例在修复前均失败（132检查、2失败），保留原日志。

`cover_physx_query.h` 现于原版调用前检查这两个实际float中间量；合法大坐标仍可通过。`cover_physx_query_code.h` 增加转换函数494字节、跳板5字节及四处常量28字节的精确哈希，原有查询源码核验扩大527字节；这是该转换链的补充，不宣称整个传递调用图已认证。

严格编译及最终离线结果：原版文件／查询适配151项通过，完整命中归属122项通过，实际查询Session适配86项通过。测试以原DLL作为只读数据，未加载或执行原PhysX；Session使用明确Host调用替身。首次Session编译因构建命令缺少MinHook头目录失败，补齐后退出0，日志保留。

在r93冻结Native源码上只覆盖上述两个头文件完成DLL编译：SHA256 `8a94c32c7a31a14fcbdfaa605351778b6fafd338d0cbc7c6db72a5187449d970`，400项项目依赖核对通过，无外部工作树依赖，开关保持一致。不是整个当前脏工作树的集成或r99运行接线证明。未stage／安装、未启动游戏／XR。证据及原版函数／字节记录保留在 `analysis/cover-system-20261108/capsule-conversion-r100/verification.json`。

1A5740确有零轴身份矩阵分支，但尚未证明所有后续窄阶段分支的退化胶囊等价球体行为，继续拒绝零轴胶囊；不启用球体查询，不用盒体或零长度胶囊冒充已通过。探索文件中的sphere／box名称只是临时标签，目标形状编号对应尚未认证。下一步从原版分派表逐项核对球体所需查询语义，随后完成当前几何／占用与唯一C0事务接线。CV-01–24／AC-01–22、完整游戏／双眼及头显验收仍未完成，public v71及工程25%保持不变。


## r101：独立球体重叠查询接入现有C0适配（2026-11-10）

原版PhysXCore哈希仍为22051aa7…ddd48。静态分派核对：公开表23CA98+320→1683D0，内部表22CCB8+258→A6560；包装函数内部返回未命中时，继续走126780→1859B0的原版回退链；A6560使用1AB270宽阶段及原形状虚调用。完整保存函数ASM／伪码、表字节、921字节新增核验范围，未把静态分析写成实际执行证据。

新增独立Sphere输入（centre三float、radius一float），仅开放overlap；通过既有QueryEngine、Session及cover_physics_c0_live入口接线，沿用当前C0／世界／人物身份、前后源码核验、唯一查询执行与故障未确定保留。两个原版球体分派槽增加到现有来源检查，不增加身体写入端。零轴胶囊继续拒绝，Sphere sweep未开放；球体重叠的命中或空返回均不授予占用／移动许可。

严格编译／运行六组全部退出0：查询适配176项、独立球体入口28项、完整命中归属122项、类型身份读取257项、查询Session87项、功能关闭分支21项。球体入口检查实际生产QueryEngine与来源／代码检查；仅模块pin和原调用为Host替身。覆盖正确入口与接收者、payload、原始命中／未命中、分派槽替换、各新增范围末字节修改、查询后来源／代码变化、非法半径／溢出、故障保持未确定及不强关资源。原DLL仅作为只读数据，真实球体参数与返回语义尚待当前C0受控实跑。

在r100冻结Native源码上仅覆盖5个相关生产文件完成集成编译，400项项目依赖核对通过，无外部工作树依赖，原开关保持（CoverBodyPrototype与PhysXQueries编译ON，运行配置不变）。DLL SHA256 `46c26ee3fde78c65281d9137ba87721e3c2c3a60677c407a84ce5d20664865d7`，只保存在证据目录，未stage／安装；未启动游戏／XR。不是当前所有武器脏改动的集成或完整碰撞通过。

证据：`analysis/cover-system-20261108/sphere-overlap-r101/verification.json`。下一步在已有受控游戏路径验证一次有界的原版球体读取调用，继续完成连续球体扫掠、完整当前几何／世界时相／终点占用和唯一C0事务接线；完整CV-01–24／AC-01–22、双眼与实体头显验收仍未完成。public v71及工程25%保持不变。


## r102：当前掩体C0的一次原版球体观测入口与独立准备（2026-11-10）

`sample_current_cover_pose`在当前掩体眼点测量后调用现有C0球体适配。新增观测默认OFF，每次Native载入最多尝试一次；失败亦消耗该次观测，不换来源重试。原版调用外不持私有锁，既有身份／源码／元数据前后复核及故障不确定保留机制继续生效。观测后重新核对当前上下文和掩体键，不把调用前测量冒充调用后的当前数据。

显式INI选项`FullBodyC0Native.cover_sphere_probe`须同时启用cover_queries；独立私有入口使用`GEARSVR_COVER_SPHERE_PROBE=1`，仅在实际simulator_operator、continuous_session及既有observe-only子进程环境均成立时允许开启原查询会话。普通配置及物理头显配置不自动开启。状态字段统一使用cover_sphere_probe_前缀，记录尝试／完成、原始返回、世界及代次、人物、Link／槽位、Scene／Compose／序列／线程、开始结束时间及实际球体。1cm半径仅为原调用诊断输入，不是已认证头部碰撞体，不授予占用或移动。

关闭分支严格编译和35项检查通过，包含INI／环境请求及非法值拒绝。首次隔离Native编译因历史保存策略缺少before／after变量失败，原源码与日志保留；v2补充只读掩体身份闭合后编译通过，v3加入受控显式环境入口与唯一状态前缀再编译通过。最终DLL SHA256 `2e65bfd483935f0c15fa89313701274e48d0528ed6b92a4f2e946c7ebe27d862`，400项项目依赖、原开关及无外部工作树依赖核对通过。冻结基线保留原Core眼点测量分支，只加入本轮查询与身份闭合；当前源码已有V3测量，不将这个隔离DLL宣称为整个当前源树或V3运行通过。

新独立尝试`equipment-controls-prepared-20261110-cover-sphere-r102`与`analysis/cover-system-20261108/sphere-call-r102/run_cover_eye_independent.py`准备检查退出0；新Native、原PG／XR、363项运行驱动与依赖身份已绑定。复用r97普通接近／原版吸附和五个短观测阶段，新增一次Native原查询；旧r97已消耗尝试和r80原始五文件未修改，未重绑旧窗口票。仍使用同一RuntimeGuard，从新鲜备份经stage／运行到正常恢复全程持有，离线编码／审阅在其外。

本轮只离线准备，claim未创建、未stage、未启动游戏／XR。实时发现共享游戏先为54968，随后9596，最后55756（XR55112、录制56968），均不是本轮启动，不干预其窗口。新入口原版查询的实际执行、语义及正常清理仍待运行窗口，不把准备通过计为游戏通过。证据：`analysis/cover-system-20261108/sphere-call-r102/verification.json`。完整球体扫掠、几何／世界时相／占用、身体提交及双眼／实体头显验收仍未完成；public v71及工程25%保持不变。


## r103：r102实际42332来源拒绝与正常恢复（2026-11-10）

上一轮是源码／准备推进。本轮先实际观察共享进程，第一次独立入口被同一RuntimeGuard拒绝，未备份／stage／启动，claim未消耗；拒绝日志另存。随后重新观察游戏／XR／录制均已退出，独立入口取得真实运行锁，持有到全部正常清理和恢复。未通知统筹、未派发其他会话或武器工作。

正式运行使用r102 Native `2e65bfd4…7d862`，游戏42332（creation134387761076846786），launcher8400（creation134387761002794238）。普通接近、原版CT2吸附和五个短观测阶段完成，operator退出0、inner退出0；独立入口最终退出1，正确保留本次球体观测失败。球体确实requested／attempted／finished各1，返回Code4（source），不是queried。记录绑定世界2312208640／代次1、人物16477978880、Link4929110784／slot0、Scene218、Compose13491、序列1502、线程55720；输入球体(-3921.65381,-189.660019,626.241882,r=1游戏单位)。raw_hit=0不构成无碰撞结果。当前总返回码可出自多层来源拒绝／catch；本构建无原调用开始／返回计数，不能断言具体拒绝层或原callee已／未执行。

Native及entries正常清理、虚拟手柄恢复、武器drained、XR退出0（stop_signaled／wait）均有同局记录；手部stop返回5但drained=true，不能改写成所有原返回码0。29项文件字节／缺失状态及Config／SaveData文件集合恢复，public／原用户文件未写；随后真实guard释放。原始终局报告在retention注解前另存，其SHA256为25a653f3de9f8f673befb2be0cca631d1d6f96d0b3f6565e754a44d2c4e8ba6e。

原始眼点trace3046728字节；旧格式检查退出0，189／189 eligible、numeric mismatch0、overflow0；五个完整QPC阶段同Pawn／world的CPU行分别7／6／6／7／6。Trace没有Probe Compose字段，同Scene关联不是同C0／物理姿态证明，未做最近时间拼接。两页完整双路总览及五张阶段原图已逐一查看：头肩仍严重挡视野，画面FAIL，未修复。启动三人称／等待卡保留。双路录制与离线编码退出0；game530帧31.833s，启动采集最大间隙2.267s；sim676帧31.816s、最大间隙283.33ms；两路末端覆盖500ms内均为true，不等于完整渲染／头显通过。

编码／审阅在guard外。常规媒体保留助手因另一运行的game／XR仍活跃而deferred，未删除媒体；稍后实时确认56252／56768，不干预，保留待空闲执行事项。r102 claim已经消耗，不重跑或重生这个尝试。证据：`analysis/cover-system-20261108/sphere-call-r102/runtime/closeout-review.json`、`phase-coverage.json`及原始报告／原图。

启动前还修正新入口的两处问题：计划残留的“无原查询”说明更正；已消耗尝试若不能证明baseline恢复，错误路径保留实际运行锁进程，不自动释放、强停或重试。实际入口的四个离线异常场景共7项检查通过（显式fake guard及注入等待边界，未取得真实锁或运行游戏）；本次正常恢复路径不进入该保留状态。保存独立入口及对应错误路径检查源码供审阅。

并行的原版静态扫掠调查另存 `analysis/cover-system-20261108/sweep-research-r103/findings.json`：胶囊分派行index4指向3字节XOR AL,AL／RET，返回false；该index的当前形状标签尚未新认证。球体helper有相等端点分支，但胶囊helper会先计算轴叉积归一化再走边测试，完整零轴等价性仍未证明；mesh／height／convex的D9490和Box的D5A70仍待针对性解释。没有启用球体扫掠，没有将原空返回签发完整覆盖证明。

下一步为来源Code4增加有界层次与原调用观测，定位C0准入／类型读取／查询内部的第一拒绝，不放宽校验，不重复未改动的查询。CV-01–24／AC-01–22、完整路径／占用、实际身体／视图／命中合流及头显验收仍未完成；public v71及工程25%不变。


## r104：来源拒绝层次与原调用开始／返回观测（2026-11-10，尚未运行）

针对42332的实际Code4总错误，增加三层只读观测：C0外层准入／掩体／guard／alias／Session；Session的几何、PhysicsRoles、ColliderRoles、ComponentRoles、Engine及关闭复核，保留实际roles返回码；QueryEngine的来源开始／重复／guard、代码核验、原调用、返回、关闭复核及异常标记。现有许可、原始时间、查询参数、Code返回语义和失败时输出不变。

QueryEngine分别记录进入Calls包装的次数与其正常返回次数；生产Calls立即转入既有原函数SEH包装，Host测试替身仍明确不是原函数执行。Probe记录调用前／后的计数，未进入Session时不抄旧Session阶段。该观测只供已序列化的同一owner调用后读取，最终发布仍经已有Probe互斥快照；不新增跨线程原始查询读取者或身体写入端。

严格离线检查全部退出0：球体入口36项，真实Session适配94项，完整命中归属122项，关闭分支35项，原查询／文件数据176项。新增反例证明：调用前来源拒绝不增加开始计数；正常原调用替身返回后再抛读取异常，仍可返回同一个Code4，但开始／返回计数均增加、阶段指向source_end且exception=true；SEH式故障只增加开始、不增加正常返回，并继续保留不确定资源。Session类型来源拒绝保留其roles子码，Host callee无Native证据时supported=false。

在r102 v3冻结源码上仅覆盖三个相关生产文件，Native编译通过：SHA256 `487708269814b99c6b8c487da4bc6be67f36c3ea044a0c9196905a41a86b82a6`，400项项目依赖核对、原开关及无外部工作树依赖通过。独立新尝试`equipment-controls-prepared-20261110-cover-source-stage-r104`和入口准备退出0，复用已验证的普通接近／吸附和短观测动作，仅新增上述来源阶段／原调用记录；r102已消耗尝试完全保留。本次尚未stage或启动游戏，cause仍未证明，不能先称来源修复。

证据：`analysis/cover-system-20261108/source-stage-r104/verification.json`。继续同一RuntimeGuard与真实占用检查，正常清理／恢复完成才释放；错误保留运行锁逻辑复用已保存的r102入口。public v71、工程25%及完整CV-01–24／AC-01–22范围不变。


r104运行入口补充：共享进程40964、42324、34780、54756先后实际占用。一次run模式请求被同一OS guard拒绝，尚未backup／stage、claim仍未消耗，拒绝日志另存。新未消耗入口将run模式的零等待锁获取移到冻结来源校验之前：这是当下实际启动尝试，校验失败立即释放且无stage；没有未来队列、等待票或prepare-only预占。所有来源校验、真实占用复核、全量新鲜备份和恢复仍在该guard中；prepare-only继续完全不获取guard。实际入口异常路径7项离线检查通过，并确认run校验在fake guard已进入后才执行；未使用真实锁／游戏。重新prepare-only退出0，Native DLL未变，r102已消耗入口未修改。


## r105：独立掩体实例准备与动作输入隔离（2026-11-10，尚未运行）

上一目标轮仅核对既有需求文档，对开发／玩法没有新增完成证据；本轮据当前源树重新核对r102终局，确认自有正常清理、29项恢复和runtime_released已闭合。按最新主人独立运行要求，后续不执行旧r104的solo stage／restore入口，不排共享窗口、不控制其他实例，也未通知统筹。HB历史恢复回执另核SHA256 03146891…8e42，10项baseline_checks均true；它不构成掩体或HB玩法通过。

新增隔离准备脚本，为唯一cover-1110-r105-source-stage创建自有Native、PG、XR、配置／存档、私有输入及cover-output。三个制品逐项复核：Native48770826…2a6（r104来源层次观测）、PGf8e6f3c2…807、XR16840b27…7fc；不复用seed的其他武器二进制。仅新私有config保持本掩体检查0.4，公共工具默认、用户文件、publicv71和原EXE未改。prepare实际退出0，未启动游戏、未消耗运行尝试。

原Source1输入读取器保留共享内存magic／版本／游戏及publisher creation／序列／reference／100ms时效检查，将全机唯一XR枚举替换为本次launcher和session中精确XR身份、独立XR路径及source1核对。原普通接近／CT2吸附和五段眼点观察脚本已冻结派生到本次输出；不修改共享原脚本或原动作。输入前live同时复核本次目录／报告／session ID、owner／game／XR完整身份和实时creation，不使用全机唯一game／8720作为条件。双路录制继续使用原按PID+creation+HWND绑定的录制模块。

实际生成live函数的离线反例14项通过：更换目录／报告、owner／game／XR、session、phase或任一实时creation均拒绝；无关额外进程不阻塞本实例。这里使用明确假身份提供者和临时合成报告，没有发输入或运行游戏；语法检查不能代替实跑。新原查询报告解释器的6项合成控制此前通过，旧r102缺新字段仍返回unknown，不将raw_hit0或查询错误签发无碰撞证明。

证据：analysis/cover-system-20261108/isolated-r105/cover-1110-r105-source-stage/prepared.json、operator-prepared.json与identity-check.json。下一步完成本次启动／正常停止owner、显式一次probe环境和原动作接线，按真实内存准入执行一次来源层次观测；当前没有自有game／XR运行。来源拒绝原因、完整身体扫掠／当前占用、唯一C0实际提交、双眼遮挡及CV-01–24／AC-01–22和实体头显仍未完成，工程25%不变。


## r105实际启动前失败、依赖修复与当前离线继续（2026-11-10）

上一轮独立准备属于progress。本轮新增并保存独立owner入口d6400b8b：启动前核本次manifest／config／Native／PG／XR及解释器动作源码，正常结束或首败均只请求本实例正常stop；原双路录制按当前game／XR PID+creation+HWND绑定。运行中以200ms间隔抽查前台窗口，发现本方激活即停止动作并正常清理；该抽查不是连续不抢焦点证明，既有静音／behind／no-activate环境亦不等于实际行为验收。

cover-1110-r105-source-stage实际调用独立工具start成功返回worker57040（creation134387794694322429），随后launcher退出1。日志为“VR render size rejected before game launch”；原XR EXE单独尺寸预检退出-1073741515。核对发现原冻结builds/sim_xr只存EXE，独立复制缺openxr_loader.dll。没有launcher-session或birth回执，isolated-done记录launcher1／owned_game_absent=true／forced_termination=false；同一worker身份已实际消失，不能把这次记作Native清理失败、游戏动作失败或来源拒绝重现。前台抽查4次未见本方激活，仅覆盖此启动前短窗口。

7f0baa19修复私有包依赖：新输入目录稳定复制原XR EXE及现部署加载器，loader SHA256 66946ea6…932df；启动前再次核私有loader。新名字cover-1110-r105b-source-stage准备退出0，不覆盖r105失败或原冻结制品；同样Native48770826／PGf8e6f3c2／XR16840b27及0.4保持。新派生live函数14项身份隔离反例通过，仅假身份／合成报告，不是实际输入或游戏通过。

对r105b的实际run请求在claim前被现有内存门槛拒绝。随后只读记录可用物理1001472000字节、可用commit1342246912字节，低于1GiB／6GiB要求；未抢占其他实例。r105b没有claim、launcher-session或本方game／XR。随后收到最新人类转达“刚刚又到前台了”，已暂停所有新的game／XR启动，不自行重启测试焦点，继续离线开发；不暂停完整掩体目标，也不向统筹回复或汇报。

证据：analysis/cover-system-20261108/isolated-r105/closeout.json、failed-prebirth/及cover-1110-r105b-source-stage/。本轮未取得新的原查询执行／游戏／双眼证据，Code4首次拒绝层仍未知。后台修复和资源条件满足并按最新授权恢复运行后，继续本次未消耗来源阶段检查；期间推进离线碰撞及共同身体接线。完整CV-01–24／AC-01–22、身体路径与终点占用、实际C0提交与视图／命中合流、头肩遮挡、联合战斗及实体头显仍未完成。publicv71与工程25%不变。


## r106：底层扫掠分支查证与四段原代码核验补齐（2026-11-10，离线）

上一轮启动前失败／依赖修复属于progress；本轮遵守最新暂停新game／XR启动要求，继续完整身体路径所需的原查询查证，不重启试焦点、不占其他实例、不派助手。原PhysXCore SHA256 22051aa7…ddd48不变，Ghidra只读分析后已正常close，原DLL未加载／执行。

新保存四个完整函数ASM／伪码及Evidence：D5A70（516字节）将胶囊半段送CC030、以中点／半径调用D7570；D9490（1354字节）在D95D4／D95E3／D95F2检测三轴半段为0，走D9635直接D7570分支，非零段先经CC820再调用；D0180（1633字节）在D0206／D020A直接求轴长并倒数，没有此前单独等端点球分支；D7570（2232字节）在D79B9剔除零面积面，并在D79D6／D79DB根据未归一化面法向与扫掠方向点积正值剔除，随后才到D7A27的D7E30。

这证明三角面包装确有等端点专门路径，不能仅凭另一helper出现零轴归一化就判定完整球体扫掠一定失败；同时确认方向性面剔除，raw miss仍不能签发完整身体路径或占用许可。尚须认证CC030／CC820生成面、D7E30／CF9E0叶子语义、所有形状分派和当前参与组件／面策略及世界时相。本轮未开放Sphere sweep或零轴Capsule，未写身体或视图。

生产核验发现具体缺口：四个新追到的callee末字节各改1bit，原外层证书仍接受。先新增回归并用旧证书严格编译，180检查中4项按预期失败，原日志保留。随后cover_physx_query_code.h由22段10970字节增至26段16705字节，只增加这四个完整函数范围5735字节；来源绑定原PE节／文件SHA和Evidence完整范围。当前查询192项、独立球体overlap36项、完整命中归属122项全部退出0，逐段末字节篡改、截短及读失败检查保持。测试只将原DLL作为数据，原调用／模块pin为明确Host替身。

在原r104冻结源码上仅覆盖上述一个生产头文件完成Native编译，SHA256 66bf94166d40e2c449c928cc72689e346eef5b29ebb3b1edd15ff650669b2140；400项目依赖、无外部工作树依赖及原开关一致均通过。不是整个当前多武器脏树集成，旧r104／r105b运行候选未改、未stage／交付。四段证书是增量原代码完整性检查，不是全部传递调用图或几何正确性认证。

证据：analysis/cover-system-20261108/sweep-research-r106/verification.json、findings.json、certificate-ranges.json、四函数原始记录及before-v1／after-v1。实际r102 Code4首次拒绝层仍未知；恢复运行前继续遵循最新后台／静音及启动授权。完整CV-01–24／AC-01–22、当前占用／连续身体路径、实际C0提交／视图／命中合流、头肩遮挡、联合战斗及头显验收仍未完成，publicv71和工程25%保持。


## r107：原胶囊扫掠顶点100单位分支与请求域修复（2026-11-10，离线）

上一轮四段代码证书修复属于progress。本轮继续原PhysXCore 22051aa7…ddd48的离线只读分析，新增CC030／CC820／D7E30／CF9E0完整ASM与伪码。CC820先按原面法向与移动方向点积<=0保留面，再为每面生成7个三角面及原索引，并按需要反向生成面绕序；CC030由原box角点表构造12面后交给CC820。因此后续生成面换绕序不会补回已被拒绝的原面。

D7E30的顶点分支在D823B加载RVA22A510常量，D824C／D826C可拒绝投影下界或候选距离超限。原PE节字节0000c842精确解为float32的100；这是原查询参数的native单位，不将其自动宣称为人物世界100cm。独立算术见证：前方顶点投影150、半径1、垂距平方0.5，理论首次相交约149.292892、原调用者请求200，但该顶点分支会在固定100下界检查拒绝。此为绑定原ASM的分析推导，不是执行原DLL或完整原三角面调用重放；当前所有目标类型与世界实际分派仍待证实。

生产QueryEngine新增形状专用sweep_motion：胶囊closest及all-hit平移长度超100即原子拒绝Code::request，既不截短，也不将其记作miss；同时按double长度和原float平方和核对边界。Box输入域不因此收窄，胶囊自身轴长也不受此平移上限裁短；Sphere sweep／零轴Capsule仍未开放。外层Session的closest／stream入口同样先核对，避免先进入typed来源读取再拒绝。完整长路径仍须在共同身体路径层逐段处理和取得当前世界／终点占用证据，本修复不将较短请求冒充原整条路径成功。

先用旧QueryEngine复现4项超范围回归失败，再修复；外层新增回归首次因夹具AllSweep误写raw成员编译失败，保留session-before-v2，改正夹具后旧Session的2项行为回归确实失败，之后接通外层检查。最终严格检查退出均0：query210、sphere overlap36、完整命中归属127、Session96。覆盖100边界、两侧方向、三维长度、nextafter边界、长胶囊短平移、Box域保持、输出不变，以及已prime的真实QueryEngine/Host原调用缝处拒绝不dispatch。Host与只读原文件数据明确分开，无原DLL载入／执行。

原代码核验再绑定D7E30完整1207字节及常量4字节，28段共17916字节；不是完整传递图证书。基于r106冻结树，最终只覆盖query.h／query_code.h／physics_query_live.inl三个生产文件编译Native v2成功：SHA256 5ef02da97a29906d480e09ddfedfefe3795e5a8052229328b2d9d88fe3de5760，400项项目依赖、无外部工作树依赖、原开关保持。中间v1的081b1d23制品及原日志保留，未stage或交付任何本轮制品。

运行安排如实分开：期间收到修复版后台入口恢复授权后，仅重新prepare了cover-1110-r108-source-stage（仍使用r106 Native66bf9416、原PG／XR并补齐loader），派生输入live14项离线检查通过；未调用run、未创建claim或launcher-session。随后用户再次报告前台问题并撤回恢复授权；现automatic-runtime-pause.json仍为true，没有新game／XR启动或重试。自有入口也在消费claim前以及运行观察循环检查该标记，若运行中出现则走本方正常stop，不删除标记、不移动用户窗口、不抢回焦点。r108旧准备材料仅作为未运行记录保留，不据其认为后台已验收。

四函数provider已正常close，保留本次REA快照供后续精确读取，未修改原文件。证据：analysis/cover-system-20261108/sweep-research-r107/verification.json、vertex-cutoff-witness.json、cutoff-certificate.json及原反例／最终检查；独立准备另在isolated-r105/cover-1110-r108-source-stage/。r102来源Code4首拒绝仍待实际观测。完整CV-01–24／AC-01–22、连续身体路径／当前占用、实际C0提交与视图／命中合流、头肩遮挡、联合战斗及实体头显仍未完成；publicv71／工程25%不变，目标active。


## r109：完整胶囊平移路径的分段几何与终点准备（2026-11-10，离线）

上一轮原查询请求域修复属于progress。本轮保持automatic-runtime-pause=true，不启动game／XR，不改旧私有候选或用户窗口。新增cover_physx_capsule_path.h纯生产几何接口，并从现有查询Session头接入编译；它接收调用者提供的原胶囊、完整平移及显式精度补偿预算，准备全部分段和独立终点胶囊，不是实际查询执行、世界证据或身体提交。

采用最多64段的2次幂细分，逐轴验证step*count与原请求精确相等，禁止丢小轴、截短位移或只输出能处理的前缀。每段平移在原100 native单位域内；超预算、非正常数、非法／退化胶囊、别名、数值溢出、丢失小轴和终点退化均整笔拒绝，公共输出保持原字节。Scratch可变化，不作为成功结果。真实身体的关节旋转／姿态变化不被暗中当成整胶囊刚性平移。

各段起点和终点直接从原始端点及精确dyadic偏移重算，不累计世界坐标舍入。TwoSum保留double加法余项，以向外取整的L1误差界生成可表达的float半径补偿，补偿须落在调用者明确预算内；不修改人物尺寸或视觉骨长。相同分段平移使端点误差在该段时间内恒定，以端点误差凸组合覆盖整段胶囊轴，外加原球半径后覆盖原请求连续体积；接缝处共享的理想胶囊包含在相邻准备体中。另准备t=1的占用查询几何。该论证只针对提供的几何，不认证原PhysX数值求交、场景或完整身体。

独立精确分数检查通过27组计划／106段及所有终点，核对每轴整条位移不变、每段精确平方长度<=10000、实际float半径覆盖端点欧氏误差及报告误差上界，以凸性推到整段而非离散时间采样。删段、改一位运动参数、取消必要半径补偿的3个负控制全部拒绝。另有终点才出现的2^24坐标精度退化反例，证明不能因前面各段可准备就发布部分计划。

检查还复现一处新边界问题：主轴100加1e-30小轴，普通double／float求和丢掉小轴平方而误取1段；boundary-before-v7确实失败1项。新增向外平方和后取2段，小轴保留，最终精确分数检查通过。Nearest舍入模式只读核对，不擅改游戏浮点控制；独立测试切向上舍入后验证拒绝并恢复自身测试线程。首次Session严格编译发现参数workspace遮蔽夹具全局的C4459，保留session-v1，改名后最终C++几何88项／原Session96项退出0。

基于r107 v2冻结源，最终Native v3仅新增路径头及Session include，编译SHA256 111332fa51d38ae005662133aa6fe05172187592458ddf34ef2b44a1269ee705，401项项目依赖、无外部工作树依赖和原编译开关均通过。早期v1／v2编译、初始检查和边界失败完整保留；未stage、未交付或载入本轮DLL，原query／Sphere sweep许可不变，没有第二写入端。

证据：analysis/cover-system-20261108/capsule-path-r109/verification.json、exact-coverage.json及geometry-witness.jsonl。下一步仍须实际查询执行器在同一当前来源／世界约束下消费所有段与终点，处理命中、自身排除、等待、释放和陈旧结果，再接共同身体事务；Sphere／Box及关节姿态路径、完整当前形状覆盖与世界时相、r102 Code4首拒绝、实际C0／视图／命中合流、头肩遮挡、联合战斗和实体头显均未完成。完整CV-01–24／AC-01–22、publicv71及工程25%保持，目标active。


## r110：同一C0下的分段全命中查询与终点原始观测（2026-11-10，离线）

上一轮完整平移几何准备属于progress。本轮继续遵守automatic-runtime-pause=true，没有game／XR启动、安装或窗口操作。新增RawPath：保留原计划、每段命中区间／数量／回报数量、统一平铺的完整原始Hit及归属、首段类型来源快照，以及原始未过滤endpoint overlap布尔值。重复命中逐项保留，不用去重丢失分段来源；路径合计仍限制1024命中／256回报，不按段重置总预算。

现有Session::sweep_capsule_path在同一个串行owner操作内内部准备全部几何，逐段调用原有all-hit完整归属链，再调用原终点胶囊overlap。typed Physics／Collider／Components来源持续复核，后续段的来源快照须与首段一致，输入位移／预算／策略在guard前后复查；原C0、身份、原时间及100ms有效性不刷新。只有整条路径、终点及最终owner／metadata关闭复核都成功后，才把大结果从Session私有空间赋给公共输出。任何中途changed／stop／truncated／fault保留旧公共结果，不交出“成功前缀”。大归属缓冲和路径结果均在Session内，不放139KB级临时到实时调用栈，原调用外不持私有互斥锁。

实际C0入口及编译OFF分支已接线，复用原query准入：当前selected C0、已捕获未prepared／无pending提交、当前cover身份和同一源、原owner及metadata复核；fault_unknown仍请求原proof_failure正常停机，uncertain／pending不强关。默认查询开关保持，未增加按帧自动调用或身体写入端。该API返回的queried只指原始查询序列完成，不能从共同Actor／Scene指针推断物理世界没变化；endpoint overlap未排除自身，false和空命中都不授予占用许可。

最终严格检查均退出0：新路径执行187项、原Session96项、C0关闭36项。新检查记录实际传给Host原调用缝的四段起点／75单位移动与301终点、全命中顺序／重复归属、全空结果、8命中／4回报恰满仍能查终点、前缀预算不足不继续查、末段超预算、第二段来源／component变化、释放drain、输入变化、嵌套查询拒绝、原调用故障后保留pending，以及终点才来源变化时整笔不发布。模块／dispatch均为明确Host替身，真实typed读层与Session代码执行，不是原DLL／游戏动作证据。

原失败保留：checks-v1嵌套main夹具重名；checks-v2新夹具漏分配synthetic arena导致独立测试C0000005，已补VirtualAlloc／正常VirtualFree，且错误C0测试文件名导致原汇总中止；checks-v4 OFF夹具变量path与INI路径重名，改为path_output。它们不记作生产游戏失败或原调用异常；自有CPU进程终态已观察，全部修正后最终检查通过。详见initial-failures.json与原日志。

在r109最终冻结源上只覆盖C0 h／inl、Session inl、新观测头四个生产文件完成Native编译：SHA256 0da3cf99b9d340c8fbf5bd74749c8a356c9f0e716ea1622f2ef00fc8a59b5e6f，402项目依赖、无外部工作树依赖、原开关不变。本轮制品未stage／交付或载入；不是整个当前多武器脏树集成。证据analysis/cover-system-20261108/capsule-executor-r110/verification.json、checks-v5／checks-v4及native-build-result.json。

下一步仍需完整身体形状与姿态路径、当前世界／碰撞参与范围、精确自身排除和终点占用证明，再把结果关联到from-version及共同身体实际提交／视图／命中消费者；不能凭本轮raw结果解除原no_cover保护。r102 Code4首拒绝、头肩遮挡、真实抓边闭环、联合战斗与实体头显仍未验收，完整CV-01–24／AC-01–22、publicv71和工程25%保持，目标active。

## r111：逐命中自身来源证据与整条路径保留（2026-11-10）

在 r110 原始查询结果上增加 self_sources：只给当前查询已认证的具体形状记录人物碰撞体、Pawn 自有组件或主 Mesh 物理体的正向来源。形状所属公共 Actor、内部物理体、Scene、组件 BodyOwner 和主数组成员必须一致；附件根／清单成员身份不授予自身排除。任一外部组件或外部骨骼体与该 Actor 别名冲突、主 Mesh 归属缺失、内部体／场景不符时，标记为零，继续保留命中。主查询关闭全部 typed source 后计算，无额外原版调用、输入刷新或写入端。

所有原始命中、顺序与重复项继续保留，整条分段路径复制相应来源字节，空结果及尾部清零。正向标记仅证明这些已返回形状的来源，不证明完整自身集合、物理世界静止、空返回覆盖完整，也不授予身体移动；终点原始 overlap 仍未区分自身，尚不能放行。

CPU：OwnedSweep 144、CapsulePathSession 191、Session 96、OFF 36，均 exit0。真实生产 QueryEngine／Session 配 Host 模块与派发缝；没有执行原 DLL 或游戏。正向来源及内部体不符、外部组件／PAA 别名、场景／来源不符等为明确包变异检查，不能称实际游戏分支已覆盖。首轮输出目录父级缺失、以及 /WX C4459 测试全局名冲突均保存，修正后检查通过。

从 r110 冻结 Native 源仅覆盖 self_evidence、query、query_live 三文件，403 项项目依赖全在冻结树，开关不变。Native SHA256 `58bdbf940abb2e087ff845df7715087f452c2a71a581033caa3bb382d023f333`。仅编译，未安装／运行，公共 v71 不变。证据 `analysis/cover-system-20261108/self-evidence-r111/verification.json` 与 native-build-result.json。

收到主人最新“修复后台后继续跑”转达并核对 independent-runtime-20261110.md 最后一节；中央暂停标记已由维护方归档。后续仅以 cloak1110／绑定模拟器重新 prepare 全新本方实例，保留身份、新鲜度、静音、存档和正常清理。旧 background1110b 不复用，旧仅前台焦点检查不作为桌面不可见证明。先完成隐藏录制接线与定向 source-stage 运行，再推进终点占用、完整身体与实际提交；不向统筹主动发送占用／进度。

全目标及 CV-01–24／AC-01–22 未完成，工程 25% 不变；实际 source4、头肩遮挡及完整游戏／双眼／头显验收继续开放。

## r112：合成隐藏录制接线，动作启动失败保留（2026-11-10）

本方独立 runner 改用现有 ProjectionRecording 读取实例私有 GPU 双眼投影，取消对可见 HWND 录制的依赖；输入前等待两张新鲜帧，但不据此推断场景内容或玩法通过。每轮检查精确 game/XR 身份和 DWM CLOAKED；未隐藏、读取失败或本方进前台即停止动作并正常收尾。录制在 game/XR 停止前结束，避免身份消失造成工具竞态。实际 runner AST 的八项合成 Win32 检查通过，包含隐藏状态、读取／枚举失败、进前台、PID 复用与暂停；不作为游戏画面证据。

资源不足首次在 claim／启动之前拒绝，之后原占用者退出才启动 cover-1110-r112-cloak-source-stage。game55032／creation134387846856524106，XR58312／creation134387846934804956，Native r111 SHA58bdbf94…333。实际310次自有窗口 CLOAKED 检查，失败0；私有双眼投影3帧，无录制错误。两帧同内容启动等待卡＋最后一帧真实场景／双手已亲自查看，原图完整保留。

实际首失败是生成文件 operator.py 与 Python 标准 operator 模块重名，pathlib 导入发生循环，动作进程 exit1，未发掩体动作。probe attempted0／finished0，不能写成 source4 再现或碰撞通过。改名 cover_action.py，并在生成器中从实际输出目录执行标准库导入验证；修正新实例的同目录 bootstrap 与14项实际 live() 身份保护检查通过。

正常 stop0；Native恢复、入口恢复、手柄恢复均 true，XR0、launcher0，所有本方 game/owner/XR 已不存在，未强杀。统一武器 fire_disable2／hand_stop5／drainedfalse，整体 normal_cleanup_verified=false 原样保留，不能称全部清理通过；进程正常终结关闭本次内存对象生命周期，未取得 fire 停止确认这一失败仍开放。未回滚或改写共享源码解决该状态。

新的 cover-1110-r113-cloak-source-stage 使用同 r111 Native、冻结PG/XR及cloak1110准备，动作文件已修正；资源检查仍发现两个活跃自动实例时在 claim／启动前拒绝，不占运行、不操作他人。r113尚未启动／消耗，接下来只在资源允许后运行这一已修正的定向 source-stage 流程。r112不可重跑。证据 isolated-r105/r112-closeout.json、cloak-guard-check.json 与实例原始 run-result／projection，公共v71不变。

本轮不是掩体功能通过。原 source4 与头肩遮挡未解决，工程25%不变，CV-01–24／AC-01–22和物理头显继续开放。

## r113–r116：实际脚本路径修复与终点形状枚举取证（2026-11-10）

r113 在资源允许后启动，game59296／creation134387853785518413，XR53608／creation134387853851311494；Native仍r111。脚本已不再遮蔽标准 operator，但旧 wrapper 根据 parents[3] 构造依赖目录，迁移后误指 analysis/analysis，resolve_camera 导入失败，未发掩体输入；probe attempted0，不是 source4 重现。382 次本方窗口 CLOAKED 观察无失败，实际双眼3帧：前两帧与已审启动等待卡逐字节一致，最后真实场景和双手已亲自看图。完整原图与失败日志保留。

本方 Native／入口／手柄恢复true、XR0／launcher0，game／owner／XR 精确身份已不存在，未强杀。fire_disable2／hand_stop5／drainedfalse及 normal_cleanup_verified=false 原样保留，不能称全部收尾通过；实际查询、动作及早停开火确认问题仍未解决。

准备器改为显式绑定 canonical OLD，补 game-playtest 依赖；新增实际生成动作 --prepare-import-check，只执行依赖导入、原有源码转换与编译，早于游戏／输入部分退出，不修改产品规则。r115 新检查在启动前抓到缺失 game-playtest，失败保留且无game。修正后 r116 完成该实际预检和14项 live() 身份检查；0.4、原制品哈希、私有读写与新鲜度保护不变。r116 申请在两个其他活跃worker资源上限处被拒绝，尚无 claim／launcher-session，后续只在资源允许时运行；r113已消耗不可重跑。

等待资源期间对原SHA22051aa7…dd48 PhysX做只读静态取证。公共 Scene vtable+2f0 → RVA172160（完整201bytes），内部+248 → A9F10。该候选通过1AB130枚举，准确分支逐形状virtual+120，virtual+40返回公共形状，输出为8字节形状指针；与既有64字节 sweep Hit 不同。公共原count经1275A0尾跳188440作备用场景合并，不能仅据反编译void误判无返回。

重要接线边界：备用场景分支会按已返回数移动 caller buffer、减少容量；耗尽时还可能传null/0触发原分配，回调数据不保证等于初始batch。后续必须认证全部参数、缓冲区及回调预算／计数，不能直接复用现有等基址64字节Hit collector。准确参数9与候选-only分支要分清。此为静态候选，不是实际原版枚举调用；尚无终点占用、自身完整集合或移动许可，零Capsule／Sphere sweep仍不启用。

REA provider已关闭，完整snapshot及8条聚焦证据已保存。证据 overlap-enumeration-r114/{findings,selected-evidence,verification}.json。当前没有新的 Native／PG 实现或构建；公共v71不变。下一步先运行r116定位实际source4，再实现并校验终点形状枚举、完整身体路径和C0提交接线。工程25%及全部CV-01–24／AC-01–22、头肩遮挡、物理头显未完成状态不变。

## r117–r121：终点形状收集器、录制与独立读取器修复（2026-11-10）

新增 cover_physx_overlap_stream.h，按 r114 的九参数原版形状枚举 ABI 收集8字节公共形状指针，保留顺序／重复与所有报告；固定64项caller batch、1024项输出及报告预算。允许caller batch内的合法剩余子区间（含零count one-past），错位／越界／未认证的原分配缓冲区明确拒绝而非视为空。容量／报告耗尽保持truncated，返回count不能掩盖未收齐；坏receiver、跨线程、过期回调与别名不读陌生对象／不覆盖输出。原调用异常保留失败，不声称原锁或生命周期已恢复。

Sweep与Overlap共用线程内唯一reporter入口，双向拒绝重入，不增加锁或第二写入端。query.h只包含并编译该适配层，尚无原版枚举生产调用；现有终点仍是原始bool overlap。完整typed source、代码认证、逐形状归属、世界／变换闭合和终点占用还需接入；该collector不是移动许可，未知结果不放行。

最终CPU：OverlapStream54、原Stream40、Session96、CapsulePathSession191，均exit0。54项包括实际九参数适配器配合成callee、正常返回但callback截短、原调用SEH、子区间／尾项、错误缓冲区、预算与跨类型重入。v1的44／40／Owned144亦通过，随后补充边界和异常检查；不是原DLL执行。Native从r111冻结源仅叠加 overlap_stream、stream、query 三文件，404项依赖全在冻结树、开关不变。SHA256 `f529a65ecaf4e3f23581b67d92ba680555355b082a3b0e4901633217a3764347`，私有诊断使用，公共v71未动。

r118 game58584启动后未在七秒取得两帧，record线程第一次stop join也超时，只有一张已审启动等待卡；未启动动作／probe attempted0。保留原projection_error和缺失manifest，不补造录制通过；runner最终终结且game／owner／XR均正常消失。录制原因未被该次无分段记录唯一定位。

本方复制既有ProjectionRecording做局部阶段观察，记录identity、request、等文件、打开／加载／转换／保存及append；等文件期间响应原stop_event。输入前改为等实际 first_stereo_submission（XR单调时间与非runner观察标记）再开始录制；两帧分别五秒的原等待预算合计允许十二秒，不以Native running或等待卡当场景证明。失败后在同一owner正常停止后重等原capture线程，保留初次错误。12项实际recorder／就绪函数配合成BMP检查、8项实际cloak guard检查通过，非游戏证据。

r119 game54576与r120 game50960实际录制分别78／80帧，录制errors空、线程正常结束。两次共8张全跨度overview已逐页亲看，均为真实场景与双手；没有掩体动作，不是手部贴合或完整掩体通过。两份VFR编码exit0，原时间与图像保留。两次reader仍在输入前拒绝：旧断言分别只允许index running／session_owned；启动器on_report交接后 prepare_level 又写 waiting_for_level，不能把该索引阶段当最终游戏运行状态。该真实读取失败不是source4重现。

读取器保留精确实例、原session_id、game／owner／XR完整身份、XR制品和Source1核对；只允许已知交接索引 session_owned／waiting_for_level，且实际同编号session必须running。终态／导航态不放行。16项实际生成reader的合成文件／身份／Reader构造检查及14项live()检查通过，不刷新原样本时间／100ms边界。r121已用当前入口重新prepare并通过实际依赖导入检查，资源上限发现两个其他活跃实例后启动前拒绝，尚无claim／game；r118–120不可重跑，后续只运行已修正r121。

r118／119／120隐藏观察567／414／494次，失败均0。静音回读分别252／193／222次已静音、verify_failed0、target_shared_rejected0、initially_muted_sessions均1；muted0表示原已静音而无须本次写入，shared_rejected为全扫描累计，不当成本实例失败。均Native／入口／手柄恢复true，XR0／launcher0，精确自有进程已不存在，未强杀；三次fire_disable2／hand_stop5／drainedfalse、normal_cleanup_verified=false照实保留，不称全清理通过。保留早停开火确认缺口。

证据 overlap-collector-r117/verification.json、native-build-result.json、review-r119／review-r120/media.json 及各私有实例原报告；原始失败、等待卡与录制错误完整保留。source4根因、掩体头肩遮挡、终点认证及实际C0身体提交仍未解决，工程25%不变，CV-01–24／AC-01–22与物理头显继续开放。

## r122：完整胶囊重叠枚举事务与同源形状归属（2026-11-10）

新增 QueryEngine::overlap_all(Capsule)，提供 AllOverlap 和 AllOverlapOwners 原始观察。与原 sweep_all 共用同一 collect_all 事务、busy owner、原始调用计数、typed asset/collider/component 前后核对和最终发布；不复制第二套身份准入。每个返回的公共shape通过现有完整成员／内部体／Scene反链观察，再计算保守self_sources，保留外部及冲突命中、顺序和重复项。地址适配使用私有carrier复用归属观察，不把它伪称原版64字节sweep Hit，也不对外发布伪包。

新枚举独立校验 public vtable+2f0 与 internal+248，及五段完整过程范围：172160=201、A9F10=1233、1275A0=21、188440=291、1AB130=151，共1897bytes。范围来自完整Ghidra body而非第一个unwind片段；字节由原SHA22051aa7…dd48 DLL只读取得。枚举前、原返回后、逐shape归属结束后均复核。旧28段／17916bytes及原虚表认证保留，新校验只作用于新枚举，不给无关查询增加全局阻断。REA provider已关闭，原信息／汇编与完整snapshot保留。

支持合法caller-buffer子区间；截短、陌生fallback buffer、返回count不符与任何来源／代码变化均保留调用方旧输出。未知原调用异常仍pin模块、pending并禁止本owner重试；没有解锁／清理冒充。原始零返回只记观察，不签发占用或移动许可。零轴Capsule仍在dispatch前拒绝。

Session与C0入口接入同一原writer／当前捕获C0／未提交身体上下文；带归属的大结果留在Session私有成员，避免139KB游戏栈临时值。预算与胶囊输入在外部guard前后保持一致；停机／排空、部件替换及元数据变化不发布旧结果。关闭构建返回unavailable且输出不改，无每帧调用、输入注入或新的pose writer。

CPU：QueryEngine旧Query210／OwnedSweep144通过；新OverlapQuery最终87（真实事务、Host模块与原调用缝，原DLL仅作数据）、新OverlapSession53（真实typed roles／Session，派发和owner包为Host缝）、既有Session96／CapsulePath191、C0 OFF38，全部exit0。新测试涵盖五段尾部／两虚表变异、重入与pin、空与重复结果、错误buffer／count、预算截短、来源／代码／归属失败、原未知异常及停机。v1驱动漏给新程序DLL数据参数，exit2未执行检查；局部驱动修正后v2/v3通过，原失败保留，不当游戏／原调用错误。

Native从r117冻结源仅覆盖以上六个生产文件，405项项目依赖全在冻结树，无外部工作树依赖、开关不变。SHA256 `34c15590a10a1fa9f2f380ed0b2285afbd3d5889939e1c30f3f46542076dd1e9`。仅编译，未安装／运行；公共v71未改。保存后新运行需当前共享入口重新prepare独立实例，不复用旧入口哈希的r121。

重要未完成：新接口仅Capsule原始形状枚举；原分段路径终点仍为bool overlap，尚未替换为该接口，Sphere／Box及完整身体覆盖、transform／world fence、终点占用和C0实际身体／视图／命中／枪口提交都未证明。原source4仍未在修正后的输入路径触发。工程25%、头肩遮挡及全部CV-01–24／AC-01–22／物理头显未完成状态不变。证据 overlap-query-r122/verification.json、certificate.json、各检查和native-build-result.json。

## r123：实际掩体闭环恢复，source4定位到typed布局层（2026-11-10）

在保存r122后，用当前CLOAK／静音入口重新prepare本方独立实例 cover-1110-r123-cloak-source-stage。Native34c15590…d1e9、PGf8e6f3c2…807、XR16840b27…a7fc，0.4。game19480／creation134387893621457706，owner50296／creation134387893545464491，XR55936／creation134387893666015908。实际Source1读取通过，原版接近与CT2进入、五段中立／固定位置转头／回正／实际追踪上移0.10m／恢复全部执行，operator0；不是物理头显验收，也未做抓边／虚拟身体移动。

本次一厘米原始Sphere诊断 requested/attempted/finished均1，外层code4。outer returned6、session_entered1、Session physics_roles2／detail layout5、Engine idle0；original_started与returned前后均0。解释器按实际indexed session路径及本次Native SHA核对，first_rejection_layer=typed/physics_roles/layout；本次诊断尚未进入原版查询，raw_hit0仍是未知而非无障碍。Scene553／Compose33395／sequence2990／thread49740；读括号100616283951500→100616284724400ns。World2574680320/gen1、Pawn95491588352、Link2205697776/slot0，原始球(-3917.90698,-188.254379,621.074341,r1)保持原观察范围，米／单位几何尚非完整认证。

这是本次运行的具体失败层，不追溯冒称r102也是同一内部原因。typed sample已越过请求／初始对象读取，但layout仍包含schema、Owner类型、数组／root及body布局／成员检查，哪个字段拒绝尚不明；不靠放宽layout、选择任意Actor或把pending当空来修。下一步在现有读取路径记录有界布局见证并修实际不符项，原版调用、完整终点、世界覆盖和身体提交仍缺证。

235张实际双眼投影，无录制错误、thread joined；12张全跨度overview和5张原始阶段contact已全部亲自查看，VFR编码exit0。五个阶段均有自身头肩／躯干遮挡，视觉FAIL保持；前段也存在手部不显示帧，保留而不归因为新查询。阶段图只按事件墙钟区间选择（每段7张完整请求／观察落在区间内），不是nearest或同C0／GPU消费证明，不以数字读回替代画面。

正常stop与launcher0、Native恢复／入口恢复／手柄恢复true，fire_disable0／hand_stop5／drainedtrue，XR0，本方精确game／owner／XR均已不存在、未强杀，normal_cleanup_verified=true。此次正常关闭不改写早停实例的fire_disable2失败，也不称所有退出路径已修。CLOAK观察585次失败0；目标会话静音回读241次、initially_muted1、verify_failed0、target_shared_rejected0，guard正常结束，未动其他应用音量。

证据 overlap-query-r122/runtime-r123/{probe,closeout,media-review}.json 及 session-original.json 原报告副本，原报告SHA另存closeout。新Capsule枚举接口本次没有调用，仍只是编译／离线通过。公共v71不变，工程25%与全部CV-01–24／AC-01–22、头肩遮挡和物理头显未完成状态保持；无新增自动重跑请求，下一次必须是具体布局见证或修复的新候选。

## r124：typed布局拒绝的有界字段见证，源码与私有编译（2026-11-10）

r123实际尚未进入原版查询，当前继续定位，而非改宽准入。现有PhysicsRoles读取新增LayoutStep，按原顺序标记mesh/asset/instance类、每项属性及固定offset、数组inner、Owner引用、数组边界及Body成员布局；同时记录首次/复核pass、body序号、既有proof计数和已读取的属性offset。pass0不输出上次proof值。所有信息来自现有读取和工作区，不加额外游戏内存读取、原版调用、hook、writer或逐帧诊断；最后尝试的步骤不是单独的通过/安全许可，必须结合原Code和Session阶段解释。

Session同一串行owner导出见证，现有一次Sphere观察通过原summary保存。新查询开始清诊断状态；失败仍不发布调用方结果。body_layout同时供组件读取复用，改为可选见证参数，其他调用者保持原接口与判断。OFF路径无读取/guard/原版调用。

最终MSVC W4/WX/O2/UNDEBUG：PhysicsRoles282、Session105、OverlapSession53、CapsulePath191、C0 OFF38检查均exit0。新检查包含属性offset、inner类型宽度、Owner引用、body布局/重复、完整兄弟成员、失败输出不变、pass轮次、早期guard及Session拒绝前不dispatch；Host调用缝不当真实原版查询。首轮泛型Work编译C2664和driver误写不存在的c0_disabled测试名均保留，修为可选诊断参数及现有c0_off后通过；初次Native失败树另存failed-v1。

新Native严格以r122冻结树叠加3文件，405个项目依赖、外部工作树依赖0、开关不变，编译链接exit0。SHA256 f5d3cf695c33bb78bde9a3f1d99d1d9546043f549534b99e8eb20bb2f0e39850；未替换公共v71或固定手动入口。本记录时尚未启动新game/XR，实际具体字段拒绝、原版调用、完整占用/世界覆盖、唯一身体C0提交、头肩遮挡和物理头显仍缺证，工程25%与CV01–24/AC01–22未验收保持。下一步仅使用新候选执行一次CLOAK/静音/私有投影的定向验证。

证据 analysis/cover-system-20261108/layout-witness-r124/{verification,layout-steps,native-build-result}.json 及checks-v2/v3；源码先保存后新运行。

## r125：实际进入CT2与五段观察，布局诊断未到达（2026-11-10）

保存b40db9d7后，当前CLOAK/静音入口新建cover-1110-r125-cloak-layout；Native f5d3cf69…9850，PG/XR沿用匹配冻结版本，0.4。game54104/creation134387912043325145，owner59216/creation134387911939008697，XR55772/creation134387912123551617。Source1输入、原版接近及A进入CT2、neutral/turned/restored/raised/restored五段均实际执行，operator0；未做抓边或虚拟身体移动。

关键负结果：Sphere requested1但attempted0/finished0/code1，outer/Session/Engine/layout均idle0、pass0，原版started/returned0。此运行未到r124新布局检查，既不证明布局通过，也不复现r123具体layout5，更不能把raw_hit0当无碰撞。实际C0 captures693/commits376与原CT2已读到；固定源码唯一调用位于body_c0_goal_policy_live::sample_current_cover_pose在current/key/marker/measure/recheck之后。可定位下一观察范围为probe之前的C0准入/掩体姿态测量，哪个条件失败尚未证实，不删这些条件、不盲目重跑同候选。

286张实际双眼投影、录制错误0、线程joined，VFR编码exit0；15张全跨度overview及5张原始phase图全部亲自查看。进入前可见双手/手臂，进入后五段均有自身头肩/躯干严重遮挡，visualFAIL保持。phase图严格选事件区间内完整请求/观察括号（5/8/7/7/7帧可选），不称同C0或GPU消费证明；静止开场持续约16秒也保留。实际probe没有发生，不把“动作脚本passed”当掩体功能或原查询通过。

正常stop、Native/入口/手柄恢复true、fire_disable0/hand_stop5/drainedtrue，XR0/launcher0，本方精确game/owner/XR均已不存在，无强杀。CLOAK617次失败0；目标静音回读260次、verify_failed0/target_shared_rejected0，保留其他会话及系统音量。原报告按launcher-session索引原样复制，SHA与实际身份绑定；失败路径不改写此前早停记录。

证据layout-witness-r124/runtime-r125/{probe,closeout,media-review,session-original}.json。下一步先给现有sample_current_cover_pose的早退条件增加有界失败见证，再定向修复实际拒绝；r124布局见证保持可用。公共v71、完整CV01–24/AC01–22、工程25%及物理头显未完成状态不变。

## r126：掩体姿态测量前置拒绝见证，编译与定向检查（2026-11-10）

上轮r125已正常CT2但probe attempted0，本轮继续定位之前的掩体姿态测量。核对发现当前canonical早已包含035f9d7f的当前Tick测量重载与e60a3af7完成消费者核验，而冻结r124继承的r102候选仍使用更早Scene测量/原完成逻辑。版本差异成立，是否导致r125未测量仍未证明；不将差异直接说成根因，也不盲目切算法或混入共享在研源码。

在同一sample_current_cover_pose增加17类固定见证：stopping/idle、current、key/type/slot/link、marker及measurement原Status、closing key/current/stop/revision、缺entry、complete与exception。每项只保存首份不可变Scene/Compose/sequence/world/Pawn/原Status，固定计数；摘要通过acquire读已完成槽，不在原查询/读取期间持锁。不增加游戏内存读取、原调用或身体写者；保持每个谓词原顺序和原返回语义，complete只表示到达probe入口，不是查询/移动许可。

同一instrument变换分别用于canonical与r124冻结方法，并逐文本复核；冻结候选仅body_c0_goal_policy_live.inl一文件变化，405项项目依赖、外部工作树依赖0、开关不变，Native编译链接exit0，SHA256 d7cb104d3dcf7e77dfa127f28064d1b3cfbb3df2c6a8f16d416fee2c9c88c2e5。本次未启动游戏/XR，未替换公共v71或固定手动入口。canonical的Tick/完成消费者改动保持，但本私有DLL未暗中吸收它们。

实际Goal/adapter/marker/Backend/Kernel路径定向8场景（16–23）共158检查0失败，W4/WX/O2/UNDEBUG编译通过；身份/属性/Core/原查询为明确Host缝，未当游戏证据。共享手型在研导致首轮编译失败；隔离61e93fd1源码后只叠本方方法/测试，排除未使用的cosmetic include及不执行的mode>=26新API分支。首轮私有截取误删实际pose_begin，断言实际拒绝，改为精确括号范围恢复真实入口后v8全通过。失败编译、私有断言与准备错误完整保留，不改产品保护来通过。

证据pose-probe-r126/{verification,probe-stages,native-build-result}.json、checks-v8及prepare_checks.py/instrument.py。下一步使用新候选一次定向后台验证，先取得probe前拒绝项，再决定如何接回已存在的当前Tick测量。完整CV01–24/AC01–22、头肩遮挡、身体碰撞/占用/唯一C0提交与联合战斗、物理头显仍未通过；工程25%不变，目标active。

## r127：观察器注册表变化首败与一次完整重新解析修正（2026-11-10）

前轮0d734832为源码/定向检查进展。本轮新建cover-1110-r127-cloak-pose-gate，Native d7cb104d…c2e5，匹配PG/XR冻结版，0.4。首次资源检查先后见两组其他worker仍活，均在claim前拒绝；本方没有重绑票据或抢占。只读精确PID/creation/image与进程handle等待后，资源guard允许同一未消费候选实际启动一次；admission-wait.json保留等待，未作为玩法失败。

实际game42268/creation134387936291943740，owner58124/creation134387936211889401。实际epoch1双眼投影后，ordinary operator在第一次current()的sample_playground构造LiveRegistryAnchorSampler时抛AnchorRegistryChanged: Object registry changed since resolution，operator1。原版接近/A进入及五段phase均未执行，前置姿态各gate计数0、Sphere attempted0；本次不复现r125未测量的具体原因，也不把Native session passed或raw0当玩法通过/空碰撞。

129张实际投影、录制错误0/线程joined，VFR编码exit0；7张全跨度overview均亲自查看，只有起点场景与双臂/手部，局部约0.9s/9.8s有手部像素缺失，无掩体phase/contact验收。没有对本次未执行的掩体动作作视觉结论；r123/r125头肩遮挡FAIL保持。

正常stop、Native/entry/pad恢复true，fire_disable0/hand_stop5/drainedtrue、XR0/launcher0，本方game/owner/XR精确身份已不存在，无强杀。CLOAK505次失败0；目标静音读回216、initially_muted1/verify_failed0/target_shared_rejected0。原报告与operator原日志已复制并逐SHA绑定，早败保留。

针对这次真实观察器构造失败，read-only Playground adapter只在AnchorRegistryChanged时丢弃失败括号、关闭旧Resolver，并在同一session/game/owner/XR身份下最多完整重新发现一次；第二次仍失败、其他错误或人物/会话变化均终止。成功保留第一次拒绝原evidence和新样本原始时间，不修改旧证据/100ms/原guard，不发输入。8项实际adapter路径检查（读进程为Host缝）全通过，覆盖重建/关闭、二次失败、非registry错误、换会话、错误人物及原空UI约束。新operator generator将此adapter独立冻结为私有模块，runner校验散列，避免运行时无记录地变化。该修正尚未新游戏验证；Native不需重编。

证据pose-probe-r126/runtime-r127/{closeout,pose-probe,media-review,session-original}.json与operator-original.log/observer-checks.log，解析器7项合成报告检查通过。下一步用新鲜票据和冻结修正观察器定向进入，读取r126原拒绝见证；公共v71、工程25%和完整CV01–24/AC01–22/物理头显未完成状态保持。

## r128：C盘耗尽导致证据写失败，保留不完整收尾并恢复本方空间（2026-11-10）

b121d9c4保存r127观察器修正后，新票cover-1110-r128-cloak-registry完成实际冻结adapter导入检查，Native仍d7cb104d…c2e5，PG/XR不变，0.4；资源满时持续只读等待，释放后实际启动一次。game20160/creation134387945036784154，owner55412/creation134387944961903055，XR42940/creation134387945102409663。实际epoch1投影已发生，但read_private_input第一次写证据时ENOSPC，operator未取得输入观测；随后run-result.json及projection/recording.json最终写入也失败，二者保留原始0字节，不重建为成功报告。

现场确认C盘free0。启动器exit1，isolated-done记录owned_game_absent true/无强杀；另以精确PID/creation/image复核game/owner/XR/worker均不在。可读的原session与Native status仍是running旧快照，Native/entry/pad正常恢复、drained与XR exit均无终态回执，normal_cleanup_verified=false（未证），不能由进程消失补签正常清理。最后可读gate计数均0只是旧观察，不当最终查询证明。没有已验证的原版掩体进入、前置姿态拒绝或原查询执行。

保留51张实际JPEG且全部可解码，3张按文件序号的overview均亲自查看，只见起点场景与手臂；计时manifest已丢失，不编造帧时刻、phase对应或VFR视频。音频日志只剩一次mute_state_verified/verified_observations1，无整段summary，不称完整静音验收。原失败日志、最后running报告、Native status和0字节摘要均单独保留。

仅迁移本方已终态且精确进程已不存在的r118/r119/r120/r123/r125/r127旧实例至J:/gearsVr/analysis/cover-disk-recovery-20261110-r128，保留原C路径junction；r118/119/120/127全文件前后SHA一致，r123/125原10张phase图SHA一致。另将本方r124/r126的57份可重建obj/pdb（约64MB）移至同一J归档并逐SHA，源码、DLL、日志及其他会话文件均保留。初始复制后递归删除方案被自动审查拒绝，改用直接Move-Item归档加原路径映射完成，无单独递归删除；不绕开用户文件边界。空间回升约6GB，随后其他正常写入导致浮动，以closeout瞬时读数为准。

已只读审阅统筹新增共享require_disk：prepare、资源准入及worker真正启动前均检查证据卷至少4GiB，不终止现有实例或删文件；本会话未修改共享工具，也不改旧票hash绕过检查。后续使用新票绑定最新来源。证据pose-probe-r126/runtime-r128/{closeout,media-recovery}.json及原始日志。完整CV01–24/AC01–22、工程25%、公共v71、头肩遮挡及物理头显未完成状态保持；下一步仍是新磁盘准入下的定向掩体观察。

## r129/r130：磁盘准入真实拒绝；实际PhysicsAsset成员检查达到128身份见证上限（2026-11-10）

上轮e136b287为实际ENOSPC记录和本方空间恢复进展。本轮先复核r112/r113旧实例stopped及精确game/owner/XR/worker全不在，仅将这两组各约914MB归档J盘、全文件前后SHA一致并保留C路径junction。r129使用最新共享来源9f350220…345a准备，但真正资源准入时free3880353792<4294967296，require_disk明确拒绝；无worker/index/game，独立于玩法失败，未改旧票hash重用。自身runner新增claim前disk检查，原共享start/worker仍再查。改动已在当前Git历史c5bc7f54保存，运行r130实际加载的旧runner没有disk_before字段，不追称其已实跑新前置检查。

空间回升后，新票cover-1110-r130-cloak-pose-witness通过准备与冻结adapter真实导入；两实例资源占用时持续核对精确worker等待，资源释放后同一未消费票只启动一次。Native d7cb104d…c2e5、PG/XR冻结版、0.4；game58348/creation134387957377725571，owner58484/creation134387957292749552，XR55864/creation134387957447722033。Source1输入、原版接近/A进入CT2、五段neutral/turned/restored/raised/restored均执行，operator0。未做抓边/虚拟身体位移。

首次Sphere requested/attempted/finished均1，外层source4，Session physics_roles2/detail layout5，Engine idle0，original_started/returned前后均0。LayoutStep4=mesh_asset_property，pass1，mesh sources132/identities128，instance sources2/identities3，offsets584(0x248),0,0,0,0，body尚未采样。代码清零字段后，binding::property成功才写offset，因此已越过该属性自身解析，失败在role_property的完整成员闭合。身份数已达Proof固定128容量，容量耗尽是当前首要机制；本次未记录membership内部究竟哪个条件拒绝，仍不排除其他同时存在的不符项，不直接叫所有根因修复。

对应首次测量complete1的Scene410/Compose25204/sequence4508与Sphere同C0；之后measurement拒绝243次，原Status2=core，首份Scene411/Compose25266/sequence4509，是另一C0。不能拼成一次失败或据此单独断言时限/字段根因。Sphere读括号107011982808600→107011983642300ns，World2186117376/gen1、Pawn16800153856、Link2320974592/slot0，原球(-3930.43384,-190.230759,614.0177,r1)。无原版调用/完整空碰撞/移动许可。

308张实际双眼投影，错误0/线程joined，VFR编码0；16张全跨度overview与5张原始phase图全部亲自查看。五段仍严重自身头肩/躯干遮挡，visualFAIL；开场约20秒静止和间歇手部缺失保留。phase只按事件区间内完整请求/观察括号选取（7/3/7/6/7可选帧），不称同C0/GPU消费。新adapter完整操作序列通过，但registry重建分支本次是否实际触发为False，不将正常序列当该特殊分支已证。

本次正常stop，Native/entry/pad恢复true、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0、精确game/owner/XR均不在，无强杀；CLOAK744次失败0，目标静音读回317/initially_muted1/verify_failed0/target_shared_rejected0。r128缺失的恢复回执不因此补签。原报告原样复制并按Native SHA解释，证据pose-probe-r126/runtime-r130/{query-probe,pose-probe,closeout,media-review}.json。

下一步为typed完整成员见证提供有界分页/独立存储，维持全部兄弟、身份/字段/声明及closing验证，避免全局Proof扩容引起其他writer/栈尺寸变化；先离线复现跨128成员与末端突变，再编译定向修复。后续Core测量拒绝单独处理；完整当前占用/世界覆盖、原查询、唯一C0身体/视图/命中/枪口、抓边联合战斗和头显仍未完成。工程25%、公共v71与全部CV01–24/AC01–22状态保持，目标active。

## r131：独立有界成员见证修复128项上限，源码/定向检查/私有编译（2026-11-10）

上轮b85d3480已实际定位PhysicsAsset完整成员检查且shared Proof身份数128。本轮先在真实typed roles/registry代码中追加192个合法合成mesh sibling，before-v1严格复现offset0x248已解析、mesh_asset_property/layout5、identity128和调用方输出不变（8检查0失败，验证的是原缺陷）。未改旧游戏记录或把该before结果算玩法通过。

新增Cover专用MemberStore，固定最多4096个唯一成员及8192查找槽，保存完整Identity和原Next；先按原规则核对身份/declaring owner/唯一选名，再双读链指针、记录与原样比较。重复遍历复用同地址见证，内容不符拒绝；完整链仍保留4096 visited/cycle边界。结束时在同一metadata/锁/当前来源保护下逐成员重新读取身份与Next。存储满明确capacity、无截断成功；clear只重置活动长度/索引，旧scratch不参与输出，不在游戏栈创建整个大Store临时对象。

PhysicsRoles、ColliderRoles、ComponentRoles三个已有读取Workspace各拥有独立存储，所有生产role_property/pointer_property及body_layout都传入并在末尾闭合，支持选中属性之后的兄弟成员。公共weapon_hand_scene_binding::Proof保持128、布局与源码在冻结树逐SHA不变；不触碰其他writer或共享在研binding修改。没有新输入、hook、原调用、身体/骨骼/相机写者，也不从这些源见证签移动许可。Sphere摘要追加member_count/exhausted以便实际验证。

同一192成员fixture修复后完整通过，记录201成员且普通Proof不到128。50项新增检查保留public legacy路径128拒绝，覆盖末端循环/重复选名/错误owner、第二遍之后类型/Next/注册表变化、拒读、碰撞/存储耗尽/clear恢复。实际8程序合计1145项0失败（Member50/Physics282/Collider219/Component207/Session105/OverlapSession53/Path191/OFF38），MSVC W4/WX/O2/UNDEBUG；派发和内存为明确Host缝，不是游戏/原查询证据。首次检查目录父路径遗漏在编译前拒绝，创建本方目录后复现；未放宽警告。初版Native成功但审查调整component结束复核先metadata后成员，final重编与受影响4程序after-v2通过。

最终Native以r126冻结树仅叠5文件，406项目依赖、外部工作树依赖0、开关不变，编译链接0；SHA256 4a9a6f6b77c52538c75e19651a7ed20e233ab34e5025d0b7224fda9754bb744b。当前尚未stage或启动新game/XR；公共v71/固定手动入口保持。证据member-witness-r131/{verification,native-build-result}.json、before-v1与after-v1/v2。下一步保存本源码后新票定向进入，检查是否越过具体layout阻断再按实际下一拒绝修复；r130后续Core拒绝、完整当前占用/世界覆盖/唯一C0身体及视图/命中/枪口、抓边联合战斗与头显均未通过，工程25%及全部CV01–24/AC01–22保持。

## r132：完整成员闭合后原版Sphere首次实际执行；双眼遮挡仍失败（2026-11-10）

r131源码已保存d59ff30d。新独立实例cover-1110-r132-cloak-member-witness使用Native4a9a6f6b…744b，原冻结PG/XR、Source1、0.4；game51668/creation134387980269591055、owner18892/creation134387980196534942、XR31304/creation134387980465030763。原版接近/A进入CT2及五段观察完成，operator0；本次没有抓边或虚拟身体移动。

首次Sphere requested/attempted/finished均1，code0、raw_hit0；Session complete7、Engine complete10，original_started/returned均0→1。完整LayoutStep40/pass2，225项独立成员见证、capacity0，offsets584,592,96,108,104，结束复核通过。Scene447/Compose27519/sequence3345，世界2448851200/gen1、Pawn95227143808、Link2302462176/slot0；原球(-3914.51929,-154.767059,614.237732,r1)，读取括号109300844682600→109300863139300ns。这证明该次原始球查询调用并返回，不证明完整形状/世界覆盖、身体路径、终点占用或移动许可；新capsule枚举接口仍未在游戏调用。

姿态预检complete279，首份同Scene447/Compose27519/sequence3345；measurement/core拒绝54，首份Scene451/Compose27767/sequence3352。它们属于不同C0，不拼成一次事务，也不以数量对比宣称时限或测量根因修复。后续Core拒绝单独保留。

276张真实私有双眼投影、错误0、线程joined、编码0；14张全跨度overview和5张原始phase接触图全部亲自查看。中立、转头、恢复、抬高、恢复五段均严重自身头肩/躯干遮挡，visualFAIL；开头长静止及间歇手部可见性保留。phase严格按事件区间内完整请求/观察括号选图，不称同C0或GPU消费证明。

正常停止、Native/入口/pad恢复，fire_disable0/hand_stop5/drainedtrue，XR0/launcher0，无强杀，保存时精确game/owner/XR/worker均不在。CLOAK626次失败0，静音验证273/verify_failed0/target_shared_rejected0。实际disk_before free6725824512≥4294967296；不追补r128缺失恢复回执。r128与r130仅本方已消费实例全文件SHA前后一致归档J盘、原C路径junction保持，各921792452/1017521687字节，未动其他实例。

证据member-witness-r131/runtime-r132/{session-original,query-probe,pose-probe,media-review,closeout,additional-owned-archive}.json。下一步接完整身体路径/终点的形状枚举与当前世界覆盖，再连接唯一C0身体提交及同版视图/命中/枪口；自身头肩遮挡和Core拒绝仍需分别修复。全部CV01–24/AC01–22未完成，工程25%、公共v71、固定手动入口不变，目标active。

## r133：完整路径终点改接原版形状枚举与归属闭合（2026-11-10，未运行游戏）

上轮r132已按2bfd8918保存原版Sphere执行1/返回1及双眼遮挡FAIL。本轮沿CV-16完整路径/终点要求继续实现：RawPath终点由Overlap布尔值改为AllOverlapOwners，Session在最后一段后调用现有原版capsule overlap_all接口，保留每个原始shape顺序、重复项、owner及正向self来源。它与路径扫掠共用同一max_hits/max_reports剩余额度；额度已耗尽时拒绝终点查询，返回数量/报告超预算或raw_count不一致时拒绝。终点asset bodies/collider/component清单与路径任一不一致，整次操作changed，最终Session身份/输入/元数据closing保持。

较大的终点包仍置于既有序列化Session的RawPath中，原地清零，不引入游戏栈大型临时对象、新writer、hook或输入。空枚举仍不签发完整世界覆盖、自身排除、当前占用或身体移动许可；这些证据与实际消费者尚未接完。

四组实际typed-reader/Session离线检查共587项0失败：path391、Session105、overlapSession53、C0关闭38。覆盖原始形状/归属保留、空返回、跨所有查询共享预算、终点数量与报告超限、三类来源清单不一致、终点调用中身份变化/故障/停止/输入突变/重入及未完整操作不改调用者输出。原查询模块/dispatch使用Host fixture，不是游戏执行。

在r131冻结Native源上仅叠cover_physics_query_live.inl与cover_physx_capsule_path_observation.h，完整编译链接通过；Native SHA256 d23474316dbbaa272f94b3cbca09877abf265e18b5ec4def5b0ed74f21cd8d2b，406项项目依赖均来自冻结源，外部工作树依赖0，原GEARS开关不变。未stage、未启动游戏、未调用原版路径/终点枚举。证据path-endpoint-r133/{verification,native-build-result,native-source-manifest}.json与checks-v1/。

下一步认证实际身体体积路径和终点枚举、完整当前碰撞世界覆盖，并接唯一C0提交及同版视图/命中/枪口；自身头肩遮挡、Core测量拒绝和完整抓边/保持/重抓/战斗/恢复仍开放。工程25%、公共v71、固定入口和CV01–24/AC01–22总范围保持，目标active。

## r134：独立一次胶囊路径/终点原查询入口已编译（2026-11-10，启动前保存）

沿r133终点枚举接线新增明确私有cover_path_probe，仅受控simulator/continuous/observe-only可启用，与sphere probe互斥。沿用原有当前C0姿态测量入口与全部身份/原时间/当前cover/owner检查，一次尝试后不重试；实际调用现有sweep_capsule_path，段与终点共享32hits/8reports，最多2段。大RawPath包置于静态序列化scratch，调用不持有私有锁，完成后发布小回执并保留每项原始hit/shape/owner/self来源。故障沿既有停止/未确定清理机制处理。

用于此次调用认证的是当前测得eye处厘米级诊断胶囊及1厘米平移，不是假定人物碰撞体。现有CylinderComponent半径/高度及transform仍欠原版语义、有效shape和最终身体覆盖认证，不能直接以历史34/72合成正式身体。此诊断即使空返回也不给完整身体、世界覆盖、终点占用或移动许可。

在r133冻结源上只叠cover_physics_c0_live.inl，Native0098be93163f11c4f3007115d8f2154f484c9bf15338d59cbf5f440a048c37cb完整编译链接0，406项项目依赖、外部工作树依赖0、开关不变。关闭状态51检查0；精确SHA解析器9合成报告检查0。prepare增加r134精确来源选项，runner明确--probe path且拒绝无匹配能力票，原sphere默认保留；两脚本语法通过。尚未启动游戏。

磁盘准入实际3490852864<4GiB拒绝，本轮未因此启动游戏。确认r132精确game/owner/XR/worker不在且normal_cleanup后，将本方626文件/1009501963字节全哈希核对迁至J盘，原C路径junction保持，迁后哈希一致；其他实例未动，C空闲恢复7565504512。证据path-probe-r134/verification.json及J盘r132-hashes.json。

下一步只用新独立票定向调用路径及终点原查询，保留首失败、不改身份/释放/时效；之后继续完整身体几何、当前世界覆盖、唯一C0提交、视图/命中/枪口与遮挡修复。全部CV01–24/AC01–22未完成，工程25%、公共v71、固定入口保持，目标active。

## r135：实际启动前资源竞争拒绝；补齐未创建进程的收尾分类（2026-11-11）

r134源码已保存dc2d9874。新票cover-1110-r135-cloak-path准备与真实adapter导入0；等待时持续按精确worker身份复核资源，资源释放后前置准入available_physical15284228096、free8718278656通过。然而共享isolated_sim启动边界已被新Boltok实例先占用，require_memory在创建worker前明确拒绝，start_exit1。无worker/index/game/XR或原查询，本票claim消费保留，未重用或覆盖。原run-result与启动拒绝stderr完整保留；normal_cleanup_verified原值false，不补签正常清理，实际不需要清理。

自身runner原finally在这种明确未出生分支仍等100秒isolated-done。本轮增加严格分类：只对冻结共享启动器明确Two automatic instances/disk_admission预创建拒绝、exit1、stdout空、无worker和launcher-session回执时跳过不存在的结束等待。缺文件本身不作未出生证明；真实启动、其他故障仍保持原100秒和全部恢复核对。8项实际函数离线检查0，旧r135使用旧runner，不能称新跳过分支已被该次执行。

等待期间复用已编译EyeFaceInspect重新读r132真实283条CPU眼点几何，283匹配/0拒绝。五段完整CPU读括号内分别4/6/6/7/7条，模型眼骨中点到authored眼点约3raw units，base forward与authored face约82.6–87.3度；前者吻合原mount，后者未含最终HMD旋转，不能据此断言最终视线错误或唯一遮挡根因。不添加任意相机偏移；最终双眼遮挡仍FAIL。新证据path-probe-r134/r132-eye-{face,phase-comparison}.json和runtime-r135/prebirth-refusal.json。

下一次用新票继续同一r134候选的第一次实际路径/终点调用；本轮资源拒绝不算游戏复查，不重跑已通过Sphere。完整身体几何/世界覆盖、C0同版提交/视图/受击/枪口、遮挡和全部CV01–24/AC01–22保持未完成，目标active。

## r136：实际路径在source_repeat/guard拒绝，原版调用0；正常关闭与双眼失败已核验（2026-11-11）

r135预创建拒绝及等待分类修复已保存626c6dd0。新票cover-1111-r136-cloak-path在精确worker资源释放后只启动一次，Native0098be93…37cb、冻结PG/XR、Source1、0.4、path模式，sphere requested/attempted均0。game16040/creation134388003952038420、owner21152/creation134388003872202839、XR55760/creation134388004017415947，worker47672/creation134388003871254438。原版接近/A进入CT2及五段观察完成，operator0；没有抓边或虚拟身体移动。

path requested/attempted/finished均1，code6=changed；Session engine5/detail6，Engine source_repeat3/source_code2=guard、exception0，original_started/returned前后均0。LayoutStep40、225独立成员见证、capacity0，已越过physics/collider/component初始读取，未越过引擎第二次来源复核。source::capture返回guard只证明其调用的当前来源guard为false，尚未区分内部typed读闭合、身份、元数据、原时间或owner等具体条件，不猜计时/身份根因、不删除复核。零hit/endpoint/segment字段是未成功输出，不是空碰撞证据。

本次Scene431/Compose26633/sequence3588/thread25192，World2312601856/gen1、Pawn95194022016、Link2309112576/slot0；诊断胶囊(-3921.29443,-148.927185,615.215271)→(-3921.29443,-148.927185,617.215271),r1，delta(1,0,0)，括号111654972425700→111655005645500ns。仍是厘米级ABI控制，非正式身体体积。姿态complete316，首份同该C0；measurement/core24首份Scene448/Compose27715/sequence3615，属于另一C0，分开保留。

251张实际私有双眼投影、录制错误0/线程joined、编码0；13页完整跨度overview和5张原始phase图全部亲自查看。头肩/躯干仍遮挡双眼大块区域，visualFAIL。phase完整请求/观察括号内各7可选帧，不称同C0/GPU消费。正常stop，Native/入口/pad恢复、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0；精确game/owner/XR/worker均不在，无强杀。CLOAK618次失败0，静音验证248/initially_muted1/verify_failed0/target_shared_rejected0；启动disk_before7454486528≥4GiB。新prebirth分类在本次为false，真实启动清理分支保持；不拿此运行冒充跳过等待特殊分支实测。

原报告原样复制并按精确SHA解释，解析器新增本次source_repeat/guard拒绝案例后10项合成检查0。证据path-probe-r134/runtime-r136/{session-original,path-probe,pose-probe,media-review,closeout}.json。下一步记录来源guard首个具体拒绝谓词，定向修复后再查；不重跑未改Sphere或同一失败路径，不放宽原始时效/身份/释放。完整身体/世界覆盖、唯一C0提交及同版视图/命中/枪口、遮挡与全部CV01–24/AC01–22仍未完成。工程25%、公共v71和固定手动入口保持，目标active。

## r137：来源guard首个拒绝谓词见证已编译（2026-11-11，启动前保存）

上轮r136实际source_repeat/guard拒绝和正常清理已保存20f47ea4。本轮仅分解原有同序短路判定并记录首失败，不增加游戏读取、时间采样、锁、hook或writer，不改变原时间/身份/释放/metadata/当前来源条件。外层OwnerStage记录session/captured/prepared/pending/entry、两份source、ActualContext.current及其原rejection/rejection_detail、cover读/同一键、metadata读/同一及backend metadata。每次query用局部小Witness保留首拒绝，返回时发布到原线程本地诊断，嵌套调用不会覆盖外层局部记录。

SessionStage分别记录stopped、physics/collider/component读取与相同快照、query_source/asset/collider/component匹配，typed读取失败保留原Code。每个已接纳操作在busy检查后清空见证，重入拒绝不覆盖外层；后续拒绝不覆盖首项。最终小Observation随现有一次path诊断回执发布，没有移动/占用许可。

实际typed reader/Session离线五组623检查0：guard23、Session105、path391、overlapSession53、C0关闭51；新检查覆盖首项锁定、跨操作重置、owner拒绝/原始source错配/停止的具体分支和调用者输出保持。模块/原dispatch为Host替身，无游戏或原查询执行。见证解析器9案例0。

在r134冻结源上仅叠cover_physics_c0_live.inl、cover_physics_query_live.inl、cover_query_guard_observation.h，Native10880b1c1e259adf56a09ad238efe4a35d070d98f2a6dec5ff5c575dff066ff5完整编译链接0；407项项目依赖、工作树外部依赖0、开关不变。prepare增加r137精确来源选择，保持独立path模式准入。尚未stage或启动游戏，下一次只定向复查此前路径的首个guard拒绝。

确认r136精确game/owner/XR/worker全不在及normal_cleanup后，仅将本方533文件/998890209字节录像实例完整归档J盘，前后全文件SHA一致、原C路径junction保持。C空闲恢复10776584192；未动其他实例。证据guard-witness-r137/{verification,native-build-result,native-source-manifest}.json与checks-v1/，归档manifest为J盘cover-1111-r136-cloak-path-hashes.json。

完整身体几何/当前世界覆盖、实际路径/终点原调用、唯一C0提交及同版视图/命中/枪口、自身头肩遮挡和CV01–24/AC01–22仍未完成，工程25%、公共v71与固定入口不变，目标active。

## r138：真实首拒绝为ActualContext来源过期，未放宽时限（2026-11-11）

r137源码先保存b0a73357后，新票cover-1111-r138-cloak-guard在两实例资源释放后只启动一次。Native10880b1c…6ff5、冻结PG/XR、Source1、0.4、独立path模式；game58172/creation134388022483876946、owner54612/creation134388022438460371、XR51104/creation134388022549958139、worker58904/creation134388022437680932。原版接近/A进入CT2及五段观察完成，operator0；无抓边或虚拟身体移动。

path requested/attempted/finished均1，code6；Session engine5/detail6，Engine source_repeat3/source2=guard，original_started/returned前后均0。LayoutStep40、225成员见证、capacity0。新增外层见证21,21,8,3,6明确第21次Owner guard在ActualContext.current返回false，原Verdict3=source_age_expired、detail6；内层4,4,2,3,0表示第4次SourceGuard在physics_read得到typed Code3=guard。当前冻结ActualContext的detail6来自身体来源/装备来源时效判断，hard依赖先检查；它尚不区分其中哪项age失败，未观测起始age，不将本次37.2984ms自身耗时直接等同完整source age。不称身份失败，也不改原时间、延长时限或删除二次复核。

本次Scene395/Compose24378/sequence3992/thread36648，World2319417600/gen1、Pawn95208479360、Link2314748672/slot0；诊断capsule(-3920.83569,-190.812454,621.348755)→(-3920.83569,-190.812454,623.348755),r1，delta(1,0,0)，括号113513466828200→113513504126600ns。姿态complete2首份同该C0；measurement/core328首份Scene399/Compose24623/sequence4000，是另一C0。零segments/hits/endpoint是未成功输出，不是空碰撞证明，原版路径/终点均未调用。

267张实际双眼投影、录制错误0/线程joined、编码0；14页全跨度overview及5张原始phase图全部亲自查看。各段仍严重自身头肩/躯干遮挡，visualFAIL；长静止开头及间歇手部可见性保留。phase完整请求/观察区间内各7可选帧，不称同C0/GPU证明。正常stop，Native/入口/pad恢复、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0；保存时精确game/owner/XR/worker全不在，无强杀。CLOAK660次失败0，静音验证274/initially_muted1/verify_failed0/target_shared_rejected0，启动disk_before5136269312≥4GiB。

证据guard-witness-r137/runtime-r138/{session-original,guard-probe,pose-probe,media-review,closeout}.json。下一步减少重复全反射解析的读取成本，保留完整兄弟链/身份/声明/字段闭合及原时效，先定向证明等价拒绝与读量，再编译复查；若须进一步拆分年龄源，沿现有拒绝码记录，绝不刷新旧数据时间。完整身体/世界覆盖、实际路径与终点调用、唯一C0同版视图/命中/枪口、头肩遮挡及CV01–24/AC01–22仍未完成。工程25%、公共v71和固定入口保持，目标active。

## r139：单次采样内复用完整成员链，保留全身份/Next结束复核（2026-11-11，未运行）

r138实际来源过期及正常清理已保存2f172a9c。本轮针对重复读取成本实现同一次typed sample内的完整链证书：第一次仍遍历全部兄弟、声明outer、唯一名字与相同selected身份，完整结束后才记录owner/head及唯一成员区间；之后同owner属性只从该完整区间查找，仍将当前head纳入原Proof。未知/不完整/容量不足不发证书，head不同、重复或不同selected均拒绝。所有成员的完整身份和原Next仍由原有members.current结束重读，所有字段/声明/元数据Proof关闭保持；sample clear即失效，不跨采样、Tick、人物或metadata复用，不更新任何原时间。

实际同一typed reader/192额外成员的前后对照：原完整观察34440 reads，新24696，减少28.2927%；首个membership均3106，第二个3106→2。这里只是合成注册内存fixture的实际读取计数，不是游戏延迟或来源过期已修复。原版调用/输入/writer/时限未变。修复前后基线与两份旧源码均保留member-chain-r139/before-*。

十组受影响离线检查1404项0，加显式cache读量阈值的24项0；覆盖末端cycle/重名/outer/身份/注册表/Next变化、cache后链头/末端变化、容量、Legacy Proof128仍拒绝大链、全部typed physics/collider/component及Session/path/overlap/guard/C0关闭。默认benchmark23项包含在1404中，显式24项为同一程序带阈值复查，不重复算成独立覆盖。

在r137冻结源上只叠cover_member_witness_live.inl及cover_physics_roles_live.inl，Native8814ea68079dcb16b6815f3d72dd32ec3d5a7fdf2395d6302a5d19fb4eccc32b完整编译链接0；407项目依赖、外部工作树0、开关不变。固定链证书表置于既有Workspace，clear仅清count/索引，无大型栈临时。准备器增加r139精确来源选择，guard解析器接受调用者显式匹配SHA、默认仍拒绝其他artifact。尚未stage/启动游戏，下一步只复查来源过期这一处。

证据member-chain-r139/{verification,native-build-result,native-source-manifest}.json及before-v1/after-v1/cached-threshold.log。完整身体几何/当前世界覆盖、实际路径/终点、唯一C0同版视图/命中/枪口、自身遮挡及CV01–24/AC01–22仍开放，工程25%、公共v71及固定入口保持，目标active。

## r140：激活前严格anchor采集超时，优化Native未执行（2026-11-11）

r139源码及离线读量先保存b7a9f8cd；新票cover-1111-r140-cloak-chain资源准入后启动，game34692/creation134388043499140447、owner39240/creation134388043409774082、XR28276/creation134388043703130168、worker13816/creation134388043408916129。启动disk_before5821665280≥4GiB。候选8814ea68…cc32b按SHA复制正确，但激活前只读anchor配置采集在initial获取拒绝，native_activation_attempted=false、native_started=false、native_status=null，未执行operator或记录私有玩法projection，原查询0。不能将此说成成员链缓存或路径逻辑运行失败，也不能声称优化已在游戏通过。

原始config-failed回执显示单次严格采集strict_guard_accepted=true、无语义异常，但耗时3536.1894ms；本次获取的原deadline为200ms，故acquisition accepted=false，理由Anchor guard acquisition deadline exceeded。计时115625775453000→115629311651100ns，拒绝发生在注入候选Native之前。未延长门限、未改旧样本时间或自动重试，初始原因和完整raw guard保留；慢读的进一步原因尚未确定。

launcher1、Native cleanup false/error incomplete or deadline、entry restore无证明、XR1（terminal exiting without end、对象销毁和shutdown完成），input pad恢复true。本轮normal_cleanup_verified=false完整保留，不补签；isolated-cleanup-pending旧回执owned_game_absent=false也不修改。其后独立isolated-done为owned_game_absent=true，保存时精确game/owner/XR/worker全不在，remaining_owned空、无强杀；进程退出与正常恢复证明分开。CLOAK405次失败0，静音验证367/initially_muted1/verify_failed0/target_shared_rejected0，不虚构玩法画面或编码。

证据member-chain-r139/runtime-r140/{session-original,config-failure-original,closeout}.json及prepare/operator日志。下一步检查本次激活前配置获取的慢路径，保留严格获取/身份/新鲜度条件；r139实际查询效果仍待新候选复查。已有离线1404与读量减少28.3%不替代此项实际结果。完整身体/世界覆盖、唯一C0同版视图/命中/枪口、自身遮挡及CV01–24/AC01–22继续未完成，工程25%、公共v71与固定入口不变，目标active。

## r141：激活前 CameraActor 发现路径收窄（2026-11-11，源码／离线）

- r140 的 3536.1894 ms 严格采集超时仍按原始失败保留：Native 未激活，r139 DLL 未执行；本次没有子步骤实测，不能认定 CameraActor 查找是唯一原因。
- `cinematic_anchor.py` 冷查找复用现有 Resolver 的批量类候选发现，避免逐个解码普通游戏实例名称；已有 CameraActor 缓存候选仍先处理，兼容无该能力的原精确查找。只改变发现提示，选中对象仍经过 fresh 注册表／名称／类证明、完整证书复核及时间戳括号内两次目标验证。
- 额外拒绝提示称 CameraActor、但实时名称或 number 已变的候选；发现提示不能更名当前类。原 200 ms 获取期限、身份、新鲜度、输入及恢复保护全部保持，不把发现移到期限外，也不续时。
- 41 项相关离线检查全部通过，包括真实生产类扫描器与合成反射内存接线、热缓存避免全扫描、假提示、注册表后变、扫描失败和冷发现超过原期限。独立 771 对象固定内存比较取得相同候选，旧逐对象查找 771 次读取、现有类提示扫描 7 次；只证明此合成读取量，不是实际延迟或 r140 因果修复。
- 配置组有两项既有失败：`test_guard_attempts_exhaustion_not_retried_by_sample_layer` 和 `test_sample_after_guard_exhaustion_keeps_actual_partial_sample`。修改前 HEAD 的 cinematic 源码与修改后分别复跑同组，均为相同两项失败（160 对 5、32 对 1）；不报告整组全绿，也不在此改动采样重试策略。
- 本次尚未启动游戏／XR、未更换任何 DLL 或公开 v71；r139 实际查询耗时、完整身体路径与占用、抓边移动、遮蔽／受击／枪口及头显仍待验证。工程进度 25% 保持。下次先绑定实际启动脚本哈希，再使用新私有实例定向验证启动与已有 path 查询；不复用已消费 r140。
- 证据：`analysis/cover-system-20261108/camera-discovery-r141/verification.json`、对应基线／修改后日志及 `verify.py`。本轮是源码与验证证据进展，完整 CV-01–24／AC-01–22 目标继续 active。

## r142：严格启动采集通过，姿态 core 门未到查询入口（2026-11-11）

先保存583a74af，准备新票cover-1111-r142-cloak-discovery，并绑定8个实际anchor启动来源SHA；结束后逐项一致。等待真实worker资源释放后准入，disk_before128625233920≥4GiB。game11788/creation134388064387918283、owner21348/creation134388064304674851、XR39640/creation134388064477169381、worker31316/creation134388064303553040。Native仍8814ea68…cc32b，PG/XR保留原冻结版本；公开v71不变。

实际anchor五次guard均在原200ms内accepted：initial59.6409ms、sample_before1.7749ms、sample_after6.3687ms、assembly_after2.0053ms、final_boundary1.547ms。Native启动、实际projection及原A进入CT2与五段eye观察完成。只证明本次采集成功，不以跨进程59.64对3536.19宣称速度倍率或r140唯一因果。

path requested1、attempted0、finished0，owner/session guard调用均0，原查询started/returned均0；r139成员链优化的实际调用收益仍未被执行验证。既有姿态first-failure统计只有measurement258次/detail2=core，首证Scene340/Compose20953/seq4277；complete0。精确冻结body_goal_view_adapter规定50ms且core_snapshot_valid是多条件谓词，当前回执没有失败分项，不能把全部失败定性为年龄、布局或本次优化回归；也不能用最后一次状态拼成首证同C0。

302实际私有projection帧/errors0、编码0；16页全跨度总览与5张原始contact已亲看，五个阶段头／肩／躯干模型严重遮挡双眼，画面验收FAIL。每阶段整段落内7/7/7/7/8张候选；取中间contact只说明时间段，非同C0／GPU骨证明。没有抓边或虚拟身体移动，不宣称路径／占用安全。

launcher0、Native/entry/input恢复、武器fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常，独立精确身份复核全不在，无强杀。CLOAK717/失败0、前台453/owned_activation false；静音305/initial1/失败0/共享目标拒绝0。原始session、anchor、run-result及done保存于camera-discovery-r141/runtime-r142，保留r140原失败。

下一窄点是在现有测量first-failure记录中区分core谓词失败及原始时间，不重读游戏、不刷新来源、不放宽50ms或100ms。不要原样重跑本批。完整身体几何与世界覆盖、实际抓边移动、身体遮挡／暴露／枪口／受击、CV01–24／AC01–22仍未完成；工程25%和目标active保持。

## r143：同一测量首个 core 拒绝的只读分项候选（2026-11-11，未实跑）

r142实际measurement258/core、query attempted0保留。新cover_measurement_core_observation.h只检查已持有数据：记录原policy.now、50ms上限、原publication/HMD/controller时间、票据/引用代次/头部标记、严格Core与原non-age谓词、控制器当前匹配，以及独立Tick模式的实际classify/rejection_detail和metadata/Tick原时间。未来/无效时钟不转换为巨大无符号年龄；旧Core年龄诊断与Tick分类分开，不把旧Scene过期直接解释为当前Tick过期。

沿用既有measurement首个失败CAS槽，同一条Scene/Compose/seq下附加cover_pose_probe_core_8 CSV；只在detail=core的首个失败写一次，release/acquire发布，不新增game read、clock、锁、writer、查询或自动重试。原接受条件、源选择、顺序和关闭复核保持；canonical及精确冻结测量函数去除observer参数后的文本与各自之前逐字相同。

专用合成数据50项通过，覆盖50ms边界/超界、publication/头部/控制器分别过期、未来时间、引用奇数、空票据、非法flags、旧Scene与合法当前Tick区分、Tick年龄/硬拒绝及输入字节保持；CSV解释8项通过。第一版复用旧通用Core测试因固定ABI尺寸/偏移断言与当前头不匹配而编译失败，日志保留，未改旧测试。第二版边界夹具时钟排列不合法而失败，按有效cohort顺序修正后第三版50项通过；没有放宽产品校验。

完整Native在r139原冻结树只改body_c0_goal_policy_live.inl并新增一个观察头，SHA5522ac2a49119075662a24176601bee613eec20467d672cdadeefe54d04ec093；408项目依赖均来自冻结树、外部worktree依赖0、GEARS开关相同、编译链接0。当前主线的Tick来源分支与冻结旧Core分支分别原样保留，未混入共享未提交Native改动。prepare新增--native-r143，只接受这两个delta与上述冻结编译证明。

本轮未启动游戏/XR，不能认定r142的core首败是年龄或其他具体条件，也没有新的查询/身体移动/画面通过证据。下一次用新票限定读取首个实际拒绝分项，保留50ms/100ms与原身份、输入、恢复检查；不重跑未改玩法矩阵。r142模型遮挡FAIL、完整身体/世界碰撞覆盖、CV01–24/AC01–22与头显仍开放；工程25%、publicv71、固定手动入口不变，目标active。

## r145：首个测量拒绝确定为旧头部样本58.7917ms（2026-11-11）

r144票在最终共享准入前被其他worker占用空位，start1、stdout空、明确Two automatic instances拒绝，无worker/index/game；claim保留消耗，normal_cleanup_required=false，不补签正常收尾，不算一次游戏。随后新票r145继续等真实worker资源释放，未放宽2实例/内存/4GiB规则。

r145实际game32320/creation134388087505037769，Native5522ac2a…ec093、同冻结PG/XR；启动anchor各获取最终均在原200ms内接受，8个anchor来源SHA前后一致。A进入CT2、五段eye观察完成，measurement241次core、complete0、path requested1/attempted0/finished0、原查询0。

首个同一测量证据Scene337/Compose20751/seq5433：mode1旧Core，strict0/non_age1/controllers_match1、expired2，仅原头部样本超过50ms。原now120038702219900、stereo_received120038643428200，年龄58.7917ms；publication120038664074200，年龄38.1457ms；controller_received120038643459500，年龄58.7604ms仍在其100ms内。票据338/引用代次6/size1864/ABI1/flags7/status1。证明这个首证拒绝是头部年龄，不将同一结果扩张到全部241次或倒推r142首证。

现主线已有035f9d7f的BodyTickSource测量接口：当前追踪40ms、元数据50ms、原classify及当前CoverKey前后检查；私有冻结测量仍只用旧Core。下一修复应只把既有当前Tick测量重载/接线补入冻结候选，保留旧Core路径原拒绝和原始所有时间，不通过更新旧Core或扩大时限放行；尚未编译/实跑此后续修复。

323实际私有projection帧/errors0、编码0；17页完整跨度总览和5原始contact亲看，五阶段模型头肩/躯干严重遮挡双眼，仍FAIL。每阶段7个完整落入动作时间段的候选，不能当同C0/GPU骨证明。launcher0、Native/entry/input恢复、fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常，独立精确身份全不在、无强杀；CLOAK860/失败0，前台524/owned_activation false，静音349/initial1/失败0/共享拒绝0。disk_before104385679360。原始session/anchor/run-result/done及同测量分项已保存core-measurement-r143/runtime-r145。

本轮实际根因证据是进展；完整身体/世界碰撞覆盖、实际抓边移动、遮挡/暴露/枪口/受击、CV01–24/AC01–22及头显仍未完成。工程25%、publicv71和固定手动入口不变，目标active。

## r146：冻结候选补入既有当前Tick测量接线（2026-11-11，已编译未实跑）

依据r145首个原头部58.7917ms/50ms拒绝，复用2f042279中主线已有的measure_cover_pose_math及Core/BodyTickSource两个测量重载，按原文区块补入r143冻结body_goal_view_adapter；其余place/prepare/目标转换与共享Native保持冻结。已有BodyTickSource开启时使用context.body_source，否则沿用原Core。诊断同样按实际分支记录mode，不再把V3的身份parent年龄当成其当前追踪年龄。

原V3 classify与当前CoverType/Link/slot准入及结束复核保持；独立追踪40ms、元数据50ms、旧Core50ms均不变。测量观察时间取本次metadata_end_ns，Scene/publication/ticket和原追踪时间逐字保留，不修改Core、不重取输入、不授予查询/身体写入/移动许可。当前主线本来已有此接线，本次是实际私有冻结候选的窄整合，不用整个共享脏树重建。

31项实际冻结API离线检查通过：老Core仍拒绝、合法当前Tick测量可用、同原C0几何不重复叠加HMD、原Input/Frame字节保持、绑定原metadata_end/Scene/publication、8类过期/引用/publisher/cohort/未来时间负例及覆盖Input的别名拒绝。前两版借用head-only Core夹具缺少合法独立controller cohort，原classify以hard0xcd08拒绝，失败日志保留；第三版使用既有完整Core/控制器夹具后通过，没有改产品判据。骨骼数据来自保存的实际reference/C0，只验证数学/读取契约，不称实际身体移动。

完整Native编译链接0，SHA49f76f0135f8393ff7035afc51450401e484f4a10cf95c6c7511090c1201a472；r143上精确2文件delta、408项目依赖、外部Native worktree依赖0、GEARS开关相同。测试头依赖另逐项核SHA，只有明确的自有测试夹具来自主目录。prepare新增--native-r146并验证该冻结delta；本轮没有再开游戏。下一次用新票限定复查测量完成及原path查询，不重复未改玩法矩阵。

r145只是首个实际旧头部超时的原因证据；r146未实跑，不能称查询、碰撞覆盖或画面修复通过。模型遮挡、完整身体与世界几何、抓边/松手保持/重抓、暴露/命中/枪口、CV01–24/AC01–22及头显保持开放；工程25%、publicv71与固定入口不变，目标active。

## r148：当前Tick测量完成并到达path尝试，组件阶段上下文年龄拒绝（2026-11-11）

先保存68b81740及8e00075a：共享首次准入已支持锁外有界等待60秒，本方启动timeout从30兼容为90秒，并绑定isolated_sim/runtime_receipts新SHA；原名额2、内存、4GiB、身份、暂停检查仍由锁内及worker执行。本方advisory只对明确ResourceUnavailable转入共享等待，不吞其他错误；20项只读兼容检查通过。r147外层见空位后内层advisory被抢占，未产生claim/start-request/worker/index/game，原失败另存，使用新名r148。未修改旧prepared哈希或重用失败请求。

r148实际game54628/creation134388103051301203、owner44832/creation134388102957884329、XR9880/creation134388103149022211、worker35196/creation134388102956219331。Native49f76f01…1a472、冻结PG/XR不变；anchor initial58.0816ms，其余2.0393/5.7539/1.9463/2.0571ms均在原200ms内接受，八个anchor来源前后一致。共享工具SHAe6b4a0ae…42c2b/7db0a084…abefb按新准备记录核对，不替换成历史9f3502哈希。

body_mount_source_enabled1。原A进入CT2，测量complete1的Scene325/Compose20038/seq5513/world2212528384/pawn95434014848与path尝试键逐项相同。此前旧Core年龄门本轮没有记录为测量首败；另有measurement257/detail4=neutral，首证Scene327/Compose20163/seq5519，具体中立数学谓词尚未记录，不能混成同一个C0或称所有姿态都可用。

path requested/attempted/finished1，code4，Session stage4 component_roles/detail3；Owner guard第7次拒绝，context/source_age_expired/detail6，Session SourceGuard尚未进入。layout40/member225/未超容量，engine stage0，原始started/returned均0。query时间121595420167000→121595455453800，35.2868ms；不能拿这段自身耗时当原来源总年龄或判断body/equipment哪个过期。诊断capsule仍1raw半径、2raw高、1raw平移，不是完整身体/世界安全证明。V3测量接线这次确实到达后续消费者，但完整path查询仍未执行。

321实际projection帧/errors0，编码0；17页全跨度总览与5原始contact已亲看，五段模型头肩/躯干遮挡仍FAIL，时间段候选7/7/7/7/8，不声称同C0/GPU骨/头显证明。launcher0、Native/entry/input恢复、fire_disable0/hand_stop5/drained、XR0、gamegone/worker正常、独立身份全不在、无强杀。CLOAK860/失败0、前台534/owned_activation false，静音340/initial1/失败0/共享拒绝0，disk_before87473856512。

修正报告reader将path attempted纳入query_attempted（之前只看sphere），9项解释自测通过；不是改运行行为或补造原调用。下一窄点复用现有source_rejected实际cause bits，区分prepare阶段本次body/tracking年龄与equipment年龄；当前commit-only journal不能证明该分项。不放宽时限或重取/续时旧输入，不原样重跑此批。源/原始结果/画面在current-measurement-r146/runtime-r148；完整CV01–24/AC01–22仍开放，工程25%、publicv71及固定入口不变，目标active。

## r149：私有查询同次prepare年龄原因观察候选（2026-11-11，已编译未实跑）

r148组件读取阶段Owner context/source_age_expired/detail6没有body/equipment分项，不能凭35.2868ms推断总年龄。原ActualContext已将原因传入ready_wrist::source_rejected：bit1为BodyTickSource::source_age_only，bit2为equipment_fresh=false；原journal只记录commit，因此不覆盖本次prepare失败。

新增cover_query_source_cause.h纯记录器，只有private sphere/path查询在原context.current(prepare)调用范围临时安装线程局部Scope；按Entry地址、Compose、thread、prepare与source_age_expired精确匹配，只收首个已有回调原因。相同地址晚一Compose、错线程/阶段/判词、嵌套查询和Scope结束后的事件不能串入；退出/异常恢复原Scope，不保留栈指针。ready_wrist原commit journal筛选与行为不动；默认无private查询时不安装观察。没有额外game read/clock/锁/输入/lease/guard/engine调用或writer。

query原RAII发布旁附加原因副本，现有path一发结果保存并输出cover_path_probe_source_age_cause七项。解释器先核精确Native，再核同一Compose/thread及Owner context/detail6才解释两位原因；未观察时保持unknown，不将0视为新鲜或通过。只解释现有拒绝，不授予碰撞/身体移动许可。

实际纯记录器25项通过，覆盖关闭/空key/入口、Compose、thread、stage、verdict不符，其他真实CPU线程、嵌套、异常退出、首证不覆盖和地址重用；解释15项通过。canonical与精确冻结query函数去掉观察代码后，原guard顺序/判据逐字相同，原source_rejected commit筛选亦逐字相同。源码按各自原文窄变换，不把共享其他Native改动带入冻结树。

完整Native在r146冻结上仅改ready_wrist_entry_live.inl、cover_physics_c0_live.inl并新增一个头；编译链接0，SHAac5cf37f3683a20900d3927510e56ab42cb0528080f0ea31f360f3537344ebda。409项目依赖全部冻结，外部Native worktree依赖0，GEARS开关相同。prepare新增--native-r149，只接受这3文件delta。原测量、查询和当前BodyTick来源/装备时限未改。

本轮未再启动游戏，r148具体哪一位为真仍未知，不称已定位或修复。下一次新票只取得同次原因及原查询结果，不重复未改玩法矩阵，不续时旧来源。measurement neutral257的实际数学分项、完整身体/世界覆盖、抓边/松手保持/重抓、遮挡/受击/枪口及CV01–24/AC01–22仍开放；工程25%、publicv71和固定入口不变，目标active。

## r150：原版A未进入矮掩体，年龄分项诊断未触发（2026-11-11）

基于已保存24e0d72c，用新实例cover-1111-r150-cloak-cause准备r149 Native ac5cf37f…ebda，冻结PG/XR及共享隔离工具哈希不变。等待进程实际存活并观察原名额拒绝，资源释放后只调用既有runner一次；没有重用旧claim或启动请求，没有修改原时限、身份及输入保护。

实际game34696/creation134388127400340663、worker4360/creation134388127260774890。原始A之后CoverType仍0，operator1并保存Original A did not enter low cover。未发送pose_request，五段eye_observation未执行，因此本轮没有可用的同查询年龄原因；不能将未观察到cause解释为新鲜，也不能声称r149已定位body/equipment分项。进入前人物(-3982.51294,-145.28207,579)，控制yaw -4218；A之后(-3484.23877,-271.63196,583.015625)，仍CT0。与r148相同候选槽位但实际接近轨迹不同，原因尚未证明，不能归咎Native观察器或直接重跑。

202实际投影帧/errors0；11页全跨度已逐页亲看，编码0，原始五阶段接触图0张，不补造。前段可见持续初始场景，后段导航、滚动及身体穿插；完整掩体画面未验收。Native/entry/input/武器恢复、XR及launcher0、正常清理true，无强杀，独立核对本实例精确身份均不存在。原失败、session、操作记录和媒体哈希保存在query-age-cause-r149/runtime-r150。

下一步先核对本次普通进掩体前后的导航/方向/距离及输入记录，针对实际失败改进后再用新实例继续r149同查询诊断；不放宽任何来源年龄或碰撞证明。完整CV01–24/AC01–22仍开放，工程25%、publicv71和固定入口不变，目标active。

### r150后普通A入口观察补充（源码与离线检查）

只在本方prepare_operator冻结转换加入entry_observation：既有0.15秒前进后、唯一原版A之前，保存sample_playground实际状态、读取起止时间及原始报告；原A一次、时长1和finally释放0.8秒保持原代码。观察及保存会延长前进保持的真实时间，已明确记录该限制，不宣称与旧运行时序完全相同。六项离线检查实际转换及原tap成功/异常后的finally释放，通过0；拒绝缺失/多处/重复转换。未启动新游戏，不证明入口已修复。下一实例仍须原版CT2实际认证后才可请求r149查询。

## r151准入未出生、r152普通步行受阻与定时脉冲修正（2026-11-11）

r151等待活进程观察到名额释放后共享锁内准入再次被抢占；原60秒等待耗尽，typed ResourceUnavailable、exit1、stdout空，无worker.json/log、launcher-session或game。保留claim/start-request和原stderr，normal_cleanup_required false、verified false，不补造正常清理。8490509b已保存，未重用请求。

新r152 Native ac5cf37f…ebda、PG/XR和共享工具哈希不变。准备时本方未先等前一步句柄终态，首次operator准备因prepared.json尚未存在退出1；随后确认前一步0且未产生operator/claim，重新完成准备和导入检查，资源等待仍在继续，没有因此启动两次游戏。本失误与实际游戏失败分别保存。实际game38096/creation134388136853985148、worker51936/creation134388136771900244。原版A之前的步行已失败Controlled walking blocked before cover；未发A、未取before_original_A、未发pose_request、五段eye_observation未执行，同查询年龄分项仍未知。

十个实际motion观测：距接近点161.303→113.885→207.802→341.589→233.565→269.479→180.252，最后四位置完全相同(-3913.15576,-307.79456,579)。显示越过目标后反复方向修正，最后触发原停滞限制；不能只据此认定碰撞或死区原因。175投影帧/errors0，9页全跨度亲看，编码0，接触阶段0；后段身体穿插/遮挡仍FAIL。Native/entries/input/武器/XR/launcher正常结束，独立精确身份均不在、无强杀；closeout及原始结果在query-age-cause-r149/runtime-r152。

检查驱动发现ctx.stick采用固定14次渐变发布，ctx.live/put耗时叠加到每一档保持；本次相邻motion间隔0.74–1.56秒，不能当作已测每次发布延迟，但固定次数时序缺陷由代码可见。修正cover_ordinary_approach改为实际经过时间推进0.14秒升/降渐变，沿用原0.04/0.22秒中段、0.62幅度、35到达界、3次停滞及42步上限；同一原g.put发布端、每次身份检查及finally零输入释放，没有新线程/写入端或位置/旋转setter。慢调用仍可能超时，记录实际时长和超时量，不声称硬实时保证，也不能声称入口已修好。

11项离线检查通过，包括实际脉冲慢检查/慢发布/发布异常/finally释放/停钟上限及原导航限制；初次仅浮点比较0.6799999999999999与0.68失败，改为近似断言，产品未为测试改门。prepare_operator将实际approach源码封入本实例operator并记录SHA，避免之后共享文件变化改变已准备动作。r153新私有实例已准备、实际冻结脚本导入检查0，仅准备未运行。下一步用此已保存候选做一次定向入口/查询检查，保留r150/r152实际失败，不重跑旧claim。完整CV01–24/AC01–22仍开放，工程25%、publicv71及固定入口不变，目标active。

## r153：启动线程消失87拒绝，原冻结不变、完整事务有界重试修正（2026-11-11）

基于bf57aec6准备的r153等待活句柄观察原两名额占用，资源释放后单次启动。实际game39264/creation134388146522999539、owner53828/creation134388146430178398、XR39540/creation134388146609309585。XR preflight6.324秒后，before_activation/start_gamepad/background_input.change/FrozenThreads.OpenThread出现WinError87：枚举到tid36712，已挂起15条；新的同进程快照明确failed_tid_still_present false，失败线程已消失。实际异常堆栈及原诊断已保留，不归咎掩体、Native查询或新步行脉冲。

Native continuous native_started false，operator未启动、projection0、未输入A/步行/pose_request，步行修正和查询年龄分项均未取得运行证据。launcher1/XR1，normal_cleanup_verified false；后续精确game/owner/XR/worker身份均已不存在、无强杀，这一进程事实不替代正常清理。原始结果、首次失败和堆栈在query-age-cause-r149/runtime-r153-startup。

未改当前已有未提交的background_input.py，也未跳过任何线程。playground_loader增加_enable_gamepad_gate，仅当原FrozenThreads已完整展开并抛出带唯一见证的OpenThread87、失败tid来自旧枚举且在新快照消失时，允许最多3次完整enable事务；每次保留check_live、精确创建身份、完整status原代码认证，不接受enabled/pending/未知代码，不重复Native Start或GateLease。其他87、仍存活线程、观察失败、权限/恢复错误均直接拒绝；第三次不再重试。成功报告保留原异常和见证，失败保留重试历史。原Freeze的异常恢复失败会变RuntimeError，不能进入此重试。源码与离线检查不证明实际恢复或入口已修好。

26项startup/loader检查及9项background_input检查通过，包含真实start_gamepad函数的模拟依赖调用、首败留存、身份变化/代码未知拒绝、次数界及原回滚/释放；没有调用游戏/真实冻结。prepare现在绑定原8项来源以及loader和background_input两项启动依赖SHA，已有准备记录不改哈希，新运行须重新准备。下一步保存后使用新私有实例定向验证启动、定时步行及原CT2入口，再继续原查询原因定位。完整CV01–24/AC01–22、工程25%、publicv71/固定入口及实体头显状态不变，目标active。

## r154准入未出生与共享有界等待参数接线（2026-11-11）

r154已准备绑定10项启动/anchor来源，操作导入检查0；等待活句柄持续观察名额占用，外层见名额后，原共享first_admission再次争用，60秒届满typed ResourceUnavailable退出1、stdout空。无worker.json/log、launcher-session和game，normal_cleanup_required false、verified false，原claim/start-request和拒绝原文另存runtime-r154-prebirth。没有启动/输入/投影或正常清理证据，启动/脉冲/查询修正尚未实证。

本方runner增加--admission-wait-seconds，默认保持60；只接入共享工具原本支持的整数0..600秒上限，subprocess超时为该预算+原30秒调度余量，参数及实际调用均记录。12项实际参数/dispatch检查及20项原准入兼容检查通过，无资源预留或游戏。下一新请求直接使用现有runner和共享600秒等待，避免外层先等空位再竞争；共享锁外等待、锁内名额2/内存/磁盘4GiB/暂停/身份/新实例检查、首次commit后不重试全部保持。未编辑共享准入工具，不修改或重用旧票据。目标active，完整掩体未完成，25%/publicv71不变。

## r155：定时步行与原CT2进入通过，同查询明确body年龄拒绝、equipment未过期（2026-11-11）

在d264443c保存后，新r155直接调用既有runner --admission-wait-seconds600；共享锁外等待/锁内原准入最终接受，未复用r154请求或放宽名额、内存/磁盘。Native ac5cf37f…ebda及冻结PG/XR不变，10项anchor/启动来源前后相同。实际game58564/creation134388167581202750、worker47468/creation134388167497036032。后台输入门首次成功、attempts1、startup_thread_exit_retries空，正常启动链通过，但未实测新87重试分支。

三个普通步行脉冲planned0.320秒，actual均约0.328秒、samples12/13/13、overrun约8ms，随后原版A确实进入CT2，五段eye观察完成，operator0。新before_A观察记录(-3963.17554,-139.47527,579)、yaw -5792、CT0，实际读取+保存窗口16.8663ms；这个额外耗时仍明确保留，不宣称与旧运行完全同序。该次限定接近/入口已触达，不代表所有路径或掩体玩法通过。

complete2首键Scene400/Compose24726/seq3953/world2248507648/pawn16826695936与path键逐项相同；另measurement334/detail4=neutral首键Scene404/Compose24971/seq3961，不能混成同C0或判定全部测量通过。path requested/attempted/finished，code6；Owner第21次context/source_age_expired/detail6，Session第4次physics_read/guard，实际第一拒绝层engine/source_repeat/guard；engineStage3/sourceCode2，原started/returned0/0。时间128023511475700→128023547451200，共35.9755ms。

本次匹配prepare原因见证observed1/entry14674320/Compose24726/thread55064/stage0/verdict3/causes1：body_source_age true、equipment_age false。这是同查询实际上下文回调，确定本次body年龄分支拒绝且equipment在该次检查仍新鲜；不能细分tracking40ms、metadata50ms、capture或已烧毁age，也不能倒推r138/r148相同分项。仍不刷新旧输入/时间、不放宽年龄门。下一窄点为复核重复physics/source读取开销及source至query调度，保留完整关闭检查；不原样重跑此候选。

259实际投影帧/errors0，编码0；13页全跨度与5原始contact均亲看，候选7/7/7/8/7，五段自身头肩/躯干遮挡仍明显FAIL，接触图另存哈希，不声称同C0/GPU或头显验收。Native/entries/input恢复、fire_disable0/hand_stop5/drained、XR0、launcher0、normal_cleanup true，无强杀，独立精确身份全部不在；CLOAK660/失败0、前台415/owned_activation false，disk_before14328074240。原始session/launcher/操作/查询/媒体在query-age-cause-r149/runtime-r155。完整CV01–24/AC01–22、工程25%、publicv71及固定入口仍不变，目标active。

## r156：逐布局/逐遍mesh关系认证去重，源码/离线/编译完成、未运行（2026-11-11）

依据r155同query body年龄拒绝/equipment未过期，检查实际physics复核链发现values在每个BodyInstance上重复认证同一mesh class及OwnerComponent声明关系。只在现有BodyLayout保存成功键(mesh_class、reference、pass)，同布局同一遍后续部件复用；pass2重新认证，每次sample原有layout清零仍执行，不跨sample/查询/帧缓存。所有部件实际identity/index/owner/main/auxiliary仍两遍读取，完整layouts_current的metadata/proof/所有成员关闭检查、owner guard及全部年龄门原文未改。不刷新来源或减少身体部件，也没有第二写入端。

actual冻结r149基线与新单文件delta两边编译真实typed reader。2部件safe_copy读取7224→7076（2.05%），16部件9800→7580（22.65%），64部件18632→9308（50.04%）。这是该合成reader整次sample的调用量，不是实际游戏毫秒、全query成本、原始碰撞调用或年龄门修复证明；实际BodyInstance数量与全query收益仍待测。

checks-v2相关consumer/guard/path/overlap/OFF各程序均0，原physics282项保持。补充checks-v3聚焦两遍中途变更：在pass1和pass2首个部件认证后分别改变声明reference、mesh父类、mesh实例类、非root owner、成员Next和metadata代次，均拒绝并保留原输出；同时核对pass/class/reference键改变、pass0不缓存及关闭证明拒绝。新focused执行388检查含原282，不能相加当作全新覆盖。checks-v2的8个其他相关程序分别282/219/207/23/105/391/53/51通过，不重复跑未改consumer；CPU fixture不证明真实引擎调用。初次checks-v1因冻结运行源码不含CPU测试翻译单元而编译失败，失败保留；仅补入测试cpp到独立检查树，运行头仍冻结，无产品门更改。

Native ababb095c3fd5a24ff37a368d99a895cecf4d4dc06e07ddfd769dd670a39f2d8 编译链接0。exact r149冻结树仅cover_physics_roles_live.inl一处运行文件delta，409项目依赖均来自本冻结树、外部worktree0，GEARS flags不变。verify确认原sample事务/全部dynamic读取（除该重复class调用）/layouts_current原文保持，既有r155证据在新SHA下拒绝，inspect_cause新增显式expected_native支持，默认仍严格r149。prepare支持--native-r156并认证单文件delta、未staged/flags/依赖；尚未准备/运行新实例。

源、比较、初失败、依赖、构建manifest和核对在mesh-role-repeat-r156。下一步Git保存后，以新私有实例一次定向检查原CT2入口、实际query层次/耗时及画面；不原样重跑r155，不放宽source_age或其他守卫。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口和实体头显状态不变，目标active。

## r157：新去重候选实际CT2通过、collider复核仍body年龄拒绝（2026-11-11）

eb8c46b5源码/构建保存后，以新cover-1111-r157-cloak-meshrole、Native ababb095…f2d8及冻结PG/XR，既有runner共享600秒准入做一次定向运行。实际game10688/creation134388186580061229、owner3196/creation134388186507862975、XR56032/creation134388186649960118、worker25076/creation134388186506656610。十项准备时anchor/启动来源前后相同。原A进入CT2、五段eye观察与operator0，Native/entry/input/武器正常恢复，XR0、launcher0、正常清理true、无强杀；独立精确进程身份均不在。

complete2首键Scene396/Compose24437/seq3244/world2321973504/pawn95227484800与本path逐项相同；另measurement266/detail4 neutral首键Scene400/Compose24682/seq3252，不能混合。path code4，first engine/source_begin/guard；Owner第15次context/source_age_expired/detail6，Session第2次collider_read/detailguard，engineStage1/sourceCode2。匹配原因entry14674320/Compose24437/thread25376/stage0/verdict3/causes1，仍body_source_age true/equipment_age false，原started/returned0/0。query129914469800400→129914502250100，32.4497ms；与r155的35.9755ms处在不同拒绝阶段且开始年龄/调度未匹配，不能据此称实际全query提速，更不能称年龄故障修复。

277实际投影帧/errors0，编码0；14页全跨度及5原始contact均亲看，阶段候选8/7/7/7/8，五段自身头肩/躯干遮挡仍FAIL，图像哈希与原图副本保留，不声称同C0/GPU或头显证明。CLOAK590/失败0，前台373/owned_activation false；disk_before5430648832，仍过原4GiB门但余量不多，下一运行前须重新检查，不删他人或未清理实例。

等待期间收到共享launch_reserve.py出生回执原子发布窄修正通知：最多6次同payload替换，仅Win5/32/33、重试前整份原回执相等，非启动/注入重试；本方未改该共享文件，未回复/转发或为此新增运行。在本方worker登记前另记source SHA a4c44d9d…99edf及对应只读证据，结束后源文件SHA相同；不改旧prepared哈希，不把源码前后观察当作精确已加载版本或真实Win5修复验证。此后新prepare将launch_reserve作为第11项启动来源绑定，当前r157仍明确是原10项加独立观察。

原始session/launcher/动作/新artifact解释/媒体/关闭结果在mesh-role-repeat-r156/runtime-r157。已证明的合成读取量收益保留，实际来源年龄与画面缺口仍在；下一窄点继续检查collider/source复核成本及来源到query调度，不放宽新鲜度、不刷新旧样本、不原样重跑。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及实体头显状态不变，目标active。

## r158：第二遍重复schema查找改为完整既有证明重读，已编译未运行（2026-11-11）

r157仍在collider_read期间body年龄拒绝且原query0，故继续实际physics/collider复核路径。两个typed reader第一遍仍完整选择/认证原属性与类型；第二遍保留fresh对象identity、原始动态字段及数值条件，但不重新搜索第一遍已认证的schema，而先复核与第一遍完全相同的对象身份/metadata代次，并用原binding::current完整重读对应Proof全部identity与source字段，再消费缓存offset。collider按Pawn/Component/Body三段分别在其动态读取前复核；第一遍无BodyInstance而第二遍出现对象时直接changed，不读取未建立body字段。每次sample原清空证明仍在，不跨sample/帧/query复用。最终members.current与全部Proof关闭检查原文不变，样本入口/出口owner guard事务除last-pass选择参数外原文不变，所有原动态读取调用及physics动态尾部保持；没有改任何年龄、输入或移动写入门。

actual冻结r156对照与新两文件覆盖均通过同一focused738检查（包含原physics282+collider219，不称738全新项），覆盖两遍中途offset、kind、reference、父类、成员Next、inner宽度、metadata变化，以及body空/非空双向转换与失败保持原输出。8个相关consumer/guard/path/overlap/OFF程序分别282/219/207/23/105/391/53/51全0。所有检查树的运行头来自冻结树，测试cpp单独复制，实际依赖无外部worktree。这些是合成数据/Host seam，不是原引擎调用或完整物理证明。

相对r156该层sample的safe_copy次数：physics2/16/64部件分别7076→6313、7580→6817、9308→8545（10.78%/10.07%/8.20%）；collider有body的两种body_data状态均7455→6005（19.45%），无body5407→4571（15.46%）。该计数不是实际query毫秒，不能把不同层收益叠加当真实全链加速，更不能据此称年龄拒绝已修复。

Native58c2fe1d9fa2a108010f33008278060f9ad3a2ccde53d3d87f3e27c829680c7f编译链接0；exact r156冻结树仅cover_physics_roles_live.inl和cover_collider_roles_live.inl两运行文件delta，409项目依赖、外部worktree0、GEARS flags保持。delta.patch/verification.json核对完整关闭和动态调用；prepare新增--native-r158认证精确delta/flags/构建状态，后续启动来源含launch_reserve在内11项绑定。源、对照、测试依赖与构建记录在schema-repeat-r158。本条仅源码/离线/编译，未准备或启动新实例，不替换publicv71和固定入口。

下一步Git保存后准备新私有实例，做一次原CT2/同query实际拒绝或原调用及画面检查，保留r155/r157年龄失败；不刷新旧样本、不放宽年龄、不原样重跑r157。完整CV01–24/AC01–22仍开放，工程25%、实体头显未验收，目标active。

### r159仅准备与共享预启动正证更新核对

r159准备时绑定到新的isolated_sim/runtime_receipts SHA7e630461…eaa8/385f5e5a…eee3；启动前发现变化并停止进入启动步骤。核对python-prelaunch-rejection-20261111.json四项实际源SHA，且把当前两个共享工具中仅有的新typed prelaunch cleanup分支反向移除后，恢复到此前e6b4a0ae/7db0a084精确哈希，证明原准入/等待/资源/身份代码未改。launcher/Start-DebugGame的新正证须exact receipt路径、脚本SHA、三种attempt均false、出生回执不存在；worker仅在exact自有launcher退出后采用，不伪造game identity或owned_game_absent，旧worker不热更新。不回复/转发，不启动额外检查游戏；复用来源记录的5/81/15离线结果，未称本方重新执行。

r159无claim/start-request/worker/index，原准备记录不改、不复用此名；下一新实例将launcher.py和Start-DebugGame.ps1补为第12/13项启动绑定，另仍绑定共享两个准入工具SHA。详情schema-repeat-r158/shared-startup-review.json。Native58c2fe1d候选仍未运行，完整目标active。

## r160名额/r161磁盘出生前拒绝与本方关闭缓存归档（2026-11-11）

r160准备/操作导入检查通过，Native58c2fe1d候选及13项启动绑定不变；共享600秒届满因旧HB052600/旧Lancer139 worker占用拒绝，无worker/log/index/game。收到主人明确授权释放旧worker的协调回执，证据owner-resource-release-20261111.json；是另行强制结束遗留Python worker、非游戏强杀或正常生命周期通过，本方未执行/回复/转发，不把旧失败改绿。r160请求已消费，不复用。

资源释放后用新r161完成准备/导入检查，正式启动边界磁盘4228911104低于原4294967296字节下限而拒绝，无worker/log/index/game，未放宽磁盘阈值。两次normal_cleanup_required false、verified false，不补造正常清理；原结果/start-request保存在schema-repeat-r158/runtime-r160-prebirth和runtime-r161-prebirth。候选仍未执行游戏，无投影或query证据。

随后只检查本方r148/r150/r152/r155/r157的normal_cleanup true且精确game/owner/XR/worker身份均不在，逐文件将各自PSOCache/sp_factory_p/D3DGraphics.ushaderprecache移动到J:/gearsVr/analysis/cover-disk-recovery-20261111-r162/PSOCache。5文件共498835255字节，绝对source/destination均检查留在指定根，逐个移动前后SHA一致；未递归删除，未动源码/二进制/日志/媒体或其他/未清理实例。archive-plan/result保留原路径、目标和哈希，旧实例不重跑。操作后C盘观察6726025216字节可用，不能把全部空间变化归于本方499MB归档。下一次新实例仍重新执行真实磁盘/内存/身份/单次启动门。源码7c853c7f和准备依赖f549aab6已保存；完整掩体/年龄拒绝/画面仍未修复验收，目标active、25%及publicv71不变。

## r162：结构复核去重候选完成实跑，原查询前仍拒绝身体来源年龄（2026-11-11）

本轮首先核对当前工作树、启动源及磁盘，再以新cover-1111-r162-cloak-schema验证Native58c2fe1d…0c7f。13项准备来源前后相同，准备与操作导入均0，既有共享600秒准入接受；运行前磁盘73000828928字节。原版A进入CT2、五段eye动作与operator0；274投影帧、errors0、编码0，正常清理true、无强杀，game/owner/XR/worker精确身份独立确认均不在。CLOAK749/失败0，前台456/owned_activation false。原始会话、动作、来源和退出记录保存在schema-repeat-r158/runtime-r162。

本path首键Scene437/Compose26594/seq4372/world2137293056/pawn95255003392与complete首键逐项相同。path code4、engine/source_begin/guard，Owner第9次context/source_age_expired/detail6，Session第1次physics_read/guard；原因见证observed1/entry14674320/Compose26594/thread54328/stage0/verdict3/causes1，身体来源年龄拒绝、装备来源本次未过期。原版碰撞started/returned仍0/0；query156032411584500→156032433858000共22.2735ms。与r157拒绝层内部位置、入口年龄及调度不匹配，不能据此声称实际全query加速或年龄修复。读取优化未打通本次原调用，不刷新旧样本、不放宽时限。

14页全跨度和5张实际阶段contact均亲看，候选7/7/7/8/7；五阶段仍有严重自身头肩/躯干遮挡，画面FAIL，非完整掩体或头显通过。阶段区间不冒充同C0/GPU证明。历史媒体按主人清理指令可能已删除，历史观察仍作为原记录，不重新打开或为补图重跑。

下一窄点是来源到查询的前缀：Session先完成physics/collider/component初始样本，engine入口SourceGuard又进入完整physics采样；本次在该首复核拒绝。后续须定位各层成本与入口实际年龄，再选择保留完整身份/动态读取/关闭证明的修复；不原样重跑本候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71和固定入口不变，目标active。

## r163：原查询前缀分段计时已编译，尚未游戏运行（2026-11-11）

r162仍在engine/source_begin的physics_read首复核遇到body年龄拒绝。当前读回无法区分initial physics/collider/component与后续重复复核各自成本，故本轮增加有界、仅既有私有path one-shot开启的分段观察，不删守卫、读取、重复采样或来源关闭证明，不放宽时限，也不改身体/相机/输入/移动。生产Session在8个阶段记录calls、closed spans、total/max ns；clock invalid与累加overflow明确记录，未启用Scope时不读时钟。engine包含内部复核，时间嵌套重叠，不可相加；关闭span不代表操作成功，计时本身开销包含在结果内。仅在原one-shot登记成功后开Scope，跟随当前path键记录，不跨查询使用旧结果。

新增计时focused135项包含原Session105，不称135新项。覆盖未启用不读钟/原操作仍执行、返回引用、嵌套Scope恢复、异常原样传播并闭合记录、倒退时钟、累加溢出、实际Session成功与guard拒绝时输出不变。原guard23/query105/path391/overlap53/OFF51均通过；Host替换模块/dispatch，未执行原引擎或游戏。首次编译因测试局部名inner/denied遮蔽已有fixture全局且/WX而失败，原失败保留；仅重命名两处测试变量后focused通过。解码器11项检查拒绝不完整/负数/缺项/溢出/超出query区间，不将缺项当0或通过。

Native50ced042f8d7f9a55fb8c4f3e859fbb6ea75872889d7e1dbb27c754cdad65f79编译链接0。exact r158冻结树仅cover_physics_query_live.inl、cover_physics_c0_live.inl和新cover_query_timing.h三文件delta，410项目依赖、外部worktree0、GEARS flags保持。verify删除计时包装后重建原Session及C0文本逐字相同，实际guard顺序/条件/读值/返回表达式保持。prepare新增--native-r163并认证精确delta及原依赖/flags/构建状态；未准备、启动或运行新实例，不改publicv71。

下一步使用新实例做一次原CT2/同query分段观察，并保留来源年龄及画面实际失败。取得各段时间后再选取保持全部验证的修复；此候选只提供诊断能力，不声称性能提升、年龄修复、碰撞可用或玩法完成。全部CV01–24/AC01–22仍开放，工程25%、固定入口和实体头显状态不变，目标active。源码/测试/构建与文本核对在query-timing-r163。

## r164：同查询分段实测，初始组件目录为最大成本，仍在重复collider复核年龄拒绝（2026-11-11）

4839a96c保存后，以新cover-1111-r164-cloak-timing和Native50ced042…65f79做一次既有后台path观察。准备/操作导入0，13项启动来源前后相同；原A进入CT2、五段eye动作和operator0。265投影帧/errors0，编码0；14页全跨度和5实际阶段contact全部亲看，候选7/7/8/7/7，五阶段严重自身模型遮挡仍FAIL，不声称同C0/GPU或头显通过。正常清理true、无强杀，Native/entries/input恢复、XR及launcher正常退出，独立game/owner/XR/worker精确身份均不在；CLOAK624/失败0，前台393/owned_activation false。

path键Scene396/Compose24443/seq3679/world2310701312/pawn95226069120与complete首键逐项相同；engine/source_repeat/guard，Owner第19次context/source_age_expired/detail6，Session第3次collider_read/guard。实际原因见证entry14674320/Compose24443/thread33292/stage0/verdict3/causes1仍为body年龄拒绝、equipment当次未过期。原碰撞started/returned0/0，diagnostic小胶囊不证明完整身体或安全移动。

本query157934240187800→157934268685300共28.4975ms；计时invalid0/overflow0、所有span闭合。initial physics0.8626ms、collider3.1175ms、components15.5895ms；repeat physics3次共2.2219ms、collider3次共6.1059ms，components repeat0次，engine8.4879ms、closing0次。engine包含内部复核，不能重复相加；闭合不代表成功，计时开销包含在内，不作跨运行提速对照。这次实际测量将最大单段定位为initial component目录，尚未拆分其内部结构查找/动态读取/关闭成本，也不证明它是唯一或全部年龄原因。

源、动作、同query分段、原始退出和画面审阅记录在query-timing-r163/runtime-r164。下一窄点检查component root与schema重复认证，保留两遍动态读取、身份、完整证明及全部年龄门；不原样再跑计时候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口和实体头显状态不变，目标active。

## r165：组件目录第二遍Actor根结构认证去重，已检查/编译、尚未运行（2026-11-11）

依据r164同query initial components15.5895ms最大单段成本，本轮只修改cover_component_roles_live.inl中第二遍root选择：先新读Actor身份，要求与第一遍同index根节点完全相同、当前metadata字节/代次仍同，再用binding::current完整重读该root第一遍全部Proof identity/source，之后才消费缓存的五项属性偏移与数组结构。初次schema/class/五属性/inner认证保持；第二遍所有world/base/list data/count/capacity/原数组成员及递归节点仍真实读取，base的动态类型关系仍认证。新节点或顺序改变不能使用别的root字段；每次sample原clear不变，不跨sample/帧/query复用。最终members与全部root/component/asset/body Proof关闭、首尾guard及原输出不变条件保持，不改任何年龄/输入/碰撞/移动门。

actual冻结r163与新单文件覆盖通过同一focused346检查，含原component207，不称346全新项；覆盖两阶段offset、kind、reference、父类、成员Next、inner宽度、metadata和world变更，以及附着图增长与拒绝时输出保持。相同真实sample内safe_copy读取：2根24772→21520（13.13%）、8根40488→32070（20.79%）、16根61440→46134（24.91%）。仅合成fixture的整次component sample计数，不是原query毫秒、全链收益或来源年龄修复。

初checks-v1/v2/v3新增多根fixture在before/after均失败：名字池index704落到arena+0x4c000覆盖既有Body；另source_field每次实际双读，nth2仍在第一遍，不能作为第二遍变更。保存这些初失败和定位输出；v4仅将新增名字索引改到独立1000+范围、第二阶段触发改occurrence3，并移除定位输出，运行delta不变。v4 before/after346全0，原v1其余component207/guard23/query105/path391/overlap53/OFF51全0；所有运行头均来自各自冻结树且实际依赖外部worktree0。CPU fixture/Host dispatch不执行原引擎或游戏。

Native3f1d5454286e3991917f462ebab5749f614db22003d31a3810fbd985560be64d编译链接0；exact r163冻结树仅一运行文件delta，410项目依赖、GEARS flags保持。verify逐字重建去除新分支后的原控制流，确认动态root读取、完整关闭和clear原文不变；prepare新增--native-r165并认证精确单文件delta/依赖/flags/构建状态。源码/初失败/最终检查/读取对照/依赖与构建记录在component-root-repeat-r165。

下一步保存后使用新私有实例验证原CT2及同query分段，检查initial components实际成本与拒绝点是否改变，并核对画面；不原样重跑r164，不放宽时限或刷新旧样本。本条仅源码/离线/编译，尚未准备/启动游戏，不替换publicv71或固定入口。完整CV01–24/AC01–22仍开放，工程25%、身体来源年龄/自身遮挡/实体头显验收未解决，目标active。

## r166：组件优化候选实际进入通过，六轮复核后仍身体年龄拒绝（2026-11-11）

9564799d保存后，以新cover-1111-r166-cloak-components和Native3f1d5454…be64d做一次原CT2及同query分段验证。源码核对、准备和操作导入通过，共享启动两文件SHA保持7e630461/385f5e5a，13项anchor/启动来源前后相同；原A进入CT2、五段eye观察与operator0。290投影帧/errors0，编码0；15页全跨度及5实际阶段contact均亲看，候选7/7/8/8/7，五阶段自身头肩/躯干遮挡严重FAIL，不声称同C0/GPU或头显验收。正常清理true、无强杀，Native/entries/input恢复，XR和launcher正常退出，game/owner/XR/worker精确身份独立确认不在；CLOAK652/失败0，前台511/owned_activation false。头部遮蔽记录head_section_applied678、palettes678、uncomposed432、shoulder_armor678；这些累计记录不证明实际每Draw消费，不能替代或推翻本次遮挡像素。

本path键Scene372/Compose22860/seq3678/world2309652736/pawn95368151168与complete首键逐项相同。engine/source_guard/current；Owner第29次context/source_age_expired/detail6，Session第6次physics_read/guard。原因见证entry14674320/Compose22860/thread34024/stage0/verdict3/causes1，身体来源年龄拒绝、equipment当次未过期。原碰撞started/returned0/0；diagnostic小胶囊仍无完整身体或安全移动证明。

query159405188618100→159405220391000共31.7729ms，计时invalid0/overflow0、全部span闭合。initial physics0.8903ms、collider2.7614ms、components7.7426ms；physics repeat6次共6.0789ms、collider repeat5次共13.6629ms、components repeat0次；engine19.9980ms、closing0次。engine包含上述复核，不再相加。初始component实测低于r164的15.5895ms，但不同入口年龄、调度/Actor上下文未控制，不能把全部降幅归为源码因果；本次读到更多复核且最终仍拒绝，不是完整查询性能通过。当前最大观察成本为引擎内反复physics/collider复核，不放宽时限或刷新旧样本。

记录在component-root-repeat-r165/runtime-r166。下一步检查这些复核的真实调用位置和重复schema成本，保持每次动态与身份重读、关闭证明和全部年龄/反重放约束，不原样重跑本候选。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及实体头显状态不变，目标active。

## r167：同query碰撞体复核复用已完整重证的布局，并补动态字段关闭缺口；已编译未运行（2026-11-11）

依据r166五轮collider复核累计13.6629ms，本轮只将Session::SourceGuard内部collider调用改为recheck；每个新execute的初始sample仍清空并建立全新证明。sample仅首尾guard和关闭全部通过后登记sample_owner及ready；recheck核对完整owner key/Scene/special_move，同步作用域内才可使用。进入复核事务即清ready，任何该事务失败均不能重试复活旧证明；原SourceGuard对初始Snapshot的same比较仍保留，不新增写入或动作许可。跨query/帧不复用，下一execute必须sample。物理reader与其他模块未改。

recheck每轮仍两遍读取实际Pawn/Cylinder/BodyInstance数据，使用各阶段offset前由既有values(repeat=true)重读完整identity/source证明，最终metadata/member链/Pawn/component/body全部Proof关闭保持；首尾owner guard仍调用，不改来源时间、年龄、输入、释放或反重放。与旧sample差别是跳过已认证结构的重复查找，及在两遍之间逐次与最初snapshot核对。

新增fixture还复现了原scalar动态读的具体缺口：第二遍BodyData读取时才改变此前已读的OwnerComponent，旧sample+SourceGuard同值比较仍接受；首版复用候选也接受，checks-v1失败保留。最终将8项动态字段（Pawn两指针、component owner/body、body index/owner/data/spring）纳入各自既有Proof::source_field，实际双读/记账并在完整current关闭时重读，原失败在新候选拒绝且不更改输出。这是该离线时序缺口修复，不推断实际游戏已复现或所有并发变化均已覆盖。

checks-v2保留了原BoneSpring测试因读取方式改变而错触发的失败：原hook挂BodyData第2读，改为paired reader后仍属第一遍，发生在首次BoneSpring观察前。v3将hook改为BoneSpring自身第2观察，before/after都检查真实两次观察间变化；不删除该检查或放宽预期。最终before380/after407全0，包含既有collider219，不称407全新项；无可用证明状态、同owner多轮、三种body presence、布局/identity/metadata变更、两阶段动态变更、失败不复活、不同Scene、首尾guard、锁竞争/外线程及输出保持均有聚焦检查。旧baseline晚到owner接受在v3明确单独记录，新的拒绝与其对应，不把旧失败改成修复通过。相关collider219/component207/guard23/query105/path391/overlap53/OFF51全0；Host dispatch与合成内存不调用原引擎。

五次实际reader复核safe_copy计数：无BodyInstance22855→18375（19.60%），body存在且BodyData空/非空均30025→22255（25.88%）。该计数包含新增动态关闭读取；不是原query毫秒或全链提速，initial sample新增的动态读取开销也需实际测量。Native a41fadcf144fdde43a2573b1e5f52530e43ceca8eef256439ed505d0994b3ce8编译链接0；exact r165冻结树只改cover_collider_roles_live.inl和cover_physics_query_live.inl两运行文件，410项目依赖、外部worktree0、GEARS flags保持。verify核对原调用分支只换collider重查入口、两遍/首尾guard/原关闭均保留；prepare新增--native-r167并认证精确delta/构建状态。源、初失败、最终检查/对照/依赖/构建与核对在collider-recheck-r167。

下一步保存后以新实例验证原CT2与同query分段，确认重复collider实际成本、查询到达位置及画面；不原样重跑r166、不放宽时限、不刷新旧样本。此条仅源码/离线/编译，尚未准备或启动新游戏；完整CV01–24/AC01–22、身体来源年龄/自身遮挡/头显仍未完成，工程25%、publicv71/固定入口保持，目标active。

## r168：同query collider重证候选实跑八轮，仍身体来源年龄拒绝（2026-11-11）

b8389dd3保存后，以新cover-1111-r168-cloak-recheck、Native a41fadcf…b3ce8做一次现有后台path观察。准备/导入0，13项启动来源前后相同，共享两文件SHA仍7e630461/385f5e5a；启动前磁盘47905591296字节。原A进入CT2、五段eye观察及operator0。275投影帧/errors0、编码0；14页全跨度和5实际阶段contact均亲看，候选8/7/7/7/7，五阶段严重自身头肩/躯干遮挡仍FAIL，区间图不冒充同C0/GPU或头显证明。Native/entries/input恢复、XR和launcher正常退出，normal_cleanup true、无强杀，game/owner/XR/worker精确身份独立确认不在；CLOAK719/失败0，前台427/owned_activation false。

path键Scene326/Compose19992/seq4014/world2422112512/pawn95206631040与complete首键逐项相同。engine/source_guard/current，Owner第39次context/source_age_expired/detail6，Session第8次collider_read/guard；原因见证entry14674320/Compose19992/thread38732/stage0/verdict3/causes1，body年龄拒绝、equipment当次未过期。原碰撞started/returned仍0/0；新recheck确实进入实际同步查询并完成前序复核，但未通过整链。离线晚到OwnerComponent变更的拒绝未在游戏主动注入，不称该race实跑通过。

query160968869125600→160968895936700共26.8111ms；计时invalid0/overflow0、所有span闭合。initial physics0.9051ms、collider2.5732ms、components10.5233ms；physics repeat8次共7.2814ms、collider repeat8次共4.8876ms，components repeat0；engine12.3712ms、closing0。engine已包含内部复核，不能再相加。与r166五次collider13.6629ms相比，本次八次4.8876ms是实测观察；入口年龄/调度/Actor上下文未控制，不能认定全部差异来自代码、不能据此称全query修复。新动态字段关闭检查已在正常静态游戏路径消费，完整身体/世界覆盖/移动许可仍false。

头部遮蔽累计head_section_applied623、palettes623、uncomposed426、shoulder_armor623；最终像素仍遮挡，因此不能用累计应用计数替代实际消费或画面验收。原始动作、同query分段/原因、退出及审阅记录在collider-recheck-r167/runtime-r168。下一步核对同query入口各来源已消耗的年龄及重复physics成本，保留原时间与全部动态/身份/关闭检查，不原样重跑、不放宽年龄或刷新旧样本。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及头显状态不变，目标active。

## r169：同query来源年龄细分观察已编译，未运行（2026-11-11）

r168仍在第八轮collider复核body年龄拒绝，但原因位没有细分tracking40ms、metadata50ms、capture基准或既有age_burned。因此本轮增加通用纯值观察头body_source_age_observation.h，在既有body_tick_source_live的classify调用后复制它已有的observed_now，在current成功/年龄返回点复制它已有的now/capture_begin。复制原始metadata begin/end、tracking begin/end、stereo/controller received、sync、输入序列、Tick/世界/Pawn/Scene/线程和实际verdict/latched。没有新取时、游戏读取、追踪获取、source续时、状态判定、writer或动作许可。原hard提前返回仍原样，缺少观察保持未知。

仅既有私有path one-shot登记成功后，在同一query作用域开启TLS观察；world/Pawn/Scene/线程须与scope匹配，compose关联该实际query。固定四条：首次classify/current及各自首次source_age_only，另有计数/溢出标志。首次与首次失败不被后续覆盖，退出恢复原TLS；不同线程无观察，新scope清空旧记录。四条值复制仍有观察开销，不声称零成本。

记录区分两次已有取时：classify的tracking/metadata阈值只按它自己的observed_now解释，不能用稍后的current.now倒推早先分类。current单独记录capture年龄和latched。首次记录附相对query begin偏移，不冒充零时刻入口；原始时间不改。解码器要求精确Native及同query key、同Tick/输入cohort、时间落在query区间和有效采集次序，缺项/未观察/溢出不当作新鲜或通过；硬拒绝不推断年龄子项。40/50ms阈值源文件在冻结delta中未改。

通用观察器26项、解释器15项通过，包含OFF/TLS外线程/键不匹配/首条与首失败保持/嵌套恢复/计数溢出、新scope清理、40ms等值和超1ns边界、缺项/错key/cohort/区间，以及current稍晚导致的数值超龄不得倒推原classify、latched单独保留。相关cause25/timing135/guard23/path391/OFF51通过；CPU fixture/Host不调用游戏原引擎。verify移除纯观察代码后逐字重建原body源码及C0控制流，原clock调用次数保持。

Native0de22cab918329d51091f4e640043ffb43ec0f17586a93affab763d333660e17编译链接0。exact r167冻结树仅新观察头、body_tick_source_live.inl和cover_physics_c0_live.inl三文件delta，411项目依赖、外部worktree0、GEARS flags保持。prepare新增--native-r169认证精确delta及原依赖/构建状态。源、检查、解释器、依赖、构建和核对在source-age-detail-r169。本轮收到HB单实例对照临时让行及提前解除通知；无本方游戏/编码、无待登记启动等待，未新增游戏，构建自然完成，不改全局锁或并发保护；不因解除通知额外启动测试。

下一步Git保存后用新实例取得实际年龄细分及同query分段，再选择来源到查询调度或读取成本修复；不原样重跑r168、不放宽年龄或刷新旧样本。本条仅源码/离线/编译，未准备或运行新实例，身体年龄/自身遮挡及完整CV01–24/AC01–22、实体头显仍开放，工程25%、publicv71/固定入口保持，目标active。

## r170：来源年龄首查有效，原查询前owner_links拒绝；录像尾段缺失（2026-11-11）

d921621d保存后，cover-1111-r170-cloak-age使用Native 0de22cab…60e17完成一次现有后台path观察。准备、operator导入及实际五段动作0，原A进入CT2；13项启动来源前后相同。CLOAK710/失败0，前台430/owned_activation false。Native/entries/input恢复、XR和launcher正常结束，normal_cleanup true、无强杀，game/owner/XR/worker精确身份独立确认不在。runner退出1另因录像No complete fresh projection frame，不把正常生命周期与录像错误合并成整轮通过。

同键Scene398/Compose24497/seq3648/world2312601856/Pawn95251267840/thread57812，complete首键逐项一致。查询163050996290000→163051019056500共22.7665ms，code12=owner_links，原碰撞started/returned 0/0；Owner39次、Session8次均没有failed-call，来源年龄cause未观察到，body/equipment年龄原因是unknown而非false。原解释器误把残留engine source_guard/current阶段当首次拒绝，现按code10/11/12归为执行前owner资格失败；13项解释器检查通过，仍不知道asset/collider/components哪一reader及哪条关联失败。

新增年龄见证实际classify/current各39次，只有首次两行，没有first-age行。身体Tick24496与query Compose24497分别保留；身体stereo/controller来源3649与query seq3648也不混同。首次classify在query开始后0.1933ms，tracking age0.7929ms、stereo age16.7353ms、metadata age1.3459ms；stereo距原40ms限制尚23.2647ms。current独立时钟的capture age0.6200ms；本轮没有记录到年龄拒绝，不推广为历史年龄问题已修复，也不刷新时间或放宽期限。

initial physics0.7485ms、collider1.9149ms、components8.2071ms；重复physics8次共6.4971ms、collider8次共4.8254ms、components0次，engine11.5239ms已包含其内部复核，不重复相加。所有span闭合。新下一步是定位owner_links确切reader和已读关系，不再原样跑年龄候选。

230实际投影帧，记录只到25.0188883秒，录像捕获线程已收尾。12页已有跨度概览及4原始阶段图均亲看，中立/转头/恢复/升高仍严重自身模型遮挡；rise_restored区间没有完整捕获帧，保持缺失。仅从实际现存帧生成captured-span，编码0不改变原录像错误、缺尾和画面FAIL，不称全跨度/五阶段视频通过。原始动作、查询、年龄、分段、退出及审阅记录在source-age-detail-r169/runtime-r170。完整CV01–24/AC01–22未完成，工程25%、publicv71/固定入口及头显状态不变，目标active。

## r171：owner_links确切读取器和已读关联观察已编译，未实跑（2026-11-11）

r170排除本次年龄拒绝后，下一实际阻点是原版调用前owner_links，旧状态没有具体读取器。因此新增cover_owner_link_observation.h，在asset、collider、components已有六个links返回点复制既有Actor/Table/预期Scene/实际Scene/Internal/已读Reverse与线程；记录scene_or_internal或reverse分支。reverse尚未读取时明确reverse_read=false而非宣称关联为零。只有既有私有path one-shot同步作用域开启观察，绑定world/Pawn/Scene/Compose/thread，查询原有owner组分别限定before/after dispatch阶段。TLS只保留首条失败，阶段及嵌套退出恢复；未观察保持unknown。无新时钟、游戏读取、原版调用、筛选、grant、source续时或第二身体writer。

定向89项检查实际调用三种生产reader，覆盖场景、内部指针、反向关联三类失败，共九条实际分支，启用前后读取次数和返回拒绝一致；另含OFF、无阶段、无效键、跨线程、嵌套、首失败保持及新scope清旧。相关asset90/collider92/components108/path391/OFF51通过；解释器22项通过，精确Native/同query key沿用现有reader，拒绝矛盾分支和伪造反向读取，不把缺失当成功。首次测试遗漏bcrypt链接库、随后新fixture Body块只有0x90却写0x90字段导致异常，两次失败保留；修正为0x98后定向89项0，不涉及产品检查放宽。

verify移除纯观察代码后逐字重建r169的三个reader、QueryEngine和C0五份原文件，原判定/调用/读取/关闭次序不变。Native a2b53166f37fbed0329d9019c8e97a21962499e3e0786b69872dc52c901c8b95完整编译链接0；exact r169冻结源六文件delta，412项目依赖、外部worktree0、GEARS flags保持。检查、解释器、构建、依赖及delta保存在owner-link-detail-r171；prepare新增--native-r171，仍认证精确delta及原冻结构建约束。

本条只有源码/离线/编译，尚未准备或启动新游戏。下一步Git保存后在新实例用现有受控path查询取得第一条实际失败关联，按真实分支决定修复；不原样重跑旧年龄候选、不跳过异场景Actor、不弱化关联或释放保护。owner_links、严重自身模型遮挡、CV01–24/AC01–22和实体头显仍开放，工程25%、publicv71/固定入口不变，目标active。

## r172：实际asset内部反向关联值28，原碰撞未执行；正常结束（2026-11-11）

4bb31ba6保存后，新cover-1111-r172-cloak-links以Native a2b53166…c8b95完成一次现有后台path观察。prepare/operator导入0，原A进入CT2、五段eye动作及operator0；启动前C盘14653812736字节、原资源准入通过。13项启动来源前后相同，共享两文件仍7e630461/385f5e5a。CLOAK689/失败0，前台419/owned_activation false。Native/entries/input恢复、XR/launcher/runner正常0、无强杀、normal_cleanup true，game/owner/XR/worker精确身份独立核验不在。

query键world2310045952/Pawn95274860800/Scene396/Compose24376/seq4161/thread21936，与complete首键逐项一致；165621403132800→165621425915900共22.7831ms。code12 owner_links，原碰撞started/returned仍0/0。新首失败见证明确reader=asset、before_dispatch、reverse分支：Actor2309310912/Table140720217927848，预期与实际Scene均15959962496、Internal95275827456格式有效；既有Internal+0x10读取成功但Reverse=28，未指回Actor。不是本次场景错配或内部指针格式失败。仍不证明Actor损坏、具体内部类型、正确反向字段或唯一生命周期原因；下一步按原二进制核对类型与布局，不跳过该Actor、不猜偏移、不删除关联检查。

Owner39/Session8均无failed-call，年龄cause未观察，body/equipment原因unknown。classify/current各39次，只有首次两行：身体Tick24375、来源序列4163，与query Compose24376/seq4161分别保留；首查偏移0.2170ms，tracking0.8123ms、stereo1.4843ms、metadata1.4514ms，无记录到年龄拒绝，不将历史年龄问题视为已修复。initial physics0.7406ms/collider2.5525ms/components7.6377ms；physics repeat8共6.9899ms/collider repeat8共4.2953ms/components repeat0，engine11.4666ms含内部复核，不能重复加；全部span闭合。

256投影帧/errors0，已有捕获跨度到29.515秒；13页全跨度概览及五阶段原图00173/00192/00211/00231/00249全部亲看，中立/转头/恢复/升高/下降仍严重自身头肩躯干遮挡，画面FAIL。五段联系只按实际时间区间，不冒充同C0 GPU原子证明；编码0不表示视图通过。原始动作、退出、精确失败值、年龄、分段和审阅在owner-link-detail-r171/runtime-r172。完整CV01–24/AC01–22及头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r173：原二进制确认NpActor内嵌拥有者，五个读取器关联修复已编译（2026-11-11）

本轮progress。从r172实际asset Body+0x10=28失败继续，REA打开精确PhysXCore SHA22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48。IDA provider未配置，显式改用Ghidra12.1.4，未改安装/注册。原NpcActor构造1328b0的汇编明确LEA [RSI+28]调用1204c0，并MOV [RSI+30],RSI；后者写入Actor vtable2324f8并将其+18（即NpcActor+40）初始化为空。NpActor后续debug分支分别显示该指针为空的Static和非空的Dynamic。因此旧body/internal+10==public Actor不是已认证的拥有者链。NpcActor+40动态对象与嵌入Actor是两类对象，不能混用；原28的具体语义仍未知，不称flags字段。完整32份REA证据导出owner-link-detail-r171/actor-layout-rea-evidence.json后正常关闭；findings链接原汇编Evidence IDs和局限，无新游戏/原DLL执行。

新增共用纯读取cover_physx_actor_backlink.h：认证NpcActor+28的嵌入Actor表2324f8，读取NpcActor+30并要求它等于当前public Actor。asset/collider/components与actor_shapes/shape_owner五个实际读取器都改为该真实链；保持原primary表、Scene、当前Body指针、各形状/public/internal/compound成员关系、同源guard、两次读取和关闭比较。Body非空仍必须格式有效，合法静态Body空值独立处理；self匹配允许精确全链一致的静态null Body，不凭null或Actor身份单独放行。所有来源、世界覆盖、排除与移动许可仍各守原规则，不增加writer、调用、输入或grant，不续旧时间/放宽年龄。新增读取有成本，不称零成本。

旧/新12项生产reader同输入对照：正确embedded owner但Body+10=28，旧拒绝新接受；embedded owner错误但伪造旧Body+10==Actor，旧接受新拒绝；静态Body为空与错误嵌入类型也分别对照。既有夹具补实际+28/+30字段；原reverse破坏和race刺激移至真实拥有者字段，非法指针刺激由合法null改为1，未删除原检查。asset92/collider94/components110/optional asset169/attachments150/shape owner519/actor shapes77、路径391/OFF51、观察89、完整owned sweep146通过，合计1900；另有旧基线12与解释器23。静态全链匹配有正例，静态shape与非空collider Body冲突仍拒绝。最初夹具脚本一次substring歧义中止后精确修正；checks-v2/v3路径66项失败源于旧host缺嵌入Actor字段，补齐基础和cylinder夹具后checks-v4原391通过，原失败保留。不是游戏实跑结论。

Native f585196291ca8481ef9ffd8db17032a60e2de6c2eff1b58a847cb8f51e8426cd完整编译链接0，exact r171冻结树仅八文件产品delta，413项目依赖、外部worktree0、GEARS flags保持，其他冻结源逐项相同。原始时间/新鲜度/唯一writer和所有不相关源不变；prepare新增--native-r173认证精确delta。source/delta、检查、前后反例、原始失败、来源manifest、依赖、build及核对在actor-backlink-r173。

下一步Git保存后以新实例短查：原版碰撞是否真正开始/返回、真实owner链是否通过，再处理第一个实际剩余失败；不重跑未变年龄候选，不跳过异场景对象或放宽关联。当前仅源码/离线/编译，未准备/启动该候选；实际owner_links修复、完整CV01–24/AC01–22、严重自身模型遮挡和头显仍未验收，工程25%、publicv71/固定入口不变，目标active。

## r174：修正Actor候选在原owner读取前已发生40ms年龄拒绝（2026-11-11）

4d17da8f保存后，新cover-1111-r174-cloak-backlink、Native f5851962…426cd完成一次现有后台path观察。prepare/导入0，原A进入CT2、五段动作及operator0；启动前C盘7076175872字节，原资源准入通过。13项启动来源前后相同，共享两文件仍7e630461/385f5e5a。CLOAK740/失败0，前台459/owned_activation false。Native/entries/input恢复、XR/launcher/runner正常0、无强杀、normal_cleanup true，game/owner/XR/worker精确身份独立确认不在。

query键world2309259520/Pawn95208079616/Scene376/Compose23187/seq4183/thread40184与complete首键逐项一致；167700877905600→167700907009200共29.1036ms。code4 source，engine/source_guard/current；Owner第35次context/source_age_expired/detail6，Session第7次collider_read/guard，原因body=true/equipment=false。engine begin的source_guard阶段失败早于asset/collider/components owner组，新关联reader尚未到达；first link witness未观察，原版碰撞started/returned0/0。因此不能用本轮未再报owner_links宣称关联修复实跑通过，也不能将本次更早的年龄失败归因于尚未执行的新字段读取。

classify/current各35次，首次与首次age共四行。身体Tick23186/来源序列4184与query Compose23187/seq4183分别保留。首查比query begin晚1.0829ms，此时tracking3.2878ms、stereo12.2946ms、metadata3.9061ms，capture current3.1347ms；原40ms的最旧stereo还剩27.7054ms。首次age记录在167700907006100，stereo40.3122ms、controller40.2832ms、sync40.3065ms全部超原40ms；tracking31.3054ms与metadata31.9237ms未超，current capture31.1519ms也未超，latched=false。首次真实年龄子项已明确，未续时间或放宽期限。

initial physics2.2441ms/collider2.9930ms/components10.1253ms；重复physics7次共6.0740ms、collider7次共4.1032ms、components0次，engine11.8714ms含内部复核，不重复相加。全部span闭合。下一修复方向是同query来源到查询时延或初始组件/元数据与重复读取成本，保留全部身份/动态字段/当前值/收尾及原始时间；不通过不变重跑挑选较新输入或更快调度的一轮。

286投影帧/errors0、编码0，已有跨度到32.773秒；15页全跨度概览和五阶段原始00202/00221/00241/00258/00279均亲看，全部严重自身头肩躯干遮挡，视图FAIL。实际区间帧不是同C0/GPU原子证明，不作头显验收。原始动作、退出、精确原因、四行年龄、分段和审阅在actor-backlink-r173/runtime-r174。完整CV01–24/AC01–22及头显仍未完成，工程25%、publicv71/固定入口不变，目标active。

## r175：同query物理角色复核复用已认证布局，动态字段与成员收尾补齐；已编译（2026-11-11）

本轮progress。从r174实际query29.1036ms、最旧stereo首次12.2946ms、七轮physics/collider复核和40ms拒绝继续。发现SourceGuard每次physics复核都调用sample清空布局并重推属性链，而collider已有同一execute内的recheck。因此physics新增同样有界入口：每个execute仍先sample并使旧ready失效，成功才保存精确Binding/Scene/special_move/完整pose metadata；只有同一次同步查询的SourceGuard改用recheck。每轮进入先清ready，保留两个真实values读取、各自与initial比较、完整prior proofs在偏移前复核和原closing；任何已进入的失败不能复活旧certificate，重新sample才恢复。不同线程/Scene/metadata、锁失败、owner拒绝及异常均有定向检查，不跨Tick/execute缓存，不续原始时间/放宽40ms，不增加writer、input、original call或grant。

动态字段还进入已有Proof的双读和最终current：mesh PhysicsAsset/PhysicsAssetInstance两指针、PAA Owner/Bodies data/count/capacity/Root共七项，以及每个Body的真实数组成员、BodyIndex、OwnerComponent、BodyData和BoneSpring五项。原完整数组读取、身份、空值、重复/容量检查都保持，未截短64项。repeat只在完整instance/body proofs已重读、fresh owner identity等于initial、mesh类/引用精确一致时复用先前owner/mesh类关系推导，所有路径身份/父链仍由原完整Proof复核；首次sample独立重建。既有字段改写后即使对象地址相同也不能延用。

实际生产reader前/后对照530/562项均0，包含原physics282项；旧版有12种第二次读后动态变更未被closing覆盖，新版全部拒绝：两个OwnerComponent、Root、Bodies指针、Mesh instance、BodyData、BodyIndex、PAA Owner、Mesh asset、capacity/count及非root数组成员。BoneSpring刺激的fixture回调在copy前执行，原版已能拒绝这一项，保持其拒绝、不伪称旧漏洞。每次五轮复核，2 Body读数31565→22945（-27.31%）、16 Body34085→27355（-19.74%）、64 Body42725→42475（-0.585%）；最大场景收益较小，原因是补齐每个成员的收尾检查，未为性能删除它。读次数不是游戏时间，不称已消除实际年龄拒绝。相关collider219/component207/guard23/query105/path391/overlap53/OFF51全部0。

早期结果保留：checks-v1功能0但64 Body读取增加1.10%，未放宽比较；v2在重证后复用类关系推导改善。v3扩大动态反例时把BoneSpring当前读误标成late漏洞，before一项失败，按真实回调时相改回原拒绝；v4全530/562及相关检查通过。又补Bodies逐成员Proof后完整重编，旧Native/source/build/manifest保存为-v1历史，最终receipt绑定当前文件，不冒领旧构建。完整源码/计数/失败/依赖/delta/verify在physics-recheck-r175。

最终Native1e037608fc9b3722f0bc80bd0afbeac1f4499d85033c69b1954fa337f8491dfb编译链接0，exact r173冻结树仅cover_physics_roles_live.inl及cover_physics_query_live.inl两文件delta，413项目依赖、外部worktree0、GEARS flags保持，其余冻结源逐项相同。r173真实NpActor内嵌拥有者修复保持。prepare新增--native-r175，仍校验精确delta/原依赖和build-only状态。本条尚未准备或启动新游戏，不将离线速度外推实际query通过。

为持续验证空间，按既有结束实例缓存整理授权，仅对本chat的r170/r172/r174逐项正常cleanup、精确game/owner/XR/worker不在再次核验后，将315个PSOCache文件共2655147954字节复制到J:/gearsVr/analysis/cover-cache-archive-20261111-r175；全部目标/source SHA复核一致、再次核验进程不在才逐文件移除C盘缓存。只移除这三处缓存文件，目录/源码/文本数值证据/媒体/存档/配置/seed及其他实例均保持；cache-archive.json记录全部哈希与路径。没有删除其他owner或活动实例文件。

下一步Git保存后以新实例短查重复physics实际耗时、40ms入口余量、原查询started/returned和第一个实际失败；不原样重跑未改候选，也不以较好运气的一轮代替修复。真实年龄、NpActor修复实跑、严重自身模型遮挡、完整CV01–24/AC01–22及实体头显仍未完成，工程25%、publicv71/固定入口不变，目标active。

## r176：物理复核候选推进到第18次来源检查，查询仍因40ms超时拒绝（2026-11-11）

757d0807保存后，新cover-1111-r176-cloak-physics使用Native1e037608…91dfb完成一次现有后台path观察。prepare/导入、原A进入CT2、五段动作及operator均0。13项启动来源前后相同；CLOAK616/失败0、前台387/owned_activation false。Native/entries/input恢复、XR/launcher/runner正常0、无强杀、normal_cleanup true；保存时再次独立核对game/owner/XR/worker精确身份不在。

query键world2310373632/Pawn95229772032/Scene386/Compose23802/seq3609/thread28816与complete首键一致；169742703028700→169742734643600共31.6149ms。code4 source，engine/source_guard/current；Owner第79次context/source_age_expired/detail6，Session第18次collider_read/guard，body=true/equipment=false。原碰撞started/returned0/0，owner link witness未观察仍为未知，不用无记录宣称完整关联修复通过。

classify/current各79次，四行保留首次和首次年龄拒绝。身体Tick23801、来源序列3611与query Compose23802/seq3609分别记录。首查在query begin后0.2091ms，tracking0.7839ms、stereo8.6470ms、metadata1.1649ms，current capture0.6358ms；首次age时stereo40.0503ms、controller40.0206ms、sync40.0455ms超原40ms，tracking32.1872ms、metadata32.5682ms、capture32.0391ms未超，latched=false。不续时间、不提高期限，source_age_repaired=false。

initial physics0.7613ms/collider2.2084ms/components9.7828ms；重复physics18次共8.8081ms、collider18次共9.4600ms，components重复0次。engine18.4832ms包含内部复核，不重复相加，span闭合且无overflow。比r174推进7→18次Session来源检查；本次总query31.6149ms并未更快，两次输入/调度非受控对照，不把复核次数或平均耗时差异直接当作因果性能结论。下一步处理首次组件目录中重复属性扫描，完整身份、字段、成员收尾和原时间约束保持，不原样重跑碰运气。

248投影帧/errors0、编码0，13页跨度概览及五阶段原始00165/00185/00204/00222/00241全部亲看。进入掩体后五阶段仍有严重自身模型遮挡，视图FAIL；这批区间帧不是同C0/GPU原子证明或头显验收。原始动作、退出、年龄、分段和审阅在physics-recheck-r175/runtime-r176。body_geometry/world_coverage/movement仍未通过；CV01–24/AC01–22及实体头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r177：联合扫描根目录五项属性，完整成员链收尾保持；已编译（2026-11-11）

从r176首次components9.7828ms和真实40ms拒绝继续，检查发现每个Root独立查找WorldInfo/Components/AllComponents/Base/Attached时反复遍历相同继承与成员链。现在只在cover_component_roles_live.inl新增root_schema，一次走每级Class目录并选出五项：保留32级父链/4096成员边界、类/extent/head/parent来源、最近声明者语义、同级重名拒绝，以及每个选中描述符的fresh身份、类型、dimension/width/offset/reference检查。所有访问过的完整成员身份与next进入同sample MemberStore，包括未选和末尾成员；另一Root仅在当前owner/head相同后共享完整目录，仍fresh认证选中字段。sample仍clear所有目录，repeat与最终current继续完整重读；不跨execute缓存，不改原始时间、40ms、guard、writer或移动许可。

真实生产组件reader原207项保持通过；前/后实际读取对照各308项通过，覆盖完整schema/共享目录、字段/链接/头/extent/父类/非选中名字变更、坏dimension/offset/kind/owner、环与末尾重复字段。基础schema加完整closing读取2078→949，减少54.33%；追加128项实际成员的长链15646→4917，减少68.57%；长链末尾改名形成late重复仍被最终成员复核拒绝。这是离线读次数，不是游戏耗时或完整碰撞通过。相关query105/path391项全部通过，依赖只来自各自冻结scratch或系统。

早期失败保留：v1缺少冻结测试include，编译未开始测试；v2测试中聚合Workspace清零形成大栈临时量，两版均stack overflow，改为测试专用heap reset后v3两版290项通过。v4新增长链fixture的名字表地址与原Class区间重叠，前/后同样失败；修正合成名字索引到不重叠区间后v5两版308项通过。未改产品去适应fixture，也未把失败算通过。

完整Native de07b0c1daf61f1aa16f5824c986bebe0f136f895bdea51320eae30aae01b8f1编译链接0；exact r175冻结树仅组件角色一文件delta，413项目依赖、worktree外漏0、GEARS flags不变，其他冻结源前后哈希一致。r175 physics复核/r173关联修复保持。原始脚本、失败、前后结果、manifest、build和verification保留在root-schema-r177；没有替换公开包或固定入口，尚未准备/启动此候选。

下一步保存后按资源准入准备精确候选，做一次有修改依据的短path观察，核对初始组件耗时、完整来源、原query started/returned及第一个实际失败；真实年龄修复、完整owner/world/body几何、严重自身模型遮挡、CV01–24/AC01–22和实体头显均未通过。工程25%、publicv71/固定入口不变，目标active。

## r179：新候选在原A前候选读取收尾拒绝，无终态碰撞查询（2026-11-11）

r177接入--native-r177后，仅选择精确一文件delta/原413依赖和原flags，source/build保持。归档正常结束且独立精确身份不在的r176 PSOCache105文件885049318字节到J盘，全部SHA一致后仅移除缓存文件；853791d6保存。r178准备和导入0，但runner前置require_disk实际3938926592字节小于原4294967296，在claim/worker/start请求前拒绝；各attempt文件独立确认不存在，不复用该身份。随后同样核对已结束r166/r168，将210缓存文件1770098636字节逐文件校验归档J:/gearsVr/analysis/cover-cache-archive-20261111-r178；源码/文本数值/媒体/存档/配置及其他实例保持。

新cover-1111-r179-cloak-root、Native de07b0c1…01b8f1 prepare/导入/启动0，启动前4768755712字节通过原磁盘门槛。实际game58696、creation134388601048933864，observer在cover_boundary_chain候选CoverQuery.finish处抛World-state metadata/registry changed at closing boundary，operator/runner1。原A与五阶段未达，严格reader无终态query。没有components耗时或源码修复游戏证明，不将No terminal query伪装0次原调用的已测计数；未观察到的首项身份/字段/registry差异仍未知。

Normal cleanup true，Native/entries/input与XR正常退出、remaining_owned空、无强杀；保存时独立确认game/owner/XR/worker精确身份不在。13项启动来源前后相同，CLOAK499/失败0、前台330/owned_activation false。136投影帧/errors0/编码0，7页实际概览全部亲看，仅覆盖进入掩体前场景；五阶段及contact0，明确标记动作跨度不完整，不作掩体视图验收。

发现Query.finish已记录closing_rejection的before/after，但外层只保存通用异常，具体首差异丢失。下一步仅在本chat的隔离operator生成时保存这份已有差异并重抛原异常；不放松registry变化、不续期、不跳过候选、不使用失败观察或新增重试。先补已存在证据的落盘，再有依据地复查，不原样重跑。r177实际碰撞/40ms、自身模型遮挡、CV01–24/AC01–22与头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r180：候选读取的现有收尾差异落盘，严格拒绝保持（2026-11-11）

r179首败在原A前候选q.finish，Query已在原读取中保存closing_rejection，但operator只保留通用异常。现在仅在本chat prepare_operator生成的私有cover_boundary模块，对该候选收尾调用包一层：失败时保存已存在的link address/stage/closing_rejection及missing-is-unknown，立即重抛原异常。成功时不保存额外诊断，调用数不变。原world_state.py/CoverQuery.finish、原boundary来源文件与全部Native/PG/XR均未更改；没有新读取、输入、重试、registry增长放行或使用失败观察。

实际replacement执行identity/metadata/registry/unknown/success五例全部通过；每例finish一次，四种拒绝保持同一异常对象，成功不多写；原finally继续执行。原来源三处q.finish数量不变，检查原始boundary哈希06f16552…09254。生成器将diagnostic源码SHA纳入operator-prepared回执，AST核对实际变换后来源。仅验证已有差异的保存，不将其称为实际首差异已知、候选选择修复或r177查询实跑通过；没有启动新游戏。r179缺少五阶段，media-review及生成器明确动作跨度不完整，不能因无录制错误就算完整掩体流程。

下一步按资源准入使用新身份执行一次带该确切差异记录的短观察。若原失败重现，针对真实identity/metadata/registry差异修复；若不重现，仅报告未重现，不作原因修复结论。r177性能/40ms、自身模型遮挡、全CV/AC与头显仍待验证，工程25%、publicv71/固定入口不变，目标active。

## r181：组件联合扫描候选实际query仍超40ms；原候选收尾失败未重现（2026-11-11）

带r180具体closing差异记录的新cover-1111-r181-cloak-close使用原r177 Native de07b0c1…01b8f1。prepare/导入/启动0，原A进入CT2、五阶段及operator/runner0；候选严格收尾通过，没有candidate_closing_rejection。r179失败本次未重现，不把记录层当成原因修复。生成器回执绑定50979a3c…41eca。正常cleanup true、remaining_owned空、无强杀；保存时game/owner/XR/worker精确身份独立确认不在。CLOAK607/失败0，前台374/owned_activation false，13启动来源前后相同。启动前另对r179已结束精确身份核验后，将105 PSOCache文件885049318字节校验归档J:/gearsVr/analysis/cover-cache-archive-20261111-r180，只移除该缓存，源码/文本数值/媒体/存档/配置保持。

query键world2224455936/Pawn95203878528/Scene403/Compose24829/seq3512/thread23612与complete首键一致；171869734261500→171869769855500共35.5940ms。code4 source、engine/source_guard/current，Owner78/context/source_age_expired/detail6，Session18/collider_read/guard，body=true/equipment=false，original started/returned0/0，link witness未观察仍未知。

首次classify比query begin晚0.6801ms，body Tick24828、追踪seq3514与query分别保留；首次tracking1.4236ms/stereo5.4775ms/metadata2.0536ms，current capture1.1743ms。首次age时stereo40.3869ms、controller40.3511ms、sync40.3787ms超原40ms；tracking36.3330ms/metadata36.9630ms/capture36.0818ms未超，latched=false。没有续时间或扩大时限。

initial physics1.2492ms/collider2.9179ms/components3.3837ms，重复physics18次12.3993ms/collider18次14.3040ms/components0次；engine27.0113ms含内部复核，不重复相加，全部span闭合无invalid/overflow。首次components比r1769.7828ms低，但输入/调度不同，不把时间差全归因于优化；总query仍35.5940ms，真实超时未修复。下一步审查18次重复身份/目录/完整Proof读取的具体成本，在保持所有原始时间、动态字段和closing语义下减少重复工作；不原样重跑挑一轮。

265投影帧/errors0/编码0，14页全跨度概览及五阶段原始00181/00200/00220/00238/00257全部亲看；中立/转头/恢复/上移/恢复仍严重自身模型遮挡，视图FAIL。区间帧不是同C0/GPU原子证明或头显验收。原始session、动作、projection数值清单、年龄、分段及审阅在query-close-detail-r180/runtime-r181。完整world/body几何、movement、CV01–24/AC01–22和实体头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r182：实际冻结reader离线读取构成，下一步定位嵌套Owner复核（2026-11-11）

未启动新游戏、未改产品。以r177冻结源码哈希逐项核对后，在独立scratch复用真实cover_collider_roles_live_test及原safe_copy hook，分别做五轮实际physics/collider recheck，19项检查0，Native call/write0，编译依赖未外漏worktree。physics23455次、collider22255次，均按精确地址/大小分类且总数逐项相等。

physics中fixture Owner guard13140次（56.02%），其余registry headers2500、object/name blocks与slots各1250、4字节1935、8字节870、16字节10；collider中fixture Owner同13140次（59.04%），registry headers2280、object/name blocks与slots各1140、4字节1610、8字节665。这是现有fixture h::current_tick_actor的读次数，不是生产Owner完整成本或真实耗时。实际cover_physics_c0_live::query Owner还检查C0 context、sample_current CoverKey和pose metadata；不以该比例直接认定游戏根因。

源码进一步确认SourceGuard每轮分别给physics和collider的recheck传完整Owner，各自前后调用；生产CoverKey sample_current又包含两次完整current_tick_actor及三次Cover values读取。当前同一次同步只读操作存在多层完整Owner检查。下一修复须明确同一操作的首尾认证边界，原版调用前后完整实际身份、源龄、当前掩体/姿态和停止/释放仍必须有效，不能简单传true、跳过Owner或缓存授权至下一次guard/execute。先针对实际拒绝/中断的反例证明边界，再实施必要优化，不重跑原候选。

原始probe、类别计数、完整build/dependencies和结果在recheck-cost-r182。r181仍40ms拒绝且严重视图遮挡；完整CV01–24/AC01–22及头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r183：掩体链接完整继承链并入现有Proof，末级父类收尾缺口修复；已编译（2026-11-11）

沿r181的40ms拒绝和r182重复读取证据，未减少Owner/来源/释放的调用次数。找到cover_context_live::link_layout每次values均重新derives完整CoverLink类链，然后又begin_prepare读取属性Proof；同一sample_current三次values、每个Owner又调用sample_current。现在首次完整derives后，把每级类身份、Class类型和父类边全部认证进原link_proof，包含末级祖先的零父类边。后续使用原begin_prepare/current统一fresh复核；metadata generation或class key变化仍使原Proof失效。pawn/link已ready且当前class地址精确匹配时，不再在完整Proof前额外读一次同一class；该class身份仍由完整Proof实际重读。未改共享binding、追踪、骨骼、时间、40ms或Writer，不把缓存当授权，也没有传true跳Owner。

原206项context检查保持通过。真实生产reader前/后各288项0：不同继承层数、字段/父类/类型/身份/槽位/世界代次/录制及线程门禁，以及最后一次slot读取时把末级Actor父类改成环。旧版该晚变更确实被接受，baseline_unclosed_ancestor=1；新版最终link_proof current拒绝且输出保持。类链的初始推导一直存在，新增的是完整落入原有末尾Proof。

五轮完整sample_current（含原Owner绑定与三轮cover values）在额外继承0/4/12级的读取24220→22645（-6.50%）、28560→24345（-14.76%）、37240→27745（-25.50%）。v1只合入完整类链，前后288项0且有收益；v2再去掉Proof之前重复的相同class读。所有原始结果保留。收益依赖实际类深度，不能外推实际游戏耗时或40ms已修；OFF编译/运行0，未动关闭分支。

完整Native0d4437fd5427902f84e5571cb1e3f577c201b114bfaf2843b3e4483f998cdf6f编译链接0，exact r177仅cover_context_live.inl一文件delta，413项目依赖、worktree外漏0、GEARS flags不变，其余冻结源前后相同。测试新增main命名宏只为复用原fixture，不属于产品delta。prepare新增--native-r183认证精确候选。代码/反例/读取计数/原失败范围/manifest/build/verify在link-path-proof-r183。

下一步Git保存后用新实例短查实际CoverKey复核和原query入口。该候选尚未启动，实际40ms、完整owner/world/body几何、自身模型遮挡、CV01–24/AC01–22和实体头显未完成；工程25%、publicv71/固定入口不变，目标active。

## r184：新候选在原A前严格拒绝注册表增一，已取得精确前后值（2026-11-11）

c9b8ba8b保存后，新cover-1111-r184-cloak-path使用r183 Native0d4437fd…cdf6f，prepare/导入/启动0。实际game33600、creation134388623588570950，候选CoverQuery.finish拒绝，operator/runner1；原A/五阶段未达，严格reader没有终态query。因此没有r183继承优化的实际query耗时，不能声称40ms已修复或原查询实测0次。

r180记录取得首个差异：candidate link2303863648，stage candidate_selection_before_original_A；registry地址5398218544，16字节before00002f160000000061540300f86f0300→after00002f160000000062540300f86f0300，data372178944/capacity225272保持，count218209→218210，missing-is-unknown=false。只能确认此注册表头的这一差异，未取得新增项的语义，不能证明与候选无关。严格原检查拒绝未变化，未开启append-only放行或复用失败证据。

normal_cleanup true、remaining_owned空、无强杀，保存时独立确认game/owner/XR/worker精确身份不在。CLOAK546/失败0，前台352/owned_activation false，13启动来源前后相同。145投影帧/errors0、编码0，8页实际已有跨度全部亲看，仅进入掩体前场景；contact0/阶段0，明确动作跨度不完整，不作视图或头显验收。原始session/差异/动作/投影清单及review在link-path-proof-r183/runtime-r184。

下一步缩短候选观察中的重复读取，尤其数组头/完整Slots重复采样、逐槽Bool mask重复读取和收尾的重叠数值窗口；必须保持相同身份/字段/目录/slot值的严格收尾和时间/容量限制，不延时等待或跳过变动候选。完整world/body/movement、严重模型遮挡、全CV01–24/AC01–22及头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r185：候选CoverQuery收尾复用本次新读完整窗口，严格目录双收尾保持（2026-11-11）

针对r184已知registry count增一的实际拒绝，不放宽append-only、不跳过候选、不增加重试或等待。发现CoverQuery.values同时保留完整Slots raw块和内部每个字段的首次witness；finish原来先重读整个raw，又逐槽逐字段重复读取已包含的字节。现在在同一次finish内按地址/长度排序，新读原先已记录的完整窗口，所有被包含的首次witness逐项对照该新读字节；不包含或只部分重叠的窗口仍分别完整重读。没有扩展读取范围、丢字段或跨finish复用。两个原super.finish仍在全部values前后执行，identity/metadata/registry逐项严格相同，3秒/读取/数组容量限制不变，目录增长仍拒绝。

实际现有cover_state.py reader原14项检查0、新18项检查0；新增覆盖每字节变动、内部首次witness不一致、读取失败、部分重叠、第二次finish不复用缓存。既有目录增一、mask在逐槽期间变化、identity/结构指针和第二收尾中变化的反例仍拒绝。真实slot_snapshot前后对照3/16/32槽，capture读取49/101/165保持、全部46/137/249首次witness数保持、每项解码rows SHA相同。收尾读取88→67（-23.86%）、179→67（-62.57%）、291→67（-76.98%）。这是合成内存的实际reader读取次数，不是游戏时间，也不是registry race已修复。

prepare_operator新增cover_state/world_state两份实际reader来源SHA；私有runner在claim前与正常清理后核对并记录，来源改变不能记通过。原13项启动来源、共享隔离/资源门槛和输入保护不变，两个来源另列，不伪装历史案例已有该回执。生成器/runner AST核对通过。仅Python观察器变化，Native/PG/XR不重编，继续精确r183 Native0d4437fd…cdf6f。原始前后来源/测试/计数/哈希/delta与verify在closing-windows-r185。

下一步Git保存后，新实例绑定这两份reader哈希，短查候选是否通过原严格收尾、r183 query来源/耗时/原调用开始返回。若目录变化仍在原窗口内发生，继续保留实际拒绝，不选择性重跑。r183实际40ms、world/body/movement、严重自身模型遮挡、CV01–24/AC01–22及实体头显仍未完成；工程25%、publicv71/固定入口不变，目标active。

## r186：组合候选进入掩体，首次来源已26.4991ms，query仍在原调用前过40ms（2026-11-11）

feb1f0e4保存后，新cover-1111-r186-cloak-close使用精确r183 Native0d4437fd…cdf6f及r185窗口收尾reader。prepare/导入/启动、原A进入CT2、五阶段与operator/runner全部0。候选严格收尾通过，r184 count增一未重现，不以本轮通过宣称目录race因果修复。两份cover_state/world_state来源SHA在claim前/清理后相同，原13项来源也相同；启动前9291214848字节通过原门槛。CLOAK731/失败0、前台442/owned_activation false。normal_cleanup true、remaining_owned空、无强杀；保存时game/owner/XR/worker精确身份独立确认不在。

query键world2252701952/Pawn95181176960/Scene318/Compose19580/seq4319/thread54900与complete首键一致；174581640226000→174581654719800共14.4938ms。code4 source、engine/source_begin/guard；Owner第8次context/source_age_expired/detail6，Session第1次physics_read/guard，body=true/equipment=false，original started/returned0/0；link witness未观察仍未知。首次重复physics在其Owner前置检查即拒绝，本次未到后续typed owner组。

身体Tick19579、追踪seq4320与query分别保存。首次classify在query begin后0.7327ms，此时tracking2.1943ms、stereo26.4991ms、controller26.2785ms、sync26.3101ms、metadata2.8914ms；最旧stereo距原40ms仅13.5009ms。按同一原时间相减，query begin时该stereo已25.7664ms，可用14.2336ms，实际query14.4938ms，不能把全部26.5ms算成query新耗时。首次age时stereo40.2551ms、controller40.0345ms、sync40.0661ms超原40ms；tracking15.9503ms、metadata16.6474ms、current capture15.6549ms未超，latched=false。没有重写时间或扩大时限。

initial physics1.8795ms/collider4.5246ms/components5.4962ms；physics_repeat1次0.2104ms、collider/components repeat0次，engine0.5644ms含内部复核，不重复相加，span完整无invalid/overflow。query比先前短不代表性能通过，因为更早失败且初始输入更旧；r183优化的完整query仍未验证。下一点应同时考虑来源到查询的延迟和嵌套完整Owner复核成本，不能挑较新输入重跑、延长40ms、续期或跳过中断/身份/closing。

311投影帧/errors0/编码0，16页全跨度概览及五阶段原始00221/00241/00262/00284/00304全部亲看，仍严重自身模型遮挡，视图FAIL。区间帧不是同C0/GPU原子证明或头显验收。原始session/动作/projection数值清单、年龄/分段和审阅在closing-windows-r185/runtime-r186。world/body几何、movement、全CV01–24/AC01–22及实体头显未完成，工程25%、publicv71/固定入口不变，目标active。

## r187：相邻纯读取使用完整前后Owner校验，失败凭据不可恢复；完整候选已编译（2026-11-11）

针对r186尚未到原查询就因原40ms来源过期的失败，本轮只将相邻纯读取归为同一次同步事务。initial physics/collider/components由6次完整Owner检查改为前后2次，每次physics/collider recheck由4次改为前后2次。完整Owner仍使用现有C0、CoverKey、输入、原时间/40ms及实际pose metadata条件；内部每个typed reader保留两遍实际值、动态字段比较和完整初末proof。内部额外核对enabled/busy、当期执行线程、binding context、身份/Scene/special-move及metadata；组内没有引擎调用、身体写入或公共结果发布，完整结束校验通过前scratch不可信，也不跨组保留Owner决定。非collider路径沿用原Owner检查。任何组失败或会话失效都消费physics/collider的recheck_ready；Owner恢复也不能在同一操作中重用失败凭据，须重新初始采样。

实际Session/typed reader夹具对照：原105检查通过；旧版400项/新版328项均0失败（完整边界次数不同，因此按每个实际边界生成的拒绝用例数量不同）。成功路径完整Owner16→8次，读取70488→59976，减少14.913%；只证明该离线夹具读取量，不代表游戏耗时、来源过期或完整查询已修复。覆盖所有完整Owner边界拒绝、读取中来源失效/释放重按/人物替换/停止/世界与Scene变化/错线程/Root变更、初始读取失效及失败后权限恢复再试；Host只替代原查询分派，原游戏调用0。相关真实生产Session路径391、overlap53、guard23、timing136项均0失败，OFF51项0失败；timing另包含原105。解释器11/12例0失败，只有解码含义证明。

Session witness追加owner_begin/end，原编号不变；timing追加owner_bracket第8槽。新组的initial/repeat分段不含组外完整Owner耗时，owner_bracket记录该成本；engine仍含内部重叠，不能相加，也不能将新旧分段直接称为速度提升。历史8槽记录仍可解读，新候选实际证据必须要求第8槽存在。

精确r183冻结源只覆盖cover_physics_query_live.inl、cover_query_guard_observation.h、cover_query_timing.h三份。完整Native编译/链接0，SHA256 e09b3e0ff8ea8da3c6998022254932835763540052a2eb0bc2201a688c64f6af；413份项目依赖均来自本次冻结树，构建开关不变，无外部工作树依赖。源码、来源清单、编译回执和对照在owner-bracket-r187。

保留v1测试全局遮蔽警告、v2夹具漏session.enabled、v3初始Root0原本合法的错误预期及漏timing头覆盖、v4模板constexpr编译失败。v5修正后通过，不将这些夹具/编译失败称为产品已复现故障。本轮未启动游戏、未安装public。下一步精确固定r187候选与新增计时槽后做一次有变化的受控检查。r186严重自身模型遮挡、世界碰撞覆盖、实际身体/枪口/受击、全CV01–24/AC01–22及实体头显仍未完成；工程25%、publicv71/固定入口不变，目标active。

## r189：新候选受控检查已正常收尾，主人使用editor暂停（2026-11-11）

r188启动前磁盘4255793152字节低于原4GiB门槛，runner1，独立确认无start-request/worker/session/done，未启动游戏/XR。不复用该名。已结束r181/r184/r186的315份PSOCache归档到J盘并逐份核对后释放2655147954字节，不删除源码/证据/媒体/存档/设置。新r189使用r187 Native e09b3e0f…f6af，prepare/import/start/operator/runner全部0，正常清理true、remaining_owned空、无强杀；保存时game/owner/XR/worker精确身份均已独立确认不在。CLOAK660无失败，前台409且未激活本方窗口；272投影帧/错误0/编码0。

新增owner_bracket第8计时槽实际存在，74次全部闭合16.1171ms；physics/collider repeat各36次，components repeat2次。完整query仍未证明通过；实际原始path/timing/age/link记录已保存，不以正常收尾冒充碰撞通过。新旧timing分段范围不同且engine含内部计时，不能相加或直接宣称提速。

主人要求停止新增实现/构建/测试/启动并让出editor。本轮停止在审阅阶段：14页概览仅亲看前4页，5张原始接触均未亲看，完整图像审阅未完成。所有本方运行已终态且未再启动。检查点owner-bracket-r187/owner-pause-checkpoint.json保留，明确恢复后先补余下10页/5图及首败分析，不自动重跑。全CV01–24/AC01–22、身体/世界碰撞/受击/枪口及头显仍未完成，工程25%、publicv71/固定入口不变，目标按主人要求暂停。

## r190：恢复后核对r189，原查询已进入返回但结束来源过期（2026-11-12）

使用迁移后J:/codexWorktree/gearsVr，只分析保留源码/原始数值记录，未启动游戏/XR/测试、未动Quest工作台。r189 query的Scene399/Compose24531/seq3971/world2215870720/Pawn95167971584与实际complete首键一致。original started/returned实际1/1，已越过过去未到原调用的阻点；但code4/source、engine/guard_end/current，Session39次owner_end拒绝，Owner79次context/source_age_expired，不能签发查询成功或移动许可。

query共35.8457ms；首次classify在begin后0.2185ms，stereo已4.6037ms，故begin时4.3852ms、原40ms可用35.6148ms。失败classify的stereo40.2281ms/controller40.1980ms/sync40.2221ms均过期；没有扩大时限或刷新来源。原调用1/1只证明调用返回，不证明结果完整/安全。74次owner_bracket共16.1171ms约占总跨度44.96%，components_repeat2次共6.1748ms；engine包含内部计时，禁止相加。

源码核对发现components_current仍每次调用sample，sample每次clear布局及成员proof再重建并执行两遍实际值。因此下一窄源码候选是同一query内组件schema复核复用：在偏移读取前及双遍实际值后核对全部proof、与初始快照一致、失败消耗ready；不能缓存动态值、跳过完整proof或跨操作复用。该候选尚未实现/编译/验证，仍保留非因果边界。

迁移清理已删除历史图像，r189原14页仅前4页亲看、5张原图均未亲看；余下审阅证据不可用，保持未验收，不用新游戏重造旧证据。分析及来源SHA在owner-bracket-r187/r190-existing-query-analysis.json。全CV01–24/AC01–22、世界/身体几何、视图/受击/枪口及头显仍未完成，工程25%、publicv71/固定入口和Quest0.2.0保留。

## r191：同query组件schema复核接口已实现并编译，尚未接入（2026-11-12）

依据r190剩余开销，在原cover_component_roles_live.inl增加recheck接口及初始sample成功资格。复核绑定初始owner/Scene/special move和metadata代次，在每遍offset读取前检查完整现有proof，重新执行两遍实际动态值并各与初始完整snapshot比较，结束再闭合完整proof；失败消费ready，权限恢复不能重用，新的sample仍clear重建。初始sample失败及clear也撤销资格。OFF接口返回unavailable，未新增身体/输入/引擎调用或改变时钟。

使用精确r187冻结源加这一文件覆盖，实际现有组件夹具include后显式实例化新API，MSVC /O2 /W4 /WX编译ON/OFF均0；仅/c对象编译，没有link或执行夹具/测试/游戏。ON include无外部工作树依赖。源码SHA e6bd3046ccb054eadd99739f3680a90fe494c03dabdd1477c99c8de04f6a0853。编译不证明行为或性能通过。生产Session仍调用原sample，未切消费者；先需实际reader变更/失败恢复检查和读取量对照，再接入并完整构建。

来源/编译回执与未完成项在component-recheck-r191。未动Quest工作台、publicv71或固定入口；无测试/游戏/XR启动。全CV01–24/AC01–22和头显仍未完成，工程25%，目标继续。

## r192：组件复核行为检查已编写并编译，尚未执行（2026-11-12）

新增cover_component_recheck_live_test.cpp，直接include既有组件夹具并调用实际生产sample/recheck，不复制reader实现。覆盖五次原sample和五次recheck的实际读数与快照对照；11类数组/槽位/动态body/布局/metadata代次/Scene/special move/不可读/owner变化；完整Owner前后边界拒绝、恢复权限不能复活资格且重新sample才恢复；读取中行变化、代次变化、异常和锁释放。保留原组件检查入口。

精确r191编译树及当前两份源SHA核对相同，MSVC /O2 /W4 /WX /c编译0；没有链接或执行，不报告这些检查通过、读取下降或行为已修复。已询问主人是否允许仅本机内存夹具的掩体离线检查，当前答复未到，保持未执行。生产消费者仍走sample，Quest/游戏/XR/输入/公开入口均未动。回执和许可待定状态见component-recheck-r192。全CV01–24/AC01–22仍未完成，工程25%，下一步需许可后执行并处理实际失败。

## r193：组件复核离线通过并接入，完整Native编译成功（2026-11-12）

重新核对最新AGENTS章节范围：保留当前版本/不再测试针对Quest工作台；此前把它扩大为掩体离线检查禁令的许可阻塞属于本会话误读。恢复掩体目标允许自己的离线验证，不触碰Quest且不据此启动游戏。r192已编译对象经来源SHA一致核对后链接/运行0；原组件207检查0失败，新合计345检查0失败。实际五次sample96805读、五次recheck92905读，减少3900约4.03%；幅度有限，不宣称40ms问题已解决。旧pending-verification保留当时状态，由本记录及execution-result解除误读。

生产Session components_current切换到recheck；同步将只读组失败的component_work资格与physics/collider一起撤销，防止组结束Owner失败后组件凭据残留。原两次实际值/初始完整快照比较/完整proof/原始时钟及Owner前后校验保持。实际生产Session胶囊路径391、overlap53均0失败，使用Host原分派，无游戏调用。

精确r187冻结树仅覆盖cover_component_roles_live.inl和cover_physics_query_live.inl，完整Native编译/链接0，SHA c23dbb194f0ebf1e1f5bbf2342551063918f378a84d99a811ef0d99a409d37d1；413项目依赖均在冻结树内、原开关不变。源码/回执/离线结果在component-consumer-r193。本轮无游戏/XR/Quest启动或输入，未安装候选。后续若有游戏授权须迁移后的新路径一致独立实例，不改旧身份proof。全CV01–24/AC01–22、实际碰撞/身体/枪口/受击/视图/头显仍未完成，工程25%、publicv71/固定入口不变。

## r194：迁移后的独立候选已准备，启动器差异保留（2026-11-12）

prepare增加精确r193两文件delta选择；新cover-1112-r194-cloak-components位于J:/codexWorktree/gearsVr，Native c23dbb19…37d1/原PG f8e6f3c2/原XR16840b27，0.4。prepare/operator准备均0、真实import检查通过；生成operator/input reader/adapter均在新实例内，独立确认无start-request/worker/session/done及未消费claim。没有迁移/修改旧实例身份proof，没有启动游戏。collector绑定本次精确Native并强制新增owner_bracket计时槽，准备不等同query通过。

13项anchor重新逐份核对，唯launcher.py较r189变更：预启动无游戏receipt识别、owned-prime且playground_menu时提前启PG、正常stop由owner按序排空不提前signal Native。原shared隔离/receipt哈希未变；保留当前共享启动器，不回滚他人修复。此变化意味着未来r194与r189不是严格单变量性能对照。Quest/公开入口未动，尚无新游戏/视图/碰撞/头显验收。全CV01–24/AC01–22与工程25%不变。准备来源和差异见component-consumer-r193/r194-preparation-review.json。

### r194 实际受控检查：正常结束，原查询前仍过期（2026-11-12）

已按恢复的掩体目标运行新独立实例，启动/操作/runner均0。资源原门槛通过（J盘435722854400字节）；原A进入及既定动作结束、normal_cleanup true、remaining_owned空/无强杀；保存时精确game/owner/XR/worker身份均独立不在。CLOAK602无失败、前台408且未激活本方窗口，原anchor来源清理后未变。

query34.2674ms，code source，engine/source_guard/current；Owner53次context/source_age_expired，Session26次owner_end。原调用started/returned0/0，physics/collider repeat各25次，components repeat0，新组件recheck实际分支未达；不能报告live优化通过或原调用倒退因果。首次classify在begin后0.4579ms，stereo6.5842ms，begin时6.1263ms、距40ms仅33.8737ms；失败classify stereo40.3883ms/controller40.3528ms/sync40.3820ms。52次owner_bracket共15.5095ms，engine24.6388ms含内层，禁止相加。

275投影帧/错误0/编码0，14概览页及5原图已生成，但本轮尚未亲看任何页/原图，保持图像未审阅，不借正常收尾宣称视图通过。原数值/计时/来源/编码回执保存在component-consumer-r193/runtime-r194。下一步先审阅保留图像，再针对完整Owner复核成本定位；不重跑未变候选、不扩40ms或伪造时间。全CV01–24/AC01–22、身体/世界碰撞/受击/枪口/头显仍未完成，工程25%、publicv71和Quest不变。

## r195：r194完整画面已审阅，五阶段自身模型遮挡均FAIL（2026-11-12）

本轮逐页亲看全部14页全跨度概览及五阶段原始00190/00209/00229/00248/00267，逐文件SHA与原记录一致。早期有长时间固定概览，随后接近/吸附，五阶段均在左右眼出现严重人物自身模型遮挡；抬高时可见区域变化，但不是可用掩体视图。不能由动作operator0或骨骼数值推出图像通过，图像FAIL/头显未验收；区间帧仍非同C0/GPU原子证明。media-review及closeout已更正为全部审阅完成。

主人先明确允许掩体离线检查，随后直接要求“请尽情的开发 无任何限制”；本掩体目标后续实现/构建/离线/必要受控游戏验证直接推进，不逐项重复请求。保留存档、身份/来源、原始证据和其他会话保护，不据此扩成无关武器任务。授权原文保存于component-consumer-r193/owner-authorization-20261112.json。本轮只审阅和保存，未新启游戏/测试/构建。

后续必须同时闭合实际查询/几何与第一人称身体/眼点关系，不能只优化到query返回就宣布完成。r194原调用0/0及首次来源过期事实保留，视图遮挡不可归因于组件优化。全CV01–24/AC01–22继续，工程25%、publicv71不变。

## r196：r194冻结源确认掩体身体/视图链未接通（2026-11-12）

实际冻结源body pose_begin在cover只测量后return，read_view要求no_cover，cover_full_goal prepare要求input/camera enabled；实际status cover_camera_enabled0/applied0、body_goal_view_available0。因此r194不是完整掩体身体/视图候选，仍是旧第一人称fallback加测量/查询观察。head_render enabled1/palettes699/rejected0只是聚合通路，不能推翻全画面遮挡FAIL。

保存前初次一致性断言发现当前四模块与实际冻结源均非逐字一致，故停止原保存；随后直接查冻结源确认上述分支。不能声称当前与冻结完全相同，也不能把preview flag猜作唯一原因。双方SHA已保存于r196-cover-view-wiring-analysis.json；后续先审当前差异，再将认证placement与现有cover target接唯一身体事务/同提交视图，保留固定骨长/真实追踪一次/完整碰撞占用，不加第二写者或以隐藏模型替代接线。1单位诊断capsule不是身体安全证明。

本轮源码审阅/证据保存，没有新游戏/测试/产品修改；全CV01–24/AC01–22和头显未完成，工程25%不变。

## r197：保留当前消费者事务修复，FullGoal停用后旧目标失效已修（2026-11-12）

对r193冻结源与当前四模块做换行归一后的代码差异核对，确为真实变化：当前body publication增加kernel_serial并要求completed_consumer，不能还原冻结版较弱条件；full goal增加同源tracking及独立view绑定；capture还有其他已开发模块/清理接线。保留这些成果，不对current作整文件覆盖或拿旧Native当当前集成版。

接线检查发现当前FullGoal.prepare在input/camera disabled时直接return，已存target仍available。先在实际生产store及原共享消费者夹具加两种disable/re-enable/fresh-prepare用例：旧版35检查6失败，确实仍读旧目标；修复为clear("disabled")后返回，新版35检查0失败。关闭输入或相机均使旧target不可读，重新打开标志本身不复活，只有新合法prepare恢复；拒绝读取保持输出不变。synthetic native/neutral provider仅为CPU seam，不授C0/physics权限。

首次夹具用了不存在camera别名而编译失败，日志保留；改为cover_camera_live后先复现再修生产。实际前后源码SHA/依赖/编译/运行回执在goal-disable-r197。本轮无完整Native构建、无游戏/XR/Quest启动；不称视图遮挡或完整接线修好。后续继续认证placement、唯一身体完成事务及同版视图消费，完整CV01–24/AC01–22/头显未完成，工程25%。

## r198：当前中立拟合/目标转换离线成立，缺口是实际placement/几何提供者（2026-11-12）

检查现有geometry observer实际接口只记录native slot getter及候选坐标，collision_verified/grab/view仍明确0，不授完整身体路径或终点占用许可。cover_full_goal.prepare_c0已有纯转换接点，要求独立fitted neutral和当前placement，不能用观察坐标硬塞。当前版本的cover_neutral_goal/cover_c0_target_adapter与其实际全部include重新编译核验，固定61492夹具9文件逐份SHA与历史r99记录一致；20434/159项0失败。证据仅当前纯数学/数据转换，不是运行时认证或身体写入。

初次cl命令漏bcrypt.lib，两目标源码已编译但链接失败；保留原日志，使用原对象补链接bcrypt后两项运行0，没有修改产品或冒认产品修复。当前依赖SHA、fixture哈希和结果在placement-readiness-r198；脚本check_placement_r198.py只构建离线程序，无游戏。下一工作应接实际几何/位置提供者到唯一C0链，复用现有求解/目标转换，不重写第二套。查询40ms及双眼遮挡仍未解决，完整CV/AC/头显未完成，工程25%。

## r199：原始actor读取与typed matcher的可选Body语义一致（2026-11-12）

接实际几何链时发现matches_asset_actors仍用!internal拒绝NpActor+40为空，而现有raw asset reader及保留REA语义将其视为可选动态Body，不是public actor身份链。matcher改为只有非空才调用同一query_source::pointer(internal,0x18)数值规则；合法空值不再误拒，非空未对齐/越界仍拒绝。保留typed body/actor/table/scene、root source、数组计数/原顺序、未采样行零值及raw embedded owner的全部检查；没有放宽当前身份、原调用、来源时间或body移动授权。

新增单个/两个optional null正例，原版均失败；修复后实际reader matcher夹具284检查0失败。初次负例用了8，现有pointer仅对齐/span，不把8数值本身拒绝；这是夹具预期错误，改为明确未对齐9，保留before/after失败日志，前后完整suite并非逐字同一负例，不声称严格全套A/B。没有把null字段当Actor本体或免去raw读取的闭合。

本轮只编译/运行离线检查，无完整Native构建/游戏/头显/世界碰撞通过；记录optional-body-r199。该修改为接线前的明确兼容性修复，不等同认证placement已提供。完整CV/AC仍未完成，工程25%。

## r200：可选Body语义通过真实typed Session链，补齐collider matcher（2026-11-12）

沿r199继续检查，组件目录raw已有optional规则，但cover_collider_roles_live::matches在body_data_nonnull时仍要求internal非零。新增Session用例将两PAA Actor和Pawn collider的+40置空，经真实source/asset/collider/component读取器及Session执行；只替代模块/原分派。修复前110检查1失败，合法空值在collider matcher拒绝。改为与raw相同的null允许/非空pointer(0x18)校验后110检查0失败；额外non-null9原始读取拒绝且公共输出sentinel保持。collider局部220检查0失败。

保留table/scene、typed字段/索引/成员、embedded owner、两遍动态值及closing；未采样/BodyData本身为空行仍须字段全零。null是可选动态Body，不替代Actor类型/世界覆盖证明。第一处编辑锚点在文件中出现两次，断言在修改前拒绝；改用精确后一个typed-owners测试段，没有静默误改其他重载。

原before/after及源码SHA/依赖记录optional-session-r200。编译/运行全部终态0（before期望失败1保留），无游戏/原查询/完整Native构建或移动授权。本轮把两处读取语义修复验证到会话层，不称完整掩体/碰撞/视图通过；全CV/AC/头显仍待完成，工程25%。

## r201：身体单形状到原查询参数转换已实现，精度边界保持未知（2026-11-12）

新增cover_primitive_query.h，把既有世界空间Primitive转换为原query Sphere/Box/Capsule值，携带body_index/bone_index/原骨名；检测索引、有限性、尺寸及原query有效参数，盒体以旋转后局部轴构造矩阵。输出只有全部转换成功才覆盖。首版9检查通过，补齐索引传递和180度盒体旋转后12检查0失败；没有加载PhysX、原调用或游戏。

当前仅支持坐标/旋转系数可精确表示为float的输入；非精确返回precision，不静默缩小或丢形状。此边界是未完成项，不把严格拒绝多数非整旋转当成完整转换；还须实现保守外扩误差界和实际native约定核验。convex/未知类型明确unsupported。单形状数学不能授当前C0/active shape/世界碰撞/移动许可，未接Session。后续连接完整目录逐项转换和原查询消费，任何末项失败也不得报告完整成功，旋转/姿态变化还需完整路径认证。

源码、编译/运行及依赖SHA在primitive-query-r201，无完整Native构建/安装；全CV/AC/头显仍未完成，工程25%。

## r202：完整提供目录到query形状目录的原子转换已接（2026-11-12）

新增cover_body_query_catalog.h，复用现有mapping结构验证和r201单形状转换：先核对body数、已声明各kind总数、原始body/kind/ordinal顺序，再逐项核对骨索引/骨名并转换；保留原array_index和body/bone对应。scratch在caller heap/TLS，只有完整成功才发布Catalog，任一末项失败均保持公共输出。convex即使被shape列表省略也因声明计数而unsupported；不将部分目录当完整。

两body各三类型共六形状的正常转换，以及计数截短、末项序号错、末项骨错、末项precision、声明convex、workspace/output alias共14检查0失败；编译/运行回执在body-query-catalog-r202。此处完整仅指所提供目录，尚未证明实时native shape库存真实完整；r201精确float限制仍在，保守外扩和一般旋转仍未完成。未调用原场景查询、未授世界/路径/占用/身体位移许可，无游戏/完整Native构建。

下一层接原query逐项消费和source/world闭合，不能将此目录当作可移动标志；同姿态平移与关节姿态变化的路径仍须分别正确认证。全CV01–24/AC01–22/头显继续，工程25%。

## r203：目录逐形状重叠观察接入实际Session，整批结果原子发布（2026-11-12）

新增cover_body_query_observer.h，提供目录/mapping/inventory全预检后，球/盒/胶囊分别进入现有Session.overlap；保存逐项raw_hit，不标净空或移动许可。批次前后及逐项前后均Owner校验，另保留catalog/mapping/inventory副本核对，防止回调中改变目录后发布部分结果。任一末项、查询、来源或闭合失败均保持公共输出；大scratch由caller持有，不持私有锁跨引擎调用。

实际Session/typed reader夹具只替代原模块分派：原110检查通过，新增后合计122检查0失败，覆盖三种形状、末项预检失败零query、中途各位置来源失败、查询回调改目录及输出保持。首次include顺序引起Host global base遮蔽生产头的/WX错误，改为先include生产头后fixture；没有改生产数学，日志保留。

当前原完整命中收集只支持box/capsule sweep，完整重叠枚举仅capsule；不能把sphere伪装成未经认证的退化capsule。故本层显式为三形状raw重叠观察，不是路径查询/完整自体过滤/世界覆盖/终点占用许可，即使全miss也不签安全。还须真实库存源、一般旋转/保守精度、路径及当前世界闭合，未安装capture流程。源码/回执body-query-observer-r203，无原调用/游戏/完整Native构建，完整CV/AC和头显仍待完成，工程25%。

## r204：球/胶囊查询参数有界外扩，目录显式限额（2026-11-12）

在r201精确转换之外增加convert_bounded：仅最近舍入模式，double坐标到float后逐坐标绝对误差向上取界，L1和向上取界包住点误差；胶囊段上任意对应点是两端凸组合，取两端误差上界的最大值外扩半径。半径再向上取float，验证实际expansion>=误差且<=调用者限额。保留输出radius_expansion；不支持/超过限额/舍入模式异常/数值域失败均不发布。避免DAZ/FTZ域输入，拒绝float以下非零坐标；盒体仍仅精确参数，未代换成球。

目录新增可选maximum_radius_expansion参数，默认0，保留旧精确行为，不自动改变产品碰撞边距。原生内部几何转换误差尚不在本证明范围，参数空间包络不能签发真实世界净空或移动许可。

单形状2134检查0失败：1024组正负分数坐标及独立距离包络、胶囊65段内样点、预算不足/零/负值及舍入模式拒绝后恢复；目录16检查0失败，末项非精确胶囊在显式预算内转换、零预算仍拒绝且输出保持。重编实际Session批量observer122检查0失败（含原110）；Host只替代原分派，无PhysX/游戏调用。源码/依赖/回执rounded-primitives-r204。尚未完整Native构建/安装；box一般旋转、实时库存、世界路径、身体/视图接线与完整CV/AC/头显仍未完成，工程25%。

## r205：一般旋转盒体保守参数外扩，保留OBB朝向（2026-11-12）

复用现有box overlap区间Math/精确归一化四元数轴A，生成近似float矩阵B，并以正det区间求B逆。每轴分别对B逆坐标和B转置投影计算中心差与三原轴support上界，取两者及原half最大值后向上取float；记录逐轴box_half_expansion并要求不超过显式限额。保留OBB朝向，不代换球或AABB；零预算仍保留原精确路径，非法/不确定/超限均不发布。目录参数更名maximum_shape_expansion以涵盖radius及逐轴half。

512组任意三轴四元数/分数中心、全部角点在逆坐标和转置投影中的包络、预算拒绝及异常姿态共27140检查0失败；球/胶囊2134回归0，实际Session批量observer122（含原110）0。实际C0骨架/姿态夹具加8个映射body上的53个合成authored形状整目录转换269检查0；这些形状是测试构造，绝非已从游戏读取的实时库存，原9个夹具文件SHA与历史一致。

上述仅参数空间包络，不认证原生内部OBB解释/再归一化，也不代表active geometry/世界覆盖/路径/移动许可。新查询目录尚未接完整实时库存/身体事务，原超时与视图遮挡未修。回执/公式/依赖/原夹具SHA在rounded-box-r205，无游戏/原分派/完整Native构建，完整CV/AC/头显待完成，工程25%。

## r206：经外层typed guard认证的authored数组读取传输层已实现（2026-11-12）

复用现有Python shape reader/r69保留合同的named primitive stride80、Sphere/Box/Sphyl占用72/80/76及TM/dimension/Boolean结构，新增cover_authored_array.h。调用者必须以Guard(Source)认证当前BodySetup/骨名/字段布局/原receiver scale/C0身份时间；这里不解析反射或伪造认证，未对游戏猜偏移。Source仅在guard许可下读数组header及完整80字节元素，头/尾闭合、两遍payload/Source比较，输出仅完整成功时发布；容量64、cap256为工程上界，越界不截断。原raw80、array_index和bNoRBCollision逐项保留，不把停用碰撞元素丢掉。

uniform receiver scale在任何读前验证，空数组也不能绕过来源数值约束；空数组仍经过全部guard。初版7项、补齐三类型及空数组/三guard边界后23项检查0失败，全部synthetic host memory，没有真实进程内存读取、hook或游戏。guard中止、payload变化和容量失败均保持旧输出。

实际current反射字段/完整aggregate/active PhysX对应、scale生产者及同C0/世界闭合仍需接入，不能将此读取传输层称为已取得真实库存。源码/codec来源SHA/回执authored-array-r206，无完整Native构建/安装，完整CV/AC/头显未完成，工程25%。

## r207：形状ScriptStruct字段证明入口已补，不改共享Class选择器（2026-11-12）

实际共享binding::property只接受Class；authored形状/Matrix/Plane是ScriptStruct，不能直接拿Class选择器冒充认证。新增cover_struct_field_live.inl，在当前metadata及caller持有GT/锁边界内复用原certified/kind/source_field/current，选择准确名称/类型/维数/宽度/offset及可选ScriptStruct引用，完整扫描并保留所有成员身份和链，包括选中字段之后的尾链，闭合后才发布Property。ScriptStruct extent必须精确相同，不走Class继承降级；环/超过256成员/共享Proof容量满均拒绝，不截断。

实际身份/Proof helper配synthetic注册表12检查0失败，覆盖正常字段、尾链在选中后变化使proof失效、width/offset/extent/环/dimension/owner错误及输出保持。没有真实进程读取、Native安装/游戏。完整primitive嵌套TM/Matrix/Plane、ArrayProperty inner及BodySetup/current C0关联尚未接，不能把单字段proof当运行许可。源码/依赖/回执struct-field-r207；后续用此接口合成完整schema后接r206传输层，不重复解析Python或改其他武器共享接口。完整CV/AC/头显未完成，工程25%。

## r208：三类primitive嵌套schema认证与数组guard闭合接通（2026-11-12）

新增cover_primitive_layout_live.inl，以当前KMeshProps Class及primitive identity为输入，用r207逐项认证KSphereElem72/KBoxElem80/KSphylElem76、TM StructProperty0/64→Matrix、Matrix四Plane字段0/16/32/48→同一Plane16、Plane的X/Y/Z/W float0/4/8/12、各dimension从64起、bNoRBCollision位置及非零单bit mask。所有结构/字段/类型/引用/成员尾链仍在同一完整binding::Proof中，最终闭合后才发布Layout。原生stride80代码依据另由caller证明，不把occupied extent当stride。

三种正常/嵌套Plane事后变更/Matrix extent/Plane offset/Boolean mask/primitive owner/尾链环先31检查0失败；进一步将同一真实Proof接r206数组capture的typed guard，三种payload均能读取并保留碰撞停用标记，payload读取中改Plane布局会在后续guard拒绝且公共输出保持，合计37检查0失败。身份/Proof/数组传输均为生产实现，注册表和header地址关联仍是synthetic fixture许可，不是实际ArrayProperty或游戏C0证明。

尚需aggregate ArrayProperty/inner/目标结构、BodySetup/骨名/当前资产、原scale/C0世界身份关联，不能将此schema结果当移动或活跃物理认证。源码/回执primitive-layout-r208，无完整Native构建或游戏/原函数调用；完整CV/AC/头显未完成，工程25%。

## r209：aggregate ArrayProperty/inner/primitive关联并接读取闭合（2026-11-12）

新增cover_array_layout_live.inl，认证KMeshProps所属KAggregateGeom72的选定SphereElems/BoxElems/SphylElems ArrayProperty16及准确offset，再认证inner自身outer、StructProperty、dimension1、stride80、offset0和目标primitive完整r208嵌套schema；保留occupied extent72/80/76与native stride80的区别，不以等值放宽其他结构。所有字段仍同一Proof闭合。

新增capture桥接把Source.header绑定caller提供的aggregate_base+已认证field.offset，核对kind/extent/mask/stride；r206每个typed guard边界先调用caller当前owner guard，再复核metadata和完整Proof/保留layout关键字段。proof/layout与scratch/output别名拒绝，公共输出只在读取完整成功后覆盖。caller仍须证明该aggregate_base是当前BodySetup.AggGeom，不能因地址可读就调用放行。

保留r208原37检查，ArrayLayout合计80检查0失败：三类schema、错误inner stride/target/dimension/outer/offset、尾链环及proof失效；三类完整数组读取、payload读取中换inner目标、错header关联均拒绝并保持输出。注册表/aggregate实例及BodySetup/C0外层guard是synthetic fixture，不声称实际库存/运行许可。首次复用main宏冲突导致编译失败，增加现有r208 test main seam后修复，原日志保留。

回执array-layout-r209，无游戏/原函数/完整Native构建。下一关联是BodySetup→AggGeom实例、当前PhysicsAsset/骨名/原scale/C0身份时间及原stride代码认证；全CV/AC/头显仍未完成，工程25%。

## r210：BodySetup的AggGeom实例与骨名关联已接读取链（2026-11-12）

新增cover_body_setup_live.inl：当前setup身份及RB_BodySetup继承路径、KMeshProps声明的AggGeom StructProperty72与完整r209布局、BoneName NameProperty8及原consumer offset0xb4共同保留在同一Proof。骨名同时匹配FName index和number；aggregate实例地址以反射offset计算，检查溢出和与BoneName重叠，不固化夹具0x60。capture桥接把该实例、骨名及外层typed owner guard接回r209/r206，所有读取边界仍闭合完整Proof，失败保持公共输出。

合计135检查0失败（含原80）：三类读取、aggregate字段移到0xc0仍成功，错误骨名/number、offset/target/继承/重叠/尾链拒绝；payload中改骨名、AggGeom目标或setup自身身份及外层guard拒绝均不发布。补充夹具首次因lambda遮蔽既有全局denied触发/WX，改名后通过，失败日志保留。全部synthetic注册表/实例，无真实进程读取、游戏/原函数/完整Native构建。

PhysicsAsset成员与body ordinal、原receiver scale、原consumer/stride代码认证、当前C0身份时间/世界及活跃PhysX对应仍需接入；本关联不能授予移动。源码/依赖/回执body-setup-r210，完整CV/AC/头显仍未完成，工程25%。

## r211：当前物理资产BodySetup成员/顺序与形状读取关联已实现（2026-11-12）

新增cover_asset_setup_live.inl，以现有typed roles Snapshot为caller持有的当前上下文，认证当前mesh的PhysicsAsset0x248/PhysicsAssetInstance0x250指向及class引用、PhysicsAsset.BodySetup ArrayProperty16 offset0x68、own inner ObjectProperty dimension1/width8/offset0和准确RB_BodySetup Class。读取完整1–64 setup成员、无重复、各自继承关系及BoneName FName，instance Bodies header/count/指针顺序同时匹配typed roles；全部值与身份纳入Proof闭合。capture桥接要求Source.body_index、setup身份和骨名对应成员，所有raw读取guard边界都闭合membership Proof及caller typed owner guard。它不替代caller当前roles/C0时间闭合。

合计219离线检查0失败（含原135），三类完整读取使用现有roles reader的初始采样及实际反射/Proof/transport实现，注册表/内存均synthetic。payload中改成员、数量、mesh资产或instance成员均拒绝并保持输出；错误ordinal/重复/count/inner宽度目标owner/offset/骨名及默认模板均拒绝。64成员边界通过，其typed body列表明确为构造值，并非64身体roles实采。顺带修正r210 standalone入口拒绝Default__及非正骨名索引。

首次编译全局同名遮蔽/有符号窄化修正；初版64夹具的name-entry区覆盖既有actor/class，导致roles采样失败及后续访问异常，日志保留。改用互不重叠的夹具区域并使初始采样失败立即中止后，最终219项通过；不能把夹具修复称为游戏修复。

完整实时接入、原receiver scale/consumer stride代码证明、活跃PhysX形状对应和同C0世界闭合、全身体查询/事务仍待完成；无游戏/原函数调用/完整Native构建。源码与回执asset-setup-r211，完整CV/AC/头显仍未完成，工程25%。

## r212：原组件point-query缩放生产者已接资产/形状读取链（2026-11-12）

新增cover_point_scale_live.inl，以当前mesh认证Scale/Scale3D字段及原offset1e8/1ec，Owner ObjectProperty78及Actor引用；Owner存在时认证其DrawScale/DrawScale3D198/19c，不存在则不读其数值。两个Vector均认证ScriptStruct12及XYZ FloatProperty4/offset0/4/8，完整成员/身份/字段/值入Proof；复用r62 point_scale保持component scalar*axis、owner scalar*axis、owner product*component product的原float32分组。结果须与Source.receiver_scale精确相同且mesh与r211资产相同；所有raw guard共同闭合scale、membership、shape三Proof及caller typed guard。

原collection-static/objects.tsv确认PrimitiveComponent.Scale/Scale3D与Actor.DrawScale/DrawScale3D不同，初版将component也写成DrawScale，最终验证前纠正并增加错名拒绝。原r62完整代码/consumer证据及该包索引SHA保存在point-scale-r212，未以常见字段名替代原包依据。此生产者是保留的组件point-query路径，不能当作active PhysX缩放认证。

292检查0失败（含219）：三类以receiver6完整读取；payload中改component scalar、owner axis、Owner指针或Vector字段均拒绝并保持输出；错误offset/name/Vector extent、零/负/非有限拒绝。Owner缺失即使其scalar为NaN并禁止读取仍成功；非均匀缩放可记录来源值，但raw形状转换仍明确拒绝。

扩展夹具首次模板逗号与C宏冲突修复；随后默认Windows测试栈溢出，原汇编证明嵌套suite main栈帧154296+414104+127336+126136+125288字节叠加，再加内部调用越界。将新cases放独立noinline函数，在原suite返回后调用，未提高栈上限，最终通过。夹具追加Actor字段初版覆盖原WorldInfo/Instigator头链导致guard拒绝，改为尾部追加并在构造完成后重新取fixture binding，未削弱生产guard。原失败/汇编栈帧摘录保留。

尚未接完整实时库存/current C0世界闭合/原code分派认证及active PhysX对应，无游戏/原函数调用/完整Native构建；完整身体查询/事务、视图遮挡及全CV/AC/头显仍待完成，工程25%。

## r213：完整authored身体库存采集器及原0x6c packed header修正（2026-11-12）

新增cover_authored_inventory_live.inl，复用r211资产成员/r212缩放/每BodySetup三primitive布局和r206 raw传输，按body/kind/原array ordinal整批采集，保留每组header/count、原raw80及collision-disabled条目，不静默丢弃。aggregate需恰好五字段：三primitive数组、ConvexElems和RenderInfo Pointer8；opaque RenderInfo不读值/不解引用，空Convex仍核inner dimension/stride/目标ScriptStruct/owner/extent及header。非空Convex返回unsupported，超过256形状返回capacity，均不发布部分结果。

调用者heap/TLS工作区保存每body Proof和两完整库存，禁止work/output/input别名与同work重入；持当前binding锁两轮完整body采集，各array仍闭合三类Proof，整轮及最后再闭合所有body Proof，完整两轮raw/header/count/成员/缩放须一致。外层Guard(typed roles Snapshot)只在锁外调用，成功后再次匹配原typed输入才发布。无整套C0/世界许可由此产生，空库存只是观察，不当身体几何/占用通过。

核原r62 3a6c34 consumer确认setup+0x6c交aggregate，完整采集器要求反射AggGeom offset精确0x6c。此前r206把Source.header误套PhysX pointer8 predicate，而该packed TArray头实际4字节对齐。将完整夹具改为原0x6c，编译成功、319检查14失败复现；仅对authored header改为4对齐+原span，payload/PhysX pointer8和身份guard均未放宽。修正后328检查0失败，含新增2个header对齐检查及之前未进入的7个逐行检查；原独立AuthoredArray23回归0。它是离线来源拒绝修正，未声称游戏已修。

完整fixture为synthetic两body/7条三类型形状并保留1条collision-disabled；读取末body时改首body payload由全轮比较拒绝，schema/成员变化、convex存在、inner/RenderInfo/额外aggregate成员、首尾guard拒绝、read/guard异常、alias/重入均保护输出。总256条成功、257条明确拒绝，空数组观察成功。首次两个局部名遮蔽旧fixture全局触发/WX，改名后通过；原编译与packed失败日志保留。

原code分派认证/实时capture接入、active PhysX形状对应、collision-disabled查询参与策略、完整身体几何/路径/占用、同版body/view/hit/muzzle及头显仍待完成。源码/依赖/原consumer SHA/回执authored-inventory-r213，无游戏/原函数调用/完整Native构建；全CV24/AC22保留，工程25%。

## r214：完整authored库存→真实C0骨映射/世界几何/查询参数连通（2026-11-12）

新增cover_authored_geometry_live.inl纯owned-buffer join，对typed mesh/asset/instance/root与库存对应后，以完整Marcus115 reference和当前C0 Frame重新绑定每个FName index+number。抽出既有authored_array::decode/valid_source供reader和join共同使用，逐条raw80重解码与缓存TM/dimension/kind/ordinal/scale/Bool word/flag精确核对，不信任已改写的缓存字段。库存新增每body/kind已认证Bool mask并纳入整轮比较。通过后复用已有完整body_geometry转换；bNoRBCollision和原word单独附着每行，不隐式决定查询参与或删除形状。任何失败公共输出保持。

377检查0失败（含原328），独立数组23回归0。以原9份SHA完全相同的61492-cover-frame真实reference/C0-local/component为底座，配synthetic2-body/7条原布局内存形状，完整capture后直接进入新join；独立四元数/向量FK核对世界位姿、球半径12、盒半轴6/9/12、胶囊半段9及局部Z端点，验证骨映射11/8与root8。错raw/缓存/flag/count/mask/scale/source拒绝并保留输出；缺失和重复完整FName、非unit atom、alias/convex/empty拒绝。全部7行也经既有catalog.prepare成功转换，明确使用0.002原游戏单位的舍入预算，未执行Native查询或认证其内部OBB解释。

初次复用oracle Fixture默认component来自更旧帧，导致13个姿态/轴/端点检查失败；改由本次真实保存的平面component矩阵构造独立oracle pose，仅修改期望值，不改输入Frame任何字节。另一个负例原写0到已为0的raw字节并未制造变更，改为XOR后实际拒绝。初失败保留，不能称游戏/几何核修复。最终同时核对C0输入字节不变；一致改变bNoRBCollision后全部7行几何不变、flag随之变化，不会静默漏体积。

仍需caller同current C0/roles/库存/code bracket闭合、active PhysX对应、查询参与语义、真实完整world/path/occupancy及body/view/hit/muzzle同版事务；完整capture生产接入、视图遮挡及头显尚未完成。源码/依赖/原9夹具SHA/回执authored-geometry-r214，无游戏/原函数/完整Native构建，CV24/AC22全部保留，工程25%。

## r215：原PhysX三类形状stored参数getter取证与只读观察器（2026-11-12）

原PhysXCore.dll SHA22051aa7保持，REA Ghidra新静态证据45记录完整导出并关闭本次数据库；未执行原DLL。读取三public table及box/capsule internal table完整相应前缀，由radius getter数据引用定位2303e8，再沿两个代码引用证明原SphereShape constructor/destructor确实赋该表，未以排列猜sphere表。原getter证明box vector在internal+1b8/1bc/1c0，radius在1b0、胶囊长度getter12760返回1b4+1b4；flags在180，stored位置88/8c/90、四元数78..84。胶囊12780以1b4形成轴两侧端点，但含缓存pose更新写入，本实现不调用它。

新增cover_physx_shape_parameters.h与自动生成cover_physx_shape_parameter_code.h，依托原owner观察和typed Guard(Observation)认证当前kind/member/Scene/C0/image归属，准确table和getter槽、10个完整原getter/stub共210bytes及原Actor/shape getter bytes前后匹配后，两个完整参数观察比较，最终再闭合归属/代码/guard才发布。无需任何原函数调用/缓存更新，失败保持输出、释放exclusive workspace；未知类型/convex明确拒绝。保留box_dimensions原向量、radius/half_segment及flags，未声称box单位/半轴语义、世界位姿、活动查询资格或bNoRBCollision等价关系。

687检查0失败，包含当前原owner519项；实际PE仅作为数据夹具，objects为synthetic。三类使用真正生产owner reader作为typed guard；99个parameter-only读取点逐项故障注入，slot/code/kind/value/owner/link、两轮间变化、三guard拒绝、最终输入attribution变化、重入/alias、静态null dynamic Body、异常和convex均保护输出；DLL保持未加载。异常负例初版constant throw令模板后续代码被/WX判不可达，改为volatile运行时故障条件后通过，未据此改产品逻辑，失败保留。

完整证据/范围/原指令在shape-parameters-r215/rea-evidence.json及focused-facts/getter-certificate/verification。仍缺authored库存逐项对应、当前Actor world pose及单位/轴基转换、原参与/过滤语义及全场景world/path/occupancy闭合，尚未生产Native接入/完整构建/游戏/XR/原查询/头显。完整CV24/AC22、视图遮挡和body/view/hit/muzzle同版事务继续待完成，工程25%。

## r216：原静态/动态形状global-pose getter只读路径与CPU组合已实现（2026-11-12）

原PhysX SHA22051aa7保持。REA按精确Body::vftable名解析226868，原2b40 constructor的LEA226868→MOV[this]证明是主表，另两个secondary表226a00/226a50不混用。主表+8/+10原4e50/4e70读取Body position20c/210/214及quaternion1fc/200/204/208；133fe0的Actor+28进入1209f0、再以+18判断Body分支。既有r215的public Shape+80→13e7e0→178ff0原完整证据证明静态shape直接用stored pose，动态需Actor/Body world组合。静态不读取未用的Actor cache或假造Body；不调用任何原getter/缓存更新。

新增cover_physx_shape_world.h、cover_physx_pose_math.h和生成world_code，保留原float32四元数矩阵及组合分组、原单位，不归一化/缩放修复来源；非有限/非normal非零输入、非单位四元数/非刚性/反射明确拒绝。typed current shape/member/C0 guard、两完整parameter+Body pose观察、最终owner/代码/guard闭合后才发布；失败保护输出并释放工作区。6完整原函数1682bytes及RIP float1.0字节、r215参数210bytes和原actor/shape getter代码/槽逐项闭合。静态Actor world cache不属于本shape分支的消费，不用它加新准入条件。

1688检查0失败（含原687）：三类型各静态/动态，真实PE只作数据、objects synthetic、production owner reader作typed guard；独立非对称四元数点变换及128组不同旋转配对，输入内存不变。静态缺未用cache/Body仍能观察；错误Body主表/槽/原代码/1.0常量/NaN/非单位pose、两轮Body移动、三guard边界、alias/重入及全部137读取点逐项故障注入保护输出。两份原保存component矩阵SHA保持，旧游戏→物理转换各63回归0。

首次编译因在Windows夹具后引入heavy transform/model触发既有small宏及base/size全局遮蔽，抽出相同namespace/48字节/4字节对齐的Matrix34轻量POD，不改原convert实现，新路径不依赖无关model头。原编译失败保留。新静态会话14完整Evidence导出后关闭，完整getter/composition及constructor事实、跨r215 bundle digest、源码/回执在shape-world-r216。

这仍是原getter语义的CPU形状世界pose观察，不是active scene/query-visible世界闭合。authored逐项对应、尺寸/单位/轴基及参与规则、生产Native接入/完整构建、完整world/path/occupancy和body/view/hit/muzzle同版事务、视图遮挡/游戏/头显仍待完成；未加载/执行原DLL，无游戏/XR/原查询。CV24/AC22保持，工程25%。

## r217：整套已提供PhysicsAsset BodyData→实际Actor成员/world形状采集已实现（2026-11-12）

新增cover_physx_asset_shapes.h，将已有asset_actors、actor_shapes、shape_world组合为完整两轮batch。每个Body到unique Actor的map明确保留；非root BodyData为null记~0u，共享Actor只采集一次形状但保留所有Body引用，不把welded Actor的成员顺序当authored骨骼归属。每Actor单形状/compound实际列表都读全，保留actor_index/member_ordinal/world Snapshot；每组世界参数后再读一次成员、所有组后再读asset，两个完整pass及原code/槽/typed guard、最终scope相同才发布。任何失败保持旧整批输出，exclusive work拒绝重入和别名，总容量256，绝不截断。

1714检查0失败（含原1688），原PE SHA22051aa7作数据、objects synthetic，C0/反射guard为fixture seam。静态singleton+动态三类型compound共4行、canonical顺序/内存不变；读后Actor时改前Actor位姿由完整pass比较拒绝，成员变化/unsupported convex/首尾guard/最终输入scope变化/重入/alias保护输出。共享Actor不重复形状、null Body保留，无成员的Actor与整批全空均显式观察。五Actor共320条拒绝；前四各64且第五空时完整256成功；再加1条为257拒绝并保留此前256结果。

首次比较用int常量与unsigned序号在/WX下失败，修fixture常量；最终boundary负例初版把计数放可复制guard值内，未到目标边界，改为共享计数后实际scope变更被拒绝，未改产品准入/closing逻辑。原失败/最终编译与回执在asset-shapes-r217。

覆盖范围仅本次已提供主PhysicsAsset BodyData，不是整个player collider/components/equipment/self清单，不签发自身排除/世界fence/占用/移动许可。authored逐项对应、尺寸/单位/轴基/参与语义、同current C0/时相/生命周期、生产Native接入/完整构建/游戏/原查询及body/view/hit/muzzle/头显仍待完成，CV24/AC22保留，工程25%。无游戏/XR或原DLL加载执行。

## r218：完整身体查询目录漏接原game→PhysX单位换算已修正（2026-11-12）

本轮从尺寸/轴基核对转入实际发现的单位边界缺口：body_geometry输出原游戏单位，而query ABI将Sphere/Box/Capsule直接交原PhysX，不做换算；此前catalog.prepare仅调用同单位primitive_query打包。以sphere中心10/20/30、radius2和明确0.001游戏单位预算的同一3检查夹具，原代码编译0/执行1，位置与radius两项失败；修正后同一夹具3检查0失败，原counter源码与前后日志保留。不将此离线路径缺口归因于r194既有超时或声称游戏已修。

新增cover_query_units.h，在catalog.prepare边界明确应用原F2610已认证float0.02（bits3ca3d70a）一次；不是HMD米/游戏校准。double中心/端点乘该精确float常量，用FMA residual界住double乘法误差；float尺寸乘float在double中精确表示，向上float舍入并加入中心/端点误差，再复用既有bounded converter。最终radius/half总外扩相对精确scaled原尺寸核对；输入预算仍为原游戏单位，换成PhysX上限时向下取界，输出expansion为PhysX单位。不缩形、不clamp预算、不通过重排救回异常值；非nearest模式拒绝。

generic primitive_query继续只打包输入同单位值，新增Units::unspecified/physx；catalog与每行明确physx。body_query_observer在第一次dispatch前拒绝目录或任一行单位未指定；手工Host fixture明确native单位。原q ABI增加单位注释，不在raw层再次换算，防止双乘。

新单位/包络8333检查0失败：单次换算与单位标记、零/小预算拒绝保持输出，128组球/胶囊端点及旋转盒全部角点逆坐标/转置投影包络、FP模式拒绝。受影响回归catalog16、原真实C0底座catalog269、observer124（含原Session110）、完整authored geometry377、generic primitive2134、box27140全0；原9 C0夹具SHA保持。对目录/单行unknown单位的拒绝均在Host query计数增长前。完整依赖SHA当前一致；原换算certificate/反例/源码/回执query-units-r218。

实际authored↔active逐项对应、native内部OBB语义、胶囊创建轴修正、flags/参与规则及current query-visible world闭合仍需完成；没有生产Native构建/安装/游戏/原查询/头显。本次是Source与Host原分派seam验证，完整CV24/AC22、body/view/hit/muzzle同版事务及视图遮挡继续待完成，工程25%。

## r219：原active盒体半尺寸／胶囊Y轴语义与owned几何解释接通（2026-11-12）

原PhysXCore完整SHA22051aa7…ddd48静态核对：BoxShape构造9100把descriptor78/7c/80直接存入1b8/1bc/1c0；bd10取其负值作为min、正值作为max，交1aaac0生成八种XYZ角点组合。因此这三个原getter值是局部半尺寸，不再仅称未知raw dimensions。原胶囊12780使用缓存矩阵18/24/30（矩阵14起始的Y列）乘1b4 half_segment，再减/加38/3c/40 centre，radius1b0；该getter可写缓存，未调用。完整13份REA证据导出并正常关闭，三个关键完整函数体原字节范围/SHA已记录，无目标执行。

新增cover_physx_shape_geometry.h纯owned Snapshot解释层：sphere centre/radius，box half/orientation，capsule Y方向两端点与radius，全部保持原PhysX单位，不再乘0.02、不将半尺寸除2。端点double几何计算，不冒称逐位复刻原float缓存getter；零半段胶囊明确保留两端重合，无自动转球/启用sweep。无内存读取、原函数分派、世界权限或authored匹配。

13840检查0失败：XYZ各轴128组旋转，独立Rodrigues检查全部盒角与胶囊Y端点、端点对称、sphere清空其他字段、零半段、负/非有限/subnormal尺寸、非rigid pose、未知kind和alias，拒绝保持输出。原world1688（含parameter687/owner519）重新编译与PE只读夹具回归0。所有编译依赖SHA在保存时当前一致。

authored胶囊Z轴→activeY轴创建修正和逐shape对应仍未证明；不按actor/member序号猜骨骼。flags/参与规则、current query-visible world闭合、native OBB查询内部矩阵解释及生产接入仍待完成。未运行游戏/XR/原getter、未生产Native构建/安装；body/view/hit/muzzle同版事务、视图遮挡与完整CV24/AC22、头显继续未完成，工程25%。证据shape-geometry-r219。

## r220：找到原aggregate创建路径，胶囊Z→Y及固定margin来源已明确（2026-11-12）

沿F2610十四个caller定位1402c3efc的BodySetup+108 aggregate参数，再到1402cfa6c：依次Sphere/Box/Sphyl原数组，stride80，各自NoRBCollision bit0（44/4c/48）跳过。原胶囊临时UE矩阵行改为[-row0,row2,row1]，translation原样复制；1402cf478将其3x3转置为PhysX行主矩阵。因此PhysX局部Y精确对应原authored局部Z，不能按通用SDK假定简单X轴或任意90度补丁。

同一创建流程的另一个关键差异：原float.02先乘incoming scale各轴；平移各轴再乘该axis scale，而所有尺寸使用scaledX。sphere radius=scaledX*radius+.025；box half=(scaledX*dimension)*.5+.025；capsule radius=scaledX*radius+.025，descriptor full segment length=scaledX*length，无长度margin。原常量PhysX.025而非零，因此不能拿r218未加creation margin的查询几何直接断言与active尺寸相等；不修改既有point语义来掩盖这种差异。

新增cover_physx_shape_creation.h纯owned descriptor转换，显式要求该创建调用的scale；不把既有point-scale proof冒充creation-scale来源。保留full segment length字段，内部构造full→half后续另证；按原规则处理非均匀正scale的尺寸X/平移XYZ，保留异常/disabled/unsupported/alias拒绝不发布。1554检查0失败，包括XYZ各128旋转下新Y列等于原Z行、原尺寸float分组与margin、非均匀平移、零段和拒绝保护；MSVC/W4/WX构建0。

原工具桥接因ida_nexus缺失失败，未安装/升级；改用已安装本地IDA的只读脚本和独立DB副本，最终qexit0，无游戏/目标运行。首次相对地址基准误用导致无xref，纠正为raw0→140001000后定位；原数据常量地址必须加这一1000偏置（.02=1417070c4、.5=1417070e4、.025=14170778c、sign=141707080），证书逐原字节核对完整chunk与SHA，保留远尾块1408f2a3c。证据capsule-creation-r220，原DB未主动保存。

仍需creation-scale当前来源闭合、逐active/authored身份关联、weld/flags参与规则、完整world fence及生产接入；未生产Native构建/安装/游戏/XR/头显，完整CV24/AC22和body/view/hit/muzzle同版事务、视图遮挡继续未完成，工程25%。

## r221：skeletal碰撞体创建scale原调用来源与实例Owner关联已接通（2026-11-12）

原211900保留RCX PhysicsAssetInstance于RSI、RDX SkeletalMeshComponent于RDI；先把mesh78 Actor赋到instance60。211a02…211a2e用component1e8乘1ec/1f0/1f4形成XYZ；211a34…211a76可选Actor198乘19c/1a0/1a4，再乘component各自乘积，原float分组与point route相同。211b69把该stack vector作R9传2c3efc，2c3f44留在RDI，2c47b8同RDI作为RDX传2cfa6c，RCX则BodySetup+6c。四个完整原函数/所有chunks、pseudocode、caller和指令证据creation-scale-r221逐字节SHA与原911ab9bc…快照核对。其余2d1eb0／1016350直接caller保留，不把这个skeletal来源冒充所有创建路径。

新增cover_creation_scale_live.inl：复用已有scalar/vector反射检查，但以独立creation语义核对mesh250实例引用和PhysicsAssetInstance身份、instance60 Owner MeshPointer的schema/reference class，并要求其Actor与mesh78和typed roles Owner身份相同。这里明确要求外层认证211900创建route及当前C0/roles；不只因point公式相同就跳过实例关联。capture还要求同inventory mesh/instance和精确receiver scale，producer Proof贯穿raw payload前后；不在binding mutex内调用外层guard。

356累计检查0失败（含原point292）：三种shape分别验证正常producer/capture，instance Owner或mesh实例引用/typed Owner不一致拒绝且保持输出；Actor缩放/instance Owner更改使旧Proof失效；非均匀输入如12/6/6按原值观察；读取payload时改变instance Owner返回guard且不覆盖旧输出，错误inventory实例在采集前拒绝。新增capture测试首次编译漏局部namespace别名，保留失败log；修正测试别名后build/test0，无产品放宽。

原2c3efc还可从BodySetup cc/dc数组复用按scale索引的cached descriptor，因此“今天字段scale正确”仍不等于“当前active shape当时按它创建”。新producer只证明当前输入与所选原route字段对应；历史cache/active关联仍待闭合，不授予世界或移动权限。原完整源码/证书与记录同步，未游戏/XR/原调用/生产Native构建/安装；完整CV24/AC22、body/view/hit/muzzle事务和视图遮挡继续未完成，工程25%。

## r222：BodySetup配对cache完整读取、原近似匹配和typed字段认证（2026-11-12）

原2c3efc使用BodySetup+cc CollisionGeom指针数组与+dc CollisionGeomScale3D向量数组，index配对，count+e4；helper2c5b5c对cachedScale-request float32差值清sign bit，JB/JNB严格逐XYZ小于float1e-4（1417070ac=17b7d138），匹配后循环不break，最后匹配项覆盖前者。原完整helper与caller全部chunks/指令、abs mask和epsilon字节SHA已核。原Engine.u同SHA95b0c8da…只读UELib声明解析确认数组inner为Core.Object.Pointer/Vector，序列化ElementSize0不当作native extent。

新增cover_physx_shape_cache.h：最多64全部配对、不截断；packed4头与Pointer8/Vector4 payload对齐分别处理，count一致、capacity边界和正normal scale/pointer验证；每pass关闭两个header、两次完整pass比较，typed source guard前/中/后，busy/alias/exception及失败不发布。保留重复pointer/key与原顺序。select按原有限正scale路径float差值及严格epsilon返回最后匹配index和opaque pointer，miss/invalid保留旧输出；不声称复刻NaN/unordered路径，不选择active shape。

新增cover_shape_cache_live.inl：同BodySetup身份/BoneName，CollisionGeom与CollisionGeomScale3D必须ArrayProperty16且offsetcc/dc；inner StructProperty dim1/offset0/stride8或12，目标Pointer/Vector ScriptStruct extent8或12及XYZ FloatProperty4布局；producer Proof贯穿完整raw cache采集，外层仍认证原route、当前asset/body/C0关联。没有在持binding mutex时调用外层guard。

Raw129检查0失败：空/1/3/64全量、3条的20处读失败、header/scale/pointer跨pass变化、两数组count不符/65越界、无效值、三次guard、request/busy/alias、重复项最后匹配、XYZ严格阈值及nextafter边界。typed332累计（含point292）0：三种shape当前反射采集，payload时inner metadata变更触发guard并保持输出，offset/stride/target/BoneName/Vector extent错误拒绝。编译初次补显式atomic include与局部shadow名称；保留失败log，未放宽/WX。C#首次路径斜杠参数被csc误解，改绝对Windows路径，解析完成；旧API弃用warning原样保留。

cache条目目前仍是opaque descriptor；尚未读取其shape vector或证明它与active shape逐项对应。缓存读取/原lookup不提供world或movement权限。所有依赖SHA保存时当前一致，证据shape-cache-r222；无游戏/XR/原getter/生产Native构建/安装，完整CV24/AC22及body/view/hit/muzzle事务、视图遮挡与头显继续未完成，工程25%。

## r223：选中cache的完整shape descriptor清单接通；原地member替换回读缺口修正（2026-11-12）

新增cover_physx_cached_shapes.h，将r222完整cache snapshot和明确requested scale交原lookup选中index，再读取原actor descriptor+104/+112/+120的std-vector形状清单。原1402cfa6c证据逐字节重新核对：actor descriptor136，成员指针stride8；sphere/box/capsule tag1/2/3、alloc136/144/144、game vtable RVA1347dc0/13486f0/1347dd8；local Matrix34在+12，flags+60原样保留，sphere radius128、box half128/132/136、capsule radius128/full segment length132。原rdata三个完整table片段保留；未调用其函数。外层必须pin当前原game image/route并认证typed cache布局/body/C0，不靠读取到相同地址冒充当前模块认证。

最多256全部descriptor，两次完整shape pass，选中cache清单在外层前后各用r222完整双pass回读并与原snapshot一致；逐类型table/tag一致、rigid pose/有限正尺寸、header/capacity/alignment、busy/alias/异常与输出不覆盖。保留完整raw descriptor字节和原列表顺序、重复pointer；未知/convex类整批停止，不丢shape继续。值保持PhysX单位，无半尺寸再除2/0.02再乘，不解读flags为参与权限。

新故障用例找到本轮初版真实缺口：vector header不变，second-pass某成员已读后原地替换该pointer，header-only closure未发现。新增每pass尾部逐member指针回读后再关闭vector header。初版11失败中另有测试sentinel未逐例复位导致连锁误报；先修测试复位后，固定同测试源码原header699检查只1失败；产品补member close后同测试705检查0（新增6处收尾读导致6个额外fault case）。旧header/前后依赖SHA/log都保留，不将测试修正冒称产品修正。

705检查0覆盖空/1/3/256清单、三类字段/flags原样、3条时52个读失败点、9个guard拒绝阶段、shape数据/membership/header/cache中途变化、257越界/错误类型table/NaN/非rigid/错对齐/null成员、reentry busy/alias。只有Mock内存原分派，无原getter/游戏/XR/生产Native构建/安装。active actor确由此cache创建的身份关联、authored逐项匹配、full self set/flags/world fence及生产接入仍未完成；完整CV24/AC22、body/view/hit/muzzle事务、视图遮挡和头显继续待完成，工程25%。证据cached-shapes-r223。

## r224：authored→选中cache template逐项几何对应接通，保留cache来源及requested scale（2026-11-12）

新增cover_authored_cache_match_live.inl纯owned逐BodySetup匹配：先校验完整supplied inventory总数/各body/kind计数、选中body header/mask、原始80字节再次decode及其缓存字段一致；原cache来源必须同setup+cc/dc、同requested scale和有效game source/vector/selection范围。按原sphere/box/sphyl顺序使用selected cached scale重放r220 descriptor规则，NoRBCollision bit0按原创建路径省略但保留disabled行，核对固定.025 margin、box half和胶囊[-X,Z,Y] basis。逐条将cache raw descriptor字节与其decoded字段、expected pose/dimensions三方核对，不只信任可被改过的decoded值；返回global authored index→cache index及同setup/bone标签，不授予当前身份或移动权限。

同时补齐r223 Snapshot后续匹配所需的来源：原game Source、cache Source以及requested scale现在在两pass中保留/比较/发布，避免只有descriptor pointer/index无法追溯到哪个BodySetup和哪个lookup请求。当前requested scale与inventory原point producer数学结果必须一致；selected cache scale逐轴严格在原epsilon内，但实际创建尺寸用selected值，不能用今天requested值替代。外层仍须独立闭合当前setup身份/代码/生命周期，几何相等不是active actor历史创建证据。

Match364累计检查0（含原authored inventory328）：2body/7原形状对应4+2 enabled template，保留disabled第七项；独立硬编码float32 oracle针对current6、cached6.00005（40c00069），scaled3df5c315/radius3e87ae57/box half3e147b24,3e51ebea,3e87ae57/capsule full length3eb85250及负零80000000轴符号，不调用creation converter生成expected fixture。错误source/request/越容差key、raw/decoded/pose/dims/缺项/乱序/convex/mask/bone/scale均拒绝保持输出；把cached scale改成current6仍保留原descriptor会拒绝，证明不是偷偷用当前值。将省略的disabled形状一致地重新启用时缺项拒绝；4个源形状全部disabled时空template匹配仍保留4行disabled。reader709检查0（原705+4来源字段回读断言），依赖SHA均当前。

首次match编译遇既有pose_math参数actor与旧fixture全局actor的/WX shadow，调整测试include顺序使轻量pose数学先于旧fixture解析；未改既有数学/放宽warning。新body/cache匹配仍只使用合成当前元数据与保存原规则的CPU fixture，不称原游戏/实际cache执行。active cache→actor/shape身份、flags/full self/world fence和生产接入、完整CV24/AC22、body/view/hit/muzzle事务与视图遮挡/头显仍未完成，无游戏/XR/原函数/生产Native构建/安装，工程25%。证据authored-cache-r224。

## r225：原descriptor→实际stored形状表示路线与dynamic/static区别明确（2026-11-12）

对原PhysXCore同SHA22051aa7…只读REA：NpActor vtable2330a8 slot88→1336b0 createShape，NpActor+28交Actor120780→124660；Actor+18正是publicNpActor+40可选Body。124660把公开descriptor local Matrix34转换为内部position/quaternion：动态Body存在时保留local；Body为null静态时先与Actor+48（NpActor+70）矩阵合成world，再转换。不能用动态local规则套静态，或把静态stored world再乘一次Actor。

胶囊公开radius80/full length84作为qword复制至内部78/7c；Scene vtable22ccb8 slot70→9ef30 case3→104b0构造，内部78 radius直接入1b0，7c full length乘原0.5后入1b4 half_segment。共同B0790把内部40/44/48位置写stored88/8c/90，4c/50/54/58 quaternion写78/7c/80/84，不normalize。胶囊另外的b4..c0 backend旋转不覆盖这些stored quat。完整11份Evidence导出/REA正常关闭，六个完整函数体与两个vtable slot逐PE字节SHA核对，未运行原constructor/getter。

新增cover_physx_shape_construction.h纯owned表示桥：显式dynamic_local/static_world，静态Actor pose须调用者认证，动态不消费未使用Actor矩阵；保留原float矩阵合成及trace/最大对角分支四元数转换分组，dynamic trace=(m11+m00)+m22、static=(m22+m11)+m00；静态translation row0 Y+X+Z+T、rows1/2 X+Z+Y+T。sphere radius/box half原样，capsule full length转half，不再乘单位或margin。不normalize/clamp，要求nearest；异常、非rigid、subnormal/半段下溢、unsupported/alias拒绝保持输出。

33041检查0：XYZ各128旋转，两种placement和三种kind，每例四个独立点用Rodrigues及quat cross-product oracle核对local/Actor合成world，另核尺寸、零段、min-normal长度除半成subnormal拒绝、NaN/无穷/负数/非rigid、动态忽略无效unused Actor、unsupported/alias/FP mode拒绝。所有依赖SHA当前一致。此为原静态证据和CPU数学重建，不称全部浮点逐位相同、所有初次load路径已证或实际active形状验收；当前actor/template身份、staticActor当前读取、weld和flags/full self/world fence及生产接入仍待完成。无游戏/XR/生产Native构建/安装，完整CV24/AC22、body/view/hit/muzzle事务及视图遮挡/头显未完成，工程25%。证据shape-construction-r225。

## r226：完整actual asset形状读取接入原QueryEngine／Session／C0接口，隔离Native编译通过（2026-11-12）

不再只留独立离线observer：QueryEngine新增observe_asset_shapes，复用实际固定Image、begin/end source/code/typed guard及active/uncertain/cleanup；调用r217完整asset shape observer，全部关闭后才发布输出。外部heap/TLS AssetShapeWorkspace持有candidate，失败不覆盖旧输出。接口在NoCalls空类型下可编译，原query dispatch不能被实例化；Code::queried在这个接口表示只读事务完成，不是原生查询发生。

Session同一序列化execute事务新增无geometry的AssetReadRequest，角色采集、所有者/metadata/源时效及最后关闭检查保留。大Snapshot作为Session成员，避免在GT回调栈新建大结果。cover_physics_c0_live::observe_asset_shapes通过既有当前Entry/C0/cover/metadata/time guard及query admission，额外拒绝scratch与C0 state/Entry/source/output别名；OFF分支unavailable且不覆盖。只新增接口，没有增加默认触发、原查询、body写入或新移动权限。

离线Engine1723累计0（含原asset1714）：实际QueryEngine/HostImage用原DLL作为PE数据，4个模拟原布局形状，跨actor变化拒绝/不覆盖、重入、别名、正常关闭、原DLL未加载。Session125累计0（原110+新15），实际Session/typed roles加明确空Host packet，仅验owner替换、drain及零query dispatch，空包不当成actual形状证据。OFF52累计0，构造数学33041回归0。首次Session编译暴露公共头pose_math参数actor与旧fixture全局actor的/WX shadow，改参数名actor_pose且数学不变；lambda内if constexpr改直接type trait解决MSVC捕获常量问题，未弱化guard或关闭warning，失败log保留。

完整私有Native在精确r193冻结树上做明确22文件overlay（实际14个changed/new，含此前已证r200 nullable Body修复），原全部GEARS布尔flag保持，独立source/build目录、420个项目依赖逐SHA、无外部工作树混入；GearsCapture.dll SHA50b76bd0f0bec9d518ecbef70a26b381b809741c46764d7d537e9af160abc750。未安装/放到公共入口、未启动游戏/XR、未分派原query。编译不等于C0实际调用或玩法验收。

后续先核实/修正既有eye sphere/path探测入口的game→PhysX单位与一次性触发，再做新的受控真实读取；不因接口编译通过重跑旧计时案例或放宽原来源时间。actual template/shape身份关联、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务、视图遮挡、完整CV24/AC22及头显继续未完成，工程25%。证据asset-shapes-engine-r226，原publicv71和固定手动入口不变。

## r227：旧sphere/path探测补原game→PhysX单位；完整形状一次性读取入口已编译（2026-11-12）

实际C0 measurement入口的eye是game units；旧sphere/path直接传eye及units_per_metre*.01给native ABI，遗漏原F2610固定float .02。本轮新增cover_probe_geometry.h统一转换centre、胶囊两端、radius、delta和path epsilon，仅一次game→PhysX转换；cover_physx_units.h与既有transform共享原常量，transform数学不改。旧三个构造表达式用同测试复现3检查3失败；修复28检查0，含已知正负坐标、50/100单位标定、无效值、溢出/退化胶囊、别名、非nearest浮点模式及失败不覆盖。不是头部/body collider，也不把旧r194原calls0超时归因于单位。

通过已有observe_sphere_once入口新增GEARSVR_COVER_SHAPE_SNAPSHOT/INI cover_shape_snapshot显式一次性只读；sphere/path/shape互斥，shape仅现有simulator_operator、continuous_session和observe-only场景允许，默认关闭。读取调用r226实际C0→Session→QueryEngine全形状接口，原时间/角色/owner/metadata/关闭检查不改；大workspace/packet为静态独占一次性存储，回调不持私有锁读游戏。拒绝也算本次终止不重试；预置detail=request避免尚未进入Engine的拒绝被标成observed。只成功才导出完整shape行、stored pose/dims/world matrix及原始调用计数，保留source/guard失败。未授予movement/world权限。

功能关闭65检查0，查询目录16回归0。首次完整构建暴露cfenv首次在anonymous namespace内引入nested std；改用C浮点控制接口，测试在全局引用cfenv，保留失败build/source/log。v2在r226精确冻结Native上仅5文件overlay，原GEARS flags不变、独立源和构建、无外部工作树混入；完整构建通过，DLL SHA为3972af3f35a5c17c37e463a890fdc68654311e28b52a4f696d1f705b9a11f3b7，项目依赖422逐SHA验证。未安装、未启动游戏/XR，尚无新C0实际形状读取证据。

后续进行新的一次受控真实shape读取，保留原source freshness；继续闭合active/template身份、static/weld/flags/full self/world fence及body/view/hit/muzzle同版事务。完整CV24/AC22、视图遮挡及头显仍未完成，工程25%；publicv71与固定手动入口不变。证据probe-entry-r227。

## r228：新全形状读取真实进入C0／Session后在shape inventory拒绝；正常退出（2026-11-12）

私有新实例cover-1112-r228-cloak-shapes使用r227 Native3972af3f…f3b7，prepare新增显式固定r227选择，逐422个冻结依赖SHA认证；runner新增shape模式且三个环境开关全部明确互斥，保持原输入/owner/启动/资源/静音/0.4/CLOAK/清理保护。生成operator导入通过，实际live身份guard14离线检查通过。准备与runner代码已保存9ce46d24；未修改publicv71/固定手动入口/Quest。

实际游戏42816/creation134389104154093969，XR33080；原A进入掩体、五段eye观察operator0。新读取确实触发：attempted1/finished1，admission6/session_entered1、Session engine stage5，q::Code10 owner_read，asset_shapes::Code7 shapes，14.6558ms；owner guard35/session guard16均无failed_call，source_cause未观察，original started/returned均0。整批失败，bodies/actors/shapes发布数0；不能把0解读成真实空清单，也不能声称完整读取或source-end关闭已成功。当前失败位于asset observer内shapes::observe，尚缺nested shape_code与failed_actor/member；下一步补出已有workspace诊断字段，依具体布局失败修复，不重跑旧timer或放宽来源时效。

268实际投影帧、错误0、14全跨度overview已生成、5阶段contact已逐张亲看；五张双眼自身头肩/躯干遮挡严重，视图验收FAIL。overview未全数亲看，保留full_span_personally_reviewed=false，不称全跨度人工验收。实际captured-span编码0。正常清理true，Native/entry/input/weapon drain恢复、XR/launcher正常退出，自有remaining为空、无强杀；CLOAK639次失败0，前台421次无自有激活，13项anchor来源和cover reader前后相同。

本实例终止后观察到另一个游戏37740/XR49828，未触碰；媒体保留清理因其他活动会话暂缓，仅保存deferred记录。完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务和视图遮挡/头显继续未完成，工程25%。本轮的实际拒绝已缩小下一步范围，证据asset-shape-runtime-r228/runtime-r228。

## r229：针对r228真实拒绝补形状清单分阶段进度，完整Native通过（2026-11-12）

cover_physx_actor_shapes新增Stage/Progress，分别保留first/last两pass实际进行到的读取/检查阶段及member序号；原read调用、顺序、布局、guard及接受/拒绝条件不变。阶段包含actor header/links/slots、Body pointer、backlink、root header/slot/kind、array读取/count、member/internal header/slot、parent/body/public归属及type table、重复和完成。不是新增诊断查询或重试；最终Snapshot只在原双pass关闭成功时发布。

一次性形状summary在finished后输出原workspace已有asset_code/shape_code/world_code、failed_actor/member、两pass progress及first scratch中已读取的actor/root/type/count/arrays；显式partial_unpublished=1，不能把scratch当作成功清单。不在summary内新增game读或原函数分派，保持原私有锁与只读事务边界。

Engine1723累计检查0，含owner519/parameters687/world1688/asset1714与实际QueryEngine/HostImage；原DLL只作为PE数据，无原查询执行。OFF65检查0。独立完整Native在r227冻结树上仅两文件overlay，flags保持，422项目依赖逐SHA、无外部工作树混入，DLL SHA2d252d054acb0c00ee12d9c4fcd0412eea39b2460fd827042bd7d87edcd9df41。该版本尚未游戏运行；下一次新shape观察用于定位r228未导出的nested失败，不能用本次编译声称失败已修。

完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle、视图遮挡和头显仍未完成，工程25%，publicv71/固定入口不变。证据shape-read-detail-r229。

## r230：实际定位actor0/member0的Body归属比较失败；原构造证明d0/148字段不同（2026-11-12）

新实例cover-1112-r230-cloak-shape-detail使用r229 Native2d252d05…df41，准备逐冻结依赖核对、操作导入与身份guard14通过，prepare选择已保存27631665。实际game40660/creation134389109050030515，原靠近/进入掩体及五阶段eye操作operator0，shape模式仅一次；11.7553ms后q10 owner_read/asset7 shapes，nested asset0/shape5 links/world0，failed_actor0、member进度0、first Stage15 body_link、last未进入。第一个单形状actor type3/count1。原owner guard35/session guard16无失败，original calls0；未发布整批结果，不把partial scratch当有效形状清单。

由此针对body比较查保存的原构造：124660以local_1a8作descriptor，descriptor+8来自Actor+18（NpActor+40 optional Body），descriptor+20来自Actor+8（NpActor+30 public-owner backlink）；1336b0将NpActor+28传入Actor创建。b0790拷descriptor+20至Shape+d0、descriptor+8至Shape+148。现有reader错误把Shape+d0和NpActor+40 Body比较。证据沿用r225完整原PE字节认证的三个function，完整pseudocode/assembly及原bundle SHA保存在original-owner-construction.json；这是静态来源推导，当前游戏仅导出失败阶段，没有导出实际d0值，二者明确分开。后续以原构造字段单独replay反例并修正两条归属，不删除比较。

269投影/errors0、14总览、5实际阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未亲看全部overview，full_span人工验收false。编码0，正常清理true、owned remaining为空、无强杀，CLOAK589失败0，前台386无owned激活；anchor/cover reader前后相同。完整CV24/AC22与同版body/view/hit/muzzle、full self/world fence/头显仍未完成，工程25%，publicv71/Quest/固定入口不变。证据shape-stage-runtime-r230。

## r231：修正Shape公开Actor／optional Body字段混淆，同counter旧6失败→新0（2026-11-12）

按r230实际Stage15与原124660→b0790构造证据，新增独立cover_physx_shape_owner_fields_test.cpp，明确原Shape+d0=public Actor、+148=optional Body、NpActor+40=Body；不从被测reader推expected字段。相同counter源码/实际原DLL仅作为PE数据：旧header9检查6失败，既错误拒绝正确dynamic/static，也错误接受把public owner字段替换成Body。旧两个product header与旧依赖/log保留；修复后同9检查0。

actor_shapes与shape_owner两处同时修正：d0独立等于当前public Actor，148独立等于NpActor+40 optional Body，最小内部shape pointer范围扩至150。保留已有public↔internal shape、actor backlink、scene、type code、membership双pass、owner/source/关闭比较；不是删除Body校验或将错归属当non-self。原Body为空static路径仍受两条分别资格检查。下游world的internal_actor仍指optional Body，语义不改。三个旧synthetic fixture改为原构造字段，counter自身源码不变。

Engine1741累计0（owner531、parameter702、world1706、asset1732），新增read引出额外故障点逐一拒绝；actor inventory79、Session125、OFF65均0。原DLL未加载/未分派query，Session空Host packet仅作owner事务回归。完整Native以r229冻结树仅两个product header overlay，flags不变，422依赖逐SHA无外部混入；DLL SHA3ca85b2f8642d8a3ef9e58f030ca58be18c94df586c24607c71d3ce36b157471。未安装/未游戏运行此修复，下一次新shape模式须验证实际失败能否越过此点，不提前称整批成功。

完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务、视图遮挡与头显仍未完成，工程25%；publicv71/固定手动入口/Quest不变。证据shape-owner-fields-r231。

## r232：修正后的Actor/Body归属实际通过；下一拒绝转至world pose（2026-11-12）

新cover-1112-r232-cloak-owner-fields使用r231 Native3ca85b2f…7471；准备422冻结依赖、生成操作导入/身份guard14通过，选择代码ade0214b。实际game39576/creation134389114448133788，原进入掩体和五段eye操作operator0。一次全形状读取19.8285ms，原owner guard47/session guard22无失败，original calls0。actor0/member0的inventory first/last Stage19 complete均成功，原Stage15 body_link拒绝不再出现；nested asset0/shape0/world8 pose，q10 owner_read、asset detail8 world。失败输出整批仍未发布，不能声称整批成功或world/body闭合。

这给r231字段修复取得实际越过旧失败的证据；后续world pose在参数四元数、Body四元数或compose何处拒绝尚未知，补已有raw输入与阶段记录，不提前放宽rigid/normal阈值或归一化原数据。

264实际投影帧/errors0、14总览、5阶段contact逐张亲看仍自身头肩/躯干严重遮挡FAIL；未全数亲看overview，full_span人工验收false，编码0。正常清理true、自有remaining为空、无强杀，CLOAK620失败0、前台412无owned激活，anchor和cover reader前后相同。完整CV24/AC22、full self/world fence、active/template、body/view/hit/muzzle及视图遮挡/头显继续未完成，工程25%，publicv71/固定入口/Quest不变。证据shape-owner-runtime-r232。

## r233：world pose拒绝保留已有原始输入与阶段，数学规则不变（2026-11-12）

针对r232 actual world::pose，world Workspace增加first/last Diagnostic，分参数/shape slot/stored pose/body header/class/values/body pose/compose/complete；保留已有读取的shape stored位置/四元数、Body table/位置/四元数。只在owned scratch记录，仍仅原全部关闭成功发布正式Snapshot；summary在一次finished后输出明确pose_unpublished=1，不增加game读、不执行原getter，不改scalar/normal/rigid/norm阈值或归一化。

新增两处定向检查分别损坏stored与Body的w为2，验证拒绝、正式输出保持、正确阶段以及精确原输入在scratch保留。Engine1743累计0（world1708/asset1734），原owner read_budget228、parameter_reads102、world_reads140保持，OFF65通过；说明诊断没有新增memory reads。完整Native在r231冻结树仅world与C0两个文件overlay，422依赖逐SHA、flags不变、无外部工作树混入，DLL SHA63888223462c41302676357c5303742fc59f205acdd3c92a5e58619a65cf2caa。此版尚无游戏内pose输入证据，下一次明确新版本读取后再判断具体布局/数学问题。

完整CV24/AC22、full self/world fence、active/template、body/view/hit/muzzle、视图遮挡与头显仍未完成，工程25%，publicv71/固定手动入口/Quest不变。证据shape-pose-detail-r233。

## r234：实际world Stage7 Body quaternion；原数据近单位但严格矩阵判定拒绝（2026-11-12）

新cover-1112-r234-cloak-pose-detail使用r233 Native63888223…2caa，准备/操作导入/身份guard14通过，准备702f44e7。actual game25596/creation134389118562337067；原进入掩体与五段eye操作operator0。一次全形状读取14.9296ms，actor0/member0 inventory两pass完成，world first Stage7 Body pose、parameters0成功，原query调用0、owner guard47/session guard22无失败，整批未发布。Body q=.559741497,-.394303977,.607427657,.402782083，p=-78.8432846,-3.34397388,11.5933132；stored q=0,.707106769,.707106769,0，p=.0599999949,-6.82121011e-15,-2.13162816e-16，均为9位round-trip float原值，不是归一化结果。second world pass尚未进入，不把其零值当数据。

原Body q norm平方0.9999879342658389，在已有3e-5 norm范围；按原1209f0逐float算式重建后Gram最大偏差3.289429316311043e-5、Frobenius5.71721064577549e-5、det0.9999596083569477。故现有四元数norm检查通过而严格矩阵3e-5检查拒绝。原getter不归一化。后续用实际数据replay，再以显式原矩阵误差记录区分近旋转读取和严格rigid query权限，不能只提高常量或偷偷归一化。

259实际投影/errors0，13总览已生成、5阶段contact逐张亲看仍双眼自身头肩/躯干遮挡FAIL，未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK568失败0、前台380无owned激活，anchor/reader前后相同。其他游戏/XR会话未触碰。完整CV24/AC22、active/template/full self/world fence/body/view/hit/muzzle/视图遮挡及头显仍未完成，工程25%，publicv71/固定入口/Quest不变。证据shape-pose-runtime-r234，算式证据另见observed-pose-r235/original-pose-arithmetic.json。

## r235：按原getter保留未归一化观测矩阵及显式Gram误差；实际值replay通过（2026-11-12）

新增cover_physx_observed_pose.h，按原1209f0 float分组构造矩阵，不归一化、不clamp；已有quaternion norm容差3e-5保持。Observed Pose明确携带R^T R-I的Frobenius上界（亦为谱范数上界），double累加/sqrt舍入另加保守margin。原理：精确四元数矩阵Gram特征值1和1+4*(norm平方-1)*|xyz|平方，再计逐float算式operator rounding界；用此导出单Pose资格范围与compose传播界，不将近旋转冒充严格rigid。source/owner/metadata/时间/双pass/关闭guard不改。

world snapshot保留原actor/shape矩阵及各自gram_error，same也比较误差；summary成功行输出两误差并明确rigid_query_permission=0。既有pose_math::rigid、quaternion、compose及shape_geometry/query入口严格判断均不改。新的读取结果仍须后续带数值包络进入几何/查询事务；没有因为读到矩阵授予碰撞/移动权限。

以r234原float记录建立相同replay，旧代码3检查1失败，新代码5检查0（新增成功后断言被执行）：保留raw q/p逐float相同、原矩阵仍不满足strict rigid，不加载原DLL。独立math2060检查0：256组norm边缘±2.9e-5，以未归一化Rodrigues点旋转oracle核对两级变换，显式误差界、超norm/零/NaN或Inf/denormal、反射/低报误差、非nearest舍入拒绝及输出保持；实际原矩阵继续被严格geometry拒绝。Engine1743、Session125、OFF65均0，内存读取预算228/102/140不变。独立Python每op float-round原式及原getter evidence/PE来源SHA保存original-pose-arithmetic.json，原数据norm/Gram/det数值见r234。

完整Native在r233冻结树仅world/observed-pose/C0三文件overlay，423项目依赖逐SHA、flags不变、无外部工作树混入；DLL SHAc9ea318097be628ab96ac48b4c97f9ab6b545e91047bee3b7d8a5e03f4059e25。此修复尚未游戏运行，下一步新版本一次形状读取验证是否越过Body pose，再闭合全清单与非严格rigid数值包络；不称actual asset通过。

完整CV24/AC22、active/template/full self/world fence、body/view/hit/muzzle、视图遮挡及头显仍未完成，工程25%；publicv71/固定手动入口/Quest不变。证据observed-pose-r235。

## r236：观测近旋转实际越过旧pose失败，actor3时原source deadline拒绝（2026-11-12）

新cover-1112-r236-cloak-observed-pose使用r235 Nativec9ea3180…9e25，准备423依赖/导入/身份guard14通过，准备c4fae7f2。actual game53596（精确身份见原run-result），一次形状读取34.6018ms；原Body pose失败已越过，后在actor index3的shape guard拒绝。owner147/failed147 Gate8 context/detail3 source_age_expired/extra6，Session72/failed72 owner_begin；source cause1，original calls0、整批未发布。当前world scratch两pass Stage9 complete，不能把这些局部结果当全asset通过。实际game身份与source原始值以run-result/native-session-status为准。

264实际投影/errors0，14总览已生成、5阶段contact逐张亲看仍自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK552失败0，前台372无owned激活，anchor/reader前后相同。未触碰其他会话。新失败指向同一只读事务的重复全owner开销；保留原source时间，不以更新时间或放宽期限修复。

## r237：无分派的完整asset读取复用同步只读分组，全owner138→6（2026-11-12）

Session只在asset Snapshot类型的engine执行分支复用既有read_source_group。整个完整资产observer是一个同步READ-ONLY、无原query分派的操作；每一次typed SourceGuard仍调用原roles::recheck、两遍dynamic values、全部prior/final Proof与初始结果比较。分组内部的owner变成已有read_current（相同线程、enabled/busy、scene/key/special move/metadata）；分组开始/结束仍执行原完整owner/cover/pose/原时效，execute closing还复核一遍。私有member result只有全部关闭通过才写外部输出；分组失败清除recheck资格。未将权限/缓存跨execute保留，未修改实际query分支、原时间戳或时效限额。

同一Session fixture用64个重复typed SourceGuard调用、相同源码前后比较：原full owner138次，新6次；old825/new165均失败0，检查数差异来自逐个full owner拒绝边界138→6，原110基础项保留。中途expired即使Host已写私有结果仍不发布，逐个现有full boundary拒绝、root替换、drain及无原调用通过。Host结果明确为空，仅验真实Session与typed role事务，不冒充游戏全形状。Engine1743与OFF65通过。首次MSVC编译拒绝nested lambda局部constexpr判定，改直接Output type trait，保留失败log，未改guard。

完整Native在r235冻结树仅cover_physics_query_live.inl单文件overlay，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHAc5e3d0f2f4d00de93c7db48b6626e36d35f5eaeb085c40801e84f610269e73d8。此版尚未游戏运行，下一步明确新版本一次asset读取，验证真实耗时及全清单能否在原deadline内关闭；不因离线full调用减少声称时效问题已修。

r236/r237均不改变publicv71/固定入口/Quest。完整CV24/AC22、active/template/full self/world fence、非strict rigid数值包络、body/view/hit/muzzle、视图遮挡与头显继续未完成，工程25%。证据observed-pose-runtime-r236、asset-read-bracket-r237。

## r238：只读分组实际推进至actor11，21.1594ms后旧norm cutoff拒绝（2026-11-12）

新cover-1112-r238-cloak-read-group使用r237 Nativec5e3d0f2…73d8，准备423依赖/导入/身份guard14通过，准备c10a13a1；actual game34060/creation134389133967750033。一次读取至actor index11/member0，21.1594ms，owner4无失败、typed Session guard250无失败，q10 owner_read/asset8 world/world8 pose、first world Stage7 Body pose、parameters0，原query调用0。尚未group-end或全asset关闭，不能把未观察到deadline拒绝称为最终时效通过。

Body q=.401522607,.304909706,-.0472320467,.862281024，norm平方.9999497638780792；stored q=-.517145097,-.482245654,-.482245654,.517145157，norm平方.9999999062283447。旧观测器沿用3e-5 unit cutoff在Body处拒绝，未发布整批。此数据说明不能逐个把常量调大来取得全清单；须使原数据读取与严格query资格分开。

259实际投影/errors0、13总览、5阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK545失败0、前台367无owned激活，anchor/reader前后相同。证据asset-bracket-runtime-r238。

## r239：观测矩阵资格与严格几何资格分离，不逐次调大unit容差（2026-11-12）

observed_pose保留finite/nonzero原q、原float getter矩阵、可表示运算范围及完整Gram误差，不再以unit quaternion norm固定3e-5截断只读观测。资格改为矩阵有限、det>0及Gram谱误差上界G<1；此条件给出奇异值sqrt(1-G)..sqrt(1+G)和非退化proper polar rotation，是显式可传递数值性质，不是查询精度预算。compose继续传播并检查误差和proper/nonsingular条件，不归一化或clamp。新增float中间量/translation求和的double范围预检，有限极端输入在运算前拒绝，未先触发float overflow。

原pose_math的3e-5 unit/rigid规则、原query ABI资格保持。strict shape_geometry现在额外从raw stored/Body q+p重建严格姿态，检查Body presence与owner pointer一致，再要求派生world矩阵逐字节匹配；即使非unit q.w=2/xyz0的原getter矩阵为identity，也不能直接进入strict geometry。旧owned-only几何fixture补齐其原raw q/p，使严格资格有可追溯数据。该库不授予world/movement权限，非strict原数据后续仍须数值包络和实际query事务。

相同r238记录replay旧3检查1失败，新5检查0，raw q/p与误差保留、strict geometry仍拒绝。Math4369检查0：包含norm范围±.1与近unit的未归一化Rodrigues点oracle、矩阵资格/误差、反射/退化/下报误差、非nearest、零/NaN/Inf/denormal、极端有限值与overflow flag、identity但非unit的严格gate拒绝。Geometry13840、Engine1743、Session165、OFF65均0；Session full owner6/64重复typed checks保持。源SHA逐一当前。

最终完整Native v3在r237冻结树overlay observed_pose/shape_geometry，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHA828e7e2400be8456b1ff7de0450480e9b75b7e4ee2ab2b8164fcfec7601682c6；geometry是否Native实际依赖=False，其严格转换通过独立模块测试，仍不称已接入玩法查询。中间v2构建保留；v3包含追加的可表示范围预检。此版尚未游戏运行，下一步新版本一次读取，检查全asset和原deadline关闭。

完整CV24/AC22、active/template/full self/world fence、非strict数值包络、body/view/hit/muzzle、视图遮挡及头显继续未完成，工程25%，publicv71/固定入口/Quest不变。证据pose-read-contract-r239。

## r240：内部完整清单两遍通过，98.8438ms后全owner关闭因原时效拒绝（2026-11-12）

新cover-1112-r240-cloak-pose-contract使用r239 Native828e7e24…82c6，准备/导入/身份guard14通过，准备fc4d6e25；actual game52092/creation134389146201766454。完整asset observer detail0，failed_actor/member均unset，最后shape/world两pass完成，内部清单及姿态已全部比较一致；但整个事务98.8438ms，原full owner5/failed5 context source_age_expired extra6，Session1172/failed1172 owner_end，q4 source拒绝。原query调用0、正式bodies/actors/shapes0，不能把内部成功称作整批已发布或时效通过。

266实际投影/errors0、14总览生成、5阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，精确CLOAK/前台计数见closeout，anchor/reader前后相同。证据pose-contract-runtime-r240。

## r241：同一无分派只读事务借用入口字段证明，完整关闭前重新校验（2026-11-12）

实际r240的1172个SourceGuard重复完整角色字段回读是主要剩余工作量；本轮只对asset Snapshot的同步NO-DISPATCH分组新增AssetReadGuard。初始roles::sample仍两pass证明全部类/字段/引用/数组/body动态值；组内每个guard确认read-ready、当前metadata cache/generation、相同线程/busy/context/key/metadata，并用原matches_query_source/matches_asset_actors核对每个原生来源快照。原生Source/asset/actor/member/shape/pose双pass与全部代码/归属闭合保持。完整typed roles::recheck移到最终SourceGuard关闭（两遍值、初始比较、完整前后Proof）；原full owner/cover/pose/source时间在分组前后和关闭仍执行。只有全部关闭才发布私有结果，没有跨execute的证明借用，也不将此guard用于真实query分派。

Session202检查0：从原空Host packet推进到实际asset actor observer读取合成当前typed内存中的2body/2actor，错误raw actor packet拒绝；原始shape/world/original模块执行仍是Host seam，无这些玩法证据。初始sample之后完整physics_rechecks=1，64重复typed来源匹配、full owner6；每个full拒绝、expired即使私有结果已写仍不发布、root替换/drain以及六种持久修改（BodyIndex/OwnerComponent/BoneSpring、属性offset/width/reference class）均拒绝输出保持。初测两失败是mutation写到fixture未使用位置，依据既有property helper改正确94/d0，仅改测试目标，失败log/result及说明保留，不当成产品修复。Engine1743、OFF65通过。

C0 summary新增完整回读/借用guard计数，候选计数明确requires_close，成功时额外导出Body实例ordinal→唯一Actor index及地址/scene/internal映射；只读取已完成owned scratch，不增加game读。完整Native在r239最终冻结树仅Session/C0两文件overlay，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHA27adaac79c8ea6e5a17303ed1f678b339ff13e5f3247b690f19428c87418d95d。此版尚未游戏运行，下一步新实例检查完整读取、原deadline关闭和真实proof计数。

完整CV24/AC22、active/template/full self/world fence、非strict数值包络、body/view/hit/muzzle、视图遮挡及头显仍未完成，工程25%，publicv71/固定入口/Quest不变。证据asset-proof-scope-r241。

## r242：实际完整PhysicsAsset 29刚体／36形状在5.7533ms内关闭并发布（2026-11-12）

新cover-1112-r242-cloak-proof-scope使用r241 Native27adaac7…d95d，准备423冻结依赖/导入/身份guard14通过，准备0faa9723；actual game39160/creation134389157760080182。一次C0读取成功：q0 queried（本接口表示读取事务完成，不是原query调用）、detail0、Session stage7 complete；29 Body instance、29唯一PhysX Actor、36 shape，全部同源完成并导出。5.7533ms，owner7无失败、Session1173无失败，完整physics_rechecks1、borrowed guards1172，source cause未观察到拒绝。原source时间/生命周期/最终typed闭合和时效保持，original started/returned均0；movement_permission0。

导出29个Body ordinal→唯一Actor映射及36完整行；实际21capsule、13box、2sphere。解析后逐项核对原始状态文本、每个float按binary32回读、body/actor/internal归属、唯一shape/internal地址、actor内member ordinal连续、统一scene/module base、数量与源SHA。36个world矩阵以Decimal120独立计算Gram Frobenius与det，均proper且所报G覆盖高精度值；最大shape G=.00033565757003262825（无量纲）。actual-asset-packet.json/packet-verification.json与可复跑verify_packet.py保留。数据仅当前角色supplied PhysicsAsset；不把附属component、full self、flags参与规则、世界fence或同版body/view/hit/muzzle称为已完成，历史数据不能授权实时移动。

264实际投影/errors0、14总览生成、5阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未全数亲看overview，且image与此C0数据未证明同一时刻，不作手/身体接触拟合接受。编码0，原五段操作operator0，正常清理true、无强杀、自有remaining为空；CLOAK575失败0、前台376无owned激活，anchor/reader前后相同。

这次关闭只读数据链路。下一步将完整原始姿态的数值范围接入几何/场景查询，再接实际仍明确拒绝cover的body goal事务分支；其视图/碰撞/受击/枪口必须按原需求同版提交，不能用读取成功替代。完整CV24/AC22及头显继续未完成，工程25%。publicv71、固定入口与Quest不变。证据asset-scope-runtime-r242。

## r243：原始观测形状的数学保守AABB与36实际行独立核对（2026-11-12）

新增cover_physx_observed_bounds.h，不改变严格rigid转换器，也不归一化原q。box逐行取原始矩阵列与half的绝对乘积和；capsule取原矩阵Y列乘half_segment的线段再加原未缩放半径；sphere保持原半径。double乘积对binary32输入精确，每次累加和最终端点向外nextafter，避免相消导致范围缩小。仅数学几何AABB：不是PhysX窄相位浮点执行误差证明，不提供query姿态、scene fence、freshness或移动许可。

owned Snapshot适配器重新执行stored/body原getter与composition，逐字节匹配world/actor矩阵与各自Gram记录；body存在标记与internal_actor一致，失败不写output。另有明确只接受供给几何的primitive入口用于历史数据，不能认证来源。非法尺寸/非finite/反射或过大Gram/错误rounding/alias拒绝。34004检查0失败，MSVC W4/WX编译0；独立角点/线段端点枚举覆盖不同转轴、近单位q、巨大平移和闭合篡改。MSVC long double与double同精度，不能把该枚举称为高精度证明。

r242原始packet SHA0cb97269…c445，36行全部通过新增C++ primitive后，另用Python Decimal160枚举每个box角点、capsule线段加原半径及sphere极点；全部精确数学极值处于输出上下界内，逐轴slack和原输出SHA保留packet-certificates.json。历史packet缺少每行body原q/p，故该回放只检验primitive入口，未伪称通过owned Snapshot复核。新增头尚未接入Native编译/实时集合，没有新游戏或XR运行，工程25%、CV24/AC22与视图遮挡继续未完成。证据observed-bounds-r243。

## r244：完整asset范围集合全有或全无，接入Native只读输出（2026-11-12）

新增cover_physx_asset_bounds.h：校验body→actor映射（共享引用显式保留、null BodyData允许）、每个actor必须被引用、唯一actor、连续完整member组、row actor/member ordinal、shape/internal唯一和member归属。容量body/actor64、per-actor shape64、总shape256；逐行执行r243 owned Snapshot原getter/Gram复核，任何一项失败保留调用者旧output；全部成功才一次提交rows/aggregate，空集合has_shapes=false，没有虚构原点碰撞体。

AssetBounds2391检查0，覆盖64actor/256shape上限、空actor组、共享/null、数量超限、漏项/重复/错误归属、末行矩阵/半径失败与alias。现有QueryEngine供给的实际observer四形状合成内存packet关闭后接新转换器通过，Engine1744；OFF65通过。这里Engine执行的是实际QueryEngine与原始PE只读数据、合成对象内存，非游戏形状证据。

C0仅在完整observe_asset_shapes返回queried之后，才对owned packet计算整组范围；单独记录bounds begin/end与code，明确derived_after_close=1、mathematical_only=1、query/movement_permission=0。失败不输出任意bounds row；成功逐行带actor/member/shape保留映射。此后计算不会续原source时钟、复活借用proof、分派原query或授予移动权限。原关闭guard/借用范围保持。Native基于r241最终冻结树仅C0及两个新头overlay，425编译依赖逐SHA、flags不变、无外部工作树依赖，DLL SHA66a96c12d55de850daf87dc7a203a29d86d29367ff6619214c5dcf114e5097f4。尚未实际游戏运行/安装；下一步新实例验证36实际形状能否整组输出及计算耗时。

数学AABB不是PhysX窄相位浮点误差界，也不是world fence/full player self；模板对应、实时query/body/view/hit/muzzle与自身遮挡仍未完成。CV24/AC22与头显不宣称通过，工程25%。publicv71、固定入口、Quest保持。证据asset-bounds-r244。

## r245：实际36形状范围全组成功，计算0.026ms，视图仍遮挡（2026-11-12）

新cover-1112-r245-cloak-asset-bounds，准备7547f879、425冻结依赖/导入/14身份检查通过，使用r244 Native66a96c12…97f4；actual game3952/creation134389174615272113。完整实际读取29 Body/29唯一Actor/36 shape（21capsule/13box/2sphere）后，owned Snapshot范围集合code0、36行全数输出。C0观察总5.5001ms，其中完整关闭后的数学范围派生0.026ms；owner7/Session1173无失败，完整typed recheck1、组内guard1172，原query started/returned均0。每行actor/member/shape与raw geometry逐一对应；Decimal160再次独立枚举36形状的数学极值，全部被实际模块输出bounds包住。原时间/proof不续期，输出明确derived_after_close=1/mathematical_only=1/query_permission0/movement_permission0，未称世界query或移动通过。

264实际投影/errors0、14总览生成、5contact逐张亲看：两眼右侧自身肩甲/躯干遮挡，右眼尤其严重，转头/抬头与恢复均仍FAIL；不能把较多环境可见解释为修复。全overview未亲看、图片与此C0未证明同一时刻，头显与拟合均不接受。编码0/operator0，正常清理true、自有remaining为空，CLOAK560失败0/前台379无owned激活，anchor/reader未变。原始run/native文本、可复跑extract.py、actual-bounds-packet.json与每轴Decimal极值保留。

这轮将完整实际形状的数学范围接入原只读路径并验证，不扩大到full self/world fence/原query/同版body-view-hit-muzzle提交。接下来应处理实际场景查询与掩体body goal仍拒绝cover的缺口，不能重复相同读取代替玩法推进。完整CV24/AC22、工程25%、publicv71/固定入口/Quest保持。证据asset-bounds-runtime-r245。

## r246：单位修正后的原版sphere查询实际调用返回，5.3198ms（2026-11-12）

新cover-1112-r246-cloak-sphere-units，准备8b0d2c4b、425冻结依赖/导入/14身份检查通过，r244 Native66a96c12…97f4；actual game44992/creation134389177751536075。这次切换到sphere动作，验证r227单位修正后未实际覆盖的原查询链；不是重跑相同asset读取。C0 query code0、Session stage7 complete、engine stage10 complete、source0/exception0；原始调用started0→1/returned0→1，5.3198ms完整关闭，raw_hit1。sphere native中心[-78.6054764,-3.88316274,12.4197283]、radius.0199999996（游戏1厘米按每米单位与.02原物理比例换算）。半径只诊断调用/ABI，不是头部实际碰撞体。

raw_hit1未区分自身与世界，不能称墙体命中/安全移动/occupancy或世界fence通过。普通query仍执行完整typed/source/owner前后关闭，未使用r241仅限asset无分派读取的借用guard。下一步新实例验证带命中对象列表的capsule path，核对自身归属、路径与终点；不能以sphere布尔结果授权body写入。

264实际投影/errors0、14总览/5contact生成、编码0，但本轮未逐张亲看，personally_reviewed_contacts空、full_span/visual/headset接受均false；不把生成图片当作视觉验证。operator0、正常清理true、自有remaining为空、anchor/reader未变。原始run/native文本、extract.py及closeout保留。CV24/AC22、完整身体碰撞/模板/full self/world fence/body-view-hit-muzzle和已知遮挡仍未完成，工程25%；publicv71/固定入口/Quest保持。证据sphere-units-runtime-r246。

## r247：路径观察在原A前候选关闭因registry新增一项而拒绝，未到query（2026-11-12）

新cover-1112-r247-cloak-path-units，准备1f3969ef，r244 Native66a96c12…97f4。原ordinary approach在候选CoverLink读取q.finish时触发原严格World-state metadata/registry changed at closing boundary。已有r180拒绝见证保存确切差异：registry地址5398218544，data372178944/capacity225272保持，count218210→218211。不是identity/字段变更，也不把新增一项当作允许旧样本通过；原读取正确拒绝，未调用原A候选操作与物理path。path attempted0、原query0/0，不能记作path query失败或原source时效失败。

operator1、133实际投影/errors0，正常Native/entry/XR/game清理true、自有remaining空；anchor/reader未变。原run/native、候选拒绝JSON、operator traceback、recording清单完整保留；本轮未亲看帧/未编码，不作完整视觉验证。下一步仅一次新实例完整新鲜读取复核该前置阶段，保持所有registry/identity/metadata/值关闭条件，不开启append-only放宽、不复活旧样本或重复未知原调用。若再失败则围绕新确切拒绝处理，禁止无变化循环。

该前置拒绝不撤销r246实际sphere查询通过，也不证明full self/world/body/view。工程25%，CV24/AC22及头显仍未完成。证据path-units-runtime-r247。

## r248：新鲜严格候选通过，path首次原调用返回后在36.7614ms因时效拒绝（2026-11-12）

仅一次新实例cover-1112-r248-cloak-path-fresh复核r247明确的一项registry新增，准备08bc6e36、r244 Native66a96c12…97f4，actual game49776/creation134389182322303407。未修改产品/原reader/任何关闭条件，原候选选择与五段ordinary观察operator0。此时实际到达path：attempted/finished1、q4 source、Session stage5 engine/detail4、engine stage9 guard_end/source_code0/exception0。原调用started0→1/returned0→1；不是原调用故障，也不是r247未尝试状态。36.7614ms后完整owner91/failed91 context source_age_expired extra6，Session guard45/failed45 owner_end，source_age_cause observed1/verdict3/causes1。hits/segments/endpoint均0是未发布，不是无碰撞；原调用一次返回之后不自动重试。

timing：physics initial.4819ms、collider initial1.7541ms、components initial2.4775ms，physics repeat42次合3.4007ms、collider repeat42次合5.7734ms、components repeat2次合4.7551ms；engine31.1325ms；owner brackets86次合17.0784ms（嵌套项目不相加当总耗时）。首次与最后source-age窗口保留原tick/metadata/tracking时间，明确是原年龄过期，不抬deadline或续钟。此前sphere原调用1/1已完成全关闭；现在缺口具体缩到带owner集合的读重复与关闭时效。

266实际投影/errors0，记录清单保存但未逐张亲看/未编码，视觉接受false；正常Native/entry/XR/game清理true，自有remaining空，anchor/reader未变。下一步对QueryEngine纯读sources及owner归属的同步分组减少重复完整角色证明；borrow只限无dispatch组内，分组前后完整typed证明与原owner/cover/pose/原时效必须保持，所有原调用边界继续用普通完整guard。不能将r241 asset-only借用guard直接包住原query。优先复用现有组内读取/最终关闭结构及fault/mutation测试，修复后一次新实例定向检查。

CV24/AC22、完整身体几何/full self/world fence/body-view-hit-muzzle/遮挡和头显仍未完成，工程25%；publicv71/固定入口/Quest保持。证据path-fresh-runtime-r248。

## r249：原query之外的同步sources/owner读取分组，完整关闭与调用guard保持（2026-11-12）

针对r248实际36.7614ms/42次physics+collider重复/91次owner的关闭年龄拒绝，QueryEngine collect_all只在两个明确无dispatch的段落新增可选read_group：原调用之前asset/collider/components读取，及返回后shape-owner归属与asset/collider/components关闭读取。普通begin/end、原调用、callback均在分组之外；没有将asset专用借用guard包住query。无此方法的旧caller保留原行为，所有原始raw/typed匹配、原native两遍读取/代码/归属关闭保持。

SourceGuard的组入口与出口均执行完整physics/collider typed recheck和原owner/cover/pose/time，另完整component recheck关闭；内部read_source_group仍由完整owner前后括住。QueryReadGuard只在同步组内匹配入口typed的query-source/asset/collider/components，并每次检查stopped、三workspace read-ready及metadata/generation/cache相同、同线程/Session busy/context/key/special-move/metadata。关闭失败不发布、不会将proof跨越原调用、不续原source时间、不延长deadline。所有派生guard在原调用前已销毁；C0新增完整recheck/借用read guard计数供下一实际运行核对。

QueryReadGroups1046检查0：真实Session+typed/raw asset/collider/component读者，组内64重复匹配，正常Host dispatch1；78个完整owner逐个失败、BodyIndex/OwnerComponent/属性offset、component owner/member、collider Body变化，组内过期/metadata generation变化/drain均保持旧output；组内失败时Host调用0，模拟原调用中root变化仍由普通guard拒绝。ReadGroupEngine44检查0：实际QueryEngine collect_all分别sweep与overlap枚举两read组，Host原调用仅发生在组外；第一/第二组拒绝分别原调用0/1，均无结果发布/重派/悬空owner。原OwnedSweep146、Overlap87、CapsulePathSession391、AssetShapeSession202、AssetEngine1744、OFF65通过。

测试初遇OwnedSweep旧夹具仍将Shape+d0写成Body且不填+148，与r231已经由原constructor/实际读取证明的public Actor/+148 optional Body相反。以r244原q头与本轮q头分别编译同一旧夹具，两者均22失败、输出完全一致；只修夹具d0/148后146通过，新分组Engine原2个失败亦随之通过，未为此改产品。原before-owned三源码/初始build与test logs保留。另初次编译W4/WX拒绝lambda名owner_reads与测试global同名，改局部名owner_read_group，保留初始build失败证据。

Native冻结r244最终树仅C0/Session/QueryEngine三文件overlay；425实际依赖逐SHA、flags不变、无外部工作树，DLL SHA78460f418dd097eaf5cb427d73191b0445781ddcdd6558144c47bdb7deae9cf3。仅编译，尚未实际运行或安装。下一步新实例定向复核路径原调用前后全部关闭、原时效与实际recheck计数；不声称已有完整路径/世界fence/body-view-hit-muzzle或遮挡修复。工程25%、CV24/AC22与头显未完成，publicv71/固定入口/Quest保持。证据query-read-groups-r249。
