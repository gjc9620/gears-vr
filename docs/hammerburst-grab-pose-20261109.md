# Hammerburst 逐部件 GrabPose 与吸附验收

按主人 2026-11-09 追加要求及 [逐部件合同](weapon-development-20261109/grasp-and-snap.md) 执行。位置、角度、接触范围由本枪根据原模型自行标定；不能用通用握拳、机构成功回执或目标矩阵代替皮肤贴合。本文是当前缺口与实施清单，不是已完成证明。固定骨长、公共手部仲裁和唯一 C0 writer 保持。

## 当前验证域更正：1395不是整只手的完整表面

前面记录中把1395三角称为“完整手”的表述撤回。它们是原decoder按每顶点bones11..26合计权重>.999筛出的严格手骨域；原36637三角中，只要任一顶点含手骨正权重就有1891三角，额外496涉及Forearm_L10、ForeTwist2_L29及掌/指11/21/24/25。真实全部四影响必须保留，不能因混合权重把掌根/腕面排除。旧计数保留作原范围的历史观察，绝不是全掌、腕、袖口接受；当前没有已发布/已验收HB专用手型。下面新增工具和后续完整域检查优先于此前“完整”措辞。

## 实际交互件与姿势清单

| 部件／所属手 | 实际几何和身份来源 | 要定义的掌、指、腕与过渡 | 当前证据与缺口 |
| --- | --- | --- | --- |
| 主握把，右手 | 当前原 HB 实例、实际组件与主握挂点；沿原主手持枪绑定 | 掌贴握把背部；拇指绕侧；中／无名／小指包握；食指区分待机与真实扳机；腕和袖口随原挂点；取枪／收枪过渡 | 62728 实际双眼可见主手持枪和射击，未取得正侧斜角全部接触面证据。现有公共 weapon 手型不能直接算本枪专用标定完成。 |
| 护木第二握点，左手 | 本枪护木几何与现有 support socket；原物理控制器作为测量来源 | 掌心贴实际支撑面，四指分别绕下缘，拇指沿侧面，食指无扳机权限；Snap／Grab／UP 回 Snap 的姿态连续，退出范围回物理手 | 70816 已验证被动 Snap 不控枪、Grip 控枪、UP 退回跟随。完整掌／指／袖口表面和所有角度仍未验收；不把新 Lancer 专用 profile 套到 HB。 |
| 弹匣卡扣／翻盖抓点，左手 | 原 b_HB_MagLatch 实际骨矩阵、原旋转轨道和本枪接触点；盖与卡扣属于同一实际机构抓点 | 独立拇指／食指对夹，其他三指收拢避开接收机；掌不能穿盖；腕随实际转动，开／闭全过程与真实释放保持分开 | 原机构开闭曾有游戏证据，但尚无完整专用手型证据。仓库 weapon_hammerburst_grasp.inl 是未接入当前编译调用链的历史草稿，其 hammerburst_latch 枚举也不是当前公共 Kind；不能作为已交付功能。不可直接 include 该草稿。 |
| 原旧弹匣，左手 | 原 HB 独立弹匣、实际释放／拆出后的组件矩阵、同一物件身份和租约 | 按本枪弹匣实际截面包握；拇指与四指沿相对两侧，食指不承担扳机；拔出、持件旋转、原匣返座、松手分开定义 | 7712 原匣返座和返射通过；这是机械／库存证据。现有 generic magazine 手型注释描述 Lancer 几何，不能当 HB 表面已标定。 |
| 新弹匣／入座阶段，左手 | 真实肩部供弹预留物件、原 HB magazine 资产与当前实际组件；插入仍测原物理手 | 可复用旧弹匣本枪手型基础，必须按新件实际挂载方向校准；肩取→持件→入口导向→入座→交还显示的腕／指过渡独立验证 | 53224 机械 replacement 返射曾通过；153818 将验证修复后有限库存链，仍不自动授予手型接受。持件时不得被护木或盖抢手。 |
| 无备弹／总零请求，左手 | 原供弹拒绝与当前空手；不生成虚拟弹匣 | 保持实际空手，无凭空闭合持件；一次有效取弹请求对应一次反馈 | 153818 尚未实测，不能从 CPU 检查或六连发推断通过。 |

原 HB 的 Root／MagLatch／Mag 三骨及 Idle／Reload／Reload_Fail 三段已核对，没有原资产独立可操作外露栓柄和拉栓轨道。本枪不新增栓柄或单发实弹 GrabPose。该结论仅限定当前一代 Reloaded 原资产；审计见本枪开发记录及 hammerburst-bolt-original-audit.json。

## 实施边界

逐件标定保存在 HB 独立几何／GrabPose 模块。公共层仅复用已有姿势提交、120ms 渐变和唯一 C0 手指写入；若需要新增视觉 preset，先与当前共享编辑者约定准确枚举和分支，不改变 ABI 大小、不改变其他枪选择。HB 对卡扣和弹匣的分派不得继续依赖 Lancer 私有机械状态。现有历史草稿须经新接口与实际模型校准后重写，不能以名称相同视为可复用。

Snap 仅提交合理近距离的视觉辅助；机械角度、拔出和插入行程继续使用原物理手及真实 Grip 资格。已持物件先实际释放，再以新边沿取得下一对象；按住旧 Grip 从区外回来不产生新抓握。失追、切枪、暂停与空间换代仍沿现有身份／新鲜度／租约规则处理。

## 画面检查与当前冻结候选

每件分别查看靠近、无 Grip、真实 Grip、持件移动旋转、机械操作、释放和再次抓取，补查超范围退出及失追／切枪／暂停恢复。优先取能看见掌、拇指、四指、腕和袖口的正侧与必要斜角；看不清就记录未验收。一次具体失败对应一次针对性修复，不反复跑未变整套流程。

HB153818 finite-supply 继续其原已受理范围和不可变 PG56bc／Nativec985／2251源／19入口。本文的新手型工作不混入该候选、不据该局宣称新手型通过；取得该局弹药链结果并完成原队列交接后，再以独立标识的本枪手型候选提交必要验证。Publicv71 和其他枪候选不变。

## 逐件手型前置：卡扣原三角接触重新核算

已重新核对原HB卡扣OBJ SHA69d1a394...与原mesh SHA4b6bd784...，1994顶点/2232三角，最近原三角1617得到接触(-2.10061312,24.28469712494818,3.9139482172608178)，相对历史目标(0,24.0049845123,6.0003657341)距离2.973878390278255原模型单位，与历史surface结果独立重算一致。三角绕序法向(0,-.13287482034451467,.9911328276867905)同时记录导出曾反转绕序，不冒称已重新证明外向或闭合实体；原始mesh坐标不等于已验证的运行骨／腕／世界坐标。脚本audit-latch-contact.py、报告latch-contact-original-triangle-audit.json保存原三角顶点。尚未生成或接受本枪15关节手型，不把最近点当实际抓握／机械权限。

