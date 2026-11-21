# Smoke 烟雾手雷：交互需求与现有手雷会话交接

2026-11-11 最新用户范围与顺序确认：破片手雷与烟雾手雷均由当前会话负责，先完成破片，再继续烟雾专项开发。两者共用持续按住右Grip握持、真实松手投出、无需食指准备、2kg、直接跟手和保留原链条运动的交互约定；枪械按一下握持的规则不套用于手雷。烟雾保留原烟雾弹体、独立库存、效果与声音。既有Smoke源码和历史实测保留，不作为破片已完成或烟雾已验收的证明；完成状态仍以各自实际证据记录。


2026-11-10 首个Smoke定向Game41620：私有Nativea21efda6/PG9d6566e2，指定P2P目录，Source1、0.4、后台/静音/物理输入隔离；原地图找到真实Weap_SmokeGrenade拾取物（-3756.929,-372.514,514.885）。实际首败发生在拾取前步行：沿旧Frag斜向绕行撞到Y约-280处石栏，原步行stall保护中止；未抓取Smoke、未投掷或生成烟雾，不能算Smoke交互通过。动作子进程exit1，启动/生命周期runner仍exit0，两者明确分开。Native清理、输入恢复、武器drain、PG shutdown0/入口恢复、XR exit0及11项原件逐字节恢复均通过，shared guard已释放。

41620完整双路录像编码exit0；1张全跨度paired总览及2张实际阻挡接触图已亲看，普通启动第三人称暴露仍保留为未修复现象。已按原范围后侧通道改为三个真实步行waypoint（-4150,0）、（-4150,-472.514）、（-3756.929,-472.514），沿用同一navigate/stall/原X拾取，无传送、库存写入或碰撞豁免；新路线仍需Game复核。修正后87363与63345两次仅因现有统一互斥占用返回busy，均未stage/start，不算重跑Game。离线工作不保留游戏权限；媒体清理本次因其他实测活跃明确deferred，无删除，不算完成。证据analysis/frag-grip-20261108/smoke-phase1/41620-closeout.json及41620-route-repair.json。

本轮另保留owned-run-162219的启动前脚本错误：Frag→Smoke替换误改日志父目录，尚未创建Game即失败；11项原件恢复后修正为已有smoke-phase1目录，随后才进入上述41620。当前候选equipment-controls-prepared-20261110-160906-smoke-core，入口/脚本22项检查及指定游戏路径拒错检查完成；运行/恢复使用现有RuntimeGuard，保留原RepositoryLauncher.Owner检查，不采用远期前后继链。下一步实际空闲时仅复核修正后的原Smoke拾取及同一核心流程，完整目标active，Smoke效果/声音/返回、最终手型和头显仍未接受。

2026-11-10 Smoke生产源码接线完成并编译：独立Smoke普通PostPlayerTick owner已接实际装备hold代次、原producer历史、持续Grip自动准备/真实Grip释放、2kg直接跟手；配置与投前核当前Weap_SmokeGrenade/Proj_SmokeGrenade/WarAmmoType_GrenadeSmoke/Emit_SmokeGrenade及原脚本指纹，回执保留Smoke配置/入口/弹药/emitter/运动与参考代次和实际一次消耗。复用同一C0结果/组件writer及同一显示lease，增加精确Smoke认证与原补雷松手空手返回分支；正常stop/service/drain/reload/reset含Smoke准备与方向恢复债务，无新Native writer或输入发布器。原外部扣量仅在原未点燃准备已真实停止后可退休，不借此确认VR弹体/扣量或解除模糊投掷。

