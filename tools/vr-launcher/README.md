# 仓库内统一入口

2026-10-14 当前交付范围：支持头部6DoF的第三人称同帧双目首版，继续自动实现并用模拟器诊断；真头显由用户指示开始和结束。Recovery2 已完成一次约14分49秒 VDXR 真头显游玩，用户反馈整体效果不错。ADS准星、换枪UI是位置偏离而非图形变形，字幕尚未确认，三者均为首版必做。位置输入与矩阵应用已有证据，平移方向/比例、近处视差及距离感仍需确认，尚不宣称6DoF体验验证完成。当前发布包尚未完成。

## StartupMenu1：启动菜单到立体场景

实验清单 `manifest-startup-menu1.json` 固定 StartupMenu1 native/XR 构建。启动即创建XR会话，通过游戏窗口捕获显示双眼共用的平面菜单；收到首个成功提交的菜单帧后等待关卡玩家，首张匹配票据的双眼场景成功渲染后单向交接。之后不会把桌面SBS重新捕获到菜单面板。捕获支持窗口尺寸变化，宽度上限1920、约30Hz；当前仍经过CPU拷贝，后续可优化。平面菜单不计成立体场景证据。

目标预留Hook改为持有启动器的进程身份和句柄，菜单等待不再受旧180秒上限限制；取消或启动器退出仍执行恢复。启动器在菜单等待与自动导航期间检查游戏、XR和取消状态，未出现有效菜单首帧时不激活场景Hook。默认清单与旧构建不变。

2026-10-14 PID34900模拟器实际测试：菜单等待超过180秒后仍可见，25865帧菜单后，同一XR进程22604自动交接到关卡双目。已保留菜单、关卡各一张实时MCP双眼截图。立体输出252.746474秒、提交22750帧、复制18008张眼图，最大提交间隔18.5689ms；正常停止、XR退出0、native清理与入口恢复通过，游戏随后正常关闭。原始报告 `analysis/runtime/continuous-stereo-34900-20261014-160759-cee54b3e.json`。这证明该次模拟器菜单→关卡流程，不替代真头显菜单体验或HUD定位修复。测试包括34项启动器/导航、31项持续会话及native owner生命周期和XR CPU检查。

同一PID只读反射确认准星脚本以Canvas中心定位、换枪菜单使用HUD安全区中心；脚本执行入口与Canvas DrawTile/DrawMaterialTile/DrawText入口已定位，但尚未改坐标。字幕运行时enabled位为true、forced-off位为false，仍需实际对白绘制证据。两处DLSS运行时字段均为0；防止运行中重新启用的锁尚未实现。定向证据保留在 `analysis/ui-position-runtime` 和 `analysis/graphics-lock`。

实际启动经过 `Start-DebugGame.ps1` 时会执行 `graphics_policy.py apply`，在游戏关闭状态下将保存的DLSS设置强制为False；如有修改则保存可恢复备份，仅改变该键。它不阻止运行中重新启用DLSS，完整运行时锁仍是发布待办，不能把只读配置或启动覆盖称作“锁死”。

## Git 检查点：Recovery2

2026-10-14 保存当前源码、测试、启动工具及首版范围。`manifest-recovery2.json` 固定本机已测试的 Recovery2 native 与 SessionExit1 XR 构建；它是开发检查点，不是发布清单，不替换现有两个默认清单。真头显记录为PID10920，模拟器确认框/重载记录为PID38516；原始运行证据保留在被忽略的 `analysis/runtime`。

本检查点的native源码中 capture.cpp、frame_link.h/test、gpu_viewport_stats.h/test、anchor_observer.h/test 均与Recovery2构建冻结副本一致。已执行26项native检查，其中FrameLink包含15720项检查；新增启动DLSS策略7项测试通过。已知未完成项为启动菜单接入、ADS/换枪UI与字幕、6DoF体验校准、运行时DLSS锁和部署包。

Git 保存源码，二进制与第三方依赖保持在本机并继续忽略。恢复历史源码时请新建独立检出目录，避免覆盖当前工作；不要直接覆写仍要回滚使用的 `build-recovery2` 和 `build-session-exit1`。另行重编译可使用独立构建目录，native CMake 指定 `-DCAPTURE_OUTPUT_NAME=GearsCaptureRecovery2`；编译环境需要MSVC、CMake，以及既有MinHook/OpenXR loader依赖。重编译产物的SHA256可能变化，必须重新验证并使用独立清单，不能把旧哈希直接用于新DLL。