当前源码检索未发现hammerburst_grasp.inl／weapon_hammerburst_grasp_pose.h被Playground或Native调用链include；后者原注释“HB profiles selected in C0”缺乏当前接线，已仅修正注释，未改变代码行为和任何冻结候选。下一标定必须用原表面、实际当前组件／骨变换及Marcus手皮肤，独立处理拇指食指夹持和其他三指避让，再经唯一C0消费与实际双眼验收。

## 原 Marcus 皮肤夹持基线（未作为本枪 profile 发布）

audit-pinch-skin-baseline.py复用原只读decoder前段，重新核原body export SHA cc2db56c76efb9354a39affebf95f72c56b60a32624b0cbc760450621a886a4d、BoneMap与权重，1395左手三角，bind/inverse skin残差5.684341886080802e-14；骨长／权重未改。对当前通用loading姿态实际计算15局部quat与皮肤：旧pad anchor(9.75,-4.6,-2.1)最近的纯index支撑皮肤vertex1004位于(10.1238046743,-4.6103144500,-1.4191517935)，距离.7767822106；thumb vertex1132位于(10.2317838010,-4.7571822559,-2.3493756429)，距离.5648097943。两选点分隔.9479166916原模型单位；只是到旧marker最近的顶点距离，不等于完整两指表面最短间距、夹紧厚度或已适配原卡扣。

结果保存pinch-skin-original-baseline.json并含原decoder/当前finger header SHA及local15表。该表明确为generic baseline，不发给C0、不冒称HB authored。尚未把原卡扣mesh frame与运行腕frame闭合；下一步须用原卡扣双面及实际腕基准拟合本枪手型和接触锚点，避免用历史抽象pad中心掩盖皮肤偏差。Native171233冻结及19运行入口保持不变，当前仍未stage/run。

## 卡扣双面宽度与首轮皮肤拟合（离线失败仍保留）

fit-latch-pinch.py沿原三角1617接触点向几何法线负向作原网格交点计算，最近对面三角1582，跨度4.419794732847194原单位；正向无交点。这是该局部射线跨度，不冒称所有卡扣位置厚度或实际手可触面。原skin两tip候选vertex1004/1132分别由末节bone14/26权重1驱动；原间距.94791669。首版固定历史腕侧目标的7角度拟合仅到2.78033083、明显不够，JSON/图保留first-failed。第二版将腕刚体平移和间距约束分开，435次离线目标求值后间距4.4205239952；原index角67.5/65.25/24.25，thumb -3.25/13.75/31.25/22.75，所有骨长与皮肤权重不变。

第二版两个方向完整离线模型图已亲看，仍可见手／部件相交且拇指接触被部分遮挡，不发布C0、不称手型通过。当前JSON的landmark_target_errors是相对首版旧腕固定目标，不能误作当前刚体安装后的接触误差；provisional wrist明确未实测。下一步需在相同原模型上检查完整三角交线并利用剩余腕绕夹持轴自由度与逐指调整消除穿插，之后仍需实际组件／骨架基准与游戏双眼验证。Native171233及运行19入口不变。

## 完整表面交线否定仅靠腕滚转的旧卡扣抓点

refine-latch-wrist.py对原1395手三角与2232卡扣三角双向检查非共面线段/三角相交；穿过、分离、端点触碰、共面4个解析几何控制通过。沿固定两指夹持轴比较12个30度腕滚转方向，最少仍有818个edge/triangle命中（270度，手边→卡扣483，卡扣边→手335），原0度897。计数是边/三角命中，不是818个独立穿透区或毫米深度；不覆盖共面重叠、闭合实体包含或袖口。best三方向图已亲看，仍可见严重手体穿插。

据此否定“只要两指间距符合整块盖厚度，再转腕就能完成”的当前候选；不发布任何表或新握点，固定骨长、原机械接触及Native171233均不改。下一步需要结合完整枪身上下文重新选择可触的原卡扣/边缘表面，并分别校准拇指、食指及其余指的避让，不能继续优化旧大面夹持假设或只看两点误差。结果latch-pinch-wrist-refinement.json/png，运行未启动。

## 当前机械抓点范围纠正与原Idle复核

完整原9576顶点/9966三角上下文图已亲审；历史marker邻域内8个三角中心采样没有得到“跨度.3..1.5且双向10原单位射线无阻挡”的候选，不能据有限采样宣称不存在可触边。不得借此扩半径或移动现行机械抓点。

关键源范围纠正：上述离线旧marker(0,24.0049845123,6.0003657341)是历史authoring目标，不是当前runtime机械抓点。当前root及171233冻结runtime都已使用local(-7.09357487,2.10061312,1.88769577)/units，原代码明确为最近表面外1mm。复核原mesh/Idle export SHA并直接比对原serialized quat/position/parent记录，三骨reference与HB_Idle组合变换最大差3.4228541557e-8，排除当前离线差异来自不同开盖动画的假设，但不代替实时矩阵证明。把当前local抓点组成reference mesh坐标，恰为最近原表面沿法线+.1raw（1mm标准尺度），误差<1e-6；报告runtime-contact-reference-check.json及latch-reference-idle-check.json。

因此保留现行机械contact和所有冻结候选，继续修正视觉手型接触面／腕和逐指姿态；不得将前面历史marker偏差或跨整块盖的失败夹持假设描述为当前机械抓点错误。尚无新的HB专用手型或游戏通过证据。

## 同面接近与完整枪身复核仍未接受

fit-latch-surface-approach.py保留现行机械contact，仅以两候选tip中点对准既有表面、夹持轴沿表面切线，收拢其余三指；骨长／权重不变。将完整9966枪三角与1395手三角双向检查，12个腕方向最少仍923 edge/triangle命中（30度，563+360）；完整三方向图已亲看，仍见手与接收机/部件相交。同面两点接近不等于对夹，也未证明掌面、法线接触或袖口。不能发布。下一步在现contact周围的真实外露表面和手的接近方向上联合约束，不靠单一表面法线强设腕方向，不改机械许可或扩大接触范围。结果latch-surface-approach.json/png/log。

## 当前抓点邻域外露侧面与整手接近失败

find-latch-accessible-contact.py改为每个原三角到当前contact的最近点（不是仅三角中心），在3.5raw有限邻域检查97个候选；29个候选沿各自几何外向射线10raw内没有命中完整9966枪三角。最近侧面triangle1576，点(-2.89071918,24.28474939,3.91394645)，法线(-1,0,0)，距当前contact .796410055raw。射线无阻挡不等于掌指空间可用，不改变机械contact/radius，也不把绕序法线当实测腕方向。

在最近侧面外.1raw布置纯视觉夹持中心、保持原骨长权重并单独标明cosmetic_displacement，fit-latch-surface-approach.py --surface-candidate比较12个腕方向；最少仍894 edge/triangle命中（150度，471+423）。三方向图已亲看，手部仍相交且部分形态不自然，不发布C0、不算HB专用手型。结果latch-accessible-contact-candidates.json、latch-surface-approach-side.json/png/log。下一步需联合调整原逐指局部姿态和腕，并复核与实际骨骼消费的姿态一致性；不继续用单条清空射线替代整手检查。

