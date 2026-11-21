# Unified Manual Candidates 2 First person, free hands, guns, Lancer, gun.

Version **6dof-unified-20261028-2**. The fixed entrance is still `J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe`. Source code is in the Unified Working Tree `codex/6dof-unified`, actually submitted together `9a41f92` Keep both sides of history. Old initials, old body bags and historical branches are kept.

## Four-step chain.

| Order | Current code connection | Authentication status |
| --- | --- | --- |
| First | Automatically select, same eye/body base, Ref 115 Neutral, candidate for near cut | Compiled/packaged, not authentic |
| Free hands. | Full position/rotation, fixed bone length, side-by-side tracking and animated interface | Compiled/packaged, not authentic |
| Shoulder/pants rounding guns | Real RGrip Edge, original inventory switching and attachment, same-father | Compiled/packaged, not authentic |
| Lancer, fire! | Right trigger to the ground, R.2, actual Muzzle socket returns to consumers with the shot starting and base direction | Compiled/packaged, not authentic |

This edition `unified_fire_ready=true`, normal manual session allows for the assembly of a compiled gun after the device is connected. `original_aim_only` Not returning path as a uniform mode; not start new VR Rs without current valid shot/shoot snapshot 2 Keep the original entry, identity, freshness and normal cleaning restrictions.

## Use

Connect physical headlights, using the system OpenXR or VDXR. Default multiple rate **0.4**# Close the camera **0**, first person is declared open; a purchase path has been configured and does not require running scripts or separate versions of the weapon.

1. Enter a common controlled scene, observe first person's name, real head horizontal/turning, then move separately, rotate the right hand.
2. When no valid arm is recorded, the arms horizontally spread while holding both sides for at least two seconds. The right hand then press RGrip to collect the weapon in the vicinity of the shoulder/pants pocket. This round starts with fire. **Lancer**Left shoulder weapon 2 Right shoulder weapon. 1; the existing mapping of the left pocket pistol and right-pants hand grenade is maintained, but not the new version of the firing suitable.
3. Press the right trigger for a secure game scene to observe the opening/bulleting of the gun when the head is not moved, the shooter is turned and the gunman is not moving, the head is turned. Click " Stop VR" after unzipped and then exit the game; logs can be used as a starter for "Export Log".

Original log is still on the list `analysis/runtime/`, Interface log in `analysis/vr-launcher-ui/`. Session JSON `unified_weapons` Recording equipment connection/fire capable state; these are only operational and not hits or video proof of acceptance.

## Achieving borders

- The gun is read only in normal PostPlayerTick, with actually held and actually attached support for Lancer `MuzzleSocketName="Muzzle"` and type `GetSocketWorldLocationAndRotation`. Do not configure the direction of the gun pipe with HMD forward, old HUD queries or wristbone positioning.
- Current manual barrel configuration **Socket Local +X**; waits for user direction feedback; it is a clear realization configuration, not accepted bore certificates.
- The same shot freezes the starting point and direction of the same gun. `Pawn.GetWeaponStartTraceLocation → GetAdjustedAim → GetPushedForwardStartTraceLocation → CalcWeaponFire`;to be called to the physical starting point, where help is also processed in the same shot.
- Change the inner layer. `WarPawn.GetAdjustedAimFor` Base direction, retain outer layer `WarWeapon.GetAdjustedAim` . Subsequent original collisions, injuries, dispersal and effects continue; each original function is still performed once, former RAX and abnormal paths are maintained.
- Native Notifications call back only for consumption preparation POD, shot scope, type metadata and atomic life cycle; no caller caller, rereading Cliter Frame, owner/cache lock, consume VM parameters or create a second book. Return field is limited to this call 12 Byte Victor/RotatorResult; undefined parts are written to the reserve service obligation and cannot be erased through unwind.
- Independent HUD query for a quasi-star**No new rays yet.**The target star cannot be considered as evidence of alignment.
- Participant gives unknown shooting snapshots when his left hand is not tracked/unknown, and this edition will temporarily reject new firing in this state. The left hand is being followed normally and the non-handgun single status is still allowed; no one can be claimed to have supported single-handed fire after loss of his left hand.
- 0.12 m Close cut will also cut off very facey hand or scene objects. No new rounds, bunkers or other weapons are suitable for firing.

## Build and Back

Only source code checks, compiles/links and packs. No Game, XR, Simulator, Test, Self-Detecting, Relay or Receiving and Inspection prechecks are run. New PG only builds `GearsPlayground` Target, original Native/XR uses the actual translation of the package in part unchanged. Failed to maintain and amend records instead of successful translation as a substitute for real machine acceptance.

| Component | SHA 256 |
| --- | --- |
| Native | `458cdf929da1df7d6263c75f2adcac042cd7f4dbce077eb347b668e8322ba36b` |
| XR | `6db1b9bc72a42c4ef5a2b3d0fe99858c794e16f7206620ec4e25fe35ca53d727` |
| PG Gun Candidates | `80086cf51dd9b57f666ed108524f9a7069441e6e297c2f0dd6c6cca7df943f1c` |

Build log and freeze source directory ignored in working tree `analysis/6dof-unified-20261028/pg-package2-build/` and `package2-source/`. The latest Junker, menu, Focus, Independent Left Eye History, Diagnosis and Normal Discontinuation/Audience/Archive Protection is maintained.

Keeps the initial resource in use while updating, without stopping a user-made session. Backup stops and exits normally, then starts with the retained first package or old body manual/J home directory starter; do not start multiple games in parallel. The four-step code is connected, the user's real machine check is still incomplete and cannot be recorded as all playouts.

---

## 中文

# 统一手动候选 2：第一人称、自由双手、取枪、Lancer 枪口开火

