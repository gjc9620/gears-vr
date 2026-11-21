# 6 DPF Body Tick Inputs to Mark Associated Contract with Scene

Status: Specific candidate design not accepted as a writeable path. First achieves only real Tick-read sample to confirm whether new input actually exists; no change in existing body input, no change in any available access or writing.42、PG a 54d 592／0ec 1 The current record of failures and clean-up continues to freeze.

## Problems and choices

EJ's 17 In a sample of ready, consultr sync already entered at Compose 36.506–39.139 ms, old Scene already 34.413–36.273 ms;real Composequery only 0.926–1.154♪ I'm, query still ♪6.346–6.869 ms. The original path from Scene is published, the cache is maintained, and the actual outside MeshTick Tick Tick is used with the same unchanging Scene Core. The subsequent reading of the cache does not retrace or continue.

Move query to a more early position in Tick with the maximum approximate 0.32 ms observed TickCoopee interval; delayed release of the same package will not update data. Available evidence is not available earlier for a real Scene production point, but cannot be found in C 0 Change the Scene number or time. The PG finely parsed copy times are not supported by sufficient evidence to support the required wall. Select therefore the evaluation**Original Scene Device Qualification + Independent Real Tick Posture Input**, keep the only C 0 writer。

It's not old. `admission_budget()` . Continues: Old Path Requires Scene and its Old Tracking 40 ms; new path must be clearly defined**New tracking of actual consumption and current body benchmark**Pass.40 ms, original Scene identity/qualification continues to be complete 50 ms check. If it's still old, scene tv 40 This solution does not solve EJs as an absolute prerequisite. Old path defaults with original 40 They decided that all reservations could not be replaced by 50 I'm saying it's a success.

## Two distinct sources

| Source | Getting a location and content | Identity and time | Facts as to be available |
| --- | --- | --- | --- |
| Scene Mark Source | Original `try_submit_pair`／`rig_tracking_live::publish` The real story of the parent Hand has been submitted to Frame | Former Scene/publication/bindingGeneral/complete Framekey, former head/controller/sync/receive time, Native example and cookie | Current held/attachment/fixed gripwrist marked for each sync query; not new traching or write permission |
| Body Tick input | Existing, complete authentication real outer MeshTick, original match creation and homeer validation successful, original function and C 0 Read current Raw HMD/Hand Matching/Continuity once before starting | Separate real-life links, physical collection start and end of collection, original raw sequences and sync/receive, publicsher and reference review | New Tracking frozen in the same Tick; not disguised as Scene, not replacing old Core |
| Current Body Benchmark | Old C 0 `backend.capture` Current CTI/metadata/model, and actual-local netral | Independent, conservative physical Capture start time; original intestry/tick/metadata and geometry identity | By `neutral::move(stored,currentCTW)` and `prepare_cover_view` Get this real worldbasis; not to add the old Scene matrix to the new time |

Do not modify Rig V 1, the original coreSnapshot or PG request/response field meaning.0ec 1 Keeps the original bytes, continues to consume the former Framekey and Rig of Scene and implements them in their entirety.50 ms, two real Game independent observations, typed assets/enclosed/generation and final source review.1 Respone has no world target; it's only for the same C 0 Selects the fixed label.

## Tick Access Rules

1. Only available actual source MeshTick call point, not C 0. Follow the old World case, GT/caler/owner and Core Lat check to verify. Collect once; this failed Tick has no new input and cannot retry the swap for life.
2. Short `stereo_mutex` Try-lock, protect only the life of a given institution; do not hold a Rig/binding mutex, do not hold a lock without PG callback and wait for a lock. Refusal to re-enter and compete with stereo.
3. Only call open read-only `stereo_bridge.read`、`controllers.read_matching`／`publisher_identity`、`continuity_bridge.read_matching`. They copy the original package without updating the reference. Call is forbidden `stereo_gate.update`、`matching_input`、`select_continuity`, recenter rounds, opens/closingmapping or pushes existing Scene tracking ticket.
4. HMD, hands, and continuity must come from a precise match of Raw session/security/space/source and publicsher. Keep the original sync, received, predicting timescale, profile/input, producer/tracking epoch; these fields cannot be replaced with localnow.
5. The next and the next Native reference snapshots must be stable, corresponding to the original Scene actual reference review/epoch; stable reference position can be identified as coordinates, not using old view/world position as new data. Different steaum/reference, restarting process, back-sequence, and change of same sequence payload are rejected. Same series and same package can continue to be observed but not at the lower age.
6. There is no proof of the existence of a new actual sample. Read-only probe to record the original Scene package and the Tick new package, the original timescale and the results; it cannot be considered as input for updates by API.

