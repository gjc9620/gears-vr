# Original authentication without submitting copy of source view

2026-10-12,Pid 17824 Single-person level,20 sisecond sampling. No screenshot, no head moves enabled, no second view added or original collection modified.

## Result

& Inline for regular Draw 0x3a148c Call Point 0x2c8909 , the game thread is executed only once:

1. 0x1c03c8 Allocation 0x430 Bytes, Alignment 8。
2. 0x194454 Constructs a separate copy from the current source view.
3. Checks whether the matrix, the associated pointer, the four known arrays and the original view and assembly remain unchanged.
4. 0x15ca7c Photo by Analyze.0x53ea50 Release its storage.

Original result is `attempted=2 stage=6 checks=255 nonempty_arrays=2`: eight checks were all passed. Two non-empty arrays have the same content and different storage addresses; matrix by bytes, associated pointer maintained, original view all 0x430 Byte and original assembly header, first view pointer, remains unchanged. Copies are not entered into the rendering pool.

This confirms the feasibility of copying and pairing a source view at the current level. It cannot be claimed that any level, all container branches, abnormally spread paths or long-term resource stability have been verified.

## Sample and Recovery

Records 89,808 Lines, drop the queue. 47 The strip. Present 6,910 Second, both successful. Renderer construction 6,907 Consumption, second 6,906 Sub-sync 6,907 Number of times;association 6,897 The whole life cycle. Check through. Preparation phase. 6,910 Number of times, change is still limited to matrix 24、matrix 25。

Stop result `disable_status=0 active=0`Read-only check all of them independently 10 The game process is still responding. The lack of focus during the sampling period is a static baseline, without user rotation or current round experience conclusion. DLL and springboard remain in the process until normal exit.

Local evidence:`analysis/runtime/source-shadow-17824.*`. Original log does not enter Git.

## Protection and self-measurement

New mode must be visible `--shadow-view`, the starter and DLL check the running of the game version and four additional entrances on both sides. Tests only work with observed main Draw call points, single view collections and perform once in the game line.

Non-empty +0x140 Chain packagings temporarily rejected; known array +0x380、+0x390 Every element 16 bytes,+0x3EC、+0x400 Every element 8 bytes. Single array limit 256 entry. If a copy is found to be accidentally shared with its own storage, the normal clean-up is stopped and this allocation is maintained so as not to misdetermine the original view resource; the result is judged unsuccessful until the process exits without retrying. This round does not enter the branch.

`SourceViewShadowTest.exe` The existing foundations, collections, rendering copies, replicators, life cycle and preparation stages are also six self-measured. These local simulation tests do not replace real engine validation.

## Next

Copy Save Source View +8 . The question of whether each eye can share it, and cover one another's historical matrix is still unresolved. It also requires confirmation of multiple view paths for creating/release, visualizing and output targets in an independent state to submit additional eyesights.

Code:`tools/native-capture/source_view_shadow.h`、`source_shadow_test.cpp`、`analyze_shadow_view.py`. This machine diagnoses DLL:`tools/native-capture/build-source-copy/GearsCaptureSourceCopy.dll`It's still not a fully deployable VR Mod.

---

## 中文

# 未提交源视图副本的原生验证

2026-10-12，原版游戏 PID 17824，单人关卡，20 秒采样。没有截图、没有启用头动，没有追加第二视图或修改原集合。

## 结果

在普通 Draw 的集合提交入口 0x3a148c、调用点 0x2c8909 中，游戏线程只执行一次：

1. 0x1c03c8 分配 0x430 字节、对齐 8。
2. 0x194454 用当前源视图构造独立副本。
3. 检查矩阵、关联指针、四个已知数组，以及原视图和集合是否保持不变。
4. 0x15ca7c 析构副本，0x53ea50 释放其存储。

原生结果为 `attempted=2 stage=6 checks=255 nonempty_arrays=2`：八项检查全部通过。两个非空数组的内容一致、存储地址不同；矩阵逐字节一致，关联指针保留，原视图全部 0x430 字节及原集合头、首视图指针保持不变。副本未进入渲染集合。

这确认了当前关卡中一次源视图复制与配对释放的可行性。不能据此声称任意关卡、所有容器分支、异常展开路径或长期资源稳定性均已验证。

## 采样与恢复

记录 89,808 条，队列丢弃 47 条。Present 6,910 次，均成功。渲染器构造 6,907 次，消费 6,906 次，析构 6,907 次；关联出 6,897 个完整生命周期，检查通过。准备阶段 6,910 次，前后变化仍限于 matrix24、matrix25。

停止结果 `disable_status=0 active=0`，独立只读核对全部 10 个入口字节恢复。游戏进程仍有响应。采样期间失焦，属于静态基线，未要求用户转镜头，也没有本轮画面体验结论。DLL 和跳板仍留在进程中，直到正常退出。

本地证据：`analysis/runtime/source-shadow-17824.*`。原始日志不进入 Git。

## 防护与自测

新模式必须显式传入 `--shadow-view`，默认不开启。启动器与 DLL 两侧都核对游戏版本及四个额外入口的运行时签名。测试仅作用于已观察的主 Draw 调用点、单视图集合，并在游戏线程执行一次。

非空 +0x140 链式容器暂时拒绝；已知数组 +0x380、+0x390 每元素 16 字节，+0x3EC、+0x400 每元素 8 字节。单数组限制 256 项。若发现副本意外共享自有存储，停止正常清理并保留这一个分配，避免析构误释放原视图资源；结果判为失败，直到进程退出不再重试。本轮未进入该分支。

`SourceViewShadowTest.exe` 覆盖非空数组独立复制、资源配对、矩阵比较失败后的清理、共享存储拒绝清理、分配失败及输入边界。现有基础、集合、渲染副本、渲染器、生命周期和准备阶段六项自测同样通过。这些本地模拟测试不替代真实引擎验证。

## 下一步

复制保留源视图 +8 的关联状态指针。每只眼能否共享它、是否会互相覆盖历史矩阵，尚未解决。还需确认独立状态的创建/释放、视口与输出目标的多视图路径，才能提交额外眼视图。

代码：`tools/native-capture/source_view_shadow.h`、`source_shadow_test.cpp`、`analyze_shadow_view.py`。本机诊断 DLL：`tools/native-capture/build-source-copy/GearsCaptureSourceCopy.dll`。它仍不是可部署的完整 VR Mod。
