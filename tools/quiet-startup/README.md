# Experimental background startup

The repository background launcher defaults to the frozen quiet11 Windows x64
startup policy. Diagnostic builds are separate and are not promoted automatically.
On the development PC, actual-game background startup passed after the user
approved a FancyZones exclusion for `GOWDE-Steam.exe` (2026-10-16). FancyZones
automatic remembered-zone placement otherwise restored and activated the game
outside the process-local policy. This machine-specific compatibility setting
is not automatically installed on players' machines.

The exclusion also disables manual FancyZones zoning for this game; other apps'
settings are unchanged. Ordinary window movement remains available. DisplayFusion
was independently excluded as a necessary cause and remains closed by the user.
See `docs/68-window-recovery-and-quiet-startup.md` for observations and limits.

The first actual-game observation (9j) recorded two foreground events even though
all eight API hooks remained installed. The user subsequently confirmed manually
switching to the game, so these events do not establish automatic focus stealing.
VR separately stopped because the simulator reused an existing
swapchain handle after a menu resize. See `docs/68-window-recovery-and-quiet-startup.md`;
neither issue is covered by the fixture's passing result.

`GearsEarlyLaunch.exe EXE GearsQuietStartup.dll [arguments]` creates a new owned
child under a temporary debugger, restores its one-byte entry breakpoint, holds
the initial thread, and detaches before loading the policy. Only that new child
is targeted. A failed bootstrap terminates its own unreleased child; it never
attaches to an existing game, retries ordinary startup, or modifies disk code.
Remote-thread timeouts do not free live arguments or terminate individual threads.
Before release, the launcher queries the installed policy and checks the restored
entry. Visible windows before the entry point are rejected as too late.

The policy covers process-local ShowWindow, SetForegroundWindow, SetActiveWindow,
SetFocus, SetWindowPos, BringWindowToTop and CreateWindowExA/W. When the game is
already the foreground process, calls are passed through. Background top-level
windows are created hidden and shown without activation; caller extended styles
are preserved. Hiding uses SetWindowPos with NOACTIVATE; minimizing uses
SHOWMINNOACTIVE. No global focus lock, synthetic keyboard input or focus-restoring
loop is installed. Policy code remains loaded until process exit.

For an explicit repository test, set `GEARSVR_QUIET_STARTUP_BUILD` only in the
launcher's child environment to the frozen build directory. `background_launch.py`
also accepts `--quiet-build`; it rejects combination with `--foreground`, validates
the game installation and returned PID/creation time/path, then arms the existing
background-controller helper against the game PID. The bootstrap PID is distinct.
Unset the option to use the hash-verified frozen quiet11 build. The launcher does
not edit third-party settings or global registry/runtime settings.

The command builder leaves simple arguments such as `-NoPauseUnfocus` unquoted.
The game's raw command parser accepts only end-of-string, space or tab after a
switch, so surrounding that switch with quotes disables it even though Windows
argv parsing succeeds. Arguments that need quotes still use Windows backslash
escaping. `QuietCommandLineTest` checks the exact raw switch and argv round trips;
the actual game must additionally verify its internal command line and behavior.

Build with the existing MSVC x64 toolchain and CMake; MinHook is the existing
`tools/third-party/minhook` dependency. Use a new build directory after changes.
The native fixture runs offscreen and exits, while observing creation, delayed
activation, parent/child visibility, owned windows, hide/minimize/restore and
unchanged extended styles. `check_fixture.py` separately checks foreground events,
missing policy rejection and actual failed-child exit. It never manipulates the
user's keyboard or mouse. Passing it does not prove game compatibility or manual
click/Alt-Tab behavior.

Known limits: TLS/DLL initialization occurs before the EXE entry; this boundary
does not guarantee that no earlier focus request occurred. A whole-launch event
monitor is still required. Creating initially hidden changes visibility during
creation callbacks. A background automatic maximize request uses a nonactivating
normal show; manual foreground maximize is passed through. The current default
is windowed. No multiplayer or anti-cheat compatibility is claimed.

