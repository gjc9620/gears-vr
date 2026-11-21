# OpenXR Independent diagnostic and submission tests

No changes to the game or system OpenXR registration form. Use your own machine OpenXR 1.1.63 x 64 loader，
Source code relies only on official header files and import library. Compressor packages are unpressed to Git ignored
`tools/third-party/openxr-loader`。

```powershell
cmd /c J:\gearsVr\tools\openxr-probe\build.bat
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\openxr-probe\run_probe.py
```

Basic probe count extensions, creation of instance, query HMD system and recommend left-and-right eye parameters.
Do not create a session or submit a screen. Default to run the system;`--runtime <manifest.json>`
Sets the sub-process only XR_RUMTME_JSON without changing the registration form.
Python Packer in 25 Ends its own subprocesses when the second is timed out and saves diagnosis JSON to `analysis/openxr/`。

Bit Session Test (release directory created before call):

```powershell
& J:\gearsVr\tools\openxr-probe\build\OpenXRPoseProbe.exe 12 J:\gearsVr\analysis\openxr\pose.csv
```

Program Call D 3D 12 Graphics updates, create devices and queues based on GPU LuID supplied at running time.
Creates a session, LOCAL and VIEW reference spaces. Copy READY for BeginSsession.
WaitFrame  ⁇ BeginFrame  ⁇  LocateViews / LocateSpace  ⁇  EndFrame, recording head and eye position
Four-digit, visual field, VALID / TRACKED bits. Pure position mode submits zero layer.
Regular exit request ExitSsession, consumption STOPPING and EndSearch, destruction of resources.
Original calls may be blocked by the effects of running; unattended packages are always overtimed as run_probe.py.

It is recommended that the existing total time-out packaging should be used to auto-set and save CSV, complete logs and brief analyses:

```powershell
& $py J:\gearsVr\tools\openxr-probe\run_pose.py --seconds 12
& $py J:\gearsVr\tools\openxr-probe\run_pose.py --seconds 5 --render
```

The timeout is for test seconds plus 20 sec; timeout only terminates its own diagnostic subprocess.
`--runtime` Also valid for the sub-process only; the result is not automatically a claim to true tracking success based on TRACKED or position changes.

Independent video submission test:

```powershell
& J:\gearsVr\tools\openxr-probe\build\OpenXRPoseProbe.exe 5 J:\gearsVr\analysis\openxr\pattern.csv --render
```

Every eye 512×512; Dark red left eye, dark blue right eye, white central cross.
Clear D after Acquisition / Wait 3D 12 Release after GPU fence is finished.
First reading back every eye image, saving `pattern.csv-left.ppm` / `pattern.csv-right.ppm` With different checksums.
Converts RENDER_TARGET  ⁇ COPY_SOURCE  ⁇ RENDER_TARGET during return reading to comply with the XR image status protocol.
EndFrame submits a two-eyed program player. This test validates the resource and the submission link, which is a tool for monitoring the process.
Not a game with real vision.

Math tests:

```powershell
& J:\gearsVr\tools\openxr-probe\build\PoseMathTest.exe
```

`tools/vr-common/pose_math.h` Define OpenXR ⁇ UE coordinates transformation, relative backsizing, four-digit synthesis,
Position unit conversion and UE Rotator unit output. Default 100 Units/metres are only preset to be calibrated within the game.
It is a public code that has not yet been accessed and does not represent current real-time head start tracking.

`PoseGateTest.exe` Test pose_gate.h: Default not enabled, confirmation of real head after streaming arm;
100 ms Freshness, complete VALID/TRACKED, focus on games and sessions, sequence regression/false inspection.
Allows multiple frames to repeat the same fresh sample. Return clean view and clear back the positive benchmark when it is invalid.
The time stamp must come from a single clock shared by the producer/consumer.
OpenXR prediced Displaytime; new producer session has to be created.
This is not yet a shared memory transfer or real-time input interface in the game.

