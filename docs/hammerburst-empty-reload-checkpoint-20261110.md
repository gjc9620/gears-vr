# 兽族步枪普通空仓换弹候选检查点

目标仍为完整兽族步枪开发和验证。本检查点不代表整枪完成或头显验收。

当前候选 hammerburst-prepared-20261110-000625-empty-first-failure，prepared SHA256 2f53b54a1cd435270551e4c176a9a1bb0b3725966f59af07bf063a18b9e6427c。PG a05a6c3ce20b4fa6a82f41a03a38ab690b69543d84193ad96c3b75f8619f0d5c，Native 806dcbd6a92ecd337923f4ecf315e56ab7afd475e247a8a3cacdfe2ab30b8b0c；公共 v71 未改。

生产修复是已有手动换弹权限分类补入 HB，使原 CanReload 拦截可覆盖本枪。编译和离线分类检查已过；真实回调尚待本候选实测。严格空仓脚本遇到第一次锁扣拒绝即保存并停止，禁止旧版恢复输入重试。

本次目标链：核心六发后 L72/R78，十二次真实六连发至 L0/R78，一次空扳机验证不补弹、不产生射线且真实 veto blocked；随后真实非零 reload serial、原模型骨姿开盖、旧匣提取释放、肩取新匣、座入关盖，L78/R0，再六发回射至 L72/R0。首败停止。尚未运行，不把包装器退出零等同动作通过。

Boom 独立受理当前 v3，同一原排队位；证据 analysis/boomshot-development-20261109/successor-HB230535-admission-v3-check.json，SHA256 c94e2ba7357e9e5e4efbb6d5bf92c2e31a270323d4d284fedd5f6ec9d1697529。当前无最终交窗、无 claim/ticket、未 stage/run。必须等 Boom 新实际运行及完整正常清理、审片、编码、十基线恢复与保留收尾后的唯一明确交接。

入口 v3 SHA256 d60ba49bc8aaa71c0ca1b45e80223a03fa1144703da26fa549b7d29dd313734c；ready 4edac018f8651d7b6677445458f61b83631f98c1165f1ff299a7207b62665c5d；plan b7286f9d4712a866a72ef7ecd607f9fb029a74d9d2eebf18b5ac28f54deac371。精确 prepare-only 已通过。v2/v3 除四个固定身份字符串外 AST 相同；内层实际 Python 环境检查通过，消除继承优化及 safe-path 干扰。源及两项复核程序见相邻 source.patch。此检查不启动游戏或获取运行权。

本次另独立检查唯一后继 Frag232727 v3 的最终交接身份、精确十路径基线及子进程环境隔离，并实际仅执行 prepare-only：8860 host files、冻结源/六制品/运行依赖 bad=[]、六 import 检查通过，队列字节不变、真实 claim 不存在。证据 analysis/hammerburst-20261108/successor-Frag232727-admission-v3-check.json SHA256 7159cb8b1060a88761643c0549591bcadb183f1beb09a10a62bac62be079b91d。仅登记为本枪未来完整收尾后的唯一后继，不授当前执行权。

上一轮 50148 的真实开盖骨姿失败与 serial0/提前补弹观察保留。当前候选不继承旧 PG 的动作通过结论。手型贴合、完整换弹、全部中断条件及头显感受仍未验收；本轮不扩展耗尽总备弹和无限离线手型拟合。

## 运行前证据边界复核

当前源码确认 needs_reload 为未满弹匣且有备弹（或无限备弹），只有该分支调用原始 begin 并取得 serial。50148 的 L78/R0/serial0 走 visual_only 检视分支，因此其骨姿失败不能证明真正空仓原始换弹分支同样失败。保持真实骨姿阈值，不据此提前改变动画实现；下一局必须先观察非零原始 serial。五组纯谓词例子见 analysis/hammerburst-20261108/empty-reload-source-path-audit-20261110.json，不是引擎验证。

另发现现有 empty driver 的 manual_veto_consumption 是全局累计日志，blocked>0 没有本次触发前后增量、武器或 press 绑定。因此只能称本局曾发生原始 CanReload 拦截，不足以独立证明本次空扳机回调；未来报告必须把该弱证据与同枪 typed L0/R78 前后不变、零新射线的可观察行为分开。本次不改已受理候选、不重冻结或扩大实测，保留此限制，不将弱证据升级为单次调用证明。

## 护木四态旋转回归

在现有 weapon_hammerburst_support_test.cpp 中新增五种枪旋转姿态，使用本枪真实已编写握点坐标和原生产 SupportGripParticipant/SupportSocket mode。每种依次检查 Snap/single、移动枪与手后 Snap/single、按 Grip Grab/two、移动后保持 Grab/two、释放回 Snap/single、离开范围 Free/single；Snap 必须无租约，观察不能改写物理 Frame。MSVC /W4 /WX 且 assert 开启，编译0、执行0。原四态和11拒绝检查保留。receipt: analysis/hammerburst-20261108/support-authored-rotations-check.json。该回归只证明合成输入下策略与仲裁，不证明游戏枪变换消费、皮肤接触或头显自然手型；未更换当前已受理运行候选及制品。

