# 开镜后的外圈／附加图标调用配对

2026-10-17，提交896e2f3。观察器由最早64条改为无绘制／有绘制各32条，仍只用于诊断，不改归属、渲染参数或发布语义。容量互不挤占及丢弃计数测试通过。

CallTrace42冻结源码位于analysis/native-source-calltrace42；DLL SHA256 `cafc1ecc7027e6309592c359a7d5bad2fae05b3eca3e645f20f9af0f9d046966`。必要运行自测程序已完整构建，5项聚焦自测通过；启动仍执行同次完整自测。默认Reticle39和玩家ZIP保持原版。

## 实际记录

会话continuous-stereo-40664-20261017-080518-4c4537fa，游戏PID40664；完整身份见calltrace65-entry-proof.json。实际首投影后，数字方向下／左，再LT／RT同次请求3秒。该控制序列选中了参与Weap_LocustAssaultRifle.DrawAdditionalCrosshair路径的实例，不能称为Lancer测试。

统计总请求598719，候选／接受／完成均955，拒绝及深度拒绝0；正文Tile495，Canvas归属拒绝0。保留32条空调用和32条有绘制调用，分别另丢弃725及166条；没有扩大固定记录总量。

有绘制记录形成16组：scope1 WarHUD.DrawWeaponCrosshairGOW3每次4 Tile，嵌套scope4 Weap_LocustAssaultRifle.DrawAdditionalCrosshair每次1 Tile。它们共有同一父UI令牌，附加调用parent_child等于外层child ID；16组的entry／exit区间均满足严格嵌套。内外调用的各自计数没有重复归属。此结果不证明4 Tile或1 Tile的纹理可见内容身份，更不等于头显融合验收。

## GPU配对

其中4组与保留的消费端记录按父令牌对应，root629／631／633／635，ticket86／87／88／89。每组外层payload有16顶点／4收据，另一个payload有4顶点／1收据；同ticket、list及其generation、target，已记录实际索引／几何匹配及命令完成。

两部分每眼的最终viewport和scissor逐字符串相同，准星考虑／计划／应用均196，center／geometry／projection拒绝0。这批实际样本里，四个外圈quad在一个payload内，而附加tile在另一个；没有复现“四个外圈象限分散到多个payload而拒绝校正”。不能把该静态覆盖缺口认定为这轮的根因。

配对报告calltrace65-pair-audit.json，正文原始行calltrace65-body-rows.json。它支持已记录范围的调用／GPU关联，不证明纹理alpha可见中心、武器真实命中点或物理头显融合。

运行时44个入口证明再次通过；正常停止、原生清理、停止后独立入口恢复和普通游戏关闭通过，保存两份4K GPU图像。所有私有资料留在ignored analysis。

下一项优先验证动态头姿下的实际UI位置及另一步枪；未取得更多证据前不启用跨payload分组或猜测性准星偏移补偿。
