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