## 冻结候选同组护木回归

后续核对发现当前工作区 weapon_support_grip.h 已加入后续的可选 SupportBox 及接触缓存，而本枪2f53冻结候选仍是先前球形接触实现。为避免把工作区检查冒充运行制品检查，把同一测试复制到独立 harness，仅将三处顶层 include 指向候选原存档目录，由该目录解析其全部相对头文件；原存档未改。五旋转/四态/11拒绝及物理 Frame 不变检查在冻结头文件下再次编译0、执行0。当前与冻结两者的结果均为有限离线策略证据。源码差异、harness、build.cmd 和各 SHA 收据在 analysis/hammerburst-20261108/support-frozen-rotations/，result.json SHA256 0912949caf3fa725e46d8e68d27dec24e90b1124c2198a7f8fd2ed619f624845。未替换制品、未重新申请运行位、未 stage/run。

## 005428 实际一次启动失败及独立恢复

收到Boom新game24964完整收尾交接后，独立核收据33d4a30c、七原文证据及六原图SHA、十基线、idle与精确prepare-only0，再保存本轮即时十项raw baseline。入口v3真实一次消费并stage；launcher30156/creation134387168841386715在创建游戏前失败：graphics_policy.locate报Multiple settings bindings for the selected installation，reservation_failure_cleanup=no_birth_receipt。未生成游戏、未注入Native、未启动XR/动作/双路录像。启动器exit1；外层随后另KeyError session_report/exit1，两个失败分开保留，无自动重试。

config原有反斜杠P2P key，既有Longshot selection helper又按本枪plan斜杠字符串添加同一安装key；规范化后两个匹配，原唯一绑定保护正确拒绝。独立idle后通过原restore helper精确恢复十项原字节，public只读核；launcher进程已终。一次retention正常完成、删0，保留24964/47996/9764。没有录像则审片/编码不适用，不称encode0或游戏cleanup0。证据analysis/hammerburst-20261108/startup-005428-failure/review.json SHA498f1e012bde619dd2dfb8b7c011926d5bcf18f323e1927a95417ac6b8593612，含原session/config/reserve/done/claim与恢复收据。

新增HB自有选择器，仅在明确owner选择的安装中按与graphics_policy一致的resolve/casefold替换路径别名，保留其他安装绑定，拒绝非法映射。不改共享Longshot helper、不改graphics_policy保护。实际原graphics_policy.locate在临时fixture目录下复现旧函数双别名失败，修复后10例通过：两种slash/case、不同目标的重复别名、无绑定、其他安装、幂等及非法映射拒绝；真实配置/INI未修改、未启动游戏。源码和复现见相邻settings-binding-repair.patch。旧2f53候选已消费，尚未采用新helper、不复用旧ticket/claim；后续需新冻结身份并沿真实尾重新排队。

## 后续入口草稿的无出生失败收尾

新入口v4草稿增加已观察到的无session_report分支：仅在启动器非零退出、done/worker/launcher身份对应、remaining_owned_game明确None、phase=incomplete、无game_identity且no_birth_receipt时可进入；先独立idle，再通过原restore恢复十raw，输出startup_prebirth_verified失败收据且返回非零。不把它当游戏cleanup成功，也不在证据不足时恢复或重试。实际本次归档及12个错误归属/状态反例共13项校验通过；只执行提取的真实校验函数，未导入入口、未stage/restore/game。源码与复现见相邻prebirth-failure-entry.patch。草稿hammerburst-prepared-20261110-010515-settings-alias尚未冻结/受理，不能执行；需同版本GN依赖确认及新身份。原候选和已消费claim/ticket不改。

## 新settings-alias候选仅封存，尚未排队

GN确认4文件版本稳定，原说明SHA4423b4d04cbf0821ef2d500c77525484b6c136c3827006776045105d2f597b5c已核。新候选hammerburst-prepared-20261110-010515-settings-alias：prepared a186945574e57260852df87aca2be769d4db19301843a5932273e35b570ae112，plan aa75ebf064afa17894b1bbd1154575e264aa822f2a9f446d7b27686b7d447704，ready d0a1844503c03d51b86218a84e4aa29ac3a3a66d14027499d05c61cc2a93e632，entry 14ce87a843677ea3985dda7be1b2f1424ea0ff70ef7b1efc2ceb02d45dc73ef9。2270归档源、1338 Native源、六制品核SHA；PGa05a/Native806不变。完整1057 Python边界仅更新四个GN已确认原件，其他1053逐项不变，校验未删除；HB20直接入口不变。采用本枪自己的settings helper，避免依赖仍在编辑的LS辅助函数。

