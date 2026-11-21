# 6DoF held main-hand mount query — Native/PG contract

Status: implementation contract for the next private candidate. Nothing in this
document enables a default launcher or changes Rig V1, Hand V1, aiming, shooting,
or the original wrist → weapon bone animation increments. Native owns the shared
C0 consumer and broker; the weapon chat owns the PG callback.

## Purpose and current evidence

Native39/B17 accepts a fixed dual-wrist point through the original Compose return.
B18 accepts the bounded fixed-head hand trajectory. B19 accepts eligibility
fallthrough to the original controllers and reachable-goal reacquisition; its
asynchronous intermediate Limb read remains explicitly separate from a final
handoff return fence. These are development checks, not full cover/headset
acceptance. The next target is consuming the actual held Lancer calibration in
that same C0 transaction, with free-hand calibration on an ordinary unavailable
query.

The existing PG-local `nativeequipment_main_mount::prepare_retained` from weapon
commit `b58524c` is the sender implementation. Its data agreement remains intact:
the committed Hand frame must name the retained Rig Scene, not a C0 counter, and
all Native/actor/stream/input/reference/binding identities must agree.

## Registration and lifetime

- Add one supplemental export, `GearsVrAttachWeaponMainMountProviderV1`, following
  the existing optional Call/Action attachment pattern on `weapon_hand::Broker`.
  Use the current parent hand cookie and Native incarnation. No new cookie,
  independent pin, second broker, or mutable Frame extension is introduced.
- Registration supplies exact request/response sizes, one prepare callback and
  the same context as the parent. The executable callback must belong to the
  already-pinned parent PG image. An identical retry is idempotent; conflicting
  attachment is busy. Wrong/retired cookies do not disturb a newer registration.
- The provider attaches once per parent lifetime. Parent unregister/Native stop
  disables admission and drains it together with existing callbacks/transactions.
  There is no separate detach or permission to free PG context early.
- Every query takes the parent's existing callback count/admission before calling
  PG, copies the registration while admitted, releases the admission mutex, then
  invokes PG under the ordinary engine-free callback/reentry guard. End accounting
  runs exactly once on return or fault. Stop during a query cannot unpin or reset
  its callback/context; worker collection still requires exact-cookie quiescence.
- Admission also excludes existing callback/restore, Call/Action/external scopes,
  transactions/invalidation/debt and a poisoned ledger. This C0 query additionally
  requires Scene/stereo depths zero; it does not relax other callback contexts.
  It maintains both `callbacks_` and `normal_calls_` and rechecks cookie, revision,
  operation epoch, revocation generation and the committed Frame after return.
  A nonempty support/left-hand lease or `Broker::authorized()` is not required.
- PG additionally enters its local active-callback lifetime guard, checks running,
  exact cookie/incarnation and GT before touching local equipment state, and
  closes that guard on SEH too. Worker LifecycleScope/SRW is not a GT substitute.
  No wait, join, engine call, setter, asynchronous retention or source publication
  is permitted in the callback. Existing copied-Frame/current-Game accessors and
  bounded read-only asset/attachment checks are allowed.

## Input

The new header will be `tools/vr-common/weapon_main_mount_provider.h`. It contains
fixed-width, standard-layout POD and explicit x64 layout tests; PG-local structs
are mapped field-by-field, never passed as an accidental ABI.

Frozen header SHA256:
`3f20163b1b65b7a74811958d4547b5c43605a6708d063a4049e4935deaa2f2cc`.
Measured MSVC x64 layout: Rig V1 Frame=1824, BodyGeometry=112, Request=1968
(body offset=1856), Response=504, Registration=48. The affected wire test passes
28 checks. These layout tests prove data shape, not runtime admission/lifetime.

`Request` contains its size/version, an immutable retained Rig V1 Frame and actual
Native C0 body data: mesh identity, the complete Marcus geometry signature,
uniform model scale, units, nonzero body revision, basis=2 and basis revision=2.
The maximum source age is exactly 50 ms. The parent cookie/NativeKey comes from
the actual admitted parent; Native projects the same immutable Core with
`project_for_authenticated_registration`. Projection alone is not admission.

