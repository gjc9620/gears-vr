# 双语实验启动器 / Bilingual experimental launcher

2026-10-21 玩家安装反馈修复：自动查找同时支持启动器旁的 `GOWDE-Steam.exe`、`Binaries_x64/GOWDE-Steam.exe`，以及上述目录下的一层 Mod 子目录；保留已有有效路径，最多向上一层查找。启动预留失败时直接显示原始子进程错误和退出码，并继续保存完整本地日志；启动前失败也可在导出日志里保留原因。见[反馈处理记录](../../../docs/164-launcher-steam-install-feedback.md)。

2026-10-21：默认 OpenXR 改为系统 Runtime；只有手动选择 VDXR 或模拟器时才覆盖子进程 Runtime。启动器 EXE/窗口图标使用用户提供的彩色图案，右下角为32px灰度淡化图案；ICO和小图嵌入EXE，不增加运行时外部图片依赖。原图保存在assets/launcher-artwork.png，build-icon.ps1生成16–256px图标及32px内嵌小图。以下旧日期的默认VDXR描述属于历史记录。

2026-10-21：打开启动器时保留已有有效游戏路径；路径为空或文件已不存在时，自动查找启动器根目录下的 `Binaries_x64/GOWDE-Steam.exe` 并填入。查找不依赖快捷方式的工作目录，不扫描其他磁盘；正常启动时继续执行原有游戏版本检查。找不到时仍可浏览选择，点击启动后按既有方式保存。

在仓库内用 Windows PowerShell 执行：

```powershell
powershell -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1
Start-Process '.\GearsVR Launcher.exe'
```

使用系统已有 .NET Framework 编译器，不安装依赖。构建会隐藏运行 `--self-test`，检查仓库路径、命令参数、状态解析、配置字段保留、HUD 能力门禁与数值范围、保存读回、owner 身份握手及 Python 可用性，不显示 GUI 或启动游戏。输出保存在忽略目录 `analysis/vr-launcher-ui/`，并复制到仓库根目录的 `GearsVR Launcher.exe`，可直接双击。

默认选择 Quest 3 / Virtual Desktop (VDXR)，也可明确切换系统 Runtime 或本地模拟器，然后启动。VDXR 使用 Program Files 下 Virtual Desktop Streamer/OpenXR/virtualdesktop-openxr.json；文件不存在时直接提示，不自动回退模拟器。游戏路径写入现有 `tools/vr-launcher/config.json`，保留其他字段。选择模拟器时使用 `analysis/runtime/simulator-quiet2/openxr_simulator.json`；系统选项让子进程使用系统 Runtime，清除继承的 `XR_RUNTIME_JSON`，不修改注册表。 quiet2 修复菜单交换链重建后编号复用、导致双眼拿到旧菜单纹理的问题；同一重建序列已在实际游戏验证左右眼使用独立新编号并持续投影超过90秒。这项修复不代表所有闪烁或黑屏均已解决。

模拟器预览保持可见，自动启动不获取桌面焦点；用户可以点击它或移到第二屏。正常连续 VR 会话没有自动结束计时，不主动最小化或重新定位预览。手动点击 GUI 的“启动 VR”会传入 --foreground，允许游戏正常前台启动，同时保留后台手柄支持。CLI 默认仍使用不抢焦点策略；静默方式不等于将游戏最小化。独立模拟器启动观察通过，游戏自行激活的行为仍需实际观察，见文档68。

启动调用 `launcher.py start --foreground --manifest tools/vr-launcher/manifest.json`，不会传入自动导航或结束计时参数。当前本地真机候选为 Native23 / XR18-vd：Native23 / XR16 已完成1080p和4K实际验证，XR18-vd 增加 VDXR 等待卡和菜单的类型化纹理上传兼容校验，真机完整体验仍在测试，HUD大小控件已启用；旧版可用 `manifest-game-recovery8.json` 回滚，仍依赖仓库工具和Python，不是便携发布包，也未提升发布状态。

Python 按 `GEARSVR_PYTHON`、随包 `runtime/python/python.exe`、当前用户 `.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`、PATH 中的 `py.exe` / `python.exe` / `python3.exe` 顺序探测。可通过 `GEARSVR_PYTHON` 指定完整路径。探测取得 `sys.executable` 后直接启动该 Python，确保跟踪的进程就是实际 owner。后台命令隐藏窗口，异步读取标准输出和错误。

