# Tick/C0 body main-mount source contract

Implementation draft,2026-10-26. Body owns the shared header and Native implementation; Weapon owns the PG equipment-witness implementation after header agreement. No default or runtime acceptance is implied. The old Rig/Hand/supplemental main-mount V1 interfaces remain unchanged, including their complete old-tracking freshness and world-grip checks.

## Reason for the separate source

EO used the same Native46 and original action with one PG parsing change. All19 held records had exact planned consumer/after Local and matching FK, but the original40 held/6s gate still failed. Saved ready-prefix PG query medians were EN6.5387ms and EO3.7711ms. Original Scene-to-publication remained about6.27ms and publication-to-C0 about30ms. EO controller sync preceded that Scene by median8.1922ms. These are different prefix observations, not a paired FPS benchmark.

The old tracking is an actual qualification dependency: Native Broker before/after checks, PG Client Frame freshness, bind's exact stereo/controllers and world-grip recomputation, and old-right-Grip usability. It cannot simply be called unused metadata and have its checks removed. Conversely, the current body target uses genuine new Tick raw tracking and current C0 neutral/CTW, while the returned mount is a fixed, certified model calibration. The new source makes those dependencies explicit rather than forging a new Scene or changing an old timestamp.

## Three separate inputs and authority

| Part | Actual origin | Required current checks |
| --- | --- | --- |
| Original Scene/device ancestry | Original cached Core projected without modifying any field; exact committed parent Hand Scene key | Native incarnation/cookie/lifecycle/action generation, world/controller/pawn/mesh/weapon/reference/model/units, immutable original cache association and source ticket. Original Scene observation and publication remain within50ms. Old tracking is historical data; its V1 validation can remain stale. |
| Tick tracking | One actual certified outer MeshTick acquisition of matching raw HMD/controllers/continuity | Original sync/receive/acquisition times within40ms, exact source/session/sequence/space/publisher/profile/input/continuity/reference, no regression, no second acquisition or reference mutation in C0. |
| Current C0 basis | Actual canonical Compose, actual capture-start time, current actor/CTW/reference/model and actor-local authored neutral | Capture within40ms; actual GT/world lease/caller, current actor/metadata/model geometry/scale/units/reference; exact immutable basis/source association before/after query and before each write. |

Ids and matrices are data. A request claiming a Tick, cache revision or Compose cannot authenticate itself. Native constructs this source only inside the existing certified owner and admits it through the exact live parent broker. Native alone owns pose conversion, target composition, fixed-profile reconstruction and the existing single CAS/Kernel writer.

The original Core/Rig/Hand payload is never patched with newer poses or clocks. A pure projection for the new historical-association field must be named and checked as such; it is not a successful V1 fresh read. No `now` substitution, source renewal, cache permission or producer-time relabeling is permitted. A failed new dependency cannot resample or silently fall back to older tracking.

## Shared data shape

Draft header: `tools/vr-common/body_main_mount_provider.h`, namespace `gears_body_main_mount_provider`, separate export `GearsVrAttachBodyMainMountProviderV1`. Its version1 is a new contract, not a version change to the existing weapon main-mount V1.

- `Source.original_scene` retains the complete original Rig POD and original timestamps. `scene_ticket` and `scene_cache_revision` refer to the actual Native cache/latch association.
- `Tracking` carries actual Tick/thread/cookie, acquisition interval and unmodified raw Stereo, controllers, continuity, publisher and Native reference revision.
- `Basis` carries canonical Compose, actual capture-start, certified `BodyGeometry`, current component-to-world and base view. PG returns no world pose.
- The admitted Broker assigns query id and snapshots actual Hand lifecycle/action generation. The three age ceilings are fixed at new tracking/capture40ms, original Scene50ms and fresh equipment50ms.
- `Response` echoes the exact source/basis key, query and Native parent stamps, and carries actual equipment observation times plus fixed mount data. The embedded mount's original FrameKey remains the Scene/device key, never Tick/Compose.
- Registration uses the same active Hand parent context/cookie/pinned image, existing callback counts, reentry exclusion, stop/revocation/action-barrier and drain. No independent registrar/pin/detach or equipment-call capability is created. Large scratch stays in persistent owned storage.

