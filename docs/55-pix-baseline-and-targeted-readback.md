# PIX Original Path Contrast to Point Read Back Precondition

2026-10-13. Keep the same frame multiple view main line and double-render it in the same frame order as an option to exclude AER. This round does not enable new rendering Hook, without repairing the right black screen.

## Controlled PIX startup

Normally close the last round of pre-enclosed PIDs 44960, CloseMainWindow and waiting to exit successfully. Use this machine PIX 2603.25 official pixtool, directly start the known game path with parameters `-NoPauseUnfocus`, requests a frame of capture. This round does not call VR DLL injection or the old RenderDoc import replacement script.

Result: pixtool exit code 1, not generated `baseline-20261013.wpix`。Windows Application Error 1000 Records the game crashes in relation to start-up time:

- Time:2026-10-13 15:33:04（Asia/Shanghai）。
- Game PID:44060"In the event `0xac1c`;in the log 50016 is a PIX-assisted process that should not be misconceived as a game PID.
- Unusual:`c0000005`。
- Modules:`C:\Windows\System32\d3d12.dll`Version `10.0.26100.9278`。
- module offset:`0x10538`。

Original evidence is stored in `analysis/pix/baseline-20261013.log` and `baseline-20261013-crash.xml`. WeR Report.wer access was denied to the operating system, no permissions were changed or read successfully, and therefore no crashing anchor and complete load list of modules was obtained.

This narrows the failure to capture D during startup.3D 12 An anomaly cannot be identified as the source of the anomaly being a system DLL, game protection or PIX package. The path to AgySDK in the log is a PIX-assisted process, nor can it be used as evidence that the version was loaded into the game. The front-office permission warning appears in the history successful smoke capture log and this failure cannot be explained by the warning alone.

## Normal startup contrast

Same EXE, work directory and `-NoPauseUnfocus` Parameters are directly started, PIX or VR injection is not used, PID 37692 Normal access to the level. Window screenshot confirms world geometry:`analysis/game-io/frames/20261013-153429-0545ccfe.jpg`。

Module records `analysis/pix/baseline-direct-modules-37692.json` Show dxgi,d 3d 12、D 3D 12 Core from Systems 32. The game is kept running. This is a contrast to support the capture start-up problem and does not prove that all normal starts are stable with all levels.

## Intermediate Texture Rereading cannot directly reset backbuffer

Existing `gpu_readback.h` OneShotReadback relies on the swapchain/ queue binding obtained at creation, obtains the current backbuffer before Present, copys the PRESENT  ⁇ COPY_SOURCE  ⁇  PRISEENT records and reads them after the fence is finished. It only supports specified entries 8/10 bit colour format.

The middle scene colour may have different status, format, subresources and cross-queuing, and cannot be used directly after replacing the source pointer. The current view tracker records the RTV description, resource address and drawing area without recording the full resource barrier, SLV binding and queue submission. The current information is therefore not sufficient to perform a secure and accurate mid-texting back.

The subsequent site test needs to be determined:

1. The final processing of the resource, format and mip/array slice that is actually read, rather than recording only the RTV it has written down.
2. The list of commands and queues that produce this element confirm the reading time point, actual resource status and synchronised dependency.
3. Copy to the independent readback resource before content is overwritten, restoring the known status; retain source resources until GPU is complete and do not wait for or write a file while drawing a thread.
4. Keeps the same test/framemark, eye/retroverse, resource and subresource identity, copy point and finish fence for output. Unknown status, description or tracking spill should be refused.

## Next set of directed static analysis targets.

Snippled while running `d82f58` The following calls exist before the end quad. Checked against the address of.asm:

| Call Point RVA | Target RVA | Purpose of analysis |
| --- | --- | --- |
| `d831aa` | `2754f4` | Check final processing parameters/kiding readiness |
| `d831c3` | `1665dc` | Quote current view-related objects, and track each view input source as a priority |
| `d83260/d832ef/d83373` | `1b4ca0` | Check whether the global/object data that is being fed is texture, sampler or constant |

These are candidate input preparation paths that have not been back-compiled to confirm callee and have not been created to D 3D 12 SSV maps cannot be named as recognized SceneColor binding functions. The next round of orientation analysis of these functions avoids extending to full EXE or adding blind Hook.

