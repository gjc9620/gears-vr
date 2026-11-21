# Game original GPU render object observation

2026-10-12 Current version single-person level, PID 28720，20 Seconds are collected. This cycle does not change the lens, does not copy or modify the GPU resource, and does not submit game texture to the header.

| Object | This is the evidence. |
| --- | --- |
| Game exchange chain | 4,328 Second Present, same object, same game HWD,1920 × 1080 |
| Back buffer | Two resources, current index 0 / 1,Format Value 24（R 10G 10B 10A 2_UNORM） |
| D 3D 12 Equipment | Switch Chain & Command Line GetDevice back to the same interface address |
| Command Queue | A Direct line that has been observed,87,083 Bar saved ExecuteCommandLists records from two threads |
| Native View | 5,845 A strip view record with all arrays limited; this round of players is changing |
| Stop | disable_status=0，active=0; independently read five portals, all matching expected bytes after recovery |

Queue the game caller for the portal to return RVA as `0x1aa4fc`, Precent's game caller returns RVA as `0x21f955`. Normal code shows submission points from `[rdi+8]` The object in question `+0x60` Fields get queued and then call vtable `+0x50`These are current versions of the investigation trails that have not yet formed a queue locator to be used across the rebooting line.

The collection uses log lines, which are also used to create a web-based online network.50 The record is discarded because of locked competition or queues.87,083 is the number of records saved; statistics are not a no-lose GPU time line. Maximum call interval 0.35 sec, it cannot judge the script VM pause or diagnostic cost alone. No performance comparison of open / closed diagnosis is performed.

## Basis for the next step

"System with equipment, only one Direct queue" does not prove that the queue is a Present queue tied to the creation of the exchange chain.3D 12 The exchange creation parameter flows into the Direct command queue, and Present runs the queue specified when it is created. Next steps are to record the queue  ⁇  swapchain binding at the time of the creation of the exchange chain or prove this relationship from the game's creation path. Reference:[CreateSwapChainForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd)、[D3D12 swap chains](https://learn.microsoft.com/en-us/windows/win32/direct3d12/swap-chains)。

Only when confirmed can you arrange a resource state transition with the queue, and GPU copying and fence sync. This round buffer is 10-bit packed RGB, not directly from the previous window. 8-bit BGRA。

Original GPU image transmission is still pending; currently validated images are collected from Windows game windows and displayed in a 2-D panel. Left and right eye renderings are still pending.

## Recurrence and evidence

Add `--gpu-diagnostics` Independent self-measurement actually submitted an empty D before startup.3D 12 Command list to check the access of Hook forwarding, recording and stopping; game collection continues through version Hashi, player relationship and camera entry.

```powershell
& $pythonExe tools/native-capture/start_capture.py --pid 28720 --seconds 20 --gpu-diagnostics --build-dir tools/native-capture/build-gpu --dll-name GearsCaptureGpu.dll
& $pythonExe tools/native-capture/collect_capture.py --build-dir tools/native-capture/build-gpu --output analysis/runtime/gpu-28720.json
& $pythonExe tools/native-capture/verify_stopped.py --pid 28720 --status analysis/runtime/gpu-28720.status.txt --output analysis/runtime/gpu-28720-restoration.json
```

The PID, DLL name and directory are only examples of this cycle. DLL is loaded even if stopped and locked by Windows, and cannot be overwritten; subsequent revisions to the current round will be verified independently `build-gpu-next/GearsCaptureGpuNext.dll` Build and self-measure, no more infusion. DLL and Trumpoline are released when the game exits.

The original evidence on this machine is located in the `analysis/runtime/gpu-28720.{csv,json,status.txt}` and `gpu-28720-restoration.json`, do not enter Git. Independent access check for running-time signature validated with version of camera, for system DXGI/ D 3D 12 Core uses disk PE bytes and applies DIR 64 Repositioning; only the specified entry is verified and cannot represent the entire module.

---

## 中文

# 游戏原生 GPU 渲染对象观察