## 生产局部手指消费与离线计算一致性

check-skin-consumer-parity.py冻结当前hand_finger_pose c5c61d71、pose_math d89ece54、stereo_frame bf438b85、hand_reference_continuity 1c5c154b，C++直接调用生产rotation而非复刻函数。loading与generic authored两路径×15关节×闭合amount0/.5/1=90项；与独立Python/offline输出比较最大quat差1.1123893155135927e-16，全通过。该证据只覆盖局部旋转和闭合插值，不证明世界蒙皮、实时wrist、硬件buffer、surface或袖口。

最初独立只include hand_finger_pose编译失败，因其新authored分支stereo_pose_valid声明依赖stereo_frame上游顺序；已告共享编辑者，不擅改root。fixture使用真实声明header后编译0。离线NLERP最初将归一化closed与略非单位reference直接混合，造成3.3805572697e-9预期误差；已按生产loading保留reference范数、authored先归一化的不同路径精确建模，amount0也保留原reference，不是放宽比较容差。证据skin-consumer-parity-check.json、snapshot头SHA、90条输出和首编译错误日志保存。穿模不能仅凭此归咎局部quat实现，下一步仍需联合解决实际接近方向、逐指形态和世界腕基准。

## 伸展食指假设未能解除整枪穿插

check-latch-extended-index.py将初始食指设为更伸展的15/45/25度，其余三指用65/70/30、70/72/32、75/70/30而非前次95度过屈；仅离线角度求解，仍使用原两侧接触跨度。拟合gap4.4277739271对原span4.4197947328，保留所有骨长和皮肤权重。随后对完整9966枪三角/1395手三角检查12腕方向，最少仍856 edge/triangle命中（60度，512+344），三方向图已亲看，手/部件相交依旧，不发布。单独换食指初态不足，后续应把碰撞按掌、食指、拇指、其余指归因，针对主要穿插单元修正，不再仅凭总间距或腕枚举判断有效。

原Marcus骨名亦只声明Finger_01..12与Thumb_01..03，当前四组MCP在腕坐标z约-3.58/-0.97/1.92/4.25，thumb根z约-2.29，首指组邻拇指；此几何只辅助原分组，不冒称实时手指追踪。局部quat消费一致性已另证；world/wrist/完整三角与像素验收仍开放。当前HB171233排位和机械接触不变。

## 穿插按原蒙皮分组定位与中指单关节修正方向

localize-latch-hand-crossings.py按原4影响权重，对每条手边/手三角作最大权重组归属，不丢弃混合面；掌0、食指368、中指330、无名指33、小指0、拇指125，双向分组严格重合原512+344=856总命中。数值是edge/triangle命中，不当作穿透体积、毫米深度或独立区域。当前姿态不是掌面主导，因此不再凭总数整体移腕。

固定腕、食指、拇指、接触、其余骨姿，仅改变中指MCP bone15：侧向Y -20..20度五个针对性对照最少846，改善小；另仅追加局部-Z屈曲0/15/30度，原0度重现856，+15=761，+30=634（377+257）。更强屈曲有明确减少但仍严重穿插；最佳三方向图已亲看，仍不自然且有遮挡，拒绝发布。不增加骨长或改权重，不修改机械contact/radius及HB171233。数据latch-hand-crossing-localization.json、latch-middle-splay.json、latch-middle-flex.json/png。下一步在已定位食指/中指/拇指上分别处理，再做全手协调和实际骨骼/双眼验收，不以局部计数降低称通过。

## 固定食指接触IK未改善，原法线发现接触朝向不成立

refine-latch-index-ik.py在固定腕/拇指/食指tip目标、原骨长/权重条件下，只改变食指MCP局部两侧轴及PIP/DIP作有界IK：MCP14.25度候选tip误差.6352raw超过.1raw上限拒绝；原29.25度/45.5/24精确重现634；MCP44.25度候选tip误差.00409raw但全枪相交增至745。因此不以点位误差小接受姿态，也不放宽骨长/接触容差。

audit-latch-pad-normals.py进一步读取原Marcus相同原export内每顶点stride64的packed normalZ(offset16)，使用原骨图/蒙皮旋转和相同腕变换。index vertex1004原字节131,19,59,255，thumb1132为35,112,214,255；posed原打包法线与原三角fan叉积点积分别-.999656/-.996199，说明两者绕序约定相反，不能把未校验fan法线当外向。按打包法线与预期相向接触面法线比较，两点分别-.432070/-.145732，即此前两点位置对齐并未形成正确相向表面。此是两个候选点的方向证据，不宣称已识别解剖指垫或证明全部skin normal消费；原无向edge/triangle穿插计数不受法线翻转影响。

下一步按原皮肤packed法线与真实可触面共同选择指垫区域，再作位置/朝向约束；不继续仅用旧marker最近顶点与两点跨度调关节。结果latch-index-ik.json/png/log、latch-pad-normal-audit.json保留。当前无手型发布，无游戏运行，HB171233与mechanicalcontact均不改。

## 原packed法线筛选与法线/间距联合候选

select-latch-opposing-pads.py在原末节bone14的33个纯权重皮肤顶点、bone26的28个顶点间检查924组配对；同时要求两侧normal朝对方cos>=.7，只有3组满足。旧1004/1132的相向cos为-.432/-.146，不再当有效指垫。选择双侧最小cos最高的1002/1018组（起始.96581/.86885，gap1.73360），这是几何相向皮肤patch，不冒称解剖指垫已认证。

fit-latch-normal-aware.py联合原两侧跨度与朝向约束，固定骨长权重，141次离线求值后gap4.4195235204对原4.4197947328，两侧cos .8503569721/.9668508931，达到本轮明确gap误差<=.1raw与双侧cos>=.85。新index16.25/34.5/14.5、thumb8/4.25/40.75/37，其余中指MCP沿前次+30保留；局部15矩阵归档。只算位置/方向子约束通过，不是GrabPose交付。

check-latch-normal-aware-whole.py随后在完整9966枪三角/1395手三角上复核12个腕方向，最少仍397 edge/triangle命中（60度，230+167），三方向图已亲看。仍有接触区域遮挡和穿插，不发布C0、不称surface/腕/袖口/游戏接受。结果latch-opposing-pad-candidates.json、latch-normal-aware-fit.json、latch-normal-aware-whole.json/png/log；无机械contact/radius、骨长或输入许可变更。下一步针对新候选剩余具体碰撞部位作校准，不回用已否定旧皮肤点。

## 新法线候选的全枪分组及收紧朝向对照

localize-latch-normal-aware.py重建同一15骨蒙皮与归档腕变换，在完整1395手三角/9966枪三角上精确重合397命中：掌0、食指210、中指17、无名指0、小指0、拇指170。进一步按原枪组件分解：食指卡扣165/其余枪身45，中指卡扣17/其余0，拇指卡扣142/其余28；双向和仍230+167。混合权重面全部归属，无丢弃。此是离线无向边/三角命中定位，不是穿透深度、区域数量、实体包含或游戏验收。三视图再次亲看，仍可见手指被部件遮挡和穿插。

