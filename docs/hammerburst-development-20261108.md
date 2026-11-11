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

截至2026-11-09，源码与测试已保存，独立Playground完整编译；公共v71未改。当前在C盘同一worktree工作，运行采用独立冻结候选，其他武器的未提交源码不混入。

| 范围 | 当前证据 | 状态边界 |
| --- | --- | --- |
| 右肩取HB、实际原版开火 | 14688与后续多场；来源一致的原生轨迹 | 已模拟器实录 |
| 护木Snap→Grab→松Grip回Snap | 70816实际手腕/枪体矩阵与4组6发 | 四态功能通过；自然手指/全角度贴合未接受 |
| 原匣提取、不释放装回、关卡扣 | 7712生产阶段1→2→3→4→7→8与complete result1 | 本次真实流程通过 |
| 原匣库存与回射 | 7712保持72/78，真实6发后66/78 | 已模拟器实录，无补弹/奖励 |
| 新匣替换与普通换弹 | 53224真实提取释放/肩取/装入关卡扣、72/78→78/72，回射6发→72/72 | 功能通过；开关各一次移动gap后真实重抓，不算无中断通过 |
| 零备弹、总零 | 7500组纯账本等离线检查 | 仍缺本枪游戏动作证据 |
| 静止基线与失追proof撤销 | 实际生产分支21+7项红/绿回归 | 7712静止计数0、未失追，不能称live覆盖 |
| 完美/取消、换弹中断/收枪 | 有实现与部分纯测试 | 仍缺本枪实际动作证据 |
| 完整自然手型、舒适度、实体头显 | 尚无接受 | 未通过 |

当前070119-refire-supply已在实际72536后撤回，不得交付或不变重跑。其HB timer认证/延迟路径实际执行，但78射击编号只54条有效轨迹，耗弹目标仍失败，尚未进入零备弹/总零。共享源码7e9c0c1c恢复Lancer-only defer入口，独立契约/测试保留研究，撤回后的隔离PG仅编译通过，未stage/game。current-candidate-state.json保存retired状态；下一步是原六发burst在R2-up后的已提交尾段与撤销契约，不放宽源时效、不删射线断言。实际原匣7712、新匣53224及失败53624/72536范围、影像与正常收尾见本文末尾。

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


## 原拒绝回执补准确间隔

14312旧回执只有reason5，无法区分准确的前后样本间隔。只在原最多8条失败输出补last_observation_ms、sample.milliseconds和geometry原阈值；pure stroke增加只读getter，不改变其reset/observe/begin/连续见证或授权，未增游戏字段读取/采集器/Hook。源码精准提交fd8033a4仅此两文件，其他caller的service/recover_serial修改未混入独立产品。生产分支回归已持久化为tools/playground/weapon_hammerburst_latch_handoff_test.py，真实MSVC W4/WX编译执行10条通过；独立PG完整编译链接0。

冻结012813/PGfe0bd568e312df0ef519ce20fea887d6298fd8ba790a933ddc27253a66026751，替代未stage012255，动作仍连续圆弧/最多一次真实松开重抓原匣单点。全源/制品哈希核对，latch-interval-build-check.json；未stage/实跑，未声称间隔根因修复。现队列Snub→Boltok→Frag011230→Longshot005036→Cover reader→HB012813→Lancer012317，Cover已知最新候选。


## 实际容量78库存边界离线回归

14312类型化双读实际C78/L72/R78，而原7500合成矩阵上限C60。保留原矩阵并增加实测容量78专例：原匣返回即便闭合在perfect window内也只exit_without_fill/inspection、72/78不变且无奖励；备用0只搬已有72发、serial0且不发库存请求；总0拒绝替换匣。MSVC/W4/WX/UNDEBUG真实编译执行通过，日志supply-observed-capacity.log。首次cmd相对正斜杠路径未执行编译，修正绝对反斜杠调用后通过，原调用错误另存。该提交只有测试，不改012813二进制或未完成的游戏覆盖。


## 44560：开盖/原匣提取达到，同匣座入驱动换轴错误

精确012813/PGfe0bd568实际44560肩取/6发后获得serial1，连续圆弧无gap/无regrab达1→2→3→4，实际开盖角0.822600225；真实骨链初观测0.761657触发既有endpoint_intervals选择，随后0.822699045一致并建立feed。原匣实际外拔最大记录depth0.0926914653，随后同匣返回等待to7失败，关盖/回射未达到。不能扩成原匣完整通过。

实际同一回执的hand_quat对照发现测试驱动左Grip basis换轴乘序错误（maxdiff1.49356）。旧算法在计划depth0时真实预测depth19.6805mm/lateral18.8273mm，超过原6mm座入深度，和当前实测相符。使用本次已认证world左Grip旋转、先断言raw orientation未改，再转换到production model，矩阵maxdiff3.46e-5；对同一真实itemoffset复算depth1.3465微米/lateral0.6425微米。只修输入规划，不改产品/容差/库存，不能把离线预测当实际座入通过。保留magazine-input-basis-44560.json。

2页完整双路总览与4张真实接触原图亲看；自然手型、袖口遮挡和近脸构图仍未接受。Native/entry TRUE，XR/launcher0，game退出；config/settings逐字节恢复、head原缺失恢复。编码35734 terminal0、retention54890 terminal0实删2432媒体/417441210bytes，文本源码保留。详runtime-44560-review.json。

冻结022007，产品仍PGfe0bd568，源/制品全哈希核对，driver语法及真实数据旧错误/新规划深度断言通过。Lancer明确未stage并接受按具体失败许可仅这一坐标驱动修正复查；本次全收尾后执行一次，若新失败保留后交Lancer、不连续追加。后续既有Lancer012317→Snub013007→Boltok013903→Frag020326→Cover新只读项→Longshot021930。


## 50736：真实松开重抓恢复后再遇112ms，未座入

022007/同PGfe0bd568实际50736/creation134386357043262872已肩取/6发；opening第一gap=111ms（970317728→970317839），机械角0.643563172；一次真实UP/new Grip在当前接触点恢复，机械角继续到0.786032468，第二gap=112ms（970320619→970320731）按驱动规则停止。未到4，正确座入规划尚未实际执行，不能称原匣完成或gap修复。原100ms规则保持。

完整2页双路总览+3张原接触图亲看；Native/entry TRUE、XR/launcher0、game退出；config/settings逐字节恢复、head原缺恢复。编码89058 terminal0、retention62914 terminal0删612媒体/86251703bytes，核无残留后已最终唯一交Lancer012317，再既有尾部。交接消息首次creation数值录入错误，已立即据正式result更正；正式runtime-50736-review.json始终是真实134386357043262872。

## HB seek新鲜观察复用候选

原seek_impl先read_owned/reconcile，随后prepare_controls又observe_expected完整双选node/资产/字段；实际前置信息在后者已新鲜读取。仅exact HB改为先同一完整observe_expected，严格比较武器/组件/资产/node/sequence全部identity、动画名、长度、reload_start、playing/rate/looping、有限time与pending期望，再prepare_controls(...,true)复用该观察；SetPosition原函数/参数/当前输入与lease最终准入和post-seek read_owned不变。其他枪仍原reconcile_seek+prepare_controls路径。原observe_kind/observe_expected/read_owned/prepare_controls/read_latch_curve/reconcile_seek逐字核对未改，未跳过内容或selected-node读取，不缓存授权。与Snub明确非重叠，不收入其独立分支。

生产reconcile函数原样提取、真实ObjectIdentity语义执行53项身份/回收对象/名字/内容/时钟/playing/pending/NaN等正反例通过；独立PG完整编译0，源码10f70100。此为源码候选，尚未证明111/112ms根因或消除gap。

冻结hammerburst-prepared-20261109-023148，PG28710bea1b8ca5b2e8452ca3067df66b37352b1624f6bb68337364d11476527c，全部源与制品核对，seek-observation-build-check.json；未stage/game。后续只申请既有队尾一次原匣同点，不重跑其他套件。首次冻结脚本已完成复制/哈希/回执后，在追加文档时误复用变量触发TypeError，未写入文档；本次只据保存回执补文档，不重建或改写冻结。


## 同一开盖候选的有界seek耗时

为后续若再超限时区分读/写准备耗时，仅在原最多8条拒绝行补previous_seek_ns：最后一次改变角度的seek_latch_angle调用总耗时，两次原GT clock读取，不增加游戏字段/采集器/Hook/输入或许可。该值不保证就是上一静止样本，不单凭它认定gap根因。实际opening/release分支更新后10项执行仍通过，独立PG完整编译0。

新冻结hammerburst-prepared-20261109-023513，PG821966d17fe7c86997713f317e95182e85bfc6f9ece8000b79415e7598467ce2，替代未stage023148；53纯reconcile用例的生产helper不变，10控制分支重新编译执行通过。所有源/制品哈希核对，seek-cost-build-check.json；未stage/game，排位沿用原申请，不增加动作或时长。


## 观察回归夹具补强及最新协作约定

017ee559仅补强测试：weapon/component/asset/node/sequence使用互不相同的真实类型ObjectIdentity，另加有效node/asset身份互换两类拒绝，避免相同基准身份掩盖错位比较。对实际隔离生产helper原样提取执行55项通过，函数hash仍99dd1882d65575a52ea2a8b545d1b847c5927e5ecc61009547bcb27a35890319；产品和023513冻结未改，不重复编译游戏DLL，也不将新测试文件假称已收入旧冻结。

主人最新经既有统筹转达：不主动向01a1bb5e-5946-7271-af0d-1068759697e0发送进度、候选、测试、队列、收尾，也不回信确认该通知。已经停止抄送；仅在本会话/本记录保存进展，直接相关会话必要的唯一窗口交接与依赖协调仍可进行。等待继续有界保持会话、不据idle抢窗、不用Computer Use。当前Frag69684已全收尾唯一交Cover owner-only-r2，后Longshot021930→HB023513→Lancer023554→Snub024436。


## 提取证明在明确撤销路径的适配器回归（未入已排冻结）

纯HammerburstMagazineExtraction已对无效sample清complete，但运行适配器在当前frame不可用及left_tracked=false两个撤销分支仅清held/stroke/insertion，未清extraction。真实生产撤销/松手分支原样提取+真实纯提取器CPU夹具：先连续0→6cm完成证明，走明确tracking撤销后complete仍true，旧源实际exit1/FAIL tracking loss clears completed extraction proof；未借假输入或默认成功。

只在这两条既有撤销路径补extraction.reset，后续已知UP不能再凭撤销前的旧证明stow，按既有未完成提取取消；新源7项通过，涵盖两种撤销/之后UP、合法continuous提取后UP正常stow，以及replacement失追原预留释放行为不变。旧红/新绿记录分别tracking-witness-before/after，源字段/引擎stow与cleanup为fakes；只证明这些条件分支，不证明实际Native生命周期必到该路径或全部中断恢复已通过。

这一安全源码与测试单独保存，尚未完整编译或收入023513/PG821的不可变冻结；当前队列原开盖候选、驱动与源归档均不改，不假称该补丁已在下一正常开盖段被游戏验证。主线仍等待Longshot最终交窗，之后唯一交Lancer023554。


