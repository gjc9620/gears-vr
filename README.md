# GearsVR 研究原型

目标是为 Gears of War: Reloaded 交付第三人称同帧双目 VR Mod，包含 6DoF 头部跟踪、菜单和游戏 UI。当前同帧双目已经过真实头显游玩，启动菜单到关卡的同一 OpenXR 会话切换已在模拟器验证；准星、换枪 UI、字幕、位置追踪体验、运行时 DLSS 锁定及便携发布包仍未完成。

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
