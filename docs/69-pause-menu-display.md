# Pause 菜单显示：共同菜单面板与恢复边界

2026-10-16。SpaceRecovery7 已实现严格识别的 Pause 菜单共同面板；CPU 检查通过，实际游戏验证另记于下。未实现或宣称原生 mono 输出边界。

## 实际观察

quiet9k 的 game6680 出现暗画面时，实际显示 Pause / Resume Game，而非已证实的场景全黑。只读 MenuReader 一致复读得到 `DeltaPause-Campaign-Host / WarUIScene`、input_mode=1、player_index=0、input_block_count=0，焦点为 `LBtnResumeSingle / UILabelButton`（Enabled、Focused）。现有 reader 不输出独立 scene_flags。

对应 menu.bin 读取时为 schema1、mode0（UNKNOWN）、sequence927、年龄约225ms；game/owner 的 PID与创建时间匹配本会话。monitor 日志同样记录 `mode0 / DeltaPause-Campaign-Host`。会话证据前缀：`analysis/runtime/continuous-stereo-6680-20261016-001324-ce0a104f`。

协调线程记录一次严格认证后的后台 Enter，见 `analysis/runtime/quiet9k-resume.json`。随后本次只读复读保存于 `analysis/runtime/quiet9k-after-resume-menu.json`：仍为相同 Pause tag/class、player0及 Resume 焦点，但 top 已从 `0x3f5333180 / number1` 变成 `0x162b87a180 / number2`。不能据此宣称已持续恢复 gameplay，也不能仅据此断定是未退出还是退出后重新进入 Pause。本次复读没有发送输入。

## 改动前的源码链路

- `tools/native-capture/menu_display_monitor.py:23–30` 只识别 DeltaGameover 菜单及已观察的非模态 gameplay 场景；Pause 落为 UNKNOWN。
- `tools/openxr-probe/startup_panel_policy.h:19–23` 在首次交接后允许 UNKNOWN 继续 projection；只有现有 death_menu 模式显示菜单quad并禁止projection。
- `tools/native-capture/run_stereo_continuous.py:442–453` 每250ms发布菜单状态。Native9没有消费该菜单协议。`tools/openxr-probe/pose_probe.cpp:455–460` 按XR session状态发布 focused/visible/should_render，472行才读取菜单mode；现有菜单模式不会暂停native SBS。
- `tools/openxr-probe/game_screen.h:220–226` 抓取完整客户区，388–390行把整图作为双眼共同quad；没有单眼crop。若native仍在生成SBS，仅把Pause分类成菜单会把两个菜单一起放进quad。

## 曾考虑的原生 mono 路线，未接线

可复用的临时回退是：在严格认证的菜单显示意图下发布新序列、`should_render=0` 的sample。`tools/vr-common/stereo_frame.h:109` 会返回inactive；`tools/native-capture/capture.cpp:1909–1913` 不因inactive永久停止连续会话，2100行回退原始renderer。仅清focused不足，因为visible也允许渲染。启动诊断项 submit_mono_control 与当前view_union/output_bounds组合互斥（capture.cpp:3131–3133），不应直接动态切换。

**inactive只是拒绝本次双目候选，不是原生ack，更不是原始renderer完成、GPU完成或全幅mono Present的证明。** 必须先设计并验证同一菜单epoch下的真实mono输出完成边界，再让WGC开始接受该边界之后的菜单帧；单纯重置WGC时间戳仍可能采到队列中的旧SBS。恢复gameplay还需现有身份、时间边界及新双目票据守卫。未建立这套边界前，不把Pause直接映射为DEATH_MENU，也不将所有UNKNOWN视作菜单。

## SpaceRecovery7 实现

采用已经完成 GPU 复制并释放的左眼交换链作为双眼共同的 2D quad。沿用原接收器，原生 SBS 继续工作；不引入第二个接收器，不依赖 WGC 截取可能仍为 SBS 的窗口。此面板不计入 stereo/projection 成功，也不代表已实现菜单背景的双目深度。

