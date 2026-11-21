# Steam, positive baseline and archive migration

2026-10-21. The user requires a comparison of the file, processing current Mod start-up compatibility; then confirms that the existing archive will be moved to the original version and will be developed/tested only in the original version, and explicitly discontinues additional compatible game running tests.

## Document identity

| Install | Location | Full inventory |
| --- | --- | --- |
| Copy of the working part of the project | `J:\gearsVr\Gears of War Reloaded` | 3,609 Documentation,76,345,902,085 Bytes |
| Original installation registered by Steam | `J:\SteamLibrary\steamapps\common\Gears of War Reloaded` | 3,609 File, all with working copy SHA 256 Unanimously. |
| Old historical installation | `J:\gearsVr\Gears.of.War.Reloaded.v1.0.1.3816947-P2P\Gears of War Reloaded` | 4,601 files;shared with original 3,609 Documentation |

Full calculation of SHA for each regular file on all three counts 256; not only compare file names, lengths, time stampes or samples. The positive and old directories of the project complete their inventory before testing; the original Steam Library installation is independently completed after confirming that the switch has been restarted.3,609 Files are all identical. Old common files installed 3,607 Exactly the same, except for:

- `Binaries_x64/steam_api64.dll`: Old version 980,576 Bytes, SHA 256 `7149e2681604f00b310c6749512c8c0c2b83d86a3052596de5f566823776b00b`;real 298,856 bytes,`a44e5537939ae4eebc69000589aa9b2437a667813a1657cc779198bae9b815a9`. The original DLL with the old directory `.rne` original backups are the same; old directories exist separately `steam_emu.ini` The distribution of tags is sufficient to explain the different start-up environments for old installations.
- `WarGame/Config/Steam/Cooked/Coalesced.ini`: original Hash `6cf7a0c3a59d85ae6fedf5fc49d8bb406edda9adda7429448d27510e729a8f16`;old directories `799723628b465552bc2f1847f91370c10c35e98dbd9460695e5f369956da70de`. Offline recalculation confirms that the difference is precisely equal to the restartable patches already available for the project: delete four opening Logos, and enable in-house construction to skip online tuneables waiting.

The old directory is unique.992 documents, of which 973 The other are the distribution of accompanying documents and historical analysis projects. They are not the missing game resources in the original version.2,518 The CookedPC documents are all the same as the original version.

Three cores. `Binaries_x64/GOWDE-Steam.exe` It's all versions. `1.0.381.6947`、26,278,928 Bytes, full SHA 256 Is:

```text
bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4
```

The current work based on this disk EXE and the same version of the resource is therefore still valid. History is running with a snapshot that retains its independent Hashi and Module base, so it cannot be compared to the memory of the whole process, Steam service, GPU or headline behaviour. `ev_bc9cd5e918caa519f8fd7e24584cfb19e665f2b5c7856c58df2f49527e431ed0`。

## Actual detection and rehabilitation

The first time you run the original project, you can see it in the original version.22 Prestart raw self-measurement, game PID 46548 Created successful, but returns in the preparatory phase `Target priming failed (1)`. The game exits without producing a valid current cycle prime status file; this number is the remote line exit code and cannot be interpreted as Win alone 32 Error or particular Hook malfunction.

Steam Log clearly records the original API request at the same time `steam://run/2523720//-NoPauseUnfocus`, and then Steam creates the library `Launch_GOWDE.exe` PID 48564. This corresponds to the mechanism described by Valve for “restarting registration in Steam Library when running directly”.[Steamworks API Document](https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary)This resumption and the withdrawal of the original process are described.

Getting right in the test subprocess `SteamAppId=2523720`、`SteamGameId=2523720` , and then the project's original PID 25228 The government has been able to protect the people.13 The byte check at the entrance to the game is passed and no more folders are handed over to the library. This fixation has been written `game_install.launch_environment()`, for silent start-up, common front-stage start and visible RenderDoc start; environmental variables only function in subprocesses, handle Windows case sensitive variable names, and do not change the parent process/global environment. Steam DLL and account authorization processes remain as they are.

The original version was first run and created its own settings for active Steam account, making the old test account and the original one two sets `*EngineUserSettings.ini` Co-exist. `graphics_policy.locate()`The request is exactly one and will be rejected later. The multiple files are now only from the current user's Steam `ActiveProcess/ActiveUser`Reads the account number, matching exactly the settings file for that account; does not read login documents and is not guessed in the order of the latest time or directory. The account number mismatch is still rejected, and sets the existing behavioral reservation for a single document.

85 Debug-launch and 76 The line launcher offline check is passed, covering the subprocess environment, parent environment preservation, back-office window policy and multiple account selection. Current normal native is still `GearsCapturePlayground1.dll`（`bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`XR and Playground also maintain the original frozen identity without having to re-compile the original module for this path/Steam environment.

## Run evidence boundaries and close

The second round automatically continues without entering the level because of the first opening/title process. The round of independence is cancelled/entry and the menu bridge is restored, with five protected files restored after normal closing; failure reports are retained.

3rd round of integrated Steam environment rehabilitation, test-driven without additional supply of Steam App ID, original set-up and normal `DeltaMotD` Welcome news pages. Users are clearly stopping subsequent testing at this time, so they do not continue to confirm menus, activate level or enable binary access. owned-prime portals, menu components and test audio back through. WinError occurs when closing secondary windows 1400, preserving original failed; confirming normal game exit and resumption after reading cleanup reply alone 10 A protected file. No killing game or retest injection.

This is the full disk identity, original selection process start/prepare compatible and welcome pages that can be reached. No new regular level GPS drawings, FPS, continuing chapters, physical header or complete acceptance are added, nor are games started because users stop.

## Archive and Default Path

Both original versions are installed under the same Windows user `%LOCALAPPDATA%\Coalition\WarGame\SaveData`. The migration is made by the name of the game book file, and the archive is not copied into two installments.

Old account is currently `.ue3profile`、`.ue3profile.local`、`.ue3profile.Narration`、`.ue3profile.WarfarePLS` Four files have been copied to the corresponding file name of the current Steam active account. Source old files are kept; source four and target four files are fully saved before migration. The zlib data verification and declaration length of the main/local file is valid, no compression load or sequenced content changes, target 4 SHAs 256 Same as source by source. No replacement `steam_autocloud.vdf`, modify the Steam cloud switch or edit cloud cache metadata. As requested by the user, no more games are being opened to verify that the archive is read.

Backup directory ignored by this machine `analysis/retail-compatibility-20261021/save-migration-20261021-222503/`;complete migration map, SHA 256, save copies before recovery and return confirmation in the directory and `save-migration.json`. The public document does not record the personal Steam account identification.

The current default path for the normal list, common user configuration, DLSS list, basic startup scripts, backstage collection/GPU tool, resource export tool has been switched to `J:\gearsVr\Gears of War Reloaded`;no longer using old P for new tests 2 The list of historical experiments and records are maintained to maintain the source. The original installation in the Steam library can still be selected by GUI without changing its game file. Current users' first name, dead area, rendering ratio etc. is retained;`v0.0.1`The label and the package that has been released remain unchanged.

Local original evidence is located in `analysis/retail-compatibility-20261021/`Three complete inventories, two comparison results, REA resource records, Steam start failure/success prefixing, cleanup and audio recovery, archive format checks, archive migrations and default path migration returns. All proprietary derivative data and personal files are not submitted or released into source code or release packages.

---

## 中文

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
