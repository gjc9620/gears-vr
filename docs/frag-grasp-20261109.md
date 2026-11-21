# Frag 原握柄与逐部件手型

2026-11-09扩大覆盖后的补充：此前“自身0”只覆盖解剖分组明确的不同掌／指组，不是整个手部自相交通过。现对全部1395个已选择的原右手面纳入同指内部和110个混合指根面，先按原顶点ID相邻性统计，当前2923闭指姿态得到177对交线；再仅按原绑定位置（1e−6原单位）与完全相同的骨号／byte权重识别UV接缝的相邻顶点，当前仍有37对／52面，原绑定姿态为0。接缝依据来自原始绑定数据，绝不把新姿态中接近的表面焊接或改几何。阈值仍是超过1mm非共面交线跨度，不是穿入深度；不能把这37对因原GR_Loop也有交线就当作正常通过。它们主要为同指关节与拇指根混合面，整手贴合仍未接受。

`whole-hand-self-contact-surfaces.png`采用真实深度测试显示六个离线方向，已逐图亲看；红色是涉及的面，不等于交线本身一定在可见外表，内褶皱、材质遮挡和真实混合的可见性仍待确认。数值与全部三角面／原交点保存`whole-hand-self-crossings.json`、`whole-hand-source-seam-self-crossings.json`及`frag-extended-contact-review.json`。以前不同组子集0／物体交线0仍只在各自范围有效，不扩大为本次完整表面验收。

为下一次独立拟合实录准备了私有`frag-handle-static/visual-plan/frag_preparation_stop.py`：复用同一原路线／一次真实Grip／原单次arc断言／正常关闭，先等本局新产生的现有authored发布日志，再保持Grip1、trigger0将真实控制器移到近前，展示正面、两侧、返回原位四段稳定画面；没有新增HMD输入、Engine查询、hook或写者。旧发布日志不得重用，发布accepted只证明格式／当前owner admission，不证明C0或像素。控时Host覆盖正常、先前已有投掷、原arc拒绝、发布拒绝／只旧回执、首视图失主／异常投掷共36项检查通过，首次失败无后续输入或松手恢复。这是未来a21／2923的私有准备，没有申请第二槽、部署或执行；排队200930的冻结源／scope完全未改，必须先获得当前退出修复的实际目标结果。腕袖、实际缩放、键间连续变形及头显依然待验。

当前已在隔离源码接通并编译私有Native／Playground拟合候选，尚未部署或实录，也未合入共享源码或手动交付包。后续自身表面审计发现首个ccfd候选仍有43对手部自相交，因此不采用其闭指数据；新的6b1候选只替换15个闭指旋转及profile标识，限定检查内手部自相交、手与原root／Counter相交均为0，仍不等于全贴合验收。下文先保留首轮实现和构建证据，最新修正见文末。持续右Grip、松Grip投出、2kg、直接跟手和原链条规则保持，烟雾仍在Frag完成后开始。正在排队的172809退出修复候选未被这些工作修改。

原 `SkeletalMesh'COG_Bolo_Grenade.Frag_Grenade'` 的613418字节从原包指定范围重新读取，SHA256为 `f015fe8af442469bd23f7b24c9cfc58b11b99085abeedc349236b8cbd4f2a7ac`，与已存清单和骨骼审计一致。12个刚性骨组、5186三角面分别检查，保留整个原演员的链条联动，没有把链节增加为可抓部件。原root握柄范围为X5.780827…10.219170、Y±1.942855、Z−6.612073…8.356934；当前作者抓点(8,0,−15)位于Counter颈部，不能把它称为root圆柱握柄中心。原绑定几何图和截面数值保存于 `analysis/frag-grip-20261108/frag-handle-static/`。

独立重读原Marcus右手皮肤，mesh SHA `cc2db56c76efb9354a39affebf95f72c56b60a32624b0cbc760450621a886a4d`、115骨canonical `3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193` 均保持。原绑定逆变换回代误差5.68434e−14。使用实际右手863顶点、1395三角面及冻结Native5e51的原grenade15关节生成对照，固定全部骨长。比例1是这次明确的离线条件，当前未记录实际Frag组件矩阵，不能把该条件冒称为实测比例。

