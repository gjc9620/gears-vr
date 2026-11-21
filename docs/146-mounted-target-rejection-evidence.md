# Mounted target rejection evidence

## 2026-10-19 evidence reconciliation on RuntimeRelease2

The earlier uncertainty below is partly superseded by actual mounted sample
`analysis/runtime/quest3-mounted-inventory-20261018.jsonl`, session35016.
Its consistent role sample identifies Turret_TroikaCabal2254762016, Driver
4765758336, reciprocal Controller/Pawn links, and Driver.DrivenVehicle equal
to that turret. Controller.ViewTarget, ServerViewTarget and
Camera.ViewTarget.Target all equal the turret. This supports the existing
target rule for that observation; do not widen it to Driver on the older
static-only uncertainty.

The later explicit user black-screen report is session5940, archived under
`analysis/runtime/mounted104-user-black-report-20261018/`. Its world log has
three initial registry-change rejections and one acquisition deadline with
last error `Mounted camera timestamp changed at bracket boundary`, three
attempts,57.9896ms. It did not commit a mounted rebind. Target relation is
unknown in those compact errors; the earlier sample cannot fill that gap.
There is no saved mounted GPU image or complete native capture-status for
this report, so it does not establish a downstream render failure.

Current Python includes the later deferred reflection and two-layer mounted
sampling changes; Release2 also includes the native Owner/wire4 correction.
No possessed-turret acceptance of this current combination was found. The
next actual test is ordinary play -> mount -> dismount, with the existing
read-only `analysis/runtime/mounted-transition-collector.py`, native/schema4
handoff, published tickets and XR receipts correlated. First establish whether
the mounted rebind succeeds; investigate mounted pixels if it does. Unmounted
scouts, registry unit tests, and checkpoint recovery do not replace this test.

2026-10-17. This is a failure-only evidence correction following the unresolved mounted black-screen feedback in [docs120](120-v1-headset-mounted-feedback.md). It does not change rendering or accept a new camera target, and has not been exercised while actually mounted.

Current Python and native mounted certificates both require `Camera.ViewTarget.Target` to equal the controlled Turret. The earlier scout in [docs76](76-mounted-streamed-level-certificates.md) contained no possessed turret and did not establish that relationship during mounted play. There is no demonstrated Python/native serialization mismatch here, and the target assumption remains unverified.

`MountedContextCertificate._capture()` rejected a different target before reaching the ordinary camera guard, so it lost the latter guard's discriminating target evidence. The existing `LiveRegistryAnchorSampler.guard()` already preserves `error.camera_observation`; the mounted early rejection now supplies that bounded field.

The failure receipt retains controlled Turret, Driver, controller and camera identities; target field/value and readable target/class identities; target-is-Driver comparison; timestamp/header expected and observed bytes; bounded mismatches and the first structured read error. `consistent_read` can become true only after fresh checks of all captured identities, pointer/layout/read bytes, registry headers, target and timestamp. Sequential equality is explicitly not an atomic snapshot or admission proof. Diagnostic failures retain the original plain rejection and cannot grant retry authority. The accepted target path retains its existing read schedule and checks.

Tests preserve acceptance of the existing Turret fixture and rejection of Driver/null/foreign targets. They cover changing target/identity/read errors, bounded failure observations and propagation through the outer guard as one non-retryable attempt. The bundled Python passed 126 mounted, anchor, cinematic, integration and world-rebind tests. No native DLL, default, existing ZIP, IDA database or global runtime setting changed.

The next actual mounted observation must distinguish a stable alternative target from role/registry changes and downstream GPU failure. A stable non-Turret target would identify this admission boundary for further investigation; successful schema3 rebind and accepted native anchor tickets would direct investigation downstream. Neither finding is currently established, and neither justifies admitting a Driver/foreign target automatically.

## 2026-10-19: current-code temporal review and bounded synthetic cost model

Correction to the compact session5940 timing: `world_rebind_monitor._guard_failure` copies `attempts[-1].elapsed_ms`. Therefore57.9896ms is the last strict-guard attempt, not total acquisition time. Three reported attempts do not demonstrate premature termination of the5-attempt/200ms guard policy. The historical failure precedes later two-layer/batched-read changes; it cannot establish the current implementation's failure cause.

Current source still encloses both inner registered role/name/link passes inside the camera timestamp interval. A private deterministic fixture measurement is saved as `analysis/runtime/mounted-current-bracket-cost-20261019.{py,json}` with current source hashes. Across40 cases (5/10/25/50/100us per synthetic read,60/90Hz,4phase offsets), the inner interval had103 reads, costing0.515/1.03/2.575/5.15/10.3ms in that model. All32 cases up to50us accepted within the existing budget; at100us,2/4 at60Hz and0/4 at90Hz accepted. Rejected acquisitions ended at215.2ms because the deadline is checked after an attempt completes. This is deliberately virtual timing with fixed topology/POV, not actual process latency, mounted acceptance or proof of a live defect.

Moving role validation outside the timestamp interval would preserve repeated topology checks but change their explicit same-frame temporal contract. No default change is justified by this historical report alone, and none was made. The next actual mounted sample must identify current rebind success and separate inner camera-window duration from whole guard duration before deciding whether this contract needs redesign. If rebind succeeds, inspect published mounted pixels rather than continuing Python admission changes. No game or runtime setting was changed by this review.

---

## 中文

