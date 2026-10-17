# 非中性模拟头姿的准星采样

2026-10-17。CallTrace42实验版，默认Reticle39及玩家包未替换。没有启用跨payload校正。

## 姿态与实例确认

会话continuous-stereo-20040-20261017-081036-233490de；游戏PID20040，XR PID38508，完整创建身份见pose66-ready.json／normal-close.json。Simulator MCP报告runtime_live=true、log_file=openxr_simulator.38508.log，与本次XR身份对应，未把上一会话相同PID或缓存FOCUSED状态当作活跃实例。

实际首投影及至少10次提交之后，将模拟器姿态从位置(0,1.7,0)、零旋转设为(0.05,1.7,0)、yaw10°／pitch0°／roll5°。set_head_pose首先只返回queued，立即读到的诊断仍是旧姿态；原pose-applied标记实际只表示命令已排队，不是确认。后续诊断明确报告x0.05、yaw约0.175弧度、roll约0.087弧度，提交记录也有非零四元数与旋转后的眼间偏移。因此实际应用以此后续证据为准，不据排队回执宣称成功。

提交记录frame4643／4644的左眼pose位置约(0.018606,1.697211,0.005536)、四元数(0.003802,0.087073,0.043453,0.995247)，两眼rect均1920×2160，保留各自不对称FOV。单个pose字段是投影眼位，不能直接与头中心位置作相等比较。

## 调用与图像

方向下／右后LT／RT同次请求3秒；本轮实际参与scope3 Weap_AssaultRifle.DrawAdditionalCrosshair路径。32条正文样本仍为16组：scope1每次4 Tile，scope3每次1 Tile。接受／完成1705，候选拒绝及Canvas归属拒绝0，正文Tile575。准星计划／应用147、finite147、direction_fallback0，center／geometry／projection拒绝0。

两份3840×2160图像：ticket279／source277是之前的中性姿态场景；ticket869／source866是输入后的非中性姿态开镜画面。后者可见场景倾斜、射击和白色标记，但同时留有换枪UI，不能凭外观把偏离中心的小标记认定为附加准星。可见白色括号接近单眼中心是后续调查线索，不是已证明的分层错位。

保留的首条计划记录来自ticket829，viewport左眼约(386.843,-178.937,1920,2160)，并非PNG的ticket869。不得用829的计划位置直接判定869的像素残差或宣布渲染器未应用校正。

后续测试应先等待武器选择UI消退，并取得与保留plan同一ticket的图像，再核对可见标记及源范围；不能继续凭图像猜测根因。MCP诊断、提交数据、原始行、图像和裁剪位于ignored analysis/runtime/pose66-*。

## 恢复与边界

测试结束后提交中性姿态恢复命令，随后在仍活跃的同一XR实例中确认位置(0,1.7,0)、yaw／pitch／roll均0，再正常停止。原生清理、停止后入口恢复、XR退出及游戏普通关闭通过。未更改全局OpenXR注册表。此为模拟器链路及有限绘制样本，不是物理头显显示、瞄准命中或舒适度验收。
