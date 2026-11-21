# Original Continuous GPU Image Sharing

2026-10-12. First person name Mod to keep installation, image settings 1920 × 1080; this is a road-access study, not a two-eye stereo.

## Mechanisms

`gpu_share.h` Use one shared DEFAULT texture and two shared Ds on proven create queue/ exchange chain binding 3D 12 Set. The side of the game is copied before matching Present, and then recovered with Present, shared texture COMMON, and Signal ready set. Receiver uses the same independent D card.3D 12 DIVIce, open shared texture and fence, Wait ready, copy, restore COMMON, Signal done.

Sendend only writes the next frame when previous serial number is done and allocator commands are reusable; otherwise skipping does not block the game waiting for the receiving end. Current copy interval at least 16 ms, single-cell design is the correct prototype and final ingestion/ delay has not been assessed. Create resource, Map metadata and shut down resource is on the workspace; Present does not wait for GPU, does not write disks. The workspace is buffered after release of the reservation after GPU completion.

The shared object name contains the producer PID; metadata check the mass / version / size / PDID / acceptance LUID/ texture description. Only one recipient-end identification is allowed, and no heat replacement is allowed after the receiving end exits to avoid having to go with the old GPU command. New testing uses a new game process. Size / format change report failed, recreated automatically after Resize Buffers is not supported.

Cancel Hook and confirm the activity's callback exit when you stop sending, then mark the shared channel to stop, pending the final submission of your own GPU to release the resource. If this fails, keep the COM object that may be in use in the GPU to the process exit.

## Authenticated

- Two separate processes, each format 12 Frame,19 × 13 (a) Non-regular width;8-bit RGBA and game 10-bit RGB format is passed. Each frame colour changes, the receiving end-by-end pixels compares to the expected value, checking all frames and line fills.
- The first frame of the receiving end is deliberately delayed. 150 ms, a strip occurs at the end of the sending; the duplicate receiver is rejected. Both formats return the recipient 0 And the release of resources is successful.
- Practical Game PID 32616: Observation 180 Seconds, take them in when you enter the level. 20 sec. Game release 324 Frame, receive. 323 Frame, content changes 322 Last received serial number 323. The extra frame sent is that there is still one release after the acceptance test, which is not considered to be a lost frame.
- 1920 × 1080，R 10G 10B 10A 2_UNORM. Receiving port header save 640 × 360 Diagnosis maps, manual checks identified as actual stone pillars, corridors, candlelight scenes, consistent with the cut-off of independent game windows.
- Game Record 55,074 Second Present, failed 0 Logs are abandoned 0;sharing status ready, HRESULT=0、busy_skips=10,812. This figure includes a single slot waiting after the receipt is slower than the dispatch and exit of the receiving procedure, which cannot be converted to a separate loss of performance.
- After Stop 0 The release of resources was successful and the external access points were independently read bytes restored.

The diagnostic receiver is still read back to CPU by the texture of the GPU it receives for verification and retention of the frame; the game  ⁇  recipient transfer itself does not have CPU image transit. The diagnosis receiver cannot be read 323 / 20 The government has been able to provide a complete picture of the situation.

Local evidence:`analysis/runtime/gpu-share-32616.{json,jsonl,ppm,png}`、`gpu-share-consumer-32616.txt`。

## OpenXR receiver

Add `game_gpu_screen.h` / `run_pose.py --gpu-game`, copy the received GPU texture directly to the same format OpenXR swapchain; convert to available GPS shadow if the original format is not supported 8-bit SDR format, then submit two-dimensional Quad with eyes visible. No WGC, CPU image rereading or scaling. SRGB target uses decoded RTV encoding to keep coded SDR input; no HDR is available. Processed through Meta XR Simulator 30 Second test,1,112 Sub-GPU copy,2,130 Subquad submission, normal exit; details [Emulator records](22-openxr-simulator.md). The current round of connections is not visible and this new route has not yet been validated in Quest 3 .

Cross-process 10-bit  ⁇  sRGB shadow also passed 12 The frame is known to be colour-by-pixel verification, and the tolerance is different. 1; slow and repeat receiver ends refuse to pass. The machine does not provide D 3D 12 debug layer。`--gpu-snapshot` For an optional receiving-end return, do not change the normal GPU transfer per frame.

The game does not have a completion line for sight nodes, retrospects, real delay and comfort.

## Rewind

```powershell
& tools/native-capture/build.bat build-share GearsCaptureShareUnused
& tools/native-capture/build-share/GpuShareTest.exe
& $pythonExe tools/native-capture/launch_binding.py --seconds 180 --gpu-share --build-dir tools/native-capture/build-share
# 在关卡中运行诊断接收端（与 OpenXR 接收端二选一）：
& tools/native-capture/build-share/GpuShareTest.exe --consume 当前PID 0 analysis/runtime/gpu-share-当前PID.ppm
& $pythonExe tools/native-capture/collect_binding.py --pid 当前PID --build-dir tools/native-capture/build-share --output analysis/runtime/gpu-share-当前PID.json
```

OpenXR receiverend running on the new game-sharing channel `run_pose.py --seconds 30 --gpu-game --runtime 运行时清单路径`. A channel can only be claimed by one receiving end, running the diagnostic receiver and not reconnecting to OpenXR; normal exit is required and the game is restarted.

References [CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle) and [OpenSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-opensharedhandle)。

---

## 中文

