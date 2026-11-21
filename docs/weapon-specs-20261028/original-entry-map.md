# Original semantic access and connection contract

2026-10-28 Offline survey and design v 2. Counterpart[Weapons-specific cards](weapon-cards.md)and[Current cycle of review](interaction-review-v2.md). The function names here are from the saved UnrealScript export, resource default object and existing source/historical test. They are determined**Where to pick up, what to keep.**; export with unsolved natural token cannot be used to guess the layout of C++ parameters or to call directly to an address.

## E 0 - Level of evidence and call

| Level | Meaning | Use of this set of documents |
| --- | --- | --- |
| S | Save script/resource static evidence | Specific class, function relationship, original model reference, constant default; not the result of this operation |
| H | Historical running or byte evidence with corresponding version of identity | Reusable existing access surveys, still requiring current case identification and ownership; not to be called the current machine inspection |
| D | Design requirements for freezing this item | Gears, quota filling, branching and interruption; not disguised as original mechanism |
| U | Uncertified binding details | native ABI, false cells, state actual functions, coordinates/timer controls; unbound |

core package:`J:/gearsVr/analysis/packages/decompressed/WarfareGame.u`，SHA 256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`。

Map package:`J:/gearsVr/analysis/weapon-models-20261021/maps/MP_WarMachine.war`，SHA 256 `55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840`This is a recheck of the full Hashi; not retail EXE Hashi.

core saving function:`J:/gearsVr/analysis/re-kb/scripts/WarfareGame/Function_WarfareGame.<类.函数>_.uc`. Map new export: This checkout `analysis/weapon-specs-20261028/map-script/Function_<完整类.函数>_.uc`and `Class_*.uc`。`map-script-provenance.json` Link object path, sequenced offset/size, original object SHA 256 Export text SHA 256 Reconcile warning.

## C 1 - All conventional reload chains.

The semantic chain to save the code is: `ForceReload`/ Status Transfer  ⁇  State `Reloading.BeginState` → `PlayWeaponReloading` Arrange for original animations and `EndOfReloadTimer` Zoom in `EndOfReloadTimer` → `PerformReload` Transfer inventory returns to serviceable status. Specific sub-categories can overwrite these functions, and must be analyzed by current object and state, and cannot be called in a uniform manner.

| Get the gate. | Original role | VR Fitness Responsibility and Readback |
| --- | --- | --- |
| `WarWeapon.GetMagazineSize/GetMaxSpareAmmoSize` and current inventory reading | Acquisition of capacity, reserve ceiling and real count | Each instance bind/service starts reading; default object is only used for checking, not for writing capacity |
| `ForceReload`、`Reloading.BeginState` | Accept original reload, record ReloadStartTime, process status/reward | The first clear physical action of this gun is triggered once; the original acceptance is only timed. Option R 3 Sharing of business with gestures; confirmation of status and automatic termination of ownership |
| `PlayWeaponReloading`Status `EndOfReloadTimer` | Artifacts/weapons animation and automatic bookkeeping | Keep the necessary original action and sound; current local example is replaced by a sign-to-be, without fixed 8 Seconds automatically complete |
| `PerformReload` | Transfer of stocks, usually to the target of this round | No reassignments afterward; no distinction must be made between unrecorded and imprest submissions |
| `SetAmmoUsedCount/SetSpareAmmoCount` | Update real count and include side effects such as copying, bullet mark/tip | If a quota is required, read and read back through a check-up original or original inventory interface; not writing two fields |
| `SetWeaponAmmoBoneDisplay` | Use `AmmoControl` The hidden control of the | The handheld component is dislodged during the takeover. |
| `SetPawnAmmoAttachment` | Former MagazineMesh hangs to Pawn left hand, setting shadow parent/light environment; cleaning on unloaded | VR and hand parts can only be owned by one person, so that the original animation is not notified to take it back. |
| Guns! `Notify_Ammo*` | Subcategory is attached/hidden, and part of the base class notification is empty | One hand over against a valid action; no refilling guess by name |
| `AbortWeaponReload`、`Reloading.EndState` | Cleans up timers and animations;Abort specific fire mode 10/11 Call PerformReload; Endstate may call Abort | Interruption must also be in the same non-duplicate bookkeeping; Abort cannot be simply cancelled |

**The quota must be completed first and shared.** The service identity contains at least session, weapon example/generation agent, original reloading serial number, input serial number, plan version; each object has its own IID, and every inventory is submitted token and prior to submission. Only the bullet is retained, the insertion request q is executed only if the current instance/version/number still matches. The reply gives at least a realistic count-up and status of the original; failure does not play the completed feedback, unknown execution results are read back first, and no blind attempt can be made to drop the bullet.

The original two issues are not natural atoms. The executive end is submitted in a series of lines along the same game; if only one update succeeds, read back and compensates for its definitive status, prohibiting “success” before repairing inventory. The online authority does not declare support without authentication, and the first line is the Playground/warm of native authorities.

The original fit-out should be maintained for timers, fire mode, feedback, animation and subcategory exit clearances while preventing the second transfer of inventory from this service.**Select the PerformReload inhibition or independent exit entry for this instance to be certified first, and not directly to the global hull.** If ownership cannot be obtained, physical alternation is not activated, the behaviour is maintained and reasons for non-support are recorded.

v 2 Three additional adaptive constraints: old box plug-in is cancelled and cannot be called to fill; full/no backup physical check does not force the call for ForceReload; Gnasher/Boltok partially filled has been closed early to submit a normal quantity exit without giving an perfect reward. All three must go through their own inventory/timer and not again with the exit chain. The existing fixed step seter will need to add early locking paths in the future, and this wheel has not changed its source code.

## C 2 - Perfectly swapped.

`GetActiveReloadValues` Provides a window for current examples. The specification uses the original perfect zone endpoint as the last necessary gesture, not the `ReloadDuration` Or uncovered-1 Replace the member. Call the original `LocalActiveReloadSuccess/ActiveReloadSuccess` The synonyms and specific state of the parameters must be checked before waiting;`PlayActiveReloadSuperSuccess` Once the end timer is reset, the reward must also enter C.1 (c) The scope of ownership.

The end-point time of gestures is recorded separately from the time of inventory confirmation: a valid action sample determines whether to be timely and the original inventory/mechanical backfill is awarded once. The normal completion without certification period cannot be defaulted as perfect. If the old general ReloadModel imposes a valid term, it needs to fit with clear ordinary results instead of filling in a large number.

History 2026-10-22 The retail evidence of the exec sha.256 Yes `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`：

| Saved Entry | History RVA | Saved bytes comparison range | Use and limitations |
| --- | --- | --- | --- |
| GetActive ReloadValues Script Entry | 0x3e7ffc | 256 Bytes | Entry location H;not a specified parameter layout |
| DetermineActiveReloadScruise OrFairure script entry | 0x39bfcc | 256 Bytes | Results of original survey H; cannot replace process with time for original buttons |
| GetActive ReloadValues Fake | 0x3e81b0 | 4096 Bytes | History running matching history snapshots; current process not read this time |
| DetermineActiveReloadSeccess OrFaiIure Fiction | 0x3e7d28 | 4096 Bytes | Idem;does not equate RVA with byte or ABI |

Original file is `J:/gearsVr/analysis/weapon-reload-probe-20261022/native-entry-byte-check.json` and `native-virtual-byte-check.json`. The current integration still requires the authentication of modular identities, precise parameter layouts, game threads to call boundaries and actual return values; no copying of historical PID/base site.

## W - Gun-by-gun entrance watch

W in Table 01–08 Reuse C 1/C 2；W 09–12 The base category is the WarfareGame package by default.

| Item | The original entry and the behaviour that must be retained. | Difference between log module and fit-out already in place |
| --- | --- | --- |
| W 01 Lancer | Weap_AssaultRifle.Reloating.Beginstate; Notify_AmmoGrab/Release/Reload; original launch/saw chain extension mainline | `weapon_reload_model.h`, Lancer feed_archor/insulation/magazine_pose; change test timers to service control, drive real card buttons/boxes |
| W 02 Hammerburst | Weap_LocustAssaultRifle.Notify_AmmoGrab/Release/Reload/Throw；FireAmmunition | `weapon_hammerburst_reload.h`;lock buttons tack box locks, keeping the original firing rhythm and notifying resource handover |
| W 03 Gnasher | Weap_Shotgun.CustomFire/FireAllPellets；ShotgunCocking.BeginState/FinishedCockingShotgun；PlayShotgunCocking | `weapon_gnasher_mechanics.h`; both reload and finish after firing; individual back-to-back call for each round of inventory and leverage |
| W 04 Longshot | Weap_SniperRifle.PlayWeaponReloading；Reloading.EndState/ForceReload/TargetingModeChanged；ShouldAutoReload | `weapon_longshot_reload.h`; retaining retrospect and targeting clearance, not automatically replacing a second time after the single live ammunition is submitted |
| W 05 Snub | Weap_COGPistol.PlayWeaponReloading/StartFire；CockOpen/CockClose/CockCloseFinish/EjectClip/InsertClip | `weapon_snub_reload.h`;cock/Clip function is sound only, not an API; the Silo/Silo plan is selected by D rule |
| W 06 Boltok | Lock/Clip audio notification for Weap_LocustPistol, inventory and status from C 1 | `weapon_boltok_reload.h`；Object 03 DriveObject 02/08，Object 01 Stay in root; original animations are still not found and offline design angles are inappropriately running endpoints; sound effects cannot open the bullet nest |
| W 07 Boomshot | Weap_Boomer_base.Notify_AmmoGrab/Release/Reload；GetProjectileClass/GetProjectileFirePosition/ProjectileFire | `weapon_boomshot_interaction.h`;filling selections of original MagazineMesh 0, the treatment of incentive properties continues to be maintained by the former ProjectFire |
| W 08 Torque Bow | Weap_Bow_base.PlayWeaponReloading/Notify_AmmoGrab/Notify_AmmoReload；Charge.BeginState/EndFire/Tick/EndState；CheckFireShot；ChargeEnded；Engine.Weapon.ClearPendingFire/PutDownWeapon；WarWeapon.Active.BeginState | `weapon_torque_bow_interaction.h`;for the order of cancellation for non-launch, see C 3;CanReleaseFire is not a minimum level of retention. Static combinations are clear and ABI/Practical Status Readback Uncertified |
| W 09 Hammer of Dawn | warfaregamecontentweapons.WEAP_HammerOfDawn.BeginFire/EndFire/GetTraceImpact/Tick/UpdateFX；BeamManager.NotifyValidTarget/NotifyInvalidTarget/NotifyStopFire/BeginBeamWarmup | `weapon_hammer_of_dawn_interaction.h`; actual target track is the same as the original BeamManager state, stopping chain final AbortFire; not just display beams |
| W 10 Troika | Turret_TroikaCabal_base.ClaimTurret/UnclaimTurret/DriverEnter/DriverLeave/UpdateTurret/GetPhysicalFireStartLoc；Weap_Troika_base.Active.BeginFire/WarmingUp.EndFire/InstantFire/WeaponFired/GetReloadDuration/Reloading.BeginState/EndState | `weapon_troika_interaction.h`;situation of position occupancy, fixed axis and double-gun target to read; retention Loader associated automatic recovery |
| W 11 Frag | Weap_Grende.StartGrenadeSpin/StopGrenadeSpin/StartGranedThrow/AnimNotifyThrow/ThrowGrande/GrenadeThrownNotification; Proj_Grende.StopBounce/AttachInit/BeginExelosionSequence/Hitwall/Bounce/DoExprosion; Proj_FragGrende | `weapon_bolo_grenade_interaction.h`、`grenade_throw.h`; press T-T 1 Replace birthplace and initial speed; ordinary stop...1+1 s, attached 2.5 s Both implemented under the original conditions |
| W 12 Smoke | Same Weap_Grenade chain; Proj_SmokeGrende. StopBounce/AttachInit/GrandeExplose, Emit_SmokeGrende effects/cleanup | `weapon_smoke_grenade_interaction.h`、`grenade_throw.h`; general stop direct DoExprosion, adhesive create direct smoke; no fung timer; inherit special Hitwall still |

## C 3 – Torque Bow off-launch group

Only when the current instance is Charles and local VRs own the request: stop putting the trigger in the poach `Engine.Weapon.ClearPendingFire` The launch mode of the clean-up business. `bPendingEndFire` → `Weap_Bow_base.ChargeEnded` • Normality is transferred to Active; the actual cut is called normal PutDownWeapon into WebPuttingDown. The transition to normal status is automatically implemented by Charles.Endstate, clears PlayMaxCharge, simulation, sound, holding switch, vibrating and trigger feedback, fire mode. Do not hand-write EndStat and then re-turn.

The only thing that Charles Endeded did was set up is the TensionIntroEnded and some of its sound effects, which were not enough alone;StopFire will call the current EndFire, Charles. EndFire again in CheckFireShot, not to be used for cancellation.Active. Beginstate will restart the PendingFire that remains, so you have to clear it first.WeaponPuttingDown.Beginstate left for ForceEndFire when he was cutting his gun, and still needs to certify dynamic assignments.Last read, non-Charge/Fire, no pending, still running out of bullets, and then de-blue the trigger.If a fire has entered, it cannot be falsely cancelled or the arrows that have been sent back.

This is the minimum combination design to save script support; current native parameters, state call and unsorted co-serial sequence are not yet valid. Former CharlestimeCount starts with MinChargeTime and rewards branch can be used 2 Multiply; MaxChargeTime+HoldTimeUntilTired=6.5 is the default count threshold, not a fixed reality holding time.

## T 1 - The original boundary of hand gestures.

Original `AnimNotifyThrow` ThrowGrende, then GreeneThrawown Notification. Throw Grende takes GetPhysicalFirStartLoc, Spawn Original projectile class under original authority; and ConsumeAmmo, after success, calls IniteDir, writes Velocity from GetGrendeTossVelacy and saves InternalVX/VY/VZ. There are also AI-specific correction branches where local VR replacements do not override AI.

So the node is fixed to three semantics:**The position of the player before Spawn, the initial direction required for Init, the first speed before the first physical step/velocity copy cache.** Priority is given to the current ThrowGrende domain covering the return value and direction input of Getter, so that the original logic is written naturally. Do not create it in camera before moving to your hand; then birth collisions and echoes may have occurred.

| Time | Input/Action | The original reply that must be made |
| --- | --- | --- |
| Ready. | Example of current grenade, original readiness, active handing sample | No call generation without a gesture release token; original fuses activated and separate records are initiated |
| Untie the trigger. | A release event with a rise in serial numbers, alignment of attitude time, fixed body spatial generational and type | Release token only once; failure to track does not create a loose key |
| Before Generating | Legitimate hand position, release reference point, direction, speed; short distance volume check | Start without a wall, old Spawn uses that point; collision failure is consistent and does not push behind the barrier |
| Old Spawn/Init | Call the original class and inject it into the same plan position/direction/first speed | Effective atomic body identity, initial Location/Velacy and cache; failure to produce a shell or blind re-capture stock |
| Handover. | Former ConsumeAmmo and GlenadeThrowNotification | One bullet-consuming, original sound/vibration/DetachWeapon; no more fieldings after animation |
| After flight | gravitational force, collisions, rebounds, fuses and effects | Release exit, VR. Keep the parabolic line as desired without a frame-by-frame puller. |

`StartGrenadeThrow` It is about preparation tips/flash, etc., and not the actual Spawn point. Replaces it or hides only the original trajectory preview without changing the actual drop.`GrenadeThrownNotification` It also contains original aerodynamics, Detach Weapon, audio, etc. so it is not possible to call Spawn and skip the post-investment process.

Speed uses the history of release reference points in the same tracking space:

```text
p_item(t) = p_grip(t) + R_grip(t) * r_item
v_item    = 对有效短窗口的 p_item(t) 拟合
p_world   = release_body_basis * p_item(release_time)
v_world   = units_and_axes(v_item) + base_translation_velocity_once
```

Existing `grenade_throw.h` Provide maximum 140 ms is ready to be used as a basis for preparing, etc.; end-weighting or device speed can be compared at subsequent hand sense stages. The wrist rotation is already reflected in p_item history and cannot be added once. If the device line/angle velocity is changed, it will be synthesized with the base v_grip+ ⁇ xr and the valid position checked. See OpenXR for reference.[Description of the original programme](../all-weapons-interaction-plan-20261028.md)。

The proposed completion in the joint tracking space is only one release: head-to-head, Snap shift, session/reference generational change, time leap to clean history and recollect valid samples; original bAddBaseVelacy/InitialFrameOfReVelacy and VR role speeds are selected as a composite point. Roles/carryers can be moved continuously at a level that cannot be repeated by world historical differences with swinging poles.

Low-speed release is a design support item that needs to be minimally prototyped 0.5 m/s／4 cm The movement threshold is separated from the tracking effectiveness; effective static discharge should be at near zero hand speed plus a base speed.2026-11-08 User changed the Frag to a continuous right Grip hold, and a pine Grip cast 2 kg Virtual weight target, see[Current rules and actual evidence](../frag-grip-release-20261108.md). The measured speed is not reduced by mass or applied to the same amount after writing the speed.

Proj_Grenade Static GravityScale=2、Speed=1500、MaxSpeed=0、LifeSpan=10; these are engine data that cannot be directly the gravity of SI-world.10 Second-sec fuse or speed cap 0. If the visual preview shows that active gravity/units are used in the current atomic body; physical motion is always driven by the original physics and no simple parabolic lines continue after the first collision.

Fuse activation has been detected to save scripts: preparation/ rotation and Spawn/Init did not schedule explosions timer; no new cookies are added. Frag inherits Proj_Grende. StopBounce, after movement is stopped, and BeginExpletionSequence; default.1+1 S, Base is Pawn 1.5+1 s, AttachInit direct arrangement 2.5 s. Smoke overstuffs StopBounce, directly DoExplesion; its AttachInit directly generates attached smoke. Stopping can be triggered by speed/backball counts and is not equivalent to the first landing.

Both categories also retain the special duplicate location branch of HitWall, BeginExplesionSequence.0, TimeAfterBeep). BeginExplesionSequence returns directly to simulated bodies and is heavily protected against DoExprosion timer; DoExposion has bHasExploded door control. It cannot be unified as “free start countdown” or “Smoke collisions of any kind are immediately effective”. See full branch.[Review form](interaction-review-v2.md). The actual call to ABI/Routing Time Order is not verified; once the original fuse is activated, it does not result in a failure or re-capturing of the brush. StopGrendeSpin clears for animation and does not equal the defuse of an existing bullet.

## I 0 - Achieving the organizational and non-renewal components

Keep existing `weapon_hand_client.h`、`weapon_action_boundary.h` and the main line only Native/playline execution end. The gun-by-gun module generates action requests, executes end-string verification and calls to original functions, returning to actual status. It is not possible for each weapons head file to call upon Procesevent, write bones or start a game.

Suggested additional thin matching file to remain as `tools/playground/weapon_reload_adapter.inl` and `weapon_throw_adapter.inl`；**No source files created this time**. Publicly fit to carry identity/original timer/stock/reciprocate; gun-by-gun program is kept within the specific type available. Model attitude is given to the existing only component/bones writing end, without creating a second bone replacement.

The configuration fields should be divided into three groups: static sources (class, resources, Hash), design steps (actions and quotas), case calibration (real object/bones table, axes, pivotal axis, itinerary, original window, status entrance). `current_instance_verified=false`. The offline specification index cannot be automatically loaded into the production cycle or generate a "enabling all weapons" switch.

## I 1 - A narrow list of tasks before access.

| Item | Specific to be certified | Unlock after completion |
| --- | --- | --- |
| B 01 Public swap. | Current state function parsing; timer cancel/ restore; PerformReload and Abort avoid duplication of same business; setter parameters and backback; reward for resetting timer | W 01–08 Real inventory is secure. |
| B 02 Activity | Recovery of bone, father-child benchmark, original animation for the same weapon example; fixed fillings; Snub handheld original box components | Mechanical action and handheld ammunition display |
| B 03 Gnasher | The original timer of ShotgunCocking was closed only once with the gesture; the exit status and launch interval were kept together | Each hand lever |
| B 04 Boltok | Static staft/blaster relationships and new six-holes delivered; real case scale/end, original animation or alternative driver restoration, 6 slots and partial filling account still required | Round-up of original parts and supplements |
| B 05 Torque Bow | C 3 Unassembly static positioning; still required parameters, actual status assignments, serialization of game threads and reading back to unconsumpted ammunition | Resuscitation of untraceable/gun-cutting disposal |
| B 06 Hammer of Dawn | GetTraceImpact weapons starting point/ aim parameter; BeamManager identity, enabler/target condition, EndFire call complete | Finger to the original and stop. |
| B 07 Troika | Gun position collect/exit parameters, original axis limit, double-gun rule, Loader state | Fixed machine gun double-handle operation |
| B 08 Two grenades. | Two static circuits of fuses are separated; still need to be replaced by a pre-return value from Spawn, Init parameters, initial speed/cache sequence, animation notify to weight and actual branch cleanup | Real gesture trajectory. |
| B 09 Original asset loaded | Models produced; one Gnasher engine load, component, material, depth, GC/destruction still required | Three full live ammunition and a nest to load the pipe. |

These are limited binding gaps, not re-reverse the whole game. Each subsequent primary survey continues to look at existing knowledge bases and preserve evidence; minimum IDA/operational observations are made only when required for corresponding gaps. The current wheel completes static and design without triggering the actual task.

---

## 中文

# 原版语义入口与接线合同

2026-10-28，离线调查与设计 v2。对应[武器规格卡](weapon-cards.md)和[本轮复核](interaction-review-v2.md)。这里的函数名来自保存的 UnrealScript 导出、资源默认对象和已有源码／历史试验。它们确定**在哪里接、必须保留什么行为**；带 unresolved native token 的导出不能用于猜 C++ 参数布局或直接调用地址。

## E0 — 证据与调用等级

| 等级 | 含义 | 本套文档的使用 |
| --- | --- | --- |
| S | 保存脚本／资源静态证据 | 具体类、函数关系、原模型引用、常量默认值；不是本次运行结果 |
| H | 有对应版本身份的历史运行或字节证据 | 可复用现有入口调查，仍需当前实例身份和所有权；不能称为本次实机验收 |
| D | 本项目冻结的设计要求 | 手势、定额装填、分支与中断；不伪装成原版机制 |
| U | 尚未认证的绑定细节 | native ABI、虚函数槽、状态实际函数、坐标／计时器控制；保持未绑定 |

core 包：`J:/gearsVr/analysis/packages/decompressed/WarfareGame.u`，SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`。

