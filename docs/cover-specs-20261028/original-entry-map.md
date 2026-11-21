# Original code portal

Evidence is saved. `WarfareGame` Script, reflect reader and history record. Original package `WarfareGame.u` The current identity is SHA.256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`;The round is in private `analysis/cover-prep-20261028/script-evidence-index.json`. The following original function name is a positioning key and is not a C++ declaration that can be called directly.

## Status and action chain

| Access | Original act/implementation decision |
| --- | --- |
| `WarPC.CoverAcquired` → `WarPawn.CoverAcquired` | The former selects the right- and left-hand side, notifying slots, clears RodieRun timer, trigger-cam BeginTurn, and chooses either regular or state. The latter has a CoverType/Action, CovPosInfo, Claim slot that can run into special actions, update times and transitions. Retain status/slot claim; local VR self-pressors automatic alignment of cameras. |
| `WarPC.PlayerTakingCover.BeginState` | Saves the control remapRotation, clears the break logo/input state. The model cannot be rotated without neglecting the original control remagnation; bunkers enter the syntax of moving from the wall to the same place. |
| Two state `PlayerMove` | Read CT/CA/CD, call this state's UpdatePlayerPosture, SetPawnCoverType, SetPawnCoverAction, SetCoverDirect,SetPawnSide, and finally,user.PlayerMove. Here is the preferred action output appropriate border. The original process is one, input and special actions side effects are maintained. |
| Normal `UpdatePlayerPosture` | Get the current Link/slot, update CurrentSlotDirect, static bunker switching, mirror/special move and left-side preferences. Last call is DetermineLeanDirect. Not all functions are replaced by empty. |
| Normal `DetermineLeanDirection` | For heavy load/cutting gun/360 ailing clear output, clicking on targing, delaying, slot endpoint, bCanPopUp, bLeanLeft/Right and Circular status; there is also a blind-fire branch without setting. It cannot be checked if the LT is pressed. |
| Stationary `DetermineLeanDirection`、`UpdatePlayerPosture` | There is an independent upward detection and blinding logic;updater can still write up blind shooting after the Lean function. Therefore, submitting to boundary two states in final action cannot be a single function. |
| `WarPC.SetPawnCoverAction` → `WarPawn.SetCoverAction` | The Contractor issue creates events for blind action, and calls Pawn matter. The Lawn issue updates LastCoverActionTime, Authority NetUpdateTime and handles non-human motion change intervals. The CoverAcade byte cannot be written directly over a long period of time. |
| `PlayerTakingCover.ProcessMove` | The VR grabs must leave the original legal entrances and cannot be seen as turning all forward gestures. |
| `TryToCoverSlip` → `WarSpecialMove_CoverSlip.InternalCanDoSpecialMove` | Checks bunker status, special move, break sign, circle type, left and right slot endpoint, bCanCoverSlip_Left/Right, input and distance. Keep the original Cando/ Do chain and call directly from scratching border signs. |
| Two state `ClientBreakFromCover`／`EndState` | break sign driver exit; Endstate clears CA/CD/CT, Statery and sound mode. The original exit is authoritative; expands the listening to it and releases the lease/turnover without blocking it with a new key state. |

Normal shipping path readable in script `CA=3/CD=1`、`CA=4/CD=2`、`CA=7/CD=3`;blind-fire paths CA 1/2/8, the incident string of the Controller matter is clearly corresponding to the BrendLeft/BlindRight/BlindUp. Only these static branches are identified here. The actual count/reference segments are still checked with current reflector certificates and cannot be plugged directly into an uncertain parameter address.

### Clear choice for action.

Select the actual selected `PlayerMove` Keeps CT, CurrentSlotDiraction, exit/turning and original aiming/fire requests; CA/CD expresses current real exposure and legitimate blinding, and is now available for the user to read.**No more reverse-drive automatic camera or body detection**. If the original animation is based on CA and does the same line again, it must be C 0 The strategy prevents duplicate supersing, rather than setting all CAs permanently to zero.

Existing `cover_call_config.py`、`cover_call_trace.h`、`cover_call_live.inl` Only passive observation is provided. Two sets of PlayerMove/ UpdatePlayerPosture and two matters have been exported from the historical records. Two PlayerMove locals size 20, CT/CA/CD is independent byte. The second round of updates the T-test history report: two sets of calls calleder-local did change and two later setter retained new values; therefore no longer includes "value back" as completely unsubstantiated.**This is still not the current full out argument ABI or secure write permission authentication**. Recheck the actual carler frame, funct bytes, locals, updater return values and setter reading points before achieving this; parameters flags do not rely on export statements. Resault/RAX is not an outpam address. It is forbidden to create another global certificateEvent hook or write later locals.[Second round of replenishment](research-followup-v2.md)。

### Original firing waiting chain (second round added)

`WarPawn.DoingAnimationTransition` The decision is made by the real transition/simulation state, and the difference between the CA and LastCoverActionAnim of the cover, bDoingCoverAction AnimTransion.`WarPawn.ShouldDelayFiring` In human cover/CA 0/ Not 360 (b) Waiting directly while aging, and reading the animation judgement under other conditions;`WarWeapon.ShouldRefire` The continuation of the firing was refused.`WarPawn.Tick` Consumption waiting for results, bow. `Weap_Bow_base.CanReleaseFire` The chain is also checked under blind shooting conditions. The stabilization semi-protocol must be legally open and cannot close the entire area until judgement has been reached.20。

## Original Protection and Injury

`WarPawn.IsProtectedByCover` It is a readable script function that no longer lists the entire protection logic as unknown in general terms: it first checks CoverType/player identity, excludes body-detection or blind-shooting actions, then judges according to Pawn Rotation, direction of attack and CoverProtectionFOV. The shift from back to wall may change that judgment, and must be matched by physical protection of surfaces/collapse, not merely animated.

`WarPawn.AdjustPawnDamage`: Multiple walkers `MultiplayerCoverDamageAdjust`; In a single person, the injury of a non-minority bunker calls for protection judgement and is satisfied with Damage and Momentum; then humans blindly return independently in zero form, and non-humans have harm shrunk. Both protections must be addressed together.`MultiplayerCoverDamageAdjust` CoverType/Action, Pawn Direction, Cylinder, Left-Ridd, Distance; this round does not extend individual support to online authority.

The selection range is currently local VR Pawn's cover-only protection judgement and blindly re-entry early. Decisions to preserve protection are based on actual hit/exposure evidence, then continue the chain of injury. Original injuries, attackers, HitLocation, Momentum, DamageType, DamageCauser, HitInfo and call identification must be tied; no CoverAcade, change other roles or write Health bypasses through temporary modifications. When a center area cannot respond to the same submission, it cannot claim that exposure has been addressed.

## Collision entrance Actor.

The diagnosis is... `tools/game-playtest/cover_trace_query.py`;it is not a frame of production backend. Accredited cohort condition: native index 277，`ParmsSize=116`，return offset 108,FunctionFlags is located `0xf4` And value `0x424401`, the history of the portal RVA `0xcdacc`, Gnatives History RVA `0x1b86280`. Disk retail EXE SHA 256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`;history running codeblob SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`I can't mix these two identities.

| Parameters | Offset/Size | Semantic |
| --- | --- | --- |
| HitLocation / HitNormal | 0/12；12/12 | Output |
| TraceEnd / TraceStart | 24/12；36/12 | Endpoint/start |
| bTraceActors | 48/4 | bool mask 1, select explicitly when calling |
| Extent | 52/12 | Sweeping expanses; zero extent only rays |
| HitInfo | 64/40 | Material、PhysMaterial、Item、LevelIndex、BoneName、HitComponent |
| ExtraTraceFlags | 104/4 | Diagnosis Supported Visibility 0 or 8；8 For blocking queries |
| ReturnValue | 108/8 | Actor returns value; call failure must be separated from valid hit |

The authentication requirements include full reflector head, field type/flags/order, bool mask, Gnatives[277 Target and code chunk bytes; call if not address matching. `_certificate` Specially validates the original Ufield.Next order because the packaging produces NativeParm tokens in that order. Do not repeat history PID/base or ignore return fields as non-natural alignment 116 byte block.

World inquiries, dynamic inactor queries, self exclusions and volume overlaps must be clearly distinguished.17 The world-only query is not covered by dynamic characters/mobile objects. The shape of the Express Trace and the syntax of the intimal-overlap also have to be A-based 04/A 05 Authentication; AABB cannot claim to be a precise human capsule test for all gestures.

## Where can history be used?

Checked [PROGRESS](../releases/0.0.4/PROGRESS.md) CT 17 Records multiple hits and valid empty results of the same Link/slot, toplined off-site 96–112 units；`collision_verified=false`、`grab_authorized=false`. Following 4 Not compiled. Early bones, tops, camera tests are used only as a locator thread; current protection writer is not changed, the current frozen Native/XR is not replaced and this round is not automatically authorized to run.

---

## 中文

# 原版代码接入口

证据源为保存的 `WarfareGame` 脚本、已有反射读取器及历史记录。原包 `WarfareGame.u` 的已有身份为 SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`；本轮逐份脚本哈希在私有 `analysis/cover-prep-20261028/script-evidence-index.json`。下列原函数名是定位键，不是可以直接调用的 C++ 声明。

