# 枪体组件变换的可恢复租约候选

此记录属于 `codex/weapon-6dof`。当前包含纯C++组件及默认关闭的同GHW1枪体适配器。R2两局未进入枪体写入；R3至R7的c至g局取得部分组件更新与交还，完整枪体跟随仍未通过。公开手部或枪体写权限仍未授予，普通启动器未启用。

## R8初始准备块名称读取候选

R8已在干净应用commit `319a3bc151af49913a4b2b2d164e59c7d0d2bef7`封存742源／164工件，source manifest SHA `9d70b0354fc7c43f1b1926d4bf5c29050a27d47a1317f10f98ada02bf45dd168`，Native／XR／loader沿用R7。h准备后先等待外部启动器；只读核实发现无子工作进程的GUI被旧inventory误判为活跃owner。新i入口只对精确路径＋固定SHA、完整PID／Parent库存且无后代的已知GUI例外，其他游戏、CLI owner、AVP、XR／Guard／quiet照旧拦截，启动前仍两次检查。13项受影响边界检查与独立源码窄审通过；原流程／15s／256／3s reserve／动作／恢复均按字节或AST与h相同，不构成整段原子互斥。

i实际入口在instance.prepare的validate_game即拒绝：原工作零售副本路径已不存在，未创建instance、游戏、Guard、Native或XR，手／枪0写。owner38020退出已核实，原receipt SHA `54cf1c09d04a105f21b36db6da304afc4ca6efae4432a9e1a0442cec56f022a2`，56文件封存SHA `6be6f705b869b2e7657afc88f48888d0779725c491d5c2be5a7e2910e843e534`；不将此前共享配置变更造成的准备拒绝算作游戏重试。后续新j入口固定现存Steam-managed零售EXE，不使用历史目录fallback，实际EXE与shipped config新增为依赖；核心EXE／配置只读校验通过，没有重做全3609文件兼容调查。j准备194依赖／26复制／24模块，Popen0，13项未改helper检查继承不重跑；尚待最终入口复核和一局真实R8测试，普通启动器及冻结Native／XR不变。

唯一源码变化在component_apply：已有component_names::Scope从capture前覆盖world、mount、initial_read、absolute_storage及目标构造，在Forward／Backend、initial_cost.finish和lease.acquire前析构。wanted仅外提纯声明，原位置赋值；全部早返恢复父TLS，faulted入口关闭，invoke全段及内部失败／外部回滚保持旧范围。R7三个计时跨度、原getter／first-match／动态检查顺序、同Plan和100ms均不变，不新增缓存。

实际完整component_apply／Scope／invoke提取58场景4,818检查首编首跑通过，Astra独立窄审通过；ON唯一严格W4／WX／EHsc／C++17构建首次成功，23导出与R7相同。OFF与default完整实际EP原bytes完全相等，未重编；未重跑旧counter、页／租约／几何套件。成功DLL SHA `a05516969ddd8400b1c0dcfa4aee997821d7312c69e96a3ff9f96668afded5aa`，真实build base `96622b61fa334a647dbca8feebf43d5c1466b514`，742项目源／16vendor前后相同。协调者核对60支持工件、MSVC实际设置／路径／PE导出、旧R7冻结输入与g封存57文件＋7外部证据后按字节应用一源，直接复用已编译DLL，无同输入重编。

报告SHA `f163b548e6700373eb2149e021ec911ba7422081a25a5a37ebfec236d9a9731b`，Astra `0d30f4f2049be8e3634c4913e6082c0a5a931b69adb69418e0aa2ca4fa9d85ab`，应用复核 `3e25cb3ade51855cb9ead50793a69ac3aac423dd4b3126ea368443c26308cc8d`，位于ignored `analysis/hand-component-initial-scope-20261023/`。root首次应用脚本误把嵌套g_evidence当平铺map，在任何live写入前拒绝；原脚本及失败记录保留，修正证据路径展开后通过，未改候选或重编。当前尚无R8实际游戏局，不能承诺解决g的103.9ms源证书拒绝，普通启动器与Native／XR保持。

## R7购买版g：源证书提交拒绝与三类实际成本

R7封存 `analysis/hand-mesh-phase1-r7-candidates/a7bbc8bc…f4ad4fc/`含742源／162工件，PG `ba40192d…ec8fe1b7`，source manifest SHA `11113b4c9de9bb107602e60aa7f92962bbd7e3c91ed7ef8edc5f7156b8973cf8`；真实build base2327f52、应用commit432dd53分开，未应用后重编。g入口仅OUT／TAG、pins与工件数161→162变化；受影响两入口检查通过，Popen0准备191依赖／26复制／24模块，186共用依赖与模块均和f同源同SHA。15s／256／3s reserve及Mute1／Behind1／ObserveOnly1、后台物理输入关闭保持。

购买版g/PID8620 Start0，仍在Rotation前结束。首次失败ordinal165／group4、known127／checks2047／reject1024，实际为authorize_xr_plan的check10：commit时certificate_recent为false。本次now／live／commit338683531，until338683537，毫秒期限仍余6ms；ticket_ms与plan_ms均338683437，仅支持94ms的本域快照／票据年龄，最新Context年龄0。authorize_ns338683698854600、commit_ns338683698855000，stereo_ns338683594941100、input_ns338683595029700；同NS域源接收年龄分别103.9139／103.8253ms，均超过原100ms。Policy实际求值且mask0，但未记录其所有内部路径，不唯一归因；后Stop11／Shutdown12因短路未执行，不假造结果。R6→R7不同首失败位置不是本诊断候选带来稳定加速的证明。

