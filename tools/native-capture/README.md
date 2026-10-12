# 原生视图调用采集原型

新增 `--live-head` 受限实时头动测试，与其他视点测试互斥；需先启动带
`--render --bridge-pid <游戏PID>` 的 OpenXR 位姿生产者。仅在已有 Quest 3
真实头动验证后用于接入测试，旋转限制每轴 ±5°、平移长度 2 单位，
100ms 过期、失焦或跟踪丢失返回原视点；F8 中止，F9 回中。
使用独立 build-live/GearsCaptureLive.dll；说明和证据见 docs/14-live-head-camera-test.md。

这一工具在当前游戏进程里加载我们编写的 DLL，并临时 Hook 原生候选入口。它会改写函数入口，不属于纯只读调查；它保留原函数参数、返回值和视点数据，不加入位姿变换。

不加 `--wobble` 时上述采集行为保持不变。加 `--wobble` 开启受限的渲染 yaw 测试：只在匹配的当前玩家场景调用中改写临时旋转输出，最大 1.5°，基线 3 秒后摆动 12 秒，再停止；失去焦点会终止。CSV 增加 source_yaw、yaw_delta、cache_yaw、test_state，valid 的 64 位标志表示非零偏移已施加。游戏内验证见 `docs/05-render-yaw-wobble-validation.md`。

新增互斥的 `--sweep` 模式：3 秒基线，然后依次用 4 秒正弦波测试 pitch、yaw、roll、
水平镜头前方、水平镜头右方、世界向上。旋转上限 ±1.5°，平移上限 ±2 游戏单位。
共 27 秒后恢复干净视点。不是实时头显跟踪；尚未校准游戏单位与米的比例。
CSV 的 pose_phase=0 基线，1–6 对应六轴，7 恢复；额外记录源视点和全部偏移。
仍只修改最终视图读取的临时输出，不改 CameraCache 或人物位置。

```powershell
& $pythonExe 'J:\gearsVr\tools\native-capture\start_capture.py' --pid <当前游戏PID> --seconds 45 --sweep --build-dir 'J:\gearsVr\tools\native-capture\build-sweep'
```

## 依赖和构建

