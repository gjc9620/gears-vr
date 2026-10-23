GearsVR 0.0.3

Install and play

1. Open the game normally first. Set Display mode to Windowed / Borderless, Frame rate limit to Unlimited, and DLSS / DLAA to Off. Apply, then exit the game completely. Keep Steam running in your usual online or offline mode.
2. Extract the complete ZIP into the game's root folder. Keep GearsVR Launcher.exe, runtime and tools together, beside Binaries_x64.

Gears of War Reloaded/
  GearsVR Launcher.exe
  Binaries_x64/GOWDE-Steam.exe
  WarGame/
  runtime/
  tools/

The launcher can also find the game when placed beside its EXE, or in one mod subfolder under either location. Use Browse if needed. Game path now remembers up to ten installations and also accepts direct input.

3. Connect your headset and open GearsVR Launcher.exe. System OpenXR runtime is the default. Use your preferred OpenXR runtime; Virtual Desktop users can also select Quest 3 / Virtual Desktop (VDXR).
4. Click Start VR, enter a level, and play with your usual keyboard/mouse or gamepad. Keep the game in the foreground for gamepad input.

New in 0.0.3

- First-person preview: enabled in the launcher by default. Each new game starts in third person; press L3 + R3 on a gamepad or E + R on the keyboard to switch, then release both keys before switching again. This uses the original game controls, not tracked motion-controller aiming.
- Right-stick deadzone: adjustable from 0 to 50 percent; default 2 percent.
- happyotaku playground: select the new main-menu entry to try eleven weapons/throwables, six respawning targets, a mounted gun and unlimited reserve ammunition. Normal magazine use, reloads and active reloads remain.
- Improved cover aiming, sniper-scope handling, mounted-gun/world recovery, stereo frame delivery, Steam offline settings selection and game-path history.

Recenter is optional. Lower Render scale if performance is poor; Camera closer = 0.4 is a suggested third-person starting point. Restart the game and VR after changing these options.

Updating

Close the game and launcher, then extract the full 0.0.3 ZIP into the same location and replace the previous mod files. Keep runtime and tools together with the launcher. Recheck your game path and preferences after updating.

Known limits and reports

- Supported game EXE: 1.0.381.6947. Other builds, including Windows Store, have not been established as compatible.
- First-person is a preview. Close walls, cover, movement animations, weapon sights and comfort still have testing limits. Full 6DoF weapon handling and physical reloading are not included.
- Mounted transitions have improved and the user considers their current delay acceptable. Zero delay is not claimed.
- DLSS / DLAA remain disabled. Later chapters and every headset/runtime combination have not all been verified.
- A local left-eye-edge white-flash report remains tracked; color-path changes and offline GPU checks do not by themselves certify a headset fix.

Click Stop VR, wait for cleanup, then exit the game normally. Exit the game before starting another VR session.
For problems, click Export logs and send the ZIP with your headset, GPU and a description. It includes the latest three startup attempts and startup-summary.txt, even if VR never started. For startup failures also include analysis/runtime/vr-launcher-session.reserve.log from the mod folder.
The game is not included. No separate Python installation is required.


简体中文

安装与启动

1. 先正常打开游戏，将显示模式设为窗口化或无边框、帧率限制设为不限制，并关闭 DLSS / DLAA。应用后完全退出游戏，保持 Steam 运行，可使用正常在线或离线模式。
2. 将整个 ZIP 解压到游戏根目录，让 GearsVR Launcher.exe、runtime、tools 与 Binaries_x64 并排。也支持 EXE 同目录或上述位置下的一层 Mod 子目录。未自动识别时用 Browse 选游戏；路径输入框现支持最近十条历史和直接输入。
3. 连好头显，打开启动器。默认使用系统 OpenXR；VD 用户也可选择 VDXR。
4. 点 Start VR，正常进入关卡，用原来的键鼠或手柄操作；使用手柄时保持游戏在前台。

本版新增第一人称预览：启动器默认启用切换能力，每次新游戏仍从第三人称开始，L3＋R3 或 E＋R 切换，松开组合键后可再次切换。右摇杆死区可调0–50%，默认2%。主菜单新增 happyotaku playground，提供11种武器／投掷物、6个重生靶、固定机枪和无限备用弹药，保留正常与主动装填。

改善掩体瞄准、狙击镜、上下机枪／场景恢复、双目帧传输及 Steam 离线配置识别。也顺手照顾了一下玩家从“其他地方”找来的兼容版本。😉

Recenter 按需使用。卡顿可降低 Render scale；第三人称想拉近镜头可试 Camera closer = 0.4。修改后重启游戏和 VR。
更新前关闭游戏与启动器，把新版整包解压到原位置并覆盖 Mod 文件，保留 runtime、tools 与启动器；更新后核对路径和偏好。

支持的 EXE 为1.0.381.6947，未建立其他构建／Windows Store版兼容结论。第一人称仍是预览，不包含6DoF体感持枪或体感装填。上下机枪延迟用户反馈已可接受，但不宣称完全无延迟。DLSS/DLAA仍关闭；后续章节、近墙／动画／瞄具及各头显组合保留测试边界。已记录的左眼边缘局部闪白仍需跟踪，颜色路径调整和离屏检查不等于真头显问题已解决。

结束时点 Stop VR，等待清理后正常退出游戏。遇到问题请用 Export logs 导出ZIP，附头显、显卡和问题描述；未进入VR也可导出最近三次启动记录及 startup-summary.txt。启动失败时额外附上 analysis/runtime/vr-launcher-session.reserve.log。
不含游戏，无需另外安装 Python。
