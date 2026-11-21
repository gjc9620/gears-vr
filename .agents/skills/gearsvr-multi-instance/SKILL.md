---
name: gearsvr-multi-instance
description: 在 gearsVr 项目中复用现有入口准备、启动、检查和正常停止独立游戏与 OpenXR 模拟器实例。适用于隔离多开及启动排查，不用于 Quest 工作台或普通手动启动。
---

# 独立游戏＋OpenXR 模拟器多开

工作区：`J:/codexWorktree/gearsVr`。复用 `tools/multi-instance/isolated_sim.py`；细节查阅项目内 `tools/multi-instance/README.md`。不要重新实现多开或直接重复启动游戏 EXE。此 skill 只规定操作方式，按当前用户任务执行，不因加载而自动启动。

## 实例与参数

- 每个会话使用自己的唯一实例名；每次重启另取新名，已启动或失败的名字不复用。操作已有实例前先查状态并确认归属，只控制自己拥有的实例。
- 使用 `--visible --lowest --render-scale 0.5`，窗口显示在桌面和任务栏；不要求固定 1080p，不启用录像。`--lowest` 是现有自定义低负载配置，未证明等同游戏菜单全部最低选项。
- 设置、存档副本、模拟器输入、日志、Native 状态与运行身份在 `analysis/instances/<实例名>/`；游戏资源共享，无须复制整份游戏。
- 固定实例数量上限及 1 GiB 可用物理内存／6 GiB 可用提交内存拦截均已取消。内存只记录，实际资源分配失败如实报告；保留磁盘与身份检查。
- 不关闭其他会话的游戏、模拟器或启动器，不按进程名称批量结束进程，也不为新实例关闭旧实例释放资源。

## 操作

在工作区用 PowerShell 执行。首次设定全新 `$instanceName`；后续检查、停止沿用本次名字。重启须更换名字并重新 prepare。

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$instanceName = 'chat-' + [guid]::NewGuid().ToString('N')
& $py tools/multi-instance/isolated_sim.py prepare --name $instanceName --visible --lowest --render-scale 0.5
```

prepare 成功后启动，再检查：

```powershell
& $py tools/multi-instance/isolated_sim.py start --name $instanceName --wait-resources-seconds 0
& $py tools/multi-instance/isolated_sim.py status --name $instanceName
```

需要结束自己的实例时正常停止：

```powershell
& $py tools/multi-instance/isolated_sim.py stop --name $instanceName
```

## 启动判据与故障边界

只有状态输出 `startup_status.startup_confirmed=true` 才可报告启动成功：它要求实际双眼画面提交回执、运行中的 Native 会话及身份匹配且存活的启动器／游戏／XR 进程。`awaiting_level` 表示关卡尚未初始化完成；进程存在、模拟器窗口出现或请求进入关卡均不足以确认。

保留可见入口通过独立子进程参数 `-nomoviestartup` 跳过启动影片的行为，以及私有玩家发现 180 秒等待上限；超时走现有取消和清理路径。失败时保留状态与日志，不改共享游戏文件或普通手动启动的影片设置。

报告实例名、启动判据、实际失败及清理状态，区分启动验证、玩法和实体头显验收。2026-11-15 历史验证仅证明五套在比例 0.5 同时进入关卡并提交双眼画面；八套正常出画面、长期稳定性和全部最低画质尚未验证。历史实例不代表当前存活或本会话拥有。
