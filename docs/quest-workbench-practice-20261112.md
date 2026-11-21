# Quest workbench 0.3.0 controller practice and left-shoulder supply

Owner scope: remove runtime motion recording/replay, move the component inspector farther away,
and create an edit → controller practice → edit loop for Lancer acquisition, GrabPose, regions,
snap, magazine removal/replacement, left-shoulder supply, charging, C starter and sights/firing.
The later direct request explicitly includes left-shoulder ammunition pickup. This supersedes
the previous instruction to leave the0.2.0 workbench unchanged; it does not authorize PC game runs.

Source: `J:/codexWorktree/gearsVr/tools/quest-workbench`, branch `codex/6dof-single`.
No helpers, original game, PC XR/simulator or shared Mod runtime writer/build was used.
No public package/tag or game maturity/headset acceptance changed.

## Delivery

- Private candidate: `J:/gearsVr/candidates/quest-workbench-20261111/Quest Workbench.exe`.
- APK: `GearsVR-Quest-Workbench.apk`,0.3.0-dev, code9,16434539 bytes.
- SHA256: `d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c`.
- Build: `J:/gearsVr/analysis/quest-workbench-20261112/apk-build-v9-final`.
- Rollback: candidate `rollback/0.2.0-build8`, original APK75d28bb3…e8c1ff.
- Current delivery status: compiled/signed/aligned16K/packaged; **not installed or run on Quest**.
  ADB returned no devices during this change. A concise reconnection question is pending.
  Do not describe0.2.0 device receipts or historical UI PNGs as0.3.0 validation.
- Review/source hashes: `analysis/quest-workbench-20261112/delivery-v030-review.json`
  and `quest-source-v030-manifest.json`. Full Chinese instructions accompany the candidate.

## Implemented behavior

`practice.h` adapts the same portable Mod geometry/state kernels through a frozen36-header
dependency snapshot in `game_rules/`; each source hash is in `assets/game-rules.json`.
The snapshot is a defined source version, not a promise of every later private Mod change.
Source game modules, strict kernels and active game test artifacts were not edited.

Right squeeze acquires the main grip and right trigger fires into a10m target. Main-hand
acquisition uses the existing35-degree authored wrist mount; free left/right use20-degree
calibrations. Startup-held controls cannot acquire or fire, and menu aiming suppresses firing
until a real trigger release. Focus/tracking/space/time discontinuities clear ownership.
Controller grips require valid AND tracked position/orientation flags15. The gun, not HMD,
owns shot direction and the shared25m sight zero. Acquisition transition blends only on grab;
ordinary tracking is direct after it completes, without continuous smoothing latency.

Left squeeze or trigger selects the nearest admissible part. Support uses the shared two-hand
solver and the configured release boundary. Magazine extraction must observe the real4.5cm
outward stroke; the same old item can be restored. After removal, the left shoulder uses the
shared body-slot default to grant a replacement from the same pure inventory model. Insertion
uses the shared fitting/6cm guide/2cm real approach/180ms seat policy. Empty replacement requires
a subsequent E full pull and observed release/return before firing. A short12cm E pull cannot
finish; the original long slot and bounded10cm endpoint assistance remain. E waiting-open and
return gates consume actual sandbox render acknowledgments, not elapsed time alone.

C uses the shared local rail,8cm stroke,98% endpoint, real motion/minimum hold and observed
part constraints. The motor can remain held indefinitely; reverse stops it, and first actual
release starts a3s tail. It cannot start from stationary proximity or stale presses. Original
engine acknowledgment fields in this standalone adapter explicitly describe simulated scene
side effects. They are not game execution/actor proof.

Shoulder center and size are visible in its setup page and optional practice hints. The default
body frame preserves workspace/initial yaw while translating with the head; it does not rotate
the shoulder region when only the head turns. A real left controller can place the supply
center, and sliders adjust XYZ/scale. Q3 supplies no torso tracker, so this remains a body
position proxy and authored armspan layout; B/Y recenters after turning.

