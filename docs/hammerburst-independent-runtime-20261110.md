# 兽族步枪：独立测试接线与当前失败记录

工作区：`C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。本记录不改变整枪开发目标或验收范围。

## 当前运行限制

2026-11-10 收到主人经统筹转达的新要求：独立启动仍发生抢前台，暂停新的 game/XR 启动，不自行重启测试焦点，继续离线开发。本次没有启动游戏或模拟器；`hb-1110-181318` 仅完成准备和制品校验，不能算启动、实际武器回归或头显验收通过。

后续恢复启动时仍须遵守 `docs/weapon-development-20261109/independent-runtime-20261110.md`，并采用修复且实际验证过的后台/静音入口。禁止重新 stage/restore 共用 solo 实例，也不以环境变量存在代替实际后台行为验证。

## 本次实际修改

新增 `analysis/hammerburst-20261108/hammerburst_isolated_operator.py`。它只适配本枪的既有动作及只读观察脚本，不启动游戏，不修改共享脚本或生产武器机制。

- 输入命令、动作输出、launcher 观察、Playground 日志、停止入口全部指向传入的独立实例。
- 两个原先通过全机进程枚举寻找共享 XR 的读数脚本，改为验证本实例报告中的 XR PID、creation 和冻结 EXE 路径/哈希。
- 拒绝外实例输出路径、共享实例名、任意辅助脚本、Source 非 1、被复用的进程身份和变化的 XR 制品。
- 动作循环只核对已绑定的 owner/game/XR 进程身份，不在每次手部输入时重读 manifest 或整份 EXE，避免给原有输入时效链增加文件开销。
- 复用已有按 game/XR PID+creation 录制准确 HWND 的录像器；未新增桌面截图或全局输入。

新增 `check_isolated_operator.py`，14 项离线测试全部通过，覆盖身份复用、外实例/XR、制品变化、物理输入源、输出越界、辅助脚本越权、准确停止目标和适配代码编译。五份适配后的原脚本编译成功。这些只证明离线隔离路由及拒绝条件，不证明运行时录制、换弹或射击成功。

## 保留的实际游戏失败

旧 return-observation-r1，game34780：起始六发有效且来源一致；下一轮原始按下 event7 被捕获，但 frame231 的输入通过 `frame_active` 判为 inactive，库存 72/78 未变化。记录的复制输入年龄为 97.7204ms，复制耗时 15.3889ms；不能把它写成已观测到超过100ms，也不能把源中 trigger=1 写成实体松键。具体失败的 Frame 时间戳/形状条件尚未定位。没有到达本次新增的换弹后完整回射观察分支，整枪仍未通过。

证据：`analysis/hammerburst-20261108/runtime-34780-first-failure-analysis.json`。原 operator=1、wrapper=1 保留。游戏/Native/XR 正常收尾，但旧 wrapper 的全机 idle 检查将离线编码误当占用，阻止自动恢复。随后在没有其他活动实例且匹配旧候选的条件下，已准确恢复十项原始基线；恢复记录为 `standalone-return-observation-r1-recovery.json`，SHA256 `031468910137e9ca47f270420200eee766b4dce8cf007dc0aae90cbc65188e42`。手工恢复成功不改写原 wrapper 退出码。

更早 game56252 已实际完成空仓、取替换匣、入槽、锁扣关闭及原版 complete，回射六条真实射线；最后库存读数仍缺，不因此宣称完整回射闭环通过。原有四态护木、原匣返座、普通/完美/取消换弹、有限库存、打断退出、完整拟合及交付要求继续保留。下一次实际测试应在后台路径修复后进行，并继续定位当前首发源失效，不能重跑不变代码碰运气或放宽100/20/250ms保护。

## 枪栓结论

原版 Locust_Hammerburst 的待机/换弹/换弹失败动画只有 Root、MagLatch、Mag，未提供独立可操作枪栓/拉机柄。保留真实锁扣与弹匣交互，不虚构拉栓部件。详见 `analysis/hammerburst-20261108/hammerburst-bolt-original-audit.json`。
