# First person called Mod Local Installation and Restoration

2026-10-12, user requires to experience downloaded Firsterson 1.0. VR development targets continue to be suspended. This round is installed with the first person-pack of third party plane, without opening a header Hook or VR display program.

Source directory:`J:/gearsVr/First Person-21-1-0-1764109229`Two. `.xxx` Unreal package by `Engine.u`、`WarfareGame.u` Name replacement game `WarGame/CookedPC` . The original download is the same.

Install the first two original game files to be fully backed up:

`J:/gearsVr/analysis/backups/first-person-1.0`

of which `manifest.json` Recording objectives and SHA 256. Original file SHA 256：

- Engine.u：`8d6ad508162609482656fd892cdadc30e0438c8ce2c44fc87163a99c0601666a`
- WarfareGame.u：`cd6d9a98c809d5520783a732f955916111e20bb6bf3155fb8b15dea86d7d0538`

The installation and restoration require the game to exit. Scripts verify both targets and backups, then prepare a temporary copy of the directory, check, replace atoms; refuse to overwrite unknown Hashi. Backup does not cover, delete, or enter Git.

This round has actually implemented the installation, restoration and reconciliation of two original Hashes, re-installed and verified two Mod Hashs. The installation experience is installed in Mods. Starting the game still uses the already skipped header and `-NoPauseUnfocus` Debug starter.

2026-10-12 Follow-up main line survey: User requests to reset the original version and has implemented recovery in game exit status, Engineering.u and WarfareGame.u 256 Checks are checked. The current two packages are original, the Mod download and the original file backup. New game PID 46420 For the original comparison; this recovery only concerns these two packages and the diagnostic startup configuration is still in place.`analysis/runtime/original-packages-46420.json`。

Next game PID 44992 Normal access to the level.00:43:49 It's... 640 × 360 The game window is taken at the point of view of one person, without the original shoulder-backed character. Only the loading and current viewing points are confirmed at this time, without testing bunkers, sightings and special animation; the transect is kept on this machine `analysis/game-io/frames/20261012-004348-8bd50c11.jpg`。

## Restore original

Exit the game first, execute in PowerShell:

```powershell
& 'J:\gearsVr\tools\debug-launch\Restore-FirstPerson.ps1'
```

Or let the assistant perform recovery. It only restores these two packages, without restoring or changing the fast start configuration, archive, image setting.

Install again:

```powershell
& 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' 'J:\gearsVr\tools\debug-launch\first_person_mod.py' install
```

The last one. `install` For `status` Can not open message

The author labels this version Beta, which may be the result of lens misplaced and short internal model displays when animated or bunkered.[Author Page](https://www.nexusmods.com/gearsofwarreloaded/mods/21?tab=description). The effects of this machine are recorded separately and the file Hashi is validated by not equal to all the games.

---

## 中文

# 第一人称 Mod 本地安装与恢复

2026-10-12，用户要求体验已下载的 First Person 1.0。VR 开发目标继续暂停。本轮安装的是第三方平面第一人称包，没有开启头动 Hook 或 VR 显示程序。

源目录：`J:/gearsVr/First Person-21-1-0-1764109229`。两个 `.xxx` Unreal 包分别以 `Engine.u`、`WarfareGame.u` 名称替换游戏 `WarGame/CookedPC` 中的同名包。下载原件保持原样。

安装前两个游戏原文件已经完整备份到：

`J:/gearsVr/analysis/backups/first-person-1.0`

其中 `manifest.json` 记录目标和 SHA256。原文件 SHA256：

- Engine.u：`8d6ad508162609482656fd892cdadc30e0438c8ce2c44fc87163a99c0601666a`
- WarfareGame.u：`cd6d9a98c809d5520783a732f955916111e20bb6bf3155fb8b15dea86d7d0538`

安装与恢复都要求游戏退出。脚本先核验两个目标及备份，再准备同目录临时副本、核验、原子替换；拒绝覆盖未知哈希。备份不覆盖、不删除，不进入 Git。

本轮已实际执行安装、恢复并核对两个原始哈希，再次安装并核对两个 Mod 哈希。安装体验时状态为 Mod 已安装。启动游戏仍使用已有跳过片头与 `-NoPauseUnfocus` 调试启动器。

2026-10-12 后续主线调查：用户要求恢复原版后重测，已在游戏退出状态下执行恢复，Engine.u 和 WarfareGame.u 两个原始 SHA256 均核对通过。当前两个包为原版，Mod 下载件及原文件备份保留。新游戏 PID 46420 用于原版对照；此次恢复只涉及这两个包，诊断启动配置仍保留。恢复证据：`analysis/runtime/original-packages-46420.json`。

随后游戏 PID 44992 正常进入关卡。00:43:49 的 640 × 360 游戏窗口采图已见第一人称视点，没有原先的肩后主角身体。此时仅确认关卡加载及当前视点，未测试掩体、瞄准和特殊动画；截图保存在本机 `analysis/game-io/frames/20261012-004348-8bd50c11.jpg`。

## 恢复原版

先退出游戏，在 PowerShell 执行：

```powershell
& 'J:\gearsVr\tools\debug-launch\Restore-FirstPerson.ps1'
```

或让助手执行恢复。它只恢复这两个包，不恢复或改变快捷启动配置、存档、画面设置。

再次安装：

```powershell
& 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' 'J:\gearsVr\tools\debug-launch\first_person_mod.py' install
```

将最后的 `install` 改为 `status` 可查看当前文件状态。

作者将此版本标为 Beta，动画或掩体时可能出现镜头错位和短暂模型内部显示，见[作者页面](https://www.nexusmods.com/gearsofwarreloaded/mods/21?tab=description)。本机运行效果另行记录，文件哈希通过不等于全部游戏流程已验证。
