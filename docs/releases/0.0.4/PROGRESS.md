# GearsVR 0.0.4 开发进度

本文件保留武器与掩体两个分支的开发历史，便于共用 Native 手部实现合流。各条记录仍以其原始验证范围为准；私有候选、只读采样或离线检查不代表完整功能、默认构建更新或头显验收。

## 2026-10-26 — 整理typed Tick／C0查询的独立装备观察边界，等待共享头

按Body／Astra收窄方向，仅整理Client／PG只读方案：在同parent callback、actualGT、Client epoch／Gate内提取exact committed key／五对象作历史关联；新raw40负责右Grip资格，PG以当次Game／heldGeneration／双typed asset／attachment／全部末读形成query-bound metadata witness。旧V1 tracking确实参与frame_active／右Grip／world_grip资格，不能删sync检查或伪造Frame；原copy_frame_on_gt／source_game_on_gt／PG V1和f85／EO不动。方案额外明确Hand committed本身无publication／binding_generation，不能假称从Client读出；domain_ready只有ready／depth围栏，没有可导出的actionGeneration，须等Body明确共享action语义。

最小本地操作建议单Gate内执行固定metadata reader，禁止递归调用旧Client source方法；两次fresh Game包围双asset／实际attachment，保留各reader内部末读及最后epoch／holdGeneration／domain／time检查，返回真实观察范围而非Frame／world pose／grant。具体提案见`docs/weapon-tick-mount-equipment-witness-plan-20261026.md`。本轮仅源码依赖分析／文档，无生产改动、构建、case或Game；等待Body唯一共享typed头及hash后才实施PG路径，不抬成熟度。

## 2026-10-26 — EO实际查询耗时缩短，持续门仍失败，转查生产消费时序

Body独占完成EO及原finalize一次并交回空窗口。其报告Game1500／Native2480512716131，19 held全部完整pose通过、四角色实际应用各19、380event0drop；原firstheld40／6秒仍失败，未移手，HandStop5／quiescent／freehand继续／输入恢复／bridgeclose与正常106／Native／entry／XR0／PG0／十文件／audio10／准确PID恢复通过，host瞬时false保留。Root本次仅核对保存的PG／预算时标与清理字段，不重算19条姿态，不扩大对身体侧结果的独立验证声称。

Root按query及全部来源身份关联EN／EO原件。PG握枪样本EN26／EO27条的callback中位6.5356→3.7464ms，实际样本缩短2.7892ms；EO进入PG时原sync年龄中位反而44.87425→47.0972ms。两边各16条最终response12均在response入口已超50，EO范围50.3525–53.4922ms。EO Native前缀含query0，只关联PG1–31；失败的15条关联样本在compose进入原sync已45.6674–48.0854ms，compose→query中位0.983ms。不能错按行关联PG32，不能用独立运行计数或算法次数宣称配对因果／全场景保证，也不能用前缀解释全65 stale_broker。详细范围见`docs/weapon-en-eo-query-timing-20261026.md`及ignored `analysis/weapon-parser-candidate-20261026/en-eo-saved-query-comparison.json`。

冻结候选与EO，不追加同类解析微优化、新case或Game；身体侧继续回查生产消费时序，保留40／50与原门。原19条姿态通过不代表持续握枪／移手／射击通过，整体36%与W2 40%保持，无默认集成或头显接受。

## 2026-10-26 — PG单次查询解析优化完成源码与定向验证，待原动作验证实际收益

按统筹确认的唯一候选范围，Root在PG provider取得独占entered后安装本次查询的解析工作区，finally恢复原TLS并清空工作区。FName只记本次查询内的长度界限；命中时仍重新读取slot、index、全部字符和终止NUL，同长文字变化仍由原快照见证捕获，长度或可读性变化拒绝。三个原field扫描器将重复地址查重改为精确地址哈希集合，原前缀数组／4096上限／首个同名字段及类型／所有读取与末读保持，嵌套扫描或容量不足回退原线性查重。保留两次fresh Game观察、两种typed资产、独立最终观察、原身份／资格／时间戳／40与50ms及默认非trial路径；未改Native、wire或R2运行开关。

新PG完整TU严格构建通过，SHA256 `f85a00dda92f09c0c7548151b676c7cb37dc5ec0879472201855ad0248615650`；实际callback／attach定向933个断言与实际三个scanner／fresh_name定向8330个断言均通过，覆盖重入、工作线程TLS、SEH恢复、查询清空、碰撞、循环与4096边界、文字／指针变化和读取失败。8330是断言数，不能写作8330个场景。独立Astra增量审查无P1/P2发现，审查本身未执行测试。沿用PG0ec1相同编译flags和vendor，763份源与构建receipt一致，生产侧无test-stats宏；准确增量为六份已有构建／源码／测试文件和两份新头文件／测试，证明及唯一build位于ignored `analysis/weapon-parser-candidate-20261026/`。

算法示例的8,390,656次线性比较降为6,153次哈希探测、LivePawn字符读取9次降为1次只说明减少操作，不能换算真实游戏节省毫秒。原EN回复需省0.9208–6.1447ms且后续仍需Native消费，是否足够须由Body独占同原动作验证；保留原firstheld40／6秒与106完整恢复，收益不足则回查生产消费时序，不连续堆微候选。尚无本轮Game／XR／GPU／头显验收，成熟度不增加。原EN held日志标签另有补充：前缀10次PG ready对应6条held记录，其中5条pose通过、query30失败；原回执保持，日志成员关系不能叫姿态成功。

源码提交`73076dd`后，从原EN派生唯一EO并交Body独占运行。最终pins `365adfc4d3fa5df273c5c986e5b1cb6c2a6a00fb1bed3a422cd535342985341e`，coordinator `27bed41e965077d0c0cd3821df7845b1953baee3390583c7888e5d8349138d2a`；Native46／BQ／CY和所有动作／门槛／后台保护保留，manifest只改PG摘要，私有Python只必要namespace／PG摘要／coordinator校验摘要替换，16个PG导出名称保持。PG PE及当前选择元数据已更新，所有pins与原EN保护通过；封存时无instances、无Root Game／XR。完整原两session与一次normal finalize命令在ignored EO的`PG-PARSER-README.md`，交付后不再改case。此为可运行候选交接，尚非实际收益或持续玩法通过。

## 2026-10-26 — EN保住27／28条完整姿态，继续收口PG原来源50ms成本

Body独占完成EN及原finalize一次并交还空窗口。实际Game20148／Native2447539276000，新typed／v3／alpha分别SHA a987aa5a…858f、da05cc27…0004、5e1e7150…c1af；28条中27条consumer/end、Local、FK及四角色actualApplied通过，552个事件无drop。Root直接核对三原件、逐条sequence／tick／Compose／thread／PG query，独立重算原矩阵腕位置误差一致。异常准确为sequence5／query30，四角色no-token，不能忽略或说成最后序号；q30本身PG ready，不能混作PG source_mismatch。原fixedheld40／6秒门仍失败（ready43／held28），未到后续移手、射击或头显接受。本轮HandStop5／quiescent、cookie0后freehand继续、controller恢复、bridgeclose及106／Native／entry／XR0／PG0／十文件／audio10正常恢复通过，host瞬时false保留，EM旧freehand失败不覆盖。

PG实际只保留first32／总111次，truncated1；与Native原预算query1–32的Scene／publication／cookie／原sync逐条关联一致。该前缀6 unheld、10 held-ready、16 held在完整retained／Game／资产／bind通过后由sender::response最终context返回12；16条response入口原sync已50.9203–56.1443ms。其PG耗时6.3998–7.3612ms，仅使回复≤50就需省0.9208–6.1447ms（中3.2709），尚需后续Native逐写复核。全程broker source_after63不能由此32条全部分类，末failure110与last_query111/raw0也不强配；budget commit_end不是成功或精确CAS时刻。

按Body要求，本轮止于已有日志／源码分析，未改PG／Native、派生、构建或重跑。Root给出下一最小可改边界为claimed单次PG callback内纯解析工作区：长度界限辅助fresh全名读取、精确地址集合加速field cycle查重，保留原全部fresh Game／typed资产／独立末读／身份／generation／50ms与默认其他路径；先前copy比例不充作零售墙时收益，现无收益承诺。具体范围、缺口与禁止续时／放宽门槛在`docs/weapon-en-pg-budget-review-20261026.md`，独立回执为ignored `analysis/lancer-native46-review-20261026/`。仍须另保留seq5 token异常，下一原动作必须有真实增量，不能重复同46碰运气。

## 2026-10-26 — EN封存Native46同次已提交消费认证候选

Body冻结Native46最终build-cover-body46sim2后，Root仅从EM派生EN，准确选DLL `65f55c25b111bea5c7e818dfb3bdaf0acd8a7bb2ef243abdd1440450c67a951b`、manifest `d523040f…cea80`、receipt `4b1fa4b9…bb6e0`与最终source46-before-build2 `459c7671…94709`。该候选改变同次已提交消费认证并将hard故障独立；写入40／Scene资格50／原alpha1-Scale0和PG／wire不变，已有alpha侧录保留。Body实际完整TU／43exports／22startup／23preflight、32红例／真实owner-Rig-Kernel与ActualContext/Kernel wrapper定向结果及Astra关闭记录按其receipt来源复用，Root未重建、扩矩阵或Game。

最终EN pins `0866765458013f065b78db11602ed2518d6dabafe603a1ab16345a11f2976fa6`，coordinator仍`1b4f9f2a…7da95`。PG0ec1／BQ／CY、控制器原8轨迹、firstheld40／6秒及106完整保护保持；INI逐字等EM，所有私有Python仅case/instance替换，证明在ignored case的native46-selection-preflight.json。原EM输入／结果及alpha／typed／v3三原件保持。已向Body交精确原两retained session及一次normal finalize命令并冻结EN，交付时无instances／ready，Root不自行Game/XR。尚无该轮行为接受；仍以实际Local/FK／持续held／移转／交还验收，不把新消费候选、原grant或返回时标当作最终姿态证明。

## 2026-10-26 — EM诊断取得右臂prior-compute拒绝链，保留返回时标解释边界

Body独占完成EM及一次normal finalize，完整106／Native／entry／XR0／PG0／原十文件／音频／exact PID正常恢复并fresh CIM空；本轮HandStop4→5／quiescent、双controller恢复与bridgeclose通过，但cookie0后freehand继续超时，独立保留，不用正常清理或EL成功覆盖。first-held门仍失败，未进入后移转；Native45只是被动诊断，未当修复或跟随接受。

ARMALP01原件SHA256 `dc9f4e15411f070b51c83ac6180598f44a421b584dbeda39a0bffcc2acff3e62`，实际Game53920／Native incarnation2410681145942；24条held记录、330个事件，dropped0。Root直接按144B结构核对全部raw字段与修正后的em-alpha2，并逐条绑定typed4424／v32120的sequence／tick／Compose／thread，一致。RH limb／single各24次tokenRejected1，均对应compute_failure6（identity）／weight_failure12（prior_compute），尚未进入sample／owner；LH limb16与single5次真实sample okay、alpha1、Scale0、readmask127，继而owner通过并记录actualApplied。

统筹指出的覆盖解释已由Body修正reader标签：Compute与owner中的零填不当作最终packet观察；原em-alpha1保留，Native和游戏未重跑。Root只使用em-alpha2的修正语义，并独立用v3真实QPC频率及typed原sync复算96次Compute RETURN源龄；RH失败为50.0018–50.9297ms。按failure分组，code1的21条返回观察为47.8729–50.0303ms，code6的75条为49.9995–50.9297ms，有边界重叠；这不是精确早先拒绝时点，也不将qpc排序为阶段耗时。

源码链显示continuous_hand_owner control_begin／plan仍依赖旧same_source50，拒绝可令scope sticky，使CCTRL.current依赖的hand.control_current失效；已提交body在owns内部另用retained consumer_current。此静态链与运行拒绝相符，但具体因果及原检查不松的retained消费接口仍待Body的Astra设计／actual Host复现，当前没有据返回时标单独认定修复点或接受新行为。Root不派生新case、不改PG／Native、不Game；独立raw/年龄/完整清理结果在ignored `analysis/lancer-native45-alpha-20261026/` 的em-alpha-root及em-closure-and-return-ages，EM原件和host瞬时false均保持。

## 2026-10-26 — EM冻结Native45同held alpha诊断，原消费行为不变

Body交付唯一被动alpha诊断后，Root从EL派生EM，准确选择Native45 `7d0d2d542d2c661a04f4b352a5c9ad791a3db049ea95b9c6b27273c3f9813be2`、receipt `36e9be28…8ed91`与冻结source45 `c7fc7c4c…a7baa`。PG0ec1／BQ XR1a05／CY378及BodyTickInput1全部保留；所有私有Python只必要namespace替换，native_sections与EL逐字相同，43个导出名称相同。原8命令、firstheld40／6秒、alpha1／Scale／40／50／PG／wire／实际XMM编辑、0.4后台与106完整恢复保护不变。Body原完整TU、startup／preflight及定向owner／CCTRL结果按receipt复用，Root只做选择／封印，不重建或加矩阵。

最终EM pins `05f347344c5b7f6f903364602556515dd94f759b3a6562dc4b4059a3bd89d1bc`、coordinator仍`1b4f9f2a…7da95`；namespace与INI证明在ignored case的native45-selection-preflight.json，原EL输入／结果及typed／v3原件保持。NATIVE45-README给原两retained session和一次normal finalizer完整命令，Root已交Body独占窗口并冻结EM，交付时未启动Game/XR、无instances／ready。

新sidecar为native/body-arm-alpha.bin，ARMALP01／Header64／Record9256（40＋64×144）／first32成功held，每条64 events，最大296256B；Header末u32是event_capacity64。它按hand_record sequence／tick／Compose／thread关联原typed4424与v32120，不使用旧first128容量冒充held。Body／Hand／CCTRL stopped且全drained后CREATE_NEW导出，sample Reject／owner deny／最终applied分别记录，qpc多为原return观察，不能排序或当阶段耗时。Body严格reader已就绪，Root finalizer仍保留原范围。尚无EM实跑结果，诊断选择不是修复或持续跟随接受。

## 2026-10-26 — EL取得29次held提交，原控制消费后覆盖仍阻断持续跟随

Body独占完成EL及一次正常finalize后释放窗口并确认fresh CIM空。新typed输入361次取得均有效并正常Tick end；主握ready49，实际held commit29，原first-held40／6秒门仍失败，未到后续降枪／移转，未降低门槛或重跑碰运气。实际Game51152／Native incarnation2350130212567，新日志128360B（Header64＋29×4424），原v3为61552B（Header72＋29×2120）。Body严格reader验证输入／PG／Native join及重算数学目标：29条腕目标误差全0，head goal约8e-6u，但只有1条完整pose_pass，不能说持枪跟随通过。

Root独立核对两份原binary的header、29条sequence／tick／Compose及PG response／parent query／原Scene key，另直接由v3 SpaceBases×CTW重算两腕位置误差，与Body reader一致。更具体的阶段证据是29／29进入原consumer时九骨均逐字等planned（mask511），消费结束后仅1条仍全相等；alpha grant mask的0／1／3／7／15分别为4／7／10／7／1条，与after_local保留mask448／451／455／479／511逐条对应。29条中head／腿51／52／55保留，alpha四角色分别对应8/10、11、32/34、35的保留；这是本轮精确相关，不替代具体拒绝分支或修复证据。右腕后FK位置误差中位31.8073u，左腕中位0.0001067u但部分明显偏离，旋转也须按reader完整检查，不能只取接近的位置样本。

Body后续仅扩读现有POD的el-pose2进一步明确这些是owner grant，发生在CCTRL最终后置检查和XMM／packet改写之前，不能等同实际applied。Root将29条新增mask／count／original bits与原v3逐条核对一致，pose_pass仍1；回执为el-owner-grant-recheck.json。下一最小诊断为同32条held关联的alpha sidecar，区分既有单次sample Reject、Native deny stage与最终packet应用，不复用旧first128容量，不放宽alpha1／Scale／40／50；该sidecar尚未有实跑证据。

原记录的29次normal_consumer_end均通过，本轮HandStop4→5／quiescent、两controller恢复、bridgeclose及cookie0后freehand继续实际通过；EK／EI的超时原件保持。完整106／Native／entry／XR0／PG0／原十文件／音频／exact PID正常清理通过，host瞬时false仍保留。committed_ns为postcommit观察，原sync在该观察时刻超过50ms不单独证明任何一次CAS超龄；Root没有将该时刻冒作最后检查时间。独立结果在ignored `analysis/lancer-native44-consumption-20261026/` 的el-consumption-root与el-alpha-role-correlation，源场景未修改、未重跑或重建。Body下一步只定位原control alpha交接拒绝，PG／40／50／原行为门保持。

## 2026-10-26 — EL绑定Native44显式typed消费者，原行为门保持

Body交付真正typed消费者Native44后，Root从EK派生EL，选择实际build-cover-body44sim2的DLL `1dafeb65f369e01b38b3b961f25bad5f3dfa27ff2930cd9fd9550b91f100e180`、最终source44-before-build2来源`3df45c10…92140`；PG0ec1／BQ XR1a05／CY simulator378不变。INI只移除BodyTickTrackingProbe、增加BodyTickInput enabled1；不是只读探针，明确新Tick／当前C0基准最终每写40ms、原Scene资格50ms，原Core／PG／wire语义保留，不宣称旧Scene40门通过。Body原完整TU／43exports／22startup／23preflight及其定向typed／真实C0Host验证按对应receipt复用，Root未重编或另跑Game／旧矩阵。

最终EL pins `0a2e635328e5408853d56121f087835c3587fc532da9ff1374131f034c6fcb8f`，coordinator仍`1b4f9f2a…7da95`；namespace证明在ignored case的native44-selection-preflight.json。host／coordinator／runtime_support／finalizer／106验证器／dispatcher仅case/instance替换，profile去掉各自唯一新section后逐字一致；first-held门、原8命令、R2 OFF、所有身份／输入／存档／音频／后台／退出保护保持。EK原输入、结果及SHA69266c…d1b9的探针原件未变。两次派生recipe在创建case前分别因多余转义断言、旧manifest文件名拒绝，失败版本保留；最终另修继承的diagnostic-only描述并重新封印，未改变可执行host。

EL原启动三命令及当前选择已交Body主会独占运行，Root冻结case、不并发；交付时没有instances／ready，尚无实跑接受。新的成功关联原件为native/body-tick-commits.bin，BTICMT01／Header64／sizeof新POD／前32条，须由Body严格reader与原body-hands-commits.bin v3按sequence／tick／Compose逐条关联；包含独立Input、当前base／CTW、avatarAsset、原PGResponse与parent receipt。committed_ns仅postcommit观察，不是CAS精确时刻。Root finalizer仍只验证原v3及正常清理，新的日志不代替持续held／小移转／交还或实体头显接受。

## 2026-10-26 — EK实际证明真实Tick新输入可取得，旧held／交还行为仍未通过

Body独占完成EK及一次正常finalize，释放窗口并确认fresh CIM无Game/XR。BTIPRB01原件122176B、Header64／Record3816／32条，SHA256 `69266cbe43418984477a5ab3208e474ce295234cbca664f58e8738d336a2d1b9`，实际Game55712／Native incarnation2333248191503；Body严格reader关联通过。32／32 valid、newer及可选真实C0 basis存在，raw sequence比原Scene前进2–4；新hands.sync源龄0.2243–10.8548ms（中位7.36925），原sync29.3217–47.2362ms（中位41.1907），采集0.0251–0.0558ms。basis观察在采集结束后0.891–11.3381ms，原时标未更新。首32可能在held之前，仅证明本轮同真实Tick的新成组来源及当前基准可取得，不推为持枪轨迹／目标计算或稳定性能结论。

Root直接核对原binary完整header与32条tick／thread／cookie／起止时标／failure／valid／newer／basis prefix，逐条与Body严格reader一致，另独立重算其聚合指标及准确session；不冒称重做全部字段语义解析。回执为ignored `analysis/weapon-parser-budget-20261026/ek-source-availability-root.json`，旧消费者stable状态另解为`analysis/lancer-main-mount-ec-rejection-20261026ef/ek-native43-diagnostics.json`。

原消费者未切换，first-held门仍失败：ready16、stale／fault／context拒绝0、held commits0、post40拒绝17，降枪和小移转未触达。HandStop4→5／quiescent、两手恢复、bridge close正常，但停止后自由双手继续更新检查超时，不能称交还行为通过；正常106／Native／entry／XR0／PG0／原十文件／音频／exact PID全部恢复通过，host瞬时no_remaining_games=false保持原值。EK现场与PG0ec1冻结；Body继续实现真实新typed输入和同C0关联，不改旧Core／PG wire，后续候选再沿原门验证持续held、移动与交还。

## 2026-10-26 — EK选择Native43只读Tick探针，原消费者／动作／清理保持

Body交付Native43真实outer-Tick只读探针后，Root从原EJ精确派生私有EK，仅替换Native build／DLL／hash／provenance及PE元数据，向原native_sections追加`[BodyTickTrackingProbe] enabled=1`。新Native SHA256 `1517cfb04235655c03e4345df9afc13fc903d4185286aace15d549d2c02dc426`，PG仍a54d592／0ec1、BQ XR1a05、CY simulator378。43个导出名称、构建receipt／冻结source43来源已核对；Body原完整构建／22startup／23preflight按明确来源复用，Root未重编、未运行旧矩阵或Game。

EK pins为`b6f1820a87eaa660c2813ddda7404d4af13a24f3d04c373ff0a297286469f104`，coordinator仍`1b4f9f2ab451d8a37232033ac00f3fc32e5a71c16da630a610c53abf5997da95`。host／coordinator／runtime_support／finalizer／106完整验证器／dispatcher逐字保留，仅必要case/instance namespace不同；profile去掉新探针section后与EJ一致。原first-held门、最多8命令、R2 OFF、40/50、0.4／behind／observe-only／mute-restore／physical-background-OFF／原十文件保护均保持，EJ原输入、结果及预算原件未变。离线封印及完整启动命令在ignored `analysis/equipment-native-rgrip-runtime-20261026ek/` 的native43-selection-preflight与NATIVE43-README。

Body确认fresh CIM无Game/XR后已接管唯一EK运行窗口，Root冻结case、不并发启动；本条尚无该轮实跑结果。正常drain后探针原件为实例native/body-tick-probe.bin，BTIPRB01／Header64／Record3816／最多32条122176B，ABI及严格reader由Body实际构建验证。前32个真实Tick可能在持枪前，只可用于判断真实新整包／原时标／来源与可选C0基准是否可取得，不当作同次held、消费者切换、持续跟手或头显接受；Root finalizer保持原清理范围，不新增探针成功门。来源只读证据通过后才接新typed消费与原持枪小移转／交还验证。

## 2026-10-26 — EJ排除Native计算热点，先核实源龄预算再选择改动

EJ由身体会话以冻结Native42／PG0ec1／BQ／CY及原动作独占执行，Native ready17、held commits0，首门仍未通过。此次HandStop后的自由双手继续更新与106完整正常恢复通过；EI同项超时仍保留，不能用EJ覆盖旧失败或据单轮证明稳定性。预算二进制11880B、24个query及1个无query源龄拒绝记录，SHA256 `a5656dacaf91b57a8d1615b8a4c818d0d01825d464ed9f3485bc8e92ceb092b7`，实际PID45644／Native incarnation2288270726650。Body严格解码与Root独立原字节／身份／阶段比对一致。

17个ready样本的Compose入口→query为0.9256–1.1539ms，query为6.3464–6.8692ms；sync在Compose入口已老36.5061–39.1390ms。身体侧源码与原时标说明Scene中发布的同一快照经缓存被后续真实Mesh Tick消费，publication→Compose为28.2771–30.1130ms；读取不会更新来源时标。该区间尚未细分为CPU／GPU或精确游戏帧，不能称Native模型计算30ms或靠缓存模型修复。

Root从同一binary逐条复算原40ms预算：query入口剩余-0.0922–2.4939ms，中位0.6156ms，2条已超限；再扣实际query结束→first40检查的0.1007–0.1962ms后，3条即使query耗时为0也无余量。其余14条要求query墙时减少64.0%–97.4%，各自所需倍率中位9.21；17条允许query耗时的中位0.5119ms，且未包含后续hands／final40或安全余量。此为保存样本的反事实预算上界，不是性能预测、FPS或最终提交证明。原始stage／identity与解码报告逐项相同，计算及独立回执在ignored `analysis/weapon-parser-budget-20261026/`。

PG源码存在同次名称逐字重复读取及字段链线性查重，但解析优化尚无覆盖上述预算的实测证据。新增独立owned-memory计数夹具，除copy入口计数两处外与原fixture逆向逐字一致，使用未改的实际observe：普通例11025次copy／515次fresh_name／27个entry，加64个同名ignored前缀为15761／643／28；两次观察均接受。假设完美的同query长度界限命中且仍fresh读取全部字符和NUL，最多省总copy调用33.01%／40.82%，字符字节不减；未实现此优化，也未测visited-set或零售墙时，不能将调用比例变成实际耗时比例或宣称达到所需倍率。证据为ignored `analysis/weapon-parser-cost-20261026/result.json`，SHA256 `19739a31b6bd495a6cef9d1a67db0978592a0b8057df3d14d93081d46aef8f94`。

先保留PG0ec1，不叠加小候选实跑。已将预算与计数交Body及统筹，用于选择保留原40／50ms、完整可变值与身份重读、独立末次Game观察的最小合法方向。消费者源码另确认parent已提交Hand Frame、retained Rig与BodyContext必须属于同一真实Scene／publication／generation／reference和原始输入，不能只替换一侧controller或把C0当Scene。真正新来源须有独立来源证明，不能延后发布旧值、重标时或在当前C0偷换cohort。Root未改Native／PG生产源码、默认启动器或启动新Game；完整武器功能和实体头显验收仍未完成。

已有Astra／High仅审新来源的消费者协议决策，未重审旧矩阵、开IDA或改代码：现V1不能把Tick冒充Scene，但main-mount回复仅含固定grip-to-wrist校准／身份／revision，因此可考虑显式区分“同步query-correlated的原Scene校准来源”和“独立Tick姿态来源”。PG V1布局与reader可保持，前者继续原50ms完整资格检查，后者须由Native新契约独立证明真实整包tracking、base／reference及原sync／receive时标，并在最终使用复核40ms。此方案尚未实施，不能称旧Scene的40ms检查已通过。

新组合契约必须在同一实际C0连接相同parent／lifecycle、当前world／controller／pawn／mesh／weapon／component／asset、hold/model generation、geometry／scale／units／basis和reference continuity，分别保留两份来源身份；不重写旧response key／binding generation，不跨query缓存ready，不二次解码Grip、取得Scene support lease、授权fire或增加writer，失效路径需明确。整体Hand／Rig迁为Tick语义的影响更大，必须显式协商新协议；若Body能将真正一致的Scene生产／消费提前，则可保留现协议。此窄决策已交共享Native唯一负责人及统筹，Root保持PG冻结，等待具体来源／join实现后接收验证。

## 2026-10-26 — EI的27次持枪query全部有效，剩余阻碍转实际C0预算阶段

EI以a54d592／PG0ec1、原Native41／BQ／CY及同动作实际运行。正常退出后的PG记录29／29无截断，query1–2为真实unheld；query3–29全部context／retained／response ready、source bind／资产检查通过并正常publish。当前样本的27个持枪回调为6.2567–6.8577ms，最终sync源龄43.9903–46.9392ms；Native稳定ready27、stale／fault／context拒绝0，commits仍0、post-admission拒绝27。该组是实际查询样本，不是匹配FPS基准或无限期稳定性结论，也不将独立累计计数伪装成逐query的后续提交trace。

取Lancer、Source1和同次metadata／身份校验继续有效；held40首门仍未通过，降枪／移转未触达，不能宣称枪体跟随完成。HandStop4→5／quiescent、两手恢复、bridge close及106完整恢复／Native-entry／XR0／PG0／原十文件／音频／ownedPID正常清理通过。原EI输入和所有结果保留。

后续逐项核对纠正本节原先的freehand通过表述：实际runtime-review的bridge_error为post-quiescent继续提交超时，body_continues_after_stop并非true，body_hands最终0；该行为检查未过，cleanup-only accepted不能代替它。after_quiescent_baseline为attempts159／solved0／commits0／status8=158／cookie0，没有成功的after_hand_stop_native快照。主门失败后右手仍在肩部，未到right-ready，因此该停止姿态的freehand可达性也未独立证明；当前不把此超时归因于PG parent未交还或特定solver缺陷。EJ保持原输入如实复测，结果另验。

阶段／原时标／exact parent关联及稳定计数已保存ignored analysis/lancer-main-mount-ec-rejection-20261026ef/ei-query-ready.json与ei-native41-diagnostics.json。此后冻结PG0ec1，不继续猜测或叠加PG改动；身体会话将定位compose／capture／model／Goal／gate／query／post／hands／CAS实际时间，再决定Native的具体修正。Root不改Native、不并发Game、不更改40／50ms或源标记，完整武器功能及头显验收仍待后续实现。

## 2026-10-26 — EH取得首次有效主握回复，继续收口附着检查中的重复Game观察

EH实际取Lancer仍成功，held首门未过且commits0；首次Native ready1。PG正常drain后16／16未截断记录明确：q1–3真实unheld；q4–11及13–16初始context ready、retained complete且bind／资产均ready，最终response返回source_mismatch；q12成功publish并被Native接受。held查询约11.2–11.9ms，开始sync年龄约38–40ms，q16末sync50.905ms；Native stale12全部source_after，post-admission14，projection／pre-source／C0-current0。一次ready不算身体提交；与EG的稀疏max差异不当作匹配性能基准。106及原全部正常清理通过，EH原现场／日志保留。

进一步源码核对发现attached_frame内部前后current各再执行一次完整Game观察，因而上轮retained仍有四次观察。仅retained路径改用元数据core：将旧attachment读取主体原样提取read_attached_frame，old attached_frame保留weapon非空→current→core→current短路顺序，R2／其他调用及preview不变。retained通过prepare_observed的编译期分支使用core，仍由同次copy-frame真实Game与末尾独立source_game／完整same_game包围，并在core前后增加廉价GT／domain／armed／注册／frame_active栅栏。资产两读、typed schema、唯一right-bone／可见性、array／entry／Mesh selector复核、generation、eligibility及final bind均保留，无跨帧缓存／新权限／时标续写。

实际adapter318项通过，含retained-core／preview-wrapper路由、core内Game／generation变化及softinvalidate；完整严格PG构建通过，761源／vendor稳定。源文本证明metadata主体除签名和两次current外逐字保留，wrapper顺序固定；该证明不是二进制或实跑等价。Astra只复核此新增提取／retained分支，无具体finding，旧537 provider和其余矩阵复用。PG `0ec1cf1e…c88415`，attempt1792966517339153800；Native41／XR／sim继续冻结。身体端Astra预算审阅提示PG-only未必足够，若下一轮仍commits0再用原C0阶段时间定位实际成本，不根据publication间隔臆造Native热点或放宽40／50ms。

## 2026-10-26 — EG定位post-source过期，PG复用同次真实观察并保留阶段记录

EG由身体端唯一实跑并正常清理，取Lancer成功、held首门仍失败。稳定Native41显示stale6全部broker source_after；projection／source_before／C0-current拒绝均0，post-admission拒绝另有9次。最后source failure_query10、mask1541，六源龄为46.0638／51.8660／52.1526／52.9531／52.9305／52.9508ms。末值raw callback12=source_mismatch，但last_query被后续unavailable归零，二者不视为同query关联。原50ms来源限制和40ms admission不变，106完整恢复及其余退出保护通过。稳定数据解码保存在ignored analysis/lancer-main-mount-ec-rejection-20261026ef/eg-native41-diagnostics.json。

PG源码进一步确认wire source_mismatch只直接来自sender::context或最终sender::response；retained bind失败会翻译为inactive，而非12。新增first32 claimed callback的有界POD阶段／实际返回／原时标记录，包含query／parent cookie／Native incarnation／lifecycle，区分context、retained细分阶段和response。GT不做日志IO、分配或额外游戏调用，原entered／finally／callback计数不变；仅原PGShutdown完成parent quiescent、停准入及PG drain后输出，Initialize在同样已停条件下重置。无新增Native导出／V1布局／源发布或写入权限。

只在retained链减少一次重复初读：原Client.copy_frame_on_gt丢弃同次真实Game，prepare随后又source_game_on_gt；现改用原双输出overload并同步传入私有prepare_observed。preview保留原初读，helper入口仍检查GT／domain／armed／同cookie注册存活；末尾独立source_game和完整same_game、两次typed资产读取、附着、hold generation、最终eligibility及retained bind全部保留。没有跨帧缓存、假Frame或续写时间；公共Client／wire／source／candidate／rig binding头的原摘要未变。原b58524c四文件中仅实际adapter inl因这次具体实跑问题作上述窄修改。

Sol／Medium新增直接包含实际adapter的owned-seam检查；前两次严格构建的测试夹具类型／命名错误已修正并原样留档，没有放宽生产检查。最终完整PG严格构建通过，761份源码／vendor闭包稳定；实际provider537项、实际adapter284项通过。Astra／High仅审本次增量，无P1／P2，发现正常publish失败被误记成未正常返回的P3，已让其经过共同诊断epilogue，并用owned VirtualProtect失败路径回归关闭。新PG `bed19e2b…2b2a9c`，构建attempt1792965623362730100；Native41／XR／sim不重建、旧PG矩阵复用。减少观察和离线通过不是已测性能改善或实际held验收，下一轮仍比较同一动作。

## 2026-10-26 — EG保留原动作，仅绑定Native41被动阶段诊断

EC失败后按身体会话交接，先从冻结EC派生ignored analysis/equipment-native-rgrip-runtime-20261026eg，并用实际runtime-slot执行门禁止未完成选择时启动；准确Native41交付后再解除该私有staging状态。最终Native `2d8de020…2a69`，PG仍 `a4657e0d…44472`、BQ XR仍 `1a059d2d…2b4e9`、CY仍 `378e785f…14894`。没有全抄native-only manifest中的旧XR：只替换Native字段／provenance及实际Native PE元数据，43导出同集合。原coordinator `ce4e0091…f4773`、host、PG、106验证器和全部动作／时效／背景保护同字节复用，不重跑旧PG矩阵。

现有host动态收集body_main_mount_*，无需改代码即可保存新增26字段：projection／broker／C0拒绝、post-admission拒绝、query branch、callback raw status、来源失败mask与六种原时标源龄。统计末值不视为逐query一致trace，不把不同query的raw status和last failure强行关联。另备独立有界只读解码器，校验保存的session-status与精确Game／Native incarnation；旧Native40无字段时明确available=false，不伪造零。

最终EG pins `f19afb69…e1bd6`，选择回执及NATIVE41-README已交身体会话；身体端确认依赖及新鲜CIM后接管唯一实际运行。Root没有并发Game/XR、没有修改EC原失败输入／结果或PG／Native源码。此条是被动诊断选择交付，不是拒绝原因或修复验收；等待该轮稳定实跑字段决定具体代码修正。

## 2026-10-26 — EC实际取Lancer成功，主握首门拒绝，106正常恢复通过

身体会话独占执行EC revision2后已正常释放窗口。实际右肩pulse3、生产held回执及独立Live.held附着认证通过；38／38一致postFK骨骼采样可读。shoulder-fixed-first32首门超时，Native mount ready0／commits0／stale14／last_query15／max15.2285ms，无fault或context_rejected；v3 held记录0。未到降枪／小移转，不能宣称持枪跟随完成。

HandStop4→5、cookie0后自由双手继续、原两手恢复、bridge close及normal stop通过。身体端一次finalize的accepted=true仅为cleanup：独立106完整范围恢复、Native／entry、XR0／PG0、原十文件、音频、ownedPID及fresh CIM通过；宿主瞬时no_remaining_games=false原值保留，不覆盖成true。Root没有并发Game或改动本次EC／Native／PG。

Root按交接只读保存PG证据和真实生产路径：普通tick preview的held行已认证Lancer资产及右手附着，body_unavailable来自其故意提供空BodyBinding；该行不是Native supplemental callback，不能据此定位失败。实际provider的context→retained(copy／bind／完整资产检查／final bind)→response阶段没有持久日志，scratch清理后无法用这次PG文件区分具体拒绝。Native stale是汇总分类，15.2ms也不是完整C0耗时。证据／输入摘要保存于ignored analysis/lancer-main-mount-ec-rejection-20261026ef/evidence.json；身体会话查Native首拒分支，Root暂不重跑旧矩阵、不放宽鲜度或预算，等具体条件后再修。

## 2026-10-26 — Lancer主握联合用例EC离线交付，运行窗口归身体会话

按身体会话及统筹要求，从已通过EB复用私有宿主、实际RGrip边沿／装备回执、独立Source1读取器及保留pending导出的API，只准备ignored `analysis/equipment-native-rgrip-runtime-20261026ec/` 联调胶水。Native40 `f146c542…de8ce`、PG3715853 `a4657e0d…44472`、BQ XR `1a059d2d…2b4e9`、CY simulator `378e785f…14894` 均绑定准确成品；采用身体端已修正XR选择的联合manifest629c92。没有修改或重建身体／渲染／XR／模拟器／PG源码，没有由Root启动Game或XR。

Body19实际INI配置保留FP、C0 hands、arm-control ownership、CCTRL／phase、Fresh／Atom／Goal，并启用held_main_mount及selected-only诊断。动作限定为左手可达基线、右肩pulse3取Lancer和独立Live.held认证；肩部固定输入必须再取得至少40个新Native held-selector joint commits及一致腕部采样，才降枪并小移2cm／转10度（明确转换为弧度）。总输入上限八条包含最后双手原状态恢复。HandStop后另取quiescent基线，要求自由双手继续提交、not-ready fallback增长且held commits不再增长；未知pending输入／导出不覆盖或重放。R2保持OFF，无额外枪械RPC。原45秒协调窗口、300秒宿主、.4／behind／observe-only／测试静音恢复／physical-background OFF／十文件保护不变。

恢复核对发现EB验证器已有Control8／Fresh3分支，只是Native29未启用；因此保留其无条件固定Pose8 union，仅从实际Body19增加Atom7完整范围分支，避免直接覆盖丢失旧保护。独立验证器SHA `6c88c33c…a2a2`，finalizer进一步要求固定26范围的source／target-RVA／完整长度／固定摘要全部匹配、总106及原Native／entry／XR0／PG0／音频／十文件／PID清理条件。Body19旧provenance摘要与当前源不符的事实保留；采用实际源摘要，不据旧清单宣称覆盖。正常停止后有界保存v3 first32原始二进制并检查header72／record2120／Game PID／Native incarnation；腕部／枪体数值比较仍由身体会话执行。

离线host-preflight通过33份编译、12个私有namespace入口、4项原API检查；完整范围验证实际函数54项完整读取／截断／尾损检查通过；实际phase的Native-only／Pose-only／错误marker负例、固定范围覆盖负例、双手旋转恢复等11项及原watch finally6项通过。有界Sol／Medium仅审这次Python胶水，无其他具体finding。最终pins `2bd3cef6…9ad4`、coordinator `9e7e8937…74aa`，准确路径、运行步骤和边界见EC/JOINT-README.md，已交身体会话唯一运行。此处不新增联合游戏、可见枪体跟随、权威射线、散布／后坐力或头显验收，不据胶水交付增加成熟度。

交付后离线复核修正一个具体误拒风险：原左右控制器z=-.4可能不可达，先恢复再检查自由双手提交会将正常交还判失败。EC revision2仅调整停止顺序：在当前可达姿态HandStop，等待真实cookie0状态后取计数基线，验证自由双手继续提交和held计数不增，再用原预留两条命令恢复手柄并close／normal stop。实际finally AST的正常路径、诊断读取失败仍Stop／restore／close、未知导出不重放／不close三例通过。最终pins更新为 `71c4fdf0…0233b`、coordinator `ce4e0091…f4773`，先前seal／源／结果原样留存；总输入／安全门／Native均不变，修正前后均未启动EC游戏。

同时更新历史瞄准链文档，区分EB真实R2闭环与尚缺的逐发数值链：主握Response只是腕部标定，不能直接补齐完整枪口Pose、逐发Grip或射击写入权限。后续从现有CallFunction所有者关联实际Lancer原调用的基础瞄准／误差后方向与起点／终点／碰撞，不额外调用随机瞄准函数。

## 2026-10-26 — 主握附加query的PG入口／映射／父注册接线完成，待Native联调

身体会话已正式冻结weapon_main_mount_provider.h SHA `3f20163b…2f2cc`，Root按同字节复制；实际x64布局RigFrame1824、Body112、Request1968（body偏1856）、Response504、Registration48。附加注册复用同一Hand parent cookie／Native incarnation／context和已pin模块，只有新的Native attach导出，不增加PG导出或独立detach。Body独自负责Native broker、C0消费和写入，V1及原51／52增量不改。首轮R2 mode OFF。四份b58524c主握发送源保持原hash。

新增默认OFF的GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL，要求既有Native装备／主握准备。HandStart在创建parent前验证新Native导出属于该已持有模块；parent和既有CF attach成功后才做child attach，精确校验回执cookie／incarnation并在返回前再次确认registration_alive。限定协议测试发现中途失联窗口后已补该末尾检查；任何partial attach错误都走原begin_disconnect，保留父注册等正常Stop，而非提前卸载。

PG回调所有入口计入原callbacks，使用SEH __finally关闭计数、清空持久工作区和释放原子重入标志。新增Wire Request／Response／Candidate／Diagnostic均在独占持久工作区；只允许一个已认领调用使用，重入返回非法status交Native fault路径，不清掉外层数据。legacy parser只有原flag存在时才临时禁用并在finally恢复，不编入第二writer。回调不做Engine调用、日志、Native重入、等待或GT LifecycleScope。既有有界Rig／Hand／metadata局部快照按双方确认继续保留，不扩改Reader或冻结四源。

纯mapper把Native已认证的保留Scene／body数据映射现有BodyContext，调用真实prepare_retained，再逐字段返回实际Candidate。独立检查query id／hand lifecycle／parent／Scene／完整对象／hold generation／geometry／scale／units／basis／revision与固定Lancer calibration。进入与返回都用原始Rig／stereo／controller sync时戳检查50ms，不靠重新读取或helper时间刷新来源；失败外部Response为空且仅保留size/version，ready只返回标定数据，不返回writer lease、wrist世界目标、aim或support权限。Native仍须按它的parent revision／operation／revocation及40/50ms窗口做返回端复核。

299项纯字段映射／时效与owned fixture检查、289项实际provider.inl回调／attach协议检查通过，后者覆盖wrong context／GT／disabled／domain、soft/hard revoke、真实重入、SEH finally恢复和父回执错误，不冒充真实asset read或Native admission。原装备受影响检查及完整PG严格编译通过，760份源码／vendor稳定，PG `a4657e0d…44472`。第一次完整PG因parser命名空间仅在旧flag中存在而失败，已按原ifdef修正并保留失败源／回执；协议fixture单独打开parser stub分支，不启用实际旧writer。关闭新provider guard后，两处接入文件与0b0aaf6旧路径文本机械完全一致，非二进制等价或Game声明。

Astra／High仅审阅本次新callback／映射／父注册危险边界，无具体P1/P2；依赖已确认的Native锁外调用／module pin／统一inflight/drain合同，未重审旧R2或把CPU测试视为C0验收。Native／XR未由Root重建，实际query调用、枪体／手腕跟随、联调清理和头显仍待。源码及验证在Root分支，ignored回执为analysis/lancer-native-r2-20261026dw/pg-build/attempt-1792960173173668200/与provider-off-equivalence.json。无默认替换、发布或额外身体writer。

## 2026-10-26 — EB通过严格停止交还，Lancer扳机短闭环收口并交接主握query

EB使用a09767e／PG `a3087ecb…24707`及收紧后的协调器，实际首次R2按下、原版key down、主动松开key up、356→347和两个新347／up通过；第二owned press后仍owned1，随后明确在mode2／source9执行status4、game_r2=1→owned0，没有running source-loss提前替代。runtime-review.complete／behavior_complete和独立r2-behavior-audit均true，errors为空，无pending export。本短动作内装备input_gap和R2来源拒绝日志均0；这不是全程或物理头显稳定性证明，仍原版瞄准，未测逐发数／命中／枪管射线／伤害。

EB记录了copy与后续读取分段：示例两份后段有效输入的input_elapsed分别6.5542／6.5237ms，含copy；不能用game_elapsed约3ms直接对比旧版约6ms来宣称一半总耗时。额外的第二press action仍17.889ms，后续优化不得把它忽略。当前只确认这次原版输入闭环及严格停止语义通过，不作FPS收益结论。

EB控制器恢复、HandStop／quiescent／bridge close、88项独立恢复、Native／entry、XR0／PG0、原十文件、音频10/10、准确自有PID和fresh CIM无Game／XR全部通过，已交还运行窗口。证据为ignored analysis/equipment-native-rgrip-runtime-20261026eb/runtime-review.json、runtime-final.json、r2-behavior-audit.json；pins `bf6d0583…9db46`，原DW／DY失败、EA假绿判读及DX既有通过均保留，没有重跑旧成功用例。

当前下一主交付转为实际Lancer主握。身体会话已给出B18连续双腕和B19限定ownership fallthrough／reacquire通过范围，正在实现同parent cookie／ModulePin／统一inflight／drain的可选附加query注册；正式合同确认后Root仅接PG端，Body唯一负责Native消费／写入。主握发送端四源仍b58524c未改，未导出prepare_retained不能直接调用；query首轮R2 mode OFF，失败退stock mount，V1 Frame和51／52原增量保持。最新Client双输出仅PG C++访问器，不改变query lease／bridge admission，旧单输出行为经回归保持。无默认替换／新发布／头显验收。

## 2026-10-26 — 独立判读纠正EA假绿，Client同次返回真实Frame／Game

EA实际首次按下／主动松开及356→347通过，第二press也得到owned；但frame350在running mode1／source_status4先交还，后面的stopping mode2只有accepted、game_r2=0。协调器原complete／behavior_complete虽为true，未证明“停止时交还仍自有的射击输入”，不能作为完整目标通过。原始review／final不改，新增独立r2-behavior-audit.json：以实际日志的第二owned press为界，要求后续mode2／source9／status4／game_r2=1→owned0，且中间没有running hand-back。直接复用DW／DX／DY／EA四局原件哈希，独立目标判定依次false／true／false／false；DX真实停止交还保留，不重跑。四局清理均通过。

EA的入口去重复观察后仍有装备前序sync年龄111ms和R2前序100.3851ms，后一项触发正确安全释放。不能把game_elapsed的局部下降推为整体性能或稳定性修复。EA正常88项独立恢复、Native／entry、XR0／PG0、原十文件、音频10/10、准确PID及fresh CIM无Game／XR后交还窗口；故障和语义缺口均保留。

下一窄增量为PG C++ HandClient.copy_frame_on_gt双输出重载，返回同一成功调用的局部current Game，而非committed_game缓存。旧单输出入口委托同一实现；两个输出先清空，所有原GT／gate／key／epoch／publication／前后freshness／same_game检查均保持，全部成功后才一起输出。没有类字段、Registration／Frame、Native ABI或writer改变。R2 tick立即同栈使用这份Frame＋Game，期间只有controller admission／clock等无引擎步骤；使用点检查armed以拒worker soft invalidation，registration_alive不能替代。typed R2两次读取／对象结构复核和最终独立observe／same_game仍在；source已确认后的元数据失败保持unknown分类，不转loss-fallback重试。最终观察成功后再检查armed。execute／ammo／安全release均不接这份seed，继续原独立元数据路径。

复用原Client fixture覆盖双输出的全部Frame-copy失效／过期／重入／disconnect竞争，两输出失败清空、成功一次observe、旧单输出一次observe，以及copy后soft invalidation仍保registration但armed=false。15既有＋30 Frame-copy＋17 source-current＋8 Game-output案例通过，原84 cases／900项Runtime、69 cases／733项Input及420项R2检查、完整PG严格编译通过；755源码／vendor稳定。PG `a3087ecb…24707`，Client头SHA `d41e2e98…db996`。新增copy_elapsed_ns／input_elapsed_ns用于明确包含copy的计时范围，不能只比较被缩小的game_elapsed。设计问题仅由已有Astra限定回答，没有额外全量审计或新测试体系。

EB沿用原host／Native29／XR／sim／reader和原动作，协调器已收紧上述owned stopping语义；实际函数用DX正例、EA反例及去掉停止行的DX反例通过，全部清理和来源门不变。候选pins `bf6d0583…9db46`，实跑待窗口。身体端B18连续双腕与B19限定交还／重新跟踪已给出通过范围，双方开始独立主握query交接：Root发送端仍b58524c原四文件未改，Native端由身体会话唯一负责；已给精确路径／哈希及DX比较PG，强调现有prepare_retained未导出、同cookie admission／inflight／drain尚需正式合同，module pin不构成PG调用准入。无默认替换／发布／头显验收。

## 2026-10-26 — DY否定单纯消费顺序交换，撤回该候选并消除重复鲜Game观察

下节记录的调度尝试已实跑DY，行为失败，不作为完成的修复保留。首次R2 press成功后，下一Frame的前序sync年龄达到109.3368ms；保护正确执行原版release，物理模拟扳机仍down时正确不重放。仅356→355，主动松开步骤等不到对应owned release，第二press及持按停止未触达。装备gap减少不能补偿R2失效，整轮complete=false保留。DY的disable0、无pending、控制器恢复、HandStop／quiescent、88项独立恢复、Native／entry、XR0／PG0、原十文件、音频10/10及准确PID／fresh CIM正常；正常清理后释放窗口。证据为ignored analysis/equipment-native-rgrip-runtime-20261026dy/。

已按实际DY构建manifest的逐文件hash保存六份候选源到analysis/lancer-native-r2-20261026dw/rejected-dy-source/，然后把Runtime／PG调度／backend／相关测试全部恢复e70174f。保留DX通过的原始先R2顺序；不把Astra静态无finding或CPU回归通过当作运行顺序验收。

下一窄增量仅在weapon_lancer_r2_native.inl：原tick的current调用Client.source_current_on_gt，而它已调用完整fresh observe，随后read_game又观察before／after，扔掉第一份有效Game。改用同一Client.source_game_on_gt的真实输出作为before，保留原精确key／epoch／注册存活／前后源龄校验，typed R2两次读取、全对象／结构复核、最终独立observe和same_game比较仍在。source_current输出只在原来源证明确认后置true；后续typed元数据失败保持原unknown分类，不转成source-loss释放重试。null-source的独立安全交还路径不变，不缓存跨帧Game、不刷新Frame时间、不改100ms。另保留ammo_queried的有界同值诊断日志。

首版严格PG及原受影响快速检查通过，755份源码／vendor稳定，PG `2739de07…be6ff`；Runtime／Input已恢复原用例，420项R2检查仍过。但Astra窄审查发现P2：若对所有read_game(source)都新增Frame freshness，tick见证后元数据耗时可使源过期，随后owned release会因这项新增条件被拒。DZ已准备但从未执行，保留原pins与二进制并标记DO-NOT-EXECUTE。

修正为只有tick传source_current输出指针时才请求并复用真实来源见证；execute／ammo／装备交接所有旧调用均不传该指针，保持原XR-independent fresh observe路径，即使release收到先前的Frame也不新增source限制。Astra仅复核该分支与调用点，明确关闭P2，无新具体问题。最终完整PG及原受影响检查通过，PG `6aedb359…c8f83`。EA从已通过DX复用原host／协调器／全部动作和安全门，仅换精确PG；原Native29／XR／sim／reader均未变。当前仍待实跑，不将预期减少一次观察当作已测性能改善。共用身体／主握接口继续冻结，无默认替换或发布。

## 2026-10-26 — Lancer原版R2短闭环通过，定位并调整抓握／射击消费顺序

DX沿用DW的实际host／Native29sim／XR／simulator／reader，只选择PG `cb187cdf…6e18e`及诊断后新鲜UP观察门。实际首次扣下／独立原版key down／松开key up通过，原版总弹药356→348，两个新post-release读数均348／up；随后第二次DOWN得到owned press，保持物理模拟trigger down时Disable0，独立原版PC key已up。runtime-review.complete和behavior_complete均true，errors为空、无pending export。仍沿用原版瞄准，耗弹8不是逐发或命中数；主握／枪管权威射线／伤害／头显未验。

DX控制器恢复、HandStop／quiescent／bridge close、88项独立恢复、Native／entry、XR0／PG0、原十文件、音频10/10及准确PID／新鲜CIM无Game／XR均通过，窗口已交还身体会话。证据为ignored analysis/equipment-native-rgrip-runtime-20261026dx/runtime-review.json与runtime-final.json，pins `291cda1b…79b5`。DW原失败及独立scope说明保留。新增R2代码与诊断阶段已提交e70174f，保持默认关闭。

DX仍有两次装备previous_sync_stale。精确配对相同Frame／当前sync／前序sync后，R2消费时前序年龄91.3954／98.9844ms均合格，到后续装备输入消费已100.7385／106.8725ms，晚了9.3431／7.8881ms。记录到的R2游戏读取8.3–13.4ms、两次press约16.1／17.2ms、已记录的弹药查询约11ms；这些是稀疏状态变化样本，不是完整帧分析，第二次同值ammo查询在该版未输出耗时。不能把所有间隔或DW失败归结为ammo查询。配对与证据哈希保存于DX/timing-review.json。

据此新增仅PG-local的同步调度点：稳定held的RGrip仍在Runtime内先采样，再调用R2消费者，随后复核原始Frame current并执行原库存／身份／附着／动作年龄检查。三参数Runtime::tick走原空消费者，不增加额外current；四参数只借用同栈时机，不授引擎／writer权限或保留Frame。装备在派发前早退时，event_hook必须补调R2一次，确保真正源丢失仍能交还owned press；已派发后任何早退均不重复。原切枪前yield保持。另补ammo_queried的有界日志，避免同值第二样本漏记耗时。

新增真实Input＋Runtime顺序回归：相同90ms前序样本，旧顺序先做20ms其他工作会拒绝；先采样再同步工作可正常切换，但工作61ms使原Frame过期仍必须拒绝且不跨帧重放。覆盖consumer后current失效、suspend、重入、baseline顺序与派发前早退。原受影响测试与420项R2测试、完整PG严格编译通过，72 cases／764项Input检查，755份源码与vendor闭包稳定。Astra／High仅审阅该五文件调度增量，无具体P1／P2；未重审旧控制器或重跑Game。最终PG `f8ca11ee…e86a6`。DY原host／动作及所有安全门不变，已准备等待身体B18运行窗口；此处还不能声称调整后的实际连续输入稳定性通过。

## 2026-10-26 — Lancer接入Native帧的原版R2，首次扣弹与松开通过

新增默认关闭的PG私有Native Lancer R2消费者，直接使用当前注册的真实Hand Frame；旧Hammerburst bridge保留。仅支持已认证、实际右手附着且已握持的Lancer，经原版typed ButtonPress／ButtonRelease(Name R2)输入，不改枪口、瞄准、伤害、Native ABI或骨骼。切枪前必须交还自有按下；跟踪失效后不重放旧边沿。完整五对象身份不符或原版释放失败时保留未交还状态，不重置掩盖。私有控制沿用旧R2三个导出，生命周期与现有PG callback drain结合。

有界Sol／Medium实现纯策略测试，Astra／High仅审查危险的输入所有权及退出增量。审查发现其他PlayerController和不适合处理输入的嵌套Tick原会被误记成源丢失，已改为在消费前跳过，并覆盖原owner仍能释放、真实新owner不能接管旧债。两个早期编译的纯测试失败来自fixture未递增release计数，原件保留；只修fixture，没有放宽生产输入结构。最终DW构建PG `6d8fea67…ea809`、真实wrapper与原ledger的380 checks及原受影响装备检查通过，755份源码和vendor闭包稳定，Native／XR未重建。

DW购买版实际右肩Grip取出Lancer，首次R2按下／松开和独立原版bHoldingButton_R2读回通过；GetTotalAmmoInGun从356到347，松开后两份新样本均347且key up。耗弹9不是逐发计数，也未测命中或伤害。第二次DOWN落在输入重建基线时，未得到owned press；整轮complete=false保持，持按停止未触达。原回执scope沿用DU的认证／Snub文字且shots=0不是实测计数，已另写绑定原review／final哈希的behavior-scope-addendum.json，保留原件，不为文案重跑。首次耗弹仅证明原版瞄准下输入链路，不是枪管权威射线或6DoF握枪验收。

DW的R2 disable0且无pending export、控制器恢复、HandStop／quiescent、Native／entry／88项独立恢复、XR0／PG0、原十文件和音频10/10通过，准确自有PID与新鲜CIM无Game／XR，窗口交还身体会话。ignored证据为analysis/equipment-native-rgrip-runtime-20261026dw/。runtime-final.accepted指恢复闭包通过，不能覆盖runtime-review.complete=false。

针对第二次DOWN，先加真实wrapper回放：previous-sync gap后即使当前帧新鲜也必须丢弃旧DOWN，直至真实UP后新按下，source0／1均覆盖。再补有界source状态变化日志和game／action／ammo各阶段实际耗时；弹药查询后报告真正推进的新鲜UP，供测试协调器等待后发送第二DOWN，不改100ms限制、账本或引擎权限。420 checks与完整PG编译通过，PG `cb187cdf…6e18e`，源码闭包稳定。DX仅派生协调器的查询后观察门与修正scope，复用原host／Native／XR／sim／reader；原全预检和已审cleanup门复用，变化的真实lambda与cleanup门离线检查通过。DX实跑和耗时原因定位仍待运行窗口；当前不把查询耗时假设当根因。身体端唯一writer及主握发送端b58524c保持冻结，无默认替换／发布／tag。

## 2026-10-26 — Lancer主握发送端绑定同一Scene，补齐身体来源适配

继续Lancer实际主握接线。先核对B14实际Native37sim的冻结源，而非通用tools目录：Scene中的WeaponHand prepare／commit尝试先于Rig发布；稍后的C0保留那份Scene来源，body::plan之后才做hands::compose／Kernel.prepare。因此后继同步查询可使用那份Scene已成功commit的Root Frame，但Rig有效本身不能证明武器commit成功。C0／Kernel调用编号和publication_sequence不能代替hand.FrameKey.frame。五份实际冻结源、source37-before-build清单与37sim DLL哈希已独立核对，静态来源回执为ignored analysis/lancer-main-source-20261026dv/source-order-review.json；不将通用源码里未发现live Kernel的早期检查扩张到实际37，也不声明38时序已验。

修正Root的weapon_rig_binding：原来匹配Native身份、全对象、原Stereo／GVC和reference，但漏了显式hand.key.frame==rig.scene_invocation。新增scene_mismatch拒绝，保留原状态枚举序号。旧DP真实ready记录两编号本来一致；新增回归特意保持所有输入样本／时间不变、只改Scene，确认仍拒绝，publication序号则可独立推进。测试fixture也修正为同一个Scene，而不让原错误的独立常量继续掩盖漏检。

新增PG-local BodyContext／bind，要求Native键、publication／Scene／world／binding／reference五来源代数、完整Mesh、Rig单位、实际几何／scale、basis2／revision2均匹配，再从真实已commit Hand Frame复制BodyBinding.key；不构造C0 Frame、不写任何pose或授权。Native仍负责真实C0几何认证与使用前后有效性检查，known布尔值不是认证证明。prepare_retained入口先冻结调用者自有Rig／BodyContext，调用原Client.copy_frame_on_gt取得已提交帧，再绑定、执行原完整held／asset／attachment准备、最后复核来源时效。所有拒绝清空候选，不保留跨帧结果。

该入口仍是PG本地函数，没有导出或新登记，不改变Native／Hand V1。当前调用要求已有PG callback lifetime lease；后继Native调用必须由身体唯一负责人纳入既有broker的准入、同cookie在途计数和退出／quiescent流程，不能仅靠模块pin调用。实际消费位置应在同C0 body plan之后、右手目标组成之前；51／52原动画增量继续由唯一writer保留，不能额外给标定加90°。本轮未修改身体会话源码或安装另一个writer。

严格完整PG编译以及受影响的453项Rig绑定／328项Body来源绑定检查通过，752份源与vendor闭包稳定；后者使用合成数据，覆盖各来源／几何／基准拒绝、清空旧输出，以及绑定后进入现有Lancer候选策略，不等于实际Native几何或回调生命周期验收。一次有界Sol/Medium复核Root新增适配入口，无具体finding。PG c89070b7…174dd，回执为ignored analysis/lancer-main-source-20261026dv/pg-build/attempt-1792950587535241200/receipt.json。未重建Native／XR，旧非相关检查复用。

DV购买版普通场景复用DP原host／Native29sim／XR，实际右肩Grip→Lancer held后取得4个不同publication的ready记录（原协调器门为4；DP上一局观察到5并不等于门槛为5）。四条均frame==scene，完整绑定仍由实际函数核验；源龄50.4155–66.1822ms、发布龄35.3064–50.6330ms，units100。complete／behavior_complete true、无错误，仅证明强化后的持枪准备姿态读取通过，不是37的C0写入测试。额外主握预览认证Lancer一次12.6ms，但body_binding／candidate始终0，prepare_retained尚未被Native调用。证据为ignored analysis/equipment-native-rgrip-runtime-20261026dv/runtime-review.json，pins470bcef8…72ba4。

DV控制器恢复、HandStop／quiescent、88项独立恢复、Native／entry／XR exit0／PG shutdown0、原十文件保持、音频10/10与准确自有PID退出通过，随后新鲜CIM无Game／XR，窗口已交还身体会话。当前下一可玩检查点仍为唯一C0执行点上的Lancer跟手／瞄准／射击；Native查询准入和实际腕部覆盖修复仍待双方接通，不把本地适配／准备数据当作真实手腕或枪体运动。无新发布／tag或默认替换。

## 2026-10-26 — 定位持枪后输入消费过晚，提前同tick读取Grip

在995fd2f之后继续追踪DQ／DR的第二次Grip失败。旧记录只有sync／received，没有实际Input::next消费时刻，无法区分两处source_gap。新增纯诊断Gap细分、observed_ns与previous_observed_ns，在真实next拒绝位置最多记录16条，同时保留原始Frame.now、sync、received和sequence；没有替换时间公式或扩大100ms阈值。严格PG编译和原Runtime／实际Input→Runtime定向回归通过，复用原Native29sim／XR／host执行DS同两个动作。

DS实际取得两条previous_sync_stale：当前样本源龄44.4435／45.8568ms，而前同步点到消费时刻为122.224／101.3919ms；消费相对该Frame.now延迟37.5691／43.2679ms。两条并非当前样本源龄超过100ms，也不是源次序倒退。DS第二次切枪仍失败，complete=false；这次取证把具体拒绝分支和实际消费时刻补齐，不把调查进展写成玩法完成。原日志与机械运算在ignored analysis/equipment-native-rgrip-runtime-20261026ds/runtime-input-gap-review.json。正常88项恢复、Native／entry／XR0／PG0、原十文件、音频12/12和随后新鲜CIM无Game／XR通过，运行窗口已正常释放。

源级核对确认：baseline在库存前读取输入，已持枪路径却先做库存、身份、附件和多次完整observe，再读取同一Frame输入。修正只在稳定held且没有pending／future时，沿baseline附近的现有边界读取一次，并立即重核current；结果只存于本tick栈上。原位置使用同一结果和原cursor，不二次读取或重放。后续库存、持枪身份、附件、当前Frame源龄、event tuple／age和动作前检查全部保留；任何提前返回都丢弃局部事件，原pending／future路径顺序不变。不会用旧结果授权下一帧。

新回归使用真实Input→Runtime→模型，模拟40ms当前源龄、50ms同步推进和28ms后续库存／附件耗时：旧Runtime在同一新增时序断言实际失败，新版通过；库存失败、附件失败和源过期都不切枪，恢复后无重放，新按压才执行。严格PG编译、原Runtime与69例733项输入检查通过；一次Sol/Medium仅复核该顺序差异，无具体finding。新PG为2eb6b5fc…a0bfa，750份源／vendor闭包稳定，未重建Native／XR或修改默认启动器。回执为ignored analysis/equipment-input-early-20261026dt/pg-build/attempt-1792949065954512600/receipt.json；旧Runtime的预期失败保存在同区old-runtime-check-1792949119833556300/receipt.json。

DT真实同两动作运行中，没有记录到source_gap或baseline重建，Lancer认证1条／11.9059ms；第二次按压仍未切枪，complete=false保留。进一步核对发现私有脚本从DQ继承了错误裤兜坐标：vertical=-.72／depth=0，不是原DG已通过的-.55／.02。真实按压位置约比中心低17cm，距离约17.1cm，超过生产半径16cm。将DT记录的实际head／last-press grip送进实际编译的body_slot函数，在受控中立reference下返回none；原DG位置返回pistol。回执为ignored analysis/equipment-input-early-20261026dt/slot-check-1792949450884873600/receipt.json。这是独立测试输入错误，不能把先前所有失败都归因于消费延迟，不能扩大生产槽位来适配错误坐标。

DT正常88项恢复、Native／entry／XR0／PG0、原十文件、音频12/12及随后新鲜CIM为空通过。DU仅恢复私有脚本的DG裤兜坐标，PG／Native／XR／两动作目标和所有时效守卫不变；预检首次因私有coordinator在artifacts／extraArtifacts中的两份旧hash未同步而拒绝，未启动游戏，保留失败后只更新这两个精确派生工件条目，完整预检通过。历史DQ／DR／DS／DT和各自封存输入不改写。

DU随后真实完成两个检查点，complete／behavior_complete均true、两段episode、errors为空：frame470右肩slot3取得Lancer held及实际资产／唯一右手附件认证，耗时14.0466ms；frame505左裤兜slot0实际切到Snub且held，hold generation从1变2，资产准备返回unsupported_asset／unsupported_weapon（status3），未延续Lancer认证。所有记录candidate／body_binding仍0。输入顺序修正没有被用来放宽保护：本局仍记录3次previous_sync_stale，源龄分别29.3905／46.5215／24.7161ms，前同步点累计107.1264／113.1679／113.5128ms；恢复基线后完成了目标。不能将本局通过扩张成所有卡顿情况下输入均稳定，也不能单凭前后对照声称消费延迟是DQ／DR全部失败的唯一原因。

DU正常停止完成控制器恢复、HandStop4→5／quiescent、88项独立恢复、Native／entry／XR exit0／PG shutdown0、原十文件保持、音频12/12和准确自有PID退出，随后新鲜CIM无Game／XR，窗口已交还。行为、拒绝实值和清理分别在ignored analysis/equipment-native-rgrip-runtime-20261026du/runtime-review.json、runtime-input-gap-review.json、runtime-final.json；pins为3eeb3b10…e5779。下一检查点回到共用C0上的Lancer实际主握／瞄准／射击；当前只完成输入消费修正与切枪后的资产认证失效验证，实际腕部、权威弹道及头显验收仍未完成，无新发布／tag／默认构建替换。

## 2026-10-26 — Lancer实际资产与主握候选认证，诊断预览减负

本轮保持单一目标：在正常GT装备消费者内，按当前真实持枪状态认证Lancer资产，为共用身体执行点准备固定Grip→Wrist标定。新增默认关闭的NativeEquipmentMainMountTrial，不新增Native登记／导出／共享ABI或骨骼／组件writer。现有source_current_on_gt提取为可返回当前GripGameState的source_game_on_gt，保留原GT、登记、精确FrameKey、身份、源龄与调用末尾检查；失败清空输出，不发布新Frame或产生写入权限。

实际资产读取要求当前Weap_AssaultRifle、Instigator／Pawn链接、类型正确且可见的枪Mesh与COGAssaultRifle SkeletalMesh；重复核对属性选择、对象身份和链接后才返回。沿既有唯一b_MF_Weapon_R_End附件验证，附加预期枪Mesh一致性；准备函数在前后重新读取当前游戏状态、hold generation、资产和登记／时间。候选还必须收到身体唯一负责人认证的同帧几何、modelScale／units和marcus115-ref-eyes-face-forward-y-authored-v2基准，才能输出固定右腕Calibration。无身体绑定时输出为空，不假定默认Marcus／比例，不设置WeaponMount.valid，不改变51／52子链或后坐力。身体基准的面向修正不等于骨骼局部轴改变，不能再给固定腕标定叠加90°。

DQ购买版普通场景实际完成右肩RGrip→Lancer held，取得6条真实资产与右手附件认证；准备耗时12.9455–14.9817ms。由于预览故意未提供身体绑定，6条均为body_unavailable／candidate=0。随后左裤兜RGrip→Snub未完成，complete=false与source_gap／baseline重建原记录保留。原输入最大间隔为100ms，既检查连续sync间隔，也检查当前时间距上次sync；不能只凭相邻sync约55–66ms排除拒绝。每帧重复深读确实增加负担，但尚未证明它是本次切枪失败的唯一原因。证据为ignored analysis/equipment-native-rgrip-runtime-20261026dq/runtime-review.json。

DQ正常停止、控制器恢复、HandStop4→5／quiescent、88项独立恢复（含八完整Pose chunks）、Native／entry／XR exit0／PG shutdown0、原十文件保持及音频12/12恢复通过，随后新鲜CIM无Game／XR。原host退出瞬间的no_remaining_games=false保留，不改写历史。清理accepted=true仅限正常退出与恢复，不代表两个目标动作通过。

减负改动只限制诊断tick：按登记／世界／五对象身份／持枪代数／动画与切换状态变化重置有限预算，间隔至少100ms、最多8次，取得认证或确定拒绝后停止。预算不保存或复用任何认证结果、几何、Calibration或pose；实际prepare每次仍执行完整新鲜校验。受影响严格PG编译、资产读取42例、候选118项、真实Client当前状态回归及既有相关Runtime／Hand／Shot／CF／protocol检查通过。最早候选测试因合成fixture漏填眼部单位四元数失败，已只修fixture并保留原失败；没有降低生产检查。最新私有PG为574160cc…bf0cc，750份编译源及vendor闭包稳定，Native／XR未重建。回执为ignored analysis/lancer-main-mount-20261026dq/pg-build/attempt-1792947521251763900/receipt.json。

DR已按统筹建议完成同两个动作的对照，保持原输入时效、八命令上限和正常恢复。持有Lancer取得1条实际认证，单次15.0932ms，之后该持枪状态不再重复深读；candidate／body_binding均0。第二次切Snub仍在source_gap／baseline重建中超时，complete=false，没有取得实际Snub拒绝样本，因此旧Lancer认证不延续到手枪这一目标尚未通过。此对照不支持将原连续诊断负担当作唯一原因，也不证明它没有影响。证据为ignored analysis/equipment-native-rgrip-runtime-20261026dr/runtime-review.json，pins为7c207b3d…7461a。

DR正常finalizer accepted=true，88项独立恢复、Native／entry／XR exit0／PG shutdown0、原十文件、音频12/12和准确自有PID退出通过，随后新鲜CIM无Game／XR，运行窗口已交还身体会话。下一步离线核对实际输入消费顺序与两个source_gap时间边界，必要时只补有限实值日志，不继续重复同一动作累计、不放宽100ms。共享身体／腕部执行仍由身体会话负责；其B14证据已把六根手臂骨的变化缩小到原Compose调用内部，尚未修复实际腕部偏差，因此本轮不能算自由双手、枪体跟手、射击或真机交付。完整需求和默认启动器不变，未发布／打tag。

## 2026-10-25 — 普通装备消费者读通共用Rig，Lancer固定主握映射就绪

本轮继续Lancer握持接线，没有扩展其他武器或新增writer。新增默认关闭的NativeEquipmentRigReader，将现有hand_rig_client从旧独立IK测试profile解耦，沿同一个已保留Native登记读取GearsVrReadRigTrackingFrameV1；旧两参数读取保留原编译期来源，新消费者显式使用认证Frame的来源。五处attach／seal／quiescent／detach生命周期条件复用原Admission与Native pin，停止／重连围栏保持。数据不跨帧缓存，不产生scope、武器／手部所有权或写入权限；新的开关没有启用旧骨骼／组件writer，V1布局和导出数量未改变。

新绑定器同时要求Native手部Frame与Rig有效、原100ms内、同PID／GT／birth／Native started／cookie、world generation、五个完整对象身份、参考代数与姿态、原Stereo和完整GVC逐字段一致，并保留producer continuity要求。右Grip必需；对出现的左右Grip按实际final pre-HMD base／reference／units重算核验，不能把render-camera forward或HMD方向代作武器方向。读取前后仍核当前GT与原实际对象链接，记录时间再核绑定。世界Grip是准备后的跟踪坐标，不是实际已写手腕或枪矩阵。

DP购买版普通场景在实际右肩Grip→held后取得5个不同publication的ready记录，原输入源龄43.4153–60.3024ms、发布龄35.9553–49.3073ms，units=100；协调器complete／behavior_complete均true、无错误，仅表示本轮“持枪时读取匹配准备姿态”目标通过。使用PG `b000d7ca…64a3c`、原Native29sim／XR／reader，仅私有RigTracking enabled=1数据发布，Body／IK／component writers仍全0。Root当前与Native29冻结RigFrame／HandV1头哈希一致。本轮不是Native35双手写入、持续枪体跟手、权威射击或头显验收，实际记录在ignored `analysis/equipment-native-rgrip-runtime-20261025dp/runtime-review.json`。

已新增designer-authored `marcus115-lancer-held-grip-v1`／revision1固定映射，复用既有Marcus115几何签名、实际modelScale／units和同一掌心marker；held右腕改为UE F/R/U对齐，并相应换算Grip→Wrist位移，避免只转手腕而令掌心接触点移动。已查看既有LOD0侧面图，模型枪管沿mesh +Y、up +Z；原挂接约-90°gun-in-wrist方向关系仅用于数学核验，传给后继唯一writer的是Calibration，不是gun目标／额外组件旋转。该profile不证明物理校准、当前武器资产认证、枪口／权威弹道或support mount，不能仅凭它设置WeaponMount.valid。Native执行仍保留51／52原动画增量和子链。

严格完整PG编译及受影响检查通过；新Rig绑定429项和固定Lancer映射752项仅为合成CPU数据检查。实际hand_rig_client源码另以Windows线程／事件测试旧wrapper、显式来源拒绝、重入、读取期间seal／拒绝detach、登记变化清空输出和drain后detach，19项通过；provider为合成，不能称为真实Native pin并发试验。一次有界源级复核未发现新生命周期缺口。构建回执为ignored `analysis/lancer-rig-consumer-20261025do/pg-build/attempt-1792941739671401900/receipt.json`，Windows测试为同目录`reader-check-1792941532680971100/receipt.json`；两个私有harness缺include／include顺序的首次编译失败也保留。

DP控制器恢复、HandStop4→5／quiescent、88独立恢复（含八完整Pose chunks）、Native／entry／XR exit0／PG shutdown0、音频10恢复与原十文件保持通过，随后新鲜CIM无Game／XR，窗口已交还。0.4／幕后／observe-only／静音策略和默认启动器保持。本轮未发布／打tag。

双方已对齐后继最小内容：main_hand_held、当前weapon身份、按资产固定Grip→Wrist Calibration、provider revision，沿现有登记／Frame关联，在同一C0 body plan后、Kernel.prepare前由身体唯一负责人消费；V1只读不动。共享小幅版本合同待身体会话freehands可用后落实，Root负责实际资产认证和发送适配。身体B10已正常清理，但其双腕baseline不可达、无hand commit，不算双手交付。Root下一可玩检查点仍是同一双手候选上的Lancer实际持枪／瞄准／射击；预计有效工作时间和累计窗口等待未知，完整最终范围保留。

## 2026-10-25 — Lancer取得合格同帧骨／枪读数，确认末端与枪体方向存在偏移

继续同一个Lancer交付。骨名查找现按认证GT帧分步准备，每步最多512项、扫描循环3ms预算；原身份／源时效检查在循环前后独立执行，整个准备步骤实测约5–6ms，不声称总用时被硬限制为3ms。只保留名字索引，不缓存姿态／Frame；名字表变化、源过龄与缓存名字内容变化会阻止旧进度用于调用。新增冷表60000项、耗时身份检查、过龄、表替换、非零NUL后填充及未终止字符串的直接回归。曾把身份检查计入扫描预算导致DK游标0，又曾误要求读取缓冲末格0而漏掉有效短名字导致DL超时；两处均修正，失败及源／编译回执保留。

DM在第27573项集齐三名，原MatchRefBone35、GetBoneMatrix35、MatchRefBone51已实际成功；接着读取51矩阵前身份／时效守卫拒绝，记录时源龄约101.14ms。六次原getter加完整前后核验不能稳定放在同一100ms认证帧内，因此改为每帧一个骨与同帧Gun的独立比较，轮转52／35／51，不把不同帧骨矩阵拼成同一帧骨链，不取消任何守卫或延长源龄。

DN候选PG `75730158…3fe82`在真实肩部Grip→held后得到4组合格读数：52两组、35与51各一组，独立分析原源龄92.18–99.49ms，矩阵有限／非退化且无反射，Absolute三项全0。两组End52与Gun世界原点完全相同，但End52_in_Gun方向均为约+90°yaw；Hand_R35_in_Gun原点约(-2.3226,0,0)，方向也约+90°yaw。该结果证明当前原版挂接含固定方向偏移，不能由组件相对Translation／Rotation为零推导世界矩阵相等，也不能将wrist矩阵直接用作gun矩阵。原始矩阵仅存ignored `analysis/equipment-native-rgrip-runtime-20261025dn/attachment-analysis.json`，没有将游戏派生矩阵加入公开源或包。

DN第5组被聚合changed_before_record拒绝，记录时源龄约101.5975ms；旧日志没有细分首先失败的是哪个最终稳定性守卫，不能据此认定组件真实跳变。协调器complete=false、未达到各骨8组配额的结果保留，四组合格证据不等于整轮通过，更不等于控制器驱动枪体、射击／双眼／实体头显验收。之后源码仅细分最终拒绝原因，并在实际记录时间再验证Frame有效性；最后诊断PG `961f7cf2…68001`的严格完整编译和受影响检查通过，构建回执为ignored `analysis/lancer-native-attachment-20261025dh/pg-build/attempt-1792939680249726900/receipt.json`，此版本未启动Game／XR，须与DN实际游戏版本区分。当前不为了静止样本配额继续开局，下一次动态测试随共用双手候选进行。

DK／DL／DM／DN均完成控制器原状态恢复、HandStop4→5／quiescent和正常退出；各自88独立恢复含八完整Pose chunks、Native／entry／XR exit0／PG shutdown0、原十文件与音频10项恢复通过，各轮随后新鲜CIM均无Game／XR，窗口已交还身体会话。四轮都复用Native29sim、XR和reader；0.4／幕后／observe-only／静音与存档保护保持，默认启动器未变。

已把实际方向偏移及下一契约缺口交给身体会话：保留35→51→52自然子链及51／52原动画增量，由唯一共用writer执行；Root负责Lancer握持映射与发送端。现有WeaponHandParticipant仅含左手候选，尚不能传main_hand_held／武器专属右腕mount，不能假装已有可消费接口。当前下一可玩检查点为共用双手＋Lancer持枪瞄准／权威射击，等待身体会话最小双手及消费接点；预计有效工作时间与累计运行窗口等待时间均未知。完整最终需求保留。

## 2026-10-25 — Lancer右手挂接只读采样，首失败保留

本轮唯一目标是取得同一普通游戏GT帧中的Hand_R35、Weapon_R51、Weapon_R_End52与实际枪组件矩阵，核验原挂接链，供共用双手后续消费；没有新增骨骼／组件writer。私有AttachmentObserver默认关闭，沿现有NativeEquipment消费者执行，逐次核对Frame时效、实际对象身份、唯一挂接、反射参数契约和采样前后组件稳定性。只将既有只读MeshPoseProbe移出旧arena开关以便复用，未启用旧组件写入。当前PG候选a38b51e7…c43e对应ignored `analysis/lancer-native-attachment-20261025dh/pg-build/attempt-1792936667671300600/receipt.json`；完整PG翻译单元及受影响快速检查通过，Native29sim、XR和reader均复用原产物。

DI先在真实Lancer读到AbsoluteTranslation／Rotation／Scale均0、组件相对位置／旋转为0，但预期GetBoneName并未反射暴露，因此未取得骨矩阵。随后改用项目已有的MatchRefBone（现有辅助函数名为bone_index，实际调用不是GetBoneIndex），保留完整类型及身份检查。DJ右肩Grip仍成功进入实际held，首个骨名冷查找尚未完成即触发原100ms源时效限制：记录observed_ns减sync_ns约101.7704ms，reason=bone_names_missing_or_source_expired。该结果只确定查找步骤被时效拒绝，不能断言三骨名不存在；没有合格矩阵，也没有动态跟随验收。DI、DJ的complete=false均保留，没有用正常清理accepted替代玩法成功。

DJ控制器状态恢复、HandStop4→5／quiescent、bridge关闭、88项独立恢复（含八完整Pose chunks）、Native／entry／XR exit0／PG shutdown0、音频10项恢复及原十文件保持均通过；新鲜CIM无Game／XR。证据位于ignored `analysis/equipment-native-rgrip-runtime-20261025dj/runtime-review.json`、`runtime-final.json`和`attachment-analysis.json`。0.4、后台不激活／不调窗、静音测试策略保持，候选未替换默认启动器。

已通过成果仍为6063d99的认证Native帧Grip消费者及03b510c记录的Lancer→Snub→同一Lancer往返。当前武器侧唯一技术阻碍为取得时效合格的三骨／枪组件关系；下一步将骨名准备移出整次冷扫描，再用当前认证帧核验，不延长源龄、不缓存骨矩阵。整体下一可玩检查点仍是共用双手持续更新后完成Lancer持枪与权威射击，身体会话尚未交付双手writer，预计有效工作时间未知。DJ核对清理后已释放运行窗口给身体会话B9；等待期间仅处理这一交付相关离线工作，累计等待时长尚未统计。

## 2026-10-25 — 真实Grip完成手枪与Lancer库存往返

DG沿同一个PG `41ef279a…879ee`、Native29sim、Source1 XR与DC reader，仅将目标改为当前场景的手枪槽0和右肩步枪槽3。右手到左裤兜的press1经正常生产消费者请求原版切换，实际当前武器由Weap_AssaultRifle变为Weap_COGPistol；右肩press2随后切回原来的同一个Weap_AssaultRifle实例。两次均经历switch_requested／pending到held，独立读取的PC／Pawn／Weapon／两个Mesh身份、当前武器链接、唯一b_MF_Weapon_R_End挂接及HiddenGame=false一致；Grip松开为0后仍完成持枪。协调器complete／behavior_complete均true、两条episode、无错误，未进入arena或通过模型提交导出来替代原版库存动作。

这证明当前零售场景的“真实模拟器Grip→正确库存切换→实际挂接与软件握持”往返已经打通。它仍使用原版切枪过渡，尚不证明枪体持续6DoF、原切枪动画移除、自由双手、射击／后坐力／伤害、双眼图像或实体控制器验收；DF的不可用槽结果及其脚本假设失败仍保留。完整最终需求不缩减，下一步消费共用双手执行基础并完成Lancer握持和权威射击。

自有控制器原状态已恢复，HandStop5／quiescent及bridge关闭通过；正常finalizer的88项独立恢复含八完整Pose chunks全部匹配，Native／entry／XR exit0／PG shutdown0／音频10恢复及原十文件hash保持，准确自有PID与随后新鲜CIM均为空。完整行为与清理证据分别保存在ignored `analysis/equipment-native-rgrip-runtime-20261025dg/runtime-review.json`和`runtime-final.json`。窗口已交还共用身体会话，当前候选没有替换默认启动器。

## 2026-10-25 — 普通游戏取得认证手柄帧与库存，修正取枪消费者的旧通道依赖

本轮唯一玩法目标是Grip导致正确库存切换并实际持枪。DD已在购买版普通场景取得真实Source1 GVC3／Stereo匹配，初始样本原源龄约1.47ms；HandStart及唯一Native CF登记成功，正常PostPlayerTick消费者取得当前Frame并完成实际库存读取。但输入始终baseline_waiting，三秒后停止，未发肩部Grip。首个确定失败为消费者依赖了另一个历史Root分支的EquipmentIntentBridge v6，而实际冻结Native29sim源码根本没有该producer／头；不是实际手柄样本无效，也不是已发生的库存切换失败。

DC私有只读读取器已完成真正独立target／publisher／reader三进程回归，保留公共same-process guard、原六头、匹配循环和最终双源100ms检查；它在DD实际会话中已成功，后续复用。DD的原失败保留，正常停止取得HandStop5／quiescent、控制器恢复及bridge关闭；独立88项含八完整Pose chunks、Native／entry／XR exit0、PG shutdown0、音频10项及原十文件hash、准确PID退出通过，随后新鲜CIM无Game／XR。该清理accepted不代表取枪成功。

生产修正改为在已有普通GT路径直接消费同一认证Native Frame，只路由右Grip四槽；使用当前head位置与稳定reference朝向、记录的last-press pose及明确校准，不创建新Native producer／hook／骨骼或相机writer。连续原始sync与序列夹住最新按压，created_ms从较早原sync保守折算，原始时间继续保留；失效或长间隔重建基线而不重放旧按压，空中按压不妨碍下一次肩部按压。Native注册前从已认证并pin住的模块旁capture.ini读取两项实际校准，缺失或非法即拒绝，不猜臂展；重连前检查PG callbacks已退，避免旧GT仍借用配置时worker重写。Sol复核关闭该生命周期缺口。

实际新输入→Runtime→模型定向检查、原Runtime回归与Windows模块路径／UTF16配置检查通过。完整PG TU及相关hand／shot／CF／protocol检查随后通过，737份编译输入保持原hash；候选`GearsPlaygroundNativeEquipment.dll`为`41ef279a3c8b7d83a8a214879082bea0d11d3a4dccf26a286152b0556ad879ee`，13导出、Gate／Entry仍OFF。构建回执在ignored `analysis/equipment-native-rgrip-20261025cw/pg-build/attempt-1792931495360835400/receipt.json`。DF复用DD host／原Native29sim／XR／DC reader，仅换PG与独立场次，近期顺序同步到武器规格，完整最终需求保留。

DF首次真正触达目标Grip：右肩press1经生产输入进入slot3 held，独立读回同一个实际Weap_AssaultRifle、当前Pawn／PC／两个Mesh、唯一b_MF_Weapon_R_End挂接及HiddenGame=false均通过。该枪原本已是当前武器，证明同槽建立握持，尚不能声称换另一把枪。第二次左肩press2被正确识别为slot2，生产返回empty_slot并沿原反馈路径完成；它表示该槽没有可用物品，现有记录尚未区分无对象与不可选的空弹武器。脚本原假定该槽会held而超时，原complete=false保留。不得为通过脚本而生成物品或跳过可用性检查。HandStop5／quiescent、控制器恢复、bridge关闭、88独立恢复及原十文件／音频通过，fresh CIM无Game／XR。下一DG沿用同一个PG／Native／XR／reader，改测左裤兜手枪0与右肩步枪3，核对实际不同武器切换；尚未替换默认启动器，也不代表枪体6DoF或头显通过。

## 2026-10-25 — 正常游戏与XR启动通过，RGrip派发前的依赖路径修正

DA实际使用购买版、Source1模拟输入、0.4缩放、置后窗口、tiny只观察及自有游戏静音，保持原启动窗口门，成功进入连续Native／XR并发布ready。Quiet六次真实只读查询均完成原线程退休、有效读取及关闭，无待处理调用；这补足此前受控OS seam测试的实际查询证据。单局启动通过不能说明cx原窗口失败的根因已修复，也不代表身体或武器交互通过。

RGrip协调器随后在构造Bridge之前因子进程缺少已有pefile库的搜索路径而退出，未调用HandStart，也未发送Grip。原会话正常停止：Native cleanup、入口恢复、XR exit0、PG shutdown0／入口恢复、音频10项恢复和原配置／存档10份hash保持均通过；独立读取80项全匹配，原host结束瞬间的进程仍在退出记录保留，后续新鲜CIM确认准确自有PID及Game／XR为空。继承的finalizer还要求八段完整Pose代码证据，但本局Pose模块关闭时旧targets未包含它们，故`runtime-final.json`的accepted=false及空pose_checks保持；不能将80项结果写成完整八段验收或99项。

新DB仅在私有host接入J盘现有pefile／ordlookup五文件，导入前核对hash、导入后核对精确module origin，不安装依赖；实际独立导入已通过且未构造Bridge。只读停止验证器在原stopped／进程／manifest守卫之后，将既有三个固定Pose入口加入原targets，沿用原八段完整范围及预期hash；没有启用Pose、改变允许的hook清单或降低finalizer条件。Sol有限只读审查未发现必须修复项，30份Python、12个fresh namespace、6个CLI help及3个retained CPU场景预检通过。DB pins为`c9fc59bf24877106c31174209471a5c4bd1baf16f3e37f23b850eeeab914708d`；当前候选仍独立于默认启动器。

DB随后实际完成第二次Native／XR启动和Bridge attachment，但初始只读reader返回66，export_history／commands／episodes均为空，未HandStart／Grip。Root与Sol由实际冻结源码确认：66对应bridge open失败，而ControllerInputBridge消费者明确要求目标PID等于本进程；独立reader传入GamePID违反该调用合同，即使Stereo打开也会被拒。这是Root测试reader的确定接线错误，尚未执行时效或Source1匹配，不能归因于实际追踪生产失败。后续修正限制在私有跨进程只读observer，不改生产bridge进程guard。DB正常finalizer accepted=true严格限host／清理范围：88项独立读取全匹配，包含八段完整Pose chunks；Native／entry、XR exit0、PG shutdown0、音频10项恢复、原十文件hash及准确自有PID退出通过，随后新鲜CIM确认Game／XR为空。原DB行为结果complete=false与错误保留，完整功能进度仍不计取枪成功。

## 2026-10-25 — 启动诊断阶段判断修正，保留两次未执行交互的实跑

cx的prime原始记录进一步确认：同一真实交换链HWND在ResizeBuffers前被观察为iconic、client0×0，而desc／buffer仍1280×720；早先通过的掩体tree1父局也有该中间状态，因此不能把它直接定为cx失败原因。后续cz保持原可见／640×360／30秒门，仅加OwnPID窗口元数据和已有Quiet诊断导出的有限查询。查询模块使用原retained adapter，限定固定Quiet DLL／两个只读导出，原线程返回后才读回并释放自有输出；每次派发前重核原module path／base／SHA／executable image，未知不重发。九个受控OS seam场景覆盖有效非空记录、超时、exact exit、readback／decode／写入／回调保存失败；Astra关闭其1P1／1P2。它们不构成实际Quiet／窗口验收。

cz实际启动时，新增诊断guard错误地要求birth receipt的injection_attempted一直为false，正常false→true推进被当成异常，watch在约两个窗口样本后提前取消。连续Native／XR、Quiet RPC、Grip及participant均未启动，不能用这局解释原启动窗口问题。已修的driver terminal／join记录路径确实执行并保存原失败。host结束瞬间game仍在正常退出，原no_remaining_games=false保留；Root另行新鲜CIM确认Game／XR／原owner已退出，并核prime取消／入口恢复、PG预留shutdown0／PE恢复、音频1项恢复、原配置存档10份hash一致、tiny monitor与watch已join。独立pre-session清理回执位于忽略的`analysis/equipment-native-rgrip-runtime-20261025cz/startup-closeout.json`，SHA256 `9ce4e6304a963e09122a0c8c8f995ed7f1d92c9b376da764ee61d535aaca4a08`；不覆写原host的瞬时false。

新DA保留cx／cz和全部冻结输入，仅删除只读诊断对该可变阶段位的要求；kind、完整进程身份、精确image、当前PID／birth核对与后续incarnation检查保持，原启动Prompt点击仍仅限pre-injection。直接提取实际guard AST验证同身份false／true及三类坏身份共5例；Sol独立窄审无新遗漏。另将Root额外19工件纳入实际verify_pins强制校验，避免只验证40个host文件。全套新namespace预检通过30份Python、12个fresh child、6个help、实际CLI／INI及3个既有retained用例，未启动Game／XR。首次派生后的AST验证器误选外层if而停止，实际源已派生；保留失败并由独立验证器只重验同一字节，没有重拷贝或启动。DA pins为`cf91c76871aceeff0b269edaaccfbc1f59553b0a49223c04c2945f63f0fcb46a`；本条落盘时等待掩体Native30实跑正常收尾后协调下一窗口。

## 2026-10-25 — RGrip实测在启动窗口门前停止，尚未执行交互

为验证正常输入消费者，冻结独立cx host、实际13导出PG／42导出Native29sim、原Source1 XR及私有cy模拟器。cy只补既有文件命令的可选有限数值grip；完整DLL、588项实际parser／赋值检查和68项既有behind策略检查通过。Root逐项核25份cy源、365份实际compiler includes及每个delta的内存逆向恢复，原9abbeae4模拟器不变；另确认PG／29sim实际六份wire头仅CRLF表示差异，只读GVC3／Stereo helper使用六份真实headers编译，保留原publisher与source时间检查。host的27份Python、12个私有namespace、3个actual retained-adapter用例及实际CLI／INI／导出预检通过。Astra关闭协调器1P1／2P2后才执行，不把预检称为游戏动作成功。

本机原默认游戏工作副本已不存在，选择与掩体近期实验相同、哈希仍为bdc6705d…8fa4的现存购买版Steam EXE；配置与存档使用新的私有实例。与掩体协调并确认Game／XR空闲后，执行cx一次后台启动：0.4、静音、置后、tiny-output只读、物理手柄后台输入关闭。原启动门在30秒内未找到可见且客户区至少640×360的游戏窗口。末次只读诊断有两个该PID的HWND：一个visible但client为0×0，一个hidden且未读取rect；尚无class／title，不能由此判定原因。

实际Native只完成启动预留／prime，连续session尚未激活，XR未启动，PG仍为reservation-only；host与等待协调器均已terminal，未生成ready、未StartParticipant、未发送Grip或切枪。0.4每眼1440×1280仍只是启动计划，不是本次GPU尺寸证据。本次不能计入取枪、自由双手、游戏立体或头显验收，也不能归因于尚未运行的新装备路径或cy Grip代码。

Root独立核定本次pre-session收尾：prime cancelled且entry_restore_verified、watchdog无pending；PG reservation shutdown0／PE restored；音频10项恢复无pending，原配置／存档10份hash一致，tiny monitor已join，新鲜CIM确认Game／XR及原owner均不存在。此范围的cleanup_verified为true，保留startup_passed=false／RGrip_accepted=false；未套用要求完整Native／XR会话的99项验收。所有失败及原冻结文件留在忽略的`analysis/equipment-native-rgrip-runtime-20261025cx/`，专门收尾为`startup-closeout.json`。下一步只读定位原启动窗口未就绪的原因，并补齐host在driver先结束时遗漏的watch终态记录；不放宽窗口门、不前置激活或调整用户窗口。

## 2026-10-25 — 正常游戏RGrip库存消费者完成实现与离线验证

新增默认关闭的`GEARS_PLAYGROUND_NATIVE_EQUIPMENT_TRIAL`，在原PlayerTick返回后处理Native既有GVE6身体槽位事件；不依赖测试场、旧switch/intent导出或预先提交的拿枪模型。每次以当前Native注册、进程出生、world generation、完整World／PC／Pawn／Mesh身份和原始XR输入时间绑定，成功建立cursor基线后才处理新抓握。重复press不重复切换或提示，最多保留一条原样future事件；同actor已发出的原版切枪在输入暂失时只继续观察，不取消、回滚或重发。

不同槽位只调用一次原版`SetWeaponFromSlot`，其返回仅表示开始等待；五秒内还需观察目标完整身份、实际当前Weapon、原切换动画结束和真实挂接，才提交held。当前槽位也必须确认唯一可见Weapon.Mesh挂在当前Pawn.Mesh的`b_MF_Weapon_R_End`；HiddenGame通过严格typed Reader读取，读失败不能当作可见。请求槽位无枪／无可用弹药时沿用原empty cue。新增`copy_frame_on_gt`读取既有严格publication，以及仅复核原source的`source_current_on_gt`，后者不授予动作许可、不更新committed_game、不刷新追踪时间；它允许软件拿枪状态在本次提交后与旧publication暂时不同。私有held binding补Native incarnation／cookie／world generation，防止旧注册继续满足握枪判定；公开wire和client数据布局不改。

库存原调用仍只发生在正常PG边界。新domain门在任何frame／inventory工作前排除CF／NoAuth栈和ActionGate递归；CMake拒绝与controlled IK或旧库存writer混用，并要求NativeEquipment+ActionGate配套实际Entry路由。初始化重置仅在client off且callbacks为零时执行，关闭重置在Native quiescence、停用PE hook和PG callbacks排空之后。纯诊断计数在排空后输出，区分无Frame、非本PC、域拒绝及inventory查询阶段，未新增引擎写入或后台轮询器。

实际Runtime编排的严格MSVC测试为84 cases／685 checks；真实client读帧／source复核共60组通过。冻结734份项目源的完整PG候选及四个相关既有host、四种实际CallSubscriber协议模式通过，包含递归域拒绝断言。六组真实CMake组合检查与Gate+Entry完整PG构建通过；Sol只读复核关闭此前HiddenGame P1和两个配置／递归P2。最终补诊断后的完整构建及相关测试仍通过，候选`GearsPlaygroundNativeEquipment.dll` SHA256 `2617d3226b616dd392545dafcd28d11c75d4a22e9e78d6ca4cf633e228ed81a8`。证据位于忽略的`analysis/equipment-native-rgrip-20261025cw/`；此前缺snapshot include、命名／WX编译失败，以及protocol harness漏argv的失败均保留，未覆盖成通过。

本记录只证明实现、离线编译与协议／逻辑检查。真实XR Grip到实际库存／挂接的实验正在另行准备，尚未执行；当前保留原版切换动画，stow明确返回unsupported并保留原握持，尚未达到用户要求的无切换动画取／收枪。持续自由双手、身体对齐、动画交还、全武器换弹／射击／伤害及头显体验仍未完成，完整目标继续active，默认冻结Native／XR和启动器均未替换。

## 2026-10-25 — 校准卡实际模拟器合成、状态变化与正常退出

与掩体Native28测试协调独立窗口后，使用未改字节的c84a0e7d候选和固定9abbeae4置后模拟器运行约98.5秒。此局只有自有Python显示fixture与XR41548，没有Game／Native／PG，不调用个人测量或保存。0.4参数产生每眼1440×1280的计划，日志明确这不是游戏GPU分配证据；实际唯一UI swapchain为768×256。全部8818帧有模拟器追踪，日志game_input_injected=false；CSV source0仅是常规XR来源编码，运行时明确simulation_not_physical_tracking=true，不能当物理测量。

真实compositor截取英文等待、中文50%、绿色保存及停止显示心跳后回等待四种状态，MCP诊断log PID与实际child41548一致，FOCUSED、无errors／warnings。成功提交8817层UI，只有4次GPU上传；saved序号356取得对应exact成功xrEndFrame ack，游戏立体提交／scene copies均为0。正常stop_event后同句柄wait到exit0，无force；实际terminal EXITING路径未调用endSession，既有session_exit政策仍确认objects_destroyed／shutdown_completed。UI／session／instance按正常顺序清理，前后台焦点句柄前后相同，新鲜CIM确认Game／XR均为空后释放窗口；不把前后焦点快照当作全程Z-order观测。

截图中卡片偏扁已按固定runtime源码和本次CSV FOV解释：quad-only D3D12按1920×348预览客户区创建画布，Fill不保持内容比例，每眼viewport960×348；tangent投影预测3:1卡片变为约8.7975:1，再按截图缩小到约202×23像素，与实图吻合。保留产品1.4×0.4667m尺寸，不反向补偿模拟器预览，也未修改模拟器设置／源码。该结论限定本次超宽Fill预览，物理头显比例与可读性仍待验收。

实际日志、四份原样JPEG、显示harness、比例审查及收尾回执位于忽略的`analysis/armspan-hmd-ui-20261025cv/`，总审计`closeout-v1.json` SHA256 `8c47f79c8e6dc1e727ef333fbf1ab3aefa28dc20dd0f5f6d1ff4b281523283bb`。本组证明真实模拟器显示及生命周期，未证明完整首测流程、玩家臂展、游戏内双手／动画交还或全武器玩法。

## 2026-10-25 — 武器分支接收完整C0身体内核

在头显校准UI独立提交f5e2884后，接收掩体c1ed4bf的完整纯数组body planner／测试及16行CMake注册。唯一冲突为历史进度记录，保留双方内容并明确Native26c／27原实跑属于掩体工作树；未复制其capture.cpp或其他未提交Native接线。原头／脚目标、Root／脊柱／腿与输出新肩位的接口保持，不把尚未接入的身体输出称为实际跟手。当前范围仍为Marcus115、scale1、固定脚和yaw±60度；HMD眼位到头骨目标转换、走路换脚、完整掩体角度与实际writer均未完成。

Root核对四份fixture、原始采样二进制／解析回执／本人先前骨名join的来源哈希，冻结本分支659份Native／common源码，使用当前实际共享headers与严格W4／WX配置编译FullBodyC0PlannerTest。确认CTest精确注册1项、实际执行9456 checks／0fail并生成20份115×32B候选；独立FK检查最大头／脚／段长残差分别为0.000254662／0.000570187／0.000314261原始单位，9456主要是逐骨检查，不是玩法场景数。最初harness把Windows反斜线路径与CTest JSON正斜线argv作字符串比较而停止，保留原回执；按实际路径归一核验后只运行已成功编译的同一exe，没有重编以覆盖失败。

接收证据为忽略的`analysis/body-c0-receiver-20261025cu/`，最终`validation-1792916254856275600/receipt.json` SHA256 `82c09368bbd6c17eecf82b2d7ce32eb15280959ac634b8491471d5afcdff8bfa`。本次未启动Game／XR／IDA、重建Native DLL或启用骨写入；完整武器功能、身体校准与动画交还继续开发，默认冻结构建不变。

## 2026-10-25 — 首次臂展校准接入头显双语说明与进度

校准候选在Native激活前的独立XR子会话中显示双臂展开示意、两个Grip保持2秒的说明、长按进度及绿色保存结果，并沿用启动器中英文设置。CPU卡片采用内存GDI而非屏幕采集，中文缺字有明确英文回退；正常启动菜单／返回游戏／追踪丢失三种原静态卡仍走原颜色与一次上传路径。动态卡只在内容改变时上传，严格等待XR取得图像及GPU完成；非exact成功、会话失效或GPU超时不冒险释放／销毁不确定资源。补充正Acquire SESSION_LOSS_PENDING的持有状态处理，原冻结二进制不改。

新增112B显示协议，以本次game／owner／publisher身份、随机token和单调序号绑定，只读消费者使用750毫秒UI期限；显示心跳不能推进测量或刷新追踪。长按未完成且实际采样停顿超过100毫秒时清空窗口。原子保存先于绿色卡片，只有对应卡片实际提交给成功xrEndFrame后才确认反馈，再保留至少0.6秒；失败保留已存记录并阻止本次Native激活。读者关闭、XR独立正常退出后才释放显示mapping／mutex。UI模式不创建WGC／Native画面接收器，独立计数，不冒充立体游戏证据。

冻结250份XR／common源码，完整OpenXRPoseProbe候选编译通过；新增3项CTest全部通过，含221项实际IPC／进程寿命检查、246项像素检查及25组939项真实WARP上传读回／协议故障检查，另有6项原卡片／菜单／会话检查和2项probe自检。真实Python→独立C++读者的112B往返保持原packet字节，退出后命名对象消失；15组无效CLI在创建XR实例前拒绝并清理自有stop event。14份英中文无损预览已生成，Root目视核对英文说明、中文说明／99%／保存状态。启动器在Python3.14.4与便携运行时同版本的3.12.14各自整套284项无skip通过；首次测试因未接现有pefile依赖路径出现2项import错误的回执及SDK相对路径预检失败均保留，未放宽生产逻辑或安装依赖。

证据统一位于忽略的`analysis/armspan-hmd-ui-20261025ct/`；XR完整构建为`xr-build/attempt-1792915291110463800/`，只读审计与像素／Python UI／跨进程回执独立保存。本组没有启动Game／XR session、替换默认Native／XR或发布版本；实际HMD显示、完整校准体验、自由双手写入及动画交还仍未验收。下一步接收掩体共用C0身体内核，保持完整武器需求与优先级。

## 2026-10-25 — 保存臂展接到共用计划，修复重复帧测量重置

原C++ArmspanCalibrator把同sequence重复观察与倒序一起清空并重开计数；游戏帧率高于输入采样率时可能测不完。独立提交cb32e45改为：同key且实际测量字段完全相同的重复样本不推进样本数／时间、不清窗口；倒序或同seq内容改变则清空并返回，失效与T-pose检查仍先执行。字段逐项比，无padding memcmp或整Frame栈副本。严格CL新增1616项及原断言通过；同新测试对冻结旧头在重复进度处失败的反例保留，Python11项亦通过。此修复属于武器分支原有legacy测量路径，掩体无需引入其body-slot模块。

新增共用个人校准数据／参考尺寸／配置模块，把原schema1的两字段从单次启动读取接到连续只读owner。加载不再要求legacy controller observer必须开启，已请求的共用手部模块也能消费；requested预读取仍不启用Hook或授予owner。启动器已有校准文件不改格式，Native不保存文件，legacy/mailbox仅复制该次启动snapshot。真实Windows临时INI的75项检查通过，覆盖空值、部分字段、坏数值、consumer/stereo条件、截断、失败不变及自有文件删除；首次测试错误断言INI flush返回值的失败原件保留，生产reader未因此改动。

从同源已核对canonical115参考父链求8/32肩根间距及两条8→10→11、32→34→35段长，仅乘一次model_scale／units。v1估计s=(保存span−参考肩宽−双mount偏移模之和)／四段原长和，保存四段绝对米长；mount偏移最大向外共线延伸是可能估短的authoring假设，不恢复真实grip朝向、肩宽或人体比例。纯模块175项、带真实RO3参考189项通过，与既有Native pure-buffer FK的最大尺寸差2.5176e−8m。未知模型、坏几何、单位、非有限值、溢出及别名不改输出，不夹比例或修改完整腕定位。

连续owner跨Tick按记录、模型／units／scale、mount／估计版本及实际session／asset／mesh／world generation绑定缓存；每次把保存的绝对段长除以本次原始component→world段长，再传既有接口。目标腕距不进入派生或cache key，有记录时拒绝adaptive；没有记录仍保留原只读诊断默认1。当前实际owner仍只接纳model_scale=1、Marcus115；既有unit CTW和strict／near-rigid输入策略未放宽。CHOWNER1 Header／TickRecord／Plan／Failure定义不变，新诊断仅报告估计、四段目标长度与绑定／应用计数；仍不写Native输出数组或手骨。

完整Native CR1已实际编译链接，DLL64929eeb…974ce，201项已注册相关CTest通过。继承25的cache漏了RO3 matrices/reference，首次未注册owner专门模式；Root在独立CS host build补齐三份fixture并强制核对测试名，再执行额外46项：全部19个连续owner模式、真实参考测试及新增可注册HandGoal用例均通过，未重编或覆盖已成功的Native DLL。新五模式覆盖多目标／原始段长变化下四段长度固定、缓存复用与变化重绑、默认值及错误保持。只新增测试证明实际uniform CTW1.1／1.2按既有strict早拒，没有把Root最初混淆similarity与control::rigid的静态判断误记为生产修复。

证据在忽略的 `analysis/hand-personal-calibration-20261025cq/`、`hand-personal-native-build-20261025cr/` 与 `hand-personal-owner-host-20261025cs/`。主构建／CTest回执40c89597…9cc43，额外46项回执bc8b1045…3c8f0fb；独立接线窄审c558866c…144eba7未发现P1/P2。Root本轮没有启动Game／XR或替换默认二进制，Source1／host值不冒充玩家物理测量；首次HMD校准界面、实际跟手／身体对齐、骨写入、动画交还与头显验收继续推进，完整目标仍active。

## 2026-10-25 — 共用固定个人段长接口与最新组合完整构建

以独立三文件提交35a4497向共用LimbInput／LimbData增加upper_length_scale与lower_length_scale，默认均1，adapter每次显式传入已有LengthPolicy；control检查finite／positive，无效或数值溢出不改输出。preserve保持本次原始输入段长乘以固定比例，adaptive仍可按原规则伸长／重分配；调用者必须绑定正确模型与保存的校准，并使用原始基线，不能把改过的结果继续当作原长。未从腕距、眼骨或IPD推比例，未改头手／世界单位，也未加入live LocalScale写入。

独立严格CL检查为Adapter9组820项及Control24 cases／417项通过，包含非均匀1.25／0.9、三个目标距离下固定1.5、复用工作区恢复默认、非法值／溢出不改输出及旧回归。Core／grip／个人比例仍明确为synthetic，矩阵使用原RO3样本。Astra限定审未发现P1／P2，报告395a5829…3f65。private LimbInput／LimbData及嵌入Workspace的C++布局确实变大，须同步重编消费者；公开Frame／ReadRequest／Core头未改，不能将私有C++布局称为ABI不变。

随后在武器实际工作树上冻结完整组合：掩体fa283ca（f5矩阵策略、连续只读owner、post-weight只读观察）加武器输入／唯一wrapper及新段长接口。完整capture.cpp TU编译、链接通过，私有Native为ec9b5b75…ee628e；727份C/C++／ASM／CMake输入与当前源码相同，原42导出一致。198项相关CTest通过；继承的25 cache未提供caller fixture，另构建实际ContinuousHandOwnerTest并使用原pin样本／argv执行14个模式，全部通过，已成功Native DLL字节未变。没有把这14次直接执行改称CTest，也没有重新执行已通过组来覆盖缺少fixture的注册范围。

完整组合审计为忽略的 `analysis/hand-shared-native-build-20261025cp/closeout-v1.json`，60dbe936…b1d8d；主构建／CTest回执10aedf9a…24699，14模式回执7d8d3f7a…e9471e。独立接口交付 `analysis/hand-fixed-length-calibration-20261025co/handoff-v1.json` 为925ef815…57ea8，已交掩体会话按单一版本接收。Root没有启动Game／XR或替换默认二进制；旧24b／25冻结源与旧2bc实跑结果保留。玩家首次臂展采集到此接口的连接、身体／眼位对齐、真实连续手骨跟随、动画交还与头显验收仍未完成，完整武器目标继续active。

## 2026-10-25 — 接收连续只读手部 owner 与 null 身份复核修复

在f5基础合流cf0383d之后接收掩体独立提交ac03bd4，11份原生源和两份文档同时保留，未使用其两个未跟踪旧C3文件。此路径不依赖Pose事件，在真实outer Tick锁存一次Core并让四个原生手控节点复用；cap-OFF路径保留实际no-op语义。它仍是只读计划，不发布正式手骨姿态，也不提供writer或physics fence许可。

Root独立核对13份固定交付文件及11份源的真实commit表示，按Native24b实际ABI重新解析原始132944B sidecar，与原报告完全相同：411 Tick、411次getter、394次完整四计划，reject计数45不是失败Tick数；末32条环形记录全部complete／failure0，QPC起止有效。Root接收回执 `analysis/hand-continuous-receiver-20261025cn/attempt-1792905481414545600/receipt.json` 为2db36ef7…1dc04。owner原96项恢复／XR0／audio10／原十文件通过；Root本轮未启动新Game／XR，仍区分接收核验与亲自执行生命周期。

该实跑明确启用了adaptive数学诊断，部署24b保留旧2bc输入算法；当前合流源使用f5显式矩阵策略，二者未混记成同一已测DLL。未验收正常臂长、身体／眼位对齐、独立手部运动、实际骨写入或头显。新组合将以独立完整TU构建继续核对。

另用独立最小提交ab2ec38修复双方旧Python共有的null Controller.Pawn末次身份检查：绕过本次assembly hint缓存，真正fresh_read controller／field。合法typed-null仍按原政策允许启动重试；reuse/coalesce四组合下controller／field／offset变更均硬拒绝。31项限定测试及前述929项完整Native Python通过；只改该一行生产逻辑和两份回归，不改start_capture分类、renderer、Scale或手骨权限。相应证据在cm/sol及cm最终Python回执，供掩体和最新默认源独立接收。

## 2026-10-25 — 共用手部基础实际合流与完整 Native 编译

在独立 `codex/weapon-6dof` 整合掩体稳定提交f5da0d1，保留双方历史与功能，未切换或写入掩体／J工作树。10处冲突分别合并：Native Runtime／Core／HandGoal与武器controller mailbox／intent／equipment共存；CallFunction仍只有唯一wrapper／original，武器原只读观察体迁至独立include；停止、owned和exports均保留。L3同票据回中后的最终reference才进入Core快照，避免带旧reference epoch发布。构建／staging清单及两套Python测试同时保留。

已冻结实际合流源并编译、链接完整capture.cpp TU，私有DLL为6cdd51c2…211153；核对实际568项compiler dependencies，当前718项C/C++／ASM／CMake输入与冻结源一致，原Cover导出全部保留，共42项。185项相关CTest通过，覆盖手部／Rig、消息连续性、UI调用、SEH及停止等实际host路径；18种消息及嵌套类型的197字段布局逐项一致，Core仍1832B，公开Rig仍1824B／Read40B。内部StereoGate仅增加两个observer指针；这不是公开wire ABI。

Python Native组929项（12项按既有条件跳过）、启动器264项、XR6项最终均无失败；该完整工作区检查包含下一条独立提交的null guard修复。旧测试startup返回值补齐真实stdout／stderr形状，并mock原本遗漏的进程枚举，避免测试查询用户实际游戏。首次CTest清单缺少尚未构建exe的command、Python式正则不受CTest支持、缺NumPy环境、过宽audit拦截只读CIM及旧mock失败全部保留；未重编已成功的Native DLL来覆盖这些后续harness失败。

交付审计为忽略的 `analysis/hand-shared-native-merge-20261025cm/build-closeout-v1.json`，542b627e…89341；185项CTest回执99788075…9c4f，最终Native Python回执a86e29fc…b4b94c，Astra／Sol落源与布局记录同目录保留。本组确认实际共享源码可一起构建及host合同，未启动Game／XR、替换J默认二进制或发布版本；未验收手骨写入、实际动画交还、0.7伤害整链或头显。下一份连续只读owner接线按掩体独立提交继续接收。

## 2026-10-25 — 第一人称实际目标、身体基准与显式矩阵策略

继续按用户要求先完成双方共用的自由双手。Root对同一 Native23b模拟器DLL和同一XR、固定头／手原始Pose的两局购买版数据独立重放：普通视角实际policy为0，第一人称局为1；两局分属不同进程／时刻，不把它们冒充同一骨姿态。第一人称的Core.world_head到C1 head55由约1.744m缩至0.269m，但实际左腕目标距肩61.900原始单位，仍大于约46.995的原版臂长，preserve求解拒绝且输出未改。右臂原矩阵先被严格规则拒绝；其有限位置计算的目标距离85.687单位只作诊断，未记为合法输入或成功求解。

对全部115骨参考签名、原始CCTRL与C1重新核对，Limb肩原点经component-to-world变换与对应C1肩点一致；未发现一侧误用局部／世界空间。第一人称实际两眼骨间距约5.35427单位，eye midpoint与head55相差约9.845单位，当前world_head到眼骨中点仍有约18.152单位偏差。肩线相对实际base right约转过−42.866°，不能假设相机横轴就是身体肩轴。以上米制数值仅沿用原u100；骨pivot不是脸部表面、玩家身高或标定依据，未根据模型比例改头手／IPD单位。

离线重放工具与证据在忽略的 `analysis/hand-native-core-input-20261025cj/`。FP2原样重放回执b60a09e7…fa63e，相同DLL双局比较及身体尺寸见 `dimensions-v1/`；最终眼骨交付eb042cef…a4a56。原始Core、控制矩阵、参考骨与前后hash保留，缺失阶段继续缺失。仅把所有腕目标平移到head55的反事实试算也明确分开：左侧几何可达、右侧仍不可达，没有应用到游戏或当作定位修复。

接收掩体稳定提交f5da0d1的共用 `OriginalMatrixPolicy`：默认仍strict_unit；显式bounded_near_rigid仅允许原Limb输入3×3的有限正行列式、严格仿射和限定Gram误差，component-to-world、输出及Single专用规则不放宽。该工程容差也会接纳足够小的真实缩放／剪切，不能宣称只处理浮点舍入或已证明独立LocalScale语义；没有开启实际writer或默认opt-in。

Root以精确17份源接收编译，适配器8组626项通过，回执d3038c09…bef3de。随后把未改的真实FP2控制器腕目标分别交给两种策略：左侧unreachable→unreachable，右侧invalid_matrix→unreachable；四次失败均保持输出原字节，无adaptive调用。回执为 `analysis/hand-matrix-policy-adapter-20261025cl/actual-fp2-1792903324632193800/receipt.json`，25527283…b834df。此结果将原矩阵接纳与身体／臂展问题分开，不能算自由手、连续骨写入、动画交还或头显验收。

## 武器 6DoF 分支记录

本文件记录 `codex/weapon-6dof` 的后续工作。已发布的 0.0.3、既有分支历史和冻结产物继续保留；早期武器记录见 [0.0.3 进度](../0.0.3/PROGRESS.md)。完整需求见 [武器 6DoF 交互设计](../../weapon-6dof-interaction-design-20261022.md)。这里的私有候选和自动测试不代表新版本发布或头显验收。

### 2026-10-24 — 唯一开火调用入口的真实联调准备

武器候选保持 `c41a65c` 的 PG subscriber（6157a180…5c69），与掩体会话交付的 Native9sim（e8109e81…5727）及 XR5sim（6e0f44c7…4839）进行一次私有互通验证。Native 独占 CallFunction，PG 只管理 ProcessEvent；保持正常 UI，不再使用关闭 UI 后并列安装第二个 CallFunction hook 的路径。原 hand v1 和补充调用 ABI 字节保持不变。此组合只用于模拟器联调，不覆盖最新默认启动器；今后正式集成须以用户指定的 support3 Native／menu-modal1 XR 冻结基线为起点，保留左眼独立历史、菜单修复和诊断功能。

私有测试使用购买版、独立配置与存档，每边渲染比例 0.4，静音、窗口置后、tiny-output 只读，保持物理手柄后台输入关闭。一次真实原版 R2 开火前，由游戏线程对当前 Lancer 的实际槽位提交一次模型拿枪，再连接真实 Native hand／call 注册；不设置 single fixture、不直接调用伤害 getter，也不把模型提交当成身体 RGrip 或双手抓握。测试须分别证明 PG exact-cookie 排空、PG 的 PE 恢复且 Native 调用入口保持、Native 正常停止后的两个原始入口恢复、XR 退出、原音频与原配置／存档恢复。

离线预检查已通过并固定实际工具来源。历史 AP 的 Cover 工具路径只作参考，实际运行使用武器工作树的 1221 项工具源码；派生平台固定 Source1、原始 flags、真实 XR runtime 名称／版本／模块路径和 8720 监听进程身份，未放宽正常头显路径。新增的 passive SupportJournal 单独固定来源，未改已冻结平台源。第一次预检查因历史 Cover 诊断 helper 已变化而拒绝，第二次发现缺失的 passive helper；原失败均保留。

输入脚本采用私有 SafeBridge：提交前持有 request 与实际 Submit thread，原绝对期限覆盖反射、提交和完成等待。Submit 仍在执行时即使 GT 已写完成也不回收；仅已知拒绝码 1／2 可按未受理处理，未知线程退出保留不确定状态。超时只观察同一请求，不重发开火；补发释放以实际 dispatch 记录为准。静音由 root 的唯一 Guard 管理，启动器复用精确身份的 ready 凭据，避免两个 Guard 相互记录已静音状态。晚到的 owned game 和清理本身抛错均继续进入确切游戏／音频收尾。

9 项人工输入边界检查和 45 项 schema2 日志规则检查通过；Astra 对原先四项收尾问题及最后两处修正完成限定源码复核。人工检查不等于跨 DLL、实际伤害、GPU 或头显结果。准备回执为 002d5573…ba556，私有 bridge 检查 4898bbae…ae66；证据位于忽略的 `analysis/weapon-call-runtime-1024bi*` 目录。本条落盘时尚未创建 BI 游戏实例或启动游戏／XR，实际结果须另行记录。

真实副握 mount、全武器装填与黄色引导、自由手动画交还、投射物／延迟伤害、枪口弹道与准星、瞄准镜、体感电锯和投掷、默认合流及整局／Quest 验收仍未完成。完整武器 6DoF 目标继续进行。

### 2026-10-24 — BI-r3 真实共存启动与清理，测试场顺序修正

首次 BI 在创建游戏前因多传 `remeasure_armspan` 配置字段被拒绝；新的 BI-r2 在同阶段因隔离 Python 未显式提供既有 pefile 库路径被拒绝。两个已终止 worker、无 birth 的原始失败、各自一次性 marker 和私有配置保留，没有覆盖或因观测超时重启同一测试。BI-r3 去掉无关字段并仅向私有 wrapper／后代补充既有库路径，未安装依赖、改正常启动器或重建 Native／PG／XR。冷子进程导入检查通过后才启动。

BI-r3 在购买版实际完成正常 UI、Source1 启动及 9025 次立体 projection 提交。实际 XR 名称、版本 281474976710683、runtime／operator 模块绝对路径及 IPv4／IPv6 的 8720 监听 PID 均与确切 owned XR 对上。请求比例为每边 0.4；启动游戏图像资源实测 1536×864。提交计数与尺寸不是画质、可见手部、瞄准对齐或头显验收。

功能试验未通过：测试脚本先自动进入战役，再调用 `PlaygroundTestEnter`，状态在 100 秒观察期内保持 `travel_requested`。源码明确仅在 `WarPC_Menu` 分支发送 ClientTravel；该顺序无法进入测试地图。没有执行 hand／call 注册、观察器或 R2，Native 的 register／attach／admitted 计数均为零，因此没有真实握持或 0.7 倍伤害结果，也没有将其解释成 Native 卡死或已定位的渲染缺陷。

PG Shutdown0，PG 停止后 PE 原64字节恢复；Native 正常停止后独立读取 CF 原32字节及 PE 原64字节均匹配。Native cleanup／entry restoration 为真，XR exit0，hand callback／transaction／aux scope／sticky debt／pin／cookie／retained 均0。确切 owned game、launcher、XR、root 与音频 helper 已退出，9个音频会话恢复、pending0，原10份配置／存档 hash 不变。失败发生在 Native wrapper 基线采集前，不能用“与缺失基线比较为false”推断 wrapper 被改变，也未声称已证明该中间阶段。实际收据 d990fdc3…cf0d，31文件封存与限定汇总 e96184a4…91c9 位于忽略的 `analysis/weapon-call-runtime-1024bi-closeout/`；平台生命周期通过与功能失败分别记录。

BI-r4 仅修私有脚本顺序：不自动进入战役；启动器已启用 PG、仍在 Native 激活前等待 level 时，从主菜单提交一次 TestEnter。随后仍须真实 player／camera 就绪才允许 Native 激活，实际 hand 注册／R2／排空流程保持。Native／PG／XR 产物及 SafeBridge 字节不变，新一次性准备 d84494e2…da7e 已通过。测试窗口已明确归还掩体，等待其完成当前 AQ 后再协调一局，不并发启动；本条不预先声明 BI-r4 运行通过。

### 2026-10-24 — BI-r5 真实共享握持到 Lancer 返回值的有限验证

BI-r4 实际从主菜单进入测试地图，11个拾取物／6个目标就绪，原版 Lancer 的 GiveTo／装备／左手 IK 查询完成。随后启动器可选 right-stick dead-zone 策略在该现场 binding 上以 `Not a right-stick look-axis command` 拒绝并正常停机；无 hand 注册／开火，清理和原10份文件校验通过。具体 binding 字符串未独立采集，不能仅据此认定由测试场改写或归因给当前正常默认版本。BI-r5 只在私有 manifest／config 中不请求这项无关的可选修改，策略 guard 和普通启动器不变。

BI-r5 保持原 Native9sim、PG subscriber 与 XR5sim 产物，实际完成同槽位模型拿枪 status5、Native hand register accepted1／call attach accepted1，PG Start0 并获得 active／armed 状态258。随后只调用一次原版 R2 press 和匹配 release，总时长0.3048秒，原持键 Bool 依次为 true／false；没有直接调用 damage getter 或设置握持 fixture。真实 Native owner 的登记 cookie／incarnation 与四条 `native_shared_owner / fire_invocation / fixture_ignored=1` packet 一致，各自保留 owner frame、input sequence 和 world generation。

四条合格 getter 返回均完成一次 compare-exchange 和读回：三条12→8.399999618530273（`41400000→41066666`），一条20→14（`41a00000→41600000`）。后面三条复用同一 Result 地址，但对应独立 getter／row／scope-call，重新取得原版值20、12、12，没有按旧写入值反复乘0.7。实际 schema2 raw／context／writer 审计通过，Astra 有界复核 c2970a86…2a07 未发现推翻此限定结论的矛盾。

停止时 exact HandStop 首次返回 quiescent5，client off0，PG Shutdown0，PE 原64字节恢复且 Native CF detour 与基线相同；Native 停止后 CF 原32／PE 原64再次独立匹配。Native 已入场的37392个 begin／finish相等，最终 callback／transaction／aux scope／sticky debt／pin／cookie／retained 均0。Native cleanup／entry restoration 为真，XR exit0，确切 owned game／launcher／XR／root／音频 helper 已退出。实际 viewport1536×864符合每边0.4，物理后台输入为 original，无测试窗口遮挡；9个音频会话恢复、pending0，原10份配置／存档 hash 不变。测试窗口已明确归还掩体。

本次21项限定交叉检查通过，33文件封存及汇总4c9a3edd…c23b位于忽略的 `analysis/weapon-call-runtime-1024bi-r5-closeout/`。原 r5 的 `passed=false` 保留，汇总单独记录“共享 owner 到同调用存储已验证”，不改成完整功能验收。`shot_entry_runtime_verified=0`、审计器 `runtime_binary_provenance_verified=false` 及所有早期失败保持；4个 getter 不构成独立证实的4发，也没有测量最终敌人 HP／TakeDamage、投射物或延迟伤害。

拿枪来自真实游戏线程的模型提交，仍不是身体 RGrip；mount／副握几何尚未认证，不能外推 Two、手模型运动、动画交还、全武器或 Quest。该 writer 仍为 Lancer／私有 arena／显式 opt-in／64行的受控版本，未安装到默认启动器。下一步继续接投射物与多目标／延迟结算，并完成真实副握 mount、自由手动画交还及全武器交互；完整武器 6DoF 目标继续进行。

### 2026-10-24 — 手部组件局部字段读取合并

在默认关闭的 TAIL 候选中，同一次 mesh 读取的五个字段改为一次批量定位，读取后再核对 mesh class 和首个匹配的五个字段地址。每个值仍经过原类型、维度、大小、范围、单 bit mask 和 Vector／Rotator 布局检查；字段地址不跨本次非 VM 读取保存。前后身份检查、slot VM 次数、每个 setter 的最后准入、100 ms 原始源龄、0.1 原始单位／0.1° 精度要求及 rollback 路径保持。此项减少重复查找，尚无实际耗时收益或 AX 超限已解决的结论。

新增实际 helper 的 CPU seam 检查56项通过，既有字段检查207项及连续组件 lease 检查1326项通过；完整 TAIL PG 以 MSVC `/W4 /WX /EHsc` 编译成功，私有 DLL 为 a799ecbf…2591。TAIL 关闭时实际 phase1 文件的预处理结果与本次基线相同（仅规范换行和空行；不是完整 TU／ABI 证明），记录 b3f10658…db2f。Astra 有界复核没有发现需修复的保证丢失；CPU fixture 的 resolver／typed reader／schema 使用桩，真实元数据行为的保持来自源码对照，不能据检查数量推断运行验证。

证据位于忽略的 `analysis/hand-component-read-20261024bj/`、`analysis/hand-component-read-build-20261024bj/` 和 `analysis/hand-component-live-build-20261024bj/`。本阶段未运行游戏／XR、未改 Native／XR／默认启动器；掩体会话继续独占已经协调的实际测试窗口。后续自动测试继续使用每边0.4、静音、窗口置后和 tiny-output 只读的独立实例。拾取／近战的同步动画交还仍待独立 ActionGate 补充通道实现，现有 hand V1／call V1 保持不变。

### 2026-10-24 — 共享握持 observe 的 pawn 字段局部合并

单次 observe 中的 pawn 六字段改为同批解析：Controller／Mesh／Weapon／bDeleteMe／bSwitchingWeapons／bNoWeaponFiring。每次取值前核对对象、class、owner structure 和完整 Property，消费 binding 前重新匹配首个同名字段，再执行原有全部末次身份、结构、属性、链接、Bool 和 table 复读。保留派生层首个错误类型拒绝、Core 包、精确 ComponentProperty、维度／宽度、owner 与 pawn 双重 extent、链循环和容量限制；没有跨 prepare／commit／shot／其他 observe 缓存，没有更改注册、拿枪模型、100 ms 或写入授权。

包含实际 helper、原 selector 和生产 metadata Reader 的人工内存检查36类通过，原 hand client 检查21类通过，证据 d37650e6…71df 位于忽略的 `analysis/weapon-hands-transport-20261024bj/cpu-attempt2/`。root 使用冻结 BI subscriber 的13项开启选项编译完整当前 PG，私有 DLL 为7e237915…4395；实际 subscriber／client／shot／writer 的6个离线集成场景通过，包括 nested fire、复用 Result 和原 SEH 传播。它们使用人工内存及本地 Native Host 模型，不是新的跨 DLL／游戏运行结果；旧 BI-r5 工件和实际证据保持原样，也没有预先认定耗时收益。

完整构建位于忽略的 `analysis/weapon-hand-pawn-live-build-20261024bk/`。本次未运行游戏／XR、未安装默认产物。独立 ActionGate 的 draft wire／PG POD 语义已与 Native owner 对齐审查；未知 Scene／stereo 上下文的 NoAuth 通知需要独立无引擎撤销路径，不能复用有隐藏引擎调用的旧 before_action。独立 PE／bridge 前置许可和即时 cache 撤销仍待 Native provider 对齐；在覆盖和验证完成前，subscriber 与持续 STREAM／STOW 的组合继续禁止。

### 2026-10-24 — 动作交还传输与无引擎撤权路径

新增独立 `weapon_action_gate.h`（e8908d94…d361）及默认关闭的 PG Gate subscriber，保留原 hand V1／call V1 字节。Gate 在同一 hand／call 登记之后接入，使用相同 cookie／Native incarnation／context；128B journal 与 PG 本地事务覆盖 begin、原调用空档和 end，停止也须等待它们闭合。原 CallFunction hook／original／左手 owner 仍归 Native，PG 没有新增第二入口或仲裁器。目前 Gate 仅接原只读动画观察，尚未接引擎恢复，也没有实际 Native provider／跨 DLL 结果。

独立 `inspect／revoke_no_engine` 为不能调用引擎的入口提供撤权：GT 检查先于非原子状态读取，按 hands／stow 分别处理相关动作、恢复回声、dirty 和 pending；撤销旧 Plan／binding／readiness，保留未决 journal，禁止通过旧 before_action 间接调用 GetStateName／finish／yield。stow 回声不能掩盖 hands 的义务，真实 PE／CallFunction 不消费 component 的 owned bridge self-token。旧非 phase1 hand Backend 缺少在途纯写入 fence；related 仍只有 bool，pending 自然状态豁免仍需引擎查询。这些限制继续保留。

Gate 的14个离线场景通过，覆盖嵌套、原调用空档停止、错误身份／LIFO、正常容量耗尽、C++／SEH 故障、observer 无异常拒绝，以及清理计数不足后重试不得扣减其他新租约。源码复核发现并修正了 observer 的 void 返回无法证明 token 关闭、全局容量溢出误判、失败释放未记录 attempted 的问题；原动画观察仅新增本次 ledger 关闭的 bool 回执，旧 post／完成计数行为保留。Astra 对最终 subscriber 没有剩余 must-fix。

纯撤权 helper 的五配置人工 host 检查共56类通过；root 完整 TAIL PG 及其17类检查通过，仅将一个与 PG 全局变量冲突的形参改名。Gate 开／关的完整 PG 均以 `/W4 /WX /EHsc` 编译通过，关闭时既有 subscriber 的6个离线集成场景通过；完整 STREAM／STOW 与 subscriber 组合仍被实际 CMake 配置拒绝。保留首轮夹具骨矩阵错误、runner 负退出码识别问题和严格编译的变量遮蔽失败，不用后续通过覆盖它们。

限定汇总1c0b0dfc…c418及14场景记录d2bdea3e…0072e在忽略的 `analysis/weapon-action-gate-20261024bl/`；三个完整构建分别为 `analysis/weapon-action-gate-build-20261024bl/`、`analysis/weapon-action-gate-off-build-20261024bl/` 与 `analysis/weapon-action-handoff-full-build-20261024bl/`。最终 Gate 私有 DLL 为5e8b49cb…1a48。它们证明离线传输／撤权构件，不证明原动画接管或恢复、实际双手运动、Two、投射物伤害或头显验收。本阶段未运行游戏／XR、未改冻结 Native／XR 或默认启动器。

Native owner 正在补显式 Scene／stereo-lock 跟踪和无需加锁的撤销 generation，并审独立 PE／owned-bridge 前置 boundary scope；GT 或普通 Scope.depth 不能代替该许可。持续 STREAM／STOW 的兼容开关继续关闭，直到前置恢复、同帧 cache 撤销与实际拾取／近战／收枪交还齐备。完整武器 6DoF 目标继续进行，后续实际测试仍使用购买版的独立0.4比例、静音、置后实例。

### 2026-10-25 — 原恢复函数的结果回执与前置边界本地排空

hands 原 before_action 实现移到独立文件，并增加 checked 回执；旧 void 入口保持。回执在旧 Action 析构及其可能的 finish／drain 结束后判定，记录实际 restore／yield 返回值，不能因字段恰好干净而把失败判为成功。机械逆变换确认原动作过滤、引擎调用及其顺序未改，母文件除原位 include 外相同。实际生产函数被直接包含的两配置 CPU 检查共62类／60次 checked 与 void 轨迹状态比较通过，覆盖晚期析构新出现 dirty／fault／cleanup_failed／relinquished。

stow 同样保留旧 void 入口，新增 checked 与 pending 完成回执；取消分支仍仅调用一次原 yield_hands，Stop 不充当成功代理。已 Stop 但本次确实交还成功可以 clean；未关闭 pending、无效 slot、restore false 或残留 dirty／fault／visibility 不会假 clean。原 Hammerburst 自然 firing 状态边界保持独立 pending_state_exempt，不能折叠为“已取消 pending”。37类／74次原版、checked、void wrapper 的 host 轨迹与末态比较通过，机械替回后完整原文件相同。

复制 Native owner 的前置 boundary 草案6dac28b3…6709到公共源码，保持它是未冻结、未启用的草案，hand／call／Gate V1 不变。默认关闭的 boundary client 只接导出预检查、同登记绑定及本地生命周期。单原子的 closing-bit＋count 在 worker 停止时先封门；即使 Native Open 没授 scope，已进入的 PG 调用也阻止 HandClient 清空 GT 状态和 FreeLibrary(Native)。Entry 地址／代际防复制及重连混用，清理 attempted 防重复扣减。8类本地 admission 检查和实际 PG stop 路径的 healthy／faulted 两种 fixture 通过：count0不抹掉 sticky fault；内部 GT 拒绝不会封门。原公开停止入口本已拒绝 GT，此处属于内部 helper 硬化。

完整 checked-hands TAIL PG 和 boundary-lifetime PG 均以 `/W4 /WX /EHsc` 编译通过，分别为f7d42842…2aa与68cb312a…af43。后者尚无实际 PE／bridge Open／Close 调用或恢复阶段；生命周期 fixture 使用本地 API 回执与持有的系统模块引用，不是 Native provider 验证。Astra 对目前 admission／client／stop 的限定源码复核没有剩余 must-fix。

证据位于忽略的 `analysis/weapon-action-restore-20261024bm/`、`analysis/weapon-hands-transport-20261024bj/stow-checked/` 和两个 `analysis/*-build-20261025bm/` 目录；限定汇总为 `result-r2.json`（442a77a7…a163）。原汇总仅将包含草案的 header pin 字典误命名为 frozen_headers，保留原件并用新汇总明确更正，没有新增冻结基线。恢复阶段仍须保持 rollback：不能提前用 NoAuth relinquish，也不能预增 stow epoch。还需显式 finish、不可被后续 finalize 清除的阶段故障门槛，以及 Open 前地址稳定的 State512 pool；不确定 Open／Close 必须保留原槽位。上述接线、Native provider、真实动画交还和双手运动仍待完成，持续 STREAM／STOW 组合继续禁止。本轮未启动游戏／XR，未改默认 Native／XR／启动器。

### 2026-10-25 — 固定边界存储、单次恢复阶段与导出传输

新增地址稳定的128槽 State512 pool。每槽保留本次 Request、Open 输出、冻结 Receipt 和独立 Close 输入，票据绑定自身地址及不回绕代际；不确定 Open／Close 隔离原槽，不重置、不复用。正常 NoScope 只接受约定的五种返回，并核对零 State、无 active／engine 标志和无 scope。Native 后来撤销的全局 generation 不会替换本次 Open 的冻结回执。公共 boundary ABI 继续使用 owner 固定的6dac28b3…6709，既有 hand／call／Gate V1 字节不变；这没有建立新默认冻结基线。

恢复协调器记录本次单次消耗，显式执行 hands → finish → stow → finish。具体 adapter 调用已有 checked helper，保留精确 pending_state_exempt；先撤销前向计划，但不提前 relinquish rollback，也不预增 stow epoch。原 Action 析构在外层 hold 下不触发恢复。阶段内部的 C++／SEH 故障只归还公开词法守卫，不清掉私有 busy／restoring／dirty，并留下独立的永久故障门槛。后续写入、reset、正常 finalize 和 client 停止都不能用表面 clean 覆盖它。嵌套 poison 后，两个显式 finish 在调用旧 helper 前再次检查健康状态。

新增导出 transport：取得本地 Admission 后才能读取 API／cookie／context；先 Revoke，再按需 Open。无论是否获得 Native scope，本地 Entry 都跨越原版调用间隙，直到终结通知／Close 完成。未知导出历史、异常、回执损坏和失败 Close 先隔离／quarantine，再释放本地 Entry；正常重复 finish 不重试导出或重复扣减。未返回至完成位的 Revoke／Open 不能被 caller 的完整 Finish 假报 clean。该 transport 尚未挂入实际 PE／owned-bridge 入口，尚未证明同一 active cell 与唯一实际 stage 的入口绑定。

CPU 检查分开记录：pool22类；协调器12类；具体 adapter21个独立进程、37个外层场景；最终 transport38个进程场景，另跑实际 PG client 生命周期3种与 pool，可复算汇总共42个作业。最后补充了旧 NoScope entry 仍在原版间隙、新 entry 已复用同槽的场景：旧 finish／重复 finish 不触碰新代际，worker 停止也不能提前释放模块。adapter 的旧 helper／grant／client 是明确的 host seam；transport 的 Native API／原版 body／成功 Finish 是 host seam。实际 client stop 路径执行过，但 Native provider、真实动画、引擎控制器和 GPU／头显均不在这些 CPU 结论内。

完整 TAIL adapter PG 与独立 boundary PG 均以 `/W4 /WX /EHsc` 编译通过，分别为697bf4cf…19bf和18150ff8…79ca。Astra 对 coordinator／具体 adapter／故障门槛和最终 transport 的限定源码复核无剩余 must-fix；其后的测试源码只增加上述 NoScope 复用案例，生产代码未变。旧 review、历史 CPU 尝试与首两次 C4459 构建失败日志保留。最终 transport作业回执为ebbf61dd…2905，具体 adapter 的 CMake 回执为a5828698…c574；证据位于忽略的 `analysis/weapon-boundary-transport-20261025bn/`、`analysis/weapon-restore-stage-20261025bn/` 及对应构建目录。

本轮仍未启动游戏／XR，未改 Native／XR／默认启动器或启用 STREAM／STOW＋subscriber 组合。下一步是实际入口的递归 NoAuth 分流、初始 owned self token 的独立消费、POD finally 覆盖原版间隙，以及 Native provider 联调；真实自由双手与拾取／近战／收枪的动画交还继续待验。后续实际测试继续每边0.4、静音、窗口置后、tiny-output只读并禁用实体手柄后台输入；保持最新 support3 Native／menu-modal1 XR 与其他会话修复。

### 2026-10-25 — 实际动作入口接线、递归撤权与异常闭合

新增默认关闭的 ENTRY 试验选项，把 PE 和 owned invoke 接到实际 invocation-local controller；continuous forward 的源码入口也接到同一路由，最终 authorize_once 与 setter 仍紧邻。相关 PE 在旧 scope 构造前完成许可判定及一次恢复，随后裸转发原版；递归 PE 包括无关 Tick 直接走原版，不重复进入旧 pre／post／队列工作。真实 CF 的 whole-gap 拒绝标记复用已有 Gate linked depth，相关 CF 仅撤权、不新开恢复 scope。NoAuth 不调用旧 Action／hold／观察器析构来暗中恢复。

owned 初始组件 token 在任何 Revoke 前单次消费；收枪新增绑定 exact Request 地址、object、function 的初始 token，在最终 Args::run 前才武装。消费只豁免对应 participant，不豁免另一侧。mutating／releasing 保留为观察信息，不能单独抹掉动作义务。backend 的 Request／Receipt 副本必须与 active pool cell 一致，begin 还核对真实 active_stage 地址；换一个 Journal 无法复用父 stage。

全部相关 PE／bridge／CF 在 Native Revoke 前先以 context_lost 撤销本地 HandClient publication；Native generation 与 PG published 两条撤权分别闭合。已消费 token 之后才采样 lexical Snapshot。准备阶段的自有 C++／SEH 屏障不包住原版，准备失败仍转发真实 PE；原版异常类型、code 和 opaque 参数继续传播。private cleanup、已捕获 TLS 恢复、transport retirement 与 callback 释放分开闭合；cleanup 自身故障保留固定 State、隔离通道并精确退还本地 lease，不覆盖原异常，不补造正常 post。

本次 ENTRY 运行配置仍明确排除 STREAM／STOW。旧 normal_tick／PostTick 的引擎恢复没有获得整段 Tick 的新许可，ExternalBoundary 仍只授权本次 pre-action 恢复；不能把 Native cached authorized 状态当安全执行点。该禁止组合的 CMake 负例按预期拒绝。连续双手真实写入、实际恢复 helper 与新 Native provider 联调尚未通过，因此候选未集成默认启动器。

完整 ENTRY PG 以 `/W4 /WX /EHsc` 编译通过，SHA 为b74c8fed…8462。实际 controller／无 participant backend／真实 client、subscribers、transport 的39个 host 场景通过；包含 cleanup SEH、原版异常、CF整段递归、不同实际 Journal 及被改过的 backend 副本。另有28个 stage／active-cell 绑定场景。local client prepare／commit 真正发布后，clean NoScope／CFrelated 在 Native Revoke 调用前已撤 publication；旧 snapshot 尚未超过100ms也被拒绝。旧 helper、Native API、original／legacy 和游戏对象分类仍分别是明确的 host seam，不能据此认定真实动画交还。

初始收枪 token 使用实际源码1018字节精确片段，五配置的61个独立进程通过；foreign-thread 的 PAGE_NOACCESS Request 在解引用前拒绝。实际纯 handoff 的同时 dirty／pending／fault 状态保持，engine_reads／engine_writes／logs 均0。旧 handoff 测试同步修正宽泛回声豁免期望，原五配置56类通过。最终 CMake 复算117个作业通过，包含入口39、绑定28、完整纯交接16、core12、adapter21和旧完整handoff一个作业；adapter与handoff各自的外层case计数单独保留，不相加充当游戏覆盖。Astra对最终生产源码限定复核无剩余 must-fix；之后仅增加了测试目标／精确片段生成及选项说明。

证据位于忽略的 `analysis/weapon-entry-routes-20261025bo/`、`analysis/weapon-entry-binding-20261025bo/`、`analysis/weapon-entry-initial-handoff-20261025bo/`。回归编译误复用了BN私有构建目录，该目录中的未发布PG DLL现在对应BO源码，历史BN哈希不能再按该路径当前文件复核；原manifest与8c1e9bc源码保留，本轮另行封存BO产物和此差异。冻结support3 Native／menu-modal1 XR已分别复核为4752801a…ae14／37f46312…a4d，未改动。未启动游戏／XR／RE；后续与Native16模拟器候选配对须继续独立0.4、静音、置后和observe-only，完整武器目标继续进行。

### 2026-10-25 — 按用户要求优先完成双方共用的自由双手

用户明确要求先实现双手自由移动，掩体会话也需要同一输入。执行优先级据此调整：先打通同一Scene的左右手追踪、完整世界变换、回中／转向及epoch，再用实际双手骨骼位移证明独立跟手和动画后重新接管；武器与掩体共用该结果。b16cfbe入口候选保留，新的writerless boundary互通探针后置；完整12武器、换弹、瞄准镜、电锯、投掷、UI等目标没有缩减。

源审确认旧GHW1世界转换与Cover点映射在相同基准下相容；正确基准是双眼实际采用的最终pre-HMD base_view，不能一概换成原source_view。Cover当前Scene已冻结一次matched controllers并选出最终view，但没有旧GHW1发布段。双方协调由Native唯一owner发布同票RigTracking数据，明确原始输入／reference／source times、world/context identities、最终基准、units与失效纪元；数据不能充当骨架setter许可，不改旧hand V1，不另读一次“更新”的输入或重盖时间戳。

AX的0 movementcommands已定位为试验顺序问题：runner先等枪体joint receipt，初次等待失败，未到第一条手部命令。现有clean waiting的hand_only路径允许先验证左右手各4cm纯平移及回位，之后再验动画重入；实际手骨世界矩阵、另一只手不串动和原有0.1单位／0.1度、100ms限制继续保留。RuntimeTracked0与MetaOperatorSimulated1必须按实际来源分别核对，模拟器身份不等于certificate source1。Support3标准冻结本身没有GHW1 producer，后续需要最小、可核对的共源发布候选，不能直接复用AX旧renderer或其scale1配置。当前仍在离线准备，运行窗口已留给Cover完成AR14；所有实际试验继续0.4、静音、置后和observe-only。

### 2026-10-25 — 独立手部运动候选，先于枪体接管发送手部命令

新增默认关闭的 `GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL`。该私有配置保留原 TAIL、certified、persistent hand-only 依赖、304字节 Start、原15秒期限和逐次 Plan／对象／时效／journal 检查，固定不进入枪体 acquire／update。normal_tick 先执行原手部恢复并退出 Operation，再要求 clean waiting 或带成功 drain 证明的 clean suspended；不恢复枪体区间，也不清除 closed、drain 或故障记录。普通配置继续原路径。编译时来源只能明确选择 RuntimeTracked0／MetaOperatorSimulated1，请求还需与相同的单字符来源环境值对应；不借可选 R2 模拟器开关决定手部来源。

Astra 对三文件限定源码复核无剩余 must-fix。新的 `analysis/hand-independent-motion-20261025bp/root-builds/attempt1/` 独立目录完成 source0、source1 和 feature-off TAIL 三种完整 PG 的 `/W4 /WX /EHsc` 构建，分别为83e90e24…82b1、ee61f1d0…c807、58371cf5…b854；没有复用或重建此前封存候选目录。这只是编译结果，未运行游戏／XR，尚无四条实际移动命令或动画恢复的运行收据。

新增 `test_hand_independent_motion_profile.py`，机械抽取实际 normal_tick、hand-only readiness、模式选择与 component dispatch guard；三种严格 CL 配置、15个独立进程通过。两个来源下均验证 clean suspended 连续两次手 journal acquire／restore，component resume／apply／acquire／update 均0，原 closed／成功 drain 记录保持；dirty／failed-drain／revoked 拒绝且保留原记录。实际 Policy 的100ms边界、任一原始来源超时1ns、MS期限及错source拒绝保持。完整结果为 `lifecycle-cpu-attempt5/result.json`，此前宿主构建和 fixture 构造失败分别保留；引擎 restore／finish／end 是明确 host stub，纯 primary journal 不证明 tail writer、实际手骨或游戏动画。

与 Cover 协调的共享 `rig_tracking_math.h` 已按 d52efa9d…7502 原字节采用，使用最终 pre-HMD base_view；完整姿态输出是 UE world forward/right/up，不能重复做 XR 轴转换。Native 的新只读 Rig 帧仍在准备，原始追踪数据本身不能冒充旧 GHW1 certificate；现有 XR companion 仅提供实际 stream／tracking 纪元，旧 reference_epoch 则来自 Native StereoGate 的独立连续性计数。后续须以匹配当前冻结渲染来源的真实 producer 候选接入，缺失信息保持不可用。本条未改变默认 Native／XR／启动器，也不代表自由双手完成。

### 2026-10-25 — 共用 Rig 读取、最新 XR 同源输入与独立试验收尾

独立手部核心已提交为 `eda686d`。按 Native owner 交付的58cbb26f…76b0原字节采用 `rig_tracking_frame.h`：真实 producer stream／逐手 tracking 与实际 NativeReference 分开，Frame1824、ReadRequest40。Root 在自己的依赖下严格编译并核对全部给定 offset／align8；新只读 `rig_tracking_reader.h` 的18个 fake-provider 检查通过，验证 exact NativeKey／source／Scene 下界、读后时钟、原始双来源时效以及缺 producer 拒绝。此 helper 只复制数据，尚未接真实 Native 调用，不授予骨架写入许可。typed RigPolicy／完整 receipt 的前向再授权接线仍在进行，不从 raw Rig 拼造旧 GHW1 certificate，也不增加新的 GHW1 publisher。

XR 改动从准确 menu-modal1 源另建私有副本：provenance为c089c4d8…390e，586个原文件和两份封存 JSON 构建后均保持原 hash。normal 候选只增加7个文件／13个 hunk 的 GVC3＋GHC1 发布接线；同一原始 actions sample 及时间先进入实际 SourceTracker／两桥，再尝试发布 eyes。未发布、失追、失焦均保留真实源历史，不二次采样，不增加 producer reference。原颜色、菜单修复与原 WAIT ON／SIXDOF OFF／HDR OFF／编译 flags 保留。四个原菜单 CPU 和三个严格桥／实际 loop 抽取 CPU 通过。normal XR为4174d0e3…1ead，manifest为056682a7…6b4，位于忽略的 `analysis/hand-xr-continuity-20261025bq/`；额外完整 `/WX` 试验的四处原 SDK C4459 失败另行保留，正式候选按母体原 flags 构建。它尚未运行，Source1sim 是独立后续候选，不能重标这份 normal。

新的 `hand_independent_motion_run.py` 默认 UNBOUND，在项目 helper／prepare 前拒绝。四条手部命令先于任何枪体 joint 等待；即时 typed control／tail 字节、后续 Tick 的实际 pre_restore 手骨、restore 回执分别核对，缺同 lease 的 post-restore 世界手骨仍 pending。复核发现的晚到 reservation child 和 raw Start 在途参数释放问题已修：先收拢严格身份的原 worker／birth，再清理 owned game；实际 retained adapter 分别保留 request、Submit／export 原线程和 GT 结果，未知时不重派发、不 free、不卸载 Native，直到原线程或确切进程实例退出。修后限定源码复核无剩余 must-fix；34个 runner 离线场景和22个 fake Win32／thread adapter 场景通过，分别保留在 `runner-final-attempt5/` 与 `bridge-cpu-attempt2/`，不是实际进程／DLL／游戏收尾证明。

本节没有启动游戏／XR／SDK session，未改冻结默认、原已发布包或其他会话 checkout。Native18b／新XR配对、真实左右手4厘米运动、完整旋转与大伸手、动画交还及头显验收仍待完成；后续自动测试继续每边0.4、静音、置后、tiny-output只读和关闭物理手柄后台输入。

### 2026-10-25 — typed Rig 手部接线完成编译，执行阶段仍待合流

新增独立 `RigPolicy/RigPlan`，完整1824字节 Native Frame进入 Plan；Scene publication 与 raw XR sequence 独立检查，首帧 baseline、新 Scene 同 raw 不重复写入、逐手 loss、实际 NativeReference 与完整身份／source／cookie／epoch／base 约束保持。两份原始 source 时间按100ms严格授权，局部 receipt 的 MS 时间仅作诊断，不伪称 Native uptime。新策略1261项检查／18组场景、旧 stream25组／certified12组回归通过，旧 policy 源未修改。

PG 私有 Rig 分支已从真实 Native getter 读取，完整 typed Plan贯穿 tail backend，写前及 written 后均重新取得 live Rig，并在该读取返回后重读本地 action／对象／输入代际。恢复不依赖 fresh Rig。原配置8组／16份选中代码 token 等价检查通过，实际 helper全文及授权路径提取146项检查／15组 host场景通过。新增数据 getter 的 closing-bit/count 生命周期与24字节 owned Connect capsule分别复核；它们保留真实模块与登记，Connect不加载 Native，不改变原 Start304。

稳定源码的 `full-pg-attempt2/result.json` 中 source0／source1／Rig-off TAIL 三个完整 `/W4 /WX /EHsc` 构建通过且 source pins 全程不变。attempt1 三个编译也 exit0，但代理在构建窗口补了 GT 和读后 action 复核，因此一致性检查按原样保留 `passed=false`，不将其当封存候选。Native18b采用 owner 的不可覆盖 `native-body18b-ready-semantic-final1.json`，943fea2b…c304；首次 adb69…旧元数据 pin 拒绝及原件保留，两个 DLL未变。新XR Source1sim为1a059d2d…b4e9、manifest c189bcca…efa80；实际 operator模式、0.4与三个后台环境门槛明确，normal与menu-modal1母体保持原 hash。Root本轮没有启动游戏／XR。

**这些 PG 目前仍只是编译与限定 CPU 候选，不能按 Native18b 互通 READY 使用。** 继续源码核对发现原 handanim CF hook 承担同步 hands/stow 交还，不能只迁移其观察或假置 reserved。Native18b是唯一 CF owner；现 CallV1禁止该同步恢复，CMake的 subscriber＋stream/stow 以及 ENTRY＋normal Tick禁配继续保留。现 Native ActionGate已有受限恢复权限，PG还需接真实 Gate-bound恢复stage；正常 Tick写入也需要 Native实际安全阶段，RigRead和原PE返回均不授这项许可。双方正比较实际阶段接线与 Native最终骨架执行路径，既不安装第二CF hook，也不把四条idle命令当完整交还证明。审查回执为 `pg-call-owner-coexistence-review.json`（ed078cdf…1b17）。完整自由双手、与掩体的真实运行和动画交还仍未完成。

独立脚本随后完成实际 RigV1 recipe：固定真实 MSVC layout／header，完整1824字节 sidecar 与 pose、Native/game/XR、五对象及原始双 source 时间交叉核对；四条4厘米命令、原残差和后续 pre_restore 骨骼检查保留。Connect从同一确切游戏 handle 枚举已加载 Native，按精确路径发送一次 owned24请求，使用原 startup期限；Hand15秒仍从HandStart前开始。新脚本737fbe7c…39a5与旧 retained adapter源码经限定复核无 must-fix，85个离线场景通过，证据位于 `runner-rigv1-attempt7/`。三项 `sole_native_cf_owner/cf_handoff_ready/normal_frame_stage_ready` 默认false，真实 pinned source manifest也须逐项证明；当前编译候选不满足，仍在任何artifact／SDK导入前拒绝运行。该 preflight声明不是运行时 Grant，尚无实际4厘米移动或最终世界手骨恢复证明。

### 2026-10-25 — 自由双手优先，共用 Native 骨架执行方向

依用户最新要求，自由双手先于依赖它的武器与掩体交互。双方选择 Native 的 C1 最终骨架工作区，由唯一 owner 按身体、手臂与武器顺序计算和发布。只迁移旧 PG 观察入口无法解决同步动画交还，因此不再为它新增 normal Tick 前向授权；现有 subscriber＋stream/stow、ENTRY＋normal Tick 禁配继续保留。C1 工作区的实际发布与消费、下游物理／附件关系和动画恢复仍在由 Native owner 核对，尚未宣布可运行的最终骨架 writer。

新增共用 `hand_two_bone_pose.h` 与实际 header 测试，分别固定为015d06ac…922f与2aabe8d4…4d67。输入是同一坐标空间的肩、肘、腕原姿态及完整腕目标，不包含骨号、引擎调用或写入许可。保留长度模式对不可达目标明确失败；自适应模式调整两段长度并精确保持腕目标，原 packed Atom scale 由 Native adapter 保留。完整腕旋转保留 roll；上臂／前臂按原始骨轴施加摆动旋转，不假定模型骨轴。关节极限、碰撞和奇异姿态的唯一解不在这份纯数学能力内。

严格 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译与16组 CPU 场景通过，涵盖内外可达边界、伸长／折叠、镜像、128次刚体变换的三类场景、极端单位量级、完整腕 roll、4厘米平移及回位、失败输出不变。最终回执为忽略的 `analysis/hand-two-bone-20261025bs/attempt2/result.json`，较早尝试原样保留。这是数学验证；实际角色手骨、武器附件、最终骨架发布与 GPU／头显验收均未完成。当前不改默认 Native／XR 或普通启动器。

### 2026-10-25 — Native Gate 绑定的单次恢复实验

新增默认关闭的 `GEARS_PLAYGROUND_WEAPON_GATE_RESTORE_TRIAL`。实际 Native Gate.begin journal 链接后、observer 与原版调用之前，adapter 核对完整 Invocation、Native State 地址、GT、cookie／Native 实例、当前 Entry、backend 与 stage 地址；在第一个 helper 前持久记录一次性 claim。重复、拷贝、嵌套、原版间隙或返回后的尝试不能重放恢复。沿用现有 hands → finish → stow → finish／verdict／POD cleanup，撤销前向计划但保留私有 rollback；不生成 external Open 回执，不授 normal Tick／前向写入权限。State128、hand／call／Gate V1 和原版调用位置保持。

38个独立 host 进程场景、1225项检查通过，旧14场景回归及两份旧文件各三种 OFF 预处理输出去空白一致。完整 GateON／GateOFF PG 均以 `/W4 /WX /EHsc` 构建通过，DLL分别为1f981675…37ec与98733010…7c66，八份 source pins 和实际 DLL hash 在 Root 复核时均匹配。限定独立源码复核未发现新增 P0／P1。证据位于忽略的 `analysis/weapon-gate-restore-20261025bs/`，失败尝试与日志保留。

这些 full DLL 仍未包含 STREAM／STOW，配置禁配保持；实际 helper、Native 锁／阶段及 provider 在 CPU 场景中是 host seam。此项仍是恢复实验代码，不是自由双手、真实动画交还或运行 READY。未启动这两个 DLL 的游戏测试，也未改默认候选。

同轮首次私有只读骨名采样 `hand-reference-names-20261025bt` 在任何 game birth／Native session 前被 launcher 的 playground artifact 路径检查拒绝，未发送 Pose event。原10文件 hash 相同，事后 Game／XR／launcher worker／8720 均不存在；缺少 session 的 finalizer 如实保留 accepted=false，不能当 Native／entry／audio 清理通过。原因是派生 manifest 将要求相对路径的 playground_build 改成绝对路径，新的私有 bu case 将恢复正确精确 origin，额外离线核对实际 loader 与 launcher，保留 bt 全部失败原件。

### 2026-10-25 — 同局骨名与真实父链采样通过

bu 修正 PG 精确 origin 后仍在 game birth 前拒绝：私有 PowerShell 直接启动共享 `graphics_policy.py`，Python 的脚本目录优先级使它导入了旧 instance policy。失败的 bu 与 bt 均完整保留。新的 bv 用固定12个 target 的私有入口先载入严格私有 context，再保留实际脚本路径、参数、退出和清理行为；共享规则未改。全局配置修改、物理后台输入和 instance prepare 的入口仅允许离线 probe，实际路线不可执行。12个 fresh-child namespace probe、25个 Python 编译、实际 PG／driver 配置和 PowerShell 检查通过，限定独立复核未发现 P0／P1。

随后 bv 在购买版与 Source1 模拟器中完成一次实际只读采样。前后115个 FName 的 index／number／名称、完整 opaque RefSkeleton 原始字节及同一 game／owner／XR／Native session／mesh／asset 身份一致；本次唯一 PoseEpisode 的 C0／C1／Plate0／Plate1／L1 均与之匹配，父链只取该局实际 C1 parser 输出。已能确认当前模型左右真实手臂的直接父链，未将 IK helper 当作真实手骨，也不将本局骨号推广到其他模型。Native 二进制594792字节，SHA b5098d75…9e57；join为00511f9a…245e，final为fe2b7dad…3d91。完整原始数据仅保留在忽略的 `analysis/hand-reference-names-20261025bv/`。

该局 `runtime-final.json` accepted=true：Native 正常停止、入口及88项独立核对、8份完整 pose chunks 还原通过；XR exit0；原10文件 hash 相同；匹配的音频10次尝试均恢复、pending0；PG shutdown0／entry restored／stopped；tiny monitor joined；确切 owned PIDs为空。host较早记录的 `no_remaining_games=false` 原样保留，后续独立 finalizer 与全局 Game／XR／8720 查询均已为空，没有回写旧瞬时结果。

测试保持每边0.4、静音、窗口置后、tiny-output只读和物理后台手柄输入关闭。Body／Rig／hand writer 与 engine restore 全部关闭；本次是骨名、父链和生命周期证据，没有控制器运动命令，也没有新增骨架／游戏字段 writer。已向掩体 Native owner 交付，继续用于共用 kernel 与真实 Atom／父链适配；自由双手运动、动画交还、最终发布／物理／附件与头显验收仍未完成。

### 2026-10-25 — 本局真实手臂链的纯缓冲区求解验证

将上述 bv 的同局 C1 原始 Atom、父链、component 和已认证骨名作为输入，只复制唯一 shared kernel 与 Native owner 的纯缓冲区 adapter，未改双方生产源码。当前 session 的 requested configuration、实际 Native command／config／status共同确认100 units／metre。左右各自4厘米平移／回位、90度腕轴旋转及多轴完整旋转共8个正向场景，以及两侧 preserve 不可达拒绝通过；实际 header 以 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译运行均 exit0，12份原输入、源与 header 副本前后相同。

float Atom 打包后经实际 FK，目标腕位置最大误差3.289601e-5原生单位，旋转最大1.707830e-6弧度；每步保留115个 scale 字节、112个非链 Atom 及另一手 world 字节，19个相关后代的纯 FK 重组通过。输入、源码、依赖、结果和首轮记录保留于忽略的 `analysis/hand-arm-actual-20261025bw/`；最终 `result-attempt2.json` 为61f067c1…8659，已交 Native owner。

这些目标是 Native wrist pose，未把 Rig world_grip 旋转直接充作腕骨；grip→wrist 校准仍须真实接入。八个4厘米目标均未触发长度调整，因此没有新增大伸手模型验收。回位只表示腕目标位置／旋转回位，量化后链上的三个 Atom 不保证原字节；这不能清除运行时 rollback 债务或替代保存原始 preimage。本节没有启动 Game／XR／SDK，也不证明真实骨架写入、模型变形、最终渲染／物理／附件或头显效果。下一步由 Native owner 的默认关闭、有限次数 C1 overlay 验证实际写入与恢复，仍保持唯一执行端和18b／默认冻结版不变。

### 2026-10-25 — 原版合成变化回归与控制器到腕骨变换

核对 Native owner 的未接 live 草案时，发现其 C1／排债 L1 要求完整 C0 Local 与 C1 Local 相同。重新按 bv 的实际 parser 范围和 hash 比较，原版恰会修改左右两条真实臂链的六个 Atom，包括 quaternion 和少量 translation。原 host 的 compose 只更新 Space，没有覆盖这个实际性质。Root 对稳定私有 c2ecc4c3…259e 源快照补了合法原版 Local 变化回归：严格 CL 编译0，功能期望 applied，实际返回 context、writer_calls0，测试 exit9／functional_expectation_met=false 如实保留，未启动实机写入。证据为忽略的 `analysis/hand-c1-overlay-review-20261025bx/`。

Astra 的限定审查另指出 C0 部分归还后的 consumer 安全、失败后的 phase 可恢复性、Space journal 提前退休，以及 desired 数组／实际控制存储的别名保护问题。审计 ebc88d1f…71b7 与真实六骨变化、失败回归已交 Native owner；其正在分离 C0 输入、原版 C1 preimage、overlay after 和归还来源，并补对应 fixture。此处是对指定未冻结草案的发现，不是新 Native 候选的通过回执。Scope flags、C1／L1 样本和 host seam 均不授实际写入权限，仍待真实入口、恢复及 consumer 分支接线；18b、默认 Native／XR 和普通启动器不变。

并行新增 `hand_wrist_calibration.h` 及实际 header 测试，固定为d73d075c…d224与bcefadc2…dabe。`Calibration` 默认 unknown，保存米制 grip-local 偏移与完整相对 quaternion。`from_caller_aligned_world_pair` 只在调用者已另证对齐的前提下计算相对变换，`native_wrist_target` 由同 UE 世界 grip、单位倍率和已知 offset 产生完整腕目标；平移只换算一次，完整 roll 保留，失败不修改输出。known仅表示数学校准存在，不证明物理对齐、身份／时效或 Native 权限；旧临时 binding、solver、Rig、PG、Native 与 CMake 未改。

严格 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译和10组 CPU 检查通过，涵盖单位、90度及多轴旋转、完整 roll、回中／原位输出、世界刚体变换协变、左右独立 offset、合法 q 归一化、未知／非法／溢出输出不变。实际编译依赖、五份源及快照前后 hash 相同；最终回执位于忽略的 `analysis/hand-wrist-calibration-20261025by/attempt1/result.json`，c078001a…9810。实际标定流程、持久化和 live 连续跟手仍未实现。本节未启动任何 Game／XR／SDK／IDA，也未改其他会话源码。

### 2026-10-25 — 原生控制输出路线与独立追踪数据核心

Native owner 的 C3 私有修订已移除 C0／C1 相等假设，并改进部分失败、blocked 阶段及已知别名处理。Root固定六份源码，重新从本局 bv 提取完整 C0／C1／L1 原始 Snapshot，用原 C3 header／test 严格编译运行，8455项／43场景通过，16份源码／输入前后相同。证据位于忽略的 `analysis/hand-c1-overlay-review-20261025bz/bv-paired-attempt1/`，result为6a98b4f3…12ee7。实际执行的 IO／GT／Request／proofs仍是 host seam，paired overlay只验证默认左链。Astra继续指出 Local 任意分量变化即整字段退债，以及 Space 仅凭 different-after 判完整原生生产的未闭 P1，审计775c53f4…8d8c保留。C3不接 live，不因这份 CPU 回执变成写入 READY。

后续主路线改为复用原生手臂控制的临时输出，由原 Compose 负责正式骨架和后续传播。Native owner 的新 RO4 前置独立窗口取得12个控制节点，实际手链为活动的 Limb／SingleBone 与强度0的尾节点；旧 PG 四通道恰选尾节点，不能复用其强启用结果作为新路线证明。Root已读原始前后／final：前置 accepted=true，后置因观察期间值改变拒绝，整轮 accepted=false、cleanup verified=true，未拼成原子 Pose 或提升执行权限。原生节点实现、目标空间、完整旋转、最大伸手限制与临时输出传播由唯一 Native RE owner 继续核对；Root未运行新游戏或使用 IDA。

并行源码检查确认当前 Rig producer 在发布前后已依赖真实 hand 注册，不是只在 public read 上检查 cookie。现有同次 Scene 的 TrackingBundle／tracking_for、最终 pre-HMD base 与原始来源／reference 时间可以复用，但尚无供内部节点使用的独立完整快照入口。Root据此新增 `native_rig_tracking_frame.h`／实际测试，NativeSessionKey为24字节、无 cookie，CoreSnapshot为独立1832字节／align8；source_ticket与native_reference_revision仅作内部形状检查，current核对由 Native 外层实施。原 V1 Frame1824／ReadRequest40及全部成员布局保持，公共验证仍要求非零真实注册。

公共 `rig_tracking_frame.h` 只抽取同一份 payload validator及按成员原字节拷贝 helper；Core与public共用原数据策略。`project_for_authenticated_registration` 仅在调用者已另证真实 broker／receipt 后按同 Native session 投影，再跑原 public validator；不造 cookie／producer／时间／sequence，不授调用权限。默认50毫秒和100毫秒上限、head-only及无 companion 的原数据语义保留；手部 consumer仍需实际 producer、各手mask、当前 ticket／reference／stop和安全阶段。

最终两个生产 header 固定为d6b295eb…c2b2（Core）、af103284…d684（public）；测试83f2f7d8…b8cf。严格 CL actual headers／冻结旧 V1检查通过：Core8组204581项，含58484份 Frame及328份 Request行为差分、48220份合法变异的完整 V1输出字节比较；原 RigPolicy1261项／18场景回归通过。布局、原声明／request_valid原文、payload抽取、17份源码／依赖及11份只读输入均核对，attempt1原件保留；最终回执为忽略的 `analysis/hand-native-rig-core-20261025cb/attempt2/result.json`，f45580fd…fa35。Astra限定复核无 P0／P1，37a8296e…ebac。

本轮没有实现 Native store／Scene builder／getter／latch，也未修改其他会话 live／capture／shared源码、运行 Native DLL 或启动 Game／XR／SDK。数据模块已交 Native owner 接线，仍需真实原生临时输出、控制器持续跟手、动画交还、全部角色／装备状态与头显验证；默认冻结 renderer／启动器和18b保持不变。

### 2026-10-25 — 原生分支接收数据核心，准备控制输出解析

Native owner 已按最终三文件 hash 接收 Core。首次接收检查因其本地 `stereo_frame.h` 与原 CPU 依赖不同而正确拒绝，失败保留；后续明确重新用实际本地 headers 与冻结旧 V1 编译，而非复用 Root CPU 结论，8组204581项检查通过，实际依赖与三份源码前后相同。Root已读取 `native-rig-core-receiver2/actual-local-result.json`，passed=true，回执a490c99d…e488e；这只证明另一组实际依赖下的数据层接收，Native发布／缓存／getter／latch仍未接入。

下一观测候选由 Native owner 以冻结18b的最小增量准备，默认关闭，记录原生控制返回的临时矩阵、权重及相关状态；不在该轮混入 Core live 迁移或姿态 writer。Root初读了 register-only thunk／protected bridge，观测后仍调用原方法并在 finally 记录和清理；候选尚未冻结，因此不以本次初读作为最终ABI／运行通过结论。Root本轮没有启动游戏、XR或IDA，也未修改其他会话源码。

Root另交付了离线解析所需的MSVC布局字段清单，位于忽略的 `analysis/hand-native-control-output-20261025cc/plan-draft1/required-layout.json`，89e46f0c…1939c。它明确为DRAFT_NOT_FROZEN：所有size／align／offset仍为null，尚未实现decoder或接受实际binary。清单覆盖Header／Row／NodeState／Method及Pose依赖类型、数组stride／count、基础类型与bool表示、Failure枚举，并要求确认episode／ordinal／linked-row关系、矩阵空间及参数来源。最终解析必须绑定冻结源码、实际MSVC布局和同session CPOSE数据，保留异常／失败／缺失／嵌套／并发／非有限值；端点相同不证明读者排他、未来权重、最终渲染或物理结果。

### 2026-10-25 — 原生控制输出实际布局与精确阶段关联

用户再次确认先完成武器与掩体共用的自由双手，双方继续使用 Native 唯一身体／手部执行端。Native owner 已冻结控制观测 v1b：生产 inl 为a3276d4f…7725c，四源交付9f0dde8a…00d26。Root在自己的忽略目录独立编译布局 helper，直接 include 对方冻结的真实 test／inl，由实际生产类型导出，未重新手写结构或运行 test main／observer。`/std:c++17 /W4 /WX /O2 /MT /EHsc /UNDEBUG` build／run均0；实际编译依赖、四源及其他依赖前后相同。

实际 Header72／Row1944，九类全部成员的size／align／offset、数组data offset／stride／首末、基础类型与bool／float表示、Failure十五值及rows128均已导出。布局为忽略的 `analysis/hand-native-control-output-20261025cc/layout-v1b/attempt-1792887601705353300/layout.json`，2d0b064d…9529。CCTRL001是默认关闭的只读记录；本次ABI检查没有证明任何实际sidecar、节点姿态写入或跟手结果。

新增离线CPOSE精确关联器，要求同session、完整owner、thread、1-based episode、outer tick和Compose invocation相等。Astra首审指出缺少控制QPC区间矛盾检查，以及L1／physics pair调用错配；旧源码与报告5bc212a3…ab3c完整保留。修订后要求正整数QPC（排除bool）满足C0≤before≤after≤C1；主／late物理各自配对并核对原caller、kind／phase、invocation及physics_matrix；L1核对原outer tick。缺端或矛盾保持具体原因，不借用其他阶段的数据。Weight只有原parser已精确关联且时间位于对应区间时才列为可观察记录，仍不解释未知应用权重。

严格关联与反例42场景通过，六份源码／输入前后稳定；回执为同目录 `join-v1b/attempt-1792888199784113300/receipt.json`，e56f5fd6…2e3d。Astra仅复核两项原P1与新增weight过滤，均已关闭／未发现新P0或P1，报告d263715d…aa03。此处输入是此前真实bv CPOSE加明确标记的synthetic控制行，没有冒充新CCTRL实采；矩阵相等始终只是数值观察，reader fence、最终渲染／物理、writer权限和游戏验收均为false。Root本轮没有启动Game／XR／IDA；连续跟手、动画交还及Native内部追踪发布接线仍未完成。

同目录纯Python CCTRL001 decoder最终固定为4698aa2f…ff03，绑定实际布局的原始文件与canonical内容hash，即使传入已准备的可变Layout也重新核对。11组CPU／33份保留synthetic二进制及CLI成功／拒绝检查通过；原生failure=okay与额外比较过滤分别记录，完整坏行、float原位bits／非有限分类、参数差异及显式compute→weight关联均保留。最终CPU回执 `parser-v1b/cpu-attempt3/result.json` 为c444c953…c75e，交付回执3c1e1cb0…c09c；首次仅因两个fixture留存文件重名而拒绝的失败原件仍保留。

Root对最终decoder做三项端到端集成：旧bv真实CPOSE数据与明确synthetic控制bytes匹配时，一组compute／weight及完整矩阵逐bits对照通过；不同scene与非有限输出均保留且拒绝比较。七份源码／输入前后稳定，回执 `join-v1b/integration-1792888738648462900/receipt.json` 为f7ee80c9…df39；旧decoder的同类回执另留存。本轮实际Native19测试由Native owner执行，但其在启用前报告控制代码匹配检查拒绝，未派发capture event，也未产生新CCTRL／CPOSE；该失败不能用本节CPU结果代替，正在定位具体guard。离线工具已交Native owner继续使用，尚无新实采或自由双手运行通过结论。

### 2026-10-25 — 内部追踪发布、读取与单 Tick 快照候选

Native owner授权Root并行完成实际Core发布／读取候选。Root先以其当前真实生产依赖严格编译旧RigTrackingLiveTest，16个mode全部通过，35份依赖固定为原件和隔离候选，回执6055b424…e066。后续修改只在忽略的 `analysis/hand-native-rig-live-20261025cd/`，未改双方现有live源文件或启动Game／XR／IDA。

候选使用独立CoreSnapshot保存原Shared唯一TrackingBundle、最终pre-HMD base、实际Scene／源时间／连续性和ticket／reference revision，不再要求外部注册cookie。内部getter按调用方另行认证的body owner／原Scene与当前Native session、停止／故障、world generation、ticket、reference、缓存修订及原始源龄核对。内部缓存修订只用于失效，不冒充XR epoch或输入时间。公开V1仍通过真实已发布Broker注册鉴权，并调用已审共用投影，Frame1824／ReadRequest40与原导出不变。

新增非copy的TickLatch，在真实outer meshTick入口一次取得快照，四个控制节点使用同一对象／key／原Scene数据。每次使用仍重核时效与当前性；重复begin、错误key及失效后不能换入新帧。错误end只使帧失效并保留active key，必须由原真实outerTick结束关闭；正确end不能作为同Tick重新取样开关。实际Scope独占、异常finally、当前owner认证、全局头引用及四节点调用尚须Native owner接入，不能把helper当作执行或写入许可。

Astra首审保留三组P1：最终mutex后的停止／引用／源龄收尾、注册变化或reserved cookie被误作已发布，以及错误end解除sticky。修订在锁内重新取时钟并重核原源龄与全门，公开读在最后检查后再核真实注册；Broker新增只读SC state／cookie／revision稳定predicate，既不使用hand authorization，也不改变phase-invalid数据语义。最终inl为aa401a2b…7e0b，Broker为fbca2c51…db18。新26个mode／669 checks通过，36份实际依赖前后稳定，回执68765c37…f8e9；最终旧16个mode回归亦通过，bd2fcbd8…b16c。第一轮新测试的ticket分派错误仅修测试，失败完整保留。

Root另用host自身guard page精确暂停真实Broker的result copy：cookie已reserved但尚未published时公开读拒绝，内部Core保持原字节；继续真实短拷贝SEH使注册失败，随后worker collect通过。32项检查、严格编译／运行均0，回执ce2e0b7f…addb。pin hook只在私有fixture，不在生产实现；Actor／Scene／PG／模块pin和Runtime端点仍为host seam。实际Runtime源码另固定为5d9255f6…9481作外层初始化／Call／停止合同，不替代真实生命周期测试。

37份最终源码由Astra复核，三组P1在声明范围内关闭，未发现新P0／P1，报告212cce14…0dfb。冻结交付 `delivery-v2.json` 为4e51c153…e42，已发Native owner；包含两份实际生产文件改动、共用Core头依赖、四份fixture改／新增及未接调用点。默认renderer／启动器和此前19／20／21冻结原件保持，实际持续跟手、动画交还、最终姿态写入与头显验收仍未完成。

### 2026-10-25 — 原生控制实际样本与独立阶段复核

Native20的分段日志将此前拒绝定位到weight getter的MinHook create status8；未派event、没有新姿态样本。Native21以冻结20最小派生，构建时仅对已认证exact6字节getter生成兼容trampoline，vendor原件保留。购买版RO3随后取得1次event、CCTRL8rows／15624字节和CPOSE1episode／594792字节。Root已实读final：accepted=true、errors空、Native／entry清理及96项独立核验通过，原8份pose完整chunks和新增control范围匹配；它是普通CT0／CA0状态只读采样，不是掩体／连续跟手或头显验收。

Root固定两份原始bin、最终decoder、原CPOSE parser／layout、关联器及final八个输入，另在忽略的 `analysis/hand-native-control-actual-20261025ce/` 以真实CLI独立解析。8行全部可比较，4组weight关联及4个compute的精确CPOSE关联通过，原件前后hash相同；回执29e11064…3495。两条本局链为8／10／11与32／34／35，父关系分别6／8／10与30／32／34；未用其他session的GNames补骨名。Limb各前两个矩阵与C1逐bits相同，末端矩阵有约1.30e-4／2.37e-4原生分量差；其后的两Single矩阵各与C1逐bits相同。四次实际alpha返回均1，+AC原字段值均1，但不因此证明未来v248返回或写入阶段条件。

四对Compute／Weight的真实entry RSP相同，前者输出header参数位于+0x80，weight的原R9实际位于+0x70，不能把它当pose输出。现有schema没有采weight返回后的pose／scale header；零数组不是空header证明，v240的RET imm0也不是数值0。后续必须关联前一Compute的真实header并在目标阶段重新读取／核对；本轮没有新增writer、reader fence、最终physics或自由手运动通过结论。

Root另重hash20／21／21sim全部源树682／684／684项、八份ready artifact，并验证normal三文件差分、CMake逆向逐字恢复、capture及vendor原字节、原76加observer共77项definitions，以及sim唯一原命名hunk。审计80936d51…c4b7；实采CCTRL为597b2d01…f5b7，CPOSE为103e331a…dfc0。旧21-ready元数据仍含logging_delta_only=true，Root已向owner指出其实际为getter兼容修复／测试增量，需另行澄清，未改旧冻结选择，也未以此字段替代源码或运行证据。

### 2026-10-25 — 共用默认手部定位与真实控制矩阵适配

继续优先完成武器和掩体共用的自由双手。Native owner已按实际本地依赖接收前述Core发布／读取候选，原16、新26和真实Broker注册窗口共43个独立host运行通过；接收回执保留在其 `native-rig-live-receiver1/`。真实入口与只读Core观测由该会话继续接线，Root本轮不启动Game／XR／IDA，不修改对方live文件或争用运行窗口。

Root在忽略的 `analysis/hand-authored-mount-20261025cf/` 制作 `marcus115-openxr-grip-authored-v1`。本地静态导出的全部115根参考骨，其原始四元数／平移／父链与RO3的五个有效阶段逐字节一致；canonical几何签名为3b3c978c…5193，不包含GNames编号或地址。这证明指定骨架几何对应，不证明零售整资源字节相等，缺失的Pmain阶段也没有算入。左右手默认握持轴和掌心位置明确标记designer_authored，完整旋转、米制偏移及模型倍率只换算一次；未知模板拒绝，不从任意动画姿态自动标定。实际手掌法线、贴合与其他模型仍待目视／头显核对。

新增私有 `hand_core_target_adapter.h`，由同一锁存Core、当前模型／左右手绑定和已知默认定位一次产生完整腕目标，Limb与Single复用。保留原始Core完整字节和时间，检查原Scene／controller源龄、手部mask及显式producer连续性策略，不重新读取桥接数据。调用者仍需核对真实当前Tick／owner／asset／停止与执行许可；Target只是普通数据，准备时刻的校验不等于当前写入授权。长度模式继续由调用者明确选择，复用原共用求解器；失败不改变输出。

实际RO3 Single的m15分别为0.9999998807907104和1.0000001192092896，旧精确等于1的校验会错误拒绝。Root仅在私有Single旋转替换路径允许有限的abs(m15−1)≤2×float epsilon：验证副本将该项设为1后沿用原rigid规则，输出不归一化，七个非3×3字段逐bits保留。Limb和component-to-world保持原严格规则，投影、scale、shear、reflection、NaN／Inf及超界尾值仍拒绝。原头和修复前397项收据／失败原件均保留，未覆盖Native21／22冻结源。

严格CL构建下，默认定位699项通过；适配器7组603项通过，包含原始Single row2／6直接成功、实际Limb输入、边界及输出不变反例。最大位置残差3.60395e−6原始单位、旋转basis元素差4.75608e−8，仅为本次CPU数学误差。Core／grip／typed asset身份仍为明确host输入，不冒充游戏实采追踪。最终适配器回执542c9ba8…b622，Root独立核对16份源码／冻结副本和7份输入，审计1a0dd7c2…4b86；默认定位回执fc85d84d…0f7。Astra对三个生产头和限定测试复核无P1／P2，报告883f4738…a1dd5。17份源与4份fixture的整体交付 `delivery-v1.json` 为5622ab3c…f902，已发Native owner接入。

本组是交给Native唯一执行端的离线候选，没有安装到默认启动器。持续独立跟手、完整roll、远伸手、追踪丢失、回中／转向、进出掩体和原版动画接管后恢复仍需实际运行，之后再接身体取枪取弹、单双手握枪和各武器装填。完整12武器目标及已有support3／menu-modal1默认保持，所有后续自动测试继续每边0.4、静音、置后和tiny-output只读。

### 2026-10-25 — 首次游戏内共用Core通路与独立原始字节复核

Native owner的22私有候选完成 `cover-host-1025core1` 单event只读取数：真实outerTick27090／Compose27091中的四个控制计算共用一次latch，无外部注册cookie。原 `runtime-final.json` accepted=true／errors空，Native和entry恢复、96项独立核验、XR exit0、音频10项恢复及原10文件保持；Root已实读该final，而非由进程消失推断清理。此局仍为模拟来源，原生身体／手部writer和姿态写入关闭。

Root在忽略的 `analysis/hand-native-core-actual-20261025cg/` 固定三份实际bin及五份最终报告，用22冻结源的12份真实依赖重新严格CL导出Header112／Record1920／Core1832及所需offset。布局构建／运行0，依赖前后相同，回执193a0c8a…b34d。随后独立读取7792字节Core原件：四份完整snapshot逐bytes相同，Native／Scene／outerTick／Compose／ordinal／node／endbone／原时序精确关联到CCTRL原始row hash和字段；另对冻结输入重跑保留的owner纯checker，结果与其parse逐字段相同。Root未重写该checker算法，也未重测游戏生命周期。

实际Core文件为24ab74c4…662c，Root核验回执47e1fc85…b4a4；原始输入前后不变。四份实际publication262／sourceTicket263／referenceRevision6／原published_ns一致，但world_pose_mask=1、flags=0，只有头部世界姿态，不能将其计为双手追踪或运动成功。下一步由唯一Native执行端接入前述已冻结手部适配器，再用实际matched左右手grip与完整姿态验证持续跟手；原动画、权限、输出寿命、最终physics和头显验收边界保持。Root本轮未启动Game／XR／IDA，未修改Native22冻结源或默认启动器。

### 2026-10-25 — 当前骨架对应与独立XR手柄发布准备

Root继续核对core1实际CPOSE：五个有效阶段中全部115骨q/T／parent均与默认定位模板canonical 3b3c978c…5193逐bytes相同；没有把缺失的Pmain0／1补成有效，也未使用跨session的GNames。当前C1参考骨副本及核验保存在 `analysis/hand-native-core-actual-20261025cg/`，`current-model-binding-v1.json` 为d5e58475…04d4。此项支持当前样本的模板选择，未来live asset身份、模型倍率和当前执行许可仍由Native caller核对。

core1的私有host明确配置no_xr_controller_producer，实际XR命令factory未传`--controller-input`，因此上一轮没有手部世界姿态；这是该只读测试的设置，不是已经定位到的追踪故障。既有BQ sim XR 1a059d2d…b4e9包含该CLI与Source1连续性发布，Native Shared tracking读取也不依赖旧Native controller_input_observer开关。Root在 `analysis/hand-xr-controller-producer-20261025ch/` 准备仅给XR factory追加该token的私有差分，原Native factory逐字节不变；只执行实际factory的AST dry-run，使用伪PID，确认全部原参数保留并仍为0.4。回执e34523bc…0e62已发Native owner。

该副本只是下一派生profile的差分载体，保留父profile路径，不能直接当新runtime-ready运行；由Native owner并入下一私有runner后按原流程冻结。没有修改core1原件、重建XR或启动Game／XR／IDA，也未增加Native observer／writer参数。实际sim controller active／flags15、与同Stereo精确匹配、手骨移动和动画恢复仍须实采；后台置后、物理后台输入关闭、静音及observe-only要求保持。

### 2026-10-25 — 游戏内真实双手Core接收与身体原点问题

Native owner已将共用Core基础、event观察及默认手部定位分别提交为d051cfe／822d525／f80433b；其实际本地依赖下默认定位699、适配器603及原数学417项均通过，接收回执29e19472…1c05b。Root已核三生产头实际hash，没有把它们安装到普通启动器。其后Native23完整接线编译遇到Windows RPC的small宏命中共用solver局部名，owner仅在capture全局adapter include处push／undef／pop该宏，保留共用数学hash和失败原件，修正候选为23b。

Core2使用原Native22sim／原BQ XR和原静止模拟器，唯一输入变化是给XR命令追加controller-input。购买版实采四份完整Core逐bytes相同，实际左右grip flags15／active1，world mask7／flags3，来源MetaOperatorSimulated1；Native session和XR session分别保留，没有混同。Root子任务在 `analysis/hand-native-core-input-20261025cj/core2-v1/` 从实际Native22类型严格CL导出controller等offset，核四条Compute和C0／C1的完整cohort、原始range hash与QPC bracket。Pmain0／1缺失保持，原owner纯checker明确按同算法重跑，不冒称新的独立payload实现。

原Core为34ad0b06…6cf6，四snapshot共同hash14f856a0…e44b；最终核验57bd5c01…d40e2，整体交付37dc1fbe…d6cb。Root另重hash全部32项输入／源，31d73013…fe61。原publication／Scene／stereo／controller时间未刷新，最后Compute的源龄均小于15毫秒；这仅是本次样本时效，不是性能或连续运动结论。owner原final的Native／entry96、XR0、音频10与原10文件清理通过，Root本轮没有启动Game／XR／SDK／IDA。

实际空间核验揭示首个需修复的问题：u100／模型倍率1时，两段臂长左46.99497022、右46.99497366原始单位；真实grip加已知authored mount后的肩到腕目标距离分别144.45525572／126.48935208。实际共用control::solve两侧preserve均返回unreachable且输出不变，没有用adaptive或动画配对校准掩盖距离。当前115骨canonical仍与3b3c978c…5193对应。实测回执 `reach-v1/attempt-1792899149053572100/receipt.json` 为9de6f1fa…8f2f。

进一步从实际Core和C0／C1定位：world_head、base camera和source camera位置相同；相对C1头骨55的距离为1.742732米，按实际base forward／right／up投影为−1.678775／+0.460439／+0.082604米；与肩中点距离1.716038米，C0结论相近。头骨语义来自已完整几何对应的静态模板，不读跨session GNames。最终space回执6bffb941…034f3、space-handoff d388325d…c406a；原7db版本数值相同，另留存起始self-pin改进。此项证实当前样本原点明显偏离身体，尚未证明具体修复方式。

双方据此保留Native23只读adaptive规划的限定用途，先测真实FP状态下的头／肩差及preserve可达性，再进入持续手骨输出。旧first_person_view预览使用anchor.location加eye_height，再沿forward加0.35米／向上加0.10米；实际Rig取view_owned而非开关bool，不能仅凭打开FP推断已对齐，也不能在未配对测量时断言这些常数错误。固定模拟器姿态、原始校准与源时间保持，避免改握持偏置凑通过。完整自由双手及武器目标继续进行。

### 2026-10-25 — 独立手腕roll测试输入候选

当前冻结模拟器的controller命令只有yaw／pitch。Root为后续完整旋转验证另复制其24份源，仅修改runtime.cpp／mcp_integration.h，新增可选roll弧度／单手内部字段。默认roll0走原两角路径保持pose浮点位，省略roll保留已有值，显式0复原；其他字段继续父协议默认。原head-relative语义、源类型／epochs、tracking、后台输入、渲染与export均保留；父runtime／当前测试和默认选择不变。

实际parser及机械抽取的原／新pose和赋值块以W4／WX／UTF8编译，83项检查通过，4915b8d1…a452。头部globals是明示host输入，未创建SDK会话。完整DLL沿用原Ninja／Release／MSVC14.51.36231与原flags，构建及父5项CTest通过，95ea815f…6c97；两条编译和链接命令仅规范化source路径后逐字符相同，审计e7c24bda…f653，原export ordinal／name一致。早期GBK／Windows near宏的测试fixture失败及一次命令过滤失败均保留，生产差分未因此扩大。

Astra在静态pose范围窄审无P1／P2，08af6cab…23e2。冻结交付 `analysis/hand-simulator-roll-20261025ci/delivery-v1.json` 为9ca672c5…4a43，DLL为50d61004…a803，已发Native owner按后续私有profile采用。隔离roll必须每次带完整固定posXYZ／yaw／pitch／按钮状态，并固定头pose；只发hand和roll会触发父字段缺省重置。角速度仍是父yaw／pitch近似且z=0，不能用于动态roll速度、投掷或physics验收。尚无该新runtime的实时roll、手骨写入或头显结果。


## 共用 Native 与掩体分支记录

### 2026-10-24 — 明确真实站蹲／物理探身与抓边并存

按用户纠正，改写[交互规格](../../6dof-cover-interaction-design-20261024.md)第4节：真实站起、蹲下、弯腰和左右物理探身均直接影响视点及掩体暴露，无需LGrip；左手抓边提供额外虚拟位移，可以单独或叠加使用。仅对“位置完全不变的纯旋转”禁止额外程序化探身，不能丢弃自然转头／抬头／侧倾伴随的真实位移。虚拟中立眼点不能将最终视点锁在顶沿以下，松手保持也不能锁住真实头部运动。

同步操作表、实现P4、AGENTS与早期文档的解释，新增C19–C21，当前为21项掩体验收及4项跑动稳定验收，共25项。此轮为需求修正，未修改运行时代码或默认构建。

### 2026-10-24 — 追加跑步／冲刺镜头稳定要求

用户反馈当前跑步镜头晃动过大，要求跑步时镜头不要晃动。已在 `6dof-cover` 的[交互规格](../../6dof-cover-interaction-design-20261024.md)第4.1节加入必做相机修复，覆盖普通跑动、冲刺、游戏允许的沿掩体跑动及起停／进出掩体过渡。去掉游戏额外跑动颠簸、摆动与角度晃动，保留真实头显运动、正常位移、地形／姿态变化、主动转向和武器行为。

源码复核确认现有第一人称预览仍采用原视图forward与Pawn／EyeHeight输入，不能因已经去掉部分位置动画和滚转就认定跑步稳定完成；本轮未认证运行时根因。规格新增独立实现项及R01–R04四项验收，连同原C01–C18共22项。已同步分支AGENTS要求；此轮仅补充规格，没有修改运行时代码或默认构建。

### 2026-10-24 — 独立 6DoF 掩体分支与交互规格

按用户要求建立独立托管 worktree `C:\Users\11\.codex\worktrees\6dof-cover\gearsVr`，分支精确命名为 `6dof-cover`。从 `codex/0.0.4` 的 `9ffa710` 出发，在该分支提交根目录尚未提交的29项相关0.0.4源码、诊断和记录，形成基线 `85fc2b3`；根目录分支、源码和用户配置未改变。与本功能无关的未跟踪内容未纳入基线。

新增[6DoF 掩体交互与实现规格](../../6dof-cover-interaction-design-20261024.md)，覆盖左右／上探、真实头动、LGrip抓边与反向拉拽、掩体朝向与眼点、原版吸附／退出、输入互斥、射击／受击／碰撞一致性、生命周期与18项验收场景。用户已明确松手保持身体位置、重抓反向移动才缩回。旧交互文档的摇杆探身、松手回位及不保留进入／退出状态已标注被替代。

源码检查确认第一人称预览仍统一水平前移0.35m、抬高0.10m，没有按掩体高度建立眼点；只定位待改入口，不将其称作经实测确认的唯一原因。已按知识库流程保存8项限定词查询，得到掩体动作／槽位／相机的脚本对象及历史字符串候选，未把声明当成当前进程可调用接口。只读核对了独立武器分支的双手输入桥及LGrip复用需求，没有合并或修改该分支。

实际support1 native／XR冻结源、二进制与来源回执在新worktree内按字节复制并核对，连同限定查询共1,196个文件；默认native仍为 `5e78c2a7…fe7a`，XR仍为 `df54d17c…1b48`。记录保存在 ignored `analysis/6dof-cover-planning-20261024/baseline-copy.json`，同时保留support1与left-history1冻结回执。当前工作区源码快照不冒充实际默认构建源；后续仍从实际冻结源最小增量开发。

文档检查确认新增规格的6个相对链接可解析、18项验收编号完整，Git差异格式检查通过；1,196个私有副本再次核对哈希一致，根工作区分支／HEAD／状态及已复制来源文件哈希保持不变。检查回执为 ignored `analysis/6dof-cover-planning-20261024/document-validation.json`。

本轮完成规格与实现准备，未改掩体运行行为、未重编默认DLL／XR、未启动游戏或新增IDA导出。下一步是只读认证原版掩体状态、合法边缘、输入分派和相机来源，再进入纯逻辑与游戏接线；自动化、真实游戏和实体头显验收均按新功能单独记录。

### 2026-10-24 — 运行时支持观测集成默认入口

按用户要求补齐五类运行证据：XR运行时实际提供的系统名称、vendor ID、追踪支持和尺寸上限；游戏交换链所用队列设备与XR实际设备的适配器信息／LUID对应关系；游戏与XR进程实际加载的图形模块文件版本，以及已安装／运行中VR组件版本；匿名交换链编号下创建、绑定接受、resize边界／返回值、Present进展和原有捕获／owned管线计数；以及首次故障及时落盘与运行中native状态保存。头显连接方式、厂商不提供的型号／版本保持不可用，不用注册清单或已安装显卡列表代替实际运行证据。

原生回调仅写有界内存并使用try-lock，文件和适配器描述查询在现有worker完成；累计全入口Present和原有受限绑定Present分别记录，不能当作实体头显FPS。使用匿名chain ID保留关联，未导出地址。原生日志两份轮转、单文件预算，首次负HRESULT另存并flush／commit；Windows并发读取测试发现安全CRT默认独占打开，已改为允许读取。启动器守护线程以进程创建时间核对game／XR身份，持续保存状态；记录采集失败原因，读不到不记作零。首次观察到的故障经fsync及原子创建保留，后续清理错误不覆盖；启动阶段另有独立首次故障回执，即使普通时间线满也保留。原生QPC、频率和观察时间保留，跨进程需依时钟字段关联；最早失败返回不自动等于根因。

导出修订为 `support-evidence-3`：增加support轮转历史、首次故障、运行中native快照、VR软件补充清单及逐项runtime覆盖状态；支持包仍筛除私人路径、账号、凭据、指针、姿态与原生指令。当前配置标明为导出时信息，后续会话另记录本次请求参数。新增源码模块已加入portable载荷白名单。正常会话未出现故障时没有首次故障文件；缺失值、截断和采集失败仍可区分。

构建严格基于已验证冻结源：先核对left-history1的582项源码哈希，复制私有源后仅加入3个native源／构建文件及共享支持header／test；XR仅对原色彩冻结pose源加入观测。保留全部原构建开关、FPS两槽与独立左眼历史，未将当前源的其他实验整体重编。默认native现为 `build-support1/GearsCaptureSupport1.dll`／`5e78c2a736526b3bf22a87023959d1635d82d8bcec80ddd05ed24fede4d6fe7a`，XR为 `build-support1/OpenXRPoseProbe.exe`／`df54d17c03068f46cad60f43c5b17114dcc61bf612321efc85611822cd181b48`。原left-history1与原色彩包保留，根GUI仍为0.0.4 Dev。冻结回执在忽略目录 `analysis/runtime/support1-frozen.json`，完整源哈希和构建记录在 `analysis/support-instrumentation-20261024/`。

最终购买版／私有配置／behind-windows模拟器测试 `continuous-stereo-30188-20261024-020809-bad3a100` 正常启动，约22.0秒有效段提交1983次场景projection、复制1914次图像（含重复投影，不能等同独立新帧数）。真实日志包含system、native／XR adapter、按chain匹配结果、双方模块快照、交换链快照及live状态，观察器无错误并正常join。native清理、入口还原通过；原10份配置／存档哈希不变，未启用截图。此前初版运行测试为 `continuous-stereo-52204-20261024-014313-c36b1934`，记录保留。最初一轮仅因测试清单仍指向不存在的旧工作副本而在启动前拒绝，随后改为当前已选正版Steam副本，未改用户选择或重启旧P2P调查。

对最终真实会话导出21项，ZIP **1,035,984 bytes**、展开 **10,386,072 bytes**。逐项确认系统／选卡／模块版本／交换链／live状态进入ZIP，私人路径／邮箱／账号格式／64位指针扫描为零；正常运行没有伪造首次故障。启动器229项测试、运行控制／支持诊断67项测试、39672项FrameLink断言、独立历史状态测试、DLL ABI及连续停止自测通过；默认GUI自检、85项portable载荷文件及默认清单哈希验证通过。回执为同目录 `final-validation.json`／`export-audit-final.json`。仅支持观测和生命周期验证，不宣称实体Pimax／Sboys验收或Chtiblue根因已解决。尚未创建0.0.4公开发布、tag或上传此本地支持包。

最终复核另修正长会话保留：原生256项事件缓冲使用循环索引，flush后持续接收后续事件，超出预算保留最近事件并显式累计丢弃数，首次失败另存不受影响；轮转重新记录已有适配器。启动器日志轮转时重放有界设备／软件上下文，保留最初观察时间，避免多次轮转后仅剩后半程计数。三次轮转上下文保持、缓冲超限后后续stop保留、首次故障不变及已满启动时间线仍能导出首次失败均有回归验证；最终native／XR采用新空build-c目录，重新核验依赖前缀及493／532项依赖后构建。

### 2026-10-24 — 一次导出保留故障定位所需的现有证据

Chtiblue的已收两份0.0.3支持包分别保留native激活失败与启动后零双目产出的摘要，但旧导出器没有带上同次native-start、native-status、session-status及完整停止原因；增加XR CSV字段还会导致旧白名单丢弃整份CSV。这是证据导出缺口，尚未证明该玩家故障的具体根因，不能从摘要推断SteamVR崩溃、相机距离或某个API layer为根因。

`diagnostics_bundle.py` 导出修订为 `support-evidence-2`，已接入正常GUI调用的同一 `launcher.py export-logs` 路径，无需专用入口或重编GUI。最近三次已识别会话各增加 `debug/session-N.json`：保留native激活错误／Python回溯文件名和行号、native生命周期／捕获／绑定计数、停止请求来源、XR错误及数值输出、CSV和菜单／世界／遥测历史。当前XR新增数值列已纳入，未知列不会使已知列全部丢失；姿态列不导出。已有启动时间线和组件哈希继续保留，0.0.4-dev版本后缀也可正确导出。

系统清单保留CPU、GPU、显卡驱动、RAM及Windows；新增只读OpenXR运行时与API layer注册清单，仅保存名称、版本、DLL文件名、清单哈希、注册状态及已声明的环境禁用标志。此清单属于导出时环境，不是旧会话实际加载证据。没有启动额外XR会话、读取游戏进程或改注册表。HMD型号、连接方式仍需已有运行记录或用户反馈；现有冻结XR尚不记录这些信息，也未增加驱动／GPU崩溃转储。

每个附件上限4 MiB和8192条记录，长文件保留开头64 KiB与末尾，记录预算优先保留首尾与错误；ZIP总展开预算32 MiB，优先缩减长历史，保留核心激活／状态内容。manifest逐附件记录读取范围、缺失／截断／变化状态、过滤／非法行数、保留数与错误遗漏数，避免将不可用当作零。历史候选池现保留较新的候选，不再在目录前512个候选后停止；目录遍历仍有200000项上限及显式告警。所有输入保持固定同前缀附件、拒绝链接／重解析点、拒绝跟随报告提供的路径；支持包过滤私人路径／账号／凭据、指针／姿态／原生指令、存档及二进制。

验证：45项导出器测试与226项启动器Python测试通过，包括激活原始错误、零出图捕获计数、停止来源、当前／未知CSV列、错误位于长历史中间、总大小预算、较新候选、隐私／重解析点与原子写出检查。复核额外修复扩展历史中过滤掉来源状态的问题：摘要与扩展记录均保留首次采样、数据推进／不变、计数重置、身份不匹配／不可用状态及XR／尺寸读取错误类型，未知私人字段继续过滤。真实三会话离线导出20个文件，ZIP **1,120,082 bytes**，展开 **11,856,449 bytes**；最新三会话与三次启动时间线核对通过，native-start／native-status／session-status及CSV均有记录，扩展遥测中fresh状态保留通过，私人路径／邮箱／账号格式／64位指针扫描为零。私有审计回执在忽略目录 `analysis/support-evidence-20261024/offline-export-audit.json`，并未上传此本地包。

默认native仍为 `a637128c…d758`，XR仍为 `58ef6c39…539a`，根GUI仍为既有0.0.4 Dev；本次只改Python导出和测试／说明，没有重建或替换冻结运行时。已发布0.0.3包／标签保留，未创建0.0.4发布。旧支持包未收集的文件无法由此更新追溯恢复；当前改动提升一次反馈的证据完整度，不表示所有未来故障均能仅凭一包确定根因。

完整度复核：当前已补齐“采集到但没导出”的重要缺口，仍不足以认证“一次反馈必能确定底层根因”。后续诊断构建应优先补足实际XR系统／头显信息、游戏与XR各自选择的GPU适配器及匹配关系、故障时实际加载的图形模块／层、swapchain创建／选择／resize／Present与捕获拒绝的关联进展，以及首次故障先于清理错误的持久记录。注册清单不能替代实际加载证据，已安装GPU清单不能替代实际设备选择，正常会话导出成功也不能替代对激活失败／零发布／运行时退出等故障包的逐项验收。本次未把这些待补项写成已实现，也未因此更换冻结XR／native默认构建。

### 2026-10-23 — 从0.0.3创建闪烁修复分支

按用户要求丢弃0.0.2分支临时未提交的启动账号识别修改和本地配置改动，回到 `codex/0.0.3`，从 `9ffa710` 创建 `codex/0.0.4`。恢复此前保留的0.0.3调查记录和用户配置；原stash仍保留，未删除其他来源文件或忽略的调查证据。根目录GUI恢复0.0.3已发布文件 `45cf2da2…7fb6` 并通过隐藏自检；默认native `7571fee6…bab9`、XR `58ef6c39…539a` 仍为0.0.3基线。尚未创建0.0.4发布或标签。

用户在0.0.2也观察到闪烁，不能据此将0.0.3新增的FPS优化认定为根因。0.0.2隔离测试已成功注入，但动态模糊开启的当前画质组合在ticket2出现 `draw_state`／flags258／pair0，双眼投影和复制均为零；不能冒充有效的无闪对照。原始配置与存档未变，两个后台测试及XR进程均已退出。0.0.2临时启动改动已按要求丢弃，不移入0.0.4。

0.0.3实际手动会话已在7200×3200完整原生SBS接收源中捕获左眼清晰／强烈模糊交替、右眼清晰，详见[既有调查](../../steamvr-left-eye-flicker-20261023.md)。本轮优先核对动态模糊和左右眼历史状态。源码存在左眼复用原玩家ViewState、右眼使用独立工厂状态的非对称性，这是待验证假设，不是已证实根因。修复尚未实施，未取得运行／头显验收。

私有回执和图像继续保存在忽略目录 `analysis/flicker-simulator-20261023/`；分支／GUI切换回执为 `branch-004-baseline.json`。

### 2026-10-23 — 左眼历史状态隔离已接入默认构建

已在同一购买版游戏、每眼3600×3200、原色彩策略、动态模糊开启的静止场景中复现用户截图的左眼强烈径向拖影。普通游戏帧与VR帧穿插时，旧版VR左眼复用原玩家的历史状态，右眼为独立状态。现将VR左眼也交给原生工厂创建的独立持久状态，保留原玩家状态供普通渲染；不通过关闭动态模糊或更换颜色规避问题。修复限定正常连续VR会话，保留诊断控制模式原行为。

| 配对测试 | 完整SBS源图明显左眼模糊 | 预览模糊采样 | 普通帧回退次数 |
| --- | --- | --- | --- |
| 0.0.3基线 C | 5 / 7 | 17 / 88 | 1325 |
| 独立左眼状态候选 C | 0 / 7 | 0 / 88 | 1463 |
| 最终默认构建 E | 0 / 7 | 0 / 220 | 750 |

完整源图为7200×3200，位于XR转换／合成之前；右眼始终保持清晰。基线左眼梯度RMS约3.61–11.69，候选C约11.69，右眼均约15.26。指标只用于这个匹配场景；预览为若干连续采样窗口，不能换算成整个会话的闪烁频率。最终E包含瞄准、暂停和重载，因此其预览与C并非同一静止阶段的频率对照。

早期A/B未复现的隔离测试不能充当无问题对照：私有启动脚本将无BOM配置写成UTF-8 BOM，游戏随后重写部分画质为低档。保留原始编码后，配置保持原高画质，基线自然重现问题。只修正忽略目录内的测试脚本；用户配置／存档没有被替换。

候选D与最终E均通过普通瞄准／射击、暂停恢复、检查点1→2代重建；瞄准／射击证据限于FOV和弹药响应，不是命中精度验收。新状态只在工厂身份、虚表和与原玩家／右眼不别名检查通过后使用；停止及换关沿原生release／延迟删除路径清理，等删除完成后才允许重建。两个会话均正常停止，双眼状态释放、钩子入口还原、XR退出通过，资源保留和原生UI／材质故障计数为零，用户原10份配置／存档哈希不变。全部测试进程已退出。

默认native现为 `build-left-history1/GearsCaptureLeftHistory1.dll`，SHA256 `a637128c5604a7c88d9dc7658a2e5248a3a5c6b76559d6ff3a0d57ab7710d758`；从实际默认冻结源而非含其他实验的当前源整体重编译，保留原构建开关与FPS两槽修复，额外启用独立左眼状态。XR仍为 `build-runtime-colors1`／`58ef6c39…539a`，未更改颜色策略。最小源码增量已按唯一上下文合并至当前源码，保留其其他已有内容。

验证包括独立状态、双眼构造、提交生命周期等9项针对性原生检查，当前源码新增状态／双眼构造2项编译运行检查，实际启动自测，217项启动器Python测试，以及GUI隐藏自检。根目录 `GearsVR Launcher.exe` 已更新为 **0.0.4 Dev**，SHA256 `4c864dc26d8376c8ebb29aacc2f4cb5a749864b3cf84d74e2048218cc570e913`；启动诊断版本为 `0.0.4-dev`。无需使用独立实验入口，正常重启即可加载默认修复。

冻结回执为忽略目录 `analysis/runtime/left-history1-frozen.json`，配对图像和完整会话证据在 `analysis/flicker-simulator-20261023/`。原生／模拟器像素与生命周期回归通过；**物理头显、完整战役及FPS效果未验收**。未创建0.0.4发布包／标签，已发布历史包保持。

### 2026-10-24 — 6DoF 掩体分支开始实现

工作限定独立 `6dof-cover` 分支／worktree。需求以 `docs/6dof-cover-interaction-design-20261024.md` 为准：真实站起、蹲下和左右探身直接生效；左手 LGrip 拉拽提供额外连续偏移，松手保持，再抓反向缩回。保留原生进入、吸附、沿边移动与退出，替换 LT／RT 自动探头。用户已授权自主后台测试及多个 agent；测试保持显式 behind-window 构建、静音、tiny-output 只观察、真实控制器后台输入关闭，所有运行均使用隔离配置／存档。

已实现纯 `CoverInteraction`、`CoverControllerAdapter` 与 `LeftHandInteractionOwner`。控制器桥的三份源码从武器分支提交 `da775ed7d12118910b140c156b9750a0c678a9bd` 按 blob 原样引入，未编辑武器 worktree。抓边测试覆盖三方向、冻结局部轴、松手保持、重抓、20000 帧重复／漂移、总范围、所有权、追踪／生命周期中断与独立头显位移；适配器 629 项检查，左手决定接口 203 项检查通过。它们是可接线的 CPU 逻辑，尚不代表原生掩体或跨 DLL 的互斥已经生效。

新增只读 `cover_state.py` 及原生调用元数据准备／FFrame 一致性捕获／有界 wire packet。当前购买版进程已实际导出两套 State 各自的 PlayerMove／UpdatePlayerPosture 和两个 setter，共 6 个函数、162 个身份及 13956 项源检查；两个 PlayerMove 的 locals size 为 20，CT／CA／CD 是三个独立 byte。暂未把 out 参数 flags、alias／copyback 或动作枚举解释为写权限。被动 observer 复用现有唯一 dispatch hook，原函数调用一次，保留返回寄存器，不读取 Result 作为 outparam，不改 gameplay。

跑动相机候选只修改当前玩家已认证 CameraBone modifier 的 `bDisableAmbientCameraMotion` 和 `RoadieRunShakyCamDampeningFactor`，保留其他 bool 位与自定义／开火动画。实际应用、读回、条件恢复已通过，2 次写入及 2 次恢复，无待恢复项。走路 A/B/A 受控输入对照中，运动帧的 CameraBone Alpha 由基线约 0.0034–0.0735、恢复后约 0.0036–0.0878，变为候选阶段 0；候选运动样本相对控制器的额外 pitch／yaw／roll 变化为 0，camera Z−Pawn Z 恒为 76 native units。基线与恢复阶段对应角度变化约 0.077°–0.253°、高度仍有扰动；角色继续真实移动。这是普通相机缓存读回，尚非 GPU／头显证明，也尚未证明疾跑与掩体进出。

一轮初始输入检查因测试窗口盖住副屏任务栏而拒绝，没有绕过保护或运行时调整窗口；后续在隔离副本启动前使用 1280×720 窗口配置，后台检查通过。另一轮请求实际触发翻滚与剧情通话，明确排除出疾跑验证。用户正常 10 份配置／存档在已结束的隔离会话前后哈希不变。

尚待连接并验证：有效掩体中的姿态结果→setter 同调用回执、合法边缘与连续身体／碰撞、面朝掩体与遮蔽眼点、实际武器／左手消费者、疾跑及过渡、头显验收。`CoverMovementPct` 的现有脚本用法是速度归一化，不能凭名字当探身权重；单人原生掩体／盲射免伤分支也必须纳入身体暴露验证。默认 support1 native／XR、独立左眼历史与原色彩策略保持；未发布新的版本或标签。私有证据位于本 worktree 忽略目录 `analysis/cover-implementation-20261024/`。

### 2026-10-24 — 正版矮掩体端点与相机会话恢复

在隔离副本加载已有医院测试进度，四份来源存档逐项核对 SHA256，并只映射到实例内已有的 profile 文件。该进度保留 assisted／既有 health assistance 标记，不构成无辅助战役验证；正常存档与设置没有替换。医院最近的可上探矮掩体实际进入成功，`CoverType=2`、slot 0、原生 stationary 标记成立；原版瞄准产生 `CoverAction=7 / CoverDirection=3`，释放后恢复 0。后退沿原逻辑清除 cover、退出吸附；输入驱动在特殊状态转换时中止继续施加移动。测试用户原 10 份文件的哈希仍保持。

新增只读动画图 sampler，验证当前控制关系、Mesh Owner、Animations 实例与模板区别、SkelComponent 回链，以及选中节点的 Children／MaskList 元数据和结束一致性。在真实角色上读取到 177 个节点，含 2 个 CoverBlend 和 18 个 PerBone blend。该矮掩体中实际有权重的 CoverBlend 从中立 child 0 切到原生上探 child 4，收回后恢复；提前读取的动画过渡样本因值变化被拒绝，稳定后通过。13 组新增 synthetic 测试与已有 cover／world 测试合计 41 项通过。SpaceBases 为 115 个 Matrix、stride 64，RefSkeleton 反射仍为 Int4，未把它猜成骨名表或擅自解码 opaque 数据。

被动调用候选从 support1 实际冻结源做最小增量，保留独立左眼历史和原色彩。注册表只允许在数组地址／容量不变时增加 count，选中身份、脚本、函数、类型及其他源字节仍逐项复验；回退、搬迁与已选源变化拒绝。新候选用显式 event 开启有限观测窗，不轮询文件、不发送 gameplay 命令。两次窗实际开始／封存 64 个 PlayerMove 调用，拒绝与 dropped 均为 0；192 条 updater／动作 setter／方向 setter 回执保留同调用的 frame、locals、原始字节及代码位置。两窗分别覆盖稳定中立与稳定上探；由于每窗开始时动作已稳定，这批数据尚不证明输出从旧值改变到新值的 alias／copyback，下一轮在按下／释放前开窗取证。绝不把 Result 地址或不透明 RAX 当作 outparam。

候选 native 为 `build-cover-observe4/GearsCaptureCoverObserve4.dll`，SHA256 `3bd0c30ca1dbc49a6d2628073c7e67f987bcc1258f1983e2a7efb484f9d7b8f0`，仅用于私有实验清单。原生依赖解析 498 项；实际会话正常停止，native drain、入口还原及 XR 退出全部通过。它没有替换正常启动器的 support1。

相机设置现由可选 `RunCameraSession / RuntimePolicy` 管理，只有显式清单能力才启用，默认能力尚未打开。准备完成、首写之前独立打开并核对原进程创建时间与镜像的 wait handle；身份查询失败不当作退出。退出时恢复仍待确认的 journal 会保留 reader／writer 与 owner 锁，由只执行清理的 non-daemon guardian 继续处理；专属 sidecar 逐次核对 session，不覆盖主报告。会话／wrapper／原 Policy／right-stick 检查合计 134 项通过，另通过真实 CloseHandle 失败路径的针对性检查。实际候选会话自动应用 2 个字段、正常关闭时恢复 2 个字段，released／closed 均为 true，pending 为空，无 policy 错误。启动器能力、portable 模块清单及诊断已接线，尚未宣称疾跑、GPU 或头显通过。

身体实现继续沿原生 cover 混合姿态与必要的局部骨链修正推进，保持逻辑 Pawn／cover anchor 稳定。没有采用每 tick 把整个 Pawn 根位置来回移动的方案。只读 native 查询的两个 getter 及其非连续冷分支、实现函数已在私有 IDA 导出，并在当前正版进程逐块验证完全一致；证书保存在 ignored analysis，不把源码散列或相同 RVA 当作当前字节证明。连续身体权重、骨姿态发布、真实射击命中消费者与掩体／盲射免伤两个分支仍待联接和验证。

### 2026-10-24 — 掩体姿态实际变化与骨骼查询接通

上一轮 T 测试于本地 05:13 结束，随后没有自动接续施工；用户询问停滞和进度后，已如实报告，并恢复实现。T 的完整日志现已解码：64 个完整调用组、0 拒绝，其中两组实际记录 updater 的 caller-local 从 CT2/CA0/CD0 变为 CT2/CA7/CD3，以及反向恢复；后续两个 setter 均在同一 caller/locals 保留新值。其余62组只证明稳定调用连续，未将两个独立窗口的数值差异误记为同次调用变化。会话停止、native 清理、80项入口还原和 XR 退出通过，原10份配置／存档再次哈希匹配。

新增限定骨骼查询 adapter，只经已加载且哈希已知的 Playground 游戏线程队列调用 `MatchRefBone`／`GetBoneMatrix`。只使用当前受控 Pawn 的 Mesh，类型、参数布局、返回宽度与所有8段 getter／实现代码在各次调用前后核对；不改 Actor、Mesh 或碰撞字段。队列已发布完成才释放请求分配，未知／超时不重试、不擅自释放。模块检查区分磁盘和当前进程：实际启动后改变的是 PE Resource 目录，Import 保持原值；只允许该购买版散列与已保存的确切 Resource 元组，其他文件头与 section 布局变化仍拒绝。修复属性查询提前扫描无关祖先全 children 链造成的读取预算超限，仅认证声明 owner；首次见证与结束复读保留，没有提高全局预算。35项针对性测试通过。

V 隔离正版会话已取得普通姿态、矮掩体收拢、原版上探和缩回的头／骨盆矩阵，各次查询完成且临时分配释放。当前角色的头骨索引55、骨盆1；该次收拢 head Z约-40.44，上探约17.75，缩回约-37.87，骨盆约-71.80→-31.91，Pawn 根位置基本不变。采样不是原子同帧姿态，端点会有待机动画变化；不能据此假设 blend权重与世界高度线性，也不证明碰撞／渲染或枪口。上探两次 getter 队列查询在有界瞄准输入中取得，随后已释放输入。

实际 CoverBlend 更新、父 blend Tick 和子权重传播函数的全部已导出代码段均与当前进程一致；反射确认 `bDoingCoverActionAnimTransition` 为原生 post 读取／写入的字段。连续半探可能被误判为长期动画过渡，需在姿态原型中单独验证新发射请求，尚未宣称瞄准／开火闭环完成。新增两套只在原函数前调用 helper、随后透明跳到原函数的 x64 thunk；合成测试121项通过，覆盖指定 volatile 寄存器、浮点／栈参数、返回通道、标志位和 unwind。真实身体姿态钩仍在接线，默认能力关闭。

用户追加要求后台游戏测试采用 `render_scale=0.4`。新 V 会话已从启动配置使用0.4，每眼1440×1280、接收源2880×1280（推荐3600×3200的该次模拟器尺寸），继续仅一份测试游戏、置后、静音、只观察输出，不改正常游戏画质或用户窗口。完整私有回执位于 ignored `analysis/cover-implementation-20261024/`。新原型尚未并入正常启动器，武器 worktree 未修改。

### 2026-10-24 — 连续上探身体原型完成接线，等待实际姿态检查

新增默认关闭的 `GEARS_COVER_BODY_PROTOTYPE`，已接入现有 native 提前 reservation、统一 install、入口认证与停止流程；运行时还需测试子进程显式环境变量，普通启动不生成原型 INI。两个透明入口在 CoverBlend tick 前恢复上轮自有 Weight，随后在原生父 Tick 完成权重计算、首个非空 child 传播前，写入 neutral/top 的 `1-u/u`。原生传播与动画求值继续执行，没有修改 Pawn 根位置、DesiredWeights、TotalWeight、CA/CD 或动作过渡位。当前仅支持实际受控角色的稳定 CT2 矮掩体、SpecialMove0 与正的 incoming weight，不代表左右探身已接通。

测试事件按 `0→.25→.5→.75→1→.5→0` 推进，10秒无新事件撤回。它是验证连续身体姿态的私有原型控制口，尚未接到 LGrip 或真实头动。反射布局从实际 receiver／当前受控 Pawn 的相关类链取得，首次写入前可随有效上下文更新；写入前逐项复验所选属性、对象和存储关系。日志按各步骤保留首轮中立／上探 child 的传播前后样本，避免第一步耗尽后续日志空间。

每个字段条件恢复仍等于本模块安装值的内容，外部改变不覆盖；world、当前 Pawn 或存储关系变化并不等于旧实例销毁，未知或仍存活的旧实例保持 pending。只有对象槽不再指向原地址或 fresh 完整身份已不同，才可退休义务。停止前给游戏线程最多5秒恢复，逾期明确未完成并保留钩子／资源，不能报成功。新 `CoverBodyJournalTest` 将68项合成检查纳入版本控制，覆盖存活旧实例、身份失效、部分写入回滚和准入；两套 thunk 的121项检查、原型 ON/OFF 编译与33项启动配置测试通过。

实际候选从已验证的 observe4 冻结源叠加上述最小增量，保留原构建开关、独立左眼历史和原色彩。body2 DLL 为 `build-cover-body2/GearsCaptureCoverBody2.dll`，SHA256 `6ae83dad2d22cc91d3d302e79a5752aa7e41dd7f12ff4b1bc83fd4db0c0779b0`，依赖及完整构建回执保留在 ignored analysis。第一轮 W 在游戏启动前因少一份 preflight helper 被拒绝，未注入；补齐后检测到另一会话的游戏／XR 测试，后续 X 在实例建立前拒绝，未叠开第二份游戏。当前等待独占实际测试窗口，没有把编译／合成检查写成身体姿态或头显通过。

### 2026-10-24 — 连续上探七档实际身体校准与独立清理

body3 修正普通 NPC 的 CoverBlend entry 提前恢复玩家 Weight 的时序问题：正常 entry 只恢复当前节点，只有 Reset、超时和停止才在已绑定游戏线程撤回全部自有字段；撤回未完成时不消费下一档输入。非目标 child 在进入 Scope／mutex 前拒绝。独立 journal 检查为 99 项通过。候选仍从 observe4 的实际冻结源叠加最小增量，body3 DLL SHA256 为 `ea3dcade1e68fb8cedba3fd298a6ba551d818dff2122d8226fa4e578ae7be8e9`；本分支没有把该实验写入正常默认启动器。

ab 隔离正版会话使用 render scale 0.4、后台置后、静音、tiny-output 只观察。当前角色稳定 CT2／SM0，活动 CoverBlend #4；七档均取得 Head、Pelvis、Foot_L、Foot_R 四个原生骨矩阵，查询全部完成并释放临时分配。各档耗时 4.64–5.45 秒，在 10 秒 TTL 内，但未达到 2 秒目标；矩阵按顺序获取，不能声称同帧原子姿态。

| 上探权重 u | child 0／4 | Head Z | Pelvis Z | 原生动作过渡位 |
| --- | --- | --- | --- | --- |
| 0 | 1／0 | -39.16 | -72.11 | false |
| .25 | .75／.25 | -22.95 | -60.46 | true |
| .5 | .5／.5 | -3.20 | -48.70 | true |
| .75 | .25／.75 | 13.30 | -37.00 | true |
| 1 | 0／1 | 28.54 | -25.56 | true |
| .5 | .5／.5 | -3.87 | -48.86 | true |
| 0 | 1／0 | -38.09 | -71.95 | false |

坐标为游戏原生单位。七档采样的 Pawn root 完全相同；最终中立相对首次中立，头／骨盆 Z 相差约 1.07／0.16。双脚有原版姿态的水平移动，Z 变化约 4.7 单位，尚未证明碰撞、受击或 GPU 对齐。实测证明身体姿态连续上探并能收回，未接 LGrip、真实头部到身体的驱动或掩体专用眼点。fresh BoolProperty 的 mask 从 `+0xd0` 取得，确认 `bDoingCoverActionAnimTransition` 在 `+0x540 / 0x20`；u>0 时持续为 true，包括 u=1。下一步需使原版 post 判断读取原生 Weight、后续 pose 求值读取连续 Weight，不能直接清除过渡位，也不能据现有结果声称已经能开始射击。

停止认证新增两个固定安装入口 `0x2a85a8`／`0xfa680` 的完整代码段 SHA256 检查，同时检查 CoverBlend 非连续冷段；它不接受 native 自报还原作为替代，未知 RVA 仍拒绝。32 项停止认证与 4 项路径关闭恢复检查通过。ab 先 Reset 至 pending=0，再正常停止：session passed、native cleanup、独立 entry restore／all_match 均为 true，XR exit 0，原 10 份配置／存档哈希一致，游戏／owner／XR 三个自有进程均已退出。此前 aa 的独立 verifier 异常仍保留为未完整验证，没有追认为通过。

完整证据保留在 ignored `analysis/cover-implementation-20261024/body3-ab-calibration-review.json` 及 `analysis/runtime/continuous-stereo-53688-20261024-150032-c1505489.restored.json`。测试存档保留原 assisted／health-assistance 标记，本轮不构成无辅助战役、射击、疾跑或实体头显验收。用户明确授权各会话直接协调测试；当前按菜单→武器→掩体排队，完整清理后移交窗口，自动测试都使用私有 0.4 配置。最终集成需基于届时最新默认 support2／菜单修复，保留其他会话已完成的内容。

### 2026-10-24 — 连续 pose 与原生动作判断分离

body4 改为固定两个寄存器参数 ABI 的 call／post 包装：每轮首 child 前安装连续 Weight，最后非空 child 的原传播返回后恢复本轮 native Weight；原 CoverBlend post 返回后再安装连续 Weight，供 pose evaluation 使用。TotalWeight 仍只由原生传播计算，未增加 CA/CD、过渡标记或 Pawn 根写入。新增跨原函数的 Scope／wrapper／事务责任、嵌套 frame、异常 finally 和条件恢复；临时 native 阶段不等于清理完成。自有 helper 的普通 C++ 故障在 RAII 解锁后停止新修改，原函数异常继续穿透；未知内部 SEH 不作可恢复承诺。Scope 构造失败也会归还自身 active 增量。

独立 ASM 319 项、生产 journal／生命周期 705 项通过。冻结候选严格从 source3 叠加五文件和一个唯一 Scope constructor hunk，反向替换整份 capture.cpp 等于 source3；CMake 与其他文件不变，实际 flags 保持，完整 DLL＋9 helper 通过。body4 DLL SHA256 为 `c3da142950b1a5967011c772661242c24bf63dfd7a60f5cfc668ce950b81d4e1`。

ae 正版实际日志已记录 u=.5：native post 的 child0／4 Weight 为 1／0，而 child TotalWeight 为 .5／.5；pose rearm 的 Weight 为 .5／.5，TotalWeight 保持 .5／.5。它证明两阶段分离，尚未证明实际半抬骨骼、过渡标志持续正确或新开火响应。ad／ae 完成中立新几何和四骨查询，实反射过渡位为 false；两轮均正常停止、独立83项入口匹配、XR0、原10文件一致、所有自有进程退出。两次校准中断来自本地文件共享竞争与过严的状态发布时间判断，已保留失败并修正只读等待。af 则为真实 startup panel `no_eligible_window` 拒绝，未 continuous activation／未射击；prime、Playground、音频与身份清理均通过，没有前台替代或调整用户窗口。证据为 ignored `body4-owned-runtime-review1.json` 与各实例回执。

ac 有界疾跑夹具通过原版 CanDoSpecialMove(4,false)、held getter 和唯一 TryToRoadieRun(false)，条件 End 分支在同 Pawn SM4 时执行，结束恢复 SM0。完整元数据检查耗尽采样窗口，没有形成 during-SM4 相机多时间戳；3.578 秒实际租借超过原1秒／后3秒目标，均明确记录未达标，不以缺失样本的零速度当作稳定结果。相机字段2写入／2恢复、held／latch、timer、native／独立入口／XR／原10文件和进程清理均通过。疾跑相机稳定验收仍待补齐。

### 2026-10-24 — 半探新开火通过，接入私有中立眼点预览

ag 同一隔离正版实例完成 body4 主验证。固定 .5 权重下，只执行一次新的 LT＋Shoot 输入动作，原武器 AmmoUsedCount 4→8；射击时 CA／LastCA=7／7、Weight=.5／.5，起始的原生过渡随后完成，射击样本 `0x540 & 0x20=false`。双层释放已确认，没有第二次发射动作。此结果证明发射响应，不代表命中精度、枪口遮挡或伤害校准。

同实例的中立／半探／全探 Head 相对 Pawn Z 为 0.708492／35.724415／67.756161，半探位于两端的 0.522254，三次 Pawn root 相同。射击后的首次快照因动画变化在 getter 前拒绝，随后保留原 owner，以新的 pose-only Reset→Advance 轮次取得两骨 half 与四骨 full，没有重复射击。最终 fields／nodes／in-flight／transactions 为0、failed=0；native、独立83项入口、XR0、Playground、音频、原10文件与全部自有身份清理通过。完整证据保留于 ignored `body4-owned-runtime-review2.json` 和 `cover-host-1024ag-body4-final.json`。

新增纯 CoverView 与 CoverInputOwner 接线逻辑：基础相机只接受冻结中立眼点和虚拟拉拽，真实 HMD 在原 stereo_matrices 中只叠加一次；身体目标单独计算虚拟偏移＋真实平移，使用调用者提供的测量曲线，不假设权重和头部世界高度线性。唯一共享 LeftHandInteractionOwner 接受 cover 与真实已提交的武器租约；cover 只优先新请求，松手身体偏移不占手，失去左手输入不冻结真实头动。新增552项视点、48项 owner 组合检查通过，既有 kernel／adapter／lease 检查保持。

新增完整8段 native getter 哈希与 ABI 适配，用动态 FName→bone index 读取头矩阵，拒绝未加载 SpaceBases、越界索引和 MasterPose 路径，38项独立检查通过。只读 neutral provider 在当前 GT／场景／Pawn／Mesh／World／Link／slot 与中立姿态认证后，冻结 HeadWorld−PawnLocation；201项检查通过。CoverCameraPreview 将它接在 first-person prepare 后、stereo_matrices 前，成功时跳过 cameraCloser；只在测试子进程精确环境变量启用，普通路径默认关闭。首次冻结 PawnYaw+π 为本场景面墙的预览候选，尚未旋转身体 Mesh；100ms校准等待和未知上下文保持原有路径，进入过渡仍需实际调校。1236项合成接线、body ON/OFF 单TU编译与配置拒绝检查通过。body5候选从 source4 最小5文件叠加，DLL SHA256 `156085d845021507be77e0a56da34aa19d3f82f8f3820ed57362455e1730d0b5`，完整9 helper通过；实际眼点／朝向与直接 getter 验证待执行。

只读 CoverLink geometry sampler 已实际取得 Link／Pawn typed变换、slot flags／offset及 live Stand130／Mid70、StandingLean[0,78,69]／CrouchLean[0,70,19]，未将这些本场景值硬编码为所有掩体。Python世界组合仍标记 candidate，未等同碰撞表面。左右分支与伤害保护继续独立验证；没有以本段结果宣称 LGrip、左右／上抓边或受击已完整接通。

### 2026-10-24 — 流送关卡校验修复与低眼点实际验证

ah 的 body5 相机未应用：provider停在 current_link，applied=0。当前只读几何成功通过 Link／Pawn／PC 的 WorldInfo 相同检查，但 Link.outer 与 Pawn.outer 不同；原 provider 错把同一世界的流送关卡掩体拒绝。修复分别认证三个角色的 WorldInfo 属性类型、owner／extent、继承路径与引用类，在相同游戏世界内允许不同 Level.outer。没有仅删除身份验证。neutral 320 项与 camera 1257 项通过，body ON／OFF 编译通过。body5b 从冻结 source5 只替换 neutral 一个文件，其他源码、renderer flags、body4事务、CMake不变；DLL SHA256 为 `e4893743f7563fd1cb934c104506ef4bba4e74c26f08192e1bc77caaf74b5400`，完整 DLL＋9 helper检查通过。

ai 在0.4后台正版实例中，原生 getter实际校准Head55，固定绑定一次，最终camera_applied=679。中立、first-person开、first-person关、唯一一次原LT均accepted；LT期间CA0→7，没有射击。四阶段保留的基准眼点与固定朝向、相对Pawn位置在日志精度下均未变化。中立眼点为[-381.723053,-3132.8313,-29.4882965]，相对Pawn高度9.59703；first-person事件确实消费两次，最后requested=0。它确认新眼点路径实际运行及对旧相机抬高的覆盖，尚未证明身体Mesh面墙或枪口／受击对齐。

最后after_aim的只读动画快照因“Cover value changed during observation”拒绝，整轮complete仍为false；没有重发动作，也未把该快照当作相机故障。ai没有重新采集碰撞几何。与ah同fixture、同Link188的candidate顶部比较，眼点约低43.49单位；这不是已认证碰撞面。现存PNG属于启动普通视角，不能替代后续掩体画面或头显验收。后续诊断增加实际每次base view的origin／从矩阵反算的origin／Scene invocation，避免将初次校准日志误作当前渲染矩阵证明。

ah和ai均正常Reset／Stop，native及独立83项入口检查、Playground、XR exit0、音频9／9恢复、原10配置／存档哈希、自有进程退出全部通过。证据保留在ignored的`body5-ah-sameworld-evidence1.json`、`body5b-ai-runtime-review1.json`及两轮final报告；默认support2／menu-modal1未替换。

### 2026-10-24 — 连续实时目标与几何准备

新增独立side profile，仅在当前原生镜像下将child5作为单侧连续端点，尚未写镜像或混合二维姿态。live target入口按当前GT、完整Pawn／Mesh／World身份、world generation、严格递增frame／QPC和100ms时限接收任意u。每轮冻结自己的u与endpoint，新的输入仅供下一轮使用；传播、native-post、pose-rearm及停止恢复沿用既有事务。重复frame不刷新时限，未知身份继续保留恢复责任。中立判断使用实际u，不能因小幅探身落入诊断bucket0就当作中立。1717项事务／live检查、24项关闭构建检查和64组环境组合通过；此时尚未完成新的实时输入游戏验证。

新的相机／输入组合从同一份StereoResult读取精确匹配的controller-v3 frame，raw追踪坐标映射与双眼使用同一reference；真实HMD平移只给身体目标，基础相机只接虚拟偏移。手部失踪不应冻结真实头动，头显固定位置旋转不生成姿态位移。私有top实验显式使用ag测得的三点高度曲线，不把该角色／场景标定当成通用常数。合法表面与可达性未认证前，runtime的LGrip获取仍关闭；没有把只读候选平面升级成抓握许可。

Native只读geometry模块认证当前Link／Pawn变换、live高度、slot结构212字节与选定Bool的D0 mask，655项启用和2项关闭检查通过；所有组合位置／顶沿均标记candidate，未建立原生getter或碰撞证明。动画诊断补充选定child的bMirrorSkeleton反射offset、原始word、D0mask与解析值，不写字段；新增镜像类型／范围／竞争检查与既有动画、几何共27项Python测试通过。接下来在同一隔离实例验证真实模拟头显驱动，并只读捕获两个原生slot getter与Actor.Trace的当前证书，继续闭合合法抓边。

### 2026-10-24 — 连续物理上探实测与统一左手接线

AN在正版隔离实例完成9段模拟HMD运动，render scale=0.4，窗口置后、静音、tiny-output只观察。每段都将Native选择的精确sequence与全量XR CSV关联，全部保留source1；不属于真实头显验收。同位置yaw15°／pitch10°的physical up与top weight均为0；上移0.15／0.30／0.55米得到u=0.214188／0.428376／0.811942。后两段Head相对中立Z增加33.289／59.440，Pelvis增加20.093／37.839；归零后child0/4恢复1/0，四个骨查询阶段PawnRoot逐值不变。骨查询非原子限制保留。左右各0.20米仅证明不误触发top，尚未证明侧探身体。

九段camera base及HMD应用前matrix origin保持不变；它们不是最终双眼矩阵或GPU画面的验收。原姿态已恢复，Reset后pending／fields／nodes／in-flight为0；Native、独立83项入口、XR exit0、原十文件哈希、音频9/9、PG关闭及准确自有进程退出均通过。证据为ignored的`body7-an-motion-summary1.json`及`cover-host-1024an-camera-final1.json`。AM先取得当前GetSlotLocation／Rotation入口，AN再将两个wrapper与implementation的7个完整chunks同当前内存两次稳定读取逐字核对；尚未调用这两个getter或认证墙面碰撞。

统一左手接线现在覆盖所有有效Scene，包含未进入掩体的ordinary CT0。角色绑定独立认证当前PC／Pawn／Mesh／World／Weapon及typed Instigator，不依赖body布局曾经初始化，支持已知空武器和换枪。Native只解码一次逻辑LGrip，冻结帧后执行weapon prepare→唯一owner分配→weapon commit→cover complete；未结束的prepare事务、callback与模块引用在停止／unregister期间继续保留。迟到unregister仅对精确已退休cookie返回transport quiescence，不把它当作PG资源恢复证明。

PG忙碌或提交失败时，当前新Scene可以重新认证上一次已提交的偏移并组合当前真实HMD；不发布本次未提交拉拽或租约，避免视点跳回中立或让真实起身依赖左手。事务745项、shared组合30场景568项、runtime ON/OFF 424项及独立binding579+1+1项检查通过。Native8选定source8c：只将19组输入／生命周期hunks和两段CPU targets移植到frozen7，均能反向逐字恢复母体，不携带当前树的其他NGX实验；DLL SHA256为`3a05b91fa0453dda9e8a32916f05330c681f9c9291a901bd291e4e4fad93ed55`，23原DLL/helper和85个focused CTest通过，实际依赖516项。V1注册／注销导出已存在，两ABI头与武器5565eec相同；实际PG注册联调尚未完成。

LGrip runtime获取仍关闭，候选slot几何不能授予抓握。两侧镜像、完整上／侧组合、Mesh朝墙、碰撞／受击／枪口一致性、跑动SM4实测和最新默认集成仍待完成。固定child0/4/5只有三角形姿态域，不能把归一化三权重称为完整二维跟随。默认support2／menu-modal1及其他工作树未替换，无新release/tag。

### 2026-10-24 — 原版左右姿态、原生掩体坐标与LGrip输入实测

AO使用隔离正版、0.4 scale及原有后台保护，关闭body／camera／input三个原型，取得完整七阶段原版数据：CA依次为0、3、0、4、0、7、0；左侧CD1、右侧CD2，始终同一Link／slot0／CT2。三条原生LT租约全部确认释放，五条平移租约都确认归零及writer关闭，没有Shoot。四个已知骨骼各阶段查询通过，PawnRoot逐值不变；这些getter仍非同一原子姿态快照。原版左右child5 Weight均为1，top为child4=1；child0／4／5边的bMirrorSkeleton全部false，Pawn.bWantsToBeMirrored则左true、右false。Mirror_Master与TransitionBlend祖先尚缺typed权重／镜像值，不能据child5边值就认定原版左右的翻转位置，更不能把child5=0当作全局镜像切换不会改变中立姿态的证明。

新增`cover_surface_getters.py`通过现有已附着PG队列执行原生GetSlotLocation／GetSlotRotation，不另建hook、不注入新模块。先后认证当前UFunction签名、Link／World／Pawn／slot、7个完整代码chunks，任一身份／代码／数组头变化即拒绝。12秒总界限与单次1.5秒队列界限内，未知pending保留回执，不重试。AO实际getter pair返回Location=(-388.7710266,-3114.7421875,-58.9570160)、Rotation=(0,-31860,0)；现有候选位置相距0.0060243游戏单位，只说明该slot位置接近，不认证顶沿、墙面或可达性。另7个RotatorMatrix／MirrorAxes／mirror consumer chunks与当前游戏匹配。新增14项和相关13／35项共62个Python检查通过。

AO全量CSV有11332个连续published/source1帧。Native清理、独立本次实际80/80入口、XR exit0、原10文件哈希、音频9/9恢复、PG关闭、tiny worker及三个准确自有进程退出均通过；原型关闭时不把本次80项写成83。证据保留于ignored的`native-pose-ao-summary1.json`、`cover-host-1024ao-camera-final1.json`和`signed-side-ao-evidence-review-astra.json`。

独立测试模拟器grip1仅将私有文件通道的optional squeeze接到已有XR action状态，保留source1和raw追踪标记，原运行时与正常配置不变。真实v3 receiver在8秒无游戏bench回读0→1→保持→0→1→完整pose恢复，保持不重复rising，最终累计press/release各2；原10文件和前景窗口不变，端口空且两个自有进程退出。XR exit1为dummy接收端没有游戏GPU提交的已知结果，objects_destroyed／shutdown_completed另有成功回执，不称为游戏渲染通过。第一次bench因Operator端口早于实际模块receipt而提前拒绝，0输入，失败与后续自然退出检查保留；第二次增加有界receipt等待并按实际raw head_flags=15核验后通过。证据为ignored `simulator-grip-source1/bridge-bench2-receipt.json`，它验证输入链，未验证真实抓边获取或身体拉拽。

Native9统一CallFunction入口正在离线接入。最外层UI hook改为POD placement storage与finally逆序恢复，游戏SEH／RAX／四参数不变；接入辅助桥后的生产outer hook已通过763项检查。辅助通知与模块pin必须覆盖pre→original→end整个空隙，通知fault或不确定结束保留sticky debt；完整冻结DLL构建与真实PG联调仍待完成。局部child5镜像仅准备显式opt-in A/B候选，不作为已认证的原版左右开关。所有未完成的完整玩法与默认集成范围继续保留。

Native9随后完成fresh冻结构建。normal DLL SHA256为`50601ea6bf86ffcd6b0532e62975ed80704198846ba4d02ae660ff5f2cf6d2dd`；以frozen8c的628文件为母体，仅12文件变化、7个capture hunks与CPU CMake块，capture／CMake反向逐字恢复，其他文件与渲染flags保持。三个注册／注销／辅助attach导出存在，hand V1与补充ABI分别保持5565eec／32b416a。23个原DLL/helper和94个focused CTest通过，包含41个runtime独立场景、30个shared场景；独立Broker1348、runtime ON516／OFF85和outer763项检查也通过。Attach只接受已由Native安装的唯一wrapper、同cookie／native incarnation／context与原已pin PG image；原调用空隙即使callback计数为0仍阻止unpin，未知结束不可由后续closed抹除。

Native9sim仅改变既有一个sim连续模式guard，其余633文件／CMake／flags逐字相同，整体反向恢复normal；DLL SHA256为`e8109e819c63e105637178fc9e33213ed0d81be46ac38efd0b7f78702f8c5727`，23项helper重跑通过，CPU94因源码逐字相同明确复用。证据保留于ignored `native-body9-build-result.json`、`native-body9sim-build-result.json`及两份source provenance。候选已交武器会话配合其冻结c41a65c subscriber，继承旧PG pin仅为reference，实际测试必须换成该subscriber的真实hash；当前仍未完成真实跨DLL注册／开火／清理验收，没有替换默认组件。

### 2026-10-24 — 上层镜像定位及原生几何接入

AP关闭三个原型，在0.4后台正版实例完成七阶段原版对照。Master#2 child0的镜像位依次为false、true、true、false、false、false、false，对应CA0、3、0、4、0、7、0；左探收回后仍保留上层镜像。Transition#7两条边始终为false／Weight1和true／Weight0。四骨前后14个祖先区间accepted，身份、布局、mask和采样值保持；独立读区间仍不等于同一pose epoch。清理80/80、XR0、音频9/9、原十文件及全部自有进程退出通过，证据为ignored `native-pose-ap-summary1.json`与`cover-host-1024ap-camera-final1.json`。

Native10仅在frozen9上覆盖三个body文件，保留默认关闭的`side_mirror_trial`局部child5 A/B能力。typed BoolProperty和masked CAS与Weight共用租约；原生post可暂见原值，但pending责任保留；零round实际进入node cache且进入fresh mesh epoch后才允许一次固定方向试验，回零／Reset／失效后不能在同cohort二次激活。Journal2682项、MASM319项及OFF分支通过，normal23个helper与94个CTest重跑；sim23项重跑，CPU94明确复用。normal／sim DLL SHA256分别为`1e1af1a824e09d9066ce2b4524ffefd443770b897824c0266a450a739ca1319b`／`4ccc24cd6f286a2f73edc18de63fb81a3efd62cb114dcce1dcf981cf8e0448b3`。这不是已认证的原版左右开关或完整二维控制；实际骨骼效果、最终输出独占及pose并发仍待验证。

Native11在frozen10上接入原生几何读路径。当前Link／Pawn／World／slot及字段布局在一次body mutex内认证，native getter Guard反复检查冻结的Scene Receipt、Context和原始Values；独立存储原生slot位置／旋转，原候选平面和顶沿仍不认证。新的显式`CoverGeometryObserver`默认关闭，只在有效Scene取最多16条成功样本，间隔至少100ms；没有view、body target或抓握写入。几何ON953／OFF2、原生getter helper208项通过，normal与sim两版均实际重跑23个helper和94个CTest，body2682仍过。normal／sim DLL SHA256分别为`d83775bdde24298279946b27b6c93e4701f90d4d653ecce2c768086186d42405`／`b3d72dc8ed274c52e9c2daee30258df175a1d34f34a57f6f50883e8efbceaca0`。此前9／10候选不变；运行时几何、镜像A/B和c41a65c跨DLL检查继续排队，没有替换默认。

补充发现并修正Python诊断字段：`UFunction.FunctionFlags`应读`+0xf4`，旧`+0xfc`实际与函数name index一致。已由保存的ProcessEvent完整chunks及CallFunction的`R9+F4`、Native bit10消费者ASM确认；首次大dispatcher伪码导出超出预算的失败保留，第二次仅机械导出ASM成功。相关Python64项检查通过，新增用例确保name index里的0x400不能授予Native许可。AP历史`function_flags`字段作废并保留原记录，独立NativeIndex／Parms／Return／入口／实际getter值未重写，也未重新执行游戏动作。更正及私有metadata／名称读取helper2在ignored `function-flags-offset-errata1.json`。它不改变hand／supplementary ABI、Native11或既有身体驱动。

最新默认已前移至support3 Native `4752801a…ae14`与menu-modal1 XR `37f46312…a4d`。本分支候选保留原冻结渲染来源，后续正式集成需叠加最新support3被动诊断及菜单修复，不能拿旧候选覆盖默认。LGrip合法表面／碰撞、独立上下左右组合身体求解、Mesh朝墙、受击／枪口一致性与疾跑实际A/B验收仍为未完成范围。

### 2026-10-24 — 冻结完整三维目标与实际几何读回

新增 `CoverBodyTarget`，将同一已提交虚拟偏移、原始 HMD 平移、视点基准和完整 World／Pawn／Mesh／Link／slot／Scene／XR epoch 绑定冻结为一个求解输入。左右、上下和前后独立保留，支持真实站起、下蹲和身体侧探；没有三角形归一化或高度饱和，固定位置的旋转不产生平移目标。源追踪和目标各自超过100ms即失效。当前模块只存储目标眼点，头骨到眼点校准、骨骼求解、同epoch发布和碰撞均明确未通过，未添加骨骼写入。

Native12严格由frozen11叠加五个文件、六个capture hunks及三行CPU CMake，反向恢复逐字相同，其他文件、冻结ABI和渲染flags保持。normal／sim SHA256分别为`40173f6973da047e2ff80dd98b614ef9f7c0acba751e28f820f3b739f8e25598`／`27baf0bf9225171ce0c6fd0124a0a367b66738a7fba3c737cd60a7c6c07e328a`。两版均实际重跑23个DLL／helper及95个focused CTest；新增目标包316项，既有body journal2682、geometry953／OFF2、surface getter208项均通过。证据为ignored `native-body12-build-result.json`、`native-body12sim-build-result.json`与各自冻结source provenance。未替换默认，也未将这些离线结果计为实际身体求解通过。

AR-A／A2的Native11sim实际正版会话均在稳定CT2／CA0／slot0下取得16条原生表面观测：slot位置为(-388.771027,-3114.74219,-58.957016)，旋转为(0,-31860,0)，原候选位置相差0.00602429416游戏单位。它只验证该slot的原生getter与候选组合接近，尚未认证墙面、顶沿、抓取可达性或碰撞。两轮均在任何Advance前被测试脚本拒绝：A误把inactive Transition child1的合法mirror=true／Weight0当成false；A2等待了仅在停止日志输出的target_u字段。失败保留，没有镜像效果或左右姿态通过结论。A2清理独立83项、Native入口、XR0、音频9/9、原十文件和准确自有进程退出全部通过。后续脚本必须将运行时步号对应的请求目标与实际typed Weight／Total／骨骼证据分开。

AQ疾跑夹具在进入SM4前的PE身份预热拒绝，未执行Try／End或持有跑动输入，没有during-SM4样本。完整清理独立80项、XR0、音频9/9、原十文件和自有进程退出通过。为定位实际差异，`cover_bone_query._module`仅增加失败字段、section差异及头部hash诊断，原拒绝谓词保持；相关64项Python检查通过。疾跑相机稳定仍未验收，不能将无样本写成零晃动。

### 2026-10-24 — 显式Scene与stereo锁上下文

Native13新增内部context ledger及真实`StereoMutex`归属记录，所有11处stereo lock／try-lock使用同一类型；保留原底层互斥行为。完整Scene入口以POD helper及finally包住原有body，原六个参数、返回值和异常继续转发。嵌套、复制／错误地址／错误ledger、溢出和非LIFO结束不能被误记为上下文清空；未知状态保留poison。它目前只提供上下文记录，不授予引擎调用或字段写权；普通callback、ActionGate及前置PE／bridge许可还未连接，不能据此称完整动画交接通过。

候选由实际frozen12的640文件最小叠加为643文件，严格9文件变化、10个capture hunks、3行CPU CMake及四个inl的5处guard替换；capture／CMake／inl均反向逐字恢复，保留各母体原换行。normal DLL为`build-cover-body13/GearsCaptureCoverBody13.dll`，SHA256 `82dbf0779730ce48603ab0ebd198a8f56808e7faae687acd7ac7a47d02802693`。23个原helper与96个focused CTest全部重跑通过，其中context102项涵盖实际mutex所有权、try-lock、跨线程隔离、Scene嵌套及真实SEH穿透；body2682、geometry953／OFF2、surface208继续通过。正式capture对象的实际Ninja依赖525项包含两个新头，37个exports和原3个participant导出、冻结ABI及渲染flags均保持。仅normal版，没有13sim或实际游戏／实体头显通过结论，默认与旧9／10／11／12 normal、sim产物均未替换。证据在ignored `native-body13-build-result.json`和`native-body-source13-provenance.json`。

AR-A3继续在任何正权重前被脚本拒绝：它读取了清理时才复制到runtime报告旁的status文件。原生运行时实际写入各隔离实例`native/session-status.txt`，AR4已改为认证instance_root、native_command build-dir、game／owner incarnation及started_qpc后读取该LIVE路径。A3清理独立83项、Native／入口、XR0、音频12/12、原十文件与自有进程退出均通过。AR-A4／A5则在continuous激活前被原有startup窗口门槛拒绝，没有probe或Advance，不记为身体失败或连续83项通过；窗口诊断为一个visible但client0×0的候选及一个hidden候选，tiny-output曾观察1280×720，但不能将它等同实时帧。未切前台、调整窗口或放宽门槛。A4完整prime／entry、reservation13/13、PG、tiny、音频12/12、原十文件与当前准确进程清理已核；A5保留独立后续审查。

### 2026-10-25 — 完整Support3基线合并与测试阻断定位

按实际源清单复核后，明确本分支冻结渲染源的共同母体为Support1（584文件）；source13仍缺Support2的完整启动timeline，不能只叠加Support3最近三个文件。真实Support1→Support3恰7文件。以source13为ours做三方合并，root审核三处capture冲突，保留cover／hand retained、完整stereo summary scope及hand初始化／失败顺序，再加入原Support3 trace。source14为645文件，恰7文件增量，其余source13内容逐字保持；旧默认及9–13候选保持。

Native14 normal DLL SHA256为`438ac963d8ebbf40427ba1a5245594badcad38eab638560c5550494a35a79651`。23个原helper、97个CTest（原96加AtomicStatusFileTest）及独立SupportStartupTraceTest实际通过；context102、body2682、geometry953／OFF2、surface208保持。76项defines、实际capture编译flags、37个exports及冻结hand／call ABI与13保持，实际依赖526项含新trace／context头。证据为ignored `native-body14-build-result.json`、`native-body-source14-provenance.json`及原始／规范换行的三方冲突记录。随后将相同增量合入公开源码，保留公开树已有实验和测试取舍，没有用混有实验的整份源码替换冻结构建；公开同步记录在`public-support3-rebase1-applied.json`。当前没有14sim、实际游戏或头显验收，也没有替换默认启动组件或加入新ActionGate／Boundary provider。

A5独立启动清理已补齐。A6的有界只读窗口采样实际记录主窗口IsIconic=true、showCmd=2、client0×0；只能确认采样时处于最小化，不能确定触发者。该局随后进入continuous，未进行恢复窗口／前台替代；在两次已完成转向之后，脚本直接索引暂缺的rotation样本而停止。A7改为最多3秒等待新鲜合法姿态，保持原轴向租约与八次上限，进入稳定CT2／CA0／slot0后，被祖先只读区间的registry closing检查拒绝。A8仅首次Advance前允许该精确纯读失败重采一次，每次原始结果独立保留，原closing谓词和后续正权重／骨骼阶段不放宽。A6／A7／A8均完成独立83项、Native／入口、XR0、音频12/12、原十文件及准确自有进程清理。

A8通过中立基线、零档请求、typed child0／5 Weight及cache观测，首次骨骼查询在分配／提交前因PE校验拒绝。新诊断指出同一正版EXE的index12 `.themida` 节raw_size=0，PointerToRawData磁盘11354624、运行时0，其余节字段相同。离线以原1536字节头仅改此字段并叠加已复核Resource目录布局，重建hash与A8实际live header hash完全一致。兼容只接受该正版SHA256、精确index／整条节记录和0目标值；其他头字节、节布局、Import、全部函数代码及每次查询的完整原始live header closing复读保持。兼容记录不识别改写者，不修改游戏文件／内存，也不认证映射节的运行时内容。新增6组含多字段变异、未知hash、closing变化和代码变化拒绝检查，相关Python70项通过；新兼容的当前游戏读回仍待执行。依据为[Microsoft PE字段说明](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)，私有证据在`bone-empty-raw-pointer-layout-ar8.json`。正权重侧探A/B、实际抓边、完整三维身体求解、碰撞／受击／枪口一致性、原LT/RT自动探头移除和疾跑稳定验收继续实现，未计为完成。

### 2026-10-25 — 掩体与武器动作边界的兼容接线

新增 Native ActionGate 与外部动作边界 provider，仍复用唯一 CallFunction owner、既有 hand pin 和同一个左手仲裁器。CallFunction 顺序为 Gate begin、V1 begin、原函数、V1 end、Gate end；所有已接纳责任跨越完整原函数并在 POD finally 收尾。恢复引擎调用只允许认证 GT 上、本次入口的显式许可；Scene、stereo、普通回调、嵌套及未知上下文拒绝许可。外部 State 只保存描述符，TLS／责任节点留在 Native 固定池；停止、异常、不确定状态与未完成责任保留模块，worker 不提前卸载。`WeaponActionGate.engine_restore` 默认关闭，没有启用 STREAM／STOW。

Fast Revoke 仅短 POD copy 和原子撤销，旧 Frame commit 不能覆盖新 generation。Action 引起的 PG invalidate 延后到下一合法 Scene Frame prepare 或停止后 owner worker；original 前即时撤 Native 授权，但不增加回调或取得 stereo 锁。Shared Scene 在同帧缓存之前检查 generation／barrier，撤销后只可重用完整相同且仍新鲜的已认证视角，并清除手部授权；不重复提交同序号掩体观测或新身体输出。新鲜 Scene 失败时继续认证已提交虚拟偏移及当前 HMD 平移，未提交拖动不外泄。released／no_candidate／needs_release 的 accepted 提交可以没有手部授权。

Core 独立 MSVC 测试 4,443 项通过（旧 Broker／Runtime 1,949、原 CF outer 763、新 provider 48 进程场景 1,731）；另 35 个生产 Shared consumer 组合场景通过，覆盖同帧撤销、revision 变化、整个外部 scope 阻挡和 prepare／commit 间撤销。证据为 ignored `action-provider-core1/run-20261025-020201-782171/result.json` 和 `shared-action1-20261025-020340-477559/result.json`。这些是 CPU／生命周期证明；完整候选构建、PG ENTRY 互操作和真实游戏动作验收分开记录。

AR-A9 的初始纯读结果中 optional posture 拒绝 closing，但脚本错误索引其 roles，未执行正权重。A10 改正 schema 后，在任何 Advance 前被 guard_cover 拒绝；Native 的16条原生几何观测证实本局曾处 CT2／CA0、同 Pawn／Link／slot0，最后 host 为 SM0。拒绝瞬间脚本未保留完整 sample／cover rejection，因此不能断言未进掩体、closing 错误或特定瞬态。A9／A10 均完整清理；A10 独立83项、Native／入口、XR0、音频12/12、原十文件和准确自有进程退出通过。后续只补失败原始记录，不放松状态守卫。

AQ3 实际通过 PE／VM 预热，三次原查询分别为 CanDoSpecialMove(4,false)=true、ControlScheme=0、IsHoldingRoadieRunButton=false。随后属性准备累计触到既有32,768 source-check上限，未发送 Try、未持有跑步输入、无 SM4 样本。两个相机策略字段暂设并全部恢复，不计为镜头稳定。独立80项清理、Native／入口、XR0、音频12/12、原十文件和准确进程退出通过，证据为 ignored `cover-host-1025aq3/sprint-1.json`、`sm4-aq3-summary1.json`。后续修复须缩小重复元数据认证范围并保留原上限、归属、类型和 closing 检查。

Native16 normal已完成完整冻结与编译：DLL SHA256 `07837b14539ee81c42eb87aafe56819afc7b8a564533032841db86b072584641`，source16 provenance `121066957d119eb58eb7e939517ac551a80a039f279cf1475e840134e64c4872`。实际source14仅13文件增量、650文件，反向逐字节恢复；原76defines、正式Capture flags、Support3被动诊断、原色及独立左眼history保持。23 helpers、151 focused CTest、独立SupportStartupTrace全部通过；41exports为原37加4，provider48场景1731、boundary328、context102、body2682、geometry953/OFF2、surface208通过。证据为ignored `native-body16-build-result.json` 与 `candidate-cover-body16-manifest.json`。失败Native15是capture forward声明未同步，原source15／provenance／build及失败记录完整保留；16另外修正声明，两个新CPU target显式静态CRT。构建记录诚实保留：同一payload先以遗留CoverBody15.dll basename测试，再逐字节复制为CoverBody16.dll；frame-link随机临时目录只记模板，未伪造原命令。尚无16sim、实际PG ENTRY互操作、Game／XR／头显验收或默认替换。

AQ4独立证书修补后，正版游戏完成一次Try及0.703秒held租约、恢复held后conditional End回SM0，取得107个accepted SM4样本和47个不同相机时间戳；相交完整CSV中61帧为固定source1模拟HMD。所有新增证书保持32,768上限及完整归属／closing，fast sampler字节保持；97项CPU通过。两个相机字段保持候选值并恢复2/2；游戏清理独立80项、Native／入口、XR0、音频10/10、原十文件及准确进程退出通过。实际Pawn根位移269.063单位、Z范围-41.350至1.699，不能将地形位移算晃动；相机相对Pawn的三个局部坐标范围约25.058／0.306／62.071单位，旋转相对PC范围17／0／0 RU。尚未有FP开启、无步态残差、最终眼矩阵或A/B证明，不能宣称稳定已修复。证据在ignored `sm4-aq4-summary1.json` 与本局 `sprint-1.json`。

### 2026-10-25 — 跑步稳定视角候选与共用追踪坐标

Native17只在actual frozen16上叠加7文件（654文件）：独立跑步kernel/live及组合测试，加Capture/CMake/first_person glue。SM0校准必须为CT0、未蹲伏／无蹲伏请求、无掩体／镜像过渡；SM4以当前Pawn根位置和已校准眼高构造基础视角，方向来自PC yaw/pitch，HMD仍只在既有下游组合一次。持续新鲜同context的跑动不设任意10秒切回，支持正负90度pitch；失去有效Scene／身份／epoch／时效时本次可选覆盖不可用，不停止stereo。配置默认关闭，仅在第一人称实际preview生效且非原生狙击scope时覆盖，普通Scene在FP关闭时只读预热。Root另以ordinary规则排除cinematic／mounted／unpossessed。该候选尚未解决第三人称跑动，也未完成实际FP开关／SM4衔接及最终眼验收。

Normal DLL SHA256 `ec3e12f62aa3eee2c8226feb4cd743baf70b76d4050b0c34566c8427d0efc11a`，sim `cf8787ee6947cc944d3095ee24c8ab358719b62e6d1fb480ff0442057628a12d`。Normal实际23 helpers、154 CTest及trace/OFF2/surface208通过，新增kernel1524、live195、OFF2；76defines/mainFLAGS/41exports/既有wire及Support3／原颜色／独立左历史保持。17sim仅一处旧sim guard差异，23helpers以正确17sim文件名实际通过，154CPU依赖闭包不含capture而明确复用normal。首次私有builder遗留16sim名的attempt1和重新以正确名运行的证据分别保存。ignored证据为 `native-body17-ready.json`、两份build-result及manifest；未替换默认。

新增共用 `rig_tracking_math.h`，547项严格MSVC检查通过。它以最终pre-HMD base_view、原reference和原tracking pose映射head／左右手到同一游戏世界，使用完整basis与显式单位；不再叠加HMD位移，姿态转换到UE Xforward/Yright/Zup。level cover+已提交virtual offset与原cover_tracking_point一致，也验证了真实stereo_matrices的两眼中心、双手平移、扒墙抵消、rotation-only、rebase和正负90度。它不包含手骨校准、authoritative aim证明或setter许可。已将精确header交武器会话复用，共享原始输入来源及新Rig数据快照接口另行接线，未扩展冻结hand V1。

AR-A11实际初次neutral父CoverBlend缓存完成（node／mesh tag同2400）；后续保存的freshness读值同3121，未满足“旧node／新mesh”。该额外镜像写入条件不适用于没有mirror写入的A control，A12只对A保留completed cache而不请求fresh-cache，B仍拒绝。A12到0.25请求后因baseline变化停止，但拒绝数据未先保存，不能归因某一字段。A13没有启动coordinator，空跑到期；独立83项清理、XR0、audio10、原十文件及准确进程审计通过，原缺review finalizer失败保留。A14将host与coordinator连续启动，零位四骨前的child5 typed interval拒绝，未进入正位移；完整83清理、XR0、audio10及原十文件通过。后续不将这些计为侧姿态通过，同次原生pose相位观测用于替代跨多次远程getter的姿态推断。

AQ6已在正版游戏实际运行Native17sim的第一人称／跑步候选。一次0.703秒原SM4 held租约取得112个accepted样本、54个不同相机时间戳；Native实际751次prepare，697次校准、54次应用、0拒绝，校准眼高60单位，FP应用609次且无sniper scope。相交CSV的62帧均为固定source1模拟HMD；这证明候选确实在实际SM4期间应用，尚不证明最终眼矩阵无步态残差或实体头显舒适度。保留camera_stability_accepted=false、final_rendered_eyes_verified=false，未做A/B或默认替换。独立80项、Native清理、入口还原、XR exit0、音频10/10及原十文件保持通过；停止后无自有Game/XR存活。证据为ignored `sm4-aq6-summary1.json`、`cover-host-1025aq6-camera-final1.json`。final1末尾“camera OFF”是旧模板文字，与实际配置／计数不符，原文件保留，另以`sm4-aq6-acceptance-boundaries1.json`明确本轮边界，不据此扩大验收。

`world_state.Query.finish()`现在保存首个closing拒绝的identity／metadata／registry前后值，registry另保存pointer／count／capacity；原拒绝谓词、短路顺序、读取序列／次数、异常字符串及growth语义不变，`CoverQuery.finish()`未改。20项测试对照旧实现通过。AQ6仅在首次FP toggle前，依据明确记录的registry count增长允许最多两次、三秒内的全新只读认证；第二次必须完整通过原strict gate及完整cohort／allocation核对。其他失败立即拒绝，toggle后不重发或重采；这项夹具处理不赋予生产runtime新权限。

共用Rig接口的provider与Shared consumer已完成16＋36个CPU场景、1,450项检查，MSVC x64 Frame为1824字节、ReadRequest为40字节；冻结hand／call／action V1不变。原匹配GVC3只读一次，额外continuity companion必须与该输入及publisher完全匹配；缺失时producer计数为0，不伪装GHW1证书。新Native reference observer在重置／重新居中／追踪失效的状态改变前原子撤销旧快照，4,888项observer检查及既有stereo测试通过。Root已接入最终pre-HMD基础视角和只读export，正准备Native18组合构建，尚无该组合的真实跟手验证。

新增只读pose阶段观测器：在原Compose／Physics／外层Tick保持原调用与异常转发，事件触发后最多8个episode、每个最多7阶段，记录LocalAtoms／SpaceBases及相关矩阵到ignored二进制。实际编译代码先通过完整hash证书；默认关闭、不写骨骼、不授予独占或setter。core116、真实MASM/ABI/SEH492、live147项检查通过。该观测器与Rig的完整Native18构建和游戏采集仍待验证，不能将静态阶段关系或无观测并发等同安全最终写入点。

Native18b normal完整组合已通过：实际frozen17的654文件加21文件最小闭包为670文件，所有变化逆向逐字节还原；保留原76项渲染defines、actualCaptureFlags、Support3、原颜色及独立左眼history。新test-only fixture cache单列，未改渲染配置。DLL SHA256 `bc588e78c8c7cc4927f6ba4cd3fcd3a8c26ea27f9732cd6dc2007b1a134eced8`，42exports；实际fresh编译、23helpers、176focused CTest、原StereoFrame独立实跑、trace/OFF2/surface208通过。Pose core116／thunks492／live149、Rig720／Shared730、math547、observer4888。最初18的编译及23helpers已经通过，但一项CTest漏传私有fixture，保留175/176负例；18b只改CMake测试参数和测试输出唯一目录，production capture与18完全相同。构建脚本旧source变量与typed-cache参数的前置失败也原样保留，没有放宽测试。证据为ignored `native-body18b-build-result.json`、manifest及source provenance。

18bsim也已READY，DLL SHA256 `cf6bcecd98dcfdb76ba9c9798082af08c490d94e369f1657dff3a1437664c3fe`；仅历史operator guard一处差异，670文件及逆向核对通过，实际fresh编译并以正确basename运行23helpers。176CPU／trace／OFF2／surface检查经实际源码依赖闭包证明后明确复用normal，没有冒称重跑。最终`native-body18b-ready.json` SHA256 `adb69c39faa0481aea775a77423418d21ed53cedc16c30c47b4c4eb79f14cee4`。继承的旧报告文案另有修正回执及原备份，DLL和原日志不变。只读Pose文件parser通过离线检查，首轮收敛为一次事件；等待最新菜单同源XR source1sim后再进行新游戏采集。无18b Game/XR、真实跟手、完整身体或头显验收，未替换默认。

上段首次交接之后，构建agent又删除了报告继承的gait-only／excluded-Pose-Rig矛盾字段，连带更新sim manifest／provenance及READY pins。武器会话据旧pin正确拒绝接收；原`adb69…cee4`字节完整保存在`native-body18b-ready-pre-semantic-metadata-audit.json`。Root重新核对9个关联文件后，用CREATE_NEW另存固定选择`native-body18b-ready-semantic-final1.json`，SHA256 `943fea2b3e0def7899c9ead53c47fc2bcc8a3ead4e11e01c82a49348ac51c304`；最终sim manifest为`77f94b9447f9cb091f491509122b72ea9025be9fb6738c67887c974e6b741c01`。交接回执`native-body18b-handoff-final1.json` SHA256 `f47c9bc04e4dad1d9117d37d6f3253a3bcb35bb7c4b711859628977769855664`。两个DLL与原源码／运行日志保持，未重建或重测；后续采用新命名文件，不继续覆盖交接选择。

配套XR source1sim已交回，基于最新menu-modal1同源normal制作，仅私有sim选项及9个限定hunk；原菜单、颜色、CSV尾部与normal输入采样loop保持。EXE SHA256 `1a059d2d3b6aed6b4f8911ee1790f5051a1908b0d91515f3451655e535a2b4e9`，manifest `c189bcca43d4a41420359fc36acb39100a00cbe0cb3e0174f091948539befa80`。Root独立核验7个EXE／loader／源与CPU回执／外部runtime及operator文件，均匹配；证据为ignored `pose-phase-ro1/xr-receiver-preflight1.json`。实际runtime身份、完整模块路径及本次XR进程的fresh8720监听仍须在拥有的会话启动后核对，文件校验不替代实际运行验收。只读Pose单次采集夹具准备中，此处尚未启动新Game/XR。

### 2026-10-25 — 实际姿态阶段、骨名与原生数据适配

Pose RO1首次实跑在Native worker启动前因私有child的`prime_owner`导入路径错误退出，未发送事件；原失败／incomplete、XR exit1及finalizer正常停止timeout均保留。独立事后核对无准确自有Game/XR进程、音频10/10已恢复、PG／early prime恢复通过，host原十文件比较保持；没有补造完整Native独立清理通过。RO2只修三个具体child导入路径并验证真实CLI闭包，保持所有Native/XR／动作／权限和原只读协议。

RO2正版实跑成功，0.4 scale、behind／observe-only／mute、后台物理输入关闭；仅一次SetEvent，取得1 episode、594792字节，SHA256 `29ca02928e54a7b42711a4e3adbc11cd801b5df2ca9db8603d0a21c3f6b36110`。C0／C1／Plate0／Plate1／L1五阶段有效，main physics两阶段未经过；无exception／nested／duplicate，观测concurrent为0不等于独占。状态receipt为CT0／CA0／SM0、link null／slot -1，且不是与pose原子同epoch的掩体认证。正常停止、Native／entry、独立88项（含三入口完整8个已知code chunks）、XR0、音频10/10、原十文件、PG0／stopped、tiny monitor和准确自有PID清空全部通过。证据在ignored `pose-phase-ro2/runtime-final.json`及`runtime-binary-parse.json`。

实际115骨C1满足行向量`Space=Local(q,t,s)*ParentSpace`、`World=Space*component`，重建最大平移误差3.668e-5游戏单位、旋转元素误差9.664e-7；C0不能当作当前local已组合完成的space。C0→C1只有骨号8／10／11／32／34／35的local改变；C1至L1的四组原始数组在记录端点相同，不能据此称callee不写、GPU已消费或physics碰撞体已更新。后续静态证据已具体收敛到DD91C／真实vtable方法、12E330尾调用和physics接收者；F20F4是矩阵归一化、F2610是48字节转换，不能再当body getter。定向新ASM与原历史blob全chunks核验，原IDB保持；IDA连接插件缺模块时仅用既有本地CLI私有副本，不安装或迁移工具。

新增`cover_skeleton_ik.h`的FK、world pose提取及world→local／后代适配，统一复用武器会话`hand_two_bone_pose.h`，没有第二套长期双骨solver。固定长度不可达明确失败；adaptive完整目标通过显式lower/end local translation调整实现，保留scale及其他local原字节。数据只写调用方私有数组，不赋予游戏内写入许可。严格MSVC synthetic486项、含真实C1 fixture共2040项通过；4单位位移／返回／完整roll和超出原长度的adaptive目标最大位置误差约2.71e-5游戏单位。CMake两个focused test通过，未重跑无关176 suite或重建Native DLL。独立构建使用frozen18b的最小四文件增量、673文件；初始worktree缺ignored MinHook、复制文件只读及日志解码失败均保留，未装依赖或改旧冻结树。证据为ignored `skeleton-ik-cpu1/delivery-shared.json`和`skeleton-ik-cmake2/result.json`。

武器会话新bv只读会话进一步完成同session骨名／Ref raw／本次Pose parent闭环，Root实际核验join SHA256 `00511f9aace8ba9dfa16ba7906edd18179b6af8dd8a1a2ce50df814383df245e`及final SHA256 `fe2b7dad240bce2aec9e63f168cfcea39f244a6ffee42fb3631697ec96203d91`。当前115骨模型的左链为`b_MF_UpperArm_L` 8→`b_MF_Forearm_L` 10→`b_MF_Hand_L` 11，右链32→34→35；113／114是另外的IK helper，不能混用。仅按该asset／Ref签名绑定，不能全模型硬编码。该轮Native／entry／88独立checks／XR0／原十文件／音频10／PG及准确PID清理通过，所有writer关闭。下一步是另做默认关闭、有界且可恢复的Native C1姿态写入试验；当前仍没有真实自由双手、完整掩体身体／碰撞或最终发布验收。

有限姿态写入纯IO草案尚未接入Native或进行Game/XR写入。武器会话独立审计冻结草案`c2ecc4c3…9e`，以合法原生Compose修改Local的新增回归确认旧C0==C1前提会拒绝实测行为，原失败保留。另发现C0逐字段归还失败可能留下混合姿态、C1／L1失败阶段不能正常收尾、Space责任提前清除、非空desired数组未纳入别名检查。修订中将原生C0输入、本轮原生C1输出、本次overlay值和后续归还条件分别保留；C1原始输出是即时回滚的精确目标，新动画输入不得由旧姿态覆盖。任何不确定或部分写入不能当作消费者可以继续运行；实际Native入口、并发消费者顺序与故障出口仍待实现／验证。审计与回归在武器worktree ignored `analysis/hand-c1-overlay-review-20261025bx/`，此处不把host测试或caller proof位提升为实机写入许可。两会话共用solver，grip到wrist完整位置／旋转标定由武器会话独立小模块推进，未修改另一worktree。

共用`hand_wrist_calibration.h`已按字节接收，SHA256 `d73d075c7b854c14addb5fa919292d6301f4116dff0b864987009b32ccdad224`，与武器会话原件及`pose_math.h`依赖完全匹配。接口需要显式已对齐的grip／wrist pair，保存grip局部米制位置和完整四元数偏移；默认unknown拒绝、失败不改输出，任意原版动画姿态不能自动当成物理标定。Root以实际本地header严格CL编译并通过10组CPU检查，包括非交换旋转、完整roll、单位只换算一次、刚体协变及溢出拒绝；证据为ignored `wrist-calibration-cpu1/result.json`。加入未来组合构建的focused CMake target，本轮没有重建Native DLL或重跑176 suite；实际资产／左右手绑定、标定来源、存储交互与跟手验收仍待实现。

有限overlay C3的纯IO修订已冻结为header SHA256 `2bce0f10b7fa8cb0517ecbe821236cc6e5c9c1019b001b8fb39800ecda87941b`，strict CL synthetic7031 checks／39 scenarios、含真实RO2 C0／C1配对的8455 checks／43 scenarios通过。真实六骨变化保留，即时失败精确回本轮C1原像，新动画输入不由旧姿态覆盖；有债务的失败明确fatal／保留责任，Space不再凭近似FK归零，desired与metadata/control别名检查补齐。Root实际重核源码与新回执hash，并交武器会话独立窄审；回执为ignored `overlay-trial-cpu1/delivery-native-c1-c3.json`（SHA256 `64d76d6efddc705a661c5df55e522f67df1831a39a5a5b2be05ada8aa12e0189`）。没有Native接线、游戏内写入或真实读者同步许可，这个草案不作为实机READY。

定向ASM审查进一步确认Compose按有效AnimTree `mesh+230`、分轮标签和control链选择输出；`master=0`／desired全覆盖不足以保证全部Space本轮重写。Root据Astra建议优先定位原生手臂SkelControl实际目标输入，让原Compose自行产出Local／Space并沿既有physics／render传播；C1直接数组overlay保留私有验证。最小只读采集沿用已有18bsim／source1sim／0.4后台协议，新增受控mesh实际AnimTree、逐骨control map、链头和真实虚目标inventory，随后才针对实际计算实现确认effector／弯曲方向／目标空间／完整旋转来源。未修改strength／LOD来强制启用，未增加setter、重复Compose或PG前向写权限。当前仍处于采集准备，不把inventory等同控制器已执行或碰撞体已更新。

RO3在event前因实际AnimTree class不可用拒绝。Root复算已保存指令`1289B1: 48 8B 0D 98 8E AF 01`，确认`next RIP 1289B8 + signed disp 1AF8E98 = runtime RVA 1C21850`；IDA显示的数据符号`1C20850`是raw-loader注释，不能直接当运行时RVA。原失败和无event保留，Native／entry及88独立项（8个完整chunks）、XR0、音频10、原十文件、PG0及准确PID清理全部通过。RO4使用新实例，只对当前未hook的7B指令逐字认证后解码signed RIP，不在活进程修改采集器或重试。

RO4前置inventory独立accepted，取得受控mesh的12个control节点；骨号11、35分别映射到Limb→SingleBone→Limb→SingleBone链，原strength均为1／1／0／0。这里的侧别仍需本次ControlName／Ref FName归属认证，不能凭跨进程相同骨号补名。实际Limb虚槽230／238为RVA `3A4908`／`1D2A04`，SingleBone为`19E2FC`／`1D396C`，root虚270为`F7940`。标签count115、取值0／1，原始标签与parents保留；inventory的逐骨分轮计算显式假设全部骨在desired，不能当本次真实Compose覆盖证明。前置文件SHA256 `bc93485fca5ae6236dccdb173f18e24b15605fa2f653affd31d2e805e4de5bf1`。

RO4唯一event已发；后置严格CoverQuery因值在采样中变化而拒绝，整轮`accepted=false`保留，没有把前置独立窗口拼成原子Pose或后置通过。完整清理88项／8chunks、XR0、音频10、原十文件、PG0／tiny和准确自有PID退出通过，事后Game／XR／8720空；final SHA256 `d556c81c8732af7b4c6b7382806e1cb390930e6e01ca85583905c3b13dd6349e`。Root已检索主知识库并只在新私有IDB导出这5个实际函数ASM，全部chunks匹配历史blob且前64B匹配本次实机读取；完整函数当前运行时字节仍未认证。原IDB hash保持，CLI exit0／2.156秒；衍生证据留ignored `ida-body1/native-controls1/`，不进发布包。下一步审实际目标字段、空间转换、完整旋转及长度限制；尚未进行control目标写入或自由跟手验收。

武器会话独立C3复审仍指出两项P1：Local按整个16B／12B字段仅因部分字节不同就判native-replaced，会丢失仍含overlay分量的责任；Space各分量不同也不能证明来自完整原生输入重新生成，1ULP变化不等于恢复。这些不是host已有checks覆盖充分的证明。C3保持未接线、未启用和私有回归用途，原冻结源码不再变动，不以再开C4替代当前原生control主线。复审为武器worktree ignored `analysis/hand-c1-overlay-review-20261025bz/astra-review-c3.txt`，SHA256 `775c53f4c4bedbf92617442b7f99675ed53ef30eb1170a1b4c89551724828d8c`；后置动态读取拒绝的具体字段也仍未定位，不隐去失败或认定它只是无关变化。

实际原生Compute合同现已收敛到临时输出：Limb v238给3个component-space Matrix64，按upper／elbow／wrist排列，Single给1个wrist；原Compose负责从这些矩阵生成Local／Space且不再次限长。`cover_control_output_pose.h`复用exact shared solver，默认preserve、不可达拒绝，显式adaptive可达完整目标；Single直接替换3×3完整旋转并逐字保留translation／affine bits。仅接受proper unit-scale。strict CL synthetic417 checks／24 cases、含RO2 C1编码样本475 checks／27 cases通过；C1样本不是v238实际输出采集。header SHA256 `67109c6d79698a6759e156c5747c66bfe6254f4a760214ccd81f55788ca45b36`，回执`control-output-pose-cpu1/delivery-unit-scale-v1.json` SHA256 `553dff2cc86f46bada9635533b3412ee5bbc3ce768a8213166c24ddb5edcfd76`。尚无运行时写入或真实跟手验收。

v278权重和v248额外scale明确是两项调用：分别return12911F和129474，v240还可能返回per-bone scale数组。首个输出候选必须分别核对真实alpha、g、scale数组和Single的ApplyTranslation；矩阵轴长不能证明LocalScale保持。Root已准备默认关闭的三入口被动观测：Limb／Single原Compute与原weight getter，依附已有Pose event，记录真实栈header、临时矩阵、节点参数、vtable目标及原weight返回；v248只读目标，不额外调用getter。5个完整历史chunks共5891B证书已由原blob及机械导出生成，运行时仍须在hook前认证；getter确为6B `vmovss xmm0,[rcx+70]; ret`，本轮该6B也与RO4读取吻合。观测接线／测试正在实现，没有将WIP编成新候选或覆盖旧默认。

Native内部Rig数据源与public registration耦合另行拆分。已按字节接收武器会话的独立`CoreSnapshot`1832B／`NativeSessionKey`24B，不含public Frame或假cookie；public V1仍1824B／ReadRequest40B且真实cookie检查保持。Root第一次receiver因本地`stereo_frame.h`依赖差异在复制前拒绝，未复用不相同依赖的CPU结论；第二次用实际本地headers和冻结旧V1严格CL，8组204581 checks通过，包括原行为差分和有效payload全bytes投影。Source／实际include和前后hash确认，证据为ignored `native-rig-core-receiver2/actual-local-result.json`。这是纯数据层交接，Scene builder／Native-session cache／getter／outerTick一次latch仍未接入；不重读bridge、不刷新源时间、不把Scene改名为Tick，也不赋骨骼写入许可。后续Native19只读观测先从实际frozen18b最小增量构建，维持其原V1 header与全部76旧defines；Core live迁移另作明确增量。

Native19只读control观测现已完成最小源闭包构建：从实际frozen18b的670文件派生为682文件、15项增量，保留76个原defines和原capture flags，仅增加observer capability。normal DLL SHA256 `4f601096265f02af844eec11ee6b07c17ee360657a4effe8a10276465f10aeb4`，23个实际DLL helper与183个focused CTest通过；sim DLL SHA256 `8b7cc0eb276de778864e5988208c070670e9a6cdfd148bef201afe5c70b7f8fc`只保留既有私有simulator兼容谓词增量，23个实际DLL helper通过，183项CPU结果按真实依赖闭包复用并明确未重跑。来源和CREATE_NEW选择固定在ignored `native-body19-ready-final1.json`，旧18b与19来源、二进制、结果均保持不变。这些结果不覆盖真实MinHook安装、控制器实机采样或跟手写入。

`control-output-ro1`使用19sim／source1sim／0.4后台隔离协议，在派发唯一event之前被control observer的configure/reserve共用保护拒绝，未产生CCTRL或CPOSE。该错误文字尚不能区分完整代码hash、入口prefix或实际MinHook失败；getter使用VEX是待验证假设。整轮accepted=false，原失败与finalizer正常退出超时保留。session为incomplete，Native cleanup=false、entry恢复未验证、XR exit1，不能称完整清理通过；PG恢复通过、音频10项恢复、原十个设置／存档hash未变，00:40 UTC复查准确自有PID及Game／XR进程为空。补充回执为ignored `control-output-ro1/runtime-failure-audit1.json`。接下来先补足拒绝分段证据与真实本地MinHook试验，不能绕过代码匹配保护或将缺失样本补成通过；最新默认未替换。

Native20仅从实际frozen19派生三文件拒绝日志增量，Row1944／Header72与所有动作保持。normal SHA256 `b7c2ac308f825d711166f001af0295b41227eaf5b17920e1f14c943185f080c0`，23个实际DLL helper和183个focused CTest通过；sim SHA256 `98fa60907c636fd5e43a7725e1fb8e39f0fabe632536e2b2f473585e157deb55`，23个实际DLL helper通过、183项按相同依赖闭包复用。stage／RVA／MH_STATUS区分代码hash、入口读取／匹配、MinHook创建与创建后入口变化；strictCL实际thunk528／live690／两个OFF组合各11通过，源和新回执另存，原19与旧失败不改。

`control-output-ro2`实际0.4后台启动已定位同一预激活拒绝：`minhook_create / RVA515BB0 / status8 / certified1 / reserved_mask3 / calls0`。这表明五个完整known chunks已在Native先后两次匹配，两个Compute成功预留，随后exact VEX getter被MinHook以MH_ERROR_UNSUPPORTED_FUNCTION拒绝；代码证书仍不是独立还原证明。隔离自有VirtualAlloc试验直接链接19和19sim各自实际库，两次各172 checks通过：原VEX getter可CPU执行，HDE却len1／F_ERROR_OPCODE；经典SSE getter的真实trampoline／detour／禁用还原完整通过，两个Compute只Create／Remove且不执行。冻结报告为ignored `minhook-control-cpu1/delivery.json`（SHA256 `628b386f120c4e962a2c1d26324185de8ce6ab17a445e6dd2cc98bf690a79144`）。仅在取得20的实际错误后才确定它是本轮激活阻塞点，未反推其他历史故障。

RO2未发event、无CCTRL／CPOSE，accepted=false。正常stop尝试及finalizer超时保留，Native cleanup=false、entry未验证、XR exit1；PG恢复、音频10项与原十文件通过，00:50 UTC后准确自有PID及Game／XR进程空，补充回执为ignored `control-output-ro2/runtime-failure-audit1.json`，不能将进程退出改称完整Native清理通过。当前推进仅首指令exact六字节getter的最小decoder兼容候选，须实际验证trampoline调用、相似编码拒绝和原像还原后另建21，不换SSE或广泛开启VEX、不改vendor／旧冻结源、不绕过代码匹配、不替换默认。

Native21从frozen20派生为684文件，只改CMake、增加build-local trampoline生成器和真实MinHook测试，不改vendor原件、capture逻辑、控制参数或目标数组。生成器要求原trampoline完整SHA256 `824ed887ec9da606960962af8973205d1f97cd03241e75517eba485280fb6377`、唯一原decoder guard及修复后SHA256 `825edb0f2e75fc4a72d2f1f374040aa672badfd997aed27ded78ae7b86bdd4b3`，仅observer ON时接受首条exact `C5 FA 10 41 70 C3`。错误来源明确拒绝，其他解码错误仍保留；normal／sim／OFF四个MinHook C对象实际defines和flags与20逐项相同。隔离fixed实际242 checks、baseline206通过，正例真实trampoline／原返回／禁用原像恢复，四个相似VEX反例仍拒绝。OFF的实际单独CMake库＋测试再次baseline206通过、未生成修复源。仅Root第一版21构建脚本错误残留source20，Ninja以新测试target不存在拒绝，未接受DLL／manifest；失败目录和脚本保留，正确构建使用`build-cover-body21a`。

正确21 normal DLL SHA256 `0a3db8c0a48bceaf8b3dea6f1bac160099438e4d90acfb191ac714a70083ac27`，23个实际DLL helper与184个focused CTest通过；sim SHA256 `841f88cba1b4dde010ffb3bbb2e4b0facc543edbec5f1830425f6ad457153f8e`，23个实际DLL helper通过、184项CPU按实际源／Ninja依赖闭包复用。原76+observer defines、42导出、原色、独立左眼history和冻结支持／菜单来源保持；选择固定为ignored `native-body21-ready-final1.json`，不改旧19／20和正常默认。

`control-output-ro3`首次成功完成新通路实机观测：购买版、source1sim、0.4、behind／tiny只观察／测试静音，Body／Input／Camera／Run／FP／Rig／Weapon／engine_restore全部OFF。场景为普通CT0／CA0，非掩体附着证明。唯一event取得同一episode的8条CCTRL（15624B，SHA256 `597b2d01a9abe82aaff898b67d505ad770a4a16150cdfb018c9bbffaa923f5b7`）和CPOSE（594792B，SHA256 `103e331a0c1c79e2c6eeddb40e57a38fc279027851c5d6db6d0bf80f3ebedfc0`）。control reserve mask7／MH0、rows8、rejected／nested／protocol／failed均0；Native／entry完整恢复，96项独立检查全匹配（control8＋原Pose8包含其中）、XR0、音频10恢复、原十文件hash未变、PG／tiny／准确自有PID退出全通过。final accepted仅指既定只读采样与清理合同，非writer或完整玩法接受；证据为ignored `control-output-ro3/runtime-final.json`。

武器会话冻结v2 decoder按真实MSVC Row1944／Header72实际解出8行，4条Compute与4条真实weight完整对应。首次join因Root finalizer直接parse函数的报告缺layout_sha256被正确拒绝，原失败保留；随后以同一个冻结Pose parser的原CLI重解同一binary生成新报告，核心字段与旧报告逐项相同，原输入hash不变，真实layout SHA由原CLI写出。新join qualified_compute_rows4／global_issues为空，记录在`offline-receiver2.json`和`runtime-control-pose-relation.json`。两条当次骨链8／10／11与32／34／35各产出Limb三矩阵和Single一矩阵，真实alpha均1，Single+C8均4／ApplyTranslation=false；+AC字段前后1、v240目标C3F20为RET imm0空函数（不是数值0返回）、v248目标516100为+AC float getter。scaleHeader和未来g返回仍未采样，端点相等不证明未来值、reader fence、最终渲染或physics；CPOSE的Pmain0／Pmain1仍缺失。

Root再以这次真正v238临时矩阵和同Row component-to-world生成两个264B私有数学夹具，调用实际21构建的`CoverControlOutputPoseTest.exe`，两次各475 checks／27 cases通过，最大preserve／adaptive位置误差分别2.081e-6／3.201e-6原单位。测试程序stdout继承旧C1样本标签，原日志保留并在回执明确真实输入是本次v238，未冒称物理grip输入。证据为ignored `control-output-v238-math-cpu1/attempt-1792890843986105300/receipt.json`。数学合同现在覆盖真实输出编码，仍没有Native手臂／身体写入或碰撞验收。

后续提交点的定向审计建议采用原v278返回后、12911F返回前，此时已取得实际alpha且v240已完成，仍可更新未消费的原scratch输出。关键ABI：weight入口R9是v240遗留scaleHeader=原RSP0+70，不能当pose out；必须取与关联Compute相同的原RSP0+80，affected=+90，hasScale=+48。现有weight行的三个header／矩阵是未采初始化值，不能据此开始writer；需要先在该阶段补齐真实读取／复读、scale与未来g条件、当前目标／code／owner／cohort／age与故障处理。原v238路线只读结果和这个更窄提交阶段不授writer／物理fence权限；审计保存在ignored `control-output-ro3/native-control-output-post-weight-audit1.md`。独立Native Rig core的Scene发布／store／native getter候选由武器会话在其私有analysis继续推进，两边现有live源码无交叉改写。

Native Rig provider v2按武器会话冻结交付接收，37项实际本地依赖全部吻合，仅约定6项源码／fixture变化；Root另以真实本地headers严格CL重编跑完legacy16、Core26及真实Broker guard注册窗口，共43次执行，来源前后hash稳定。内部缓存不依赖public注册cookie，公开V1仍以真正已发布的registration_current校验，不以hand授权代替注册；新增非copy TickLatch保留原Scene，四node用同一key／快照，失效后sticky、错end不清原key。接收回执为ignored `native-rig-live-receiver1/attempt-1792891869053074900/receipt.json`。共享provider独立提交`d051cfe`，包含全局Core头接入，便于后续与武器会话合并；没有修改另一worktree。

Native22从21最小派生691文件／16项变化，保留21的77个defines与实际capture flags，只增加Core event observer capability。Source／default／vendor旧件保持；normal DLL SHA256 `c9c9341085299b3d255a8235935caec0ee32d8c1d41ea33b7cf2f4f55d6d9837`，23个实际DLL helper与239个focused CTest通过，含旧公开V1的204581差分检查、Core26／Broker注册guard及新observer的24个ON模式／3个OFF组合。首次收尾脚本把JSON ABI误当普通文本，正确拦住报告；原失败保留，第二份finish脚本读取真实verified-build及已成功的23／239回执，验证JSON Header112／Record1920／Snapshot1832后只执行未跑的剩余检查，没有重编或冒称重复测试。sim DLL SHA256 `05c7e44d9d46971bc3b855d4e82fee89add23e1d7d18ec44140cea3b1f6663b7`，23个实际DLL helper通过，239项按实际源／Ninja依赖闭包复用。42导出和公开V1 wire布局保持。

新增`cover_native_rig_observer_live.inl`默认关闭，只在第一个已认证event outerTick持有真实全局Scope的world admission，独立TLS Frame保存非copy latch，四个Compute回调在CCTRL锁外读取同一latch。原tick finally在Pose finish／CCTRL结束后、release前执行exact end；stale／异常／drain仍可释放，错end／未知责任保留pending。新增pending进入stop、native cleanup、资源retained和entry-restored检查。局部strict MSVC182 checks为Rig／Phase／CCTRL／World stub，独立Astra窄审没有可落实P1／P2；不把它们当真实连续Owner或执行权。真实Native生命周期由下述试验另证。

`native-core-ro1`使用0.4／behind／tiny只观察／测试静音，Rig和WeaponHandParticipant仅数据runtime启用，没有注册、cookie为0；Body／Input／Camera／Run／FP／engine_restore仍OFF，source1sim仍head-only。真实一次event得到Core sidecar7792B（SHA256 `24ab74c4fac560ace18e16e92b0b81c7259ae469ce38d3b4c496918d319c662c`），1次Core getter、1次latch、4次read成功／0拒绝、4记录／1 exact end／0pending／0failed；Rig发布336、拒绝20，启动／场景拒绝未进一步归因，不能写成整段零拒绝。Scene264与outerTick27090／Compose27091保持分开。C++检查器直接使用22实际Core／observer类型，四份1832B快照逐字相同，publication262／sourceTicket263／refRevision6／published_ns不刷新，world_pose_mask=1；纯payload在ComputeEnd时刻有效，不等当前liveness或写权限。另从实际文件构造8种私有损坏副本，截断／尾字节／writer／count／session／ordinal／无效payload／有效但不同ticket都按预期拒绝，原文件不改。

同轮CCTRL8行、4组Compute／weight关联及CPOSE解析通过；Core的4条记录与CCTRL的线程、session、Scene／QPC／generation、outerTick、Compose、ordinal、node、kind、end bone和前后QPC逐项同。联合回执为ignored `native-core-ro1/runtime-core-control-join1.json`，检查器／快照回执为`runtime-core-parse1.json`。Native／entry完整清理、96个独立范围检查、XR0、音频10恢复、原十文件hash、PG／tiny和准确自有PID退出全部通过。CPOSE依然没有Pmain0／Pmain1，这一普通CT0场景不证明掩体、手追踪、后续Compose成功、physics／render fence或连续跟手。首次只读真实调用通路已打通；独立连续Owner、已知模型／完整手腕目标、实际临时输出写入及掩体身体动作继续实现。

21的ready-final1误继承logging_delta_only=true已由独立源码审计发现。旧frozen、pins和RO3原报告不改，新增`native-body21-ready-clarified1.json`（SHA256 `96e957bd7c760793c5c01529b40d802d7ef25e54bd7bdc777e830056f5417c62`）明确21包含exact getter兼容修复／测试，所选8个实际产物逐字相同；无需因元数据描述重编DLL。22的选择没有沿用这一标签，默认清单没有替换。

共享hand target包已接收：`hand_authored_mount_profile.h` SHA256 `63bdc29eae4ad9cd49c3ae9a17242e680a9acaa9785a1dd9aaefe32f44f11ec7`、`hand_core_target_adapter.h` SHA256 `5bd88f2c983e7c17d592eff9b8aacccc6de445ca8252ec0672ae883c002ca477`和Single数学最小修复`cover_control_output_pose.h` SHA256 `2bc3f16d80a64f821507869750f9fdfe5bc4e1bd29727dfd63507c76141620f8`。实际本地17依赖逐项匹配，仅上述3生产项／2测试变化，旧math与22冻结源保持；Root严格CL重编profile699、adapter7组603、旧math417检查均通过，实际include与前后hash固定在ignored `hand-core-adapter-receiver1/attempt-1792896688960933200/receipt.json`。Host Core／binding和实际RO3矩阵的混合fixture不等同实际tracked手或执行权。

默认`marcus115-openxr-grip-authored-v1`明确为designer_authored：静态骨架的四指根几何定义掌心marker与握持轴，不以任意动画腕和实际grip自动校准。选择依据为115骨index／parent／qT28构成4140B canonical SHA256 `3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193`，不是骨数或跨进程FName整数；RO3和当前core1五个有效参考阶段均与之逐项匹配，其他模板仍拒绝。每手一次prepare生成完整腕目标，Limb和Single复用；单位按model scale和Core.units换算。当前owner／asset／source／stop及输出寿命与执行许可继续由Native端认证，物理掌面、手型、模型外观仍待验证。

实际Single row2／6的m15为0.9999998807907104／1.0000001192092896，旧严格affine验证会拒绝合法实机输入。修复只在Single的验证副本允许finite abs(m15-1)<=2*float epsilon后置1再走原rigid检查，实际输出七个非3x3字段（含m15）逐bits保留；不是把真实矩阵normalize，也没有放宽Limb／component-to-world、shear、reflection或scale条件。原失败397和真实Single正例／超过阈值等负例由交付包保留，两个原生Single现在真实通过full-rotation规划。尚未进行live姿态写入。

`native-core-ro2`在同一Native22sim和冻结XR source1sim上，仅让私有XR factory增加`--controller-input`，没有启用旧Native controller observer或任何writer。0.4／behind／tiny只观察／静音的正版实跑取得双手grip_flags15／active1、world_pose_mask7；同一outerTick24515／Compose24516的四个Core副本逐bytes相同，publication237／sourceTicket238／refRevision6，保持原Scene239及时间而未重标。只调用1次getter，4次cached read均成功，exact end后pending0；没有public注册，cookie0。实际7792B文件SHA256 `34ad0b06e6135544310cd17f7613a64d70a87235219a8a362b795ef210366cf6`。typed parse及与同轮四组CCTRL的node／ordinal／Scene／QPC关联通过，保存于ignored `native-core-ro2/runtime-core-parse1.json`、`grip-flags-inspection1.json`和`runtime-core-control-join1.json`。完整Native／entry96项独立恢复、XR0、音频10恢复、原十文件、PG／tiny及自有PID退出通过。此轮是默认模拟controller pose数据接通，不是翻腕、物理掌面、连续跟手或写骨骼验收。

新增默认OFF的`hand_goal_observer_live.inl`在同一次event的四个原Compute之后，只读规划双手完整腕目标及Limb／Single输出；不写Game／node／Local／Space／input，不追加Core getter或bridge read。C0仅提供owned Ref／metadata，live asset身份、Ref header及内容前后复核，已知Marcus115 signature才选authored profile；每手prepare一次，Limb与Single复用同腕目标／同Core。Single只规划旋转，保留原七个affine字段，不能据此声称平移已经到达目标。子agent实际519项CPU检查通过；Astra对Root接线的窄审无当前只读范围P1／P2，明确冻结快照收尾检查不代表返回时source仍有效、`planned`不授writer许可。Root另建typed二进制解码／数学重放工具，实际受控fixture正例和10个损坏副本拒绝通过，原生Game数据仍待Native23组合构建及采集。所有fixture与派生证据留在ignored analysis，未替换默认。

Native23第一次组合编译发现Windows RPC的`small`宏污染shared solver的局部变量；原23失败源码／构建保持，只在capture的adapter include周围push／undef／pop该宏，未改shared算法。Native23b以697文件、4既有项／6新文件从22的实际冻结源派生，并保留逆patch；78旧defines及实际渲染flags保持，只增默认OFF的HandGoal capability。Normal DLL SHA256 `0131375505ac8a881151de31aa2fc89bed1a204315175e7318b96c89a238e212`，269项focused CTest及profile699／adapter603、原Stereo／trace／surface／编码检查通过；sim DLL SHA256 `795a432fa60691189703db4fe3b9bfe57d253aca6fc0add812f81f387ef5c7a2`，仅旧simulator guard差异，实际编译与预检通过，CPU按实际源／Ninja依赖及flags审计复用。42exports、Support3、原颜色和独立左眼history保持。两次报告／路径规范化断言的失败原样保存，finish只继续未完成的验证，没有为元数据错误重编DLL或重复预检。

预检计数口径更正：本构建系列继承的23项preflight实际上包含15项加载所选DLL的CaptureSelfTest和8项独立component执行；历史“23 DLL helpers”的标签不准确，保留旧frozen报告并以本说明限定，不能按23次DLL加载理解。Root的hand parser另对actual frozen23b headers严格CL编译，正例与10个损坏副本拒绝通过，选择保存在ignored `hand-goal-parser-cpu1/frozen23b-parser-selection1.json`。这些解码／数学结果尚不等同Native实际规划、连续Owner、writer、physics或头显接受；下一步按`hand-goal-ro1`实际采集。

`hand-goal-ro1`已以冻结Native23bsim完成0.4／behind／observe-only／mute正版实跑，保持Body／Input／Camera／Run／FP／engine_restore OFF，仅数据Rig／Weapon／Core／HandGoal ON。唯一event取得真实10208B `hand-target-plans.bin`，SHA256 `b418f1b7f2b7f97623a0412beddc6eb91160377fc08e5ad48d8375db35767166`；每手prepare1、planned4、reject0／failed0／pending0。Core getter1、cached read4／reject0，outerTick28017／Scene272／GT21456、Native session1588117652527。typed parser对四个实际完整腕目标及规划矩阵重放逐bits一致；原始矩阵、component-to-world、1832B Core副本逐bytes与同轮CCTRL／Core文件关联，四组Pose／control关系通过。Limb腕位置数值误差3.174e-6／1.763e-6原单位；Single仅旋转，原平移仍保留，绝不将它的目标距离误写为已达到位置。完整Native／entry96项恢复、XR0、音频10、原十文件hash、PG／tiny及准确自有PID退出通过，窗口已释放。证据为ignored `hand-goal-ro1/runtime-final.json`、`runtime-hand-plan-parse1.json`、`runtime-hand-core-control-join1.json`。

该轮明确选adaptive只读数学政策，不能计正常臂长跟手通过。武器会话对前一Core2实际数据独立重放发现：原左右总臂长各约46.995单位，肩到腕目标约144.455／126.489单位，preserve均unreachable且失败输出不变。当前FP OFF的测试将完整手姿映射到第三人称pre-HMD相机基准；这是真实空间偏差线索，不是solver故障证明。源码还表明旧FP preview有前移0.35m／上移0.10m，故仅打开FP亦不能预设身体对齐。接下来先以实际world_head／参考头骨／肩与视点基准核对并修正手与身体的坐标关系，再做正常臂长和实际写入验证；不得靠三倍伸臂或任意静态姿态自动校准掩盖偏差。持续只读owner正在独立私有candidate实现，尚未合入当前代码，也没有原生手骨writer或碰撞验收。

`hand-goal-fp-ro1`首次FP对照在ready前被INI配置保护拒绝：私有manifest仍为first_person_preview=false，GUI选择器因此未启用Native能力；没有发送FP toggle或pose event，原失败及缺event回执导致的finalizer解析错误保留，独立96项／XR0／原十文件及准确PID清理通过。`hand-goal-fp-ro2`只补私有manifest的已存在FP能力并离线执行实际gameplay_controls函数确认选择为true，原Native／XR／runtime及固定追踪姿态不改；通过现有PID FirstPersonToggle event一次切入实际preview，再发送唯一pose event。Native最终enabled1／requested1／transitions1／applied246／rejected0，完整Core metadata另证实际view_policy=1。原普通清单／旧preview算法及用户配置保持。

FP2真实数据的4组Core／CCTRL／CPOSE关联通过，但HandGoal只计划左Limb／Single两项：右Limb原始末矩阵第三basis长度0.999979923355763，偏离1约2.0076644e-5，超过原2e-5输入门槛，故invalid_matrix并sticky拒绝最后Single；原矩阵精确affine、有限、CTW严格通过。这次计划接受明确false，完整Native／entry96／XR0／音频10／原十文件／PG／tiny／准确自有PID退出仍通过。武器会话以同三原件独立参数化重放（receipt SHA256 `b60a09e70b2579f87034b801a9574b230aaf837707d4649022431ed46c3fa63e`）确认FP2视点距本局C1骨55约0.26913m，而FP OFF为约1.74273m；实际左右肩到controller派生腕目标仍为61.900／85.687单位，原臂长46.995单位。左preserve确为unreachable；右只可报告有限原始row-product的几何距离及invalid_matrix，不能冒称已接纳刚性姿态。打开旧FP不能直接完成身体空间对齐。

独立Astra矩阵审计给出私有显式opt-in输入政策候选：仅original Limb可用Gram Frobenius总预算1e-4，CTW／模型scale／编码输出与Single尾部保留原规则；默认仍strict。61项针对检查与原417项／24例通过。正例目标是原始Native末端，绝非实际controller目标；此候选不会解决上述实际reach偏差。预算是工程近似，微小真实scale／shear与漂移不可仅靠矩阵区分，现阶段仍缺control LocalScale作用域证据，未合入shared header／adapter或writer。

连续只读owner candidate1在独立548文件私有树编译并通过1111项CPU检查、40 Tick／160 Compute，已经脱离event／C0／诊断容量；不能把它当从23b697实际冻结源派生的runtime产物直接运行。Root独审无只读P1，发现child Compose／Control相同depth错Entry地址仍消耗计数的P2，已要求candidate2以exact地址保留责任并补两条回归；完整caller字节仍留ignored证据，candidate2改为双读完整SHA256认证与小callsite校验，避免将专有完整chunks写进tracked源码。待该小修及独审完成，再从实际23b最小迁移与采集；当前没有连续实机、原生骨骼writer、受击／枪口／碰撞或头显验收。

Root按与武器会话共同确认的单一接口接收矩阵政策候选：`LimbInput`与`LimbData`末尾新增OriginalMatrixPolicy，默认strict_unit；adapter每次显式复制data政策到work.limb，坏enum走实际solve的invalid_policy，CTW、model-scale、Single尾部和编码输出仍用旧规则。Root实际本地严格CL验证adapter8组626、原有math417／24例、近刚性专项61通过；包括bounded→strict重复Workspace、失败output不变、坏enum、CTW保持严格和近似政策会接纳微小真实scale的边界。只改共享math／adapter／其测试3项，旧CF／Native23b／已冻结Native24的2bc3源与二进制保持，不把新政策偷带入连续runtime构建。未开放live opt-in或任何writer，实际controller目标仍不可达。来源及实际include回执保存在ignored `shared-original-matrix-policy1/attempt-1792902299784375500/receipt.json`。

连续只读owner的candidate2已修正Compose／Control相同depth错误Entry地址消耗责任的问题，两条exact-address回归通过。调用认证的完整798／26B历史代码仍在ignored fixture；tracked certificate只保留RVA、长度、SHA256和16B callsite，不包含完整游戏代码。Native24首次组合构建的cap-OFF helper因引用cap-ON声明失败，原24保留；24b在新接线周围添加真实编译guard，关闭能力仍走原路径，没有no-op替身。24b从实际23b最小派生702文件（6既有／5新增），80个defines保留旧渲染政策，284项focused CTest、15项实际DLL helper及8项component预检通过。Normal DLL SHA256 `b6d4fff5b7ec56ed64882097bf808ea0ca89a949dd911b8ed1f72647ac22f5b6`，sim DLL `fdc9da253163bfb3ddbd1704f31b527ed37fd116a683db2463667c8dd39a0584`；ready选择为ignored `native-body24b-ready-final1.json`，SHA256 `84dc7495a9d2b2f6c1b1a2943515ffecfd728a4b55ac1aa265034a1a7f43ac0b`。42导出、原renderer／MinHook来源保持。

`continuous-hand-ro1`以24bsim完成0.4／behind／tiny只观察／测试静音正版实跑。Rig／Weapon／Continuous数据能力开启，旧Pose／CCTRL／Core／HandGoal事件观察关闭，Body／Input／Camera／Run／engine_restore保持OFF；通过既有FP toggle切入preview，没有发送Pose事件。411个真实outerTick各调用一次Core getter，Core读取拒绝0；394 Tick完成四份计划，左右prepare分别411／405，总拒绝计数45不是失败Tick数。最终pending0／failed0，完整Native／entry96项独立恢复、XR0、音频10、原十文件hash、PG／tiny与准确自有PID退出全部通过。二进制`continuous-hand-plans.bin`为132944B，SHA256 `891b36404852cce6e039decf97b1c36ac4eaf75f205c9401651d56249670da62`；容量32的最后32条均complete且failure0，独立ABI parser无issue。总计算不受32条环形存储容量限制。

该轮明确使用adaptive只读诊断及24b的旧strict矩阵算法；`runtime-final.json`的accepted只表示连续采集和生命周期通过，不表示正常臂长、FP对齐、骨骼写入或physics／render fence通过。当前tracked已选择接入24b的6既有／5新文件，capture／CMake仅应用对应hunk，逆patch完全回到当前前态，并保留已提交的新shared政策。Root以当前实际headers严格CL编译owner／actor两个helper，13个owner模式加caller SHA认证／actor检查全部通过，回执为ignored `continuous-hand-tracked-integration1/cpu-1792904583612796200/receipt.json`；第一次缺vendor include路径的失败保留。此CPU结果不是当前shared政策与continuous组合后的完整DLL编译，已运行的冻结24b与当前tracked来源继续分开记录。

武器会话对FP2同局眼部参考骨57／60的进一步核对给出：world_head距双眼骨pivot中点约18.1519cm；之前26.9132cm是距head55，不应等同真实眼点误差。该参考仍不是实体眼睛或IPD标定。肩线相对相机右方向约偏42.8655度，单独移动视点不足以解决完整双手reach。现有骨4的原生Recoil控制可位于双肩／头的祖先链，但必须保留它自己的自然状态gate、状态更新和原FK路线；正在补齐只读实际调用与post-weight scale数据，不强制激活原控制，不以endpoint数据推定未来调用许可。LGrip抓边／保持与缩回、躯干最终朝墙、低位中立视点、受击／枪口／碰撞及跑步最终视角仍未完成，不能将持续数据接通描述为掩体玩法可用。

Native25在实际24b上只改CCTRL／CMake并加4个文件，706文件整树逆patch回到702基线；原80defines、renderer／MinHook来源、strict2bc和42导出保持。9个新增CPU场景463 checks、BodyOff／ControlOff各11 checks及ABI检查通过，累计296项focused CTest，normal与sim各实际15 DLL+8 component预检通过；sim的296项CPU按真实Ninja／源码／flags闭包复用，不冒称重跑。Normal DLL SHA256 `b2ae70ba1d99c4c17ca1ec7703b1f54f46f91d2abb964239caf7e38254cf4400`，sim `4139c64a46f8bbba999ffd144bbd1999d9408c66994835ca8bbd323eb0dd473f`；选择为ignored `native-body25-ready-final1.json`，SHA256 `19663a3f7b0a1658e96448c0ae260056f6a64a93f4ca7b0655fd9688a629aa75`。该模块默认OFF，仅复用旧v278返回观察，没有新增hook／getter／engine调用或pose写入；cap-OFF显式enabled请求会拒绝，不静默忽略。

`post-weight-scale-ro1`在发送事件前被Root测试guard误拒绝：新模块只有capture-status的启动／最终print声明，periodic session-status未含该字段，实际INI及启动声明均为ON。未发Pose事件、空88B sidecar；原失败及finalizer缺event错误保留，96项清理独立通过。`ro2`仅修正私有guard从当前capture-status取真实启动声明，核对当前launcher PID／创建时间和Native PID；对旧真实失败轮读回通过，缺声明／错owner／错PID三反例均拒绝，未改DLL或削弱原身份／输入／窗口保护。两轮均0.4／behind／observe-only／mute，Continuous与HandGoal OFF，Pose／CCTRL／Core数据ON。

`post-weight-scale-ro2`实际session1650050334510／PID54836／Compose44873取得四组`CPSCL001`，24984B，SHA256 `f7cf3b3ee85517bd90a2e286883d81af30d0dd340fd008437a29189d79f61260`。Root使用25实际ABI和独立parser再次解析，4/4只读观察qualified，骨序分别8→10→11、11、32→34→35、35；前后read_mask均127，cached flag为实际DWORD0，scale header的data／count／capacity全0，真实alpha bits均3f800000，完整返回packet保持，Compose正常关闭且无nested／concurrent。嵌入8个Compute／weight Row与原CCTRL逐字关联，唯一变化为实际close更新offset1938的compose_returned 0→1，其余包括padding逐字相同，不做伪零归一化。完整96项恢复、XR0、音频10、原十文件与准确PID退出通过，窗口释放。

独立parser SHA256 `f789d7d76e2eeea06ac7eafb998172a957f7c5a43f820f78487e40df1df0d8f8`，13项格式／cohort／claims／截断／未读值等正负检查通过；Root实际结果为ignored `post-weight-scale-ro2/runtime-scale-parse1.json`。它只排除本轮四组v240额外缩放，未来v248/g、其他状态、正常reach、骨骼writer／物理传播未获证明。Scale25专属2旧／4新文件已最小接入tracked，CMake保留当前其他内容、逆patch精确，当前headers的ON九场景／ABI和两个OFF helper严格CL通过，回执为`post-weight-scale-tracked-integration1/cpu-1792905767363545400/receipt.json`；当前combined完整DLL编译仍与已运行冻结25分开记录。
### 2026-10-25 — 掩体分支补充记录，随C0内核接收

以下Native26c／27实跑和原始证据属于掩体分支，接收其历史记录不代表武器分支已构建或运行这些Native候选；证据目录位于掩体独立工作树。

独立接收武器会话的null-link身份复读修复`ab2ec3818b7e1ec0d7e3094a9e1bcfe13e88b8a2`，本分支为`fe4f90e`：只将UI组装期间判定可重试null前的controller／field身份检查恢复到真正fresh_read，避免assembly hint cache掩盖变化，既有typed-null／启动分类保持。Root本地两个修改测试模块实际7项通过；初次错误目录的0项不计，收尾误期待对方31项而失败的元数据断言保留，随后仅依据原exit0／7项日志与源码hash澄清，未重跑或冒称31项。实际范围和来源见ignored `analysis/ui-null-fresh-receiver1/receiver-final1.json`；未改根默认清单或扩展为新启动兼容性测试。

固定个人臂长接口按武器会话独立提交`35a44974575a22fd4ca8cc25ad07f4f60f58ed92`接收，本分支为`6affa4b`。`LimbInput`／`LimbData`的upper_length_scale／lower_length_scale默认均1，adapter每次显式复制到底层既有LengthPolicy，不从target推导、不改变输入矩阵／头手／IPD／world units。preserve只保留原始输入段长×固定比例；adaptive仍可伸长／重分配，不能把calibrated状态标签当作已完成玩家测量。私有C++ Workspace／输入布局变大，消费者必须同步重编，public Frame／Read／Core wire未变；玩家测量／持久化和live opt-in未接。Root首次raw hash预检遇Git checkout CRLF差异而在编译前拒绝，随后对peer实际原件hash、双方LF canonical和已提交Git blob逐项认证，仅换行不同，实际本地raw hash独立保留。严格CL adapter9组820、math24例417通过，当前连续owner／actor全部模式另以新headers重编通过，回执分别为`fixed-length-receiver1/attempt-1792907100878442400/receipt.json`和`continuous-hand-tracked-integration1/cpu-1792907047716080400/receipt.json`。本地helper不冒称Root完整TU；武器会话727源组合TU／198相关CTest及14个continuous模式为另一份明确来源的证据。

Native26初始Recoil候选只读窄审修正opaque scale的stride4假设：只保存稳定双读的真实三header／DWORD，新增headers_read512，非空scale明确拒绝output_read64，不读取payload。Root再发现nested且selected=false的复制WeightCall仍可能消耗depth责任，26在已编译后保留为superseded，26b对所有counted call加exact top／previous地址链并补反例；错误end保债，真实nested／outer顺序可排空。26b在实际25上4既有／4新增，710文件的inverse精确回到706；81defines只增BodyRecoil能力，42导出、renderer／MinHook／strict2bc与25 Scale接线保持。normal DLL SHA256 `90fa333909401783a91fae512bff878df71923c8261e48d4467254199dfc1bae`，sim `7e7323639cf56a3ef6a27fa040455ce1a977dbce285b3cc7b9bfdefd8eba723b`；ready为`native-body26b-ready-final1.json`，SHA256 `29cb851a8b50b358fa47fa46a24b4e1b5176e5848a1972c5909019edb4dbad10`。normal307项focused、Recoil205 checks（含opaque25／nested-end24／OFF7），两版各15 DLL+8 component通过，sim307按实际闭包复用。旧26、构建前参数失败和basename旧回执均保留。

`body-recoil-ro1`已以26bsim完成0.4／behind／observe-only／mute正版连续观察。旧event／post-weight诊断OFF、Continuous／BodyRecoil只读ON，通过一次既有FP toggle，无Pose event，不强bit／strength或调用Compute。实际408 Compose观察、自然调用计数0、拒绝16；32槽末尾记录均failure0／returned1／no_natural_call1／self-qualified1、mask399（context／model／map／begin／end／code），没有headers或output读位。二进制165464B，SHA256 `83e7fce686f08a7546d129a527ba5921af1af402fe810dcb0c24941da7358a51`。Root用外部receipt的session1663201348914／PID50808／base140000000运行独立parser，结构／claims／cohort均通过，32条独立一致的输出为0，不能把该静止场景、环外记录或16个拒绝窗口推定为整段inactive。完整96项独立恢复、XR0、音频10、原十文件和准确自有PID清理通过；这不是身体姿态或writer验收。

Recoil parser SHA256 `252abcfd24784da8f7c1899eeae63c4bdb7afd8bedf5ba2afbfe43f419f520c1`，与26b机械完整ABI一致：Header88／Record5168，19项格式／readmask／cohort／opaque／异常／环／CREATE_NEW检查通过。Root实际结果为`body-recoil-ro1/runtime-body-parse1.json`。已把26b的4既有／4新增选择接入tracked，capture／CMake仅应用对应hunk并精确inverse，当前新shared固定长度headers下的Recoil ON九场景／ABI／OFF严格CL通过，回执`body-recoil-tracked-integration1/cpu-1792907275905002200/receipt.json`。编译和运行默认仍OFF，不授writer、独占、v248或physics fence。

限定静态审计进一步确认：原Compose在1293F5／1293FA已消费scratch到SpaceBases，129419已写转换后的Atom，之后才12946E→129474取v248/g；等真实g返回再只改旧scratch已晚。下一最小观察为旁观原调用并关联现有post-weight四组，不额外调用getter，不将端点+AC或多次g=1当区间恒定。审计为ignored `analysis/post-weight-future-g-audit1.md`；躯干自然动作覆盖和g旁观候选继续私有实现，当前未开放LGrip抓边或最终骨骼写入。

Native26c增加默认0的`freeze_on_first_natural_output`，只有合格自然输出已保存才冻结环写入，原调用／Core／累计计数继续，Header88／Record5168保持。首次312项检查中一项新测试因WinProfile相对INI路径失败，原source26c／build保留；source26c2只修测试绝对路径，生产字节相同，normal312项与Recoil1596 checks通过、sim依实际闭包复用，两版各15 DLL+8 component通过。ready SHA256 `1f55b584598b2e0b62e3a7547eee3e0824104d6963194e0684221dd2d0730340`；normal DLL `d5a25d6559d8f280305ff778aca4c03a13af3d2a21f2ae84a265ad95e1eba99c`、sim `18704cef03f4664414b28cc6c131136e657a278b3206f5d9fdf76dcc01b700fc`。并未改正常默认。

`body-recoil-fire-ro1`以26csim完成一次原生owned HWND `Game_Shoot`输入：请求80ms，无aim／retry，现成pulse在finally释放，同身份UP后请求正常stop。输入释放验证通过，未取得post-shot ammo，不能声称ammo／damage验收。实际Header sequence491／composes509／natural_calls12／rejections21，32条保留记录中有1份独立一致的自然输出，mask1023／alpha1／hasScale0；冻结阻止后18次更新覆盖。165464B SHA256 `a0a0c7b933764c426a51fe0e5470e2a9594d9faa255b77407549fbb5f554b18a`。实际输出QPC区间在派发DOWN之后约11.952ms开始、在UP之前约72.502ms结束；这只是时间关联，不替代弹药因果或伤害证明。完整96项独立恢复、XR0、音频10、原十文件与准确PID清理通过。

freeze parser2原本假定最终状态只有一行，实际固定producer的`write_session_status`另写`complete=1`末行，maker因此正确保留拒绝回执。Root新增独立parser3，仅对固定native26c产物接受“唯一状态行＋精确complete=1末行”，仍拒多个snapshot、缺失／错误／多余末行；旧parser2／全部原文件保持。实际freeze receipt与typed解析通过，另6个真实格式损坏副本均拒绝，结果为`body-recoil-fire-ro1/runtime-body-freeze-parse2.json`和`analysis/body-recoil-parser3/validation-final1.json`。producer／session1704512707745／PID17792／base140000000／创建时间／清理状态仍逐项核对，未放宽身份或计数合同。

Native27从26c2的710文件最小派生718文件（5既有／8新增），82defines只增FutureG能力，339项focused实跑、sim按实际依赖闭包复用、两版各15 DLL+8 component通过；42导出、原渲染／颜色／左眼history与旧ABI保持。新9B getter需要的精确VMOVSS搬运仅cap ON生成，vendor与cap OFF源保持，ON新源hash明确为`24cc672eb7d20d9c38763a282e7c2abd90d3eb177a2834b0d8d79a8ca1d47984`。真实call独立exact-self计数及饱和pending已修复复制end导致计数回绕的P2，失败版本保留。ready SHA256 `4b131ba4efdadc82eba4f4adcef960e8b6748df5506b8d6d56387685dcc1c8b7`，normal DLL `8934b07d97f0b7f60abb4f305eef71dcce9ac7bdb10d6bf695997ada4bc51123`、sim `a62d6f9f73c5f99c59b003af5736ed009772c935458deba016a45aab97d192ea`。

`future-g-ro1`实际一次FP toggle与一次Pose事件取得四组原生g返回，全部1.0，Scope=session1732915906423／PID7868／Compose45116，Header88／Record7728共31000B，SHA256 `51511acbd5169dc1a3fd1dc490f1f6e8f6dcc3053c432ae08630877e12661b53`。独立parser按真实ABI与外部event cohort解析，4/4 qualified，旧Post／CCTRL逐字关联通过，完整98项独立恢复（原96＋新getter完整9B／固定32B）、XR0、音频10、原十文件和准确PID退出通过。解析器未预读新g参数块，旧Post原件的参数不变性没有被推为当前或未来写资格。实际结果为`future-g-ro1/runtime-future-g-parse1.json`；writer／g区间／producer→C0／physics fence仍为false。

连续body正式收敛到原动画输出进入基础FK前的C0事务，Recoil不再作为持续carrier。Root按KB先查，在私有IDA副本机械导出F7940、109970及F6D8C／577E5F／10AEB0，原IDB SHA404ae保持。已有RO4 F7940仅盘点，不是本帧producer执行认证；F9341后还可能root-motion及109970处理，最终提交边界应为F9487前。IAT thunk的实际raw6解出131CA18，与旧IDA表达131BA18不同；磁盘PE无匹配不冒称memcpy身份。唯一producer→C0定点样本仍在准备，不扩整棵动画树，证据在`ida-body1/fresh-local-output1/`及`fresh-local-helpers1/`。

新增真正完整的纯C0 body planner，当前Root源码为`full_body_c0_planner.h`／测试／CMake。以完整115 canonical签名和真实C0 Atom32＋CTW构模，输出115个Local Atom、changed mask、更新头／肩／骨盆／脚与残差；用原有保段长双骨solver和有界脊柱FABRIK，所有非Root LocalT与scale逐字保留。独立物理位移和已提交Grip偏移各合成一次，Root heading与头旋转分开，两种posture的纯转头不移动Root／骨盆；包含Ref中立上身。失败保持整个Output旧字节，不缩不可达目标。首支持固定脚、scale1、身体yaw±60度；不等完整cover180、脚步切换或运动验收，head-bone world目标的HMD／眼作者映射仍由上游提供。

该planner独立审查无当前纯数组范围P1／P2，Root接收只改三include，原件／fixture保持。当前实际tools共享headers严格CL重编，9456 checks／0fail、20份完整115×32B候选，最大头残差0.000254662u、脚0.000570187u、段长0.000314261u；主要计数是逐骨验证，不能称9456个独立玩法场景。Root tracked编译与实际include回执为`full-body-c0-tracked-receiver1/cpu-1792914362233462300/receipt.json`。CMake缺私有fixture时不注册该fixture用例，不把skip当通过。尚未把planner结果写入Game，没有宣称C0所有权、reader或physics fence、LGrip实际抓边与最终玩法完成。
