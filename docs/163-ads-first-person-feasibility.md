# The feasibility of targeting the first person.

2026-10-21 User asks: If the current capability is sufficient after a common weapon is opened, except for sniper mirrors. This round has completed evidence studies; it has not been achieved, games are started, games are repackaged, default construction is modified or IDA is opened.**Existing cameras and binoculars are sufficient to support the restricted prototype; complete available experience also requires first person anchor, model visibility and shooting/shield verification.**

## The current default build must be separated from the workspace

Default list for study 1，DLL SHA 256 `befe08352b5847c5854f0d370611227eb0f8322300ae52e1e8f9c9bb50f8e77a`;in this round check actual DLL, selected source code and its HUS list.

| Capacity | There's evidence. | Meaning of this function |
| --- | --- | --- |
| The same replayed candidate eye points, HMD and left-eye matrix | Default freezing of anchovid / stereo links and historical emulator evidence | Re-enactable eye, head and rendering call associations without re-establishing VR output |
| Modify Render Point Origin | Default Freezing Source Contains `origin_trial.h`、`view_origin.h`, access before eye-to-eye matrix generation | Specific access points; old A 1/B/A 2 Diagnosis is not a product function to be switched by the point-of-fire state. |
| Read with Normal Aim Status | Current Workspace `crosshair_assignment_config.py` Parsing `bIsTargeting`，`crosshair_assignment.h` Read and carry state | Reusable to achieve direction; default frozen version at the time of study not available |
| Move camera forward | `camera_closer.h` and documents 74 Historical running evidence | Move forward only to the original camera, maximum 0.5 Rice, and keep the person in front of it.0.4 distance;not equal to first person |
| First person called Visual Reference | Document 12、18、19;old plane Mod once entered the level to get first person view | Proves that the game can obtain this view through camera/resource processing; does not prove that the current original model and VR combination are already working |

Move field local camera front-to-side configuration to 0.40 Rice. The first person calls for an independent vision-point strategy; simply raising this parameter cannot remove the shoulder deviation, or create eye spots or solution models.

## The first person in the original version called it a branch and sniper exception.

