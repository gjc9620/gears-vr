# Final deployment delivery target

## Current range (%)2026-10-16）

User has subsequently revoked the first Low/High standalone eye map slot, requesting current resolution rendering of the game; each eye below 2K/4 K. The stand-alone target and the old percentage table are kept as historical records, not being used as a basis for current first-version delivery requirements or calculation of progress.1920×1080 and 3840×2160 Game source path; eye map sizes are determined by actual SBS, and square texture does not constitute a condition for VR correctness.

The first edition still requires third person to be a pair of eyes and head.6 DIF/FOV/Runback, start VR and menu/gauge/checkpoint transitions, complete UI categories listed by users, DLSS/DLAA disabled locking, ordinary play and pass stability, and player deployment and recovery without the need to develop environment. First person ' s interaction with hands weapons is still on track.

Current Status With `docs/39-mainline-progress-and-acceptance.md` The most recent evidence quoted is: local GUI default has been switched to Native 23/XR 16 The blog is a study candidate.1080 p and 4 See you back at the K Checkpoint/Main menu.docs/70，Recovery 8 Keep rollback; the graphical lock is connected when running, and cannot follow the old conclusion below " Not yet reached." The six-axis virtual image evidence for simulator docs/71, real headscale and comfort remain to be accepted. Chinese studies on GUI and path selection/HUD size/stop/state portal are available but still relying on warehouse and Python, not all of which are ready for hand-held; UI coverage, CG and short-term flashes have yet to be completed.

## Historical requirements and experimental records

2026-10-15 OwnedXr 1/2: The experiment has already completed the separate Law Texture and freeze the paper ticket access to shared publicsher, which is actually done separately 120 Frame/119 issues;Xr 1 Receive the full eye map back to the end. But both rounds are equal 0: medium age of producer at time of publication 147.10 ms, more than reservation 100 ms door limits. Build/queuing and scene consumption still need to be addressed, not being able to upgrade the default list or claim independent resolution VR availability.39 Experimental return and 6 The test was checked for compatibility.58 The entrance is restored and exited normally; Owner remains here until exit.`analysis/ui-position-runtime/owned-xr2-42588.audit.json`。

2026-10-15 OwnedSequence 3: Independent Low Goal accomplished 16 A continuous scene picket, with two copies of the first and second GPUs.2048×2048(a) Eye maps, which are not dark and change over time;34 Return, return.58 Access is restored and normal exits are passed. Only the boundary scene output is verified, Canvas/XR has not yet reached this resource, High and long-time travel are unknown. Owner and creation list references remain available until process exit, starting with inlinecomplete, cannot be used as deployable evidence.`analysis/ui-position-runtime/owned-sequence3-45772.audit.json`;list of experiments`tools/vr-launcher/manifest-owned-sequence3.json`, does not replace the default release list.

2026-10-12 Users are clear: ultimately, a deployable version is needed.
The compiled successful sampling tool cannot be used as the final Mod delivery.

The final delivery shall include:

- A centralized, clear Mod catalogue and replicable deployment package contains only the codes we have prepared and the dissemination dependencies.
- A startup entrance, which automatically recognizes games, versions, players and headers, does not require manual designation of a PID or running sample script.
- Version check, editable configuration, centering/stopping access and logs for easy positioning.
- Repeatable installation, start-up, recovery and unloading instructions do not cover the unknown original documents.
- Complete validation in restart, reload and ordinary play processes, with actual support scope and known problems.

2026-10-14 Users are clear: Release 1 first releases the third person ' s two-fold frame VR Mod, which is activated and tested in the user ' s presence.
The first edition includes a reliable head.6 DIF (rotation and right, left, front, front/down, direction, ratio and near view correctly), start-up i.e. VR access, display switch to the menu to level, correct acoustic/show UI/subtitle, and DLSS closed and run without re-enablement.
Maintains the current available third-person double-eye baseline; first-person, controller and VR special bunker/fly pounce interactively to be left for subsequent version. Normal operations such as original game mouse/handhold bunkers, roller are still within the basic play compatibility of the first edition.
Follow-up interactive design still reference docs/10-vr-interaction-design.md, does not use all its contents as a threshold for publication in the first edition.
The first version is deployed in the direction of an independent portable starter (planned, not achieved):

