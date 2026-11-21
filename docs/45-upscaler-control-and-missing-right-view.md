# DLSS Contrast: Zoom Out, but Black on the right

2026-10-12, original single-person level, first person called Mod, is maintained. This time advanced to visible pixel diagnosis without achieving eye display or complete VR acceptance.

## Control Conditions

Actual user settings are in `%LOCALAPPDATA%/Coalition/WarGame/Config/*EngineUserSettings.ini`. Read confirmation `DLSS=True`、`AADefaultQualityMode=3`Other FSR 2、FSR 3. The quality mode comment for the game's own configuration will be 3 Mark as Ultra Performance; source area for previous rounds 640×360 Correspond 1920×1080 A third of it.

Add `tools/debug-launch/configure_upscaler.py`: After the game has been properly quit, only the only DLS key will be temporarily changed to False. Backup original files, codes and SHA 256;no changes to other drawing keys. When you restore, verify the backup and target path, only restore it and keep other current settings. Changes are refused when running processes, multiple settings, other frequency enablers or independent settings are in place.

The current round closes the DLSS, both primary-source float and integer areas are as follows: `(0,0,1920,1080)`. Normal two-view experiments use the right and left eye microdiversions and 960×1080 range;no single view, duplicate view or output area substitute contrasts enabled.

## Original records and images

PID 46856，60 Seconds 10,461 Consumption, preparation and destruction 10,461;pool closing, pending decommissioning 0 Reservations 0 Skip 0 The independence is normally released.13,949 The first failed record.11 Restores each entry. CSV construction span 59.9988199 Second, life cycle analysis passed.

The target area for the left and right integer levels before and after preparation is the same. `(0,0,960,1080)`、`(960,0,960,1080)`. Global buffer 1920×1080, sample vector around left `(0.25,-0.5,0.5,0.25)`Right about `(0.25,-0.5,0.5,0.75)`. Right area and bias data do exist, but the existence of data does not prove that right eye pixels have been drawn or synthesized.

Enable post-enabled appointment 8 The window chart of the second displays:**Left half contains full level view, right half black**. The previous massive zooming of the DLSS model disappeared; this supports the frequency path involved in the image anomaly, but a single comparison does not confirm the specific error function. Left and right partitions compress the original width ratio, which is uncorrected for the current round, so that the left-hand characters are narrower and cannot be used as final eye maps.

The game is restored after normal closure; the file of the tool report is re-established by the original backup by bytes. The provisional diagnostic graphics are not kept as permanent settings.

The user then explicitly requested to permanently close the DLSS. The game exits again to perform the shutdown, and the original configuration backup is kept; current and subsequent debugging is used to `DLSS=False` , and this does not automatically restore unless the user requests otherwise. This does not change the conclusion that the right black screen is still unresolved.

## Local evidence

- `analysis/runtime/mono-independent-46964.*`: Independent single view contrast, normal.
- `analysis/runtime/no-upscale-pair-46856.*`: Double view count, area and entrance to close DLSS.
- `analysis/game-io/frames/20261012-165345-f0eb0f9e.jpg`: Two views midway, full left, black right.
- `analysis/upscaler-config/`: Locally set backup and recovery list, not submitted to Git.

## Next piece of evidence you need.

A distinction needs to be made between the right view sending out a drawing, writing what target and whether it is omitted or removed from the final synthesis. Currently only views consume/preparation/destruction and window output, without evidence of GPU mapping for each view. Next priority is given to tracking the videoport, scissor and drawing/synthetic calls in the left-and-right area; the right eye cannot be declared rendered based on life cycle count alone.

The deployable version still requires two complete correct eye maps, projection/IPD/world unit calibration, OpenXR left-and-right eye texture access and agreed head-hand interaction. DLSS compatibility should be independently verified; the diagnosis of closing down DLSS cannot replace this compatibility conclusion.

---

## 中文

# DLSS 对照：放大消失，但右侧仍黑

2026-10-12，原版单人关卡，第一人称 Mod 保持恢复。本次推进到可见像素诊断，没有达到双眼显示或完整 VR 验收。

## 对照条件

实际用户设置位于 `%LOCALAPPDATA%/Coalition/WarGame/Config/*EngineUserSettings.ini`。读取确认 `DLSS=True`、`AADefaultQualityMode=3`，其他 FSR2、FSR3、MFSR 开关为 False。游戏自带配置的质量模式注释将 3 标记为 Ultra Performance；前几轮的源区域 640×360 对应 1920×1080 的三分之一。

新增 `tools/debug-launch/configure_upscaler.py`：游戏正常退出后，仅把唯一的 DLSS 键临时改为 False。备份原文件、编码和 SHA256；不修改其他画质键。恢复时验证备份与目标路径，只恢复该键并保留其他当前设置。存在运行中进程、多个设置文件、其他升频器启用或独立设置变化时拒绝修改。

本轮关闭 DLSS 后，原生源浮点和整数区域都为 `(0,0,1920,1080)`。正常双视图实验分别使用左右眼微偏移和 960×1080 区域；没有启用单视图、重复视图或输出区域替代对照。

## 原生记录与画面

PID 46856，60 秒提交 10,461 次，消费、准备、销毁各 10,461；池关闭、待退役 0、保留 0、跳过 0。独立状态正常释放，13,949 次 Present 未记录失败，11 个入口恢复。CSV 构造跨度 59.9988199 秒，生命周期分析通过。

准备前后的左、右整数目标区域分别为 `(0,0,960,1080)`、`(960,0,960,1080)`。全局缓冲 1920×1080，采样向量左约 `(0.25,-0.5,0.5,0.25)`、右约 `(0.25,-0.5,0.5,0.75)`。右侧区域及偏置数据确实存在，但数据存在不能证明右眼像素已经绘制或合成。

启用后约 8 秒的窗口图显示：**左半边包含完整关卡视野，右半边黑色**。此前 DLSS 模式的巨大放大消失；这支持升频路径参与了画面异常，但单次对照尚不能确认具体错误函数。左右分区压缩了原宽高比，本轮未校准投影，因此左侧人物显得较窄也不能作为最终眼图。

游戏正常关闭后已恢复 DLSS；工具报告恢复后的文件与原备份逐字节一致。没有把临时诊断画质留作永久设置。

随后用户明确要求永久关闭 DLSS。游戏退出状态下再次执行关闭，原配置备份继续保留；当前及后续调试以 `DLSS=False` 为准，不自动恢复，除非用户另行要求。这不改变右侧黑屏尚未解决的结论。

## 本地证据

- `analysis/runtime/mono-independent-46964.*`：独立状态单视图对照，正常。
- `analysis/runtime/no-upscale-pair-46856.*`：关闭 DLSS 的双视图计数、区域和入口恢复。
- `analysis/game-io/frames/20261012-165345-f0eb0f9e.jpg`：双视图中途，左侧完整、右侧黑。
- `analysis/upscaler-config/`：本地设置备份与恢复清单，不提交 Git。

## 下一项需要的证据

需要区分右侧视图是否发出绘制、写入什么目标，以及是否在最终合成中被遗漏或清除。目前只有视图消费／准备／销毁与窗口输出，缺少每个视图的 GPU 绘制证据。下一步优先追踪左右区域的 viewport、scissor 和绘制／合成调用；不能仅根据生命周期计数宣布右眼已经渲染。

可部署版本仍需两个正确完整眼图、投影/IPD/世界单位校准、OpenXR 左右眼纹理接入，及约定的头手交互。DLSS 兼容性应单独验收；关闭 DLSS 的诊断不能替代该兼容性结论。
