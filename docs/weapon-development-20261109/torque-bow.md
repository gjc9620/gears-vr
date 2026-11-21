# Torque Bow 扭力弓：交互需求与开发验收

日期：2026-11-09。用户已确认并授权开发。装箭的完美换弹期限统一4秒；机构尺寸、角度与接触范围由本枪会话自行标定。配套[公共合同](common.md)。

## 1. 操作目标

左肩取原箭，沿弓体槽送入锁定；右扳机按住蓄力，正常松开按原规则发射。**Charge中fresh R3短按明确取消，必须与松扳机发射分开。** 不增加传统手拉弦步骤。

## 2. 玩家操作步骤

| 阶段 | 玩家动作 | 必须结果 |
| --- | --- | --- |
| 取弓 | 对应肩槽点按RGrip | 正常主握；左手可按现有副握规则支撑，箭槽和支撑互斥 |
| 取箭 | 缺箭时左手到左肩新按LGrip | 预留一支本枪原箭，原接受开始本轮装填 |
| 装箭 | 持续LGrip，箭沿固定槽位正确方向送入至锁定 | 只提交1发；到位后结束本轮装填，无额外拉弦／枪栓 |
| 蓄力 | 已装箭、可射击时按住R2 | 进入原Charge，弓臂／声音／反馈按原状态；基础方向随弓 |
| 正常发射 | 主动松R2 | 走原CheckFireShot；可能立即发射或等待原Pawn条件，不能伪称未到最低蓄力所以必不射 |
| 主动取消 | 尚在本轮Charge时fresh R3短按 | 清本轮待发并完整离开Charge，不发箭、不扣弹；该R3不同时换弹 |
| 取消后重试 | 先真实松R2，再新按R2 | 松开不能补射；新按才可重新蓄力 |

## 3. 取消、中断与疲劳

取消顺序沿已定原版链：停止继续注入 → 清本事务PendingFire及bPendingEndFire → ChargeEnded → 正常状态转换离开Charge，自动完成EndState。切枪沿正常PutDownWeapon；必须核对无待发射、声音／模拟／反馈停下及弹量不变。

不能用普通StopFire或Charge.EndFire取消，它们可能发射；不能只调用ChargeEnded，清理不完整；不能直接转Active却留下PendingFire。已进入Fire／已生成箭，必须如实记作已发射，不能退款或销毁假称取消。

失追、死亡、剧情、回中／空间换代按非发射取消，不把无效追踪当松扳机。恢复后不自动续蓄旧Charge。正常持有过久的原疲劳行为保留，不能随意改成永远蓄力或自动取消；原计数受MinChargeTime和奖励倍率影响，不硬编码现实6.5秒。

## 4. 弹药、模型和反馈

L0/S1装箭后L1/S0，重复插入不能加第二箭。未插松手归还预留，错方向／不足行程拒绝；单发没有普通半仓换弹。已装箭不再次取箭。

复用原 Locust_Torquebow.Locust_Torquebow_Rocket；保留原多骨弓臂和跨骨三角形蒙皮。黄色引导区分肩取、入槽、锁定；Charge显示原进度／反馈并能区分取消成功与真实发射。头转而弓不转，原基础发射方向不变。

## 5. 必须验收

1. 空弓取箭／装箭／蓄力／松扳机，原箭生成和扣弹各一次。
2. 蓄力中R3取消后不发箭不扣弹；持续按着R2不重启；松R2也不补射，新按有效。
3. 同次R3不能既取消又取箭／开始换弹；重复R3不重入。
4. Pawn暂不允许释放时，原pending语义保留；随后取消仍完整清理。
5. 失追、切枪、剧情／读档及恢复，不续发旧箭；已发箭不回滚退款。
6. 正常疲劳释放与奖励下计时保留原行为；不由手部驱动重置原时钟。
7. 单／双手、转头、近墙、原飞行和爆炸效果及自然手型分别检查。

## 6. 实施入口与待核对

复用 weapon_torque_bow_interaction.h、原Charge/CheckFireShot与现有调用／资产／库存后端；主动cancel_charge使用普通取消语义，不混成故障interrupt。本枪会话自行认证原ABI、箭槽坐标／接触范围、事件与pending归属及完整停止。装箭完美期限已定4秒；这是换弹期限，不修改蓄力和疲劳的原版计时。

