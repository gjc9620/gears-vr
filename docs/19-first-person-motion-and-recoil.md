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
