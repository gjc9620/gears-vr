# 缩短调试启动流程

2026-10-11：已修改配置，用户已确认启动画面成功跳过。联网等待和直接地图入口仍待验证。

游戏的 `WarGame/Config/Steam/Cooked/Coalesced.ini` 是序列化字符串数组，
头部计数为 111，后面是 37 组文件名、内容、时间戳。每个字符串以有符号
32 位长度开头，负数表示 UTF-16 字符数（包含末尾 NUL）。因此不能直接
用文本编辑器保存。我们的工具严格检查整个结构、原始版本哈希并保存原始备份。

本次只修改两个配置内容字符串，其余字符串的原始字节保留：

- SteamWarEngine.ini 的 `[FullScreenMovie]` 中删除 SplashScreen、MGSLogo、
  TheCoalitionLogo、UE3AnimatedLogo 四项 StartupMovies。保留 Startup 加载影片、
  吸引模式、菜单背景和剧情影片。
- SteamWarGame.ini 的 `[WarfareGame.WarLoginManager]` 中设置
  `bSkipDownloadingTuneables=true`。这是游戏自带的跳过在线调参下载选项。
  尚不能确定用户看到的所有联网等待都由此步骤产生，需实际验证。

没有调整账号登录、跨平台绑定、网络驱动、多人超时或许可证设置。

## 使用

退出当前游戏后，正常快速启动（保留主菜单和继续游戏流程）：

```powershell
& J:\gearsVr\tools\debug-launch\Start-DebugGame.ps1
```

实验性的直接进入监狱地图入口：

```powershell
& J:\gearsVr\tools\debug-launch\Start-DebugGame.ps1 -DirectLevel
```

该入口向游戏传入 `SP_Prison_P` 地图 URL，实际地图文件存在。
参数是否被此发行版本接受、是否能完成玩家初始化仍需重启测试。
它不是恢复存档命令，可能从关卡默认起点开始；重要进度应先另行备份。
脚本拒绝在已有游戏进程时启动第二个实例，不会强制结束当前游戏。

恢复原始配置：

```powershell
& J:\gearsVr\tools\debug-launch\Restore-Startup.ps1
```

原始配置和哈希记录保存在 Git 忽略的 `analysis/startup-config/`。
恢复时拒绝覆盖不属于原始/已修改哈希的文件，避免抹掉之后的外部修改。
所有修改在下一次启动生效。

## 验证记录

解析验证：完整读取 111 个字符串，恰好到达 EOF；重序列化后重新解析，
确认两项内容修改正确，另外 109 个字符串字节不变。
实际执行 apply → restore → apply，并检查恢复后的 SHA256 与原始一致。

原始 SHA256：`6cf7a0c3a59d85ae6fedf5fc49d8bb406edda9adda7429448d27510e729a8f16`

修改 SHA256：`799723628b465552bc2f1847f91370c10c35e98dbd9460695e5f369956da70de`

后续：用户确认启动画面已跳过；新游戏进程已进监狱关卡并正常操作。
在线调参等待是否消失、地图 URL 是否直接进入关卡尚未验证。
