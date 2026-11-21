# Changelog

## [0.0.3](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.3) — 2026-10-23

### Added

- Switchable first-person preview: L3 + R3 on a gamepad or E + R on the keyboard. Unscoped sniper weapons support the preview; scoped aiming retains the original camera.
- Right-stick deadzone control from 0 to 50 percent, default 2 percent.
- A main-menu happyotaku playground with eleven weapons/throwables, six respawning targets, a mounted gun and unlimited reserve ammunition. Normal magazine use, reloads and active reloads remain.
- An editable game-path history dropdown with browsing, autocomplete and up to ten recent installations.

### Fixed and improved

- Improved cover-aim state handling and added the missing binocular sniper-scope material path.
- Improved mounted-gun/world transitions, stale-pose handling, recovery retries and configuration preparation. The user considers the current transition delay acceptable; zero delay remains a future goal.
- Optimized stereo frame tracking and shared-image delivery. No universal FPS gain or 90 FPS guarantee is claimed.
- Improved Steam online/offline settings selection and per-installation configuration handling, including compatible versions found "elsewhere". 😉
- Added SteamVR menu-format compatibility. The release keeps the original runtime-dependent color selection after the requested rollback.
- Retained 0.0.2 startup compatibility, non-English path handling and startup exports; launcher/build reports identify as 0.0.3.

### Validation and limits

This is the same development feature set previously called 0.1.0, renumbered to 0.0.3 at the user's request. First-person and Playground have recorded game/simulator validation; individual fixes retain their documented physical-headset limits. A local left-eye-edge white-flash observation remains tracked, and color selection/offline GPU tests do not certify that it is fixed. DLSS/DLAA remain disabled; full 6DoF weapon handling and physical reloading are not included. Later chapters and every headset/runtime combination have not all been verified.

Close the game and launcher before updating. Extract the complete new ZIP over the previous mod installation, keeping runtime and tools together with GearsVR Launcher.exe. See the bundled README.txt for controls and reporting problems.

### CREDIT Chinese

- Add a first person to the preview, L 3＋R 3/E+R switch; unopened sniper gun can be previewed and the mirror is maintained with the original mirror camera.
- Add Right Rock Dead Zone 0–50%Default 2%。
- Add the happyotaku playground:11 Weapons/throwing,6 Re-entry targets, fixed machine guns and unlimited spare ammunition, with normal and active filling.
- Adds editable drop-down history, browse and auto-completion of the last ten game paths.
- Users confirm that the current machine gun switch is acceptable and continue to optimize completely non-delayed targets.
- Improving Steam's online/offline startup experience and taking care of compatible versions that players have found from “other places”.
- Compatible with the SteamVR menu format, returning to the original colour-format path according to running time capabilities as requested by the user.
- Reservations 0.0.2 Start, Chinese path and diagnostic rehabilitation, harmonization of markings 0.0.3;old 0.1.0 Develop functionality to publish in this version.

First person claims to be a preview. Local left eye edges are kept white and can not be declared resolved by simply rolling back or off screen; DLSS/DLAA is closed without 6 DoF has a feeling of having a gun and filling it in.90 FPS, Subsequent Chapters and Heads up Groupings retain the validated boundary.

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

### CREDIT Chinese

0.0.2 Based on 0.0.1 The initial compatibility restoration:

- PowerShell recovery downloaded scripts are intercepted by Windows Implementation Strategy; settings are set for subprocessors that only function on the trigger.
- Provides the correct Steam application context for the original game, avoiding the loss of the starter after Steam has re-raised; please keep Steam running and login.
- Select the draw setting for the current Steam account when multi-account settings are saved.
- Supports the automatic search of a Mod subdirectorial at the root directory, EXE and below the above position, and maintains a valid saved path as a matter of priority.
- Keeps the complete startup error output and shows the specific reason for failure in the starter.
- UTF-Unified 8 Detecting Python, repairing problems of miscalculating the package under non-English routes such as Chinese.
- Enhancement of the startup log: Exporting the last three start-up phases time, component fingerprints, package Python selection, context for the Steam subprocess and independent thread/play process exit status; `build-info.json`、`startup-attempts.json` and `startup-summary.txt`, can also be exported if you have not yet entered VR. Writing a log failure does not affect start-up and clean-up.