## 9540：新鲜观察复用仍超限，未证性能修复

等候时仅给原回射补真实扣弹/射线计数只读断言，冻结034516/PG仍821966d1，全hash核对、无新增输入/动作。二十项HB关键脚本/依赖与冻结一致；本枪Resolver/wire reader路径不import game-playtest.world_state.Query，Cover五行cache键修复未混入本枪。root另存失追proof撤销补丁未入该冻结。

实际9540/134386407745387891肩取/6发、原版begin serial1和opening2达到，随后两gap112/117ms，机械角均0.35093584；最后一次changed-angle seek耗时13.5967ms，该值不是上一静止样本耗时，不能当根因。一次真实UP/newGrip后再超限按规则停止，未到4/座入/关盖/回射；新的扣弹断言也未执行到。复用观察未解决gap，不能称性能修复。

第二gap sync975409239000000比第二次圆弧第一命令975409332132100早93.1321ms，支持检查真实重抓后的静止等待，但没有记录实际前后local hand位移，不能直接称完全静止。下一只考虑保留严格输入/真实Native同lease/不携带运动的条件，不能直接放宽100ms或将旧press重新授权。

54秒双路2页overview+3张按QPC选取实际接触图亲看，选择时间差12.021/-18.318/-0.643ms；自然手型和全角度仍未接受。Native/entry TRUE、XR/launcher0、game退出，config/settings原字节和head原缺恢复；编码64883=0、retention31381=0删1340媒体/206534215bytes、核无残留后唯一交Lancer023554。之后Snub024436→Frag030808→此前已受理Boltok030739→Cover新只读→Longshot034956，已直接协调，无统筹抄送。详runtime-9540-review.json。


## 同一当前Native抓握的静止基线，不带跨gap运动

9540第二gap早于第二圆弧命令，但保存的XR CSV只有眼位，不含Grip；Native continuous-hand-plans32条环尾最早975415224430000，已晚于gap975409239000000，不能从它补造当时手位。保留实际位移未知。

针对“同一真实Native lease一直retained、接触点数值不变却被消费采样间隔撤销”的条件，旧生产opening/release分支实际CPU红复现。dc59ee41只为HB显式opt-in：当前Frame tracked/known/down、granted严格通过、decision retained且非new_grant、held与当前lease完全相同时，纯stroke允许≤1e-5m数值接触噪声重建静止基线；不seek、不推进角度/端点/阶段、不更新原reload时钟或奖励。任何0.1mm移动跨100ms仍拒绝；实际tracking/frame撤销已清stroke/held，旧press不能复活。其他消费者默认参数false，原100ms观察/输入时限和所有身份/原函数/写入门不变。仅原transition/失败/cleanup行附stationary_holds聚合，不新增日志行/采集器。

旧源17项夹具到静止条件实际exit1；新源21项执行0，新增同lease无动作/后续及时运动、new_grant/别的lease/lifecycle/untracked/release_required/foreign frame/真移动/显式撤销拒绝。另7项明确失追proof撤销回归已保存，55项seek纯reconcile不变。独立GearsPlayground完整编译链接0。旧整套legacy模型测试在其另外的forgiving-profile段因3个早已不存在的字段编译失败（default-latch-compat.json保留）；未为过测添加或放宽字段/接触范围。其原有continuous_hinge_motion函数原样单独提取、当前默认false路径编译执行0（default-latch-motion.json），不称整套通过。

冻结hammerburst-prepared-20261109-043309，PG611a40908e79f66f720d09042c3f64289d890e7e0ffd28806e9bd6187004e3a5；全源/制品哈希核对，stationary-hold-build-check.json，未stage/game。包括已独立回归的两处extraction.reset，但静止实际是否满足10微米条件仍待原记录/游戏计数证明，不称9540根因已定。后续只申请既有队尾Longshot034956之后同一原匣点，不插队。


## 遗留模型测试按当前严格契约修正

Lancer只读确认上述forgiving-profile不是其本轮改动，未补写共享头。现仅修正HB测试夹具：保留历史原源码/编译失败，将不受支持的3字段/12cm宽松profile改为当前实际API的明确拒绝与精确端点检查。新断言包括12cm配置拒绝、离轴超抓取体拒绝、静止只报closed而不open、0.69不提前open、0.79真实到open、1.05超程拒绝、默认静止151ms仍gap、0.1不提前closed、0.01到closed，以及unheld/inactive即使显式opt也不可复活。没有改生产范围/过冲/端点阈值。

更新后的完整既有纯model/curve/latch suite真实MSVC/W4/WX/UNDEBUG编译执行0，strict-legacy-model.json；不冒充旧forgiving能力已实现，不把其旧R3抽象模型视为当前Grip入口/实际库存或游戏证明。先前default-latch-compat编译失败和旧fixture都保留。

同一静止候选原失败行补retained_same和真实前后手位移hand_delta_m，仅用stroke已保留样本，不读新对象/不增日志行；生产控制分支21项重跑0、隔离PG完整编译0，避免后续再只能猜手是否静止。Getter只读，行为与043309相同。


最终同一静止点冻结hammerburst-prepared-20261109-044514，PGa03ec04513c879b105c35da6ae3cf8e6f2f1aca6fe37d11e94106d04fe69d264，替代未stage043309；21+7+55及严格旧纯模型测试的边界如上，完整PG0/全source与制品hash核对，stationary-metric-build-check.json。未stage/game，动作scope和排位不增加，真实位移/静止保持次数等待实际回执。


## 7712：同一原匣取出装回及真实回射通过

044514/PGa03ec045全归档源码/6制品及12项当前运行输入hash核对，唯一交窗后stage。实际7712/134386451280970537：肩取6发后72/78，原版begin serial1；卡扣1→2→3→4，真实取出原匣、同匣不释放装回4→7，关盖7→8，complete result1。原匣库存仍72/78、capacity78、bonus0；回射6条来源一致的有效原生轨迹、实际loaded72→66，reserve78不变。operator0，非仅wrapper通过。修正后的实际Grip基底在本次达到真正座入和完成，未靠库存setter。

没有latch拒绝或重抓，stationary_holds始终0；新的静止保留分支没有live覆盖，不能据此宣称9540旧gap根因确定或静止修复已被游戏验证。失追proof撤销也未作为本段动作验证。

56.3s双路全2页overview及simulator00690/00730/00844/00920原接触图亲看。可见弹匣提取/回座过程，但近脸构图、手/袖自然贴合与全角度仍未接受；初始第三人称/等待卡和HUD既有问题保留。game lane最大捕获gap1533.3488ms在启动段，不当游戏停顿/FPS根因。Native/entry/pad恢复TRUE，hand_stop5/drained，XR0、launcher0、game退出，config/settings原字节及head原缺恢复。编码64780=0，retention33602=0删1954媒体/332573205bytes，核idle后最终唯一交Snub042943→Frag045142→Cover r8。runtime-7712-review.json保存范围；全枪/实体头显不算完成，下一验证是实际新匣替换与库存守恒。


## 53224：真实新匣替换及守恒通过，移动gap保留

050438-replacement沿同PGa03ec045/Nativec985/XR1684、原驱动未改，前驱Longshot71272完整收尾唯一交窗后重新核6制品/12关键运行输入并stage。实际53224/134386475569110764，原肩取6发后L72/R78；原版serial1，卡扣1→2→3→4；真实连续提取+UP使4→5，左肩真实新Grip token3得到78发新匣、仅预留备弹6；同握持续接近入槽6→7后实际L78/R72，关卡扣7→8、普通complete result2。回射6条有效且来源一致的原生trajectory，实扣L78→72/R72，总量与发射一致，bonus0。operator0。

此次不是无中断卡扣通过：opening stage2 angle0.000645289526与closing stage7 angle0.813532753各gap112ms，actual retained_same均1，实际hand_delta分别0.000619197414m与0.00178300227m，last changed-angle seek17.2725/14.3677ms。二者都是移动而非≤10µm静止，strict100ms正确拒绝；各一次真实UP/newGrip后完成，保留两个实际失败及恢复，不把driver整体pass覆盖它们。stationary_holds0，新静止分支未live覆盖；不能把seek耗时单项当全部gap因果。

76.95s双路3页全跨度overview及simulator00825/00950/01211/01343原图已亲看。可见旧匣释放、左肩取新匣和回座，但自然手指/袖口、近脸构图、完整多角度及头显仍未接受；game startup capture gap1850.0287ms不当游戏停顿/FPS证明。Native/entry/pad恢复TRUE、hand_stop5/drained、XR/launcher0、game退出、config/settings原字节和head原缺恢复；编码99894=0。媒体保留41102=0，删除2655媒体/464596636bytes；核idle后最终唯一交Snub051415→Boltok051519→Frag045142，runtime-53224-review.json保存实际终态。


供弹边界驱动在运行前改为已实录的真实R2按下/释放突发，每次只在读到实际弹量进展后才继续，首个不扣弹即停止；不假定一次长按能耗尽突发武器。增加多次真实边沿和无进展退出的假引擎测试，九项通过，check_supply_driver.py及精确hash随冻结保存。旧054953保留未stage/run，新冻结同一队尾位置，产品与目标不变。


## 53624：耗弹前提射线缺证，零备弹目标未达到

055335-supply-bursts全2229源/6制品及13当前关键输入核验后stage。实际53624/134386503726632506，肩取和首6发通过；必要耗弹达脚本库存empty/reserve保持断言，随后actual轨迹数断言失败，operator1。全局只75条有效来源一致trajectory，缺shot20/40/72；其中耗弹段69条而非72，无128截断或drop回执。未进入任何换匣/R0/总零支路。失败helper在return前未持久化before/after数值，明确缺少原始typed count快照，不按控制流补造观察数值；只记录先前库存断言已通过。下一驱动要把已读观察先保存再断言，不增游戏读取或放宽射线要求。

现有准入计数最后candidates144/bound144/stale_sample6/source_ready138，最近rejected_source sync103.4301ms/stereo103.4345ms、sample59.2257ms、frame78.2286ms，held_current1/frame_active0。与3个缺失shot有关联方向，但仍须逐发/原函数契约核对，不能当完整因果已证或改变100ms。冻结源码中lancerrefire::defer与capture_initial显式lancer_only，HB另有自己的WeaponFiring.RefireCheckTimer/BeginState/ShouldRefire/FireAmmunition；不能直接放开类型复用Engine循环认证。

32.78s双路2页全跨度overview+4张按相邻已记录shot的QPC选取实际原图亲看，simulator00547/00589/00658/00671，偏差20.9695/-5.7572/14.7535/2.5184ms。这是失败附近画面，不是缺失shot的像素/射线证明；完整自然拟合与头显仍未接受。Native/entry/pad恢复TRUE、hand_stop5/drained、XR/launcher0、game退出，config/settings原字节与head原缺恢复；编码15219=0、retention19806=0删2010媒体/368256485bytes，核idle后唯一交Frag既定053956位，后续Longshot060433→Cover r12。runtime-53624-review.json保存缺证与实际失败。


