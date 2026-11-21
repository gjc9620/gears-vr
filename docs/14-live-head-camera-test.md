# Real-time head-to-head access prototype

2026-10-12 Code compilation, independent self-measurement and restricted real time head start testing at the current level.

OpenXRPoseProbe shows left-and-right eye test maps from independent VDXR sessions and passes
Local\GearsVR.Pose. < GamePID> Share memory to publish the latest head position.
The name-based mutually exclusive lock protects the fixed structure; game backshows are only done zero for reading and when you lock up, return to your clean view point.
Creation of a second producer is rejected; mapping is verified by target PID, version, size and QPC frequency.
Timetamps are QPC nanoseconds when they are published, not OpenXR prediced Displaytimetime.

New --live-head mode and --sweep/ --webble reveal. Only local verified players
SceneView Returns the address of the event in view 0x15d0ca Up.
Builds a clean view point from the original function and then applies a change in the head attitude relative to the first active.
No changes to CameraCache, no changes in position of the person, no first-person anchor or eye view.

Validity check follows PoseGate: Effective and tracked, OpenXR session focus, game focus,
100 ms data within which, single-modified sample serial numbers and limited values; failure returns the clean view point and resumes it again.
Ratio is used temporarily 100 Game units/m, uncorrected; actual test limits per axis ±5° Length of the & Bits 2 Units.
Limiting the magnitude of the impact test should not be considered a complete head-on experience of natural ratios.
F 8 Abort the wheel (no longer resumed), hold down F 9 Returning. The capture timeout is still cancelled by the mechanism already in place.
DLL and map holders remain in the game until the process exits, so the next round uses a new game process.

Independent self-measurement of real Hook/trapleline path check: sharing memory producers/consumers, refusing to duplicate producers,
The first frame is centered, small head moves into temporary output, rotates and moves the cap, and expired data returns to original output.
ABI/Cessation of Self-Suspection and PoseMathTest/PoseGatetest have also passed.
These results are not a substitute for in-playing validation or user visibility.

Test order: Current version of camera identification  ⁇  Start verified Quest 3 Position producer
(-render --bridge-pid PID) ♪ GearsCaptureLive.dll
(-live-head)  ⁇  Users are subject to controlled head move  ⁇  Stop and check CSV/entry for recovery.
CSV Adds pose_status /pose_security, please=8 is the actual application head start.
PoseStatus numeric value is unarmed=0、inactive=1、untracked=2、stale=3、replayed=4、
invalid=5、recentered=6、applied=7。

The current round of headlight shows dark red/dark blue test maps, not game images. Image submission and eye rendering is a follow-up.

## Current level results

Play PID 28720，Quest 3 / VirtualDesktopXR 1.0.10。
Bit-producer Run 120 Seconds, in the game Hook 55 seconds;tool activation is rejected by Windows,
Users give focus to the game by hint and move flat. Users confirm action is completed, test maps are continuous and no apparent anomalies are visible.

- OpenXR 14,383 All samples are limited, effective and tracked;14,382 A different gesture.
- SceneView 16,129 bars;matching event status:inactive 5,990、recentered 1、applied 10,141。
- 10,141 The events of the strip,10,140 A header is applied to the bar view log, which involves 4,136 A different position number.
  Each application can be matched to the original OpenXR CSV valid, tracked and FOCUSED position.
- 3x3x rotations change;pitch/yaw maximum  ⁇910 Rotator units (approximately  ⁇ )5°,roll range-619..910。
  Maximum length of horizontal 2.000244838 in units, the trace above the mathematical limit is rounded by fload position.
- The original event location and rotations have only one value each; the CameraCache yaw in the modified view has only one value.
  The projection matrix always has only a set of values, and the view matrix changes head-on; all the views arrays recorded are limited.
- Change the caller for other events to 0,Defence change to 0,Procent failed as 0。
- stopped disable_status=0、active=0;rowing 9 Article : Records are not without damage.
  After stopping, read the two game entrances independently, which correspond to the original signature bytes.
- The lost focus view has 5,988 The Camera Cache time stamp, which demonstrates that it is updated during the inactivity of this round, is not available.

All nine cross log analyses are passed. Analysis tools:tools/native-capture/analyze_live_head.py。
Local evidence:analysis/runtime/live-head-28720.{csv,json,status.txt}、
live-head-28720-Videation.json.
live-head-28720- Cross-validation.json, and
analysis/openxr/pose-20261012-001141-498918.{csv,json,stdout.txt,stderr.txt}。

This round proves that real head-to-head can enter the original retrofit point of the current level and cannot declare that there are already playable VRs.
Unpredicted active disconnection, recovery of expired data or F 9 (a) Returning (these are self-measured only);
The first person, anchor, game unit scale, game image submission, eye view rendering still pending.

---

## 中文

# 实时头动接入原型