- Player downloads and depresses the Mod package, selects the first location for the game installation; the starter saves the path and checks the supported EXE version.
- Player connection headlights, starter recognizes OpenXR environment and allows selection; runtime selects only for Mod process without unauthorized changes to the system ' s global settings.
- Click Start VR, load Mod by the starter, display menu and then connect to the double-eye of the level. Provide stop, center and export diagnostic log portals.
- The release of packages is ready to distribute the operational dependencies; players do not need Python, Ghidra, IDA, compiler tools or develop warehouses.
- The original game file is not covered by permanent patches; settings are set to save backups, close/offload the actual change record and not overwhelm other settings that have been modified after the player.
- Updates the list of files that replace the Mod application and maintains the user profile; unmounts to stop the Mod, restore changes in its settings and then deletes the Mod directory.

Initial support ranges only for actual validated game version/issue platform/headprint runtime, and cannot extend the test of the current build to all Steam or other store versions. Unknown version is clearly not supported and does not test writing deviations.e 80aad 0 It's all in line. `config.game_path` The portal still relies on the warehouse Python and build directory, relying on seals, life-cycle interfaces and unmounted recovery to be delivered.
Specific infusion packages and final EXE/DLL documents to be deployed are finalized.

2026-10-15 User requests to align progress with the full first edition and expand UI screen-up to a full game interface.UI compatibility list includes: main menu/sets/stopping/loading and bullet windows;(b) Normal and open-scope, weapons-enabled;(a) The replacement of gun icons and text;Ammunition, active reloading, life/injury tips;Operation/pick-up/tip-through;Mission, teammate and landing rescue markers;Real subtitling, chapter/checkpoint/extratext;Death/failure and retest interface.The list is the range of delivery, and the actual mapping paths and displays must still be recorded on a case-by-case basis and cannot be replaced by an ordinary HUD sample.(a) Public Tile/Text/Material/UIScene path priority batch processing;The quasi-star and world markers are to be kept in their respective targets, depths and eye-to-eye projection, which is not uniformly moved to the centre of the screen.

Estimated progress of the current first-page project **45–50%**. This is estimated by functional weight, not test coverage, accepted function ratio or projection of remaining time. The following crude particle completion factor is weighted:47.75%Only about half of the external reports are available; independent helper and static back-compilation is only prepared, not operational.

| Functions | Weights | Estimated completion of work | Evidence available and remaining threshold |
| --- | ---: | ---: | --- |
| The third person is a pair of eyes. | 20% | 90% | Real headlights are available for playback, multiple-wheel simulator stable output; more status/level still is steady |
| 6 D.F., F.V., middle and distance. | 10% | 60% | Position and eye-to-eye FOV entry matrix; horizontal direction/scale/nearview differential and lens distance require real machine verification |
| Start VR, menu and level switch | 10% | 60% | Starts the full cycle of plane level double-eye and XR sessions; paused/returned/re-introducing incomplete |
| Full UI | 20% | 25% | Partial icons/text links and HUD plane are running; acoustic, real subtitles, Materal, UIScene and world tags are still missing |
| Every eye.2K/4 K-Independent Rendering | 10% | 25% | Two test charts and independent XR eye links passed; original game scene target not yet independent |
| Locking on DLSS/DLAA running | 5% | 25% | Start configuration shutdown and CPU route strategy completed; actual hoops and menu changes stopped not enabled |
| Stability and normal play. | 15% | 50% | Normal stop/entry recovery of evidence available; reese, load, menu cycle, bunker roll still to be covered |
| Player Deployment and Recovery | 10% | 15% | Research starter and version check exists; path selection, reliance on sealing, loading and unloading and portable packages are not completed |

Current priorities: Independent scene output and desktop mirror separation, parallel to UI public channel inventory; then complete class-by-category UI revision and real subtitles; access graphic lock and full life cycle; end with regular machine games, packing and restoration validation. First person/controller gun is still in the range of subsequent versions.

The preparation code for locking on the graphics is located at `tools/native-capture/graphics_lock.h`Yes.38 The CPU test, covering menu requests, parameter collections, queue consumption and maintaining the call behaviour when failure occurs. It has not yet accessed native books or had a real-time menu switch-off validation, so it is still not possible to declare "DLSS locked." The current startup setting policy only ensures that the DLSS is closed before the pre-start durable configuration. The actual line/ABI and parameters are validated as independent boundaries. `graphics_lock_routes.h` The direct part of the line is distinguished from the queued part of the lined entrapment, which requires explicit verification of caller/thread. CPU route testing has been passed but was not accessed and validated at the time of running.
Game and third-party raw tools, game packages are not included in the release package or Git.