## Same C.0 Inline Link Reply

Follow-up writeable paths should be used in separate Native `BodyTickTrackingInput`/ Group reply, at least retain:

- Practical tick, intestine, Compore, GT, Native example/game PID and start-up identity, world generation, formerly Scene identity.
- Former parent cookie/lifecycle/full Framekey, same query query id and actual resonese key; no rewriting reponse key or binding General.
- The two sources are the same: the current Avatar asset/reference header is separated from the model and the search returned by the webon consponent/asset, hold generation.
- This is a continuous process.

Must be in the same certified C 0 Scene still has the above data associated. The search is always synchronized, query-currelated, without retaining a license to re-use an eligibility or crossquery. Fixed tagging POD can be used as data upon return of Callback, but does not create new Hand/support/fire privileges or second writer. Parent revokes/activates/gun exchanges, etc., to render the connection ineffective and cannot continue using help to mark it.

## Old and new protected mappings

| Original protection | Old Path | New body path requirements |
| --- | --- | --- |
| Target Tracking Age of Access | Former Scene Core and its Tracking 40ms | Real Tick entered the original HMD/controller sync/receive and actually acquired time 40 ms, check before last use; cannot be renewed |
| World/camera basis fresh | The original Scene view comes with Core Zen.40ms | Use this actual C 0 The CTV and the actual-local neutral constructs the same base, real Capture starts time 40 ms;target calculation does not read old Scene base_view/world_head/world_grip |
| Scene/parent qualification | Same as Scene, original identity and life cycle | Full of Scene Core and query.50 ms, parent/world/actor/weapon/reference still checked; this is metadata dependent and does not claim old Scene 40♪ Ms by |
| Consistency with the same package | Old TickLatch is the same Core, version and original byte | Original Scene match as it is; another private Tick match reads only after real Tick has been created, all head/hands use the same new package and base/reference |
| Each writing of identity/time/metadata | Former ActualContext, Validate/exchange, CAS, Reread | All retained; new Tick original timescale and current body benchmark are checked before end-use/every writing 40 ms, old Scene-described dependence is still intact 50 ms. Current CTI/Reference/stop failure is still denied and does not slow down the current success result |
| Failed-page and Restore | The new episode is the real after-no debt. | Neither reliance on either failure anymore CAS; not C 0 Repacking or reopening. Follow the old Rollback/finally/revision/debt rule, actual Tick end clears two match-checks |

Read-only sources have been confirmed by the purchase of EK:32 All the actual Ticks are updated 2–4 Group package with a serial number and current C 0 Benchmark. Private Native 44 Independent type of access and same C as contract entered in table 0 Consumer, complete the orientation host and complete TU; default to retain old path with a new mode only enabled by visible compilation/INI. The actual gun-held check is not possible. A CoreSnapshot feed interface for change of time/controller cannot be constructed.

## Same animation consumption submitted for attitude

Every time before the bones are written,40／50 After writing, Kernel successfully notify and authenticate consumer, the remaining Arm alpha arbitration of Compose used a separate typeped interface to certify this already written Local; it can no longer prepare/committee, create CAS, refresh, reset, reset or extend. It must remain accurate bagend/Kernel case/stamp/GT, Canonical Scop address and Tick/Commove without finish, stop, exteption or protocol feature.

The consumption interface is not allowed to write back when it is applied. The application of the typed interface is not available; it is only used for mode unenabled. It is not valid immediately after the actual Finnish, and is wrong lead/thread/scope.