## HB原突发射击timer独立契约适配（尚未游戏验证）

按本地reverse-engineer-anything证据流程，只读取已验证原包及既有UELib分析工具：WarfareGame.u SHA91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13，export37979的HB WeaponFiring.RefireCheckTimer原串71bytes/hashdd4d481e，runtime声明79bytes，差值为两个反射property操作数由32到64位的正常重定位。完整跳转0x19/0x32、bWeaponPutDown、PutDownWeapon、ShouldRefire、FireAmmunition、bWasTargeting和BurstDelaying分支匹配；UC含未解析native token，不仅凭反编译文字授权。hb-refire-static-contract.json保存偏移、包/导出/工具hash和局限。初次私有解析器缺已有Unsafe依赖，仅复制同一既有工具目录的精确DLL后成功，不安装/升级软件，不执行游戏脚本。

与Lancer原writer边界确认后，提交47f69b96新增独立HB matcher和timer契约adapter。共享refire入口只增exact HB选支，复用不变的Engine TimeWeaponFiring循环timer证明及同一个DeferredEvent/normal PostTick；不新增事件owner、回调、writer或HB BeginState捕获。Lancer原Engine/capture_initial分支、实际R2 press/release/counter、身份/引用/hold/freshness、100ms输入和20ms调度余量、已有250ms事件原期限均保留，聚合/重发不续期。HB专用timer自身zero-param/void/VM/无locals、同包原class/state、typed原函数、Bool property/输出、完整bytecode和关闭读都严格核验；只对已拥有的实际R2定时器事件生效，不能宣称全部释放后burst尾段都已覆盖。

167项独立原始71bytes重定位夹具/全字节变异/截断/错误identity matcher通过；实际生产adapter在有界metadata fakes下52项拒绝/闭合检查通过，Engine证明和生命周期在该夹具为stub；既有Lancer原事件/loop145项通过，隔离完整PG0。9项drain-helper假引擎检查增加断言失败前的实际before/after/已导出轨迹保存验证。只这些离线证据，不把它们当53624因果或游戏修复证明。冻结065400-refire-supply/PG63da3c54，2236源/6制品核对，未stage/run；请求在所有已受理尾项后复核一次真实耗弹前提，过后才续R0/总零。


静态继承链补核：PutDownWeapon由WarfareGame.WarWeapon export35863覆盖（UC SHA045eb54e），非仅Engine基类。旧47f69b96生产adapter在此原包归属的metadata夹具实际断言RED(-1073740791)，保留war-owner-before源与log；50a208b4仅给HB此调用启用既有严格Engine/WarWeapon函数白名单，默认War归属+Engine兼容及外来归属拒绝54项GREEN，已有145事件检查及完整PG0。没有为过测接受同名其他类。065400未stage/run，后续精确冻结hammerburst-prepared-20261109-070119-refire-supply、PG30239f21a306c31d5fd1d357556aa11c1f32201d2b57cbe824c28aa109f719ce，仍2236源/6制品，替换同一队尾位置。


## 72536：已进timer路径但缺轨迹恶化，撤回不完整接入

070119/PG30239f21全2236源/6制品及13关键运行输入核验后stage，实际72536/134386554201091833。Native timer原契约admitted、deferred及normal PostTick delivered确实执行；不能再说路径没触达。首6发通过，随后13次真实R2按下/释放耗弹before L72/R78→after L0/R78，typed数值和所有已导出trajectory在断言前保存。全局54条有效同源轨迹，其中耗弹48而非72，缺shot18/23/24/29/30/35/36/41/42/47/48/52/53/54/57/58/59/60/64/65/66/70/71/72；operator1，未到任何换匣/R0/总零，断言未放宽。

此结果比53624的75/78更差，退回候选且不交付。源码确定本版defer/deliver仍要求owns_press/trigger held，而原HB ShouldRefire对本地真人按BurstCount<NumBurstShots继续原突发，即使已R2-up；六发尾段的新鲜姿态/原意图授权及撤销尚未接齐。此缺口与缺失增加高度相关，但尚不把所有逐发因果当已证明。不得伪造R2-down、续旧时间、增加原100ms/20ms/250ms、补造射线或改原弹药来过测。

33.43s全部2页双路overview及simulator00522/00553/00596/00689实际原图亲看；这只是对应射击附近画面，非缺失射线证明，自然手型/头显仍未接受。Native/entry/pad TRUE、hand_stop5/drained、XR0/launcher0/game absent、config/settings原字节/head原缺恢复；编码80782=0、retention21276=0，删2133媒体/358730210bytes，核idle后最终唯一交Coverr16→Snub071103→Boltok072100→Frag074333→Longshot075334，runtime-72536-review.json完整保存。

7e9c0c1c只恢复共享defer的Lancer-only入口（未接入HB原first-state或finite尾段前不启用），独立HB契约matcher/夹具保留；撤回源码完整PG编译0，非新runtime验证，公共v71和Native未改。后续原有限burst的授权与失效清理需要一起实现：真实有效第一发才可形成有限剩余承诺；每发仍需新鲜Native姿态；实际R2-up不伪造按键，失追/生命周期/收枪需经原版停止和正常状态转换收尾，不能在失效恢复后自动续旧burst。

## 原有限burst授权与原状态撤销的离线实现（未接入运行入口）

保存db7f1737/7e268558/3518de36：HB独立Completion只允许真实有效首发形成最多剩余5发承诺，实际R2-up允许同一press的一次release后继，不重造按键；一轮结束、重复事件/旧ticket、身份/hold/reference/session/space/input/profile变化和中断后旧回调都不再授权。预分发保持100ms和20ms余量，实际回调保持原100ms，不把已完成射击的观察强加未来余量。766项CPU夹具边界检查通过，不是游戏证明。

新增原PendingFire数组全零和Core.Object.GotoState(native113/24bytes)退出适配器，原ButtonRelease必须先完成；只调用原状态转换进入HB BurstDelaying，让原Firing.EndState负责原refire timer清理，不写计数/弹药/VM/定时器。初版夹具错误假定Weapon.InvManager/Byte元素；原包完整object inventory确认Engine.Inventory.InvManager(export14497)、InventoryManager.PendingFire(export14545)及其IntProperty4 inner(export14544)。按真实布局改夹具后，旧适配器实际RED(-1073740791)，保存cancel-original-layout-before源/log；生产修正后103实际adapter fake边界/顺序检查及隔离完整PG0。不能把最初错误夹具的GREEN当真实ABI证明。

离线读取原ShouldRefire37982(83 serialized/111 runtime)、FireAmmunition37971(16/24)和Engine Firing.EndState29658(116/152)，hb-burst-static-contract.json保存原包/原串/hash/属性身份。完整ShouldRefire的真人判断、原WarWeapon super、HasAmmo、Byte BurstCount/NumBurstShots、分支/转换/返回，以及原Byte++再调用WarWeapon.FireAmmunition均逐字节匹配；timer加这两段共454原串重定位/变异/截断/identity CPU检查通过，仍非运行中native/脚本证明。

工作区新增weapon_hammerburst_burst_contract.inl：正常GT预检实际原函数/属性、原IsHumanControlled和完整bytecode，关闭读，不在回调调用getter。weapon_hammerburst_burst_runtime.inl将真实首发、有限尾段scope、POD轨迹观察和原状态撤销串为同一事务；这两模块仅在私有隔离playground.cpp增加compile-only include，完整PG编译0。共享root playground/refire/R2/unified-fire尚未加调用，HB入口继续withdrawn；runtime桥实际生产代码的275项metadata/source/engine fake范围检查通过，原函数certificate在该夹具是stub；原first-event捕获/after-up source准入及正常6发结束边界的共享入口仍需接齐，不能stage/run或交付当前隔离DLL。

Lancer原writer确认这些共享入口当前无其未保存改动，但R2/unified-fire已有Longshot脏改，继续完整保留，只做exact-HB增量且精确暂存自己的hunk。无新HB候选/窗口排队，不重复070119/055335；公共v71、Native和吸附/Grip四态保持现状。


## 完整HB首发/尾段事务与共享入口（默认OFF，私有ON已编译，未游戏）

9272e97e后的exact-HB共享入口以GEARS_PLAYGROUND_HB_FINITE_BURST编译隔离，root默认未定义/关闭；公共v71、Nativec985和XR1684保持原制品。私有playground.cpp仅显式ON，已把首发捕获、原timer同一DeferredEvent、真实R2-up后新鲜Source、逐次射击scope及停止/失追清理接入，完整PG编译0，尚未stage或新game。Lancer旧分支及原100ms、20ms余量、250ms原事件期限保留；缓存只保存元数据/原press承诺，不续输入或pose时间。

核原包export37976确认HB外层BeginState为191 serialized/271 runtime、20对象引用/3个虚FName，先由原脚本BurstCount=0/音效反馈再super；不能假定super一定再次经过ProcessEvent。现改为整段外层原事件完整匹配并延后，内部super仍按原VM执行，无新hook/FFrame跳转/计数写。13种引用各自严格原包名/owner/类型，Bool mask/局部Name/函数布局与关闭读校验；旧Lancer/general script默认上限仍256，只有已匹配HB外层显式512容量用于271原串，未扩大旧门槛。574原串逐字节变异/截断/引用检查及实际Reader owned-memory用例通过；第一场新game仍须证明outer=1实际捕获/真实射击。

复核发现跨快照Property.identity/type是本地slot，原meta.same_property跨snapshot会误拒绝。已改按完整UObject注册身份、类型身份及next/flags/dimension/width/offset布局比对，真实Reader的不同发现顺序快照复现旧比较FALSE/新比较TRUE，错误注册身份/布局仍拒绝；不以slot相等替代身份。右手grip失追另行明确撤销有限尾段，因为frame_active本身不要求该手tracked；无输入DOWN伪造或不新鲜pose继续。原timer在6发末尾/真实无弹时仍经原ShouldRefire=false与BurstDelaying状态证明正常退出，不补造射线/弹药。异常范围退出保持已有GT输入/清理owner活着先释放R2/全PendingFire并原状态撤销，避免提前全局fault导致disable来不及收尾；失败单独有界记录，不将后续真实新R2按压当旧事件重放。

同一私有源最终受影响7个测试程序编译/执行0：ledger823、整段outer matcher574、cancel metadata fakes103、实际transaction fakes289、实际event adapter fakes118、真实Reader路径/布局/关闭及canonical cache68、原asset50+新增kind24。原HB timer adapter54和既有Lancer once-only/loop145仍0。不得相加称游戏覆盖；full retail certificate、公共dispatcher和engine在部分夹具是stub，真实tracking/VM/画面仍待新game。默认OFF整PG0、私有ON整PG0；hb-finite-profile-builds.json/hash保存。独立快照应用曾拒绝Stop/Longshot yield同hunk，随后只移除其三行外来增量再移植，Longshot/Boltok脏改未暂存/混进本私有binary。