现有通用手指直接放到root握柄后仍有408对超过1mm交线跨度的非共面表面相交；当前Counter颈部放置有400对。它们是离线真实三角面的相交计数，不能作为穿入深度、运行时像素或连续动态结论。一个只用一般关节角的第一候选得到零交线但拇指形态未通过亲看；限制拇指角度的第二候选达到求解次数上限、仍有6对交线，明确拒绝。这些记录保留，没有用它们交付。

原专用 `AnimSetMarcus_CamSkel_Grenade` 已重读23个序列，animset SHA `a3e4fc9b67fffd968e35048b54ce5770a9adf794747757b797e3fdfc7aeb8ec6`。原Weap_Grenade的WeapAnimLoop确实指向 `GR_Loop`；选中序列 `AnimSequence_1` SHA `3ee466ea3a1b7635f8d00246d95bb1931faf9944518302796c383a3aa6d54834`，右手15指关节均单键。按既有RAW RefBone约定转换非root W，保留原reference平移；动画平移与reference最大差2.53054e−5原单位，没有把动画平移覆盖骨长。

基于该原手雷持握姿态的当前候选，抓点移至root轴心(8,0,0)，腕内圆柱中心为(8.580185303936336,−4.018786487146307,0)，当时离线图腕方向设为identity（后查明生产保持Rx180，见文末更正）。局部指关节在原姿态上作有限偏转。求解使用实际root XY凸包沿原Z范围形成的外包络，并检查手部顶点、唯一边中点和面重心共4509个点；独立真实三角面检查在1395手面／842原root+Counter面上没有发现超过1mm交线跨度的非共面相交。原试算椭圆并非严格包住原握柄的全部棱面，已改用实际凸包；最初3对中指交线与未收敛记录保留。当前求解器success、原长度未变，亲看掌／侧／斜三视图可见原拇指及四指围绕root段；这仍不等于全接触面、腕袖、实际链条和游戏像素验收。

武器独立数据模块已生成至 `frag-handle-static/profile/weapon_frag_grasp_pose.h`，SHA256 `c539ca3b1986048ad626d96c6f2deb4df1de670e8fa29ef568bf4ca72ce37080`。MSVC19.51 Release/W4/WX编译／显式检查程序通过：15组reference/closed输出与原metadata及拟合数据逐值误差0，30个四元数归一，错误geometry signature拒绝且不写输出。模块只返回本枪数据，不发布姿态、不授予Grip、不写骨骼。`profile/check.json`保存二进制、源码与数值证据。

公共V4请求形成端已编译检查，保留原Frame／控制器时间，不刷新100ms窗口；真实Grip保持、原始有效追踪、场景标志和实际模型签名才形成请求，食指输入不是前提。只有请求格式通过，不能当作Native授权或动作许可。11个实际通用头文件在最终检查前另存并固定，原reference／closed数据逐值编译核对为0误差。

受控C0检查直接复现当前通用consumer对held_frag忽略15个已提供关节：原606检查中15失败。在隔离副本只增加held_frag消费选择后同606检查全过，补充当前发布缺失回退原grenade和空手忽略旧载荷后678全过。第一份夹具的auto多声明类型不一致、后续两段回退时间倒退失败均已保留，修正的是夹具声明／时间，没有放宽生产计算条件。它们是原reference/local/component夹具及受控腕目标，不是实际Native或追踪证据。

私有Native采用精确已验证5e51基线，前移植公共V4的右主手子集，并在原唯一C0消费路径中读取本枪手型；左部件V4在该私有候选中不可用，原Lancer V2与其他姿态保留。Native当前 SHA256 `a21efda67e1ea91338e13322b40aacac12ac04b68be625623ac89230c9704b7a`，383个实际C/C++输入、6个实际MASM输入，原104个GEARS设置及40个编译／链接flags和原MinHook源共145个缓存值保持。只有6个实际输入变化：capture、body_c0_hands、hand_finger_pose、full_body_c0_native_live及新增两份通用V4／右主手消费代码。6个ASM和原MinHook清单均匹配冻结基线；初次构建辅助器漏传MinHook源路径的失败保留，修正后没有升级依赖。

额外owner检查只读取既有当前Scene的world／pawn／mesh／weapon、world generation、context_valid／weapon_known以及已选择的held_frag；公开broker原render admission继续生效。消费端保留GT线程、原100ms、输入epoch／profile／控制器边沿、原对象／会话／空间／reference generation、模型signature与15个reference的逐项检查，并拒绝当前失焦／失追／Grip释放和trigger endpoint。实际私有新代码与实际value-only owner函数体在受控broker／clock／对象转换Host下122检查通过，实际5e51前移植C0也通过678检查；不把这些Host seam当作真实Scene查询或Native运行。没有新hook、Engine查询、输入写者或骨骼写者。