map 包：`J:/gearsVr/analysis/weapon-models-20261021/maps/MP_WarMachine.war`，SHA256 `55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840`。这两项本次重新核对过完整包哈希；不是零售 EXE 哈希。

core 保存函数：`J:/gearsVr/analysis/re-kb/scripts/WarfareGame/Function_WarfareGame.<类.函数>_.uc`。map 新导出：本 checkout 的 `analysis/weapon-specs-20261028/map-script/Function_<完整类.函数>_.uc`，以及 `Class_*.uc`。`map-script-provenance.json` 关联对象路径、序列化偏移／大小、原对象 SHA256、导出文本 SHA256和解析警告。

## C1 — 所有常规换弹的原链

保存代码的语义链为：原 `ForceReload`／状态转移 → 状态 `Reloading.BeginState` → `PlayWeaponReloading` 安排原动画和 `EndOfReloadTimer` → 状态内 `EndOfReloadTimer` → `PerformReload` 转移库存 → 返回可用状态。具体子类可以覆盖这些函数，必须按当前对象及当前状态解析，不能统一调用同名基类地址。

| 接入口 | 原作用 | VR 适配责任与读回 |
| --- | --- | --- |
| `WarWeapon.GetMagazineSize/GetMaxSpareAmmoSize` 及当前库存读取 | 取得容量、备用上限与真实计数 | 每次实例绑定／事务开始读取；默认对象只用于检查，不强写容量 |
| `ForceReload`、`Reloading.BeginState` | 接受原换弹，记录 ReloadStartTime，处理原状态／奖励 | 本枪首个明确物理动作触发一次；原接受才计时。可选 R3 与手势共享事务；确认状态并取得自动结束所有权 |
| `PlayWeaponReloading`、状态 `EndOfReloadTimer` | 人物／武器动画和自动记账时刻 | 保留必要原动作与音效；当前本地实例改为等手势完成，不使用固定 8 秒自动完成 |
| `PerformReload` | 转移库存，通常会补到本轮原目标 | 不能在已逐发提交后无条件再调用；必须同事务内区分尚未记账和已定额提交 |
| `SetAmmoUsedCount/SetSpareAmmoCount` | 更新真实计数，并包含复制、收弹标记／提示等副作用 | 若需要定额提交，通过验证后的原 setter 或原库存接口成对操作并读回；不裸写两个字段 |
| `SetWeaponAmmoBoneDisplay` | 使用 `AmmoControl` 的显隐控制 | 接管期间与手持组件互斥，退出恢复。它不改库存 |
| `SetPawnAmmoAttachment` | 原 MagazineMesh 挂到 Pawn 左手，设置 shadow parent/light environment；卸下时清理 | VR 跟手组件只能有一个所有者，防止原动画通知把它抢回人物手上 |
| 各枪 `Notify_Ammo*` | 子类负责挂接／显隐，部分基类通知为空 | 对应有效动作时一次交接；不按通知名字猜是否补弹 |
| `AbortWeaponReload`、`Reloading.EndState` | 清理计时器和动画；Abort 在特定 fire mode 10/11 会调用 PerformReload；EndState 可能再调用 Abort | 中断也必须进入同一防重复记账范围；不能把“Abort”当纯取消函数 |