The original read-only hand monitor remained the same as the previous failure and sticky.Only when a named observation point expressly determines for the first time that source_age_only and retained is authorized can observer_source_expired be recorded without setting up a permanent hard-control-faault;Old TickLatch burned down as usual, was not read or restored and no longer sought to solve the old Tracking hypothesis.(a) The former Compute before/after continues to validate node/stack/headers, etc.;Their hard failure, entrapment, anomaly or stop is still permanently rejected and the recovery of fields or normal demixing in this Tick cannot be reauthorized.No blanket exemption based on failure of the gross particle sizes of the watch/metalata/prepare.

Native 46/EN already 28 Link in the help record 27 Full consume/after Local/FK matching and four role actualpacket application; sequence 5／query 30 Still failed and kept as it is. The door was kept open.40／6 s Not implemented. This only confirms that there are covered sub-consumer amendments, which do not constitute complete held follow or covert experience. Build, host boundary, original byte and running evidence is recorded in`docs/releases/0.0.4/PROGRESS.md`The blog also shows the number of candidates who have been selected for election.

## Invalidity and acceptance of borders

Collect missing, locked, non-matched hands or links to trust, change of reference, stop, identity/geometry/query failed, either source expired: this Tick does not generate new body writing permissions and uses the existing unwritten/original animated transfer path. The new model does not quietly return old traching to disguise success; normal no-hold eligibility and hard failure are still processed according to current classification.

Read-only running evidence completed: the same real Tick obtained HMD/Hands updated from original timescale and in group, public/reference/oldScene was clearly linked, no opening/off, recenter or ticket side effects were opened and all recovered.0 The new typeped goal/hands entrance and correlation check, running the original gun/fixed posture/small transfer movement, accepting actual held joint, consumer/original return and normal recovery. The source/host/ready count is not a substitute for target bone results; rays, collision/hitting, complete Cover, running stability and physical headline are still pending separately.

---

## 中文

# 6DoF 身体 Tick 输入与 Scene 标定关联合同

状态：具体候选设计，尚未作为可写入路径接受。第一实现只做真实 Tick 只读采样，确认新输入是否实际存在；不更换现有身体输入、不改变任何现有准入或写入。Native42、PG a54d592／0ec1、现有失败与清理记录继续冻结。

## 问题与选择

EJ 的17个 ready 样本中，进入 Compose 时原 controller sync 已有36.506–39.139ms，原 Scene 已34.413–36.273ms；真正 Compose→query 仅0.926–1.154ms，query 仍6.346–6.869ms。原路径从 Scene 发布、保留缓存、实际 outer MeshTick 的 TickLatch，一直使用同一不可变 Scene Core。后续读缓存不会重采样，也不会续时。

将查询挪到 Tick 的更早位置最多涉及约0.32ms的已观察 Tick→Compose 间隔；推迟发布相同包不会使数据更新。现有证据没有更早真实 Scene 生产点可用，不能在 C0 改 Scene 编号或时间。PG 小解析优化的 copy 次数证据不足以支持所需墙时。因此选择评估**原 Scene 装备标定资格 + 独立真实 Tick 姿态输入**，保留唯一 C0 writer。

这不是旧 `admission_budget()` 的原样延续：旧路径要求 Scene 及其旧 tracking 都≤40ms；新路径必须明确让**实际消费的新 tracking 和当前身体基准**通过40ms，原 Scene 的身份／标定资格继续完整50ms检查。若仍将旧 Scene≤40ms作为绝对前提，这个方案不能解决EJ。旧路径默认与原40ms判定全部保留，不能将它改成50ms后宣称成功。

## 两个明确区分的来源

