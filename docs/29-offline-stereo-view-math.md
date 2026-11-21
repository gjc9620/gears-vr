# Offline check for left- and right-eye view calculations

2026-10-12. Continue Document 28 . This step only deals with stored CSVs, without connecting games, without modifying processes, without starting headlines or emulators.

`check_stereo_view_math.py` Take the lens position, view and projectile from the same scene. The first column of the rotation section of the view gives a right-hand direction to the world space under this layout; shift one composite game unit in that direction, adjusts the horizontal horizontal shifts at the end of the view line separately, generating parallel eye views.

The tool validates two relationships: the eye ' s own world space position should be changed and returned to the same lens original point; the left-level projection at the front synthesis test point should correspond to the lens spacing, distance and projection scaling. The CSV location itself is rounded with error, so that the same baseline margin difference is used for comparison, without recording errors moving as view.

Original Sports PID 43632 It's... 9,868 All samples passed, maximum margin of error. 4.26e-7. Synthetic eye spacing 2 Game units, forward point distance 1,000 Game units. Example: submersible projector with left or right horizontals +0.00119150 / -0.00119201, margin approximately 0.00238351- It's the same as the calculation.

This proves that the offline algorithms used are consistent in recorded lens movements and do not prove that the game has been deciphered. Synthetic space is not a real IPD for human beings; asymmetric cones, unit scaling, original multi-view structures and eye texture submissions of the units and the OpenXRs still need to be verified and realized.

source code save Git;original report `analysis/runtime/renderer-original-motion-43632-stereo-math.json` Saves only on J disk as per the rule. This step is to re-use existing evidence and does not require a user to operate again.

---

## 中文

# 左右眼视图计算的离线核对

2026-10-12。接续文档 28 的 CPU 行向量布局证据。本步只处理已经保存的 CSV，不连接游戏、不修改进程、不启动头显或模拟器。

`check_stereo_view_math.py` 从同一条场景记录取镜头位置、视图和投影矩阵。视图旋转部分的第一列给出该布局下的世界空间右方向；在该方向各偏移一个合成游戏单位，分别调整视图末行的水平平移，生成平行的左右眼视图。

工具验证两个关系：各眼自己的世界空间位置变换后应落回同一镜头原点残差；前方合成测试点的左右水平投影差应符合镜头间距、距离和投影缩放计算。CSV 位置本身有舍入误差，因此使用同一基线残差比较，不把记录误差当成视图移动。

原版运动 PID 43632 的 9,868 个样本全部通过，最大残差误差约 4.26e-7。合成眼间距 2 游戏单位、前方点距离 1,000 游戏单位。示例左右水平归一化投影约 +0.00119150 / -0.00119201，差值约 0.00238351，与计算一致。

这证明所采用的离线算式在已记录镜头运动中一致，不证明游戏已经分别渲染左右眼。合成间距不是人的真实 IPD；游戏单位与米、OpenXR 各眼非对称视锥、单位缩放、原生多视图构造和双眼纹理提交仍需验证和实现。

源码保存 Git；原始报告 `analysis/runtime/renderer-original-motion-43632-stereo-math.json` 按规则只保存在 J 盘。本步复用现有证据，没有要求用户再次操作。
