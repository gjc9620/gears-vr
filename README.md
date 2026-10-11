# GearsVR 研究原型

目标是为 Gears of War: Reloaded 实现双目立体画面和 6DoF 头部跟踪，最终交付可部署版本。当前已验证受限真实头动控制原生游戏视点，以及游戏窗口画面进入头显的二维面板；正在调查原生 GPU 渲染路径，完整 VR Mod 尚未完成。

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
- [调查工具使用说明](tools/package-inspector/README.md)

## 当前验证结果

摄像机定位器通过 EXE 版本哈希、模块 RVA、运行时反射元数据和对象关系识别当前玩家摄像机。已在两次游戏启动以及一次检查点重载中验证。工具不会把旧进程中的堆地址当作新进程地址使用。

原生视点的三轴旋转和三轴平移已通过当前关卡的受控微扰记录验证。Quest 3 / VDXR 真实头动及测试图显示已验证；受限头动已接入游戏临时渲染视点，停止后入口恢复。游戏窗口图像的 OpenXR 二维面板提交已通过 GPU 像素回读和 30 秒实测，用户确认关卡画面连续、颜色正常。真实脚本 VM 停顿、第一人称锚点、游戏双眼渲染与完整 6DoF 仍待实现。

## 仓库边界

Git 保存我们编写的脚本、版本配置与研究文档。游戏、第三方软件、第三方 DLL、下载包、解压后的游戏包、运行时内存快照和反汇编输出留在本机，由 `.gitignore` 排除。`analysis/` 中的本地原始证据不随仓库复制；可通过调查工具重新生成。

现有工具中的路径以当前 Windows 工作目录为准。重新部署时需要自行准备游戏和工具依赖，参见工具说明。仓库不包含游戏文件或完整可运行的 VR Mod。
