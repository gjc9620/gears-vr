# Troika 固定机枪：交互需求与开发验收

日期：2026-11-09。用户已确认并授权开发。最新进入方式：空手触碰真实机枪柄即请求进入；尺寸、角度与接触范围由本会话自行标定。配套[公共合同](common.md)。

## 1. 操作目标

空手靠近原炮位并触碰真实机枪柄即可请求进入，无须额外按钮或先按Grip；原占用确认后，双手持续Grip抓两个握把，绕固定炮塔枢轴瞄准，右扳机开火。退出沿现有原主线操作，恢复自由手和正常身体／视点。

**Troika不是可随身拿走的枪，不进肩槽，不用手动换匣。**

## 2. 玩家操作步骤

| 阶段 | 玩家动作 | 必须结果 |
| --- | --- | --- |
| 接近 | 空手正常走到炮位操作位置 | 仅靠近不进入，必须触碰实际机枪柄；保留原占用和人物状态条件 |
| 进入 | 空手触碰真实机枪柄，不按额外进入键或Grip | 一次接触沿请求原ClaimTurret／DriverEnter；成功才进入操作，失败不建立VR占用，不逐帧重复请求 |
| 抓握 | 左右手分别对准实际握把并持续按LGrip／RGrip | 两手取得各自当前抓握，模型接触合理；不沿普通枪点按保持 |
| 瞄准 | 双握移动，围绕固定枢轴改变方向 | 原父子层级带动炮塔，遵守原俯仰／水平限位；头显自由观察 |
| 开火 | 双握有效时按住R2 | 原预热／射击及双枪口行为；不新增单手模式 |
| 暂停射击 | 松R2或任一Grip释放／失追 | 停止拥有的原开火并解除手部驱动；仍处炮位的状态与真正退出区分 |
| 再抓再射 | 重新抓双握把，R2先松再新按 | 不因重新抓住而自动开火，不补发失效期间射击 |
| 退出 | 使用原主线炮位退出动作 | 原DriverLeave／UnclaimTurret完成，双手自由，视点／移动正常，无炮位占用和射击残留 |

## 3. 原自动流程

原CanReload=false，但自动Reloading存在，可能有粒子、计时及Loader差异。保留原机制和实际实例时长；不能看到状态名就加弹匣，也不能未经证明改成自定义热量条。

双枪口轮流或同时发射以原InstantFire／WeaponFired为准；VR只接对应方向与合法原起点，不自造一条中央枪口代替。

## 4. 反馈与身体

握把／炮位提示能区分可进入、被占用、已进入未双握、双握可射与原恢复中。黄色只提示当前握把或步骤；不能把世界炮位自动吸到玩家手边。

固定枢轴和限位来自实际炮位，双手不一致时不能拉长炮身／臂骨来拟合。炮位相机与HMD观察分开，退出后不保留强制朝向。炮位占用期间不同时分派肩取枪或掩体抓边；相关输入由当前上下文唯一拥有。

## 5. 必须验收

1. 空手触柄无按键进入；只靠近、持物触碰、失追跳变不进入。占用失败／已有其他占用不绑手，同次接触不重复请求；离开接触区后才能形成新的进入接触沿。
2. 左右握把正确识别，单双手缺失不能持续开火；普通枪RGrip规则不被破坏。
3. 固定枢轴、上下左右限位、仅转头不带炮口；实际枪口与射线一致。
4. 原预热、真实双枪口射击和自动恢复；Loader存在／不存在分别保留原逻辑。
5. 持R2松任一Grip立即停止拥有的开火；再抓不自动射，松R2重按有效。
6. 原退出、失追、暂停／剧情、死亡／读档后，手部／炮位／相机和射击无残留。
7. 双眼检查握把接触、双手姿态、炮体／身体遮挡；循环音和伤害停止分别确认。

## 6. 待核对与实施入口

进入已由用户定为“空手摸机枪柄”，实现以真实空手、手柄几何接触与原炮位准入为条件，不等待额外键位确认。接触范围、角度、枢轴与握把尺寸由本会话自行考虑／标定；不得用旧交互按钮作为默认进入前置。退出保留现有原主线映射，由本会话查清并记录；无常规换弹，不新增四秒流程。

复用 weapon_troika_interaction.h、原Turret_TroikaCabal／Troika_base的Claim／Unclaim／DriverEnter／DriverLeave／UpdateTurret及现有单写入后端。待核实际握把轴、枢轴、限位、双枪口、Loader和原恢复语义。固定机枪与Junker载具枪位分开，不以其历史通过替代。

