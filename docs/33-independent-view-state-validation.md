# Validate the birth cycle of an independent view state

2026-10-12,Pid 11656 Single-person level,20 s. No screenshot, head start or second view is submitted. This round creates only a candidate status object that does not participate in the drawing.

## Operation and Results

Game thread calls verified factory in the main Draw assembly submission back 0x33BDD4. Return object to original view +8 , the table is the RVA expected in the current version. 0x1353918。

Then call on the normal resources to release the package. 0x4CE80C, the engine handles resource release and object cleanup. The diagnostic tool does not call directly to a contempor or free. 0x33BC70, preserve parameters, return values and original behaviour; only digital identities are recorded when the packaging returns, and object memory is not read.

Results:`attempted=1 created=1 release_returned=1 destroyed=1 original_preserved=1 flags=1`. Temporary object is the address of this process 0x95228F80, remove the call point for the package 0x33BC69, with +0x14 The sub-object returns to the normal path of the underlying object.

Delete in Thread 40012, the same thread as the group submiter/renderer. Present and renderer consumption thread is 45532. This clean-up cannot be described as necessarily occurring at the rendering line; creating/releaseing static command branches is not equal to dynamic thread conclusions.

This confirms the completion of a package with an independent state created, normal release and demarked markings on the current path. Neither is release judged by reading released memory nor does it prove that all GPU resources are complete at the time, long-term stability or status for additional eye viewing.

## Peripheral authentication

Records 90,241 Lines, drop the queue. 47 The strip. Present 6,944 Second, both successful. Renderer construction 6,941 Consumption 6,938 Infra, dissecting 6,937 Preparation 6,944 Sub-tectonic. Constructive, life cycle and preparation analysis are passed.

Original View +8 Status pointer remains unchanged. On stop `disable_status=0 active=0`;researching all of them independently 11 The byte of the entrance is restored and the game process still responds. DLL and the springboard remain in memory until the process exits.

## Tools and protection

`--shadow-state` It is a one-time mode that opens visiblely, and it retraces other modified modes. The tool only triggers the call points, single view pools from the observed main Draw; failure to create or inconsistency with the header does not repeat or force the removal of unknown objects. Status destruction observations recognize only first match calls to prevent misidentification of other objects after the address has been reused.

The starter checks the running signature of the plant, resource release package and deletion packaging on both sides of the DLL. On startup, preset the deleted jumpboard without opening; only enters the level to enable it. Add this observation point, complete assembly mode is highest. 11 A entrance.

`CaptureSelfTest.exe --state` Simulates the delay queue during an independent test process: Only once a call matching the context was created, and in another line real release test page checked for recall and parameter retention, original pointer unchanged, entry restored, and read without release. Six original self-measurement and source view copies are also measured as being passed.

DLL:`tools/native-capture/build-state-shadow/GearsCaptureStateShadow.dll`. Data:`analysis/runtime/state-shadow-11656.*`, do not enter Git. Results checker:`tools/native-capture/analyze_shadow_state.py`。

## Next Border

The results of the original matrix and visual mouth structure after the independent status of the certified copying/cleaning of the non-submission source view are now available separately. The two independent experiments are still not equivalent to stereo rendering.

---

## 中文

# 独立视图状态的原生生命周期验证

2026-10-12，原版游戏 PID 11656，单人关卡，20 秒。没有截图、头动或第二视图提交。本轮只创建一个不参与绘制的候选状态对象。

## 操作与结果

在主 Draw 的集合提交回调中，游戏线程调用已核对的工厂 0x33BDD4。返回对象与原视图 +8 的状态不同，首表为当前版本预期的 RVA 0x1353918。

随后调用正常资源释放包装 0x4CE80C，由引擎自行处理资源释放与对象清理。诊断工具没有直接调用析构或 free。只针对新对象观察删除包装 0x33BC70，保留参数、返回值与原行为；包装返回后只记录数字身份，不再读取对象内存。

结果：`attempted=1 created=1 release_returned=1 destroyed=1 original_preserved=1 flags=1`。临时对象为本进程地址 0x95228F80，删除包装的调用点 0x33BC69，与 +0x14 子对象回到基础对象的正常路径一致。

删除发生于线程 40012，与集合提交/渲染器构造线程相同。Present 和渲染器消费线程为 45532。不能把这种清理描述成必然发生在渲染线程；创建/释放的静态命令分支也不等于动态线程结论。

这确认了当前路径中一次独立状态创建、正常释放和带删除标志的包装完成。没有通过读取已释放内存来判断是否释放，也没有证明所有 GPU 资源的完成时刻、长期稳定性或状态用于额外眼视图的正确性。

## 周边验证

记录 90,241 条，队列丢弃 47 条。Present 6,944 次，均成功。渲染器构造 6,941 次、消费 6,938 次、析构 6,937 次、准备 6,944 次。构造、生命周期及准备分析均通过。

原视图 +8 状态指针保持不变。停止时 `disable_status=0 active=0`；独立只读核对全部 11 个入口字节恢复，游戏进程仍有响应。DLL 与跳板直到进程退出仍留在内存中。

## 工具和防护

`--shadow-state` 是显式开启的一次性模式，与其他修改模式互斥。工具只从已观察的主 Draw 调用点、单视图集合触发；创建失败或首表不符不会重试，也不会强行删除未知对象。状态销毁观察只认第一次匹配回调，防止地址被复用后误认其他对象。

启动器与 DLL 两侧核对工厂、资源释放包装及删除包装的运行时签名。启动时提前预留删除包装跳板但不开启；进入关卡后才启用。增加这一观察点后，完整集合模式最多 11 个入口。

`CaptureSelfTest.exe --state` 在独立测试进程中模拟延迟队列：符合上下文的调用只创建一次，在另一个线程真实释放测试页，检查回调与参数保留、原指针未改、入口恢复，以及没有释放后读取。六项原有自测与源视图副本自测同样通过。

本机诊断 DLL：`tools/native-capture/build-state-shadow/GearsCaptureStateShadow.dll`。数据：`analysis/runtime/state-shadow-11656.*`，不进入 Git。结果检查器：`tools/native-capture/analyze_shadow_state.py`。

## 下一步边界

现在分别验证了“未提交源视图复制/清理”和“未提交独立状态创建/清理”。下一步是组合两者，验证副本关联独立状态后的原生派生矩阵和视口构造；之后才能提交两个视图。两个独立实验通过仍不等于立体渲染已经完成。