实际精确primary -E -s -B -X utf8 --prepare-only退出0，未查询共享runtime、未stage/start/claim/ticket。新v4完整复制入口八种前置失败与八进程竞争检查通过（仅临时根，1消费7拒绝），真实无出生校验13例通过。入口/冻结复现原文见相邻settings-alias-freeze-source.patch。当前request_ready=false；尚未向尾申请。旧槽仍仅持交接收尾，等待Frag已确认的最小依赖替换，并不利用旧窗口再次运行HB。

## 无出生失败最终交接完成

Frag同槽v4替换仅更新289边界中的两GN稳定原件、计划边界引用和固定入口身份；独立核全部289、差异、精确准备命令退出0，真实claim不存在。最终唯一交接startup-005428-final-handoff-Frag232727-v4.json SHAab032fb57d639d5828ee831bd9648295ce985d7bbd43b37dfc1fb332b7573eff，绑定prepared9d5/plan8182/runtime-ready64f8/entryc8ce/执行ready8edf。无birth收尾范围明确，不冒称游戏cleanup或编码0；十raw与idle再次核过。HB已释放窗口，停止共享运行文件写入。

HB新候选在完成旧交接后仅把request_ready标记改true，prepared/plan/entry不变；最终ready SHA c4315976ff9302b074155d241c3e5379e3d8005e648177347856208d9b3dfe96。原实际prepare-only观察仍保留对应旧metadata SHA，不冒称新运行。当前已沿Frag唯一后继链请求真实尾定位，尚未受理新位。完整单一packet为analysis/hammerburst-20261108/admission-v4/tail-packet.md。

## 新候选唯一请求已提交HOD实际尾

HOD线程01a1bf64-8db6-7a20-9953-0e4a76532222确认其equipment-controls-prepared-20261110-hod-beam7-tomb execution051fe4654985183467ba7a694af8b7343ad30ddbde82c13b7770c92606a6a4eb为Bol之后已受理实际尾。仅向它提交一份新HB申请，不授窗/不stage。不可变完整packet: analysis/hammerburst-20261108/admission-v4/hod051-request-packet.json SHA8cc4eac75dee0b158305b853193ad8c266d1656582eed1c17140524b4cbcacef，包含完整身份/命令/环境/依赖/固定claim与ticket/精确十raw和异常合同。最终readyc431重新实际精确prepare-only0，无runtime query/claim/ticket；收据hod051-request-prepare-only.json。当前settings-alias-queue.json状态requested_waiting_HOD051_review，未受理。未来只有该HOD本轮新实际运行全终后的明确唯一交接可授窗，旧13204及其他前序终态均不能借用。更晚LS意向须待HB正式成为尾后才由HB按到序核验，不新增并行位置。

## HOD051同槽必要入口修补（游戏候选不变）

实际尾独立复现旧admit可借13204旧review及OPENXR_UNDECLARED/GEARSUNDECLARED/PYTHONUNDECLARED残留，原执行包未受理且未消费。本次只派生执行入口v5，compiled prepared仍a186、PGa05a/Native806、20动作driver/1057边界/13011host全保留。新plan a2888238a00f918fa95a0273f66a07c2315b3f1bf8e769bafad85e13da7d8beb，ready a606e1aee5101be66bf16055dd1717a85abf7b58f16ecdfc56993e4a73a49891，entry edbdbae69e10b749dfd6cd67fafa61ace80ff56c719bdefdd0138b66480188a3。完整原实现源及合同见相邻hod051-boundary-repair.patch。

新入口解析并绑定当前HOD051实际固定claim、terminal result、原before-stage基线身份及十raw、同一真实game的正常cleanup/launcher done/双路亲审/encode/retention原JSON与独立review。旧13204/其他前驱/仅idle/活跃或无终态claim拒绝。HB新基线保存前序raw handoff/claim/terminal/baseline四SHA；同一首消费claim在原字节验证后fsync绑定自身身份与基线证据，并在stage和两条restore前复核。环境在prepare/idle/stop/全部子进程之前按upper清GEARS*/PYTHON*/XR*/OPENXR*，再覆原固定ENV，SystemRoot/PATH保留。

真实提取函数32个精确seal后的语义正反例、实际旧13204 receipt的admit反例、真实main全部环境AST节点以及同一claim落盘/篡改拒绝均过。完整复制入口8前置失败、prepare不消费及8进程race1/7通过；旧无birth validator与新函数AST一致，不伪称重新运行原13例。精确v5 prepare-only退出0，无runtime query、真实claim或ticket。不可变替换packet: analysis/hammerburst-20261108/admission-v5/hod051-request-packet.json SHA763fc762b94e513396a25ebfacb135174894a75ce094627f6dca864f3003849e。仍同一到序、待尾定向复核，无新运行权；未来HOD若异常不满足合同则失败关闭，不造终态证明。

## HOD051正式仅受理HB v5（未授窗）

