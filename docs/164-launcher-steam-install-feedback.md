# 启动器 EXE 同目录识别与启动错误反馈

2026-10-21，玩家 Notagameaddict 反馈：Mod 已放在游戏 EXE 旁，启动器没有自动填入游戏路径。截图中手动选择的文件为 Steam 游戏目录下的 `Binaries_x64/GOWDE-Steam.exe`；VDXR 已返回每眼 2496×2688 的推荐尺寸，随后出现 `Fresh reservation failed; no retry`，启动退出码 1。

用户确认对方尚未发送详细日志。因此自动识别缺陷已经确认，但不能把截图中的启动失败归因于路径、DLSS、VDXR 或游戏版本，也不能把识别修复称为该玩家启动问题已经解决。

## 修改

GUI 原先只查找启动器根目录下的 `Binaries_x64/GOWDE-Steam.exe`，漏掉启动器本身就在 `Binaries_x64` 的安装方式。现在保留已保存且仍存在的路径，否则依次查找启动器目录中的 `GOWDE-Steam.exe`、`Binaries_x64/GOWDE-Steam.exe`，再在父目录查找同样两个位置。这样也覆盖解压到一层 `GearsVR` 子目录的情况。查找不依赖当前工作目录，最多上查一层；真正启动时继续执行既有 EXE、配置与构建哈希验证。

Python 启动预留失败时，优先显示子进程原始 Python 异常，避免末尾的 `CalledProcessError` 包装盖住根因；没有异常行时显示有界的错误尾部。提示包含子进程退出码和详细日志路径，完整输出仍保存在本地 `.reserve.log`。错误文本最多 1500 字符；现有会话状态与脱敏诊断导出继续保留其摘要。子进程输出解码替换无效字符，避免解码失败遮住原始启动错误。保留单次启动和按进程身份清理的既有行为。

## 未确定的启动错误

截图里的 `Fresh reservation failed` 是父进程汇总，不能确定失败发生在自检、启动前策略、进程创建、依赖等待还是注入阶段。`Check eye size`/运行时尺寸查询成功也不证明实际 XR 会话或 GPU 出图已通过。

源代码中另有一条可核对的可能原因：`Start-DebugGame.ps1` 对外部安装执行 `graphics_policy.py check`；如果用户已保存 `DLSS=True`，会以 `Persisted DLSS must be OFF before release startup` 拒绝启动。此处仅是代码条件，不是对该玩家现场的判断。本次不改变该策略。可请玩家先在普通游戏内关闭 DLSS、正常退出游戏后重试。

若仍失败，需要该 Mod 目录下 `analysis/runtime/vr-launcher-session.reserve.log` 末尾 20–30 行。原版诊断包不保证包含这份原始子进程日志；更新后的启动器会把提取到的原因保留在界面和导出摘要里。

## 验证与交付边界

GUI 隐藏自检覆盖 EXE 同目录、游戏根目录、一层 Mod 子目录、已有有效绝对/相对路径优先、无效路径回退、中文与空格目录、无关工作目录和最多一层父目录的边界。Python 回归覆盖具体错误展示、包装异常剥离、缺失输出与长度限制、完整日志保留、单次调用、失败清理以及尚未建立立体会话时的脱敏导出。

142 项 Python 检查及 GUI 编译、隐藏自检通过，覆盖启动器、诊断导出、便携暂存和打包四个模块。正常 GUI 已同步更新，当前默认 native/XR 清单和用户配置的 SHA256 与修改前一致。未覆盖已发布的 0.0.1 ZIP 或线上资产。本次验证不启动游戏，不新增实际游戏、GPU 或物理头显验收结论。验证回执保存在忽略目录 `analysis/feedback-launcher-20261021/result.json`。