## 实际65044：完整首发证书首拒绝，有限尾段未到达

冻结110627/PG0cb1248a（prepared SHA5e9efb76）在Snub110649/17548明确最终交窗后，独立idle、2251源/6制品/13入口hash核对，再于私有实例实际启动game65044 creation134386691257760825，Nativec985/XR1684、0.3后台observe-only/mute/物理隔离，公共v71不变。真实肩部取HB成功；第一真实R2发生native_hammerburst_first_proof_rejected outer=1 stage=1 bytes=0 parent_parameters=0，未取得任何authoritative HB trajectory。operator65804 exit1保留；没有到达原6发、真实UP有限尾段、耗尽守恒、换匣/R0/总零供弹，不称新事务已通过。原有Native总ammo在release后读到156；未另取本局typed弹匣/备用快照，不能从总数虚构分配。

58207 runner、56150配对review/encode、87081 retention均实际terminal0；Native cleanup/entries/pad均TRUE、unified drained1/handstop5/XR0/launcher0/game absent。config/settings逐UTF8字节复核、head原缺恢复。24.333333秒H2641920x660，全部1页paired overview及game00337/sim00338/game00382/sim00382四原图亲审；startup第三人称/等待卡/白色拾取HUD仍保留，后段枪靠右脸，完整手掌/袖口/全角度和头显fit不接受。retention删除1499媒体248444063B，最新3保留65044/17548/30188，48h；源文数值不删。完整记录analysis/hammerburst-20261108/runtime-65044-review.json与restore/PG原日志/ffprobe。所有终态及独立idle后已明确唯一交Longshot112230，再由其维护Coverr32；本聊天不复用窗口或原候选。

原stage1合并了完整原包路径、结构、参数/返回布局、extent、VM形状和脚本数组，尚不能确定某一条件为因果。下一增量仅把原谓词按原读取/短路顺序拆成阶段1..8，并为路径原有各节点/类型检查记录node/gate；有界首拒绝日志附已读extent/parameters/result，不新增游戏内存读取、getter、hook、输入、状态转换或放宽校验。79项实际Reader owned-memory检查（包括11项新首失败位置检查）通过，118项event adapter fake检查通过；初次event夹具编译因无Snapshot.failure成员失败已保留，最终日志不再依赖该stub缺失字段。此为观察修正，尚无拒绝原因修复或游戏成功证明。完整私有PG构建状态单独记录。


## 实际25860：首发参数区8字节，修正原Name参数证书与完整保留

观察候选115111/PGf17cb02e（prepared ebdf0792，源码090133e3）在Coverr32/36040全终明确交窗后，独立idle、2251源/6制品及19实际根入口核对，私有game25860 creation134386704155119374实际首拒绝：outer1 stage3 path_node4/path_gate9 extent8 parameters8。完整原路径/结构已通过，旧证书错误要求parameters0，因此首发未到。result0只是尚未读取的reset值，不是原返回offset。真实肩部取枪PASS、authoritative射线0、operator38380 exit1/runner94753 exit0，未测6发/UP尾段/耗尽或任何换匣边界。仍保存原release后总ammo156，不虚构typed弹匣/备用分配。

45029 review/encode与34663唯一retention实际terminal0；Native/entries/pad均恢复、unified drained1/handstop5、XR0/launcher0/game absent；config/settings逐字节/head原缺复核。全部1页paired overview+game00327/sim00328/game00352/sim00441四实际原图亲审，20.833333s H2641920x660正常；startup TPS/等待卡/白拾取HUD及完整手型/头显未接受保留。一次retention删916媒体146680782B，保留25860/36040/12980；独立idle后已最终明确唯一交Frag120000（114500同槽），Bol115420申请完整转真实尾，不自授新窗。

离线HBParameterProbe只读原包37975/37976，确认PreviousStateName为NameProperty、ArrayDim1、PropertyFlags原始数值128即0x80参数标记、Next空；不是旧反编译文本所写无参数/local。原包hash与前记录相同，原始字段及日志hash见hb-outer-parameters8-original.json。第一次相对Csc路径及Flags.ToString解析错误分开保留，改安全字段输出后parser0，无游戏调用。

先仅将测试基线改为真实参数8/原flag0x80，旧生产Reader与event适配器均实际RED(-1073740791)，分别卡旧参数证书及旧入队size0，保留before日志。随后生产证书严格要求ParmsSize8/Name参数0x80/width8/offset0/唯一子属性/关闭读，原VM shape也要求8；同一原DeferredEvent保留原调用的8字节，super继续由原VM执行。新测试检查非零Name双word完整保留、调用方随后变更和重复事件不替换原payload/原capture时刻/serial，实际原invoke适配器收到原8字节；空参数地址拒绝并完成原释放清理，0/4/16参数区或local/out/return flags仍拒绝。83真实Reader及127event adapter fake检查通过，生产GearsPlayground目标完整编译0；新增fixture曾有auto混合有/无符号编译失败，分开声明修正后0，日志保留。仍非游戏成功或整枪完成。默认OFF、公共v71、Nativec985/XR1684、100ms/20ms/250ms与单C0/原输入/原事件owner全部不变。


## 实际34836：原Name8与完整首发证书通过，保留事件尚未派发

121040/PG882ed93d/prepared e4478363/源6fec538e 在Bol115420/71888全终明确交窗后，独立idle和2251源/6制品/19入口/plan/SHA清单核对，私有game34836 creation134386730624676595实际出现native_hammerburst_first_captured outer1/event1/bytes271/retained1，原Name8、完整原脚本/引用/关闭读证书已被本局实际行使并通过。这不是射击通过：original_delivered记录0、authoritative射线0、operator47288 exit1，R2随后由原路径释放，原总ammo仍156，未typed分项；6发/UP尾段/耗尽/换匣/R0/总零全部未达。旧日志没有HB service/delivery/enter首等待点，不能归因到某个guard或计时。

60477 runner、53189 paired review/encode、91786唯一retention实际terminal0；Native/entries/pad/unified drained1/handstop5/XR0/launcher0/game absent，config/settings原bytes/head原缺恢复核对。20.633333s H2641920x660完整1页paired及game00300/sim00375/game00329/sim00436四实际原图亲审；startup TPS/等待卡/白拾取HUD、完整掌/指/袖及头显不接受保留。retention删797媒体124204776B、保留34836/71888/33016/48h，源文数值不删。全终及独立idle后已最终唯一交Frag122000，其后Snub122512由真实尾维护。

下一增量仅分开原service、delivery和enter的短路条件位置，并在首发等待时最多输出12条已有本地状态和原始Source时间POD。service输出在当前决定后、下一Native Frame复制前，delivery输出在当前决定后；无新增游戏内存读取、getter、Engine调用、输入/pose/时间stamp更新或新writer。attempted_source只作观察，不能用于许可/重放。元数据关闭/身份/Actor/当前Native/手追踪/枪口/按压计数及原100ms、20ms、250ms全部保持。291实际runtime adapter和138实际event adapter fake检查通过，新增区分identity拒绝、muzzle未就绪、手失追和真实时效余量拒绝，均不增加射击或替换原保留事件；原Name8证书83项不变复用。仅GearsPlayground生产目标完整编译0，不称原因修复或游戏成功。


## 实际64316：确认纯追踪来源无render回执，显式来源类型修正

125528/PGae6b43bc/prepared d4a2c8ce/源c51451f4在Bol125154/9816全终交窗后，2251源/6制品/19入口/plan及idle核对，实际game64316 creation134386764884888140再次完整271outer/Name8/capture通过。三次service0/validated1，deliver10/enter14：frame219/220/221的sync年龄46.412/50.1101/61.2629ms，stereo46.4173/50.1162/61.2671ms，sample19.7001/21.0114/24.7145ms，原event等待62.4915/133.7835/210.9481ms，render原值全部0。进入14前的身份、按压计数、原事件和BurstCount等均已通过，全部现有时间满足80ms派发余量/250ms事件期限；新账本将原有无render端点的已认证tracking-only分支错误当作未知来源拒绝，原因已实际定位。original-delivered0/rays0/operator72228=1，仍不称开火通过。

71572 runner、9651 paired encode/review、16461唯一retention实际terminal0；Native/entries/pad/unified drained1/handstop5/XR0/launcher0/game absent，config原bytes/head原缺恢复。原settings有UTF8 BOM，初次我方核对器把decode后的text直接encode而误拒绝，原settings raw SHA始终与备份一致；已用记录的原raw SHA复核且BOM完整保留，未改settings。完整1页paired+game00300/sim00375/game00341/sim00444四原图亲审，ffprobe正常；旧startupTPS/等待卡/拾取白HUD及全手型/头显不接受保留。retention删828媒体127441373B，保留64316/9816/28872。全终idle后已明确唯一交Snub131050，真实尾维护Longshot133536等，旧槽不复用。

新Source.tracking_only默认为false。仅正常GT派发先通过原ready_for（同Frame/当前held/registration/muzzle/20ms），且原sample_target的follow_target_known为真、render原值为0，才显式标记已有tracking-only分支。回调只在同一活动scope/Frame继承该类型，无新getter/读取。未知0仍拒绝，tracking-only与非零render冲突仍拒绝；存在render回执时继续检查其原时间，sync/stereo/sample的100ms及20ms、原250ms、全部身份/源/事件/counter/真实UP/防重放保持，不创造或续任何时间。

先把64316真实年龄和已认证tracking-only样本用于旧生产，ledger和event适配器分别实际RED(-1073740791)。修正后1517纯账本、297实际transaction fake、195实际event adapter fake编译执行0，覆盖两种来源的完整6发/真实UP续发/精确身份与counter反例，未知零/冲突来源/过期或未来必需时间仍拒绝；完整outer原Name8 payload在tracking-only路径交付也通过。生产GearsPlayground目标完整编译0，Native/XR/C0/defaultOFF/publicv71不变。

本次同时收紧现有HB core测试断言：原一次R2 .35down/.6up及原记录读取完全不变，已收集的有效ray必须恰好6条，0/1/5/7或无效六条均失败并保留实际数据；六组fake输入对原driver共30项检查通过，断言发生在真实UP之后，不补动作或弹药。该ignored分析源的原字节SHA记录于tracking-only-source-check.json，下一不可变prepared将保存其精确源码及19入口新SHA。尚无修正后Game或ammo守恒证明，仍需新一次既定core验收。


## 主人新增：核查是否存在枪栓/拉机柄（原资产核对结果）

主人要求“查下有没有枪栓，有的话做拉栓”。核对对象限定当前Reloaded实际装备的一代Locust_Hammerburst，不套用后续HammerburstII外观。重新读取原SkeletalMesh导出62424426/1238355bytes，SHA4b6bd78407481887e6efcd80532fd64b7108dce2ee6c05d259e19e8b44c0152b与既有清单一致；9576顶点/9966三角均按原骨归属，只有b_HB_Root、b_HB_MagLatch、b_HB_Mag三骨，无独立bolt/charging-handle骨。完整原对象目录同族只有该SkeletalMesh与独立Hammerburst_Magazine静态网格，无单独枪栓/拉机柄模型。

