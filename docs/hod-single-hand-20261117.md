# 黎明之锤单手候选（2026-11-17）

本次范围：右手单手取枪、举枪/枪口指向、长按右 R2 使用原版锁定/预热/天空光柱、收起/换出或释放时停止。无需副手、双手或换弹。按“指哪打哪”默认采用室内可用语义；只放宽当前实际持握且右 R2 按下的 HOD Tick 中原版目标合法性结果，原版命中、冷却、锁定计时、Beam、音频和伤害流程保留。

唯一协作 worker：`01a1e54e-2089-7aa1-a06c-ccb96ec3fe22`，Kaku / DeepSeek，现有侧栏条目保留。主会话审阅并修正其关键实现与不实验收结论；没有增加第二个 worker。

私有启动入口：`J:/gearsVr/candidates/hod-single-hand-20261117-task55/GearsVR Launcher.exe`。未替换原启动入口。最终 Playground SHA256 `495ebf750c60c71c5cfca50dc6cfe742fda094bc1d9f17dbe2efeca20b8376a0`；Native `022520be9b05ff755cf3e4a593f8d0bcd594d765e550e645999c211d4aff9464`、XR `aa6f3ba5ea1937baa96341e7f356cc9fb3ed3521baaba69d56623584786987e8`。

构建源在 `analysis/hod-20261117/task-45/source`。该私有副本保留 revision4/35° wrist 配置，与实际导入 Native 一致；共享源的另一会话 revision5/55°配置未回退。Native二进制实际比较校准常量，原先 Native35/PG55 导致 READY 回调被拒绝；配对后该问题已排除。HOD-only 改动包含现有持握交接、原版枪口射线、室内合法性和清理；未新增 Native hook 或 wire ABI。

室内判定通过现有统一 CallFunction 参与者读取并验证实际 HOD Tick 的 FFrame、当前世界/manager/持握/新鲜输入和 Bool/Vector 方法签名，只提交该次返回值。Tick 从存活调用帧认证，不长期占据射线 scope，避免阻碍下一次 GT 准备。旧 source 和输入边沿没有被重新标新或重放。

收枪时先执行并确认原版 OFF；可见光柱、目标效果、输入状态均已停止后允许交接。尚存音频淡出时保留精确原拥有者的清理记录继续观察；未知调用或效果未停仍拒绝交接。该候选没有清掉原来的效果债务以伪造成功。

唯一最终行为记录为 `analysis/hod-20261117/task-62/actions.json`：`passed=true`，原版预热及音频、实际 Beam firing、R2 持续按下时 put-away、松开扳机前停止、最终效果停止均有记录。实际路径为收起 HOD，未演示另选或开火另一把枪；不要声称所有切换组合或音频尾音分支均已逐项运行。模拟源曾中断一次，松开并重新扣扳机后完成，录像如实保留。未进行实体头显贴合/舒适度验收。

最终录制已正常退出：WGC exit0，官方 done `launcher_exit=0`、`owned_game_absent=true`、`owned_children.remaining=[]`、`forced_termination=false`、`worker_error=null`。完成后不再追加运行或测试矩阵。

录像：`analysis/hod-20261117/task-62/hod-single-hand-demo.mp4`，约47秒，实际 VR 双眼镜像、1倍速、静音、中文字幕。按 `.agents/skills/gearsvr-demo-video/SKILL.md` 使用 pythonw + 无窗口 FFmpeg 编码并等待退出；源帧/采集时间/输入/ASS/脚本保留。普通游戏窗口在部分时段为 WAITING FOR VR FRAME，故视频使用本轮同时采集的实际 XR 镜像，没有用等待卡冒充游戏画面。

演示观察已去掉新 Beam 出现后重复完整校验游戏 EXE/扫描对象表的路径：当前对象身份直接通过当前 live registry slot 与头部边界验证，并在完整读取末尾复核。没有放宽产品100ms输入/姿态保护。动作范围仅限本次要求，未追加旧矩阵、其他武器或握持全角度检查。

右手主握沿用现有 authored profile。本地已参考的 owner 导出实采条目是左手支撑；没有将其替代右手主握，也不据此否认 owner 可能另有捕获。后续根据实际头显反馈调整，不新增本轮验收门槛。

## 最新复录：task67（70ea3af9）
用户要求修复后完整重录。原版预热、Beam开火和松开停止成功，但R2持续按下时收枪stow_failed；整条流程passed=false。录像约48秒并如实标注失败。不得用task62旧成功声称修复后版本已全流程通过；后续只需针对收枪失败处理。见analysis/hod-20261117/task-67/demo-route.md。