本阶段基础握点来自Smoke自己原模型counter刚性部分bounds的中点，并沿既有普通持有手腕基准；仅为完整流程的初始关系，未做最终GrabPose/完整接触/头显认证。状态聚焦2217项、基础mount与投掷原点一致性173项通过。当前分支1594份源码冻结的GearsPlayground目标编译链接0（DLL82b10f4d），编译器读取清单确认所有Smoke与共享接线文件；另有actual2956父源码私有接线编译通过。未运行测试全集，未构建Native、stage、启动Game或改手动候选/Publicv71，不能称Smoke已可玩或效果已验收。下一步在既有受控runner准备一个原地图Smoke拾取/握持/真实释放/原烟雾寿命与声音组件/补雷返回/新抓取短检查；实际启动前仅核实时占用与互斥，结束立即正常清理恢复，不恢复远期交接链。证据analysis/frag-grip-20261108/smoke-phase1/runtime-update.json。目标active，完整Frag/Smoke/手型/头显仍未完成。

2026-11-10 本会话范围确认：破片与烟雾均由当前手雷会话开发，不另开会话。持续按住右Grip、真实松手投出、2kg、直接跟手及原链条运动适用于两者；烟雾保留自己的库存、弹体、烟雾效果和声音。按最新阶段顺序，Frag已有核心闭环后继续Smoke生产流程，完整流程之后再做体验和最终GrabPose。

2026-11-10 Smoke资产接入增量：新增独立smoke_hold范围，只接受Weap_SmokeGrenade与根包COG_Bolo_Grenade内精确Smoke_Grenade模型，不扩展Frag或枪械范围；枚举追加以保留既有序号。实际资产选择器54项owned-memory检查通过，差异空白检查通过。原脚本导出另外确认Smoke覆盖ThrowGrenade、继承Weap_Grenade.AnimNotifyThrow；后续原调用必须保留该虚调用及通知路径，不能套Frag专属notify。证据analysis/frag-grip-20261108/smoke-phase1/asset-update.json。当前仍是状态模型与资产认证增量，尚未接通Smoke生产适配或取得游戏内烟雾验证。

2026-11-10 Smoke原调用适配增量：已新增weapon_smoke_original/preparation/launch.inl三个独立文件，保留原准备的release/target/spin偿还、原方向临时替换的恢复及精确旧动作身份保护。独立认证通用继承notify、Smoke覆盖ThrowGrenade与通用base ThrowGrenade，投掷结果要求覆盖层/base各一次、原ConsumeAmmo/Init/通知各一次，以及真实初始位置速度匹配；不绕过Smoke覆盖层的原事件。三个文件在actual2956 PG冻结源码的私有include注入中均出现在编译器实际读取清单，GearsPlayground目标编译链接0（DLL f28d5897）；并保存13份对应原脚本导出及哈希。首次误选ALL构建遇到旧测试夹具缺声明并中断，单独DLL成功不等于全部目标通过。尚无Smoke每帧调度、订阅者或清理生命周期接线，未stage/run/更新手动候选；这批DLL仅编译检查，不能当可玩Smoke交付。下一步补Smoke专属配置与真实回执、原始历史输入调度、返回和清理接线。证据analysis/frag-grip-20261108/smoke-phase1/original-adapter-update.json。目标active，完整烟雾/手型/头显均未通过。

日期：2026-11-09。用户已确认并授权开发，Smoke质量明确为2kg；尺寸、角度与接触范围由手雷会话自行标定。**沿用现有“手雷”会话，先完成Frag再开始Smoke，不新建会话。** 配套[公共合同](common.md)。

## 1. 最新规则与适用范围

W12最新修订明确：持续右Grip取用／持有，真实松Grip投出，直接跟手，保留Smoke自己的主体和链条。旧扳机准备路线已被替换。烟雾有独立库存、原弹体、原效果及触发规则；Frag的实跑证据不能自动计入Smoke。

Smoke开发已获授权，仍沿现有会话先后次序推进，不打断已拥有的Frag运行或混入其冻结检查。

## 2. 玩家操作步骤

