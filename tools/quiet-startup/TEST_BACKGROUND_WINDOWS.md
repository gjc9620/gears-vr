# Automatic-test window placement

User operating preference (2026-10-19): background game and simulator tests are
authorized at any time; deferring a physical-headset test does not defer these
tests. Keep audio muted and windows behind the user's applications. Do not
activate or cover their current window. For unattended resolution tests, use a
temporary 1280x720 windowed desktop setting, with exact pre-test settings/profile
backup and restoration after the owned game exits. Do not change the normal
player launch configuration or infer the VR eye dimensions from this window.

This opt-in policy keeps the game's own automatically shown windows behind
existing windows. It does not minimize, resize, move or activate windows, and
does not poll or repeatedly repair desktop Z order. A user's foreground game
window retains the existing behavior.

For an automatic test, set these variables in the test's child environment:

```text
GEARSVR_TEST_WINDOWS_BEHIND=1
GEARSVR_QUIET_STARTUP_BUILD=J:\gearsVr\tools\quiet-startup\build-test-behind2
```

The explicit directory must contain a successfully built `GearsEarlyLaunch.exe`
and `GearsQuietStartup.dll`, plus `test-background-policy.json`. The proof must
use schema 1, policy `hidden_bottom_showwindow_v2`, and a `sha256` object containing
the matching hashes of both artifacts. Missing or mismatched artifacts reject
launch before spawning. The proof is a local build receipt, not a signature
authenticating an untrusted build.

New visible windows are created hidden, then shown and placed at `HWND_BOTTOM`
in one `SetWindowPos` call using `SWP_SHOWWINDOW | SWP_NOACTIVATE`. No later
`ShowWindow` call follows this operation. A failed initial show leaves the
window hidden. Subsequent show/position requests from the background game keep
the same opt-in placement policy; there is no desktop watchdog. The original
quiet11 default directory and hashes are unchanged. `--foreground` retains its
ordinary behavior.

`BackgroundPolicyTest` is a pure C++ test with injected callbacks; it creates no
window and calls no window-management APIs. CMake applies `/UNDEBUG` to this
target so Release assertions remain enabled. Run it with:

```text
ctest --test-dir tools/quiet-startup/build-test-behind2 -R "^BackgroundPolicyTest$" --output-on-failure
```

The combined actual-run check below supplements the offline/mock validation;
it does not establish every startup instant or desktop layout. The simulator
preview requires its own runtime-side policy; this game startup DLL does not
control windows owned by the OpenXR probe process.

The matching simulator candidate is described by
`tools/openxr-probe/simulator-quiet/test-behind-atomic-provenance.json`. For local
automatic simulator tests, explicitly select
`J:\gearsVr\analysis\simulator-behind-atomic-source\bin\openxr_simulator.json` using the
launcher's runtime argument (or `XR_RUNTIME_JSON` in the probe child environment),
alongside the two variables above. Do not change the system ActiveRuntime.
The preview is owned by the OpenXR probe process, not a separate simulator.exe.
Both policies must be selected: using the old quiet2 runtime would leave preview
placement unchanged even with the new game startup DLL. The candidate suppresses
showing if initial placement fails; it never uses a periodic desktop watchdog.

Keep `GEARSVR_TEST_MUTE_GAME=1` and
`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1` on autonomous test children. Do not set
these variables globally or on normal player launches. Do not fall back to the
old startup/runtime pair if the test candidates or their hashes are unavailable.

## Actual combined check (2026-10-18)

`analysis/runtime/path-fit-runtime9-20261018.json` used both candidates and the
explicit test flags. The game and OpenXR probe ran, and XR reported projection
submissions. A read-only Z-order walk sampled 785 times after owned identities
were available; no visible owned game/probe window appeared above the current
foreground window. This is sampled evidence, not proof of every startup instant
or every multi-monitor layout. The check made no focus, move or resize requests.

The longer UI hold check stopped on an unverified native-status read, so the
whole VR regression did not pass. Native cleanup, independent entry restoration,
normal game close and five settings/profile restores did pass. This outcome
supports the new placement policy only within the sampled interval; the status
read failure remains separate work. The system runtime registry was not changed.

## Coverage limitation and expanded offline detector

The foreground-only walk above can miss a test window covering another visible
application on a second monitor or below the foreground window. Its zero count
must not be described as proof that no user windows were covered.

