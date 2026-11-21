# Independent game instances

The isolated simulator entry `isolated_sim.py` has no fixed instance-count
limit (owner override, 2026-11-15). The owner also removed the 1 GiB free-RAM /
6 GiB free-commit admission threshold: memory is recorded but does not block a
launch. Disk availability and process identity checks remain. Existing sessions
keep running if a new launch fails.
Its background mode hides the owned game and simulator windows with DWM cloak;
the processes continue rendering, but no preview appears on the desktop or
taskbar. Instance count and window visibility are separate settings.

For owner-requested desktop-visible instances, use a fresh name:

```powershell
$instanceName = "visible-$([guid]::NewGuid().ToString('N'))"
python tools/multi-instance/isolated_sim.py prepare --name $instanceName --visible --lowest --render-scale 0.5
python tools/multi-instance/isolated_sim.py start --name $instanceName --wait-resources-seconds 0
```

Visible mode selects the verified non-cloaking startup/runtime, shows the game
and simulator as normal taskbar windows, and retains private profiles and normal
stop/cleanup. An already-running hidden instance must be stopped normally and
replaced with a fresh visible instance; it is not hot-switched.

Add `--lowest` to `prepare` for the owner's low-load multi-instance comparison.
It backs up each private EngineUserSettings INI, lowers existing graphics fields
(shadows/AO/reflections/post effects off, low detail, texture groups capped at
256), sets the desktop startup dimensions to 800x600 and VR scale to 0.25.
The per-instance `lowest-graphics.json` lists every changed field and its hash;
`lowest-graphics-originals/` retains the copied original settings. This does not
overwrite shared settings or represent a verified in-game named preset.

The current owner-requested comparison overrides the low-load preset's VR
scale with `--render-scale 0.5`; there is no fixed 1080p requirement. Use a new
name for every restart, including after a failed start. Do not adopt or stop
another chat's instance. No recording is enabled by these commands.

The optional older `--full-hd` mode requests a 1920x1080 desktop and total
side-by-side stereo image, with 960x1080 per eye at scale 1.0. Runtime render
receipts remain separate from the requested size plan.

Visible load-test instances skip startup movies via the private child-only
`-nomoviestartup` switch. This avoids the observed intro-movie startup stalls;
normal shared/manual launch behavior is unchanged. Private player discovery is
bounded to 180 seconds and uses the existing normal cancellation/cleanup path.
`status --name ...` now includes `startup_status.startup_confirmed`: it requires
an actual first-stereo receipt, a running native session, and matching live
launcher/game/XR identities. A process or queued map request alone is not ready.

`start.py` provides an explicit multi-instance entry point. Each new name gets
private game settings, profiles, game logs, shader-cache copies, native DLL/status
files and a launcher index under `analysis/instances/<name>/`. The game EXE and
shared game assets are unchanged. The process-local early startup policy maps
only `Guardian_UnrealEngine3_1` to the chosen instance namespace. Unrelated OS,
input and driver mutexes retain their original names.

Start another ordinary player instance with the configured OpenXR runtime:

```powershell
python tools/multi-instance/start.py --name player2
```

Start an autonomous Mod regression beside an existing player instance:

```powershell
python tools/multi-instance/start.py --name autoplay --playtest --seconds 21600 --fixture analysis/playtest-fixtures/20261020-cole-window47-history.json
```

Use a fresh name for each launch. The command prints the worker PID, private
launcher index and test report directory. A player launch retains the normal
configured runtime; an autonomous run uses the verified behind-windows test
simulator, muted audio, observe-only tiny-output monitoring, disabled physical
background-controller input and no saved images/video. It never changes the
system OpenXR runtime.

After explicit user authorization, set
`GEARSVR_TEST_ALLOW_MANUAL_PREVIEW_ACTIVATION=1` in the test child environment
to keep playing when the user brings that run's simulator preview forward.
The observer verifies the XR process identity and exact preview window class,
records the allowed overlap, and retains the exception while that same preview
remains present. Startup still uses the behind-windows policy; the game window,
other runs and unrelated dialogs do not receive this exception.

To stop an autonomous run, create `stop` inside its printed playtest directory.
The runner releases its inputs, cleans up its own capture/XR session, closes
its own game normally and restores its private settings/profiles. It does not
restore old backups over a user's shared profile or terminate another game.

Normal repository launches ignore only registered private processes whose PID,
creation time and EXE path still match their private index. Unknown games retain
the shared-profile protection; use this entry point to give another player a
separate profile. The ordinary player launcher and the test launcher hold
different named owner locks.

The early folder override is restricted to LocalAppData queries originating in
the main game image. RoamingAppData and driver/SDK folder queries retain their
normal locations. Game-data/log path operations are redirected consistently,
including ordinary reads/writes, attributes, enumeration, atomic move and delete.
This is targeted game-instance isolation, not a general Windows sandbox.

Initial shader-cache copies are bounded to 512 files / 2 GiB. Copies consume
private disk space but avoid the large cold-cache rebuild seen with empty
directories. Each source is read twice and each destination is checked; a changing
source stops preparation. Native artifacts and copied files are hashed in
`instance.json`. Private game derivatives remain ignored under `analysis/`.

The new startup policy passes its exact-mutex, unrelated-mutex, folder and
physical-original-file preservation tests. Background placement is also checked
separately. A legacy fixture's restore/minimize expectations do not apply to the
test-only behind-windows show policy; its whole-suite result is retained without
being promoted to a pass. Actual game occlusion and foreground conditions remain
input guards: a failure is recorded and never repaired by moving user windows.

---

## 中文

# 独立游戏实例

