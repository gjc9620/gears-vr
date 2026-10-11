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