Turns the game and starter off before updating, depresses the whole package to its original position and overwhelms the Mod file, and keeps `runtime`、`tools` with the starter. There is no need to install Python.

Native Rendering and OpenXR Programs Follow 0.0.1 Release binary. Through the active version, submission of simulator menus, depression and automatic check, users have given feedback to initiate recovery;0.0.2 The return of the independent head is still to be measured. 3 The result is that 0.0.1 DLSS/DLAA remains to be shut down, machine guns entering/departing may still be suspended for about two seconds and the subsequent sections are not fully validated.

## [0.0.1](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.1) — 2026-10-21

- First public test release of the third-person VR mod for Gears of War: Reloaded.
- Includes the Windows launcher, bundled Python, System OpenXR / VDXR selection, render scale, camera distance, optional recentering, and diagnostic export.
- Act 1 was tested with KRVR + Apple Vision Pro and Virtual Desktop + Meta Quest 3.
- The first third person, VR Open Test Edition, provides a starter, package Python, OpenXR/VDXR selection, graphics and lens distance adjustment, as required, and log export. The first act has been tested and both headline combinations have been tested.

---

## 中文

# Changelog

## [0.0.3](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.3) — 2026-10-23

### Added

- Switchable first-person preview: L3 + R3 on a gamepad or E + R on the keyboard. Unscoped sniper weapons support the preview; scoped aiming retains the original camera.
- Right-stick deadzone control from 0 to 50 percent, default 2 percent.
- A main-menu happyotaku playground with eleven weapons/throwables, six respawning targets, a mounted gun and unlimited reserve ammunition. Normal magazine use, reloads and active reloads remain.
- An editable game-path history dropdown with browsing, autocomplete and up to ten recent installations.

### Fixed and improved

- Improved cover-aim state handling and added the missing binocular sniper-scope material path.
- Improved mounted-gun/world transitions, stale-pose handling, recovery retries and configuration preparation. The user considers the current transition delay acceptable; zero delay remains a future goal.
- Optimized stereo frame tracking and shared-image delivery. No universal FPS gain or 90 FPS guarantee is claimed.
- Improved Steam online/offline settings selection and per-installation configuration handling, including compatible versions found "elsewhere". 😉
- Added SteamVR menu-format compatibility. The release keeps the original runtime-dependent color selection after the requested rollback.
- Retained 0.0.2 startup compatibility, non-English path handling and startup exports; launcher/build reports identify as 0.0.3.

### Validation and limits

This is the same development feature set previously called 0.1.0, renumbered to 0.0.3 at the user's request. First-person and Playground have recorded game/simulator validation; individual fixes retain their documented physical-headset limits. A local left-eye-edge white-flash observation remains tracked, and color selection/offline GPU tests do not certify that it is fixed. DLSS/DLAA remain disabled; full 6DoF weapon handling and physical reloading are not included. Later chapters and every headset/runtime combination have not all been verified.

Close the game and launcher before updating. Extract the complete new ZIP over the previous mod installation, keeping runtime and tools together with GearsVR Launcher.exe. See the bundled README.txt for controls and reporting problems.

### 简体中文

- 新增第一人称预览，L3＋R3／E＋R切换；未开镜持狙击枪可用预览，开镜保留原镜内相机。
- 新增右摇杆死区0–50%，默认2%。
- 新增happyotaku playground：11种武器／投掷物、6个重生靶、固定机枪和无限备用弹药，保留正常及主动装填。
- 新增最近十条游戏路径的可编辑下拉历史、浏览与自动补全。
- 改善掩体瞄准、狙击镜双眼绘制、上下机枪／场景恢复及双目帧传输。用户确认当前机枪切换延迟已可接受，继续优化完全无延迟的目标。
- 改善 Steam 在线／离线启动体验，也顺手照顾了一下玩家从“其他地方”找来的兼容版本。😉
- 补齐SteamVR菜单格式兼容，按用户要求回到原有的按运行时能力选择颜色格式的路径。
- 保留0.0.2启动、中文路径与诊断修复，统一标识0.0.3；原0.1.0开发功能改以本版本发布。

第一人称仍是预览。局部左眼边缘闪白保留跟踪，不能仅凭颜色路径回滚或离屏检查宣称已解决；DLSS／DLAA关闭，不含6DoF体感持枪与体感装填，不承诺90FPS，后续章节和各头显组合保留验证边界。

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
