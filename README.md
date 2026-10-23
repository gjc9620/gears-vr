# GearsVR 0.0.3

2026-10-24：当前独立功能工作区使用 `6dof-cover` 分支，已整理[6DoF 掩体交互与实现规格](docs/6dof-cover-interaction-design-20261024.md)，继承0.0.4开发基线。此处仅完成规格和实现准备，新的抓边探身行为尚未实现。下文保留0.0.3发布背景。

按用户2026-10-23的要求，原0.1.0开发内容改以0.0.3发布，当前分支为 `codex/0.0.3`，原0.1.0分支已删除，内容和提交历史保留。本次发布进度见[0.0.3记录](docs/releases/0.0.3/PROGRESS.md)，此前[0.1.0记录](docs/releases/0.1.0/PROGRESS.md)保留为历史证据。

本机开发版使用仓库根目录 `J:\gearsVr\GearsVR Launcher.exe`，游戏主菜单的 `happyotaku playground` 入口由当前开发清单加载。解压到游戏目录的公开包使用独立清单，功能以该包版本为准。2026-10-22已修复Playground初始化失败中断VR启动的问题，见[定位与验证](docs/playground-startup-reservation-20261022.md)。

2026-10-22 已合并 `codex/0.0.2` 的启动兼容、EXE识别、中文路径／UTF-8及启动诊断修复，保留第一人称、Playground和渲染改动。公开最新版本为 [0.0.3](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.3)，已完成本次发布，变更见[CHANGELOG](CHANGELOG.md)。下文0.0.1包及验证结果保留其历史范围。

2026-10-21 起开发与测试使用已购买的Steam正版，默认工作目录 `J:\gearsVr\Gears of War Reloaded`，旧进度已备份并迁移到正版账号。两份正版完整3,609文件哈希一致；本地已补Steam启动身份与多账号设置兼容。用户已停止追加兼容性游戏测试，本次未新增正版双目／头显验收，不更新下述已发布0.0.1包。见[迁移记录](docs/retail-steam-compatibility-20261021.md)。

Gears of War: Reloaded 第三人称同帧双目 VR Mod。2026-10-21 已生成 0.0.1 本地公开测试包 `releases/0.0.1/GearsVR-0.0.1.zip`，已发布到 [GitHub Release](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.1)。用户已确认第一幕通过、KRVR + AVP 与 VD + Quest 3 可玩，并确认准星问题解决。保留机枪切换约2秒延迟、后续章节未全部验证及DLSS禁用的限制。安装见[随包教程](docs/releases/0.0.1/README.txt)，发布文案见[发布说明](docs/releases/0.0.1/RELEASE_NOTES.txt)，构建与检查范围见[打包记录](docs/releases/0.0.1/PACKAGE_BUILD.txt)。

当前包冻结 AdsAnchor2 native / MountedLatency1 XR，VR每眼尺寸为 OpenXR 推荐值 × 启动器 Render scale；游戏内分辨率控制桌面镜像。压缩包内置 Python，不要求玩家准备开发环境。以下日期记录及研究结果保留其历史范围。

2026-10-15 玩家测试入口已按要求撤下 Low/High 固定分辨率，改用游戏输出尺寸。双击本机仓库根目录 `GearsVR Launcher.exe`，选择模拟器或系统 OpenXR Runtime，点击“启动 VR”后自行进入关卡。“停止 VR”正常停止会话，不自动结束测试或强杀游戏。1920×1080 游戏输出对应每眼 960×1080；更改游戏分辨率后请重启游戏和 VR。新组合已通过编译及离线检查，闪烁是否消失待用户实测。参见[启动器说明](tools/vr-launcher/ui/README.md)和[当前构建记录](docs/62-game-resolution-launcher.md)。

