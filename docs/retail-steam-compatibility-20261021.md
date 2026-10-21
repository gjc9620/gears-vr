# Steam 正版基线与存档迁移

2026-10-21。用户购入正版后要求比较文件、处理当前 Mod 启动兼容；随后确认将现有存档迁移至正版，今后只用正版开发／测试，并明确停止追加兼容性游戏运行测试。

## 文件身份

| 安装 | 位置 | 完整盘点 |
| --- | --- | --- |
| 项目内正版工作副本 | `J:\gearsVr\Gears of War Reloaded` | 3,609文件，76,345,902,085字节 |
| Steam登记的原始安装 | `J:\SteamLibrary\steamapps\common\Gears of War Reloaded` | 3,609文件，全部与工作副本SHA256一致 |
| 历史旧安装 | `J:\gearsVr\Gears.of.War.Reloaded.v1.0.1.3816947-P2P\Gears of War Reloaded` | 4,601文件；与正版共有3,609文件 |

三份盘点均对每个常规文件完整读取计算 SHA256；不是只比较文件名、长度、时间戳或抽样。项目内正版和旧目录在测试前完成盘点；Steam库原始安装在确认重启交接后独立完成。两份正版的3,609文件全部一致。旧安装共有文件中3,607完全相同，仅有：

- `Binaries_x64/steam_api64.dll`：旧版980,576字节、SHA256 `7149e2681604f00b310c6749512c8c0c2b83d86a3052596de5f566823776b00b`；正版298,856字节、`a44e5537939ae4eebc69000589aa9b2437a667813a1657cc779198bae9b815a9`。正版DLL与旧目录的 `.rne` 原始备份相同；旧目录另外存在 `steam_emu.ini` 和分发标记，足以解释旧安装的不同启动环境。
- `WarGame/Config/Steam/Cooked/Coalesced.ini`：正版原始哈希 `6cf7a0c3a59d85ae6fedf5fc49d8bb406edda9adda7429448d27510e729a8f16`；旧目录 `799723628b465552bc2f1847f91370c10c35e98dbd9460695e5f369956da70de`。离线重算确认差异精确等于项目已有的可恢复启动补丁：删四项开场Logo、启用内建跳过在线tuneables等待。

旧目录独有992文件，其中973个是已有测试／游戏生成的Stats，其余为分发附带文件和历史分析工程。它们不是正版缺失的游戏资源。2,518个CookedPC文件与正版全部一致。

三份核心 `Binaries_x64/GOWDE-Steam.exe` 都是版本 `1.0.381.6947`、26,278,928字节，完整SHA256为：

```text
bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4
```

因此现有基于该磁盘EXE和同版本资源的工作继续有效。历史运行时快照仍保留其独立哈希和module base，不能把磁盘相同推为整进程内存、Steam服务、GPU或头显行为相同。REA只读PE资源身份记录为 `ev_bc9cd5e918caa519f8fd7e24584cfb19e665f2b5c7856c58df2f49527e431ed0`。

## 实际发现与修复

初次按原启动方式运行项目内正版时，22项预启动原生自测通过，游戏PID46548创建成功，但预备阶段返回 `Target priming failed (1)`。游戏退出且没有产生有效的本轮prime状态文件；此数字是远程线程退出码，不能单独解释为Win32错误或特定Hook故障。

Steam日志在同一时间明确记载原版API请求 `steam://run/2523720//-NoPauseUnfocus`，随后Steam创建库内 `Launch_GOWDE.exe` PID48564。与Valve所述“直接运行时重启到Steam库内登记版本”的机制相符。[Steamworks API文档](https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary)说明了这种重启及原进程退出行为。

在测试子进程中传入正确 `SteamAppId=2523720`、`SteamGameId=2523720` 后，项目内正版PID25228预留成功，13处游戏入口字节核对通过，不再交接到库内包装程序。该修复已写入 `game_install.launch_environment()`，供安静启动、普通前台启动和显式RenderDoc启动使用；环境变量只作用于子进程，处理Windows大小写不敏感的变量名，不改父进程／全局环境。Steam DLL和账号授权流程保留原样。

正版首次运行还为活动Steam账号创建了自己的设置文件，使旧测试账号和正版账号两套 `*EngineUserSettings.ini` 共存。原 `graphics_policy.locate()`要求恰好一份，后续会拒绝。现在多文件时仅从当前用户的Steam `ActiveProcess/ActiveUser`读取账号数字，精确匹配该账号的设置文件；不读取登录凭据，不以最新时间或目录顺序猜选。账号不匹配仍拒绝，单设置文件的既有行为保留。

85项debug-launch与76项launcher离线检查通过，涵盖子进程环境、父环境保留、后台窗口策略和多账号精确选择。当前正常native仍为 `GearsCapturePlayground1.dll`（`bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`），XR与playground构建也保留原冻结身份，无需因本次路径／Steam环境调整重编译原生模块。

## 运行证据边界和收尾

第二轮自动继续因首次开场／标题流程超时，没有进关卡。该轮独立owned-prime取消／入口恢复和菜单桥接恢复均通过，正常关闭游戏后恢复五个受保护文件；失败报告保留。

第三轮使用代码内已集成的Steam环境修复，测试驱动没有额外供应Steam App ID，正版预留成功并到达正常 `DeltaMotD` 欢迎新闻页。用户此时明确停止后续测试，故没有继续确认菜单、启动关卡或启用双目。owned-prime入口、菜单组件和测试音频恢复通过。关闭次要窗口时发生WinError1400，保留原始失败；单独读取清理回执后确认游戏正常退出并恢复10个受保护文件。没有强杀游戏或重试注入。

本次确认的是完整磁盘身份、正版选定进程启动／预留兼容和欢迎页可达。未新增正版关卡GPU出图、FPS、继续章节、物理头显或完整玩法验收，也未因用户停止而再启动游戏。

## 存档和默认路径

两个正版安装在同一Windows用户下共用 `%LOCALAPPDATA%\Coalition\WarGame\SaveData`。迁移按游戏账号文件名进行，不把存档分别复制进两份安装目录。

旧账号当前的 `.ue3profile`、`.ue3profile.local`、`.ue3profile.Narration`、`.ue3profile.WarfarePLS` 四文件已复制至当前Steam活动账号的对应文件名。源旧档保留；迁移前完整保存了源四文件和目标四文件。主档／local档的zlib数据校验和声明长度有效，没有改动压缩载荷或序列化内容，目标四份SHA256逐一与源相同。没有替换 `steam_autocloud.vdf`、修改Steam云开关或编辑云缓存元数据。按用户要求没有再开游戏验证存档读取。

备份目录为本机忽略的 `analysis/retail-compatibility-20261021/save-migration-20261021-222503/`；完整迁移映射、SHA256、恢复前副本和完成回执保存在该目录及 `save-migration.json`。公开文档不记录个人Steam账号标识。

正常清单、普通用户配置、DLSS清单、基础启动脚本、后台采集／GPU工具、资源导出工具的当前默认路径已切换为 `J:\gearsVr\Gears of War Reloaded`；以后新测试不再使用旧P2P目录。历史实验清单和记录保持原路径以保留来源。Steam库内原始安装仍可从GUI手选，不改其游戏文件。当前用户的第一人称、死区、渲染倍率等其他配置保留；`v0.0.1`标签和已经发布的包不变。

本地原始证据位于 `analysis/retail-compatibility-20261021/`：三份完整盘点、两份比较结果、REA资源记录、Steam启动失败／成功预留回执、清理与音频恢复、存档格式检查、存档迁移和默认路径迁移回执。所有专有衍生数据和个人存档都不进入源码提交或发布包。