`window_occlusion.py` analyzes a supplied top-to-bottom snapshot across monitors,
subtracting rectangles already covered by higher windows. It reports test-window
areas above other visible applications, including negative screen coordinates.
Hidden, minimized and cloaked windows are excluded. It neither reads window
titles nor moves, activates or reorders anything. Explicit shell HWND exclusions
belong to the snapshot caller; do not exclude ordinary applications by guessing.

Five offline regression tests cover the foreground-only blind spot, partial and
complete coverage, invisible windows, and nonoverlap. Run with Python unittest
discovery in this directory using pattern `test_window_occlusion.py`.
`window_snapshot.py` supplies a bounded read-only Win32/DWM snapshot, with no
window title collection. It checks HWND process/thread identity and repeats the
Z-order walk; query failure or order changes invalidate the sample. Five further
mocked tests cover bounds, failed queries, identity/order races and the traversal
limit. The unexecuted private `analysis/runtime/dlss-native-menu5.py` driver now
uses both helpers at 250 ms intervals after owned identities are available,
requesting owned-test cancellation on occlusion or invalid observations. It does
not repair desktop Z order. Startup before identity discovery remains a gap.

These tests verify only the snapshot logic and rectangle analysis, not live
placement or the cause of the user's reported
occlusion. Live snapshots can race window changes and rectangles do not model
transparent pixels or irregular regions. No new game launch is justified by
these offline results alone.

## 2026-10-19: actual USER32 cause isolated without using the input desktop

`private_desktop_show_test.cpp` creates a private desktop, attaches only its own
thread, and destroys its own fixture windows before restoring that thread. It
never calls SwitchDesktop or addresses windows on the user's desktop. Failure
to create/attach the private desktop stops the fixture without a fallback.

On this machine, bottom-then-SW_SHOWNA raised the test window above the existing
fixture in all four cases (ordinary/topmost creation, ShowWindow/ShowWindowAsync).
A single SetWindowPos bottom+show operation kept it below in all four controls.
There were zero API/setup errors. This demonstrates the ordering defect in the
old API sequence; it does not prove all actual game/preview lifecycle paths.
The asynchronous case uses the same window thread and does not certify cross-
thread dispatch timing. The test stays opt-in, outside ordinary CTest execution.

The new game build passes both registered C++ tests; all 24 background-launch
Python tests pass, including rejection of the old v1 receipt. Ordinary launcher
behavior remains unchanged. The simulator's incremental atomic patch passes its
five registered tests and preserves the default show callback. No game or XR
session was started for this revision. Manual game play and system runtime
selection were not modified.

