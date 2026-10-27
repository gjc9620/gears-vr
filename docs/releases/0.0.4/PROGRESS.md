# GearsVR 0.0.4 开发进度

本文件保留武器与掩体两个分支的开发历史，便于共用 Native 手部实现合流。各条记录仍以其原始验证范围为准；私有候选、只读采样或离线检查不代表完整功能、默认构建更新或头显验收。

## 2026-10-27 — ready同entry目标／提交／原返回观察合同对齐，尚无新候选运行

统筹将下一唯一交付限定为稳定跟随：Body独占准确HJ Native中的最小被动ready观察，武器会话只交字段／坐标合同与最小消费。双方已确认默认OFF Source1 fixture，full inverse(head)*grip的ready识别仅为诊断（1mm／1e-3rad），原writer／source资格不变。明确ready启动后有限64条不轮转，捕获失败与未知资格也保留entry／UNKNOWN，直到声明的非ready／身份变化／容量停止；未知不能当非ready漏掉，旧first32不改，不再赌异步近邻epoch。

最小骨35／51／52复用旧hand9槽5／6／7：本entry compose成功的world wrist目标、实际Kernel结果及planned／consumer-pre Local、原called＋returned后的Local／Space35／52＋CTW，normal scope_close独立；未计算／未到达／未认证组有valid位，不用旧workspace或默认0冒充。consumer_pre准确位于before_original，不是原调用返回；capture成功但prepare失败路径可由Body单独认证后做被动读，不能绕原保护借旧model。枪体／Muzzle同步字段本轮UNKNOWN，不额外读资产或用旧PG几何填同entry。所有数学按原UE行向量world=Space*CTW、game原单位，raw XR米空间单列；不以相同raw右手替代整个head／left／view／basis来源相同。

Root逻辑合同`analysis/lancer-ready-same-entry-20261027/consumer-contract-v1.json` SHA `b860b295ee1a35e86d89adf931260074c533a8281bf77d5b3c34e4366eaa8e32`及`docs/weapon-ready-same-entry-observation-contract-20261027.md`已交Body／统筹，不预先猜binary offsets。Body已确认开始实现并将交冻结布局／有效性／Host证据，Root随后最小file-only消费；当前只有合同／文档，无新Native／PG／collector／writer／Game、编译或测试，不扩大枪械／射击许可或默认整合。

## 2026-10-27 — HJ三phase与恢复通过，8个phase内Muzzle配对；同raw腕／枪姿态仍变化

Body后续仅对现存hand／BMS首32做有限join（`pg-first32-finite-join.json` SHA `22a3548822811b9cc2d53797a666da46f484a92460ae28d9e9608fde6ece138f`），9个PG实际mesh＋epoch／end精确对应均0。prefix1272..1337中没有1280／1290实际记录，邻近epoch不替代；ready／move超出prefix，PG读区间也未被已存query区间包含。因此现有资料不能定位差异到Native target／consumer／end；Root未重解或重算矩阵，新接收`hj-finite-join-received.json`保留原件hash与界限。相位角度归属按原Root逐项数值明确为ready最大32.54642°、move8→9约23.60479°，旧PG JSON不改。下一最小需求是覆盖这些实际差异点的有限精确观察，交统筹／唯一共享Native owner安排，当前无新Game／编译／测试／候选或writer。

Body唯一HJ组合710756／reader01d8／PG5fdc已完成并释放：Game60808／Native3165621632666。原review complete=true、8动作三phase通过（status0→41／4.485336s、ready41→50／1.075517s、move50→59／0.8634748s），Stop后bodycontinue／bothrestore／bridgeclose均true，原finalizer一次106／full／Native-entry-PG／XR0／十文件／音频与fresh空通过，32handtrace有效。Root核交接摘要SHA `693a9be13ee40b06112d8aee93d29290ada21cde8c6bc944fcd1b0c4f18613a6`并复用协议结果，不重复Body v3／Native解码。reader_failures=[]只表本局没有记录66失败stderr，不把旧66判修复或给组合被动诊断宣称单一因果。

Root独占40791B原PG日志（SHA `89e837fbf5c5ddc90d9845c81f52365c97f5d1c96937e7ca08bd5d61b15bac3f`）：9／9组件＋Avatar＋Muzzle配对有效，其中2held／3ready／3move完整落在原phase，1行prephase。初始实际间隔536.1317／538.1829ms，后继仍50ms配置；actual Ref9 bone0／socket相对几何全相同。binary32数学重放最大误差1.78e-15，gun相对Avatar bone52固定关系残差0.0002301，Avatar CTW逐字不变。私有`analysis/lancer-component-readback-20261026/hj-saved-muzzle-review.json` SHA `5bd618475033a183b453f854f781c5916431b5eb2bd4e4e668665e99f91882ba`保存完整raw／source scope／实际phase窗口与矩阵。

原协议通过仍不能判稳定跟手：ready同raw右手的样本4／5／6（Avatar epoch1359／1360／1362）腕／枪／枪口一起变化，4与6近而5偏离，最大Muzzle位移39.54088u；move的7／8近而9再切，8→9枪旋转差23.60479°；32.54642°属于ready阶段。每条内部前后读取稳定，当前不支持枪组件单独脱骨，但顺序观察不是同C0或最终GPU，且raw右手相同不替代全head／left／view来源证明。已给Body exact epochs，要求只利用已有source-target／consumer／end能覆盖的点做有限对应，超prefix明确缺证，不开新局／扩矩阵／新writer或归因原动画。R2关闭，getter／权威bore／散布／回缩／前推／实际命中／物理头显未验，默认不替换；功能计分仍由统筹依固定节点核定，不能按9个sample推整项完成。

## 2026-10-27 — BHELD003最小消费扩展交齐，等待唯一HJ联合实跑

按统筹明确分工，Root新增ignored `analysis/lancer-component-readback-20261026/decode_held_captures_v3.py`，SHA `54ef375080ece0cae984a72fdb16710b60241488d558fb0e304bab135c679a90`。只沿未改v2／v1做内存共同字段投影，显式BHELD003／version3／Header120／Record600，capture reason392／Context400，旧guards／admission移至408；原文件／时标／身份／late-arm／coverage保持，不保存冒充原件的投影文件。合同683f5b95／header256cf9b2／Backend48e173b8均按准确hash校验，旧v2 reader与全部旧交付source不变并严格拒新magic，新v3拒旧v2，不覆盖旧证据。

UNKNOWN是本entry未观测，不转零成功；0仅证明capture返回true。原因1..24保留首个实际原操作数，后继未执行不当通过；只有11／24可携Context，其他非UNKNOWN组合及未知枚举拒绝，11／24缺verdict仍missing不verified。四个helper标签只代表边界，不猜内部条件；新首拒只细化已记录的Native backend_capture，不编造分支时间或抢先替换原guard／Kernel结果。capture拒绝与held commit并存判矛盾无效，首个无input与统计只用trusted记录。late arm／容量／导出跨度不能扩为完整phase、总提交数、consumer／GPU／aim通过。

8个v3专用合成方法0.031s通过；追加上述矛盾边界后只复验相关1方法0.006s通过。未重跑v1／v2套件或实际128条，未启动Game／重建Native／XR；复用Body原journal36／Backend245布局证据。交付`held-capture-v3-reader-delivery.json` SHA `a8f64c5854fa63e273b5beae86ebac2b40f797e5aa177c7029483e8644186bc9`与`HELD-CAPTURE-V3-READER.md`，2520B四记录例明确SYNTHETIC非实机。已交Body／统筹复用CLI／hash，Body独占下一实际v3解码／Native分析，Root只消费实际PG Muzzle与Stereo-open stderr，不双路重复。

组合候选保持准确Native710756／reader01d8／PG5fdc，b768不选。Body已报告新DLL原startup22／preflight23实际45／0及HJ38／12／4案例预检完成；这些是新候选的检查，未冒充父通过。Root没有新Game、玩法加分、默认替换或另开接收平台；核交付后由Body按既有授权唯一HJ原动作／原统计／恢复门运行。

## 2026-10-27 — 私有reader的Stereo-open准确子阶段诊断完成，未启动联合游戏

按统筹独立分工，Root从实际0c707 reader冻结9源／原flags派生候选，仅私有`stereo_bridge.h`／`reader.cpp`改变；共享Native／XR／headers不动，原7源／测试及旧exe逐hash保持。原OpenMutex→OpenFileMapping（即使前者失败仍执行后者）、MapView、0等待、原复制／释放／QPF／packet短路次序保留，WAIT_ABANDONED仍释放拒绝，正常复制后ReleaseMutex失败也不新增拒绝条件。仅在原初读期间记录实际调用即时失败Win32、已有wait返回和已复制packet首拒；无额外open／sample／read／QPC／retry／权限。旧stderr行及stdout／exit66／400ms匹配和age尾部不变，只追加一行`reader_stereo_open_v1`。未执行／未复制和成功调用的旧LastError不能当有效失败，后续普通read不覆盖open记录。

完整严格reader编译、19个baseline／candidate API替身场景、原／新各3个只针对本Python PID的CLI64／64／66小检查通过；实际缺自有Stereo对象时错误2即时记录、stdout为空。无Game／XR、bridge producer、目标RAM或旧右手命令重放；不重跑未改controller整套。一轮窄Astra只读两文件审阅无P1／P2。实际`/sourceDependencies`包含所选7个私有headers，原9源前后hash保持。

选定exe ignored `analysis/stereo-reader-open-diagnostic-20261027/validation-1793056273353595900/reader.exe` SHA `01d8b9db36bfa57b580dc98d47e0969eec1b31be32d44e8b03759fcfc11ffc83`；`candidate-manifest.json` SHA `0dac77b18fde597cadac1660f246be2757c89093683e61b1ab30a0e2951c649b`、`result.json` SHA `b66b388aa1e9234e957d2d733d3de405af30d10c01ad6d6f242bb3f8769ea0fb`、差分SHA `098147ac281eac3b975355d7d0b010c0955b2964eb1ad644c45445def069e846`已交统筹和Body复用。说明为`docs/weapon-reader-stereo-open-diagnostic-20261027.md`。这是诊断交付，不能认定HI根因／fresh gap或玩法完成；新Native BHELD003／capture首拒候选由Body独占，现v2 reader继续严格拒新magic，待统筹安排最小适配后与5fdc cadence按原门一次串行组合，无默认替换／加分。

## 2026-10-27 — HI取得两组真实Muzzle几何，均早于held阶段；后继采样间隔候选只准备

Body后续窄分析已封存`analysis/body-held-outcome-runtime-20261027i/held-restore-saved-summary.json` SHA `c34acbb80d7ecf21ff8b532a7752f858b7be0a45ba57e201574c1b6100c218c6`（Body文档a250e7d）。Root仅核交付hash并引用，不重解持枪／restore：journal因late arm仅有1条recorded commit，不能当全phase总数；首已记录阻断backend_capture的内部首拒子条件未存。右恢复66已确认stereo.open，但缺内部子原因／调用clock，不能证明同案fresh gap。采样b4c1b0a／5fdc2f候选继续只暂存，待共享定位后串行组合；中间b768不选、不立即开局，原所有失败与Muzzle证据保持。

Body唯一HI／PG207实跑Game29688／Native3144730156562已结束，Root只消费49675B／SHA `7fb11641f10e35cad2653b27a952851ff0d9726fff6fbf79eedd78facf909770`原PG小日志。3行全部socket before／after completed20／pass2／Reason0，2行完整component＋Avatar＋Muzzle配对接受，另1行被原game_or_generation拒绝而未发布derived world。实际唯一socket与当前Ref9的bone0匹配、相对位置 `(72.6682053,-0.00669687986,11.25)`／rotation0；9位打印恢复原binary32后独立矩阵重放与两个原derived世界位置double误差0。这证明精确Core.Object schema在这些当前点已有效，不反推HH唯一早退。Root回执`hi-saved-muzzle-review.json` SHA `c468aa12d1d77cb2b15f2ff42857401d22894fd9aef3ef7255247152cd5dc327`保留全部raw／时标／名字与身份scope／原件hash。

两有效区间结束在原held phase前261.8747／86.4689ms，固定raw输入下Muzzle位置相差0.53072u；没有phase内或ready／move观察，不能提升稳定可见跟随／同C0／原getter／bore／权威射击或GPU证明。原40／6秒＋pose门失败，status0→1只是观测；Stop后body继续，但右控制器恢复未验证、both=false，首32验收header_valid=false／trace仅1。106／full／音频9／9和fresh空的accepted只属cleanup。Body交接指出恢复66为reader_open/stereo发生在Source1／age匹配循环前，不能因文案含fresh而断言过期；共享协议／restore分析由Body独占，Root不重解held或另读reader。

Root仅在MUZZLE ON路径调整最初3个sample的原间隔50→500ms，后继attempt4..24仍50ms；3／pose、24／identity、8identity上限及所有source／metadata／权限／原动作／门保持。它只把初始有限观察分散，不是phase触发器，仍须实时时标关联。完整PG编译得到 `5fdc2f59037db4fc06055d5ca9344c79eb89873403f9c9b706a734d090b223cd`，相对207仅一个probe文件，原flags／16导出／OFF父源保持，原Pose12／Socket42因源／相关定义不变复用不重跑。回执`analysis/lancer-muzzle-observation-20261027/attempt-1793055422892249000/receipt.json` SHA `a4b53aa3b7337f65dac50382a7960190c039ec46eb13fdbd961895357b946213`，审计SHA `6f3d2e1e15941d64d93faa5f8d975adb684c0293a7d6b9ab591b441126927ac3`；中间b768全阶段500ms候选未选／未实跑。新候选只准备，等待共享门定位和协调后的串行组合，不立即另开Game，不加玩法分或替换默认。

## 2026-10-27 — HH无有效Muzzle数据；修正已证实的Core.Object字段owner并加入实际早退记录

计数口径更正：Body随后独占核同局v2，合法111条／drop0，phase内实际有42个held commit markers（counter2..43）；26条带pose的joint／status观测最高35，因此不能将35说成Native只提交35，也不能仅以42条marker把原双计数>=40联合门改判通过。Root复用Body交接结论，不重解该111条；held阶段无Goal10／11与postStop后独立reason10的定位仍由Body处理，不扩为Muzzle旁支调查。Body随后已封存`analysis/body-held-outcome-runtime-20261027h/two-blockers-saved-summary.json` SHA `585b1703ca6a21b520b3202c844a633dc97733872409422213c7b7d59a96e3e8`；Root仅核这份交付hash并复用标量，不重复统计。第40个distinct in-phase held marker正常end在marker后5.7116094s，live producer/status的freshness仍缺证，原联合门保持失败；postStop独立reason10在phase末后142.1621ms，只三项observed epoch1→2不等，16实际空间／身份项一致，其后2.406s attempts／reject各+87而plans／body提交不动。这些保存证据不把两项阻断判为同一根因，也不归因Muzzle。原HH小日志／阴性回执与全部历史pins不改。

Body唯一HH局Game50892／Native3134767077917已结束：启动门／Grip／episodes1通过，但原shoulder-fixed40／6秒联合门status观测0→35、6.0179205s，后续ready／move未到，Stop quiescent成立而body续更超时。原finalizer一次accepted只证明106／full／Native-entry-PG／XR0／十文件／audio9／9及fresh空恢复；不把cleanup当动作通过。Root复用Body `runtime-handoff-summary.json` SHA `b64799c8db911de7d9728aa256f26e3e516801578c25aa4eb1a23d7c57568058`，只消费其21648B／SHA `64a1dc290faf28c7a4e917f82e3267d0f0caeb54fef3becf965e8f886f3bcf28`PG日志：肩部3／3component与Avatar配对接受，Muzzle前后／pair均0，无derived_world；所有socket0／count0等是未录默认值。两行早于phase179.369／70.1742ms，第三行只始于phase后0.2454ms，不补整段。私有`hh-muzzle-negative-saved.json` SHA `df7d7cacc300a7871781330796e62bbb92c8a1d3a1efeab8ccec56a176381636`保留原件和边界。

单个既有助手与Root从保存metadata确认新增reader的具体源码错误：`Vector/Rotator`属于`Core.Object`而非直接`Core`。旧31例的合成正例也错误构造直接Core，不能证明真实owner兼容。修正只接受准确ScriptStruct→真实Core.Class Object→根Core链，extent12和原所有身份／字段／动态值门保留，假owner和旧直接Core新增拒绝。历史`live.json` SHA `65bcb1c55b619cf5f8014a411b8269a4d5d1e5c96c929b67a9ee59307ae57e66`与Core `objects.tsv` SHA `636376e841f7c82b639e1480885edc650554a4df0d995f99d3faef2c9efb139a`为依据，不是当前Game getter／调用证书。HH未记实际失败step，故不将此缺陷认定为HH唯一首拒或原联合门／Stop失败原因。

同时只给现有返回加入step／pass／actual metadata Reason，无额外诊断Game读取／clock／guard；未进入Reader时Reason未知，实际none=0，失败Socket仍不发布。新PG `207cb5373f5ac675a24efb64a6f08237f17c72b2662fb6d046b80b0855f8f231`相对80b826只变4个观察文件；完整严格PG构建、受影响Socket42例通过，原位姿数学／测试源逐hash相同并复用旧12例。16导出／原flags保持；OFF probe精确父源。回执`analysis/lancer-muzzle-observation-20261027/attempt-1793054247587862700/receipt.json` SHA `be549385365f5f786b94f7fb11832ff1b9d5025aa7e5fb7a7851ebfa6ee4f012`、工件审计SHA `90be72f306dbe4c7715960d9aeeb5a8f78917c6b489ed9827e5152412d481d17`。这是有具体变化的候选，尚无实跑／Muzzle或玩法完成，不重建Native／XR、不重解held／旧suite、不放宽门或替换默认。

## 2026-10-27 — 实际Muzzle观察PG已交Body复用HG动作，CPU通过但尚无实跑结果

Root／单个既有助手分别实现9骨纯数组位置观察与实际typed Socket读取，集成为`83140a9`。默认OFF的Muzzle probe依赖既有bounded component probe，只读当前MuzzleSocketName／Sockets唯一对象／BoneName／RelativeLocation／Rotation及枪9骨Ref／Local／Space／epoch前后；原Frame、身份、时效、附件、原component／Avatar配对和采样上限均保留。`muzzle_pair_accepted`与旧观察分别记录，getter等价／权威bore／sameC0／最终renderer均未证明，没有新骨骼writer、引擎getter或mount_known／R2放行。非Actor对象只认证当前注册身份，不认证未知pending-kill flags。

直接以HG failclosed PG dfd995的实际source2及17 vendor源为父，只变2既有／加5源，811份source闭包不变；原FLAGS保持、DEFINES仅加Muzzle观察。严格完整PG构建、12个枪骨／位置与31个typed Socket owned案例通过，16导出保持。PG SHA `80b826235774f6b9126db0017fdc24f1867cc2127e99b68bb6ddfb436a3db150`；ignored `analysis/lancer-muzzle-observation-20261027/attempt-1793053485363798500/receipt.json` SHA `63bde1d5b8f4de45c1dcd344af6461977b65a5102dfcda6e23a5d7464c662809`。只读工件审计`artifact-audit.json` SHA `bbf633081f3076c9aad9c7c7deb0e30dffeba1e62de04001ce9f8391a14d2eb3`证明OFF去块后probe与逆除新增CMake文本精确等于父源。首次漏独列vendor的configure失败和后一次vendor路径键规范化拒绝保留；均未到编译／Game，只修driver后才取得上述成功产物。

Root已将Body后台09e0554最小3文件修正cherry-pick为`f567793`，不回旧build4／不改历史HF／HG pins。候选已交Body唯一runtime owner：后继只换PG，复用HG准确Native22268／XR1a059／quiet19b355／EarlyLaunch c12172及原8动作、40-6s／Stop／106／音频存档保护，0.4置后静音，R2继续OFF；Body确认正在准备。Root不另开Game、重复旧suite／held解码或重建Native／XR。当前只到构建与新CPU检查，实际枪口位置／持续可见跟随／原权威射击和默认整合均待证，⑤按统筹40%保持。

## 2026-10-27 — HG原持枪与交还协议通过；复用9组腕／枪配对，实际枪口仍缺证

Body交HG已封存实跑：Game64176／Native3118170081683，Native `22268f8a…313b6`／PG failclosed `dfd995e4…904669`／XR `1a059d2d…a2b4e9`／quiet `19b355db…15ef3`。同局原8动作、首40／6秒门0→46（约5.232秒）、两后继阶段+8／+10、Stop后hands89→124且held68不变、正常106／full及音频9／9恢复均通过；复用Body `runtime-review.json` SHA `2d73d4b0b67fd80546d319492e1d000d99238154ea022f7ad128ac0b3612e89d`、`runtime-final.json` SHA `c574abf9c20a6bb87fc818091f80a55130825aabea7327f7a59564bb2d445431`及保存摘要，不重解其128条v2／丢3或追未阻断协议的局部拒绝。R2关闭，实际可见跟随／权威瞄准／默认整合仍未证明，⑤保持统筹40%。后继新case须重基09e0554的build5后台策略与准确19b355／c12172选择，旧HF／HG pins不改。

Root在HG现成32715B PG日志取得9／9稳定before-after腕骨／枪体配对（4.9722–5.6081ms），gun相对bone52的固定-90°矩阵最大元素残差0.00024615；同raw输入时腕35／骨52／枪一起变化，最大相邻枪位移32.43099u／旋转32.77917°，角色CTW一致。只证明这些顺序观察点的附件关系，不能把非原子summary计数当精确writer时点或升为GPU稳定。新私有回执`analysis/lancer-component-readback-20261026/hg-saved-component-review.json` SHA `bbe78ada3f355fad5990b13b0aad000ab949a1aa59dd612c2fad9c246a38d01b`及入口保留所有raw、scope、原时标与EQ／ES旧证据hash。

Body复用既有hand-v3／BMS，确认仅PG epoch1295／1297分别能匹配Native seq1／2同mesh，consumer／end通过且PG发生其后；ready／move六个epoch均超出前32末1363，没有exact post-consumer join。Root未重复解码该前缀。单个既有助手只读现有aim／socket资料：历史MuzzleSocketName与静态9骨已知，当前Muzzle的实际Socket／BoneName／相对几何仍缺，不能用旧Magazine探针或+Y静态轴代替。下一交付在既有有界PG只读路径闭合当前枪口数据，必要共享Native改动交Body，继续保留mount_known=false及原CallFunction逐发消费者边界；本轮无新Game／编译／旧suite／Native修改或进度加分。

## 2026-10-27 — BHELD002早退解码交付；保留旧520合同，尚无新局修复结论

Body冻结仅追加实际Native／Goal返回观察的journal合同，header SHA `0abf184b0d3f2ab13ad9e36a45e0ee1132962447fd84f35c0bf5b131bc60d881`，BHELD002／version2，Header120／Record584，容量128／7秒不改。Root新增ignored `analysis/lancer-component-readback-20261026/decode_held_returns_v2.py`，SHA `3724aeb179f5a4e93a7b101f30d6d37b52cca9af0863a12b1af0d6ace9ad2401`；旧v1 reader及其交付全部source hash保持。共同字段通过去除新增8个U64的临时内存投影复用旧reader，原时标／身份逐字保留，未生成冒充原件的投影文件；两版相互拒绝不匹配的magic／version／layout，输出独占创建不覆盖旧证据。

raw captured／key flags不直接授本entry身份：actual Goal reason1..6及19(exception)的derived source／key均保持未记录，只有7..18已通过原wrong_entry／same_entry检查，且key再需rawsource1／rawkey1／initialized1及合法已记录19位。reason7本身未通过current_prepare，不能当freshness成功；adapter只在actual reason14解释，reason10等rawadapter0保留但显示未记录。NativeBegin0才是实际captured结果，UNKNOWN不当成功；Goal缺reason也不当成功。无新分支clock，at_ns仍null，后继apply／exception不替换已有guard／Kernel首拒。新增first_no_input_after_held_commit精确关联首个成功held后无input entry的index／tick／Compose与边界，仍不证明phase完整或复现旧HE历史帧。

复用Body Host29／0的584／120布局证据，Root不重建Native／PG／XR或重跑v1。v2定向10个合成方法一次通过（0.026s），覆盖上述reason5／6假有效flags、reason10默认adapter0、版本交叉拒绝、截断／身份／未知枚举、零mask与独占CLI等；2456B四记录样例明确标SYNTHETIC，非实际HF。说明`HELD-RETURN-V2-READER.md`和交付`held-return-v2-reader-delivery.json` SHA `08028f735f8734f24182f208cd851cc73b9d53927a4ae24687c41e03f952bee1`已交Body与统筹复用。Root没有新游戏／助手／玩法通过或默认替换，整体36%保持；后续只读Body唯一新候选正常drain的真实同局导出，不提前称修复。

## 2026-10-27 — he首份非空journal解码：取枪一次，阶段首个已记录拒绝在Kernel commit；原门与Stop续更失败

Body补充既有budget／Goal保存分析，Root仅核该同局摘要与三个共享回执hash并关联精确IDs，不重解raw／重审来源。Body `analysis/body-held-outcome-runtime-20261027e/goal-budget-saved-summary.json` SHA `985c5d9ac40791d8cf65b9286da9618c5175ffd789e6c0cef1638f9f1dd46b7e`记录实际budget9条query／no_query0，phase中只有exact query8／9。首query8的既有prepare_end／commit_end返回边界给出原Scene age48.9722／50.5291ms；这些不是首个Kernel失败谓词时刻或其Context verdict，仍不唯一归因age。全局first-failure tick29620／Compose29621发生phase前1172.3921ms，不匹配query8／9，不能填补当前Kernel拒绝。

后继Stop首拒reason10、keybits0x7e3ff，只有observed stream/reference/head从1→3不等，其余16项（含实际XR space、Native reference与参考记录）一致；实际时点在phase末后146.0534ms，只证明后继阻断，不能倒推首个无input entry index2／tick33464／Compose33465为何早退。Body准备在同一有界journal追加实际Goal返回原因及已计算keybits，不加guard／clock／engine读取；Root要求冻结新header hash／version／Record／Header及layout回执后再增量适配，UNKNOWN／valid位必须区分未到达与默认0，旧520 decoder和原件不覆盖。Body隔离PG1336并以真实guard CPU三路验证的dfd995e4…904669仅为CPU artifact；Root不重复测试、不宣称实跑修复。接收记录ignored `he-existing-budget-received.json`保留范围和摘要hash，无新Game／构建／来源门变更或进度加分。

Body唯一he局正常drain后交首份非空`body-held-outcomes.bin`，Game68132／Native3072491780786，PG `1336a597f7c7d47115e364e8c688fdba2c40ac3b9746ca13e902b3286c63990d`。Root直接复用已交decoder，以同局PID／Native及phase `[307257894873500,307263918239700]`调用，56800字节＝Header120＋109×Record520，errors0／invalid0，dropped／nested／foreign均0。未启动Game／测试／编译／IDA或全量重审。记录从phase起点晚37.116ms开始，虽然总span约6.094s并延伸至阶段末后107.7466ms，仍不完整覆盖请求phase；不能从容量或span提升为6秒验收。

本局确实记录一次Grip held(slot3,event1,press1,frame307,source2684)，原阶段Native mount只+1／40，整轮不通过。指定阶段完整落入窗口的106条中，第一条tick33250／Compose33251／query8：两个SourceGate均okay、三次实际Tick admission均okay／passed、选择held_lancer、Kernel prepare=prepared(0)，commit=guard_rejected(12)。下一条tick33357／Compose33358／query9记录commit=committed(1)、held_counter1／trace_sequence1。其余104条本阶段entry未记录input/query/guard/admission/Kernel结果（UNKNOWN），不能把默认值当通过，也不能仅凭未到达字段选定Goal早退原因；全journal此类107条。已记录首拒不是phase开始缺失37ms内所有可能失败的首个原因。

首条原Scene age在final admission为48.8083ms，entry end为50.8076ms；此时新controller age约12.3798ms、capture约9.1784ms。entry end还包含原Compose等后继工作，不是Kernel失败时刻；格式没有该时刻的Context首错／verdict和Kernel操作时标，不能仅凭末age越50ms认定唯一age根因。将精确tick／Compose／query交Body，若已有budget／first-failure可精确同局关联则复用，不能外推前缀或另开实验。

Stop后free-hand续更也超时，`body_continues_after_stop`没有赋真、bridge_error保留；原finalizer一次106／full／Native-entry-PG／XR0／十文件／audio9/9与fresh空接受仅cleanup。另Body／统筹指出实验PG1336的bulk失败转scalar fallback有PAGE_GUARD自动清除差分，Body独占真实CPU对照与失败即拒修正；该候选不具默认整合资格，也不将此差分冒充he根因。event行追加的previous_bound等扩展字段在Input尾部Evidence聚合重赋中变为默认0，不能当真实prior unbound／counter0；gap／baseline原已验证扩展字段不受该event重赋影响。此限制随本局事件证据保留，不为它另行重编。

Root产物ignored `analysis/lancer-component-readback-20261026/he-held-journal-decoded.json`及`he-phase-first-outcome-saved.json`，后者保留原件hash／两个实际Kernel结果／phase边界／缺证与安全限制，生成入口`analyze_he_phase_first_outcome.py`。一次取枪不等于稳定跟手、40-6s／Stop／射击或GPU验收，无默认替换、修复结论或玩法进度加分。

## 2026-10-27 — hd早采样候选实际生效但未修复；按下前gap后首次press被baseline吸收

Body在其私有`analysis/body-held-outcome-runtime-20261027d`只运行一次不同候选：PG `013c39f377b1fff7dec2897778004f6839c8b98ee995d19990a2028d444adc43`将已有early输入资格从held扩大到普通unheld，仍在真实当前clock执行原100ms，保守event.created、后续inventory／current／身份／attachment／250ms、baseline／pending／future保持，局部event早退丢弃、不重播。复用Body一次Astra增量审查及原CPU Input72case／778、Runtime84case／901验证，Root不重复。Native／XR未重编，不改40-6s或按住时长。

Game44240／Native3056117722988的窗口通过，但无装备结果／episodes0／未进held40-6s，journal120字节。仅关联两个目标帧：frame342／source3173在实际press3178之前，rawpress0／lastpress0已出现previous_sync_stale，next_after_inventory0；旧sync age102.8921ms、current25.1187ms、相邻sync差77.7734ms、Frame now→Input23.8938ms、consumer前后观察差66.9259ms。该gap观察还早于press命令写入7.5696ms；随后frame343／source3178第一条press1／lastpress3178成为baseline，previous_bound0、evidence_valid1、event0。故本候选早路径确实运行但未修复，此局为“先无按下gap，再新press遇到baseline”，不能回填成hc“按下所在帧被拒”的同一具体事件。只排除本Tick**后继**inventory是该early拒绝的唯一耗时；不定位前置工作、上一Tick、copy/current/binding或CPU／GPU成本。

Body原finalizer一次106／full／Native、entry、PG／XR0／十文件／audio9/9／fresh空恢复通过，仅cleanup。实际代码范围在清理后独立澄清并保留旧manifest／hd pins／原回执：相对hc当前仅`weapon_equipment_runtime.h`一个生产文件和`weapon_equipment_input_test.cpp`测试变化；历史“两个生产诊断字段文件”是更早diagnostic差分，不是当前early delta。澄清文件`analysis/equipment-input-early-admission-candidate1/source-manifest-scope-clarification.json` SHA `33c270d805a67a9e307b7a1669f07942936a6150ff10328e89283d99ffd7164d`；Root核该hash、原manifest和hd pins hash，不重建或重审全部source。

Root有界接收记录ignored `analysis/lancer-component-readback-20261026/hd-early-input-failure-received-20261027.json`。未将失败候选默认整合或增加玩法进度；原hc机制证据、hb缺证和旧ES均保留。Body后继只核共享binding读取成本候选与当前acquisition/current实际路径关系，Root不重复该调查、不新增泛日志、助手、编译或Game。

## 2026-10-27 — hc补齐同帧raw counter，具体RGrip边沿在时效拒绝／baseline吸收的链路闭合

Body私有`analysis/body-held-outcome-runtime-20261027c`已单次原动作运行并清理，原100ms／40-6s与脉冲时长不变。Root仅关联保存的两个目标帧（gap／同帧outcome／后继baseline三行），不新开Game／测试／编译或重审整套源码。Game64604／Native3046028507935，诊断PG `3eb4932cdc17e43a1889b51778d63a6033f58f5b976443466e7aa0ef2b8927ac`；复用Body私有PG仅加诊断字段／校验的证据，不把它视作输入修复。

frame278／source3553的rawpress1、release0、lastpress3550，存储的previous_bound1／source3547／press0／release0／lastpress0，实际reason previous_sync_stale且`next_after_inventory=1`；同帧outcome `input_evidence_valid=1`／sequence3553、baseline_waiting／event0，排除了旧evidence或把Outcome默认press0误当raw0。frame279／source3558实际baseline evidence_valid1／rawpress1／lastpress3550、previous_bound0／event0、`next_after_inventory=0`。本局slot3-press回执也为pressed1／lastpress3550，source/session/space/profile/input与gap行一致，3550位于3547与3553之间。因而特定边沿已进入consumer，在intent前的旧sync门被拒，重置后被baseline吸收；不是设备未发布按下，也不是intent生成后的装备动作门拒绝。这是hc本局闭合，不倒填hb缺失的raw字段。

hc旧sync age111.4226ms、current47.4542ms、相邻sync差63.9684ms，触发的是原100ms old-sync门；Frame now至input observation39.557ms，`next_after_inventory=1`只证明采样在inventory之后，不把整段时间宣称为inventory／CPU／GPU函数耗时。仍未equipment held／未进入原40-6s；Root没有改门、替换clock或重播press。Body原finalizer一次106／full／Native、entry、PG／XR0／十文件／audio9/9及fresh空恢复通过，Root复用其恢复回执；整轮无装备结果超时仍保留。

有界关联回执ignored `analysis/lancer-component-readback-20261026/hc-input-edge-closed-20261027.json`含目标行、同局身份／command关联及原件SHA。已交Body／统筹；Body按人类授权使用一位Astra作最小安全修复或唯一必要时标的窄问，Root不重复开助手或重跑。实际延迟归因与安全修复尚待结论，无默认合流或玩法进度加分。

## 2026-10-27 — hb第一轮RGrip首断点：输入旧sync门拒绝后重建baseline，尚未生成装备intent

完成统筹追加的窄任务，仅用hb命令／输入／装备回执及实际PG7e927e17…61aa冻结源码；核实际私有PG bytes与build receipt的Input／Runtime／backend／router四源码hash。Game73444／Native3029853403801，Body producer-timeline原件SHA `eaca4af6e22410167b9565b11a1f5ea7ef95473943a225e23bd3d2c3bffdef8c`复用，不重复XR来源／窗口调查。position／press／release的source/session/space/profile/input不变，producer pressed0→1→1、released0→0→1、lastpress2969；按下已在producer观察到。

只查看已知press附近267–278帧装备标签（上限48KiB），实际frame273／source2972进入`Input::next`的`previous_sync_stale`分支：实际观察302993948340200、previous sync302993840423700、current sync302993895962000；旧sync age107.9165ms触发原100ms门，当前sync age52.3782ms、相邻sync差55.5383ms，不能宣称XR sync间隔大于100ms。同期Outcome为baseline_waiting／source_gap／event0，frame274为baseline／event0。冻结Input:154在new_press／router／event生成前return，reject重置router／bound／cursorMAX；Runtime:324随后input_boundary。故已证首断点是消费者已进入、在intent生成前被时间门拒绝并重建baseline，不是消费者未运行或生成intent后被装备动作门拒绝。inventory_stage=ok只证明metadata步骤，不替代后续tap／switch。

当帧原始press_counter／last_press_sequence虽已复制到Evidence但未打印，前一基线counter／source sequence和后一基线consumed counter也缺；Outcome.press=0不是raw counter。因此机制与该按下被baseline吸收相容，尚未把特定边沿逐字段闭合。frame.now→input observed41.5446ms也只是一对原时标差，不是某函数／CPU／GPU已定位耗时；early-held／late-unheld next路径未逐帧记录。禁止用新2975替换保存2972或改写时钟让旧门通过。最小缺证点已交Body关联现有生产／消费边界，只需既有有界log保留owned counter／sequence、baseline消费值和采样路径；本任务没有实现该改动、重放输入或放宽门。

机器回执ignored `analysis/lancer-component-readback-20261026/hb-input-first-break-saved.json`，解释与源行号在`HB-INPUT-FIRST-BREAK.md`，生成入口`analyze_hb_input_first_break.py`。没有Game／测试／编译／IDA／助手或共享Native、武器源码更改；交接停在这个首断点。原hb装备结果超时／episodes0、真实journal仅空头、原ES及持枪40／6门问题仍开放；没有修复或玩法进度加分。

## 2026-10-27 — 独立交付held journal离线decoder；hb真实空头正确判无持枪条目

按统筹分工，Root仅在ignored `analysis/lancer-component-readback-20261026/`完成`decode_held_outcomes.py`／合成fixture／最小检查和交付说明。先查Body held候选／runtime／相关saved-analysis，只有生产原始导出和Host布局证据，无BHELD001 decoder；旧reader.exe为controller bridge格式，未混用。读取合同精确pin `held_outcome_journal.h` SHA `a18b7a947b876260adbf3f6b61de6bb5f5bf7b4c0847014479c9bbefb2bd5be1`，复用Body原Host20／0的Record520／Header120，不重建Native／XR／PG。

入口保留全部raw来源／capture／新Tick时标、两个before／after SourceGate与Context、三次实际admission及第四预留槽、mount、Kernel prepare／commit code和held／trace sequence；默认零／UNKNOWN／未到达／实际失败分开。首拒仅按冻结阶段的实际已记录结果归类，不把commit_end调用边界、旧Scene年龄或未知时间变成通过／唯一根因。PID／Native身份、尺寸／截断／尾部、枚举／时钟／slot丢失与容量overflow明确输出；依据实际span和可选原阶段范围标窗口不完整，128容量／7秒armed不证明6秒覆盖，`gameplay_acceptance`与完整phase证明始终不置真。6项合成定向检查通过，新增首拒摘要后相关2项复验通过，未扩矩阵／加助手／启动游戏或改武器、Native源码。

合成1680字节样例明确标SYNTHETIC，输出source_after拒绝、Kernel prepare拒绝、held commit记录三类且窗口不完整。Body hb正常drain后提供的真实`body-held-outcomes.bin`为120字节，SHA `b57c6bbf0543b8702f58b789d3f08214e2d9da74d17866400a5756a49e990561`，以同case PID73444／Native3029853403801实际调用CLI，正确输出0条、errors=[]、`empty_or_not_armed_no_runtime_outcome`，未把解码exit0当动作通过。当前仍无实际held阶段日志条目。

准确命令、输入合同与结果例在analysis的`HELD-JOURNAL-READER.md`；parser SHA `e86ee2cc0a9eccf53787433b79f4db039cfc86c04ec92fadaac16011557a4084`；交付回执`held-journal-reader-delivery.json` SHA `9fdc8dab6183ea819ce8015fd6e426df9261610a7a0c4045d91cab4e7fdfef8b`封存源码／synthetic与真实空文件结果／Host和enum来源hash。交Body后，下一检查点为以后合法原动作且正常drain的actual-held journal，不为reader重开一局。hb更早未取得equipment held结果、未进入原40／6秒门的首阻碍，依统筹新分工转由武器侧只读保存输入／装备回执及冻结consumer定位；不重复Body启动调查或生产侧时序采样，不改共享Native／放宽门／新增游戏、编译或助手。

## 2026-10-27 — Body held-journal A启动窗口拒绝；无持枪日志运行证据

Body在其私有`analysis/body-held-outcome-runtime-20261027a`单次0.4后台尝试，未到ready、Native未激活、XR未起、控制命令0，真实held日志采样／Stop／Goal epoch均未验证。保存WindowDetails为可见HWND处于iconic=true、rect约(-16000,-16000)、client0×0，原30秒／可见且client至少640×360门拒绝；此结果不证明Native／XR／新journal导致最小化。没有绕开原门、抢焦点、移动／恢复／调整窗口或重放原样持枪局。

原finalizer只执行一次，`accepted=false`及`KeyError('session_report')`原样保留；Body另立startup-cleanup-audit，而非改成运行会话通过。该审计记录prime取消／入口恢复、PG reservation-only shutdown0／entry恢复、十文件／设置存档不变、准确owner/Game及审计时fresh Game/XR空；本局音频实际2／2恢复、pending0（以本局审计为准）。`startup_cleanup_observed=true`不等于active Native／XR exit0或正常运行验收。Root仅核该审计四输入hash及音频原件hash，复用Body结论，没有启动Game／测试／编译／IDA或修改武器／共享Native。

审计SHA256 `8537d0f31c6cda317af6aa98c8bdd725413932775eef8dbd90d327112b762509`，Root接收记录ignored `analysis/lancer-component-readback-20261026/held-journal-startup-a-received-20261027.json`。原ES及B失败、Goal／来源缺证均保持开放；新journal仍仅离线候选，无默认合流、修复或进度加分。Body后继只离线比较quiet_startup窗口序列与成功B，FPS仍由人类暂停，Root不恢复它或另开GPU局。

## 2026-10-27 — Body Goal epoch B触发并记录瞬态uncaptured；持枪原门超时，旧ES持续停滞未复现

Body随后对同局保存片段作逐entry分析，Root仅核四原件hash并复用，未重跑／重解raw。26／26姿态样本读取成功、0 pose错误；`coherent=null`是原`mount>=40`谓词未满足后未选样，不能解释为姿态缺失或全数读取失败。10条v3 partial held trace按Body实际reader，9个planned atoms均与consumer／end相同、三个返回状态均1；仍未满32，不提升成完整验收。前32 budget只有25条落入该阶段，覆盖phase+0.0558至+2.0687秒，不能解释6秒后段。14条code3／branch7为回调后`body_mount_current` false，后续postquery admission age拒绝，未到hands；11条ready中1条final age拒绝、10条到commit调用结束边界，仅3条有精确tick／compose关联成功held trace；另7条没有逐entry Kernel结果／实际mount选择，不能用累计commit-code倒推。14条原Scene末age50.2164–51.665ms，但其bool资格包含多条件且没首错，新Tick raw年龄不在旧预算里，因此不能唯一归因于age或宣称新来源修复失败。

Body保存分析为其worktree `analysis/body-goal-epoch-runtime-20261027b/held-phase-saved-analysis.json`，SHA256 `1a04aac6549a5f8baeb3ebd6d5df31b9fcfb9f1c124686bfd511161a6582ae43`；Root接收记录ignored `goal-epoch-b-held-analysis-received-20261027.json`。下一步由Body只准备actual held阶段被动有界首错／admission／mount／Kernel结果journal，按tick／Compose／query连接，并保留原时间和结果，不追加guard评估、renew／重采样、GT I/O或放宽任何门；正常drain后导出。此处仅诊断准备方向，没有新局或代码合并／玩法验收结论。

Body私有`analysis/body-goal-epoch-runtime-20261027b`已单次0.4后台运行，原武器源码／父ES均未改。Root仅接收summary及ready／review／final保存回执，核session、输入hash和失败阶段字段，没有运行Game／测试／编译／IDA或合并Native。Game62936／Native2930735530854，诊断Native `40d2063db8fb32b7180f4c99740eae15a15fc9c5bd258a084a2735d52ee741e3`，PG仍`7e927e17…61aa`，pins `5baed00ba3acebdd342c5a4cefe01edc6857ada4da47ed4f1222bb77ab36d9bd`。阶段前后诊断absent epoch0，Stop后epoch1，实际触发已观察到；first reason4按Body冻结定义为uncaptured，记录没有可用source／key（相关字段0），不将这些0当有效来源或key相等证据。

本轮整轮不通过：`shoulder-fixed-first32`原40提交／6秒期限门超时，失败阶段累计held MainMount0→10、context rejects3→48、post-admission rejects3→49、source age rejects38→153、Native commits180→190；coherent=null。它们是同局失败区间的聚合计数，没有per-query／C0配对，不能由此认定独立来源修复失败或选定根因，亦不能降低原门或用后续恢复覆盖持枪失败。

Stop本次直接返回[5]并quiescent，随后Native commits206→235、hands27→56、Goal plans291→320，held MainMount10不变，Goal rejects128→129；observed stream/reference/head保持1。首拒后能继续+29，故本轮只是瞬态uncaptured和续更通过，没有复现旧ES代数2后的持续规划前拒绝，也没有解释或修复该旧失败。finalizer一次cleanup接受，原106／full chunks／Native、entry、PG／XR0／十文件／audio／准确PID恢复通过。Body已交窗FPS，无武器侧代码合并、默认集成或进度加分。接收回执ignored `analysis/lancer-component-readback-20261026/goal-epoch-b-received-20261027.json`，Body summary原SHA `0efe28b424e5368ba5efd506c97cc6fde9765851c36a226ce933ff98b4affe6b`；保留早先所有通过与失败记录。

## 2026-10-26 — Body诊断派生局续更通过，原ES失败未复现；首拒诊断覆盖未证

Body在其worktree从ES私有派生`analysis/body-goal-first-reject-runtime-20261026a`，使用其只读Goal first-reject候选Native `efdf83825f28a7a23b8a30d01338aa55aa9cbafefb7d17986bf492cc46f47462`，原动作与Stop恢复门运行一次。Root仅读其ready／runtime-review／runtime-final三保存回执及精确PID／Native，未重复其构建／测试或启动Game。Game59052／Native2854251650177，Stop4→5、quiescent、coordinator complete/errors0，基线后Native commits302→332、hands75→105、Goal plans349→379，held MainMount56不变、Goal rejected86不变；observed stream/reference/head均保持1。原106／full chunks／Native、entry、PG、XR0／十文件／audio／准确PID恢复通过。

诊断`body_goal_first_state`前后均0，且本轮Stop后没有新的Goal拒绝，不能据此证实首拒触发已正确覆盖。Body提出“cookie1→0与absent_quiescent隔帧使同帧epoch触发漏记”的源码推测，尚待其定向验证，Root不把它作为已证原因。此局是一次续更通过与原ES失败未复现，不是修复证明：ES在observed代数2后的Goal规划前持续拒绝仍开着，不能用新局覆盖它，也不跨进程硬配累计值。Root接收回执ignored `analysis/lancer-component-readback-20261026/goal-first1-received.json`记录三原件hash与范围；Body已交窗口给FPS，Root无Game／测试／编译／IDA或共享Native更改，没有默认合流或进度加分。

## 2026-10-26 — 仅复用 ER／ES 保存记录，Stop 续更差异缩到 Goal 规划前条件

按统筹19点有界交付，仅比较实际ER PID23040／Native2694171951921与ES PID31188／Native2701965669072，各自独立时间线和增量，不按行号／累计计数跨局硬配。两局Stop均4→5且quiescent基线cookie0／revision4／callbacks、transactions、pin0／admission与fault0。差异为ES此时内部observed stream/reference/head/hand代数由先前1变为2，ER实际续更通过窗口仍1；Native reference epoch2／flags1／failed0相同。ES后12条观察中ticks／Native captures／Goal attempts均+78、Goal rejects+78，但plans／MainMount查询／hands attempts与commits全0；ER通过窗口上述规划／查询／hands提交均+34，held提交0。`body_goal_view_held` ES+78是旧缓存视图使用计数，不能误称握枪。

冻结Native47源码`body_c0_goal_policy_live`的same_source_basis将observed stream／reference／head纳入中性基准key，初始化后key变化的pose_begin分支会poison并在规划前拒绝；运行计数与此路径相容，但没有保存实际origin key／poisoned／拒绝分支，尚不能定因或排除其他规划前条件。ES最早在quiescent后完整基线观察到代数2；前后live状态读取只是可能滞后的摘要，不是精确连续性事件时标。ER更晚的正常退出摘要也升为2、ES最终升3，不能把final摘要当Stop基线。

按实际头和session核对，ER／ES BMS都只是1–32前缀，最后记录分别早于quiescent基线4625.04／4519.46ms；single first-failure分别早8145.20／7867.91ms，均不覆盖Stop。只解既有头和leading时标，不重算来源／矩阵、不外推first32。差异表及缺证见`docs/weapon-stop-freehand-er-es-20261026.md`，机器回执ignored `analysis/lancer-component-readback-20261026/stop-er-es-saved-review.json`。交Body最小下一动作是在既有pose_begin首个Stop后拒绝处记录具体谓词及old/current key与session/tick/Compose，不增加Hook／writer或放宽资格。本次没有Game／XR／测试／编译／IDA或共享Native更改；ES整轮仍失败，原finalizer成功仅恢复通过，进度分不变。

## 2026-10-26 — ES取得9条骨骼／枪稳定配对；整组腕骨姿态分叉与放手续传仍失败

Body后继仅复用保存原件完成时标对照：PG rows4／5／6（epoch1644／1645／1647）被同局joint obs26→27／26→27／27→28括住，保存状态的MainMount commits均45→45；rows7／8／9（epoch1662／1664／1666）被obs30→31／31→32／31→32括住，均54→54；hands attempts／commits及Native commits也各自不变。v3／BMS前32条末epoch1612、perf_ns270208184290100，早于这些点1754.78–2932.96ms，不能配成当前epoch的C0。Root仅核对Body回执的PID／Native和三份输入hash，复用其结论，没有重复矩阵／journal推导。回执为Body worktree `analysis/cover-implementation-20261024/es-native47-pose-timing-review.json`，SHA256 `3054388dfe3ed3099f9e0627db1f3e2b0dd80b0ffca52538cc337728c63af81a`。这些是非原子、约0.2秒且可能滞后的状态摘要括号，只能说所保存计数未变，不能证明每个PG采样点绝无新的实际GT提交，更不能定位原动画／其他writer／GPU阶段。该限制与Body原回执一致；没有新实验、源码修复或进度加分。

Body CT10正常退出后明确交窗，Root仅运行已封存ES一次：Game31188／Native2701965669072，原shoulder门45次、ready9次、移转9次held提交。实际PG原组件9／9及新增pose pair9／9接受，前后epoch／全115 Space、Local、Ref／角色CTW相同、变化位图0，区间5.0517–5.7871ms，中位5.2277ms；不是ER外部两次顺序读的35.4808ms方法。每条实际角色asset／Ref52名及原source／hold／input关联一致。

保存矩阵按row-vector重算`gun_world * inverse(bone52_world)`，9条均同固定旋转矩阵`[0,-1,0; 1,0,0; 0,0,1]`与近零位移，最大元素差0.000029427游戏单位（旋转元素误差亦远小于该量）。ready同raw的样本4／5／6，腕35的z为1.54785／-12.62361／1.54778，bone52与枪z同步1.54783／-12.39745／1.54775；small-move同raw样本7／8／9也见整组高低姿态切换。角色CTW九条一致。这里直接支持“这些观察点枪没有单独偏离骨骼，而是腕35／骨51／52整组姿态变化”，不能由此定位动画／writer／attachment更新阶段的最终原因，更不能推出全程或GPU效果。原EQ分叉仍是待修问题，只是定位范围缩小。

本轮动作**不通过**：host83942 exit0、coordinator15041 exit1，report complete=false、errors=[]，首个失败单独保留为`No post-quiescent continuing free-hand commits / stable held counter`超时。HandStop[4,5]与quiescent通过后，baseline held68／hands70／handsAttempts474／NativeCommits464；后12条原pose观察这四个计数不增长，continuous_hand_ticks仍552→630。不能把tick继续等同自由双手继续，也不能被空errors列表或finalizer accepted掩盖该失败。Root将具体采样epoch／时间／矩阵和此计数交Body唯一writer所有者，对照保存的Native／BMS数据，不新增writer或放宽成功门。

原finalizer正常执行一次exit0；Native／entry／XR0／PG0／原106及完整块／十文件／audio／准确PID恢复全部通过，final accepted的scope仍仅host／cleanup。fresh CIM空，窗口已交回Body；没有重跑。Root独立保存数据复核`analysis/lancer-component-readback-20261026/es-saved-pose-review.json`，保留所有ER失败与原EQ记录。没有新Native构建、默认合流、稳定跟手／枪口射击／GPU／头显接受或进度加分。

## 2026-10-26 — ER原动作与恢复通过，外部配对39次全部拒绝

后继最小源码：在既有默认OFF的PG组件probe内加入Ref115／Space／Local／epoch／CTW两个有界快照，typed读实际角色SkeletalMesh资产并前后复核descriptor／identity／全局表，无引擎调用或新增Hook。两个快照包围枪组件重复读取，记录bone35／51／52前后世界矩阵、epoch、实际角色资产及Ref52的FName、变化位图；附加pair失败仍分别保留原组件读取结果，不把组件accepted偷换成pair accepted。采样24／8／三次／50ms预算、原Game／Frame时间资格及typed provider不变。此处仍是同PlayerTick的顺序只读cohort，非同C0／GPU或挂枪／射线许可。

新核心18个owned案例、实际typed适配器5个owned案例通过，覆盖稳定读取与world乘法、不可读、资产／master／数组bounds与地址溢出／NaN、结束epoch／header／asset／CTW变化、两次snapshot的bone35/52／Local／Ref／epoch／CTW变化，以及descriptor偏移／类型／读中selector变化。实际PG完整TU同原flags严格`/Zs /W4 /WX`通过；适配器fixture最初使用错误成员名的编译失败保留后修正，未运行该失败二进制。旧预算、Matrix、Body等检查直接复用，未重测。准确回执在ignored `pose-owned-1793010260063126400`（核心18通过／adapter初次编译失败）、`pose-adapter-owned-1793010288688309800`（5通过）及`pose-pg-syntax-1793010340722190600`。尚未构建新的PG DLL、运行新case或判断分叉原因。

后继源码`3c8b0e0`已单次完整PG编译／链接为`7e927e1748e1d3549fa13df83eaffa72013a14f658b9e93f9d13e3d3c08b61aa`，787源及vendor稳定，实际生产defines／FLAGS与PG38d完全相同，16导出名称保持，Native／共享合同／provider／XR无源码差异或重编。build `analysis/lancer-component-readback-20261026/pose-pg-build/attempt-1793010396695942700`。ES封存于`analysis/equipment-native-rgrip-runtime-20261026es`，pins `b6fdd6714e88ae480d1b187e2a008fbb91f0eb3c680bf48a3e7f822c5f8f76f6`，coordinator `818345328debc87889cf8d8c7f39ca09b54abeaa3ee153e3dce602ba6280c55b`。外部body_observation恢复为EQ原字节，不调用ER wrapper；其余原host／gate／轨迹／finalizer仅namespace／PG摘要替换，Body后继共享query差异仍通过私有pin原EQ旧字节隔离。全部私有AST／实际原pose入口import／原件保存／hash审计通过，最小证明`es-pose-sealed.json`，原two-session／finalizer一次及保护说明在ES `POSE-PAIR-README.md`。此处只完成封存，等待独占窗口运行；不宣称分叉原因、稳定跟手或头显接受。

Body CT8完整收尾并明确交窗后，Root仅执行已封存ER一次：host12792／coordinator39259均exit0，原finalizer一次exit0。Game23040／Native2694171951921，shoulder／ready／small-move实际45／8／8次held提交、原8命令complete且errors为空，39条原coherent姿态继续保留。9条原PG组件观察各自接受；外部新增pair却0／39接受：38次两次原骨骼观察比较发生变化，1次Query结束metadata／registry检查变化。pair开始到拒绝耗时28.7729–55.0061ms，中位35.4808ms；拒绝记录没有保留after字段，因此无法从现存数据定位哪项变化，更不能直接解释成原动画覆盖、attachment延迟或GPU闪烁。这次没有得到稳定配对，而非枪体稳定通过。保持现有拒绝条件，不重跑同一输入、不删失败。

HandStop[4,5]、quiescent／自由双手继续、控制器恢复／bridgeclose、Native／entry／原106及完整块、XR0／PG0／十文件／audio／准确PID恢复通过，final accepted的scope仅host／正常清理；host瞬时no_remaining_games=false保留。fresh CIM无Game／XR／执行worker，窗口已交Body。保存复核`analysis/lancer-component-readback-20261026/er-saved-pair-review.json`。下一最小候选只将骨骼与枪组件读取收进现有PG组件只读采样点，先做有界owned检查；不新增Hook、不改Native或身体writer、不扩展其他枪，不把同PlayerTick误称同C0或最终GPU阶段。稳定跟手、枪口／射击与头显接受继续待验。

## 2026-10-26 — EQ原动作已完成，固定输入下组件姿态分叉；配对观察离线通过

更新：Body明确交出空窗口后，Root仅执行一次已冻结EQ及原finalizer；Game20012／Native2635598176705，原shoulder40／6秒门通过，三个阶段实际新增43／9／10次coherent held提交，8命令完成、errors为空。9条组件记录各自重复读取通过，ready→small-move的一组相邻样本旋转约9.99998°；但small-move输入与epochs完全相同的后两条样本，组件位置差14.93523游戏单位、角度差24.06287°。最后样本在该阶段结束前640.34ms，不能归因于Stop清理。Avatar矩阵9条相同；只能确认组件响应，尚未证明稳定跟手或分叉原因。旧异步骨骼样本距这些行24–105ms，不能硬配成同次姿态。每条组件采样约4.96–5.23ms，包含普通tick的33–37ms跨度不能全归于probe。保存证据与Root只读复核位于ignored `analysis/lancer-component-readback-20261026/eq-saved-component-review.json`。

正常HandStop5、quiescent后自由双手继续、控制器恢复／bridgeclose、Native与entry恢复、原106／完整保护块、XR0／PG0、十文件／audio和精确PID收尾通过；host瞬时`no_remaining_games=false`原值保留，不覆盖最终清理结果。运行窗口已交回Body，当前FPS随后使用窗口，Root不抢占。

针对分叉，源码`59e2cca`新增`tools/game-playtest/weapon_component_observation.py`，在原`probe(observer)`外包`wrap_probe`：原骨骼读回→typed枪／角色组件／唯一weapon_end attachment→第二次原骨骼读回→实际Query身份／属性选择／字段与注册表末读→骨骼epoch／Space／Local／CTW末读→再末读Query。附加记录失败仍返回第一次原姿态结果；第一次原姿态本身失败仍原样抛出。不会放宽原40／6秒成功条件，不写游戏、不调用engine getter、不改Native／PG／XR。有限顺序重复读取明确不是同C0原子快照，也不证明最终GPU阶段、枪口或射击。

实际生产pair reader＋现有`world_state.Query`在owned内存fixture上16项定向测试通过，含稳定相对矩阵、epoch／Space／Local／reference变化、组件迟变、两次末读、属性选择／身份／注册表变化、错误／重复attachment、不可读、附加与原姿态失败区别、discovery hints拒绝和仿射逆矩阵。fixture姿态callback是seam，未冒称又验证一次原live骨骼算法。回执`analysis/lancer-component-readback-20261026/pair-observer-offline-receipt.json`，SHA256 `5e8a5899ca52390ef64c9c7ee0363e3ee70f0215c5b6846bb881ce2c0672018a`。

ER已接入口并封存：`analysis/equipment-native-rgrip-runtime-20261026er`，pins `4618c3bd9ac62e0ad4d32f50cb7fe24b55aaf5465eee817a2ceb99465e5a4f3b`，原coordinator摘要仍`0cb264965702af66b83796c29297c2fd7bbfad1d51c6c73ec225827287f83b08`。原`body_observation.py`字节完整保留为前缀，仅末尾追加alias／wrapper；实际私有import确认`probe.__wrapped__`就是原函数。helper／world_state各自复制并pin，全部私有Python AST、原host／轨迹／gate／finalizer除namespace外相同、原件不变通过。首次clone因Body共享cover_surface_query后继改动被guard拒绝，未启动游戏；ER保留该拒绝，将hash完全等于原EQ的旧文件复制为私有同名依赖，按现有import优先级使用且准确pin，不回写Body、不接纳无关后继差异。完整最小证明在ignored `er-pair-sealed.json`及ER `PAIR-README.md`。未重建未变Native／PG／XR，也未重新运行旧检查；等待现有独占窗口交接，只跑原动作。稳定跟手、sameC0／画面／枪口／射击及头显接受均待验。

以下保留EQ封存时的历史交付说明；“待运行／Root不launch”描述的是该时刻，已被上方实际运行记录更新。

源码`28babdd`经单次PG-only完整编译／链接，得DLL `38d897b4ff4f355b958874235327a1851a9acb5fc9c7bfa05be91774d3a68912`，783源及vendor不变；实际生产defines相对EP仅加`GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE=1`，原flags及16个PG导出名称保持。新flag OFF后原tick／playground文本还原，Native／共享合同／typed provider未改，旧检查不重测。精确build在ignored `analysis/lancer-component-readback-20261026/pg-build/attempt-1792999696227354400/`。

从EP派生EQ，最终pins `2e8591fa29b289d62d572cdcdf7c5a3c4957b61e18dff0e00bc98951947986ac`，coordinator `0cb264965702af66b83796c29297c2fd7bbfad1d51c6c73ec225827287f83b08`。Native47／BQ／CY、INI及原8动作／40held-6秒／106／R2 OFF／0.4后台保护不变，全部private Python只必要namespace和PG／coordinator摘要替换，原EP输入／成功结果／BMS-v3-alpha原件保留。Body独立reader的实际Record8616与准确exe摘要记入选择元数据，不修改动作或finalizer。

EQ已交Body独立审diff后独占执行，Root不launch，交付时无instances／ready，交付后冻结。组件采样将按shoulder／ready／small-move实际marker与时间分析；每条只代表parent V1元数据观察，显式不证明同C0、GPU、枪口或射击许可。COMPONENT-README与component-selection-preflight在ignored case给出完整原两session／normal finalize及保护证明。源码／链接／本地检查不是实际组件跟随通过，等待唯一原动作读回。

## 2026-10-26 — EP原持枪门与移转通过，接续原枪组件只读验证

Body以Native47／PG5a34完成EP原动作并正常退出：原40／6秒门、right-ready8次、小移＋10°10次提交均有coherent post-FK。Root只读复核三raw头及32条sequence／tick／Compose／thread关联，独立重算v3腕FK位置／Local，最大重算位置误差0.000255116游戏单位；四角色实际Applied各32、640event0drop。Body严格新BMS reader的完整source／ready回声关联与新目标派生按其证据复用，不冒充Root重新推导全部来源；32条也不扩充为全程62条姿态。HandStop5／quiescent／自由双手继续／bridgeclose与正常106等通过，cleanup-only不替代玩法。PG保留first32前缀6 unheld／15 ready／11 inactive，与BMS成功提交集合不同，均保留。现存证据只读复核，未重跑Game。

Root下一单项交付为原Lancer组件位置／方向读回：新增默认OFF的有界normal PlayerTick probe，保存原copy_frame真实Game输出、直接反射读枪／角色Matrix并前后重复、双typed资产／唯一attachment／末Game／hold／domain／Frame资格检查，不调用原bone/socket getter、不写枪／相机／射线、不以新metadata witness授权普通GT动作。运动触发采样给后继ready／移转保留预算；47预算断言、实际Matrix reader8 owned案例与完整PG TU严格语法通过，旧Body／Client检查复用。具体边界在`docs/weapon-lancer-component-readback-20261026.md`，源码候选尚不代表GPU画面跟随／真实枪口／瞄准射击／Cover／头显通过。

## 2026-10-26 — Native47／PG typed联合候选EP封存并交Body，尚未运行

Body交付Native47 `78705ddd823a79a88aed943d559decb3bc873ce32eaf6d05ae9bba7c3d731348`后，Root仅对已封存`f611000`源码做一次PG feature DLL完整编译／链接，得`5a34a2808a1fb066091841f2d08b474650fda142cda55fb3cdb61870e4f3be0b`。778源与vendor保持，实际Ninja生产defines相对f85只增BodyMainMount新flag、原flags不变，无test-stats宏；既有Host结果直接复用，没有重建Native／XR或为报告重测。唯一构建及receipt在ignored `analysis/body-mount-pg-candidate-20261026/pg-build/attempt-1792994530678841000/`。

从EO派生EP，最终pins `fc3c560f3d6cebe6305347154fc1cd8d70fe2a09ffb52f6cbb024d8b26e08885`，coordinator `91bd06a0ad0a6cb3ef8780c02e7e1963ff23b479854cb2ab4f41e07df47664c9`。Native receipt `cae944a5…60ed`、manifest `6ed5c005…bf75`、source47-before-build3 `b14c684b…a4af`及source2全部787源hash复核，14改5增；Native44导出只比46多独立Attach，原defines只加新source开关，PG16导出名称保持。BQ／CY保持，不采用Native generic manifest内旧XR／PG选择。INI仅新增`BodyMainMountSource=1`，保留BodyTickInput1／旧probe0，启动guard确认新节及实际enabled状态；动作coordinator只PG摘要替换，原轨迹／8命令／40held-6秒／106／0.4后台保护不变。

新typed原件为BMSCOM01／Header64／Record8616，`body-mount-source-commits.bin`，禁止套旧Tick reader；v3／alpha与cleanup-only finalizer不变。封存时Body独立reader尚pending，随后交付reader `d3a21d45…0aaa`及actual schema头`2eb74168…901c`，Root只验hash并在area补`bms-reader-supplement.json`，不重封或修改EP；Body自行读取，不把shape／source join或postcommit年龄当精确CAS／真实GT／Kernel证明。

EP已交Body独立审动作diff后独占运行，交付时无instances／ready，Root未启动Game／XR，EO输入／结果／三raw完整保留。完整命令在EP的NATIVE47-README，实际源码与联合构建交付在area的delivery-source／joint-build-delivery和EP native47-selection-preflight。联合源码／链接通过仍非持续玩法或头显通过，整体36%／W2 40%不变，无默认集成或发布。

## 2026-10-26 — PG typed Tick／C0装备观察源码与有界Host通过，待Source47联合构建

Body共享头按SHA `6ad0a08fb3633bb0cc3d39841634a69c2a65771ed29886e7c32ceeb9c03b4b3d`逐字接入，Root新增独立identity-only Game／asset／attachment入口、Client单Gate／epoch同步metadata操作和typed sender／provider，新flag默认OFF且依赖原provider。原V1 Client／Frame fresh方法与读取函数体不改；新旧provider共享entered／callbacks／parent pin／drain。两Game包住双asset与唯一attachment，再验holdGeneration／held_binding／domain／epoch；真实begin计metadata50，新raw／capture40与原Scene50各自验证。Native reference_revision仅非0偶数shape／echo，与原StereoGate reference_generation明确不同；pub／ticket／action为Native认证关联，PG不伪称独立观测，不造Frame／world pose／grant。

严格定向结果：sender272 checks、实际Client新操作80 checks、actual typed Game／asset24 checks、actual attachment9 cases（owned解析seams）、actual provider+equipment编排271 checks（owned metadata／Client seams）及原Client15+30+17+8 cases通过。实际PG完整TU同f85 flags加新flag通过`/Zs /W4 /WX`，没有生成obj／DLL。一次Astra PG增量风险审查无具体P1/P2；Native实际source认证仍是独立前提。差分证明原V1在去掉纯新增／flag围栏后逐字还原，typed reader除入口签名外逻辑与原版逐字一致，f85／EO保持冻结。见`docs/weapon-body-main-mount-implementation-20261026.md`与ignored `analysis/body-mount-pg-candidate-20261026/`。

此轮未构建PG／Native DLL、建case或Game／XR。等待Body冻结Source47／新export后再一次联合构建与原40／6秒动作；源码／Host与语法接受不能抬升持续玩法成熟度，整体36%／W2 40%不变。AGENTS外部修改保留不入本提交。

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

Native28在实际冻结27上只迁入4既有／9新文件，727文件／83defines，保留原82项定义、42导出、26c freeze、FutureG、strict2bc与原渲染。Optional三参入口不再读取caller第五槽，Root五参保留；返回packet在全部观察复读后比较，失配拒绝并只恢复Mod自有packet。normal真实356项定向CTest及15 DLL／8 component预检通过；sim真实TU／预检通过，356项按实际依赖和flags闭包复用。ready SHA256 `651e2b37464423d21c5ffb314a03746cc2449119f71fbc96c3e82118c9a9af51`；normal DLL `43476a3837f47f1d8e6cb96b5d5186aeb7c3701c0c1080eae3ac9ba134d7c41d`，sim `25c3658573bf1da8d1cf2ed030511863188752436e0ce6861931118b81756f81`。sim脚本的父源计数在执行前由718修为727，缺失独审报告补齐，旧冻结源保持。

`fresh-producer-ro1`以0.4／behind／observe-only／mute完成一次既有FP toggle，无Pose event／射击；实际session1765175332446／PID51640／GT20748，tick282／Compose283得到五stage全部31的真实Root→Optional→C0样本。FPOSE001共66480B，SHA256 `d3278f14cf526a8e25af868c86d5cf7c9954ab8882d2da8d7006a974c4b4e4ee`；Root第五参数真实读取，Optional第五为零，packet／真实闭合通过。外部身份／模块base和session独立关联，完整99项恢复（旧96＋F7940 hot/cold及109970三完整chunk）、Native／entry、XR0、音频10、原十文件及准确PID退出通过。最终实际typed复核为`fresh-producer-ro1/runtime-fresh-review1.json`，SHA256 `661a6d365d8bc946199b3eacd67c470ebb355e24eac929a0d3ec008c25ab3801`。root70=0／cache空，首child weight1／flags0的v270仍为已挂钩F7940；不能由这一层或端点相等推成fresh全覆盖。只补实际执行递归分支所需的有限call树，不扩整个离线动画树。

保存的F7940还存在零贡献时直接把旧Local送入F6D8C缓存的路径，cache-miss返回不授退役receipt。静态审查与真实样本继续分开。阈值常量由原始RIP字节解为实际RVA17070D8／17070D4（raw `acc52737`／`58ff7f3f`），旧IDA全局表达均少1000；机械证据为ignored `analysis/fresh-producer-branch-constants1.json`。新测试不得采用旧表达地址或把weight==1作为全部branch条件。

全身C0事务内核接入tracked：完整Source／Output／Desired及115骨FK、changed mask、段长和七个marker复核之后，仅对变化的Q16及Root T＋原scale16使用CAS；最多116单元。失败逆序exact ours→before撤销，未知值保留责任。消费通知在任何身份拒绝前闩锁，原返回／异常的typed finally也兜底；原Compose开始使用后绝不回写旧C0。后来同owner／旧存储且真实覆盖全部欠责字段的已认证producer才能退役，值相等、错end或部分覆盖不能清债。最初错误通知后仍可能回滚的P1已修复，最终两份独审及冻结delivery保持。Root仅适配一个include，以当前真实共享headers严格CL重编，原RO3和本次新采集的精确C0各24741 checks／0fail，真实本进程CAS128及PAGE_NOACCESS SEH通过；计数是13个实际单元逐字段／故障矩阵断言，不是24741个玩法场景。接收回执为`full-body-c0-transaction-tracked-receiver1/cpu-1792918283854668100/receipt.json`，SHA256 `76d59ebcb247876b40aebc383f5728f1fe0fd8b7a10c1a712cb786f12d2cbce7`。Native backend仍明确未实现／拒绝，Host通过不等同Game写入、独占区间或物理验收；真实接线继续进行。

新增纯`body_head_authored_mount.h`，从完整已签名的静态Ref与独立骨名关联选择Head55、LtEye57／RtEye60，以两眼中点构造固定Head→Eye marker，中立HMD方向显式采用作者指定的UE actor +X／+Y／+Z，不读取当前动画Local／CTW或旧FP camera自动校准。Eye/HMD UE-world pose与Head骨pose使用不同类型；转换保原Eye输入，每次复核完整canonical／Ref，未把脸骨间距作为玩家IPD。固定eye位置纯旋转时，Head骨因固定offset走弧，该骨delta不能再次计入physical／Root／Pelvis。原世界参考下的固定Head局部offset约(4.86286695,-8.56014083,-0.00011918)原单位；这只是designer_authored模板，不是物理校准或相机高度修复已生效。

Root逐项验冻结交付后原字节接入，当前真实共享headers严格CL重编；原RO3及最新fresh-ro1完整Ref各1013 checks／0fail，包含独立quaternion FK与192组固定eye位置旋转往返、错模型／Ref／输入和失败输出不变。实际最大往返位置误差约8.7e-15u，独立FK差约1.17e-6u。回执`body-head-authored-tracked-receiver1/cpu-1792919063575648600/receipt.json`，SHA256 `50c664222ebada11c599b0dcfdad3fee1602f6f61e5259848e9ddae8b131d7ec`。真实protected cover neutral、Rig与renderer共用base和Native body writer仍待接线，未替换默认或宣称HMD视觉接受。

Native29仅在实际28上4旧／1新增加有界实际F7940调用记录，728文件／83旧defines完全不变，42exports和旧FPOSE ABI保持。Normal真实374项定向CTest及15 DLL／8 component通过，sim真实TU／预检与实际闭包复用374通过；ready SHA256 `0b8d75e46aebe2635fae90be70478edb7b080d43037a077398c9a5494ec14cd7`，normal DLL `ddc16beb34047e5e095a95f692ebe12115360dd1053f98403db2f111c979b8b3`、sim `cdb75328edf4ec21cc8d7d65adab14640c3757677b5ae96d5d77fbe94058450c`。派生脚本的recipe键／source token及sim旧输出名错误均在编译前保留失败记录；source29sim已stage后只继续未完成的编译／复用审核，没有为元数据重复已通过的normal374。

`fresh-tree-ro1`实际session1796769383690／PID30400／GT55932，tick300／Compose301取得13次真实F7940入场，全部原返回／exact end／packet保全；树86064B，SHA256 `6d176e046e6ac8d986fdb9349b237bb583d932dd6134b55754be39d126b17747`。99项恢复、Native／entry、XR0、音频10、原十文件及自有PID退出通过。`transport=true`但`accepted=false`原样保留：7个unknown边分别为F5C1D、10B561、385817及四个FADC4；out／Desired／R9／fifth没有变化，是未观察的override层缺口，未把最近F7940 parent冒称完整原生父。实际树分析在ignored `analysis/fresh-tree-runtime-branch-review1.json`。Root只对实际6目标及当前leaf／cache helper做私有IDA机械导出，原404ae IDB保持，所有Owned CLI exit0；F8AF4 cache-miss进入E08A0，故不能只凭返回或copy库模块名推全frame洁净。

全身planner增加显式±π heading，以及可选`world_absolute`目标。骨盆始终追上游稳定neutral world＋physical＋committed Grip各一次，Root使用显式世界Q；绝对模式拒非零relative yaw，Head／feet仍是实际绝对目标。默认旧relative行为保持，脚不会暗移，不可达完整Output不变。Root接收合并heading与absolute，仅对测试include作两个准确替换、将旧越界反例更新到新π上限之外；当前真实共享headers严格CL编译与365012 checks／0fail通过，其中13504为旧计划／heading，351508为1000帧回灌逐骨不变量，并非365012个玩法场景。Root／Pelvis回灌漂移0，head／feet残差约6.63e-5／6.26e-4u，48份继承产物逐bytes一致。回执`full-body-c0-absolute-tracked-receiver1/cpu-1792922639039692300/receipt.json`，SHA256 `53fde47e52cb0d183b05cb0ad0e8b2c0a901f04dd3c7dea1f53860038477acfc`。私有Goal从232增至296B，消费者需同步重编；Frame／Output及公开wire未变。稳定neutral、朝墙方向和脚步策略仍由上游actor／Cover提供，不能从含旧偏移的动画或上次Output重新校准。

私有实际Native Backend已实现真实read／CAS128及同latch C0接点，c2修复了结束路径错误依赖新工作admission的P2：stop／Core过期／hand sticky不再阻止原World lease仍有效的exact finally，错Entry／metadata／冲突仍保债。实际live＋actor reader严格638检查和Native ON／OFF编译／42exports通过；最终独审为ignored `analysis/full-body-c0-native-backend-final-audit2.md`。当前Goal策略尚未调用，字段覆写receipt正在实现，未在Game提交body pose。后续以原生真实Q16／T12＋scale4写入事件退役Local责任，区分字段覆盖与全frame BaseClean；时间更新、相同数值或未知解码格式不能代替store证据。同步C0入口契约仅作为工程推断，不宣称全程序reader／physics fence。

真实Native内存核心的header／65项Host测试现已原字节接入tracked；只接内存后端，不夹带旧28 capture／CMake或尚未组合的live。Root以当前Goal296、事务及实际共享headers严格CL重编通过65／0，覆盖真实进程read／CAS128／PAGE_NOACCESS SEH、混合source／ours、非owned字段／Ref／Desired／header／CTW改变、单次地址permit及消费后的保债。实际Root includes和前后hash见`full-body-c0-native-core-tracked-receiver1/cpu-1792923404915454200/receipt.json`，SHA256 `db77cb73410a6d6d692d75170d3dbe45184697ff6f385fc9f669a040ab240d58`；它不代替638项实际live审查，也没有声称已在Game写入。

纯`body_goal_view_adapter.h`及其测试已原字节接入tracked，复用当前Goal296、静态Head/Eye marker与实际共享Rig/Core headers。稳定neutral、原始HMD及已提交Grip偏移转换为同一绝对Head目标和pre-HMD render base；物理位移各进入头部／身体一次，render base仅含neutral／virtual，旧Core.world_head与动画Local不参与重新校准。Root严格CL与真实Core1832／完整Ref115重编实跑159检查／0失败，覆盖六向平移、纯旋转、保留Grip、独立轴向oracle和失败输出不变。回执`body-goal-view-tracked-receiver1/cpu-1792925368609010900/receipt.json`，SHA256 `638f520ba7ad8f8dceca348c6daf6c21a0da1d240cf4df36cc7d6eb782b2be39`；没有Native权限或Game／碰撞／头显验收结论。CMake仅在显式提供私有Core与C0／Ref时注册该fixture用例。

Native30私有组合已完成：从actual29的728文件复制后，仅迁入经过逐项SHA校验的Atom5旧／15新增增量，再加入当前Goal296／Head marker、GoalPolicy与Host兼容增量；最终762文件、5个旧文件修改，原83配置仅追加3项默认OFF能力。真实129CC0/E0A73支持字段覆写责任退役，独立World Scope不被称作Root scope，不推出BaseClean／Space／physics或新写许可。旧SM／装备变化使用当前存储认证结束旧责任，新工作继续走原SM／武器准入。已修正动画数组误用骨骼512上限的问题，并用非授权`may_match`在完整校验前过滤NPC／无bound／非整Atom；所有原调用和物理finally计数保留。独立最终Host1088／0、真实MinHook17／0；Goal与新receipt接口兼容Host265／0，均明确seam与无Game范围。

本次C0策略确实以同latch Core执行adapter和Native.apply，首次完整C0注册component-local neutral，以后只随CTW搬运。原Compose正常返回、exact end与无协议错误才发布；Scene已在FP／run／cover准备后读取同一个pre-HMD base，供stereo和Rig一起使用，并排除camera_closer再次修改；`body_goal_view_used`记录实际getter成功次数。当前仅为受控standing SM0／typed CT0／两项crouch=false、固定脚、committed virtual0；完整Cover／Grip／运动／碰撞／射击仍未接完，不能将standing组合编译视为完整掩体接受。

Normal真实capture TU／Atom ASM／DLL、416项定向CTest及23预检（15 DLL＋8 component）全部通过；sim仅沿用原有单处operator guard，真实重新编译TU／ASM／DLL及23预检，416项CPU以实际Ninja依赖闭包、源／CMake／生成对象flags相同且排除唯一capture.cpp差异后复用。42 exports、原渲染flags、原色彩／菜单与独立左history保持。ready为ignored `analysis/cover-implementation-20261024/native-body30-ready-final1.json`，SHA256 `193f0dbb3616b9e973ede132a8ec9e9b54101acfd6e0c440b1493216fcc8e902`；normal DLL `9842056b7717d4ba6c27d3bec22111ff014eeb9cee251483552981b4062e85c3`，sim `e619be88d7487f10db4d7c68069b02e5af0f07357bea0857f7648de6e8c76047`。Native30没有启动Game／XR，没有替换默认或取得连续身体／头显接受。

保留normal编译完成后的Python日志编码失败及修正前脚本；只恢复尚未执行的预检／CTest，没有重复编译。sim生成器源目录token遗漏在任何复制／编译前被拒，修正前脚本和原derivation回执保留；旧29／29sim源码及产物未被覆盖。完整掩体整体进度继续按约35%报告，不把离线检查数换算成游戏内功能进度。下一步为带新增Atom完整入口恢复核验的0.4后台standing连续身体验证，然后接完整Cover交互。

`body-live-standing1`已在购买版实际运行Native30sim＋既有source1 XR，0.4／behind／observe-only／mute及原十文件保护保留。session1869370775539、Game PID6812，Native captures3075、实际commit1；Goal plans2087／applied1／published1／view_used1，Atom qualified65，最终body／Goal／Atom pending全0。证明真实身体和Scene路径接通过一次，连续更新仍失败；`runtime-final.accepted=false`原样保留。当前聚合rejected不能区分plan、prepare或commit，下一步以第一次真实拒绝的分流／输入快照定位，不能凭pending0称连续成功或直接清Kernel错误。

该局正常停止，Native／entry恢复、XR exit0、PG shutdown0／entry、音频10、原10文件hash和精确自有PID退出均通过；新增Atom7完整chunks加既有99，共106项独立恢复全部匹配。宿主刚返回时`no_remaining_games=false`瞬时记录保留，后续独立核验和fresh CIM均无Game／XR。该局未发送模拟头显位移／旋转命令，不能作位移响应接受；外部post-FK探针149样本均被自身仅接受ObjectProperty的Mesh类型条件拒绝，与Native一次后拒绝分开。后续私有standing2生成器兼容当前Native支持的ComponentProperty并重复验证属性metadata／指针，添加实际Game／owner／XR创建时间、image及8720 listener检查。私有精简审查`body-live-standing1/runtime-body-review1.json` SHA256 `f8934f6a1142b94558248edbab32ea56cce909a225242b8ba6714b6fde6b0e30`；完整Cover／碰撞／受击／物理头显仍未接受。

Native31仅在30上增加首失败诊断、Kernel的const停止状态getter及对应Host工具，目标是区分真实第一处拒绝而不被后续stopped覆盖。Root normal实际433项定向CTest／23预检通过；sim重新编译TU／ASM／DLL与23预检，通过实际源／依赖／flags闭包复用433项CPU。ready SHA256 `adc0f8203be4ef2c3e9440f7548f969a1e3c110fb2e5878bcdd2d7658d790c91`，normal `3d1278123325a37b14b70aa4ed28f1b1b0a8765ba4155f1bc6db140a44a61a51`、sim `d7d36b2202e874a20d15924f96ff97195292a142f58db674a6c988f7c249ebf2`。所有身份／时效／事务校验、neutral、渲染与公开wire不变；诊断默认OFF，只在B2私有INI启用，GT不写文件。

`body-live-standing2`购买版实际取得首失败16896B快照，SHA256 `e607f48d4d17e8910dac1488f25dc90b8ff21f4fb0069731482231775657383a`。Root实际CPU重放得到plan solved、saved report完全一致、head error约7.93e-6u；真实第一次拒绝为commit stage／guard_rejected／unit6，Kernel failed+stopping，fields0／protocol0，随后337次prepare=stopped。快照记录apply约23.85ms，其中commit含回滚约22.64ms；Core源龄24.52→48.37ms，Stereo29.03→52.89ms、controller29.01→52.86ms，后两者在快照结束时超过原50ms门。`same_source→read_tick→current_copy`前后验证这些年龄；精确拒绝瞬间的年龄未单独采样，不将结束时间冒充该瞬间。下一步仅优化重复7chunk完整SHA／provider热路径：保留每次完整当前字节／hook／caller验证和50ms门，再实测连续行为，不以重置Kernel或重新校neutral换通过。

B2在模拟头动前被独立standing样本条件拒绝，未发送HeadPose命令；第一处原生失败仍在早先C0处理中被有效记录。正常106项独立恢复、Native／entry／XR0、原十文件、音频10及自有PID退出全部通过，`accepted=false`与watch终止记录保留。私有精简审查`body-live-standing2/runtime-first-failure-review1.json` SHA256 `1586a92cf392426046723cd9ed01d27c688956f555fefef0bded5287d80d5b7c`。当前优先级依已获用户认可的`J:/gearsVr/docs/6dof-implementation-roadmap-20261025.md`收敛到固定站立场景连续身体／相机／双手闭环；Cover live候选冻结保存，暂不扩大复杂状态。总目标仍为完整6DoF掩体，不把此中间交付视为完成。

Native32保留原50ms、每次完整代码字节／hook／caller认证，仅将已验证7chunk SHA/provider初始化从每单元热路径移到注册阶段。B3因私有包遗漏CaptureSelfTest而在任何Game／XR前失败，已保留该失败；补齐原字节9个helpers，并从实际将启动的目录执行22项startup检查。B4购买版0.4后台静止观察约31.9秒取得1663次commit／publish增量、146份有效post-FK样本；聚合accepted=true仅适用于静止连续路径，未发送HMD位移。其首个cas_fault发生于正常stop请求后约113.8ms，最终责任全部结清；不能作为观察期硬件CAS故障结论。

B5的8个私有模拟头显命令（中立、右3cm、左3cm、上2cm、下2cm、yaw、pitch、回中）全部被当前隔离runtime消费并逐项读回，但body在轨迹前已永久停写，commit／publish仍各1；43份post-FK样本不构成运动响应通过，accepted=false保留。首失败位于commit／unit1，apply约1.20ms，Stereo源龄49.1487→50.3476ms。当前backend会将原子操作前Context拒绝与真正未知CAS故障同算fault_unknown，Kernel随后永久停止；该具体问题与B2热路径开销分开记录。B4／B5均完成106项独立恢复、Native／entry／XR0、音频10、原十文件和精确自有PID清理。

后继私有Native33／34增加精确age-only分类、有效permit且未调用CAS的no-write结果、hard优先及准确End／零债务后的新Kernel episode；所有owner／ref／storage／source basis及registry重新认证，serial续号，跨线程drain只操作永久stop标志。额外40ms准入为原50ms逐单元门预留预算；不能将旧时间改成新时间。中立camera基准独立于旧Core寿命与本次apply结果，仍检查当前Scene／Ref／CTW／新鲜Stereo，HMD只由原stereo路径应用一次；held view不代表body写入成功。窄审发现恢复候选自己再次超时可能误升hard，34以函数内okay／defer／hard和Host模式27修正，33原源与回执保留。

typed／transaction／真实Backend定向Host及34个live集成模式通过，包含部分写入后超时回滚、未调用CAS证明、身份／错scope／真实fault／exception／stop不恢复、恢复再次超时延期和旧body跳帧时current-CTW camera基准。34sim实际TU／DLL、42exports、原flags、23预检及实际包22项startup通过，DLL SHA256 `111b0404ff6d01106c8a6d2d459f7930dbb22696947d4671ac44cdb9da983539`。证据在ignored `analysis/native-c0-transient-recovery-candidate1/`；B7使用原0.4／behind／observe-only／mute保护和既有8步轨迹开始目标游戏验证，尚未据此接受运动body、双手、完整Cover或实体头显。没有替换默认冻结构建。

B7在头动前被旧FP toggle的三条件后台guard拒绝，正常106项恢复，未发HMD命令；短世界阶段171次commit／publish不能代替运动验收。B8只加具体只读guard回执，沿用完全相同Native34／XR，后台／唯一Game／无遮挡均成立；21.005秒观察内941次commit／publish增量、43份有效post-FK，8条HMD命令均消费并读回。baseline及左右3cm／上下2cm各5份settled样本，Eye和Pelvis对应位移最大误差约1.56e-5／7.63e-6原单位；0.4／后台保护和正常106恢复保持。聚合accepted=true来自旧连续计数条件，完整运动接受仍false；独立审查`body-live-standing8/runtime-motion-review1.json` SHA256 `ef4c382a463c9115c73120919ae79bc52e3bce2c5fc9c35554bb76c7c238723c`。

B8旋转阶段380次spine_unreachable首样本可逐字重放，但已定位测试脚本单位错误：隔离runtime文件命令yaw／pitch直接进入sin/cos半角，单位rad；旧脚本将10°直接写为10rad，Eye Q差147.042°与该值规范化吻合。不能据此宣称正常10°旋转有缺陷。对同一首失败完整C0，仅恢复baseline Eye Q后plan通过，±10°／±30° yaw和pitch八个正确角度也全部通过；未改planner／marker／骨长／可达边界，证据`analysis/continuous-rotation-first-failure1/`。旧失败源与源龄保留；最后neutral缺少settle样本也不算回中接受。

当前唯一检查点：用同一Native34／XR重测显式radians的正常转头／抬头及回中，B9仅修轨迹角度、公差和末步settle，不重建Native／XR。当前等待武器DJ正常释放窗口，等待时间不折算进有效实现时间；准备与一次目标验证预计20–40分钟有效工作，非完整功能完工承诺。正常轨迹游戏读回与画面闭环后再接同一写入端的双手；原动画交还、完整Cover／Grip、跑步视角、碰撞／受击／射击及实体头显仍待完成，不凭view-held或测试数上调完整功能百分比。

B9已完成上述游戏骨骼读回检查点：仍为同一Native34／XR，22.338秒、1303次commit／publish增量，8条命令均按metres／radians消费并逐项读回；49份有效post-FK中41份settled样本逐份核对通过。左右±3cm、上下±2cm、原地yaw／pitch10°与neutral全部对应；最大Eye位置误差3.31e-5原单位、Pelvis7.63e-6原单位，转角约9.99998–10.00002°，固定feet最大漂移.000793原单位。纯旋转Eye／Pelvis不额外平移，最后回中已有settle样本。独立matrix／quaternion reader先与B8 C++ authored-marker oracle对齐后计算，未改源时间或混用eye与head bone。审查`body-live-standing9/runtime-motion-review1.json` SHA256 `57468bcbe3cc7c5a17705e053c8aa07f1c4eb47c1c3d20f1f56d6811a0611ca4`。

B9总1405次plan均solved／commit成功，1次额外admission skip；无观察期提交故障，age-recovery计数0，所以这局证明连续运动与跳帧后继续，并未真实触发新episode重开路径，后者仍仅有定向Host证据。最终body／Goal／Atom责任全0，106独立恢复／Native／entry／XR0／PG0、原十文件、音频10和精确自有PID及fresh CIM通过。该接受范围是固定standing的头／身体运动；双眼画面／实体头显、正常动画交还、双手写入和完整Cover均未接受。窗口已正常交还武器会话。

当前唯一后继交付转为把已有controller arm／wrist目标合入同一C0 Output／Kernel，保留Hand_R35→Weapon_R51→Weapon_R_End52传播，并验证正常动画接管和恢复；最小手部可消费版本尚未提供，有效工作时长未知。已验证的typed freshness、事务Failure／no-CAS分类和memory Backend边界先保存到本分支；tracked Backend继续保持原未接receipt的边界，实际Native34 live／receipt／camera组合仍在冻结私有候选，未整包覆盖Root最新default或声称运行时默认集成完成。

本分支实际tracked源码已由主实现者重新编译5个受影响Host目标并运行21个定向进程，全部通过；没有借私有候选的includes替代当前tracked头。回执`analysis/native-c0-transient-recovery-candidate1/tracked-host1/receipt.json` SHA256 `07912d32a64048c911d593479a2b3186fff1a651e3f97054b6841692fe34b4ff`。保留用户自行更新的AGENTS／工作方式配置，退役的overlay trial未纳入提交。

2026-10-26：双手进入同一C0 Output／Kernel的私有候选35，纯组合器与实际CAS／Registry Host接线检查通过，但B10实际控制器baseline后hands solved／commit为0，571次unreachable。右腕目标距肩57.171u，真实固定双骨长度46.995u；不能缩骨长或放宽拒绝来凑通过。完整Ref115证据显示Marcus模型实际+Y朝前、-X向右，旧作者Eye profile的+X前向偏90°。候选36保留旧profile，新增明确Basis=2、`marcus115-ref-eyes-face-forward-y-authored-v2`，同时修正Head→Eye旋转为inverseHead×EyeRef；骨局部轴与Grip→Wrist语义不变，不给武器再叠一次yaw。实际C0离线重放、64组独立刚体与相关36个Host进程通过，原段长及50ms守卫保持。36sim DLL SHA256 `ff40e1efd9567dd5c973a4aadefaa410aa319363b012cf4dcf7a9ea9771e6b78`，仍为原flags／42exports／23预检／实际22startup；未替换默认。

B11失败在连续Native激活前：Attract确认后真实菜单一直显示“Contacting Gears of War services / Establishing Uplink”，保留原60s超时和错误用于该阶段的finalizer false。另立pre-session-closeout，核实prime取消入口恢复、PG shutdown0／入口恢复、原十文件与音频10恢复、host及随后CIM无Game／XR；XR退出1如实保留，不凑106项body hook覆盖。B12只沿用相同Native36／XR，控制器baseline前伸由25cm改15cm，未改启动超时或后台保护；本次成功进入目标场景，11条controller命令按metres／radians消费，39个coherent异步Space样本、735次观察期joint commit增量，journal共755条正常原函数返回记录。末步neutral命令已到，但32s窗口内结束标记未完成，`sweep_finished=false`保留。

B12手部验收仍失败：38个样本与target journal存在唯一mesh／epoch匹配，24个settled样本最大腕位置误差32.4766u、方向161.954°。该journal只有target，异步probe时间戳在读取前，不能将“相同epoch”冒充“原Compose返回后”的时序证明，尚未定位误差产生阶段。限定源码及实际失败candidate复核确认arm Q确实进入同一Output、完整Local字节diff、Backend检查与CAS，没有发现mask／索引漏接；失败candidate不能代替成功提交读回。35→51→52的异步Space相对关系在样本间几乎不变，但还没有逐次匹配的原Local字节证明。独立报告`body-live-standing12/runtime-hands-review1.json` SHA256 `1ebb925f12ea3683d2dc2df1248f6bf1601909806983f2a04b0ae97fa76d24cd`。

新face的first-C0基准对照另见Eye约2.07u／5.60°差；该first-C0尚未证明就是neutral真实初始化帧，不据此接受或定位新Head／Eye基准。B9原候选的相对运动结果保留，不能继承为新profile朝向或新Head baseline通过。B12正常106项恢复／Native／entry／XR0／PG0、原十文件及音频12/12恢复；host结束瞬间no_remaining_games=false原值保留，最终准确自有PID检查及随后CIM为空，已向武器会话交还窗口。

当前唯一后继：在现有同一C0 hook增加有界只读记录，关联成功计划、consumer验证与Local、原Compose刚返回的Local／Space和前后QPC；只跑一个中立静止双手目标，先区分Local未保持与Local正确但Space不符，再决定下一处调查。没有增加第二writer，不开展完整轨迹或Cover矩阵，不把commit／cleanup计数当双手完成。实际手部、动画交还、完整Cover／Grip、跑步稳定、碰撞／受击／射击、双眼画面和实体头显仍待完成，有效完工时间未知。

Native37已完成上述被动观测，仅一旧.inl相对36变化，first32记录version2／2064B；原38个受影响Host回归和3个定向观测模式通过，reader能区分Local改动与Local正确但Space改动。纯Host的两次测试fixture断言错误已修正，原失败日志保留，未改生产守卫；窄审确认无新的authority／债务／越界／GT文件I/O或生命周期阻断。37sim SHA256 `01f8a2915526ed52b33fa6039607e3178c64f208f19bb69828b09969a71aba82`，同flags／42exports／23预检／22实际startup。生成器误保留的旧Body36文件标签及原构建回执保留，正确Body37 alias逐字节一致并做所选路径smoke，未重编相同内容。

B13复用原XR、只发一个中立baseline的两手命令；本局542次尝试中541次unreachable／0 solved／0 joint commit，version2 journal仅72B空header，所以没有成功原返回边界可分析，目标验收false。正常106恢复／Native／entry／XR0／PG0、原十文件、音频11/11、自有PID与fresh CIM空成立，窗口再次交还武器。下一步先用本局完整C0／Goal、规划后肩位及final base做完整当前planner／composer离线重放，明确两侧可达裕量与精确单一输入，再复用37取证；只看异步肩距或缩短轨迹不能替代这一步，不改骨长或固定标定来迎合测试。B12原偏差仍未定位，B9新基准与手部仍不加验收分。

B13跨快照Host重构已明确标注来源缺口：FPOSE C0与保留32份Core无publication精确配对，无法证明neutral初始化帧；以该C0 pelvis/root/feet为显式Host anchor seam、final Core的Eye/base重构当前view→body plan→hands compose，49检查通过且base bit-exact。选定单一点x左右±.18m／y-.30m／z0／yaw0／pitch-.3rad，左右最小内外裕量18.3985／18.7529u；原-.15m在该Host也通过，故不能声称已解释B13的541次unreachable。未放宽骨长／标定／源时间／50ms，未继续扩网格。

B14沿用同37／XR跑上述唯一点，61次新增joint commit、8coherent样本、结束标记完成。32条version2边界全部valid，20条位于最后命令ack至少250ms后；入口consumer前9骨Local逐字等planned，原Compose返回后32/32都只有六臂8/10/11/32/34/35不同，且不等原before。after Local与after Space链相符（最大2.89e-5u），腕仍偏26.1409u／161.604°；因此手部目标验收false，旧finalizer的liveness accepted=true不能代替它。Head在此单静点对本次C0 Goal误差2.22e-5u／9.54e-5°，51／52 Local逐次source=plan=consumer=after；不替代新neutral初始化或连续头身／手部验收。T只有浮点重算微差，scale仍1，不由字节变化推断拉伸。正常106／Native／entry／XR0／PG0、原十文件、音频12/12及准确PID／fresh CIM空通过，窗口释放给武器。

该证据把偏差缩小到原Compose调用内。按先KB后原文，复用已保存IDA分析：controller结果应用的完整LocalAtom store为RVA129419，前面已写Space，不能单独跳Local store。现有weight getter515bb0及其原返回packet提供候选接点，返回+0在非负阈值下跳1294b3，越过Local／Space和scale-only应用；只对同一成功joint／consumer scope的认证四arm角色考虑让位，其余原样，保留唯一C0 Local writer与原FK。旧CCTRL导出128／post-weight4容量耗尽后不更新资格，必须先将实时验证与导出容量解耦，不能拿历史row／计数／first32编号作权限。当前尚无行为修复或新默认集成。

Compose main／cold完整字节与B14恢复证据逐项匹配；编码RIP复核显示阈值实际RVA17070d8、历史raw4 acc52737（约1e-5），旧IDA文字17060d8低1000，后续策略仍必须读当前实际数据值，不把历史float当live验证。证据`B14-compose-encoded-identity1.json` SHA256 `335e9506916ca6dc0ca18898a4d83b3820e9338ee52270aaae578792678643a7`；边界报告`body-live-standing14/runtime-c0-boundary-review1.json` SHA256 `d072999d2d00ca02ee34015ca6c213c3b80a8d9260c12232b274a8859471ec4b`。本次MCP IDA私有打开未返回而取消，准确自有orphan已停止且registered lease空；另一次私有CLI60s超时保留，未改配置／安装工具／原数据库，改用现有IDA导出做机械字节复核，没有持续盲重试工具。

Native38私有候选接入现有getter返回packet的四arm角色仲裁：只有同一canonical continuous scope已完成joint commit及consumer验证、实读当前阈值raw4、空scale／flag0／原alpha1与严格相邻一次token均通过，才将XMM0低4字节置精确+0；原观察先运行，未添加Local／Space writer或hook，实时资格独立于128／4导出容量。窄审发现Native drain在retained验证中到达后仍可能grant，已保留红测并补grant前永久stop复查及CCTRL最终active gate；Native真实API Host 5模式、CCTRL 2603检查与旧OFF回归通过。38sim SHA256 `3bc9e9838d78c488aaa4e185bcd475805cd4c9bcf2d7a8dfa12d80e2eb278865`，原flags／42exports／23预检／22实际startup，v3记录2120B的grant尾部只表示Native资格，不能代替packet修改或实际pose证据。stop闭合回执`analysis/body-c0-arm-alpha-candidate1/stop-fix-closure1.json` SHA256 `2e87af5cb796d0ce0528177992125221aeaf7a6c9d41220782bbc8e64faaca99`；候选未集成默认。

B15在continuous激活前被实际配置拒绝：CCTRL开启但必要的CoverPosePhaseObserver仍关闭。没有发送controller／HMD命令；Native cleanup=false、entry restore=null、XR1原报告保留，另立pre-session-closeout核实prime／PG入口恢复、原十文件／音频恢复及准确PID退出，不伪称106。B16只修正私有phase配置与匹配guard，复用同38／XR及同一双手静点；实际643次body commit、97次joint commit，32条v3边界全部有效，consumer Local逐字等plan，但原返回32/32六臂再次变化，腕最大偏24.1537u／159.099°。Native alpha owner调用0、grant0、CCTRL修改0；实时权限路径尚未执行，双手验收仍false。正常106恢复／Native／entry／XR0／PG0、原十文件、音频10/10和准确PID退出通过，窗口已交还武器。独立手部结论`body-live-standing16/runtime-hands-review1.json` SHA256 `37689fdf1da7bcab84b1046feb4c2b6fb196186a9e97a0dcbb8839139362cff0`，旧finalizer accepted=true仍只表示liveness。

B16后已在源代码确认canonical producer缺口：hand compose_begin设置scope／continuous／admitted／tick却未设置Compose.address，而CCTRL continuous current已严格要求该精确地址；原Host mock直接读scope，漏掉实际接口条件。当前仅在独立Native39派生补producer及生产等义的红／绿Host，不放宽receiver、不借历史row、不更改50ms／事务／渲染或武器合同。修复对实际游戏的作用仍待同一静点复测；完整双手轨迹、动画交还、新neutral初始化、装备主握接线、Cover／Grip、跑步稳定、碰撞／受击／射击及实体头显均未据此接受。

Native39完成上述最小producer修复，源码仅一生产.inl及对应Host变化；旧producer严格consumer红测35检查／5失败，出现null地址、sticky／control9，与B16一致；补精确地址建立／匹配end清理后35／0、40tick 881／0及错end／nested／stop／revoke通过。39sim保持原flags／42exports／23预检／22实际startup，SHA256 `876bc3019440651914b73da0033681623093ce1684c0e11732863ca63ea45729`。B17复用原XR及同一HMD静止双手点，实际300次alpha修改、75次joint commit；32条有效原返回记录四角色均mask15／count4，consumer与after Local全部逐字等plan，51／52逐次保持。腕最大位置／方向误差.000174688u／.000092081°，Local→Space链7.04e-6u；最后命令ack至少250ms后的15条target完全相同，8份coherent异步Space观察最大.000234934u，固定单点接受。原no_remaining_games瞬时false保留，最终106独立恢复／Native／entry／XR0／PG0、原十文件、音频10/10、自有PID及fresh CIM空通过，已交窗武器DX。

B17手部独立报告`body-live-standing17/runtime-hands-review1.json` SHA256 `f4aa627bc9486ce2d3bebc4e2bc660492e07ec64dcaf914dd3cfeff1133030db`；这里只接受固定点，不接受连续双手、动画交还、新neutral初始化、画面／头显或完整Cover。下一步仅复用同39／XR做双手小幅三轴移动、yaw／pitch及回中，纯测试轨迹／读回变化不重建Native；之后验证原动画接管。装备主握Native合同继续冻结，默认冻结renderer／support构建未替换。

B18仅改私有测试轨迹／读回，Native39／XR未重建。18条metres／radians命令完成baseline、左右外移／内移、两手反向depth、上／下、相反10° yaw、相反10° pitch及neutral共9阶段；96s只为有界多阶段观察预算，50ms／启动清理／逐命令身份listener与ack保护不变。55份coherent Space、38份ack至少250ms后settled观察全阶段通过，最大腕位置.000232537u、方向.000156848°；1410次观察期joint commit。35→51→52相对关系最大变化.000054181u／.000069978°，没有额外90°或枪口补偿。初始32条原Compose返回记录仍为mask15、consumer／after Local逐字等plan；该有界journal只覆盖首段，不冒充每阶段返回fence。

主agent用冻结Core与真实模拟器float32姿态构造、生产rig transform／actual Marcus签名profile／wrist target计算各阶段期望，两腕对v3 target误差0；保存源时／身份不改，32个Core的base／reference／units／owner／head固定，55个实际CTW支持显式Host model-scale1，整局binding／reference／stream／head字段不变。实际轨迹接受来自按ack阶段settled异步coherent数据与此期望比对，不拿nearest target或receipt作FK权限。报告`body-live-standing18/runtime-hands-review1.json` SHA256 `972b3b1aeb239bf12d228fb66439f4fc99a0f6cef538e446d14c0d43d7a14fe3`；正常106／Native／entry／XR0／PG0、原十文件、音频10/10、准确PID及fresh CIM空，窗口交武器DY。下一唯一门是手部失去资格时原控制器接管及再次恢复；controller roll实机、新neutral初始化、画面／头显、完整Cover／Grip、跑步稳定、碰撞／受击／射击仍待完成，装备主握合同未因此解冻或接入。

B19继续复用39／XR，8命令完成reachable→两手x±2m明确不可达→reachable恢复→最后不可达，47coherent观察。两不可达阶段ack1.5s后各7份记录joint／alpha严格平台，unreachable8计数继续增长；恢复阶段7settled腕误差最大.000181941u／.000109789°，原控制器31份normal complete／mask15及最后1份latch5无效记录均保留。末段一份异步Space精确等原右Limb output，但距后继Single旋转9.656954°；两后继cohort与双方原Single均<.001u／.001°。这明确接受手部资格让位与重新跟踪，并保留中间计算阶段读数的时序限制；不能当作每帧post-return、菜单／过场、物理release或头显动画完整接受。32条首段joint原返回仍mask15／Local=plan，不冒充handoff的返回fence。

B19独立报告`body-live-standing19/runtime-hands-review1.json` SHA256 `764e1e3626bfa344c865bab13fd5fb46e0ae24be4cab87bf1cdf61d95238412f`；正常106／Native／entry／XR0／PG0、原十文件、音频10/10及准确PID／fresh CIM空，窗口已交武器EA／EB。连续双手与资格恢复门可推进独立装备主握Native查询；原b58524c PG-local发送端仅准备数据，不等于Native lifetime准入。下一步沿用同parent cookie／module pin／callback inflight／drain，先冻结最小附加合同，再由本会话接唯一C0消费者，武器会话只接PG入口；V1 Frame与原51／52增量保持。

canonical Scope producer的建立／匹配end清理已从实测39候选单独移入本分支正式源码，CMake注册address-contract回归；没有整包覆盖私有Native源或Root最新default。主agent使用正式tools源码重新编译实际owner Host并运行18个独立进程，全部通过，包含地址／40tick／错end／stop／revoke／typed age及身份硬拒绝。首次驱动误写不存在的typed-hard模式，原失败和全部输出保留；更正为实际typed-age-owner／typed-age-cohort后通过，没有因此改生产逻辑。回执`analysis/body-c0-arm-alpha-candidate2/tracked-scope-host2/receipt.json` SHA256 `20e8022f2e061f781f6119bcc3f9ec83530cd5effb1c605f689054f5a4603885`。完整Native39 body／receipt／alpha／camera组合仍为冻结私有候选，未声称Root默认集成或完整Cover完成。

主握附加数据合同现已冻结：`tools/vr-common/weapon_main_mount_provider.h` SHA256 `3f20163b1b65b7a74811958d4547b5c43605a6708d063a4049e4935deaa2f2cc`，现有Rig V1 Frame仍1824B，新Request1968／Response504／Registration48B，28项x64布局／形状检查通过。初版测试错误猜测Rig1840／Request1984，编译拒绝及机械sizeof探针保留；更正的是测试期望，header始终未变。仅一个attach export附在现Hand parent，复用module/context/cookie、callbacks与normal_calls、原drain；锁外普通callback并复核revision／operation epoch／revocation／committed Frame，排除active Call／Action／external／poison等及该C0入口Scene／stereo。查询不要求非空左手lease或authorized。详情`docs/6dof-held-main-mount-query-contract-20261026.md`。

Native40私有候选在同一body plan后查询，传真实Y2 Marker验证当前model／frame，只把通过同Scene／实际held Lancer数据校验的结果选为显式右手held_lancer profile；由Native再构造固定标定，Source／左手／51→52／唯一Kernel writer不变，callback后仍原40ms准入／50ms每写守卫。普通unheld／busy／stale／unsupported回stock；异常／篡改／malformed／reentry走原parent失效。大Wire／C0工作区持久存储，未迁移原有有界metadata局部；无新hook或新Hand transaction债务。

定向Host：Broker1934／Runtime新22及旧159、pure composer6042／adapter624、真实C0 ActualContext／Backend／CAS／Kernel／Goal／Marker的11模式1484检查全过，后者Runtime query为明确stub；源码与实际PG资格不混同。真实C0反例query内>40／50ms、Native stop／metadata／actor失效均0commit且不改Local／源时间；缺marker或实际X1 marker不查询。两次限定审阅分别收口合同与新实现，后者无新增实质P1／P2，结果在ignored `analysis/body-c0-main-mount-query-candidate1/`。

Native40 ON／OFF完整TU／DLL均完成原23预检／22实际startup，均43exports，只新增`GearsVrAttachWeaponMainMountProviderV1`；ON只比39多相关GEARS_WEAPON_MAIN_MOUNT_PROVIDER定义，原renderer flags不变，compile／INI默认OFF。ON SHA256 `f146c542d8780752d9d39d10299f4a3ec1b69aecb3bc044c7c5a5d53885de8ce`，OFF `ef078f2db94a96962290e2faa83d8dc133de5375dea45a055b026dfe998d42d1`。首次build驱动误传-DNAME给要求NAME=VALUE的build_verified，未创建DLL；原失败保留，仅修驱动、冻结source不变。

首轮联合选择明确覆盖基础manifest继承的旧通用PG：改用武器3715853候选PG `a4657e0d2a6fd3f643958dbdb157c9a88b95ccbdb337bed02b935a6e9b644472`；XR仍原BQ `1a059d2d…e9`不重建，Runtime必须用已验证支持实际right squeeze的CY `378e785fa284f281deab1bab01ba7b2d081901185afc508c9f04c67fd4114894`，不能沿用39测试的旧9abb。精确选择在`analysis/body-c0-main-mount-query-candidate1/joint-selection1.json`与联合manifest。当前只完成离线／构建，尚未启动联合Game；下一步仅实际Lancer取枪／同C0 held结果／小幅右手移转／恢复，R2始终OFF，0.4／behind／observe-only／mute／禁物理手柄后台输入及106恢复保持。武器会话准备其所属最小host／协调器胶水，本会话审查后独自运行。默认Support／renderer／公开版本均未替换，完整Cover与头显仍未接受。

联合启动前的独立核对又发现native-only manifest继承的XR仍为support1 df54，前述BQ描述当时未落实到两份联合JSON；这不是游戏失败。已在不改冻结native-only清单／不重建的前提下，将联合manifest及joint-selection1 revision2统一为B19实际BQ路径与完整1a059 hash，旧JSON均保留before-xr-fix1。纠正后联合manifest SHA256 `629c92feb352e95c298a6409bf18399ebb871f6cdbc921ec954d4c666f8656a1`，selection SHA256 `7c494a52f9755e2bbd5c93ee5966c08a6103b4755e261ab78e31ba2e6deb9b82`；`joint-xr-fix1.json`核实实际文件，PG／Native／CY Runtime身份均保持。Root准备端已收到同一新选择，最终host runtime-manifest／pins一致后才启动。

EC首次联合实跑已结束：Native40／PG3715853-a465／BQ XR／CY Runtime的实际RGrip取Lancer及独立right-bone attachment检查通过，但shoulder固定姿态的首个held-C0门超时。Native查询ready0、stale14、held commits0，v3 journal记录0；未宣称枪手跟随。查询最大15.2285ms与混合stale计数不能区分精确拒绝阶段，PG常规preview的body_unavailable也不能充当Native callback故障证据。HandStop4→5后cookie0、free-hand body继续／held提交不增长、两手原状态恢复和bridge close通过；正常106完整恢复、Native／entry／XR0／PG0、原十文件／音频／准确PID与fresh CIM空全部通过。host结束瞬间no_remaining_games=false原值保留，finalizer accepted=true仅表示cleanup。独立记录`analysis/body-c0-main-mount-diagnostics-candidate1/ec-first-failure1.json` SHA256 `bc3ddbbb89f4c5ba1857df181a57f305f0dc25ec7b4654f31803e81baa9f7b92`。

后继Native41只在精确冻结40源上加被动查询阶段与源龄诊断，未改原predicate、40ms准入／50ms逐写、wire合同／PG／writer或hook。775文件中仅Broker／full-body live与针对拒绝分类的测试变化；source-before／after、callback raw状态、projection／C0拒绝及源龄分别计，运行中独立atomic统计不冒充逐query一致trace，stop后末值才稳定。Broker1938与真实C0 Host11模式1484检查通过；41sim SHA256 `2d8de0209cf0099a180fe2f09efd324c8a228af67c2fbc88121fb06f23d02a69`，原flags／43exports／23预检／22startup通过。冻结在`analysis/body-c0-main-mount-diagnostics-candidate1/`；下一次EG复用EC动作和PG，只替换此Native，由身体会话唯一实跑。完整Cover／Grip、跑步稳定与头显接受仍待完成，Root默认未替换。

EG诊断复跑保持EC动作、PG／BQ XR／CY Runtime，只换Native41，实际取Lancer仍通过，首held门仍失败。停止后稳定统计将6次Native stale全部定位到broker callback之后的source校验：projection／source-before／C0-current／fault／context拒绝均0；reply分支4次，post-40ms admission拒绝9次，ready／held commit为0。末次source失败query10，mask1541对应Rig整体失鲜、committed-now、Scene及sync年龄；原龄依次46.0638／51.8660／52.1526／52.9531／52.9305／52.9508ms，没有重标时间。PG最后raw12=source_mismatch另记，不能和覆盖后的last_query0或独立atomic末值强行配成同次因果。PG静态返回映射进一步表明wire12来自sender context或response路径，retained绑定拒绝会被翻译为inactive1；具体PG阶段待定向记录。

EG HandStop4→5后cookie0、free-hand继续和全部输入／bridge恢复通过，最终body hands commits54；完整106／Native／entry／XR0／PG0、原十文件／音频／准确PID及fresh CIM空通过，host瞬时false保留。独立报告`analysis/body-c0-main-mount-diagnostics-candidate1/eg-after-callback-expiry1.json` SHA256 `63841cebccdca85d2581835aa382e1612ec01ae532a7052fa4415d4df561e1e3`。下一步由武器唯一修改PG，去掉同调用真实Game的重复初读并保留最终独立Game／双资产／附着／源复核，补被动POD阶段记录；Native41冻结复用，不放宽50／40ms，不扩大矩阵。离线held oracle已准备，但EC保留的末条Core记录缺有效asset／geometry，严格拒绝；journal0独立明确NO_HELD_COMMIT_EVIDENCE，没有挑记录或补造Core来凑target pass。

EH只替换武器80c4ee8／PG `bed19e2bf27611d84aecb206dd3e45f9fc48a9039cdf65b7ad4cc58e7b2b2a9c`，Native41及原动作／背景恢复保护均复用。新PG退出后输出16条完整query POD无截断：q1–3真实unheld；q4–11及13–16的context／retained／binding／agreement／asset均通过，在response最终源检查返回12；q12完整发布ready。Native独立总计ready1／held写入0、post-admission14与post-source stale12，不能把独立末计数冒充逐query trace。held样本callback入口sync年龄38.269–39.937ms，retained11.183–11.939ms；q12结束49.786ms，可过50ms但不足原40ms准入。前后两局未单独隔离一次observe的因果性能收益，不能用max差作基准。独立记录`analysis/body-c0-main-mount-diagnostics-candidate1/eh-retained-budget1.json` SHA256 `487346bae35939876084f314456bbd5c0537bd52d3583a11b392ad15b03b058f`。

EH完整106／Native／entry／XR0／PG0／原十文件／音频／精确PID与fresh CIM空均通过，仍无held跟随接受。Astra限定只读预算审查保存在`analysis/body-c0-main-mount-budget-review1/review.md`：publication→callback含C0前和调度，不能直接称Native计算热点；必要时先记录实际C0分段，不预先省掉actor／逐写current检查或缓存许可。武器又找到attached_frame前后隐藏的两次完整Game observe，a54d592只对retained路径提取原附件元数据core，用同次真实Game及末尾独立Game包住，双资产／附着原校验和其他调用旧包装均保留。EI将只换PG `0ec1cf1e85a69f00535c4c5e4eeb8c020e862f72c7e4999dfbc85351a0c88415` 复跑同动作；若仍held0再测Native各阶段，不放宽40／50ms，不另跑性能矩阵或扩大功能。统筹整体进度保持36%，未因测试数上调。

EI的PG查询已27次ready／stale0，实际held提交仍0；这局HandStop后free-hand continuation超时，不能宣称动作交还通过，完整106恢复等退出检查独立通过。随后EJ只增加Native42被动分段计时，24条query中17条ready仍held0。17条ready的Compose→query为0.926–1.154ms，PG查询6.346–6.869ms；进入Compose时原sync已36.506–39.139ms、原Scene已34.413–36.273ms。实际outer Tick→Compose仅0.319–0.384ms，不能把旧数据在采样／发布到消费之间的间隔称为30ms Native计算。EJ HandStop后的free-hand继续恢复通过，EI的失败仍单列；完整106／Native／entry／XR0／PG0、原十文件／音频及精确PID与fresh CIM空通过。原始预算文件SHA256 `a5656dacaf91b57a8d1615b8a4c818d0d01825d464ed9f3485bc8e92ceb092b7`，来源顺序核对在`analysis/body-c0-budget-timing-candidate1/order-review1.md`。

下一步已明确为真实outer-Tick来源与当前C0基准的独立输入，合同见`docs/6dof-body-tick-input-contract-20261026.md`：拟议新body输入在最终使用／每写前仍须40ms；原Scene作为设备／标定依赖保持50ms。该合同明确改变body消费者的数据来源，不能称原Scene40ms已通过，也不能用改时间或拼接CoreSnapshot实现。先验证原包可用性，再实施typed消费者。

Native43是此步骤的只读探针，基于实际冻结Native42的778文件私有源，仅6个接入文件和2个新增文件变化。真实GT的原outer-Tick owner／lease／latch成功后，每Tick一次，以已打开bridge读取原Stereo、精确匹配hands和continuity、原始时钟与稳定reference，Stereo只用try_lock；不打开／关闭映射、不更新StereoGate、不重新解码Grip、不改变旧Scene／40／50消费者／CAS。当前C0派生base只作为关联记录，basis_available不代表accepted或commit。新增头眼几何验证和parent／last时钟非回退检查由Astra限定增量审阅提出并闭合；predicted仅比较同源顺序，不转QPC年龄。

Native43 sim DLL SHA256 `1517cfb04235655c03e4345df9afc13fc903d4185286aace15d549d2c02dc426`，source43清单SHA256 `fe2c8a1c490356cb630c7bb109525e81cb15836ef469624337392f27f3a1f09b`。原renderer flags、43exports、22startup／23预检通过；实际probe Host43检查、原owner7模式1021检查、C0 Host11模式1484及budget15检查通过。退出后限drained导出的BTIPRB01为Header64／Record3816，最多32条；严格reader接受Host原包并拒绝截断、额外字节、错PID和错magic。早期Host编译／新增fixture错误、reader编译与fixture时间错误、build驱动参数格式错误均保留，未修改冻结生产源或用失败算通过。证据在ignored `analysis/body-tick-input-probe-candidate1/`，下一轮购买版0.4后台实跑只验证新原包与真实base可用性。默认Support3／菜单／颜色构建未替换；持枪连续body、完整掩体／Grip、跑步视角稳定、碰撞／受击／射击和实体头显仍未接受，整体36%不变。

EK实际来源验证完成：原日志SHA256 `69266cbe43418984477a5ab3208e474ce295234cbca664f58e8738d336a2d1b9`，绑定Game55712／Native2333248191503，32条均valid／newer／basis，序号比原Scene增加2–4。新hands sync原龄0.224–10.855ms，中位7.369ms；原sync为29.322–47.236ms，中位41.191ms；采样0.0251–0.0558ms。当前basis观察在采样后0.891–11.338ms，不能冒充精确capture时刻。旧消费者first-held仍失败，ready16／stale0／held0／post40拒绝17；HandStop4→5与quiescence、双手输入恢复和bridge关闭通过，freehand继续再次超时而非行为交还接受。最终106／Native／entry／XR0／PG0／原十文件／音频／精确PID与fresh CIM全部通过，host瞬时false保留。精简结果在`analysis/body-tick-input-probe-candidate1/ek-availability1.json`，不以probe抬36%。

后继Native44显式实现上述两源合同：新私有Input保留原Scene和真实Tick原始HMD／双手／continuity／reference／获取时间，旧Core／Rig／PG投影与公开wire不改；单次outer-Tick acquisition、不可变latch和原正常Tick end，无缺包回退／中途重采。纯goal及hands typed overload共享原math／solver，使用同一当前neutral／CTW基准。ActualContext在原每次validate／exchange处重验输入和实际capture开始≤40ms，原Scene依赖完整≤50ms；body／Parent／Scene硬失败优先于age-only，retained rollback／consumer／end不要求失效源重新fresh。held Parent仍按原同步PG查询关联query／FrameKey／hold generation／设备component和asset，逐写重核registration、lifecycle、action generation、barrier和债务。PG退出后cookie0允许新Tick freehand，不把设备身份当tracking权限。

Native44 sim DLL `1dafeb65f369e01b38b3b961f25bad5f3dfa27ff2930cd9fd9550b91f100e180`，最终冻结`source44-before-build2.json` SHA256 `3df45c10d5b6ecfba0ff69f91ef0dd8cfbb36c27553342e0b3e62918d1f92140`。782文件中10个既有文件及4个新增文件变化；完整TU／43exports／22startup／23预检通过，渲染flags保持，只显式开新body输入并编译关闭旧probe。Astra限定审阅确认后修复匿名namespace别名／ADL接入和probe编译OFF时的合法配置；第一次TU失败、旧清单和驱动均保留。新actual acquisition Live27，pure typed116、旧hands6042／goal159，旧owner1021／C0Host1484，以及新C0 Host7模式1046通过；后者实际执行CAS，覆盖旧Scene46ms但新tracking fresh的成功、过期／硬失效零CAS、原正常结束／零债务后的新Tick恢复和cookie0连续freehand。

Native44新增退出排空后只读`BTICMT01` Header64／Record4424，最多32条成功joint，与旧v3按sequence／tick／Compose关联；保留独立Input、当前base／CTW、Avatar asset、完整原PG Response与Parent回执。committed_ns仅为commit后观察，不能声称精确CAS时刻。严格reader用Host实际18CAS／正常FK／finally的非空日志验证：目标误差0、FK腕约0.0002u、头约1e-5u，Host alpha0如实保留且pose_pass=false；错PID／截断／query篡改均拒绝。EL正在保持原0.4／behind／observe-only／mute保护、PG0ec1／BQ XR／CY Runtime及原取枪／固定held／小移转腕／交还门槛实跑；尚不据Source／Host／构建宣称真实held跟随、完整掩体或头显接受。证据在ignored `analysis/body-tick-input-consumer-candidate1/`，Root默认未替换。

EL完成：真实新输入361次取得／361次正常Tick end，原fixed-held门仍失败，未进入后继移手。held联合提交已从0变为29，但严格新类型／v3逐条对齐只有1条完整pose_pass；29条consumer入口九骨全部逐字等planned，差异发生在后续after_local，右腕最高误差32.322u。owner mask／count分布为(0,0)4条、(1,1)7条、(3,2)10条、(7,3)7条、(15,4)1条，与相应arm／wrist被原动画改写的角色相符。owner grant在最终CCTRL后检查及XMM修改之前，不能等同actual applied。新tracking在postcommit观察时原龄12.890–16.735ms，旧tracking46.251–50.051ms；后者观察时超50不能当精确CAS超龄证据。HandStop4→5、quiescence、freehand续更、两controller恢复和bridge关闭通过；最终106／Native／entry／XR0／PG0／原十文件／音频／精确PID及fresh CIM通过，host瞬时false保留。精简证据`analysis/body-tick-input-consumer-candidate1/el-runtime1.json`，Root也独立重算了consumer／afterLocal／FK对应关系。当前仅接受新来源已经进入实际唯一writer，未接受held跟随，整体36%保持。

下一步Native45只增加匹配原前32 held提交的alpha sidecar，冻结在`analysis/body-held-alpha-diagnostics-candidate1/`；782文件中仅policy、CCTRL与full-body live三个文件变化，原sample／owns／两次current调用次数、短路顺序、alpha==1／Scale／40／50／Parent及原低四字节编辑均保留。日志分别记录既有sample Reject、Native首个deny stage、owner grant、最终post-owner拒绝或实际applied；raw_rdx完整原值与read_mask避免把未知role或未读零解释成真实数据。旧128条／4条observer容量不作新held覆盖证据。

Native45 sim DLL SHA256 `7d0d2d542d2c661a04f4b352a5c9ad791a3db049ea95b9c6b27273c3f9813be2`，source45清单SHA256 `c7fc7c4c4ed9001af10bc1e298b812f718e48a0dffcc70830db2115d4a0a7baa`，原flags／43exports／22startup／23预检通过。实际Native owner603、原CCTRL690、实际sample318及仲裁调用计数seam194检查通过；seam的sample／owner明确是stub，非实际游戏权限。Astra限定三文件审阅无新增P1/P2，包含stop期间只保留已绑定C0观察、三方完全排空后导出。新`ARMALP01` Header64／Record9256（每条最多64个144字节event，32条上限296256字节），绑定typed／v3同sequence／tick／Compose／thread；六个严格reader案例验证损坏／PID／thread／writer及空event解释。CCTRL阶段qpc是原return observation，不能重排或当阶段耗时；缺少／空／溢出事件不能自动算拒绝或接受。后继仅原动作一次定位，不因诊断构建或29提交宣称功能完成。

EM完成：Game53920／Native2410681145942，24条held日志与330个alpha事件逐条关联、无溢出，原alpha文件SHA256 `dc9f4e15411f070b51c83ac6180598f44a421b584dbeda39a0bffcc2acff3e62`。右臂Limb／Single各24次在sample之前因Compute identity6／后继prior_compute12而拒绝；左臂Limb16次、Single5次完成sample okay、owner grant及实际applied。Compute返回时旧sync原龄接近50ms，但仅凭返回观察不能断言精确失效时刻。严格reader修正零初始化Compute的packet-observed标签后保存em-alpha2，原em-alpha1保留。firstheld40／6s仍失败；HandStop4→5与quiescence、输入／bridge恢复通过，freehand续更本轮超时，独立保留。正常106／Native／entry／XR0／PG0、原十文件／音频及精确PID／fresh CIM空通过，未接受held跟随。

冻结45的实际owner／Rig Host32项确认因果：只推进旧source超过50ms，首次age-only烧毁原latch并设置sticky，同一精确Scope的CCTRL current随之失败，actor／reference／metadata／原Core字节不变。Native46据此增加同次已提交消费的显式typed认证及独立永久hard-control-fault，不更新源、不复活latch、不重notify或授予新写入。Kernel只读getter核实真实backend／lease／stamp／thread、joint已commit／notify、consumer verified及未finish／stop／protocol；同次消费仍独立核实actor、metadata、model、Core／reference／source ticket和Parent生命周期。写入前新tracking40ms／旧Scene50ms、原alpha1／Scale0及唯一writer保持。expiry只停止旧source重读和旧假设plan，原Compute before／after与reject(control)继续执行，避免硬节点／堆栈故障恢复后重新授权；Astra发现的这一P2已修正并静态闭合。

Native46最终私有DLL为`build-cover-body46sim2/GearsCaptureCoverBody46sim.dll`，SHA256 `65f55c25b111bea5c7e818dfb3bdaf0acd8a7bb2ef243abdd1440450c67a951b`；最终源清单`source46-before-build2.json` SHA256 `459c767117ef68fb93e2111ab23ead3140b08b994542ca5724d60e4381c94709`。七文件delta含一个Host seam，原flags／43exports／22startup／23预检通过，首轮修正前build及源清单保留。原getter正常／stop／真实SEH781项通过；实际ActualContext／Backend／Kernel wrapper805项验证旧source过期仍可认证、metadata故障后永久拒绝且零新增CAS。另一个实际owner／Rig＋真实Kernel／bodyhands的279项窄组保留burned latch并验证hard优先、nested unwind、stop、wrong scope、before／after故障恢复及closed lease均拒绝；其中Context／typedInput／节点读取为明确Host seams，没有冒充完整sample→applied或fresh覆盖退休。原CCTRL690／sample318检查通过。证据位于ignored `analysis/body-retained-alpha-consumer-candidate1/`。EN仅将原EM的Native替换为此最终候选，PG0ec1／BQ XR／CY、原八命令／firstheld40／6s／106恢复与0.4后台保护保持，正在由身体主会话独自运行；未替换Root默认构建，整体36%不变。

EN已完成并正常清理：Game20148／Native2447539276000，28条held中27条完整pose_pass，目标误差全部0，27条四角色均mask15／count4、consumer与after Local逐字等planned，独立alpha sidecar的实际Applied四角色各27次；552事件、0溢出。sequence5／query30保留四角色no-token、pose失败，不能当作stop或删除；其postcommit旧sync47.763ms也不能定位后续硬拒绝原因。原40held／6s门仍失败，后继移手未执行，MainMount ready43／held28／stale_broker63。该轮最多32条预算中10条ready query耗时6.358–8.160ms，C0→query0.993–1.284ms，commit1.334–3.582ms；这不是FPS比较或总CPU成本。当前仅接受有覆盖的27次动画消费修正，连续held跟随仍未接受。HandStop5／quiescent、freehand继续、controller恢复和bridge关闭通过；106／Native／entry／XR0／PG0／十文件／audio10／准确PID及fresh CIM空通过，host瞬时no_remaining=false保留。证据`en-alpha1.json`、`en-pose1.json`、`en-budget1.json`、`en-runtime1.json`；PG旧源拒绝由武器会话据原日志限定分析，不改门槛或重跑未变候选。完整Cover／Grip、跑步稳定、碰撞／受击／射击和头显接受未据此完成，整体36%保持。

Host证据关联补记：263项初组的入口SHA256 `07a59d1f…947ee9`后来被279项最终测试入口`e3880443…62c0f`同路径替换，未单独保存旧入口逐字副本。两套原回执／compiler deps／obj／exe／用例输出均保留；不能拿现入口重建原263源码主张。最终279加入实际CAS及before／after计数、Local／CAS不变断言和awaiting-fresh／protocol输出，未改生产源码。准确关联与限制见`retained-owner-entry-provenance.json`。

EO仅将PG替换为武器73076dd的query-local纯解析候选`f85a00dda92f09c0c7548151b676c7cb37dc5ec0879472201855ad0248615650`，Native46／BQ／CY及原动作／40held-6s／106保护保持。实际Game1500／Native2480512716131，19条held全部pose_pass、mask15／count4、四角色实际Applied各19、380事件0溢出；目标误差0，最大腕位置误差左0.000204816u／右0.000225270u。原连续门仍失败，未移手；PG ready41／held19，Native22次age-only提交失败、65次broker stale。Stop5／quiescent、freehand继续、controller恢复／bridge关闭以及106／Native／entry／XR0／PG0／十文件／audio10／准确PID及fresh CIM空全部通过，host瞬时false保留。原finalizer仅执行一次；结果在O46的eo-alpha1／eo-pose1／eo-budget1／eo-runtime1，不据finalizer accepted或19姿态上调36%。

PG真实前缀ready查询中位由EN6.5387降至EO3.7711ms，但Scene→publication仍约6.27ms、publication→C0约30ms，EO sync比Scene早中位8.1922ms而EN为1.4223ms；两局不是同输入配对FPS基准。Root独立PG前缀也确认仍有16条最终response12，不能用这些前缀分类全部65 stale。纯解析候选及EO保持冻结，不再做同类微优化。Astra实际依赖审阅确认旧tracking还参与Broker before／after、PG Client／bind／world_grip重算／右Grip资格和Native逐写，不能把固定标定输出当作“metadata无tracking”而删除guard；当前没有证据支持更小的引擎生产顺序修正。

后继改为明确新增真实Tick／C0主握来源，合同见`docs/6dof-body-main-mount-source-contract-20261026.md`。原V1完整保留；新合同分原Scene／设备历史关联、新raw40、当前capture40、Scene observation50、query当次equipment witness50，Native固定私有C0验证在Broker锁外执行，不能让POD自报Tick或已预留的自身callback计数冒权限。新共享头`tools/vr-common/body_main_mount_provider.h` SHA256 `6ad0a08fb3633bb0cc3d39841634a69c2a65771ed29886e7c32ceeb9c03b4b3d`，独立AttachBodyMainMountProviderV1；实际x64 Tracking1792／Basis256／Source3888／Request3944／SourceKey160／Response712／Reg48、align8，旧Frame1824／Req1968／Resp504／Reg48不变，原50项布局／关联检查通过，Astra单次schema审阅无P1／P2。PG只echoNative publication／binding／ticket／actionGeneration，不假称独立观察；Client epoch本地Gate取，NativeRef snapshot revision与StereoGate reference generation不得混同。

Native47从精确冻结46私有源派生，新纯类型`gears_body_tick_source::Input`及goal／hand／composer独立overload已完成：原44整包age保留，新类型使用原Scene QPC／publication50与新tracking／acquisition40，旧raw只作未改字节的历史／坐标硬关联。旧raw51／Scene45／新raw约1ms时旧类型拒绝，新类型可计算；全部fresh的新旧目标／最终Local一致，Scene51／raw41／未来时钟／硬身份等仍拒绝且输入输出不变。新类型190项及受影响旧hands6042／goal159通过，独立SourceTarget不冒充旧TickTarget；此阶段无Live／Kernel／PG／Game权限接受。主代理接实际源／Broker／Native，武器会话接专用query-bound设备观察，尚未构建或运行47。等待期间仅只读核对既有跑步实现：旧候选仅FP/SM4，新body SM0视点优先，R01–R04仍需共同基准／衔接／最终eyes验证，没有改当前实验或宣称防晃完成。

Native47实际接线随后完成于ignored `analysis/body-tick-mount-source-candidate1/source`（精确46父源782文件，现787文件；14改／5增）。独立Rig metadata latch／Scene观察策略保留旧Core原始字节与原V1失败；实际outer Tick单次采集、goal／hands新类型、每写Native context、同parent新Broker query、固定私有C0 validator、独立新Attach及BMSCOM01 drained journal均已接入。实际Rig 309、实际SourceLive最终85、Owner／Rig routing227、新Broker467及旧V1 targeted590检查全部通过；seams分别记录，不将组合计数冒充Game权限。整合Host运行实际ActualContext／capture／select／fixedvalidator／Goal／hands／CAS128 Backend／Kernel／original-finally／retirement，8例最终969检查通过：旧raw51／Scene45／新raw1时held和cookie0 freehand各commit1／CAS18；metadata硬变化、查询ready或notready回程parent撤销及accept后写前撤销均commit0／CAS0。

本轮Host发现并修复两个实际实现边界：任何query回程都保留真实pre-query parent依赖，不能因registration失效降级freehand后继续写；exact consumed Kernel的retained只验硬连续性／idle parent，equipment49→51ms不再错误烧hard，但新写仍拒绝并明确分类source_age_expired。未知／未来／倒序equipment时间仍为hard，原BEGIN50写入限制保留。修前失败、复现及修后回执均保留，最终FullBody SHA256 `f349d9ad70372b0545abb3142ceb72304996cb53d74d83ddc8fe931a0b0a4b24`。完整capture TU使用原46生产flags加一个新flag进行/Zs编译，原基线与候选各29条相同warning、无新增；窄Host为/W4 /WX。尚无47 DLL／Game／XR／物理头显验收，也未替换默认构建，风险增量审查及候选构建是下一步。连续held、完整掩体和跑步防晃未通过，整体进度维持36%。

后续单次Astra增量审查发现并闭合一个P1：若Tick采集的非零parent在选择／查询之前已经消失或替换，早退freehand仍可能写入。实际Source.observe现在每次独立认证immutable acquired cookie与Native incarnation；不查idle计数，正常reserved callback不自拒绝，cookie0初始空手保留。实际SourceLive最终89项、Native整合9例1000项通过；preselection revoke为query0／commit0／CAS0，cookie0仍commit1／CAS18。审查报告保留发现与闭合，范围内无未关P1／P2，不冒充Game证据。

Native47最终候选已构建：`tools/native-capture/build-cover-body47sim3/GearsCaptureCoverBody47sim.dll` SHA256 `78705ddd823a79a88aed943d559decb3bc873ce32eaf6d05ae9bba7c3d731348`；冻结清单`analysis/body-tick-mount-source-candidate1/source47-before-build3.json` SHA256 `b14c684b61cc8356bb971df623f102be5f0a571a22fe82d9774de69d4923a4af`，实际私有source2保留787文件／14改5增。44 exports只新增AttachBodyMainMountProviderV1，原46 renderer flags仅加一个BodyMainMountSource define；实际22startup／23preflight通过。第一次构建被非UTF8 compiler-prefix探针拒绝；第二次真实TU发现新增idle函数非限定调用的ADL歧义；旧冻结source及失败build均保留，source2只限定该函数名、语义未改，实际Source89复跑通过。尚无47 Game／XR／headset／默认集成；已向武器f611000c发送精确候选，准备同一原动作40held/6s测试，未放宽连续门或106保护。

EP原动作在正版0.4后台完成，Game55200／Native2559925659979，测试前另一个Game自然退出后才启动，完成后fresh CIM Game/XR空并把独占窗口交还帧率会话。原40次／6秒固定握持门通过；后继`right-ready`和右手小移并转10°分别新增8／10次主握提交，各有连贯post-FK观察。BMSCOM01前32条独立严格解析：32/32完整来源／ready回声关联、32/32新同步追踪postcommit 7.3631–18.0295ms，旧V1追踪43.4009–58.6423ms；32/32双手pose_pass、目标误差0、最大腕FK误差左0.0001563／右0.0002552游戏单位；ARMALP01实际四角色Applied逐行32/32、640事件0丢失。Native全会话main-mount ready75／commits62，原始age拒绝105／hard拒绝2保留；前32日志容量不能覆盖全部提交。右手输入改变后右腕与武器骨的异步世界位置亦改变，尚非同帧画面或权威射击证明。HandStop5／quiescent、自由双手继续、控制器／bridge恢复；finalizer仅一次，106独立恢复、Native／entry、XR0、PG0、原十文件、音频10/10及准确自有PID清理通过。完整证据和范围见`docs/6dof-body-native47-ep-runtime-20261026.md`。装备主握共同前提已过；LGrip抓边、实际掩体身体／碰撞／受击／枪口、权威开火、跑步R01–R04、双眼和实体头显仍待实现与验收。默认Support3／菜单／颜色构建及公开发布均未替换；按根工作区`docs/6dof-progress-dashboard.md`的固定11节点口径，整体仍36%、武器⑤仍40%，本轮前提突破尚不足以提升相应节点至下一档。

掩体抓边的碰撞证据新增单次诊断入口`tools/game-playtest/cover_trace_query.py`。它只在已由本会话持有、已加载Playground且处于当前有效CoverLink的Game中，核对当前Actor.Trace反射、GNatives277和完整历史代码片段后，限时提交一次靠近Pawn的有界Trace；未知排队状态不重试，未确认完成的私有请求保留到原进程退出。保存样本上的参数布局与8项拒绝检查、Python编译／导入通过，记录在ignored `analysis/cover-trace-diagnostic-candidate1/offline-contract1.json`。目前没有调用实际Game；Trace单射线结果也不等于边缘碰撞体或LGrip合法性，不据此启用抓边或调整36%。运行窗口仍由帧率会话持有，待其结束后才做0.4后台命中验证。

随后的限定审阅发现参数证书缺少原始`UField.Next`顺序，而Playground的`EX_NativeParm`实际按此顺序发出。现已在Trace调用前要求原始链与九个反射参数顺序完全一致，离线故意交换前两项可稳定拒绝，见ignored `analysis/cover-trace-diagnostic-candidate1/offline-order2.json`。此修正没有引擎执行或碰撞结论；之前记录的运行窗口已释放，正在准备新的私有0.4后台掩体场景。

CT1私有0.4后台正版局实际到达`CoverType=2`／`SpecialMove=0`，cover与geometry只读采样均accepted；slot0允许左右及上探，候选slot前向约`(-0.9962,-0.0869,0)`，并未验证它是碰撞墙面。外部采样最初两次因工作树缺省`pefile`依赖导入失败，随后使用现有主工作区依赖路径读到真实掩体数据；五分钟会话上限先进入stopping，`Actor.Trace`调用数仍为0。停止命令与自动停止交错且无command-11回执，保留为控制流程限制。最终session stopped/passed、Native／入口恢复、XR exit0、Playground shutdown0／entry恢复、原十文件一致，fresh CIM Game/XR空；host较早的`no_remaining_games=false`瞬时快照不冒充最终残留。原回执及精简结论在ignored `analysis/cover-trace-diagnostic-candidate1/ct1-runtime-summary.json`。这没有提升碰撞、抓边或总体36%。

为下一次短测，CT2在ignored `analysis/cover-trace-diagnostic-candidate1/ct2-prepared.json`固定了fresh私有脚本、精确Native／XR／PG及当前只读源哈希；`run-ct2.py`把原进入掩体、一次world-only有界射线及正常停机串在同一五分钟预算内，先验离线语法／无执行入口检查通过。`bTraceActors`现在必须是显式布尔值，两种参数打包均通过离线核对。尚未启动CT2，射线即使命中也只能证明所测一段的实际返回，不直接授权LGrip或身体穿越。当前游戏窗口已交帧率会话做其一次无Game构建，待其释放后运行CT2。

CT2在自检窗口交还后实际运行：正版后台Game24044／XR62568、0.4，原输入命令进入掩体，读到`CoverType=2`、`SpecialMove=0`、slot0，独立geometry accepted。此时`cover_state.capture`的完整非原子采样accepted=false；CT2脚本按自己的双证明门停在射线提交之前，`Trace`调用数0，原拒绝文本因脚本只保存摘要而缺失。没有把geometry单独通过当作碰撞许可或暗中放宽该门。正常stop命令11有成功回执，host exit0，session stopped/passed、Native／entry恢复、XR exit0、Playground shutdown0／entry恢复、原十文件一致、fresh CIM Game/XR空；证据在ignored `analysis/cover-trace-diagnostic-candidate1/ct2-runtime-summary.json`。CT3已在ignored `ct3-prepared.json`和`ct3-runner-final.json`固定fresh私有脚本及同一二进制，只给完整的只读cover／geometry采样最多3次有限重取，每次保留拒绝文本；仍须CT2/SM0且两项均accepted后才提交唯一Trace。CT3尚未启动，帧率会话正在独占随后的一次三段实测，整体36%不变。

CT3在帧率会话明确交还窗口后实际运行：正版后台Game62176／XR45436、0.4，一次只读采样即同时取得`CoverType=2`／`SpecialMove=0`及cover／geometry accepted，构造一段靠近Pawn、`bTraceActors=false`的world-only候选射线。当前`Actor.Trace`函数头与先前实际保存的反射样本不一致，严格证书在提交前拒绝，`Trace`调用数仍为0；实际差异字段未被本轮旧诊断记录，不能猜测或绕过。原始回执为ignored `analysis/cover-trace-diagnostic-candidate1/ct3-runtime.json`。正常stop命令8成功，host exit0，session stopped/passed、Native／入口恢复、XR exit0、Playground shutdown0／入口恢复、原十文件与设置存档不变，fresh CIM Game/XR空。已按联合排程把下一测试窗口交给武器EQ，不连续重开CT4。

离线入口已作最小诊断修正：下一次函数头拒绝会同时记录当前与预期的NativeIndex、参数大小、返回偏移、函数标志和入口RVA；不会放宽任何字段、调用引擎或授权抓边。保存的实际Trace样本通过，五种逐字段错误均以包含当前值的拒绝信息退出，Python编译通过。后续需在武器EQ清理后按新证据决定是否再做一局碰撞查询；单射线命中仍不足以证明左／右／上边缘、手部可达与连续身体碰撞，整体36%不变。

武器EQ交还窗口后，CT4只读私有0.4正版局实际读取Game63516当前`Actor.Trace`：NativeIndex277、参数大小116、返回偏移108、入口RVA`0xcdacc`，函数标志`0x424401`；九个字段的偏移、类型标志及布尔mask仍与旧样本相符。完整surface查询在关闭时因`World-state metadata/registry changed`拒绝，未把`trace_code_verified=false`或单次头部数据冒充完整证书；引擎调用数0。stop7、host exit0、session stopped/passed、Native／entry恢复、XR0、Playground shutdown／entry恢复、原十文件与设置存档不变，fresh CIM Game/XR空，随后将窗口交帧率会话。原回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct4-runtime.json`。

旧值差异已由保存的**实际原始函数头**定位，而非猜测：`body7-sample-12.json`头部偏移`0xf4`原本就是`0x424401`，偏移`0xfc`才是旧报告的`0x5c1a`。先前`cover_surface_query.py`在`e8930ef`已把读取偏移从`0xfc`修到`0xf4`，而新诊断调用证书误沿用旧报告值。现严格要求偏移`0xf4`和原始字节值`0x424401`，其余NativeIndex、布局、顺序、GNatives槽和代码片段证书均保留。旧原始样本与CT4现值一致，五种故意错误仍拒绝，surface原13项测试／Python编译通过；审计在ignored `ct4-header-offset-audit.json`。两个只读查询器也保留具体`q.closing_rejection`，供下一次元数据关闭拒绝定位，不放宽原子性门。CT5已从原CT3单次调用私有脚本派生，13脚本／19共享源哈希和语法复核通过；启动前fresh CIM看到用户从Explorer启动的另一份零售游戏，因此**未启动CT5**，不抢占或操作该游戏。碰撞、抓边与整体36%仍未提升。

桌面启动的游戏自然退出且fresh CIM空后，CT5独立0.4后台正版局实际到达同一`CoverType=2`／SM0槽位；首个geometry关闭样本拒绝后，第二个完整cover／geometry采样通过。修正后的`Actor.Trace`头部、原始参数链、GNatives277、完整代码片段及调用前后对象／会话证书通过，第一次实际引擎Trace完成、`accepted=true`，当前Pawn／Link／slot与调用前一致。Pawn附近world-only水平射线高度`15.887`，返回actor0及全零hit location／normal；根据同槽位候选slot Z与MidHeight70，它约高于候选顶沿4.844游戏单位，这只是未命中的一种解释，不能证明物理顶沿。stop11、host exit0、session stopped/passed、Native／entry恢复、XR0、PG shutdown／entry恢复、原十文件与设置存档不变、fresh CIM Game/XR空；证据在ignored `ct5-runtime.json`与`ct5-ray-analysis.json`。此轮验证了受控单次调用链，不证明墙面、碰撞体积或LGrip合法性。

CT6沿当前采样的slot位置与`MidHeight=70`构造候选顶沿下25单位、长约200单位的同槽位水平射线。world-only第一次完整返回且`accepted=true`，仍为actor0／全零位置法线；仅在该完整空返回后，第二次同射线启用`bTraceActors=true`，观察到队列完成和原始空返回，但调用后注册表count从228192增至228193，严格关闭证书拒绝，保留`accepted=false`及具体`closing_rejection`，没有重试或把原始输出当作接受命中。两次调用分别约565／520ms、读取8255次；是否命中墙面仍未知，下一步先建立地面短竖直射线的正向命中对照，再判断Trace flags／几何选择。原版CoverType2／SM0及cover／geometry采样通过，stop10、host exit0、session stopped/passed、Native／entry恢复、XR0、PG shutdown／entry恢复、原十文件与设置存档不变、fresh CIM Game/XR空；原回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct6-runtime.json`。没有启用抓边或改变整体36%。

正向对照CT7现已准备，尚未启动：同一当前Pawn XY从Z+40竖直到Z−240，线段280游戏单位，先world-only／`ExtraTraceFlags=8`；仅在第一次完整且被接受的空返回后，再以`bTraceActors=true`／flags0作第二次对照。保存的历史`execTrace`局部控制流显示world-only分支在此flags位上不区分0／8，actors分支才改变掩码；它是静态线索，不能替代当前游戏命中。`cover_trace_query`只增加显式0或8两个诊断值，其他数值与布尔伪装均拒绝；四种合法打包、八种非法值、Python编译及原13项surface检查通过。CT7仍要求原版掩体双证明、同一会话前后证书、每次未知状态不重试和正常清理，未改变抓边授权。帧率会话正在独占下一次低画质基线窗口，CT7须待其交还。

CT7在帧率会话交还窗口后完成正版0.4后台实跑。Game63580／XR21844到达`CoverType=2`／SM0，双掩体证明及Trace前后证书通过。一次world-only／`ExtraTraceFlags=8`的竖直280单位射线从`(-387.6745,-3114.8633,0.9113)`到同XY的`Z=-279.0887`，返回`SP_HOS_S00_SMC`，命中位置`(-387.6745,-3114.8633,-127.0000)`、法线`(0,0,1)`；返回点位于射线段内，且与起终点XY一致。这是当前ABI与场景地面碰撞的正向对照。第一次已命中，预先限定的actors-enabled第二次未执行；不能从它推断掩体墙、左／右／上边缘或手部可达。`Trace` accepted、调用数1、原角色／Link／slot一致；正常stop11、host exit0、session stopped/passed、Native／入口恢复、XR exit0、PG shutdown／入口恢复、原十文件与设置存档不变，fresh CIM Game/XR空。原回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct7-runtime.json`。窗口已交帧率会话；下一步离线限定真实掩体边缘候选与碰撞射线，再进行单局受控验证。LGrip未启用，总体36%不变。

离线对照同一固定槽位：CT7地面命中Z为−127.000，slot候选Z为−58.957，相差约68.043单位；CT5／CT6水平空射线分别高于该地面约142.887／113.043单位。先前用`slot.z + MidHeight`猜测顶沿的解释不能成立为已证实几何；两条空射线可能都在真实矮墙上方。下一次CT8限定为同局Pawn竖直地面复核，再在当局地面上35单位、沿slot候选前向穿过Pawn附近作一条world-only水平射线。只有命中点在线段内部、法线接近水平且朝向来射方向、Actor／组件排除角色自身并保持同一CoverLink／slot时，才记为候选墙面阳性。即使命中也仍不能授予LGrip或边缘/身体碰撞资格。CT6第二条实际`bTraceActors=true`，但关闭注册表变动而`accepted=false`；它既非通过的actor对照，也不可改记成world-only。CT8尚未启动，帧率会话持有游戏窗口。

CT8在帧率会话交还后由fresh私有0.4正版实例完成两条预定射线。Game17984／XR64840进入`CoverType=2`／SM0，同一CoverLink／slot和前后Trace证书通过。第一条Pawn XY竖直向下world-only命中场景地面`SP_HOS_S00_SMC`，Z−127、法线`(0,0,1)`，与CT7独立局一致。第二条在当局地面以上35单位即Z−92、沿slot候选前向从`(-314.0550,-3108.2214,-92)`至`(-513.2977,-3125.6101,-92)`，world-only／flags8命中`StaticMeshActor`，位置`(-427.7049,-3118.1401,-92)`、法线约`(0.99621,0.08694,0.00004)`；线段参数0.5704、线误差0.000038，法线与来射方向相反。两次`accepted=true`、调用数2，候选槽位不变，墙面Actor不同于Pawn／Controller／地面Actor。这是该槽位附近的**低位实体挡面候选阳性**，同时支持CT5／CT6测得过高的解释；尚不能证明该挡面与CoverLink的可抓边界一一对应。没有扫描左／右／上边缘，没有认证手部可达、身体体积扫掠、头部遮挡、受击或枪口一致性。正常stop8、host exit0、session stopped/passed、Native／入口恢复、XR0、PG shutdown／入口恢复、原十文件与设置存档不变，fresh CIM Game/XR空。原回执与私有脚本保留于ignored `analysis/cover-trace-diagnostic-candidate1/ct8-runtime.json`和`run-ct8.py`，运行窗口已交武器ER。下一步离线限定单一边缘的实际几何搜索和抓取/扫掠许可；LGrip仍关闭，总体36%不变。

武器ER释放窗口后，CT9尝试用当局地面、低位墙面再到地面以上60／80单位的最多四条射线限定上沿。第一条地面Trace接受且再次命中Z约−127；第二条墙面Trace在**引擎提交前**因当前UObject注册表count从228194变为228202而被严格关闭门拒绝，`trace_calls=0`、无pending，不是墙面空返回。按门禁未发出60／80单位射线，也没有得出上沿证据。stop9、host0、session passed、Native／entry／PG／XR恢复、原十文件及设置存档不变、fresh CIM Game/XR空。回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct9-runtime.json`。下一次fresh CT10只对同类未提交、无pending的只读注册表变动允许一次短重取，并重新校验同一CoverLink／slot；任何已提交未知结果都不重试。LGrip保持关闭，总体36%不变。

CT10 fresh实例首次墙面查询再次在提交前因对象注册表仅count增加被拒；脚本核对原Pawn／Link／slot后允许唯一一次无提交重取。第二次确实queue2／`trace_calls=1`，返回与CT8相同的`StaticMeshActor`低位点`(-427.7049,-3118.1401,-92.0000)`及近水平法线，但关闭时对象count又从228201增至228202，原严格门判`accepted=false`。第一条地面仍accepted，合计两次引擎调用；60／80单位高位射线没有发出。原始CT10保持拒绝，不事后改判。stop10、host0、session passed、Native／entry／PG／XR恢复、原十文件和设置存档不变、fresh CIM空；回执在ignored `ct10-runtime.json`。随后窗口交武器ES，掩体转离线门禁审查。

离线审查显示CT9／CT10的对象数组data与capacity未动、仅count单调增加；原`CoverQuery.finish`对全局count严格相等，而仓库已有其他只读查询的append-only策略。Trace专用`TraceQuery`初版只允许对象表尾部增长，名称表头要求不变，并对**本次实际使用的每个旧对象**另做原索引槽、名称入口与文本、原始身份字段见证；原有identity／metadata／Cover值、模块／函数／GNatives与代码、Pawn／Link／slot、请求输入及生命周期证书全部保留。任何旧依赖改变、所用对象落在新增尾部、count回退、data／capacity变化继续拒绝。增量审阅指出初稿先用父类宽松finish读表头、再重读可漏掉已观察的短暂增长／回退；随后的修正统一在一个Trace专用高水位检查中读两表，并以两种读序回归复现后修复。52项聚焦离线测试通过。普通`CoverQuery`未改变；不能把CT10改判、授权LGrip或调整36%。

首次CT11真实运行暴露该候选的实际读取预算问题：为1811个被遍历对象逐项增加槽／名称见证，使第一条地面Trace在引擎提交前达到`16385`读、超过原`16384`门；`trace_calls=0`，无地面/墙面/上沿新结论。正常stop／host0／Native／PG／entry／XR恢复、原十文件及设置存档不变、fresh CIM空；原拒绝和巨大见证集合留在ignored `ct11-runtime.json`。没有增加预算或放宽三秒限制。后继源码只限定使用现有`anchor_config.LiveRegistryAnchorSampler`：其`fresh()`每次已核对旧对象槽、实时名称入口／索引／文本及原始身份，Trace查询开始与关闭时对**所有1811个实际使用对象**保留原Query身份重访；删除重复的逐对象`q.read`，仍要求对象/名称索引在原Resolver范围、名称表头不变、对象表data/cap固定且count单调不减、原metadata与Cover值一致。假采样器类型被拒绝，选定对象数量改为精简回执，不再输出整个见证表。52项Trace/World/Cover聚焦测试通过；`anchor_config`33项在使用仓库现有`pefile`依赖路径后通过，最初裸环境一项导入失败单独保留。此优化尚未实跑，旧CT11不改判，LGrip关闭，总体36%。

CT12使用精确冻结的后继Trace reader在正版0.4后台Game27908／XR65688完成原定四条world-only／flags8射线，均`accepted=true`、`trace_calls=1`、同一CoverType2／SM0与Link／slot证书通过。当前Pawn XY地面再次命中Z−127.000；同一`StaticMeshActor`及HitComponent原始指针在地面以上35／60／80单位均命中，位置依次约`(-427.7049,-3118.1401,-92.0000)`、`(-427.7059,-3118.1401,-67.0000)`、`(-427.7067,-3118.1401,-47.0000)`，法线均约`(0.996213,0.086943,0.000041)`并朝向来射方向，线段内误差小于0.00013单位。每条reader仍为8255读，未改16384/三秒门；+60调用期间对象count228203→228204，+80期间228204→228205，名称表头及已使用的约1904个对象身份/值/代码证书保持原样，选定依赖增长证书按**当时实现**通过。这是当前调用链在自然对象追加时的真实射线返回，不意味着整个未读注册表前缀不变。后继审阅发现当时的`LiveRegistryAnchorSampler.fresh()`虽核对名称入口索引/文本，却不与原入口地址比对；因此CT12不能宣称“所用名称槽指针不变”，其认证边界留待下述补证。CT6同槽、同方向的约地面以上113单位world-only射线曾完整空返回，因此顶沿暂以**80..约113单位的跨会话候选范围**记录；尚未在同局精确夹逼，也未证明左/右/上抓边或LGrip可达。stop10／host0、session stopped/passed、Native／entry恢复、XR0、PG shutdown／entry恢复、原十文件与设置存档不变，fresh CIM Game/XR空；原回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct12-runtime.json`。窗口已交统筹排程，总体36%不变。

针对名称槽指针缺口，Trace专用reader现在仅为**实际用到的唯一名称索引**从原Resolver缓存指针取得一次见证，并用原`CoverQuery.values`在每次关闭时复读该槽；普通CoverQuery、对象数组/名称表头、16384读/三秒预算、其他身份/代码/生命周期门均不变。使用真正的`LiveRegistryAnchorSampler`与现有owned内存fixture重现“新入口地址保持相同索引与文字”：原sampler接受，新增Trace查询拒绝；两种对象/名称count回退、旧槽变更及新增尾部索引也保留回归，54项聚焦测试通过。CT11保存的1811个已遍历对象对应1676个唯一名称，按CT12原8255读及一次见证+三次关闭复读估计为14959读，低于原16384；这只是离线预算估计，不代替实际CT13验证。旧CT12返回不事后提升为新版证书；新源尚未在游戏实跑，LGrip关闭，36%不变。

CT13 使用 fresh 私有 0.4 实例尝试运行后在**启动期**被原有窗口门拒绝：游戏窗口候选可见但 client rect 为 0×0，正常的启动 VR panel 等待以 `Game window unavailable for startup VR panel` 超时，launcher session 为 `incomplete/passed=false`。host 在 ready 之前以 exit0 结束，掩体未进入、`Actor.Trace` 调用数为 0，名称槽新证书因此仍无实际游戏验收。外层诊断脚本的清理回读又把直接的 `launcher-session.json` 当作含 `session_report` 的索引，抛出 `KeyError`，这只是报告错误，不能遮盖原启动拒绝；原始 `ct13-runtime.json` 保持，单独的 `ct13-cleanup-audit.json` 用哈希绑定原 case、session 和 host 结果。Native 未激活，Playground 仅预约且 shutdown/entry 恢复，prime 取消并恢复入口，音频 9/9 恢复且无待清理，原十文件与设置存档不变；host 与 fresh CIM 均无剩余 Game/XR。**没有**正常 session passed、Native 激活后清理或 XR exit0 的证据，不把 CT13 算作射线验收。窗口已交帧率会话，后继只在新实例、完成启动原因核对后重试；不用前台启动或自动改窗口。抓边／碰撞接入门另列于[碰撞实施顺序](../../6dof-cover-collision-integration-20261026.md)，LGrip 仍关闭，总体 36% 不变。

CT14 是 CT13 **启动前零次调用**之后的一次有界全新实例重试；冻结脚本保留相同四条射线、0.4 scale、后台置后、静音及只观察输出，不扩大预算或放宽窗口门。正版 Game460／XR 在 CoverType2／SM0 同一 Link／slot 上，地面Z≈−127命中，墙面在地面以上35／60／80单位连续命中同一个 `StaticMeshActor` 与 HitComponent，法线约 `(0.996213,0.086943,0.000041)`。四条各 `trace_calls=1`、`accepted=true`、`pending_possible=false`；新版 Trace reader 对实际所用约1904个旧对象及1768个唯一名称槽指针、身份／metadata／Cover值和代码链在读回边界复核。第一条地面射线观察对象count 228201→228203，+80观察228203→228204，所选依赖与名称表头保持；读预算每条15325/16384、约144551字节。这个证书只覆盖被选中的依赖与有限读回边界，不证明未读的整个旧注册表前缀、精确上沿或人体体积。stop11／host0、session stopped/passed、Native／entry恢复、XR exit0、PG shutdown／entry恢复、原十文件及设置存档不变，fresh CIM Game/XR空；原回执在 ignored `analysis/cover-trace-diagnostic-candidate1/ct14-runtime.json`。窗口已交帧率会话，LGrip仍关闭，总体36%。

Native接入前的纯CPU事务准备增加只读 `CoverInputOwner::pending_movement(ticket)`：仅在同一GT已决定而未提交的ticket中返回已提交与候选虚拟偏移、候选视点／真实头部位移，以及前者与当前world／Pawn／cover／lifecycle的基础坐标是否匹配。它**不是**碰撞许可，不修改本次候选或武器提交，失败 `complete(false)` 仍只能保留原已提交偏移。新回归实测抓取→候选上探0.3m→失败提交→下一帧保持0，并检查旧ticket失效、cover与lifecycle换代失效；定向MSVC `CoverInputOwnerTest` 83项、`CoverInputTransactionTest` 745项均0失败。当前工作树缺 `tools/third-party/minhook`，新的完整CMake配置在该依赖门失败，所以这两项是直接MSVC定向编译／执行，不声称完整Native构建。正式Scene内Trace、非零extent扫掠、动态阻挡、失败fallback当前占用、身体受击／枪口及最终视点仍待接线和验收。

CT15 原计划同局复核地面／低墙再测地面以上80／96／112单位，最多五条射线。全新0.4后台正版实例的 CoverType2／SM0、Link／slot初次证书成立，地面 world-only Trace `accepted=true`、`trace_calls=1`、15325/16384读，再次命中Z约−127。提交**第二条**墙面射线前，按原脚本重新调用 `observer.sample`、严格`CoverQuery`和`cover_geometry_state.capture`；保存的前后 Controller/Pawn/World/Link／slot均相同、CoverType2／SM0仍成立，但 `geometry_accepted=false`。脚本当场拒绝，墙面与三条高位射线均未调用，未取得同局顶沿夹逼，也不能从这次拒绝推断墙面不存在。CT15未保存该几何reader的具体 `rejection` 字符串；后继只在新实例的下一条**未提交**射线前保存原因并允许一次完整只读重取，任何身份/状态变化或已提交未知结果都不重试。正常stop8／host0、session stopped/passed、Native／entry恢复、XR0、PG shutdown／entry恢复、原十文件及设置存档不变、fresh CIM Game/XR空。ignored原件 `analysis/cover-trace-diagnostic-candidate1/ct15-runtime.json` 保留，LGrip关闭，总体36%。

CT16 将 CT15 的单次只读几何重取限定在**下一条未提交射线之前**，同时保存具体拒绝原因；脚本的离线编译和固定入口检查通过。但全新0.4后台正版实例在启动期再次被原窗口门拒绝：唯一可见的游戏HWND client rect为0×0，另一候选不可见，30秒内没有满足启动VR panel条件的窗口。host ready前 exit0，session `incomplete/passed=false`，Native未激活、XR未启动、Actor.Trace调用0次；预定的几何重取根本没有执行。这与CT13的启动失败相似，不能当作掩体几何或射线结果。Playground预约shutdown/entry恢复、prime取消且入口恢复、音频9/9恢复且无待办、原十文件及设置存档不变，fresh CIM无Game/XR；原回执在ignored `analysis/cover-trace-diagnostic-candidate1/ct16-runtime.json`。CT14成功与CT16失败使用相同0.4配置、测试用后台置后构建和sim Native哈希；两局tiny-output观察最终均出现1280×720，因此不能仅由渲染输出尺寸推断HWND失败根因。下一步先离线核对启动窗口状态与quiet-startup路径，再考虑有界新局；不绕开窗口门、不抢焦点或调整用户窗口。LGrip仍关闭，总体36%不变。

为下次同类启动拒绝增加**一次性只读窗口详情**：30秒原门仍按可见且client rect至少640×360选择；仅在超时时，对当次已枚举且仍属该Game PID的HWND记录window rect、线程、iconic/zoomed、style/extended style。诊断失败单独记录错误，不改超时判定，不显示／移动／缩放窗口。`py -3 -m py_compile`与原窗口等待/取消/身份定向单测通过；尚无新游戏实跑，因此不能据此断言0×0成因。

CT17 在帧率会话明确交还后，以全新0.4后台正版实例完成CT16未能运行的同局顶沿夹逼。Game29196／XR34148进入 `CoverType=2`／SM0，原Controller/Pawn/World/CoverLink/slot在五条world-only、flags8的 `Actor.Trace` 前后保持，五条均 `accepted=true`、`trace_calls=1`、`pending_possible=false`；提交前一次只读几何重取许可没有被迫使用。地面命中Z约−127；地面以上35单位再次命中低位墙面；+80（Z约−47）和+96（Z约−31）仍命中**同一** `StaticMeshActor` 和HitComponent，法线约 `(0.996213,0.086943,0.000041)`，线段内误差小于0.00013单位。+112（Z约−15）为**有效空返回**，不是未提交、超时或证书拒绝；空结果的全零HitLocation／Normal不表示场景坐标。因而该同槽、同线段的实体挡面顶沿被夹在地面以上96命中与112空返回之间，尚非精确边缘点，也未证明它和CoverLink可抓边缘完全对应。每条完整查询约15323～15325读／16384，保留所选旧对象和名称槽见证；顶部有效空返回的所选对象1903、唯一名称1767。正常stop7、host0、session stopped/passed、Native／入口恢复、XR0、PG shutdown／入口恢复、原十文件及设置存档不变，fresh CIM无Game/XR。ignored原回执为 `analysis/cover-trace-diagnostic-candidate1/ct17-runtime.json`；没有非零extent身体扫掠、抓取距离/路径、动态障碍、受击/枪口或实体头显验收，`collision_verified=false`／`grab_authorized=false`，LGrip仍关闭，总体36%不变。

为共享身体底层排查 Goal 在注册退出后的首个拒绝，另从冻结 Native47 `source2` 建立私有 `analysis/body-goal-first-reject-candidate1/source3`；787个文件中只替换 Goal/共享输入两份诊断源码，原 `source2` 哈希前后不变。真实 `epochs(next)` 中非零cookie→`absent_quiescent`／零cookie记一次 epoch，初始零cookie不记；Goal保持原短路门顺序和poison行为，首个拒绝只读记录状态与19位key相等掩码。CoverShared Host 的 absent 11项、goal-transition 15项均0失败；修正测试自身的返回码后，独立 `cpu-build2` 的 Goal mode40 为12项／0失败，单独单位比例不等只落在bit13，plans为0。先前 `cpu-build1` 的若干失败曾在调试重跑中被覆盖，不把它们称为独立封存回执。

私有 `GearsCaptureCoverGoalFirst1.dll` SHA256 `efdf83825f28a7a23b8a30d01338aa55aa9cbafefb7d17986bf492cc46f47462` 已按 Native47 的实际编译flags构建，44项导出、22项原启动自检、23项原preflight通过；源码清单与逐项回执在 ignored `analysis/body-goal-first-reject-candidate1/`。它仅是定位候选，尚未运行Game／XR，也未证明玩家Stop原因；默认Support3／菜单构建没有替换，LGrip仍关闭，整体36%不变。

随后将原武器ES固定8动作/Stop/恢复案例只读派生到 ignored `analysis/body-goal-first-reject-runtime-20261026a/`，原父pin `b6fdd671…5f8f76f6` 不变；新案例单独pin CT16启动器超时详情增量及武器校准私有拷贝，离线37个脚本编译、12个命名空间检查、4个CPU案例通过。一次正版0.4后台局实际加载上述诊断Native（Game59052，`started_qpc=2854251650177`）、原PG `7e927e17…8b61aa` 和BQ XR `1a059d2d…b4e9`。原8动作、40/6秒held门、HandStop4→5及quiescent通过；Stop后的 `body_native_commits` 302→332、Goal plans349→379、applied302→332，main-mount commits保持56，Goal rejected保持86，原 `body_continues_after_stop=true`。但 `body_goal_first_state=0`，**没有首拒分支**；此局不能定位原玩家的间歇故障。shared-hand cookie在动作期为1、Stop后为0，absent计数232→278；现有epoch仅在前帧非零cookie、当前零cookie和当帧absent同时成立时触发，动作屏障使absent晚到是源码上的可能漏记路径，尚无逐帧顺序证据。不能把此推断写成实际成因或已修复。

该局coordinator complete/errors0，finalizer仅一次 accepted：session stopped/passed、Native／入口／PG、106项及完整代码段恢复、XR exit0、原十文件和音频9/9恢复，fresh CIM无Game/XR；窗口已交还FPS会话。完整身份和前后计数保留在 ignored `goal1-runtime-summary.json`，原 `ready.json`／`runtime-review.json`／`runtime-final.json` 不改写。下一步先离线补“cookie退场和absent隔帧”的真实路径测试，再决定是否增加持久待决事件并申请新的独占测试窗口；LGrip仍关闭、总体36%不变。

离线进一步发现 `body_goal_first_state=0` 也可能只是本局**没有新Goal拒绝**，不能单凭它判断 epoch 是否已经武装。因此先做更小的一文件被动诊断：ignored `analysis/body-goal-epoch-visibility-candidate1/source4` 从已实跑source3精确复制787文件，仅在Goal状态输出增加 `body_goal_absent_epoch`，事件与拒绝门均未改。独立Host mode40 15项／0失败，候选 DLL SHA256 `40d2063db8fb32b7180f4c99740eae15a15fc9c5bd258a084a2735d52ee741e3` 保留Native47实际编译flags、44导出、22启动及23 preflight通过；尚未运行Game／XR，不据此断言跨帧漏记或故障根因。碰撞Native窄Trace也正仅在另一个私有副本做离线原型，场景锁内调用未认证前保持失败关闭，LGrip仍关闭、总体36%。

Native窄Trace的**私有离线原型**现封存在 ignored `analysis/cover-native-trace-observe-candidate1/`：冻结Native47 source2 的787文件原件不变，副本仅改 `capture.cpp`、`cover_pose_phase_live.inl` 并加三个窄文件，`cover_shared_input_live.inl` 的零range/空edge不动。它限定Actor.Trace/native277的反射链、GNatives、完整代码块、选定对象及名称槽证书、Scene身份与鲜度、输出分类及计时；但 `scene_lock_certified=false` 是编译期固定门，任何INI值都不能使新Trace调用发生，状态明确为 `scene_lock_unproven`／`called=0`。最终DLL SHA256 `ad915290d57a1755dfc020ff6ddfc87bc35ff95082ff87bc032030c2cd19173b`，16项传输假调用、32项元数据变异、原22 startup／23 preflight均0失败，capture实际flags和44 exports与父候选相同；首次编译路径及有符号比较警告导致的失败在私有README保留。尚无Game／XR、Scene锁/重入安全、非零extent、动态阻挡或实体头显证据，不能给LGrip或碰撞授权，整体36%。

后继接线审阅确认candidate1在旧body/camera/input三项运行时关闭的新局中，通常首先因旧body GT未latch而使neutral reader拒绝；旧body metadata/layout/hook所有权与中立校准还会继续阻塞，且neutral自身包含原生骨getter。因此“新Trace门关闭”不等于整个前置链零引擎调用，也没有现场证书可达验收。正在另建被动CoverTraceContext，复用独立的共享actor绑定快照、单独认证当前Link/slot/世界/位置，绕开旧neutral/geometry与所有骨getter。原candidate1及失败事实保留。同期从原数据库的私有副本用现有IDA批处理机械导出9个窄函数／983条指令；包装chunks匹配CT17，但23条直接调用及21条间接调用仍不能证明Scene锁安全。MCP因缺失ida_nexus退出，本地既有IDAPython批处理exit0；没有安装工具，原DB哈希保持，owned进程退出。证据在 ignored `analysis/cover-native-trace-lock-audit1/audit.json`，整体36%。

2026-10-27恢复服务中断后的保存进度：私有candidate2完成独立CoverTraceContext，实际observe假Scene324项／0失败、transport16项／0失败，覆盖三个旧开关0、旧body状态冷启动不变、20次CT0等待后入CT2仍可达关闭门。完整DLL编译exit0，SHA256 `f8dc0243c7201faa7812ebf436021216f67492c4fe25416a8b9c962019e6dd1d`；原45图形/启动自检尚未运行。后继审阅发现identity内部registry头未累计全部实际观察，存在增长后回退被掩盖的缺口；已要求在独立candidate3中修正并保留原件，不能把candidate2作为最终验收。根KB artifact2历史WarfareGame包的WarPawn直接children933项，candidate2全链身份容量512且多次重复扫描，需先解决有界规模，不能以小fixture代替真实成本；可重现census与包身份见 ignored `analysis/cover-native-trace-lock-audit1/warpawn-field-census.json`。本阶段无Game／XR，Trace门始终关闭、LGrip关闭，总体36%。

共享Stop优先案例已从成功的goal1原动作只读派生为 ignored `analysis/body-goal-epoch-runtime-20261027a/`，Native选择已过原45自检的epoch可见性DLL `40d2063d…ee741e3`；新pins `c9a18711c9c27b38c0635565b95edee6772c1dce63b0ed018553c1c9a1d56083`，62私有文件，原coordinator及INI不变。新案例37个Python脚本编译、12个namespace、4个CPU preflight通过，原ES和goal1三回执保持；尚未启动Game／XR／Operator。用户正在运行其他游戏且FPS排程待交还，图形自检/新局继续暂停，仅低开销源码与回执工作；下一可用实跑先区分epoch是否武装与实际新Goal拒绝，不宣称已定位间歇问题。

上述A案例随后在FPS明确交接后完成一次0.4后台启动尝试：原30秒窗口门拒绝，可见HWND的client为0×0；未到ready、Native未激活、XR未启动、控制命令0，因此没有Goal epoch运行证据。host exit0但complete=false，coordinator exit1；原finalizer仅执行一次，因缺少session_report留下KeyError／accepted=false，原失败不覆盖。另立只读startup-cleanup-audit：prime取消及入口恢复、PG预约shutdown0及入口恢复、音频9/9恢复、原十文件不变、fresh CIM无Game/XR，startup_cleanup_observed=true，不把它改称会话通过。B案例仅移入已有的超时后只读窗口详情诊断，保留原窗口门、Native、INI、动作；pin `5baed00ba3acebdd342c5a4cefe01edc6857ada4da47ed4f1222bb77ab36d9bd`，37/12/4离线预检通过，尚未启动。

保存的旧ES与成功Goal1回执进一步独立对照：旧ES从quiescent baseline至final，Native captures543→663、Goal rejected88→208，而Goal plans525、Native apply_calls525／commits464不变；成功局同期增加30次Goal plans／applies／身体commits。旧ES observed stream/reference/head在baseline前已从1→2，native reference epoch始终2；冻结Goal的basis包含这三个连续性值，key mismatch会poison origin。这是与计数相符的具体前置拒绝候选，旧局没有记录首拒分支或连续性变化原因，仍不算根因。逐项原回执哈希与源码身份在 ignored `analysis/body-goal-epoch-visibility-candidate1/saved-es-goal1-comparison.json`。

Native只读上下文的candidate3/4仍为私有准备。candidate4尝试单次owner链发现并保留所有访问的前缀见证，夹具加入历史WarPawn933及Engine.Actor360条声明和增长后回退顺序；不能用candidate2的小规模通过数替代。FPS交接的短CPU窗口内，一次BelowNormal编译约6秒因两处局部slot遮蔽成员而在/W4 /WX下失败，驱动exit1，测试executable未启动、测试数0、fit未知，cl原生返回码未保存；失败回执与日志独立保留，未放宽warning或自动重建。没有DLL／D3D／Game／XR操作，CPU窗口已交还。LGrip关闭、默认冻结构建未替换，总体36%。

随后仅将上述两处局部变量改名，精确delta保留为source-fix-after-attempt1；FPS再次明确交接后，独立attempt2保留原/W4 /WX和BelowNormal，编译exit0，fixture进程以3221225725（0xC00000FD／栈溢出）退出、驱动exit1，约10秒；stdout为空，检查数、fit和budget均未知，不记通过。attempt1三项原回执及日志SHA不变，没有自动重试、完整DLL或45项图形自检，cl／fixture无残留。当前只读审阅夹具和live调用链的大对象栈占用，不能靠增大测试/STACK隐去真实GT栈风险。

同一明确交接窗口内，B单次正版0.4后台局Game62936／Native started_qpc2930735530854实际到ready，未触发本次新增的窗口超时详情。原coordinator更早在shoulder-fixed-first32的40次/6秒held-selector coherent门超时，complete=false／exit1，原门未降低且未重跑。清理路径的Stop/quiescent仍完成；新增诊断实际观察absent epoch0→1，首拒reason4为uncaptured，source/key均未捕获，不能解读成key mismatch。quiescent baseline到Stop后Goal plans291→320、applied206→235、Native commits206→235、hands27→56、mainmount10不变、Goal rejects128→129，observed stream/reference/head均保持1，body_continues_after_stop=true。因此本局确认epoch可武装及后续身体恢复，仍未复现旧ES持续停滞，也未通过完整持枪姿态验收。ignored `analysis/body-goal-epoch-runtime-20261027b/epoch-runtime-summary.json`保留选定字段和原回执SHA。

B的host completed=true；finalizer仅一次accepted=true只指正常清理：session stopped/passed、106项及完整代码块恢复、Native/entry/PG恢复、XR exit0、原十文件及设置存档不变、音频9/9恢复、fresh CIM无Game/XR/cl/fixture，用户其他游戏保持运行。窗口已明确交还FPS，后续仅只读分析。碰撞接入文档同步改为现有外层meshTick的普通TickLatch读探针规划，Scene仍是唯一仲裁/提交点；未启用writer获取读权限，未开放原Scene锁内Trace门，跨Tick几何当前性和同ticket等待路径尚待实现。LGrip关闭，总体36%。

B保存片段的离线关联另封存为 `held-phase-saved-analysis.json`：原6.034秒phase内26次observer均有完整pose、pose_error0；`coherent=null`的筛选本身要求mount达到40，不能改称姿态采样消失，也不能将status先读、pose后读的异步行当同帧关联。Native v3记录仅10/32，结构与身份有效但不是原完整32条验收；10条都位于该phase，normal/consumer/end状态均1，9个planned atom与实际consumer及end读回逐字节相等、epoch与时间顺序相符。这证明有限成功帧有实际局部姿态消费，不能补足缺失30次或证明画面、权威瞄准与碰撞。

Trace栈审阅区分live与fixture：live的自动ReadContext按Windows x64字段布局推算440208字节；Proof原为namespace静态，不能合算为每次栈。fixture main保留两个各约83296字节的OldState，另有两个不同作用域reader；原PE stack reserve为1MiB。缺少崩溃栈和实际frame大小，未断言入口或复用方式，也不把fixture溢出等同真实GT已溢出。私有candidate5仅改copied context/live两文件：唯一静态reader槽、atomic busy／线程门／权威binding admission后认领GT、RAII；逐字段原地reset全部小状态和count，旧数组payload仅为失效存储，不建立ReadContext/Proof大临时量。fixture改静态大对象并准备busy、错线程、真实重入、reset失效负例源码。原candidate4源码和两次失败回执不变；未增加/STACK、GT heap或放宽任何registry/closing/预算，Scene Trace门仍关闭。ignored `analysis/cover-native-trace-observe-candidate5/delivery.json`为source_prepared_not_compiled／runtime_ready=false；未编译未运行，没有新checks或fit结论，下一动作等待FPS交接后单次低优先级CPU夹具。

FPS随后明确交接单次最多35秒的BelowNormal CPU窗口，candidate5实测总8.01秒：cl exit0／5.90秒，fixture exit1／0.113秒，482项检查／1项失败，未重试，窗口立即交还；fresh无cl/fixture/Game/XR。实测context440208、静态Proof213168、合653376字节；本夹具不再栈溢出。历史933+360正常链fit=1：context30767/32768、Trace812/32768，prefix1300、三closing累计14713／23206／30311，原50ms门保持；七种历史变异均到达且拒绝，duplicate尾拒绝，engine calls0。但不能记全套通过：mode4改变Scene.thread，旧断言仍要求preflight_rejected=1；新ReaderLease在polls/preflight前更早拒绝，故该旧期望失败。只读源审阅已定位，原script/cpp/日志及实际cl/test返回码完整保存，独立 `fixture-attempt1-analysis.json`列出SHA。仅准备独立attempt2脚本，改该项为thread_rejections=1且rows/attempts/preflight/polls均0，不改两份生产inl；另把binding累计读取和observe自身增量分别输出，原353228累计值不能当作单次observe成本。attempt2未编译未运行、未完整DLL/45自检/实际Game；delivery为fixture_failed_one_stale_thread_expectation／runtime_ready=false，LGrip仍关闭，总体36%。

FPS再明确交接一次CPU窗口后，candidate5独立attempt2总6.48秒、BelowNormal cl exit0／4.39秒、fixture exit0／0.109秒，482项／0失败；生产context/live哈希未变，原attempt1失败文件逐项SHA保持，窗口立即交还，未DLL／D3D／Game。新计数区分出完整observe的safe_copy增量实际为353201（setup前累计27、最终353228），并非约3.16万；它包含共享binding sampler在两个局部budget外的读取。本夹具normal path通过原50ms和各局部预算，不证明实际游戏总耗时、完整读取预算或每帧适用性；合成safe_copy也不能替代实际guarded copy及成本Scope。`fixture-attempt2-analysis.json`保留边界，delivery为cpu_fixture_passed_dependency_cost_open／runtime_ready=false。后续先处理依赖成本和真实Tick接入，不因小夹具通过就开放Scene Trace或LGrip，总体36%。

共享持枪失败的下一项日志已准备为私有 `analysis/body-held-outcome-candidate1/`，从冻结Goal4精确复制787文件，仅两个inl改动及新POD header；正常源码未替换。现有真实Compose begin/end接入独立GT/CAS记录槽，以实际PG callback-ready仅武装七秒／128条观察，保存原source/capture与新Tick原始时间、query前后body_mount_source_current首个实际失败表达式及Context verdict、三次准入实际结果、mount选择、Kernel prepare/commit和held/trace序号；旧budget格式及所有权限门保留，GT不IO，按原drain门输出独立文件。机械审阅原19个短路表达式顺序、实际query/current/Kernel调用次数相同，不等于运行或时序等价证明。源码审阅修正了错线程apply拒绝前读取mutable状态及诊断槽竞争／无效时钟武装问题，初稿和三次源码修订均留存。Host负例源码已准备但尚未编译／运行，完整DLL/原45与新局均未执行；`delivery.json`明确source_mechanically_audited_not_compiled／runtime_ready=false，不据日志候选宣布修复。

日志Host首次BelowNormal编译以cl exit2拒绝：测试全局parent/stamp在模板实例化时触发/W4 /WX的C4459；测试未启动、未重跑，原test/header/回执/log均封存。仅改测试全局名后，FPS另行交接的独立host-attempt2总7.11秒，cl exit0／3.37秒、test exit0／0.027秒，20项／0失败，record520／header120字节；生产header和接线inl的788文件manifest保持。检查覆盖160次bootstrap不占容量、首个callback-ready但source-after拒绝帧保留、实际结果字段、错误finish、真实std::thread的异线程拒绝与重入、容量／期限／无效clock。它仅是独立记录器验证，完整Native接线和off-GT导出未构建，未原45／Game／XR；delivery为standalone_host_passed_integration_unbuilt／runtime_ready=false。窗口已交还FPS，完整冻结flags的纯CPU构建入口已准备，尚未执行，不替换默认构建或宣称故障修复，整体36%。

完整日志候选首次CPU构建停在showIncludes：cl探针exit0，实际CP936中文前缀被原build_verified严格UTF8解码拒绝，尚未CMake configure或Native编译。原失败/raw保留。只读核对父console输出CP65001、系统ACP936，原成功父构建前缀为UTF8；另一次独立交接恢复原builder对既有console的继承，不强加CREATE_NO_WINDOW，不创建console、不修改codepage，仍BelowNormal/parallel2，CP非65001则拒绝。复用原唯一探针，在CMake前严格decode/encode逐字节往返通过，实际前缀等于原成功记录；没有忽略错误、硬编码前缀、额外探针或跳过依赖证明。

第二次完整CMake configure／build／Ninja依赖均exit0（build约477.59秒，总496.94秒）；driver后置匹配却因假设Ninja对象路径使用正斜杠而在Windows反斜杠行assert，原driver exit1完整保留。立即交还FPS且fresh无compiler/Game/XR，没有重建、DLL加载或GPU自检。随后仅从既有Ninja／verified-build／DLL做独立只读审计：路径正规化后actual FLAGS/DEFINES与冻结父版本完全相同，788文件manifest不变，新header依赖有效；用项目既有pefile读取的44项导出完全相同且无forwarder。私有 `analysis/body-held-outcome-candidate1/build2/GearsCaptureSupport1.dll` SHA256 `2776ded8955feabcd25432a1bf552e6c2f57316bb0a8c6f0b430c25f23f9ac05`；`native-build-attempt2-artifact-audit.json`明确区分原driver失败与产物审计通过。29项编译warning的文件/代码/数量与冻结父build一致，新header warning0。原22启动／23 preflight、实际Game／完整六秒日志均未执行，delivery为cpu_artifact_verified_driver_postcheck_failed／runtime_ready=false，未替换默认或宣称故障解决，整体36%。

原budget的first32只覆盖phase前段+0.056～+2.069秒的25条：14条code3/branch7，依据冻结broker路径表示source_before成功、回调后body_mount_current失败；同entry记录no_query_age=1且只到post40_end，未到hands/prepare/commit。它们query末原Scene age为50.2164～51.665ms，但bool包含多门、未记首错，新Tick原始位姿时间不在这份旧Core预算中，不能独占归因为age。另11条ready里10条到commit_end调用边界、1条在final40 age拒绝；仅3条能用tick/compose与成功held trace精确匹配，其余7条缺逐entry Kernel结果，不能以聚合commit code计数强配。第一份C0失败发生在本phase前约7.075秒，也不能冒充phase首错。下一可验证动作限定为准备针对真实held阶段的有界被动逐entry结果记录，保留实际source/capture时刻、query前后首个实际失败门、post/final admission、选中mount及Kernel结果；不续时、不额外求值guard、不改40/6秒或任何时效/key门。此段无新Game、编译、IDA或额外助手，原回执均不改。

日志候选随后完成原有独立22项startup／23项preflight：45项／0失败、15.497秒，所有子进程正常退出、产物前后SHA不变；私有DLL仍为`2776ded8…3f9ac05`。原build driver的Windows路径后置检查失败仍保留，没有重建或覆盖旧回执。`analysis/body-held-outcome-candidate1/verification1-analysis.json`明确这是standalone检查，不是某一Game调用的startup proof，也没有GT日志导出、实体头显或默认集成证据。

FPS会话被用户暂停并明确交还窗口后，单次正版0.4后台ha局（Game11912／creation134375142772941504）使用同一原40次／6秒动作、原30秒窗口门和新被动日志DLL。窗口门拒绝：主HWND9443022可见但iconic、client0×0，window rect为屏幕外的最小化位置。Native未激活、XR未启动、控制命令0，新held journal未被测试；host completed=false，原finalizer仅一次因缺少session_report保留KeyError／accepted=false。独立只读`analysis/body-held-outcome-runtime-20261027a/startup-cleanup-audit.json`确认预约prime取消／入口恢复、PG reservation-only shutdown0／入口恢复、原十文件与设置存档一致、音频**2/2**恢复且pending0、fresh Game/XR及准确自有owner/Game无残留；startup_cleanup_observed=true不等于会话验收。测试窗口随后释放，没有连续重开。

保存的B成功启动与ha拒绝进一步按原始quiet二进制哈希、窗口序列和源码离线对照：二者早期STARTUPINFO和API策略序列相同，均将游戏的ShowWindow2改为7、先出现visible/iconic/client0；B稍后才出现同HWND的非最小化640×360。B最后一组quiet导出早于该变化，原日志没有覆盖后继调用；CBT的after-CallNextHookEx也不是User32操作完成后的最终状态。`GameWindowFrames::recover_window`实际仅枚举并重新绑定已有可用HWND，不执行窗口展开或移动，不能将B成功归因于它恢复了窗口。可重现脚本与`startup-saved-comparison.json`保留输入SHA及root_cause_located=false；无新Game、窗口操作或门限变更。LGrip仍关闭，总体36%，默认Support3／menu-modal1未替换。

晚期缺口收敛为ignored `analysis/late-startup-boundary-candidate1/`：复用唯一quiet Observer，前两组3/12秒不变，第三组从固定24秒移到原窗口门已得出结果的边界，仍最多3组／6 exports、各128条环、原始输出326112字节；带精确PID/creation/token的一次性rendezvous只在原门结果之后最多等待1.5秒，前后只读窗口状态，不新增窗口或输入操作。原身份、30秒／640×360门、持枪动作与Native不变；删除新增诊断节点后launcher AST与rehome父版相同。26项CPU负例通过，覆盖无host截止、pending不发新请求、不重放、预算、身份/token/时钟/超大输入拒绝。新hb私有预检的38脚本／12namespace／4 API检查成功；外层写回执时撞到继承的历史preflight-attempt1.json而FileExists失败，旧件保留，独立audit恢复实际成功host-preflight，child返回码不可追溯，不伪造也未重跑。新pins为`d8affd28d4b5742731f15574b8eb2b079e10699a1dd8289bc44f5efc7f16ce45`。

唯一hb正版0.4后台局Game73444／Native3029853403801通过原窗口门；边界观察0.266秒、within_rendezvous=true，6份quiet全部valid／退休、无pending/error/overflow，捕到23条API／3条CBT，前后主HWND2500834均非iconic、640×360。主窗口初次minimize后约26.4秒起确有restore9→8、normal1→8和后继CBT normal1；这是本次成功启动的晚期控制，不是旧ha首次最小化或未展开的唯一根因，也不把成功BOOL调用残留LastError当失败。原动作更早在`Expected production equipment result absent`超时，episodes0、未到原40次／6秒held阶段；新journal仅120字节头。真实按下counter1／lastpress2969，已有PG有界16条gap日志均`previous_sync_stale`；按下附近frame273旧sync age107.9165ms、当前sync age52.3782ms、相邻sync差55.5383ms，不能称XR两次sync间隔超过100ms或擅自归因外部进程。此所属输入调查已交武器会话，未改门或重放按下。

hb正常Stop／quiescent后身体继续、双手控制器恢复、bridge关闭；原finalizer仅一次accepted=true只指清理：106及完整代码块、Native／entry／PG恢复、XR0、原十文件及设置存档不变、音频9/9恢复、准确自有PID及fresh Game/XR空。host较早no_remaining_games=false的瞬时快照不覆盖最终清理。ignored `analysis/body-held-outcome-runtime-20261027b/saved-runtime-summary.json`保留原输入哈希、晚期API与时效分解；没有第二局、Native重编、默认替换或功能完成加分，总体36%。

hb生产侧时序已另行封存为ignored `producer-timeline-saved.json`（SHA256 `eaca4af6e22410167b9565b11a1f5ea7ef95473943a225e23bd3d2c3bffdef8c`），与武器会话消费者归类按同seq/time交接，避免双路重复调查。实际XR `1a059…b4e9`的sim-candidate/source manifest及四个相关source文件哈希相符；position／press／release的source1、session302958259964100、space1、profile2、input2不变，右手press0→1→1、release0→0→1、lastpress2969。GVC3/GHC1/stereo均3700次publish／0 skip，tracker0拒绝／epochs1；这些是本局统计，不等于实体追踪验收。`received_ns`实际在XR采样和边沿更新完成、bridge publish前写入，不能冒充游戏收到该包的时间。frame273/source2972的sync→该时标为0.0167ms，到frame.now为10.8169ms，再到input-observed为41.5446ms；后一个只是原时标间隔，尚非定位到具体函数的耗时。原CSV在240帧后降至1Hz，缺少exact2969的逐帧publish/匹配read起止；不能补造时刻、以当前时标续期或拿更新的2975替换固定stereo2972。此轮只有既有证据与源码只读核对，没有新局、构建或门限调整。

武器侧独立首断点核对与上述同seq/time相符：frame273确已到Input::next的previous_sync_stale，位于new_press/router/event之前；reject重置bound/router/cursor，frame274重baseline；还不能用Outcome默认press0代实际rawcounter，缺少当帧和此前基线计数的逐字段闭合。Root因此准备ignored `analysis/equipment-input-edge-evidence-candidate1/`：精确复制实际PG `7e927…61aa`冻结787源码／17vendor，生产仅Input Evidence追加旧bound/seq/press/release快照、backend在原16条gap及既有status行增加原始字段；status另标input_evidence_valid防止旧值冒充当前，GT布尔只记录实际next之前是否经过inventory。没有新sync/query/read/clock或权限、没有重放/续期/放宽100ms；删除诊断members/assignments后原Input文本相同。原Input加8项有意义的基线重置证据断言后69例／741 checks、Runtime84例／900 checks通过，source2仅续补backend日志，header/test/runtime与已测版本字节相同，未重跑不变测试。

私有PG首稿实际编译exit0，driver后置检查仍因Ninja Windows路径格式留下ValueError，失败保留；独立只读审计确认flags及16导出一致。随后为补足baseline当前rawcounter与next/inventory路径标签建立独立source2，原稿与回执不改；source2 configure/build exit0，实际flags与冻结父版一致、16导出同名无forwarder、零compiler warning，DLL SHA256 `3eb4932cdc17e43a1889b51778d63a6033f58f5b976443466e7aa0ef2b8927ac`，`pg-source2-attempt1.json`保留来源。Native `2776…ac05`／XR `1a059…b4e9`均未重编或替换默认。新hc私有案例pins `b5d709ac134119629f4e8c9a4f647f2cc4bec8b33611ca9aae8933f47c4cc056`，38脚本／12namespace／4 API预检exit0；coordinator只改PG_SHA常量且其余AST相同，新SHA `24a973b6bdbdfd0ed55ced9fe9a0ce508f5ca69bcd9510b19ae94bf0ec229ef3`，原按下/松开、40次/6秒、身份、106恢复门保留。此处仅记录候选与离线检查，hc尚在单次实跑中，不算修复或功能加分，整体36%。

hc随后单次正版0.4后台Game64604／Native3046028507935通过原窗口门和正常Native启动，仍在同一装备结果门超时、episodes0，未到held阶段／journal仅120字节头。新增日志把具体3550边沿逐字段闭合：frame278/source3553的rawpress1／release0／lastpress3550，前次bound1/source3547／press0／release0／lastpress0；实际previous_sync_stale在new_press/router/event之前拒绝，next_after_inventory1。下一frame279/source3558确有input_evidence_valid1／baseline／rawpress1／lastpress3550、previous_bound0、event0、next_after_inventory0。因此本次具体按下进入消费者后被时效门拒绝，并被新baseline吸收，不是依据Outcome.press0猜测。原时标算得旧sync111.4226ms／当前47.4542ms／相邻63.9684ms，frame.now→Input39.557ms；callback顺序标签不等于定位inventory耗时，不宣称XR采样中断或旧局唯一原因。`analysis/body-held-outcome-runtime-20261027c/edge-rejection-saved-summary.json` SHA256 `1ca306047633db0c66e67b99a6b90235b7e2cdc22fbc5eb7fc413e90116a0412`保存原件哈希。finalizer仅一次accepted=true／106完整恢复／Native-entry-PG／XR0／原十文件／音频9/9，fresh Game/XR及准确自有身份空，Stop后身体继续，测试窗口已释放。按用户明确授权仅向一个Astra6/max助手提出三个实际PG文件内的最小安全修复或唯一缺证问题，禁止广泛调查、放门、续期、更新源替代、重放或新局；尚无行为修复，总体36%。

Astra源码建议与增量审阅支持一个小候选：仅将Runtime已有held early input资格扩大到普通unheld（去掉`model.held()`条件、改局部名），仍以真实观察时钟执行原100ms准入，event.created保留原previous_sync保守界，后续inventory/current/identity/attachment及250ms动作门不改；baseline/pending/future不改，同tick局部event早退丢弃不重播。ignored `analysis/equipment-input-early-admission-candidate1/`的Input72例／778、Runtime84例／901检查通过，包括unheld90ms准入后20ms库存工作及失败/源到期无动作无重播；实际PG构建`013c39f3…4adc43`同冻结flags／16导出，Native/XR未重编。准备时两项纯元数据假设（测试标识出现次数、继承案例的报告名/PG_commit键）拒绝，未执行Game；原失败脚本/回执保留，修正后hd pins `ecd4c8879d06fc805eca5a53ddb497ad78ed00ad65f163e72d555f1053ad5256`及38/12/4预检通过。此候选不保证hc早采样实际可在100ms内，源码审阅不代实际修复。

唯一hd原0.4后台局Game44240／Native3056117722988仍未通过装备结果门、episodes0、held未到/journal120；early实际next_after_inventory0生效。在按下3178之前，frame342/source3173的rawpress0已经previous_sync_stale（旧102.8921ms、当前25.1187ms、相邻77.7734ms、frame.now→Input23.8938ms，前次观察间隔66.9259ms）；下一frame343/source3178首次rawpress1即进入新baseline、event0。这与hc的按下帧拒绝是不同的具体事件，不能混写；本tick后继inventory不可能是early拒绝的唯一耗时，仍未定位前置函数成本。原finalizer仅一次acceptedtrue／106完整恢复／Native-entry-PG／XR0／原10／音频9/9／freshGameXR空，未原样重跑或默认整合，保存于ignored `analysis/body-held-outcome-runtime-20261027d/saved-runtime-summary.json`。

清理后为early候选另存`source-manifest-scope-clarification.json`，SHA256 `33c270d805a67a9e307b7a1669f07942936a6150ff10328e89283d99ffd7164d`；sealed manifest `0343673c…016a1`、hd pins及所有旧回执不改。明确其继承的delta_files/production_delta_files2仅描述历史诊断父版，cpu_input_runtime_sources_exactly_unchanged仅描述诊断source2相对source1；当前相对hc实际是Runtime生产1文件及Input test，相对最早冻结PG共3生产文件。没有为元数据澄清重编或重测。

直接调用链核对还区分了两套元数据路径：PG copy_frame/source_current走自身`weaponhand::observe→gripdamage_metadata::raw_identity`；Native共享binding虽然与candidate5同SHA `666219dc…8ed`，其353201调用数不能直接套成PG当前耗时。下一私有候选`analysis/equipment-identity-read-batch-candidate1/`仅把PG原0x40/0x48/0x50连续24字节字段三读合一，首次注册仍在前，class/outer/name/最终注册复核次序不变；块失败回原3标量，零缓存/时标/门/权限变化。Astra源码增量审阅无具体安全问题，限定为源码层且无游戏时间承诺；实际PawnRead40例／ClientOffline21例通过，读调用bulk20／原标量等效22／失败bulk回退23，含不可读子字段、修改后完整观察拒绝及真实PAGE_NOACCESS页（不冒充PAGE_GUARD覆盖）。实际PG `1336a597f7c7d47115e364e8c688fdba2c40ac3b9746ca13e902b3286c63990d`同冻结flags／16导出，原Input/Runtime源不变未重跑；Native/XR未重编。he新pins `faa7d4754c4c847bb86141d203c1e593a54c0e7684303c4b2bc5e69f323ff5db`、38/12/4预检通过，coordinator只换PG SHA、其余AST相同；目前只启动唯一不同候选原动作0.4后台局，未获得本局玩法结果，整体36%。

HE最终结果及候选安全修正（2026-10-27）：唯一正版0.4后台Game68132／Native3072491780786中，slot3真实Grip确已成功（event1／press1／frame307／source2684），但原shoulder-fixed-first32门6.023秒只有1/40次held提交，Stop后身体继续门也超时；不能以正常清理代玩法通过。原finalizer仅一次accepted=true，106及完整代码块／Native-entry-PG／XR0／原十文件／音频9/9恢复、fresh Game/XR空。武器会话机械解码109条有效held记录：phase内106条含query8的Kernel commit12拒绝、query9的commit1成功，后104条无input/query/guard/Kernel结果，均保留未知。Root另从实际源码解码同局budget（9 query／0 no-query／120 attempts／111 ignored）：只有query8/9精确匹配phase；全局first-failure为phase前1172.3921ms的tick29620／Compose29621，不能替代本次原因。Stop后的原FirstReject明确为reason10/key_mismatch、0x7e3ff，只有observed.stream/reference/head由1→3，另16项实际身份／参考空间／模型参考数据相等；该记录在phase结束后146.0534ms才产生，只定位后继持续阻断。原FirstReject只在absent epoch启用，budget不保存普通Goal无query早退，因此成功query9之后首个无input的tick33464／Compose33465究竟先在哪个实际返回点拒绝仍缺证，不将此前guard12或后继key mismatch强配给104条。可重现只读分析与输入SHA封存于ignored `analysis/body-held-outcome-runtime-20261027e/goal-budget-saved-summary.json`，未改key/age门、续时、重放或重复开局。

HE所用PG `1336…990d`随后发现独立PAGE_GUARD问题并隔离为历史实验：实际memcpy+SEH整块读取首次触发guard会清除该页guard，继而回退标量可能接受旧标量路径本应拒绝的观察；没有证据表明HE发生了此情况或它造成持枪停滞。保留source1原件，私有source2改为第一次24字节读取失败立即拒绝，成功路径及后续身份检查不变。实际冻结旧标量／1336回退／修正版三路各自重新武装真实PAGE_GUARD，结果依次0／1／0，且每路首次访问后guard确已清除；PawnRead41例／ClientOffline21例通过。修正版正确build2 DLL SHA256 `dfd995e44aad9fe6a088cce75095d1cbe4d525dbe58012251a1facdefe904669`，实际父flags、16导出、789源码／17vendor逐项审计通过。原`pg-build-attempt2.json`虽编译build2 exit0却后置误读build1，产物字段无效且原件保留；独立只读`pg-source2-artifact-audit.json`才是修正版产物依据，没有重编或为旧测试标签重测。该修正版尚无Game／XR／持枪连续性或实体头显验收，默认Support3／menu-modal1均未替换、LGrip关闭、总体36%。

针对HE精确缺证新增私有 `analysis/body-held-early-return-candidate1/`，从实际2776父版788文件仅改Goal／Native原返回点和原held header三文件；撤销增量逐字节还原父源。原128条／7秒界限不变，新 `BHELD002`／version2、Record584／Header120追加Native begin实际返回类别、Goal实际reason与有条件keybits；未执行保留UNKNOWN，不新增clock／游戏读取／许可guard，不改原短路求值顺序。独立Recorder29项／0失败；完整Native实际父flags、44同名非转发导出、788源清单通过，82.292秒，DLL SHA256 `22268f8af3c0f03136c6c0befdfe017dd9921099256779723d189381b0a313b6`；原22startup／23preflight共45项／0失败、21.439秒。冻结产物没有为展示字段重建：`BHELD002-validity-contract.json`明确raw captured不证明state属于本entry，derived source/key仅在原same_entry已通过的reason7～18解释，reason1～6及19不授有效性；reason7仍不证明freshness，adapter仅实际reason14解释，其他raw0不冒充okay。武器会话独立v2 reader `3724aeb1…ad2401`及10个合成窄方法0失败已交接，仅复用其来源/布局/两项语义负例，不重跑v1／整套检查。以上均不是连续身体修复或Game启动证据。

唯一HF的pins为 `785024bf9b41dcab5184e0ed000b0b4079cd208911efad70fa994fb3fbe57aff`，选择新Native22268、failclosed PGdfd995及原XR1a059，38脚本／12namespace／4API离线通过。正版0.4后台Game71384／creation134375248384348224仍被原30秒窗口门拒绝，Native未激活、XR未启动、命令0、无v2身体记录。原finalizer仅一次accepted=false／KeyError session_report保留；独立startup-only清理确认prime取消／入口恢复、PG预约shutdown0／入口恢复、音频9/9、原十文件与设置存档不变、fresh Game/XR和准确自有身份空，不冒充106运行态清理或玩法通过。失败末次quiet导出6份valid／无pending/error/overflow，旧HB成功与HF失败的可重现只读对照封存在 `startup-boundary-comparison.json`：HF同一主HWND已收到restore9和normal1，均转为effective8／Native SetWindowPos且返回1；restore完成早于结果边界4259.7332ms，窗口在边界前后仍iconic／client0×0。因此本局不能解释为没有restore或仅需延长30秒，也不能由成功BOOL推断实际恢复。

独立私有 `analysis/quiet-behind-noniconic-candidate1/`只改一行测试后台策略：behind=true时minimize2/6及中转7选择现有原子HWND_BOTTOM show；behind=false仍按旧最小化语义，show_behind／CBT／原窗口门和截止不变。当前18份源快照与原build4 DLL控制分开取证，不冒称旧二进制由当前源逐字节重建。新DLL SHA256 `19b355db1a30a54cf2498eae3a57c23645bf8d394599ac1247f634715b115ef3`的实际flags／5导出与旧版一致，原EarlyLaunch `c12172f5…97d36`逐字节复用；单次构建及验证20.883秒。5个独立子进程均在私有非input桌面实际安装旧／新DLL、各执行10步同步/异步show：旧behind复现最小化后无法由该show路径恢复；新behind的新非iconic窗口保持client有效、置底／无active／原rect不变；旧/新普通模式均保持原最小化／恢复语义；新behind的已iconic控制仍不恢复，明确只防新测试窗口进入iconic。266项检查／0失败，5/5私有桌面关闭；未SwitchDesktop或操作用户窗口。所有原源码／build4及默认构建保持，后继HG仅选择这份独立测试bundle验证，不能把夹具通过当作游戏或头显通过。

随后唯一HG pins `167d1c75bf5e074359411f9631d9e0164de822a5536b3f4de1ec92860d640618`只在3份私有Python入口更换quiet路径/身份，Native22268／PGdfd995／XR1a059、原coordinator字节、30秒窗口门及所有动作/持枪/恢复门不变；38/12/4离线预检通过。正版0.4后台Game64176／creation134375257939895023、Native3118170081683实际通过原窗口门，完成原8动作、单次Grip／episodes1、complete=true／errors0。原40次/6秒门通过，shoulder阶段marker区间5.232秒、前后held计数0→46，随后right-ready46→54、小移动/yaw10°54→64且原coherent观测完成；不将这些聚合状态硬配到未保存的具体entry。HandStop code5／quiescent、Stop后身体继续、双控制器恢复及bridge关闭均通过。原finalizer仅一次accepted=true只指清理，独立106及完整代码块／Native-entry-PG／XR0／原十文件与设置存档／音频9/9全部恢复，fresh Game/XR／编译器空，首32 v3 hand trace头有效。详见ignored `analysis/body-held-outcome-runtime-20261027g/saved-runtime-summary.json`，测试窗口已释放，没有追加重跑。

HG仅独占复用武器v2 reader解出128条合法／0错误；capacity丢3条，虽然观测span包住首phase也不宣称完整phase/7秒覆盖。全文件首个成功held后无input的index2（tick27151／Compose27152）在首phase marker前10.8351ms：Native实际captured、Goal reason18/apply、19位key全等且poison0；apply前置门内部哪项仍未知。首phase内首个导出无input为index4（tick27365／Compose27366），实际NativeBegin8/backend_capture→Goal4/uncaptured，未执行key/adapter保留未知，不猜更深子门。全文件Native captured117／backend_capture11，Goal apply57／未记录60／uncaptured11，无reason10/11；各状态观测的stream/reference/head保持1。因此本局是正常连续更新和Stop后续更的成功对照，**没有复现或修复HE／旧ES的持久key poison**；既有source-after、admission和Kernel拒绝也不能被整体complete抹去。没有可见武器跟随、权威射线、实体头显或掩体碰撞/LGrip验收，整体仍36%。

确认HG与正常清理后，将已验证的同字节后台策略header及定向unit源同步到6dof-cover；仅把自动multi-instance默认测试构建由build-instance4改为build-instance5。新目录逐字节复制已实跑quiet19b355和原EarlyLaunch c12172、沿用严格原子置底v2证明并显式记minimize-prevention revision1，现有validator复核通过；没有为路径切换重编或重跑上述套件。原18份父源码另封存于ignored `parent-source-at-integration/`，build4及其三份哈希保持；真实场景/case/来源和集成范围见 `analysis/quiet-behind-noniconic-candidate1/integration-receipt.json`。普通启动模式、默认Support3／menu-modal1及武器代码未替换，也无发布/tag。该自动测试启动修正代码为独立小提交 `09e0554`，便于统筹按最新主线集成；LGrip仍关闭。

武器会话随后交来私有Muzzle观察PG `80b826235774f6b9126db0017fdc24f1867cc2127e99b68bb6ddfb436a3db150`（原receipt `63bde1d5…62809`），精确以HG实际dfd995 source2／17 vendor派生；811文件哈希、2既有／5新增、实际Ninja compiler FLAGS相同且DEFINES恰加单一MUZZLE宏、16同名非转发导出均核对。复用其新Pose12／typed Socket31项原通过回执，不重建Native/XR或重跑旧suite。Root从唯一HG模板派生HH/h，pins `ce01cfc987aa2defea7bb73206212ff0152b28efe7c4a2bdb627b2ec75b9b7ff`；共享start.py仅对已集成09e0554的build4→5单一字面差分逐字节反向核对后重基，旧HG pins/原件不改。PG origin明确验原compiler FLAGS及唯一新增宏，sidecar保留`parent_flags_equal=false`，不伪称所有defines相等；coordinator仅PG身份常量变化、其余AST相同。38脚本／12namespace／4API新案例预检通过，Native22268／XR1a059／quiet19b355／EarlyLaunch c12172与原8动作、40/6秒、Stop、106和R2OFF门均保持。

唯一HH正版0.4后台Game50892／creation134375274580084799、Native3134767077917实际通过启动门并完成单次Grip／episodes1；shoulder-fixed-first32原门6.0179205秒held状态0→35，未达40，后续ready／move阶段未到，complete=false。正常HandStop4→5／quiescent及双控制器恢复、bridge关闭成立，但Stop后身体继续门也超时，不能以正常退出掩盖该失败。原finalizer仅一次accepted=true只指清理：106及完整代码块／Native-entry-PG／XR0／原十文件与设置存档／音频9/9全部恢复，fresh Game/XR／编译器空，首32 hand trace头有效。原PG小日志21648字节、SHA256 `64a1dc290faf28c7a4e917f82e3267d0f0caeb54fef3becf965e8f886f3bcf28`及ready/review/final已交武器会话独占Muzzle分析，Root没有另写/解Muzzle reader、重解HG held或追加同局重试。精确动作/清理交付为ignored `analysis/body-held-outcome-runtime-20261027h/runtime-handoff-summary.json`（SHA256 `b64799c8db911de7d9728aa256f26e3e516801578c25aa4eb1a23d7c57568058`）；这一次失败不证明观察probe是原因，也没有权威枪口射线、后继动作或头显验收。Muzzle候选未集成默认，测试窗口已释放，LGrip关闭、总体36%。

HH后继仅复用已存v2／phase／postStop核对，须更正上述35的解读：它是原status文件/观测值，**不是实际Native提交总数**。v2共111条合法／0错误／0drop；首phase marker区间内有42条不同Tick/Compose的held Kernel提交标记并正常original返回，counter2～43，第40条end为marker后5.7116094秒。与此同时，26条同marker且有pose的joint观测最高仍35，phase最后读值也35；原 `phase()`要求实时status计数及携带该计数的pose观测都达到before+40，故原联合门仍失败，不能用事后标记改称通过。`status()`的perf_counter只记文件读完时刻，未保存该组body计数的独立producer刷新见证，不能从现存数据定位producer/collector/文件哪层未刷新。phase首个实际写入拒绝index1／tick27452／Compose27453记录initial-admission source_age_only；保存时钟下原Scene age50.0016ms、真实Tick输入较新，但metadata-age／sticky burn等内部子条件未独立保存，后续提交确已恢复，不能将这条拒绝当整门未过的唯一原因。

同一窄交付另证HH持枪区间没有Goal10/11；postStop首拒为tick39432／Compose39433、reason10/key_mismatch、0x7e3ff，在phase结束后142.1621ms才发生，仅observed stream/reference/head由1→2，另外16项实际身份／空间／参考／模型比较相等，origin已poison。随后quiescent baseline到host final约2.406秒，attempt/rejected各增87而plans/applied/body commits均不增；这能定位后继停更的首个实际分支，不倒推持枪首因，也不强配全部87次拒绝或断言实际XR参考重置。最小证据、源哈希、原联合predicate与下一可验证动作封存在 `two-blockers-saved-summary.json`（SHA256 `585b1703ca6a21b520b3202c844a633dc97733872409422213c7b7d59a96e3e8`）。本轮没有新Game、诊断、助手、Native修改或测试；下一候选需分别处理实时观测新鲜性和持久neutral键与逐帧输入连续性的职责，保留40/6秒、现时owner/age/reference/Kernel门，不续时或从已修改pose重捕neutral。

统筹接收HH双阻断结论后，唯一HI/i继续只换武器的具体typed读取修正PG `207cb5373f5ac675a24efb64a6f08237f17c72b2662fb6d046b80b0855f8f231`（原receipt `be549385…4f012`）：相对HH仅4个观察文件，811源哈希、实际FLAGS及DEFINES完全相同、16导出保持；修正真实Core.Object.Vector／Rotator所属链并记录原返回阶段，未增加engine read/clock/guard。复用新Socket42项及未改数学/旧Pose12证据，不重建Native/XR或重跑旧suite。新pins `1186141bf975dd5e707d0c07363e15992b145c5fa9a4e29be89b42ad5c659363`／38脚本／12namespace／4API预检通过；Native22268／XR1a059／quiet19b355／EarlyLaunch c12172、原40/6秒+pose／Stop／106／0.4 behind-mute-observe-only／R2OFF保持，没有混入新计数机制或Goal策略。

HI正版Game29688／creation134375284554559420、Native3144730156562实际通过启动门和单次Grip／episodes1；肩部原联合门6.0170403秒只观察到status0→1，仍失败，后续ready／move未到，不能将status值自动当完整实际提交计数。HandStop4→5／quiescent及Stop后身体继续通过；右手恢复却缺少exact fresh Source1 GVC3/Stereo确认（错误66），`both_controllers_restored=false`，仅能记“恢复未证”，不假定命令未执行或在bridge关闭后盲重放。bridge正常关闭，原finalizer仅一次accepted=true只证明106及完整代码块／Native-entry-PG／XR0／原十文件与设置存档／音频9/9和fresh Game/XR／编译器空；不等于双手恢复或玩法通过。原hand trace仅1条，原first32 `header_valid=false`，不当满32、跟随或瞄准证据。49675字节PG原日志SHA256 `7fb11641f10e35cad2653b27a952851ff0d9726fff6fbf79eedd78facf909770`与ready/review/final已交武器独占Muzzle分析；Root不另写/解Muzzle reader、不重复HG/HH v2或追加同候选实跑。交付封存在ignored `analysis/body-held-outcome-runtime-20261027i/runtime-handoff-summary.json`（SHA256 `7155db1e5035539555df4cf582313e83514681009f939fde8d38011559da3a6e`）。测试窗口释放，Muzzle候选/新Native策略均未集成默认，LGrip关闭、总体36%。

HI后继窄核对只复用本局原件和武器既有v2 reader：11条合法／0错误／0drop，phase内仅记录1条实际held Kernel提交并正常original返回（tick35867／Compose35868、counter1）。但journal到phase后3.9547164秒才arm、仅覆盖末段，故完整phase实际提交总数仍未知，原status0→1和联合门失败均保持。首个已记录阻断index1／tick35974／Compose35975是NativeBegin backend_capture，其后Goal uncaptured不作独立首因；真实capture包含current、metadata/shape、memory、immutable及重复读多层，未保存首拒子条件。另3条source-after明确context/source_age_expired、保存Scene age50.4574～52.2486ms且tracking/controller较新，但不能倒推首capture同因；本文件没有Goal10/11。唯一首阻断缺证为capture实际首拒子条件（若current失败则其现有verdict），本轮没有增加Native诊断或放宽任何门。

右恢复66的精确分类也已纠正：实际reader `0c707034…fe179e`及原构建9个源哈希全匹配，stderr是 `reader_open stage=stereo`，发生在Source1／100ms匹配循环之前；不能据coordinator统一错误文字宣称输入过期。StereoBridge::open未区分OpenMutex／OpenFileMapping／MapView／首read（含0ms锁与packet）失败子项。右命令写于314491205180300、消失观测314491240386300，随后左命令314491305209000，左样本seq6091／同session／space1成功；只证明后来可采样，不能补证右恢复，也没有可证的同案fresh-source缺口。原both=false和106仅清理范围保留。输入哈希、11条紧凑记录、命令时间线、边界与两项缺证封存在ignored `analysis/body-held-outcome-runtime-20261027i/held-restore-saved-summary.json`（SHA256 `c34acbb80d7ecf21ff8b532a7752f858b7be0a45ba57e201574c1b6100c218c6`）；原ready/review/final/pins未改，没有新Game、构建、助手或Muzzle重复分析，LGrip关闭、总体36%。

统筹随后将唯一Native增量限定为Backend::capture首拒条件。ignored `analysis/body-capture-first-reject-candidate1/`精确从已测22268冻结788源派生，仅改backend／live／journal三文件；CaptureObservation可选POD出参按原短路顺序记录第一个实际失败表达式，两个原current失败时才读取当次已有rejection_，不重查current、不增加游戏读取／时钟／阈值／写授权。Backend无新增成员，父/新实际sizeof均24032，shape使用的对象区间不变；剔除诊断后的capture token序列机械相同，helper体保持原字节，metadata／shape／immutable helper只记边界，不虚构内部首因。新日志显式BHELD003、Header120／Record600，原v2前392字节保留，capture_reason@392、capture_context_verdict@400，guards/admissions后移16；0仅capture成功，1～24为原第一失败表达式，只有11/24 current失败才有实际verdict，其余未执行字段UINT64_MAX。原cookie／128条／arm7秒及late-arm覆盖限制不变，旧v2 reader不能按旧偏移静默读取新文件。

受影响journal36项／0失败，Backend原135项真实read／CAS128／SEH加新增共245项／0失败，通过首拒优先、失败current只读一次已有verdict、不执行后续门、真实不可访问页、数组越界先于索引、成功unknown清理和跨entry/thread隔离检查。私有DLL实际冻结FLAGS／DEFINES及44导出相同，SHA256 `71075694c716d8dd3df5cd43b2345a32e95db126ad74ebb59e5bc4b8e807445b`；`candidate-manifest.json` SHA256 `cec627e1b13ad92721375459493e6f330c25bb9840039eddcbdce36fbb1083aa`，字段契约SHA256 `683f5b9553742f14af83a328f6a656176c5ee87df596972ff43f03a19dc1549d`。原45项成功回执作为父版历史保留引用，没有拿它冒充新DLL通过；新DLL startup22／preflight23尚未执行，当前仅CPU验证的诊断候选，已交统筹与武器会话，未开Game／XR、未修改reader／Goal策略／cadence或默认。右66由武器独立处理，候选齐后才协调唯一窗口组合；LGrip关闭，总体36%。

统筹授权组合后，准确710756新DLL已实际执行原startup22＋preflight23，45项／0失败（19.428419秒），新 `verification1-result.json` SHA256 `0436917b54e5f681d6db3eaeac773d4fc5671ad54323de2c73356b59ab9ea28f`，不再仅引用父通过。武器提供独立Stereo-open reader01d8b9及Muzzle cadence PG5fdc2f；Root未重复其实现/窄测试。新HJ/j pins `56b6faa00015a25ec08e578bc988637dc76dd1d433b5fb4a9b74ad5278e3d42b`只换三候选身份，其他XR1a059／quiet19b355／EarlyLaunch c12172沿用，coordinator除PG／reader path/hash三字面其余AST相同，8动作／40次6秒＋pose／Stop／106门不变。38脚本／12namespace／4API预检通过；武器v3 consumer54ef375及交付a8f64c、全部源／合同／头／Backend／旧v2哈希核验后才启动。原HI全部pin及原件不变，不重放HI已关会话的未知右恢复命令。

唯一HJ正版0.4 behind／mute／observe-only Game60808／creation134375305491354783、Native3165621632666完成原协议，review complete=true、episodes1、8命令、0错误。肩部首阶段status0→41，4.485336秒；ready41→50，1.075517秒；小移动/转10度50→59，0.8634748秒，三原联合门全过。HandStop4→5／quiescent、Stop后body继续、双控制器恢复及bridge关闭均true，本局无reader失败/66，因此不宣称旧HI66已定位或修复。finalizer仅执行一次accepted=true，只指独立清理：106及完整chunks／Native-entry-PG／XR0／原十文件与设置存档／音频9/9／准确owned及fresh Game-XR-compiler空；首32 handtrace header有效，仍非可见跟随或权威aim验收。`runtime-handoff-summary.json` SHA256 `693a9be13ee40b06112d8aee93d29290ada21cde8c6bc944fcd1b0c4f18613a6`保留准确动作时标及清理边界，PG原日志SHA256 `89e837fbf5c5ddc90d9845c81f52365c97f5d1c96937e7ca08bd5d61b15bac3f`已交武器独占Muzzle分析，窗口正常释放，无第二局。

HJ既有v3 consumer只解析一次，128合法／0错／0invalid，capture true117、seed_model9、initial_current2且实际现有verdict均source_age_expired3。首个已记录capture拒绝index2／tick28110／Compose28111在首phase前28.2227ms，为原!seed.binding.model；phase内首capture拒绝index25／tick30571／Compose30572亦同。原caller明确seed.model来自hand slot asset.address，故此局证实当次seed为0，Context及后继current/metadata/memory门未执行，保持UNKNOWN；尚未解释该producer值为何0，也不回填HI。最早全局记录index0实际是Kernel guard拒绝，不能把seed_model叫所有body首因。首phase内41条不同Tick/Compose实际held提交标记counter2～42，第40条正常original-return end在marker后4.3563774秒，提供本次40/6秒的正证；全日志跨度6.7959321秒、drop2，full-phase coverage仍false，不能声称完整提交总数或全阶段/后Stop每帧覆盖。记录中无Goal10/11，但journal末尾早于post-quiescent baseline，不升级后Stop结论。`capture-subconditions-saved-summary.json` SHA256 `be734728d03ca65192377a4bbbe3cd80a94f45c9d6051539b4bc126fa631a273`保存输入哈希、全部11拒绝、有效性与覆盖边界；若下一步处理剩余capture拒绝，只查当前Tick hand slot asset生产/清空链，不弱化model门或复用旧model。没有默认替换、头显验收或新Gameplay完成加分，LGrip关闭，总体36%。

HJ有限join收尾后，统筹将唯一后继限定为right-ready固定输入下同一骨骼调用的target→实际commit→consumer_pre→returned_end对应。Root准确从HJ710756冻结788源派生独立默认OFF的Source1 ready-entry journal；实际最终source3为790文件，仅1既有live＋2新观察文件，旧first32／Backend／Kernel／solver原样。选择按当前已复制XR输入的full inverse(head)*grip识别(.18,-.30,0,pitch-.3)，1mm／1e-3rad只作测试诊断选择，不作writer准入；同cookie／owner窗口独立64行不轮转，早于captured/pending的已开启窗口调用亦保留UNKNOWN，已知输入更新同一行，明确非ready／cohort变化／容量才结束。每组保存实际身份／Tick／Compose／input seq／reference／basis与独立有效性，三骨35/51/52的commit、consumer_pre及returned_end只在原准确边界被动读/复读，阶段内部metadata自洽，跨阶段pose epoch变化如实保留；gun/Muzzle不读、不拿旧PG补值。最初Host夹具编译命名失败回执保留，生产未因此改变；新114项选择/未知窗检查0失败，未改观测函数的60项真实私有页／SEH通过复用。source1 aa47及source2 597440中间构建未选未跑，最终Native67b513b50626fb426af8afe0212bbc632d9eae12209d4cc4f8d4437f51ff7dc3实际原startup22＋preflight23全45／0，冻结flags及44导出同HJ。候选清单f4e073e5…7d6a5、RWENT001 H128/R2024合同27493ce7…3520与compiler layout已交武器最小消费；合同six doubles文字误差按准确源码/布局的5个标在readiness，不改旧原件。

唯一K/k pins `f0be8eb17d467bafb3a4847c3dec650b4454b9e9520e8e9ea52f40e0baf65f6a`保持PG5fdc2f／reader01d8b9／XR1a059／quiet19b355，只换Native及私有INI diagnostic_ready_wrist=1和实际启用guard；coordinator动作AST、8命令、40/6秒＋pose、Stop、106门不变。新38脚本／12namespace／4API预检通过，武器RWENT consumer821d016及12个必要方法通过交付26a8c3核验后才开始一局正版0.4 behind/mute/observe-only。Game74284／creation134375340335681979、Native3200498446623，原review complete=true／episodes1／8动作／0错误；肩部status1→44用5.1591248秒，ready44→53用0.8514886秒，move53→63用1.1350803秒，三原联合门全过。Stop4→5／quiescent、Stop后body继续、双控制器恢复、bridge关闭均true，无reader66；finalizer仅一次accepted=true只指106及完整chunks／Native-entry-PG／XR0／原十文件与设置存档／音频9/9、准确owned与fresh Game-XR-compiler空，旧32头有效。`runtime-handoff-summary.json` SHA256 `42f14d99a1fa0112571e0d7f17e870410507027cbf2deda58995e56fe6521ce0`封存边界，窗口已释放，无同候选第二局。

新 `body-ready-wrist.bin` 36560字节／18行，SHA256 `b3563225378946723d82e7f91bd15062bc54a02216bba330f04f3dfe648ab28b`，drop/nested/unmatched均0。武器独占一次消费得到parsed f0c0d74…4723b与review fc7cf1c5…9e7a5；Root不重复矩阵解码。18行=17 known-ready＋1 closing非ready，9实际commit（1/3/5/8/10/12/14/16/18）三骨planned→commit→pre→end全bit相同，返回腕目标位置误差2.9047e-5～2.35253e-4原游戏单位、角度2.075e-7～7.461e-7rad，只报数值、不作新阈值或头显验收。另3 commit guard_rejected单位ordinal17/3/9（非骨号）、3 mount0未到target/prepare/commit、3 seed_model捕获拒绝。拒绝3条的consumer_pre三骨与THIS before全bit相同，其后原调用返回才变化，不能把它说成成功写后动画覆盖。完整映射相同的7→8及12→13→14→15→16 target位置/角度delta均0。按原capture_ns落在ready门内的是2～16共15条／7成功；1在marker前1.3549ms（1354900ns，修正原13.549us换算笔误），17/18在ready后move marker前，不能称移动阶段覆盖。Header unavailable293是整个启用期尝试统计，不能当293条ready丢失或故障；未解释分流仍保留。

统筹随后把唯一主点限定为RWENT7／Tick36868／Compose36869／capture320060475570900。Root只对K新BHELD003用既有v3 reader解一次（127合法／0错／drop0），准确匹配index101：两次PG source guard和三次admission全过、mount1/prepare0，最终commit12 unit ordinal17；最后准入Scene age48.0162ms，entry end51.9455ms，而其余保存的tracking／capture／equipment年龄仍在40/50ms内。13/15只作有限交叉，分别49.531→50.6525、48.9518→51.1967ms。已存first-failure实际为更早Tick25546／Compose25547／commit12 unit13，不能借给seq7。冻结Kernel hard失败后不能再次prepare0、唯一重建要求settled_source_age，后继成功支持age-only恢复推断；这仍不是该次序列化的失败verdict，也未区分validate与exchange。`first-commit-rejection-saved-summary.json` SHA256 `521de298d51b9b2c1f5a174a8de4a98966d7569fc492b856027e6a2321e78130`明确分离直接证据、推断和缺失字段，seed_model／mount0不扩成三条调查。

按唯一缺证准备ignored `analysis/ready-wrist-source-rejection-candidate1/`：只在ActualContext已有BodyMainMountSource source-age return且stagecommit，首次保存原rejection_和已计算body-source-age／equipment_fresh事实mask；不再调用current、取时、读游戏或改Kernel／Backend／solver，未走／Hard／非commit保持UNKNOWN。显式RWENT002 H128/R2040，仅在原2024字节后追加两个U64，所有Key／Stage offset与选择、旧60实读函数不变；新23项精确所有权／首次／阶段过滤／OFF／布局检查0失败。CPU构建Native `9250c85a07c270f4513f7db2132abe675f296f55c2cb6520e729af3bff91946f`同冻结flags／44导出，candidate-manifest SHA256 `6005d10fe46a11e2848f09a7763ef48fb29449a17e2178f1ae781b9780ff23fa`，delta合同 `fca9c717500f2abe2b7bbb5da41fe4c2c46cd178182f1cbafcf24a8a0562fbfb`。该候选尚未执行新DLL原45、未请求v2消费或第二局，父67通过仅保留为父历史，不回填K实际verdict；未替换默认、未声称根因修复或稳定跟随完成，LGrip关闭、总体36%。

统筹随后安排准确Native与RWENT002消费器并行准备，以上CPU-only状态保留为当时历史。新Native9250已独立执行原startup22／preflight23共45项，45／45通过，耗时11.8465134秒；没有启动Game／XR或替换默认。Weapon提供v2 file-only reader SHA256 `9923573d99c72408c56b8f59570ec7dd599b6f429c60dd80d8a12fae157dfa00`，delivery SHA256 `feddc65f9e445172a75af3936996183791f182aed45a9c85eae447d2f76fb039`，6项必要方法0失败；Root核对准确合同、源头、布局及交付文件哈希，未重复旧数学套件或重解K。联合准备清单 `analysis/ready-wrist-source-rejection-candidate1/combined-readiness.json` SHA256 `50d766fad9a0b811e96eabd586f661cd44ebafcebce9945d2f855eaef4b7e6d9`关联新45回执与消费器。此结果只确认诊断产物准备完成；K当次失败verdict仍未知，没有第二局或游戏修复通过，整体36%、LGrip正式交互关闭。

统筹授权以准确Native9250／RWENT002跑一次原HK限定场景。HL在出生前被原保护拒绝：派生选择了build1纯DLL目录，缺CaptureSelfTest辅助程序；无Game／XR／Native激活、无输入、原10文件不变，全部失败原件保留于 `analysis/body-held-outcome-runtime-20261027l/`。Root修正为刚通过原45的verification1完整同SHA目录，派生独立HM，保持原HK动作AST、8命令／单episode、三门、Stop和106清理规则；HM pins SHA256 `f1025e1f05f735bdfd48bfc0b300a1e1ea7342dcffa296841fa65e1c8a70c557`。离线38脚本／12 namespace／4 API通过，实际Game26228／creation134375362718608329／Native3222855251894，Native9250、PG5fdc2f59、reader01d8b9db、XR1a059d2d、quiet19b355db保持准确选择。测试始终0.4／behind／observe-only／mute，物理后台控制器输入关闭。

HM原协议complete、1episode、8命令，shoulder4.8872659s状态0→49、ready1.069927s49→58、move0.8493902s58→68，三原门全过；这些是状态观测非全量commit计数。Stop4→5、quiescent、body继续、左右控制器恢复及bridge关闭；正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XR0／原10文件／audio9恢复9全部通过，fresh Game/XR/compiler为空。`analysis/body-held-outcome-runtime-20261027m/runtime-handoff-summary.json` SHA256 `8c5aa80bd60cc23fecb3ca5b6bfdab6a16607f70fd99c49e6bb6d626c077a38c`；PG日志SHA256 `0dd927d78addbd03f06ad6c4397105389d9d73945205778883bbd88f1e543a52`。HL失败不计覆盖，HM清理通过仍不等于稳定跟随验收。

新RWENT002原件SHA256 `015902a9217c2085837209d8d381695a630568794fe5c805886770df394f4b1b`，H128/R2040、23行／drop-nested-unmatched全0，Weapon唯一一次v2消费parsed SHA256 `237343af2fe523b7a5fd332c4b904722306ff3d321f638198ed63b3348f49132`，窄review SHA256 `a0dcdc545a27e4acc8f1911588ac7e64c8c2b446a373a4d2212986c25cd96382`，Root复用不重复矩阵解码。23=11成功（10保持、1变化）／5commit12／6mount0／1seed_model；五条commit12为6/8/12/16/23，实际尾均verdict3/causes1，即已有body-source判定source-age-only，未置equipment_fresh=false位，其他18尾UNKNOWN不推通过或Hard。首seq6 Tick36468／Compose36469／epoch958／unit ordinal6；本局BHELD003仅解一次，128合法／0错但容量drop5，五条仍全部精确身份／capture／tick／compose匹配，不能把RWENT无drop套到BHELD。`actual-commit-source-return-summary.json` SHA256 `729741b4bca865e73e6b39147ccbc762b1bfa9e0fbada361f256b50436e52e41`封实际尾与exact join；未定位更深age子条件或validate-vs-exchange，未回填K。

HM成功seq13新增独立差异：Tick37217／Compose37218／epoch965，准确ready capture322296336150400，prepare0／commit1、三stage status1／epoch965、CTW相同、normal original return及scope close；planned→commit→pre三Local逐bit一致，pre→returned_end仅Local35变化（角度1.218694751rad／平移2.76401076e-5原游戏单位），51/52不变；末腕world误差17.056824296单位／0.569835610rad。Root只审冻结源码顺序：returned_end先于THIS Kernel.finish，故其后的finish／rollback不能造成已采变化；原invoke返回到快照之间仍有finish_guarded／control_end／recoil_end／fresh_end／Goal资格读取，当前不能归具体原函数或写者。`successful-path-source-boundary-audit.json` SHA256 `52c7d98fcdf64fcd9c7e35411d5d1f900b8ab41134dffd4bc09d93124906d668`明确修正“during original call”只能指整个pre→returned_end观察区间。实际ready覆盖seq2～21，1在前、22/23在后（23closing），不称肩或移动覆盖；Header unavailable302是整个启用期尝试，不作ready故障计数。没有第二次实际游戏、保护放宽、默认替换或头显验收；LGrip仍OFF、整体36%。

HM seq13继续只查现有证据与冻结源码。Root仅解析本局ARMALP01的Header和32条记录key，实际Tick25768～32081，不含37217／37218，未解事件或借相邻帧；`alpha-first32-coverage-boundary.json` SHA256 `42fbffb55652f38e246473276cd71c711a02269e265fafed308ed4652b9c1c78`保留有限范围。逐个审查end callback主体，未在这些主体发现Local35 store；此有限静态阅读不等于动态写者跟踪或排除并发写者。现有证据仍缺raw original正常返回后、所有finally/end callbacks之前的独立快照，不能直接命名动画、原函数内部写者或rollback。

按统筹确认的唯一新主点，准备ignored `analysis/ready-wrist-raw-return-candidate1/`：在准确9250冻结790源文件上只改五文件，原RawInvoke正常返回后增加一次同严格metadata／Local／Space重复读取，保留原returned_end，原2040-byte字段前缀、入门／时效／source guards／Kernel／Backend／solver／选择均不变。显式RWENT003 H128/R2368/cap64，仅append raw_return Stage@2040／328字节；原抛出、非同entry／GT／kind0或未捕获不提供新边界成功证明，缺失UNKNOWN，旧K／HM不补字段。新增22项阶段／归属／OFF／首次／UNKNOWN／布局检查通过，fixture首次误用translation.x编译失败原回执保留，只修fixture为数组[0]；Native源码未随fixture修改。旧读取函数与Stage布局精确复用，不重复旧矩阵套件。

新Native SHA256 `aaccfc1cec777751c687ad38190b73b5d0d244f3f7d1873ba5feec11a5f1ddef`保持冻结flags／44导出，准确新DLL已独立原startup22＋preflight23全部45／45通过，11.8046176秒。候选清单选辅助程序齐全的verification1目录，SHA256 `cb6a10f21412dc39e5ec6248b76f1240b4515d962cb3e6652c97b246a65226c6`；RWENT003合同SHA256 `671b56da794fa3238e04af21abd120d91f799006370ea194874544ddc35c3c42`，源Header SHA256 `89745a2ef6d6617857f528b0e98743376635c4dc503ceb85ad82f1039a322051`。已直接交Weapon窄file-only v3适配，与准确DLL检查并行，无新游戏或RE。该候选只准备相邻边界诊断，不声称HM变化写者已定位，不扩body-source更深age主线、不放宽门或改默认；LGrip OFF、整体36%。

Weapon RWENT003 file-only适配交付reader SHA256 `235875fca7e1d47eb858e76225607068be69a9b466d6b9d5686ebfeeb4c42122`，delivery SHA256 `6fabad75c421d46b7f017a13a62707cbe097fe61295e463fb036078adbed558e`，7必要方法0失败；v1/v2合成兼容不改旧结果、不重跑旧矩阵套件或重解K/HM。Root核准确合同／源Header／compiler／全部交付hash，联合准备 `analysis/ready-wrist-raw-return-candidate1/combined-readiness.json` SHA256 `5342c40e59ed9d191403f3958ca55350cbec5818e7eff2c2521368bb2e904570`。按既有授权从冻结HK派生独立HN，pins SHA256 `7f957200c8d9989429eef7f9b183108bddbb9d871bc9dda7be3661401d80a521`，离线38脚本／12 namespace／4 API通过；选准确aaccfc1c及完整verification1目录，原PG／XR／live reader／quiet与8指令、三门、Stop、106恢复、0.4／后台／observe-only／静音恢复／物理后台输入关闭保持。

HN唯一这轮新边界场景Game59204／creation134375372787792949／Native3232920772391。原8命令／1episode／三门全过，shoulder4.9771591s状态0→45、ready0.9037463s45→54、move1.0850167s54→64（仅状态非全量commit数）；Stop4→5／quiescent／body继续／bothrestore／bridgeclose，正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XR0／原10文件及audio恢复、freshGameXRcompilersempty全部通过。runtime-handoff-summary SHA256 `879254c2df97970a72c7cba3f71c8bf888d4d609d70fc17aba42af515ba0b8fa`，PG日志SHA256 `1668c76a6c1f2248af4ab24f0a35964c0574abd0fb98703e2d73bdb4a1c991b5`。新RWENT003原件SHA256 `8cda052cad78930cf0f3d49132e91c1f60a43a1bc6ed982929fd31e09a33afb7`，19行／drop-nested-unmatched全0；Header unavailable332为整个启用期统计，不能当ready故障数。

Weapon仅一次v3消费parsed SHA256 `b153ecc0f96b586bdc49dbaecb0ec8c7318726be54b6c318e5e5fb82e360f72b`、review SHA256 `9b6d2501a04c9b8d869ab59d55eddf6f08a6c90e2d5d85fd32adce19b7a1d407`，Root复用未重复矩阵解码。19=9成功（8保持、1变化）／2commit拒绝／6mount0／2seed capture；17个raw status1，均raw→end三Local及采到的Space／CTW保持，2capture失败rawUNKNOWN。成功seq13再次出现目标差异：Tick41283／Compose41284／thread29720／mesh5309084336／epoch947、valid127／prepare0／commit1／finish4／正常原返回和scope close；commit/pre/raw/end全status1同epoch和CTW，planned→commit→pre一致，pre→raw仅Local35变（1.1438664598118258rad／1.9913304345913984e-5原游戏单位），51/52保持；raw→end三Local逐bit保持，raw/end腕目标误差完全相同16.77294139841474单位／0.571302645061472rad。capture323303988349600位于准确ready；capturepoint1..17在ready，18/19在其后move前（19closing），肩／移动不覆盖。

本单可验证边界已收窄为consumer_pre→正常raw original返回采样：该Local35变化在后续finally/end callbacks及THIS Kernel.finish之前已观察到，后者不能解释已采变化；仍无具体原指令写入trace、并发排除或旧HM/K的回溯raw证据。`analysis/body-held-outcome-runtime-20261027n/successful-path-pre-to-raw-boundary-summary.json` SHA256 `82c1887ff38562044da10d3af1dd2eea53beec27d256d39aeb3332384af1a6e2`封准确身份／三界原快照／数值／phase与源码链。不将9成功总体称稳定、不循环重跑、不扩body-source更深age或PG/Muzzle，不放宽保护／不改默认；物理头显验收、LGrip正式交互仍未完成，LGrip OFF／整体36%。

统筹下一单限定为HN kind0 raw路径内首个能解释Local35变化的写入点。Root先查 `tools/re-kb/kb.py` 的artifact1函数128920与入边，复用现有完整函数伪代码／机械IDA导出；没有新开IDA、启动游戏、安装或迁移工具。核验历史code blob911ab9、保存导出的C0 main4b65／cold d4a5整块，并与HN正常清理后实际整块hash一致，严格区分历史运行快照、零售磁盘EXE和当前代码一致性；没有把磁盘加密字节或同RVA直接当作一致证明。`analysis/ready-wrist-writer-localization-20261027/kind0-local35-writer-candidates.json` SHA256 `c477e7e0d6994a369e4ec2b68eacb934ded0697a731dff335065ab9471531ae3`保存调用／数据流、具体候选和局限。

最窄具体caller Local写入候选为RVA129419的整32字节骨骼替换，以及分数权重路径129893起的首个Q分量写入（该路径T／scale从129837开始）。原RDX是Local-header，128984保存／12915e重载，受影响bone index左移5后定位Local atom；既有230／238／240虚调用及129119的278权重返回，经过12936c栈Atom转换到caller写回。相邻mesh+26c写入的是Space，Backend认证Local/Space ranges disjoint；单独Space写回或scale-only不能解释本次Local35 Q变化。旧AtomOverwriteReceipt认证129cc0／上游e0a6e，不能作为这些store执行证人。此次异常帧的真实node／affectedlist／finalalpha缺失，不能选定整替换或分数支路，也未排除前面的间接调用或并发写者；未形成最小修复依据。源码rig明确right hand_bone35／chain32,34,35，已向用户纠正此前口头“上臂”为右手／手腕，原身份／数值不变。

唯一可区分验证动作已具体化：在原同entry arm-alpha诊断的ready窗口保存已发生的compute／weight／owner／applied POD、最终返回alpha与affected/output identity，以及该权重边界的一份严格Local35快照。若此处已变化，追前面的间接调用；若此处仍planned而非零finalalpha之后raw变，缩到后续caller替换／混合family；零值、缺失或非法仍UNKNOWN。没有新中函数detour、重新调用guard/current、刷新时钟、写游戏pose或放宽保护。现有首32 alpha不含异常entry，不能借用。另按真实指令位移机械核验常量地址是17070d4/d8（IDA文本错标17060xx）；历史数据段c5deb081中相应值约0.9999899864与0.00001，只是历史值，不能以代码相同推当前数据相同。验证需保存这两个实际比较常量raw4或已有认证值，不能只凭alpha推当前支路。`historical-weight-thresholds-and-validation-detail.json` SHA256 `6992833491aff7ffd3303d1202deedab4ca67f6e4a4629c9c665b619ef6ac358`保留该限定。本单没有新Game／IDA／运行修复或泛审；具体写者仍待测，LGrip OFF、整体36%。

按统筹确认的唯一后继准备 `analysis/ready-wrist-weight-boundary-candidate1/`：在准确aaccfc1c冻结790文件上仅改五文件，保存现有arm-alpha POD在ready窗口的同entry事件，并在既有finish_guarded仲裁／catch之后复制实际返回packet低32位alpha、已取得的compute/node/root/output/affected身份。原arbitrate_arm_alpha函数逐字保持，旧first32事件照常保存，无新detour／guard/current重调／alpha或pose写；原严格Stage读取函数和raw_return读取、Kernel／Backend／solver／body-source guards／选择保持。新增只读观察采原Local35/51/52（Space不提供）及实际base+17070d4/d8两raw4，失败有独立mask／UNKNOWN，不让诊断值进入保护或求解。

显式RWENT004 H128/R16416/cap64，原2368-byte v3前缀及所有offset保持；append四count/drop字段、64个原Diagnostic144和8个WeightBoundary600，每界包含已取得WeightReturn248／原Stage328／常量mask与raw值。source初次锚点把原函数参数rsp误写entry_rsp而停止的未完成源保留source-attempt1，未编译或运行，修锚点后新完整源可逆核对通过。首轮28项针对检查通过；补常量mask2／全缺两个支路后30断言0失败、四编译／运行child全exit0，driver仅在独占保存已存在layout时失败，原失败回执保留；独立确认新layout输出等于已封layout后以 `checks-attempt2-completed.json` SHA256 `bacc7167837f1dd436683d0cf779f2f6b444e2be7f1b22f19bbdd828dcda8a4f`完成证据保存，不重跑child、不改Native。准确新Native SHA256 `a9fcaf049f3720d6ee6a6a8c1f0ceef4c91551bf5249552cd1eae135306e218b`同原flags／44导出，原startup22/preflight23共45／45通过12.447511秒，完整verification1 helper目录作为所选工件；候选清单SHA256 `41366b2345641ee43001a76e270e85bf74d1f909fc6b96b2865528af816ef263`。

RWENT004合同SHA256 `31797a2e5847c8090833ebe76c56b07db7c254a52856a8611c5259a44194a89d`、Header SHA256 `bea00ea83861a2cbade41ac99bb6d835572cfa40cc75055ff0d60bdda893e850`已直接交Weapon窄v4 file-only适配，与准确DLL检查并行。Weapon询问阈值等号后，Root仅复用保存ASM明确有限alpha>zero进入此writegroup、alpha>=full为整替换候选、zero<alpha<full为fractional候选；还须匹配原Diagnostic.sample stage4/reason0、before/after flag_read均有效且cached_scale_word两次观测0，否则R12D非零旁支或缺证不能套两候选。原合同不改，追加只收窄解释的 `RWENT004-comparison-supplement.json` SHA256 `6c2e5ed6c95955d9e231f39baa2ab0c459fd8af34318a802342af3fdfb029ec9`；zero/invalid/missing仍候选UNKNOWN，实际0事实保留，不能排除间接／并发或把两store当穷尽。尚无新Game、ABI消费者未完成；HO独立派生pins `fcc5d2c670b7e67dcc685922947c7694a256727f25c76521ad95d8da799c7f0b`，38脚本／12 namespace／4 API离线预检通过，原8命令／三门／Stop／106恢复／0.4后台静音恢复规则保持。具体写者及修复仍未证实，LGrip OFF、整体36%。

Weapon v4适配交付reader SHA256 `3d9ce0cb014a03f0b94daa7fbf6ef2fad9f704b3b8f053558322467554bfa8b4`、delivery SHA256 `e299b52b49b79f670df94ad41a84c992457f7d553e7bbe06f3bf210588e562f4`，最终12必要方法0失败；合同31797、比较补充6c2e与实际源／ABI／交付文件hash已核，旧v1-v3合成兼容、K/HM/HN不重解不回填。联合准备 `analysis/ready-wrist-weight-boundary-candidate1/combined-readiness.json` SHA256 `df6c31a60f6e0addb6ce1fc7684b7de68fe15119f6c273d7d453bede06396487`。由Root唯一运行HO新weight界场景，保持原流程／门槛／0.4／后台／observe-only／静音恢复与物理后台输入关闭，没有中函数hook或通过改alpha／pose试错。

HO目标未达：Game41768／creation134375409062195917／Native3269225923034，shoulder原门6.0073855秒status0→0／无coherent joint pose，protocol complete=false，1episode／实际6命令，ready和move未发送。Stop4→5与hand quiescent、左右控制器恢复／bridgeclose已证；因没有合格held baseline，body continues-after-Stop未获证明，不能以cleanup代替。正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XR0／原10文件及audio恢复／freshGameXRcompilersempty通过。runtime-handoff-summary SHA256 `f7f0b199ead80e464c1b63559fedd2f21ecd86ecbc1177d258934753982615a7`、PG日志SHA256 `32495f34ba2e0cfc01f0ec52a543704360cbf175fbcb29322b2a310c97fa2cdd`；清理通过与场景失败明确分开。

新RWENT004只有128B／count0／selected0／window_cookie0，SHA256 `7a30ff5d5808e54f34fc2ccc46e6c78171740e1ac48afb84d5abf0ba78d7b75c`，Weapon仅消费一次exit0，parsed SHA256 `dffc595115fdffc491ab83a5cc6ac06ec40fcf2f9519c8f16c757afe003d7bc9`、empty review SHA256 `b3c1632ec215b146eb9a74a6710c5cb64895de10b41e61a50414740536c158a6`。无alpha／weight目标覆盖，不能称目标未变化、无自然callback、collector运行通过或新Native回归；启用期nonmatching168／unavailable491不当ready丢帧。旧hand first32是count0，finalizer header_valid=false源于要求32条的接受条件，未称格式损坏或32条通过；BHELD003仅H120／count0、budget仅80B、first-failure文件未生成。

Root只核本局已存聚合与冻结源：body_goal_plans/applied90，body_hands_attempts90／solved0／commits0／status8(unreachable)90；main_mount_attempts/not_ready均90，last_query0／12个query branch全0，PG两query summary也0。说明没有进入实际mount查询，失败范围在select_main_mount/select_body_mount的query前共享not_ready出口；marker/basis/eye转换、cookie、BodySource policy／parent registration／current、revision/generation中的first-false尚未序列化，不能由聚合选其中一个或宣布因a9诊断而退化。BodySource450/451 acquired、age_rejected0不作逐call准入证明。`analysis/body-held-outcome-runtime-20261027o/empty-target-prequery-boundary-summary.json` SHA256 `20f57c83d9d26ece9ed60db0481996f316f8ec2c36363109dd113c1f1131e5f7`封空目标／原肩门失败／正常恢复和缺证边界。未为补覆盖再开局、未放宽门、未扩PG/Muzzle或深层age、未改默认；本单权重界前后仍UNKNOWN，LGrip OFF、整体36%。

2026-10-27后续时段更正：HO原聚合90是较早累计值，不能称肩阶段仍在生成／应用计划。原before→after hands及main_mount attempts均90→90，Goal plans/applied90→90，Goal attempts300→451／rejected210→361，Native captures299→450；新增151次在Goal计划之前拒绝。更早90次query前not_ready不能解释后面151次。原证据报告保持，追加 `analysis/body-held-outcome-runtime-20261027o/phase-counter-erratum.json` SHA256 `4f7f0ae3dbe1491fe861fd4f8e4bd7ced3174eadaa8b4a17f04e7be6a31965d2`；下一单明确记录实际query前first-false，不混用两个时段。

在准确a9fcaf04冻结源上完成两文件首拒绝观察增量 `analysis/body-prequery-first-reject-candidate1/`，790项源清单其余保持；不重复调用eye_from_head、registration_current或context.current，保持短路顺序／缓存拒绝值／原保护和写入语义。BHELD004 Header128／Record648／cap128保留旧120／600前缀，追加arm_kind及六个prequery字段。kind2用原entry.begin触发首个真实not_ready的7秒有限窗，原PG-ready kind1保持；不把窗口当准入或完整shoulder覆盖。另在原cookie0立即返回处，仅测试诊断模式保存一次非权威gate20回执：cookie保持0、begin/end0为未观测、不取新clock、不arm，原active=false及here/exact失败保持，不能补造身份。针对97断言／四child零失败，准确新DLL原startup22＋preflight23共45／45通过12.1889275秒，原flags／44导出不变；早期fixture失败保留。准确Native `verification1/GearsCaptureCoverPreQuery1.dll` SHA256 `a3388eef6edc73019882d54942061a28652ab4c3f13ff78b4523220123282aa2`，使用完整verification1辅助目录；candidate manifest SHA256 `fee490439cbc857560046d1b151c539847eb52b5218bfc53bb26bcef79ddeca1`，合同 `05070c3367fd5ad483f339a0e059e46aa3259561cc0b4e81a0b19e8eb2895261`，联合准备 `6abf95dc7c4a9ab99ab2ea75a5ba79f3914c778ab57d8d46e41adb0456b3cac9`。Goal policy和RWENT004权重观察保持原字节／ABI。

Weapon窄BHELD004消费者SHA256 `1e0c7d3037378a29fbb8a052f80d72c2552646a8e82da01b96d0ae854ffaf0ef`及delivery `27a2f8b47fb5bb6032adc5184d2668a1aaac1d3292d41bf913a98d7300e58ea8`直接交接，与Native必要检查并行；11必要方法通过，cookie0／kind2和原索引分列，v3合成输出保持，不把新window伪装成旧PG-ready。Root核对准确合同／布局／工件，不重复解码。由冻结HK直接派生独立HP，pins `9e667914a5753a18771219f8331394e20ddc1e035746e4baf3f56ffa8e9a4766`；离线38脚本／12namespace／4API通过，原8指令／三门／Stop谓词／106恢复／0.4后台置后／observe-only／测试静音恢复及物理后台输入关闭保持。

HP Game33264／creation134375449427094269／Native3309563927982：原shoulder门4.3257423秒状态0→42，ready0.8369653秒42→52，move1.0705665秒52→62，8指令／1episode／errors空，三门均通过；状态增量不称整阶段commit数。Stop4→5／hand quiescent／双控制器恢复／bridgeclose已证，但原3秒续更谓词超时 `TimeoutError('No post-quiescent continuing free-hand commits / stable held counter')`，protocol complete=false，body_continues_after_stop未通过。正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XRexit0／原10文件及audio恢复／freshGameXRcompilersempty通过；清理不替代续更。runtime-handoff-summary SHA256 `a5eb04ac78135037006af8457b348523ebde8db13912e81e427a5c9a617abb0d`。

HP BHELD004原件SHA256 `e625cebbba20a970fe793dbcf52d541594942354837b4a2e29385f5ef795b329`，Weapon仅消费一次，parsed `84002ceba60dec3374d1bbaf102f3ee5d15bc7acbfb66bea9d76da38f92ccd8e`／review `fbe2455744028b4e5b30f43fc6095f3478d82897919065b959b4e41db32d8dfb`，128raw＝127普通＋1独立gate20，invalid均0，kind2／capacity drop6。首普通raw1／Tick27266／Compose27267为gate12、cookie1／parent0，在肩marker之前583140700ns，不能代解释HO或肩期。117条prequery通过，7条未观测，2条实际gate14/detail3分别在肩／move；独立ready raw98／Tick37748／Compose37749是Goal reason14／adapter2，19keybits同而prequery UNKNOWN，不借别行原因。有限窗末尾比move_end早298166800ns，原行held markers不当全量提交。

按第37轮用同一HP现成权重原件唯一消费，不重开局：RWENT004原件 `a703a45902ee62bfa97cdc99d1f22919784b1129fdb9c19222007a727bfe402f`，parsed `401da0db3dde210dbaf9dca3f0e73c95e7aa8a0ec31d8a9f8fb93871ce2c2c23`／review `fa760a0529f74f5dc5cfc113a12bab3e0b9eb919e40bc6017c64a1d783cc5462`。9成功entry各两个合格end35 weight，三Local pre→eachweight→raw→end均逐bit保持；18个合格右腕事件旧sample4为1，但最终实际packet raw4均+0，不能用旧sample当最终非零alpha，整替换／fractional归因UNKNOWN。本局未复现HN成功后变化，不能宣布修复或排除并发。总18entry／264alpha／64weight、相应drop均0，与BHELD drop6分列；seq2..17在ready，seq1早于marker24.8424ms，seq18是ready后move前closing，无肩／move权重覆盖。

Root并行只核既存HP Stop记录及冻结源，封 `analysis/body-held-outcome-runtime-20261027p/stop-continuation-existing-evidence-review.json` SHA256 `0e1e2097d9d3bc70f723a0e8a993ca33c3a18d4c4af046f84e539599c6326f44`。post-quiescent baseline330969462935700、cookie0，hands commits68／held66／not_ready260；后续11个独立保存样本至＋2456288500ns这些计数和hands attempts340均不增，Goal attempts407→493／rejected83→169、Native captures402→488，plans380／applied324保持。最终既存status仍hands68／held66／not_ready260，Goal attempts534／rejected210／plans380。因此并非仅缺记录或held计数继续跑，已见Goal计划前阻塞；完整3秒内每次poll未保存，最后status无独立采样时标，保留覆盖界限。

首实际Goal拒绝reason10 key_mismatch／Tick42131／Compose42132／QPC3309689141744@10MHz，在该baseline前548761300ns；origin initialized且poisoned1，observed stream／reference／head均1→2，其余记录的world／XRsession／space／reference generation／native revision及epoch保持。冻结goal policy L136-147把这些observed计数纳入neutral原点key，L169-171遇mismatch永久拒绝，未有恢复事务；tracking的fail／duplicate-loss／stream-key／reference-revision都可能增epoch，现存记录不能唯一选触发点，不能声称Stop导致或所有后续拒绝同因。最窄后继是审当前完整fresh身份与硬参考坐标不变时的neutral标定恢复契约，不重采已改写骨架、不简单清poison／跳过epoch。一个只读Astra助手仅审该具体难点，唯一Native实施者收到结论后选择一条最小恢复路径；没有新采集器／源改动／构建／Game／默认集成。LGrip OFF、整体36%，真机候选尚未就绪。


2026-10-27据HP实际Stop阻碍，完成单文件 `analysis/body-neutral-observed-rebind-candidate1/`：790冻结源仅body_c0_goal_policy_live.inl变化。hard key保留完整actor/weapon/asset/Native身份、XR session/space、独立reference generation/Native revision/epoch/flags/pose、units/model及完整参考骨骼字节；same_key正常执行／发布仍严格。只有已初始化且未poison的原点、观测stream/reference/head非零单调增长、无policy或Native债／失败／stopping／exception／installed atom时，才尝试保留原Stored/Marker，并在原move、adapter、receipt、两current和standing通过后先invalidate旧view再仅接续这三个观测字段；不清已有poison、不重采已改写Local、不更新源timestamp、不改tracking epoch或Kernel独立时效恢复。accepted rebind计数不是commit数。一个Astra/high助手仅审具体恢复契约及一次实际差分，未发现可行动错误。

实际提取的策略函数＋原typed Binding比较，使用明确Host current/admission/adapter/Kernel seams，132断言0失败；检查硬身份／参考变化、zero/regression、已有poison、全部债／失败状态、先后guard失败、迟来fresh重试、重复失联及apply拒绝，Stored/Marker与其他source字节保持。前两夹具编译失败（Windows宏／枚举、空Call警告）保留，Native源未因之改变。准确Native `851d50fc11cbfce3052fa66e0e2680a8404d429e5c540b4e7e037f2ca75c9da7`原flags／44exports构建通过，原45／45通过12.2156142秒；candidate manifest `768465ac84a7d76f395630e24e67409e89f476e0bd6583ba5c9139e6e1be6634`。

HQ独立一变化场景（`analysis/body-held-outcome-runtime-20261027q/`，Game74204／Native3327371033689）原三个门全过，Stop5、cookie0后hands97→134／not_ready277→314、held71→71，约1.0103586秒后实际+37 free-hand提交，protocol complete=true。正常finalizer仅一次accepted，106完整块／Native-entry-PG／XR0／原10及audio恢复，freshGameXRcompilers为空。handoff `d1feda2dc989547e7208bd99a377e20eb5c3bd5b6b27acff51f9fc65d07576dc`。但rebind attempts/deferred/accepted均0，本局未复现HP观测边界，严格只算该流程非回归，不能宣布新恢复分支实跑或HP已修复；未重解本局权重／PG-Muzzle，也不原样重刷。

后继限定为一个可区分的controller-only失联／恢复输入，HMD及硬参考保持，仍需实际XR API flags、Native observed变更、接续后持续提交及原Stop／恢复共同证明。启动前发现依赖清单保留的atomic9ab不是runtime-slot实际选择的CY378e；基于旧atomic的24源重构与631d模拟器工件保留在 `analysis/body-controller-loss-stimulus-candidate1/`，明确未选／未加载，不能挪作CY增量。其机械脚本／夹具失败、整数JSON accessor误将0.5截成0及后续修正均保留；没有更改共享或默认模拟器。

准确后继 `analysis/body-controller-loss-cy-candidate1/` 直接核实际CY handoff/source_pins的25文件、所选DLL378e及原构建，完整冻结清单一致，仅mcp_integration/runtime两文件加exact-env testTracking0/1与原一次性ack的可选确认字段；缺省／未启用／非法值保持原状态，原grip Apply helper和窗口／render路径保持。实际命令／ack 57断言及原6／6（含588 grip）通过，同CY flags/exports，新独立runtime `7c7bf28141003ce99fec1e01d7a7f06c64d839dfb384daf0094e18e9965eaf28`，Native851d／XR exe／PG／消费者不重建。HR增加两条受控输入，共最多10命令，实际API观察后才核ack避免消费/确认竞态；原完整Stop/finally逐字保持，0.4／behind／observe-only／测试静音恢复／物理后台输入关闭／私人存档保持。准备脚本曾因encoding／路径分隔／命名空间及重复pin更新停止，失败保留，最终38脚本／12namespace／4API离线预检通过；HR实跑结果另记，不能将准备通过算分支覆盖。LGrip OFF、整体36%，默认未集成、头显验收未完成。


HR唯一受控变化场景已完成：`analysis/body-held-outcome-runtime-20261027r/`，Game55896／Native3346184197844；所选Native851d、准确CY派生runtime7c7b，10命令。Source1实际右grip active1、flags15→0→15、sequence递增，HMD pose／XR session334562897742500／space1保持；Native observed stream/reference/head1→2，独立reference epoch2／flags1／failed0保持。rebind attempts1／deferred0／accepted1，实际触及新恢复策略，原三门4.6759645／1.0734295／0.8448924秒均过，protocol complete=true。

严格续更证据以首个已经包含rebind1的保存样本为起点：hands72／Goal applied327→post-quiescent hands102／applied357，后继再+30；该区间held69→71、not_ready258→287。之后原Stop4→5、cookie0基线hands102／held71／not_ready287→hands139／held71／not_ready324，自由双手及Goal applied再+37而held稳定。失联前到首个接受样本的总+6不全部归因恢复后提交；lost Native保存状态仍1，真实observed2在后继样本确认。两者均在正式窄review明列，不能只以命令ack或rebind计数称通过。

正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XR0／原10及audio恢复、freshGameXRcompilersempty全部通过。首次封摘要因未生成标准case-selection.json而停止在写目标之前；保留该错误后仅从原冻结pins／prebirth选择补元数据再封摘要，没有重跑finalizer或游戏。summary SHA256 `69f9659e23a655553eddb061434e9cc71d7d08b0f4b6796fd4b5dacea6cf57f0`；正式 `controller-loss-rebind-game-review.json` SHA256 `8c7b0d28e3db632e9527655048cd92f719996215c002c891b37e81a17b614f85`。最终真实coordinator69cfa的差分／索引 `controller-stimulus-coordinator-final-index.json` SHA256 `e9c460635be4f5e3857ebb0e838fd81a17d89fdbb7b7578795c4b2fca1c07e86`替代旧中间索引的解释用途，旧文件保持，原Stop/finally逐字核对保持。

Native候选追加独立 `analysis/body-neutral-observed-rebind-candidate1/validated-handoff.json`，保留先前离线candidate manifest不回写。该结果证明本地受控controller失联／恢复后的neutral接续和持续提交，历史HP到底哪条自然publication guard触发仍未知，也不提供已有hard poison对象的恢复、Native独立age failure的豁免、稳定武器跟随／射击或物理头显验收。准确851d已交Weapon及统筹作为后续联合6DoF Native候选，未新解RWENT／BHELD／矩阵／PG-Muzzle、不改默认support3／menu-modal1或发布工件。LGrip OFF、整体36%，下一步回到连续持枪联合目标，不继续原样重刷该已通过恢复场景。


第39轮直接回到W2连续持枪：HS `analysis/body-held-outcome-runtime-20261027s/` 复用Native851d／XR1a059／PG5fdc／reader01d8／实际CY378e，未重建核心、未重刷HR失联。原取Lancer／shoulder通过；ready实际命令已到，原Native held41→89且coherent body存在，26个不同pose_epoch；新增gun component配对条件未满足：全局0／48（未标记4／shoulder18／ready26；全局47次pose changed＋1次metadata closing changed），ready自身0／26（25次pose changed＋1次metadata closing changed）。后续平移／旋转未发送，不能报跟随失败或通过；7指令、protocol false，原Stop续更、双控制器／bridgeclose及正常finalizer仅一次accepted／106完整块／XR0／原10／audio恢复通过，freshGameXRcompilersempty。summary `961ec3f4b30d788d40ddcafb5759d98a3f073db561a4714f4f9fdfd383cf244c`；精确首阻碍 `continuous-follow-first-obstacle-review.json` SHA256 `2b4f3b0109939fac6368625698f9a45ae0a2d55dc9d86c87bf6cc3f2c6ed6a80`。继承通用summary将整个扩展ready门标original_gate_passed=false，窄review明确其原Native提交／coherent条件其实已满足，失败仅是新增配对观察门；原summary保留。

已直接向Weapon下单修该唯一首阻碍：沿现有pair_probe将昂贵typed属性／附件／参考发现与闭合移出短动态body＋gun读取窗口，保留完整身份／参考／epoch／字节与前后校验，不暂停游戏或放宽一致性；Root不并行改该reader，不新开助手／核心构建／旧案解析。同时Root准备下一变化场景在原Native门满足时即请求现有私有both-eye projection图像，使配对拒绝也不压掉可取得视觉证据；原完整Stop/finally、45s窗口和最多11动作命令保持。等待具体新reader工件期间不原样重跑HS、不转逐枪／掩体扩展；目标仍是实际平移和正常角度旋转后的持续手／枪跟随。LGrip OFF，整体36%。


HT已触达W2全部动作：复用Native851d／XR／PG／CY，唯一改用Weapon窄pair窗口模块4b473368（manifest e78e71f3；22必要CPU方法通过，不重建核心）。原shoulder／ready／6cm纯平移／+10deg小移／同位置-10deg／回ready均完成，11命令、protocol true。新reader取得62obs中48accepted／14拒绝（11 pose close、3 metadata）；明确5.4491..10.2156ms动态窗及其后完整慢关闭，仍非同C0或最终render证明。原Stop hands177→204、held151→151、not_ready254→281，正常finalizer仅一次accepted／106完整块／Native-entry-PG／XR0／原10及audio恢复，freshGameXRcompilersempty通过。summary `4775c65ef5646a8a85c6e4d077dbcea15ce7b0af45f0649265e72126d932e8eb`，新joint JSON `e6133c067807eeafc6d6286be2f85fe2c48b022215fd82fbcfb43f945d9e52d9`。

Weapon只消费本局已存JSON的连续跟随review `dcbd1f5eaae3295dd8c2382c40a3a93e297a8844c7f468cbaefbc224e483a83c`：完整window包含原phase的配对为shoulder12／ready3／6cm6／+10三／-10三／return5；原门完成时return4与最终包含统计分列，全48都保留。静态weapon／component／asset／unique attachment原字节及相对参数、AvatarCTW均同一。固定marker/Avatar下obs4 epoch1279→obs5 epoch1284已有右腕31.461657u／34.2787deg、左腕25.299966u跳变；第一处直接gun/end大差obs14 epoch1336 shoulder为31.002087u，另obs21 epoch1378 ready为16.645682u。ready／+10／return有两姿态族，6cm六样本没有干净位移证明，不能只筛20deg旋转的好样本称持续通过；左输入固定且squeeze0，不强求跟右同移，漂移仍列出。缺实际目标／basis证人时不编绝对UE预期，不把附件不变等同同C0／脱附／某写者／最终画面。W2持续跟随尚未通过。

五次原post-request截图门超时并非未产图：只保留最后both-eye BMP1920x315及status，capturedFrame6318／request6319，为请求前最近完成帧，不能称postrequest fresh；旧阶段BMP已覆盖。Root view_image观察到两眼场景地面与大面积近处肉色几何遮挡，手／枪无法可靠辨认，不凭像素认定具体head mesh、上传故障或五阶段显示失败。`stereo-images/visual-capture-review.json` SHA256 `97b7d8346e8cf199eeabc0db6f8f0b85d2375f22afb47817528d725f032aa1f3`。仅准备现有capture_stereo的至多两次顺序请求修正：保留首幅最近完成图和初始requestFrame，第二幅必须跨过初始frame才称fresh；不覆盖pending／丢拒绝图、不改renderer或窗口。该实际函数的6个文件／时序Host检查通过0.025秒，尚无新runtime验收，留给下一真正变化场景，不为截图原样重刷。

沿首实际不一致继续，而非停在交接：HT新RWENT004原件 `4fe331a8ec3c792fe07a54128babe5271fc74271d16878a7ea08bc69ad51b5ab`，64saved／selected65／drop1；仅交Weapon一次冻结reader3d9消费，精确关联mesh／epoch1336和1378／pair窗口到现有pre-weight-raw-end/Space/CTW，不借邻epoch或重解HP／HR，不泛查age／PG-Muzzle。初步覆盖1336不在1364..1563保存范围，保持UNKNOWN；1378有唯一同mesh seq16／Tick41489／Compose41490正常成功entry，raw/end与weight读取在该pair窗口内，pre在其前。具体三Local／Space／CTW及actual alpha仍待这份新解析收窄，不能先选写入根因。默认、头显、瞄准／射击与LGrip完成均未证，整体暂36%。

随后准确核对唯一新解析 `ht-ready-wrist-parsed-v4.json` SHA256 `f6fb4b1863121ea04ac2e1a9de690a3cf2cb721d1a5c852a4b6be3d4b38a781d` 及精确边界审阅 `ht-exact-pose-boundary-review-v4.json` SHA256 `77aadd63f789c919d7b933d3f61fa143eef998476e3a69e7a0b49eaffa2b9f1a`：epoch1378的计划→commit→pre→两个合格end35 weight→raw→end，所选Local35／51／52逐bit相同；raw/end世界35／52列表与外部配对身体完全相同，全部四个实际最终alpha packet为+0。该入口没有所选Local后续覆盖证据，不能继续拿旧HN的覆盖猜测解释它；枪组件CTW关系不一致仍是观察事实，具体更新写者未知。1336缺精确记录、完整115骨骼hash／完整Pawn与Ref join／枪组件更新时标／同C0及最终render均保持UNKNOWN。

下一窄边界选择已有外层MeshTick结束回调，先审能否在现成ready-wrist通道中被动关联同Tick父骨骼和已认证附属枪组件；不延长已过期C0写许可、不直接强写枪矩阵、不调用仅有声明的ForceSkelUpdate／SetTranslation或翻未知标志。已存FA730正常更新路径中的条件分支只是线索；历史代码快照、磁盘EXE与当前runtime仍分列。仅检查既存代码及证据，不新增助手、不重复游戏、不改851d冻结Native／默认support3／menu-modal1。近处几何遮挡与枪刷新时序仍分别待定位，不能互相代解释。

第41轮实际实现现有通道的最小被动边界：`analysis/body-outer-tick-component-candidate1/` 在准确851d源上改三文件并新增一个内部include；RWENT005保持Header128／旧Record16416前缀，尾部新增C0正常close的枪样本和准确外层MeshTick正常返回后的父→枪→父关闭括读，Record17800。原Native body文件移除唯一观察调用插入后与851d逐字一致，Goal／neutral rebind不变；新时标不作为source续时或写许可。对象registry／PID-start-Native-cookie／mesh／实际Tick和epoch分别保留，typed属性链、52字节附件内层五字段与唯一bone52附件核验；完整115参考在外层父读中两次与原参考比较。仅已有GT phase／ready-wrist通道，无新hook／采集器／引擎调用或游戏写者。

初版c879编译与原45项通过，实际回调Host24项通过；首个Host编译因Windows small宏与既有数学头冲突失败，fixture按capture.cpp已有#undef修正，保留失败日志、不改Native。candidate2补binding配置try-lock保护，Native `c87527d06145ef51e730a0c8f8a892e85ecf455bec6ab19d2c97c4583236447a`、原45／45通过13.30秒、Host25项通过。正式合同 `RWENT005-contract.json` SHA256 `e48f61d2315ca3851a411425378153cabfd9dd96d6140d0418ffa290e287168e` 给出实际C++ offset／身份40字节／新OuterStatus域／显式采样序／UNKNOWN限制；last_compose_matches是最后记录Compose相等布尔，并非次数。typed发现只在外层结束执行，按类／world generation缓存一次，不在失败后每帧重扫；首次compose_gun可能unprepared，不借下一行补齐。

Weapon独立v5消费者 `443dbad4e21c83ecf30c909ef8599f391cd3794ea552aadf947b70734b599e0b`，14项CPU／完整合成CLI通过。新ObjectIdentity布局独立于旧WeightReturn；旧v1-v4 dispatch保持，零四元数合成样本保留POD并把数学派生不可用单列。原Host fixture前缀缺entry身份不伪装成完整认证行，完整入口另用明确合成fixture；首轮零四元数失败记录保留。不解旧HT/HN/HP/HR，不重复旧矩阵分析。

HU `analysis/body-held-outcome-runtime-20261027u/` 用c875，其余XR1a059／PG5fdc／实际CY378e／pair4b复用，首次合并已有两次顺序截图读回修正。目标仅肩位→固定ready新边界，保留40提交／6秒原肩门、ready原提交与3epoch条件，另最少3秒保留窗；Stop/finally逐字保持。实际Game67884／Native3453347428551在肩门6.0055252秒只到held0→35，未到40，因此protocol false、ready与截图未触达。肩期独立body cohorts24／accepted gun pairs13，与依赖40门的coherent_observation_present=false分开，不称完全没身体读回；mount context_rejected0→23本身不能唯一归因age。新RWENT005仅128字节Header，count／selected均0，无任何边界覆盖，也无新typed发现运行证据。

HU原Stop4→5、hand quiescent、body_continues_after_stop、双控制器恢复与bridgeclose通过；正常finalizer仅一次accepted，106完整chunks／Native-entry-PG／XR0／原10及audio恢复、ownedPID空。首封摘要因恰有Root自身下一候选编译器在途而拒绝fresh-empty，保留失败后等其正常结束再封，不重复finalizer。summary `c3e2cc7ab0da920fb0210447a73e47e13626ed6445d6037fe2c8d53b6186c1ae`，窄未触达review `7fa3096ef8c0d7d5a9e65536ea4ac8e9e8028037dd1cc91e09b77cde561e3af7`；Weapon只核三份输入及该review，未打开空wire。

Root自审补上新增outer_end的owner_thread前置立即返回，避免非GT回调在scope检查前读取GT私有ticket；candidate2未触达新窗口不能证明该风险不存在。单行candidate3源 `analysis/body-outer-tick-component-candidate3/` 保留前两版原件，Native `fbaf466220e79f4275ada58750ee448477147741e88bc99279e92dd25fc16306`；Host26项（新增错线程不触碰／不消费ticket）通过5.29秒、该新DLL原45／45通过13.94秒，原flags／44导出不变。旧e48f合同／ABI不回写，另附 `producer-provenance.json` SHA256 `725778803397a034345e0d90a89769a9d1155855f34e455a746d19a1fc2fc275`，准确说明控制流修正和新Native来源。新HV仍用原40／6s门，准备后进入同一最小已知异常场景；新格式的实际typed／更新边界与可见效果尚待真实结果，整体36%，LGrip OFF。

HV真实最小场景已完成：Game17532／creation134375598747616710／Native3458878466732，准确fbaf，原肩门0→46／5.4799746秒，ready46→71／3.0143692秒，protocol true。Stop后hands86→118／held75保持／not_ready255→287／Goal applied328→360；正常finalizer仅一次accepted，106／Native-entry-PG／XR0／原10与audio恢复、fresh资源空通过。summary `f251d3ce944e032b27d2eec4621af1e12307232a47c88756d5904e2635a0fadb`。新顺序截图实际跨首请求完成：initial request4662、第二request4668／captured4667，明确不是保证在第二请求之后；两次BMP都保留。Root直接查看最新both-eye图，仍有大面积近处几何遮挡，无法可靠判断手枪跟随，不认定具体头网格／上传故障、不盲加相机前移。视觉review `a66886c7e6a631e9a5099d4d6fe457856163db8487a7204b76dc69ce35645584`。

HV RWENT005原件 `49a9ffdfb8b2ef515aa1e8d0a37ae90307eccd0595816c8d9683c8e3e196661b`，59saved／selected59／drop0／nested0／unmatched0；Weapon唯一解析 `415ceb72b24543b5689598e2db80965ca388185a13fc786de6e1cb2c89783a90`、最终分层review `630bc6d1a438a5c50a3cde34371255acc3da2be752fb60501f4d6487a617e53b`。25条held_lancer／valid127／finish4／commit1／normal_scope=true均命中准确outer、called=returned1，但全部typed layout step1拒绝，compose_gun unprepared，父／枪／关闭均UNKNOWN；没有eligible漏outer，也没有可用跨界矩阵比较。其余7条held但commit12、15条free_hand、12条角色UNKNOWN按资格分列，不混入25条。last_compose原始0位于未执行赋值路径，不能解读为实际mismatch；缓存同失败也不等于25次完整遍历。没有据此声称子枪不刷新或父后续覆盖。

由这唯一首拒绝完成candidate4：保持所有原条件／七个typed属性表达式与短路顺序，仅现有layout_step细分具名条件，仍RWENT005／Record17800，不新建消费者或采集器。准确Native `0c92eba97a577b936a0150337ca4085e9f1eb4dbf4d8dc877059cf6a08bc2f09`，source manifest `1400bf0e2443d5a396a3173955b6dcde4b3cc6bf278c98a6bc1a73962f2f75b1`，Host40项／该DLL原45项通过13.61秒；独立producer映射 `de46a6ba2ca85435025ba24b4eb3d3eb9bc00b36f16a278ccad31e41dd717220`，旧e48f原件不回写、443d读器及14项不重复。忙mutex当前尝试与缓存终端发现结果分开，避免一次忙覆盖先前具体失败。

HW复用与HV逐字相同的coordinator c4aaf9ce及原XR／PG／CY／pair，Game37048／creation134375612668877206／Native3472837061334。肩0→46／5.4074063秒、ready46→65／3.0379769秒，protocol true；一次正常finalizer／106／Native-entry-PG／XR0／原10及audio恢复通过，窗口释放。summary `ab1b5351f8aaf931825e75d2595d39b728c2ba748f2ffb57efe4eab16abb375d`，原wire `36266437f23317841fab029df7cff862104c9659b40442b08e06e666ff67c534`，57saved／selected57／drop0。Weapon唯一parsed `8bb370d7822a717b4b1a0ee5183c892ba767df6404c22346cd961eacaf1706fa`，正式review `129919de501f9a53ad900fcf8ca17b5e9ea00207e357878716f9f0e978e3a5e7`：20条eligible全命中准确outer且step105，即Weapon.Mesh typed property；101身份、102Pawn.Mesh、103Pawn.Weapon、104Instigator在首发现中先通过。剩余37按角色／valid／finish／commit分列，其中一条held valid63的finish与commit UNKNOWN不并入另四条commit12。没有有效outer父枪，所以不做矩阵数学；105内部identity／kind／shape／声明目标条件仍未知，不能凭分组拒绝放宽验证。

后继仅为这项拒绝内部取证：candidate5准备使用现有property校验的机械等价私有诊断副本，保持严格SkeletalMeshComponent声明要求与原短路读取，原共享helper不改；只记录本来已经读取到的引用指针／identity／Class／name拒绝，不重放guard、不增加Game读取、不采失败后的父枪。其首build因私有类型别名遗漏失败，原791源／manifest／patch／失败日志完整保留在attempt1；补类型命名空间别名的准确源再构建。未取得有效声明类型证据前不猜MeshComponent或改变权限，旧851d／所有冻结渲染器与候选原件保留。整体36%、LGrip OFF，持续跟随／射击／可见效果与真机候选仍未完成。

2026-10-27 后续小步及新优先级记录：candidate5准确build2 Native `f135c3a205c96a5ddd4ce5d1f66ee5b4367ade7dacfc408519d1e1ca5a35164c`，源manifest `51152b111df7e67d7f65525bd52f3f023f7a6b1b5701b6fd7b0420d832308fc4`；Host40／原45项通过13.22秒。Host首次因缺原helper的rejected_field缝编译失败，补fixture声明后通过，不改Native，失败保留。producer `59204251321cbe574ddbbaca892fcbdd52283b0730fd817b6414b26cf7d6b9bc` 明确私有property副本在剥除namespace及拒绝观察后精确等于原函数，仍严格要求SkeletalMeshComponent声明；原共享helper与读取／返回顺序不改。

HX实际两门及protocol true，肩0→40／4.6219391秒、ready40→67／3.0035789秒；正常finalizer一次accepted／106／Native-entry-PG／XR0／原10和audio恢复，summary `41db7cd271178b023dba8f66900822fae61c3927aabced1d1c9ee600a2c73eee`。唯一新wire `347e10881144babe46d78c968c2e0d8a4ac4dc4e48193cce3a5bc70e0b18d5d3`，61saved／selected61／drop0；Weapon parsed `aac88a592f7c6b0005819aa4b6ad85e43803a18e5c5c7490708a2c28e6a020eb`、正式review `4f1e0678bc89a6b5b49c7447d6676394786d792c66df8d0bb7c7a2cf47140c38`：28条eligible外层全10570，实际已通过引用读取／identity／Class-kind，原声明类名比较失败，捕获名为MeshComponent。声明目标类与实际SkeletalMeshComponent实例必须区分；未序列化拒绝时的完整Class身份，不能借此命名枪刷新根因。所有外层父／枪仍UNKNOWN，不做矩阵比较。

据该真实拒绝仅修观察器：candidate6把Weapon.Mesh声明预期改为实际MeshComponent，并新增10580独立认证实际组件的完整ClassPath既派生SkeletalMeshComponent又包含准确声明Class，gun样本前后重新闭合相同路径；其余原typed／extent／identity／附件、Gameplay、输入时效、写者及清理不变。准确Native `55ac90d340af940061e83d5da03ae2082d8e20bf68e4741d145ed1bec750a1d9`，源manifest `0c3258cbd3c8fb2d02194062e786ea0177962ce5b3c44a4461b27c8de02709f4`，Host41项／该DLL原45项通过13.25秒；producer `41f9366c64f5932016c0dd266a46cb8f56ce96dad586f752c95d17c272121d1a` 引用HX实际证据与新类型检查语义，e48f原布局合同不回写。修正后的实际typed外层采样仍待验，不能称W2修复。

HY原肩门未满足完整合取，具体为Native0→46已越40，但第一条保存且计数>=40的body行采样begin在6.269289秒，晚于phase末6.027458秒；不能把它报成35/40或完全没有身体读回。原40／6s条件未降，ready未发，新wire仅Header128。正常finalizer一次accepted／106及其余恢复通过，summary `22b7b4636f98381ae1ba8e54a90d275121ac8a9464e9401fa996de0e509ecc9c`，未触达review `16a3b03babfb7d9a5f957caf3c7baf051e4b492f24e2474a324cf5ce91eeccac`。采样begin不是文件完成时间，不补造后者或宣称CPU／GPU根因。

随后只改私有harness肩位观察调度：该原门只要求完整coherent body，故用未改probe做肩位预热，optional gun pair留到ready及其他阶段；原coordinator c4aaf9ce、40／6s、body一致性及Stop／106逐字保持，Native55ac／XR／PG／CY／pair及其检查不重复。实际AST分支6项通过，变更Host预检通过；准备脚本首次匹配缩进失败在写pins前停止，修成实际三空格后完成，未启动游戏，错误记录保留。

当前子项 **D0**：变化HZ及唯一有上限HAA重试都在原Source1／startup panel预检前终止，Native主功能未启动／未激活，无ready、协调器未执行、动作0。保存CSV各2完整行符合Source1／flags15，但未达原>=30行且缺startup_menu_first_frame＋end_frame_succeeded标记；generic wait card不替代该条件，保存行数也不是总执行帧数。原XR发stop后35秒未返并按原owned-child策略结束，exit1。prime entry_restore、PG reservation-only entry_restore、audio及原10恢复、无剩余Game分别成立；标准finalizer每局仅一次accepted=false，均非正常106／XR0生命周期通过。HZ Root review `b42b0188c9239f78ae0185fe2af23e70e0e1bc2f9b6f4ca4b7dfbf204c8bd95d`、独立review `4bee5997bb146442f79fa41fa490a476b61721d5f8103bb4825d573f1192cc51`；HAA review `edce7bd3940c002d3bee7c3b29218d49a074ae973520faf5e32c289f0821cc0d`。已停止等价Game重刷，不改焦点／窗口／preflight门，未把Tigon SSL timeout指定为原因。

D0当前具体动作是准确冻结XR的窄call边界诊断，由现有Weapon准备CPU候选、Body唯一运行；不新增助手或默认构建。首次既有normal-parent4174、CY原环境的6秒无Game／未请求render测试已运行：自有XR73776未在30.109秒外部deadline退出，受限结束该子进程，exit1 forced，pose.csv零字节。实际仍有D3D12设备初始化；该结果不能叫纯无图形测试、Source1／WGC验证、原故障同根因或零实际内部帧。review `37f2e9b764b5fcaf2570a8d50ec3c652a386355eb3d62ae61dd2b431d3d3ad88`。因此新诊断须覆盖共享无render主路径及xrPollEvent；仅首8帧或只在WGC回调导出都不能保证后来未返回调用有证据。沿既有stderr最大45秒／65536记录并显式exhausted，无新线程／IPC／等待或返回改变，首8只是初始标记。原1a059、Native55ac／851d和默认渲染原件保留；旧暂停／采栈thread_wait_sample不使用，冻结的纯CPU/WCT helper可对已确认owned XR pending线程至多3次、每child1.5秒查询，无暂停／进程内存／栈／权限更改；当前尚未执行该采样器。

用户经统筹明确新v2顺序及计分：只读 `J:/gearsVr/docs/6dof-detailed-workplan-v2-20261027.md` 的具体顺序／三个卡点与下一交付ID。11顶层维持，B1/B2/B3/C1细项重算后的总分 **30.23%** 取代旧36口径，仅旧证据重算，非本轮新增完成或回归。D0既定窄诊断收尾后，主线为 **H1：B1.01／B1.09／B1.10／B2.01／B3.02**，先同一候选固定头手、正确基准和清晰可见场景／双手，再指定小运动与B3.13／14／17已支持接管恢复；不把完整W2枪跟随／射击当H1额外前置。当前 **D1** 观察器类型修正只有离线及对应拒绝证据，未有有效外层样本；**D2** 最新画面几何遮挡仍未知。H1源码有真实Ref眼骨57／60的固定头眼配置，初始化只在首次写前捕获；body view在renderer中覆盖legacy FP基准，不能靠盲加相机前移或拉臂掩盖。下一实际检查点先取可推进的XR call进展，再取同候选初始HMD／眼点／头／骨盆／双腕及清晰画面；未交付H1前不让用户调脚本。默认Junker／冻结renderer／既有保护不回滚，LGrip仍OFF。

D0 新诊断／身份回执：现有Weapon仅在准确normal4174／sim1a059父源和原flags增加有界stderr调用边界及xrPollEvent，当前normal `1b02d7052c1c184fa440e9c85767ae1b6b64164a69c25a15c8a6b90f45634714`、sim `188670a75f11f197a0d540504ed25231395ce20b4599f609102f77770f95945e`，contract `60181ab4df984ad5d0dc7a02b031dcb4ea9d26f896e431c057851a94d47fa0fa`。未选早期无Poll候选，不改默认／API语义。首轮normal26436／TID21832在30.032秒仍无creation completed，仅initialized ticket0／frame0／phase0及已有create_instance_enter；外部结束exit1。该phase0不是pending API，且缺唯一活体birth时Root拒绝猜PID、实际WCT0。早期观察未见CSV，此后统筹及Weapon按实际argv路径复核CSV存在但0字节，保留时点差异，不当作零内部执行帧。Weapon窄review `b0c5c0e09d2bfd1099bc179c3a0e883c8e04975af31c7b6b57d6616f2f9dd83a`；不能把缺completed、layer日志或扩展警告指定为根因。

按统筹45只补Root私有harness：保留同一normal1b02、6秒workload／30秒外部deadline及子环境，Popen后以保留handle读取PID／creation／完整image并验证准确SHA，再等待；自查询identity自检通过。新normal58284／creation134375735388994641／实际TID45452，无Game／未请求render，不改XR／Native／模拟器。已有setup前置记录关联的第1次纯CPU/WCT后，本轮实际出现creation completed，前两帧EndFrame成功返回；最终record44／ticket22／真实frame3为xrEndFrame enter_intent，至外部结束无return。3次查询分别0.344／0.328／0.328秒，身份均核验；第1次3节点，后两次准确关联ticket22均单节点，CPU kernel/user/cycles相同。动态WCT无cycle和单节点不构成无死锁／API内部阻塞结论，前置日志IO间隙仍包括在未返回区间；此轮与26436不同边界，不偷换同根因。

新轮30.156秒owned-child受限结束、exit1 forced，非正常XR清理通过；当前独立查询无剩余Game／XR，三次预算已耗尽不重复查询。stdout1128字节、CSV存在0字节，trace44条；Root封 `analysis/xr-startup-progress-20261027/normal-owned-identity-review.json` SHA256 `f7502786614033ad46ddc653fd88dbe364b38a39df942c52184d73217bbf1ada`，关联birth／原stderr／marker与三个WCT原件。D0仍未解决，30.23%不增；下一步仅由现有Weapon审这一份本轮与准确源的无render EndFrame路径，禁止等价Game重刷或再造采集器，H1优先级保持。

D0后继找到真实参与的隐式层对照入口：本轮私有OBS日志显示共享mirror consumer及OFXRBridge链；只读实际HKCU注册和两个manifest提供的disable_environment，未动全局配置／OBS进程。原normal1b02不重建：仅子环境跳过OBS的14128依然在record44／ticket22／frame3 EndFrame前置区间未闭合，30.203秒forced exit1；仅跳过OFXRBridge、保留OBS／operator／CY的9228正常完成533帧／533次EndFrame返回和533条CSV，11.438秒exit0、不forced。实际清理是request_exit_session_returned→EXITING8→DestroySession／DestroyInstance／cleanup／mainreturn，不声称观察到未执行的EndSession返回。三个对照封 `analysis/xr-startup-progress-20261027/layer-isolation-review.json` SHA256 `626eefc34f5efcd5e1a9d6363732c495a6d72d75c754b0532352ab293ff445c7`；首次封稿把清理marker误用EndSession而断言失败，修为实际request_exit再封，不改运行原件。仅支持此配置对OFXRBridge参与敏感，插件内部原因与真机均未知；CY零projection尾部仍有图形／preview工作，不称完全无GPU。

继而新ab仅继承child `XRFG_DISABLE_OFXR_BRIDGE=1`，原Native55ac／XR1a059／0.4和Source1启动门保持，不跑RGrip协调器、HandStart或控制器动作。Game60280／creation134375740706825380实际进入ready并完成45秒被动窗口；标准finalizer唯一accepted=true、errors空，session passed／stopped、Native清理及entry、完整106／8pose chunks、XR0、原10文件及音量恢复、PG shutdown／entry和monitor joined、最后exact owned空均通过；之后独立CIM亦无Game／XR。host较早的no_remaining_games=false与稍后无残留分别保留。handtrace0记录符合无HandStart，不算身体动作通过。启动生命周期封 `analysis/body-startup-bridge-isolation-20261027ab/startup-lifecycle-review.json` SHA256 `a9fa2975e7ad10ef7ca8ff3342dedad0d13a525919eece363e5cde57f26428e3`。现有Weapon源码及三组独立窄审 `be66c9a950f9112743e5b968471b552fc4aa2fb1d8c385596485267d7eedb68b`，未要求新构建或额外W2门。

D0此环境的启动／正常收尾已可继续，整体30.23%不增，默认和系统设置不改。主线现为H1固定头手：新ac私有fixture仍准确Native55ac／XR1a059与同child隔离，关闭仅限held的trace筛选及W2 ready-wrist观察，保留主Gameplay、arm-length和原Start／Stop／恢复／106；两手自然目标相对头为±0.18／-0.30／-0.25米、无grip／trigger／头显移动／取枪，最多四条手部指令包含原值恢复。已有身体只读cohort替代optional枪对照，实际hand commits／body view更新／三独立pose epochs和双眼图片分别检查，不把W2跟枪串为前置。准备中尚未执行ac，现有Weapon仅交付实际Ref115头眼消费者，用原coherent已读bytes计算，不增加Game读数或Native写者；未修D2遮挡、未给H1加分或要求用户调脚本。

H1 ac实际接入与首轮：同Native55ac／XR1a059／child bridge隔离，16项实际fixture静态检查通过，完整原finally AST／Episode身份及输入／screenshot语义保持。现有Weapon纯消费者 `authored_eye_consumer.py` SHA256 `b50516f805c45dedd5b16a29552fe1374586442b7b37e2bf4a62c6681ad5beea`、handoff `1b2c711071e7be8250a007f8a2de5caa8dca6da04c17eace6ed28893e42827e1`，三项针对CPU检查通过；Root只核准确hash并把实际coherent已经读到的Ref115／Space55*CTW交给它，不重复旧测试或新增Game读取，旧常量仅留legacy字段。ac实际17个coherent行都得到actual authored eye，无probe_error；Ref SHA `fef334cf78e72a23c1bfd289780652feb12816ba508b92fdc0e052b9c61ed5ce`，完整canonical `3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193`，不把几何摘要充当新命名／owner／时效权限。

ac Game52500启动通过，但协调器第一次initial=episode.sample使用原01d8读器返回2、stdout仅matched:false／stderr空；在HandStart之前终止，commands0、无图片、无固定姿态或D2通过。既存finally即使从未Start也检查续更，原超时与空Stop codes／both_controllers_restored=false保留；没有控制器修改，不能把它称为失败的真实Stop或身体回归。标准finalizer唯一accepted=true／errors空，Native-entry／106／8pose／XR0／原10及audio／PG恢复和exact owned空均通过。Root封 `analysis/body-h1-fixed-runtime-20261027ac/h1-initial-sample-failure-review.json` SHA256 `f63eb7aa149ad535c7f7695c60b26d7b04a78b698e35b23c903205f2f8fed5b0`。总分仍30.23%，H1尚未就绪。

准确01d8源窄审证return2仅表示原400ms匹配窗口耗尽，open66／process-current65分别保留；原短路循环未导出各原条件，因此现存记录不能指定失败项。后继只给现有Weapon一个必要读器增量：记录原已执行stage／首次末次和现成observer reason／已读字段，失败末stderr一次；成功stdout、exitcodes、400ms／Sleep5／100ms、短路及身份门保持，不复读、不重新匹配、不刷新clock或等价重刷。只同flags一次CPU构建与针对检查，Root仍唯一Game运行者；不改眼点／Native／默认，不扩展W2前置。

人类经统筹再次明确第一人称先成立：ac现存capture-status实际为 `first_person_preview enabled=1 requested=0 transitions=0 applied=0 excluded=497 rejected=0`，不能把config True说成已切第一人称；Native body_goal_applied／published／view_used及hands均0，因此也没有新的body view接管。原body_observation.capture会调用owned `fp_toggle.apply`，但本轮runtime-host.watch只采cohort，确实遗漏原实际切换步骤。准确55ac选择逻辑先legacy FP prepare、后body read_view若owned覆盖整份base_view，非叠加两套位置。下一同候选在真实受控场景、ready前复用原单次owned FirstPersonToggle事件／背景身份与physical-input保护，并先取实际XR左右眼图，再读手输入与接管；最终requested／applied来自实际capture-status、图像与body-view计数分列。无图的ac只承认未接管时段，不能将普通监视画面直接当XR双眼结论；优先B1.01／09、B2.01、B3.02／18，不改相机常量或新增助手。


## 2026-10-27：按用户最新指令交付第一人称／双手手动候选

本节取代此前仍要求继续自动验证的执行顺序。用户通过统筹明确停止自动 Game／XR／模拟器、测试用例、自测及回放，只继续源码、编译、链接和打包；首失败诊断保留冻结，不再作为交付门槛。先交第一人称与共用身体基准，再接完整位置／旋转的自由双手，使用同一候选，用户真机反馈前暂停后续射击／换弹／掩体扩展。未把源码或编译成功计成玩法通过，原 30.23% 证据口径未提高。

此前 ad／ae 原型实际启用第一人称后，手臂仍不可达且画面有近距离遮挡；ae 的 reference_neutral 试验没有解决这些问题，不能当作通过。未继续运行 first-rejection 新游戏案例。已有自动测试进程正常结束，之后没有启动 Game、XR、模拟器或测试程序。

源码整合以最新 Junker Native `23621917…7590` 冻结树为底，并入已有 body prototype；不是用旧 Support1 整棵覆盖当前默认。最新 focus1 与物理 RuntimeTracked BQ controller／continuity 源同父三方合并，保留 focus、菜单、颜色，物理 producer 五文件逐字匹配原 BQ 源。Junker Playground `75e54302…61581` 原样保留。J 主默认 manifest 和公开包没有替换。

A：候选显式初始化第一人称请求；身体模式去掉预览固定前移／抬高；静态 Ref115 eye/root/pelvis 配合原实际脚部接触，避免首帧武器动画扭身成为共同基准；增加候选第一人称 0.12m 近裁剪。近裁剪会影响所有很近的几何，尚不能声称已证明只遮头网格或画面已修好。A 编译链接 exit0 后才接 B。

B：武器提供纯变换 helper `93e2c62d…84d1`，由 Root 接入同次 `prepared.desired_eye`、完整原始 tick key、HMD 和 controller，位置与完整四元数只转换一次。固定原骨长，超臂展只投影腕位置到可达壳；左右 solved mask 分开，失跟踪侧不给 alpha 控制权，沿用原版动画／单写入恢复。B、最终 Native 及物理 XR 都编译链接 exit0。只构建目标，无测试或运行验收。构建记录保留初次失败和修正，源码/选项有快照。

交付目录 `J:/gearsVr/candidates/6dof-manual-20261027-1`，正常 GUI，默认 0.4、相机拉近 0、第一人称勾选；已配置购买版路径。Native `322d8733291660a26445d8646d18a39d1aa32dc4f499d8d66f8b624fdd3f209b`，XR `9356fff38724d42141723285cf3c90445163aedfe2a6c51fd058b504ad3666cf`。显式 `sixdof_manual=true`，不含自动自测 EXE 和模拟器；原身份、新鲜度、启动、恢复和停止保护保留。说明见 `docs/6dof-manual-candidate-20261027.md`，包内 README／CANDIDATE／candidate-state／inventory 分列实现、编译、打包和待真机状态。

当前状态：**代码实现／编译／打包已完成；实际启动、清晰双眼、持续双手、停止及真机验收未确认。等待用户反馈，暂停后续功能，不继续自动测试。** 回退使用正常停止并退出游戏后，再打开 J 主目录的原正常启动器。


2026-10-27 武器会话完成RWENT001纯文件最小消费者交付。准确source3合同27493ce7…3520，reader821d0167…8ddea，回执26a8c3aa…92940，私有件在ignored `analysis/lancer-ready-same-entry-20261027/`。独立ctypes逐字段对compiler ABI，12项必要检查／0失败（0.020秒），CLI成功及覆盖／截断拒绝通过。valid组和read-status分别约束可信字段，失败记录保留；比较Local35／51／52四个相邻阶段以及returned_end的Space×CTW世界手腕误差，raw米与target游戏单位分开，不套未认证阈值或自动归因。跨entry必须有完整映射观测和相同cohort，rawRight单独固定不充当目标固定证明；原ns和QPC频率边界保持，gun／Muzzle缺证不补。Body准确67b513候选与K预检已由其交付，消费者已通知Body可开原独占一局。未动／构建Native、PG、XR，未重跑旧套件或旧案例；K实跑结果待收，不计跟手功能完成度。详见 `docs/weapon-ready-same-entry-observation-contract-20261027.md`。


2026-10-27 武器会话完成唯一K新RWENT原件的一次实际解析：Game74284／Native3200498446623，18条=9提交成功／3guard拒绝／3mount0／3seed_model捕获失败。9成功样本planned→commit→pre→end的Local35／51／52逐bit一致；3拒绝seq7／13／15(code12/unit17／3／9)在pre仍逐bit等于before，end腕离target约15.77..16.18u；known-ready的11份有效target均相同，完整相邻mapping相同仍发生成功／拒绝分流。优先交Body定位capture／mount／commit准入，不能归因为成功写后动画覆盖。解析原件SHAb3563225…ab28b、输出f0c0d74b…4723b、派生review fc7cf1c5…9e7a5，均见ignored `analysis/lancer-ready-same-entry-20261027/`及同次观测文档。原ready phase仅15capturepoint／7成功；新观测不覆盖肩／移动，启用期unavailable293不当ready丢失。Body原动作／恢复／106清理通过，不增加功能完成百分比；未重复v3／PG解码或新Game／Native构建。


2026-10-27 按统筹安排完成RWENT002窄file-only适配，reader9923573d…dfa00／回执feddc65f…fb039，准确合同fca9c717…2fbfb。显式v2／R2040，仅尾2024 verdict与2032 causes；复用冻结v1前缀数学，原件身份与内存投影分别标记，v1结果及K原件保持不回填。UNKNOWN不推通过，只有verdict3＋mask1／2／3且prepare0／commit12合法；mask仅已计算事实，不推先后／branch clock／validate-exchange或深层原因。6项必要方法／0失败（0.007秒），CLI新合成与v1兼容通过、覆盖／半缺对拒绝；未重跑旧数学／旧案，无Game或Native／PG／XR构建。已交Body／统筹结束此离线交付，新候选验证安排另循唯一窗口，不增加功能进度。


2026-10-27 武器会话完成HM新RWENT002一次消费（原件015902a9…f4b1b／输出237343af…49132／派生review a0dcdc54…96382），Game26228／Native3222855251894，23条=11成功／5guard拒绝／6mount0／1seed_model。5拒绝尾全verdict3／causes1，首seq6 tick36468／Compose36469 epoch958 unit6，实际记录BodySource source-age-only事实；其余18尾UNKNOWN，不推出先后或branch clock。11成功中10保持、seq13出现新差异：planned→commit→pre Local35／51／52逐bit相同，pre→end仅35变（1.21869rad），CTW与epoch965保持，world腕离target17.0568u／.569836rad；accurate key／stage／phase已交Body定位本次写者／rollback边界。K9成功保持与HM此条分别保存，不互相覆盖。ready capturepoints2..21共20，肩／移动不覆盖；HL预启动失败不算HM覆盖。原三门／恢复／一次106清理由Body通过，与稳定跟随接受分离；无重解K／PG／旧v3、新Game或构建，功能百分比不涨。


HM采样边界补充：Body核对returned_end早于THIS Kernel.finish，排除其后的finish／rollback解释该已采Local35变化；原调用返回至snapshot尚有wrapper收尾及Goal读取，暂不能归单一原函数内写者。仍只定位pre→returned_end区间，原parsed／review不改，不新增Game／RE。


2026-10-27 完成RWENT003原函数刚返回边界的窄file-only消费者：reader235875fc…42122／回执6fabad75…d558e／合同671b56da…c3c42。H128／R2368，原2040前缀及Stage布局不改，只加raw_return328；分别比较pre→raw和raw→end，记录epoch／CTW资格及world误差，不推具体写者／写入瞬间。7项必要方法／0（0.008秒），CLI新合成与旧v1／v2兼容通过，覆盖／截断拒绝；旧数学套件复用、旧K／HM不重解或回填，原reader／parsed hash保持。已交Body／统筹，未开Game或Native-XR构建，准确新候选验证由统一窗口安排；定位准备不增加功能百分比。


2026-10-27 HN唯一新v3消费复现成功后Local35变化并缩小边界：原件8cda052c…3afb7／parsed b153ecc0…0f72b／review9b6d2501…1d407，Game59204／Native3232920772391。19=9成功（8保持1变）／2reject／6mount0／2seedcapture；17有效raw全raw→end Local三骨／Space35、52／CTW保持，2rawUNKNOWN不算稳定。成功seq13 Tick41283／Compose41284／epoch947 prepare0 commit1 finish4，planned→commit→pre保持，pre→raw仅35变1.14386646rad，raw／end同腕误差16.77294u／.571303rad。因此本局变化在finally/endcallbacks前已出现，排除其后callbacks／Kernel.finish解释，但不命名原指令或排除并发。准确key／4stage／flags／phase交Body，HN不代填HM。capture点1..17在ready、18/19在ready后move前；肩／移动不覆盖。原三门／恢复／106清理通过与稳定玩法／头显分开；无旧案重解、age／PG-Muzzle扩查或再开Game，不涨功能分。


2026-10-27 完成RWENT004既有weight回调边界的file-only消费者（reader3d9ce0cb…fa8b4／回执e299b52b…562f4，合同31797a2e…4a89d）。原v3前缀保持，H128／R16416，新增bounded Diagnostic／WeightReturn+Stage／actual raw4 constants。严格count/drop／identity／signed字段／mask／partial，独立pre→weight→raw Local比较，无weight Space不编造world。按Body已有ASM supplement6c2e5ed6收窄候选条件：必须同entry/node/compute+weight ordinal的sample4/reason0、前后cached_flag读取0且其它角色／snapshot／alpha／实际常量全合格，才描述alpha>zero且>=full或fractional；零／无效／缺失保持UNKNOWN，条件不证明store／排并发／穷尽集合。最终12必要方法／0（0.039秒），CLI合成及旧v1-v3兼容通过，非法pair／覆盖拒绝；旧数学复用、K/HM/HN不重解或回填，原hash保持。已交Body／统筹，无新Game／RE／Native-XR构建或PG-Muzzle／age扩查，候选实跑另由唯一窗口安排，不涨功能进度。


2026-10-27 HO新v4为准确空覆盖：原shoulder门6.0073855秒status0→0／无coherent，ready／move未发送，未放宽门。原件7a30ff5d…7b75c仅Header128/count0/selected0；Game41768／Native3269225923034。冻结reader只解一次exit0，parsed dffc5951…d7bc9／empty review b3c1632e…158a6，旧HN等不替代不重解。新weight边界无目标覆盖，文件有效／正常恢复清理不算成功或修复证据；启用期491 unavailable非ready丢帧。Body沿本局旧first-failure／held记录解释门0，本侧不改weight接口、不再开局或扩PG-Muzzle／age，不涨功能分。


HO后续Body既有记录明确：90attempt／90query前not_ready、query0，fallback hands unreachable8；first-false未保存，不归具体准入条件。该局没有held baseline，body-continuation未证，不能以106清理代替。Root保留原空解析／review和冻结v4 reader，仅接收此边界，不做泛审／再开局。


2026-10-27 完成查询前首false的BHELD004最小consumer（reader1e0c7d30…af0ef／回执27a2f8b4…58ea8／合同05070c33…95261）。H128／R648，正常行复用未改v1/v2/v3纯row helpers，v4独立真实arm规则；kind2不伪PG-ready，仍只约束begin≤deadline，end可越界。cookie0 gate20单独非权威diagnostic，保留source0／UNKNOWN、rawindex/slot，不补header身份／cookie／时间或送旧行验证。11必要方法分批通过（10/.007s＋1/.002s），首批旧合成equipment时间导致16断言失败的版本保留，修fixture不放宽reader；CLI特殊／混合窗／旧v3兼容与非法数据拒绝通过。原Goal/capture/guard字段和新prequery首false分开，不用HO90基线解释新增151Goal拒绝。权重reader3d9不改，旧实际案不重解，无Game／RE／构建或宽调查；已交Body／统筹，定位工具不涨功能百分比。


2026-10-27 HP新BHELD004一次消费通过格式验证：Game33264／Native3309563927982，原件e625cebb…5b329／parsed84002ceb…ccd8e／reviewfbe24557…d8dfb。128raw=127normal+1gate20、drop6；kind2非PG-ready。首normal raw1 gate12 cookie1,parent0发生于肩marker前.5831407秒；117前置通过／7未观察／2context_prepare cached source_age3分属shoulder raw32及move raw117。独立ready Goal14 adapter2/core raw98的prequery UNKNOWN，不借别条填补。三个原门通过，但protocol/bodycontinue均false，post-quiescent free-hand续更超时原样保留，不能借106正常清理替代；新记录不证明HO修复。有限savedspan止于move末前.2981668秒，drop不补值。只消费新BHELD，不追加权重／矩阵／PG-Muzzle／age或旧案/新Game，不涨功能分。


2026-10-27 第37轮仅消费HP已有RWENT004权重原件a703a459…e402f一次（parsed401da0db…c2c23／reviewfa760a05…c5462），未新Game。9成功entry的18合格右腕weight前后Local均保持到raw/end，本份成功后变化未复现。实际原alpha/sample4=1，原applied事件与最终packet均0；sample4/reason0、前后缓存flag读取0、当前raw4阈值齐全仍不能给零alpha套full/fractional归因。其它final1/flags7缺匹配compute角色，不能填入成功证据。264alpha/64weight事件drop0，RWENT globaldrop0与BHELDdrop6分开；权重capture只ready限定片段，肩／移动不覆盖。按一个下一动作交Body继续已有HP Stop回执定位free-hand续更首阻碍，不新增采集器或原样重跑，不借HN/HM补值、不扩PG-Muzzle/age，无Native改动/构建；整局protocol/bodycontinuefalse与稳定跟随仍未验收，不涨功能分。


2026-10-27 接收Body HR受控neutral observed-rebind真实游戏结果（报告8c7b0d28…14f85／Root receipt037de4f3…14499）。准确Native851d50fc11cbfce3052fa66e0e2680a8404d429e5c540b4e7e037f2ca75c9da7原45通过；Source1右grip tracking flags15→0→15，observed1→2且硬refepoch2/flags1保持，rebind1。首次已保存接受rebind样本后hand/Goal+30；Stop4→5后free-hand+37、held71保持，三门/10命令/protocoltrue。106全恢复/XR0/原10/audio9/9/实例空另列。受控接续/Stop续更通过，不冒称原HP自然触发重建、所有硬key独立序列化或稳定跟枪/射击/头显/default验收。Root只核报告身份和字段，无wire解码/源码复审/Game/构建；下一联合候选复用该工件，PG/consumer冻结，百分比由统筹按证据更新。


2026-10-27 针对HS新增pair门48次0接受（47跨组件发现body推进/1closing拒绝），完成单文件只读pair动态窗口候选4b473368…a8ca5／manifest e78e71f3…ab013。完整typed/身份/附件/Ref发现+预关闭在前，第二次未改原coherent probe取fresh body，短window内gun raw重复、AvatarCTW+未改115骨骼_close_pose闭合，完整static/selector/identity/ref/registry后关闭移到window外；动态值不再跨慢关闭pin，成功返回fresh保存cohort，失败原样fallback，明确window时点及sameC0/render未证。22 CPU方法分20/.159s+2/.014s通过，一次窄Sol审无P1/P2并补窗口后body推进fixture；原provider/字节/epoch/typed guards不放宽。只改Python模块和必要fixture，Native851d/PG/XR/held-RWENT消费者冻结，无Game/构建/collector或HS原件修改。已交Body下一唯一后继实跑，CPU结果不称运行修复或增加功能分。


2026-10-27 HT完成Lancer全动作／Stop续更／106正常恢复，武器侧仅消费本局已存3JSON。Game23504／Native3385921311899，分析dcbd1f5e…3a83c。新pair4b473实际取得48/62有效配对，动态窗中位6.54125ms（外层87.7139ms）；按完全包含phase计shoulder12/ready3/translation6/+10 3/−10 3/return5，未混门完成时4。48份附件/weapon/component/asset/relative参数与AvatarCTW保持，但同marker读回仍跳：obs4→5右腕31.461657u/34.28°；首直接gun/bone52关系大偏差obs14 epoch1336 gap31.002087u/34.402633°，另obs21 epoch1378 gap16.645682u/33.995248°。不筛样本，ready/+10/return两组姿态交替，6cm尚无干净响应证据，−10三份单独稳定不扩全程；左输入未改、无support grip，不强求跟右平移。已交Body两个精确epoch作既存证据关联，缺链保持未知，不命名写者/渲染原因。截图五freshness门超时、仅最后retained BMP待单独像素检查；typed配对非sameC0/非firing/非GPU/头显接受。无新wire/旧案解码、源审、collector、构建或Game，稳定跟随仍未通过，百分比由统筹按证据处理。


2026-10-27 HT两个异常点现存RWENT004一次消费完成（原件4fe331a8…1b5ab／parsed f6fb4b18…a781d／focus review77aadd63…b9f1a）。obs14 epoch1336不覆盖，保持UNKNOWN。obs21 epoch1378 exact同mesh保存seq16/T41489/C41490，valid127 commit1，pre/commit窗前、4weight/raw/end读取位于pair窗口内，epoch/CTW一致；planned→commit→pre→右腕weight→raw→end Local35/51/52逐bit保持，raw/end world35/52与外部body世界矩阵精确同。4实际alpha最终0，原sample/cache0/实际常量齐也不作两个store归因。这条排除已观察selected Local在这些边界被改回的说法，外部gunCTW关系差异未解释；wire无gun更新时点及全Ref/Local/Space SHA，不作完整hash或sameC0/render/写者结论。64saved/65selected/drop1保持，无其它wire/PG-Muzzle/age/源码/建/Game或collector。已交Body/统筹，稳定跟随未通过。

2026-10-27 按Body请求完成现有父骨骼→挂载枪组件刷新入口的窄查询，没有找到已经验证、可在当前owned GT调用且顺序明确的现成路径。主知识库保存的 `Engine.SkeletalMeshComponent.ForceSkelUpdate()` 只是native声明（文件hash eb1f25cd…bc3fd8）；历史字符串RVA不是函数地址，当前typed调用、child刷新语义及相对C0顺序均未知。AttachComponent参数导出不完整；现有装备挂载读回仅作metadata，旧phase1 setters是另一受lease约束的变更路径，不能借其权限刷新。receipt保存在ignored `analysis/lancer-continuous-follow-20261027/existing-attachment-refresh-entry-review.json`，已交Body唯一实施后继Native定位，没有新增PG写者、Game、构建或IDA。

同日应Body新合同 e48f61d2…7168e，实现独立RWENT005 file-only消费者 `analysis/lancer-ready-same-entry-20261027/read_ready_wrist_v5.py`，SHA256 `443dbad4e21c83ecf30c909ef8599f391cd3794ea552aadf947b70734b599e0b`。Header128／Record17800，16416字节v4前缀不变，compose_gun304及outer1080追加；独立核对实际编译字段和新ObjectIdentity布局、OuterStatus域、exact外层Tick/thread/mesh及调用返回、latest-compose布尔、父→枪→父关闭原字节一致和QPC顺序。Compose父/枪仅顺序观测，无父关闭括读；外层父epoch保留实际后继值，绝不要求等于gun自身epoch。身份/附件/world generation一致后才作跨边界比较，缺失或last_compose不匹配保持UNKNOWN，无邻行补齐、写者/最终render/射击/头显证明。

v5必要离线检查14项/.031秒通过，四个旧版本dispatch逐一等于冻结reader；完整合成CLI单行exit0。原Host样本58376d82…77387只独立解新tail核epoch42/顺序/括读，完整入口因其不完整旧前缀明确拒绝missing entry identity，未改原样本。独立v4合成前缀+ctypes v5 tail测试完整入口，全部标为合成非Game。首轮11/12过、Host零四元数触发旧数学校验的错误另存；新OuterStatus只认证读取，现保留原POD并单列数学不可用，旧v1–v4不改。tests SHA101dc25f…40ab4／receipt3e8b2d0e…ff8ed8；准确候选及CLI已交Body。Body持有HU固定ready真实运行窗口，Weapon不另开Game，不消费旧HT/HN/HP/HR，等待HU新原件一次消费；Native c875、真实结果、稳定跟随和默认集成尚未由本消费者验证，不增加功能分。

2026-10-27 HU已正常清理释放窗口，但原肩位40提交/6秒门只到35，ready未触达，protocol=false。Body窄review及其三份输入hash已核对；RWENT005 count/selected0、仅Header128的结论按report记录，不打开/解空wire，不用13份肩位gun配对或旧case代替ready/外层更新证据。Stop4→5、body续更、双控制器/bridge恢复与唯一finalizer accepted另列，不能替代动作门；23次context rejection不单独解释为age。武器接收记录 `analysis/lancer-ready-same-entry-20261027/hu-not-reached-received.json`，读取器443d保持冻结。Body发现outer_end访问GT专属ticket前缺owner_thread快速门，正在唯一实施该明确修正与必要检查；c875不视为完成修复，不扩大武器功能分或重复读器测试。

2026-10-27 Body候选3仅在outer_end访问GT专属ticket前增加owner-thread立即返回，新Native fbaf4662…16306、原45及实际回调26项通过。独立producer-provenance72577880…fc275／源manifest436324d6…9248f及DLL/header/live/两份验证结果实hash均核对，武器receipt eb83d6d5…3efd6；e48f原合同/c875/旧工件不回写，443d读取器复用且不重复14项。随后HV过原shoulder/ready门并正常Stop/一次finalizer/106/XR0/原10/audio恢复，Body释放窗口；这些是正常协议结果，仍非跟随/射击验收。

HV唯一新v5原件49a9ffdf…6661b／1050328B，Game17532/creation134375598747616710、Native3458878466732，冻结443d/e48f+producer72577880一次解析得到 `hv-ready-wrist-parsed-v5.json` SHA415ceb72…83a90。59saved=59selected/drop0/nested0/unmatched0；正式 `hv-outer-boundary-qualified-review-v5.json` SHA630bc6d1…7e53b按角色/valid/finish/commit分层：25 held_lancer1/valid127/finish4/commit1/normal_scope=true全部有准确outer Tick/thread/mesh、called/returned1，无eligible漏outer。其compose_gun全unprepared11，outer全layout4/step1，outer.body/gun/body_close均UNKNOWN；首seq3 T38489/C38490/endepoch995，末seq55。其余7 held但commit12/valid63、15 free_hand/valid35、11角色未知/valid33、1角色未知/valid35均按设计不eligible，不混全局48 raw/end作同角色比较。

HV没有可接受的outer父/枪括读，所以父后续变化、枪后续变化、外层是否收敛全部UNKNOWN；全部last_compose原0在prepare_layout失败后尚未赋值，不能报成观察到mismatch。首/后续unprepared不借下一行或旧HT填补，反复step1不等于完整typed发现反复执行；当前记录未给具体失败field/子条件，不猜锁/属性/age原因。中间未分层d6dd草稿保留但非正式交付；正式630b已交Body/统筹，下一唯一边界是现prepare_layout step1具体失败点。没有新增consumer、Native/PG写入、Game/构建、旧案解码或矩阵重算；实际枪体跟随、视觉、射击、头显和默认集成仍未验收，不增加功能分。

2026-10-27 Body candidate4仅细分既有layout_step具名字段，Native0c92eba9…c2f09／源manifest1400bf0e…f75b1，40实际回调Host及原45检查通过；producer附页de46a6ba…17220的DLL/源header/live/manifest/两份验证实hash已核对，武器receipt001aec09…3f4b3。e48f原合同/header/ABI/OuterStatus与443d读器保持，不重复14项或重解旧case。HW新实际场景通过原shoulder/ready/Stop/正常唯一finalizer/106/XR0/原10/audio恢复；窗口由Body占用并正常释放。

HW原wire36266437…7c534／1014728B只解析一次，Game37048/creation134375612668877206、Native3472837061334；parsed `hw-ready-wrist-parsed-v5.json` SHA8bb370d7…706fa，handoff17a3173e…890a7，正式 `hw-typed-boundary-qualified-review-v5.json` SHA129919de…3a5e7。57saved/selected、drop/nested/unmatched0；20 held_lancer1/valid127/finish4/commit1/normal_scope=true全部有准确outer/正常called-returned，无eligible漏outer。20条全停在新具名step105=Weapon.Mesh typed property，compose_gun全11 unprepared，outer父/枪/close全UNKNOWN；首seq1 T37421/C37422/thread51016/mesh3486117312/endepoch1342，末seq54。其余37按free_hand/valid35 16、held/valid63/finish4/commit12 4、held/valid63但finish及commit未知1（seq22）、角色未知/valid33 15和valid35 1分别保留，不混入20条有效角色范围。

本次将第一阶段拒绝收敛到具体Weapon.Mesh属性验证；其内部究竟kind/声明类型/extent/identity/current哪项未满足仍UNKNOWN，不据此放宽保护，也不将20次同终端step当20次完整typed遍历。前序101身份/102Pawn.Mesh/103Pawn.Weapon/104Instigator按原短路路径先于105；后继条件尚未证明。last_compose原0仍是失败后未执行默认。因全拒绝，不做矩阵解释、不借邻行或旧HT补值；没有新消费者/测试、PG/Native写入或Game/构建由Weapon执行。正式结果已交Body唯一定位该属性内部条件及统筹，枪体外层是否收敛、持续跟随/视觉/射击/头显/默认集成仍未验收，不增加功能分。

2026-10-27 对照装备已有源码：其Weapon.Mesh读取后认证实际对象为SkeletalMeshComponent；Python组件配对也校验实际实例派生关系，而Native观察器额外要求属性+d0声明Class名完全等于预期。保存KB仅有ComponentProperty Engine.Weapon.Mesh库存项，不提供实际声明目标身份，未据此猜本局原因或放宽验证。Body candidate5保留原共享helper，以机械等价私有诊断副本仅标记已执行的引用读/identity/Class/name失败，Native f135c3a2…35164c、producer59204251…d6b9bc、manifest51152b11…308fc4；40 Host/原45及acc6等价记录的来源实hash已核对，武器receipt ec3c4ff6…0d946。首namespace编译失败由Body保留，准确后继build2用于HX；旧合同/header/ABI/443d读取器不变，无重复14项。

HX真实场景通过原两门/protocol/Stop/唯一finalizer/106/Native-entry-PG/XR0/原10/audio恢复，窗口正常释放。仅消费新wire347e1088…8d5d3／1085928B一次：Game17708/creation134375630804911331、Native3491002809899；parsed `hx-ready-wrist-parsed-v5.json` SHAaac88a59…020eb，handoff3efcc196…f115e，正式 `hx-declared-type-qualified-review-v5.json` SHA4f1e0678…40c38。61saved=selected、drop/nested/unmatched0；28 held/valid127/commit1/finish4/normalclose均有准确outer及正常call-return，无eligible漏outer。全28记录实际step10570：原严格引用pointer读、目标identity和Class-kind已通过，在原name比较处拒绝，已捕获声明目标名为MeshComponent，观察器期待SkeletalMeshComponent。首seq3 T36036/C36037/thread61124/mesh3482881520/endepoch918；其余20freehand/valid35、1held/valid63/finish4/commit12、1held/valid63但finish/commit未知（seq18）、11角色未知/valid33独立保留，无资格行不混入28。

HX确定的是观察器声明类型预期不匹配，不能当作玩家跟枪缺陷根因或已修复。声明名与实际枪组件实例是不同对象，拒绝样本未序列化实际gun类/完整声明package/raw Class identity；不借KB/旧局补证。28条compose_gun仍unprepared11，outer父/枪/close全UNKNOWN，last_compose原0未执行，无矩阵计算、旁行填充或刷新顺序结论。已交Body唯一修正声明预期并保留实际实例认证、typed/extent/link/current保护，再取有效外层样本；Weapon不改consumer/PG/Native、不建/Game/解旧案或PG/Muzzle/age。枪体外层收敛、稳定跟随/视觉/射击/头显/默认集成仍未验收，不增加功能分。

2026-10-27 Body candidate6据HX实际声明修正观察器的Weapon.Mesh预期为MeshComponent，并以原derives/same_path额外认证、闭合实际SkeletalMeshComponent完整继承路径及准确声明Class；Native55ac90d3…0a1d9／producer41f9366c…121d1a／manifest0c3258cb…709f4。41Host/原45及引用HX正式证据实hash核对，武器receipt f4b67b04…f9028；e48f布局合同与443d读取器不变，不重测未变组件。HY由Body记录为未触达ready：保存的首条Native提交>=40身体采样begin6.269289秒已晚于门end6.027458秒，不能推CPU/GPU根因；未解Header-only。后继HZ私有host将额外gun pair从shoulder延后到ready，原完整身体probe/coordinator/40提交6秒/coherent/Stop106条件保持，Native/XR/PG/CY/pair不重建；Body保留唯一运行权。

HZ随后在ready之前由owned launcher terminal incomplete终止。Weapon只读追到具体XR预检，来源已封 `analysis/lancer-ready-same-entry-20261027/hz-startup-preflight-review.json` SHA4bee5997…2cc51：launcher/session/stdout/stderr/CSV/first-failure/当前controller源码/host result八份实hash。Game48108/creation134375656487625405、XR51108；原预检要求>=30完整CSV、末20行source1/state5/render1/head及view flags含15，并要求stdout startup_menu_first_frame/end_frame_succeeded/非stereo面板标记。本局只保存2条完整CSV，虽均满足逐行条件，但没有合格启动面板marker，只有generic wait card；不能用wait card替代。native_started及native_activated均false，故没有ready/outer证据；这定位的是观测到的启动边界，不排除所有passive reservation hook影响，也不将2条保存行当实际总帧/FPS或特定API阻塞证明。

HZ正常XR停止等待35秒超时后执行terminate_owned_child，XR exit1，不能写成正常XR0或106玩法清理通过。prime入口恢复、PG reservation-only shutdown0/entry restored及audio guard restored字段单列；完整收尾由Body继续核对。stderr另有SSL timeout，但没有其与XR停进展的因果证据，不命名网络/GPU根因。此回合未改launcher/Native/PG/fixture、未开Game/读进程内存/解任何wire；精确只读review已交Body和统筹，下一步由Body唯一跟进XR启动/面板进展，保持原预检及后台置后约束。武器侧继续冻结443d，等待真实有效父/枪新记录，不增加功能分或声明玩法修复。

2026-10-27 D0当前动作：HAA唯一新私有重试由Body确认再次在相同XR启动预检拒绝，停止原样开Game；标准finalizer accepted=false／XR35秒强制结束exit1与已恢复prime/PG/audio/原10分列。按Body明确新分工，Weapon只在ignored `analysis/xr-startup-boundary-20261027/` 完成准确frozen XR的最小被动诊断源及CPU构建，Body独占所有XR/Game运行和shared Native。原1a059完整591文件/flags核对后私有复制，第一候选b493保留；没有从当前杂合源码构建或替换默认。

D0首失败与分离：原1a059 CLI明确禁止无Game的Operator或显式scale组合，因此没有伪造Game PID绕admission；先交准确normal parent4174的6秒无Game命令，私有CY/LOCALAPPDATA/NO_ACTIVATE/BEHIND/OBSERVE_ONLY与原物理后台输入门保持。未传render/scale，指应用没有图像swapchain，仍会初始化D3D12 device和runtime/preview资源。Body实际执行该parent后30.109秒外部deadline未退、exit1 forced、CSV0字节，不能称普通帧进展、与HZ同根因或排除所有graphics。准确源码证明无render仍走Poll／Wait／Begin／LocateViews／LocateSpace／End；据此新诊断必须覆盖共享路径及原xrPollEvent循环，而非仅WGC分支。

D0交接产物：最终normal诊断 `normal-build1/Release/OpenXRPoseProbe.exe` SHA1b02d7052c1c184fa440e9c85767ae1b6b64164a69c25a15c8a6b90f45634714，准确4174父源码与原normal flags；sim诊断 `sim-build2/Release/OpenXRPoseProbe.exe` SHA188670a75f11f197a0d540504ed25231395ce20b4599f609102f77770f95945e，准确1a059父源码与原sim flags。两相邻loader均66946；各591父文件完整核对、各592候选文件，只改相同两原文件／新增一header，18处机械替换逆向精确恢复各自父版，实际vcxproj仅路径归一后flags完全相等。manifest238aa858…5fa27／contract60181ab4…fa0fa，已交Body/统筹，旧父版及b493均保留。23实际CPU核心检查复用，header仅增poll enum/name可逆回原已测core；新增poll原while赋值/短路/调用次数/正负enum真返回检查及两完整构建通过，Weapon没有执行XR/Game或新助手。

D0诊断合同：复用既有stderr，45秒／最多65536条，所有选定非嵌套调用记录ticket/frame/真实TID/QPC/frequency/真实返回；首8仅标签，后续仍记录。xrPollEvent／Wait／Begin／LocateViews／LocateSpace、Source1采样发布段、启动及fallback GameScreen.render、End均在覆盖内；内部WGC acquire/update子调用及未选API仍UNKNOWN。原子pending只供已有WGC回调辅助，超过1秒／间隔1秒／最多3次，不以其保证导出；没有新线程/collector/IPC/文件通道、pose或source clock刷新、API重放、等待／返回／admission／颜色／清理改动。enter_intent含日志IO间隙，缺return不能单独证明API已进入或卡在其中；coverage_exhausted明确后续UNKNOWN，若终末transition是returned仍保存真实结果。新增日志可能改变时序，不用于FPS结论或冒称修复。

D0下一动作：Body可用已交 `run_standalone_normal_diagnostic.py --execute`（command69c21ec7…a6e9／runner83e125f8…d5fa8，新私有目录、原6秒workload、独立30秒外部守护）独占执行新normal诊断并按owned PID/start＋真实TID补已有bounded WCT，不改内部等待。D0收尾后遵循v2当前顺序，Weapon作为H1消费者只核B1.01/B1.09/B1.10/B2.01/B3.02共同基准与自由双手、最终可见缺口；完整W2枪体跟随／权威射击不再是H1额外前置。W1/W2/W3和逐枪原需求保留，B3.10臂展配置仍归Weapon既有非共享范围但不立即并开；本D0交付不增加玩法完成分。

2026-10-27 D0实测后继：Body已唯一运行normal诊断1b02，子进程26436／实际主TID21832，6秒请求在外部30.032秒限时后强制结束exit1、normal_exit=false，无Game。Weapon只按新任务审这一份已存trace，八份来源实SHA封 `analysis/xr-startup-boundary-20261027/normal-setup-trace-review.json` SHAb0c5c0e0…dd83a。只见初始化边界ticket0/frame0/phase0/result_known0，它不是pending API；既有xr_create_instance_enter之后有层协商／初始化／扩展查询／app信息消息，没有completed或Poll/Wait/Begin/Locate/End边界。stdout0字节；按result.argv实际路径当前CSV存在0字节，不推文件创建／完成时标。阶段停留在合同明确的setup缺口，不能把缺completed单独当API内部阻塞或WGC/Wait/GPU/网络根因，更不能假定与旧失败同一原因。

D0首失败与下一动作：本次没有在原GetSystem支持日志前取得live PID/start birth回执，Body没有猜身份，WCT实际0次。Body仅在自己的运行harness补Popen后真实GetProcessTimes birth回执，再沿现有setup真实TID进行<=3次有界纯CPU/WCT，保留不暂停／不读内存或栈／不变权限／child1.5秒等原约束。Weapon未写runner身份增强、未重建／重测23项、未扩大SDK或其它case分析，候选／contract／runner均保持；正式新trace review已交Body/统筹。没有新H1/身体/双手/枪体动作验收，不增加玩法分，仍按已定共同基准与自由手顺序等新候选。


2026-10-27 D0 EndFrame源码窄审收尾：按Body给定本轮边界，normal1b02的pose_probe.cpp与已封manifest实hash一致；无render／无Game argv下，聚合初始化的layerCount=0/layers=null是源码推断，不是穿过API层后的参数捕获。当前CY runtime.cpp 7200-7518中，无projection仍调用preview准备、D3D12槽开始／关闭／同步／消费及status等尾部，因此不能据无layer排除graphics，也不能从App enter_intent断言进入了哪层或runtime内部。未扩大到SDK、插件反编译、旧案或新构建。

只读核对Body已经完成的三局并封正式receipt `analysis/xr-startup-boundary-20261027/endframe-source-and-isolation-review.json` SHA be66c9a950f9112743e5b968471b552fc4aa2fb1d8c385596485267d7eedb68b：normal-owned-identity／58284与child-only OBS-off／14128均有前两次EndFrame成功返回，第三次ticket22无闭合，分别30.156／30.203秒外部结束exit1；child-only OFXRBridge-off／9228使用同1b02、原6秒workload及30秒守护，11.438秒正常exit0，CSV533且533次EndFrame真实success，request-exit、space/session/instance销毁及cleanup/main-return标记完整。OBS disable key未与OFXR disable key同时设置；各局独立LOCALAPPDATA路径差异保留。结果支持沿该child-only条件继续Game启动验证，不定位插件内部原因、不称普遍修复。Weapon没有运行Game/XR、查询进程、读内存、改global/default/renderer、重建或新助手；审阅已交Body直接续作，不等重复确认。Body唯一执行后继Game/Source1启动及正常清理，随后回H1固定可见头身与自由双手；完整W2跟枪不额外挡H1。此无Game诊断不增加玩法分，Game／双眼画面／H1动作及头显验收仍待证。


2026-10-27 D0后继交接：Body报告 `analysis/body-startup-bridge-isolation-20261027ab/` 使用同child-only Bridge disable、原XR1a059／Native55ac，进入ready并执行45秒纯被动测试后，唯一finalizer accepted=true，Native/entry恢复、106、8pose、XR0、原10、audio/PG恢复及ownedPIDs空。此处为Body运行负责人回执，Weapon未重复审整局或重跑；本局无HandStart和动作，不能据此认定H1／双手／W2完成。Body已转H1固定可见头手，武器D0窄审收尾，后续消费同一候选的共同基准、可见自由手及一次交还证据。


2026-10-27 H1独立消费者交付（Body明确派单）：在ignored `analysis/h1-authored-eye-consumer-20261027/` 实现单文件纯Python `authored_eye_consumer.py` SHA b50516f805c45dedd5b16a29552fe1374586442b7b37e2bf4a62c6681ad5beea，接口 `authored_eye_from_reference(reference_bytes, head_world_row16)`。消费原coherent probe已经读到的115*68 Ref及实际Space55*CTW行矩阵，按准确Native qxyzw@12／translation@28／parent@56，原raw-q矩阵FK、头旋转逆变换与实际world头旋转计算authored eye midpoint；输出world眼点、head_to_eye_local、raw refhash/canonical及limits，替代观察器写死的EYE_LOCAL，不新增Game读取／IO／writer／相机位移／IPD推断。完整canonical签名锁Marcus115，严格count／parent／finite／q norm1e-3／proper similarity及原unit component容差2e-5；不授予当前live owner／namedRef／freshness许可。

H1来源与验证：准确candidate6的body_head_authored_mount.h、full_body_c0_planner.h、cover_skeleton_ik.h及hand_authored_mount_profile.h均实hash入handoff；保存Ref fef334cf…5ce与原namedRef join00511f9a…245e的115行完整raw／parent全等已核，头55／眼57,60由原实名证据认证而非序号猜测。只执行3项针对纯CPU检查，实际exit0、0.010秒：旧真实Ref与独立normalized quaternion FK oracle、输入不变与重复确定性；90／180度世界旋转和平移的解析预期；坏count/parent/非finite/q归一/未知geometry及错误世界矩阵拒绝。旧Ref重算局部偏移[4.862866949520468,-8.560140831466375,-0.00011918239688627906]，不是新live采样结论。正式 `handoff.json` SHA1b2c711071e7be8250a007f8a2de5caa8dca6da04c17eace6ed28893e42827e1，含单文件接口/来源/限制，已交Body可直接导入原probe，用world眼点替换常量并保留本局refhash。未改Root/共享Native、未建／开Game/XR或新增助手；真实H1静点／双眼／运动／头显仍由Body当前候选验证，不将CPU通过补作玩法分。


2026-10-27 H1 ac实测消费与纠正：准确b505眼点helper已接入Body `body-h1-fixed-runtime-20261027ac/body_observation.py`，接入预检实hash一致；实际 `joint-observations-live.json` 的17个coherent body行全部含真实reference计算结果，raw Ref均fef334cf…ed5ce、canonical均3b3c978c…5193，没有probe_error。此前将coordinator commands0／HandStart未触达误说成eyehelper未执行，现已按独立body观察纠正。17行只证明纯消费者实际执行，不证明第一人称画面、可见手、手部动作或对齐。Body封ac首读失败review f63eb7aa…ed5b0，Game52500/XR74652，首initial sample使用准确01d8 reader返回2、stdout matched:false、stderr空；commands/episodes0，无HandStart／图片。Body的106/XR0正常清理与无Start仍执行旧finally续更而超时分开，不能把后者写成Stop后回归。

按Body明确新任务，仅审准确01d8冻结源及ac既存记录，封 `analysis/stereo-reader-match-diagnostic-20261027/ac-source-rejection-review.json` SHA e8aa3ecb987585c399c0cfcd1d40fd68323e087bfbb5d64d1d47e76f17a522a7。exit2只来自原400ms循环结束：两bridge open成功，否则66；退出处未命中身份/存活65，但不能据此证明每次read/match成功。原循环短路且不记录拒绝，ControllerInputObserver的reason也没有导出；CSV291行只是另一路保存的视图数据，不能对应本reader某次GVC3 tuple/100ms年龄或publisher条件。现存证据无法唯一指出拒绝条件，不猜GPU／插件／freshness根因。

已获准的最小后继独立读器交付：`analysis/stereo-reader-match-diagnostic-20261027/build1/reader.exe` SHA57f112c9ad4cb52b1f69657688ca251cd190028c5afcb6b3016b1af279f8dc2f，准确01d8父版／原编译flags。只改private reader.cpp和ControllerInputObserver，新增diagheader，另7父文件byte不变；机械逆向精确恢复两原文件，实际compiler private includes均来自该副本。原400ms／Sleep5／100ms、调用及guard短路顺序、clock次数、stdout与退出码保持。每次原condition只执行一次，保存首次和最终拒绝stage、现成input reason及原match谓词实际首拒、原before/after与已读stereo／cached selected tuple；未执行字段有known位，内部stereo.read／frame_shape仍不具备的细项明确UNKNOWN。仅return2之前一次stderr `reader_match_timeout_v1`，有界1800B适配原coordinator stderr[-2000]，不加新读／retry／match／clock刷新，诊断存储仍可能扰动时序，不作为FPS／修复证据。

新读器仅构建一次；新针对CPU32断言通过：与冻结原predicate返回一致、每一首拒条件、短路不展开、first/last/UNKNOWN、记录长度与原afterclock。初测试枚举名误写，修正为准确MetaOperatorSimulated后仅重编该测试，原失败log/build记录保留；没有重复旧23项、Game/XR/新助手/Native构建或共享源码变更。contract SHA bd0578eeb3afc6abcde7a76b51bf55aea173a0f222035c77362fcd0bc6ca0c4e，manifest SHA bdbe36ad1fe20c0d5b2f5f741bffc0f59d3f5c092e45c0ef3324838da197221b，已直接交Body唯一下一局，不等待重复确认。

本轮吸收人类明确的第一人称6DoF验收顺序：同候选的实际XR双眼第一人称眼点／身体／腕目标，先保证静止自由手清楚可见，再做手动／头动。first_person_preview能力flag和旧局姿态不等于本局最终画面。Body唯一处理真实FP通路／view选择及Native，不叠两套偏移、不盲目前移；Weapon仅沿既定读器和Ref眼点消费者范围接入，不把完整W2枪体／射击／换弹增加为H1前置。当前没有新H1图像或动作通过，不增加玩法分。


2026-10-27 H1 ad当前消费者实际结果封存：Body采用准确reader57f112及原flags/400ms/100ms，`runtime-review.json`已保存initial_input与fixed_input均matched=true/source1，同session361892787067500，序列5238→5264，两手grip_active1/flags15；各原stdout输入sampled-received为5,408,800ns和3,657,300ns。rgrip_coordinator准确reader路径/实hash已核，reader_failures0；Startevent返回0，实际4条为左右自然固定与随后左右恢复。整体complete=false，首失败已经前进到fixed-free-hands的手提交／owned body view／coherent pose合取超时。此成功样本不证明诊断代码修复ac，ac具体首拒仍未获真实timeout行；Start0／指令已发不等于手求解、提交或可见通过。只读窄receipt `analysis/stereo-reader-match-diagnostic-20261027/ad-consumer-result.json` SHA07b471bb1f5b8352e3a9a85dcf042c8cbb75caf630ccec8b0842727fb7027607，列来源hash、两段实际输入时间及证据边界。

Body运行负责人进一步回执（本侧不重复审Native／FP／图像）：FP requested1/transition1/applied422，bodygoal applied617/view_used626，但hand attempts618/solved0/commits0且status8；实际XR projection双眼图仍有大块粉棕几何遮挡、手不清楚，H1不可用。唯一finalizer true、106/XR0，Stop4→5及两手恢复／bridgeclose由Body确认。武器侧冻结已可用57f112 reader与b505 eyehelper，不再追加读器／采样／构建／Game或W2，Body唯一定位status8及第一人称几何遮挡后按具体消费者缺口交接；当前不能补计第一人称／静止手／运动通过或玩法分。


2026-10-27 H1新明确数学消费者项（Body派单，非重复Native审阅）：仅用ad已存fixed-free-hands的41个coherent rows、fixed_input和准确55ac source/profile，计算臂长、肩线和眼/身体关系，封小JSON `analysis/h1-arm-geometry-20261027/geometry-summary.json` SHA390491cac1d12440d5ec9632f87e51cb076ee87382c459596bea7bf5f9b46c4d（3829B）；完整sources/hash与首末/范围在 `geometry-receipt.json` SHAaea09c8bf4eb054f7a59a95e65106129bf68415f95310e461147bdc64ac3ed76。41行与host-result/joint-live逐项相等，raw Ref fef334cf…ed5ce锁定，准确9份source均对candidate6 manifest实hash。两臂上臂约24.63014UU、前臂22.36483UU，总约46.99498UU；肩线相对authored-eye右轴偏38.87–44.69°，明显不对称扭身。已记录的是同一cohort几何，而非某次failed compose前的精确肩/目标快照。

条件性可达性：明确假设“实际HMD世界姿态==本cohort authoredEye marker、手适配器同基准/同fixed_input、requested100UU/m==latchedunits”，原free-hand mount和输入不变时，左目标距肩34.43–35.80UU，右54.36–55.67UU，右超原最大臂长7.36–8.67UU；同输入放在静态Ref模板下左右42.432/42.436UU，均小于46.995UU。这支持检查原扭身/肩基准、不是伸臂或减目标；它不是Native实际失败距离，也不是reference_neutral重放。实际render_base不能由CTW替代，尚缺失败invocation的base/source.reference/units/当次controller和pre-hand shoulder/computed_wrist/side。世界与marker局部距离双算共82组差<1e-7，无新测试框架／Game／XR／构建／Native改。

准确源进一步收窄：当前compose三条实际prepare_hand路径仅返回验证错误/invalid_target/okay，不会返回adapter::unreachable，故本55ac路径Status8具体可达来源为body_c0_hands.h:111的two::solve；侧以及outside/inside仍未序列化未知，不能据上述条件性右臂数据断言实际失败侧。原Posture::reference_neutral只在1/3/4/5/6/30/54替换静态Ref q，直接针对骨盆/脊柱/锁骨/颈基准，保留animated translation、长度、scale、foot/headgoal和handmount；部分已存骨矩阵不足以精确重放完整分支，不能保证actual reach/base/遮挡修复。小JSON与该边界已交Body直接续作，Body唯一改Native并处理FP遮挡；统筹已获知这是Body新派的独立数学项，W2/B3.10顺序不变。未取得新H1画面或手提交通过，不增加玩法分。


2026-10-27 人类新流程立即覆盖旧6DoF自动测试授权：按统筹与Body转达及AGENTS最新owner override，停止新增Game/XR/Simulator、测试用例、回放和自测；已有仅正常收尾，源码／编译／链接／打包可继续。此刻Weapon无自有运行，first-rejection消费者只保留未编译consumer.h，不再扩展或作交付门。交付顺序为A实际第一人称与统一眼/身基准、遮挡保手→B双手完整位置旋转→同候选编译打包→用户真机反馈后再后续；原全武器需求保留。

B独立代码实装：新增 `tools/vr-common/free_hand_eye_targets.h`，与Body约定直接消费同一次C0 a::Output.desired_eye（UE world Pose/full quat）及同一body_tick_source::Input.observed.stereo.head/controllers（原XR tracking Pose）。EyeFrame/TrackingFrame均携实际game pid/start/NativeStarted/worldGen/mesh/bindingGen/GT/Tick/Compose与stereo session/seq/space/refRevision、controller seq/inputEpoch/profile及predicted_time Key，按字段比同key并核原tuple，不查clock／IPC／Game或合成新票据。prepare_one对接每侧现有binding.authored_mount.calibration，prepare_both给独立known_mask/每侧原因；单侧无追踪不生成旧目标或污染另一侧。原始HMD逆变换控制器完整pose→原XR到UE约定一次→actual eye world一次→原静态grip-to-wrist mount一次，保留完整roll/旋转；不再加source.reference／HMD位移／legacy FP offset、不拟合动画腕、不解IK或伸骨、不抢动画交还／唯一writer。Root继续唯一A/共享Native/相机/骨骼整合。

B编译状态：最终keyed源SHA93e2c62d5df8210eb1293d7e647fe54b23dbbdbf4ef465843e20477cc00784d1；同MSVC14.51、C++17/EHsc/O2/W4/WX/MT，仅/TP /c实际编译obj成功exit0，没有创建／运行测试或回放exe，没有Game/XR/助手。原未带key版本编译保留在analysis/free-hand-eye-targets-20261027，最终编译在compile-keyed/，接口/源码/编译receipt与明确pending状态在keyed-integration-handoff.json。代码已实现/obj已编译与Body已整合/Native最终链接/同候选已交用户/真机反馈分别记录；后四项仍待完成，不能把编译当玩法验收，不加功能分。


2026-10-27 A/B代码整合后继（Body整合负责人回执，非本侧重跑）：A已在最新Junker/Support3 Native整合并compile/link exit0，B已采用本侧93e2c62d…784d1 free_hand_eye_targets、同次actual desired_eye／逐侧solved mask／固定骨长，compile/link exit0；本次仅源码和编译，没有测试运行。`analysis/free-hand-eye-targets-20261027/body-integration-receipt.json`把负责人回执与手动候选已交付／真机反馈分开；后两项仍未完成。

按Body打包所需，Weapon只读查明物理XR冻结来源：BQ candidate-source完整591文件实hash均匹配producer manifest83abf4be…07539，normal4174d0e3…0c1ead与loader66946ea6…1932df实hash；RuntimeTracked物理producer与sim-source／1a059区分。最新focus1 source-provenance11b0a84e…2c9ae的588文件也全匹配，两源完整586文件父树byte相等、均来自menu-modal1 37f463；共同内容冲突仅CMakeLists.txt/pose_probe.cpp，BQ新增5个controller/actions/continuity文件，focus新增2个focus_diagnostics文件，合并union593。建议保留最新focus1为底，只拷精确BQ5文件并合并两处重叠，保留focus event/presence和源手Actions单次采样／whole-loop tracker／continuity→controller→eyes次序／原清理；仅宏ON不够，手动物理入口必须--controller-input及实际stereoPID，不携带simulator-operator/CY override。精确路径/flags/identity/建议封 `physical-xr-source-handoff.json` SHAba5f61f44e5b3501cafb9a808a4406246d81352c76b83d8843dbb1eb76b506a8并已交Body直接续作。未改Root／共享代码、未新建助手／构建／Game／XR或测试；明确旧build_normal.py/cpu_checks.py/focus build.py含历史测试执行，当前不得直接运行，Body只编译OpenXRPoseProbe目标后打包给用户。编译／源码归因不等于真实手或第一人称画面验收。


## 2026-10-28：实际合并并接入固定统一手动入口

用户经统筹明确恢复继续，撤销上次H1交付后的等待条件；仍完全停止自动Game/XR/Simulator/test/replay/selftest，只源码、编译、链接、打包，由用户真机。统一开发目录6dof-cover工作树，创建codex/6dof-unified，从37de6ef实际merge f397f9b，合并提交9a41f92，双亲历史保留。13冲突处理后原未提交AGENTS从限定stash原样恢复，两个未跟踪trial及J主目录未提交支持工作未碰。无新助手或管理会话，Root独占Git index/提交/构建。

共享Native保留实际FP、同次eye、Ref115中立、固定长度／逐侧handoff，向同eye手部转换传入已确认held mount；配置held1+BodyMainMountSource1。现有肩／裤兜真实RGrip→装备Runtime→原库存和附着确认→typed持枪见证接入正常手动入口。缺臂展时复用原真实Tpose完成值的data-only Native查询，不捏造measurement，FP/自由手不等待新验收门。手动session在Native running后连接PG，正常停止先关闭fire/hand连接再原清理。

首次统一Native/XR/PG均compile/link exit0，仅指定生产目标，未运行程序或测试。PG含最新Junker源码三方合并与原车辆清理；修复合并重复声明/target和既有Junker局部变量遮蔽引起的WX兼容问题，保留失败日志。固定包入口J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe，版本6dof-unified-20261028-1；首包先交已接通FP/双手/取枪，unified_fire_ready=false，不启旧original_aim_only。Native458cdf…ba36b，XR6db1b9…3d727，PG4eea5d…84981。开火的实际muzzle/权威aim基准返回层由Weapon在统一目录限定文件继续实现，Root统一接入，不把它当首包交付前置，也不静默回退原版瞄准并报完成。

详细状态见docs/6dof-unified-manual-20261028.md。源码已接通／编译／包交付／用户真机验收分列，原成熟度分数未凭编译增加，四步代码链继续推进；后续换弹、掩体和其他武器扩展仍待办。


### 同日统一候选2：接完Lancer实际枪口开火代码

没有停在首包等反馈。Weapon在统一目录限定三文件落实现有Lancer实际Muzzle getter采样／typed返回消费者，Root统一接入单一Native call subscriber begin/normal return/cleanup，保留128B Journal并为不确定返回写入增加sticky位，防止后续unwind清除事务债务。射线前后起点及基础方向来自同shot冻结的实际Muzzle，外层原误差与后续原碰撞/伤害流程保留；原original各一次。callback无Getter/Client Gate/rig读取/锁/参数消费，无第二骨骼或枪体writer。

Root修正anonymous namespace include造成的ABI forward遮蔽与局部original名称遮蔽WX；最终normal PostTick还要求实际同源single/two射击快照（完整key、weapon/world/controller/pawn/mesh），避免左手unknown时允许press而返回层全部拒绝后悄然走原瞄准。此状态暂时拒绝开火，不假定左手丢失时已支持单手。预备源时效和原stamps保持，既有Scene/Native身份保护不放松。

仅PG新目标compile/link exit0（含失败和修正记录），最终PG SHA80086cf51dd9b57f666ed108524f9a7069441e6e297c2f0dd6c6cca7df943f1c；Native458cdf…ba36b/XR6db1b9…3d727原产物复用。版本6dof-unified-20261028-2，unified_fire_ready=true。明确socket局部+X为待用户方向反馈的配置，HUD独立准星尚未接新射线，未声称画面/弹着/手感/停止或真机已通过。完整边界见docs/6dof-unified-muzzle-candidate-20261028.md。没有启动任何Game/XR/Simulator/test/selftest/replay/preflight。后续不扩散换弹、掩体、多武器；以用户真机反馈继续修正当前链。


### 2026-10-28：用户手动启动34的窄修复与候选3

用户反馈进不去；两次实际reserve日志均在playground_reservation返回InitializationError34，Steam EXE验证通过，Native未激活/XR未启动。已先把整个旧候选、两次first-failure和用户config原始字节快照到忽略目录analysis/6dof-unified-startup34-20261028/。当前用户选择真实SteamLibrary副本，完整路径历史和hud_horizontal_fov_degrees=65须保留，不用模板覆盖。

唯一生产源码差分playground.cpp：HAND_ANIMATION_CALL_PROBE下的test_environment门仅对明确UNIFIED_MANUAL_PROFILE=1且使用Native WEAPON_CALL_SUBSCRIBER的手动档不要求测试环境；旧实验门、EXE/进程/64字节入口/注册/生命周期等保护保留。顺查Reserve/Initialize/Enable/WeaponHandStart/VrFireEnable相邻手动链；R2已有手动档分支，其余未启用实验/诊断入口仍保留原门。没有向正常启动器注入静音或置后变量。

仅编译受影响GearsPlayground目标，最终exit0；新PG 4f8a5dfaab0d191d45f2867fb084a6f414172933d43d1f1487844a14620b1036。Native458cdf…ba36b、XR6db1b9…3d727复用未改产物，未运行任何Game/XR/Simulator/test/selftest/replay/preflight。候选6dof-unified-20261028-3保留用户全部config与旧包/日志，固定入口继续6dof-unified。源码拒绝条件已修正，下一次真实启动及玩法仍待用户确认；既有HUD/左手unknown/socket轴限制不变。详见docs/6dof-unified-startup34-fix-20261028.md。


### 2026-10-28：用户v3会话退出——重复Capture键的Python修正

v3真实手动记录continuous-stereo-60532-20261028-021200-e9bfcdb8顶层error为Capture.first_person_preview重复，实际UTF16 INI第22/55行各一次。Native已激活、PG读到level_player、XR随后正常停机、Native cleanup_verified=true；旧34已越过，但不据此声称画面/头显通过或断言原生crash。修改前完整v3/logs/config/失败INI保存到ignored analysis/6dof-unified-duplicate-ini-20261028/。

只修两处Python生成方：hud_size_ini唯一写first_person_preview，手动档明确True；sixdof_manual_profile.CAPTURE移除该共享键，仅留四个身体专属键。未改strict ConfigParser，未跳过world_rebind。实际INI顺查无其他重复键/节，AST读取常量确认profile键/节唯一；没有执行生成器/测试用例。v4身份与manifest/metadata/inventory同步，三个Native458cdf…ba36b/XR6db1b9…3d727/PG4f8a5d…b1036原样复用，无重编译及Game/XR/Sim/test/selftest/replay/preflight运行。

当前Steam路径、全部用户config/路径历史/FOV65逐字节保留，旧包和日志保留；发布不停止用户进程。候选6dof-unified-20261028-4仍为源码/打包修复，下一次用户启动与原四步真机表现pending。详见docs/6dof-unified-duplicate-ini-fix-20261028.md。
