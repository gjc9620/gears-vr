# Game audio launch policy

Actual packaged-helper regression (2026-10-19): game PID 43444's automated run
matched and muted one exclusive game session, restored it once, and exited zero
with restore_pending/read_failed/mute_failed all zero. Receipt:
`analysis/runtime/vr-launcher-session.1f2201b2d8a84306b70176c9baa3c2ea.audio.jsonl`.
The UI navigation attempt itself failed; this receipt establishes only audio
mute/restoration. Manual 100% mode still has offline/policy evidence only.

`GEARSVR_TEST_MUTE_GAME=1` opts the repository launcher into per-process test muting.
Set it only on the automated test's child environment. It takes precedence over
the manual foreground launch policy. Manual foreground launches set each newly
observed exclusive game audio session to 100% and unmute it once; subsequent user
adjustments are preserved. Background launches without the test flag do not
change audio. No system/endpoint master, grouping, registry or other application's
volume is written. Game-internal sound settings are unchanged.

For the packaged helper, run `build_helper.py` from an x64 MSVC developer shell
with a new `--build-dir` and `--output-dir tools/game-audio`. Supply `--cmake` and
`--ninja` if they are not on PATH. The builder runs only pure policy tests and
produces the EXE plus `audio-helper-provenance.json`; it never runs an audio mode.
Existing output is refused. The launcher and package preflight verify the receipt
and EXE hash. Both files and the Python guard are included by the portable stager.

2026-10-19 integration: root compiled the helper and passed 117 Python tests plus
the native policy test. The runtime-resolution development stage with this helper
passed isolated bundled-Python/import/PE dependency preflight. These are offline
and package checks, not actual manual 100% audio acceptance. Root receipts are in
ignored `analysis/runtime/manual-audio-root-tests.json` and
`runtime-default-001-audio-preflight.json`.

Build with the existing Windows SDK/MSVC toolchain (no dependencies to install):

```powershell
cmake -G Ninja -S tools/game-audio -B analysis/runtime/test-audio-guard-build -DCMAKE_BUILD_TYPE=Release
cmake --build analysis/runtime/test-audio-guard-build --config Release
```

Use a single-configuration generator so `GearsTestAudioGuard.exe` is directly in
that directory; run in the existing x64 developer environment. `AudioGuardPolicyTest`
is a fake-session CPU test. `GearsTestAudioGuard --enumerate` performs read-only
Core Audio enumeration and prints counts without session names or identifiers.

The launcher starts the guard after successful reservation and exact executable /
PID / creation-time verification. This minimum integration does **not** mute the
initial reservation/intro window. The guard independently authenticates game and
launcher process creation times using retained process handles. It enumerates all
active render endpoints every 250 ms for at most six hours, handling later audio
sessions. Cross-process and system-sound sessions are excluded. No matching session
is recorded as zero matches, never as confirmed muting.

The first mute state and exact session COM object are retained before mutation.
Only `SetMute` is called. `SetMasterVolume` is never called, so user volume changes
are preserved. Normal stop, cancellation and startup exceptions signal a unique
stop event before existing prime cleanup. Game exit, owner exit and the bounded
deadline independently trigger restoration in the guard. The original session
objects survive until restoration attempts finish, including after process exit;
no replacement PID/session is selected for restoration. Failed restorations retry
for three seconds and remain a reported failure. Python waits four seconds and
never kills an audio guardian on timeout; existing native/prime cleanup continues.

JSONL counts distinguish matched sessions, successfully muted sessions, restore
attempts, successful restores and pending restores. Windows persists session mute
settings, so a failed restore is not success and must be inspected before another
player launch. An OS/audio-service failure or forcibly killing the guardian cannot
be claimed to restore audio. Offline policy/build tests and read-only enumeration
do not establish actual-game muting, delayed game-session behavior or persistence
after game exit; those require the coordinator's separate runtime test.

