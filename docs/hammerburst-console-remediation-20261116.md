# 兽族步枪会话：控制台弹窗整改

本轮目标是落实前述离线入口、子进程、拒绝检查和Video Demo技能规则，执行实际窗口验证后再完成。工作区J:/codexWorktree/gearsVr。本轮未启动游戏、未接管其他会话进程，也未重录或重新编码现有视频。

## 修改

- `analysis/hammerburst-20261115/invoke_hidden_python.ps1`：离线默认选择GUI子系统pythonw.exe；继续UseShellExecute=false、CreateNoWindow、重定向日志和显式等待。游戏运行所需的canonical python.exe例外收紧为三个完整路径，不再按同名文件放行。
- `official-owner-support-action/hammerburst_isolated_operator.py`：当前子进程读取器选择同目录pythonw.exe；缺失时拒绝执行，不回退python.exe。原本进程内读取仍在原进程内执行。正常停止游戏属于运行入口，原解释器规则保留。
- `official-owner-support-action/hb_recorded_operator.py`：原始窗口录像后处理改为GUI解释器，保留CREATE_NO_WINDOW、日志和退出码处理。
- `make_demo_video.py`：保留并验证无控制台句柄检查与GUI解释器要求；普通离线工作若使用控制台版解释器直接拒绝，不继续字幕/编码。只有明确的短测子进程模式允许验证canonical python.exe的无窗口创建行为。
- `finalize_normal_video_review.py`：补齐PowerShell子进程缺失的CREATE_NO_WINDOW；未重新执行其旧交付状态写入流程。
- `.agents/skills/gearsvr-demo-video/SKILL.md`：共享的离线GUI解释器、子进程无窗口和实际验证规则保留；不把它扩展成全局替换游戏解释器。

## 验证

`verify_console_remediation.py` 已实际执行：

1. 当前Python修改可编译。
2. 本会话分析目录顶层与当前official-owner-support-action目录的直接subprocess调用均显式设置creationflags；历史快照目录没有改写或重新运行。
3. GUI解释器正常选择，缺失时不降级。
4. 生产读取器及录像后处理调用点使用GUI解释器与无窗口参数。
5. 实际GUI读取器`--check`成功，编译适配器并确认game_started=false、runtime_verified=false，不声称验证了游戏交互。
6. 真实GUI字幕生成入口成功生成25段字幕；未编码视频。
7. canonical python.exe即使以CREATE_NO_WINDOW运行，正常离线录像入口也拒绝它，未生成后续处理产物。

实际窗口记录`console-remediation-goal-probe.json`：GUI父进程58148的console_hwnd=0，无本方可见窗口；短测子进程54572的console_hwnd=0；FFmpeg短测51488退出0，158次窗口采样无本方可见窗口。这些验证进程已结束。

汇总与文件哈希：`analysis/hammerburst-20261115/console-remediation-goal-review.json`；完整验证日志：`console-remediation-final-verification.stdout.log`和对应stderr。

## 范围与后续入口

当前正式本方脚本使用上述入口；旧active-stow、tracking等备份副本只保留为历史证据，不再作为当前运行入口。游戏启动仍复用isolated_sim.py，canonical python例外保留其安装校验兼容性；本轮没有重新运行游戏启动链，也没有声称其他会话独立的启动器已修复。共享技能更新不等于其他会话正在运行的进程自动迁移。

本轮完成结论仅覆盖已修改和已测试的本方离线启动链。之前只凭隐藏参数宣称不弹窗的结论不再作为验收证据。