The Broker's actual C0 validator is fixed and Native-private, not a public POD permit. Under its mutex the Broker authenticates/snapshots parent and original Scene association, reserves query scratch and inflight lifetime; it releases that mutex before actual C0/source/basis validation and PG. It reacquires it to check exact parent/revocation/query state before publishing. Do not call a full context function that reenters Broker while holding this mutex. The source validator must not use the generic zero-pending-callback parent predicate to reject its own precisely reserved query, and must not globally subtract callback counts. Parent validation and Native source validation remain distinct; every later write still requires the normal fully idle parent.

Two distinctions are mandatory. The committed Hand Frame does not contain Rig publication/binding generation: only Native can authenticate those against its Core/cache. PG can independently obtain the original Hand key, five object identities and reference generation; echoing additional Native ids is correlation, not a claim that PG observed them. Likewise PG cannot observe Native's private action generation. It echoes that broker stamp; Native verifies generation/barrier before/after. PG independently checks its existing domain/shot/action depth and its own Client epoch, which it captures locally under its Gate rather than trusting a requested epoch.

`Tracking.reference_revision` is Native's actual `reference_snapshot` revision, the same source as Core.native_reference_revision. It is distinct from original_scene.reference_generation, which is the original StereoGate generation and is matched to the actual committed Client Frame. Never compare those two counters for equality. PG checks the Native revision's shape/echo only and exact stable NativeReference against the original Scene. Native verifies the actual current snapshot revision and complete NativeReference before/after and every write; the request cannot authenticate its own Native revision.

## Query-local PG equipment witness

Add one separate synchronous operation under the Client's existing Gate/epoch/parent lifetime. It extracts the exact registered committed key and identity fields as ancestry, without returning or manufacturing a Hand Frame and without using old tracking as the new tracking qualification.

The fixed read-only provider performs a fresh Game/held observation, both original typed asset reads, exact right attachment and the independent final Game/held observation with `same_game`, hold generation and held binding. All original object/registry/class/outer/property/attachment repetitions remain. The fresh observations enclose the metadata work; registration/domain/epoch/held generation are rechecked at the end. Equipment begin/end are actual clocks from this call and must remain within50ms. New right-Grip eligibility uses the new authenticated raw bundle, including usable/active/focus/continuity and40ms checks.

Take equipment begin before the first actual Game read and end after both assets/attachment/final Game and epoch/hold/domain checks. Require positive ordered nonfuture times and measure age from begin; end cannot renew earlier evidence. Native validates both outer and embedded mount query ids against the request, both headers/reserved fields, exact old Scene FrameKey/lifecycle/identities and geometry/calibration. SourceKey is deliberately an echo subset; exact complete-source/reference/basis checks remain necessary even when it matches.

Reuse the real metadata primitives, factoring only where the existing reader's Frame carrier is incidental. Do not call unchanged `copy_frame_on_gt`, `source_game_on_gt` or the outer `attached_frame` wrapper and pretend the old Frame is fresh. Do not recursively enter a Client Gate. PG returns query-bound equipment/asset/calibration metadata, no world pose, shot, ownership grant or arbitrary calibration. Native converts the new raw grip using its certified current basis and independently reconstructs the fixed Lancer mount.

## Native source and write path

The new mode needs its own typed source classification/admission, not a weakened old `same_source` predicate. Every hard identity, cache/ticket, reference, world/actor/metadata/model, lifecycle/action and structural/nested/stop check carried by the old route must have an explicit equivalent. Keep the old observer's historical age failure separate from the new source's qualification, with permanent independent hard-fault propagation and original node/stack before/after checks.

Each actual write still checks immutable new raw/capture40ms, original Scene-observation/publication50ms, query-local equipment evidence50ms, current parent/actor/metadata/model/reference and the original CAS/Kernel protocol. No successful guard is cached as permission; no second writer, alternate restore receipt, repeated notify or new packet within C0 is introduced. Retained consumption after a real commit stays a separate authenticated same-lease operation; it cannot generate a new write.

The actual pre-query parent identity/revision/action-generation snapshot must remain a dependency after any query result, including non-ready. A parent revoked during the query may not turn into a permitted freehand write in that C0. Keep this snapshot separate from accepted equipment evidence. It is stored after the synchronous query returns, so the Native source validator does not run a normal zero-callback predicate against its own reserved callback. Every later write checks the normal idle parent again.