版本 **6dof-unified-20261028-2**。固定入口仍是 `J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe`。源码位于统一工作树的 `codex/6dof-unified`，实际合并提交 `9a41f92` 保留两边历史。旧首包、旧身体手动包和历史分支均保留。

## 四步代码链

| 顺序 | 当前代码接线 | 验证状态 |
| --- | --- | --- |
| 第一人称 | 自动选择、同次 eye/body 基准、Ref115 中立、候选近裁剪 | 已编译／打包，未真机确认 |
| 自由双手 | 完整位置／旋转、固定骨长、逐侧跟踪和动画交接 | 已编译／打包，未真机确认 |
| 肩部／裤兜取枪 | 真实 RGrip 边沿、原库存切换与挂接、同父 held mount | 已编译／打包，未真机确认 |
| Lancer 开火 | 右扳机→原 R2，实际 Muzzle socket→同 shot 起点与基础方向返回消费者 | 已编译／打包，未真机确认 |

本版 `unified_fire_ready=true`，正常手动会话在装备连接后启用已编译的枪口消费者。原 `original_aim_only` 不作为统一模式回退路径；拿不到当前有效枪口／射击快照时不启动新的 VR R2 按下，并释放已有连发。保留原输入、身份、新鲜度和正常清理限制。

## 使用

连接物理头显，用系统 OpenXR 或 VDXR。默认倍率 **0.4**、相机拉近 **0**、第一人称开启；已配置购买版路径，不需运行脚本或另开武器版本。

1. 进入普通可控场景，观察第一人称、真实头部平移／转头，再分别移动、旋转左右手。
2. 没有有效臂展记录时，双臂水平展开，同时持续握两侧 Grip 至少两秒。之后右手在肩部／裤兜附近按 RGrip 取库存武器。本轮开火先用 **Lancer**：左肩武器 2、右肩武器 1；左裤兜手枪、右裤兜手雷的现有取物映射保留，但不是本版新开火适配对象。
3. 对安全的游戏场景目标按右扳机，分别观察“头不动、转动持枪手”和“持枪手不动、转头”时的枪口／弹着。松开扳机后点击“停止 VR”，等停止完成再退出游戏；日志可用启动器“导出日志”。

原始日志仍在候选的 `analysis/runtime/`、界面日志在 `analysis/vr-launcher-ui/`。会话 JSON 的 `unified_weapons` 记录装备连接／fire enable 状态；这些只是运行状态，不是命中或画面验收证据。

## 实现边界

- 枪口读取只在正常 PostPlayerTick，使用实际已持有、实际附着的支持 Lancer 的 `MuzzleSocketName="Muzzle"` 和类型核对后的 `GetSocketWorldLocationAndRotation`。不以 HMD forward、旧 HUD 查询或腕骨定位配置冒充枪管方向。
- 当前手动枪管配置采用 **socket 局部 +X**，等待用户方向反馈；它是明确的实现配置，不是已验收的 bore 证书。
- 同一射击冻结同一份枪口起点和方向。原链为 `Pawn.GetWeaponStartTraceLocation → GetAdjustedAim → GetPushedForwardStartTraceLocation → CalcWeaponFire`；实际调用到物理起点 helper 时也在同 shot 内处理。
- 改的是内层 `WarPawn.GetAdjustedAimFor` 基础方向，保留外层 `WarWeapon.GetAdjustedAim` 的原误差计算。后续原碰撞、伤害、散布和效果流程继续执行；每个原函数仍执行一次，原 RAX 和异常路径保留。
- Native 通知回调只消费预备 POD、已有 shot scope、类型元数据和原子生命周期；不调用 getter、重新读取 Client Frame、抢 owner/cache 锁、消费 VM 参数或创建第二个 hook。返回字段仅限本次调用的 12 字节 Vector／Rotator Result；不确定的部分写入保留事务债务，不能通过 unwind 重试抹去。
- 准星的独立 HUD 查询**尚未接到新射线**，不能把原准星当作已对齐证据。枪口轴、弹着、手感、持续射击和停止仍等用户真机反馈。
- 已有 Participant 在左手未跟踪／未知时给出 unknown 射击快照，本版会暂时拒绝这种状态下的新开火。左手正常跟踪、没有扶枪的 single 状态仍允许；不宣称左手丢失后已支持单手射击。
- 0.12m 近裁剪也会裁掉非常贴脸的手或场景物体。未新增换弹、掩体或其他武器开火适配。

## 构建与回退

只有源码检查、编译／链接和打包。没有运行 Game、XR、模拟器、测试、自测、回放或验收预检。新 PG 仅构建 `GearsPlayground` 目标，原 Native／XR 未改动部分复用首包实际编译产物。保留失败及修正记录，不以编译成功代替真机验收。

| 组件 | SHA256 |
| --- | --- |
| Native | `458cdf929da1df7d6263c75f2adcac042cd7f4dbce077eb347b668e8322ba36b` |
| XR | `6db1b9bc72a42c4ef5a2b3d0fe99858c794e16f7206620ec4e25fe35ca53d727` |
| PG 枪口候选 | `80086cf51dd9b57f666ed108524f9a7069441e6e297c2f0dd6c6cca7df943f1c` |

构建记录和冻结源码在工作树忽略目录 `analysis/6dof-unified-20261028/pg-package2-build/` 与 `package2-source/`。最新 Junker、菜单、focus、独立左眼历史、诊断和正常启停／音频／存档保护保留。

更新时保留正在使用的首包资源，不停止用户手动会话。回退先正常停止并退出，再用保留的首包启动器或旧身体手动包／J 主目录启动器；不要并行启动多份游戏。四步代码已接线，用户真机验收仍未完成，不能记成玩法全部完成。