2026-10-14 Resolution requires update: VR paints only provide the Low / High two tranches, per eye 2048×2048 / 4096×4096 Design. The resolution of the game menu and the size of the desktop window does not determine VR scene resolution; the desktop is a stand-alone mirror. The slot starts with the selection, switch and restart VR. The default Low, High is an optional slot. DLSS/DLAA continues to be disabled; texture, shadows, etc. do not change without permission from this resolution slot.

This is a goal to be completed, not a current state of achievement. 3840×2160 Backbuffer issues SBS, which is every eye 1920×2160. Real Low/ High requires independent engine scene retrofitting targets (SBS) 4096×2048 / 8192×4096, matching scene color/dispth/postprocess size and final output verification; cannot be called native after magnifying existing images 2K/4 K. The XR receiver end should use a stand-alone image, runtime before startup, with an eye and single size limit, without assuming support 8192 Large combined XR diagrams. If the slot is selected beyond the limit, it is clearly reported and the slot is not down in silence.

2026-10-15 Create validation of actual game original target: both SBS sizes create expected RGBA 8 Single-sample colour texture and release the reference held in this probe from the original line; no target is bound to be drawn or the above actual display size changed. The visible two-tier boundary of the source view has also returned through CPU. The scene/Canvas redirection, target holding period, GPU status/queuing connection and desktop mirrors remain gaps. The native pool may be delayed for recovery, and emptied does not amount to immediate destruction of physical distribution as proven.

OwnedMetadata 1 Further validates the independent profile/gang height/show parameters and normal releases in actual games. The displaying parameters removes dependence on the long-term survival of the original view object. The login is still called from the paragraph-by-paragraph validated original logic, retaining the current gamma; no copy was made of the original viewer.299 CPU Checks and 13 The block GPU status wire test passed, with existing binary and 40 The entrance is restored. New textures have not yet entered scene/Canvas/GPU, so independent resolution remains to be completed; actual queue holdings with GPU fence retirement, complete barrier coverage and desktop mirrors cannot replace the probe. `analysis/ui-position-runtime/owned-metadata1-49524.audit.json`。

Follow-up, OwnedScene.1 Already point a single Low picket coppy-family target at this adapter: two eyes fixed 2048×2048, two final Draw matching 4096×2048 Independent resource, and return once in line with the same DIRECT; renderer is normally destroyed without a failure. Current evidence stops when orders are submitted, pixels, GPU fence, Canvas and continuous output remain unverified. Experimental raw references are reserved to process exit, and the full-cleaning of the starter is judged as incomplete;40 The entrance is independently verified and the game is normally out of play to finish the experiment. This step does not change the resolution evidence available to the player. `analysis/ui-position-runtime/owned-scene1-37624.audit.json`。

The image projection continues to use every asymmetric FOV and bits returned with square texture that does not mean square or symmetrical view field. High resolution means pixel budget, AVP compatibility with other high-end header displays must also be verified separately for their PCVR streams/OpenXR paths, running time limits and actual displays, without prior commitment of support.

OwnedPixels 5 The GPU, which has achieved the actual single-frame Low Independence goal, reads:4096×2048 The continued independence of Low/High, the intermediate scene buffer size, desktop mirroring and normal retirement are still incomplete without updating the percentage of the player's resolution or overall project.56 The entrance resumed and the exit was normal, but the experiment Owner intentionally retained the process to exit and the starter remained incomplete. `analysis/ui-position-runtime/owned-pixels5-40520.audit.json`。

Historical status test correction: the source Barrier interface was wrongly tied to the slot 24, now corrected to SDK slot 26♪ And join in 30 entry compilation period offset check; previously "13 The conversion conclusion of the GPU state connection test has been replaced.13 Downed-output,8 The debug layer is not available when the scene-GPU and WARP pixels check is passed.5 The actual target is maintained at RT from creation to reading back, without covering the side state conversion branch.[Recent evidence and limitations](00-roadmap-and-learning.md)。

---

## 中文

# 最终部署交付目标

## 当前有效范围（2026-10-16）

