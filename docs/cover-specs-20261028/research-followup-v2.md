# Second round offline preparation: original call, submission sequence and firing

2026-10-28. Continue the first round of packages, which are still not connected to games, and do not change the production of C++, switches, builders or starters. This round adds call border checks, main line weapons consumers ' positions and enforceable specifications reference checks. The new source snapshots and original scripts are only stored in `analysis/cover-prep-20261028/revision2/`；[Supplementary Source Index](revision2-code-locations.json)Records 10- A document,19 The main line is still being modified and the following is repositioned by symbol, without returning a snapshot.

## 1 The original version of the call evidence is more complete than the first round

[History](../releases/0.0.4/PROGRESS.md)"Practical change in bunker attitude connected to bone search" log: T test 64 Two of the complete call groups were observed in the same caller-local caller caller `CT2/CA0/CD0 → CT2/CA7/CD3` ; and then action/direction matter retains a new value in the same caller/locals. This is stronger than two previous stable observation windows.

Current round of inspections `cover_call_decode.py::_row/analyze`, confirm that the decoding device check is with input/arm/world/thread/caler/locals/parent, call sequence, original byte window and actual parameter token. The parameter reference order forupdater is `DeltaTime, PawnCT, PawnCA, PawnCD, bNewLeftSided`,setter quotes `PawnCA`、`PawnCD`;token shape is EX_LocalVariable+ Authenticated Portty Reference, last EndParm. It does not consider RAX or Ressult as out of address.

So, U-1 Zoom in to:**Re-authens the specific field/call point of the current two state fields and add permission to the restricted writing and injury branch**, no longer need to re-samp the "Are caller-local changes" as a completely unknown problem. The old T original configuration/record of the directory examined in this round is not available and is H reported evidence; it cannot replace current writing certificates with historical values.

Preferably write point further clarified as original `UpdatePlayerPosture` Synchronization of borders, original after normal return `PlayerMove` Not yet read local CA/CD. Re-use existing dispatcher structure `cover_call_live::CallScope::returned` is a time frame; the function is still a read-only observer and cannot directly open write permissions.

Adds a new writing that proves the current instance and caller to PlayerMove, True Return Success, Script/ Fields/locals remain unchanged and updater has finished writing back. Only CA/CD is suitable, without changing CT, moving fields along walls or all locals. Two-by-tee updates are a restricted matter: Save original value before writing, previous caller resumes pre-readback; intermediate failure restores its own modification only if it still matches the original. Unable to recover eligibility for processing with existing failure processes and not be able to return to "success". The bond pointer cannot be saved for next Tick to write.

## 2 Continuous access will also affect the original fire waiting.

Add static call chain:

`WarPawn.DoingAnimationTransition` → `WarPawn.ShouldDelayFiring` → `WarWeapon.ShouldRefire`and `WarPawn.Tick` Delayed firing/ping-fire consumption;`Weap_Bow_base.CanReleaseFire` Animated animations are also temporarily resumed for inspection under blinding conditions.

There are two specific risks:

- There's a bunker. CA is...0 Humans control and not.360 When I was aging,`ShouldDelayFiring` You will return to the waiting period. Revert all bunkers to the same thing.0 The government has also been able to prevent the shooting of the originals.
- `DoingAnimationTransition` Check that the CoverAction is inconsistent with LastCoverActionAnim, or `bDoingCoverActionAnimTransition`. If successive half-protaching is always treated as a transition, the original firing chain may remain undisturbed even if the key and gun point are correct.

F 19 Adds to the question sheet. Implement an example of matching action with attitude consumption, stabilizing half-proximity cannot permanently take over “uncompleted transition”. Returning value appropriate is only handled when current VR Pawn transition conditions, original call and identical posture are certified; mirrors, guns replacement, filling, real animation transition, original no-firing and special actions remain. `bNoAnimDelayFiring=true` Or put the whole thing in. `ShouldDelayFiring` Force false. The wrinkled bow continues to use the cancellation/release chain of the weapon pack, without early release of arrows due to bunker suitability.

## 3 Input confirmation cannot be discarded for body surrender that has already occurred

The first round has distinguished between input ticket and intertick immutable inent. This round is completed with the most easily missed sequence:

| Order | Determination of conduct |
| --- | --- |
| It happened without handing over. | Cancel pending indent; late queries or submissions cannot move body |
| The body has been submitted. | Sync the actual position of the group ledger, then release and cancel the remaining indent; keep the newly submitted location |
| The new guy's sample replaced the old candidate. The old query came back. | Int id/from-version not matched, old query denied |
| Old confirmation submitted later | Cannot roll back to new position; reject old confirmed overwrite, as is currently the LEDger |
| Pressed after shearing | Rebuild starting point with this hand position and actual atfset while synchronizing receipt, next counter-point immediately |

