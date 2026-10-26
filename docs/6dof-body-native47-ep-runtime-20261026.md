# Native47 + EP 正版后台验证

2026-10-26，`6dof-cover` 私有候选。此记录将装备主握链的实际结果与完整掩体功能分开；默认启动器和公开包未替换。

## 输入与边界

- 正版 Steam EXE，模拟器 `render_scale=0.4`，测试窗口置后、输出只读、游戏静音并恢复；物理手柄后台输入关闭。测试前确认 Game/XR 全空，游戏进程 55200、Native `started_qpc=2559925659979`。既有其他 Game 进程自然退出后才启动本轮。
- Native47 DLL SHA256 `78705ddd823a79a88aed943d559decb3bc873ce32eaf6d05ae9bba7c3d731348`；PG SHA256 `5a34a2808a1fb066091841f2d08b474650fda142cda55fb3cdb61870e4f3be0b`。运行封存于武器 worktree 的 `analysis/equipment-native-rgrip-runtime-20261026ep/`，原八条操作和 `40` 次提交／`6` 秒门槛不变。
- 新来源日志是 `BMSCOM01`，使用独立严格 Reader 解析；旧 `BTICMT01` 读者未接触新格式。主握日志前 32 条的来源查询、双手 v3 姿态和 ARMALP01 事件以同一 pid／Native incarnation、sequence、tick、Compose、thread 联接。

## 实际读回

| 检查 | EP 结果 | 可证明的范围 |
| --- | --- | --- |
| 原固定握持门 | 6 秒内新增 40 次 Native 主握提交，并有连贯 post-FK 观察 | 连续装备主握不再卡在先前的门槛；不是画面或开火证明 |
| 后继移手 | `right-ready` 8 次、右手小幅移动并转 10° 10 次提交；各有连贯 post-FK 观察 | 不同输入后仍持续提交；两次观察不是与每条提交同帧的权威命中证明 |
| 新来源前 32 条 | 32/32 完整来源关联、32/32 ready 回声关联、32/32 capture 与 postcommit 来源有效 | 此日志有 32 条容量，不能据此推断全部 62 次主握提交的逐条姿态 |
| 追踪时龄 | 新同步信息在 postcommit 7.3631–18.0295 ms；旧 V1 同步信息 43.4009–58.6423 ms | 新旧来源不混写；postcommit 是观察时点，不是精确 CAS 时钟 |
| 双手姿态 | 32/32 `pose_pass`，目标误差两手均 0；post-FK 腕位置最大误差左 0.0001563、右 0.0002552 游戏单位 | 已记录的双手 Local/FK 与新来源目标一致 |
| 实际消费者 | 32/32 有四角色 `Applied`，640 事件、0 丢失、0 拒绝行 | 四角色实际应用，不由 owner 授权计数代替 |
| 生命周期 | HandStop 5、quiescent、之后自由双手继续、双控制器恢复和 bridge 关闭；Native/entry 恢复、106 独立检查、XR exit 0、原 10 文件与音频恢复；fresh CIM 无 Game/XR | 本轮正常停机与独占测试窗口释放 |

`body_main_mount_attempts=422`、`ready=75`、`not_ready=303`、`context_rejected=42`、`stale=2`、`commits=62` 是整段会话的诊断计数；其中包含固定握持以外的阶段，不能把 62 条都说成前 32 条完整姿态证据。`body_mount_source_age_rejected=105` 和 `hard_rejected=2` 也保留原值，不因门槛通过而抹掉拒绝路径。

右手 `right-ready` 到小幅移动阶段，独立观察的 pose epoch 为 1310→1332，右腕世界坐标差约 `(-3.191,-1.488,0)` 游戏单位，武器右侧骨差约 `(-3.294,-1.879,0)`；骨盆基本未移动。该关联支持后继输入产生了真实骨姿态变化，但这两个异步观察仍不足以认定屏幕中的武器视觉跟随或权威射击射线。`runtime-review.json` 自身保留 `visible_weapon_follow_proved=false`、`authoritative_aim_proved=false`。

## 后继

装备主握这一共同前提可继续推进掩体接线。仍需实际认证高／矮掩体的侧边与上边、碰撞合法范围、面向掩体的中立眼点、LT/RT 原自动探身替换、LGrip 左／右／上拉拽与保持、真实头动／站起、身体受击／枪口／权威射击、跑步视角 R01–R04、双眼画面及实体头显。不得把本轮 cleanup-only `accepted=true` 或局部骨骼移动提升成这些项目的验收。

原始与派生证据：EP 的 `runtime-review.json`、`runtime-final.json`、`instances/equipment-native-rgrip-1026ep/native/`；本 worktree 忽略目录 `analysis/body-tick-mount-source-candidate1/ep-source1.json`、`ep-alpha1.json`、`ep-pose1.json`、`ep-budget1.json`、`ep-runtime1.json`。首次严格 Reader 调用使用了相对武器路径，因当前工作目录在掩体 worktree 而以 `open` 错误退出；`ep-source-reader-attempt1.json` 保留该尝试。随后的调用改用同一原包绝对路径并通过，未重跑游戏。