## 自动模拟器预览

当前本机预览入口使用用户指定的 OpenXR-Simulator v1.5.0、PauseUi1 native 与 SessionExit1 XR 构建：

```powershell
& J:\gearsVr\tools\vr-launcher\SimulatorVR.ps1 start
& J:\gearsVr\tools\vr-launcher\SimulatorVR.ps1 status
& J:\gearsVr\tools\vr-launcher\SimulatorVR.ps1 stop
```

start 会持续运行，直到在另一终端执行 stop。开始前须正常退出已有游戏；脚本会自动启动游戏、按内存菜单状态进入现有战役，再独立确认关卡就绪并开启同帧双目。不会改系统 OpenXR Runtime，单独生成绝对 DLL 路径清单以兼容本机 loader。

该入口使用独立的 `manifest-simulator-preview.json`，未替换原 `manifest.json`。已完成 4K 场景与六轴模拟姿态的 120 秒测试；这不证明物理头显或所有关卡状态均已验证。

PID39464 后续已覆盖非空 `source+0x140`：正常转镜头触发真实隐藏集合，4055 次全部接受，最大13节点、六项集合检查通过。约60秒发布/复制4930张双目图像，四阶段各4944对且矩阵无错配，正常退出并恢复28入口。此缺口已关闭，不把它继续列为待验证项；证据为 `analysis/runtime/sim-source-map2g-39464.audit.json`。暂停/恢复、重载等更完整的使用流程仍须验证。

**暂停菜单黑屏已在 PauseUi1 的模拟器测试中修复（PID14128）。** 暂停使用另一条最终输出路径 `d808d8`，随后还会按整屏绘制菜单；此前前者没有被识别，后者会破坏双眼画面的匹配。新版识别这条输出路径，并仅在同一命令列表已有完整双眼画面、调用链匹配的情况下，把菜单分别绘入左右眼。未知写入和过期保护继续保留。

实际暂停10秒内发布724→1484，模拟器双眼截图均显示完整可读菜单；恢复后继续出图。全程约60秒，最大XR提交间隔19.7603毫秒、stale拒绝0；25974组菜单双眼绘制均通过关联检查，四阶段4430对/mismatch0。退出恢复28入口、owner/XR退出0。发布4423与XR复制4422相差1，严格等数检查仍为false，不能据此声称每张源图均被接收。当前只验证了该关卡的一次暂停/恢复，主菜单、重载、其他画质路径及真机新版本仍须另测。证据：`analysis/runtime/pause-ui1-14128.audit.json`，原图 `analysis/runtime/pause-ui1-14128-paused-mcp-capture_screenshot-1.jpg`。

后续 PID17732 重复暂停试验发现了更具体的限制：两次暂停/恢复持续发布，但打开“读取检查点”确认框后，`frame_link` 每命令列表32条事件容量溢出（11443次），发布停在10731，anchor匹配仍继续增长。XR最大提交间隔33611.1284毫秒；正常清理、进程退出0和28入口恢复不表示连续显示通过。确认框尚未确认，不能据此判断关卡重载或对象重新绑定是否成功。原菜单控制器还暴露了本地化键识别与动态文本快照问题，完整失败记录为 `analysis/runtime/pause-ui1-reload-17732.failure-audit.json`。

隔离实验构建 PauseUi2 已合并连续且完全等价的菜单关联记录，保留实际绘制顺序、逻辑次数和未知写入拒绝；32条不同事件的容量保护不变。10532项针对性检查及24项原有native自测通过，证据 `analysis/runtime/pause-ui2-build-evidence.json`。但 PID29092 实测仍失败：确认框10秒发布1183→1183，事件溢出5925次；合并记录不足以修复确认框，需继续区分绘制关联失败与后续容量溢出。正常确认读取检查点后已观察到新Pawn，anchor身份检查持续拒绝，证明同关卡角色重新关联仍未实现。本轮4次菜单输入、正常停止、28入口恢复均不能替代出图成功；预览入口不升级到此实验构建。

