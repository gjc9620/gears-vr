# The final output call and regional parameters are observed.

2026-10-12，PID 16996 original level, DLSS=False, FSR 2/FSR 3/MFSR=False. Still not having been accepted correctly for binary pixels.

## Evidence of direct parameters

Continue only decoded from captured call pads `1af42c`: This function uses the first eight float-point parameters to form target and texture ranges, and the last four integer parameters are integrated into windows/texture dimensions, generating four vertexes, calling DrawIndexedInstanced.`d83b17` The call point uses the view float area as the target area, and the integer region as the texture area.

Add an optional view of the entry in the viewport diagnosis: original parameters are forwarded as they appear, only the test consumer function is recorded as a linear process and the call point is in place `d82f58..d83db9` call.64 A heliogram of fixed capacity, with a clear spill report. The new entry is bound by EXE Hashi and the original command signature, which is preloaded only for trampoline, not enabled; independent matching of signatures after stop.

10 Second test record:

| Target area | Texture Area | Window Dimensions | Texture Size | Number of calls |
| --- | --- | --- | --- | ---: |
| 0,0,960,1080 | 0,0,960,1080 | 1920,1080 | 1920,1080 | 1,531 |
| 960,0,960,1080 | 960,0,960,1080 | 1920,1080 | 1920,1080 | 1,531 |

Call return point is both `d83b1c`. The right view actually moves into the final area to draw, and not only the object of the processing. The target is correct with the parameters of the source area, nor does it prove that the separator's binding texture, constant, resource content or ultimate pixels are correct.

Present round of submission/consumption/preparation/destruction 1,531,CSV Construct Range 9.9939195 sec; life cycle analysis passed, status released, pond empty.22 The analysis was not considered to be a 3D image.

## Authentication and Failed Record

- New ABI self-measures that the sender and stack parameters are transmitted as they appear, non-test call is not recorded, after stopping through a function of eight float and four unsigned parameters in the real MinHook packaging 32 Byte entrance resumes.
- Both initiation and official sampling require self-measurement.
- PID 42484 The pre-entitled white list was omitted at the time of initial activation and installation was rejected; no two viewing was enabled, and normal. The correction allows only the known Quad entrance to be used again without easing any address.
- Local `analysis/runtime/output-quad-16996.*` (c) Preservation of direct parameters, life cycle and recovery of independent access.`output-quad-42484-install-failed.status.txt` Reason for failure to retain.
- Starts a continuous return for 10 seconds in a user-identified manner, after which only one level check is performed.

## Next

The location of the area, the back buffer size, and the right-and-right output call are all supported. Priority is given to checking the texture content and binding of the final shadow, distinguishing between the right side of the middle colour that does not generate a wrong image/script with the shadower. Evaluating uses the graphic frame capture tool to view single frames directly to reduce the need for handwritten diagnosis. Two eyes projection and OpenXR delivery have not been completed.

---

## 中文

# 左右最终输出调用和区域参数均被观测到

2026-10-12，PID 16996，原版关卡、DLSS=False、FSR2/FSR3/MFSR=False。仍未达到正确双眼像素验收。

## 直接参数证据

从已捕获的调用栈继续只读解码 `1af42c`：该函数用前八个浮点参数形成目标与纹理区域，用后四个整数参数归一化窗口／纹理尺寸，生成四个顶点，调用 DrawIndexedInstanced。`d83b17` 调用点将视图浮点区域作为目标区域、整数区域作为纹理区域。

新增可选 viewport 诊断中的该入口观察：原参数原样转发，只记录试验消费函数同线程、调用点处于 `d82f58..d83db9` 的调用。64 种固定容量直方图，溢出明确报告。新增入口由 EXE 哈希及原生指令签名约束，预加载时只预留 trampoline，不启用；停止后独立比对签名。

10 秒试验记录：

| 目标区域 | 纹理区域 | 窗口尺寸 | 纹理尺寸 | 调用次数 |
| --- | --- | --- | --- | ---: |
| 0,0,960,1080 | 0,0,960,1080 | 1920,1080 | 1920,1080 | 1,531 |
| 960,0,960,1080 | 960,0,960,1080 | 1920,1080 | 1920,1080 | 1,531 |

调用返回点均为 `d83b1c`，直方图零溢出。右视图实际进入最终区域绘制，不是仅存在处理对象。目标与源区域参数正确，也不证明 shader 的绑定纹理、常量、资源内容或最终像素正确。

本轮提交／消费／准备／销毁各 1,531，CSV 构造跨度 9.9939195 秒；生命周期分析通过、状态释放、池为空。22 个入口独立恢复，停止后正常单视图恢复。没有把这份分析报告当作立体画面验收。

## 验证与失败记录

- 新 ABI 自测通过真实 MinHook 包装带八个 float 和四个 unsigned 参数的函数，核对寄存器和栈参数原样转发、非试验调用不记录、停止后 32 字节入口恢复。
- 启动与正式采样均要求该自测通过。
- PID 42484 首次激活时遗漏预留入口白名单，安装被拒绝；没有启用双视图，关闭正常。修正只允许已知 quad 入口复用，不放宽任意地址。
- 本地 `analysis/runtime/output-quad-16996.*` 保存直接参数、生命周期和独立入口恢复。`output-quad-42484-install-failed.status.txt` 保留失败原因。
- 启动按用户确认的方式连续回车十秒，结束后只检查一次关卡状态。

## 下一步

区域位置、后备缓冲大小、左右输出调用都已有证据。优先检查最终 shader 的输入纹理内容和绑定，区分中间颜色右侧未生成与 shader 取错图／后续覆盖。评估使用图形帧捕获工具直接查看单帧资源，以减少继续添加手写诊断。双眼投影和 OpenXR 交付尚未完成。
