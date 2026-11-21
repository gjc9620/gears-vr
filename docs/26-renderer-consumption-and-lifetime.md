# Rendering thread consumption and view life cycle

2026-10-12. Restore single person level after original package, PID 40592，60 sec. No screenshot, no viewing points. This validation object is given the path from the game thread to the rendering thread, without adding eyes.

## Paths and Principles

Line Call for Game 0x3a148c Present the collection,0x116250 Constructs the rendering object and copys the view. The submitting creates a subsequent 16 Byte Command: Candidate Address RVA 0x136c4b8，+8 Save Rendering Object, give it to 0x1c08a4 Queue to submit the assistant.

Candidate +8 Point 0x15a7f4;this function starts with command +8 Remove Object, Call 0x15a80c. All consumption calls back to the current round 0x15a801, matches the queue path. Other points around a table cannot be directly identified as the same type of method.

0x15a80c Call the default path by object sign 0x11908c Or another candidate path. 0xe29520, and then call 0x15a974 Clears the object. Default path inverted to read objects +0x68 / +0x70 , and press 0xD30 Walks through the clock. There is no dynamic division between two drawing branches.

0x15a974 Call 0x15aab8, and then released 0x140 bytes objects. The latter are counted in arrays and 0xD30 Step Call 0x15bda4 View parsing, and cleaning arrays. This evidence indicates that these views have a tectonic/constructive syntax and cannot add the second view to the naked memcpy at will.

Both consumption and release Hook will only read the object before the original call, and only local records will be processed after return; the original call may have been released.`--lifetime` Auto-detect the page where the actual VirtualFree object is located, authenticate the forwarding, return values and access to avoid relying on post-release reading.

## Dynamic results

| Item | Result |
| --- | --- |
| Complete Construct | 21,010 Line, Thread 42120 |
| Consumption | 21,002 Line, Thread 25740 |
| Release | 21,010 Line, Thread 25740 |
| Present | 21,011 Line, Thread 25740 Failure 0 |
| Construct to Consuming Association | 20,995 Yes; end to median consumption interval 0.0798 ms |
| Complete construction, consumption, release correlation | 20,989 Group |
| Release Call Back Address | All 0x15a950 |
| Drop Record | 93 Article |
| Stop / Restor | active=0 All nine recovered. |

Recorded consumption and pre-release state is satisfied: clustering in object +8; one view; view points to the continuous storage of objects; owner points to the collection; matrix area is limited. Full construction analysis, life-cycle analysis are passed. No matching records are completed and queues are not no-lose tracking.

The associated tectonic results are consistent with the pre-consumer matrix region; neither before consumption nor before release is fully consistent. 32 of elements 24、25, i. e. view +0xD0、+0xD4;in the three full paragraphs are linked 20,698、20,989 The meaning of the field and its writing position are not confirmed. They cannot be considered as anti-saw-dent vibrating by a value alone.

User feedback completes the action, normal and no anomalies, but current consumption records are all for 0, the matrix and scene rotation are only in one set. So this is a life cycle baseline under focalless state, not covered as trans-scenes. The original motion evidence was available in the document. 25, cannot be impersonated as a campaign validation for the new consumption entry into this round.

## Tools and boundaries

Current Build `tools/native-capture/build-lifetime/GearsCaptureLifetime.dll`, the starter defaults to use this construction. Early preset seven primary jumpers before entering the level and observing two DXGI portals. Version verification has released access due to missing pages 0x40 Command prefix denied access; correction after checking real-time bytes, failure wheel not activated Hook.

Add `inspect_vtable.py` Read only the candidate lists within the Hashi certification process;`analyze_lifetime.py` Reduces error in address correction based on QPC, object, embedded inter-space and thread correlation records and reset object association at new construction/release.

Evidence saved in ignore directory:`analysis/runtime/renderer-lifetime-motion-40592.*`(Previously planned for retention of document name, actual baseline), corresponding `-restored.json`、`-construction.json`、`-lifecycle.json`、`-matrix-diff.json`. Read only invert compilation `analysis/native/render-command-*-46140.json`、`render-draw-destruct-40592.json`、`render-view-prepare-destruct-40592.json`。

This round confirms the line-to-line and release sequence of the actual observation path; it does not prove that GPU has been completed, all games branches, multiple view security or stereo rendering. Next steps are to update the positioning matrix and its derivative data, re-engineer a revocable left-eye differential test.

