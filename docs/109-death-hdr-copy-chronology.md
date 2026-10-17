# 死亡HDR写入、复制及采样顺序

2026-10-17。独立Hdrtimeline52源提交650bd486814f89fbc3311bbeb59828e9432704bf，DLL SHA2566a09fac49fe791da4dc02369de7857bc36b2b66ecf72e56ed10eebf861e0af71。DEATH_SOURCE_CONTROL关闭，新增默认关闭DEATH_HDR_TIMELINE，只观察已有Hook，14项冻结CPU测试通过。默认仍Reticle47。

实际92会话continuous-stereo-40132-20261017-113435-e5578f99，选中死亡票据703/704/705/751。每帧六个COM身份均成功，身份缓存无溢出、无读取失败；copy、所选quad、HDR Draw和Execute没有丢弃。OM绑定行触及128上限，四帧分别丢135/151/147/10项，stored_observations_complete均为0。Clear未拦截、Barrier Hook未启用，其他线程及非首RTV不覆盖，state_coverage_complete始终0。

同一票据的Draw采用连续同形状段聚合，copy、sample、OM、Execute或丢弃会拆段；不跨复制合并同一RT的写入。资源身份由现场真实COM对象查询，不以指针相差0x30推断。

稳定票据751的关键顺序：

| CPU序号 | 操作 | canonical资源 | Execute序号 |
| --- | --- | --- | --- |
| 105 | 早期全幅copy | SRC1a4e1ae0 → DST1668cfb670 | 35686 |
| 166/168 | 2ab6d9左右眼，右viewport为1920,0,1920,2160 | 写SRC1a4e1ae0 | 35693 |
| 184 | 第一遍后续全幅copy | 同SRC → 同DST | 35697 |
| 191 | 一次全幅HDR Draw | 写DST | 35697 |
| 193..199 | 后续左右HDR Draw | 写SRC | 35697/35698 |
| 201 | 第二遍后续全幅copy | 同SRC → 同DST | 35699 |
| 203 | 一次全幅HDR Draw | 写原SRC | 35699 |
| 229/255 | d80a7b左右眼 | 采样DST | 35700 |

上述操作记录在同一DIRECT queue1a5c3640。四帧均呈现这条关键顺序；最后一次选中copy分别为703/seq466、704/seq565、705/seq518、751/seq201。CPU Execute关联不是GPU完成证明。没有观察到最后copy与两个death pass之间的DST HDR Draw，不等于排除其他clear、alias或未覆盖写入。

GPU源票据750/780均有明确无占有提交记录，右眼近黑97.86%/97.84%；正常游玩前后票据288/1014双眼非黑。一次检查点world1→2，恢复观察5676ms（含加载）。正常原生清理、游戏正常关闭及四份存档哈希恢复通过。

## 下一步的单处HDR像素证据

不再改UV。只计划读取稳定第49个死亡票据在左右2ab6d9之后第二遍匹配全幅copy的源。通过同command list、紧邻原Copy追加一次相同源subresource到自有READBACK buffer，复用原Copy的COPY_SOURCE合法前提，不猜Draw处状态、不建独立queue/list或barrier。此处尚未实施或验证快照成功。

GPU可见对象必须同时满足记录generation已成功Reset/退休及相关GPU执行完成后才能Map；仅首个fence完成不足以排除generation重放。限制一次Execute，重放、跨queue、未知提交、Signal失败或未完成停止均标记不确定并保留资源，不能提前释放或宣称读回成功。保留原始HDR字节和footprint，再离线解码。运行copy范围还需匹配历史1c6033起128字节SHA2569a2b3a678a07074110f5209b21e01a66b6a93a34872b76c01eee70f2d30b2c86；历史blob身份仍为artifact1，尚未声明新进程匹配。

证据在忽略目录analysis/runtime/hdrtimeline92-*及会话.native-status.txt、.native-status.hdr-timeline.json、GPU图。真实头显与整个第一版仍未完成。
