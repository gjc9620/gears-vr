# Fresh PathChoice owner graph sampling

2026-10-17. Offline implementation and synthetic verification only. No game, simulator, process memory, GUI, input or XR Runtime was accessed during this work.

`tools/game-io/scene_owner_snapshot.py` adds `capture_scene_owner(resolver, client, scene, expected_controller, expected_local_player)`. The caller owns an exclusively used, prepared/build-verified Resolver. The helper checks the supported profile, base/PID and fresh relevant UObject/name/registry fields. It does not hash the executable itself; the output names the profile's game digest `profile_game_sha256`.

The sampled graph requires:

- Exact GameUISceneClient class, its UIInteraction outer and reciprocal SceneClient.
- Interaction's viewport outer and the viewport UIController backlink.
- Expected LocalPlayer's ViewportClient and Player.Actor, plus expected PlayerController.Player.
- A common Engine outer with exactly one GamePlayers member equal to that local player.
- Unique ActiveScenes membership, UIScene_PathChoice/UIScene ancestry and the scene's SceneClient backlink.
- PlayerOwner equal to that player, or null under the complete single-player graph. Null is labelled `unowned_single_local_player`; this does not infer the engine's render scope or authorize placement.

Cached reflection supplies candidate addresses only. Selected properties are independently checked for fresh identity, exact name/kind/defining owner, dimensions/element size/offset, actual field-chain reachability, earlier property shadows and array-inner ownership/type. Unrelated fields use lightweight index/slot/outer/FName/next headers rather than decoding every field's name/class. Hint reads share the bounded read budget and the original/nested reader is restored even on failure.

All accepted source bytes are reread and registry headers close the acquisition. Result evidence contains bounded identities/classes/property/field-header closure, graph links, arrays, counts and a source-byte digest. It explicitly reports no atomic snapshot, allocation identity, render admission or GPU proof. It is not interchangeable with the root callback's FCanvas, target, frame ticket or draw membership.

## Realistic bounds and tests

The existing package inventories contain 1,359 immediate WarPC entries and 453 PlayerController entries. These are exported declarations, not a current runtime field-chain measurement. They exposed the original 512-per-class limit as unsuitable for this schema. Bounds are now 2,048 fields/class, 4,096 total fields, 32,768 total read calls and 2 MiB read bytes. The synthetic 1,359 + 453 scale fixture succeeds in 19,538 reads while preserving late-header mutation rejection; actual process timing remains unmeasured.

Inventories: Engine objects.tsv SHA256 `cf331d8b9142183f6420d5d68ebfe55082f5d33340209645feebd06d50b7d719`, WarfareGame objects.tsv SHA256 `c414f4b797705af9991d16a9035f59e27c61174efe4c2fd1beb92a63dd5a8c0a`. Counts/script remain under ignored `analysis/runtime/scene-owner-field-counts.json` and `count-owner-field-inventory.py`. They are script-package evidence, separate from native historical code bytes.

195 tests passed across owner sampling, menu state/control/navigation/display monitoring and staging/packaging. Coverage includes stale hints, wrong/unreachable/shadowed properties, metadata and graph mutations, budget/read failures, exact-scale chains, default-path preservation and restoring nested readers.

## Optional menu integration

`MenuReader.sample(include_owner=True, expected_controller=..., expected_local_player=...)` shares the existing outer sample verification. CLI `--owner` requires paired `--owner-controller` and `--owner-player` current addresses; incomplete arguments reject before opening a process. It can accompany `--layout` to gather both CPU graph and layout evidence when the branch UI is visible. Default menu sampling performs no owner reads. The portable allowlist includes the helper; existing ZIPs were not replaced or repackaged.

Next actual work requires a coordinated visible PathChoice sample, compared with the native root observer's scene identity and collection/flush behavior. Neither this CPU graph snapshot nor passing synthetic tests enables UI movement. Fresh native entry/completion admission and shared simple/material receipts remain to be integrated after that evidence.

---

## 中文

# 新鲜路径选择所有者图样

2026-10-17。仅离线执行和合成验证。在这项工作中,没有访问游戏、模拟器、进程内存、图形用户界面、输入或XR运行时间。