| 阶段 | 玩家动作 | 必须结果 |
| --- | --- | --- |
| 选择种类 | 沿现有原版／主线手雷种类选择方式切为Smoke | 裤兜读取真实Smoke实例和数量，不克隆Frag；本稿不新增切种类按键 |
| 取用 | 右手到右裤兜持续按住RGrip | 原装备完成且本次Grip仍有效后自动准备；无须食指扳机 |
| 持有 | 继续Grip，正常移动／转腕 | 直接跟手和原链条运动；不增加拖拽惯性或在手cooking |
| 投出 | 主动松RGrip | 有效手部轨迹确定原点／初速，原Smoke生成后只扣烟雾一枚 |
| 轻放 | 手静止或低速下主动松Grip | 有效零／低速也能释放；失追不算松手 |
| 再取 | 原补雷完成后新裤兜Grip | 补雷不等于自动握住，空手显示稳定；新抓取恢复原组件 |
| 耗尽 | 烟雾库存为零时再取 | 空手、一次No ammo，不生成假烟雾，不改破片库存；原自动切枪按原规则 |

## 3. 真实运动与中断

手部原始轨迹决定速度，身体基准转换与原基底速度只合成一次；转头不改变投向。不能从渲染手、掩体人工位移或贴合骨骼制造甩手速度。

暂停、失追、回中／Snap、切种类、切枪、死亡／读档：废弃未投token及不连续窗口，旧Grip恢复不能重新准备或补投。装备完成前已松手取消，不在稍后补发。实际已生成Smoke继续走原弹体，不能清掉或重启其效果／引信。

Frag与Smoke互换后，旧缓存不能生成错种弹体或扣错库存。手雷贴附若仍为原版可用能力，必须独立选择并验证，不能将普通持有或松Grip解释成贴雷。

## 4. 烟雾专属效果

原Proj_SmokeGrenade.StopBounce直接停止并DoExplosion，GrenadeExplosion创建原Emit_SmokeGrenade。原“停止”由速度／反弹条件决定，不是第一次触地必然生效。

专属AttachInit直接产生附着烟雾，不套Frag的2.5秒贴附计时；继承的特殊HitWall分支仍保留，不笼统声称一切碰撞立刻放烟。原效果寿命、声音、遮挡与清理沿原emitter。不给破片爆炸伤害作fallback。

原抛物线预览按沿用Frag交互的方向关闭，不删除实际模拟／生成所需数据；烟雾的显示返回值路径需要自身认证与画面验证。

## 5. 显示与质量配置

保留Smoke原模型、分节链与原效果，不能仅换Frag贴图。投后原补雷期间下一枚不应留在张开手里；可复用已经适用的所有权／隐藏机制，必须独立验证原Smoke模式和组件。

用户本轮已明确Smoke质量 **2kg**，保持直接跟手、不加惯性。保留真实释放位置和速度，不因质量将速度乘除2；需要冲量时按已认证物理模式计算，避免重复施力。尺寸、握点和接触范围由手雷会话自行考虑、标定及验证。Smoke无普通换弹，不新增四秒奖励流程。

## 6. 必须验收

1. 烟雾种类取用、持续Grip自动准备、真实松手，原Smoke及库存各一次，Frag库存不变。
2. 上抛／轻放／转头，实际原点初速对应；零速有效、失追无效，近墙合法生成。
3. 普通停止、原烟雾发生／维持／消失、声音和资源正常清理，绝不出现Frag伤害fallback。
4. 有限库存、空槽反馈、原补雷、松手后空手和新Grip再次取用，无第二次意外投掷。
5. 暂停／失追／回中／切种类后旧请求不生成弹体，已生效果不刷新。
6. 原支持贴附和特殊碰墙分别取得自己的证据；未覆盖明确保留，不借Frag通过代替。
7. 主体／链条／手型、双眼无蓝线、出手至补雷的显示交接与头显手感。

## 7. 接入和来源

复用 weapon_smoke_grenade_interaction.h、grenade_throw.h与现有Frag中适用的公共发射／资产／可见性接口；独立绑定Weap_SmokeGrenade／Proj_SmokeGrenade／Emit_SmokeGrenade，不能执行Frag状态机后改类名。

