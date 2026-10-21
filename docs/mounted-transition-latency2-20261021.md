# 上下机枪恢复延迟第二轮优化（2026-10-21）

用户确认剩余约 1 秒延迟只发生在上下机枪后恢复画面，机枪上持续转头和操作没有对应延迟。本轮从最新默认 ADS Anchor2 native 与 MountedLatency1 XR 继续修改启动器 Python，保留已经集成的瞄准、准星和其他修复。

## 改动

- `world_rebind_runtime.py` 增加一次完整采集路径。指针变化提示触发后，直接构建完整候选，再从候选取得完整世界身份，由 monitor 的原有本地玩家、变化与尝试次数规则决定是否发布。省去之前先 observe、再完整 prepare 的重复采集；提示本身不授予准入权限。定时常规探测仍走原路径。
- `world_rebind_monitor.py` 与 `run_stereo_continuous.py` 在有界 building/Prepare/Commit 事务期间把 owner 休眠从通常 100 ms 缩短为 10 ms。building 只有收到变化提示、完整观测确认变化或 native 要求恢复后才加快；普通未变的周期观测、正常游玩、失败退避、事务超时及关闭后保持原频率。完整证书、native 重验、旧 scope/GPU 排空和新世界图像准入保持原有规则。
- 实际回归遇到一个启动前重试分类遗漏：嵌套 timestamp guard 耗尽后，外层 sample 可以继续并最终因跨帧 measurement 耗尽，但回执的 stage 仍为 `sample_before` 或 `sample_after`。`start_capture.py` 现在也检查这些阶段的每一份原始失败证据；只有全部符合既有暂态分类时才在原有总期限内从头采集。身份变化、其他阶段和未知失败继续拒绝，失败值不会进入证书。

## 检查与证据范围

Python 检查共 240 项通过：monitor/runtime/owner 109 项、world/mounted 证书 67 项、启动采样与分类 64 项。检查覆盖提示误报、完整身份改变、本地玩家/Pawn 限制、重试耗尽、发布前拒绝、超时、退避及清理。日志位于 ignored `analysis/runtime/mounted-latency2-*-tests.log`。

原版对照回归 `mounted-latency2-before-actual-20261021.json` 完成第一次 checkpoint 的新 generation 和独立 XR projection 回执；Prepare 到 owner 确认 committed 为 312.7694 ms，worker 为 1436.3792 ms，其中 observe 207.0149 ms、prepare 1229.3643 ms。随后诊断 GPU PPM 请求没有产生新文件，该轮严格记录为失败，不能称作两次 checkpoint 或像素验证通过。

最初两轮新版回归在 native 激活前被只读采样拒绝，分别为 PID 29764、19128；尚未执行本轮切换路径。原报告保留为失败，游戏与 owner 正常退出，五份设置/存档文件均还原。两份失败回执中的样本全部为 timestamp/有效 registry growth 暂态，修复分类后仅允许重采，未接受其中任何样本。

新版第三轮 `mounted-latency2-after3-actual-20261021.json` 使用同一默认 native/XR，第一次 checkpoint 的 generation、native 发布进展和独立新世界 XR projection 回执通过。该次实际触发一次采集路径（`single_acquisition=true`），worker 1378.9826 ms、独立 observe 0 ms，Prepare 到 owner 确认 committed 为 206.9238 ms。对照只各有一次样本，准备时长仍有波动，不能把这组数字当作稳定性能比例或头显上下机枪黑屏时长。

第三轮第二次菜单操作未匹配到预期 Resume 状态，按既有状态保护停止；整体 `passed=false`、`Exact checkpoint state unavailable: resume`，没有改记为两次恢复通过。退出时 owner 返回 0、native cleanup 通过、79 项入口恢复检查全部匹配、游戏正常关闭、五份文件还原，没有剩余游戏。该轮不要求诊断 PPM，不报告逐世界 GPU 像素检查通过。

完整准备仍是主要耗时。后续 `mounted-latency2-profile` 测试收窄为一次 checkpoint 加一次只读完整候选性能采样，候选在发布前明确拒绝；通过条件明确使用 `single_reload_verified`。这是一轮新的独立检查，保留此前失败报告。该采样正在等待另一任务的 FPS 工作者，不打断其现有测试。

## 集成与边界

本轮 Python 源文件由正常启动器下次启动加载，不需要选择实验入口。native 默认 SHA256 为 `94b73bb04b6f36d4a4bc16f84f1e95d49a1a3144792e0e41d004ef3b802b4304`；XR 默认 SHA256 为 `0a2b7f7dc6a65f8ee23c255423e5fac841726ac6f2ef9bc249ac8e5f4902d56c`。本轮没有替换它们或覆盖默认清单。

完整采集和排空仍需时间，不能据轮询间隔宣称恢复只需 10 ms，也不能将 checkpoint 数字当作头显上下机枪的黑屏时长。头显及实际机枪验收随积累修复统一进行，尚不宣称完全消除等待。

私有源码、摘要和运行证据位于 ignored `analysis/mounted-latency2-source-20261021/` 与 `analysis/runtime/mounted-latency2-*`，不进入发行包。测试采用已验证的 behind-windows game/simulator、只读 tiny-output 与后台静音，禁用物理控制器后台输入，等待其他 FPS 工作者，不打断其他测试。
