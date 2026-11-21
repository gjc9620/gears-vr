# Six Axe Head Track: Simulator to Real scene image

2026-10-16，Native 23 / XR 11 Private candidate, no raise of the default player list.

## Modification of the evidence boundary

No changes to native camera math or Hook were made during this cycle. `stereo_matrices` Applys the same position and rotation to the original view of the left or right eye relative to the fixed heading reference, default 100 Game units/m. An optional image-ticket diagnosis with an associated HMD position/quaternion document for the actual image generation instead of the latest runtime poste when the screenshot is saved. Summary scheme is raised to 2 and with a syntax; selection rules,4096 Line caps and image submissions remain the same. The log is not added when the diagnosis is defaulted on.

Freeze construction:

- Native23 DLL SHA256：`bf71e3ae9a7df228d12d8e4665f9bb6a178e4d3a9aa599e43b1923298ce90c21`。
- XR11 EXE SHA256：`99dc740901faa0b3c9a4909e226de78678c2533b3475aef475d1182fbe25fea2`。
- Visible use `analysis/runtime/simulator-quiet2/openxr_simulator.json`;PSAPPI check target XR process actually loads the same directory DLL, which is not supported by a system default runtime registration form.

## Actual

Session `continuous-stereo-36652-20261016-112128-68493f7b`: Automatically enters the level, without focus, without handle input.1920×1080 I'm gonna be right and right.960×1080, set by game resolution. At least wait after each turn-over 2 seconds, request the GPU source image again.

| Phase | Relative Initial Posture | Imageticket |
| --- | --- | --- |
| baseline | Location (%1)0,1.7,0) No rotation | 286 |
| x | x Increase 0.15 Rice | 1042 |
| y | y-Add 0.15 Rice | 1653 |
| z | Reduction of z 0.15 Rice | 1855 |
| yaw | 8 degrees | 2056 |
| pitch | 8 degrees | 2661 |
| roll | 8 degrees | 2869 |
| return | Back to baseline. | 3093 |

8 A snapshot is linked to the same ticket that was successfully submitted, freezing position and request with a maximum error of less than 5e-8 Mi, four-digit error less than 3e-8. Diagnosis retention 3099 Okay, drop it.0 Line; this is the complete retention of the selected image instrument, not all pixels for display frames.

Native buelt, consume_before, preparate_before, prepare_after 3127 View, Average `mismatch=0 / view_error_max=0`It proves that the matrix of actual engine stages is consistent with freeze expectations; it cannot be shown separately that GPU was ultimately visualized correctly.

8 Zhang GPU source SBS has both eyes.24 Pixels, y-diverse vertically approximately+12/+21 pixels, yaw-like+66/+63 Pixels, pitch vertically equals about +69 Pixels; roll images are visible tilted. The flow field of z-pasts is space-diffed and cannot be moved by single-digit medians representing the camera before and after.3,0) pixels; the scene animation and light are still different, not pixel-by-pixel. Matching is only supported by image motion and is not used for precise scale or camera marking.

## Validation and cleanup

- 12 The XR CPU test was passed, including image instrument field ownership, precision, schema, spill and return to the old synthetic rule.
- Native `StereoFrameTest` Passed, including independent quatorion point conversion oracle, six axes, heating, Gate, matrix, Bridge.
- Normal stoppage, clean-up of primary and resource,43 The entrance is restored and the XR object is destroyed; then the game identity check sends WM_CLOSE and exits. No strong, no active windows; the simulator attitude has been restored to its initial value.

Original PPM, freeze of birth state, document authentication, identity and image analysis left in signored `analysis/runtime/six-axis-*`, do not enter the release package.

## Not proven

This round supports the current rendering path with a six-axis response, which cannot be declared completed for real appearance 6 DoF accepts. Physical migration scales, re-exhausture, tracking loss recovery, border comforts, different headline and normal playtime movements are still to be verified; no CG, all UI or long-term black frames are covered. Default player version is not up yet, V 1 The publication targets remain open.

## Non-zero position and attitude next in progress (XR)12）

Follow-up session `continuous-stereo-41892-20261016-113223-9298c461` Actual Location (Present)0.2,1.8,-0.1# I'm a man, I'm a man 20°/pitch 10°/roll 8°Save the ticket first 1190, call the official `launcher.py recenter`. original 1→2@ticket 1293 Submit, reference position and request error less than 1e-6 Mi, reference quartonion only 20°Horizontally. Back to center and quickticket 1522 still holding full HMD rotation; independent calculations with reference reverse rotation, residue and pitch 10°×roll 8°Consistent (with less than a mass error)1e-6) does not separate the head and side.