原HB_Idle、HB_Reload、HB_Reload_Fail三段导出全部重新核SHA，三轨均为Root/MagLatch/Mag；只有MagLatch有75/120旋转键，枪身与弹匣相对父骨的位移/旋转键均常量，无后拉/回位轨道。已将原LOD0位置/索引按原骨颜色画出两侧、顶部、底部及四分之三视角并亲看，未找到原资产独立外露可拉机柄。报告analysis/hammerburst-20261108/hammerburst-bolt-original-audit.json，模型侧视hammerburst-original-side-inspection.png；仅离线检查图，未替换资产。首选matplotlib不可用，改用已有Pillow/NumPy对原三角作软件深度排序，未安装依赖；初图正面/背面label不准确，另生成明确Side+X/-X及Top/Bottom图，结论用后者核对。

结论限定原模型提供的可操作部件，不宣称虚构枪械内部不存在枪栓；本版原资源没有可独立拉动的枪栓/拉机柄及对应动画，按已确认弹匣锁扣/翻盖与弹匣机制继续，不增凭空拉栓步骤。条件性拉栓要求已核查；不改变冻结135655的源码、制品、strict6 core范围或排队身份，首发/有限尾段/耗尽验证目标继续。

## 9896 startup 状态读取首败及HB私有修复

冻结135655实际9896在取枪/开火前失败：共同run_menu_firstperson_check.py:99读取同局session_report JSON遇PermissionError；不是游戏内存权限或输入准入拒绝。operator1、wrapper0，XR1/launcher1如实保留，零projection/零武器覆盖，不证明tracking-only或strict6通过。Native/entry/pad/drain与config/settings/head原始字节恢复通过；8.6s paired全1页+4实际raw已审阅，encode0，retention完整终态（删2592/456291670bytes，留9896/59768/61036），独立idle后最终唯一交Longshot142004。证据runtime-9896-review.json。

仅HB ignored私有run_hammerburst_operator.py在内存装载共同脚本后精确替换该一行：PermissionError/JSONDecodeError记累计数、最多4条首错误，sleep.02后继续原45s循环。每轮原live断言、原ready/error条件、原deadline全部保留，不缓存旧session、不发失败输入；未知IO继续原失败。共享脚本/Native/XR/PG均不修改。实际变换后的原loop经fake IO/clock执行8项：占用及半JSON后ready、永久占用/半JSON原期限超时、identityloss先于read、recordederror、原readygate、未知IO失败、正常ready。CPU0且无游戏/输入动作，check-startup-read-retry.py与startup-read-retry-check.json保存证据。新私有operator SHA01ea4c758427bfc00ced2bc14771d5ac6a6db669b937424eb6d8987553f55f8c；本次为driver修复，旧135655保持不可变，新候选另排尾，不复用已关闭窗口。

## 62728 原始首发与真实UP六连发通过（范围限定）

新144203 prepared f35d20f7 / PG56bc12d8，独立2251源/6制品/19root+AST与idle全核后实际62728 creation134386830742508421。原outer271捕获并在同帧真实tracking_only1/render0来源上交付；原计数1..6和6条同源有效权威弹道齐全。前3发r2_source_down1/input_owned1，后3发真实UP为0/0但仍以已提交原有限尾段完成，非伪造按下；after_release总弹药150，未取typed弹匣/备弹快照，不扩称完整ammo守恒。原pause/B返回通过。新增私有状态读取重试本局没有出现重试，不把本局当瞬态分支覆盖；其8CPU记录独立保留。

operator64452=0、runner40138=0、XR0/launcher0/gamegone、Native/entry/pad/drain及config/settings/head原字节恢复通过。26.733333s完整paired1页+8raw亲审，首/末发游戏与sim双端有实际持枪/开火/目标受伤画面；startup第三人称/waitcard与暂停卡保留，整手/手指/袖口/所有角度及实机接受未通过。encode14809=0、retention62202=0，删1511媒体252627541B，保留62728/33732/71440，最后独立idle。证据runtime-62728-review.json，原64316/9896等失败不撤销。全终后唯一交Frag145754（preparedf7bf5137/plan6e830bea），不追加本窗。下一项应另排真实尾、用现有finite-supply driver检验耗尽/换弹/零备弹/总零；当前没有这些新增游戏通过证据，publicv71/defaultOFF/Native/XR未变。

## 60196 finite-supply 在 Native 激活前拒绝（未覆盖供弹）

prepared153818/58a36d0c、PG56bc/Nativec985/XR1684在独立2251源/6制品/19root+AST、idle核对后stage164458。实际60196 creation134386875278469827在原anchor启动保护final_binding首败：before_stamp和strict_after_stamp均1086681247、final_stamp0、timestamp_only0、attempts1、admitted0。未取枪/发射/耗尽/换弹，不覆盖finite-supply，也不撤销62728的strict6通过。当前日志不能区分scene_binding_matches、末次capture_pawn_binding还是equal_binding具体子条件，不能将其称为单纯时间戳竞争或直接重试。

operator29628=1、wrapper70580=0、launcher1/XR1保留。native_started false、native_cleanup_verified false、entry_restore null、unified cleanup null如实保留，不称hand_stop通过；所存session-status属于旧59528，identity_verified false，不作本局证据。当前prime明确stopped cancel/disable_status0/active0/entries_match1，pad恢复，config/settings/head原字节通过，XR对象销毁/shutdown完成、游戏不存在。9.566667s完整paired1页+4raw亲审，仅TPS/等待卡；encode0、retention67340=0全终，删1617媒体269521220B，留60196/26480/59528；独立CIM/idle通过后最终唯一交LS160159（重新1596源/6制品/18root+reader/AST全核0）。证据runtime-60196-review.json。后续仅离线定位原final_binding子条件，不放宽identity/guard、不复用本窗、不重跑未变候选。

## 60196 最后绑定检查：私有只读细分诊断已编译，尚未运行

准确c985来源为equipment-controls-prepared-20261108-205200及其source SHA、CMakeCache-native、lancer-hover-handoff-source-before.json。独立副本native-binding-source基于1332保存源，只改anchor_observer.h/capture.cpp两个生产文件：保留原scene_binding_matches/equal_binding和原读取顺序、时钟、重试；失败后仅记录site/mismatch mask或已有PawnBindingDiagnostic字段，无新游戏读取、输入、hook、C0写者或Native ABI。可审核源码差异保存在hammerburst-native-binding-observation.patch，不改root共享两文件，也不混入新Lancer手型。

同一完整原guard夹具分别执行旧/新header，23335条Read地址/大小/返回逐项一致，两次原CPU夹具均0；新增断言区分final read_failure与原number变化的final_compare mask1。首个夹具误将后者预期为capture失败，已改正期望，失败日志保留；生产判据未变。最初归档全CMake配置因未收录continuous_hand_owner_test.cpp失败，未用测试桩掩盖。按原生成生产vcxproj精确提取7个C++/ASM源和有效编译定义、/MT，补6个git-clean原ASM并记录commit/SHA，复用hash-pinned原minhook.x64.lib及归档头；只构建GearsCapture生产目标，配置/编译均0，Native806dcbd6a92ecd337923f4ecf315e56ab7afd475e247a8a3cacdfe2ab30b8b0c。107原cache选项、有效定义、依赖SHA及补充源在native-binding-source-check.json/native-binding-build.json保存。未stage/run，不称60196根因修复或供弹通过；需新冻结与真实尾受理再验证，旧153818不原样重跑。

## 2026-11-09 实际44504：启动及核心六射通过，有限耗尽首败并闭环

HB171233 prepared81d07ec554d98e87148f7cb1ff96804395562bd56f694863662608c6f1cbf119沿Lancer165800明确最终交窗后，独立核2253 PG源、1338独立Native源、6saved制品、19实际运行脚本SHA/AST与idle，才stage/run。Native806dcbd6和PG56bc12d8保持冻结，不混入任何作者手型。实际game44504 creation134386943167237575，Native激活通过，不等于已证明60196启动问题因观察补丁修复。

肩部原取枪及一轮严格六发实际轨迹通过，六条trace_valid/write_source_consistent/trace_source_consistent均1；typed初始loaded72/reserve78/cap78、非无限。耗尽过程第6个真实press/release后没有进一步耗弹而首败，最后loaded58/reserve78，之前实际共消耗14发；停止后续动作。replacement、R0 loaded reuse、total0 empty_supply均未触达，不继承旧机械运行结果作为本轮通过。原日志可见第一组耗尽original_count仅1..5，后一次新press计数已出现；后续组也在count5或4时出现新press。原核心使用.35DOWN/.6UP，旧drain使用.35DOWN/.25UP，测试节奏差异已找到；不能把输入counter/新鲜度撤销当库存逻辑通过，也不削弱新press不可复用旧burst的防重放规则。

wrapper44619=0、operator44840=1，首败停；Native/entry/pad恢复TRUE，hand_stop_code5但drainedTRUE、fire_disable0，XR0/launcher0/gamegone。wrapper最终idle guard因当时录制收尾未idle而跳过文本恢复，原notice保留。独立idle后用本轮原备份恢复config9b5773b5、settings af500f0b（UTF8 BOM保留）、head原缺，逐字节SHA匹配预检；没有以settings_text去BOM后的ee74ffa9冒称原字节，也无依据归因其他会话写入。14份原PG/Native/prime/session/入口日志状态已归档。encode31486=0，ffprobe30.100000s/5191433B，完整2页与game00360/00441、sim00489/00637原图亲审；445game/642sim末端均覆盖，game最大采集间隔2133.3728ms是采集缺口，不是FPS或游戏卡顿结论。画面可见主手持枪、左空手及部分枪身近相机遮挡，本轮不是手型接受。

retention68003=0仅执行一次，实删1081媒体159600676B/errors0，保留44504/48324/31716、48h，原文/源码/数值记录保留。独立再次idle、3基线复核、Boom173042完整3954归档源/6saved/15实际driver+AST/1024归档py/86实际runtime依赖+AST、原game与review解释器及两outer guards均过后，明确最终唯一交Boom173042。Boom已回执受理并将原scope stage/run，随后唯一后继更新为HODcore4-query，旧core2/core3撤回。HB不再有旧窗，不追加本局测试。证据runtime-44504-review.json、runtime-44504-restore.json、runtime-44504-successor-Boom173042-check.json。

## 44504 后仅修正耗尽脚本完成窗口并收紧每轮断言（未再次游戏验证）

hammerburst_supply_boundary_chain.py改为与已观察到完整六发的core同一.35DOWN/.6UP，每轮typed弹量必须精确减少min(6,previous_loaded)，不再只接受大于0。部分/无进展均首败立即停，不重试输入；保留真实UP、18s总界、原128轨迹上限与129截断标记、库存/奖励不写与原所有身份/新鲜度/anti-replay规则。新增每轮前后loaded/实际spent/expected落盘，防止只保留最后一次快照丢掉早期部分burst信息。