---

## 中文

# 渲染线程消费与视图生命周期

2026-10-12。恢复原版包后的单人关卡，PID 40592，60 秒。不截图、不改写视点。本次验证对象从游戏线程交给渲染线程的路径，尚未新增双眼。

## 路径与原理

游戏线程调用 0x3a148c 提交集合，0x116250 构造渲染对象并复制视图。提交方随后建立一个 16 字节命令：候选表地址 RVA 0x136c4b8，+8 保存渲染对象，交给 0x1c08a4 队列提交助手。

候选表 +8 指向 0x15a7f4；此函数从命令 +8 取出对象，调用 0x15a80c。本轮所有消费调用的返回地址为 0x15a801，符合此排队路径。表附近的其他指针不能直接认作同一类型的方法。

0x15a80c 按对象标志调用默认路径 0x11908c 或另一候选路径 0xe29520，随后调用 0x15a974 清理对象。默认路径反汇编读取对象 +0x68 / +0x70 的连续视图数组，并按 0xD30 步长遍历。未动态区分两条绘制分支。

0x15a974 调用 0x15aab8，然后释放 0x140 字节对象。后者按数组计数和 0xD30 步长调用 0x15bda4 视图析构，再清理数组。此证据说明这些视图具有构造/析构语义，不能用裸 memcpy 随意增加第二个视图。

消费和释放 Hook 均只在原调用之前读取对象，返回后仅处理本地记录；原调用可能已经释放对象。`--lifetime` 自测让原函数实际 VirtualFree 对象所在页面，验证转发、返回值和入口恢复，避免依赖释放后读取。

## 动态结果

| 项目 | 结果 |
| --- | --- |
| 完整构造 | 21,010 条，线程 42120 |
| 消费 | 21,002 条，线程 25740 |
| 释放 | 21,010 条，线程 25740 |
| Present | 21,011 条，线程 25740，失败 0 |
| 构造到消费关联 | 20,995 对；结束到消费中位间隔 0.0798 ms |
| 完整构造、消费、释放关联 | 20,989 组 |
| 释放调用返回地址 | 全部 0x15a950 |
| 丢弃记录 | 93 条 |
| 停止 / 入口恢复 | active=0，九处全部恢复 |

记录的消费和释放前状态均满足：集合位于对象 +8；一个视图；视图指向对象自有连续存储；owner 指向该集合；矩阵区域有限。完整构造分析、生命周期分析均通过。未匹配记录不补齐，记录队列不是无损追踪。

所有关联的构造结果与消费前矩阵区域一致；消费前与释放前则均不完全一致。变化仅在 32 元素中的 24、25，即视图 +0xD0、+0xD4；完整三段关联中分别有 20,698、20,989 次改变。具体字段意义和写入位置尚未确认，不能仅凭数值认定是抗锯齿抖动。

用户反馈完成动作、正常无异常，但本轮消费记录 focused 全为 0，构造矩阵及场景旋转均只有一组。因此这是失焦状态下的生命周期基线，不作为转镜头覆盖。此前原版运动证据见文档 25，不能冒充本轮新增消费入口的运动验证。

## 工具与边界

当前构建 `tools/native-capture/build-lifetime/GearsCaptureLifetime.dll`，启动器默认使用此构建。早期预留七处原生跳板，进入关卡后才激活，另观察两个 DXGI 入口。版本核验曾因遗漏释放入口的 0x40 指令前缀拒绝接入；核对实时字节后修正，失败轮没有激活 Hook。

新增 `inspect_vtable.py` 只读取哈希验证进程内的有界候选表；`analyze_lifetime.py` 根据 QPC、对象、嵌套区间和线程关联记录，并在新构造/释放时重置对象关联，降低地址复用误判。

证据保存在忽略目录：`analysis/runtime/renderer-lifetime-motion-40592.*`（文件名保留原计划，实际是基线）、对应 `-restored.json`、`-construction.json`、`-lifecycle.json`、`-matrix-diff.json`。只读反汇编在 `analysis/native/render-command-*-46140.json`、`render-draw-destruct-40592.json`、`render-view-prepare-destruct-40592.json`。

本轮确认实际观察路径的线程交接和释放顺序；不证明 GPU 已执行完毕、所有游戏分支、多视图安全或立体渲染。下一步定位矩阵更新及其派生数据，再设计可撤销的左右眼差异测试。
