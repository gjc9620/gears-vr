# Single frame double view into original rendering path

2026-10-12 Original single-person level, PID 46052. The first person calls Mod to stay back, without intercepting or requesting a trans-scenes.

This round gives two temporary source views to the rendering function of the normal caller, and continues in queues from the game. 41 Unlike the non-submitted object, consumption, preparation and destruction of that object are recorded in this round.

## Original Results

- `submit_pair_enabled=1 attempted=2`;width check of left and right source view `63`, the reason for rejection `0`。
- Two renderings have their own view location, matrix, status, grouping, index conformity check; the temporary source view is then released.
- `consumed=1 prepared=1 destroyed=1`, the two-view structure, consumption, preparation and destruction records of the same object in CSV exist and then a normal single view frame appears.
- The snapshot of the original assembly and source view remains unchanged. Adding a right state to release through normal Release after destruction; removing observers; thread ID 22340。
- 12 Seconds 39,523 Article,3,040 The Present, unrecorded Present failed. Game still responds and stops after 11 One entrance is restored.

Local evidence:`analysis/runtime/submit-inset-46052.*`;analysts `tools/native-capture/analyze_submit_pair.py` Pass. Four analysers test to cover missing consumption paths, unreleased status, Precent failure, error PID and refusal not to be miscalculated as successful.

## Revisions in current cycle

First PID 36252 The log does not explain the rejection phase. 38476 Show input range as `(640,360,640,360)`, is rejected as a zero prefix. The wheel release, the object is retained and the entry is restored, which cannot be called a two-view success.

The amended version is split within the original area, with a vertical origin: left `(640,360,320,360)`Right area `(960,360,320,360)`. Continue to require limited, integer, non-negative, even width and boundary areas. Native structure and holding view check passed. Why is the area smaller than desktop resolution not confirmed and cannot be directly equated with the final header size?

## Borders and Next Steps

Whether you use a synthetic positive or negative game unit offset, original projection. Only one frame is not a rice-made/IPD-marked VR eye image, and there is no physical eye pixels, long-term stability or independent historical effects.

The next step is to maintain the process of submitting and receiving the body eye. The new state must be maintained until all submitted objects are destroyed, and the release of those who cannot be released early is suspended or discontinued.

---

## 中文

# 单帧双视图进入原生渲染通路

2026-10-12，原版单人关卡，PID 46052。第一人称 Mod 保持恢复，不截图、不要求转镜头。

这一轮把两个临时源视图交给正常调用者的渲染器构造函数，由游戏继续排队。与文档 41 的未提交对象不同，本轮记录到了该对象的消费、准备和销毁。

## 原生结果

- `submit_pair_enabled=1 attempted=2`；左右源视图构造检查 `63`，拒绝原因 `0`。
- 两份渲染器自有视图的位置、矩阵、状态、所属集合、索引符合检查；临时源视图随后释放。
- `consumed=1 prepared=1 destroyed=1`，CSV 中同一对象的双视图构造、消费、准备、销毁记录均存在，之后出现正常单视图帧。
- 原集合与原源视图的快照保持不变。新增右侧状态在渲染器销毁后通过普通 Release 释放，删除观察器完成；线程 ID 22340。
- 12 秒记录 39,523 条，3,040 次 Present，无记录到的 Present 失败。游戏仍响应，停止后 11 个入口恢复。

本地证据：`analysis/runtime/submit-inset-46052.*`；分析器 `tools/native-capture/analyze_submit_pair.py` 通过。四项分析器测试覆盖缺失消费路径、未释放状态、Present 失败、错误 PID，以及拒绝不能被误判为提交成功。

## 本轮修正

初版 PID 36252 提前回退，日志不足以解释拒绝阶段。补充阶段信息后，PID 38476 显示输入区域为 `(640,360,640,360)`，被原点必须为零的前置条件拒绝。该轮状态释放、原对象保留和入口恢复通过，不能称为双视图成功。

修正版在原区域内部拆分，保留纵向原点：左区域 `(640,360,320,360)`，右区域 `(960,360,320,360)`。继续要求有限、整数、非负、偶数宽度及有界区域。原生构造与持有视图检查通过。该区域为何小于桌面分辨率尚未确认，不能把它直接等同于最终头显目标尺寸。

## 边界与下一步

左右使用合成的正负一游戏单位偏移、原投影。仅一帧，不是经过米制/IPD 标定的 VR 双眼图像，也没有肉眼验收像素、长时间稳定性或独立历史效果。

下一步是有界持续提交与肉眼验收。新增状态必须保持到所有已提交对象销毁后，停止或中止不能提前释放尚在使用的对象。通过后才接入 OpenXR 双眼投影和纹理提交。
