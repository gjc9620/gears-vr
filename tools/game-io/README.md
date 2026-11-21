# Low-consumption semi-automatic debugging

2026-10-15 Add a new separate `background_input.py`: The project common game and VR share boot access defaults the backstage processing of the primary controller, with the actual backstage " Enables the restoration of the re-enabled" camera and movement of people cross. See changes and stop methods.[Backstage handle record.](../../docs/67-background-controller.md)。`game_io.py action` Default is still in the middle of a focal failure; identification and patch check for visible backstage actions are shown below.

Valided in Réloaded, single window image capture, virtual Xbox 360 Handheld,
Restores the pause menu, left and right lenses, read-only lens data. The service does not contain automatic customs clearance or recognition models.
Select a small number of actions by the assistant against the current test target and view a picture if necessary.

2026-10-14 Menu Navigator Reuse `menu_state.py` and `menu_control.py`: Read UE 3 The current scene, player zero focus chain and bullet window text send a confirmation to the verified game window. There is no page-by-page cut-off or a fixed time frame for returning to the car.

```powershell
& $py J:\gearsVr\tools\game-io\menu_state.py --pid <本次游戏PID> --out J:\gearsVr\analysis\runtime\menu-state.json
& $py J:\gearsVr\tools\game-io\menu_control.py --pid <本次游戏PID> --seconds 60 --out J:\gearsVr\analysis\runtime\menu-control.jsonl
```

Reader validates the version of the EXE Hashi, repositioning object registration forms and reflecting fields; does not modify the memory of the game. Supports calibrated title pages, English online service errors, battle entrances, single-person games, continuing operations. Wait and timeout when unknown bullet windows, different languages or focus do not match, do not automatically confirm. 0.6 seconds, reread the status and check PID/HWND.8 or the service stop tags aborting.

The title page contains the AttractLauncher overlay that does not receive input, with only three validated scenes being specially processed. The bullet window is read through the first UIString original text node of Title/MessageText; the original node layout is obtained by cross-calibration while running this version, and the unknown vtable, error attribution or super long string is rejected. 1 M-sec, click the window. 1.6 milliseconds;first positioning agreement 1–2 seconds, read failure during the transition is repeated.

`loading_requested` Only the "continue of battle" was sent, which does not mean that the level is loaded. VR starts a camera time stamp that remains independently certified for controlled WarPawn, Controller, Level relationships and updates. JSONL records changes in state and actions, without automatically saving or uploading intercepts.

Actual: PID 16336 From the main menu to the continuation of the battle 5.313 seconds;fresh PD 33964 Wait and complete all five steps from UI without initialization 21.297 seconds, then the starter independently confirms that the player is ready.40 The field offline test is passed. Support is limited to the above calibrated path, not an arbitrary menu or any archive selection.

## Rationale

`WindowCapture.exe` Through Windows Graphics Capture `CreateForWindow(HWND)`
Tie game windows to use D in standalone processes 3D 11 Read capture texture without relying on inside game D 3D 12 Hook。
It cuts the window border, title bar, defaults 1920×1080 Client area reduced to 640×360 JPEG (Quality) 0.72）。
Enable DPI perception while processing HWND, avoid system 200% The scaling leads to the upper left corner only.
Reporting errors when failure or window minimization is not reversed to the desktop screenshot.

The window image includes the menu that the game draws itself; what is captured when the game is suspended will also be paused.
WGC may show the capture border briefly when you start capturing.

`game_io.py serve` Maintain a Vigem virtual Xbox 360 Hand handle, take orders using local file queues.
The service does not listen to the network, does not inject keyboards and does not modify the game memory. The handle is a durable device that avoids re-entry at each operation.

And then there's something else. `confirm_window.py`Send a group of Enters to every game with only a path check
WM_KEYDOWN/WM_KEYUP, does not send global buttons. The failure of the network window and battle menu have been observed receiving this message.
Even without the focus of the game, it works; this does not mean that all game inputs can be executed in backstage.
Confirms the current menu when you have a manual diagnosis; auto-navigation uses the memory status controller above.
The handle action of the game is default to focus; backstage actions must be visiblely enabled and process identity bound.

When user specifies the position of the taskbar, you can use separate `focus_taskbar.py` Activate the window. For example, the user specifies Win+0：