The initial body revision is the actual retained Core binding generation and is
echoed for this query only; full current mesh/signature/scale/basis checks remain
independent. This number is not a Hand Frame, C0-to-Scene conversion or writer
token. All source timestamps and the original Scene invocation remain unchanged.

The admitted broker also supplies a unique query id and the actual committed
parent Hand lifecycle. Ready echoes that id and must match that committed Hand
key. Neither value is synthesized from a C0/Kernel counter.

Native prepares this request only after `body::plan` succeeds and before
`hands::compose`, from its current authenticated context and model. It rechecks
that context/source after the callback and retains the original 40 ms admission
budget and 50 ms per-write guards. Query time never renews a source.

## Output and PG mapping

The prepare callback returns a bounded status: ready, inactive, no_weapon,
unheld, animation, busy, unsupported_asset, not_attached, source_mismatch,
body_mismatch, unsupported_geometry. A non-ready result has an empty payload.
Header size/version stay valid; reserved fields are zero. Invalid status, SEH,
request mutation, malformed ready payload or forbidden reentry is a callback
fault handled by the existing parent invalidation/drain path, not a free-hand
ownership grant.

`Response` contains the actual committed Hand FrameKey; world/controller/pawn/
mesh/weapon/component/asset identities; hold generation; provider/body/basis
revisions; geometry/scale/units and the explicit known right grip-to-wrist
translation/quaternion. No pointers, target world pose, aim ray, muzzle, support
mount or writer lease are returned.

PG maps the retained Frame and Native body data into the existing local
`BodyContext`, calls `prepare_retained`, and maps the ready Candidate into the
response. It retains the actual held Lancer asset/right attachment checks,
hold-generation checks, before/after source binding and final source-age checks.
It must not synthesize a Hand Frame, force busy/held false, assume geometry, alter
the frame key or carry a previous Candidate into a later query.

## Native consumption

Native accepts ready only when the returned key/cookie/incarnation, Scene,
identities and source agreement match the exact retained input, and the returned
geometry/scale/units/revisions match its actual C0. It independently reconstructs
the fixed `marcus115-lancer-held-grip-v1` profile and compares the full right
calibration, rather than accepting arbitrary callback offsets.

The held profile is a separately identified internal mount choice. It does not
relabel or overwrite the immutable Core's stock authored mount. The pure target
adapter explicitly validates either the existing stock mount or the verified
held-Lancer profile. Native supplies the latter only for this ready query.

The one shared C0 output/Kernel remains the only bone writer. Left-hand targets
retain their current policy; 35→51→52 propagates through original FK/increments.
An unavailable/stale/unheld/busy/unsupported query uses the existing stock mount;
it cannot retarget a weapon's authoritative fire ray or claim muzzle alignment.
Malformed responses grant nothing. No result is cached beyond the exact C0
query/transaction. All large query/work buffers are persistent GT-owned storage,
not added to the assembly hook stack.

## Targeted validation

Before gameplay: fixed ABI/layout; exact same-cookie/module/context attachment;
idempotent/wrong-cookie/conflicting cases; callback stop/unregister race and
worker-only unpin; nested/SEH/malformed response; normal non-ready stock fallback;
actual profile/geometry/key/source rejection; unchanged-source/sole-writer target
composition and no-CAS age behavior. Test only changed scopes, not full unrelated
renderer or PG history.

First game candidate uses 0.4/background/behind/observe-only/mute, current save
and audio protection, and R2 mode OFF. Observe one actual Lancer RGrip hold,
controlled simulator orientation/position changes, then unhold. Physical
controller background input remains disabled. Correlate actual
PG ready key/held asset with the Native mount choice and C0 wrist result. Keep
normal cleanup and pending final render/headset/aim/cover checks explicit.
The owner starts any later physical-controller/headset check manually; it is a
separate acceptance step.

---

## 中文

# 6 渔业部掌握主手挂载查询——土著/PG合同