API references: [session persistence](https://learn.microsoft.com/en-us/windows/win32/coreaudio/audio-sessions),
[GetProcessId and cross-process rejection](https://learn.microsoft.com/en-us/windows/win32/api/audiopolicy/nf-audiopolicy-iaudiosessioncontrol2-getprocessid).

Actual game check (2026-10-18): session `continuous-stereo-35320-20261018-212357-b5ff9b15` matched and muted one game audio session; stop restored that same session, with restore_pending=0, read_failed=0 and mute_failed=0. Receipt: `analysis/runtime/vr-launcher-session.1bfb91ffd2374e8389066345a9da13e0.audio.jsonl`. The background game and audio helper exited. This validates audio mute/restore only: the combined UI test did not acquire its layout lease, and its native cleanup verification failed, so it must not be advertised as a successful VR regression.

---

## 中文

# 游戏音频发布策略

实际的包式帮助回归( E)2026-10-19: 游戏 PID 43444 自动运行
匹配并调音了一个独家游戏会话, 恢复一次, 退出 0
恢复/ 读取 _ 失败/ 输出_ 失败全部为零。 收到 :
`analysis/runtime/vr-launcher-session.1f2201b2d8a84306b70176c9baa3c2ea.audio.jsonl`.
UI 导航尝试本身失败; 此接收只建立音频
哑巴/修复。手动 100% 模式仍然只有离线/政策证据。

`GEARSVR_TEST_MUTE_GAME=1` 选择寄存器启动器为每个进程测试变换。
仅设定在自动测试的孩子环境上。 它优先于
手动前地发射政策。
观察到的独家游戏音频会话 100% 并解密一次; 后续用户
调整被保留。没有测试旗的背景发射不
更改音频。没有系统/端点主控器、组、注册或其他应用程序的
音量为写入。游戏内部声音设置不变。

对于已包装的帮助, 请运行 `build_helper.py` 从 x 中64 MSVC 开发者 shell
带有新内容 `--build-dir` and `--output-dir tools/game-audio`供应 `--cmake` and
`--ninja` 建筑商只进行纯政策测试,
生成 EXE + `audio-helper-provenance.json`; 它从未运行过音频模式。
拒绝现有的输出。 发射装置和包件在飞行前验证接收
和 EXE hash。文件和Python 守护器都包含在便携式舞台机中。

2026-10-19 整合: root 编译了帮助器并获得通过 117 活字测试加
本地政策测试。使用此帮助的运行时间解析开发阶段
已通过隔离捆绑- Python/ 进口/ PE 依赖性飞行前。这些已离线
和包检查,不是实际手工 100% 音频接收。根收据在
ignored `analysis/runtime/manual-audio-root-tests.json` and
`runtime-default-001-audio-preflight.json`.

与现有的 Windows SDK/MSVC 工具链构建( 没有依赖性可安装) :

```powershell
cmake -G Ninja -S tools/game-audio -B analysis/runtime/test-audio-guard-build -DCMAKE_BUILD_TYPE=Release
cmake --build analysis/runtime/test-audio-guard-build --config Release
```

使用单配置生成器 `GearsTestAudioGuard.exe` 直接输入
该目录; 运行于现有 x64 开发者环境. `AudioGuardPolicyTest`
是一个假的 CPU 测试。 `GearsTestAudioGuard --enumerate` 仅执行读取
核心音频计数和打印计数,没有会话名称或标识符.

发射装置在成功保留和精确可执行文件后启动警卫 /
PID / 创建时间验证。 此最小集成效果 **not** 安静
初始预定/ 内置窗口。 守护者会独立认证游戏和
使用保留进程处理的启动器进程创建时间。 它列出所有
活动将结束点转换为每 250 最多6小时,处理以后的音频
。不包含跨进程和系统声音会话。没有匹配会话
记录为零匹配,从未确认为变种。

第一个变异状态和准确会话COM对象在变异前保留.
Only `SetMute` 被叫来。 `SetMasterVolume` 从未调用, 因此用户音量会改变
。正常的停止、取消和启动例外表示一个独特的
在当前质清除前停止事件。游戏退出、所有者退出和限制
初始会话
物体在恢复努力完成之前,包括在进程退出之后;
没有选择要修复的 PID/ 会话替换。 失败的修复重试
3秒后继续报告失败。 Python 等待4秒后
永远不要在超时杀死音频守护者;现有的本土/初级清理工作仍在继续。

JSONL 计数区分匹配的会话, 成功调减会话, 恢复
尝试、 成功恢复和待恢复。 Windows 坚持会话哑弹
设置, 因此失败的恢复不会成功, 必须先检查另一个
操作系统/自动操作服务失败或强行杀害监护人不能
。离线策略/构建测试和只读的计数
不确定实际游戏变异、延迟游戏会话行为或持久性
游戏退出后;这些需要协调员单独运行时间测试.

API 参考文献 : [持续会话](https://learn.microsoft.com/en-us/windows/win32/coreaudio/audio-sessions),
[获取进程和跨进程拒绝](https://learn.microsoft.com/en-us/windows/win32/api/audiopolicy/nf-audiopolicy-iaudiosessioncontrol2-getprocessid).

实际游戏检查( E)2026-10-18: 会话 `continuous-stereo-35320-20261018-212357-b5ff9b15` 匹配并小声了一个游戏音频会话; 停止恢复同一会话, 恢复_ 等待=0,读出_失败=0 哑音(_P):0。收件日期: `analysis/runtime/vr-launcher-session.1bfb91ffd2374e8389066345a9da13e0.audio.jsonl`。背景游戏和音频帮助退出。这只验证音频哑弹/重存:合并的UI测试没有获得其布局租赁,其本土清理验证失败,因此它不能被公示为成功的VR回归.
