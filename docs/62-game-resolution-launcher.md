# GameResolution1 与本机测试启动器

2026-10-15。用户报告桌面及模拟器左眼闪烁，要求先撤下 Low/High，按游戏分辨率渲染，并自行使用启动器测试。当前尚不能把闪烁归因为自定义分辨率或左右眼叠加。本轮不启动游戏，不延续固定目标试验。

## 本轮切换

- 新建 `build-game-resolution1/GearsCaptureGameResolution1.dll`；游戏现有 backbuffer 及其真实尺寸驱动同帧 SBS，关闭所有固定 owned 目标、resize reuse、owned Canvas/Material 路由及独立桌面镜像。桌面可能显示 SBS，不保证单眼镜像。
- CMake `GEARS_GAME_RESOLUTION=ON` 明确拒绝同时开启任何固定目标相关选项，原生状态输出 `resolution_source=game`。旧研究代码及清单留作回滚与诊断，不进入当前玩家路径。
- 删除未接入启动入口的 `vr_quality.py` 及其专用测试。玩家配置不接受 `vr_quality`；新 GUI 没有档位控件。
- 保留同帧双目、原有 HUD 平面、准星方向修正、Material 观察、菜单显示及恢复、DLSS/DLAA 启动／运行时／菜单锁。固定目标专用 UI 路由随该路径关闭，不能把它的像素证据直接计作当前构建验收。
- 游戏 1920×1080 时各眼 960×1080；3840×2160 时各眼 1920×2160。未改游戏分辨率配置。启动后改变实际输出尺寸的恢复未验证，先改设置并正常退出，再启动新会话。

## 本机构建

默认 `tools/vr-launcher/manifest.json` 与显式 `manifest-game-resolution.json` 内容相同；原默认保存为 `manifest-legacy-continuous3.json`。

| 文件 | SHA256 |
| --- | --- |
| GameResolution1 native DLL | `e51040fe5a3f30896d4126ee32c46184ad057a18d9dae511ad6e1aec1fb1250c` |
| MenuRecovery1 OpenXRPoseProbe.exe | `9c9eb058ea0411509d71f8f26f16e1e2ba30ba3d6903567bd582022c1cd98e68` |

native 使用 MSVC x64、Release、NMake，在全新构建目录开启：`GEARS_GAME_RESOLUTION`、`GEARS_GRAPHICS_LOCK_OBSERVER`、`GEARS_GRAPHICS_RUNTIME_LOCK`、`GEARS_GRAPHICS_STARTUP_LOCK`、`GEARS_GRAPHICS_MENU_LOCK`、`GEARS_UI_MATERIAL_ROUTE`、`GEARS_UI_RETICLE_DIRECTION`。其余实验选项均 OFF。本机命令保存在忽略目录 `analysis/runtime/build-game-resolution1.cmd`，CMakeCache 与验证报告记录实际开关。不要覆盖已供玩家测试的构建；另行重编译需新目录和新哈希清单。

GUI 为 C# WinForms，源码 `tools/vr-launcher/ui/GearsVrLauncher.cs`，构建脚本 `ui/build.ps1`，使用系统已有 .NET Framework csc。产物复制到仓库根目录 `GearsVR Launcher.exe`；不安装工具或新运行库。此入口依赖本机仓库、Python、原生构建和 Runtime，不是便携发布包。

## 操作

正常退出已有游戏，双击启动器，选择本地 OpenXR 模拟器或系统 Runtime，点击启动，然后自行进入关卡。没有自动导航或自动结束计时。停止按钮仅请求现有 owner 正常清理；关闭 GUI 时活跃 owner 会先收到停止请求，窗口保留等待。很早点击停止时会保留请求，直到对应 PID／创建时间的停止接口就绪。

模拟器使用本地已准备的 `analysis/runtime/simulator-focus1/openxr_simulator.json`；系统 Runtime 选项清除子进程继承的 `XR_RUNTIME_JSON`，不改系统注册表。`SimulatorVR.ps1` 也切到新构建并取消自动导航。游戏路径仍受版本哈希验证。

## 验证边界

31 条 native CPU／ABI 命令通过，包含实际新 DLL 的启动预检与持续会话自测、1920×1080 非固定矩形、独立左右视图、FrameLink、HUD／准星及图形菜单保护。启动器 36 项与菜单发布监视器 7 项测试通过；GUI 编译无警告，路径／转义／配置保留／owner 身份握手／Python 探测自测通过。游戏、native、XR 文件哈希全部核验。native 编译有原有 MSVC 警告，未宣称无警告。

原始检查报告在 `analysis/runtime/game-resolution1-validation.json`。第一次监视器测试误用了不存在的文件名，测试发现返回 5；已改用 `tools/native-capture/test_menu_display_monitor.py`，实际 7 项通过，原调用保留在报告中。

没有启动游戏、截图或真头显验收。本次说明只确认切换路径、构建与离线检查；闪烁、右眼更新、菜单／UI 实际显示以及正常退出需由新构建的真实运行结果判断。