现况:下一个私人候选人的执行合同。
文档启用默认的启动器或更改 Rig V1 手掌五号1 瞄准 射击
或原始手腕 ~ 武器骨动能增量。
C0 消费者和经纪人;武器聊天拥有PG调回.

## 目的和现有证据

Native39/B17 通过原编译返回接受固定的双面折角点。
B18 接受有界固定头的手轨。19 接受资格
由原控制员完成,可达目标再购置;
读取的同步中间体Limb与最终体明确分开
交接返回围栏。这些是开发检查,而不是全盖/头套
下一个目标是消耗实际持有的Lancer校准
同一C级0 交易,在普通不可用时使用自由手校准
query.

现有的本地 PG `nativeequipment_main_mount::prepare_retained` 从武器
commit `b58524c` 数据协议保持不变:
承诺的手框必须指定保留 Rig Scene 的名称,而不是 C0 计数器,以及
所有土著/行为者/流/输入/参考/约束性身份必须商定。

## 登记和寿命

- 增加一个补充导出, `GearsVrAttachWeaponMainMountProviderV1`,后面
  现有的可选调用/ 行动附件模式 `weapon_hand::Broker`.
  使用当前父手饼干和原生化。没有新饼干 。
  引入独立 pin, 第二代理, 或可变框架扩展 。
- 登记用品 确切的要求/答复规模,一个准备回调和
  上下文与父的上下文相同。可执行回调必须属于
  已绑定的母 pG 图像。 相同的重试是 idepotent; 冲突
  附件繁忙。错误/退休的饼干不会扰乱新的注册。
- 提供方每生父母一次附加。父母未注册/停止
  禁用录用,并排出它与现有的回调/交易。
  没有单独的解析或权限可以提早释放PG上下文.
- 每个查询在拨打前都会使用父的已发回/ 允许的计数
  PG,在录用时复制登记,释放录用mutex,然后
  在普通引擎无回调/重返守护下引用 PG。 结束核算
  返回或错误时运行一次。在查询中停止不能撤消或重设
  其回调/语录; 工人收藏仍需要准确的cookie quiescence.
- 招生还排除了现有的回调/恢复、呼吁/行动/外部范围,
  交易/无效/债务和有毒分类账。0 附加查询
  需要 Scene/stereo 深度为 0; 它不会放松其他回调上下文 。
  两者都维持 `callbacks_` and `normal_calls_` 并重新检查饼干, 修订版,
  操作时段、撤销生成和返回后承诺的框架。
  非空支持/左手租赁或 `Broker::authorized()` 不需要这样。
- PG还进入了本地的主动召回终身警卫 检查运行
  精确饼干/化身和GT在接触当地设备状态之前,以及
  关闭安全防护系统。工人寿命周期/SRW不是GT的替代。
  不等待、 加入、 引擎调用、 设置、 同步保留或源发布
  允许在回调中。
  允许进行有限度的只读资产/附加检查。

## Input

新标题是 `tools/vr-common/weapon_main_mount_provider.h`中包含
固定宽度、标准铺设POD和明确X64 版式测试; PG- 本地结构
被逐个地绘制,从未作为意外ABI通过.

冻头 SHA256:
`3f20163b1b65b7a74811958d4547b5c43605a6708d063a4049e4935deaa2f2cc`.
测量的MSVC x64 版式: Rig V1 Frame=1824, 体格测量=112 请求=1968
单位:千美元1856),回应=504 登记=48。受影响的电线测试通行证
28 检查。这些布局测试证明数据形状,而不是运行时间录入/使用时间。

`Request` 包含其大小/ 版本, 不可改变的保留 Rig V1 框架和实际
原产地 C0 身体数据:网状特征,完整的马库斯几何特征,
统一型号尺、单位、非零体修订,基数=2 和基准订正=2.
最大源年龄是准确的 50 ms. 母饼干/ NativeKey 来源于
实际被接纳的家长; 土著项目与不可改变的核心项目相同;
`project_for_authenticated_registration`单是预测就不算录取。