## 状态和动作链

| 入口 | 原行为／实施决定 |
| --- | --- |
| `WarPC.CoverAcquired` → `WarPawn.CoverAcquired` | 前者选左右侧、通知达到槽位、清 RoadieRun timer、可触发相机 BeginTurn，并选择普通或 Stationary state。后者设 CoverType/Action、CovPosInfo、Claim 槽位，可能跑入特殊动作，更新时间和进入过渡。保留状态／slot claim；本地 VR 单独抑制相机自动对齐。 |
| `WarPC.PlayerTakingCover.BeginState` | 保存 ControlsRemapRotation、清 break 标志／输入状态。不能只旋转模型而忽略原控制重映射；掩体沿墙输入仍走原移动语义。 |
| 两个 state 的 `PlayerMove` | 读取 CT/CA/CD，调用本 state 的 UpdatePlayerPosture，再 SetPawnCoverType、SetPawnCoverAction、SetCoverDirection、SetPawnSide，最后 super.PlayerMove。这里是首选的动作输出适配边界。原流程一次，输入和特殊动作副作用保留。 |
| 普通 `UpdatePlayerPosture` | 取得当前 Link/slot、更新 CurrentSlotDirection、静止掩体切换、镜像／特殊移动和左右侧偏好，末尾调用 DetermineLeanDirection。不可整函数替换成空。 |
| 普通 `DetermineLeanDirection` | 针对重装／切枪／360 aiming 清输出，按 targeting、延迟 targeting、slot 端点、bCanPopUp、bLeanLeft/Right 和 circular 状态生成探身；无 targeting 时还有 blind-fire 分支。不能只查 LT 是否按下。 |
| Stationary `DetermineLeanDirection`、`UpdatePlayerPosture` | 有独立的上探和盲射逻辑；updater 在 lean 函数后仍能写上方盲射。因此在最终动作提交边界收敛两种 state，不能只 hook 一份函数。 |
| `WarPC.SetPawnCoverAction` → `WarPawn.SetCoverAction` | Controller setter 会为盲射动作生成事件，再调用 Pawn setter。Pawn setter 会更新 LastCoverActionTime、权威 NetUpdateTime，并处理非人类动作切换间隔。不能直接长期写 CoverAction byte 替代。 |
| `PlayerTakingCover.ProcessMove` | 以原视角／Pawn 朝向重映射 double-click，尝试 MantleUp、CoverSlip、MantleOver、SwatTurn、Evade。VR 抓边必须让开这些原合法入口，不能把所有前向手势映射为翻越。 |
| `TryToCoverSlip` → `WarSpecialMove_CoverSlip.InternalCanDoSpecialMove` | 检查掩体状态、special move、break 标志、圆形类型、左右槽端点及 bCanCoverSlip_Left/Right、输入和距离。保留原 CanDo/Do 链，不凭抓边标志直接调用。 |
| 两 state 的 `ClientBreakFromCover`／`EndState` | break 标志驱动退出；EndState 清 CA/CD/CT、Stationary 与声音模式。原退出是权威；扩展监听它并释放租约／交接，不用新按键状态阻挡它。 |