References:[OpenXR D3D12 Image Status Specification](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_KHR_D3D12_enable-swapchain-image-state.html)、
[Khronos Official SDK Example](https://github.com/KhronosGroup/OpenXR-SDK-Source/tree/main/src/tests/hello_xr)。
# Show mode for game windows

`run_pose.py --seconds 30 --game-screen --runtime <VDXR manifest>` In the independence process
Continuously collect path-checked games HWD to 960×540 Target date 15 Hz Update to OpenXR
2D panel. Only the game window, not the entire desktop; this wheel does not drive the game view point, not the stereo scene rendering.
First frame saves the collection of PPM and swapchain GPU to read back to PPM and check pixels.
This model is reticent with --render/-bridge-pid, and by default retains the original diagnostic behaviour.
Validation and restrictions are available docs/15-game-screen-display.md。

---

## 中文

# OpenXR 独立诊断与提交测试

无需修改游戏或系统 OpenXR 注册表。使用本机 OpenXR 1.1.63 x64 loader，
源码只依赖官方头文件和导入库。压缩包已解压到 Git 忽略的
`tools/third-party/openxr-loader`。

```powershell
cmd /c J:\gearsVr\tools\openxr-probe\build.bat
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\openxr-probe\run_probe.py
```

基础 probe 枚举扩展、创建 instance、查询 HMD system 和推荐左右眼参数。
不创建会话，也不提交画面。默认使用系统运行时；`--runtime <manifest.json>`
只给该子进程设置 XR_RUNTIME_JSON，不更改注册表。
Python 包装器在 25 秒超时时结束自己的子进程，并保存诊断 JSON 至 `analysis/openxr/`。

位姿会话测试（调用前须创建输出目录）：

```powershell
& J:\gearsVr\tools\openxr-probe\build\OpenXRPoseProbe.exe 12 J:\gearsVr\analysis\openxr\pose.csv
```

程序调用 D3D12 graphics requirements，根据运行时提供的 GPU LUID 创建设备及队列，
再创建 session、LOCAL 与 VIEW reference spaces。收到 READY 才 BeginSession。
WaitFrame → BeginFrame → LocateViews / LocateSpace → EndFrame，记录头部与眼睛位置、
四元数、视场、VALID / TRACKED 位。纯位姿模式提交零图层。
正常退出请求 ExitSession，消费 STOPPING 并 EndSession，再销毁资源。
原生调用可能受运行时影响阻塞；无人值守时应像 run_probe.py 一样用子进程总超时包裹。

建议使用已有的总超时包装器，自动建目录并保存 CSV、完整日志和简短分析：

```powershell
& $py J:\gearsVr\tools\openxr-probe\run_pose.py --seconds 12
& $py J:\gearsVr\tools\openxr-probe\run_pose.py --seconds 5 --render
```

总超时为测试秒数加 20 秒；超时只终止自己的诊断子进程。
`--runtime` 同样仅对该子进程有效；结果不会凭 TRACKED 或位姿变化自动宣称真实跟踪成功。

独立画面提交测试：

```powershell
& J:\gearsVr\tools\openxr-probe\build\OpenXRPoseProbe.exe 5 J:\gearsVr\analysis\openxr\pattern.csv --render
```

每眼仅 512×512；左眼暗红、右眼暗蓝，中心白色十字。
Acquisition / Wait 后清除 D3D12 render target，等 GPU fence 完成再 Release。
第一次回读每眼图像，保存 `pattern.csv-left.ppm` / `pattern.csv-right.ppm` 与不同校验和。
回读期间做 RENDER_TARGET → COPY_SOURCE → RENDER_TARGET 转换，遵守 XR 的图像状态约定。
EndFrame 提交一个含两眼的 projection layer。这个测试验证资源和提交链路，
不是带真实视差的游戏双目渲染。

数学测试：

```powershell
& J:\gearsVr\tools\openxr-probe\build\PoseMathTest.exe
```

`tools/vr-common/pose_math.h` 定义 OpenXR→UE 坐标变换、相对回正、四元数合成、
位置单位换算，以及 UE Rotator 单位输出。默认 100 单位/米只是预设，待游戏内尺度校准。
它是尚未接入游戏的公共代码，不代表当前有实时头显跟踪。

`PoseGateTest.exe` 测试 pose_gate.h：默认未启用，需确认真实头显流后显式 arm；
100ms 新鲜度、完整 VALID/TRACKED、游戏和会话焦点、序列倒退/伪造检查。
允许多个游戏帧重复使用同一份新鲜样本。无效时返回干净视点并清除回正基准，
恢复后首份有效数据重新回正。时间戳必须来自生产者/消费者共享的单调时钟，
不能直接使用 OpenXR 的 predictedDisplayTime；新生产者会话须新建 gate。
这尚不是共享内存传输或游戏内实时输入接口。

参考：[OpenXR D3D12 图像状态规范](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_KHR_D3D12_enable-swapchain-image-state.html)、
[Khronos 官方 SDK 示例](https://github.com/KhronosGroup/OpenXR-SDK-Source/tree/main/src/tests/hello_xr)。
# 游戏窗口显示模式

`run_pose.py --seconds 30 --game-screen --runtime <VDXR manifest>` 在独立进程
持续采集经过路径核验的游戏 HWND，以 960×540、目标约 15Hz 更新到 OpenXR
二维面板。仅传游戏窗口，不传整个桌面；本轮不驱动游戏视点，不是立体场景渲染。
首帧保存采集 PPM 和 swapchain GPU 回读 PPM，并逐像素核验。
此模式与 --render / --bridge-pid 互斥，默认仍保留原有位姿诊断行为。
验证及限制见 docs/15-game-screen-display.md。
