# 原生 D3D12 队列绑定与单帧回读

2026-10-12，主线恢复。测试条件：已安装用户下载的 First Person 1.0，保留现有跳过片头 / tuneables 与 `-NoPauseUnfocus` 启动参数。此条件不同于先前原版相机测试，不混用相机位置、FOV 或骨骼表现的结论。

## 队列绑定：已证明

原先只观察到交换链与队列返回同一设备。本轮在启动时 Hook 三种交换链创建入口，记录创建输入的 D3D12 Direct 队列与返回的交换链，并通过规范 IUnknown 接口地址匹配后续 Present。不同 COM 接口裸地址可能不同，不能直接假设相等。

成功游戏样本 PID 46712，45 秒：

- 游戏使用 `CreateSwapChainForHwnd`，游戏调用者返回 RVA `0x45355f`。
- 交换链尺寸 1920 × 1080，格式 24（R10G10B10A2_UNORM）。
- 捕获一个有效创建绑定，关联的交换链随后出现 10,799 次非 TEST Present。
- 绑定窗口归属该游戏进程、队列类型 Direct；这是创建输入与实际显示交换链的对应证据。
- 日志丢弃 0；停止后 active=0，四个入口在 DLL 内与外部独立读内存核验中均恢复。

地址仅用于本轮匹配，每次启动重新记录，不将已退出进程的队列地址写成定位配置。

启动时第一次提前加载观察 DLL 未能确认成功，游戏保持正常运行；随后独立模块清单确认没有该 DLL，较晚的受控加载成功。新启动流程等待 dxgi.dll / d3d12.dll 已加载，再接入观察代码，尚不要求出现窗口或进入关卡。此修改后的新启动成功捕获创建。不能由此断言第一次失败的 Windows 错误码或唯一根因已确定。

独立自测已实际调用 CreateSwapChain、CreateSwapChainForHwnd、CreateSwapChainForComposition，核对创建参数转发、返回接口匹配及停止后的入口字节。没有覆盖 CoreWindow 或其他接口的创建路径。

## 单帧 GPU 回读设计

新增可选 `--readback`，只在已记录创建绑定的游戏窗口交换链上取一帧：

1. 创建时保留交换链与队列 COM 引用，工作线程准备专用 readback buffer、命令列表、allocator 和 fence。
2. 绑定至少 15 秒后，在匹配交换链的 Present 之前取得当前后缓冲。
3. 在创建时绑定的同一队列提交 PRESENT → COPY_SOURCE、复制、COPY_SOURCE → PRESENT。
4. Present 回调不等待 GPU，不写磁盘。工作线程确认 fence 完成后 Map、转换和保存。
5. GPU 完成后释放游戏后缓冲引用，避免妨碍后续 ResizeBuffers。单次捕获后不再提交复制。
6. 停止时撤销入口补丁，再确认活动回调退出；若提交后同步失败，保留可能在使用的 GPU 资源到进程退出，不强制释放。

支持本轮 packed 10-bit RGB 及测试用 8-bit RGBA/BGRA。10-bit→8-bit 做分量解包与四舍五入，不包含 HDR PQ / 色彩空间变换。它是截图诊断，不是最终 GPU 纹理共享方案，也不是双眼渲染。

GPU 自测使用 19 × 13 的非对齐宽度，实际清屏为已知颜色，再执行复制 / fence / Map，对全部像素核验，覆盖 8-bit 和 10-bit 两种格式及 row pitch 填充，两项通过。

真实游戏首次回读 PID 44240，45 秒：1920 × 1080、格式 24、当前后缓冲 index=1，GPU fence 完成并保存 640 × 360 PPM，HRESULT=0。图片检查确认为游戏开始界面，包含正常的红白 Logo 与提示文字；这证明原生 GPU 画面读取链路，但不是关卡或连续 VR 显示验证。对应创建绑定后观察到 10,857 次 Present，丢弃 0，资源释放成功，四个入口外部独立核验恢复。此旧样本尚未记录 Present 失败计数，不能据此断言每次 Present 均成功。

新增 `--manual-readback`，创建一个带游戏 PID 的本机会话事件。进入关卡并检查游戏窗口截图后，使用 `request_readback.py --pid 当前PID` 请求下一次匹配 Present 的单次复制，避免固定 15 秒只取到开始菜单。请求器验证游戏路径与已加载观察 DLL；请求成功仅代表事件已发送，最终以回读日志和图片为准。停止时若回调未退出，保留资源并报告失败，不释放回调可能使用的对象。

随后 PID 10720 使用手动请求模式，由窗口定向确认消息逐步进入 Continue Campaign，每次确认前检查低清游戏窗口截图。进入关卡后请求回读，保存图像检查确认为实际石柱、走廊与烛光场景，与窗口采集的场景一致，不含桌面。该轮没有移动人物或修改相机。相机反射定位仍成功，CameraCache=0x3ac、POV=0x4、PlayerCamera=0x37c 等布局一致；采样瞬间 CameraCache FOV=80，不能将 Mod 默认属性中的 105/120 当作所有时刻的实际 FOV，也不据此证明最终渲染 FOV。

此轮 180 秒最终日志：创建绑定确认，54,394 次非 TEST Present，Present 失败 0，记录丢弃 0，回读成功（1920 × 1080、格式 24、index=1、HRESULT=0），资源释放成功、active=0，四个入口外部独立读内存均恢复。游戏进程仍响应。这只证明此轮有界单帧诊断成功，没有持续复制或头显显示测试。

实现遵循 [Microsoft 交换链说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/swap-chains)与 [Readback heap 说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps)：Present 队列来自创建参数；Map 本身不等待 GPU，必须通过 fence 确认复制完成。

## 重现

```powershell
& tools/native-capture/build.bat build-binding GearsCaptureBindingUnused
& $pythonExe tools/native-capture/launch_binding.py --seconds 45 --build-dir tools/native-capture/build-binding
& $pythonExe tools/native-capture/collect_binding.py --pid 当前PID --build-dir tools/native-capture/build-binding --output analysis/runtime/binding-当前PID.json
```

GPU 单帧回读在启动命令中加 `--readback`。启动器拒绝已有游戏进程，不强制杀游戏；游戏由现有调试启动器启动，先版本哈希核验，DLL 内独立核验 EXE。F8 可提前结束观察。

需要在指定场景回读时，改用 `--manual-readback --seconds 180`，在观察结束前运行 `request_readback.py --pid 当前PID`。采集器同时要求创建绑定、回读成功（若启用）、资源释放与四个入口独立恢复核验；失败时返回非零，不把成功启动视为完成验证。

DLL / trampoline 到游戏退出才卸载。同进程不热替换 DLL。原始 JSONL、图片和本机地址保存在 Git 忽略的 `analysis/`，仓库保存代码与文字结论。
