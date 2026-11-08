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