已亲核HOD受理收据successor-HB010515-v5-accepted.json SHA4a5ade264b1985e932239bdf19bc9417fcf59cf2dc5747928f2491b495f51332及独核4e71858523510568ac9f46e60b238114ac631c26ef4839e5317efd036941ef76，所有引用pins/packet/合同一致。仅在HB自身settings-alias-queue记录accepted_by=HOD线程，state=accepted_waiting_HOD051_actual_final_handoff，仍无final/window/stage/run；不改HOD记录。本枪当前关键下一步仍是真实空仓换弹回射，不把受理或prepare当玩法完成。

按已到序仅核更晚LS011500的入口边界，真实AST临时fixture复现新LS身份可配旧HB005428给Frag的handoff通过window gate，以及OPENXR_UNDECLARED/GEARSUNDECLARED残留；首次提取误选bootstrap导致NameError在os.open前即停，无真实入口导入或claim；改为明确window赋值节点后得到上述两反例。证据successor-LS011500-source-review.json SHA6915c8e5813506f14c7b17f2979640cb08e457bfb5a799b5598ccd299b563df3。已请求两类必要同槽源修，未受理/授窗、未调用其prepare或host全核；其后GN意向只保留既有顺序，尚不另行受理。

## 实际HOD49912取消的精确接收分支（未获交窗）

HOD051实际game49912/creation134387215205673057停在原Contacting Gears of War services，未章节/Native/Beam；session cancelled/native_started=false，Native cleanup字段未激活不观察，XR退出1而terminal_exiting/objects_destroyed/shutdown_completed真、EndSession失败。launcher35524原PG正常shutdown0/prime entry恢复，pad恢复；HOD原十raw及九private文件/集合恢复、实际审片/encode0/retention0已记录。本人亲看1全跨度overview+6raw并核7图SHA，确认标题至联网画面、无章节武器动作；编码输出/日志/recorder收据及九保存原件SHA已核，不冒称交接前已查当前私有文件或OS活进程。

只新增HOD051-49912-134387215205673057这一实际case，锁定原claimcfb312a0/terminal97d4fe95/baselinea66e1447/review2d5d4076及7项实际原证据哈希、launcher/XR出生身份。正常接收函数AST与已受理v5原函数完全一致，普通Native/XR0规则不改。取消分支要求如实normal_cleanup_verified=false/startup_cancelled_cleanup_verified=true，单独核原PG/prime/pad、XR真实销毁/无forced cleanup、原十raw和九private当前字节及集合，接收时fresh OS查固定4PID全无，不能用“当前idle”或自述代替。例外case marker与四SHA同样durable绑HB baseline/首claim。

基于原归档状态与临时重定向的私有副本、仅OS观察替身，34个正反例及正常dispatch通过；未读写当前共享runtime或当前private文件，未实际执行process query。正常函数AST一致，完整复制入口8前置失败/prepare不消费/8进程race1/7通过，不重跑旧GN/C++。新执行-only v6：prepared仍a1869455，plan93c9130f1e53b3b957ace6b336518b3de7f694214cf1f449ae2e13db0342a930，readyf7ee0986b2000c7d61e28e68b68aa2f5ca826d5bb88d23ac22ced222dde165ca，entryb85850bfe2d7294bbc7cd28d09913b6de8e09742d62ca879bc0b730c83282397，合同89e3fe2caf8438a86f7f6905fe18984e03827319b806a021887d5bc31e6b518f。实际exact primary prepare-only0，无runtime query/stage/claim/ticket。源与精确合同见相邻hod49912-cancelled-admission.patch。首个未发布packet记录ready继承别名，已退役；最终packet见后续精确记录；仍同槽待HOD独核/新raw final，尚未接收窗口。

发布前HOD指出ready两顶层alias继承旧v3，而entry/plan/实际invocation始终为v6；只修ready这两字段，最终ready SHA f7ee0986b2000c7d61e28e68b68aa2f5ca826d5bb88d23ac22ced222dde165ca，plan93c9130f/entryb85850bf/contract89e3保持不变。最终metadata实际prepare-only0，无runtime查询或真实claim/ticket。未发布初包保留并显式retired；最终不可变packet admission-v6/hod051-request-packet-final.json SHA2083a2564b6b3c6035ab2a02ee3a9b1f6333dd0bf6fc971b420b826204e08e87。

## 实际13712：第五轮有限连发不完整，空仓换弹未到达

HOD49912实际最终证据独立复核后，HB v6单次消费真实claim并运行所选原游戏13712/creation134387238055131993。PGa05a6c3c、Native806dcbd6、XR16840b27及原game bdc6705d保持冻结，未借旧attempt。肩部取得与core实际6条一致射线通过；loaded72/reserve78/cap78。后续四轮各6发依次72→66→60→54→48，第五轮仅48→47、实际1发，首次失败即停，operator退出1。总31条实际射线与used31一致，trace_valid/write_source_consistent/trace_source_consistent均真。普通空仓触发、原reload serial、翻盖、取旧匣/装新匣和换弹后回射均未尝试，不能称空仓换弹已通过。