```powershell
& $py J:\gearsVr\tools\game-io\focus_taskbar.py --slot 0 --pid <本次游戏PID> --creation-time <本次FILETIME>
```

This tool verifies the game path, PID and creation time by sending specified shortcut keys only when it is not focused and does not hold the qualifying key. Finally, confirm that the front desk HWND is the game. The slot is provided by users without guessing about the taskbar; no game input is sent if a failure occurs. It does not restart virtual handle services or consider " shortcuts sent " as a focus success.
`telemetry` The video is re-used as a previously read-only, version-of-the-art camera locator.

Hand-held connections are not the same as being assigned to game players.2026-10-14 PID 34688 Use window Enter
After entering the level, the original XInput device is still undistributed (player index -1); pure rocker commands delivered
Windows, but skipped by a game. A regular Back short-sum player 0 After that, the same right swinger
Native camera turns 20.484°. Automatic motion tests should check actual camera changes, not just the commands `ok`。
Back's behavior depends on the current interface; this validated normal level does not have a new menu, which does not mean that you can
Unknown interface blindly pressed. Evidence is located in `analysis/runtime/input-assignment-34688-*.json`。

Enter to check the full EXE path first; default mode requires focus of the window. One action 0.02–3 seconds;per cent 10ms
Check the protection conditions, stop files and F for mode 8, and finally zero the buttons, the boomers, the triggers.
Service restart cancels old queue command, more than 90 Second orders are not executed.
F 8 The response is also affected by the system ' s movement.

## Start

```powershell
& J:\gearsVr\tools\game-io\Start-GameIO.ps1
```