- MenuDisplayMonitor 增加数值3 `PAUSE_MENU`，只接受实际观察过的 `DeltaPause-Campaign-Host / WarUIScene / input_mode1 / player0`，保留一致复读、只读、无输入阻塞与进程创建身份校验。不同焦点选项仍属于同一 Pause 场景；未推广到未知子菜单。64字节 schema1 不变，旧 XR validator 会将3拒绝为UNKNOWN。
- 菜单 quad 取左眼完整图像 `(0,0,source_width/2,source_height)`，双眼共同显示，宽1.6米、距离2米，高按原游戏窗口宽高比计算。普通 Canvas 的X坐标在 native replay 中缩为一半，因此用 source aspect 还原其形状，而非眼纹理的宽高比。认证HUD有另一套映射，不将此比例规则推广到所有UI。
- 入菜单与恢复均要求新边界之后的已完成图像票据，同时保留 session、origin、年龄、尺寸和资源重建校验。UNKNOWN 不越过待恢复状态；仅认证GAMEPLAY加新票据允许恢复投影。
- `eye_menu_first_frame` 只在 `xrEndFrame` 成功后记录，共同面板计数与投影计数分开。原生构建仍为 GameRecovery9；新增实验清单 `manifest-game-recovery9-window3.json`，默认清单未提升。

冻结 XR7 SHA256：`59220f534b389d51667fa988b6cd236a7ee35261114af11b2ad4537d5198cf2c`。

验证：全量编译成功；直接执行 MenuDisplayStateTest、StartupPanelPolicyTest、MenuEyePanelTest 和 `--projection-receiver-self-test` 均退出0，Python menu monitor 12项通过。CMake 当前未注册 CTest，因此没有把 CTest 的“无测试”当成通过。另完成一次独立窄范围静态审查，未发现确定阻塞问题。以上不等于真实菜单视觉或头显验收。

## 9l 实际运行

会话前缀 `analysis/runtime/continuous-stereo-35456-20261016-004600-51750857`；game35456/创建FILETIME `134365563390630203`，owner34892/创建 `134365563322509862`，Native9 + XR7 + quiet2 + quiet5。首个投影在运行器开始后32.603秒，初始化延迟尚未解决。90秒观察结束后仍持续运行，观察器不会自动停止游戏。

实际记录3次 Pause 菜单进入、3次共同面板成功提交、2次返回投影。每次首面板均为960×1080左眼纹理、1.6×0.9米quad，票据sample_ns晚于该菜单边界；实际模拟器截图显示双眼相同完整Pause菜单、恢复16:9外形。人工头显舒适度及所有菜单尚未验收。

协调线程尝试打开菜单前的状态守卫发现游戏已经Pause，因此没有发送Escape；随后只在认证Resume按钮后发送一次后台Enter（`pause-menu9l-open.json`、`pause-menu9l-resume.json`）。日志确认返回投影，但很快再次Pause；临时采样器的旧registry失效，生产monitor重建reader后识别到新Pause对象，未把这一段当成持续gameplay通过。用户随后明确反馈失焦仍自动暂停，下一项针对失焦暂停路径调查。

模拟器检测器的 `MISSING_PROJECTION_LAYER` 在本轮包含有意提交菜单quad的阶段，不能将累计值直接当成黑屏次数。上述画面检查只有一次截图；其余检查依据menu状态、票据、xrEndFrame及进程身份日志。

## 9m 暂停/恢复复验

修正quiet启动器的开关引用（见文档68）后，game35136的同一后台运行中：认证GAMEPLAY→一次Escape→认证Pause/共同菜单quad→认证Resume的一次Enter→新票据投影。恢复后30秒60次状态均为后台GAMEPLAY，未再自动进入Pause；WorldInfo.Pauser为空且游戏时间推进。此轮无需截图或抢回焦点。证据见 `analysis/runtime/pause-menu9m-{open,resume}.json`、`unfocus9m-after-resume.json` 与 `continuous-stereo-35136-20261016-011605-c5356ffd.stdout.txt`。

## 9t：设置子菜单空白与受上下文约束的分类修复

