# First Person 1.0：本地包对比研究

2026-10-11。输入为用户下载的 `First Person-21-1-0-1764109229`。
只解压分析副本、解析及对比；没有安装 Mod、替换游戏包或执行其中脚本。

## 结论

这是通过改写 UE3 包中默认属性、人物模型及镜头动画实现的平面第一人称 Mod。
可参考其镜头位置、原版镜头模式和身体显示处理；它没有提供 OpenXR、
实时头显输入或双目场景绘制。我们仍需在已经验证的原生视点入口加入头显位姿。

## 文件与证据范围

仅两个文件，未附 DLL、EXE 或安装说明：

| 文件 | 字节数 | SHA256 |
| --- | ---: | --- |
| Engine.xxx | 26270651 | 983f0d5d6757e2652ce29f27bc8fb8bd7a979ff3d87446e004c771e5188ae8f5 |
| WarfareGame.xxx | 261301387 | 98872b93f7a2257e1aeb0a25ae29347dc5950e3ee75af178df12bc091eec219a |

两包版本 835/76，与当前安装游戏的分析副本相同。
此对比的基线是我们当前安装版本，不是作者制作 Mod 时的原始底包。
所以差异也可能包含版本/平台差异，不能把所有差异都归为作者有意修改。
LZ4 分块解压尺寸全部核验通过；输出位于 analysis/first-person/decompressed。
UELib 对部分属性反编译失败，因此另外编写严格边界检查的属性标签读取器，
读取 Vector、Rotator、布尔值和嵌套 ViewOffset，避免依赖错误的反编译文本。
对象前缀中的属性起点通过限定前 80 字节、完整标签流解析定位；这是格式研究工具，
不是通用包编辑器。未解析的原生资源尾部保留长度记录，不推断其具体几何。

## 改动类别

Engine 共 37299 个 export，WarfareGame 共 149763 个；没有新增/删除 export。
两个包的全部 Class export（1211+1117）及 Function export（5373+7702）
序列化数据与原版完全相同。WarfareGame 名称表新增 38 项，原有名称索引和值未改。
因此没有观察到函数字节码或类定义改写，主要是类默认对象及资源改动。

原始哈希差异为 Engine 587 个、WarfareGame 3484 个。
其中 582 / 3413 个资源的差异全部符合绝对文件偏移随 export 搬移而更新的模式。
这是字节级分类，不是完整资源格式证明；不能把所有不同哈希都当作作者更改资产内容。
其余主要是 5+56 个默认对象、14 个动画序列和 1 个 Marcus 骨骼模型。
这些默认对象的完整属性流解析均通过，具体差异 JSON 保存在本机分析目录。

## 镜头是怎样移到人物附近的

普通镜头原 ViewOffset：

| 子字段 | 原值 | Mod 值 |
| --- | --- | --- |
| OffsetHigh | [-128,56,40] | [0,0,0] |
| OffsetMid | [-160,48,16] | [0,0,0] |
| OffsetLow | [-160,48,56] | [0,0,0] |

负 X 的肩后偏移和侧向偏移被去除，多个分屏/视口调整也被清零。
同时按状态设置 PawnRelativeOffset，仍使用游戏既有镜头模式：

| 模式 | Mod 新增 PawnRelativeOffset |
| --- | --- |
| WarCameraMode_Default（普通） | [35,0,15] |
| WarCam_Targeting（瞄准） | [40,0,10] |
| WarCam_TargetingGrenade（投掷物瞄准） | [40,0,10] |
| WarCam_Cover（掩体） | [80,0,50] |
| WarCam_Crouch（蹲伏） | [0,0,0] |
| WarCam_RoadieRun（冲刺） | [65,0,-36] |

“新增”表示原对象没有显式序列化该字段，不能将其原有效值直接视为零，
也不能忽略父类继承。各字段相对的原生枢轴及最终位置还需运行验证。
翻滚 EvadePawnRelativeOffset 从 [0,0,-28] 改为 [100,0,-20]。
掩体另有站立、低掩体、探身、弹出射击偏移和转向限制的专门修改，
并非所有状态只使用同一个固定第一人称位置。

## 减少镜头拖尾、调整避障和 FOV

WarCameraMode / Default 把 LazyCamSpeed、各轴 FollowingInterpSpeed、
跑动和横移插值速度提高到 999 / 99999（另有 100009），BlendTime 改为 0.1。
普通模式的跑动/横移附加偏移清零，RunOffsetScalingThreshold 改为 99999；
bDirectLook=true。结合字段含义，预期减少镜头拖尾及自动偏移，实际平滑仍需原生逻辑核验。

WarPlayerCamera：

- DirectLookInterpSpeed、WorstLocInterpSpeed、TemporaryPivotInterpSpeed 等为 99999；
- PenetrationBlendInTime / OutTime 为 0，Focus_BackOffStrength 为 0、Focus_MaxTries 为 0；
- 默认镜头模式的 WorstLocOffset 清零；Marcus 的 bBlockCamera=false；
- bIgnoreRigidBodyPawns、bIgnoreEncroachers、bNoEncroachCheck 等为 true；
- bAlwaysTick=true、bHardAttach=true，多个碰撞标志为 false；
- WarCameraMode FOVAngle 80→105；WarPlayerCamera DefaultFOV 70→120，
  ForcedCamFOV / LockedFOV=120，相关启用标志为 true。