脚本中可读到普通 targeting 路径 `CA=3/CD=1`、`CA=4/CD=2`、`CA=7/CD=3`；盲射路径 CA 1/2/8，Controller setter 的事件字符串明确对应 BlindLeft/BlindRight/BlindUp。这里只认定这些静态分支含义。实际枚举／参数字段仍以当前反射证书核对，不能把数字直接塞到不确定的参数地址。

### 动作适配的明确选择

选择在实际选中的 `PlayerMove` 调用中、posture updater 返回之后、setter 读取之前，适配本实例的动作输出。保留 CT、CurrentSlotDirection、退出／翻越和原 aiming/fire 请求；CA/CD 表达当前真实暴露与合法盲射，**不再反向驱动自动相机或身体探出**。原动画若根据 CA 再做同一次 lean，必须由 C0 策略阻止重复叠加，而不是把所有 CA 恒设为零。

现有 `cover_call_config.py`、`cover_call_trace.h`、`cover_call_live.inl` 只提供被动观察。历史记录已导出两套 state 的 PlayerMove／UpdatePlayerPosture 和两个 setter，共六函数；两个 PlayerMove locals size 20，CT/CA/CD 为独立 byte。第二轮补充查到 T 试验历史报告：两组同调用 caller-local 确实改变，随后两个 setter 保留新值；因此不再把“值是否回到caller”列为完全没有证据。**这仍不是当前完整 out 参数 ABI 或安全写入权限的认证**。实现前用现有dispatcher重新核对实际caller frame、function bytes、locals、updater返回后的值和setter读取时点；参数flags不凭导出声明猜。Result/RAX不是outparam地址。禁止另建全局ProcessEvent hook或写事后保存的locals指针。详见[第二轮补充](research-followup-v2.md)。

