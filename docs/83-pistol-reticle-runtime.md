# 手枪准星的实际投影检查

2026-10-17。针对用户“中心图标正确、后坐力外圈错位”，保持默认Caption38／Borderless1和本机HUD45°、1.2／0.8、镜头拉近0.3米，未修改原生代码或新增Hook。第一版UI验收仍未完成。

## 当前实现边界

冻结源码afd364d及caption38-source.json确认GEARS_OWNED_UI_BATCH／OWNED_OUTPUT_TRACKING均OFF，不能把未启用的owned路径HUD回退当作本候选原因。活动路径仅在纯Canvas准星几何通过中心门禁时使用reticle plan，纯准星不会落入HUD平面；不通过时仍可能保留原半宽overlay位置。

plan不接收segment/root/extent，分组只改变准入。在几何、目标、aspect及有限点／无穷方向分支都相同的条件下，已通过的内外圈有共同viewport仿射。仅“同一ticket”不能推出这些条件成立；不同scissor也可能让有限点分支对一个draw失败，随后回退无穷方向。当前分组限于单payload，四象限若分在四个payload仍有覆盖缺口。这些是代码上的条件与反例，没有当作已复现的Caption38故障。

旧Native36 ticket2088的16顶点与4顶点payload同root，实际左右viewport/scissor完全一致，但不标明手枪，也不证明纹理可见中心或物理融合。不能把raw左右像素位置相同当VR融合标准，OpenXR双眼FOV不对称时本应不同。

## 新手枪样本

实际会话continuous-stereo-19512-20261017-063430-d503c29c，游戏PID19512／创建FILETIME134366636578209339。前台虚拟手柄Down后，以现有反射只读工具确认当前实例Weap_COGPistol；160／160采样通过，AmmoUsedCount从5到6、SpareAmmoCount60，记录一次实际射击。没有开启后台实体手柄或重试未知武器选择。

3秒LT／RT请求期间采样GPU：首次成功projection约在请求开始后1.277秒，因此前两个截图请求早于首投影并可能合并／延迟；不能把请求时刻当保存时刻，更不能认定捕获后坐力峰值。有效原始收据为ticket3／10／274，源序号1／7／268。运行中phase=running不等于已得到第一张双目图，下一次动作应等新图和epoch内提交后再开始。

原生ui_reticle_direction considered／planned／applied均86，center_rejected／geometry_missing／projection_rejected均0，有限点计划／应用86、方向回退0。保留GPU行均为纯准星单quad（4顶点），本样本没有独立内圈图标与外圈的配对，不能证明二者同心。

保存图ticket10／source7，3840×2160：两眼均有正常场景、人物持手枪和白色外圈。对应row7计划中心分别(1193.248093,886.034927)、(2645.779790,886.034927)。在每个计划中心附近91×111窗口，以RGB各通道≥235、通道差≤12选择近白像素，外圈包围盒中心分别(1192.5,885.5)、(2645.5,885.5)，距计划中心约0.920／0.604像素。人工检查同一帧裁切确认所选窗口对应可见外圈；这是有界像素启发式，不是通用物体识别或子像素精度声明。

这个结果支持“当前这一帧手枪外圈确实落在其修正计划位置”，排除该样本外圈门禁拒绝；不证明计划位置等于实际子弹落点、不证明头显融合、不覆盖独立中心图标、后坐力峰值或全部武器。

## 证据与退出

ignored analysis/runtime下pistol48-audit.json、ammo.json／stdout、ring-pixel-audit.json、reticle-crop.png及上述会话原始GPU／状态／日志保留。原生清理与入口恢复通过、XR正常退出，游戏身份核验后正常关闭，见pistol48-normal-close.json。

下一项最小检查是先等待实际新图，再采同一ticket中独立内圈和外圈，并核对实际viewport／scissor。若两者映射相同，需检查源Canvas变换、可见纹理中心或GPU结果，不能继续凭门禁计数盲改位置。