10个离线fake用例通过，覆盖同core按放时长、9→3→0两轮真实按放、部分12→8立即停且不发第二press、无进展、timeout、备弹突变、无效/缺失轨迹、原128截断有/无marker和所有失败后的真实UP。此是测试脚本修正，不声称生产枪械修复或有限弹药链已通过。driver旧SHA b95f3bf5aec1a9b626ed55cd94dbbc3c57c5489d920c29d38fa2c5942bb298a5，新7aaca77db2c12677d0247fa8562f8373ae3f30b3791ddd37345b49f926b935bb；check_supply_driver.py SHA1928a97add18661b006487535dd89ce43cb579805215ac12806a3fa09d508e29。可审补丁docs/hammerburst-supply-driver-44504.patch、数值receipt supply-completion-window-repair.json。HB171233原冻结不改且已消费；下一验证必须另冻结身份、沿实际队尾申请，不复用已交出的窗口。

修正后的下一批已冻结为hammerburst-prepared-20261109-185428-completion-window-supply，prepared438d416c11bb11a4bb5f314b9595b2293117fcfc65a2d5cd73b6fedbd59df256；completion-window-runtime-plan.json bc08995f9953e44ad3066e15ebda101eac9de32db404b319cfde892b6a487726；completion-window-runtime-inputs.json 737b451557edc4c68155cdc20a37b0d3530a7bba90703b5c116418e5654d38f7。2254归档源/1338独立Native源/6saved/19实际运行入口SHA+AST通过，6二进制与44504完全一致；仅上述driver修正和离线检查/文档更新。未stage/run、未受理新窗，已向Boom请求实际当前尾定位，须排所有已受理早到位之后；不进入其已受理HODcore4-query直接继任位，不复用44504。未来check-completion-window-preflight.py需在明确最终交窗后独立idle再执行，目前只检查AST，没有在他人窗口中执行带环境基线采集的preflight。

HB185428新位已由实际尾Bol180947正式全核单槽受理，证据analysis/boltok-development-20261108/successor-hb185428-check.json；当前只入队、不授窗，仍未stage/run。其后HB独立全核并正式仅受理Boom191450（prepared fb48f5c4bf5718215b39f6d80beda0cb05297e7aa530662fc3dcd6cc0bf73300 / ready7d60b4f410c7c3daf8cd23effb36f24b98d3beb72d9b4011fcfd190e5c8b357c / plan ded6e6d1aee081c4bebc8123cccf46f1c40c3b9c859e45ef1efb29ec43c4d743），不是旧173042。3955归档源/6saved/PG1348和Native1583源码snapshot逐项/15当前driver+AST/1024归档py/86实际runtime+AST/2guard/原game与review解释器/build及49864失败receipt全部核过；snapshot含工程和测试文件，不称全MSVC系统依赖闭包。相对原snapshot仅变Boom自己的muzzle_clear与muzzle_schema两个文件。正式链Coverr54→Bol180947→HB185428→Boom191450，前序全部早到位保留。HB必须等Bol新完整收尾后的明确最终交接，不能复用44504；之后本枪也必须运行、审片、编码、精确恢复、保留和独立idle全终才唯一交Boom。记录successor-boom191450-check.json/current-candidate-state.json。

## 185428实际50148：严格耗尽通过，随后原翻盖真实骨姿首败（全枪未通过）

收到Bol180947/game18152完整新最终交窗后，逐SHA核handoff/review/restoration/retention/successor凭据，独立CIM/idle空。2254PG+1338Native归档、6saved和19当前输入/AST复核0；按原185428身份stage，prepared438d416c/PG56bc12d8/Native806dcbd6不改。运行前260774b7已保存本轮作者源码；另新鲜保存10项即时原字节基线（manifest/config/settings/head、3private bins、public state、2EngineSettings），不复用旧基线。新手型与获取换算均未接入本运行，public不变。

实际game50148 creation134387043892654996、source1模拟。真实肩取及原核心6条射线通过。其后12次真实.35DOWN/.6UP，各typed精确消耗6，loaded72→66→60→54→48→42→36→30→24→18→12→6→0，reserve全程78、capacity78、非无限；72条实际trace_valid/write_source_consistent/trace_source_consistent均1，与核心合计78条。这次证明修正driver完成窗口后首轮真实耗尽成立，未削弱anti-replay、100/20/250ms或原128导出上限，未用计数/库存setter。

随后reserve_consumption_prerequisite换弹首败，8秒未进入to4 original magazine。cycle前typed0/78，但opening日志为loaded78/reserve0、serial0；该变化的原调用/时序原因尚未证明。逻辑1→2→3、期望angle.822637367，prepare_feed连续8次真实bone readback约0，触发原line383 cleanup并3→0，不能把逻辑opened当原模型已打开。原匣提取/替换座入未完成，R0 loaded reuse、末66发耗尽及total0拒绝均未触达。保留完整失败，不不变重跑185428；下一窄查是零loaded前后库存/原reload与visual_clip选择、原动画真实骨姿发布，不放宽已保留的身份/租约/原骨姿验证。

wrapper79361=0只是生命周期正常，operator29164=1是实际功能失败；Native/entries/pad restored TRUE、hand_stop5/drained TRUE/fire_disable0、XR0/launcher0/gamegone。27份原PG/Native/prime/session/入口文本与5份现有numeric449504B归档后，独立idle按本轮10项原字节恢复并再次逐项SHA一致，public只读核不写；settings BOM保留。完整63.3秒双路3页overview与game00375/00496/00639、sim00511/00746/00981六原图已亲审，保留startup标题/TPS/等待卡、后续第一人称/实射及近相机遮挡；未看到原匣提取完成，不接受完整手型。857game/1352sim均覆盖末端，最大采集gap1833.3101/233.329ms不是游戏FPS/停顿结论。

编码78985=0，ffprobe H2641920×660、63.300000s/11763129B；唯一retention8700=0，实删1414媒体251779531B/errors0，保留50148/18152/41460、48h，源码/文本/数值保留。新鲜独立idle及10项原字节再次全核0，Boom191450完整3955归档/6saved/1348PG+1583Native snapshot/15current driver+AST/1024listed archive py/86actual runtime+AST/2guards/game+helper/build与旧失败receipt均再次核过。之后仅最终唯一交Boom191450，不能把仍排队的LS等当HB第二后继。证据analysis/hammerburst-20261108/runtime-50148-review.json、runtime-50148-restoration.json、runtime-50148-final-idle-baselines.json、runtime-50148-retention-result.json及successor-boom191450-check.json。
## 2026-11-10 game51936：首次实测定位到当前帧过期，完整流程仍未通过

独立候选 hammerburst-prepared-20261110-latch-history-press（prepared98b9a1ee、PG305b5447、Native806dcbd6）实际运行后，核心6发和前六轮耗弹各6发通过，第七轮36→33仅3发，合计45条有效且来源一致的射线。尚未到空匣、换弹或历史轨迹分支，不把该分支称为实测通过。

原日志第468行记录 frame303/sequence1868 的 native_equipment_frame_read failure=5、input_age_ns=103046300。对照该候选冻结的 weapon_hand_client.h，5明确为 stale_before；copy_frame_on_gt 在最初 fresh 检查拒绝并撤销发布。紧接 continuation gate1/no_frame、committed count3、firing_debt1、cancel0，随后取消并清理。该证据证明缺少当前帧导致连发中断，不证明输入年龄具体耗在哪个阶段；已有稀疏 PlayerTick 耗时记录没有覆盖失败帧，不能据此归咎原游戏更新、元数据读取或调度。

沿冻结源码确认：Native共享选择绑定同一Scene及Stereo，rig继续消费该选择；PG装备读取在原PlayerTick之后，读取失败会撤销当前动作/持枪发布，R2随后没有当前帧。继续检查此链上的时间消耗和原始输入选择，保留100ms时效、20ms派发余量、250ms原按压窗口及原时间戳。不得以旧姿态补射、续期时间戳、忽略身份丢失或第二个姿态写入器规避失败。

本轮操作者exit1，wrapper0只代表生命周期收尾正常。运行的Native/入口/输入/XR/launcher正常收尾、10项原字节恢复已核对，运行锁已释放。离线编码session35690已终止exit0；两张完整paired overview与按失败QPC前/当时/后各取双路共6张原图均亲看。原图保留近脸枪身、右眼严重遮挡及相邻采集姿态差异；不接受完整贴合，不从画面反推精确输入失效时刻。新增18份原运行sidecar归档，未占用共享游戏窗口作离线审片。

证据：analysis/hammerburst-20261108/runtime-51936-first-failure-analysis.json（含解析的5条拒绝、原日志SHA、6份冻结源码SHA、6张原图QPC偏差及18份sidecar SHA）。完整拿枪→开火→耗尽→换弹→再开火仍未完成；不变候选不重跑，先针对已定位的时效链推进。

## 2026-11-10 时效链窄记录已编译，未进游戏

针对51936已证明的stale_before，HandClient仅保存成功commit前最后一次现有fresh检查的now，现有copy失败记录附带commit_check_input_age_ns及since_commit_check_ns。它们分别描述该检查时输入已多旧、到首次读取又经过多久；不将该检查时刻称为精确发布完成时刻。沿用原时钟读取，不新增clock、游戏观察、输入读取、Frame、Native写入器或准入分支，不改变100/20/250ms。无当前发布时字段保持-1；后续独立真实commit替换观察值。该变化用于分段定位，不是少发修复。

由实际51936候选source独立复制，仅3文件变化（client头、client测试、装备失败log），逐文件SHA比较确认未带入其他武器未完成修改。完整PG编译exit0，DLL cd28c194374223496efd19ac9de38fb75ccb6ae601a5b31ee33efa7734ca0c55。初次生成独立副本时字符串替换误落在更早的support日志，被MSVC格式检查阻止；已从不可变父文件重新生成正确限定修改并编译通过，失败产物未使用。

冻结副本WeaponHandClientSourceStateTest编译/执行exit0，新增8组覆盖单手与support、新鲜/延迟/过期、拒绝不触发observe、发布消失不保留可用时钟及新发布替换观察值；其余既有客户端用例全部通过。证据analysis/hammerburst-20261108/publication-age-isolated-r1/result.json、unit-result.log、build-final.log；共享backend尚有他人改动，仅将本次独立日志差异保存为docs/hammerburst-frame-age-observation.patch，不整体提交该共享文件。未获取运行锁、未stage或启动游戏，完整换弹与再开火仍待验证。

## 2026-11-10 game40928：同一次连发第六发晚于首次库存检查，修复测试等待

publication-age-r1实际game40928、creation134387732759989314、原指定游戏，PG86dc3b36（保持原GearsPlayground6DofUnified3输出名；此前cd28c194是默认输出名的离线构建记录，不是本次装载DLL），Native806dcbd6。Release编译选项与上一候选逐项相同，源码仅3项观察/测试变化。源、入口、计划均在45ac21c9保存后，公共RuntimeGuard实际获取窗口才消耗新一次性claim并stage。

