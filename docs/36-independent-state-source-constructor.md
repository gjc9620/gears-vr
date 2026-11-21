# Validation of combinations of independent state and complete source view configuration

2026-10-12 Original single-person level, PID 47956. This round is static, unintercepted and temporary view does not engage in rendering.

## Process

Creates a single independent state of the game before the main player pool is submitted, checking its false list and address different from the original. The full source view construct functions receive the original matrix, size, collectivity, but the third parameter changes to that independent state.

The authentication tool accepts the new state pointer in a visible way, still checking the original assembly, main matrix, four arrays of content and independent storage, as well as all fast-line bytes of the original view. It then analyzes and releases the temporary view before processing it through the general release interface of the status. If an unsafe ownership problem arises with the temporary view, the state is kept with the view to the process exit, and cannot be released earlier.

## Result

- Source View `attempted=2 stage=6 checks=255`, two non-empty arrays cross.
- Seven derivative arrays are limited, maximum difference. **0**. The new status does not affect the results of the inspected matrix that has not been submitted in this round.
- Independence status `created=1 release_returned=1 destroyed=1 original_preserved=1`, remove the sign as 1。
- Delete the line that occurred in this round of group submission 48816;Precent threads are 38236. The status cannot be said to always be deleted by the rendering thread.
- 20 Seconds 79,732 - The strip. Drop it. 36 Article;6,133 The first record failed.
- Structure, consumption/destruction, preparation stage analysis passed; when discontinued `active=0`，11 Checking back at one entrance.

Local evidence:`analysis/runtime/constructor-state-47956.*`. Combine analysis tools:`tools/native-capture/analyze_constructor_state.py`Local resource tests add the "clearly expected new state acceptable, undefined replacements should fail" test and original hooks and state life cycle self-measures are passed.

## Borders and Next Steps

This creates the unsubmitted experimental capability of "Independent status + Full view + Correct release sequence" on the current path. The view is not given to the renderinger, and therefore has not been shown as having a normal use for GPU replicating, no second eye image or long-term stability.

Next check three structures 16 Byte position-related input is associated with view matrix, and is then subject to eye deviation. The formal stereo requires a left-and-right eye projection, retrofitting target, visual, historical state and submission route together.

---

## 中文

# 独立状态与完整源视图构造的组合验证

2026-10-12，原版单人关卡，PID 47956。本轮静止、不截图，临时视图不参与渲染。

## 流程

在主玩家集合提交前的游戏线程上，一次性创建独立状态，核对其虚表和与原状态不同的地址。完整源视图构造函数接收原矩阵、原尺寸、原集合，但第三参数改为该独立状态。

验证工具显式接受新状态指针，仍核对原集合、主矩阵、四个数组的内容和独立存储，以及原视图的所有快照字节不变。随后先析构并释放临时视图，再通过状态的普通释放接口处理状态。若临时视图发生不可安全清理的所有权问题，状态与视图一同保留到进程退出，不能提前释放其状态。

## 结果

- 源视图 `attempted=2 stage=6 checks=255`，两个非空数组的对照通过。
- 七组派生矩阵都是有限值，最大差 **0**。新状态不影响本轮未提交构造的已检查矩阵结果。
- 独立状态 `created=1 release_returned=1 destroyed=1 original_preserved=1`，删除标志为 1。
- 删除发生在本轮集合提交线程 48816；Present 线程为 38236。不能据此说状态总在渲染线程删除。
- 20 秒记录 79,732 条，丢弃 36 条；6,133 次 Present 无记录失败。
- 构造、消费／销毁、准备阶段分析均通过；停止时 `active=0`，11 个入口恢复核对通过。

本地证据：`analysis/runtime/constructor-state-47956.*`。组合分析工具：`tools/native-capture/analyze_constructor_state.py`。本地资源测试增加了“明确预期的新状态可接受，未明确预期的替换应失败”检查，原有钩子和状态生命周期自测也通过。

## 边界与下一步

这建立了当前路径上“独立状态＋完整视图＋正确释放顺序”的未提交实验能力。没有把视图交给渲染器，因此还未证明独立历史能正常用于 GPU 渲染，没有第二只眼画面，也未验证长时间稳定性。

下一步核对构造的三个 16 字节位置相关输入与视图矩阵的关系，再做受限眼位偏移。正式立体需要左右眼投影、渲染目标、视口、历史状态和提交通路一起成立。