| 来源 | 取得位置和内容 | 身份和时间 | 可提供的事实 |
| --- | --- | --- | --- |
| Scene 标定来源 | 原 `try_submit_pair`／`rig_tracking_live::publish` 及 parent Hand 的真实已提交 Frame | 原 Scene／publication／bindingGeneration／完整FrameKey、原head/controller/sync/receive时间、Native实例与cookie均不改 | 每次同步查询得到的当前held／attachment／固定grip→wrist标定；不是新tracking或写入许可 |
| Body Tick 输入 | 已有、完整认证的真实 outer MeshTick，原latch建立和owner校验成功后、原函数和C0开始前，一次性读取当前raw HMD／匹配双手／continuity | 单独的真实tick invocation、GT、Native/world/actor关联、实际采集起止时间、原raw序列和sync/receive、publisher及reference revision | 同一Tick内冻结的新tracking；不伪装为Scene，不替换原Core |
| 当前身体基准 | 原C0 `backend.capture` 的真实当前CTW／元数据／模型，和一次捕获的actor-local neutral | 独立、保守的实际capture开始时间；原entry／tick／元数据和geometry身份 | 由 `neutral::move(stored,currentCTW)` 与 `prepare_cover_view` 得到本次真实world basis；不是把旧Scene矩阵贴新时间 |

不修改 Rig V1、原 CoreSnapshot 或 PG request／response 的字段含义。PG0ec1可保持原字节，继续消费原Scene的原FrameKey与Rig，完整执行其原50ms、两次真实Game独立观察、typed资产／附着／generation和最终来源复核。V1 response没有world target；它只为同次C0选择固定标定。

## Tick 取得规则

1. 只在现有认证的实际 outer MeshTick 调用点取得，不在C0中补取。沿用原World lease、GT／caller／owner与Core latch验证。采集一次；失败则本Tick无新输入，不能重试换包来续命。
2. 短暂 `stereo_mutex` try-lock，仅保护既有bridge和参考系的生存期；不持有rig／binding mutex，不跨PG callback持锁，不等待锁。拒绝已有stereo重入和争用。
3. 只调用已经打开的只读 `stereo_bridge.read`、`controllers.read_matching`／`publisher_identity`、`continuity_bridge.read_matching`。它们复制producer原包且不更新renderer reference。禁止调用 `stereo_gate.update`、`matching_input`、`select_continuity`、recenter轮询、打开／关闭mapping或推进既有Scene tracking ticket。
4. HMD、双手、continuity必须来自精确匹配的raw session／sequence／space／source及publisher。保留原sync、receive、预测时标、profile/input、producer/tracking epoch；这些字段不能用本地now替换。采集起止时间是另列观察时间。
5. 前后Native reference快照必须稳定、与原Scene实际reference revision／epoch对应；稳定的reference pose可以作为坐标标定，不得把旧view/world pose当新数据。不同stream／reference、进程重启、回退序列、同序列payload变化均拒绝。相同序列且相同原包可以继续观察，但原年龄不得减少。
6. 实际新样本是否存在尚无运行证据。先以有界只读probe记录原Scene包与Tick新包、原时标及结果；不得凭API可用就认为输入更新。

## 同一C0内的关联回执

后继可写路径应使用单独的Native `BodyTickTrackingInput`／组合回执，至少保留：

- 实际tick、entry、Compose、GT、Native实例／game PID及启动身份、world generation，原Scene身份另存。
- 原parent cookie／lifecycle／完整FrameKey、同次查询query id和实际response key；不改写response key或bindingGeneration。
- 两源相同的world／controller／pawn／mesh／weapon；当前Avatar asset／reference header与模型，以及查询返回的weapon component／asset、hold generation分开列明。
- geometry签名、实际scale／units、basis／revision、reference／producer／tracking连续性。BodyTick自身身份不能借Scene lease冒充。

必须在同一个已认证的C0关联上述数据。Scene查询仍每次同步、query-correlated，不保存eligibility或跨query复用许可。固定标定POD在callback返回后可作为数据，但不产生新的Hand／support／fire权限或第二writer。Parent撤销／动作失效／换枪等使关联失效，不能继续使用held标定。

## 新旧保护逐项映射

