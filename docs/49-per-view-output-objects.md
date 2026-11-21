# Both left and right view have final handlers

2026-10-12, keep checking the right black screen and DLSS is closed. This round of additional observations follows the existing Draw entrance without adding a primary function to it.

## Read Range

On full screen,1920×1080, format 24 The first RTV candidate Draw keeps up to four rendershots, with a maximum of two views per view 16 A processor object. Recording is from a living object that remains in the consumption function; no reference rendering was removed after the original consumption returned. Read array headers, parameters and false tables without modifying them or holding references.

## Evidence and limitations

PID 12800 Four snapshots cover two different renderingers, each view of the array count =1、capacity=17. Use separate objects, false statements `0x141675cf0`,Fake Table+8 All point to `0x140d82f58`,flags are all 12。

Left view floats and integer areas `0,0,960,1080`;right view both `960,0,960,1080`. Therefore, neither the Right View Finally Processed Object nor the Right Area Turns to Left Area are valid on these snapshots. The existence of an object does not prove that the view actually completes the final output or that the texture content is correctly entered.

Backstage test plan for the current round 10 sec, actual submission 1,439 Yes, then the source input range check refused, rejection=1、attempted=3,CSV Construct Range 9.282 sec. Consumption/preparation/destruction 1,439 The pool is empty, it's released.21 The entrances were independently restored.**The full round of continuous submission of analysis was not passed and is incomplete 10 Second acceptance.** Four early valid snapshots can be used for the limited judgement described above; the reasons for refusal thereafter still require a precise distinction between input-inner/state/array conditions and cannot be removed from inspection to continue.

PID 34032 The previous Focus version did not submit a double view and was not valid data.

## Backstage Options

`--submit-background` Allows a two-view diagnosis of the view for up to 10 seconds when explicitly enabled; F 8, the time limit and object check are still valid. CLI and DLL both limit grouping and duration and default remains out of focus.11 Second combination confirmed rejected by CLI; current round 0, does submit in the backstage and restore a single view after stopping. This option does not remove the focus limit of head and lens swing mode.

## Local evidence and next steps

Original data is located in `analysis/runtime/output-post-background-12800.*`, the parsed=false is clearly saved in the analysis file. Next on the location function `d82f58` Reads the actual call and input parameters for each view, then looks at the color texture. The correct right eye map is still not available, rather than being deployable VR Mod.

---

## 中文

# 左右视图均有最终处理对象

2026-10-12，继续排查右侧黑屏，DLSS 保持关闭。这轮新增观察沿用现有 Draw 入口，不增加原生函数挂钩。

## 读取范围

在全屏、1920×1080、格式 24 第一 RTV 的候选输出 Draw 内，最多保存四次渲染器快照，每次两视图、每视图最多 16 个处理对象。记录来自仍处于消费函数内的活对象；没有在原消费返回后解引用渲染器。读取数组头、参数和虚表，不修改它们，也不持有引用。

## 实测证据及限制

PID 12800 的四次快照涵盖两个不同渲染器，每个视图的数组 count=1、capacity=17。左右使用各自独立的对象，虚表同为 `0x141675cf0`，虚表 +8 都指向 `0x140d82f58`，flags 都为 12。

左视图浮点和整数区域均为 `0,0,960,1080`；右视图均为 `960,0,960,1080`。因此在这些快照上，“右视图没有最终处理对象”及“右侧区域变成左侧区域”都不成立。对象存在并不证明该视图实际完成最终输出，也不证明输入纹理内容正确。

本轮后台试验计划 10 秒，实际提交 1,439 对，随后源输入范围检查拒绝，rejection=1、attempted=3，CSV 构造跨度 9.282 秒。消费／准备／销毁各 1,439，池清空、状态释放，21 个入口独立恢复。**整轮持续提交分析未通过，不作为完整 10 秒验收。** 四次早期有效快照可用于上述有限判断；之后拒绝原因仍需精确区分输入 owner/state/数组条件，不能移除检查强行继续。

PID 34032 的前一次焦点版试验没有提交双视图，不作为有效数据。

## 后台选项

`--submit-background` 明确启用时，允许最多十秒的 viewport 双视图诊断在后台运行；F8、时间上限和对象检查仍有效。CLI 与 DLL 均限制组合和时长，默认仍保持失焦停止。11 秒组合已确认被 CLI 拒绝；本轮 focused_pair_constructors=0，确实在后台提交，停止后恢复单视图。该选项不解除头动和镜头摆动模式的焦点限制。

## 本地证据与下一步

原始数据位于 `analysis/runtime/output-post-background-12800.*`，分析文件明确保存 passed=false。下一步针对已定位函数 `d82f58` 读取每个视图的实际调用和输入参数，再查输入颜色纹理。仍未得到正确右眼图，不称为可部署 VR Mod。