用户后续已撤销首版 Low/High 独立眼图档位，要求跟随游戏当前分辨率渲染；下文的每眼2K/4K、独立目标及旧百分比表保留为历史记录，不作为当前首版的交付要求或进度计算依据。已验证1920×1080和3840×2160游戏源路径；眼图尺寸按实际SBS裁取，正方形纹理不构成VR正确性的条件。

首版仍要求第三人称同帧双目、头部6DoF/FOV/回中、启动VR及菜单/关卡/检查点切换、用户列出的完整UI类别、DLSS/DLAA禁用锁定、普通游玩和过场稳定性，以及无需开发环境的玩家部署与恢复。第一人称和双手武器交互仍在后续范围。

当前状态以 `docs/39-mainline-progress-and-acceptance.md` 及其最新证据引用为准：本地GUI默认已切换到Native23/XR16研究候选，1080p和4K检查点/主菜单回归见docs/70，Recovery8保留回滚；运行时图形锁已接入，不能沿用下文“尚未接入”的旧结论。模拟器六轴实际图像证据见docs/71，真头显尺度与舒适度仍待验收。已有中文研究GUI及路径选择/HUD大小/停止/状态入口，但仍依赖仓库与Python，尚非便携包；UI覆盖、CG与短时闪黑也尚未全部完成。

## 历史要求与实验记录

2026-10-15 OwnedXr1/2：实验已将独立Low纹理和冻结pose ticket接入共享publisher，实际各完成120帧/119次发布；Xr1接收端读回完整双眼图。但两轮projection均0：producer发布时pose年龄中位147.10ms，超过保留的100ms门限。构建/排队及scene消费耗时仍需解决，不能升级默认清单或宣称独立分辨率VR可用。39条实验回归与6项关闭实验的兼容检查通过，58入口恢复并正常退出；Owner仍保留到退出。新实验默认OFF、无Canvas，证据见`analysis/ui-position-runtime/owned-xr2-42588.audit.json`。

2026-10-15 OwnedSequence3：独立Low目标完成16个连续scene ticket，首尾GPU读回各为两张2048×2048眼图，内容非黑且有时间变化；34条回归、58入口恢复和正常退出通过。仅验证有界scene输出，Canvas/XR尚未接入此资源，High和长时间游玩未测。Owner及创建列表引用仍保留至进程退出，启动器如实incomplete，不能作为可部署完成。证据`analysis/ui-position-runtime/owned-sequence3-45772.audit.json`；实验清单`tools/vr-launcher/manifest-owned-sequence3.json`，不替换默认发布清单。

2026-10-12 用户明确：最终需要可部署版本。当前 tools 下均为研究原型，
不能把编译成功的采样工具当作最终 Mod 交付。

最终交付应包含：

- 集中、明确的 Mod 目录与可复制部署包，只包含我们编写的代码和可分发依赖。
- 一个启动入口，自动识别游戏、版本、玩家对象和头显，不要求手工指定 PID 或跑采样脚本。
- 版本检查、可编辑配置、回中/停止入口，以及便于定位问题的日志。
- 可重复的安装、启动、恢复和卸载说明，不覆盖未知的原有文件。
- 在重启、重载和普通游玩流程中完成验证，并列明实际支持范围和已知问题。

2026-10-14 用户明确：第一版先发布第三人称同帧双目VR Mod，由用户在场启动和测试。
首版包括可靠的头部6DoF（旋转及左右/前后/上下平移，方向、比例与近处视差正确）、启动即接入VR、菜单到关卡的显示切换、正确的准星/换枪UI/字幕，以及DLSS关闭且运行时不可重新启用。
保持当前可用的第三人称双目基线；第一人称、控制器持枪及VR专用掩体/飞扑交互留待后续版本。原游戏键鼠/手柄的掩体、翻滚等正常操作仍属于首版基本游玩兼容范围。
后续交互设计仍参考 docs/10-vr-interaction-design.md，不将其全部内容作为首版发布门槛。
首版部署方向为独立便携启动器（规划，尚未实现）：

- 玩家下载并解压Mod包，首次选择游戏安装位置；启动器保存路径并核对受支持EXE版本。
- 玩家连接头显，启动器识别OpenXR环境并允许选择；runtime选择只作用于Mod进程，不擅自更改系统全局设置。
- 点击“启动VR”，由启动器加载Mod、显示菜单，再接续关卡双目。提供停止、回中和导出诊断日志入口。
- 发布包带齐可分发运行依赖；玩家无需Python、Ghidra、IDA、编译工具或开发仓库。
- 原游戏文件不被永久补丁覆盖；设置修改保存备份，关闭/卸载按实际变更记录恢复，不覆盖玩家之后修改的其他设置。
- 更新替换Mod程序文件并保留用户配置；卸载先停止Mod、恢复其设置修改，再删除Mod目录。