因此只满足相向cos>=.85还不足以保护指垫周围皮肤。fit-latch-strict-normal.py保持同一原骨长、权重、皮肤点1002/1018、腕外的7关节参数，收紧到双侧cos>=.98并加强朝向惩罚，547次有界求值后gap4.3965476660（目标4.4197947328），cos .9621518606/.9662507457，食指MCP到0度边界，仍未达到明确朝向约束，判失败。没有放宽阈值、冒称最优不可达、改骨长或发布姿态。结果latch-normal-aware-localization.json、latch-normal-aware-component-localization.json、latch-strict-normal-fit.json。下一步须检查接触patch周围曲面及可用关节自由度；不把单顶点贴合当成完整指垫贴合，不继续忽略卡扣周边体积。

本次仅离线作者研究；HB171233冻结候选、机械contact、公共v71均未修改。仍等Lancer165800显式最终窗口交接后才启动已接受的有限弹药流程。

## 原骨指垫、真实双面与连杆接触的进一步限定

本轮不是继续只调间距。新增MCP原局部X/Y两轴、固定原骨长/权重，在原1002/1018皮肤点和旧两面上求解：间距4.4147074630，双侧cos .9999882236/.9999732344，明确位置/方向子约束通过，但完整9966枪三角最少789命中，比397更差，三视图亲看拒绝。相关latch-mcp-normal-fit.json及latch-mcp-normal-whole.json/png。原397按单骨最大权重重新分解：bone12/13/14为43/101/66，15/16为13/4，25/26为93/77，其余0；严格重合230+167。混合面仍全部归属。检查纯末节表面相对两个接触平面：原normal候选食指仅一顶点越过.0108999raw，拇指0；新MCP候选两侧均0。此仅平面包络，不是实体碰撞深度，也不能排除末节在曲面/边缘相交。

按原骨局部坐标审视，旧1004食指点法线主要+Z，所选1002法线主要-Y，thumb1018是末端+X cap，不应把其方向正确等同解剖指垫。原末节屈曲轴-Z对应的-Y表面候选为index1002/thumb1014，保存原坐标和packed法线；使用该两点的有界关节拟合也达到朝向/位置子约束，但完整枪最少493命中，仍不发布。

真实枪身阻挡复核：在原机械触点3.5raw邻域的29个向外10raw射线清楚表面中，要求向内第一面属于卡扣、跨度.3..10raw、对侧法线cos>=.8且对侧向外10raw清楚，0个符合（10个先撞其余枪身、4个对侧被挡、7个法线不相向、8个跨度不符）。仅说明本轮射线/最近三角样本，不宣称所有抓法不可行。原最近侧面1576向内.34839321raw就撞remainder3885，同向法线，不能把它当虚构相对夹面。拓展的是离线全件几何研究范围12raw，绝未扩展运行Snap Volume；2189三角样本有54个双面候选。其余枪身已并入所有验证。

候选1828/2076在距机械触点3.97501195raw处，宽4.25136328raw、双面法线几乎相反，但完整手最少758；使用-Y两指垫后818，图均亲看，仍显著穿插。改为原下方连杆532/436（距现机械触点8.99348439raw，宽2.00306859raw）后，原两点拟合全手最少173。此只是同一原部件的视觉作者候选，未挪动运行机械触点，也未证明控制器到物理腕/触点对应可交付。

进一步fit-latch-actual-faces.py直接约束两个原表面各自的真实法线及接触位置（各向外.02raw），不用两面法线必须反平行的代理条件。固定原骨长/权重、两皮肤点及同一原表面，45次迭代721求值，位置误差2.21866e-6/2.90854e-6raw，两侧cos均>.99999999999；完整枪仍142命中（94+48），图亲看拒绝。按原权重分组为食指121、拇指21，其余含掌/中/无名/小指均0；这是命中归属，不是每组全部表面净空或袖口证据。初版优化曾绕世界原点转腕并触平移边界，已修正为绕接触中心作刚体扰动；修正后的上述结果才是有效记录。

向下连杆520/431移位的同约束离线对照仍166命中，拒绝。保持532/436精确接触、只锁定食指MCP27.799/42.799/57.799度并解其余关节/腕变换，分别142/140/150，全部保持位置误差<.0001raw与cos>.999999999，但没有实质消除穿插。refine-latch-exact-contact-mcp.py结果保留；不把140称为通过，不继续仅凭调MCP/腕roll整体猜姿态。下一步须定位实际相交面的空间位置与连杆边缘，并保持真实两面接触，不能用更远机械触点、扩大范围或改变骨长消去失败。

本轮所有作者候选未进入产品/C0，未运行游戏。HB171233的19项实际运行脚本再次逐项SHA/AST核对一致，无stage、无抢占Lancer当前窗口。新证据见analysis/hammerburst-20261108下latch-pad-plane-envelope、latch-normal-aware-joint-localization、latch-flexion-side-pad-candidates、latch-true-twoface-candidates、latch-wholepart-twoface-candidates、latch-actual-face-fit/localization及latch-exact-contact-mcp各JSON。

## 精确接触后的实际相交位置与局部避让对照

locate-latch-actual-face-crossings.py使用与原审计完全相同的非共面边/三角判据、AABB、端点eps，逐条导出点位/边/面身份，严格重合94+48=142。全部归属原末节bone14（121：卡扣98、其余枪身23）和bone26（21：全卡扣）。食指命中至最近目标点距离中位1.730868raw、最大2.659799raw；拇指中位1.033462raw、最大1.183081raw。局部三视图将原相交枪面标红、手面标橙并按深度遮挡，已亲看；接触点附近薄边与指面确有可见重合，不能因点误差微小就称拟合完成。数据latch-actual-face-spatial.json/png。

保持同一532/436表面而互换食指外侧/拇指内侧的单候选也解到精确位置/法线，但全手增至820，拒绝，未采用。对原94条forward命中实际涉及的55皮肤顶点/141个顶点-原面配对，加入局部面外半空间避让约束；本轮100次有界求解维持原接触点误差.003303/.002787raw、cos .999415/.999772，但完整枪重检反增204（126+78），拒绝。局部平面代价3.0403降至2.8653并不意味着实体净空或相交减少；不把该代理目标当通过。结果latch-outside-index-fit.json、latch-clearance-fit.json。后续应基于原非凸连杆/邻接枪身的真实曲面，重新选择有皮肤面积净空的抓握patch及手指路径；保持单C0、真实机械权限与原骨长，当前作者候选继续不发布。

## 有限面积指垫空间筛选与内部角面候选

