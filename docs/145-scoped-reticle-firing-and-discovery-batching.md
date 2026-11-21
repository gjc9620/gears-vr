# Scoped background firing and camera-class discovery batching

2026-10-17. Actual121 and Actual122 used Native67 DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`, frozen native source `f55f713`, with the Python discovery change below. These tests establish a scoped background firing method and new paired reticle evidence; they do not accept the reported headset reticle defect or mounted gameplay.

## Bounded startup change

`Resolver.find_camera_class()` now uses the existing `discovery_identity_reader()` for its single registry scan. The reader's bounded page hints live only for that scan, fall back to exact reads, and do not supply certificate validation. Camera-class predicates, registry/index rejection, subsequent instance/link/layout/POV checks and fresh acquisition on retry remain intact. No wait or retry cap changed.

Synthetic integration coverage compares the former exact-read scan and the batched scan for successful discovery, missing class and index mismatch. It verifies identical discovered identities/rejections and fewer underlying reads. The bundled Python passed 18 discovery, name and initial-preparation tests.

Actual120's certificate acquisition including retry was 3618.36 ms. Actual121's was 3163.26 ms, with an observer-preparation registry change causing reacquisition. Actual122 also succeeded but native preparation took 5025.81 ms; its first projection followed camera readiness by 5116.18 ms. Different retry/registration timing prevents a causal performance claim from these runs. This optimization reduces synthetic discovery reads; startup latency remains open.

## Scoped input established

Actual121: `continuous-stereo-42928-20261017-204506-fc307b39`. Right/left mouse down/up messages were posted only to the fresh identity-checked game HWND after a verified first stereo submission and a consistent gameplay menu state. There was no focus action, global controller input or game-memory write; physical background controller response remained disabled. Foreground HWND remained 789028 before and after. The current live weapon was `Weap_AssaultRifle`.

The strict read-only ammunition observer completed 160 accepted samples; `AmmoUsedCount` advanced from 0 through 11. This establishes actual game response to this scoped firing sequence. The earlier W-only negative result in [docs143](143-scoped-background-input-test.md) remains valid for that bounded test; it did not prove that every scoped gameplay input is unusable. Walking, turning and mounting remain to be demonstrated independently.

Actual121's four paired outer/inner draw groups, tickets1009–1012, had identical per-eye viewport/scissor maps. They preceded left-button down; firing GPU image ticket1030 is a different ticket. Those pairs alone are not firing-frame pixel proof.

## Same-ticket firing evidence

Actual122: `continuous-stereo-19184-20261017-205029-ed02aaef`. The first diagnostic helper exited before any input because its startup index did not yet contain a session path. The helper was corrected to wait for the path, and resumed against the same running session; the game was not restarted for that observation failure.

The subsequent scoped sequence posted aim/fire together and requested a bounded set of early GPU snapshots. Every posted down had its corresponding up in the dispatch ledger, with no release error, and foreground HWND again remained 789028. The ammunition observer accepted values0,1,2 before rejecting registry growth and ending incomplete. Thus this is partial actual firing evidence, **not** a passing eight-second observer run. The original failure is retained in `vrdof122b-ammo.json`; it was not hidden by rebinding.

Four paired native groups were recorded at tickets261–264. Each group contained an outer 16-vertex/four-receipt payload and a separate inner four-vertex/one-receipt payload sharing root, ticket, command list/generation and target. Actual index callback, command completion and geometry matching were recorded. Both parts' viewport/scissor maps were identical for each eye. All72 considered reticle draws used the finite plan: no center, geometry or projection rejection and no direction fallback.

Three groups match saved GPU images by exact ticket:

| Ticket | GPU snapshot index | SHA256 |
| --- | --- | --- |
| 261 | 2 | `747cb519bc375f3d7aa990ce684e9c4d41f8f263db74bb40453c2a47076617c6` |
| 262 | 3 | `e70b07ce1d5da6b25b93de9c2a755a39814362e1c85fccd8fa015c3afb3f10e5` |
| 264 | 4 | `825822b96f8c3f65ea8de3923214a95946314e21a10037b3071f12140446b4e6` |

The 2534×1009 final GPU images contain both scene halves. Coordinating inspection of bounded crops shows the sampled aim/fire content, but the near-white pixel heuristic also selects background lights and is not semantic reticle segmentation. It does not prove visible inner/outer concentricity, bullet impact alignment or headset fusion. The matched samples rule out differing inner/outer viewport/scissor maps for these groups, not all weapons or recoil states. The known cross-payload quadrant coverage gap remains a separate source-level possibility that these complete four-quadrant payloads did not exercise. No guessed reticle offset or widened admission follows.

## Lifecycle and remaining work

Both 60-second read-only monitors passed. Native/XR stop and cleanup, entry restoration, normal owned-game closure, all owned-process exits, four profile restorations per run and full original settings restoration were verified. Native source/defaults/ZIPs were unchanged; the Python source change is local and not yet in the previous portable ZIP.

Ignored summaries: `analysis/runtime/vrdof121-result.json`, `vrdof122-result.json`, both `*-reticle-pairs.json` files, `vrdof122-matched-reticle-pixels.json` and the underlying native/GPU/menu/ammunition/session evidence. Mounted-machine-gun output, PathChoice placement, the reported reticle appearance, startup latency, headset pacing and extended continuity remain open. `headset_verified=false`, `release_ready=false`.

---

## 中文

# 范围为背景射击和照相机级的发现批量

2026-10-17 实际121 实际数122 已使用原生67 沙发256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`,冷冻本地来源 `f55f713`,下面有 Python 发现的改变。这些测试建立了瞄准镜背景射击方法和新的对联的雷管证据;它们不接受所报告的耳机雷管缺陷或挂载游戏播放。