| 原保护 | 旧路径 | 新身体路径要求 |
| --- | --- | --- |
| 目标tracking准入年龄 | 原Scene Core及其原tracking≤40ms | 真实Tick输入的原HMD／controller sync／receive和实际取得时间≤40ms，末次使用前再核对；不能给原包续时 |
| 世界／camera basis新鲜性 | 原Scene view来源随同Core≤40ms | 用本次实际C0 capture的CTW和actor-local neutral构造同一base，真实capture开始时间≤40ms；目标计算不读旧Scene base_view/world_head/world_grip |
| Scene／parent资格 | 原同Scene、原身份及生命周期 | 原Scene Core和query完整50ms，parent／world／actor／weapon／reference关联仍检查；这是metadata依赖，不宣称旧Scene40ms通过 |
| 同包一致性 | 原TickLatch同一Core、版本和原字节 | 原Scene latch原样保留；另一个私有Tick latch在真实Tick建立后只读，所有head／hands使用同一新包和同一base/reference |
| 每写身份／时间／元数据 | 原ActualContext、validate／exchange、CAS、回读 | 全部保留；新Tick原时标和当前身体基准在最终使用／每写前仍检查≤40ms，原Scene标定依赖仍完整≤50ms。当前CTW／reference／stop等失效仍拒绝，不缓存current成功结果 |
| source-age失败与恢复 | 原burn、normal end、零债务后真实新episode | 两依赖任一失败均不再CAS；不在C0换包或重开。沿用原rollback／finally／retirement／debt规则，实际Tick end清两份latch |

只读来源已由购买版EK确认：32次实际Tick均取得更新2–4个序号的成组原包和当前C0基准。私有Native44已按表中合同接入独立类型及同一C0消费者，完成定向Host与完整TU构建；默认仍保留旧路径，新模式仅通过显式编译／INI启用，实际持枪检查另记。不能构造一份改时间／controller的CoreSnapshot喂旧接口。

## 已提交姿态的同次动画消费

每次骨骼写入前的40／50ms与同次写入资格不变。写入完成、原Kernel成功notify并验证consumer之后，原Compose余下的arm alpha仲裁使用单独typed retained接口认证这份已经写入的Local；它不能再prepare／commit、产生CAS、刷新源、重notify或延长lease。必须仍是精确backend／Kernel lease／stamp／GT、canonical Scope地址和Tick／Compose，且未finish、stop、exception或protocol fault。

该消费接口每次独立检查actor／当前metadata／Marcus签名、原Core与reference／ticket的硬连续性、typed latch原字节和Parent生命周期。仅elapsed age不能撤销同次已提交姿态的消费，也不能授权下一次写入。适用的typed接口若denied，不得回退旧owner_current；not_applicable仅用于模式未启用。实际finish后、wrong lease／thread／scope等均立即失效。

原只读手部观察器仍保留原失败和sticky。只有命名观察点首次明确判定source_age_only且retained认证通过，才可记observer_source_expired而不设置永久hard-control-fault；原TickLatch照旧烧毁，随后不再重读或恢复，也不再求解旧tracking的假设plan。原Compute before／after继续验证node／stack／headers等；它们的硬失败、嵌套、异常或stop仍永久拒绝，本Tick内恢复字段或正常解嵌不能重新授权。不得按粗粒度latch／metadata／prepare失败一概豁免。

Native46／EN已在28条held记录中关联27条完整consumer／after Local／FK匹配和四角色实际packet applied；sequence5／query30仍失败并原样保留。原连续held门40／6s未过，后继移手未执行，因此这只确认有覆盖的同次消费修正，不构成完整held跟随或掩体验收。构建、Host边界、原字节和运行证据记录于`docs/releases/0.0.4/PROGRESS.md`及ignored候选目录。

## 失效与接受边界

采集缺失、锁争用、无匹配双手或continuity、reference改变、stop、身份／geometry／query关联失败、任一来源过期：本Tick不生成新身体写入许可，使用现有无写入／原动画让位路径。新模式下不悄悄退回旧tracking来伪装成功；普通无held资格与硬故障仍按现有分类处理。

先完成只读运行证据：同一真实Tick取得了原时标更新且成组的HMD／双手，publisher／reference／oldScene关联明确，没有打开/关闭、recenter或ticket副作用，全部正常恢复。之后才实现同一C0新typed goal／hands入口和关联校验，跑原取枪／固定姿态／小移转腕动作，接受实际held joint、consumer／原返回与正常恢复。Source／Host／ready计数均不能替代目标骨骼结果；射线、碰撞／受击、完整Cover、跑步稳定及实体头显仍分别待验收。