成本header为QPF10MHz、10／64行、pending1／frozen1／full0；十行均闭合、时钟已知，同lease1／sequence793／revision20。initial8.8377ms，三组snapshot为2.4178／2.3178／2.3091ms，六个name_scan共1.0138ms（包含于snapshot、约占其14.39%），本局不足以支持先优化骨名扫描。initial.begin→末snapshot.finish的纯QPC窗口60.9642ms，initial＋三snapshot非重叠实测15.8824ms，段间45.0818ms尚未唯一归因。沿本版pose_bridge.h:13直接QPC/QPF公式并采用同系统10MHz频率，initial.begin→commit69.7245ms，其中53.8421ms未由这四段覆盖，stereo接收时间在initial.begin时已老34.1894ms；不与GetTickCount64混算，不直接把未覆盖间隙命名为某一函数耗时。逐字段成本复核在 `coordinator-cost-analysis.json`，SHA `4f1c09e28e5e94a3f65c31b003ffc155b5bd0d4604697b8ed06505cd0e078325`。

组件与f同为部分Absolute＋Translation更新、Rotation前向未调用，acquired0／restored1／setter_calls4尝试／fault1；最终dirty／owned／revoked0、safe1，手部0写。独立30项核验确认本次失败、raw三组／三份九Key、8控件／44附件／32和64字节入口原值、冻结742源／162工件／191依赖与实际复制，Stop／Shutdown0及CIM无自有残留；新增成本记录检查不构成性能或整局验收。FPP47、GHW1 matched66／published52／rejected14只确认本局发布。9专属会话已静音、守护读／静音错误及pending0，精确游戏退出后Guard0。component／whole=false、Native入口verifier false、XR1／图像0保持。

原receipt SHA `781f7696438f7f1fbbedc9992b4c3e8dfbe79db7f70bc4d4703e9d254751a19a`，PG log `489285ab56f44f45b69c594f7c0c40c764ed85384b43591dd4dea01e3d9c226e`，独立摘要 `7a7eb69612581dc1b14887912be30a89604bfa7d683f1bf13dacc53c4198aff3`；57文件＋7外部证据封存SHA `bb16f521ec1c287231899064583e4d7431997d201f59d77b495c797718cb26c3`，均位于ignored `analysis/hand-mesh-runtime-1023g/`。旧f的56文件／7外部工件重新核对不变。下一步只把现有component_names scope扩至初始准备块，在lease.acquire前明确析构；所有提前返回恢复TLS，invoke全段关闭、内部失败和外部回滚保持旧范围，不缓存或减认证。它只能影响initial中的解析部分，不能承诺节省整个8.8377ms或足够过关，目前尚无R8源码。

## R7三类分项计时候选

R7仅新增诊断，未作第三次解析优化。固定64行记录component_apply在phase／右手早返后的初始块，正常路径止于lease.acquire之前；原Forward.prepare短路位置中的snapshot整体；该snapshot内左右手name_value扫描。扫描只带已有names.count与命中index／结果，不逐entry计时或新增getter。记录lease／sequence／revision／group及clock known，snapshot与扫描为inclusive嵌套项，不能相加当总耗时。

invoke全段暂停子owner，fault／null不采前向样本；首次合格失败先置pending，已开span闭合后才frozen，避免early return或失败snapshot丢失退栈计时。QPC／QPF失败、零／负值与倒退仅记unknown，容量满只停诊断，不改变原返回、资格、时钟权限或恢复。终端要求双journal clean且open0，沿原drain／preTick重试，在旧terminal flag早返前只输出一次；热路径不写文本或flush。Name扫描原copy／memcmp条件、原engine／Stop调用及100ms短路保持。

实际wrapper提取59断言13场景、独立actual helper合成时钟163断言18场景均首编首跑成功，Astra源码窄审无阻断。PG ON唯一严格W4／WX／EHsc／C++17构建成功，23导出与R6相同；DLL SHA `ba40192d041168f15a54fc5afc4e9ddacb738bb85cdc59386cee10afec8fe1b7`。OFF／default两完整EP仅空白行不同，其余行bytes相等，未重编这两配置，未重跑R6页／租约／动态套。真实build base `2327f5259df8a61f82effaaadf6a82b46bed2e47`，742项目源／16vendor构建前后不变。

协调者核对60支持工件、742源／16vendor、实际MSVC工程／路径／PE导出及旧f的60份证据后应用4源；当前742与成功编译输入按字节相同，直接复用staged DLL，无应用后重编。报告SHA `b711a32ad11ff1cb02c9aa333ab95500cfabff057c9e0b3b1bc934221c48f46e`，Astra review `2f0f10f5bbd10d15b7bf6eee893557dea36926418d979ff30cce9d8b12b9ffa4`，应用复核 `1762a05302f77788aa9cd495a89ecb73388ff90fb3ac75fd3331915ff8dc0fe2`，均在ignored `analysis/hand-component-timing-20261023/`。尚无本轮实际计时、性能或枪体跟随通过结论；旧f失败、Native／XR、公开写权限与普通启动器保持。

## R6购买版f：仍超期，下一步先分项计时

R6封存 `analysis/hand-mesh-phase1-r6-candidates/c7faadd2…350aa2b/`含742源／161实际工件，PG `8079a0d3…5cefdbb8`、manifest SHA `08c4bf810c1ca57b8421396da579b52843616aad93e88ffd98739c88965986c6`；build base41d7c96与应用commit `4d00626`分别保留，没有应用后重编。f入口仅OUT／TAG与候选pins相对e改变，工件数仍161；两项受影响入口检查通过。Popen0准备191依赖／26复制／24模块，186共用依赖及24模块与e同源同SHA，15s／256／3s reserve、Mute1／Behind1／ObserveOnly1与物理后台输入关闭不变。

