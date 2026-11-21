# Regional tracking versus single view

2026-10-12, original single-person level. The first person says Mod is still alive. This paper distinguishes between the life cycle of the object, actual pixels and assumptions that are still to be validated.

## Excluded area assumptions

The area is read before and after the source view, two temporary views, copies of the rendering device and preparation phase. It is not referenced after the object has been released. 41076 The original source float area is `(640,360,640,360)`, the whole number of target areas is `(0,0,1920,1080)`. After dividing the source float area, the primary renderinger changes the integer target area of two copies to `(0,0,320,360)` and `(320,0,320,360)`。

Read-only inverse confirmation `EXE+15c890` By Settings Object `+2b8` The above-mentioned floating point areas are consistent. 1920×1080 One third of the scale. The region cannot be attributed to a window failure; the game is focused during testing.

Add Optional `--submit-output-bounds`, two integer target areas 960×1080 View. PID 19424 It's... 15 The second test life cycle passes, the screenshot is too late to produce intermediate pixel evidence. 17684 It's... 60 Second retrospect submission 11,005 (b) Second, consistent consumption, preparation and destruction counts, with new state releases,11 Restored entrances;about once enabled 8 The second-collected game window still shows the narrow bars and zoom in, tailoring the subject.**The pixel problem is not solved by simply changing to the output area.**

Numeric Renderer `EXE+116250` Recalculates based on all view ranges, reset target origin and resize the buffer sizes of the entire area `+3d0..+3dc` . The use of old sampling ratios cannot be asserted by the source constructor parameters alone.

## Single View Rebuild Contrast

Add `--submit-mono-control`: Keep all structural inputs, arrays and areas in the original view unchanged, only submit a reconstruction view. Do not change eye position. It returns validated structures and release paths; the second interim view is released immediately without submission. This contrast is not VR or stereo mode.

PID 15724，60 sec:12,442 Sub-submission, consumption, preparation, destruction 12,442;pools closed and awaiting decommissioning, retention, skipping 0; new normal releases;15,638 The first failed record.11 Restores each entry. CSV construction span 59.9992039 Second, original life cycle analysis passed.

Pre- and post-enabled appointment 8 The images collected in seconds are complete and show the person, the doorway and normal view. 0.26, as a support only; manual reading of mid-view images does not find any significant magnification or tailoring in the two view mode. This results support concentrating investigations on multi-view areas, reprocessing and synthesis; no value equivalence for reconstruction of any scene is demonstrated.

The normal single view also produces a tectonic record, which is marked by the diagnostic model of the actual test's construction, consumption, preparation and destruction records. `test_state=77`. Continuous analyser reading `submit_view_count` and tags to avoid including common frames in the comparison. Eight analytical tests contain unmarked log exclusion, error mark rejection; source view and life cycle C++ self-detection pass.

## Local evidence and next steps

Local original record (not submitted Git):`analysis/runtime/rectangle-trace-41076.*`、`screen-percentage-41076.json`、`render-target-packing-17684.json`、`output-bounds-19424.*`、`output-bounds-retest-17684.*`、`mono-control-15724.*`。

Key game window picture:`analysis/game-io/frames/20261012-155045-cd8b992a.jpg`(Dual view output area re-diagnosed, abnormal),`20261012-160629-59c56595.jpg`(Unilateral view in front of)`20261012-160643-e91ae4e6.jpg`(single view contrast, normal).

## Same complete view contrast with remaining differences

PID 35160 Use `--submit-duplicate-control` Two identical matrices, a view of the same complete area and a second independent status.60 Seconds 10,520 Sub-submission, consistent consumption, preparation, destruction, status release and 11 restores each entrance;CSV span 59.9981057 seconds. After active, about 8 Second-second game window picture `20261012-162330-48c40013.jpg` The clearly enlarged columns were shown and still not checked in pixels.

Both views are in floating-point areas. `(640,360,640,360)`Integer target area `(0,0,640,360)`. Sample vectors before and after preparation are about `(1/6,-1/6,1/6,1/6)`Global buffer size 1920×1080. The same is true for the integer target area that was reconstructed by single view, so the field change alone is not sufficient to explain the difference.

This contrast excludes eye-switching and division as a condition, but**It's not possible to prove the number of multiple views alone.**: A difference remains between the single view in its original state and the two views in its second independent status. `--submit-mono-independent-state`, request to be enabled simultaneously `--submit-mono-control`, only the original area view using an independent status is submitted.

PID 46964 Independent status single view complete 60 seconds,13,115 Submittal, consistent consumption, preparation, destruction, state release,11 restores each entry; CSV tectonic span 59.997188 sec. Mid-window chart `20261012-164949-aa3ea141.jpg` Displays the normal full scene. Supported by a primary identity check and window chart: Independence is used separately for the current static scene, without causing observed zooming. The next shift to an up-frequency and multiple view display path is shown in the next section.

Add Local Evidence `analysis/runtime/duplicate-control-35160.*`、`multi-view-render-35160.json`、`multi-view-post-35160.json`、`upscale-mode-35160.json`. The source four float-point area, the target integer region and the sampling vector tracking are read only, without rewriting the unknown field.

Two complete visions, asymmetric projection, IPD/World Unit and OpenXR left-and-right eye texture submissions are still incomplete.

---

## 中文

# 区域追踪与单视图对照

