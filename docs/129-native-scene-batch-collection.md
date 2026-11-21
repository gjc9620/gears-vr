# Mixed native UI scene batch collection

2026-10-17. Offline source work only. This adds the producer collection component needed by the native PathChoice route; it does not enable the root hook or change any rendered UI.

The previous direct text collector selects one current sort key and only accepts simple batches. `ui_scene_batch.h` instead inventories all bounded FCanvas sort elements, recognizing the existing certified simple and material item layouts. It owns copies of meaningful 44-byte simple vertices, 80-byte transform descriptors and material payload evidence. It does not use a subtitle label for native scenes.

Two complete captures must agree on stack/element/item headers, pointer lists and copied contents. A delta matches keys by value, preserves old per-key item order, every old meaningful vertex prefix, and all existing material evidence. New keys, new simple/material items and simple vertex appends are supported. Relocating a vertex allocation is permitted when the owned old prefix remains identical. Removed items/keys, replaced payloads, unknown types, changed old transforms/materials, duplicate identities and failed reads reject without changing the caller's output.

Bounds are 64 keys, 128 total items and 1024 total copied simple vertices, using heap-owned vectors. Simple allocation capacity uses the existing certified 1048576 bound; capacity 2048 with count 4 is valid. Padding after the meaningful 44 bytes is excluded. These limits may reject a larger actual scene and must not be mistaken for proof that it is empty or safe to transform partially.

## Verification

Standalone MSVC `/std:c++17 /W4 /EHsc /UNDEBUG /MT` build and `UiSceneBatchTest` passed. Cases include mixed-key/material appends, key-array reordering, relocated vertex storage, spare capacity, exact limits, malformed caller inventories, prefix edits, item removal/reordering, duplicate keys/items/payloads, unknown types, material changes, read failures and mutations during capture. `inventory` also validates the final owned descriptor/header consistency before publishing it.

The CMake target is `UiSceneBatchTest`. Private build files remain under ignored `analysis/ui-scene-batch-cpu/`. No DLL was built and no game or simulator was launched.

## Integration still required

This is bounded repeated-read consistency, not an engine lock, atomic snapshot, allocator/ABA identity, or proof that a scene did not flush before the root returned. Scene/client/local-player admission, entry/completion checks, an immutable shared group receipt, actual indexed membership/material consumer checks, frame-ticket association and coordinate/cropping compatibility are still required before using the placement helper in production. If a root flushes internally, producer hooks must capture consumed work before that flush; a later delta cannot reconstruct it.

The native route and source provenance are in docs 122 and 124; the affine placement component is in doc 127. Existing producer consumers and subtitle behavior are unchanged.

---

## 中文

# 混合本地 UI 场景批量收藏

2026-10-17。仅离线源工作。这增加了本地路径PathChoice所需的生产者收藏组件;它不能启用根钩或更改任何已提供的UI。

之前的直接文本收藏器选择一个当前排序键,只接受简单的批次. `ui_scene_batch.h` 将所有有边框的 FCanvas 分类元素全部清点, 并承认现有的经认证的简单和材料项目布局。 它拥有有意义的副本 。 44- 字节简单的顶点, 80 字节变形描述符和有效载荷证据,它没有为原生场景使用字幕标签。

两种完整的抓取必须就堆栈/元素/项目头、指针列表和复制的内容达成一致。一个三角洲匹配按值的键,保存旧的每键项顺序,每个旧有意义的顶点前缀,以及所有现有的物证。新键,新简单/mat 支持序列项和简单的顶点附件。 当拥有的旧前缀仍然相同时, 允许重新分配顶点。 删除的项目/ 键、 替换的有效载荷、 未知类型、 更改旧变换/ 材料、 重复身份和失效 。d 读取拒绝,而不改变调用者的产出。

界是: 关 64 keys, 128 总计和 1024 使用堆积自有向量, 复制的简单顶点总数。 简单的分配能力使用现有的认证 1048576 约束; 容量 2048 计数 4 。在有意义的之后加注 44 字节被排除。 这些限制可能拒绝更大的实际场景, 并且不能被误认为是部分转换为空或安全的证据 。

## Verification

独立的 MSVC `/std:c++17 /W4 /EHsc /UNDEBUG /MT` 建造和 `UiSceneBatchTest` 实例包括混合键/材料附件、键盘重排、移位顶点存储、备用容量、精确限度、调用器清单错误、前缀编辑、项目删除/重排、重复键/项目/负载、未知类型、材料变化,在捕获时读取失败和突变. `inventory` 还在发布前验证最终自有描述符/标题的一致性.

CMake 目标为 `UiSceneBatchTest`。私有构建文件仍然被忽略 `analysis/ui-scene-batch-cpu/`. 没有建立 DLL,也没有推出游戏或模拟器.

## 仍然需要整合

这是有界的重复读一致性,而不是引擎锁、原子快照、分配器/ABA身份,或者证明一个场景在根返回前没有冲走。 Scene/client/local-player 承认、输入/完成检查、一个不可改变的共享组 receip t,在生产中使用投放辅助器之前,仍然需要实际的索引成员/物质消费者检查、帧-计程器协会和协调/裁剪兼容性。如果根冲出内部,则生产机钩必须捕获消耗的工作。冲浪前方,三角洲后方无法重建

本地路线和来源在文献中 122 and 124; 外接置放部分为 Doc 127 现有生产者消费者和字幕行为不变。