**定额事务必须先完成，逐枪共用。** 事务身份至少含 session、武器实例／生成代、原换弹序号、输入序号、计划版本；每个实物另有 item ID，每次库存提交另有 token 和提交前库存 revision。取弹只保留，插入请求 q 只有在当前实例／版本／数量仍匹配时执行。回执至少给出实际前后计数与原状态；失败不播放完成反馈，未知执行结果先读回，不能盲目重试扣弹。

原两次 setter 不是天然原子事务。执行端在同一游戏线程串行提交；若只有一项更新成功，读回并补偿到明确状态，禁止先回“成功”再修库存。联机权威未认证时不声明支持，第一接线范围是原本地权威的 Playground／战役。

原完成适配应保留计时器、fire mode、反馈、动画和子类退出清理，同时阻止本事务第二次转移库存。**选择本实例的 PerformReload 抑制或独立退出入口需要先认证，不能直接全局 hook 掉补弹。** 如无法取得所有权，不启动物理换弹表现，保持原行为并记录未支持原因。

v2 新增三个适配约束：旧匣插回属于取消，不能调用补满；满匣／无备用的物理检查不强制调用 ForceReload；Gnasher／Boltok 部分装填后提前闭锁以已提交数量普通退出，不授完美奖励。三者都必须经过本事务的库存／计时器去重，不能沿原退出链再次 PerformReload。现有固定步骤状态机未来需增加提前闭锁路径，本轮没有改源码。

