# 手枪准星与 UI 批次容量

## 55° HUD 候选（2026-10-16）

源码的共同 HUD 平面从 70° 调小为 55°，深度保持 2 米；批次与 Material 路径继续使用同一默认参数，换枪及字幕沿用现有映射，准星方向校正保持独立。平面中心与双眼视差保持不变，宽高约为原来的 74.35%，使边缘 HUD 向中心收拢；状态日志从实际默认参数输出角度和深度。几何测试核对非对称双眼投影中的中心、收缩比例和边界；这是待 Native13 验证的源码候选，不代表已取得游戏图像或头显可读性验收。正在进行的冻结 Native12 运行仍使用原 70° 配置。

Native13已独立编译，DLL SHA256 `f6f12ccf16015470bf7e0b19b7593fec7e8da606bd1a9a482309798d5945cd21`；UiHudPlanTest、FinalTransferPolicyTest、FrameLinkTest、UiPayloadReceiptsTest、UiObserverTest、GraphicsMenuLockTest、WorldRebindMetadataTest通过，记录在ignored `analysis/runtime/world13-unit-tests.json`。实际游戏与头显显示尚待验证，默认清单未提升。

随后实际游戏会话 `continuous-stereo-42972-20261016-064504-bcc8002c.json` 使用Native13与MainMenu8；运行时状态确认55°、2米，planned/applied均2721、sequence_fault0。4K同世界暂停恢复和尺寸变化后恢复各10秒采样均有持续新图像及projection，正常停止/入口恢复通过。自动GPU快照3840×2160显示两眼暂停页面，不是游玩HUD可读性证据；准星有限深度、换枪和字幕布局仍未由此通过，真头显大小与舒适性待确认。

### 有限深度准星的下一证据入口

对已有KB脚本的有界复核找到 `WarPC.UpdateHUDActions`：由PlayerTick分支调用，使用原有相机方向/起点执行CalcWeaponFire，并保存LastCrosshairHitLocation。它是非开火瞄准查询候选，优于缓存上一枪命中；但无HUD、特定ActiveAction和烟雾等分支会提前返回，不能凭字段非零或PlayerTick返回推断新鲜。DrawWeaponCrosshair/GOW3自身仍按屏幕中心绘制，没有证明使用此世界坐标。

后续应先观察原查询的实际执行、祖先调用范围和当前world/PC/Pawn/weapon身份，再核对与渲染ticket的关联；不额外调用Trace/GetAdjustedAim、不复用旧命中冒充持续瞄准。共享VM hook会被内联nested脚本绕过，新增函数名单不等于获得返回拦截。上述仅是已有脚本关系，不是新进程运行证据或有限深度准星实现。

### Native13 后台开镜及检查点回归

会话 `continuous-stereo-28908-20261016-065416-4d7e13eb.json`，1080p，同一Native13/MainMenu8。显式后台动作先校验进程PID/FILETIME与已启用手柄补丁，保持原有虚拟控制器；没有Win+0或焦点激活。手枪身份复读及ADS FOV45均确认，四步菜单状态认证重载检查点后Pawn/weapon身份更换，重载后手枪开镜GPU ticket2209仍见两眼场景及准星。最终正常停止passed=true，native清理/入口恢复及XR退出完成；WM_CLOSE关闭游戏，输入服务正常停止。

HUD55°应用6021次、sequence_fault0，准星应用101次、三类拒绝0；批次容量拒绝/poison0，kind2 Canvas查询1945次，原生文字source1发布176次。这些计数不等于换枪或字幕像素验收。此次请求的selector快照实际未见完整换枪图标组，重载后的快照未见字幕；ADS快照中弹药图标与数值布局仍有可疑分离，需与原版/关闭HUD变换对照，尚不认定为新引入的问题或通过全部UI验收。证据 `world13-ui-scenario.stdout.txt` 与同会话GPU原图均留在ignored analysis。

对照准备还修正了源码开关不一致：Material路径现在与indexed batch共同受ui_hud_plane_enabled控制；关闭时已认证kind2/3使用普通双眼overlay，kind0始终ordinary，kind1/陌生值仍拒绝，原有证书与原调用次数保持限定。GameUiMaterialTest的两态分类及原生命周期/恢复检查通过；冻结Native13 DLL未改变，该变更待Native14实际ON/OFF对照。不能把此开关修正等同弹药布局根因已确定。

### Native14 的实际 ON/OFF 对照与像素澄清