An acquired nonzero Tick parent cookie is already a hard identity dependency before selection or any query. Every current/retained source check requires that exact registration and Native incarnation still exist; disappearance or replacement cannot take a pre-query freehand early return into an allowed write. This identity check does not require zero callbacks. A Tick acquired with cookie0 retains the independent freehand bootstrap path.

After the exact Kernel lease has actually been consumed, elapsed equipment age alone must not poison original control consumption. The retained operation still checks hard source continuity and normal idle parent identity/revision/action/barriers/debt; it does not renew equipment age or grant a new write. A new write still requires equipment age from begin within50ms. This distinction is covered by a targeted equipment49→51ms Host boundary, independently of fresh raw tracking.

## Minimum proof before one original game action

Use one concrete boundary case with old tracking>50ms, original Scene<50ms, genuine new Tick raw/capture<40ms and fresh equipment observations. V1 must still reject stale. The new typed route may pass only under its distinct source, through the same actual query, native context and each real write, with recorded original source ids/times/basis and response association. A reference/source/parent revocation between query and write must deny without substituting a packet; stop uses the existing callback drain. Host seams and partial-retirement evidence stay explicit.

After the shared header/ABI and this bounded path are checked, build a separately identified candidate and run the unchanged retail0.4 background action and40/6s gate. Keep prior EN/EO failures and all original restoration checks. Do not infer continuous held, aiming/fire, full cover, run-camera or headset completion from this contract or a successful Host boundary case.

---

## 中文

# Tick/C0 机构主货源合同

执行草案,2026-10-26。机体拥有共享头和原生执行;武器拥有PG设备-证人执行,在头协议后,不暗示默认或运行时间接受。旧的 Rig/Hand/supplement主山V1 界面保持不变,包括完整的旧轨新鲜度和世界-粮食检查。

## 单独来源的原因

欧文使用相同的原生46 和带有 PG 解析更改的原始动作。所有19 持有的记录有准确的计划消费者/在本地和匹配的FK之后,但原始40 held/6 s 门仍然失败。 已保存的已准备的 PG 查询中位数为 EN6.5387 毫秒和欧元3.7711 原文原稿到出版物的原稿仍为6.27 毫升和出版物对C0 about30 毫秒 EO 控制器同步在以中位数表示的场景之前8.1922 ms. 这些是不同的前缀观测,而不是对齐的FPS基准.

旧跟踪是一个实际的资格依赖:检查前/检查后本地经纪,PG客户端框架新鲜度,绑定精确的立体/控制器和世界-grip重制,以及旧右-Grip可用性。它不能简单地称为未使用的元数据和 hav e 取消了检查。反之,当前身体目标使用真实的新滴答原追踪和当前C0 中性/ CTW, 而返回的挂载是一个固定的,经过认证的模型校准。新来源使这些依赖性明确,而不是构建新的场景或改变旧时标。

## 三条独立的投入和权力

| Part | 实际来源 | 需要的当前检查 |
| --- | --- | --- |
| 原始场景/场景祖传 | 原始缓存 核心投射而无需修改任何字段; 精确承诺的父手景密钥 | 原生化身/饼干/生命周期/动作生成、世界/控制者/当/mesh/武器/参考/模型/单位、不可改变的原始缓存协会和原始票。50 ms. 旧跟踪是历史数据;其V1 验证可以保持停滞状态。 |
| 滴答跟踪 | 一次实际认证的外 MeshTick 获取匹配的原始 HMD/控制器/连续 | 内部的原始同步/接收/获取时间40 ms, 准确源/ 会话/序列/ 空间/ publishher/ profile/ input/ continual/ reference, 无回归, 无第二次获取或参考突变在 C0. |
| 现 C0 basis | 实际装配、实际捕获启动时间、当前演员/CTW/参考资料/模型和演员-当地作者的中立 | 内捕40 ms; 实际GT/世界租赁/调用器, 现时的演员/元数据/模型几何/尺度/单位/参考; 查询前/查询后和每次写作前准确的不可变的基/源关联. |

IDs和矩阵是数据。 请求请求一个 Tick、 缓存修订版或 Compute 无法认证自己。 本地人仅在现有认证的拥有者内部构建此源, 并通过精确的活性母经纪人承认它。 仅土著人拥有 Conver sion,目标组成,固定标注重建以及现有的单 CAS/Kernel 写入器.