9t（game37956，创建FILETIME `134365618036683799`）用新增 `MenuReader.sample(include_tree=True)` 在同一复读范围中枚举 Pause 子树，实际找到 `LBtnOptionsSingle`。枚举默认关闭，不增加生产monitor的读取量；检查节点/深度上限、Children/StateStack所有权、焦点成员与完整复读。

只发送有身份及稳定焦点守卫的后台方向键、Enter与Escape，实际进入设置首页、游戏、显示、画质、声音及输入设置，未改动设置值。对应top依次为 `DeltaPause-Options`、`DeltaGameConfiguration`、`DeltaVideoSettings`、`PenderGraphicsSettings`、`DeltaAudioVideoSettings`、`DeltaInputSettings`，均为WarUIScene、input_mode1、player0、block0。所有子菜单的ActiveScenes都保留有序的Pause→Options上下文。证据 `analysis/runtime/menu9t-*-tree.json`、`menu9t-gameconfig-state.json`、`menu9t-options-tour.json`；tour前后foreground相同，未激活游戏。

旧分类器将这些子菜单一直标为UNKNOWN；XR7已从Pause进入待恢复状态，因而既不提交菜单quad也不提交projection。9t末次模拟器状态 `lastLayerCount=0`，XR日志有19369次 `menu_recovery_pending`，这是本次设置页空白的直接软件路径，不推广为此前所有游戏黑屏的根因。结束时原生清理和入口恢复成功、XR退出0，再正常WM_CLOSE游戏。

修复只把上述六个精确菜单映射为既有PAUSE_MENU=3，不改变协议或XR二进制。设置首页要求活动栈中存在准确Campaign Host Pause祖先，其余五页还要求后续Options祖先；top必须与栈末项的地址、name、class、input_mode一致，name等于tag。保留原共同守卫，陌生子菜单、缺失/乱序上下文、不同玩家及不一致采样仍拒绝。16项分类/发布测试通过；实际修复后显示测试另记。键位绑定、主菜单设置及其他尚未观察页面尚未扩展。

## 9u：六类设置页及返回双目的实际复测

9u（game35824/创建FILETIME `134365626163166846`、owner34088、XR16128，`continuous-stereo-35824-20261016-023038-8b6ff376`）加载修复后，完成Pause→设置首页→上述五个设置子页→各自返回→Pause→Resume。`menu9u-options-tour.json`记录18次稳定状态、17次有守卫的后台按键及18次显示核对，每次要求菜单packet身份、时限、mode3、最新monitor菜单名均对应，并要求模拟器相同XR进程的新鲜状态有1个提交层；全程foreground为同一窗口3348328。未改设置值。

一次最终模拟器画面核对显示两眼为相同的完整设置面板。XR日志记录各次共同面板成功提交；Resume后记录 `menu_display_recovered` 与真实新票据projection，模拟器projection计数重新推进。首个Escape尝试遇到EHole提示界面，守卫拒绝且未发输入；待其自然结束后才在认证GAMEPLAY发送。这不计为一次菜单导航失败或重复输入。

本次修复消除了这六类页面持续无层输出的问题。菜单切换时仍观察到短暂UNKNOWN（包括读注册表失效和过渡中不满足分类条件），会暂时不提交菜单层；不宣称切换无闪烁、所有UI已覆盖或已完成真头显验收。模拟器 `MISSING_PROJECTION_LAYER` 累计值包含正常菜单quad阶段，不能直接作为黑屏次数。此次仍使用冻结Native9/XR7，未提升默认清单。

9u随后在后台自然进入DeltaGameover；认证btnCheckpoint后发送一次Enter，日志记录death_menu面板及读取检查点后的 `menu_display_recovered` / 新票据projection。之后仍可再次进入Pause。该轮没有重启游戏或原生双目服务，见 `menu9u-death-checkpoint.json`；只增加本条实际覆盖，不代表所有死亡/加载路径均通过。

## 同址同数量注册表中的槽复用