Native14 SHA256 `7c6d7461219a579939cf5f2f6973d8a8b6f0348724383a2bbc40dd89f035cd40`，XR仍MainMenu8。ON会话 `continuous-stereo-34228-20261016-070437-60973b2a`、OFF会话 `continuous-stereo-43844-20261016-070612-76ad6dd0`；同1080p、手枪、非开镜FOV80/开镜45，每态3秒。原尺寸GPU快照时间均在对应动作内，身份/FOV复读与正常退出清理通过，汇总 `world14-hud-ab-report.json`。全程后台输入，之后正常关闭两个游戏实例及输入服务。

开关确实同时作用两路：ON Material hud_draws492、ordinary0；OFF ordinary300、hud0。ON的非开镜与开镜面板均缩小内移，OFF两态均显示完整相邻的054和图标。原尺寸最近邻裁切进一步确认ON的数字仍在小面板旁，不能把960像素预览的难以辨认误报为删除/裁切或Native14分离。已记录HUD viewport451.14×268.56相对普通单眼960×1080约为水平47%、垂直25%，足以解释文字压成少量像素；55°在当前低分辨率路径的可读性仍需调整或真头显确认。此对照没有证明Native13所有瞬态分离均不存在，也没有验收所有换枪/字幕。默认清单未提升，不以关闭HUD变换替代用户要求的缩小、集中与可读布局。

### 可调角宽源码候选

后续Native15源码默认65°，相比原70°温和缩小，深度保持2米。单一运行参数同时用于batch、Material及日志；显式配置 `hud_horizontal_fov_degrees` 仅在清单声明 `hud_size_control=true` 且HUD平面启用时接受，范围45–100°。旧配置和旧清单继续原调用链，不静默丢弃用户给旧DLL的显式设置。launcher→runner→start_capture→INI已贯通；native有界十进制解析拒绝非法值后才允许激活。102项Python与几何/解析测试通过，记录 `hud-size-plan-validation.json`；冻结13/14 DLL不变。此项仍需新构建实际显示验证，启动器图形界面尚无调整控件，不作头显舒适度结论。

随后Native15 SHA `e96010fc71c884125bae3369e16bd8c0028e0145a64387045fb8ef79047d8a32`、MainMenu8实际会话 `continuous-stereo-29616-20261016-072231-bf8e7497` 验证显式65.5°传入；native状态确认为65.5而非截断整数，HUD batch applied1938、sequence_fault0，Material hud_draws145、failed0。后台手枪非开镜/开镜采样、正常停止及入口恢复通过，之后WM_CLOSE游戏、停止输入服务。仍须真头显确认舒适度，不能仅靠计数宣布可读性完成。

启动器GUI源码也已增加同一角宽控件，仅默认清单声明能力时显示；旧清单保持旧界面、不新增字段，保存保留其他配置。编译及隐藏自测覆盖能力、数值、配置保存/读回和旧行为；未显示GUI窗口。默认发布清单未提升，因此旧默认DLL尚不支持该控件，不将私有15构建测试算作玩家发布完成。

### 当前进程的瞄准赋值证书

新增 `crosshair_assignment_config.py` 从调用者准备的只读Resolver重新解析当前PC/Pawn/weapon、赋值函数、局部字段和类继承，核验精确赋值tokens、GNatives槽、execLet原入口及Vector复制分支；不调用游戏函数或复用历史heap地址。Native15/game29616实际暂停期间构建成功：48个身份、writer脚本5799字节、WarPC.PlayerTick及PlayerDriving.PlayerTick的结构均核验，证书保存在ignored `world15-crosshair-fresh-certificate.json`，带PID/创建时间、会话与来源hash；游戏现已退出，不能复用这份地址授权新进程。

配套INI serializer与 `crosshair_assignment_config_loader.h` 已加入，拥有存储的copy/move重新绑定指针；严格解析失败不改输出。Python相关23项及C++ loader测试通过（CMake目标 `CrosshairAssignmentConfigLoaderTest`，使用Python生成的模拟INI）。这仍只是启动证据；未安装赋值Hook、未证明PlayerTick实际覆盖或assignment→tick→scene时序，未把缓存命中用于当前准星。下一步在现有VM owner内验证tick范围并观察原赋值，保留原调用与返回值，再独立核对同帧source-view关联。

## 复现：GameRecovery7，2026-10-15

实际游戏 PID24112（`continuous-stereo-24112-20261015-195606-390abba0.*`）通过对象/类身份与双向关系复读确认从 `Weap_AssaultRifle` 切到 `Weap_COGPistol`。两次正常手柄开镜各3秒，首轮采样的相机FOV由80变为45。没有修改游戏武器或相机内存。游戏EXE SHA256沿用清单中的 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。