购买版f/PID57956 Start0，终止于component_acquire_failed，手部0写。首失败ordinal165／group4最早deadline门，known67／checks3／reject2：live_now334506796比write_until334506787晚9ms；ticket_ms334506687、plan_ms334506703、context_ms334506781。后续NS／Policy／commit／Stop条件未执行，不把unknown0解释为实际检查结果。**plan_ms实际来自plan.context.observed_ms，是所携带游戏快照的观测时刻，并非Plan构造入口。** 本局ticket年龄109ms、该快照年龄93ms、两观测间隔16ms、最新Context年龄15ms；此前按plan_ms计算的“Plan age”也只能按快照年龄理解。e／f的109／93不能作为稳定加速16ms或组件耗时的证明，未混用ms与sourceNS推算源年龄。

pre_restore支持Absolute000→110、Translation到目标、Rotation保持原值，dirty／owned mask3；终态acquired0／restored1／setter_calls4／sticky fault1，最终dirty／owned／revoked0、safe1。四次为调用尝试，不是四次成功；本次未完成acquire或跨Tick枪体运动。独立只读25项确认raw三组／三份九Key、8控件／44附件、32-byte CallFunction与64-byte ProcessEvent原值保持，Stop／Shutdown0及CIM无自有残留。FPP44、GHW1 matched57／missing0／published45／rejected12，仅确认本局发布，未按rejected计数归因。原component／whole=false、Native入口verifier false、XR exit1／图像提交0保留。

9个专属会话已静音，守护muted／read_failed／mute_failed／restore_pending均0，精确游戏退出后Guard exit0。独立摘要SHA `64fb853309c2f955b3afc2f49f67e0109ce5fc1294c1fd216819d1ee00b7374a`明确不是整局验收；原receipt `e5f483ebf424fd8fd25c6bd2af5b6937e8fce4bf5ddb45947c059acde1e90185`，PG log `8169a33d46138d2050edacbec9286c66d1cee4e89a02df43383ed10a4244d534`。56文件＋7外部证据封存SHA `dd6938cc63138203cfc19ce835e33e6084d9a0a384331d8f2e9b6052d2b8f139`，位于ignored `analysis/hand-mesh-runtime-1023f/`；旧e的55文件／7外部工件重新核对不变。

现有f的write_evidence为0.1031ms，顶层hand_snapshot_identity均值5.0533ms，但不是各组Forward快照分项。源码仍在每次read_snapshot中进行左右手name_value全局名称扫描，组件capture／world／initial_read／absolute_storage也处于Backend TLS之外；现有聚合不能判定哪项主导剩余开销。下一步只加初始块、Forward快照与其中骨名扫描三类有界QPC诊断，按group隔离前向与恢复，首失败冻结、终端输出；快照与扫描为嵌套项不能相加，不新增getter或逐entry计时，不放宽100ms或缓存元数据。

## R6前向调用范围内的名称读取候选

R6只改全局name的私有文本复制分支与三处scope接线，仍由既有phase1宏保护。Backend read／set入口的RAII明确赋值为forward且未faulted，false也覆盖外层true，离开时恢复；invoke入口显式false，覆盖admission、原函数、递归回调及after observer。这样Args／schema、Forward的普通反射及四槽Args／output可使用页内复制，实际原生调用与回调交还仍旧路径。R5组件私有lookup不改；不缓存字段地址、元数据、动态值或检查结果，不减少原槽查询／setter／认证，不改100ms、Plan或Stop。

全局name的Name／Array／entry／index前缀及检查顺序原样，count0在分派前拒绝；泛化页内复制至实际count，末位NUL可成功，无NUL时终止out[count−1]并返回false，奇地址跨页保持原2-byte读取，失败不重试。Astra对最终两源复核无阻断；移除3个新增私有宏区与2条Backend scope后，分别精确还原R5原文件。实际Scope与完整invoke提取的102断言14场景首编首跑通过，覆盖false覆盖、fault／null、回调关闭、提前返回与恢复；独立Win32页测试33场景396调用首编首跑通过，count0／1／64／96／128、保护页、请求长度后sentinel及原名称规则均核对。该结果不证明Names并发原子快照或实际性能。

PG ON严格W4／WX／EHsc／C++17唯一构建成功，23导出与R5相同，DLL SHA `8079a0d347d41ef7a0bc3d0f4a037b1708af3a7c71890dfd756ccac75cefdbb8`。OFF与default的实际完整EP原bytes不相同；仅删除纯空白行后，其余每行bytes完全相等。初次raw比较断言失败和原输出保留，不计作PG编译失败，不为此改稳定源或重编OFF／default。真实build base为 `41d7c96025c4714ff6a2e5edf4584d818d0dffb5`，742源／16vendor构建前后相同。

协调者独立核对59份支持工件、742源／16vendor、实际MSVC工程／输入路径／23导出及旧e的58份证据后，按字节应用两源；当前742与成功staged编译输入完全相同，直接复用该DLL，不做同输入重编。报告SHA `3954136c6bc6fe9228eaf2ad3845adf94de399f130a10532bb7eb18d151666bc`，Astra复核 `598b3ebbd867b119577705b10cd732cb28f13ad55749ed17fb6bf3a833f2a776`，应用复核 `925b4976e86708bf7c014b4baf3ff42dd44f665401f885c4d20e73a04c6d27d5`，均在ignored `analysis/hand-component-scoped-name-20261023/`。尚无R6实际游戏局，不宣称追回e的9ms；旧d／e原失败、Native／XR冻结候选、公开写权限和普通启动器保持。

