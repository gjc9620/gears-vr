# Native Rendering Point yaw rewrite validation:2026-10-11

## Result

The player has observed the game in a static manner after reopening, entering the level, confirming that it does have “swing” and noting that it feels different from the normal mouse-to-hand control view. Logs confirm that temporary rendering is modified and source point and camera cache yaw remain unchanged; this round has verified that the yaw rewrite of the original rendering path changes the actual image.

This is not a complete VR: The OpenXR position has not been accessed, the pitch/ roll/ location modification, first person lens or left and right eye independent drawings are not verified. Input, aim, HUD, pass, other levels still need to be specifically validated.

## Realization of principles

Continue `0x15cf08` scene view function and `0x15c968` The Player View Event Packer function is Hook. The scene view Hook saves the current LocalPlayer and the rotation output pointer with a local line variable. After the Player View Event is called to the original function, changes are allowed only if:

- Call point from scene view, return position to `0x15d0ca`。
- The current scene view of the thread is for the current player to be re-deconciled, and the rotation output pointer corresponds to that view.
- The controller matches, and the LocalPlayer.Actor link to Controller.PlayerCamera is still valid.
- The game has a front-stage focus, collection is still running and testing is not completed or terminated.

Only the temporary rotation output yaw that is called will be offset without writing to the CameraCache or controller. Then lets the original scene view logic generate view arrays, projectors and derivative data. Other callers do not receive this test deviation when reading a player view point.

Start by observing the CameraCache time advance and avoiding running out of testing in a pause menu that has not yet been restored. Time is calculated in QPC: 3 Seconds to zero, then 12 Seconds to be 4 Sine swing of seconds, max 1.5°, and then stop permanently. The loss of focus or the invalidation of object relationships during operation will terminate this swing and will not automatically start again.

The normal mouse input passes through the game's third person-to-person lens and control logic; this round changes only the orientation of the luminum. The swing felt by the player is different, and corresponds to the test design, but does not specifically verify whether all control inputs are completely unaffected by visual change.

## Evidence

The target game version Hashi is the same as before, the new PID is 15712. This time you will find objects independently from the locator, without using the old PID. The current LocalPlayer is `0x86bc51a0`, the Controller is `0x897b2ae0`; these addresses describe only this experiment.

| Indicators | Result |
| --- | --- |
| scene view call | 4232 |
| Present Call | 4232;failed HRESULT as 0 |
| An event with a bias point of view | 720 |
| A scene view to apply deviation | 720 |
| Caller for Modify Event | All 0x15d0ca |
| Other callers modified | 0 |
| Modified without focus | 0 |
| Offset Range | -273 To +273 Rotation units, approximately  ⁇1.4996° |
| Actual non-zero offset record margin | 11.98357 sec |
| Original Event yaw | Always. 22344 |
| CameraCache yaw while the change is under way | Always. 22344 |
| Different Matrixs during Modifying | 251 |
| Event output does not match source yaw plus offset | 0 |
| Final Test Status | 2: Complete end |

The original function still provides the original point of view, while the current luminum dyes direction and matrix changes, which support each other by the image swing observed by the user. This does not mean that role positions are completely static: the log still has a change in the position of the primary camera, and no additional shift is added to this round.

This is the one that's been thrown away. 1 bar queue record, not a no-lose track. The master view corresponds to the Present count and does not match one or more records. The medium interval between focus views is called during focus periods 16.653 Ms, Precent 16.6576 ms。

Stop state is `disable_status=0 active=0`; read and check only the entry bytes of both game functions after stopping, and the camera is still positioned. DLL and Trumpoline remain in place and release after exiting the game.

The independent test first validates the original function forwarding, re-entrying, then matching scenes are offset, independent event calls keep the original output and time boundary returns zero; both compile and self-measurement are passed. The extra external read-only sampler for this round has no catch test window, and is cancelled and not considered as proof of validation.

Original evidence is stored locally `analysis/runtime/native-wobble-1.csv`、`.json`、`.status.txt`Don't go into Git.

## Use and Next

Build `build.bat build-wobble`, run with a new game process `start_capture.py --pid <当前PID> --seconds 180 --build-dir <build-wobble绝对路径> --wobble`. Notify players to resume games, static observation until the collection is started 25 sec. Default not available `--wobble` It's still only collected.

`stop_capture.py --pid <当前PID>` Request to cancel the entrance patch in advance, then use `collect_capture.py --build-dir <同目录> --output <分析JSON路径>` Collect. Stopping the request does not in itself mean that it has been successfully withdrawn, and must check the state and access bytes.

The next step is to do an independent validation of the OpenXR running, head-over and coordinate axes, then rotate the relative head to this rendering entrance. Still have to process the combination of initial orientation, repositioning and original game vision points. The horizontal, stereo-rendering and special lens paths are accepted separately; the current round does not prove that it can maintain a head response when the real script VM stops.

---

## 中文

# 原生渲染视点 yaw 改写验证：2026-10-11

## 结果

