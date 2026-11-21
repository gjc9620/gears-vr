# Correlation of left- and-right intermediate targets to window back buffers

2026-10-12. DLSS remains closed. Two views still do not reach the correct eye pixel acceptance.

## Add observation

Enable three Ds before creating level target 3D 12 Descriptor view entrance: CreateRender TargetView, CopyDescriptions, CopyDescriptionsSimple. Original parameters are forwarded as usual, description metadata with fixed capacity are kept without resource references. 180 The entry is restored over time; official sampling can take over the access.

The official sample tracks the first colour target of OMSetRenderTargets and its dimensions, format, mip. Only the first RTV is covered, with no coverage of all MRT, depth objective, ExecuteIndirect or GPU content. The command list slots are bound to histogram capacity, spilling clear reports and not being considered a complete GPU audit.

## Effective field testing

PID 5296: Loading of the record 363 Creates a RTV, describing sign copy and evict zero.10 Double-Sec View Submit 1,576 Consumption, preparation and destruction 1,576; state of release, empty pool,21 The window is still left-relevant cards, right-hand black and then returns to normal single view.

| First Colour Resource | Size/Format Number | Left View Draw | Right View Draw |
| --- | --- | ---: | ---: |
| `0x161d9c6650` | 1920×1080／28 | 5,807 | 5,817 |
| `0x3db1ae1b0`When you tie four targets | 1920×1080／26 | 41,851 | 40,866 |
| The same resources. When you set a target, | 1920×1080／26 | 59,387 | 61,407 |

The main colours captured above are bound to the full range of targets, and do not support the assumption that "these goals have a half-screen width so they are cut off on the right."0 Records indicate that no colour-colored target is bound and may contain depth drawings, which cannot be evidence of an error in size.

Yeah. 147,132 CSV records read in a strip-to-stream fashion, will make Present/Prosent 1 The GetBuffer resource identity corresponds to the RTV histogram:

| Actual window back buffer | Size/Format Number | Present Records | Draw within the Test Range |
| --- | --- | ---: | ---: |
| `0xb066c000` | 1920×1080／24 | 2,130 | 992 Fullscreen view |
| `0xb06656d0` | 1920×1080／24 | 2,130 | 994 Fullscreen view |

Both belong to the same game window `0x951bd8`. So you can determine these formats 24 The target is the window's actual back-up buffer, not just a formal guess. Identity is used only for this capture and cannot be reused over the process. Full screen output may be normal composite; it has not been proven to be the cause of the right black screen or correct pixels in the middle target.

Command List Overload 653,020 No, no direct drawing followed 2,674,405 - It's a lot of time.1 Still refers only to the test consumer function's wiring range, and other route paths may not be covered.

## Invalid or prepare rotations

- PID 32036 In focus, double view is submitted zero times; target creation observation starts too late, CreateRTV is zero and cannot be used as evidence for the dual view.
- PID 41192 The early status file is unique and not ready for reading; no two-views are enabled. Three earlier access points are restored by stop signal and independent verification, and subsequent changes to the file sharing method are made.

## Validation and evidence

- Fixed-capacity metadata testing covers copying, unknown source removal, cover, Reset, volume expulsion and over-repeated rejection.
- Real D 3D 12 Offline self-detected RTV creation/two reproductions, early observation stop, formal takeover and portal recovery, without actual mapping.
- Eight analytical tests were submitted for approval.
- Local original evidence:`analysis/runtime/target-prime-5296.*`;reciprocal reports `target-prime-5296-backbuffers.json`. Original evidence ignored by Git.
- Fluid matching tools:`tools/native-capture/correlate_backbuffer_targets.py`, the required stop state is confirmed and no count is accepted or accepted as pixels.

## Next

The collection output reaches a small number of call anchors that are identified as backup buffers, locate the original final output path and check its input texture and regional parameters. It cannot be forced to cut or output ranges simply because of full-screen viewing. Correct binary pixels, independent OpenXR eye texture, projection/unit/IPD calibration and interaction remain to be completed.

---

## 中文

# 左右中间目标与窗口后备缓冲的对应关系