私有Playground基于未实录的0a8退出修复，SHA256 `ccfda4089e12f16cfb75f9cacfb0b7e28f0473f8ea788c4cc33414f94ac581d1`，191个实际输入、原93设置／58GEARS保持。三个已有文件只改global includes、已认证Frag sample后的姿态发布和本枪握柄几何；新增本枪数据／请求／发布与通用V4四文件。发布仍要求现有精确Frag资产绑定、同一weapon和hold generation及equipment当前持有，Native地址必须属于已验证模块的可执行区；最多8条变化日志。发布成功不是实际C0使用或像素贴合证明。

**释放点需要新验收：** 现有make_config本来就用作者几何推导 `centre_in_right_grip_metres`。握柄／腕内偏移改变后，原投射物root的出生点及该点随真实控制器转动的速度贡献也会改变。因此这里只能说原始轨迹计算、质量／原调用／库存／引信代码没有修改，不能说世界出生位置或投掷速度数值完全不变，也不能直接继承旧几何的伤害／碰墙结论。该偏移仍是原始控制器轨迹上的固定作者几何，不从吸附后／渲染骨骼回读。新出生点、原once生成／扣弹、碰撞和返回过渡仍须实录。

上述两制品仅保存在 `20261109-182749-frag-authored-native` 和 `20261109-183542-frag-authored-pg` 分析目录，没有申请新队位、部署或运行。当前172809仍只验证原准备退出顺序，不能顺带把本拟合算作验证。汇总及限制见 `analysis/frag-grip-20261108/frag-authored-build-review.json`。

需继续分开检查：裤兜接近／被动吸附／真实Grip获取、准备持有和旋转、松Grip张手及原返回、原独立近战贴附；每项均要掌／拇指／四指／腕袖与正侧斜实际双路画面。腕袖、源动画与当前原链条姿态、实际缩放及过渡仍未验收。手型数据和数值只供实施，不产生任何机械权限。

主要证据：`geometry.json`、`hand-fit.json`、`original-grenade-animation-index.json`、`mesh-defaults.json`、`original-loop-fingers.json`、`original-shaft-fit-proposal.json`、`crossings-original-fit.json`、`hand-original-fit-review-inputs.json`。图为 `frag-handle-bind.png`、`frag-hand-mount-comparison.png`、`frag-hand-original-loop.png`、`frag-hand-original-fit.png`。所有文件均在上述ignored分析目录，仅原始模型资料与CPU离线图，没有新游戏运行。

2026-11-09后续自身接触修正：独立检查原手部1395面中1285个解剖分组明确的面，排除共享顶点的相邻面和110个分组混合的指根面，统计不同掌／指组之间超过0.1原单位交线跨度的非共面相交；它不是穿入深度，也没有覆盖连续运动、同指内部、全部混合面或腕袖。冻结通用grenade为361对、原GR_Loop为76对、首轮root作者姿态为43对，数值和原交点都保留。不能仅因原动画也有相交就忽略当前相交，也没有改变1mm交线统计阈值。

先限定小指／无名指间距，小指PIP作额外伸展，得到手与原握柄0对但掌／MCP仍17对的`root-digit-clearance-proposal.json`。对残留掌根作有限MCP松弛／PIP补偿：6组初筛后，20度中指／无名指仍有2对中指根交线；仅中指再放松6度，形成`root-palm-clearance-proposal.json`。该候选保留同一抓点／腕内中心和全部原reference平移，限定手部自身检查为0对；1395个实际手面与842个原root／Counter面独立复核也为0对。4509个原顶点／边中点／面重心的root外包络采样均在外，最小gap为0.0499806904原单位。这些是离线限定条件，不能外推成每个表面、动态链条或实际像素无穿插。

亲看`frag-hand-root-digit-clearance-proposal.png`、`frag-hand-root-palm-crease-50-proposal.png`及最终`frag-hand-root-palm-clearance-proposal.png`的掌／侧／斜视图后，保留最终候选用于下一次实际贴合检查。拇指和四指围绕root，局部松弛没有改变骨长；掌根接触、实际缩放、袖口和过渡仍未接受。中间局部Y分指方案虽只有16对自身交线，却有15对小指与物体交线，明确不采用。

