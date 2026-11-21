# PathChoice: actual layout and deferred scene geometry

2026-10-17. Actual129 used the private Native69 UI-root observation build, SHA256 `a7ea494b0414426a281324e541d7dec2a7c73a7b0c02f45b38172c292836a070`, and the already fixed quiet2 simulator. The player default remains Native70. Original profiles/settings were protected before applying the previously recorded chapter fixture, then restored and hash-verified after normal game closure.

## Actual scene evidence

Five HWND-scoped forward pulses reproduced four movements followed by collision; every release had zero measured neutral drift. The subsequent GPU snapshot displayed CHOOSE PATH near the top. The earlier immediate movement probe had ended before observing this delayed scene, so collision was not evidence that the trigger never fired. This run used no foreground activation, global input, controller emulation, native function invocation or game-memory write. Physical controller background response stayed disabled.

After the scene appeared, the corrected optional reader obtained a consistent 13-node `DeltaChoosePath` / `UIScene_PathChoice` layout, a fresh owner graph classifying it as `local_player`, and current client/active-scene membership. The safe-region panel spans most of the screen; the actual title, background panels, arrows and choices occupy approximately y=72..260 in the scene's original 1080-height CPU coordinates. This agrees with the high placement visible in the GPU snapshot, without equating CPU bounds with GPU scissor ownership.

All eight bounded native root observations have the exact same scene address as that CPU scene. They show valid before/after inventories, four simple geometry ranges, 284 added vertices, zero material records, restored transform stacks, and zero simple/material Execute calls **inside** the root invocation. This identifies a concrete deferred geometry producer. It does not yet prove the later flush's exact indices, mixed-scene membership or replay lifetime, and does not authorize identifying arbitrary GPU draws by their coordinates.

## Reader corrections established by runtime

The right arrow has directed bounds approximately `[1331.812,148.392,1281.811,248.392]`. The old optional sampler rejected all layout evidence when left exceeded right. It now preserves those original edges/raw bytes, separately returns normalized `ordered_values`, and marks reversed axes. Finite-value, reflection layout, registration, ancestry and consistent-reread checks remain. Synthetic tests cover both axes, zero-size bounds, the actual recorded values and nonfinite values on every edge.

Owner sampling also rejected an unrelated, registered WarPC field whose FName number is 481. It now compares the complete `(name_index, number)` for required property membership while retaining every traversed field header in the final reread. Required properties still require the exact unnumbered identity and declaring owner; numbered siblings cannot shadow them. Corrupt ownership, registry membership and mid-read changes still reject.

Finally, the real one-player GamePlayers array has count=1 and capacity=17. The reader had applied its 16-member budget to allocator capacity. It now separates bounded occupied members from allocator slack, caps capacity at 4096, preserves address-span checks, and reads only the bounded member count. The complete graph still requires exactly one matching local player. Tests retain count, negative/overflow capacity and link rejection.

## Validation and next step

The menu-state suite passes 66 tests and scene-owner suite passes 13. Actual fixed layout/owner sampling succeeded with `consistent_reread=true`. The session stopped normally with native cleanup, 45 independently restored entries, XR exit0 and all owned processes gone. All four original profiles and the entire settings file were restored with hashes verified.

Next, associate the authenticated root's deferred ranges with existing receipt/generation/exact-index replay rules before applying a group transform. No UI placement fix or physical headset acceptance is claimed yet. Evidence is ignored under `analysis/runtime/vrdof129-*` and `continuous-stereo-36772-20261017-231358-90f5aede.*`; no proprietary derivative is shipped.

---

## 中文

# PathChoice: 实际布局和延期场景几何

2026-10-17 实际129 使用私人土著69 地下观测大楼,SHA256 `a7ea494b0414426a281324e541d7dec2a7c73a7b0c02f45b38172c292836a070`,和已经固定的静默2 模拟器。 玩家默认值仍然是原生70。原始配置/设置在应用先前记录的章节固定之前受到保护,然后在正常游戏关闭之后恢复和散列验证。

## 实际现场证据

5个HWND 扫描的前进脉冲复制了4个运动,然后发生碰撞;每次释放都具有零度测量的中性漂移。随后的GPU快照显示CHOOSE PATH靠近顶部。早期的即时移动探测器在观测到此之前就已经结束了。 延迟场景, 因此碰撞不是触发从未发射的证据。 此运行没有使用前置激活、 全局输入、 控制器仿真、 本地函数引用或游戏记忆写入 。 物理控制器背景响应被禁用 。

现场出现后,被更正的可选阅读器获得了一致的 13-node `DeltaChoosePath` / `UIScene_PathChoice` ,将它分类为 `local_player`,以及当前客户端/活动-场景成员。安全区域小组横跨大多数屏幕;实际标题、背景面板、箭头和选择占大约 Y=72..260 现场原创 1080-height CPU坐标。这与GPU快照中可见的高位置一致,而不将CPU的边界与GPU剪刀所有权等同起来。

所有8个有界的本地根观测都与CPU场景完全相同。它们显示在目录前/目录后有效,四个简单的几何范围, 284 添加顶点、零材料记录、恢复变换堆栈和零简单/材料执行呼叫 **inside** 根引用。此选项可识别一个混凝土的推迟几何制作人。它尚未证明后冲的确切指数、混合场面成员或重播寿命,也未授权通过坐标识别任意的GPU绘图。

## 按运行时间确定的读者更正

右箭头已经左右方向 `[1331.812,148.392,1281.811,248.392]`。旧的可选采样器在左侧超过右侧时拒绝了所有布局证据。它现在保存了这些原始边框/绘图字节,并分别返回归正 `ordered_values`。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴。“符号”和“符号”的反向轴”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“符号”“。“符号”

所有人取样也拒绝一个不相关的、注册的WarPC字段,其FName号为 481。现在它比较了完整 `(name_index, number)` 需要的属性仍然需要准确的无编号身份和宣布所有者; 编号的兄弟姐妹不能给他们阴影。 腐败的所有权、 注册成员hi p和中读的更改仍然拒绝.

最后,真正的单人游戏游戏机GamePlayers阵列有计数=1 和容量=17 读者应用了该词。 16 将被占用的成员与分配者松懈、限制其能力区分开来。 4096,保留地址宽检查,只读入受限成员数。完整的图表仍然需要完全匹配本地玩家。测试保留了计数、负/流能力以及链接拒绝。

## 验证和下一步

菜单状态套房的通行证 66 测试和现场业主套房通行证 13 实际固定布局/所有者取样成功 `consistent_reread=true`。正常情况下,会议停止了本土清理, 45 独立恢复条目, XR 退出0 全部恢复了全部四个原始配置文件及整个设置文件,并核实了散列。

其次, 在应用组变换之前, 将已认证的根的延后范围与现有的收/ 生成/ 精确索引重放规则联系起来。 目前尚未要求使用 UI 设置固定或物理耳机接受。 证据在下面被忽略 。 `analysis/runtime/vrdof129-*` and `continuous-stereo-36772-20261017-231358-90f5aede.*`;没有发运专有衍生物。
