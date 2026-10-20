GearsVR 0.0.1

Hey Gears! Welcome to GearsVR!

This is the first public test release of my third-person VR mod for Gears of War: Reloaded.
I've played through Act 1 and tested KRVR + Apple Vision Pro and Virtual Desktop + Quest 3.
Both setups are playable.

Let's get you in

1. Extract the whole ZIP and keep the files together. Don't run it from inside the ZIP.
   Put the mod next to the game's Binaries_x64 folder if you'd like the launcher to find the game for you.
   Keeping it in a separate folder works too; you can browse to the game yourself.
2. Connect your headset and PCVR streaming software, then open GearsVR Launcher.exe.
   If the game is already running, close it first so the launcher can start it.
3. Check Game path. If Binaries_x64\GOWDE-Steam.exe is next to the launcher, it fills in automatically.
   Otherwise, click Browse and select that file in your game installation. Your saved path is remembered.
4. System OpenXR runtime is selected by default, using your usual active OpenXR setup.
   You can also choose a specific setup:
   Quest 3 + VD: Quest 3 / Virtual Desktop (VDXR).
   AVP + KRVR: set up KRVR OpenXR first, then choose System OpenXR runtime.
   You can also select a specific runtime JSON if that's how you normally connect.
5. Click Start VR, wait for the game, and enter a level as usual.
   Keep the launcher open. Sit comfortably, face forward, and click Recenter.

You can switch between English and Chinese at the top right.
Use the usual keyboard/mouse or gamepad controls; VR motion-controller weapon handling isn't in this version.
Keep the game window in the foreground when using a gamepad.

Make yourself comfortable

I tested with in-game 4K, medium settings and DLSS off, as a starting point.
If it feels choppy, lower Render scale from 1.0 to something like 0.8 or 0.7.
You'll lose some sharpness, but it should ease the GPU load.
VR eye size uses the OpenXR recommendation multiplied by Render scale; game resolution controls the desktop mirror.
Use Check eye size to see the requested size. In-game 4K doesn't mean 4K per eye.

Want to get a little closer to the action? Try Camera closer. I like 0.4, but see what feels good to you.
0 keeps the original distance, and it goes up to 0.5 metres. You can adjust HUD size as well.
After changing these settings, exit and restart the game and VR to apply them.

A few things to know

- Getting on or off a mounted machine gun may still pause for about 2 seconds. Give it a moment.
- DLSS isn't compatible yet, so I've disabled it in the mod. Turning it on in the game menu won't enable it.
  DLAA and depth-of-field blur in VR scenes are also disabled for now.
- I've completed Act 1; I haven't tested all the later chapters yet. Let me know how you get on!

When you're done, click Stop VR, wait for it to finish, then exit the game normally.
To start VR again, close the game first and relaunch it from the launcher.
For updates, close the game and launcher, extract the new version into a fresh folder, and select your game again.
To remove the mod, close everything and delete its folder.

Found a bug?

Click Export logs, save the ZIP, and send it to me or post it on Discord.
Let me know your headset, connection method, GPU, and where and how it happened.
A screenshot or video helps too. The export saves a local file for you to send.

The game isn't included. The currently supported EXE file version is 1.0.381.6947.
If the launcher reports a version mismatch, let me know as well.

Thanks for playing and helping out, Gears. Have fun!


简体中文

Hey Gears！欢迎来试试 GearsVR！

这是我做的《战争机器：重装上阵》第三人称 VR Mod，第一个公开测试版本。
我自己已经打通了 Act 1，也试过 KRVR + Apple Vision Pro 和 Virtual Desktop + Quest 3，
这两套组合都可以玩。

先玩起来

1. 把压缩包完整解压，里面的文件放在一起，不要直接在压缩包里运行。
   放在游戏的 Binaries_x64 文件夹旁边，启动器就能自动找到游戏。
   你也可以放在单独的 Mod 文件夹里，再手动选择游戏路径。
2. 先连好头显和 PCVR 串流，再打开 GearsVR Launcher.exe。
   如果游戏已经开着，先退出游戏，让启动器来启动。
3. 看一下 Game path / 游戏路径：同目录下有 Binaries_x64\GOWDE-Steam.exe 就会自动填入。
   没有自动找到的话，点 Browse / 浏览，选中游戏里的这个文件。已有的有效路径会继续保留。
4. 默认选择 System OpenXR runtime / 系统 OpenXR，使用你平时配置的运行环境。
   也可以按需要手动切换：
   Quest 3 + VD：选 Quest 3 / Virtual Desktop (VDXR)。
   AVP + KRVR：先配好 KRVR 的 OpenXR，再选 System OpenXR runtime。
   如果你平时用指定的 Runtime JSON，也可以在启动器里选择它。
5. 点 Start VR / 启动 VR，等游戏打开后，照常进入关卡就可以了。
   启动器保持开着，坐好、看向正前方，再点一下 Recenter / 视角回中。

右上角可以切换中文和 English。操作还是用原来的键鼠或手柄，
这一版还没有 VR 手柄持枪。用手柄时，让游戏窗口保持在前台。

调到自己舒服就好

我测试时用的是游戏内 4K + 中等画质 + DLSS 关闭，供大家参考。

如果觉得卡，先把启动器里的 Render scale / 渲染倍率降一点，
比如从 1.0 试到 0.8 或 0.7。画面会没那么清楚，但通常能轻松一些。
VR 的每眼分辨率由 OpenXR 推荐尺寸和这个倍率决定；游戏内分辨率主要影响桌面镜像。
想看具体尺寸，可以点 Check eye size / 查看实际尺寸。这里说的游戏内 4K 不是每眼 4K。

如果觉得镜头离角色远了些、代入感还差一点，可以调 Camera closer / 镜头拉近。
我个人觉得 0.4 挺舒服，你也可以按自己的习惯来。0 是原来的距离，最大可以调到 0.5 米。
HUD size / HUD 大小也可以调，大一点更容易看清，小一点更集中。

这些参数改完后，退出游戏，再重新启动游戏和 VR 就会生效。

还有几个小问题

- 进入和退出固定机枪时，还是可能有大约 2 秒延迟，稍等一下。
- DLSS 目前还不兼容，我已经在底层关掉了，游戏菜单里打开也不会生效。
  DLAA 也保持关闭，VR 场景的景深模糊暂时关掉了。
- 我目前打通了第一幕，后面的章节还没全部测完，欢迎大家帮忙试试。

玩完以后

点 Stop VR / 停止 VR，等它结束，再正常退出游戏就好。
停止 VR 不会帮你关掉游戏；如果想再开 VR，要先退出游戏，再从启动器重新启动。
以后更新时，先关好游戏和启动器，把新版完整解压到新文件夹，再选一次游戏路径。
不想用了，退出后直接删除 Mod 文件夹即可。

遇到 Bug 记得告诉我

在启动器里点 Export logs / 导出日志，保存生成的 ZIP，发给我或者发到 Discord。
顺便说一下你用的头显、连接方式、显卡，以及在哪一关、做什么时出了问题。
方便的话，附张图或者视频就更好了。日志导出到你选的位置，需要你自己发送。

包里没有游戏，需要先装好游戏。目前匹配的 EXE 文件版本是 1.0.381.6947；
如果启动器提示版本不匹配，也请把情况反馈给我。

谢谢各位 Gears 的体验和反馈，玩得开心！