孤立的模拟器条目 `isolated_sim.py` 没有固定的例数
限制( 所有人覆盖, ) 2026-11-15。主人还移除了该设备。 1 GiB 免费内存 /
6 GiB 自由承诺录取门槛:内存被记录,但不屏蔽 a
磁盘的可用性和处理身份检查。
如果新发射失败, 继续运行 。
它的背景模式用 DWM 外衣隐藏了自有游戏和模拟窗口;
进程继续渲染,但桌面上没有预览或
任务栏。实例计数和窗口可见度是独立的设置。

对于所有者要求的桌面可见实例,请使用新名称 :

```powershell
$instanceName = "visible-$([guid]::NewGuid().ToString('N'))"
python tools/multi-instance/isolated_sim.py prepare --name $instanceName --visible --lowest --render-scale 0.5
python tools/multi-instance/isolated_sim.py start --name $instanceName --wait-resources-seconds 0
```

可见模式选择已核实的非 cloaking 启动/ 运行时间, 显示游戏
并保留私有配置和正常配置
停止/清理。一个已运行的隐藏实例必须正常停止,
替换为新可见实例; 它不是热切换。

Add `--lowest` to `prepare` 用于所有者低载多进制比较.
它支持每个私有的引擎用户设置 INI, 降低已有图形字段
(阴影/AO/反射/后效应,低细节,纹理组封顶于
256),设置桌面启动维度为 800x600 和 VR 比例表为 0.25.
平时的 `lowest-graphics.json` 列出每个已更改的字段及其散列;
`lowest-graphics-originals/` 保留复制的原始设置。此选项不包含
覆盖共享设置或代表一个已验证的游戏中命名的预设 。

当前所有者要求的比较, 覆盖了低载预设的 VR
缩放 `--render-scale 0.5`; 没有固定 1080 p 要求。 使用新程序
每次重新启动的名称,包括失败的启动后。不采纳或停止
其它聊天实例。 这些命令不允许录制 。

年长者可选 `--full-hd` 模式请求 a 1920x1080 桌面和总计
边立体声图像,带有 960x1080 每只眼睛在比例上 1.0 运行时间渲染
收据仍与要求的面积计划分开。

可见的装入测试实例通过只限孩子的自定义跳过启动电影
`-nomoviestartup` 切换。这可以避免所观察到的内映电影启动档;
普通共享/ 手动启动行为不变。 私人玩家发现是
以 180 秒,并使用已有的普通取消/清理路径。
`status --name ...` 现在包括 `startup_status.startup_confirmed`: 需要
实际的第一定型接收、运行中的本地会话和匹配的直播
启动器/游戏/XR身份。 仅一个进程或队列地图请求尚未准备好 。

`start.py` 提供了明确的多事件入口。每个新名称都
私人游戏设置、配置文件、游戏日志、阴影- caches 副本、 本地 DLL/ status
文件和发射器索引 `analysis/instances/<name>/`。游戏 EXE 和
共享游戏资产不变。进程- 本地启动早期政策图
only `Guardian_UnrealEngine3_1` 以选择实例命名空间。无关的 OS,
输入和驱动程序变种保留原名。

以配置的 OpenXR 运行时间启动另一个普通玩家实例 :

```powershell
python tools/multi-instance/start.py --name player2
```

在现有玩家实例旁启动自动 Mod回归 :

```powershell
python tools/multi-instance/start.py --name autoplay --playtest --seconds 21600 --fixture analysis/playtest-fixtures/20261020-cole-window47-history.json
```

每次发射使用新名称。命令打印工人的 PID, 私人
发射机索引和测试报告目录。玩家发射程序保留正常
配置运行时间; 自动运行使用已校验的窗口后端测试
模拟器、变音器、只观测到的微量输出监测器、 已禁用
背景控制器输入和未保存图像/视频。它永远不会改变
系统 OpenXR 运行时间。

用户明确授权后, 设定
`GEARSVR_TEST_ALLOW_MANUAL_PREVIEW_ACTIVATION=1` 试验儿童环境中的
当用户将运行的模拟预览带往前时, 继续播放。
观察员验证 XR 进程身份和准确的预览窗口类,
记录允许的重叠,并保留例外,同时保留同样的预览
仍然在存在。启动仍然使用后窗政策; 游戏窗口,
其他运行和无关对话框不接受此例外。

要停止自动运行, 请创建 `stop` 在其打印的播放测试目录中。
跑者释放其输入,清理自己的捕获/XR会话,关闭
普通游戏并恢复其私人设置/配置。它不
恢复用户共享配置文件上的旧备份,或终止另一游戏.

正常寄存器的发射只忽略了已注册的私人进程,其PID,
创建时间和 EXE 路径仍然符合其私有索引。未知的游戏保留
; 使用此切入点给另一个玩家一个
单独的配置。普通玩家发射器和测试发射器将
不同的名牌主锁。

早期文件夹的覆盖仅限于本地AppData查询, 来源于
。 RoamingAppData 和驱动程序/ SDK 文件夹查询保留其
常规位置。游戏数据/日志路径操作被统一重定向,
包括普通读/写、属性、点数、原子移动和删除。
这是目标游戏-智能隔离,而不是一般的Windows沙盒.

初始阴影刻录副本的边界为 512 文件 / 编号 2 复制件消耗
私人磁盘空间, 但避免空眼看到的大冷笼重建
目录。每个源被读取两次,每个目的地被检查;一个变化
源代码停止准备。原始的文物和复制的文件被散列在
`instance.json`。私有游戏衍生工具在 `analysis/`.

新的启动政策通过精确的mutex、不相关的mutex、文件夹和
实物文件保存测试。背景位置也检查
遗产的恢复/最小化预期不适用于
测试仅显示后窗;其全全结果保留而不包含
实际的游戏隔离和前缘条件
输入守护符:记录失败,永远不通过移动用户窗口修复.
