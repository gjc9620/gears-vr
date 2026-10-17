# 外圈首个四边形的前驱类型证据

2026-10-17。提交593c0bd增加观察器专用逐Tile事务，原始flagOFF路径保持。CallTrace44冻结源码和完整构建记录见analysis/runtime/calltrace44-build-proof.json；DLL SHA256 `b28336cb37119b64c054ce0e4abab517b97f6ef0857cb5601a4e754d5f5d1199`。6项聚焦自测通过，包括两次一致链读取、范围限制、外来vtable不读payload、变化拒绝测试。

会话continuous-stereo-22748-20261017-084919-8209cb2c；完整创建身份及实际模拟器非中性姿态确认见pose73-ready.json／simulator-evidence.json。游戏输入前检查游玩状态，LT开镜，RT=0；保存5张GPU图像，正常恢复姿态和退出。

保留20条失败事务全部符合：sample_ok=1、snapshot_attempted=1、before_ok=0、current_ok=1、tail_ok=1，前驱vtable均为`0x14136c5f0`，不等于已认证的simple-element类型`base+0x133cc98`。这些失败调用后的只读严格快照均成功，得到新的simple payload及vertex_count=4。失败后的快照没有参与发布。

例如root1633：调用前选中sort_key0，尾项为0x16746b3ba0、vtable0x14136c5f0；调用后simple payload为0x17c8a6c0，四顶点，但没有收据发布。这证明了旧的调用前快照拒绝路径会漏掉本次创建的新批次，而不是Canvas参数或调用后归属校验失败。具体类型语义仍未知，未把这个vtable强行命名为材质或文本。

成功记录固定32条，早于上述失败样本，不能宣称同一root的后三次成功事务均已保留。pose72另有同ticket的外圈3/4已知与附加图标完整来源配对，二者共同支持需修复新批次前驱处理，但不证明所有武器的问题或所有历史黑屏均由此产生。

修复要求：保留严格simple快照和未知索引拒绝规则。用独立、显式的foreign-tail调用前状态，只保存有限旧项指针值及一致链；调用后必须完整保留旧前缀、恰新增一个不同simple项、恰有四顶点，再复制新增区间。任何类型变化、前缀变化、别名、额外追加或读取失败都拒绝。外来payload不能被读取或重新解释为空批次。

实际原始事务保存于ignored analysis/runtime/pose73-transactions.json。该会话原生清理、停止后独立入口恢复、XR退出和游戏普通关闭通过。玩家默认版及ZIP未更改。
