# 低消耗半自动游戏调试

2026-10-15 新增独立的 `background_input.py`：项目普通游戏和 VR 共用启动入口默认放行原生控制器的后台处理，实际后台“启用→恢复→再启用”镜头和人物移动对照通过。修改与停止方法见[后台手柄记录](../../docs/67-background-controller.md)。本文件下述 `game_io.py action` 的失焦自动回中规则仍然适用；自动测试工具的输入保护与游戏原生输入能力分开。

已在 Reloaded 本机运行中验证：单窗口画面采集、虚拟 Xbox 360 手柄操作、
恢复暂停菜单、左右转镜头、只读镜头数据。服务不包含自动通关或识别模型。
由助手按当前测试目标选择少量动作，必要时查看一张图片。

2026-10-14 菜单导航改用 `menu_state.py` 和 `menu_control.py`：读取 UE3 当前场景、玩家零的焦点链及弹窗文字，再向核验过的游戏窗口发送一次确认。无需逐页截图，也不按固定时长连续回车。

```powershell
& $py J:\gearsVr\tools\game-io\menu_state.py --pid <本次游戏PID> --out J:\gearsVr\analysis\runtime\menu-state.json
& $py J:\gearsVr\tools\game-io\menu_control.py --pid <本次游戏PID> --seconds 60 --out J:\gearsVr\analysis\runtime\menu-control.jsonl
```

读取器按 EXE 哈希校验版本，重新定位对象注册表和反射字段；不修改游戏内存。支持已校准的标题页、英文在线服务错误、战役入口、单人游戏、继续战役。未知弹窗、不同语言或焦点不匹配时等待并超时，不自动确认。每次动作前要求状态稳定至少 0.6 秒，再重读状态并核对 PID/HWND。F8 或服务停止标记中断操作。

标题页存在不接收输入的 AttractLauncher 覆盖层，仅对已验证的三项场景栈作专门处理。弹窗通过 Title/MessageText 的 UIString 首个原始文本节点读取；原生节点布局是本版本运行时交叉校准所得，未知 vtable、错误归属或超长字符串均拒绝。普通菜单缓存后约 1 毫秒，弹窗约 1.6 毫秒；首次定位约 1–2 秒，过渡期间读取失败会重试。

`loading_requested` 仅表示发送了“继续战役”，不表示关卡加载完成。VR 启动仍须独立验证已控制的 WarPawn、Controller、Level 关系和更新的摄像机时间戳。JSONL 记录状态变化与动作，不自动保存或上传截图。

实测：PID16336 从主菜单到继续战役用时 5.313 秒；fresh PID33964 从 UI 尚未初始化开始等待并完成全部五步共 21.297 秒，随后启动器独立确认关卡玩家就绪。40 项离线测试通过。支持范围仅限上述已校准路径，不是任意菜单或任意存档选择器。

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

另有 `confirm_window.py`，每次只向路径核验过的游戏 HWND 发送一组 Enter
WM_KEYDOWN/WM_KEYUP，不发送全局按键。联网失败弹窗与战役菜单已观察到接受该消息，
即使没有游戏焦点也能工作；不代表所有游戏输入都能后台执行。
手动诊断时先确认当前菜单；自动导航使用上面的内存状态控制器。
游戏内手柄动作仍遵循焦点要求。

当用户明确指定任务栏位置时，可用独立的 `focus_taskbar.py` 激活窗口。例如用户指定Win+0：

```powershell
& $py J:\gearsVr\tools\game-io\focus_taskbar.py --slot 0 --pid <本次游戏PID> --creation-time <本次FILETIME>
```

此工具核验游戏路径、PID及创建时间，仅在尚未聚焦且没有按住修饰键时发送指定快捷键，最后确认前台HWND就是该游戏。槽位由用户提供，不猜测任务栏排列；失败时不发游戏输入。它不重启虚拟手柄服务，也不把“快捷键已发送”当成聚焦成功。
`telemetry` 复用先前的只读、版本哈希验证过的摄像机定位器。

手柄连接成功不等于已经分配给游戏玩家。2026-10-14 的 PID34688 使用窗口 Enter
进入关卡后，原生 XInput 设备仍为未分配状态（玩家索引 -1）；纯摇杆命令虽然送达
Windows，但被游戏跳过。经一次普通 Back 短按认领为玩家 0 后，同样的右摇杆使
原生相机转动 20.484°。自动运动测试应核对实际相机变化，不能只检查命令 `ok`。
Back 的行为取决于当前界面；这次已验证的正常关卡没有新增菜单，不代表可以在
未知界面盲按。证据位于 `analysis/runtime/input-assignment-34688-*.json`。

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

1. 从启动菜单进入已有关卡时，运行内存菜单控制器；不会要求逐页截图或手动交接焦点。
2. 收到 `loading_requested` 后，独立检查玩家与关卡就绪，再开始 VR 测试。
3. 普通游戏手柄动作仍需要焦点。优先用镜头时间/角度等数字检查结果；未知 UI 或需要验证渲染时才截一张 640 像素宽图片。
4. 没有定时截图或视频录制；F8/停止标记可以中止自动菜单输入。

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