最新隔离Playground为`20261109-192057-frag-authored-palm-pg`，SHA256 `6b1bae0ff5b6a340d00dada3e269936c07fe9c4f106021c6bb8b5d696f60a46c`；继续配已编译但未实录的Native `a21efda6…04b7a`。相对ccfd的191个实际编译输入，只有`weapon_frag_grasp_pose.h`变化：15个闭指旋转和独立profile_id `4652414753484132`，header SHA256 `67b9e4f65b6879a95aa2d3b1857b0f58c71962dbe391f436af7bc5e6d755754a`。原93设置／58GEARS保持，Native、V4格式、唯一C0、抓点和投掷机械代码无新增变化。旧ccfd的源码／二进制没有被覆盖。新的实际header独立MSVC Release/W4/WX编译读回15行，与选定姿态和原reference逐值误差0，30个四元数归一，错误signature拒绝且不写输出；通用V4／C0实现未改，没有重复未变的通用测试。此前root抓点改变产生的新释放点验收义务仍然存在。

原武器链动画也已单独重读：`COG_Bolo_Grenade.GrenadeAnims`六个序列中，GR_Prepare／GR_Loop／GR_Idle／GR_Release的root存储单键为零平移／identity旋转；Counter在GR_Prepare有40个平移键，Loop与Idle存储位置也有变化。因此原Counter不是静止握柄的可靠替代。这里只记录原动画键，没有读取实际blend／physics矩阵，Airborn的root多旋转键不在本次单键解码范围。原链条仍由游戏处理，不能据此声称动态链条已验收。

最新汇总`frag-self-contact-review.json`、数据表检查`20261109-192057-frag-authored-palm-pg/profile-check.json`与各有界试算保留所有中间失败。6b1未申请新队位、未stage／run／root合入／手动交付；当前只等待172809原准备退出修复的最终交接。Frag全需求和头显体验仍未完成，完成Frag后才开始Smoke的实现及实录。

后续对原Counter位移键作有限反例检查：由每段原export字节和独立SHA校验重读GR_Prepare的40个连续float平移键，以及Loop／Idle／Release各1键；计43键，逐段验证root零位移／identity与Counter单键identity，使用原绑定Counter矩阵差值变换实际Counter三角面。故意把6b1手型在每个键都固定为闭手，1395手面与Counter面的超过1mm非共面交线计数均为0。此结果保存`counter-key-clearance.json`；它仅覆盖这些存储键的原几何，不代表实际blend／physics、链条／尾球、键间连续面、真实缩放或开合手过渡通过，也没有改变任何原链动画或私有制品。

2026-11-09腕方向更正：前述400／408物体交线数字、identity腕方向图与首轮Counter键检查，只是该离线摆放条件，不能称为实际挂载几何证明。检查真实`WeaponMeshFollow::bind_mount/solve`才发现生产`weapon_frag_grasp`一直硬编码Rx180，模块内新写的identity方向未被消费。未改变原Rx180；作者数据改为同一Rx180，并由实际挂载函数消费这一个方向定义，避免再次分离。新的`root-runtime-basis-proposal.json`、`frag-hand-root-runtime-basis-proposal.png`以及`crossings-root-runtime-basis-proposal.json`使用实际转置方向，亲看掌／侧／斜视图，物体交线仍为0；原手指数据及原reference逐字不变，所以限定自身计数0仍成立。43个Counter存储位移键在正确方向重新复核也全为0，另存`counter-runtime-key-clearance.json`，旧证据保留。

最终隔离PG位于`20261109-193756-frag-authored-basis-pg`，SHA256 `29231113cbfb6746ac8cb2ca5e78ba82b6c94108b75782226bf998cf0a7c6de9`；仍配Nativea21。相对6b1的191个实际输入仅两header变化：数据中的腕方向／独立profile_id4652414753484133，以及实际grasp从数据读取方向。实际挂载方向、中心和15指旋转与6b1保持，没有追加释放点或机械变化。受控实际old/new挂载检查先复现旧数据／consumer不一致exit2，再以原`bind_mount/solve`在2个单位比例、3个mesh比例、3个控制器旋转、3个网格点上核对54个腕内点变换全部通过。夹具首次用错类型ViewAngles的编译失败已保存，仅改为既有Rotator；没有改生产守卫。它们仍是受控几何，不是实际Native、追踪、画面、动态混合或整体贴合验收。此前相对官方5d32更改root抓点的释放验收义务仍保留。最新汇总为`frag-runtime-basis-review.json`；2923未申请新窗、未stage／run、未改手动包或公共制品。