The v2 protocol uses ShowWindow's prior-visibility return semantics for intercepted
synchronous shows; asynchronous shows return the positioning API acceptance.
See Microsoft's [SetWindowPos reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
and [ShowWindow reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

### First actual v2 attempt: cancelled before VR activation

The `dlss-native-menu5` run reached reservation, then its new snapshot guard
cancelled before VR activation because the desktop contained 637 top-level
windows, exceeding the original 512 traversal limit. This was not a recorded
occlusion. The prime entry restoration was verified; the game subsequently
exited and all five settings/profile backups were restored. No DLSS pair or
actual combined game/preview layering acceptance was obtained.

Traversal now has a separate 4096-handle bound and a 256-visible-window bound.
Cycle, traversal overflow and visible overflow have distinct errors. Hidden,
minimized and cloaked windows do not consume the visible budget. Thirteen
snapshot/rectangle tests pass, including a 637-window/40-visible fixture. A
subsequent read-only desktop capture completed with 637 traversed and 24 visible
non-cloaked windows. These numbers are a sample, not a fixed desktop assumption.

### Actual v2 menu6 interval

The subsequent `dlss-native-menu6` run completed its bounded native graphics-menu
route and returned to the game UI. Its 527 cross-window snapshots reported no
occlusion or invalid samples; no actual game-foreground sample was recorded.
This supports v2 during the sampled interval only, not every startup instant or
all desktop layouts. Native cleanup, independent entry restoration, normal game
close and five settings/profile restores passed. No GPU projection was published
by this observer candidate, so this is not visual or DLSS acceptance.

---

## 中文

# 自动测试窗口设置

用户操作偏好( E)2026-10-19:背景游戏和模拟测试为
任何时间都允许; 推迟物理头套测试不推迟这些测试
测试。在用户应用程序后保留音频变音和窗口。不要
激活或覆盖当前窗口。对于未覆盖的解析测试,请使用 a
temporary 1280x720 窗口桌面设置, 并带有精确的预测试设置/配置
返回和修复自有游戏退出后。不要更改正常
玩家的发射配置或从此窗口推断 VR 眼尺寸。

此选择进入政策将游戏本身自动显示的窗口保留在后面
。它不会最小化、调整大小、移动或激活窗口,以及
不投票或反复修复桌面 Z 命令。用户的前置游戏
窗口保留已有的行为。

对于自动测试,将这些变量设置在测试的孩子环境中:

```text
GEARSVR_TEST_WINDOWS_BEHIND=1
GEARSVR_QUIET_STARTUP_BUILD=J:\gearsVr\tools\quiet-startup\build-test-behind2
```

明确目录必须包含一个成功构建的 `GearsEarlyLaunch.exe`
and `GearsQuietStartup.dll`附加 `test-background-policy.json`。必须证明
使用计划 1 政策 `hidden_bottom_showwindow_v2`,并设置a `sha256` 包含对象
两种文物的匹配散列。
证明是本地的建筑收据,不是签名
认证一个不可信建筑。

新建可见窗口后隐藏, 然后显示并放置在 `HWND_BOTTOM`
在一个中 `SetWindowPos` 调用 `SWP_SHOWWINDOW | SWP_NOACTIVATE`。 以后不作
`ShowWindow` 调用此操作。 初始显示失败后, 将离开
窗口隐藏。背景游戏保存的随后显示/位置请求
相同的选择入选设置政策; 没有桌面监视器。 原始
quiet11 默认目录和散列不变。 `--foreground` 保留其
普通行为

`BackgroundPolicyTest` 是一个纯 C++ 测试,带有注入的回调; 它创建了无
。 CMake 应用程序 `/UNDEBUG` 给这个
目标,所以仍然启用“释放”断言。运行时使用:

```text
ctest --test-dir tools/quiet-startup/build-test-behind2 -R "^BackgroundPolicyTest$" --output-on-failure
```

以下实际运行的综合检查补充了离线/模拟验证;
它不建立每一个启动即时或桌面布局。模拟器
预览需要自己的运行时间侧政策; 此游戏启动 DLL 不
OpenXR 探测程序拥有的控制窗口。

匹配的模拟候选人描述于
`tools/openxr-probe/simulator-quiet/test-behind-atomic-provenance.json`。用于本地
自动模拟测试, 明确选择
`J:\gearsVr\analysis\simulator-behind-atomic-source\bin\openxr_simulator.json` 使用
发射器运行时参数(或) `XR_RUNTIME_JSON` 探险儿童环境中的,
与上面的两个变量并列。不要更改系统活动运行时。
预览由OpenXR探测器进程拥有,而不是单独的模拟器.exe.
两种政策都必须选择:2 运行时间会预览
即使与新游戏启动 DLL 一样, 设置不变。 候选者会压制
显示初始放置失败; 它从不使用周期性桌面监视器 。

Keep `GEARSVR_TEST_MUTE_GAME=1` and
`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1` 。不设置
这些变量在全球范围或正常玩家启动时。不要返回到
如果测试对象或其散列时间不可用,则使用旧的启动/运行时间对。

## 实际合并核对2026-10-18)

`analysis/runtime/path-fit-runtime9-20261018.json` 候选人和候选人的任职情况
明确测试旗。 游戏和 OpenXR 探测器运行, XR 报告投影
提交。仅读 Z 顺序行走样本 785 时间,在拥有身份之后
可用; 没有可见的自有游戏/ 探测窗口出现在当前之上
前景窗口。 这是抽样证据, 不是每个启动瞬间的证据
或每个多监视器布局。检查没有突出焦点、移动或调整大小请求。

用户界面检查时间越长,未核实的本地身份就停止了,所以
整个 VR 回归没有通过。
普通游戏关闭, 5个设置/ 配置恢复已通过。 此结果
仅在抽样间隔内支持新的放置策略; 状态
读出失败仍然是单独的工作。系统运行时间登记册没有更改。

## 覆盖范围限制和扩大离线探测器

上面仅面向前景的行走会错过覆盖另一个可见的测试窗口
应用到第二个显示器或前台窗口下方。它的零数
不得将它描述为没有覆盖用户窗口的证据。

`window_occlusion.py` 分析一个从上到下提供的快照 横跨显示器,
减去已覆盖在更高窗口的矩形。 它报告测试窗口
区域高于其他可见应用程序,包括负屏幕坐标。
隐藏、最小化和隐形窗口不在此列。 它既不读取窗口
标题或移动、激活或重排任何内容。明确 shell HWND 排除
属于快照呼叫器; 不排除通过猜测的普通应用程序。

5次离线回归测试覆盖了前场仅盲点,部分和
覆盖、 隐形窗口和不重叠。 运行 Python 单元测试
使用图案在此目录中发现 `test_window_occlusion.py`.
`window_snapshot.py` 提供有边框的只读赢32/DWM 即时显示,没有
窗口标题收藏。它检查 HWND 进程/线索身份并重复
Z 命令行走; 查询失败或命令更改使样本无效。 另5个
模拟测试覆盖界限、失败的查询、身份/顺序赛和逆变
限制。未执行的私人 `analysis/runtime/dlss-native-menu5.py` 驱动程序
使用两个助手, 时间为 250 在拥有身份后间隔 ms ,
请求在隐匿或无效的观察中取消自有测试。它确实
无法修复桌面 Z 命令。 在身份发现前启动仍然是个漏洞 。

这些测试只验证快照逻辑和矩形分析,而不验证活性
放置或用户报告的原因
封存。 现场快照可以比赛窗口变化, 矩形不能建模
透明像素或不规则区域。没有新的游戏发布理由
仅这些离线结果。

## 2026-10-19 实际使用费32 无需使用输入桌面就导致孤立

`private_desktop_show_test.cpp` 创建私有桌面, 只附加它自己的桌面
线程,在恢复线程之前先摧毁它自己的固定窗口。
用户桌面上从未呼叫 SwitchDesktop 或地址窗口。失败
创建/附设私有桌面,可以不退缩地停止固定。

在这部机器上,当时的SW_SHOWNA将测试窗口提升到现有之上
所有四种情况下的固定式(普通/最顶级创建,Show Window/ShowWindowAsync).
在所有四个控件中,一个SetWindowPos底层+显示操作将其保留在下.
API/ 设置错误为零。 这说明命令的缺陷
旧的 API 序列; 它不证明所有实际的游戏/预览生命周期路径 。
同步大小写使用相同的窗口线程,而不验证交叉-
线程调度时间。 测试会保留选择性, 常规的 CTest 执行之外 。

新游戏的构建通过两个注册的 C++ 测试; 所有 24 background-launch
包括拒绝旧V1 普通发射器
行为保持不变。模拟器的增量原子补丁通过
5 个注册测试并保存默认的显示回调。 没有游戏或 XR 。
开始此修订版的会话。 手动游戏游戏和系统运行时间
选择没有修改。

维基百科中的相关条目: 维基百科中的相关条目: 维基语录链接:名人名言 - 文学作品 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语 - 谚语2 协议使用 ShowWindow 的先前可见返回语义来截取
同步显示; 同步显示返回定位 API 接受度 。
见微软 [设置窗口选项引用](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
and [显示窗口引用](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

### 第一实际 v2 尝试: VR 激活前已取消

The `dlss-native-menu5` 跑步到达预定地点,然后是新的快照守卫
VR 激活前已取消, 因为桌面包含 637 top-level
窗口,超过原窗口 512 横跨限制。 这不是记录
封禁。 质值条目恢复已被验证; 游戏随后
退出, 并恢复所有五个设置/ 配置备份。 没有 DLSS 对称或
实际获得合并游戏/审查层接受。

特拉弗斯现在有另外一条路 4096- 手绑着,一个 256- 隐形窗绑
循环、横溢和明显的溢出都有明显的错误。
最小化和隐形窗口不会消耗可见的预算。
短片/反面测试合格,包括 a 637-window/40- 隐形固定 A
之后只读桌面抓取完成 637 过路和 24 visible
。这些数字是一个样本,而不是一个固定的桌面假设。

### 实际 v2 menu6 interval

其后的 `dlss-native-menu6` 运行已完成其本地图形的限定菜单
路径并返回游戏 UI. Its 527 跨窗口快照报告无
封存或无效样本;没有记录实际的游戏前样.
此支持 v2 仅标出样本间隔期间,并非每个启动瞬间或
所有桌面布局。原生清理、独立条目恢复、普通游戏
关闭和5个设置/ 配置恢复通过。 没有发布 GPU 投影
由这个观察者候选人,所以这并非视觉或DLSS接受.