Windows reference: [debug events](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent),
[remote thread lifecycle](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread),
[show commands](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

Development evidence is under ignored `analysis/runtime/quiet-startup*-*.json`.
No proprietary snapshots, analysis databases or pseudocode belong in this folder
or in a release package.

For bounded diagnosis only, `GEARSVR_QUIET_TRACE=1` records the first 128 API calls
in process memory, including suppressed and passed calls, NULL targets, arguments,
foreground windows/timestamps and raw return addresses. It does not wrap or log
to disk. `GearsQuietTrace(void*)` copies completed records into a caller-owned
`TraceSnapshot` (37920 bytes); its independent ABI does not change `Status`.
Busy slots are skipped and overflow is explicit. Treat the addresses as private
runtime evidence, not release content.

Tracing additionally observes five activation-related messages through
DefWindowProcW/A, forwarding them unchanged. These two extra hooks are absent
when tracing is disabled. Their records describe message delivery, which can
occur after activation; they do not by themselves identify the requesting API
or process. Actual game-only tests still reproduce foreground activation while
all eight policy hooks remain installed. The quiet policy remains experimental.

---

## 中文

# 实验背景启动

仓库背景发射器默认为已冻结的静态11 窗口 x64
启动政策。诊断性建筑是分开的,不会自动推广。
在开发的 PC 上, 实际游戏背景启动在用户之后通过
批准排除 `GOWDE-Steam.exe` (2026-10-16) 风景区
自动恢复并激活游戏的记忆区设置
进程本地化策略之外。此机器特定的兼容设置
并不是自动安装在玩家的机器上。

排除也会禁用此游戏的手动 FancyZones 分区; 其他应用程序'
设置不变。普通窗口移动仍然可用。显示聚合
被独立排除为必要原因,用户仍予以关闭。
See `docs/68-window-recovery-and-quiet-startup.md` 用于观察和限制。

第一次实际游戏观察( Q)9(j) 记录了两个前台事件,尽管
API 钩子全部安装完毕。 用户随后手动确认
切换到游戏,因此这些事件不会自动建立对焦偷盗.
VR 单独停止, 因为模拟器重用已有的
菜单大小调整后互换链柄。参见 `docs/68-window-recovery-and-quiet-startup.md`;
问题都由定点结果处理

`GearsEarlyLaunch.exe EXE GearsQuietStartup.dll [arguments]` 创建新拥有的
临时调试器下的孩子,恢复其单字节的输入断点, 持有
初始线程, 并解析后再装入策略。 只有新来的孩子
失败的靴子会终止自己的未释放的孩子;它永远不会
附加到已有的游戏中,重试普通的启动,或者修改磁盘代码.
远程线程超时不能释放活参数或终止单个线程.
发布前,发射装置会查询已安装的策略并检查恢复的
条目。入口前可见窗口被拒为太晚。

策略涵盖进程- 本地显示窗口、 SetForeground Window、 SetAactive Window、
SetFocus, SetWindowPos, 带Window ToTop 和创建WindowExA/W。 当游戏是时
已使用前置进程, 呼叫会通过。 背景为顶层
窗口被创建为隐藏和显示而不激活; 调用器扩展样式
保存。隐藏使用 SetWindowPos 并带有无线终端; 最小化使用
SHOWMINNATIONATION. 没有全局焦点锁,合成键盘输入或焦点恢复
循环已安装。 策略代码将一直装入到进程退出 。

用于一个明确的存储器测试, 设置 `GEARSVR_QUIET_STARTUP_BUILD` 仅在
发射装置的孩子环境到冷冻的建筑目录. `background_launch.py`
也接受 `--quiet-build`;它拒绝与 `--foreground`,验证
游戏安装并返回 PID/ 创建时间/ 路径, 然后将已有的带入
背景控制器帮助对游戏 PID。靴子陷阱 PID 与众不同。
取消使用经散列验证的冷冻静态选项11 构建。发射装置是
不编辑第三方设置或全局注册/运行时间设置。

命令构建器会留下简单的参数, 例如 `-NoPauseUnfocus` unquoted.
游戏的原始命令解析器仅接受字符串尾端、空格或标签后
切换,所以如果在带引号的切换周围设置,即使Windows
argv 解析成功。 需要引用的参数仍然使用 Windows 反斜线
escaping. `QuietCommandLineTest` 检查确切的原始开关和转弯;
游戏必须额外验证其内部命令行和行为。

与现有的 MSVC x 一起构建64 工具链和 CMake; MinHook 是现有的
`tools/third-party/minhook` 依赖性。在修改后使用新建目录。
本地固定器会关闭屏幕和退出, 同时观察创建, 延迟
激活、父/子的可见度、自有窗口、隐藏/最小化/恢复和
不变的扩展样式。 `check_fixture.py` 单独检查前景事件,
缺少策略拒绝和实际失败的孩子退出。它从来不会操纵
用户的键盘或鼠标。通过它不能证明游戏兼容性或手动操作
点击/ 选择标签行为 。

已知限制: TLS/ DLL 初始化发生在 EXE 条目之前; 此边界
无法保证没有发生先前的焦点请求。
显示器。在
创建回调。背景自动请求最大化使用不激活
普通显示; 手动前端最大值通过。 当前默认值
。不主张多人游戏或反热兼容性。

窗口参考 : [调试事件](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent),
[远程线程生命周期](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread),
[显示命令](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

发展证据被忽略 `analysis/runtime/quiet-startup*-*.json`.
此文件夹中没有专有快照、 分析数据库或伪代码
或放行包中。

仅限诊断 `GEARSVR_QUIET_TRACE=1` 记录第一个 128 API 电话
正在处理内存,包括被压制和通过呼叫,NULL目标,参数,
前景窗口/ 时间戳和原始返回地址。 它不包或日志
转到磁盘。 `GearsQuietTrace(void*)` 将已完成的记录复制到一个呼叫者手中
`TraceSnapshot` (37920 字节);其独立的ABI不更改 `Status`.
忙碌的空档被跳过, 溢出是明确的。 地址作为私人地址处理 。
运行时间证据,而不是发布内容.

追踪通过下列途径还观察到5个与激活有关的信息:
DefWindowProcW/A, 转发时不变。 这两个额外钩子没有了 。
。他们的记录描述信件发送,可以
激活后发生; 它们本身不识别请求的 API
或进程。实际的仅限游戏测试仍在复制前景激活,同时
八个保险钩都安装好了 安静保险钩仍然在试验中