2026-11-09编译输入归档：Nativea21与PG2923分别保留383／191个实际C++输入、Native的6个MASM输入、原23个MinHook文件、两个DLL、Native map及原MSVC读取／命令记录。另存相关几何和未来持有视角脚本，共625文件、23,171,607字节；清单SHA256为`783c79c064b015e6833e142ba937fce11450d12e35e44d5e9e78a515e9643ea1`，目录`analysis/frag-grip-20261108/frag-handle-static/compiled-input-capsule-20261109-211747`。两套实际头文件分别保存，没有用文本归一化替换原字节。45个共用输入中14个原字节一致、30个仅换行／BOM不同，另一个差异仅为Native已有左副握持inline helper；实际V4请求／帧格式相同，PE发布入口与Native map一致。这是源／PE／编译断言证据，不证明实际发布成功、动态效果或手型通过。

首次归档因错写MinHook文件数14而在发布清单前拒绝，原未完成目录211348保留；已依据两个原清单完全一致的23项修正并成功归档。归档不包含完整SDK／系统链接环境，不声称可直接重建，也不是可stage的prepared候选。完整1395面检查仍保留37对关节附近交线，不能沿用先前只检查不同解剖组的零交线结论。当前200930准备退出候选不变，待前序最终交接；此归档没有新游戏、stage或运行，烟雾实现仍在Frag完成之后。

随后对已发现的关节折叠作一次限定8姿态试算，保持抓点、实际Rx180、全部原平移和链条不变，只调整局部关节旋转。完整1395手面／842模型面重现基线自身37对、物体0对；统一PIP舒展12度降至自身17对，却新增17对物体交线。MCP舒展4度加PIP8度虽为自身17／物体0，手指更松，仍不是可用修复。其余姿态也未消除自身交叠，全部不采用。九幅深度测试掌／侧／斜图已亲看；图中红色是相交涉及的三角面，并非每处实际可见交线。另列掌部和远端骨主导顶点到原root三角面的最小距离，只作样本诊断，不冒充已注册指腹、连续面间距或穿入深度。新姿态文件不继承父级旧solver成功／旧零计数字段。证据为`frag-joint-fold-study-review.json`及`frag-handle-static/joint-fold-study/`。不改Nativea21／PG2923或已排队200930；后续须逐指确定实际接触和弯曲分配，同时检查自身与物体，实际游戏贴合仍未验收。

原37对交线随后逐条取25个内部采样点，沿相同六个方向对原手部及实际方向的root／Counter作射线遮挡检查：两面遮挡条件下共有18对暴露采样点，其中13对涉及拇指。亲看`crossing-segment-visibility.png`六图，仅放大标出实际可见采样点，不再把整片参与三角面涂红当作交线。六个解析射线对照通过；两种单面剔除符号另列，实际材质剔除、连续全角度、实际LOD／缩放／袖口和像素仍未认证，没有因为某视角遮挡就忽略其他交线。

针对拇指作3个局部旋转分配试算，保留同一远端原三角面重心作为作者标记，调MCP／DIP补偿PIP舒展12／24／36度。虽数值收敛，实际完整面检查分别仍为自身31／26／26对，物体13／26／30对，全部拒绝。追加独立Counter凸包约束后的3次结果相同；再将实际相交处39个重心坐标表面采样纳入一次PIP12修正，仍为自身33／物体14对，拒绝。检查发现部分新交线位于主导骨为掌部35、但仍受拇指权重影响的原三角面。有限顶点／边中点／面重心和标记误差不能认证整片表面避让，solver成功也不作通过。后续应协调拇指、混合权重掌面和握柄接触，而非继续统一舒展或只锁定末端标记。原试算源码与全部失败保留于`thumb-fold-study`、`thumb-fold-counter-study`、`thumb-fold-cuts-study`；汇总`frag-thumb-fold-review.json`。没有新编译、部署、运行或修改已排队200930，Frag与Smoke均未标为完成。