不能由此认定所有镜头避障已经禁用：部分 bDoPredictiveAvoidance 仍为 true。
最终 FOV 的优先级也尚未运行测量。VR 的双眼投影应使用头显提供的每眼 FOV，
不会直接把 105/120 的平面参数当作双眼投影。

## 身体、武器与动画

Marcus 主骨骼模型 `COG_MarcusFenix.COG_Marcus_Fenix_CamSkel` 的 export
从 3067964 字节缩为 1544075 字节，原生资源内容大幅变化。
尚未解码顶点/三角形，不能具体宣称删除了头部或哪块身体。

Marcus WarPawnMesh、WarWeapon WeaponMesh 明确设置 HiddenGame=false、
bOwnerNoSee=false、bOnlyOwnerSee=false，并改动剔除、灯光和阴影等属性；
多个枪口、弹壳和武器烟雾组件也有显示/剔除参数变化。
这说明它并非简单把整个身体隐藏。具体第一人称可见身体、持枪位置和穿模问题仍需观察。

14 个 AnimSequence 改动，其中 13 个位于 AnimSet_CameraMovement，
包括 NumFrames、SequenceLength、RateScale 及原生动画数据。
例如若干 101 帧序列改为 44 帧、长度 3.3667→7 秒、RateScale 新增 2；
若干 66 帧序列改为 43 帧、RateScale 新增 0.75。
另有 Marcus AnimSet 中一项 RateScale=2.5。
未映射动作名称或完整解码曲线，不能宣称所有镜头晃动都被消除。

Engine 的 CameraActor 默认对象还新增 Tag=None、bDeleteMe=true。
它可能影响过场镜头生命周期，但没有实测；不能当成已跳过所有过场。

另外观察到 Engine 默认对象字体列表、IniLocPatcher 后缀列表和
OnlineURLManager 的重试/超时字段等差异。这些与第一人称没有直接关系，
可能来自制作底包差异；未将其归为第一人称所需修改，也未用于调整联网等待。

## 对 GearsVR 的用途

可借鉴：按普通/瞄准/掩体/冲刺等状态建立基础视点；分开处理第三人称肩后偏移、
自动镜头运动和身体显示。我们的头显相对位姿再叠加到已经核验的原生临时视点。
必须验证原版动画/掩体自动转向与头部自由观察的关系，而不是盲目照抄全部属性。

这个 Mod 仍沿用原版状态和动画。它没有解决我们的双手武器瞄准、
头部观察与枪口朝向分离、虚拟站蹲/探身交互，也没有解决双眼绘制。

作者将其标为未完成 Beta，并说明动画和掩体可能出现镜头位置错误、模型内部闪现。
这是作者说明，与本轮静态分析应分开看；本机尚未安装实测。
[作者页面](https://www.nexusmods.com/gearsofwarreloaded/mods/21?tab=description)。

## 已采用的第一人称相机参考方向

2026-10-12，用户确认：后续 GearsVR 可以参考该 Mod 的第一人称视点处理。本次只记录方向，不据此新增实现或修改游戏包；VR 开发仍保持暂停。

包对比表明，它调整原有第三人称镜头的偏移和默认行为，没有观察到新增独立第一人称相机类或函数。它提供的是一组可用的第一人称视点参数参考，不是可直接复用的原生相机地址。

采用以下研究方向：

- 参考将肩后 `ViewOffset` 清零的方式，研究稳定的第一人称基础视点。
- 参考普通、瞄准、掩体、冲刺和翻滚分别设置 `PawnRelativeOffset` 的方法；现有数值作为调查起点，先验证坐标参照、人物锚点和各状态的最终视点。
- 在我们已验证的原生临时视点入口，组合基础视点、相对头部变换及每眼变换；旋转和平移需按坐标系组合，不能简单累加所有欧拉角。
- 参考身体 / 武器可见性和模型稳定处理，但自行验证穿模、枪口遮挡、骨骼控制和后坐力，避免直接忽略全部骨骼控制。

必须保持最终交互方案的要求：头部观察与枪口瞄准独立；头显俯仰不能被当作虚拟站蹲 / 探身操作；身体翻滚不能强制让头显视点翻转。用户观察到该 Mod 抬鼠标时掩体镜头升高，这一联动的具体公式尚未验证，不能原样作为 VR 控制方式。

以下任务仍由 GearsVR 独立实现和验证：左右眼各自渲染、OpenXR 位姿与图像同步、真实头部平移、尺度与回中、生命周期处理，以及双手持枪和掩体交互。第三方包和模型不纳入我们的部署包；参考研究不代表已获授权复用作者资产。

后续证据：[安装与恢复](18-first-person-mod-install.md)、[晃动与后坐力细查](19-first-person-motion-and-recoil.md)。前文“未安装”和“未映射动作名称”描述的是 2026-10-11 的调查状态；后续已安装确认关卡第一人称视点，并映射相关动画名称，完整曲线与各动作运行效果仍待验证。

## 工具与复核

- ExportInventory.cs：使用既有 UELib 只读列出 export 位置、长度、哈希、名字表。
- compare_export_inventories.py：对比 inventory，分类偏移修正候选，输出简短计数。
- compare_default_properties.py：读取默认属性，支持 --include-class AnimSequence / SkeletalMesh。

原始包及导出内容均 Git 忽略；仓库只保存我们写的工具和研究报告。
没有复制第三方模型或包进入 Mod 源码。
