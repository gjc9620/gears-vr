# 低消耗半自动游戏调试

已在 Reloaded 本机运行中验证：单窗口画面采集、虚拟 Xbox 360 手柄操作、
恢复暂停菜单、左右转镜头、只读镜头数据。服务不包含自动通关或识别模型。
由助手按当前测试目标选择少量动作，必要时查看一张图片。

## 原理

`WindowCapture.exe` 通过 Windows Graphics Capture 的 `CreateForWindow(HWND)`
绑定游戏窗口，在独立进程使用 D3D11 读取捕获纹理，不依赖游戏内部 D3D12 Hook。
它裁掉窗口边框、标题栏，默认把 1920×1080 客户区缩为 640×360 JPEG（质量 0.72）。
处理 HWND 时开启 DPI 感知，避免系统 200% 缩放导致只截左上角。
失败或窗口最小化时报告错误，绝不回退到桌面截图。

窗口画面包括游戏自己绘制的菜单；游戏暂停时捕获到的也会是暂停画面。
WGC 开始捕获时 Windows 可能短暂显示捕获边框。

`game_io.py serve` 维持一个 ViGEm 虚拟 Xbox 360 手柄，使用本地文件队列接收命令。
服务不监听网络，不注入键盘，不修改游戏内存。手柄是持久设备，避免每次操作重新接入。
`telemetry` 复用先前的只读、版本哈希验证过的摄像机定位器。

输入先核对完整游戏 EXE 路径及窗口焦点；单次动作 0.02–3 秒；每约 10ms
检查失焦、停止文件和 F8，最后通过 finally 将按钮、摇杆、扳机全部归零。
服务重启取消旧队列命令，超过 90 秒的命令不执行。
F8/失焦停止是尽力而为的轮询机制，响应还受系统调度影响。

## 启动

```powershell
& J:\gearsVr\tools\game-io\Start-GameIO.ps1
```

服务在后台隐藏运行，保持手柄中立，不会自动操作游戏。无需每轮重新启动。
当前 Python 路径：

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$io = 'J:\gearsVr\tools\game-io\game_io.py'
& $py $io status
& $py $io telemetry
& $py $io capture
```

`status` 返回游戏进程、焦点、手柄连接和 XInput 槽位。
`telemetry` 返回镜头位置/角度/FOV，并比较 0.2 秒内镜头时间是否更新。
时间停住只是暂停/停更线索，不能单独证明游戏的具体菜单状态。

截图保存于 `analysis/game-io/frames/`，默认只返回文件路径、分辨率、亮度、变化分数等
短 JSON，**不会自动把图片传给模型**。需要判断画面时，助手再打开选定图片。
亮度变化分数只是粗略像素差，不能单独证明移动成功。

更低清晰度可用 `capture --width 320`；读取小字时按需用 `--width 960`。
低 token 成本主要来自少读图片、降低图片尺寸，JPEG 文件字节数不是图像 token 数。

## 操作示例

尝试把焦点交给游戏（Windows 可能限制后台程序激活窗口，此时需要点游戏一次）：

```powershell
& $py $io focus
```

短按确认键，默认结束后截图一次：

```powershell
& $py $io action --buttons a --seconds 0.12
```

轻转镜头，不截图，之后用数字检查结果：

```powershell
& $py $io action --right 0.55 0 --seconds 0.45 --no-capture
& $py $io telemetry
```

可用按钮：a/b/x/y/start/back/up/down/left/right/lb/rb/ls/rs。
摇杆 `--left X Y` / `--right X Y` 范围 -1..1；扳机 `--lt V` / `--rt V` 范围 0..1。
`--wait-focus 45` 可先等待用户将焦点交给游戏，再执行一段短动作。
摇杆需超过游戏死区，小数值可能没有效果。

停止服务并归零手柄：

```powershell
& $py $io stop
```

也可以按住 F8 停止服务。失去焦点只中止当前输入，不退出整个服务。
不要在 Windows 窗口激活失败后反复发输入；用户点击一次即可。
实体手柄若占了另一个槽位且游戏忽略虚拟手柄，需要用户断开实体手柄再重新连接服务；
当前实测虚拟手柄占槽位 0，游戏已接收。

## 推荐工作流程

1. 开始一轮调试时，将焦点交给游戏；让工具根据暂停画面选择恢复操作。
2. 助手读取一张 640×360 图片确定状态，执行一小段确定的动作。
3. 优先用镜头时间/角度等数字检查结果；需要看场景时才再截一张。
4. 用户回来聊天时中止输入；下一轮重新交接焦点。没有定时截图或视频录制。

## 依赖与重建

已利用本机现有 ViGEmBus 驱动，未安装/重装系统驱动。
vgamepad 官方仓库固定在 v0.1.0，commit `60bfb9012d7d87df6eb56844bda15626f60a04b1`，
位于 Git 忽略的 `tools/third-party/vgamepad`。直接引用其中 Python 模块和客户端 DLL，
未运行 setup 或驱动安装器。

```powershell
git clone --depth 1 --branch v0.1.0 https://github.com/yannbouteiller/vgamepad.git J:\gearsVr\tools\third-party\vgamepad
cmd /c J:\gearsVr\tools\game-io\build.bat
& $py J:\gearsVr\tools\game-io\test_game_io.py
```

构建需要本机 VS、Windows SDK 的 C++/WinRT 头文件及 CMake；路径见 build.bat。
第三方、编译文件、截图和日志都排除在 Git 外。

参考：[微软单窗口捕获 API](https://learn.microsoft.com/en-us/windows/win32/api/windows.graphics.capture.interop/nf-windows-graphics-capture-interop-igraphicscaptureiteminterop-createforwindow)、
[vgamepad 官方使用说明](https://github.com/yannbouteiller/vgamepad/blob/v0.1.0/README.md)。