9u只读探针实际记录到UI对象槽181779从缓存0变成当前 `DeltaButtonBar` 地址 `0x26e89e0`，但objects头仍完全相同：data=16869228544、count=258886、capacity=309766。原reader仅在data/count改变时刷新指针库存，所以对新对象重复抛 `Object registry identity changed`。三次失败后，同一reader重取names/objects库存、清缓存并核验原client身份，约8.994毫秒恢复完整Pause快照；未重新prepare或全局扫描client。证据 `analysis/runtime/menu9u-cache-probe2.json` 和同名脚本。

据此，`sample()`仅对精确类型RuntimeError及精确上述消息增加一次有界刷新：核验两张registry头与有界指针库存，清名称/对象/属性缓存，要求初始化保存的client完整身份不变，再重新执行全部快照、关系与复读检查。失败快照丢弃；第二次失败直接交回原monitor重建路径，不放宽UNKNOWN显示、也不延长旧图像许可。69项reader/控制与16项monitor测试通过；实际自动恢复结果另记。

在同一9u游戏进程中，用新reader再进入Options，实际槽258760同样在头部不变时从缓存0成为 `DeltaPause-Options` 地址 `0x115ed9000`。此次由生产 `sample()` 自动调用一次刷新（9.372毫秒），60次观察没有对外采样异常，无手动缓存干预；完整输出见 `menu9u-cache-probe3.json`。这是新reader对真实游戏的验证，9u原owner已加载的旧reader模块未热替换。另观察到正常转场中 `input_block_count` 从1回0：此条件仍会触发原显示分类UNKNOWN，不能把这一剩余空白全部归于注册槽缓存，也尚未放宽输入阻塞守卫。

## 9v基线与进一步减少转场空白

9v（game34876，创建FILETIME `134365639961272269`，`continuous-stereo-34876-20261016-025336-f18c3cc3`）在正式owner中加载库存刷新修复后，再次通过全部六类菜单、18次显示状态核对与返回新票据projection，tour期间foreground不变。未再外抛旧库存错误；首次进入Pause仍因一次完整ledger复读变化走了reader重建。正常清理后WM_CLOSE退出。

`menu9v-layer-observation.json`在100秒内记录11段零层输出，单段约0.516–1.063秒、合计8.782秒。它是模拟器状态采样估计，含3次短暂文件访问失败，不是像素逐帧测量，也未覆盖稍后的Resume。后续观察器改为Windows共享删除读取，减少与原子文件替换的冲突。日志显示设置页间主要UNKNOWN已对应一致采样中的block1转场。

下一修复分两部分：ledger期间/末次复读变化抛专用SnapshotChanged，丢弃整份快照并从头重取一次，与库存刷新共用总计两次采样预算；最初采样时间不延后。仅精确认证的Pause及这六类设置页面允许整数block0或1继续显示共同面板，输入控制、GAMEPLAY恢复和DEATH_MENU仍要求0，其他数值/类型/菜单仍拒绝。已有脚本中的BlockUIInput包含未解析native token，TransitionAnimStart/End仅为native声明，不据此宣称完整还原了转场实现。72项reader/控制与19项monitor测试通过，9w实际复测另记。

## 9w：正式显示链路的转场复测

9w（game12724/创建FILETIME `134365643321455031`、owner36212、XR1500；`continuous-stereo-12724-20261016-025912-293fff89`）加载上述两项修复，在后台完成Pause→设置首页→五个设置子页各自返回→Pause→Resume。19次状态/显示核对、19次有守卫输入通过；返回后又有30次一致UI采样，未自动重入Pause，XR成功提交新票据projection。tour前后foreground均为2955120。

100秒 `menu9w-layer-observation.json` 覆盖本轮菜单路径及Resume，仅记录GAMEPLAY→PAUSE_MENU→GAMEPLAY三次状态，所有采到的模拟器提交层数均为1，未采到零层区间，观察器无读取错误。正式menu日志也未在这些设置页间降为UNKNOWN，XR只有一个Pause epoch，随后成功恢复projection。与9v相比，这条路径的重复菜单层撤销已消除；状态采样不能排除采样间的极短异常，也不等于逐像素测量或真头显验收。未改变冻结Native9/XR7、游戏分辨率或默认发布清单。未认证菜单、预录视频和其他游玩黑屏仍分别待验证。