**Each round of processing capture, move, release and synchronized completion prior to failure.** The original body consumer successfully completed is the point of entry for movement; input back just confirms facts, not a second move permit.`CoverCommitReceipt` Adds physical cutting, locking axes, capturing the identity of hand samples for reset starting point. Rendering body-only publications; entering a later kernel confirmation should not allow the image to retreat.

The suspension/release/cancel can only cancel an unsuspected movement and not allow the deletion of a submission that has occurred in history. If the submitted location has subsequently crashed, it goes back to processing instead of placing the logical book zero.

## 4 Specific location of the current gun-point consumer

The main line at the time of investigation has been filled with the following entry points, and cover has achieved that evidence should be added to these interfaces without a separate set of shooting coverage:

| Location | Current responsibilities | The bunker needs additional constraints. |
| --- | --- | --- |
| `weapon_unified_fire.inl::prepare_current` | PostPlayerTick secure Muzzle sockt, gun point and current hand/arms source, maintaining original receipt time stamp | Prepares for gunpoint and block request associated with current cover group/basis; older post-point guns cannot be re-labeled as new |
| `MuzzleSample`、`source_matches_shot` | Verify instance, hold, input space/reference in order of origin, allowing normal PostTick sources to be available earlier than the next publicization | cover requires explicit reference to a certificate in the form of a document, which is not clear; original monotonous and freshness cannot by itself prove cross-wall security |
| `typed_result` | Identify aim, trace-origin, Pushed-origin, philsical-origin returns values in restricted original calls | Four starting/direction paths must use consistent cover and blocking decision-making, not missing pushed-forward starting points |
| `normal_return/store_result` | Review after normal return typed schema/shot, write back slots in terms of original values and process incomplete writing | Add the currentcover version check; cannot add Trace, engine getter, lock waiting or enter the client file |
| `weapon_native_lancer_follow.inl` Following/guns | The main line is following the plan with a perfect physical gun model. | Linked to the same cover body transport; model, socket and shooting source cannot be moved once each |
| `weapon_call_live::invoke` | The only original call and notification ending of Native CallFunction | Continue to keep; do not install a global dispatcher for the bunker |

`AppliedEvidence` The current provision of diagnosis, such as shot, input, origin, aim is not per se a bunker occupancy or hit certificate.`full_body_c0_native_backend.h` Clearly, the existing subject does not provide a full program phsics set;`consumer_verified` Nor can it be interpreted as “validized in a true shot”. These restrictions have been reversed.2/U 3。

Add a new bunker check to limit the cover branch only, not change main line M during this round 4 Behaviour. When the current certificate is not available in the preparation phase of future operations, no pre-presence will be sent back to the original shot; stop the new launch intent and continue reading the real release, without placing the old button in an infinite queue of ghost shots later. The bullet that hits the wall legally can still be fired in its original meaning; blocking cannot be equal to a ban on the trigger.

## 5 Specification vectors that can be used directly

[contract-vectors.json](contract-vectors.json) Provision 28 Synthetic Time series cases,3(a) An example of a place transfer;[offline-contract-checker.py](offline-contract-checker.py) is an independent Python reference model.

```powershell
py -3 -X utf8 offline-contract-checker.py --output contract-check-results.json
```

Current round [Check results](contract-check-results.json)：28 Time series cases,78 A statement and 3 The list of cases for each place is fully adopted. Covering the waiting to release, submitting before input confirmation, rejecting old versions, re-arrangement, recapturing, failure recovery and non-consumer versions are all available; C++ can be achieved using the same input/predict as test material. The original consumer also requires that the "uncertainty/restoration" status be maintained, without which the memory cannot be assumed to have rolled back because it has not been published successfully; only a new occupancy certificate does not discharge this obligation.

The reference model expresses only the sequence and version rules, and the legal edge/physical snapshot/empty endpoint inixture is a manual assumption. It does not implement the existing C++, does not certify Trace, skeleton, original injury, original fire or headline; production types cannot accept these fixture bool as certificates. The three migrations only check vectors once, without verifying matrix conversion or real head rotation. Do not write those numbers into game acceptance passes.

---

## 中文

# 第二轮离线准备：原调用、提交顺序与开火衔接