玩家在重开游戏、进入关卡后进行了静止观察，确认“确实有摆动”，并指出它与普通鼠标左右控制视角的感觉不同。日志确认临时渲染输出被修改，源视点和摄像机缓存 yaw 保持不变；本轮已经验证原生渲染路径的 yaw 改写能改变实际画面。

这不是完整 VR：尚未接入 OpenXR 位姿，未验证 pitch / roll / 位置修改、第一人称镜头或左右眼独立绘制。输入、瞄准、HUD、过场、其他关卡仍需专门验证。

## 实现原则

沿用 `0x15cf08` 场景视图函数与 `0x15c968` 玩家视点事件包装函数的 Hook。场景视图 Hook 用线程局部变量保存当前 LocalPlayer 和旋转输出指针。玩家视点事件调用原函数后，只有同时满足以下条件才允许修改：

- 来自场景视图调用点，返回位置为 `0x15d0ca`。
- 当前线程的场景视图属于重新解析的当前玩家，旋转输出指针与该场景视图一致。
- 控制器匹配，LocalPlayer.Actor 与 Controller.PlayerCamera 链接仍有效。
- 游戏拥有前台焦点、采集尚在运行，测试没有完成或终止。

只给该调用的临时旋转输出 yaw 加偏移，不向 CameraCache 或控制器旋转字段写入。随后让游戏原有场景视图逻辑生成视图矩阵、投影矩阵和派生数据。其他调用者读取玩家视点时不会得到这个测试偏移。

启动需先观察到 CameraCache 时间推进，避免在尚未恢复游戏的暂停菜单里用完测试。时间以 QPC 计：前 3 秒偏移为零，随后 12 秒做周期为 4 秒的正弦摆动，最大 1.5°，之后永久停止。运行期间失去焦点或对象关系失效会终止本次摆动，不会自动再开始。

普通鼠标输入会经过游戏的第三人称镜头和控制逻辑；本轮只改变本帧渲染朝向。因此玩家感受到的摆动方式不同，与该测试设计相符，但并未专门验证所有控制输入是否完全不受视觉变化影响。

## 证据

目标游戏版本哈希与此前一致，新 PID 为 15712。本次由定位器独立查找对象，不使用旧 PID 的对象地址。当前 LocalPlayer 为 `0x86bc51a0`，Controller 为 `0x897b2ae0`；这些地址仅描述本次实验。

| 指标 | 结果 |
| --- | --- |
| 场景视图调用 | 4232 |
| Present 调用 | 4232；失败 HRESULT 为 0 |
| 施加偏移的视点事件 | 720 |
| 施加偏移的场景视图 | 720 |
| 修改事件的调用者 | 全部为 0x15d0ca |
| 其他调用者被修改 | 0 |
| 无焦点时被修改 | 0 |
| 偏移范围 | -273 到 +273 个旋转单位，约 ±1.4996° |
| 实际非零偏移记录跨度 | 11.98357 秒 |
| 原始事件 yaw | 始终 22344 |
| 修改期间 CameraCache yaw | 始终 22344 |
| 修改期间不同矩阵组 | 251 |
| 事件输出与源 yaw 加偏移不符 | 0 |
| 最终测试状态 | 2：完整结束 |

原函数仍提供原始视点，而本帧渲染朝向和矩阵变化，与用户观察到的画面摆动相互支持。这不意味着角色位置完全静止：日志仍有原生摄像机位置变化，本轮没有添加位置偏移。

本轮丢弃 1 条队列记录，不是无损追踪。总视图与 Present 计数恰好相等也不能证明每条记录一一对应。焦点期间视图调用间隔中位数 16.653 ms，Present 约 16.6576 ms。

停止状态为 `disable_status=0 active=0`；停止后只读核验两个游戏函数入口字节均恢复，摄像机定位仍通过。DLL 和 trampoline 仍驻留，退出游戏后释放。

独立测试先验证原函数转发、入口恢复，再验证匹配的场景调用得到偏移、独立事件调用保持原始输出、时间边界返回零；编译和自测均通过。本轮额外安排的外部只读采样器没有捕获测试窗口，已取消，不把它当作验证证据。

原始证据保存在本地 `analysis/runtime/native-wobble-1.csv`、`.json`、`.status.txt`，不进入 Git。

## 使用和下一步

构建 `build.bat build-wobble`，使用新的游戏进程运行 `start_capture.py --pid <当前PID> --seconds 180 --build-dir <build-wobble绝对路径> --wobble`。确认采集启动后才通知玩家恢复游戏，静止观察 25 秒。默认不加 `--wobble` 时仍只采集。

`stop_capture.py --pid <当前PID>` 可提前请求撤销入口补丁，随后用 `collect_capture.py --build-dir <同目录> --output <分析JSON路径>` 收集。停止请求本身不等于撤销已经成功，必须检查状态和入口字节。

下一步先做 OpenXR 运行时、头显位姿与坐标轴的独立验证，再把相对头部旋转接到这一渲染入口。仍需处理初始朝向、重新居中和原游戏视点的组合。平移、立体渲染和特殊镜头路径分别验收；本轮没有证明在真正脚本 VM 停顿时也能保持头部响应。