## 9w：退出确认与主菜单重建的独立缺口

在同一9w进程中，按已认证Pause焦点顺序进入 `LBtnSurrenderSingle`，实际出现 `WarfareConfirmation / UIScene_Confirmation / input_mode1 / player0 / block0`，焦点是仅Enabled的scene根，而非按钮。只读Source文本为 `Title=<Strings:WarfareGame.Generic.ConfirmSurrender_Title>`、`MessageText=<Strings:WarfareGame.Generic.ConfirmSurrender>`；ActiveScenes保留准确的Pause祖先。旧分类UNKNOWN，形成持续菜单空白。证据 `menu9w-return-prompt-tree.json`。现只对该精确对话及Pause上下文复用共同面板，不泛化全部确认框，也不改变按键许可；22项分类/发布测试通过。

已保存的脚本 `UIScene_Confirmation.CustomInputKey` 显式包含Enter→PlayerAccepted分支，但其表达式含未解析native token，不能当成完整语义还原。一次身份、稳定快照、精确文本与scene根焦点核验后发送后台Enter，实际返回DeltaMainMenu；随后用现有菜单控制器的三次认证Enter继续当前存档。`menu9w-confirm-return.json`、`menu9w-main-continue.jsonl`保存真实结果。

关卡重新加载后，UI已回认证GAMEPLAY，但Native9发布计数停在13770，anchor匹配停在67132，拒绝持续增加；终止日志为identity_failed。当前只读比较 `menu9w-anchor-difference.json` 证明旧controller/camera/pawn已失效，旧PersistentLevel地址已复用为CoverLink；WarLocalPlayer仍保持原身份，新相机/控制器已能由现有resolver发现。故此路径需要新world的完整证书重建与安全代次切换，不能以放宽身份或复用旧票据解决。停止时原生清理、入口恢复及XR退出0均通过，再正常WM_CLOSE。

主菜单显示还有另一条独立缺口：当前已交接的XR在UNKNOWN时不显示WGC；主菜单又缺少可用新双目票据。frame_link的“没有关联到双目”不证明源是全幅mono；其CPU关联不覆盖全部clear/copy/indirect/bundle。后续需正面认证菜单输出布局、backbuffer/resource代次及GPU完成边界，才选择全幅菜单或裁剪单眼。Astra Medium只读审查已确认此边界，尚未据此修改GPU协议或声称主菜单恢复完成。

## 9x：确认框文字重建竞争

9x（game41388，`continuous-stereo-41388-20261016-032553-88ad3bb5`）加载确认框分类修复，monitor确实输出mode3并成功提交共同面板。但对话原生Nodes在重建时出现empty、root pointer及text bounds竞争，旧读取器重建导致UNKNOWN/空层；一次模拟器画面核对实际为黑，不能只据分类或首面板日志宣布修好。原生发布持续增长、anchor拒绝0，表明此处不是旧world身份故障。

文字读取增加有限恢复：早期root/vtable/parent/string header无效时先复核对应的源存储边界，只有真实读值变化才在既有三次获取预算中重取；稳定错误继续拒绝。well-formed empty Nodes与确认竞争允许最多两次1毫秒退避，普通路径不睡眠。DialogStorageChanged与ledger/registry恢复共用总计两次完整快照，不延后monitor最初观察时间。

77项reader/控制及22项monitor测试通过。新读取器在9x同一实际确认框25秒内取得244次有效mode3，零采样错误；这验证新读取器，未热替换旧owner。`menu9x-prompt-new-reader-probe.json`保留结果。9x正常停止后清理、入口恢复与XR退出0通过，再正常关闭游戏；完整owner显示复测另记。