Then the next relative new baseline continues x-diversion 0.15 Me, Ticket.1735 Two-eye image changes;go back (0,1.7,0)/ Zero is officially returning, generation 2→3@ticket 2198 Submit, ticket 2398 The two-eye scenario recovers the level of the baseline, midway back and forth, continues to move evenly, returns to the Chinese Communist Party for the second time 5 The GPU source map is linked to the correct attitude note.2431 Check matrix without bias; normal stop,43 The entrance is restored, XR destroyed and WM_CLOSE exited. `recenter-events.json`、`recenter-requests.jsonl`、`recenter-native-status.txt`、`recenter-verification.json`And GPU images.

The new GUI's "Faceback" command path has been constructed/self-checked; this time it actually implements the formal backend command that it shares, without automatically clicking on the GUI. Physical experience is still subject to user verification and does not close the whole item 6 DoF's acceptance.

## Quest 3 / Virgin Desktop Truth Starter Repair 17）

2026-10-16 User requests real experience, stop previous visible simulator sessions and specify the genre-runtime --Virtual Desktop Streeter/OpenXR/virtualdesktop-openxr.json; no change to global runtime. First Native 23/XR 16 Session 37616 The Virtual DesktopXR session was successfully created, but the game was normally closed after the XR object had been destroyed after waiting for a card construction report, Invalid transfer-card upload footprint.

XR 17 Only fixture card: Accepts the selected RGBA/BGRA UNORM resource and its own typepeles, still strictly validates size, aray/mip/sample, alignment and copying borders; uploads the footprint using the selected typed UNORM to verify again when obtaining the image. TranstionCardLayoutTest/TransionCardPixelstest passes. New builder 17-vd，SHA 256 6efd90db1b380d0956259631c42d785f928e48d165fb2953fe96442dfaa14aa5, use an independent temporary freeze without replacing the default freeze version.

Actual 23616/XR 35784 Session 2aa 35bb 9 Confirm that the runtime_name is = Virgin DesktopXR, simulator_oporator=0;diagnosis 87/resource 90 B.8G 8R 8A 8_UNORM and corresponding Typeles, dimensions 768×256，typed footprint 87/pitch 3072/bytes 786432. Session into state 5, the WCC starts successfully and the frame continues to increase until entry level is reached without setting an automatic stop. This results in a genuine machine start-up compatibility fix, which does not replace the user 's actual menu/bieye/6 DoF visual acceptance. Original evidence is continuous-stereo-23616-20261016-174554-2aa 35bb 9 And human-qest 3-vd 17.json, save in signored analysis/runtime。

---

## 中文

# 六轴头部追踪：模拟器到实际场景图像

2026-10-16，Native23 / XR11 私有候选，未提升玩家默认清单。

## 修改与证据边界

本轮没有修改原生相机数学或 Hook。已有 `stereo_matrices` 将相对于固定 heading reference 的位置和旋转共同应用到左右眼原生视图，默认100游戏单位/米。新增的是可选 image-ticket 诊断中的冻结 HMD position / quaternion，关联实际生成图像的票据，而非截图保存时的最新 runtime pose。摘要 schema 升为2并注明语义；选择规则、4096行上限和图像提交行为保持原样。默认未启用诊断时不增加日志。

冻结构建：

- Native23 DLL SHA256：`bf71e3ae9a7df228d12d8e4665f9bb6a178e4d3a9aa599e43b1923298ce90c21`。
- XR11 EXE SHA256：`99dc740901faa0b3c9a4909e226de78678c2533b3475aef475d1182fbe25fea2`。
- 显式使用 `analysis/runtime/simulator-quiet2/openxr_simulator.json`；PSAPI 校验目标XR进程实际加载同目录DLL，未以系统默认runtime注册表代替证明。

## 实际运行

会话 `continuous-stereo-36652-20261016-112128-68493f7b`：自动进入关卡，无焦点激活、无手柄输入。游戏源1920×1080，左右眼各960×1080，按游戏分辨率裁取。每次切换姿态后至少等待2秒，再请求GPU源图像。

| 阶段 | 相对初始姿态 | 图像ticket |
| --- | --- | --- |
| baseline | 位置(0,1.7,0)，无旋转 | 286 |
| x | x增加0.15米 | 1042 |
| y | y增加0.15米 | 1653 |
| z | z减少0.15米 | 1855 |
| yaw | 8度 | 2056 |
| pitch | 8度 | 2661 |
| roll | 8度 | 2869 |
| return | 回到baseline | 3093 |

8张快照均关联到成功提交的同一ticket，冻结位置与请求最大误差小于5e-8米，四元数分量误差小于3e-8。诊断保留3099行，丢弃0行；这是选定图像票据的完整保留，不是所有显示帧的像素验收。