2026-10-12，继续排查原版关卡中的右侧黑屏。DLSS 保持关闭。双视图仍未达到正确双眼像素验收。

## 新增观察

在关卡目标创建前，先启用三个 D3D12 描述符观察入口：CreateRenderTargetView、CopyDescriptors、CopyDescriptorsSimple。原参数照常转发，保存固定容量的描述符元数据，不持有资源引用。观察最多 180 秒，超时恢复入口；正式采样可接管这些入口。早期状态文件允许读取，便于独立确认接管前状态。

正式采样再跟踪 OMSetRenderTargets 的第一颜色目标及其尺寸、格式、mip。仅覆盖第一 RTV，不覆盖所有 MRT、深度目标、ExecuteIndirect 或 GPU 内容。命令列表槽位与直方图容量有界，溢出明确报告，不能当作完整 GPU 审计。

## 有效现场试验

PID 5296：加载期记录 363 个 RTV 创建，描述符复制及驱逐均为零。10 秒双视图提交 1,576 次，消费、准备、销毁各 1,576；状态释放、池清空，21 个入口独立核验恢复。试验期间窗口仍左侧有关卡、右侧黑色，停止后恢复正常单视图。

| 第一颜色资源 | 尺寸／格式编号 | 左 viewport 绘制 | 右 viewport 绘制 |
| --- | --- | ---: | ---: |
| `0x161d9c6650` | 1920×1080／28 | 5,807 | 5,817 |
| `0x3db1ae1b0`，绑定四目标时 | 1920×1080／26 | 41,851 | 40,866 |
| 同一资源，绑定一目标时 | 1920×1080／26 | 59,387 | 61,407 |

上述已捕获的左右主要颜色绘制都绑定全宽目标，不支持“这些目标只有半屏宽，所以右边被截掉”的假设。大量 targets=0 记录表示无颜色目标绑定，可能含深度绘制，不能作为错误尺寸证据。

对 147,132 条 CSV 记录逐条流式读取，将 Present/Present1 的 GetBuffer 资源身份与 RTV 直方图对应：

| 实际窗口后备缓冲 | 尺寸／格式编号 | Present 记录 | 试验范围内绘制 |
| --- | --- | ---: | ---: |
| `0xb066c000` | 1920×1080／24 | 2,130 | 992，全屏 viewport |
| `0xb06656d0` | 1920×1080／24 | 2,130 | 994，全屏 viewport |

两者属于同一游戏窗口 `0x951bd8`。因此可以确定这些格式 24 目标就是窗口实际后备缓冲，而不是仅凭格式猜测。身份仅用于这次捕获，不可跨进程复用。全屏输出可能是正常合成行为；没有证明它是右侧黑屏原因，也没有证明中间目标中的右侧像素正确。

命令列表容量溢出 653,020 次，未跟踪直接绘制 2,674,405 次。phase=1 仍仅指试验消费函数同线程范围，其他线程路径可能未覆盖。

## 无效或准备轮次

- PID 32036 失焦，双视图提交零次；目标创建观察开始太晚，CreateRTV 为零，不能作为双视图目标证据。
- PID 41192 早期状态文件独占，准备核验无法读取；没有启用双视图。三个早期入口经停止信号及独立核验恢复，随后修正文件共享方式。

## 验证与证据

- 固定容量元数据测试覆盖复制、未知源清除、覆盖、Reset、容量驱逐及过大复制拒绝。
- 真实 D3D12 离线自测覆盖 RTV 创建／两种复制、早期观察停止、正式接管与入口恢复，不执行实际绘制。
- 既有八项提交分析测试通过。
- 本地原始证据：`analysis/runtime/target-prime-5296.*`；对应报告 `target-prime-5296-backbuffers.json`。原始证据由 Git 忽略。
- 流式对应工具：`tools/native-capture/correlate_backbuffer_targets.py`，要求停止状态已确认，不将计数作为像素验收。

## 下一步

采集输出到已确认后备缓冲时的少量调用栈，定位原生最终输出路径，再检查其输入纹理与区域参数。不能仅因全屏 viewport 就强改裁剪或输出范围。正确双眼像素、独立 OpenXR 眼纹理、投影／单位／IPD 校准及交互仍待完成。