The inspector defaults pinned60cm to the side,30cm above and25cm farther behind the selected
anchor, with45..120cm adjustable side distance. The user can fix/follow or close it. Practice
hides large editor cards. A/X returns to editing; pressing enter again preserves scene ammo
and mechanism state, while30/360 and0/360 reset options begin explicit fixtures. Previewed
library poses can be adopted and tested directly. Authored defaults are labelled游戏默认;
real captures and their provenance remain preserved.

The original bone8 magazine is a separate mesh. Geometry sum remains13397 triangles:
body10798, receiver1300, C1089, E54, magazine156. Full Lancer.glb stays identical to0.2.0.
Moving-part capture transforms now include arbitrary part deltas, C angle/pull, E travel and
detached magazine position. Legacy scalar captures remain readable. The frozen legacy
`adopt_lancer_captures.py` was preserved; new0.3 compatibility is isolated in
`adopt_moving_lancer_captures.py`, leaving existing source-adoption gates unchanged.

Runtime Recorder/history/replay is removed from Android and host build targets. Archive
compatibility codecs remain unused by the app; existing old.take files are not deleted.
Single real hand capture, calibration, independent archives and pose editing remain. No
synthetic26-joint samples or source timestamps are created from controller/preview hands.

## Verification and practical limits

- Android arm64 NativeActivity/GLES/AAudio compiled without warnings in final compile log;
  signing, manifest version9 and16K library/zip alignment verified.
- Windows companion compiled. APK packaged asset hashes and36 frozen header hashes verified.
- Host core checks passed: bases, real-capture flags, shape/snap math, fixed finger lengths,
  input edges, controller object following, selectors/rings and independent atomic archives.
- New host practice trajectories passed: cold presses, shots/ammo, menu suppression, stale
  input/epoch/tracking withdrawal, support release and volume rejection, stable shoulder,
  tactical and empty shoulder replacement→insertion→charge→fire, old magazine restore,
  short/full/render-waited E, C no-gesture/no-render denial, sustained motor, release tail,
  reverse stop/restart and tracking-loss stop. These are CPU fixtures, not real tracking.
- Four existing Python editor/library conversion tests passed. Four retained actual0.2.0
  left snapshots round-tripped through arbitrary moving-part fixture deltas, worst position
  error7.11e-15 original units and rotation-matrix error4.45e-16. Those rebases are explicitly
  offline fixtures, not newly captured poses. See check-practice-capture-conversion.json.
- Two concrete adapter failures were reproduced/fixed: exact seated floating-point endpoint
  could start slightly negative, blocking extraction; E attempted move on its begin sequence
  and immediately cancelled. A test expectation about inventory reserve was corrected to the
  shared model's returned-old-rounds/reserved-replacement semantics. No kernel was relaxed.

Cadence, visual recoil and synthesized audio are labelled previews and editable. The original
game engine, original audio, animation, physics/mass, recoil, enemies and executions are absent.
The app cannot establish complete game feel parity, collision-free surface fitting or physical
comfort by itself. Main/body full-arm IK is not run here. Rendering retains1.15 recommended
eye dimensions and2xMSAA, but new-device clarity/performance remain unobserved.

Full projects save wrist/fingers, region/snap/shoulder/practice settings and panel placement.
Desktop V2 export remains fingers-only. The existing separate source-adoption flow handles
full wrists/fingers; new region/snap/shoulder/feedback parameters are **not automatically
consumed by the Mod**. Later explicit game-side configuration integration and a real-game
comparison remain required. Do not infer full Lancer or original C defect acceptance from this
standalone workbench. No Git commit was requested or made for this turn.


## Installation follow-up (2026-11-12)

Owner reconnected Quest2G0YC1ZF9H0MWD and authorized installation.13 external application files
(22733604 bytes) backed up to analysis/quest-workbench-20261112/pre-v9-device-backup.
Coverage installation succeeded; package reports0.3.0-dev/code9. Installed APK pulled back and
SHA256 exactly matches d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c.
PID4922 initialized Touch actions, simultaneous input, both1931x2023 eye targets and AAudio;
ready logged, OpenXR stayed IDLE/state1 and Android reported Asleep. No projection/frame/GL
receipt or controller practice action observed. Owner explicitly chose to keep installation and
try later. Stop further launches/checks; do not treat initialization as headset acceptance.