`tools/game-io/scene_owner_snapshot.py` adds `capture_scene_owner(resolver, client, scene, expected_controller, expected_local_player)`。呼叫者拥有一个专门使用、准备/建立-验证的解析器。帮助者检查支持的配置文件、基础/PID和新的相关的 UObject/name/registry 字段。它不给可执行文件本身造成散乱;输出时将描述文件的游戏摘要命名 `profile_game_sha256`.

抽样图表要求:

- 精确的GameUISceneclient类,其UIInteractive外置和对等的Sceneclient.
- 互动的视图端口外侧和视图端口UI控制器回路.
- 本地服务器的视图端口和玩家。 显示器, 加上预期的 Player Captain. Player 。
- 一个普通引擎外置, 完全有一个GamePlayers成员 等于那个本地玩家。
- 独占活动成员 UIScene_PathChoice/UIScene祖先与场景的SceneClient回路.
- Player Owner 等于该玩家, 或者在完整的单玩家图表下无效。 Null 被标记为 `unowned_single_local_player`;这不能推断发动机的渲染范围或授权放置.

仅对缓存反射供应候选地址进行独立检查。所选属性将发现新身份、确切名称/类型/定义所有者、尺寸/要素大小/抵销、实际可到达的野外链、较早的财产阴影和阵列内自有 ership/ 类型。 无关字段使用轻量级索引/ slot/outer/ FName/ next headers 而不是对每个字段的名称/ 类进行解码。 提示读取共享绑定的读取预算, 并且即使失败, 原始/ 取消的读取器也会恢复 。

所有接受的源字节都重新读取,登记页眉关闭了获取。结果的证据包含有边框身份/类/财产/字段-标题关闭、图链接、数组、计数和源字节摘要。它明确报告没有原子快照, 配置身份、 提交录用或 GPU 证明。 它不能与根调回的 FCanvas 、 目标、 帧票或 抽签会员身份互换 。

## 现实界限和测试

现有一揽子清单载有: 1,359 立即输入和 453 玩家控制器条目。 这些是导出声明, 而不是当前运行时的字段链测量。 它们会曝光原始 512- 适合这个计划,每个等级的限制,现在有界号了 2,048 fields/class, 4,096 总计字段, 32,768 总计读取电话和 2 米B读取字节。合成 1,359 + 453 缩放固定符成功 19,538 读取时保留了后发头突变的拒绝;实际过程时间仍未计量。

库存:引擎对象.tsv SHA256 `cf331d8b9142183f6420d5d68ebfe55082f5d33340209645feebd06d50b7d719`, WarfareGame 对象.tsv SHA , 存档副本 。256 `c414f4b797705af9991d16a9035f59e27c61174efe4c2fd1beb92a63dd5a8c0a`。计数/标语仍然被忽略 `analysis/runtime/scene-owner-field-counts.json` and `count-owner-field-inventory.py`。它们是脚本组合证据,与本地历史代码字节分开。

195 测试通过所有者取样、菜单状态/控制/导航/显示监测和中继/包装。覆盖范围包括: stale提示、错误/无法获取/阴影属性、元数据和图表突变、预算/阅读失败、精确比例链、默认p 保护和恢复巢读器。

## 可选菜单集成

`MenuReader.sample(include_owner=True, expected_controller=..., expected_local_player=...)` 共享现有的外部样本验证。 CLI `--owner` 需要对齐 `--owner-controller` and `--owner-player` 当前地址; 在启动进程前拒绝不完整的参数。 它可以伴随 `--layout` 当分支 UI 可见时,可以同时收集 CPU 图表和布局证据。默认菜单采样没有显示所有者。可移植的允许列表包括帮助器;现有的 ZIP 未被替换或重新包装。

下一步的实际工作需要协调可见的 PathChoice 样本, 与本地根观测者的场景身份和采集/ flush 行为相比。 此CPU 图示和通过合成测试都无法实现 UI 移动 。 fresh 本地条目/ comple 电磁接收和共享简单/材料收据在证据之后仍有待整合。