Check eight selected scripts export and index after the knowledge base has been retrieved; source is artifact 2 The depressure. `WarfareGame.u`，SHA 256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`. This is the package source, separated from disk EXE and historical run-time code snapshots. The script still contains undeconstructed native token, type and incomplete signature; the following is an exported code relationship, not a trajectory for this rotation.

- Foundation `WarWeapon.UseFirstPersonCamera` Return false, sniper cover back to yourself `bZoomed`。
- Normal aim and bunker. `HijackCamera` Each branch uses this return value to obtain the starting point of the weapon and the eye direction of the person.
- `WarPawn.GetWeaponStartTraceLocation` The same person is also consumed in the judgement that the branch involves calibration of targeting and a retrenchment of the gun. Thus, changing the return value of an ordinary weapon to a single rue would affect both the firing starting point selection.
- Sniper. `SetZoomed` Weapons are also being used to conceal disposal; sniper mirror paths cannot directly prove the existence of a usable first-person model for carrying guns.
- There's a pistol. `bZoomed`. The sniper exception must be combined with weapon type and status identification, not all zoomeds should be used as sniper mirrors or only the FOV threshold.

The restricted prototype gives priority to switching the original location where the existing rendering view is constructed, preserving the original weapon logic; sniper mirror exits this strategy and continues with the process. This is a project choice and does not mean that the current VR of the sniper mirror shows accepted.

## Actual work not available

1. **(c) A stable and appropriate anchor.** Old prototype. `Pawn.Location + EyeHeight` It's a candidate point. `GetPawnViewLocation` The export of a crotch, pelvis or parent path in different states cannot be described as the real eyebone at fixed heights, let alone directly bind all skull animations to create a point of view vibrating.
2. **Independent authentication of targeting status.** Current reader takes the status of HUD query; product switching cannot rely on a limited HUD hit point that is just as available. The certified controller/weapon/content should be associated with the rendering instrument and clearly invalidated, re-guns and uncut keys. Aim ahead to replace normal camera croser, avoiding double folding.
3. **Body, gun model and close drawing.** When the camera is near the person, it needs to physically examine head/shoulder lines, rear-cut marks, cut short and remove from the original third person; selective concealment or bone/weapons attitude adjustment may be required. There are no fully validated first person arm/machine-sight matching programs in the current tool chain.
4. **The shooting was consistent with the sight.** The use of weapons-authority sights and their sceneal intersections continues; the headline changes only observation and projection, so that the world can not be shot up. HUD query points, animation gunpoints, image centres and actual firing rays are checked separately; mobile visual points do not automatically confirm door-to-door/astrostar/blast alignment. See document. `weapon-aim-hit-chain.md` and `reticle-camera-freshness-20261020.md`。
5. **Shelters and transition.** Common/shelter targeting is different, with differences in detection, crouching, left-and-right mirror and collision. Near wall/low bunker/shore edge needs to be verified; death, drama, machine gun, menu and world switch should exit the strategy. Switching times and transitions also require physical image and head comfort verification, which cannot be directly applied to flat-screen animation.

## Prototype and authentication order

First GPU images must confirm that the same frozen anchor is used, the person is not shielded and the same world target is projected to be in the direction of the point of view. The laser/dispersion/killing connection and close wall testing is then completed.

Backstage operations are still subject to the tested behind-windows test for construction/runtime and set up the user-requested observe-only, behind and testing silent environments; do not modify or seize other game sessions. The CPU, actual games/GPU, RenderDoc, real header recognition is recorded separately. This round does not run these acceptances.

This is why it can continue to be a prototype in this direction; it cannot be used as a function of the current starter, nor can it promise full mechanical targeting by changing only one camera distance. After completing the necessary validation, preserve integrated restorations according to user preferences for the latest default version.

## Reuse of evidence

- Current round of open-ended audits: signed `analysis/ads-first-person-20261021/audit.py`、`evidence.json`, which contains a distinction between knowledge base search, eight scripts, Hash, package identity, default DLL and six frozen source files, and default/workspace capabilities.
- Default source:`analysis/integrated-fixes1-source/` and `integrated-fixes1-source-sha256.json`。
- The old eye point diagnosis. CPU results:`analysis/runtime/fp-origin1-native-ready.json`;relevant running boundaries are found in documents 56. The old CPU validates by not replacing this function with a real game.
- Camera reference and model limitations: documents 12、18、19、74; original package, script and model derivatives continue to be left in signed `analysis/`, do not enter the distribution package.

---

## 中文

# 普通瞄准切第一人称的可行性

2026-10-21。用户询问：普通武器开镜后进入第一人称，狙击镜除外，现有能力是否足够。本轮完成有界证据研究；没有实现此功能、启动游戏、改包、修改默认构建或打开 IDA。结论是：**现有相机与双目能力足以支持受限原型；完整可用体验还需要第一人称锚点、模型可见性及射击/遮挡验证。**

## 当前默认构建与工作区须分开

研究时默认清单指向 IntegratedFixes1，DLL SHA256 `befe08352b5847c5854f0d370611227eb0f8322300ae52e1e8f9c9bb50f8e77a`；本轮核对实际 DLL、所选冻结源码与其哈希清单。

| 能力 | 已有证据 | 对此功能的含义 |
| --- | --- | --- |
| 同次渲染的候选人物眼点关联、HMD 与左右眼矩阵 | 默认冻结源码的 anchor / stereo 链路及历史模拟器证据 | 可复用双眼、头动和渲染调用关联，不必重新建立 VR 输出 |
| 修改渲染视点原点 | 默认冻结源码包含 `origin_trial.h`、`view_origin.h`，在逐眼矩阵生成前接入 | 有具体实现入口；旧 A1/B/A2 诊断不是按瞄准状态切换的产品功能 |
| 普通瞄准状态读取 | 当前工作区 `crosshair_assignment_config.py` 解析 `bIsTargeting`，`crosshair_assignment.h` 读取并携带状态 | 可复用实现方向；研究时的默认冻结版本没有这个 reader，不能称已部署 |
| 镜头前移 | `camera_closer.h` 及文档74的历史运行证据 | 仅沿原相机前向移动，最多0.5米，并保留人物前方0.4米距离；不等同第一人称 |
| 第一人称视觉参考 | 文档12、18、19；旧平面 Mod 曾进入关卡得到第一人称视点 | 证明该游戏可以通过相机/资源处理得到这种视点；不证明当前原版模型与 VR 组合已通过 |

工作区本机镜头前移配置为0.40米。第一人称应有独立的视点策略；单纯提高这个参数既不能去掉肩侧偏移，也不能建立眼点或解决穿模。

## 原版第一人称分支与狙击例外

知识库检索后核对八份选定脚本导出与索引哈希；来源为 artifact 2 的解压 `WarfareGame.u`，SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`。这是包来源，与磁盘 EXE 和历史运行时代码快照分开。脚本仍含未解析 native token、类型及不完整签名；以下是导出中的代码关系，不是本轮调用轨迹。