screen-latch-pad-space.py对前54个双面候选的两指分配共108种，用估计radius1.25/1raw球体检查原9966枪三角表面，0种两侧同时净空；这是几何作者筛选，不是完整皮肤、实体包含或保守包围证明。原最近三角取点常落在边缘/凹处。改为原全部2232卡扣三角内部质心后，有限球体表面净空且两侧原射线清楚的共36种，但最近外盖728/1226宽10.18183612raw，原皮肤/关节限位拟合仅gap9.01865024raw、cos .95805/.51093，明确约束失败，没有拉长骨或放宽角度限位。

进一步保留原有限指垫空间清楚的单面patch（index474/thumb518），探索非同一直线的两个真实表面；间距2.5..8raw、两法线及连线朝向约束、两个球体不互穿仅用于筛选，不当作力闭合或机械抓取资格。得到11778配对，归档最邻近100。1738/795的两个内部表面间距3.92069655raw，两外向法线dot-.80787023，两侧估计球体距全枪表面恰1.25/1raw。原固定骨长/权重，原皮肤1002/1018在两真实表面各+.02raw的目标下，25次迭代401求值，接触位置误差<3.5e-7raw、两法线cos>.999999999999；完整手重检仍130相交（64+66），所以不发布。结果latch-pad-space-screen.json、latch-interior-pad-space-candidates.json、latch-corner-pad-space-candidates.json、latch-outer-cap-fit.json、latch-interior-corner-fit.json/png。当前机械触点、Snap范围、唯一C0均不变；这些候选没有混入44504有限弹药运行。

## 混合蒙皮域修正与控制器中心一致性（当前阻止发布的具体原因）

1738/795原两面候选130命中，细分index18（全卡扣）、thumb112（81在bone25、31在26，均撞其余枪身），说明拇指后方骨节进接收机；改用原thumb-Y侧vertex1014后116，仍拒绝。386/1738侧角候选53，全为拇指，原接触点误差.269/.282raw及index法线cos.946未达子约束；其法线初始化、较宽腕搜索、distal1016、±60初始方向、原thumbroot±35轴与少量distalY附加自由度对照均未形成可接受全表面。真实表面法线外移.1..1.2raw最少仍26且接触未验证，未采用；七个局部负向最近顶点平面修正仍60，未把局部代理代价改善当净空。

在原外盖728与下缘913选两点的cup候选，1395严格域首次非共面0，但原固定质心目标误差.1648/.1704raw，仍不接受。按两真实原面法线联立作最小刚体平移.1605932391raw后，接触落在原728及相邻同平面914（原913质心投影超出其三角3.13%，故必须重新求全枪原面最近点，不把面外点当表面）。两点严格落在原三角内部，bary728=(.116736,.252493,.630771)、914=(.031310,.335168,.633522)，skin1002/1014距原面均.02raw，cos .981761/.985530；非共面0+0。额外共面SAT四个解析控制通过，1e-6raw平面eps/1e-8平行cos判据下，1395×9966原面无正面积共面重叠。三视图亲看，仅限本离线域。

原down20+left镜像的Marcus115 grip→wrist映射反算该cup所需controller center=(-7.76195416,31.99573906,-9.07669738)raw，当前本枪机械获取contact=(-2.10061334,24.27140964,4.01306150)raw，两者距16.21905769raw，超现3.5raw获取半径。不能在当前获取点直接发布而远拉皮肤，也不能扩半径。固定现获取点、保留同一原calibration、仅解关节与controller方向的有界对照保持center误差0，但目标误差.441/2.511raw、1395域657，拒绝；此单次拟合失败不证明全部姿势不可达，不据此直接迁移机械contact。

验证域进一步纠正：1395严格域漏掉496混合手/前臂面。按完整原4影响、原参考父姿加入1891域后，cup从0变38（24+14），固定旧center候选从657变1968（948+1020）。参考父姿不是实际C0 arm IK，不能冒称实际袖口碰撞深度或已定位完整实时原因，但已足以撤回零交线的“完整手”含义，所有候选继续拒绝。原controller marker wrist-local=(5.4255638123,.4567337036,.2020151615)，1395域3ray仅一odd，不能证明掌内；1891域最近skin1.7578607364raw且3个去重斜ray均odd，支持参考掌内位置。整体闭合实体/实际物理校准仍未认证，不把这个局部证据当数学绝对不可达证明。结果latch-hand-domain-scope-audit、latch-controller-palm-marker-mixed-audit、latch-mixed-skin-reference-parent-check、latch-outer-cap-grip-coherence各JSON。

已新增本枪离线工具tools/weapon-models/hammerburst_hand_surface.py：必须显式提供完整115 posed矩阵和inverse bind，缺父骨不自动填reference；按任一手骨正权重选三角，保留所有原4影响，输出原三角身份及严格/混合计数。6个回归用例通过，包含混合面保留、bone29真实影响、不悄悄补缺矩阵、非有限父姿、原权重不重归一化。原body SHA cc2db56c...真实集成检查36637→1891=1395+496，bind identity残差5.68434e-14；只在测试中转原ForeTwist29十度会改变所选域289顶点，而旧1395域变化恰0，证明旧域确实观测不到该受影响皮肤。此为工具计算验证，不是实际C0 palette或游戏通过。证据mixed-skin-module-check.json，下一步必须用现有唯一C0的真实/等价解算前臂、扭转、腕姿一起约束蒙皮和raw controller获取；不再仅优化两个指垫点。

Lancer确认当前V4通用契约2984字节/完整32SHA/reference15稳定，HB真实magazine lease可走独立左part profile，不需虚构hammerburst_latch枚举；当前共享consumer实际腕读回差异尚未知。HB不改shared C0，不发布未通过provider，不把离线零数值当实际消费/双眼/头显验收。HB185428仅有限供弹driver修复冻结，所有本节手型/工具均未混入该批。

## 现有 C0 完整父姿探针与扩大皮肤域复核

新增 `tools/weapon-models/hammerburst_c0_skin_probe.cpp`，在独立 owned buffer 中调用现有 `body_c0_hands::compose_common`，输出完整115骨 world palette；无引擎读取、hook、controller publication、真实时钟查询或游戏写入。构建仅独立 HammerburstC0SkinProbe 目标，Visual Studio 18 2026 编译0。源码 e490ebd7cbf20f15d7f208a8bb9155d06ffad5671803d4e146aa2d800add464b，程序6dddfdb8ee36e513fb7cd8bc902ae9009962086057b257352e66bc7a97447458，28个冻结 quoted headers/fixture manifest39029b0f24dde6ef4e4b074992155bb814e4cd39f75810d2a90b00062fb0dd2d。初次 host 调用将嵌入 CoreSnapshot 的 base_view 同时作参数，触发原 alias 防护；改为独立值拷贝后通过，未改产品防护。原失败保留 first-alias-rejection.json。

输入组合旧62276真实保存的 reference/C0-local/component 与44504真实 HB 主手到枪模型的相对挂载，再在该保存身体上构造目标，明确不是同场 tracking/cohort 或实际当前消费。作者15关节使用728/914 cup，现有C0负责肩、肘、扭转、腕和手指。基线 wrist 被固定骨长 reach 投影，目标差 .41184883752raw；1395及1891面域均130条双向严格交线。仅作几何对照，将构造的主手/枪向左肩移1cm，wrist差 .0001885924raw，1395与1891域均0；这不修复真实获取中心16.219raw不一致，也没有发布该姿势。

