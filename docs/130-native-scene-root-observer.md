# Opt-in native UI root observation

2026-10-17. This integrates the mixed batch inventory with the saved native root adapter. It does not enable PathChoice placement, semantic publication or rendering changes.

`GEARS_UI_SCENE_ROOT_OBSERVER` defaults OFF. An experimental build can reserve the adapter at RVA `296178` only when its exact 13 bytes match. Runtime selection requires the exact client call return `2c3dce`, caller instruction bytes, client table/slot and scene table/slot, with graph rereads. These are structural candidates only: registry identities, ActiveScenes membership and local-player relationships remain unproven. The patched adapter entry is deliberately not compared to original bytes during callbacks.

Source evidence was queried through the existing KB and reviewed in the saved `1402c3cf0.chunks.json` and `root-adapter-window.json`. Caller `2c3dc8` is `ff90b8020000` (six bytes), returning to `2c3dce`. The historical code snapshot SHA256 is `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, module base `140000000`; source/provenance boundaries remain as documented in docs 122 and 124. No new IDA database was opened.

For at most eight matching invocations per session, the hook captures a bounded before inventory and after delta, plus the top 80-byte matrix descriptor and stack header. It records simple/material Execute callbacks occurring synchronously on that same thread. Nesting restores the previous TLS observation pointer. The native original runs once and its returned RAX bits are forwarded; return semantics are not inferred.

Counts are printed only after the existing capture lifecycle drains active readers. They do not prove payload ownership, whole-scene completeness, absence of an internal/asynchronous flush, GPU execution or pixel visibility. A restored matrix report means identical saved stack header and top descriptor; a changed allocation header can make it false without establishing a rendering error. Unknown types, larger inventories or changed prefixes reject collection; they must not authorize partial placement.

## Offline verification

- `UiSceneRootCandidateTest`: saved instruction/table drift, wrong caller, read failures, overflow, graph reread races and matrix-stack/top changes; output remains unchanged on rejection.
- `UiSceneBatchTest`, `UiGroupPlacementTest`, `UiHudLayoutTest`: all passed in the same frozen experimental build.
- Full DLL compiled with the new observer ON and the prior VRDof61 option set. The ABI/self-test loader returned 0 against this DLL.
- Experimental DLL: `analysis/native-build-ui-scene-root1/GearsCaptureUiSceneRoot1.dll`, SHA256 `8acf295a4cc7136bf4e1109edaf2f5969cbfa434abc34f19d44bb82614775986`.

Frozen source overlay hashes and build logs are under ignored `analysis/runtime/ui-scene-root1-*`; existing local MinHook sources were copied with recorded hashes, without installing tools. The first configure attempt lacked ignored vendor sources and was corrected. An initial self-test invocation lacked its DLL argument and returned usage code 2; the corrected invocation passed. No game, simulator, GUI or XR Runtime action was performed, and no local default manifest or existing test package was replaced.

## Remaining before UI movement

Use a coordinated actual run to establish that this route covers the reported scene, obtain its layout/coordinate contract and inspect collection/flush behavior. Then add fresh scene/client/local-player admission and carry one immutable group identity through both simple indexed and material consumers before applying the shared placement recipe once. This observer supplies the missing runtime sampling point; static table matches alone cannot authorize that transformation.

---

## 中文

# 本地 UI 根观测

2026-10-17。此选项将混合批量目录与保存的本地根适配器整合。它无法启用 PathChoice 放置、语义发布或渲染更改。

`GEARS_UI_SCENE_ROOT_OBSERVER` 默认关闭。实验性建筑可以将适配器保留在 RVA 上 `296178` 只有在准确的时候 13 字节匹配。 运行时选择需要准确的客户端调用返回 `2c3dce`,调用指令字节、客户端表/slot和场景表/slot,并附有图表重读。这些只是结构候选人:注册身份、活动成员以及本地玩家关系仍未证实。补丁的适配器条目是故意的。 与调用时的原字节相比,没有 。

通过现有的KB查询了原始证据,并在保存的表格中进行了审查。 `1402c3cf0.chunks.json` and `root-adapter-window.json`。呼叫者 `2c3dc8` is `ff90b8020000` (6字节),返回 `2c3dce`。历史代码快照 SHA256 is `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`模块基数 `140000000`;源/证明边界与文件中记录的相同 122 and 124。没有打开新的开发协会数据库。

最多每个会话有8个匹配引用, 钩子捕捉到一个在目录之前和三角洲之后的界限, 加上顶部 80-字节矩阵描述符和堆栈头。它记录在同一线条上同步发生的简单/材料执行调回。 Nesting 恢复了以前的 TLS 观测指针。原生的原生运行一次,并转发返回的 RAX 位;转语义不能推断。

计数只在现有的捕获生命周期排出活动读取器后打印,它们不能证明有效载荷所有权、全场完整性、缺乏内部/同步冲洗、GPU执行或像素可见度。恢复的矩阵报告意味着身份识别 已保存的堆栈头和上标; 更改的分配头可以不建立渲染错误而使其作假。 未知的类型、 较大的目录或更改的前缀拒绝收藏; 它们不能授权部分放置 。

## 离线核查

- `UiSceneRootCandidateTest`: 保存指令/表漂移, 错误的呼叫者, 读取失败, 溢出, 图表重读种族和矩阵- 存储/ 上位变化; 拒绝时输出不变 。
- `UiSceneBatchTest`, `UiGroupPlacementTest`, `UiHudLayoutTest`: 全部通过在同一冷冻的实验建筑中.
- 与新观察者和前一个 VRDof 一起编译的完整 DLL61 选项设置。ABI/自测试加载器返回 0 对抗这个DLL。
- 实验 DLL : `analysis/native-build-ui-scene-root1/GearsCaptureUiSceneRoot1.dll`, 莎莎 (英语).256 `8acf295a4cc7136bf4e1109edaf2f5969cbfa434abc34f19d44bb82614775986`.

冻结源覆盖散列和构建日志被忽略 `analysis/runtime/ui-scene-root1-*`; 现有的本地 MinHook 源头被复制到记录的散列, 没有安装工具。 第一次配置尝试缺少被忽略的供应商源, 并且被纠正。 初始的自测试引用缺乏其 DLL 参数并返回使用代码 2; 更正引用通过。 没有执行游戏、模拟器、 GUI 或 XR 运行时动作, 也没有替换本地默认的显示或已有的测试包 。

## 留在UI运动前

使用协调的实际运行来确定这条路线覆盖了所报道的场景,获得其布局/坐标合同并检查收集/浮点行为。然后添加新的场景/客户/本地玩家的录用,并携带一个不可改变的团体身份通过 简单的索引化和物质化的消费者在一次应用共享的放置食谱之前都会使用。该观察者提供缺失的运行时间采样点;仅靠静态表匹配无法授权这种转换。