初始机构修订是实际保留的核心约束生成,并且
只为此查询提供回声; 完整的当前网格/ 签名/ 比例/ 基准检查仍然保留
独立。此数字不是一个手框, C0- 切换到场景或写入器
符号。所有源时间戳和原始景物引用保持不变。

被录取的经纪人还提供了独特的查询身份和实际承诺
手的生命周期。准备回声到 ID , 并且必须匹配承诺的手
键。两个值均不从 C 合成0 柯纳计数器

本地编写此请求时 `body::plan` 成功和之前
`hands::compose`,从当前认证的上下文和模型中。它重新检查
后调用上下文/源并保留原 40 ms 录取
预算 50 毫秒/ 每字的守护。 查询时间不会更新源 。

## 产出和PG绘图

准备回调返回一个有限制的状态: 准备、 不活动、 不武器、
未持有, 动画, 繁忙, 不支持的_ 资产, 不支持的, 源码匹配, Name
对象_ mismatch, 不支持_ 几何。 未准备好的结果有空有效载荷 。
标题大小/ 版本保持有效; 保留字段为零。 无效状态, SEH,
请求突变、不完善的准备有效载荷或禁止重返是召回
由当前父路径无效/ 排水路径处理的故障, 而非自由手
所有权赠款。

`Response` 包含实际承诺的手框Key; 世界/控制器/当/
网格/武器/部件/资产身份;持有生成;提供商/身体/基底
修订;几何/尺度/单位和明确的已知右抓拳
翻译/方位。没有指针、目标世界的姿势、瞄准射线、口罩、支持
租借或租借的纸币被退还。

PG 将保留的框架和原生体数据映射到现有的本地
`BodyContext`,电话 `prepare_retained`,并将准备中的候选人映射到
保留实际持有的Lancer资产/右附件检查,
进行生成检查,在源头约束和最终源头年龄检查之前/之后进行。
它不能合成手框 强迫忙碌/持有假, 假设几何, 更改
框键或将前一个候选人带入后期查询。

## 土著消费

原生接受 只有在返回的钥匙/cookie/化身,Scene,
身份和源协议匹配所保留的确切输入,返回
几何/尺度/单位/订正与实际C相符0。它独立重建
固定的 `marcus115-lancer-held-grip-v1` 配置和比较全右
校准,而不是接受任意回调抵消.

所持有的配置文件是单独列出的内部挂载选择。 它不是
重贴标签或覆盖不可改变的 Core 的股票创建挂载。 纯目标
适配器明确验证现有库存挂载或已核实的
保存的 Lancer 简介。 本地仅为此即时查询提供 。

分享C的那个0 输出/ Kernel 仍然是唯一的骨质写入器。左手目标
保留其现行政策; 35→51→52 通过原始FK/插件传播。
无法使用的/出售/未持有/忙/不支持的查询使用现有库存挂载;
无法重新瞄准武器权威的射线 也无法声称枪口对齐
错误的回复不会给出任何结果。 任何结果都无法缓存到精确的 C 以外0
查询/交易。所有大型查询/工作缓冲器都是持续的 GT 拥有的存储,
未添加到装配钩堆中 。

## 目标验证

游戏播放前:固定的ABI/layout;精确的同型-cookie/module/context附件;
专有/错误-曲奇/冲突案件;召回停止/未登记种族和
仅限工人未接线;巢/SEH/成型反应;正常的未准备库存回落;
实际配置/几何/键/源拒绝;无变化的源/单写目标
组成和无 CAS 年龄行为。测试只改变范围,而不是完全无关
渲染器或PG历史.

第一次游戏候选游戏使用 0.4/背景/后方/观察-只/默特,当前保存
和声波保护,以及R2 模式关闭。
控制模拟器方向/位置变化,然后取消控件。
控制器背景输入仍然禁用。 校正实际
PG 备用密钥/持有的资产,有本地挂载选择和C0 手腕结果。 保留
正常清理和待最后制作/头套/头套/盖盖检查明确无误。
主人在以后任何物理控制器/头套检查中手动启动;这是
单独的接受步骤。