- 基础 `WarWeapon.UseFirstPersonCamera` 返回 false，狙击枪覆盖返回自身 `bZoomed`。
- 普通瞄准和掩体瞄准的 `HijackCamera` 都有使用这个返回值的分支，再取得武器起点和人物眼部朝向。
- `WarPawn.GetWeaponStartTraceLocation` 也消费同一第一人称判断，分支涉及调整瞄准和枪口回缩。因此把普通武器的返回值统一改成 true，会同时影响射击起点选择。
- 狙击枪 `SetZoomed` 还调用武器隐藏处理；狙击镜路径不能直接证明普通枪存在可用的第一人称持枪模型。
- 手枪也有 `bZoomed`。狙击例外必须结合武器类和状态识别，不能把所有 zoomed 都当狙击镜，也不能仅用 FOV 阈值判断。

受限原型优先在已有渲染视图构造处切换原点，保留原武器逻辑；狙击镜状态退出此策略并继续原有流程。这是工程选择，不表示狙击镜现有 VR 显示已验收。

## 还缺的实际工作

1. **稳定且合适的锚点。** 旧原型的 `Pawn.Location + EyeHeight` 是候选眼点。已有 `GetPawnViewLocation` 导出在不同状态引用颈部、骨盆或父类路径，不能将固定高度称为真实眼骨，更不能直接绑定所有头骨动画造成视点抖动。需要按当前人物、姿态与武器确定稳定视点及少量偏移。
2. **瞄准状态的独立认证。** 当前 reader 随 HUD 查询取得状态；产品切换不能依赖恰好有可用的有限 HUD 命中点。应把已认证的 controller/weapon/context 状态与该次渲染票据关联，并明确失效、换枪和松键时的回退。瞄准前移替代普通 camera closer，避免叠加两次。
3. **身体、枪模与近处绘制。** 相机到人物附近后，需要实际检查头部/肩膀挡视线、枪后部穿模、近裁剪与原第三人称剔除；可能需要选择性隐藏或骨骼/武器姿态调整。当前工具链尚无已验证的完整第一人称手臂与机械瞄具对齐方案。
4. **射击与视线一致性。** 继续使用武器权威瞄准线及其场景交点；头显只改变观察和投影，不能抬头就改变武器世界瞄准。HUD 查询点、动画枪口、画面中心和实际开火射线要分别核对；移动视点不自动证明照门/准星/子弹对齐。参见文档 `weapon-aim-hit-chain.md` 与 `reticle-camera-freshness-20261020.md`。
5. **掩体与过渡。** 普通/掩体瞄准是不同模式，存在探头、蹲伏、左右镜像和碰撞差异。需要验证近墙/低掩体/掩体边缘；死亡、剧情、机枪、菜单和世界切换应退出此策略。切换时间和过渡方式也需实际图像与头显舒适度验证，不能直接照搬平面镜头动画。

## 原型与验证顺序

先选择普通步枪，在空旷站立状态验证按住瞄准进入、松开回第三人称、狙击镜排除；再覆盖手枪、霰弹枪、换枪/换弹以及头显平移和转头。第一批 GPU 图像必须确认两眼使用同一冻结锚点，人物没有遮挡，以及相同世界目标在视点切换前后的投影关系。随后补开火范围内的射线/散布/命中关联和近墙测试，再扩展掩体。

后台运行仍须使用已检查通过的 behind-windows 测试构建/runtime，并设置用户要求的 observe-only、behind 与测试静音环境；不修改或抢占其他游戏会话。CPU、实际游戏/GPU、RenderDoc、真实头显验收分别记录。本轮没有运行这些验收。

因此可以继续做这个方向的原型；还不能把它作为现有启动器里已经完成的功能，也不能承诺仅改一个相机距离即可得到完整机械瞄具体验。后续完成必要验证后，按用户既有偏好合入最新默认版本，保留已集成修复。

## 证据复用

- 本轮有界审计：ignored `analysis/ads-first-person-20261021/audit.py`、`evidence.json`，包含知识库检索、八份脚本哈希、包身份、默认 DLL 与六份冻结源文件校验、默认/工作区能力区别。
- 默认来源：`analysis/integrated-fixes1-source/` 及 `integrated-fixes1-source-sha256.json`。
- 旧眼点诊断 CPU 结果：`analysis/runtime/fp-origin1-native-ready.json`；相关运行边界见文档56。旧 CPU 通过不能替代本功能实际游戏验证。
- 相机参考与模型限制：文档12、18、19、74；原始包、脚本和模型衍生内容继续留在 ignored `analysis/`，不进入发行包。
