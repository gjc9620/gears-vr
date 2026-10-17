# 外圈新批次首个四边形修复

2026-10-17，代码提交6a49647。新增独立Before适配器，保留原有严格simple快照API：严格快照失败时，只允许一致、有限的foreign-tail前驱状态。调用后须保留旧指针前缀、恰新增一个不同的simple项且只有四顶点，再复制新增区间；不读取foreign payload，不将其视为空。后续来源、索引、字节匹配、ticket和target校验不变，allocation_identity_proven仍为0。

UiCanvasTileBatchTest包括旧严格路径拒绝／新路径成功的回归样本，旧数组释放及重分配、64→65边界、伪造Before、别名、类型／前缀／header变化，以及失败时不改输出。与Canvas旧路径、收据、索引、准星计划等共11项聚焦测试通过。完整构建日志及结果保留在analysis/runtime/calltrace45-build-proof.json。

## 实际游戏复测

CallTrace45冻结自6a49647，DLL SHA256 `40921e6bfbdff24040779cdb4571a2cc930adc65b08c05df7c40032875bf3a46`。会话continuous-stereo-34980-20261017-085452-4deacd45；完整创建身份与模拟器姿态确认见pose74-ready.json／simulator-evidence.json。实际位置(0.05,1.7,0)、yaw10°／roll5°，数字方向下／右、等待五秒、LT开镜，RT=0。

root1595保留完整事务序列：scope1首调用严格before_ok=0但typed_transition=1，成功发布[0,4)；后三次严格路径分别发布[4,8)、[8,12)、[12,16)；scope3附加图标发布另一payload的[0,4)。共7条保留的typed transition均追加／发布成功，不再跳过首个四边形。

实际保存的图像ticket501、508与详细行相交。两帧外圈均16顶点／4收据／24准星索引／0未知，附加部分4顶点／1收据／6准星索引／0未知。各帧两部分共享list、generation、target，完成及几何检查通过；每眼viewport和scissor逐值相同，两部分均有applied=1。

例如ticket501，左眼viewport为(384.752777,-180.893875,1920,2160)，右眼(1837.26147,-180.893875,1920,2160)，两部分相同。图像与计划来自同ticket，不再混用此前帧坐标。

本轮ui_batch尝试／发布4230，before／append／publish拒绝均0；HUD unknown_or_mixed0；准星258次考虑／计划／应用均成功，finite258、fallback0。相较pose72／73，这证明已修复本次实际遇到的新批次来源缺口，不能推为所有武器、所有黑屏或物理头显融合已验收。

## 恢复及后续

同一活跃模拟器实例确认中性姿态恢复后正常停止，原生清理、停止后独立入口恢复、XR退出及游戏普通关闭通过。原始记录位于ignored analysis/runtime/pose74-result.json及会话同名文件。默认Reticle39与ZIP尚未替换；下一步构建关闭诊断观察器的玩家候选版，并验证手枪与检查点恢复。