- Canvas观察到303次kind1准星绘制；GPU准星分类与校正均为0。
- `ui_payload_store`：capacity_rejected=1、groups_poisoned=16、disabled=1；批次表满后永久停止接纳，后续语义标签丢失。
- 首轮开镜保存GPU原始图ticket368，1920×1080，两眼准星均落在各自矩形中心。双眼使用非对称FOV，矩形中心不代表同一空间方向。
- XR提交1822次、复制896张、跨度20.233秒、最大间隔24.269毫秒。原生清理、42处入口恢复及XR退出完成，游戏正常关闭。该短段不能复验检查点/CG或真头显。

原始图和脚本保存在ignored的`analysis/runtime/`；不得加入发布包。旧样本47092的矩阵审计显示，方向修正后的双眼中心射线误差约0.000000214°，而未修正矩形中心约相差29.44567°；它是几何校验，不能替代本次手枪图像或有限距离命中验证。

## 修复：GameRecovery8

UI批次表容量从16组增到64组（顶点副本仍按需分配，每组最多1024顶点）。表满时只拒绝尚未入表的新payload，不驱逐已有记录，不自动reset，也不使整表永久停用。稍后接纳的非零起始范围把漏记前缀保留为unknown；消费者仍逐索引验证归属、原始顶点字节、原调用绑定及当前场景票据，不宣称拥有整个payload的语义。

已有group发生metadata冲突、回退/重叠、单组容量或分配失败时，仍保留poison tombstone直到消费/显式退休边界；generation耗尽仍永久拒绝。多atlas文字的原有整次撤销逻辑不变。没有新增hook，也没有启用有限距离瞄准缓存。

15项检查通过，覆盖满表保留原组、持续追加、空槽释放后恢复、漏记前缀/混合索引不获得纯标签，以及原有HUD、准星、索引、Material、FrameLink和DLL生命周期。扩大容量使准星测试中多个内联局部Store触发栈溢出，测试fixture改为堆分配后通过；生产Store为静态实例，没有改变游戏线程栈上的分配。首次失败记录保留在`game-recovery8-validation-pre-fixture-fix.json`。

冻结DLL SHA256：`f5cb96399eb5e6ca7d0932cb5a761d9892afc984fd06a69f48a29f95cd942558`；清单`manifest-game-recovery8.json`，XR继续使用SpaceRecovery3。

## 同操作复测与当前入口

GameRecovery8 / PID29060（`continuous-stereo-29060-20261015-200432-eee3ef86.*`）再次通过身份复读确认COG手枪及开镜FOV45，两次各3秒。批次published=7008、claimed=3302，容量拒绝/poison/disabled均0。准星considered/planned/applied均254，center/geometry/projection拒绝均0；HUD面板应用2147次、sequence_fault=0，原生字幕索引绘制65次。字幕计数不等于本轮已目视验收字幕。

开镜ticket376的1920×1080 GPU图显示准星已离开两眼矩形中心，位于各眼对应空间方向；右上弹药HUD恢复为较小的面板布局。三个矩阵齐全的实际准星样本，中心射线夹角约0.000000413°（原矩形中心约29.44567°）。只证明无限远方向修正与实际图像位置，不宣称有限距离命中、所有武器或真头显融合已验收。

XR提交1864次、复制868张、跨度20.680秒、最大间隔22.159毫秒。原生清理、42处入口恢复及XR退出完成，正常关闭游戏。report仍为incomplete，不借短段结果宣布CG/加载黑屏修复。早先PID17932在只读菜单复读变化时测试主动中止，未执行切枪；脚本现仅对这类临时读竞争重试，不重复输入操作。错误日志保留。

玩家`manifest.json`提升为Recovery8；Recovery7与更早冻结清单保留回滚。源码、清单与结论提交Git；测试图像、对象地址和日志仅留在ignored的analysis。下一步确认换枪菜单和字幕在持续运行/重载后仍保持UI布局，并继续有限距离瞄准与CG/加载显示缺口。

## 重载、换枪及字幕图像复核

同一冻结Recovery8 / PID11748（`continuous-stereo-11748-20261015-200929-b0879391.*`）完成手枪开镜、换枪界面、四步状态认证的检查点重载、再次选择手枪开镜。复读确认重载后的Pawn及武器对象更换；旧调用语义不重新标到新对象。

- 准星应用148次，三类拒绝均0；重载后的ticket2610/2650/2694仍有双眼方向校正记录，ticket2610同时保存GPU原图。
- 批次published=18653、claimed=8327，容量拒绝、poison、disabled均0；当前Pawn重关联1392次。2次结束绑定拒绝、3次时间戳竞态保持原有限定，不影响后续恢复。
- HUD面板应用6982次、sequence_fault=0；kind2换枪绘制972次。ticket819图像两眼均见中心区域换枪图标/文字。
- 原生字幕绘制442次；重载后ticket2610图像两眼均可读到同一句英文对白。只验收此样本，不推广为所有字幕/语言/菜单均完成。
- XR提交6403次、复制2903张，跨度71.112秒、最大提交间隔22.303毫秒；并非新游戏图像每22毫秒必然更新。原生清理、42入口恢复和XR销毁完成，游戏及本次输入服务正常关闭。