2026-10-12：代码编译、独立自测及当前关卡内受限实时头动测试通过。

OpenXRPoseProbe 可在独立 VDXR 会话显示已有左右眼测试图，并通过
Local\\GearsVR.Pose.<游戏PID> 共享内存发布最新头部位姿。
对应命名互斥锁保护固定结构；游戏回调只做零等待读取，锁忙时返回干净视点。
创建第二个生产者会被拒绝；映射按目标 PID、版本、尺寸和 QPC 频率核验。
时间戳是发布时的 QPC 纳秒，不是 OpenXR predictedDisplayTime。

新 --live-head 模式与 --sweep/--wobble 互斥。只在已核对的本地玩家
SceneView 内、视点事件返回地址 RVA 0x15d0ca 上变换临时输出。
先从原函数输出构建干净视点，再应用相对于首次有效头部姿态的变换。
不改 CameraCache、不改人物位置、不实现第一人称锚点或双眼场景。

有效性检查沿用 PoseGate：有效且被跟踪、OpenXR 会话聚焦、游戏聚焦、
100ms 以内的数据、单调样本序号和有限数值；失效返回干净视点，恢复后重新回中。
比例暂用 100 游戏单位/米，未校准；实际测试限制三轴每轴 ±5° 和位移向量长度 2 单位。
限制影响测试幅度，不应将此原型当作自然比例的完整头动体验。
F8 中止本轮头动（不再恢复），按住 F9 回中。捕获超时仍按已有机制撤销 Hook，
DLL 和映射持有者留在游戏内直到进程退出，因此下一轮用新游戏进程。

独立自测在真实 Hook/trampoline 路径检查：共享内存生产者/消费者、拒绝重复生产者、
首帧回中、小幅头动进入临时输出、旋转和平移上限、过期数据返回原输出。
已有 ABI/停止恢复自测及 PoseMathTest/PoseGateTest 也通过。
这些结果不替代游戏内调用验证或用户头显观察。

测试顺序：当前版本摄像机身份核验 → 启动已验证的 Quest 3 位姿生产者
（--render --bridge-pid PID）→ 注入独立 build-live 的 GearsCaptureLive.dll
（--live-head）→ 用户受控头动 → 停止并检查 CSV/入口恢复。
CSV 新增 pose_status / pose_sequence，phase=8 表示实际应用头动。
PoseStatus 数值依次为 unarmed=0、inactive=1、untracked=2、stale=3、replayed=4、
invalid=5、recentered=6、applied=7。

头显本轮显示暗红/暗蓝测试图，而不是游戏画面。图像提交和双眼渲染是后续工作。

## 当前关卡实测结果

游戏 PID 28720，Quest 3 / VirtualDesktopXR 1.0.10。
位姿生产者运行 120 秒，游戏内 Hook 55 秒；工具激活游戏被 Windows 拒绝后，
用户按提示给予游戏焦点并做转头、平移动作。用户确认动作完成、测试图连续、无明显异常。

- OpenXR 14,383 个样本均有限、有效且被跟踪；14,382 个不同姿态。
- SceneView 16,129 条；匹配事件状态：inactive 5,990、recentered 1、applied 10,141。
- 10,141 条头动事件、10,140 条视图记录应用了头动，涉及 4,136 个不同位姿序号。
  每条应用事件均可对应到原始 OpenXR CSV 的有效、被跟踪、FOCUSED 位姿。
- 三轴旋转均变化；pitch/yaw 最大 ±910 rotator 单位（约 ±5°），roll 范围 -619..910。
  平移长度最大 2.000244838 单位，超过数学限制的微量来自 float 位置差舍入。
- 原始事件位置与旋转各只有一个值；修改视图中的 CameraCache yaw 只有一个值。
  投影矩阵始终只有一组值，视图矩阵随头动变化；所有记录到的视图矩阵均有限。
- 其他事件调用者修改为 0，失焦修改为 0，Present 失败为 0。
- stopped disable_status=0、active=0；队列丢弃 9 条，记录不是无损。
  停止后独立读取两个游戏入口，与原始签名字节一致。
- 失焦视图有 5,988 个不同 CameraCache 时间戳，证明本轮失焦期间仍更新。

交叉日志分析的九项检查全部通过。分析工具：tools/native-capture/analyze_live_head.py。
本地证据：analysis/runtime/live-head-28720.{csv,json,status.txt}、
live-head-28720-validation.json（含独立入口恢复核验）、
live-head-28720-cross-validation.json，以及
analysis/openxr/pose-20261012-001141-498918.{csv,json,stdout.txt,stderr.txt}。

本轮证明真实头显姿态能进入当前关卡的原生渲染视点，不能宣布已经有可玩的 VR。
未实测主动断连、过期数据恢复或 F9 回中（这些只有独立自测）；
第一人称锚点、游戏单位尺度、游戏图像提交、双眼场景渲染仍待验证。