# 挂载的目标拒绝证据

## 2026-10-19 运行时解析证据2

以下先前的不确定性部分被实际挂载样本所取代
`analysis/runtime/quest3-mounted-inventory-20261018.jsonl`届会35016.
它的一致角色样本可以识别Turret_TroikaCabal2254762016 驱动程序
4765758336,对等的控制器/光线链接,以及 DrivenVehicle 等效的驱动程序
给那个炮塔,控制器。
相机、录像、瞄准器都等于炮塔,这支持现有的
目标规则;不要将其扩大至对老年人的驱动
静态的不确定因素。

后一个明确的用户黑屏报告是会话5940,存档于
`analysis/runtime/mounted104-user-black-report-20261018/`其世界日志
3项初始变更登记否决和1项购置截止期
上次错误 `Mounted camera timestamp changed at bracket boundary`3个
attempts,57.9896 ms. 它没有执行挂载重置。目标关系是
错误的缩写; 先前的样本无法填补这一空白。
没有保存挂载的 GPU 图像或完整的本地抓取状态
因此,报告没有确定下游失败。

当前 Python 包括后期的延后反射和两层挂载
采样变化; 释放2 还包括本地所有人/电线4 correction.
未找到自有的炮塔接受当前组合。
下一个实际测试是普通播放 - > 挂载 - > 卸载, 并带有已有的
read-only `analysis/runtime/mounted-transition-collector.py`,本地/省4
交割、公布的票和XR收据是否相关。
重置成功; 如果挂载像素成功, 则调查挂载像素。 未挂载
侦察、登记单位测试和检查站恢复不能取代这项测试。

2026-10-17。这是在未解决的上载黑屏反馈后,仅更正了证据。 [docs120](120-v1-headset-mounted-feedback.md)。它不会改变渲染或接受新的相机目标,在实际安装时没有被执行。

当前 Python 和本地挂载证书都要求 `Camera.ViewTarget.Target` 以等值于控制下的涡轮。 [docs76](76-mounted-streamed-level-certificates.md) 包含没有附着的炮塔,并且在挂载游戏中也没有确定这种关系。这里没有显示 Python/ 本地序列错配,目标假设仍未验证。

`MountedContextCertificate._capture()` 在到达普通的摄像卫士之前,它拒绝了另一个目标,因此失去了后者的歧视目标证据。 `LiveRegistryAnchorSampler.guard()` 已保留 `error.camera_observation`; 挂载的早期拒绝现在提供 线段。

故障接收保留了受控的Turret、驱动程序、控制器和相机身份;目标字段/值和可读目标/类身份;目标-is-Driver比较;预计和观测到的时间戳/标题;边界不匹配和第一个结构 已读错误 。 `consistent_read` 只有在对所有捕获的身份、指针/放出/读取字节、登记页头、目标和时间戳进行新检查后,才能成为真实。序列平等显然不是原子快照或承认证明。诊断失败保留了原始的平原 r 弹出并不能授予重试权限。 接受的目标路径保留其现有的读取时间表和检查 。

测试保留了对当前Turret固定和拒绝驱动/核/外国目标的承诺,包括改变目标/识别/读误、限制故障观察和通过外卫传播,作为不可重复的尝试。毕达森通过 126 挂载、 主播、 电影、 集成和世界重置测试。 没有更改本地 DLL、 默认、 现有 ZIP、 IDA 数据库或全局运行时间设置 。

下一次实际安装观测必须区分稳定的替代目标与角色/注册变化和下游GPU故障。 一个稳定的非Turret目标将确定这一接收边界,供进一步调查;成功的计划3 重新封装和接受的本地主播票将指导下游调查。 目前还没有确定调查结果,也没有证明自动接纳司机/外国目标是合理的。

## 2026-10-19: 现行编码时间审查和限定的合成成本模型

压缩会议更正5940 timing: `world_rebind_monitor._guard_failure` copies `attempts[-1].elapsed_ms`因此,57.9896 ms是最后一次严格保护尝试,而不是总获取时间。5-attempt/200 ms 守护策略。 历史失败先于后期的两层/ 备注读数变化; 它无法确定当前执行失败的原因 。

当前源仍然将内挂角色/名称/链接都包含在相机时间戳间隔内。 `analysis/runtime/mounted-current-bracket-cost-20261019.{py,json}` 以当前源码散列。40 案件数量(5/10/25/50/100 我们每个合成读数,60/90Hz,4 相位抵消),内间隔有103 读取,成本计算0.515/1.03/2.575/5.15/10.3 模型里有几分音符,全部都用32 案件数量最多50 在现有预算中被接受;100us,2/4 at60 日内瓦0/4 at90 驳回的收购于215.2 ms 是因为尝试完成后会检查最后期限。 这是刻意虚拟时间, 使用固定的地形/ POV, 而不是实际的流程延迟性、 挂载接受或活性缺陷的证明 。

将角色验证移到时间戳间隔之外,可以保留重复的地形检查,但改变它们明确相同的时间范围。本历史报告没有提出任何默认更改的理由,也没有作出任何更改。下一个实际安装的 Sam ple 必须识别当前重新组合的成功, 并且将内置相机窗口时间与整个守卫时间分开, 然后再决定是否需要重新设计合同。 如果重新组合成功, 请检查已公布的挂载像素, 而不是继续 Python 录用 chan Ges. 本审查没有更改游戏或运行时间设置 。