References:[Microsoft pixtool document](https://devblogs.microsoft.com/pix/pixtool/). This command format and option check the installation version simultaneously `--help launch`、`--help take-capture`。

---

## 中文

# PIX 原版路径对照与定点回读前置条件

2026-10-13。保持同帧多视图主线，同帧顺序双渲染为备选，排除 AER。本轮未启用新渲染 Hook，未修复右侧黑屏。

## 受控 PIX 启动

正常关闭上一轮仅预留入口的 PID 44960，CloseMainWindow 和等待退出均成功。使用本机 PIX 2603.25 的官方 pixtool，直接启动已知游戏路径，参数为 `-NoPauseUnfocus`，请求一帧捕获。本轮没有调用 VR DLL 注入工具或旧 RenderDoc 导入替换脚本。

结果：pixtool 退出码 1，未生成 `baseline-20261013.wpix`。Windows Application Error 1000 记录了对应启动时段的游戏崩溃：

- 时间：2026-10-13 15:33:04（Asia/Shanghai）。
- 游戏 PID：44060（事件中的 `0xac1c`）；日志中的 50016 是 PIX 辅助进程，不应误认为游戏 PID。
- 异常：`c0000005`。
- 模块：`C:\Windows\System32\d3d12.dll`，版本 `10.0.26100.9278`。
- 模块内偏移：`0x10538`。

原始证据保存在 `analysis/pix/baseline-20261013.log` 和 `baseline-20261013-crash.xml`。WER Report.wer 访问被操作系统拒绝，没有更改权限或读取成功，因此没有取得崩溃调用栈与完整加载模块列表。

这把失败范围缩小到捕获启动期间的 D3D12 异常，不能据此确认异常源头是系统 DLL、游戏保护或 PIX 包装。日志中 AgilitySDK 的路径属于 PIX 辅助进程，也不能当成游戏加载了该版本的证据。前台权限警告同时出现在历史成功 smoke 捕获日志中，单凭该警告不能解释本次失败。

## 普通启动对照

相同 EXE、工作目录与 `-NoPauseUnfocus` 参数直接启动，未使用 PIX 或 VR 注入，PID 37692 正常进入关卡。窗口截图确认世界几何正常：`analysis/game-io/frames/20261013-153429-0545ccfe.jpg`。

模块记录 `analysis/pix/baseline-direct-modules-37692.json` 显示 dxgi、d3d12、D3D12Core 来自 System32，筛选范围内没有 PIX、RenderDoc 或 GearsCapture 模块。游戏保留运行。这是一次对照，支持捕获启动相关问题，不能证明所有普通启动与全部关卡稳定。

## 中间纹理回读不能直接复用 backbuffer 假设

现有 `gpu_readback.h` 的 OneShotReadback 依赖创建时取得的 swapchain/queue 绑定，在 Present 前取得当前 backbuffer，按 PRESENT → COPY_SOURCE → PRESENT 记录复制，并由 fence 完成后读取。它只支持指定的 8/10 位颜色格式。

中间场景颜色可能具有不同状态、格式、子资源和跨队列使用，不能把其中的源指针替换后直接使用。现有 viewport tracker 记录 RTV 描述符、资源地址和绘制区域，不记录完整资源屏障、SRV 绑定与队列提交关系。因此当前资料还不足以安全、准确地执行中间纹理回读。

后续定点实验需要先确定：

1. 最终处理真正读取的资源、格式及 mip/array slice，而不是只记录它写入的 RTV。
2. 产生该内容的命令列表和提交队列，确认读取时点、实际资源状态及同步依赖。
3. 在内容被覆盖前复制到独立读回资源，恢复已知状态；保留源资源直到 GPU 完成，不在绘制线程等待或写文件。
4. 为输出保存同一试验/帧标识、眼睛/矩形、资源及子资源身份、复制时点与完成 fence。未知状态、描述符丢失或跟踪溢出应拒绝复制。

## 下一组定向静态分析目标

运行时快照的 `d82f58` 在最终 quad 前存在下列调用，已对照 .asm 核实地址：

| 调用点 RVA | 目标 RVA | 分析目的 |
| --- | --- | --- |
| `d831aa` | `2754f4` | 核对最终处理参数/绑定准备 |
| `d831c3` | `1665dc` | 入参包含当前 view 与 shader 相关对象，优先追踪每视图输入来源 |
| `d83260/d832ef/d83373` | `1b4ca0` | 核对被传入的全局/对象数据究竟是纹理、采样器还是常量 |

这些是候选输入准备路径，尚未反编译确认被调用者，也未建立到 D3D12 SRV 的映射，不能命名为已确认的 SceneColor 绑定函数。下一轮先定向分析这些函数，避免扩大到全 EXE 或新增盲目 Hook。

参考：[微软 pixtool 文档](https://devblogs.microsoft.com/pix/pixtool/)。本次命令格式和选项同时核对了安装版本的 `--help launch`、`--help take-capture`。
