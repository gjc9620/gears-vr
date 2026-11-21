# Original call path for the final output of windows

2026-10-12，PID 37528,DLSS off, original level.10 Two-second view submission/consumption/preparation/destruction 1,640；21 The site window is still left-side and right-hand black, but has not been repaired.

## There's a call to the inn.

Only double view consumers with a line, full screen view, first RTV 1920×1080 Format 24 timecow; maximum trying 64 Number of times, reservations 8 Group, highest per group 24 frame. Records are only used for positioning, without changing the Draw parameters and without extending storage by frame.

Keep two groups and each one of them. 14 Frame, resources. `0x18cb30e0` and `0x18cb1dc0` The actual backup buffer for Present. Both games call the same stack back to RVA:

`1af92b ← d83b1c ← 2b0bdb ← 11a6f9 ← 15a943`

The starting point of the corresponding range is identified by the playtime anomaly table:`1af42c`、`d82f58`、`2b0a98`、`11908c`. The anomaly table range may be a funclet, which cannot be relied upon to identify the full semantics alone. These returns are not defined functions in Ghidra and therefore the current round uses only read-code decoded from the validated process, without writing to Ghidra or changing the game code.

`2b0a98` Read replacer by renderer `+68` View array,`+70` Number of times, step by step `D30`;every view goes through again `+C84/+C8C` . Call the false list of objects after condition matches `+8`, parameters are RCX objects, EDX mode, R 8 View, R 9 Renderer `+90`. This supports next-time observations of output objects and parameters per view, rather than full screen viewing errors.

`d82f58` Save R 8 with R 14;at the call point of the captured Draw path `d83b17`, read from this view `+48/+4C/+50/+54` Floating-point area to XML 0–3 And put `+58` Asterisk range extension, call `1af42c`. The whole screen view cannot replace the actual values of these parameters. The full number of calls, conditions and input textures for each view of this function have not been observed.

## Validation and local evidence

- Metadata binding reading tests cover unknown lists, normal binding and Reset clearances. Fixed capacity tests pass.
- Real D 3D 12 Ports forward/early stop/take over self-detection.
- `analysis/runtime/output-stack-37528.*` Save life cycle, access recovery and back-up buffer.
- `output-unwind-37528.json` Saves read-only anomaly table matches.
- `output-functions-37528.json` The first two of the two ranges are decoded.794／98 Directive, without touching the maximum number of commands; does not amount to covering all indirect calls.
- `output-stack-suffix-37528.json` is the suffix that was read earlier from the return point and cannot be considered a full function.

## Next

Record the number of times, area parameters and branch conditions for entering the left and right view separately in the location output object function, and track input texture. The current evidence is still not sufficient to determine that the middle right colour is correct or that the final synthesis is lost on the right. Keep the original parameter and do not blindly modify the final Draw or View.

---

## 中文

# 窗口最终输出的原生调用路径

2026-10-12，PID 37528，DLSS 关闭，原版关卡。10 秒双视图提交／消费／准备／销毁各 1,640；21 个入口独立核验恢复。现场窗口仍左侧场景、右侧黑色，尚未修复。

## 有界调用栈证据

只在双视图消费函数同线程、全屏 viewport、第一 RTV 为 1920×1080 格式 24 时采栈；最多尝试 64 次，保留 8 组，每组最多 24 帧。记录仅用于定位，不改 Draw 参数，不逐帧扩展存储。

这轮保留两组、各 14 帧，资源 `0x18cb30e0` 和 `0x18cb1dc0` 分别与 Present 的实际后备缓冲对应。二者相同的游戏调用栈返回 RVA：

`1af92b ← d83b1c ← 2b0bdb ← 11a6f9 ← 15a943`

通过游戏运行时异常表查出对应代码范围起点：`1af42c`、`d82f58`、`2b0a98`、`11908c`。异常表范围可能是 funclet，不能单凭它认定完整语义。Ghidra 中这些返回地址没有已定义函数，因此本轮使用已验证进程的只读代码解码，没有写入 Ghidra 或修改游戏代码。

`2b0a98` 的已读取代码按渲染器 `+68` 视图数组、`+70` 数量遍历，步幅 `D30`；每视图又遍历 `+C84/+C8C` 的对象数组。条件匹配后调用对象虚表 `+8`，参数为 RCX 对象、EDX 模式、R8 视图、R9 渲染器 `+90`。这支持下一步针对输出对象和每视图参数做观测，而不是猜测全屏 viewport 错误。

`d82f58` 保存 R8 为 R14；在已捕获 Draw 路径的调用点 `d83b17`，从该视图读取 `+48/+4C/+50/+54` 浮点区域到 XMM0–3，并把 `+58` 等整数区域传入栈参数，调用 `1af42c`。因此最终 Draw 仍涉及视图区域参数；全屏 viewport 不能代替这些参数的实际值。尚未观测该函数每个视图的完整调用次数、条件与输入纹理。

## 验证与本地证据

- 元数据 binding 读取测试覆盖未知列表、正常绑定和 Reset 清除，固定容量测试通过。
- 真实 D3D12 入口转发／早期停止／接管自测通过。
- `analysis/runtime/output-stack-37528.*` 保存生命周期、入口恢复及后备缓冲对应。
- `output-unwind-37528.json` 保存只读异常表匹配。
- `output-functions-37528.json` 保存两条范围起点的有界解码，794／98 条指令，未触及指令数上限；不等同于覆盖所有间接调用。
- `output-stack-suffix-37528.json` 是较早从返回点读取的后缀，不能当作完整函数。

## 下一步

在已定位输出对象函数处记录左右视图分别进入的次数、区域参数及分支条件，再追踪输入纹理。当前证据仍不足以确定中间右侧颜色正确，也不足以确定最终合成丢失右侧。保持原版参数，不盲目修改最终 Draw 或 viewport。
