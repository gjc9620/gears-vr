# 仓库内统一入口

2026-10-24 **运行时支持诊断已接入默认构建**（`build-support1`）。正常重新打开启动器、复现问题后，使用原有“导出日志 / Export logs”即可收集：runtime提供的头显系统名称／厂商／追踪能力，游戏各交换链与XR实际选卡及匹配关系，双方已加载图形模块的文件版本，相关已安装／运行中VR组件版本，交换链创建／绑定／resize／Present进展，以及持续保存的native状态与首次故障回执。头显连接方式和厂商未提供的版本标为不可用，不猜测。诊断只读并保留原渲染／颜色／左眼历史修复，不自动上传、不改注册表。

`debug/session-N.json` 的 `runtime_observation_coverage` 逐项标明是否收集；`.support.jsonl` 的数据整理在其中，`.first-failure.json` 保留启动器首次观察到的故障，原生首次失败API返回也保留原QPC时间。后续清理错误不会覆盖首次回执；需结合时间线判断根因，不能把首次失败API直接当作根因。每个交换链使用会话内匿名编号，未导出指针。长会话轮转保留最近事件，并随日志保留最初设备信息及其原观察时间。`software-environment.json` 是导出时补充环境；每次运行的 `.support.jsonl` 才是该次观察。详细验证与限制见[0.0.4开发记录](../../docs/releases/0.0.4/PROGRESS.md)。此更新尚未发布为新版本包。

2026-10-23 当前开发版为 **0.0.4 Dev**。已将VR左眼的历史状态与普通玩家渲染分离，修复在实际游戏／模拟器中复现的“左眼清晰与强烈拖影模糊跳变、右眼正常”问题。默认native为 `build-left-history1`，原颜色策略和FPS两槽修复保留；同配置源图／预览对照、瞄准、暂停、检查点重建与退出清理已自动验证，物理头显仍待用户验收。正常重启根目录 `GearsVR Launcher.exe` 即可加载，详见[0.0.4开发记录](../../docs/releases/0.0.4/PROGRESS.md)。本机开发更新未形成0.0.4发布包。

2026-10-24 **导出日志已补齐技术支持证据**，通过正常GUI的“导出日志 / Export logs”使用。ZIP保留最近三次连续会话和三次启动时间线，并增加同次会话的native激活错误／回溯位置、完整范围内的捕获与绑定计数、原始停止原因、XR输出，以及CSV／菜单／世界恢复／遥测的筛选历史；CPU、GPU、驱动、RAM、Windows与OpenXR注册信息随包提供。注册信息明确标记为导出时观察，不能当成旧会话实际加载的证明。新增字段不会使整份CSV失效，头部姿态仍不导出。

查看ZIP中的 `manifest.json` 和 `debug/session-N.json` 可区分缺失、截断、读取时变化和真正为零的计数。每个诊断附件最多读取4 MiB，最多保留8192条记录；较长文件保留开头与末尾，记录预算优先保留错误并披露遗漏数量。总展开预算32 MiB，优先缩减长历史，保留激活／状态证据。路径、账号、凭据、指针、原生指令、存档和二进制不进入支持包。此更新修复本地开发版的导出缺口，已发出的旧ZIP不会自动补回缺失证据，已发布0.0.3包未修改。

2026-10-21 已迁移至用户购买的 Steam 正版。普通入口和 DLSS 入口默认使用 `J:\gearsVr\Gears of War Reloaded\Binaries_x64\GOWDE-Steam.exe`，Steam 库内的原始安装也可手动选择。启动子进程带正确的 Steam App ID，仍需已登录且拥有游戏的 Steam 账号；多个设置文件按当前 Steam 账号选择。旧进度已备份后迁移至正版账号。两份正版共3,609文件完整哈希一致，历史旧副本只有 Steam DLL 和已知启动配置不同；用户要求停止追加兼容性运行测试，本次不宣称正版双目／头显验收。见[迁移与证据](../../docs/retail-steam-compatibility-20261021.md)。