## R5购买版e：仍在旋转提交前超期

R5封存 `analysis/hand-mesh-phase1-r5-candidates/b7edd9f7…5b46606/`含742源／161实际工件，PG `dde8242e…edc45133`、source.json SHA `67a931aca1f332b27f15270a5d8662d60ba3326d545195246b94269ed3bf73ab`。真实build base696892d、应用commit `09e7acb515dc3831916c3f884329489e594a3c46`与成功staged来源分别保留，未应用后重编。e只改OUT／TAG、pins与真实工件数；Popen0准备191依赖／26复制／24模块，186共用依赖及24模块与d全同，固定15s／256／3s reserve及三后台标志不改。

购买版后台静音e/PID27852 Start0，以component_acquire_failed结束，手0写。固定首失败仍ordinal165／group4、auth known67／checks3／reject2：live_now332160546大于write_until332160537，超9ms；ticket_ms与plan_ms均332160437，context_ms332160531。NS／Policy／commit及后续Stop／Shutdown条件在该次被最早时限门短路，不能归因于未执行分支。d的ticket age125ms包含16ms ticket→plan间隔，其plan age109ms；e两者同为109ms，故25→9ms并不是可宣称加速16ms的对照性能证据。尚未证明本优化足以完成事务。

组件终态与d同为acquired0／restored1／setter_calls4／fault1，最终dirty／owned／revoked0、safe1；partial pre_restore支持Absolute＋Translation改变、Rotation未调用且保持原值。独立raw基线／Start前／Shutdown后三份快照核对三组及九Key相同，8控件／44附件／两PG入口保持，Stop／Shutdown0。独立25项只读收尾通过；计数4不当作四个成功setter，未完成acquire或跨Tick运动。FPP49，GHW1 matched58／published46／rejected12，未单凭rejected计数归因；component／whole=false、Native entry verifier false、XR exit1／无提交图像继续保留。

9个专属会话已静音，守护read／mute错误、restore_pending均0，精确游戏退出后Guard exit0。独立CIM确认owner55648／game27852／XR59712及Guard／launcher不在；旧d的55文件与7外部清单再次核对不变。e原receipt SHA `db7a409d65851e93222254549008ed7ec6d27388fcb7ac5ab9470c9246b16545`，PG log `fcc7ee57bacbd761179f7dae17d3acf2dffd34f12430eed50de655b39feebbda`，独立摘要 `9c3203e3fed4c20c5dc9d3d28ab44eac0b2b3a5029f09189f48b14c3b1f4cc60`；55文件封存SHA `9e1e6dbf97050ea33b9f02270d9868a04bc134c8578b3a1af42dd2ef0dfd20a2`另固定7外部证据，均位于ignored `analysis/hand-mesh-runtime-1023e/`。

下一步只分析仍用原逐字名称读取的Args／schema、Forward snapshot／equipment gate及四槽路径；候选方向是限定前向调用栈的私有bulk scope，回滚关闭，不缓存元数据或减动态检查。当前尚无该R6源码／构建，100ms／同Plan／Stop边界继续保留。

## R5私有页内名称解析候选

R5采用比元数据描述符更窄的改动，只在组件adapter内增加私有lookup。保留原Name→Array→entry→entry.index检查、忽略Name.number、128 wchar上限及大小写比较；按当前系统页内的完整wchar分块copy，在本地遇NUL就停止，无NUL才读下一页，奇地址跨页仅原2-byte copy，任何读取失败直接false不重试。只冷读一次OS页大小，不缓存游戏字段地址、元数据、动态值或检查成功。内存与页保护稳定时保持旧返回和所触及保护页集合，不新增并发Names原子快照保证。

原40／4096首匹配、缺失bDeleteMe返回false、typed字段及Key前后读取顺序不变。Backend仅forward且lease未faulted时选快路径，pure fail内部恢复虽有forward指针但sticky fault已置位，因而与外部null回滚、诊断读一起保留旧resolver；apply初始read／absolute_storage显式fast。capture／world、四槽原getter、stow read_state、Args／Forward／schema／triple子字段、SelfScope与100ms最终门均未改。

实际源码提取的动态检查111断言／35场景通过，非名称字符读取及槽查询序列相同。独立Win32 VirtualAlloc页测试33场景／132次gold-fast×name-named调用通过，exe仅执行一次：页末NUL后的NOACCESS／GUARD零触碰，首文本GUARD／奇地址跨GUARD一次fault且false，不发生fallback；127索引NUL、128无NUL、大小写与坏元数据等通过。页fixture计数wrapper的一次ADL编译失败单独保留，未修改提取函数体。短名7→1、跨页25→2仅是copy调用计数，不能作实际耗时或游戏性能结论。Astra对稳定源 `c2549e97e3857cbc10fee44f94a40918af7857e917cf921b615c28e605a38bac`窄复核无阻断。

PG ON首编成功，严格Level4／WX／EHsc／C++17，23导出与R4相同，DLL SHA `dde8242e4cfe0fd74716c9efcde8731e0147be8ef27576af6ac42af7edc45133`；default与phase1 OFF的实际预处理均原字节相同，未重复这两构建。报告 `analysis/hand-component-fast-lookup-20261023/report.json` SHA `86651a1fad9fa3685479129fee640c1f296c87d9c4ad2f57169e758d15edd39b`；协调者核对742源／16vendor、59支持工件、真实项目／PE及旧d58文件／外部清单后应用一源，live742与实际staged编译输入完全相同，应用复核SHA `27db3cd97c89039dcfc35fa3b547f14d4ad2b080eaa50424fc52370acd22e3bd`。成功DLL可直接复用，无应用后重编；真实构建base `696892db8631bd608e8065490beec3197a79c46d`与后续应用commit分开记录。