初始支持范围仅声明实际验证的游戏版本/发行平台/头显runtime，不能把本机当前构建的测试推广到所有Steam或其他商店版本。未知版本明确提示不支持，不试探性写入偏移。e80aad0已贯通 `config.game_path` 到预留加载器和正常启动脚本，启动前验证EXE/安装布局/已知配置，并核对进程创建时间与映像路径。自选安装目前只读，要求DLSS已关闭且配置属于已认证集合；这一能力还没有用户界面。入口仍依赖仓库Python及构建目录，依赖封装、生命周期界面和卸载恢复均须完成后才可交付。
具体注入打包方式及最终EXE/DLL文件名待部署实现确定。

2026-10-15 用户要求按完整首版对齐进度，并将 UI 排查扩展到整套游戏界面。UI 兼容清单包括：主菜单/设置/暂停/加载与弹窗；普通与开镜准星、武器附加准星；换枪图标和文字；弹药、主动换弹、生命/受伤提示；操作/拾取/教程提示；任务、队友和倒地救援标记；真实对白字幕、章节/检查点/过场文字；死亡/失败与重试界面。清单是交付范围，各项实际绘制路径和显示仍须逐项记录，不能由一段普通 HUD 样本代替全部覆盖。公共 Tile/Text/Material/UIScene 路径优先批量处理；准星与世界标记需保留各自目标、深度和逐眼投影，不统一挪到屏幕中心。

当前首版工程进度估算约 **45–50%**。这是按功能权重估算，不是测试覆盖率、已验收功能比例或剩余时间预测。以下粗粒度完成度加权为47.75%，对外只报告约一半；独立helper和静态反编译只计准备工作，不计为运行时功能完成。

| 功能 | 权重 | 工程完成度估计 | 已有证据与剩余门槛 |
| --- | ---: | ---: | --- |
| 第三人称同帧双目 | 20% | 90% | 真头显已有可玩反馈，多轮模拟器稳定输出；更多状态/关卡仍属稳定性验证 |
| 6DoF、FOV、回中与距离感 | 10% | 60% | 位姿和每眼FOV进入矩阵；平移方向/尺度/近景视差及镜头距离需真机复验 |
| 启动VR、菜单与关卡切换 | 10% | 60% | 启动平面→关卡双目同XR会话通过；暂停/返回/重进等完整循环未完成 |
| 整套UI | 20% | 25% | 部分图标/文字关联和HUD平面已运行；准星、真实字幕、Material、UIScene和世界标记仍缺 |
| 每眼2K/4K独立渲染 | 10% | 25% | 两档测试图及独立XR眼链已通过；游戏原生场景目标尚未独立 |
| DLSS/DLAA运行时锁定 | 5% | 25% | 启动配置关闭及CPU路由策略完成；实际hooks和菜单更改拦截未启用 |
| 稳定性与正常游玩 | 15% | 50% | 正常停止/入口恢复已有证据；resize、重载、菜单循环、掩体翻滚等仍需覆盖 |
| 玩家部署与恢复 | 10% | 15% | 研究启动器和版本检查存在；路径选择、依赖封装、安装卸载和便携包未完成 |

当前优先顺序：独立场景输出及桌面镜像分离，与UI公共通道盘点并行；随后完成逐类UI修正和真实字幕；接入图形锁与完整生命周期；最后进行真机常规游玩、打包及恢复验证。第一人称/控制器持枪仍在后续版本范围。

运行时图形锁定的准备代码位于 `tools/native-capture/graphics_lock.h`，有38项CPU测试，覆盖菜单请求、参数采集、队列消费及失败时保持原调用行为。它尚未接入 native hooks，也没有菜单实时切换验证，因此发布仍不可声称“DLSS已锁死”。现有启动设置策略只确保启动前的持久化配置关闭DLSS。集成前仍须验证真实线程/ABI和参数生效边界。独立的 `graphics_lock_routes.h` 已区分同线程嵌套 collector 内的 direct commit 与需明确验证 caller/thread 的 queued commit，CPU 路由测试已通过，但运行时仍未接入、生效。
游戏和第三方原始工具、游戏内容包不加入发布包，也不纳入 Git。

