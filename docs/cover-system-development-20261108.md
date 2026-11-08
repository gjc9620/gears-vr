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