## 2026-11-12 Quest工作台反馈与后续游戏接入

玩家认可工作台整体与C区域进入展开/离开收起，要求补齐运行结束后的滑柄退回和收起。工作台0.3.1已修复并安装，游戏侧没有修改；核心便携规则同源，但接入层及原引擎反馈未整体等效，不能声称手感几乎一致。后续负责Lancer的会话请先读 [工作台交接](lancer-workbench-handoff-20261112.md)，核对现有C恢复路径与配置消费者；保留已有严格保护及历史失败，未做游戏验收。


## 2026-11-12 Quest Touch水平握持枪口上翘反馈 / 0.3.2

玩家再次报告手水平时枪口约上翘45度，并提醒Mod阶段已提过相同问题。核对冻结
weapon_lancer_wrist_mount.h，35度配置明确针对旧PSVR Aim/Grip观察，不是通用Quest
物理标定。不能把35度直接称为此次45度的实测因果：离线Grip中立计算反而给出向下35度，
实际Touch Grip与物理手水平之间的角度未采样。本次修复的是不适用的方向接入方式。

工作台0.3.2/code11持枪时改用真实Touch Aim方向确定枪朝向，Grip位置/真实侧握和追踪
仍负责位置及获取；手腕由枪和当前主握GrabPose反解，并保持原掌心锚点。手腕/手指
修正不会再额外改变枪口方向。双手护木仍可参与已有方向求解，HMD不控制瞄准。
Aim与Grip均要求15有效/追踪标志；已用Aim获取后丢失Aim会释放，重新握持需新按键。
不使用HMD水平锁枪，不盲加固定-45度，不修改原始XR数据或游戏侧PSVR标定。

CPU回归通过水平、上下俯仰、不一致Grip俯仰以及主手腕修正情况下枪方向与Aim一致；
已有换弹/C/E/输入流程检查通过。Android编译/签名/16K对齐通过。尚无新版实机确认，
当前USB无Q3，安装连接问题已提出；最后安装仍0.3.1/code10。

后续Lancer会话应按实际输入设备区分Aim/Grip与握姿标定，保留既有PSVR证据；不要直接
删除游戏35度配置或声称工作台与游戏完全等效。此改动仅工作台。


## 2026-11-12 连续满匣拔出与肩部供弹 / 0.3.3

玩家报告拔出弹匣后不消失，背后无法取新匣。代码复现：已装满60发后再拔匣，
ReloadModel.begin返回full，工作台仍把外观设为移除，但库存事务停在idle，肩部只
允许feed_port，故第二轮卡住。另一个明确遗漏是松手后的旧匣一直留在场景。

0.3.3/code12仅修复独立工作台接入：拔出满匣/无备弹分支将实际旧匣弹量记录保存，
再以移除后的弹池开启本地换弹事务；原匣仍握着插回时恢复原数量，新匣从同一总弹池
领取，不额外造弹。总弹池为零时仍不能凭空供弹。底层共享ReloadModel没有放宽或修改，
不代表游戏侧库存提交已经验证。原匣在手中可见、可插回；松手后隐藏并释放部件占用，
肩部重新按键可领新匣。隐藏旧匣不再保留可交互的残留模型。

CPU新增连续三轮拔匣→松手隐藏→左肩取新匣→插满匣→再次拔出通过，已有旧匣归还、
空仓拉栓、C和Touch Aim方向检查亦通过。APK编译、签名/16K对齐通过。
包含0.3.1 C复位和0.3.2 Touch角度修复。当前USB无设备，尚未安装，最后实装仍code10。
后续游戏侧会话应核对原游戏满匣检查与正常取新匣的事务策略，不盲套本地reset/prime。