实际核心6发、耗弹第一/二轮各6发通过，第三轮首次typed观察60→55，于84545563711000记录；随后84545579881300出现第24条有效且写入/射线来源一致的实际射线，相差16.1703ms。这是该轮第六发，最终实际24条均有效一致。原操作者在首次5/6观察处已判失败，exit1；没有补按/重试。整局没有continuation/release拒绝或frame-read失败记录，所以本次没有练到新增年龄记录，不能将本次诊断写成51936的输入过期。也不能因后续第六发出现而把原失败运行改判通过。空匣/换弹仍未到达。

针对这个实际测试缺陷，耗弹driver现在在同一次原始按压得到部分完成时，等待该次连发的实际库存进展；仍使用原整段18秒deadline，不续期，不补按，不放宽原Native/首枪/时效/派发/anti-replay。新出现的明确连发拒绝立即失败，库存增加/储备或容量变化失败，无耗弹保持原立即失败。等待中逐次保存真实typed观察，完成后仍要求精确min(6,loaded)消耗及有效射线。新增9组离线检查覆盖延后完成、重复部分读、原拒绝、原deadline、无进展、库存增加、储备变化及过量消耗；原10组耗弹检查也通过。只保存本人driver差异docs/hammerburst-same-burst-completion.patch及检查源，未重编Native/PG或不变重跑游戏。

wrapper36320终止0，操作者54992=1；原Native/入口/输入/XR/launcher正常退出，十原件SHA全部恢复，共享运行锁已释放并短告知实际使用方Bol。26份初始原运行文本/sidecar、原PG日志与动作记录独立归档；停止capture以硬链接独立保存。编码29645终止0，一页完整paired overview及失败QPC前/中/后双路6原图均亲审；保留启动标题/TPS/等待卡、枪口火光和严重近脸/右眼遮挡，完整手型仍不接受。旧媒体保留清理尚未执行，不冒充已完成。

证据analysis/hammerburst-20261108/runtime-40928-first-failure-analysis.json、runtime-40928-evidence/、same-burst-completion-check.json和supply-driver-check.json。下一步冻结仅此driver等待修正后继续同一完整空仓换弹链；51936真正过期失败仍单独保留，完整开发目标未完成。

## 2026-11-10 game57052：明确拒绝即止步，修复持枪关联丢失后的原定时器漏拦截

same-burst-completion-r1 preparedf04504cd、PG86dc3b36/Native806dcbd6，入口与冻结计划在51ef8bf9保存；公共锁等待后实际运行game57052 creation134387740078685230。核心6发与第一轮耗弹6发通过。第二轮frame249装备copy已拿到当前源，随后held_chain_observation返回unavailable，当前持枪关联清空；R2无当前帧触发release gate1与count2取消。新driver遇明确拒绝立即停止，没有把取消当作可继续等待，更无补按。

本轮仅14条实际有效/写入来源一致/射线来源一致的导出射线，最后typed loaded61/reserve78，即库存累计使用17发，存在3发未解释差额，必须保留为失败。较早frame63确有stale_before：commit_check_input_age_ns96280400加since_commit_check_ns4159300等于input_age_ns100439700；它属于先前启动阶段，不得当成frame249的因果证据。frame265后另采到装备工作81.5161ms，但不是frame249精确分段证明。不得把全部差额或全部少发归因于已知输入年龄。

源码追查发现具体缺口：公共refire路由先要求HB targets匹配当前held_binding，HB defer自身也要求相同；一旦当前关联清空，仍欠正常清理的原RefireCheckTimer会返回false，可能落回stock、脱离本次来源一致射线适配。用实际事件适配器fixture将same_hold见证置false后，新回归在旧defer处明确失败；初次仅清空假binding不会改变fixture固定的same_hold，已纠正fixture后才采用该复现，不把第一次断言错误当作产品证据。

窄修复：公共路由始终让HB适配器先判断；适配器仅对已认证证书、准确武器地址、准确原timer地址、未清债务、原GT、当前对象/函数身份仍通过的回调保留拦截。当前held关联失效时只撤销并消费这个原回调，不捕获新事件、不执行引擎调用、不写弹药/计数器/定时器，不提供新Frame或开火权限；原上下文/注册/时效/20ms/250ms验证仍约束任何实际派发。不同对象/函数、无欠债、非原GT等保持不误拦；注册失效不允许该旧债回落stock。最多8条owned_timer_blocked记录用于之后确认实际分支是否练到。

独立副本相对本轮冻结源仅事件适配、公共路由和事件测试3文件变化，完整PG编译exit0，最终DLL714db1bfd2f62ba090e93a2edd1d45eefd462de58d591b7b667263492dd1ff18；冻结副本实际事件适配280项fake检查exit0，包括丢关联、重复旧回调不建新事件、错误对象/函数、注册丢失、无债、同GT正常清理与既有连发路径。证据analysis/hammerburst-20261108/retained-refire-target-isolated-r1/result.json及retained-refire-target-check/frozen-result.log。此修复尚未进游戏，不声称实际3发差额已解决，也没有修复导致持枪库存观察失效的耗时链。

运行入口29256终止0只是生命周期正常，操作者失败；Native/入口/输入/XR/launcher正常收尾，十原件SHA全恢复，共享锁已释放。27份原文本/sidecar、PG日志、动作记录及停止capture独立保存；编码93073终止0。一页完整paired overview及6张事件QPC前中后原图亲审，近脸/右眼遮挡持续；simulator00575右眼出现大片白色几何，作为未接受视觉缺陷保留，不推断根因。旧媒体清理仍未执行。空匣/换弹/回射与完整视觉接受仍未完成。下一次仅带本次窄修复继续受控验证，先核对取消后弹药与射线是否一致。

## 2026-11-10 game55756：未持枪按压读取晚于库存观察，提前同帧读取已编译

retained-timer-r1 prepareddcefb581/PG714db1bf/Native806dcbd6，入口计划5b771360保存后实际game55756 creation134387753202710275。肩取未取得held receipt即操作者失败，0条实际射线；没有练到旧timer拦截修复，更未到耗尽/换弹/回射。wrapper38445终止0仅代表正常生命周期，Native/入口/输入/XR/launcher正常结束，十原件SHA恢复全通过，共享锁释放。27份原文本/sidecar、动作记录、PG日志独立归档，停止capture独立硬链接保存；编码62983终止0，一页完整paired overview及6张失败前中后原图亲审。右手靠近右肩/头侧并返回，未见持枪/火光，不能称为取枪或手型通过。旧媒体保留清理仍未执行。

实际frame103 input_gap=producer_sync_gap，前后选中原sync间隔112.3757ms，读取时最新样本57.5027ms、实际记录按压121.0891ms；原Frame.now到读取38.7007ms。当前源码对未持枪输入先做完整库存观察、后调用next；原有持枪路径已提前采样。不能仅凭这些数值把38.7007ms全部归于库存，但该顺序确实会消耗按压的100ms解码有效期。更早frame35的failure8并非该按压失败证据，独立保留。

窄改Runtime：未持枪和持枪都在同一同步tick的库存工作前采样现有输入，随后只消费这一次局部结果；pending/future路径保持原处理。原完整库存/身份/附件/当前Frame/250ms原事件/提交检查不变，任何失败仍丢弃局部结果；不保存、重放或续期按压。现有只读简化库存路径仍仅用于原先已发布持枪的quiet状态，不拿它替代未持枪的完整库存或改变idle视觉条件。

从实际冻结父源复制，只改Runtime头和私有测试2文件；未覆盖root共享测试中的他人修改。新增8场景在旧顺序先明确触发输入时钟断言失败；新顺序下实际Runtime/模型共128场景1639检查通过。覆盖不同库存耗时、过期、库存拒绝、身份变化、当前性丢失和目标识别失败，保留完整库存调用、原创建时刻、失败无动作且后续无局部事件重放。私有测试差异保存在docs/hammerburst-unheld-input-order-tests.patch。完整PG编译exit0，DLLae1f81c5100411aebb60c7378816771cc91403847bf5081664e3bd1eaa4124f0；尚未进游戏，不能称取枪失败已修复。

证据analysis/hammerburst-20261108/runtime-55756-first-failure-analysis.json、runtime-55756-evidence/与unheld-input-order-isolated-r1/result.json。下一轮仅带提前读取改动继续原完整流程，同时保留旧timer拦截修复及其实际取消分支尚未验证的状态；全枪目标仍未完成。

## 2026-11-10 game56252：空仓机械闭环和六条回射已记录，最终库存采样过早

unheld-input-r1 prepared76a2ae8c/PGae1f81c5/Native806dcbd6，fc2f21a7保存入口后实际game56252 creation134387765681624264。肩取、核心6发及12轮各6发真实耗弹72→0/reserve78通过；空仓真实扣扳机保持0/78、无新射线。实际原begin serial1、1→2→3→4→5→6→7，新匣入槽后78/0；首次实际使用原raw-history latch路径（gate8 used1，多组112/111ms跨段），随后7→8 angle0.00606436423，原reload_complete result2/78/0。此处不是逻辑阶段替代真实机构，现有实际原骨/写入ACK检查保持不变；不等于全表面贴合。

回射driver仍在固定.35DOWN/.6UP结束即取首个样本：五条回射、typed73/0 observed87881907941200，因此外层期望72/0断言失败。随后第84条实际有效且来源一致的射线在87881923572500记录，即该次第六条回射晚15.6313ms。整局84条全部trace_valid/write_source_consistent/trace_source_consistent=1，没有HB continuation/release/frame-read拒绝，也没有owned_timer_blocked记录；旧timer修复的取消分支本轮仍未练到。没有新的typed最终72/0样本，不能追改原操作者exit1为通过。

窄修复只在reload测试driver补入observe_return_burst：保持一次原按压，复用原closure/completion的8秒截止时刻，不新开或续期等待；要求准确min(6,loaded)实际射线及typed库存消耗同时到位，字段/有效来源全核。新拒绝、旧timer拦截、库存增加/异常、过量射线和超时立即失败；不发新输入，不补按、不改库存或生产保护。12组离线检查通过，覆盖射线/库存各自先到、延后第六条、明确拒绝、读取本身越过原期限和异常值；原10组耗弹检查也通过。实际源码及docs/hammerburst-return-burst-observation.patch保存这项差异，PG/Native无需重编。

wrapper97983终止0仅生命周期正常，操作者48544=1。Native/入口/输入/XR/launcher正常退出、十原件SHA恢复全真、锁已释放。27份原文本/sidecar、PG与动作记录独立归档，停止capture硬链接保存；编码41651终止0。完整3页paired overview及原history-close/关闭后库存/第六条回射双路6原图亲审：机械变化与火光可见，但近脸/右眼枪体遮挡和掌指腕袖口贴合仍未接受；不能从图像代替最终72/0数值读数。旧媒体清理仍未执行。

证据analysis/hammerburst-20261108/runtime-56252-first-failure-analysis.json、runtime-56252-evidence/、return-burst-observation-check.json。下一轮只带同一次回射观察修正重新读取最终准确库存，然后继续其余完整需求；不开新枪、不把本次阶段进展缩成全枪完成。



