# GearsVR · First-Person 6DoF Weapons and Cover

This is the development handoff for the first-person VR mod. The integrated PC candidate (r8) built successfully. The 12 scoped simulator checks were run across revisions r3 through r8. The Pimax headset playtest found the issues below. Full headset acceptance remains open.

| Gameplay or feature | Working or verified | Open issues and requested changes |
| --- | --- | --- |
| Launcher and startup | The Pimax OpenXR preflight passes with automatic, aspect-preserving resolution capping. Both 1× and 2× resolve to 4096 × 3290 per eye on the tested setup. The engine's side-by-side limit is 8192 × 8192. | Verify full gameplay startup after the cap patch. The Stop button takes too long. Time game exit, XR and Native cleanup, and UI refresh separately. |
| Campaign and first-person view | The r8 simulator entered playable Campaign through the original controller A input, submitted both eye views, and shut down normally. | The first attempt crashed before mod activation and the rerun passed. Full Campaign stability and progression remain unverified. |
| Playground, pickup, and weapon switching | The simulator picked up a Lancer from the ground, drew the same weapon from the left shoulder, switched shoulder and hip slots, holstered and redrew it, and fired while moving and raising it. | On Pimax, the left shoulder yields neither weapons nor ammunition. Repeated shoulder interactions can slow or stop walking. In some runs, the VR hands and gestures disappear while thumbstick controls still work. Restarting the game restores them. Shoulder and weapon-switching grab volumes feel too small, and the player body is still visible in first person. |
| Lancer | Basic pickup, draw, aim while moving, and firing were verified in the simulator. | Full reload, grip poses, and physical headset feel still need acceptance on the current build. The playtester called one good reference “Launcher”. Confirm which weapon they meant before using it as a calibration baseline. |
| Hammerburst | The r3 simulator verified off-hand snap, two-handed movement and firing, a reload using the previous interaction, and firing again afterward. | With the arm held level, the gun pitches up about 80° on Pimax. The whole gun jumps vertically during firing. The charging handle fails and the yellow grab volume is too small to pull the magazine out. Keep only direct magazine removal and insertion. Remove charging-handle and other moving-part interactions. |
| Hammer of Dawn | The simulator verified one-handed aiming, warmup, beam activation, stopping on trigger release, and stopping when stowing while still holding the trigger. | The gun pitches up about 60° on Pimax. Compare the grip transform and local forward axis with a known-good weapon. A controller-wide calibration issue has yet to be established. |
| Boomshot | The r8 simulator fired the original projectile, snapped and seated the replacement round carrier, reloaded, and fired again. | In the headset, the left shoulder supplies nothing, the carrier is hard to grab, and the reload fails. The two-handed pose feels awkward and the weapon stays still when both hands move. |
| Snub pistol | The simulator verified switching to the original Snub, holstering it, and drawing the same instance. Basic firing was outside that run's verified scope. | Shots travel upward even with the visible muzzle level. Remove the two-handed grip, which flickers. Enlarge the magazine grab and snap volumes. The magazine is hard to grab and seat, and the gun stopped firing after roughly two attempts to chamber a round. Compare existing GrabPose captures and the workbench magazine offset, then test repeated reloads and firing. |
| Boltok revolver | The r3 simulator fired with one and two hands, reloaded using the shoulder cylinder, closed it, and fired again. | On Pimax, the revolver refuses to fire, aims about 10° high, and the cylinder could not be opened or removed. Test full, partially loaded, and empty states. Whether a full cylinder caused the reported issue remains unknown. |
| Frag grenade | The simulator verified the ready state, throws while stationary and moving, inventory deduction, and switching after the last grenade. | On Pimax, hard, soft, and upward throws all land about two to three meters away. The grenade appears upside down when drawn. Check release velocity, direction, and the in-game trajectory. Correct the draw orientation. |
| Smoke grenade | The headset playtest confirmed the draw orientation and smoke effect. | Full inventory, cancellation, and repeated draws still need coverage on this build. |
| High and low cover | The r4 simulator verified basic entry, interaction, and exit for both cover heights. Physical left and right leaning works in high cover on the headset. | Cover movement stutters and firing in low cover runs at a low frame rate. The left edge switches the gun to the left hand, and entering cover can remove the held weapon. Keep remote snap-to-cover, fast transitions between cover positions, and movement while in cover. Remove empty-hand grab gestures for peeking and automatic hand switching. Let players peek by physically leaning. Verify that the weapon stays in hand and fires from the right hand. |
| Performance and tracking | The r8 Campaign run recorded actual game stereo submissions. Performance changes are present in the code. | 120 Hz has yet to be verified. Investigate cover frame pacing and correlate the 6DoF hand dropout with controller routing, tracking status, and virtual hand publication. |
| Gnasher, Torque Bow, Troika, and Longshot | The repository contains specifications, original-game research, or partial code and tests for these weapons. | These modules were outside the verified scope of this integration pass. Check the individual module records before relying on them. |
| Quest Workbench | Version 0.4.0 (version code 13) was installed and verified by APK readback. It supports model and pose authoring for 12 weapons, with a dedicated Lancer practice mode. | Post-install headset testing and full configuration import into the PC mod remain open. Use existing physical pose captures as references after checking the weapon, part, and hand. |

