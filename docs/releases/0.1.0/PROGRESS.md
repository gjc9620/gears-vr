# Gears VR 0.1.0 开发记录

## 武器 6DoF 独立分支（2026-10-22）

2026-10-23 用户最新安排：当前运行环境有问题，先暂停本分支的游戏／模拟器测试，等待 0.1.0 发布完成，再将 `codex/0.1.0` 合并到 `codex/weapon-6dof`，同步运行环境后继续验证。本轮没有创建 k 入口或启动新的游戏／模拟器，也没有留下本分支的测试等待器；不自动重试 i／j，不修改另一会话的工作区或默认启动器。现有 R8 冻结候选与 Longshot 离线 stage 保留，离线源码复核不构成运行验收。此安排暂时覆盖此前允许随时进行后台测试的偏好，不代表整个 6DoF 功能已完成或放弃。

Longshot 原生矩阵转换离线部件完成：明确米制 RH 相机到 native 行向量／+Z 的坐标基、原点、单位和有限 normal／reverse 深度，不复用主眼投影。严格限定 CPU 构建通过 84 场景／19,737 检查与 2,736 个独立世界点，另两次构造参数缓冲区调用仅为本地 C++ 函数。初版 80／81 场景结果保留；Astra 发现近正交 float 旋转的转置恢复可能把实际约 0.359 mm 相机误差报零，改为求解实际矩阵 `C·R=−t`；另拒绝量化后有限远面退化成无限远的两种情况。三项回归实际调用旧 `c929`，确认旧版误准入、新版拒绝且输出不变；旧失败审查没有改为通过。最终头 `ba2e767a…8117e5`、报告 `e925ecb2…032ee9`、Astra 修正窄审 `1458ba14…63716c`、root 收尾 `8a858a5b…efbaa7` 保存在 ignored `analysis/longshot-native-matrices-20261023/`。协调者核对 13 个合同／证据输入、17 项保守记录输入和 63 份支持工件及实际严格编译项目；记录输入不是编译器精确 include 清单。未应用 live、修改冻结742或启动 Native／XR／游戏／模拟器。

该矩阵部件当前只支持两眼均合格的 pair，尚不能覆盖常见单眼进入眼盒的实际瞄镜场景。下一独立离线 stage 增加按原眼索引输出的单眼转换，并复用已修正的数值路径；原生单 source／family、独立目标、跨线程生命周期、GPU 和镜片仍待接，不把此部件写作瞄准镜完成。游戏／模拟器测试继续等待 0.1.0 发布及合并。

暂停运行测试期间完成 Longshot 独立 view state 的离线部件与 Astra 窄审：私有默认关闭的 `split_source_views.h` 路径在所有 factory 回调前冻结主／scope 左右四个指针，拒绝空值及交叉复用，实际构造与输出匹配共用快照；旧 null／OFF 路径、map／资源／回收保护保持。新增 30 场景／2,028 检查、既有 factory OFF／ON 严格 CPU 检查通过；协调者复核 live／original／stage 各 252 输入、69 份支持工件及成功程序实际字节。原始预处理差异、测试侧构建失败及协调者误读 before-map 根目录导致的只读核对失败均保留，未重写为首轮全过。报告 `a954e445…9b4cdb8`、Astra `3b02aa2a…aff437`、root `dbbb8b0b…bf03c` 位于 ignored `analysis/longshot-independent-sources-20261023/`；候选未应用，未构建 Native／XR或启动游戏。真实 state 生命周期、完整 family、独立目标、跨线程 renderer 路由和镜片 GPU 读取仍待接。

交互设计同步明确 Longshot 的独立相机／场景提交／镜片读取三步，并修正换弹验收表：截止时刻包含等号，超过才算普通。旧声明核对段改为历史过程，当前 Lancer 实例约 0.8 秒的既有 Getter 输出已重新只读核对，未新增游戏样本，其他武器仍不得沿用该常数。下一离线部件为 scope 米制相机到原生矩阵的显式坐标转换；游戏／模拟器继续按上述安排等待。

2026-10-23：j在外部游戏退出后调用一次入口，正式零售EXE验证已过，但游戏创建前被多个用户INI的旧身份识别拒绝；无birth／Guard／Native／XR及手／枪写入，owner／子进程退出、私有INI及正式EXE／配置不变，51文件＋3元数据封存。已最小带入当前默认启动器已有的离线Steam身份fallback，仍须活体且稳定的客户端与唯一离线选择，不猜账户或改登录状态；19项检查及真实受保护配置只读status通过。未移入无关UI／安装绑定，未改共享默认文件；742冻结源、R8 DLL与Native／XR不变，下一入口使用新编号，未取得R8实际跟枪效果。

2026-10-23：R8按应用commit319a3bc封存742源／164工件，manifest `9d70b035…45dd168`，Native／XR沿用R7。h等待期间修正测试入口把无子进程GUI当owner的误拦：精确路径／SHA＋完整进程亲缘证明才例外，真实游戏、CLI／AVP／XR等仍拦，两次启动前检查保持；13边界检查与独立窄审通过。i实际入口因原零售工作副本路径已不存在，在validate_game、创建instance或启动任何游戏前拒绝；手／枪0写、owner退出，失败56文件封存，未改写为枪体通过。新j固定现存Steam-managed零售安装，增加EXE／配置pin，已有194依赖／26复制／24模块Popen0准备；只做核心文件只读校验，未重开兼容调查或重跑旧套。尚无真实R8枪体效果，普通启动器与冻结Native／XR保持。

2026-10-23：R8仅把既有私有名称读取scope覆盖component初始准备块，在lease.acquire前结束；早返恢复父TLS、faulted入口和invoke／rollback旧范围保持，wanted仅纯声明外提。原getter／调用顺序、R7计时、同Plan与100ms均不变。实际提取58场景4,818检查、Astra窄审及唯一严格ON构建首编首跑通过，23导出与R7同，DLL `a0551696…afded5aa`；OFF／default完整EP原bytes相等，未重编或重跑旧套。协调者核对742源＋16vendor、60支持工件、实际MSVC工程／输入路径／PE导出及旧R7和g封存后按字节应用一源，复用成功staged DLL。报告 `f163b548…d9a9731b`／应用复核 `3e25cb3a…308cc8d`，真实build base96622b6；root证据map形状错误在写入前拒绝并留存，修正后通过。尚无R8实际游戏局，不宣称解决g源证书超期，Native／XR与普通启动器保持。详见[组件适配器记录](../../weapon-mesh-component-lease-20261023.md)。

2026-10-23：R7购买版后台静音g/PID8620首次取得三类实测，枪体事务仍未通过。首失败ordinal165／group4，known127／checks2047／reject1024，对应commit的certificate_recent：毫秒live／commit338683531仍比until338683537早6ms，但同域NS下stereo／input接收时间已老103.9139／103.8253ms；Policy已求值mask0，不能唯一推断其内部全部原因，后Stop11／Shutdown12未执行。10行成本同lease1／seq793／revision20，initial8.8377ms、三snapshot2.4178／2.3178／2.3091ms，六个内嵌name scan共1.0138ms，不能再加到snapshot总和。initial→末snapshot的QPC窗口60.9642ms，非重叠已测15.8824ms，间隙45.0818ms尚未唯一归因；沿pose_bridge.h直接QPC/QPF换算，initial→commit69.7245ms，未混用GetTickCount。原partial Absolute＋Translation／Rotation未调用、acquired0／restored1／setter4尝试／fault1、手0写及最终clean保留。独立30项仅本次失败／恢复／冻结输入／成本记录核验通过；原值、九Key／8控件／44附件／两入口、Stop／Shutdown0、9会话静音零错误／pending与精确退出后Guard0、CIM无自有残留均确认。FPP47／GHW1 matched66、published52、rejected14；whole／component=false、Native入口verifier false／XR1／图像0保持。receipt `781f7696…751a19a`／独立摘要 `7a7eb696…98aff3`，57文件＋7外部封存，旧f的56＋7不变。R7封存742源／162工件，PG `ba40192d…ec8fe1b7`／manifest `11113b4c…8973cf8`，build base2327f52与应用432dd53分开。下一步仅补齐初始准备块现有名称读取scope，在acquire前结束，保留原生回调／rollback关闭；预期只影响8.8377ms中的解析部分，不承诺过关，尚无R8源码。

2026-10-23：R7仅新增三类有界QPC诊断：component初始块、各组Forward snapshot及其中name_value扫描，固定64行、invoke回调暂停、fault／null不采样；首个前向失败先pending，已开span收尾后frozen，双journal clean后一次dump。QPC失败／倒退、容量满及嵌套计时不参与权限，原getter／调用／短路／Stop与100ms不改。实际wrapper59断言13场景、独立counter163断言18场景及Astra窄审通过，均首编首跑；唯一严格ON构建成功、23导出与R6同，DLL `ba40192d…ec8fe1b7`。OFF／default实际EP仅空白行差异，未重编或重跑旧套。协调者核对742源＋16vendor、60支持工件和f的60份证据后按字节应用4源；报告 `b711a32a…c48f46e`／应用复核 `1762a053…8dc0fe2`，真实build base2327f52，直接复用同一staged DLL。当前尚无R7实际计时局，不宣称已定位主要开销或修复跟手，旧f失败、Native／XR与普通启动器保持。

2026-10-23：R6购买版后台静音f/PID57956仍在Rotation调用前超期9ms，首失败ordinal165／group4、known67／checks3／reject2，live334506796>until334506787。ticket_ms334506687、plan_ms334506703、context_ms334506781；源码明确plan_ms是Plan保存的Context快照观测时刻，93ms仅该快照年龄，不是Plan构造或组件事务耗时，不能用e的109与f的93宣称优化16ms。NS／Policy／后续Stop门未执行。Start0／手0写，组件acquired0／restored1／setter_calls4／fault1，pre_restore为Absolute＋Translation目标、Rotation原值；独立25项仅本次失败与交还检查通过，最终raw三组／九Key、8控件／44附件／两PG入口保持，Stop／Shutdown0。FPP44／GHW1 matched57、published45、rejected12；9会话静音错误／pending0、精确退出后Guard0、CIM无自有残留。原component／whole=false、Native入口verifier false、XR1／无图像保留。receipt `e5f483eb…1e90185`／独立摘要 `64fb8533…0b7374a`，56文件＋7外部封存，旧e的55＋7核对不变。R6封存742源／161工件、PG `8079a0d3…5cefdbb8`／manifest `08c4bf81…65986c6`，build base41d7c96与应用commit4d00626分别记录。现有write_evidence仅0.1031ms，Forward snapshot、骨名扫描和组件初始块尚无单独耗时；下一步只加三类有界QPC诊断，不做第三次猜测性优化，不改100ms或普通启动器。

2026-10-23：R6将页内名称读取扩大到私有前向Backend调用范围：read／set无条件设置并恢复TLS，invoke从admission到原调用及after observer全程关闭；全局name保留同一份Name／Array／entry前缀，仅scope内改变文本复制。R5局部lookup、原getter／setter与动态检查顺序、100ms／同Plan／Stop均不改。Astra源码复核、实际提取102断言14场景、独立页33场景396调用均首编首跑通过；PG ON唯一严格构建成功，23导出相同，DLL `8079a0d3…5cefdbb8`。OFF／default实际完整预处理仅空白行不同，其余逐行bytes相等，原raw比较断言失败保留，不计PG编译失败且未重编这两配置。协调者核对742源＋16vendor／59支持工件与e的58份证据后应用两源；报告 `3954136c…51666bc`／应用复核 `925b4976…c6d27d5`，build base41d7c96。直接复用已核对的staged DLL，无应用后重编。当前尚无R6实际游戏局，不能据字符复制优化宣称追回9ms；旧e失败、Native／XR及普通启动器保持。详见[组件适配器记录](../../weapon-mesh-component-lease-20261023.md)。

2026-10-23：R5购买版后台静音e/PID27852仍未通过枪体事务。首失败仍ordinal165／group4 authorize最早时限门：live332160546>until332160537，超9ms，ticket_ms＝plan_ms332160437，context_ms332160531；NS／Policy及后续Stop条件本次均未执行。d的ticket age125但plan age109，e两者均109，不能把25→9ms误作R5加速16ms的对照结论。Start0／手0写；组件acquired0／restored1／setter_calls4／fault1，pre_restore为Absolute＋Translation目标、Rotation原值，最终三组原值／九Key、8控件／44附件／两PG入口保持，Stop／Shutdown0。独立25项通过；9会话静音守护错误／pending0、精确退出后Guard0、CIM无自有残留。FPP49／GHW1发布46；原component／whole=false及Native entry verifier false、XR1／无图像保留。receipt `db7a409d…6b16545`／摘要 `9c3203e3…1f4cc60`，55文件＋7外部证据封存；旧d不改。R5封存742源／161工件，PG `dde8242e…edc45133`／manifest `67a931ac…3bf73ab`，build base696892d与应用commit09e7acb分开。下一步只读分析剩余Args／schema／Forward／四槽解析开销与私有scoped bulk name方案，不放宽100ms或刷新事务票据，尚无R6源码。

2026-10-23：R5私有组件名称解析优化已按字节接入独立分支，尚未运行新游戏局。逐字copy改为系统页内分块、NUL即停、奇地址跨页保留原2-byte读取，copy失败不重试；不缓存游戏元数据或结果，保留40／4096首匹配及全部typed／Key／links／slots读取。仅forward且未faulted的组件读取走快路径，内部fail／外部null回滚与诊断、capture／world仍旧resolver，Args／schema／Forward／100ms不变。实际提取动态111断言35场景、独立页33场景132调用、Astra复核均通过；页fixture一次ADL编译失败保留，PG ON首编成功、23导出，default和phase1 OFF预处理原字节相同。协调者核对742源＋16vendor／59支持工件及d的58文件后应用一源；成功DLL `dde8242e…edc45133`直接复用，无应用后重编。报告 `86651a1f…5edd39b`／应用复核 `27db3cd9…22e3bd`，真实build base696892d。字符copy次数减少不等于实际性能通过，下一局仍须验证是否消除d的25ms超期；旧d失败与原普通启动器保持。

2026-10-23：R4购买版后台静音d/PID5612已取得明确首因：group4（Rotation）在实际调用前的最早时限门拒绝，live_now328843921大于write_until328843896，超25ms；ticket_ms328843796，期限为其＋100ms。该次未读取authorizeNS／commitNS、未执行Policy及后续Stop／Shutdown条件，不能把失败归给这些未执行门，也不能用ms与sourceNS混算源姿态年龄。固定首失败ordinal165在339条观察／63条尾部覆盖中保留。Start0／手0写，组件acquired0／restored1／setter_calls4／fault1；pre_restore支持Absolute000→110和Translation→目标、Rotation仍原值，最终三组原值／九Key、8控件／44附件／两PG入口恢复，Stop／Shutdown0。协调者25项只读复核通过，scope=`rotation_precall_deadline_rejection_partial_component_and_rollback`；component／whole=false、Native entry verifier false和XR1／无图像均保留。FPP55／GHW1发布47，9会话静音守护错误／pending0，精确game退出后Guard0，独立CIM无自有残留。原receipt `da10f0c4…7d643ea9`／摘要 `100824db…2a93234`，55文件＋7外部证据封存；旧a／b／c不改。此结论仅属于d，不能补写c未记录的失败原因。下一步限组件私有元数据解析缓存，仍逐次读取完整Key／links／slots／schema／expected与同Plan时效，不放宽100ms、不集成普通启动器。

2026-10-23：R4仅补同Plan Lancer组件的首失败诊断，尚未修复或重新实测c的根因。固定首失败＋63条有序尾部记录复制既有gate／值比较／journal／调用结果和授权时钟，无额外getter／frame／clock／atomic或权限调用；pure acquire读区分setter内部旧值与真正expected差异，双drain结束后重试输出，避免回滚覆盖首因或clean Shutdown前丢记录。Astra两项finding已闭合；实际提取的合成检查Auth274／Backend69／Drain10通过，12组原版与诊断操作序列相同。严格ON编译真实为2次尝试：首次C4459局部参数遮蔽失败，机械rename后成功；原失败输入／日志保留。成功PG `a8276f74…67c1af8`／23导出，default预处理原字节相同，phase1 OFF仅空行和同一个return花括号差异；未重复OFF/default构建。协调者核对742源＋16vendor／76支持工件和真实PE／项目后按字节应用3源，复用该staged DLL，不再编译相同输入。报告 `62c32832…f29475e`／应用复核 `09c77441…eeb206e`；构建base仍17709c3，后续应用commit另记。R4尚未启动游戏，旧a／b／c证据不改，普通启动器未启用。详情见 [枪体组件租约记录](../../weapon-mesh-component-lease-20261023.md)。

2026-10-23：R3购买版后台静音c/PID8120完成实际局。Start0，首次写前认证及postflush已通过，取得1张planned pose；之后 `component_acquire_failed`，手部0写入。组件记录0完成acquire／1次交还／2次setter调用尝试、sticky fault1且最终dirty／owned／revoked均0、安全卸载1；不能把2次尝试解释为两个setter成功。唯一pre_restore表明absolute标志000→110，Translation／Rotation仍原0；最终独立raw快照确认三组及九Key保持原值，故只支持这次部分absolute更新后的交还，不是完整枪体跟随通过。Stop／Shutdown0、原8控件／44附件／两PG入口保持，FPP累计61／Native GHW1发布50；9个会话静音守护错误／pending0，精确游戏退出后exit0，独立CIM无残留。协调者21项只读收尾通过；原component／whole=false保留，receipt `be31b13b…adcd367`、摘要 `2944a33d…3b19edf`。实际失败的Backend gate和时刻尚无记录，不能唯一归因于超时、expected值或调用结果；下一步仅补首个失败的有界诊断，不放宽认证。R3封存PG `64956c11…4c47689`／manifest `cf760058…488550`，旧a／b不改，普通启动器未启用。

2026-10-23：R3仅在两处手部／枪体动作入口前移纯名称分类，成功解析的非动作getter不再做组件关联查询，共用原46动作集合；GT、一次性self token、unknown fallback、InvManager／八对象边界及双journal交还均保留，Policy／Bridge／100ms／最后授权不改。Astra窄复核无阻断，实际函数提取548检查通过，替回R2 admission的反例rc1捕获多余getter读取。staged ON严格构建一次，DLL `64956c11…4c47689`／23导出通过；default实际MSVC预处理原字节相同，combined ON／phase1 OFF仅空行与相同return花括号，窄归一化相同，故未重编这两种配置。协调者核对742源＋16vendor／25份支持工件后按字节应用两源；live742与实际编译输入完全匹配，直接复用该DLL，不重复编译同样输入。报告 `bfcecb70…b9e94c`／应用复核 `1d94db80…002515`。尚无R3实际游戏局，a／b失败及普通启动器保持原结果。

2026-10-23：R2 Lancer 枪体候选两局购买版后台静音测试均未取得实际写入，原失败不改。a/PID17996 在独立基线读取器多查不存在的 `Weapon.WeaponMesh` 时停止，尚未 Start；新 b 读取器仅删除该多余查询，保留真正 `Weapon.Mesh`、八条 links 与九个 Key，38 helper／35 runner／56 audit CPU 检查通过。b/PID27712 已读取真实原枪、资源和三个 setter schema，Start0，但首次写入前授权未通过，最终以 `xr_certified_pose_rejected` 结束，手与枪均为0 leases／0 setters。38次 prewrite 表示已产生可接受 Plan，不能误称未绑定；最终 bridge rejected 的分支／包时间未记录，不能断言唯一原因。原8控件／44附件／两PG入口均保持，b 三组组件原值及完整身份亦保持；两个 Shutdown0。两局静音守护各匹配9会话，读／静音错误和待恢复均0，精确游戏退出后Guard exit0，独立CIM确认自有进程消失。a／b独立只读收尾分别20／19项通过，只证明本次失败与清理，不是枪体跟随通过；摘要 `cd6e54c2…ea3bfbf`／`22dfb6e4…7c60e39`，原receipt `26cc4852…341ceec`／`9e894c07…1ef6775`。已定位新增hook在名称分类之前对普通getter反复查询组件关联；正在独立副本缩减这项无副作用读取，不放宽100ms或动作边界，尚无修复后实测。

2026-10-23：同GHW1 phase1 Lancer枪体租约适配器R2已接独立分支，默认OFF且与旧writer互斥，三个setter与手IK消费同Plan并分别重验；ended后仍可双journal排水，真实动作未交还先撤权再保留ABI转发，自有请求未清则拒绝。Astra发现并复核修复InvManager换枪关联遗漏；staged三构建／pure278／真实提取生命周期41检查及相关反例通过。live ON在新目录仅编译一次，23导出与C1相同、4种拒配通过，742源前后一致；报告 `e32fb274…bf2d917`，DLL `25c9b9ed…7086ce2`。尚未运行此枪体候选或验证实际setter／跨Tick可见跟随，未启用普通启动器；详见 [组件适配器记录](../../weapon-mesh-component-lease-20261023.md)。

2026-10-23：持续双手C1购买版后台静音PID6568完成262写／262恢复，超过17秒和256次后继续更新，第四ACK后baseline258再到262；22项组件审计及独立原日志数学／计数复算通过。首窗184次跨mesh Tick／368手样本，最大位置误差 `6.84221e-5` 原单位、完整旋转 `0.023209°`；184张GHW1证书、本局Native发布318及XR发布2741核对通过。Shutdown0、原8控件／44附件／两PG入口恢复；9会话已静音、Guard零修改／错误／待恢复，精确游戏退出后Guard exit0，独立CIM确认自有进程退出。原whole=false保留：8×8／GPU capture0／linked0／XR图像0及exit1、Native entry verifier false；晚段逐手几何、final-write NS、真实Quest与画面验收仍未覆盖。首次入口别名校验在one-shot／游戏创建前拒绝，原证据保留，修补后30 runner／46审计CPU检查及Popen0准备通过，唯一实际局使用准备 `16e8783d…dc3306`。原receipt `92d56bab…55cdec`／独立摘要 `466aa11d…b6f0ea3`，详见 [持续会话结果](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：枪体组件可恢复租约新增纯C++候选，分组三absolute flags／Translation12／Rotation12，完整Key与expected重验、失败后读回、foreign／未知身份撤权、只读原值清账、sticky fault与其它owned组独立恢复。Astra发现并复核修复了两个公开getter作为wanted时的自身别名清零问题；修正版268项CPU断言、CTest1/1及两旧顺序反例通过，旧72文件保留。新CMake测试目标另在全新持续ON构建中执行一次、268检查通过，9份源码快照及旧C1工件核对无变化；集成报告 `ac7f10c2…0b4f59`，新持续DLL `425f6868…43e8a0a`，尚未运行新游戏。该组件不管理附件44字节，也未接游戏、射线或左手副握；combined与旧mesh-follow仍互斥。详情见 [枪体组件租约记录](../../weapon-mesh-component-lease-20261023.md)。

2026-10-23：持续双手源码候选已完成CPU验证，新增默认OFF的 `GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL`，要求certified＋combined。它把同一次固定实例Start的寿命与15秒／256次诊断预算分开，不新增writer或改变304-byte Start；每张Plan仍由三个原时间戳checked加100ms后的最小截止时间约束，全部最后提交检查、逐Tick恢复、动作交还及身份失效处理保留。热事件另限1024，诊断停止不停止认证；权威计数耗尽终止、统计饱和、Status计数钳位。ON／联合OFF／普通默认三构建、旧有限helper31场景／持续21场景／真实yield与phase块4场景、新Session及原Policy／Lease／Start／绑定检查通过；两种缺前置拒配通过。Astra发现的clean-but-ended后被资格分支覆盖为等待状态已修复并复核。验证报告 `analysis/hand-continuous-session-20261023/report.json` SHA `27be235f…361e6c`，ON DLL `172f2153…937ec3`。本候选尚未运行新游戏局，不能以h的限时结果代替持续运行验收；公开ownership／apply和普通启动器不变。

2026-10-23：source 0配套候选h已完成一局购买版后台静音联合测试（PID23528），组件24项检查及独立只读复算通过。25次手部写入全部跨网格Tick、25次恢复；收枪后左右手的位置／完整旋转响应取得8个新lease，取回武器后取得新绑定及9个lease，四次装备回执为11→10→11→10。50个双手样本的位置残差最大 `3.9101e-5` 原始单位，完整旋转角差最大 `0.021346°`，未放宽原门槛。25张GHW1证书均关联本局Native／实际XR身份；Native matched104／published98／去重rejected6，XR发布1126。Shutdown0、原8控件／44字节附件及两PG入口恢复；9个专属音频会话已经静音，守护零修改／零错误／零待恢复，精确游戏退出后守护exit0，独立CIM确认owner／game／XR已退出。原始receipt `c5a2d0ec…92c141` 仍为whole=false：8×8输出、GPU capture_attempts=0、原Native五项DXGI入口verifier false及XR exit1均保留。本次仅支持固定场景、15秒／256上限内的模拟器双手连续运动与装备交接；真实手柄、掌心校准、最终可见效果、持续跨世界和正式动作所有权尚未验收，尚未集成另一会话的默认启动器。私有摘要 `analysis/hand-equipment-1023h/verification-summary.json`，详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：启动链独立预检已完成22/22项（17项CPU／ABI／状态，5项独立DXGI／D3D12），未启动游戏或XR。准备实际连续模式时发现PG把可选R2调试宏误作手部来源选择，而当前连续host只允许runtime wire source 0；提交 `911c172` 已固定为 `StereoSource::RuntimeTracked`。实际初始化块在R2_SIM=1下的回归确认source 0两帧可建立绑定、source 1拒绝；31项提交边界、3种PG构建、11项组件检查、4种错误配置拒绝及737份源码核对均通过。新配套封存为 `222a9b2f…467bae2`，manifest `19af0b18…4b900ebd`，PG `fcaeca76…5e44e6d`；Native／XR保持同一已核对候选。旧 `b28d12a8…` 封存和失败运行证据保留，不能拿其source 1 PG执行新局。新h目录尚未进入游戏；实际owner仍须执行同次启动自检，不能用独立预检冒充运行通过。后台测试继续强制静音、窗口置后与只读观察，精确游戏退出后才关闭音频守护；普通启动器音频不变。

2026-10-23：连续性代码已分两项提交 `7ddb0f3`／`c5262b3`，737份项目源码及PG `6004449e…`／Native `ecd30c01…`／XR `886362d5…` 和对应回执已配套封存为私有bundle `b28d12a8…4d3ffe0`，manifest `7d1714ef…18b3e70`。8项启动helpers已构建但未执行完整启动自检；下一步是这套候选的启动链预检和后台静音实际联合测试，尚无新游戏局。源码树保持独立，未替换另一会话0.1.0的默认文件，详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：新增默认关闭的 GHC1→GHW1 双手连续性候选，分开保存几何校准与每次写入权限。XR 对每张源样本先累计 stream／每手 loss epoch，发布失败或 Native 跳帧不能抹掉中间失追；Native 在原 stereo_mutex 内、同票据回正后冻结实际参考 epoch／generation，逐字段比对新旧输入和真实 XR publisher，再连同两个原始源时间提交独立世界姿态。纯过龄可以暂停并保留不可写 mount；失追、回正、坐标／动作／身份等边界仍重建相应 baseline，坏新包永久封闭本次 Policy。写前仍有三道装备认证、日志后的当前 Context／双源 100ms／Stop／deadline 检查；旧 GVC3、GVE6、FrameLink 和 304-byte Start 布局保持。XR ON／OFF 构建及5项离线检查、Native ON构建及7项检查、Native OFF构建及7项检查／1种拒配、PG三种构建及11项检查／4种拒配通过；旧25场景原字节测试和新12组认证场景通过；机械提取实际提交helper的31个调度／停止／年龄／时钟回归全过，删除guard的17个、仅删NS回退检查的1个变异反例均被检出。冻结默认 Native／XR／loader 未变，当前未启动新游戏局，不能据此改写 f／g 的 motion 失败或宣布自由双手／完整6DoF已验收。后台实测继续 Mute1／Behind1／ObserveOnly1 与精确退出后音频恢复。协议与限制见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：182e7b0／c37e入口测量候选 g 购买版 PID31604 已完成25写25恢复／完整槽位往返／Shutdown0，4份诊断有效。66条entry全部known且same_state，PE入口→transport平均0.016773ms／最大0.0389ms，所有入口ms与transport相同；6次phase2旧票据在入口已经109／110／109／109／109／125ms，新15–16ms，age-only potential为0。**实测排除了仅前移当前refresh能补足余量的假设**；组件仍唯一motion失败，原failed receipt `978f271d…453d66` 不改。FPP累计80、原8控件／44附件／两Hook恢复及9个已静音会话零修改／错误／待恢复通过，game／owner／XR／Guard均已退出；Native entry verifier false／XR1／整局false仍保留。下一步应单独验证校准保留与当前写权限的区别，先补独立参考／失追边界证据，不能把未见loss或status镜像当区间连续性；详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：PE入口到现transport的只读测量已接线：最外层TLS scope先核GT，只保存const快照／时钟，本地指针仅沿当前调用配对；独立1024行缓冲，same_state比较原始stamp，零覆盖或clock错误不影响原权限与恢复。Astra静态复核无阻断，相关五种构建和65项独立entry审计自检通过；新候选尚未运行。late created_ms≤entry_ms仅作年龄条件，未在入口读取bridge，不能证明那张票据当时已发布／可读。另明确手柄→手骨刚体校准本身无100ms数学期限，之后可单独评估age-only suspend，但当前latest-only会遗漏中间loss且GVE6未包含Native reference_generation，不因此直接放宽现有候选。详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：bf95959／d456 连续性候选 f 购买版 PID56488 完整 25写／25恢复、11→10→11→10、Shutdown0 和原8控件／44附件／两Hook恢复；FPP累计86，静音9会话无错误／待恢复，精确游戏退出后Guard结束，自有进程全部退出。三份独立 timing／shadow／transport 审计有效，65条 transport 中24次调用、21次双手续龄；普通持枪／取回 phase1 的14次均续龄且零超龄，收枪 phase2 的10次为7续龄＋3早期过期（旧109／110／109ms，新15／16／16ms）。**组件仍仅持续运动门失败**，部分重绑发生在新姿态到达时，不能据此称跟手通过。原failed receipt `560457d5…330dfc2` 和原审计保留；Native entry verifier false／XR1／整局false不改。下一步仅测PE入口到现transport点的前缀时间、同Tick状态与早期票据可用性；Astra指出现有计时在transport之后才开始，不能直接证明前移能追回余量。100ms及三道装备／最终写入门均保留，详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：按 e 的早期新帧证据实现独立 transport continuity 候选，开关默认关闭且必须显式联合试验。最早入口以第三个独立 cursor 读取，只给尚未超龄且仍 tracked 的既有手绑定保存真实 created_ms；旧 101ms／future 先撤销，不能由新帧复活。调用必撤旧 Plan，不生成 Plan、不创建绑定或消费 baseline、不改历史 Context／ready 或 writer 游标；后置三次装备认证及最终 authorize 均保留。8 个固定 stream fence 阻止跨路径重放，合法完整同票据只允许正常 writer 重验一次，身份／action／ready 变化清例外。25 组 Policy 边界、九项组件检查／四拒配、连续性开启／关闭两种联合及默认／独立 stow 构建通过。Astra 发现的 bridge 消费后拒绝而未返回 Ticket 的跨 reader 缺口已补：首次建基以外的拒读先封写、按原路径恢复、再清计划／绑定并失败结束，不改 ABI；5 个编译期分类断言通过，最终只读复核无剩余确定阻断。独立 transport 审计 75 自检通过。尚未跑新候选，不能据此宣布持续跟手修复或完整 6DoF 完成；后台联调继续 Mute1／Behind1／ObserveOnly1 与精确退出后音频恢复。见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：只读shadow63317ca／ad22候选的e实际PID35740取得74条完整配对、0异常；phase2的9次双手age撤销在最早入口旧78–94ms／新0–16ms，晚期旧125–141ms但早期新仍46–63ms，全部same_state且无其他撤权；phase1另7次也全部early可用。54项shadow审计和实际结构检查通过，证据支持独立transport-liveness设计，尚未实现续龄或修复motion。23写23恢复／完整交接／Shutdown0及原字段恢复通过，component仍仅motion失败；FPP99、Native entry verifier false／XR1整局false保留，9会话静音正确收尾，所有自有进程退出。原failed receipt`0464570b…23be51`／shadow`ec76cc17…18af5db`保留，83项历史文件未改；详见 [早期连续性证据](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：restore前的独立cursor只读shadow已接线，最多1024条内存记录、结束后一次输出；常规observe后配对真实binding_age，并检查动作代次／phase／历史状态未变及早期新票据到晚期仍≤100ms。Policy新增const按值continuity_snapshot只查询，不续龄／消费游标或改计划；15纯场景、五种相关构建、九检查／三拒配通过。尚未跑新候选，也未实现liveness接口，不把诊断候选当作跟手修复。详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：诊断提交6467b1d／ccda候选的实际d局 PID31544 完整23写23恢复／11→10→11→10／Shutdown0，独立QPC块通过（23开始22完成1尾部pending）。phase2旧binding票据在observe平均126.667ms／最大140ms，9次明确age撤销；新票据平均48.556ms。pre_restore→observe平均35.275ms，restore9.806、单次三处装备认证约7.1、dirty stow2.149、原Tick8.241ms；这些inclusive分项样本不同、不可相加推FPS。写前证据约0.085ms，主成本不是该flush。组件唯一motion门仍false；8控件／44附件／两Hook、game_absent、9会话静音恢复均通过，Native entry verifier／XR1整局失败保留。原receipt`c867d64b…b1034c`、独立计时`7772c813…3e6306`不改写；尚未声称优化或完整6DoF完成。见 [测量详情](../../hand-ik-xr-stream-trial-20261023.md)。

Astra Max指出认证内部去重不足以独自补上26.7／40ms余量；下一步先在restore前用独立shadow cursor只读测传输连续性并配对稍后撤权，不改Policy／writer／Context。仅证实最早时刻仍≤100ms后才评估单独liveness接口，始终保留真实created_ms与后置完整写入授权；该shadow与接口当前均未实现。

2026-10-23：联合候选新增仅观测的15项内存QPC聚合和Policy逐原因／手别年龄诊断，终端一次输出，覆盖写前／写后证据flush与实际lease acquire；不改100ms created_ms续龄、15秒／256预算、Plan／authorize或任何装备门。12组Policy边界扩展断言、五种相关构建、九检查／三拒配通过，尚无新成本测量或跟手修复结论。后台测试继续显式Mute1＋置后／窗口只读，Guard保留至独立确认精确游戏退出；普通启动音频不变。见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：新候选64b的联合c实际 PID38884 完整11→10→11→10，23写／23恢复，取回新绑定、最终stowDirty0x10005直接Shutdown0及8控件／44附件／Call32＋Event64恢复均通过；FPP累计88，9音频会话无写入／错误／待恢复且Guard在game_absent后结束。**组件仍false：唯一失败是收枪phase2移动响应**，lease6–14全bind3，尚未保持同一绑定连续跟手；不得把完整交接流程称完整6DoF通过。receipt`a4336048…75ea83`／audit`eec68346…96c63c`保留。strict Native年龄等待实际两次约11.64／23.15ms取得≤1秒新状态，预算不延；23 runner／38 audit／10 pure检查与Popen0准备通过。Native clean但独立entry verifier false、GPU0／XR exit1和整局failed继续保留。下一步测量剩余认证／stow tick成本，100ms绑定语义不改；见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：联合b购买版 PID47248 Start0、槽11→10，原手4＋owned-detached4共8次双手写入／跨Tick采样并全部恢复，FPP累计63；但每次bind_mask3，移动被重绑吸收，未达持续运动／取回／dirty联合Shutdown验收。host Native复合新鲜度检查停止，失败四数缺失，不能唯一归因；原failed receipt `95a42eb4…3e18fa`保留。Shutdown0、8控件／44附件／Call32＋Event64恢复、自有game_absent和9会话静音收尾通过；Native clean但entry verifier false、图像8×8／GPU0／XR exit1均保留。Astra Max确认旧票据125／125／109ms触发按设计撤权；`f8ec02f`仅将Context变为纯构造，将六次装备认证去重为三个显式门。新候选`64b965a9…ca6798`（160源码、manifest`5e15c276…b2f40e`）strict构建／九检查／三拒配通过，100ms／15s／256不改，尚未跑新局。

2026-10-23：其他实例自然退出后，联合a实际购买版 PID46048 在双手Start前因运行器误要求全部8 strength为+0停止；冻结C++要求0／1／4／5为(+1,+1,+0)、尾2／3／6／7为(+0,+0,+0)，真实读数完全满足原生门。未Start、未槽请求、未Native／XR session，原失败receipt `d845dbf9…399fad`和audit保留。Shutdown0，8控件全字段／44附件精确恢复，Call32／Event64独立匹配、game_absent；9音频会话无mute／restore写入或错误，Guard在游戏退出后结束。正在新b目录仅修运行器／审计位模式，候选159947不变，尚无联合交互结论。

2026-10-23：联合源码已提交 `e8b3cf5`，候选160项 live／保存源码再次匹配。私有 runner 17检查、独立审计30自检、禁止Popen的默认prepare均通过，固定182依赖／23实际模块；Astra Max窄复核的FPP硬门、Stop异常仍Shutdown及静音Guard异常保活问题已修。三项后台标志显式开启，只有精确game_absent后才恢复原静音状态。准备回执 `a34c4706…31d96`；首次run在注入／私有实例创建前被另会话活动游戏的idle门拒绝，**尚无联合游戏结果**，不干预其他实例。详情见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：双手／暂收联合候选已接线并封存 `159947f1…97229d`（160源码），默认关闭，须显式 combined＋XR stream＋stow，其他 writer 仍禁配。稳定 owned-detached 可重新取得双手资格，pending／切换先清旧计划与绑定；助手前要求手journal clean，Shutdown 先封两方。foreign 撤权先于可重入 handback，已回原手路径新增只读 relinquish，旧收枪写权不随再detached复活；新显式acquire才可开新lease。strict联合／独立stow／默认构建、九检查、三拒配和导出隔离通过，Astra Max最终源码复核无剩余确定阻断。私有联合runner正在准备，**未跑联合游戏**，未变更冻结Native／XR、普通启动器或公开ownership；详见 [联合接线记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：同一 `121f116`／dc870候选的购买版 PID 18776 按钮联调 **passed=true**，36 正式读数＋6 pending 样本。实际 owned R2／L2 各自 Release 成功并变 up，随后原 BurstDelaying EndState→Active BeginState，自然完成 pending，再原助手暂收；4 槽确认 `11→10→11→10` 均有本请求唯一 ack/API。持续 both 的收枪／取回阶段各 0.6913／0.6744 秒，原键 up／AmmoUsedCount=6 不变；松开后新 both 才再次 pressed／耗弹。最终 dirty0x10005 Shutdown0，先恢复原44字节挂点再卸Hook，两入口独立还原、publisher/game/owner正常退出；九个原已静音会话无错，Guard 持续到 game_absent。修正的 private runner 28检查通过，fresh discovery首次0.2224秒成功，未实际覆盖churn重取。原 failed a/b 不改写；c receipt SHA `e9c1b0e6…07a1e4d`。候选仍默认关闭，开始双手驱动与稳定 owned-detached 的联合接线；Quest／最终画面与完整6DoF未验收。

2026-10-23：原开火结束等待与原子请求仲裁已提交 `121f116`，候选 `dc870cd1…1111f2` 封存 160 源码、七项纯检查及默认导出隔离通过。实际新局 PID 42312 在首个 Pawn.Controller 只读字段发现中因 Name registry changed 中止，Publisher／Enable／tap／helper／pending 均未开始；原收据 failed 保留，不能判定新功能结果或附件往返。Shutdown0、两入口原字节匹配、精确游戏退出、owner0和九个已静音会话无错清理通过；未抢占之前轮换的其他游戏实例。正在修正私有 runner 的启动期完整只读发现重取，不修改共享 Resolver 或重放在途请求，之后继续同一候选联调。

2026-10-23：真实原 R2／L2 联调 PID 57152 成功按下两键并消耗 Hammerburst 弹药，但开火中的身体槽请求被旧候选返回 12，整局收据保持失败；当时为 BurstDelaying，缺少 yield 分阶段日志，尚不能唯一归因。清理成功，静音保护持续至精确游戏退出。新修订增加逐手 Release 回执、最多 1 秒的原开火状态自然结束等待；等待先封新 press、没有附件写权，身份／挂点／原状态／按键认证失败即取消，Stop／Shutdown 排空后才卸钩。外部与 XR intent 统一原子 claim，等待保持 processing，终值只在读回与日志后公布。严格候选／默认关闭构建和新增 mailbox／pending 纯检查通过，尚待封存后实机联调；详见 [暂收试验](../../equipment-stow-trial-20261023.md)。

2026-10-23：请求确认竞态已由单请求 mailbox 修复，reserved／queued／processing 期间始终返回 0，原调用／读回／模型／完成日志结束才公开结果；真实跨线程暂停和 producer 竞争回归通过。新候选 SHA256 `81601d4b…80cdec` 的购买版 PID 36000 补测 **passed=true**：Hammerburst 槽 3 完整往返，外部 true 交还后 35 份样本持续 11.5607865 秒 detached／dirty=0、无旧 deadline 拔回；再取回／暂收后在 `0x10005` dirty 状态直接 Shutdown。日志先恢复原 44 字节挂点、clean／fault=0／revoked=0，再还原 Hook；Shutdown=0，两入口原字节独立匹配、精确游戏退出、owner0、九个已静音会话无错误／无待恢复。源封存 158 份，严格候选／默认隔离及六项纯检查通过；启动前私有 ini 原字节快照保存，未补 resize／前台恢复。回执在 `analysis/equipment-stow-runtime-1023b/`，前局失败不改写。两局均仅外部槽请求且 R2／L2 mode off；下一项为原按钮联调。XR intent 与外部请求还需原子仲裁，实际身体命中、肩部图像、与手骨 writer 共存及头显仍未验收，详见 [暂收试验](../../equipment-stow-trial-20261023.md)。

2026-10-23：暂收候选首局购买版 PID 47872 的真实 Hammerburst 槽日志 `11→10→11→10→11`，原手附件往返 44 字节一致；外部 true 进入前按内部 false／附件读回／clean／action 的顺序交还，随后 35 份样本持续 11.698669 秒保持 detached、dirty=0，跨过旧 10 秒截止且无内部 helper 拔回。49 份外部读数的库存／Active／八控件位模式保持。**原收据仍失败**：取走队列后、写回本次结果前的确认窗口暴露旧状态 10，runner 在本次真实 11 完成前提前退出；当时 journal 已 clean，不能当 dirty Shutdown 通过。Shutdown=0、原附件及两入口独立还原、精确进程退出与静音清理通过；失败收据／机械 audit 保持在 `analysis/equipment-stow-runtime-1023a/`。正在修复请求全处理期的 mailbox 回执，并补本请求新日志确认；不以同局组件证据改写整局状态。

2026-10-23：身体槽暂收新增默认关闭的有限候选，先在模型副本提议，再由原 `SetWeaponTemporarilyHolstered` 与当前附件读回决定是否提交。独立 journal 记录八对象完整身份、四槽和原手骨名／相对变换，原动作前在反射请求／ProcessEvent／CallFunction 三处同步恢复并结束本 lease；未知或 foreign 状态永久撤销写权限，之后再 detached 也不补 false。R2／L2 新增同步 yield，停用时不等新 XR 帧；取回后封存旧边沿，持续按压不会自动开火，明确松开的新 baseline 允许后续正常单次按压。Shutdown 等待游戏线程恢复和在途回调，锁顺序已由 Astra Max 复核修正。严格候选／默认关闭构建、导出隔离、缺 CallFunction 的配置拒绝及五项纯检查通过，候选 SHA256 `016b587c…a95ef7`；尚待单局零售版有限联调，未替换启动器，详见 [暂收与按键交还试验](../../equipment-stow-trial-20261023.md)。另初始 arena PID 28452 的三组读数及 20 帧观察均已持 Hammerburst，旧 1,003 帧也都非零 Weapon；没有取得真正 `Weapon=None` 基线，不因此开放零武器手骨写入。该观察局 Shutdown／两入口还原、精确 game_absent 与静音恢复通过。

2026-10-23：实际双手 XR 数据链新增两局购买版模拟器记录，Native 的 FREE_HAND／CROSSHAIR 开启、CALL_BOUNDARY 关闭可与 Playground 动画入口共存。PID 40872／53564 各完成 43 次 GVE6 新姿态驱动和 43 次精确恢复；前一局有一次 dirty 原近战入口立即交还，后一局请求在干净 Tick 边界进入，均观察到原近战／槽播放和退出后新票据驱动。前局未启用第一人称，临时偏移过大，一次左手位置残差约 9.76075 原始单位；其余 85／86 手样本小于 `6.87e-5`。后一局实际切到第一人称（Native applied=173），86 个跨网格 Tick 手样本位置残差最大 `6.57e-5`、完整旋转角差最大 `0.02348°`，另有一个 Stop 到达后的 planned 尝试未写入、不计 lease。两局 dirty／fault 为零，PlaygroundShutdown=0、两原入口独立匹配、九个已静音专属会话无修改／无待恢复且自有游戏退出。**两份整局 receipt 仍为失败**：1280×720 源与 8×8 backbuffer 不匹配，GPU 未捕获／XR 无图像并 exit1；保留 tiny-output 只读规则，没有自动 resize／前台补测。Native 干净关闭，五项 DXGI 磁盘不匹配的独立关闭后 hash 均等于同局 manifest 安装前 original、不同于 Native patched；原 verifier 基准接入缺口与原 `all_match=false` 保留。原始回执和组件／入口复算只在 `analysis/hand-xr-1023c/`、`1023d/`；这支持有限手骨数据链，尚非 Quest、掌心标定或图像／头显验收，见 [XR 双手记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：原版收枪助手的购买版只读观察取得当前实例证据。自有置后静音 PID 18068 调用人物 `SetWeaponTemporarilyHolstered(true)` 后，当前 Lancer 的同一 Weapon.Mesh 从人物 Mesh 脱离；finally 调用 false 后恢复到原右手 `b_MF_Weapon_R_End`。四槽、库存链、当前 Weapon／slot 2、Active 与 `bSwitchingWeapons=false` 保持，稳定基线后 419 帧的八个 cached IK 控件对象／强度位模式不变；观察器共 441 帧，无其覆盖的 watched 动作事件。原 `bTemporaryHolster` 始终 false，说明不能把人物助手与武器的暂收旗标混用。私有 runner 错误等待该旗标 true 而超时，原失败 receipt 保持未通过；独立机械 audit 只总结已保存的字段、附件及 slot 样本，不重写运行结论。前期三次准备／签名拒绝均在实际暂收调用前停止，未作通过证据。自有 PID 的实际退出、PlaygroundShutdown=0、两个原入口匹配及九个本来已静音会话的零修改／零待恢复均核实。回执／复算位于忽略的 `analysis/hand-stow-observe-1023d/`；尚未证明肩部代理渲染，也未接入身体槽与 R2 持枪许可。

2026-10-23：实际 GVE6 grip 到双手 IK 的默认关闭候选已接线并完成严格编译／离线检查。每手保存完整刚体局部绑定，独立核验新票据、身份／动作／XR 各代次和 100 ms 新鲜度，失去一只手不要求另一只手重绑；原动作入口同步恢复 journal，重新安静后等待新 baseline 再接管。Lease 新增选中通道接口，未选手不读写；日志刷新后重验新鲜度，拒绝第二次 Start 不覆盖排队请求，256 次写入预算不可被原动作重开。完整绑定、12 组票据边界、选择通道／失败恢复及 304 字节私有 Start 检查通过，XR／合成／默认禁用构建及导出隔离通过。只允许独立 status 冻结的新 Native 无 handoff 测试场，不能把 frame-link epoch 冒充 committed 同步 getter，也不能把票据自身代次当外部基准；后续购买版模拟器数据链与整局失败分开记录于上文。详见 [XR 双手有限试验候选](../../hand-ik-xr-stream-trial-20261023.md)。原公开手部 ownership／apply 与 GVE6 ABI 不变，普通启动器及冻结包未替换；本轮子任务不启用 Fast。

2026-10-23：自由双手新增默认关闭的四通道逐帧驱动，复用原左右手链尾的世界位置／整数旋转；每次先保存四组目标与强度，下一 PlayerTick 在原版及桥接请求前精确恢复，原动作入口可在同帧先恢复再交给原版。购买版 PID 15952／20624 两局各完成 32 次位置／yaw 驱动、96 份完整矩阵样本及 32 次恢复；PID 56972／37196／43348 三局再完成 X／Y／Z 与 pitch／yaw／roll 的三轴组合目标和反向目标。五局各有 256 次 Setter 和 256 次目标写入／还原，第 16 次写入后立即触发原版 Lancer 近战，真实动作／槽播放、同步交还及结束后重新合格接管均通过，dirty／fault 为零。三轴三局各 31 个跨 Tick 样本中两手位置目标残差均小于 `6.25e-5` 原始单位，三轴旋转段完整目标角差均小于 `0.022°`；同帧第 16 次不计效果样本。这是小幅合成目标的当前手骨窗口证据，实际 grip 标定、XR 持续驱动、其他固定动作和最终渲染／头显仍待完成，公开 `ownership_proved`／apply 保持零。

前两局 PID 29424／51004 各完成 16 次恢复和立即交接后干净超时。诊断确认 Lancer `AR_Melee_Saw_Idle` 在父槽退回 base、三种子权重全零后仍保留子 `bPlaying=1`；不再用该标志单独否决当前零姿态贡献，非零退混合、未知字段、动作和身份门仍拒绝，也未写原动画字段。权重和剩余混合按 binary32 位检查零，DAZ／FTZ 模式下的 volatile 次正规数拒绝测试通过。Astra Max 复核已保存脚本、原日志与恢复／调用顺序；本轮 subagent 均未启用 Fast。四通道部分写入、helper 失败后已写入、foreign 值不覆盖、精确恢复和零权重条件的 C++ 检查、旧受控 IK／CallFunction Hook 回归及 30 项 Python 运行器测试通过。所有七局的两入口活体还原、九个静音会话恢复及自有游戏退出通过；默认禁用构建和导出隔离通过，冻结 Native／XR、默认启动器及包未替换。回执 `analysis/hand-ik-stream-runtime-1023a.json` 至 `1023g.json`，当前 DLL SHA256 `87308482f6e60502361f81032e96b28f9b2d2971b02b9f2962d1046643090563`；详细协议、数值和限制见 [双手连续 IK 试验](../../hand-ik-stream-trial-20261023.md)。

自由手动画内部调用缺口已有购买版证据：默认关闭的 CallFunction 候选在自有置后静音 PID 59828 取得 44 对调用、11 次真实槽播放、274 个快照、零 caller frame 拒绝。拾取伴随槽 3／10／14 在配对调用 18／19／20 中，把同一子节点从 `AR_*Holster_Rt` 改为 `AR_*Equip_Lt`；此前未观察到的槽复用现有同步边界。主 Kickup 和普通近战 A／C 的槽播放也已观察。候选不消费 caller 参数、不改返回值，RVA `0x1d6f30` 入口已被其他所有者改写时拒绝安装；停机后独立读取 CallFunction32／ProcessEvent64 均与原始证书一致，游戏退出及 11 个静音会话恢复通过。私有／默认关闭构建、转发与还原离线测试、观察器／Hook 测试及 23 项 Python 检查通过。回执 `analysis/hand-animation-call-runtime-1023a.json`，详细字段和证据见 [动画观察记录](../../hand-animation-probe-20261022.md)。该只读观察局本身不写手骨，也未覆盖取消／处决或头显画面；后续有限驱动见上文，原所有权门不会由新日志自动解锁。

自由手新增独立纯逻辑 `hand_animation_ownership.h` 与测试：默认拒绝，仅对完整配对的拾取 53／普通近战 A、C 起止、稳定实例／控制链、槽实际／目标权重与剩余混合退出及后续新 Tick 输出 `normal_path_ready` 必要条件；`ownership_proved` 始终为 false。缺边界、取消／受伤、其他固定动作、身份变化、槽复用、丢样和超时锁闭，重置后空闲也不自动获权。认证的空数组元素可保留，未读成功的零值不能代替空洞证明；`PendingBlendOutTime` 保留时长及精确 -1 哨兵均不当作剩余混合。MSVC Release `/W4 /WX /EHsc` 的新门、既有观察／自由手回归及私有保存日志回放共 4/4 通过。PID 57480 的 19 个槽数组元素含两个空洞；伴随收枪槽 3／10／14 在 snapshot 26 无配对 Play／Stop 边界复用为 Equip，严格门因此拒绝，不能由主 Kickup 已安静宣称可交还。该前 26 快照回放人为补足了空洞读取成功、Tick 代理值和预绑定拾取目标等字段假设，**仅为首次拒绝位置的诊断性离线复核，不认证当前运行适配器**；派生源码和回执均留在忽略目录，见 `analysis/hand-animation-ownership-offline-20261022.json`。没有启动游戏、IDA、接入运行桥或改变默认构建；持续双手写入与头显验收仍未完成。

自由双手原 IK 受控复测：默认关闭的六窗试验在两局自有置后静音购买版进程 PID 53904／60116 各取得完整 `S,+X,−X,−X,+X,S`、307／317 条原始采样、12 次 Setter 和 12 次 owned 目标写入／还原；字段恢复、Hook 关闭、自有游戏退出及各 11 个音频会话恢复通过。离线脚本按原定前后不同网格 Tick 夹取及同跨度无写入对照重算，两局四个活动窗右手 X 残差约 `±0.096–0.106`／`±0.096–0.105` 原始单位，均超过未放宽的五倍噪声门槛 `0.035896`／`0.042369`；假操作上界 `0.000825`／`0.004228`。首个恢复新 Tick 并非全部在噪声内，第二个新 Tick（回滚后约 18–22 ms）全部在噪声内。初版仅用脉冲前两 Tick 外推的分析在第二局因较大 sham 偏差未通过，旧回执保留；原采集终行仍为 `effect_evidence=0,inconclusive=1`。这只支持**当前实例短时右手骨 X 响应候选**，尚非 XR 持续双手、动作接管、最终渲染或头显验收。复算工具、私有回执与方法见 [IK 对照记录](../../hand-ik-controlled-trial-20261022.md)。默认启动器及 `apply_mask=0` 不变。

启动前臂展校准现有一个源码工作树候选：新清单布尔门 `armspan_startup_calibration` 必须与手柄观察、owned startup menu 及既有 crosshair／UI／anchor 前置能力同时启用；冻结的普通清单、安装版启动器、Native／XR 构建及便携包白名单均未改变。缺失或无效记录时，在 Native 激活前由自有 XR 子进程发布同票据的双手姿态，只读消费者核对版本、进程身份、频率、跟踪与 100 毫秒新鲜度；双手张开并连续按住两个 Grip 至少两秒且取得 60 个不同样本才原子保存到本机配置。有效记录自动复用，启动器可选重测；测量或保存成功前失败保留旧记录。保存成功后若 XR 清理失败，则保留新记录但阻止 Native 激活，不能回滚覆盖并发更新。Python 新增 32 项、既有 launcher 78 项、原校准 2 项、C++ ABI 对照、C# 候选构建与隐藏自检通过；未跑真机 OpenXR／头显。双语桌面提示与进度已接线，头显内专用引导、肩／裤兜试抓确认和真实玩家验收仍待完成；详见 `tools/vr-launcher/armspan_candidate.md`。

原版拾取／普通近战新增默认关闭、严格只读的 `GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` 候选，位于独立武器分支。它在原自动 Hook 的 PlayerTick 过滤前记录真实 `source=hook` 命中，并将桥接直接调用原函数的请求单独记为 `source=bridge_invoke`；有界记录身体槽子节点实际／目标权重与剩余混合，以及八个原手部控制器身份、强度和当前树链成员。每次采样复核测试场和 PC／Pawn／World／Mesh／AnimTree，全部记录保持 `ownership_proved=0 writes=0`。MSVC Release `/W4 /WX /EHsc` 候选、默认关闭构建、调用关联／缺边界／容量单测及 Hook 回滚测试通过；候选有三个专用导出，默认关闭 DLL 无这些导出。离线回执 `analysis/hand-animation-probe-build-1022a.json`，字段说明 `docs/hand-animation-probe-20261022.md`。

购买版自有置后静音测试场 PID 57480 的有效序列中，测试前不拥有的 Longshot 由原版 `ServerUse` 拾取，`SpecialMove` 进入 53，人物 `Custom_FullBody` 槽播放 `AR_Idle_Ready_Kickup`；约 1.34 秒后原版 `OnAnimEnd` 的序列节点与该槽相同，返回时 `SpecialMove` 已归零，该子节点的实际／目标权重均为零。随后原版 `StartMeleeAttack` 请求使当前武器进入 `MeleeAttacking`；请求维持约 1.4 秒，观察到两次 BeginState／EndState，人物 `Custom_UpStand` 与 `Slot_Std_Idle_Lower` 播放 `AR_Melee_Smack_A`／`C`，各次 EndState 后相应子节点权重归零。189 个边界／周期快照均读到 19 个身体槽；原数组预留容量为 42，先行探针误将预留容量也限制在 32 而漏读槽，之后已把活动槽上限 32 与可接受预留容量分开并复测通过。八个手部控制器在全部快照保持同一对象、当前网格和动画树链身份；链尾 VR 可用目标强度仍为零。终行 `calls=9`、`event_lines=18`、`overflow=0`、`ownership_proved=0`、`writes=0`，`PlaygroundShutdown=0`，11 个静音会话恢复，游戏进程退出；私有回执为 `analysis/hand-animation-probe-runtime-1022e.json`。这局证明正常拾取与普通近战的这些边界和相关槽变化；VM 内部直接调用、取消／受伤交接、其他固定动作、XR 双手驱动和头显可见性仍未证明，**不能由本局把全局手部所有权置真**。两局先行脚本分别因属性类型读取写错和测试人物已持有 Hammerburst 在探针启动前拒绝，均正常清理，不计动画证据。

右手原版 IK 的默认关闭单帧试验已完成一局购买版置后静音运行：新 `GEARS_PLAYGROUND_HAND_IK_TRIAL` 仅在自有测试场、八控制器与原动画稳定后，将当前右手骨世界位置沿 X 移 1 原始单位，用原 `SetSkelControlStrength(0.1, 0)` 激活一帧；下一次原 `PlayerTick` 前归零强度、确认三强度值为零，再恢复原 `EffectorLocation` 的 12 字节，继续观察至少 0.25 秒。PID 36904 的最终回执为状态 5 `inconclusive`、Setter 2 次、目标写入／还原 2 次、`restored=1`、`safe_to_unload=1`、`PlaygroundShutdown=0`；11 个静音会话恢复，自有游戏退出。渲染时间推进，角色 `TickTag` 从 420 到 421，右手在脉冲期间沿目标 X 方向约移动 `0.093` 单位，但左手及原待机动画同时漂移，恢复后又有持续动画位移，**不能把这局判成 IK 骨骼求值或可见双手跟随证明**。旧探针把 `Engine.SkelControlBase.ControlTickTag` 错按 `IntProperty` 读取，保存的包对象盘点实为 `BoolProperty`（artifact 3，object 24790）；所以旧日志的 `tick_known=0` 是类型门控失败，不是未 Tick 证据。随后私有单脉冲候选已改用布尔读取、独立记录网格整数 Tick 与控制器布尔标签，并禁止没有 sham／自然动画基线的单脉冲返回效果通过；离线 Release 构建和 Hook／策略测试通过，修正版单脉冲未再运行；后续六窗对照见本节开头与专项记录。私有回执 `analysis/hand-ik-trial-runtime-1022d.json`。先行零售局因把 `SkelComponent` 错认成 `ObjectProperty` 而在写入前状态 7 拒绝，诊断确定它实际是 `ComponentProperty`；修正后保留了对当前角色网格的身份校验。误输 DLL 哈希的一局在注入前拒绝，不计功能试验。正式默认构建、只读双手候选及 `apply_mask=0` 未改变；拾取／近战交接和真实 XR 双手仍待验证。

自由手动画交接边界已用保存的原版脚本作窄范围复核：拾取可能先 `GrabPickup` 改变库存再进入 `SM_WeaponKickup`／`SM_WeaponPickup`，正常结束须由本动作身体动画的结束通知退出特殊动作，取消可直接替换下一特殊动作；近战从武器 `MeleeAttacking` 状态进入／退出，Lancer 结束后还会有约 0.2 秒身体槽退混合，受伤可能转入踉跄。仅见 `SpecialMove=0`、`bDoingMeleeAttack=false`、武器 `Active` 或 `CanSwitchWeapons=true` 均不能证明双手已交还。购买版现已验证一局正常拾取 53 与普通近战的起止及槽变化，见本节前段；取消、受伤、Lancer 处决、其他固定动作及跨动作槽复用仍缺完整运行序列。丢失任一边界继续保持 `ownership_proved=false`。

当前人物双手 IK 边界先经只读取证：购买版自有置后静音 PID 60744 的当前 WarPawn 缓存了左右各四个手部控制器，均指向当前角色网格且都在其六条 AnimTree 骨骼控制列表中；左手骨 11 对应列表 0，右手骨 35 对应列表 2。每侧链顺序为 `IKBone_*Hand → IKBoneRot_*Hand → IK_*Hand → IKRot_*Hand`。末端 `IKCtrl_*Hand` 位置及 `IKRotCtrl_*Hand` 旋转均以 World 空间为目标，当前强度／目标强度／剩余混合时间都是 0；两侧链首控制器仍受原版动画驱动，左侧取样时尚有约 0.05 秒混合。角色网格未全局忽略控制器。资料在忽略的 `analysis/pawn-ik-runtime-1022b.json`，探针 `PlaygroundShutdown=0`、11 个音频会话恢复和自有游戏退出通过。它认证当前实例的可复用入口；后续单帧右手写入／回滚试验见本节前段，原版动作争用与双手持续驱动仍未验证；默认自由双手候选继续 `apply_mask=0`，不能称可见双手已自由移动。

并行候选已纳入本独立工作树：`GEARS_PLAYGROUND_FREE_HANDS_PROBE` 默认关闭，消费 GVE6 同票据左右手完整世界位姿，在原版 `PlayerTick` 后只读记录原角色双手骨骼矩阵和固定动作状态；离线状态机覆盖双手独立失追、拾取／近战／切枪动画接管、新票据恢复、角色／会话／空间变化。候选与默认 Playground Release 构建、`FreeHandControlTest`、`PlaygroundHookTest` 和五项 Python 日志解析测试通过；导出只存在于候选 DLL。随后自有购买版置后静音 PID 56756 的原角色手骨探针完成 160/160 帧：左手 `b_MF_Hand_L=11`、右手 `b_MF_Hand_R=35`，当前角色 `SkelControlIndex` 长 115、动画树控制列表 6 条；160 帧双手骨矩阵均有效，因没有 XR 姿态 `xr_samples=0`，写入与 `apply_mask` 均为 0。忽略回执 `analysis/free-hands-probe-runtime-1022a.json`，`PlaygroundShutdown=0`、11 个音频会话恢复、自有游戏退出通过。当前 `ownership_proved=false`，**游戏内可见双手并未被驱动**。原版拾取完整生命周期、可恢复的双手骨骼写入、掌心标定和头显运行仍待解决。

空手姿态发布增量：另一个默认关闭的 `GEARS_FREE_HAND_POSE_OBSERVER` native 候选在同一已认证 XR 票据内允许当前武器为空，或原版固定动画暂时接管手部时，继续发布只读双手世界姿态；当前 PC↔Pawn、世界、字段布局及存在武器时的实例身份仍须通过校验。装备意图依然要求非空武器，失追可单手独立撤销，动画结束后只接受新票据。`HandPosePublicationTest`、`EquipmentIntentBridgeTest`、`FreeHandControlTest` 和默认／候选构建通过；候选 DLL 哈希 `65c67d957fab3b28362d9ac296c4da4c34c11657a4ed59a11928b59833f7c8ed`。XR 失焦时发布端继续拒绝有效输入；此候选未做购买版或头显测试，`apply_mask=0` 及默认启动器均未改变。

枪上弹数卡片新增默认关闭的独立 OpenXR 候选：从经认证的原版 `GetMagazineSize − AmmoUsedCount` 与 `SpareAmmoCount` 生成“枪内／备用”纹理，在当前枪体局部挂点姿态上构造双眼 Quad，前后向及过期／错票据均拒绝；手雷独立显示可用数，不放完美换弹条。MSVC `/W4 /WX` 候选测试 3/3 通过，默认 `OpenXRPoseProbe` 构建通过，CPU 预览已核对。当前没有经认证的游戏线程弹药快照跨进程桥、同票据枪模位姿和逐枪挂点，也没有原 HUD 弹数／完美条各自绘制归属的证据；所以卡片尚未进入生产 XR 循环，右上角原 HUD 尚未移除，不能声称头显可读。

Lancer 控制索引继续只读核验：自有购买版置后静音 PID 58808 的当前枪模 `SkelControlIndex` 长度 9，骨骼 0–7 均为原始字节 `255`，骨骼 8（弹匣）为 `0`；`RequiredBones` 为 0–8，`AnimTickArray` 正好是当前 `AnimTree` 与其 `AnimNodeSequence`，原 `AmmoControl.SkelComponent` 指向该枪模组件。实时 UFunction 元数据确认 `SetAnimTreeTemplate(NewTemplate)`、`UpdateAnimations()`、`SetSkelControlStrength(NewStrength, InBlendTime)` 的参数结构，但没有调用它们。这个索引使“仅追加卡扣 7、机匣 5 的控制链就生效”的假设不成立；仍须引擎管理的对象创建、数组维护、索引重建与 GC／恢复证明。忽略回执 `analysis/lancer-anim-tree-runtime-1022c.json`，`PlaygroundShutdown=0`、11 个音频会话恢复及游戏退出通过。

完美换弹期限从推导推进到当前实例实测：自有购买版置后静音 PID 57472 在一次原版 Lancer 换弹中，经游戏线程调用原 HUD 本来使用的 `GetActiveReloadValues`，两次分别于换弹约 0.201／0.463 秒取得相同四值：起点约 `0.700000`、预反应 `0.25`、完美宽度约 `0.100000`、普通成功宽度 `0.25` 秒。按已匹配原版判定函数的 `elapsed ≤ start + super`，这次 Lancer 的完美**终点**为约 `0.800000` 秒，自 `ReloadStartTime` 起算；这不是所有枪或所有游戏状态的常数。两次 Getter 调用前后主动换弹尝试标志均未变化；原版换弹随后自行结束、已耗弹从 1 归零。数值回执在忽略的 `analysis/lancer-reload-getter-runtime-1022a.json`，`PlaygroundShutdown=0`、11 个音频会话恢复、自有游戏退出均已核对。没有用实体动作结算完美奖励，也没有头显验收。

Lancer 动画树当前实例只读核验：购买版自有置后静音 PID 46552 的 `Weapon.Mesh.AnimTreeTemplate` 为 `AT_COG_AssaultRifle`，`Animations` 是另一份当前实例 `AnimTree`，仅有一个 `AnimNodeSequence` 子节点；其 `SkelControlLists` 只有 `b_AR_Magazine → AmmoControl`，`bIgnoreControllers=0`、`bIgnoreControllersWhenNotRendered=false`。这为保留原动画通知并另加卡扣／机匣骨骼控制提供真实拓扑，但**没有创建或挂接新控制器**，也未证明动画计算顺序／GC。只读回执在忽略的 `analysis/lancer-anim-tree-runtime-1022b.json`，私有 `PlaygroundShutdown=0`、11 个音频会话恢复、游戏退出通过；先行试验把模板字段误查到武器而非其 `Weapon.Mesh`，在图读取前停止，没有作为通过证据。后续仅在引擎对象创建、列表所有权和恢复路径认证后做默认关闭的单控制器试验；不能直接改骨数组或把现有 `AmmoControl` 换掉。

控制器接线审查确认当前证据还不能安全实现新增 `SkelControlSingleBone`：缺少当前构建的引擎对象创建入口、`SkelControlLists` 的引擎管理扩容、`SkelControlIndex` 重建和新对象的 GC／解绑证明。现有 `SetAnimTreeTemplate`、`UpdateAnimations`、`SetSkelControlStrength` 声明不等于这些路径已经验证。下一步只读记录当前实例控制索引、相关数组和实时接口签名，之后才考虑默认关闭、零强度的单控制器试验；不直接串改 `AmmoControl.NextControl` 或用 DLL 内存替代引擎数组。

Lancer 原版换弹机构新增同帧只读核对：自有购买版置后静音 PID 46348 中，对一次原版 `ForceReload` 取得 321 组枪体／卡扣／机匣世界矩阵，160 组处于 `Reloading`；卡扣 `b_AR_Latch` 为骨骼 7，相对枪体绕局部 +X 最大转 114.5°，机匣 `b_AR_Receiver` 为骨骼 5、绕 −X 最大转 28.5°，两者枢轴位移均低于 0.00031 游戏原始单位。卡扣从约 313 ms 起运动、约 2,282 ms 回位；机匣从约 360 ms 到 2,266 ms，说明原版序列里两件大部分时间同时处于打开状态，不能凭名称把卡扣直接当作玩家插弹后才拉动的“枪栓”。只读原始回执与确定性运动摘要分别保存在忽略的 `analysis/lancer-reload-bone-runtime-1022c.json` 和 `analysis/lancer-reload-bone-motion-1022c.json`；新分析脚本可复算矩阵。`PlaygroundShutdown=0`、11 个静音会话恢复、游戏退出均已核对。一次先行启动没有可用菜单视口，在采样前停止；没有被用作通过证据。这些矩阵不确定实际可抓表面、黄色高亮或头显中的动作，也没有完成手动换弹。

逐枪离线模块进展：Hammerburst 新增专用弹匣／闩锁换弹事务与黄色步骤，固定动作顺序必须先经当前原枪实例认证；Gnasher 新增每轮最多三发、每发库存提交、射后双手杠杆循环与原版动画交接；Snub 新增左裤兜手枪身份、弹匣守恒、空仓／半仓逐实例认证步骤、失追恢复与一次性原版提交；Longshot 新增单发物理换弹认证计划，以及双眼独立相机、离轴投影、虚像焦平面、镜筒遮挡和真实瞄准交点投影接口；Boomshot 新增无默认弹数／机械顺序的认证计划、原版库存提交与射击回执门禁。五者均接入独立 CMake 单测并在 MSVC Release 构建与运行通过；Snub 合成 114 组弹药守恒／重试、Longshot 换弹 178 项和光学 136 项、Boomshot 611 项通过。Gnasher 当前步骤在取弹时指向左肩，握住子弹后才转装填口。这些是纯逻辑／数学基础，握点、装填口、时限和镜片光学参数仍需零售游戏实例标定，尚无真实手柄、GPU 瞄准镜或默认启动器接线，不能算逐枪完成。

Torque Bow 逐枪纯逻辑另已接入 CMake 并通过合成测试：R3 左肩取箭、认证步骤高亮、原版 EndFire 请求和实际弹体／弹耗回执分离；失追和姿态过期不能提出放箭。保存脚本显示持续蓄力到上限也可能调用发射，所以当前零售版没有证明“不发箭中止”能力时，生产配置的蓄力门禁保持关闭。装填顺序、原版完美终点及安全中止入口都待购买版实例验证；它不代表游戏里已可用。

Boltok 专用纯逻辑已接入 CMake、MSVC Release 单测通过：左裤兜取枪、左肩获取经认证的实弹、当前实例提供机构移动／逐次装入计划、每步黄色提示、原版库存一次性回执及原版射击弹道／后坐力保留门禁。测试中 7 发容量仅是合成夹具，未写作游戏容量。原包网格通用骨名与默认 Snub 动画轨道不匹配，因此没有默认装填动作、部件语义或弹药模型；游戏线程和手柄尚未接入。Hammer of Dawn 另有目标／资源／蓄力／照射与双手动画交接协议，合成 500 项在 Release 构建通过；原版 `IsAValidHODTarget` 等原生签名、使用条件和非发射取消入口未实证，故不启用实际使用，也不虚构弹匣或普通 R3 换弹。

Bolo 手雷专用纯逻辑已接入 CMake、MSVC Release 1,144 项合成检查通过：右裤兜无弹时只产生绑定当前实例与原版音效身份的一次性提示意图；有弹时保留一枚并通过原版准备／发射票据，最近 140 毫秒手部与手腕运动给原版投掷物提供 3 kg 模型的初始位置／速度，库存只认原版一次性回执。扳机按下与松开必须属于同一次持有，释放票据匹配手部帧序列、输入纪元和同步时间，失追、过期或恢复造成的伪边沿不能投掷。原版引信状态在中断后不能免费重置，旧投掷不会缓发；原链条、爆炸和音效必须由原版保留。释放输入、引信归属、原生初速覆盖及安全取消入口尚未在购买版当前实例验证，配置默认闭锁；没有游戏内真实投出或头显手感结论。

Troika 与 Smoke Grenade 各有独立纯逻辑模块并接入 CMake，MSVC Release 合成测试分别通过 350／1,730 项。Troika 按固定机枪处理，验证固定枢轴、双手握把与两枪口身份，原版上下机枪动画、射击、资源和热量各依回执；不假定双枪口交替，也不默认身体槽、左肩弹匣或 R3。Smoke 单列原版烟雾武器／投射物／弹药／发射器／音效类，右裤兜无弹音效带一次性令牌；释放票据必须匹配同帧手部序列，投出仍走 3 kg 手势轨迹及原版一次生成／一次耗弹，原烟雾和音效路径保留。两者缺当前实例的原生调用、机械／引信／安全停火与渲染证明，生产配置保持闭锁，尚无游戏内实际交互。

Lancer 锯刃候选新增仅只读的同帧探针：在购买版自有置后静音 PID 53896 中，以已核原枪网格的同一 `PlayerTick` 矩阵取得 16 帧枪体和原两枢轴矩阵，并记录由 12 点可见锯链外缘近似变换的世界位置；离线复算各点与对应枪矩阵最大差约 `5.15×10⁻⁵` 原始单位。完整坐标、日志与来源哈希留在忽略的 `analysis/lancer-blade-edge-runtime-1022p.json`；游戏退出，测试音频守护对 11 个会话恢复成功。此回执只证明候选外缘随当前枪矩阵同帧变换，不证明锯齿准确碰撞、敌人接触或原版处决。两次先行尝试分别因工作区 Python 库路径和私有 DLL 未编入探针在采样前停止，均关闭各自游戏并恢复音频，不作通过证据。

枪口射线复核：保存的普通 `InstantFire` 链在原版随机散布之前可观察 `GetAdjustedAimFor` 基础方向，但最终 `PushedForwardStartTrace` 可再改动起点；Gnasher 有独立 `CustomFire/FireAllPellets` 和使用玩家视角方向的近距离分支，不在普通步枪观察器覆盖内。原版后坐力还会经 `SetWeaponRecoil → ProcessViewRotation` 改变瞄准基底；若直接替换为手柄轴可能丢失实际反冲。先对单发与短连发做只读关联，再决定逐枪安全改向入口；此项是静态证据，未声称当前零售版枪口开火通过。

Lancer 锯刃接触几何更正：同一哈希已核原模型 LOD0 的锯链模糊材质构成枪体局部 Y `26.30–75.86`、Z `-7.21–10.31` 的环，原先两根骨枢轴只在环内部，不能直接当接触线。独立分支新增基于可见环外缘 12 点的稀疏候选及 `LocalToWorld` 刚性检查接口；原枪身份与同帧性仍由未来游戏线程适配器负责；接触模型改要求原版权威、持锯、目标存活与控制器、双方资格、遮挡及粘附目标一致性门禁。纯状态／姿态测试、私有 XR 候选与默认关闭构建通过。材质／骨骼二维审计图只留在忽略的 `analysis/lancer-blade-geometry-1022/`。购买版自有置后静音 PID 61076 再通过原版 `Spawn`／`SpawnDefaultController()` 创建一名 `WarAI_Locust` 控制的独立敌人，其 `Controller.Pawn`／`MyWP` 回指一致，两项原版近战资格函数均为 true；常规无控制器靶被原版特殊近战资格函数拒绝。只读资格测试没有持锯、实体接触或处决；`PlaygroundShutdown=0`，游戏退出且音频恢复，回执仅在忽略的 `analysis/lancer-saw-target-runtime-1022m.json`。边缘坐标仍是静态近似，XR 适配器没有真实敌人接触或原版 `AcquiredVictim` 调用，后续须验证同帧物理接触、遮挡及双方动画自然完成。

Lancer 电锯动画交接修正：已保存的原版脚本显示，处决特殊动作 34 刚进入时松开 B 可能经近战 `EndFire` 强制结束动作。独立状态机改为在原版自行退出攻击后才请求松 B，并待原版按键字段清零才恢复双手 6DoF；处决进入与 XR 失追同帧的竞态也由原版动画优先。私有原版 B 适配器增加特殊动作 34 的释放保护。默认关闭的新 `GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST` 把认证 GVE6 真实手姿、当前 Lancer 枪矩阵、临时拉柄点和连续拉程接到原版 B 待机，须显式 Arm；15 秒试验看门狗和失追清理仅供开发，未实现实际锯刃目标获取。状态机／姿态纯测试、私有与默认 Playground 构建通过。购买版自有置后静音 PID 44484 验证显式 Arm 后没有 XR 手姿不会误按 B：约 0.45 秒均维持待命 2，Stop 后状态 8，原版近战／锯链／受害者为空、关闭码 0、自有游戏退出且音频恢复，回执仅在忽略的 `analysis/lancer-saw-xr-no-pose-runtime-1022k.json`。最后一次释放门更新后，PID 51708 取得 9 个原版 B 保持、锯链运转、自动寻敌计时器关闭的样本；显式停止、关闭码 0、自有进程退出及音频恢复通过，回执仅在忽略的 `analysis/lancer-saw-xr-final-hold-runtime-1022l.json`。两次运行都没有真实 XR 拉动或敌人处决，不能作为头显验收或启用默认启动器。

体感动作单位比例接线：私有双手姿态桥从 GVE5 升至 GVE6，原生发布端把与手部世界姿态**同一 XR 票据**使用的游戏单位／米比例写入帧，桥接边界只接受有限的 10–500 范围，旧协议的映射名隔离。Lancer 电锯新增世界手部姿态到米制手拉样本的转换，核对同一控制器／人物／武器／世界代次及 100 ms 新鲜度；纯测试证明 100 或 200 单位／米时同样的实际 19 cm 拉程都提出原版待机请求，失效比例、过期或身份变化拒绝。桥单测、电锯状态机与转换单测、私有 native 与 Playground 候选／默认构建通过。这是原版电锯按键的**输入适配准备**：拉柄尚未在原枪定位或渲染，游戏线程尚未调用原版 B／抑制寻敌计时器，未在游戏中启动锯链，也未合入默认启动器。

Gnasher 原版持枪手骨增量：购买版自有置后静音测试场 PID 8720 按原版拾取／装备流程取得当前霰弹枪，先确认人物 `ShouldLeftHandIKBeOn=true`，再于待机姿态读取 24 组枪主骨、杠杆、左右手骨及左手 IK 骨世界矩阵。换算到枪主骨，右手骨原点中位数约 `(-2.3163, -17.5396, -9.9111)`、左手约 `(8.5540, 25.9871, -9.7627)` 原始单位；最大位置漂移分别 `0.0925`／`0.2972`，选定样本的朝向跨帧最大漂移分别约 `0.28°`／`0.36°`。右手骨到已测杠杆枢轴的垂轴距离约 38.8 原始单位，故测试几何现用实测手骨原点和枢轴构造下掰圆弧，纯测试通过。**手骨原点不等于握把网格接触面或 Quest 控制器握点**；仍需实枪接触、控制器到掌心标定及头显验证。本次完整回执与 24 组矩阵只在忽略的 `analysis/gnasher-held-runtime-1022i.json`，可由 `tools/weapon-models/analyze_gnasher_grips.py` 重算摘要；`PlaygroundShutdown=0`、游戏 PID 退出、音频守护恢复。前三次诊断分别遇到资料表刷新、不可读名称及探针提前返回，均未用作通过证据且各自已清理；一份未记录 IK 状态的先行 24 帧样本仅作交叉检查。默认 DLL 未启用该机构。

Gnasher 杠杆手势校准：复用购买版 PID 43448 的 35 帧 `SG_Chamber` 实例矩阵，修正原纯逻辑样例要求下掰 0.8 rad（约 46°）、超过原版最大 32.43° 行程的问题。新候选采用实测局部 −X 转轴与枢轴 `(0.01479, 20.62221, −2.92057)` 原始单位；完整下掰阈值 0.44 rad（约 25.2°）、拒绝超过 0.70 rad（约 40.1°），后两者是待头显校准的 VR 容差而非原版时限。双手几何和循环单测已覆盖完整下掰回位、渐进越程、方向、半径、追踪失效和头部运动不改变杠杆角度。右手握把实际接触点尚未从原版资源标定，游戏线程、原骨骼运动、射后禁射及黄色引导仍待接入；本条没有新零售版运行，也未启用默认 DLL。

Lancer 原枪网格跟手的购买版组件验证：在自有置后静音测试场 PID 40068，先通过私有右手在左肩槽取武器 2意图让 VR 装备模型确认持枪，再让游戏线程以 12 个短行程合成右手姿态驱动**同一个**原版 `COGAssaultRifle` 组件。世界矩阵跟随前后约 `+16／−8` 游戏原始单位并发生小角度旋转，逐姿态稳定后的最大 X 误差 `0.00977` 单位；候选退出后原相对变换与绝对标志恢复，`PlaygroundShutdown=0`。第一次未提交槽位取枪时模型正确拒绝；第二次立即读取 `LocalToWorld` 得到上一帧值，因此最终试验保持每个位姿 32 ms 后核对。原始运行回执只在忽略的 `analysis/lancer-support-probe-20261022/capture-mesh-follow-trial-retail-3.json`；原生入口还原、XR 退出 0、游戏静音恢复、准确自有 PID 退出均确认。此结果证明私有候选能移动和恢复原组件，**不证明 Quest 追踪、当前帧渲染、枪口弹道或后坐力**；默认 DLL 未启用。为下一步实际握点标定，`WeaponMeshFollow` 新增显式的原版手掌局部姿态与手柄到掌心朝向接口。已保存的原版 24 帧样本中右掌局部中位点约 `(-2.322616,0,0)`、手骨坐标相对枪体约偏航 `+90°`，但 Quest 手柄到掌心标定还未取得；新接口只经数学测试，尚未接入运行时。

Lancer 启动件原创模型原型：新增 `assets/weapon-6dof/lancer-starter.glb` 与确定性生成器，将固定插座、跟手 T 形拉柄和可延长绳分成三个节点；704 三角形，不含原游戏模型或贴图。忽略目录内的静态预览已经检查。尚未在游戏中定位、渲染、拉伸绳或用头显验证，不能把资产原型称为体感电锯完成。

购买版自有置后静音测试场 PID 51388 复核原版电锯边界：对已装备 Lancer 调用一次原版 `ButtonPress('B')`，按住约 0.85 秒期间控制器 B 与人物近战意图为真、`bDoingMeleeAttack=true`、`CurrentFireMode=2`、`SpecialMove=33`、锯链运转，`CheckForMeleeTarget` 计时器有效且没有受害者。清除该原版寻敌计时器后再保持 0.45 秒，近战／锯链继续，计时器没有自行重建；调用原版 `ButtonRelease('B')` 后近战、锯链、计时器和意图均恢复。回执只在忽略的 `analysis/lancer-support-probe-20261022/capture-chainsaw-semantics-retail-3.json`。反射直接调用类方法 `IsMeleeing` 即使处在上述状态也返回 false，不能拿它作为状态判据。这只证明无目标场景中的按键与计时器边界，尚未验证接触受害者后的原版处决、VR 拉柄或头显画面。Playground 入口／静音恢复通过，停止命令后仍留存该自有测试进程，已核对路径和创建时间再结束；完整 VR 会话未完成。

新增 Lancer 体感电锯目标：左手抓拉柄并完成连续拉程后启动，锯刃实际接触有效敌人时调用原版双方处决，原版动画结束后恢复双手 6DoF。原包静态脚本核对了 `WarPC.PreButtonPress('B')`、`Weap_AssaultRifleBase.MeleeAttacking.BeginState/CheckForMeleeTarget/AcquiredVictim` 及原版清理链；原版前方寻敌计时器不能在 VR 交互期间抢先取得目标。原枪 `b_AR_Chainsaw_Handle` 是 685 顶点／1,089 面的既有独立刚性部位，尚无证据可作为启动拉柄，检查 OBJ 仅在忽略的 `analysis/weapon-partitions/Lancer-chainsaw-handle-audit/`。独立分支新增 `weapon_lancer_chainsaw.h`：按握持边沿、拉程、方向、连续性和身份做启动候选，原枪锯刃线段／敌方胶囊接触候选必须再经原版有效目标与遮挡确认，明确在处决期间让原版动画持有双手、结束后再交还。合成测试通过；游戏线程 B 状态适配、原版寻敌计时器抑制、可见拉柄／拉绳、真实接触与头显测试仍未完成，不接入默认启动器。

后续私有 Lancer 电锯待机适配已补齐原版 B 键保持、每个自有游戏帧清除 `CheckForMeleeTarget` 自动寻敌计时器、身份丢失时释放，以及退出时等待原版 B 按住字段清零。购买版自有置后静音测试 PID 39340 的 9 个持续样本均为原版 B 按住／近战／锯链运转／模式 2，计时器关闭且没有受害者；明确停止后原版状态恢复。PID 61196 在 B 仍按住时执行 `PlaygroundShutdown`，关闭码 0，原版 B、近战、锯链与模式均恢复空闲。两次自有进程退出且音频守护恢复；证据只在忽略的 `analysis/lancer-saw-hold-runtime-1022a.json`、`analysis/lancer-saw-shutdown-runtime-1022b.json`。此结果不覆盖 XR 手拉启动、可见拉柄、实体锯刃接触、原版双方处决或动画后 6DoF 恢复；默认 DLL 未启用该测试选项。

另一次购买版自有置后静音只读探针 PID 61168 核对了原版 Lancer 电锯函数签名：`AcquiredVictim(VictimPawn)` 的参数区为 8 字节对象引用；`IsTargetValidForSpecialMeleeAttack(P)` 为对象入参加布尔返回。`MeleeAttackImpact` 有伤害和命中输出参数，不能当成实体锯刃接触入口。本次未调用目标获取或处决；回执仅在忽略的 `analysis/lancer-saw-api-runtime-1022c.json`，自有进程退出、音频恢复。下一步仍须逐帧定位原枪锯刃骨骼、验证敌方碰撞和遮挡后，才允许对同一有效受害者做一次原版动画移交。

原枪锯刃骨骼只读采样改用同一游戏线程帧，排除第一次跨反射调用时枪组件在两次读之间更新的无效尝试。购买版 PID 53752 普通持枪及 PID 36516 原版锯链运转各采 16 帧，`b_AR_Chainsaw`／`b_AR_ChainsawEnd` 索引均为 1／2；枪体局部枢轴约 `(0.008,34.127,5.651)`、`(0.008,56.562,4.476)`，距离约 22.466 原始单位，两组漂移均小于 0.001。PID 52172 的增强采样逐帧复核原版 B、锯链和人物近战确实持续为真，仍得到相同枢轴。启动电锯没有改变枢轴相对枪体的静态位置。原始矩阵仅留在忽略的 `analysis/lancer-saw-bones-runtime-1022e.json`、`analysis/lancer-saw-bones-held-runtime-1022f.json`、`analysis/lancer-saw-bones-held-runtime-1022g.json`；`tools/weapon-models/analyze_lancer_blade_bones.py` 可重算摘要。三次有效试验 Playground 关闭码 0、精确自有游戏进程退出，音频守护恢复。枢轴不等于可见锯齿外缘，不能凭此认定敌方碰撞或处决已实现；默认 DLL 未启用探针。

合成左手拉绳至原版电锯待机的私有端到端路径通过购买版 PID 32496 验证：临时枪体局部拉柄点 `(6,26,10.65)` 随原枪矩阵变换；同一世界单位比例／身份检查后的连续拉动在 297 ms、18.81 个原始单位时仅请求一次原版 B。其后 9 个样本中原版 B、近战和锯链运转，前方自动寻敌计时器均关闭、受害者为空；显式停止后状态恢复，Playground 关闭码 0，自有进程退出、音频恢复。首次 PID 41264 已达原版请求但漏发运行状态码，验证器超时；现已修复，以忽略的 `analysis/lancer-saw-gesture-runtime-1022i.json` 为通过证据。`weapon_lancer_chainsaw.h` 也修正为只在启动前要求 `CanDoSpecialMeleeAttack`，进入原版持有状态后依原版 B 状态持续，纯测试通过。**这仍是模拟手部轨迹**，临时点位未可视化、未校准真实 Quest 掌心；真实锯刃接触、原版双方处决及动画后恢复双手 6DoF 未完成。默认 DLL 仍不包含该候选。

范围更正：玩家确认先前清单误列 Mulcher 和 Ink Grenade，本轮只实施交互设计中剩余的 12 件武器／投掷物。此前“14 种”与这两件待查模型的条目属于当时的历史记录，已由本条和更新后的设计清单取代；无需再为它们追索模型或开发交互。

Lancer 下一机构边界审核：已核验原包的 `AR_Reload` 中 `b_AR_Latch` 与 `b_AR_Receiver` 各有 38 个旋转键、仅一个平移键，`AR_Reload_Fumble` 各有 60 个旋转键；`b_AR_Latch` 的原 LOD0 是 298 个刚性顶点／276 面，位于枪下方，源骨枢轴候选在枪体局部约 `(0.008,21.391,-7.540)`。有界解码显示 Latch 的存储局部姿态经历约 114.5° 相对旋转，但尚未验证动画键时间、游戏运行骨矩阵和可见操作面，因此不能只凭名字把它叫作枪栓。解码回执和检查用 OBJ 只留在忽略的 `analysis/lancer-support-probe-20261022/`、`analysis/weapon-partitions/Lancer-latch-uv/`，没有接入发行模型。

Lancer 插入判定候选：独立分支新增原枪固定弹匣井锚点、原版左手携弹匣姿态与当前枪组件矩阵的同帧几何检查；须先在井外合理深度且轴向、横向对齐，之后连续向内移动至到位。相同输入序号、空间／输入代次变化、跟踪间断、瞬移、背离和错向均不能记作插入。原版采集末段弹匣约在井口外深度 1.5、横向 1.8 个原始单位，当前私有容差只作候选，尚无 Quest 握把到掌骨标定。纯几何测试覆盖正常、旋转枪体、枪体移动和拒绝路径；默认关闭的 Playground 只记录插入候选，不转移弹药、隐藏手中件或完成装填。候选与默认禁用构建通过。购买版置后静音 PID 48504 复测原有 1.8 秒模拟左手轨迹，弹匣跟随、原版通知后的重挂、松握卸下与原版一次性结算均通过，未出现虚假的插入候选。新增测试专用的对准路径在购买版 PID 2348 上从井外深度 12 连续移到 2.34 个原始单位，横向 1.8、轴向及朝向对齐值均为 1，只产生一次插入完成候选；松握后原组件卸下，原版装填仍一次性结算。原始回执在忽略的 `analysis/lancer-support-probe-20261022/capture-insertion-observer-retail.json` 与 `capture-insertion-aligned-retail.json`。两次 Playground 入口和八个音频会话恢复通过，但启动器停止命令返回 1、各自原测试进程仍在，核对创建时间与路径后单独结束；完整 VR 会话仍为 `incomplete`。这些都是模拟手轨和候选记录，枪栓、黄色提示、完美奖励与头显验收均未完成。

Lancer 连体模型处理推进到原版独立组件运行验证：购买版自有置后静音测试场 PID 39128 只读反射确认 `SetPawnAmmoAttachment(bAttach)`、`SetWeaponAmmoBoneDisplay(bShow)` 与 `MagazineMesh` 的 `SetAbsolute`／`SetTranslation`／`SetRotation` 参数签名；该组件使用原版 `Assault_Rifle_Magazine`。PID 53212 将它挂到原角色、世界 X 平移 5 个原始单位并卸下回位，实测组件矩阵到目标误差约 0.015，接口调用与恢复通过。独立分支新增默认关闭的 Lancer 左手跟随候选，复用这个原版组件和枪上 `AmmoControl`，并用已测原版左手骨骼相对姿态构造**暂未标定 Quest 掌心**的握持候选。PID 38068 的测试专用 1.8 秒左手轨迹在原版 R3 装填保持期间取得四个同一组件的游戏线程样本：枪上控制器目标始终为 1、独立组件始终挂在角色上，原版通知将其卸下后候选重新挂接；跳过首次 `LocalToWorld` 尚未更新的样本后，组件世界位移 8.644 单位，手到弹匣相对偏移最大漂移 0.0015 单位。松握后组件卸下，随后原版装填完成函数一次性结算弹药。原始回执仅在独立工作树忽略的 `analysis/lancer-support-probe-20261022/`；三次游戏、Playground Hook、静音和入口均正常收尾，整局 VR 会话仍为 `incomplete`。这证明原件可被接管和移动，**不等于实体插入、枪栓、头显画面或任何其他武器已完成**。正式默认 DLL 与启动器未启用该候选；非 Lancer 部件继续优先核对原骨骼与原独立组件，只有缺失可用原件才制作可发布的独立覆盖模型。

Lancer 换弹结算边界新增零售版隔离验证：在购买版自有置后静音测试场 PID 44552，先由原版 `ForceReload` 进入 `Reloading`，确认 `EndOfReloadTimer` 计时器存在，清除后游戏时间越过原版 2.5 秒至 3.221 秒，已耗弹量仍为 1 且仍在装填；随后精确调用原版 `Reloading.EndOfReloadTimer` 状态函数，已耗弹量变为 0 并退出装填。证据在忽略的 `analysis/lancer-support-probe-20261022/capture-44552-reload-hold.json`。据此在独立分支增加 GVE5 认证 R3 意图（会话、空间、输入代次、世界和当前枪身份），以及默认关闭的 Lancer 游戏线程计时器移交试验。它只接受私有测试场、当前原版 Lancer 和可装填弹药，将原版自动完成改为 8 秒原版计时兜底；明确完成仍走原版状态函数，不自行改库存或发完美奖励。购买版 PID 53584 的**测试专用完成入口**验证了 3.270 秒时仍为已耗 1 发，完成后已耗归零、退出装填；回执在忽略的 `analysis/lancer-support-probe-20261022/capture-53584-reload-adapter.json`。这一运行样本未由真实 R3 或实体插弹触发，未验证 8 秒兜底实际到点、弹匣/枪栓动作、黄色引导、完美奖励或头显画面。最终加固了新输入代次与新鲜手部姿态匹配、空弹提示音和完成失败时重建兜底计时；这些后续改动编译及桥单测通过，尚未重复零售版运行。默认启动器与正式 DLL 未替换。两次自有游戏均退出，Playground 入口恢复、音频恢复各自通过；完整 VR 会话仍为 `incomplete`。

Lancer 左手副握点运行样本：购买版私有置后静音测试场 PID 47456 在原版装备动画结束、`ShouldLeftHandIKBeOn` 为真后，完成 24 次原版 `PlayerTick` 后的同线程人物左右掌骨／IK 骨及枪组件矩阵采集。枪体局部左掌心中位数为 `(8.556242, 43.525180, 0.165157)` 游戏原始单位，跨帧最大漂移 `0.000231`，左右掌心距离约 `44.86`。原始数据与分析留在忽略的 `analysis/lancer-support-probe-20261022/`；探针关闭、游戏退出和音频恢复完成，完整 VR 会话仍为 `incomplete`。据此将实测点接入**私有、Lancer 限定**的原枪组件双手跟随候选：左手必须在接触半径内新按 LGrip，松键、失踪或离开范围回退右手；绑定时已按住不能误抓。纯数学／边沿测试、候选和默认禁用构建通过。尚无实际手柄带动枪模、后坐力、射线或头显验收；未换正式启动器／DLL。

模型运行时只读增量：第二个购买版私有置后静音测试场 PID 37984 再次完成 24 帧原版持枪样本，并记录当前 Lancer 的独立 `MagazineMesh` 确实指向 `Assault_Rifle_Magazine`；`HiddenGame=false`、未挂在人物骨架，枪上 `AmmoControl` 存在且 `ControlStrength`／`StrengthTarget`／`BoneScale` 都为 0。一次原版静止持枪不能说明弹匣最终渲染可见性或装填交接时刻；下一步在原版换弹的同一条时间线上观察两份组件。首轮对象扫描遇到已失效对象类引用后已改为跳过该槽，复跑成功。私有探针退出码 0，游戏进程退出、Playground 入口恢复及游戏音频恢复均核对；完整 VR 会话仍是 `incomplete`，默认构建和发行包未替换。

Lancer 原版换弹视觉交接实测：购买版私有置后静音测试场 PID 34536 在测试场把原枪已耗弹量临时设为 1，调用原版 `ForceReload`，私有游戏线程只读采集 318 个顺序样本。相对原版 `ReloadStartTime` 的采样间隔：枪上 `AmmoControl` 由 0 到 1 为 `0.258–0.275 s`；独立 `MagazineMesh` 挂上角色骨架为 `0.774–0.791 s`；独立件卸下且枪上控制器回到 0 为 `1.479–1.500 s`；原版退出 `Reloading`、已耗弹量由 1 归 0 为 `2.497–2.510 s`。这些是连续两帧之间的事件范围，不是动画通知的精确时间，也不是 VR 完美期限。`HiddenGame` 全程为 false，但挂接和渲染可见性不同；正常 VR 弹匣仍需绑定左手控制器并验证画面。原始逐帧数据与派生摘要留在忽略的 `analysis/lancer-support-probe-20261022/`。此次游戏进程、Playground 入口和音频均清理，完整 VR 会话仍为 `incomplete`。默认启动器／DLL 未更换。

购买版实例补充：私有置后静音测试场 PID 47308 的当前 Lancer 实例，只读反射验证了装填时长 `2.5 s`、起点约 `0.7 s`、三个未覆盖宽度字段 `-1`、完美宽度数组 `0.1／0.075／0.05 s` 和角色索引 0。据已匹配的 Getter 主块与历史尾块推导此实例候选终点约 `0.800000012 s`；未调用 Getter，也未触发装填奖励，不把候选值配置为正式期限。回执在忽略的 `analysis/weapon-reload-probe-20261022/reload-window-instance-47308.json`。私有组件编译、入口还原、原生清理、音频恢复和游戏正常退出通过；完整 VR 会话仍为 `incomplete`。正常启动器及正式 DLL 未更换。

完美边界原生核验增量：仅分析已与购买版当前 Lancer 虚表目标逐字节匹配的判定函数主代码块，确认完美条件为 `elapsed ≤ start + super`，包含窗口右边界；独立分支纯装填事务已按此修正，恰好到点／晚一步测试通过。Getter 主块使用默认对象上的难度索引数组作为未显式设置时的完美宽度；其分离尾块目前仅为历史快照。哈希核对的购买版 `WarfareGame.u` 基类数组为索引 0／1／2 对应 `0.100／0.075／0.050 s`，六把枪及中间基类未显式覆盖。报告在忽略的 `analysis/weapon-reload-probe-20261022/active-reload-defaults-with-difficulty.json`。逐枪当前游戏实例输出和索引尚未读取，这些默认值推导的候选期限不写入正式配置。数组解码单测与 `WeaponReloadModelTest` 通过；未替换默认 DLL 或启动器。

模型与装填原生实测增量：从当前 0.1.0 工作区选择性移植早期 Playground 预留及菜单定位修复到独立分支的测试组件，保留本分支私有观察器；`PlaygroundHookTest` 的邻近窗口／全窗口占用正反例、装备意图和默认禁用构建通过，未替换正式默认 DLL。购买版私有置后静音测试场 PID 52656 读到原版换弹包装入口 RVA `0x3e7ffc`／`0x39bfcc`；前者前 256 字节、后者完整 47 字节与历史运行时代码快照相同。两者分别转经武器虚表 `0x638`／`0x630`；随后 PID 44876 的当前 Lancer 实例验证了虚目标 RVA `0x3e81b0`／`0x3e7d28`，各 4 KiB 与历史运行时代码快照逐字节一致，Getter 的分离尾块不在比较范围内。这只证明当前代码身份，尚未算出逐枪完美期限。PID 43448 成功取得 Gnasher 原版 `SG_Chamber` 的 35 组真实骨骼矩阵，相对枪身最大转角约 32.43°、局部 −X 转轴，骨原点对应原参考枢轴。原杠杆骨骼可直接作为模型运动源，仍未接手柄、黄色引导或装填奖励。上述原始字节与矩阵仅在忽略的 `analysis/weapon-reload-probe-20261022/`；完整 VR 会话保持 `incomplete`，Playground 入口还原、原生清理、XR 退出 0、音频恢复和测试游戏正常退出分别核对，不当作头显验收。

原生换弹入口定位接线：私有 Playground 在已认证武器和自有置后静音测试场读取 `WarWeapon.GetActiveReloadValues`／`DetermineActiveReloadSuccessOrFailure` 的 UFunction 入口，验证当前游戏映像可执行页后仅记录 RVA、native 索引及参数大小；不调用或修改装填函数，也不依赖实体手柄。装备意图候选和默认禁用构建通过；此时尚无零售版入口地址或完美期限运行证据。模型方面继续按已核验的骨骼／独立弹匣组件处理，不将检查用拆件 OBJ 直接覆盖原游戏资源。

枪上弹匣控制器观察增量：只在私有认证手部／装备测试中沿原版 `FindSkelControl(AmmoControl)` 查询当前枪模，只读记录 `ControlStrength`、`StrengthTarget`、`BoneScale` 与是否存在。`Engine.u` 中该函数的参数和返回类型分别为 `NameProperty`／`ObjectProperty`，调用前还按运行时反射复核。私有装备意图和默认禁用构建通过；尚无零售版有效运行样本，控制器数值也不能代替实际画面可见性。

模型原件观察增量：在只限私有认证手部／装备测试的游戏线程回调里，加入对原 `MagazineMesh` 的只读检查，记录当前武器是否有独立弹匣组件、其静态模型名、显隐与是否挂在角色骨架上。它与已有原枪组件姿态观察配对，用于避免未来实体换弹时同时显示枪上和左手两份弹匣。私有装备意图、枪模跟手及默认禁用构建通过；没有新的零售运行样本，不能据此认定换弹视觉交接完成。

Snub 静态拆件复核：在已验证的原包上用同一刚性骨骼提取器分出滑套 445 面、左右翼片各 60 面，三个目标各自与剩余枪体恰好覆盖原 LOD0 的 3,045 面；UV0／法线与来源哈希记录在忽略的 `analysis/weapon-partitions/snub-b_HG_*/`。这补齐原先只拆出 Snub 弹匣的检查预览，不是游戏内可抓取、动画驱动或发行资源。

Boltok 静态拆件增量：哈希匹配原包后对 `Object01`–`Object11` 每根骨骼导出活动件／其余枪体配对预览，保持 UV0、法线和原材质槽；11 件共 2,417 面、主体 4,325 面，合计原 LOD0 的 6,742 面。检查件留在忽略的 `analysis/weapon-partitions/boltok-verified-Object*/`，不作为可发布或已接入的游戏模型。

Boltok 模型／动画绑定复核：SHA256 核验的原版 `Weap_LocustPistol.WeaponMesh` 显式引用 `Geist_Pistol.Boltock_Pistol` 网格与 `COG_Pistol.COG_Pistol` 动画集；前者 12 根通用命名骨骼，后者 `HG_Fire`／`AR_Reload`／`AR_Reload_Fail` 只有五根 `b_HG_*` 轨道且无同名项。暂不把 Snub 活动件方案移植到 Boltok；先查运行时实际动画和可见部件，必要时逐件控制或做覆盖件。包内轨道回执位于忽略的 `analysis/boltok-pistol-animation-audit-20261022.json`，并非运行时验证。

完美窗口静态复核增量：新增只读、哈希约束的原版默认对象字段审计，重新确认 WarWeapon 基类 `AR_SweetSpotDuration`／`AR_SuperSweetSpotDuration`／`AR_PreReactionWindowDuration` 为 `-1`，Lancer、Hammerburst、Gnasher、Longshot、Snub、Boltok 具体类未显式覆盖这三项；已有 `AR_PossibleSuccessStartPoint` 和 `ReloadDuration` 数值重现。结果位于忽略的 `analysis/active-reload-explicit-defaults-20261022.json`。这仍未确定原版 SuperSuccess 窗口终点，不给逐枪体感期限填猜测数值。

继续按保存的原版 HUD 脚本核对出显示层的完美区终点候选为 `ActiveReloadStartTime + SuperSweetSpotDuration`（均为 `GetActiveReloadValues` 的实际输出），普通成功区另加 `SweetSpotDuration`；历史真实 SuperSuccess 样本仍见对象上的三项时长为 `-1`。因此下一步只分析两个原生函数的有界 getter／判定路径，或在受控零售会话只读截取其真实输出，不用显示层公式冒充原生严格边界，也不主动调用未知副作用的原生函数来声称只读。

完美窗口原生定位增量：先查 `knowledge.sqlite` 后在私有 IDA 库中补入并 SHA256 核验历史运行时的代码与五个数据节，确认 `GetActiveReloadValues`／`DetermineActiveReloadSuccessOrFailure` 名称字面量分别位于 `0x14174ed60`／`0x14174edb0`。同 RVA 的零售磁盘 EXE 是 Themida 保护字节，不能与历史运行时内容等同。当前字面量没有直接 IDA xref、绝对指针或指向精确字符串的 RIP 相对 LEA 命中，尚未定位两个原生函数体或证明完美截止值。私有报告 `analysis/ida-reload-window-20261022/lookup.json` 保存来源与失败边界；IDA 租约已关闭。

从 `codex/0.1.0` 提交 `315473dca5008e753011ae905783079de6f04770` 创建独立工作树与 `codex/weapon-6dof` 分支；原工作目录仍由其他会话用于 0.1.0 开发。仅将本次[武器 6DoF 交互设计与验收规格](../../weapon-6dof-interaction-design-20261022.md)及相关文档索引迁入新分支，未复制原目录中其他会话尚未提交的 native、Playground 或启动器改动。后续完成功能并验证时，以届时最新默认版本为集成基线。

规格记录自由双手、身体槽位取枪取弹、R3 物理装填、逐枪黄色引导、霰弹数量不足与射后循环、枪口弹道及原版后坐力、武器弹药卡片、手雷手势投掷、动画接管、臂展校准、可配置转向与普通行走去惯性。用户确认的完美规则为原版该武器完美窗口结束前完成全部动作；提前完成也算，超时为普通装填且不加卡壳惩罚。

分支建立时只迁移需求文档。随后开始实现：OpenXR 探针增加显式 `--controller-input` 路径，在同一会话内读取双手 grip／aim 姿态、扳机、握持、摇杆及 ABXY，并在立体图像发布前写入独立的 v3 手柄共享帧。原立体帧 v4 格式没有变动。采集端为按钮、扳机和握持键保留按下／松开累计次数，握持按下还保留当时的手部位置，避免 Native 帧率较低漏掉短按或用延迟后的手位判错身体槽。失焦、交互配置或 LOCAL 空间代次变化会换代，重获焦点的首帧只做基线。Native 侧仅在 `controller_input_observer=1` 时按会话、序号、LOCAL 空间代次和预测时间读取同一帧；原有认证 PlayerTick 所在构建还可只读冻结最近有效输入，识别双手肩部／裤兜、R3、L3 和扳机意图，记录首次基线、丢失、过期与世界代次拒绝。当前没有向游戏注入手柄动作。

纯逻辑模块已加入两秒双握持稳定臂展测量、按左右手区分的肩部／裤兜槽位命中，以及装填弹药事务：弹匣和逐发装填、霰弹一轮最多三发且库存不足缩量、取弹保留与松手归还、取消时守恒、未完成机构阻止射击、射后循环、原版完美窗口**终点前**完成的严格判定及超时普通结果。这里的槽位半径和站姿偏移是待头显调整的初值；装填模块尚未接原版库存、动画、计时器、奖励或音效，不能据单元测试称实际游戏已经完成体感换弹。

验证：私有 Release 构建 `OpenXRPoseProbe`、默认和已启用原有认证 Tick／嵌套调用观察器的 `GearsCapture` 均成功；手柄共享帧与按键边缘、身体槽位意图、臂展／槽位、装填事务、既有第一人称视图和当前武器身份用例通过。编译沿用现有 Windows SDK 的警告，未启动游戏或模拟器，未更换默认启动器、DLL、XR 可执行文件或发布清单。下一步在原版认证游戏线程上关联真实开火与武器身份，再接实际手模、枪模、库存和逐枪机械动作；各枪完美窗口数值、弹药模型与枪口最终发射线仍待验证。

后续独立分支增量：私有连续会话增加 `--controller-input-observer`，只有认证的 UI／anchor／crosshair 观察器齐备才同时给 XR 生产者和 Native 消费者开启只读手柄帧；普通启动仍不启用。有效双握持两秒臂展测量可在会话完整、原生入口恢复确认后写入本机 `%LOCALAPPDATA%/GearsVR/weapon-calibration.json`，下一次私有会话复用；`--remeasure-armspan` 忽略旧值进行本次重测，失败不覆盖原记录。后台置后测试和已知模拟输入不写入该记录。校准引导界面和启动器开关尚未加入，不能称首次校准体验已完成。玩家补充 Gnasher 是左手稳握前端、右手下掰握把并回位的杠杆式霰弹枪，射后高亮应是握把／杠杆而非泵动前护木；纯换弹模型现在要求与武器匹配的机构动作，真实双手轨迹判定仍待接入。Longshot 需要镜片内独立远景渲染，单纯放大画面不可作为完成；最初 14 种武器／手雷清单后经玩家更正为 12 种，Mulcher／Ink 不纳入开发。此增量仅完成私有命令与离线逻辑，未在游戏／头显验收。

离线复查：Native 默认及认证 Tick 私有 DLL 构建、换弹模型、校准格式与私有命令接线通过；连续会话 60 项测试及便携文件清单 21 项测试通过。`docs/weapon-aim-hit-chain.md` 和 RE 知识库再次核实 `InstantFire`／`GetAdjustedAimFor` 是脚本链，现有共享 VM 入口已有 Hook，不能把函数名当独立机器码入口直接叠加新 Detour；真正开火链仍缺一次原版实际调用轨迹，下一步先在原 Hook 所有者下只读关联开火范围、返回 ABI 和当前枪身份，再改枪口方向与起点。

Gnasher 杠杆手势增量：新增纯逻辑的双手行程识别器，要求左手持续抓住前端、右手握把有效追踪，在指定武器／世界／追踪空间代次下完成原位→下掰→回位；一帧跳角、采样中断、松手和回中会重置，成功后才把 `lever_down_return` 事件交给换弹／射后开火门。该角度必须由未来经原版动画核对的杠杆枢轴计算，当前尚无游戏枪模几何接入，阈值也未按头显调定。独立测试构建与轨迹／射后门控用例通过。

枪姿态增量：加入只消费同一 XR 双目 ticket 的双手枪体求解器。右手握把决定枪体锚点；左手确实握住副握点且追踪与臂距合理时参与方向修正；枪口位置及轴线由逐枪局部挂点计算，并能用冻结的回中参考转换为游戏世界射线。纯逻辑测试覆盖单／双手、仅头动不改变枪线、错帧和失去追踪后的拒绝或单手回退。当前没有已核对的逐枪握点／枪口局部坐标，也未写入游戏枪模或开火函数；测试只证明数学与数据门控，不证明实际弹道改变。

模型拆件核验增量：原版六件已定位的武器 `SkeletalMesh` 通过包内哈希和骨骼层级审核；Gnasher 确有独立命名的杠杆骨骼，Lancer／Hammerburst 的弹匣、Longshot 的枪栓／镜体、Snub 的滑套／弹匣也有相应骨骼。查看用 GLB 丢失蒙皮，不能由其外观看出原版部件是否独立。新增可复现的只读审核脚本，报告放在忽略的 `analysis/weapon-bone-audit.json`。动画行程和游戏内控制未验证；Boltok 骨骼命名不具部件语义，其余武器仍待逐件调查。

武器几何增量：只读审核从六件原始模型中提取参考骨架的父骨和局部姿态，Gnasher 杠杆的父骨确认为枪身。新增左手稳住枪身、右手沿杠杆圆弧下掰并回位的几何适配器，合成姿态和顺序识别测试通过。枢轴方向、右手握点和行程尚需原动画标定，仍未接入原版射击或奖励，不能视为可玩功能。

装填几何增量：增加弹药件从装填口外沿指定轴向连续插入的判定及装填口／枪栓／杠杆单步引导状态；合成姿态验证了对准、行程、瞬移、误触和回中失效。原模型局部坐标、高亮渲染及实际弹药事务仍未接入。

模型覆盖增量：在已定位的另外四件地图包网格及 Frag／Smoke 手雷上复用来源哈希、骨骼参考姿态与 LOD0 蒙皮审核。清单中已有 12 件模型获静态核验；其中 10 件可刚性分出检查用部件，Lancer 的 36 个混合权重顶点和 Torque Bow 的 48 个跨骨骼三角形阻止**整件网格**无缝逐骨拆分。后续逐区段复核确认 Lancer 的混合权重只在电锯模糊特效，不在弹匣。Boomshot 弹匣、Hammer of Dawn 扳机、Troika 多个活动部件及 Bolo 手雷分节链条已有原骨骼证据。Mulcher、Ink 的网格与所有武器的动画行程／游戏内操控仍待核验。

零售包身份增量：只读解压购买版工作副本的 `WarfareGame.u` 与 `MP_WarMachine.war` 后，两个解压包的完整 SHA256 均与既有模型分析来源逐字节一致。临时解压文件已清除；这是静态包核验，不是运行或头显验收。

原版 R2 输入实测：独立、静音、窗口置后的零售版测试场中，Hammerburst 仅一次 `WarPC.ButtonPress('R2')` 就建立原版持续开火状态，约 1.2 秒弹药消耗 15 发；仅一次对应 Release 即清除控制器开火／按住状态。此前诊断脚本为机枪显式双调 Pre+ButtonPress，不能当作最小调用证明；本轮独立证据支持后续 VR 扳机采用单次 ButtonPress/Release 语义。测试停止后独立游戏进程退出，菜单桥接 Shutdown 返回 0，游戏音频恢复。其他武器、追踪丢失和输入来源去重仍待实现与验证。

模型蒙皮增量：继续解析经哈希验证的 LOD0 活动骨骼表及 64 字节顶点权重，发现 Gnasher 杠杆对应 1,192 顶点、1,260 三角形，与主体三角形无交叉。已用保守工具把 Gnasher、Hammerburst、Longshot、Snub、Boltok 的可刚性分组灰模导出到忽略目录 `analysis/weapon-partitions/`，并逐件核对三角形总数和输出哈希。Lancer 整件有 36 个混合权重顶点，保守工具拒绝整件刚性拆分；后续已查明这些顶点属于电锯模糊特效，弹匣本身可刚性分出。Longshot 镜体／镜盖骨骼不在当前 LOD0 活动骨骼表内。导出仅有几何，不含贴图、骨骼动画或运行时替换；实际动画与 VR 操作仍待验证。

开火链只读观察增量：为现有脚本身份凭证增加可选的六函数序列（`WarWeapon`／`Engine.Weapon.InstantFire`、`WarPawn.GetAdjustedAimFor`、`WarWeapon.GetAdjustedAim`、`Engine.Weapon.CalcWeaponFire`、`WarWeapon.ProcessInstantHit`）。仅在私有手柄观察会话申请；原始角色、包、脚本地址／大小与入口每次由当前进程核验。复用已有 VM 调用边界，不设第二个 detour，不调用武器函数，也不读取结果缓冲或参数。普通 `WarPC.PlayerTick` 的同步调用栈中，记录原始调用前后元数据；子调用必须有实际 `Engine.Weapon.InstantFire` 函数体祖先，才标作射击体内观察。完整 Tick 结束后再次验证身份和世界代次才提交有限行数；世界重绑定也申请同样凭证。纯内存测试通过；尚无实际游戏开火轨迹，因此不证明该 Tick 覆盖连发计时器，也未取得射线数值、更改弹道或接通枪口。

开火方向只读采样续作：现有共享 VM Hook 的同一次同步返回中，只有经当前进程 `ReturnValue` 结构身份、12 字节大小、`0x580` 标志、`Rotator` 目标与准确调用链重新认证的 `WarPawn.GetAdjustedAimFor`／`WarWeapon.GetAdjustedAim`，才读取原入口 R8 所指的 12 字节并记录三个原版 Rotator 整数；RAX 保持原样透传。为两次调用加直接父帧及 `parent_aim_id` 限制，不能把 HUD 查询或另一次射击混入。Python 26 项配置测试、C++ INI 解析变异测试、私有观察器和默认关闭构建通过。未实测购买版返回值；`CalcWeaponFire` 射线参数、碰撞和实际枪口弹道仍未读取／修改。

首发射线输入只读采样：继续在同一个共享 VM Hook 中认证 `Engine.Weapon.InstantFire` 的原始 `CalcWeaponFire` 调用点（字节码 `0x95` 起）、`StartTrace`／`EndTrace`／`PushedForwardStartTrace` 的 12 字节 `Vector` 结构和局部区边界。仅在 role4 的直接调用者执行帧代码指针是 `script+0x9e`、当前枪与父帧匹配且前后帧／局部区指针稳定时，复制这三个原版输入向量；`ImpactList` 只核对令牌身份而不读取，递归穿透不作为首发射线。变异测试拒绝令牌、字段大小、局部区、父帧、非有限坐标；私有构建与解析单测通过。尚未取得购买版开火样本，实际碰撞／伤害与 6DoF 枪口一致性仍未验证。

实验性游戏线程 R2 接入：独立的 `OriginalR2Fire` 状态机只消费精确匹配的 XR 帧与累计边沿，核对原版 PC／Pawn／当前枪身份；处理漏帧内的短按、已有游戏按键占用、追踪失效和 100 ms 过期。只有已实测持续开火的 Hammerburst 被私有 `Playground` 候选允许调用原版 `ButtonPress/Release('R2')`，Torque Bow 等松开可能发射的武器仍禁止。候选入口需要显式启用，默认构建编译选项为关闭。最终候选的隔离购买版测试场以模拟来源完整经过共享帧、游戏线程及原版开火链：按住约 1.2 秒使用 15 发，55 ms 输入空档未误松开，正常 Release 清空 `bFire`／`bHoldingButton_R2`；再次按住后终止发布端，过期恢复清空两项且弹药停在 24 发。`analysis/playground-20261021/vr-fire-bridge-1022f/vr-fire-bridge-receipt.json` 标记通过，游戏、工作者退出和八个音频会话恢复已核实。中间一轮因测试控制文件共享错误在开火前停止，另一轮测试发布器把单次共享锁竞争当作永久故障提前退出；均完成桥接停机和进程清理，修复测试发布器后最终回执通过。离线状态机及候选／默认两种 DLL 构建通过；这尚非真机、全部武器、枪口弹道或默认启动器验收。

原版 L2 与候选接入：单独零售版实测 `WarPC.ButtonPress/Release('L2')` 在 `bToggleTargeting=false` 的 Hammerburst 上建立／退出 `bPressingTargetingKey` 与 `bIsTargeting`，不需要额外调用 Pre；这一样本的 `FOVAngle` 未变化，不代表真实狙击镜已处理。隔离回执 `analysis/playground-20261021/l2-semantic-1022a/l2-button-receipt.json` 通过，游戏退出、桥接 Shutdown、八个音频会话恢复已核实。新增可选 `GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2`，在现有候选共享帧上处理左扳机边沿，原版切换开镜设置会被阻止；短按、共用输入状态、追踪丢失和身份改变的纯逻辑测试通过，默认 DLL 无相关导出。合并候选的后台联测在菜单视口尚未出现时超时，未投送模拟输入，故没有游戏内 L2 桥接通过结论；`analysis/playground-20261021/vr-l2-bridge-1022a/` 记录关闭与音频守护，独立游戏进程随后按身份清理。Gnasher 的两次 `SG_Chamber` 运行骨骼采样也未到达菜单入口，仍需在正常启动后读取主骨／杠杆矩阵；此时静态分件证据仍有效，但没有动画行程或逐实例骨骼控制证据。

Gnasher／Longshot 原版动画静态核验：新增 `tools/weapon-models/audit_animation_tracks.py`，用源包与导出各自的 SHA256 检查有界读取 `AnimSet` 和 `AnimSequence`，选定一根骨骼才解码采样，衍生结果留在忽略的 `analysis/`。购买版身份已核对的 `WarfareGame.u` 中，Gnasher `SG_Chamber` 的杠杆骨骼有 18 个旋转键，枪身仅 1 个；压缩键的杠杆父局部峰值约 32.40°，先转到约 24.02°、再回位。`SG_Reload`／失败／成功序列另有 45／60／27 个杠杆旋转键。Longshot `LS_Reload`／失败序列的枪栓各有 50／160 个旋转键，镜体和镜盖各只有 1 个。Python 语法检查与两枪完整轨道审核通过，Gnasher 18 键解码曲线复核通过。动画存储四元数还需转换到网格／世界坐标，游戏播放速率与完美窗口也须另证；零售游戏启动未进入菜单视口，因此本轮没有新增运行时骨骼或头显验收。此时无需真的裁切原版 Gnasher 枪模；优先复用独立蒙皮骨骼。Lancer 混合权重位置已在后续复核中更正为电锯模糊特效，而非弹匣交界。

扩展动画审核：允许某些序列省略压缩格式属性时只读取明确写入的轨道偏移／键数，不推断未写的默认格式，也不做该轨道的四元数采样。Lancer、Hammerburst、Snub、Bolo 四组 AnimSet 的完整有界审计通过。Lancer 原版装填同时驱动弹匣、机匣、卡扣；Hammerburst 装填活动锁扣而弹匣骨骼常量；Snub 开火时滑套有 6 个平移键；Bolo 准备动作驱动多节链条。相应哈希与计数回执位于忽略的 `analysis/{lancer,hammerburst,snub,bolo}-animation-tracks.json`。这进一步限定了哪些部件应复用原动画／蒙皮、哪些独立手持弹药需另建交互，但不构成游戏内操作通过。

地图包动画审核：已与购买版逐字节比对的 `MP_WarMachine.war` 另有 Boomshot、Torque Bow、Troika 的 AnimSet。工具在该源包哈希下完整读取三组轨道；Boomshot 装填枪管有 86 个平移键、弹匣骨骼为常量，Torque Bow 张弓在八根弓臂骨骼上各有 54 个旋转键，Troika 双旋转件各有 10 个旋转键。结果在忽略的 `analysis/{boomshot,torquebow,troika}-animation-tracks.json`；Torque Bow 的跨骨三角形继续使用蒙皮，不能通过逐骨刚性切割代替。

Gnasher 实例骨骼入口复核：Astra Max 只读核对已有脚本／对象证据，确认默认 `WeaponMesh.Animations` 直接为 `AnimNodeSequence`；`WarWeapon.SetWeaponAnim`、`FreezeWeaponAnim`、`SetWeaponAnimPosition` 为先验证原 `SG_Chamber` 姿态映射的最小原脚本路径。它接管整条序列且 `SetPosition(..., false)` 不触发通知，不能冒充单骨覆盖或完整音效流程。真正叠加单骨的 `AnimTree`＋`SkelControlSingleBone` 仅有声明和原脚本兼容路径，实例创建、`bIgnoreControllers`、GC 与更新顺序尚未验证；现有 Playground 请求在原 Tick 前执行，不是已证实的动画后修改点。下一次能稳定进入菜单的隔离零售版测试，先读当前枪实例与原序列主骨／杠杆相对矩阵，再决定最小控制候选。

模型运动测试入口已落地为 `tools/playground/gnasher_sequence_probe.py`：使用已拥有的置后、静音私有购买版进程，通过原版拾取／装备与 `SetWeaponAnim`、`FreezeWeaponAnim`、`SetWeaponAnimPosition` 在 35 个位置取主骨／杠杆矩阵，运行时按骨名校验索引，停用 Playground 后通知测试启动器收尾；失败回执与衍生骨骼矩阵都只写忽略的 `analysis/`。脚本语法和命令入口通过，**尚无新的游戏样本**；此前两次菜单未就绪的结果仍不构成动画控制证据。Gnasher 本体不需预先切割，真正交互接线还要先拿到有效实例矩阵与手柄方向映射。

该探针两次隔离购买版运行的收尾结果已核对：首次在原版 UE3 名称／对象表可用前建桥失败，因此只对此类瞬时表头错误增加最长 90 秒重试；第二次建桥成功但 `PlaygroundEnable` 等待菜单视口 60 秒超时，未拾取 Gnasher、未取骨骼矩阵，回执 `analysis/gnasher-sequence-runtime-1022d.json` 的样本数为 0。`PlaygroundShutdown=0`，被拥有的游戏进程已退出，音频守护 `restored=true`；不再无条件重复同一开局。运行时动画行程与手柄驱动仍无有效样本。

Gnasher 完整霰弹模型原型：已查原版资源只有抛出的开口空壳，故新增纯程序化 `assets/weapon-6dof/gnasher-round.glb`，封口、底座、底火及四色材质均为新几何／材质，参考尺寸 33.7×113.5 mm，544 个三角形。`tools/weapon-models/build_gnasher_round.py --check` 逐字节复现提交的 GLB，结构与尺寸检查、本地侧面／斜视预览通过。该原型**尚未加载进游戏**，装填口配合、抓点、碰撞、纹理和头显尺度仍待验证；原空壳网格／检查图仍留在忽略的 `analysis/`。

启动器校准入口增量：`launcher.py start --remeasure-armspan` 现在只在清单启用已认证的 controller／crosshair／UI／anchor 观察器时传给连续会话。双语 WinForms 界面在该清单下显示臂展行：无有效本机记录时首次测量必选且不可取消，有记录时默认复用并可选重测；测量仍由已有 OpenXR 双握持两秒逻辑执行，失败不覆盖旧值。普通冻结清单未启用该能力，界面不会误导玩家以为当前默认版已有 6DoF。启动器 Python 78 项测试、连续会话及校准 62 项测试、C# 编译与隐藏自检通过；私有 GUI 候选留在忽略的 `analysis/vr-launcher-ui/`，未安装到玩家默认入口。头显内进度／槽位确认与真机测量待完成。

完美换弹期限复核：按项目规则先查逆向知识库与已保存的 UnrealScript 类默认值，Astra Max 只读复查了七把已定位武器。Lancer／Hammerburst 的 `AR_PossibleSuccessStartPoint` 默认 0.70 秒、`ReloadDuration` 2.50 秒；Gnasher 为 0.85／3.00，Longshot 为 1.00／3.00，Snub／Boltok 为 0.50／2.00；Boomshot 这两个值未取得。现有保存证据没有这些枪的**完美窗口终点**，开始可能点和普通时长均不得代用。体感换弹的逐枪截止值仍未配置，原版奖励／音效接入仍待完成；详见交互设计中的计时表。

本轮离线验证还覆盖 `OriginalR2FireTest`、`OriginalL2AimTest`，并成功构建默认、仅 R2、R2+L2 三种私有 Playground DLL；导出表分别为无 VR 入口、仅 R2 入口和额外的 L2 状态入口。模拟发布器增加左扳机及双扳机测试动作，仍只用于置后静音测试，不进入默认启动清单。

L3 回中接线增量：认证的游戏 Tick 仅将已接受的 L3 边沿写入一次性请求，包含当前世界、XR 会话和 LOCAL 空间代次。渲染线程先验证实际冻结的立体帧，再把匹配的请求应用到原有 `StereoGate` 回中参考；首帧已以当前头姿建参考时不重复回中，拒绝帧、换世界／会话／空间或重放均不能消费为有效动作。私有认证 Tick Native DLL 构建、立体参考测试和新增请求边界测试通过；普通冻结清单及玩家默认 DLL 未更换，真实手柄、游戏内身体槽与枪体同步效果仍待验收。

Lancer 弹匣拆件复核：原先分析工具错误地把整件网格的活动骨骼表用作每个区段的顶点骨骼映射，由此得出的“10 个弹匣接缝三角形、218 面弹匣侧”结论**已撤回**。经原包哈希、LOD0 区段索引、各 chunk 的 `BoneMap` 及 CPU／GPU 顶点逐项核对，13,397 个原始三角形中真正归属弹匣骨的是 156 面，另外 13,241 面归属枪身／其他部件；弹匣边界无需权重裁切。更正后检查用 OBJ 与报告在忽略的 `analysis/weapon-partitions/Lancer-corrected/`。这仍不是可直接游戏装卸的模型，原包和默认版本未改。

Lancer 拆件错误来源已定位：先前 62 面的 `X≈0` 片状块位于材质 `BlurringBlades_Delta_MAT` 的电锯模糊区段，不能归入弹匣。正确弹匣是 120 个刚性顶点／156 面的单一连通块；整枪其余 36 个混合顶点、40 个关联面和 8 个跨骨面都在电锯特效区段。审核器与两个拆件工具现按 chunk 自身的 `BoneMap` 取骨骼，并逐顶点校验源 CPU／GPU 几何和影响；12 件已定位网格重新审核通过，只有 Lancer 的旧骨骼归属计数变化。旧错误派生预览已移至 `analysis/weapon-partitions/Lancer-weighted-invalid-20261022/` 并标记无效，新工具 5 项单测通过。要做可装卸件，仍需原 UV／材质、碰撞和运行时动画／显示核对；已有 `MagazineMesh` 可优先复用。

刚性部件首次导出增量：`tools/weapon-models/extract_rigid_bone_lod0.py` 仅接受目标骨骼所有有效顶点权重为 255 且目标三角形不跨界的模型，记录源材质槽、源哈希与派生 OBJ 哈希。已离线导出 Lancer 弹匣 120 顶点／156 面、Gnasher 杠杆 1,192／1,260、Hammerburst 弹匣 271／272、Snub 弹匣 279／314、Longshot 枪栓 687／894、Boomshot 弹匣 700／1,236、Hammer of Dawn 扳机 164／156、Troika 两个旋转件各 488／600；Lancer 弹匣在两角度灰模预览中位于枪身下方。跨 chunk 相同局部骨骼索引、无关部位混合权重、目标自身混合权重和跨界面均有合成单测，7 项通过。当时声称“保留源主 UV”**已撤回**：首次导出读取了错误偏移，几何归属和面数有效，旧 OBJ 的贴图坐标无效；更正如下。

刚性部件 UV／法线更正：购买版包内三件不同骨骼部件的面法线核对及 [UEViewer 的 GoWU 专用 64 字节顶点读取分支](https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealMesh/UnMesh3.cpp#L3743-L3778) 均指向此 GPU 顶点的两组打包方向位于偏移 12、16，UV0 始于偏移 20；旧工具从 24 读取两个浮点，实际是 UV0.v 与 UV1.u。工具现按 20–27 读取 UV0、按 16–19 解码法线并反转 OBJ 面序。九个刚性部件重新导出到忽略的 `analysis/weapon-partitions/uv-normal-fixed-20261022/`，源哈希、顶点与三角形数量逐件保持一致，导出面与法线平均点积为 0.698–0.984；旧 `*-uv/` 目录有失效说明，不能用于贴图。提取工具的 8 项合成检查通过；新的元数据保存原骨骼父级、原始父局部姿态及逐级组合的静态参考枢轴候选。九件候选距各自几何包围盒中心均小于对角线的 0.43 倍，但原版运行时骨矩阵、铰链轴和手柄行程仍未证实。新 OBJ 仍不含原材质贴图、蒙皮、碰撞或游戏内显隐控制，不能直接装入 Mod 或公开包。

成对模型拆分增量：同一来源校验工具现在同时导出活动骨骼件与不含该件的枪体剩余部分，保留源 UV0、法线及材质槽索引，要求目标骨骼无混合权重和跨界三角形；剩余部分可含原版混合蒙皮，OBJ 仅作静态检查。九组新导出在忽略的 `analysis/weapon-partitions/paired-20261022/`，逐组核对两份 OBJ 的面数、哈希和合计原始 LOD0 面数，均无少面或重复面；例如 Gnasher 杠杆 1,260＋枪体 5,060＝原网格 6,320 面，Lancer 弹匣 156＋其余 13,241＝13,397 面。提取工具的目标／剩余面覆盖与拒绝跨界单测通过；`paired-preview.png` 的侧面和斜面预览也显示杠杆与弹匣落在正确局部。原版脚本另确认 Lancer、Hammerburst、Boomshot 的装填通知使用 `AmmoControl` 隐藏／显示枪上弹匣，`SetPawnAmmoAttachment` 可把独立 `MagazineMesh` 接到左手，Lancer／Boomshot 弹出时还有物理弹匣；VR 运行时应先接管这些现成对象和显隐，避免复制弹匣。当前只有离线几何和脚本证据，手柄附着、原版通知时序、材质及实机渲染未验证。

原版库存槽位映射增量：经知识库索引与已保存脚本核对 `WarPC.WeaponSlotFromDpadDirection`、`WarPawn.SetSlotAttachment`，将 VR 左裤兜手枪、右裤兜手雷、左肩武器 2、右肩武器 1 分别映射到原版 0／1／2／3 号槽；取弹位置无库存武器槽。`BodySlotsTest` 编译和校准／位置／编号断言通过。镜像角色的视觉肩位会交换，且没有调用原版切枪；游戏内装备动作仍待单独验证。

取放枪所有权状态增量：新增 `WeaponEquipmentModel`，将右手 RGrip 槽位点按与原版库存快照绑定。空槽只返回一次反馈请求；非空槽先请求原版切枪，待当前武器身份和动画结束同时确认才锁定持有；原槽再次点按请求收枪，RGrip 松开不改变持有。原版拾取动画期间双手由动画占用，结束后按实际槽位和当前武器交还；世界／角色切换、武器移除及原版拒绝切换会清除挂起状态。独立 MSVC `/W4 /WX` 用例通过，覆盖空槽、切枪等待、收枪、拾取交接与身份突变。此模块尚未接入游戏线程库存读取、原版切枪、枪模显隐或音效，不能当成游戏内取枪已完成。

取枪动画边界复核：Astra Max 仅用已保存的 UnrealScript 和现有 VM 观察文档确认，原版 `SetCurrentWeapon`／`ChangedWeapon`／`WeaponPuttingDown`／`Active` 负责复制、挂接及计时，不能跳过；身体取放枪姿态分别在 `TimeWeaponPutDown/TimeWeaponEquipping` 内调用。姿态函数还被手雷重装与其他动作复用；现有 CallFunction 入口的参数尚由 caller frame 在函数内求值，直接吞掉调用会错过参数消费。未新增危险的全局动画抑制或同步完成装备 Hook，下一步先做原游戏 caller／callee 边界与参数消费的只读验证。

库存只读候选增量：新增默认关闭的 `GEARS_PLAYGROUND_EQUIPMENT_OBSERVE`，只在本地 `WarPC.PlayerTick` 游戏线程上每 500 ms 调原版 `GetWeaponInSlot`、`CanSwitchWeapons` 与手雷 `HasAnyAmmo`，并核验 `WarPC`／`WarPawn`／`WarInventoryManager`、`CharacterSlot`、当前枪和 `bSwitchingWeapons`。手雷数量取 `SpareAmmoCount`，按原版人类玩家 `HasAnyAmmo` 判定是否可取；若原版无限备用弹药允许使用，则标记为无上限。之前考虑的 `GetTotalAmmoInGun` 包含当前弹仓，不适合手雷选择门槛，已弃用。查询或类型不符即丢弃快照；`WeaponEquipmentModel` 对未知手雷数量改为按空槽处理。独立状态单测、私有候选 DLL 和默认关闭的 DLL 均完成 MSVC Release 构建。尚未拿到实际购买版运行时快照，且只读候选不执行取枪、收枪、黄色提示或音效，默认启动器未改。

库存观察运行尝试：独立购买版测试子进程 PID `31700` 按置后、静音、禁后台物理手柄输入策略启动，私有候选 DLL `b607bb7e…16d35b` 注入后 `PlaygroundInitialize` 成功，但 120 秒内 `PlaygroundEnable` 一直找不到菜单视口，未进入场景，日志也没有有效 `equipment_snapshot`。`PlaygroundShutdown=0`、测试游戏退出、音频守护恢复，回执在忽略的 `analysis/weapon-equipment-snapshot/owned-probe-receipt.json`。这与先前 Gnasher 探针的菜单未就绪同类，不能推断库存查询成功或失败；在拿到启动原因／不同的启动路径之前不再按同样条件重复。默认 DLL、启动器及用户配置均未更换。

Gnasher 首发杠杆门槛补齐：`ReloadModel::prime` 可用同一原版库存快照的当前弹仓／备用数量，先把已经装备且装弹的枪纳入状态机；第一次射击后即要求右手握把下掰回位，未完成时 `can_fire` 为假。重复初始化不能清除待操作杠杆，切枪／换世界必须由适配器显式重置。完美窗口终点尚未从原版取得时，`prime` 仍能启用射后杠杆，但 `begin` 继续拒绝未知期限，避免假造完美换弹时间。`WeaponReloadModelTest` 与 `GnasherLeverTest` 重新编译运行通过。此逻辑尚未接到原版射击事件或游戏内禁射／骨骼驱动，不等于实枪已要求每发手动循环。

库存观察启动诊断：第二轮改用此前成功测试所用的捆绑 Python 3.12，仍在 120 秒内 277 次收到 `PlaygroundEnable=2`，未创建菜单视口；候选正常关闭、测试进程退出、音频守护恢复。第三轮改用完整私有 VR 启动器并在私有清单关闭默认 Playground，原生预留器 240 次轮询后明确记录缺少 `d3d12.dll`／`dxgi.dll`，`injection_attempted=false`，故候选甚至未到加载阶段。其关闭消息未让测试子进程退出；按启动记录 PID／创建时间／路径核对后只终止该测试 PID `12912`，确认已消失。两轮原始回执都在忽略的 `analysis/weapon-equipment-snapshot/` 与对应 `J:\gearsVr\analysis\instances\equip-snapshot-1022b/1022c/`；不能据此判断库存反射正确与否。需要先定位当前购买版启动停在图形模块之前的原因，或使用已证明可进入场景的不同运行条件；正式启动器及用户配置未改。

手雷抛掷数学原型：新增 `GrenadeThrowTracker`，用同一 XR 会话、空间／输入纪元及冻结回中参考下的最近 140 ms 手部轨迹作最小二乘初速度估计；短于 55 ms、静止、追踪失效、回中改变、跨世界／武器或瞬间跳位都不会产生投掷。出手后给出 3 kg 对应冲量、9.81 m/s² 抛物线及经同一参考帧转换的游戏世界位置／速度；质量不被错误地当作减速系数。握点到手雷质心的局部偏移现可逐帧随手旋转，速度拟合包含手腕甩动；世界速度还可叠加原版角色平移速度。`GrenadeThrowTest` 以 `/W4 /WX` 编译并通过轨迹、手腕旋转、角色速度、冲量、世界坐标及断续输入测试。实际各手雷握点偏移／角色速度来源仍待游戏适配器量测接入；它尚未替换原版投掷曲线、生成／扣除手雷、处理碰撞引信或加载手持模型，也没有真机手感验收。

启动记录只读对照：前次成功进入战斗场景的 `vr-fire-bridge-1022f` 与本次 `equip-snapshot-1022b` 使用同一购买版工作副本、同样的置后／静音启动策略、相同的 `-windowed -ResX=1280 -ResY=720 -NoMovieStartup -NoPauseUnfocus` 参数及相同私有配置内容；当前没有证据将菜单未就绪归因于这些参数。第三轮管理式启动 `1022c` 只带 `-NoPauseUnfocus`，是一个可解释的条件差异，但前两轮同参结果不同，仍须另找启动状态或时序证据；本轮未再次启动游戏。

入口返回码勘误：源码中 `PlaygroundEnable=2` 既可能是 `WarGameViewportClient` 未找到，也可能是 `PlayerController.PlayerTick` 函数未找到。此前将 `1022a/1022b` 的返回码直接写成“未创建菜单视口”超出了现有回执，应改读为**测试入口未就绪，原因未区分**。这不影响“没有取得有效库存或骨骼运行样本”的结论；下一次在确有不同启动条件时，应分别记录这两个寻址结果，再决定是否进一步诊断游戏启动。

私有库存入口细分验证：仅在 `GEARS_PLAYGROUND_EQUIPMENT_OBSERVE` 候选中新增只读 `PlaygroundEquipmentReadiness` 位码，分别观察 Hook、视口、`PlayerController` 类、`PlayerTick` 和 `WarLocalPlayer`；默认关闭的 DLL 不导出它。候选与默认 DLL 均重新编译，导出隔离核对通过。一次不同条件的购买版置后静音测试 `equip-snapshot-1022d` 将候选注入延后 20 秒，120 秒观察后仍仅得位码 `1`（Hook 已启用，其余四项均未出现）；`PlaygroundEnable=2` 计 278 次，没有有效库存快照。回执在忽略的 `analysis/weapon-equipment-snapshot/equip-snapshot-1022d-receipt.json`；`PlaygroundShutdown=0`，测试 PID `52392` 已退出，音频守护已恢复。延后注入未解决本次初始化未就绪，不能归咎于库存查询，也不能据此否定过去成功进入场景的独立样本。相同条件不再重跑；下次须先找到游戏初始化条件变化的证据。

库存切换私有候选：核对已保存的 `WarInventoryManager.SetWeaponFromSlot` 脚本，确认原版槽位选择由它处理无弹药反馈和 `SetCurrentWeapon`，而非直接改当前枪指针。新增默认关闭、必须同时启用库存观察的 `GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST`：外部只提交 0–3 号槽；游戏线程重新读取有效快照，仅限已创建的私有 Playground 场，才调用原版 `SetWeaponFromSlot`，并观察装备完成、身份中断或 5 秒超时。测试子进程还须带置后和静音标记。私有测试 DLL 和默认关闭版均编译通过，导出表证明测试入口仅在前者，`WeaponEquipmentModelTest` 通过。当前购买版启动尚未达到游戏线程库存边界，**此候选没有游戏内调用证据**；不接入默认启动器，不视为可玩的取枪实现。

右手抓取到原版槽位的私有接线：原生已认证游戏 Tick 中的 XR 输入路由现在可把右肩／左肩／左右裤兜四种抓握意图，经新的进程绑定、8 事件环形桥发布给 Playground。桥按序号消费并拒绝溢出、重放、超过 250 ms 的事件；Playground 在原版游戏线程再次比对 PC、Pawn、当前武器及四槽库存，仅在置后静音的私有场里调用已有 `SetWeaponFromSlot` 候选。`EquipmentIntentBridgeTest`、启用与关闭候选的 Playground 构建、启用与关闭认证 Tick 的 native 构建均通过；默认 Playground 导出表仍无测试入口。当前未取得购买版游戏中的有效库存 Tick，不把此接线视为真实 RGrip 切枪验收。原版切换动画、收枪显隐、空槽提示音和持枪 6DoF 姿态尚未完成；原版脚本的无弹药反馈只覆盖有武器对象的槽。

空槽音效私有候选再利用原枪音源：库存模型判空时，在同一有效游戏线程快照里优先取目标槽武器、其次取当前武器的 `FireNoAmmoSound`，用原版 `WeaponPlaySoundLocal` 播放；没有有效 `SoundCue` 则回报状态 9，不伪称已发声。候选编译通过，默认编译排除这条路径；由于购买版本轮未到有效游戏 Tick，**没有音频播放实测**。左肩无备用弹药以及完全无当前武器的通用提示音仍待接入。

同一库存快照现在逐个已占槽调用原版 `HasAnyAmmo`。枪对象仍在但已无任何弹药时立即判无货，不再进入无法完成的五秒切枪等待；先判断“把已持枪放回原位”，所以弹药见底仍可收枪。私有 Playground 候选构建、纯状态机测试均通过，尚无运行时库存查询／声音验收。

左肩取弹意图已接入同一认证事件桥。私有适配器在有效游戏线程快照里核对当前枪、角色和测试场，按原版 `SpareAmmoCount` 与 `HasInfiniteSpareAmmo` 给出可供给／无弹药反馈；无弹药时尝试原版空仓 Cue。状态接口区分可供给、Cue 已调用、无 Cue 和验证失败，**有弹药时仍未生成或扣除实体弹／弹匣**。更新后的事件桥测试、两侧私有候选编译及默认 Playground 导出隔离通过，购买版真实手柄与声音仍待验证。

双手连续世界姿态接线：native 从同一已认证 XR 双眼 ticket 取左右手抓握姿态，经冻结的回中参考和实际用于双眼生成的游戏相机视图转换到游戏世界，在 GVE3 进程绑定桥的独立最新姿态槽发布；游戏线程私有 Playground 重验 PC／Pawn／当前武器及测试场身份，拒绝过期姿态并限速记录。新增 `xr_pose_in_game_world` 的平移、水平转向、俯仰／侧倾与无效矩阵测试；`WeaponPoseSolverTest`、`EquipmentIntentBridgeTest`、native／Playground 候选、默认 Playground 构建及导出隔离均通过。未运行新的购买版游戏测试，枪模、枪口射线和动画还未受这条姿态控制；此前私有测试没有进入库存／视口边界，本轮静态通过不改变该结论。

原枪组件只读探针：私有 Playground 在同一身份已验明的手部姿态旁每 500 ms 记录 `Weapon.Mesh` 的组件／资源名、相对变换、`LocalToWorld` 原点和绝对变换／显隐标志，供下一次有效购买版场景确定枪模控制入口。候选编译通过；未取得游戏读数，也未移动枪模或替换模型资源。

Lancer 原枪模跟手私有候选：`WeaponMeshFollow` 从有效原 `LocalToWorld` 与右手世界姿态锁定一个首帧不跳位的临时偏移；仅在测试场已持有 Lancer 且 XR 姿态、PC／Pawn／武器／世界身份持续有效时，于原 `PlayerTick` 后调用游戏已有组件变换函数。候选保存原相对变换和绝对标志，断追、切枪、收枪和身份变化时尝试恢复，私有 Hook 关闭前必须已恢复。`WeaponMeshFollowTest` 通过平移、转动和不合格矩阵测试；启用、关闭及默认 Playground 构建通过，默认导出无该入口。没有新的有效购买版游戏运行样本，因此**不能称为已经可玩或后坐力已保留**；临时偏移仍须替换为逐枪握点校准。

双手输入接线增量：GVE4 私有桥在同一已认证 XR ticket 中加入 `input_epoch` 与两手 `grip_down_mask`；只给有效追踪的手设置按住位，失焦／配置换代后的基线不产生新的抓握。游戏线程日志可区分左手姿态与实际握持，Lancer 跟手候选在输入代次改变时恢复原组件。`EquipmentIntentBridgeTest` 覆盖左右手有效位、无效组合和换代数据，私有 native／Playground 与默认 Playground 构建通过。副握点尚无原版逐枪标定，双手持枪纠偏仍未启用；本轮没有新的购买版运行验收。

Lancer 副握点取证：已有 `AR_Fire` 枪械序列没有人物手轨，已盘点武器 socket 只有 `Muzzle`／`Magazine`；人物动画树的左手 IK 则指向 `b_MF_IK_Hand_L`。这些静态线索不足以给出副握点或吸附半径。私有 Playground 候选新增仅在显式请求后执行的游戏线程采样：原版 `PlayerTick` 返回后、枪模候选移动前，以同一世界／角色／原枪身份读取三根人物骨骼和前后不变的人物／枪组件矩阵，最多 24 帧；`lancer_support_probe.py` 读取日志并要求 24 行连续完整才签发回执。无写入原版骨骼／武器，输出仍在忽略的 `analysis/`。候选和默认 Playground 构建、两项日志解析测试通过；购买版测试尚未取得样本，渲染线程同步、握点与吸附半径仍未验证。

Lancer 弹匣 socket 同帧零售版验证：私有只读游戏线程探针确认原版 `Magazine` socket 位于 `b_AR_Magazine`，待机 16 帧的枪体局部中位点为 `(0.007997, 28.145977, -6.484180)` 原始单位，最大漂移 `0.000182`；与静态参考枢轴加原版 socket 偏移的预测点相差 `0.000078`。另一局原版换弹 169 帧中，socket 开始后约 `0.271 s` 跳离待机点，约 `0.738 s` 时远离超过 `120` 原始单位，结束后回位。因此 VR 弹匣井提示／插入判定不能跟随换弹中的活动 socket，需以枪体坐标冻结参考挂点。`tools/playground/vr_lancer_socket_probe.inl`、采集器与离线分析器可复核，原始回执仅在忽略的 `analysis/lancer-support-probe-20261022/`；两次测试的私有 Hook、游戏子进程和静音均已收尾。私有／默认 Playground 构建、3 项解析测试、离线摘要复算与差异检查通过。当前仍没有确认插入轴、手柄抓点或真实黄色渲染，正式默认 DLL 未加入该探针或实体换弹。

Lancer 弹匣挂点纠正：先前待机稳定的 `Magazine` socket 实际靠近弹匣下缘，不能当弹匣井中心。新购买版私有测试将 socket、`b_AR_Magazine` 骨矩阵与枪组件矩阵在同帧采集 16 次；骨枢轴局部中位点 `(0.007966, 28.146028, 4.396064)`、最大漂移 `0.000230`，距静态参考枢轴 `0.000026` 原始单位。枢轴到 socket 为 `10.880222` 单位、方向约局部 `−Z`；原模型弹匣 Z 范围 `[-6.950642, 4.325475]`，支持枢轴靠近上缘、socket 靠近下缘。插入提示应从枪体上缘附近的固定挂点开始标定，不能把下缘 socket 当目标。采样与复算回执留在忽略的 `analysis/lancer-support-probe-20261022/`；私有 Hook 停机码 0、入口还原、静音恢复、该局零售游戏子进程退出均已核对。手持弹匣相对左手的朝向、实际口面及容差仍未验证，未启用实体换弹或正式版黄色引导。

固定挂点接入私有 Lancer R3 保持适配器：新增枪体局部上缘参考点和外向轴到原枪组件世界坐标的投影，非法／镜像／剪切矩阵会被拒绝；每次保持期间重验 `COGAssaultRifle` 及同一网格身份。纯数学测试与私有、默认 Playground 构建通过。购买版测试场记录 7 个同一网格挂点姿态，原版换弹开始状态 1、明确完成状态 6，库存由原版一次性结算；私有回执 `analysis/lancer-support-probe-20261022/capture-49940-fixed-feed.json` 已通过有界解析。游戏、Hook 和静音均正常收尾，整局 VR 回执 `incomplete`，不能视为头显验收。尚未取得左手手持弹匣姿态、玩家插入行程或黄色渲染；正式版没有启用实体换弹。

独立弹匣的原版左手姿态取证：购买版私有换弹观察器在 `Assault_Rifle_Magazine` 实际挂到人物骨架期间，同帧读取它与 `b_MF_Hand_L`、枪体矩阵，得到 43 帧。连续 21 帧的中段近似刚性握持位于换弹后约 `0.889–1.206 s`，弹匣原点相对左手骨骼中位点 `(19.036035, -5.566177, 0.139646)` 原始单位，最大漂移 `0.000227`；方向矩阵漂移 `0.0000019`。整段挂接前后仍有明显相对动作，最大位置偏差约 `8.147` 单位，不能将原版送回动画作为玩家插入判定。`tools/weapon-models/analyze_lancer_magazine_grip_runtime.py` 和合成测试可复算稳定区及全段差异；原始矩阵和摘要只在忽略的 `analysis/lancer-support-probe-20261022/`。本次私有 Hook 停机码 0、入口还原、静音恢复，零售子进程退出；整局 VR 回执仍 `incomplete`。手柄掌心偏移、实体弹匣随手显示与玩家插入行程待实现。

无弹取枪规则校正：静态脚本证实 `WarInventoryManager.SetWeaponFromSlot` 仅在 `HasAnyAmmo || bCanSelectWithoutAmmo` 时请求原版切枪。私有游戏线程库存快照现读取逐槽 `bCanSelectWithoutAmmo`，纯取枪状态机在无弹但原版允许时仍请求切出，反之给空槽反馈；已持枪收回原槽仍不受无弹限制。增加合成库存分支测试及日志位，实际游戏内字段读取和切换待测。

对既存测试启动失败再做只读归因界限：`equip-snapshot-1022c` 的完整启动器回执显示购买版游戏进程已建立，但在有界等待后仍未加载 `d3d12.dll`／`dxgi.dll`，故启动器在注入前退出；它不是库存观察器、网格跟手候选或过期清单的运行失败。此前直接游戏启动样本 `1022d` 只证明 Hook 就绪而未找到视口／`PlayerTick`。本轮没有再次启动游戏；下一次测试须先确认启动条件有实质变化，并分别记录图形依赖、视口和游戏线程状态。

## 当前显示边界与端到端验收证据核对（2026-10-22）

核对最新默认集成的源文件／native／XR／配置摘要一致，WorldProfile1 的 504 份冻结来源及 MountedLatency1 XR 的 47 份冻结来源摘要均匹配；默认构建中的 WorldHandoffDisplayTest、WorldHandoffIdentityTest 两项无窗口 CPU 策略检查退出 0。显示源码明确在世界 quiescing／generation 改变时使旧图像失效，新投影继续要求当前世界票据与资源证明；native 提交继续要求旧状态退休、GPU bridge 与 owned UI 回执。此核对不构成 GPU 或头显新增验收。

现有最近五份世界恢复日志均为空，且没有任何 `.world.jsonl` 晚于当前默认生产 Python 源文件。默认 native 目录留下的最后状态为 stopped、generation 1、Prepare／Commit 均 0；更早非空记录是旧版拒绝事件，不能用于评价当前全部修复。现有证据因此无法确定剩余真实机枪等待处于 CPU 准备、资源退休还是首个新世界 XR 投影，也不能证明“无延迟”或最新版本机枪已不再退出 VR。普通启动器原有 `.world.jsonl`、`.stdout.txt`、native／session 状态记录足以供用户后续统一验收时关联这些阶段，不需要新的实验入口。本轮只记录证据边界，保留已集成修复，未启动游戏或模拟器；目标保持未完成。私有核对回执为 ignored `analysis/mounted-end-to-end-audit-20261022/audit-1.json`。

随后两次重新核对均确认默认源码／二进制／用户配置与最新集成回执一致，仍无当前版本的新世界恢复记录；同一端到端证据缺口已连续三轮确认，达到受阻判定边界。累计修复及最近 681 项检查通过的默认版本保持，进一步定位真实剩余延迟依赖用户安排的机枪切换与头显验收证据。未启动游戏／模拟器，也未把离线结果记为无延迟完成。后续回执为同目录 `audit-2.json`、`audit-3.json`。

## 减少 UI 对象表传入 native 发现时的整数展开与复制（2026-10-22）

UI 对象表保留原双次稳定读取；当已按 manifest SHA 验证并成功取得 native 类发现导出时，helper 可提供同一份完整、不可变 bytes 的小端 Q 只读视图。native 适配器把该视图直接复制到原有的自有输入 array，省去整表先展开为 Python 整数 tuple、再逐项构造 array 的步骤。runtime 的新 reader 准备路径显式绑定该可选能力。未提供能力、导出不可用、非 bytes／长度／计数／主机字节序不符时仍走原计数解包；可写／非连续／其他格式视图等外来输入保留原适配器转换规则。复制后的输入 array、输出 array 与回调仍共同存活至同步 native 调用结束，没有把游戏地址直接作为受信证明传入，也没有跨轮缓存对象表。

保存旧 UI 与 native Python 适配器，用冻结 WorldProfile1 DLL 的真实类发现导出及完整 UI builder，对高密度重复非类实例地址的合成表作六组交替对照。10 万／40 万／100 万项的完整 UI 中位分别为 32.06085→30.5911、46.9531→40.3981、82.5454→57.88445 ms；配置、UTF-16 输出与全部读取跨度相同。40 万项机枪 UI 与 Python 类发现回退路径的配置／输出／读取跨度也一致；回退只作等价核对，不据单次计时宣称性能收益。百万项单独三组 tracemalloc 的 Python 临时分配峰值中位 52,054,633→24,584,502 字节，不含既有夹具、C++／OS 分配。上述仅为合成 UI 组件，未测量完整恢复或实际游戏／头显黑屏时长。

681 项 Python 检查通过（482 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖不可变来源／无符号值／顺序／重复项、回调中 GC 与输入生命周期、外来视图回退、注册表变化及取消、普通／批量／实际 native 的完整 UI 输出与读取等价、双读表变化／长短表、选中类结束变化、native 不可用回退，以及 runtime 实际绑定后确实调用视图入口。三份生产模块已接入默认启动器，83 项打包文件核对通过；Resolver 本体恢复并核对为本轮前原内容。此前全部修复、native／XR／Playground、清单及用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-ui-pointer-view-20261022/`。

## 减少每次 UI 获取的对象表格式编译开销（2026-10-22）

补齐 UI builder 中仍逐项展开 `Q` 的对象表解包，将格式改为等价计数形式，沿用小端 64 位无符号值、完整 tuple、对象顺序／重复项、双次稳定读取及全部选择／身份／结束证明。与此前 Resolver 改动相同，此处没有缓存对象表或跳过验证，仅消除随对象数增长的格式字符串及编译格式对象。

保存旧 UI 源码作完整 UI 获取对照，10 万／40 万／100 万项合成对象表的四组交替冷格式中位耗时分别为 16.65915→12.85735、33.30585→19.85065、69.093→39.41625 ms；热格式分别为 13.725→13.248、20.8243→20.80535、43.4254→40.42135 ms。完整配置、18,504 字节 UTF-16 输出与全部 1,061 次读取跨度一致；40 万项机枪 UI 的完整配置／输出／读取跨度也一致。百万项编译格式对象的 __sizeof__ 为 32,000,088→120 字节，指该格式对象本身，不包括对象表与返回 tuple。额外对照空项／高位值／非类重复项、重复 owner 类、短表及读取失败，接受／拒绝结果与读取顺序保持。数值来自合成内存组件，未测量实际游戏／GPU／头显时长。

672 项 Python 检查通过（473 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。一处生产代码变更已接入默认启动器的 UI 依赖，83 项打包文件核对通过；已有恢复修复、native／XR／Playground、清单及用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-ui-pointer-decode-20261022/`。

## 首次恢复／观察复用启动相机的发现位置（2026-10-22）

启动 Anchor 记录已有相机／Controller／本地玩家身份、最终 guard 与 resolver，之前只有第一次观察或准备完成后才保留这些发现位置，第一次获取仍执行全对象表相机发现。现从可选启动元数据提取提示，先核对进程／EXE／模块、三项完整角色及 name index、配置字段与 camera cache／POV／timestamp 地址关系和固定 POV 布局；缺失或不一致则沿用原全局发现。提示保留独立副本，不含接受权限；使用时仍由新 Resolver 完整准备注册表／EXE 门禁，实时复查三个身份并重新构建全部证书。实际旧启动记录的格式核对通过，过程中未打开游戏进程。

提示失效时以确切 DiscoveryHintChanged 拒绝当前 reader，关闭后立即从新 reader 做完整发现，避免新增固定 50 ms 等待。三次尝试上限、共享截止时间和所有证书／代码验证保持；其他注册表变化、未知子类与同消息普通错误沿用原等待／拒绝规则。首次挂上机枪的真实 build_candidate／Anchor／UI／准星暂停证书及原子发布路径，保存旧 runtime 对照：全局 resolve 调用 1→0，仍有一次新注册表准备、两次证明 reverify 和各一次 reader／证明上下文退出；发布的完整 UTF-16 候选字节及 SHA256 一致。

对测试 Python 自身分配的对象头／对象表做真实 ReadProcessMemory 组件对照，执行原 Resolver.resolve 的实例扫描，固定类／属性发现、EXE 哈希和注册表装载以隔离该步骤。六组交替样本：10,000 个无关合成对象时中位 19.9971→0.1084 ms，100,000 个时 214.614→0.1024 ms；本进程 RPM 分别 10,000／100,000→3，所选相机操作字段一致。新路径三次 RPM 是附加角色槽验证，其他证明读取仍在合成内存中执行。这些值仅为被省去的发现组件，不是完整恢复、实际游戏对象数或头显黑屏时长。

672 项 Python 检查通过（473 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖构造不打开进程、首次完整机枪候选发布、实时挂载观察、三个角色的身份变化、21 类可选提示错误／回退、独立副本、结束 guard 变化、失效提示关闭 reader 后完整重建，以及立即回退的确切类型／尝试／截止时间约束。三份生产模块已接入默认启动链路，83 项打包文件核对通过。此前全部恢复修复、native／XR／Playground、清单及用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-startup-camera-hint-20261022/`。

## 观察与完整准备之间发生状态变化时继续恢复（2026-10-22）

补上双阶段恢复的一处分类遗漏：旧观察已经取得完整 WorldSnapshot，随后完整候选证明成功，但实际状态已改变时，原代码按通用 RuntimeError 累计普通失败，进入 2 秒退避并可耗尽同目标准备次数。现仅在旧观察确为 WorldSnapshot、两次完整本地玩家身份完全相同且候选已经通过原完整构建时，发出确切 WorldObservationChanged 与 acquisition_churn 标记；当前候选仍被拒绝，不发布、不更新发现提示，下一轮重新观察并准备全部证书。缺失／错误观察、本地玩家身份不符、同消息或同类型错误来自证明／构建／发布其他阶段，均不获得此重试分类。单次获取路径继续要求原快照谓词明确返回 True。

保存旧 runtime 源码，用真实 Anchor／UI 构建并冻结的两份机枪候选、合成 owner 时钟／证明与 native 回执作三组回放。模型中的状态在 20 ms 后稳定时，重试计划等待由 2,000→50 ms；连续五次观察／准备状态不同时，旧版三次普通失败后不再调用准备，新版第六次获取到匹配状态后提交。先发生一次真实硬失败的模型同样可在五次状态差异后恢复，并始终保留该硬失败次数直到成功提交。每组成功均只发布最终匹配的一份候选、Prepare／Commit 各一次，所有证明上下文均退出。此为代码回放，未测量实际游戏／GPU／头显时长。

663 项 Python 检查通过（464 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖完整候选拒绝后内容保持、匹配候选原样发布、缺失／外来观察、其他阶段同消息／确切类型错误、连续变化恢复、先前硬失败预算与单次获取谓词。更新已接入默认启动器的 runtime 依赖，83 项打包文件核对通过。前轮全部恢复修复、native／XR／Playground、清单及用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-observation-race-20261022/`。

## 只读短重试后追加一次较早的完整获取（2026-10-22）

原有 50／100／150 ms 短重试耗尽后，再从最后一次失败等待完整 2 秒，默认下一次获取因此约在 2.3 秒。现保留全部既有短重试；若它们在首次失败的普通退避截止时间前结束，最多追加一次该较早时点的完整获取，同时保留原有 2.3 秒及随后普通尝试时点，避免状态恰在 2 秒后稳定反而错过原尝试。已在较早截止时间之后结束的慢获取或小 backoff 不追加追赶探测。适用于已认证的 timestamp_sample／acquisition_churn，只变更等待计划；每次仍重新取得完整证书，真实失败预算、进程／session／candidate／发布／native 回执要求保持。成功完整获取或提交会清空本轮时间状态，真实错误会丢弃旧截止时间且保留已用短重试预算。owner 等待也遵守短重试之后的截止时间与原 10 ms 最小休眠。

保存旧 owner 源码，用真实 Anchor 构造／配置校验、合成反射内存与时钟／native 回执作六组对照：链接在 310／1,990 ms 后有效时，模型恢复由 2,300→2,000 ms；20 ms 后有效仍为 50 ms，2,010 ms 后有效仍为 2,300 ms，更长不可用保留原 4,300／6,300 ms 尝试。每组最多多一次只读获取、仅发布一份候选并各接受一次 Prepare／Commit。模型未包含真实获取耗时、GPU 或头显时长，不能据此认定实际黑屏减少 300 ms。

656 项 Python 检查通过（457 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖两种暂态范围、尝试上限 1／3／10 与 10／200／2,000 ms backoff、慢工作线程、实际等待尾段／提前 hint、较早探测后的原时点、真实失败预算、成功重置以及 foreign candidate／session／进程／停止／关闭限制。更新已接入默认启动器的 monitor 依赖，83 项打包文件核对通过。前轮空 Pawn 链接及其他已集成恢复修复保持，native／XR／Playground、清单与用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-sampling-retry-deadline-20261022/`。

## Controller 初始未占有时继续完整恢复（2026-10-22）

补上 Anchor 构造早期 Controller.Pawn 暂时为零的分类缺口。原路径把该情况包装为通用空对象错误，进入 2 秒普通退避并累计真实失败次数。现在仅在此链接为空时，重新确认对象／名称表边界、Controller 身份、Pawn 属性身份／类型／所属类链／偏移／ArrayDim／ElementSize，双读链接并复查所有来源；仍为空则发出确切 AnchorControlledPawnUnavailable，变为有效 WarPawn／Turret 则发出确切 AnchorControlledLinkChanged。两者均拒绝当前获取，要求重新完整获取全部证书，再进入已有有限短重试与普通退避。其他空指针、错误身份／类／布局／不可读内存以及未知子类保持原拒绝规则；正常非空路径没有增加读取。

保存旧源码，用真实 Anchor 采样器、合成反射内存及 owner 时钟／native 回执作两组回放：模型中的链接在 20 ms 后可用时，计划重试等待由 2,000→50 ms；连续五次初始未占有后，旧版三次普通失败耗尽而不再提交，新版第六次完整获取后完成模型提交，仅发布一份候选、Prepare／Commit 各一次。正常路径完整 config／INI、最终身份 guard 及全部读取跨度与旧行为一致。上述为软件回放，未测量实际游戏／GPU／头显黑屏时长。

647 项 Python 检查通过（448 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖稳定空链接、零到有效机枪链接、所有来源与属性布局变化、注册表变化、消息／未知子类拒绝、真实 owner 恢复提交和三次内部失败均释放 reader 且不激活。三份生产模块已接入默认启动链路；83 项打包文件核对通过。此前恢复修复、WorldProfile1 native、XR／Playground、清单及用户配置保持。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-anchor-unpossessed-20261022/`。

## 已知获取暂态耗尽后继续完整恢复（2026-10-22）

补上只读获取层的错误分类遗漏：内部已按暂态处理的 Anchor／UI／crosshair／discovery 注册表变化、确切类型的未占有／controlled-link 变化，以及确切 UIConfigRejected 的 changed-read，在三次内部获取耗尽后，原先会被 live owner 当作普通失败累计；同一目标三次后可停止准备。现共享原有窄分类，适配层只在未发布的只读获取边界标记 acquisition_churn，进入已有有限 50／100／150 ms 短重试及普通退避，每次重新完整获取。发现／相机提示／采样阶段同样保留该分类，所有选中身份、代码、进程、候选及 native 提交证明保持。

原始异常类型与原因保留，日志将 acquisition_churn 与既有 timestamp_sample 区分；时间戳规则不变。消息相同但类型不符、未知 controlled-link／UI 未占有子类、真实身份／代码／未知内存错误、超期成功及发布阶段异常保持原处理，暂态也不清除先前真正的失败次数。启动前的内部尝试与共享截止时间沿用原规则，仅提取复用分类函数。

七类保存旧源码回放均复现：旧版在三次失败后停止调用准备；新版连续五次暂态拒绝后仍发起第六次完整获取并完成模型提交，只发布一份候选、发送 Prepare／Commit 各一次。该回放使用合成时钟与获取后端，不证明实际 GPU／头显时长。另有真实 adapter→build_candidate→acquire_observer_preparation 的失败用例，核对三次内部获取均关闭各自 reader，退出证明上下文且不发布、不激活；随后出现的真实身份错误与晚完成结果仍拒绝。

638 项 Python 检查通过（439 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖分类正反例、观察边界、完整失败链、发布边界、先前硬失败次数和恢复后提交；当前检查保留所有既有控制通知／时间戳／worker 唤醒等修复。三份更新模块已接入正常启动器依赖；WorldProfile1 native、XR／Playground、清单及用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-acquisition-churn-20261022/`。

## 缩短未接收控制通知后的恢复等待（2026-10-22）

修复 Prepare／Commit 控制事件明确未接受（OpenEvent 返回失败或 SetEvent 抛出 OSError）后，首次仍等待 2 秒才重试的路径。默认最多三次名义 50／100／150 ms 短重试，继续复用同一份已发布候选、请求编号和路径；每次仍经过进程、确切 session、native 请求／候选／generation 检查。已接收通知继续等待确切回执，超时不触发盲目重发；未知控制结果仍停止该事务。短重试结束后衔接原普通退避的时间点与后续节奏，额外探测不把该时间点向后推；等待函数遵守发送重试截止时间，并保留 10 ms 下限。

同时修复 Prepare 已超时但后来收到有效回执、紧接着 Commit 通知未接受时，旧 timed_out 标记继续抑制重试截止时间的情况。未接受的新阶段会清掉上阶段超时标记，成功发送后再建立该阶段的正常回执期限。普通退避有独立计数，覆盖尝试上限 1／3／10、10 ms／2 s 退避参数，短探测不会提前耗尽原普通节奏。日志补充控制通知重试的范围、间隔和短重试标记。

保存旧源码，用真实 owner 类、已等待结束的工作 Future、合成时钟／状态和事件后端作九组回放。后端在首试后 20 ms 可用的模型中，Prepare／Commit 的 False 和 OSError 情况，控制送达计划等待由 2,000→50 ms；310 ms／3 s 持续不可用场景仍在原 2,000／6,000 ms 时点送达。迟到 Prepare 回执后的 Commit 失败同样由 2,000→50 ms。每组均只生成一份候选，Prepare／Commit 各被接受一次并完成模型提交。这是软件调度模型，2 ms native 回执与 250 ms 普通 owner 间隔为模型输入，未测量实际游戏／GPU／头显切换时间。

628 项 Python 检查通过（429 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖两个阶段的失败类型、真实等待函数截止时间、截止前不发送、普通退避衔接、迟到回执、未知错误，以及 owner／game／session／停止／foreign request／candidate／generation／禁用／关闭约束。更新已接入正常启动器的 monitor 依赖；全部既有修复、WorldProfile1 native、XR／Playground、清单及用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据与测试日志在 ignored `analysis/mounted-control-signal-retry-20261022/`。

## 减少恢复候选序列化中的诊断树复制（2026-10-22）

serialize_candidate 原来为重新验证配置及来源关系，递归复制整个 Anchor／UI 元数据。现保留全部顶层要求，只深复制配置、三项最终来源身份、resolver 及可选的准星配置／机枪暂停证书；其他诊断图保留在完整原候选中，不再重复遍历和分配。非 Mapping 元数据继续拒绝。所有来源身份、PID／SHA／module、相机字段关系、强制准星要求和原有规范 INI 重新生成校验保持，发布器的独占路径、fsync 和原子替换不变。

使用真实 build_candidate／Anchor／UI 构建函数、合成的机枪与分流式 Level 内存和模拟 attestation，构造 sample_pose=False 且启用准星观察的完整冻结候选。保存旧源码作 40 组交替序列化对照：中位 2.54515→0.8631 ms；单独五组 tracemalloc 测量的临时分配峰值中位 358,599→91,039 字节，不包含既有候选存储。两版 36,498 字节 UTF-16 数据及哈希完全一致，原 Anchor 123,458 字节／UI 11,290 字节 JSON 等价元数据保持；另有 43 项来源身份、resolver、权限标记和机枪暂停证书篡改，两版均拒绝。上述只测量序列化组件，不代表游戏／头显或完整恢复时长。

622 项 Python 检查通过（423 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖不遍历诊断图且原记录仍保留、非 Mapping 输入、来源标记与强制准星要求、真实完整机枪候选序列化和暂停／最终来源／controlled 关系篡改。更新已接入正常启动器的 world_rebind 依赖；此前修复、WorldProfile1 native、XR／Playground、清单及用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据在 ignored `analysis/mounted-serialization-copy-20261022/`。

## 合并密集反射字段的发现读取（2026-10-22）

在单轮字段发现范围内，同页字段头的前两项沿用原发现路径，后续字段可复用最多 64 页、256 KiB 的只读发现数据；失败／短页读取回退原精确读取，同页失败探测记录也有 64 项上限。链前缀 32 个字段仍未出现密集页时，本次遍历停止密度跟踪并沿用原路径。范围外、跨页字段头、分散字段均保留原路径，范围结束后不再使用页提示，已挂起的遍历器恢复时同样如此。选中字段的独立实时身份／类型／偏移证明和全部结束链接复查保持；已知 RegistryChanged 不被页读取失败的通用回退吞掉。

保存旧源码，以测试 Python 自身分配的合成内存作真实 Windows ReadProcessMemory 对照，没有打开游戏进程。2,000 字段、26 查询、256 字节间距的组件中，读取调用 5,805→4,242，最终 1,926 项链接复查、属性结果、全部链接与偏移读取一致；十二组交替样本中位 16.2421→15.06375 ms。合并扩大了读取字节数（215,912→549,864），以额外有界数据换取较少系统调用。2,048／4,096 字节间距的分散布局均保持 5,805 次读取和原字节数；其计时有波动，不能据此声称实际游戏性能无回归。

完整 UI 夹具提供与现有 native class-only 入口相同的类地址结果，隔离比较后续字段发现：读取 5,249→5,063，完整 config／序列化、52 个选中身份的 401 次头部读取及 337 项链接复查一致。617 项 Python 检查通过（418 项相关检查启用实际 WorldProfile1 DLL、57 项 package-inspector、142 项启动器／打包）。新增覆盖密集／分散／稀疏前缀、失败及短页回退、边界与容量、结束后恢复遍历、原始读取仍实时、实际使用页提示后选中身份变化仍拒绝，以及 RegistryChanged 不回退接受。

更新已接入正常启动器的 Resolver 依赖；此前所有优化、WorldProfile1 native、XR／Playground、清单和用户配置保持，83 项打包文件核对通过。未启动游戏或模拟器，上述为发现组件证据，完整恢复与头显时长仍由用户统一验收。证据在 ignored `analysis/mounted-field-pages-20261022/`。

## 减少恢复获取时的重复反射属性遍历（2026-10-22）

在已有单轮 discovery field hint 范围内，属性查询现在保留已走过的字段前缀，并按需从上一次位置继续；只对当前查询名称分类，保留同名字段顺序与子类优先规则。每次选中属性仍读取当前偏移，所有选中身份／类型／继承证明及结束时的 head／next 复查保持。范围外使用原遍历，达到容量上限退回原遍历；结束、最终读取错误均清空新提示。损坏迭代器会禁用本轮索引，避免其异常终止后被误当作“未找到属性”。没有跨恢复缓存字段或证明。

保存旧源码交替作组件对照。2,000 字段合成链的 26 次查询，中位耗时 16.72465→3.39965 ms，身份提示调用 25,753→1,978；3,852 次链接读取、26 次偏移读取、1,926 项结束复查及返回值一致。完整 UI 夹具增加 300 个未选中字段后，字段遍历 3,403→1,270，完整 config、UTF-16 序列化及全部 5,606 次内存读取跨度一致；此组件测量中位 56.3819→53.0038 ms。数值来自合成组件，不是完整恢复或游戏／头显黑屏时长。另用固定种子生成 100 个含同名／非属性／继承环／非法偏移的图，共 3,000 次查询，成功结果、错误和链接／偏移读取完全一致。

606 项 Python 检查通过（417 项相关检查启用实际 WorldProfile1 DLL，47 项 package-inspector 检查，142 项启动器／打包检查）。新增覆盖提前命中不读后续字段、同名非属性、子类优先、偏移实时读取及上限、容量回退、结束／下轮隔离、链变化、晚出现的环／错误 owner、最终读取错误和选中属性最终身份变化。更新的 Resolver 已接入正常启动器依赖；前轮所有优化、WorldProfile1 native、XR／Playground、清单及用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据在 ignored `analysis/mounted-property-hints-20261022/`。

## 减少恢复准备中的整份诊断记录复制（2026-10-22）

RuntimePreparation 原来为提取 config 和最终身份 guard，两次递归复制整份 anchor 元数据，包括不参与此处操作的诊断记录。现只复制 config，为构造发现提示直接读取已冻结的最终 guard，生成的提示仍是独立数据。完整候选、诊断记录、所有实时校验和发布流程保留；被接受的原始候选继续原样交给发布器，没有截短证据或复用旧证明。

保存旧源码，用真实机枪合成夹具生成并冻结的 181,510 字节元数据，交替执行两版真实 prepare_changed 的数据处理部分，各取 40 组：中位耗时 5.6429→0.578 ms；输出快照、回执、发现提示完全一致，每次均有一次候选获取、一次证明上下文、两次存活检查和一次原候选发布。候选生成与磁盘发布由夹具代替，这只是数据处理组件测量，未测量完整恢复或实际游戏／头显时长。

525 项 Python 检查通过（383 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查）。新增覆盖仅访问操作需要的元数据、接受／拒绝分支、原始候选及诊断保留、配置与提示的独立副本。两处生产代码变更已接入正常启动器的恢复依赖；WorldProfile1 native、XR／Playground、清单、用户配置与此前优化保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据保留在 ignored `analysis/mounted-metadata-copy-20261022/`。

## 减少注册表指针解包的格式开销（2026-10-22）

Resolver 准备名称／对象表时，将为每个指针展开一个 `Q` 的格式改为等价计数格式。两处代码变化，保持小端、64 位无符号值、顺序、重复项、输出 tuple、读取地址与长度以及所有前后校验。避免编译格式本身随指针数逐项膨胀；此项没有引入注册表数据缓存。

保存旧源码，用合成模块／文件／内存 API 驱动两版真实 Resolver.prepare。名称表 1,000,000、对象表 1,000,002 项时，四轮交替对照的冷准备中位耗时 68.69565→24.25715 ms，热准备 29.11965→26.33835 ms；两版输出元组及五次读取跨度完全一致。单个百万项编译格式对象的本运行时 __sizeof__ 为 32,000,088→120 字节，仅指格式对象，不是数组或整个进程内存。另有十万／四十万规模对照。这是合成注册表组件测量，未测量实际游戏计数、完整切换或头显时长。

523 项 Python 检查通过（381 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查）。新用例实际执行 Resolver.prepare，覆盖指针值／顺序／重复／高位、格式对象大小、读取跨度、注册表边界在大块读取前拒绝、短缓冲区、EXE 哈希及名称零项校验。更新后的模块已接入默认启动器依赖；WorldProfile1 native、XR／Playground、清单和用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机效果由用户统一验收。证据保留在 ignored `analysis/mounted-pointer-decode-20261022/`。

## 减少钩子证明中 DLL 读取的过量分配（2026-10-22）

证明边界原来每次 `read(64 MiB + 1)`，实际 WorldProfile1 DLL 只有 2,379,776 字节。现先从打开的文件描述符检查实际大小和原有 64 MiB 上限，按实际大小＋1 读取，并核对返回长度及读取后的大小；每个边界继续完整 SHA256、PE／detour 范围、模块／进程／owner、manifest 与目标／relay 双读。没有缓存哈希结果或改变任何输入授权。

保存旧源码，用真实构建的 DLL 副本和合成 RPM／身份作六组交替对照：一次构造加两次 reverify 的组件中位耗时 86.59105→16.38595 ms；每组均有六次完整 2,379,776 字节哈希、六次模块查询、十二次进程身份查询及十二次钩子读取。最初与 Python 检查并行的测量另存，最终数值在检查结束后取得。这是本地证明组件开销，不是实际游戏或头显时长。

519 项 Python 检查通过（377 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查）。新增覆盖每次读取按实际文件大小分配且六次完整哈希不减少、零／负／超限大小在读取前拒绝、读取期间增长、同长度篡改在最终边界拒绝。更新后的证明模块已接入正常恢复流程；native／XR／Playground、清单和用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据保留在 ignored `analysis/mounted-attestation-read-20261022/`。

## 证书工作线程完成后唤醒 owner（2026-10-22）

准备工作完成后，owner 原先仍可能等当前 10／100／250 ms 休眠结束。现在为每个准备 Future 注册独立的 Python 完成通知，主循环等待可由该任务完成唤醒；下一次 poll 继续核对完整状态与候选并发送 Prepare／Commit。工作线程不操作 native 控制事件，通知本身不接受候选。每个任务使用独立通知，旧回调不会唤醒新请求；状态空档中一次消费通知，避免完成回执尚不能处理时发生立即循环。退避、禁用／关闭和无工作时保持原等待策略，关闭继续取消并等待自有工作线程。

515 项 Python 检查通过（373 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查）。新增实际线程等待／完成测试、提前完成回执、状态空档不重复唤醒、旧回调与新 Future 隔离、退避／禁用、关闭后晚完成不发送控制，以及真正执行一轮 owner 循环的接线检查。原接线夹具进入循环前便收到停止状态，已补运行中迭代用例。

保存旧源码的确定性模型设工作于 5 ms 完成：owner 准备请求从原 250／10 ms 检查点改为完成通知后的 5 ms 检查点；这是模型，不是实际游戏或头显时长。两份 Python 模块已接入默认流程，保持上一轮 50 ms 提示节奏、全部身份／代码／GPU／世界交接证明、WorldProfile1 native、XR／Playground、清单与用户配置；83 项打包文件核对通过。本轮未启动游戏或模拟器，真机效果由用户统一验收。证据保留在 ignored `analysis/mounted-worker-wake-20261022/`。

## 缩短发现机枪切换前的调度等待（2026-10-22）

连续会话的只读拓扑提示改为名义 50 ms 间隔，owner 空闲等待遵守提示／定期完整观察中较早的截止时间，保留退避下限和 10 ms 最小休眠。仅在当前 native session／进程检查通过且无 pending candidate 时启用该节奏；状态缺失／不匹配、有待处理候选、适配器禁用或没有提示回调时沿用普通等待。构造器通用默认值保持，实际连续启动路径显式选择 50 ms；非有限超时输入被拒绝。每次提示仍只读取两个链接，候选与通知继续要求完整证书和原有所有证明。

保存旧源码的确定性调度模型中，变化设在一次检查后 1 ms：普通 100／250 ms 循环的准备请求计划等待分别为 99／249 ms，新路径均为 49 ms；观察／准备调用数保持。这只证明软件调度等待减少，未测量 Windows 实际唤醒、完整 owner 循环或游戏／头显延迟。对本测试进程的两次指针读取及七次身份查询，200 次组件测量中位 0.9275 ms、P95 1.6093 ms；检查频率提高至名义 20 Hz，不能据此声称实际游戏 CPU／FPS 不变。

509 项 Python 检查通过（367 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查），新增覆盖提示截止时间、保持完整观察周期、无额外证书、状态间断、pending／停止／session 不匹配、退避与最小休眠、禁用／旧适配器及非有限时间输入。两个更新后的 Python 模块已接入默认流程；native／XR／Playground、清单与用户配置保持，83 项打包文件核对通过。本轮未启动游戏或模拟器，真机由用户统一验收。证据保留在 ignored `analysis/mounted-hint-deadline-20261022/`，组件开销记录为 `analysis/runtime/mounted-hint-cost-20261022.json`。

## 减少恢复协调器的重复获取与全局发现（2026-10-22）

旧世界已排空且当前 session 明确要求恢复时，协调器改用已有的单次完整获取入口，省去前置独立观察；没有该入口的适配器保持原流程，foreign local、已有 pending candidate、停止状态和进程／owner 约束继续执行。保存旧源码对照确认该路径由观察一次＋准备一次变为单次完整准备，Prepare／Commit 数量与最终提交结果一致。

完整候选生成后，以其中已验证的相机信息与最终 guard 更新发现位置提示，避免连续上／下机枪在下一轮又做全局相机发现。提示只保留相机／controller／local-player 的完整身份及 name index；后续仍加载当前注册表、重新读取三个身份并生成完整证书。缺失或不一致元数据清空提示并回到原发现路径。旧源码对照中下一轮全局发现调用 1→0，当前注册表准备仍执行一次，返回相机一致。机枪合成内存用例确认下机枪仍由实时 guard 识别，同地址身份代次变化会使提示失效。

504 项 Python 检查通过（362 项相关检查启用实际 WorldProfile1 DLL，142 项启动器／打包检查），覆盖同一身份恢复、短退避、旧适配器、foreign local／pending／停止、候选拒绝不更新提示、发现数据独立副本和身份复用。现行默认启动器直接使用这两个更新后的 Python 模块；沿用 WorldProfile1 native／XR／Playground，未重建 DLL、未改清单或用户配置，83 项打包文件及全部既有修复保持。对照只证明获取和发现工作减少，没有测量实际切换时间；本轮未启动游戏或模拟器，真机效果由用户统一验收。证据保留在 ignored `analysis/mounted-owner-acquisition-20261022/`。

## 缩短世界准备的配置读取开销（2026-10-22）

继续推进代码与离线检查。将世界准备、Anchor／UI／crosshair 的逐字段 Windows INI 调用，改为现有只读文件锁内一次读取、准备线程局部查表；规范格式以外沿用原 API，所有实时证明和提交条件保持。原生合成 939 字段组件测量中，原 API 查询中位数 39.0304 ms，新路径载入 0.2489 ms＋查询 0.2249 ms，合计约 0.47 ms。这是配置文字读取工作，不是实际机枪黑屏时长。

42 项原生／启动前检查和 495 项 Python 检查通过；新增 36,340 项字段等价、大小写、截断、格式回退、文件锁和缓存失效检查，机枪组件在两种读取路径下执行 233 项检查。353 项相关 Python 检查启用实际新 DLL，142 项启动器／打包检查通过。

从最新 WorldPrepare2 冻结源接入默认 build-world-profile1/GearsCaptureWorldProfile1.dll（SHA256 01d2823fcb4d6ad87ade125e91497037c2bdb7d238f54104644047ae87b6d6a1）；八份相关源文件改变，490 项编译依赖／源哈希及 83 项打包文件核对通过。只替换三个 native 字段，保留已有修复、XR／Playground 和用户配置。本轮未启动游戏或模拟器，真机验收由用户统一安排。见[实现与组件测量](../../mounted-profile-snapshot-20261022.md)。

## 机枪准星准备阶段的暂态失败继续恢复（2026-10-22）

按用户要求继续代码修复和自动化验证，头显效果由用户统一验收。补上 WorldPrepare1 的另一条诊断遗漏：机枪准星恢复已识别相机时间戳跨帧，但准备适配层丢掉原因，世界准备仍可能累计普通次数并停止。现将精确诊断传回，接入已有确切请求回执和完整重新采样；身份／连接／读取错误保留原规则，不使用旧诊断，不改变武器瞄准或准星投影。

41 项原生／启动前检查及 495 项 Python 检查通过。机枪原生组件使用合成反射内存／INI 完成 209 项检查，覆盖跨帧同时断连、角色不符、读取失败、旧诊断清除、流式 Level 与下机枪后的普通状态。353 项相关 Python 检查启用实际新 DLL，142 项启动器／打包检查通过；没有启动游戏或模拟器。

从最新 WorldPrepare1 冻结源接入默认 build-world-prepare2/GearsCaptureWorldPrepare2.dll（SHA256 76018f7c3760f5da2cf8ee44490ef7742927a1037780c98788bcbeb9513b4321），四份相关源文件变化，489 项编译依赖／源哈希及 83 项打包文件核对通过。只更新三个 native 清单字段，保留已有修复、XR／Playground 和用户配置。目标继续进行，尚未声称实际无延迟。见[后续加固与验证](../../mounted-native-prepare-retry-20261022.md)。

## 原生准备时间戳拒绝后的持续恢复（2026-10-22）

修复原生 Prepare 的纯时间戳 bracket 失败仍累计普通失败、同一身份三次后可能永久停止生成候选的缺口。新增确切请求回执，owner 只在 session／owner／generation／请求／拒绝增量／无候选均匹配时撤回本次暂态失败次数，重新完整采样并生成新请求；保留既有硬错误次数、所有校验与交接证明。前三次名义重试间隔 50／100／150 ms，之后普通退避，原生真正接受候选后重置短重试计数。迟到成功、结构／读取失败及不明 UI／crosshair 拒绝保持原处理。日志增加工作请求／开始／结束的 QPC 纳秒边界。

40 项原生／启动前检查和 495 项 Python 检查通过（353 项相关检查启用实际新 DLL，142 项启动器／打包检查）。保存旧源码与新版作相同离线回放，旧版三次拒绝后停止准备，新版连续六次暂态拒绝后在条件稳定时完成提交；覆盖旧／错回执、进程复用、保留硬失败预算、迟到成功和时间边界。489 项编译依赖及冻结源哈希核对通过。

已从最新 WorldWake2 冻结源接入默认 build-world-prepare1/GearsCaptureWorldPrepare1.dll（SHA256 7eb03a5b87e96ca39a482a599d56e840e62c0c99af36fc0cd3c472a654bf2906）。只更新三个 native 清单字段，83 项打包文件存在，保留 XR／Playground／用户配置和已有修复。本轮未启动游戏；修复的是可复现的恢复停滞条件，重试间隔不是实际黑屏时长，无延迟目标尚缺当前机枪／头显验收。见[修复边界与验证](../../mounted-native-prepare-retry-20261022.md)。

## Playground 菜单零尺寸修复（2026-10-22）

玩家 PID44720／49184 均创建了菜单按钮，但旧实现清零 `DockTargets`，将四个 `TargetFace` 设成 `UIFACE_Left=0`；场景更新把四条边归到0，实际 `RenderBounds` 与 `Position` 全0。这解释了“能通过键盘进入、画面却没有入口”。之前仅凭文字和键盘导航的验收不构成可见性通过。

已改用游戏 `SetDockTarget(TargetFace=UIFACE_MAX=4)` 解除绑定，保留 OwnerWidget 初始化，沿现有菜单列与末行高度放置，并重新请求 docking／位置／样式更新。状态增加实际范围与 `menu_layout_valid`；活跃主菜单切换后重新挂接入口。当前默认仅提升菜单组件至 `build-playground6/Release/GearsPlayground.dll`，SHA256 `a7b4f3fbe1a900afb5222d5a7ba463bb19bad0cb8b1c859485354e703a4c0535`，保留当时最新 native／XR 与用户配置。

实际零售后台 PID32444：正常启动创建非零按钮（宽约463～500、高29.33像素），Down／Enter 进入测试场，11拾取物／6靶／Troika／失败0，已有实际双目提交；原版返回主菜单后新按钮非零。后台初始菜单图为黑帧，返回图为旧地图帧，未取得菜单视觉通过，也未取得本轮完整停止/清理通过，不能用布局数据代替像素验收。候选5的扩大 native 调用路径未解决问题并已撤回；候选7附加跟随动画未完成验证，未集成。用户明确要求停止进一步验证并自行进入检查，因此停止后续测试，仅接入已有实际进入证据的6号构建。证据在忽略目录 `analysis/playground-menu-visible-20261022/` 与 `analysis/playground-20261021/playground-menu-visible6a/`。


## Playground 初始化中断 VR 启动修复（2026-10-22）

用户未见入口，先确认其游戏子目录公开包没有Playground；随后根目录开发版实际报初始化5／MinHook创建9。仅扩大窗口的正版候选仍失败于trampoline分配，未作为场景通过。修复为启动早期在同一Playground DLL创建disabled Hook，引擎就绪后再启用；完整64字节认证、跳转可达性和独立还原检查保留。资源失败只有在原入口完整未变得到独立确认后才允许继续VR，未知／身份／证书失败仍拒绝。正常停止及启用前取消均回收预留，清理不重新注入。

正版 PID49328 实测菜单Down／Enter进入测试场、11拾取物／6靶／Troika／失败0、射击击杀、换弹／备用弹药不减少和重生通过，XR提交14,031次游戏投影。Playground64字节还原、native自身清理、XR退出0通过；总校验仍有7个DXGI入口缺少安装前证据且与磁盘不匹配，整体回执保持incomplete，未放宽。PID49544启用前取消的预留回收及native prime入口还原通过。213项启动器检查、Hook受限地址窗口正反例和722文件便携依赖检查通过；旧预留检查2项mock报错在未改HEAD源码复现并保留。

默认组件更新为 `build-playground4/Release/GearsPlayground.dll`（SHA256 `b6c32ebfd84cbaa068c6943bfc2a178caaea3bb9009c3a709d6288fbc096608a`），启用 `playground_early_reserve`。只改最新默认的3个Playground字段，保留刚集成的CoverAim2 native、XR及用户配置，不改公开包。见[定位、验证与限制](../../playground-startup-reservation-20261022.md)。

## 合并0.0.2启动修复（2026-10-22）

按用户要求切回 `codex/0.1.0`，将 `codex/0.0.2` 的完整修复与发布记录并入。合并前开发检查点为 `963d965b8590555bae31d047bef6ccc27d3e6469`，热修复源为 `a0a39b0`。公开0.0.1是独立根提交，两分支没有共同祖先；以公开源码 `4b621fc430fa5f96eba3bf70399810d3fd51e202` 为明确的内容基准进行三方合并，并保留两个原始父历史，不改写既有提交或发布标签。

已保留0.1.0第一人称、Playground、右摇杆死区和渲染源码／默认清单；便携模块白名单同时包含Playground与新增startup_diagnostics。接入0.0.2的UTF-8解释器探测、真实Steam子进程环境回执、有界启动分阶段日志、失败原因和Export logs摘要。根README与AGENTS更新为0.1.0开发上下文，公开版本的构建指南不覆盖本分支已有的删除决定。合并前本地配置仅有格式差异；其全部参数值保留，并恢复本分支的第一人称和死区选项。原始配置与合并回执保存在忽略的 `analysis/git-integration-0.0.2-to-0.1.0/`。

离线验证：88项debug-launch、204项vr-launcher检查通过；GUI编译与隐藏自检通过，涵盖第一人称、死区、配置保留、路径发现及Python探测。消除了内容合并重复引入的相同Steam环境函数和测试定义。新增Python文件语法、冲突标记、差异空白及源文件范围检查通过；默认native／XR／Playground文件SHA256与0.1.0原清单一致。

附加的13项预留生命周期／预启动回归中10项通过、3项旧夹具报错。用合并前 `963d965` 的启动预留源码回放同组测试，复现相同3项错误：两处mock返回值缺stdout／stderr，一处旧mock仍等待PowerShell `-Command`，随后解析了占位DLL。这些失败保留为既有测试限制；两个回放日志位于上述忽略目录。本次没有启动游戏或模拟器，不新增GPU／头显验收，不生成或发布新包。发布版0.0.1／0.0.2、热修复分支和既有本地二进制保留。

2026-10-21 分支保存：按用户要求，将当前第一人称、Playground、性能／恢复优化、正版迁移及启动修复等项目源码保存到 `codex/0.1.0`，随后从公开0.0.1基线分出0.0.2，只做启动兼容修复。此检查点不代表0.1.0已经发布或新增全量玩法验收。

解压0.0.1本机失败已定位到下载脚本的PowerShell执行策略拦截，并独立确认旧版多账号设置检查会拒绝。仅对子进程指定执行策略、保留两路错误输出／UTF-8，配合Steam环境和账号选择后，后台实际游戏到达AttractLauncher并提交启动菜单；正常退出、prime还原、音频及11个保护文件恢复通过。49项启动层检查和107项启动器／诊断检查通过；0.0.2版本与包的检查将在新分支继续。详见[本机复现记录](../../164-launcher-steam-install-feedback.md)。

## 工作副本直接启动入口（2026-10-21）

用户反馈直接双击工作副本 `Binaries_x64/GOWDE-Steam.exe` 仍会进入反作弊启动画面。已有正版启动证据确认，缺少 Steam App ID 上下文会请求 `steam://run/2523720`，转由 Steam 库中的 `Launch_GOWDE.exe` 启动。按 [Valve 官方 API 文档](https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary)，现仅在工作副本 EXE 同目录新增本地 `steam_appid.txt`，内容为 ASCII `2523720`，使直接运行时 Steam API 不再要求通过库中入口重启。Steam 仍需运行并登录拥有游戏的账号；正常 VR 启动器继续使用已有的子进程 App ID 环境。

已回读确认文件为7字节、无BOM且内容正确，位于忽略的游戏目录中；没有修改 EXE、Steam DLL、EAC 组件、Steam 库内原始安装或发布包。删除此新增文件即可撤销。本次遵守停止追加兼容性游戏运行测试的要求，未启动游戏；尚未实测本文件方案下的启动画面或耗时，也不声称卸载或全面关闭游戏内反作弊。

## 正版基线、启动兼容与存档迁移（2026-10-21）

按用户要求，今后开发和测试改用购买的 Steam 正版，默认目录为 `J:\gearsVr\Gears of War Reloaded`，Steam 库内原始安装保留。完整读取并计算三份安装的 SHA256：两份正版3,609文件完全一致；与历史旧目录共有的3,609文件中3,607一致，差异仅为原版／替换的 `steam_api64.dll` 及已有可恢复启动补丁 `Coalesced.ini`。核心EXE均为1.0.381.6947、26,278,928字节，SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`；2,518个CookedPC文件一致，现有核心逆向成果可继续使用。

首次正版启动复现 `Target priming failed (1)`，Steam日志确认原进程请求 `steam://run/2523720`，Steam另起库内启动包装程序，原预留PID退出。启动层已在子进程环境中传入本游戏 `SteamAppId`／`SteamGameId`，保留真实Steam DLL、登录及授权检查；Windows环境变量名按大小写不敏感去重，不改变父进程。修复后正版预留成功、13处入口字节核对通过，保持选定正版目录。首次启动生成第二套用户设置后，新增按当前Steam活动账号精确选择设置，避免多文件拒绝和改错账号。85项debug-launch及76项launcher离线检查通过。

用户随后明确停止继续兼容性运行测试，并要求迁移存档。所有自建游戏／XR进程已停止，owned prime、菜单组件和测试音频恢复通过；最后一轮在新闻欢迎页停止，未进入关卡、未激活双目native，不作正版GPU／头显验收。因关闭过程中次要窗口先消失产生WinError1400，单独复核原预留清理后完成正常进程退出及10个受保护文件恢复；保留原失败回执。

当前旧账号4个存档伴随文件按正版活动账号命名复制到共用的 `%LOCALAPPDATA%\Coalition\WarGame\SaveData`。原旧档和正版覆盖前副本均已备份；目标四文件逐一完整SHA256与源相同，主档和local档zlib校验及声明长度有效，未改序列化内容。按用户要求没有再启动游戏验证读取／继续章节。普通／DLSS清单、用户配置和开发工具默认路径已更新；native、XR、playground二进制保留原冻结哈希，历史实验清单、公开v0.0.1标签和已发布包不变。见[完整记录](../../retail-steam-compatibility-20261021.md)，本地回执在忽略目录 `analysis/retail-compatibility-20261021/`。

## happyotaku playground 测试场（2026-10-21，已接入默认）

按用户要求新增主菜单 `happyotaku playground` 入口，已向 Astra Max 咨询并采用包含现成 Troika 的 `MP_WarMachine`。11种枪械／投掷物拾取点、6个兽人静止靶和原有机枪生成通过，失败计数0；靶子死亡后三秒重生，枪库使用后补回。无限备用弹药保留正常弹匣消耗、装填及主动装填。

首次完整 VR 输出失败已定位并修复为 FXAA 最终输出调用的缺失分类：增加完整函数字节认证及现有 owned world／ticket／左右眼像素映射约束，不放宽通用绘制、HUD 或 GPU 提交检查。三轮完整启动器／模拟器流程正常停止通过，分别发布18,008、37,157、6,248帧；前两轮测试场实际射击击杀、换弹／备用弹药不减少和靶子重生通过，上下机枪通过，后两轮使用每眼1536×1632。GPU读回确认地图、枪械、机枪与靶子可见。普通战役回归没有生成测试场或启用测试场弹药维护。三轮 native 清理／入口还原、XR退出0、菜单桥接独立入口还原均通过。

启动器189项检查、受限输出几何正反例、原生启动检查及721文件的隔离便携目录依赖检查通过。当前默认 native 为 `build-playground-output1/GearsCapturePlayground1.dll`（SHA256 `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`），菜单组件为 `build-playground2/Release/GearsPlayground.dll`（`e1d07405d0b8c3393783d5de549c44d7e20647efcf5bcc2e82b0646e6cd038ee`）。仅更新当前默认清单的7个相关字段，基于原 `045ca20a…b9d` 冻结源码保留已集成修复，用户配置不变。失败回执继续保留，不作玩法／VR通过证据；0.0.1 标签和已发布包不变。实体头显、VR手柄与整体游玩体验仍由用户之后统一验收，本次不是新版本发布。见[实现与验证记录](../../happyotaku-playground-20261021.md)。

20:55 完成当前默认构建的补验：11种武器逐项原版拾取／库存和无限备用弹药通过；枪库补回16次，11拾取物／6靶／失败0。原版退出主菜单后，Down／Enter 再次进入新世界通过。按原版 R2 按住流程，Troika 经过0.5秒预热实际发射28发，松开停止、上下机枪通过；此前缺少控制器按住状态及脚本参数／名称扫描错误的诊断回执保留，不作为发射证据。合并检查再次通过，无需更换已集成构建。测试入口64字节还原、游戏音频恢复、独立游戏进程退出均确认，当前状态完成审计 `analysis/playground-20261021/completion-audit.json` 通过。

## 玩家启动失败修复（2026-10-21）

用户反馈普通入口进不了 VR。已从其 PID44044 日志定位：UI 组装的256条可选身份缓存满后直接拒绝配置，native 尚未激活。修复为缓存满后执行原有实时读取，保留最终96条证书身份限制及全部结束复核。新增超过256个扫描成员的回归用例，247项相关检查通过；标准后台正常启动完整复测通过，实际256条缓存满后发生1743次回退，约90秒8136次双目提交，native/XR/owner exit0、Hook 校验及音频/死区/存档/设置恢复均通过。保留第一人称、2%死区与现有恢复优化，见[故障与验证记录](../../vr-startup-ui-cache-regression-20261021.md)。

2026-10-21 玩家启动日志跟进：22 项自检及游戏创建成功，失败点为 `Target priming failed (1)`；排除前述 DLSS 保存设置检查拦截。核对 0.0.1 冻结源码后确认 1 是远程线程退出码，尚不能认定为某个 Hook 返回值或游戏崩溃。待玩家提供 `target-prime-status.txt` 及报错后游戏存活情况，见[证据与边界](../../164-launcher-steam-install-feedback.md)。

## 体感装填前期：弹药与弹匣模型（2026-10-21）

按用户要求先找可手持装填的完整弹药。已从主包及三张地图的保存清单中导出17个真实弹药相关网格，提供 OBJ、GLB、独立本地预览页与静态多角度图。Lancer、Hammerburst 独立弹匣及其原有 MagazineMesh 引用已确认；另有 Boomshot 弹匣命名资源。Gnasher 霰弹与四种单发步枪／手枪候选经实物检查及抛壳模块引用复核，属于空壳，不能当成完整未发射弹药；Boltok、Gnasher 的独立完整弹药仍未确认，结论限于已检查范围。

17份源导出哈希、OBJ／GLB／网页数组对应检查及 JavaScript 语法检查通过。补查全部1,034个非本地化资源包名称表，全部成功；另核对 Commando Dom 的 Bullet_Strap 骨骼／角色几何线索，未确认可直接使用的完整单发。名称扫描不能证明任意网格内不存在完整弹药。自动打开 `file://` 页面被浏览器安全策略拦截，未绕过；浏览器交互未验收。原版普通换弹和主动换弹成功／失败入口已从保存脚本定位，native 精确时序及卡壳连接待核。记录用户方案：左肩取弹、插入、最终上膛动作，限时内完美换弹，超时普通装填且无卡壳惩罚；本轮没有修改启动器或游戏装填逻辑。见[资源与逻辑记录](../../ammo-and-physical-reload-20261021.md)。所有专有派生资源继续留在忽略的 analysis 中。

## 版本切换基线

2026-10-21 按用户要求，将本地当前源码、工具、资源和文档统一提交为 `5787a48`，完成 0.0.1 阶段的本地归档；后续工作在 `codex/0.1.0` 分支继续，目标版本为 0.1.0。

该归档也包含下文已有的开发中改动，不等同于 GitHub 已发布 `v0.0.1` 的精确源码或新增玩法验收。已发布的标签与下载包保持原样；运行记录、逆向分析数据和编译产物继续排除在源码提交之外。

2026-10-21 玩家安装反馈：已修复 Mod 与游戏 EXE 同目录时无法自动识别的问题，补齐一层 Mod 子目录；启动预留错误直接显示具体原因。修复合入本地正常启动器，保留当前 native/XR 默认构建。玩家截图中的启动失败仅有汇总错误，对方尚未提供详细日志，根因未确认。见[反馈处理与验证](../../164-launcher-steam-install-feedback.md)。

2026-10-21，用户确认开始 0.1.0 工作，首项为继续缩短上下机枪时的黑屏／画面恢复等待。完成代码和必要自动验证后接入正常启动器；头显体验由用户有空时统一验证。

## 首项：上下机枪恢复延迟

首轮缩短等待的实现及必要自动验证已完成，已接入正常启动器；头显上下机枪体验待用户之后统一验收。优化包含一次完整采集路径、有界快速握手轮询、同次属性链复用及 UI 类发现的批量读取。保留最新默认 native 中已集成的 FPS、ADS 和瞄准修复。

同进程四次交替对照中，成功准备的中位数从约 756 ms 降到 643 ms，节省约 113 ms（14.9%），四份权威配置内容一致。该结果衡量 CPU 配置准备，头显黑屏总时长尚未实测；严格跨帧保护引起的重采次数和总耗时保留在技术记录中。

289 项 Python 检查全部通过。对照轮和合入默认源码后的回归轮均完成 checkpoint 新世界恢复与独立 XR 提交，正常退出、79 项入口还原及五份保护文件还原均通过。测试按用户要求等待其他 FPS 工作者，不跨任务发送消息、不打断现有测试。

完整结果见 [首项完成记录](../../mounted-transition-latency4-20261021.md)，前序改动及失败回执见 [第一轮优化](../../mounted-transition-latency2-20261021.md) 和 [属性链优化](../../mounted-transition-latency3-20261021.md)。本记录表示开发进度，不表示 0.1.0 已经发布。

## 继续朝无延迟切换推进

当前完整目标为上下机枪持续出图、没有额外恢复等待，尚未达成。新增静态证书准备、认证模块选择、菜单观察后台化及到期探测的单次采集路径。第一轮只读对照中，成功准备中位数约 643 → 513 ms，但整轮遇到共享源码变动，保留为失败，正在改用固定源码运行目录复验。CPU 配置准备改善不能代替实际机枪切换与头显连续出图证明，见 [后续开发与验证记录](../../mounted-transition-latency5-20261021.md)。

后续固定副本第三轮的新路径已通过恢复、独立 XR 出图及退出还原，完整记录仍区分 checkpoint 与实际机枪。继续减少 Anchor/UI/Crosshair 单次组装中的重复身份读取；当前工作区相关 433 项 Python 检查全部通过，正常启动器已使用该代码。最新固定副本在 native 激活前失败，未获得本轮计时。用户约 16:03 明确由自己真机测试，本任务自动开局、采样及排队工作已全部结束，后续不为本任务寻找机枪或自动开局；实际收益等待用户反馈。见 [身份组装优化记录](../../mounted-transition-latency6-20261021.md)。

## 帧率目标继续验证

默认 native 仍为 CopySimple 全线程分支和 frame-link 有效前缀版（DLL SHA256 `ca66cdeac91915be17d30a5b1ff5008ae575acb6f7e60d82ff4ffb6f5f8a18c0`）。正式启动器模拟器关卡回归在 4K 游戏输出、VR 缩放 0.25、120 Hz 匹配尺寸条件下取得约 63 秒窗口 **111.098 个新立体帧/秒**，尚未达到 120，也不能作为新版头显 FPS。viewport 槽位前缀实验的 A/候选/B 绘制负载不相近，未集成。私有帧事件延迟清零实验已得到默认 A 114.412、候选 111.043 FPS，两段各 60 有效玩法样本，末段默认 B 因另一项自动试玩工作进程仍活跃而取消；候选未集成。下一私有实验准备测正式构建中细粒度 QPC 计时自身的开销，只完成源码预检，尚无编译或性能结论。完整证据见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

补充：帧事件候选在较轻绘制负载下仍慢，未集成；其末段默认 B 因旧空闲门槛没有排除活跃独立自动试玩工作进程而安全取消。旧守护器停止时只在等待，没有本轮游戏子进程。现有 FPS 空闲检查已加入工作进程枚举，实测能拒绝 4 个并行试玩 PID。后续优先在所有试玩工作进程退出后构建并夹测细粒度计时开销候选。

自动试玩退出后，细粒度计时轻量候选已完成私有构建（DLL SHA256 `773a19e6588d0c52b56d21f610a502c09c9e6c6687ac4f7060ffdf8aaeb36b3b`）、481 项依赖核对与 24/24 原生自检。新版空闲门槛下的默认 A → 候选 → 默认 B 三段模拟器关卡测试已排队，当前尚无 FPS 结果；默认启动器未更换。参见 [详细证据](../../157-vr-fps-hot-path.md)。

随后默认启动器合入第一人称预览与右摇杆死区控制，默认清单 SHA256 变为 `b902bfa3…f6a3b`、native DLL 为 `045ca20a…b9d`。上述旧基线计时轻量三段测试在启动游戏前检测到默认清单变化，`runs=[]`，安全终止。旧候选没有玩法 FPS 结论。计时轻量改动已复制到新默认冻结源码的私有副本，500 个文件只改 `render_cost.h`；新构建和测试脚本的只读预检通过，仍需等并行自动试玩结束后构建、夹测。之前的 111.098 FPS 是前一默认版本的模拟器回归，不能写作新默认版的 FPS 或真机实测。

新版计时轻量私有构建与三段对照已交给一次性隐藏排队器 `analysis/runtime/idle-fps-render-cost-lite-preview2-queue-b-20261021.status.json`，PID 21512；先等约五分钟稳定空闲，不占用正在运行的游戏。构建前将重核默认清单哈希，构建后要求 26 项原生检查通过；候选尚未有新基线玩法 FPS，也未集成。旧排队器的 UTF-8 命令引号问题在尚未构建和开局时发现并修正，旧状态保留。

新版计时轻量 DLL `e0cd2215…14c6` 已完成私有构建、484 项依赖核对和包含第一人称检查的 26/26 原生自检。基线采用第一人称预览提升回执的实际文件哈希，修正了冻结源码附带哈希表滞后问题；物理源码只改 `render_cost.h`。A→候选→B 的一次性后台守护 PID 40704 已排队、等待稳定空闲，暂无关卡 FPS 结论，默认版仍未替换。

17:10–17:23 根据用户要求补测高 VR 分辨率。当前默认第一人称预览 DLL `045ca20a…b9d` 在 4K 游戏输出、120 Hz 匹配尺寸模拟器的同一室外街景，Render scale **0.7 / 0.25** 分别测得 **60.487 / 61.426 个新立体帧/秒**，各 60 个有效玩法样本、约 63 秒窗口；0.7 的实际双眼尺寸为 4302×2285。绘制调用中位数约 1629/1548，截图场景一致，用户设置均已恢复、游戏退出、无并行游戏和试玩工作进程。单眼像素数相差约 7.84 倍而 FPS 仅差约 1.5%，故此前室内场景 0.25 的 118.213 FPS 不能用来评估升至 0.7 的损失；室外的低帧率更应追查游戏基础渲染与 CPU/同步/提交路径。计时轻量候选因 UI 证书暂存身份上限连续三次激活前失败，没有有效 FPS，未集成。详见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

17:45 的 720p 游戏输出、0.7 VR 缩放、同室外街景补测为 **59.422 新立体 FPS**，60 有效样本、draw 中位 1622；对照 4K 游戏输出、0.7 的 60.487 FPS/draw 1629，游戏桌面像素数降九倍仍无收益。双眼会话、游戏退出、设置恢复和无并行负载检查通过。当前约 60 FPS 不主要随这两处像素数变化，继续量化高绘制量下的主线程、帧锁与提交成本；尚不能宣称 CPU 根因，也不能将模拟器视为真机结果。

已把完成 26/26 自检的私有计时轻量候选重新排入 0.7 缩放、4K 输出的默认 A→候选→默认 B 夹测；UI 身份缓存满的现行修复通过 11 项 Python 针对性检查。新守护状态 `analysis/runtime/idle-fps-render-cost-lite-preview2-scale07-20261021-180345.status.json` 当前因用户游戏运行保持 `runs=[]` 等待，不干扰游戏，候选仍未集成。

复用 0.7 室外与 0.25 室内原生状态各前 120 个 ticket 的阶段时间：原始消费区间中位 **18.374 / 7.292 ms**，消费返回后到 ready 和实际发布各约 0.1–0.2 ms、0.03 ms。较大场景差异位于包含原游戏、Mod 钩子、驱动和等待的消费区间；不能由此把责任归于某一锁或 GPU。Astra Max 只读复核修正了 `frame_lock()` 短路条件的误读，建议待现有计时轻量对照完成后，低频抽样实际入锁等待和最外层持锁时间，按调用点与线程归因，再选局部候选。默认 DLL 仍未更换。

为在重场景定位消费者 CPU 栈，已准备私有 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07.py`，只读预检、实时状态合成 fixture 与旧室外/室内绘制负载识别通过。该诊断尚未运行；当前 QPC-lite 三段守护进程仍在等另一局游戏退出，保持互斥，不叠加采样。采样器若运行，其暂停线程的诊断开销使该局 FPS 只用于场景有效性，不作为优化收益。

匹配的私有离线分析器 `analysis/runtime/analyze-fps-consumer-stack-preview2-scale07.py` 也已准备，锁定当前默认 DLL 和采样器哈希，并要求堆栈时间戳落在 4K/0.7 室外玩法窗口内。语法和历史回执场景门槛检查通过；仍待取得本轮真实堆栈后才能做 CPU/锁归因。

18:39 检查到另一项 `analysis/playground-20261021/driver.py` 自动试玩工作进程在两局之间仍活跃，但原私有 FPS 空闲门槛只拦到了游戏映像。补齐工作进程识别，实际读到 PID `50276`；旧 QPC-lite 等待器确认 `runs=[]`、无子进程后安全替换。新隐藏等待 PID `38404` 已核实同时拦截该工作进程与正在运行的游戏，仍未启动任何一段 FPS 对照。未中断并行游戏，正式 DLL、用户配置未变。

另准备一个仅在单次 draw 内合并 owned target tracker 重复只读查询的私有源码候选，500 个冻结文件中仅三份实现文件及相应测试不同，见 `analysis/runtime/fps-tracker-snapshot-preview2-source.json`。它尚未构建、自检或测 FPS，默认版未改；先等现有 QPC-lite 对照结果，且该局部改动没有单独达到室外 120 FPS 的时间预算证据。

同一正式 DLL 的室外/室内各前 120 ticket 重算：消费 CPU 均值 **11.979/4.427 ms**，消费墙钟中位 **18.371/7.289 ms**，draw 中位 **1629/353.5**，`frame_lock` 入口计时中位 **5.640/2.383 ms**。类别有嵌套、样本只在会话开头，不能把差值直接当成某项可删除成本；下一步仍是室外玩法窗口的调用栈/等待归因，而不是依据局部查询候选承诺 120 FPS。

18:53 从正式 DLL 的首 120 ticket 跳过前 10 条后核对供帧间隔：室外 4K/0.7 与 4K/0.25 各 109 个间隔均分布为 72 个约 16.67 ms、37 个约 25 ms；720p/0.7 为 1/90/18 个约 8.33/16.67/25 ms，轻室内则 92/109 个约 8.33 ms。室外降像素后仍每两到三个 120 Hz 周期才来新帧，支持优先查游戏/Mod 消费路径与同步；这段开头采样不能单独确定具体根因。详见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

4K/0.7 室外前 120 ticket 的九类成本 Scope 调用合计中位 20,279 次/ticket，正式版每次两次 QPC，约 40,558 次 QPC/ticket；计时轻量候选是对此的端到端检验，仍在等待无并行游戏的 A/B/A 结果，不能预判收益。

19:12–19:33 的 4K/0.7 室外默认 A→计时轻量候选→默认 B 三段对照已完成，各 60 个有效新立体玩法样本，完整窗口 FPS **62.330 / 62.068 / 64.062**；首 120 ticket draw 中位 **1635.5 / 1599 / 1591**，双眼会话、游戏退出、设置恢复、无并行游戏均通过。候选比两端默认分别低 **0.42% / 3.11%**，默认漂移 2.78%，无端到端 FPS 收益，**不集成**。三段消费线程首 120 ticket CPU 平均 **12.500 / 10.156 / 10.677 ms**，基线本身明显漂移，不能将候选与 A 的差额当稳定 CPU 节省。默认 DLL/清单及用户配置 SHA256 不变。下一步仍需重场景消费线程等待位置和锁持有者证据；完整回执与限制见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

19:34 私有重场景消费者堆栈诊断预检通过，隐藏等待器 PID `9728` 已启动；状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-20261021-1935.status.json` 检测到另一项游戏与试玩工作者，仍 `runs=[]` 等稳定空闲。它使用当前默认 DLL、4K/0.7/120 Hz 模拟器，一局中有界外部栈采样，诊断 FPS 不判优化收益，不改发布版或用户设置。

19:46 发现另一试玩启动者 `analysis/playground-20261021/vr_driver.py` PID `43952` 未被空闲门槛识别；补入工作者白名单，新的只读进程检查返回该 PID。原堆栈等待器确认 `runs=[]`、无子进程后安全替换，新隐藏 PID `42464` 状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-vrdriver-gated-20261021-1946.status.json` 同时挡住试玩游戏与工作进程，未启动我方诊断游戏。默认清单和用户配置未改。

19:47 为覆盖同一 `analysis/playground-*` 目录其他 `.py` 试玩脚本，将空闲门槛扩展为该目录内 Python 工作进程。PID `42464` 仍无游戏/子进程时安全替换为最终隐藏等待器 PID `32956`，状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-playground-gated-20261021-1948.status.json` 首次空闲检查已通过、`runs=[]`，发布版和用户设置不变。

19:51 等待器 PID `32956` 在 `runs=[]` 阶段遇瞬时 `PermissionError(13)` 退出，未启动游戏；来源缺少 traceback，不能确定。为恢复安全等待，状态文件原子替换对访问拒绝有界重试，空闲观察错误使稳定计数归零并记录错误，终止错误保留 traceback；语法和模拟一次拒绝的重试检查通过。新隐藏 PID `48656`、状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-resilient-20261021-1954.status.json` 已预检并核实存活，仍 `runs=[]`。默认 DLL/清单及用户设置不变。

19:58–20:01 的正式 DLL 重场景 4K/0.7 堆栈诊断取得 60 个有效玩法样本、draw 中位 1618、266/266 个恢复成功的外部栈快照；但全局监视器在同局发现另一 Python 试玩 PID `48396`，状态为 `profile_incomplete`，**不作为清洁验收或性能收益证据**。探索性分类有 69 次帧锁等待、56 次游戏 WaitOnAddress，不能确定并行工作者是否覆盖 8 秒采样，快照比例也不是每帧耗时。正式离线分析器已加整局无并行工作者门槛并实际拒绝该回执。后台监视器与采样器新增首次见到工作者及采样墙钟时间，便于下次确定重叠；语法/哈希预检通过，新隐藏等待 PID `46348`、状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-repeat-20261021-2003.status.json`，尚 `runs=[]`。

20:25 旧第一人称预览堆栈等待器 PID `46348` 在达到空闲门槛后的开局前校验发现默认清单已由其他开发于 20:21 变更为 `fc54cd22…54e3bd`、新 DLL `bf0a8f73…978855`，安全退出且 `runs=[]`。模拟器 profile、外部采样器及用户配置未变；不能把旧 DLL 诊断当当前默认版。新 4K/0.7 单轮基线脚本已用当前清单/DLL 哈希预检通过，隐藏等待 PID `40480`、状态 `analysis/runtime/idle-fps-current-default-scale07-playground-output1-20261021-2028.status.json` 已核实 `waiting_for_idle`、`runs=[]`；若清单再变会安全拒绝，默认发布文件和用户设置未被本项改动。

20:33–20:35 当前默认 DLL `bf0a8f73…978855` 的 4K 桌面、Render scale 0.7、120 Hz 模拟器室外重场景测试完成：60 个有效玩法样本，约 63.986 秒完整窗口 **66.920 个新立体帧/秒**，样本均值 **66.456**、前后 30 样本 **66.411/66.501**，draw 中位 **1646**。双眼会话通过、游戏退出、设置恢复、无并行游戏或试玩工作进程，清单/DLL/模拟器配置哈希匹配。该结果离 120 仍远，也不是真机实测；旧构建 FPS 不是此版的受控 A/B 对照。回执见 `analysis/runtime/idle-fps-current-default-scale07-playground-output1-20261021-2028.status.json`、`fps-resolution-4k-current-default-scale07-203317-20261021.json`；继续定位当前构建的帧关键路径。

20:48–20:50 同一当前默认 DLL 的 0.7 室外重场景消费线程外部堆栈诊断通过：60 个有效玩法样本、draw 中位 **1614**，**258/258** 次有界采样成功且线程均恢复，暂停最长 **0.909 ms**；整局无并行游戏/试玩工作者、用户设置恢复。瞬时样本中 **90/258** 次顶层栈显示等 Mod 全局帧锁、**44/258** 次在游戏 `WaitOnAddress`，其余为执行或其他等待。每条栈最多16帧，均标记部分展开；这些比例既非每帧耗时，也不能证明谁持锁。该局受外部采样器扰动，FPS 不作提帧验收。严格分析回执为 `analysis/runtime/fps-consumer-stack-current-scale07-analysis-20261021.json`，下一步用私有锁等待与持有计时按调用点/线程归因，正式默认版未改。

当前默认构建的私有锁等待/持有计时源码在 495 个文件中只改 5 个、标注 48 个 `frame_lock()` 调用点；仍保留原始锁及调用顺序。另一项游戏退出后，隐藏构建器在连续 20 次空闲检查后完成 DLL `4d4de7a0…977b2`，484 项依赖与 26 项原生自检通过，未覆盖默认版。诊断版 4K/0.7/120 Hz 室外关卡的一次性隐藏等待 PID `43540`，状态 `analysis/runtime/idle-fps-lock-profile-current-scale07-20261021-2104.status.json`；先等约五分钟无其他游戏、启动器空闲和低 CPU，再测实际关卡。诊断插桩影响 FPS，只用来找锁竞争热点。

私有锁计时局随后完成，60 个有效玩法样本、draw 中位 **1631**，双眼会话、游戏退出、设置恢复及无并行游戏/试玩工作者通过；清单/DLL 哈希匹配，默认版未替换。整段原生会话消费 **4979** 个 ticket，消费线程最外层锁等待累计按 ticket 粗归一化 **4.627 ms**；其中 Reset **2.038 ms/约75.7次**、DrawIndexed **1.135 ms/约1546次**、Viewports **0.650 ms/约94.3次**。非消费线程的 Execute 钩子最外层持锁累计约 **1.673 ms/ticket**，多个非消费线程也频繁 Reset。计时覆盖启动与玩法、增加 QPC 开销，归一化数值不是纯玩法每帧因果成本；诊断局完整窗口约 **65.098 FPS**，不得用于优化收益。它支持先缩短真实持锁工作并保持 Reset/Execute/Present 顺序，再做当前默认版 A/B/A 对照。见 `analysis/runtime/idle-fps-lock-profile-current-scale07-20261021-2104.status.json`。

当前默认版在相同 4K/0.7 重场景首 120 个 ticket 跳过前 10 个后的逐帧原生记录，消费线程平均约 **11.364 ms CPU**、**16.148 ms 墙钟**、约 **1644 次绘制转发**；内部计时类别互有重叠，不能相加。私有 128-ticket 锁时间线保持原锁及 Reset/Execute/Present 顺序，DLL `218ba72a…961cbc5` 的 26/26 项自检及一局游戏通过；原生状态 **325,322** 条事件、128 个完整 ticket、零溢出，60 个有效玩法 FPS 样本、draw 中位 **1597.5**，游戏退出、设置恢复、无并行负载。观察器各有一次 `render=unavailable` 与 `native=unchanged` 瞬时读数，严格按两次缺口、原生发布计数单调和其余会话条件恢复诊断资格；该局约 **59.254 FPS 受诊断开销影响，不作提帧比较**。同一批 128 帧中消费线程平均每帧等锁约 **6.807 ms**，与其他线程 Reset 跟踪更新、原始 Execute、原始 Reset 持锁阶段分别重叠约 **1.290 / 1.169 / 1.062 ms**；余约 **3.01 ms** 包含未记录的其他钩子持锁和调度唤醒，不能称为可节省时间。正确分析见 `analysis/runtime/fps-lock-timeline-current-scale07-analysis2-20261021.json`。下一私有诊断为所有非消费钩子在消费线程等待期间的锁持有重叠计时，源码已固定、只读预检通过，隐藏构建等待 PID `37460` 尚未更换默认版。

补充进展：该全钩子重叠诊断 DLL `2e149532…36c934` 已完成私有构建、484 项依赖核对和 **26/26** 原生自检；Reset 更新细分为附件退休、HDR/PSO 状态、owned scene、目标与视口、frame-link 等十段。一次性隐藏测试 PID `31288` 于 `idle-fps-lock-overlap-current-scale07-20261021-2208.status.json` 等约五分钟稳定无游戏，当前因另一局真机/游戏运行保持 `runs=[]`，没有抢占或覆盖。正式默认版 0.7 的只读整卡 NVML 诊断 PID `45672` 排在共享互斥锁之后，状态 `idle-fps-current-default-gpu-scale07-20261021-2217.status.json` 为等待前项；GPU 百分比只作瓶颈线索。正式默认 DLL/清单和用户配置哈希仍与 20:33 基线一致。两项尚无新 FPS 或 GPU 结果。

22:25 旧重叠诊断达到空闲门槛后，在**开局前**发现正式清单从 `fc54cd22…54e3bd` 变为 `050319a7…bfb86`，状态 `runs=[]`、安全结束；旧 GPU 排队器也在 `runs=[]`、仅等待时按 PID/命令行核对后停止并记录 `replaced_before_probe_due_default_change`。新旧清单除游戏相对路径外相同，正式 native DLL SHA256 `bf0a8f73…978855` 未变，新路径 EXE SHA256 仍为 `bdc6705d…18bb7`，用户配置改为新游戏绝对路径并保持 Render scale 0.5。本任务没有覆盖这些用户改动。只将私有运行清单的游戏路径重新固定，私有诊断 DLL 不重编、不替换默认版；回执 `analysis/runtime/fps-lock-overlap-current-runtime-rebase.json`。新隐藏重叠测试 PID `50556` 状态 `idle-fps-lock-overlap-current-scale07-rebased-20261021-2230.status.json` 为 `waiting_for_idle`、`runs=[]`，正式默认 GPU 诊断 PID `33812` 状态 `idle-fps-current-default-gpu-scale07-rebased-20261021-2230.status.json` 排队等待共享互斥锁。两轮仍各要求稳定空闲及正确加载哈希，未有新运行结果。

22:36–22:47 两项排队诊断已顺序完成采样，但**均未通过设置恢复验收**。私有全钩子锁时间线取得 128 个 ticket、326,582 条事件，消费线程等锁 7.207 ms/ticket 中与其他线程持锁重叠 6.166 ms；Reset 跟踪十段合计 1.706 ms/ticket，未发现可保持排序证明且单独回收 ≥2 ms 的局部工作。该局设置文件的 `ALLOWD3D10`、`StoredPPQuality` 在运行中变化，约 60.975 FPS 不进入优化对照，原生数据只用于探索性热点排序。正式 DLL 的独立 0.7 GPU 观察局有 63 个匹配玩法的 NVML 样本，整卡使用率中位 50%、最高 53%；该局 `QualityBloom=True→False`，约 62.805 FPS 同样不能用于正式对照。两局游戏均退出、无并行游戏，用户设置未被回写；默认 DLL 仍为 `bf0a8f73…978855`。详细回执与限制见 `docs/157-vr-fps-hot-path.md`。此前**完整通过**的 0.7 同关卡正式版基线仍是 66.920 新立体 FPS，距 120 目标较远。接下来优先归因正式版的 on-CPU 消费线程调用栈；现有 WPR 权限不足，既有 118 条非等待瞬时栈只可导航，不能当 CPU 耗时占比。

22:56 已用当前正式清单 `050319a7…bfb86` 和 DLL `bf0a8f73…978855` 通过只读预检，排队一次私有 0.7 重场景消费线程外部堆栈诊断 `analysis/runtime/idle-fps-consumer-stack-current-scale07-triple-20261021-2256.status.json`，隐藏等待 PID `46376`，最初状态 `waiting_for_idle`、`runs=[]`。守护器要求约五分钟连续无游戏、无试玩工作者、启动器空闲和低 CPU；游戏窗口置后、静音、只观察输出，禁后台物理手柄输入。它用现有已自检的采样器顺序取三段各最多 8 秒的瞬时栈，每段独立核实线程挂起恢复，整局仍只作诊断，不把受采样干扰的 FPS 当优化收益；若用户设置再次变化，严格会话仍拒绝 FPS 验收。默认 DLL 和用户设置未由此准备步骤改动。

23:02–23:04 三段栈诊断已完成，游戏/守护进程退出、画质设置恢复、无并行游戏/试玩工作者、正式 DLL/清单哈希一致；每段采样器都恢复了消费线程，合计 **783/783** 条栈，最长暂停 **0.587 ms**。XR 关闭缺 EndSession，严格会话 `passed=false`，本轮约 59.805 FPS 不作为性能对照；原生双眼提交和清理正常，故只把栈当探索性热点导航。380 条非等待快照的栈顶为游戏113、Mod102、NVIDIA驱动86、`ntdll`52、其他27，分布没有指向单一可安全删除 ≥3 ms 的函数；快照占比不是 CPU 毫秒。离线回执 `analysis/runtime/fps-consumer-stack-current-scale07-triple-exploratory-20261021.json`。未集成新的 FPS 候选，正式版 0.7 完整通过的 66.920 FPS 基线仍有效，120 FPS 未达成。

在用户明确要求提高到 0.7 且上述后台探针全部结束后，将 `tools/vr-launcher/config.json` 的 `render_scale` 从 **0.5** 更新为 **0.7**，其余当前配置字段保持相同；新配置 SHA256 `4c2c008a9f94a1c87a4fd25d881f317613bf69460ffd9d3da0589a541db08b6a`，当前没有运行中的游戏或启动器。之前的性能探针均通过临时覆盖使用 0.7，没有依赖本次持久配置修改。新默认画质只代表启动器选项，不代表达到 120 FPS。

为直接比较用户要求的 0.7 与低缩放，准备同一正式 DLL 的 4K 桌面、120 Hz 匹配模拟器室外关卡 **0.7→0.25→0.7** 三段夹测，测试只用临时配置，用户启动器保持 0.7。首段 23:23–23:26 取得 60 个玩法样本、设置恢复和游戏退出，但严格会话因独立恢复核验的 7 个 `dxgi.dll` 入口与磁盘字节不一致而未通过；它们在 Mod 安装前已有 `E9` 入口字节，原生停机自检显示恢复到安装前字节。58.637 FPS 不用于正式比较。真正的失败项是 `entry_restore_verified=false`；`terminal_exit_without_end` 在先前通过的基线也存在，不是本项的失败原因。旧独立回执未记录实际停机字节，不能追认这局。

`tools/native-capture/verify_stopped.py` 现仅在系统图形模块入口的安装前/停机自检快照唯一、一致且原生报告恢复成功、独立远程读到的停机字节与安装前 **32 字节完全相同**时，允许“恢复到安装前状态”；其他情形仍用磁盘 PE 字节验证，游戏入口认证不变。输出记录三方哈希及匹配类别，25 项 verifier 测试全过，未改正式 DLL。新一次性隐藏夹测 `analysis/runtime/idle-fps-current-default-resolution-aba-20261021-2332.status.json` 已排队；因为用户启动器当前打开，空闲门槛保持 0，尚未启动下一局，不干扰手动测试。

等待夹测期间，针对 120 FPS 的下一候选方向准备了私有 128-ticket 双眼绘制阶段诊断，源码在忽略目录 `analysis/fps-eye-phase-current-source/`。它仅在每次原始 renderer 消费调用内记录同线程直接 Draw 的普通左/右眼与最终左/右眼阶段切换、QPC 和相对线程 cycles；不新增游戏钩子、不变更锁或正式 DLL。当前正式 0.7 室外回执聚合的普通直接绘制约为每 ticket 左眼 963、右眼 654 次，表明两眼不完全相同。495 文件逐项哈希仅两文件不同，`build-fps-eye-phase-current.py --preflight-only` 通过；**未构建/未运行**，不会把该探针 FPS 当收益。待分辨率对照结果和用户窗口空闲后再构建，先判断有没有足够大的可共用阶段。

该私有诊断补充已有 `hook_geometry` 的按眼/stage/mode 包围计时与线程 cycles，直接判断这一有限逐眼函数是否够大；计时包含现有 Draw hook 观察开销，不代表原游戏纯函数或完整双眼渲染。修改后私有源码回执重新封存，495 文件预检再通过，正式版及其哈希未改。

逐眼诊断的离线校验器 `analysis/runtime/analyze-fps-eye-phase-current.py` 已准备：要求 2–129 共 128 个唯一 ticket、各段 QPC/cycles 连续、分段耗时及绘制数与原生汇总一致、零溢出，并把原始消费总墙钟、首次绘制前和四类绘制关联区间分开报告；`hook_geometry` 的整局聚合另列，不与首 128 帧直接相减。语法检查和合成完整/篡改回执拒绝检查通过。它尚未读过真实逐眼运行数据，探针仍未构建或用于性能结论；0.7→0.25→0.7 对照仍因用户启动器打开而等待，后台进程存活且未启动游戏。

23:55 用户从启动器运行游戏时保存了配置文件，Render scale 仍为 0.7，但完整配置 SHA256 从 `4c2c008a…8b6a` 变为 `ed66c7fd…6257`；旧 0.7→0.25→0.7 守护器 PID 49200 尚 `runs=[]`，若照旧运行会在空闲后因旧配置哈希退出。核对其命令行、零子进程、等待阶段后仅停止该守护器，将旧状态标记 `replaced_before_probe_due_user_config_change`，未碰用户游戏/启动器。私有守护脚本现到连续稳定空闲之后才锁定当前用户配置哈希，之后每段前后要求哈希一致；三段只在子进程临时覆盖缩放，不写用户配置。语法与清单/DLL/模拟器预检通过，新隐藏 PID 41524 状态 `analysis/runtime/idle-fps-current-default-resolution-aba-20261021-2356.status.json` 已核实存活、`waiting_for_idle`、`runs=[]`；当前启动器仍打开，尚无新 FPS 对照。

逐眼私有探针再补每个首 128 ticket 内 `hook_geometry` 按眼原函数包围耗时和调用次数，以便和**同一 ticket**的 `original_consume` 墙钟对照；这是包含现有 Draw hooks 的区间，若调用嵌套还可能重叠，不解释为独占游戏 CPU。离线校验器同步检查这两眼字段及完整/篡改合成记录。495 文件哈希回执重新封存、构建预检通过，原回执单独保留在 `analysis/runtime/fps-eye-phase-current-source-before-ticket-geometry.json`；尚未编译或测游戏，正式版不变。0.7→0.25→0.7 等待 PID 41524 仍存活，因用户启动器打开而保持零轮。

## 掩体举枪的独立瞄准状态锁定（2026-10-22）

修复掩体举枪时因 HUD 命中点查询回执缺失而将 aiming 当成 false、使相关 UI 退回随头定位的遗漏。改为在当前场景帧中独立读取经身份／反射属性／持有关系双次检查的控制器瞄准状态；有限点授权不再覆盖它。无查询及换枪后仍可保持瞄准锁定，松开时使用当前状态。

13项针对性原生检查、22项启动前检查、208项启动器检查通过；补齐原生夹具元数据，并修复一项启动诊断测试的 Windows 文件时间戳假陈旧断言，生产诊断逻辑不变。正版私有实例实际进入掩体，取得10次掩体瞄准采样；796帧举枪全部没有查询回执，新逻辑仍生成4354次瞄准 HUD 规划，当前状态读取拒绝0、预算超限0。后段原版跨平台弹窗中断剩余输入，原始严格退出回执的7项既有系统DXGI磁盘差异保留为 incomplete；补充审计独立停机哈希与同PID安装前32字节记录全部相同。原生资源、XR退出0、Playground入口、音频及游戏退出检查通过，用户原10个设置／存档哈希不变。未施加实际模拟器头动，也没有GPU像素／发射线／实体头显新增验收。

已集成正常启动器，仅更新3个 native 字段，使用 build-cover-aim2/GearsCaptureCoverAim2.dll（SHA256 04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0），基于最新默认 bf0a8f73…98855 冻结源保留第一人称、Playground和输出修复，XR及用户0.7比例保持。头显体验按用户安排之后统一验收；不要求单独测试入口，不构成发布。见[修复与证据](../../cover-aim-lock-20261022.md)。

## 上机枪直接结束 VR 模式（2026-10-22）

真机日志已定位：机枪恢复提交成功后，首个新资源帧仍携带 commit 完成记录前约 2.551 ms 的姿态，出图保护以 `ticket_invalid` 拒绝，调用方再将其升级为 `owned_error=19`，结束 VR 模式。已保留旧姿态拒绝和全部结构／资源校验，单独识别 `world_sample_stale` 并允许丢弃该帧、继续后续新姿态出图。

33 项原生／启动前检查、105 项启动器 Python 回归及附加图形锁检查通过；实际离屏 D3D12 检查覆盖三种资源状态下的边界旧姿态拒绝、共享槽不变、后续双眼像素成功发布及坏编号／坏几何继续拒绝。按用户安排未启动游戏测试机枪，真实上下机枪体验待后续使用反馈。

从最新 CoverAim2 冻结源制作，仅修改三份相关文件；正常启动器现使用 build-mounted-resume1/GearsCaptureMountedResume1.dll（SHA256 de25adb9d43b4886ff2e70cd37495a244f2031b97145d13294105b5ba61b90c2）。仅替换默认清单三个 native 字段，保留当前掩体瞄准、第一人称、Playground、renderer、XR 和用户配置；新游戏进程生效，未操作当前用户游戏。见[故障与修复记录](../../mounted-vr-exit-20261022.md)。

## 机枪恢复流程继续加强（2026-10-22）

按用户要求增强恢复边界和失败后续处理。新世界姿态截止时间前移到 StereoGate，旧姿态在构造 renderer 前跳过且不消耗回正；保留末端 GPU 旧帧保护。native 已暂停旧世界并明确拒绝候选时，同一身份可在退避后继续完整验证；通知暂时不可用或 SetEvent 失败时保留同一恢复包继续重试。已发送成功的事务不因超时重复通知，冲突及身份保护保持，下一恢复任务清除上一超时标记以恢复快速轮询。普通失败尝试仍有界，持续恢复退避默认封顶 6 秒，不给成功路径增加固定等待。

45 项原生／启动前检查、248 项 Python 检查通过；其中 64 次连续世界边界验证、三种状态下的离屏 D3D12 双眼像素恢复检查及连续失败后恢复／迟到回执／冲突保护均通过。修正一个启动自检夹具的提前停止位置，显式断言不会启动游戏。没有新增游戏或实体头显测试，实际上下机枪延迟待用户之后反馈。

已基于最新 SniperScope1 冻结源集成正常启动器，保留狙击镜、掩体瞄准、第一人称、Playground 及 renderer 修复；使用 build-mounted-resume2/GearsCaptureMountedResume2.dll（SHA256 e89ff571364c6afde938d558ab2860405b6bb13ad813c95e91546abed7c774fe）。485 项编译依赖核对、82 项打包文件存在性检查通过，只更新清单三个 native 字段，XR／Playground／用户配置不变。重新从启动器启动新游戏会话后生效，不构成新发布或头显验收。见[恢复加强记录](../../mounted-vr-exit-20261022.md)。

## 减少机枪恢复证书的读取工作（2026-10-22）

继续推进“无延迟上机枪”，将 UI 名称和瞄准对象身份中相邻字段合并为两次新鲜读取，保留选定字段比较、最终逐项复查、代码／脚本／链接和 attestation 证明。未知名称布局仍走原读取路径，没有跨采样复用认证结果。独立保存源码对照中，人形 UI 读取 1171→1061，机枪 UI 1215→1095（减少 9.88%），含举枪状态的瞄准证书 1534→1182（减少 22.95%），三项完整配置语义和 INI 字节一致。

336 项 Python 检查通过（231 项相关检查、105 项启动器检查），覆盖快照间身份／名称变化、短读取、填充变化、最终校验和原恢复状态机。补齐 MountedResume2 的命名候选清单，满足已有默认构建可追溯规则；82 项打包文件检查通过。优化随普通启动器默认启用，原生 DLL／XR／Playground／默认清单及用户配置不变。未启动游戏；读取次数不是实际恢复时间或黑屏缩短比例，无延迟目标尚未验收完成。见[证书读取优化与证据](../../mounted-vr-exit-20261022.md)。

## 缩短恢复拒绝后的等待与重复类发现（2026-10-22）

当原生 phase 4 已明确清空当前候选，且请求、拒绝计数和会话完全匹配时，前三次计划重试间隔由 2／4／6 秒缩短为 0.1／0.2／0.3 秒，之后回到原退避；owner 按剩余时间唤醒，旧超时标记不会拖慢新尝试。新尝试仍完整重新采样、使用新的请求路径；未收到回执的已发送事务不重发，其他错误／冲突不进入短重试。

同时仅保留已验证 `CameraActor` 类的查找地址，在每个新读取器中重新调用原发现读取，完整 guard 和最终证书证明保持。256 个无关对象的独立旧源码对照中，人物／机枪证书各减少 274 次读取，四项观察／证书结果一致，证书 INI 字节相同。查找预算模型和槽位／名称／元类／属性／代码篡改拒绝检查通过。332 项相关检查加 105 项启动器检查，共 437 项通过。

本轮仅集成 Python，保留刚推进的 SniperFirstPerson1 默认 DLL（9a43860f…dde75）及其以 MountedResume2 为父基线的世界恢复文件；82 项打包文件检查通过，native／XR／Playground／默认清单和用户配置未由本轮更改。没有启动游戏或新增头显采样，计划等待与读取次数不等于实际黑屏时长，无延迟目标继续进行。详见[恢复等待与类发现优化](../../mounted-vr-exit-20261022.md)。

## 恢复证书的只读原生类遍历（2026-10-22）

将 UI 类发现的逐对象 Python 循环移到原生辅助导出，通过当前 Resolver 的只读回调枚举候选；完整证书构造和最终身份／槽位／名称／继承／属性／脚本／代码／链接检查保留。能力缺失或布局不支持时回退原 Python 路径，DLL 按信任清单哈希加载，取消及注册表变化继续拒绝整次采样。

100,003 个合成对象的八段交错对照中，候选列表完全一致，遍历中位由 19.87415 ms 降至 3.1421 ms。该数字属于本地合成缓冲，不含实际游戏读取、完整恢复或头显时间。39 项原生／启动前检查、481 项 Python 检查通过；最终相关检查均启用实际 DLL，无跳过原生用例。人物及机枪完整 UI 配置／INI 相等、新名称槽位／变化、错误哈希、布局回退和取消等检查通过。

从最新 SniperFirstPerson1 冻结源制作并集成默认 build-class-discovery1/GearsCaptureClassDiscovery1.dll（SHA256 df7a5873185df14a6e7497758f433bdee3569aaaf86c387f8d1b67529a9849ad），保留各项已集成修复，488 项编译依赖核对通过。只更新三个 native 清单字段，打包白名单包含新辅助模块，83 项文件检查通过；XR／Playground／用户配置保持。未启动游戏或新增头显验收，无延迟目标尚未完成。见[实现、验证与边界](../../mounted-native-discovery-20261022.md)。

## 时间戳暂态拒绝后的快速重新采样（2026-10-22）

根据保存会话中五次严格采样均因相机时间戳跨帧失败的记录，修复该未发布候选的暂态错误仍等待 2 秒、并可能耗尽同一身份硬失败次数的问题。只对真实采样器异常和全部明确失败／可重试／纯时间戳的 guard 证据生成内部标记，前三次计划等待为 50／100／150 ms，之后回到普通退避；每次都关闭旧读取器／租约，重新完整验证，成功后重置短重试预算。发布和通知边界、五次／200 ms guard 上限及硬错误策略保留。

旧源码独立调度对照证实一次失败的计划等待 2 秒→50 ms，三次暂态失败后，旧路径在条件稳定后仍因尝试耗尽跳过准备，新路径可完整恢复。490 项 Python 检查通过（348 项相关检查启用实际当前 DLL，142 项启动器／打包检查），83 项打包文件检查通过。沿用当前 ClassDiscovery1，native／XR／Playground／默认清单及用户配置未变；没有启动游戏，调度间隔不是实际黑屏时间，无延迟目标仍未验收完成。见[时间戳重采样与边界](../../mounted-vr-exit-20261022.md)。

## 原生交接事件唤醒与有界快速轮询（2026-10-22）

将连续会话每轮固定 Sleep(20) 改为等待既有 Stop／Prepare／Commit 事件；收到通知可唤醒等待中的线程，auto-reset 事件通过待处理回执衔接原 poll，保留后续同类信号。有效交接进入名义 1 ms 轮询、最多 250 ms，包含提交后首个新图像的尾段；相同请求不延长窗口，到期回到 20 ms。停止清理仍休眠 20 ms，避免手动停止事件持续触发空转。候选授权、文件和所有生命周期／GPU 证明保持。

40 项原生／启动前检查和 490 项 Python 检查通过，含实际 Win32 事件的 1000 轮准备／提交、同时／后续信号不丢失、停止优先、等待失败、快速期／尾段界限及原有离屏双眼像素检查。首轮候选在增加边界防护后重新以全新目录构建，未集成首轮或复用其缓存。

已从最新 ClassDiscovery1 冻结源集成默认 build-world-wake2/GearsCaptureWorldWake2.dll（SHA256 180595f8969ad6c2ddab931997e827204374a2bba83d9c29a0294d4d0c36870b），保留所有已集成修复；489 项编译依赖及 83 项打包文件检查通过，只更新三个 native 清单字段，XR／Playground／用户配置保持。未启动游戏；1 ms 是名义等待间隔，无延迟目标尚无当前机枪／头显验收。见[实现与验证](../../mounted-world-event-wake-20261022.md)。

## 狙击枪开镜退出 VR（2026-10-22）

用户现场首个错误为 ticket 96429 的 `material_overlay_plan`（28），最终 `owned_error=24` 结束 VR。保存脚本及源码确认一个对应的支持遗漏：狙击镜全屏材质在 kind 1 准星范围内绘制，而旧材质规划无条件拒绝 kind 1。旧现场日志没有细分内部条件；新增分类诊断以便后续确认。

已补全屏准星材质的双眼路径，继续要求上传、命令列表、资源、帧编号及四角几何检查；镜框沿用同帧准星投影，出视野正常裁剪，不套用普通 HUD 的缩放或修改武器瞄准。54 组投影／回放用例及负例、共 40 项原生／启动前检查、109 项启动器检查通过，离屏 D3D12 共享输出回归通过；没有新增游戏／狙击镜 GPU 像素／头显验收。

从最新 MountedResume1 冻结源构建，仅五份相关文件变化，485 项编译依赖与源码哈希核验通过。已接入普通启动器：build-sniper-scope1/GearsCaptureSniperScope1.dll（SHA256 7f8e100e3144750354951e01b46348fe7c682ec080614cc78c1804d90c7ba693），仅替换三个 native 字段，保留机枪恢复、掩体瞄准、第一人称、Playground、XR 和用户配置；新游戏进程生效，不构成发布。见[修复与验证记录](../../sniper-scope-vr-exit-20261022.md)。

## Render scale 1.0 帧率基准与逐眼诊断排队（2026-10-22）

用户要求后续以 scale 1.0 的真实游戏关卡持续测试，有其他游戏使用时等待释放。旧工作树中 0.7→0.25→0.7 三段守护及其后继诊断均确认 runs=[]、无游戏子进程后停止，并标记为 scale 1.0 替换前退出；未采样也未改用户配置。当前默认 MountedResume2 DLL e89ff571…774fe 的 4K 桌面、120 Hz 匹配模拟器、scale 1.0 单轮 60 样本基准已通过源码语法、清单/DLL/模拟器配置哈希及设置只读预检；隐藏守护 PID 33284，状态 analysis/runtime/idle-fps-current-default-scale10-20261022-0340.status.json。它要求约五分钟无游戏、无并行试玩、启动器空闲和低 CPU，并用子进程临时缩放、窗口置后、静音、只观察输出、禁后台手柄输入；当前检测到用户游戏/启动器，runs=[]，没有开局。若默认清单在等待期间变更则安全拒绝，不能把旧构建结果写为新默认 FPS。另将私有 128-ticket 逐眼时序探针移植到 MountedResume2 冻结源码，核对 497 个文件、仅 capture.cpp 和 geometry_observer.h 两处预期变更；编译及 26 项原生自检的只读预检通过。该新私有探针尚未编译或运行，不能声称性能收益，后续需等游戏与基准结束后再构建和采样。

用户随后明确要求不要开启计时器或计数。新建的每小时 FPS 跟进已立即删除；scale 1.0 空闲等待器 PID 33284 在 runs=[]、无游戏子进程时停止，状态标记 stopped_before_probe_no_timers_requested。未启动 1.0 关卡测试，用户 Render scale 仍为 0.7。私有逐眼源码保留作离线材料，未编译、未运行；不会自动继续计数或测游戏。

## 模拟器提高到 AVP 总像素量近似档（2026-10-22）

用户要求模拟器优先采用 AVP 级分辨率。Apple 官方仅给双眼合计约 2300 万像素，未给可核验的单眼宽高，因此采用每眼 3600×3200、双眼 2304 万像素作为近似值；这不是 AVP 光学、视场或设备仿真。模拟器源码允许自定义每眼最高 4096×4096，并将该值报告为 OpenXR 推荐渲染尺寸。当前手动模拟器设置 `%LOCALAPPDATA%\OpenXR-Simulator\settings.json` 已从每眼 2560×2560 调至 3600×3200；Quest 3 视场/IPD、桌面预览窗口大小、预览帧率及 `show_stats=false` 保持不变。原始设置备份与新独立配置分别保存在忽略目录 `analysis/runtime/simulator-user-settings-before-avp-pixels-20261022.json` 和 `analysis/runtime/simulator-avp-pixelcount-settings-20261022.json`。

正常启动器配置仍为 Render scale 0.7，以免改动用户真机画质；另外准备仅供之后模拟器测试使用的 `analysis/runtime/launcher-avp-scale10-config-20261022.json`，与用户配置相比仅 `render_scale=1.0`。0.7 时按推荐尺寸计算约为每眼 2520×2240，1.0 时目标为每眼 3600×3200；桌面预览依旧缩放到原窗口尺寸。已静态核对模拟器尺寸边界、配置加载和 OpenXR 报告路径，并回读配置；没有启动游戏、后台等待器、计时或帧数采样，实际新配置生效及 FPS 尚未通过运行验证。

随后对当前私有模拟器运行时单独调用 OpenXR 渲染尺寸查询（未创建游戏会话）：它实际返回推荐每眼 3600×3200；Render scale 1.0 规划每眼 3600×3200、双眼并排源 7200×3200，0.7 规划每眼 2520×2240。查询成功仅确认运行时尺寸报告和启动器规划，不证明实际游戏 GPU 图像或 FPS。桌面预览仍为 2744×1107 的双眼缩小镜像，因此窗口观感不能直接代表源渲染分辨率；没有改动预览布局或用户正常启动器的 0.7 设置。

默认启动器现对“本地 OpenXR 模拟器”单独使用 Render scale 1.0；切换回 VDXR／系统 Runtime 时仍显示并使用原有的 0.7。模拟器的“检查单眼尺寸”和“启动 VR”从当前用户配置生成临时配置，仅替换 `render_scale`，会话结束后删除，不写回真机缩放。C# 启动器自检通过；当前模拟器 Runtime 的只读尺寸查询返回每眼 3600×3200、双眼源 7200×3200。默认入口 `GearsVR Launcher.exe` 已更新，SHA256 `c2356b5d25b0763853f976276aee9bf460a05e243771197b0d71a89e4e3e4315`。没有启动游戏、计时器或新的帧率采样；实际游戏输出及高分辨率帧率仍待许可后验证。桌面预览窗口保持 2744×1107，仍会对源画面缩小显示。

## 狙击枪持枪时第一人称失效修复（2026-10-22）

用户确认“第一人称换到狙击枪退回第三人称，第三人称持枪按 E＋R 不能进入第一人称”。根因是当前预览无条件排除整个 Weap_SniperRifle 类族。工作副本 Engine.u / WarfareGame.u 与原始 Steam 包哈希一致，未发现旧第三方第一人称替换包残留。复用知识库中保存的 UseFirstPersonCamera / SetZoomed 脚本后，改为读取并认证当前武器的实际 bZoomed：未开镜可预览，实际开镜保留原版镜内相机且不叠加镜头拉近，收镜恢复用户选择。

从最新 MountedResume2 冻结源增量构建，仅六份相关文件变化，保留已集成的机枪恢复和狙击镜材质路径。37 项原生／启动前检查、485 项编译依赖及源码哈希核对、109 项启动器 Python 回归通过；新增用例覆盖用户两种操作、开关镜、换枪竞态与错误／失效属性。已接入普通启动器：build-sniper-first-person1/GearsCaptureSniperFirstPerson1.dll，SHA256 9a43860f0adaa047314f4b676683c15ada1ef5027662a75f241361c37bbdde75；仅更新三个 native 字段，用户配置不变，新游戏进程生效。本次未新增实际游戏、GPU 狙击枪画面或实体头显验收，不构成发布。见[修复与验证记录](../../sniper-first-person-20261022.md)。

## 命令列表状态查询私有候选（2026-10-22）

继续追查 120 个新立体帧的 CPU 路径时确认：`gpu_viewport_stats::Tracker::find` 在每次绑定、画笔状态或 Draw 查询时最多顺序检查 1024 个命令列表槽。既有 4K/0.7 室外关卡回执记录首 120 ticket 的 Draw 中位约 1600 次、目标绑定表 `list_highwater=389`；这只证明热路径调用量，不是可换算的 FPS 收益。私有候选改为仅扫描已用槽前缀，并在原互斥锁内缓存最近 64 个列表的槽**指针**。查询仍读取槽中最新状态，Reset、绑定更新、retire、clear 与列表地址复用不借用旧状态；未改变原始 Draw、全局帧锁或 Reset/Execute/Present 顺序。

起初基于 MountedResume2 的候选在编译前发现普通启动器已切换到狙击枪第一人称新 DLL，哈希门槛安全拒绝。随后从最新 `analysis/sniper-first-person1-source/` 冻结源重建私有候选 `analysis/fps-viewport-cache-sniper-first-person1-source/`，497 个文件只有 `gpu_viewport_stats.h` 及其测试改变；私有 DLL `analysis/native-build-fps-viewport-cache-sniper-first-person1/GearsCaptureFpsViewportCache.dll` SHA256 `56afc5f2064d80aa139f72e1a376fe25ef744a0209b296505df17687b8c05fbb`，485 项编译依赖核对通过。`GpuViewportStatsTest`、原生基础／viewport／geometry／quad 四项自检通过，新增测试覆盖缓存冲突、更新、Reset、retire 后槽复用、clear 后新建。未启动游戏、计时器或帧数采样；因此**没有 FPS 收益证据，不接入普通启动器**，正式 DLL 与用户配置保持原状。

在同一私有候选上继续合并每次 Draw 的 owned target 绑定、绘制状态和粘性描述符丢失标志读取，传递同次值快照给 owned scene 判定，保留现有外层全局锁、原始 Draw 及 Reset/Execute/Present 顺序。新构建 `analysis/native-build-fps-viewport-snapshot-sniper-first-person1/GearsCaptureFpsViewportSnapshot.dll` SHA256 `0633fb44a05eed5f658b9d7275aa6667a6a7d59c13b5327930347c0dbc696b4c`；497 源文件中仅 `capture.cpp`、`gpu_viewport_stats.h`、`gpu_viewport_stats_test.cpp`、`native_owned_scene.inl` 变化，485 项编译依赖核对通过。新增快照测试检查未创建、更新、值拷贝、retire、Reset、clear；原生基础／viewport／geometry／quad 与完整 `GpuViewportStatsTest` 均通过。该私有版本尚未运行游戏或验证 FPS，也未替换默认 DLL；自检仅证明覆盖的状态语义。

另用固定的 30,000 步离线状态序列对冻结默认头文件与私有候选逐步差分：Reset、retire、绑定、Draw、描述符复制／拒绝、clear、查询及快照的严格／旧模式状态摘要相同，回执 `analysis/runtime/fps-viewport-differential/receipt.json`。此检查不含并发交错、实际游戏和性能计时，不能据此宣称 120 FPS 或集成候选。

Astra Max 复核后，将达到 120 所需的大幅 CPU 缩减优先放在逐眼命令生成是否可分离上，而不是继续假设局部缓存能跨越完整预算。已用现有知识库和历史运行时代码快照做有界静态核对：`original_consume` 仍一次消费整个 renderer；历史逐眼调用点在几何函数前通过 `sub_140173170` 写入线程局部 render context 的 `+0x14c0` 当前视图，几何函数随后读同一 TLS 链并能调用间接虚函数。冻结 Mod 的当前眼观察路径也读取该 TLS 偏移。因此直接把右眼 `original_geometry` 送到另一线程会失去当前眼上下文，不能作为安全的提帧候选。进一步复核此前**已验收**的 4K/0.7 模拟器室外重场景：该 hook 的八组眼／阶段／模式各调用 5053 次，但直接 Draw 均为零；另外的真正绘制路径在两眼分别有 4,864,014 / 3,303,606 次普通视口直接 Draw。零 Draw 不等于零 CPU，但说明上述 hook 不是重场景 Draw 工作的并行切点。两条代表性 Draw 栈已通过现有 IDA 索引映射到历史游戏函数：索引绘制顶层 `0x1401ACD80`、非索引绘制顶层 `0x1401AF054`，向上共经 `0x14011908C` 和原始消费入口；这才是后续切点调查范围，栈快照不构成完整调用图。可复核脚本与回执为 `analysis/runtime/audit-fps-eye-separability.py`、`fps-eye-separability-static-20261022.json`；其静态代码身份是历史 PID 16812 快照与当前零售磁盘 EXE 的匹配哈希，**没有证明当前运行时代码或全部传递调用相同**。下一步应为这条实际出 Draw 的逐眼路径建立输入、TLS、共享写入和资源代次的完整闭包，再判断可并行的 CPU 工作是否足以跨过 8.333 ms；本轮未开游戏、计时器或帧率计数。

Astra Max 按实际 Draw 栈继续复核后指出：历史游戏函数 `0x1400F14A4` 在条件成立时已把绘制区间送入 `0x1401695B4` 的原生任务槽；代表栈还经过任务取得／等待 `0x14016B49C` 和工作入口 `0x1401A847C`。因此“游戏尚未多线程”不是已证实根因。任务入口先设置 GS/TLS 渲染上下文，再调用实际绘制，结果经引用计数和父级归并；具体的命令列表、allocator、三项外部指针及任务间别名尚未闭合，不能直接把右眼或原始 Draw 移到另一线程，也不能仅凭任务栈证明并行耗时。私有有界审计 `analysis/runtime/fps-native-task-ownership-20261022.md` 记录任务载荷、上下文、join 与缺口。另比对当前零售磁盘 EXE 与历史运行时代码段：该 PE 经过封装，所选 11 个函数入口的磁盘原始字节均与历史运行时快照不同，部分虚拟代码没有磁盘原始字节；回执 `analysis/runtime/fps-native-task-bytes-20261022.json`。**同一磁盘 EXE 哈希不能证明当前运行时函数字节等于历史快照**。仅离线读文件，未启动游戏或 FPS 采样。

另基于正式版历史室外状态 `frame_link` 列表高水位 300、全会话约 140 万次 Reset，准备帧关联查找私有候选：在原互斥锁内以 2048 槽的直接映射索引缓存最近列表位置，每次命中仍核对身份、已用前缀、有效位、代次和世界 epoch；Reset/Create 更新索引，Present/回收后的旧索引因前缀或槽身份检查失效。首个 SniperFirstPerson1 冻结源候选在构建前遇到普通启动器升级，哈希门槛安全拒绝。确认新版 `ClassDiscovery1` 冻结源的两份目标文件与旧基线逐字节一致后，移植为 `analysis/fps-frame-link-cache-class-discovery1-source/`；500 源文件仅 `frame_link.h` 和其测试改变。私有 DLL `analysis/native-build-fps-frame-link-cache-class-discovery1/GearsCaptureFpsFrameLinkCacheClassDiscovery1.dll` SHA256 `675289678490c68ddcd5614a49d5a53bf528fec8404f484dfc0e4fedec5c9751`，488 项编译依赖核对通过；`FrameLinkTest` 39,195 项检查以及基础／frame-link／continuous／viewport／geometry／quad 原生自检通过，新增覆盖缓存冲突、代次失效、世界切换、Present、回收和重用。**尚无关卡 FPS 对照，不接入默认启动器，也不能据此推断 120 FPS 收益**；本轮没有开游戏、定时器或帧率计数。

随后从历史运行时 IDA 数据库的**私有副本**做有界函数核对并关闭数据库：实际 indexed Draw 包装函数 `0x1401ACD80` 取由上游 GS/TLS `+32` 传入的渲染上下文，经上下文 `+4732 -> +20` 的虚函数槽 `+104` 发出调用；非索引包装函数同一路径使用槽 `+96`。任务入口取得池化上下文并安装到线程 TLS，结束时归还；这支持继续检查任务是否各持独立命令列表，却尚未证明内部列表／allocator 绝不别名。选择性 IDA 导出 `analysis/runtime/fps-native-task-draw-context-20261022.json`，无整段游戏伪代码外传。另对旧的探索性消费线程 783 条栈样本离线分组：180 条曾被归为游戏 `0x16B595` 等待，实际该地址属于**通用**等待辅助函数 `0x14016B530`；其中 59 条栈含绘制分派 `0x1400F14A4`，28 条明确含绘制任务 join `0x14016B49C -> 0x1401695B4`。回执 `analysis/runtime/fps-wait-stack-routing-offline-20261022.json`；瞬时、部分展开的栈次数不能换算成每帧耗时，也不能把全部 180 条都说成绘制任务等待。后续若试命令列表分片锁，先证明同一列表的 Reset/Draw/Execute/Present 代次顺序与跨任务列表/allocator 唯一性；本轮仍未启动游戏、计时或 FPS 采样，普通启动器不变。

进一步核对历史任务结果交接：工作线程完成绘制后从其 TLS 上下文的 `+4716` 生成引用计数结果，父线程完成 join 后才把各任务槽的结果移动并归并到父上下文 `+4724` 所指向的所有者结果存储（`+4724` 本身是指针）；这证明保存代码中的**结果交接顺序**，仍不证明同时工作的命令列表／allocator 唯一。Astra Max 据此建议仅考虑经过严格认证的普通工作线程 indexed Draw 快路，但复核 ClassDiscovery1 默认构建的四项相关开关均为 ON（owned UI batch、owned UI material、UI material route、owned depth-clear）；`hook_draw_indexed` 在最终 `filtered` 统计之前仍可能更新双眼／UI／传输证明或触发拒绝，因此不能拿旧 filtered 总数当可放行绘制数。快路所需的副作用等价、列表代次／绑定／关注资源凭据、跨 Reset/Execute/Present 的撤销排空及重入顺序尚未闭合，当前拒绝改动运行时锁。详细边界记于忽略的 `analysis/runtime/fps-native-task-ownership-20261022.md`。此轮仅离线查证及评审，未构建、运行游戏、计时或计数；普通启动器和用户配置不变，AVP 近似档 scale 1.0 的真实关卡 FPS 仍未测量。

在上述审计期间，另一工作流将普通启动器集成至 WorldWake2（DLL SHA256 `180595f8969ad6c2ddab931997e827204374a2bba83d9c29a0294d4d0c36870b`）；本轮只读核对新清单哈希匹配、四项相关构建开关仍为 ON，并以两份冻结源码直接 diff 确认 `capture.cpp` 的 Draw／Reset／Execute／Present／帧锁钩子没有变化，新增改动集中在世界控制等待。锁快路的风险判断仍适用于这些未变的钩子；世界切换协议则须以 WorldWake2 为准。没有将任何旧私有 FPS 候选覆盖新默认版。

沿历史任务上下文继续有界核对后，找到更具体的 D3D12 对象链：任务取得池化 TLS 上下文时，在 `context+4716` 的嵌套状态中取得或构建一个引用计数对象，并将其置于 `context+4732`；对象 `+12`、`+20` 的虚函数槽依次与本机 Microsoft D3D12 SDK 中的 allocator Reset、命令列表 Reset／Close／DrawInstanced／DrawIndexedInstanced 位置一致。任务完成后当前对象被清出，结果在父线程 join 后归并。由此可高可信度识别实际 Draw 使用的列表接口与 allocator 存放位置，下一步缺口缩小为**并发任务是否可能经池化或引用转移取得同一底层列表／allocator**，以及当前零售运行时代码身份。选择性 IDA 回执 `analysis/runtime/fps-task-command-object-20261022.json` 和详细说明 `analysis/runtime/fps-native-task-ownership-20261022.md` 保留了历史快照哈希与 SDK 头文件哈希；私有 IDA lease 已关闭。这个静态结论仍不能允许直接拆全局锁；本轮未改默认 DLL、未运行游戏、计时或计数。

随后另一工作流又将默认启动器更新为 WorldPrepare1（DLL SHA256 `7eb03a5b87e96ca39a482a599d56e840e62c0c99af36fc0cd3c472a654bf2906`）；本轮只读验证清单哈希、WorldWake2 父源和源码差异。`capture.cpp` 仅增加世界准备时间戳请求状态及输出，Draw／Reset／Execute／Present／全局帧锁钩子仍未变，四项相关构建开关仍为 ON；世界准备协议须以后者冻结源为准。未用 FPS 私有构建替换默认版。

继续在历史 PID 16812 运行时代码快照中追查原生命令列表寿命：列表关闭后，七个记录数量字段之和为零时才放回任务上下文的本地池；有记录的列表作为结果交给提交线程。后者从批次组装 D3D12 命令列表并调用队列的 ExecuteCommandLists，再把批次放进待回收链。帧末路径读取 D3D12 fence 的已完成值，仅处理达到该批次记录值的条目；后续汇编复核证明此处把批次对象指针转移至管理器公共池、清空原数组，再回收批次，并非释放同一批对象。这为“提交后不能立刻复用列表”提供了具体代码证据，也缩小了锁优化需要验证的对象寿命范围。完整证据与边界见忽略目录 `analysis/runtime/fps-native-task-ownership-20261022.md`。当时管理器公共池的插入路径尚未定位，后续段落已补齐这一条；跨线程底层列表／allocator 别名及当前实际运行时代码仍未证明，不能据此拆分 Mod 的 Draw 全局锁或声称提帧。私有 IDA lease 已关闭；未启动游戏、计时器或计数，未替换默认 DLL 或改变用户配置。

随后从已校验六节的历史数据恢复 IDA 库复制私有副本，读到该命令对象实际虚表 `0x141668948`；首个虚函数指向释放包装，再调用资源析构函数，释放对象 `+12` 的 allocator、`+20` 的命令列表及其他资源，并可释放对象内存。这个析构函数没有向管理器公共池插入对象，因此公共池若被使用，生产者在别处。已关闭私有数据库 lease，原知识库未改；这个额外静态线索仍不能证明无提前入池／底层资源别名，也不是当前运行时或 120 FPS 验收。本轮没有启动游戏、计时或计数。

继续在同一历史运行时快照的私有 IDA 副本中找到公共池生产者：上下文 `+4724` 指向管理器；管理器 `+32` 为命令对象池、`+40` 为数量、`+56` 为取对象时使用的锁。`0x14021E810` 在批次通过 fence 完成检查后，将批次 `+0x90` 的命令对象指针按数量转入该池并清空来源。这修正了此前“生产者未找到”的缺口，说明已提交批次的一条回池路径经过 GPU 完成门槛。但该转入代码中未见管理器 `+56` 锁，外层线程时序／同步、其他入池路径、底层列表／allocator 别名与当前真正在跑的代码身份仍需证明；不能凭历史静态代码拆 Mod 的 Draw 全局锁，也没有得到任何新 FPS。证据和地址见忽略目录 `analysis/runtime/fps-native-task-ownership-20261022.md`；私有 IDA lease 已关闭，原数据库未改。本轮没有启动游戏、计时器或帧率计数，没有替换正式 DLL 或改变用户设置。

再按用户此前要求请 Astra Max 对重场景 CPU 差额给出有界建议：历史正式版消费线程约 11.849 ms/新立体帧，要进入 120 Hz 的 8.333 ms 预算，至少须缩短约 3.516 ms（约 30%），仍不保证 GPU／提交节奏。Astra 建议优先查双眼是否重复计算可复用的眼位无关准备结果；Draw 锁等待是墙钟值，不能直接当成 CPU 节省，且当前 `original_consume` 对双眼 renderer 只调用一次。按知识库 artifact 1 的历史函数哈希只读核对 `0x14015A80C → 0x14011908C → 0x1401FC060/0x14013CFCC`：上层 `0x1401FC060` 在视图循环前只调用一次；视图循环按 `0xD30` 步长逐视图设置 TLS 并调用 `0x14013CFCC`，后者读取视图／阶段数据、测试可见性、写回视图状态且调用间接虚函数。当前观察器的可见性哈希也只预览 16 字节，不能证明两眼全量输入相同。**拒绝整函数双眼复用**；尚无输入输出及副作用闭合的更小可复用子结果。证据与限制见忽略目录 `analysis/runtime/fps-eye-reuse-static-20261022.md`。这轮未修改运行时锁或正式版、未启动游戏／计时器／帧率计数；AVP 近似分辨率 scale 1.0 仍未做关卡 FPS 验收。

进一步只读缩小逐视图准备入口：`0x14013CFCC` 连续调用的十个准备辅助函数均传入当前视图指针和视图字段／结果偏移，源偏移还随阶段变化；这个调用点没有直接暴露可安全跨眼复用的结果，不能凭两眼视图的短哈希或同一模型资源复用整个准备序列。另查本机静态拓扑：i5‑12600KF 为 6 个双逻辑线程高性能核心（掩码 `0x0FFF`）及 4 个单逻辑线程效率核心（`0xF000`），显卡为 RTX 5090 D，当前 Windows 电源方案为“高性能”。这仅说明未来可以核对消费线程是否被调度到效率核心；既有回执没有 CPU 核落点证据，**不能把大小核调度当成当前低帧率根因**，也未修改进程亲和性。默认 DLL 哈希仍与清单的 `7eb03a5b…4bf2906` 相符。此次只编译运行仓库已有的静态拓扑查询工具，未运行游戏、计时器或帧率计数。

补核历史资料后更正大小核线索：此前已经在 4K 桌面、Render scale 0.25、120 Hz 模拟器关卡做过正式版 A／仅 P 核／正式版 B 夹测，完整窗口新立体 FPS 为 101.067／104.337／102.465；仅 P 核那局首 120 ticket 的 Draw 中位比两端默认少约 7.1%，因此小幅帧率差不能归因于核心绑定，也不应重复把它当作新候选。详见 `docs/157-vr-fps-hot-path.md` 的原始回执。

又把现有 4K／0.7 重场景探索性消费线程快照的非等待游戏 RVA 映射到历史运行时代码节的函数块：频繁出现的地址分别落在命令对象取得／Reset（43 个瞬时样本）、索引 Draw 包装（31 个）、另一条索引 Draw 路径（27 个）、命令列表关闭／结果转移（18 个）及帧提交外围（15 个）。映射脚本与身份／限制记录在 `analysis/runtime/map-fps-nonwait-stack-rvas.py`、`fps-nonwait-stack-static-20261022.md`。该旧会话没有通过严格 EndSession 验收，堆栈全部为部分展开；样本次数不能换算成每帧耗时，历史函数 RVA 也不认证当前运行时代码。Astra Max 复核认为现有离线证据尚找不到安全、足以节省至少约 3.516 ms／ticket 的修改，因此不据此拆全局 Draw 锁或把私有缓存候选接入默认启动器。普通启动器与用户配置没有改动；本轮没有启动游戏、计时器或新的帧率计数，AVP 近似档 scale 1.0 的真实关卡 120 新立体 FPS 仍未验收。

再次按 120 新立体 FPS 的实际预算筛选旧候选，并让 Astra Max 复核遗漏：既有 4K／0.7 室外锁调用点诊断显示，普通 `CopyDescriptors` 消费线程等锁约 0.075 ms／ticket、所有线程持锁约 0.022 ms／ticket；撤回把它当下一项高收益候选的建议。旧的细粒度 QPC 轻量化夹测在同一类室外关卡为默认 A／候选／默认 B 完整窗口 62.330／62.068／64.062 FPS，已拒绝。4K→720p 游戏桌面输出对照为 60.487→59.422 FPS，亦不支持通过降低桌面像素数达到目标。当前较大的 Reset／Execute 锁交集是跨线程等待的墙钟时间，不能算成可安全删除的消费线程 CPU；旧正式版重场景消费 CPU 约 11.849 ms／ticket，进入 120 Hz 的 8.333 ms 预算至少需约 3.516 ms／ticket 的真实减负，尚无满足代次／observer 语义的已证实切点。这些旧回执的构建与当前 WorldPrepare1 不同，诊断值有插桩扰动，不能当作当前版本验收或外推至 AVP 近似档 scale 1.0。本轮只读复核、没有启动游戏、定时任务或新的计时／帧率计数；默认启动器未被 FPS 候选替换。

对 CPU 预算再核验其时间窗：上述约 11.849 ms／ticket 来自室外正式版会话**最初 120 个**已归属 ticket 的同线程 `GetThreadTimes` 汇总，而整局有 5052 个 owned ticket、约 64 秒玩法窗口。该会话回执有 60 个玩法 FPS 样本，却没有与这 60 秒逐窗匹配的消费线程 CPU 汇总；因此 3.516 ms／ticket 是“若这段工作量持续到稳态，要达到 120 Hz 必须减少的串行 CPU 量”，不是整局或 AVP 近似档 scale 1.0 已证实的定量瓶颈。此前 0.25 轻场景的整窗线程资料不能代替 0.7 重场景。下一项真正能改变方案选择的证据，应是同一重场景稳定玩法窗口内的消费线程 CPU、实际新立体帧及锁等待匹配；用户当前说不要开计时器／计数，已发出范围澄清，本轮继续按禁止新测量处理。没有据不完整时间窗修改正式 DLL。

静态核查发现历史游戏启动代码读取 `PARALLELRENDERING=`，把值写入一个被逐眼准备辅助函数和 Draw 分发器读取的全局开关；当值为 1 且工作项多于一个时，这些路径可进入原生任务分支。进一步核验哈希匹配的历史机器码：解析前明确将默认值设为 1，参数未匹配时解析器不改此值，因此“启动器没传参数，所以没开并行”的猜测不成立于这份历史代码。上溯一个调度器初始化层级又找到 `nofastasync` 参数及两种调度器对象选择；普通启动链也没有传它。任务提交和等待仍可能在当前线程内执行，静态分支不能证明实际跨核并行程度。历史快照未保存开关所在内存页，旧的部分栈也不能认证当前运行时值，所以没有改启动参数或正式 DLL、没有宣称帧率收益。地址、源码、快照身份与限制见忽略目录 `analysis/runtime/fps-parallel-rendering-static-20261022.md`。本轮没有启动游戏、定时器或帧率计数；AVP 近似档 scale 1.0 仍待允许测量后验收。

对 AVP 近似模拟器画质作了有界静态核对：当前每眼推荐尺寸仍为 3600×3200，普通启动器 scale 0.7、测试专用配置 scale 1.0，桌面双眼预览只有 2744×1107。对私有模拟器 DLL 的 IDA 副本只检查 `full_render` 读取及直接使用，发现 `false` 让合成预览按应用提交的有效 `imageRect` 裁剪，`true` 用整张源纹理；未见它改变 OpenXR 推荐尺寸或应用的 swapchain 请求。预览观感偏低的直接原因是大幅缩小到窗口，并不证明源图只有这个分辨率；改 `full_render` 没有已证实的画质／FPS 收益。证据与 DLL 哈希见忽略目录 `analysis/runtime/fps-simulator-full-render-static-20261022.md`，IDA lease 已关闭。没有启动游戏或新计时／帧率计数；scale 1.0 的实际 GPU 图像仍待验证。

从 WorldPrepare1 冻结源码另建私有 FPS 候选，针对旧 4K／0.7 室外探索性锁交集里靠前的 Reset 跟踪路径，为帧链接、视口状态及附件绑定的列表身份查找加可验证提示缓存与已用前缀扫描；原始锁、D3D12 Reset、Reset／Execute／Present 顺序保持。碰撞、列表退休／复用、代次与世界 epoch 回退到原扫描。Release 构建使用冻结默认的编译开关，`FrameLinkTest`、视口统计、附件跟踪碰撞／复用及 DLL base／frame-link／quad 自检通过。复核历史资料时发现：本轮一度加入的“Reset 只清列表头、不清 32 条事件记录”早有独立玩法试验，在较轻 4K／0.25 场景默认 A **114.412**、候选 **111.043 FPS**，候选首 120 ticket 绘制中位数还较低（329 对 371）；默认 B 被取消，不能精确归因，但这是负面信号。现已撤销该重复改动，保留原来的事件清零语义；重编私有 DLL SHA256 `69e5c2f6990bf6b353ffd7bda4cd71b8f11cfe4b08e592821e00eb3504037d6b`，离线自检复过。旧探索性重插桩会话没有通过严格设置恢复，其墙钟锁交集也不是本候选可省的 CPU 时间；因此这些离线检查**不代表帧率收益**。普通启动器仍指向 WorldPrepare1（SHA256 `7eb03a5b…4bf2906`），没有集成私有候选、启动游戏、计时或帧数采样。AVP 近似档每眼 3600×3200、scale 1.0 的真实关卡与实际 GPU 图像仍待用户所禁计时／计数范围明确后验收。详细源差异、回执及限制在忽略目录 `analysis/runtime/fps-frame-index-private-20261022.md`。

按用户要求再次请 Astra Max 复核 120 FPS 的高收益切点，并直接审计 WorldPrepare1 的普通 indexed Draw 路径：当前构建的 UI 材质路由、批处理、输出跟踪和相关证明仍启用；Draw 在原始驱动调用前可能消费 UI 序号、重放多次绘制、故障拒绝或即时更新 `frame_link` 全局代次／最大 ticket，随后还有输出、视口、几何和引擎观察。旧 `filtered` 计数发生在这些前置副作用之后，不能拿它或列表身份直接划出可无锁转发的大量普通 Draw。历史原生任务快照也未证明当前零售运行时的底层命令列表／allocator 独占和完整池同步，故不实现 per-list 锁分片或延迟 observer。Astra 指出既有 **11.849 ms/ticket** 只属于旧构建首 120 ticket，未与整段稳定玩法匹配，所谓还差约 3.5 ms CPU 是条件估算；当前 WorldPrepare1、AVP 近似每眼 3600×3200、scale 1.0 的稳态数据仍为空。若用户重新允许计时／计数，下一项决定性证据应是一局固定重场景里同窗的消费线程 CPU 增量、新立体 ticket 增量及严格会话／设置恢复，而不是再从旧部分栈次数推算毫秒收益。本轮只读审计与保存证据，未启动游戏或采样；详细分支和边界见忽略目录 `analysis/runtime/fps-draw-fastpath-closure-20261022.md`。

继续核对历史原生任务的命令列表生命周期：保存的知识库里，共享命令对象池的直接取用点不只有 TLS 绘制上下文，提交准备函数 `0x1401AA600` 也会直接取用或经本地池助手取用，写进待提交批次、调用图形列表方法并 Close，之后提交线程才 Execute。这说明拟议的逐列表 Draw 锁不能只证明各绘制工作线程的列表独占，还须覆盖提交路径和回池顺序；历史反编译仍有缺口，当前零售游戏的运行时代码身份未认证，故未拆锁或宣称收益。详细证据及限制补在忽略目录 `analysis/runtime/fps-native-task-ownership-20261022.md`。此轮没有运行游戏、计时器或帧数采样。

为 AVP 近似分辨率下的 120 FPS 验收复核模拟器节奏：默认 GUI 的 quiet2 模拟器 OpenXR `xrWaitFrame` 固定 90 Hz，故虽已提高到每眼 3600×3200 并对模拟器采用 scale 1.0，它不能作为 120 新立体 FPS 的验收 Runtime；`preview_fps=90` 只限制桌面镜像。已有独立测试 Runtime 在设置 `GEARSVR_SIM_REFRESH_HZ=120` 时源码选择 120 Hz，二进制哈希与既有来源回执吻合。现为它单独复制 AVP 像素配置至忽略目录，并用 `GEARSVR_SIM_DATA_PATH` 隔离用户模拟器设置；只读 `render-size` 返回每眼 3600×3200、双眼源 7200×3200。未进入 `xrWaitFrame`、启动游戏或帧数采样，120 Hz 实际节奏、GPU 图像及游戏 FPS 均未验收；默认 GUI 仍用 quiet2。Astra Max 再次有界复核冻结源码，排除“多余第三份桌面世界渲染”的猜测，也未找到有完整语义闭包及 ≥3 ms/ticket 预算依据的高收益补丁。证据、哈希及后续门槛见 `analysis/runtime/fps-avp120-simulator-profile-20261022.md`。

用户将当前 FPS 目标收敛为 AVP 近似模拟器重场景的每秒 90 个不同的新立体帧，并明确只禁定时任务／无人值守循环，允许单次手动 FPS 测量。本轮没有注册或启动定时任务。一次性 4K 桌面、每眼 3600×3200、Render scale 1.0、90 Hz 模拟器真实零售关卡，在冻结 WorldProfile1 DLL SHA256 01d2823fcb4d6ad87ade125e91497037c2bdb7d238f54104644047ae87b6d6a1 上取得 60 个有效玩法样本：样本均值 62.342 FPS，前后 30 样本 62.498／62.186；完整 62.504 秒窗口新帧差分 62.252 FPS，XR 循环均值 89.828 Hz，实际 GPU 双眼图 7200×3200。双眼会话、原生停机、独立钩子入口恢复、游戏退出、用户画质设置恢复与无并行游戏均通过。同步只读 GetThreadTimes 的 56 个玩法间隔显示消费线程占一个逻辑核约 0.608、游戏合计约 2.501 核；匹配的 57 个 NVML 整卡样本 GPU 利用率中位 67%、最高 70%、功率中位约 262.6 W。这些使用率不能单独证明帧关键路径；90 FPS 仍未达到，私有 FPS 候选未接入默认版，模拟器结果不是头显实测。回执见忽略目录 analysis/runtime/fps-avp90-load-posthoc-audit-20261022.json。

这一局的原始单次脚本末尾误把模拟器自行保存到私有副本的预览窗口高度 1107→1068 当作渲染配置变化，故原状态仍记录 error；独立补审只接受这一个私有预览字段变化，保留原状态及故障事实，不改用户模拟器配置。探针的后续校验现用启动前冻结哈希，且继续核对每眼尺寸。较早一局 WorldPrepare2 60 样本约 59.476 FPS 的停机校验也因启动前钩子字节分存两份状态、以及未携带冻结 hook-entry-manifest 而误判；已按同一进程／owner 身份保存两份原始证据，79 个停机入口中的 72 个匹配固定或磁盘字节，另 7 个独立读回 SHA256 匹配安装前密封字节，原报告保持 incomplete，补审见 analysis/runtime/fps-avp90-posthoc-audit-20261022.json。独立校验器与连续会话驱动现保存并交叉核对 prime-status 和 hook-entry-manifest，针对预存第三方 DXGI 跳转要求独立停机读回一致；相应 27／60 项 Python 检查通过。后台游戏只在用户已有窗口之后启动，静音、输出只观察、禁物理手柄后台输入，未覆盖并行游戏。

## AVP 近似档稳态锁计时（2026-10-22）

用户澄清仅禁止定时任务，允许单次 FPS 测量；本轮未注册任何定时任务。以冻结 WorldProfile1 正式版为基线，在忽略目录制作只增加 QPC 采样的私有诊断 DLL `GearsCaptureAvp90Timing.dll`，SHA256 `82adcff80f2b7b6a7f445fa1504ea03b2fab8f69b6fc202682cf62b1372ef6e4`，24 项启动前检查通过。第一次启动前缺少 `GraphicsMenuLockTest.exe` 被门槛拒绝，游戏没有启动；修复私有构建后才进行一次零售游戏真实关卡采样。每眼 3600×3200、scale 1.0、90Hz、4K 桌面下取得 60 个有效玩法样本，诊断构建约 59.235 新立体 FPS，OpenXR 循环 89.837Hz，实际双眼图 7200×3200；会话、设置恢复和游戏退出通过。诊断 FPS 有插桩开销，不能与正式版 62.252 FPS 直接当作优化对照。55 个同步玩法线程间隔显示消费线程约占 0.580 个逻辑核，整卡 GPU 利用率中位 68%。四段各 30 个有效 ticket 的原始消费墙钟均值为 14.483／17.407／21.420／14.660ms，其中 `frame_lock()` 获取锁的墙钟均值 5.153／6.359／7.571／5.136ms，约 1730 次获取／ticket；原始 Reset 转发仅约 0.47–0.78ms／ticket。`frame_lock` 数字目前混合递归获取、其他线程持锁和唤醒调度，不能全部称为争锁瓶颈；GPU 整卡利用率也不能证明具体关键路径。原始回执 `analysis/runtime/fps-avp90-once-20261022-153744-707685.status.json`，派生分析 `analysis/runtime/fps-avp90-once-20261022-153744-707685.status.timing-analysis.json`。

按 Astra Max 有界建议，正在忽略目录 `analysis/fps-avp90-holder-timing-source` 制作第二个私有探针，把所有 FrameHookMutex 获取区分外层／递归，按其他线程持锁期间的 Reset 前置、转发、后续、Execute 与其他阶段计算与消费线程等待的真实交集。固定 120 个抽样 ticket，无钩子内 I/O／事件大环形缓冲，保持原始锁与 D3D 调用顺序，并加入 CPU 自检。它尚未编译或运行；有人启动另一局游戏且外部 CPU 负载高，按并行排除规则等待，不与之争抢或打断。默认启动器、正式 DLL 和用户画质未因上述诊断改变；90 新立体 FPS 尚未达到，当前没有可集成的性能修复。

私有锁归因 DLL `analysis/native-build-avp90-holder-timing/GearsCaptureAvp90HolderTiming.dll` 已构建，SHA256 `fd3e2d342542f61d3fb8794210118b7d67cc4d175d89c324af91b22a15a4e101`；24 项常规自检通过，另单独执行 `--continuous` 的跨线程持锁／消费等待、外层／递归分类自检返回 0，回执 `analysis/runtime/fps-avp90-holder-timing-build.json` 与 `fps-avp90-holder-timing-selftest.json`。私有清单与当前普通清单仅三个 native 构建字段不同，哈希回读一致。尚未运行它的游戏关卡 FPS；随后发现另一款游戏仍在本机运行且持续占用多个 CPU 核，依用户要求不与其他游戏重叠。默认入口仍未替换。

随后静态复查发现 Execute 钩子在原始转发之前会递归持有第二层同一帧锁；第一份私有探针只在深度 1 接受阶段标记，会漏记 Execute 转发阶段。它未启动游戏，其先前通过的跨线程自检没有覆盖“递归层内阶段切换”。已修正私有源码，使持锁交集在递归阶段切换时记账，并扩充自检要求在消费线程等待期间同时观察 Reset 前置和递归层转发两个阶段。旧 `fd3e2d34…a4e101` 私有 DLL 标记为被新诊断取代，绝不作为玩法归因数据使用；新 DLL 尚待在其他游戏结束后重新构建、自检和单次测量。

在另一款游戏持续运行期间，对旧的探索性锁交集回执只读复核：0.7 缩放的首 128 ticket 上，消费线程约 6.166ms／ticket 的跨线程真实持锁交集中，Reset 跟踪、原始 Execute、原始 Reset 各约 1.329／1.145／1.048ms。进一步按旧私有源码行号核对，多个工作线程在 site 4020 的合计约 2.3ms／ticket 属于 **Reset 外层钩子**，不是 Draw；此前据行号将它归为 Draw 的判断已撤销。旧回执整局设置验收失败且采样窗口／构建／分辨率不同，不能当作新目标的 FPS 对照或收益承诺。修正版私有源码仍把 Draw、DrawIndexed、Targets、Barrier、Present 的外层持锁分别分类，以防未标记来源混入“其他”；目前没有证据证明 Draw 是 AVP 目标的主要瓶颈。分析脚本同步更新。504 个冻结源文件只在预期的 capture.cpp、owned_scene_gpu.inl、render_cost.h 三处不同，Python 语法检查通过。当时修正版尚未重编／运行；外部游戏 PID 21068 存活，未启动重叠 FPS 测量或创建定时任务。

修正版私有锁归因 DLL `analysis/native-build-avp90-holder-timing-b/GearsCaptureAvp90HolderTiming.dll` 已用 IDLE_PRIORITY_CLASS 低优先级完成构建，SHA256 `b3a369d98cdcba57c05727935a2bb5d84681c10006345751cb987d1617b2ad56`，25 项检查均通过，其中 `CaptureSelfTest --continuous` 覆盖消费等待期间另一线程递归层内从 Reset 前置切到转发阶段。构建回执 `analysis/runtime/fps-avp90-holder-timing-build-b.json`；私有清单 `analysis/runtime/manifest-avp90-holder-timing-b.json` 与正式清单只差 native 构建路径、DLL 名及哈希，正式启动器未替换。单次测量脚本补入本机已知另一款游戏 `sp26-cod.exe` 的排他检查；实际运行其前置检查返回 `skipped_busy`、退出码 2，未进入游戏、未改设置。外部游戏 PID 21068 仍存活，所以还没有新一轮 FPS／持锁交集数据。

Astra Max 复核了高分辨率目标的后续路线：先用上述修正版私有探针在无其他游戏的同一关卡量出消费线程等待与真实跨线程持锁的交集，尤其区分 Reset 的前置／原始转发／后续跟踪及 Execute；旧 0.7 探索回执只能提示方向，不能推算 1.0 的收益。只有证据表明可移除的关键路径足以接近 90 新立体 FPS，才考虑把录制期状态改为按真实 Reset 世代、Close／Execute 提交序号保留的不可变快照，并把消费线程状态隔离。原始 Reset／Execute 必须仍同步转发；成功／失败 Reset、地址复用、重复 Execute、跨 Present 提交、队列顺序和 GPU 资源退役都须逐项证明。正式候选还要用同关卡默认版夹测验证，插桩版 FPS 不可充当收益。当前仍在等待外部游戏退出，没有启动重复测试、定时任务或替换默认 DLL。

用户已澄清只禁用**定时任务**，单次 FPS 测量可以继续。为遵守“另一款游戏使用期间等待”，启动了隐藏的一次性等待进程 PID 33200，状态 `analysis/runtime/wait-avp90-holder-once-20261022.status.json`；它没有注册 Task Scheduler 或 Codex 自动化，最多启动一局，前置误遇忙碌可重新等待，限定 12 小时。20 次各含约五秒 CPU 采样且间隔十秒的稳定空闲检查完成后，才调用上述私有 DLL 的一次性 3600×3200／scale 1.0／90Hz 关卡探针。正式清单、私有清单和 DLL 哈希被固定；正式清单若在等待中更新，停止并要求重基，不以旧版诊断新默认。启动后的首轮回执显示外部 `sp26-cod.exe` 存活、CPU 约 41.52%、`stable_idle_checks=0`、`actual_game_runs=0`；本轮未开游戏也未改变用户设置。

静态复核当前冻结源的归因边界：`hook_reset_list` 在同一外层帧锁内完成前置、嵌套像素包装的原始 Reset 和成功／失败后的多份代次跟踪；`hook_execute` 同样在外层锁内准备提交成员、递归持锁转发原始 Execute 并提交各跟踪器结果；`hook_present` 的外层锁覆盖原始 DXGI Present 及前后关联。三个路径均可能影响消费线程，不能仅依据先前一个源码行号挑选 Draw 或把原始 API 转发单独移出锁。新探针分别记录这些阶段与消费线程等待的真实交集，采样后才选择拆分边界。17:06 状态确认一次性等待进程仍存活、游戏测试 0 局，外部 `sp26-cod.exe` 与另一局 `gowde-steam.exe` 均被排除。

检查单次测量脚本时发现：它的启动前检查已把已知外部游戏加入忙碌列表，但共享 FPS 驱动在运行期只将旧 Gears 可执行文件与第二个 Gears 实例判为重叠，若 `sp26-cod.exe` 在探针启动后出现，可能错误保留受干扰回执。现将运行期判定统一为“最多一个本轮零售 Gears 进程，其他已列入 `BUSY_IMAGES` 的进程一律污染本轮”；`analysis/runtime/test_idle_fps_pair_overlap.py` 两项单元测试覆盖单实例通过、外部游戏和第二实例拒绝，测试通过。修改的是本地忽略目录测试驱动，不影响正式启动器或在等待中的用户游戏；一次性等待进程仍未开局。

19:07:51 一次性等待器完成 20 次稳定空闲检查，但在启动器约 10 秒的预约自检期间，另一项 Playground 自动测试先于本轮启动零售游戏，导致 `launch_reserve.py` 以 `Existing game must exit normally first` 正确拒绝本轮开第二局。首轮 FPS 探针回执 `analysis/runtime/fps-avp90-once-20261022-190758-687585.status.json` 为 `probe_incomplete`：**0** 个玩法样本、无本轮游戏身份、原生未激活、没有可用 FPS；子进程记录另一任务 PID 424 和当时的游戏 PID 54296。画质设置测试前后 SHA256 都为 `8fbb442a…2285d6`，另以当前文件与本轮备份逐字节比对相同，未覆盖并行试玩。原等待器终止，没有重复开局，也不能把这次算作候选慢／快。

私有等待脚本现仅在精确预约拒绝、零样本、无本轮游戏出生身份、测试前后设置及备份字节一致时，把冲突归类为“开局前相撞”，重新走稳定空闲门槛；任何真实开局或设置差异仍立即停止。`analysis/runtime/test_wait_avp90_holder_collision.py` 的正反用例与共享重叠判定合计 3 项检查通过，真实失败回执也被分类器确认。唯一重试等待进程 PID 54608、状态 `analysis/runtime/wait-avp90-holder-once-20261022-retry.status.json` 已启动；没有定时任务，正式清单哈希仍为 `123ca1d…ab8ea`，正式 DLL 未改。它在后台等待其他测试结束，最多实际启动一局诊断。

19:38–19:41 修正版私有锁归因探针实际在零售游戏关卡运行，双眼 3600×3200、scale 1.0、模拟器 90 Hz、4K 桌面；取得 61 个玩法 FPS 样本与 120 个完整 native ticket，原生／XR 会话通过、游戏退出、用户画质恢复，诊断 DLL SHA256 `b3a369d9…17b2ad56`。但另一项 autoplay 工作 PID 58408 于游戏运行中加入，严格对照回执 `analysis/runtime/fps-avp90-once-20261022-193849-720139.status.json` 判为 `probe_incomplete`，约 57.951 FPS **不得与正式版对比或宣称优化收益**。仅作探索性归因的 `analysis/runtime/fps-avp90-once-20261022-193849-720139.status.holder-exploratory.json` 核对了四段各 30 ticket、全部在玩法窗口、无记录溢出／嵌套／锁深度错误；前 3 段每 ticket 的真实跨线程持锁交集为 4.683／4.550／4.664 ms（均值 4.632 ms），其中 Reset 原始转发约 1.03 ms、Reset 后续跟踪约 1.48 ms、Execute 原始转发约 1.25 ms；Draw／DrawIndexed 合计约 0.17 ms。第 4 段交集增至 5.926 ms，受并行工作污染风险更高。`analyze-avp90-holder-timing.py` 新增显式 `--exploratory` 门槛与 `comparison_valid=false` 输出，正式模式仍拒绝无效会话；Python 语法检查通过。原单次进程已终止，未自动重测。另启动唯一新的隐藏一次性等待进程 PID 12764、状态 `analysis/runtime/wait-avp90-holder-once-20261022-retry2.status.json`，继续等待无其他游戏／autoplay 和稳定低负载，至多再开一局；无定时任务，普通启动器仍未替换。

20:00–20:02 第二局私有锁归因探针在无其他游戏／autoplay 的零售关卡取得 60 个正值玩法样本和 120 个完整 native ticket，双眼 3600×3200、scale 1.0、模拟器 90 Hz、4K 桌面；游戏退出、画质备份与恢复逐字节相同，原生／XR 会话通过。不过整段玩法采样中夹有一次 `menu_mode=0`，原严格校验正确拒绝连续窗口，状态 `analysis/runtime/fps-avp90-once-20261022-200023-568213.status.json` 保留 `probe_incomplete`，约 57.784 FPS 的事后 52 样本稳定子窗 **不是正式版对照或优化收益**。显式 `comparison_valid=false` 的 `analysis/runtime/fps-avp90-once-20261022-200023-568213.status.holder-posthoc-subwindow.json` 只用于定位：四组各 30 ticket 的消费线程真实跨线程持锁交集为 5.000／5.011／5.692／4.677 ms；Reset 原始转发约 1.071–1.392 ms、Reset 后续跟踪约 1.460–1.687 ms、Execute 原始转发约 1.240–1.509 ms。与首次受并行 worker 污染的前三段约 4.632 ms 同量级，说明需要拆解 Reset 后续跟踪并核对命令列表依赖；诊断构建的 FPS 不可当作正式版收益。

按 Astra Max 的代次／命令列表所有权建议，已从冻结正式源码派生只在忽略目录中的 Reset 后续细分诊断版，记录 22 个阶段及持锁者与消费线程原始列表指针的相同／不同关系，保留原 Reset／Execute／Present 顺序和全局锁。私有 DLL `analysis/native-build-avp90-reset-followup/GearsCaptureAvp90ResetFollowup.dll` SHA256 `2347b43d…62a611b`，构建回执 `analysis/runtime/fps-avp90-reset-followup-build.json` 的 25 项离线检查全过；正式默认清单 SHA256 仍为 `123ca1d…ab8ea`，未集成诊断版。原始指针不同不证明 COM 对象不同，必须结合代次和命令提交顺序；诊断版开销也禁止用于宣称提速。

为避免一次菜单闪现吞掉整局，私有 AVP90 FPS 探针改为截取连续 60 个 `menu_mode=1`、原生数据新鲜、尺寸一致且新帧计数单调的玩法样本，并记录精确窗口；专用校验器对旧回执的合成连续窗口接受、对夹入菜单的窗口拒绝，语法检查通过。共享 FPS 校验器及正式启动器未动。私有一次性等待脚本 `analysis/runtime/wait-avp90-reset-followup-once-20261022.py` 已准备并通过语法、默认清单、私有清单及 DLL 哈希校验；它仅在约五分钟无并行游戏／autoplay、启动器空闲和低 CPU 后开最多一局后台静音诊断，不是定时任务。

20:11 一次性 Reset 细分诊断等待进程 PID 59216 已以隐藏窗口启动，状态 `analysis/runtime/wait-avp90-reset-followup-once-20261022.status.json`；首检 `waiting_for_idle`、`actual_game_runs=0`，没有注册任何定时任务。

20:20–20:22 私有 Reset 细分诊断完成唯一一局零售关卡；父状态 `analysis/runtime/wait-avp90-reset-followup-once-20261022.status.json` 为 `verified`，一次性进程已退出，没有定时任务。子状态 `analysis/runtime/fps-avp90-once-20261022-202023-159729.status.json`：双眼 3600×3200、scale 1.0、模拟器 90 Hz、4K 桌面，60 个连续有效玩法样本／60.419 秒，原生／XR 会话通过、无并行游戏或 autoplay、游戏退出、用户设置备份与恢复 SHA256 均为 `8fbb442a…2285d6`，DLL SHA256 `2347b43d…62a611b`；匹配 56 个整卡 GPU 样本中位 70%，消费线程整窗 CPU 37.781 秒／60.790 秒。计时插桩版测得 64.301 个新立体帧／秒；**不作为正式版提速证据**，正式版 62.252 FPS 基线和正式清单 SHA256 `123ca1d…ab8ea` 均未变。

细分归因 `analysis/runtime/fps-avp90-once-20261022-202023-159729.status.reset-followup-analysis.json` 有 120／120 个位于玩法窗内的 native ticket，四组各 30，记录溢出／嵌套／锁深度错误均为 0。消费线程每 ticket 的外层锁等待均值为 5.103／5.363／5.051／6.366 ms，其中真正与另一线程持锁重合为 4.422／4.654／4.415／5.475 ms。120 ticket 总体均值：Reset 原始调用阶段交集 1.045 ms，Reset 附件退休 0.259、owned scene 0.432、owned target 0.229、frame link 0.234、frame target 0.230 ms；Execute 原始调用阶段 1.226 ms，DrawIndexed 0.170、Target 0.225、Barrier 0.068 ms。Reset／Draw／Target／Barrier 被采样到的交集没有相同原始列表指针；不同原始指针可有 COM 别名，Execute 没有可用列表指针标签，所以**不能据此直接移出全局锁**。原分析器字段 `comparison_valid=true` 容易误解为插桩 FPS 对比有效；已把私有分析器未来输出改为 `diagnostic_valid` 与 `fps_comparison_valid=false`，旧文件保留原样，按其 limits 和本段解释使用。下一步先证明列表代次、Reset／Execute／Present 的提交顺序与 GPU 退休，再做可夹测的优化候选。

20:40 将 Astra Max 的并发审查结论用于私有 `analysis/fps-avp90-recording-split-source`：`frame_link` 的待提交证明由整局全局 revision 改成世界／输出代次、提交前状态及每个实际成员列表的记录 revision。不同列表的 Reset、普通记录或未提交的 owned 状态事件被丢弃，不再单凭全局 revision 否决已经转发的 A 列表；A 自身事件变化仍拒绝。私有 `FrameLinkTest` 的 37,633 项检查通过，回执 `analysis/runtime/fps-avp90-recording-split-unit1.json`。这只验证内存状态机，没有改 D3D 钩子的全局锁、没有编译或运行候选游戏 DLL，也没有 FPS 收益。下一步仍需列表与 allocator 的同对象占用、原始 Execute 队列顺序、Present／world 边界和重入证明，然后才能把原始驱动调用搬到全局锁外。默认清单和正式 DLL 未改。

对高频 Reset 的一个局部开销另做隔离研究。现有 `frame_link::Tracker` 每次 Reset 把包含 32 个 Event 的整个 List 清零；所有 Event 读取均由 `count` 限界，新增 Event 又完整写入，因此私有版只清空 List 的有效头部。`FrameLinkTest` 37,633 项通过（`analysis/runtime/fps-avp90-recording-split-unit2.json`）。无争锁 CPU 单项基准（30 万次、5 轮）中冻结源码最短约 231.749 ns/Reset，私有头部清理最短约 13.924 ns/Reset；原生诊断局约 180 万次 Reset／约 4168 个 Present，粗算每帧最多约 0.1 ms 的该项机会，不能把微基准当 FPS 收益。记录在 `analysis/runtime/bench-avp90-frame-link-*/run.log`。

另从冻结正式源码派生独立私有候选 `analysis/fps-avp90-reset-bookkeeping-source`，只带上述头部清理、Reset 内三处命令列表 COM 身份查询合并成一次，以及同步更新的自检调用；不带录制期 revision 研究改动，原始 Reset／Execute／Present 与全局锁顺序未变。首轮构建发现自检仍按旧指针参数调用新 API，于游戏启动前编译失败，修正私有自检后重新构建成功。候选 DLL `analysis/native-build-avp90-reset-bookkeeping-b/GearsCaptureAvp90ResetBookkeeping.dll` SHA256 `9890b90b936e3cde52e81b6ea053fdf6e7e74d858ee72656de78a63086b6af86`，26 项原生自检均通过，其中 `FrameLinkTest` 37,615 项，回执 `analysis/runtime/fps-avp90-reset-bookkeeping-build-b.json`。尚无游戏 FPS 对照，不能集成或宣称提帧；默认清单和正式 DLL 均未替换。无新增定时任务。

21:14–21:33 对上述私有候选做第一轮真正游戏 4K 桌面、每眼 3600×3200、scale 1.0、90 Hz 模拟器默认 A→候选→默认 B 夹测，回执 `analysis/runtime/avp90-reset-bookkeeping-aba-20261022-210848.status.json` 为 `verified_aba`，三段各 60 个有效玩法样本，完整窗口新立体 FPS **61.588／66.158／63.362**，候选比两端默认均值高 **3.683 FPS**；三段前后 30 样本分别 **59.393/63.758、66.407/65.936、62.468/63.964**，首 120 ticket 每帧 draw 中位 **1098/1097/1098**。三段会话和设置恢复通过，游戏进程退出，无并行游戏／autoplay，XR 均约 89.8 Hz，正式／候选／正式 DLL 哈希按清单匹配。整卡 GPU 利用率中位 **69/73/69%**，只作吞吐线索。前 120 ticket 跳过 10 后的 `reset_tracking` 中位 **0.509/0.366/0.465 ms**，见 `analysis/runtime/avp90-reset-bookkeeping-aba-20261022-210848.cost.json`；这是启动段而非整个玩法窗，无法单独解释完整 FPS 差。默认两端漂移 1.774 FPS，首段也有明显升温，候选收益需独立复测后再考虑集成。目前另一局游戏在用，私有 `analysis/runtime/avp90-reset-bookkeeping-confirm-pair-20261022-214034.status.json` 一次性普通进程等待其退出及稳定空闲，至多再跑候选→默认两局；不注册定时任务，不覆盖该局或用户设置。正式入口仍未替换，90 FPS 未达到。

独立候选→默认复测后来完成（同一画质备份 SHA256 `8fbb442a…2285d6`、模拟器设置 SHA256 `26e4b071…dd185`、4K 桌面、3600×3200 每眼、scale 1.0、90 Hz）：两段各 60 个有效玩法样本，候选 **65.968 FPS**（前／后 30 样本 65.711／66.447，约 1102 draw/ticket），默认 **53.927 FPS**（54.000／53.267，约 1094 draw/ticket）。双眼会话、进程退出、设置恢复、无并行游戏／autoplay 均通过；回执 `analysis/runtime/avp90-reset-bookkeeping-confirm-pair-20261022-214034.status.json` 为 `verified_confirm_pair`。候选两次很接近，但末段默认比第一夹测两端 61.588／63.362 FPS 低很多，不能把 12.041 FPS 全部归因于本改动。首 120 ticket 跳 10 的默认帧锁中位 7.420 ms、总消费中位 21.191 ms，候选分别 6.274／17.045 ms；`reset_tracking` 为 0.528／0.388 ms，见 `analysis/runtime/avp90-reset-bookkeeping-confirm-pair-20261022-214034.cost.json`。局部 Reset 成本下降有证据，端到端改善仍须排除默认末段环境波动；另一个只跑默认版的有界单次回归 `analysis/runtime/avp90-reset-bookkeeping-default-recheck-20261022-220654.status.json` 正等待其他仓库游戏／autoplay 退出及稳定空闲，尚未开局。集成脚本准备于 ignored `analysis/runtime/integrate-avp90-reset-bookkeeping.py`，设定复测和至少 2 FPS 对最好默认版的门槛，未执行；正式清单／DLL 未变，无定时任务，目标 90 FPS 尚未达到。

对五局报告另核对原游戏 `Present` 速率：默认 A／候选／默认 B／候选复测／异常默认的 60 样本均值分别 **61.82／66.42／63.38／66.26／53.78 Present/s**，与对应新立体帧 **61.59／66.16／63.36／65.97／53.93 FPS** 接近。这表示这几局的帧数差异主要在游戏产帧／Present 节奏，而非 OpenXR 90 Hz 循环重复提交的统计假象；它仍不能独立归因于哪一处 CPU/驱动等待。五局 `owned-scene-*-last.ppm` 在 128×128 网格的跨局 RGB 平均绝对差异约 **1.7–5.5/255**，每局 `last` 对 `late` 自身差异约 **3.2–4.5/255**；仅说明没有明显换场景，不能证明逐帧画面完全相同。只读回执在 ignored `analysis/runtime/avp90-reset-bookkeeping-scene-check.json`，原始 PPM 均未移动到发布目录。

22:26–22:30 的第三次默认版单局复测通过，回执 `analysis/runtime/avp90-reset-bookkeeping-default-recheck-20261022-220654.status.json` 为 `verified_default_recheck`，60 个连续有效玩法样本，4K／每眼 3600×3200／scale 1.0／模拟器 90 Hz、绘制中位 1098；完整窗口 **59.201 FPS**（前／后 30 样本 60.823／57.903）。原生／XR 双眼会话、游戏进程退出、画质恢复、无并行游戏／autoplay 与构建哈希均通过。六局对照里候选两局均值 **66.063 FPS**，相对四局默认中最快的 **63.362 FPS** 仍高 **2.701 FPS**；异常偏慢的默认局不作为收益大小依据。构建 26 项原生自检、严格六局门槛、启动器文件与便携清单校验后，将候选原生 DLL 及测试 EXE 复制到 `tools/native-capture/build-avp90-reset-bookkeeping1/`，只切换默认 `tools/vr-launcher/manifest.json` 的 `native_build`／`native_dll`／`native_sha256` 三字段；新清单 SHA256 `3b9c9674ea17d820b5e153bc21d3cb76bb54f5fbb98665566c51526265199709`，DLL SHA256 `9890b90b936e3cde52e81b6ea053fdf6e7e74d858ee72656de78a63086b6af86`。完整回执 `analysis/runtime/fps-avp90-reset-bookkeeping-integration.json`。源代码把 `frame_link` 只清有效头部、Reset 中三次列表 COM 身份查询合并为一次；保留原始 Reset／Execute／Present 和全局锁顺序，已逐段核对 tracked Reset 实现与游戏验证候选一致。用户正常画质配置未变；这只是模拟器增益，约 66 FPS 距 90 目标仍差约 24 FPS，也未做真机验收。未创建定时任务。

Astra Max 对剩余约 4.03 ms/新帧缺口的只读复核建议：先验证已有列表身份→槽位提示缓存，再考虑高风险的原始 Reset／Execute 拆锁。旧诊断中附件退休及三份 viewport／target 跟踪与消费者等待重合约 0.82 ms，扩大到 frame-link／owned-scene 后约 1.49 ms；这是这些阶段的上界，缓存只能节约其中的查询，且本次已集成的头部清理／身份合并已吃掉一部分。按旧源码、旧缓存候选和新已验证 ResetBookkeeping 源做六文件三方合并，隔离源码 `analysis/fps-avp90-frame-index-source/` 与哈希回执 `analysis/runtime/fps-avp90-frame-index-rebase.json` 已准备；唯一冲突是 `frame_link` 新列表初始化，同时保留头部清理与 cache 更新。未构建、未开游戏、未提交帧率收益；后续应先验碰撞、代次、退休、槽位复用与世界切换等价，再对同关卡做 A/B/A。正式默认版保持上述已集成构建。

私有 AVP90 列表缓存候选已从上述重基源码构建为 `analysis/native-build-avp90-frame-index/GearsCaptureAvp90FrameIndex.dll`，SHA256 `efdae7bc7efd6acdb6ca7a36b4d46cf7d0d0edb9da4f7a7ae69e6f4a04b5bc21`。相对已验证 ResetBookkeeping 源只改 `frame_link`、viewport 与附件跟踪三份头文件及其三份测试；490 个源文件清单不变，28 项原生检查通过，其中 FrameLink 39,672 项，并保留已有头部清理和单次 COM 身份查询。构建回执为 `analysis/runtime/fps-avp90-frame-index-build.json`。默认清单 SHA256 仍为 `3b9c9674…5199709`，未替换。隔离清单 `analysis/runtime/manifest-avp90-frame-index.json` 仅改三项原生构建字段；普通一次性进程 PID 58824、状态 `analysis/runtime/avp90-frame-index-aba-20261022-225021.status.json` 已启动默认→候选→默认夹测，逐局约五分钟稳定空闲门槛，后台窗口置后、静音、只观察输出、禁物理手柄后台输入，不注册定时任务。状态首次核查零局，等待进程实际存活；尚无本候选 FPS 收益证据。

逐帧复核之前的 Reset 细分诊断，`analysis/runtime/avp90-queue-overlap-20261022.json` 将 120 个玩法 ticket 的原生 selected／begin／returned／publish 时间戳与成本回执逐个核对。四组各 30 ticket 中共有 116 对连续帧；新帧选中时上一帧仍在消费的比例为 116/116。选中→本帧消费开始平均 13.333 ms；连续对中该等待平均 13.379 ms，其中 12.505 ms（逐对平均比例 93.4%）与上一帧的消费区间重叠，区间外仅 0.874 ms。消费 begin→returned 平均 15.083 ms，选中→发布平均 28.743 ms；这些帧流水线重叠，28.743 ms 是单帧延迟而非产帧周期。先前 5.471 ms 取锁等待中 4.742 ms 有其他线程持锁交集；诊断早于 ResetBookkeeping 集成且有插桩开销，不能把这些旧数直接当当前可省预算。当前列表缓存 A/B/A 的默认 A 已以 60 样本通过，60.299 新立体 FPS；候选已以 60 样本通过，73.759 FPS，绘制中位 1099/ticket，双眼会话及设置恢复通过。默认 B 仍在等待稳定空闲，且一次外部 CPU 高负载使门槛重新计数；未完成夹测前不归因、不集成候选。

23:26–23:29 列表缓存默认 B 结束，三段 A／候选／B 都各有 60 个有效玩法样本，完整窗口新立体 FPS **60.299／73.759／57.675**，前／后 30 样本依次为 **58.835/62.559、75.046/72.668、56.247/59.976**。两端默认差 2.624 FPS，候选比最快默认高 13.460 FPS，尚不能把单轮差额全部归因于改动。三局均为已购 Steam 游戏、4K 桌面、每眼 3600×3200、scale 1.0、模拟器约 90 Hz，绘制中位 1093／1099／1099；原生／XR 双眼会话通过，游戏退出、设置恢复，无并行游戏／autoplay，构建哈希与清单匹配。游戏 Present 均值约 60.83／77.77／58.31/s，候选的新立体产帧与 Present 间还有约 4/s 差额；候选整卡 GPU 利用率中位 82%，两端默认 69.5%／66%，都只是吞吐线索。粗网格场景图跨局 RGB 差异 2.913–4.484/255，见 `analysis/runtime/avp90-frame-index-aba-scene-check.json`，不能证明逐帧画面相同。A/B/A 回执 `analysis/runtime/avp90-frame-index-aba-20261022-225021.status.json` 已标记 `verified_aba`，原进程退出。独立反向候选→默认的普通一次性复测进程 PID 20328 已启动，状态 `analysis/runtime/avp90-frame-index-confirm-pair-20261022-233454.status.json`，每局重新等待稳定空闲；无定时任务。正式默认 DLL／清单尚未切换，90 FPS 和真机均未验收。

独立复测的候选段以 60 个玩法样本通过，**68.629 新立体 FPS**（前／后 30 样本 66.467/70.156），原游戏 Present 均值 69.987/s、绘制中位 1092；画质、构建、双眼会话、游戏退出、设置恢复、无并行游戏／autoplay 均通过。首轮候选对应为 73.759 FPS，不能拿最高值当稳定结果。原生发布单槽第一候选局 `capture_attempts=5064,published=4825,busy=237`，第二候选局 `4713,4610,101`；跳过的绝大部分是该共享槽仍忙，直接取消 GPU fence/ACK 等待会破坏图像所有权。即使追回这部分，游戏 Present 仍低于 90/s。首轮 A／候选／B 的前 120 ticket（跳前 10）消费墙钟中位 23.804／14.233／16.270 ms，帧锁入口汇总 8.380／4.542／5.675 ms，Reset 跟踪 0.511／0.175／0.363 ms，见 `analysis/runtime/avp90-frame-index-aba-20261022-225021.cost.json`。这些是开局早段而非完整玩法窗，且墙钟锁等待不能当作可删除 CPU 工作。

反向复测的默认段在 00:35:38 通过约五分钟空闲门槛后开局，但 00:36:59 另一工作树自动玩法任务启动并与它重叠，回执 `analysis/runtime/avp90-frame-index-confirm-pair-20261022-233454.status.json` 为 `incomplete`；该默认局无有效样本、不得比较。测试器退出自己的游戏 PID 59356，发现其他游戏仍在运行而跳过设置写回。测试开始前备份与试验配置哈希相同；其他游戏退出后，当前配置与备份仅差游戏自动将 `QualityBloom=True` 改为 `False`。在无游戏／自动玩法／启动器且低 CPU 时按原备份做了狭义延迟恢复并复核哈希，回执 `analysis/runtime/avp90-frame-index-interrupted-bloom-restore.json`，未覆盖另一局进行中的设置。新的**单局**默认版回归 PID 56084、状态 `analysis/runtime/avp90-frame-index-default-recheck-20261023-004038.status.json` 已排队；为避免其他任务约每数分钟启动一局，本次要求 40 次连续低负载空闲检查后才开局。它仍是普通一次性进程，不是定时任务。集成门槛脚本准备于 ignored `analysis/runtime/integrate-avp90-frame-index.py`，尚未执行；默认启动器未切换。

00:51–00:54 的单局默认复测现已通过，状态 `analysis/runtime/avp90-frame-index-default-recheck-20261023-004038.status.json` 为 `verified_default_recheck`，父进程退出。已购 Steam 游戏、4K 桌面、每眼 3600×3200、scale 1.0、模拟器约 90 Hz 条件下，60 个连续有效玩法样本得 **66.779 个新立体帧/秒**（前／后 30 样本 65.846／67.632）；绘制中位 1093/ticket、原游戏 Present 平均约 67.096/s。原生／XR 双眼会话通过，无并行游戏或 autoplay，游戏退出，设置备份与试验哈希相同且收尾恢复通过；默认 DLL SHA256 `9890b90b936e3cde52e81b6ea053fdf6e7e74d858ee72656de78a63086b6af86`、清单 SHA256 `3b9c9674ea17d820b5e153bc21d3cb76bb54f5fbb98665566c51526265199709`。相对有效独立候选局 **68.629 FPS**，仅差 **1.850 FPS**；首轮默认 A／B 为 60.299／57.675，说明跨局负载／场景仍有显著漂移。`integrate-avp90-frame-index.py --check-only` 因未达到预设至少 5 FPS 的稳定收益门槛而拒绝，未运行集成，正式清单及 DLL 未变。列表缓存保留为私有候选，不声称收益；目标 90 个新立体帧／秒和真机均未验收。下一步针对完整玩法窗口测量消费者的原始游戏／驱动调用、Mod 钩子及锁等待，不能把首 120 ticket 的墙钟锁入口求和当作可直接省去的 CPU 时间。

## 同进程列表缓存交叉试验（2026-10-23）

跨局默认版由 57.675–60.299 漂到 66.779 FPS，现有缓存候选的独立 68.629 FPS 无法通过原定 5 FPS 门槛。按用户授权咨询 Astra Max 后，下一项限定为**同一局 OFF→ON→OFF→ON** 的一次私有干预，暂不推进原始 Reset／Execute 拆锁。`analysis/fps-avp90-cache-crossover-source/` 从当前已验证 ResetBookkeeping 与未集成缓存源码准备；OFF 保留三份跟踪器的原扫描／空槽选择，补记供 ON 使用的已用前缀；切换只使提示缓存换 epoch，保留命令列表、代次、事件、世界与资源状态，命中继续核验身份。三份既有测试增加 OFF／ON 两种入口和跨切换的 Reset、退休、槽复用、世界代次等价序列。源码 505 文件回执 `analysis/runtime/fps-avp90-cache-crossover-source.json`，相对默认八文件有差异（包含新私有控制头）。本阶段尚无构建或游戏收益证据。

普通隐藏单次进程 PID **60512**（py 父 PID 45608）已确认存活，状态 `analysis/runtime/avp90-cache-crossover-20261023-012224.status.json` 为 `waiting_for_idle_build`、`runs=[]`。每次构建／游戏前均要求 20 次连续无游戏、无 autoplay、启动器空闲及低 CPU 检查；第一构建尝试因另一局游戏启动在创建构建目录前被安全拒绝。该单次进程只构建私有 DLL 并最多启动一局，不注册定时任务。原生试验预先固定 30 秒升温、四段各 80 秒、每次切换后及下次切换前各排除 5 秒，每段取最早 60 个连续有效玩法样本，必须完成全部四段。额外每 128 ticket 记录一份有界成本、每段切换后取一次带 GPU fence 的双眼图，图片须在排除期内保存。按原门槛要求两段 ON 均高于最快 OFF 至少 5 FPS，并要求 draw 中位最大／最小不超过 1.05、粗网格 RGB 平均差不超过 6/255。未通过即结束本缓存候选调查；通过也只证明增量候选，不构成 90 FPS 或真机验收。原启动器清单和 DLL 保持上述已验证哈希。

私有交叉源码经 Astra Max 独立只读复核后，补齐两处测量边界：每段截图保存完成后立即在 publish bypass 下关闭自身 GPU／读回对象，再记录完成时间，避免后段额外积累读回分配；成本 `Window` 复用既有 QPC 读取保存实际 begin／end，分析器不再用提前登记时间推算结束。相对默认现为九份变更，505 份源文件重新封存。合成验收器 8 项对抗性检查通过（最快 OFF／最慢 ON、工作量变化、缺末段、图片保存太晚、模式标签错误、世界变化等）；这只是分析规则验证，不是游戏收益。

首次隐藏构建在 showIncludes 本地化字节解码阶段停止，尚未编译；已给独立隐藏控制台显式设置 UTF-8，并保留原依赖前缀核验。新目录 b 的编译在新增附件自检的局部变量遮蔽警告（/WX）停止，已修正为不同变量名。旧单次进程和构建进程均已退出，未开游戏。新的隐藏单次进程 PID **20672**（py 父 PID 10080），状态 `analysis/runtime/avp90-cache-crossover-20261023-013345.status.json`，重新等待稳定空闲，后续只在新目录 `analysis/native-build-avp90-cache-crossover-c` 完成构建／原生检查后才可能运行上述唯一一局。当前其他游戏活跃，未覆盖其设置。正式启动器清单 SHA256 仍为 `3b9c9674…5199709`，目标 90 FPS 未完成，无定时任务。
01:34:42 的后续只读核查确认其他游戏已退出；新进程 20672 仍存活，正在累计构建前空闲（4/20），runs=[]。

01:42 私有交叉 DLL 已在新目录 c 完成验证构建，SHA256 **65dd573bbbb2134217d20c0c5a2b787e9021c521dae903a8e2bb7b046e735a2f**。491 项依赖包含模式控制与实际 Window 时间戳头；**31/31** 原生检查通过，FrameLink 在初始 OFF／ON 两种入口各为 42,754 项检查，附带跨模式等价序列。构建回执 `analysis/runtime/fps-avp90-cache-crossover-build.json`，私有清单 `analysis/runtime/manifest-avp90-cache-crossover.json`；默认清单未变。PID 20672 随后进入 `waiting_for_idle_game`，重新累计稳定空闲后只跑一次四段试验。游戏开始前另封存方案／分析器／探针等七份脚本与来源摘要于 `analysis/runtime/avp90-cache-crossover-protocol-20261023-0140.json`；门槛保持 5 FPS、draw 比例 1.05 和 RGB 差 6/255，没有游戏收益结论。

01:57 同局交叉试验完成，父状态 `analysis/runtime/avp90-cache-crossover-20261023-013345.status.json` 为 `verified_crossover`，父／游戏进程已退出。370 个连续有效玩法样本覆盖完整固定流程，四段各取预定最早 60 样本，七份预封存脚本／来源摘要事后全部相同；会话通过、设置恢复、无并行游戏或 autoplay，4K／3600×3200 每眼／scale 1.0／90 Hz 身份一致。分析回执 `analysis/runtime/avp90-cache-crossover-20261023-013345.status.analysis.json`：

| 同局段 | 新立体 FPS | 游戏 Present/s | Draw 中位 | 粗 RGB 差/255 | 消费墙钟中位 ms | 锁入口中位 ms |
|---|---:|---:|---:|---:|---:|---:|
| OFF A | 63.843680 | 63.876854 | 1099.5 | 0 | 14.47675 | 5.03005 |
| ON A | 72.846819 | 78.960187 | 1101 | 2.1203 | 12.10570 | 3.82760 |
| OFF B | 62.114301 | 62.146950 | 1100.5 | 2.7285 | 14.89285 | 5.20230 |
| ON B | 70.387011 | 74.469784 | 1102.5 | 2.8502 | 12.35380 | 4.06590 |

每段 30／37／30／36 份成本样本均用实际 Window begin/end 证明完全处于该段采样窗；这些是诊断成本而非可相加的纯 CPU。四图双眼均显示同一房间、人物和视角，人工查看 OFF A 与 ON B，没有用粗图差单独证明逐帧相同。最慢 ON 比最快 OFF 高 **6.543331 FPS**，保守时间差 **1.456091 ms/新帧**，通过原定 5 FPS／draw 比 1.05／RGB 差 6 门槛。此结果回答了先前跨局波动造成的不确定性；先前独立跨局 1.850 FPS 未过门槛的原记录和检查脚本保留。ON 的 Present 比新立体发布快约 4.08–6.11/s，表明提速后下游发布损失增大；Present 自身仍低于 90，单项发布优化也不够完成目标。ON 整卡 GPU 中位 84%／80% 只作线索。

02:03 复算原始计数、图像与方案摘要，确认六份 tracked 源均仍等于当前冻结默认后，按用户持续集成授权，使用**原先已完成 28 项构建检查并独立测过的无切换 DLL** `efdae7bc7efd6acdb6ca7a36b4d46cf7d0d0edb9da4f7a7ae69e6f4a04b5bc21` 集成到 `tools/native-capture/build-avp90-frame-index1/`。只移植三份跟踪头及其三份测试；未集成控制头、计时切换、截图或新成本记录代码。默认清单只换三项 native 字段，SHA256 **9076cad575ec324b4a990b8a30a17d6054868bc41574fcb1d1cf71c1b0d35d05**，冻结别名 `tools/vr-launcher/manifest-avp90-frame-index1.json`；XR 与现有功能保持当前冻结版本。启动器文件与 portable payload 校验通过，回执 `analysis/runtime/fps-avp90-frame-index-integration.json`。这不是新发布，也不构成头显或 90 FPS 验收。

默认入口独立回归的普通隐藏单次进程 **52048**（py 父 PID 49900）已确认存活，状态 `analysis/runtime/avp90-frame-index-integrated-regression-20261023-020322.status.json`；等待稳定空闲后只跑一局上述目标设置的 60 样本回归，不注册定时任务。已将新的因果结果及剩余 Present／发布缺口交给同一个 Astra Max 进行有界只读方案复核，尚未构建下一性能候选。

02:13 默认入口独立回归完成，状态 `avp90-frame-index-integrated-regression-20261023-020322.status.json` 为 `verified_default_path`；等待／游戏进程均已退出。原始子状态清单摘要 9076cad5…35d05、native efdae7bc…b5bc21 与默认入口一致，60 个连续有效玩法样本、60.754 秒完整窗口 **75.683 新立体 FPS**，样本均值 76.420、前／后 30 样本 76.809／76.031，draw 中位 1094，XR 89.823 Hz；4K／每眼 3600×3200／scale 1.0，双眼会话通过、画质恢复、无并行游戏／autoplay。原报告 `fps-resolution-4k-avp90-default-021133-20261023.json`；集成回执已附原始状态／报告摘要并关闭 pending 标志。此单局证明默认路径可用，不作为新的单项因果收益，也未达 90 或头显验收。

Astra Max 后续复核建议先做固定双槽 GPU 传递：两槽各自 texture／allocator／list／source／ready／done，按 transport generation＋slot＋sequence 定义租约，FIFO 接收，world／resize／stop 全槽排空，绝不以另一槽 ACK 或仅 CPU 标记回收资源。依据是同局缓存 ON 的 Present 与新立体发布相差 4.08–6.11/s，另有 busy skip 计数；该项单独不足以完成 90，原始 Reset／Execute 拆锁的重入／提交顺序研究暂后置。下一候选限 ignored 私有源码／成对 native-XR 协议，不改当前默认或用户设置，无新增定时任务。

双槽私有候选已从 504 文件 FrameIndex 冻结源与 47 文件 MountedLatency1 XR 冻结源派生，SDK 头／链接／loader 文件一并固定摘要，未改默认。协议升到 v8，两槽独立 texture／allocator／list／source／ready／done；按 generation＋slot＋sequence 验证租约，接收严格 last＋1，跨槽采样漏读不得跳帧。Astra Max 指出的 FIFO 读取交错和 done fence 双重读取遇 device removed 的边界均已修正；resize 先释放本地资源引用，再交还 consumer claim。

`fps-avp90-two-slot-units-c.json` 三项离屏套件通过：273 项 CPU 所有权策略检查；6 组跨进程格式／真实与模拟 ticket 图像传输及 116 项 resize 检查；owned GPU 三种 idle 状态、双队列阻塞读取、两槽占用后第三次捕获跳过、GPU ACK 延迟、旧 generation／sequence ACK 拒绝、三份编码双眼图像逐像素核对、世界交接全槽排空。当前系统未提供 D3D12 debug layer，此处证明实际像素／fence 流程，不称 debug-layer 验证。首轮 a 因新增 CPU 目标未显式 C++17 编译失败；b 的旧跨进程测试假设偶数次采样必遇 busy，双槽后不成立，已将该调度假设改为真实双队列确定性检查，保留 a／b 失败回执。随后又扩展两槽都未读时 resize／stop，并加只统计成功 xrEndFrame 的新图像／重复图像计数及现有 1Hz CSV 列，最终成对构建与游戏验收仍待执行，不能宣称 FPS 增益。

用户睡前授权继续随意测试并提高到最高画质；后续以最高游戏画质、4K 桌面、3600×3200 每眼、scale 1.0 重新建立默认版与候选对照，不把旧画质的 75.683 FPS 当作新设置基线。继续排除并行游戏／autoplay，保存并安全恢复原设置，不创建定时任务。

用户随后明确“结束后不用恢复原设置”，覆盖上一段的画质恢复安排：菜单中设置并重启验证后的最高画质及 scale 1.0 将保留；参考备份只留在 ignored analysis，不回写旧低画质。测试用静音与后台输入限制仍按测试生命周期处理。

双槽最终成对构建 `fps-avp90-two-slot-pair-a.json` 的 37 项检查通过，native SHA256 `7571fee615d413b7250bed794d2917d79e9aed7fe1da52d07f5098e76756bab9`，XR SHA256 `4c261a0eb10552f9a0f7b489250b07b96e49d3da014e92c0596280f7343ca963`；仅保留在 analysis，默认未替换。最终实际 GPU transport 检查包含 128 项 resize 检查。为使对照双方都统计成功 xrEndFrame 的实际新图像，另构建仅增加被动计数的 v7 单槽 XR 对照 `fps-avp90-one-slot-metrics-build-b.json`，6 项 XR 检查通过，SHA256 `67c89b3849800f736f616a4ad3df91fb42ab130800e875c184554da14da3cf50`；native 仍为正式 efdae7bc…b5bc21。首轮 control a 缺少单槽常量头 include 的编译失败保留。新 CSV 分析器 6 项检查通过，区分采集／复制／丢弃／新图像提交／重复提交；尚无双槽游戏收益结论。

最高画质菜单准备 worker b 达到空闲门槛并启动一局，但暂停菜单同时包含 Single／Coop 两份 Options 控件，唯一控件假设失败。`avp90-highest-ui-20261023-025733.status.json` 保留失败、通过的最终双眼会话和空剩余游戏列表；游戏已正常退出。已改为在新鲜的实际键盘焦点链中选择可达 Options，再检查目标叶子后发送确认；worker c 经预检后隐藏启动，只允许一局，保留用户的画质设置，无定时任务。最高画质应用和重启确认仍待结果。

最高画质菜单准备 c 随后因把重复的 `WarUIButtonBarLabel` 装饰标签也当成按钮而停止；已依据既有 UI 证据只枚举 `UILabelButton`／`WarUIListButton`。d 已实际选到 Ultra，但 `optlPostProcessQuality` 不在当前零售 PC 菜单的十一项实际焦点循环中，严格导航拒绝对隐藏控件发输入；之前“Ultra 后再把该项调 High”的假设撤回。c／d 最终会话均通过、游戏正常退出、剩余游戏列表为空，未取得性能比较数据。后续最高画质定义为游戏实际提供的 Ultra（纹理 Ultra 6GB、阴影 Ultra、反射 Epic、16x、特效 High、AO Quality、动态模糊 On、Native）；隐藏的旧版控制值只记录，不冒充可调整菜单。菜单配置／重启只做设置验证，使用两次低负载且无并行游戏的空闲检查；真正 FPS 三段比较仍每局连续约五分钟空闲。worker e 单次重新应用并保留该配置。

03:23 `avp90-highest-ui-20261023-032015.status.json` 成功应用 Ultra 并处理确认／重启两类准确消息，重新打开菜单各关键值一致；正常退出，scale 1.0 保存到普通启动器。03:25 `avp90-highest-restart-20261023-032346.status.json` 为 `verified_highest_restart`：另一局重启后只读确认 Ultra／Ultra 6GB／Ultra shadows／Epic SSR／16x／High effects／Quality AO／MotionBlur On／Native／AA None，未修改画质，最终会话通过、无剩余游戏。按用户要求保留该配置，未回写旧低画质。此处不含最高画质 FPS 结论。

有限三段对照脚本 `avp90-highest-two-slot-aba-once.py` 经哈希、配对清单同功能、原生／XR 身份和最高画质保留摘要预检后隐藏启动；顺序为单槽正式 native＋被动 XR 计数对照 A → 双槽私有 pair → 同一单槽对照 B。最多三局，每局须连续约五分钟低 CPU、无其他游戏／autoplay／启动器；每段取 60 个连续玩法样本并分析实际成功 XR 新图像提交，默认漂移上限预设 5%。任何会话／设置／并行实例／构建证据失败均保留失败并停止，不自动补跑，不创建定时任务。默认启动器尚未集成双槽。

首个三段父状态 `avp90-highest-two-slot-aba-20261023-032639.status.json` 在 A 启动时停止，没有有效 FPS。新生游戏 10820 尚未注入 Mod、未加载 d3d12/dxgi；准确原因是游戏自己的原生对话框 `Last launch did not complete successfully. Revert to default settings?`，不是低 FPS 或双槽失败。原生对话框只提供 IDYES 6／IDNO 7；对已由本轮 birth receipt 的 PID＋creation time＋EXE 路径证明归属的对话框发送 IDNO，保留最高画质。随后游戏正常加载，WM_CLOSE 正常退出，未强杀、未点击其他窗口；`avp90-control-a-loader-failure-windows.json` 保存完整私有证据及收尾，最高画质设置 SHA256 仍为 `de30ad4c790c4391d1bedf918b3274732ea000f428c1822de692cb1514671f3e`。

私有 FPS 探针新增 `avp90_startup_dialog.py`：只从自己 live launcher 的直接 `launch_reserve.py` 子进程命令行取得专属 nonce birth receipt，核对 owner、native、零售 EXE 身份；只有上述准确文本、Message 类原生对话框及 6/7 两个按钮且无并行游戏时发送 IDNO，避免恢复默认画质。探针也提前取得 birth identity，启动失败时能够正常关闭自己的游戏。7 项策略检查、实际 Windows 命令行解析及语法检查通过；未修改默认启动器。原失败状态保留，新有限 A/B/A 进程由 py 父 PID 12808 隐藏启动，仍至多三局且每局重新等待稳定空闲，无定时任务。

Astra Max 只读复核再次确认正式成本记录只覆盖前 120 个 ticket，rolling latest 无成本指针，不能把启动首窗当稳定玩法关键路径。后续若需要新诊断，应在稳定 ROI 每 64 ticket 采一份、地址不可复用，保存 Window 实际 QPC 起止／CPU 时间，并按真实跨线程持锁区间并集区分原始 Reset、原始 Execute 和跟踪开销，禁止相加嵌套 Scope 或把二者合并当 Reset 收益。无调用栈 GPU 调度配置 `avp90-gpu-scheduling-no-stacks.wprp` 已通过本机 WPR 只读 profiledetails 解析预检，使用现有 WPT，尚未开始 ETW 采集；不混入本轮性能对照。

03:51–03:54 最高画质的单槽对照 A 完成：`fps-avp90-once-20261023-035147-019120.status.json` 为 verified，60 个连续玩法样本／60.110 秒，native 新立体发布 32.491 FPS，样本前后半段 32.995／32.976；成功 xrEndFrame 的实际不同图像 **32.970945 FPS**，重复 56.999459/s，XR 循环 89.981 Hz。零售游戏、4K／3600×3200 每眼／scale 1.0／Ultra、会话通过、设置逐字节保留、无并行游戏或 autoplay、退出均通过；单槽 native efdae7bc…b5bc21，计数 XR 67c89b38…3cf50。此为新画质基线，不能和旧低画质 75.683 FPS 混成回归结论。

发现旧 `gameplay_window()` 取全会话前 60 个玩法样本，没有跟随连续后缀窗口。新离线 `analyze_avp90_exact_load.py` 使用报告的准确 `sample_window_ns`，CPU 两端均须在窗内，保留原始状态不改写；7 项正常／对抗检查通过。A 对齐后为 54 个 CPU 间隔／58.900 秒、消费线程 CPU 31.750 秒（仅使用量，不是关键路径）、55 个 GPU 样本中位 99%、功率中位 428.742 W；`*.status.exact-window-load.json` 明确旧汇总窗口不匹配。负载原始样本仍覆盖准确窗口，没有因此重跑游戏。

04:00–04:02 双槽候选取得 60 连续玩法样本且会话、退出、设置保留和无并行均通过，但 XR 循环均值 **78.202933 Hz**，低于预先声明的 87–93 Hz 门槛。子状态 `fps-avp90-once-20261023-040014-720764.status.json` 保留 error，父 `avp90-highest-two-slot-aba-20261023-034357.status.json` 停在 failed，未运行末段 B，进程均已退出，未集成。仅供归因的 `*.status.exploratory.json` 明确 comparison_valid=false：native 发布 52.451025 FPS、实际不同 XR 图像 52.921656 FPS、重复 25.105558/s，XR 总循环下降，因此禁止称为合格 A/B 收益。准确窗口 56 个 GPU 样本中位 99%，消费 CPU 32.516 秒／60.035 秒；整卡占用不能单独确定具体 GPU pass。

A／候选自动 GPU 快照均为实际 7200×3200，双眼同一房间、同一玩家位置，已查看缩略图；这不替代完整窗口的逐帧视觉证明。当前两者的 XR 等待诊断宏未启用，没有现成等待耗时可用于精确归因。Astra Max 同意下一有界 XR-only 候选：只在获取新立体图像时，单次读取每槽 ready/done fence 完成值，先拒绝 device removed，再在队首未就绪时保持原 lease／ticket／last／ACK，直接返回并复用先前已完成的私有图；world quiesce 仍使用原 queued Wait→discard 排空，不改生产者、全局 D3D 锁或 native DLL。新私有源 `fps-avp90-two-slot-ready-source` 580 文件已封存，增加真实独立队列的未就绪不偷偷入队 Wait、编码双眼 FIFO、延迟 GPU ACK、阻塞生产者时 quiesce／resize／stop 检查，以及现有 1Hz CSV 中的 ready-deferred 和累计 XR 等待计数。隐藏构建由 py 父 PID 55348 启动；仍未开启游戏验证、未改默认清单。

04:23 ready-only 私有候选完成构建前检查：首次 a 的四套真实 GPU／策略检查全部通过（279 项两槽策略、跨进程图像传递与 128 项 resize、owned 双槽、ready-only 独立队列）；随后 XR 尚未编译即因构建包装器要求 `NAME=VALUE` 而传入 `-DNAME=VALUE` 失败，原失败回执保留。修正参数格式后，b 只续编 XR，复核同一份 580 文件源码与 a 的依赖回执、保留原四套结果，不重复跑 GPU 测试。七项 XR 检查通过，合计 11 套，构建回执 `fps-avp90-two-slot-ready-build-b.json`；实际 GPU 检查证明生产者阻塞时 16 次获取均不偷偷排队 Wait、双眼编码按 FIFO 1→2、延迟 ACK 禁止复用、阻塞下 quiesce／stop／resize 不伪造完成。系统仍无 D3D12 debug layer，未称其验证。

新私有 XR SHA256 `992bc97be02ccff36f3e86828d9002e6512e2c546a3c16740ff10b0901ace79a`，native 沿用双槽 `7571fee6…56bab9`；私有清单 `manifest-avp90-two-slot-ready-b.json`，默认 9076cad5…35d05 未变。实际等待统计宏开启，现有 1Hz CSV 分别累计 local fence、左右眼 swapchain、WaitFrame、EndFrame 的次数／墙钟，未增加每帧文件输出。新准确窗口等待分析器 7 项正常／对抗检查通过；只按 FPS 窗内两端累计差分析，明确 WaitFrame 包含正常节拍，非 GPU 耗时。另修正私有单次脚本：CPU／GPU 汇总直接取报告连续窗口；首 120 ticket 的 draw 不再错误改名为 distributed，旧报告保留不回写。

04:23:50 普通隐藏单次 worker `avp90-highest-ready-smoke-20261023-042350.status.json` 启动（py 父 PID 53796，Python PID 14888），最多一局、每局前 20 次连续空闲检查；先验证新候选能保持原定 87–93 Hz 与实际新图计数，再决定是否值得新的 A/B/A。最高画质摘要 de30ad4c…71f3e 与 scale 1.0 保留，无定时任务，没有单凭离屏测试集成双槽。

04:29–04:31 ready-only 首个游戏诊断完成，60 连续有效玩法样本／60.249 秒、native 新发布 46.258 FPS（前／后 30 样本 46.709／45.748）、Present 样本均 56.057/s；会话通过、设置保留、无并行、游戏及父子进程均已退出。原 child `fps-avp90-once-20261023-042923-928995.status.json` 仍为 error：第 5 个派生 XR hz 为 null，telemetry 明确 source=unchanged，1Hz CSV 与 1Hz 观察边界撞位，直接 sum 抛 TypeError；并非原始 XR 帧记录缺失。保留失败，不把回执改成通过，也不声称完成 A/B/A。

独立 CSV 59 观测／58.526 秒位于同一准确玩法窗：完成循环 **89.8063745 Hz**，实际不同 XR 图像 **46.1675845 FPS**、重复 43.6387901/s，0 invalid／discard。`avp90-ready-smoke-20261023-042923.retrospective.json` 明确 original_probe_passed=false、prospectively_accepted=false、comparison_valid=false；仅证明诊断数据与原定 87–93 Hz 一致。准确窗口整卡 GPU 55 样本中位 99%、功率中位 401.888 W；消费线程 CPU 使用约 0.518 逻辑核，不据此直接判关键路径。

同窗累计等待差：local fence 2700 次、平均 2.168659 ms、总 5.855379 秒（窗口 10.00%）；两眼 swapchain 各 2702 次、总墙钟分别 0.568／0.120 ms；WaitFrame 5256 次均 8.383013 ms（75.28%，含正常节拍）；EndFrame 均 1.442666 ms（12.96%）。这些是 XR 线程墙钟等待，不能相加当游戏 GPU 执行时间。ready-only 延后约 38.769 次/s，没有通过放宽旧图年龄掩盖它。

后续测量规则在再次开游戏前修正为准确 ROI 内完成 CSV 帧计数／实际时间差，保持 87–93 Hz 门槛并要求至少 55 观测、55 秒、focused／rendering、无计数倒退或大空洞；`avp90_xr_cadence.py` 的 7 项检查通过。原单局失败留存，不自动翻转结论。已将上述结果交给同一个 Astra Max 做有界只读建议；与此同时准备仅加相同被动等待统计的 v7 单槽 XR 对照，防止下一对照双方的测量开销不一致，默认 native／协议保持不动。

04:37 相同被动统计的单槽对照构建完成：`fps-avp90-one-slot-waits-build-a.json` 七套 XR 检查通过，XR SHA256 `f594f4d8c52b9b10532495b7744c3b2704190576cc6ccb698fe2249b16a333db`；只相对旧单槽源改四份测量文件，577 文件封存，v7 gpu_share 与正式 native efdae7bc…b5bc21 均未变。候选／对照采用相同 CSV、相同 wait recorder 和编译宏；对照 ready_deferred 恒零，不增加 ready-only 逻辑。

Astra Max 建议先完成 ready-only 公平验收，再用独立 10–15 秒无栈 GPU 跟踪定位，三局期间不混入 ETW。它明确否定“消费 CPU 已被证明最大瓶颈”的过度结论；2.17 ms fence 包括排队／调度／通知，WaitFrame 包含节拍。现有代码确认 XR queue 同时传入 XrGraphicsBindingD3D12KHR，故 XR 进程／队列忙不能全部归给双眼复制；游戏 native shared copy 与原渲染共用 DIRECT queue，后续若需拆分须稀疏 GPU 时间戳，保持原 fence 完成后异步读回，不加 CPU 等待。

04:38:50 普通隐藏父进程 py PID **31016** 启动唯一有限三段 `avp90-highest-ready-aba-once.py`：对照 A → ready-only 双槽 → 对照 B，各局前 20 次连续空闲，最多三局，无定时任务。预先固定各段至少 60 连续玩法样本、CSV 至少 55 秒／55 观测、XR 87–93 Hz、两端对照漂移 ≤5%、候选比更快对照至少 +5 distinct FPS、首 120 ticket draw 比 ≤1.05（仅启动工作量）、早期 7200×3200 双眼图粗差每眼 ≤6/255 及人工复核；首图不冒充 ROI 逐帧工作量证明。设置／会话／并行／构建摘要必须全部通过，任何失败即停，无自动替换。最高画质 de30ad4c…71f3e 与 scale 1.0 保留，正式清单仍未替换。

04:47 新公平对照 A 完成且被前瞻性规则验收：child `fps-avp90-once-20261023-044424-539211.status.json` 为 verified，60 个连续玩法样本／60.631 秒，native 新发布 33.745 FPS；准确 CSV 60 观测／59.571 秒、distinct XR **33.7246091 FPS**、XR **89.7756146 Hz**、0 invalid／discard。两半样本均值 35.416／32.979，首 120 ticket draw 中位 1107。会话通过、设置保留、无并行／autoplay、游戏正常退出；54 个匹配 GPU 样本中位 99%，消费 CPU 31.578 秒／59.174 秒。对照 local fence 2009 次均 11.528885 ms，说明单局 ready-only 的 2.17 ms 值值得继续检验；此时仍没有完整因果对照结论。

04:48:22 只读核查：父 py 31016／Python 40580 均真实存活，父状态 `avp90-highest-ready-aba-20261023-043850.status.json` 为 waiting_for_idle，下一段 two_slot_ready，6/20 空闲检查、目前没有游戏／XR 进程；未启动重复任务。最高画质摘要仍 de30ad4c…71f3e，默认 scale 仍 1.0。已人工查看首个 ready-only 诊断与新 A 的完整 SBS 缩略图，同一房间／人物位置且双眼图像正常；它们仅是早期快照，不代替剩余两局或完整 ROI 验证。

04:55 新公平候选局完成且通过：child `fps-avp90-once-20261023-045222-154423.status.json` 为 verified，60 连续玩法样本，native 新发布 **47.112 FPS**；准确 CSV 的实际不同 XR 图像 **46.9472107 FPS**，XR **89.818 Hz**，本地 fence 平均 **1.801734 ms**。会话、设置保留、游戏退出、无并行及构建检查通过。与首段 A 33.7246091 FPS 存在较大差距，但末段 B 尚未完成，未宣称完整因果收益，未集成。

源码迁移预检 `avp90-ready-integration-preview.json` 已在 ignored analysis 生成 19 份预览；CMakeLists、capture.cpp、owned_scene_gpu.inl 仅应用唯一上下文差异，保留当前并行修改，其余文件核对冻结基线或为新文件。`integrate-avp90-two-slot-ready.py --source-preflight-only` 已验证 19 份当前／预览摘要、原成对 37 套检查和 ready-only 11 套检查；正式源码／构建／清单未写。实际集成仍要求三段量化门槛、原始 CSV 与 native 回执复算、三图人工复核、无其他游戏、最高画质摘要相同；双端 ABI8 必须成对切换。

04:59:14 只读核查发现另一局零售游戏 PID **38884**（04:58:43 创建，父 PID 31076）正在运行；本轮候选游戏已更早正常退出，没有采样重叠。三段父进程 py 31016／Python 40580 均存活，状态 waiting_for_idle，next=control_b、0/20，busy_images 含 gowde-steam.exe。遵照等待要求，不关闭／覆盖该局，不启动新测试或 GPU 跟踪；默认清单仍 9076cad5…35d05。最高设置文件只读查得 SystemSettings USEVSYNC=False、ClientSettings TargetFramerate=0；DefaultSettings 的 USEVSYNC=TRUE 是另一节默认值，不能混作活动设置或实际 Present interval 的证明。

另一局随后正常退出，原三段父进程继续累计空闲并在 05:05 启动末段 B，无重复测试。05:07 三段结束，`avp90-highest-ready-aba-20261023-043850.status.json` 为 completed_aba_pending_review，父／子／游戏均退出。每段 60 连续有效玩法样本、会话／设置／无并行／身份／退出均通过，三局均无 ETW：

| 段 | 实际不同 XR 图像 FPS | native 新发布 FPS | XR Hz | 前／后 30 样本 FPS | local fence 均 ms |
|---|---:|---:|---:|---:|---:|
| 单槽 A | 33.7246091 | 33.745 | 89.776 | 35.416／32.979 | 11.528885 |
| 双槽 ready-only | 46.9472107 | 47.112 | 89.818 | 47.485／46.593 | 1.801734 |
| 单槽 B | 33.8755514 | 33.998 | 89.764 | 35.639／33.009 | 11.870683 |

两端基线漂移 **0.446574%**，候选比更快对照高 **13.0716593 distinct FPS（约 38.59%）**，通过预定 +5 FPS 门槛。首 120 ticket draw 为 1107／1116／1101.5，比值 1.013164（只代表启动工作量）；三图全幅 7200×3200，候选双眼粗 RGB 差 0.929／1.452，末段差 1.445／1.222，均低于 6/255。已逐一查看三份完整 SBS 缩略图，同一房间、玩家位置与视角，双眼均有场景且有正常视差，无明显空半边、损坏或缺场景；小幅人物动画／光照差仍存在。`*.visual-review.json` 保存检查及其限制，不称逐帧 ROI 像素相同或头显验收。三段整卡 GPU 中位均 99%，仍不足以确定单个 GPU pass。

05:19 原始 native／CSV 复算、三份回执与脚本摘要、19 份源码预览和人工图像检查全部通过后，按持续集成授权**成对集成**，回执 `fps-avp90-two-slot-ready-integration.json`。默认清单 SHA256 **5f5c8f150af1acc128ae30bb0e7e3638415064072c365bab08c02316c640cff8**，冻结别名 `tools/vr-launcher/manifest-avp90-two-slot-ready1.json`：

- native `tools/native-capture/build-avp90-two-slot-ready1/GearsCaptureAvp90TwoSlot.dll`，SHA256 **7571fee615d413b7250bed794d2917d79e9aed7fe1da52d07f5098e76756bab9**。
- XR `tools/openxr-probe/build-avp90-two-slot-ready1/OpenXRPoseProbe.exe`，SHA256 **992bc97be02ccff36f3e86828d9002e6512e2c546a3c16740ff10b0901ace79a**。
- 只改清单五项 native／XR 身份，其他既有功能保持当前默认值；26 份二进制／检查器逐字节校验，运输四套检查器采用最终 ready-only header 对应版本。启动器文件与 portable payload 检查通过。
- 19 份相关源码按预览迁移，三个有并行修改的文件应用唯一上下文增量；原发布标签、0.0.2 分支／包及其冻结构建保留。这是 0.1.0 开发集成，不是新发布或 90 FPS／头显验收。
- 最高画质摘要仍 de30ad4c…71f3e，scale 1.0 保留；未回写低画质。default_path_regression_pending=true、headset_validation_pending=true、goal_90_fps_achieved=false。

05:20:36 普通隐藏单次默认入口回归由 py 父 PID **34692**／Python **58484** 启动，状态 `avp90-ready-integrated-regression-20261023-052036.status.json`；每轮前仍 20 次连续空闲，最多一局，无定时任务。它在完整 60 样本 FPS ROI 结束后，另取独立 15 秒 GPU 调度窗口；新跟踪助手 7 项策略检查通过（含异常收尾／外部跟踪／ROI 边界），使用本机 WPR 明确支持的唯一 `-instancename`，start／stop 始终指向自己实例，不停止外部跟踪。源码摘要保存在 `avp90-default-gpu-tail-source.json`。05:21:09 父子真实存活，正在等待空闲，尚未启动该局或实际 ETW；采集完成后仍须检查事件丢失、HAGS／队列完整性，不能以成功写 ETL 直接断言瓶颈。若 WPR 不可用，已完成 FPS 采样与正常退出仍独立保留。

05:30–05:32 默认入口回归已通过，父状态为 `verified_default_path`，父子／游戏／XR 均正常结束。回执 `fps-resolution-4k-avp90-default-053024-20261023.json` 含 60 个连续玩法样本、60.453 秒；同一最高画质、每眼 3600×3200、scale 1.0，native 新发布 **45.986 FPS**。按成功 xrEndFrame 新图像 key 复算，59 个 CSV 区间／58.511 秒共 **2695** 个不同立体图像，**46.059676 distinct FPS**，重复提交 2559 个、无非法 key／运输丢弃，XR 循环 **89.795004 Hz**。无并行游戏／autoplay，双眼会话通过，设置仍为 de30ad4c…71f3e。原集成回执已被本轮脚本摘要固定，因此不修改其历史 pending 字段；新的 `fps-avp90-two-slot-ready-integration.regression.json` 逐一复核源脚本、清单、遥测和侧车摘要后关闭 default_path_regression_pending。头显验收待做，90 个新图像 FPS 目标未达到。

准确 ROI 内 54 个 CPU 区间／58.999 秒，消费线程约 0.507 个逻辑核；55 个 NVML 样本整卡 GPU 中位 99%、最低 95%、功率中位 398.961W。XR local fence 已降为均 **2.1253ms**，WaitFrame 均 8.3113ms（包含正常 90Hz 节奏），EndFrame 均 1.5263ms。这些 CPU 等待和整卡数据不能单独确定原游戏某个 GPU pass 的瓶颈。

FPS ROI 结束后独立 WPR 启动返回 **0x80070005 Access is denied**；唯一自有实例 stop 返回无活动 profile，未产生 ETL。05:44 的只读自有实例及全局 status 均为 `WPR is not recording`，证据 `*.gpu-tail.cleanup-verification.json`；原失败回执保留，不提权、不重复启动该路径。现有 SDK／PDH 本机预检成功：本机已验证的 GPU Engine 通配计数器一次读取 884 个有效实例，原生查询约 3.8ms。Astra Max 建议先采游戏／XR 的每进程、每物理引擎占用，再只给主要一侧增加稀疏 GPU 时间戳；不合并并行引擎百分比，不把缺失实例当 0，不把时间戳区间叫作纯 copy 执行耗时。

有限私有 PDH 诊断 `run-avp90-pdh-tail-once.py` 于 05:45:51 隐藏启动，py 父 PID 58532，状态 `avp90-pdh-tail-20261023-054551.status.json`。20 个稳定空闲检查后至多启动一局，先取得 20 个连续玩法暖机样本，再用 500ms 间隔采 15 秒 GPU Engine；游戏与 XR 均核对 PID＋creation time，目标 adapter LUID 只读自有 v8 共享元数据取得。原始 FPS 回归／WPR 源文件及默认构建未修改；两项实例身份／缺失与重复实例策略检查通过。此轮只用于定位，不能作为新的优化 FPS 对照或真机结果；仍保留最高画质及 1.0 缩放，无定时任务。

05:53:23 该 PDH 单局完成，父状态 `captured_pending_analysis`，随后独立 `*.status.analysis.json` 复核 20 个连续暖机样本、会话、设置及源脚本摘要并确认游戏／XR 的 creation identity 已退出。共享元数据为 v8／1192 bytes、7200×3200 SBS、目标 adapter LUID `0x00000000_0x00013711`，前后绑定一致。该 adapter 物理 0／3D 引擎 0 上，游戏 30 个有效样本中位 **81.1448%**、均 80.3532%；XR＋模拟器同引擎中位 **3.9588%**、均 3.9523%，均无缺失、无效或重复实例。其余引擎分别保留，未累加成“整卡百分比”。这是游戏侧 GPU 工作较重的线索，不能区分原游戏、native copy 和桌面镜像，也不能换算成单帧 GPU pass 时间。

实际 30 次 PDH 采样用了 **22.4345 秒**，相邻采样中位 **0.7216 秒**（0.6890–0.8523 秒），PDH 调用本身中位 3.8513ms；归属／场景复核延长了目标 500ms／15s 计划。保留原始实际时标，不称已取得 15 秒固定 500ms 的采样。最高画质与 scale 1.0 仍保留，默认二进制未改。

Astra Max 静态复核：成功接管的 family 是两视图替代单视图，不能推断每帧固定渲染桌面＋左右眼三遍。需要进一步在稳定玩法中覆盖 owned 与普通 renderer 消费、真实目标／Reset generation 和 Execute 提交，才能判定额外桌面场景是否存在；无 ticket 不构成可安全跳过消费的证据。另有独立桌面镜像 capture／Present draw，native 共享 copy 很小也不能排除镜像成本。当前回归已有 64 行有界 fallback 记录，63 行位于 ROI 前约 9.7 秒，均 identity_match=0、lifetime_proven=0；这些不是完整 GPU 消费归因，不能直接删掉普通渲染。

按游戏侧占用线索，05:54:43 隐藏启动私有 native GPU bracket 构建，py 父 PID 40124。源码 `analysis/fps-avp90-native-gpu-bracket-source` 从已冻结 ready-only 源逐文件校验复制，默认 native／XR 及已固定的旧测量脚本均未修改。每 128 个共享序号采一份同 command list 的屏障＋copy GPU 时间戳区间，每槽独立 query/readback、最多 64 行；仅在原有 producer ready 完成后读取，槽复用仍受既有 done ACK 约束，不增加队列／CPU 等待、Signal 或 ACK，保留失败 quarantine／unsafe detach。耗时按该 queue 的 GetTimestampFrequency 换算，不能称纯 copy 执行时间，不能从 CPU fence wait 相减推导排队耗时。新增双槽实际 GPU 门控检查及既有原生门禁正在执行；单次父状态 `avp90-native-gpu-bracket-20261023-055605.status.json` 至多一局，构建通过后重新等约五分钟稳定空闲，取得 60 样本再分析；这是诊断版，不用于宣称优化收益。

05:57 独立 `GpuTimestampBracketTest` 已通过，两个查询槽均完成有真实 copy 的区间，测试中各 16 次未完成 ready 轮询无提前读取，并拒绝 UINT64_MAX；这是单独 GPU 检查，不是游戏中的两槽采样覆盖证明。Astra Max 窄审未发现必须停止当前 attempt 的生命周期缺陷；指出全局 serial%128 在严格轮换时偏向单槽，后续分析须报告实际各槽样本数，不得声称游戏双槽均已采样。CPU record_qpc／read_qpc 使用原生 QPC 频率，GPU begin／end 另用 queue timestamp frequency，不能混用。

06:07 私有 native shared-copy 诊断完成：33 项原生检查通过后，单局取得 60 个连续玩法样本、会话通过、原有最高画质及 scale 1.0 保留、无并行游戏／autoplay、游戏／XR 已退出。父状态 `avp90-native-gpu-bracket-20261023-055605.status.json`，分析侧车 `*.status.gpu-brackets.json`，DLL SHA256 **ec25a9363f2bec7b45c6cb2f61694bc469b35317921b25dcc45b68b9ce01e51d**，XR 仍为已集成 992bc97b…e79a。全会话 24 条有效 GPU 区间，严格以 CPU record/read 起止都处于玩法 ROI 筛出 **22 条，全部 slot 1**；slot 0 无实际游戏样本。GPU 区间中位 **0.083408ms**、均 **0.092559ms**、P95 **0.091072ms**、最大 **0.404224ms**，无创建／Map／非法值／未完成／容量跳过。GPU frequency=1,000,000,000，CPU QPC frequency=10,000,000，分别换算。三项离线解析检查通过，含两频率区分、ROI 读取边界和未完成数据拒绝。

这支持把下一步主排查从该已采样槽的共享复制转向原游戏渲染／独立桌面镜像，不能证明另一槽、所有镜像或原游戏 pass 的 GPU 成本。诊断版 native 47.571 FPS、XR 循环 89.677Hz 只记录环境，不作为优化收益；当前默认仍为已验收的双槽 ready-only pair，90 个新图像 FPS 目标未达。

### 2026-10-23 06:23 — 连续消费归因诊断已构建，避让并行游戏后补完门禁

下一项采用稳定玩法显式触发、连续 **64 次全部 renderer consume** 的 CPU 观察，不按 owned ticket 过滤；保存 family／renderer 视图数量及其原有有效位、world、ticket、consume 序号，关联已有 canonical list＋真实 Reset generation 和原始 Execute 前保存的列表成员。原始 consume／Draw／Execute、既有提交验证、锁、GPU 等待与 ACK 顺序不改，原函数返回后不解引用 renderer。跨消费／无 scope 混入、arm 前已录制前缀和没有 Draw 观察的实际提交均保留为 mixed／prefix_unknown／unknown，不能按无 ticket 跳过原函数。

Astra Max 的窄审发现并已修正：arm 时点前缀缺失（加入同锁串行的真实 generation 水位）；有效但未见 Draw 的 Execute 被漏记；第 64 次 begin 提前关闭未知观察（改为等全部已接纳消费返回）；视图数量丢失原有 family 128／renderer 4096 有效位。纯 CPU 用例覆盖这些边界，Python 两项归因策略检查通过。未观察原生 Dispatch／Indirect／Bundle，Draw 计数是既有转发前观察，不能称实际 GPU Draw 总数或据缺失记录排除这些工作。

冻结修正版源码 `analysis/fps-avp90-consume-classification-valid-source`，回执 `fps-avp90-consume-classification-valid-source.json`。新 DLL `analysis/native-build-avp90-consume-class-b/GearsCaptureAvp90ConsumeClass.dll` SHA256 **21660bbcb2243c6a2612f48be3b0e04a2dbd910726ba1fb0b51862d9c3f4cdc3**，493 项头依赖确认。前 8 项独立检查全部通过（含实际 GPU 运输与新的纯 CPU 分类器），但在启动后续检查前发现另一局 GOWDE，检查器按空闲门禁退出；没有启动本诊断的游戏、没有中断外部游戏。旧 A 的 26 项检查也因该外部游戏停止，且 A 缺少视图有效位，另有 `*.runtime-rejection.json` 明确禁止 A 游戏测试，未集成。

只补未执行检查的隐藏进程于 06:21:47 启动，py 父 PID **26300**／Python **55780**，状态 `fps-avp90-consume-class-build-b-resumed.json`。它固定已编译文件、源码、原失败回执摘要，复用已通过 8 项检查，持有共享 FPS 观察互斥，待约五分钟稳定空闲后执行剩余门禁；不重新构建。游戏诊断父 py PID **33748**，状态 `avp90-consume-class-20261023-062219.status.json`，只接受补完检查后的 B 清单；通过后另等约五分钟稳定空闲，至多一局，20 个连续玩法暖机样本后发送一次仅记录事件并观察约 15 秒。06:23 补检仍在等空闲、游戏父在等补检，均未开局。两者均为有限当前进程，无定时任务；默认清单仍 5f5c8f15…cff8，最高画质摘要 de30ad4c…71f3e 与 scale 1.0 保留。

06:30 B 补检结束，回执 `fps-avp90-consume-class-build-b-resumed.json` 为 `verified_private_diagnostic`、**33 项全部通过**（8 项既有通过记录＋25 项补检），未重新构建；冻结私有清单 `manifest-avp90-consume-class-b.json`，native 仍为 21660bbc…4cdc3，XR 不变。游戏父 Python PID **58816** 已转入自己的 20 次连续稳定空闲检查；06:32 尚为 `runs=[]`，未启动该局。

另对现有 PDH 原始数据做只读外部负载复核：同物理 3D 引擎上，非自有 PID 1484 的 30 个非零观察中位约 10.688%、PID 1360 约 3.381%。当前进程名分别为 csrss.exe／dwm.exe，creation time 均早于该采样、当前仍存在；这是同 PID 属于系统显示进程的时间身份线索，不能把占用全算作 Mod，不能断言它们与测试显示工作无关。未对这些进程采取任何操作。只读 nvidia-smi 显示 RTX 5090 D，当前／默认功耗上限均 **575W**；不是载荷中的时钟、温度或节流验收，也不以此单独证明原游戏瓶颈。

### 2026-10-23 06:43 — 连续 64 次消费归因完成，尚无可删除普通渲染的证明

`avp90-consume-class-20261023-062219.status.json` 为 `analyzed_bounded_consumption`，单局 20 个连续玩法暖机样本、同一 world、双眼会话、无并行游戏／autoplay、正常退出和最高画质保留均通过；game 36916／XR 33416 及父进程均已退出。一次 record-only 事件后实际观察 15.885 秒，64 个已接纳 consume 均返回、没有容量溢出；它们的 CPU begin/end 覆盖约 1.106 秒。该短窗是连续消费归因，不冒充 60 秒分布式 GPU 采样或 FPS 收益对照。

`fps-resolution-4k-avp90-consume-class-063546-20261023.consume-class-analysis.summary.json` 保存有界概要：56 次 `registered_two_view`、8 次 `unregistered_one_view`；记录了 13,804 个实际提交的 command-list generation，其中 64 个 mixed、2 个 arm 前前缀未知，另有 2,403 个有效 Execute 成员没有 Draw 观察、42,152 个 scope 外 Draw 观察。没有 unresolved scoped Draw／非法 Execute ref。未注册单眼没有得到明确纯归属的 swapchain Draw 阳性，但 mixed、MRT／未知目标、scope 外工作及未观察的 Dispatch／Indirect／Bundle 仍存在，不能据此断言没有额外场景，更不能跳过原始 consume。

两类 wrapper 墙钟中位分别 13.928／8.358ms，包含 Mod 钩子、观察锁、调度和等待，不是 GPU 耗时或可删除成本。多数已归属 generation 的首个目标尺寸为 7200×3200，这不是完整逐 Draw 目标／viewport 分布，不能直接认定每次都渲染全幅像素。当前诊断未保存 Draw thread／caller，尚不能区分 scope 外工作是其他 worker 还是同线程后续 UI／呈现；已把这组小概要交给同一 Astra Max 只读咨询下一项最小诊断。默认双槽 ready-only pair 未改变，最高画质及 scale 1.0 按用户要求继续保留，90 个新立体图像 FPS 仍未达成。

### 2026-10-23 06:59 — 镜像 GPU 诊断门禁通过，预先固定筛选门槛

Astra Max 建议先完成独立桌面镜像的 GPU 成本筛选，暂不扩充 CPU TID／caller 日志。私有源码 `analysis/fps-avp90-mirror-gpu-bracket-source` 从已封存 ready-only 源校验复制；只在原镜像 copy／每个 draw slot 的命令列表加入稀疏时间戳和 Resolve，复用该操作原有 fence 的完成值读回，不加 Wait／Signal／ACK。每 128 个对应角色 serial 选择一次、每 role/slot 最多 64 行，保存 native session QPC、world、source generation、serial、ticket 和实际 slot。安全 close 释放 GPU 资源并保留记录，未确认的 Signal 仍走原有 unsafe 保留；Astra 窄审未发现新增阻断项。本轮禁止游戏中的镜像截图 readback，并要求同 world／source generation、报告实际槽覆盖，不将 retire 后丢失的旧槽记录冒充完整覆盖。

`fps-avp90-mirror-gpu-bracket-build-a.json` 已为 `verified_private_diagnostic`、**34 项检查通过**，含独立 GPU Probe 门控、镜像异步多槽／像素／状态／退役门禁，以及 copy／draw 各一条真实时间戳在关闭后保留的新增检查。native SHA256 **d0195e4a1c639f06fb47ab2fa3fcac273d75782fd8a96c3f292791ae26721406**，XR 仍 992bc97b…e79a；私有清单 `manifest-avp90-mirror-gpu-bracket-a.json`，默认未替换。

有限隐藏父进程 py **35248**／Python **31064**，状态 `avp90-mirror-gpu-bracket-20261023-065543.status.json`；通过构建后重新累计 20 次稳定空闲，至多一局、60 个连续玩法样本，无定时任务。只读采样已有 atomic session-status 的镜像计数，用原生 QPC 到外部实际读时刻约束整数计数在准确 XR 窗口端点的上下界，缺失／陈旧／身份不符不作 0；三项计数策略和两项筛选策略检查通过。预先固定 `C=(copy/s×copy均ms＋draw/s×draw均ms)/distinct XR/s`，以计数上界计算保守 C；若 C<0.5ms/新图且两类 P95 均<0.5ms，则结束镜像方向，转取原游戏 GPU pass 成本证据。C 包含调度／屏障影响，不是可直接回收的帧时间。若持续显著较高，候选是只复制 shader 实际读取的左眼快照，须另做正确性和同画质收益验证，当前未实现或集成。

07:07 首轮镜像诊断已正常结束：`fps-resolution-4k-avp90-default-070428-20261023.json` 的 60 个连续玩法样本、会话、退出、最高画质保留、无并行游戏／autoplay 及构建身份均通过；父／子／game 20312 均退出。独立分析 `avp90-mirror-gpu-bracket-20261023-065543.status.mirror-gpu.json` 使用同一 59.418 秒 XR 窗，共 2757 个不同成功提交图像、**46.400073 FPS**，诊断版数值不作为优化收益。15 个镜像 copy GPU 区间（前／后半窗 8／7，全部 copy slot0）中位 **0.068320ms**、均 **0.196305ms**、P95／最大 **1.489248ms**；其中 13 个为 0.061792–0.071104ms，另两个 0.592672／1.489248ms 分别位于两半。25 个 draw 区间中位 **0.058080ms**、均 **0.056877ms**、P95 **0.061696ms**，draw slot0／1／2 覆盖为 17／8／0，未称覆盖未观察槽。所有 query 读回和容量／完成检查通过，游戏 readback 编译关闭。

精确 XR 窗口的镜像计数上下界：copy 1855–1941 次（31.2195–32.6669/s），draw 3148–3257 次（52.9806–54.8150/s）；由此 C 为 **0.197024–0.205396ms/新图**。平均较低，但 copy 的 P95 不通过预定 0.5ms 门槛，所以没有把镜像方向改成已排除。该窗 54 个 NVML 样本整卡中位 99%、显存活动率中位 18%、功率中位 393.412W，仅是利用率线索。保留两条慢样本，不能删除为“异常”或称其为确定可回收的复制耗时。

Astra Max 赞同只补一次更密分布：interval=32、每 role/slot capacity=128，每 role 至少 40 样本且前后半窗各至少 15，原 C／P95 门槛和 nearest-rank 算法不变。`fps-avp90-mirror-gpu-dense-source.json` 已在开局前固定上述规则；只改私有 Probe 的采样常量／容量和镜像测试的预期条数，仍不加等待或改画质。新构建 `fps-avp90-mirror-gpu-dense-build-a.json` 正在执行门禁，尚未开游戏。

首个密采样父 `avp90-mirror-gpu-dense-20261023-071013.status.json` 因在构建首次写出回执前读取该文件而退出，明确 `runs=[]`、未触发游戏；其进程已经退出，原失败回执保留。仅修复等待首次回执的竞态，另起有限替代 `run-avp90-mirror-gpu-dense-ready-once.py`，py PID **17780**，同时固定原失败文件摘要。它仍只允许本轮一局，构建通过后重新等待五分钟空闲。密采样后无论门槛通过、仍有尾部或样本不足，均不追加第三轮镜像测试；届时保留相应限制并转向原游戏已有 GPU 标记／计时证据，不直接给任意可重复 Execute 的游戏列表追加 query。默认清单及最高设置未变。

### 2026-10-23 07:35 — 镜像补充采样收尾，低平均成本但尾部仍未排除

密采样构建 34 项门禁通过，native SHA256 **db754618964d2658f5204ebe296882412dd5ef5b0455789f0bcbf5ceb73efdcb**。`avp90-mirror-gpu-dense-20261023-071322.status.json` 完成唯一一局：60 个连续玩法样本、60.470 秒，双眼会话／构建／设置／退出／无并行游戏及 autoplay 均通过；game 15904 及父子进程均退出。该诊断版 native 45.841 FPS，不作为收益比较。

原密采样分析器因最后一条 XR 记录晚于最后一份有效 mirror 计数而拒绝准确端点，失败由 `avp90-mirror-dense-endpoint-analysis-rejection.json` 保留，原脚本、回执、原始记录均未改。仅新增离线 `analyze_avp90_mirror_gpu_dense_overlap.py`：按时钟／身份求共同窗口，仍要求至少 55 个 XR 观察／55 秒、每 role 40 个 GPU 样本、每半窗 15；保留完整原 XR 窗口 P95 作为额外约束，不按 GPU 值选择窗口或删除慢样本，不重开游戏。

离线对齐结果 `avp90-mirror-gpu-dense-20261023-071322.status.mirror-gpu-aligned.json`：共同窗口 2683 个不同成功提交图像、**45.912362 FPS**；copy 54 样本（半窗 28／26，slot0），均 **0.152461ms**、中位 **0.064944ms**、P95 **0.514624ms**、最大 **2.698080ms**；draw 101 样本（半窗 51／50，slot0／1／2=61／40／0），均 **0.061206ms**、中位 **0.059968ms**、P95 **0.065024ms**。C 保守范围 **0.166790–0.179537ms/不同立体图像**。共同窗及完整原 XR 窗 copy P95 均未通过原 0.5ms 门槛，因此保留“平均成本低、尾部未排除”，没有把镜像升级为已排除或把左眼复制候选集成。补充轮次上限已到，**不允许第三轮镜像游戏**；主线转向原游戏 GPU pass 成本证据。

### 2026-10-23 07:41 — 既有 GPU 标记／计时调用观察器准备完成

Astra Max 建议先观察游戏已有 BeginEvent／EndEvent 和 timestamp EndQuery／ResolveQueryData，而非给可重复 Execute 的任意游戏列表插入 query。冻结私有最终源 `analysis/fps-avp90-existing-gpu-valid-source`：稳定玩法显式触发后记录连续 64 consume，有界复制 96 字节标签，保存 canonical list＋真实 Reset generation、嵌套关系、原始 Close 成败与原 Execute queue。仅 CPU 观察，不添加 GPU 命令、不读 query result；新观察的原 API 调用位于新增 frame mutex 范围之外，保留原 Reset／Execute 锁与顺序。新增五个 SDK vtable 槽用编译期 ABI 断言验证，并同时扩充 viewport 与 hook-entry 证明容量；两个未构建草稿及其摘要保留，没有运行旧容量草稿。

`fps-avp90-existing-gpu-build-a.json` 为 `verified_private_diagnostic`、**35 项检查通过**；native SHA256 **534b8a7e6b2de3cd2feb07c339f629ccb823fe50f6f6fe2a46ed413ca55d23d1**，XR 仍为默认 992bc97b…e79a。真实 DLL 转发测试的五个 API／两分支共 10 次调用、bad=0、active=0，验证参数、64 位 offset、HRESULT 与单次转发，没有解引用测试借用指针。另有前缀、失败、跨队列、边界和即时标签复制的 CPU 用例；六项 Python 分析对抗检查覆盖不同消费计数、重复 Execute、prefix、resolve 同 heap／范围／顺序、timestamp index 被覆盖，以及截断／未知编码标签。

只承认本机 Microsoft SDK 10.0.26100.0 `pix_win.h` 证明的完整 legacy 编码：metadata0 UTF16LE、metadata1 的 ASCII 子集，长度含 NUL；其他二进制版本及截断均 unknown。`avp90-existing-gpu-label-encoding.json` 固定 SDK 文件摘要，不猜 PIX3 blob。推进门槛是至少 8 个不同 consume 各自具有同名、完整、平衡、成功转发的事件，以及同一真实 generation／queue 的已提交现成 timestamp 与后续覆盖 index 的 resolve；同代共现、重复事件或失败／未 Close／prefix 不算。即使结构候选通过，仍需人工识别 pass，不能直接读取 GPU 正在改写的结果或称测到 GPU 时间。

有限隐藏单次 `run-avp90-existing-gpu-once.py` 由 py PID **41644** 启动，至多一局，20 个连续玩法暖机后只发送一次 record-only 事件并观察 15 秒；开局前仍需约五分钟稳定空闲，无定时任务。不满足既有计时推进门槛则收尾这条路线，不重复采集。普通默认仍是 5f5c8f15…cff8／已集成双槽 ready-only，最高画质与 scale 1.0 按用户要求保留；90 个新立体图像 FPS 目标未达。

### 2026-10-23 07:59 — 既有阶段画像因容量失败，转核实游戏自身计时开关

`avp90-existing-gpu-20261023-074125.status.json` 的唯一一局已退出，20 个连续玩法暖机、双眼会话、最高设置保留、正常清理、无并行游戏／autoplay 均通过，但阶段画像严格判定 **incomplete**：64 consume 全部返回，generation 表达到 8192，overflow／unresolved 均 34877。已保留的 15689 条调用中有 timestamp EndQuery 11940 条、timestamp Resolve 3749 条。这是溢出后的部分观察，不能量化完整调用率、声称全窗没有 GPU marker，或称已经测到某 pass 的 GPU 时间。失败与原始证据保留，不扩大表重跑阶段观察。

历史知识库存在 `nogputiming` 字符串，尚不证明当前 retail EXE 接受该参数或它的语义。Astra Max 建议用可逆启动参数先核实：同一轻量计数 DLL、不带参数／带 `-nogputiming` 各一次稳定玩法短窗，同时统计 TIMESTAMP EndQuery、timestamp Resolve 调用数和 Resolve 的 NumQueries 总和。预先固定阳性门槛为按 consume 归一后的 EndQuery 与实际 resolved query 数均下降至少 **90%**；只减少 Resolve 调用数不算阳性，还须 Draw／consume、视图分类、双眼图像、分辨率及最高画质可比。诊断版 FPS 不算优化收益；仅阳性后才使用无新增诊断的实际默认 DLL 做同场景 A／B／A 帧率比较。

私有计数源 `analysis/fps-avp90-nogputiming-counts-source` 与回执 `fps-avp90-nogputiming-counts-source.json` 已冻结；固定 15 秒 arm／stop 窗，只用容量无关的 64 位计数，统计转发完成并等已接纳 consume／查询离开后 ACK。标明间隔端点可能包含部分 consume，不把预转发非空 direct Draw 观察称完整 GPU 工作量。没有 query result 读回或新 GPU 命令，原 Reset／Execute／Present 顺序和锁保持。最高画质、scale 1.0 继续保留；未创建定时任务，默认未改。

08:06 计数版 **35 项门禁全部通过**，native SHA256 **b0f1d36a44f0c3eaa419cfa2780fc31f2dda7e1b1b29ae7d77fd0dd084dc9ff4**，私有清单 `manifest-avp90-nogputiming-counts-a.json`，XR 仍 992bc97b…e79a。新增计数测试覆盖超过旧容量的大量调用、Stop 时尚未返回的 query、最后 consume 排空、world 与视图有效位和 64 位溢出；真实 DLL 五 API／两路径共 10 次转发、bad=0。四项离线策略用例拒绝只减 Resolve 调用、丢渲染工作和未排空结果。私有启动副本由 `avp90-nogputiming-launch-source.json` 固定，要求 behind／mute／observe-only 测试环境和显式 0／1，只追加唯一 `-nogputiming`，普通入口未改；采样前须核验实际自有游戏 argv。

有限父 `avp90-nogputiming-semantic-20261023-080400.status.json`（py PID 43832）至多两局，每局前重新累计约五分钟稳定空闲。Astra Max 对计数区间／门槛窄审未见阻止测试的问题，并明确：外部目标为约 15 秒，原生 Stop 在后续 consume 边界生效，实际用 QPC begin／stop 衡量且要求 14.5–25 秒；Done 仅证明已接纳 consume／query 的 CPU 调用返回，不是 GPU 完成或 scope 外 Draw 返回。两项限制继续保留。

### 2026-10-23 08:21 — `-nogputiming` 语义实测阳性，进入无新增诊断默认 DLL 的 A／B／A

两局计数、正常退出与设置检查均完成，无并行游戏／autoplay；同一计数 native b0f1d36a…c9ff4、默认 XR、4K 桌面、3600×3200／eye、90Hz、最高画质与 scale1。实际自有进程 argv 分别严格为 `-NoPauseUnfocus` 和额外唯一 `-nogputiming`。对照原生窗口 **15.672593s**，832 consume 全部返回，EndQuery **478055**、timestamp Resolve **149539**、实际 resolved timestamp **299078**；参数局 **15.680269s**，933 consume 全部返回，三项均 **0**。两局 open／pending／overflow／aborted／未知视图／嵌套均为 0，原调用转发数匹配。

按 consume 归一的 EndQuery 和 resolved timestamp 均减少 **100%**，预转发 direct Draw／consume 比例 **0.994398**，两类视图比例最大差 **0.013952**，通过原设门槛。只证明当前支持 retail EXE 的该参数确实关闭这些既有 timestamp 调用；**尚不证明帧率收益或可回收 GPU 时间**。诊断版 FPS 未作收益比较。

`avp90-nogputiming-semantic-20261023-080400.images.json`：两张早期 SBS 均 7200×3200，左右眼粗图平均 RGB 绝对差 4.003／4.425（0–255），在既定 6 以内。协调者实际查看两张预览，同一房间／相机／材质／光照、双眼完整且有视差；NPC 动画位置不同，无明显空眼或渲染缺失。`*.status.visual-review.json` 保留独立评审，不改原回执；这是早期粗查，不能冒充准确 ROI 的像素或 GPU 工作量等同、头显或 FPS 验收。

无新增计数／query hooks 的实际默认 DLL **7571fee6…bab9** 与 XR **992bc97b…e79a** 已通过条件预检。有限隐藏父 `avp90-nogputiming-clean-aba-once.py`（py PID **60528**，08:21:42）只改变私有启动 argv，按 A无参数／B参数／A无参数三段，每段前约五分钟空闲、60 连续玩法样本；FPS ROI 之后再只读核验该局实际命令行。预先固定默认两端漂移≤5%、候选超过更快对照至少5个不同成功提交立体图像FPS、第一批Draw及早期图像合理，随后仍须人工复核。未创建定时任务，普通默认和最高设置未改，90FPS目标仍未达。

### 2026-10-23 08:34 — 无参数对照启动被绑定 guard 拒绝，保存失败后只准一次全新组替代

首组 `avp90-nogputiming-clean-aba-20261023-082142.status.json` 为 **failed**，仅尝试 A 无参数，**没有有效 FPS 样本**。game PID33760 的 `anchor_startup_guard` 为 expected_identity=1、admitted=0、attempts=1、initial_binding，尚未进入严格时间戳阶段；before_stamp=0 不能解释成时间戳瞬态，现有日志没有具体失败字段。默认冻结、计数冻结和当前 `anchor_observer.h` 的 SHA256 均为 **433935326c54f1d36a0cfd51e9dd3e9570e470915037eaab4bbf6be0e2ec71a7**，Python相关源文件仍为前一日版本，无 ABI 漂移的直接证据。未绕过 guard，未同进程重试激活。

当前会话 native_started／native_cleanup_verified 均 false；`session-status.txt` 留着历史 PID38960，**不能用它宣称 PID33760 清理通过**。游戏和父进程实际已退出，探针 remaining_game_pids=[]、最高设置核对通过；这些事实与 native 清理证明缺口分别保留。`avp90-clean-activation-failure-20261023-082825.json` 将17份证据（含当时 capture.ini、anchor/UI证书及历史状态）复制到独立忽略目录并固定摘要，原失败回执不改。

Astra Max 窄审建议允许一次新的进程／证书、完整从 A 开始的 A/B/A，因为没有 FPS 样本可供挑选，也未降低准入；任何新一局再拒绝就停止整组，之后从原来失败读取传出 PawnBindingDiagnostic 的 stage／field／expected/actual，禁止事后重读冒充现场。`avp90-nogputiming-aba-retry1-source.json` 预先固定此停止点和失败摘要；有限替代 py PID **37172** 于08:34:30启动，额外至多3局、包括失败局总尝试上限4，无定时任务、测量与所有原门槛保持。普通默认、最高画质及 scale1 未改。

08:48 替代组 A 已通过：子状态 `fps-avp90-once-20261023-084452-885465.status.json` 为 verified；60 个玩法样本，ROI 60.189s、XR 实际计数窗口59.4114s，2797个不同成功提交图像、**47.078507FPS**，XR89.999563Hz，transport丢弃／非法key均0。实际 argv 为单独 `-NoPauseUnfocus`，会话、退出、画质／scale、构建、外部负载检查均通过。离线按真实中间观察点划分所有原区间，前／后半为 **46.904149／47.258894FPS**；不插值、不删慢样本。该窗 GPU54样本中位99%、消费线程CPU约0.518核，不能据此单独证明某个具体pass。当前B参数局仍等空闲，尚无完整A/B/A收益结论。

08:57 B 参数局也通过：`fps-avp90-once-20261023-085342-014349.status.json`，60 连续玩法样本，XR计数窗口59.4553631s，2980个不同图像、**50.121635FPS**；前／后半 **50.265344／49.972491**，实际 argv 唯一多出 `-nogputiming`，Draw首批中位1123（A为1116）、默认 native/XR 不变。双眼会话、退出、设置、并行负载和实际命令行核对通过。B比A高3.043128FPS（约6.46%），目前低于预设保守的5FPS增益门槛，**不能宣称该门槛通过或已达到90FPS**。同窗GPU仍中位99%，消费线程约0.529核；CPU时间不能直接相减当GPU成本，local fence均墙钟2.552ms也不是GPU渲染时间。A2仍排队，普通入口尚未启用参数。

### 2026-10-23 09:22 — 开关路线按原门槛收尾，不集成；转实际 Execute 批次 GPU 诊断

完整替代组 `avp90-nogputiming-clean-aba-retry1-20261023-083430.status.json` 已完成三局，均60有效玩法样本、正常会话／清理／退出、最高设置、无并行游戏／autoplay和实际argv检查通过。A／B／A2 为 **47.078507／50.121635／46.780722FPS**，两端漂移 **0.634534%**；B比两端均值高 **3.192020FPS**、比更快端高 **3.043128FPS**，不通过预设5FPS门槛。A2前／后半 **46.526645／47.034500FPS**；首批Draw中位1116／1123／1113，比例1.008985。

早期图像自动门槛也未通过：A与A2左眼粗图RGB差 **6.058594>6**。协调者实际查看全部三张预览，A首次SBS左眼有强烈模糊、右眼清晰，B与A2两眼清晰；这是实质图像差异，不能当NPC动画抹掉。首次图像不证明模糊持续到FPS测量期，也没有稳定ROI图像证明能排除它。**原组 quantitative_gate_passed=false、integration_approved=false 保留，不能靠事后补拍改判通过。** `-nogputiming` 语义关闭时间戳已证实、FPS改善有线索，但本轮不晋级为默认修复，90FPS未达。默认仍5f5c8f15…cff8／native7571fee6…bab9／XR992bc97b…e79a。

一组晚期视觉补证脚本已准备并于09:21:04排队，但Astra Max复审建议不为开关晋级追加该两局。确认父Python11272（py58076）身份、runs=[]、waiting_for_idle且仅2个空闲检查后，于09:21:56只结束该自有等待进程，**没有启动游戏或补拍**。`avp90-nogputiming-late-visual-20261023-092104.status.cancelled.json` 与pre-cancel副本保留取消证明；原草稿和原失败数据不改。没有定时任务、没有覆盖用户设置，最高画质及scale1保留。

下一项采用Astra建议的更窄范围：在一个真实 `hook_execute` 批次紧邻原始提交的前／后，插入自有预录制timestamp列表，以自有fence完成才读回／复用。不能把整个CPU consume前后GPU时间戳跨度当纯GPU工作，因为会包含CPU录制／提交空隙。Astra从既有记录首／中／末三个注册双眼consume抽取64／63／61个明确关联generation，全部Execute在消费返回前，最后提交分别早约0.302／0.375／0.295ms；这些是局部CPU提交证据，不能代表mixed／unknown。新诊断仍须声明批次跨度可能含调度与其他队列影响。**begin一旦提交即在途，后续原调用异常、end缺失、world改变或Signal失败必须保留／detach，绝不能按“没有end”提前释放。** 目前尚未构建或运行该诊断。

### 2026-10-23 09:45 — 单批 GPU 诊断已构建初版，游戏前修正并发归因

用户补充“结束后不用恢复原设置”：后续保持当前最高画质和 `render_scale=1.0`，不恢复早期低画质；普通入口音量仍按原有策略恢复。当前最高设置摘要仍 de30ad4c…1f3e，默认清单仍5f5c8f15…cff8。

私有 A `fps-avp90-execute-gpu-build-a.json` 为 **33项门禁全部通过**，包含真实 D3D12 双独立槽、故意延迟完成、GPU完成前禁止Map／复用、缺end／Signal失败的CPU隔离、元数据和既有连续会话检查；native为cc52425d…9268。**A未启动游戏，不作为可用归因版**。Astra窄审发现 `Submission.count` 仅在prebirth设置，正常birth_ok路径保存refs但count仍为0；又指出条件frame_lock及bypass提交会使三次Execute非原子。B冻结了按原始n保存已有refs及unknown／截断的修正，但未构建／测试。

最新私有C `analysis/fps-avp90-execute-gpu-c-source` 在上述修正上，明确只在 `frame_guard.owns_lock()` 时触碰诊断非atomic状态；所有hook最终原提交（含bypass／未持锁路径）经过只计数的atomic entries／active观察，自有两marker排除。begin前已活动、区间内其他／递归提交或结束仍活动均标污染；污染行保留，但不进入单批分布。没有为三次提交新增互斥锁。只在整数hash命中后查询队列身份、保存refs，避免对每批建立完整元数据。原始游戏count／成员数组和一次转发、Reset／Execute／Present证明及原锁顺序保持。

预先固定60秒单次观察、8槽、512行、hash mask2047，至少60条无污染有效行且两半各25；原mask4095按既有默认约315k次Execute／百秒记录，存在稀疏窗不足60行的风险，故在任何游戏采样前调整，未据FPS选值。六项离线对抗检查通过，覆盖污染剔除／保留、早读、身份／容量、hash选择、refs和并发证明。隐藏builder py PID26300于09:42:10启动C的构建及门禁，尚未启动该诊断游戏；普通默认未改。所有批次跨度仍含CPU提交间隙、GPU调度及其他队列影响，不是纯pass成本，也不能直接相加当可回收帧时间。90个新立体图像FPS仍未达。

09:47 C 已为 `verified_private_diagnostic`，**33项门禁全通过**，native SHA256 **c2f0c167bf9421e1537187e1fce26ad708e4cf622c20cef78b1fd3aa16b1b16d**，私有清单 `manifest-avp90-execute-gpu-c.json`，XR992bc97b…e79a不变。Astra复核C未发现阻止既定单局测试的新增问题，确认epoch覆盖、顺序、持锁准入和clean样本门槛；它只排除已覆盖的Execute转发交错，不证明没有queue Wait／跨队列依赖／调度停顿。有限隐藏父已启动，先累计20次稳定空闲（约五分钟）后至多一局；不创建定时任务，最高设置及scale1保留，普通默认未换。

### 2026-10-23 09:58 — 103个实际GPU批次取得有效结果，尚未归属具体效果

唯一一局已完成，父 `avp90-execute-gpu-20261023-094746.status.json` 为 `analyzed_sparse_execute_gpu_batches`；game42064／XR60648／父Python55120及py19480均退出。20个连续玩法暖机、双眼会话／清理、原启动argv唯一 `-NoPauseUnfocus`、最高设置逐字节保留、scale1、无并行游戏／autoplay全部通过。`*.status.gpu-window-review.json` 独立将全部样本映射至实际native QPC窗：**60.2774893秒**内59个连续玩法观察，覆盖59.11423秒、同world1／4K输出、native fresh，通过；没有按速度删样本。仍是模拟器诊断，不是头显或FPS收益测试。

`fps-resolution-4k-avp90-execute-gpu-095312-20261023.execute-gpu-analysis.json`：原观察窗191872个转发serial、hash选中103个，**103个全部完成且clean**，前／后半50／53；busy／capacity skip／other queue／unmatched／errors／slot errors／pending／retained均0，103次Map均在自有fence完成后。GPU队列跨度中位 **0.096096ms**、均 **0.289142ms**、P95 **0.676928ms**、最长 **9.042976ms**；两半均0.346147／0.235364ms，最长尾部在前半，不能据一个9ms尾部断言常态瓶颈。原始driver调用CPU中位 **0.0151ms**、P95 **0.0336ms**，三次提交CPU区间中位 **0.0538ms**，仍分别保留，不直接从GPU跨度中相减。

102行来自游戏caller0x1401aa4fc、一行来自原生Present内路径；所有行的**调用线程TLS**均inside_consume=0／ticket0，不能等同于未关联游戏帧或未执行双眼。最长9.043ms批有3个已知generation成员，driver CPU0.0165ms、提交CPU0.054ms；另两条长批约1.989／1.894ms，各5个成员。尚无命名pass／shader角色，不作整帧成本求和或可回收预算，亦不把GPU调度／queue依赖排除。结果已送同一Astra咨询下一项最小角色归因，禁止重复一局只有相同跨度数据的测量。普通默认5f5c8f15…cff8保持，90个新立体图像FPS未达。

实际使用计时槽覆盖slot0 **98行**／slot1 **5行**，353个generation成员均已知、无截断；不宣称其余6个备用槽都在这局覆盖。现有知识库对caller1aa4fc的历史查询命中sub_1401AA3D4及早期双线程队列记录（docs17）；历史伪代码存在不可可靠解释的跳转／类型，未开新IDA、未由RVA相同推断当前字节或GPU pass。

### 2026-10-23 10:17 — 角色版B准备完成，保持真实generation及未知边界

Astra选择在同一GPU bracket附加成员角色，暂不靠降低Ultra筛选。私有角色表固定2048条live录制、每代8个不同主RT／尺寸格式／mip／视口scissor元组，以canonical identity＋原tracker实际Create／Reset generation关联；ARM前没有观察到的录制为unknown。Draw摘要使用原已有identity／绑定／状态，没有逐Draw新COM查询或新hook；记录的是CPU pre-original非空direct Draw观察，不等于实际GPU Draw总数。PSO／Dispatch明确unknown；主RT不能代表其余MRT，未观察Draw不能称空批次。

原Execute返回后只给纯CPU条目标“已提交”，不冒充GPU完成。已提交条可回收；回收或失效后遇到同代Draw／Execute不得补建完整记录。Astra指出A中“已提交但未回收的同代又见Draw”仍保留旧valid的问题；B修复为立即失效并计post_submit_draw_invalidations，只有真实新代恢复，已冻结采样副本不变。容量损失为全窗角色行动阻断；不扩大表追逐采样。角色签名保留原批成员数、每成员角色元组重数，不能把集合并集当“一遍／两遍”相同工作；混合批次跨度不分摊。

A构建在首个空闲检查发现其他GOWDE游戏、CPU33.8%，`checks=[]`、**任何编译或角色游戏之前停止**，保留失败。没有操作该游戏。10:16对方退出后，B隐藏builder py PID30704启动，源码及证明 `analysis/fps-avp90-execute-roles-b-source`／`fps-avp90-execute-roles-b-source.json`，普通默认不动；Astra复核B未发现新增生命周期或接入阻断。两次源码准备断言失败发生在忽略目录、未构建，原部分副本保留于prepare-failed目录，不影响公共源码。

六项新增离线分析检查通过，覆盖真实ARM水位、角色计数／溢出、MRT未知、全clean分母及重复元组；既有六项GPU括号策略亦保持。预先固定至少60clean GPU行且每半25；角色线索需同签名前后各3行，并且两半各占**所有clean批次**≥10%，或两半跨度中位都≥0.5ms，同时全窗capacity_losses=0。该门槛仅允许定点审查，不证明冗余、不能直接跳过pass。当前尚无角色游戏结果，最高画质和scale1继续保留，90目标未达。

10:22 B 构建及**34项门禁全部通过**，native SHA256 **b9bddc007d52b86e64ae133793a3121aec1614d762a4c88036c0662efc8e42cf**，私有清单 `manifest-avp90-execute-roles-b.json`，XR仍992bc97b…e79a。有限父 `avp90-execute-roles-20261023-101803.status.json`（py55960）已由waiting_for_build转waiting_for_idle，runs=[]，重新累计20次稳定空闲后至多一局；没有定时任务。默认5f5c8f15…cff8、highest de30ad4c…1f3e、scale1保持，尚不宣称角色归因或优化收益。

### 2026-10-23 10:31 — 角色B单局完成，但没有通过可行动角色门槛

`avp90-execute-roles-20261023-101803.status.json` 为 `analyzed_sparse_execute_graphics_roles`，唯一game53900／XR及父子进程均退出。准确原生窗 **60.0288051秒**，**107条全部clean**、前／后50／57；59个同窗连续玩法观察覆盖59.16918秒，双眼会话、清理／退出、无并行游戏／autoplay、原argv、最高设置及scale1均通过。GPU跨度中位 **0.114560ms**、P95 **0.467264ms**、最长 **7.579616ms**，诊断FPS未作为优化收益。自有fence完成后才107次Map，pending／retained／容量skip／并发污染全0。

角色表记录1,430,837个实际开代、701,954个尚未提交就被新Reset替代的录制、5,746,133次非空direct Draw观察。434个采样成员全部找到精确代，prefix／missing／generation mismatch／orphan／post-submit-draw失效／eviction／capacity loss均0。**31个clean批次角色完整可解释，76个含未知或混合覆盖；角色行动门槛false，候选0。** 非互斥原因：57批含“未观察Draw”的成员、22含未知Draw、21含MRT、1含每代tuple溢出。这些不表示批次空白，也不允许丢掉未知成员后把余下跨度分摊。

最长7.580ms批有3成员：一份未观察Draw；一份40次Draw／4tuple，7200×3200格式26／10、左右3600×3200；另一份1次左眼半幅Draw。2.310ms的3成员批也含未观察Draw，另两份为21／1次、在7200×3200目标中使用3840×2160视口，录制ticket未知；这只能作为工作身份线索，不能宣布多渲染了可删除桌面场景。三条最长批CPU三次提交分别约0.055／0.061／0.061ms，GPU跨度仍可能含依赖／调度。

已完整分组中最多4条，前后分布1／3或3／1；没有满足预设两半各3加频率／中位门槛的角色。没有改门槛、放大样本或再次运行同方案，已向同一Astra咨询未观察Draw成员及更窄单列表成本的下一项最小行动。当前普通默认、最高设置和scale1继续保持；90个新立体图像FPS仍未达，未称头显实测或完成修复。

### 2026-10-23 10:58 — 单次操作分类诊断准备，保留最高设置

按用户“结束后不用恢复原设置”，继续保留当前最高画质、scale1；保护测试的起始快照本身已是最高设置，不恢复历史低档。Astra建议先辨别未观察direct Draw成员内的计算／间接／复制类操作，暂不向原游戏列表插入时间戳，避免重放、跨队列和query归属风险。沿角色B增加唯一Dispatch hook，复用已有ExecuteIndirect及复制／清理／屏障／resolve等hook；按实际成功安装冻结11类覆盖，另5类保持未观察。计数仅CPU pre-original，不等于GPU执行次数或工作量；原参数、一次调用、持锁准入、generation、GPU括号fence生命周期不变。

私有操作A完成36项门禁，未进游戏。复核发现旧角色summary仍写死dispatch_observed=0，B仅修正为实际安装覆盖位并独立冻结；10:57隐藏builder py19252构建B。A／B均未集成。B源码证明沿用的前代dispatch_observed=false字段以独立probe证明明确说明其历史含义；实际运行必须Dispatch覆盖和修正后的summary同时成立，不把未覆盖计数当零。

新增六项离线检查通过，包含未观察类、零rect全表面clear、间接最大命令数语义及所有clean分母。预先固定原20玩法暖机＋60秒观察、至少60clean且每半25；操作候选要求完整批成员／真实新代、前后各3，同签名两半各占全部clean≥10%或跨度中位均≥0.5ms，并无角色容量损失，只作为后续审查线索，不能删pass或把整批跨度分给某成员。有限隐藏父py42220等待B的36门禁和约五分钟空闲后至多一局；不创建定时任务，不操作并行游戏。普通默认5f5c8f15…cff8、highest de30ad4c…1f3e保持，90目标未达。

11:02 操作B构建及36项门禁全部通过，native SHA256 **bd2e91b8b5c64539535cf2d5829661b7aa11e23b91c4b52a89bab9705a74581f**，私有清单 `manifest-avp90-execute-ops-b.json`，XR992bc97b…e79a。有限父继续等约五分钟连续空闲，至多一局；该诊断没有集成默认，尚无运行归因或性能结论。

### 2026-10-23 11:16 — 操作分类单局通过，未观察Draw成员仅见屏障，不允许删除

等待中曾检测到其他GOWDE实例，空闲计数安全归零；未干预它。后来20次稳定检查通过，唯一game59752／XR57232启动。父 `avp90-execute-ops-20261023-105822.status.json` 已为 `analyzed_sparse_execute_operations`，父子／游戏／XR均退出。20个连续玩法暖机、实际native窗 **60.3323036秒**及其中60个玩法观察（覆盖60.1869562秒）、同world／4K／fresh、无并行游戏／autoplay、正常argv、双眼会话清理、highest逐字节保持及scale1通过；独立 `*.status.gpu-window-review.json` passed。诊断FPS不作优化对照，默认没有更换。

`fps-resolution-4k-avp90-execute-ops-111154-20261023.execute-ops-analysis.json`：**107个全部clean，前后51／56**，477成员精确generation，marker错误／busy／容量skip／pending／retained全0；11类安装覆盖2047，CPU pre-original观察715,742次，orphan／post-submit／uncovered均0。GPU整批跨度中位 **0.106368ms**、P95 **1.590688ms**、最长 **7.924704ms**；仍含CPU入队间隙、调度／队列依赖，不是纯pass成本。

74个valid未观察direct Draw成员各有一次ResourceBarrier，覆盖的Dispatch／Indirect／copy／resolve等计数均0，另5类API保持未观察。最长7.925ms混合批含3成员：未观察Draw者一次28个barrier；40次Draw成员另有6次Dispatch与7次barrier；最后一份1次Draw。因此不能把整批耗时归给屏障，更不能直接删掉它。角色仍有2个tuple溢出成员、82个含未知Draw成员，未知保留；34批角色可解释、73批混合／未知。**操作与角色行动门槛均false，候选0**，不放宽门槛或再跑同类分类局。

Astra建议停止继续细分稀疏批次。只有同时记录相邻成功发布的GPU时间、CPU完整入队时间及可靠GPU/QPC对齐，才值得下一项诊断；仅consume入口至发布出口宽区间会再次混入供给等待，不实施。已回传本局结果，请其复核是否能复用自有共享copy尾时间戳作为边界，避免增加marker提交。可靠校准不足或无法核对发布顺序时否决；不改变系统GPU电源状态，不把时间跨度减11.11ms直接当可节省预算。当前90目标仍未达到，最高设置保留。

### 2026-10-23 11:32 — 相邻发布时钟诊断已验证构建，等待单局

Astra确认可复用现有共享copy尾时间戳，不要求在Signal后额外提交marker。新私有源码 `analysis/fps-avp90-publication-clock-source` 从已验证copy bracket复制582文件，仅改timestamp header、GPU share接入和原GPU测试；沿用copy列表、原ready fence完成才Map、独立slot复用与失败隔离，不新增GPU提交／等待。每32个实际发布采serial余数0／1的连续两点，每槽256行；记录同queue／session／world／generation、ticket、capture attempt／busy累计，以及原Execute前后、ready Signal成功返回、发布元数据提交后的QPC。只有最后成功后accepted；采用更晚的committed_qpc作全入队判据，比Signal返回更保守。

每边界保留两次GetClockCalibration原始pair、CPU调用包络及CPU／GPU频率；不修改GPU电源状态。包络宽／前后及跨边界漂移会使该对不确定；观察到的一致性不冒充硬件误差严格上界。预先固定额外提前入队余量至少1ms、GPU区间超过11.11ms加至少0.5ms／观察误差，至少60个有效对、前后各25；行动需两半各至少6个阳性且各占**全部采样对（含不确定）**至少20%。未对齐／阴性不证明CPU瓶颈，跨度不能相减当可删工作或批准Reset解锁。

七项单对与六项整体分析检查通过；首个整体fixture的ROI中点使后半只有24对而正确触发门槛，修正离线fixture边界后通过，未改真实门槛。原双slot GPU测试保留延迟ready／removed值／不早Map／精确key证明，并检查新增CPU／校准记录。Astra只读复核未发现接入或生命周期阻断，强调总体门槛由上层执行，现已实现。

11:31 构建及**33项原生检查全部通过**，native SHA256 **1d153fcd8029cb8731f62951775911c5cb3f67cfb676b9739beeeee50e18314e**，私有清单 `manifest-avp90-publication-clock-a.json`，XR992bc97b…e79a不变。隐藏有限父py58416启动，先20次稳定空闲后至多一局；证明 `avp90-publication-clock-probe-source.json` 固定分析策略及分母，非定时任务。普通默认5f5c8f15…cff8及highest／scale1不变，当前无此诊断游戏结果，90目标未达。

### 2026-10-23 11:42 — 相邻发布取得稳定的已提前入队长区间线索

唯一一局已完成，父 `avp90-publication-clock-20261023-113159.status.json` 为 `analyzed_publication_clock`；game19620／XR17260／父子均退出。60个连续有效玩法观察覆盖 **60.6805151秒**，正版EXE、私有native、原XR、双眼会话及清理、无并行游戏／autoplay、最高设置逐字节保持和scale1全部通过。独立 `*.status.clock-window-review.json` 校验报告、原生、遥测、脚本哈希及同world／4K／fresh连续窗口，passed；仍是模拟器诊断，非头显或提帧对照。

`*.status.publication-clock-analysis.json` 保留ROI **87对（前44／后43）**；84对时钟与发布关系有效（43／41），另3对含busy／capture attempt间隔，保留在全部分母。观察校准模型下，**47对（26／21）**符合“下一发布完整入队比上一GPU尾点至少提前1ms（已扣观察误差）、随后GPU区间仍超过11.11ms加余量”；占两半全部采样对 **59.09%／48.84%**，通过预先6对／20%门槛。阳性提前入队净余量中位 **5.557116ms**、最小1.04818ms，阳性GPU区间中位 **19.071392ms**。两半均反复出现，不依据单次尾部。

84个有效相邻区间中位 **19.559760ms**、均20.673727ms、范围17.618496–29.601728ms；全部87对最大47.40272ms含跳过，不当单个游戏帧。校准调用包络中位 **0.00485ms**、最大0.0373ms，没有以“前后一致”宣称厂商保证的误差界。该结果支持下一步优先图形队列工作／依赖调查，不能把19.6ms叫纯场景GPU成本，也未排除queue Wait依赖后续CPU／其他队列、GPU调度或外部图形负载。没有给出可删除毫秒或Reset解锁许可。已交同一Astra选择一个最小下一行动，避免再做宽区间计时；普通默认、highest／scale1保持，90目标未达。

### 2026-10-23 12:25 — 有界主队列等待核查D通过36检查，单局排队

Astra选择先查一次主队列Wait依赖，暂不扩展shader／PSO。新增Queue Wait／Signal两个观察点；全部原Execute／Wait／Signal转发计epoch，自有marker和完成Signal以TLS排除。只给抽中的入口未满足Wait套原已验证的8个不可变marker槽／自有fence；begin立即拥有GPU生命周期，缺end／Signal失败保留隔离，完成后才Map。诊断atomic try-owner仅保护缓冲，失败时原调用继续，不增加游戏互斥锁。64类以有界保留的canonical fence身份和caller关联，指针不在测试期间重用；Signal稀疏及命中在途fence记录，CPU Fence::Signal未覆盖、未匹配来源仍未知。每次真实发布主队列的方法入口须与安装target一致，零调用结论只限这条已核对的队列。

A／B／C／D都独立冻结。B增加已有实际driver转发活动时跳过，防潜在重入；B构建检查在28项时发现其他GOWDE实例而安全停止，未启动游戏、未干预对方。Astra发现首检后抢占、Stop／Done后迟到线程仍可能提交marker的竞争；C在取得诊断owner后再查armed／stopped／queue／world，Done等已进入的Queue回调退出。D再加真实queue入口匹配和实际Stop交错回归：让真实hook在首检后暂停、置Stop再恢复，验证不增加采样且原HRESULT仅返回一次。原C资源／转发逻辑保持；D共590文件。

12:22 D构建及**36项原生检查全部通过**，包括新ABI／非阻塞owner／固定密度／保守计数、真实DLL四次Wait／Signal精确转发、Stop交错三项。native SHA256 **dc5c35c369419aa5b712584b6f6c238e79423a0f4d3c1528f431228ddb5ae89c**，清单 `manifest-avp90-wait-dependency-d.json`，XR992bc97b…e79a。Astra只读复核C后未发现另一阻断，认可D的实际交错及入口覆盖补充；未由顾问代测。

分析另有**16项离线检查通过**，包括有／无Wait、未覆盖／未知、计数边界、并发污染保留及正向GPU记录。固定20玩法暖机后60秒，按暖机未满足Wait频率选择power-of-two mask、目标256样本，ARM后不按耗时改密度。全量计数有ARM／30秒中点／Stop快照，回调边界不确定上界保守扣除。推进需同依赖两半各至少3个clean，且按全频率／固定hash推算的较小值、缺失污染当零，并扣整个CPU提交包络后仍各≥3ms／成功原生发布；仅为调查生产者线索，不称纯Wait、可删时间或FPS收益。无主队列Wait或未过门槛就停止这条方向，不扩大同类测量。

12:24:51隐藏有限父py45044启动，状态 `avp90-wait-dependency-20261023-122451.status.json`，先20次稳定空闲后至多一局，不创建定时任务。普通默认5f5c8f15…cff8、highest de30ad4c…1f3e及scale1保持。尚无此项游戏结果；90个实际新立体图像FPS未达。

### 2026-10-23 12:42 — Wait诊断D启动失败保留，E修正自有初始化旁路并通过36项检查

D唯一游戏20016在暖机与ARM前失败，`fps-resolution-4k-avp90-wait-dependency-123018-20261023.json` 没有有效玩法样本或采样窗；native published=0、首个command_context故障ticket2、GPU跟踪错误13，清理未验证、owner／XR退出码1。游戏及XR随后均退出、保护设置恢复且没有剩余游戏。**该失败不能得出无Queue Wait或性能结论**，原回执及源码保留，D不重复运行。

源码复核发现D的自有marker槽初始化没有沿用已验证Execute C的`StereoPublishBypass`：创建／关闭自有command list可能进入游戏跟踪。Astra Max确认明确的跟踪污染入口，与本次故障一致，但不能凭静态检查解释全部清理异常。私有E只把整个slot.prepare范围同时置于`StereoPublishBypass`与`OwnSubmission`，覆盖资源／列表创建、录制、Close及内部失败清理；不改原游戏转发、等待顺序、采样密度或判定门槛。

E独立冻结源码 `analysis/fps-avp90-wait-dependency-e-source`，构建及**36项原生检查全部通过**，native SHA256 **6830a3b8448e607c196bdd395567f48870e8a58917d78e991820bb7a3c2db6b6**，私有清单 `manifest-avp90-wait-dependency-e.json`。有限父 `avp90-wait-dependency-e-20261023-123723.status.json` 已转等待约五分钟连续空闲，最多一局，以恢复此前未取得任何样本的调查；不创建定时任务、不与其他游戏并行。构建监督状态文件沿用旧build-d命名，但实际命令、E回执及E清单均核对为E，未启动第二个构建。

普通默认清单5f5c8f15…cff8及native7571fee6…bab9不变，最高画质de30ad4c…1f3e和scale1保留。E当前只是诊断启动恢复，尚无有效游戏结果或新提帧收益，90个实际新立体图像FPS仍未达到。

12:45另核对构建监督器：真正builder和全部测试已退出，成功E回执／stdout已完整，但外层`Start-Process -Wait`及其自有console残留。确认PID、创建时间、精确E命令及不存在构建子进程后，只关闭自有console和空闲PowerShell包装器，未碰游戏或用户窗口。旧名build-d-123550监督状态因此exit4294967295／failed，仅表示包装器终止；**E构建仍36项通过**，另存 `avp90-wait-dependency-e-build-wrapper-review.json`，不把监督异常改写成编译／测试失败，也不重复构建。E游戏父仍正常等待连续空闲。

### 2026-10-23 12:52 — E启动恢复通过，等待采样17秒提前停止，不能形成耗时结论

E唯一game60960／XR37248已正常退出，`fps-resolution-4k-avp90-wait-dependency-e-124641-20261023.json` 有**20个连续有效玩法暖机样本**，会话／清理通过、owner与XR退出0、最高设置恢复且剩余游戏为空。自有初始化旁路修正取得游戏证据；诊断没有集成，也不把此局FPS作为收益对照。

原生ARM至Stop仅 **17.0846035秒**，未到60秒／中点，tail为incomplete。最后一条遥测phase仍running、menu1、world1／4K均未变；前一条fresh的published从1936跳至2031，随后一条native状态unchanged、published仍2031、rate空。探针复用`running_samples()`，任一次unchanged就清空连续后缀，因此提前报场景变化；这不能证明实际换关，也不能补成完整60秒。`avp90-wait-dependency-e-incomplete-window-review.json` 保存原回执、状态及遥测哈希和失败判定。

这17秒真实主队列Wait计数增 **1908**、入口未满足增 **1330**、成功原生发布增809，实际方法target核对通过。81个hash抽中项全部因**选择点已有原始API活跃**而跳过，GPU行0，错误／pending／retained均0；现有字段分不清同线程外层调用与其他线程并发。协调者最初提出可能包含当前Wait自身的假设，经源码及Astra复核否定：当前Wait的SubmissionEpoch只在后续forward才进入，不能减1或移除重入保护。**不能由0行说没有等待，也不能以1330次未满足说它是帧率根因。** 原60秒／重复依赖／3ms门槛不变；尚未启动另一局，已请Astra评估结束此计时路线或给一个真正能改变决策的最小下一行动。90目标仍未达，默认／highest／scale1保持。

### 2026-10-23 13:02 — 结束Wait计时分支；离线计算重复候选不准入

Astra最终复核建议关闭本轮Wait计时路线，记为当前采样方式未取得可用跨度；TLS来源分类无法在不改变原判据的情况下恢复81个样本，不值得另开一局。保留17秒失败窗口，不扩大同类测量、不放开重入、不删Wait，也未开TLS分类局。提前停止原因已在独立核查中拆分为遥测freshness，不把它改判成已证实换关。

下一项按建议仅审查已保存PSO／shader证据，先查知识库及既有证据，未打开IDA、未启动游戏。历史选中计算着色器的缓存和容器哈希、既有系统DXC身份及新离线解码均核对；程序线程组8×8×1。相关历史同列表／PSO／root代的两次调用，记录覆盖为两块左右相邻但不重叠的区域；完整资源绑定和GPU执行仍未证实。当前AVP操作B明确`pso_observed=0`，不能以Dispatch的XYZ相近将旧着色器关联到今天的混合批次。**没有相同输出子资源与重复覆盖的证明，候选不准入，不实施Dispatch合并或跳过。**

结果 `avp90-compute-duplicate-admission-20261023.json` 固定全部输入／工具／新解码哈希，game_runs=0、optimization_candidate_admitted=false；字节、DXIL及详细推导均留在忽略目录。两次离线审核脚本的CRLF／元数据索引解析失败保留在独立partial目录，修正后核查通过，不影响既有源码或运行回执。已向同一Astra回传否决及缺失关联，请其选择一项能改变实际性能决策的最小下一行动。普通默认、highest与scale1不变；90FPS仍未达。

### 2026-10-23 14:04 — DLSS Quality 私有移植已编译，图形启动自检发现共享接口重复安装

Astra选择单局原生OFF→DLSS Quality→原生OFF对照，沿用当前Ultra、4K桌面、每眼3600×3200输出、scale1、90Hz；每段至少60个连续有效玩法样本并取后段双眼图像。输出尺寸保持不等于内部原生分辨率，必须记录SDK实际输入／输出和有效区域；控件选择成功不等于SDK输出通过。前后OFF漂移≤5%、候选相对较快OFF至少增加5个实际新立体图像FPS为预定收益门槛，90FPS目标另外验收。尚未开始这项游戏测试，没有DLSS收益数据。

私有移植从当前两槽冻结源码开始，机械核对旧功能差异，保留144份当前受保护文件、全部XR／ABI8／FIFO／ready／ACK／world drain及瞄准字段；旧手动前台启动器没有启用。Astra复核发布前准入、过渡帧丢弃、构造视口、DSV接口容量和后台启动条件，未发现所审范围的新增阻断。A编译因丢失当前world状态计时器局部声明而失败；B只恢复该声明，编译成功并通过52项检查，随后`--color-copy-system`退出50。原失败回执、源码与日志均保留，没有跳过门禁或集成。

定位到`viewport_hooks()`下两次安装同一CopyResource／ExecuteBundle接口：NGX共享wrapper已包含原owned使用跟踪／未知标记及自有提交旁路，但移植遗漏旧版owned installer的两处条件。C独立冻结598文件，仅在`owned_scene_gpu_hooks.inl`恢复旧有`!GEARS_NGX_MV_TARGET_CAPTURE`条件；不放宽MinHook重复安装准入、不改变原API转发或传输协议。待原生检查和实际游戏证明；新版构建器同时保证失败输出先入回执，避免`check=True`抛错丢失自检stdout。当前普通默认清单仍5f5c8f15…cff8、native7571fee6…bab9，highest／scale1保持，90FPS未达；没有定时任务。

### 2026-10-23 14:28 — DLSS私有候选69项检查通过，三段单局脚本待最后复核

C全部68项原生检查通过，包含此前失败的图形接口安装／GPU检查及原字节恢复；native SHA256 `5697bc2cfa6a2dbd785589ae0defb9a2dec6c336051468d4d2e5f6cfaccdb346`。Astra确认当前TARGET_CAPTURE开启、DEATH_PSO_CACHE关闭时，两条共享wrapper已保留原参数、一次转发、owned观察及旁路；另指出PSO cache开启组合不在本轮语义证明内，builder明确断言关闭，没有扩大安装准入。

D只在C上增加一个独立进程DLL自检export及include，599文件冻结、生产回调不变。真实WARP DIRECT／BUNDLE列表和两个不同资源在fixture状态前创建；临时同签名原函数stub验证每次参数原样、原调用恰一次。CopyResource分别将目的／源设为owned identity，正常只增一次prebirth creation.uses，旁路不增；Bundle正常增加对应unknown／unsupported／prebirth计数，旁路不增，并核对回调及TLS深度归零。RAII先恢复trampoline与临时identity，随后才释放COM；该进程立即退出，不与GPUCheck串接。fixture不伪造游戏caller／SDK选中／GPU完成，也不证明这些行为。Astra复核未见所述范围阻断。

14:25核对D **69／69检查通过**，native SHA256 `03542cd6923df593ef92af194518a81d2d5bb3771db9301a9cf638b713fe74c5`，私有清单 `analysis/runtime/manifest-avp90-dlss-quality-d.json`，XR保持992bc97b…e79a；没有集成默认。单局脚本复用正版保护探针，约五分钟连续空闲后最多一局，三段各20暖机＋60连续玩法观察，后段ROI外各取两张7200×3200双眼图，连同自动启动图不超过原8张限制。设置按当前最高快照开始；只允许本次SR／AA／Bloom明确变更，遇外部字段变更拒绝，不覆盖未知写入；结束恢复当前最高原生设置。源码哈希、10份脚本语法及5项设置边界检查通过，实际argv固定正常计时flag=0。当前仍在最终只读复核，**尚未启动这项游戏测试，尚无DLSS FPS／实际输入尺寸／画质结论**；90目标未达，普通默认和最高设置保持。

14:37补齐Astra提出的三处测量保护：每段暖机后、ROI前与ROI后保存实际设置，仅AA两字段可不同，其余逐字节对比（包含Bloom），不能以退出时允许恢复Bloom替代三段画质一致；每张晚期截图前／完成后核对新鲜玩法遥测world，后观测须晚于图像保存时刻；私有菜单route及真正输入helper共用协作取消事件，在下一步及发送前检查，已开始的普通按键先完成必需释放，父等待退出后才收尾，不再仅terminate父Python。未确认drained时不写回设置。原手动菜单脚本保持；真实跨Python进程取消两例、5项设置边界、14份脚本语法检查通过。

14:37:32启动单次隐藏父py49844／实际python36980，`analysis/runtime/avp90-dlss-quality-once-launch.json` 固定命令、清单及创建时间；已独立核对两进程真实存活。父持共享构建／FPS guard，先最多两小时有界等待并累计20个连续空闲检查（约五分钟），之后最多一局；状态文件在准入后创建，stdout将给出准确路径。没有注册任务或定时器，没有重复游戏启动。此时仍无该项游戏数据，不能宣称DLSS已提帧或完成90FPS目标。

14:42最后复核发现取消检查位于通用按键`try`内，若在down之前抛出，`finally`仍可能发孤立up；该游戏已有release驱动选项证据。确认旧父stdout空、无任何GOW／XR、精确命令和parent匹配后，仅停止尚在空闲准入的python36980，未启动游戏；`avp90-dlss-quality-once-idle-replacement-review.json` 保留核查。把最终取消检查移至try之前后，调用实际私有`Target.send`方法、替换为零真实UI的消息记录器，验证取消在身份检查前、身份检查后／down前、已down后三种交错：前两种零消息，已down者恰有down／up一对。三例通过；未重复构建原生69项。

14:44:24用修正后源码重启唯一有限父py57920，`avp90-dlss-quality-once-b-launch.json` 和独立b.stdout／b.stderr保留新身份，启动前核对旧同命令worker已全部退出。15份脚本语法、5项设置边界、2项真实跨进程取消和3项实际发送方法交错检查通过，源码哈希重新冻结。重新累计完整约五分钟空闲，仍最多一局，无定时任务；本次尚无游戏性能结果，90FPS目标保持未完成。

### 2026-10-23 16:18 — 第一段窗口因147ms时间边界拒绝，修正选择器后重排单局

B父20次空闲准入完成，唯一正版game59976／XR36792实际运行；父回执 `fps-avp90-sr-quality-20261023-145001-050615.status.json` 最终为probe_incomplete。`fps-resolution-4k-avp90-sr-quality-145001-20261023.json` 在OFF A阶段取得80条有效玩法观测，但设置快照时第21条已存在，快照时间比固定index20首采样晚 **147.2507ms**，触发“Settings observation did not precede the FPS ROI”。未开启SR、未选定有效ROI、未拍晚期图，不能将80条补认作已完成基线或提帧结果。

owner／XR退出0、双眼会话及清理通过、剩余游戏为空、最高设置恢复、无并行游戏／autoplay；已核对全部父子进程退出。`avp90-sr-first-window-failure-review.json` 保留报告、父状态及遥测哈希，失败时的运行脚本／证明已独立保存在忽略目录 `analysis/fps-avp90-dlss-quality-first-window-failure-source`。正常启动／双眼运行证据与测量失败分开；没有改写失败回执。

新选择器按固定时间规则，取**设置检查点之后最早60条连续有效观测**，并保留紧邻前置20条暖机；不按FPS挑窗口，不降低样本数或改动场景。真实失败轨迹回放确认80条仍不足，须再等一条真实观测；合成第81条只用于选择器单元检查，明确不是游戏证据。边界／单调性及真实轨迹回放共14项通过，连同5项设置、2项跨进程取消、3项实际发送交错、16份脚本语法检查重新通过。

16:17:27确认没有遗留测试后启动唯一隐藏父py13540，清单仍 `manifest-avp90-dlss-quality-d.json`，原生SHA03542cd6…74c5／69项门禁不变；新 `avp90-dlss-quality-once-c-launch.json` 和c.stdout／c.stderr独立保存，重新等待完整连续空闲后至多一局。没有重新编译、没有注册定时任务、没有集成私有DLSS。普通默认仍5f5c8f15…cff8，最高设置de30ad4c…1f3e与scale1保持；90个实际新立体图像FPS未达。

### 2026-10-23 16:37 — 第二局在注入前启动超时；修复私有启动提示路径后重新排一局

C父 `fps-avp90-sr-quality-20261023-162243-001626.status.json` 为probe_incomplete。唯一正版game14904在固定启动期限内未加载D3D12／DXGI，241次模块检查后退出；尚未尝试Mod注入或prime，没有XR会话、有效玩法样本或DLSS收益数据。游戏／owner／父子均退出、设置恢复且没有剩余游戏。独立 `avp90-sr-loader-failure-review.json` 固定报告和本局reserve日志哈希，失败脚本已存忽略目录，未改写原回执。

源码核对发现确定的保护缺口：原提示处理器只接受公共launch_reserve.py，而本次启动器直接运行私有launch_reserve_avp90_nogputiming.py，导致它不识别本局出生回执。私有副本仅加入这一精确完整路径，保留owner／game创建时间、manifest原生身份、唯一nonce、注入前、唯一自有游戏、精确提示文字与按钮ID等条件，仍只对已核对的恢复默认提示选择No以保留最高设置。不改25秒期限、不用前台回退、不处理未知提示。**本次未记录实际弹窗，不能把恢复提示宣称为启动超时的已证实原因。**

21项零UI冷检查通过，包含公共／私有路径、近似路径拒绝、owner／artifact／nonce／重复参数拒绝、精确提示及未知提示拒绝和探针实际导入；连同既有5项设置、2项跨进程取消、3项发送交错、14项窗口选择及18份语法检查重新通过。源码哈希重新冻结，原生D仍69项通过、SHA03542cd6…74c5，未重复构建或集成。

16:36:37确认无其他游戏／XR／同类父进程，启动唯一隐藏有限父py8772，`avp90-dlss-quality-once-d-launch.json` 保存身份。重新累计约五分钟连续空闲，之后最多一个游戏会话执行OFF A／Quality／OFF C，仍每段20暖机＋60连续有效观测和后段双眼图；没有注册定时任务。普通默认清单5f5c8f15…cff8不变，最高设置和scale1保留。当前验收结论仍是默认约46–47个实际新立体图像FPS，90目标未达到，DLSS游戏对照及实际SDK输入／图像验证待完成。

### 2026-10-23 16:48 — Steam账号在空闲等待期间失效；尚未进入游戏，补齐账号准入

D父在约五分钟空闲检查结束后、调用探针前退出，stderr明确为多份用户设置且无法确定当前Steam账号。stdout为空，未创建本轮测试状态、未启动游戏或探针、未写设置。已核对py8772／python25880消失；`avp90-sr-account-wait-failure-review.json` 固定stderr和源码证明哈希，18份失败时脚本保存在忽略目录。只读核查发现Steam在16:38:59重新启动，随后ActiveUser实际为REG_DWORD零；两份配置仍在，其中最高设置SHA仍de30ad4c…1f3e，不选择其他账号或修改登录状态。

私有有限父把实际账号及配置准入加入每次空闲检查和启动探针前复核：必须有非空当前账号、定位结果文件名与该账号匹配、两次读取账号一致、原字节哈希符合当前最高设置；不可解析或读取失败使ready=false并清空连续空闲计数，不会启动游戏。6项离线fixture覆盖正确账号、错误画质、未登录、账号错配、读取间切换与定位异常，通过；既有5项设置、2项跨进程取消、3项发送交错、14项窗口选择、21项启动提示及20份脚本语法检查保持通过。

Steam未登录时，离线检查显式以`--offline-highest-snapshot`只读唯一匹配既有最高哈希的文件，证明中标记此来源，**不冒充当前账号准入**；实际游戏仍须上述账号检查。未改公共graphics_policy、游戏设置、登录状态或默认清单。

16:47:07唯一隐藏有限父py20024／实际python28072启动并已核对存活，`avp90-dlss-quality-once-e-launch.json` 固定新源码证明；最长等待两小时且最多一局，当前因账号不可用停留在游戏启动前，不重复启动测试，不注册定时任务。普通默认5f5c8f15…cff8、原生D的69项检查及最高设置保持，没有新的DLSS FPS结论；90目标继续未完成。


### 2026-10-23 — 修复手动启动的 Steam 离线账号识别，保留在线与已知旧安装准入

用户指出已登录 Steam 并要求兼容离线／在线、此前旧副本及 Windows Store 版。重新核对 Steam 自身日志：16:39:01 为 Start offline - 1，16:39:02 为 Success - OK；ActiveUser=0 不能等同用户未登录。此前16:55／16:56两次手动启动在 graphics_policy 多配置选择处失败，尚未创建游戏。本项纠正账号判断，不改写旧日志。

新增 steam_account.py：在线非零 DWORD 账号继续优先；零值才允许对活 Steam 客户端的离线选择元数据做只读回退。按唯一 AutoLogin／MostRecent 与明确离线标记选择，双标记须一致；绑定注册表 PID 的实际 steam.exe 路径、进程创建身份，前后复核注册表／进程和文件字节，限制文件大小、语法及嵌套，不按时间戳或目录顺序猜账号。不更改登录、授权、Steam DLL 或游戏EXE。

Start-DebugGame.ps1 将已校验的实际 EXE 路径传给画质检查。可选 game_settings_paths 将每个完整 EXE 路径绑定到本地 Config 内已有设置文件，支持使用不同本地配置的已知安装；换路径不沿用绑定，拒绝任意文件／相对路径／错误类型／重复绑定，私有隔离实例忽略共享绑定。默认未写入任何本机绑定，未改用户设置或存档。错误改为无法确定在线／离线配置，不再直接宣称未登录。正常GUI下一次启动直接加载修复脚本；便携清单已包括新模块，暂存／诊断导出验证不携带本机配置绑定。启动诊断补记新选择模块与graphics_policy源码哈希。

共 **193项检查通过**：账号选择6、画质策略16、安装／启动参数13、启动器76、便携暂存21、启动诊断9、脱敏诊断36、打包16；另PowerShell语法及diff空白检查通过。一条不存在的test_portable_preflight.py发现命令运行0项，未计入成功数；实际暂存测试覆盖portable_preflight及新增模块隔离导入。验证回执与源哈希在忽略目录 analysis/startup-platform-compatibility-20261023/validation.json。

本机只读预检准确选中当前离线账号配置，persisted DLSS=False且设置字节保持。Steam库内正版与历史旧副本通过既有EXE／安装配置哈希准入；项目工作副本旧路径当时不存在，该项不是构建不兼容结论。本项没有启动任何游戏，不新增在线／离线运行、GPU或头显验收；其他既有后台工作独立存在，不干预其进程。普通默认清单仍5f5c8f15…cff8、GUI仍c2356b5d…4315，native／XR冻结清单保持，未发布新包。

用户确认 Windows Store／Xbox 版尚未安装；无该平台EXE／版本／配置／启动证据，本次未冒用Steam哈希或地址放行该平台，Store版待实际安装后独立适配与验证。说明见 docs/steam-offline-install-profiles-20261023.md。


### 2026-10-23 — SteamVR启动菜单sRGB兼容修复，22项检查通过并接入默认XR

用户已把SteamVR设为OpenXR但17:09／17:10仍未进入VR。核对手动日志确认SteamVR/OpenXR实例、D3D12设备及XR会话均创建成功；随后GameScreen报 No game display swapchain format，native_started=false、提交帧0。本次已经通过离线账号与游戏预备，不把这次故障归为未登录或未选择运行时。

实际枚举[29,91,2,10,24,40,55,45,20]只包含8位sRGB候选，没有菜单原先只接受的28／87普通UNORM。新增共享菜单格式策略，在保留已有优先级下补齐RGBA／BGRA sRGB；启动菜单与过渡卡同时使用。布局继续要求所选typed或同族typeless资源，上传／读回检查按通道族覆盖四种格式，不放宽尺寸、array／mip／sample或复制边界；关卡GPU转换、跟踪和双槽协议保持。

从当前默认992bc97b…e79a对应冻结源码独立构建，避免混入其他开发XR改动。A因隐藏console未设UTF8在编译器前缀准入处停止，B发现冻结副本漏复制native头文件而编译失败；原失败保留。C补齐相同原始头文件，依赖证明529条及 **22项检查全部通过**。新增WARP GPU上传／读回覆盖四种typed格式及同族typeless的8例，每例513×3＝1539像素均零字节差异；不创建游戏／XR会话或用户窗口，未宣称合成器色彩或头显通过。

默认仅将xr_build／xr_sha256更新到tools/openxr-probe/build-steamvr-menu-srgb1，XR SHA256 5f8d67696d6617b58e0faed16da569192a33c1afd091e9e73bb6f566615fe3f5；新默认清单SHA256 52e820d26074dad2b8aa5b0b6b448d6278ab29e629183061307a253e91e766b4。native7571fee6…bab9、loader与其他清单字段保持，旧XR构建保留，未覆盖已发布包／标签。未改用户配置或系统运行时，SteamVR完整运行／头显复测仍待完成。

期间后续用户17:14局32304／XR36764实际为VirtualDesktopXR，已通过39帧启动预检并记录first_stereo_submission；它使用旧XR，是VDXR独立运行证据，不能替代新候选SteamVR验收。本项未启动游戏或干预该局，修复供后续新启动使用。证据与回执：analysis/steamvr-menu-srgb-20261023/；说明docs/steamvr-menu-format-20261023.md。当前90FPS目标不因兼容修复改变。


### 2026-10-23 — Game path可编辑下拉历史接入默认GUI

按用户要求将Game path从单行文本框改为可编辑ComboBox，保留浏览选择和直接输入，并提供基于列表的自动补全。选择历史、浏览确认、完成有效EXE输入或启动时保存本机历史；最近使用在前，最多10条，按完整规范路径和不区分大小写去重。缺失路径仍保留历史以便恢复磁盘，但不会因为输入不存在的路径而写入或替换当前有效选择。现有启动EXE／构建校验保持。

本机已填入用户此前使用且当前存在的Steam库内正版与旧副本两个路径，保留当前选中路径及其他所有配置字段。game_path_history仅留在本机config，便携暂存重新生成配置、脱敏导出排除该字段；相关21项暂存和36项诊断检查通过。GUI编译及隐藏自检通过，新增历史顺序／JSON持久化／去重／10条上限／无关字段保持检查，不启动游戏。最初通过PowerShell7反射加载Framework序列化器失败，未写配置；改用Windows PowerShell的相同生产保存函数后成功。

默认GearsVR Launcher.exe已更新，SHA256 ea3642e0f5be7211a10b6dc4792d2e020c0621f4f962d0e5e5bcb9f2a7665adf。旧GUI正在运行时仅将其映像文件移入忽略的analysis备份，再放入新EXE，未结束旧GUI或游戏进程；下次打开启动器显示新下拉框。当前SteamVR兼容清单52e820d2…66b4及native／XR保持，未覆盖发布包／标签。回执与旧EXE备份在analysis/vr-launcher-ui-game-history/。


### 2026-10-23 — 用户确认SteamVR头显已显示菜单平面，尚未进入关卡

新手动会话continuous-stereo-36536-20261023-173204-64cdda32确实使用已集成build-steamvr-menu-srgb1及SteamVR/OpenXR。菜单选择91、实际资源90，菜单和过渡卡均创建成功；37帧启动预检及startup_panel_submitted通过。后续持续等待Controller Pawn／相机，native_started=false，最终游戏进程退出；没有再次出现原先的格式选择错误。

用户随后明确确认：桌面停在主菜单／登录／确认阶段，头显能看到游戏菜单的平面画面，只是还不是立体VR。该证据确认此轮SteamVR菜单平面已实际可见；完整关卡立体渲染仍待用户进入关卡、建立玩家Pawn／相机后验证。按当前流程菜单平面先于关卡VR激活，不应把这个阶段本身称为启动失败，也不把菜单可见宣称为立体关卡／完整头显验收。未新增代码、未启动或干预游戏。独立回执在analysis/steamvr-menu-srgb-20261023/steamvr-menu-user-confirmation.json。


### 2026-10-23 — 更正SteamVR头显菜单可见性判断，转查实际显示链路

用户进一步澄清：VDXR的主菜单在头显中是大屏幕，SteamVR只有桌面窗口，头显内没有任何游戏画面。此前将简短选项回答解释为“SteamVR头显已看到菜单平面”不准确，该可见性结论撤回；原回执保留为被后续澄清取代的记录，新更正回执analysis/steamvr-menu-srgb-20261023/headset-visibility-correction.json明确引用其哈希。

17:32会话应用端startup_quad_layers=9949、格式91和资源90通过，不等同头显显示。SteamVR同一进程启动日志明确选中krvr_native为活动HMD，17:32／17:33合成器反复WaitForAcquire timed out并说明driver未取走sync texture。当前未见KRVRStreamServer进程；这些是显示路由／消费者线索，不能仅凭它们断言根因。正在向用户核对本次实际头显与连接方式；未切换驱动、未重启SteamVR或启动新游戏，仍不宣称SteamVR头显／立体关卡验收通过。

### 2026-10-23 17:52 — 修复路径历史导致默认启动器拒绝配置

用户截图及GUI日志确认17:45:08／17:49:07均在launcher.settings报Unknown configuration field并退出，尚未启动游戏。用实际保存配置直接复现，唯一未识别字段为新增GUI写入的game_path_history；此前下拉历史验证遗漏了Python启动端配置准入。

默认launcher.py现在接受game_path_history作为GUI元数据，仍仅使用game_path选择EXE；同时接受既有game_settings_paths配置入口，其路径／配置文件边界仍由graphics_policy.locate严格验证。真正未知字段继续拒绝，错误增加字段名便于定位。保留实际配置、两条路径历史、当前选择和所有画面选项；现有GUI每次启动重新加载此脚本，无需替换或重启GUI。

新增持久化配置与模拟完整启动回归，验证历史不替换当前路径、不写配置、错误拼写仍拒绝，及当前／旧清单兼容。首轮78项启动器检查中77项通过，另1项发现此前SteamVR默认清单缺少完全一致的命名候选；补齐manifest-steamvr-menu-srgb1.json原样副本后，**78项全部通过**，未更改默认manifest.json。连同21项便携暂存、36项脱敏诊断和16项画质策略，共**151项检查通过**，相关diff空白检查通过。

实际当前配置只读预检通过，并核对所选正版EXE、native／XR／Playground文件哈希；配置、GUI、默认清单前后哈希相同。默认清单仍52e820d2…66b4、native7571fee6…bab9、XR5f8d6769…fe3f5。验证脚本、回执及启动器检查日志在忽略目录analysis/launcher-config-history-fix-20261023/。本项未启动游戏或XR会话，不新增GPU／头显验收，不覆盖发布包或其他开发改动。


### 2026-10-23 — 确认VD／SteamVR被KRVR附加驱动抢占，停用冲突驱动后用户确认SteamVR界面恢复

用户明确本次通过VD连接、VD选择SteamVR且SteamVR已为默认OpenXR，KRVR主程序没有运行。SteamVR 17:10:10.891日志先将活动HMD设为krvr_native，17:10:11.035随后拒绝VD shim对应的oculus头显，原因Can't add a second HMD；原来应用的9949层菜单提交因此不能说明画面已送到用户正在用的头显。KRVR主程序未开，不代表已注册且启用的SteamVR附加驱动未加载。此处有明确路由冲突证据，不再用等待关卡解释头显空白。

确认无GOW／XR／KRVR活动会话后，经本机注册的vrmonitor://quit协议正常退出SteamVR，未强杀进程。备份退出前和退出后的最新steamvr.vrsettings，只新增driver_krvr_native.enable=false，深比较验证其余设置保持；驱动安装和注册保留，不改VD／游戏／OpenXR默认选择。17:49:52通过隐藏vrstartup正常重启，新日志明确Not loading driver krvr_native because it is disabled in settings，活动HMD为VD shim对应的oculus，未再出现第二头显拒绝。

用户随后确认头显已能正常显示SteamVR界面。当前SteamVR仍为系统OpenXR；只读尺寸查询退出0，推荐每眼3072×3264，不创建游戏或XR渲染会话。这验证了SteamVR显示路由恢复；本轮未启动游戏，Mod菜单和关卡立体画面仍待下一次实际启动验证。此前将SteamVR头显菜单判为可见的结论明确由headset-visibility-correction.json撤回，原证据保留。

配置备份、精确变更及新旧路由回执在analysis/steamvr-hmd-routing-20261023/，repair.json保留源哈希与用户确认。以后使用KRVR连接时需要重新启用该SteamVR附加驱动；本次未卸载KRVR。默认Mod清单52e820d2…66b4及GUI ea3642e0…65adf保持，未发布新包。


### 2026-10-23 — 用户主观报告SteamVR略清晰，等尺寸输出与色彩路径只读核查

用户表示SteamVR似乎比VDXR清晰一点，未要求修改画质。对照17:57:23 SteamVR会话43804与18:03:30 VDXR会话45136，两次均已激活native并记录首次立体提交；使用相同native7571fee6…bab9与XR5f8d6769…e3f5。两次实际GPU source_size均6144×3264、两眼XR image_size均3072×3264、scale1，不能归因于这两次每眼输出尺寸不同；这些尺寸也不能替代内部渲染分辨率和显示质量验证。

两次GPU快照源格式均28（RGBA8 UNORM）；SteamVR枚举无28、有29，而VDXR枚举同时含28／29。按冻结GameGpuScreen的现有选择规则，SteamVR应转换到29 sRGB，VDXR应直拷28 UNORM。该路径差异是源码和实际枚举推导，未增加所选XR格式的独立实测、未验证当前源像素颜色语义或头显合成结果；颜色空间／对比度差异可作为主观锐利感的调查候选，不能宣布已找到清晰度根因或确认哪边正确。未做同场景／同视角／码率固定的画质或FPS对照，未启动游戏或调整设置。只读回执analysis/steamvr-hmd-routing-20261023/clarity-comparison-readonly.json。


### 2026-10-23 — SteamVR左眼偶发闪黑调查，补齐连续sRGB图像轮换GPU检查

用户实际对比后确认本机SteamVR比VDXR清晰，同时报告进入关卡后左眼偶尔黑一下／像漏一帧，目前只在本Mod中观察到。清晰度主观报告按用户测试保留，不改分辨率或伽马。对其已完成的正版手动会话37508只读检查：左右眼各11316次XR图像等待，无超时／错误；这不能替代物理显示证据。同一SteamVR日志的WaitForAcquire警告在18:05:56、Mod XR客户端18:06:09连接之前已出现，不能仅据此认定Mod闪黑根因，也不能据此排除Mod。没有闪黑时刻的精确图像／提交对应关系。

新增EyeChainRotationGpuTest及CMake目标，用当前默认冻结GpuBlit检查SteamVR所走的RGBA29 sRGB路径，覆盖typed29／typeless27资源、每眼独立转换对象、每眼3张图像不同序轮换及逐帧变化的左右眼源数据。在WARP和本机NVIDIA显卡上分别运行，每组合96帧、每眼257×129，逐像素读回比较，4组合共25461504像素均通过，未发现该离屏路径的漏写、串眼或旧帧。全新构建目录、/W4 /WX及MSVC实际依赖追踪通过；不是原生生产、实际XR链或头显检查。

新增docs/steamvr-left-eye-flicker-20261023.md，忽略目录analysis/steamvr-left-eye-20261023保存原手动会话／SteamVR日志、冻结源码、独立构建和investigation.json。默认manifest仍52e820d2…66b4、native7571fee6…bab9、XR5f8d6769…fe3f5，未新增游戏测试、修改设置或替换默认二进制。**闪黑根因与修复仍待完成，本轮测试通过不宣称闪黑已解决或头显验收通过。**

### 2026-10-23 — 用户确认上下机枪切换延迟已可接受

用户反馈：“上下机枪‘完全无延迟’虽然还没有完成，但是我觉得已经可以接受。”记录为当前开发版上下机枪切换等待体验已达到用户可接受水平；就切换延迟这一项而言，不再把尚未完全无延迟单独视为0.1.0发布阻塞。

发布说明可表述为“优化上下机枪切换与画面恢复，减少切换等待”。完全无延迟仍是后续优化目标，不宣称零延迟已实现。本次仅记录用户体验反馈，没有新增运行测试、耗时测量或扩大其他功能的验收范围。


### 2026-10-23 — 按用户要求统一sRGB提交并接入普通启动器，更正局部闪白症状

用户进一步说明异常只在左眼边缘／局部出现，随后明确更正为“应该说是闪白”。以此取代先前整眼闪黑／漏帧假设；原始日志和调查回执保留，补充symptom-correction.json及调查文档更正。局部闪白的根因与修复仍未确认。

用户明确要求将VDXR也全部改为sRGB，再自行对比清晰度。游戏GPU输出现在统一选择RGBA sRGB29／后备BGRA sRGB91，取消运行时支持源UNORM时的直拷优先；菜单、影片平面和过渡卡统一BGRA sRGB91／后备29，诊断图案也选择sRGB。缺少两种sRGB格式时明确拒绝，不静默回退UNORM。新增xr_color_policy记录实际路径、所选格式及游戏源格式／是否转换，方便下一局核对。保留游戏原生源格式、现有编码值保持转换、每眼分辨率和所有原生逻辑，不把切换格式本身宣称为清晰度或闪白修复。

基于最新默认52e820d2…66b4对应的冻结XR源码，应用有限格式／日志变更，保留其他已集成修复及冻结native。新编译、实际MSVC依赖证明及24项XR／GPU检查全部通过；包含真实记录的SteamVR／VDXR枚举、菜单typed／typeless上传及WARP／本机NVIDIA连续双眼图像轮换。默认接入后78项启动器和21项便携暂存检查通过，实际当前配置及全部启动文件哈希预检通过。

默认更新为tools/openxr-probe/build-srgb-output1，XR SHA6f00c956889e89f472a4470ae592c8129f55d328d079505536fbdc1a34e50953，manifest-srgb-output1.json与默认完全一致，仅变更xr_build／xr_sha256。native7571fee6…bab9、loader、GUI、用户配置及旧发布包保持；没有结束现有进程或启动新游戏。docs/xr-srgb-output-20261023.md记录行为与限制，analysis/xr-srgb-output-20261023保存冻结源码、24项检查、99项回归与integration.json。下一次普通启动即可用新路径，实际VDXR清晰度与局部闪白由后续头显复测确认。


### 2026-10-23 18:46 — 按用户要求恢复KRVR显示路由

用户要求设置为KRVR。只读检查发现KRVR程序已运行，64位ActiveRuntime已由用户端指向KRVR XR Runtime；此前为VD排除抢占而设的driver_krvr_native.enable=false仍在，SteamVR反复HmdNotFound。确认无游戏／Mod XR会话后，通过已注册vrmonitor://quit正常退出SteamVR，备份最新设置，离线仅将该驱动enable恢复为true，深比较确认其余设置保持；没有改写OpenXR注册表或停用VD驱动。

SteamVR正常重启后18:45:46日志确认加载krvr_native、HmdDriver::Activate ok及Active HMD set to krvr_native；双手控制器也已激活。当前默认XR的只读--render-size-info --render-scale 1.0查询KRVR原生OpenXR退出0，推荐每眼4096×3288、SBS8192×3288。首次查询误传单独1.0，被参数校验以2拒绝且未进入runtime；保留空输出并用正确参数完成查询，不把初次参数错误称为运行时失败。

原设置备份、精确变更、重启及当前HMD证据保存在忽略目录analysis/krvr-routing-20261023/，switch.json记录验证。没有启动游戏或XR渲染会话，未扩大KRVR游戏／GPU／物理显示验收；普通默认仍为刚集成的sRGB输出构建，用户游戏配置保持。


### 2026-10-23 — 用户确认sRGB明显更清晰，左眼偶发闪白列为已知问题并暂缓修复

用户明确反馈：“我已经确认了，就是颜色的问题，srgb清晰很多”。记录为本机实际头显对比后的清晰度观感确认，保留当前默认build-srgb-output1；不把颜色观感改善称为渲染分辨率增加，也不扩大各运行时的完整兼容性／GPU验收。

用户同时确认左眼仍会偶发闪烁，“就好像有一帧是白色的”，并要求“先不修，记录下来”。作为此前左眼边缘／局部闪白的补充描述，记录其类似白帧的瞬时观感；没有实际异常帧捕获，不改写为整帧全白已被证实，不认定sRGB就是闪白根因。此项状态为**已知问题，暂缓修复**，按用户指示停止进一步排查、修改和测试，待重新要求处理。

已同步更新docs/xr-srgb-output-20261023.md、docs/steamvr-left-eye-flicker-20261023.md，新增忽略目录analysis/xr-srgb-output-20261023/user-feedback.json，原构建／集成／调查回执保留。本轮只登记反馈和处理决定，没有修改源代码、默认构建、配置或运行时设置，没有启动游戏／XR／自动测试；不将先前KRVR送显问题宣布已解决。


### 2026-10-23 — 按用户要求恢复OpenXR原颜色，保留SteamVR格式兼容

用户要求“openxr还是用原来那个颜色，那个没有闪烁”。结合前面的VDXR／SteamVR比较，恢复此前的能力协商规则：游戏优先采用运行时支持的源格式，支持UNORM时直接复制；源不可用时仍按原有29／91／28／87顺序转换。菜单、影片平面和过渡卡恢复87／28／91／29顺序，诊断图案恢复UNORM优先。对已记录的真实枚举，VDXR恢复游戏28／菜单87，SteamVR继续游戏29／菜单91；不靠运行时名称硬编码，不切换OpenXR注册表或驱动。xr_color_policy日志改为按实际所选格式输出srgb／unorm。

从最新默认sRGB冻结源码构建，仅6份XR颜色选择／提示／日志／用例文件发生变化，其余冻结源码保持。25项XR／GPU检查通过，包含实际VDXR／SteamVR枚举回归、UNORM双眼复制和菜单typed／typeless上传；接入默认后78项启动器与21项便携暂存检查通过，实际当前配置及启动文件哈希预检通过。没有启动游戏或物理XR会话，没有继续闪白根因调查。

默认改为tools/openxr-probe/build-runtime-colors1，XR SHA58ef6c397af8facc5ae50c8adba659005a13efd603e7bcfee73ba890d1e2539a，manifest-runtime-colors1.json与默认一致，仅改xr_build／xr_sha256；native7571fee6…bab9、loader、GUI、用户配置及旧候选保留。analysis/xr-original-colors-20261023保存冻结源码和source-a／build-a／integration回执，相关颜色与闪白文档已更新。用户关于“sRGB更清晰”及“原颜色没有闪烁”的主观反馈都保留；闪白仍为已知问题、暂缓修复，本次颜色回退不宣称其根因已修复。


### 2026-10-23 — 用户更正两种颜色似乎都会闪，撤回原颜色无闪判断

用户最新反馈：“好像都会闪，不是颜色的问题”。记录为原颜色／UNORM与sRGB路径似乎都可能出现闪烁，颜色回退未消除该现象；撤回“原颜色没有闪烁”作为当前结论，不把颜色格式视为闪烁的已知根因。原话中的“好像”保留其观察性质，根因仍未知；此前sRGB明显更清晰的主观反馈继续保留，清晰度与闪烁分别记录。

原回退请求和构建／集成回执不改写，最新更正另存analysis/xr-original-colors-20261023/flicker-feedback-correction.json，并同步两份颜色／闪白文档。继续遵守“先不修，记录下来”：闪白仍为已知问题、暂缓修复，本轮仅更新记录，没有改动代码、默认构建、颜色配置或运行时，没有启动游戏、XR或测试。
### 2026-10-24 — 独立武器 6DoF 分支补记

在独立worktree的 `codex/weapon-6dof` 继续开发，已合并发布版0.0.3；本补记不构成0.1.0或新的发布验收。按本会话提供的进度归档要求，同时在这里记录后续验证，详细来源保存在[0.0.3后续进度](../0.0.3/PROGRESS.md)。单双手均可射击、单手伤害为同条件双手0.7倍已写入[交互设计](../../weapon-6dof-interaction-design-20261022.md)；真实伤害入口尚未接线。

最新购买版后台y会话完成一次双手应用和一次完整枪体应用，第二次枪体尝试的旋转写入前因姿态源龄102.1423ms超过原100ms拒绝；部分写入回滚后，三组原始组件、手部控制、挂接、入口和8音频session独立恢复通过。后置快照第1次即完整，没有实际触发重试；此前x的最终组件快照未知仍保留。y正式协议、持续运动和整局覆盖未通过，几何审计行未生成，不能称作持续自由双手或全武器完成。封存seal e1975344…b05b、result79a0f8f4…5e7eb，exact owner／game／XR已退出；静音／behind／observe-only和实体手柄后台输入关闭的约束保持。

首次保存原值、每帧新授权更新的持续组件协议与真实类测试已加入独立分支，30场景／1326检查首次strict编译与唯一执行通过，Astra独立窄审无阻断。旧运行路径未引用新header，跨Tick持有、真实四参Backend、动作交还及实际性能仍待验证，默认入口未集成未完成候选。详细来源见上述0.0.3后续进度，不将纯CPU结果扩大为持续自由双手或伤害链通过。


## 2026-10-24 补记 — 两 Tick 持枪观察的默认 OFF 源码

独立武器分支加入固定两次最外原版PlayerTick的枪组件保留观察，首次正常联合应用后不再写新目标或生成新许可，手部先恢复，异常／动作／Stop／Shutdown仍按原协议交还。实际提取代码的首次编译通过、首次运行因fixture误期望较轻失败码而停止；仅修测试为真实原end的严格status8及日志／恢复断言后，第二次编译运行通过39场景／202检查。原失败完整保留，一组HOLD关闭的源文本MSVC预处理对比通过。五源接线默认OFF，其余774源保持；PG构建和真实跨Tick观察尚待进行，不代表持续6DoF、伤害倍率或头显验收。详细来源记录在0.0.3后续进度，本补记不构成0.1.0发布。


## 2026-10-24 补记 — HOLD 私有构建与初始预检结果

默认OFF的两Tick观察候选完整PG构建、冻结及779源闭包核对通过，未集成默认入口。z实际购买版后台运行在初始只读component snapshot的0x60对象头双读检查拒绝，尚未Start或进入HOLD／后续continuous Native-XR；前期Native预留hooks已启用。缺初始component基准，三组恢复只能保持unknown；8控制器、44字节挂接、原入口及8音频session恢复通过，root独立确认确切自有进程退出。z封存85项含seal edf7f29f…5d5e，失败不改写。新入口将处理有限完整快照重读，原guard与写入流程保持；持续6DoF、单双手实际伤害、图像／头显仍未验收，详细来源见0.0.3后续记录。本补记不构成0.1.0发布。


## 2026-10-24 补记 — 两 Tick 有限持有数据完整外部重审通过

相同私有DLL的新aa入口实际完成一次联合应用、两对原版PlayerTick持有与末尾恢复；hold期间component新增setter0，raw3／8控制器／44挂接／入口／8音频session恢复一致，确切自有进程退出。Native／XR独立通过，仍无头显或连续更新结论。初始snapshot新增有限重读的14类／114检查首轮通过，实际仅第1次成功，未演练追加尝试。

原审计误将异步GT begin限制在同步Start返回前，aa原receipt／audit失败及115项封存保持，seal1b991cc4…7fb1。另建外部auditor，按原Start请求一秒armdeadline修正时间门，11新场景／24检查首轮通过；同一封存数据的一次完整离线重审e42febd5…c038六项均过，未再启动游戏或改写原结果。默认入口未安装试验，下一步只准备默认OFF连续组件接线；持续6DoF、伤害倍率、全武器及头显仍待实现／验收。详细来源见0.0.3后续记录，本补记不构成0.1.0发布。

## 2026-10-24 补记 — 连续组件引擎接线加入独立武器分支

五份默认OFF的CONT接线源码经root审查并原字节应用，其余774源保持。实际dispatch／生命周期片段合计21类／287主机检查通过，默认关闭的新一组严格源文本/EP对比通过；原失败和测试替身修正记录保留，已通过的dispatch未因lifecycle编译字面量修复而重跑。原100ms、15秒／256次和手部先于枪体恢复的次序保持。当前仅有限Lancer区间，完整DLL构建、游戏持续更新、全武器、0.7伤害与头显仍待完成，默认入口未安装。详情见0.0.3后续进度，本补记不构成0.1.0发布。

e30e9a30…ce49随后完整PG严格构建与23导出核对通过，私有bundle0b9b8910…cb80冻结779源／27依赖／333工件／1140实体，PG9c86ef31…c021；父170份非PG工件保留，Native／XR／loader没有重建，root闭包核对通过。新的AB单区间更新运行流程已完成源码准备与独立源审，尚未启动游戏；不继承HOLD验收，不改变默认入口。

AB随后完成8类helper／parser检查并在三次空闲后实际运行一局，Start0后初始设备0→1的clean交还被PostTick自身operation_depth1误拒，terminal7／lease0，尚无枪体持有和两次姿态命令。失败保留，142项含seal87067093…3109；root独立核对raw3／8控制器／44挂接／入口／8音频session恢复和确切owner／game／XR退出。Native／XR独立35层／34拷贝通过，CONT正式审计仍false；只准备新stage修正初始阶段判断，不放宽busy drain、原100ms或已持有后的动作关闭。详情见0.0.3后续进度，本补记不构成0.1.0发布。

首次0→1交接的两文件修正现已源审并原字节应用，其余777源保持。仅never-held／全clean／depth1初始路径可获新提示；原单参、epoch／binding／本帧阻写与begun后的关闭／延后恢复保持。新14类／426生命周期检查（含18拒绝变体）首轮通过，未变dispatch11／138沿用，aggregate25／564；新OFF一组/EP通过，proof35b32c68…1105。修后PG构建与AC实际测试待进行，默认入口未安装，AB封存不变；不构成全武器、0.7伤害或头显完成。

修后PG完整严格构建与冻结随后完成，bundle2ffd36c1…e14a／PG868b898d…9b6，779源／27依赖／401工件／1208实体核对通过，父332份非PG工件保持。唯一AC实际局在同一owner记录1次取得、4次更新尝试、5次联合提交，两次手势命令后确认新输入与同身份枪体姿态变化；双手应用／恢复各5次、枪体最终恢复1次。计划Stop0／Shutdown0后终态7，原整体审计仍false；准确授权拒绝原因未保留，不能认定Stop竞态或过期。raw3／8控制器／44挂接／入口／8音频session独立恢复、确切自有进程退出，110项含seal1720be63…cc26，root-result e15c1004…f1a。详细判定及后续写前取消分类见0.0.3后续进度；不重标AC失败，默认入口未安装，不构成全武器、0.7伤害或头显完成。

随后单XR文件接入当前调用的写前授权拒绝分类，其余778源保持，原授权／时钟／恢复次序不变，真正Stop／Shutdown取消可请求6，未知／先行拒绝和既有失败仍保留7／8。受影响13类／350主机检查首轮编译运行通过，CPU e3e67d3b…1c79；新OFF唯一一组两次/EP及严格非空行对比通过，proof083cc697…660a。诊断行在运行前移出保留前缀，旧48份未执行输入保留，auditor没有放宽；新PG与AD实际局待进行。详细来源见0.0.3后续进度，不重标AC或构成头显／伤害完成。

新PG完整严格构建与23导出核对完成，bundle d2ad1c9a…20af／PG28ce0a16…73f7冻结779源／27依赖／479工件／1286实体，父400份非PG保持。唯一AD实际局在同一owner完成1次取得／3次更新尝试／4次提交，确认两组姿态变化，双手恢复4次、枪体恢复1次；计划Stop0／Shutdown0、phase6，受限scope／update／cleanup正式审计通过。本次停止走xr_stopped_after_restore，新写前取消分支没有实际触发，仍仅主机覆盖；不回填AC未知原因或失败。raw3／完整8控制器／44挂接／入口／8音频session恢复、确切owner／game／XR退出，87项含seal5a7c27c0…bc90、root-result35ceafb8…db5b。默认入口未安装未完成6DoF，完整范围与证据限制见0.0.3后续进度。

持续组件会话已加入独立分支（014cebec…b0ba、局部重名修正3fb9ae4f…eef5）：一次Start内区分session与clean interval，正常动作完整交还后可等待新输入重开，15秒／256次只冻结诊断；真正故障和未知状态仍终止。新OFF源文本对比通过并按仅ON分支两token改名的严格逆向比较沿用；修后完整PG严格构建与23导出核对通过，DLL2ff0ef1a…7b42。受影响CPU与实际游戏结果仍待记录；phase2／无右手Plan当前暂停整组writer，尚无组件暂停期间独立freehands。默认入口未安装，详细范围、首次编译失败及静态多武器／伤害边界见0.0.3后续进度，本补记不构成0.1.0发布。

随后7类／505项受影响生命周期检查首次严格编译运行通过，17实际片段／38完整输入绑定，CPU a1bfe680…5162。真实Policy和hand Lease已用，组件／asset／stability／原动作仍有声明过的host服务，不代表真实A2 acquire或整局。实际游戏与组件暂停时独立freehands仍待完成，详见0.0.3后续进度。


2026-10-24 AF补记：独立双手已接入原事务（8d167556…8611），关闭模式源文本对比、完整PG严格构建／23导出及受影响7类／539主机检查通过。唯一静音置后购买版局记录枪体5提交／1恢复、双手16应用／16恢复，含hand_only11；收枪后左右手四命令及return11 ACK完成，取回持枪验证因诊断空字段解析错误提前结束，原正式失败与84项封存不改。三组raw／8控制器／44挂接／入口及8音频session恢复，自有owner／game／XR退出。原日志重算发现部分手骨位置偏差，整段最大3.268784单位（约3.27厘米），旋转最大0.020772°；当前怀疑IK可达范围，肩肘证据尚缺，保持原动作和0.1精度继续定位。修后AG仅8项离线窄回归通过，未prepare或再启游戏。默认入口未安装，单双手0.7／1.0设计已确认但实际伤害入口未接；详情与原失败见0.0.3后续进度，本补记不构成0.1.0发布。


2026-10-24 AH补记：353f9890…f39e加入最多16条的同帧肩肘只读诊断；OFF源文本对比与完整PG严格构建／23导出通过。唯一静音后台局执行原8命令及收枪／再取枪，双手21应用／21恢复，含hand_only11；枪体两区间10提交／2恢复。11个有效同Tick记录的22个手部结果中6个超过原0.1位置门限，全部符合实测臂链径向可达上限，最大偏差4.107229单位（约4.11厘米）；物理审计仍失败，不能称自由伸手完成。清理审计通过，原字段／入口及8音频会话恢复，自有owner／game／XR退出；AH93项封存，AF原失败不改。下一步另做默认关闭的手掌位置覆盖与完整回滚实验，并检查手臂蒙皮／手—枪关系／后坐力。设计0.7／1.0已记录，实际伤害尚未接；详见0.0.3后续进度。本补记不构成0.1.0发布。

2026-10-24 AI补记：唯一静音后台局只读取得实际末端平移Vector／空间／None骨名与共享Bool字mask，原始8控件／枪组件／挂接前后相等、音频恢复、自有进程退出。该局在原有取回枪后重授权失败，正式passed与cleanup_passed均false，原结果保留，不能由独立恢复证明提升整局结论。新增平移扩展及四通道组合策略的一次实际严格CPU目标构建运行通过28类／370项，尚未接入引擎字段；内存服务不能证明物理位置或画面。没有默认安装／Native或XR重建，详见0.0.3后续进度。本补记不构成0.1.0发布。

2026-10-24 AJ补记：末端世界位置扩展已在单独默认关闭开关下接线，保留原Limb四通道、同Plan100ms和原动作清理。普通snapshot和Backend后置拒绝都会锁存该手撤权，晚到相同值不复权；完整静止Trial才可重建。两轮新开关OFF源文本对比通过，首轮严格构建因局部重名被/WX拒绝并保留；R2仅改名，完整PG／23导出与实际字段reader／SEH helper的15类／198项内存检查通过。新候选DLL459e0f67…934bf仍待游戏／物理／蒙皮和原动作验证，无Native／XR重建或默认安装，0.7伤害未接。详细证据与限制见0.0.3后续进度。本补记不构成0.1.0发布。

2026-10-24 AK补记：新位置覆盖候选首次静音后台实测在首个枪体提交后重授权返回零，尚无手骨／新增字段写入或姿态命令。实际tail五阶段记录的前四项通过，源龄已越过100ms；精确第一个拒绝子条件未保留，不能据时序猜定。独立8控件含新增typed观察、3组件、44挂接恢复，failure Stop0／Shutdown0、音频8/pending0与确切owner／game／XR退出；正式流程和清理审计仍false，75项封存8ada9ae7…93e40。下一步检查过期后有序恢复与新票据重接，保持原100ms／15s／0.1门限，无默认安装或头显验收，真实0.7伤害仍未接。详见0.0.3后续进度，本补记不构成0.1.0发布。

2026-10-24 AL补记：默认关闭TAIL内新增实际67/3/2早期截止拒绝的有序age交还，原Plan不续期、本次hand/extra须零写且owner健康；零depth恢复后才等新输入。源审两项历史applied／倒退context问题已修；独立首Auth记录不受八行计时容量覆盖。22实际片段的新12类／315项主机检查与唯一完整PG严格构建／23导出、OFF源文本对比通过，原通过项未重跑；记录器收尾路径下标错误保留并仅修正报告，无构建重跑。DLL884d9f0b…9356a待游戏验证，100ms耗时来源与物理位置尚未解决，无默认安装。伤害证据盘点补齐静态瞬发／聚合／投射物Init边界，真实0.7仍未接；详见0.0.3后续进度。本补记不构成0.1.0发布。

AL实测补记：b8bd6abb…eb98的唯一修后静音后台局在更早component acquire失败，1尝试／0提交／1恢复，fault与cleanup_failed均保留、最终dirty0；新后置交还尚未触发，手骨和扩展字段零写，8命令未开始。独立8控件／3组件／44挂接、入口及音频8会话恢复，确切owner／game／XR退出；正式流程与清理仍false，79文件封存ad31db33…a6c3ee。具体失败事件仍被恢复日志覆盖，不能拿最后prewrite PASS或中间差异定因，下一步保留内部拒绝与成本，不放宽100ms／动作／精度。无默认安装或头显通过，详见0.0.3后续进度，本补记不构成0.1.0发布。

2026-10-24 AM补记：默认关闭候选修正中间态比较提前冻结诊断的问题，并独立保留首个实际Auth拒绝。新4类／225项主机检查、完整严格PG／23导出及OFF源码对比通过；唯一静音后台实测首次触发原67/3/2早期截止拒绝的AL有序交还，clean_close1，随后hand_only7应用／7恢复、23对末端样本审计通过。初始未变化手势的14手部结果均在原位置／旋转门限内，尚未执行8命令，不能确认大幅伸手问题解决。第二次component acquire仍失败，具体内部原因未保留；首Auth pin描述已恢复的前一事件，不可移作终止原因。正式passed／cleanup仍false，独立原字段／音频恢复、自有进程退出已核对，77文件封存95598b2b…13c。Astra只读源审建议下一步保留每次实际操作的失败上下文并定点计时原callback，保持100ms／8动作／0.1门限。0.7伤害未接，无默认安装或头显通过；详见0.0.3后续进度，本补记不构成0.1.0发布。

AN源码补记：默认关闭TAIL内加入每次真实acquire／update失败上下文及原prepare／hand_direct两个显式计时点，均在原授权时钟前结束；不解除invoke暂停或改变权限／写入／恢复。实际cost／trace和SEH finally的新6类／55项CPU检查、完整严格PG／23导出及OFF源码对比通过，DLL4d50dff8…8604；尚未游戏验证，不构成自由伸手、0.7伤害或头显完成。详细边界和host服务见0.0.3后续进度，本补记不构成0.1.0发布。

AN实测补记：唯一静音后台局实际捕获首个component acquire最后group4旋转前授权截止：原前三组Auth3，末组known67／checked3／rejected2，now69612484>原ticket＋100ms截止69612475。四次原prepare约7.7–8.2ms、最终hand_direct约2.5–2.7ms，尚未隔离具体reader成本。组件0提交／1恢复，原fault保持，双手与扩展字段零写、0移动命令，正式流程／清理仍false；独立字段／音频恢复、自有进程退出核对通过，97文件封存cd40f4a9…858fd。下一步单次观察内批查first-match地址，保留实时typed／删除检查及原门限，无默认安装；不回填AL／AM未知原因或构成头显／伤害验收，详见0.0.3后续进度。本补记不构成0.1.0发布。

AO源码补记：默认关闭TAIL的同次observe改为两组首同名地址批查，再保留全部原live typed／deleted等检查，raw观察后独立重查两组地址并比较，未增加跨调用缓存或放宽权限。Astra实际三文件源审通过；真实原／新resolver和page text新8类／53项检查、实际受影响reader／SEH目标16类／207项回归、完整PG严格构建／23导出及含controller的六实际运行源OFF对比通过，DLL a96aedd0…cdc2。尚未实际游戏验证，不承诺毫秒节省、100ms通过或完整伸手／伤害／头显验收，默认入口未安装。详细证据与host边界见0.0.3后续进度，本补记不构成0.1.0发布。

AO实测补记：唯一静音后台购买版局首枪体提交后有序age交还，双手7应用／7恢复、23对末端字段样本审计通过，14个初始未变化手势结果均在原门限内。第二次acquire在平移前明确因最终NS证书新鲜度check10拒绝（110.1797ms）停止；MS原截止尚未越界。三段prepare约7.5ms、最终手部校验约2.35–2.47ms，不是受控性能比较，未解决完整流程。组件1提交／2恢复、0移动命令，正式passed／cleanup仍false，独立原字段／8音频恢复与自有进程退出核对通过，59文件封存952075aa…2f08a。无默认安装或伤害／头显完成，详细来源及下一步有界优化见0.0.3后续进度。本补记不构成0.1.0发布。

AP源码补记：默认关闭TAIL内加入同次observe的有界删除查找比较材料，每次复用仍实读完整路径／GNames／原始名称字节；未知或变化锁存拒绝，正匹配和未准入的资源上限走原实时deleted路径，原36次identity等校验不变。Astra三源审查、新实际数组／名称路径8类／405项检查、受影响reader／SEH16类／207项回归、完整PG严格构建／23导出与六实际运行源OFF对比通过，DLLcfd25b0c…8bf5c。Lookup约131,344字节栈、额外保守拒绝和本地数据／copy服务边界已记录，尚无游戏收益或完整伸手证明，无默认安装／0.7伤害接线。详见0.0.3后续进度，本补记不构成0.1.0发布。

AP实测补记：唯一静音后台购买版局首次acquire最后group4仍以NS新鲜度check10拒绝停止，stereo源龄104.0706ms，MS票龄94ms尚未过截止。四段prepare约7.33–7.81ms、最终手部校验约2.48–2.51ms，没有受控收益或逐Lookup启用证明。组件0提交／1恢复，双手零写／0命令，正式流程及清理仍false；独立8控件／3组件／44挂接和8音频会话恢复、自有进程退出核对通过，73文件封存e0fae948…177ad。无默认安装、完整伸手／伤害／头显通过，详见0.0.3后续进度。本补记不构成0.1.0发布。

AP收敛补记：源审建议下一步只批查read_control的17／19项字段、保留原实时逐项校验与首尾重新解析；约200–208次名称遍历到16次的静态工作量减少尚无耗时证明。已撤下收益未确立且增加约131KB栈与保守拒绝的AP删除查找候选，三源恢复AO原字节，786源与既有通过构建逐一相同；不重做旧检查，AP失败封存保持。新控制器批查尚未接线，完整功能／0.7伤害仍未完成，无默认安装。详细证据见0.0.3后续进度，本补记不构成0.1.0发布。

AR 源码补记：默认关闭 TAIL 已把同次 read_control 的17／19项地址定位改为首尾批查，原实时逐项类型／值／注册／bool mask／struct 等校验及尾布局／撤权次序保持，不跨调用缓存。Astra 窄审、新实际 old/new 八控件8类／659项检查、完整 PG 严格构建／23导出和六实际源 OFF 对比通过，DLL589cc44b…14c5d。实际游戏收益仍未验证，无默认安装、真实0.7伤害或头显通过；来源和host边界见0.0.3后续进度。本补记不构成0.1.0发布。