## 启动时有固定的更改

`Resolver.find_camera_class()` 现在使用已有的 `discovery_identity_reader()` 用于进行单一的注册扫描。读者的限定页面提示只为扫描而活,返回到准确的读数,而不提供证书验证。 重试时的获取保持不变。没有更改等待或重试盖 。

合成集成覆盖比较了以前的精确读取扫描和批量扫描,以了解成功发现、缺失的分类和索引不匹配。它验证了相同的发现身份/拒绝,并减少了基础读取。捆绑的 Python 通过了 18 发现、名称和初步准备测试。

Actual120 包括重试在内的证书的获取 3618.36 实际数121 曾经是 3163.26 4 ,ms,加上观察员准备登记处的变更导致重新购置。122 也成功但本土准备 5025.81 ms;它的首次投影是在相机准备后 5116.18 ms. 不同的重试/注册时间会阻止这些运行的因果性能要求。 优化后可减少合成发现; 启动延迟性仍然开放 。

## 已建立的范围范围投入

Actual121: `continuous-stereo-42928-20261017-204506-fc307b39`。右/左鼠标下/上的信息只发布到新身份检查的游戏 HWND 中,此前已核实的第一个立体提交和一致的游戏播放菜单状态。没有焦点动作、全局控制器输入或游戏记忆写入;物理反转 声音控制器响应仍然被禁用。 前景 HWND 仍然保留 789028 之前和之后,现在的活武器是 `Weap_AssaultRifle`.

完成严格的只读弹药观察 160 接受的样品; `AmmoUsedCount` 高级从 0 through 11。这确定了游戏对这个瞄准射击序列的实际反应。较早的W-只负结果为 [docs143](143-scoped-background-input-test.md) 仍然适用于此设定的测试; 它没有证明每个范围游戏输入都无法使用。 步行、 转弯和挂起仍然需要独立演示 。

Actual121 4对外/内绘组合,票1009–1012,有相同的每眼视窗/剪刀地图。它们先于左键下; 启用 GPU 图像票1030 单是这两张就不是射击框架像素证明

## 同样的枪托射击证据

Actual122: `continuous-stereo-19184-20261017-205029-ed02aaef`。第一个诊断帮助器在输入前退出,因为启动索引尚未包含会话路径。帮助器被更正为等待路径,然后恢复到同一运行的会话;该观察 fai 游戏没有重新启动lure.

随后的瞄准序列张贴了瞄准/火力,并要求一套有界限的早期GPU快照。每个下方的调度分类账都有相应的内容,没有发布错误,前方的HWND再次保留 789028 弹药观察员接受了价值0,1,2 从而成为部分实际发射证据, **not** 输入了 。 `vrdof122b-ammo.json`;它没有被重新绑定而隐藏。

4对土著群体在票上被记录261–264。每组包含一个外层 16- 顶点/四接收有效载荷和单独的内四顶点/一接收有效载荷共享根,票,命令列表/生成和目标. 实际索引回调,命令完成和几何匹配被记录下来. 两部分的视图/剪切图是id 每只眼睛都贴上牙72 被认为是Reticle绘图使用了有限的计划:没有中心、几何或投影拒绝,也没有方向倒转。

三组匹配 GPU 图像保存 准确的票:

| Ticket | GPU 即时索引 | SHA256 |
| --- | --- | --- |
| 261 | 2 | `747cb519bc375f3d7aa990ce684e9c4d41f8f263db74bb40453c2a47076617c6` |
| 262 | 3 | `e70b07ce1d5da6b25b93de9c2a755a39814362e1c85fccd8fa015c3afb3f10e5` |
| 264 | 4 | `825822b96f8c3f65ea8de3923214a95946314e21a10037b3071f12140446b4e6` |

The 2534×1009 最终的 GPU 图像包含两个场景的两半。 协调对有边框的作物的检查显示被采样的目标/ 火含量, 但近白像素的高度也选择了背景灯, 而不是语义的 retuble 片段。 它没有证明可见的内涵 。r/outer 的同心圆、弹射对齐或头部组合。匹配的样本排除了这些组的不同内/外视港/剪切图,而不是所有的武器或后坐力状态。已知的跨负载四象限覆盖差距仍然是单独的酸度。ce级可能使这些完整的四水量有效载荷没有运动,因此没有猜测的回旋器抵消或扩大吸收。

## 生命周期和剩余工作

Both 60- 第二次只读显示器通过。 本地/ XR 停止和清理、 输入恢复、 正常自有游戏关闭、 所有自有进程退出、 每运行四个配置恢复以及完整原始设置恢复 。 原源/ 默认/ ZIP 已解析 。nged; Python 源变化是本地的,尚未在上一个可移植的 ZIP中.

忽略摘要 : `analysis/runtime/vrdof121-result.json`, `vrdof122-result.json`,两者 `*-reticle-pairs.json` files, `vrdof122-matched-reticle-pixels.json` 机载机枪输出、PathChoice放置、据报的列车外观、启动时的空档、头盔间隔和延长的连续性仍然开放。 `headset_verified=false`, `release_ready=false`.
