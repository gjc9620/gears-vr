# Changelog

## [0.0.2](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.2) — 2026-10-22

Startup compatibility hotfix for the 0.0.1 public test release.

### Fixed

- Downloaded startup helpers can run when Windows script execution policy would otherwise block them. The execution-policy setting applies only to the launcher's PowerShell child process.
- The launcher supplies the correct Steam application context so the selected retail game stays in the process started by the mod. Keep Steam running and signed in.
- Graphics settings are selected for the active Steam account when multiple accounts have settings files.
- Game discovery supports the game root, the EXE directory, and one mod subfolder under either location. A valid saved game path keeps priority.
- Startup failures retain the helper's detailed output and show the underlying error in the launcher.
- UTF-8 interpreter probes correctly select bundled Python from Chinese and other non-English installation paths.

### Startup diagnostics

- Export logs now includes `build-info.json`, `startup-attempts.json`, and `startup-summary.txt`, including when startup fails before a VR session exists.
- The latest three startup attempts include stage timings, component fingerprints, bundled-Python selection, and the Steam application IDs supplied to child processes.
- Failure records distinguish remote-thread results from the original game's process exit status. Hook summaries are limited to the current startup attempt; diagnostic write failures do not interrupt startup or cleanup.

### Update and validation

Close the game and launcher, then extract the complete 0.0.2 ZIP over the previous mod installation. Keep `runtime` and `tools` together with `GearsVR Launcher.exe`. No separate Python installation is required.

The native renderer and OpenXR application retain the published 0.0.1 binaries. Retail startup, simulator menu submission, portable extraction, and 277 automated launcher/debug checks passed; the user also reported that the startup fix worked in their test. The added diagnostics were checked without another game launch. Act 1, KRVR + Apple Vision Pro, and VD + Quest 3 results remain the 0.0.1 baseline. A separate 0.0.2 physical-headset regression remains pending.

Known limits remain: supported game EXE `1.0.381.6947`, DLSS/DLAA off, roughly two-second mounted machine-gun transitions, and later chapters not fully verified.

### 简体中文

0.0.2 是基于 0.0.1 的启动兼容性修复版：

- 修复下载后的 PowerShell 启动脚本被 Windows 执行策略拦截；设置只作用于启动器的子进程。
- 为正版游戏提供正确的 Steam 应用上下文，避免 Steam 重新拉起游戏后启动器丢失进程；请保持 Steam 运行并登录。
- 多账号设置文件并存时，选择当前 Steam 账号的画质设置。
- 支持在游戏根目录、EXE 同目录或上述位置下的一层 Mod 子目录自动查找游戏，并优先保留有效的已保存路径。
- 保留完整启动错误输出，在启动器内显示具体失败原因。
- 统一使用 UTF-8 探测 Python，修复中文等非英文路径下误判随包 Python 不可用的问题。
- 增强启动日志：导出最近三次启动的阶段耗时、组件指纹、随包 Python 选择、Steam 子进程上下文及独立的线程/游戏进程退出状态；新增 `build-info.json`、`startup-attempts.json` 和 `startup-summary.txt`，尚未进入 VR 也能导出。日志写入故障不影响启动和清理。

更新前关闭游戏与启动器，将新版整包解压到原位置并覆盖 Mod 文件，保留 `runtime`、`tools` 与启动器。无需另外安装 Python。

原生渲染和 OpenXR 程序沿用 0.0.1 发布二进制。已通过正版启动、模拟器菜单提交、解压与自动检查，用户已反馈启动修复实测正常；0.0.2 的独立真头显回归仍待复测。第一幕、KRVR + AVP、VD + Quest 3 的既有结果属于 0.0.1。DLSS/DLAA 仍需关闭，机枪进入/退出仍可能暂停约两秒，后续章节未全部验证。

## [0.0.1](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.1) — 2026-10-21

- First public test release of the third-person VR mod for Gears of War: Reloaded.
- Includes the Windows launcher, bundled Python, System OpenXR / VDXR selection, render scale, camera distance, optional recentering, and diagnostic export.
- Act 1 was tested with KRVR + Apple Vision Pro and Virtual Desktop + Meta Quest 3.
- 首个第三人称 VR 公开测试版，提供启动器、随包 Python、OpenXR/VDXR 选择、画质与镜头距离调整、按需回中和日志导出。已测试第一幕及上述两种头显组合。