## 2026-11-11 私有画面时效连续性候选（实测前保存）

58164原Native记录47次stale，其他拒绝类别0；原Gate对同参考系的纯年龄过期保留连续性，外层却无条件撤销手状态。私有Native71f280b0仅在原Gate前后同稳定epoch、无pending、focused且原接收时间确已超过100ms时保留逻辑等待；原画面拒绝与真实失跟踪/身份/时钟错误撤销仍保留。配合PGb7dcb9ec将已验证的真实UP后WAIT接回原回调，原250ms事件截止、100ms来源和20ms派发余量不变。17项原Gate检查与457项实际适配器检查通过，完整Native/PG编译通过；尚无此组合的游戏证据，不宣称修复成功。公共v71与手动入口不变，全部13项要求继续保留。


## 2026-11-11 实际45516：有限弹药测试窗口截断

Native71f280b0/PGb7dcb9ec实测72条一致射线，core6加11组六连发，最后完整库存6/78。第12组DOWN仅87.6207ms即被原18s整段测试截止收尾UP，首baseline至末UP17.99699s，未到空仓。不是产品超时修复通过；Native年龄分支27次实际命中均在开火前，input_loss0，连发中间断与UP-WAIT仍无实测覆盖。单独修正私有测试适配器为36s整段窗口，DOWN前至少留3.5s，否则不按下；原产品100ms/20ms/250ms及单次首发观察不变，12项回归通过。正常清理全0、无forced、316帧/无捕获错误/编码0，3页48对全跨度影像亲阅，近右眼巨枪/手分离/白几何仍不合格。全部13项保留，不算整枪通过。


## 2026-11-11 实际6896：严格prepare后输入拒绝

同Native71f280b0/PGb7dcb9ec，修正后的36s测试窗口实际剩余35.4076621s，核心6条一致射线后，Native原prepare367因flags13缺input_matched、controller序号/时戳0、原Stereo年龄100.2764ms拒绝并撤销input。外层年龄分支93次保留不覆盖严格prepare。其后新鲜UP375、DOWN383实际status1/owned0，384明确blocked8，未开火；不能把quiet当已消费DOWN，已修正测试读取仅承认status3/owned1或明确拒绝8，14项检查通过，无补按。尚未证明该新DOWN拒绝的具体转移原因，下一步读实际Input状态流，保留严格时效和防重放。正常清理全0、无forced、170帧/捕获错误空/编码0、3页48对亲阅，手枪贴合仍失败。整枪和空仓仍不通过，全部13项要求保留。


6896后续精确转移已查明：OriginalR2Status1是baseline，非quiet；UP375/376后，实际copiedFrame377在PG读取时101.8667ms过期（source_status0/continuity_failure10），Input据此seal。下次新鲜Frame383已经DOWN，按原防重放只作baseline1，384blocked8直到真UP。这是预期安全转移，不再推测Input状态机错误；下一步只查原Scene发布至PlayerTick消费的时效/耗时，不补按、不放宽拒绝、不原样重跑。


## 2026-11-11 单实例36524：空仓和换弹通过，回射窗口不足

按用户原模拟器协调授权，四工作会话临时停止新增game、现有运行自然结束；14:11:59精确worker/game/XR/编码/构建均空闲后，单次原有限空仓流程。Native71f280b0/PGb7dcb9ec不变：78条一致射线、库存0/78、空扳机无射线和原veto消费、普通换弹关盖78/0通过；回射前原8s关盖完成测试窗口仅余0.8467882s，预算检查未发送DOWN，整流程仍失败。原8s被复用于返手/新鲜UP/回射，现仅私有适配器保留关盖8s，返手前一次性建立独立12s观察窗口，不在按下/部分射线后续期；产品100/20/250ms和输入次数不变，12项原库存回射检查及8模块编译通过。Native年龄31次保留、prepare958因未匹配input拒绝1次（Stereo14.7082ms，不得称stereo过期），单次对照不证明并发根因或修复。正常清理全0、无forced、637帧无捕获错误/编码0，3页48对全跨度亲阅，近眼巨枪/手分离/白几何仍不合格。已通知统筹解除让行，不再追加本次单实例测试。全部13项保留，回射72/0与整枪验收仍缺。


## 2026-11-11 普通准入4400：首次实际seek WAIT后历史拒绝

普通准入等96.875s后启动，与Lancer共用原2实例上限；Native71f280b0/PGb7dcb9ec未变。78条一致射线、0/78空仓无射线/veto通过。开盖1→2，568原历史108ms分段接受后，seek因派发前过期返回false，首次实际wait1/no_dispatch1/seek_pending0，将纯stroke/history回退到162870638；569原history gate6失败，回退后跨度245ms，原gap reason5拒绝并cleanup642。不是成功恢复，未到弹匣提取/新匣/关盖/回射，新12s回射窗口仍未实测。下一步用实际纯核复现已验证历史被回退丢失的覆盖问题，区分观察轨迹与真实机械角，保留原6点/100ms/同身份/真实释放保护，先覆盖静止待提交端点。正常清理全0、无forced，445帧错误空、编码0、3页48对亲阅，视觉仍不合格。全部13项保留，不原样重跑。


## 2026-11-11 已验证观察与实际角度分离候选（尚未game）

4400精确previous序号4679、已验证4683、下一4689：原MotionHistory容量实际8（先前六点描述不准确，六是本次后续跨度），回退后delta10必被原build拒绝，保留观察后delta6可满足原范围。新私有PGb03ad129保留已验证但未派发的纯stroke/history，seek失败时实际angle/stage不动；下一新鲜同Grab帧仍走原setter。静止的待提交关闭/中途/打开端点由原纯核读取已观察结果，已提交端点不重复seek；失跟踪/松手/token/epoch/边沿变化保留拒绝。12项实际谓词/调用块、实际relative/history/stroke与静止调用块检查、原reload/history测试通过，完整PG编译通过。中间轨迹位置/时间是有界合成夹具，不是实际完整producer ring或引擎setter证明；尚未game。Native71f280b0/XR/公共v71不变，全部13项保留。


## 2026-11-11 实际38256：开盖登记在来源撤回后丢失

PGb03ad129/Native71f280b0原普通准入完整空仓流程，78条一致射线和0/78无射线/veto通过。开盖仅1→2，未到打开3/原弹匣4，cleanup时实际角0.00908119557。首次捕获stale_before581仍magazinelease3，587无lease，590game_difference10；Nativeinput_loss0、外层age48次保留。新观察保留WAIT没有命中，不宣称4400恢复修复；后续to4等待8s超时，651正常cleanup。源码unavailable_frame目前只保护new_mag ammo，其余清held/stroke，下一步用原Client/Native-owner复现opening/closing同一latch登记在pose撤回后丢失；只在证据支持的状态保留逻辑incumbent，不返回旧Frame、不派发动作/续时，不拓展old_mag，真实UP/失跟踪/身份与mount仍由下一独立新鲜prepare/commit处理。正常清理全0、无forced，523帧错误空/编码0、3页48对亲阅，贴合仍不合格。全部13项保留，新12s回射窗口仍缺实测。


## 2026-11-11 已观察latch登记保留候选（尚未game）

38256的opening同一latch登记在stale_before后清空、Native无input_lost，已用实际旧unavailable函数+实际HandClient/sole-owner适配器模型复现下一fresh帧lease_lost。私有PG5b3d71fb仅对opening/closing、未released、stroke已active、原lifecycle的magazine lease，在publication/stale_before保留逻辑登记；new_mag旧保护不变，不拓展acquiring/old_mag/其他失败，不产生Frame/动作/时戳。下一prepare/commit仍自行检查当前输入/token/追踪/身份/mount，原stroke/history连续性不变。60项附加helper/Client/owner夹具通过，旧函数丢失与新函数same lease retained、realUP release均复现；反射引擎/真实接触/完整broker仍非此夹具证据。完整PG编译通过，Native71f280b0/XR/公共v71不变，尚未game，全部13项保留。


## 2026-11-11 实际38380：首次明确原弹匣提取间断

PG5b3d71fb/Native71f280b0完整有限空仓流程，78条一致射线/0/78空仓无射线veto、开盖1→2→3→4通过。原弹匣提取613序号4754深7.76483857mm接受；614序号4759、111.4423ms间隔、深20.643711mm首次被原100ms提取限制拒绝并reset，之后到75.011mm均accepted0/complete0，真UP按未完成提取规则cleanup491。与47240的未证明猜测不同，此次有逐次原observe证据。latch pose-gap与observation WAIT均0，不能从普通开盖通过宣称其因果修补成功。下一步仅用已有producer原中间点分段验证OLD extraction，复用现有new_mag投影/身份/控制/映射，逐点原100ms和速度/对齐检查、trial至完整当前端点后提交；不插值/不扩大间隔/不造Frame或grant，不因本次间断拓展old_mag登记政策。Nativeinput_loss1、agekept28/29另存原记录，非提取根因替代。正常清理全0、无forced，514帧错误空/编码0、3页48对亲阅，贴合仍失败。全部13项保留，完整换弹回射仍不通过。


## 2026-11-11 原生产者提取历史候选（尚未game）

私有PGedccad22在old_mag阶段复用现有producer ring/投影/同context与控制检查，按原序号和时戳生成原记录对应的FeedSample（不插值）；原Extraction内部最后已接受sample须与previous逐项匹配，每段序号连续且仍<=原100ms，每点运行原几何/速度/起点/完成规则。试算副本只在全部点和当前真实端点匹配后提交，缺失/畸形历史仍走原直接observe拒绝。原插入advance实现不变，不拓展old_mag登记，不造Frame/lease/动作/新时钟。32项新检查包括38380端点111.4423ms直接拒绝和真实原投影/advance调用块，另原magazine与21项insertion-history通过；中间点/时间为合成夹具，非game历史。完整PG编译通过，Native71f280b0/XR/公共v71不变。下一次原完整空仓流程须记录actual extraction_history gate/used/points与原提取完成；全部13项保留。


## 2026-11-11 实际30496：原首发事件到期，提取历史未触达

PGedccad22/Native71f280b0核心6条一致射线后，首次drain原ButtonPress362已调用并捕获事件，未产生首发。FIRST service_loss明确gate4（不是report中的service_stage3），捕获165084982066600至拒绝165085244213900为262.1473ms，原250ms事件到期；先前delivery8在137.0426ms时缺新鲜同帧20ms枪口余量，withdrawal363100.8265ms。Nativeinput_loss0。修正初步record_loaded解释把阶段3误当拒绝gate3的错误，以原gate4为准；count6属上一完整cohort。新提取/登记/WAIT均未到，不判新改动回归或修复。下一步仅在原GT路径用现有PlayerTick分段时钟记录有界被动tail，匹配首发等待实际耗时，不归因未匹配的稀疏34.98/47.79ms样本、不延长原期限/补按/原样重跑。正常清理全0、无forced、158帧错误空/编码0、3页48对亲阅，贴合仍失败。全部13项与实际提取历史覆盖仍保留。