原日志frame269显示preparation rejection时source age94.3283ms；20ms准备余量在100ms边界内不足。记录的stage8/follow_stage6同时也是上次成功端点，跳过准备可能保留旧值，不能将它们直接当本次拒绝根因。随后source/owned变化与held-chain unavailable仍须定位，不能凭图像或时序断言已因果修复。保留100ms/20ms/250ms、原时间戳、release/anti-replay与原回调次数，未延长输入、重试或同窗补跑。

实际33.5秒双路全部2页overview及6张按事件QPC选取原图已亲阅，摄取结束覆盖完整；存在近脸枪身遮挡，手型/全角度/物理头显均未接受。wrapper0，Native/entries/pad/drain正常、XR0、launcher0、game已退出，原十raw存在性与字节恢复。单次编码15493实际terminal0；单次retention83968实际terminal0，保留13712/49912/7760、48小时，删除2489媒体/441941574bytes，errors空，源/文本/数值保留。未重复编码或留存。34个原文本/JSON/数值文件1699177bytes有SHA归档。

实际证据集中在analysis/hammerburst-20261108/runtime-13712-{first-failure-analysis,personal-paired-review,encode-exit,retention,restoration,closeout-review}.json及runtime-13712-evidence/archive-manifest.json。旧v3后继入口实际late-body helper覆盖缺陷由LS自行发现并撤回，当前仅同一LS011500槽v4替换；HB独立核14函数AST不变及真实最后load/sha对str/Path均通过，exact Jvenv prepare-only0，未消费LS claim/ticket。正式释放窗口仍须最终新idle/十raw核验与唯一v4 final。

原版枪栓条件需求已按原模型与三段原动画核查，详hammerburst-development-20261108.md对应章节与hammerburst-bolt-original-audit.json：当前一代HB只有Root/MagLatch/Mag，无独立外露可拉机柄或后拉/回位轨道；按原锁扣/翻盖和弹匣机制继续，不增虚构拉栓步骤。该结论不宣称虚构枪械内部没有枪栓。

## 13712首次失败定位源码（尚未实跑）

在原HB after_tick同一短路顺序上补失败条件编号1无frame、2非running、3已cancel、4目标不符、5右手tracking、6当前装备拒绝、7已commit后need_source拒绝；只记录已有POD，最多16条，不额外读game/控制器或时钟，不授input。need_source同步记录自身已执行到的阶段，避免把旧准备成功stage当本次原因。原tx::revoke/release调用和先后关系保持，100ms/20ms/250ms、原回调及单writer未变。这是观测修补，不是已证实的少发修复。

MSVC /W4 /WX /UNDEBUG实际编译并执行两个受影响夹具退出0：actual completion bridge297、actual event adapter213，包括新增六种首次失败条件和既有tracking条件断言，确认没有增加原shot调用。夹具game/engine/certificate/dispatcher有替身，不能算游戏或头显验证。结果与源码SHA保存analysis/hammerburst-20261108/continuation-rejection-r1/result.json。当前未构建新完整PG、未stage/实跑；既有已消费冻结候选完全未改，公共v71未改。下一步从冻结PG派生隔离构建，再按实际尾申请变更后的单次定位验证，目标仍完整普通空仓换弹回射。

## 续段拒绝定位完整隔离构建与制品封存

continuation-rejection-isolated-r1从manual-veto-isolated原2253文件逐项复制，仅替换两HB inl；原CMake flags原样重定向私有source/build，configure0、完整Release GearsPlayground编译实际terminal0。新PG SHA f48d50d4ab0b991f2b69979225b4b68624368657d8ce498d8b43b3d846c37f90。冻结product hammerburst-prepared-20261110-continuation13712-r1，prepared SHA cdbce75e7cdbfe263484c9d665f66f234cfad47979536741e75436ff14576c24，归档2270源/1338 Native源/6制品全SHA核过；Native806d及其他五制品未改。

源对应核查首次过严要求全部文档/Python等同旧隔离树，实际六个差异为旧隔离构建后合法更新的文档和Python driver，故在任何copy/封存前停止；随后逐项确认全部C++/h/inl/CMake编译输入一致。新product继承settings-alias的普通空仓完整动作driver，未回退到隔离树旧Python。新event fixture也归档，实际编译输入仅两inl差异。没有改旧product或公共包，没有stage/run/claim/ticket。prepared-check.json保存精确结果。尚待按实际唯一尾绑定新执行计划和新一次性入口，request_ready=false，不复用旧消费许可；已向当前LS唯一持窗会话只发一次新批尾定位意向，不计游戏验收。

## 新批真实路由与离线执行依赖核验