## C2 — 完美换弹

`GetActiveReloadValues` 提供当前实例窗口。规格采用原完美区终点作为最后必要手势的期限，不用 `ReloadDuration` 或未覆盖的 -1 成员代替。调用原 `LocalActiveReloadSuccess/ActiveReloadSuccess` 等前必须核对参数语义和具体状态；`PlayActiveReloadSuperSuccess` 等可能重设结束计时器，奖励也必须进入 C1 所有权范围。

手势终点时间与库存确认时间分开记录：用可信动作样本决定是否及时，原库存／机械回执齐全后才授奖一次。没有已认证期限时普通完成，不能默认为完美。旧通用 ReloadModel 若强制要求有效期限，需要通过明确普通结果适配，而不是填造一个大数。

历史 2026-10-22 的零售证据 EXE SHA256 为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`：

| 已保存入口 | 历史 RVA | 已保存字节比较范围 | 用途与限制 |
| --- | --- | --- | --- |
| GetActiveReloadValues 脚本入口 | 0x3e7ffc | 256 字节 | 入口定位 H；不是已说明参数布局 |
| DetermineActiveReloadSuccessOrFailure 脚本入口 | 0x39bfcc | 256 字节 | 原结果判定调查 H；不能据此把全流程替成原按钮时机 |
| GetActiveReloadValues 虚函数实现 | 0x3e81b0 | 4096 字节 | 历史运行与历史快照匹配；本次未读取当前进程 |
| DetermineActiveReloadSuccessOrFailure 虚函数实现 | 0x3e7d28 | 4096 字节 | 同上；不把 RVA 相同等同字节或 ABI 相同 |

原始文件为 `J:/gearsVr/analysis/weapon-reload-probe-20261022/native-entry-byte-check.json` 与 `native-virtual-byte-check.json`。当前集成仍须模块身份、确切参数布局、游戏线程调用边界和实际返回值认证；不照抄历史 PID／基址。

## W — 逐枪入口表

表内 W01–08 都复用 C1/C2；W09–12 走专用使用／投掷链，不强行套普通换匣。基础类默认属于 WarfareGame 包。

| 项 | 本枪原版入口和必须保留的行为 | 已有逻辑模块与适配差额 |
| --- | --- | --- |
| W01 Lancer | Weap_AssaultRifle.Reloading.BeginState；Notify_AmmoGrab/Release/Reload；原发射／电锯链延用主线 | `weapon_reload_model.h`、Lancer feed_anchor/insertion/magazine_pose；把试验用计时器保持改成事务控制、驱动真实卡扣／机匣 |
| W02 Hammerburst | Weap_LocustAssaultRifle.Notify_AmmoGrab/Release/Reload/Throw；FireAmmunition | `weapon_hammerburst_reload.h`；锁扣 → 匣 → 锁扣，保留原射击节奏及通知资源交接 |
| W03 Gnasher | Weap_Shotgun.CustomFire/FireAllPellets；ShotgunCocking.BeginState/FinishedCockingShotgun；PlayShotgunCocking | `weapon_gnasher_mechanics.h`；既要接原换弹 timer，也要接原射后完成 timer；每发库存和杠杆回位独立回执 |
| W04 Longshot | Weap_SniperRifle.PlayWeaponReloading；Reloading.EndState/ForceReload/TargetingModeChanged；ShouldAutoReload | `weapon_longshot_reload.h`；保留退镜与瞄准状态清理，单发实弹提交后不由自动换弹补第二次 |
| W05 Snub | Weap_COGPistol.PlayWeaponReloading/StartFire；CockOpen/CockClose/CockCloseFinish/EjectClip/InsertClip | `weapon_snub_reload.h`；Cock/Clip 函数只有音效，不是机构 API；空仓／半仓计划由 D 规则选择 |
| W06 Boltok | Weap_LocustPistol 的 Cock/Clip 音效通知，库存和状态来自 C1 | `weapon_boltok_reload.h`；Object03 驱动子件 Object02/08，Object01 留在 root；原动画仍未找到，离线设计角度不当运行端点；音效不能开放弹巢 |
| W07 Boomshot | Weap_Boomer_base.Notify_AmmoGrab/Release/Reload；GetProjectileClass/GetProjectileFirePosition/ProjectileFire | `weapon_boomshot_interaction.h`；装填件选择原 MagazineMesh0，原 ProjectileFire 对奖励属性的处理继续保留 |
| W08 Torque Bow | Weap_Bow_base.PlayWeaponReloading/Notify_AmmoGrab/Notify_AmmoReload；Charge.BeginState/EndFire/Tick/EndState；CheckFireShot；ChargeEnded；Engine.Weapon.ClearPendingFire/PutDownWeapon；WarWeapon.Active.BeginState | `weapon_torque_bow_interaction.h`；非发射取消顺序见 C3；CanReleaseFire 不是最低蓄力门槛。静态组合已明确，ABI／实际状态读回未认证 |
| W09 Hammer of Dawn | warfaregamecontentweapons.WEAP_HammerOfDawn.BeginFire/EndFire/GetTraceImpact/Tick/UpdateFX；BeamManager.NotifyValidTarget/NotifyInvalidTarget/NotifyStopFire/BeginBeamWarmup | `weapon_hammer_of_dawn_interaction.h`；实际目标 trace 和原 BeamManager 状态同源，停止链最终 AbortFire；不只改显示光束 |
| W10 Troika | Turret_TroikaCabal_base.ClaimTurret/UnclaimTurret/DriverEnter/DriverLeave/UpdateTurret/GetPhysicalFireStartLoc；Weap_Troika_base.Active.BeginFire/WarmingUp.EndFire/InstantFire/WeaponFired/GetReloadDuration/Reloading.BeginState/EndState | `weapon_troika_interaction.h`；炮位占用、固定枢轴和双枪口要读原实例；保留 Loader 相关自动恢复 |
| W11 Frag | Weap_Grenade.StartGrenadeSpin/StopGrenadeSpin/StartGrenadeThrow/AnimNotifyThrow/ThrowGrenade/GrenadeThrownNotification；Proj_Grenade.StopBounce/AttachInit/BeginExplosionSequence/HitWall/Bounce/DoExplosion；Proj_FragGrenade 的原物理 | `weapon_bolo_grenade_interaction.h`、`grenade_throw.h`；按 T1 替换出生点和初速；普通停止 .1+1 s，贴附 2.5 s 均沿原条件执行 |
| W12 Smoke | 同一 Weap_Grenade 链；Proj_SmokeGrenade.StopBounce/AttachInit/GrenadeExplosion，Emit_SmokeGrenade 的效果／清理 | `weapon_smoke_grenade_interaction.h`、`grenade_throw.h`；普通停止直接 DoExplosion、贴附直接创建烟雾；不能套 Frag timer；继承特殊 HitWall 仍保留 |

## C3 — Torque Bow 非发射取消组合

仅在当前实例 Charge 且本地 VR 拥有该请求时：停止注入扳机 → `Engine.Weapon.ClearPendingFire` 清本事务的待发射模式 → 清本轮 `bPendingEndFire` → `Weap_Bow_base.ChargeEnded` → 正常状态转到 Active；实际切枪则调用正常 PutDownWeapon，进入 WeaponPuttingDown。正常状态转换自动执行 Charge.EndState，清 PlayMaxCharge、模拟、音效、蓄力开关、震动与扳机反馈、fire mode。不要手调 EndState 后再转状态。

ChargeEnded 只清 TensionIntroEnded 与部分音效，单独不够；StopFire 会调用当前 EndFire，Charge.EndFire 又进 CheckFireShot，不能用作取消。Active.BeginState 会重启仍保留的 PendingFire，因此必须先清输入。切枪路径 WeaponPuttingDown.BeginState 后续 ForceEndFire 时已离开 Charge，仍需认证动态分派。最后读回非 Charge/Fire、无 pending、弹耗未变，实际松扳机后才能再按。若已进入 Fire，不能伪报取消成功或补回已经发出的箭。

这是保存脚本支持的最小组合设计；当前 native 参数、状态调用和取消与发射的同线程排序尚未运行认证。原 ChargeTimeCount 从 MinChargeTime 开始，奖励分支可 2 倍推进；MaxChargeTime+HoldTimeUntilTired=6.5 是默认计数阈值，不是固定现实持有时长。

## T1 — 手势投掷的原生成边界

原 `AnimNotifyThrow` 调 ThrowGrenade，随后调 GrenadeThrownNotification。ThrowGrenade 在原权威条件下取 GetPhysicalFireStartLoc，Spawn 原 projectile class；成功后 ConsumeAmmo，调用 Init(SimulationDir)，从 GetGrenadeTossVelocity 写 Velocity，并保存 InitialVX/VY/VZ。另有 AI 专属修正分支，本地 VR 替换不能覆盖 AI。

因此接点固定为三个语义量：**Spawn 前的出手位置、Init 所需初始方向、首次物理步／速度复制缓存前的初速度。** 优先在本次 ThrowGrenade 的作用域覆盖对应 Getter 返回值和方向输入，让原后续逻辑自然写入。不要先在镜头处生成，再搬到手里；那样出生碰撞和回调可能已发生。

| 时刻 | 输入／动作 | 必须得到的原回执 |
| --- | --- | --- |
| 准备 | 当前手雷实例、原准备状态、有效手势采样 | 没有手势释放 token 就不调用生成；原引信是否已启动单独记录 |
| 松扳机 | 一次上升序号的释放事件，对齐姿态时间，固定身体空间代和类型 | 释放 token 只消费一次；失追不形成松键 |
| 生成前 | 合法手位、释放参考点、朝向、速度；短距离体积检查 | 起点未穿墙，原 Spawn 使用该点；碰撞失败保持一致状态，不推到障碍后 |
| 原 Spawn/Init | 调用原类，注入同一计划的位置／方向／初速 | 有效原弹体身份、初始 Location/Velocity 及缓存；生成失败不能伪造弹体或盲目再扣库存 |
| 交接 | 原 ConsumeAmmo 与 GrenadeThrownNotification | 恰好一枚弹耗、原声音／振动／DetachWeapon；动画 Notify 后到时不得再投一枚 |
| 飞行后 | 原重力、碰撞、反弹、引信和效果 | 释放事务退出，VR 不逐帧强拉弹体继续沿理想抛物线 |

`StartGrenadeThrow` 本身只涉及准备提示／flash 等，并不是实际 Spawn 点。只替换它或只隐藏原轨迹预览不改变实际投掷。`GrenadeThrownNotification` 又包含原空中动画、DetachWeapon、音效等，所以不能仅调用 Spawn 而跳过投后流程。

速度采用同一追踪空间内的释放参考点历史：

```text
p_item(t) = p_grip(t) + R_grip(t) * r_item
v_item    = 对有效短窗口的 p_item(t) 拟合
p_world   = release_body_basis * p_item(release_time)
v_world   = units_and_axes(v_item) + base_translation_velocity_once
```

现有 `grenade_throw.h` 提供最长 140 ms 拟合等基础；末端加权或设备速度可在后续手感阶段比较。手腕旋转已经体现在 p_item 历史里时不能再加一次 ω×r。若改用设备线／角速度，才按同基准 v_grip+ω×r 合成，并检查有效位。OpenXR 规范参考见[原方案中的说明](../all-weapons-interaction-plan-20261028.md)。

拟合在共同追踪空间中完成，释放时只变换一次：头转不加速度；回中、Snap 转向、会话／参考空间代改变、时间跳变清空历史，重新收集有效样本；原 bAddBaseVelocity/InitialFrameOfRefVelocity 与 VR 角色速度只选定一个合成点。角色／载具连续平移可加入，但不能再由含摇杆位移的世界历史差分重复加入。

低速释放是设计支持项，需要把原型的最低 0.5 m/s／4 cm 运动门槛与追踪有效性分离；有效静止放落应得到近零手部速度加一次基底速度。2026-11-08 用户将 Frag 改为持续右 Grip 持有、松 Grip 投出及 2 kg 虚拟重量目标，见[当前规则与实际证据](../frag-grip-release-20261108.md)。不以质量简单缩放实测速度，也不在已写速度后再施加相同冲量。

原 Proj_Grenade 静态 GravityScale=2、Speed=1500、MaxSpeed=0、LifeSpan=10；这些是引擎数据，不能直接当 SI 制世界重力、10 秒引信或速度封顶 0。视觉预览若显示，使用当前原弹体的有效重力／单位；实际运动始终由原物理推进，首次碰撞之后不继续套简单抛物线。

引信启动已查清到保存脚本：准备／旋转及 Spawn/Init 未安排爆炸 timer；不新增在手 cooking。Frag 继承 Proj_Grenade.StopBounce，停止运动后安排提示和 BeginExplosionSequence；默认 .1+1 s，Base 是 Pawn 时改 1.5+1 s，AttachInit 直接安排 2.5 s。Smoke 覆盖 StopBounce，直接 DoExplosion；其 AttachInit 直接生成附着烟雾。停止可由速度／反弹计数触发，不等于第一次落地。

两类还保留继承 HitWall 的特殊重复位置分支 BeginExplosionSequence(0,TimeAfterBeep)。BeginExplosionSequence 对模拟弹体直接返回、对已有 DoExplosion timer 防重入；DoExplosion 有 bHasExploded 门控。因此不能统一成“松手开始倒数”或“Smoke 任意碰撞都立即生效”。完整分支见[复核表](interaction-review-v2.md)。实际调用 ABI／运行时序未验证；一旦原引信启动，不因失追或重新握持刷新。StopGrenadeSpin 清准备动画不等于取消已存在弹体的引信。

## I0 — 实现组织与不重做的部分

保留现有 `weapon_hand_client.h`、`weapon_action_boundary.h` 和主线唯一 Native／游戏线程执行端。逐枪模块产生动作请求，执行端串行验证并调用原函数，返回实际状态。不能让每个武器头文件各自调用 ProcessEvent、写骨骼或启动游戏。

建议新增的薄适配文件仍为 `tools/playground/weapon_reload_adapter.inl` 和 `weapon_throw_adapter.inl`；**本次没有创建这些源码文件**。公共适配承担身份／原计时器／库存／回执；逐枪计划保留在已有专用类型内。模型姿态交给现有唯一组件／骨骼写入端，不建立第二个骨骼更新者。

配置字段应分为三组：静态来源（类、资源、哈希）、设计步骤（动作与定额）、实例校准（真实对象／骨表、轴、枢轴、行程、原窗口、状态入口）。第三组未齐时保留 `current_instance_verified=false`。离线规格索引不能被生产循环自动载入，也不生成“启用全武器”开关。

## I1 — 接入前的窄任务清单

| 项 | 具体待认证内容 | 完成后解锁 |
| --- | --- | --- |
| B01 公共换弹 | 当前状态函数解析；timer 取消／恢复；PerformReload 与 Abort 的同事务防重复；setter 参数及回执；奖励重设 timer 的处理 | W01–08 的真实库存安全完成 |
| B02 活动件 | 同一武器实例的骨表、父子基准、原动画接管恢复；固定装填口；Snub 手持原匣组件 | 机械动作与手持弹药显示 |
| B03 Gnasher | ShotgunCocking 的原 timer 与手势闭锁只完成一次；原状态退出和发射间隔并存 | 每发手动杠杆 |
| B04 Boltok | 静态支架／弹巢关系及新六孔件已交付；仍需真实实例比例／端点、原动画或替代驱动的恢复、六槽与部分装填记账 | 原部件与补件组成的逐发换弹 |
| B05 Torque Bow | C3 取消组合已静态定位；仍需参数、实际状态分派、游戏线程同序执行与未消费弹药读回 | 蓄力的失追／切枪处理 |
| B06 Hammer of Dawn | GetTraceImpact 的武器起点／aim 参数；BeamManager 身份、启用／目标条件、EndFire 完整调用 | 手指向原照射与停止 |
| B07 Troika | 炮位领取／退出参数、原枢轴限位、双枪口规则、Loader 状态 | 固定机枪双握把操作 |
| B08 两种手雷 | 两种引信静态路径已分清；仍需 Spawn 前返回值替换、Init 参数、初速／缓存时序、动画 Notify 去重和实际各分支清理 | 真实手势轨迹弹体 |
| B09 原创资产加载 | 模型已制作；仍需一件 Gnasher 的引擎加载、组件、材质、深度、GC／销毁 | 三款完整实弹及弹巢共用加载管线 |

这些是有限的绑定缺口，不是重新逆向整个游戏。后续每次原生调查继续先查现有知识库和保存证据；只有对应缺口需要时才做最小 IDA／运行观察。当前轮只完成静态与设计，不触发上述实机任务。
