# 相机切换观测与菜单恢复

2026-10-15。继续采用冻结 GameRecovery8 / SpaceRecovery3，遵循游戏分辨率；没有增加渲染 Hook，也没有启用 DLSS/DLAA。

## UI 最后输入玩家不等于本地玩家身份

实际 PID33072 的一致复读显示：`PenderCrossplayInvitePopUp / WarUIScene_CrossplayInvitePopUp / input_mode=2`，阻塞计数0，但 `player_index=-1`。`MenuReader` 返回的字段来自顶层 `UIScene.LastPlayerIndex`，不是 `LocalPlayer` 或 Controller 的编号。旧显示分类器统一要求0，会把这个已观察到的游戏状态误分为 UNKNOWN；死亡菜单之后，这可能阻止恢复候选出现。

现在仅对上述精确匹配的非模态场景接受整数 -1 或0。死亡菜单仍要求0；其他负值、其他玩家、缺失值、类型错误、阻塞、场景或复读不符均拒绝。GAMEPLAY 只是显示恢复候选，实际双目仍需要身份正确、恢复时间边界之后的新渲染票据；没有把 UI 状态当成相机或 GPU 证据。

9项 Python 分类/发布测试通过；现有 StartupPanelPolicyTest 通过，保留死亡→UNKNOWN→游戏候选过程中旧票据不能解除恢复边界的条件。未在这一轮人为制造死亡，故不声称已经实际覆盖死亡恢复。

## 只读相机路线采样

新增 `tools/game-io/camera_phase.py`：按进程 PID＋创建时间绑定，沿当前反射字段读取受控 Pawn、ViewTarget、CameraStyle 和相机时间戳。核验角色身份、相互关系、注册表与名字槽，并对读取内容复读；竞争时拒绝该次并重新发现，进程身份变化终止。它不发输入、不改内存，也不授权显示。

`bCinematicMode` 只保存原始存储字节，未验证 BoolProperty 的位掩码，不输出剧情 true/false。CameraActor 也不一律等于剧情。输出保存 EXE 哈希、模块基址与反射元数据，时间戳一致不代表游戏原子快照。

实际运行：

| 运行 | 相机采样 | XR 双目提交 / 图像复制 | 跨度 / 最大提交间隔 |
| --- | --- | --- | --- |
| PID33072，`203226-8bc8d4e3` | 612次接受、13次拒绝；重载后 Pawn 替换 | 5424 / 2446 | 60.235秒 / 12.234毫秒 |
| PID6816，`204724-b7b115ae` | 606次接受、29次拒绝；开场 CameraActor、返回 Pawn、重载后新 Pawn | 5321 / 2477 | 59.110秒 / 12.288毫秒 |

两轮均实际通过状态检查后执行同进程检查点重载；完成原生清理、42入口恢复和 XR 销毁，report 为 stopped/passed，随后正常关闭游戏。第二轮使用新的 UI 分类规则。提交次数包含旧图短时保留，不代表每次都有新游戏图像，也不是黑色像素检测或真头显验收。

开场采样确认了 CameraActor 路线，但重建读者期间存在采样空白，不能把两个观测点之间的全部时间都归到同一路线。首次接入仍需约35秒（包含菜单、等待相机与原生准备），没有宣称延迟已消除。上一轮 PID18196 因测试输入服务残留 stop 标记取消自动导航，未进入关卡；不作为渲染失败或 CG 证据。

原始 JSONL、完整报告及带 SHA256 的 `camera-phase8-audit.json` 位于 ignored `analysis/runtime/`，不得打包。实时剧情相机、预录视频、所有场景的黑帧，以及真头显 UI/准星舒适度仍待验证；本轮没有放宽非 Pawn 相机的立体渲染准入。