Start with the 6DoF hand dropout and shoulder failures because they affect every weapon. When investigating aim, check the visible weapon orientation, projectile direction, and controller pose separately. For each follow-up, record the build, headset, reproduction steps, observed result, and timestamp in the logs.

[Detailed handoff and local evidence index (Chinese)](gearsvr-handoff-20261121.md) · [Weapon specifications](weapon-specs-20261028/weapon-cards.md)

### Quest hand-pose capture APK

[Download GearsVR Quest Workbench 0.5.3-dev (build 17)](releases/quest-workbench/GearsVR-Quest-Workbench-0.5.3-build17.apk). This is the standalone Quest app for capturing hand poses. It uses Android package `com.gearsvr.workbench`. The APK passed signature and 16 KiB alignment checks. Headset testing of this exact build remains open.

SHA-256: `b5887d27a3c47131a68d00f8ab9cafdb322314a2000a37ece3a9c1f61b4d1e6f`.


---

## 中文 · 第一人称 6DoF 武器与掩体交接

这是第一人称 VR Mod 的接手快照。PC 整合版 r8 已编译，集成验收队列共 12 项，覆盖 r3 至 r8 的模拟器用例。真机反馈来自 Pimax 实际游玩，完整真机验收仍未通过。以下按玩家操作列出已有证据和未完成项。