2026-10-21 已接入 `happyotaku playground`：重新打开正常启动器，在游戏主菜单末尾选择同名选项进入 `MP_WarMachine` 本地测试场。机枪后方有11种枪械／投掷物，前方有6个死亡后三秒重生的兽人静止靶；武器拾取后补回，Troika可按原版方式使用。无限备用弹药保留正常换弹和主动装填。菜单导航、实际射击／换弹／重生、上下机枪、双眼GPU出图、普通战役回归及正常清理均已自动验证；实体头显和VR手柄体验留待统一验收。详见[实现及验证记录](../../docs/happyotaku-playground-20261021.md)。

2026-10-21 第一人称预览已集成到普通启动器。启用“第一人称预览”后，在游戏前台同时按 L3＋R3 或 E＋R 切换，再按一次返回第三人称；初次进入仍是第三人称。新增右摇杆死区，默认2%，修改后下次启动应用；叠加的原右轴命令死区在会话内清零、退出恢复。已通过实际游戏双目视点切换与生命周期验证，掩体、枪械动作和实体头显仍需继续验收，详见[实现及验证范围](../../docs/first-person-preview-20261021.md)。

2026-10-21 玩家启动反馈：正常 GUI 已补齐与游戏 EXE 同目录及一层 Mod 子目录的自动识别；启动预留失败会显示子进程具体原因和退出码。此修复不更改 native/XR 默认清单，截图中该玩家启动失败的原因仍待其详细日志确认。见[反馈与验证](../../docs/164-launcher-steam-install-feedback.md)。

2026-10-20 机枪切换延迟优化：默认 native/XR 为 `build-mounted-latency1`。通过两个指针读取提前发现角色变化，缩短候选准备，并立即发布重绑定握手进展，消除原两秒发现周期和一秒状态发布周期造成的等待。159项Python、40项原生/启动检查及两次实际游戏检查点重建通过；正常重启后生效。头显上、下机枪的实际延迟仍待复测，见[计时与验证记录](../../docs/mounted-transition-latency-20261020.md)。

2026-10-20 DLSS独立测试入口已更新为 NgxManual3，保留[NgxManual2启动修复](../../docs/162-dlss-manual-startup-proof-isolation.md)，并减少单描述符复制的CPU跟踪开销。同场景A/B/A、渲染比例1.0、每眼3072×3264，旧版稳定段22.02/21.97，新版25.79对新双眼画面/秒，提高17.2%；三轮GPU出图、正常清理和设置恢复通过。这是DLSS关闭时的实际游戏/模拟器结果，开启DLSS后的帧率和物理头显仍需另验。请关闭旧测试启动器后重新双击此入口；已有GUI保留旧DLL副本。见[优化与验证记录](../../docs/dlss-descriptor-copy-performance-20261020.md)。

2026-10-20 DLSS 头显手动测试：双击根目录 `GearsVR DLSS Test.cmd`，选择 Quest / Virtual Desktop (VDXR)，
启动后先进入关卡，再从游戏画质菜单开启 DLSS（先用 Quality/质量）。此独立入口已解除菜单锁，
保留正常音频及前台启动。首次保持同一关卡、分辨率和渲染倍率；结束后正常退出游戏。
重新测试需关闭测试启动器，再次双击该入口。普通入口保持原配置。
见[实验构建、操作及验证边界](../../docs/154-dlss-sr-startup-and-output-route.md)。

2026-10-20 关卡载入后的 VR 启动/退出修复：默认 native 为 `build-startup-output5`，包含初始化武器重绑定、普通 Output 到 owned 目标的两步输出识别和1024项命令列表容量；保留 MountedRecovery6 关卡恢复、坐姿准星和 MenuSmooth1。39项原生检查通过。动态模糊开启、渲染比例1.0、每眼3072×3264的实际游戏/模拟器测试取得791对新画面、1656帧立体投影，GPU双眼图像和正常清理/设置还原通过；物理头显仍待复测。正常通过根目录启动器重启游戏即可加载。见[原因、修改与验证范围](../../docs/161-vr-startup-output-fallback.md)。