2026-10-14 分辨率要求更新：VR 画质只提供 Low / High 两档，按每眼 2048×2048 / 4096×4096 设计。游戏菜单的分辨率及桌面窗口尺寸不决定 VR 场景渲染分辨率；桌面作为独立镜像。档位初期在启动时选择，切换后重新启动 VR。默认 Low，High 是可选档。DLSS/DLAA 继续禁用；纹理、阴影等质量项目不随此分辨率档位擅自更改。

这是待完成的目标，不是当前实现状态。当前仍从游戏 3840×2160 backbuffer 发布 SBS，即每眼 1920×2160。真正的 Low / High 需要独立的引擎场景渲染目标（SBS 4096×2048 / 8192×4096）、匹配的 scene color/depth/postprocess 尺寸及最终输出验证；不能把已有画面放大后称为原生 2K/4K。XR 接收端应使用每眼独立图像，启动前查询 runtime 的每眼及单图尺寸上限，不假定支持 8192 宽的合并 XR 图。若选定档位超出限制，明确报告，不静默降档。

2026-10-15 已完成实际游戏原生目标创建验证：两种SBS尺寸均创建出预期RGBA8单sample颜色纹理，并在原生线程释放本探针持有的引用；没有将目标绑定绘制，也没有更改上述实际显示尺寸。源视图构造的显式两档边界也已通过CPU回归。场景/Canvas重定向、目标持有期、GPU状态/队列关联与桌面镜像仍是接入缺口。native资源池可能延迟回收，引用清空不等于已证明物理分配立即销毁。

OwnedMetadata1 进一步在实际游戏验证独立 adapter 的 surface/宽高/显示参数及正常释放，移除了对原 viewport 对象长期存活的依赖。显示参数仍调用已逐段核验的原生逻辑，保留当前 gamma；没有复制原 viewport 析构器。299 项 CPU 检查与 13 项 GPU 状态接线测试通过，现有双目和 40 入口恢复也通过。新纹理尚未进入 scene/Canvas/GPU 绘制，因此独立分辨率仍待完成；实际排队持有与 GPU fence 退休、完整 barrier 覆盖及桌面镜像不能由此探针替代。详见 `analysis/ui-position-runtime/owned-metadata1-49524.audit.json`。

后续 OwnedScene1 已在实际游戏将单个 Low ticket 的 copied-family target 指向该 adapter：两眼固定2048×2048，两个最终 draw 对应4096×2048独立资源，并随同一DIRECT队列一次Execute返回；renderer正常销毁，无adapter故障。当前证据止于命令提交，像素、GPU fence、Canvas及持续输出仍未验证。实验原生引用保留至进程退出，启动器完整清理判断为 incomplete；40入口另行独立核验通过，正常退出游戏完成实验收尾。这一步不改变上述玩家可用分辨率状态。证据 `analysis/ui-position-runtime/owned-scene1-37624.audit.json`。

画面投影继续采用头显返回的每眼非对称 FOV 和位姿，正方形纹理不意味着正方形或对称视场。高分辨率只表示像素预算，AVP 和其他高端头显的兼容性还须分别验证其 PCVR 串流/OpenXR 路径、运行时限制与实际显示，不预先承诺支持。

OwnedPixels5已取得实际单帧Low独立目标的GPU读回：4096×2048两眼均有游戏场景及视角差异，实际创建状态、队列提交和读回fence已关联；没有Canvas/HUD，XR仍使用原有输出。持续独立Low/High、中间场景缓冲尺寸、桌面镜像和正常退休仍未完成，不更新玩家可用分辨率或整体工程百分比。56入口恢复并正常退出通过，但实验Owner有意保留到进程退出，启动器仍为incomplete。证据 `analysis/ui-position-runtime/owned-pixels5-40520.audit.json`。

历史状态测试更正：ResourceBarrier接口原误绑槽24，现修正为SDK槽26并加入30项编译期偏移检查；先前“13项GPU状态接线测试”的转换结论已被替代。修正版13项owned-output、8项scene-GPU和WARP像素检查通过，调试层不可用。OwnedPixels5实际目标从创建到读回一直保持RT状态，没有覆盖游戏侧状态转换分支。详见[最新证据与限制](00-roadmap-and-learning.md)。