# 原生连续 GPU 画面共享

2026-10-12。第一人称 Mod 保持安装，画面设置 1920 × 1080；这是显示通路研究，不是双眼立体。

## 机制

`gpu_share.h` 在已证明的创建队列 / 交换链绑定上使用一个共享 DEFAULT 纹理和两个共享 D3D12 fence。游戏侧在匹配 Present 前复制后缓冲，恢复后缓冲 PRESENT、共享纹理 COMMON，再 Signal ready fence。接收端使用相同显卡的独立 D3D12 device，打开共享纹理和 fence，Wait ready、复制、恢复 COMMON、Signal done。

发送端仅在 done fence 已完成上一序号、自己的命令 allocator 可复用时写下一帧；否则跳过，不阻塞游戏等待接收端。当前复制间隔至少 16 ms，单槽设计是正确性原型，尚未评估最终吞吐 / 延迟。创建资源、Map 元数据和关闭资源在工作线程；Present 不等待 GPU、不写磁盘。工作线程在 GPU 完成后释放保留的后缓冲引用。

共享对象名称包含生产者 PID；元数据核验 magic / version / size / PID / adapter LUID / 纹理描述。只允许一个接收端认领，接收端退出后不允许热替换，以免与旧 GPU 命令并发。新测试使用新游戏进程。尺寸 / 格式变化报告失败，暂不支持 ResizeBuffers 后自动重建。

停止发送时先撤销 Hook 并确认活动回调退出，再标记共享通道停止，等待最后一份自身 GPU 提交完成，释放资源。失败时保留可能在 GPU 使用中的 COM 对象至进程退出。

## 已验证

- 两个独立进程，每种格式 12 帧，19 × 13 非对齐宽度；8-bit RGBA 和游戏的 10-bit RGB 格式均通过。每帧颜色改变，接收端逐像素对照预期值，检查全部帧及行填充。
- 接收端第一帧故意延迟 150 ms，发送端发生跳帧；重复接收端被拒绝。两种格式接收程序均返回 0，资源释放成功。
- 实际游戏 PID 32616：观察 180 秒，进入关卡后接收 20 秒。游戏发布 324 帧，接收 323 帧、内容变化 322 次，最后接收序号 323。发送多出一帧是接收测试结束后仍有一次发布，不视为全程无丢帧。
- 1920 × 1080，R10G10B10A2_UNORM。接收端首帧保存 640 × 360 诊断图，人工检查确认为实际石柱、走廊、烛光场景，与独立游戏窗口截图一致。
- 游戏记录 55,074 次 Present，失败 0，日志丢弃 0；共享状态 ready、HRESULT=0、busy_skips=10,812。该数字包括接收慢于发送及接收程序退出后的单槽等待，不能单独换算性能损失。
- 停止后 active=0，资源释放成功，四处入口外部独立读取字节均恢复。

诊断接收器为了核验和保存首帧，仍将其接收到的 GPU 纹理回读到 CPU；游戏 → 接收端的传输本身没有 CPU 图像中转。不能将诊断接收器的 323 / 20 当作最终头显通路性能。

本地证据：`analysis/runtime/gpu-share-32616.{json,jsonl,ppm,png}`、`gpu-share-consumer-32616.txt`。

## OpenXR 接收端

新增 `game_gpu_screen.h` / `run_pose.py --gpu-game`，直接将接收到的 GPU 纹理复制到同格式 OpenXR swapchain；不支持原格式时用 GPU shader 转换到可用 8-bit SDR 格式，再提交双眼可见的二维 quad。无 WGC、每帧 CPU 图像回读或缩放。sRGB 目标采用解码后 RTV 编码以保持已编码 SDR 输入；尚无 HDR 处理。已通过 Meta XR Simulator 的 30 秒测试，1,112 次 GPU 复制、2,130 次 quad 提交，正常退出；详见 [模拟器记录](22-openxr-simulator.md)。用户本轮不方便连接头显，尚未验证此新通路在 Quest 3 中显示。

跨进程 10-bit → sRGB shader 转换也通过 12 帧已知颜色逐像素核验，容差 1；慢接收和重复接收端拒绝仍通过。机器未提供 D3D12 debug layer。`--gpu-snapshot` 为可选的一次接收端回读，不改变正常每帧 GPU 传输方式。

游戏视点头动、每眼场景渲染、真实延迟与舒适度不属于本轮完成项。

## 重现

```powershell
& tools/native-capture/build.bat build-share GearsCaptureShareUnused
& tools/native-capture/build-share/GpuShareTest.exe
& $pythonExe tools/native-capture/launch_binding.py --seconds 180 --gpu-share --build-dir tools/native-capture/build-share
# 在关卡中运行诊断接收端（与 OpenXR 接收端二选一）：
& tools/native-capture/build-share/GpuShareTest.exe --consume 当前PID 0 analysis/runtime/gpu-share-当前PID.ppm
& $pythonExe tools/native-capture/collect_binding.py --pid 当前PID --build-dir tools/native-capture/build-share --output analysis/runtime/gpu-share-当前PID.json
```

OpenXR 接收端在新的游戏共享通道上运行 `run_pose.py --seconds 30 --gpu-game --runtime 运行时清单路径`。一个通道只能由一个接收端认领，先跑诊断接收端后不能再接 OpenXR；需要正常退出并重开游戏。

参考 [CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle) 与 [OpenSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-opensharedhandle)。