R5尚无实际游戏局，d的25ms首失败仍保留；不能仅凭解析检查通过宣布枪体跟随修复。下一局继续固定预算与静音／置后／只读输出，不刷新事务内票据或放宽任何时限。

## R4购买版d：旋转提交前时限拒绝

购买版后台静音d/PID5612使用上述R4 PG `a8276f74…67c1af8`、manifest `e87ed90cc8171e48352d9d1f6f7726a6765c24e9ca40ca334ff8542c13f69919`，配套封存为 `analysis/hand-mesh-phase1-r4-candidates/53e64808…e4c342/`，742源／178实际工件。构建base17709c3、应用commit `5662a7e6fe95297dbbfd1ed6435c6a31fc4609c7`分别保留，未应用后重编。Popen0准备固定191依赖／26复制／24模块，与c的186共用依赖和24实际模块均无变化；只绑定OUT／TAG、候选pins与真实工件数，原15s／256／3s reserve、几何审计、Mute1／Behind1／ObserveOnly1及后台物理输入关闭不改。

Start0后手部0写入，以component_acquire_failed结束。首失败ordinal165为group4、forward、wanted非original的authorize：auth known67（args／cutoff／live）、checks3、rejected2，now＝live_now328843921，write_until328843896，ticket_ms328843796。最早live_now>write_until条件超25ms即短路，未执行该次authorizeNS／Policy／commit clocks及后续Stop／Shutdown条件；保留这些字段unknown，不能把默认0当执行结果或将ms和sourceNS跨时钟域相减。该记录证明d的旋转setter在调用前被有限时限拒绝，不能据此为旧c补写未观测首因。

诊断观察339行，63条尾部已覆盖276行，但固定首失败仍完整。组件终态acquired0／restored1／setter_calls4／fault1，dirty／owned／revoked0、safe1。调用计数包含forward和回滚尝试，不等于四个成功setter。唯一pre_restore中dirty＝owned3、Absolute000→110、Translation与目标字节相同、Rotation仍原值；外层回滚后独立raw三组及九Key均与原基线一致，仅支持本次部分更新与交还，没有完成三组acquire或跨Tick运动。

Stop／Shutdown0，原8控件／44附件和两PG入口保持；FPP累计55、Native GHW1 matched58／published47／rejected11，未单凭rejected计数归因。9个专属音频会话已静音，守护read／mute错误、restore_pending均0，精确游戏退出后Guard exit0；独立CIM未见owner12524／game5612／XR19344及Guard／launcher残留。协调者25项仅只读收尾通过，scope=`rotation_precall_deadline_rejection_partial_component_and_rollback`；原component／whole=false、Native entry verifier false、XR exit1／无提交图像仍保留。

ignored `analysis/hand-mesh-runtime-1023d/`的原receipt SHA `da10f0c4229a9dcdafdb3e5842dd81901ff5a03fd0a2c36592ba143d7d643ea9`，PG log `07acaf6d73bb9e4d0de49f168921d6e57e28da45fd3b966af9e298f5ac9cecad`，独立摘要 `100824dbf275b9510fca9adc7faa5fb2da42ceb6cdf2ec69b1bf2a60f2a93234`，55文件封存SHA `0fa1a65d80d7fc54601e000e6149838752fd7129a620b316a32cf8b017cb7998`并固定7个外部证据。c的54文件及外部清单再次核对不变。下一步只降低组件重复反射／名称解析成本：缓存须有完整解析路径与元数据身份守卫，动态Key、链接、槽值、schema、expected及最终同Plan授权仍逐次重读；实际节省时间要另测，不能用静态调用次数宣称性能通过。

## R4首失败诊断候选

R4只改三份私有适配源码，保持原100ms、同Plan最终认证、逐setter schema／完整Key／expected检查、原返回值、短路顺序、真实hook转发及unsafe卸载规则。它不修复c的运行根因，仅增加固定首失败和63条有序尾部记录；字段来自原有game读取、原有clock／atomic与授权调用，不额外重采样或续龄。输出为独立gate前缀，原written／pre_restore／end schema不变。

Astra复核修正两项诊断缺口。纯lease组前或组后值不符可能发生在read返回true之后；现仅对ok／known／完整Key相等、group0／forward／已有snapshot／无旧fault的既有读值比较owned，先固定失败，排除setter内部正常旧值及内部／外部回滚。dump同时等待组件depth与dual_draining归零；第三源只在双journal均已尝试、发布状态后增加终端重试，覆盖非Tick动作及clean-ended后立即Shutdown，不依赖下一个Tick。DumpAtExit在旧terminal重复提前返回前构造，不需要改旧terminal标志。

实际函数提取使用合成backend：Auth274项／16case、Backend69项／12case、Drain10项通过。原R3与R4的时钟、Policy、Stop／Shutdown及backend调用序列相同；覆盖Rotation返回成功但无效果、超过63行的两组清理、次生回滚认证失败、组前foreign、false有／无写、skip组与正常original清账。这些结果不是实际游戏调用证明。

ON实际发生两次编译尝试。首次在严格W4／WX下因新增SetScope参数original遮蔽现有全局变量而C4459失败，无DLL；首次源码、命令、输入与失败日志保留在on-attempt-1。仅将该参数及其绑定使用改名wanted_original，机械替回可重建Astra已审原文件完整SHA；第二次成功。成功DLL SHA `a8276f74e247454a98a695d57d0a5817524c8f164b8d7e9657a1fa36167c1af8`，实际项目为Level4／WX／EHsc／C++17，23导出与R3相同。default的MSVC预处理输出原字节相同；combined ON／phase1 OFF仅空行及同一个write_until失败return花括号差异，窄归一化后相同，没有重复这两种构建。末次改名在被移除的私有块中，不重复预处理。