默认 `tools/vr-launcher/manifest.json` 的布尔能力 `hud_size_control` 为 `true` 时，窗口显示“HUD 大小”角宽控件：45–100°，未配置时为 65°，已有 `hud_horizontal_fov_degrees` 会读回。调大更易读，调小更集中；点击“启动 VR”时连同游戏路径保存，下次启动生效。保存保留其余配置键。旧清单没有该能力或值为 `false` 时，不显示控件，也不新增角宽字段；旧 DLL 不会因此获得调整能力。清单能力在窗口打开后变化时，需要重新打开启动器。

武器 6DoF 开发清单显式启用 `controller_input_observer` 时，窗口另显示臂展测量行。没有有效的本机 `%LOCALAPPDATA%/GearsVR/weapon-calibration.json` 时首次测量为必选；已有有效记录时默认复用，玩家可勾选重新测量。点击启动会把重测选择交给 `launcher.py start --remeasure-armspan`，后端再交给现有双握持两秒测量器；失败不覆盖旧记录。清单还必须具备已认证的 crosshair／UI／anchor 观察能力，否则启动前拒绝。当前冻结的普通 `manifest.json` 不声明此手柄能力，因此普通启动器不显示这一开发中的选项，也不会在未经验证的默认 DLL 上假装完成校准。头显内的引导、槽位试抓确认及玩家真机验收仍待实现。

“停止 VR”只运行 `launcher.py stop`；不直接终止游戏或 owner。很早收到停止请求时，若 Python 尚未启动则取消本次启动；否则保留请求，等待索引中的 owner PID/创建时间与本窗口启动的 Python 匹配。launcher 写入该索引前已建立停止 event，身份确认后才调用 stop；等待期间不会假报停止成功。关闭仍在运行的本窗口会询问是否请求停止，并保留窗口等待 owner 退出；选择“否”继续运行。清理未完成时保留窗口与日志，可继续查看状态或再次停止。不要用任务管理器强制终止 GUI；异常关机或外部强制结束不在此关闭策略覆盖范围内。

“查看状态”调用现有身份检查状态接口，完整输出显示在窗口。日志保存在 `analysis/runtime/vr-launcher-ui.log`，可用“打开日志”定位目录。状态不自动刷新，需点击获取。启动按钮等待其 owner 完成后才能再次使用；原生资源退出限制与现有 launcher 相同。

启动器窗口中的“双目新帧”每秒读取当前会话的遥测，显示新发布的双目画面 FPS，并同时显示 OpenXR 循环 Hz。数据来自 `analysis/runtime/continuous-stereo-*.telemetry.jsonl`；只有会话 owner 匹配、样本新鲜且原生采样有效时才显示数字。启动前、采样中断或会话结束时显示等待数据。OpenXR 循环 Hz 不代表游戏新帧率，也不等于头显实际显示帧率。

“视角回中”用于本窗口启动的VR会话，复用 `launcher.py recenter`。请求前核对启动器owner身份，后端继续核对运行阶段、游戏/XR身份和原生会话；未运行、正在停止或已有回中请求执行中时不可重复点击。成功提示只表示已请求，实际应用仍由有效追踪帧决定，可用“查看状态”检查 `reference_generation`。回中保留真实抬头和侧倾，不更改游戏存档或镜头距离。

当前默认候选已更新为 Native32 / XR23（`manifest-material32-movie23-vd.json`），覆盖上述旧版本说明。独立 Low/High 分辨率功能已移除，启动器不提供画质档位或覆盖游戏分辨率；请在游戏设置中调整分辨率。HUD 大小仅调整界面角宽，不改变渲染分辨率。默认真机入口仍为 Quest 3 / Virtual Desktop；模拟器需要明确选择。

右上角的“中文 / English”选择器可立即切换界面文字、状态和启动器对话框。首次默认英文，已保存的中文选择仍会保留，选择会立即保存到现有配置的 `launcher_language`（`zh-CN` / `en`），下次启动读回；其余配置字段及尚未保存的界面输入会保留。后端原始日志和系统文件对话框按钮保持其原始语言，便于排查问题。

Select **中文** or **English** in the upper-right corner to switch the launcher immediately. The choice is saved as `launcher_language` (`zh-CN` / `en`) in the existing config and restored next time; other config fields and unsaved form inputs are preserved. English is the default when no preference is saved; an explicit Chinese preference is preserved. Portable packages also start in English. Raw backend logs and system file-dialog buttons retain their original language. Build with the PowerShell command above; the hidden self-test also checks catalog round trips and language preference preservation.