原Core/Rig/Hand有效载荷从未加装新的姿势或时钟。必须命名和检查新历史联系场的纯投影;它不是一个成功的V1 刚读。没有 `now` 允许替换、源更新、缓存许可或生产者时间重新标签。失败的新依赖无法重新模拟或默默地回到旧的跟踪。

## 共享数据形状

草稿头 : `tools/vr-common/body_main_mount_provider.h`名称空间 `gears_body_main_mount_provider`,单独导出 `GearsVrAttachBodyMainMountProviderV1`。其版本1 是一个新合同,而不是对现有的武器主峰V的版本修改1.

- `Source.original_scene` 保留完整的原装Rig POD和原装时间戳。 `scene_ticket` and `scene_cache_revision` 参考实际的土著缓存/挂链协会。
- `Tracking` 载有实际的Tick/thread/cookie,获取间隔和未修改的原始立体,控制器,连续性,出版商和原始参考修订.
- `Basis` 编译、实际捕获启动、认证 `BodyGeometry`,当前组件对世界和基准视图。PG返回时没有显示世界的姿势。
- 被认可的经纪公司指定查询身份并快照手性生命周期/动作生成。三个年龄上限固定在新的跟踪/抓取上。40 ms, 原场景50 毫升和新鲜设备50ms.
- `Response` 与精确的源/ 基准密钥、 查询和本地母章相呼应, 并带有实际设备观测时间加固定挂载数据。 嵌入式挂载的原始 FramKey 仍为 Scene/ device 密钥, 绝不是 Tick/ Conforse 。
- 注册使用相同的主动的 Hand 母版上下文/ cookie/ 绑定图像, 现有的回调计数, 重新进入排除, 停/ 撤销/ 行动障碍和排水。 没有创建独立的注册员/ pin/ detach 或设备调用能力 。n 长期自有储存。

经纪人的C型0 校验器是固定的,是本地-私人的,而不是公共的POD许可。在其mutex下,经纪人认证(mostker exertates/snapshots)母和原始Scene协会,保留查询抓取和飞行寿命;它在实际C前释放了mutex。0/ source/ basis 验证和 PG。 它在发布前重新要求它检查确切的父/ 撤销/ query 状态。 不要调用一个完整的背景函数, 以在持有此 mutex 时重入 Broker 。 源验证器不能使用通用的 零待用调用 返回父代码的上游以拒绝它自己精确保留的查询,并且不能从全球范围减去回调数。父代码验证和本地源验证仍然不同;以后的每个写入仍然需要正常的完全闲置的父代码。

强制有两个区分。 承诺的手框并不包含 Rig 发布/ 绑定生成: 只有原住民可以验证这些设定是否符合其核心/ 缓存 。 PG 可以独立获取原始的手键, 五个对象身份和引用源 。共生; 重复额外的土著身份是相关, 而不是PG观察到它们的说法。 同样, PG 无法观察土著的私人动作生成。 它与经纪人章相呼应; 土著验证生成/ 障碍在前后。 PG 独立检查其 e xisting doors/shot/action deep 和它自己的客户端时代,它在其门下本地抓住这个时代,而不是信任一个请求的时代.

`Tracking.reference_revision` 实际为本地 `reference_snapshot` 修订版,与 Core.native_Reference_Revision 相同的源。它不同于原始的_scene.reference_generation,它是原始立体Gate生成,并与实际承诺的客户端框架匹配。从不比较这两个对等的计数器 y. PG 检查土著修正的形状/ echo 和 准确稳定的土著参考物与原始景物。土著验证当前实际的快照订正物,并在前后和每篇写作之前/之后完成土著参考物;请求不能认证其 自己原生修订。

## 查询-当地PG设备证人

在客户端现有的 Gate/ oc/ parent 寿命期下添加一个单独的同步操作。 它提取精确的注册密钥和身份字段作为祖先, 不返回或制造手框, 也不使用旧跟踪作为 新的跟踪资格。