使用现有 Visual Studio x64 C++ 工具链及 Windows SDK。当前 `build.bat` 指向本机 `E:\vs`。第三方 [MinHook 官方项目](https://github.com/TsudaKageyu/minhook) 使用 v1.3.4，commit `c3fcafdc10146beb5919319d0683e44e3c30d537`；其源码位于忽略目录 `tools/third-party/minhook`。第三方源码和所有构建产物不提交到本仓库。

```powershell
& 'J:\gearsVr\tools\native-capture\build.bat'
& 'J:\gearsVr\tools\native-capture\build\CaptureSelfTest.exe' 'J:\gearsVr\tools\native-capture\build\GearsCapture.dll'
```

独立测试通过真实 MinHook trampoline 调用测试函数，检查六个参数、原函数产生的输出和返回指针、事件嵌套记录，以及采集到的 32 个矩阵元素。它验证 Hook 的转发机制；不能代替游戏 ABI 验证。

当前游戏内旧 DLL 会阻止覆盖构建产物。可用 `build.bat build-next` 在独立目录构建并自测；启动器和收集器接受 `--build-dir` 指向该目录。新构建仍需要新游戏进程，不能在已有采集 DLL 的进程中替换。源码增强版自测还核验停止后的入口字节和原函数再次执行。

诊断重试可使用 `build.bat build-next GearsCaptureNext` 和启动器的
`--dll-name GearsCaptureNext.dll`。已有同名 DLL 或活动采集一律拒绝；
不同名称也必须通过玩家与入口原始字节核验。这用于失败安装后的受限诊断重试，
不替换/卸载旧 DLL，不支持同时采集。常规测试仍优先新游戏进程。

本机碎片化地址空间导致原版 MinHook 1 GiB 搜索分配失败；CMake 校验源码标记后
生成保留许可证的 buffer.c 副本，将范围扩大到 1.5 GiB，保留 rel32 距离余量。
原第三方源码保持不变。实际失败与重试证据见 `docs/09-six-axis-render-validation.md`。

## 使用

启动游戏并进入单人关卡。调试启动器默认使用 `-NoPauseUnfocus`，切回聊天仍可继续渲染；需要暂停时手动操作。常规新一轮采集先关闭上次游戏进程；同名采集 DLL 拒绝重复加载。先通过摄像机定位器确认当前进程和版本，再执行：

```powershell
$pythonExe = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe 'J:\gearsVr\tools\native-capture\start_capture.py' --pid 32532 --seconds 120
```

启动器每次运行独立自测，再核对游戏哈希、当前对象关系和目标入口字节。DLL 内部另做哈希、PID、LocalPlayer.Actor 链接及入口字节验证，拒绝未知游戏版本或已被其他工具改写的入口。此版本没有通用配置化 Hook 目标，候选入口来自我们自己的反汇编调查。

开始后按明确安排切回游戏进行短测试。采集时间从 Hook 启用时开始，包含准备和暂停时间，不会等待游戏焦点。当前日志文件在采集期间可能被 CRT 锁定；启动器使用采集信号确认启动，结束后再读取完整文件。

```powershell
& $pythonExe 'J:\gearsVr\tools\native-capture\collect_capture.py' --output 'J:\gearsVr\analysis\runtime\native-capture-1.json'
```

## 记录内容

- 场景视图候选 `0x15cf08`：时间、线程、调用者、玩家对象、原函数返回的视图指针、位置、旋转，以及视图矩阵 / 投影矩阵候选。
- 玩家视点事件包装函数 `0x15c968`：时间、线程、调用者、控制器及原函数的两个输出。
- DXGI Present / Present1：提交对象、时间、线程、返回 HRESULT 和是否属于测试调用。使用无窗口的临时 D3D12 composition swapchain 找到方法入口；没有调用临时 swapchain 的 Present。
- 可选 `--gpu-diagnostics`：交换链 HWND、尺寸、格式、缓冲索引及资源 / 设备接口地址；ExecuteCommandLists 的队列、类型、设备与列表数量。只记录元数据，不复制 GPU 资源。模式与相机改写互斥，自测额外实际提交一个空命令列表。valid 128=设备接口读取成功，256=当前后缓冲资源读取成功；地址不是跨重启标识，也不证明交换链与队列绑定。

CSV 的 valid 是位标志：1=位置读取成功，2=旋转读取成功，4=矩阵读取成功，8=CameraCache 时间读取成功，16=对象与初始解析玩家匹配，32=非 DXGI_PRESENT_TEST 调用。不同类别只使用适用位。相同数值不代表已证明矩阵坐标约定；Present 与 Present1 可能嵌套，不应把计数相加当帧数。

回调只保存记录到有界队列，文件写入由工作线程完成。队列满或锁竞争时丢弃记录并计数，避免等待日志锁阻塞游戏。输出统计必须结合 dropped 数量解释，记录不是无损追踪。

## 集合提交诊断与早期跳板预留

`--family-diagnostics` 只读取原生集合提交参数，记录最多四个视图指针；与视点改写、GPU 诊断模式互斥。先正常关闭已有游戏，再运行 `launch_reserve.py` 启动新游戏并预留相机和集合入口跳板。预留不启用 Hook，入口字节由 DLL 和外部读取分别核验。进入关卡后使用：

```powershell
& $pythonExe 'J:\gearsVr\tools\native-capture\start_capture.py' --pid <当前PID> --seconds 20 --family-diagnostics --use-reserved --build-dir 'J:\gearsVr\tools\native-capture\build-lifetime' --dll-name GearsCaptureLifetime.dll
```

此时重新定位当前摄像机，复核版本、对象与入口，再开始短时采样。完成后先运行 `verify_stopped.py`，再用 `analyze_family.py --capture <JSON> --restored <恢复JSON> --output <结果JSON>` 检查同线程的前序玩家视图与提交数组元素是否对应。当前实测及局限见 `docs/23-render-family-investigation.md`。预留或采样失败均不支持在同一 DLL 重试，需正常退出游戏后重新开始。

新增副本观察构建位于 `build-clone/GearsCaptureClone.dll`，启动预留和采样都要指向同一构建目录及 DLL 名。它还预留 / 核验 0x193d50 复制入口，kind=8 记录构造前后矩阵和所属集合；六处入口可由恢复工具独立核验。注入前追加 `--clone` ABI 自测，采集后使用 `analyze_clone.py --capture <JSON> --restored <恢复JSON> --output <结果JSON>`，结果与范围见 `docs/24-render-view-copy-validation.md`。旧构建缺少新自测导出时应重新构建，不能直接用于此模式。

上一阶段构建为 `build-renderer/GearsCaptureRenderer.dll`。相比历史的 family / clone 构建，新增 0x116250 完整构造 Hook、`--renderer` 四参数自测及 kind=9 构造完成结果。用 `analyze_renderer.py` 核对副本存储和所属集合。收集时可传 `--pid <当前PID> --summary`，防止误收旧进程日志并缩短终端输出。原版对照结果见 `docs/25-renderer-construction-validation.md`。

当前构建为 `build-lifetime/GearsCaptureLifetime.dll`，`launch_reserve.py` 默认使用它；用 `build.bat build-lifetime GearsCaptureLifetime` 构建。新增消费 0x15a80c 与释放 0x15a974 的调用前观察（kind=10 / 11），原函数返回后不再读取游戏对象。`--lifetime` 自测会实际释放模拟对象页面。当前源码要求五项相关自测导出，历史 DLL 不能直接复用，新构建须重启游戏。采集后用 `analyze_lifetime.py --capture <JSON> --restored <恢复JSON> --output <结果JSON>` 检查生命周期；结果和范围见 `docs/26-renderer-consumption-and-lifetime.md`。停止工具核验七处原生与两处 DXGI 入口。

## 停止和边界

最新默认构建更新为 `build-prepare/GearsCapturePrepare.dll`，使用 `build.bat build-prepare GearsCapturePrepare`。新增 0x1fc060 前后观察（kind=12）和 `--prepare` 自测；当前源码要求六项相关自测，历史构建不能直接复用。上面的采样命令应改用这个目录及 DLL。停止工具现在核验八处原生、两处 DXGI 入口。使用 `analyze_prepare.py --capture <JSON> --restored <恢复JSON> --output <结果JSON>` 检查准备与消费、释放的嵌套关系，见文档 28。

完成后可用 `verify_stopped.py --pid 当前PID --status 采集.status.txt --output 恢复检查.json` 独立读取入口进行核对。仅接受匹配 PID 的 clean stopped 状态；相机使用运行时版本签名，系统图形模块使用磁盘字节并应用重定位。检查不写内存、不卸载 DLL，仅覆盖已记录入口。新增 GPU 模式的结果和下一步见 `docs/17-native-gpu-observation.md`。

时间到后自动撤销 Hook 入口补丁、关闭日志。DLL 和 trampoline 留在进程中，直到游戏退出，避免正在返回或延迟执行的代码访问已经释放的地址。不会自动卸载 DLL。停止失败会写入状态，不能将失败状态解释为补丁已经撤销。

也可由工具设置 `Local\GearsVR.Capture.Stop.<PID>` 命名事件提前停止。当前版本不提供同一 DLL 重复启停或在同一游戏进程里替换 DLL 的功能。

完成 sweep 后用 `analyze_sweep.py <CSV> --output <JSON>` 复核六轴变化、调用者、
焦点、矩阵和恢复阶段。程序不会读取或写入游戏。当前成功记录见上述六轴报告。

每帧的额外记录有运行开销，独立自测通过也不能保证未知游戏分支一定安全。动态成功只证明实际观察到的路径；菜单 / 焦点暂停不等于 VM 卡顿，采集不验证立体渲染、6DoF 或摄像机改写效果。