后续将握柄与拇指协同调整，而非仅在固定握点上转指。先验证1／2／3mm单轴移位都未解决穿插（物体分别4／9／11对），再在4509个全手顶点／边中点／面重心和39个已知交点作者采样上联立求解。PIP舒展12度得到自身29／物体0；相同六方向暴露交线由18对降至11对。新腕内握点相对旧值变化`(0.1442384992,-0.25,0.1)`原单位，约3.05mm，Y／Z作者边界已到，不能据收敛声称最优。六图已亲看，仍有可见交线；单面剔除、真实缩放、LOD、腕袖、动态和实际像素仍未认证。随后PIP24替代虽自身26对，却新增3对物体交线，拒绝。保留`thumb-mount-fit/pose.json`只作后续作者基底，未替换Nativea21／PG2923、未编译／部署／运行。任何采用该握点偏移的版本都须重新验证释放原点、角速度贡献、原生成／扣弹once、碰撞效果和返回过渡；仍只从原始控制器轨迹取运动。汇总`frag-thumb-mount-review.json`。当前200930准备退出候选、手动包和publicv71均不变，Frag未完成，Smoke尚未开始。


食指后续局部检查：在thumb-mount29／0／11基底上，仅舒展食指PIP并补偿MCP，8度与16度的自身交线分别25／28对，物体仍0，但原中节三角面作者标记偏离约2.88／5.57mm。重读发现该标记起点自身距原root约6.18mm，锁住原点并不等于贴合。改用原root实际三角面的最近点，加0.05原单位外侧距离作作者目标；食指更靠近握柄，但完整检查为自身30／物体0，六方向暴露交线增至15对，亲看六图后拒绝。三份局部方案均不采用，保留原thumb-mount29／0／11作后续作者基底；作者标记不是已认证指腹，原单位按1cm假设换算，实际尺度和像素仍未验。详见`frag-index-contact-review.json`、`index-fold-study`和`index-surface-target-study`。本轮没有新编译、stage、Game或手动包变更。

串行运行状态另有外部变化：上游Boom191450的队列记录已为`superseded_before_run_for_owner_required_retail_path`，其会话明确正冻结购买版路径派生计划。此事已向直接前序LS核对，Frag200930仍是LS201902已受理的唯一直接后继，尚无最终交接；不以旧Boom候选完成或当前系统idle推定窗口，也不擅自接受其替代身份。Frag自身既有冻结tuple和用户明确授权的验证副本范围不变，仍须新的直接最终交接、完整预检、独立idle及自有基线后才可部署／启动。


2026-11-09 22:04，将thumb-mount29／0／11姿态接成新的隔离Playground构建，目录`analysis/frag-grip-20261108/20261109-220423-frag-authored-thumb-mount-pg`，DLL SHA256 `3fbb6e61d9cac2a2a0dc2d28510f80b721caee0b0252e9c782c348aa22057943`。配原已编译、未实录的Nativea21，Native未重建。相对2923的191个实际PG C++输入，以及1055个完整保存源码文件，只有`weapon_frag_grasp_pose.h`变化：profile_id4652414753484134、闭手拇指三关节和约3.05mm腕内握点偏移；原reference、其余手指、所有骨长和平移及Rx180保持。该握点数据会影响释放点与角速度贡献，不写成单纯外观变化。原93选项／58GEARS保留；新静态MinHook库实际10个项目编译输入和PG链接读取记录分开保存，不混入PG191个C++输入数。

实际新header由独立MSVC Release/W4/WX程序读回15组reference／closed行及握点、方向／profile ID，数值误差0；无效signature拒绝且输出不变，单位四元数检查通过。实际`WeaponMeshFollow::bind_mount/solve`在54个单位／缩放／旋转／网格点组合上均与新作者几何一致。它们只证明编译数据与实际计算消费一致，不证明Native实际采用、硬件像素、动态或贴合。原旧姿态43个Counter存储键检查不自动继承给本构建；完整手型29对自身交线、六方向暴露11对仍保留，腕袖／实际尺度／链条及新释放生成扣弹／碰撞效果／返回过渡待验证。

本制品没有申请新队位、stage／run、root合入或替换手动包／公共v71。已排队200930仍为PG0a8／Native5e51的原准备退出短测，本轮其完整现态预检通过，不等于窗口。直接前序LS明确其旧201902还因4份Python边界漂移待不可变派生与真实前驱重新独核；同一LS→Frag唯一后继顺序保留，替代身份待确认，旧tuple仅作历史，未收到最终交接。主要记录`frag-thumb-mount-product-review.json`。Frag和Smoke均未标为完成。