来源：[W08](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/weapon-cards.md#w08)、[明确R3取消规则](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/offline-tech-prep-20261028/shared-contracts.md)、[非发射取消顺序](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/interaction-review-v2.md)。


## 7. 本会话实施检查点（2026-11-09，尚未完成）

唯一源码工作区 C:/Users/11/.codex/worktrees/6dof-single/gearsVr，分支 codex/6dof-single。启动时已核对并保留现有共享 dirty 修改；没有创建分支／worktree、操作共享 Git 暂存区、启动助手或修改公共 v71。

已实现并离线验证：

- weapon_torque_bow_interaction.h：左肩 fresh LGrip 与原接受装填回执共同启动且同时预留原箭，单容量、单插入步骤；4秒固定游戏时钟截止，截止当时可完美，超时普通。重抓和暂停不重启，未插松手释放预留。R3不再启动装填。
- 独立 cancel_charge(stamp, edge)：fresh R3仅取消本轮，覆盖Begin／EndFire调用回执尚未到达、原Pawn延迟释放的pending分支；取消清待发意图，已取消后真实松R2才能重新按下。保留实际原疲劳发箭事件，已发箭不退款。原pending_fire／Fire状态不能冒充取消完成。
- weapon_torque_bow_original.h：非发射取消执行合同，停止注入→清所属pending→清所属pending end→ChargeEnded→正常离Charge→读回完整清理；同一GT回调有界执行，不把步骤分散到后续Charge.Tick之间。未知／部分调用保留原owner和恢复责任，不盲重试，已Fire不回滚。该合同的测试后端是合成的，真实原ABI尚未认证。
- weapon_torque_bow_feed.h：复用公共FeedInsertionStroke，保留原始控制器与显示Snap分离；横向／小角度吸附不改变轴向深度、不授插入权限。槽位／箭挂接没有猜测默认值，必须由实测原资产绑定。正在装入的校准不能被表格变更缩短行程或放宽入口。
- Playground独立Bow原版ABI准备模块与当前枪分派已接入；只做当前Bow准备，actions_enabled=0。基本持有严格绑定Locust_Torquebow.TorqueBow原资产父链，不开放supported_instant_fire。沿同一现有C0 component lease加入Bow显示资格，没有新增Native骨骼writer、输入Hook或运行器。共同入口hunk已与Lancer／Boltok／Gnasher／Boomshot直接约定。

实际结果分别记录：

| 范围 | 当前证据 |
| --- | --- |
| interaction、original cancel、feed、asset scope定向检查 | MSVC /std:c++17 /W4 /WX /UNDEBUG真实编译执行通过；仅合成回执／内存／几何证据 |
| 完整Playground | 私有1942源码初始快照＋23项MinHook依赖＋明确overlay，Release编译链接0；最终PG SHA256 9fc287ec3e30a4c15f728f7653915853894880add30554a682d9a06b756822ea |
| 初始失败 | 保留漏收MinHook、Troika头作用域、Gnasher ADL和Bow局部名C4459原日志；仅依赖补收／重用各作者精确修正，没有降警告或整树回迁 |
| Native | 未重建；首个私有准备候选复用c98568f0的已保存通用Native及其旧源码 provenance，不称当前root Native已编译 |
| 模型检查 | 实际查看9488顶点／9742三角三向原检查图；该GLB无原蒙皮权重，只用于定位，不替换弓臂原蒙皮 |
| 首次游戏检查 | torque-bow-prepared-20261109-164457-abi已冻结，prepared SHA ff9504acbd14169474584b7c0ce964305789f29fd0ad6f3ee3fa037b465383c9；1965源码／219Python依赖，--prepare-only真实0，未stage／未运行 |
| 运行位 | 经HB向LS／其实际已受理继任查询真实尾；尚未获最终唯一交窗，不凭idle启动。普通排队保持目标active |
| 完整玩法／自然手型／双眼／实体头显／交付 | 均未通过，未作为成品交付 |

首个短游戏计划仅原控制器地面拾弓→实际肩取→当前实例取消ABI只读预备回执→两个持有角度→正常退出；不蓄力／装箭，不借此称本枪玩法通过。第一具体失败停止并针对修复，沿现有后台observe-only／静音／物理隔离、身份／新鲜度、双路录制、正常清理、编码和保留机制。实际启动前仍要核全部冻结依赖、前驱完整收尾／恢复及明确唯一交接，然后独立idle。

下一实施项：实际取消ABI与状态字段认证；原箭资产／固定槽标定及公共库存提交；唯一R2所有权中的Charge／正常release／fresh R3互斥接线；主握／左支撑／箭／入槽四个独立GrabPose及掌、拇指、四指、食指、腕、袖口和退出转换；实际原弹体／弹量一次回执与双眼接触面审片。尺寸由本会话标定，不回问已定规则。未完成全弓，不改变公共v71或全武器／头显接受状态。

详细本地证据：analysis/torque-bow-20261109/development-review.json、runtime-plan.json、window-queue.json、focused-regression.log、build-5.log、model-inspection.json和bow-model-orthographic.png。运行计划SHA 9d86138c0589ea8997c5886c3116d56f929db721deeba663024f46b958f90d95。

## 8. 原箭与完整停止读回前置（2026-11-09，仍未实跑）

本轮属于progress，不是完整扭力弓／实体头显完成。Coverr47已直接独立核验本枪164457的1965源码、219当前及归档驱动、438次AST、6项saved制品和所选EXE，正式单一受理本会话为唯一后继；回执analysis/cover-system-20261108/shape-layout-r47/bow-successor-check.json。最终交窗尚未发生，未stage／run；既有prepared SHA ff9504ac保持原样。后续研究没有替换排队制品。

- 从保存的MP_WarMachine原包按原inventory范围读取11968字节静态箭export并逐字节核SHA c06788de423217fd5b2a89fe64606271699e7aaa29e7534cd407e682f0c98a16。原位置流296点是148点的精确双份，648个索引只引用0..147，共216三角。原材质Locust_Torquebow.Locust_NewTorquebow_Mat。实际三向图已亲看，箭头朝源-Y、+Y为尾叉；不能沿用+Y朝前假设。检查副本仅保存在ignored analysis，不作为新分发模型。
- 原箭shaft在Y4.2/Z0的真实三角面交点X0.9953870763372161，确定持件接触参考；原tip为(0,-18.724075317382812,-0.007449000142514706)。weapon_torque_bow_arrow_mount.h统一源组件中心、掌面接触、原始控制器上的箭尖和吸附后反解，保留调用方实际uniform scale／units，不拉长箭或回读已吸附皮肤。多组scale／units和非单位腕朝向数学检查真实编译执行0；手掌／手指像素贴合仍未接受。
- weapon_torque_bow_arrow_source.inl只认证当前原MagazineMesh及原StaticMesh的严格所属者／资源父链；换owner、错package／错asset、读取中途换件反例均拒绝。其W4/WX/UNDEBUG owned-memory测试0；没有原箭显示／装入动作成功证明。
- weapon_torque_bow_state.inl加入实际只读停止观察：原state、原R2、primary PendingFire／bPendingEndFire、CurrentFireMode、pawn蓄力、音效、粒子、原endpoint、模拟体隐藏／碰撞／Tick probe和两个蓄力timer。Hidden不能代替停止模拟；IsProbing本机ABI仍需实际认证。只有原对象和当前Pawn Weapon链接精确一致可读取；tracking过期时须另有既有action lifetime。该完整PG新私有构成编译链接0，尚未stage／实跑，不作为排队164457替换。
- weapon_torque_bow_original.h补未知调用的只读迟到回执协调：只有同一owner／serial的完整步骤效果得到认证才继续，unknown或false不盲重放；真实Fire和弹量冲突分别保留。新增fault-injection反例真实W4/WX/UNDEBUG测试0，未宣称原ABI通过。
- 共享R3 veto与左右authored GrabPose提交能力已向唯一Native/C0写者及LS输入边界直接协调；未擅自增加Native writer、Hook或套用Lancer support_surface到Bow。主握／支撑／箭／入槽四套实际自然手姿、原箭入槽库存事务、真实Charge／release／R3取消接线、弹体／双眼／头显与交付仍全部必须完成。

证据analysis/torque-bow-20261109/development-review-r2.json、arrow-geometry-review.json、arrow-contact-landmark.json、arrow-mount-test.log、arrow-source-test-2.log、original-reconciliation-test.log、state-build.log、state-build-wrapper-r2.log。构建包装器的初始locale读／stdout编码失败分别保留；第一次build_exit0后的打印异常不算编译失败，最终修正包装器exit0，未降低编译警告。

## 9. 原取消执行适配已编译（2026-11-09，尚未启用）

新增weapon_torque_bow_cancel_live.inl，复用原GT调用与既有action lifetime；只接受仍归本轮journal的准确serial／weapon／controller／pawn链接，调用前记录责任并重复认证原函数identity、原脚本／native113状态转换及参数签名。只清本轮primary PendingFire和两个固定typed packed Bool字段，保留邻接位；清pending后正常转Active，切枪走原PutDownWeapon，读取其Bool拒绝不冒充成功。没有StopFire／Charge.EndFire／手调EndState取消路径。

未知写入／原调用结果与未写入拒绝分开；未知只经同owner读回协调，不盲重放。完整停止必须同时有实际观察结果和原Charge.EndState正常返回一次的既有Call订阅观察；输入局部fence不能提前丢弃原journal。完成后可按准确serial退役，未完成／unknown／已经Fire不静默忘记。该模块已放入私有完整PG构成并编译链接0；r2的auto同声明类型错误日志保留，r4最终编译通过。尚未接R2实际owner／Call正常返回分派，不能称原取消ABI或游戏取消通过，也没有在游戏启用。

共享写者已明确授权本枪新增通用R3 veto头／独立export alias，并沿现有同一fence处理，旧LS导出／ABI和100ms保持；尚未实施这一共享增量。左右authored GrabPose公共V4由Boltok维护独立2960字节wire和Native适配，Lancer负责唯一C0左右消费／fullbody，仍未端到端闭合；Bow继续独立author四套表，不套用Lancer support_surface或再造writer。

仍是Coverr47→Bow164457。本会话已独立核验并单一受理后到Lancer165800为唯一后继：1364编译源码／6saved制品／17当前驱动及独立逐字节审阅snapshot／34AST／所选EXE通过，回执lancer165800-successor-check-r3.json；初始审阅路径误选及历史helper副本误选保留并纠正，没有改其候选。后到HB／Boomshot沿该真实尾继续，不在Bow登记第二继任。以上仅队列登记，不是最终交窗；当前仍未stage/run，排队164457完整SHA不变，公共v71不变。

详细证据analysis/torque-bow-20261109/development-review-r3.json及cancel-build-r4.log。原箭入槽库存、真实Charge／release／取消与失追／疲劳、四套自然手姿、双眼审片、实体头显和交付仍全部必须完成，目标active。


## 10. R3兼容接口、独立指节表与边沿解码（2026-11-09，仍未启用玩法）

已落实获准的通用weapon_button_veto.h及独立GearsCaptureWeaponButtonVetoV1导出；旧LS名称／wire作为完全相同类型alias保留，两个入口都只转发同一既有publish，不新建按钮fence或输入writer。root原scope_input_mask和publish函数体未改。原始时间戳、Broker当前实例admission、100ms包含截止、菜单组合键优先级和消费后不重放规则保持。严格MSVC/W4/WX/UNDEBUG新旧兼容测试及原未改LS用例实际通过；Broker边界为mock，不能称运行ABI已验收。

私有Native兼容构成以实际编译过040742源码为基底，补旧LS接口、通用alias、现有Route消费能力和已单独记录的原汇编源，沿原production-target配置编译链接0；DLL SHA256 2e6e781ff85da962b160c11aee2bb743d4ec8fa698ae5e26562557b5c5e0d53d。dumpbin确认新旧导出相同RVA 0026AD10。此构成仅证明兼容增量可编译；没有称当前root Native全树已编译，没有stage或替换排队候选。早先不完整基底的Cover类型／缺失测试、汇编及Route失败日志保留。完整946源码和两个MinHook依赖身份见veto-native-final-provenance.json。

新增本枪input_fence客户端只使用当前Bow和现有held实例，通过已认证Native模块的通用导出提交原Frame；它不会注入按钮或写原R3。加入先前私有取消适配构成的PG实际编译链接0，SHA256 385b00d71847fd7f8c66c805e52aba071d377e814c5c5d5f7cfa705503f12f8d；初次C4459遮蔽全局名的失败保留，仅重命名新增局部变量修正。源码逐文件另存veto-client-pg-source-manifest.json，尚未调用此客户端来开启游戏动作。

weapon_torque_bow_grasp_pose.h给主握、左支撑、箭杆和入槽分别独立author十五个参考局部旋转，主握食指另有扳机端点；其余三组没有扳机驱动。实际公共V4 structural/reference_matches检查及四套整体表不同、端点只影响食指、参考不被修改等严格测试通过。这些仍是初始指节表；掌面、腕与袖口的模型对应、实际显示接线、自然接触和双眼皮肤贴合全部尚未接受。

weapon_torque_bow_controls.h从原累计XR边沿解码完整短按，Charge拥有者存在时fresh R3优先于同帧R2松开，只给取消意图。回放、过期／未来sync、超过100ms间隙、失追、代次变化、计数倒退或多周期顺序不明确时不补造放箭；恢复保持baseline。严格定向测试实际通过，初始fixture漏space_epoch的失败记录保留并修正。此解码不授原调用／输入所有权，不是取消玩法已运行。

Cover已通知r47获得其前驱最终交接并正在短只读检查；全收尾后唯一交Bow164457，尚未收到最终交窗。本会话重新只读核验原候选1965冻结源码、219当前和219归档驱动、438 AST、6saved制品及指定EXE全匹配，--prepare-only真实0；prepared与plan SHA均保持不变。只读核验不是stage许可，当前未stage/run。后继仍唯一Lancer165800；更后Gnasher等申请单次沿实际尾转，不登记第二继任。

本轮详细证据analysis/torque-bow-20261109/development-review-r4.json、prestage-readiness-r4.json、veto-native-final-provenance.json、veto-client-pg-build-r2.log、controls-test-r2.log。原R2/Call journal接线、箭显示／原槽／库存、正常原弹体与疲劳／失追分支、四部件接触审片、实体头显和交付仍未完成，目标active，公共v71不变。


## 11. 原EndState正常返回观察已接线（2026-11-09，仍无游戏动作）

经Lancer明确留出对应PG hunk，现有Call subscriber的normal返回分支加入唯一torquebow_charge::returned；include前仅增加受原Native-follow＋Unified宏保护的forward。所有现有枪分支保留。新观察器不调引擎、无输入／结果／骨骼writer；只能给已存在准确取消journal提供原Charge.EndState返回，同一call_id交付不重复计数，两个不同原EndState仍导致“恰一次”验证不通过。尚未由实际R2 owner启动该journal，所以不能声称游戏取消已经执行。

该接线在先前私有完整PG构成上编译链接0；最终源码、DLL和编译日志已另存analysis/torque-bow-20261109/charge-observer-linked-final，不修改排队164457。旧r4的385b00d7为当时真实编译观察，mutable cancel-build此后已被新构建替代；后续以此次独立冻结的完整身份为准。development-review-r5.json记录新SHA、全部源身份及初次旧private snapshot缺Gnasher anchor的assert停止，不将其记作编译失败或整树覆盖。


## 12. 首次实际游戏31716与完整交窗（2026-11-09，整次检查失败）

Coverr47最终明确唯一交窗后，本会话重新全核164457：1965冻结源、219当前＋219归档驱动、438 AST、6saved制品、prepared／plan及指定EXE全部匹配；独立idle通过才stage。实际运行仍是原冻结Native c98568f0／PG9fc287ec、actions_enabled=0，不包含本轮后来编译的R3、指节、按钮或取消观察代码；公共v71未动。

实际31716 creation134386917604141822完成原地面X拾弓和左肩slot2实际取持，准确原weapon3d946e680。完整日志有5次当前gate0，认证到ClearPendingFire／ChargeEnded／GetStateName／24字节GotoState签名及bPendingEndFire offset3100/mask2，原库存容量1／已用0／备用5；这是实际实例的只读准备证据，不是原取消调用或玩法通过。

operator取记录时以最近一条为判据，该条frame288/sequence3010为ready0/gate7，因此operator真实exit1、首失败停并正常关闭，两个预定持有角度没有继续。冻结query只在成功后写出完整Counts，且capacity=1，所以可以从实际counts和代码证明该条拒绝位于最终nativeequipment::current；没有当时age／withdrawal详细回执，不能再把具体原因断言为100ms、身份变化或已经修复。早先／稍后gate0不能抹掉整次失败，也不把缓存ABI当成新动作许可。

2页完整49.3秒paired总览及6张原始关键图（约41、45、48.5秒各game与双眼模拟器）已亲自检查。原弓实际出现、随肩取持有可见；主握／掌根／腕袖存在遮挡或重叠，四部件自然姿态仍未接入这份冻结候选，完整贴合FAIL／未接受。startup TPS、等待卡和镜像捕捉间隙保留，不能把capture gap自动说成游戏卡顿。566game／1055sim帧，两路覆盖结束；视频7752127B，49.3秒。实体头显未测。

正常收尾实际：wrapper工具5533=0、operator1、encode88084=0、retention90155=0。Native cleanup、entry restore、pad restore，fire_disable0／hand_stop5／drainedtrue，XR0、launcher0、原gamegone。wrapper finally短暂idle race导致settings未按原字节复原，原日志保留；独立idle后仅恢复本私有settings准确原字节，语义原本相同。预备baseline copy循环在本来缺失的可选head文件处WinError2停止，shell仍继续已授权运行；自己的config/settings此前已精确保存，head缺失与前驱一致，两个INI以已恢复的前驱准确字节为参考并在结束匹配，不伪称它们是自己的fresh前拷贝。

结束219当前驱动SHA＋AST、3实际stagebin匹配，5基线全部准确匹配，独立CIM／idle通过。媒体保留实际删2537项／481446992B、errors0；最新3局31716／26028／11108，48小时，源码／制品／文本保留。详细first-runtime-result.json、first-runtime-closure-proof.json、first-runtime-key-frames.json、first-runtime-retention-result.json及first-runtime-text-31716。

全收尾后再次全核唯一已受理Lancer165800的1364冻结源、6制品、17当前及独立快照驱动、34AST、prepared／plan／EXE与idle通过，已明确最终唯一交窗，不在本槽追加case。lancer165800-final-handoff-check.json。更后Coverr54等同一申请按要求单次转实际尾，未设第二继任。

## 13. 原R2按钮适配与首败观察增量（2026-11-09，未运行）

weapon_torque_bow_charge_buttons.inl只给既有唯一R2 owner提供原WarPC ButtonPress/Release('R2')适配，认证原owner、参数、脚本与identity指纹。调用方必须先journal准确serial、持续owns并提供同一当前Frame的原弹体来源ready回调。空弓不发原press、不进入自动计时补弹；press前预备完整非发射取消ABI，正常release必须当前来源，取消／失追不能借此普通release。沿现有R2 Name及正常GT原调用，没有新增raw输入／Hook／writer。已派发调用失败为unknown，不能按未调用重放。初次新增局部original遮蔽全局trampoline的C4459保留，仅改名button_call；完整PG编译链接0，1971源与DLL独立冻结charge-buttons-final，SHA81dd7ee55704d7c155890238cc8cc7792dc207eb049b61099b2955c049af1901。尚未开放Bow到shared R2路由，未调用此适配做游戏动作。

针对31716的具体最终重验拒绝，原prepare已把库存query、容量及最终current分别标gate7／8／9，记录同一个原Frame的开始／结束输入年龄和准备耗时。只读取既有withdrawal候选；它没有完整cohort标签，即使frame/sequence相同也仅候选提示，不能当身份／原因证明。没有新增source读取、续期、放宽100ms或将某次gate0缓存成行动许可。该变更与原按钮适配一起只编译，不沿刚交出的窗口重跑；完整身份见development-review-r7.json。仍需实际R2 ownership、原箭槽／显示／库存与奖励、真实原弹体／疲劳／取消／中断和四套手掌指节腕袖双眼接触闭合，目标active。

## 14. 原输入journal、弹体记账和失追清理边界（2026-11-09，仅源码与私有编译）

纯交互模型现在强制携带原XR input_sync_ns，以该时间计算100ms姿态期限；GT晚处理不重新得到100ms。零、未来、过期、倒退sync及正在蓄力时输入失效均拒绝新发射／进入非发射中断。原疲劳已经真实发箭但R2仍按住时，随后真实松开仍交还原R2键，不加第二箭、不退款、不制造新按下。严格W4／WX／UNDEBUG回归通过，期限边界的初始fixture错误期望保留在日志。

新增weapon_torque_bow_cleanup_anchor.inl，只能在实际press前从当时当前Frame捕获准确EquipmentIdentity和journal serial；旧ExistingActionLifetime在soft input loss后仍按原合同失效，未改HandClient契约。清理anchor重复认证原world／controller／pawn／weapon身份、双向所属链接、Native注册和GT，且不授新Frame、普通release、姿态或射击许可。仅演员指针元组不能排除world-generation／ABA，因此新增独立只读GearsCaptureOriginalInputCleanupContextV1合同和PG查询客户端；准确Native模块缺导出或不能证明同一世界实例时一律拒绝。**Native实现当前不存在，唯一Native/C0写者本轮优先处理Lancer真实集成问题，未承诺该接口完成；本枪没有越权新增Native writer，也不会因此打开Charge。** 真实HandClient soft revoke＋owned-memory actor字段测试通过，但Native世界证明边界是mock，不是运行ABI验收。

原取消执行器改用预先捕获的cleanup anchor；packed Bool清理使用既有整字compare/exchange，邻接位变化则拒绝，不覆盖邻接位。准确已拥有但原press在进入Charge前被拒绝／未转态的情况，只停输入和清本轮pending并核对完整中立；不人为调用ChargeEnded、GotoState或EndState。实际Charge仍完整沿原取消链，必须看到正常自动Charge.EndState恰一次。unknown步骤只读协调，已执行步骤不盲重放。原取消fault-injection回归通过。

weapon_torque_bow_input_owner.inl加入唯一原按钮journal：reserve→press-inflight→down→release-inflight／await-neutral，调用前记录准确serial；unknown保留原责任并拒绝普通release重试。清理先fence后核对原状态，完成前不丢旧owner。清理已完成但原弹体回执尚未记账时，journal继续保留，再次服务只等待回执，不重新运行原取消链。实际已发箭的弹量变化单独返回，不将“输入已中立”冒称“不发箭取消成功”。该journal尚未接共享R2 tick或完整高层编排，不是已开放游戏动作。

独立torque_bow_projectile scope加入现有同步Call consumer，Bow仍不进入通用supported_r2_fire／instant-fire或Boomshot许可。只复用既有严格枪口来源、typed getter compare／rollback与FastTrace模式；原GetProjectileFirePosition、SafeSpawnLoc、掩体修正和原弹体碰撞逻辑不改。准备枪口前必须已保留本枪实际CanReload非动作veto，保留表满或无本枪准确绑定均拒绝；不授物理换弹权限、不改弹药。原WeaponProjectiles[0]按Engine.Weapon实际ArrayProperty／ClassProperty、原Arrow祖先、资源身份和重复读取认证，不调用ProjectileFire来获取配置。

每个原ProjectileFire root还必须属于本轮已journal的准确R2 serial；仅准备好枪口不能收养外部原发射。准入serial随原root保存到实际返回回执，getter写入继续核对同一owner。只观察实际原ProjectileFire返回对象、准确原类、Owner／Instigator及原shot packet，callback不查询弹药、不调用引擎或写弹体。GT随后必须同时取得该真实回执和原库存准确1→0单箭扣减／备用不变，才向真实TorqueBowInteraction交付一次；单独弹量减少不能当发箭证明。模型已接受后若回执退役未完成，仅重试退役，不重复交付弹量。重复第二箭、错serial／world、库存冲突或未知读回均保留责任；失追后已经生成的真实箭报告safety_violation并保留扣弹，绝不退款。

最终定向journal回归真实执行0，包含实际生产journal＋真实纯交互模型；原按钮、状态、cleanup witness、弹体队列边界为注入后端。原Arrow class／独立scope的owned-memory回归也通过。初次新增fixture填错ObjectIdentity known／对齐字段导致断言失败，以及完整构成中局部observed遮蔽既有全局的C4459编译失败，均保留并修正；未关闭/WX。最终完整私有Playground编译链接0，没有运行测试目标或启动游戏。

完整2092源和33份当前Bow源码、DLL、构建参数／日志、最终定向测试源码与制品按准确SHA归档analysis/torque-bow-20261109/ownership-r8-final，详细development-review-r8.json。归档是源码／编译检查点，**没有stage、准备新运行候选或重新占窗**。当前root其他枪的后续共享变更不冒充已包含于此私有编译；未触Git index／commit，公共v71不变。31716整次失败和完整交窗结论保持。

剩余必须完成：唯一Native写者的同世界只读清理证明；既有共享R2唯一分派与完整模型／真实控制器／原状态编排；原箭显示、测量槽位、预留库存和4游戏秒奖励事务；四套掌面／指节／腕／袖口实际双眼接触；新冻结候选获得新的最终唯一交窗后实际Charge／release／R3取消／疲劳／中断证据，以及实体头显与交付。整枪和四部件贴合仍未接受，目标active。

## 15. R2高层驱动与共享分派已编译（2026-11-09，未stage／未运行）

weapon_torque_bow_fire_driver.h现在组合既有TorqueBowInteraction、既有XR累计边沿Decoder和既有原按钮journal；weapon_torque_bow_input.inl为生产backend及正常GT入口。没有重写纯交互模型或另起R2输入生产者、Hook、线程、骨骼writer。开始、正常松开、R3非发射取消、unknown只读协调、弹体先记账后状态回执、原完整中立退役均由这一驱动串接。Backend仍逐次认证当前Frame／原actor／held／ABI／枪口，模型的绑定布尔值和缓存不能代替原调用许可。

共享weapon_lancer_r2_native.inl加入Bow独立pre-original、正常tick和equipment yield分派，enable拒绝旧Bow责任尚未退役。相邻GN写者已明确留出这些准确hunk；现有GN prepare／ready／clear、角色／scope、Boomshot及通用分支保持。开始Bow前要求既有R2／L2、Lancer初发／返回、LS scope、HOD、Frag及启用的HB责任中立；旧Bow责任在失去选择或停止时只走非发射清理，不开始新输入。未加入通用supported_r2_fire或instant-fire资格。

原PlayerTick前先服务新输入／R3，再为已拥有的原Charge.Tick或Pawn延迟释放准备当前枪口。前后入口读到字节相同的同一包，只服务已有责任，不再生成边沿、up观察或续期；同一准确Frame的已认证枪口在保留原100ms期限／20ms余量及实际reload veto时可复用。R3取消前先提交本次原输入包的Native veto；前一包的veto即使过期，也不会因本轮取消立即完成而跳过新包的消费。

原状态读取新增实际Active标签，只有Active且完整停止可reserve／press；中立的PutDown或其他状态不是新的Charge资格。生产时钟读取并重复认证实际Engine.WorldInfo.TimeSeconds及原world／Native实例，将游戏时间换算微秒用于模型stamp；没有以QPC替代游戏时间或写原计时器。**原箭首次物理装填开始、4游戏秒奖励与库存事务尚未接入，不能称物理换弹计时已经通过。** 普通持有期间重复的只读ABI巡检在驱动已绑定或拥有责任时跳过，原31716冻结日志／候选不改；当前巡检字段明确为preparation_only。

快收快取的同一个原弓actor也比较真实hold_generation；新持有代次重新建立控制器基线，不接收收枪期间的R2边沿。Decoder／驱动／journal均保留实际输入来源边界，模拟来源与物理来源切换不转换为边沿或原弹体资格。原世界切换且全部旧责任已确实退役后，新模型游戏时钟可以从新世界的零开始；同一世界的倒退仍不伪造时间。reset同时拒绝任何未完成的装填步骤／请求，不能成为丢库存责任的捷径。

纯模型补两个真实时序边界：已派发begin内出现的准确原箭可先记账，未派发begin仍拒绝箭回执；真实追踪UP观察可在取消后重新arm，但不生成普通EndFire边沿。疲劳已发箭时R3不能吞掉真实R2 key-up。原press／release返回unknown时不重放调用；若原箭已经完整记账而原状态仍为Fire，只清本journal输入／PendingFire／bPendingEndFire，等待原Fire自然结束，不调用ChargeEnded／状态转换／手调EndState，也不称“不发箭取消”。这个post-shot入口必须已有准确回调＋扣弹记账；单独Fire标签不够。原未知步骤和突发未认证Fire的拒绝／责任保留规则保持。

定向验证实际通过：纯交互、原取消／post-shot fault injection、高层驱动＋真实Decoder、实际journal＋真实模型四组W4／WX／UNDEBUG，以及输入来源边界回归。驱动覆盖短按、同帧R3／R2松开、延迟Pawn释放、真实UP再arm、unknown、失追／过期、疲劳、旧实例／新hold、世界时钟归零及未退役拒绝reset；模型和decoder是真实实现，原调用／Native世界证明／状态／弹体后端仍为注入。初次驱动的中立回执衔接失败、fixture未提供新鲜Active观察的断言及早期日志均保留；修正后才通过，没有关闭/WX。

最终完整私有PG编译链接0。analysis/torque-bow-20261109/driver-r9-final保存2095源、36份当前Bow源码、准确R2路由、DLL及编译／定向测试制品；四组MSVC实际依赖JSON的24份项目源／头全部与归档源码一致，未发现编译后改写。它是在已保存R8构成上叠加本枪修改；CPP只有两个独立include（归档构建器归一换行为LF），没有把其他作者随后root变动冒称已经全树编译。DLL SHA256 0e40a4e47db7a68e0265ae2e167f88f60f83ff614a10768a0e4adbaf86f6f14a；详细development-review-r9.json及独立patch。

本轮再次检查当前Native源码，GearsCaptureOriginalInputCleanupContextV1仍无实现／导出；本枪未越权改写或构建当前Native，缺证明继续拒绝press。以上只证明源码接线及私有编译，不是实际原Charge／R3／疲劳或世界切换清理验收。未stage、准备新候选、启动游戏或重新占用已交出的窗口；公共v71、31716失败结论和Git index／commit保持。

下一步仍须完成原箭显示／实测固定槽／预留库存及4游戏秒奖励、四套实际自然手型与无阻挡双眼接触，并与唯一Native写者闭合同世界只读证明和既有Call正常返回观察。死亡／剧情／读档必须在原actor所属链接消失前沿既有安全动作边界清理；这部分尚无实际证据，不能用本轮soft-pose注入回归代替。随后需新冻结完整候选、新的最终唯一交窗、真实游戏定向动作、实体头显和交付。整枪及四部件贴合仍未接受，目标active。

## 16. 原箭资源复用与原弓表面审计（2026-11-09，源码／私有编译）

原MagazineMesh0准确指向StaticMesh'Locust_Torquebow.Locust_Torquebow_Rocket'；重新按原export SHA解析默认组件和两个socket。Muzzle在b_TB_Root的原坐标约(58.0071,0.1089,8.7500)，ChargeUp约(48,0,9)，未发现专用入槽socket。原组件未出现Translation／Rotation／Scale标签不等于运行时单位缩放，仍须读取实际继承值、实际模型缩放与父级关系；MagazineMesh0剩余4字节不透明尾保留。准确来源及可重现读取器保存original-component-and-sockets.json和audit_original_bow_resources.py。

原九骨骨架和LOD0完整蒙皮审计发现48个跨骨三角形；因此继续保留原整副skin／弓臂联动，没有按独立刚体拆弓。基于原Root权重三角形的12个截面及原模型三视图均已亲看：Muzzle轴在多数截面进入弓体，不能直接当手持箭的外部槽口。四个限定roll方向的静态箭／弓Root顶点匹配仅找到少量重合，未证明原箭入座变换；该有界负结果不等于排除其他变换、缩放或原动画。固定槽位仍未接受，不以扩大插入半径或猜一个枪口终点闭合。

恢复了原TorqueBowAnims八条九轨动画的准确审计，以及原CustomAnimSets所指Marcus Torquebow六条75轨换弹动画的准确审计。原保存脚本Notify_AmmoGrab／Notify_AmmoReload分别通过SetPawnAmmoAttachment挂到左手socket和卸下原箭；反编译文本的参数缺失／本地变量错误仍不能当完整ABI。原弓动画、手部换弹及socket的相对位置尚未全部解出，不能称已获得插入终点。

共享weapon_part_assets增加通用create_existing_static路径：只保留已认证的原StaticMesh／实际材质引用，创建自己的隐藏临时StaticMeshComponent，不构造原LOD、不重建UV／材质或RHI、不接管原网格root。组件最终世界缩放必须由具体消费者提供并认证；没有默认把原箭缩成单位大小。SetMaterial仅对自己的组件调用，随后GetMaterial准确读回；新增完整输入／返回布局、重叠、额外参数、链环、MaterialInterface及Engine包校验。原材质对象本身不修改。

检查时发现旧bind_mesh只允许池拥有root的网格，会挡住新借用模式；现按“自己拥有root”或“借用且没有root”分别准入，混合责任拒绝。实际生产reserve／bind／cleanup主体有定向回归：借用模式只释放自己的组件，绝不unroot原资产；旧构造模式仍释放自己两种对象；隐藏失败保留清理责任，混合责任隔离。原组件不被SetMaterial或换mesh。已有set_pose的真实HandClient／左手ammo或magazine lease、当前Frame和light环境要求保持；资源准备不授装填或库存权限。

定向两组W4／WX／UNDEBUG真实执行0，MSVC依赖JSON的12份项目源／头均与完整编译构成一致；Engine调用和实际画面仍未验证。R9准确冻结基底仅叠加上述7份资源文件，完整私有Playground编译链接0。不可变original-resource-r10-final保存2101源及原始资源／截面证据，manifest SHA256 f10c29a569aef2707b95871fc65f07cea36e4aae3a3fb9a76f716594d0fa8f0e，PG SHA256 768a91ff1039bc4929b36cf9b748d8006a59466180a967fc4175df3bb729f8ab；详细development-review-r10.json。当前root的其他枪后续共享变更未吸收，未冒称root全树编译。

原箭具体显示消费者、实际原材质／缩放采样、原始控制器尖端到固定槽的映射及库存／4游戏秒事务仍须接入；此编译点没有启动或stage新候选、占游戏窗、改Native或Git index／commit。31716失败、后继最终交窗、公共v71及全部实际游戏／双眼／头显未验收状态保持，目标active。

## 17. 原箭实际属性与父socket缩放采样（2026-11-09，源码／私有编译）

原箭选择器现在复查已读字段、结构、对象、名字及对象／名字表；新增独立attributes读取实际原MagazineMesh的Scale、Scale3D、AbsoluteScale及当前头像mesh的LightEnvironment。所有值按实际反射类型、维度、大小、范围和重复读取验证；当前mesh必须仍是同Pawn.Mesh，原箭组件／资产／武器绑定在前后重读中保持。不以缺少默认标签或静态分析里的1代替运行时值；也不把主枪GetMaterial当成原箭材质。

核对原SetPawnAmmoAttachment的左手socket父级后，进一步加入实际骨骼资产Sockets数组和Pawn最终左右socket选择的只读验证。按实际bIsMirrored选择LeftHandSocketName或RightHandSocketName，只接受准确且唯一的SocketName匹配和该资产内的SkeletalMeshSocket，读取其实际RelativeScale。数组／inner描述、重复socket、错类型、越界、读取变化及不完整父级均拒绝；元数据字段按身份去重，保持原64字段上限。原Marcus左右socket静态审计有3份手部条目／共15个socket，LeftHand位于b_MF_Weapon_L_End；RelativeScale未显式序列化仍不当单位值。准确export SHA和可重现读取器保存original-marcus-hand-sockets.json及audit_original_bow_parent.py。

新增纯缩放组合保留原component scalar与三轴值，非AbsoluteScale时再乘实际socket相对缩放和已认证当前committed头像model_scale；AbsoluteScale按原语义忽略父级乘数。结果必须是有限、正、在原箭挂接支持范围内的统一缩放，非均匀／镜像缩放不被平均成可用尺寸。没有拉长原箭或改变原顶点来够槽口。此为依据已读原属性推导的目标世界缩放，尚未由游戏LocalToWorld或实际渲染读回认证。

weapon_torque_bow_arrow_appearance.inl只读原箭组件的GetMaterial(0)，在严格原Engine.MeshComponent ABI下重复查询并比较准确MaterialInterface身份；原箭绑定及当前Native Frame在两端再认证。头像数据来自现有公共handrig／committed合同，未借Lancer私有换弹owner，也没有新骨骼writer。这个读取器尚未由物理装箭资源消费者调用，未创建或显示一支游戏箭，不等于原箭显示已完成。

实际生产选择器／attributes／parent和纯缩放代码的W4／WX／UNDEBUG定向回归执行0，覆盖原组件身份、原资产包、实际typed scale／light、socket父级与镜像选择、重复匹配、读取中途变化、AbsoluteScale、头像与socket缩放以及非均匀拒绝。原Args／Engine.GetMaterial和Native头像数据并未由此fixture执行；初次fixture把已经是RawName的值再次包入RawName导致的C2440保留，仅改fixture构造后才通过。最终MSVC依赖JSON的32份项目源／头全部与归档完整构成一致。

最终完整私有Playground编译链接0。arrow-appearance-r11-final保存2105源、40份当前Bow源和准确DLL／构建／测试记录；R10的7份资源文件SHA保持。manifest SHA256 460d9b9d683575e4ff79719e86875902cd4189949934f1414e6153ea2bb70e35，PG SHA256 1a462d6de7bcf84d0f6cc19cee92d4441e09bf2cb39016111956aa83fe5b11bb；详细development-review-r11.json。构成仅在R10基底叠Bow源及独立include，没有吸收其他枪后续root共享修改或冒称当前root全树编译。没有stage、启动游戏／新候选、取得运行窗、改当前Native、公共v71或Git index／commit。

下一点是把这份原箭外观接到既有临时组件／真实左手ammo lease，再闭合原始控制器挂接、实测固定槽与连续插入、原库存预留及4游戏秒事务；四套实际掌指腕袖姿势、唯一Native清理世界证明、真实Charge／release／R3／疲劳／中断、双眼／头显及交付仍未完成，目标active。

## 18. 原箭组件消费者与原始箭尖链（2026-11-09，私有编译，未接正常GT调用）

weapon_torque_bow_arrow_tracking.h复用既有同Scene rig／HandClient数据核对，只取同一原始左控制器的world_grip；原网格中心和原箭尖由同一挂接及准确原tip坐标推导。挂接内保存的中心／箭尖／米制尖端必须一致，单位和缩放必须与当前rig／原箭外观相符；错Scene、错输入包、缺左手世界位姿、方便位姿与原始控制器不符或晚于原100ms均拒绝。输出保留原sequence／sync_ns，没有C0／Snap输入或续期。实际代码的合成源测试54检查通过；箭尖与中心刚体移动及原点重建一致。

新增weapon_torque_bow_arrow_visual.inl，把R11原箭外观接到原有组件池的create_existing_static／advance／light绑定／set_pose／destroy，只有自己的隐藏临时组件，没有原组件setter、原材质写入、弹药或装填调用。准备可以跨Frame推进同一handle，失效不再创建第二个；材质／缩放／父级变化或持有代次变化先退休原handle。公共drain已清理的准确旧handle可正常退役，池槽被其他generation复用时不收养或销毁它；隐藏／清理失败保留原责任。

显示同时要求当前真实ammo lease、准确press token、原交互模型仍持有而未入座的一支箭、模型身份、该Driver真实绑定的hold_generation及当前资源父级一致。两个只读Driver getter和模型身份／held_arrow_serial getter不修改布局、库存或输入；不会给模型暴露可写引用。单独同一弓actor不够，快收快取后的旧Driver／旧箭在新资源代次明确拒绝，必须来自新持有模型与新Grip。已入座或释放的箭不再随手显示。

自己的组件先隐藏设置原始位姿，再调用原Actor.ForceUpdateComponents，明确bCollisionUpdate=false／bTransformOnly=true，限定当前own component／ammo lease并检查准确两个Bool参数、掩码、返回位置与参数链。随后按原Engine.GetAxes的离散Rotator基准读取实际LocalToWorld；位置／旋转／缩放匹配后才显示，显示后再次读回。任一步失败均撤回。原共享资源源文件未改、原网格／RHI责任不变，没有新Native／C0 writer或Hook。实际矩阵读回仍只是CPU组件观察，不当GPU像素／双眼／自然贴合证明。

组件owner实际生产主体的49项CPU检查通过，覆盖隐藏准备、真实模型预留、lease误配、错矩阵、原更新失败、来源失效、同actor新hold拒旧模型、隐藏／清理失败保留、公共drain先完成和材质变化退役。该fixture使用真实纯模型、原始rig核对和组件消费者，Driver绑定、Engine／外观／资源池为注入后端；原Actor更新ABI并未在游戏执行。另跑真实Driver／模型／Decoder回归，新增只读代次视图检查通过。旧输入取消、疲劳及unknown策略保持；未开放物理reload_plan。

首次完整编译暴露Windows near宏与新增near函数名冲突，仅改为close_vector；随后修正ue_angles需传rotation_matrix而非Quaternion。未改其他枪源码或降低警告。原测试初始main插入位置、缺少Quaternion dot重载、合成源移动时未同步stereo sequence、fixture的release_required默认1及跨namespace检查计数等失败均保留日志；最终四组W4／WX／UNDEBUG（interaction、tracking、visual-owner、driver）实际执行0，32份MSVC项目依赖与最终归档源码一致。

手型方面，使用原Marcus完整混合权重皮肤重建当前shaft15节表，保持全部骨骼translation／scale，抽出1853个手部表面三角形。首个夹持点仅按食指／拇指末端表面选点，原间隙约0.683raw，未接受。随后只对食指MCP／PIP、拇指opposition做75个小范围离线候选，对排名前5作原手／原箭三角面检查；最佳仍有79组超过0.1raw交线跨度的表面相交，集中于中指／食指／拇指。两张四视图已亲看，最终图覆盖完整手与箭；首图近景有裁切，未当完整视角。没有signed penetration-depth／手自交／完整袖口／动态／运行时相对缩放或双眼验收；**这些挂点与开指候选全部拒绝，未接产品，也没有为此缩箭、改骨长或扩大槽判据。** 证据arrow-shaft-current-profile.json／png、arrow-shaft-opening-candidate.json／png及两个可重现producer。

最终完整私有PG编译链接0；不可变arrow-visual-r12-final保存2111源、46份当前Bow源、DLL／四组定向测试／依赖／原表面候选与失败记录。manifest SHA256 06c5e166bf41a437599406dd6ec3d8a2043f226262e1fbf5cf6056871dfe209c，PG SHA256 421318cf728758363fe78da97131f462e6ae7608fb973b78cc5ae9e00724bca3；详细development-review-r12.json。构成沿R11基底叠本枪源及3个独立include，未吸收其他枪后续root共享修改，不冒称当前root全树编译。

此时组件API已实现并编译，**正常GT的warm／show／withdraw／retire分派尚未接入**，没有在游戏创建或显示原箭。下一步先修真实shaft手面／挂点，再接既有正常GT和生命周期入口，闭合实测固定槽、连续原始插入、原库存预留及4游戏秒事务；其余三套手型、唯一Native清理世界证明、真实原动作／中断、无阻挡双眼／头显及交付仍待完成。未stage、启动游戏、取得运行窗、改Native／公共v71／Git index或commit，目标active。

## 19. 隐藏预准备与生命周期分派、原版装填姿势参考（2026-11-09，仅源码／私有编译）

现有正常GT的torquebow_runtime现在为已选中的原弓跨Frame准备同一个隐藏原箭组件，绑定完成后不反复查询材质或重做构造；真实R2／Charge责任存在时跳过该准备和只读ABI巡检，避免占用动作的来源期限。隐藏组件不计入fire-input pending，资源责任继续由既有公共池统计。停止、换枪、新hold或来源父级变化使自己的组件退休；旧slot被别代复用时不收养。已知材质／缩放变化在show时退休旧缓存，未知读取则撤回。

共享equipment Frame通知只增加一行本枪on_frame及CPP前置声明；它仅作CPU撤回标记，不读游戏来源、不调用Engine或setter、不直接销毁。可见组件遇新Scene／sequence／原sync即要求既有公共GT drain隐藏；身份、参考系或生命周期改变则标记准确旧handle退休。输入yield／失追先fence并结算原输入，随后才允许显示组件清理调用Engine，保持原取消责任先行。没有新增Hook、线程、C0 writer或原组件写入。

原组件消费者＋正常GT隐藏预准备的W4／WX／UNDEBUG定向回归实际78检查通过，模型／原始rig核对为真实实现，Native／Engine／appearance／pool仍是注入后端。覆盖跨帧隐藏准备、缓存不重复查询、活动原输入跳过准备、新帧仅标记、hold变化、停止／换枪和材质变化退役。完整私有Playground最终编译链接0；28份MSVC项目依赖全部匹配准确构成。不可变arrow-warm-r13-final保存2111源、46份当前Bow源、DLL、构建和定向检查／原始失败记录，manifest SHA256 87bfb0a1a14687083e52c38f66eb1ace681db6f2488cd113fcc5155e6ba9a6a9，PG SHA256 c97d99f0b95b4c2278c5a07ea762956d0a9cfecda4e908fc73c67d6f625ad754；详细development-review-r13.json。沿准确R12基底只叠本枪源与上述两个共享hunk，没有吸收当前root其他作者后续修改；R10共享资源源文件保持不变，不冒称root全树编译。

离线表面覆盖纠正：上一节R12的1853个三角采用全部顶点已选中，漏掉38个混合边界面。本轮采用任一顶点受手骨影响，完整手面为1891，前臂／袖口／手面为3146；旧记录保留，不追改为已覆盖。固定全部reference translation／scale，快速15指节FK对独立完整FK最大误差约4.2633e-14raw。圆柱代理漏箭头／尾翼，改原箭513截面的最大半径包络后仍有交线；加入边／面轴最近点、撤掉强制中指接触及90个局部中指候选也未获得合格整手。手自交及混合边界检查进一步拒绝这些候选，未改产品角度／挂点／箭大小或槽判据。

进一步解出原Marcus Torquebow AR_Reload的75条track、60个key，全部translation／rotation span与原export精确匹配；多key为带旧24字节范围头的Fixed48NoW，单key为Float96NoW。codec对照固定版本的UEViewer原始实现：[UnMeshTypes.h](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMeshTypes.h#L136)及[UnAnim3.cpp](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnAnim3.cpp#L1097)。原Notify注释显示AmmoGrab约0.44秒、AmmoReload约1.5008秒；child ScriptName及实际key时间映射尚未运行认证，原2秒动画也不取代用户要求的4游戏秒奖励期限。

以原b_MF_Weapon_L_End及LeftHand socket显式Roll32768重建4个原作者取样姿势，保持全部手指reference骨长。finger translation最大差约1.3113e-5raw，socket未显式location／scale仅供离线按零／一假设，不能替代实际属性。箭在手腕内的方向与旧草拟沿腕前向明显不同；key15／25／35／43分别有0／86／51／93组大于0.1raw交线跨度的箭／整臂表面相交。两页各四幅、共四key两视角已亲看，key15零交线仍有接触间隙，其他key明确相交；没有把原作者动画直接当可接受Grip。原codec解码、静态socket缺省、实际运行父级／相对缩放与最终可见对应仍须闭合。参考姿势和挂点均未安装，新的方向只提供下一次接触修正依据。证据original-bow-reload-pose-r13.json及original-arrow-grip-r13.json／两页PNG。

当前**正常GT隐藏warm／withdraw／retire已接入，show和物理装填仍未接入**，尚未在游戏创建／显示原箭。原库存预留、实测固定槽连续插入与4游戏秒事务、四套完整掌指腕袖双眼接触、唯一Native同世界清理证明及安全原Call正常返回观察仍开放。未stage、准备新运行候选、取窗、启动游戏／XR、改Native／公共v71或Git index／commit；31716首败和最终交窗记录保持，整枪／头显未接受，目标active。

## 20. 原接受GAME时钟与单箭库存桥接（2026-11-09，适配器源码／私有编译，正常玩法未开放）

核对原Reloading.BeginState保存脚本确认ReloadStartTime取WorldInfo.TimeSeconds；该反编译文本只提供静态依据，不取代实际反射ABI或游戏证据。纯模型原先把收到接受回执的stamp.game_tick当起点，迟到处理可能延长4秒期限。TorqueBowReloadStart现在必须显式携带original_start_tick，缺失／负数／未来起点拒绝，原时间零有效；开始后重抓和暂停均不重启。模型允许已经获得准确原装填接受、自动补弹已抑制且原状态完全quiet的Reloading回执，不再要求它仍显示Active／ready；Charge／pending input／身体动画仍拒绝，新Charge的Active／ready要求不变。实际原开始1000、回执1700、闭合5000仍完美／5001普通的定向用例通过。

共享原装填服务只增加opaque serial的accepted_game_start只读查询，重复验证准确原lease、实际ReloadStartTime与原reward_start，返回原GAME起点，不开放lease字段写入。停止自动装填动画的既有原ABI路径增加Bow分类：原StopWeaponAnim和StopBodyStance只停止本弓reload片段／身体通知，保留原Charge／Fire、多骨skin及其他枪条件。原PlayWeaponReloading会启动身体stance、通知、定时器和可选枪clip；因此只清库存完成timer不足以避免另一支原箭由AmmoGrab挂出。此次新增停止逻辑仅编译，尚未在Bow游戏里执行／验收。

weapon_torque_bow_reload_live.inl实现本枪original reload journal与原库存桥接。它不再建立第二份交互／库存意图模型：接收现有TorqueBowInteraction完成连续入槽后生成的那一个request，匹配原actor／Native实例／hold、真实ammo lease、原press token和原reload serial。首次预备只保留库存观察与奖励资格，实际原调用前捕获既有同世界cleanup anchor；该Native证明缺失仍拒绝。一次真实press在预备后进入消费floor，即使原begin未派发也不能用同一press重启。

原ForceReload通过已存在begin_for_part(...,ammo,...,start_dispatched)执行一次。未派发可放弃预备；已经派发而返回／serial未知保留责任，绝不按“未发生”丢弃或再次调用。准确接受后只读取原GAME起点，交给同一模型的装填开始回执；不写原ReloadStartTime，不使用墙钟、不等动画计时补弹。当前未知begin且serial0仍须安全原正常返回／原状态证据才能进一步结算，不能称该路径已完成。

闭合桥接要求原model request的身份、serial、arrow、原库存、期限及完美结果一致，沿现有InsertionWriter调用原SetSpareAmmoCount再SetAmmoUsedCount，单token最多派发一次。结果未知只读协调实际计数；仅本token准确reserve-first半写可补偿。**一旦已经确认完整插入，后续loaded／reserve变化直接保留冲突，不能再按半写归还箭**；已提交箭不因停止／失追退款。未派发库存请求遇停止则直接退出原装填，拒绝后续迟到补交。

退出只沿既有原非补弹Abort／timer清理，每次原退出调用前记账，结果未知不再派发；后续只能观察真正已退出且timer停止。完美奖励复用既有资格、原事件去重及bonus／模式结算，不重复分数／反馈；普通闭合不冒称完美。原退出、实际库存和真实当前中立状态都得到读回后才提供action receipt；调用方必须用Driver自有stamp／event确认同一模型，桥接检查该模型已无pending／fire、实际计数及revision一致后才可退休。这里的revision为各owner本地观察计数，不伪造原游戏字段。

实际W4／WX／UNDEBUG通过四组：纯模型、原reload桥接165检查、箭组件owner78检查、原fire Driver／真实XR Decoder。桥接与同一纯交互模型为生产代码；Native、原Engine、共享Reloading、原库存及奖励后端为注入，未当运行ABI证明。覆盖迟到原起点、真正四游戏秒边界、正常／未知完整插入、拒绝／精确半写补偿、已提交后计数改变不退款、未知ForceReload serial0保留、未知Abort不重放、timer仍活不退休、奖励不重复和同press拒绝。四组36份MSVC项目依赖与最终准确编译构成一致。

最终完整私有Playground编译链接0。不可变reload-bridge-r14-final保存2113源、48份当前Bow源、DLL、构建／定向检查及保存原脚本；manifest SHA256 1331a3e54cbc317213713399c101114f648109dc154821c5e436ac1602348a2c，PG SHA256 3a8d36f3ab9e06f3c7c3134df2aee46bd41f1bfb173a8bc407ddf3c70cacb3dd，详细development-review-r14.json。沿R13基底只叠本枪源、独立include及准确公共reload服务增量；GN已经存在的begin_for_part／派发见证和gnasher动画分类作为显式依赖导入，原begin wrapper固定magazine保持，GN作者确认当前不编辑这些hunk。没有将其余GN／其他枪root WIP混入或把私有全文件回迁；R10资源源文件不变，不冒称root全树编译。

**本适配器尚未由正常肩取／入槽流程调用；production reload_plan_verified仍false，Driver仍未开放reload request分派。** 下一点是给既有同一Driver增加有界、带完整清理责任的装填入口／回执服务，并接真实左肩proposal／ammo grant／raw连续槽行程及生命周期统计，保持模型只读外部视图，不能暴露可写模型或另起owner。原箭show与四套自然手型、实测固定槽、唯一Native世界证明和原Call正常返回仍未合格。没有stage／新候选／游戏或XR／取窗／Native改写／公共v71或Git index／commit；31716首败和完整交窗结论保持，整枪／双眼贴合／实体头显均未接受，目标active。

### 2026-11-09 统筹专项方案／进度核对

已按主人这次专项询问写入独立analysis/coordinator-plan-review-20261109-2243/bow.json，完整8步与A–E五块均保留；采用统一0／.25／.5／.75／1证据等级，A20%、B30%、C20%、D15%、E15%各块当前均.25，加权工程开发／可交付评分25%，不是工时或头显通过率。主要缺口仍是实际原动作与同一Driver装箭闭环，不能用大量已编译适配器／测试／冻结数量代替。计划纠偏为先完成一个可观察空弓→取箭→锁定→原蓄力／发箭循环，减少每个微小增量单独封套及未改逻辑的ABI／旧方向离线调姿重复。

专项重读还发现first-runtime-result.json及first-runtime-closure-proof.json的31716实际image字段均指向J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/...，不满足现行允许目录证据，不能继续把该轮称为当前购买版允许路径验证。原肩取／只读ABI／失败及完整收尾记录原样保留；没有改写历史或新启动程序。下一份可运行候选必须先采用现行J:/gearsVr/Gears of War Reloaded或授权Steam目录并核对精确身份，不能沿旧准备路径直接重跑。报告本身未新增编译／测试／运行，也不占任何窗口，整体目标active。

**后续直接主人指令校正：** 统筹随后传达主人已明确提供并授权J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64用于验证；2026-10-21旧AGENTS不能覆盖这条后续直接指令。因此撤回上一段把该目录判为未授权及必须切换Steam／另一目录的限制，不再据此卡住候选。原记录的路径事实保留，仍需核对精确EXE／进程、保存／输入／后台保护，不涉及DRM或许可证修改。独立bow.json已附同一授权校正。

## 21. 同一Driver的装填／原输入连续链（2026-11-09，源码接线／私有编译，未实跑）

既有Driver现在拥有有界begin_reload、release_reload_arrow、regrab_reload_arrow、complete_reload_motion入口，仍只持有同一个TorqueBowInteraction、原XR Decoder和原R2 journal。原装填接受、连续行程闭合、库存／退出／奖励回执均由该Driver统一stamp和event序列处理，没有对外暴露可写模型。原库存桥接返回实际回执后先确认同一模型，再退役准确journal；若退役未完成，不重新ack／发原调用。真正的原发射记账继续沿原R2分支，不用装填请求伪造弹体或扣弹。

装填责任在原begin调用前发布到既有R2 pending，未知begin、原退出、奖励及部分库存保持原责任。停止／失追先进入已有fence和cleanup，未派发的闭合token也交给桥接保存并以实际未改库存／已退出回执拒绝，不能丢token或迟到补交。只有本枪原world／actor anchor允许软pose失效后读已完成的库存与原中立状态；这不授新跟踪／姿态／setter权限。无新帧时超100ms的装填也沿正常poll进入原清理，不留下等待新动作才能退出的原Reloading。旧世界或未退役责任仍拒绝reset。

原始连续行程入口另带实际input sequence起点。Driver在本箭预留／重抓时记录原输入floor与自己的模型sequence floor，拒绝旧箭、重抓前行程或当前包零行程；不会直接把Native输入序号冒充模型事件序号。模型继续核对component／config／reload／arrow、实际连续完整行程与原输入pose期限，重抓不改原开始GAME时间。新增只读pose_current和matches_source为原合同核对，不改变追踪输入或Native wire。

生产Backend将原shared begin／库存／退出／奖励桥接接到这些Driver服务，实际原GAME时钟、当前ammo grant、已准备原箭组件、原肩部body_slot及精确持有代次仍分别检查。正常running input tick已调用开始／松手／重抓；原输入与装填责任一并进入原R2停止和equipment yield路径。原before-original入口继续在单一Driver内处理已有责任，没有新增游戏线程、输入注入器、Native／C0 writer或Hook。

HandClient候选组合只增加独立Bow producer及committed一行，CPP仅加对应声明。producer只用同一Native Frame原始左控制器、稳定reference身体槽与已经准备好的本枪POD状态，不回入Native／HandClient，不调用Engine或骨读取。只有实际计划可开始且原箭warm ready时申请左肩ammo；已有准确本枪ammo持有仅作为同press incumbent保留。首次proposal的route在同一完整Frame固定，避免后续GT模型变化让回读漂移；只固定request／incumbent路由，不覆盖新鲜busy、动画、身份、挂点、原输入时间或其他真实游戏状态。新Frame必须重算。GN已确认同一else链各自只增独立分支，root保留全部peer改动，私有基底未混入其后来GN接线。

四组W4／WX／UNDEBUG实际通过：真实模型、真实Driver／XR Decoder／装填桥接连贯260检查、箭资源／route88检查，以及原fire Driver回归。连贯fixture在同一个Driver中装1箭→蓄力／松开→接注入原弹体回执且扣1→再装下一箭，未reset模型；还覆盖装箭后R3取消不发箭、真实UP不补射、迟到接受保持原起点、松手／新press重抓后5000000完美而5000001普通、旧行程floor拒绝、未知退出不重放、世界证明缺失保留及无新帧超期清理。Engine／Native／原库存／shared Reloading／原弹体后端仍为注入，并非真实游戏闭环。初次fixture第二轮未递增箭serial导致的断言失败保留；仅修正fixture按原生产backend递增，没有放宽模型anti-replay。

本轮按完整Driver逻辑改动集中构建一次私有Playground，最终编译链接0；36份MSVC实际项目依赖匹配该构成。reload-driver-r15-final保存2113源、48份当前Bow源、DLL、四组检查及首败记录，manifest SHA256 1eb192579da0e26db140e58dcd1e06ed12936ca53072ed74eb3717057736efa2，PG SHA256 d1dcf60c659f8195ed1e53901a4e3230630f8b30d22fd0f01ed94e90bf754363；详细development-review-r15.json。沿R14只叠本枪源及上面两处共享接线；R14原reload服务和R10资源源文件未改，没有回迁私有整文件、吸收其他作者后续WIP或声称root全树编译。

**实际原箭show及raw固定槽行程消费者仍未接通，production reload_plan_verified继续false。** 因此新producer不会凭空授肩部ammo，也没有开始真实装填／开火。下一点集中修原装填作者方向的最小箭挂点、认证固定槽和原箭尖，再把同Frame真实show／raw stroke接到已存在的Driver ReloadMotion，形成可运行完整构成；不能再用持弓／ABI切片代替首个装一箭并发出一箭的检查点。唯一Native同世界证明、原Call／自动Charge.EndState正常返回、四处完整自然掌指腕袖及双眼反馈、实际全中断／库存／头显和交付仍开放。未stage／准备新运行候选／取得新窗／启动游戏或XR／改Native／公共v71或Git index／commit，目标active，完整扭力弓未接受。

## 22. 原作者箭挂点、尾部入槽与正常GT消费者接通（R16，2026-11-09，未运行）

R16修正的是离线原动画解释，不是Native骨骼修复。R13已正确解出Fixed48原动画字节，但把非root动画Quaternion直接用于原始RefSkeleton时，遗漏了导入坐标约定。固定版本UEViewer的UnMesh3.cpp在2277附近对非root参考Quaternion的W取反；本轮保留原始RefSkeleton及骨长，仅将非root动画转回该参考表示后FK。原参考wrist与实际加权皮肤接近，反转原参考子骨会偏离约85raw；拇指参考至原动画由约135度变为约10度。旧错误约定预览、首败和全部R13证据保留，不能据此指称C0／Native当前输入存在该错误。主来源：https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMesh3.cpp#L2277。

原Bow native字段另证实MeshOrigin零、RotOrigin=(0,-16384,0)，不是任意component朝向猜测；Yaw-90抵消原b_TB_Root的Yaw+90。原左右两个Weapon_End socket及左箭Roll32768必须一起解释。正确原AR_Reload key44显示箭尾(+Y)先从枪口前方沿Root -X送入，箭头(-Y)始终朝Root +X；不能再用旧“枪口处于实体内所以不可能是入口”的推断否定该原动作。原最终source Root位置(57.69151886,-.00934544,8.77302037)，箭头Root X约76.41556641、箭尾X约38.96791784；原Muzzle Root X58.00706100，完整尾部行程19.03914317raw。此固定原行程不随计时或视觉Snap缩短。

最小持箭挂点使用原key25的source-in-wrist位置(5.15965322,-5.45713269,4.35798466)及原Quaternion；只由实际Marcus115参考签名、原wrist校准、avatar scale和units转换。箭的原StaticMesh／material／真实component scale继续独立认证，不改原模型或骨长。固定槽作者只支持当前实际Bow与原箭同正向等比缩放，拒绝未证明的相对尺寸／镜像；最终箭尖、尾部全行程和Root坐标均由原资产常量与实际scale转换。新作者的机械容差为横向6mm、角度8度、就位3mm，初始approach还加上3mm，最低有效行程保持完整19.03914317raw；没有放宽既有速度、间隙、anti-replay或追踪100ms限制。四组scale／units和任意Root旋转平移数学检查通过，仍不是实际拟合验收。

正常GT隐藏预备在现有Arrow warm完成后准备这份实际父实例几何，成功才允许配置完整reload plan；不再硬编码reload_plan_verified=false，也不会仅绑定半套fire-only模型。Native同世界cleanup证明和全部原ABI仍是配置前置，当前缺该Native导出，因此本轮依旧没有实际装填／Charge许可。模型reset后的新config serial必须重新准备，不能复用旧calibration serial。实际scale／appearance改变先停止已有责任、退役原显示组件，再允许重新准备，不能静默resize进行中的箭。

原有running input入口现调用持箭消费者。它重复确认实际ammo lease／model arrow serial／当前avatar和原appearance，获取同一认证RawRig的左右手；右手只经原不可变WeaponMeshFollow挂点恢复Root，左手只用原raw grip。两者按真实units转成机械米坐标，保留原controller sync与sequence；没有GT续时、C0／Snap／显示矩阵输入。重复相同包不重观察，倒序、断帧或失准必须重新走完整approach。完整FeedResult::inserted才提交同一Driver的ReloadMotion，继续由原Bridge负责唯一库存写入及原退出／奖励／回执。箭组件show也已接该正常入口，复用已有原material／lighting／真实矩阵读回ACK；原箭消耗、松手或失效立即撤显示。显示成功不构成入槽回执。

W4／WX／UNDEBUG五组实际通过：原箭挂点、原固定槽数学、同RawRig左右手／Root88检查、真实Driver＋Decoder＋Feed＋原reload bridge268检查、显示owner／route88检查。连贯Driver首次装填现由真实Feed recognizer的完整行程触发，最终近点单独不写库存，重复完成不再写一次；随后保留原R2 Charge／真实UP解码／注入原弹体回执扣1及下一次装填。Engine／Native／共享Reloading／真实库存和弹体后端仍为注入，不能称游戏闭环。原Driver R15未改部分证据继续保留。

首败记录保留：tracking fixture误以为通用WeaponMeshFollow保留近似非等比原矩阵，实际上原类按既有合同取平均；未改该类或容差。新Raw Driver fixture最初把近点至approach压在10ms内，触发原3m/s限制，仅改为30ms真实样本间距。Windows assertion的负退出码会逃过旧if errorlevel1，最终本轮脚本改为每个非零码中止，全部最终日志均有明确passed文本且退出0；首败及旧脚本不删。最初独立槽fixture超过速度的日志也保留。

原作者接触仍有未解决穿插。已亲阅修正key25／44双角度原网格图；正确1891手tri／3146手臂tri、相同原权重拓扑的cross检测中，key44箭／臂127对、self114对（相对bind新52对），不能写成自然无穿插。全Root4190tri与原箭在最终位置仍24对cross；该最小功能挂点与原尾部入槽路径不代表main／support／shaft／feed四处掌指腕袖已接受，也不通过缩模型、骨长或旧轴搜索掩盖问题。

完整arrow-feed-r16-final保存2113源、48份当前Bow源、41份MSVC实际项目依赖、五组检查、三份离线作者／audit及其图像、DLL与首败记录；manifest SHA256 6c47182904abe8002501efe6f4a59d1dd1af4f1f30dd05dcc05737ad5e661ab3，PG SHA256 be1679a86d2d369645d620cb66ce5886e6c5214caec2b0df1d97d15d247a8940。详见analysis/torque-bow-20261109/development-review-r16.json。沿R15只叠本枪源和CPP Feed include，资源／原reload服务／Native／C0／其他枪／公共v71及Git index／commit未改；不声称root其他并行WIP已编译。

仍需唯一Native同世界清理实现及原正常返回／自动Charge.EndState真实观察、四处完整自然手姿与双眼反馈，再以同一完整构成取得新唯一运行窗，首检装1→原Charge→真实R2UP→真实原箭1且扣1，随后同构成R3取消／重装／中断及亲阅配对实录。未stage／新候选整合／取得新窗／启动游戏XR，头显false，整体工程分数25%不随测试数上涨，目标继续active。

## 23. 四部位原作者V4接线与真实副握挂点（R17，2026-11-10，未运行）

本轮progress；R16不可变检查点未改。当前root与原动画JSON重新核验，原R13归档JSON SHA52b2c9d5ef289a3107aee21a167ceb7d0e11dd007d88bb5613a31008a0ddc6e6一致；按实际原MP_WarMachine包offset3080575读取24960字节，原AR_Reload export SHA1db9c32bde2c111961fe1ddf7150b37614753cea3f950257ef3fdb5e6da14812匹配。沿R16已校正的非root W约定，新增本枪artist_grasp常量，固定原Marcus115全部reference局部及骨长，没有再做旧轴角度网格。旧四套随意角度草案退出当前author入口；原归档与拒绝记录仍保留。

四部位分别有独立profile id：右主握把使用原key25的12个非食指关节，扳机压下3节用同原作者数据；释放3节用原MODEL reference，这是明确设计选择，不冒称额外Fire动画。左支撑使用原key0，箭杆用key25，入槽用key44；后两者有少量真实局部差异，并非强行共用一套握拳。所有author调用核对15节真实reference及完整模型SHA，单位但不匹配的reference仍拒绝；V4 Native再次独立匹配实际当前C0模型、全部参考、Frame与Broker。右主握只改15指节，真实扳机端点只影响食指3节；不动腕、枪姿、骨长或输入。

新增weapon_torque_bow_grasp.inl接既有V4导出，正常input tick已发布主握；同一认证RawRig持箭显示成功后，原箭握持与沿槽阶段分别发布对应局部表和相同物理grip。原装填／箭责任结束后，副握只按当前真实support lease或已释放的真实近点发布；原Charge期间副握仍持续，不因R2 journal pending错误撤掉本枪姿势。松手／release_required／不匹配press／别的物件incumbent／失追／忙碌／pinch都不能偷取副握。发布是可选cosmetic数据，缺导出或拒绝不改模型/库存/原动作，不创建另一个C0 writer、Native hook或controller输入源。

原AR_Reload key0的左腕在Root内位置(43.5271935961,-8.5617759130,.1695676694)及Quaternion已保存。准备阶段使用现有不可变右控制器至mesh挂点、原Root旋转、真实同等avatar／Bow scale、原units和左腕校准，转换为公共two-hand消费所需UE右Grip-local米坐标。未知/过小/不匹配缩放或不符合既有双握几何就关闭该副握，不扩大公共范围。新本枪接近／保持半径5.5cm／8cm、长度容差8cm、最大修正.6rad，机械输入仍不回读吸附/C0。

HandClient原support分支增加准确Bow类的独立delegate，其他枪仍走原函数。该delegate既用于Native候选也用于source-current检查，防止真实同源复核遗漏本枪挂点；只读已准备的POD和原绑定，不回入Engine／Native／HandClient。首次准备不在同一Native Frame打开新挂点，下一Frame才出现，避免同帧已提交game差异；原身份/时钟/挂点/输入检查不弱化。几何缓存另补准确component及asset身份钉住，不能同weapon/hold偷偷换组件继续使用旧校准。该接线没有改Native共享写者或其他枪模块，也没有把root其他并行WIP混入私有基底。

另关闭Bow肩取箭的pinch旁路：shoulder、producer及原ammo_grant明确拒绝index_pinch，只有本枪真实LGrip按既有Native弹药租约继续取箭／重抓。没有修改公共手部控制模式或其他枪。四组W4／WX／UNDEBUG检查0：原15节/完整SHA/V4结构及仅食指触发、实际发布调用器36、原Driver/Decoder/Feed/reload桥接272、显示/route/副握92。新增反例覆盖单位但错误MODEL reference、错误完整签名、腕挂点FK回原位置、scale错配、缺Native函数/非执行地址、release_required、旧press、弹药incumbent、pinch、忙碌/失追、副握两读一致和禁止同Frame打开挂点。Native/Engine／原库存／弹体后端仍注入，不能当实际V4消费或游戏动作证明。

已亲阅主握释放／按压、支撑、箭杆、入槽共5组各2角度原模型预览，包含每侧完整1891手tri及3146前臂/袖口/手tri。主握扳机区有穿插/遮挡，局部指尖可能被枪面遮成孤立块；箭杆／入槽仍保留R16的原作者手/箭穿插；支撑两个角度仍不足以核准全部掌指腕袖。**这是原作者最小姿势接线，不是四部位自然贴合通过。** 预览为原参考Bow网格和离线socket默认假设，不能替代原弓臂动画、真实双眼或头显。完整修正仍需看清接触面，不能因V4 TRUE或编译0提升结论。

完整私有Playground编译链接0；47份MSVC实际依赖匹配最终构成。artist-grasp-r17-final保存2116源、51份当前本枪源、DLL、四组检查、作者/provenance与5组原图及首败。manifest SHA256 c5f54c8c88205d64f8f57481bf122d166b4ab9bd0af8dea07c41a5c948aa3497；PG SHA256 54a99613d67d6eacd8f417845633f2eb027f9536e4360d8fdbcffcaf0d7f6af6。首个test误用length命名空间和caller fixture漏填完整挂点/默认release_required的失败保留；仅修fixture/API名称，未放宽产品许可。详细development-review-r17.json。

后续继续补可见原箭与掌姿共用的小范围沿槽辅助，raw全行程不得受修饰；完整自然贴合、无弹反馈、原调用正常返回与Native同世界清理、全生命周期仍开放。当前Native清理导出再次检查仍不存在，真实Charge/装填许可继续关闭。未stage、未整合新运行候选、未申请/占用新窗、未启动游戏/XR、未改公共v71或Git index/commit；整体25%和头显false保持。完成完整构成后才按原直控/脚本、唯一新窗及配对录像亲阅取得装1→Charge→真实UP→原箭1/扣1，再同构成R3取消和重装证据。目标active，完整扭力弓未完成。

## 24. 原箭与掌目标共用有限沿槽辅助（R18，2026-11-10，未运行）

本轮progress，R17不可变归档未改。新增Bow-only arrow_presentation数据求解，内部认证同一RawRig的真实双手和原不可变右挂点，复核当前原箭挂点、实际同等scale及固定槽完整几何。实际箭serial和模型config serial必须吻合；错误资产SHA、被改短行程、被改入口/箭尖、pinch、失追和过时输入均拒绝。只新增模型config serial只读getter，没有开放可变模型或改任何动作/库存条件。

显示与V4掌目标使用同一求解后的grip及原箭rigid mount。正常input消费依旧先把原RawRig双手及原sync/sequence交给完整Feed recognizer；只有raw插入成功才进原Driver/Bridge，之后立即撤箭。显示辅助在独立后段执行，既不回写RawRig，也不把修饰后的tip/grip送回机械判定。Result明确保留未改的原始source/tip/scene/sequence/sync，修饰的source/tip/grip分开保存；没有GT续时或新pose/action许可。

沿用原Bow草案的小偏差门限，箭尖获取/保持横向25/35mm、角度18/25度；只移除横向/角度误差，沿Root轴的原深度不改变，不能补造整段入槽。另按真实units限制对应掌目标最多40/50mm修正；长箭旋转即使仍在箭尖辅助角度内，只要导致过大拉掌就退回原始姿势。保持还要同一准确arrow/config、非倒序原input sequence/sync且间隔不超过原最大gap；新箭、撤显示或错误上下文清除保持状态。数值是本枪有限辅助作者数据，不是自然/舒适度接受，也没有放宽原机械6mm/8度、3m/s或100ms。

原显示组件show现在自己从实际认证Rig重复求解，并经已有GetAxes/LocalToWorld前后CPU矩阵ACK和原fresh/current检查；成功后才返回对应V4掌目标。后续left_part发布再次走原Native admission；缺导出或消费者拒绝仍不授予机制/库存权限。组件实际CPU矩阵确认不是硬件buffer/每个draw/实际蒙皮或双眼原子证明，实际C0/Blend过渡仍待实图；不能把“目标一致”写成手部像素已经贴合。

两组W4/WX/UNDEBUG真实通过：presentation120、原显示owner/support/route122。反例覆盖两种输入源、原时间戳不续租、100ms超期、进入/保持迟滞、角度边界、错误资产/入口/serial、同rig挂点反解、箭尖无轴向前进，以及正常30units/m长箭拉掌过大退回原姿势。特别加入4份各有新original sequence/sync/Scene的物理同源连续输入，从approach走到seat但始终横向偏15mm：画面目标可以对齐，原Feed每个样本仍rejected，不能装弹。实际show联查验证矩阵ACK后的显示source位置和返回掌目标来自同一解，raw output仍为原始source；错误箭/旧config拒绝并清输出与保持。Engine/Native/pool后端仍注入，不冒充游戏声音、V4实际消费或皮肤验收。

完整私有Playground生产目标编译链接0；41份MSVC实际项目依赖匹配。arrow-presentation-r18-final保存2119源、54份当前Bow源、DLL及两组验证；manifest SHA256 bf16028de54ed7675578b6b425da82277f2a3de35351979e710cef0df19aa8c8，PG SHA256 71385b8a10b18c6dcef9e3082a7c4a131ad92306a4c5a08ff99c3a57dabf1884。详见development-review-r18.json。沿R17只叠本枪源和CPP presentation include，HandClient、原Reloading服务、资源层和Native/C0保持本方未改；没有吸收root其他并行WIP或称其全树编译。

下一点无弹反馈已有具体原源码方向：保存的WarWeapon.Active.PlayClickClickEmptyClip只调用WeaponPlaySound(FireNoAmmoSound)，三个相关原脚本的路径/SHA已存no-ammo-original-cue-scout-20261110.json。这里只是已留原源码线索，未认证活函数/参数ABI或实际声音，更不能通过空射/开启原R2计时器制造提示；需要fresh左肩LGrip、零备用真实读回和一次intent一次提示，不生成箭或新动作。完整四部位自然掌指腕袖、过渡及双眼，原正常返回/自动Charge.EndState、Native同世界清理及全部生命周期仍开放。

本轮末再次查Native清理导出仍缺，真实动作继续failclosed。未stage、未整合运行候选、未申请/占用新窗、未启动游戏/XR或录像、未改公共v71及Git index/commit；25%和头显false保持。首个真实检查点仍须同完整构成装1→原Charge→真实R2UP→原弹体1且扣1，再同构成R3取消/重装/中断及亲阅配对实录，目标active，完整扭力弓未完成。

## 25. 新左肩取弹意图的一次原版无弹提示（R19，2026-11-10，未运行）

本轮progress；R18不可变源/制品保留。新增Bow独立feedback Intent和正常input消费，不启动原BeginFire、重复空射计时器、Reloading、箭预留或库存写入。只有同一准确当前Frame的真实已停原实例/模型都显示零备用、无其他手持对象、Native Decision为no_candidate且准确同press、当前和原press位置均在左肩、LGrip新物理边沿，才可尝试一声原版提示。输入先消耗再绑定/调用，未知结果不重放；库存后来变零、手后来移入肩部或ABI后来恢复，都不能使同一旧按下补提示。

Intent复用原控制器累计按下/释放计数、last_squeeze_press_grip/flags/sequence、原sync和完整Native/游戏/角色/hold/input/reference cohort。按住开机、失追后仍按住、重复包、倒序包、超过100ms、计数重置、多次模糊边沿、pinch、新hold/epoch和其他租约均不能生成新提示。确有一次release/press且原press pose序号证明属于新输入的合并包可以作为一笔新意图。失效只取消当前识别资格，不删除已消费输入上界。

加入反例时实际复现了新解析器初稿的倒退缺陷：旧released包被保存成基线后，同一已提示的press包可再触发。原FAIL日志r19-intent-replay-first-failure.log保留；修复后同cohort sequence/sync上界跨lost保留，旧包不能重置，恢复先取当前baseline而不发提示。另首次fixture遗漏真实press_token导致Frame shape拒绝的日志保留，只补fixture真实token，不改Frame合同。

原声音绑定选择准确WarWeapon类、其Active状态下PlayClickClickEmptyClip函数及当前Bow.FireNoAmmoSound。须原Core State/Function类型、无参数/locals/返回、无native index/native flag、已知原script exec，script表/字节双读、字段/身份/对象表/名字闭合及同selector重复确认；实际声音必须是Engine SoundCue。没有凭静态名字调用同名覆盖、没有硬编码步枪声音。调用前再次读取同Frame零备用/完整中立状态与相同函数/声音，调用后只读核对中立及库存未变；未知/过期不补调用。正常dispatcher、切走、失源、几何失效和equipment yield均接入原反馈失效路径；没有新线程、输入、Native/C0写者或hook。

生产Backend的last inventory observation现在标记完整原Frame，任何observe失败先撤known，避免拿上次计数发提示。Driver仅当前observed/baseline回合可响应，清理/Charge/装填/弹体责任仍排除；反馈没有把Driver的可变模型暴露出去。原发射、完美换弹、按住疲劳及反重放规则保持。

三组W4/WX/UNDEBUG通过：真实Intent96、真实元数据Reader/原cue绑定的owned-memory反例、真实feedback caller113。覆盖原函数错误owner/package/参数/返回/native/exec、声音缺失或类型错误、script读中变化、错误同函数快照、原invoke同步重入、未知返回后不重放、持握/失追恢复/旧包、其他租约、outside原press、库存非零、busy、pending及Charge。原声音/Native/库存后端仍注入；没有实际调用游戏声音或认证可听性。首个全PG编译暴露Request/invoke实际在外层namespace及observed遮蔽全局，已窄修并保存首败源/manifest/log；没有放宽编译器或调用许可。

静态原默认对象补核只读3个准确export范围并核SHA。原WarWeapon父CDO完整解析，FireNoAmmoSound指向原Weapon_AssaultRifle.Firing.CogRifleFireEmptyCue；两层Bow CDO被现有bounded decoder拒绝，保留decode_complete=false及首败，未把残缺值当完整覆盖/继承证明，也没有为此修改公共parser或硬编码父声音。实际运行仍必须读取当前Bow的真实属性。证据original-no-ammo-defaults-r19.json及源码指纹记录；这不替代活ABI/声音验收。

完整私有Playground生产编译链接0，41份MSVC实际依赖匹配。no-ammo-r19-final保存2125源、60份当前本枪源、DLL、3组检查、首败及原默认/函数证据；manifest SHA256 2770d8ec6e9cb22c540868ab04cb90c1c0cde672725f682af2ee0bf369698c99，PG SHA256 6e694ab5fa493ba448f8809242a94feba5997a82c33e7359f318550671572f5e。详见development-review-r19.json。沿R18仅本枪源与CPP自有include/声明；HandClient和其他并行WIP未混入私有构成，不能声称root全树或新Native已编译。

已核对既有sole Native维护者lancer仍在当前checkout活动，将完整已存在的GearsCaptureOriginalInputCleanupContextV1合同再次直接交其核对/实现或指出等价现有接口。这是必要共享Native接口衔接，不是统筹汇报/新helper/新会话/运行窗申请，也未打断其拥有的运行；本方继续不改共享Native。冻结时源码导出标识仍缺，Runtime ABI整合也未验证；所有原动作保持failclosed。仅演员指针、旧Frame或100ms保护削弱均不能代替同world_generation/incarnation证明。

原装填、Charge/真实UP、R3取消、原箭行程、显示/姿势及无弹提示现在都有源码路径，但首次完整真实闭环、原返回/自动Charge.EndState、自然掌指腕袖/过渡/双眼、全中断/守恒/疲劳及实体交付仍未通过。下一真实检查仍为同一完整构成装1→Charge→真实UP→原箭1且扣1，再同构成R3/重装/无弹/中断及亲阅配对录像，不能回退到无动作持弓/ABI切片。未stage/新候选整合/取得运行窗/游戏XR或录像，未改公共v71/Git index/commit；25%和头显false保持，目标active。

## 26. 黄色引导、原蓄力只读进度与真实结果标记（R20，2026-11-10，未运行）

本轮progress；R19不可变构成保留。新增Bow独立guide Selector/geometry、原进度字段reader及正常GT末尾消费者。机械动作、输入解码、箭显示与手势先完成，黄色引导最后可选执行。仍通过原Actor.DrawDebugLine绘制单帧黄线，bPersistentLines=false，不新增renderer/hook/组件/骨骼/输入写者，不改接触体积或任何装填/发射许可。整批包含准备最多8ms，过期或预算耗尽即丢剩余线；单个原调用本身不能抢占，尚未实测其运行时开销。

肩取显示稳定身体左肩圈加十字，无备用显示同肩圈加叉；固定槽圈定位原Muzzle入口，箭头沿原尾先入槽的Root-X方向。入口圈使用原组件真实scale，肩标和提示符用同RawRig的真实units。Root仍由同一原始右控制器及既有不可变WeaponMeshFollow挂点恢复；肩位经当前认证RawRig的base/reference转换，不回读C0/Snap或显示箭，不把最终箭头位置误当枪口。初版发射提示沿枪管轴，源码审查发现正对弓身时会缩成点；最终已改为与勾/叉同Y-Z平面的独立箭头符号，未改真实发射方向。

模型只新增被动结果序号：last_loaded_token仅实际accepted装填/库存回执闭合后记录；last_confirmed_cancel_edge仅fresh R3请求经过实际完整中立abort ACK且无fault/lost/shot时记录；原发射读取原有last_projectile_序号。未派发Charge前取消、失追清理、PendingFire仍在、拒绝/未知调用均不能显示取消成功。没有仅因Charge消失就冒充取消，也没有因松R2/进入Fire就冒充已生成箭。原始游戏弹体回执始终优先于同时清理结果。

独立Selector把完整Frame cohort、hold/config、原输入/帧/时间上界和三类真实回执作为只读输入。启动、新cohort、失源恢复或超过100ms间隙先取基线，旧/倒序Frame或倒退回执不能发新成功提示。原装填ACK后显示勾，完整R3非发射ACK后显示叉，原弹体记账后显示箭头；结果最多保留700ms，仅属UI，不延长任何操作或原输入期限。新装填/Charge阶段按当前真实状态显示，不以提示状态授动作权。

Charge环形刻度只读当前Bow.ChargeTimeCount/MaxChargeTime；严格FloatProperty宽度/owner Weap_Bow_base/WarfareGame、当前对象/表/字段闭合、值双读和有限范围检查。不使用QPC重构原蓄力，不写count/max，不改原奖励增长倍率、Pawn等待或HoldTimeUntilTired疲劳释放；超过100%的原计数只在显示端限为满圈，原值完整保留。进度字段未知时不画假进度。

正常消费者要求当前准确Frame/原库存snapshot、模型库存一致、主握/无busy/无动画抢手、真实左右手源、同hold几何及Native当前性。DrawDebugLine严格Actor/Engine原函数、原可执行地址/native flags/无返回、六个准确输入、Vector结构/宽度/不重叠、无额外参数或循环链。原函数身份和registration/原输入期限逐线核对，整批两端完整current。批次在首调前消耗同Frame标记，原invoke未知返回或同步重入不重放；失源/切走/几何失效/yield撤Selector资格，不另发Engine teardown。

五组W4/WX/UNDEBUG实际通过：Selector/geometry667检查、真实typed字段reader的owned-memory检查、真实guide caller201检查、原动作模型及原Driver回归。caller使用真实Selector、RawRig映射/Root和新参数枚举/批次循环；原反射基础helper、Native当前性及原DrawDebugLine为注入，不作为实际游戏ABI/像素证明。覆盖同帧同步重入、未知返回、8ms中止、失源、函数代次变化、旧scene、busy、额外/重叠/返回/循环参数，以及启动/恢复不伪报成功。模型检查额外确认装填ACK前无锁定、R3 ACK前无成功、失追或未派发取消不报成功、真实弹体序号保留。首个caller命令编译成功但cmd不识别正斜杠EXE路径，原工具失败日志保留；规范反斜杠strict脚本最终0，不记成产品逻辑失败。冻结审计首版因MSVC tlog大写Windows路径KeyError，保留日志后改按Windows大小写不敏感身份映射，SHA/时间验证未弱化。

实际C++几何另导出九格图r20-guide-overview.png并亲阅：肩取、尾先入口、锁定、25/50/100%原Charge、取消、发射、无备用均可区分。各格为独立比例的离线示意，不是原游戏/HMD/双眼可见性、遮挡或自然贴合证据。游戏中线宽、清晰度、图样位置、动弓臂遮挡、8ms预算实际余量及全部过渡仍须同构成实录检查。

完整私有Playground最终编译链接0；350份MSVC实际项目读取依赖及48份strict依赖核对源码SHA/时间。guide-r20-final保存2131源、66份当前Bow源、DLL、五组检查、原失败和九格示意；manifest SHA256 b500eb18543d49dc001433ca3b89a888053550da4892c757f94a6471d43968ba，PG SHA256 f03c608270cf23dda04f5a007b6bcd96ab9f201cb4694dc966bc601a0db24cd7。详见development-review-r20.json。仅沿R19叠本枪源与CPP自有header/forward/includes；HandClient/共享Native和其他作者后来WIP未混入，不称root全树或Native已编译。

现有sole Native维护者lancer已明确：本轮未实现GearsCaptureOriginalInputCleanupContextV1、无已核实等价接口，其当前范围只认领人类优先的Lancer自然手型/C0，不承接此查询；并要求继续failclosed，缓存指针/普通Frame不能代替incarnation。依common.md第7节“共享 Native／骨架／输入／Call subscriber 保持单一写入责任”，本会话已向人类请求仅实现该通用只读接口及必要验证的范围授权，目前尚未收到答案。本轮因此不越界编辑Native，不把peer不承接解读为写入授权；等待期间已完成上述独立Bow工作。没有自动审批拒绝或新helper/管理会话。

真实装1→原Charge→真实R2UP→原弹体1/扣1及同构成R3/重装/无弹/疲劳/中断，原正常返回/自动Charge.EndState，四处自然掌指腕袖/过渡/双眼、全部守恒/生命周期和交付仍未验收。无新Native/PG/XR运行整合、stage/运行窗/游戏/XR/录像，无公共v71或Git index/commit变更；25%及headsetfalse保持，目标active，完整扭力弓未完成。

## 27. 原作者主握与现有Native手腕的固定对应（R21，2026-11-10，未运行）

本轮progress，R20不可变构成保留。处理自然手型前先核主握的坐标对应：旧Bow走通用held_socket_basis + WeaponMeshFollow.bind_oriented。这个方法会保留第一次bind测得的控制器至组件位置偏移；R17主握15关节却是固定原AR_Reload key25，不能把这两种不同来源的配准直接当已一致。新owned反例固定右控制器与mesh_in_hand，只把拾取矩阵平移3 UE units，旧方法解出的持握位置仍差3，预期固定原接触的断言实际失败，日志r21-legacy-failure.log。它证明源码的首次偏移依赖，不是31716或其他已录游戏缺陷的因果证明。

从已固定SHA的原AR_Reload key25、原Marcus115 raw reference和Bow原RotOrigin重算Hand_R相对于Weapon_R_End，再转至原始Bow mesh坐标；不是试角度、重采拾取动画或缩放骨长。新weapon_torque_bow_main_grip.h固定原腕位置(-2.323001146315469,4.921195001899023e-7,-2.8218982919270275e-6)及完整腕basis。原动画decoded JSON仍52b2c9d5ef289a3107aee21a167ceb7d0e11dd007d88bb5613a31008a0ddc6e6；原export仍1db9c32bde2c111961fe1ddf7150b37614753cea3f950257ef3fdb5e6da14812。独立authoring脚本重算矩阵与头中常量最大差0，记录main-contact-authoring-r21.json。这是选定原作者握姿，不是当前Scene socket/default或自然表面证书。

新纯几何binder从当前Native已使用的held-wrist calibration反解同一个掌心marker，再把原Bow wrist_in_mesh与该marker组合，经现有WeaponMeshFollow.bind_mount建立固定关系。拾取矩阵只验证实际组件正向等比scale，不再提供位置/姿态偏移种子。没有抄Lancer私有接触点、改Native要求的35度现有手腕校准、改控制器输入或增加手部目标/骨骼/renderer写者；Native后续完整几何/对象/Broker/endpoint检查仍保留。新挂点与原手指表共享同一原作者腕框架，仍不是物理控制器校准或完整掌指贴合接受。

Bow独立PG wrapper在现有GT主组件binder调用，重复当前Native Frame/实例/mesh身份，只读同一avatar组件LocalToWorld并双读闭合；严格正向rigid/等比scale，必须与真实Bow scale匹配，拒绝非等比、镜像、剪切、非有限、读取中变化和过期源。不依赖尚未产生的committed手部输出来建立主挂点，不引入“等主挂点才能出手、等手才能建主挂点”的循环。数据先在局部candidate完成并收尾核对，成功才替换现有mount；失败不回落到旧拾取偏移。暖箭/库存/Charge许可仍由原完整Backend及缺失Native世界证明共同把关，没有通过本显示挂点绕开动作前置。

共享weapon_native_lancer_follow.inl只增bound_is_torque_bow的两行分支；root中现有Gnasher及所有其他分支原样保留。私有构成以R20为基底，仅把同一Bow分支插到原通用fallback前，不把root后来Gnasher源码整体吸收。CPP只增本枪头、forward和独立实现include。原15手指表、箭挂点/机械行程、原蓄力/取消/发射、时效/身份/租约/反重放及公共v71均未改；无共享Native源编辑或生产构建。

四组W4/WX/UNDEBUG实际通过：主握数学675检查、实际PG wrapper22检查、四部位V4/reference与index-only trigger回归、原Driver回归。数学覆盖3scale×3units×3控制器姿态，任意拾取位置不改解出的固定挂点；解出原腕world位置与现有Native wrist_target误差小于1e-5 UE、basis误差小于1e-8，原骨长/局部translation/scale未动。wrapper覆盖旧/错实例、失源、错误或变化scale、身份失败和重复bind，并以真实RawRig/mount/root_sample接原箭/固定槽authoring成功。Native/内存/Engine仍为注入，未运行真实C0/GPU，不把这些数值当自然手型验收。

保留两次legacy fixture首编译失败：宏分支的right局部遮蔽及return后不可达代码触发WX，只修fixture预处理作用域后才取得实际3 units行为失败；未降警告。私有源拼接初稿误选root后来Gnasher上下文，R20中无该上下文而主动断言拒绝；改为在R20原通用fallback投影同一Bow hunk，没有覆盖或抄入peer WIP。没有用这类构建工具错误解释游戏行为。

完整私有PG编译链接0；352份MSVC实际项目读取依赖和41份strict依赖核对源码SHA/时间。main-contact-r21-final保存2135源、70份本枪当前源、DLL、四组检查、旧行为首败及原作者矩阵证明；manifest SHA256 7b5a5b2982a036c90202e34b1ee476a95b5fba6ed1b0b3009e4afdeb21376c3f，PG SHA256 4e51a2a094528b518d8838212a41c59d9e0b88e69568b3e8a723288bd3261fb0。详见development-review-r21.json。root全树/当前Native未编译，R20与旧候选均未替换，不把私有DLL称作已整合交付包。

R17原artist主握/shaft/feed已知表面穿插仍未解决；本轮没有重跑不变palette预览、没有称固定腕框架消除了穿模。下一自然贴合必须基于这个明确主挂点及同一完整掌/拇指/四指/腕/袖口，而不是继续围绕首次拾取偏移补手指。Native世界实例查询源码仍缺，人类针对通用只读接口的范围授权仍待答复；遵守common.md第7节单写者边界，不把持续目标消息当新增Native授权。真实首个完整装1/Charge/UP/弹体1扣1、同构成R3/重装/无弹/疲劳/中断及亲阅实录、双眼/头显/生命周期和实体交付仍开放。无stage/新运行整合/运行窗/游戏/XR/录像，无Git index/commit变更；25%及headsetfalse保持，目标active。

## 28. 同一C0完整混合蒙皮证据与指节单独修复的边界（R22，2026-11-10，证据进展）

本轮progress来自新的完整表面证据，生产源码/DLL仍为R21，没有把未接受草稿接产品。先核root body_c0_hands.h与现有维护者已冻结V14完全同SHA 7f908bda91608dff59c0d21daa2cdae3ac001a3b721fee6ec739a2d3dc791d26，再在本枪ignored analysis复制其纯owned-buffer头文件和必要公共头，叠R21本枪几何/手指表供离线C++调用。只复用body_c0_hands.h明确声明的“Owned-buffer composition only”纯数学，不编辑共享Native、不编译/加载Native DLL、不开hook/进程/GT/input或源准入，没有替代维护者责任。当前root在本轮冻结时仍与该V14相同。

新c0_contact_r22.cpp读取既存original reference/C0-local/component fixture，prepare_model实核Marcus115全32字节签名3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193。明确GEARS_C0_CONTEXT_GEOMETRY_ONLY，只生成合成可达world grip/腕目标，不把旧Core字节改标签或冒充真实tracking/权限。保持R21原main wrist、原shaft/feed挂点和R17四套15局部表，调用同一个生产C0 compose_common/唯一手臂与手指算法，再独立FK输出全部115world joints。副握按真实relative双手分支，shaft/feed按原world loading分支、同authored preset，均不是另写皮肤或替代C0。

主握rest/trigger、副握、持箭、入槽五个steady姿势均status0；原组合627检查0失败，全部115局部translation/scale保持。合成入槽目标原本超固定臂长，整组弓/左右目标共同后移11.7045raw才在reach内，没有伸骨或改保护；这是明确制作姿势条件，不是真实动作已到达或实际用户手臂受限证明。最初author harness把source.base_view成员直接作为另一个引用参数传入，原alias保护拒绝status1；首败source/log/partial JSON保留，仅改为独立value副本后通过，没有动C0 alias guard。

按原LOD0全部字节weights和完整骨映射蒙皮。每侧1891个含任意手权重的三角，以及3146个手/袖口/前臂混合三角全部进入检查，不再把1395纯手面或原动画前臂当实际C0整手。Bow使用9742个原reference三角，箭使用216个原三角；箭实际按产品固定key25 source_in_wrist从合成physical wrist恢复，不能借原AR_Weapon_L_End动态矩阵替代当前产品挂点。早版按原动画箭socket导出的数值另保留为first，最终统计与预览使用产品固定挂点。

| 原R17表，经同一当前C0纯函数 | 弓体交叉对 | 原箭交叉对 | 手自身非相邻交叉对 | 不同手指交叉对 |
| --- | ---: | ---: | ---: | ---: |
| 主握rest | 348 | 0 | 149 | 20 |
| 主握trigger | 372 | 0 | 160 | 20 |
| 左副握 | 629 | 0 | 114 | 0 |
| 左持箭shaft | 0 | 206 | 160 | 17 |
| 左入槽feed | 97 | 140 | 113 | 2 |

这些数值是非共面三角交线跨度大于0.1raw的对数，不是signed depth、接触质量评分或GPU/全部连续角度证明。UV/normal接缝邻接由原bind位置+精确skin influences排除，不因当前姿态顶点重合忽略新交叉；原bind折叠数单独保留。不同手指按不含掌/腕的原dominant骨组划分，不能以总数下降判自然。五张各六个腕轴方向的完整混合面预览均亲阅；视图聚焦腕/掌/指/袖，超出画布的上臂裁切，数值检查仍覆盖全部3146三角。Bow仍为reference/root表面，不覆盖Charge中原多骨变形、原材质/遮罩或实际双眼像素。

新r22-fixed-skin-witnesses.json逐个追到原始byte权重：主握rest/trigger各36对、副握249对、入槽97对弓体交叉，其手面三角每个顶点在全部对应finger15子树上的权重严格为零。C0指节pass只在手臂解算后更改那15局部rotation，所以在当前固定腕/arm/object frame下，这些见证对不可能仅靠改15个手指Quaternion消失。记录保留原顶点、每个skin influence、三角和世界交点；它不推出唯一平移/旋转或游戏因果。下一步必须连同整处掌/腕/袖注册或部件关系一起修，不能用更深卷指遮掩。

对不含这类固定腕见证的shaft仅做了一个有界finger-only草稿：从正确原raw reference绝对生成屈指/拇指，使用当前C0完整skin及固定原箭，保留MCP/PIP/DIP/拇指硬边界，不叠旧种子、找旧轴/试角度、移动腕/弓/箭或改变骨长。约束代理取初始真实三角交线的单侧平面，参考手面决定侧向；它不是封闭solid SDF或已认证解剖pad。69迭代/901次函数求值，预测箭交叉206→125、不同手指17→4，但self160→171，仍明确拒绝。

该唯一草稿随后在同一个C0可执行纯数学程序回放。只改shaft的15行局部Quaternion，另外四个case、shaft全部非finger world joint、腕/arm/弓/箭world transform逐值相同；627检查0失败。完整混合skin预测对C0回放最大差3.6928905444710836e-7raw，最终exact surface仍125箭交叉/4不同手指，六向图仍有穿插及过度卷指，亲阅后未接产品。r22-shaft-draft-replay-review.json保存拒绝，不把优化数少或C0执行0当完整自然握持。

证据快照c0-contact-r22-final包含490份头/fixture/输入文件、35份实际本地编译依赖、43份检查/skin/图/首败/草稿制品；manifest SHA256 81be34e38aa549f9f761964c5cced5d22d4ba039c24ed002e3b4eb161fa8e4cd。详见development-review-r22.json。原C++实际W4/WX/UNDEBUG/O2编译执行0；只编本枪owned-buffer harness，未新增Native或Playground生产构建，当前生产PG仍R21的4e51a2a094528b518d8838212a41c59d9e0b88e69568b3e8a723288bd3261fb0，70份当前本枪生产源与R21 SHA全部不变。

完整自然四处仍未接受。下一点按这些同C0见证先修整处注册，再同时约束手指/原物件/指间及腕袖，不重跑不变palette或因数量降低发布草稿。Native世界证明源码仍缺，原通用只读查询的范围授权仍待人类答复，严格保留单写者/failclosed。真实完整装1→Charge→真实UP→原弹体1/扣1、同构成R3/重装/无弹/疲劳/中断、原正常返回/自动EndState、配对实录亲阅、双眼/头显和交付继续开放。无stage/运行整合/新运行窗/游戏/XR/录像/助手/CUA，无公共v71/Git index/commit变更；25%和headsetfalse保持，目标active。

## 29. 主握整处注册、两食指端点与原表面拓扑（R23，2026-11-10，两个草稿均拒绝）

本轮progress是整处主握制作和新的拒绝/拓扑证据，生产仍为R21。沿R22已核同一冻结V14纯数学C0、完整3146手/腕/袖/臂面及1891手面，固定合成physical wrist/arm、现有Native手腕校准、全部骨长/局部T/S，只联合制作原弓体相对腕的刚性注册和原参考绝对15关节。松开/扣下两个食指端点同时进入约束，其他12个Quaternion逐值相同；没有改Native、输入、动作许可或增加writer，没有枚举旧错误坐标轴的角度。

先按已验证skeletal GPU原position12+packed tangent/normal8格式读原Bow/手法线；原9742面与原顶点法线方向全部为负，反转面序后法线吻合度中位数0.996377408892292。原C0 mixed skin的独立finger kernel对R22完整skin最大差1.3642420526593924e-12raw。掌面及指腹区域来自原byte权重、原reference骨局部位置/法线，仍只是待视觉核对的制作条件，不冒充测得解剖pad。目标明确区分握把近侧掌面/拇指、远侧中/无名/小指及屈曲食指区域，所有目标面ID保存在r23-main-art-regions.json；最终交线检查仍使用全部原弓体面，没有删除不方便的面或放宽接触体积。

第一份33变量有界联合制作同时约束完整skin距离、上述接触角色、原参考指列分离及明示关节/注册边界。916次求值/31迭代、约238.657秒，原弓体交线从R22主握348/372降为89/131，但不能作为自然通过。第二份只针对该首败加220对实际三角交线平面见证、三指中段的前缘接触、明确松开食指范围；仅制作侧向注册边界由-3扩为-4raw，没有改Native/追踪/固定臂长或制作姿势修正上限。704次求值/23迭代、约227.469秒后118/137，整体更差，明确拒绝，不继续扩大旋转去追数字。

| 同一C0数值回放后的主握 | 弓体交线对 | 不受finger15影响的弓体交线对 | 手自身交线对 | 不同手指交线对 |
| --- | ---: | ---: | ---: | ---: |
| 第一份，松开 | 89 | 1 | 101 | 3 |
| 第一份，扣下 | 131 | 1 | 154 | 50 |
| 第二份，松开 | 118 | 0 | 101 | 7 |
| 第二份，扣下 | 137 | 0 | 150 | 45 |

因此第二份确实清掉R22那类单靠屈指无法改变的主握固定面交线，但并未修好完整主握；闭合食指尤其仍与其他手指互穿。两份各两个端点的六向腕部聚焦完整混合面预览均亲阅，仍有手指/握把/拇指外观问题，全部拒绝。连续release→trigger及全部真实姿态/双眼尚无验收。

两份均回同一个W4/WX/UNDEBUG/O2编译的owned-buffer C0程序。完整skin预测与数值回放最大差分别2.946044332929887e-7和2.2253902898228262e-7raw，原弓体顶点预测差约5.41e-5/5.49e-5raw；主握身体及全部非finger world joint逐值不变。这个对应只证明制作kernel没有用不同手臂偷换同一C0，不能转成game/GPU或跟踪准入证明。

整套五case程序两次都实际退出1：新主挂点让当前合成入槽制作姿势需要14.2063/17.6602raw整组修正，超过保留的14raw制作范围。C0数值status0不覆盖这项FAIL；没有抬高上限或把程序当全通过。原始输出在后续case失败时未闭合JSON，按原样保存；独立decoder只读取前面已完整写出的四条记录，验证其中两个main端点，不补尾/补case或把partial改标签成全套通过。两次真实工具exit1及其exe/profile/output/log SHA各有execution receipt，whole-case通过必须另有完整输出和exit0，不能仅凭文件能解析或状态行判断。

原表面拓扑补核指出现有优化代理的边界：仅按完全相同的原位置焊缝后，9488顶点→5167，9742面无任何重复几何三角，面序一致但整体不watertight；66个连接部件中35个闭合。掌/三指/拇指目标所在的1789面组件27本身开放，屈曲食指目标还跨两个开放部件。r23-main-original-topology.json保存原面/组件/角色映射；没有圆整焊接、删面、构造新碰撞器或改原资产。最近面法线距离因此只能作局部制作代理，不能当封闭金属inside/SDF证明；更低目标函数值不能保证真实交线减少，第二份实际给出反例。

保留工具首败：准备/细化脚本漏加已安装SciPy依赖路径；初次准备未成功后又尝试编译不存在的回放源，其missing-source日志保留。补显式依赖路径后再生成/编译。交线报告首稿把局部三角顶点序号用于全模型骨标签，原首稿另存；只用原ID映射修正bones元数据，顶点/配对/计数/优化结果不改，并由独立C0回放重新核数。没有用这些制作工具问题解释真实游戏缺陷或改产品保护。

证据快照main-authoring-r23-final引用R22不可变490输入/头文件，封存35份实际本地编译依赖和41份新脚本/数值/图/首败/回放/拒绝制品；manifest SHA256 5f971758173af71d1399ace51df0aa8add1c725ea38ab1e89fde132196559000。详见development-review-r23.json。R21的70份本枪生产源SHA仍全部一致，生产PG仍4e51a2a094528b518d8838212a41c59d9e0b88e69568b3e8a723288bd3261fb0，无新Native/PG生产构建或草稿发布。

下一点须把开放/闭合原部件的占据含义、真实三角交线、两个食指端点的指间关系、中段包握和原入槽可达范围一起约束；不能继续不变的最近法线代理、只优化main后忽略装箭姿势，或用较低计数代替自然接受。完整四处/过渡/双眼和头显仍未通过。Native同世界证明源码仍缺，人类针对通用只读接口的范围授权仍待答复；保留common.md单写者/failclosed及首个完整原装1/Charge/真实UP/原弹体1扣1，再同构成R3/重装/无弹/疲劳/中断和亲阅实录要求。无stage/运行窗/游戏/XR/录像/助手/CUA，无公共v71/Git index/commit变更；25%和headsetfalse保持，目标active。

## 30. 有限三角分离与入槽范围的联合反例（R24，2026-11-10，草稿拒绝）

本轮progress为新的制作约束、原C0对应和明确拒绝证据，生产源/PG继续冻结R21。原Bow握把组件开放，因此移除全局最近法线的inside/SDF代理，改用实际交线对的有限三角投影间隔。任意轴上两个三角的投影正间隔是该对分离的充分条件；候选轴集合不冒充完整碰撞分类器，最终仍独立检查全部3146手臂/袖/腕/手面与9742原Bow面及同手非相邻面。掌/拇指/三指/屈曲食指仍有原有限面接触和局部法线朝向条件，不能借整体浮空通过；接触角色仍为待视觉确认的authoring假设。

先从同一冻结V14纯数学身体规划器导出115世界骨和原合成右腕目标，W4/WX/UNDEBUG/O2实际编译运行10检查0失败，35份实际本地编译依赖核SHA/时间。独立入槽预测精确复现R22基准11.7044553856、R23第一份14.2062844203、第二份17.6601706306raw，均与原C0日志的六位值吻合。原14raw制作修正上限、固定physical腕/臂/Native校准和全部局部T/S不改。有限三角交叉→沿所选轴分离0.125raw及刚体协变反例也通过；这不是原游戏、解剖或动作许可证明。

第一份沿R23第一草稿原硬边界，松开/扣下同时约束实际Bow与不同手指交线，加入新实际交线后仅再作一轮。约22.969秒、990次求值的两轮结果仍不接受（首轮达到25次函数评估上限，次轮以xtol停止，不作自然收敛声明）：原C0复核松开54对Bow/95对self/1对不同手指，扣下91/150/50；两个端点均无完全不受finger15影响的Bow交线。包含dominant腕骨的混合面不能仅凭bones标签当成finger15不可改变。两张各六向完整混合面预览均亲阅，握把、手指和拇指仍有外观/穿插问题；上臂超出聚焦视口部分仍包含在完整数值检查中。

软入槽惩罚最终14.0362819908raw，超过原14上限，也超过制作目标13.95，因此明确失败。随后仅针对这项失败，用显式不等式同时约束有限面间隔和13.95制作余量；326个约束的SLSQP两迭代即报告Inequality constraints incompatible，未产生可接受新姿势，工具进程正常结束不等于求解成功。保留该局部固定轴方案首阶诊断：在平移±0.25raw、角度±2度和原硬边界内，最小共同违反量约0.30071793；保存具体冲突三角ID和方向。这只拒绝当前局部分离方向/线性化，不能推成全局模型无解或天然手型不可行，也没有据此扩大旋转/骨长/可达范围。

复用已冻结SHA的R23 owned-buffer C0回放exe，未重编不变Native/PG产品。预测与真实同一C0纯数学回放skin最大差4.385237843962386e-7raw、Bow顶点最大差5.417903024707016e-5raw，所有非finger世界骨逐值不变。五case程序实际exit1，原feed记录correction=14.0363及原14范围FAIL。原partial输出不补尾，独立解码仅核前四个已完整记录中的main两端点；不能把C0 status0、四条记录或工具解析成功当作五case通过。执行receipt保留实际exit和exe/profile/output/log SHA。

finite-contact-r24-final引用R22/R23不可变输入，封存35份新C++实际本地依赖及33份新脚本/日志/数值/图/失败制品；manifest SHA256 0e30246529b2c71f1371e5aed401cc7d73933772faefd35ece2e90fbfaed7b54。详见development-review-r24.json。R21的70份本枪生产源仍全部同SHA，PG仍4e51a2a094528b518d8838212a41c59d9e0b88e69568b3e8a723288bd3261fb0。全部新草稿拒绝、未接产品，无Native源修改/产品构建、新PG产品构建或实际运行。

下一点从保留的冲突三角及原握把接触角色选择协调的有限分离方向，同时显式保留入槽可行域和两个食指端点的指间关系；不重复当前固定轴/软惩罚循环，也不以较低计数代替自然握持。Native世界实例查询仍不存在，人类范围授权仍待答复，遵守common.md第7节单写者边界和failclosed。首次真实完整装1→原Charge→真实UP→原弹体1且扣1、同构成R3/重装/无弹/疲劳/中断、配对实录亲阅、四处自然贴合/过渡/双眼/头显与交付仍开放。无stage/新运行窗/游戏/XR/录像/助手/CUA，无公共v71/Git index/commit变更；25%及headsetfalse保持，目标active。

## 31. R25手型草稿仍拒绝，停止拟合转解真实动作前置（2026-11-10）

R24固定分离轴会随手指弯曲而变成局部不相容约束。R25先改为每次重算当前有限三角投影轴，145个真实交线对的批量/逐对计算最大差3.55e-15，刚体协变和配对对称性通过；入槽13.95余量改为独立显式约束。两轮草稿无不同手指交线却增至254/350对Bow交线，明确拒绝。这进一步暴露只约束历史已记录面会漏掉移动后邻面的缺口。

新增每次候选求值对原3146手/臂/腕/袖面、9742弓面和1891手面重新查询邻近有限三角。当前254/350对真实交线全被邻域覆盖；微小数值求导仅用扩展邻域加速，逐次检查坐标移动界，四类扰动与完整代价逐值一致。一次35迭代有界修正达到上限，未称收敛；最终原C0数值五姿势628检查0失败、feed修正10.6731raw，但主握松开/扣下仍147/164对Bow交线，self93/92、不同手指1/0，另有3对完全不受finger15影响的固定面交线。skin预测最大差3.18e-7raw、Bow5.51e-5raw，所有非finger世界骨逐值不变。

两份草稿各两个端点的六向页均亲阅，均拒绝。指腹至有限目标面平均距离：中指1.1945、无名指2.0609、小指2.0201raw；拇指所选区域平均法线朝向0.1741，不符合当前相向接触条件。源pad/接触角色仍只是authoring假设，不能以零指间交线或五姿势数值通过称自然握持。完整输入和失败图保留在r25-*文件，r25-contact-role-diagnostic.json记录原顶点/面；无生产手型表修改或推广。

本轮统筹转达人类纠偏：完整扭力弓授权已包含必要普通源码/只读接口，不应继续以额外人类批准为前置。此前等待额外授权的处理至此被明确替代；本会话已停止继续拟合，先解决下述接口和完整动作构成。development-review-r27.json合记此转向，不为每个拒绝草稿再造整套生产封包。

## 32. 通用只读清理上下文已实现并生产编译（R26/R27，未游戏验证）

责任已落实：Lancer确认其仅编辑body_c0_hands.h左support和hand_shoulder_pose.h，不承接本查询/include/export；统筹随后明确本轮由Bow唯一认领capture.cpp三处接入及新增独立模块。已应用global policy include、runtime后live include和GearsCaptureOriginalInputCleanupContextV1导出，未改现有Broker/Scene入口或Lancer两文件。原单写者要求保持，不再等待额外人类确认。

original_input_cleanup_context_policy.h/live.inl提供最小读合同：safe-copy准确Request并校验size/version/reserved及原EquipmentIdentity；用真实已认证Native session/注册cookie/GT、现有World EpochGate Scope、committed world_generation和完整对象/当前possess/mesh/weapon/Instigator重复读取闭合。进入现有Scope在任何runtime/metadata读取之前，同时沿用Native active导出计数和世界租约。拒绝未知、drain/terminal、注册替换、callback/transaction/aux/action/external等危险继承上下文，closing再次比对runtime revision和原request字节。无Engine调用、输入/骨骼/追踪writer、回调、新动作资格或时钟续期。

历史Frame/lifecycle/reference仅是原动作来源；旧pose自然过期不伪造换世界，也不把它更新为当前Frame。现场Broker revision只在本次查询两端独立闭合，不与旧pose撤销revision相等来阻挡既有动作非发射清理。新动作、租约、原journal serial与严格当前输入检查仍由已有PG调用方负责；读失败只拒绝，不销毁或退款既有责任。

实际新adapter配现有typed原身份/隶属reader的owned-memory检查359/0，包含recording=false的既有来源、危险ledger各类、错误/worker线程、注册/排空、真实对象/隶属/Instigator变化、同指针world_generation变化、读取中revision/world变化、request变化及不可读；现有EpochGate/RecursiveAdmission用于fixture世界租约，Native runtime注册状态为注入。disabled功能独立编译执行0。保留首个MinHook路径错误及两个fixture错误状态预期，后者只改为真实返回unavailable，未改产品拒绝策略。这些是源码检查，不是已装载Native或原游戏调用证明。

隔离生产构成以已冻结V14为基底，保留其C0/肩部精确字节/原flags；补入此前已记录并编译的Bow R3 veto模块及其consume_buttons依赖，再叠本查询。首次缺veto锚点、缺Route依赖的构成失败均保留。未拷当前root中后来Troika mounted-look等peer WIP。配套审计又拦下Native旧weapon_support_grip.h与当前PG含support_box的版本不一致；未stage该中间DLL。最终只在隔离构成对齐PG已编译的三份公共头，强制clean-first全目标重编，防止copy2旧时间戳令增量对象误复用；root公共头和C0未改。

最终Native生产编译链接0，SHA256 413b68c974a576b11a1a4da3d34ad6648b5ad842945d51a8ffabaac9533968e1。397份MSVC实际本地读取依赖核SHA/时间，原V14 compile definitions逐值保持。72个导出保留V14及原Bow veto各69个导出的并集，仅新增本查询；旧veto与LS input alias相同RVA仍保持。详见cleanup-native-build-r26.json及cleanup-native-verification-r26.json。尚未装载/游戏验证，不能把导出存在或host检查当原Charge可用。

## 33. 完整动作证据与034115联合候选（R27，已准备未运行）

只在Bow正常GT backend增加有界被动证据：已有原弹体返回和恰好一箭库存变化被模型接受后，记录准确原object/class、call/shot/input serial和前后弹量；原队列退役重试不能重复该行，callback内仍无I/O。另最多64行记录driver状态及实际loaded/cancel/projectile回执变化，不新增成功标志、Engine调用或动作权限。除此之外沿R21保存构成不改机械逻辑、原手型/挂点或其他枪。PG完整编译链接0，SHA256 8c15996bd238114fa63c8a19acdf5679a2526a1a445ca864f5d28523c8453e48。

新增torque_bow_combined_chain及现有保护路径的独立operator/wrapper。使用同原X拾取/右肩获取，若原弓已装1，先单独记录真实原发射将其打空；随后完整左肩fresh Grip取箭、连续原全行程装1、原Charge、真实R2 UP、准确原弹体1/扣1，再同构成再装、fresh R3完整非发射取消、保持R2不重启、真实UP重置后新发射及再装。首失败停止，不重复已消费局；配对录像、正常Native/XR/game/输入清理、恢复、编码和保留继续复用现有wrapper。

私有几何计划exe直接编译同PG生产主挂点、左腕/箭挂点和原feed头；从当前真实原始world-grip/input映射及组件scale反算输入，不回读C0/Snap手。规划保留原全部行程及真实左手Grip。已做非平凡旋转/反射映射的前向/逆向闭合，箭尖误差约1.07e-14 UE、basis4.44e-16；此为规划数学，不是实体手或真实游戏证明。初版手写fixture矩阵误为det+1，在执行规划前被断言拒绝，只修fixture为det-1；未改产品或输入映射。

唯一新候选torque-bow-prepared-20261110-034115-combined，prepared SHA256 923b41345cd827afadc076a0456db0776ab8ae828483df401ac18e7af2ce6655；3512份源码、223份当前driver及独立geometry规划制品，保存Native/PG各自准确编译源上下文。50份两边实际编译公共头逐字节一致。完整wrapper --prepare-only实际0，源/driver/build身份核对通过，仅为准备检查。combined-candidate-r27.json记录此构成。没有混入旧164457/31716候选，没有stage或取得新运行窗。

已向旧最终后继Lancer询问165800→HB171233之后真实确认的当前唯一尾；独立核到HOD49912最终唯一交HBv6的实际回执后，已将同一个新034115请求身份送当前HB线程定位/沿唯一后继转递，没有登记第二继任。combined-window-queue-r27.json保留同一prepared SHA和计划SHA；不从过期LS/GN/Frag/Bol/HOD名字或旧claim猜执行资格。下一可观察检查就是同构成完整装箭/原蓄力/真实UP/真实箭且扣1，再R3/再装和全跨度双路亲阅。当前没有新增真实玩家动作通过；四处自然掌指腕袖/过渡/双眼/头显、疲劳/中断/守恒和交付仍开放。无助手/CUA、无公共v71或Git index/commit变更；25%及headsetfalse保持，目标active。

队列后续实际答复：HB13712已全收尾并最终唯一交LS011500v4，最终回执SHA53932ab120943659f5be4739b36ae17d561d5d6c1d718cc4df8e0a3ff9c963ec。LS的v4实际stage后在startup import失败，尚无game/Native/XR出生且已恢复；窗口由LS保管等待v5窄修。HB已把同一个034115请求转给LS，明确未在HB受理或授位；GN→Frag020237→newBol→HODBeam8仍只属意向链，Bow新尾绑定须由LS依实际状态确认，不据此启动。

## 34. 原始抓握分析基准校正与开跑前复核（R28，2026-11-10）

034115完整动作候选及计划SHA再次吻合，没有改包、重编、stage或开游戏。完整controller动作路径已复读；曾疑虑runtime会每帧刷新config_serial，但实际weapon_torque_bow_runtime.inl在input pending或driver已bound时直接返回，疑点不成立，未作无依据修复。

只对现存原始制作数据作一次有界基准核对：R23的heuristic seed并非R22原C0手型，部分三指根关节偏差约12.24至16.63度，拇指根最大约39.94度。原thumb48的reference-local旋转轴约(0.6067568,-0.7948385,-0.0088275)、角27.790725度，不能用猜测的parent Y/Z旋转代替。此结论限制此前拒绝草稿的分析含义，不归因真实游戏缺陷，也不称生产原手型表错误。

独立reference-local曲线直接使用原reference到原authored的右乘delta轴；fraction0回到reference、fraction1精确复现现有松开/扣下两端点。最大四元数符号等价误差1.6653e-16，原C0蒙皮kernel坐标最大差4.5475e-13raw，两端12个非食指关节一致。没有新拟合、新手型、骨长/腕/挂点变化或产品写入；自然接触与解剖轴仍未认证。详见r28-original-grasp-basis-review.json、r28-original-grasp-curve.json和development-review-r28.json。

已直接把同一个请求身份送当前LS保管者。其最新快照仅报告本轮真实流程在手动开栓中断保护处停止、游戏/Native/输入/XR正常退出和十份基线恢复，并仍在证据审阅；这是保管者报告，不是Bow独立运行证据或交窗许可。Bow尚未取得正式唯一后继绑定/最终交窗，不能从线程active或正常cleanup推断授位。下一实际目标保持完整装1→原Charge→真实UP→原弹体1且扣1，再同构成R3/重装和全跨度配对画面审阅。无新玩家动作通过；工程25%、headsetfalse、全弓未完成保持。

## 35. 恢复后核实实际窗口链（R30，2026-11-10）

此前三轮未交窗已按规则登记blocked，用户恢复目标后重新开始阻塞审计。本轮从真实final逐份核对LS37448→GN21120→Frag2956→Bol022614：三份回执均unique/released且编码/保留终态，前后prepared身份及from/to thread闭合。LS final SHA134c73699fb7667dfe609e077f9d1e53a3de40923c09de5ca4a57cbacab1cb19，GN final4e1ca552909a3d3ecd7654aff8e06c1d8150769d832d49373a9136f36e2af67c；全部路径与SHA见development-review-r30.json。combined-window-queue-r27.json已保留旧副本并修正实际保管者为Bol，不能继续依据过期LS状态等待。

Bol最新线程快照报告Beam8因额度限制终止，尚无完整包/最终交接，Bol保留窗口责任。已把同一Bow034115请求完整prepared/plan/entry身份直接送Bol核对承接路径，未另占队位、未请求跳过Beam8；GN保存意向链确有Beam8→Bow→Troika→LS→Snub，但意向不等于受理。Bow当前仍无正式前驱绑定/最终交窗。

同一034115完整wrapper的--prepare-only本轮实际exit0，现用driver、saved builds及所选EXE校验通过，planning artifact核验通过；prepared923b4134及plan1e758433均保持，没有新候选或重编/stage/game。只修复窗口定位与交接信息，不增加玩家动作通过项。下一步沿Bol确认的实际唯一链完成必要绑定和最终交接，再执行完整原装箭/Charge/真实UP/箭1扣1及同构成R3/重装/配对亲阅。25%、headsetfalse和全弓未完成保持。

R31后续直接核实：Beam8已恢复并将最终9f1ef766包提交Bol独核，旧额度终止/无完整包说明已过期，尚无正式受理/final。Beam8直接回复指出GN/Frag意向快照遗漏了更早03:22的Cover-r80，Bol已ACK不得因此后移。原beam8-cover-r80-later-intent.json实际SHA68f872f9347be7778dbf168af8b930ac7fd6385d8e20d8c30214c74dd3fd3029再次核验。保存意向顺序修正为Beam8→Cover-r80→Bow034115→Troika→LS→Snub→Frag064450；Cover尚未成为Bow正式固定前驱，不据意向伪造绑定。等待实际唯一后继转递，由真实接收者确认Bow前驱；不重复prepare、新增队位或猜授窗。详见development-review-r31.json，完整候选和玩法范围不变。

## 36. 独立执行新规则与真实53536首败（R32，2026-11-10）

主人最新规则已覆盖以上历史多级前驱/后继签收：各枪独立源码、编译、离线审片；仅实际启动共享游戏时协调真实占用和现有启动互斥。动作结束或首失败后立即正常停止自有游戏/Native/输入/XR/录制，恢复原始共享配置及私有存档并释放；独立编码/审片不持运行权。不再以远期包受理/票据重绑为开跑前置。开发顺序为完整流程→Snap/Volume等体验→逐部件GrabPose，先修真实流程首个阻断。原身份、输入时效、库存/存档与唯一writer保护不变，旧拟合工具/失败证据保留。

在HOD41008和Cover均确认无占用且本方实际查询空闲后，使用现有Local\\GearsVR.RepositoryLauncher.Owner.solo-m1-1028d保护stage，原launcher接同名锁运行。新增本枪独立外层run_combined_independent.py只负责十项原始制品/配置及全部私有SaveData逐字节备份、现有stage/完整wrapper及正常退出后恢复，不增加前驱或审批条件。现有锁在外层stage→launcher和launcher退出→外层恢复之间有短交接间隙，不能称整段无缝互斥；已如实告知接口询问者，未改共享launcher。正式运行目录combined-run-20261110-145103。

原034115完整候选首次实际执行Game53536/creation134387670768123414，0.4后台/静音/物理隔离，启动与XR实际双眼提交通过。原X拾弓、右肩unholster_pending→held有日志，实际画面可见持弓。完整Bow driver在8秒准备窗口始终bound=0，首失败full Bow driver not bound after warm/preparation；没有进入装箭/Charge/真实UP/原箭/取消/重装，不能称完整流程通过。原torquebow_original_preparation确为ready1/gate0，原库存capacity1/used0/reserve5；这只排除笼统ABI准备未就绪，尚未定位observe/configure/隐藏箭准备中具体拒绝点。generic main_mount认证计数0也不能否定已经accepted1且实际可见的Bow专属follow。

operator实际exit1，wrapper/outer0仅为收尾。Native cleanup、原entries恢复、pad恢复、drained、XR0及终态objects destroyed均核实；本枪外层18项原始制品/配置/私有存档恢复通过并立即释放，之后简短通知Cover及当时准备启动的HB。已停止读写共享config/bin/head/输入/录制，离线编码在独立目录的原始媒体硬链接上完成，结束后生成物归回本枪原录制目录并移除临时链接，不让ffmpeg的共享路径保守匹配占用游戏。无公共v71/Git index/commit变更。

双路全跨度42.70秒，game683帧、simulator910帧（准确值以review-metrics.json为准）；两页完整overview及六张取弓/持弓/失败原图均亲阅。game最大采集间隔约2.067秒，不能伪称全跨度无缺口或30Hz真实渲染；两路末尾覆盖在500ms内。持弓手/腕/袖遮挡与贴合问题仍在，按新阶段顺序保存未接受，不阻挡流程修复。完整证据与正常恢复见development-review-r32.json，工程25%、headsetfalse不变。

针对这一个真实首败，只在原有backend observe/configure检查和hidden prepare返回值加入有界正常GT阶段回执，保持原调用顺序、短路和保护，无新Engine调用/输入/骨writer/动作权限。唯一生产变动为weapon_torque_bow_input.inl及weapon_torque_bow_runtime.inl；R27冻结构成其余源逐字节保持，沿原flags实际W4/WX/O2/MT生产编译链接0。新PG2d8883e362aae2e75c9aad9c164c9cf318025ed93db07c65702243569a894c72，Native仍413b68c9。新完整候选150032，prepared45770514261e05d5ae1bbcb33e0bf6c7afb36337d37c77dc04585f2ab17346c4，3512源码/223驱动/50份实际公共头一致。不是因果修复或通过；下一次在真实共享占用释放后用同一完整动作范围定向定位拒绝点，不退回ABI-only/held-only小跑。

## 37. 54012定位状态读取与完整外层互斥（R33，2026-11-10）

150032候选实际Game54012/creation134387678698953332再次到达原拾取/肩取与可见持弓，首败仍为driver未绑定。新增阶段明确observe_gate2、configure_gate0，拒绝在torquebow_state::read，尚未进入configure/时钟快照/原射击。hidden warm_result3且arrow_generation0，尚不能归因为某一字段、PendingFire空数组、输入时限或隐藏箭。原ABI仍ready1/gate0。没有装箭/Charge/真实UP/箭/取消/再装通过。该局部定位不是修复。

本轮正常Native/entries/pad/drain/XR0和外层18项恢复核实，立即通知Bol/HB释放。全程双路overview2页及6张取弓/持弓/失败原图亲阅，独立目录编码0，生成物归回原录制目录、临时硬链接移除。准确指标与首败/恢复见development-review-r33.json。

共享启动出现真实竞态：GN15:04:14已stage但其父驱动导入失败未生game，Bow随后stage/run；GN见Bow后正确拒绝恢复。本方18项恢复只证明恢复本方即时快照，不能代替GN原始10项。随后Lancer及Cover各自正常恢复的即时状态也不能冒称GN原件已恢复。暂停本方再次写共享，GN已在15:16:45明确用统一guard恢复其原始10项且逐项全true、公共状态只读后释放。

为修复原Owner锁的外层间隙，本枪唯一认领最小独立通用tools/debug-launch/runtime_stage_guard.py，接口with RuntimeGuard('solo-m1-1028d')，固定OS名Local\\GearsVR.SharedRuntime.StageToRestore.solo-m1-1028d；完整覆盖原件/存档备份→stage/config→现有launcher→正常恢复核对，离线编码在with外。保留原RepositoryLauncher.Owner内层，未改共享launcher/Native，不引入队列/票据/互审。两个真实子进程争用拒绝、子进程退出父锁仍保留、释放后可获取均验证；已给正准备启动会话同一模块/API。Bow已接入，之后实际争用HOD53920时在进入main前RuntimeBusy，无备份/stage/共享修改，证明实场挡住并发入口。guard本身不授动作资格，调用者仍须真实占用/身份及正常恢复。

对已定位的状态读取，只在现有read_once字段组、数组形状、双读fingerprint及closing-current拒绝处增加有界阶段/失败字段记录，原调用和短路保持，未改变数组合法域或输入期限。生产沿同一冻结R27基底只改本枪input/runtime/state三文件，W4/WX/O2/MT编译链接0，PGc096bdb2aaf6c7a4d76141683eac2315f2ae0690af591db6afd0fc2fd90016fa；Native不重编。新完整候选151221，prepared619ad12bc06d9f6d698a7c0b387ccd9ae7a76c8a12055bf9234f6a603d13972e，公共实际编译头50份一致。目标保持完整流程，非held-only缩小检查；等待实际占用释放后定向读取拒绝点并修具体原因。仍25%、headsetfalse、全弓未完成。

## 38. 54588拒绝点与原PendingFire整数类型修复（R34，2026-11-10）

统一RuntimeGuard已实际覆盖本轮备份到恢复，151221候选Game54588再次到达肩取，首败仍driver未绑定。状态细分gate8给出PendingFire count4/cap4，排除空数组猜测；原ABIready1。源码对照同一原版InventoryManager.PendingFire的现有生产typed reader，正确Inner为Core.IntProperty、dimension1、element_bytes4、offset0；Bow此前误写ByteProperty/1并只读首字节。修复独立为weapon_torque_bow_pending.inl，认证确切数组/Inner身份和原所属者，再读完整signed32主通道并只接受0/1。保留原count1..32、capacity<=128、nonnull、完整状态双读fingerprint和closing-current等保护，不为修复放宽任何输入期限或动作资格。

新typed-reader直接配现有owned-memory UObject/metadata Reader，13个真实编译执行用例0失败；覆盖正常0/1、非零高字节0x100（旧首字节读取会误当neutral）、负值、错误Byte/宽度/维度/offset/Inner所属者、空/超容量/空指针/不可读。测试无Engine调用，原数组逐值不变；不是实际游戏内Inner读取完整证明，也不把source修复当流程通过。生产首次完整编译在/WX下拦下新局部pending遮蔽已有全局，保留原日志/首稿，仅改名为bow_pending_readback，未降警告；最终W4/WX/O2/MT编译链接0。

新PG7d451b2f90e01cbff79fd6c7ea73a363f24f2b5b5b51f8ddaf715de837868e46，Native仍413b68c9不重编。完整候选154026，prepared4298a5d026de366591fb57810c6909f620613a785f7bc7ed147caa83486293fc，3513源码、223驱动、50实际公共头一致。尚未实际运行验证修复效果；下一次同完整控制器流程，不新增held-only检查。

54588已正常停止并18项恢复后立即释放统一guard，已短通知等待启动会话。双路全跨度2overview及6原图亲阅、独立目录编码0，临时链接移除且生成物归回原记录。见development-review-r34.json。后续一次启动被原Owner锁在baseline/stage前拒绝，现场Snub55152/launcher41152在用，没有本方共享修改；等待实际恢复释放。RuntimeGuard默认0等待保持兼容，本方仅选择可选timeout_ms45000的有界OS等待，固定锁名/API不变。完整流程/体验/GrabPose三阶段与25%、headsetfalse保持。


## 39. PendingFire实场通过与原材质ABI实测修复（R35–R37，2026-11-10）

154026候选Game38416/creation134387702640034718实际state_read0/observe0、original_ready1/stopped1，证实PendingFire IntProperty/4字节修复已通过游戏状态读取，也通过原时钟快照；下一首败configure2/geometry_current false、warm_result3、arrow generation0。正常Native/entries/pad/drain/XR0、18项原件/私存恢复并释放；双路2页全跨度overview及6原图亲阅、编码0、临时链接移除。见development-review-r35.json。

155148候选Game53720新增有界箭准备阶段回执，appearance4/material2、asset0、attributes0/parent0，说明失败停在原材质schema，不能继续归因C0用途、源资源、socket或普通状态。完整装箭/Charge/真实UP/原箭/取消/重装尚未执行。正常收尾及18项恢复后立即释放；双路2页overview和6原图亲阅、编码0，两路覆盖末尾500ms内，game最大采集间隔2116.66ms不能称无缺口；生成物归回原目录且临时链接移除。见development-review-r36.json。

160945候选Game46568/creation134387719328300968在同一完整动作入口仍准备失败；仅增加读取拒绝的最多两组元数据回执，未调用被拒函数。原GetMaterial实际parameter_bytes12/extent12、ElementIndex IntProperty4 offset0 flags80，ReturnValue ObjectProperty8 offset4 flags580；native_contract1、owner_MeshComponent1。旧schema误要求16字节/return8/flags480，这是实际定位的第二处ABI错误。只将读取分支改成精确12/0/4/580，另加index0的精确偏移要求；写入SetMaterial仍保留已有16/0/8/80分支，完整原类/包/函数/exec、字段所属者、返回offset、无重叠、参数链循环/数量检查保持。无新writer/输入时限或动作权限变化。

专门owned-memory测试按实测读取布局修正，正常读取和原写入通过，旧16字节读取、错误offset/flags/extent/owner/class/package、重叠、循环、额外参数均拒绝。W4/WX/UNDEBUG/O2实际编译执行0；生产冻结R27上仅增加这处已定位schema修复，R37完整编译链接0，PG7a0cc6370cd16bf9e3db68cc3a2ceefff725718ec110d2101905caae6c529286；Native413b68c9不重编。还不是材质实际调用或完整动作通过。46568已正常退出、18项恢复及释放，等待者已短通知；离线配对审阅进行中。工程25%、headsetfalse及全弓未完成保持。

46568后续离线审片终态：两页overview与六张实际取弓/ABI拒绝/首败双路图均亲阅，game655帧42.8662秒/最大采集间隔1866.65ms，simulator911帧42.8162秒/最大299.996ms；末尾500ms内覆盖，编码0，生成物归回原记录且临时媒体硬链接已移除。development-review-r37.json与material-abi-repair-r37.json保存实际参数和下一候选。新完整候选161818/prepared b6cbdc78c63b0273588032cce735b4e99af729c3eceb5867f2d7f36283ff1ab4，当前在统一guard前等待实际占用，无提前baseline/stage/共享修改。


## 40. 原材质读取实场通过与组件初始化拒绝定位（R38，2026-11-10）

161818候选Game54112实际appearance/material/asset/attributes/parent全gate0，证实GetMaterial精确12/0/4/580修复在实际原组件读取通过。自有隐藏组件随后在既有初始化stage6拒绝，每次cleaned1/outstanding0，arrow generation1至14而从未lit，driver仍未绑定/configure2；不能称显示组件、原箭动作或完整流程通过，也不能未取证即猜SetMaterial布局。

本轮正常Native/entries/pad/drain/XR0，18项原件/私存恢复后立即释放。双路2overview和6原图亲阅、编码0：game660帧43.1829秒/最大采集间隔1933.32ms，simulator918帧43.1662秒/最大299.997ms，两路末尾500ms内覆盖；生成物归回原记录并移除临时链接。development-review-r38.json保存实场结果。

下一完整候选162411/prepared bc0627acc14c7d26057c66740e591a39e446f308be90e66fdf7dc92d556eb7b7，Native不变，PG433ccffd1e38c45294c5f65537a768841f639759b45a1cfbde66edf0719b9822。仅将原stage6短路回执分为61 SetAbsolute、62 Scale3D、63 mesh绑定、64 material绑定，并对原SetMaterial schema拒绝做最多两组只读元数据记录，不在原组件调用setter。原调用顺序和保护保持，无新Engine调用或writer；冻结R37至新pool文件逐行对比仅此诊断变化。W4/WX生产编译链接0，3513源码/223驱动/50实际公共头一致，完整流程范围保持。25%、headsetfalse及全弓未完成不变。


## 41. SetMaterial实测packed12修复（R39，2026-11-10）

162411候选Game52104实际准备拒绝为gate64/original_materials；SetAbsolute、Scale3D和mesh绑定未在61/62/63拒绝。原SetMaterial实际parameter_bytes12/extent12，ElementIndex IntProperty4/offset0/flags80，Material ObjectProperty8/offset4/flags80、return_offset65535，原MeshComponent/native_contract1。旧setter schema16/8与实测不符，此轮未越过拒绝去调用未知ABI。

最小修复setter精确12/0/4，读写各自flags580/80及return4/65535区分保留，完整原函数/exec/类/包/字段所属者与链认证继续严格。专门owned-memory正常/拒绝测试W4/WX/UNDEBUG/O2编译执行0，旧16字节setter和offset8明确拒绝。冻结基底生产编译链接0，PG3d30d74f5c4238365bac525fbfad5fa051faf7bb5640cdf4ac76e51f28100ee1，Native仍413b68c9；完整候选162813/prepared f7cf4b42479eb7b5143a399716b4311055e2873dcc8aa613eb115085bc5e0602，3513源码/223驱动/50实际公共头一致。尚非实际setter调用或显示组件准备通过。

52104正常Native/entries/pad/drain/XR0及18项原件/私存恢复后即释放。2页全跨度overview与6实际双路原图亲阅、编码0，game623帧41.7829秒/最大采集间隔2083.32ms，simulator890帧41.8329秒/最大283.336ms，末尾覆盖500ms内；临时媒体链接已移除且生成物归回原录制目录。development-review-r39.json保存精确证据。新候选正在统一guard前有界等待别方实际占用，不提前stage。25%、headsetfalse及全弓未完成保持。


## 42. 原setter实场通过、箭组件就绪与取消入口认证定位（R40，2026-11-10）

162813候选Game40972在原Get/SetMaterial精确12字节修复后，隐藏自有组件warm_result4、arrow generation1/lit1实际就绪，geometry_known1/feed_serial1实际成立，旧stage64不再出现。仍不能将隐藏准备、原资源与几何通过冒称装箭或完整流程；下一首败configure_gate10，属于原取消入口bind_entry认证，具体方法/子因尚未观测。driver未绑定，因此没有新的Charge/真实UP/原箭/扣弹/R3/重装通过。

正常Native/entries/pad/drain/XR0、18项原件/私存恢复并立即释放。双路2overview及6实际原图亲阅；30秒接触是取弓前空手过渡，36/43秒实际持弓，未误作已装箭。game674帧45.2662秒/最大采集间隔1966.65ms，simulator966帧45.2828秒/最大316.657ms，末尾500ms内覆盖、编码0；生成物归回原目录并删除临时媒体链接。development-review-r40.json保存证据。

仅在既有取消入口认证增加每方法最多两组失败回执，记录方法、拒绝阶段、原function owner/参数/返回offset/native/flags/exec/script形状及最多8原字段；成功路径不打印，不新调用Engine、不改认证或取消动作。生产冻结R27组合11份已认领变动、其余逐字节保持，R40编译链接0，PGa8f69f31f1809ee636a145f205ca62ee6ecd3c1b6ae0e6612d88f42fc7df8c72，Native不变。完整候选163734/prepared30cfe18ebfc22d4b19e6eb66d468d7e1f0e155f233d65f5ecd79c661b7b97604，3513源/223驱动/50实际公共头一致。等待真实占用释放后，同一完整动作范围定向读取原拒绝原因，保留取消/防重放/真实松键和全部保护。25%、headsetfalse及全弓未完成保持。


## 43. 原PutDownWeapon无返回值修复与延迟恢复实录（R41，2026-11-10）

163734候选Game55888实际configure10拒绝明确为PutDownWeapon gate2：原WarWeapon script过程bytes0/extent0、return_offset65535、native0、script66，旧实现误要求size4的Bool返回。clear_pending/charge_ended/GotoState先前认证已越过，此处不能通过弱化native/owner验证跳过。新增独立生产put_down_schema精确0/0/FFFF，完整原owner/script/metadata指纹与字段链认证仍由bind_entry保留；取消调用去掉不存在的bool读取，只把原函数正常返回当一步结果，仍由原recipe完整停止状态及Charge.EndState返回确认最终完成。未改动作时限、防重放、真实UP、库存或任何writer。

专门owned-memory生产门测试W4/WX/UNDEBUG/O2编译执行0，拒绝旧bool-size4、nonzero extent、错误return、invalid/unregistered；生产R41编译链接0，PG95d80a8b38ab5dd8686d78adad993ef75a1ba6cdd3ec05e83724414f185a2a2b，Native仍413b68c9；完整候选164944/prepared a427134bba8994ddb02db170b2482990596627aec058168881f6ae7d8adad50b，3514源码/224当前驱动/50实际公共头一致。移除已完成使命的每次原SetMaterial只读预察；原setter实际认证继续保留。

55888游戏/Native/entries/XR0正常，但原旧idle在立即恢复时返回占用，外层保留备份拒绝写入并异常释放guard，不能称立即18项恢复成功。后见Boom53860并不证明guard重叠：Bow异常落盘16:41:02.585156与Boom取得guard16:41:02.585吻合，后者是在本方异常释放后开始；互斥本身未被证伪。旧idle包含离线ffmpeg和旧operator标记，具体触发进程没有当时快照，原因保持未证。双方已纠正早先竞态猜测。

本方同名guard/原Owner锁下的补恢复于16:46:31真实完成，18项原件/私存全exact、public只读、未停别方进程，随后即释放并短通知当时相关者；execution.json保留失败原样，另存recovery.json和execution-with-recovery.json。仅本枪入口新增torque_bow_runtime_occupancy，使用实际game/XR/paired recorder占用，不含离线编码；正常恢复等待实际进程完成，不因瞬态观察直接丢弃恢复责任，不改共享retention/launcher。下一轮验证此路径。

双路2overview及6原图亲阅、编码0，game620帧41.5329秒/最大1866.66ms采集间隔，simulator887帧41.5496秒/最大233.334ms，末尾500ms内覆盖；生成物归回原录制目录、临时链接移除。development-review-r41.json保存玩法首败、正常Native收尾、立即恢复失败与随后补恢复的区别。完整装箭/Charge/真实UP/原箭扣1/R3/重装仍未通过；25%、headsetfalse保持，目标active。


## 44. 无返回值取消接口实场认证通过与最终配置组定位（R42，2026-11-10）

164944候选Game54968实际通过configure10的四个取消接口认证，没有新的PutDownWeapon拒绝；隐藏组件lit和geometry_known/feed_serial仍成立。下一首败configure11，原此组串联active_name、weapon IsProbing(Tick)、cleanup-context和当前Native源，不能未观测即归因Active所属者、Tick参数、时效或8秒准备窗。仍未driver绑定/原蓄力/松键发箭/扣1/装箭/R3通过，不加分。

新真实occupancy路径正常结束并立即18项原件/私存恢复、shared_released=true、wrapper0；Native/entries/pad/drain/XR0核实。已向等候Smoke短通知释放，不持离线编码占用。双路2页全跨度overview与6原图亲阅；取弓过渡模拟器有明显重影/半透明样貌，照实保留，不能称全视觉通过或把原因归给手型。game578帧42.7329秒/最大2183.30ms采集间隔、simulator914帧42.8329秒/最大249.997ms，两路末尾500ms内覆盖、编码0，临时链接已移除且生成物归回原目录。详见development-review-r42.json。

下一完整候选170227/prepared6301a41344b10002e8e66d0c065a12eb8939d5af20fa18e9fc5216bac709de40，PG11f305540d5f1f98c9cc38b2466bcdee0deb92d1ab4c1731b6600af043e45225、Native不变。只细分原最终检查为11/12/13/14，并为既有Active名/IsProbing(Tick)拒绝记录最多两组实际字段，不更改调用次序、成功条件、输入期限或动作权限。W4/WX生产编译链接0，3514源/224驱动/50实际公共头一致。下一同完整动作范围读取第一个真实子拒绝，25%、headsetfalse及全弓未完成保持。


## 45. 原Tick探测位静态依据与只读查询替换（R43，2026-11-10）

170227候选Game42060实际configure12拒绝，Active名认证已越过；旧IsProbing反射查询未取得注册函数，缺失不能冒作Tick已停。该轮正常Native/entries/XR0、18项恢复释放；双路2overview及6原图亲阅、编码0：game630帧41.4496秒/最大1933.31ms采集间隔，simulator884帧41.4329秒/最大183.334ms，末尾覆盖500ms内，临时链接移除。development-review-r43.json保存事实。

未再开游戏猜API。直接IDA MCP因本机缺ida_nexus失效，REA的IDA provider也未配置；未改工具全局配置或安装软件。使用已有verified16812静态数据库的272MB私有副本，在隐藏、BelowNormal/双核的现有IDA批处理下只分析原Enable/Disable错误文字的RIP引用，正常qexit0；原20,027,727字节代码SHA911ab9bc完整核对。数据库映射rawEA+140001000=liveVA。Enable原过程1403e3350、Disable1403e4dd4，原指令明确probe name300..363、UObject state-frame指针+0x24、state-node+0x48、live ProbeMask+0x50；Disable对name-300位清零，Enable受原state/class mask合并限制。既有原SetUpSimulation/StopSimulation脚本分别调用117/118('Tick')，只作对应静态依据，不冒称本轮执行。

新probe_bits读取器只读确切原位，认证上述指令片段、当前actor/state对象、名称表/身份和mask前后重复读取；缺失状态帧为unknown拒绝，没有猜class fallback或新Engine调用/写probe位。指令证书在tick-probe-static-r43/read-contract.json，完整IDA反汇编/pseudocode在probe-dossier.json。owned-memory W4/WX/UNDEBUG/O2用例覆盖300/331/363位与0 mask，拒绝越界名称、null/unreadable/overflow/变动frame-state-mask，编译执行0。R43生产编译链接0，PG3076879bd77a2ca17470c222beb7e2afdf58f7c383ad2350733a67167c1b88f5、Native不变；完整172212候选8fe820e678b8524d417d26228d4feee43765c95eb9c9f2ce827c8c93987cf405。

## 46. 探测名称选择实场拒绝与精确Tick事件读取（R44，2026-11-10）

172212候选Game30612实际新reader gate3拒绝；指令字节证书和actor认证已越过，但全区间300..363名称扫描在具体哪一slot失败没有记录，不能称已读ProbeMask或已绑定driver。将选择收窄为当前receiver实际继承的Tick UFunction，并严格认证原Function/名称身份、name_number0和probe范围、最终相同函数/名称/表；无需读取无关预定义slot，原对象和状态/mask/code双读保护保持。没有放宽输入期限、移除模拟停止条件或伪造stopped。

生产首编译因使用不存在的ObjectIdentity.name成员被拒，原日志/首稿保留；只改用既有name_index/name_number，不降W4/WX。最终编译链接0，PG99fb92bd2252947dd7e9a270eefe943f27ab022cd99651614fa6e213f007e4b6。完整候选173243/prepared fe30d8a2a300fd286bc5b3bfe9c2e4599f4620ea2a990fb77df6757bb6938653，3517源/224驱动/50实际公共头一致，Native413b68c9不重编。

30612正常Native/entries/XR0，18项即时恢复释放；2overview及6实际原图亲阅、编码0：game627帧43.0996秒/最大2049.99ms采集间隔，simulator915帧43.1162秒/最大266.664ms，两路末尾500ms内覆盖；临时媒体链接移除。development-review-r44.json保留原失败和下一构成。仍没有完整装箭/原Charge/真实UP/原箭扣1/R3/再装通过，25%、headsetfalse保持。


## 47. Tick原位读取与完整驱动绑定实场通过、首个原Charge未达（R45，2026-11-10）

173243候选Game32648真实认证当前继承Tick事件，name_index336、原state-frame/StateNode/ProbeMask读到，enabled0为该原武器Active状态的观测，不冒作模拟弹体停止证明。configure0、bound1、geometry_known1、arrow_lit1、库存loaded1/reserve5，完成此前连续准备阻断。模型固定骨与手型仅基础沿用，surface_fit未接受。

同一完整operator马上进入original_initial_arrow_precondition（先打掉原已装1的预置箭），实际按R2但3秒内原Charge未达；原字段持续r2_downfalse/chargingfalse、库存1/5，没有原箭或扣弹，后续装箭/R3均未进入。driver baseline/observed交替只是一条线索，不能直接归因为解码器、原Source过期、双边界Stereo差异或原调用准备。完整流程仍失败，不能将bound通过称整枪完成。

正常Native/entries/pad/drain/XR0与18项即时原件/私存恢复后释放，已短通知等候Longshot。双路全跨度2overview及6原图亲阅、编码0：game568帧37.4996秒/最大1916.67ms采集间隔，simulator799帧37.4996秒/最大249.996ms；末尾500ms内覆盖。生成物归回原录制目录，临时链接移除，见development-review-r45.json。

下一174558完整候选/prepared85596ddb0af564e3f7fa94521f8aa5115ca467f77c25cc209c99887e5dfbb470，PGdb417c0a0ab448d9ec87286394d3781ecd22989f7caeb265fb1146567bde8cec、Native413b68c9不变。只在原pre-original与post-equipment边界加入有限packet回执：可用性、序号/原sync、cohort、真实R2电平/累计按松计数、同packet的原Stereo字节差异。它不解码、重发、更新时钟、发布输入或改动作权限/调用规则；既有driver不改。冻结构成W4/WX编译链接0，3517源/224驱动/50实际公共头一致。等待实际共享占用正常释放后定向定位真实R2首败；工程25%、headsetfalse保持，目标active。


## 48. 启动前真实占用重试与边界假设离线重现（R46，2026-11-10）

174558首次入口175040在实际占用断言退出；目录仅execution.json，未备份/stage/启动，无恢复债务。入口在备份前及stage前实际忙时现抛出既有RuntimeBusy，正常释放内外锁后有界等待；未暂存明确记录无需恢复。隔离host验证无baseline/stage且关闭本方句柄通过；外层等待先检查实际占用，避免忙时反复prepare。生产DLL/输入/时效/动作权限未改。

复用现有driver及注入backend另存诊断稿，在W4/WX/UNDEBUG/O2下编译执行0：相同controller仅Stereo.received_ns变化后，下一真正按下变baseline且presses0；未拥有原蓄力时stop后下一真正按下也baseline且presses0。两个假设均可离线重现，但不证明32648实场发生哪一种，不据此放宽防重放或改driver。证据development-review-r46.json。新完整范围运行175300正在读取R45两边界回执；实际结果与正常恢复另记。全弓/头显FALSE、25%不变。

175300实场Game56580仍在首个原Charge失败；完整装箭/R3未进入。只读packet确认phase0 frame446/seq2134真实R2=1、pressed1，frame479/seq2286真实UP/released1到达驱动入口，库存1/5未变；不能再以入口无真实按键解释，亦没有证实离线两种假设是实际因果。原日志和4条packet存本run目录。wrapper0、18项exact恢复、shared_released=true；已短通知HB留出HOD→HB现存恢复，不新启。双路离线编码/亲阅待办，不冒称像素验收。下一点查入口到原press之间的实际失效/prepare拒绝。


## 49. 真实驱动失效点只读回执及生产候选（R47，2026-11-10）

原driver的lose新增有限诊断计数，区分原状态第一次/第二次读取拒绝、decoder invalid/ambiguous及poll姿态到期；原decode_now只调用原一次backend.now并记录，不续写source时钟。两原边界在loss变化时最多32行回执，未改调用授权/流程/时限/库存/真实UP防重放。既有driver严格W4/WX/UNDEBUG/O2执行0，离线假设稿另核decoder拒绝reason4/losses1通过。

PG c7836da3编译链接0；Native413b68c9不变，实际50公共头一致。完整候选175825/prepared d3a427eab01f8080a2ec6dc34fb0bb875246159a93de4e362ab0d837fba4fe4d，3517源/224驱动，未用peer WIP。等待公共guard及实际game/XR空闲后以原完整动作范围取真实失效点，不降为held-only。

56580离线编码0，两页全跨度overview和6实际原图亲阅，game576帧37.6163秒/最大1866.64ms采集间隔、sim803帧37.6496秒/最大199.996ms，两路末尾500ms内覆盖。29秒game取弓过渡与sim手/枪显示不同，33/37秒已持弓但原Charge未达；不把画面原因归给手型，也不称贴合完成。生成物归回原录制目录并移除临时链接。完整装箭/原发箭扣1/R3/重装仍未通过，25%、headsetfalse保持。

180217/Game55928实场首败仍原Charge未达，但新回执明确原pre边界decoder invalid：第455帧seq1970后post建立baseline，第456帧seq1975真实R2=1/pressed1进入pre。原sync由89823857389400到89823969951100，差112561700ns，超过原100ms连续性上限；decode_now89824000810900仅比当前sync晚30859800ns，不能当当前packet年龄过期。记录reason4/decoded0；这是本次连续性拒绝的具体证据，不证明底层输入发布停采，也未判定哪一GT子步骤造成消费间隔。保护不得扩大或续钟。原日志/边界行保存run目录。

wrapper0、18项原件/私存exact恢复并释放；双路编码/亲阅尚待办，不称画面通过。已读最新independent-runtime-20261110.md；后续先迁移本枪动作/输入/录制/stop/身份路径到唯一私有实例，禁止再次stage/restore共享solo或共享PG。当前无本方恢复债务。下一源码点减少实际生产消费链耗时，保留两次状态确认、原100ms与realUP防重放，再用独立实例验证。全弓/头显FALSE、25%不变。


## 50. 完整弓流程迁往独立实例（R48，2026-11-10）

已读最新独立运行约定，本枪新增prepare_independent_r48.py/run_independent_r48.py，只prepare自己的唯一实例并从冻结候选175825精确核Native413b68c9/PGc7836da3/XR16840b27；Source1、后台/静音、物理隔离、render_scale0.4保持。不再stage/restore共享solo或共享PG。既有动作脚本各自冻结到同深度bow-runtime目录，root/input/session/publisher-executable/recording/stop/launcher-log均定位自己的实例；完整load1/Charge/真实UP/原箭扣1/R3/重装链字节与冻结输入脚本核对，不降为held-only。

迁移实跑失败按原样保留：180736在Source1信息未就绪时原断言拒绝；改等待原Source1就绪，不删断言。180916缺私有recorder可执行文件；新增精确复制原已编译录制器。181117仍引用旧launcher.log；改自己实例isolated-launcher.log。三者stop0/launcher0、owned_game_absent=true、无forced termination。当前181251实际运行，session78624须继续观察，不在此声称玩法通过。

55928双路离线编码0，2overview及6原图亲阅，game561帧35.6163秒/最大1566.66ms采集间隔、sim760帧35.5996秒/最大199.995ms，两路末尾500ms内覆盖；生成物归原录制目录、临时链接移除。手型/启动像素仍不作完整验收。工程25%、全弓与头显FALSE。

迁移后续：181251/Game44264已到原弓取持，但records仍用旧PG build路径拒绝；181510/Game40872替换路径的Windows反斜杠被Python解作backspace，保留OSError原样。改Path.as_posix，并在启动前AST确认PG字面量等于实际私有build路径且无控制字符、无共享solo/session/PG/XR残留；不把入口脚本错误记为产品失败。此前所有本方实例均stop0/launcher0/own_game_absent且无forced termination。

181612/Game42200独立Source1、原弓held记录、隐藏箭lit1与geometry/feed1真实达到。未driver绑定，首败full Bow driver not bound after warm/preparation；observe_gate3持续，指向snapshot/game_clock（含同incarnation cleanup-context）拒绝，后有gate2，具体内部原因尚未观测，不能直接认定缺export或实例名不符。Native413b/PGc7836/XR1684不变；真实闭合stop0/launcher0/own_game_absent，无共享solo恢复写。最新静音guard已启用但本方无匹配音频session（guard_ready matched0/muted0），不冒作已捕获声音静音确认；实际单次前台观察本方game/XR均FALSE，不冒作全时段焦点证明。全程复用behind/no-activate/test-mute保护，未激活/移动用户窗口。

已完成独立实例接线的实场验证，未通过完整弓玩法；当前无本方运行及恢复债务。下一点保留原身份/双读/100ms约束，定位独立实例原游戏时钟/清理上下文的具体拒绝；本批媒体离线编码/亲阅仍待办。工程25%、全弓/头显FALSE，目标active。


## 51. 主人前台抢焦点反馈后的运行停止（2026-11-10）

按最新直接反馈暂停新game/XR及自行焦点重试，继续离线逻辑开发。实际WindowsControl PID+creation核对本枪6次独立实例的game/XR均已不存在，正常stop/launcher0、无强杀。写runtime-start-hold.json并让两个本方运行入口在该文件存在时启动前拒绝，防止后续自动续跑意外重启；不是暂停全部开发目标。不得仅凭环境变量称后台问题修好。

离线查明Native模块绑定使用原注册函数地址的GetModuleHandleEx(FROM_ADDRESS)，并非硬编码DLL目录/实例名查找；所以不能据独立目录变化直接推断模块找不到。原game_clock/cleanup-context内部具体拒绝仍未证。旧实跑和失败证据保留，无新运行。


## 52. 独立入口有限弹药遗漏修正及gate3解释更正（R49，2026-11-10）

离线对照查到原run_v63_startup_repro会置GEARSVR_TEST_FINITE_RANGE_AMMO=1，新的isolated入口未带该开关。冻结生产playground_controller.inl明确四项测试开关全真才关闭owned range武器的bInfiniteSpareAmmo，否则置true；Bow snapshot在game_clock之前即拒绝counts.infinite。因此之前把42200 observe_gate3直接指向时钟/cleanup-context范围过窄，撤回该定位。42200没有实际infinite字段采样，不据静态差异宣称其根因已证或已游戏修复。

仅本枪入口新增bow_independent_environment.configure，保留自己的实例与四项后台/mute/no-activate，补回原finite fixture。真实Python子进程仅构造isolated环境（未start）证明finite1和四保护能继续传播；四项任一缺失均拒绝、原env字典不变。两启动入口真实执行均因runtime-start-hold在任何launch前拒绝，既有执行回执字节不变。没有game/XR或焦点重试。

原snapshot新增有限只读counts gate与game_clock子gate，输出capacity/used/reserve/infinite，区分同一observe_gate3内的不同拒绝，原所有检查/读次序/双读/时限/原库存规则不改。生产R49编译链接0，PGdfbe1e0e，Native413b68c9不变；50实际公共头一致。完整182416候选/prepared f1d4b4b942909c7302e55694a980c96679e404bc8c15c94eb11758f1aa5f064c已源码冻结，3517源/224驱动，未stage/未run。

后台前台修正未实证前继续只做离线，启动hold保持。新候选不能替代原load1/Charge/真实UP/原箭扣1/R3/重装的实际通过，工程25%、全弓/头显FALSE保持。证据development-review-r49.json；后续独立实例必须带本枪finite环境，不再shared solo stage。


## 53. 原弓十一项字段本次调用集中解析（R50，2026-11-10，仅离线）

针对已见112.562ms消费间隔继续离线减少重复元数据遍历，不扩大100ms。新增BowObserveFields复用原PawnObserveFields的严格派生优先/同名first-match/描述符认证模式，只服务原弓11个weapon字段。每次read_once独立begin；每次binding重新核receiver class/owner/原Property，值/Bool mask仍原位新读；finish再次完整first-match遍历且消费binding，然后原retained_metadata/actors/source检查和两份完整状态fingerprint比较保持。无跨帧/跨回调缓存、无新Engine调用/writer/新时钟。

严格W4/WX/UNDEBUG/O2 owned-memory测试通过，逐项和未改的原typed selector对照，覆盖精确类型/宽度、错类型派生遮蔽、后置重复字段、描述符/链变化、换class、不可读/环及消费后拒绝。固定fixture原两次11项选择10068次read调用，begin/current/finish为6964，少30.83%；只是该夹具读取调用数，不代表游戏FPS、整体耗时或112.562ms已修。首编译exact_text动态指针不匹配、两处负例fixture期望错误均保存说明并修正，不降保护。

生产PG编译链接0/bc1db915，Native413b68c9不变；50实际公共头一致。完整183110候选/prepared145e3d2b05ac970d23f8ffd716449c0ca45a0dc753e75f38d7ddd8e2654ca39b源码3518/驱动224，未stage/未run。运行启动hold保持，无game/XR/焦点重试。完整玩法与头显仍FALSE、工程25%不变；下一获准实跑必须用独立实例+原finite环境测实际消费间隔和完整动作，不用host数字代替实场。证据development-review-r50.json。


## 54. 生产值/掩码/指纹接线验证与再次停止运行（R51，2026-11-10）

把原Fields结构原样抽为weapon_torque_bow_state_fields.inl，生产和owned-memory测试直接共享同一实现；测试用Fingerprint片段与现行原实现逐字核对。严格W4/WX/UNDEBUG/O2值层测试0，覆盖掩码/原哈希一致、zero/multibit拒绝、值变化即新读/改变哈希、组件类型/错receiver/index、描述符/type改变、不可读以及非weapon原fallback。集中描述符原回归仍0，10068→6964 fixture read调用不变，不冒作实场优化。PG8d2f9b84生产编译链接0、Native413b68c9未改、50公共头一致；完整183524候选/preparedc2619b92e091472d34bb62a8b21613c4d06d5b2d124e398616fbbc9cc0200007，3519源/224驱动。

随后收到新后台入口已短验收且允许恢复的交付，核对background-runtime.json的quiet1110b两制品与模拟器SHA，归档原hold、以新独立183719实例启动；旧shared入口永久拒绝。明确保留finite fixture及新native_foreground开关，并新增只读本方foreground/style/z-order observer，违规即正常stop。

运行期间主人再次报告到前台，立即按本方stop入口正常停止。game38732/XR3120均按PID+creation确认已消失，stop0/launcher0、无forced termination；wrapper/operator均终止。会话cancelled_before_activation，本轮不资格化为完整动作检查。自身observer2167前台/572窗口采样、invalid0/violation0仅为有限记录，不能反驳主人观察或认定别方责任；撤回自动后台可恢复的结论。

重新写本方runtime-start-hold.json，并保留中央tools/multi-instance/automatic-runtime-pause.json，不删除/绕过、不因自测短通过自行恢复。仅等待主人明确恢复运行；全目标未暂停，继续离线逻辑/编译/既有录像分析，不标blocked。无本方游戏/XR恢复欠项。全弓/头显FALSE、工程25%不变，development-review-r51.json保存完整边界。


## 55. 独立实例既有配对媒体离线收尾（R52，2026-11-10）

本轮不启动game/XR/模拟器。已亲阅5648/44264/40872/42200/38732五次既有捕获全跨度9overview，另核最近三次各game/sim末段实际原图共6张；不是逐帧全部检查。最近三次40872/42200/38732双路完整编码均terminal0。较早5648/44264先保留完整数值与审阅结论，再按既有媒体规则移除生成图，不重编码即将删除的旧视频。

42200原图支持持弓/准备阶段，不支持Charge/R3/装箭发箭通过；背景与准备期白色旧捕获及长采集间隔保留，不能据此认定游戏FPS或白闪因果。38732末图是原第三人称靶场，sim仍STARTING VR等待卡；game首捕获延迟16816.48ms，sim最后原图距配对末尾3516.62ms，因此both_lanes_cover_end_within_500ms=false，不得称完整尾段/立体输出或武器动作覆盖。取消与正常清理记录仍按原样，不改成玩法通过。

滚动媒体按确切Bow game PID+creation及捕获identity归属选中，保留最新38732/42200/40872和48小时上限。实际删除21660个本枪直接生成媒体文件3459138838字节，errors0；不递归删目录、不动其他枪目录、源码/构建/资产/文字/数值保留。较早O目录只在其Bow动作/配对identity精确匹配时清生成媒体，未运行会影响其他会话的全局pruner。r52-retention-plan/result保存边界与实际结果，旧review媒体可用性已注明。

所有本轮概览/编码/清理任务terminal0，中央与本方启动暂停标记均保持，未新建运行。完整load1/Charge/真实UP/原箭扣1/R3/重装仍未通过、工程25%/全弓与头显FALSE。目标active，继续离线逻辑；仅等待主人明确恢复后才可新运行。证据development-review-r52.json。


## 56. 生产驱动装箭/发射/R3组合离线覆盖（R53，2026-11-10）

发现原fire-driver夹具的reload后端一直返回false，仅覆盖发射侧，不能据其通过声称装箭接续已经覆盖。把原Backend/Frame夹具抽成可选BackendType的共用头，原用例不改语义；新增注入原版回执的组合后端，实际调用未改生产Driver、Decoder和TorqueBowInteraction。

严格W4/WX/UNDEBUG/O2新组合及原回归均执行0。同一个driver覆盖0/5装箭→1/4→真实UP原箭记录1及0/4→再次装箭1/3→R3取消保留1/3与计数1→持键不重启→真实UP再按再射至0/3/计数2。另覆盖接受回执延迟不重复begin且保留原GAME开始时刻1000、非延迟回执2500；6001完成仍ordinary。完成回执延迟时只提交一次，model在精确回执前保持loaded0，注入engine已1；回执后记账/retire一次。装箭期间持R2不自动蓄力；未dispatch完成且无拒绝receipt时责任pending、库存不变且reset_drained拒绝。

初稿aliasF重定义及motion aggregate里arrow与begin次序写反均为测试稿错误；生产guard实际拒绝了后者，真实生产caller原次序正确，未改产品来迎合测试。显式错次序负例保留；debug-only开放访问副本只定位夹具错误，不进入候选、不作为生产通过证据。

三份生产driver/model/decoder SHA逐一等于R51冻结，无生产源码变化，所以复用原Native/PG，不进行未变重编。新增覆盖使用注入Engine回执/geometry授权，不能证明实际ABI、物理机构终点、游戏动作或头显完成。中央/本方启动暂停均保持，没有game/XR/模拟器/焦点重试。工程25%、全弓/头显FALSE保持，目标active；真实完整流程验证仍待主人明确恢复运行。证据development-review-r53.json。


## 57. 原始送箭行程到生产装箭驱动的组合（R54，2026-11-10，仅离线）

把原author_original/author_torque_bow_feed、未改TorqueBowArrowFeed和Driver组成同一离线链，只有真实raw feed返回inserted才提交motion；使用synthetic unsnapped controller world pose及注入原版backend，不再仅手填geometry flags作为这一层的唯一证明。原19.039143raw全行程，在scale0.3/units100时最少57.11743mm，不缩短。

严格W4/WX/UNDEBUG/O2组合0：approach/mid/seat→正确instance/part/arrow/begin→只完成一次1/4，重复完成拒绝；错误component201对实际标定1拒绝。半程松手模型预留释放、备用保持5；fresh重抓new arrow不能沿用旧半程，也不能仅贴seat；重新完整进槽后原reload begin仍一次，完成一次至1/4。100000001ns断流不能补完旧行程或改库存。R53装箭/发射/R3组合回归在共享夹具拆分后仍0。

首稿Windows max宏与不存在conjugate辅助、10ms过快synthetic动作、泛用测试component201不匹配实际标定1均保留说明并修正夹具，未降低生产速度/时效/部件身份检查。新增错component负例保存。

五个生产feed/mount/geometry/driver/model头SHA均等于R51冻结；本轮只改测试夹具/组合检查，复用Native413b/PG8d2f，不重编未变产品。没有game/XR/模拟器/焦点重试，中央与本方暂停标记均保持。不是实场原函数、物理tracking、所有比例角度或手型通过；全弓/头显FALSE、工程25%不变，目标active。证据development-review-r54.json。


## 58. 当前要求与证据缺口核对（R55，2026-11-10）

按原1..6节及common/grasp-and-snap合同逐项核对10项：完整装箭发射、R3与重试、R3独占、Pawn延迟释放、失追/切枪/世界退出、疲劳/奖励、单双手/转头/近墙/原弹体、原箭/多骨与反馈、四件抓握/吸附、静音后台/身份/清理。所有列出的源码及离线记录当前存在；没有任何一项据此达到完整实场/头显接受，原范围不缩小。机器可读索引development-review-r55.json。

原55928首次Charge实场失败仍保留；R49有限fixture遗漏已修但未实跑因果确认，R50/51读取优化与R53/54组合通过也不能替代它。38732在激活前取消，后台短测结论已撤回。中央与本方禁止新启动标记仍明确要求主人恢复，不自测自行解禁。

本阶段独立源码、严格编译、组合及原录像/保留处理已收尾；未发现新的有证据产品缺陷可在不运行下修复。不以重复相似测试制造进展，不越过完整逻辑门槛去精细手型。下一决定性证据需要主人明确恢复后，在实际核实后台不抢焦点的路径运行完整原动作并正常收尾。全弓/头显FALSE，25%不变；外部运行门槛已连续多轮存在，按目标阻断审计处理，不能标完成。
## 59. 主人恢复后的隐藏独立实例与实际失败（R56，2026-11-10）

主人在窗口前台问题修复后明确恢复运行；按independent-runtime-20261110.md当前入口，使用build-instance-cloak1110和清单绑定隐藏模拟器，新建独立Bow实例。中央暂停标记已归档，本方hold经核实后归档；旧shared solo启动入口永久拒绝，不恢复旧实例。GPU projection源捕获不解除窗口隐藏，编码移到正常清理之后。保持0.4、静音、Source1、100ms、身份/原始时间戳/真实UP/anti-replay及原版库存机制。

R56A bow-1110-193626/game54736的45秒启动观察超时，实际Native准备约53.984秒、首次立体提交54.960秒；没有合格武器动作。只把启动观察延长至100秒，未改武器8秒准备门槛或输入时效。116张捕获全为等待卡；overview与末帧亲阅、离线编码0。正常stop/launcher0、无强杀、game消失；5480前台及805窗口/cloak观察无违规。

R56B bow-1110-194206/game57388使用相同R51产品，实际取得Bow并完整绑定，snapshot/game_clock/configure门槛0、库存1/5、infinite0、hidden-arrow及geometry/feed1。初始箭的原版Charge仍未到达：实际R2按下frame820/seq4255，前样本间隔89.9235ms，小于100ms；不能再把所有失败归因于先前112ms断流。原版prepare/reserve/press具体拒绝位置尚未证明。保留实际失败，未执行后续装箭/R3成功声明。

234帧GPU源捕获errors0，最大采集间隔4.17865秒不作为FPS证据；全跨度overview及00130等待卡、00220/00233持弓实图亲阅，离线编码0。正常stop/launcher0、无强杀、无遗留子进程；4244前台和985窗口/cloak观察无违规。完整弓/头显FALSE、工程25%不变；证据development-review-r56.json和r56b-capture-review.json。

## 60. 原版蓄力调用分段观察（R57，2026-11-10）

仅在既有Backend.prepare/press调用处添加有界计数与拒绝阶段记录，原调用次数、顺序、时间门槛和返回条件保持。可区分模型未请求、prepare失败、20ms reserve失败及原版press结果；未新增Engine调用、库存setter、输入writer或Native变化。PG编译0，SHA8286befdb9806bd80626a43b58e97ebc28bf1a216369de1f10aef42def37baf4；Native仍413b68c9，XR16840b27。冻结候选torque-bow-prepared-20261110-194831-combined，prepared SHA68a82ff225e0286b27bb47a965f8aba90cd1f9ec9b16402446fcc261da0bdb3f。

第一次bow-1110-194902/game55144在肩部取弓时没有native held回执，尚未到R2，因此dispatch记录0，不能解释之前Charge失败。215帧errors0、最大采集间隔0.495199秒；overview和末帧亲阅，实际末段空手，离线编码0。正常stop/launcher0、无强杀、game消失、子进程0；4999前台及869窗口/cloak观察无违规。

同候选仅安排一次新独立实例bow-1110-195545的完整动作复查，动作范围及所有门槛未变。复查结果另记；不得把准备/编译/清理作为完整玩法通过。全弓/头显FALSE、工程25%保持。证据development-review-r57.json。

复查52684实际取得Bow并到R2。frame479/seq3521原始sync96659608467500，前样本96659498150900，相差110.3166ms；boundary decoded0/reason4，dispatch prepare_attempts0、press_attempts0。确定本次在100ms断流解码门槛拒绝，未进入原版调用；不能用此解释57388的89.9235ms旧失败。198帧errors0、最大采集间隔0.360699秒；全跨度overview和末帧亲阅，末帧实际持弓但未蓄力，编码0。正常stop/launcher0、game消失、子进程0、无强杀；2582前台及807窗口/cloak观察无违规。

## 61. 解码与模型之间的有界回执（R58，2026-11-10）

为旧合法gap失败补上decoded_count、既有model.observe/trigger返回值及原stamp.monotonic_ns，读取纯const can_request_fire结果；原Engine调用、请求、库存、时间门槛和输入生产者不变。原driver严格W4/WX/UNDEBUG/O2回归执行0，PG编译0。PG55ce7847758d32eeb001679577633bee3b5128298363b104fb273200c0f278bd；Native413b68c9、XR16840b27保持。候选torque-bow-prepared-20261110-200147-combined，prepared SHA f69bb982455f86dbb70d95207f7a7f0eef686ce726310fbc6dc3b4539b11e6dd，50个实际编译common头相符。

新实例bow-1110-200211先因两个实例占资源而按10秒间隔等待，资源释放后正常获准启动，没有停止其他会话或绕过上限。完整动作结果与媒体审阅另记，不根据诊断编译提升完成度。

实际game39312取得Bow并完整绑定，初始箭Charge再次失败。frame416/seq3565原始sync97072424471100，前样本97072268013000，差156.4581ms；decoded0、decoded_count0、model_edge_ns0、prepare_attempts0/press_attempts0，确定本次同样是原始输入间隔超过100ms，未进入模型边沿或原版调用。不是R3/装箭/射箭通过，未改变门槛；57388旧合法gap失败原因仍未证明。下一步定位源采样/GT处理/资源负载导致的长间隔，不安排相同条件原样重跑。

202张GPU源捕获errors0、最大采集间隔0.386663秒；全跨度overview和末帧00201亲阅，真实持弓未Charge，离线编码0。4341前台、824窗口/cloak观察无违规。stop/launcher0，精确game消失，子进程0、无强杀；Native cleanup及entry restore verified=true、XR exit0。必须单独保留unified_weapons_cleanup.drained=false、fire_disable_code2、hand_stop_code5，不得把进程正常退出或session.passed=true写成全部交互排空通过。全弓/头显FALSE、工程25%保持，development-review-r58.json保留完整记录。


按最新3个确切Bow game身份/48小时保留规则，保留39312/52684/55144；仅删除4437个本枪直接生成媒体文件490744222字节，errors0，未递归删目录，不动其他枪/源码/资产/文字/数值。R56媒体已审阅后移除，可用性已标注。所有本方运行、编码、媒体保留任务terminal；目标继续active，无新原样运行。

同期只读音频核实精确39312/134387858091517871：target_sessions_read1、target_sessions_muted1、target_shared_rejected0、read_failed0、mutations0。all_shared_rejected11是全部扫描累计，不代表本枪被拒绝；不根据旧muted写入计数0断言未静音。下一次正常新实例由isolated_sim.environment自动采用新增音频回读证据，不为此重启。

## 62. 合法输入到实际发射源拒绝（R59，2026-11-10）

上一轮是进展：R57/58确定其两次断流拒绝，但57388的合法gap失败尚无具体门槛。本轮在原Bow pre/post入口及现有observe/state/snapshot调用旁添加有界QPC计时，不新增Engine调用或输入/姿势写入。PG fbe327a1fb5880c2c4cde2cbf95f7e6d887d06841eba1a25175e5b512eabd077编译0，Native413b68c9/XR16840b27保持，50个实际common编译头相符。

独立bow-1110-201306/game56072的R2实际gap89.206ms，decoded3/count1、model_can_fire1、model_edge8(request_ready)。prepare_attempts1/gate5，press_attempts0：模型已请求，但torquebow_fire::prepare失败，不能归因于解码/模型拒绝或100ms超时。first failure仍为初始箭Charge未到达，后续原装箭/真实UP/R3未通过。不得把本轮门槛定位强行套给57388历史运行。

28条有界计时：pre/post各14条，phase总时长中位15.547/15.632ms；内嵌state调用中位10.808/9.978ms，snapshot0.599/0.545ms。R2当帧prepare分支末source age50.586ms。它们是稀疏GT墙钟观测，包含调度，不是FPS或CPU-only因果。r59-timing-analysis.json保存原行和配对。198帧errors0、最大捕获间隔0.576846秒，overview及末图00197亲阅、实际持弓未Charge，编码0。

正常stop/launcher0、game消失、子进程0、无强杀；3286前台/805窗口cloak观察无违规。Native/entry恢复verified、XR0，统一drained=false保留。全弓/头显FALSE、25%不变，development-review-r59.json。

## 63. 拒绝定位至原版碰撞查询（R60，2026-11-10）

把既有fire prepare短路依次分段记录，不改原调用顺序、次数或返回条件；PG bf648844d498889d26981e74287ad51f4aea59e526a60b2f1b62b3804bf7bfbd编译0，Native/XR保持。新bow-1110-202053/game29672启动前active_instances为空，同完整动作到初始箭R2：解码3/count1、模型允许并request_ready，prepare_attempts1/press0，fire_prepare_gate7、muzzle_prepare_stage8。已通过弹体类型两次读取、muzzle来源及当前抓握快照，具体拒绝在torquebow_muzzle_clear::query；尚不能称FastTrace已经返回遮挡，需区分ABI/源复核和原版布尔结果。

215帧errors0，最大捕获间隔0.887601秒；overview和末帧00214亲阅、实际持弓未Charge，编码0。正常stop/launcher0、game消失、子进程0、无强杀；2849前台/898窗口cloak观察无违规。Native/entry verified、XR0、统一drained=false保留。全弓/头显FALSE、25%不变，development-review-r60.json。下一轮仅记录已有FastTrace是否执行、原布尔结果与同一射线起止，不跳过碰撞或更改射击姿态掩盖失败。

## 64. 碰撞查询观察与失败前提分离（R61，2026-11-10）

在既有torquebow_muzzle_clear::query内部添加只读stage、原call.run成功返回标志、经过闭合schema/源复核后的原布尔结果及射线两端。既有Engine调用顺序/次数、Ray、碰撞和返回条件不变，未把查询失败伪装成通行。PG862e16043addf9146dcf6c45fcad7cb895e3e9b43773d1cc8dab4bff4e124020编译0；Native413b68c9/XR16840b27保持，冻结torque-bow-prepared-20261110-202725-combined，prepared SHA a7349d792202b3a327ae98dfa6c53c9d23e34c3cad975513852ebd07ec4d6f35，50个actual common头相符。

第一次bow-1110-202829/game57636到R2，但gap133.842ms，decoded0/count0、prepare_attempts0，因此没有实际碰撞查询证据。238帧errors0、最大间隔0.623632秒；overview及末图00237亲阅，实际持弓未Charge、编码0。正常stop/launcher0、精确game消失、子进程0、无强杀，3026前台/952窗口cloak观察无违规。Native/entry verified、XR0，统一drained=false保留；音频summary initially_muted1、verified_muted_observations280、verify_failed0、target_shared_rejected0。

同候选仅一次资源条件对照bow-1110-203151：等待时不占名额、不持运行期共享锁、不停止其他实例；正常admission时active_instances为空、available_commit10541924352、physical8170311680。没有保证随后独占机器。实际game49668拿到Bow，但8秒driver准备未绑定：configure_gate2、geometry_known0/feed_serial0、hidden arrow generation1/lit0。未到R2，不能拿它解释R60碰撞查询内部失败，不能延长输入/准备门槛提高通过率。

289帧errors0、最大捕获间隔1.045740秒，overview及末图00288亲阅，实际持弓但driver未绑定，编码0。正常stop/launcher0、精确game消失、子进程0、无强杀；6761前台/1148窗口cloak观察无违规。该次Native/entry verified、XR0，fire_disable0/hand_stop5/drained=true；不得追改前几次drained=false。全部本方运行/编码已terminal。

R61/R61B两次滚动合计删除821个已亲阅的本枪生成媒体105672462字节，errors0；保留最新49668/57636/29672及48小时规则。没有递归删目录或动其他枪/源码/资产/文字/数值，旧review媒体可用性已注明。

本轮进展是R59/R60把合法R2失败定位至原版碰撞查询；R61两次未取得其内部执行/返回值，原因分别为前置断流、geometry未准备。无新的产品因果修复声明，全弓/头显FALSE、25%保持。下一步以有界正常GT只读准备观察取得确切FastTrace ABI/执行/返回证据，避免把诊断是否可得完全绑在偶发合法R2窗口上；保持原版碰撞、身份、100ms、真实UP、anti-replay和完整动作范围，不按同条件原样循环。

## 65. 确认并修正原FastTrace五参数声明（R62，2026-11-10）

复用已有原包导出，不重复不支持Engine.u的REA打开操作；亲自重新核对J:/gearsVr/analysis/packages/decompressed/Engine.u SHA95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510，UELib objects.tsv SHA cf331d8b9142183f6420d5d68ebfe55082f5d33340209645feebd06d50b7d719。Engine.Actor.FastTrace实际导出TraceEnd、TraceStart、BoxExtent、bTraceBullet、ReturnValue五项；旧Bow四项/bTraceComplex声明明确不匹配。该静态证据只认证导出名称/类型，不假装认证游戏内偏移。

仅复制现有已细化的ABI读取模式到本枪独立muzzle_schema，未编辑其来源文件或其他枪。实时仍须Engine.Actor/接收者祖先、原可执行native548、三个Core.Vector、两个Bool、明确独立存储、参数/结构范围和准确返回偏移，前后完整一致；参数offset与Bool mask只用当前反射值。BoxExtent为零保留原版线查询，bTraceBullet真；真实blocked返回继续拒绝，不改Ray起点、Pawn/枪口、ProjectileFire或SafeSpawnLoc。

43个严格W4/WX/UNDEBUG/O2 owned-memory/原call桩用例执行0，覆盖偏移置换、旧参数名拒绝、缺项、类型/掩码/flags、重叠、循环、执行函数/参数/返回偏移变化、源变化和真实blocked桩。另把修前原Bow reader和修后reader放到同一个五参数fixture：旧reader在schema gate4拒绝且原call0次；修后执行一条clear线查询。不是实际游戏physics/Charge证明。最初Python/cmd嵌套引号错误保留，直接命令编译执行0，源码/EXE摘要和准确范围见r62-before-after-success.json。

PG a692aa035da45be41363927d6e473471ef128ba270c72c221b4bb97a62e44905编译0，Native413b68c9/XR16840b27保持。3520冻结输入、50 actual common头一致，候选torque-bow-prepared-20261110-204522-combined，prepared SHA570e833c92942108b1b3bb49c47d9deb2a2889a07ae8d74c95bbd27ccd54f811。

实际bow-1110-204545/game43720启动时active_instances为空，但R2 gap103.3781ms，decoded0/count0、prepare/press0，未执行修后碰撞查询。不能说游戏已证实该修复。238帧errors0、最大采集间隔1.330929秒；全跨度overview及末图00237亲阅，overview有持弓，末图是释放/清理后的空手，未Charge，编码0。正常stop/launcher0、game消失、子进程0、无强杀、worker_error null；4061前台/1040窗口cloak观察无违规。Native/entry verified、XR0，fire_disable0/hand_stop5/drained=true。完整弓/头显FALSE、25%不变；development-review-r62.json、r62-fasttrace-source-evidence.json。

## 66. 空闲读取开销与动作核对分开（R63，2026-11-10）

既有每次driver入口执行两次完整Backend.observe，每次内部又是两个read_once和fingerprint闭合。在严格无债务/ready、R2前后均released、两手控制计数/按钮未变、同来源cohort、左Grip未按下时，取消额外的第二次observe；保留第一次完整typed两读/fingerprint、actor/库存/源检查，Decoder照常核对原时间戳、姿态及100ms gap。没有新增source/cache、输入writer、Engine call或写库存。

新R2、真实UP、R3（含GT间完整tap）、R2仍按住、左Grip、来源cohort变化、原输入/弹体/装箭/模型请求/清理债务均保留原第二次完整observe及后续原prepare/commit。当前空闲样本不发射；若其第一次观察后库存变化，下一fresh意图仍重新核对，拒绝原调用。不是扩大action的许可或时效。

新增严格夹具覆盖：空闲一份完整snapshot；新R2/跳过GT的短tap第二读失败无press/release；真实UP/R3/held第二读失败仅非射击cancel；leftGrip和cohort保留第二读；第一读失败拒绝；库存随后变化不能授下一次发射；100000001ns仍decoded invalid。既有fire driver、load/fire/reload/R3组合、original raw-feed行程三组严格回归均执行0。均为注入backend/合成输入，不是玩法通过。

PG8da9f103b3a2fde2200376daefbb3d648161146c662bae5dcc3c93a4016452f7编译0；Native/XR保持，3520冻结输入/50 actual common头一致。候选torque-bow-prepared-20261110-205505-combined，prepared SHA052d4f02304b8b720b97c95d438c520e7057f515e2fce0159ca8b2c10abdbc55。新实例bow-1110-205534按既有资源规则等待后运行完整原动作；实际结论另记，不用编译/快检提高完成度。证据development-review-r63-offline.json。


R63实际bow-1110-205534/game59000：normal admission前active_instances为空。实际部分无动作样本state_calls由2至1，例如frame400/401 pre墙钟10.128/9.801ms；并非全部阶段减少，也不是匹配A/B FPS证明。R2 frame403/seq4276原sync100302376749000、前样本100302243172400，gap133.5766ms，decoded0/count0、prepare/press0，修后的FastTrace仍未被本次动作调用。完整Charge/箭/扣1/装箭/R3尚未通过。

连续稀疏帧墙钟中位：Bow pre17.054ms、pre/post之间14.905ms、Bow post17.737ms、post结束至下次pre开始54.668ms，入口间104.480ms。样本含R2按下后保持完整核对的行，不能用其中位否认空闲单读，也不能把约55ms外部间隔指认给某个线程/函数、FPS或CPU。r63-timing-analysis.json保留原行与连续帧分段。

247张GPU原projection捕获errors0，最大间隔0.730583秒；overview及末图00246亲阅，实际持弓未Charge，编码0。正常stop/launcher0、精确game消失、子进程0、无强杀、worker_error null；4889前台/968窗口cloak观察无违规。Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true；目标静音回读summary逐实例保存，不能以muted写入计数0否认原已静音。

媒体保留最新59000/43720/49668和48小时规则，删除457个已亲阅的本枪直接生成媒体59304577字节、errors0，不递归删目录、不动其他枪/源码/资产/文字/数值。所有本方运行、编译、编码、保留任务terminal；publicv71和Native保持，工程25%、全弓/头显FALSE，目标active。

## 67. 模组外间隔的有界静态核对（R64，仅分析，2026-11-10）

没有为了约55ms墙钟间隔修改游戏焦点、后台/CLOAK、物理输入隔离、Sleep或新hook。源码内未找到对应直接50ms等待，不据此判定原版没有等待。只复制自己R43的既有IDB到background-wait-r64，不碰共享数据库。

IDA MCP打开因缺ida_nexus模块使其自有idat exit1；未安装/升级插件或改全局配置。复用既有已授权本地无窗口headless脚本，自己的pid54780 exit0。验证原.text代码SHA911ab9bc和EXEbdc6705d后，仅查mov ecx,50、其后32字节内indirect call与PE regular import的Sleep/SleepEx映射；6个常量候选、有效映射命中0。该结果不是REA Evidence，也不是没有后台节流的结论：延迟导入、动态解析、包装函数、其他wait、渲染或调度均未排除。sleep50-dossier.json及r64-static-wait-review.json保存边界，原始快照未改。

下一步继续定位实际超100ms源间隔和模组外墙钟；不重跑同条件R2来碰运气，不放宽100ms/真实UP/anti-replay，不把五参数源码修复及空闲优化当实际蓄力或整枪通过。


## 68. R65 原流水线分段观察与 R66 单查询身份发现优化（2026-11-10）

R65只在R63冻结私有副本加密既有16条计时采样，没有改共享生产入口，也不作为正常交付候选。bow-1110-212704/game46532首次R2原样本gap110.4669ms，decoded0/count0、prepare/press0，未进入Charge；FastTrace修复仍未实际执行。正常stop/launcher0，精确game消失，子进程0、无强杀；13549前台/952窗口CLOAK采样无违规。232张实际GPU projection及overview、末图00231亲阅：等待卡后有实际庭院双目和持弓，无蓄力/箭矢通过；encode0。完整动作、Snap/Volume、GrabPose仍未验收。

同frame397：原CPP总44.7494ms，Bow pre8.4664ms/post9.2686ms，各一次state读；equipment其他19.4883ms、原PlayerTick1.8279ms，CPP结束至下一Bow pre50.9308ms。既有stage显示starter5.7702ms、follow13.4681ms，其中包含Bow post，不能将全部follow归给视觉或其他枪。保留r65-pipeline-analysis.json；约51ms外部间隔尚不能指认Sleep、GPU或某个线程。静态三个真实mov ecx,50候选的导入指针仍未运行核定，未补丁游戏等待。

R66仅在自己的weapon_torque_bow_state.inl及weapon_torque_bow_observe_fields.inl使用既有Reader的reuse_initial_identities：GetStateName之后单次read_once的初始字段发现启用；timers、inventory、完整first-match/retained_metadata/actor闭合之前关闭；finish另强制关闭。数值每次新读，无跨查询/帧缓存；两个read_once与fingerprint、原prepare/commit/身份/100ms/真实UP不变。Native和共享Reader没有修改。

新discovery_scope严格夹具通过：相同值下读取6976→2878；发现后数值变化仍新读，incarnation/class/属性offset/shadow/mask/unreadable拒绝且无输出，闭合强制fresh。该58.74%样例读取减少不是游戏FPS。已有state_fields及observe_fields严格编译执行均0。初次旧测试强制include使用相对路径而找不到文件，修为绝对路径后通过，原失败日志保留。冻结候选从精确R63源仅覆盖上述两个自有文件，不混入当前共享/其他枪修改，也不带R65计时覆盖。PG编译0，SHA7948071b13a27f8d31b370ced60563791a7120095eae28db535c58070a4a4455。实际新动作结果另记，工程仍25%，全弓/头显FALSE。


R66候选torque-bow-prepared-20261110-222410-combined，prepared SHA0078330c23b0ac4146817bba90646bc2770cdf0df1dfbfb28b8518f4e4a36f73；3520源/224driver/50 actual common一致。资源等待后按max2正常准入，bow-1110-222548/game53688，未要求独占。R2 frame483/seq3903原样本gap89.8153ms，decoded3/count1，prepare/fire_prepare_gate0，press_reserved1/press_result accepted；实际同一原链已走过五参数FastTrace及原ButtonPress。正clear未单独日志，调用通过为编译源码路径推论，不伪造直接query像素/回执。外部原字段读取在105678492423300ns见bChargingBow=true。

新的失败：按下帧driver215.6565ms、结束source age233.8622ms，stage已经interrupted9；并非成功保持的charging模型。其后真实UP时已无bound，8s未取得original projectile+inventory ACK，不能声称发射/扣弹/完整R3装箭成功；后续观察used=1但没有原返回记账，不能补认shot。按下后的具体失效分支及墙钟归属仍需窄观察，保持100ms/真实UP/anti-replay/严格清理，不以复用历史按下修复。R66稀疏pre/post state中位5.3419/4.98345ms，样本/并行条件不同，仅本轮墙钟，不宣称匹配A/B FPS提升。

233张GPU projection、overview与末图00232亲阅，encode0，errors0，最大捕获间隔0.848667秒。正常stop/launcher0、精确game消失、子进程0、无强杀、worker_error null；3869前台/945窗口CLOAK采样invalid0/违规0。Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true、pad restored；本目标原已静音1会话，283次readback、verify_failed0、target_shared_rejected0，guard cleanup0故障。

同一精确game内一次只读导入指针观察，前后PID/创建时间/镜像一致，原调用字节一致，无写入/暂停线程：0x140a5f470的IAT确实指向KERNEL32.Sleep，另两处指向USER32.GetSystemMetrics，入口16字节匹配。本结论仅核定API指针，不证明Sleep(50)在主循环、该帧或当前线程执行，不修改游戏等待。r66-owned-import-observation.json保留。全弓/头显仍FALSE、工程25%，目标active。


## 69. R67 按下帧窄计时（私有观察，2026-11-10）

针对R66新出现的accepted原按下但215.6565ms驱动结束后中断，冻结R66源仅在私有weapon_torque_bow_input.inl加五段墙钟：prepare、reserve_before_press、原press、read_owned、cancel；原调用参数/顺序/返回分支保持，新增计时不作为模型/输入/寿命时钟。cancel含内部read_owned，两个数值存在嵌套，不能直接相加。复用既有最多20行/phase的torquebow_timing日志，没有新增writer/hook或改全局流水线，也没有将观察覆盖写进当前生产源。

编译0，PG9bf7477884697364d487886e2016ec99cf3814b7d69da5e756bbc44cd0efcfd6；候选torque-bow-prepared-20261110-223301-combined，prepared SHA4809e7a8f8dbbe917c5ce39dc1a75f5192f89fec594933dbb7e96c51bf59b2a5，3520源/224driver/50 actual common一致。manifest明确bow_press_wall_clock_scopes/not_normal_manual_delivery；正常版本仍R66，不将此诊断当手动交付。唯一差异已审阅保存r67-private-scopes.diff。下一次仅原动作首失败定向观察，不放宽100ms和原动作保护。

R66收尾后媒体保留game53688/46532/59000及48h，删除531个本枪已完成旧输出媒体67724080字节、errors0。文本/数值/源/资产/其他枪保持。


R67实际bow-1110-223315/game58292在肩部reach时，私有controller_pose_command原子replace经原50ms precommit重试后WinError5退出；尚未取到Bow/R2，五段按下计时未被行程执行，不能替代R66按下原因。PermissionError不直接证明磁盘满或共享句柄的具体根因，原失败保留，不重放已发布输入。正常stop/launcher0、game消失、子进程0、无强杀、worker_error null；2396前台/745窗口CLOAK采样invalid0/违规0。164张GPU实际projection/overview与末图00163亲阅，encode0/errors0，实际庭院双目和自由手，末图未持弓，不伪称按下/蓄力验证；Native/entry verified、XR0、drained=true、pad restored。

统筹发来证据盘4GiB新启动边界，未回复/通知他人。只清本方28个build_exit0且DLL已保留的binding-build-r目录内196个可重建.obj等中间文件246453569字节，无递归目录删除、无其他源码/候选/资产/日志/tlog变更，计划/结果r67-intermediate-cleanup*.json。观察C盘free后来为6662434816字节，不能将全部增长归因本方清理，其他会话可能同时清理。资源短缺不是blocked。下一步先闭合私有命令precommit争用失败，再恢复R67窄计时；不无变化重跑来碰运气。目标active、工程25%、完整原动作/Snap-Volume/GrabPose/头显未接受。


## 70. R68 私有命令发布争用修复与定向恢复（2026-11-10）

检查当前哈希绑定CLOAK模拟器ReadOwnedPoseCommand：GENERIC_READ|DELETE + FILE_SHARE_READ/WRITE/DELETE + FILE_FLAG_DELETE_ON_CLOSE。关闭前delete-pending可使同名replace返回WinError5。该机制与R67现象相符，但没有直接句柄时序证据证明那次就是这条句柄。无游戏的真实Windows竞争检查复现：120ms持有时旧50ms预算拒绝，新250ms预算125ms后只发布一次；400ms占用仍250ms拒绝；owner变化拒绝，成功replace仅调用一次。证据r68-precommit-check.json，未启动游戏/模拟器。

仅在自己的r68-owned_command_io.py把尚未成功的precommit replace争用预算50→250ms；payload不变，只有明确未提交且临时文件仍在、WinError5/32/33才等，成功/不确定发布不重试，不续XR/game时间戳/输入租约。共享solo原文件仍50ms，其他实例不变。新候选driver-source冻结并记录自有覆盖SHA，独立prepare从此候选精确复制该工具；完整原动作链保持。PG与Native/XR沿用精确R67/R26，不重编。prepared torque-bow-prepared-20261110-224009-combined，SHAf31122979fcc5e325b2cfd313a66270577613e112d7c35428dc4113e324a0a78；50 actual common一致。新实例bow-1110-224014。

第一次start因free4263849984<4294967296磁盘准入而拒绝，started=false，无game/birth/worker/动作，原失败bow-execution-disk-denied-r68.json保留。仅把本方已结束R64分析数据库272237784字节移存J:/gearsVr/analysis/torque-bow-owned-offload-20261110/r64-snapshot.i64，复制SHA102a0215457326094d9b67311da27abafa9738d150a1574a48e4ed88d6d34a5b一致后移除C副本；其他源/数据库/产物未动，位置r68-database-relocation.json。free随后4563804160，变化不全归因本方。

本方runner新增preflight磁盘检查/资源等待；原start和worker强制准入仍在。只对从未启动的prepare重试，不重启任何已运行实例。当前共享isolated_sim/runtime_receipts/background-runtime来源SHA另复制冻结到实例runtime-entry-source，r68-runtime-provenance-check.json核对一致，未改旧候选散列。运行实际结果另记，工程25%/全弓头显FALSE，目标active。


R68实际game55040/XR55464正常启动，disk准入free4629569536。取弓越过R67文件失败，R2 frame586/seq3695 decoded3/count1、prepare/fire_prepare0、press_reserved1/accepted，stage9；原Charge字段曾真，但原弹体+库存ACK仍失败。首次按下driver199.8689ms：prepare12.7579、reserve9.2571、button9.3123、owned_read50.089、cancel113.051；cancel内含owned_read，非独立可相加。不能将200ms归给原ButtonPress；主要取消/确认耗时。立即ack是否把尚未原tick生效的Charge当失败仍为待证假设，不改为假成功或延长100ms。

241张GPU projection、overview与末图00240亲阅，实际持弓/双目，不声称shot；encode0、errors0、最大间隔0.74005秒。正常stop/launcher0、精确game消失/子进程0/无强杀/worker_error null，3831前台/947窗口CLOAK采样invalid0违规0；Native/entry verified、XR0、drained=true、pad restored。目标静音回读摘要与命令precommit争用事件存development-review-r68.json。原动作、Snap/Volume、GrabPose仍未完整接受，工程25%、目标active。

R69仅准备对同一现有read_owned的前16个返回结果记录ready/charging/pending_end/r2_held/animation/pending_fire/firing/库存/游戏tick和原失败gate，不新增状态读取或原调用，不变ack判定。用于直接区分原按下后暂未Charge、key已撤回、metadata读取失败及其他路径，避免按墙钟猜修。当前源生产保持R66；R69为R67冻结副本内一个自有文件的私有覆盖，不引入当前共享或其他枪WIP，非手动交付。

R69 PG编译0，SHAd858dbd2c3c91a1beb2588522927a56b615f7812f167ed368ceda7af3821f43f；相对R67冻结源仅weapon_torque_bow_input.inl一个私有观察差异，r69-owned-snapshot.diff已审阅。候选torque-bow-prepared-20261110-225015-combined，prepared SHA195adf989faf38f147eee94fd04bd4e46d08e7b3cf907b2ec76cbcb3d0f2cc61；3520源/224driver/50 actual common一致。R68私有precommit修复保留，shared baseline不动。新实例bow-1110-225019按max2资源释放后准入，实际game56236/XR58072；结果待收尾，不提前声称通过。


R69实际未到Bow/R2：启动交接Unified equipment connection rejected:2（invalid）。Native终态只读数值register_attempts1/accepted0/rejected1/faults0/cookie0/callbacks0，父注册未接受，不能归给后续子attach或武器动作，也不能把源码pair的50公共头一致当本次实际注册成功。具体schema/模块pin分支尚未知。

stop/launcher均1，game确实消失、子进程0、无强杀、worker_error null；Native/entry verified和XR0、pad restored保持独立。PG hand stop rejected:1、没有unified_weapons_cleanup成功回执，不能称整条lifecycle通过。2954前台/592窗口CLOAK采样invalid0/违规0。终态失败后才执行专用close_failed_startup_r69：显式要求这次expected failure与Native/entry/XR/game终态证明，不放宽普通成功close门槛。143张GPU projection亲阅overview和末图00142：等待卡后真实庭院双目，没有Bow动作；encode0、errors0、最大间隔0.668471秒。development-review-r69.json保留完整失败。

R70下一步只在冻结私有weapon_hand_client.inl围绕原call_register记录前后Registration/Result字段、三回调VirtualQuery模块/保护属性、返回reply/outcome和clientphase；不改Native、不改注册检查/回调/任何武器/当前共享client，不跳过或重试invalid。保留R69已有Bow确认快照观察，以便父注册真通过后继续原动作。无变化重跑R69并不能识别失败分支，故不采用。


## 71. R70 直接确认前态与 R72 有界原始开始回执修复（2026-11-10）

R70候选torque-bow-prepared-20261110-230132-combined，prepared SHAe78bd4588a887fe362d5a539e010de5ddb3ad4c8c5bce290ab51646c76e47b2f，PG558b88d530124e3a54c404196b5044aa1a97e6a34af8b382674c54dfffb5771a，50 actual common一致。初prepare因free3495243776<4GiB被拒绝，无game；资源恢复后新bow-1110-230339/game57668正常准入。

父注册本次前后观测：Registration size56/ABI4/frame1416/proposal136/decision128/reserved0，三回调同PG映像、MEM_COMMIT/MEM_IMAGE/PAGE_EXECUTE_READ；call_register completed1/reply0/outcome0，phase active2、cookie1。本次没复现R69 invalid2，不能称R69原因已证明/修复。保留独立失败，未改Native或注册门槛。

首次R2 frame517/seq3863，原按下accepted。之后六次既有owned读取known1/state_gate0/snapshot_gate0，原R2_held1而charging0、ready0、pending_end0、pending_fire0、firing0、animation0，loaded1/reserve5、同game_tick30850605；时间108012677314400至108012714916400。随后旧驱动stage9，未得原shot+库存ACK。真实状态不是坏metadata读取，也不是原ButtonPress被拒；旧ack把成功按下后尚待正常原tick进入Charge的过渡态当fault，立即走非射击cancel。记录development-review-r70.json及原行，原外部bChargingBow稍后为真保持独立。

R72仅修改当前本方weapon_torque_bow_fire_driver.h。accepted begin +确切R2 held/尚未Charge/无pending或动画/未record spawn/loaded1且reserve不变时，保留已journal的同一dispatched request，等待后续正常GT；不得再次press或先确认Charge。等待只用第一份已完成两次fresh typed/fingerprint的owned快照，不做第三份非动作重复读取；真正positive acknowledge仍保留原read/account/read。只有原pose_current和既有request.valid_until有效才继续等待，持续fresh输入不能让仍未Charge的旧请求无限延期；无新timer/时间戳/pose writer/permission。来源失效、读取失败、未知原call及其他异常仍原非射击cleanup。等待中fresh R3直接让该pending begin走cleanup，不继续等Charge，不以普通release恢复；真实UP仍原buffered-release时效及strict prepare/commit。

新增deferred_begin夹具：同packet pre/post不重复press，后续原tick进入Charge后真实UP只一次shot/扣1；完整短tap的UP等待Charge且只一次release；pending R3非射击取消、必须真实UP后再新press；60ms+60ms连续fresh输入仍不能续旧未Charge请求；100000001ns断流、veto/hold/space改变、stop、unknown/read failure和pending/animation/key状态异常保持拒绝或保留清理责任。旧驱动在相同新增场景的待确认断言失败（expected negative exit3221226505，不是游戏失败）；修复后严格编译执行0。新增测试自带Windows no-UI abort保护。原driver、reload composition、完整raw feed、idle safety四组严格回归也0。

R71初稿在第三次owned读后才等待，编译0但未prepare/run；按R70每次约7ms观测将非动作等待前移到第一份完整owned快照，保留positive检查，最终R72五组重新检查0。r71初检查rawfeed误用不存在文件名，属于检查脚本命名，保留missing记录；实际weapon_torque_bow_feed_driver_test.cpp已正确执行0。26份校验源码/依赖另冻结，现有header闭包相对R70仅本方driver一个差异，未混入其他枪WIP。

R72编译0，PG3bbecf9caa4d60d74f11e009f2860289d9e77abed3fb7eb749d5df751b4c7696；候选torque-bow-prepared-20261110-231716-combined，prepared SHA6f93ae26515d4694055bb71394063aa82374b7972968c1eddc702e6bb89b8783，3520源/224driver/50 actual common一致，Native/XR保持。一次prepare磁盘拒绝后未启动，资源恢复新bow-1110-231946准备完毕，按既有准入等待实际运行。私有R67/69/70观察仍随此候选，非正常手动交付；完整原动作/Snap-Volume/GrabPose/物理头显仍未接受，工程25%。特别是等待begin期间的真实游戏R3/未来状态清理尚未录到，离线取消夹具不能替代此证明。

R70收尾stop/launcher0、精确game消失/子进程0/无强杀/worker_error null；5065前台/899窗口CLOAK样本invalid0违规0；Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true、pad restored。237张GPU projection、overview与末图00236亲阅，实际持弓双目，无shot接受；encode0/errors0、最大间隔0.736081秒。源码保存、编译、离线行为、游戏、图像与头显证据继续分开。


R72实际bow-1110-231946/game5492/XR48704，父注册reply/outcome0。R2 frame491/seq4597原gap88.9355ms、press accepted；phase0本次保持stage4(charge_begin)，未立即fault/cancel，driver60.1075ms、source_age_end80.2142ms、prepare15.3219/reserve10.8857/button9.3136/owned_read8.978ms、cancel0。这是真实执行了新等待分支，不是已确认Charge/shot。before-original严格prepare累计2次仍通过。

同帧phase1起点source age94.9727ms，Backend.observe gate2状态读失败，state_ns6.4957ms、snapshot_ns0，随后原非射击cleanup119.1336ms、phase1末source_age235.2484ms，最终stage1 ready。失败read_gate被cleanup后续读覆盖为0，不能宣称已经直接抓到gate14或唯一原因。时效边缘是实测线索，保持100ms不放宽。外部bChargingBow随后为真，但R2普通release/原projectile+inventory ACK仍失败，未加载下一箭/未执行本轮R3完整链。

正常stop/launcher0、精确game消失、子进程0、无强杀、worker_error null；5027前台/1142窗口CLOAK样本invalid0/违规0；Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true、pad restored。266张GPU projection、overview与末图00265亲阅：实际双目持弓，无完整shot证明；encode0/errors0、最大间隔1.136371秒，不用捕获间隔声称FPS。development-review-r72.json保存原行及静音回读，工程25%/全弓头显FALSE。

R73仅在冻结私有input文件对press之后最多8次现有state.read失败，在cleanup之前保存state_gate/failed_field/pending容量及原source/当前时间；不增加状态读、原调用、writer或权限，不改R72机制。编译0，PG35f5e9c4c7bc22d464f16895f4eff0661310e25e32b5e455c9683bf617ddf6c6；候选torque-bow-prepared-20261110-232853-combined，prepared SHA08216eced64d138fdb53720d45f0763c98afb61909a5d194c32b6fcab797ddf5，3520源/224driver/50 actual common一致。新bow-1110-232857准备好按资源准入，实际结果另记，不无变化重复R72来碰运气。


R73实际bow-1110-232857/game57372/XR41928：R2 frame500/seq4153 press accepted、stage4，原wait修复仍执行。第一个post state失败在cleanup之前抓到：observed_ns109517635563100、source_sync_ns109517534685100，source age100.878ms，observe_gate2/state_gate14、failed_field空、pending_count/capacity4/4、snapshot/game_clock_gate0。gate14为两次完整typed/fingerprint读完后的allowed来源复核；该时点原样本确实超过100ms，绝非把cleanup覆盖后的gate0当成功。保持限制，后续目标是缩短同一查询内重复的身份/类/字段发现成本，最终fresh闭合及所有positive action检查仍保留；不靠放宽TTL、续时间戳或合成release解决。原projectile+库存ACK未过，下一load/R3仍未现场完成。

正常stop/launcher0、精确game消失、子进程0、无强杀、worker_error null；5399前台/924窗口CLOAK样本invalid0/违规0；Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true、pad restored。247张实际GPU projection、overview及末图00246亲阅，等待卡后双目持弓，未证明完整shot；encode0、errors0、最大捕获间隔0.347583秒。development-review-r73.json保存first-failure原行/状态/静音回读。当前所有本方运行/编译/编码terminal，公共v71/Native/其他武器未改，当前源码R66状态优化+R72 Bow driver修复，私有最新R73观察未作为正常手动交付。工程25%/全弓/Snap-Volume/GrabPose/头显未接受，目标active。


## 72. R74 单次对象关系查询减少重复发现，R75 既有读取分段（2026-11-11）

R74仅当前本方weapon_torque_bow_state.inl的actors：局部Reader初始身份发现复用，结束前关闭复用，原完整retained_metadata新读保留；同时把选中的4个关系字段布局加入fresh闭合，并最后重新读取Controller.Pawn/Pawn.WorldInfo/Pawn.Weapon/Weapon.Instigator四值。无跨actors/read_once/frame缓存，无Engine调用/新writer，不改原R2/UP/100ms/prepare/commit/库存/挂接。

实际原actors函数与修改函数、精确冻结retained_metadata函数在owned-memory夹具对照，读取3056→2071（约32%）；额外闭合已算在内，不是FPS。四条关系在发现后改值、身份class/registry_index/outer、选中属性offset/type、对象表、不可读变化拒绝；跨新查询值不缓存，初始错误shadow/Bow类/GT/读取拒绝。测试只插入after-discovery突变点，无游戏/模拟器；先用不存在object_index字段导致夹具C2039，改取真实registry_index字节后严格编译执行0，产品未因此改。又用精确R73冻结依赖编译执行0，retained_metadata源码匹配，避免当前共享peer变化混进证明。r74-actor-discovery.diff和development-review-r74-offline.json保存。

PG编译0，SHA81d7ac11c40e23cac95a28269ad6aa2354cbc3b308579c9ee0114bca422e9363；候选torque-bow-prepared-20261110-234345-combined，prepared SHAfd63ef383efaba3326a9be06211f582ef428d9aad05c1a6c448c49cacdb67975，3520源/224driver/50 actual common一致，Native/XR保持。bow-1110-234350/game54624/XR58724：父注册0，R2 frame460/seq3624 press accepted/stage4，phase0 driver52.9681ms/source age78.6413ms/state9.4095ms，prepare13.7827/reserve10.2259/button8.5479/owned7.6992ms，cancel0。不是配对FPS/CPU证明。

同帧phase1 begin时原source age96.1069ms，state5.3374ms后原闭合source gate14失败；原行observed_ns110424972800600/sync110424868802400，age103.9982ms，phase1 cancel118.9104ms。无原projectile+库存ACK，R74尚未解决时限。首个差异不是layout/关系读失败，而仍在完整两读之后的allowed gate14；不得放宽100ms来改善通过率。

正常stop/launcher0、精确game消失、子进程0、无强杀、worker_error null；4694前台/886窗口CLOAK采样invalid0/违规0；Native/entry verified、XR0、fire_disable0/hand_stop5/drained=true、pad restored。225张实际GPU projection、overview与末图00224亲阅，实际双目持弓，无完整shot证明；encode0/errors0、最大间隔0.706551秒。原动作证据/静音回读保留development-review-r74.json，工程25%、全弓/头显FALSE。

R75只在R74冻结私有state/input副本添加既有read_once的阶段时间边界，第一次既有prepare只arm12次：actors初检、GetStateName、主要flag/InvManager/pending、资源/模拟、末尾timer/inventory/selector/metadata/actors。没有增加一次读取/Engine调用/钩子/动作，不改计时或门槛，不拿clock代替输入时间戳。两私有差异r75-read-scope-observer.diff已核，当前生产源仍R74+R72driver。PG编译0，SHA971cafea5bf107b7354d22f047eeeca34cdffee1d4ed050bc7e0032776a12fdd；候选torque-bow-prepared-20261110-235749-combined，prepared SHAb315987bbfdc7ad913133f7eee716e6c89570dff21f01fa1150c7b778fe8bc79，50 actual common一致。新bow-1110-235753按现有max2等待资源，不占用/停止别人、不设blocked、不回复统筹；运行结果另记。完整原动作→Snap/Volume→GrabPose目标不变。

R74媒体保留最新game54624/57372/5492和48h，删239个本枪已亲阅旧媒体32605596字节、errors0，源码/数值/资产/其他枪保持。R75本方exec session84734再次确认存活，仍在max2准入资源等待，尚无bow-execution game回执；这不是运行完成/blocked，不因为一次观察yield或跨日期重启。下一继续同一handle取得分段结果，再按实际cost修复；当前字段/关系发现优化没有被称为整枪或头显通过。


2026-11-11 R75继续同一exec session84734，write_stdin再次返回running；仍max2准入等待，最后核对没有本方game回执，不重启/另开运行器，不设blocked。准备analyze_read_scopes_r75.py仅等本次终态原行，AST有效，不生成合成成绩；特别区分read_once内wall scopes与外层source检查/新增日志flush开销，剩余时间不能全归因source或CPU。

本方证据卷短暂再次低于4GiB新启动门槛。只对本枪已完成且散列绑定的冻结源码快照做NTFS透明无损压缩：先R74试验2143文件全部内容SHA/mtime不变、exit0；后R32至R36共5个归档逐文件前后校核一致，各compact exit0，不删除/移动/改内容，不压缩当前工作区源、运行二进制或其他枪归档。r75-source-compression-{pilot,plan,result}.json及每归档mtime/hash表保存；C free增长含同时其他活动，不将全部空间恢复归本方。本方源路径与既有候选散列保持，目标active/工程25%/全弓头显FALSE。


## 73. R75实际准备余量拒绝与R76六关系查询优化（2026-11-11）

R75 bow-1110-235753/game54660 creation134388026362494394已终态，operator1保留原initial projectile+inventory ACK失败，stop/launcher0、owned game消失、children0/remaining空、无强杀/worker error。Native/entry恢复、XR0、fire_disable0/hand_stop5/drained、pad恢复；90383前台/890窗口CLOAK样本invalid0/违规0，长资源等待包含在前台采样总数。261次目标禁音回读、verify_failed0。226张实际GPU projection、overview及末帧00225亲阅；等待卡后双目持弓，不证明shot/手型。encode0/errors空，最大捕获间隔0.5006449秒。

本次12次read_once分段全部完成、state_gate0，无first_state_failure/gate14行。单次actors初检中位0.5966ms、GetStateName0.06675ms、primary/pending0.2511ms、资源0.00215ms、closing0.9111ms。均为有界wall-clock、含观察扰动，不含外层current/Anchor开销，不当CPU/FPS或因果归属。frame480/seq3682原press接受；首owned snapshot在原source age83.6804ms完整完成，r2held1/charging0/ready0、loaded1/reserve5，R72等待分支仍在100ms内。紧接before_original_tick的第二次prepare在原20ms余量检查gate7拒绝，随后取消；不能说首owned snapshot本身已过期。总driver199.4094ms含cancel123.9416ms，prepare25.9059/reserve8.95/button9.5541/owned_read24.9166ms为嵌套/聚合，禁止相加当排他时间。完整发射链仍失败。

R76仅Bow cleanup Anchor::links的每次局部Reader发现复用，完整fresh retained_metadata保留，并额外关闭前重新核对选中六字段布局和值。原Anchor::current两次links/前后registration/context/own检查全部保留；无跨query/frame缓存、无新Engine调用/输入writer、未改100ms或20ms余量、R2/UP/防重放/库存/挂接。直接抽取原/新实际函数和冻结retained_metadata的owned-memory夹具读取3706→2405；六条晚变关系、class/index/outer/layout offset/type/registry/不可读及初始shadow/wrongBow、新查询新值拒绝均严格编译执行0。只是离线读数，不是运行修复。

完整PG编译链接0/SHA878b026fdc3d90a8efad62f2b4f7403e2387652e4e0ea965cc905ec32bc35a88，精确R75冻结基底仅加本方Anchor文件，Native/XR不重建。r76-cleanup-links.diff、development-review-r76-offline.json保存。R75媒体按原最新3/48h政策保留54660/54624/57372，删除268个本枪终态旧媒体32664910字节/errors0，源/文字/数值/资产/其他枪未删。完整原逻辑→Snap/Volume→GrabPose目标active，25%/全弓/头显FALSE。下一只验证这处新查询优化，不重跑未改候选。


R76实际bow-1111-005544/game41928 creation134388037931105597/XR52952，候选torque-bow-prepared-20261111-005532-combined，prepared SHAd7e9b3c0beb5028dbe37d8d533b51de03db5fb74f7722914900f6621de06f536。50 actual common一致；原R2 frame485/seq4113 accepted，首owned snapshot source age76.0492ms，phase0结束77.5655ms、stage4，第二次prepare gate0/无cancel；本次R75的gate7未重现，不据此宣称因果闭合。phase1 begin age89.037ms，第二次完整observe最终allowed state_gate14在103.6303ms拒绝，driver loss reason3；仍没有原projectile+库存ACK。phase0 driver52.1407/state10.2144/prepare14.0893/reserve8.7579/button7.9612/owned7.6401ms，phase1 cancel122.1081ms，嵌套wall不当CPU/FPS。

operator1保留首失败、stop/launcher0，gamegone/children0/remaining空/无强杀/worker null；Native/entry恢复、XR0、fire_disable0/hand_stop5/drained/pad恢复。16099前台/961窗口CLOAK检查invalid0/违规0；246张GPU projection、完整overview及末图00245亲看，encode0/errors空/最大间隔0.4436453秒，未验手型/shot。目标active、25%/全弓头显FALSE；本方runtime/build/encode均terminal。后续不能削掉两次原状态观察、改输入源时间或放宽100ms/20ms；继续只减少单次查询中的重复发现。

本次source压缩工具正常终态：104个本枪完成源归档root（含冻结候选tools/native-source）逐文件SHA与mtime不变、compact全部0。仅NTFS无损压缩、无删除/移动，当前产品源/运行二进制/其他枪未改；r76-source-compression-result.json及每root验证表保留。全卷空闲包含他人活动，不将净变化归本方。


## 74. 拒绝低收益字段合并，R78外层读取分段已编译（2026-11-11）

R77只作owned-memory prototype：将同对象关系字段一次扫描，保留六字段/值fresh闭合，实际R76/New Anchor函数2405→2300读（4.366%）、已有六关系/身份/布局/初始shadow拒绝夹具0。收益不足以引入新scanner，原型保存r77-rejected-prototype及development-review-r77-rejected.json，无生产构建/游戏。当前normal源精确还原R74 state（SHA97c3273a…9d59）与R76 Anchor（SHA9d55182f…9c15）；私有R75观察未带回normal，R72 driver保持。

R78精确R76冻结基底仅在私有state/input增加既有read/read_owned的前16次外层开头allowed/Anchor、第一/第二read_once、末尾fingerprint+allowed/Anchor时间边界；原短路顺序/两读/最终校验均保留，无新增一次游戏读/Engine call或动作，不修改源时间。原12次read_once分段保留。r78-outer-read-observer.diff及development-review-r78-build.json已核；完整PG编译链接0，SHAc6d620793cb13c134822516414c18b24afc4450844dd7d4f526e7d0e6e15f60e。候选torque-bow-prepared-20261111-011350-combined，prepared SHA3c959f32f37f345998cc38230c22d1a410e6cc04cfcffbc4ae8447b300757e2e，3520源/224driver/50 actual common一致；Native/XR不变。

prepare_when_disk_ready同一exec session8983仍待原4GiB磁盘准入，没有新实例/游戏/XR；不能把此等待写成实跑或blocked。r78-source-build-and-disk-wait.json保存确切handle与下一动作，不重开waiter/第二runner。旧completed R69..76对象中间文件只盘点56项70774158B并保存计划，尚未执行删除，所有源/成品/媒体/DB与其他枪保持。R76媒体保留41928/54660/54624最新3/48h，已删249个本枪旧媒体32005164B/errors0。工程25%/全弓/头显FALSE、目标active；下一取得外层实际耗时后再决定窄修复，禁止放宽100ms/20ms或删观察来通过。


## 75. R78外层读取实测与R79 FastTrace发现复用（2026-11-11）

同一prepare exec8983正常0，创建bow-1111-011913；唯一runner exec18385资源准入后实际game19816 creation134388057249361671/XR20908。原4GiB门槛未改；证据盘经主人授权的统筹清理后实际free约139GB，非本方源压缩贡献；旧终态shader缓存被授权清理不当产品损坏或重跑理由，无主动回复统筹/其他枪修改。

R78取得16个完整外层分段：普通read(kind0)5样本opening allowed中位0.6778ms、closing fingerprint+allowed0.7184ms，第一/第二read_once1.953/1.9128ms；owned read(kind1)11样本opening Anchor2.1412ms、closing fingerprint+Anchor1.9825ms，第一/第二read_once1.8444/1.763ms。是有界wall与观察扰动，不当CPU/GPU/FPS或Native内部具体函数耗时；仍保留每次两读/完整闭合。

实际R2 frame663/seq3483 accepted，phase0 stage4/prepare gate0/end source age74.7501ms、driver55.6913ms/prepare14.722/reserve9.3575/button8.3142/owned8.671ms；phase1 begin age87.2255ms，第二observe state_gate14在102.4314ms拒绝，原projectile+库存ACK仍失败。operator1保留，stop/launcher0、gamegone/children0/remaining空/无强杀/worker null，Native/entry恢复、XR0、fire_disable0/hand_stop5/drained/pad恢复。8502前台/951窗口CLOAK采样invalid0/违规0。226帧GPU投影、完整overview及末图00225亲阅，encode0/errors空；最大捕获间隔3.7164327秒原样保留，不以无错误冒称连续帧率。静音目标回读记录development-review-r78.json。

R79只在本方weapon_torque_bow_muzzle_clear.inl首个FastTrace查询的初始发现打开局部Reader identity reuse，在第一次完整fresh retained_metadata之前关闭；原调用后始终fresh，无跨调用/query/frame缓存。原五参数/zero extent/bTraceBullet/Pawn位置→实际枪口/遮挡拒绝/调用前后Native current及完整metadata/layout/位置闭合不变。完整实际query owned-memory与原函数对照7302→5509读（约24.6%），既有43case加5个在初次发现与首次闭合之间的class/selectedProperty offset/type/outer/registry突变，原/新两版48case均严格编译执行0，所有新变化均在原调用前拒绝；根目录永久测试也48case0。初始fixture误从生产冻结树找晚增test文件、缺global null logger/read-observer宏的编译错误保留，修夹具未放宽产品。

R79精确R78冻结基底仅变本方muzzle_clear，完整PG编译链接0，SHAa1fb4802a1beb05ba3542f5a1325fb024e4a33822449936a35d9414499f82853；Native/XR不变。原型/48case依赖出处、r79-muzzle-discovery.diff及development-review-r79-offline.json保存。源码/编译是进展，不是实际发射闭合；完整原逻辑→Snap/Volume→GrabPose目标active、工程25%/整弓头显FALSE。下一精确R79新候选针对性检查，不放宽100ms/20ms/真实UP/反重放或删除状态观察。


R79已封装torque-bow-prepared-20261111-013905-combined，prepared SHA13a8df74249ab8b47c43980cf8442e9b73afe2724b16028937ebd94bf771503a、3520源/224driver/50 actual common一致。prepare exec27812正常0（AST既有invalid escape警告原样保留，不影响身份/原时间），实例bow-1111-013933；唯一runner exec18406再次write_stdin确认running，目前仍max2资源准入，尚无本方新game回执。不是旧磁盘等待，也不是已实跑/blocked，不重开或抢占其他实例。

本方只读核对原UELib导出并逐文件匹配manifest SHA，六函数保存在r79-original-r2-source：PreButtonPress的R2分支设置bFire=1和bHoldingButton_R2=true，PostButtonPress R2分支为空，Charge.BeginState调用ToggleCharging等。导出仍有未解native条件、签名缺损，不能从源码推定实际分支。当前cancel off_field(true)明确处理bHoldingButton_R2而未采样bFire；这只是待核疑点，不当实际晚发射/未清理故障，也没有直接增加输入字段写入或改取消配方。

read_owned_r2_input_fields_r79.py绑定精确bow-1111-013933，独立只读exec24522再次确认running，等该实例game回执后才OpenProcess查询/读取并校核EXE SHA、PID/creation/image、对象注册身份、Bow→Pawn→Controller关系、三个typed字段和前后布局/值。一次按下前基线、原press既有日志出现后最多24样本，未知/身份变化拒绝；无游戏写入/Engine调用/暂停线程。观察时钟未独立校准GT且有外部观察开销，不能作动作许可或CPU/FPS证据。等待最长1800秒，未捕获单记，不起替代游戏。r79-verified-resource-wait.json保存两个具体handle与后续收尾/审片动作；完整目标active、25%/整弓头显FALSE，Publicv71/Native/其他武器不改。


## 76. R79预热未绑定，R80原箭资产发现优化已编译（2026-11-11）

R79唯一runner exec18406与observer24522均终态：实际bow-1111-013933/game37704 creation134388065559737596/XR47132，operator1保留full Bow driver not bound after warm/preparation（8s原限）首次失败，未触及R2/FastTrace新路径，不当R79新查询已执行。已有log configure_gate2、geometry_known0、arrow_generation0，appearance_gate6；另原始准备在source age100.3068ms/gate9拒绝。gate6含多项最终校验，具体子原因未区分，不能直接把所有预热失败归因计时或R79优化。

stop/launcher0、gamegone/children0/remaining空、无强杀/worker null，Native/entry恢复、XR0、fire_disable0/hand_stop5/drained/pad恢复。19075前台/1057窗口CLOAK采样invalid0/违规0，301次目标禁音回读verify_failed0。267帧GPU projection、overview及末图00266亲阅，encode0/errors空/最大间隔0.6701543秒，未验发射/手型。development-review-r79.json保存。最新3/48h媒体保留37704/19816/41928，删除228个本枪已终态旧媒体31959125B/errors0，源/数值/资产未删。

R79只读input observer取得0样本/AssertionError，旧捕获未保留堆栈行号，原失败不改写。静态原Engine objects.tsv第1512对象明确ByteProperty'Engine.Controller.bFire'，而旧脚本错误要求PlayerController，属确定认证错误；尚不能反推旧异常是否就在该断言，也无bFire残留/晚发射结论。R80只读脚本修正为Controller+Engine包认证，记录实际字段类型/宽度/owner/registry header、阶段和traceback；仍精确实例/进程/creation/image/EXE hash、原Bow→Pawn→Controller与前后字段布局、最多1baseline+24按下后样本。没有任何新的游戏输入字段写入或取消配方更改。

R80仅本方arrow_source和arrow_attributes两处本地Reader初始发现打开identity reuse，二者既有closing前关闭、完整fresh identity/layout/link/value/scale/socket复核全部保留，跨query/frame无复用。原/新实际函数owned-memory对照source2574→1486读、包含source前后括号的attributes12522→7932读，原asset/local scale/light/socket/mirrored/duplicate/absolute/avatar/socket/read-race检查保持通过；新增6晚source改变、10晚属性/mesh/socket/layout改变原/新均拒绝，新query的正尺度新值仍读取到。冻结R79依赖原/新执行0，根永久测试严格编译执行0；不是游戏耗时/FPS/资产像素或手型证明。

完整PG编译链接0，SHAa0c3fa3d8bc76da3d772d62bc8c014844319a10b3191455f51e040153b3e56c5，精确R79冻结基底只叠这两个文件，Native/XR不变。候选torque-bow-prepared-20261111-020041-combined，prepared SHAbf0135f136060acc96110e4f3db02e601c7288f607de9969d168333768047cfc，3520源/224driver/50 actual common一致；prepare0创建bow-1111-020145。唯一runner exec41247与只读observer19367再次write_stdin确认running，目前max2资源准入、无新game回执，不重开runner/抢占他人或设blocked。r80-verified-resource-wait.json保留两个handle与原table散列/行；下一同一运行实测预热/R2/原箭ACK后正常清理/审片。完整目标active、25%/整弓头显FALSE；Publicv71/共享Native/其他枪不改。


## 77. R80仍未预热绑定，R82既有步骤门槛观察（2026-11-11）

同一R80 runner41247/只读19367均终态：bow-1111-020145/game51180 creation134388077035603364/XR53884，operator1首次full Bow driver not bound after warm/preparation（原8s）失败，未R2/新FastTrace路径。appearance gate0和original preparation ready1各有实际成功帧，但arrow_generation0/geometry_known0未形成完整warm；另gate6仍合多个final checks，不能从分项通过或age100ms旁证指定唯一失败原因。原两行查询优化的离线结果保持，不能称已修游戏预热。

正常stop/launcher0、gamegone/children0/remaining空/无强杀/worker null；Native/entry恢复、XR0、fire_disable0/hand_stop5/drained/pad恢复。14817前台/1494窗口CLOAK采样invalid0/违规0，412目标禁音回读/verify_failed0。382 GPU帧、102.9111593秒录制、overview与末帧00381亲阅，encode0/errors空/最大间隔0.8698684秒；长startup等待卡原样保留，不当帧率或手型通过。只读认证首次成功：Engine.Controller.bFire Byte offset580/width1，WarPC.bHoldingButton_R2 offset1744/width4，WarPawn.bChargingBow offset1192/width4；一组stable baseline为0/false/false，press_seenfalse/errornull，无按下后样本及取消残留结论。原offset是本次反射结果，不作未来硬编码。development-review-r80.json保存。

R81关闭阶段复用想法未实施/未编译，只保存原helper拷贝，实际R80失败改变下一动作，normal/shared关闭逻辑未改。R82精确R80冻结基底只在private appearance/visual/runtime三文件给各自既有前24次warm调用加wall timestamps、已算得stage/result及own state日志；final world scale/material call/material identity/source read/source equality/avatar match/hold/current拆开原短路顺序，只增加标记，不增加一次游戏读/Engine调用，不改判定/100ms/250ms准备节拍/8s操作等待。关键原调用列表/顺序原新逐项一致，r82-warm-observer.diff与r82-observer-call-order-review.json保留。另无外部字段采样器叠加本轮。

首轮私有编译因32bit handle generation日志使用64bit格式，WX拒绝；原失败source/manifest/log保存，只有日志cast修正。重编译链接0，PG SHA537b7601a99842ec414420f175a3b6a0dba1f64d0c7e0bc197cb3c26063669d7，normal源码仍R80，Native/XR不重建不改。候选torque-bow-prepared-20261111-022300-combined，prepared SHAc47f4c00874d402ebd89d9e9116291d2c783e2d4a404a3690008f74b1d5374a8，3520源/224driver/50 actual common一致。prepare0创建bow-1111-022349，唯一runner exec80241再次确认running/max2准入、尚无新game；不重开、不抢占、不设blocked。analyze_warm_scopes_r82.py已准备，仅在本实例正常terminal后读取原log，嵌套时段不相加、不作CPU/GPU/FPS或未采样阶段证明。r82-verified-resource-wait.json保存具体handle与后续审片动作。

R80媒体保留51180/37704/19816最新3/48h，删248个本枪旧媒体32581262B/errors0，源/日志/数值/资产不删。完整原逻辑→Snap/Volume→GrabPose目标active、工程25%/整弓头显FALSE；Publicv71/共享Native/其他枪保持。


## 78. R82首次入口无birth与重试；R83原始请求中性证明补强（2026-11-11）

R82 exec80241终态1，bow-1111-022349 startedfalse，入口start subprocess1。该实例仅config/instance/manifest三文件；无isolated-worker.log/worker receipt/launcher index/done，精确匹配的worker进程查询为空。当前入口源码在Popen之前先xb创建worker.log，故本次未创建worker/game，不是游戏内warm结果/正常游戏收尾通过。环境校核passed、quietBuild匹配、无全局pause；旧stderr未留档，不能将失败定成特定资源竞态。r82-start-no-birth-audit.json保存，旧文件/哈希不改。只修本方E/run_independent_r68.py留存start stdout+stderr/exit至own bow-start-entry.log，同命令/环境/准入/check_returncode/清理不变。

同一未实跑R82候选prepare新唯一bow-1111-023945（不复用失败名字），唯一重试runner exec74262再次write_stdin确认running，目前max2准入，尚无game回执；analyze_warm_scopes_r82.py已改绑此实例、旧首次无birth版本另存。收到共享首次start有界锁外等待修复通知，仅作既有入口后续源码来源记录，不主动回复/转发、不新增入口测试、不改旧哈希；上限2/内存/4GiB/身份保护保持，后续新准备沿当前入口冻结。没有启动第二个runner或干扰他人。

独立安全缺口：原WarPC.PreButtonPress R2设置Engine.Controller.bFire和bHoldingButton_R2，而旧Observed::stopped只见后者；旧off_field(true)及stop_injection reconciliation也未证明前者归零。实际原/新stopped函数owned-memory对照已复现：holding已up且原始bFire1时旧返回stopped，新拒绝。此结论是源不变量缺口，不是从R80按下前baseline推定实际取消残留，也不是warm失败根因。

R83新Bow私有controller_fire辅助只接受原Engine.Controller的ByteProperty/宽1/维1且first-match owner/package正确；状态read_once将实际完整byte加入既有两读fingerprint，并令stopped/ready/输入停止reconcile要求bFire0和holding0。已有唯一取消owner在精确id/serial/Anchor/current及stop_injection阶段内先stop_local，继而用既有compare_word原语清除此byte，最后调用原holding-bit清理；不引入独立writer/hook/thread，不调用普通ButtonRelease/EndFire，不改计时/追踪/姿态/库存/原取消后续顺序。字段偏移反射，不硬编码R80的580。DWORD对齐、derived对象4字节范围、全word比较、邻3字节保持、写前后typed layout/identity/current复核；未知attempt保留原清理责任，不报告成功。

严格owned-memory夹具使用实际helper、原CAS函数和原/新Observed body，4字节位置x5请求值、无归属/写前写后归属丢失、邻字节并发、primitive不可用、写后不可读、属性布局变化、wrong GT/type/owner/width/dimension/对象末尾不足word均通过。另直接抽取实际stop_injection body验证6种入口情况，成功顺序local fence→原请求byte→原holding清理；wrong phase/id、unknown CAS、holding拒绝、邻字节竞争均不报告完成。Current/归属合作者为stub，不当Native新实跑；源码/测试散列和范围development-review-r83-offline.json及r83-owned-fire-stop.diff保存。初始夹具Windows max宏碰撞，NOMINMAX修正仅夹具；没有产品绕过。

R83精确已编译R82冻结基底，仅新controller_fire/state/cancel三文件，state保留既有私有观察边界便于归因；完整PG编译链接0，SHAbfedf98d3f721df1351cfd2389c9a590330667ef53506e3926634076f8f2cc8d，Native/XR及common不变。R83尚未封装/实跑，也没有混入已排队R82。完整原逻辑→Snap/Volume→GrabPose目标active、工程25%/整弓头显FALSE；先取同一R82真实warm门槛，再安排实际取消/释放证据，不用源补强冒称整弓或晚发射修好。r83-source-build-r82-retry-wait.json保存唯一live handle与下一动作。


## 79. R83独立候选封装，R82同一运行器仍准入等待（2026-11-11）

R83此前实际typed helper/CAS/old-new stopped及生产stop_injection抽取体检查保持0，完整PG bfedf98d…cc8d。现封装torque-bow-prepared-20261111-030522-combined，prepared SHAaa978764bde5d9378726042ba5b45c47f44b0329a4b113e9072db7f8619e9cfa，3521源/224driver/50 actual common一致。只写独立prepared-combined-r83.txt，当前prepared-combined-current仍R82的022300候选，未stage/start R83，未混入正在等待的R82。

唯一R82重试runner exec74262再次write_stdin确认running/max2准入，实例bow-1111-023945仍无新game回执。不是停掉/超时后重开，不抢占其他实例，不设blocked。当前外层advisory等待保持原进程；另仅为后续新实例保存run_independent_canonical_r84.py，普通首次start使用既有canonical --wait-resources-seconds600，让原入口在锁外有界等名额/内存且锁内复核；caller原4GiB和显式prefer-idle对比条件保留。没有新增队列/票据/保留名额或弱化上限/内存/身份，捕获原start stdout+stderr/exit，已消费首次请求不重试。该新调用器只AST有效、未启动，不为共享入口修复添加游戏测试；当前74262不变，后续新实例按当前入口源码重新冻结，不改旧哈希。

r83-prepared-r82-verified-wait.json保存两个候选指针、50 common校核、唯一live handle、未启动新调用器源码散列与下一动作。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪未改；R83取消补强尚无游戏内证明，R82 warm具体门槛仍待本次真实记录。


## 80. R82等待器正常结束；R83严格XR启动拒绝；R84仅失败分支诊断（2026-11-11）

R82旧advisory等待器exec74262已按精确PID/creation/console集合发送正常Ctrl+C，KeyboardInterrupt终态1；无worker/start-entry/launcher/game birth，无强杀、不动其他实例。r82-advisory-waiter-normal-interrupt.json保存。确认终态后才选R83。canonical首次入口等待600秒，无caller占锁/保留名额，沿当前入口源码新冻结。

R83 bow-1111-031554确实启动game39040/creation134388118909725706，XR42492/creation134388119001434829。XR在发布任何预检帧之前报Hand continuity candidate bridge creation rejected，exit2；严格source0/panel预检拒绝，Native未激活、Bow操作未进入。stop/launcher均1，gamegone、children0、noforce、worker_errornull；PG仅reservation清理shutdown0/entryrestored，prime取消/entryrestoreverified、watchdog无pending、音频4次mute读回0失败。不记作正常生命周期通过，也不作R83取消行为证据。development-review-r83-startup-failed.json保存原报告与原始身份，主runner未赋值game/XR不补造。

冻结simXR16840b27…a7fc的实际CL command/read记录和52个local依赖已核对，依赖mtime无晚于保存EXE；桥header89cf3eab…e29与来源记录/当前源一致。原open用PID+creation命名、拒绝已存在producer mutex/mapping，并验证进程身份/发布者/QPC/初始锁。历史日志未记失败分支/错误码，具体原因仍未知；没有将拒绝定为重名、内存或Bow故障。

独立r84-xr-startup-diagnostic复制343个原sim-source文件，只改私有桥header失败分支fprintf，记录stage/ambient_win32_error/target/start/producer/packetbytes；去掉日志后与原header逐字一致，成功API序列/失败条件/命名/身份/cleanup均保留，无重试/等待/绕过。ambient码在组合判定中可能陈旧，不当因果证明。严格实际header自进程测试验证invalidtarget、重复producer、已有mutex/mapping拒绝并记录183、关闭后重建、配对consumer及缺失producer拒绝；不操作游戏。原C++20/MT/宏/库保持，privateXR编译链接0 SHA c93c9ad97e7f0fb1aea9273c1b2d00b4543b5b8575b3f99c0bfa569d7f9181dc。共享正式XR/Native/Publicv71不改。

独立候选033209-xr-open-diagnostic保留R83 PG bfedf98d…cc8d/Native413b68c9…68e1，prepared d7c3a22c5b0a65137c1e9ed89db90bceac031f28453aa92a73dcec8855d68b2d。旧prepare工具固定XR校验先拒绝，发生在iso.prepare之前，无instance/game；保留失败记录。新的prepare_independent_r84只选该私有候选exactXR与原loader相同hash，绝不替换固定shared build。新唯一bow-1111-033243完成prepare，canonical runner exec32844已启动，仅作这次具体启动故障的受控复查；全部严格预检/.4/CLOAK/观察/静音/物理输入隔离/内存并发/正常收尾保持。

完整原逻辑→Snap/Volume→GrabPose仍active，工程25%/整弓/头显FALSE。R83源安全补强尚无游戏内证明，R82 warm门槛仍未取得新实际log；不把启动或离线检查当射击/取消/整弓完成。


## 81. R84实际启动恢复；warm/几何通过后原始Charge仍失败（2026-11-11）

同一exec32844 canonical等待后准入（当时只有另一个实例，availablecommit17.8GB/physical12.8GB），bow-1111-033243实际game54140/creation134388131571726871、XR55760/creation134388131645002807。严格预检通过、Native激活；桥本次无失败分支记录，XR0。仅可说R83启动拒绝未重现，历史原因仍未知，不能称修复或将其归因Bow/资源/重名。

实际warm32行：appearance5（4成功/1 phase9 final avatar复核拒绝），visual3按phase6→8→10（结果1→1→2），箭owner generation1/lit1，后续geometry_known1/configure_gate0/driver bound1。phase10结果2是ready，不是失败；首次读摘要将之误解为准备失败，已根据完整enum/驱动结果更正。故本轮不是前两次8秒warm未绑定失败。

原始初始箭前置射击实际R2一次：frame476/sequence3690 preparegate0/muzzlestage8/pressreserved1/pressresult1/stage4；phase0总56.4696ms，sync至结束69.9292ms（prepare13.7004/reserve9.9772/button8.5858/ownedread8.1397ms）。phase1同frame开始距原sync81.8202ms，保留两读/owned观察及取消：4次owned known/state_gate0/snapshot_gate0，均R2held1、charging0、pending0、loaded1/reserve5、同game_tick30082748；未取得Charge。phase1结束原sync age253.4027ms含cancel138.0945ms，不把取消尾耗时算入允许时效。通用boundary reason1不能唯一定位分支；后续核对原输入在PlayerTick中的消费顺序，100ms和20ms门槛不改。operator1真实首错original_initial_arrow_precondition original Charge not reached，无原始投射物/扣弹ACK，未到R3测试。后续frame508 typed state ready/stopped1，不能据此断言新CAS某个分支已实际写入或晚发射根因已修。

stop/launcher0、Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored；gamegone/children0/noforce/worker_errornull。14589foreground/854window与CLOAK，invalid0/violations空；mute250次actual读回0失败并正常恢复。GPU projection213帧56.215秒/maxgap1.015秒，encode0/errors空；全跨度配对overview及末帧亲自检视：前约26秒waiting cards、约30秒起世界画面、末帧Bow，未接受完整手部拟合或发射。development-review-r84.json/r84-warm-analysis.json保存。媒体最新3=54140/51180/37704，48h；仅本枪已结束旧媒体228文件33626704B删除/errors空，所有源/文本/数值/资产保留。

exec32844终态0、无本方live runtime；完整目标active/工程25%/整弓与头显FALSE。保留本次实际前置射击失败，下一步原输入消费/Charge进入顺序，不重复无变化warm或扩大目标外武器。


## 82. R85补齐已有请求字段观察，单一受控复查（2026-11-11）

R84原始输入消费静态证据已按manifest SHA核对并保存r84-original-tick-source：WarPC.ButtonPress→Pawn.HandleButtonPress/WarPC.HandleButtonPress；后者ShouldDo/PreButtonPress/weaponHandle/PostButtonPress；WarPC.PlayerTick调用super(PlayerController).PlayerTick，后者PlayerInput/PlayerMove。导出脚本不直接证明native消费点，未发现可据此断言bFire被重置的证据。历史original-bindings-57200同时有通用Fire(StartFire)和后置Game_Shoot(ButtonPress R2)绑定；历史列表不是当前选择优先级证明，不能据此给Bow添加第二次StartFire。原WarPC.StartFire导出为空也保留其native/反编译局限，不作完整native路径结论。

R83现有typed状态已读取fire_requested，但私有owned_ack_observation只输出holding/Charge，无法区分请求是否被后续Tick清除。R85精确复制已编译R83，只在既有first16 owned日志增加unsigned(actual.fire_requested)；没有额外游戏读/Engine调用/判定/时效/owner/writer，normal根源仍R83。PG编译链接0 SHA4a2e75741bc0775b4f3faa4d534e2c1a5581ae704fb2e95728bc624af69688a2，实际50 common与原Native一致。候选034843-bfire-observation，prepared a3f0458321350603c547eba57c624b54f654e0ac8e529cb6fbcf6abee2a7f2c1；仍用独立failure-only XR诊断，不修改共享Native/XR/Publicv71。

新唯一bow-1111-034858 prepare0；sole canonical runner exec74487开始同一原始初始箭前置动作的针对性复查，原100ms/20ms门槛不改，不把本次当未变化timer循环。r85-build-and-runtime-checkpoint.json记录当前实际执行状态；后续只跟踪同一handle，正常清理并检查新增已读字段，未取得结果前不下原输入消费根因结论。完整目标active、工程25%/整弓与头显FALSE。


## 83. R85本次准备等待失败，新增bFire观察未被执行（2026-11-11）

同一exec74487终态0；实际game11668/creation134388137745109433、XR21072/creation134388137815626134，严格启动通过、Native/PG实际加载。operator1真实首错full Bow driver not bound after warm/preparation（原8s）。没有R2 dispatch或owned_ack行，故不能把本轮说成实际bFire消费观察。后续frame690/692/693/694确有bound1/geometry1/configuregate0/箭generation11/lit1，说明并非从未到ready，但不能拿晚到日志推翻操作期的失败或延长等待伪造通过。

warm72行：appearance24(成功18/finalavatar phase9拒绝5/firstmaterial phase4拒绝1)，visual24(phase6 10/phase8 5/phase3 4/phase10 5)，phase10五条均result2 ready，开始sourceage44.57..77.45ms/结束60.47..93.38ms。嵌套墙钟不作CPU/GPU/FPS归因。箭generation反复至11、绑定曾丢失，下一步核对owned visual retirement/same_parent/appearance equality与同场景Frame撤回；新bFire观察仍未实跑，原Charge失败原因保持未定。不马上重跑相同动作，不改100ms/20ms/8s去过门。

stop/launcher0，Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored、gamegone/children0/noforce/worker_errornull。2965foreground/983window+CLOAK，invalid0/violations空。projection246帧66.913秒/maxgap1.580秒、encode0/errors空；全跨度配对overview和末帧亲自复核，前约30秒waiting cards/约34秒世界/末帧Bow，无发射或完整手拟合通过。音频原actual mute读回及恢复见development-review-r85.json，warm数值r85-warm-analysis.json保存。

当前无本方live runtime/encode/build/observer。R85仅独立诊断候选，正常根源仍R83，Publicv71/共享Native/其他枪不改。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%/整弓/头显FALSE。


## 84. R86仅既有撤回分支观察（2026-11-11）

前一目标回合属于progress：R84实际绑定/接受原R2而Charge未到，R85则原8s操作期未稳定绑定且箭owner generation递增11，改变下一定位动作。当前再次核对工作树/冻结R85：共享client/backend有其他未提交变化，本轮分析对应实际R85 frozen source，不把当前peer源码当已运行来源，不改共享文件/索引。

撤回路径有on_frame parent mismatch、prepare parent/retiring、appearance不等、advance拒绝、withdraw隐藏失败和外部retire。现有日志没有区分，不能凭generation把原因定为lifecycle、外观、时效或隐藏失败。R86精确已编译R85，仅私有visual.inl加前24条既有分支POD日志：reason、已有parent Frame差异mask、owner代数/hold/retiring/lit、池stage/withdrawn/revoke；不增加游戏查询/Engine调用/重试/等待/权限/新writer。去除观察后原函数体逐字相等，r86-observer-structural-review.json与diff保存；正常根源仍R83。

编译链接0，PG459f0dff7a724c46e0b2f24f7d84a90f1f75ebd1cbf34d75d571dc4a4c4d88d5，50 actual common与冻结Native一致。候选035728-visual-retirement，prepared5451700fb3c84b72085c17a060d74c14f843abde6d7ba67e7dfc0c42bd1b4cca，Native不改、独立XR仍failure-only诊断。唯一bow-1111-035732 prepare0，canonical runner exec67482已启动，全部原准入/100ms/20ms/8s/CLOAK/静音/身份/正常收尾保持。r86-build-and-runtime-checkpoint.json保存当前回执与下一动作；不额外启动重复实例，不抢占他人。

完整目标active、工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪不改。待本次真实分支记录后才决定修复，不把观察本身记作功能修好。


## 85. R86真实请求观察与第二次准备时效拒绝；R87原R2绑定查询优化（2026-11-11）

唯一exec67482正常终态0，bow-1111-035732 game38732/creation134388145679114826，XR59204/creation134388145747060967；严格启动通过。原始箭前置射击R2按下一次accepted，frame465/sequence3806首个owned双读known时bFire/fire_requested1、holding1/charging0/pending0/loaded1/reserve5/game_tick28141332，距sync85.8608ms。before_original_tick随后第二次prepare拒绝gate7（既有ready_for20ms剩余预算），在同一phase0调用内取消。后两次owned读仍bFire1/holding1/charging0，108.7632/118.8820ms；这些都是取消前的既有读，不能说已观测原始PlayerTick后bFire消费。phase0 end245.5058ms含cancel136.7191ms，不能把取消尾耗时计入输入允许时效。真实首错原始Charge未到，无投射物/扣弹ACK。

本次warm/几何/绑定实际通过，仅1条visual_retirement reason0出现在收尾，pool ready/未withdrawn，未重现R85 generation11的反复重建。不能因此撤销旧失败或归因为lifecycle/appearance。现有分支观察保留，下一动作以本次实际第二次准备开销为准，不重复无变化warm/timer。

stop/launcher0，Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored、gamegone/children0/noforce/worker_errornull。11639foreground/829window+CLOAK、invalid0/violations空；mute244次actual读回/失败0并恢复。projection210帧54.955秒/maxgap0.504秒、encode0/errors空；全跨度配对overview及末帧亲自检视：28.91秒仍waiting card、31.83秒已有世界、末帧Bow，无发射或完整手拟合验收。development-review-r86.json保存。媒体最新3=38732/11668/54140、48h，仅本枪已结束旧媒体384文件36401946B删除/errors空，源/文本/数值/资产保留。

R87源修改仅weapon_torque_bow_charge_buttons.inl：原始WarPC ButtonPress/Release bind的栈内Reader开启initial identity discovery reuse；在原有retained_metadata全量新读之前关闭，之后原Args再解析仍保持。没有跨函数/跨帧缓存、改变owner/schema/参数/script/exec/fingerprint/收尾复核/原调用/输入/写入/100ms/20ms门槛。先在独立before/after实际bind/identify/original_function/input_property/hash_script/fingerprint/retained_metadata体上测量：1061→850复制读取（减少19.8869%），严格W4/WX/UNDEBUG/O2各23项baseline/15错误布局/6closing前元数据变化/后续查询新值检查通过。最初夹具缺失helper/类型别名等编译错误仅修夹具，不是产品绕过；旧日志保留。最终使用实际binding-source-r86的fixture/HandClient/Reader环境重编译，结果仍各23项与1061/850。Args解析和Native环境是人工协作者；不作为真实耗时/FPS或游戏行为证明。去掉测试mutation钩子后与修改后实际产品bind逐字一致，development-review-r87-offline.json/r87-original-button-bind.diff及原新源码/日志散列保存。

精确已编译R86冻结源仅该Bow文件delta，完整Playground编译链接0，PG8378afc39095a943b430144384815a747602f3d04adf3289246ee0257adb2716；50 actual common与原Native一致。候选041222-button-bind，preparedd8030ff39dfd1685f251b2ac0d6b34bb0d0e7f761eb4519d6fa8b335ba5f7b96。R87只封装未选中/未stage/未启动，current仍R86；正常Bow根源为R83安全修复加本次R87查询优化，不混入私有观察。r87-source-build-checkpoint.json保存。

当前本方runtime/build/encode/observer均无live handle。完整目标active，工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪/正式XR保持。下一步同一路径元数据成本审查与必要真实原Charge复查，不能拿19.9%离线读取缩减宣称蓄力已修。


## 86. R88取消绑定查询复用与R87合并编译/单一复查（2026-11-11）

前一goal turn为progress：R86已有bFire1证据但第二次准备gate7在原Tick之前取消；R87原R2绑定栈内初始身份复用已改/验证23原新项并完整编译。本轮核对当前source与checkpoint后，继续同一准备路径的4种取消方法绑定，未改共享Native/其他枪/索引。

R88仅weapon_torque_bow_cancel_live.inl bind_entry的Reader初始发现打开reuse_initial_identities，在原retained_metadata完整新读之前关闭；原fresh retained与最终identify、script/exec/参数/owner判定以及原取消步骤和bFire安全清理均保留。没有跨方法/跨帧缓存，没有改100ms/20ms/8s或增加writer/Engine调用。实际bind_entry+original_function/input_property/transition_schema/putdown_schema/identify/retained_metadata/fingerprint/hash_script函数体在冻结R86 Reader/HandClient owned-memory夹具中，原新严格W4/WX/UNDEBUG/O2各48项通过（4方法baseline、24布局/模式拒绝、16closing前metadata变化、4后续查询新值检查）。Args解析和native环境是人工协作者，log_file null，无游戏执行。ClearPending/ChargeEnded/GotoState/PutDown复制读取956/662/1269/694→818/593/1044/627，总3581→3082，减少13.9347%；不作游戏耗时/FPS证明。去测试mutation钩子后与实际产品bind逐字一致。development-review-r88-offline.json、r88-cancel-bind.diff及完整原新源/日志散列保存。

精确R87冻结源仅该Bow文件delta，合并R87原R2发现优化；完整Playground编译链接0 PGf3f50dee021c2254cf9e17180eeb4a6660b1c84c22a8861bdccfa4fd5e7c62cf。实际50 common与原Native一致。候选041806-cancel-bind prepared5f4ac06a6b1e93daf0aa5fee2efed75c1f20108bc00434f3993b82423c7c3db2；R87从未单独实跑，不补造其游戏证明。独立Native/XR选择保持，公共/正式XR不改。

唯一bow-1111-041841 prepare0；canonical runner exec98807已开始既有首次600s有界准入，当前尚无game回执，跟踪同一handle，不重复start/占锁/保留名额/干扰他人。r88-build-package-checkpoint.json记录实际状态；后续原Charge/realUP/投射物/扣弹及完整R3/reload路线均仍要真实证明，原保护/清理/审片/媒体保留不变。

完整目标active，工程25%/整弓/头显FALSE。读取计数缩减是源/离线进展，不是蓄力修复完成。


## 87. R88实跑第二次prepare通过，同场景后段第二次observe拒绝（2026-11-11）

sole exec98807 canonical等待后实际game20972/creation134388158746126388、XR56868/creation134388158805501796；严格启动通过。原箭组件generation1/lit1、几何/driver绑定通过，唯一visual_retirement为收尾generic0，旧generation11故障未重现。原始初始箭前置R2一次pressreserved/accepted1，frame452/sequence4325，2次prepare gate0/muzzlestage8。phase0开始age19.5472ms，结束76.4842ms，driver55.5585ms（prepare总14.4794/reserve9.3982/button8.3774/ownedread8.3367）。first owned snapshot age74.9509ms、known1/bFire1/holding1/charging0/loaded1/reserve5/game_tick28840803。不能将相较R86的时间差归为纯优化效果，初始sourceage和运行条件也不同。

同一frame/sequence phase1开始age88.4627ms，2次Backend.observe阶段state总10.239ms/snapshot0.2595ms，boundary reason3=第二次Backend.observe拒绝，具体内部gate未被该通用reason唯一确定；随后fence并进入取消，cleanup前2条owned双读仍bFire1/holding1/charging0及同game_tick/库存。phase1结束244.6893ms含cancel132.9792ms，不把取消尾当允许时效。实际操作首错仍original_initial_arrow_precondition original Charge not reached；无原始投射物/扣弹ACK，未进入R3测试。不能把bFire1这些cleanup前读取说成已证明native fire消费点/原请求被清除，也不能宣布Charge固定。

额外原始脚本WarPawn.StartFire、WarWeapon.Active.BeginFire、Engine.Weapon.StartFire/BeginFire已manifest SHA核对并保存r88-original-begin-source/provenance.json：包含super(Pawn)路径、SetPendingFire、HasAmmo/SendToFiringState等；这是decompile脚本证据，不是完整native消费点证明，不足以删除第二次prepare保护。

stop/launcher0、Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored、gamegone/children0/noforce/worker_errornull。12992foreground/903window+CLOAK、invalid0/violations空，mute267次actual读回/失败0并恢复。projection233帧61.123秒/maxgap0.666秒、encode0/errors空，完整配对overview及末帧亲自检视：32.01秒仍waitcard、35.57秒世界、末帧Bow，无发射或完整手拟合验收。development-review-r88.json与r88-warm-analysis.json保存；媒体最新3=20972/38732/11668，48h，仅本枪已结束旧媒体215文件27136038B删除/errors空，源/文本/数值/资产保留。

下一步审查同场景、同不可变输入、已decode且已accepted pending begin的重复服务，先离线验证是否可只等待而不再授予动作；必须保持一个完整typed双读及current/原100ms和request deadline，新Frame/输入/身份/hold/R3/UP、实际状态推进及positive ACK原额外读检查都不能减少。不是批量删除active second observe，也不是续旧source或新计时。当前尚无该语义修改，不将方案当进展。

本轮实际进展为R88源优化+原新48项/全编译/实际prepare通过后首失败定位；当前无本方live runtime/build/encode/observer。完整目标active，工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪/正式XR保持。


## 88. R89同完整Frame已接受begin只等待；离线通过、实跑未到分支（2026-11-11）

前一goal turn属于progress：R88原新取消绑定48项/完整编译/实跑第二次prepare通过，同场景后段第二次Backend.observe拒绝。当前核对原代码/现有deferred-begin测试后，R89仅在一个完整新typed双读/current observe与guard/account之后，为同完整Frame（含key/now/身份/参考/控制器/立体数据等hand::same_frame）、已decode、accepted begin journal、原Snapshot仍ready0/charging0/holding1/无pending/fire/animation、库存同/游戏tick同、无reload/spawn/cleanup/conflict/左Grip的情况只返回owned等待。没有stamp、decode、新动作、ACK、Engine调用或pose/request时钟续期；所有Frame/输入/所报告原状态/游戏clock推进保留完整路径。第一条fresh完整双读及Native/current guard不是缓存；旧前态只作等值比较。原before_original_tick的第二次prepare仍强制执行。诊断计数饱和而不影响等待，既有8行trace_dispatch增加same_frame_begin_waits及仅计数变化可记录，不成为权限。

独立r89-repeat-begin-prototype保留原/新实际driver与6ms/observe、sourceage88ms注入对照：旧2次observe跨100ms取消，新1次完整observe后只等待；随后实际状态progress仍2次observe和positive ACK额外owned读，真实UP/投射物扣弹流程通过。Frame key/now/packet/R3、8种Snapshot/state/tick/ammo/leftGrip差异、5种时效/veto/busy/current/hold撤回，以及不续旧pose、不续原request deadline、pre-original prepare失败仍取消和超过100ms新输入gap无释放/发射均通过。初始测试fake从charge基类继承pending_fire1导致夹具begin断言，改成与实测/R72一致的完整pending状态；Windows wrapper NOMINMAX仅夹具修正；正向UP曾跨旧100ms gap而被正确拒绝，旧失败日志保留并新增明确gap拒绝用例。不是产品绕过。

新产品tools/vr-common/weapon_torque_bow_repeat_begin_test.cpp直接使用正常driver头编译运行0；新目标测试与原fire/deferred_begin/feed/driver_reload5个prototype suites严格W4/WX/UNDEBUG/O2均0。注入后端/时耗不是游戏证据。development-review-r89-offline.json/r89-repeat-begin.diff保存源和范围。正常变更只有Bow driver+原输入有界日志、新独立回归；不改全局CMake/Native/其他武器。精确冻结R88源合入driver/私有input日志与回归，全Playground编译链接0 PG78876f96a6cb94cdf63cf6906643383c9c42504335bf2cd672f11db7a973c92e；实际50 common与原Native一致。候选044743-same-frame-begin，preparedb16c4e334bc18ef1d33c2e02a403b6fa5e90a7ee93aa155887792914ee873c39。

sole exec90712终态0，bow-1111-044804实际game29000/creation134388173623880100，XR55920/creation134388173696979440，严格启动、箭generation1/lit1/几何和driver绑定通过。实际原始R2 press_attempts0，same_frame_begin_waits0，owned_ack行0。输入到frame459/sequence3954及后续release frame492/sequence4102时decoded0，boundary reason4；相应decode/sourceage23.702/26.7124ms，不能用低当前年龄证明输入连续或判定拒绝错误。原decoder invalid可来自帧配对/姿态/控件有效性、相同或倒退序列/时钟、两次被观察sync间隔超100ms、counter回退。旧generic0无prior sequence/sync，不唯一指明原因。实际首错仍initial original Charge未到，但本轮没有原始按下，不能当R89已实跑、更不能说等待修好或失效。

stop/launcher0，Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored，gamegone/children0/noforce/worker_errornull。3119foreground/912window+CLOAK，invalid0/violations空；mute272次actual读回/失败0并恢复。projection217帧56.859秒/maxgap0.570秒、encode0/errors空，完整配对overview及末帧亲自检视：30.01秒waitcard/32.97秒世界/末帧Bow，无发射或完整手拟合验收。development-review-r89.json/r89-warm-analysis.json保存；媒体最新3=29000/20972/38732，48h，仅本枪已结束旧媒体248文件32687480B删除/errors空，所有源/文本/数值/资产保留。

关闭后证据盘曾余3476979712B、低4GiB原准入门槛，暂不启动新game；随后只读盘点时余10718277632B已外部回升，未为过门伪造容量。两份terminal本枪PSOCache仅做读取盘点（各105files/885049318B），未删除/迁移/建link或触碰原始seed、存档、游戏或其他实例缓存。下一步在lose覆盖前记录原decoder前包序号/sync/cohort与既有POD条件，分清gap/replay/shape而不削弱100ms/反重放；不重复无变化timer。当前无本方live runtime/build/encode/observer。

完整原逻辑→Snap/Volume→GrabPose目标active，工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪/正式XR保持。


## 89. R90解码原返回点拒绝类别/前包回执（2026-11-11）

前一回合progress：R89同完整Frame只等待已改/离线5suite及产品回归/全编译，但game29000未原始按下，decoded0在先，新分支counter0，原因未分清。本轮核对源码后，不猜gap或重放，不改任何拒绝条件。仅Result增加Rejection和调用前known/sequence/sync；分别在原input_shape、non_increasing、observation_gap、counter_regression、ambiguous_transition原返回点赋值。拒绝结果仍重新构造原空动作/count/release false，lost/save调用和先后原样；ambiguous仍保持原save后状态。前包字段在lost/save覆盖之前捕获，previous_known区分陈旧数值；不是输入权限。

r90-decoder-rejection-prototype原新2061个对外结果/后续解码状态比较严格W4/WX/UNDEBUG/O2通过，5种准确拒绝点、fresh cohort、真实press/release/R3/混合2048步覆盖，全部原status/actions/count/release与后续结果一致。去除新增记录可逐字恢复原拒绝分支；完整旧predicate/时限/lose/save不改。正常controls test和R89 repeat_begin regression编译运行0。最初源码byte guard因旧controls LF与测试副本CRLF不同拒绝，text等价核对后安装；该时刻未改产品源，保留此工具过程，不将其当运行通过。development-review-r90-offline.json保存。仅3个Bow源改动：controls被动回执、driver带回执、原32行boundary输出；无新游戏读/Engine调用/计时/源续期/写入/权限。已核冻结Native实际依赖中无controls header。

精确R89冻结基底合入3文件，全Playground编译链接0 PG56958fcf17d09e7001d5aa3e90e21dafe87c9b73126932ae39e775189cf77011；50 actual common与原Native一致。候选051209-decode-rejection，prepared50935475736d931a972e4735ffaec4e71496b9b109422d3b0dd6a449e868fda0。共享birth-stage发布修改通知仅留存：当前新候选和后续新实例额外冻结launch_reserve.py来源，与既有3 entry source并列；不改旧source SHA、不为共享修复另开游戏、不回复或转发，也不声称Win5根因/游戏修复。build-source旧Python快照和执行entry-source分开记录，不冒充相同。

第一次prepare bow-1111-051303在iso.prepare入口require_disk时free3785834496B<4GiB拒绝，只有own candidate-config/manifest两文件，无实例root/worker/launcher/game，r90-prepare-disk-refusal.json保存。只读盘点45个本枪旧冻结source共1110729859B，未迁移/删除；容量随后外部回升至20GB。新唯一bow-1111-051710 prepare0，不复用失败名。当前sole canonical runner exec84617在原600s准入中，尚无game回执，继续同一handle不重复启动。r90-build-runtime-checkpoint.json保存候选/源码/验证/实际状态。

完整目标active，工程25%/整弓/头显FALSE，Publicv71/共享Native/其他枪/正式XR保持。待真实拒绝类别与前包时间后才决定修复，不把观察或离线一致性记作Charge完成。


## 90. R90实际拒绝点与正常关闭（2026-11-11）

bow-1111-051710已完成，sole runner84617终态0；game47736/creation134388194932335855、XR55408/creation134388195016766016。严格启动通过，原始R2实际接受一次，frame407/sequence3511，prepare2/gate0、press_reserved1/result1；仍首错original_initial_arrow_precondition original Charge not reached。无原版Charge、Projectile+扣弹ACK；same_frame_begin_waits0，不称新等待分支已游戏验证。

29条decode_sequence与当前sequence对齐的reason4记录全部decoder_rejection3：同一cohort内相邻两次解码器观察的sync间隔103.1546–157.0859ms；当包年龄15.9445–71.8794ms。证明观察间隔触发原100ms保护，不能证明XR原始追踪连续，也不能直接删掉gap条件。r90-decoder-rejection-analysis.json保留精确原前包回执。

接受R2的phase0结束源年龄77.7134ms，phase1进入95.0757ms。原torquebow_observe_state_failure给出observe_gate2/state_gate14，记录时101.5258ms：两个typed read及fingerprint相等均通过，最后nativeequipment::current(nullptr,source)拒绝，发生在Driver首次observe内、同Frame等待判断之前。该aggregate来源检查内部原因未逐项记录，不单称超时已唯一确证。copy_frame_on_gt源码是memcpy committed，不会刷新Frame.now_ns；先前时间戳刷新猜测撤回。3条owned观察仍为bFire1/held1/Charge0、同gameTick31106010。

stop/launcher0，Nativecleanup/entryrestoretrue、XR0，fire_disable0/hand_stop5/drained/padrestored；gamegone、children0、noforce、worker_errornull。16156foreground/821window+CLOAK、invalid0/violations空；音频守护真实启动ready/exit0/restoredtrue。194帧50.632秒/maxgap0.381秒、encode0/errors空，完整双眼overview及末帧亲自查看：抽样23.95秒仍等待卡、26.87秒已有世界、末帧Bow可见。不是完整手拟合/开火/头显验收。

本枪媒体保留最新3=47736/29000/20972、48小时，实际删除已结束旧媒体212文件26574984B/errors空；源/文本/数值/二进制资产保留。development-review-r90.json、r90-build-runtime-checkpoint.json同步实际终态。未额外启动游戏、改动共享入口/其他枪/正式XR、放宽时限或更新旧输入时间。下一步沿首次observe来源检查与已有raw history证据定位，完整原逻辑→Snap/Volume→GrabPose目标active，工程25%，整弓/头显FALSE。


## 91. 原始历史连续性核对原型（2026-11-11，未接动作）

前回合progress为R90实际终态、state_gate14和双眼图证据落盘。本轮从冻结实际源码核对nativeequipment::current：enabled/current_gt/domain_ready、client.source_current_on_gt精确key、frame_active当前时钟；client内部仍检查原committed来源/epoch/新鲜度及fresh adapter source observation。未删任何来源检查，也不将R90 aggregate失败唯一归成超时。

现有read_input_continuity可请求history，历史请求使用current_tracking_reader精确caller stereo/input和真实producer pid/start双读核对；continuity ring现有MotionHistory最多8原样本，motion_pair要求连续序号、严格sync/predicted、同源/stream/追踪epoch、活动追踪和累积counter合法。SourceTracker在原生产端观察缺序列/源时钟间隔/追踪边界，不能从GT两次读取gap直接反推源丢失。未改ABI/Native/共享读取器。

r91-history-prototype/bow_history_evidence.h新增独立纯核对原型：保存的完整baseline与publisher必须精确出现在当前匹配history内，核对逐帧原序列/epoch/active控件，保留每次R2/R3变化原始sequence/sync和顺序，仅给出变化是否仍在原100ms内。旧边缘140ms不能因当前包新就变新；无新增时戳、插值样本、动作/ACK/decoder baseline或pose/request续期。历史进程认证仍依赖现有reader，原型本身不宣称验证操作系统进程句柄。

严格MSVC C++20/W4/WX/UNDEBUG/O2编译运行0，28个用例包括完整8帧跨GT140ms、原边缘80ms与140ms、100ms及100ms+1、原press/release/R3顺序、publisher pid/start/零身份、缺序列、左右tracking/stream变化、focus/active丢失、reserved/count/未匹配baseline/current、时钟倒退/counter跳变、stale/future current等。实际未改Decoder在同一完整样本夹具仍返回原observation_gap且无动作，原型仅区分完整生产历史与GT观察gap；未以构造stereo绕开产品规则。

源码/编译/运行和依赖SHA见development-review-r91-offline.json。正常产品源/候选未改、未新增游戏。R90并未采到raw历史，29次真实gap仍未证明源连续。下一步在完整owner身份/新鲜来源下设计原始边缘时效和顺序消费，保留无证据拒绝、真实UP、R3优先级/反重放/原request deadline等所有约束；先离线比较再接动作。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%、整弓/头显FALSE。


## 92. 原始历史消费源码与完整PG编译（2026-11-11，未实跑）

前回合progress为R91纯历史证据原型28例，本回合将其推进到正常Bow controls/model/driver/backend。只有在原解码器观察gap>100ms、保存的完整旧输入+真实publisher+stream/左右tracking epoch全部匹配、原ring含每一连续样本、所有相关变化仍在原100ms内时，才使用历史连续性。原stereo/current shape、计数、ambiguity、R3优先级仍经原decoder；无证据/缺帧/换publisher/追踪边界/旧边缘仍原拒绝，不以新时间重种baseline。

原始R2/R3变化顺序与sync带回Driver；模型只接受与其保存pose_ns精确相等的continuous_from_sync_ns，且当前clock_valid与实际追踪仍必须通过。不能仅凭boolean说所有gap连续。TorqueBowStamp可携带原边缘时间，R2检查其非future且年龄<=原上限，queue把valid_until取旧deadline与原edge+100ms较早值；缓冲release保留原stamp，ACK后不得延长。模型对长gap有独立拒绝，最初driver用例因此失败；该失败保留，修复为精确旧pose起点证据，未放宽100ms。

backend使用既有read_input_continuity精确caller原Frame并请求原history，前后各保留nativeequipment::current；读不到证据走原路径。decode clock取历史读取之后的实际时钟；不沿用读前时间掩盖耗时。现有bounded dispatch增加history_verified_decodes字段，无新writer/Engine调用/原始输入替换/共享ABI。

27个历史消费/原截止时间用例通过：真实样本按下、短tap、同窗R3优先、过期edge、100ms/100ms+1、旧pose起点不匹配、stale/future current、缓冲release过期不提交等。Driver合成后端覆盖原按下→释放→单次projectile/deduct、deferred shot ACK、重放不重射、缺证/publisher/追踪丢失拒绝、过期release非开火取消及R3无扣弹取消。旧controls/interaction/fire driver/R89 repeat套件通过；无companion旧新2061结果/后续状态及拒绝回执比较一致。实际根源码两项新增回归直接C++17/W4/WX/UNDEBUG/O2编译运行0。初始fixture别名缺失、lost后恢复baseline断言错误、模型长gap失败和首次C++20 requires编译失败均明记；requires已换C++17 overload，不改项目flags。

精确R90冻结基底只合入5个Bow生产文件（含新history header）完整PG configure/build0；最终SHA和361实际local依赖见binding-build-r92.json/development-review-r92-offline.json。实际Native共用50头SHA全部一致，3个改动Bow header不在Native实际编译依赖中。第一次失败receipt/log保留，最终source hashes重核。正常根源码已安装同3头+新helper及仅backend方法/原bounded字段，未用私有timing整文件覆盖正常input；新2回归源保存。没有修改共享Native/正式XR/其他枪/Git index/Publicv71/已交付包，没有新游戏。

R90的29次真实gap缺raw历史，仍不称连续已证。原按下后first-observe state_gate14为另一个未解问题，本修订不声称修好；新历史路径尚未游戏验证。下一步冻结真实新PG provenance和最新启动入口源码的新私有候选，再按原保护做一次变化后完整链运行。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%、整弓/头显FALSE。


## 93. R92候选冻结与真实资源准入等待（2026-11-11）

前回合progress为正常源码及PG完整编译，本回合冻结torque-bow-prepared-20261111-055737-original-history，preparedfe929c2c1011b6f9b22931f11f9e0b55996d8148e51085e28ccffbb256e07c6b，PG76892da41a68cd415aa4a4c4a1115dc4f4555d8681dcc24a68a03e51bf40dc99；Native413b68c9/private XRc93c9ad9不变。完整compiled source逐项SHA核对、实际50共用头一致、完整原动作链drivers保存。新playground_build_provenance已指实际R92 receipt/PG SHA/361依赖；旧R90内部仍指R83的陈旧provenance不追改，也不沿用到新包。

执行入口额外冻结现有6文件：isolated_sim/runtime_receipts/background-runtime/launch_reserve/Start-DebugGame/launcher；最新预启动no-start正证收尾修正仅随本次新实例采用，不为该共享修正另开游戏，不回填旧实例、不回复/转发。编译源内旧Python快照与真实执行entry-source仍分开保存。独立新名bow-1111-055749 prepare0，全动作scope不变；SyntaxWarning invalid escape\w仅AST读取既有驱动时警告，未改动作脚本。

sole runner87039首次磁盘准入free3965427712B<原4294967296B拒绝，尚无game；未降低门槛。只读盘点旧已结束PG对象17份149846129B不足，未删任何object。随后仅8个本枪旧binding-source副本执行原地NTFS无损压缩，17086文件压缩前后逐项原manifest SHA一致，路径不变、删除/移动0，r92-source-lossless-compression.json保存。磁盘期间由3972374528B回升5818580992B，可能包含外部释放，不将全部变化归因压缩；准入后看到更高余量，真实原4GiB门槛通过。

同一87039随后进入canonical600秒first admission，无重复start。实际子PID45804/creation2026-11-11 06:01:22当前仍live，isolated-start-request绑定本进程；require_memory真实确认本机2个automatic reservation仍被已有实例占用，未干预/停止它们。当前本方无worker/game/XR出生，不能说本轮startup或历史消费已实跑。r92-build-runtime-checkpoint.json保留待续同handle。

等待期间使用实际ControllerEdgeCounter生产算法生成1024条历史，与未改旧Decoder逐原样本结果比较：25原R2边缘的顺序/原始sync一致、240次R3优先取消、768种多转换仍保守ambiguous无动作，严格C++17/W4/WX/UNDEBUG/O2编译运行0。候选源码/制品未因此改动；补充property_test及日志保留。准备了绑定本实例/实际PG provenance的终态分析器，未在非终态执行或制造通过。

下一步继续sole87039到实际准入/终态；若正常运行则核对Charge/历史counter/first-observe、正常清理和双眼媒体，若600秒准入终止则保留原请求及无出生事实，不重复旧名。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%、整弓/头显FALSE。


## 94. R92准入600秒终态与原步行源码核对（2026-11-11）

上一回合progress为新候选/独立实例冻结、磁盘保护与真实live准入等待；本回合继续同87039，不重启。已亲自多次poll87039并核对实际start PID45804/creation06:01:22直到终态。600秒首次准入耗尽，require_memory仍报告已有2个automatic reservation，sole runner/start_entry均exit1。入口stack明确失败在probe require_memory、尚未执行worker commit；selected仅config/instance/manifest/start-request，无worker.json/log、launcher-session、done或cleanup-pending；实际start PID45804已消失、无本方prefix的Python/game/XR。未启动operator/Native/game/XR，没有本轮Charge/历史分支或生命周期通过，也没有本方game需要关闭。

未干预已有实例、未伪造game identity/cleanup通过/isolated-done，未重复start，不复用bow-1111-055749。候选055737-original-history/PG76892da4继续保留；媒体latest3仍47736/29000/20972，无本轮新媒体或新retention。development-review-r92-admission-refusal.json及r92-build-runtime-checkpoint.json已同步终态，不能继续把87039记为live。

等待期间核对原manifest并保存WarPC.PlayerWalking.ProcessMove、Engine.PlayerController.PlayerWalking.PlayerMove及ProcessMove三份实际脚本和SHA至r94-original-walking-consumer-source/与r94-original-walking-consumer-review.json。这些脚本显示移动/旋转/掩体路径，未给出直接bFire→Bow Charge消费；既有PlayerTick调用PlayerInput/PlayerMove仍不足以定位原生消费点。未解析的原生token、空script body不证明没有native行为。不能据此增打一条StartFire、移动原调用点或删second prepare/current。现有R90接受R2后state_gate14仍是实际首错证据，本轮未对它声称因果修复。

下一步在真实名额可用时使用新唯一实例检验已变化R92完整原动作链；目前不重跑未改变的timer、不抢占旧预启动失败保留的reservation、不发协调消息。继续基于保存的原始证据做源工作，完整原逻辑→Snap/Volume→GrabPose目标active，工程25%、整弓/头显FALSE。


## 95. 原历史逐样本R2/R3状态一致性修复（2026-11-11，未实跑）

前回合progress/verified wait到600秒准入真实终态，无game。当前只读require_memory仍为2个automatic reservation，未重复start/准备另一个实例或触碰它们。本回合继续源码，构造7个首尾与shape/motion_history都合法、中间R2/R3状态却不配counter的历史：按下计数未到对应level、释放计数仍held、未记释放的trigger掉落恢复、R3按下/释放错位、未记R3 tap、未记trigger tap。原R92全部history_verified接受，before-run.log保存；不是声称真实producer发过这些数据，也不是R90因果证明。

正常Bow helper现逐原样本重走R2 Schmitt状态与R3实际button转换，要求p/r增量与每个转换精确一致；起点必须使用原Decoder保存的trigger_down_，不能按当前/旧模拟值>.35重新猜测。首值与起点状态不可能匹配时拒绝baseline；中间矛盾返回edge_state证据失败，原decoder仍回到既有observation_gap拒绝/无action。原current、100ms、original-control deadline、真实UP、R3优先级、反重放和旧ambiguity不变，无新clock/pose或原版调用。只2生产头变更：controls/helper；正常consumption夹具显式传起点，新edge_state/hysteresis产品回归保存。

7反例before accepted/after rejected复现通过；实际ControllerEdgeCounter生成1024数字组合仍保持25原R2边缘、240 R3优先取消、768保守ambiguity，另1024模拟阈值历史覆盖.35/.65边界及基线.5之前曾UP/DOWN两种保存滞回状态，全部通过。正常根源码edge_state/hysteresis/consumption27/history_driver/repeat5suite严格C++17/W4/WX/UNDEBUG/O2执行0；无history旧新2061个结果/拒绝回执及后续状态比较保持一致。r95-history-edge-state-prototype及development-review-r95-offline.json保存前后源/差异/日志/SHA。

精确R92冻结基底只合入2头，全PGconfigure/build0，PGee74bd611681fb07c344f370a8e15849b1cba44d1966ee2077a069145eda58f0；实际361local依赖、50 Native共用头一致。Native413b68c9、private XRc93c9ad9/正式XR/共享ABI/其他枪/启动模块/Git index/Publicv71不改。已冻结061832-history-edge-state，preparedad2fff475d40bba569e8a4009002eaacb601bfb0a1784d23297ef41ac1f0db14，当前选择指R95；旧R92全部保留且只新增superseded sidecar，不改旧immutable record/source。入口6源码另冻结，正确PG provenance指R95。

第二次只读名额核对仍满，未为相同已知条件再交600s请求；本回合无owned runtime/build/encode live，无新game/媒体。R95尚未实际游戏消费，完整原版Charge/Projectile+扣弹/手拟合/头显仍未通过，R90首次observe source-current失败仍独立开放。下一步名额真实可用后用新唯一实例跑变化后的完整原链；继续原逻辑→Snap/Volume→GrabPose全目标active，工程25%、整弓/头显FALSE。


## 96. 原生消费点历史检索与只读关联准备（2026-11-11，未采游戏）

前回合progress为R95源码/构建/新候选；本回合先只读require_memory仍2 automatic reservations，无新准入/实例/游戏。没有继续poll已终态87039。复用已有knowledge.sqlite只读索引和原runtime16812代码段，实际section SHA911ab9bc…ae6核对一致；不重新启动IDA/新provider或读取其他活游戏。207份含decimal580历史pseudocode逐项SHA核对，9个_BYTE相关候选；另70份244h/580LL变体核对未给出直接Controller bFire身份。数字偏移在组件/掩码等代码重复，不能据此当原开火消费点；原索引code-only/indirect unresolved限制保留。Controller.bFire offset580依据原R80 exact typed read，不能把类型信息从另一对象或旧地址嫁接。r96-bfire-offset-candidate-inventory.json保存有界候选。

为下一次已实际出生的本方Bow提供关联证据，复用原R80 read-only Resolver/typed身份路径准备read_owned_controller_dispatch_r96.py，不是新运行器/输入/hook。新helper native_dispatch_evidence.py只取原Controller首qword及最多512个原PE32+模块内qword，双读相等、前后module头/对象首指针/真实selected Pawn.Weapon/Instigator/Controller links与身份均需稳定；不调用任何指针、不宣称表长/函数/Tick slot/原子快照。数据可能是相邻无关数据，模块RVA还需独立代码和类型关联。仅有10本地memory夹具通过，未真实采game。

脚本要求exact already-live Bow立即验证后才可打开读句柄；对已知未出生bow-1111-055749的实际调用在OpenProcess/Windows reader导入之前assert拒绝，exit1日志保存，无dispatch receipt。guard失败/部分read/变化/越界均拒绝，不做process替选/挂起/重试到别的实例。观察器clock不作为输入新鲜度或许可。候选R95/正常Bow产品源/Native/正式XR/其他枪/Publicv71保持；未追加游戏或其他聊天消息。development-review-r96-native-consumer.json保存源码SHA/具体限制。

下一步实际获得本方运行容量后，在原R95全链运行内做这个有界只读关联，再核对原生消费点；目前不能增打一条StartFire、移动原R2调用点或省略second prepare/current。R90接受按下后的source-current首错仍开放，原逻辑→Snap/Volume→GrabPose完整目标active，工程25%、整弓/头显FALSE。


R96本轮最终阻塞审计：R94实际600s quota拒绝、R95两次容量满、R96开始/最终容量满，连续三回合同一条件成立；最终另用WindowsControl.identity核对两个existing worker真实PID+creation+image与各自receipt精确一致，非仅凭文件。无owned Bow runtime，87039实际exit1。R95修订/完整build/候选、R96现有证据检索/只读关联准备均已完成；下一步真实玩法和native类型关联必须本方game，继续重复离线检查不能证明。保持原配额/不干预已有会话，按goal协议将目标置blocked等待容量变化，非complete/非用户pause，完整需求保持。证据r96-blocked-audit.json。


## 97. 资源释放后R95实际肩取失败，正常关闭（2026-11-11）

用户授权资源释放通知后，先只读核对release记录与真实capacity0，未回复/转发。新bow-1111-114831 prepare0绑定R95 PGee74bd61/Native413b68c9/privateXRc93c9ad9，旧名未用于新start。初始工具顺序错误：prepare仍running时过早调用旧pointer runner，读到旧055749并覆盖其汇总；独占旧bow-start-entry.log的xb打开在任何start subprocess前拒绝，未启动game。r97-premature-runner-old-pointer-refusal.json保留新失败，并从早已保存development-review-r92-admission-refusal.execution原样恢复旧汇总，未改旧失败绿。修正为run_independent_canonical_r97显式out参数、任何写入前拒绝已有bow-execution；原R84脚本不改。

新显式runner79397终态0，game41836/creation134388426444838643、XR44240/creation134388426514561984；operator1实际首错torque_bow_combined_chain.py:111：original_Bow_shoulder_acquisition后未发现所选weapon的native_equipment held记录。原日志67baseline/59idle/4baseline_waiting/29unavailable/4inventory_unavailable，held0、torquebow_dispatch0。尚未R2/原Charge/历史消费，不能称R95实跑通过或归因其新历史逻辑；肩取根因未证，不即刻重跑。新只读dispatch观察器未启动，因为未到可靠持弓阶段。

stop/launcher0、Nativecleanup/entryrestoretrue、XR0、fire_disable0/hand_stop5/drained/padrestored；gamegone、children0/noforce/worker_errornull。4665foreground/1194window+CLOAK，invalid0/violations空。278帧76.428s/maxgap0.959s、encode0/errors空，完整双眼overview+末帧亲自检视：抽样38.63秒waitcard/42.60秒世界，末帧未正常持Bow。保存development-review-r95.json。当前本方game/runner/encode均终态，不再poll旧句柄。下一步核对原肩取input/cohort与当前准入，不猜新history或新增时限；完整原逻辑→Snap/Volume→GrabPose目标未完成，工程25%/整弓/头显FALSE。


## 98. 肩取阶段原装备输入回执对齐（2026-11-11）

用户授权旧终态实例PSOCache清理通知已记，不回复/转发、不重建旧缓存、不把缺缓存当产品失败。继续同41836终态证据：原Bow shoulder phase153917775918500ns附近[-1,+6]秒的32条native_equipment回执对齐保存r98-shoulder-equipment-input-analysis.json。无装备event/press/target；有baseline/baseline_waiting/cohort_changed，其中frame366 sequence4363前观察sync153918065943500到153918172437500相隔106.494ms。此为原装备输入拒绝/重建基线的实际线索，不是Bow战斗历史消费失败或RGrip边缘/具体epoch/timer原因的唯一证明；原raw ring未录，不能补造。当前不改公共装备输入、不延长按键、不重跑；下一步沿既有equipment input cohort条件核对。没有本方live runtime，R95候选保持，完整目标未完成。


## 99. 私有装备cohort原拒绝分支回执，编译冻结（2026-11-11，未实跑）

前回合progress为实际肩取32行对齐。本轮核对实际R95 frozen weapon_equipment_input.h，两个cohort_changed guard均早于sync gap判断：先bound/Native身份/五对象/XR组/reference/calibration，后已知publisher/stream/righttracking。因此撤回将该106.494ms行当作timeout原因的任何暗示；只能说该行发生cohort拒绝。现有装备输入已有认证ring squeeze edge及producer连续性规则，不直接借Bow战斗history替换或放宽。

仅在私有冻结candidate加入cohort_difference局部POD回执：1unbound、2Native key(原比较排除frame计数)、4五对象、8XR组、16referencegen/orientation、32calibration、64publisher、128stream、256righttracking。只在原失败分支用已有局部数据赋值，现有bounded native_equipment行追加字段；不新增game读取/Engine/clock/事件/重试。第一分支失败不评价后续publisher guard，bitmask只能解释实际到达的原拒绝分支。共享root装备header/backend没有修改、其他枪不改。

旧/新各自原NativeEquipmentInputTest142cases/1202checks严格C++17/W4/WX/UNDEBUG/O2均0；9个分类/组合定向用例0。最初fixture用了不存在的ObjectIdentity.name，修正为改address，产品无该问题。去掉新增POD/helper/赋值/日志可精确恢复两份原R95文件，原predicate与先后不改。r99-equipment-cohort-prototype及development-review-r99-offline.json保存。

精确R95基底只overlay2私有装备文件，全PGconfigure/build0：89833934ffc8e22d3e808164d77788a2df95cd849bc4d062fee1cbe7bf818887，实际361local依赖/50 Native共用头一致。冻结121751-equipment-cohort-receipt，prepared6737805d22689329c29fbaceeead0d608fddaabaaa994664944423f86fe2ada8，正确PG provenance/原完整action链及entry-source保留；Native413b68c9/privateXRc93c9ad9/正式XR/Publicv71不变。尚未新实例或game，当前只读capacity2为两份新现有reservations，未提交又一600s等待、未干预它们。

用户明确删除历史媒体通知已保留，不能为补媒体重开游戏。R95实际review的overview在本次检查时仍存在，故不伪称该文件已删；收到清理通知与各路径当前存在状态分开记，过去亲自审图仍是历史事实，未来若路径缺失不得称仍可复查。文字/JSON/数值证据保留。下一步名额可用后做一次变化后完整原链，依据实际mask决定repair，不把离线分类当装备成功或R95历史路径通过。完整原逻辑→Snap/Volume→GrabPose目标active，工程25%/整弓/头显FALSE。