原生built、consume_before、prepare_before、prepare_after各检查3127对视图，均 `mismatch=0 / view_error_max=0`。它证明实际引擎阶段矩阵与冻结期望一致；不能单独证明GPU最终视觉正确。

8张GPU源SBS均有双眼场景。离线稀疏背景patch匹配观察到：x平移两眼水平中位位移均约−24像素，y平移纵向约+12/+21像素，yaw约+66/+63像素，pitch纵向均约+69像素；roll图像可见倾斜。z平移的流场有空间差异，不能用单个位移中位数代表前后相机运动。回到初始姿态后背景匹配中位位移约(3,0)像素；场景动画与光照仍有变化，不要求逐像素相同。匹配仅作为图像运动佐证，不用于精确尺度或相机标定。

## 验证与清理

- 12项XR CPU测试通过，包括图像票据字段所有权、精度、schema、溢出与旧合成规则回归。
- 原生 `StereoFrameTest` 通过，包括独立quaternion点变换oracle、六轴、heading、gate、matrix、bridge检查。
- 正常停止、原生排空和资源清理、43入口恢复、XR对象销毁通过；随后核验游戏身份发送WM_CLOSE并确认退出。未强杀、未激活窗口；模拟器姿态已恢复初始值。

原始PPM、冻结原生状态、票据验证、身份和图像分析留在ignored `analysis/runtime/six-axis-*`，不进入发布包。

## 尚未证明

此轮支持当前渲染路径具有软件六轴响应，不能宣称已完成真头显6DoF验收。物理位移尺度、回中体验、跟踪丢失恢复、边界舒适度、不同头显及正常游玩移动仍待验证；不覆盖CG、所有UI或长期黑帧。默认玩家版本尚未提升，V1发布目标保持开放。

## 非零位置和姿态下回中（XR12）

后续会话 `continuous-stereo-41892-20261016-113223-9298c461` 实际设置位置(0.2,1.8,-0.1)米、yaw20°/pitch10°/roll8°，先保存ticket1190，再调用正式 `launcher.py recenter`。原生generation1→2于ticket1293提交，参考位置与请求误差小于1e-6米，参考quaternion仅含20°水平朝向。回中后快照ticket1522仍持有原完整HMD旋转；以参考逆旋转独立计算，残余与pitch10°×roll8°一致（分量误差小于1e-6）。没有将抬头和侧倾一并清零。

随后相对新基准继续x平移0.15米，ticket1735双眼图像变化；回到(0,1.7,0)/零旋转再次正式回中，generation2→3于ticket2198提交，ticket2398双眼场景恢复水平。基准、回中前后、继续平移、第二次回中共5张GPU源图均关联正确姿态票据。四个原生阶段各2431对矩阵检查无偏差；正常停止、43入口恢复、XR销毁和WM_CLOSE退出通过。证据为ignored `recenter-events.json`、`recenter-requests.jsonl`、`recenter-native-status.txt`、`recenter-verification.json`及GPU图像。

GUI新增的“视角回中”命令路由已构建/自检；本次实际执行的是它共用的正式后端命令，并未自动点击GUI。物理头显回中体验仍待用户验证，不因此关闭整项6DoF验收。

## Quest 3 / Virtual Desktop 真机启动修复（XR17）

2026-10-16用户要求真机体验，停止先前显式模拟器会话后，以子进程 --runtime 指定 Virtual Desktop Streamer/OpenXR/virtualdesktop-openxr.json；没有修改全局runtime。首次Native23/XR16会话37616已成功创建VirtualDesktopXR session，却在等待卡构造报告Invalid transition-card upload footprint并退出，原生未启动，prime入口恢复通过，XR对象销毁后正常关闭游戏。

XR17仅修transition card：接受选定RGBA/BGRA UNORM及其同族typeless资源，仍严格验证尺寸、array/mip/sample、对齐和复制边界；上传footprint使用选定的typed UNORM，在取得图像时再次核验。TransitionCardLayoutTest/TransitionCardPixelsTest通过。新构建build-main-menu17-vd，SHA256 6efd90db1b380d0956259631c42d785f928e48d165fb2953fe96442dfaa14aa5，使用独立临时manifest，不替换默认冻结版本。

实际game23616/XR35784会话2aa35bb9确认runtime_name=VirtualDesktopXR、simulator_operator=0；诊断selected87/resource90，即B8G8R8A8_UNORM与对应TYPELESS，尺寸768×256，typed footprint87/pitch3072/bytes786432。会话进入state5，WGC启动成功、帧持续递增，等待进入关卡，未设置自动停止。该结果验证真机启动兼容修复，不代替用户实际菜单/双目/6DoF观感验收。原始证据为continuous-stereo-23616-20261016-174554-2aa35bb9及manifest-quest3-vd17.json，保存在ignored analysis/runtime。