2026-10-20 准星有限目标视野边缘修复：默认 Native 切换为 `build-seated-reticle1`。侧移、歪头或近距离瞄准时，有限目标越过单眼视野边缘仍按原世界点投影和裁切；无法投影时不再跳到忽略位置偏移的无穷远方向。729组姿态/距离组合与必要启动自测通过；本次实际游戏回归因已有游戏运行而未启动，真头显与真实子弹落点仍待验证。正常关闭当前游戏，再通过根目录启动器重启游戏与 VR 生效。见[修改与验证范围](../../docs/159-seated-reticle-world-target.md)。

2026-10-20 选关菜单黑屏修复：补齐“选幕 → 选章节 → 难度 → 确认框”的战役菜单显示识别，避免返回主菜单后这些页面被当作未知状态而撤掉头显菜单。正常退出游戏并重新启动 VR 后生效；105项回归、VDXR会话四页回放通过，用户已确认真机问题修复。见[原因与验证](../../docs/158-chapter-menu-black-screen.md)。

2026-10-19 暂停菜单修复：默认 XR 已切换到 `build-pause-stereo1`。暂停继续提交同一票据的左右眼原生投影，保留背景立体感，并去掉旧共同 Quad 对已经投影的菜单进行的第二次比例转换。两轮实际游戏共验证四次暂停/恢复，原始 GPU 菜单图像、约2m UI深度测量和72组几何回归见[验证记录](../../docs/156-pause-menu-stereo.md)。重启游戏与 VR 后，仓库根目录 `GearsVR Launcher.exe` 会读取更新后的 `manifest.json`。

2026-10-15：共用普通游戏启动入口现默认启用可恢复的后台控制器补丁，并以“不激活窗口”标志创建游戏；后台转镜头和角色移动对照通过。游戏自身后续激活窗口的问题按用户要求暂缓。具体范围、日志和恢复方式见[后台手柄记录](../../docs/67-background-controller.md)。

2026-10-15 当前玩家入口：仓库根目录 `GearsVR Launcher.exe`（[源码与构建](ui/README.md)）。GUI与脚本统一使用`manifest.json`，现与`manifest-game-recovery8.json`相同，对应GameRecovery8 native / SpaceRecovery3 XR。Recovery5及原GameResolution1独立清单保留回滚。Low/High已撤下，1920×1080输出分为两个960×1080眼区，3840×2160分为两个1920×2160眼区。新版恢复同关卡重载后的UI角色绑定；仍有约0.87秒的最长实测提交间隔，已修复批次表满后准星校正/HUD布局永久失效，实际手枪开镜复测恢复；有限距离瞄准、换枪/字幕完整验收及CG仍待完成。见[准星修正记录](../../docs/64-reticle-and-ui-capacity.md)。

GUI支持模拟器／系统Runtime、启动、停止、状态和日志。`SimulatorVR.ps1 start`手动进入关卡，不自动导航。主菜单建立XR平面显示，进入关卡后交接立体；保留DLSS/DLAA保护、HUD平面与准星方向修正。运行不限时；正常停止后重新开启VR需先退出游戏。当前加入缓冲重建恢复、VISIBLE状态渲染、短缺帧图像保留和认证Material双眼绘制。150秒模拟器测试覆盖1080p→4K、同进程检查点重载→1080p，历史59秒长黑屏未复现，但仍有最长0.734秒提交间隔，整体连续性标准未通过；准星、HUD与CG仍待完成，也未做本版真头显验收。构建哈希与限制见[修正记录](../../docs/63-checkpoint-recovery.md)。

以下为历史实验和接口记录；旧段落的“默认未切换”、自动导航与启动时限，以各实验构建为范围，当前入口以上述说明为准。

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