### 原版开火等待链（第二轮新增）

`WarPawn.DoingAnimationTransition` 由真正过渡／镜像状态，以及cover的CA与LastCoverActionAnim差异、bDoingCoverActionAnimTransition决定。`WarPawn.ShouldDelayFiring` 在人类cover/CA0/非360 aiming时直接等待，其他条件还读取该动画判断；`WarWeapon.ShouldRefire` 据此拒绝续射。`WarPawn.Tick` 消费等待结果，弓的 `Weap_Bow_base.CanReleaseFire` 在盲射条件下也检查该链。稳定半探必须可合法开火，不得把原等待判断全局关闭。保留弓的释放／取消和真正动画所有权，新增验收A20。

## 原版保护与伤害

`WarPawn.IsProtectedByCover` 是本轮可读的脚本函数，不再把整个保护逻辑笼统列作未知 native：它先检查 CoverType／玩家身份，排除探身或盲射动作，再根据 Pawn Rotation、来袭方向和 CoverProtectionFOV 判断。将身体从背靠墙转为面朝墙可能改变该判断，必须用实际保护表面／来袭方向适配，不能只修动画。

`WarPawn.AdjustPawnDamage`：多人走 `MultiplayerCoverDamageAdjust`；单人中，非忽略掩体的伤害会调用保护判断，满足时清 Damage 和 Momentum；之后人类盲射又有独立的清零返回，非人类有伤害缩放。两条保护必须一起处理。`MultiplayerCoverDamageAdjust` 另依赖 CoverType/Action、Pawn 朝向、Cylinder、左右射线、距离；本轮不把单人修正推广为联机权威支持。

选定适配范围是当前本地 VR Pawn 的 cover-only 保护判断及盲射提前返回。依据实际命中／暴露证据决定是否保留保护，再继续原伤害链。原始伤害、攻击者、HitLocation、Momentum、DamageType、DamageCauser、HitInfo 和调用身份必须绑定；不通过临时改全局 CoverAction、改其他角色或直接写 Health 绕过。命中区域无法对应同一提交姿态时，不能宣称已解决暴露受击。

## 碰撞入口 Actor.Trace

现成诊断实现是 `tools/game-playtest/cover_trace_query.py`；它不是生产每帧 backend。已有受认证 cohort 条件：native index 277，`ParmsSize=116`，return offset 108，FunctionFlags 位于 `0xf4` 且值 `0x424401`，入口历史 RVA `0xcdacc`，GNatives 历史 RVA `0x1b86280`。磁盘零售 EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`；历史运行代码 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`。这两种身份不能混为一谈。

| 参数 | 偏移／大小 | 语义 |
| --- | --- | --- |
| HitLocation / HitNormal | 0/12；12/12 | 输出 Vector |
| TraceEnd / TraceStart | 24/12；36/12 | 终点／起点 |
| bTraceActors | 48/4 | bool mask 1，调用时明确选择 |
| Extent | 52/12 | 扫掠外扩量；零 extent 仅是射线 |
| HitInfo | 64/40 | Material、PhysMaterial、Item、LevelIndex、BoneName、HitComponent |
| ExtraTraceFlags | 104/4 | 已有诊断支持显式0或8；8表示 blocking 查询 |
| ReturnValue | 108/8 | Actor 返回值；调用失败与有效无命中必须分开 |

认证要求包括完整反射头、字段类型／flags／顺序、bool mask、GNatives[277] 目标和代码 chunk 字节；不是地址匹配就调用。现有 `_certificate` 特别验证原 UField.Next 顺序，因为包装器按该顺序产生 NativeParm tokens。不要复用历史 PID／基址或忽略 return 字段为非自然对齐的116字节参数块。

世界查询、动态 actor 查询、self 排除和体积重叠必须显式区分。CT17 使用的 world-only 查询没有动态角色／移动物体覆盖。Extent Trace 的形状及 initial-overlap 语义还需按 A04/A05 认证；不能把一条 AABB 扫掠宣称为所有姿态的精确人体胶囊测试。必要时采用保守包围体并报告保守拒绝，不能把漏检称为空间自由。

## 历史结果可以使用到哪里

已检入 [PROGRESS](../releases/0.0.4/PROGRESS.md) 的 CT17 记录了同一 Link/slot 的多条命中与有效空结果，顶沿位于离地96–112 units；`collision_verified=false`、`grab_authorized=false`。后续 candidate4 尚未编译通过。早期骨骼、顶沿、相机试验仅作为定位线索；不改变当前 production writer、不替换当前 frozen Native/XR，也不自动授权本轮运行。
