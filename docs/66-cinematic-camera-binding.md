# 实时过场摄像机与流式子关卡

2026-10-15。GameRecovery9 实验路线；默认玩家入口仍为已冻结的 GameRecovery8，XR 继续使用 SpaceRecovery3。游戏分辨率和 DLSS/DLAA 保护不变，没有增加渲染 Hook。

## 已确认的限制与修正

原锚点校验要求 `Camera.ViewTarget.Target == Controller.Pawn`。实际开场会保留受控 Pawn，却将 ViewTarget 换成 CameraActor 并设置 `bCinematicMode`。这条有效路线会被原校验拒绝；它是实时过场无法继续提交双目的一项已确认限制，不是所有黑屏的统一根因。

新开关 `cinematic_camera` 默认关闭，要求启用 anchor observer，与实验性 Pawn 原点替换互斥。Python 生成 schema2 证书，原生校验后才接受以下路线：

- Pawn 的注册身份、同类同外层重绑定、控制器双向关系与本地玩家关系保持原要求；UI 始终使用真实受控 Pawn。
- 非 Pawn 目标必须是完整注册、非默认对象、精确 CameraActor 类，且经过验证的 `bCinematicMode` 位为真。布尔语义的汇编和运行字节证据见文档65。
- 目标的直接 outer 必须是注册的 Level，类与人物 Level 相同。人物、控制器和目标的反射 `Actor.WorldInfo` 必须指向同一已认证 WorldInfo，且该对象 outer 为人物的 Level。
- 目标、目标 Level 和实际剧情位随 scene/family/renderer 关联凭据传递并比较；它们不能混入另一帧或另一条场景调用。首尾复读仍是非原子一致性检查，不声称具有引擎对象序列号或绝对 ABA 证明。
- 继续使用引擎实际生成的场景视图构建左右眼，不将剧情摄像机替换成 Pawn 眼高位置。

最初的“同 Level”条件被实际证据否定：PID24656 中人物位于 `WarGame_P.PersistentLevel`，剧情 CameraActor 位于 `SP_Embry_Scripting_02.PersistentLevel`，其 World/Package outer 不同，但二者及 WarPC 共享同一 WorldInfo。读取的是当前运行的反射对象属性，offset208、ArrayDim1、ElementSize8；代码使用反射证书，不将208硬编码成通用引擎布局。相关身份、模块基址及原始观测保存在 ignored `analysis/runtime/cinematic-level9.json` 与 `cinematic-worldinfo9.json`。

## 本地验证与尚未完成项

24个独立 native 检查程序通过。新增 Python 故障注入覆盖跨世界、目标/Level 身份、反射布局、调用入口/完整代码哈希、剧情位与读取竞态；旧 schema1 路径保留。生命周期测试原先将八个64帧池同时放在默认 Windows 栈上，扩展帧证书后发生栈溢出；测试存储改为堆分配，生产存储原本为全局对象。

一次实际准备期间五次采样只有相机时间戳变化、身份未变，却因通用错误文本终止启动。现在只有失败证据逐项确认是时间戳/有效注册表增长的情况，才在已有准备时限内重新发现并获取证书；不接受失败样本，不仅凭错误文本重试身份变化。

集成时另发现 CameraActor 类角色没有静态实例，原“遍历每个对象的类”规则未保存 CameraActor 自己的 Super。已显式认证目标类继承链，并在序列化校验中拒绝缺失的继承信息。PID15696 的外部只读原生探针对同一份证书仅补此项后，由 `initial_binding` 拒绝变为 `guard=1`；没有在进程中写入或放宽原生检查。此前失败轮仅显示启动平面层、场景提交0，不算双目或过场成功。

实际运行、GPU图像和恢复结果以本节后续记录为准。预录视频、任意关卡的随机黑帧、真头显过场舒适度尚未验证；不能由 CPU 测试通过推定完成。

## 本次收尾状态

源码检查点 `0f5baa3` 已提交。53项锚点/准备 Python 测试、39项连续会话测试、40项启动器测试、8项采样测试通过；24个独立 native 检查通过。最后的 GameRecovery9 DLL SHA256 为 `11b6a68712e3d729ac400c10d92b7bc8151de24188524883d62fb9a66732ce06`，保留在独立构建目录，后续 native 修改须用新目录。