2026-10-28。继续第一轮实施包，仍不接入游戏，不改生产 C++、开关、构建或启动器。本轮增加了调用边界核对、主线武器消费者位置和可执行的规格参考检查。新增源快照与原脚本仅保存在 `analysis/cover-prep-20261028/revision2/`；[补充源码索引](revision2-code-locations.json)记录10个文件、19个位置的调查时身份。主线仍在修改，后续按符号重新定位，不回写快照。

## 1. 原版调用证据比第一轮整理得更完整

[历史 PROGRESS](../releases/0.0.4/PROGRESS.md)“掩体姿态实际变化与骨骼查询接通”记录：T 试验64个完整调用组中有两组，在同一次 updater 调用的 caller-local 中观察到 `CT2/CA0/CD0 → CT2/CA7/CD3` 及反向恢复；随后 action/direction setter 在同一 caller/locals 中保留新值。这比此前两个稳定观察窗更强。

本轮检查 `cover_call_decode.py::_row/analyze`，确认解码器检查同 invocation/arm/world/thread/caller/locals/parent、调用先后、原字节窗口和实际参数 token。updater 的参数引用顺序是 `DeltaTime, PawnCT, PawnCA, PawnCD, bNewLeftSided`，setter 分别引用 `PawnCA`、`PawnCD`；token 形状为 EX_LocalVariable + 已认证 Property 引用，最后 EndParm。它没有把 RAX 或 Result 当成 out 地址。

因此 U1 缩小为：**重新认证当前两种 state 的具体字段／调用时点，以及新增受限写入与伤害分支的权限**，不需要再把“是否存在 caller-local 改变”当作完全未知问题重做长期采样。旧 T 原始配置／记录在本轮检查的目录不可用，属于 H 报告证据；不能用历史数值替代当前写入证书。

首选写入时点进一步明确为原 `UpdatePlayerPosture` 正常返回后的同步边界、原 `PlayerMove` 尚未继续读取局部 CA/CD 之前。可复用现有 dispatcher 结构中的 `cover_call_live::CallScope::returned` 所处时相；该函数当前仍是只读观察器，不能直接打开写权限。

新增写入需证明当前实例和 caller 属于选中的 PlayerMove、真实返回成功、script/字段/locals 未变，且 updater 已完成回写。只适配 CA/CD，不更改 CT、沿墙移动字段或全部 locals。两字节更新作为一个受限事务：写前保存原值，原 caller 恢复执行前读回；中途失败仅在仍拥有该调用与原值比较匹配时恢复自有修改。失去恢复资格按现有故障流程处理，不能返回“成功”后再补救。不得保存栈指针供下一 Tick 写入。

## 2. 连续探身还会影响原版开火等待

新增静态调用链：

`WarPawn.DoingAnimationTransition` → `WarPawn.ShouldDelayFiring` → `WarWeapon.ShouldRefire`，以及 `WarPawn.Tick` 中的延迟开火／pending-fire 消费；`Weap_Bow_base.CanReleaseFire` 在盲射条件下也临时恢复动画等待检查。

具体风险有两个：

- 有掩体、CA为0、人类控制且不是360 aiming 时，`ShouldDelayFiring` 自身就会返回等待。将所有掩体动作恒改为0会使原版拒绝射击。
- `DoingAnimationTransition` 检查 CoverAction 与 LastCoverActionAnim 不一致，或 `bDoingCoverActionAnimTransition`。连续半探如果一直被当作过渡，按键和枪口即使正确，原开火链仍可能不放行。

F19 已加入问题表。实施采用原动作与姿态消费相匹配的实例状态，稳定半探不能永久占有“未完成过渡”。若需返回值适配，仅在当前 VR Pawn 的 cover-only 过渡条件、原调用与同版姿态均认证时处理；镜像、换枪、装填、真正动画过渡、原 no-firing 和特殊动作仍保留。禁止全局设 `bNoAnimDelayFiring=true` 或把整个 `ShouldDelayFiring` 强制 false。扭力弓仍使用武器包的取消／释放链，不因掩体适配提前释放箭。

## 3. 输入确认不能丢弃已经发生的身体提交

第一轮已经区分“输入 ticket”和跨 Tick 的 immutable intent。本轮补齐最容易漏掉的先后关系：

