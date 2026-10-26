# ER／ES Stop 后自由双手续更：保存证据的有界差异

本轮仅比较既有 ER 通过与 ES 失败的 Stop／quiescent 记录。没有启动 Game、XR、测试、编译或 IDA，没有重算既有枪／骨骼矩阵，也没有修改共享 Native。原始报告、原件与文件摘要保留；ES 整轮仍不通过，finalizer 成功只表示正常恢复。

最小机器回执：ignored `analysis/lancer-component-readback-20261026/stop-er-es-saved-review.json`；生成脚本 `compare_stop_er_es.py`。Native 源码解释只使用实际冻结的 Native47 `source2/native-capture`，文件摘要也在回执中。

| 项目 | ER 通过 | ES 失败 |
| --- | --- | --- |
| 实际 Game PID | 23040 | 31188 |
| 实际 Game 创建时间 | 134374834037771066 | 134374841811689415 |
| Native session／started QPC | 2694171951921 | 2701965669072 |
| Native DLL | 两局相同 `78705ddd…3148` | 两局相同 `78705ddd…3148` |
| Stop 返回序列 | 4 → 5，quiescent | 4 → 5，quiescent |
| quiescent 后基线读取 perf_ns | 269433361781300 | 270212703751800 |
| 基线参与者 | cookie=0，revision=4，callbacks／transactions／pin=0 | 相同 |
| 基线 admission_closed／stop_requested／unknown_fault | 均为 0 | 均为 0 |
| 基线 observed stream／reference／head／left／right 代数 | 均为 1 | 均为 2；此前保存记录为 1 |
| Native reference epoch／flags／reference_failed | 2／1／0 | 2／1／0 |
| 续更检查 | 通过 | `No post-quiescent continuing free-hand commits / stable held counter` 超时 |

所有增量只在各自进程和 Native session 内计算。没有把独立运行的行号、tick 或累计计数配成同一事件。ER 的通过检查窗口与 ES 的超时观察窗口长度不同，下表不是帧率或性能比较。

| quiescent 后的增量 | ER：基线→实际通过检查 | ES：首条→末条后续观察 |
| --- | ---: | ---: |
| 终点读取 perf_ns | 269434365172800 | 270215429410400 |
| continuous_hand_ticks | +34 | +78 |
| body_native_captures | +34 | +78 |
| body_goal_attempts | +34 | +78 |
| body_goal_plans | +34 | 0 |
| body_goal_rejected | 0 | +78 |
| body_goal_view_held | 0 | +78 |
| body_main_mount_attempts | +34 | 0 |
| body_main_mount_not_ready | +34 | 0 |
| body_main_mount_commits | 0 | 0 |
| body_hands_attempts | +34 | 0 |
| body_hands_commits | +34 | 0 |

ES 这 12 条后续观察的第一条与 quiescent 后基线的上述累计值相同。Native 最终停止时的单独摘要还显示：从基线起 Goal 尝试／拒绝各 +120，Goal plans、MainMount 查询和 hands attempts 仍不增加。最终摘要属于更晚的恢复／退出阶段，不能代替 Stop 时刻；两局最终 observed 代数还分别变为 ER=2、ES=3。特别是 ER=2 出现在已通过续更检查后的较晚摘要，不能据此反推 ER 在检查期间也失败。

最早有证据的条件分歧是：ES 的 quiescent 后完整基线已读到 observed 代数 2，随后 Goal 层持续尝试并拒绝，却没有进入新的 Goal 规划／MainMount 查询／双手尝试；ER 同阶段保持代数 1，并继续规划与提交。ES 前一条 live 记录读取时间为 270212611437200，仍显示代数 1；首次读到代数 2 的 live 记录为 270212846876200。这里是观察器读取既有状态摘要的时间，摘要可能滞后且不是原子源事件，不能据此把连续性变化精确放进 Stop 的某个调用区间，更不能认定 Stop 导致了变化。

冻结源码给出一条具体候选路径：`body_c0_goal_policy_live.inl::same_source_basis` 要求中性基准 key 与当前 source 的 observed stream／reference／head 等代数一致；`pose_begin` 在 origin 已初始化且 `same_key` 失败时设置 `origin.poisoned` 并在规划前拒绝。ES 的已观察计数与此路径相容。**原件没有保存 origin key、poisoned 值或实际早退原因，所以没有证实这一分支，亦没有排除 captured／current／standing／cover 等其他规划前条件。** `body_goal_view_held` 在冻结源码中计的是过期缓存视图的继续使用，不是玩家握枪，也不能单独证明某条错误路径。

参与者侧没有显示“仍占着注册而不能续更”：两局均已 cookie=0、callbacks／transactions／pin=0，故不能仅把问题归为未完成 quiescent。ES 的 `body_native_apply_gate_9`（冻结 enum 中为 context）在移动阶段后到基线间增加 1，随后不再增加；它不是对之后每次拒绝的逐次记录。`prepare_code_12` 在 ES 最后移动阶段已为 1，也不能当作新的 Stop 专属错误。

以下缺证保持明确：

- BMS 只保留 sequence 1–32：ER 最后一条提交早于 quiescent 基线约 4625.04ms，ES 约 4519.46ms。本次只验证了真实 Header64／Record8616、PID／Native、序列和 leading commit 时间，未重算来源关联，更没有将前缀扩充成 Stop 时的 source。
- 单次 `C0FAIL01` first-failure 的 session 及 Header280 与冻结定义一致；ER 记录早于 quiescent 基线 8145.20ms，ES 早于 7867.91ms。它们都不是 Stop 时第一次 Goal 拒绝的记录。
- Stop export 回执保留 4→5 和 terminal／thread-exited，但没有精确 dispatch／completion 时标。其 deadline 不是源事件时间，不拿它倒推竞态顺序。
- 未保存造成 observed 代数变化的具体 publication 拒绝原因。ER／ES 的 PG 与外部观察器也不同，不能据独立运行把代数变化归因于新采样器耗时。

最小下一验证动作交给 Body 唯一入口负责人：在**既有** `pose_begin` 的 Stop 后首次拒绝处增加一条有界原因记录，保存具体失败谓词、origin initialized／poisoned、实际 old／current observed key、PID／Native session／tick／Compose 与时间。先确认是否为中性基准 key 变化早退，再决定修正。保留所有资格、恢复和写入约束；不新增 Hook／writer，不自动清除 poisoned，不放宽连续性门，也不以旧 first32 填补当前来源。本比较没有启动该验证。