Recovery1 合并实验版已编译，26项本地测试通过（`analysis/runtime/recovery1-build-evidence.json`）。它允许已提交到同一队列的完整世界双眼画面与后续独立命令列表中的已知菜单绘制关联；仍拒绝过期票据、未知写入及错误队列。角色恢复方面，每次检查重新读取当前受控Pawn，保留原Controller、Camera、Level及对象注册校验，并在场景调用前后比对角色身份。仅覆盖同关卡角色替换，不宣称完整切关恢复。实验清单为 `analysis/runtime/manifest-recovery1.json`；CPU通过不代表确认框持续出图或重载恢复通过，预览入口暂不升级。

Recovery1 的 PID43012 实测仍未通过：确认框发布713→812后停滞，换Pawn后30秒也未恢复。新Pawn在renderer阶段已接受2049次，说明角色重新关联已实际生效；原ViewState与右眼ViewState在本次同关卡重载前后保持相同。菜单跨列表分支尚未触发，前置窗口拒绝随后发展为3860次事件容量溢出，不能将跨列表假设当作已证实根因。正常停止与28入口恢复通过，记录为 `analysis/runtime/recovery1-43012.failure-audit.json`。

Recovery2 针对上述持续拒绝增加相邻、同资源的未知写入记录合并。未知写入仍污染对应帧；仅压缩重复记录，不清除容量故障、不跨眼图/菜单/资源边界合并。不同事件填满容量仍拒绝。26项本地测试通过（FrameLink包含15720项检查）；实验清单 `analysis/runtime/manifest-recovery2.json`，不替换预览入口。

Recovery2 的 PID38516 实际确认框停留10秒发布685→1330，检查点重载换Pawn后继续观察约30秒，最终发布3458、XR复制3457。事件记录最高3条、溢出0、sticky=0，合并192次重复未知写入；31331组菜单关联接受，四阶段3467对、矩阵无错配。这次不再持续停图，但仍有最大2123.7855毫秒提交间隔及87次stale拒绝，不能宣称全程无间断。正常停止、XR退出0与28入口恢复通过。唯一模拟器截图请求发生于会话结束后并超时，没有本轮像素截图证明；真头显仍待验证。原始报告 `analysis/runtime/continuous-stereo-38516-20261014-142649-08d006bf.json`。

分辨率应以实际眼图为准：当前3840×2160游戏源图按左右各1920×2160提交；`GameGpuScreen` 按接收纹理大小创建XR交换链。模拟器配置中的1280×1400是推荐眼图尺寸，并不表示这条游戏图像路径已被缩成该尺寸。模拟器预览窗口缩放、实际XR眼图和物理头显显示清晰度需分别判断。

正规入口 PID34688 实测：自动五步菜单 21.469 秒，独立玩家就绪后运行 60 秒，发布/复制均 4622 张；四阶段矩阵无不匹配，正常停止后 XR/启动器退出0、28入口恢复。本次无临时启动wrapper、无额外菜单输入或截图。完整诊断位于 `analysis/runtime/continuous-stereo-34688-20261014-105921-a1581368.*`。

## 通用入口

此入口复用当前研究工具，不是可复制部署包。保持本仓库及已验证游戏目录布局，使用仓库配置的Python运行：

```powershell
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/vr-launcher/launcher.py start
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/vr-launcher/launcher.py status
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/vr-launcher/launcher.py stop
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/vr-launcher/launcher.py recenter
```

启动自动检查manifest游戏/native/XR哈希，拒绝任何现存游戏进程，一次执行预留与priming。默认等待进入关卡；增加 `--auto-continue` 可按已验证的内存菜单状态自动进入现有战役，无需逐页截图。导航最多 60 秒，包含在原有 120 秒启动期限内；未知菜单、身份或窗口变化会停止，不激活 VR。F8/stop 可取消导航，取消后仍需原有 prime watchdog 恢复。

`--manifest <路径>` 可选择独立构建清单，所有 EXE/DLL 哈希校验仍然执行。`--runtime <OpenXR JSON路径>` 仅给当前启动器及其子进程设置 `XR_RUNTIME_JSON`，不更改系统注册表。默认构建清单保持不变；实验构建的验证结果不自动替换它。现有底层启动脚本会应用其已验证启动配置；恢复仍使用其原有恢复工具。

