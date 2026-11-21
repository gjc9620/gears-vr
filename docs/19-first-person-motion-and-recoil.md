# First person, Mod: shake, recoil and action lens.

2026-10-12. Continue to read-only check installed First Person based on user feedback 1.0 The current round does not repackage, inject or initiate firing tests and VR development targets are still suspended.

## Conclusion and strength of evidence

Not to be referred to in general terms as "dismissed gun-stave" / no recoil." The data on the package confirm that: weapons and Marcus body grids are added `bIgnoreControllers=1`, and the shooting lens vibrating, hand and spinal recoil parameters, four gun recoil camera animations and related script functions were not modified.

**The most valid inference.**: Neglecting bone control through grid layers may inhibit the procedural recoil performance of guns and arms. It is a broader control layer change that could affect other bones controls. There is no proof at this time that this setting is being executed in all actions, nor does it confirm whether it is covered by games running, so extrapolation is not written as a run validation result.

UE of Epic 3 Document describes the arm / bone of a gun used to drive a fire, followed by IK. This explains why model reflexes and observation lens vibrations can be separated:[Using Skeletal Controllers — Recoil](https://docs.unrealengine.com/udk/Three/UsingSkeletalControllers.html#Recoil). The local inverted version of the game is confirmed `WarSkelCtrl_Recoil extends GameSkelCtrl_Recoil`。`bIgnoreControllers` The specific skip branch is not yet located in the current game original code, and field meanings and the effect of this setting still need to be run.

| Effects seen by users | This round of judgment |
| --- | --- |
| Guns and arms are not wobbled. | Directly related grid control settings exist; most likely affected by them, but no switch cross-checking |
| The whole picture vibrates when you shoot. | Arguments, lens animations, related changers and scripts are kept; cancellation cannot be declared |
| The back-sitting effect is on the star or direction of the aim. | recoil parameters and settings retained, no recoilless rewriting observed |
| The bullet is off the star. | Aim for error parameters and functions, keep. No scattered zeros observed |
| Gunfire / Flash | WarWeapon Add bDynamicMuzzleFlashes=true, not to be interpreted as turning off the gun fire |
| Run / Scrambling Scrambling and Additional Offset | Multigroup offset zero, significant increase in the speed of plug-in; relevant changes are made |

## Retained shooting parameters

`WarfareGame.Default__WarWeapon` The following full attribute values are identical:

- `FireCameraShake`_Other Organiser 1 seconds;rotation ranges `[100,100,200]`, frequency `[10,10,25]`;position ranges `[0,3,5]`;FOV range 2. This is the base class parameter and cannot replace the final entry-in parameters or game settings for each weapon.
- `Recoil_Hand`: Continuation 0.33 seconds;rotation ranges `[1500,200,0]`, range `[-4,0,1]`。
- `Recoil_Spine`: Continuation 0.67 seconds;rotation ranges `[750,100,0]`。
- `PRM_Recoil`、`PRM_MinAimError`、`PRM_MaxAimError` ; Undecoded arrays are not considered to be actual final values.
- `MeleeImpactCamShake` and `bDoMeleeDamageShakes` Same thing.

`WarWeapon` Default object actually only adds `bDynamicMuzzleFlashes=true` and `bOnlyOwnerSee=false`. This round is filtered 20 Among the weapons default objects, and 19 The byte is consistent and includes specific weapons such as rifles, COG pistols, shotguns, sniper guns, etc.

Scrutinised by Recoil / FireViewShake / AimError / Spread / ScreenShake 21 The full byte alignment of the functions includes GetWeaponRecoil, SetWeaponRecoil, GetWeaponAimErrorRange, GetPlayerAimError, GetAimError, GetFireViewShake, WarInventoryManager.SetWeaponRecoil, and the addition, updating, application function of the camera vibrator. The filter range is the corresponding name in the package and does not represent the interpretation of each original function.

Two camera modifyrs default objects are also bytes consistent with three WarSkelCtrl_Recoil objects.

## Bones Control Settings

Add the following two objects in a visible form:

```text
WarfareGame.Default__Pawn_COGMarcus.WarPawnMesh
WarfareGame.Default__WarWeapon.WeaponMesh

bIgnoreControllers = 1                  // 序列化为 IntProperty，不是 bool 标签
bIgnoreControllersWhenNotRendered = true
```

The second relates only to control handling when it is not rendered, and cannot explain the shaking of visible guns on the screen alone. The first is this important clue. The object does not have two new fields for visual serialization, which directly equates " no label " with a proven running time zero.

The normal bone animation, bone control, observation lens, and game direction are different layers. Ignore bone control does not mean that all animations will cease or that the weapon's trajectory will be altered. The observed visual stability may also be affected by high FOV and motion-changed camera at the same time.

## What exactly did the camera do?

Read All within AnimSet_CameraMovation 30 A sequence of SequenceName,13 I'm not sure.17 is exactly the same. Only full bytes/Hashi comparison of the original tail is made, without assuming a compressed curve format.

**Four recoil-based animations are identical:**

`Camera_Recoil_AssaultRifle`、`Camera_Recoil_Pistol`、`Camera_Recoil_Shotgun`、`Camera_Recoil_Sniper`。

`Camera_Standing_Idle`、`Camera_Walk`、`Camera_Run_Fwd`、`Camera_RoadieRun`、`Camera_Cov_Walk`、`Camera_Cov_Run`、`Camera_Cover` It's also completely consistent. So it is not possible to say that walking, sprinting, standing-by animations are uniformly deleted.

**The modified animation is map by actual name:**

| Sequence | Change of evidence |
| --- | --- |
| Camera_Melee_Saw_A/B/C/D/Low, both 5 Item | Mod original animation tail is identical to unmodified Camera_RoadieRun; frames 101→44 length 3.367→7 ♪ The second that the baby is gone ♪2 |
| Camera_CurbStomp_A/B/C/D/E, all 5 Item | The original animation tail is identical to the unmodified Camera_Door_Kick; frames 66→43、RateScale=0.75 |
| Camera_Pickup_A/B, both 2 Item | The original animation ends with Camera_Door_Kick; NumFrames=25,Show length 1 Seconds, so frame numbers do not equal the number of frame numbers in the tail source sequence. |
| Camera_Mantle | The original animation tail is identical to the original version of Camera_Pickup_B;40→30 Frame, length approximately 1.333→1.34 Zero, RateScale 0.66 |

Besides, Marcus body animations `AR_Melee_Saw_B` Add LateScale =2.5, cannot be considered as an anti-impressive shooting.

The same byte proves the data to be reused, and does not prove specific playtimes, actual calls, final mixing of weights or motion curves. Mod also modified the list of selected actions for lens references; it cannot guess image effects only by animated name.

## Other factual explanations for users

### The mask lens rises when you lift the mouse

The user has observed this phenomenon. No mouse-lifting nodes are added to the package, all class and function numeric node codes are the same as the baseline; default parameters for a bunker lens have changed significantly. For example, the basis PawnRelayive Offset is added `[80,0,50]`, the position shift is changed to `[30,0,20]`, the middle height attitude deviation should read `[50,0,-30]`; ejection, probe, targeting patterns and other deviations.

These changes are consistent with the way that the original game bunker/topdown logic is achieved. But there is no record of how the same bunker has been down and changed in position, nor does it have a formula for positioning the final primary height, which cannot be claimed to be the only reason that a field is "throwing up or down the mouse." It is not an independent virtual station crouching interface that is clearly flattened or what we ultimately call for.

### FOV Bigger

Parameter confirmed: WarCameraMode.FOVAngel is 80→105;WarPlayerCamera. DefaultFOV is 70→120, add 120、LockedFOV=120 and enable the logo. Different lens mode and game settings may be overwritten, and eventually FOV cannot be determined by default only.

The same angle swings usually occupy less screen distance under larger FOV, so "looking low" does not in itself prove that the source of vibration was cancelled.

### Flip point horizontally

User has observed the image not flipping with your body. Normal mode EvadePawnRelativeofset `[0,0,-28]` For `[100,0,-20]`; lens offsets and plug-in values are also changed. The script bytes of the relevant Evade class, back, back, back, and bunker exit actions remain consistent.

This is evidence of “retain rolling motion and change the way it is seen”, which cannot be described as having changed a person's movement to fly-up, nor does it indicate that collision, shooting state or slow motion has been achieved.

## Next round if you need to be accurately verified

Compare the original version / Mod: The lens rotates, position, weapon bone or gun-point relative motion, actual distribution in a shot, with identical guns, same FOV and game vibration settings. If you also want to prove bIgnoreControllers' causality, then you should reposition it for skipping branches or doing a clear variable run experiment. The current conclusion is to stop by packaging evidence and user reality and not proactively modify the author’s package.

Audit tools:`tools/package-inspector/analyze_first_person_motion.py`;complete in-house evidence `analysis/first-person/motion-recoil-audit.json`, parsing error 0. The report and script enter Git, the game resources and back-compilation output do not enter Git.

---

## 中文

# 第一人称 Mod：晃动、后坐力与动作镜头细查

2026-10-12。按用户实测反馈，继续只读检查已安装 First Person 1.0 的下载包，与安装前原版包分析副本对比。本轮没有改包、注入或启动射击测试，VR 开发目标仍暂停。

## 结论与证据强度

不能笼统称为“去掉枪口抖动 / 无后坐力”。包数据能确认的是：武器和 Marcus 身体网格新增 `bIgnoreControllers=1`，而射击镜头震动、手部和脊柱后坐力参数、四种枪械后坐力镜头动画及相关脚本函数未被删改。

**最有依据的推断**：通过网格层忽略骨骼控制，可能抑制枪和手臂的程序化后坐力表现。它是较宽的控制层改动，也可能影响其他骨骼控制。当前没有证明这项设置在所有动作中的原生执行路径，也没有确认被游戏运行时覆盖与否，所以不把推断写成运行验证结果。

Epic 的 UE3 文档说明 Recoil SkelControl 用于驱动射击时的手臂 / 枪骨骼，再通过手部 IK 跟随。这解释了为什么模型反冲和观察镜头震动可以分开：[Using Skeletal Controllers — Recoil](https://docs.unrealengine.com/udk/Three/UsingSkeletalControllers.html#Recoil)。本游戏本地反编译还确认 `WarSkelCtrl_Recoil extends GameSkelCtrl_Recoil`。`bIgnoreControllers` 的具体跳过分支在当前游戏原生代码中尚未定位，字段含义与该设置效果仍需运行验证。

| 用户看到的效果 | 本轮判断 |
| --- | --- |
| 枪和手臂射击时不明显摆动 | 存在直接相关的网格控制设置；很可能受其影响，但未做开关对照 |
| 射击时整个画面震动 | 参数、镜头动画、相关修改器及脚本均保留；不能宣称取消 |
| 连射时准星或瞄准方向受后坐力影响 | 后坐力参数和设置函数保留，没有观察到无后坐力改写 |
| 子弹偏离准星、连射散布 | 瞄准误差参数及函数保留，没有观察到散布清零 |
| 枪口火焰 / 闪光 | 基础 WarWeapon 新增 bDynamicMuzzleFlashes=true，不能解释为关闭枪口火焰 |
| 跑动 / 横移时镜头拖尾与附加偏移 | 多组偏移清零、插值速度大幅增加；确有相关改动 |

## 保留的射击参数

`WarfareGame.Default__WarWeapon` 的以下完整属性值原版 / Mod 相同：

- `FireCameraShake`：持续 1 秒；旋转幅度 `[100,100,200]`，频率 `[10,10,25]`；位置幅度 `[0,3,5]`；FOV 幅度 2。这里是基础类参数，不能代替每种武器最终生效参数或游戏设置。
- `Recoil_Hand`：持续约 0.33 秒；旋转幅度 `[1500,200,0]`，位置幅度 `[-4,0,1]`。
- `Recoil_Spine`：持续约 0.67 秒；旋转幅度 `[750,100,0]`。
- `PRM_Recoil`、`PRM_MinAimError`、`PRM_MaxAimError` 的属性内容相同；未将未解码数组当成实际最终数值。
- `MeleeImpactCamShake` 和 `bDoMeleeDamageShakes` 也相同。

`WarWeapon` 默认对象实际只新增 `bDynamicMuzzleFlashes=true` 和 `bOnlyOwnerSee=false`。本轮筛选的 20 个武器默认对象中，另外 19 个字节一致，包括步枪、COG 手枪、霰弹枪、狙击枪等具体武器类。

按 Recoil / FireViewShake / AimError / Spread / ScreenShake 名称筛出的 21 个函数全部字节一致，包括：GetWeaponRecoil、SetWeaponRecoil、GetWeaponAimErrorRange、GetPlayerAimError、GetAimError、GetFireViewShake、WarInventoryManager.SetWeaponRecoil，以及镜头震动修改器的添加、更新、应用函数。筛选范围是包内相应名字，不代表已解读每个原生函数实现。

两个摄像机修改器默认对象和三个 WarSkelCtrl_Recoil 对象也字节一致。

## 骨骼控制设置

以下两个对象都显式新增：

```text
WarfareGame.Default__Pawn_COGMarcus.WarPawnMesh
WarfareGame.Default__WarWeapon.WeaponMesh

bIgnoreControllers = 1                  // 序列化为 IntProperty，不是 bool 标签
bIgnoreControllersWhenNotRendered = true
```

第二项只涉及未被渲染时的控制处理，不能单独解释屏幕可见枪的抖动。第一项才是这次的重要线索。原对象没有显式序列化这两个新增字段，不把“没有标签”直接等同于已验证的运行时零值。

普通骨骼动画、骨骼控制、观察镜头、游戏射击方向是不同层。忽略骨骼控制不等于停止所有动画，也不等于改变枪械弹道。观察到的视觉稳定还可能同时受高 FOV 和镜头偏移改动影响。

## 镜头动画具体改了哪些

已读出 AnimSet_CameraMovement 内全部 30 个序列的 SequenceName，13 个改动，17 个完全一致。对原生尾部只做完整字节 / 哈希比较，没有假设压缩曲线格式。

**四种后坐力镜头动画完全一致：**

`Camera_Recoil_AssaultRifle`、`Camera_Recoil_Pistol`、`Camera_Recoil_Shotgun`、`Camera_Recoil_Sniper`。

`Camera_Standing_Idle`、`Camera_Walk`、`Camera_Run_Fwd`、`Camera_RoadieRun`、`Camera_Cov_Walk`、`Camera_Cov_Run`、`Camera_Cover` 也完全一致。因此也不能说走路、冲刺、待机的镜头动画被统一删除。

**改动的动画按实际名称映射：**

| 序列 | 改动证据 |
| --- | --- |
| Camera_Melee_Saw_A/B/C/D/Low，共 5 项 | Mod 原生动画尾部与未修改的 Camera_RoadieRun 完全相同；帧数 101→44、长度约 3.367→7 秒、RateScale=2 |
| Camera_CurbStomp_A/B/C/D/E，共 5 项 | 原生动画尾部与未修改的 Camera_Door_Kick 完全相同；帧数 66→43、RateScale=0.75 |
| Camera_Pickup_A/B，共 2 项 | 原生动画尾部同 Camera_Door_Kick；NumFrames=25，保留长度 1 秒，故帧数不等于尾部源序列的帧数 |
| Camera_Mantle | 原生动画尾部与原版 Camera_Pickup_B 完全相同；40→30 帧、长度约 1.333→1.34 秒、RateScale≈0.66 |

此外 Marcus 身体动画 `AR_Melee_Saw_B` 新增 RateScale=2.5，不能将其当成射击反冲动画。

字节相同证明数据复用，不证明具体播放时长、实际调用、最终混合权重或运动曲线。Mod 还修改了一些特殊动作的镜头引用列表；不能仅凭动画名称猜测画面效果。

## 对用户其他实测的解释

### 抬鼠标时掩体镜头升高

用户已观察到该现象。包内没有新增抬鼠标探头函数，全部类和函数字节码与基线相同；已有掩体镜头的默认参数发生大量变化。例如基础 PawnRelativeOffset 新增 `[80,0,50]`，站立姿态偏移改为 `[30,0,20]`、中高度姿态偏移改为 `[50,0,-30]`；弹出、探身、瞄准模式还有另外的偏移。

这些修改与沿用原游戏掩体 / 俯仰逻辑的实现方式一致。但目前没有记录同一掩体下俯仰与位置变化，也没定位最终原生高度公式，不能声称某个字段就是“鼠标上下 → 身体升降”的唯一原因。它也不是头显真实平移或我们最终要求的独立虚拟站蹲接口。

### FOV 变大

参数确认：WarCameraMode.FOVAngle 为 80→105；WarPlayerCamera.DefaultFOV 为 70→120，另新增 ForcedCamFOV=120、LockedFOV=120 及启用标志。不同镜头模式和游戏设置可能覆盖，最终 FOV 不能仅从默认值决定。

同样的角度摆动在更大 FOV 下通常占据更少屏幕距离，因此“看着抖得小”本身不证明震动源被取消。

### 翻滚时视点平移

用户已观察到画面不跟着身体翻转。普通模式 EvadePawnRelativeOffset 从 `[0,0,-28]` 改为 `[100,0,-20]`；镜头偏移和插值也改动。相关 Evade 类、前后左右及掩体退出动作的脚本字节仍一致。

这是“保留翻滚动作、改变观看它的方式”的证据，不能称为已把人物动作改成飞扑，也不能说明碰撞、可射击状态或慢动作已经实现。

## 下一轮如果需要精确验证

可在相同枪械、相同 FOV 和游戏震动设置下，比较原版 / Mod：静止连射时的镜头旋转、位置、武器骨骼或枪口相对运动、实际命中散布。若还要证明 bIgnoreControllers 的因果关系，应再定位它的原生跳过分支或做明确的一变量运行实验。当前结论止于包证据和用户实测，不主动修改作者包。

审计工具：`tools/package-inspector/analyze_first_person_motion.py`；完整本机证据 `analysis/first-person/motion-recoil-audit.json`，解析错误 0。报告与脚本进入 Git，游戏资源及反编译输出不进入 Git。