2026-11-10 05:33–05:35 actual2956（creation134387335897716387）已用当前ca5e/a21完成一次真实combined。持续右Grip、trigger0完成准备；四个原controller旋转视图、真实松Grip后原调用一次生成/通知/扣量4→3、计划与原初始出生位置/速度一致。原停止到爆炸1.1290054321游戏秒，原字段0.1+1秒；原目标200→−200、玩家450→192，保留原自伤。原返回后右手空且张开，新的口袋Grip再次持有，库存仍3、没有第二投。两次原arc字段false。该实践夹具的原AI/碰撞/health保留，但战斗过渡/感知计时器事先停用，不声称原战役战斗覆盖。14892首准备失败本轮未重现，原因仍未证实。

Native/entries/input/weapon drain/XR0/launcher0正常，十原件exact恢复，单一paired encode(session50644)0，retention(session56278)0/errors[]，保留2956/21120/37448最新三局和48小时。4张全跨度paired overview和20张实际接触图已亲看；游戏爆炸帧与后续双路碎裂可见。第一组持有掌指多受近敌遮挡，右侧近场袖/前臂占画面较大，完整手型/腕袖/全角贴合仍未接受；末尾新持握视图短，不能外推完整旋转贴合。现有C0 first32回执结束于44842408360900ns，早于44892658153700ns held views，不能据此诊断后段腕旋转。normal封存在analysis/frag-grip-20261108/adoption020237-v3-normal-closeout.json（bdbf817b60d68cab8f4f4700fa4c6f86e0ff816b3a04f7d42d21c3f2a810f0a7）。当前是动作链通过，完整Frag、物理头显与Smoke仍未完成；publicv71/手动入口未升级。


2026-11-10 在Bol已确认接管后，仅从本枪冻结2956源码派生4份离线检查驱动。新held_review分派直接调用已有真实口袋Grip／原准备／原controller插值，取消本观察中的近敌接近和投掷；保持2956末尾较清楚的原grab静止锚点(.24,−.25,−.42)m，四旋转各稳定2秒，全程Grip1／trigger0，库存前后相等且原launch必须0，最后由原owner正常停止。失败不补输入，沿用原启动／cleanup保护，没有新Engine调用、writer、Native或PG变更。四文件语法检查通过，位于analysis/frag-grip-20261108/unobstructed-held-review/driver，driver-plan.json记录准确摘要。目前仅隔离源码已接通，尚未冻结新runtime产品／申请顺位／stage／run；不能当实际贴合修复或再度准备通过。


2026-11-10 06:44 冻结equipment-controls-prepared-20261110-064450-frag-held-review：prepared794d0e2c / plan0e8cdac7 / ready716877b9。新物理check/operator薄入口从同一冻结目录加载源，以原authoritative root basis执行现有驱动；不写共享入口，保留原启动／双路录制／正常停止和恢复。7项实际入口委托/错误source及focus/缺actions拒绝/实际startup重定向编译检查通过，未执行runner/idle/进程控制或游戏。保存源PG1092/Native1563，实际编译输入仍PG191、Native383+6ASM和原93/145flags；6二进制逐项匹配a21/ca5原实录产品。当前共享环境预检仍pending，不能把冻结产品当运行许可。已仅向原实际Bol交接方递送新意向c929d58c，明确追加在原全部后到序（最后Snub051707）之后；无执行绑定、claim、stage或Game。


2026-11-10 后续在等待实际窗口时，继续原皮肤离线定位。当前已编译profile4的863手顶点／1395面由原export字节、115reference及原权重重建，最大误差5.684e−14；29交叉全部同指或掌指混合区，没有邻指交叉。按具体三角面分离轴约束与新增交叉反馈作有限局部修正：拇指阶段29→23／固定六视角可见11→5，食指阶段23→21／可见5→1，原root+Counter仍0交线。两张六视图均亲看；21处仍包括食指8／中指8／无名指5，不忽略被遮住的交叉。首次只约束旧6面的32处结果、中指自由旋转28处结果以及hinge仍21处结果均保留未采用。该研究数据未生成生产header、未编译或替换064450，部分参数到边界、IP扭转自然性和真实接触仍未接受；不是完成手型或游戏验证。原抓点／释放几何／T/S／蒙皮及全部runtime权限不变。新证据analysis/frag-grip-20261108/frag-hand-contact-20261110/progress.json；目标保持active，Smoke未开始。