玩家判定额外要求注册的WarPawn、Controller双向关系、Level归属，以及连续两次相同玩家且摄像机时间戳前进。从预留调用开始最多等待120秒，给XR预检及激活留出priming余量。超时不会重试、激活或杀游戏；保持现场并等待底层180秒watchdog恢复后正常退出游戏，再重新启动。

PID44640已完成入口实测：入口自动启动、识别关卡玩家、启动持续双目并停止；发布与接收2334张，43.959秒内3166次projection，28处入口恢复且清理无残留。证据为`analysis/runtime/vr-launcher-smoke-44640-audit.json`。本轮菜单导航由助手通过game_io完成，入口只等待玩家进入关卡，没有自动操作菜单；此实测不证明物理头显追踪。

`config.json`提供10..500且有限的`stereo_units_per_metre`。`game_path`允许仓库相对路径或绝对路径，但底层当前只支持manifest指定的本机游戏位置；其他位置明确拒绝。

持续runner直接运行在此入口进程，硬杀入口会触发已有native/XR owner保护。停止入口通过`analysis/runtime/vr-launcher-session.json`索引匹配原始会话身份；启动取消为协作请求，预留中的底层调用返回后处理，不杀游戏。原始owner报告保留独立证据。

启动取消返回0只表示控制请求成功，不表示VR验证成功或prime hooks已恢复。取消状态保留`passed=false`、`native_activated=false`与`prime_watchdog_pending=true`；watchdog尚未独立核验时不宣称清理完成。

入口仅严格source0；不启Operator。source0 指 runtime 提供姿态，模拟器也使用此来源，不等同物理头显追踪。正常stop后必须重新启动游戏才能再启同一native会话。本入口尚不提供安装卸载、稳定第一人称锚点。

回中命令核验索引、owner/game/XR创建身份与native启动QPC，再复用既有请求接口；仅输出`requested`，不证明已应用。`status`显示匹配当前会话的`reference_generation`；不匹配或旧构建无代际字段显示null。

支持回中的新版native可在游戏前台按F9，或使用上述命令。回中采用当前位置与LOCAL Y-up水平朝向基准，保留真实pitch/roll，不把抬头或歪头归零。下一对有效且水平朝向可靠的视图才提交新代际，旧帧不修改。

当前manifest冻结Continuous3与XR build-shutdown3，默认不启用shutdown诊断；PDB不参与运行。PID36212的显式模拟source1有界120秒试验已验证两次命名事件回中，代际1→2→3，回中保留pitch；119.968899秒内8639次projection、最大间隔17.2533毫秒，native/XR均退出0且28处恢复。证据为`analysis/runtime/stereo-xr-36212-attempt1.recenter-overall-independent-audit.json`。本轮原点均为0，非零XYZ回中仍仅CPU验证，且不证明物理头显追踪。旧版本缺事件时保持不支持错误。

PID41704已实测统一入口strict source0的两次回中，命令成功且status匹配当前会话的代际1→2→3；但整轮失败，不能视为新版入口验收通过。70.7055秒内4981次projection、3748张发布/接收图像，最大间隔1431.7773毫秒，73次stale拒绝；owner、stop与XR实际exit1。native清理和28处恢复通过，游戏已正常关闭。旧runner另记录退出边界的XR身份查询失败，其异常路径可能终止自有XR子进程；本次未记录是否执行terminate，因此退出根因尚不能确定。缺图与退出错误分别保留，证据见`analysis/runtime/vr-launcher-smoke3-41704.audit.json`。continuous runner的passed字段只判定既有生命周期契约，不表示100ms连续输出验收通过。

runner已消除该二次PID查询竞态：使用自有Popen句柄等待，只在真实等待超时后升级终止，并记录强制操作。随后通过隔离实验入口和outage4诊断构建执行PID1756单次测试：两次回中代际1→2→3，60.1401秒、最大间隔18.0283毫秒，owner/stop/XR实际exit0，清理时仅wait且无强制终止，28入口恢复。发布3386、XR复制3385的严格等数仍为false；旧缺图本轮未重现，尚未确定根因。证据`analysis/runtime/outage4-1756.audit.json`。此实验没有改本目录manifest；outage/shutdown记录均为底层runner的显式诊断选项，默认关闭。