GPU审阅图`ui-recovery8-selector-reload-review.png`、1:1字幕裁切`ui-recovery8-subtitle-review.png`、汇总`reticle-ui-recovery8-audit.json`均在ignored的`analysis/runtime/`。HUD当前恢复原有70°/2米配置，尚未另行调小参数或改为世界固定；本次没有真头显舒适度验收。

## 退出报告判定修正

上述短段虽然清理成功且XR exit=0，旧协调脚本仍给出incomplete。核对`continuous_passed`发现它要求启动接续`one_way=true`；启用菜单恢复的XR正确报告`one_way=false`，因此被旧条件误拒绝。这是报告判定错误，与实际短黑帧分别处理。

脚本现保存启动时`menu_recovery_enabled`，按配置要求对应的接续模式。可逆模式还必须有实际`menu_recovery_evidence=true`及`panel_counts_as_stereo=false`；真正提交双目、成功xrEndFrame、owner身份、原生恢复及XR正常退出条件均保留。39项runner测试通过，含模式错配、证据缺失、把平面算立体、结构失败和退出失败的拒绝用例。

三个原报告保持原样。离线只重放退出谓词，并从各自原始`xr_command`的`--menu-state-file`恢复启动配置，结果均通过；审计文件保存原报告SHA256。这不把旧报告冒充新运行，也不消除已测得的加载间隔或CG风险。新的启动配置记录与谓词已由测试验证，尚未再为这一纯报告修正重启游戏。

## Native16：准星实际赋值观察器（尚未取得运行验收）

新增默认编译关闭、运行显式启用的观察器。启动时以当前进程重新解析控制器、Pawn、武器、脚本及两个 PlayerTick 根函数；仅在已认证 Tick 的调用链内观察既有 execLet 赋值，原函数调用一次并保留返回值，不额外调用瞄准查询、不修改准星坐标。证据按线程和世界代次封存，后续场景入口仅消费一次；这只检验调用时序，不证明相机等价或渲染 ticket 关联。

Native16 DLL SHA256：`2d57625433b04799636a94df5254c2921b137503935bcb59c1915103be827b43`。赋值状态机、配置加载、帧关联、UI 与恢复相关原生测试及三个 CaptureSelfTest 模式通过；启动器43、持续会话43、启动配置21项 Python 测试通过。观察开销以 QPC 记录，单观察段超过5ms时停止诊断，不能视为纯 CPU 时间或整 Tick 总开销上限。

2026-10-16 的 `continuous-stereo-40604-20261016-075634-79fc35f0` 在原生观察器激活前收到取消信号：`native_started=false`，XR orderly stop / shutdown_completed=true。未捕获实际准星赋值，未改变玩家默认清单，未宣布有限距离准星修复。下一步仍是取得实际执行及场景关联证据，再决定显示修改。

## Native17：首次捕获实际准星赋值与场景时序

Native16 实际激活失败：新增 execLet Hook 后，原生会话入口表仍只有42槽，第二个 resize Hook 安装时耗尽。Native17 按编译开关增加1槽（此构建43槽），并在容量耗尽时输出明确计数。独立源码核对确认无需改变 viewport 表或动态入口清单格式。

Native17 SHA256：`60a9aa9c3ec6b9c51995eb2ab9b47900a141e2201e905b18bd0766caee5b9838`。实际会话 `continuous-stereo-2796-20261016-080250-cc68bb34`：roots/candidates/completed/sealed/scene_consumed 均644，root/snapshot/completion/scene-binding拒绝均0；场景调用658。已保存32条有界时序记录。观察段累计QPC约710.69ms，最长单段0.6896ms，5ms单段预算未触发；这不是整帧性能验收。当前仅证明本轮实际 Tick 中的赋值和同线程后续场景消费，不证明相机等价、渲染ticket归属或所有武器/重载后的覆盖。

43处入口清单封存成功。原生停止/资源清理通过；旧独立校验器缺少 execLet 已知运行代码签名，误把该原生入口交给系统模块校验而拒绝，原报告保留 incomplete。补充固定历史运行签名后，对仍存活的同一游戏只读复核43处入口，`world17-independent-restored.json` 的 all_match=true。随后正常关闭游戏，输入服务停止；没有把重跑校验冒充新的完整会话通过。

XR短段提交678次、复制284张、跨度7.5014495秒，最大提交间隔22.4994ms。未进行真头显验收。准星显示仍是既有无限远方向方式；下一步是在精确场景/源视图/ticket内携带赋值证据，并实测写入相机和源视图关系，之后再实现有限距离准星。
