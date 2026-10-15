# 中文实验启动器

在仓库内用 Windows PowerShell 执行：

```powershell
powershell -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1
Start-Process '.\GearsVR Launcher.exe'
```

使用系统已有 .NET Framework 编译器，不安装依赖。构建会执行 `--self-test`，只检查仓库路径、命令参数、状态解析、配置字段保留、owner 身份握手及 Python 可用性，不启动游戏。输出保存在忽略目录 `analysis/vr-launcher-ui/`，并复制到仓库根目录的 `GearsVR Launcher.exe`，可直接双击。

选择游戏 EXE、模拟器或系统 OpenXR Runtime，然后启动。游戏路径写入现有 `tools/vr-launcher/config.json`，保留其他字段。模拟器默认使用 `analysis/runtime/simulator-focus1/openxr_simulator.json`；系统选项让子进程使用系统 Runtime，清除继承的 `XR_RUNTIME_JSON`，不修改注册表。

启动调用 `launcher.py start --manifest tools/vr-launcher/manifest.json`，不会传入自动导航或结束计时参数。当前清单为GameRecovery3 / SpaceRecovery3实验版，仍依赖本仓库工具和Python，不是便携发布包。跟随游戏当前分辨率；DLSS / DLAA由启动、运行时及菜单锁禁用。没有Low/High选项。1920×1080输出对应每眼960×1080；3840×2160对应每眼1920×2160。此版加入缓冲重建恢复、VISIBLE状态渲染及最多2秒有效旧图像重投影。1080p检查点恢复与4K重建已有模拟器证据；4K仍出现长时间无新眼图、非标准窗口尺寸的最终缩放路径仍未覆盖，尚未完成稳定性验收。范围见[修正记录](../../../docs/63-checkpoint-recovery.md)。

Python 按 `GEARSVR_PYTHON`、当前用户 `.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`、PATH 中的 `py.exe` / `python.exe` / `python3.exe` 顺序探测。可通过 `GEARSVR_PYTHON` 指定完整路径。探测取得 `sys.executable` 后直接启动该 Python，确保跟踪的进程就是实际 owner。后台命令隐藏窗口，异步读取标准输出和错误。

“停止 VR”只运行 `launcher.py stop`；不直接终止游戏或 owner。很早收到停止请求时，若 Python 尚未启动则取消本次启动；否则保留请求，等待索引中的 owner PID/创建时间与本窗口启动的 Python 匹配。launcher 写入该索引前已建立停止 event，身份确认后才调用 stop；等待期间不会假报停止成功。关闭仍在运行的本窗口会询问是否请求停止，并保留窗口等待 owner 退出；选择“否”继续运行。清理未完成时保留窗口与日志，可继续查看状态或再次停止。不要用任务管理器强制终止 GUI；异常关机或外部强制结束不在此关闭策略覆盖范围内。

“查看状态”调用现有身份检查状态接口，完整输出显示在窗口。日志保存在 `analysis/runtime/vr-launcher-ui.log`，可用“打开日志”定位目录。状态不自动刷新，需点击获取。启动按钮等待其 owner 完成后才能再次使用；原生资源退出限制与现有 launcher 相同。