| 玩家在做什么 | 已完成或已验证 | 未完成、故障或要求 |
| --- | --- | --- |
| 打开 Launcher、启动或停止 | 本机 Pimax 分辨率预检通过。超限等比缩小，1× 与 2× 均限至单眼 4096 × 3290。引擎双眼并排上限 8192 × 8192。 | 限幅补丁后的完整进游戏待验。点击停止后退出慢，需拆分记录游戏、XR、Native 和界面等待时间。 |
| 进战役、进入第一人称 | r8 模拟器以原版 A 输入进入可操作战役，实际提交双眼画面，正常退出。 | 首次曾在 Mod 激活前崩溃，随后复测通过。完整战役流程和稳定性待验。 |
| Playground 走动、转身、捡枪、收枪、换枪 | Lancer 模拟器通过地面拾取入库、左肩取同一把枪、肩槽与腰槽切换、收枪再取、移动举枪与射击。 | Pimax 真机左肩取不到枪和弹；多次肩部交互后有时走路极慢或停住，或 6DoF 手消失、手势及换弹失效，摇杆仍可用，重启游戏恢复。肩部和换枪触发范围偏小，第一人称身体仍可见。 |
| Lancer | 已有基础取用、移动举枪和实弹射击证据。 | 完整换弹、所有抓握姿势与真机手感仍需按现行构建验收。用户报“Launcher”角度良好，所指武器待核对。 |
| Hammerburst 兽族步枪 | r3 模拟器通过副手吸附、双持移动与射击、原换弹和换弹后续射。 | 真机水平手臂时约上翘 80°，射击整枪上下跳，拉柄无效、黄色 Volume 太小、拔不出弹匣。按新需求只做拔出和插入弹匣，移除拉栓及其他活动部件交互。 |
| Hammer of Dawn 黎明之锤 | 模拟器通过单手瞄准、预热、出光柱、松扣停止与按住扳机收枪停止。 | 真机枪身约上翘 60°。需和正常武器对照挂点与局部前向轴，Pimax 通用问题尚未证实。 |
| Boomshot 榴弹发射器 | r8 模拟器用真实武器实例完成首发、装填件吸附与就位、再次射击。 | 真机左肩拿不到补给、装填件抓不住、换弹失败；双持姿势别扭，双手移动时枪不跟随。 |
| Snub 普通手枪 | 模拟器通过切换到原版 Snub、收枪和重新取出同一把枪。本轮模拟器没有验证普通射击。 | 真机枪口平时子弹向天上打。双持闪烁，按新要求取消。弹匣抓握及吸附区域太小、装填困难，约上膛两次后不能开火。复核工作台的弹匣相对位置、已有 GrabPose，验证连续换弹后仍可射击。 |
| Boltok 左轮 | r3 模拟器通过单手、双手射击，以及肩部取弹巢装填、闭合后继续射击。 | 真机无法开火，枪口约上翘 10°，弹巢打不开或取不出，未能完成换弹。满弹是否相关待查，需分别测满弹、部分装填、空仓。 |
| Frag 破片手雷 | 模拟器通过准备、静止和移动投放、原库存扣减及耗尽切换。 | 真机无论快慢或朝上投都约落在两三米，取出时柄和弹体上下反了。需核对释放方向、速度和轨迹。 |
| Smoke 烟雾手雷 | 真机确认取出朝向和烟雾效果正常。 | 完整库存、取消和反复取用没有本轮完整证据。 |
| 高低掩体 | r4 模拟器通过两种掩体基础进入、部分交互和退出。真机高掩体物理左右探头可用。 | 真机掩体内移动及射击卡顿，左边缘切左手，入掩体可能丢失持枪。新版只保留远程吸附进入、掩体间快速移动和掩体内移动。删除空手抓触发的上下左右探头及自动换手，保留身体实际探头，并验证武器在手与右手射击。 |
| 帧率与跟踪 | r8 有实际战役双眼提交证据，性能工作已有实现。 | 120 Hz 未验收。追查掩体低帧率及 6DoF 手消失时的输入路由、跟踪与发布状态。 |
| Gnasher、Torque Bow、Troika、Longshot | 仓库有规格、原版调查或部分实现和测试。 | 未列入本轮整合验收通过范围。按模块资料确认接手状态。 |
| Quest 工作台 | 0.4.0/code13 已安装并回读校验；支持 12 件武器的模型和姿势相关配置，Lancer 有专用练习。 | 安装后运行验收及与 PC Mod 的完整配置导入仍待验证。已有真实采集可用于对照，须核对武器、部件和手别。 |

建议先复现影响所有武器交互的 6DoF 丢手和肩部失效，再回归各武器与掩体。处理枪口问题时分别核对可见枪身朝向、实际射击方向和头显手柄姿态。真机验收记录应包含构建、设备、操作步骤、实际结果及日志时间点。

[详细交接与本机证据索引](gearsvr-handoff-20261121.md) · [各武器规格](weapon-specs-20261028/weapon-cards.md)

### Quest 手势采集 APK

[下载 GearsVR Quest Workbench 0.5.3-dev（build 17）](releases/quest-workbench/GearsVR-Quest-Workbench-0.5.3-build17.apk)。这是独立的 Quest 手势采集应用，包名为 `com.gearsvr.workbench`。APK 已通过签名和 16 KiB 对齐检查，build 17 的头显运行验收仍待完成。

SHA-256：`b5887d27a3c47131a68d00f8ab9cafdb322314a2000a37ece3a9c1f61b4d1e6f`。