原LS v5-repair早已实际game37448全收尾交GN，再GN21120→Frag2956→Bol；不再沿旧LS持窗状态推断许可。本轮经LS明确指向Bol，Bol实际47300当前持窗、正核Beam8唯一具体包，明确仅将HB continuation13712-r1后到意向列在Beam8→Cover-r80→Bow034115→Troika045716/core2→LS050140→Snub051707→Frag064450之后，未独核受理/未授窗。pending-intent.json保存此边界，无第二槽，无共享runtime写入。

原20直接运行文件、1057 Python当前/归档及AST、13011主机依赖均重新逐项通过，2270/1338源与6bin再次核过。runtime-plan-draft.json明确offline_draft_not_runnable：entry/invocation/expected_predecessor/claim/ticket均null，execution_ready/request_ready均false；原selected游戏/设置、十raw目标、动作范围和360秒正常stop边界保留。offline-runtime-readiness.json是实际离线观察，不称prepare-only新入口、窗口等待进程或game结果。目标保持active，真实尾身份到位后再一次绑定冻结执行包；少发根因、完整空仓换弹回射仍未证实。

## 实际日志输出路径检查

补测原report_continuation_rejection的真实fwprintf到临时FILE并逐行读回，20次实际调用精确只有16行，gate/source_stage/frame和first/firing/cancel字段正确，原shot调用数不增加。MSVC /W4 /WX /UNDEBUG编译执行0：event231、runtime297；仅测试源变化，生产两inl/已封存product及完整PG完全未变，不重新构建PG或变更已发布product哈希。actual-log-output-check.json保存独立新检查，不覆盖先前冻结引用的213原收据。此项是日志观测可靠性，非少发原因修复或游戏通过。

## 全目标证据边界核对

重新读取用户四态/弹匣完整范围、W02机构卡和逐部件合同，直接检查70816/7712/53224/13712原JSON，生成analysis/hammerburst-20261108/full-requirements-audit-20261110.json十三项及原件SHA。历史护木四态/原匣/替换功能证据保留，不移作新PG证明；当前有限连发失败、空仓未达、完美/取消/中断/步骤引导完整图像与逐部件全角度/头显未接受均保留。枪栓条件要求按原资产核查解决，不意味着其余项目通过。当前仅优先少发定位→普通空仓完整换弹回射，耗尽和无边界拟合仍不扩展。没有为审计启动测试或修改产品。

## 13712同场六次真实按压时序对照

重新解析原PG日志31条实际轨迹与status3/4观测，以同一递增trigger press/release六轮分组：前五轮（含core）均6发，末发在真实UP诊断观测后335.513/411.326/391.775/312.420/316.278ms；最后失败轮仅shot31，首发后272.377ms才观测UP。成功首发延迟91.031–107.263ms，失败110.385ms；各轮DOWN→UP诊断观测294.351–430.435ms，失败382.762ms在其内。实际同场已有UP后完整续段，不支持用“松太早”或一概首发延迟异常解释失败，不能以延长按压作为修复。诊断观测不等于实际物理dispatch时刻，具体取消/身份/current-source根因仍不明。原件SHA与分组方法见runtime-13712-burst-timing-comparison.json。本次纯读原证据，不修改候选/输入/保护或新增运行。

## 原调用顺序发现释放观测缺口并补齐

直接核冻结weapon_lancer_r2_native.inl：input.advance先经execute调用lancerrefire::release_completed(source)，后面才HB after_tick；HB release_completed可能已清掉outstanding，故单after_tick观测不足以捕捉最初拒绝。给原release_completed十六个原&&条件附失败编号，仍按原短路顺序、同样一次原read_game/frame_active/clock/current/need_source，不新增engine读取或回调；拒绝时仅已有POD有限16行，再原revoke/cancel。实际297runtime/237event夹具编译执行0，新增null-frame/trigger仍DOWN/current拒绝三个条件及无额外shot断言；原UP续段测试仍过。未称根因已修。

原r1 product保留不改、尚无执行资格，已向Bol仅更新同一到序意向：待新r2隔离构建封存后单一替换，不增加队位。当前新release源码尚未完整PG构建/实跑，不得将旧f48d制品当作含该新增记录。下一步构建/封存新r2并补实际失败记录输出测试，再按真实尾绑定一次入口。

## r2完整PG与实际释放日志验证

新隔离continuation-rejection-isolated-r2完整configure/build实际0，PG SHA daee9726fa4f7da7b684e97907f1e31c8180e3493ab1a7557d0c227b8da6dd21。2253原文件对r1仅event inl变动，原flags不变。新增真实release_completed调用20次写入临时FILE并读回，精确16条gate1/frame0记录、无附加shot调用；runtime297/event258实际编译执行0。原r1产品/收据不改，当前仅准备新r2同一意向替换，没有claim/ticket/stage/game，少发根因仍未实证修复。