实验清单 `manifest-game-recovery9.json` 固定该 DLL 与原 SpaceRecovery3；不是当前默认，也不是发布验收通过。PID32900 一轮在用户切换全屏/窗口化时自动导航报找不到窗口，VR会话退出，游戏进程仍在，用户随后手动进入关卡。此轮未接入立体，不应归为游戏闪退或场景黑屏证据。接着一次启动被另一个聊天正在调整的后台启动参数解析阻止，未产生新游戏会话。

当时另一聊天正在使用游戏验证后台真实手柄，本聊天没有接管或关闭其进程。该测试结束后，本聊天完成了下述新会话验证。默认 Recovery8 保持不变。

## 窗口切换：导航恢复的离线修正

用户要求暂不使用 Win+0 获取焦点，避免干扰另一聊天的后台测试。本轮未操作、启动、停止或激活游戏，仅修改源码并运行离线测试。

`menu_navigation.navigate_core` 原先只等待首次窗口出现，绑定后短暂缺失或 HWND 改变都会终止启动。现在在同一绝对 deadline 内重新识别已保留进程的窗口；每次查询继续检查 PID/creation identity 与取消信号，关闭旧 reader，重新读取菜单并满足稳定等待。窗口失而复得但 HWND 相同也会清除旧稳定状态。导航动作账本跨重绑保留，避免重复确认和重置总动作上限；发送阶段失败不重试。发送前最后检查中的取消仍按取消处理。

17项导航测试、32项菜单控制测试与40项启动器测试通过，涵盖窗口消失/重建、构造 reader 时换窗、原时限、进程更换、取消、动作去重与部分发送失败。尚未在实际窗口切换中验证。XR 侧 `GameWindowFrames` 仍绑定启动时 HWND，真正销毁旧窗口时会报身份变化；该层恢复未包含在本次修正内。因此不能据此宣布窗口切换造成的 VR 退出已经完整修复，冻结 DLL 和默认清单均未变更。

## 首次实际过场双目与同进程重载证据

后续提交 `9ff8d2a` 加入 SpaceRecovery4 菜单窗口重绑，说明见文档68。实验清单 `manifest-game-recovery9-window1.json` 组合 GameRecovery9、SpaceRecovery4 与 quiet1 模拟器。2026-10-15 23:07 的游戏 PID36340，创建 FILETIME `134365504564396886`；会话 `continuous-stereo-36340-20261015-230749-cfe9a456`，owner PID37740、XR PID10248。后台输入 helper 回报启用成功。

第一张已保存 GPU 图像为 ticket1/source_serial1，1920×1080 SBS。其保存时刻前27.4451ms、后73.0187ms的相机采样均为剧情模式，且完整 ViewTarget 身份相同：CameraActor、number1、index168687。左右半幅平均亮度约72.95/73.10，实际检查图像可见两眼场景与视差。证据支持本次 CameraActor 路线成功输出双目；相机采样与 GPU 保存并非原子事务，不能扩大为所有过场已通过。

随后恢复 Pawn 视角，并通过已校验菜单状态的定向窗口消息在同一进程读取检查点。Pawn 从 number2/index169229 换为 number3/index234523；会话仍运行。重载后另存的 GPU 图像 ticket19723/source_serial19696 两眼平均亮度约88.70/89.83；该张保存时相机采样器已经结束，不附会不存在的相机前后夹测。采样的 native 状态为 published19693、anchor_submission_matched19873、anchor_submission_rejected1、stop_reason=none；一次拒绝的原因尚未归因，不能记作零错误。

首次成功投影提交距连续运行器启动约28.739秒（不是完整游戏启动耗时），启动延迟仍待缩短。用户要求预览持续可见，本会话留在运行状态，未执行最终清理/退出验收。只证明这一次模拟器与 GPU 路线；真头显、预录视频、死亡菜单、实际 HWND 重建及跨关卡长期黑帧仍未全部验证。

原始证据位于 ignored `analysis/runtime/camera-phase9h*.jsonl`、`camera-phase9h-evidence.json` 和该会话的 `.stdout.txt`、`.gpu.ppm*`。这些运行衍生资料不进入发布包。
