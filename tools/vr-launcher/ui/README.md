# 中文实验启动器

在仓库内用 Windows PowerShell 执行：

```powershell
powershell -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1
Start-Process '.\GearsVR Launcher.exe'
```

使用系统已有 .NET Framework 编译器，不安装依赖。构建会隐藏运行 `--self-test`，检查仓库路径、命令参数、状态解析、配置字段保留、HUD 能力门禁与数值范围、保存读回、owner 身份握手及 Python 可用性，不显示 GUI 或启动游戏。输出保存在忽略目录 `analysis/vr-launcher-ui/`，并复制到仓库根目录的 `GearsVR Launcher.exe`，可直接双击。

选择游戏 EXE、模拟器或系统 OpenXR Runtime，然后启动。游戏路径写入现有 `tools/vr-launcher/config.json`，保留其他字段。模拟器默认使用 `analysis/runtime/simulator-quiet2/openxr_simulator.json`；系统选项让子进程使用系统 Runtime，清除继承的 `XR_RUNTIME_JSON`，不修改注册表。 quiet2 修复菜单交换链重建后编号复用、导致双眼拿到旧菜单纹理的问题；同一重建序列已在实际游戏验证左右眼使用独立新编号并持续投影超过90秒。这项修复不代表所有闪烁或黑屏均已解决。

模拟器预览保持可见，自动启动不获取桌面焦点；用户可以点击它或移到第二屏。正常连续 VR 会话没有自动结束计时，不主动最小化或重新定位预览。共用游戏启动入口复用后台手柄方案；静默方式不等于将游戏最小化。独立模拟器启动观察通过，游戏自行激活的行为仍需实际观察，见文档68。

启动调用 `launcher.py start --manifest tools/vr-launcher/manifest.json`，不会传入自动导航或结束计时参数。当前清单为GameRecovery8 / SpaceRecovery3实验版，仍依赖本仓库工具和Python，不是便携发布包。跟随游戏当前分辨率；DLSS / DLAA由启动、运行时及菜单锁禁用。没有Low/High选项。1920×1080输出对应每眼960×1080；3840×2160对应每眼1920×2160。此版加入缓冲重建恢复、VISIBLE状态渲染、最多2秒有效旧图像保留、认证Material双眼绘制和重载后UI角色绑定恢复。连续两轮模拟器实测覆盖4K及两次同进程检查点重载，HUD调整可继续生效；仍有约0.87秒最长提交间隔，Recovery8另外修复批次表满后的准星/HUD永久失效，已完成实际手枪开镜复测；有限距离瞄准、换枪/字幕完整验收和CG仍待完善，非标准窗口尺寸最终缩放仍未覆盖。尚未完成稳定性或真头显验收。范围见[修正记录](../../../docs/63-checkpoint-recovery.md)。

Python 按 `GEARSVR_PYTHON`、当前用户 `.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`、PATH 中的 `py.exe` / `python.exe` / `python3.exe` 顺序探测。可通过 `GEARSVR_PYTHON` 指定完整路径。探测取得 `sys.executable` 后直接启动该 Python，确保跟踪的进程就是实际 owner。后台命令隐藏窗口，异步读取标准输出和错误。

默认 `tools/vr-launcher/manifest.json` 的布尔能力 `hud_size_control` 为 `true` 时，窗口显示“HUD 大小”角宽控件：45–100°，未配置时为 65°，已有 `hud_horizontal_fov_degrees` 会读回。调大更易读，调小更集中；点击“启动 VR”时连同游戏路径保存，下次启动生效。保存保留其余配置键。旧默认清单没有该能力或值为 `false` 时，不显示控件，也不新增角宽字段；当前旧 DLL 不会因此获得调整能力。清单能力在窗口打开后变化时，需要重新打开启动器。

“停止 VR”只运行 `launcher.py stop`；不直接终止游戏或 owner。很早收到停止请求时，若 Python 尚未启动则取消本次启动；否则保留请求，等待索引中的 owner PID/创建时间与本窗口启动的 Python 匹配。launcher 写入该索引前已建立停止 event，身份确认后才调用 stop；等待期间不会假报停止成功。关闭仍在运行的本窗口会询问是否请求停止，并保留窗口等待 owner 退出；选择“否”继续运行。清理未完成时保留窗口与日志，可继续查看状态或再次停止。不要用任务管理器强制终止 GUI；异常关机或外部强制结束不在此关闭策略覆盖范围内。

“查看状态”调用现有身份检查状态接口，完整输出显示在窗口。日志保存在 `analysis/runtime/vr-launcher-ui.log`，可用“打开日志”定位目录。状态不自动刷新，需点击获取。启动按钮等待其 owner 完成后才能再次使用；原生资源退出限制与现有 launcher 相同。