固定的只读提供者进行新的游戏/持有的观察,既要读取原始的输入资产,又要准确的右附件,还要用独立的最终游戏/持有的观察 `same_game`, 保存生成并保存绑定。所有原始对象/登记/类/出/财产/附件的重复保留。新的观察结果包含元数据工作;注册/域/地方/控件生成在结尾处重新检查。设备开始/结束为 a 电话打来的时钟必须留在里面50 ms. 新的右侧右侧合格使用新的经认证的原始捆绑,包括可用/主动/重点/连续40 检查。

设备在第一次实际游戏读取和结束之前, 既要完成资产/ 附加/ 最终游戏, 也要完成日期/ 持有/ 域检查。 需要命令的正时数, 并且从开始算起年龄; 结束不能更新更早的证据 。 土著验证了两种情况 。子宫和嵌入式挂载查询ID,既指头/保留字段,准确的旧Scene FrameKey/寿命周期/身份和几何/校准。SourceKey是故意的回声子集;准确的完整源/参考/基础检查仍然有必要。 即便它匹配时。

重用真实的元数据原始, 仅在现有的读取器框架载体为偶然的情况下进行计数。 请不要调用不变 `copy_frame_on_gt`, `source_game_on_gt` 或外层 `attached_frame` 包装并假装旧框架是新鲜的。 不要递归输入客户端门。 PG 返回带查询的设备/ 资产/ 校准元数据, 没有世界摆放、 射击、 所有权授予或任意校准。 原住民使用证书转换新原始抓取 。d 现行基数,独立重建固定的兰瑟挂载。

## 原始源和写入路径

新模式需要自己的输入源分类/输入,而不是削弱的旧模式 `same_source` 上游。每一次硬身份、缓存/盘、参考、世界/演员/元数据/模型、生命周期/动作和旧路线所携带的结构/取消/停止检查都必须有明确的等同性。 新的来源资格,在检查前/检查后具有永久独立的硬故障传播和原始节点/存储。

每张实际的书写仍检查不可改变的新生/抓获40 ms,原始场景观测/出版物50 ms,查询-当地设备证据50 ms, 当前父/ 演员/ 元数据/ 模型/ 参考和 CAS/ Kernel 协议。 没有成功的守护程序被缓存为权限; 没有第二个写入器、 备用恢复接收器、 重复通知或 C 内的新数据包0 。在实际承诺之后,保留消费将保持单独的经认证的同一份租赁操作;它不能产生新的书面。

实际的预密母身份/ 修正/ 动作生成快照必须保持为在任何查询结果后, 包括未准备好的依赖性。 在查询中被撤销的父身份不能变成允许的 C 自由手写0。保持此快照与接受的设备证据分开。它保存在同步查询返回之后,因此本地源验证器不会运行一个正常的零调回上游,以对应它自己保留的调回。以后每一次写入检查规范 我又闲着的家长。

获得的非零 Tick 父饼干在选择或查询之前已经是一个硬身份依赖。当前/保留源检查要求精确的注册和土著化身仍然存在;消失或替换不能使用前置 查询自由手提前返回到允许的写入中。 此身份检查不需要零回调。 使用 cookie 获取的勾选0 保留独立的自由手拖曳路径。

即便在实际消耗了确切的内尔租赁之后,仅是设备陈旧时间就不得毒害原控制消耗。 保留的业务仍然检查硬源连续性和正常的闲置母体身份/修订/行动/障碍/债务;它确实如此。 设备不更新或允许新写。新写仍需要设备从开始到开始的年代50 ms. 这种区别由目标设备涵盖49→51 ms 主机边界,独立于新鲜的原始跟踪.

## 在一个游戏动作之前的最小证据

使用一个带有旧跟踪的混凝土边界箱 >50 ms, 原始场景 <50 毫秒, 真实的新勾选原始/ 抓取 <40 毫秒和新设备观测。1 新的输入方式只能通过相同的实际查询、本地背景和每个真实的写法,并记录原始来源的ID/time/basis和响应关联。查询和写之间必须拒绝,而不替换一个数据包;停止使用现有的回调排出。主机接合和部分退休证据保持清晰。

在检查共享头/ABI和此限定路径后, 建立单独列出的候选对象, 并运行不变的零售0.4 行动40/6 s门。保留先前的EN/EO故障和所有原始恢复检查。不要推断合同或成功的主机边界案件连续持有、瞄准/开火、全覆盖、运行相机或头盔完成。
