# Undeclared OpenXR auto-certification

2026-10-12. The user's break is marked with the Meta XR Simulator; tools are saved on J disk. The simulator cannot replace Quest 3 Delays, comforts, real tracking, colour and final compatibility tests.

## Local Tool

- Meta CLI `1.8.0.17.10`：`J:\gearsVr\tools\third-party\metavr\bin\metavr.exe`。
- Meta XR Simulator `v207.0`：`J:\gearsVr\tools\third-party\meta-xr-simulator\PFiles\MetaXRSimulator\v207.0`。
- Validation of Authenticode as Valid, signed by the main body Meta Platforms, Inc.; depression to J disk using MSI advanced export without global installation, registration system OpenXR runtime or modification of PATH.
- The same MSI that used to be downloaded by CLI default to C disc, check with a copy of the J drive SHA 256 Deleted after consistency. Subsequent CLI use `METAVR_CONFIG_DIR=J:\gearsVr\tools\third-party\metavr\state`, the temporary directory is also located on J-disk.
- Software packages, third-party programs, configuration generation and source evidence by `.gitignore` Exclude, don't enter our source-code warehouse.

## Mechanisms and recurrence

`run_simulator.py` Read original configuration, generate in the same directory `gears_automation.json`, start a watch mode, keep the original relative resource path; setting only for test subprocesses `META_XRSIM_CONFIG_JSON` and `XR_RUNTIME_JSON`. The system is running as it is when defaulting.

```powershell
$pythonExe = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 12
# 先正常退出旧游戏，启动新共享通道并进入关卡；一个通道仅允许一个接收端：
& $pythonExe tools/native-capture/launch_binding.py --seconds 180 --gpu-share --build-dir tools/native-capture/build-share-verify
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 30 --gpu-game --gpu-snapshot
```

The menu operates on the existing semi-automatic scheme and reads a low-resolution screenshot of the game window before confirming it.`--gpu-snapshot` Run at least one of the OpenXR receivers 10 Read back in seconds the actual XR swapchain image, save 640 PPM; does not pass WGC or desktop, and normally shows no readback of each frame CPU image. This snapshot supports the texture content that is submitted, which is not equal to the final image of the company.

## Completed records

The actual number of entries for the emulator is as `[29,91,10,34,26,40,55,45,20]`, no game R 10G 10B 10A 2_UNORM（24. Therefore, add GPU shadow to R 8G 8B 8A 8_UNORM_SRGB（29. Under the coded SDR RGB scenario, shadow decode sRGB, RRTV and encode to maintain input of RGB; no HDR tone moving. 12 Frame-by-pixel verification. One more. 8-bit value. This machine does not have D 3D 12 Debug player, cannot claim to have been certified on that level.

8 Right-and-right eye test:565 The blog is a good place to record.564 Second double eye layer submission; program exit 0. Save GPU image check to left red, right blue and white cross. Default simulation position remains static (unique positions=)1) does not act as evidence of a real-time head start.

First round of GPU  ⁇  OpenXR: Game PID 34508，30 The second sample,2,130 Subquad Submission,1,112 Subtexture Copy, Exit 0, no frame CPU image read back. Game logs 43,236 Second Present, failed 0;publication 1,113 Frame, after stop 0 The game has been successfully released and readout outside the Hook portal.1920 × 1080 / Minimum settings. The wheel includes the loading phase of a level, without saving the final copositor image and without declaring each frame to be a level.

Evidence:`analysis/openxr/pose-20261012-030424-060822.*`、`pose-20261012-031249-996766.*`、`analysis/runtime/gpu-share-34508.{json,jsonl}`。

Second round of independent games PID 17584 After entering the level, 30 sec:2,157 Subquad Submission,1,200 The next GPU copy, exit normal. One XR swapchain return successfully saved with a manual check of stone pillars, corridors, candlelights and normal direction. 1,201 Frame,43,877 Second Present, Failed 0, the entrances recover and resources release successfully. The snapshot is not a final image of the company without any visible observation.`pose-20261012-031640-107823.*`、`native-gpu-xr-17584.png`、`analysis/runtime/gpu-share-17584.{json,jsonl}`。

Cover the entrance. `run_simulator.py --seconds 8` Run independently by:574 Place record, exit. 0。