进一步将任何正权重落在原 bones6..29 的肩/上臂/前臂/手/袖口共4841面全部纳入，原基线仍322（134+188），构造1cm对照仍177（76+101）。扩大域三视图已亲看，肩/臂甲与枪身仍穿插，因此1891的零交线不能升级为整个上肢净空。本探针证明必须同时约束实际身体、枪的位置和作者手型，不允许以仅掌指清楚为通过。两个候选均 `skin_fit_accepted=false`、`published=false`，本轮未启动游戏。

所有115骨相对原reference的最大长度差24.159raw来自保存动画的IK_Hand_L等helper，不是前臂伸长；与原保存C0-local逐骨父子长度比较最大 .0002674804raw，15作者局部矩阵误差小于5.6e-7。保留原保存局部平移，不强行把动画helper改回reference。证据 `analysis/hammerburst-20261108/c0-mixed-skin-probe/probe-validation.json`、`latch-c0-mixed-skin-probe.json/png`、`latch-c0-inward-one-cm.json/png`。这些离线工具仍不混入已冻结HB185428，无共享C0源改动、无第二writer。

## 双臂解算与持枪方向对照

对上述177交线逐条保留原 edge/face/皮肤顶点和主导骨身份：145条主导ForeTwist2_L29、32条主导Forearm10；没有肩或手指主导的命中。保存身体的原主腕方向经free-hand校准和真实HB挂载构成的枪口朝向，在body分量中为(.4386,.8765,.1983)，主要朝身体侧方，因此不能将此构造例的上肢穿插直接当成自然前向持枪的唯一原因。

探针增加显式 `--constructed-gun-pose pose.bin`（7个xyz/xyzw double）模式，在保持原controller到枪的相对挂载、左作者腕/15关节不变的前提下，让现有C0同时解算左右手目标；没有写入追踪源或绕过运行许可。新独立编译0，源码1984756f957fd3e2561f19b1fcb6dfa27a41668185e09119e5751e6060e0067e，程序5de34599f33798cd670e5579bac4029c51aac1d782f45b40eec2e4401093f7c9。旧二进制及源码保留为私有目录 `HammerburstC0SkinProbe-original.exe` 和 `probe-original.cpp`，旧receipt不挪用新binary身份。

仅两个人工明确的几何对照，无参数网格：原mesh+Y对齐body+X、mesh+Z对齐body+Z，枪root在body(15,-8,125)raw时，solved_mask3，左右腕误差.000172106/.000030438raw，1395域0、1891域29、4841域44（22+22），44条全主导29。将同一枪朝向移到较近主手肩侧body(10,-15,125)raw，左右腕误差.000165961/.000029890raw，1395域0、1891与4841域均10（6+4）。两份完整三视图均亲看，仍不接受；这只缩小到剩余混合袖口接触，不是游戏或头显通过，也没有消除原raw controller获取中心16.219raw不一致。

证据 `latch-c0-arm-crossing-attribution.json`、`latch-c0-front-hold-attribution.json`、`latch-c0-front-hold.json/png`、`latch-c0-front-side-hold.json/png`，以及私有目录两份front-hold/front-side-hold-receipt.json。下一步应把实际C0袖口表面加入原两指真实接触约束调整作者腕/手指；不能仅继续更换持枪例以挑出一个零计数，更不能把一个离线构造姿势提升为可发布profile。有限供弹HB185428及其sole-successor队列未变。

## 实际 C0 袖口约束后的闭合姿势作者夹具

保持同一构造body(10,-15,125)raw枪位置，进一步定位原10交线：全部主导bone29，原皮肤9869/9870/9871/9872/9873/9916与原枪平面x=-2.08756852相交。将这6顶点面外约束与两指原表面接触、法线及原关节限位一起求解；每次使用独立probe调用现有C0并按完整115骨、原4权重计算所需皮肤，不再用reference父姿替代。首轮5次/82调用将原袖口命中清除，但全4841面×原9966枪面重查新增4交线，全为index distal14的原edge1002/1003与原666/729面，最大穿入约.003526raw。首轮仍拒绝，保留latch-c0-sleeve-fit.json及其surface/attribution。

第二轮加入该index相邻面顶点约束，40次/642调用后，全1395、1891、4841三域双向严格交线都为0。仅用真实穿越edge端点1003的第三轮对照也得到相同最终输入/皮肤；不是新增一次独立游戏通过。局部平面代价仍4.2467且食指相对原固定目标误差.0433667raw，不能称原精确目标约束通过。另行对实际C0皮肤求全原枪最近点，确认1002仍投影在原latch728内部，bary(.117154,.251839,.631007)、外法线gap.06336496raw（约.634mm）、cos.980453；1014投影在原914内部，bary(.031730,.335068,.633203)、gap.02172385raw（约.217mm）、cos.986600。两投影相对原表面接触点仅移动.000387/.000496raw；偏差主要是外移，未把面外点当原模型表面。

完整4841×9966共面检查沿用原SAT，4个解析控制通过，原平面epsilon1e-6raw、parallel cosine1e-8下无正面积共面重叠；与双向严格交线0分开记录。完整上肢三视图已亲看。本轮仍未认证闭合实体包含、所有姿势、卡扣运动全程或实际游戏。旧机械获取中心与作者controller中心不一致仍未解决，不扩大Snap半径或远拉手皮肤。

保存 `tools/weapon-models/fixtures/hammerburst_latch_closed_c0_candidate.json`：明确UNPUBLISHED_OFFLINE_CLOSED_POSE_CANDIDATE，原closed mesh坐标腕姿、原bone12..26的15个局部quat、完整原资产/探针/输入/输出证据SHA及未完成项。最终输入e9956f65741afcb6a26ff83602e6f24ad54fc2925da7dbe3eaf3fc32a68ec8cb，完整115骨输出bbb9e755d8606b387e53a831d7924d11bc7e92b97e3c6c36803ce9ba3ba741ff。作者求解器、全域复查、交线身份及原面/共面复查源码保存在 `docs/hammerburst-c0-sleeve-fitting.patch`，反向git apply --check通过；数值和原失败保留，不混入HB185428。下一步用原3骨/原蒙皮复查卡扣运动，再核对controller anchor与真实lease/provider；当前未生成生产profile、未启动游戏。

## 原动画运动失败、联合约束与九个有限角度样本

本轮直接读取并核SHA的原9576顶点/9966面及原4影响、chunk BoneMap和3骨reference，不再用丢失骨归属的remainder OBJ运动。原骨刚性权重分布7311/1994/271，全部一影响255；原HB_Reload导出SHA18a9499c...的Root/Mag各为常量，唯一变化为Latch75个Float96NoW旋转键。原source闭合姿势相对reference最大顶点差1.97e-6raw，复查0；沿原3骨变换带动腕和全枪原蒙皮，原key10角.418879rad（约24°）出现6交线，key11角.837758rad（约48°）出现14。中段6全为29，打开14为29主导12、11主导2，全部撞原mag骨2的皮肤；后两条也是混合皮肤，不是独立手指穿越。闭合候选因此不能推广为全程通过。原失败保存latch-c0-original-motion-check及attribution，不覆盖。