新冻结product hammerburst-prepared-20261110-continuation13712-r2 prepared SHA80b12d98fca4900fde12bee32d793752e6009d51600d191bec036185daeb7dcf，源2270/Native1338/6bin全核。已向Bol只更新同一原后到意向，r1不请求执行，不增位。entry/claim/ticket均未创建、request_ready=false，待真实尾后绑定；pending-intent.json记录替换与原到序。

## r2精确冻结头文件夹具验证

将受影响runtime/event两个测试直接从r2 product/source编译，所有项目头与inl均由MSVC /showIncludes实际输出解析，逐项要求路径位于该冻结source且SHA等prepared map，禁止根worktree头混入。/W4 /WX /UNDEBUG编译执行0，runtime297/event258仍通过；continuation-rejection-frozen-tests-r2/result.json保存实际include闭包与SHA。没有修改已封存product、重建PG或启动游戏，原engine/certificate/dispatcher替身限制仍在。这项补齐前面root测试与实际候选版本的一致性，不增加玩法接受范围。

## 新失败记录的离线解释边界

新增纯函数analyze_hb_continuation_rejection.py读取有限失败行，不访问进程/输入/游戏。release仅gate16、continuation仅gate7表示当前真正执行need_source，其余路径把source_stage标为历史值而不归因；cancel already requested不被当成根因。23个已知gate与重复/无效字段反例、实际13712旧原日志无新记录不造诊断，共30项通过（重复字段与独立无效值均拒绝）。原日志没有新增记录不能据缺失判通过。receipt-reader-check.json保存源码SHA/合成边界与实际旧日志观察。本工具不进入生产driver/不改候选或已冻结Python边界。

## 主人最新独立执行要求：撤销远期交接依赖

收到主人明确覆盖旧链条流程的要求：各枪独立推进，只在实际启动协调占用，动作结束立即正常停止并核恢复，离线编码/审片/保留不占共享使用权。HB已停止等待远期前后继/互审/票据，派生run_hammerburst_standalone_r2.py及独立plan/ready；原v6保护/原十raw目标/选择路径/环境清理/固定时效/库存/原动作和正常停止不改。旧HOD取消例外与整个前驱读取依赖均不带入。新首次O_EXCL claim绑定本次启动前fsync原件，stage/restore前复核，恢复后queue立即window_owned=false。prepare-only精确primary参数实际0，无claim/stage/启动；运行前仍须简短核对当前真实共享访问/正在启动者，沿既有launcher互斥，不能靠旧idle或旧终态抢启动。

## 独立入口首次尝试：真实外部占用拒绝，未stage

Bow53536实际正常恢复释放后，HB重新核对四PID无并告知即将启动。独立入口55716在原idle处发现另一实际Steam game39644（2026-11-10 14:53:34 +08出生），退出1；没有prestage目录/基线、stage/config/bin写入或HB game/输入。不是ffmpeg误判；原真实占用保护保留。独立核55716已终，O_EXCL原claim consumed_unverified保留不可复用，occupied-start-refusal.json保存实际事实。无HB共享占用或需恢复的改动，不停止他人游戏；以后确认当前使用者释放后以新的独立attempt再启动，不借本消费claim。目标active，未增加玩法通过。

## 独立53484实际：78轨迹至空仓，首次阻断是driver判据错误

新attempt2未借旧claim：游戏53484/creation134387673793929071，PG daee9726/Native806d；实际全部78条原始一致射线、L72/R78→L0/R78，空仓真实trigger后无射线且L0/R78不变、无自动装填。此前13712少发未在本轮重现，不称因果修复。实际operator45852退出1，错误等待8秒to1；wrapper0不能当玩法通过。原PG日志实际存在reload_begin serial1/accepted_ms78664253及transition from1 to2/serial1/L0R78。生产begin分支直接设acquiring并return，transition日志只在后续step变化时写，所以不会有to1；旧测试条件不符合真实生产回执。修driver只核本次fresh begin非零serial＋同一serial真实1→2与L0R78，后续实际开/取/插/关/返射要求不删、不延时。提取真实改后driver函数，实际原件positive/旧to1不存在及9个拒绝条件共11项通过；没有改引擎逻辑或补库存。

所有Native/entries/pad/drain/XR/launcher正常，十raw恢复全true，own runtime16716终态0，已第一时间释放并通知Bow/LS；LS随后已独立使用并结束，不等HB离线工作。共享done/worker/session/原PG日志已复制到runtime-53484-evidence，双路capture停止后硬链接隔离到runtime-53484-offline，不改原图，独立encode85217实际terminal0；两页全跨度overview与6张按QPC取样实际图亲阅。自然手型/右眼近枪遮挡/启动第三人称等未接受，按主人新三阶段要求不阻挡流程推进。媒体retention尚待独立处理，不占游戏。完整replacement/close/return-fire未执行，整体未完成。

## 独立r3等待真实恢复与统一stage锁