2026-10-12，原版单人关卡。第一人称 Mod 保持恢复。本文区分对象生命周期、实际像素和仍待验证的推测。

## 已排除的区域假设

在源视图、两份临时视图、渲染器拥有的副本及准备阶段前后读取区域，不在对象释放后解引用。PID 41076 的原始源浮点区域为 `(640,360,640,360)`，整数目标区域为 `(0,0,1920,1080)`。分割源浮点区域后，原生渲染器将两个副本的整数目标区域改为 `(0,0,320,360)` 与 `(320,0,320,360)`。

只读反汇编确认 `EXE+15c890` 根据设置对象 `+2b8` 的比例缩放宽高，再将缩小区域居中。上述浮点区域符合 1920×1080 的三分之一缩放。不能把该区域归因于窗口失焦；测试期间游戏有焦点。

增加可选 `--submit-output-bounds`，以原整数目标区域构造两个 960×1080 视图。PID 19424 的 15 秒测试生命周期通过，截图时机过晚，未形成中途像素证据。PID 17684 的 60 秒复测提交 11,005 次，消费、准备、销毁计数一致，新增状态释放，11 个入口恢复；在启用后约 8 秒采集的游戏窗口仍显示窄条与放大、裁切的主体。**仅改用输出区域没有解决像素问题。**

原生渲染器 `EXE+116250` 会根据所有视图区域计算范围、重新设置目标原点，并结合全局渲染缓冲尺寸重新计算 `+3d0..+3dc` 的采样参数。不能仅凭源构造器参数便断言渲染时使用了陈旧采样比例。

## 单视图重建对照

新增 `--submit-mono-control`：保持原视图所有构造输入、矩阵和区域不变，仅提交一份重建视图。不改变眼位。它复用已验证的构造与释放路径；未提交的第二份临时视图立即释放。该对照不是 VR 或立体模式。

PID 15724，60 秒：12,442 次提交，消费、准备、销毁各 12,442；池关闭且待退役、保留、跳过均为 0；新增状态正常释放；15,638 次 Present 未记录失败，11 个入口恢复。CSV 构造跨度 59.9992039 秒，原生生命周期分析通过。

启用前与启用后约 8 秒采集的画面均完整显示人物、门口和正常视野。低清采图统计变化分数 0.26，仅作为辅助；人工查看中途图像没有发现双视图模式中明显的放大、裁切。此结果支持将调查集中于多视图区域、后处理和合成；不证明任意场景的重建等价性。

普通单视图也会产生构造记录，诊断模式将实际试验的构造、消费、准备、销毁记录标记为 `test_state=77`。持续分析器读取 `submit_view_count` 和标记，避免把普通帧计入对照。八项分析测试包含未标记记录排除、错误标记拒绝；源视图与生命周期 C++ 自测通过。

## 本地证据与下一步

本地原始记录（不提交 Git）：`analysis/runtime/rectangle-trace-41076.*`、`screen-percentage-41076.json`、`render-target-packing-17684.json`、`output-bounds-19424.*`、`output-bounds-retest-17684.*`、`mono-control-15724.*`。

关键游戏窗口图片：`analysis/game-io/frames/20261012-155045-cd8b992a.jpg`（双视图输出区域复测，异常），`20261012-160629-59c56595.jpg`（单视图对照前），`20261012-160643-e91ae4e6.jpg`（单视图对照中，正常）。

## 相同完整视图对照与剩余差异

PID 35160 使用 `--submit-duplicate-control` 提交两份相同矩阵、相同完整区域的视图，第二份仍使用独立状态。60 秒共 10,520 次提交，消费、准备、销毁一致，状态释放和 11 个入口恢复；CSV 跨度 59.9981057 秒。启用后约 8 秒的游戏窗口图片 `20261012-162330-48c40013.jpg` 显示明显放大的柱子，仍未通过像素检查。

两份视图的渲染副本均为浮点区域 `(640,360,640,360)`、整数目标区域 `(0,0,640,360)`。准备前后的采样向量均约为 `(1/6,-1/6,1/6,1/6)`，全局缓冲尺寸 1920×1080。单视图重建的整数目标区域也相同，因此该字段变化本身不足以解释差异。

这个对照排除了眼位偏移和分割尺寸作为必要条件，但**不能单独证明多视图数量就是原因**：单视图原状态、双视图第二份独立状态仍存在差异。新增 `--submit-mono-independent-state`，要求同时启用 `--submit-mono-control`，只提交使用独立状态的原区域视图。

PID 46964 的独立状态单视图对照完成 60 秒、13,115 次提交，消费、准备、销毁一致，状态释放，11 个入口恢复；CSV 构造跨度 59.997188 秒。中途窗口图 `20261012-164949-aa3ea141.jpg` 显示正常完整场景。由原生状态身份检查和窗口图共同支持：独立状态单独用于当前静止场景，不会造成观察到的放大。后续转向升频与多视图显示路径，见下一篇。

本地证据新增 `analysis/runtime/duplicate-control-35160.*`、`multi-view-render-35160.json`、`multi-view-post-35160.json`、`upscale-mode-35160.json`。源四浮点区域、目标整数区域及采样向量追踪均只读，不改写未知字段。

正确的两个完整眼图、非对称投影、IPD/世界单位以及 OpenXR 左右眼纹理提交仍未完成。
