# Lancer Debugging the table to interface with game side access

## Player Feedback and Current Behaviour (Performance)2026-11-12）

Player Recognition 0.3.0 The table is generally easy to use and is clearly recognized: hand-held C-handing Volume automatically spreads and leaves the automatic collection.
Players report that push is initiated, the slides are not returned automatically after the saw operation and the handle cannot be retrieved.

0.3.1-dev/build 10 Repair of the platform access layer: end of three seconds after release, unheld again
C-Slipboard 0.32 m/s returns zero process (8 And the whole trip.0.25 seconds; keep the roll-out until back, zero and press
The area approach is determined to be active/released. Hands remain in the area and are collected after leaving.
Releases are also returned. Maintaining the endpoint after start-up is real; not changing to hold ends in three seconds.
When holding a true hold again, the player does not automatically pull and the original new button, real motion and observation threshold is maintained.

Reason: Old adapter only removed electrical power from operation, not restored c_pull_; deployment conditions include c_pull_>0，
It causes slipsticks and spread residue.tools/quest-workbench/practice.h Not the side fix.

## A description of Lancer side session

1. Use this document as a follow-up access requirement and player feedback, not to use the workstation as a game.
2. Check whether existing side C recovery paths have achieved the same behaviour before addressing actual differences; do not directly cover
   Keep the original single-author of the game.
3. Player-accredited workflow: Editor of the GrabPose/Volume/Snap Hand handle test A/X edits continue to be tested.
   Left shoulder ammunition, old boxes, new boxes inserted and air silo E fired have all been accessed into the stand.
4. Newly adjusted complete parameters such as Volume/Snap/Hairspace are stored in project.json, model project copy and
   Intaction-settings.json; no automatic sync to Mod at present. The side of the game should clearly edit configuration, coordinates
   With units, current weapons/rigs and actual consumers of fields to avoid importing fingers but claiming full access.
5. There are four real left-hand positions.analysis/quest-workbench-20261112/owner-v020-captures。
   Counterpart 1794418169059-1、magazine 1794418220181-2、E1794418385098-3、
   C1794418426664-4; the right-hand owner does not hold the batch. Keep each other's evidence of capture.0.3 New with part_delta_known
   Documents must be frozen by going atopt_moving_lancer_captures.py; old adopt_lancer_captures.py.

## Consistency with the game: Same core, not yet equivalent to the whole

| Part | The relationship between the desk and Mod | What needs to be compared |
| --- | --- | --- |
| Pulling boxes, ammunition services | Compile pure logic/geometry rules for the same fixed snapshot | Original game inventory submission, animation and physical component observation |
| E-string. | Same process, end-end support, real action and observation backshow rules | Original game bone/animation consumer and time sequence |
| C-saw. | Same core orbit and start determination; continuous holding/release tails connected by adapter | Automatic re-entry, original engine status, execution, sound painting |
| Hands, wrists, machine. | Reuse port solver, template and 25 Zero. | The original game full arm IK, role posture, real time gun and full skin |
| Left shoulder. | Same default body slot layout | Head transfer agent, not real torso tracking; custom settings are not automatically imported |
| Speed, recoil, sound, vibration. | The desktop is pretentious | Full feedback from unrecovered game engine |

Fixed Snapshot 36 I'll see you at the exact Hashi.tools/quest-workbench/assets/game-rules.json. This is making the
The source version of the APK does not mean that all subsequent private Mod changes will automatically enter the workstation.
Almost identical to the game: The same core rules do not equal input mapping, access status machines, rendering feedback and engine behaviour.
Align. This omission is the actual reverse of access differences.
The actions are briefly compared at the workstation/playspace, recording differences and acceptances.

## Restoration of validation and delivery

The CPU process check has been passed and new validations added: continuous holding after start-up, end of loose ends,
Android compiles and signs
and 16 K-System passes. It is not a live/outcast or closes the game to its current C problem.

APK0.3.1-dev/code10，SHA256
b79c3690582a2f9474ac47a503b581f6085b1946a43bbad54b4cc31b104aee0b。
Build Record analysis/quest-workbench-20261112/apk-build-v10-saw-return。
This game is not started, Mod module is not changed, and no other sessions are sent.


## 2026-11-12 Quest Touch holds a gun-snatch feedback/ 0.3.2

The player is about to lose his gun when he reports it again.45 Check the freeze.
weapon_lancer_wrist_mount.h，35 The configuration is specific to the old PSVR Aim/Grip observation, not GeneralQuest
Physically. Can't put 35 This time, I'm calling it.45 The real cause and effect of the degree: the offline Grip neutral calculation gives a downwards view.35 Dou,
The angle between the actual Touch Grip and the physical manual level is not sampled.

Workstation 0.3.2/code 11 Use the real Touch Aim to determine direction, Grip position/real side grip and track.
Still responsible for location and acquisition; wrists are decompositioned by the gun and current master GrabPose, with an anchor in the original palm.
The correction will not change the direction of the gun.
Both Aim and Grip request 15 Effective/tracking sign(s); loss of Aim after acquisition with Aim releases and reholds the required new button.
Do not use HMD-level locker, do not blindly fix -45 degrees, without modifying original XR data or the PSVR-side tagging.

(a) The direction of the gun is consistent with that of Aim in cases where CPU returns by horizontal, up and down, inconsistent Grip overtows and modification of the main wrist;
The reloading/C/E/Input process check has been passed. Android compiles/signs/16 K-ray is passed. No new version of the machine has been confirmed, and the government has not yet made a decision on the issue.
USB currently has no queen 3, the installation connection issue has been raised; final installation still 0.3.1/code 10。

Follow-up Lancer session should distinguish between Aim/Grip and postures according to actual input device, retaining existing PSVR evidence; not direct
Remove Game 35 Configure or claim that the table is exactly the same as the game. This change is only for the table.


## 2026-11-12 Remove and shoulder loads from the box/ 0.3.3

Player reports that the cartridge is not removed and no new box can be retrieved behind it. Code recurrence: full 60 The first one is the "Standing" of the video.
ReloadModel.begin returned to full, and the workstation still removed the appearance but stocking was stopped at idle with a shoulder only
Allows feed_port to hold the second round. Another explicit omission is that the old box remains on the scene after it has been released.

0.3.3/code 12 The only way to repair access is by repairing the independent desk: removing a full/unprepared branch will keep an actual record of old cartridge loads, and then leaving the data open.
Local exchange services are opened with removed cartridges; original boxes revert to the original number when they remain in hold and plugged back, new boxes from the same general pool
The base share of ReloadModel is not relaxed or modified.
This does not mean that the side of the game is already being verified. The box is visible in your hand and can be plugged back; it is hidden and the parts are taken off.
Shoulder re-presses to get a new box. Hides the old box without retaining interactive residual models.

CPU added three consecutive unloaded rims, hidden by the left shoulder, and re-plugged through the box.
The AAK compiles, signs/16 K-System through.
Organisation 0.3.1 C-Role and 0.3.2 Touch angle repair. Currently, USB is not equipped and has not been installed and is still in actual condition 10。
The subsequent game side session should check the transaction policy of the original game full-box and normal pick-up of the new box, without blinding local reset/prime.

---

## 中文

# Lancer 工作台调试与游戏侧接入交接

## 玩家反馈与当前行为（2026-11-12）

玩家认可0.3.0工作台整体易用性，并明确认可：手进入C握柄Volume自动展开，离开自动收起。
玩家报告：推到底启动，电锯运行结束后滑柄没有自动退回、握柄不能收回。

0.3.1-dev/build10修复工作台接入层：释放后的三秒运行尾段结束，未被再次握住的
C滑柄以0.32m/s退回零行程（8cm全行程约0.25秒）；退回期间先保持展开，归零后按
区域接近状态决定展开/收起。手仍在区域内保持展开，离开后收起。未启动的半程操作
松手也会退回。持续真实握住启动后的端点仍维持电机；没有改成握住也强制三秒结束。
再次真实握持时不自动替玩家完成推拉，原新按键、真实运动及观察门槛保持。

原因：旧适配器只清除了电机运行状态，没有恢复c_pull_；部署条件又包含c_pull_>0，
导致滑柄和展开状态残留。修改在tools/quest-workbench/practice.h，不是游戏侧修复。

## 给负责Lancer游戏侧会话的说明

1. 将本文件作为后续接入需求和玩家反馈，不要把工作台通过当成游戏通过。
2. 先核对现有游戏侧C恢复路径是否已经实现同等行为，再处理实际差异；不要直接覆盖
   最新私有C消费者、原恢复原因记录或原有身份/时效/释放保护。保留游戏原有单一写者。
3. 玩家认可的工作流：编辑GrabPose/Volume/Snap→手柄测试→A/X回编辑→继续测试。
   左肩供弹、拔旧匣、新匣插入、空仓拉E后射击均已接入独立工作台。
4. 新调的Volume/Snap/肩区等完整参数保存在project.json、模型项目副本和
   interaction-settings.json；目前没有自动同步进Mod。游戏侧应明确版本化配置、坐标
   与单位、当前武器/rig校验，以及各字段实际消费者，避免只导入手指却声称全量接入。
5. 已有四份真实左手姿势在analysis/quest-workbench-20261112/owner-v020-captures。
   对应support1794418169059-1、magazine1794418220181-2、E1794418385098-3、
   C1794418426664-4；右主握不在该批。保留各自捕获证据。0.3含part_delta_known的新
   文件须走adopt_moving_lancer_captures.py；旧adopt_lancer_captures.py保持冻结。

## 与游戏的一致性：同核，尚未整体等效

| 部分 | 工作台与Mod的关系 | 仍需对照的内容 |
| --- | --- | --- |
| 拔插匣、弹药事务 | 编译同一固定快照的纯逻辑/几何规则 | 原游戏库存提交、动画和实际部件观察 |
| E拉栓 | 同一行程、末端辅助、真实动作与观察回位规则 | 原游戏骨骼/动画消费者和时序 |
| C电锯 | 同一核心轨道与启动判定；持续握持/释放尾段由适配器衔接 | 自动复位接入、原引擎状态、处决、声画 |
| 双手、手腕标定、机瞄 | 复用便携求解器、模板和25m归零 | 原游戏整臂IK、角色姿态、实时枪口和完整皮面 |
| 左肩供弹 | 同一默认身体槽位布局 | 头部位移代理身体，不是真实躯干追踪；自定义设置未自动导入 |
| 射速、后坐力、音效、振动 | 工作台可调预览 | 未复现原游戏引擎完整反馈 |

固定快照36个头的精确哈希见tools/quest-workbench/assets/game-rules.json。这是制作该
APK时的源码版本，不代表后续所有私有Mod修改会自动进入工作台。不能宣称手感已经
与游戏几乎一致：核心规则相同不等于输入映射、接入状态机、渲染反馈和引擎行为全部
一致。本次遗漏就是接入差异的实际反例。后续应先对齐配置消费者，再由玩家用相同
动作在工作台/游戏各做一次简短对照，分别记录差异和接受结果。

## 修复验证与交付

已有工作台CPU流程检查通过，并新增验证：启动后持续握持不超时、松手尾段结束、
滑柄归零、区域外折叠、重新进入仅展开但不启动、再次离开折叠。Android编译、签名
和16K对齐通过。不是实机动作/外观验收，也不关闭游戏现有C问题。

APK0.3.1-dev/code10，SHA256
b79c3690582a2f9474ac47a503b581f6085b1946a43bbad54b4cc31b104aee0b。
构建记录analysis/quest-workbench-20261112/apk-build-v10-saw-return。
本次未启动游戏、未改Mod模块、未派遣其他会话。玩家将自行让负责Lancer的会话接入。


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