报告 `analysis/hand-mesh-first-failure-20261023/report.json` SHA `62c328320c5c8bc7e67b600e0eed750a6cfe84bee6ec377c12f1138fef29475e`；协调者核对742项目源、16vendor、76份支持工件、真实PE／编译项目、旧R3封存及57个c证据文件后按字节应用三源，应用复核SHA `09c77441d00113e9758c87e355477bd0c04ae9b988962e6d4e7478c89eeb206e`。live742与成功staged编译输入全部相同，可复用该DLL；没有应用后重编，构建base为 `17709c39fc62991351eb1261965c6158b4a4091d`，三处staged差异和后续应用commit必须分开记录。common pure lease未改。

R4尚无实际游戏结果。下一局继续购买版、静音／置后／只读输出，独立保留实际失败及清理回执；不能以本诊断CPU／编译通过替代枪体跨Tick运动、最终图像或头显验收。

## R3分类前置候选

R3只修改 `vr_hand_ik_stream_trial.inl` 和 `vr_hand_weapon_mesh_phase1.inl`：共用原46个动作名，只有注册有效、名称读取成功且明确不在该集合中的调用才跳过组件关联查询。GT检查和owned invoke的一次性self token仍在最前；真实PE／CallFunction不消费token，名称未知保留原处理，InvManager切槽／完整八对象关联、失败双drain和转发前撤权均不减少。未缓存跨回调的关联结果，也未改Policy／WorldBridge／Session／100ms或任何同Plan最后写入门。

Astra逐集合及两文件差异复核无阻断。实际提取入口的合成backend回归548项通过；机械换回R2两段admission后，“被动getter不产生fallback属性查询”的断言失败，rc1。staged ON Release只构建一次，实际项目为Level4／WX／EHsc／C++17，23导出与R2相同，DLL SHA `64956c118f5b36131f664016f8df8f81e2c68b4b48c4c69731580a3114c47689`。default的实际MSVC预处理输出原字节相同；combined ON／phase1 OFF只有空行及同一个失败return的花括号差异，明确窄归一化后相同，没有重复OFF/default构建。

协调者核对报告、实际742项目源及16 vendor、25份支持工件、原live742和旧证据后按字节应用两源，应用后live742全部匹配真实staged编译输入；因此可复用该DLL，未再编译相同输入。报告 `analysis/hand-mesh-passive-classification-20261023/report.json` SHA `bfcecb70a96ca565a0b79db601eaa2afdab679e39007546609ff05aa35b9e94c`，应用复核 `coordinator-apply-review.json` SHA `1d94db80ffa0e06fd92b878fb1d39089a913237e32bb00c018122d5d7a002515`。构建时base为 `a8532ce12724a972730b0f56e83aca095b607fd6`，当时两份staged源与base不同；后续提交／封存必须保留这一来源事实。

R3封存目录为 `analysis/hand-mesh-phase1-r3-candidates/7e027f31…12bccdf/`，source.json SHA `cf760058406f7ae935b6146b28c521a957c5886a7bc876828bb3c3d06d488550`；742源／146工件，Native／XR／loader原字节继承R2。manifest分开记录旧构建base、两份staged差异和后续应用commit，未把它改写成新commit上的live build。

购买版置后静音c局PID8120以Popen0准备 `c815cc19…85a0570` 启动。Start0，首次authorize和postflush已通过，产生一张planned pose，之后 `component_acquire_failed`；没有手部written样本或完成lease。组件终态为acquired0／restored1／setter_calls2／faulted1，最终dirty／owned／revoked均0、safe1。setter_calls在实际调用前递增，还混合forward、内部fail回滚和外层null rollback尝试，不能解释成“两个setter成功”。

唯一component pre_restore记录dirty=owned=1，absolute逻辑标志由000变为110，Translation／Rotation的12字节仍与原值相同；外层交还后，独立raw快照确认三组原值和九个对象Key均与基线一致。这支持本次部分absolute更新后的交还，不证明三个setter完整执行或跨Tick跟随。日志中没有本局related action／yield打断，当前仍缺失败gate与时刻，不能在超时、expected差异或call结果之间唯一归因。

Stop／Shutdown均0，原8控件／44附件／两PG入口保持；FPP累计61，Native GHW1发布50。9个会话的静音守护read／mute错误与pending均0，精确游戏退出后Guard exit0，独立CIM未见owner／game／XR／Guard残留。协调者21项只读收尾通过，scope=`partial_component_acquire_failure_and_rollback`；原component／whole=false及Native／XR画面失败均保留。

原c receipt SHA `be31b13bd4c9ca8c217ed0a74fe01317f5be45478a42643de6dc2e729adcd367`、log `9eb88756cfb6b93271d86a3f52a1399de53b6c7c91bb7628e151fab50e9faaf0`、独立摘要 `2944a33ddf3f3f7423d2bd5b88e1fc43f04951852710ab1ef5d9dbb343b19edf`。证据位于ignored `analysis/hand-mesh-runtime-1023c/`，54文件原字节清单另存；a／b全部原证据不改。下一步仅记录首个组件失败gate、既有时钟和已知比较结果，不增加写权限、重读、续龄或扩大100ms。

## R2购买版测试结果

两次均使用原封存 DLL `25c9b9ed…7086ce2` 和742份源码，强制 Mute1／Behind1／ObserveOnly1，物理手柄后台输入关闭。每局有独立目录和一次性入口，没有覆盖失败回执或自动重复原版拾取／装备调用。