Lancer35276实际退出、launcher finished后，Bow补充GN早先stage后未生game仍需恢复十原件。HB没有claim/stage，已直接通知GN完成既有恢复，仅等实际共享释放，不要求前驱资格。采用统一tools/debug-launch/runtime_stage_guard.py RuntimeGuard('solo-m1-1028d')，固定同一OS mutex覆盖consume→原件保存→stage/config→启动→正常停止恢复，prepare-only在锁外只读；busy不消费claim，原launcher内层Owner锁保持。guard源码SHA进入entry_dependencies，没有另造互不相认锁。

必要准备检查发现公共run_v63_startup_repro.py后来仅新增Lancer logic-chain动作分派和该scope成功即stop、失败文字reason变化；HB empty-normal scope不变。已定向阅读三hunk，只有这一个直接/1057边界文件变化，其余不变。新runtime boundary独立复制到standalone-r3，不改已冻结product；新实际prepare-only0，无claim/stage/game。新优先级明确第一阶段逻辑全流程、第二体验、第三逐部件GrabPose，保留最终范围。

## r3实际占用拒绝（未stage）

GN十原件实际恢复释放后，第一次统一guard争用Busy，HB没有消费；真实持有者为HOD beam9/53920，不是HB或Bol。其实际运行终态/正常十原件恢复及空闲已观察后再试，HB取得guard，但原idle发现Longshot55436/XR34068/录制43704/operator27712已在用；HB54984在原件保存前拒绝退出，claim consumed_unverified保留、无prestage/部署/共享写入。已简短告LS正常完成、随后使用同一外层guard；不打断其运行，不据单一锁代替真实占用。后续独立入口把O_EXCL消费放到guard内且真实idle通过之后，避免纯争用/已占用检查消耗attempt；仍在任何baseline/stage前，不能复用旧54984消费记录。

## 独立52960：真实空仓替换至L78/R0，最后关盖gap拒绝

GN恢复全终、HOD退出后本枪r3曾取得guard但实际Longshot55436占用，在任何原件保存/部署前拒绝，已保留54984消费原件。r4将消费移至guard内且实际idle之后、任何原件保存前；Longshot18原件实际恢复释放后r4取得锁执行游戏52960/creation134387690462061371。完整78条实际trace均valid/source一致；空仓触发无射线/无自动补弹。修正后的真实begin serial1及1→2通过，实际阶段依次2→3→4→5→6→7，取出旧匣、释放/肩取新匣并入座后库存L78/R0。没有跳过原机构。

首次失败为closing stage7/reason5：previous_ms80358027→sample_ms80358139=112ms，大于既有100ms；已接近闭合angle0.0242935rad，previous seek12.9137ms，retained_same1、hand_delta1.783mm。严格首拒绝停止，无恢复重抓或重复输入，真实关盖和返射未执行，不称完整流程通过。下一步核采样/消费路径，不改100ms、不绕闭合端点。game/Native/entries/pad/drain/XR/launcher正常，十raw全恢复，统一guard及时释放并已通知；随后Bow独立使用54588，本枪离线工作不持窗。

本枪共享易变文本先隔离归档，随后capture硬链接独立目录。初始目录遍历遇另一interrupted stub重名而中断，已明确只选唯一dual-output目录完成隔离，没有覆盖原动作记录。编码73228实际terminal0，全部76.2秒三页双路overview亲阅；六张实际事件原图补审仍待完成，不伪称已阅。runtime-52960-review.json及runtime-sidecar-archive.json保存实际数值/原件SHA；媒体留存待独立安全处理。主枪遮挡/自然手型未接受，按主人第一阶段逻辑优先，不拿它们阻止该gap修复。

## 关盖112ms缺口的原样本连续性方向（尚未live接线）

复核hand_sample使用原controllers.sequence/sync_ns，local_hand由同一两只原raw grip和固定枪挂点求局部米坐标。公共controller continuity已有8点原producer ring与controller_relative_motion::build，保留每段100ms、同press/release/tracking/publisher约束；不必创造新writer或时间戳。不能照搬Longshot私有history context作为HB证明，HB仍须独立核固定mount和当前rig/source一致性。

新增HB专用纯机械weapon_hammerburst_latch_history.h：调用者必须先鉴权原ring/同抓取/固定挂点和当前Frame；输入含之前原点、中间原点及精确当前点，逐点通过原stroke.observe（retain_stationary=false），不插值、不延长100ms、不产生Frame/lease/seek/bone回执，只有最终当前点结果可交给原消费者，任一点拒绝撤销、不提交部分推进。合成实际112ms形状测试证明直接跨gap仍拒绝、两段原56ms可闭合，8种缺点/过期/乱序/换token/失追/不同基线/端点不符/几何异常均拒绝。MSVC /W4 /WX /UNDEBUG编译执行0；latch-history-r1/result.json保存源SHA及边界。这不是已取得真实历史或游戏修复，尚未include入生产PG，当前候选不改。下一步HB独立current-context/raw-ring绑定与实际原样本证据，不以纯测试声称关盖通过。
