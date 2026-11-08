# 兽族步枪开发与验证

用户授权于2026-11-08：开发 Hammerburst，弹匣交互与当前 Lancer 基本一致，编写测试并协调受控模拟器运行。工作目录为 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`。修改前源码保存：`6269cc9`。不把既有 Lancer 未通过项或旧 Hammerburst 构建记录算作本枪验证。

## 护木第二握点：用户明确的行为

1. 左手进入第二握点 Snap Volume，自动对齐其 Pose，视觉手型贴合。
2. 未按 Grip 时，手吸附并随枪运动；此状态没有枪械位置或旋转控制权。
3. 真实 Grip 按下后由既有仲裁器授予 Grab，立即启用第二手对枪械 Transform 的控制。
4. 松开 Grip 后结束该控制；仍在 Snap Volume 内则回到吸附跟随。离开范围退出吸附。

吸附输出只用于已有唯一 C0 手部显示消费者，不能伪造按键、生成行动租约、续写旧输入时钟或作为双手枪械求解的授权。Grab 继续使用当前真实输入、对象身份和既有时效检查。失追、切枪、忙碌和暂停的恢复不能重放旧 Grip。

## 弹匣目标与验证范围

复用当前 Lancer 的弹匣抓取、真实拔出、弃原匣、左肩新匣供弹、持续持握入槽、原匣返回、弹药守恒和零弹药拒绝规则。Hammerburst 原版翻盖、独立弹匣资产、骨架及实际安装方向必须独立核验；不能复制 Lancer 几何常量或套用电锯机构。

完整交付需覆盖：取枪与收枪、单手和双手瞄准/原版开火、上述四态护木交互、弹匣与翻盖完整操作、普通/完美/取消换弹、有限备弹与总零、暂停/失追/切枪退出与正常清理。源码实现、离线测试、产品构建、实际模拟器动作、录像目视核对、手动候选交付和实体头显验收分别记录。

## 测试设计

| 场景 | 必须证明的结果 |
| --- | --- |
| 未按Grip靠近护木 | 手姿吸附；移动左手不改变枪的位置/旋转或枪口射线 |
| 保持吸附并移动右手 | 左手显示随枪；枪仍由原持枪手控制 |
| 吸附中真实按Grip | 同一合法抓取进入Grab，左手位移/旋转参与枪械求解 |
| Grab松Grip且仍在范围内 | 结束第二手控制，回到Snap，无额外输入边沿 |
| Snap离开/失追/切枪 | 撤销对应显示，不能留下旧租约或旧姿态控制 |
| 按住Grip从范围外进入 | 不伪造新按下；遵守原反重放/当前接触仲裁 |
| 食指触发但无Grip | 不取得护木Grab控制权 |
| 原匣拔出/返回 | 真实连续运动且身份一致；返回不补弹、不奖励 |
| 弃原匣/肩取/持握入槽 | 同一次真实抓取可连续插入；L/R及总量守恒 |
| L>0、R=0 | 肩取等量新匣，不制造备弹、奖励或库存修订 |
| L=0、R=0 | 无新匣；每次真实抓取只分发一次原版失败音 |
| 换弹中暂停/失追/收枪 | 无旧边沿重放、重复库存写入或残留组件所有权 |

## 当前证据与下一步

已检查当前源码：Hammerburst 有专用翻盖动画、弹匣流程及手姿模块，但旧 `hammerburst-code-current.json` 明确 recorded=false、delivered=false。当前 `weapon_native_lancer_follow.inl` 的 support_geometry 和双手回退仍限制 bound_is_lancer，现有 Lancer proximity 路径也不能当作本枪四态交互完成证据。

Lancer 会话仍在同一工作目录工作，已联系协调专用文件、共享护木接入点及模拟器窗口；未启动第二运行器、未改公共v71。下一步实现 Hammerburst 独立护木几何及Snap/Grab接入，并完成离线边界测试；实际运行必须在既有窗口交接后进行。

## 第一批实现与离线构建

HB独立护木几何、Snap输出及当前真实support租约下的双手求解已接入既有消费者。几何点来自原版LOD0余部OBJ顶点2719，仍为待实跑核验的拟合候选。新增support测试实际执行通过Snap/Grab/松手/离开及11类拒绝，不是游戏或头显证据。

旧弹匣流程在松手时没有证明实际拔出；新增HB连续拔出见证，必须从座内开始，沿当前开放供弹轴移动，核对物品/武器/世界/空间身份、序列、间隔、速度、侧向和朝向。提前松手走现有取消恢复，不能凭松手开放肩取。新增测试通过连续拔出及10类失效。完整备弹/零备弹/总零供弹规则尚未接齐验证。

独立构建目录analysis/hammerburst-20261108/build-playground使用现有配置，Release完整编译链接通过，DLL SHA256为051f422b40259732d1f5d77469b59724ebeddf9df66ee1dc7275bf7d9cbade8f。首次新增头在运行时匿名命名空间内包含导致编译失败，已保留失败日志；移到playground.cpp全局头部后重编通过。源码/DLL哈希及日志路径见analysis/hammerburst-20261108/build-check.json。公共v71未更新、未stage、未启动本枪游戏测试。

后续入口修正：旧流程在满匣/无备弹检查或原版begin拒绝之前已bind事务，后续idle重试会反复bind失败。现在仅在原版接受且开始进入acquiring之后绑定，拒绝不留下事务状态。补充旋转/平移枪体下的实际拔出轴测试并执行通过，Release再次完整编译链接通过。精确新哈希见analysis/hammerburst-20261108/start-binding-build-check.json；前一批哈希保留为历史构建，不代表新二进制。尚未游戏验证。

零备弹接入调查：HB翻盖的acquire/seek目前依赖实际原版reload租约，而原版begin明确拒绝reserve=0。因此不能仅移除入口条件就声称余弹肩取完成；需增加本枪独立的无补弹视觉翻盖所有权、恢复路径，再接reserve_magazine_reusing_loaded/seat_reused_rounds。已有Snub视觉clip可参考契约，但不得直接把AR_Reload/手枪资产认证用于HB。

## 零备弹分支实现（尚未游戏验证）

新增HB专用视觉clip，明确识别Weap_LocustAssaultRifle/HB_Reload与原版动画序列，核验本枪latch曲线；共享同一动画所有权日志，停止/seek均使用原版typed调用，seek禁止notifies，恢复时通过GetWeaponAnimNodeSeq核对当前真实节点。无补弹时不创建原版reload租约/库存调用/奖励时钟；动画序列号使用已认证原匣真实抓取身份，不充当reload serial。

已接reserve_magazine_reusing_loaded与seat_reused_rounds；肩取前重新核对实际库存，L>0/R0可形成含L发的新匣，总0拒绝新匣并对每次真实抓取去重原版失败音。零serial闭合在账本中直接完成，按实际库存一致性恢复动画/显示，不等待不存在的库存请求。最初供弹测试误以为此处会产生请求而失败，已据实际账本契约修正测试及运行时闭合分支，保留失败事实。

新增供弹测试实际通过7500组有限容量/余弹/备弹守恒与结束组合，以及总0拒绝/真实press去重。独立Release编译链接通过。已冻结准确源码与PG及继承的Native/XR/启动器，路径/哈希见analysis/hammerburst-20261108/inspection-build-check.json。Lancer会话已明确交接运行窗口；尚未stage/启动本枪测试，下一步适配现有受控驱动并实跑、核对录像及正常恢复。无公共交付/游戏/实体头显通过声明。

## 首轮受控运行与驱动修正

已stage冻结hammerburst-prepared-20261108-204040（PG08ec2b5a），复用既有后台/静音/隔离输入、正常退出及配对录像流程。独立HB包装器不修改Lancer驱动，只在内存选择HB专用动作。当前选中本地安装的授权依据是本worktree AGENTS.md的2026-11-05 startup compatibility覆盖，保留其精确EXE身份/hash及原认证方式。

50140在动作驱动import时缺少resolve_camera搜索路径，未到游戏动作；正常退出，原配置/settings/head精确恢复。已补现有工具路径，短录像配对总览亲自查看、编码/保留清理terminal0；证据analysis/hammerburst-20261108/runtime-50140-review.json。

17840实际进入测试场，初始原库存武器为Weap_LocustAssaultRifle，右肩操作产生slot3/target3cbc73400的held回执。随后驱动错误要求“最新输入事件行必须held”，将baseline/idle行当作模型未持枪，导致开火前断言停止。此为测试判据错误，不证明脱枪，尚不能宣称取枪至开火完整通过。已改为匹配本枪实际acquisition回执和当前武器身份，再要求真正原版权威射线作为后续持枪/开火证据。未放宽产品身份、时效或输入保护。

17840完整保存跨度的配对总览已亲自查看，启动第三人称/等待卡仍可见，不作为已修好。两路录像已编码，launcher0、游戏/XR正常结束，原配置/settings/head精确恢复。证据analysis/hammerburst-20261108/runtime-17840-review.json；既有保留清理结果见retention-17840.log。下一步仅复查修正后的本枪实际取枪至开火，再进入护木/换弹。Lancer会话同期只编辑其分支，当前仍使用冻结候选，不混入新源。

## 14688：基础取枪与真实开火限定通过

相同冻结PG08ec2b5a，修正驱动后，实际14688/creation134386158675068471完成右肩取得原库存Weap_LocustAssaultRifle、6发真实射线、暂停/B恢复。6条trace/write来源均一致，起点均与本次枪口完全相同；该次基本链通过，不等于护木、换弹或全部间歇输入问题已通过。

完整保存跨度配对总览1页已亲自查看，录像已编码；启动第三人称/等待卡仍可见，未宣称修复。launcher0、游戏/XR正常退出，config/settings/head原字节精确恢复。证据analysis/hammerburst-20261108/runtime-14688-review.json，既有媒体保留清理日志retention-14688.log。

下一项为本枪护木四态。已基于既有受控手部路径建立独立hammerburst_support_chain.py，读取本枪weapon_hammerburst_foregrip.h，不沿用Lancer的PSVR主握把35度姿态断言；保留实际骨架/枪矩阵、无Grip不控枪、Grab控枪、松手返回、食指不控枪和原版开火判据。独立包装器以GEARSVR_HAMMERBURST_SUPPORT=1选择，语法检查通过，尚未实跑；不因基础6发通过而重复其他Lancer套件。

## 9772护木实际失败与57008目标前失败

9772/creation134386163265461500，原PG08ec2b5a：未按Grip移动左手，实际枪旋转变化约1.75e-5度；真实Grip后移动左手，枪旋转变化也只有9.43e-6度，断言Off hand did not steer actual mesh。只证明无Grip不控枪，不能证明自动贴合、Grab或完整四态。保留全部实际骨架、模型矩阵与输入规划。三页完整配对时间轴已亲自查看并编码，游戏/XR正常退出，config/settings/head原字节恢复，既有清理terminal0。证据analysis/hammerburst-20261108/runtime-9772-review.json。

源码发现HB帧仍先进入Lancer弹匣候选生成器，其专用C0观测不足可能输出无关pending并阻止support。精确HB持枪分派修正提交54029c2/6db24c6；第一次独立编译因早期读取器引用后定义bound_key失败已保留，改用同次已鉴权main_hand_held及精确HB类型后编译链接通过。由54029c2源码归档加6db24c6单文件补丁独立构建PGad22fcde，不含其他会话未提交源码。完整哈希见analysis/hammerburst-20261108/support-route-build-check.json。这是待验证方向，不是已证实根因。

57008/PGad22fcde在护木前的初次R2没有射线，未触达护木修正，不能给修正通过或失败结论。已保存原source_status/时序回执，不扩大原时效或重放按键。配对全跨度1页亲自查看并编码、launcher0、原配置/settings/head精确恢复，证据runtime-57008-review.json。后续护木驱动以匹配实际acquisition、当前同一武器及实际矩阵作为前提，不再强加无关启动点射；护木内的真实控枪/射击断言保持。公共v71未改，完整HB/换弹/头显均未完成。

## 后续离线定位与现有回执补充

9772的before_grip与neutral_left_moved_no_Grip实际左腕位移为4.47219原单位（100单位/米），跟随本次物理手移。故不能把“无Grip枪不动”当成自动Snap成立。用同场记录的实际枪矩阵/右手原始姿态/规划左手，重放原support_contact得到contact=true、distance=1.1162e-8米、mount_valid=true；这是规划几何相符，不能证明当场mount发布、Native授权或最终手姿。重放夹具最初因合成asset身份未对齐被拒绝，修正合成身份后通过，不属于产品失败。证据contact-9772-offline-check.json。

原分派的playground::instance额外依赖使现有离线读取器夹具无法编译。改为同次Reader的精确derives查询（a2a1d6f），现有WeaponHandClientOfflineTest实际45项通过，保留原身份/类型/表及输入检查。已有report_grasp_attempt普通GT回执增加同帧main_held/mount/busy/animation/input/decision字段，上限仍16条，无额外对象读取、callback、collector或控制入口；用于区分后续当前绑定/握点/抓取失效。独立PG完整编译链接通过，SHA3e7ede1c，尚未实跑。准确源哈希见grasp-receipt-build-check.json；当前共享目录其他枪未提交改动没有混入该独立构建。

57008编码/清理均已terminal0后，Longshot明确未就绪让窗，HB已将唯一运行窗口明确交给手雷会话，Snub排在其后；Lancer暂不立即运行。HB当前只离线工作，不stage或启动，后续需重新协调窗口。未标记完整目标完成。

## 分派回归及9772输入规划基准差异

现有production observer的人工内存测试新增3项：确切WarfareGame HB不调用无关Lancer producer、同名外包类型不误识别、source-only观察不生成候选。注入无关Lancer pending后，隔离源码恢复旧无条件分派实际断言失败(exit3221226505)，恢复修正后48项通过。证据dispatch-regression-check.json；这验证分派边界，不证明9772根因。曾尝试启用完整Native-equipment宏扩展夹具，但注册适配器依赖缺失而编译失败，该探索撤回，不能把当时旧exe输出当新检查通过；最终48项版本已重新编译/执行。

更具体的9772记录分析：稳定右手位置对比差3.19e-7原单位，测试读取器m和生产follow target_model basis却差34.99935度，第二握点坐标差22.395cm。64组相同frame/sequence的恢复前delayed LocalToWorld回读与目标basis仍差34.99935~35.69885度，不只是未配对的外部截图；但这些CPU矩阵仍不是每Draw/GPU证明。现有support_contact对旧读取m规划的手柄位置与读取m得到true，对同一物理手与生产model-based mount得到false(distance0.223954m)，大于原14cm。输入按生产model target重新规划后，原函数得到true(distance1.2395e-8m)，没有放宽半径/时效/授权。

因此9772的Grip无转向不能直接作纯产品授权链缺陷结论，也不能宣称Snap成立；还需核对目标模型与实际渲染差异。保留失败，增加可复现的weapon_hammerburst_contact_plan_test.cpp，MSVC C++17/W4/WX编译执行通过上述3个判定。具体数值/记录见model-vs-measured-9772.json、model-delayed-readback-9772.json、contact-plan-regression-check.json。

独立护木驱动现只在输入规划使用同右握持对应的生产target_model，明确标记planning_is_not_outcome_evidence；独立观察的实际枪矩阵、骨架、真实射线及后续像素验收保持，不把目标矩阵换成实际结果。驱动语法与离线接触重放通过，尚未新实跑。原Snap/Grab实现、最终五指/枪体贴合和完整换弹仍未验收。

## 原资产变换核对与下一冻结

只读复核既有WarfareGame.u分析副本中确切HB导出62424426/1238355字节，SHA4b6bd784匹配原记录。原有有界tag解析器从64读到185，4个标签没有RotOrigin；骨架前原生变换区24字节候选可解为Origin(0,0,0)、Rotator(0,-16384,0)。此处是保存布局推断，不是运行时反射值，不能证明运行时35度差异的根因，也没有据此硬加35度补偿。详细字节及界限见hb-origin-properties.json、hb-native-origin-candidate.json。

下一候选hammerburst-prepared-20261108-214027已冻结2216项源码/驱动哈希，PG3e7ede1c，继承原已标识Native/XR/启动器；没有stage或启动。源与二进制来自独立source54029c2加已标识HB分派/回执改动，不混入共享worktree其他武器未提交变更。精确记录support-model-plan-prepared.json。已向Snub申请在其完成既有Frag→Snub队列并正常收尾后交回窗口；当前不占运行器。保持完整护木/换弹/交付目标开放。

## 护木四态检查补强（214651冻结，未运行）

独立驱动新增实际已求值左腕相对实际枪矩阵的比较，不能只凭“枪没有转”判Snap成功：无Grip物理左手移4.47cm时，腕部在枪局部坐标的位移须<1cm、转差<2度；松Grip后也要回到此前被动吸附姿态。数值独立于生产目标/输入规划，非原子CPU回读容差不代替最终像素/五指贴合验收。

离线实际检查4项：9772旧记录被正确拒绝（相对腕位移4.472196原单位）；相同记录通过；枪和已求值骨架共同刚性变换后通过（误差4.64e-13）；仅腕朝向转21.199度正确拒绝。语法检查通过，详细snap-observation-check.json。没有启动新游戏或放宽产品半径/时效/租约。

产品PG3e7ede1c不变，下一冻结更新为hammerburst-prepared-20261108-214651，2217项来源哈希全部核对，support-snap-plan-prepared.json。原214027保持历史原字节。当前Snub已在运行，Longshot先申请其后窗口，Lancer也有窄段申请；HB已同步尊重唯一交接队列，不stage/start争用共享instance。完整护木、换弹和交付目标继续开放。

## 弹匣拔出边界补证（离线）

新增6类实际拒绝断言：错误朝向、同时间、倒退时间、NaN手位、无效轴、无效枪姿态；另增加枪同步平移/复合旋转且弹匣相对手有非零位置/旋转偏移的连续拔出，以及已完成拔出后换item身份必须撤销见证。MSVC C++17 /W4 /WX、保留assert，独立编译并执行exit0，精确source/exe/命令哈希见magazine-expanded-check.json。未修改产品抽取算法，未运行游戏；不把新测试替代原匣/新匣回座、动画恢复和实际手枪面贴合验收。

## 原弹匣检查驱动准备（未实跑）

新增独立hammerburst_reload_chain，复用受控启动/录像/退出，只用连续模拟手柄动作，检查翻盖开启、原弹匣实际拔出后不松手回座、闭合、原计数/奖励不变及权威回射。护木与本段由互斥环境选择，没有沿用旧HB脚本的GiveTo、直接输入文件或stage逻辑。

既有弹药观察器仅接受Lancer，已建立HB精确类型专用入口，只替换类型条件/错误文本，原typed属性/身份/双读/closing检查保持。离线夹具验证HB进入typed读取、Lancer和OtherWeapon拒绝，4文件语法通过；不是实际库存读取或换弹通过。初版驱动不存在ammo键的问题在运行前改成capacity/used/reserve/infinite/bonus明确比较。

最新冻结hammerburst-prepared-20261108-215626，2219项来源哈希全部核对，PG3e7ede1c不变，未stage/运行。精确记录original-return-driver-check.json。首次记录脚本在冻结完成后误把Path当字典导致文档追加失败，冻结/哈希已完成且未启动游戏，现修正文档记录。Snub尚未明确交窗，已请求核对唯一持有人，不抢共享instance；完整目标仍开放。

## 下一运行预检与明确队列

215626的7个HB专用驱动与6类继承构建制品已逐个重新核对哈希及语法，next-run-preflight.json记录一致；不代表新编译或游戏通过。Snub全部清理terminal后已明确交Longshot，Longshot确认将先运行其已冻结基础段，再正常退出/双路复查/清理terminal后唯一交HB。HB已确认该交接条件，未stage/start，也不把暂时无进程当作窗口归还。下一实际动作保持护木四态单点，原弹匣检查随后另段进行。

## 被动随枪运动判据补齐

护木驱动新增右手正常移1cm并转yaw0.04rad、左手保持无Grip/真实输入不动的一段，要求实际枪矩阵确实变化且已求值左腕相对实际枪的位姿保持；随后归还右手姿态并重新读取baseline，再进入Grab。与左手移动不控枪检查分开，避免漏测用户的随枪要求。新220323冻结继承同一PG3e7，全部来源哈希核对、语法通过，passive-follow-driver-check.json；尚未stage/实跑，不能当随枪通过。Longshot本次实际行走受阻后已退出，HB已请求正常录像复查/清理后按约交窗，不抢重查或共享实例。

## 47576护木：控枪限定通过，实际贴合失败

明确交窗后stage220323/PG3e7，实际47576进入本枪护木。无Grip枪转差1.0245e-5度，物理左手移动时腕在枪内位移2.6368e-5原单位；右手移枪1.23225原单位/转2.29183度，腕相对枪位移8.6675e-5，两个被动保持判据通过。真实Grip后左手带枪转7.3046度，随后实际掌到实际护木距离22.39714原单位断言失败。不能把前三项数值通过当自然贴合或全四态通过；松Grip/回射/食指/换弹尚未执行。

三页完整配对总览已亲自查看，实际左手悬在枪身上方，符合贴合失败；启动第三人称/等待卡仍在。编码terminal0，launcher0且remaining_owned_game null，既有媒体清理正在exec2262运行，尚未交窗。精确runtime-47576-review.json；下一步针对生产model target与实际枪体约35度/22.4cm变换差异定位，不能加大范围或换成目标矩阵假装实际通过。

47576既有retention exec2262已实际terminal0，复查无game/XR/ffmpeg/review/HB/retention进程，已按双方先前明确确认唯一交Lancer212510，并同步Frag后续申请。HB不自行重查。继续离线核对model_to_component_rotation/RotOrigin、实际LocalToWorld及既有Native握持校准链；尚未确定35度偏差根因，未硬加补偿或新增写入器。

## 47576残差轴定位与既有回执补列

64组同frame/sequence延迟回读中58组角度34.9~35.1度，模型局部残差轴约(-1,0,0)，axis范围及每对矩阵保存在model-residual-47576.json。这缩小了局部固定旋转方向，但不能证明资产RotOrigin、组件被改写或引擎下游哪一处是根因。CPU矩阵不是每Draw证明。

只在已有两条普通GT回执补列：已读取asset_origin，以及恢复前首个已读取MeshPoseProbe的component_rot/absolute。无新对象读取/日志行/collector/writer/输入入口，不改任何动作保护。独立source54029c2加此前HB overlay完整编译链接exit0，新PGfe94c6f9cfa9e762e28fabb7ee290c27abb591a3934ffd52d73fb1ce95865f27，冻结hammerburst-prepared-20261108-222846，全部源与制品哈希核对。component-readback-build-check.json记录，尚未stage/实跑，不叫修复。共享文件只提交此8行精确patch，其他会话分支保持；Lancer异常段已收尾并交Frag，HB离线准备后续申请。

## 换新匣检查模式准备

离线新增显式replacement驱动分支：实际连续拔出原匣→真实松手→等待waiting_ammo→连续左肩路线及新Grip→等待new_magazine→持续持握入槽→闭合→有限库存按实际used/reserve核对填充量和守恒→权威回射。未调用游戏setter/ForceReload/GiveTo，不自动叠到护木或原匣返回检查。需要HB_RELOAD和HB_REPLACEMENT两个显式选择，默认仍原匣返回；护木与换弹互斥。两文件语法通过，但动作、路径、供弹与实际计数均尚未实跑。总0与L>0/R0仍需另行实际覆盖。

最新冻结hammerburst-prepared-20261108-223041继承PGfe94c6，全部源与制品哈希核对，replacement-driver-prepared.json；尚未stage。222846原冻结保持原字节。手雷35856/游戏26496实际在运行，Snub确认其后原匣段正常收尾再唯一交HB，本轮没有抢窗或标完整目标完成。

## 离线引擎分析可用性核查

为解释局部X−35度尝试绑定既定游戏EXE做静态引擎分析：REA明确provider_unavailable/REA_IDA_MCP_CONFIG未配置，直接IDA list_databases为空；未取得反编译证据，未安装软件/更改工具注册/启动目标。保留engine-analysis-availability.json。该工具条件不阻断整个开发目标：下一实际证据仍由已冻结PGfe94c6现有GT回执补列获得。Frag本轮运行已结束正在收尾，Snub确认其后原匣段再交HB，尚未归还窗口；不把暂时无进程当交窗。

## 56220变换对照：组件保持，实际模型仍偏转

223041/PGfe94c6实跑再次到达被动Snap/随枪/Grip转向，实际掌到护木22.397164原单位失败，后续release/fire/reload未执行。新增原回执已取得asset_origin=(0,-16384,0)，延迟组件旋转与写入值配对核对、absolute=(1,1,0)，具体统计component-model-56220.json。用现有旋转公式和写入值重建的模型误差很小，实际矩阵仍约35度，重点转到组件控制之后的模型变换，未确定具体引擎字段根因。正常运行器exec30932已exit0，当前双路review/编码exec60387运行，尚未审片/retention/交窗。

启动前原始settings哈希一致，首次文本比较换行正规化误判已纠正；config按wrapper backup恢复后运行。Snub后续指出外层selected-copy另有原始备份，不能将wrapper临时config当外层原bytes，后续需按两层原记录恢复并保留54740原失败。pre-stage-after-54740.json保存本轮比较，不撤回Native cleanup未验证失败。

## 56220收尾与C0视觉路径候选

三页完整配对总览已亲看，左手仍悬在枪身上方；编码exec60387终止0、retention15512终止0、launcher0且remaininggame null，wrapper config/settings/head原字节/哈希核对通过。随后已按Snub selected-copy-1794148651847569800原hex恢复外层config，避免将wrapper临时selected配置当外层原bytes。详runtime-56220-review.json。核idle后唯一交更早申请的Longshot基础路线段，Longshot已明确接收，之后Lancer再Frag，不双交。

源码发现HB普通PostTick视觉写入与已有Lancer/Frag渲染结果写入路径不同。候选只在render_current/apply/on_render_result三个门加入确切已绑定HB，让视觉写入使用同一C0结果回调和同一组件lease，普通Tick仍准备动作/几何但不写HB视觉。保留全部身份/时效/当前结果/原metadata及single-writer，不增Native写入或骨架写入器，不改Longshot分支。此为针对路径差异的修复候选，不宣称35度根因已证实。

独立PG编译链接exit0，现有组件lease278 CPU检查与follow校准/无效几何检查实际执行通过；不是游戏/C0回调/贴合通过。冻结hammerburst-prepared-20261108-225749，PG8d0fc939f9954ab35612cc50bff5efb92e0cd70c73d23020c6ede2e3a36d1117，源与制品哈希全核对，c0-visual-route-build-check.json；未stage/运行。

## C0准入核查与拒绝回执

源码只读确认body mount使用basic_hold资产观察、现有render_current核对当前held generation/身份/实际结果；HB加入回调候选没有改动作prepare或原过滤条件。为避免新路径被拒绝却只有空白日志，只将HB加入原render_rejected输出条件，原16条上限/字段/失败专用/私有模拟器限制保持，不增对象读取/collector/权限。完整独立编译链接exit0，冻结hammerburst-prepared-20261108-230036，PG2ac4b7d8d12c36cd4eb7e60ef9eca45e68df03932462e547911fb4a9b37ec2f8，源和制品哈希全核对，c0-rejection-receipt-build-check.json；未stage/实跑，不能证明C0实际准入或贴合修复。前一批lease278/follow测试针对未改组件逻辑，未为本行日志条件重复运行。

## C0候选的连发只读准备边界

源码核查发现既有RefireCheckTimer在视觉lease仍安装时，仅允许同一Lancer的prepare_only只读路径。HB转C0后也可能在PlayerTick之间连发，故仅将同一已绑定HB加入该只读选择；原lease.key、weapon identity、hold generation、nativeequipment::current、完整rig/basis及source检查不变。没有使用C0视觉时间延长输入/授权、没有新write权限。独立产品编译链接exit0，冻结hammerburst-prepared-20261108-230718，PG7bed6ef74017efb877ac9bdd5716f8600ddce52a743e0ffb356b1f259700c4fd，所有来源/制品哈希核对，c0-refire-prepare-build-check.json。尚未stage/游戏验证，后续必须实际验证持续Grip中的整段权威连发与release回射，不能只验首发。

Longshot21724已明确全收尾后唯一交Lancer，接下来Frag效果、Snub原匣、HB候选，HB未占窗。

## 提前保留真实失败身份

HB包装器复用现有已核活进程身份位置，在录像/动作启动前写出默认passed=false/native cleanup未验证的retention identity结果，flush/fsync。它只保留证据，不能授予动作或作为通过。正常结果另由原操作者写入，不覆盖已有记录；不改其他枪入口/共享runner/媒体保留策略。针对59800已出现的目标前异常退出丢媒体风险补齐。临时目录实际执行默认失败、同身份幂等、4类无效身份拒绝、禁止覆盖，以及2文件语法检查通过；未运行游戏。

新冻结hammerburst-prepared-20261108-231852，继承PG7bed6ef7产品不变，所有源与制品哈希核对，capture-identity-check.json。仍需实际验证HB C0贴合/完整四态/持续连发与回射/弹匣，尚未stage或交付。

## 窗口依赖阻塞检查点

连续三轮复核仍无本枪运行窗口：37120游戏/编码处理已结束，未收到其完整审片/retention之后的明确唯一交接；当前无game/XR/ffmpeg/known review/retention进程。只读核对不能把idle当归还。已向Lancer核对并由Frag/Snub确认后续队列；顺序Lancer收尾→Frag效果→Snub原匣→HB。此处等待外部窗口状态改变，不伪造新构建或重复离线测试当进展。

最新HB候选231852，PG7bed6ef7，继承Nativec985/XR1684；C0唯一视觉路径、原只读refire边界和提前失败身份已冻结，尚未stage/实跑。下一步在明确交窗并核idle/恢复后stage该候选，仅复查护木实际贴合/四态/持续Grip权威连发与release回射，再继续原匣/新匣/零供弹及中断恢复。完整目标未完成，所有旧实际失败保留，不把排队当验收。

## 最新等待执行约定

主人经既有统筹转达：等待共享窗口时保持当前会话，有界等待，不因短暂排队结束本轮或标blocked；明确交窗后继续已授权工作，不使用Computer Use。此前目标已由主人恢复active，本会话遵从新等待约定。当前Frag69812已全收尾/retention terminal0并明确唯一交Snub232252原匣，随后HB231852 C0复查；HB后唯一Boltok首段，再Frag新伤害失败点，不双交。HB候选/产品未改，不能把窗口协调记为功能验收。

## 70816：C0护木交互与连发限定通过

明确交窗后stage231852/PG7bed6ef7，实际70816完成原库存HB肩取、被动Snap不控枪、右手移枪1.23724原单位/2.28516度时腕随枪、真实Grip带枪转约7.30度、实际掌到护木误差0.004702原单位（旧56220为22.39716）、松Grip回原主手方向并保持被动腕吸附、回射、食指不控枪/不预留原瞄准、食指释放再回射，暂停/B正常。四组实际权威射线各6发；持续Grip段6发方向变化0度/枪口位移2.336e-5原单位，原只读refire边界实际覆盖。无error，不能把此限定通过扩成整枪/弹匣/全角度自然拟合/头显通过。

四页118.267秒全跨度配对总览与5张Snap/随枪/Grab/转向/松Grip关键原图已亲看，先前明显上方悬手已消除。指尖/袖口/遮挡和全角度自然握持尚未接受；近脸持枪构图不能证明实体舒适。画面仍有原拾取提示/UI等未关闭项，启动第三人称/等待卡保留。游戏记录最大捕获间隙1.9167秒，不作为逐帧/FPS证明。

launcher0/native_cleanup_verified TRUE/entry_restore TRUE/XR0/remaininggame null，config/settings/head按本次备份字节及settings哈希核对一致。编码exec61699已terminal0，既有媒体清理exec27270运行中，尚未交窗。提前interrupted身份已在实际录像前成功保存，仅身份默认失败，不抵消正式result。详runtime-70816-review.json与contacts-70816.json。下一项仍原弹匣拔出/持握返回/库存守恒/回射，另段排期，不重复本次已过护木。

原匣下一段驱动补入70816实际使用过的近身跨胸主手输入姿势(.16,-.18,+.06,yaw25度/pitch0.3)，在原首发准备前通过既有continuous reach移动；不拉长骨骼、不修改接触半径。支撑路径不变，换弹实际接触/翻盖/原匣/回射尚未覆盖，语法通过。下一冻结hammerburst-prepared-20261109-001658继承已实跑PG7bed6ef7、全哈希核对，reload-presentation-prepared.json，尚未stage/运行。

70816既有retention27270已实际terminal0，正常恢复和无残留进程核对后按约唯一交Boltok首段，转达其后Frag真实伤害失败点、Lancer HUD/掩体已有申请。HB继续持会话等待、离线准备原匣，不自行启动第二段；后续请求在已登记队列后一次原匣检查。

## 模块化：公共被动/主动握点判定

按主人最新模块化要求，仅把原HB头内完全武器无关的free/snap/grab只读判定提取到weapon_support_socket_mode.h，命名gears_weapon_support_socket；HB原header保留Mode alias/薄转发。公共模块只依赖既有Frame/GripGameState/Decision和support_contact接口，不含HB类型/几何、游戏读写、Native/资源/UI或测试驱动。避免Longshot必须依赖HB命名模块；已通知其自行切换调用，不改其他枪分支，不扩展ABI/结构字段。

原函数体与提取后逐字一致；已有HB真实owner/participant离线测试执行Snap/Grab/松手/退出和11类拒绝通过，独立PG完整编译链接0。冻结hammerburst-prepared-20261109-004003，PG9945b703ad8a8f206af8e629758da121d8534b36c8fd6a032c1a7096bc6cbe89，全部源与制品哈希核对，common-support-policy-build-check.json。新批未stage/实跑，不把提取算新功能或游戏通过；70816限定交互证据保留，下一仍仅原匣未过单点。

公共接口按Longshot当前明确需求增加第5参数retained_snap=false，仅传给neutral最后support_contact，前方实际Grab/身份/松手/竞争物条件不变，HB薄层4参数保持默认false。新增4断言实际通过：18cm默认free、已选被动保持可snap、旧生命周期即使保持也拒绝、18cm新Grip即使保持也不能获Grab。原HB测试11类拒绝同跑通过，产品完整编译0，未更改wire布局或Native。冻结hammerburst-prepared-20261109-004159，PG28f0184d2a3fb3e5850c47c29e85f1ef74cbaf31c11154e51c7554cd0ca994ad，全哈希核对support-retained-policy-build-check.json；不把公共接口提取或纯计算检查当实际换弹通过。

## acquiring真实松手恢复边界

离线检查发现stage acquiring在granted=false时直接返回，未登记实际已知UP。失追清held/released后，UP仍不能rearm，后续新press会永久被released=false阻断，直到外部取消。用实际生产acquiring分支和原granted谓词原样提取执行的CPU夹具复现：旧分支exit1/FAIL release observed during acquisition。夹具首次新版本未显式取消Decision默认release_required=1，导致后续fresh-grant假失败；修正夹具后旧分支仍在同点失败，新分支9项通过，失败与修正日志均保留。

修正只在acquiring且动画所有权仍有效时识别已知UP、清held并登记released；不获取动画、不seek。后续仍需当前帧实际fresh grant/相同身份/新press才能继续；未知输入、无UP旧按住、foreign frame、旧retained lease、release_required、失去动画所有权均拒绝。夹具游戏读写被替代，只证明这段规则，不证明失追实际恢复已过。产品独立完整编译链接0。冻结hammerburst-prepared-20261109-010655，PG8e2a8c3873962287ce21a6707e494ffde05a686b39841e0876c5ee17e91341b2，源与制品哈希核对，acquire-up-build-check.json；未stage/运行，不改变旧实际失败。共享HB尾部其他人的service包装未收入此独立编译/本提交。


## 14312：原版reload取得，开盖连续见证间隔失败

Cover正常唯一交窗后stage010655/PG8e2a8c38，实际14312完成原HB肩取与6发权威射线；真实Grip取得原版reload serial1、loaded72/reserve78，并进入1→2 opening。首失败是reload_chain.py第74行等待to4超时，而非第70行等待to2。早期仅看同prefix异常摘要错误判断为未进入opening、随后误怀疑读者，均已用完整调用栈/原始日志纠正：公共records已正确读取to2，没有公共读者缺陷证据。

实际开盖angle0.526562711（约30.17度）/sequence2180出现hammerburst_latch_rejected reason5=gap，连续观测超过保持不变的100ms阈值；后续未达到opened/old_magazine4，未拔匣、未供弹、未提交库存。cleanup记录stage2/stopped1/pending0。本次不证明已修的acquiring失追UP恢复，也不计完整原匣换弹通过。下一定位既有动作/输入采样开销与真实松开后在当前机械位置重新抓取的恢复，不延长输入或连续见证阈值。

全部52秒双路2页配对总览和3张实际接触原图亲看，当前图像中左手在枪侧，翻盖未完全打开；自然指型/全角度/遮挡仍未接受。Native cleanup/entry restore均true、XR0、launcher0、remaining game null；config/settings按010831备份逐字节一致，head恢复不存在。双路编码完成，retention72460 terminal0删除2591媒体/452782166bytes，源/文本/数值保留。核idle后已唯一交Snub010736，随后Boltok→Frag011230→Longshot005036→Cover reader。详runtime-14312-review.json。


## 开盖gap离线恢复与连续动作驱动

实际生产release/opening两个完整分支和granted原谓词不改字提取，使用真实HammerburstLatchStroke，只有动画/库存/本机观察引擎调用由CPU夹具替代；10条实际通过，涵盖101ms撤销但机械角不变、继续旧按住不能重启、unknown UP不算release、真实UP只rearm、retained新token不冒充fresh grant、release_required拒绝、超实际接触半径拒绝、当前机械位置新抓不seek、后续连续运动/真实端点到opened。夹具首次用0.3rad当超接触范围，实际该半径对应约2.1cm仍在3.5cm允许范围，故正确被接收；将夹具改成真实超半径后通过，原夹具失败另存。此为规则回归，不证明14312gap的完整耗时根因或游戏恢复已通过。

测试驱动把原19段各自停止/等待的开盖方式换成1.6秒最小jerk连续物理圆弧，46次held Grip输入，不改游戏/阈值/手臂/机制。加入各输入采样时间与角度、明确wait目标和末8条回执；若实际再出现一次gap，只允许一次真实release→回到当前机械接触点→新Grip→继续，另记latch_regrabs/normal_uninterrupted_pass=false，第二次拒绝终止，不无限重跑。实际驱动提取执行连续样本/端点/圆弧半径/按住/单次恢复顺序离线检查通过。

冻结hammerburst-prepared-20261109-012255，PG仍8e2a8c38，产品/Native/XR不变，全源和制品哈希核对，continuous-latch-prepared.json。未stage/游戏验证，排在已接受Cover reader之后一次原匣失败点；当前Snub窗口，不从idle推断归还。