来源：[W10](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/weapon-cards.md#w10)、[原炮位入口](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/original-entry-map.md)、[跨模块互斥](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/offline-tech-prep-20261028/shared-contracts.md)。



## 2026-11-09 实施检查点（未完成运行接线）

- 唯一工作区C盘6dof-single、分支codex/6dof-single；保留其他会话未提交改动，无新分支/worktree/助手，未操作共享Git暂存区或HEAD。
- TroikaInteraction进入改为空手触柄接触沿：无Grip/interact前置，同次接触只尝试一次；占用拒绝、持物、失追/连续性缺口后必须离开柄区再接触。原确认前hands保持await_tracking。左右柄分别要求有效接触处的新持续Grip；松手、失追或暂停恢复后旧Grip不能自动重获，R2必须真实松开再新按。
- 保留原动画交接，新增有实际原占用/原输入恢复证据的同步进入/退出回执；不虚构动画serial。新原适配weapon_troika_original.inl只准备CanEnterVehicle/TryToDrive，不独立重复ClaimTurret/DriverEnter；当前身份、原空手/仲裁、100ms请求期限、类型化反射参数与Driver/DrivenVehicle/PC.Pawn读回均检查。每个binding/token至多派发一次，未知结果保留责任；旧owner收尾不写新占用者。
- 实际MSVC /W4 /WX /UNDEBUG编译执行TroikaInteraction 528项、固定枢轴solver 68项合成检查通过。solver只选择性复用原离线单文件；未整体覆盖冻结树。原枢轴轴向/角限位仍需实际实例认证，合成检查不是原游戏标定。
- 完整当前组合源码私有冻结Playground生产目标编译链接成功；原适配已进入编译，**尚无Troika tick调用，不能发起真实进入**。初次Troika通用头误放anonymous namespace产生编译失败，已移至cpp文件头并保留日志；并发Gnasher/Bow问题由其所属者修正，只更新私有编译副本。该组合DLL尚非选定、可运行或交付候选。
- 复核原TroikaTurret导出SHA7baac05a…adc037、13骨及原网格GLB SHA db8a4761…66836。左右handle骨原点到最近网格顶点分别约5.880/5.261原始单位，不可直接把骨原点当柄面；尚未换算物理单位或宣称皮肤贴合。
- 下一步仍属本枪实现：真实柄面/原始掌位置接触、独立mounted上下文（controlled WarTurret与human Driver分开，不伪造普通WarPawn Frame）、双侧逐柄/触柄/扳机GrabPose及唯一C0消费、原枢轴限位瞄准与双枪口R2/停火/自动恢复。左右手/袖口实际画面未验收。
- 未stage、未启动游戏/XR/录像、未申请实际队列槽，未改公共v71；源码、编译、游戏、双眼、头显和交付分开。现有唯一队列按已确认前驱链交接，完整候选冻结后才向当时真实尾申请，不凭idle启动。
- 记录与本枪源码补丁：analysis/troika-development-20261109/checkpoint.json、troika-owned-changes.patch、source-inventory.json、各原失败/最终构建日志。当前状态为实施中，不是Troika完成或可玩交付。


后续同日补充：新增weapon_troika_mounted_context.inl只读原占用观察，明确区分controlled炮塔、Driver人物、双Mesh和原Troika武器；检查PC.Pawn/Turret.Controller/Driver/DrivenVehicle、人物InvManager归属、Camera.PCOwner/ViewTarget.Target、world与Weapon.Instigator等关联。Mesh按ComponentProperty认证；Camera.ViewTarget按TViewTarget结构及Target成员认证，未按ObjectProperty误读。重复关联/元数据关闭检查不等于原子快照，尚无live接受证明。该模块和原进入适配均再次全PG编译链接成功；未启用tick/动作/C0或刷新输入时间。最新DLL SHA和精确源码清单见checkpoint.json。共享cpp新增其他武器include导致私有冻结依赖缺失/版本不齐的构建失败保留；仅刷新私有依赖，没有替他人修改源码。下一步仍是mounted共享追踪/手部权限与唯一C0实接，不是普通排队阻塞。


## 2026-11-09 持续目标推进：真实柄面与原矩阵读取（仍未实跑）

- 上一目标轮归类progress，当前继续完整Troika范围，未缩成单次进入完成。接触沿内核提取TroikaEntryContact，由原TroikaInteraction复用；后续未占炮位入口可使用同一去重规则，不必先认证开火射线才观察空手触柄。
- 新weapon_troika_handle_geometry.h基于原TroikaTurret LOD0的6段稀疏握柄截面，使用圆角矩形包络（原顶点显示宽角，不使用会漏掉原角点的椭圆），只在真实柄轴段内、距表面1.2cm以内接触。handleroot9参考原点(0,47.27169418,111.43261719)作为基准，不用偏离柄面的handle10/11骨原点；当前位置由原GetBoneMatrix提供，严格拒绝非有限/剪切/镜像/非均匀缩放。单位用当前同源rig的units_per_metre和原矩阵真实缩放；这仍为待实跑标定的表面近似，不宣称精确三角接触或皮肤贴合。
- 新weapon_troika_parts.inl在普通GT认证本世界Troika、原Mesh/SkeletalMesh资产路径Geist_TroikaCabal.TroikaTurret、可见/存活条件，并用原MatchRefBone逐一核handleroot/handle_L/handle_R索引9/10/11后取原handleroot矩阵。读取前后保留原身份/元数据/100ms source-current检查；不读吸附皮肤手，不新增writer。
- 新几何/接触沿实际顶点与反例19项通过，改动后的原状态机528项继续通过；全Playground生产目标再次编译链接成功。仅向既有成功私有源码冻结叠加本枪精确文件/hunk，本轮没有追随刷新其他会话整份mutable cpp。自身局部observed变量遮蔽全局的WX编译失败已改名修复并留原日志。
- 本轮仍未启用Troika tick、未stage/run/占队列，无新的游戏/双眼/头显证据。下一步实际空手接触生产者→原进入调用，再继续mounted输入权限/双柄C0与逐部位手型、原瞄准/双枪口/停火及完整游戏/双眼检查。目标保持active。


## 2026-11-09 实时进入生产者与所属者生命周期（已编译，未启用）

- 新weapon_troika_entry_runtime.inl已进入根源码include并在独立冻结生产目标完整编译链接。原炮位分批发现仅记录当前注册对象/本世界身份，单批最多512对象及750微秒发现预算；接触前另核真实部件与同源raw rig，追踪/来源/参考/epoch/cohort变化重置接触沿。持物提前退出；几何未知不当作区外；同帧多个炮位触碰拒绝，不选任意一个。
- 实际触柄沿生成一次原进入请求；相同帧身份与原控制器sync时间+100ms到期用于原适配，不伪造输入时间。CanEnterVehicle拒绝后仍需离开再碰；所属者保留binding/token高水位，未知/延迟TryToDrive结果只读观察而不重派。
- 原进入回执现在保留原调用完成和原返回接受分别记账，并保存原FrameKey。原DriverLeave只在正常GT、精确旧所属者仍实际占用时可发一次；调用未知不循环重试。原退出后只读确认恢复；新世界或新人物生命周期且原Driver关联已明确改变时，只退役旧mount记录、不写旧/新人物或新占用者。未来新增本枪开火账本必须先单独结清，不能借mount退役抹去射击责任。
- stop取消在原drive派发前再次核对；其他控制者tick不驱动本枪旧炮位。PostPlayerTick独立分派、停用drain guard和本枪构建开关尚未写入共享入口，已向实际共享编辑者提出极小hunk协调；当前configured仍FALSE，无实时调用。
- 生产模块首次near函数名命中Windows near宏的编译失败已改名within_reach并保留。一次工具调用中断后先核日志/进程，确认MSBuild/cl/cmake均已无活句柄才恢复构建；最终完整PG链接0。最新DLL与源码SHA见checkpoint.json。本轮未stage/run/XR/录像，无游戏/双眼/头显通过；公共v71与Git index/HEAD未改。
- 下一步：接上述共享tick/stop边界及冻结入口定向候选，同时继续mounted追踪/双柄权限、原枢轴/双枪口/停火、逐柄触柄扳机GrabPose与唯一C0消费；完整范围保持，目标active。


## 2026-11-09 私有入口分派/停用补丁已开启编译（未合共享hunk，未运行）

- 将共享接线做成具体shared-entry-stop-review.patch，只在已冻结私有源码实施：PostPlayerTick调用本枪tick，炮位所属上下文返回TRUE时跳过本tick普通装备/火控/跟随消费，已有部件/视觉恢复drain仍在外层；WeaponHandStop与Shutdown各有独立Troika drain guard，位于Native手部断开之前，原其他所属者清理谓词不改。
- 本枪GEARS_PLAYGROUND_TROIKA_INTERACTION默认OFF，开启需原Native participant/follow与rig reader；私有ON配置完整PG编译链接成功（entry-on-build.log）。根CMake/tick/stop尚未应用该补丁，已向实际共享编辑者发送精确可审阅hunk进行协调；根目前只有本枪独立include/模块源码，不依赖未确认的共享入口。
- 本枪drain请求停用后最多等待2秒，未结清继续draining，不跳过责任。默认OFF且无本枪责任时立即返回TRUE，不对其他枪引入额外等待。原停止位先读取client当前原子registration cookie；修正“首帧之前last_cookie=0会把同一次注册误认为新连接”的边界，仍只允许真实新注册恢复。
- 已将无rig reader的非本枪构成明确返回unavailable，未增加备用输入源/时间戳/driver。有限原回执按本次尝试记录token，不把前一所属者的回执冒充当前派发结果。
- 最新私有制品、源SHA及补丁记入checkpoint/source-inventory；未stage/run、无新游戏/双眼/头显证据，公共v71/共享index/HEAD不改。继续完整mounted双柄、原瞄准/双枪口/停火、逐部件GrabPose和唯一C0路线，目标active。


## 2026-11-09 共享入口已确认合入、当前整合源码编译通过

- Lancer明确允许独立PG/CMake hunk并转核相邻HOD stop入口；HOD随后明确两个LifecycleScope插入点无在写冲突。已逐hunk合入根playground.cpp与CMakeLists，不覆盖整文件，原HOD和其余枪owned/stop/restore条件保留。实际根改动为shared-entry-stop-applied.patch，而不再只是私有提案。
- 新integrated-entry/source是本次当前整合源码的独立冻结，完整生产目标在GEARS_PLAYGROUND_TROIKA_INTERACTION=ON时编译链接0；原根默认OFF。精确制品SHA/源清单在checkpoint及integrated-entry/source-inventory。未stage/run、无游戏或双眼证据。
- 启动依赖只读预选：040742 Native/模拟XR/launcher/simulator Runtime保存文件实际SHA全部与其prepared manifest一致，未复制到活动实例。六个关键hand/rig/controller/stereo/reference/call协议头与当前root逐字节相同且绑定原manifest；仅为入口验证依赖的源兼容证据，不当作全Native功能、当前完整手型或运行通过。entry-build-dependency-candidates.json仍明确not prepared/not staged，测试driver/scope/双路录制依赖尚需冻结后才能向实际队尾申请。
- 新静态依据：Turret.FaceRotation将AimDir赋为Controller.Rotation，GetTurretAimDir返回AimDir；mounted双柄物理空间必须有进入时固定基准，不能以随炮口旋转的相机每帧反馈重建控制手。该项是源码推导的设计约束，没有声称已实跑复现或修复反馈。
- 下一步入口定向driver/完整候选冻结与真实尾交接；排队期间继续mounted同源追踪、固定基准/原限位/双枪口、逐柄触柄扳机GrabPose及唯一C0。目标active，不把原进入或编译称为整枪完成。


## 2026-11-09 入口driver冻结与实际尾路由（未受理/未获窗）

- 新troika_entry_chain.py复用原有受控导航和raw→world规划数学，实际动作仅模拟VR平滑行走/手移动/原左手X；GetEntryLocation/GetBoneMatrix仅只读规划，未调用TryToDrive/DriverEnter/DriverLeave或射击API作为脚本动作。断言：近柄未触不进、空手无Grip/按钮触柄原占用、无Grip/R2无新增枪数、原X退出恢复。首次具体失败停止，无GiveTo/传送/直接进入退出fallback。
- run_troika_entry_check.py和stage_troika_entry.py保留现有run_v63/run_menu、精确HWND双路录制、失败先记身份、原stage/正常收尾/恢复机制；执行必须匹配最终唯一交窗receipt及独立idle，两条prepare-only均实际exit0且不stage/run。旧174639冻结未动未运行；离线复核朝向应看实际turret而非接近零的入口剩余向量后另冻174900。174 driver哈希、原EXE及6制品身份固定在plan/prepared，复用已存在PIL/numpy且导入成功，无安装。
- 当前有效prepared equipment-controls-prepared-20261109-174900-troika-entry，prepared SHA9a356a4a291ce857468e59fa57c9eee1414f43f1f62c3b13673ae9551e9d2690，plan SHA7dfe6890ce13c7c1cb1fb59de6ab927ab4483ca7e39f073be815ed25075d548b；PG50e5eaae/Nativec98568f0。完整hash/path见entry-preparation-receipt.json，不以早期174639消息代替。
- Frag明确旧161511链关闭、172809未被受理；实际已确认路由Lancer165800→HB171233→Boom173042→HODcore2-aim。已按逐方确认向HOD核其已正式受理后继，没有在任何前驱添加第二槽。目前仅路由，Troika尚无直接受理、无最终交窗；entry-runtime-queue.json明确false，不能凭idle运行。
- 路由期间新增weapon_troika_fixed_space.h及定向16项CPU检查通过：进入固定物理基准映射保留两手完整旋转/真实原输入时间、头转不改变手映射，身份/参考/epoch/100ms失效拒绝，缺手返回真实mask。该叶模块尚未接Native mounted来源/相机/C0，不影响已冻结入口候选，不是整个mounted控制通过。
- 完整Troika范围与目标active保持。下一步实际尾直接核验受理/排队，等待时继续mounted同源输入、固定view/原限位/双枪口和每部位手型；未stage/run、未占用/终止任何持有者。


## 2026-11-09 mounted同源数据链检查点（未实跑、入口候选不变）

- 新Native troika_scene_binding独立认证原mounted anchor、炮塔/Driver/双Mesh/原Troika武器/Instigator；普通WarPawn binder仍拒绝炮塔。35项合成身份/部件/退出竞态检查通过，隔离冻结Native生产目标编译链接0。根仅已加入该只读binder include，无普通Frame伪造。
- 新troika_mounted_source独立数据ABI与troika_scene_tracking复用原matching_input选择器和同一continuity bridge；每Scene只选择一次，同Scene失败不能改输入后重选，重复读取不重发publication或刷新原时间。Native真实session/cookie/reference revision、实际producer companion、Scene/输入100ms、失追mask与stop失效保留。36项合成生命周期检查通过。Getter只复制数据并核现有票据，不在读取线程查询actor/桥或获取stereo锁；使用方必须单独核原占用。
- 具体mounted-source-shared-review.patch包含默认OFF的Native构建开关、共享入口早期mounted分支/原invalidate、同一continuity函数模板复用和独立getter/无等待stop；已发送既有共享编辑者核具体hunk。当前仍仅私有冻结副本应用，ON生产Native目标完整编译链接0，根共享prepare/CMake/export尚未合入；不改他人的C0/V4。
- 新PG weapon_troika_tracking在本枪mounted tick读取数据，复用现有handrig模块存活admission，并在getter前后再次认证原占用与各Mesh/weapon；缺导出仅unavailable，不换来源。进入回执保存原物理/模拟source类型，读取必须保持相同类型。PG隔离冻结生产目标编译链接0；尚未接固定基准map/view、双柄资格、瞄准/双枪口R2或唯一C0。原完整范围和双手逐部位手型仍未通过。
- 源码/制品精确SHA、失败/最终日志和配置依赖见checkpoint.json、native-mounted-observer、native-mounted-source、integrated-mounted-reader。最初fixture/编译/配置失败均留存，未通过弱化guard或WX处理。原历史工具源清单不等于全部MSVC输入，补齐的当前配置依赖另有清单。
- 原入口174900 prepared9a356a4a/plan7dfe6890/PG50e5eaae/Nativec98568f0/174driver保持冻结未变。真实先序已纠正为Frag172809→Gnasher173101→Troika174900；Gnasher独立全核正式受理我为唯一后继，仅排位，未最终交窗/未stage/run。随后独立完整核4083保存源/4bin/21driver+support及1791PG/1138Native快照，受理Snub174315为我唯一后继，只排位、不授窗；记录entry-runtime-queue.json。
- 公共v71、共享Git index/HEAD与当前持有者运行区不动。下一步在等待正常最终交窗期间继续mounted同源来源的协调合入、固定物理基准/原限位/双枪口与逐柄手型/C0；目标active，不能把原进入、合成或编译称为整枪完成。


## 2026-11-09 进入固定基准与原部件读取（源码/合成/编译，未运行）

- 新weapon_troika_mounted_space从真实普通rig的base_view/reference、同一Native/注册、原人物Mesh和XR/publisher保存几何基准；在原TryToDrive前准备，只为其实际已派发且未结清的receipt保留。mounted侧只用新鲜原raw来源映射两手全旋转和观察头部，不从随炮口旋转的相机重新捕获。输入/focus/profile恢复保留基准并报告continuity boundary；换原点、参考、publisher、Native或原所属者拒绝。旧publication和同publication改数据不重用；46项CPU检查通过。此数据路径不授Grip/aim/fire/C0。
- 新原mounted_parts读取当前原Troika资产、root/latitude/longitude、左右muzzle与handleroot的真实GetBoneMatrix，逐一MatchRefBone/矩阵验证，并读取原YawLimit、ViewPitchMin/Max、Rotation和MuzzleSocketName。通过已有typed socket getter合同读取实际被选中的socket，再对原GetPhysicalFireStartLoc；不构造中央起点或自行轮换双管。未知原函数/元数据仍unknown，尚非原双管/轴向/限位的运行认证。
- 保存的Core/Engine原导出表证明TViewTarget、Vector2D、Rotator都是ScriptStruct；共享Reader.structure仅接受Class/Function。此前PG camera_target错用该读取器，现以本枪独立ScriptStruct类别、声明者/包、成员布局与关闭复核修正，不扩大共享类读取器。原错误源码/构建记录保留，不能把编译绿灯当该live路径已通过。新增实际生产helper合成85项验证通过，覆盖Target重复/循环、类型/归属/结构头/成员宽度与偏移变化。
- 部件查询用原100ms/source票据，调用原getters前后通过既有Native admitted getter核当前publication/ticket，不读新输入/伪造新时间。有限日志只记录状态及原数值，不是皮肤或硬件GPU证明。完整隔离PG1275文件冻结目标编译链接0；精确SHA/日志/输入清单见checkpoint与integrated-mounted-parts。
- 进入固定基准的PG源码已接；Native fixed view、sole左手仲裁mounted贡献、原瞄准/射击账本以及唯一C0 mounted Driver上下文仍未完成。共享C0 owner当前依赖普通binding/current_actor，已向既有编辑者指出独立上下文接口要求，未复制writer或伪造普通Frame。
- 原174900入口候选/174driver/二进制不变，仍等Gnasher173101全终态最终唯一交窗；未stage/run，未动当前持有者、公共v71或共享index/HEAD。完整目标active，游戏/双眼/头显与整枪仍未验收。


## 2026-11-09 Native真实进入基准已接根（未安装/未运行）

- 已将先前具体三文件mounted-source-shared-review.patch逐hunk合入根Native CMake/shared prepare/capture，重新git apply --check通过，依既有共享编辑者此前无在写冲突的信息继续推进并通知。默认OFF；普通WarPawn binder、C0、V4与其他枪分支未改，未整文件覆盖。
- 新264字节独立troika_entry_basis V1接口复用原handrig模块存活admission与实际Native注册/GT。PG只交原所属者、token和精确rig publication，不交矩阵；Native读取自己的实际当前rig缓存并冻结几何。原进入失败/准确退出取消相同几何请求，旧取消不能清新请求、旧token不能重放；Native stop清数据。Native内部已有同一基准的mounted映射数据，尚无view/C0/action consumer。
- 实际生产方法的合成生命周期检查扩为57项，通过。另先复现内部mapped数据请求错Driver仍返回缓存，补齐所有角色和输入批次核对后通过；这是数据路由修正，不是发现或修复实际游戏/C0权限漏洞的声明。
- 当前根源码1187文件隔离Native生产目标，ON和OFF均编译链接0；隔离PG1276文件目标也0。初次沿用旧配置缺少当前BodyGoal所需cover_context_live reader而失败，已保留日志，只在私有有效配置启用现有GEARS_COVER_CONTEXT_READER，不改他人源码。Native最终ON/OFF与PG精确SHA及当前源码快照见checkpoint/native-entry-basis/integrated-native-basis。
- 下一真实缺口是Native同一broker pin的独立mounted几何prepare/commit、同一participant/左手arbiter的资格和tagged Driver C0上下文；不能把mapped数据当lease，也不能每帧invalidate后声称持续握持。原坐姿应保留在同一C0交易，仅用现有固定骨长手臂/手指求解；原aim/fire还需独立mounted调用边界。该路线已记录为未实施，不扩普通Frame、不借normal rig票据、不加第二writer/hook。
- 174900入口候选、174driver和6制品保持原身份未变；仍等Gnasher最终唯一交窗，无stage/run/游戏/双眼/头显新证据。后来HB/Boom申请只沿Snub已确认的Coverr54实际尾路由，Troika仍只有Snub一个后继。公共v71和共享Git index/HEAD未动，完整目标active。


## 2026-11-09 mounted几何查询接通既有broker（未安装/未运行）

- 新troika_mounted_geometry V1通过既有participant context与唯一模块pin往返：Native实际mounted Source/原进入token → PG固定进入基准映射raw palms/读取原handleroot → 两手分别对原左右两个握把返回contact/distance。Native固定validator在回调前后复核实际Scene、炮塔/Driver/双Mesh/Weapon、缓存ticket/reference/完整批次与原100ms；PG不能替换该validator。普通Frame/WarPawn binder/普通授权不变，数据查询成功不等于Grip/aim/fire/C0。
- 查询不放入原始采样的stereo锁。try_submit_pair退出guard在内层锁释放后用同一实际Scene查询，渲染拒绝也能到达，避免几何依赖已通过的显示结果。重复/失败publication在回调前烧掉，不重跑、不更新时间。明确非ready必须空回包，tracked mask必须与原raw样本一致。
- 204项针对性broker检查和原2433项均0失败；覆盖请求改写/票据变更/超时/错误tracking/NaN/非空denial/重入/SEH/立体锁内拒绝，以及回调阻塞时stop/unregister不提前unpin。原生产mounted source/basis合成生命周期65项通过。所有均为CPU合成证据，PG getter的真实运行和部件/双眼fit未证明。
- Native1191文件当前源码快照，启用/关闭mounted source的最终生产构建均0；PG1278文件独立组合快照最终构建0。精确SHA/最终日志/源清单见checkpoint及native-mounted-geometry、integrated-mounted-geometry。此前初始构建/源清单/合同补强前header与哈希另保留；没有安装或覆盖冻结174900候选。
- 新共享hunk仅broker尾部backend/独立注册/工作区/include/清理、runtime wrapper与capture头/锁外guard/export、PG一个头include。共享负责人确认无broker编辑冲突；本轮生成独立mounted-geometry-shared.patch并反向check0，排除了他人正在做的Lancer PG调度/forward差异，未更改C0/writer或共享index/HEAD。
- 下一步必须把该data-only查询扩成同一pin贯穿prepare→现有Native participant/sole left owner仲裁→PG commit→Native complete的交易，才可授持续左Grip；届时再将每Scene invalidate改成真实边界/丢失处理。右Grip/R2新按、防重放、原aim/fire/action边界、原资源/停止账本、固定view与tagged Driver的同一C0交易及所有部件手型仍未完成。
- GN173101报告48832原参考在启动前置session缺playground字段失败、Native未激活、正常stop后仍在paired/恢复审查；尚无最终交窗。Troika174900仍未stage/run，Snub仍唯一后继。Frag200216新尾申请随后由申请者撤回，本方只沿Snub实际后继路由撤回，未受理/授窗。公共v71及整枪/双眼/头显验收不变，目标active。


## 2026-11-09 首次174900实际18952：目标getter未派发，已闭环交窗

- 独立全核GN最终交窗/check ef7799be/review f2839ba9、8项exact与自己idle后，只运行冻结174900原entry范围，PG50e5eaae/Nativec985/XR1684/runtime6aea。真实game18952/creation134387002721045557到Native running和hand connection，XR记录161 submissions/85 scene copies/3.4975592s。外层Codex review Python3c48fbdc/Pillow13/numpy2.3.5已核，包内7a4102不变。
- 第一目标GetEntryLocation尚未派发就在Bridge.call→Resolver.identity→name因name_pointers未初始化失败。attach_only只认证已加载模块，按其清理用途跳过r.prepare；本测试却要反射getter。motion/states/checks全空，没有导航/接触/原进入/原X退出样本。runtime自身passed只覆盖stereo/lifecycle，不能盖过operator1/本目标passedfalse，不宣称Troika产品通过或失败。
- 首错即停动作并由原路径正常stop/wait，无强杀；launcher0/gamegone/pad restored，Native cleanup与entries restore真、XR0、objects destroyed/shutdown completed/orderly stop真。215game/270sim覆盖12.65s；1总览+4密集页27对全跨度亲审，保留标题/等待卡、第三人称暴露→两眼原生空手世界；没有Troika或完整fit验收。配对/两lane最终编码均0。两lane首次因739/639奇数高度拒绝，已保留原日志/exits，只pad边缘为偶数后编码，不改QPC时序。
- 8项本方运行前manifest/config/settings/head/public-state/3私有bin逐字节恢复；worker/done/log/数值证据未覆盖。既有retention61012终态0，删2334媒体365858323B，最新3=18952/48832/25820、48h；源/构建/资产/存档/日志不删。18952-review/restore/retention及最终核验均保存。
- 交前再次完整核Snub4083保存源/4bin/21direct+support/1274AST、1791PG/1138Native，0异同；全部jobs终态、最后idle及8项exact后，已最终唯一交窗给Snub174315并收到确认。闭环review SHA2951ce87efefeb05a66ee518f3e5a43ea0ff7fde016028c4c52496bd729e923a；successor final check SHAbee6e87eb746897ad538258f1cbd8f02f04879fad9719ea385c0c1e7ed2eb04e。本方不再拥有旧窗口，不追加case。

## 2026-11-09 新203000 reader2独立候选（仅prepare，未stage/run）

- 新troika_entry_chain_reader2.py只在同一attach-only Bridge调用原getter前显式r.prepare并前后verify及同main-module核对，不改共享runtime.py/Resolver，不新增writer、输入桥或直接TryToDrive/DriverLeave/fire fallback。原174900 wrapper/chain/plan/prepared与18952失败保持原SHA。实际修正getter仍未game验证。
- 采用此前隔离组合并最终编译的PG3365bbe0，含本枪严格TViewTarget ScriptStruct修正和data-only mounted读者；Native仍原c985，旧Native没有新mounted/basis/geometry接口，不能由这些数据授Grip/action/C0。其他枪WIP、共享C0/Native变更未混入本候选。1455保存源/6bin/177driver/352AST、1278PG和1354Native保存源、6共享协议全核，两个prepare-only0。保存源覆盖不冒称全历史MSVC输入。
- prepared5a46660402881f73aab724e8987a097373227729148014d9aa2435308760c421 / plan3c9f8d76118115f38dc89cf2cbb9c364f0682bc156638f3c3447abfd25fc0c09 / readydf809477669d933b8b7e6b4c25ed863453d2b2dbb8c4224ff80c09208adb074e。只作为新修复申请沿Snub既有Cover/实际后继路由队尾，尚未被受理/授窗，未stage/run。GN202146也仅路由，Frag200216撤回保持。
- 原完整目标仍active：empty touch进入和原X退出尚无实际动作证明，持续双Grip/共享左手完整prepare-arbitrate-commit、原aim/fire/资源/停止账本、fixed view/tagged Driver sole C0和所有部件手型/过渡/双眼/头显仍未完成。不把本轮启动/工具修复变成整枪验收，不扩其他枪或第二writer。


## 2026-11-09 mounted左握把资格交易已接源码（未安装/未运行）

- 新独立troika_mounted_control V1只表达本枪当前左support所有权，不借普通Frame/WarPawn绑定。geometry prepare、Native固定函数调用原cover_input_live::participant/唯一left_hand_owner、PG不可变commit确认、Native complete与关闭复核共享原注册/context/module pin的一次同步callbacks/normal_calls预留。Native-only间隙也不可unpin；原普通authorization保持false，未接aim/fire/C0。
- 新troika_control_live不创建第二decoder/arbiter。真实LGrip新按且原左掌唯一左shaft接触才申请，已真正commit的同一lease可随raw掌机械运动持续；错误/歧义shaft、index替代、外侧按住后返回不新授。输入/producer/角色/任一手跟踪变化重置现有owner并要求release/new press。新默认OFF mounted共享分支只在进入/退出、raw或映射丢失时invalidate，不再每Scene杀持续握持；每publication只交易一次，重复渲染既不重新仲裁/计时，也不把duplicate视作失败撤回。
- 先用CPU case复现Native complete拒绝后缺显式abort，已在释放pin前补abort；原失败日志保留，此为合成控制交易收尾修正，不宣称实际开火/C0漏洞。380项新Broker检查+原2433/geometry204通过；真实原CoverInputOwner/唯一LeftHandInteractionOwner和生产Source/control方法的合成生命周期340项通过，含持握/释放、wrong/ambiguous contact、index、held-return、commit失败、epoch/tracking边界、旧abort/伪造/过期/stop、未complete不读回。Source/basis68项通过。均为合成输入/engine数据，不是原游戏握把或GPU证明。
- Native只对准确当前mounted Source/进入token/Driver/票据/100ms返回已complete结果；PG沿原handrig存活admission/真实Native可执行入口读取，有限日志标ownership_only_no_aim_fire_C0。PG prepare前后及commit另以本枪typed原Pauser/InputIsDisabledCount/bIgnoreLookInput/bCinematicMode/正Driver+turret Health/bDeleteMe/既有动画与switch检查拒绝暂停/禁用/死亡；不误拒原坐姿IgnoreMoveInput。六个原对象表声明已保存佐证，实际值/行为仍需game验证。
- Native1196文件保存组合最终ON/OFF生产构建0，PG1279文件组合最终0。初次PG用动态wchar指针调用既有数组模板field helper失败已保留，改用严格literal调用，不扩共享Reader。精确SHA/最终日志/源清单见checkpoint与native-mounted-control/integrated-mounted-control，保存快照不冒称全历史MSVC输入。共享改动只本方broker尾部/runtime/capture和原default-OFF mounted分支，PG共享cpp仅一个头include；独立patch reverse-check0，未改C0/普通绑定/其他枪分支或共享index/HEAD。
- 已冻结203000的prepared5a466604/plan3c9f8d76/readydf809477完全不变，仍只已发送实际尾申请，无新窗口。新Native372d39a4/OFF349e4e31/PG30325751没有安装/stage/run，不能混进203000；18952失败与已完成收尾/交Snub记录不变。下一仍需右Grip/新R2、原pivot/limit/muzzle的实际权威aim/fire/资源停止账本、fixed pre-HMD view及tagged Driver同一C0、全部部件/过渡手型与实际双眼/头显验收，完整目标active。


## 2026-11-09 双Grip／新R2资格交易（已编译，未安装／运行）

- 本枪geometry/control扩为V2，新增两侧记录的原始squeeze按下掌位置接触；当前掌和原始按下掌都必须唯一接触对应原shaft。Native跳过外侧按下帧后持握返回不得授予；跨样本超过原100ms窗口撤回并要求release/new press。原Source V1、普通六个ABI/Frame/binder/authorization不变。
- 右Grip与R2状态消费同一真实已选ControllerInputFrame，不加bridge或左decoder/arbiter。两侧持续Grip＋R2先真松再新按才产生输入意图；任一释放/失追/交易失败清除，按住R2重抓不自动射。右状态仅在原PG ack＋Native complete后保留；持R2保留原edge sequence和原prior-sync窗口，不重记时。该收据不是engine firing或C0许可。
- 生产Source/control方法和原共享CoverInputOwner/LeftHandInteractionOwner合成检查721项0fail；原Broker2433、geometry204、control380通过；保留原进入基准的press映射52通过。测试最初不平衡counter包被上游正确拒绝，改为合法平衡的多次跳沿反例，未放松生产包验证。
- 私有保存1197文件Native ON/OFF、1280文件PG均生产编译链接0；Native51d9a53c/OFFe54a5ee0/PGa07fef46未安装或run。首次命令错误使用不存在的GearsNativeCapture项目，已改实际GearsCapture目标，原日志保留。共享capture仅本枪三个动态导出V1→V2，小补丁reverse-check0，未改C0或其他枪源码/共享index/HEAD。
- 原203000 prepared5a466604/plan3c9f8d76/readydf809477及reader2脚本保持。GN实际尾只读初核通过但未正式受理/授窗；新独立execution封套固定解释器、PIL.Image/_imaging、NumPy native/.libs、编码器/录制依赖与调用ENV/cwd，在任何stage之前用固定路径O_EXCL永久消耗一次声明，验证或执行失败也不重用。只离线验证及独立fixture反例，实际claim未消耗，未stage/run；新plan/ready完整身份见checkpoint及execution-ready。
- 后续仍需真实原pivot/limits/选中枪口的权威aim、原Begin/EndFire及未知派发责任/自动恢复账本、同一C0的tagged Driver与固定观察view、各部位/过渡手型及实际双眼/头显验收。完整Troika目标active，当前仅源码资格交易进展。


## 2026-11-09 原R2责任与停止生命周期（已编译，开始资格仍关闭）

- 新weapon_troika_fire_owner消费原已complete V2资格，不重解输入。原ButtonPress派发之前已记owed/uncertain/stop-required，原release之前已记stop-dispatched/uncertain；即使SEH跳过返回赋值也保留责任。返回/未知释放不重放，只有证明未派发时可重试准备。原100ms触发窗口不续；持续R2的原edge/window不变，每个新输入仍须真实瞄准/枪口/手部资格；自动状态暂静不自行丢所属者。
- 新PG本枪原按钮适配使用严格typed WarPC.ButtonPress/ButtonRelease(R2)，原Args/owned bridge/Native唯一action+CF路径，检查真实旧mount/Driver/原Weapon/Native cookie/原控制收据/输入状态；新开始还要求真实PlayerTurreting。原PreButtonPress/PreButtonRelease维护bFire与held，Turret.Tick结合PendingFire；不能仅直接调用一次Turret.StartFire，更不能借载具PlayerDriving分支。21份保存原脚本+3对象声明文件证据已记original-R2-call-evidence.json，NFUN/NULL仍非live ABI证据。无直接bFire/PendingFire/InstantFire/EndFire写入或fallback。
- 停止确认单独读真实旧资源与当前PC flags up、原PendingFire=false/IsFiring=false且Active/Inactive状态前后同一；返回成功不自动当停完，不能用固定热量/恢复计时。原正常退出后允许只读旧资源确认，禁止向新的占用者/人物释放。缺读/新占用不当对象灭失证明，production extinct仍保守false，旧世界灭失退休待后续真实证据。音效/伤害停止仍需实际图像/运行验收。
- mounted tick已送真实控制意图到此owner；原shutdown/drain把fire owed计入，并在有责任时禁止mount退休/正常shutdown DriverLeave先行。实际fixed-pivot aim/当前选中枪口与同一C0手部资格生产者尚未接通，production readiness明确为空，**不会产生新的原开始开火**。这不是完整开火接线或可玩交付。
- 141项责任检查通过，含真实SEH跳出原开始/释放返回赋值、unknown不重放、晚就绪不延长100ms、自动暂静不早退、释放/epoch/资格丢失/损坏收据。原测试零四元数fixture被正确拒绝，修fixture；setjmp与析构可移植性WX失败改真实SEH。生产PG首次局部pending遮蔽及错误四名Lookup接口编译失败留档，改唯一名字、保持共享三名reader并独立核状态名；未放宽共享读取。
- 保存1283文件PG组合最终生产编译0，精确SHA见checkpoint，Native保持上一轮、不重编；所有新件未安装/stage/run。共享cpp只一行本枪头include；检测到peer同期改动后只叠本方精确hunk，未复制peer整文件变化，未动共享index/HEAD。
- GN正式受理203000+execution e04b6a24/f3a2ae30为其唯一后继，但无最后交窗，实际claim未消耗、未stage/run。原3pins和封套不变。后到Bol212342仅去重待独核；旧Coverr69/578f身份已由申请方明确撤回，未受理/授窗。完整Troika目标active，下一接实际权威aim/muzzle/C0生产者及后续原游戏/双眼/头显验收。


## 2026-11-09 实例柄分配／原双轴心／tagged Driver来源（已编译；先实跑进入闭环）

- 原类默认LeftHandBoneHandleName=handle_R、Right=handle_L，不能拿mesh+X固定当左。Geometry3保留物理L/R shaft bits并附实际实例hand_handle排列；PG typed读两Name，MatchRefBone10/11，前后及commit复核字段/资产，拒未知/重复；Native左右新按及原始按下接触都按实际排列，排列变化撤回旧持握。Control2、Source1/普通ABI不变。803真实共享owner合成检查、原2433/geometry228/control380通过，尚无原游戏柄像素/操作验收。
- 原latitude1→longitude2→handleroot9，两个枢轴相隔24.730187原单位。新增串联两轴求解使子轴心/方向跟随父节点，保留长度和两手残差/秩检查；原位置+合成轴/限位的180项和旧canonical68项通过。首版坐标缩步沿斜误差谷停滞，改受限2x2解析Jacobian与降误差线搜后通过。不是原轴/单位/限位标定或原瞄准已接通。
- 新独立Driver Snapshot持真实mounted Source／已complete控制／同一Native进入固定基准，沿既有连续Tick/C0回调路由Driver mesh，不填普通Binding/CoreSnapshot/Frame。原GT/caller/vtable/世界租约/不可变原参数与原100ms时钟全部检查；拒stereo锁内查询、普通并占、嵌套/源变化。pending覆盖构造、原Tick/C0 gap和正常收尾，未知构造保留责任；stop只撤新资格并结旧scope。Actual native adapter168＋构造异常5项用合成World/provider通过。此为来源接入，**现有唯一C0 writer尚未消费以写姿态**，无新writer/hooks、无pose/aim/fire grant。
- 私有Native1202保存源组合最终ON/OFF、PG1283组合最终生产编译链接0，精确SHA见checkpoint。focused测试先因缺生产NOMINMAX及复用main显式return问题失败，修测试编译入口/return后通过；未扩普通reader。新增件未安装或stage/run；原203000冻结件不混入。共享continuous只begin/end/drain/pending/summary/本枪include，capture仅本枪Geometry3导出，两个精确patch reverse-check0，未动共享Git index/HEAD。
- 主人专项方案review已写7步完整范围troika.json，工程开发/可交付评分25%、物理头显false；统筹确认。当前优先固定203000实际空手触真柄→原占用成功→稳定HMD观察→原X退出，不再横向扩控制/封套或重复核未变闭包。之后再同构成接双握/两原轴心限位/same-C0 Driver，最后满足真实aim/muzzle/双手资格才开原R2，再补自动恢复/全部中断/逐部件自然手型/交付。入口短切片不是整枪完成。
- GN受理仍非最终交窗，实际claim不存在，无stage/run；原3pins及executione04b6a24/f3a2ae30/a46f32e4不变。Bol仍唯一后继；Bol另受理Cover新r69-admission-v2，LS222840仅沿真实尾转递，未并列登记或授窗。


## 2026-11-09 23:46 203000启动前首错、导入环境修正与唯一交接

- 已收到GN最终唯一release0db29e05，独立核6证据、原3pins/execution及10即时基线/idle后，仅消耗一次203000永久claim；stage完成，但launch_reserve.py第17行导入prime_owner失败，no_birth_receipt，launcher1/outer0。**无游戏、Native、XR、operator、采集启动**，触柄/进入/HMD/原X退出均未验证，不能称XR0或动作通过。首错、原日志、claim、10备份/恢复均在analysis/troika-development-20261109/entry203000-owned-run。
- 根因对照：封套继承PYTHONSAFEPATH=1会让打包Python丢失脚本目录，找不到原sibling模块。仅移除此继承项恢复真实prime_owner发现；Bol独立以原launch_reserve --help对照亦复现1→0。本枪新增run_entry203000_childenv.py未冻结源码，仅修该ENV项并换新plan/claim/未登记parent元数据；AST逆向规范化证明动作/guard主体不变，外层-I保留。旧冻结wrapper/3pins/失败claim不改不重跑；新源无plan/ready/claim/window。
- 10基线原字节恢复全match，public未写；本次无新game/session/media，图审/编码N/A而非通过；前驱刚完成的GN retention0原证据复用，没有重复全盘删除。全部实际作业终态、idle后，Bol旧779eca因同错撤回，保留唯一待位，修正为851ea67e/d26f136b/dafb64ea，原4pins/制品/动作不变。独立最终2066文件、1811Native+1847PG保存源、4bin、严格逆补丁及精确prepare0、claims absent/10基线通过后，最终e151ccfd唯一交Bol；本枪不再写共享运行文件或启动。
- 主人最新补充顺序：当前目标逻辑完整＋必要编译后，再在唯一窗口录制游戏/模拟器并亲审；不以不可执行的零散录像替代实现，具体难点仍允许有界原始证据。完整Troika范围保留、评分25%/头显false不变。下一源码缺口是同一唯一C0 state/Kernel/Backend消费tagged Driver、真实两轴瞄准/原枪口与R2资格、逐部件手型；已向既有共享编辑者约定ActualContext/pose_begin非重叠hunk，未新建writer或伪造普通Core/Frame。


## 2026-11-10 同一C0 Driver消费源码／私有编译（未实跑）

- 新troika_c0_hands仅从真实待捕获115骨架数据＋独立mounted Snapshot构造owned候选；原坐姿身体及所有T/scale保留，只求固定骨长臂腕／辅助骨／本枪两柄手指，右食指跟R2；HMD只观察，超臂展投影明确标识，不转成射击权。柄姿仍是待真图校正的作者候选，不称掌指贴合已通过。
- ActualContext显式mounted tag接同一State/native Backend/Kernel/Lease，真实Driver原Tick/C0、参数、caller/vtable、世界租约／原源时间和原100ms检查保留；普通auth数据在mounted分支置invalid，不构造正的普通Binding/Core/Frame。新body代码以SOURCE宏隔离，OFF不要求本枪body依赖。
- 当前registry完整身份／typed Driver.Mesh／SkeletalMesh资产与Marcus115真reference捕获闭合；不把anchor完整identity与C0紧凑identity混成一类。原准备／提交、消费／finally、实际overwrite责任仍由原Kernel完成；旧状态未清仅因新占用发生。进入时已正常消费旧POD，只能通过真实Driver Tick证明同人/PC/mesh/asset/session后转移retirement描述，实际原覆盖仍须发生。50ms纯deadline恢复沿用同avatar/storage/reference/数组/session/world/真实C0操作次序，并在两种上下文间使用原有同一QPC操作stamp；原Scene/控制器时间不更新。
- 复现两项后修复：Output.report对齐填充导致相同姿态字节不一致，改整份owned输出清零；旧消费证明只核根头脚肩，合法腕/手指/辅助骨改动仍可通过，Troika现需额外全局部q容差1e-4rad及精确T/scale匹配，不能把body-only回执当手部通过。保留失败日志。
- 保存源组合1206文件上Native ON/OFF生产目标均编译链接0；ON f60ca1d06de8c74680f98572443ab92e3b28af1158e06239ab9db3e7934b62ef，OFF db1ab0750db856209be362e096546d57444d4827b9c554ee155ad03be0f91029。它是旧private-driver树上的自有增量，不是全部peer当前WIP的集成构建，也不是全部历史MSVC输入证明。主握姿数学/真实参考＋owned Native CAS128/Kernel共513项，真实scope adapter＋合成World/provider共182项通过；没有游戏、真实Driver元数据或GPU/skin验证。
- full_body_c0_native_live.inl自有精确补丁195行reverse-check0，已和既有共享编辑者确认ActualContext/pose_begin不冲突，保留Lancer body_c0_hands.h自有中性扭转修正，未动共享index/HEAD、无新hook/writer。源码与artifact详见analysis/troika-development-20261109/c0-consumer-20261110-status.json。
- 当前没有Troika窗口、新游戏或录制请求；public v71未写。原production fire Backend readiness仍nullptr。下一段继续实际mounted视点／原双轴心限位与权威瞄准／实际枪口，再接原R2，之后完整自动恢复、中断、退出及逐部件真图贴合/交付；整枪与头显均未通过，25%工程评分不因测试数上升。

## 2026-11-10 挂载固定观察视点源码／私有编译（未实跑）

- 新troika_view_live沿实际当前Scene、同批raw stereo、进入token、原占用／Driver／双mesh／weapon、Native session/reference与原100ms时钟复核后，选择进入前保留的entry_base_view。双眼仅在原stereo_matrices合成一次；不用world_head再次做base，不依赖当前炮塔相机或炮塔旋转。视点不要求Grip/contact/fire资格；没有额外相机/Controller writer。
- 只在capture.cpp增加独立include及原stereo_matrices前的SOURCE guarded选择；ordinary和SOURCE OFF仍沿原调用。失效保留候选base字节并拒该mounted双眼候选；不延长旧时间、不重新选输入。共享编辑者确认两个hunk无冲突，保留其body_c0_hands.h／hand_shoulder_pose.h改动，未动index/HEAD。
- 新数学/双眼映射233项、实际production adapter＋稀疏合成world/session106项、既有来源生命周期68项通过；涵盖两眼与手映射的同基准、非零头部运动、角色/占用/身份丢失、读取竞争、原100ms过期、reference/session/scale变化与退休token。适配器初次在checkout根启动导致夹具读取失败；同二进制从保存scene-fixture目录运行后通过，初始工具输出另存receipt，未把工具错误当游戏证据。
- 上一私有C0树最小delta保存1207源文件、构建后全清单复核通过。SOURCE ON Native e2dec86906ec2a8448133c43fc61ad5546551a6f042bfae87b57efac8cd63c5c，OFF 903f6907a95b13b2cc560016a28d9febbfef520f660468d552abb536098d3fee；两配置均configure/build0。不是全部peer WIP集成、全部历史MSVC输入或运行证明。独立shared patch SHA660aa8c5aa0e43658fe1535657653fc5c98c24584cd0a4ad08d3f6003d3e6595，root reverse-check0。
- 原脚本已核：CacheAnimNodes取两个原SkelControlLookAt；FaceRotation由Controller.Rotation取得AimDir；UpdateTurret更新两原TargetLocation；原视角流程仍有yaw/pitch限位。Engine表提供LookAtAxis、AllowRotationSpace、TargetLocationSpace、允许轴、反向与限位字段。下一核同实际实例的typed control配置，不从骨名、合成拟合或HMD方向推断原轴；主证据路径/哈希见original-aim-view-next-evidence.json。
- 详见analysis/troika-development-20261109/fixed-view-20261110-status.json（SHAa828cbf7b210bb673804bebc80db8865233242bfc5e8f382d002a450195bf933）。未stage/run/请求新窗，public v71未写。实际挂载画面／进入眼位／坐姿遮挡／双眼手型仍未证明；原权威aim、实际选中枪口与fire readiness仍缺，production readiness保持nullptr。整枪active、25%/headsetfalse不变。

## 2026-11-10 原控制器配置及原GetEnum接入（未实跑）

- 新weapon_troika_control_axes只读实际缓存Pivot_Latitude/Pivot_Longitude：两对象必须不同，实际SkelControlLookAt祖先和属性声明reference身份均核对，同一SkelComponent、准确ControlName；读取原LookAtAxis/UpAxis、允许旋转/目标空间、反向/允许轴/限位位、原标量限位/strength/alpha和有限target/base向量。双份完整值和声明元数据在原同源current检查之间关闭复核，不改公共反射读者。
- 反例实复现“缓存字段之后的属性链成环仍被首个命中接受”，新读者补声明类链有界完整检查，拒环与重复声明；同名新Class不能替换原属性reference祖先身份。错误位掩码、部分重叠字、错误声明类别、NaN、复用和查询间变化均拒绝，拒绝保留调用方输出。
- 新weapon_troika_enum_getter沿实际Core.Object.GetEnum读取原枚举名字，不假定字节编号。函数/参数E、I、返回FName的类型、标志、声明Enum reference、偏移/extent/布局无别名均在调用前后核对；每个独特枚举值还在结束重新查询，稳定UEnum身份不代替内容证明。测试故意使用与本地Axis/Space枚举不同的编号；原结果未知/变化即拒。源码能解读原名字，不表示原getter已真实执行或机械轴已认证。
- owner GT仅对新mounted publication显式读取控制配置；原contact-geometry回调保留原路径，不把原控制配置getter加成接触资格前置。没有新input writer/hooks/C0路径，原Scene/控制器时间与100ms不续。控制名字数据本身不给aim/fire资格。
- 144项不同检查在当前源码和精确冻结生产依赖上均通过（不是288项不同检查）。使用真实Reader/配置/调用schema和owned人工实例，GetEnum派发为Host模拟。因当前公共weapon_hand_client.inl与冻结PG基线不同，另从冻结树重编同一检查闭合依赖；未混入其他枪WIP。首次WX局部变量遮蔽及cls/klass编译错误、成环失败和最终日志均保留。
- 旧private integrated-handle-assignment树上的自有5文件增量保存1286文件，清单构建后全核；Playground生产目标configure/build0，DLL SHA cab74d58c258e738b3e41f35452218856a2e524bd664f8e7b1af0bab386952b4。Native未变不重编；未stage/run/请求窗口/写公共v71，未动共享cpp或index/HEAD。
- 状态analysis/troika-development-20261109/control-axes-20261110-status.json，SHA9a3e1760a3c142c67d135d5f3b619527ce777fb82cd7e6b601d696b67cca7d42。主表亦有AnimTree.SkelControlLists、SkelControlListHead.BoneName/ControlHead与SkelControlBase.NextControl；下一核两缓存控制器到实际原latitude/longitude骨的绑定，再接原空间/限位的串联机械求解和权威aim。原枪口、C0手部资格、R2、自动恢复/中断/退出/全部贴合仍未完成，production readiness仍nullptr；25%/headsetfalse、完整目标active不变。

## 2026-11-10 实际动画控制链／骨索引与参考拓扑接入（未实跑）

- 新weapon_troika_control_binding从Mesh实际Animations读当前AnimTree，不用模板代替；反射验证SkelControlLists的原ScriptStruct及BoneName/ControlHead/DrawY动态成员布局，遍历有界NextControl链并核同Mesh。两缓存控制器必须各出现一次；环、跨链共享、重复骨名、丢失/错误原对象、类型/声明reference/metadata变化均拒。Mesh实际SkelControlIndex须与发现的骨/链逐一相符，两目标链不得被其他骨重复使用。
- 读实际Geist_TroikaCabal.TroikaTurret的13条参考记录，核全部原骨名/父关系、有限位置/单位四元数及无ParentAnimComponent。复用现有Native ReferenceRecord68布局（q12/T28/parent56）；RefSkeleton反射dummy IntProperty/4单独认证，不把4当native stride。原导出与当前source ABI是依据，真实Troika内存/原控制执行仍待游戏证明。双份完整metadata/链/索引/参考字节在同原current/100ms之间关闭复核，不更新时钟。
- 发现首版按控制器名称强制对应同名骨，合成“控制名顺序相反但原链/active index一致”反例实复现拒绝；已改为按真实原对象发现bone_index，允许1/2两种唯一实际排列，机械父子顺序由原参考拓扑确定。保留失败及先前已编译组合，不把名称当机械证据。
- 179项最终绑定检查在当前源码与最终冻结依赖上均通过；此前同一helper来源上的144项axes/GetEnum兼容检查通过，最终只再变两个binding专属文件，其相关依赖清单相同，不重复核未变case。全为真实Reader/schema＋owned人工tree/reference/index数组，未执行原引擎或验证GPU/炮口。最初BoolField/PointerField声明owner helper编译错误保留，修为同一owner字段的类型化模板后通过。
- 原contact callback保留原几何路径；现有owner GT新publication的完整控制观察才可输出已解枚举＋发现绑定，最后再关闭复核原控制配置。不构造普通Frame，不产生机械校准/aim/fire权限，production readiness仍nullptr。共享cpp/C0/Native和index/HEAD未改。
- 最终1288保存源构建后全清单复核，私有Playground configure/build0；DLL SHA ffddeb032f2b8f2059fb072738e7f8cbc482f03a342ddb0e130a38e70eb83751。早期同名假设组合f0a9ff68保留且已被此组合取代。未stage/run/申请新窗/写public v71，Native未变不重编；不是全部peer WIP集成或所有历史MSVC输入证明。
- 状态analysis/troika-development-20261109/control-binding-20261110-status.json，SHAa9feefac65f4ecb6e40d40f54c6f238a53927165e73c835c7c55c27a16e41db5；下一核原空间/允许轴/参考限位的机械解释，再接双手权威瞄准与实际枪口/C0手部资格。原R2/预热/双枪口/自动恢复/全部中断/退出/逐部件真图/交付仍未完成；25%/headsetfalse、完整目标active不变。

## 2026-11-10 双握机械候选／原旋转限位GT生产者（未实跑）

- 新weapon_troika_mechanics把已发现的control-to-bone排列、单个允许轴、原命名空间和当前row-vector矩阵转成相对两枢轴的米制几何；大世界坐标先移到latitude原点。Bone/ParentBone子轴随父转，World/Actor/Component子轴保持外部方向，后者使用不同正向模型/解析Jacobian。OtherBone只接受已建模的0/1/2依赖，父关节用子空间等未建模情况拒绝，不把未知配置当通过。
- 原solve_troika_serial_grips继续要求original_hierarchy_axes_limits_verified。新增独立TroikaSerialCandidate仅供数学候选，搜索区间不是原限位，不把public严格flag置true。269项合成row-matrix/独立目标与方向、两种子轴模式、控制角色排列、坐标空间、大世界原点、非法矩阵/输入/旧资格拒绝检查通过；原180项严格serial检查保持通过。
- 新weapon_troika_original_limits为原SkelControlLookAt.CanLookAtPoint和Troika.ProcessViewRotation提供实际反射布局绑定：Core Vector/Rotator成员、原声明、参数flags/宽度/extent/偏移、Bool mask/packed别名均在调用前后复核；debug始终false，旋转查询以已有绝对目标＋零新增delta/time走原限位，非零残余delta/失效拒绝。62项真实Reader＋Host模拟派发检查在当前与精确冻结依赖通过，未证明原引擎调用。CanLookAtPoint已编译但未作为资格使用，原PointLoc/native语义仍待核。
- 新GT mounted_aim_candidates消费同一真实mounted Source／已complete双握资格／原input_allowed；掌锚点仅在实际分配shaft接触时捕获，存handleroot局部坐标，后续由当前原模型和同源raw掌求解。mount/lease/right-press/continuity改变撤候选；读原typed Turret.AimDir而非HMD方向，形成请求旋转并调用原ProcessViewRotation保存clamped候选。该GT适配器本轮仅生产编译，未作运行/夹具执行证明；没有Controller/AimDir/atom写入，没有fire readiness连接。
- 修正GT数据消费：共享mapper可能已被geometry callback先读，new_publication=false不能代表该GT读者已消费。新增data-only ConsumerCursor，各读者按实际Native key/publication各处理一次，重复/倒序/过期拒绝，原100ms/时间戳不续。240项root映射/视点＋cursor检查通过；CPU示例说明旧条件会跳过已映射的同一新来源，不冒称游戏失败复现。PG冻结mapper仅缺root既有render_base视点method，移除该无字段变化的method后源码相同，差异已记录；没有为此重编未改Native。
- 私有最终1294文件组合构建后全清单核，Playground configure/build0，DLL SHA770d2d4105540e0c1be8a35dc437fbd3556227917ecc9e0df600d93873d93d5a。先前helper构建fc2f与生产者observed参数遮蔽全局的WX失败/源均保留；仅局部改名后最终成功，不混入其他枪当前WIP。共享cpp仅global common头一行，自有独立patch SHA9e3159d7af11777f00297fa738c68d4458cd5c109a26503c60e7039589644578，reverse-check0；未动index/HEAD、无新hook/arbiter/input reader。
- 状态analysis/troika-development-20261109/aim-candidates-20261110-status.json，SHA22ce29697ad4de87c5e1401fa53dd1204f83948351655e3532c030a6bebad05d。未stage/run/申请窗口/写public v71。下一接同源原Controller旋转写入与真实消费/原选中枪口、同一C0手部资格；原native限位/point坐标/机械响应、R2/预热/双枪口/自动恢复/中断/退出/逐部件真图/交付仍未完成。Backend readiness仍nullptr，25%/headsetfalse、完整目标active不变。

## 2026-11-10 原Controller调用／挂载右视角屏蔽（私有编译，未实跑）

- 同一GT、当前挂载Source、已complete双握和原input_allowed通过后，clamped候选现接原Actor.SetRotation与Troika.FaceRotation，未直接写Rotation、AimDir或骨骼。按实际反射闭合NewRotation、Bool返回／DeltaTime的声明、布局、flags、类型与偏移；原SetRotation返回Bool与桥接调用返回分开记录。调用前记录attempt，同publication不重派发，重入观察不能替换进行中的receipt／掌锚点。失去资格后撤未来写入，不把旧旋转恢复到新占用者。
- 读取原Controller.Rotation确认目标，再调用原FaceRotation，读取原Turret.AimDir与Controller方向相符；整圈65536等价，容许一单位量化差。该receipt仅是Actor确认，未证明原UpdateTurret／动画／枪口／C0消费。没有手动补调UpdateTurret，production fire readiness仍nullptr。真实Reader＋Host模拟派发54项通过；原引擎调用尚未游戏记录。
- 发现原虚拟手柄仍可能输出RX/RY与双握Controller写入竞争，故在既有Route与XInput状态回调添加仅右轴屏蔽。已认证挂载角色建立屏蔽，只有真实game-caller收到零右轴的轮询才确认；建立／轮询／解除均用原route_mutex串行。左轴、原按钮、短脉冲及原fence保留。挂载读失败不解除限制，仅闭合通过的普通来源发布或正常stop解除；普通交接时持续偏转的右摇杆先归中再重新输入，避免持有输入突然出现。
- mounted Source升ABI2，原reserved槽现为显式control_policy=1，默认0和ABI1拒绝；Source读／映射继续要求Native屏蔽确认，旧Native不能授权这批PG写入。普通hand／rig ABI不变；未加hook、输入读者、arbiter或C0写者。纯Route屏蔽／交接24项，Source生命周期73项，原游戏手柄路由回归通过；受影响Root control805／Driver182／geometry228／mapping242／fire141项通过。C0真实reference＋人工tracking/control513项通过。精确冻结依赖重复54／24／73／513项通过，重复检查不另算游戏动作。
- 保存Native1208文件和PG1296文件组合，构建后清单逐文件核；Native ON、OFF和最终PG configure/build均0。Native ON SHAae9b428bbdf2a996b36c3babe04705b36763cf5b470b0565793cff57cc6bf6a3，OFF SHA41d27890eea63033fa407666907fc3de1d17d650bc3d95f878159851452fa621，最终PG SHAf3887f1cdf9f905841db5f511ebebe24e5c60f4357cf9d7a449012f3a24b664e。仅在旧私有组合上应用已核自有增量，不包含全部peer当前WIP；完整保留中间组合、夹具失败及命令宏重复的构建失败。
- 共享增量为vr_gamepad_input.h／test main guard、vr_gamepad_live.inl与rig_tracking_live.inl的窄hunk，独立patch SHAbf0e226b5575b121c9dece0653cbbf57d57179b03875c9a706e43338ada9c0a4；混合LF／CRLF按ignore-space-change reverse-check0。未改capture.cpp及Lancer手部WIP，未动Git index／HEAD。源码／编译状态见analysis/troika-development-20261109/original-aim-source2-20261110-status.json，SHA055648d805926549ecf33da1db23baf4d139079c00d59057778764458391c87a。
- 未stage、运行、申请新窗口或写public v71；本批没有新增玩家动作通过证据。下一接较后原动画的实际枪体消费、原选中枪口和同C0手部资格，再开放原R2路径并按授权执行配对实录／逐图复核。完整进入／退出、原预热双枪口／自动恢复、全部中断、接触／遮挡／袖口／过渡／交付与头显验收仍未完成；25%／headsetfalse、完整目标active不变。

## 2026-11-10 较后Scene枪体／实际选中枪口观察（私有编译，未实跑）

- 新gun_consumption保留上一条已获原SetRotation／FaceRotation确认的双枢轴预测。只有请求与原clamped方向一致、严格较后publication／Scene／QPC、同一新鲜挂载／输入生命期／左右握持与press window／校准才比较。根保持固定，父枢轴单独转，子及两枪口／handleroot按原分离中心和固定或随父子轴模式变换，保留row basis与scale；六组原GetBoneMatrix输出须在2mm／0.5度内，当前Controller／AimDir仍须符合前一请求。不是用Actor读回代替枪体，也没有手动调用UpdateTurret。
- GT先确认上一请求的实际枪体，再求当前输入的新请求；该gun receipt明确只描述前一请求，未被当作当前新写入或射击资格。原控制配置／tree／参考／bone绑定变化会撤校准并要求再次真实shaft接触；不把TargetLocation／BaseLookDir等运行缓存向量的变化误当配置变化，实际矩阵另核。当前raw掌须仍接触各自原shaft，距保留掌锚点不超2.5cm。GT适配器本轮是生产编译证据，尚未在夹具／原游戏执行。
- 新socket_binding实际读取SkeletalMesh.Sockets及声明的Engine.SkeletalMeshSocket类型，唯一匹配当前原MuzzleSocketName、asset所属与原muzzle骨7或8；typed相对位置／旋转／正且均匀scale、数组和元数据前后关闭。所选世界socket须符合实际对应枪口骨的相对变换，且与原GetPhysicalFireStartLoc在0.5mm内相符；旋转在0.0005rad内。没有中央枪口替代、根据手侧猜socket或本地轮换双管。
- 原physical-start getter另加完整动态schema：准确原声明、Core.Vector数值成员、输入／返回flags、非重叠存储、宽度／offset／extent与前后元数据，FireOffset显式零（支持原可选输入形式），输出须有限。原反编译Tick→UpdateTurret以及GetPhysicalFireStartLoc使用原所选socket的代码与对象表均按SHA留证；源码关系不冒称已执行。
- 521项独立合成变换／较后来源／过期及换挂载拒绝、47项实际Reader socket资产绑定、31项实际Reader physical getter schema＋模拟调用通过。冻结组合重复521／47，最终physical依赖重复31；不同冻结仅physical helper／test两文件，清单差异已核，重复不另算游戏动作。负例最初误写UProperty宽度0x8c，改为真实0x74后通过，保留日志，不称游戏／reader缺陷复现。新helper的alias与fixture-main宏构建失败也保留。
- 最终1303保存源逐文件复核，私有PG configure/build0，DLL SHA1365e5b40c212b0352b622b5c7c0c93fc31c26fb2bd42fb261f0e3d4fb6810d1。共享cpp仅加common头一行，独立patch SHA363cb106a0ac426db2e47d9ebb471db518fa897cd280eb012dfa523630a8a8c8，reverse-check0；没有整份当前peer cpp覆盖。Native未改，不重复构建。状态analysis/troika-development-20261109/gun-consumer-20261110-status.json，SHAd9f21a6c54137e7db053532f29e6c6d76dee186dc35a2103aaf16fc06804f97f。
- 下一接Native已有同Kernel C0 local／space消费的当前只读资格，再连原R2 readiness。实际原函数ABI／动画顺序和延迟／响应、原限位与point语义、完整进入退出／预热双管／自动恢复／全部中断、手部所有接触／袖口／过渡／真图复核／交付／头显仍未完成。Backend readiness仍nullptr，无新增玩家动作通过；未stage／run／申请窗口／index／HEAD／public v71写入，25%／headsetfalse和完整active目标不变。

## 2026-11-10 同Kernel C0当前读取／原R2资格接通（私有编译，未实跑）

- 新Native Store只在原C0正常返回、同Kernel scope_closed／consumer验证且完整local姿态符合计划后发布；Store自己再次调用现有Backend.read_consumed_pose，验证115骨实际Local／SpaceBases与完整计划。未创建另一Kernel、普通hand Frame、hook或骨骼写者。原双方solved且无投影、同Source／control、原asset／owner／session／world、canonical115参考必须一致；同Tick／Compose不能重复发布或刷新消费时刻。
- 新独立只读ABI GearsVrReadTroikaC0ConsumerV1沿既有Runtime／模块生命期预约及World lease查询。限定同一当前挂载publication／握持、同Kernel serial和原50ms期限，重新关闭当前身份／模型／映射／元数据，重复读取真实Local／SpaceBases／参考骨字节并与已认证副本相同才返回；不续completed_ns／observed_qpc。返回两腕／原目标／raw grip、30个实际手指局部四元数及4个helper，不代表硬件上传、碰撞或皮肤贴合验收。ON／OFF导出实表已核；OFF或无Body Backend返回unavailable。
- PG已有模块admission保护新只读调用。原R2 Backend现接当前已准备瞄准、前一请求的实际枪体／所选socket／物理出膛证据及同源Native C0消费，另核当前原Controller／AimDir、PlayerTurreting、挂载与input_allowed。顺序改为观察／准备→原R2资格与offer→提交下一原SetRotation／FaceRotation，避免先写新AimDir立即否掉本帧消费证据。原预热、双管与自动武器状态仍归原流程。
- 独立fire cursor同publication只offer一次；重复服务只能在同来源／控制及当前实际C0仍有效时维持已拥有的原动作，否则沿原stop／quiescent清理，不派发新输入或新aim。原R2／Active／Inactive名字分批只读预备，qualified要求名字已解析后才消费按下沿；保留原100ms press window，未刷新输入时间、未加预热计时、未重放unknown或持续按住的R2。
- 53项真实115骨参考＋实际Native Backend／CAS128／同Kernel的owned-page检查通过：prepare／commit不足以发布，原消费完成后正例，Local／Space／参考／epoch／资格失效拒绝，防同Tick／Compose再发；接真实责任ledger的Host模拟R2证明姿态证据丢失会停已拥有动作，证据恢复不重放held沿。精确冻结Native helper／Backend／Kernel依赖重复53通过；最后仅状态摘要／查询观察两inl变化，依赖差异已核。Runtime read_consumer和PG资格／阶段适配器仅生产编译，未作夹具／原引擎执行证明。
- 最终Native1212文件、PG1305文件清单构建后逐文件复核；Native ON／OFF与PG configure/build均0。ON SHA3ac0bc7e9d666668a8591fc1b1ee7f075adf61670e0e6e778c42f09dd16dc0e6，OFF SHA4e34e0a09dc2228fe63d08e8f29a3b3f7a8e1ad58fc2ad3da47361a6fb118b9b，PG SHAe1e09d578eb4030665470e786a6f411bb02cb680a1c849e3b670056ea948deff。旧中间Native、PG observed遮蔽全局的WX失败／源码及夹具QPC时间对齐失败都保留，不作为游戏失败复现。
- 共享capture仅common头／独立export两处，PG仅common头一行；各自patch SHA5924a50f66c37497964259a1268bc9649960f59614fea2da32c68a14a9415b89／361d59894051879159065cbd12f4a1114776471649e1f58639e27832fe7ba03c，reverse-check0。未改共享Body State／Kernel、body_c0_hands.h或hand_shoulder_pose.h，未将全部peer mutable源拷入私有组合。状态analysis/troika-development-20261109/c0-fire-20261110-status.json，SHA8d8e67f6d127e22e653421d000c18ae0f8342c0d2ed5c991011a8801c19f1fa2。
- 必须由实际记录确认PG post-PlayerTick查询时同源C0是否已完成；可能存在一publication时序差，离线／编译不能判定可用。现有摘要增加查询pub、查询当时缓存pub、原查询／完成时间及状态，只为首次受控记录判断顺序，不改变资格。Backend.extinct仍false，尚无肯定的对象灭失结清实现，不能把读不到／换占用者当灭失。完整世界／会话中断与cleanup验收仍开放。
- 下一冻结当前受控候选／driver，在新plan处理已记录的child import启动故障并取得真实唯一窗交接后再stage／实录，保留旧203000已消费claim。无新增玩家动作通过，本轮未stage／run／申请新窗／index／HEAD／public v71写入；实际原ABI／控制响应／限位、完整进入退出／原预热双管／自动恢复／全部中断、所有手部接触／袖口／过渡／真图复核／交付与头显仍未完成，25%／headsetfalse、完整active目标不变。

## 2026-11-10 core2 正版受控实录准备（已冻结，待实际交接）

- 有效候选为equipment-controls-prepared-20261110-045716-troika-core。分别保存实际编译的Native1212／PG1305文件清单和源；generic2140文件包含历史C++恢复材料，不代表实际编译闭包。沿用上一节已编译Native／PG，另固定6个运行二进制、862个runtime driver及2144个执行输入，6个共享协议头与保存的XR基线逐个相同。prepare-only返回0，只做哈希／身份／源和Python解析检查，没有stage或创建运行claim。
- 原默认零售工作副本不存在；改用已购Steam路径J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe，EXE SHAbdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4，安装与Coalesced.ini只读验证通过。新plan固定0.4、behind-windows、observe-only、测试静音及禁用物理控制器后台输入；没有复用旧reader2中的P2P／0.3配置，没有修改游戏文件。
- 使用实际打包Python和原launch_reserve.py --help复现继承PYTHONSAFEPATH=1时缺prime_owner／exit1；移除该环境变量后同一入口exit0。仅执行帮助入口，没有预约、游戏、Native、XR或录制。旧203000永久claim保留且不重启；045425错误文件名导致的离线准备失败保留；未执行core1的异常清理变量未初始化问题在请求前发现，已撤回并保存，只有core2为有效请求。
- core2脚本计划沿既有空掌真实触碰进入，双手持续Grip原分配shaft，必须观察当前原枪体／所选socket和同源C0资格后才送原R2；资格不出现便保留首失败并释放／原X退出，不直接调用enter／fire／aim绕过。资格出现后短查HMD不改AimDir、两段原R2短连发／释放、小幅双手yaw和原X退出。10秒仅是脚本观察截止，不是Mod预热计时。该脚本尚未执行；最终松Grip的ShotCount须与末段after值另核，不能仅凭脚本布尔值通过。
- 四份固定身份：prepared SHA8e805dfe722d3fe980a1824f82fd4a9812df6241b4b3ef8d5012e6be9de3183f；runtime-plan SHA9527bdd3432f1ee57396753d34f7c0b9fb7ebf3f39a911007dc386a499996ddc；ready SHA3d236c6ff4c807fc9aff889531b027932ca4615d6e03c9eeda9ec07e0640c234；execution-plan SHA12974f92bdd36ac53ce1b219237744064555092f18df700885d879df99843c5d。入口analysis/troika-development-20261109/run_troika_core2.py；运行必须绑定实际唯一受理和最终正常释放交接，再在自有窗口保存当前10项基线。idle、旧203000交接或其他枪的意向队列均不是授权。
- 唯一请求已由Bol转发到其直接核实的已受理尾部GN，等待真实队尾直接答复；accepted=false、owns_runtime_window=false。未重复申请，未占第二槽，未stage／启动游戏／录制／编码／写public v71／改Git index或HEAD。准备状态analysis/troika-development-20261109/core2-preparation-20261110-status.json，SHA873d34a7c7777c24d77074f6a6644aee35f2c8323a76917cccf42a87d8cbe8a9；checkpoint引用已核。
- 下次实际记录首先判定post-PlayerTick同源C0是否可见，再按首失败修复；不得放宽身份、新鲜度、释放或防重放。必须分别核实际出生进程、原动作、正常Native／entries／XR清理、配对编码、完整时间线和接触真图。Backend.extinct仍false；全部中断、原预热／双管／恢复、全手部贴合、手动交付和头显验收仍开放，工程25%／headsetfalse、完整目标active不变。

## 2026-11-10 原销毁路径核查（历史源码证据，未实施灭失捷径）

- 核查并逐字节保存8份原导出：Troika武器／炮位／WarPC、Engine Weapon／PlayerController／Controller／Actor的Destroyed以及Core.Object.IsPendingKill声明。Troika武器Destroyed明确处理ShotLoop／StartFireCue并调用父类；Engine Weapon继续DetachWeapon／父类，Actor脚本体为空。这些函数体没有给出旧PC的R2按键与原PendingFire／IsFiring均已结束的回执，也没有揭示native Destroy完成／GC调度。
- WarPC.Destroyed存在普通PlayerController清理与另一分支；后者可退出炮位、UnPossess并将OldPawn交给AI。因此人物或Driver已变不能推出旧射击责任结束。IsPendingKill仅有native final声明，尚未取得其native实现、调用时序或输入／timer结清关系；不据方法名增加调用或将bDeleteMe当作肯定灭失。历史native索引的限定符号／字符串查询未命中，只说明该查询未定位到实现，不能声称实现不存在。
- 证据analysis/troika-development-20261109/lifecycle-evidence1-20261110.json，SHAe6c8b701f9bd8a645de86b037e6f59c55163cd3e5443fa08457a7aa87efd65cb。生产源码和冻结core2未改，无新engine call／hook／stage／game。Backend.extinct保留false；完整中断仍须旧输入与旧武器各自结束的实际原状态回读，或同时覆盖二者的独立、已认证对象代次灭失证据。下一步仍先取得core2唯一实际窗口并核首失败，工程25%／headsetfalse不变。

## 2026-11-10 最新独立执行规则与 actual39644 首失败

- 主人最新要求覆盖上文远期前后继／逐级签收安排：日常源码、必要编译及离线分析独立完成，只有实际启动共享游戏／模拟器时，复用现有互斥和真实占用做简短协调。动作结束或首失败后，正常停止自有资源并精确恢复，立即释放共享环境；独立目录编码／审片／保留不再占游戏。不得打断别人的任务或放宽身份／时效／库存／存档保护。先完整进入→双握→射击→退出，再完善Snap／Volume／反馈，最后专属GrabPose；后两阶段仍是最终必交，不再挡第一阶段。
- 新run_troika_independent.py复用原冻结core2的validate／stage／实际动作和清理，仅移除对前驱受理及交接文件的依赖。暂持现有solo-m1 RepositoryLauncher.Owner互斥保护stage，在原launcher child创建前交还其正常启动互斥；真实占用即等待，不抢占。不改共享launcher／生产代码。新plan SHA564c45454bdf2393842d97733cac68fdad9b24a3470c1e7bb464ce050b995f77，prepare-only0；原203000和core2身份／原件保留，旧claim不重置。
- 实际独立尝试wrapper56008／launcher55348／正版Steam game39644启动，第一阻断是原对话框“需要執行 Steam 才能遊玩”，随后原loader dependencies有界等待失败、launcher1。Native／XR／Troika动作和配对录像均未启动；outer0不算玩法通过。Steam50140自11月9日17:10:57已运行，故不能认定“Steam没开”；精确客户端原因未证明。现有steam-client-launch明确限制手动前台，不能直接切换该模式重跑或绕过Steam认证。
- 原game39644沿正常WM_CLOSE退出；原始10项加私有profile共18项全部精确恢复，public只比对未写，现有互斥已关闭，已向实际询问启动者简短通知释放。运行session5252终态0，independent-core-consumed.json永久保留。证据analysis/troika-development-20261109/independent-owned-20261110-145320/first-failure.json，SHAa3063497cadd584b71a54eac9088c84f40102d42666e5f96b5b20083ff29ebe4。startup-evidence曾广泛备份历史文本，其独立current-attempt-startup-evidence.json仅列本claim实际时间区间5份原件，历史额外备份不计本次证据。
- 下一独立核查后台正版Steam启动环境的具体差异，得到修复后才使用新身份定向复查；不重复已消费尝试，不占着空闲游戏环境等报告。本次没有新增Troika动作通过证据，工程25%／headsetfalse，完整目标未缩减。

## 2026-11-10 用户指定目录／统一Guard／实际39992与55404

- 用户本聊天直接指定后，游戏目录固定为J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64；EXE仍为受支持bdc6705d…a4a8fa4。此前Steam安装登记疑问不再作为本任务前置；没有修改授权／DRM／Steam登录或安装文件。源仍为C盘6dof-single。
- 入口已复用现有tools/debug-launch/runtime_stage_guard.py的RuntimeGuard，从stage持续保护到实际恢复完成，保留原启动器内层Owner互斥。只在本进程中把独立离线ffmpeg排除出共享游戏占用分类，真实game／XR／paired recorder／控制进程仍拒绝并等待。旧入口只持内部锁的覆盖不足已修正；16:09:56尝试在未创建launcher前由宽泛idle拒绝，后来的独立会话开始使用环境，故当时未覆盖其状态。该旧claim保留，后来在统一Guard下18项补恢复完成；不将时间相邻误称为已证实的进程争抢原因。
- runtime2实际game39992／launcher4856正常进入用户目录并激活Native，先卡在走向原GetEntryLocation：14条实际位置记录最后距原点89.61单位，原测试要求35。384游戏帧／493模拟器帧、23.47秒，6页完整配对时间线均亲自看完；可见敌人接近攻击、玩家受阻／后退，不能单凭图像证明唯一碰撞者。未触柄／进入／射击。正常Native、entries、pad、drain、XR0和launcher0，18项恢复并释放Guard。原件independent-owned-20261110-161824/first-failure-review.json。
- 下一runtime3只改测试身体站位35→100，保留实际柄表面接触、1.3m工作空间及原输入／进入保护；生产Native／PG未变。实际game55404／operator54644到达近柄检查，near_without_touch_does_not_enter=true；送往计算表面的空掌未取得原占用，3秒首失败，未补Grip／X／TryToDrive。398游戏帧／540模拟器帧、25.28秒，7页全跨度配对图均亲自看完；敌人持续近距离遮挡，不能据计划坐标宣称手掌实际触柄。两路编码均0。
- 55404生产日志没有troika_entry行，但实际冻结cpp已有tick接线且TROIKA开关ON；其他设备帧203／input matched202／rig ready14等仅是汇总，不能定位前置拒绝，也不能判为tick缺失。下一补本枪有界早期入口判据观察和接触后的实际raw/world读回，再只编译受影响PG定向复查，不放宽身份／时效／原触碰范围。
- 55404实际Native／entries／pad／drain／XR0／launcher均正常，18项原件精确恢复并立即释放，已简短通知实际相关会话。证据independent-owned-20261110-162833/first-failure-review.json。滚动保留工具因其他当前运行安全defer，没有删除当前媒体；后续按既有工具完成，不据此占游戏。完整进入→双握→射击→退出尚未跑通，工程25%／headsetfalse未变，先逻辑、再体验、最后GrabPose的最终范围保留。

## 2026-11-10 专属实例迁移（离线准备完成，尚未实跑）

- 最新独立运行规则采用docs/weapon-development-20261109/independent-runtime-20261110.md及tools/multi-instance/isolated_sim.py；此后本枪不再stage／restore共享solo目录／共享PG输出，也不再等待其他会话的模拟器。四个已消费旧入口的恢复责任均已结清。自己的新实例仍受真实内存可用量和自己实例身份约束，不抢占／结束别人游戏。
- 新入口analysis/troika-development-20261109/troika_isolated.py提供prepare／check／run／action／read。已准备troika-1110-180913，Native3ac0bc7e…／PGe1e09d57…／XR16840b27…与本枪冻结制品逐项相同，不取工具默认的历史武器制品。PG／XR／Native／输入／配置存档／录像均在自己的命名目录；新静态输入适配器只按本实例session_report里的确切game/XR PID、creation、路径和SHA读取，保留Source1、100ms和同票据检查，无全局Operator或全机唯一XR枚举。
- 复用原动作脚本和真实双路HWND录制，只做参数化：私有manifest／控制命令／raw-world输入／停止入口均绑定本实例；run的finally调用isolated_sim stop --name，仅在本实例owner/game/XR原身份均消失后恢复自己的文件。旧共享stop-basic不再写。输入读回和动作变换代码、源快照及三制品SHA检查通过；尚未启动本枪独立游戏，不是动作或清理实测通过。
- 状态analysis/troika-development-20261109/isolated-migration-20261110-status.json。下一仍先为actual55404空掌进入首失败增加本枪有界前置判据观察并编译受影响PG，再以新候选／新时间戳专属实例验证完整流程。工具层双实例成功不是本枪进入／双握／射击／退出通过，工程25%／headsetfalse不变。

## 2026-11-10 启动暂止／入口判据观察已私有编译

- 收到全部静音、后台／不激活／不抢焦点并启动的指令后，已实际调用本方独立入口；当时资源保护拒绝启动，未创建troika-run-claim，未创建launcher-session，没有本方新game／XR。随后收到“又到前台、暂停新启动、不自行试焦点”的最新指令，保留旧记录并持久化isolated-startup-hold.json；继续离线开发，没有重启试验，也未碰用户或其他实例。环境变量设置和新增50ms前台归属采样均不冒称实际后台验证通过。
- 本枪入口增加可选GEARS_TROIKA_ENTRY_DIAGNOSTIC观察：最多48行、每gate最多4行，仅输出原检查已经得到的值，区分Frame／身份／恢复域、空手／租约占用、rig、候选／范围／几何、接触／边沿和原进入前metadata拒绝。原短路顺序、真实触碰、身份／时效／租约和原调用条件保持；无新输入读取、hook、骨writer或许可放宽。该宏在普通构建默认不输出。
- 保存上一1305源组合，仅替换weapon_troika_entry_runtime.inl；entry-observer-20261110新组合构建后逐文件核，configure/build均0，单target／单worker／BelowNormal，未执行测试程序或游戏。PG SHA6275309899bf621addf43b8b91e1c46fa7a9ccc8574bc6c6e41fee7a9b543e87；Native3ac0bc7e…未改、不重编。状态entry-observer-20261110-status.json。专属prepare已支持显式--entry-observer选取该准确制品，不替换历史候选。
- 独立入口新增逐实例前台进程树观察和失败标记；若本方抢前台，停止自己的输入动作并调用本实例正常stop，不激活／最小化／移动用户窗口。游戏沿已验证的逐PID静音guard，XR另接同一逐PIDguard，不调用系统总静音。以上新增适配仅源／编译检查，尚未实际运行验收，不能掩盖当前独立启动路径的前台问题。
- 下一等待实际后台路径修复证据后，按最新运行政策解除本方启动暂止，以入口观察候选做一次完整逻辑定向记录。当前本方没有活动游戏／XR，也不占共享solo，完整进入／双握／射击／退出和头显仍未通过，25%未变。

## 2026-11-10 独立183733取消／最新统一暂停仍有效

- 按当时交付的background1110b及background-runtime.json逐项校验后，重新prepare troika-1110-183733，PG为入口观察6275309899bf…、Native3ac0bc7e…；没有重启旧实例。实际worker47772／game48756／XR42816出生，未激活Native。动作脚本在初始Source1 vr_gamepad回执断言处拒绝，尚无Troika接触或原进入动作；正常stop请求exit0，但原worker的首次game_absent=false保留，不能冒称当时已完全清理。
- 后续只读核实原owner/game/XR均已消失，未强杀，才补做本实例13项原始配置／存档恢复，全部精确一致。XR实际exit1，按取消启动失败保留，不伪称normal0。逐PID XR音频guard cleanup0／mute_failed0，未匹配到本XR音频会话，不算实际有声检测；游戏静音仍由本实例原guard处理，无系统总静音。674次前台采样／67次窗口检查未捕获本方违规，仅为采样证据，不是完整事件追踪，也不否定用户“又到前台”的反馈。
- 最新用户指令已撤回自动恢复运行结论。tools/multi-instance/automatic-runtime-pause.json和本方isolated-startup-hold.json保持原样；入口在创建claim前检查统一标记，运行期间见标记则停止自己的动作并走本实例正常stop，不绕过新入口或改用旧solo／直接EXE。除明确用户恢复外不自行重启试焦点；仅继续离线工作。
- 离线接线修正：父入口此前仅见XR身份就启动动作，现继续等待同一个game的vr_gamepad.source==1，保留子入口原Source1断言、身份和100ms同票据检查；没有把检查删除。仅语法检查通过，统一暂停后未做运行验证。记录isolated-183733-closeout.json及本实例troika-result-before-late-restoration.json／troika-result.json。全目标仍25%／headsetfalse，源／构建／实际动作／后台行为分别记录。

## 2026-11-10 离线目标发现延迟修正，已编译／实跑未验

- 实际55404记录的Turret internal_index185561；旧全局对象扫描每有效调用最多512项，从0到该项至少363次，且此前排在rig ready之后。203设备帧／14 rig-ready汇总不是扫描调用计数，不能据此断言它是上轮唯一首败原因。
- 在本枪entry模块新增原WorldInfo.PawnList／Pawn.NextPawn只读发现，按原Engine元数据核类型、身份、同World、链接重复读取、metadata保留及当前Frame；每调用最多4节点，第一节点后检查250us，256项循环保护，失败清游标而不伪造null。原全局512项／750us扫描保留兜底；64候选及overflow拒绝保持。world identity／generation变化清旧候选和游标。列表不被当进入许可。
- 将发现移至已验证空手Frame之后、rig gate之前；接触仍需完整rig／原几何／身份时效／连续边沿／原进入机制，未放宽Volume、输入或原权限。未加writer／hook／引擎调用，Native不改。
- 新冻结1305源，仅entry runtime delta，完整PG configure/build0/0；最终DLL 0ffe38c8f3617af27c0d692aafe5b58fcc9accf7f20d047bb309f82d3c5def92。实际生产discover_pawns函数体的9组离线游标检查通过：续扫、空链、读失败、自环／双环、256上限、generation切换、时间预算、候选拒绝；reader为stub，不能证明真实元数据读取／链成员／实时耗时。下一离线核真实reader反例。
- 证据analysis/troika-development-20261109/pawn-discovery-progress-20261110.json。未stage／启动game或XR；统一及本方hold保持，无Git index／分支写入。publicv71／25%／完整流程未通过／headsetfalse保持。用户所选目录已记录，不再以Steam安装问答作前置。

## 2026-11-10 场景链表实际Reader反例修复及overflow恢复

- 上轮游标测试未涵盖真实读取器。本轮用现有weapon_hand_offline_fixture的自有人工反射内存，实际生产copy／Reader／identify／retained_metadata及pawn_link函数体，先复现fault8字段offset读后变化仍接受；保留字段metadata后，又复现fault11目标Pawn.WorldInfo读后变化仍接受。原失败均configure/build0、执行1，日志和源SHA独立保留；不是实际55404因果证明。
- 仅本枪entry模块修复：显式retain链接字段；把当前节点／目标节点WorldInfo读取纳入同一Reader，显式retain各自字段并结束时重复读取实际值，未知／变化拒绝且不发布部分输出。原Native current gate保持，未改共享Reader或增加writer。新增当前节点跨World／目标跨World／读后变化／NextPawn字段偏移变化反例。42实际Reader检查通过；current broker为stub，不能冒称实际帧／游戏通过。
- 另提取实际生产discover函数体，以替代对象读取／候选满表边界复现overflow固定cursor0执行1；修正拒绝时推进cursor，overflow仍拒绝进入，原全扫描周期后才能重评。旧注释称继续推进但原代码return前未推进，现已实测补齐。overflow推进／完整周期恢复／每次512上限均通过，非真实对象表性能证据。
- 新1305源冻结pawn-discovery-reader-final-20261110，仍只entry runtime delta，完整PG configure/build0/0，SHA5446b4d7f537faed42423117394f08062aeaea04d14d6ae327f802d7026d1794。Native／XR／公共交付未改，旧候选原件保留。
- 证据analysis/troika-development-20261109/pawn-reader-repair-progress-20261110.json。未stage／启动game或XR；统一pause及本方hold存在并记录哈希，未Git index／分支操作。实际链成员／耗时／接触进入待核；完整逻辑／头显未通过，25%不变。

## 2026-11-10 保存55404计划点核算／实际接触观察候选已编译

- 实际生产contact头核算原55404矩阵与保存点：移动前实读掌距0.53345425m、不接触；计划surface距0.000019849m、接触；计划6cm近柄点距0.05769946m、不接触。另8cm假设身体偏移距0.07765341m，仅敏感性例，不能当实际敌人推移。原没有送手后样本，故计划有效不等于真实触碰／首败原因已知。测试configure/build/run均0，几何头与新冻结源逐SHA相等。
- 源链复核：nativeequipment_rig经同Scene／同Frame／同输入cohort与实际世界姿势重算验证，rig与parts已拒绝不合法尺度／非有限坐标／不刚性柄矩阵。未把不可达的假设几何失败冒称新实际缺陷，也未扩大触碰范围。
- 私有诊断宏新增troika_entry_contact行，直接记录本次既有contact计算的frame／scene／sequence／sync／rig时间、真实双掌和handleroot矩阵、四组距离／mask／valid。只在近15cm、距离变化至少1.5cm或mask／valid变化等时记录；每候选槽身份最多6行，总24行，正常无宏不输出。无第二次输入／几何读取、无新许可／writer，原判据及接触沿保持。
- contact-observer-20261110冻结1305源、仍仅entry runtime delta，完整PG configure/build0/0，SHA85dadcbc30be852412bb68f4abb82fa8a461b09cf298790a8cf6d307bc5e3644。未stage／实跑，Native／XR／public不变。统一pause与本方hold保持，完整动作及头显未通过，25%未变。证据analysis/troika-development-20261109/contact-evidence-progress-20261110.json。

## 2026-11-10 原退出关联完成确认收紧（离线复现／修复／编译）

- 原Engine.Vehicle.DriverLeave导出先恢复C.Possess(Driver)、调用Driver.StopDriving清DrivenVehicle，后才调用DriverLeft清Vehicle.Driver；Turret.UnPossessed另有StopFiring。静态顺序不当新游戏实测，也不证明所有native／中断路径。
- 原observe_departed只核世界／人物／控制器身份及pc.Pawn、pawn.DrivenVehicle、pawn.Controller就清本方ledger，没有确认旧turret已不再关联原人物／控制器。实际生产observe_departed+Reader+copy+identify在自有人工反射内存重现fault0提前接受，configure/build0、执行1，原日志保留。
- 只改本枪weapon_troika_original.inl：同一Reader验证原turret身份／WorldInfo；要求turret.Driver不再等于原人物、Controller不再等于原PC；六个pointer字段保留metadata并结束时重读，字段／值／对象身份变化及读取失败均不退休。允许不同的新Driver／Controller值，但不读取其对象或向其写入，不新增DriverLeave／Unclaim重试。单独owned R2责任及extinct=false保持，不用缺失读／新占用者冒称枪火消失。
- 33项离线检查通过，覆盖半退出、残留控制器、字段读失败、身份／World变化、读中值／metadata变化、错误token／binding及新占用者不变。自有人工内存不替代实际游戏退出。
- departure-reader-20261110保留上轮接触观察候选1305源，仅original entry adapter delta，完整PG configure/build0/0，SHA12dbef42d01564d4ba1f9902bee03ebcec36c24c909f391502840344d258f16c。未stage／运行／改Native或public，统一pause及本方hold保持，无Git index操作。证据analysis/troika-development-20261109/departure-repair-progress-20261110.json。完整流程／头显仍false，25%未变。

## 2026-11-10 换人物／新场景账本退休的读取竞态修复

- 实际生产retire_for_context_change+Reader/copy/identify，在自有人工反射内存复现同World换人物时，turret.Driver首次读为null、读后改回原人物，旧代码仍接受退休（fault11，configure/build0、执行1）。只证明离线反例，不称原游戏故障已复现。
- 仅本枪original entry adapter：同World的旧Driver与WorldInfo使用同一Reader，保留两字段metadata并结束时复读实际值；所有分支清ledger前再次nativeequipment::current验证新上下文。保留原game/native身份、world generation单调、同World必须换人物且lifecycle推进、新World必须更高generation；新World仍不读取已废旧炮位。未知／读失败不是对象消失证明，不新增旧对象写／退出重试／Unclaim。独立owned-fire及extinct=false保持。
- 39换场景检查0，覆盖原人物／life未变、generation回退、PID/start/native错配、旧Driver、读取失败、值／字段竞态、新上下文失效，以及新占用者不变、新World不访问旧炮位。Native current broker为stub，不称实际新Frame验证；同文件33退出读取检查重跑0。
- context-reader-20261110保留1305源，仅original adapter delta，完整PG configure/build0/0，SHAa27d1d6a75ab79d7170ee0680bd7d70efc2b6bd2bf85fba5c2305b590b4197c9。未stage／启动game或XR／改Native或public，统一pause及本方hold保持。证据analysis/troika-development-20261109/context-repair-progress-20261110.json；完整动作和头显未通过，25%不变。

## 2026-11-10 停止射击按钮读回的字段绑定修复

- 生产buttons_up函数体及实际Reader/copy/retained_metadata在自有人工反射内存复现：两次同Reader按钮检查间，bFire offset从原字节移到另一零字节，原字节置held，旧代码仍认buttons-up。configure/build0、执行1，原日志／源SHA保留；没有据此声称实际Backend或游戏故障复现。
- 仅本枪weapon_troika_fire.inl：同Reader显式保留bHoldingButton_R2／bRawHoldingButton_R2／bFire三个property描述，结束时复读两bool mask与目标bits、bFire byte并复核metadata；quiescent最后一次原GetStateName之后再buttons_up。字段或读中目标值变化拒绝，其他按钮同字节位变化不误当R2。原PendingFire／IsFiring／两次Active或Inactive状态一致及exact-resource检查均保留。
- 26按钮Reader离线检查0，覆盖字段位移、真held/raw/fire、读取失败、读中位／mask／offset变化与非目标button bits；原引擎getter没有执行，不能冒充完整停火／伤害／声音停止。无bFire写、直接EndFire、额外release重试、重签输入或新writer；extinct=false与未知责任保持。
- fire-buttons-reader-20261110冻结1305源，只有fire adapter delta；完整PG configure/build0/0，SHA7df00a7a59eda1687f0e945c17a9cfd427f0545ef8286e15ee65f35f5e7b4b9b。未stage／game／XR／Native／public变更，pause与本方hold保持，无Git index操作。证据analysis/troika-development-20261109/fire-buttons-repair-progress-20261110.json；全流程／头显false，25%不变。

## 2026-11-10 原停火查询语义与参数布局补齐

- 原Engine.Weapon.IsFiring基础实现返回false、WeaponFiring实现true；Weap_Troika WarmingUp有单独实现。PendingFire在InvManager为空时也返回false。故保留稳定Active/Inactive、按钮up、PendingFire／IsFiring及exact-resource组合，未将单个false等同完整停火。原EndState清WarmedUp／RefireCheckTimer为静态证据，不冒充实际计时／声音／伤害结束。
- 原result_property宽度／类型／边界检查没有独立覆盖return owner、Parm/ReturnParm、dimension、原参数区／return offset和PendingFire输入输出不重叠。本轮在本枪fire adapter新增query_layout并接入三查询及结束复核：以实际metadata验证这些条件，遍历有界函数fields拒绝循环／额外未绑定参数，允许合法参数padding及输入输出次序交换，不硬定PendingFire为紧凑8字节。原查询调用数量／输入FireMode0及原机制保持。
- 新helper实际Reader/copy在人工反射内存39检查0，含合法返回布局、逆序／padding及错误owner/flags/dimension/重叠/param size/return offset/额外参数／读取失败／metadata变更。原引擎查询未执行，Call包装为替身；没有新真实getter布局或停火证据。同file按钮26检查重跑0。
- fire-query-reader-20261110冻结1305源，仅fire adapter delta，完整PG configure/build0/0，SHA71ed815fa760702d043309bc6ab850673f1524a6a10e57c2bada0271b740a32c。旧候选保留，未stage／game／XR／Native／public变更，pause与本方hold保持。证据analysis/troika-development-20261109/fire-query-progress-20261110.json，完整逻辑／头显false、25%未变。下一离线将明确命名的最新候选接入本方prepare选择，避免恢复后误取旧627观察候选；不创建运行claim或绕过暂停。

## 2026-11-10 最新私有候选显式选择已离线接通

- 本方troika_isolated新增只读candidate命令；prepare必须明确选择版本，禁止省略后回退原PG。当前flow-71ed815f固定最新71ed815f…，旧entry-observer-62753098保留，原--entry-observer只是显式旧版别名；同时传新版选择与旧别名立即报错。新候选目录按实例名隔离，不覆盖旧输入目录。
- 每候选固定result／source-inventory／DLL SHA；只读检查逐项核1305源文件、路径边界、成功构建与DLL身份。两个候选均检查0，未创建实例。准备时先验证身份再产生输入，未来troika-binding同时写候选名／构建与源清单摘要；已有绑定的sources读回会核候选名与PG SHA一致。Native／XR选择和旧实例原件保持。
- 实际CLI省略选择／冲突选项／未知选择均exit2；实例与候选输入目录项前后完全相同。仅AST核run_owned统一pause与本方hold仍在sources／运行claim之前且均return；没有执行run入口探测暂停。两个暂停文件SHA未变。
- 证据analysis/troika-development-20261109/candidate-selection-progress-20261110.json。无prepare／stage／game／XR／Git index操作，尚未实际验证新候选任何动作；完整流程／头显false、25%不变。

## 2026-11-10 全逻辑脚本补左右Grip中断／持R2再抓分支（只源码未执行）

- 源核实际覆盖缺口：旧exercise先松R2才松双Grip，未测持R2松任一Grip、再抓不自动射、R2新沿恢复。另旧最终both_grips_release_no_fire只记True未检查ShotCount，现补实际不增断言。
- 原受保护Source1脚本新增左右各一真实分支：双握先产生原射击→保持R2松指定Grip→读原GetStateName/PendingFire/IsFiring与ShotCount直到有界稳定→R2不变再Grip，并必须实际瞄准响应证明重新双握成功→按只读原WarmupTime+.4s观察无预热／发射／新shot→真实松R2再新按要求原4shot。未改游戏计时／资源／输入authority，无直接进入／开火／退出调用。延迟样本保留，不把3秒有界停止当立即或零延迟验收；音效／伤害及手感单列未通过。
- 修正旧状态行证据标签：gun和C0各自ready状态变化日志不是同publication／当前再抓证书，改为gun_and_C0_ready_log_rows_seen及明确limits；实际开火仍由原生产consumer严格门控。
- 只做Python编译与AST原Bridge只读getter白名单、左右分支和future prepare源复制/SHA接线检查；没有运行exercise、模拟动作或创建实例。Native/PG及flow-71ed815f未改、不重编。暂停与本方hold保持，完整流程／头显false、25%不变。证据analysis/troika-development-20261109/grip-release-chain-progress-20261110.json。

## 2026-11-10 动作返回类型与异常清理修复／按最新主人要求恢复CLOAK入口

- 源核runtime.Bridge.call确认BoolProperty直接返回bool；上一轮新增original_fire_state却bytes.fromhex解析，已在旧源提取函数+符合实际接口的布尔替身上复现TypeError。现直接接收bool、拒绝未知类型；旧源、失败及新接口检查保存。未隐瞒为游戏故障：旧新增分支尚未执行过游戏。
- exercise finally实际函数体以自有故障回调复现／验证：左手发布报错后仍尝试右手、复制日志和保存，原动作异常不被清理异常覆盖；无主异常则清理失败明确报错。父链也逐步独立尝试双手释放、原X一次、两个句柄关闭和保存；X请求在发布前记账，不在未知结果下重复按退出。没有新原游戏写入或强杀。
- 最新主人“解决后台后继续跑”已由统筹送达，实际读最新独立入口规范；中央pause已归档，本方仅等待后台修复的hold归档为isolated-startup-hold-resumed-20261110-193840.json。改用正式cloak1110，自己的监控核DWM CLOAKED及foreground，禁止解隐藏；改用原私有GPU合成帧录制并延后encode，不用旧可见窗口／桌面捕获判运行。
- 新prepare troika-cloak-1110-193950成功，明确flow-71ed815f／Native3ac0bc7e／XR16840b27，scale.4。私有源／helper／动作编译检查0，尚未创建action claim。首次run在require_memory被实际双实例bow-1110-193626、snub-20261110-193543拒绝，尚无run claim／launcher／game；不是动作或CLOAK失败，不抢别人、不标blocked。可待资源释放后使用同一未消费prepared实例。
- 证据analysis/troika-development-20261109/action-cleanup-cloak-progress-20261110.json。完整动作／头显仍false，25%不变。遵守不向统筹汇报／占用／释放消息，独立继续。

- 后续同轮补清理失败必须在保存前撤销既有passed标记，实际helper故障注入已验证。为冻结这一新动作源重新prepare全新troika-cloak-1110-194116，旧193950未启动原件保留；prepare/check均0。再次资源检查仍在claim前拒绝，此时实际占用为gnasher-1110-194056-c14与grenade-smoke-1110-194102，非前两实例仍被误认。当前两份本方prepared均无launcher／run claim；后续复用最新未消费194116，不因纯资源等待反复prepare。运行已获恢复授权，不再写回暂停。

## 2026-11-10 CLOAK194116实际game1688首败：保留1.3m手工作空间拒绝

- 资源释放后才启动未消费194116，worker29008／owner47232／game1688／XR58944均本实例确切身份。真实Source1、原Playground／Native和epoch1投影到达；未解CLOAK、未使用可见桌面录制。905前台采样无本方违规，125自有可见样式窗口DWM CLOAK读取，未捕获uncloaked；仅采样结论。
- 原导航从274到93.73单位进入测试站位100范围，实际before-contact源3326读到后，计算near/touch raw位置触发原1.3m head工作空间断言；未送手／触柄／请求进入／执行双握射击分支，没有Grip/X/直接进入补救。原最后body位置与手源相隔约.53秒，敌人接近／遮挡可见，不据此宣称唯一推移／映射因果。新typed-list成功亦未证，早期有界gate16均discovered0不是所有帧证明。
- 正常本方stop0、launcher0、Native恢复／entries／pad恢复、XR0，旧owner/game/XR实际均消失，13私有原件精确恢复；单独unified-weapons drained=false（fire_disable2、hand_stop5）保留，不冒称所有consumer清理通过。无强杀、无他实例修改，原run action_exit1但父清理命令正常退出不当动作成功。
- 354帧39.30s私有GPU双眼合成录制error0，正常释放实例后encode0。全跨度24接触图及原177/300/353帧亲自看过：大半启动等待卡，末段真实场景、自由手臂、敌人贴近遮挡；不算接触或全角度fit通过。证据analysis/troika-development-20261109/cloak-194116-first-failure-review.json。下一先保存断言前计划诊断并核身体/输入映射和候选扫描，不放宽1.3m／Volume或复跑不变计时器。完整流程／头显false、25%不变。

## 2026-11-10 Workspace195452实际game58248：生产触柄沿已到，TryToDrive前拒绝

- 新同候选原隐藏实例worker14672／owner58044／game58248，真实cohort body前后相同(-3719.5139,1.3174,651)，两手映射残差0；near1.013877m／touch1.044079m均在原1.3m内。近柄不进入检查通过，实际发送空掌轨迹。原目标handleroot与55404一致，不能把1688超限泛化为恒定轴错误。
- 新生产诊断证明原Turret已被发现，实际连续距离降至.012948m，下一frame167/input2870形成contacts1、token1/binding1请求；原entry result0/rejected、dispatched0/owed0，TryToDrive尚未发送。CanEnter是否调用／返回及哪条前置失败目前未分辨，不能宣称原因已修好或放宽100ms。原同次接触未重试，双握／射击／退出动作未到。
- stop0、Native／entries／pad恢复、XR0、原owner/game/XR均消失、13私有原件恢复；unified-drained=false/fire_disable2/hand_stop5保持独立失败。903前台采样／131自有DWM CLOAK读取无捕获违规，仅采样保证。355帧41.69s私有GPU录像error0，先释放实例后encode0；全跨度24图及300/354原图亲看，敌人贴近重遮挡，不以画面猜掌面；早段等待卡保留。证据workspace-195452-first-failure-review.json及完整product log。
- 新original entry仅把原严格短路组拆为24个拒绝原因，私有宏失败后最多12行记录frame/input/token、原输入年龄／窗口剩余／请求年龄；不重读权限或续签、不改变原调用顺序／防重放。初版编译因局部重名／shadow失败，保留；final1305源仅original adapter delta，完整PG0/0，SHA53131db3382364c5f3890febb065b6a1fc5446960f66091981055af8c7eaf60a。显式candidate entry-reject-53131db3已接，尚未实跑；Native/public保持，全流程／头显false、25%不变。

## 2026-11-10 Entry200459实际game32652已定位原CanEnter=false，非100ms首败

- 新53131db3候选实际触柄并请求token1；有界拒绝日志reason20，原CanEnterVehicle正常返回false，TryToDrive未发。输入年龄34.0845ms／剩余65.9155ms／请求年龄2.9215ms，不能再称这次首败为输入超时；其他准入子条件仍未证明。
- 对照原CanEnter脚本含VSize(P.Location-GetEntryLocation)与EntryRadius、原载具门及非掩体。旧测试navigate仅以水平100返回；此前具体body点水平93.5193但3D102.5176，若radius100则越原范围，实际radius尚未读取，不能先宣称此为已证因果。源已改为读取实际EntryRadius并在其三维范围内停步（不扩大原范围），同时只读记录蹲伏／掩体／控制器／Driver／DrivenVehicle／Health；未知可选诊断不授予任何动作，1.3m／触碰／原准入保持。
- 真实32652本方进程全消失，13私有原件恢复，Native/entries/pad／XR0；unified-drained=false/fire_disable2保留，hand_stop5实际枚举为quiescent，不能把它误称hand故障。1124前台采样／165CLOAK检查无捕获违规，383帧54.51s私有GPU录制error0；释放实例后encode0，全24图与382原帧亲看：启动等待卡、真实双眼及敌人重遮挡，未通过fit。
- 本方runner新增使用原sixdof Session.close在私有输入／Native回调尚在时先请求既有consumer正常收尾，后仍走共同stop；不改共享工具，不强制忽略owed或drained。此调整尚未实测，不宣称修复。
- 已prepare/check全新troika-radius-1110-201348，私有动作源与原半径读取／3D站位一致，候选仍53131db3；资源先后被真实双实例占满拒绝，未消费claim／launcher。最新占用cover-1110-r120-cloak-source-stage、hod-1110-202611；不抢占、不向统筹发消息，下一复用未消费实例。证据entry-200459-first-failure-review.json；完整流程／头显false、25%不变。


## Radiusfix54792实际原范围与近柄后重新采样（2026-11-10）

原PG53131db3/Native3ac0不变。实际GetEntryRadius属性为100；送手前人物三维距离99.29183，失败后180.69757，原准入布尔观察未发现蹲伏/掩体/已有占用，生命100。本轮没有生产contact/request或Can拒绝记录，不能归为reason20再次发生。122帧/13.8255秒实际合成双目GPU录像正常离线编码，24张全跨度概览已亲自查看；敌人约6–9秒贴近遮挡双手。不能由此端点记录证明位移唯一原因，也不能作握持/完整玩法验收。

本轮pre-teardown和shared消费者清理均fire_disable0、hand_stop5、drainedtrue；Native/entries/pad恢复、XR0，精确旧身份消失，13份私有文件原样恢复。未强杀。前台984次/CLOAK146次无记录故障。证据analysis/troika-development-20261109/radiusfix-204602-review.json。

动作脚本已修正近柄后仍沿用移动前raw/world换算的问题：近柄后检查实际原准入范围，重读原骨矩阵及同源双掌换算，保存人物前后括号，再生成触碰目标；发送前再次检查原范围。若站位丢失，明确记录夹具接近失败并停止触碰。失败触碰后保留实际raw/world读数。原100距离、1.3m手域、原CanEnterVehicle/TryToDrive、接触token/100ms全部不变；没有直接进入、瞬移或准入旁路。源解析及新实例变换后脚本编译检查通过，新独立实例troika-refresh-1110-next01的实际结果待定。完整逻辑→Snap/Volume→GrabPose及物理头显均未验收。

## Refresh40340与私有clear-entry候选（2026-11-10）

新脚本实际复查game40340：before_planned_contact约99.23，after_near_before_touch_refresh为180.45184，原EntryRadius100。真实读数使脚本在near后停止，没有发送旧换算touch，也没有把该轮当原Can拒绝。99帧/10.83246秒实际合成双目已编码0并亲自查看24张全跨度概览，敌人贴近及红色冲击效果可见，不能由此认定唯一位移因果。原消费者提前清理0/5/drainedtrue，shared恢复和旧身份/13份文件恢复见refresh-next01-review.json。

冻结源码发现原Lancer melee练习靶在(-3650,0)，接近原Troika入口(-3625.997,2,609)，和测试接近位置相邻。新私有候选clear-entry-5611518e仅在显式GEARS_TROIKA_CLEAR_ENTRY_FIXTURE宏下将该靶使用原标准target-zero位置(-2850,-350)。原靶controller/AI/health/collision不改，其他目标同原标准布局；不触及其他武器或共享源码。此场景用于关闭基本操作链，不能计为受敌干扰验收。单文件playground_controller.inl差异，1305文件完整冻结，配置/编译0/0。Native/XR不变，PG SHA5611518eb8cc0c149b0e955fe2eb2ce47382471ce6ed85735ea4799c0398a384。新独立troika-clear-1110-next02已开始，实际结果待定；公共v71未改。


## Clear-entry55204：真实占用成功，mounted源/排空仍失败（2026-11-10）

私有PG5611518e场景下near不进入，refreshed空掌接触形成生产gate2 contacts1/token1/binding1/result2/dispatched1/owed1；原PC.Pawn=Troika、Troika.Driver=原Pawn、原Pawn.DrivenVehicle=Troika通过，未请求Grip/button进入且未自行开火。基本进入链有真实证据，不能因此称完整Troika完成。

首败为双握后gun/C0就绪记录未同时出现，未发送R2。实际投影在占用后退回黑底wait card，session记录source_reconfiguration拒绝，final Native driver_ticks=0、C0_attempts=0；目前只定位到mounted camera/source或回调准入方向，未证实根因。不能把原session passed=true当玩法通过。

清理请求原X一次，未观察原角色返回；消费者提前清理fire_disable2、hand_stop4、drainedfalse保留。后续launcher正常终止，game/XR/owner旧身份消失、13份私有基线恢复，未强杀；具体Native/entries/pad/XR状态见clear-next02-review.json。159帧/17.38518秒合成双目GPU编码0、24张全跨度概览亲自查看；约10秒后为等待卡，不能称mounted画面/手型验收。公共v71不变。

下一步检查真实占用后的camera/source切换与唯一driver Tick/C0准入，不重跑相同entry、放宽原距离/100ms，或增加第二姿态写入。完整逻辑→Snap/Volume→GrabPose、原X退出/消费者排空、敌情干扰及头显验收仍未完成。


## Mounted视图选择顺序修正（2026-11-10，实际复查待定）

冻结Native源码中，body_view_required检查位于troika_view_live::prepare之前；mounted状态下普通Pawn/cover/body视图会退出，因此可能在专用视图准备前返回。私有单文件修正先调用原Troika视图验证，失败照旧拒绝，成功才加入第一人称就绪判断；排除后续camera-closer修改该已拥有的视图，不重复准备。没有放宽身份/占用/reference/时效检查，没有改camera/actor/输入写入或新增姿态writer。

1212文件冻结清单重新核对，ON/OFF配置与编译均0/0，沿用原构建选项及原有警告。ON Native793357c62d553e74a314eb7b60ff83c55d45b79edede302061004fa80173fb41；OFF cc37d238abc7ad5217ce5a1ccd3c8713324d92e845431b3b98d2ee69659468b5。PG5611518e、XR16840b27不变。以显式Native候选准备新独立troika-view-1110-next03，完整1212源与制品身份核对、变换后动作/读者解析编译通过。实际结果尚待，不将构建成功当mounted画面/手型/射击/退出通过。证据mounted-view-order-progress.json和私有patch。


## View-order53508实际复查仍未通过（2026-11-10）

Native793357c6/PG5611518e实际game53508：步行时默认前伸双掌先接触真实shaft，产品gate2/result2/dispatched1/owed1，原进入再次成功；测试navigate仍要求旧Pawn因而AssertionError，尚未到计划near/touch及双握。该轮不作视图修复验收。69帧/7.56087秒合成双目编码0，24张全跨度概览亲自查看，约5秒后仍黑底等待卡；保留失败，不称原顺序修正已解决全部mounted问题。

原X收尾未观察成功，提前consumer2/4/drainedfalse，原session收尾结果与精确旧进程/13文件恢复见view-next03-review.json。正常停止、无强杀，完整玩法及物理头显仍false。

动作脚本新增步行前将双掌通过普通Source1平滑移到身体近侧，避免默认前伸掌先碰shaft；不屏蔽产品接触，不改距离/原Can/100ms。next04准备时验证Native1212/PG1305完整源与制品，变换后脚本检查通过。资源预检已有两个worker，启动在own claim前拒绝，没有新game，不计玩法/渲染失败。下一步仍需以新证据定位mounted mapping/view未就绪，保留视图顺序修正及全部旧失败。

## Mounted映射/视图分支诊断（2026-11-10，next05实际待定）

在已冻结Native793357c6上新增私有只读诊断：原TroikaMountedSpace各短路拒绝保留原顺序及原条件，标记23种失败原因；Scene仅统计实际map尝试/成功/缺少进入basis，以及原view准备的5个拒绝分支。通过现有status输出原子计数，不额外读actor/输入、重选源、续期、放宽身份或增加writer。4文件差异，1212源清单完整核对。

现有entry-fixed mapping离线242检查在新头文件下/W4 /WX编译执行通过；包括旧发布重放、100ms过期、owner/native/driver/space/reference/publisher变化的拒绝及fresh恢复。这是合成数据结果，不等于原engine/C0/fire。新Native ON/OFF配置编译均0/0，沿用原基线构建警告；ON755b27a1f69fa52f6c3ad9e054dcd542320f3a01a8a8bbc5a1c08d779fa2864c，OFF ba4fde6f26a82bda7edbf88aa9932f50b30deffc6955661da41d2f636dc50ebc。PG5611518e和XR16840b27未改。

next05新实例使用此显式候选与步行近身手位，源/制品和变换后脚本检查通过后启动。next04只有资源预检拒绝、无game，未复用或误计为失败运行。next05实际结果待定，完整逻辑→Snap/Volume→GrabPose、mounted画面/双握/原射击/原X退出/consumer排空仍未完成。


## Map54760精确首败15与reference交接修正（2026-11-10）

Native755b27a1/PG5611518e实际game54760：近身步行双手避免先触柄，near不进入、明确空掌触碰后原占用成功且无未请求开火。双握后仍缺gun/C0就绪，未发送R2。Native诊断map_missing_basis0、map_successes0、map_last_failure15(reference_generation)，view_last_failure2，driver/C0仍0；具体最终计数见map-next05-review.json。213帧/23.20242秒实际合成双目编码0，24张全跨度概览亲自查看，约15秒后等待卡。原X未观察成功，提前consumer2/4/drainedfalse；最终正常Native/entries/pad/XR和进程/13文件恢复见同证据，无强杀。

源码原因对应StereoGate.advance_world对所有world-rebind请求recenter，原进入同世界Troika也走该交接，fresh样本增加reference_generation；Troika固定进入空间仍正确拒绝不同参考代次。不是删掉代次检查。

新私有修正保留全部旧输入失效和最小fresh样本截止：仅在pending原占用guard已认证、同world/local/controller、old Pawn=retained Driver、new Pawn=retained Troika、new Driver=同原Driver，且Native原entry owner/cookie及稳定参考完全吻合时，Native commit为该mount一次性保留heading/reference generation并增加连续性epoch。固定空间仅在该arm的精确下一world generation允许一次epoch+1接续；reference pose/generation、LOCAL space、publisher、identity和100ms保持原拒绝。所有其他world/recenter走原路径。无新action lease/输入续期。

新263项mapping检查及原完整stereo frame/gate/matrix/bridge测试通过，包括未arm拒绝、错误owner/world/epoch、reference/LOCAL变化、重复arm/旧发布拒绝和fresh恢复。首版新增测试局部名original触发/WX shadow，保留旧散列和失败记录，改为fixed_initial后编译0；不是隐藏生产失败。6文件私有冻结，Native ON/OFF构建进行中，尚无实际验收。完整交付沿用25%，不为诊断加分；full/headset仍false。

Reference-handoff候选ON/OFF配置编译均0/0。ON815ece03b1e12e569c5ef8fe01b6b98ae42cf822d3fbf4828ed71b42a9769952；OFF63f1df1d02f6b5eaa3f52f74cddf0b5a6f2a8c1d42b98a7c213cd7e9457451b0。独立next06以PG5611518e/XR16840b27和同一近身手位执行，1212/1305源及制品hash核对、变换后脚本检查通过；实际结果尚待。不因构建／合成检查加分或称画面/双握/原X通过。

Reference-handoff首个实例next06/game37392在start_vr_gamepad→background_input.change(enable)→FrozenThreads.OpenThread报WinError87，未激活Native、无原场景/动作。此前已冻结线程由原exception/__exit__恢复；XR1、launcher1保留，不算候选动作失败或通过。worker正常关闭本方game，无强杀，精确旧身份消失、13私有文件恢复。错误位置与已有_restore_gamepad_gate记录的短寿命线程枚举/退出竞态一致，缺少该TID寿命证据，不声称唯一原因。共享工具未修改，未跳过线程/冻结/字节检查；只用全新next07进行一次首次候选动作复查。证据ref-next06-startup-failure.json。


## Reference32776：Native映射/view准备219成功，PG/C0/退出未通过（2026-11-10）

Native815ece03/PG5611518e实际next07/game32776：near不进入、明确空手触柄原占用与无未请求开火通过。Native map_attempts219/successes219/last_failure0，view_attempts220/successes219/last_failure0；原参考代次拒绝已消除。driver_ticks和C0_attempts仍0，双握gun/C0就绪失败，未发送R2。原X仅请求一次但未观察退出，提前consumer2/4/drainedfalse保留；最终Native/entries/pad恢复、XR0、精确旧进程/13文件恢复且无强杀，见ref-next07-review.json。

170帧/18.54170秒实际合成双目编码0、24张全跨度概览亲自查看：正常接近/触柄炮体，末段有第三人称身体；随后录制报No complete fresh projection frame。不能把Native view准备成功当持续真实投影或头显通过，原录制错误单列保留。

源码发现PG自己也保留TroikaMountedSpace，5611518e的旧头仍严格比较entry native_reference.epoch，未接入新Native对同world原mount认证的一次连续性接续。即使Native map成功，PG对应map到达时仍会拒绝新epoch。更早input/geometry条件是否还会拒绝尚未验证，不认定唯一首因。下一修正PG只读接续：保留Native实际源/原mounted owner认证、原reference pose/generation/LOCAL/publisher及100ms，禁止自动重放旧Grip/R2，不改Native已通过部分或做不变timer运行。全交付25%沿用，完整逻辑→Snap/Volume→GrabPose和物理头显均未完成。

## PG侧原进入参考接续候选aa48b03f（2026-11-10）

在PG5611518e冻结树上，仅更新TroikaMountedSpace为已通过Native同版内核，以及weapon_troika_tracking.inl保存原entry world/reference并确认一次交接。只有原mounted owner/token/binding仍一致、原map精确停于reason17（此前reference pose/generation与world/session/space/driver/mesh检查已通过）、Native原basis曾确认保留、世界代次精确+1/连续性epoch精确+1、当前已钉住Native源读回与原mounted context均吻合时才arm；map成功后再关闭一次同源验证。只读数据连续性，不发Grip/R2，不续原时间戳，不转移旧lease。失败仍保留原拒绝，后续press也使用同一更新后的空间内核。

1305完整冻结源，2文件差异，PG配置/编译0/0，SHA aa48b03f4e9c24b664fcd911394b139f6fb761804b187ee88f6204455220bfc4；Native815ece03/XR16840b27不变。新独立next08源与制品检查、变换后脚本编译通过。首次资源申请因实际两个worker存活在claim前拒绝，没有新game；保留实例等资源，不占他人或新增排队流程。实际接续/双握/C0/原退出/排空仍未验收，交付25%沿用。


## PGref25696真实接续成功，parts2仍拒绝；原pitch类型修正（2026-11-10）

Native815ece03/PGaa48b03f实际game25696：空手明确触柄原占用和无未请求射击通过。PG仅一行fixed_reference_handoff previous_world1/next_world2 old_epoch2/next_epoch3 ready1，随后双手mapped tracked3；原PG连续性缺口已接续。炮体parts stage2(limits)未就绪，driver/C0为0，双握就绪失败而未发R2，原X退出未观察成功，提前consumer2/4/drainedfalse。正常Native/entries/pad恢复、XR0及精确旧进程/13文件恢复见pgref-next08-review.json，无强杀。

250帧/45.71884秒实际合成双目编码0，24张全跨度概览亲自查看：接近/触柄近炮体，后段有第三人称身体；不能称mounted first-person/手型/瞄准/射击或原X退出通过。录制自身错误保留在review。

原导出Engine/objects.tsv的21699/21700明确为FloatProperty Engine.Pawn.ViewPitchMax/Min；旧parts读取错误要求IntProperty。新私有单文件修正Observation/closing读回为float并要求FloatProperty4，保留值，不转为int；有限且min<max，原YawLimit及原夹紧函数不改。原声明证明错误条件，尚不能排除stage2中更早Yaw/Rotation/字段条件也失败。1305源冻结，配置/编译0/0，PG3fd5afd4bf6e8abb074e22f261467f8771b08cbefb41ee62315a6616059d53d4；Native/XR保持。next09已准备/源及脚本检查，资源申请在claim前因两个实际worker拒绝，没有新game，等释放继续。完整25%沿用，full/headset未通过。


## Pitch57952到达names3，局部扫描预算修正（2026-11-10）

Native815ece03/PG3fd5afd4实际game57952：原空手进入和PG参考交接保持，通过原limits读取并首次到parts stage3(names)；随后双握gun/C0未就绪、未发R2。stage2/0也间歇出现，不声称其他同源时效条件均关闭。原X未观察退出，提前consumer2/4/drainedfalse，最终正常Native/entries/pad恢复、XR0及旧进程/13文件恢复见pitch-next09-review.json，无强杀。

213帧/25.95366秒合成双目编码0，24张全跨度概览亲自查看；有等待卡与第三人称身体，持续mounted first-person/手型/射击/退出仍未通过。

源码发现names的1ms扫描预算在Lookup.step初次current验证前开始，两组Lookup共用同一开始时间，验证或第一组耗尽预算后可能零进展。私有单文件改为每组查找独立预算，第一次扫描时开始计时；原Lookup不改，仍每步最多2048项、扫描1ms、末尾同源关闭复核，过期/表变不提交游标，原100ms/身份条件保持。使用原Lookup及从生产文件提取的实际helper编译执行16项，重现旧cursor0、新cursor16进展、第二组独立预算、过期/表变丢弃；这是合成证明，不是本游戏游标已实测或唯一原因。PG2e3fb0c81f6a07ce91eb764693d13a35ff748078167205cba01fdeb84401ca48配置编译0/0，1305冻结源校验，新next10已准备；Native/XR不变，实际待定。


## Names53728仍names3与精确游标观察（2026-11-10）

Native815ece03/PG2e3fb0c8实际next11/game53728：原空手进入、PG参考交接保持，parts仍stage3而未到bones4，双握gun/C0未就绪，不发R2。扫描预算修正的合成16检查有效，但本游戏没有记录实际游标，不能宣称已修复名称阶段或把延长等待当修复。原X未观察成功，提前consumer2/4/drainedfalse，最终正常Native/entries/pad、XR0、精确旧进程/13文件恢复；225帧32.23636秒合成双目编码0、24张全跨度概览亲自查看，仍有第三人称身体，不是first-person/fit验收。详见names-next11-review.json。

新PG0b1fd7bf366d02b7d541362adb36a14667637c2b1650f142f0d438914e89a56d仅在原parts名称查找已返回后记录state/cursor/found/已读table边界，最多80行；没有新读取、动作权限或预算修改。1305源冻结，配置/编译0/0。新next12通过源与制品/脚本校验后启动，实际结果待定。

next10在共享start的4GiB磁盘检查处拒绝，未创建worker/launcher；保留原失败pending回执，另以13文件全部未变和检查发生在worker创建前关闭“未启动”证据，不能计gameplay失败。随后本方也在claim前调用相同磁盘守卫。仅清理本会话已完成build中可重建obj共863803456字节，保留源、DLL、日志、媒体并再次通过1212/1305源/制品校验；未归档或删除源码。详见names-next10-disk-refusal.json及两份own-*-cleanup记录。公共v71和完整25%不变。


## Namesdiag37952两组名称均ready（2026-11-10）

实际next12/game37952 Native815ece03/PG0b1fd7bf：原空手进入/PG参考接续保持。GNames表data0x67b0000/count63203/capacity100000；pivot游标到59052、barrel到59057，各found7/state1 ready。中间源过期state2未提交该步，既有游标保留。此实际证据关闭“两组名称扫描未完成”的判断；不是扩大100ms或增加等待。

枪体/C0仍未就绪、未发R2，原X未观察退出、consumer排空false。旧parts日志8行被早期2/3/0切换耗尽，不能以没有bones4日志断言从未进入该阶段。218帧23.83914秒合成双目编码0，24张全跨度概览亲自查看，后段身体近距离遮挡；不算first-person/fit/完整流程。正常Native/entries/pad、XR0和旧身份/13文件恢复详见namesdiag-next12-review.json，无强杀。

新PG772ee6338c7d57699866b7752bbab2a84f8d6619cdb402331c4d7c848c7d3562仅将原read的26个false拒绝位置记下冻结源码行，按不同位置各最多2行、总最多40位置；不额外读取或改变原谓词顺序/权限。1305冻结源、单文件delta，配置编译0/0；next13准备/检查通过，资源在own claim前拒绝，等实际worker释放继续。Native/XR与动作计时均不改，全25%不变。


## Partsdiag38988已到关闭复核：100ms内无完整结果（2026-11-10）

Native815ece03/PG772ee633实际next13/game38988：名称两组完成；原read可进入closing6，line238拒绝时source published年龄100859200ns，另一98944200ns，保留骨骼原getter/current分组170/179和分配138等失败。至少一条已明确超过100ms；不能将所有分组都归为同一原因或忽略其他字段。双握gun/C0未就绪，不发R2；原X未观察退出，consumer排空false。正常Native/entries/pad、XR0及旧身份/13文件恢复见partsdiag-next13-review.json。

新私有PG09a65ace1817bfaea870624f64ded96e3fd81da6fe7206e178484f4fa71cb74c把明确的握柄接触callback改为geometry_only读法：仍核完整原mounted context/Native当前源/资产/左右分配/原MatchRefBone/GetBoneMatrix/handleroot9与关闭复核，只不读取该几何响应完全不用的Yaw/Pitch、其余5矩阵、枪口/physical fire和控制夹紧信息。所有aim/fire消费者仍默认完整读法；薄观察标记geometry_only，不打印完整原几何成功，不授C0/fire许可，不扩大100ms/预算/身份。两文件delta、1305冻结源，配置编译0/0；Native/XR不改。

本方fixture先移动空掌到实际shaft，再用原3秒预算观察geometry_only callback ready日志，然后发全新Grip边。日志仅安排动作，不是fresh action证书，实际原Native几何/按压窗口/lease仍决定，禁止提前按下旧Grip后等待自动重放。next14准备/检查通过，实际待定；full/headset/25%不变。


## 接触几何实际ready，完整机构仍未就绪（2026-11-10 23:49）

next14/game44812 Native815ece03/PG09a65ace在实际原触柄进入后，geometry_only读取达到stage7/ready1（scene359/360，published年龄56.5897/58.4502ms），fixture随后发新Grip。完整controls读取仍在source/assignment/name组拒绝（例如scene359年龄83.7177ms），gun/C0未就绪，不发R2。不能把contact ready当完整机构或握持通过；原退出X未观察，consumer drain false。Native/entries/pad、XR0正常收尾，原绑定消失/13私有文件恢复，未强杀。234投影帧25.552861s，24张全跨度样本已亲自查看：15.702s等待卡，后段第三人称身体遮挡仍存在；完整视觉和头显未验收，25%不变。详见contact-next14-review.json。

下一候选仅在Native Scene的prepare/commit回调中使用现有TickScope解析暂存；普通PlayerTick已有scope，而这些回调可在其外执行。暂存仅name长度边界/元数据遍历scratch，不缓存对象、字段值、输入或许可，所有原检查与100ms保留。


## callback解析scope不足以推进完整机构（2026-11-11 00:01）

next15/game33316 Native815ece03/PGb0fca9df实际接触几何ready继续成立；完整controls读取仍拒绝（source/assignment/name组），gun/C0未就绪，不射击。不能称解析scope已修复整体时限。原X退出未观察，consumer drain false；Native/entries/pad、XR0正常，旧绑定消失、13文件恢复，未强杀。233投影帧26.5849183秒，24全跨度样本亲自查看，16.384秒等待卡、17.745秒后第三人称身体遮挡；未验收视觉/头显/整枪，25%不变。证据parse-next15-review.json。

下一小改仅使用原Reader已有reuse_initial_identities，在Troika纯只读link/context元数据初始发现中避免反复读取同一身份；在原links/closing前关闭，保留现有两轮link值、全部retained_metadata新鲜身份/结构/属性/名称/表检查、对象和原占用约束及100ms。不是跨查询缓存或许可。新私有PG92e9be06配置编译0/0，测试待完成。


## 初始身份发现与字段关闭复核（2026-11-11 00:09）

私有PG97e5f7c6a2eba4bc92ec1c0893e94a397415ca94c4e828bfcf6614a4dde947b6在Troika原link及mounted context初始发现中使用既有Reader.reuse_initial_identities；仅局部查询，原links/closing前恢复false，保留原两轮link、原占用、metadata身份/结构/名称/表和100ms检查。

针对性真实元数据内存夹具发现原link未把已读Property登记Snapshot.fields，关闭时改变offset未被拒绝（scenario4 actual1）；R1/92e9be06不运行，失败log保留。R2显式登记该Property并使用原retained_metadata鲜读关闭；错误指针/类型、关闭身份/字段偏移/父类children/对象表/字段name变化以及下一次指针变化均拒绝。准确抽取生产link与原关闭函数、原解析测试，/W4 /WX编译0、34052检查通过；首次测试生成器多余括号的编译失败单独保留，不当生产失败。

冻结1305源，R2生产编译0/0，Native815ece03/XR16840b27不变。next16准备/检查通过；第一次资源拒绝无claim/无game，随后内存恢复时磁盘3847262208字节低于4GiB仍在claim前拒绝。候选尚无游戏结果，继续等原资源释放，不改保护、不清他方实例。完整Troika/视觉/头显与25%均未改变。


## next16冷名称准备未完；合并重复扫描（2026-11-11）

next16/game11068 Native815ece03/PG97e5f7c6实际进入通过，但geometry名称cursor49152/63203时fixture原3秒准备等待结束，没有Grip/R2。不能把该次当接触就绪，也不能单凭一次认定代码回归或系统负载根因。X退出未观察、consumer drain false；Native/entries/pad/XR0正常，旧身份消失/13文件恢复。206实际投影帧26.0766141秒、24全跨度样本亲自查看，18.508s等待卡、19.498s后身体遮挡，未验收视觉/整枪/头显，25%不变。

新PG916bf65b8055c317cc9ea48dac9b379756e3c36e7d241567c484d429f66e1680把Troika两组3名称扫描合为6名称一遍。Lookup泛化为FixedLookup<N>，原Lookup别名仍为N3，原三名称237检查与六名称新检查合计256通过；当前表变更/源过期丢弃进度、ready鲜读标签、原2048项/1ms预算及100ms不变。接触仍只读handleroot矩阵，完整机构仍读取所有原骨/限位/枪口/controls。生产配置编译0/0，Native/XR不变。

fixture冷名称准备上限由3秒改为8秒，等待发生在任何Grip边之前；记录实际准备耗时且production_response_latency_accepted=false。它不是延长源有效期或放宽动作许可，不以更长等待宣称玩家响应速度通过。完整双握/R2/退出步骤不删减；next17尚待实际。


## 触柄进入前的原装备恢复责任（2026-11-11）

11068实际日志在mounted后出现native_equipment_visibility restore0/outstanding1、reason2/1、first_read0。源码表明：旧原Pawn的stow/visibility journal会设置含X的原按钮fence；其恢复严格要求旧PC.Pawn/旧Pawn.Controller。触柄无face edge，不会触发原pre-face恢复，TryToDrive换Pawn后这些身份链接当然不再满足。此链与X未退出/排空失败相符；未记录当时实际fence位，不能宣称所有旧失败已证明唯一根因。

新私有PG34be28fb043fa42b32fc1f3750b8e4989561172c489b7ce32b02b87d1c9794d0只在实际触柄请求、原Frame仍current时，先调用既有stow_native.restore_with_reason(2)恢复旧Pawn已拥有的记录，再要求outstanding=false与原Frame current，才准备原TryToDrive。恢复失败则拒绝进入，不放松mounted后身份，不清他人记录，不补按钮、不直接强制退出、不续100ms源时间。该候选继承单次六名称扫描，原骨/限位/枪口/controls与gun/C0/R2原校验保留。源码1305、生产编译0/0；日志换行生成器R1与其修正R2、初次生成器变量错误独立保留，不冒充运行失败。

joint-only next17在资源拒绝后尚无claim/worker，已明确撤回并指向next18，避免再产生已知旧Pawn恢复责任问题。next18准备/检查通过，实际仍待资源；本枪不抢占其他进程，不新增助手。

按既有Owner最新三次/48h策略，精确终态身份再次确认后清理了2869个本方旧录像帧/视频/完整PPM GPU图，468062731字节；未递归删目录，源码/构建/资产/配置/存档/日志/数值不删，见own-media-retention-next16-plan.json与receipt。该较早计划额外保留next13及其后全部新媒体，当前最近三次的精确滚动保留将在本方下一次正常收尾后对齐；当前无编码/媒体删除后台作业。


## 原X退出与消费者排空首次实际通过（2026-11-11）

next18/game54248 Native815ece03/PG34be28fb：原visibility恢复restore1/outstanding0，进入交接restored1/clean1/current1；原空手触柄进入通过，合并名称cursor59057/found63 ready，移动空掌之后的额外准备等待0.375秒。双Grip后gun/C0仍未就绪，不发R2；不能称完整机构修复。cleanup中的原左X在实际PC.Pawn/Driver/DrivenVehicle三项读回中确认退出，pre-teardown fire0/hand5/drainedtrue，最终消费者同样排空true，Native/entries/pad/XR0、旧身份消失/13文件恢复，未强杀。该结果支持进入前恢复旧Pawn记录这项修复，不追溯宣称所有历史失败唯一同因。

200帧实际合成双眼投影23.9759533秒，24全跨度样本亲自查看；17.232秒后第三人称身体遮挡仍在，数值退出通过不等于镜头/全表面贴合/完整身体恢复通过。完整Troika、头显、25%均未变。详见handoff-next18-review.json。

下一私有PGf80e6106ee6e96da715998f86d7eceaddc72d3deb92af640ce382c86471daffe只在既有诊断开关下记录read起始源/输入年龄、read耗时和mounted context调用次数/耗时/失败数；不新增actor/input读取。静态去除计时包装后current谓词及顺序相同，完整机构body逐字相同。原100ms、原完整Getter/限位/枪口/controls、实际gun/C0/R2全部保留；唯一parts文件delta，生产配置编译0/0。next19用于区分读入前已消耗时间与本次重复上下文读取，不延长源时间，不把日志作许可。


## 实测重复context占主要时间；单次Reader候选（2026-11-11）

next19/game16524 Native815ece03/PGf80e6106实际：接触ready样本read36.536ms，其中9次context共35.803ms，context_failed0。完整controls样本入读时原input年龄86.7367ms，read16.2276ms/4次context15.6239ms后到102.9643ms；另几条full拒绝也超过100ms。日志样本未见context校验失败，不据此排除未记录的其他原因；已证明重复context成本和入读前消耗共同挤占本次窗口，禁止扩大100ms。X退出和消费者drain继续true，Native/entries/pad/XR0、旧身份消失/13文件恢复正常，无强杀。189投影帧20.6585679秒，24全跨度样本亲自查看，后段身体遮挡仍在；完整机构/gun/C0/R2未通过，25%与头显接受不变。证据timing-next19-review.json。

新私有PG64d3fbfde0dac2b1b88a043f7d0d8cbb11030f5e5522a885819cef6d4ed23f83仅改mounted_context：同一次observe的17个typed link使用同一局部Reader，仍新鲜读取两轮actor值；保留原17链接目标/顺序、Camera结构读取、全部class/占用/identity，所有PointerProperty显式retain，两轮之间及末尾均鲜读metadata。Reader不跨observe或frame存活，初始身份复用在两轮前关闭；不缓存actor值，不删每个getter前的current校验。原typed元数据内存夹具加实际helper测试34099通过：中途指针/类/字段/名称表/children/slot/不可读变化拒绝、重复字段不膨胀、错误component类型和容量耗尽拒绝。生产配置编译0/0，只有context文件delta，Native/XR不变；next20待实际对比context成本、完整机构/gun/C0/R2/退出，不能仅以离线检查宣称已修复。


## 2026-11-11 相机字段首匹配修复（next22，未实跑）

- next21/e00f51a9在启动前新增owned-memory反例：Camera类头部已有Padding字段，绑定后仅改其名称为ViewTarget；原fresh observer拒绝，Query错误接受旧容器。失败保存context-camera-prefix-r2-failure.log。next21未有run claim/game，现已withdrawn，e00f51a9从可选catalog移除。
- 新私有PG decef40c每次read_camera重新用原typed field解析首个ViewTarget，并核对exact Property、声明owner和原TViewTarget结构。17 actor links分组fresh walks、两次值读取、原100ms/身份/输入/机制保持；没有新writer或跨帧授权。
- 34166检查通过，包含原完整metadata suite及新相机prefix负例；完整PG configure/build均0，1305source冻结、仅context文件变化。Native815ece03和XR16840b27复用不变。
- 新独立troika-camera-1111-next22已prepare/check，官方资源准入确认两个exact-live worker仍占用，未启动游戏。下一步只实跑此新候选，测实际耗时、full mechanics、双握/C0/fire、原X退出和正常清理；不把CPU/编译当完整玩法、自然手型、视角或头显验收。Publicv71与25%保持。


## 2026-11-11 next22 实际进入失败与 next23 定点诊断

- next22 Native815ece03/PGdecef40c实际game52036进入Playground及epoch1投影。空手近处不进入通过；真实触达计划与随后raw/world位置吻合，但3秒内原PC.Pawn/Driver/DrivenVehicle仍未改变，未进入Troika。原进入半径100，实际距离60.428，不是本轮半径外失败。未调用Grip/X/TryToDrive替代；完整Query/双握/C0/开火均未触达，不能据此接受或否定新Query性能。
- 原谓词日志gate16为discovered1/in_reach1/parts_read0，日志上限在接触前耗尽；没有troika_entry_contact或原进入调用。当前证据不足以区分unmounted parts反射、name progress或100ms内过期，因果保持未证明。
- 原生/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone、13项私有baseline恢复、无强杀；1095前台采样无本方前台，170cloak检查。实际游戏音频readback268次verified、verify_failed0；XR辅助未匹配独立音频会话，不冒充其mute证明。
- 实际198帧21.9295秒组成GPU stereo编码0，24样本全保存跨度已亲看；可见接近与手到枪旁，无占据/动作通过。不是独立桌面配对或逐渲染帧/头显证据。
- next23 PG e5f389b0仅在原unmounted parts九处false分支增加最多48条本地失败位置/名称cursor/found/原输入年龄记录，无新对象输入读取。移除观察后原body逐字一致；1305source完整PG configure/build0/0，原Native/XR复用。初次生成器断言误计10实际9，未改源时退出，验证继承hash后修正继续；next23已prepare/check，资源拒绝前无run claim/game。
- owned-memory读取量对照：0/8/32个无关前缀字段时，Query当前15404/16236/18732，对比原13648/16248/24048；小样本多12.87%，32字段少22.11%。仅工作量数据，不作真实游戏耗时改善证明。
- 按既有保留政策清理本方终态next18/16/15生成媒体645文件152722096字节，最新next22/20/19保留，源码/构建/存档/日志/数值保留。完整Troika/自然GrabPose/视角/头显FALSE、25%及Publicv71不变；下一点实际定点unmounted读取首败。


## 2026-11-11 next23 名称扫描首败与预算修复 next24

- next23实际game36816/creation134388085144047486，Native815ece03/PG e5f389b0。空手近处不进入通过；接触未进入，原Query/完整握持/开火未触达。所有8条有界unmounted读取失败均在name step；首两条cursor0，随后8192→17968→26160→35280→45168→53360/found0。纠正“全程零进度”：实际是首次为0、后续推进但尚无所需名称。记录的输入末年龄36.199..62.8571ms，不能归因为这些行超过100ms，也不据此排除所有未记录原因。
- 原scan budget在初始current guard前启动，已证实可被guard开销耗尽。新私有PG0076f101把同一1ms/2048项预算起点延至实际扫描，仍在初始和提交前检查current，原100ms/身份/表变更/输入保持。仅weapon_troika_parts.inl变化；1305source完整configure/build0/0。提取实际生产helper16检查通过：旧零进度重现、新扫描推进、过期/表变化丢弃进度。尚无实际修复验收，不称独占因果闭合。
- next23的195帧21.5432秒实际composed stereo编码0，24全跨度样本已看；正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone、13私有baseline恢复，无强杀。完整Troika/GrabPose/视角/头显仍FALSE，Publicv71/25%保持。
- next24 troika-entrynames-1111-next24已prepare/check，最新官方资源拒绝为exact-live cover-1111-r145-cloak-core与lancer-natural-v139-225928；本轮未有next24run claim/game。下一步直接验证此修复后的名称准备与空手原进入，随后原full mechanics/双Grip/C0/fire/X退出。
- 最新三份媒体next23/22/20保留；终态next19旧生成媒体191文件43012450字节按政策清理。源码/构建/存档/文字/数值保留，不作递归目录删除。


## 2026-11-11 next24 名称就绪与实际接触；next25 连续性诊断

- next24实际game56736/creation134388088708493129，Native815ece03/PG0076f101。名称每步2048推进，cursor59055/found7，原3骨MatchRefBone/GetBoneMatrix可读、实际contact mask1/最近.00313352411m。只证明本轮名字/接触读取到达，不把调度差异当独占因果闭合。
- 原版占据仍失败；首mask1 frame310连续位false，后续316/318/320连续位true/false交替，request位均false，无原TryToDrive调用。TroikaEntryContact在不连续时清armed，持续接触不能续出edge；保护保持。当前不知道具体身份/参考/时间间隔哪项破坏连续性，不能仅凭低帧率猜定sync间隔。
- 后续Parts拒绝包括99.9663ms骨匹配组和>100ms初始/closing，保留各组实际失败，不能把名称ready当全部读取修复。mounted Query、双握、C0、R2和原X退出未触达。
- 正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone与13私有baseline恢复，无强杀。201帧25.6281秒实际composed GPU stereo编码0，24样本全保存跨度亲看，仍仅接近/手向柄，非占据/完整手型/头显验收。
- next25 PG16a3cafa只加入接触时最多24行本地连续谓词原因：原26个合取条件加no_previous/recentered/heading三初始位，记录prev/current frame/sequence/sync，不额外读对象或输入、不改变cohort/释放/anti-replay/100ms。移除观察后runtime body逐字一致；1305source完整configure/build0/0。已prepare/check，准入因另两exact-live worker拒绝，无next25run claim/game。
- 最新三份媒体next24/23/22保留，旧next20生成媒体188文件40702854字节按政策清理，源码/构建/存档/文字/数值不删。Publicv71/25%保持，完整Troika/自然GrabPose/视角/头显FALSE。下一点仅查具体连续断点，再推进原进入及完整逻辑。


## 2026-11-11 next25 确认相邻样本间隔失败；next26 空手现有耗时记录

- next25实际game54760/creation134388096498185857，Native815ece03/PG16a3cafa。名字ready7，实际接触仍不原占据。12条contact cohort记录全部failed2097152=bit21（相邻input sync delta<=100000000失败），equivalent1；其余28个身份/参考/追踪/初始条件均通过。这些记录的previous/current frame为相邻ID，delta141386700..177418000ns。只证明本轮接触断点，不把未记录拒绝或整帧耗时来源猜成同一因果。
- 原100ms/fresh outside/anti-replay保护正确拒绝，未放宽或伪造新输入。另有Parts原getter/closing超过100ms，仍保留；mounted Query/完整双握/C0/R2与X退出均未触达。
- 正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone/13私有baseline恢复，无强杀。171帧18.9056秒actual composed GPU stereo编码0，24样本全保存跨度亲看，接近与接触姿态可见，非原占据/自然手型/头显通过。
- 现有native_equipment_post_tick_cost只在hold_generation非零记录，空手阶段没有样本。新私有PGd179805f仅在原GEARS_TROIKA_ENTRY_DIAGNOSTIC与已配置空手Troika路径启用原分段耗时记录，最多24行/每16次tick保留后续样本；普通持枪仍原16行/128次。复用原时钟/字段，不额外读游戏对象或输入，不改变动作路径。移除诊断准入/采样改动后playground.cpp逐字一致；1305source完整configure/build0/0。
- next26 troika-cost-1111-next26已prepare/check，新官方ResourceUnavailable为两个exact-live worker，未有run claim/game。下一步原分段耗时定位原游戏更新/装备/后续处理代价；不能放宽100ms或据141..177ms直接断定GPU/某模块。共享首次准入有界等待更新消息已接收，只读未回复；不重启旧名/改旧制品哈希。
- 最新三份媒体next25/24/23保留，终态next22旧生成媒体200文件48910017字节按政策清理，source/build/save/text/numeric保持。Publicv71/25%不变，完整Troika/GrabPose/视角/头显FALSE，目标active、本轮progress。


## 2026-11-11 next26 耗时取证范围修正与 next27 真实帧绑定

- next26实际game58948/creation134388104227719028，Native815ece03/PGd179805f。名字/接触继续到达，原进入仍失败；24条cohort含65..155ms，连续true时保持contact也不续出request，fresh outside保护保持。原完整Query/双握/C0/fire/X退出未触达。
- 24条原post-tick成本样本equipment1..52.6us，total中位3.43255ms、前三次pre-original107/451/171ms。样本缺少实际Native frame/controller/function准入标记；其周期相位可能未覆盖实际部件读取，不能把微秒数当整个Troika成本或直接归因GPU/Native/原游戏。保留原数据/范围，不拿该结果作性能已修复证明。
- 新私有PG4087fccb仅诊断：copy_frame_on_gt/当前controller/known/current/restore全通过后更新本地诊断serial/frame，原hook在同次troika tick前后比serial；空手耗时抽样须qualified，补controller/function/frame/end_ns。诊断序号不授权动作、不更新源时间、不新增输入/对象读取。移除诊断后两个原文件body逐字一致；1305source完整configure/build0/0，next27已prepare/check，资源拒绝前无run claim/game。
- 正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone/13私有baseline恢复，无强杀。187帧20.5849秒actual composed GPU stereo编码0，24全跨度样本已亲看，只有接近/接触姿态，不作完整手型或头显接受。
- 已整理相对当前root的8个Troika-owned文件源码差异983行及双方hash；2个共享依赖单列不整体替换。首次patch文本写入把LF context转换CRLF导致3文件dry-check拒绝，未改源码/Git；按各目标原行尾重新生成字节patch后git apply --check0。仅reviewable pending delta，未应用root/index/公共包，runtime未接受。见pending-troika-owned-integration-20261111.json/.patch。
- 最新三份媒体next26/25/24保留，终态next23旧生成媒体197文件47678503字节按政策清理，源码/构建/存档/文字/数值保留。Publicv71/25%不变，完整Troika/自然GrabPose/视角/头显FALSE。下一点next27取得实际当前帧成本，再作针对性修复；目标active、本轮progress。


## 2026-11-11 next27 当前帧确认装备块35–70ms；next28 分段待跑

- next27实际game21988/creation134388110351449059，Native815ece03/PG4087fccb。11条成本记录全部qualified_troika1，原Native frame42..222、同当前controller/function可对应；combined equipment35.2682..69.6487ms、中位48.1ms，已证实该实际块有显著代价。仍同时包括Troika进入和后续ordinary equipment，不能直接锁定某个子模块或GPU/Native。next26微秒周期样本不得代表完整当前路径。
- 名字与接触继续到达，原占据未触发；未送Grip/X/TryToDrive替代，完整mounted Query/双握/C0/R2/X退出均未触达。原100ms/连续/释放/anti-replay不改。
- 新私有PG47ff3909仅诊断分段：已验证本地Native Frame后RAII记录begin/discovery/rig/geometry/decision/return；原hook另分Troika整体和ordinary equipment。只有时钟/本地字段，无对象输入新读或权限/时间更新；去掉诊断后两个原body逐字一致。1305source完整configure/build0/0，next28 troika-stages-1111-next28已prepare/check。最新exact-live资源拒绝，未有run claim/game；不重启next27。
- 正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone/13私有baseline恢复，无强杀。189帧25.8800秒actual composed GPU stereo编码0，24全保存跨度样本亲看，接近/接触画面而非原占据/完整手型/头显通过。
- 最新三份媒体next27/26/25保留，终态next24旧媒体203文件48983789字节按政策清理，源码/构建/存档/文字/数值保留。Publicv71/25%保持，完整Troika/GrabPose/视角/头显FALSE。下一点实际分段锁定主代价后窄修复，目标active、本轮progress。


## 2026-11-11 next28 分段确认与空手空闲读取优化 next29

- next28实际game26932/creation134388120572276471，Native815ece03/PG47ff3909。16条当前Native帧绑定分段：Troika进入10.4711..56.6455ms、中位23.63175；ordinary equipment .0218..45.3817ms、中位29.3206；进入中discovery中位3.04295、rig4.593、geometry8.8376/最大33.0705ms。缺失点0按未到达处理，不虚构elapsed；原占据仍失败，完整Query/双握/C0/R2/X退出未触达。
- 实际普通Runtime空手quiet仍走完整vrequip::snapshot，含每槽HasAnyAmmo/selection及grenade count等动作决策读。新私有705c6946扩展已有held-only观察模式：仅成功baseline后的quiet/同cursor/no_pending/no_future/no_hold/no_switch/no_animation帧，提前读一次原Input结果；无event才用现有双current结构库存/原函数映射认证，加原CanSwitchWeapon与bSwitchingWeapons供既有stow visibility。ammo/throwable选择状态仍UNKNOWN，不作动作许可；一旦有实际event/游标异常/未来或pending，仍完整原inventory。没有保存Frame/event或更新原时间。
- Backend尾部可选PG-local unheld_inventory，不改Native/公共手接口；未配置consumer保持旧路径。原action dispatch suffix、完整inventory callback及现有idle_inventory源码精确不变。133cases1669后再补callback重入和撤销，最终135cases1687checks0，包含quiet不调用full、真实新事件/空槽反馈/失败必full、过期/身份/游标/baseline/future/pending/flags及无新switch/sound/hold。实际Native reader/游戏耗时改善未证明，不能把CPU test当修复接受。
- 1305source完整PG configure/build0/0，Native815/XR1684复用。next29 troika-unheld-1111-next29已prepare/check，原入场动作不改；资源等待中。source diff和风险审查见unheld-idle-source-review.json/unheld-idle-private-delta.json，未整文件替换共享root、未写Git/index/公共包。
- next28正常Native/入口/手柄恢复、fire0/hand5/drained、XR0/gamegone/13私有baseline恢复，无强杀。201帧22.1747秒actual composed GPU stereo编码0，24全保存跨度样本亲看，只有接近/接触姿态，非原占据/整手/头显接受。
- 最新三份媒体next28/27/26保留，终态next25旧媒体173文件41925405字节按政策清理，source/build/save/text/numeric不删。Publicv71/25%保持，完整Troika/GrabPose/视角/头显FALSE。下一点验证ordinary cost与原空手进入，再关闭完整逻辑；目标active、本轮progress。


## 2026-11-11 空闲优化编译/135案例通过；next29 准入拒绝与 next30

- 空手quiet优化PG705c6946生产完整编译0/0，actual Runtime+model controlled seam135cases1687checks通过；不把CPU检查当实际速度/原进入通过。动作后缀/完整inventory/原结构库存读取精确未改。Backend新可选观察仅无新event/游标不变/无pending、future、held、switch、animation才进入；CanSwitch/animation是原真实读，ammo/throwable未知不能作动作输入。重入拒绝、回调撤销仍disarm；新动作必完整原读取。
- next29仅advisory空位，canonical first admission等待60s后ResourceUnavailable/exit1，未到commit：worker.log/worker.json/launcher/done均不存在，canonical PID19032已终态，13私有baseline文件SHA未变。无Native/游戏/XR/动作，不是产品或cleanup实测失败。旧通用wrapper写restoration_pending已由独立review闭合；next29-original-result.json保留原样，实际记录附no-spawn review，不伪造game identity/done/正常运行通过。旧名withdrawn，永不重试。
- 仅本方wrapper修正：记录自己Popen canonical PID和wait退出；正式first wait显式180s（共享原API0..600范围），不再只凭注册缺失判断。只有同PID/first_admission_only请求、子命令终态非零且所有commit marker均无，才证明未spawn；直接验证baseline不变，无runtime时不再等待虚构done或fake close。任一worker.log/receipt/launcher/done出现仍保留原实际清理/身份恢复路径。12行为反例+commit先log后spawn静态审查共13checks通过。首次静态顺序检查误取整文件无关child，限定实际commit块后通过，未改共享API。
- 新唯一troika-unheld-1111-next30复用原705/815/1684制品，无重编或换机制，已prepare/check。最新官方admission为exact-live boltok-1111-033900+lancer-natural-v139-225928，run claim/start request/worker未创建。下一点仅实测ordinary段改善、原空手进入及full Query；不能声称整枪/自然手型/头显修复，Publicv71/25%保持。目标active，本轮progress。

## 2026-11-11：next31 原版进入通过，握持控制未就绪

- next30 首次准入180秒资源拒绝；实际未创建worker/game，13项私有基线未变，旧名字撤回。私有入口改由canonical单次首次准入最多600秒等待，登记后不重试，原磁盘/内存/双实例/身份保护保持。
- next31 实际game22788/creation134388140907728208，Native815ece03/PG705c6946。空手接触原版进入通过，无未请求射击；结构库存路径221次命中/0拒绝。普通装备阶段中位14.8645ms，前轮29.3206ms；不同运行，不能当严格配对因果证明。
- 完整Parts模式两次ready，但未记录可选控制/绑定/socket/physical标记，不据此称可开火。双Grip后枪械与C0就绪记录未同时出现，未发送R2；保留首败。
- 原X退出读回、消费者排空、Native/入口/手柄恢复、XR0、精确自身进程消失和13项私有基线恢复通过。197张实际合成双眼GPU图/21.501秒、24张全跨度抽样已看；16.009秒起身体/背部遮挡持续至末尾，数值退出不证明视觉恢复。整枪/手型/头显未验收，25%不变。
- 新PG b425166e 仅增加现有Parts可选标记、control receipt拒绝行及Grip/lease状态变化的有界记录，无额外源/输入/Native读取，不改权限。完整编译0及原逻辑去诊断后等价审查通过；全新next32已prepare/check，通过单次canonical首次准入继续针对性检查，tool16378，禁止重复启动。
- 证据：analysis/troika-development-20261109/unheld-next31-review.json、unheld-next31-measurements.json、mounted-readiness-source-audit.json。Publicv71不变，无助手或其他武器扩展。

## 2026-11-11：next32 拒绝点实录

- 实际game59388/creation134388150950513715，原版空手接触进入与X退出通过。完整Parts ready行controls_known1，但enums/binding/socket/physical_start均0；不能将ready当可瞄准/开火证书。双Grip press1时line209原source_current拒绝记录100.35/100.53ms，另line225 Native reply1，保留独立拒绝，唯一因果未定。未发送R2，枪械/C0首败保留。
- Native/入口/gamepad恢复、消费者排空、XR0、精确自身进程消失及13项私有基线恢复正常。195张GPU图/21.338秒，24张全跨度亲看，15.841秒已见身体/背部遮挡持续末尾；编码0，完整视觉/手型/头显仍FALSE，25%不变。
- 下一点离线分查原GetEnum schema/实际标签与physical-start getter，再修明确原因；不重跑无变化候选、不延长100ms或授予动作fallback。证据readiness-next32-review.json及readiness-next32-first-failure.json。

## 2026-11-11：原getter拒绝位置诊断已编译

- next32实际controls1/enums0/physical0尚不能确定schema、原返回名字或时效根因。新私有PG bb93edf1在原GetEnum schema/decode和physical-start schema/read既有拒绝分支记录行号，并记录原调用已返回的名字及现有socket_known/位置；无新源/输入/演员/Native读取。每行最多2、每函数有界总数，原短路和全部关闭核对保留。
- 三文件去掉诊断后原正文逐字相同；1305冻结源完整核SHA，configure/build0/0。未将离线模拟调用当原游戏执行；新next33已prepare/check，通过一次canonical首次准入启动，tool67440，禁止重复名字或因观察超时重启。Native815ece03/XR16840b27原样复用。
- 证据getter-reject-source-audit.json、pg-getter-reject-20261111/result.json。完整逻辑→Snap/Volume→GrabPose顺序、整枪/手型/头显FALSE、25%及Publicv71保持，无helpers/其他枪。

## 2026-11-11：next33 原getter实际拒绝位置

- 实际game41620/creation134388155840664751，Native815ece03/PGbb93edf1。原空手接触进入及X退出读回通过，枪械/C0仍未同时就绪，未发送R2。
- 原socket getter有实际known1及有限世界坐标；physical-start schema实际line35拒绝输入/返回参数名字或flags，尚无具体p.flags，不可直接放宽ReturnValue或FireOffset。GetEnum schema实际line44拒绝E参数声明reference的Enum/Class/Core链，具体子条件尚未知；另current阶段拒绝并存。实际没有枚举名字成功记录。未证实猜测的FName对齐原因，禁止按假设修。
- 正常Native/入口/gamepad恢复、消费者排空、XR0、精确自身绑定消失及13项私有基线恢复；192张合成双眼GPU图/21.047秒，24张全跨度亲看，15.650秒中心身体遮挡持续末尾；编码0，整枪/自然手型/头显FALSE、25%保持。
- 下一点在已定位拒绝处分解原条件并记现成参数flags/布局/声明reference，不额外读取演员或输入，然后复现修具体合同偏差。证据getter-next33-first-failure.json、getter-next33-review.json；无助手/其他枪/Publicv71变化。

## 2026-11-11：真实getter合同偏差复现与修复编译

- next34实际58108/creation134388159628562753记录GetEnum.E声明reference名Object，参数20bytes/返回偏移12；旧代码只准Enum且FName需8字节对齐。原physical-start FireOffset flags80/offset0、ReturnValue flags580/offset12、参数24bytes；旧代码返回flags只准480，实际拒绝位置一致。
- 新私有PG5102915c只修Troika两个getter：Core.Object或既有Core.Enum声明仍须Class/Core与当前typed原UEnum；返回允许原合法Out|Return580及既有480，FName按4字节对齐，指针仍8。原类型/宽度/extent/非重叠/关闭及100ms全保留，不改动作权限。
- 按真实布局改owned原fixture，旧代码axes exit1/line158和physical exit1/line26实复现；修复144项axes及31项physical均0（含原错误类型/alias/元数据变化/失效拒绝），只是合成Reader/模拟派发，不当游戏。1305冻结源完整核SHA，configure/build0/0。新next35已prepare/check，tool65264单次运行。
- next34原Native/入口/pad/消费者/XR退出正常，done0/gamegone，但最初wrapper精确worker退出证明尚未成立，恢复未执行。原回执保留；后续四个精确身份均absent才恢复13项私有基线，无强杀；私有wrapper仅加5秒有界只读身份等待，恢复仍要求全部实际absent。
- next34媒体186图/22.035秒，24图全跨度亲看，16.838秒起背部遮挡持续末尾，编码0；完整机枪/手型/头显FALSE、25%不变。证据getter-layout-next34-evidence.json、getter-contract-progress.json及getter-real-layout-regression/，Publicv71不变。

### getter合同窄修同步当前源码

- 仅当前Troika enum_getter/physical_start两个owned文件四个精确hunk，保留原字节行尾/其他内容，无诊断或共享源码/索引/HEAD/公开包更改。当前依赖实际布局回归144/31均0；初次共享client新增log_file使旧fixture编译2，保留失败，仅测试wrapper补空日志sink后通过，不改共享产品文件。
- getter-contract-root-integration.json、getter-contract-root-regression2.json留证。next35/tool65264仍首次资源等待，未重复启动；实际验证未通过，整机枪/手型/头显FALSE，25%保持。

## 2026-11-11：next35 原GetEnum真实返回及枚举边界减重

- 实际40496/creation134388165650843732，PG5102915c原GetEnum已真实返回raw2=AXIS_Y、raw4=AXIS_Z，旧声明/返回布局拒绝点越过；完整enum/physical输出证书仍未通过，不称全部getter修好。fullread多个100.04..100.99ms失效拒绝，core仍Gun/C0 notready，未R2，原进入/X退出通过。
- 正常Native/入口/pad/消费者/XR0、自身精确绑定absent、13基线恢复，208图/23.202秒，24全跨度亲看，17.386秒身体遮挡持续末尾，编码0。25%/整枪/手型/头显FALSE保持。
- 只读几何已只读handleroot，不重复旧优化。新私有PG1fa4c812仅去掉枚举host循环和返回名处理前的相邻重复current；每次原版调用前current、原schema前后、返回保留元数据及current关闭、最终current全部保留，无跨callback保留、无新动作/输入/时间戳。相同八次合成调用旧41guard/240checks与新25guard/160checks均通过，144原axes检查0，失败注入逐guard和每次调用后都不输出。CPU为语义证据，不当游戏耗时。完整编译0/0，next36 prepare中tool22992，未启动。
- 四份本会话已结束next29/30/31/32完整归档J:/gearsVr/analysis/troika-instance-archive，原C路径junction保留，逐文件哈希匹配，未删源码/存档/日志；own-instance-archive-20261111-receipt.json。候选result最初delta误用最后循环变量已保留原回执并按完整inventory纠正，仅元数据无重编。

## 2026-11-11：next36 原BoneSpace返回、完整读取仍未通过

- 实际42320/creation134388171523451509，PG1fa4c812（Native815ece03/XR16840b27不变）。除AXIS_Y/Z外原GetEnum新记录raw4=BCS_BoneSpace；完整enum/physical/gun/C0仍未就绪，无R2。不同运行且100ms截止截断，不以局部query数/名字推进当实际耗时因果或修好。私有精简尚未合入root。
- fullread记录入场input age约55..81ms，失败结束100.02..101.55ms，context无失败而剩余时间不足的记录保留；不是整故障唯一因果。只读geometry早已handleroot单骨，禁止重复旧优化。下一点核完整读取前源龄与同次原调用/关闭边界成本，不延100ms、不跨回调续证书、不动严格动作prepare/commit。
- 原进入/X退出，正常Native/入口/pad/消费者排空、XR0、精确自身绑定absent、13基线恢复通过。197GPU图/21.547秒，24全跨度亲看，16.033秒起身体遮挡持续末尾，编码0；整枪/自然手型/头显FALSE、25%保持。
- 证据enum-next36-first-failure.json、enum-next36-review.json、enum-boundaries-source-audit.json。自有新三份媒体36/35/34保留；旧33/32/31按原政策仅删生成媒体，不删源码/存档/日志。

## 2026-11-11：next37四原枚举名字与按所有权读取候选

- 新私有PGf86aef46把枚举解码定为同回调数据事务：每次原getter前current、每次schema前后/返回名retained metadata、独特值结尾原getter重新查询、最终current才输出。中间source关闭延至下一次调用前或最终输出，中间结果不授权动作、不跨回调保留；原144检查及逐9guard/每次调用后失效80检查均0，完整编译0/0。未合入root。
- 实际next37 game56048/creation134388177536440342原GetEnum四名字AXIS_Y、AXIS_Z、BCS_BoneSpace、BCS_WorldSpace均取得；完整重查/关闭与gun/C0仍未就绪，无R2，不称完整修好。原进入/X退出/Native/入口/pad/消费者/XR0、自身精确绑定absent及13基线恢复通过；196图/21.505秒、24全跨度亲看，15.982秒身体遮挡持续，编码0。
- 新查full configuration无条件执行于尚无both_owned的帧；原current/observe消费者必拒绝该状态。PG863ad8ef只复用既有一次control_receipt输出作为提前跳过工作条件：无已知both_owned就中断旧proposal并省fullread；原geometry、Native边沿/所有权事务、瞄准/开火/枪/C0消费者逐字不变，消费时仍fresh receipt/current。完整编译0/0，next38 prepare/check，单次运行tool74378，实际待验证。
- 源码审查control-qualified-source-audit.json与enum-transaction-source-audit.json；原逻辑→Snap/Volume→GrabPose及整枪/手型/头显FALSE、25%/Publicv71保持。三份旧33/34/35归档J并junction，逐文件哈希一致；runtime中有归档IO，不能作严格配对耗时证据。重复使用旧archive helper误带过时metadata修正前缀，覆盖早期delta错误备份；原内容按既有tool输出另重建并明标，当前result/source/binary未变，前缀已移除，详archive-helper-prefix-correction.json。

## 2026-11-11：next38实际省读生效，转查Grip所有权首败

- 实际game5580/creation134388182294971843，PG863ad8ef/Native815ece03。control_known0→1且both_owned0，完整controls read行0；实际省掉当前无资格的配置读取，未闭合Grip→双手所有权，不称整故障修好。原左手known/tracked/release_required0在publication30/32有实录；随后生命周期/非活动拒绝仍在。
- 原control log24额度在真实Grip前耗尽，不能据早期状态猜按下阶段唯一拒绝。下一点只扩展既有日志对原source raw press/Native receipt状态的独立有界额度，先查所有权首败，再继续完整配置；禁止无变化fullreader循环，不延100ms/削反重放或加writer。几何names冷查29publication达ready，既有几何ready beforeGrip门通过。
- 原进入/X退出及正常Native/入口/pad/消费者排空、XR0、自身精确绑定absent、13私有基线恢复通过；195图/21.295秒、24全跨度亲看，后段身体遮挡仍在，编码0；整枪/手型/头显FALSE、25%/Publicv71保持。新配置gate与前两枚举减重候选仍仅private，root只此前真实getter合同四hunk。
- 证据qualified-control-next38-first-failure.json、qualified-control-next38-review.json。自身无活跃runtime/encode/build，旧34/35媒体将按最新三份38/37/36政策收口；源码/存档/日志保留。

## 2026-11-11：Grip按压独立日志额度已编译，未新增游戏

- 基于next38真实按压前旧24行耗尽，新私有PG144f03f8只在已有observe_control成功取得同一source/Decision后增独立press日志。每左右last_squeeze_press_sequence组合最多6状态变化、最多8组/总48；原query/source sequence/sync、Native decided与本地observe时间、前次已观察query/sync、raw edges/active/value/grip/press flags、当前receipt lifecycle/status/lease/左右known/tracked/down/owned/release，以及原world/reference/input/profile标识全部取自现有数据。只在写日志时取一次本地clock；无额外输入/Native/geometry查询、无新writer或时间戳续签。
- 单一tracking文件变化，去掉新增宏块原正文逐字相同；1305冻结源逐SHA核，configure/build0/0。next39 prepare/check通过，Native815ece03/XR16840b27不变；未run/无action claim，没有新增游戏/新录像，旧失败名字不复用，25%/整枪/手型/头显FALSE/Publicv71保持，目标active。
- 共享出生回执发布窄修只读核对launch_reserve及其原12项测试文件SHA与发布notice一致；核到sim_entry动态载ROOT launcher再调用ROOT launch_reserve，无旧PG Python副本。没有改共享文件、重跑未变检查或为这项通知新增游戏，不宣称HB Win5实际根因或实跑修复。证据birth-stage-publication-adoption.json、press-receipt-source-audit.json与press-receipt-progress.json。

## 2026-11-11：next39左手先获租约后撤回，Native精确连续性观察候选

- 实际game29068/creation134388190509206875，Native815ece03/PG144f03f8。11条真实press状态记录覆盖Grip：query35左手status granted/support1/id1，query36 lifecycle4→5撤回，右按下query40未成both_owned。必须修正“左手从未获得”的猜测；首次授权仅窄证据，不当持续握持或整枪通过。
- query36 Native decided距前sync105.486ms、query40为115.680ms，current输入分别约38.137/41.364ms，raw flags f/active1/值1、world2/reference10/input2/profile2保持。decided时间晚于连续性判定，尚不能据此独称timer原因或放宽；已有普通/挂载Native分流，不归因未证实的双路径运行。
- 正常原进入/X退出、Native/入口/pad/消费者、XR0、自身精确绑定absent及13基线恢复；184GPU图/20.137秒、24全跨度亲看，14.972秒身体/枪面遮挡持续末尾，编码0。未R2、完整解码/aim/gun/C0未通过，25%/整枪/手型/头显FALSE/Publicv71保持。
- 新私有Native3960cba3只在原decide已有局部量上记最多24个press后原query、前/本sync、原now、cohort/continuous/mapped-boundary/interrupted/hand-assignment/boundary及原incumbent；既有session-status在原stereo_mutex下输出紧凑列，无新时钟/输入/演员读取，无ABI/判定/写者变化。去掉观察控制正文逐字等价，1212源SHA，开启与关闭完整编译0/0。PG144f03f8/XR16840b27精确复用，next40 prepare/check，单次tool27418取证原Grip失败，并非为共享启动回执修复加游戏。
- 证据press-next39-measurements.json、press-next39-review.json、native-control-boundary-source-audit.json及native-troika-control-boundary/results.json。共享birth窄修原12离线检查复用，未改其源码或推断Win5实测因果。

## 2026-11-11：next40准入600秒终止、next41离线复用回调上下文

- tool27418已terminal1，原canonical PID47264首次600秒资源拒绝，未创建worker/game，pre_admission_no_worker_spawn与13私有基线不变正证通过；next40原名撤回，不伪造Native正常清理/游戏退出或回填新no-start协议。最新实际仍next39，不称Native边界新记录已取得。
- 同一资源占位跨原请求及两个目标继续回合持续；精确HB052600 worker53716/creation134388198120486950与natural139 worker5584/creation134387965860868429仍活。它们不是已证明仍运行的游戏，也不能凭新共享修正把旧no_birth改成功；不强杀/不跨会话stop/不放宽2实例或内存门。
- 继续完成独立源码：PG7b4c4f84在geometry prepare以现有严格Query.begin核初始context，随后同回调parts read借同一Query.sample/current，省重复构建。Query类/所有类型与prefix、两次fresh links/Camera、metadata关闭、每getter前后current及100ms不变；不跨callback/frame保存Query，普通fullread仍创建独立Query，commit与动作/火/C0消费者不改。双文件源、1305完整SHA，configure/build0/0，仅编译/静态正文审查，尚无实际耗时或修复通过。
- 全新next41/geometry-context-7b4c4f84＋Native control-boundary-3960cba3/XR16840b27已prepare/check，无run claim/无游戏；旧未运行Native观察候选仍保留，实际待名额释放。准备analyze_native_boundary_next41.py，只有同轮精确game/Native实例及正常收尾核对后才关联原Native条件，不以推算取代缺失数据。
- 新共享Python预启动拒绝协议四源SHA按python-prelaunch-rejection-20261111.json核对，原5/81/15离线结果复用，未重跑未变检查或为修正新增游戏；未热更新/重分类旧worker。当前私有wrapper对无四身份的场景仍保守，不把普通no_birth当成功。证据python-prelaunch-rejection-adoption.json。
- 本回合可独立源码工作已完成，下一必要实际验据取决外部旧占位正常收尾。native-boundary-runtime-blocked-audit.json记录连续三回合同一阻碍与最新精确存活核对；目标应受阻而非完成，完整逻辑→Snap/Volume→GrabPose及整枪/手型/头显范围保持，25%/Publicv71不变。无新runtime/encode/build在本方运行，无助手/共享源/索引/公开包修改。

## 2026-11-11：用户释放旧worker后恢复独立准入

- 11:47:42共享记录为用户明确授权结束旧53716/5584清理Python进程，原launcher已退、无birth、无游戏/XR；属于强制清理worker，不是旧生命周期正常通过，旧失败不改。
- 本方新next42在worker/game前遇磁盘准入拒绝，正证与基线不变保留，旧名撤回。旧36..42七份终态或未启动私有实例完整迁J并junction，逐文件哈希一致，无源码/存档/日志删除；own-instance-archive-after42-receipt.json。
- 全新next43相同PG7b4c4f84/Native3960cba3/XR16840b27 prepare/check通过，正式准入已获，worker20996/creation134388430870135086，tool73957跟进；仍0.4/Source1/隐藏/静音/物理输入隔离及原身份/时效/清理要求。尚无动作结果，不称修复通过。

## 2026-11-11：next43实际Native连续性时间条件已定位

- game43860/creation134388430949183175，Native3960cba3/PG7b4c4f84；原Native now及所有局部条件24条中18条只有previous sync age>100ms令continuous=false/boundary=true；cohort相同、映射未变、无已有interrupt、手柄分配已就绪且相同、序号/时间单调。只证明本轮这些判定，不倒推旧39全部因果，不放宽100ms或反重放。gun/C0仍未同时ready，未射击。
- 原launcher0、Native/入口/gamepad恢复、消费者drained、XR0、自身绑定absent、13私有基线恢复，未强杀。206GPU图/22.589秒、24全跨度已看，后段身体遮挡仍存在，编码0；整枪/手型/头显FALSE、25%/Publicv71保持。
- 测量native-boundary-next43-measurements.json，视觉context-next43-review.json。下一点减少实际几何/上下文判定前工作量，不能用已定位的保护拒绝直接当保护代码错误。用户授权的其他终态PSOCache清理记录仅作为缓存缺失来源，不重开旧实例重建、不当产品失败。