用同一closed参考腕及同一15局部关节参数，联合这三个原动画姿势作一次40迭代/1926调用的有界C0求解，保留原固定骨长、权重、原关节限位与同一构造枪位置；局部代理代价3.992仍非零，不称数学精确约束全部通过。随后实际完整4841上肢面与原9966枪面双向重查，三个姿势均0；三姿势×三视图已亲看，原Root灰、Latch蓝、Mag绿，未把图当游戏输出。腕目标与实际orientation差约3e-5..5e-5度，未发现腕角被明显截断，不能据这批给共享consumer定故障。

重新核原75键：完整xyz quaternion tuple仅5种（非仅角度round去重），Root/Mag与三骨position键均各1；Latch非铰链分量最大4.43e-8。因此取5种原键姿势代表全部75个原离散变换，并另取4个相邻唯一角度中点，共9个有限样本。9份现有C0完整115骨及原枪完整蒙皮重查均0双向交线，4个解析控制的同一SAT共面检查也均0。覆盖离散原键和4个角度中点，不是连续碰撞数学证明、所有持枪位置或完整游戏动画/实时输入验收。

每样本1002/1014全原枪最近点都落在原Latch面4733/4977内部，正gap .0230322..0870297raw（约.230..870mm）、最小facing .977554、最小bary .0319745。原package索引绕序与先前OBJ相反，初次直接拿原三角cross作外向导致gap/facing符号反转，原unoriented报告及首次过严的packed平行断言失败日志均保留。改用原weapon packed normals确定面外向，原face cross与packed dot分别约-.998759/-.837245；后者是平滑顶点法线而非平面法线，故只验证明显符号一致，不要求两者完全平行。刚性分组由全原顶点Kabsch还原且残差<1e-4raw、det正，再按原法线确定符号；没有翻转或改写枪模型索引。

新未发布作者夹具 `tools/weapon-models/fixtures/hammerburst_latch_motion_c0_candidate.json` 保留原mesh腕及Latch bone-local腕、15局部quat和证据SHA；旧closed夹具仍保留。反算的原down20/left镜像controller中心mesh=(-8.06026716,32.33420587,-8.96293889)raw、Latch-local=(.96922136,8.06026693,-11.08830462)raw，离当前机械contact16.39825426raw（约16.4cm），仍超原3.5cm半径。未改获取点、扩大半径、发布profile或写追踪源；这项不一致明确阻止生产接入。下一步必须将真实获取与同一部件作者抓姿使用同一controller基准，再接既有真实lease/V4消费者。原连续段、游戏双眼和头显、其他部件姿势、有限供弹HB185428实际运行仍待完成。

证据latch-c0-motion-fit、latch-c0-motion-fit-surface、latch-c0-fitted-range-check、latch-c0-fitted-range-contact、latch-c0-motion-grip-coherence各JSON，图latch-c0-motion-fit-views.png；本轮六个作者/检查脚本保存 `docs/hammerburst-c0-motion-fitting.patch`，反向git apply --check通过。未动冻结HB185428、公共制品或共享Native/C0源，未启动游戏。

## 新补核：手指互穿明确拒绝；同基准获取换算源码独立通过

最新工作区要求同手非邻面检查，未穿枪不能替代自然整手。新增1891原手/混合袖口面的双向edge/triangle self检查，按原bind精确float位置与非零骨/byte权重map建立UV缝邻接，绝不以当前顶点重合排除交线。原reference有83对、全为主导palm/palm；当前motion候选closed/middle/open分别582/587/584对，其中按主导区域中/无名114、无名/小45、中/拇13、食/中4，在三姿势一致。不能用原reference83对掩盖这些新跨指交线，明确拒绝当前候选；coplanar self、闭合实体和完整自然拟合仍未认证。

只保持同一腕/食指/拇指，后三指回到原reference MCP并使用35/50/25、40/55/30、45/60/30度屈曲（移除既有中指额外MCP30）的单次放松对照，self313，仍有食/中54、中/无名11等；同时新增完整上肢对枪137交线，仍拒绝。完整两组三视图按指列着色亲看，latch-finger-columns-rejection.png；并未将较少计数当自然通过。motion夹具状态已改REJECTED_CROSS_FINGER_SELF_INTERSECTION并加入self source SHA，旧九样本枪体几何证据保留为局部条件。证据latch-c0-motion-self-surface.json、latch-c0-resting-fingers-check.json；三个脚本保存docs/hammerburst-self-surface-rejection.patch，反向git apply --check通过。

另外完成独立纯计算 `tools/vr-common/weapon_hammerburst_latch_mount.h`。输入同一原part/gun刚体frame、原part-local腕、model scale及完整Marcus115几何签名，先算目标腕，再逆原down20/left calibration得到Grip；机械获取的weapon-local metres位置与显示world Grip来自同一个结果，避免“指腹表面点等于控制器中心”的混用。转换不修改输入，不读引擎/时钟，不提供半径、lease或发布资格；未经self/自然整手接受的fixture没有接入生产，旧3.5cm获取半径、旧机械contact、V4消费者和HB185428冻结均未改。

新增同名_test.cpp，冻结28-header树仅作其项目依赖输入；独立Visual Studio18/x64 Release /W4 /WX /EHsc编译0、实际测试0。1096坐标检查验证原fixture反算中心与旧点距离16.39825426raw、五原角度下单位10/100/500及part/body缩放的腕/Grip/weapon-space往返、末字节错误rig签名、无效单位/缩放、非有限profile和坏quat拒绝且输出保持。数量不是武器通过；纯换算源码与测试已准备，production_hookup=false。准确源码/程序/header manifest SHA及构建结果在analysis/hammerburst-20261108/latch-mount-build/build-check.json。

只读实际44504存档geometry行也补核：相对contact-pivot与原reference同一局部向量旋转残差4.118e-9m，原铰链轴残差6.717e-8，支持该已观测闭合骨基准一致；不是当前原子骨/GPU或新抓姿实际消费证明。latch-44504-reference-basis-check.json绑定原PG日志SHA。下一步继续修后三指和指列排列，同时保留已完成的获取映射基础；先通过自身表面及枪体联合条件，再将真正可接受的profile接到同一真实获取/lease和C0。未启动游戏、未改公共制品。

## 后三指的保存源复用与剩余9对（仍拒绝，先保存再进入新运行窗）

只去掉中指额外MCP30的窄对照仍有跨指交叉且新增130条枪交线；原Z指列代理六自由度40次拟合把中/无名114降到1，却把无名/小45增到105，虽枪0仍拒绝。纯离线FK与最后实际现有C0的蒙皮差5.22e-6raw，不能将代理改善当自然手型。按实际交叉面建立有界15自由度避让，第一次12轮最优跨指10/枪10；保留已解近面约束的第二次12轮最优跨指15/枪0，两者也拒绝，全部中间计数保留，不把任选分数当通过。