2026-10-12，当前版本单人关卡，PID 28720，20 秒有界采集。此轮不改镜头，不复制或修改游戏 GPU 资源，不向头显提交游戏纹理。

| 对象 | 本轮证据 |
| --- | --- |
| 游戏交换链 | 4,328 次 Present，同一对象、同一游戏 HWND，1920 × 1080 |
| 后缓冲 | 两个资源，当前索引 0 / 1，格式数值 24（R10G10B10A2_UNORM） |
| D3D12 设备 | 交换链与命令队列 GetDevice 返回同一接口地址 |
| 命令队列 | 一个已观察到的 Direct 队列，87,083 条已保存 ExecuteCommandLists 记录，来自两个线程 |
| 原生视图 | 5,845 条场景视图记录，矩阵全部有限；这一轮玩家视点在变化 |
| 停止 | disable_status=0，active=0；独立读取五个入口，全部匹配恢复后的预期字节 |

队列提交入口的游戏调用者返回 RVA 为 `0x1aa4fc`，Present 的游戏调用者返回 RVA 为 `0x21f955`。运行时普通代码显示提交点从 `[rdi+8]` 所指对象的 `+0x60` 字段取得队列，再调用 vtable `+0x50`（ExecuteCommandLists）。这些是当前版本调查线索，尚未形成可跨重启使用的队列定位器。

采集采用有界日志队列，50 条记录因锁竞争或队列满而丢弃。87,083 是已保存记录数；统计不是无损 GPU 时间线。最长调用间隔约 0.35 秒，单凭它不能判断脚本 VM 停顿或诊断开销。没有做开启 / 关闭诊断的性能对照。

## 下一步的依据

“同设备、只观察到一个 Direct 队列”不能证明该队列就是交换链创建时绑定的 Present 队列。D3D12 的交换链创建参数传入 Direct 命令队列，Present 运行于创建时指定的队列。下一步要记录交换链创建时的 queue → swapchain 绑定，或从游戏创建路径证明这一关系。参考：[CreateSwapChainForHwnd](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd)、[D3D12 swap chains](https://learn.microsoft.com/en-us/windows/win32/direct3d12/swap-chains)。

确认后才能安排同队列的资源状态切换、GPU 复制与 fence 同步。本轮缓冲是 10-bit packed RGB，不能直接当成先前窗口采图的 8-bit BGRA。

原生 GPU 图像传输仍待实现；当前头显中已验证的画面来自 Windows 游戏窗口采集，并以二维面板显示。左右眼分别渲染也仍待实现。

## 重现与证据

新增 `--gpu-diagnostics` 与其他相机测试模式互斥。启动前独立自测实际提交一个空 D3D12 命令列表，检查 Hook 转发、记录和停止后的入口恢复；游戏采集继续通过版本哈希、玩家关系与相机入口核验。

```powershell
& $pythonExe tools/native-capture/start_capture.py --pid 28720 --seconds 20 --gpu-diagnostics --build-dir tools/native-capture/build-gpu --dll-name GearsCaptureGpu.dll
& $pythonExe tools/native-capture/collect_capture.py --build-dir tools/native-capture/build-gpu --output analysis/runtime/gpu-28720.json
& $pythonExe tools/native-capture/verify_stopped.py --pid 28720 --status analysis/runtime/gpu-28720.status.txt --output analysis/runtime/gpu-28720-restoration.json
```

PID、DLL 名称与目录仅是本轮示例。已加载 DLL 即使停止仍被 Windows 锁定，不能覆盖；本轮后续模式校验修订使用独立 `build-gpu-next/GearsCaptureGpuNext.dll` 构建和自测，未再注入游戏。DLL 和 trampoline 到游戏退出才释放。

本机原始证据位于 `analysis/runtime/gpu-28720.{csv,json,status.txt}` 和 `gpu-28720-restoration.json`，不进入 Git。独立入口检查对相机使用版本验证的运行时签名，对系统 DXGI / D3D12Core 使用磁盘 PE 字节并应用 DIR64 重定位；仅验证指定入口，不能代表整个模块。