The service is hidden from the backstage, with a handle neutral and does not automatically operate games. No re-starts each round.
Current Python path:

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$io = 'J:\gearsVr\tools\game-io\game_io.py'
& $py $io status
& $py $io telemetry
& $py $io capture
```

`status` Returns game processes, focus, handle connections and XInput slots.
`telemetry` Return lens position/angle/FOV, and compare 0.2 is the time of the second lens updated.
Time stops are pauses/stops more clues and cannot be independently certified for the specific menu status of the game.

Can not open message `analysis/game-io/frames/`Default returns only file paths, resolution, brightness, change fractions, etc.
Short JSON,**It doesn't automatically pass the picture to the model.**. When you need to judge the image, the assistant opens the selected picture.
Brightness change fractions are only rough pixels and cannot be shown to have moved successfully alone.

Lower resolution available `capture --width 320`;as needed to read small words `--width 960`。
Low token costs are mainly from lower reading and lower size of pictures, while JPEG bytes are not images token numbers.

## Example of operation

Try to focus on the game (Windows may limit backstage programs to activate windows, and then a little game is required):

```powershell
& $py $io focus
```

Short-click confirmation key, and once after default screenshot:

```powershell
& $py $io action --buttons a --seconds 0.12
```

Turn the camera down, no screenshots, and then use the digital results:

```powershell
& $py $io action --right 0.55 0 --seconds 0.45 --no-capture
& $py $io telemetry
```

Available buttons: a/b/x/y/start/back/up/down/ left/right/lb/rb/ls/rs.
Rock the pole. `--left X Y` / `--right X Y` Scope -1..1;the trigger `--lt V` / `--rt V` Scope 0..1。
`--wait-focus 45` You can wait for the user to turn the focus over to the game and then execute a short action.
The roller is required to exceed the dead end of the game, and a small number may not be effective.

Visible Backstage Actions (Performance)2026-10-16: Using the exact PID of this game and creating FILETIME, no window or taskbar shortcuts are activated:

```powershell
& $py $io action --background --pid <本次游戏PID> --creation-time <本次FILETIME> --right 0.55 0 --seconds 0.45 --no-capture
```

Service must run a new version `game_io.py`; old services are lacking `background_action_v1` When capability identification is performed, the client refuses to line up and requests to stop and run `Start-GameIO.ps1`。
Backstage mode calls first `background_input.change(pid, 'status', expected_creation=...)`, read-only full input function in `enabled` Status; not enabled, unknown bytes, PID or creation time inconsistent are rejected and patches are not automatically modified.
Check the PID/HWND, process creation and stop request before sending and throughout maintenance. Try to get back in as soon as you fail. Backstage mode cannot be combined `--wait-focus`; still re-use the same permanent virtual handle.
This model has been tested for mock offline protection; the effects of actual game actions in the backstage still need to be independently validated with camera, character or corresponding scene data.

Stopped service and was placed on zero handle:

```powershell
& $py $io stop
```

You can hold it.8 Stops the service. Loss of focus only suspends current input and does not quit the entire service.
Do not re-enter the Windows window after it failed; one click is sufficient.
(a) If the physical handle is taken over another slot and the game ignores the virtual handle, it requires users to disconnect the physical handle before reconnecting services;
Current virtual handles in slot 0 The game is accepted.

## Recommended workflow

1. Runs the memory menu controller when you enter a card from the startup menu; no page-by-page screenshot or manual contact focus is required.
2. Copy that. `loading_requested` , the VR test is started after independent checking of players and level levels.
3. Default handle action needs focus; mandated backstage testing can use the above-mentioned identity binding mode visibly. Priority is given to digital results such as lens time/angle; unknown UI or need to verify rendering 640 Pixels wide.
4. No time-trip or video recording; F 8/ Stop tags can stop automatic menu input.

## Dependency and reconstruction

VGEmBus driver is now available on-board, and no system drive has been installed/reloaded.
vgamepad official repository fixed to v 0.1.0，commit `60bfb9012d7d87df6eb56844bda15626f60a04b1`，
Ignored at Git `tools/third-party/vgamepad`. Directly quotes the Python module and client DLL,
No setup or driver installed.

```powershell
git clone --depth 1 --branch v0.1.0 https://github.com/yannbouteiller/vgamepad.git J:\gearsVr\tools\third-party\vgamepad
cmd /c J:\gearsVr\tools\game-io\build.bat
& $py J:\gearsVr\tools\game-io\test_game_io.py
```

Build C++/WinRT headers and CMake that require the VS, Windows SDK; path to find the built.bat.
Third parties, compile documents, screenshots and logs are excluded from Git.

References:[Microsoft Single Window Capture API](https://learn.microsoft.com/en-us/windows/win32/api/windows.graphics.capture.interop/nf-windows-graphics-capture-interop-igraphicscaptureiteminterop-createforwindow)、
[vgamepad Official User Note](https://github.com/yannbouteiller/vgamepad/blob/v0.1.0/README.md)。

---

## 中文

# 低消耗半自动游戏调试

2026-10-15 新增独立的 `background_input.py`：项目普通游戏和 VR 共用启动入口默认放行原生控制器的后台处理，实际后台“启用→恢复→再启用”镜头和人物移动对照通过。修改与停止方法见[后台手柄记录](../../docs/67-background-controller.md)。`game_io.py action` 默认仍在失焦时自动回中；显式后台动作的身份与补丁校验见下文。

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
游戏内手柄动作默认遵循焦点要求；后台动作必须显式启用并绑定进程身份。

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

输入先核对完整游戏 EXE 路径；默认模式要求窗口焦点。单次动作 0.02–3 秒；每约 10ms
检查模式对应的保护条件、停止文件和 F8，最后通过 finally 将按钮、摇杆、扳机全部归零。
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

显式后台动作（2026-10-16）：使用本次游戏的精确 PID 与创建 FILETIME，不会激活窗口或发送任务栏快捷键：

```powershell
& $py $io action --background --pid <本次游戏PID> --creation-time <本次FILETIME> --right 0.55 0 --seconds 0.45 --no-capture
```

服务必须运行新版 `game_io.py`；旧服务缺少 `background_action_v1` 能力标识时，客户端拒绝排队并要求先停止、再运行 `Start-GameIO.ps1`。
后台模式先调用 `background_input.change(pid, 'status', expected_creation=...)`，只读核验完整输入函数处于 `enabled` 状态；未启用、未知字节、PID 或创建时间不符均拒绝，不自动修改补丁。
发送前及整个保持期间检查完整路径对应的 PID/HWND、进程创建身份和停止请求，失败后立即尽力回中。后台模式不能组合 `--wait-focus`；仍复用同一个持久虚拟手柄。
本模式已通过 mock 离线保护测试；后台实际游戏动作效果仍需用镜头、人物或对应场景数据独立验证。

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
3. 默认手柄动作需要焦点；已获授权的后台测试可显式使用上述身份绑定模式。优先用镜头时间/角度等数字检查结果；未知 UI 或需要验证渲染时才截一张 640 像素宽图片。
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