| 先后 | 确定行为 |
| --- | --- |
| 松手先发生，身体尚未提交 | 取消待处理 intent；晚到的查询或提交不能再移动身体 |
| 身体已提交，下一轮才读到松手 | 先同步 commit ledger 的实际位置，再松手和取消剩余 intent；保持刚提交的位置 |
| 新手样本替代旧候选，旧查询才回来 | intent id/from-version 不匹配，旧查询拒绝 |
| 旧确认晚于更晚提交 | 不能回滚新位置；以当前 ledger 为准，拒绝旧确认覆盖 |
| 裁剪后仍按住 | 在同步裁剪 receipt 时，用该次手位与实际 offset 重建起点，下一次反拉立即响应 |

**每轮处理抓取、移动、松手、失效前都先同步已经完成的 ledger。** 原身体消费者成功完成是运动生效点；输入 ack 只是确认事实，不是第二次移动许可。`CoverCommitReceipt` 增加实际裁剪状态、锁定轴、抓取 generation 和用于重设起点的手样本身份。渲染只读身体 publication；输入内核迟一轮确认不应让画面回退。

暂停／release／cancel 只能取消尚未发生的运动，不允许删除历史已发生的提交。若已提交位置后来碰撞失效，进入恢复处理而不是把逻辑账本置零。

## 4. 对接当前枪口消费者的具体位置

调查时的主线已有下列入口，cover 实现应在这些接口上补证据，不另写一套射击覆盖：

| 位置 | 现有职责 | 掩体需要追加的约束 |
| --- | --- | --- |
| `weapon_unified_fire.inl::prepare_current` | 正常 PostPlayerTick 取得 Muzzle socket、枪口方向和当前手／武器来源，保持原接收时间戳 | 准备与当前 cover commit/basis 关联的枪口与阻挡请求；不能把较旧姿态枪口重新标成新姿态 |
| `MuzzleSample`、`source_matches_shot` | 验证实例、hold、输入空间／reference 与来源先后，允许正常PostTick来源早于下一次publication | cover需显式的commit/basis/geometry证书引用；原单调性和新鲜度本身不能证明跨墙安全 |
| `typed_result` | 在受限原调用内识别 aim、trace-origin、pushed-origin、physical-origin 返回值 | 四种起点／方向路径必须使用一致的cover枪口与遮挡决策，不能遗漏 pushed-forward 起点 |
| `normal_return/store_result` | 正常返回后复核 typed schema／shot，按原值比较写入返回槽并处理不完整写入 | 增加当前cover版本检查；这里不能新增 Trace、引擎 getter、锁等待或进入client gate |
| `weapon_native_lancer_follow.inl` 的跟随／枪口变换 | 主线正在完善的实际枪模型跟随计划 | 与同一cover body transform关联；模型、socket和射击来源不能各加一次掩体偏移 |
| `weapon_call_live::invoke` | Native唯一原 CallFunction 调用和通知收尾 | 继续保留；不要为掩体另装一个全局dispatcher |

`AppliedEvidence` 当前提供shot、input、origin、aim等诊断，本身不是掩体占用或命中证书。`full_body_c0_native_backend.h` 明确说明现有 Context 不提供全程序 physics fence；`consumer_verified` 也不能被升级解释为“真实枪弹命中已验证”。这些限制已回写 U2/U3。

新增掩体检查只约束 cover 分支，不能在本轮改变主线 M4 行为。未来动作准备阶段拿不到当前证书时，不先发原press再退回原镜头瞄准；停止该次新发射意图，继续读真实release，不把旧按键无限排队成后来的幽灵射击。合法命中墙面的子弹仍可按原语义发射；阻挡不能一概等同禁用扳机。

## 5. 可直接复用的规格向量

[contract-vectors.json](contract-vectors.json) 提供28个合成时序案例、3个位移合成例；[offline-contract-checker.py](offline-contract-checker.py) 是独立Python参考模型。可在本目录运行：

```powershell
py -3 -X utf8 offline-contract-checker.py --output contract-check-results.json
```

本轮 [检查结果](contract-check-results.json)：28个时序案例、78个断言及3个位移合成例全部通过。覆盖等待中松手、提交先于输入确认、旧版本拒绝、裁剪后反拉、换轴组合、重新抓取、失效恢复与消费者版本不匹配等；实现C++时可把相同输入／预期作为测试素材。原消费者异常另外要求保留“不确定／恢复待完成”状态，不能因未发布成功就假设内存已回滚；仅有新占用证明也不能解除这笔债务。

参考模型只表达顺序与版本规则，fixture中的合法边缘／物理快照／空终点是人工假设。它不执行现有C++，不认证Trace、骨架、原伤害、原开火或头显；生产类型不能接受这些fixture bool当作证书。三个位移例只检查向量各加一次，不验证矩阵转换或真实头部旋转。不要把这些数量写入游戏验收通过数。
