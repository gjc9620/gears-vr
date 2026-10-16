# 准星跨批次分组与子调用边界

2026-10-17。定向源码／已有资料与当前代码核验；本轮没有增加Hook或改变默认Reticle39。

## 为什么不直接扩大分组

group_extents当前只合并同一payload内的认证纯准星片段。四个象限若分散到不同payload，逐片范围可能不居中，从而拒绝校正；这是静态覆盖缺口，现有保留的手枪／步枪实际样本center_rejected均为0，未证明它就是用户反馈的根因。

hook_ui_vm只在共享VM入口产生令牌。当前实际样本大多只看到kind3 PostRender根进入该入口，DrawWeaponCrosshair等子调用走内联解释器。因此同一根令牌可能包含多个子调用，单纯跨payload按root合并无法证明是同一次准星绘制。仅支持kind1 VM根的收集器又不能覆盖目前观察到的实际路径。

字幕TextGroupGeometry的生命周期可借鉴，但它有一次同步调用的完整增量边界，且刻意限制NativeSubtitle来源；不能直接放宽为Canvas权限。后续收集必须是独立的不可变准星组，保留成员精确元数据、复制位置、矩阵一致性、容量及失效门禁。

## 已有资料与汇编

先搜索knowledge.sqlite，取得artifact1 RVA0x1d6f30／VA0x1401d6f30。伪代码SHA256 `dfa7dc7c48a75a4a25dab71d6b6f0d557cb01ac2fb357df6c14daeb2b42b7bb6` 核验通过，但其“六个double”等参数推断不适合直接做Hook声明。

仅复制 `analysis/re-kb/pilot-1000/snapshot.i64` 到 `analysis/ida/reticle-inline59` 的私有副本，用IDA导出此函数已定义指令及六个直接调用者的短窗口。原库前后SHA256保持 `4b64bd2156a9fe8173be0e65a1e47384131089766a628bfd3c03d680f00d1268`，协调器拥有的IDA lease已关闭。其他Agent只阅读保存证据。

入口保存RCX→R13、RDX→RBX、R8→R12、R9→RDI；R9参与+F8／+F4／+114分派。调用者显示四个通用寄存器参数，四个入口路径是tail jump。0x1401d7017–7032构建独立的子FFrame：+18为被调函数、+20为receiver、+28为script、+30为locals、+38为原callerFrame；0x1401d7230–7277为子解释循环。这强烈支持“self、callerFrame、result、calleeFunction”的CallFunction语义，真实符号名及有意义的返回值仍未证明。

关键限制：0x1401d7045之后也用callerFrame求值参数，区间可能包含参数表达式触发的其他绘制。入口RDX是caller而非已构建的calleeFrame；不能直接套现有三参数match_scope，也不能把区间内所有Canvas操作都贴上callee标签。

## 当前进程机器码核验

会话continuous-stereo-40216-20261017-074217-fbfba245，PID40216，完整创建身份见inline60-current-code-retry.json。原诊断脚本未调用Resolver.prepare就读取base字段，记录AttributeError；原失败回执保留。随后同进程只读重试先prepare，核对模块基址0x140000000、支持的磁盘EXE SHA和完整创建身份，再逐条比较711条已定义指令（含IDA所列尾块），全部匹配。它不是原子快照，也不证明未知填充／未定义字节或运行时调用参数。

游戏本次准备2928毫秒、相机就绪至首投影5940毫秒，正常停止／原生清理／XR退出／普通关闭通过。该普通关卡回归不能替代新入口运行追踪或准星验收。

## 下一轮有界实验

1. 仅做观察，不启用跨payload语义合并。独立四指针ABI detour；保留原三参数VM ABI；不据伪代码猜返回值。
2. 在当前模块／覆盖指令核验和正常预留、关闭、入口恢复体系下，追踪有限白名单UFunction的entry／exit ID、thread／depth、parent token、四参数及caller node/code前后值。
3. Canvas ancestry必须实际包含选定callee node/object，并回到该次entry callerFrame，才能计入其正文；排除参数求值绘制，防止共享VM与此入口重复归属。
4. sibling调用ID必须不同；嵌套与拒绝路径恢复TLS；不保存栈frame／locals／result给延迟消费者；父binding令牌仍等待原来的完成认证。
5. 新目标须纳入预留／prime／cleanup／停止后机器码恢复证据。追踪通过后，才设计root结束时原子附加不可变成员范围，不能跳过缺失／已消费／失效成员。

上述实验尚未实现，不宣称已修复用户外圈。所有反编译／汇编／运行资料留在ignored analysis，不进入玩家包。
