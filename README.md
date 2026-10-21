GearsVR 0.0.2

Install and play

1. Open the normal game first. Set Display mode to Windowed / Borderless, the frame rate limit to Unlimited, and DLSS / DLAA to Off. Apply the settings and exit the game completely. Keep Steam running and signed in.
2. Extract the whole ZIP into the game's root folder. Keep GearsVR Launcher.exe, runtime and tools together, beside Binaries_x64.

Gears of War Reloaded/
  GearsVR Launcher.exe
  Binaries_x64/GOWDE-Steam.exe
  WarGame/
  runtime/
  tools/

The launcher can also find the EXE when the mod is beside it inside Binaries_x64, or in one GearsVR subfolder under either location. Otherwise use Browse to select Binaries_x64/GOWDE-Steam.exe.

3. Connect your headset and open GearsVR Launcher.exe. System OpenXR runtime is selected by default. Virtual Desktop users can select Quest 3 / Virtual Desktop (VDXR).
4. Click Start VR and enter a level normally. Use your usual keyboard/mouse or gamepad. Keep the game in the foreground for gamepad input.

Recenter is optional. If the view feels comfortable, just play. Use it only when you want to realign your seated position and forward direction.
Lower Render scale if performance is poor. Camera closer = 0.4 is a suggested starting point for a closer view. Restart the game and VR after changing these settings.

What's changed

0.0.2 fixes Windows blocking the downloaded startup helper, Steam restarting the selected game into another process, multiple Steam-account settings files, and EXE auto-detection beside the launcher. Startup failures now retain the helper's detailed error output.
The renderer is unchanged from 0.0.1. Act 1, KRVR + Apple Vision Pro and VD + Quest 3 were tested with that version. The 0.0.2 changes were checked for startup on the retail game; a new physical-headset regression is pending.

Known limits

- Supported game EXE version: 1.0.381.6947.
- Mounted machine-gun entry/exit may pause for about 2 seconds.
- DLSS / DLAA remain disabled; keep them off. VR depth-of-field blur is also disabled.
- Later chapters have not all been verified.

Click Stop VR, wait for cleanup, then exit the game normally. Exit the game before starting a new VR session.
For a bug report, click Export logs and send the ZIP with your headset, GPU and a description. The ZIP includes startup-summary.txt and the latest three startup attempts, including failures before VR starts.
Before updating, close the game and launcher. Extract the new full ZIP into the same location, replacing the previous mod files. Keep runtime and tools together with the launcher.
The game is not included. No separate Python installation is required.


简体中文

安装与启动

1. 先正常打开游戏，将显示模式设为窗口化或无边框、帧率限制设为不限制，并关闭 DLSS / DLAA。应用后完全退出游戏，保持 Steam 正常运行并登录。
2. 把整个 ZIP 解压到游戏根目录，让 GearsVR Launcher.exe、runtime、tools 与 Binaries_x64 并排。启动器也支持放在 EXE 旁，或上述位置下的一层 GearsVR 子目录；未自动识别时点 Browse 手动选择 Binaries_x64/GOWDE-Steam.exe。
3. 连好头显并打开启动器。默认使用系统 OpenXR；Virtual Desktop 用户也可手动选择 VDXR。
4. 点 Start VR，正常进入关卡，使用原来的键鼠或手柄游玩。手柄操作时保持游戏在前台。

Recenter 可按需使用，不需要每次进入关卡都按；视角舒服就直接玩。卡顿可降低 Render scale；想拉近镜头可试 Camera closer = 0.4。参数修改后重启游戏和 VR 生效。

0.0.2 修复下载脚本被 Windows 拦截、Steam 重启到另一进程、多账号设置文件识别及 EXE 自动查找，并保留完整启动错误。原生渲染组件与 0.0.1 相同；已有的第一幕、KRVR+AVP、VD+Quest 3 体验来自 0.0.1，本版新增正版启动验证，真头显回归待复测。

支持游戏 EXE 版本 1.0.381.6947。机枪进入/退出仍可能暂停约2秒，后续章节未全部验证，DLSS/DLAA 和 VR 景深模糊仍关闭。
结束时点 Stop VR，等待清理后正常退出游戏。重新启动 VR 前先退出游戏。
遇到问题请用 Export logs 导出 ZIP，附上头显、显卡和问题描述。ZIP内包含startup-summary.txt和最近三次启动记录，也能记录进入VR前的启动失败。
更新前关闭游戏与启动器，将新整包解压到原位置并覆盖上一版 Mod 文件，让 runtime、tools 与启动器保持在一起。
不包含游戏文件，不需要另装 Python。
