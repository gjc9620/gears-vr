# GearsVR 研究原型

目标是为 Gears of War: Reloaded 实现双目立体画面和 6DoF 头部跟踪。当前成果是摄像机定位工具、原生函数调用采集工具与研究记录，尚未实现 VR 渲染或头部跟踪。

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
- [调查工具使用说明](tools/package-inspector/README.md)

## 当前验证结果

摄像机定位器通过 EXE 版本哈希、模块 RVA、运行时反射元数据和对象关系识别当前玩家摄像机。已在两次游戏启动以及一次检查点重载中验证。工具不会把旧进程中的堆地址当作新进程地址使用。

原生视点的三轴旋转和三轴平移已通过当前关卡的受控微扰记录验证，测试结束后视图恢复基线。独立 OpenXR 会话及左右眼测试图提交成功，但头部数据始终是默认姿态，尚未验证真实头显运动。坐标转换和数据有效性检查已离线测试，尚未接入游戏；真实脚本 VM 停顿、游戏双眼渲染与完整 6DoF 仍待实现。

## 仓库边界

Git 保存我们编写的脚本、版本配置与研究文档。游戏、第三方软件、第三方 DLL、下载包、解压后的游戏包、运行时内存快照和反汇编输出留在本机，由 `.gitignore` 排除。`analysis/` 中的本地原始证据不随仓库复制；可通过调查工具重新生成。

现有工具中的路径以当前 Windows 工作目录为准。重新部署时需要自行准备游戏和工具依赖，参见工具说明。仓库不包含游戏文件或完整可运行的 VR Mod。