## Autoposition interface with composite map

Meta XR Operator v 207 Loaded through official CLI `tools/third-party/metavr/state/tools/meta-xr-operator`Just... `run_simulator.py --operator` OpenXR API player; not registered to Codex Global MCP. Test script via localhost:8720 MCP Initializes, sends initiated notifications and confirms that the interface is called after application named GearsVR Pose Probe. The operation of other XR services already available is denied.

`simulator_motion_test.py` Run 30 sec, set in  ⁇10 cm Left/ Left, Right, Right, Right, Right, Right.5 cm Up and Down & nick 10° yaw, back in the middle of saving the final copositor diagram for the left and right eye to check the three-axis horizontal shifts and range of yaws for CSV. All are artificially set simulators; XR Operator returns Valid=true, tracked = false, which is not considered a real tracker by PoseGate and does not move around the head of the game to test.

The first automatic test captures the service before the session is ready and the initial setting is rejected; the simulator cleanup also exceptionally exits, so the test fails and is not counted as passed. Scripts are now waiting for valid sessions to recognize top level and content block errors and close MCP sessions after correction. `sim-motion-20261012-032340.json` Test passed:220 Different gestures, full range of targets, left-eye composite image preservation success, program exit 0. This time is not considered successful as having repaired all runtime cleanups abnormal.

Game Panel Group Test `sim-motion-20261012-032640.json`（PID 40888 Also adopted:1,194 Sub-GPU copy,2,149 Subquad surrender, position range check, exit 0. Manually check the final eye and left copositor chart, a 2-dimensional panel at the bottom of the black do indeed have stone columns, corridors and candlelights. The panel is small and currently has diagnostic layout without VR vision or comfort evaluation. The head change in this test remains simulation input, without control over game lenses.

```powershell
& $pythonExe tools/openxr-probe/simulator_motion_test.py
# 新游戏 GPU 通道进入关卡后，同样检查游戏二维面板：
& $pythonExe tools/openxr-probe/simulator_motion_test.py --gpu-game
```

## Simulator backstage resource accumulation and cleanup

2026-10-12 User report is full. Check to confirm MetaXRSimulator.exe (PID) under current installer directory 47112) and its subprocesses synth_env_server.exe(PID) 45512 From the... 03:00:49 You can always stay there. 42.97 GiB, Job package 6.14 GiB, handle. 79,007 systems; 31.81 GiB, the physical memory is only about to be agreed. 0.9 GiB is available. Private memory contains submission and page change sections, which cannot be used 42.97 GiB as all in RAM.

No active OpenXRPoseProbe test process. After checking the PID and complete J drive path of both processes, try to properly close the frontend of the emulator before stopping these two exact processes that remain; no games or other software are closed. Physical memory can be recovered to an approximate after cleanup. 17.41 GiB, the system submission balance also recovered. Evidence supports the accumulation of accumulator backstage abnormality resources and no specific leak code has been located.

The subsequent automatic watch test should not keep the front end of a full screen emulator and the permanent presence of a synthetic environment server. Each round ends with checking and cleaning up the ancillary processes that are started in this cycle; other sessions already exist cannot be terminated by name batch. Previous single tests exit 0 This does not mean that back-office environmental services have been automatically withdrawn or that resources are used normally for long periods of time.

## Not proven

The current is the same two-dimensional panel with eyes visible, and the complete stereo scene has not been rendered; real time head moves and displaying paths are still separate prototypes. Simulator MP for CLI 4 Recording attempts to return the current runtime is not supported, no video output. Early unmapped tests have been abnormally withdrawn during runtime cleanup and then the screen configured layer test has been completed normally; configuration changes are not separately identified as proven root causes.