来源：[W12最新职责及Grip规则](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/weapon-cards.md#w12)、[两类触发差异](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/interaction-review-v2.md)、[原Smoke入口](C:/Users/11/.codex/worktrees/6dof-single/gearsVr/docs/weapon-specs-20261028/original-entry-map.md)。



2026-11-10 本会话再次确认负责Smoke，顺序为Frag完成后开发Smoke。仍沿本文持续Grip/真实松手投出、2kg直接跟手和原链条要求。当前Frag14892在首次准备失败，已正常收尾；不把Frag的源码/离线检查或生命周期通过计为Smoke专项完成。


2026-11-10 后续actual2956已完成当前Frag的持续Grip准备／松Grip原一发4→3／原效果伤害／空手返回／新口袋Grip闭环，且正常收尾。第一组掌指受近敌遮挡、腕袖及完整旋转贴合仍未接受；Smoke专项仍未开始，不继承Frag的Game或贴合验收。既定顺序、2kg、直接跟手、无食指扳机要求和Smoke原独立效果保持。


2026-11-10 最新主人顺序为完整流程→体验→GrabPose；精细手型不再阻挡第一阶段。Frag2956基本闭环证据保留，旧姿态研究全部保留，暂停精调与为精调新增的Native消费诊断。当前实际Smoke头仍为Grip取用/trigger准备与释放，没有持续Grip自动准备/真实Grip释放，且Playground未接Smoke生产runtime；旧1713 CPU检查不证明本需求或游戏流程。下一步补独立Smoke状态与原准备/投掷/返回/清理接线，保留Smoke专属class/emitter及原库存/输入时效保护。证据analysis/frag-grip-20261108/smoke-phase1/current-gap.json。完整目标、体验和最终GrabPose均未取消。


2026-11-10 Smoke状态阶段已新增持续Grip模式、当前Equipment hold adoption、无食指激活准备、同帧真实Grip释放、旧hold generation/press不能投后重用及无边沿松手安全中断；Smoke class/ammo/emitter/audio和原一次生成/扣量回执校验保留。MSVC C++17 /W4 /WX /O2编译及1742项聚焦检查0；初始不存在测试字段/无效fixture计数失败已记录，未放宽frame保护。目前仅状态模型，尚无生产Smoke适配、原烟雾效果或Game实录；下一步原生产接线及明确sim provenance/原历史输入，精细手型不作前置。证据analysis/frag-grip-20261108/smoke-phase1/state-update.json。


2026-11-10 后续输入接线已加入显式RuntimeTracked/MetaOperatorSimulated来源匹配、原producer history Grip观察器、历史释放原时间/原姿态推进及原edge lease上限。真实源不能借sim数据；历史丢失/stream-tracking边界/改写edge时间会安全中断。新增空库存同hold代次提示去重，仍须新抓取才再次提示。MSVC编译与1807项检查0，包含历史释放早于轮询且速度2m/s保留、sim正确绑定和来源错配拒绝；仍无Smoke原Engine执行/烟雾实录。下一步原生产适配必须供原ring并显式2kg，模型当前帧兼容路径不算实际producer接线完成。证据analysis/frag-grip-20261108/smoke-phase1/history-update.json。


2026-11-10 17:45 Smoke生产接线已编译并在55712/37868实际拾取、持续Grip取用及原版准备；两次均在持有阶段中断，尚无原投掷或烟雾通过。37868诊断定位为history（reason4，boundary0）：同一原ring通过100ms来源连续校验，却未通过60ms运动间隔，精确间隔未记录。现将Smoke来源校验明确为100ms，当前输入仍60ms；任一sync/预测运动间隔超过60ms即清空速度样本，重新要求4样本/55ms，不跨间隔插值。其他调用默认行为保持。新增失败回归先复现，再通过Smoke2346、共享投掷及Bolo2016；私有PG88e73b06编译0，实际复查待结果。前两局全部11项原字节恢复并释放，37868双路2页已看但无烟雾效果证据。证据analysis/frag-grip-20261108/smoke-phase1/motion-gap-repair-evidence.json。


2026-11-10 actual54756首次取得Smoke持续Grip无扳机准备、真实松Grip释放位置/初速、原Smoke覆盖与基类各一次、原生成/扣量各一次2→1。私有PG88e73b06；两眼画面54..63秒可见浓烟生长/遮挡/消散，4张全跨度双路页已亲看，编码0。整case仍失败：只读全对象fresh扫描每轮约8秒，未认证Emit_SmokeGrenade，故音频组件寿命/返回/新Grip未执行，漏采因果未证。所有11项原字节正常恢复并释放。下一步仅观察器采用现有scan-local类hint预筛选、选中对象仍完整fresh身份/世界/删除检查并计时，生产DLL不变；等待共享欠恢复完成，不抢运行。证据analysis/frag-grip-20261108/smoke-phase1/54756-closeout.json。Smoke整体/最终手型/头显仍未完成。


2026-11-10 17:56 效果观察器聚焦修订完成：现有scan-local类hint只预筛选，选中对象仍须fresh注册表身份/实际class/WorldInfo/删除检查；增加每轮耗时、hint数和选中对象有限拒绝原因，同地址须按完整identity比较而非地址去重。直接提取生产嵌套emitters的9项离线夹具通过（旧身份/错世界/已删除/错class/Default/无关坏hint/地址复用），不计Game。175014独立候选prepare-only通过，生产PG88e73b06不变。当前核到外部恢复55384及Game32676/XR34940真实存活，恢复回执尚未落盘，暂不stage/start；不改别人的运行。下一步恢复结束后按原OS互斥复查同一Smoke核心case，目标仍active。证据analysis/frag-grip-20261108/smoke-phase1/effect-observer-update.json。


2026-11-10 18:11 53440在Smoke动作前Native准备失败：原strict anchor guard本身accepted，但一次耗时948.2323ms超过已有200ms采样预算；原因未证，不改预算。PG shutdown0/entry恢复、11原字节恢复并释放共享锁，但Native cleanup认证false、XR exit1，明确不是生命周期通过。按新主人独立实例要求，新增手雷独立runner/operator/read-input，仅按自己实例owner/game/XR PID+creation、专属输入/PG日志/录像/stop定位，不再stage/restore共享solo。5个实际适配模块dry compile通过。180855启动前0xc0000135定位为原freeze未带openxr_loader.dll；以固定SHA66946ea6复制原loader到自有XR输入目录后尺寸预检0。181025实例PG88e73b06/Nativea21efda6/Sim16840b27，倍率按新入口0.3，已创建30264，Smoke动作结果待定。无新手雷机制或Guard放宽，目标active。


2026-11-10 18:20 独立运行已迁移：run-isolated-smoke.py/isolated-smoke-operator.py/isolated-read-input.py，仅自己实例PID+creation及私有输入/PG/XR/配置/录像/停止，保留原Source1桥与身份新鲜度，无全局XR/8720定位。30264独立原场景/Smoke拾取/准备及正常Native/entry/XR0/game收尾通过，首次迁移漏带旧finite-range-ammo开关导致库存检查失败（原值当时未记录，不虚构具体值）；补回原fixture开关后38928库存2，真实松Grip请求token4已取走但派发0、弹体0、扣量0，首败在原调用前且原因未证，整case失败。38928同样全部正常清理。新增阶段16..20与已有请求时间诊断，条件和调用顺序不变，PGb4b398b5编译0，181847已启动实际复查。按最新全静音/后台/不抢焦点要求保留四flag并加入只读100ms前台观察，发现自有前台即取消后续动作并正常收尾；不动用户窗口/系统音量。旧实际54756一次Smoke及双眼烟雾证据仍保留，完整效果组件/返回/新Grip/最终手型/头显仍FALSE。证据analysis/frag-grip-20261108/smoke-phase1/independent-runtime-migration.json。


2026-11-10 actual47128（独立181847，Nativea21efda6/PGb4b398b5/Sim16840b27，0.3）新证据：持续Grip无扳机准备、真实松Grip原一次Smoke/库存2→1、位置速度对应，原覆盖/base/consume/notify/init均各1；85个效果样本认证实际Emit_SmokeGrenade、P_COG_Smoke_Grenade、SmokeSpewer AudioComponent及GrenadeSmokeLoopCue，最终实际bDeleteMe=true退休。来源scan最大299.2079ms，不再8秒全对象fresh扫描。释放保持不再投、Frag库存不变。2页完整双路总览已亲看且编码0，烟雾生长/遮挡/消散及投后空手可见；不称全角手型或声音可听证明。

此局在第二次新裤兜reach过程中收到最新主人“又到前台，立即停本方，禁止新启动试焦点”的指令，写本方cancel marker并调用准确实例正常stop；后续g.live拒绝，第二抓取未验收，整case保留false。Native/entry恢复true、XR0、launcher0、game absent、所有自有进程消失，无强杀。前台100ms采样1230次未捕获本方前台不能否定主人反馈，后台验收false；全静音/后台flag不是实际通过。已加本会话runtime-start-policy.json阻止新game/XR，继续离线工作，未暂停总目标。38928取token后未派发原因仍未证，47128诊断版成功不当修复。证据analysis/frag-grip-20261108/smoke-phase1/47128-core-effect-closeout.json。整体目标active；新抓取/耗尽/中断专项/贴附与特殊碰墙/体验/最终GrabPose/头显仍开放。


2026-11-10 18:34 离线暂停边界补强：原PlayerController.IsPaused脚本以WorldInfo.Pauser!=none判定暂停；原GameInfo.SetPause写原PRI。Frag/Smoke的soft pose-gap已准备保留分支此前只核独立输入/actor/旧action lifetime，没有显式暂停谓词。直接抽取两生产函数的离线Engine/input夹具重现paused与不可读仍保留（旧编译后exit1，8断言失败；初版夹具缺initializer_list编译2已修）。现仅在这两个分支要求原Pauser字段认证且为空；不可读/暂停不保留，交还原cancel/stop，原清理函数未加暂停前置。22项直接函数检查0，GearsPlayground目标编译0，私有PGc7d5b95d冻结；Native/手动包/Publicv71不变。保留此前Frag cosmetic hook，只复制函数增量，没有用root整文件覆盖旧冻结。仍未Game/XR验证，不称已修复实际暂停；runtime-start-policy仍禁止新启动。旧run-owned-smoke.py共享stage入口已明确退休拒绝运行，避免后续误写solo。证据analysis/frag-grip-20261108/pause-retention-20261110/result.json。


2026-11-10 actual39580在新background1110b/f4c88415模拟器下持有Smoke期间reason2、sampleage67.2344ms>原60ms，Grip仍1、库存2，无投掷/扣量。不是pause_guard拒绝证据，也不放宽新鲜度。首败后Native/entry/XR0/launcher0/game及自有进程正常退出；同期主人再次反馈前台并撤回自动恢复，保留其反馈，不用975次自有前台采样0来否定。两页完整双路已亲看、编码0；晚期正常cleanup第三人称帧保留。

离线查到Smoke时Frag适配器gate110，拒绝类型前仍做rig/history/inventory。现仅对无owner且无原准备债务的已知另一手雷class作提前negative reject，未知/本类仍走完整class/package/asset和输入权限；不能当67.23ms故障的因果修复。新增采样开始年龄/采样至stamp耗时POD日志，无新Engine读取。PG84d6cd28编译0并独立冻结，未Game。另已接--case pause-held：保持右Grip打开原暂停、B恢复不释放Grip、核实际Source1计数/cohort不变且旧握持不再准备、原取消释放不投、新裤兜Grip才再准备。生产分支的离线假Engine/input夹具正常与两负例通过（B忘保持Grip拒绝、原准备不停止不再发B/新抓取），5个适配模块dry compile0；不算Game或物理输入验证。统一automatic-runtime-pause.json及本方policy继续阻止启动/预检，不绕过旧入口。证据analysis/frag-grip-20261108/smoke-phase1/39580-freshness-and-offline-next.json。完整目标active。


2026-11-10 18:53 有限库存离线完整循环新增：实际Smoke状态模型+raw producer observer，连续两个独立hold代次、真实synthetic释放轨迹及原回执夹具，2→1→0、重复回执拒绝、持续释放不重准备、空库同Grip提示一次、新Grip新audio token且没有第三次prepare/launch。既有2346+新增59=2405项通过；原Engine回执明确为synthetic，不计Game。初版夹具将新hold应baseline误写为edge失败，核实原cohort合同后只改测试，生产输入规则不改。

为后续实际空槽反馈归因，在既有backend empty_sound中仅对slot1增加最多24条原调用结果/当前frame/sequence/原Grip计数日志，原current检查和原sound调用顺序/次数保持，无额外Engine读取或声音调用；日志dispatched只证明调用返回，不称静音环境中可听。首次诊断误用vr别名编译失败，改为gears_vr后PG429b050f编译0并独立冻结，Native/Publicv71未动。统一自动启动禁止标记仍true，本轮无Game/XR/输入/录像。最后一枚原自动切枪、空槽实际声音及画面仍待运行验收。证据analysis/frag-grip-20261108/smoke-phase1/finite-empty-offline-update.json。


2026-11-10 18:59 已接独立--case finite-empty，默认core保持：要求原拾取恰2，无setter；第一枚/效果寿命/返回/新抓取后第二枚1→0，原completion逐项要求reserve0、无continuation且completed1，再核实际原武器切出Smoke。随后两次真实空裤兜Grip分别须产生一条slot1 original声音dispatch（按不同原squeeze press计数），持续Grip移手无重复提示、无第三prepare/launch，Frag库存保持。限制2投掷、4次口袋Grip，第二弹体完整效果寿命/实际音量不在本专项中冒充已验收。生产验证分支+实际冻结trajectory/ease在离线假Engine/log输入sink完成正常与3负例（重复反馈/不自动切枪/错误completion）均通过，错误后不再后续抓取；5模块dry compile0，原pause分支负例仍通过。仍仅脚本准备，未Game/XR/原调用或输入桥执行；统一自动启动禁止true。候选185813，PG429b050f精确复用，无新产品DLL改动。证据analysis/frag-grip-20261108/smoke-phase1/finite-driver-prepared.json。


2026-11-10 19:12 原合同implementation-handoff明确掩体人工平移仅改原点、不注入初速。离线重现Smoke旧same_mapping对任何game_base.position变化取消已准备持有（新回归编译后exit1），Frag未使用该位置拒绝。现Smoke将同一已认证身体原点平移与参考/单位/旋转换代分开：原始XR点拟合速度不混入平移，当前认证origin仅进入出手position，原pawn velocity只加一次。身份/追踪/参考/单位/旋转及当前60ms/原release lease均保持。

没有历史身体origin时不能把最新位置冒充过去释放：如历史release晚轮询且origin在该间隔变动，仍mapping_boundary取消不扣量，mask16明确改为这一未解析历史原点原因；不插值、不制造Frame。原点稳定的历史释放仍允许。2518项检查通过，含静止手+大人工位移、2m/s手+角色基底速度一次合成、ref/unit/rotation/nonfinite拒绝、两种历史原点正负例；初始旧“位置变化应取消”单例按合同由新完整例替代，所有真实坐标换代拒绝保留。九个本地依赖闭包root/实际冻结源码除换行BOM外文本一致，复用该检查；PG新编译0独立冻结。尚无Game/XR，不称移动投掷全通过，也不称实际39580过期原因修复。来源与具体SHA见analysis/frag-grip-20261108/smoke-phase1/body-origin-offline-update.json。统一自动启动禁止仍true。