9y（game41360/创建FILETIME `134365662104404106`、owner34216、XR进程见会话报告，`continuous-stereo-41360-20261016-033031-dd096d43`）在正式owner中加载新文字读取器，再次后台进入退出确认框。45秒状态观察覆盖Pause→确认框，未采到零层、无观察器错误，正式monitor未出现读取异常或UNKNOWN；一次原子发布权限冲突在下一采样恢复，未过期撤层。一次实际模拟器画面核对，两眼为相同完整确认框。随后精确认证对话发送一次Escape取消，返回Pause，再Resume成功恢复新票据projection；未退出主菜单。证据 `menu9y-prompt-layer-observation.json`、`menu9y-cancel-return.json`、`menu9y-resume.json`。本结论仅覆盖这个退出确认框及取消路径，不等于主菜单或world重建已修复。

## Native23 配套观察器：检查点确认框及读取竞争

实际会话 `continuous-stereo-23004-20261016-094123-cc2e8cf9` 暴露检查点确认框仍被分类UNKNOWN。它的title/message为精确的 `ConfirmCheckPointLoad_Title` / `ConfirmCheckPointLoad` 字符串引用，active stack有认证的 `DeltaPause-Campaign-Host` 祖先，top为 `WarfareConfirmation / UIScene_Confirmation / input_mode1`。此前只认证Surrender组合，故Pause后UNKNOWN同时禁止eye-menu与projection，形成零层路径。outage的23.312秒仅指没有projection，包含1284次成功共同菜单quad，不能全称黑屏。

修复前会话 `continuous-stereo-24152-20261016-100224-44dbac6d` 保持该确认框15秒：`checkpoint-dialog-baseline-layers.json` 全部采样UNKNOWN/layer0，完整快照保存在 `checkpoint-dialog-baseline-full-sample.json`。分类器现仅额外接受该精确title/message整对，保留Pause祖先、active_top及共同守卫；混配、陌生文本仍拒绝，不新增输入许可。

仅分类修复后的会话 `continuous-stereo-38096-20261016-100609-ee8914db` 虽能显示确认框，15秒窗口仍在9.375–13.391秒采到约4.016秒零层。menu日志证明第二次SnapshotChanged被monitor当作reader损坏，关闭reader并退避1秒；重建含Resolver.prepare与全注册表client扫描，下一授权观察时间已过期（超过原1.5秒TTL）。孤立PermissionError延后一次发布，但没有证据认定冲突句柄来源。

monitor现在仅对exact `SnapshotChanged` / `DialogStorageChanged` 保留reader，仍立即发布UNKNOWN，下一正常poll全新采样；不增加立即重试、不更新慢采样的起始时间、不缓存授权或放宽TTL。身份、结构、I/O等其他失败仍关闭reader并退避。33项分类/生命周期测试通过，连同runner/world-monitor共96项通过。

完整修复后会话 `continuous-stereo-32840-20261016-101242-4f64a20c` 的 `checkpoint-dialog-readerfix-layers.json` 覆盖45秒：采样持续mode3、授权新鲜、layer1，无读取错误；正式monitor无UNKNOWN或发布失败。GPU请求snapshot1/ticket1069、保存QPC56900993914000在确认框保持期间，源图两眼均能读到Load Checkpoint正文；预览 `checkpoint-dialog-readerfix-source-preview.png`。这张是源图，非最终头显像素验收；当时模拟器窗口预览仅1718×17，MCP屏幕截图不适合布局验收。取消后恢复新projection至少2秒，再正常停止。

随后会话 `continuous-stereo-37000-20261016-102558-1782c0e6` 真正确认重载：menu保持Pause→确认框→GAMEPLAY，未观察到中间UNKNOWN；自动world1→2、准星证书提交1次，新Pawn完成步枪→手枪开镜→原步枪，有限准星应用97次（全会话累计）。证据 `checkpoint-dialog-reload-events.json` 及会话menu/world日志。以上各轮均先验证native清理、43入口恢复和XR shutdown，再身份校验WM_CLOSE正常退出，无主动焦点操作。没有把投影缺席当作全部零层，也没有宣称所有加载、过场、菜单或长期游玩黑屏已解决；默认玩家清单未提升，Native23/MainMenu8冻结二进制未改动。