References:[Meta XR Simulator Document](https://developers.meta.com/vr/documentation/native/xrsim-intro/)、[Meta CLI XR Environment Command](https://developers.meta.com/vr/essentials/metavr-environment/)、[Official Windows Downloads](https://developers.meta.com/vr/downloads/package/meta-xr-simulator-windows/)。

---

## 中文

# 不戴头显的 OpenXR 自动验证

2026-10-12。用户休息期间使用 Meta XR Simulator 验证显示通路；工具保存在 J 盘。模拟器不能替代 Quest 3 的延迟、舒适度、真实跟踪、颜色和最终兼容性测试。

## 本机工具

- Meta CLI `1.8.0.17.10`：`J:\gearsVr\tools\third-party\metavr\bin\metavr.exe`。
- Meta XR Simulator `v207.0`：`J:\gearsVr\tools\third-party\meta-xr-simulator\PFiles\MetaXRSimulator\v207.0`。
- 通过官方 CLI 下载 MSI，验证 Authenticode 为 Valid，签名主体 Meta Platforms, Inc.；使用 MSI administrative extraction 解压到 J 盘，没有全局安装、注册系统 OpenXR runtime 或修改 PATH。
- 原先 CLI 默认下载至 C 盘的同一 MSI，核验与 J 盘副本 SHA256 一致后已删除。后续 CLI 使用 `METAVR_CONFIG_DIR=J:\gearsVr\tools\third-party\metavr\state`，安装临时目录也设在 J 盘。
- 软件包、第三方程序、生成配置和原始证据由 `.gitignore` 排除，不进入我们的源代码仓库。

## 机制和复现

`run_simulator.py` 读取原配置，在同目录生成 `gears_automation.json`，开启 batch mode，保留原相对资源路径；仅给测试子进程设置 `META_XRSIM_CONFIG_JSON` 和 `XR_RUNTIME_JSON`。系统默认运行时保持原样。

```powershell
$pythonExe = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 12
# 先正常退出旧游戏，启动新共享通道并进入关卡；一个通道仅允许一个接收端：
& $pythonExe tools/native-capture/launch_binding.py --seconds 180 --gpu-share --build-dir tools/native-capture/build-share-verify
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 30 --gpu-game --gpu-snapshot
```

菜单按现有半自动方案操作，每次确认前读取低清游戏窗口截图。`--gpu-snapshot` 在 OpenXR 接收端运行至少 10 秒后回读一次实际 XR swapchain 图像，保存 640 像素宽 PPM；不经过 WGC 或桌面，正常显示没有每帧 CPU 图像回读。这个快照证明提交纹理内容，不等同于 compositor 最终图像。

## 已完成记录

模拟器实际枚举格式为 `[29,91,10,34,26,40,55,45,20]`，没有游戏的 R10G10B10A2_UNORM（24）。因此增加 GPU shader 转换到 R8G8B8A8_UNORM_SRGB（29）。在已编码 SDR RGB 假设下，shader 先解码 sRGB，RTV 再编码，保持输入 RGB；没有 HDR tone mapping。跨进程已知颜色的 12 帧逐像素核验通过，容差一个 8-bit 数值。此机器没有 D3D12 debug layer，不能声称经过该层验证。

8 秒左右眼测试：565 个有效位姿记录，564 次双眼图层提交；程序退出 0。保存 GPU 图像检查为左红、右蓝和白十字。默认模拟位姿始终静止（unique poses=1），不作为实时头动证据。

首轮游戏 GPU → OpenXR：游戏 PID 34508，30 秒采样，2,130 次 quad 提交、1,112 次纹理复制，退出 0，无每帧 CPU 图像回读。游戏记录 43,236 次 Present，失败 0；发布 1,113 帧，停止后 active=0、资源释放成功，四处 Hook 入口外部读取均恢复。游戏已有第一人称 Mod，1920 × 1080 / 最低设置。该轮包括关卡载入阶段，未保存最终 compositor 图像，不宣称每一帧都是关卡。

证据：`analysis/openxr/pose-20261012-030424-060822.*`、`pose-20261012-031249-996766.*`、`analysis/runtime/gpu-share-34508.{json,jsonl}`。

第二轮独立游戏 PID 17584，进入关卡后 30 秒：2,157 次 quad 提交、1,200 次 GPU 复制，正常退出。一次 XR swapchain 回读保存成功，人工检查为石柱、走廊、烛光，方向正常。游戏发布 1,201 帧，43,877 次 Present、失败 0，停止后四处入口恢复、资源释放成功。快照不是 compositor 最终图像，未经过头显观察。证据：`pose-20261012-031640-107823.*`、`native-gpu-xr-17584.png`、`analysis/runtime/gpu-share-17584.{json,jsonl}`。

封装入口 `run_simulator.py --seconds 8` 已独立运行通过：574 条位姿记录，退出 0。

## 自动位姿与合成图接口

Meta XR Operator v207 已通过官方 CLI 装到 `tools/third-party/metavr/state/tools/meta-xr-operator`。仅在 `run_simulator.py --operator` 子进程开启 OpenXR API layer；没有注册 Codex 全局 MCP。测试脚本通过 localhost:8720 MCP 初始化、发送 initialized 通知，并确认应用名为 GearsVR Pose Probe 后调用接口。拒绝操作已存在的其他 XR 服务。

`simulator_motion_test.py` 会运行 30 秒，依次设置 ±10 cm 左右 / 前后、±5 cm 上下和 ±10° yaw，回中，保存左右眼最终 compositor 图，核对 CSV 的三轴平移和 yaw 范围。全部是人工设置的模拟位姿；XR Operator 返回 valid=true、tracked=false，现有 PoseGate 不会将其视作真实跟踪，不为测试绕过游戏头动保护。

第一次自动测试捕获到服务先于会话就绪，首次设置被拒绝；模拟器清理也异常退出，因此测试失败，没有计为通过。脚本现等待有效会话，识别顶层及内容块错误，结束时关闭 MCP 会话。修正后 `sim-motion-20261012-032340.json` 测试通过：220 个不同姿态、全部目标范围出现、左右眼合成图保存成功、程序退出 0。不将这一次成功认定为已修复所有 runtime 清理异常。

游戏面板组合测试 `sim-motion-20261012-032640.json`（PID 40888）也通过：1,194 次 GPU 复制、2,149 次 quad 提交、位姿范围核对通过、退出 0。人工检查最终左右眼 compositor 图，黑底中的二维面板确实为石柱、走廊和烛光关卡。面板偏小且当前为诊断布局，不评价 VR 视野或舒适度。此测试的头部变化仍为模拟输入，未控制游戏镜头。

```powershell
& $pythonExe tools/openxr-probe/simulator_motion_test.py
# 新游戏 GPU 通道进入关卡后，同样检查游戏二维面板：
& $pythonExe tools/openxr-probe/simulator_motion_test.py --gpu-game
```

## 模拟器后台资源累积与清理

2026-10-12 用户报告内存占满。检查确认本轮安装目录下的 MetaXRSimulator.exe（PID 47112）及其子进程 synth_env_server.exe（PID 45512，Bedroom -batchmode）从 03:00:49 起一直驻留。后者检查时私有内存约 42.97 GiB、工作集约 6.14 GiB、句柄 79,007；系统 31.81 GiB 物理内存只剩约 0.9 GiB 可用。私有内存包含提交和换页部分，不能把 42.97 GiB 当作全在 RAM 中。

没有活动的 OpenXRPoseProbe 测试进程。核验两个进程的 PID 和完整 J 盘路径后，先尝试正常关闭模拟器前端，再停止仍驻留的这两个确切进程；没有关闭游戏或其他软件。清理后可用物理内存回升至约 17.41 GiB，系统提交余量也回升。证据支持模拟器后台异常资源累积，尚未定位具体泄漏代码。

后续自动 batch 测试不应额外保持全屏模拟器前端及合成环境服务器长期驻留。每轮测试结束需检查并清理本轮启动的辅助进程；已有其他会话的进程不可按名称批量终止。以前单次测试退出 0，不代表后台环境服务已自动退出或长时间资源使用正常。

## 尚未证明

当前是双眼可见的同一二维面板，完整立体场景尚未渲染；实时头动与显示通路仍为分开的原型。模拟器 CLI 的 MP4 录制尝试返回当前 runtime 不支持，未产出录像。早期无图层测试曾在 runtime 清理阶段异常退出，随后 batch 配置的图层测试正常结束；不把配置改变单独认定为已证明根因。

参考：[Meta XR Simulator 文档](https://developers.meta.com/vr/documentation/native/xrsim-intro/)、[Meta CLI XR 环境命令](https://developers.meta.com/vr/essentials/metavr-environment/)、[官方 Windows 下载](https://developers.meta.com/vr/downloads/package/meta-xr-simulator-windows/)。