改为直接复用旧62276已保存C0-local中15..23原9个局部旋转（不改腕/食指/拇指），得到枪0，后三指彼此及中/拇不再有交线，剩食/中51；该保存源不冒称已认证原artist或当前tracking。再只改3个MCP方向、保持PIP/DIP和原屈曲，在±15度/8轮有界近面约束下，实际现有C0复核剩食/中9、枪0，仍拒绝，原完整self也未通过。新未发布数据tools/weapon-models/fixtures/hammerburst_latch_saved_source_rest_candidate.json明确REJECTED，后续运动未核，不覆盖旧motion失败夹具。证据latch-c0-neutral-middle-check、latch-c0-finger-columns1、latch-c0-finger-surfaces1/2、latch-c0-saved-source-rest-check、latch-c0-finger-source-mcp各JSON。

另外测量当前原motion目标姿势的肘→腕线与腕+X（原MCP列前向）的几何夹角，闭合97.4329度、中段74.5193、打开63.7344；这不是临床或实体追踪测量，提示固定该腕、只挪后三指会遗漏整体腕方向问题。保留latch-c0-wrist-axis-audit.json；下一作者步骤应把腕/指根方向及原接触patch一起约束，不继续盲调同一curl计数。新增/修改作者脚本保存在docs/hammerburst-finger-column-repair.patch，反向git apply --check通过。

HB185428静态再次核2254PG+1338Native源、6saved、19当前runtime输入和AST一致，未stage/采样基线，证据completion-window-current-static-check.json。本研究数据与所有新手型仍未混入冻结185428。已收到Bol180947完整全终后的唯一交接，接下来仅做新鲜idle/配置基线预检，按原有限供弹scope进入185428；先完成本节Git保存，不把未通过手型带入该次运行。


## Steam 路径范围纠正与自动补弹保护遗漏

50148 实际启动了历史 P2P 目录，违反 2026-10-21 的 Steam-only 要求；EXE 哈希相同不能替代路径要求。原始日志、数值、失败、视频及不可变交接保持原样，但本次只算旧路径观察，不计 Steam 版验收。其他使用旧 P2P 路径的 HB 历史用例同样不能继承 Steam 验收。补充记录 runtime-50148-retail-scope-correction.json。后续只允许 J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe。原唯一 Boom191450 交接槽已由同源 Steam 衍生 Boom214704 取代，独立静态核对后接受，没有增加第二个运行槽，本聊天不再调用共享运行/配置/retention 帮助器。

源码确认 weapon_lancer_manual_reload.inl 的 requires_permission 遗漏 Weap_LocustAssaultRifle，因此 HB 未进入已有 CanReload 自动补弹返回值保护。经 Lancer 文件所有者确认无冲突，只增加 HB feature getter、前置声明和启用时的类型分支共三行，沿用原身份检查、原函数一次执行、物理 manual_start scope 及清理。其余武器判定不变。实际提取生产 requires_permission 编译测试，9 类×8 开关组合共72组：修复前4个启用 HB 组合失败，修复后0失败；原始失败结果保留。此项仅证明类型及开关分类，不能证明实际回调绑定/自动补弹阻止，更不能证明50148骨角失败的唯一因果或自然手型修复。

隔离构建采用 HB185428 冻结源与其原 GEARS flags，只加入上述三行；仅构建 GearsPlayground target，configure/build均0，DLL SHA256 a05a6c3ce20b4fa6a82f41a03a38ab690b69543d84193ad96c3b75f8619f0d5c。未启动游戏、未替换公共/私有运行包，未宣称手型或换弹通过。构建记录 analysis/hammerburst-20261108/manual-veto-isolated/result.json。

Steam 迁移还修正 HB 私有 run_hammerburst_check.py 的 capture identity 选定路径；原严格进程路径/PID/creation 校验完整保留。5个无进程夹具通过：Steam首次及重复登记接受，P2P/错误PID/错误creation拒绝。可重现差异保存 docs/hammerburst-steam-capture-path.patch。新源构建冻结 hammerburst-prepared-20261109-220220-manual-veto-retail，prepared SHA22f045ccbb503f98ec47051cd512a5320cf19199918b7b7c73661900ea70ccc3；2256源/19runtime输入沿用既有动作链，未混入拒绝的手型，未stage、未申请窗口。Steam启动wrapper、显式settings绑定和新鲜运行基线仍须在后续完成，当前冻结不是可直接运行的已交付测试包。


## Steam 启动前置失败与腕/原接触patch的有界探索

独立读取 Boom runtime-4380-review.json：精确购买Steam路径在Native注入前出现原生Steam-required对话框，原loader deadline后正常关闭；Steam客户端已运行，注册表PID不匹配仅为观察，根因未证明。HB不以新武器候选盲重跑此已知失败，不改Steam登录/许可/registry，不调用共享runtime helper、不抢窗口；已向既有共享启动责任者通报。新220220候选尚无Steam启动wrapper/窗口，目标保持完整，后续先解决合规背景启动再做实际自动补弹/翻盖/弹匣验证。

新增 tools/playground/weapon_hammerburst_retail_source_check.py，只读核2256归档源、1338Native源、6制品、19直接运行输入及1050项目Python归档/当前SHA与文件名单。再从原冻结源与实际隔离编译源逐文件还原验证恰为三行新增，并核实际DLL与购买EXE身份；当前运行0/零错误。Python集合只是该时点的静态上界，非实际import执行证据，后续新增/删除/修改会拒绝，外部解释器/stdlib/native依赖另核。结果manual-veto-retail-static-check.json；所有原review/handoff/prepared manifest未改，新Python界限另存补充manifest。

手型方面，新explore-latch-wrist-contact.py仅做15组刚体几何假设：绕原食指皮肤点1002保持其位置，腕向当前构造前臂轴转0/20/40/60/80度，再绕新手纵轴roll-30/0/+30度，检查拇指皮肤点1014到原2232个纯Latch骨三角的最近表面点。使用真实保存C0完整115骨与原4权蒙皮构成的现有9交叉候选，未重算新完整手臂C0，不能当新可用姿势。原0/0拇指间距0.02310raw；轴夹角降至37.4329度的样本中最近仍2.77066raw，17.4329度时最近4.05306raw。这只是构造几何，未作临床/实体自然性结论，raw不自动当厘米。原食/拇patch和手内指角固定时，单纯顺直腕会丢拇指接触；下一方向需要共同重选两个原接触patch及指间开合，不能只继续修后三指或扩大Snap半径。15组原始点/全局三角索引/矩阵在latch-wrist-contact-exploration.json，脚本可重现patch在docs/hammerburst-wrist-contact-exploration.patch。原9对自身交叉、法线/枪交线/新手臂未通过，全部未发布，未混入220220。