| 运行 | 已取得的证据 | 中止位置 |
|---|---|---|
| a，PID17996 | 原版 GiveTo／SetCurrentWeapon 各完成一次，原枪 Lancer 已装备 | 独立读取器额外查询不存在的 `Weapon.WeaponMesh`；第九个 asset Key 及 setter schema 尚未读取，未执行 Start 或启动 Native／XR 会话 |
| b，PID27712 | 真正 `Weapon.Mesh`、八条 links、九个 Key、资源 `COGAssaultRifle`，以及原组件三组和三个 setter schema 的完整独立基线 | Start返回0；38次 prewrite 后未取得最后写入权限，最终 WorldBridge read rejected，以 `xr_certified_pose_rejected` 结束；手与枪均0 leases／0 setter calls |

b的 `SetAbsolute` 参数为三个4字节 Bool、偏移0／4／8、flags `0x90`；`SetTranslation`／`SetRotation` 各为一个12字节 Struct、flags `0x80`，三个参数区均12字节、无返回值。独立脚本重新解码保存的函数头／参数元数据和身份数值，并核对基线、Start前、Shutdown后三份快照；这不证明 setter 被实际调用。b的原组件三组值保持，不能用零写入清理代替写后恢复测试。

a 的读取器修补只在新b副本删除多余字段，真实游戏适配器一直使用正确的 `Weapon.Mesh`，无须为此重编DLL。此前另修正了读取器的bound receiver验证与runner/audit范围匹配；原版本和回执均保留。b的 helper／runner／audit CPU检查为38／35／56项，尚不能代替枪体运行结果。

两局 Shutdown均0，原8控件／44字节附件／两PG入口保持；b另核对完整组件身份和三组原值。静音守护各匹配9个专属会话，未发生静音写入或恢复写入，read_failed／mute_failed／restore_pending均0，精确游戏退出后Guard exit0；独立CIM未见自有owner／game／XR／Guard残留。a／b协调者独立收尾分别20／19项通过，scope分别为 `pretrial_baseline_failure_and_cleanup` 与 `actual_lancer_schema_zero_write_rejection_and_cleanup`；原component／whole均false，b的Native entry verifier false和XR exit1保留。

原始a／b receipt SHA分别 `26cc48523649edd093ecb0420221493e05e40e60f62764a333872ec6f341ceec`／`9e894c074468aa3cb014908a3f2be78b44a83a918ec5a98a5b96b4e071ef6775`，独立摘要分别 `cd6e54c2074e9a3ed0256c7e5fbc02f1299e17d448a21e7ab0901e04eea3bfbf`／`22dfb6e4429e6f08d55e36958a9bf75b0bc999a1795371d8d3e83c5317c60e39`。全部原始证据留在ignored `analysis/hand-mesh-runtime-1023a/` 和 `…1023b/`；两目录另有48／54文件原字节清单。

下一步只缩减新增hook在确认函数名之前对被动getter反复做的组件关联读取。Policy／WorldBridge／Session与C1的源码哈希相同；不能把37次binding_age诊断误认成撤销绑定，38次prewrite也不是“没有Plan”。b没有记录最终read rejected的分支或包时刻，因此不能确定其唯一原因。两局武器／非空槽数量不同，分段耗时只支持热点假设，不能作同条件性能对比。100ms、动作名集合、InvManager边界、同Plan最后授权及双journal交还均须保留，再用新候选实测。

## 同GHW1 phase1 Lancer适配器候选

新增 `GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL`，要求certified＋combined，排斥旧mesh-follow／magazine writer。当前只处理固定Lancer实例、phase1右手单握；phase0／phase2不写枪体，左手副握和射击仍在后续范围。它消费与手部同一张Plan：完整八对象与资源身份、四槽及expected字段重验后取得组件租约，再重新读取手部状态和组件owned值，最后认证同一Plan才写IK；三个setter也各自重验实际实例Args schema和最终授权。恢复不依赖新鲜姿态或phase1，不把trial／stow bookkeeping epoch当对象身份。

恢复顺序为手IK、组件，再允许装备附件助手；两个journal均尝试，首个失败不会跳过第二个。恢复入口位于active／ended门之前，Stop不再把status8视为已清，每次drain重新发布safe_to_unload。纯lease增加 `relinquish()`：真实原动作必须转发而未能交还时，永久撤销pending组写权并保留dirty，后来只能在同Key／精确原值下只读清账；自有相关Args请求则可在未清时拒绝。自身setter使用一次性的精确object／UFunction token，真实递归hook保持原ABI和边界观察。

Astra复核发现初版只关联Weapon.Mesh，遗漏phase1／stow clean时由InvManager发起的SetWeaponFromSlot。R2已关联冻结完整八对象，并在建Key前由已认证Pawn取得InvManager；实际提取的关联入口回归覆盖该动作先交还、失败时自有请求拒绝／真实hook先撤权再转发。Astra最小复核确认缺口闭合。旧R1源码／报告／DLL保持原字节，机械替回旧关联实现的反例被新回归检出。