- [新手流程与学习路线](docs/00-roadmap-and-learning.md)
- [摄像机定位证据](docs/02-camera-analysis.md)
- [原生摄像机分析](docs/03-native-camera-analysis.md)
- [原生调用验证结果](docs/04-native-capture-validation.md)
- [渲染朝向改写验证](docs/05-render-yaw-wobble-validation.md)
- [快捷调试启动与恢复](docs/06-debug-startup.md)
- [低消耗半自动采图与手柄工具](tools/game-io/README.md)
- [采图与手柄验证记录](docs/07-game-io-validation.md)
- [OpenXR 独立会话与提交验证](docs/08-openxr-independent-validation.md)
- [六轴原生视点验证](docs/09-six-axis-render-validation.md)
- [VR 交互方案记录（待实现）](docs/10-vr-interaction-design.md)
- [真实头显连接验证](docs/11-headset-connection-validation.md)
- [实时头动接入验证](docs/14-live-head-camera-test.md)
- [游戏窗口进入头显的显示原型](docs/15-game-screen-display.md)
- [最终部署交付目标](docs/16-deployment-target.md)
- [游戏原生 GPU 渲染对象观察](docs/17-native-gpu-observation.md)
- [第一人称 Mod 安装与恢复](docs/18-first-person-mod-install.md)
- [第一人称 Mod 晃动与后坐力细查](docs/19-first-person-motion-and-recoil.md)
- [原生 GPU 队列绑定与单帧回读](docs/20-native-gpu-binding-and-readback.md)
- [原生连续 GPU 画面共享](docs/21-native-gpu-sharing.md)
- [不戴头显的 OpenXR 自动验证](docs/22-openxr-simulator.md)
- [场景视图集合提交调查](docs/23-render-family-investigation.md)
- [渲染视图副本动态验证](docs/24-render-view-copy-validation.md)
- [完整渲染对象构造与原版对照](docs/25-renderer-construction-validation.md)
- [渲染线程消费与视图生命周期](docs/26-renderer-consumption-and-lifetime.md)
- [矩阵准备与历史数据调查](docs/27-render-matrix-preparation-leads.md)
- [渲染准备阶段前后验证](docs/28-render-preparation-validation.md)
- [左右眼视图计算的离线核对](docs/29-offline-stereo-view-math.md)
- [第二视图构造与对象边界调查](docs/30-second-view-construction-leads.md)
- [未提交源视图副本的原生验证](docs/31-source-view-shadow-validation.md)
- [关联状态的创建、历史数据与释放调查](docs/32-view-state-construction-and-release.md)
- [独立视图状态的原生生命周期验证](docs/33-independent-view-state-validation.md)
- [完整源视图构造参数与调用脚手架](docs/34-source-constructor-argument-map.md)
- [完整源视图构造的原生验证](docs/35-full-source-constructor-validation.md)
- [独立状态与完整源视图构造的组合验证](docs/36-independent-state-source-constructor.md)
- [眼位输入准备与四分量参数调查](docs/37-eye-input-preparation.md)
- [未提交偏移眼位的原生验证](docs/38-native-unsubmitted-eye-validation.md)
- [主线进度与下一次验收](docs/39-mainline-progress-and-acceptance.md)
- [双视图渲染器的对象边界](docs/40-two-view-renderer-boundary.md)
- [两视图原生渲染对象的构造验证](docs/41-two-view-renderer-construction-validation.md)
- [单帧双视图进入原生渲染通路](docs/42-single-frame-two-view-submission.md)
- [持续双视图与实际画面问题](docs/43-bounded-two-view-rendering-and-pixel-failure.md)
- [区域追踪与单视图对照](docs/44-view-rectangles-and-single-view-control.md)
- [DLSS 对照与右侧黑屏](docs/45-upscaler-control-and-missing-right-view.md)
- [GPU 绘制区域与裁剪证据](docs/46-gpu-viewport-and-scissor-evidence.md)
- [颜色目标与窗口后备缓冲对应](docs/47-render-target-and-backbuffer-evidence.md)
- [窗口最终输出的原生调用路径](docs/48-final-output-call-path.md)
- [左右视图的最终处理对象](docs/49-per-view-output-objects.md)
- [最终输出调用与区域参数](docs/50-final-output-quad-parameters.md)
- [RenderDoc 接入与单帧实测](docs/51-renderdoc-capture-compatibility.md)
- [RenderDoc 调用指针检查与失败记录](docs/52-renderdoc-import-bridge-failure.md)
- [双目诊断转向与运行时 Ghidra 分析](docs/53-render-diagnosis-and-runtime-ghidra.md)
- [渲染架构复审与局部几何观察的边界](docs/54-render-route-review-and-geometry-limits.md)
- [PIX 原版路径对照与定点回读前置条件](docs/55-pix-baseline-and-targeted-readback.md)
- [最终输入绑定与实际绘制路径](docs/56-final-input-binding-and-observed-draw-path.md)
- [准星、换枪 UI 和字幕的定位进展](docs/57-ui-position-and-subtitle-route.md)
- [调查工具使用说明](tools/package-inspector/README.md)

## 当前验证结果

摄像机定位器通过 EXE 版本哈希、模块 RVA、运行时反射元数据和对象关系识别当前玩家摄像机。已在两次游戏启动以及一次检查点重载中验证。工具不会把旧进程中的堆地址当作新进程地址使用。

Recovery2 的真实头显测试持续约 14 分 49 秒，用户确认两眼有游戏场景、整体效果不错；提出疑似 3DoF、准星及换枪 UI 位置偏移。日志有真实位置变化，双眼矩阵包含平移，但尚不能据此宣布方向、比例和近处视差体验通过。StartupMenu1 已在模拟器验证启动菜单显示、进入关卡后接续同帧双目及正常收尾。最新可复现范围、实验清单和未完成项见路线文档；第一人称和控制器持枪留待后续版本。

## 仓库边界

Git 保存我们编写的脚本、版本配置与研究文档。游戏、第三方软件、第三方 DLL、下载包、解压后的游戏包、运行时内存快照和反汇编输出留在本机，由 `.gitignore` 排除。`analysis/` 中的本地原始证据不随仓库复制；可通过调查工具重新生成。

现有工具中的路径以当前 Windows 工作目录为准。重新部署时需要自行准备游戏和工具依赖，参见工具说明。仓库不包含游戏文件或完整可运行的 VR Mod。
