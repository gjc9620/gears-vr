# 后台 Xbox 输入与启动入口

2026-10-15。用户真实手柄观察：游戏失焦后角色不移动，回到前台才移动。本轮复用 SQLite、缓存汇编和伪代码，定位控制器处理的焦点门限，并实现独立、可恢复的运行时修改。没有新增 IDA 会话、全库分析或工具安装。

## 当前结果

普通游戏和 VR 的共用 `tools/debug-launch/Start-DebugGame.ps1` 默认启动 `background_input.py enable`。它在游戏完成代码初始化后核验版本与完整输入函数，再修改两个控制器分支。保留 `-NoPauseUnfocus`，游戏窗口采用 `SW_SHOWNOACTIVATE` 初始显示方式，helper 隐藏运行。

此默认只覆盖项目启动入口（包括 VR GUI、SimulatorVR、Start-DebugGame），直接双击原始游戏 EXE 不经过该逻辑。没有修改磁盘 EXE，也没有安装全局常驻服务。游戏退出后修改自然消失，VR 停止后仍保留后台控制器能力，符合用户“普通游戏也默认启用”的选择。

初始显示标志不能保证游戏后续不会自行激活。第一次启动观察确实出现游戏前台事件，因用户随后表示可能知道原因并要求暂停这部分调查，本轮没有加入激活 API Hook。窗口最小化、多人模式、真实头显本轮未验收。

## 来源与修改范围

- 磁盘 EXE SHA256：`bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。
- 知识库 artifact 1 为 PID16812 的历史运行时代码节；节 SHA256：`911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`；历史模块基址 `0x140000000`。
- 输入函数 RVA `0x19ab10`，长度 `0x147d`，完整字节 SHA256 `acab8014abe6fe624b697c25bda4ddcdf2d9ac4be1b90a50a67f950e1a851adb`。当前 PID17996 的完整函数在修改前匹配该哈希；没有通过 RVA 相同推定版本相同。
- 缓存证据：`analysis/ida/input-routing-20261014/0x14019ab10.asm.json`、`analysis/re-kb/full-native/pseudocode/14019ab10.c`。缓存伪代码存在 raw-loader 地址/参数推断问题，因此具体分支以汇编与完整字节校验为准。
- `0x19afc6` 检查 viewport `+0x4ec` 以及 client 虚函数；`0x19afe9` 比较活动 HWND 与 viewport `+0x90`。失败会跳过 `0x19aff6..0x19bd12` 控制器处理。
- 修改 RVA `0x19afcd` 的条件跳转为无条件进入控制器区块，移除 RVA `0x19aff0` 的窗口不等跳转。保留实际 HWND、前面的鼠标路径、后面的键释放清理和 viewport tick。
- 当前进程的 IAT 只读核验确认：输入开头根据条件调用 `GetForegroundWindow` 或 `GetFocus`，末尾使用 `GetActiveWindow`。记录为 `analysis/runtime/background-iat-17996.json`；没有修改这些 IAT 项。

修改工具绑定 PID/创建 FILETIME、EXE 哈希、主模块路径和完整函数字节。写入期间暂停并确认进程线程集合稳定，拒绝指令指针正落在补丁范围的情况；随后恢复内存保护、刷新指令缓存、读取验证并恢复线程。异常执行回滚，不自动循环重做写入。停用只恢复有匹配所有权日志的补丁。原始/启用/未知状态相互区分，未知代码不写。

## 实际游戏对照

PID17996，创建 FILETIME `134365480324417239`，模块基址 `0x140000000`。游戏以 `-NoPauseUnfocus` 运行，后台内存菜单导航进入关卡。测试使用短时 ViGEm Xbox360 设备，真实手柄保持连接；独立 XInput 读取确认实际虚拟设备在 slot1。驱动报告 slot0 不准确，前两次测试因此中止并保留记录；后续通过连接前后槽位差集及轴值回读确认设备。

| 操作 | 补丁启用 | 恢复原代码 | 再启用 |
| --- | --- | --- | --- |
| 右摇杆约 0.65 秒 | yaw +29.421° | yaw 0° | 反向 -28.987° |
| 左摇杆约 0.65 秒 | 角色前移 67.028 单位 | 位移 0 | 后退 51.168 单位 |

每段采样确认游戏没有前台焦点，OS 确实收到摇杆输入，摄像机时间继续推进，动作后手柄回中。人物位移来自反射出的 Pawn.Location，未把第三人称镜头绕人物转动当作角色移动。

证据：`analysis/runtime/background-controller-17996-attempt3.json`、对应 `.summary.json`、`background-movement-17996.json`、`background-input-17996-134365480324417239.json`。后台恢复原代码使输入失效，再启用后恢复响应，支持该区块的焦点过滤是本轮后台输入阻塞点。

真实手柄被动采样 90 秒、2,696 条均在后台，未捕获超过阈值的摇杆操作，因此该采样本身不构成真实手柄操作验收。用户随后先回复“仍然不响应”，只读复查确认同一 PID 的补丁仍启用、真实 XInput slot0 已连接并分配给玩家0、移动/视角忽略计数均为0、游戏仍在后台；证据为 `analysis/runtime/background-real-failure-17996.json` 和 `background-real-failure-menu-17996.json`。用户接着明确反馈“可以了”，据此记录真实手柄后台响应通过用户确认。两次反馈之间没有进一步修改代码或游戏状态，暂不推断之前未响应的原因；此用户确认不等于已同步采到真实摇杆与角色运动的完整数值轨迹。首次用窗口 Enter 进入关卡的手柄仍可能需要一次正常按钮输入认领玩家，这与焦点过滤是两件事。

## 使用与恢复

普通游戏启动：

```powershell
powershell -NoProfile -File J:\gearsVr\tools\debug-launch\Start-DebugGame.ps1
```

下次启动不应用后台手柄补丁：增加 `-DisableBackgroundInput`。恢复原来的初始窗口显示策略：增加 `-Foreground`；它与手柄开关独立。`-PauseOnUnfocus` 会恢复游戏自身的后台暂停策略，因此后台输入不保证能推进角色。

当前进程检查/恢复（换成本次 PID）：

```powershell
py -3 J:\gearsVr\tools\game-io\background_input.py status --pid 17996
py -3 J:\gearsVr\tools\game-io\background_input.py disable --pid 17996
```

日志保存在 ignored `analysis/runtime/background-launch-<pid>.json`、`background-input-<pid>.stdout.log` / `.stderr.log`、包含创建时间的补丁账本。helper 成功或等待上限结束即退出，不持续抢焦点或发送输入。启动器不因补丁拒绝而强杀游戏；必须查看 stdout 中的 `state=enabled`，不能把“已启动 helper”当作“补丁已生效”。

离线验证：补丁 7 项（完整字节拒绝、重复启用、恢复、部分失败回滚、身份变化、PID 复用、无所有权拒绝、线程暂停失败清理）；后台启动 5 项；原安装验证 6 项、VR launcher 40 项通过。实际游戏验证包括相机和 Pawn 移动对照；本轮未重新启动 VR 或验收 GPU/头显行为。启动 helper 的自动新进程整链仍须在下一次正常启动检查日志。
