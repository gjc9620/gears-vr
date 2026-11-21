# Scoped background keyboard diagnostic

2026-10-17. Actual118 used the selected Native67 development build from [docs142](142-vrdof67-development-candidate.md). The ordinary-scene startup/output monitor passed: publication advanced from 0 to 3,342 during approximately 90 seconds. This was not a mounted-machine-gun test or headset acceptance.

The fresh process (PID 35620) had 151 reflected `PlayerInput.Bindings` entries, with a 28-byte stride. W mapped to `Game_MoveForward`; A/D/S mapped to strafe left/right and move backward. Left/Right contained only content-reader commands. No physical keyboard turn binding was found. Saved package defaults support `Game_TurnLeft`/`Game_TurnRight` aliases, but those defaults do not establish a physical binding in this live profile.

One W keydown/keyup pair was posted to the target game HWND for 0.3 seconds; keyup was recorded. Foreground HWND remained 789028. Physical background controller input remained disabled (`background_input` status `original`). The diagnostic used no ViGEm, global input, focus action or game-memory writes.

Pawn location was unchanged: baseline drift and distance after W were both 0.0. Thus gameplay movement was not demonstrated. This result does not distinguish undelivered input, blocked movement or focus gating. Scoped keyboard input is not currently an established autonomous navigation solution, and this run did not reach a Troika.

Normal game close, native cleanup and entry restoration were verified. All session processes exited and all four protected save/profile files were restored. The current user authorization permits unattended background game/simulator tests at any time; older no-launch preferences do not supersede it. Focus stealing remains forbidden and physical background gamepad input remains disabled.

Evidence under ignored `analysis/runtime/`:

- `vrdof118-window-keyboard.json`: live bindings, scoped key event and pawn samples.
- `vrdof118-audit.json`: startup/output monitoring (`passed=true`).
- `vrdof118-normal-close.json` and `vrdof118-save-restored.json`: cleanup and restoration.
- `continuous-stereo-35620-20261017-200011-e4eb5180.json`: session record.

---

## 中文

# 范围化背景键盘诊断

2026-10-17 实际118 使用选中的本地67 发展与发展 [docs142](142-vrdof67-development-candidate.md)。普通场景启动/输出显示器通过:出版物从 0 to 3,342 大约在 90 秒。这不是装机枪测试或耳机接收。

新的进程( PID) 35620) 曾经 151 reflected `PlayerInput.Bindings` 条目,带有 28- 字节步进,图到 `Game_MoveForward`; A/D/S映射到左/右和向后移动。左/右只包含内容阅读命令。没有找到物理键盘转弯绑定。保存的包默认支持 `Game_TurnLeft`/`Game_TurnRight` 化名,但这些默认在此活配置文件中并不建立物理约束.

目标游戏HWND上发布了一对W键盘/键盘配对 0.3 秒; 键盘被录制。 前景 HWND 仍然保留 789028 物理背景控制器输入仍然被禁用(`background_input` 状态 `original`),诊断没有使用ViGEm,全局输入,焦点动作或游戏记忆写作.

光线位置不变:基线漂移和W之后的距离均保持不变。 0.0。因此,游戏播放器运动没有被显示。此结果无法区分未交付的输入、被封杀的移动或焦点的显示。范围化键盘输入目前不是一个既定的自主导航解决方案,而且这一运行没有到达三驾马车。

正常游戏关闭、本地清理和条目恢复被验证。所有会话进程都退出,所有四个受保护的保存/配置文件都恢复。当前用户授权允许随时不参加背景游戏/模拟测试;更古老的不- 启动首选项不能取代它。焦点盗取仍然被禁止,物理背景游戏板输入仍然被禁用。

被忽略的证据 `analysis/runtime/`:

- `vrdof118-window-keyboard.json`:活绑,范围密钥事件和当事标本.
- `vrdof118-audit.json`: 启动/产出监测(`passed=true`).
- `vrdof118-normal-close.json` and `vrdof118-save-restored.json`:清理修复.
- `continuous-stereo-35620-20261017-200011-e4eb5180.json`: 会话记录.