staged ON／OFF／默认三构建及278项pure检查通过，实际提取生命周期41项检查通过；这些检查使用合成backend，不是游戏结果。R2报告 `analysis/hand-mesh-component-adapter-20261023/report-r2.json` SHA `fb0f1d2df07445d6c0dea30552302a312bb98655dcf43f5116048e0805df5e48`。协调者另核对7份源码、8份回执记录、R1工件及16份独立vendor来源后按字节接入独立分支。live新目录只构建一次ON候选，23个导出与C1一致，四种错误组合均在配置阶段拒绝；live报告 `analysis/hand-mesh-phase1-live-20261023/report.json` SHA `e32fb274e778be42eb26cbccf816e44a158094a7483fb2d5404bdf7efbf2d917`，DLL `25c9b9edf8aeadf5eac959c8eaadc08bbef6a731306e6bab3121e1f567086ce2`。742份源before／after不变，没有重跑已通过的CPU／OFF／默认构建或Native／XR，也未替换另一会话的默认文件。

后续每局仍须Start前通过原版pickup准备Lancer，重做完整身份、8控件、附件44字节、组件三组和真实SkeletalMeshComponent的setter schema基线，不能复用b的实例地址。日志接口提供同lease／证书的written与pre_restore枪矩阵及全局journal计数；当前尚无实际枪体写入／跨Tick位置、完整旋转或写后恢复结果。同步setter读回不是最终可见跟随证明，C1的双手通过也不能作为本枪体候选验收。

旧 `vr_weapon_mesh_follow.inl` 会修改 `Weapon.Mesh` 的absolute标志、Translation及Rotation；stow journal则管理人物Mesh.Attachments中的44字节附件姿态。两个位置不是同一份状态。h的双手与附件恢复通过，不能证明旧mesh-follow的组件变换也恢复；旧adapter的地址存活检查也不能证明完整对象代次。因此暂不解除combined与旧mesh-follow的互斥。

## 接口与边界

`tools/vr-common/weapon_mesh_component_lease.h` 提供 `WeaponMeshComponentLease<Key>`，操作序列为 `acquire(expected_key, wanted, backend)`、`restore(backend)`，以及仅clean、非重入时显式 `reset()`。它精确保存三组字段：一次SetAbsolute所使用的三个flags整体、Translation原始12字节、Rotation原始12字节。Absolute不是三个可分别重写的字段，以免恢复某个flag时覆盖同次调用中的foreign flag。

backend必须用完整对象代次及调用上下文认证Key；每个setter在实际调用前再次核对Key和完整expected Value，只能改变自己的组，必须以bool报告错误。false仍可能已经写入，journal在调用前记pending，调用后重新认证读回。它不自行验证游戏线程、动作、GHW1或源时间；未来adapter须在这些字段读写之外提供当前姿态与动作授权。backend若会抛异常、改变未声明的其它字段，或无法进行写前重验，不能直接接入。

组内撕裂或foreign值会撤销该组写权、保留dirty；其它仍owned组可以独立恢复，不覆盖foreign组。Key未知、地址复用、known=false或读失败会撤销全部剩余写权；旧owned值后来重现也不会重新授予写权。撤权组只有在同完整Key下只读观察到精确original时才能清账。`restore()==true` 只表示journaled组已清，不证明never-owned的foreign字段，也不清除sticky fault。父session遇任意fault必须结束，不能用reset静默重开。不可观察的foreign写入后又改回的ABA仍不在本组件证明范围内。

## CPU验证与审阅

独立VS18 x64 Release以 `/W4 /WX /EHsc /UNDEBUG` 构建，修正版268项断言检查和CTest1/1通过，涵盖false前后产生效果、无效成功返回、组内撕裂、foreign组独立恢复、身份丢失、地址复用、写前漂移、重入及精确字节恢复。Astra发现初版在复制wanted前清理自身journal，导致把 `owned_value()` 或 `original_value()` 传回acquire时目标变零；现已先复制desired再清理。新增两getter自别名回归使用不同的非零新backend基线，核对目标、journal及最终原值；在ignored副本恢复旧顺序后，两条回归分别rc1捕获旧bug。Astra再次只读复核问题闭合。

修正版header SHA256 `7116b49ac7442a3288f06083313ba3bbf78762eda6fa92e3ce10294748588bdd`，test SHA256 `7ad9ef71675b16c68453be24d1f5157d8317fd74905f31bd32d11643bb6f3a94`。私有回执 `analysis/weapon-mesh-component-lease-20261023/getter-alias-fix/verification.json`，SHA256 `c3454567b689f8ad65d0e6b43a2b396b78befdf82e50ee11116fff0b5295154d`。初版256检查的72个历史文件及原回执保留，没有把修正结果覆盖到初版。

项目CMake另加 `WeaponMeshComponentLeaseTest` 两行目标定义，在全新目录重建持续ON DLL和该测试目标；flags与C1 trial逐项相同，新目标只执行一次，268 checks通过。9份源码before／after快照及10份记录hash已独立核对，旧C1三DLL和旧报告未变；没有重跑旧CPU或OFF／default构建。集成报告为 `analysis/hand-continuous-session-integrated-20261023/report.json`，SHA256 `ac7f10c2c2a6c386b343b9730ccc769d9be9f59b970fe2a0b72bb4efe70b4f59`；当前CMake `de450edc…` 机械移除新增目标两行可精确重构旧 `86dd65f2…`。新持续候选DLL为 `425f6868aef6d05b24ce153172386c8bb15f658348ad6950f4867f23443e8a0a`；它尚未跑游戏，不替换既有h验收身份。

纯组件阶段确定的最小运行切片是固定Lancer实例、phase1右手枪体：消费与handstream同一张认证GHW1票据，先取得组件租约再重新认证手部写入；原Tick、动作和stow helper前必须同步交还手部及组件两个journal。phase0／稳定phase2不写枪体。当前adapter已实现并进入上述R2测试，实际SetAbsolute／Translation／Rotation行为及跨Tick跟随仍待证明；左手副握、射线和正式武器切换尚未接入，CPU检查不能作为游戏／GPU／头显验收。
