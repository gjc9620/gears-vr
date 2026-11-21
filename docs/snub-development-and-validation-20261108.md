## 35aae584按下源窗口观测已编译；032540未获准入（2026-11-11）

- 48108实际记录的DOWN命令21.6604585s、UP命令22.0213648s，间隔360.9063ms。命令日志只证明发送时序，不证明游戏接受。原234至237来源缺口仍待实测，不改有效期/按键账本。
- 私有35aae584仅在原R2两个copy入口附带已有FrameReadObservation，保留最近512条内存记录，原正常release/drain达到stopped后才写文件；没有新增源读取、输入/动作调用或hot-path文件IO。移除观测语句后其余源文本与10e完全相同，公共283文件不变。Optional receipt自身有诊断计算开销，时序结果不得称零开销性能证明。当前冻结头文件没有新共享版本的两个commit时钟字段，已去除引用，输出负1表示未知；初次编译失败日志保留，最终完整编译0。
- 源码c815d612已保存，032540新实例实际Python预检0、60秒首次准入ResourceUnavailable，无worker/launcher/Game/XR。该名字已消费不重用，不作候选运行通过/失败。目标active，41.25%不变；最新实际游戏仍48108，完整逻辑、换弹、贴合、头显未接受。

## 10e86a1a实际48108：腰间取枪通过，首个350ms扳机无弹道（2026-11-11）

- 等待精确Gnasher进程退出后，共享入口获准；031419/Game48108/creation134388117600726093，XR38380/creation134388117686285277。取枪成功，实际Snub装弹12、备弹36，原HasInfiniteSpareAmmo两次为false。首个350ms真实DOWN/UP后弹道0，装弹仍12；未进入换弹，不作完整逻辑通过或取枪稳定性已修复结论。
- 中立UP233/6696已观察。随后234 copied-source但109.6515ms拒绝、continuity_failure10；235 equipment held_chain_observation unavailable；236 main hold成功；237有效源已为UP、pressed1/released1/value0。未见accepted DOWN或deferred pre-original marker。稀疏变更日志不足以证明234至237每个Native样本/早期入口拒绝原因，不能只归因35ms reserve，也不能用延长按下或放宽100ms补测冒充修复。
- 操作1，stop0/launcher0，Native/entries/XR/input/hands/game清理均true、无强制终止。2190前台样本/857自身cloak检查，违规和错误均空。本方全部工具终止。50帧25.152秒编码0，亲自查看16格全段双眼总览和末帧49，手部间歇缺失保留，完整贴合及头显仍未验收。
- 候选本身已真实运行，下一步核对按下在pre-original/post-equipment边界未获接受的位置；必要时只补缺失拒绝/原时钟证据。公共/手动入口不变，目标active，41.25%不提高。

## 10e86a1a已保存028b6e3e；031111首次准入60秒结束，无进程登记（2026-11-11）

- Python检查0，canonical first-admission wait默认60秒后ResourceUnavailable，未有isolated-worker/log/launcher/done，只有本次start-request。没有Game/XR/控制输入/新玩法结果，不作SourceScope候选失败或通过。031111名字已消费，后续必须新prepare，不删除标记或重复同一start。
- 原始最新游戏仍56796，正常清理；当前本方所有工具终止。最新source候选10e与24守卫检查已保存028b6e3e，完整目标active。资源结束记录单独保留admission-031111-review.json。

## 10e86a1a已保存5f86df68；025711首次准入60秒结束，无进程登记（2026-11-11）

- Python检查0，canonical first-admission wait默认60秒后ResourceUnavailable，未有isolated-worker/log/launcher/done，只有本次start-request。没有Game/XR/控制输入/新玩法结果，不作SourceScope候选失败或通过。025711名字已消费，后续必须新prepare，不删除标记或重复同一start。
- 原始最新游戏仍56796，正常清理；当前本方所有工具终止。最新source候选10e与24守卫检查已保存5f86df68，完整目标active。资源结束记录单独保留admission-025711-review.json。

## f608实际56796只读分支130hits但取枪仍失败；10e86a1a源校验范围候选已编译（2026-11-11，最新）

- 022424/Game56796/creation134388094258301919口袋未获held回执，仍原Weap_LocustAssaultRifle，未射击其他武器，首个目标即失败。新观察路径确实完成1次qualification/4原slot查询、130hits/0reject，action_permission0；只能称分支覆盖，不能称修复取枪/性能量已证明。多次Frame读取8/5和inventory current_after/held_chain_observation失效、baseline重建保留，未取得每次after全时钟/epoch，原因不归单一时钟。
- 操作1/stop0/launcher0，Native/entries/XR/input/hands/game正常，无强制，2034前台样本/832自身cloak检查无违规错误。55帧29.074秒编码0，亲自查看16格全段双眼总览及末帧54，自由手缺失区段保留，不作稳定输出或贴合接受。
- 10e86a1a只将既有SourceValidation Scope的opt-in扩展到已知未持枪、非busy/animation、model未held/switch、baseline已立、无pending/future/held publication/known、原左右press计数相同且两个右控件均中立时。它仅复用原每次双读全部字节、变化/容量/故障回退或失败的boolean source-current witness；原完整Frame/Game观察、输入和动作权限/所有时钟独立保持。Scope实现仅改注释，公共common/Runtime/action尾部/Native/XR均不变。完整编译0，实际入口守卫与真实Frame/EquipmentState/Model类型24项检查通过；不是实际observer/运行省时或取枪接受。初始fixture缺少GripGameState头文件已补齐，产品代码未为此更改，失败日志保留。
- 新025711有限350ms原全流程已准备，import和prepare-only通过，10e未游戏记录。共享首次准入新e6b4/7db0已按原9f35/b1a4副本逐段核对：只在spawn前名额/内存/短锁忙有界等待，默认60秒且0..600，睡眠不持锁/不占名额，每次锁内重查fresh/pause/disk/精确worker身份/内存；一旦进入commit任何错误不重试，worker/cleanup和原disk/receipt函数原样。独占start-request使一次请求结束后旧名字不能重用；本方runner也增加该拒绝，后续只使用新名，不绕过此标记。不是Gameplay重试循环或放宽资源保护。
- 原完整逻辑、快速点射、新匣实际扣减、修正拉套、完整贴合/头显仍未接受；goal active、41.25%保持。

## f608d327已保存d4c3e639，022424尚未获准启动（2026-11-11）

- 完整源/制品/import/prepare-only及资源前Python检查均通过；原内存准入因两个登记worker拒绝。等待已结束的Troika/HoD后重新准入仍被其他实例占用，无新Game/XR/输入。保留各次真实拒绝日志，后续必须重新核对资源。当前本方构建/测试/编码/运行工具均已终止，f608没有游戏效果声明。

## 51824口袋取枪未获回执；f608d327未持枪中立观察候选已编译（2026-11-11，最新）

- 014205/Game51824/creation134388073348062987正常启动，Python预检0。口袋取枪未获held/slot0回执，当前仍Weap_LocustAssaultRifle，未射击其他武器，按首个目标失败停止。没有到达Snub idle-muzzle新分支、350ms扳机或换弹，不称94dda改善了游戏耗时。
- 接触窗口内frame175/4636 copy failure8、input age97.7266ms；随后177/183/188/191/195等出现inventory current_after拒绝及重复baseline。未记录每次inventory调用分段/所有时钟，不能把current_after全部断言为时钟或唯一取枪根因。已明确源码在无动作时反复执行完整4slot/ammo/selection原查询，作为可验证的减负方向。
- f608d327只修改PG的weapon_equipment_runtime.h：原input cohort/计数器未变、两个控制均已知中立、未持枪/未切换、无hold publication/known、无pending/future时，提前读取一次原输入；仅当没有事件且cursor未动，复用既有经过4次原slot查询和双chain/原script复核的只读库存观察。未知/负值/NaN通道、任一新press计数、排队事件、pending/future均全读或原样拒绝；所有取枪/切换/声音/新hold动作尾部逐字不改，源100ms和动作期限不变。
- 新轻量路径明确不调用idle_unheld显示隐藏回调，不能用缺少selection权限的观察结果触发显示写入。审查新增回调测试先在初稿7e3重现该边界遗漏，之后补上排除及model.switch_pending守卫。7e3未实跑，DLL备份保留；原header从精确反向补丁重建并与当时SHA一致。finalf608完整编译0，真实Runtime/Frame/Model共133场景1662检查通过，包含只读快路、每种拒绝/完整读取、真实新press、回调失效和显示写入隔离。fixture初始误期待非法Frame仍应读库存，已按真实Frame validator要求先拒绝，保留失败日志；未放宽产品检查。
- 51824 operator1/stop0/launcher0，Native/entries/XR/input/hands/game全正常，无强制；1634前台样本/640自身cloak检查、违规错误空。42帧/21.142秒编码0，亲自查看16格全段双眼总览及末帧41；画面只有自由手，不当Snub贴合或取枪通过。
- 新022424有限350ms原全流程已准备，import和prepare-only通过。Native9650/XR1684、公共common源/原库存certifier/原动作尾部不变。f608尚无游戏证据；完整逻辑、短按可靠性、实际新匣扣减、修正拉套、完整贴合和头显未接受，goal active、41.25%保持。

## 94dda277候选已保存763dc874，尚未获准启动（2026-11-11）

- 014205资源前Python检查0，原资源准入因已有两个worker而拒绝。Cover正常结束后其他实例先取得剩余名额；最近以精确PID55720/creation134388065473993857只读句柄等待Bow25秒，返回仍活。记录为有时效的实际资源等待，下一次必须重新核对。没有新Game/XR/输入或候选功能结果。
- 当前本方运行/构建/测试/编码工具均已终止。最新实际游戏仍55400主握过期，正常清理；94dda候选、350ms案例、修正肩部新分支及后续空枪/拉套仍未游戏接受。原完整目标保持active。

## 55400主握检查跨过100ms；94dda277减少闲置枪口准备候选已编译（2026-11-11，最新）

- 011915/Game55400/creation134388056484297562正常启动，Python资源前检查0。口袋取枪后、首发前frame280/sequence3578 hold_frame_after失败：entry sync age87.7455ms、binding5.8024ms、完整observe10.0106ms、total15.815ms、exit103.5605ms、callback→entry62.1841ms。实际350ms Down尚未发送；不当长按/肩部候选分支覆盖。
- operator1按first main hold停止；stop0/launcher0，Native/entries/XR/input/hands/game全正常，无强制结束。1208前台样本/474自身cloak检查、违规错误空。37帧/18.520秒编码0，亲自查看16格全段双眼总览及末帧36，握枪掉出后的自由手画面保留。
- 源码发现pre_original_held_shot在中立UP更新原ledger后，仍每个新Frame完整prepare枪口；这与posttick避免闲置socket开销的原设计不一致。94dda277仅对Snub实际已知中立UP、无owned/awaiting-release/deferred/aim时提早返回，省去未使用的枪口准备。真实Down或任何待处理按下/松开/瞄准仍走完整原路径；UP ledger观察仍在跳过之前。未延长35ms reserve/100ms/250ms，未授予缓存射线新Frame权限，其他武器路径及公共common/实际Input/原ledger不改。
- 完整私有PG编译0，实际新增guard+真实Input/原ledger共2568检查通过，包含pending/owned/aim/未知analog/非Snub拒绝及原延期/松开验证。初始fixture误把.21当已按下，已改用原release_threshold+.01并保留失败日志，产品阈值未变。没有游戏节省时长或修复通过声明。
- 014205新有限350ms全流程候选已准备、import和prepare-only通过。共享cinematic_anchor实际差异为CameraActor class发现提示及额外fresh name/number复核，非旧Git历史中的相邻DWORD优化；已按当前文件精确差异审阅并只绑定已知f3c8哈希，其余原registry/Class/property/owner/code/layout源码逐字保持。首次未审阅版本不符和错误差异假设均fail closed，无游戏/输入。Native/XR及publicv71不变。
- 当前空间已充分恢复；本方另清理87个已完成可重建私有Git临时index（logical784050018bytes），保留最近3，全部refs/共享HEAD/index清理前后校验相同。随后保存c36时共享HEAD并发变更，稳定检查失败但私有commit已生成；另逐个核对1975个本方路径与commit blob完全一致，不称共享状态保存期间未变。
- 完整逻辑、短按可靠性、肩部候选新分支、有限插入库存扣减、修正拉套、全手贴合和实体头显仍未接受；goal active、41.25%保持。

## 005439启动前Python探针失败；空转本方watcher已核对并关闭，新增资源前只读检查（2026-11-11，最新）

- 取得资源后005439 launcher47292/creation134388039925860934在Start-DebugGame.ps1第48行失败：GEARSVR_PYTHON不能确认可用、no game startup actions。脚本顺序证明发生在$game/EXE/实际创建前，native_activated=false、game/xr=None、no_birth_receipt。350ms真实输入案例未运行，无玩法证据增加；不是射击/换弹失败。
- supervisor正常请求stop后返回handoff timeout；worker45164/creation134388039924915145因没有game identity而永远等待unknown children，原isolated-done未生成，原始不完整收尾保留。审计核对精确身份/命令、launcher已退出、唯一附属conhost55904且没有其子进程、Python错误代码在游戏创建路径前；随后仅强制关闭本方孤立Python观察进程，其控制台自然退出。显式watcher_force_terminated=true/game_force_terminated=false，不称正常游戏清理通过，不触碰其他worker或伪造done/Game身份。
- 相同解释器的原PythonCheckOnly当前通过；先前探针究竟超时/输出/异常未细分记录，原因未证明。私有run-snub-isolated现在在申请worker前执行原只读PythonCheckOnly，核对返回解释器精确路径和原脚本哈希；失败不占名额。原真正启动仍自行检查，没有绕过后续步骤，也不能保证瞬时故障不再出现。
- 执行实际新增preflight AST片段至worker入口之前：真实原脚本返回check_only=true、正确解释器路径，未执行资源入口或游戏，源码语法通过。再次fresh prepare因3967000576<4GiB拒绝，未有新已准备实例。当前本方Game/XR/runner/worker/conhost/编码/构建均无存活任务。
- 最新实际玩法仍47940：首发与原匣回插通过，180ms回插射击无轨迹。348肩部新弹匣分支/350ms辨别/有限库存插入扣减/修正拉套/完整手部贴合和头显仍未接受。完整goal active、41.25%保持。

## 350ms独立辨别案例005439已准备，资源等待已实际核对（2026-11-11，最新）

- 同348abd25/9650/1684，有限场地、完整换弹脚本不变。仅私有真实Down保持350ms再实际Up，增加每次按住恰好一条实际轨迹检查及quick_tap_accepted=false标签；移除这三项后与47940 core逐字一致，reload逐字相同。范围检查通过，先保存30a5a5c6。
- 未启动Game/XR或输入。实际等待Longshot/Boom worker进程，退出后仍重新执行完整源/制品/磁盘/内存检查；先后遇到磁盘低于4GiB或两个已登记实例占满，没有调整门槛/抢占/排队锁。最后对Bow15872/creation134388037855717220只读句柄等待25秒，确认仍活；该观察有时效，后续必须重新验证，不能把状态文件当活进程证明。
- 本方所有构建/测试/编码/运行/资源检查工具已终止，005439无launcher-session，当前实际玩法证据仍是47940短按无轨迹。较长按住尚未游戏记录，不把准备工作当功能通过。完整目标active、41.25%保持。

## 348abd25实际47940首发/原匣通过，回插短按延期后松开，无射击（2026-11-11，最新）

- 003940/Game47940/creation134388028198385427，原生备用弹每次false。取枪、首发12→11、原匣回插loaded11/reserve36/bonus0保持通过；inspection_return_fire没有实际轨迹，loaded11保持，正确保留失败。新肩部proposal分支尚未到达，不称已修复或已回归。
- frame607/sequence4916真实Down2，execute_not_ready sync age75.7021ms，原35ms reserve不足，R2 status10。下一frame608/4923实际Up2，源sync114105314809300，取消pending；没有snub_deferred_pre_original标记。此次没有一个到达现有提前消费点、仍保持Down的后续样本，不能以此认定该分支已覆盖或有效。
- 更早565/566/567在原匣座入/完成附近的consumer withdrawal差异4、8及100.6916ms stale另存原日志；之后snub_reload_complete result1且新鲜UP604/606已恢复，不把较早现象直接当607首发失败根因。
- 操作1因无轨迹断言，stop0/launcher0，Native/entries/XR/input/hands/game正常退出；1639前台样本/689自身cloak检查，违规/错误空。80帧/40.514秒编码0，亲自查看16格全段双眼总览及末帧79。完整逻辑、快速点射、修正拉套、提前消费、全手贴合和头显仍未接受，41.25%不变。
- 下一辨别案例仅私有真实按住350ms、再真实松开，并额外要求每次按住只产生一条实际轨迹；保留180ms失败为open，不把较长按住当快速点射修复。无重试/新Down、原250ms intent、100ms源、35ms reserve及release规则不变。新prepare因3749494784<4GiB被拒，尚无新的已准备实例或游戏，变体补丁只保存未执行。
- 按原全局newest3/48h规则，清理本方已亲审46576/57576/57196旧像素258文件63448369bytes；源码/日志/数值保留。retention现在也识别已审spare-policy报告，未扩展删除范围到其他会话。

## 348abd25启动前磁盘拒绝，未创建新游戏（2026-11-11）

- 已先保存54633bb3，随后003940启动被原磁盘门槛拒绝：3902205952 <4294967296 bytes；尚无launcher-session，无新Game/XR/输入/录像。全部本方运行、构建、测试、编码工具已终止。不是新候选游戏失败，也不算通过；仍等待资源满足后进行原有限库存完整流程。

## 有限库存57196原匣通过；肩部新弹匣同帧分类候选348abd25（2026-11-11，最新）

- 002819/Game57196/creation134388018341552838：原生备用弹标志每次两读为false；实际取枪、首发、原匣回插不补弹/不奖励、再射通过。战术原匣实际拔出并释放，新弹匣已接纳stage4→5，但尚未插入，有限库存扣减未验收。
- 更早的读消费者失败frame781/sequence4506/token3，reason7/game_difference1=request_ammo，age65.5649ms，candidate_flags65=first grant；随后frame782/4508出现stale_before100.5981ms，脚本即停。两者按先后保留，不能把后一个当最早失败，也未证明前者造成后一超时。
- 原step注释要求肩部新物件重分类等下一Native publication，但propose在held_after_frame相等时跳过incumbent分支，又因stage已变new_magazine而立即失去request_ammo。348abd25仅在同一个原frame、同token/有效ammo lease/同opening identity/new_magazine阶段重新计算实际肩部候选；下一frame继续旧incumbent分支。严格HandClient/首次grant/prepare/commit、源时钟/100ms、输入、库存、机械及公共common源均未改。
- 完整私有PG编译0；提取实际新增分支并使用真实lease_valid和strict-reader比较的25项检查通过（几何及inventory identity为受控fixture，不是游戏证据）。新003940有限场地全流程已准备，尚未启动新候选；未宣称修复已在游戏证明。
- 101帧/51.253秒编码0，亲自查看16格全段双眼总览及末帧100。operator1是上述源过期停止；stop0/launcher0，Native/entries/XR/input/hands/game均正常，1618前台样本/688cloak检查无违规错误。完整逻辑/修正拉套/早消费分支/完整手部贴合/头显仍未通过，41.25%保持。

## 实际原生无限备用弹=true；有限弹药完整流程已准备（2026-11-11，最新）

- Game57576/creation134388016021800548，实例001841实际口袋取枪，原生HasInfiniteSpareAmmo连续两次均true，loaded12/reserve36。仅策略查询，无射击/换弹；operator0、stop0、launcher0，Native/entries/XR/input/hands/game正常恢复，无强制结束。1292前台样本/516自身cloak检查，违规/错误空。
- 当前冻结2c源码playground_controller.inl原有infinite(weapon)默认设置场地武器bInfiniteSpareAmmo=true；原GEARSVR_TEST_FINITE_RANGE_AMMO=1且behind/mute/observe全开才进入有限路径。独立迁移的controlled_environment没有此开关，与46576备用弹不减现象一致；57576证明相同场地配置确实无限，不反推46576瞬时标志为已直接记录。
- 新实例002819保持生产二进制不变，启用现有有限场地开关；私有读数每次原生查询两遍、要求false才继续，完整原匣/战术/空枪/拉套动作和库存断言不变。prepare-only通过，未启动。不是新生产弹药写入器，也未降低有限扣减标准。
- 43帧/21.53秒编码0，亲自检查16格全段总览及末帧42；15.91至18.46秒样本出现双眼菜单，随后恢复枪和双手，保留此现象，不接受连续渲染稳定或手部贴合。完整逻辑、修正拉套、早消费分支、实体头显仍未验收，41.25%不变。

## 原生备用弹规则只读核对已备妥，启动前磁盘准入拒绝（2026-11-11 00:24）

- 新实例snub-20261111-001841采用相同2c387f73游戏实现，私有操作只执行原口袋取枪和HasInfiniteSpareAmmo两次查询。无射击、换弹、弹药写入；报告显式policy_query_only/gameplay_verified=false。prepare-only通过，源代码先保存21b7d5ac。
- 启动前可用3836219392字节低于既有4GiB要求，未创建Game/XR/worker，保留拒绝记录。没有玩法重跑。旧的磁盘准备拒绝001737也无游戏，不作产品失败。
- 仅对本会话已停止的独立harness及其Native/PG/XR文件做NTFS可逆压缩；原制品/脚本/JSON哈希前后相同，未删除源或二进制。清理旧已审阅像素共163文件41989418字节。余量仍波动，继续遵守原准入，不调整阈值或抢占别人窗口。当前无本方运行/编码/构建任务。Goal保持active，41.25%不变。

## 2c387f73原匣闭环通过，战术库存断言待核对原生无限备用弹规则（2026-11-11，最新）

- Game46576/creation134388000392035549，实例snub-20261110-232708：口袋取Snub、首发12→11、原匣拔出回插loaded11/reserve36/bonus0保持、回插后再射11→10通过。原匣及战术两次抓取均使用当前part-neutral原源记录（检查时约52.88/52.02ms），实际获得部件并到达原机械插入完成；新准备记录路径获得游戏覆盖。
- 战术换匣前loaded10/reserve36，完成后loaded12/reserve36；有限库存断言预期reserve34而失败。旧私有读数器未记录HasInfiniteSpareAmmo，因此不能认定生产代码复制弹药。生产写入器本已按原生无限规则保留备用弹、有限规则扣除；下一步仅直接读取同一原生规则，不放宽有限断言，不写弹药策略。
- 本轮没有execute_not_ready/status10或snub_deferred_pre_original标记；首发普通路径通过，不能称2c提前消费分支已游戏验证。战术回插后射击、空枪和修正后的拉套尚未到达。
- 108帧/54.73秒、编码0；亲自查看16格全段双眼总览及末帧107。手枪可见，稀疏画面不能接受全角度手部贴合。背景检查1834前台样本/779自身cloak检查，违规与错误为空；operator1是上述断言，stop0/launcher0，Native/entries/XR/input/hands/game全部正常清理，无强制结束。41.25%、完整逻辑/实体头显接受状态不变。

## 已准备2c387f73，等待实际资源释放；本方运行器支持一次身份绑定等待（2026-11-10，最新）

- 232708仍无launcher-session、未启动Game。资源准入原样多次拒绝，依次实际等过Bow/HB/Smoke/HoD/Troika等已登记worker退出，空位被其他实例取得；没有新游戏或重复玩法测试，没有抢占或改共享资源边界。
- 只读确认较早lancer-natural-v139-225928 worker5584/creation134387965860868429仍活着，launcher57912已不在；该实例23:03:38记录startup reservation失败，session phase=incomplete/game_identity=None，worker log空。不能据此证明其未知子进程全无，不关闭或释放他人占用；只保留资源观察，不当Snub产品失败。
- 现有本方run-snub-isolated.py新增可选--wait-worker，仅以原worker PID/creation/image验证后打开只读SYNCHRONIZE句柄，最多等待25秒，超时打印未启动即返回；退出后再做全部源/制品/背景/暂停/未使用实例检查，随后仍走共享磁盘/内存准入。不持资源锁，不自动重试，不停止对方、不造输入、无排队票据。Python解析与prepare-only通过；Game中的2c提前消费分支仍未验证，41.25%不变。

## 2c387f73已延期Snub按下提前消费候选；96e1实跑首发延期后松开，无射击（2026-11-10，最新）

- 96e1/sn ub-20261110-225556（实际实例名snub-20261110-225556）Game25676/creation134387963104223188取枪成功，首发无轨迹/loaded12保持。frame269首次execute_not_ready：原35ms reserve、sync age80.622ms；R2 status10实际deferred，frame270再次not_ready age71.8176ms，271真实松开取消。首次实际覆盖此前adeb的既有延期路径，正确保留失败；不是首发通过、也不是未消费Down。
- 较晚frame299 hold_frame_after单列：entry sync age97.3666ms，内部2.5505ms（binding.5683/完整observe1.9806）、exit sync age99.9171ms。此前进度口头误读为25.50ms并误称首发通过，现按原始数据更正。Frame还检查stereo/received等时钟，本轮未记录各时钟，不能把sync<100的失败强称该sync超过100；不据此改Runtime保留规则。新的part-ready分支尚未走到。
- 当前2c387f73只加Input只读pending getter，并在现有pre_original_held_shot中，对上一轮已接纳延期、较新原sync、当前同武器/hold-generation、已发布/基线稳定且无装备待办/新Grip计数/左手动作/瞄准拥有的Snub调用完整原vrfire::tick。首次Down仍post-equipment；250ms original intent/100ms源/35ms reserve/松开/反重放/原ledger均不变。完整tick体与父版逐字相同；公共283源及22导出不变，完整编译0。
- 实际Input/原ledger与从native提取的实际guard、真实EquipmentState/Model离线2537检查通过：80.622ms not_ready→较新40ms源只执行一次，post不重复；same source及装备/左手/身份/hold-generation等保守拒绝。Native绑定/type/engine未由离线替代，尚无此新消费分支的游戏证据。测试fixture的shadow警告、向后源时钟及行注释错误均已纠正并保留失败日志，不当产品失败。
- 96e1终态launcher0、Native/入口/XR/输入/hand/game全恢复，无强杀/子进程；stop工具因session JSON短暂PermissionError退出1单列，不能改写stop0。516次CLOAK/1332前台采样无违规/错误，275未绑定身份保留。39帧/19.85秒GPU、16格及末帧审阅、编码0，自然/头显未接受。
- 旧共享时期本方录像retention现纳入实际independent done排序；按全局newest3/48h清理5503已审阅/终态像素1,066,383,828bytes，全部source/制品/文本数值保留，旧图不再可下载。一次新prepare因磁盘<4GiB拒绝；释放后新snub-20261110-232708导入/prepare-only通过、未启动。共享background_input仅OpenThread失败加只读thread快照/异常note，原失败拒绝和完整freeze/cleanup未改，已按前后精确散列审阅保存后绑定。完整逻辑/头显false，41.25%不变。

## 96e1a1cc当前原时钟部件绑定候选：新中立回执后才单Grip，已编译未实跑（2026-11-10，最新）

- 在bee6上只改weapon_snub_runtime.inl：原bind_proposal仍在同一位置调用一次，观察其真实成功结果；新增只读中立状态/缓存控制器局部距离数学与原source/callback时钟记录（最多4096行、200ms最短间隔）。不加Native/engine查询，去掉诊断后控制体逐字等同父版；22导出/283公共源不变，完整PG96e1a1cc编译通过。新的记录不授予Grip/机构/库存权限。
- 私有脚本在最终手部靠近、真实UP完成后，以原sync_ns必须晚于该时刻且<=100ms的新记录、同Game PID/creation/weapon/world-generation、当前绑定/中立/原65mm提议距离为准备条件，最多等5秒，满足后只发一次Grip。不满足不发Grip，无重抓/源续期；原实际45mm物理捕获、真实C0/原始输入/lease/抗重放/库存/机构检查仍由生产消费者完整执行，不能用记录或化妆Snap代替。
- 22项解析边界检查通过。另执行实际生成的pre-Grip决策块4项spy检查：fresh仅一次Grip，历史source/异枪/未绑定无Grip并报准备失败；这是离线决策，不是真实传输或Native grant。
- 新snub-20261110-225556已prepare/真实冻结模块导入通过、未启动，Native9650/PG96e1/XR1684；同时包含上一轮已离线验证的无候选过期首失败停止，仅作用于真实抓取阶段。源码保存后一次受影响原匣流程验证，first failure与正常恢复分开记录。完整逻辑/套筒/体验/GrabPose/头显仍false，41.25%不变。

## bee6实跑Game56220：取枪/单发通过，首次原匣源过期发生在候选建立前；已补脚本首失败缺口（2026-11-10，最新）

- snub-20261110-223718，Game56220/creation134387953581691509，Native9650/PGbee6/XR1684。取枪/实际单发有效，loaded12→11、reserve36/bonus0。新增hold-phase实际8条均成功，完整observe耗时0.6808～2.1922ms；本轮未复现hold_frame_after，不能称其已修复，也不能把8个正常样本推广到故障分支。
- 首次目标相关过期在frame419/seq3765/token1：原source copy入口age71.8723ms、copy2.1778ms；随后state12.767ms、follow6.928ms、draw5.6699ms，接触source读时age100.4507ms撤销。此前gate4/candidate_flags1/lease0，尚未建立part候选。后续420 difference16、421 difference18和最终等to3超时均晚于该首次源失效；原匣未获租约/未开始事务。实际hold正常样本与这些接触阶段证据保持分开。
- 首失败脚本此前只在无租约且pending/requested时停止，因此漏过flags1而继续等到超时。未来新脚本加acquisition_wait_active，只在真实新Grip抓取阶段覆盖无候选lease0源过期，非抓取/故意old-Grip负例不改。保存的Game51648 frame647及Game56220 frame419、100ms/非目标等10项actual live_reload函数离线回放通过；本轮使用过的H及哈希原样保留，不倒写为已修复实跑。
- 另确认私有脚本wait(snub_reload_preparation)默认after0，可匹配启动期旧state2；不是当前原时钟准备证据。下一步准备“当前源、双手中立、当前gun/rig绑定”的只读回执后再单次Grip，不依赖化妆Snap许可、不重抓、不续期。当前缺口不等于已证实唯一原因，尚未实施或游戏覆盖。
- stop0/launcher0，Native/入口/XR/输入/hand/game全正常恢复，无强杀/子进程；626次CLOAK/1487前台采样无违规或错误，206未绑定身份保留。84帧/43.08秒GPU录制无错误，16格全跨度及末帧亲审、编码0，真实场景/双手/枪可见，2fps不支持快速接触/自然全角度接受。完整逻辑/修正套筒/体验/GrabPose/头显仍false，41.25%不变；所有本方运行/编码/压缩已终态。报告reload-56220-review.json。

## Game51744启动前磁盘失败与本方存储恢复；新身份保留同一bee6观察构成（2026-11-10，最新）

- snub-20261110-222648/Game51744/XR41608仅到xr_preflight/awaiting_level，launcher报Errno28 No space left on device。native_started=false，无operator/手枪动作/录像；stop1/launcher1与全部原始false字段保留，不把未激活Native/输入的字段解释为活跃模块清理失败。终态后OS确认Game/XR皆不在，无强杀/私有子进程；不称成功生命周期。见startup-51744-review.json。
- C盘实测free47,988,736bytes。仅本方D/build-*、已终态可重建438个obj/ipdb/iobj/ilk/pch按完整绝对边界逐一检查后删除436,113,271bytes，保留全部source/最终DLL/EXE/LIB/PDB/文本数值。再仅本方D透明NTFS压缩，进程低优先级，终0；3,820,611,903bytes逻辑数据实际2,460,706,544bytes。当前bee6完整source_snapshot及最终PG散列逐项仍匹配。未清其他会话数据。
- 共享入口新增4GiB磁盘准入已核对：prepare、start资源事务、worker启动game前三处检查，仅拒绝新开、不删除/打断已有。当前C free恢复超过6GB，新身份snub-20261110-223718按同一bee6/Native9650/XR1684重新prepare与导入通过，未启动。新实例冻结共享isolated_sim/runtime_receipts精确散列与源码到本方review，不改旧候选散列绕过检查。
- 完整逻辑/体验/GrabPose/头显仍未通过、41.25%不变。下一步仍取实际hold认证阶段时间，不重启已使用的51744实例；旧磁盘失败不当武器结果。

## 完整来源校验离线基准：扩大批次未获收益，改补实际hold认证阶段原时钟（2026-11-10，最新）

- 当前e667完整生产observer的59项人工内存回归继续通过。实验仅在独立benchmark头文件扩大同页读取批次，生产未采用：同一2418 witness字节/158 merged ranges、两轮closing reread保持，逐一修改158处均拒绝；读取198→122，padding1000→4032bytes，但单次release基准平均1445→1508ns，没有加速证据。4475次原metadata读取/23414bytes、完整observer50324ns只代表小型fixture，不能解释实际game数万读取或85ms延迟。见read-cost-benchmark.json，不把次数减少当性能修复。
- 新PGbee6d7f9仅在既有hold函数增加最多8成功/16失败的原时钟阶段记录，含原sync、原callback配对、绑定/完整观察前后与退出；去除纯诊断后控制体与e667逐字相同，22导出及283公共文件不变，完整编译0。它不改来源/100ms/两轮字节校验/身份/租约/库存/机构，也尚无游戏验收。
- 新实例snub-20261110-222648导入通过。首次预检发现共享只读cover observation由7931d1c8更新到b121d9c4；已逐段核对原sample体完全相同，新增仅遇AnchorRegistryChanged时丢弃失败观察、保持session/game/owner/XR四身份、完整重新读取最多一次，第二次失败抛出，不授输入或复用源。精确前后散列与源码存本方review后才绑定，未跳过校验。
- 继续完整逻辑阶段，41.25%不变。下一次只观察已出现hold_frame_after的实际原时钟分段，首失败正常停；没有新手枪玩法通过或新交付/头显接受。

## e667实跑Game41012：持握重发布最终时效失败，接触优化尚未覆盖（2026-11-10，最新）

- 两次因实际双实例资源占用在Game启动前拒绝，保留日志；确认旧Game53724/51084退出后第二次被新Gnasher/Troika占用拒绝，随后确认56484退出才实际启动同一尚未使用的prepared实例。无共享队列/抢占/强杀。8项实际私有live_reload函数回放通过，包含保存的647无租约pending源过期立即停止、100ms边界及非目标拒绝保持。
- snub-20261110-220651，Game41012/creation134387934961007553，Native9650/PGe667/XR1684。取枪路径到达持握后，frame184/seq2895发生hold_failed，inventory_stage=hold_frame_after；脚本在首发前main-skin检查停止，尚未到达本轮Snub首发/弹匣接触。原始最终时效拒绝保留；没有记录该最终检查的精确age，不能臆定某个观察器或其他实例为唯一原因。
- 已有耗时显示波动：frame174 pre-original85.0111ms/original16.8744ms，frame177 equipment41.6851ms。e667只省去不适用Lancer starter准备，未证明解决整体时效；下一步优先离线测量同一完整witness集合的校验开销，保留两轮全部字节重读、故障拒绝及100ms。
- 正常stop0/launcher0，Native/入口/XR/输入/hand/game全恢复，无强杀/子进程。628自有CLOAK/1610前台采样无违规/错误，320未绑定身份保留。34帧/32.19秒GPU录制、无录制错误，16格全跨度及末帧亲审、编码0；frame00031为实际黑帧且存在长捕获间隙，均保留，不称渲染稳定/自然拟合/头显通过。
- 已按全局newest3/48h删去本方五轮旧已审阅像素324文件/62768412bytes，源/制品/文本/数值保留，旧图不再可下载。当前无本方runtime/encoder，goal active、完整41.25%不变。报告reload-41012-review.json。

## e667a46f接触耗时候选：Snub跳过不适用的Lancer starter准备，已编译未实跑（2026-11-10，最新）

- 当前已认证known/main-held且类型Weap_COGPistol的exact-frame scope下，backend不再调用只可绑定Weap_AssaultRifle的prepare_starter。原函数先做完整current后才类型拒绝；此前Snub分支除有限诊断计数外没有绑定/机构动作。仅省这项不适用工作，Snub本身的完整frame/source/身份/手部/租约/库存/实际机构检查仍在原消费位置。
- 同一backend已有阶段耗时日志增加最初左Grip行，附原source-age/copy/pre-equipment时长，供下一次真实首失败定位，不新增来源读取或权限。283公共文件及22导出不变，完整PG编译通过；不宣称已修复103.478ms过期。
- 私有动作脚本的首失败停止补足“尚无租约但已候选pending/requested弹匣”的stale_before，仍是同一100ms实际拒绝，不重发Grip。新实例snub-20261110-220651导入与prepare-only通过、未启动。保存后只做一次受影响原匣切片；完整目标/41.25%/头显未验保持。

## PG4611实跑Game51648：真实单发通过，首次原匣接触因源过期未获租约（2026-11-10，最新）

- snub-20261110-213255，Game51648/creation134387924431375982，Native9650/PG4611/XR1684。真实UP回执frame287/seq2839，原sync到脚本检查36.0226ms；随后唯一Down在frame288/seq2843 status3执行，原时钟age48.0654ms，轨迹/写入同源有效，弹量12→11、备用36、bonus0。没有execute_not_ready，因此只能称本轮首发通过，不能追认adeb特定延期分支已游戏覆盖。
- 原匣接触最早拒绝：frame647/seq3504/token1，gate8/pending、candidate_flags17、lease0，source copy入口81.0351ms，复制1.9985ms，后续首次stale_before读为103.478ms。未获部件租约、未开始原匣事务。后续649/650 difference18是request_magazine与magazine_pending改变，不替代首次源过期，也不允许忽略新授予路径的变化。稍后手枪局部距离变化与全程超时单独保留；不假定GPU或某个观察器已证实为唯一原因。
- stop0/launcher0，Native/入口/XR/输入/hand/game全正常恢复，未强杀、无子进程残留。661次自有CLOAK检查/1604前台采样无违规与错误，246次身份尚未绑定保留。76帧/46.84秒GPU投影、无录制错误，16格全跨度及末帧亲审、编码0；真实场景/双手/枪可见，不当自然全角度/快速接触/头显接受。
- 当前无本方运行/编码。完整交付沿用41.25%，本轮当前构成取枪/单发通过，完整换弹/修正套筒/体验/GrabPose/头显仍false。下一步只定位源认证到首次接触消费之间的实际工作，保留100ms与原租约/连续性，不原样重跑长流程。见reload-51648-review.json。

## PG461119b7：首发前等待真实新鲜UP回执，已编译待实跑（2026-11-10，最新）

- adeb实际两次尝试分开记录：Game48520在Native激活前消失并报Winerror87，无手枪动作；Game59180正常取枪，首次Down无轨迹、弹量12不变。frame901 blocked/muzzle0/preparation_budget0，902仍按住时baseline；本轮没有execute_not_ready回执，因此不能认定adeb延期分支已真实覆盖或已修复首发。
- 最新候选461119b7仅扩充既有pre-original neutral-UP只读诊断到2048条、最短200ms间隔，附原sync_ns/observed_ns；底层Input、ledger、全部100ms/250ms/松开/身份/反重放规则不改。私有动作脚本在既有弹量读取之后只发送一次真实UP，最多5秒等待原时钟新鲜的UP回执，再发一次Down，无自动重扣。13项离线解析/拒绝检查通过；22导出、283公共文件与父版相同。未有新游戏验收。
- 全新snub-20261110-213255准备完成、未启动，Native9650/PG4611/XR1684，当前CLOAK/静音/私有Source1入口。Game59180全部正常收尾，32帧/28.13秒GPU录制16格全跨度和末帧已审阅、编码0；录制长间隙保留，不推断性能根因。48520与59180分别保留独立报告。
- 完整交付继续41.25%：A=.5/B=.5/C=.25/D=.5/E=.25，权重20/30/20/15/15。无新增完整玩家动作验收；完整逻辑、修正套筒实际捕获、体验、GrabPose及物理头显仍未通过。下一步保存本候选后，一次新鲜UP→单Down受控验证，首失败停止。

## Snub延期接线候选adeb0683：原Input/ledger不改，2284检查通过，待实跑（2026-11-10，最新）

- 在4d99上只改一处调用端条件：当前frame、同bound-key武器和同model hold-generation下，`bound_is_lancer || instance(Weap_COGPistol)`可启用既有defer_new_press_for_muzzle。底层Input/原ledger、100ms source、35ms实际dispatch reserve、250ms首次生产源intent截止、原松开/连续性/反重放规则完全未改，不添加重试器，不为其他武器扩展许可。
- 实际production Input+原ledger的2284检查通过，包含旧false接线在枪口ready且execute not_ready时blocked并不补发，新true接线延后一个新鲜帧只调用一次；持续按住不重复，松开、截止期（持续新鲜样本也不能续期）、身份/证书/忙碌/动画/来源/外部R2改变均取消且恢复不重放。调用端类型/bound-key条件另作源码核对，不冒充游戏类型反射验证。
- 完整私有PGadeb0683编译通过，22导出和283份公共交互源与父版相同，Native9650/XR1684保持。全新snub-20261110-211206使用此PG、原precision规划、CLOAK/静音/2fps GPU/首失败停止/有界本方metadata读取，组合导入与prepare-only通过，尚未启动。源码/离线成功不等于27292首发已游戏修复，完整逻辑/套筒/体验/GrabPose/头显均false。

## PG4d99实跑：初始持握通过，首发执行保留量不足而blocked；定位Snub未接入现有延期入口（2026-11-10，历史）

- 205211/Game56396（creation134387888094269882）在控制器动作前读取正在更新的本方session-report遭PermissionError，无玩法动作/录像，正常全清理。新增仅本方metadata JSON的2s有界重读，捕获sharing/文件替换/不完整JSON，9项离线故障/超时/身份拒绝检查通过；不重读输入帧、不续期源时间，仍由原精确Game/XR身份和wire检查决定后续动作。下一全新实例205745导入/prepare通过且实际进入取枪，无该读取错误；未记录实际重读次数，不声称故障分支已游戏覆盖。
- 205745/Game27292/PG4d99实际取枪成功，首发无轨迹、loaded12保持。frame257原execute_not_ready回执：dispatch reserve35ms、valid/same_frame/frame_current/held/registration/live/epoch全1，但reserved_frame_current/reserved_live0，sync_age73.2728ms；原函数未调用。R2 status8是blocked，不是baseline；continuous_poll_gap1在本帧已由原连续性证明允许，不作为拒绝根因。
- 源码对应：底层Input/原ledger已有“枪口ready但execute返回not_ready”的真实单Down延期及250ms原intent截止测试；调用端game.defer_new_press_for_muzzle只允许bound_is_lancer，Snub未接入。因此本轮返回blocked并消费边沿，后续同一按下不能自行重放。此前口头所说“只因枪口未ready才启用延期”不准确，已更正为具体调用端类型门槛；下一窄修复只把当前同bound-key/同hold-generation的Snub接入现有延期，其他来源/身份/按住/连续性/100ms/250ms/松开检查保持。尚未实现此修复，不重复原样运行。
- 4d99的59项实际observer回归及9339→4864读取量结论保留；游戏仅证明初始Snub持握路径通过，没有到达此前重发布hold_frame_after场景，不能称该故障已修复；套筒/完整逻辑仍false。两轮stop0/launcher0、Native/入口/XR/输入/hand/game退出均true，无强杀/子进程；205745的504次CLOAK/1293前台采样无违规/错误、259次未绑定身份保留。
- 205745实际GPU35帧/17.50s编码正常、16格完整跨度及末帧审阅，真实场景/双手/枪可见，无首发玩法成功证据。音频explicit readback：initially_muted_sessions1、verified_muted_observations232、verify_failed0、target_shared_rejected0，muted0表示未新增静音写入，不能当未静音。两份分开报告reload-56396-205211-review.json、reload-27292-205745-review.json；所有本方runtime/encoder终止，goal active。

## Snub单次最终认证候选4d9903a2：重复读取减少，最终认证及失败规则保持，后续实跑未覆盖重发布故障（2026-11-10，历史）

- 在6de上只改两个inl：Snub backend内部身份暂存时跳过第一遍完整observe_game，随后原本的完整observe_game、main_hand_held、全部world/controller/pawn/weapon匹配、registration及原Frame最终时效检查保持原顺序。两者之间没有原版游戏调用/GT让出，contacts/candidates均false；暂存binding不代表hold成功，失败仍false并由原Runtime清理。非Snub/默认三参数调用仍保留旧双观察路径，其他模块未改。
- 实际production staging和完整observer的59项heap元数据测试通过，涵盖不可读/坏归属/mesh丢失/删除标记/逻辑放枪及暂存后变化的拒绝。相同有效fixture元数据copy由9339降为4864，完整最终观察仍执行；仅证明读取量减少，不是游戏CPU耗时或真实Native注册证明。实际backend最终guard源码顺序复核通过，原Runtime127/1650时钟/backend失败回归另外保留，未弱化失效判定。
- PG4d9903a2编译通过、22导出与父版相同、283份vr-common完全一致；Native9650/XR1684保持。新实例snub-20261110-205211明确绑定本方4d99候选，输入precision修正、2fps私有GPU采集、首个过期许可停止、原机构/库存/100ms规则及CLOAK静音保留，导入/prepare-only通过，尚未启动。此处不是58868/58312故障已修复声明，完整逻辑/套筒/体验/GrabPose/头显均待验证。

## Game58312：帧复制计时已取得，失败转为既有持握重发布最终时效；正常清理首次全通过（2026-11-10，历史）

- 私有PG6de97379在a76a上只改两个inl，记录GT提交回调POD时间及复制前后QPC，不改任何许可/输入/机构；22导出一致，283份vr-common源完全相同。首次编译因Snub stage在该include点仅前置声明失败，改为现有纯model held-slot0/左Grip诊断过滤后编译通过；失败日志保留，未加新游戏读取。Native9650/XR1684保持。
- 第一实例203358/Game56396在Native尚未激活时，旧动作脚本45s启动等待先到，正常取消，84帧/44.63s全为启动卡；不是Native/武器功能失败或成功。随后只修本方交接：必须实际native_started与手/开火连接ready才开始动作，外层150s有界启动等待，原动作45s及100ms输入检查保持。实际录制因而从连接后开始，不再称全启动视频覆盖。
- 新实例203934/Game58312完成取枪/首发、原匣返回无补弹/奖励及返回射击；战术接触前frame660 held-chain重发布最终 `hold_frame_after` 失效并hold_failed，按首失败停止，未到套筒。8份常规已配对样本复制耗时0.6933–1.2737ms，selected age33.76–48.26ms；原Native提交回调时age10.26–21.17ms，回调到复制18.79–33.39ms。它们仅普通样本，未覆盖失败帧660的所有子操作，不能认定复制已排除为全部负载原因。源码stale_before确在完整observe之前检查；后续重点为持握重发布内两次完整元数据观察与最终检查，不放宽时效、不把backend hold false改成成功。
- 退出工具的共享修复已逐字核对：launcher调用request_stop(signal_native=False)，让原owner先正常drain武器再停Native；默认stop信号/身份/终态/recenter不变。音频ready升级15s有界等待及严格唯一game/owner/readback，源码前后差异在本方common-helper-review保存。上述不是武器玩法修复。58312本轮stop0/launcher0，Native/入口/XR/输入/hand drain/game消失全部true、无强杀/子进程，是此独立路径首次全正常清理；旧false原样保留。
- 783次自有CLOAK/1824前台采样无违规/错误，230无绑定身份样本保留。87帧/44.10s GPU稀疏画面正常编码，两轮完整16格概览及58312末帧亲自审阅；56396为启动卡，58312场景/手枪/匣动作可见，2fps不计精细拟合。全部本方实例/编码已终。报告reload-56396-startup-review.json和reload-58312-review.json，完整逻辑/套筒新落点/体验/GrabPose/头显均未通过，goal active。

## Game58228：2fps采集控制仍发生100ms许可失效，脚本已在首个实际撤销停止（2026-11-10，历史）

- 新实例snub-20261110-201535保持Native9650/PGa76a/XR1684，只把私有GPU请求从0.1s改0.5s并在原持握许可撤销reason5、原lease2/3、age>100ms时停止后续脚本动作。产品输入/时效/租约/释放/反重放/机构完全不改。本轮实际取枪/首发后，原匣阶段frame460/sequence5000/token1/lease3 age100148300ns撤销，脚本立即保存首个真实撤销、停止，无自动重抓或继续等完成。降频不是已证修复；接下来定位copy-frame入口原始年龄与同步读取耗时，不增加100ms。
- 更正前一轮时间顺序：200540原匣实际抽出在frame425，早于frame429失效及frame430惰性等待，而非“等待后才抽出”。之后回插已失去原许可，最终松手才进入waiting_ammo。旧完整原图/数值保留，本轮据此改为首个过期撤销即停止；不把后续超时当首因。
- 698次本方CLOAK、1700前台采样无违规/错误，243次未绑定身份保留。稀疏GPU录制148帧/77.04秒、无录制错误，编码终0，16格全跨度及末帧已审阅，约前37秒启动卡，其后实际场景/双手/枪可见。2fps不提供快速接触或全角度验收。stop0/launcher0、Native/入口/XR/输入/game退出true，无强杀/子进程；hand drain仍false独立保留。
- 原运行/编码全部终止后，按全局独立已完成newest3/48h媒体政策删除本方四轮已审阅旧像素2237文件/345795196bytes，源、制品、全部文本/数值及逐目录media-pruned保留。报告reload-58228-review.json、独立retention回执和capture-control脚本保存，全文历史中的旧图不能再当当前可下载媒体。完整逻辑/自然拟合/头显仍false，goal active。

## Game58868/200540：新等待分支首次真实覆盖，原匣回插未完成；完整逻辑仍false（2026-11-10，历史）

- 本轮Game PID58868复用，但creation134387860167841019与此前不同，专属实例snub-20261110-200540；Native9650/PGa76a/XR1684。实际取枪/首发后，frame430/sequence4279/stage3首次 `snub_reload_held_source_wait no_action1 permission_retained0`，未立即因main发布丢失而清事务；随后实际抽出、stage3→4，但原匣回插未完成，等待snub_reload_complete超时。不能把分支被覆盖等同于完整恢复或原匣通过。稳定读取本轮成功，无planning-failure数值；不能追认58196同因已修复。套筒仍未到。
- 相邻frame429源age100.2047ms，frame434撤回age100.9133ms，后续release_required/lease0保留。当前重点是原匣回插期间的实际物理行程/来源失效，而非放宽抓握/租约或自动重抓。同步GPU请求耗时中位32.26ms、p95 54.33ms、最长791.33ms；近似QPC对齐记录在capture-pressure-200540.json，时间重叠不证明GPU阻塞或失效因果，不据此报修复。
- 700次自有CLOAK、1686前台采样无违规/错误，243次无绑定身份保留。GPU录制688帧/76.10秒、无错误，编码完成，全跨度20格及末帧已审阅，约前30秒启动卡、后段真实场景/持枪/匣运动可见；自然皮肤/全角度/头显不接受。stop0/launcher0、Native/入口/XR/输入/game消失true、无强杀/子进程；hand drain仍false，仍非全清理通过。证据独立命名reload-58868-200540-review.json，未覆盖早期58868。
- 共享game_audio_guard.py经逐段核对只增加显式环境下哈希绑定的静音证据helper和回读汇总，本方绑定ed48d882并保留前后源码/差异；统筹随后确认原muted=0可表示本来已静音，不冒充未静音或武器通过。独立入口自动启用增强证据，未改其他应用或系统音量。原产品保持a76a，无本方活跃运行/编码，goal active。

## Game58196：新PG原匣/返射通过，战术读取稳定区间失败；新等待分支当时未覆盖（2026-11-10，历史）

- Native9650/PGa76a/XR1684 在新实例实际取枪/首发、原匣抽出/返回不补弹/奖励及返回射击通过。进入战术流程前的 `read_snub_parts tactical-gun` 三次观察未满足原稳定区间，严格拒绝；旧脚本断言前未落盘逐次drift，具体差值缺失，不放宽容差。新增只读failure落盘脚本，供下一新实例保留已有3次position/basis数值，失败仍退出且不产生规划许可。
- 本轮 `snub_reload_held_source_wait` 标记0，新PG特定等待分支没有游戏覆盖，不能称58868失效因果修复已证明；套筒修正仍未到。离线127/1650通过结论保留，完整逻辑false。
- 764次自有DWM CLOAK及1913前台采样无违规/错误，312次尚无绑定身份保留。GPU合成录制619帧/74.91秒，无录制错误，正常离线编码；20格全跨度及末帧已审阅，约30秒启动卡、后段真实场景/持枪可见，末段手枪在画面右下且主手不清晰，不当成自然贴合或全角度证明。stop0/launcher0，Native/入口/XR/输入/game消失均true；手部drain仍false，独立清理失败保留，无强杀/子进程残留。所有本方运行/编码已终。报告 `analysis/snub-20261109/reload-58196-review.json`。

## Game58868 后的窄修复：活动持握发布失效时只等待、不授予动作；已编译，后续实跑未覆盖（2026-11-10，历史）

- 私有PG a76a90b5只新增 `weapon_snub_held_source_wait.h` 及Snub tick的早返回。仅既有活跃事务、同一视觉binding、game已知且main=false、无busy/动画、当前逻辑model仍同world/pawn/weapon且hold_generation等于事务opening、活动held_binding未知时，保留惰性事务并返回，不执行动作/写姿势/库存/机构。停止/切换/事务完整身份检查仍在此前；真实model放枪、代数或身份变化、已知冲突binding均不能等待。恢复后仍走原fresh Frame/main/source/lease/骨行程检查，未保留旧权限或输入。增加最多8条 `snub_reload_held_source_wait` 供实跑核对。
- 全PG编译通过，仅2份产品文件变化；Native9650/XR1684/模板/接触范围/时效保持。原Runtime/model及新增观察失败→等待→新鲜认证→真实收枪回归127cases/1650checks通过。首个测试错误地预期重新发布后无输入时返回held，原Runtime在完成发布后返回idle；已按真实语义检查idle及实际publication/model/代数，初始失败日志保留，没有修改产品来迎合预期。
- 全新独立候选 snub-20261110-195654 绑定新PGa76a/Native9650/XR1684、cloak1110，实际导入组合/规划profile检查通过，尚未启动。原输入precision规划及配对只读诊断保留，不复用旧实例。完整逻辑及该等待分支的游戏覆盖仍待验证；此前FireDisable2/hand drain失败仍独立保留。

## CLOAK Game58868：原匣返回/无奖励与返回射击通过，战术新匣阶段主手状态撤下后事务清理（2026-11-10，历史）

- 带只读配对诊断的新实例实际取枪/首发、原匣完整抽出/返回无补弹奖励、返回后射击通过。配对读取本轮成功、无failure回执，不证明旧13236同因已修复。战术流程stage3→4→5、新匣已取得，尚未入座；frame934 `held_chain_observation` unavailable，frame935 `snub_reload_eligibility_lost` 的binding/source/known均1但main0，导致line394清理。相邻Native撤回source age100.1645ms保留；held observation内部首个失败子检查未记录，不能认定该年龄必为唯一因果。套筒修正未到，完整逻辑false。
- 初步源码对应：`Runtime::lost`保留逻辑model、撤销活动held_binding；`weapon_hand_client`的main_hand_held要求逻辑model与活动held_binding同时成立。Snub eligibility在source_wait之前把main0当实际失去持握清理事务。需要进一步区分失去活动发布与真实放枪/身份或附件变更，不得以保留逻辑model直接授予动作或延长旧租约；尚未写入新产品修复。
- 799次本方DWM CLOAK/1947次前台采样无违规或错误，274次尚未绑定身份保留。私有GPU录制774帧/89.44秒、无错误、退出后正常编码；20格全跨度概览及末帧已审阅，开头约30秒启动卡，后段场景/持枪及原匣动作可见，最后左侧有弹匣离开视野，不能称完整自然贴合。非真实头显验收。
- stop0/launcher0，Native/入口/XR/输入与game消失true，子进程0、无强杀；手部drained仍false，继续保留独立清理失败。运行、编码与审片均已终，无本方资源占用。证据 `analysis/snub-20261109/reload-58868-review.json`。下一步是针对实际主手状态发布/读取的离线修复与回归，不重复未变化的游戏流程。

## CLOAK Game13236：实际取枪/首发通过，原匣前配对读取失败；新诊断实例等待资源（2026-11-10，历史）

- 新独立路径实际 Game13236/XR56884 完成 Source1 取枪和真实首发；原匣 inspection 规划读取 `read_world_grips` 在五秒内没有得到原 GVE6/input exact ticket，流程首次失败，未到套筒修正处。原读取未记录细分，不能认定是 ring/来源/cohort/100ms 任一特定原因。新实例 snub-20261110-194336 只增加有界只读拒绝计数及最后一组序号/年龄，并在原失败分支等待1ms避免忙轮询；不放宽任何原检查，不发新动作、不续期、不伪造许可。
- 本轮539次自有窗口DWM CLOAK检查、1556次前台采样，无记录到违规或采样错误；419次尚无绑定身份样本保留，不能称全部启动窗口时刻已覆盖。原私有GPU合成帧452张/57.86秒，录制无错、退出后编码完成，完整20格概览及末帧亲自审阅，前约28秒仍是启动卡，后段实际场景/双手/枪可见。无桌面捕获、无取消隐藏；不是全角度/皮肤/头显验收。
- 正常入口stop0、launcher0，Native/入口/XR/输入恢复及game消失均true，无强杀/同名子进程残留。但是FireDisable最终仍2，HandStop5，drained=false；明确保留手部清理失败，不能因game退出就称全清理通过。报告 `analysis/snub-20261109/reload-13236-review.json` 保存。
- 新诊断实例尚无launcher-session；两次资源拒绝均发生在创建本方game/XR前，保留逐次start与result日志。上一资源持有者已经终止，但随后其他会话取得两个名额，继续按精确worker身份观察和资源检查，不抢占、不标blocked。完整逻辑→吸附/Volume→GrabPose顺序不变。

## 主人恢复运行：新 CLOAK 独立实例已准备，首次启动因双实例资源保护拒绝（2026-11-10，历史）

- 主人经统筹明确恢复自动运行，已读独立入口最后一节。此前等待后台修复的禁令已归档；不再以旧暂停作为门槛。新实例 snub-20261110-193543 使用本枪 Native9650/PG01e6/XR1684 和最新 cloak1110/Simulator92e505，未复用旧 background1110b 实例或改旧 proof。导入接线及 --prepare-only 通过，套筒 precision 输入修正保留；产品机制、身份/时效/库存/原骨行程未改。
- 本方监视改读精确自有窗口的 DWM CLOAKED；隐藏窗口保留 WS_VISIBLE 或 z-order 不再被误判为显示。前台、置顶及无法确认 CLOAK 仍停止本方动作并正常清理。录制使用原私有 GPU 合成帧，不取消隐藏、不用桌面捕获；编码延后到正常退出，不占游戏运行资源。这些是本方接线，尚非本轮运行通过证明。
- 首次 start 在创建本方 game/XR 之前因两个精确身份仍活跃的自动 worker 拒绝（boom-1110-193550-55800 / grenade-smoke-1110-193549）；日志完整保留，无本方 launcher-session、不抢占、不停止其他实例。资源释放后继续本方原完整逻辑；不是功能失败、不是共享模拟器签收队列，不因此标 blocked。完整手枪、吸附/Volume、GrabPose、头显均未验收。

## Game17844：空仓换匣与未上膛拒射已到达，套筒实际接触首失败；独立实例接线已准备、当时遵主人暂停（2026-11-10，历史）

- 最后一轮旧共享路径Game17844使用Native9650/PG01e6，实际腰侧取枪/首发、原匣完整抽出与返回无补弹/奖励、战术新匣库存守恒及两次返回射击、empty_preparation_fire_0至10每一发真实弹道通过；空匣真实退出、新匣插入后loaded12/reserve22，未上膛真实扳机尝试保持ammo且无新弹道，empty_inserted_uncharged_blocks_fire=true。随后套筒未完成，完整逻辑false，不能称手枪已完成。
- 实际首失败为frame1846/sequence7258/token6：Native new_grant1/lease3，Snub charging gate8 line139，stroke仍idle0、wanted0/full0/issued_full0。原真实物理precision手点(0.0834416478,0.0745548559,0.186577649)m到原closed_contact约109.83mm，超过原45mm capture；即stroke.begin正确拒绝，未发生真实套筒拉动。旧脚本把腕原点放在套筒骨位置，忽略固定Marcus115腕→precision地标；后续release_required并非可放宽的根因，cleanup365是本轮请求停止后原cleanup分支，不当成机构完成或首因。未出现snub_existing_hold_disarmed，01e6的特定保留分支仍没有本轮实跑覆盖，不能借本轮前进而宣称该分支因果修复已证。
- 只修私有模拟输入规划，不改产品接触范围/Native仲裁/100ms/租约/原行程/库存/奖励：由原固定physical marker及Native grip→wrist映射计算真实precision落点，并在原65mm raw候选和45mm physical capture的交集中选有余量的真实控制器位置；magazine原腕规划不改。C++原mount、physical_contact与SnubSlideStroke前向oracle检查243种planning条件：189可行例同时满足raw≤55mm与precision≤35mm，54种无足够余量例在发输入前拒绝；最大CPP位置差4.52e-10UE、precision误差33.01mm。旧17844腕规划精确复现109.8328mm偏差并被原stroke拒绝；新规划即便期望拉到端点，没有实际原骨full ack也仍不能完成。均是离线规划/算术证据，不是Native实际许可或套筒游戏验收。既有持握失效回归补验真实新RGrip仍能收枪，125cases/1620checks通过，产品01e6未改/未重复编译。
- 原共享责任全部闭合：17844 runner/launcher0，Native/入口/XR/输入/hand drain全true，十原件逐项exact、publicv71/manual只读，已隔离本轮证据和停录像并恢复释放。159.7秒双路编码正常，六页完整概览及四张末段原图亲自审阅，启动capture缺口保留；全角度/自然皮肤/GPU/实际头显仍未验收。此前八轮旧媒体按主人newest3/48h政策删除21690文件/4091089147bytes，仅自有已停且已审阅录像；源码/制品/资产/全部日志与数值、逐轮media-pruned说明保留，Git ebe6858保存。
- 主人新要求改为每会话独立实例，不再等共用模拟器或建立票据/受理链。已读独立入口文档，复制并核对本枪Native9650/PG01e6/XR1684，专属snub-20261110-184014及snub-actions-20261110-184014输入/配置/存档/输出/PG与XR复制品已经prepare，无launcher-session，未启动；旧183136初版prepare保留、不启动。专属active helpers无solo-m1-1028d/build-solo/stop-basic实际引用；game与XR都由本方session的PID+creation绑定，不再靠全机唯一进程；动作子进程CREATE_NO_WINDOW，独立原stop入口/按身份正常退出，维护Source1与原Clock/库存/机构规则。实际冻结operator、equipment模块、规划和profile哈希导入检查通过，无Game/XR/输入命令写入。
- 共享sixdof_unified_runtime.py出现一份后续清理修复，已逐字核对只是在已请求HandStop0/5后、原deadline及2s reserve内再查同一FireDisable2，最终drained仍要求实际0/0or5；不改开火/来源许可。新值8825a790绑定到独立候选，原5b6291及前后源码保存，其他common helpers仍原hash。
- 主人再次反馈前台干扰，最新automatic-runtime-pause.json明确禁用全部新Game/XR/模拟器自动启动及重试，必须等主人明确恢复，不能凭另一次短测自解禁；此规则覆盖独立文档旧“已修复可恢复”段。本方无在跑实例、不再stage/restore共享路径，未启动新的独立实例，不删除/绕过暂停标记，不向统筹发启动/占用/释放/主动进度。Native/PG源码构建与独立脚本准备保留，goal active，不标blocked、不宣称完整/头显完成。独立wrapper --prepare-only通过并返回paused=true/started=false；未来运行只用当前哈希绑定入口，监视本方前台/置顶/相对用户前台的z-order，一旦观察到违规或新暂停即停本方动作并原stop，不抢回焦点/移动用户窗口。此监视尚未实跑，旧后台短测试已撤回全面结论。先完整逻辑，再吸附/Volume，最后GrabPose的完整范围保留。

## Game36920：真实UP前置分支已覆盖，既有持握重发布后的回滚错误已定位（2026-11-10，最新）

- Native9650/PG5aa6实际取枪/首发、原匣完整抽出/返回无补弹奖励及返回射击通过；8条pre-original实际UP回执均dispatch_attempted0。随后frame564 held_chain库存观察unavailable清活动binding，frame565现有持握重新发布inventory_stage=ok但返回unavailable，按实际Runtime唯一对应已成功b.hold后current失败，旧clear_model抹掉既有持枪。精确current子检查当时未记录，不能认定必为100ms或某身份；frame669 token2因此no_candidate不是接触距离失败证明。战术新匣/空仓/套筒未到。
- 新修复严格只在Snub的was_held&&model.held、原held_identity=当前对象且attachment有效、无pending的新鲜既有重新发布路径传入opt-in；b.hold成功而紧接current失败时仍suspend清活动绑定/输入/未来事件，status仍unavailable，但保留不变逻辑model/generation，与其他lost路径一致。下一独立新鲜帧必须重跑actor/input/inventory/identity/attachment/hold/原时限。未缓存Frame/权限，不重试输入或原动作。新取枪、b.hold失败（含14696 hold_frame_after）、身份/附件变化、坏时钟仍严格回滚；后端未成功绝不保留。增加只读existing_hold_disarmed回执供实际分支核对。
- 完整PG01e6编译通过，55BOOL/22导出一致。125cases/1618checks原Runtime加故障回归通过：已成功重新发布后真实时钟推进过100ms时活动绑定撤下、逻辑代数不变；新鲜帧再认证成功；身份/附件变化仍清逻辑；新取枪及后端失败仍硬回滚。原earlyUP2155检查保留。
- Game36920正常runner/launcher0、Native/入口/XR/输入/手部清理全true、十原件exact/public未写；隔离、恢复、释放后才离线编码。64.4秒正常编码，三页完整概览及四张原图亲自审阅，原图确有后段枪消失现象但不单凭像素推断子检查。完整手枪/体验/GrabPose/自然全角度/头显均未验收。

## Game34720：首发按下被当作恢复基线，保留失败；前置真实松开观察已编译（2026-11-10，最新）

- Native9650/PGc2aa本轮取枪通过，首发无实际VR弹道、ammo12→12。原R2回执frame269 inputage100.1982ms失效，frame282/sequence1631有真实pressed1/released0、当前muzzle ready，但sole Input返回baseline1、owned0，frame283才看到release1，未调用原射击。完整逻辑false；未到14696的持握重新发布失败处，不能宣称该修复实际覆盖。正常runner/launcher0、Native/入口/XR/输入/hand drain全true，十原件exact/public未写，证据和停录像隔离后已恢复释放。26.6秒配对编码正常，一页完整概览和四张原图已审阅；启动捕获缺口保留。
- 新私有PG在既有pre_original_held_shot观察点，只对真正持握Snub、R2/L2无所属press、当前右扳机有效且已松开、Game原R2已up时，用原copy/fullread_game/exact Input continuity/100ms检查推进同一个Input的UP基线。press_ready=false，绝不调用原键；若意外要求dispatch则拒绝并fault。真实Down、已有press/释放责任和原post-equipment路径保持；无源时间、输入事件或时限续期。
- 同步增加的实际Input回归复现了重复UP观察再次yield封基线、随后Down被吞的问题。窄修复仅在全部现帧/身份/cohort/时效检查后，既有bound且未sealed、同sequence、已确认release时对重复中性观察no-action返回；未确认release、源/身份变化或超时不能走此路。该问题由新回归证明，不追认34720必为重复UP同因。2155项现有加新增原ledger检查通过（两种source、漏tap不回放、重复UP、过期UP不能解封、新Down只dispatch一次）。完整PG及55BOOL/22导出检查通过，尚未游戏覆盖新分支。

## Game14696：实际打空已通过，持握重新发布的最终帧检查失败（2026-11-10，最新）

- Native9650/PG92ec本轮实际腰侧取枪、首发、原匣抽出/返回且无补弹奖励、战术换匣库存守恒和两次返回射击通过；empty_preparation_fire_0至10每一发均有真实弹道，实际打空通过。之后空仓流程尚未取得旧匣，main hold重新发布frame1538/source6575失败，inventory_stage=hold_frame_after。此前绑定/完整observe/main_hand/registration检查通过；实际最终原帧时限检查失败。旧11036未记录细分原因，不能追认同因。空仓新匣/未上膛拒射/套筒拉放/返回射击仍未验证，完整逻辑false。
- 本次唯一新修复范围是Snub设备持握认证中的两次完整观察：保留全部新鲜身份/元数据/归属/状态复读和原100ms、registration/final Frame检查，只省去该认证不消费的支撑几何和接触候选计算；普通完整观察和实际手部/部件发布不变，其他武器使用原路径。PG完整编译通过、55 BOOL/22导出一致；50项原观察/新认证检查通过，含归属更改、不可读字段、非GT、逻辑持握消失和第二次观察前变更拒绝，未用缓存权限。测试夹具补齐已有类型查询的无授权stub及当前非Snub两路候选预期；最初构建/旧夹具失败记录保留。新产品尚未游戏验证。
- 14696的runner/launcher0、Native/入口/XR/输入/手部drain全true，十原件exact/public只读，已隔离本轮可变证据和已停录像、恢复并释放。135.833秒配对录像正常，五页完整概览和四张末段原图亲自审阅；启动与91.8秒附近捕获缺口、114秒模拟器waiting card均保留，不自动判游戏卡顿。完整手枪/体验/自然全角度/实际头显未验收，顺序仍完整逻辑→体验→GrabPose。

## Game3248新首失败：真实抽出已到达，Native输入互斥不可用触发生命周期变化（2026-11-10，最新）

- Game3248（Nativec762/PG9ff）实际腰侧取枪、首发、原匣抽出/返回且无补弹/额外奖励、返回后射击通过；新增真实feed端点回执分别在frame479、644出现。第二个战术旧匣抽出后已到waiting_ammo stage4，随后lifecycle5→6，原事务严格拒绝，未到新匣/空仓/套筒。不能把前一轮55152的空仓首失败或11036持握原因称为已修复；这里只证实新调度实际完成了两个抽出端点。
- Native既有bounded handoff证据：frame657请求输入3090，matching Code3 mutex_unavailable、latest/selected=0，原立体声收到age2.9981ms；controller decode缺输入，下一frame658 lifecycle5→6，而对象/hold/world/session/space/input/registration保持。PG随后snub_identity_boundary(stage4,same_objects1)和cleanup line379，真实失败保留。旧Code3同时包含WAIT_TIMEOUT/abandoned/error，不能倒推出必为短竞争；不忽略lifecycle也不续期。
- 新Native9650只在原cover_input_live::matching_input处复用已存在bounded_transport_read：最多3次零等待读取，同一stereo key、原stamp、100ms、publisher和packet/cohort检查；仅lock_now实际contended=true（WAIT_TIMEOUT）允许重读，失效身份/坏包/abandoned绝不重读。增加retried/recovered/exhausted计数供下轮实际分支核对。1份产品inl和1份standalone test全局include变化，原header/wire、C0/姿势、原入力均保持。
- 完整Native编译通过，92 BOOL、70 exports一致；273项current tracking（含真实mutex busy及释放后的exact cohort）和25/16/16项outside/pending-magazine/tracking组合检查全0fail。PG9ff保持。新包snub-prepared-20261110-161214-input-contention-curl-contact只读prepare通过，源码保存ceeb87b7，下一轮入口已在同一RuntimeGuard上有界等待，未取得前无baseline/stage/game。
- 3248的runner/launcher0、Native/入口/XR/输入/hand drain全true，十原件exact、public只读；已隔离本轮数值和录像、恢复并释放，直接向实际相关等待者发送一条释放通知，离线编码不占游戏。57.3秒配对录像编码正常，2页完整概览和4张失败附近原图亲自审阅；game启动1.199→2.849秒capture gap保留，双路尾部覆盖≤500ms。
- 用户最新指定游戏路径持续生效：J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64；不改授权/DRM。停止向统筹chat主动通知/回复，只用统一互斥和真实占用，必要时最少直接协调。完整逻辑→体验→GrabPose顺序与完整最终范围保持，目标active，完整手枪/自然全角度/头显均未验收，分数不自动变化。

## Game55152完整逻辑首轮与pre-PlayerTick窄修复（2026-11-10，最新）

- 主人要求的“完整逻辑→体验→GrabPose”顺序保持。Game55152实际腰侧取Snub、首发12→11、原匣取出/返回且不补弹不加奖励、返回后射击、战术新匣10/36→12/34及返回后射击、11发真实射击至0/34已到达。不是完整通过：第三次空仓换匣在old_magazine stage3、serial3仍extracted=0时release，触发cleanup line297，尚未换入新匣/未上膛拒射/拉套筒/最后返回射击。
- 首个关键失效早于cleanup：frame1469实际C0物理观察比当时now老102.0186ms，gate11拒绝；frame1474 held_chain_observation未通过，输入原时钟age102.4981ms，strict source消费清除了发布；随后左手part撤销，extracted保持0。没有hold_failed，不能说11036旧持握原因已修复。精确最耗时子读取尚未分类；不放宽100ms、不重抓、不伪造弹药或补记通过。
- 正常收尾：runner/launcher0，Game无残留，Native/入口/XR/输入/hand drain均true，十原件exact，public只读。共享数值日志和已落盘双路录像先隔离至本方logic-run-20261110-153829，再恢复并核对释放；已向实际等待者简短通知，离线编码审片不占游戏。
- 双路录制正常退出0，编码132.133333秒。1605张game/2821张simulator，双路尾部覆盖≤500ms；game启动0.933→3.033秒捕获gap保留，不能当游戏卡顿或丢失动作证明。完整5页配对概览与失败附近6张原图亲自审阅；自然手型/全角度/GPU/头显未验收。
- 源码窄修复：仅已有old/new magazine和charging动作复用现有Lancer pre-PlayerTick机会，在原角色更新撤销/老化已完成双手结果前运行原Snub tick。首次抓取继续原post路径，每次动作仍重新读取fresh Frame、原100ms输入/物理观察、exact lease/identity/cohort/库存/端点；没有新输入、writer或hook。增加真实feed approaching抽出回执，让脚本等实际抽出完成才release，保持同一Grip/接触/深度，不重试。
- 完整私有PG9ff70974编译通过，55 BOOL与22 exports一致，1801源文件只改3份（含一行声明）。Nativec762/原wire/mount/bore/follower/姿势表和所有新鲜度逻辑保持。缺少前置声明的首个编译失败日志保留，修正后通过。源码检查点7fa3949b；新包snub-prepared-20261110-155250-pre-original-curl-contact只读prepare通过，尚未game验证。
- 新入口复用统一tools/debug-launch/runtime_stage_guard.py的RuntimeGuard('solo-m1-1028d')，包住own baseline→stage→launcher→恢复；保留原Owner内层锁，离线编码外置。首次启动因实际共享锁busy在baseline前拒绝，stage/game均未执行，等待真实持有者释放；没有前后继票据/整包互审。
- 完整手枪/体验/最终每部件GrabPose及物理头显仍未完成。旧41.25%分数不据此自动调整，保留历史负例和原v71/固定手动入口。

## 主人最新顺序：完整逻辑优先，独立启动包已准备，不再以前后继票据为门槛（2026-11-10，最新）

- 主人最新直接要求覆盖旧流程：各枪独立源码/编译/验证，只在共享Game/模拟器实际启动和stage/restore时作简短协调并用现有互斥/真实占用检查。停止远期前后继、整包互审、候选受理和一次性票据重绑依赖。旧051707/链条/未绑定前驱消费者作为历史保留，不再是下一轮运行门槛，也不借旧许可运行。
- 第一阶段先实际跑通取枪→射击→原匣真实退出/返回且无奖励→战术新匣库存守恒/再射击→实际打空→新匣→真实套筒拉动/释放→再射击。复用已有基础姿势，保留真实持握、身份、100ms输入时效、反重放、原机构端点和库存/存档保护；首个实际拒绝就停，无自动重抓或补弹。第二阶段才做Snap/Volume/容错/反馈，第三阶段最后精修每部件GrabPose，完整最终范围不删除。
- 私有7份驱动已独立：主手皮肤发布仅作非阻塞观察；不再等精细手型；移除原匣流程前的Snap35/50mm和旧Grip体验矩阵调用，直接用真实释放、当前基本接触、fresh Grip动作，Native授权逻辑不改；原体验函数仍保留以供第二阶段。关闭专门手型斜角头部巡检，不改基础部件测量/坐标转换。主握hold_failed/事务失效/库存守恒/空膛未上膛拒射/原套筒端点的实际检查保留。
- 独立包`snub-prepared-20261110-152025-logic-first-curl-contact`使用完整编译c762/624e、原XR16840/Simulator6aea，`run_logic.py --prepare-only`通过。没有前驱票据参数或远期签收依赖。用本次自己的原10项快照及文件身份绑定部署/恢复，复用原受控启动/双路录像/正常停止；600秒本次切片上限只会在真实direct launcher子进程身份核对后请求原normal stop，不杀其他进程。
- 原launcher启动互斥及真实Game/XR/录制/输入进程仍阻止并发；独立文件编码不再被当成占游戏的条件（只在本方进程覆写旧idle函数，无共享源码修改）。正常结束后先复制本次共享可变日志/数值、把本次已落盘录像目录验证C盘workspace边界后隔离到本方run目录，再恢复10项和核对释放；离线编码/审片不继续占共享游戏。
- 最近真实检查Game54588及solo-m1启动互斥占用，manifest指向Longshot本轮。已只向该实际使用者发一条“结束恢复释放后通知”的简短启动协调，没有请求候选批准或新票据。本方尚未stage或启动Game。已有手型成果和反例、旧源码/二进制保留；不再继续调姿拖延逻辑。目标active，完整逻辑/最终GrabPose/头显仍未完成，41.25%不变。

## 套筒末节时序窄修已配对编译，129个C0采样过交线检查；游戏未跑（2026-11-10，最新）

- 回到真实保存C0坏面原权重：进入3/4帧及释放21/22帧的主要交线均为`Thumb03_L`末节；并非只凭整手看着不自然就加腕偏移。改变拇指时序为根部先转位`2w−w²`、末节后收`w²`后，原拇指交线清除。剩余三个过渡帧均为`Finger09_L`无名指末节；按有限原面交线约束只加3°、两端0的DIP弧线，未修改最终静态表、手腕或原物理输入生成器。前一腕路径38对/14.646mm与其他失败完整保留。
- 新唯一C0草稿只对实际Snub+LH authored magazine语义+精确profile3在已有120ms权重内选择这条曲线，释放尾段仍仅为皮肤；原20rad/s PoseBlend、Frame/输入/身份/lease/动作/时钟/骨平移缩放不改，无新writer。不能把名义20rad/s当严格实测上界，旧20.010312控制测量记录保留。
- 实际C++与Python旋转最大差2.973e-8；60/90/120Hz各43共129帧，1891完整手面/3146整臂面对全3045原枪面>1mm交线0、纯指间0，115局部T/S固定。主握所有实际影响骨（含躯干3/4/5及右臂）在129个样本中原世界骨p/q逐项差0，故复用其同一完整皮肤几何。90Hz第9/22帧仍有约.332/.633mm次阈值跨度，不能隐去或当成深度。仅0/20/22/31四个实际C0样本的顶点/面心强inside0，不是开放面完整体积证明。
- 三阶段实际C0整臂/接触共6图已亲看；图中腕角为X轴角，不冒充四MCP腕部解剖测量。原指定掌marker约8.77mm间隙及自然全角度/GPU/游戏/头显接受仍未解决或未验证。夹具仅使用Game11036 Source1相对头位置移植，实验室朝向、原固定臂可达投影、合成上方3cm保留；不是实际输入完整回放或物理追踪。
- 完整配对私有Native `c762c866` / PG `624ea194`已编译。Native相对4114只变C0头+新增曲线头，PG相对bd42只变套筒原骨腕位/closed表；主握/原匣、持握阶段日志、物理掌挂点/枪口/follower不变。Native92BOOL/70导出、PG55BOOL/22导出和7协议头一致，8051 C0/作用域检查0。源码快照1628/1801不是实际MSVC依赖数。
- 无新运行包、真实claim/ticket、stage、Game/XR/录制/编码；冻结051707原字节仍保留且未正式受理。下一份入口必须重新标识并消费实际前驱原语义、接入已验证本方首次claim/10基线/原前驱持久绑定，不能借旧意向或空闲状态运行。按用户直接要求目标保持active、守原队列等待，不因临时条件停目标。无新玩家动作，41.25%与Publicv71/固定手动入口不变，全手枪/头显未完成。

## 同物理输入的有限腕路径仍拒绝；目标按用户要求保持active（2026-11-10，最新）

- 新制作研究保持相同保存的物理控制器输入、主手和枪位，不通过换容易输入绕过首败。在前一硬指间约束曲线基础上只增加3个有限显示腕路径参数，正弦权重两端严格0、腕偏移上限2UE；仅作LBS腕权重线性估计，不是固定骨长IK，未修改现有Native/PG或实际输入/接触半径/lease/时间戳。
- 43个制作样本中纯指间仍0、MCP80°制作边界保持，但枪面最高38对，最长交线跨度约14.646mm（不是深度），进入/释放均仍失败。不能用计数从42降38宣称改善完成；前一版本和本次完整负面均保留。未进入实际C++/唯一C0回放或完整DLL。后续应基于拇指/套筒有限交线设计不同的展开/收拢与腕朝向，而非重跑不变小偏移族。
- 收到Bol转达的用户直接要求：任何情况不要仅因等待将目标标为blocked，继续等；保持完整目标active。此要求不新增运行许可、不改队列顺序或身份/新鲜度/收尾保护，也不要求主动汇报。最新直接状态为Beam8已恢复、提交原唯一具体包，Bol正独核且未发FINAL。本方仍无唯一最终交窗，不把恢复/空闲等同交窗。
- 本方CPU研究已终态。配对4114/bd42、冻结051707及真实claim/ticket均保持原状，无stage/Game/XR/录制/编码。前驱原始语义消费者仍未绑定；手枪整体验证/实际再次射击/套筒进入释放/头显未完成，41.25%不变。

## 新入口首次claim/原文件恢复已接模板，实际前驱消费者仍待绑定（2026-11-10，最新）

- 新模板接入同一首次claim的原inode/原body捕获、原10项快照、stage前当前原件复核、stage后持久绑定复核、restore前原claim/基线/前驱原字节与副本复核。公共v71若意外变化，全部恢复写入均拒绝；Windows大小写/路径别名不能冒充10个独立目标。正确选择实际plan中的公共路径原字符串，避免斜杠风格不同导致角色匹配失败。
- 隔离整合曾实际失败：Windows `os.open`未指定O_BINARY，`os.write`把LF写成CRLF，严格first_raw拒绝。原失败夹具/hex记录保留；新模板显式二进制、校验写入字节数并从原exclusive fd捕获文件身份。未规范化/放宽claim字节比较，未触真实claim。
- 真实新模板的引导片段在隔离子进程通过5组检查：prepare不建claim、首次原inode/body精确、第二次在验证前拒绝、-O消费后不可重试、真实旧plan十项物理路径/公共角色匹配。原文件capture/restore与实际OwnAttempt模块的11组隔离检查通过，包括损坏claim/原基线/保存字节/前驱原件/副本、runtime忙、公共变化、重复capture、越界和大小写别名；坏绑定均在写原文件之前拒绝。它们不是完整冻结包prepare、前驱语义或游戏证据。
- `bound-admission-template.py`仍有待冻结参数；实际前驱原始数据schema消费者明确不存在，plan未绑定时先消费本次尝试再拒绝，绝不以accepted/hash布尔值冒充许可。051707入口/plan/ready与真实claim/ticket保持原状态；没有新运行包、stage、Game/XR或媒体作业。编译配对仍为4114/bd42，源码已编译不等于运行已就绪。
- 最新直接只读尾链状态：Bol revision232仍保留窗口；后继Beam8服务额度终止且未完成交接包，尚无交窗。LS050140仍没有实际前驱绑定。本方不跳过先前意向、不把空闲/会话状态当窗口许可。完整手枪、进入/释放、实际再次射击与头显均未完成，41.25%不变。

## 主握首败原日志阶段细分已完整编译，套筒受约束曲线仍拒绝（2026-11-10，最新）

- 回到实际Game11036首败：原记录只知道`b.hold`拒绝，并不知道线程/注册/源Frame/持握绑定/observe/主手标志/最终注册与100ms等哪个检查先失败。新私有PG只在原`nativeequipment::hold`内把原短路组合拆成同序检查，为现有`inventory_stage`写具体阶段；原调用、clock次数、返回、失败后清持握、原100ms和原来源身份不变。没有新轮询、输入/pose/lease/Hook或自动重抓。
- 两份真实函数体抽取后，用明确的stand-in服务作79单/双失败对照，158检查通过：调用序列、返回和持握状态一致，首个失败阶段准确；这不是实际元数据/入场许可证明。之后完整私有PG构建通过，1801源文件中仅`weapon_equipment_backend.inl`变化，55BOOL、22导出、7公共协议头与4114 Native一致。
- 新配对Native4114/PGbd42bb7e（完整SHA见`snub-hold-stage-pair-build.json`）；未重编Native。保持原主握/原匣数据、未加入套筒新表或曲线。原251d/4114与冻结051707入口/plan/ready逐项SHA仍匹配；没有替换队列内意向，没有生成新运行包或真实claim/ticket，也没有stage/游戏。下一份真正可执行候选仍必须另定身份、消费实际前驱原始语义、接入本方首次claim持久绑定并在stage/restore复核。
- 套筒新曲线已增加硬指间边界及MCP80°边界，从零参数起步，43个Python制作采样纯指间0，但枪面最高仍42对，因此继续拒绝；前一版本纯指间70对的负面完整保留。没有拿新增约束的0纯指间冒充完整运动通过，也没有为此打完整套筒DLL。后续需要新的腕/拇指实际绕行方向，禁止重跑不变曲线。
- 运行窗口只读状态：Frag报告下一顺位已独立接管；Bol最新226报告自身运行/编码结束、10原基线恢复、留存清理尚待结束。本方没有最终唯一交接或窗口，旧历史字段不是许可。全部本方CPU/编译工具终态，0新玩家动作，41.25%与Publicv71/手动入口不变，完整手枪/头显仍未完成。

## 套筒原行程五相位几何通过，进入/释放首败保留（2026-11-10，最新）

- 使用同一持续Workspace、真实唯一C0和原20rad/s关节限速，连续43帧检查进入/释放；并未把独立静态端点当作运动通过。最终抓握位置的张手初态本就有6组交线，明确保留。现有profile3路径还先混入普通闭拳，进入纯指间最高13对、枪面最高90对。
- 私有Native头草稿只把原匣profile2的direct判断增加精确profile3，其他布尔条件、V4、原120ms腕过渡和20rad/s均不改。相同真实C0状态对照：纯指间降0，仍有枪体交线；7837原C0/作用域检查0不等于运动修复。未改冻结Native/PG、未打新完整DLL。
- 增加明确的实验室“同朝向、物理腕在最终接触点上方3cm”进入/释放控制，初态0交线，但进入第3帧和释放首帧仍有交线，主要是拇指对原套筒骨1，靠近终点时另有无名指少量交线。它不是实际追踪录制，也不提供实际接触、lease或动作许可。两组失败与原对照均保存，不以延迟到稳定帧隐藏首败。
- 另将主手原raw和枪位严格固定，副手腕与原bone1顶点同时施加同一原骨D：canonical和AR_Reload原0/47/48/49共五个相位，包括最大Y−5.435749UE；左右各1891完整混合手面/3146整臂面，对全部3045原枪面交线和纯指间均0，115局部T/S不变，0面排除。其他枪骨保持原位。它只证明这些抓稳后的独立C0几何相位，不是实际游戏行程/动画同步/时间连续性/封闭体积或头显验收。
- 6行×3斜角的实际C0接触图已亲看，保留进入与释放橙色坏面并与闭合/后拉对照。新的同步绕行曲线仅为Python制作研究：枪面最高22对、纯指间最高70对，MCP最高62.987°虽仍在80°制作边界内，曲线仍拒绝，不做产品C++回放或集成。下一次必须同时保持有限指间约束，不能只降低枪面计数。
- 原名义20rad/s PoseBlend实现保持原字节；这次实际归一化float输出测得最高20.010312rad/s，明确不是严格实测≤20的证书，也未为此更改产品限速或放宽阈值。主握V5既有精确预算证据单独保留，不能拿来替代这组副手运动测量。
- 主握/原匣4114/251d和冻结051707入口/plan/ready全部核SHA保持；真实claim/ticket仍不存在。本方没有stage、Game/XR/录制或编码。GN最新revision297报告Frag已独立接收窗口，GN无运行权；此只更新本方只读队列记录，没有给予Snub窗口，后续仍经唯一尾链。无新玩家动作，41.25%不变，手枪/物理头显未完成。

## 套筒同参考相位静态草稿通过C0，完整运动未验收（2026-11-10，最新）

- 发现制作数据相位混用并纠正：`slides[0]`来自AR_Reload，骨1实际Y−2.946172/X+.011137UE，并非canonical闭合参考；旧指腹目标与全3045枪面障碍错开。现以原骨1的511顶点/445面直接取canonical表面，逆原D重建误差3.997e-15，445原三角形一一对应。受影响旧目标表标记失效；其全枪负面交叉结果仍保留有效，不冒称产品/运行因果修复。
- 原合成夹具右手位于头后约6cm，不宜用它把某腕角推广为所有实际持枪。已取Game11036 Source1 startup/rest相对头部位置20cm右/40cm前/30cm下重基准，仍保留实验室朝向与原可达壳投影6.35584cm；不是完整输入回放/时间样本/物理追踪。原套筒姿态此时实际腕角仍74.948°；overhand参考姿态由旧66°变15.425°，说明布置确实影响腕角。四MCP扇面与真实掌下表面定义新腕框架，不改骨长。
- 放弃薄匣手型和混相位全手拟合；较宽参考种子经同相位掌指/全3045枪面/纯指间约束后，最后三处拇指交线经局部间隙修正清除。新静态草稿实际唯一C0：左右每侧1891手面/3146整臂面与全部原枪面>1mm交线均0，指间0、双手互交0，115局部T/S固定，局部q误差2.818e-8，副手真实四MCP腕角15.611°。C0强inside采样0；不能当开放三角面完整体积证书。
- 实际五指指定pad到同相位原套筒距离依次1.287/2.223/.944/2.354/1.635mm。指定掌marker8.771mm仍明确保留，不能只拿两pad贴近冒称完整掌接触；整个掌组nearest3平均1.927mm，二者是不同采样。最终整臂/接触三视图已亲看，完整自然/全角度/GPU/game/头显仍FALSE。
- 新`slide-canonical-contact-draft/weapon_snub_slide_grasp_pose.h` e79563a3仅更新原骨坐标腕位和15局部Q；原reference表、profile3、request/wrist/calibration/V4函数体不变。闭合及原后拉骨相位、各独立object/model/units缩放与旋转，651实际C++格式/校准检查0；没有完整PG/Native重编、stage或游戏。主手/原匣4114/251d和051707意向包保持原字节。
- 本方首次claim持久绑定模块另已实现并通过14个隔离文件用例：同一首次inode/body、原基线10项/公共只读角色、原前驱字节与副本、stage当前原件复核、restore前原件复核、任何失败不重绑。未接进冻结入口、未触真实claim/ticket，不验证/替代尚未明确的实际前驱语义，不称正式运行已就绪。
- 当前`snub-slide-canonical-current.json`。下一点只核新profile3实际现有C0的进入/释放与原120ms/20rad/s、同相位全枪及套筒行程；具体首败再窄修，禁止把静态0当运动通过。队尾意向与完整前驱消费绑定继续按既有链，不重复提醒。无新玩家动作，41.25%/Publicv71/手动入口不变，全手枪/头显未完成。

## 新短验包冻结与队尾意向，套筒原姿态仍拒绝（2026-11-10，最新）

- 新内部源/二进制包 `snub-prepared-20261110-051707-curl-contact`，Native4114cc83／PG251d86b4／XR16840b27不变。prepared deb3bb8b、plan6b64d2f4、readyff7c9f7e、入口3c6fb33b、依赖17f93a6e；3428 Native/PG源快照、376项目driver当前/归档、5私有driver、9272依赖与7实际module origin绑定。只读preflight0，审计禁止任何子进程/文件写入时仍通过；隔离副本3例确认优化prepare不消费、失败real先消费、第二次独占拒绝。真实claim/ticket均无，未stage/game/XR/录像，数字不是玩家验收。
- 现051707仅文件/启动字符串级准备，不是正式可执行准入。结合GN公开消费者反例，实际前驱identity/首次claim/baseline/terminal/normal-closeout原件必须在执行消费者中语义绑定，且本方首次claim还需inode/body/原基线/交接持久绑定与stage/restore复读。实际前驱尚未受理确定，不能用外层bool、任意哈希集或其他枪claim代替；不运行现入口、不借221733授权。
- GN21120持有者直接确认保存 Snub051707 待定意向，排在Frag020237→Bol022614→Beam8→Bow034115→Troika045716/core2→LS050140之后。未独核产品/消费者、未受理/授窗，普通队列不当blocked或verified live wait。原件与回执保存在 `snub-curl-contact-tail-intent.json`、`snub-curl-contact-tail-ack.json`，无多次提醒/统筹上报。
- 独立套筒原key46按当前closed b_HG_Slide参考进入真实唯一C0：完整1891手面/3146整臂，70纯指间、330枪面交线，真实四MCP腕角90.673°，拒绝。不是用axis角代替实际手方向。固定参考生成的简单overhand仍3/56/114纯交叉，全部拒绝。
- 改正腕方向与掌面注册后，40°制作倾斜分支实际C0腕角15.039°、纯指间0，但完整枪面83交线；降低倾斜或携带原匣手表仍失败。拇指单独修正76交线/4.751mm强inside，联腕4.72mm预测仍52交线/2.349mm，均拒绝。两指腹对445原套筒面有限目标注册后，实际两pad各1.854mm近似，却有429整手交线/11.666mm强inside；证明只对两pad不足，不能把局部贴近当全握持。失败过程/旧metadata索引覆盖纠正在 `snub-slide-natural-current.json` 保留。
- 已亲看套筒原姿态/中性注册各整臂+接触4张图，115局部T/S保持；未改任何产品/已冻结主手弹匣包、未为草稿打DLL。下一步套筒掌面/四指/拇指共同约束、保留全部3045枪面和445套筒目标面后再同一C0核验；主手/原匣短验按队列与最终消费者绑定推进。11036持枪首因、实际换匣/套筒/奖励/中断/反馈/头显全范围仍未完成，41.25%与新玩家动作0不变。

## 食指完整弯曲路径与双手局部修正已编译（2026-11-10，最新）

- 手枪在指定 `codex/6dof-single` checkout继续；Publicv71与固定手动入口未变。本轮无游戏/XR/窗口申请/助手，目标active，41.25%及新玩家动作0保持，完整手枪/自然动态/头显未验收。
- 显式 cosmetic V5用相同2984byte布局与新导出，V4仍严格version4/reserved0；先卷IP、后收MCP，共享原调用的now与20rad/s上限，真实Native扳机输入、Frame/timestamp/100ms/身份/权限/唯一C0 writer不变。PG主手单独选V5，弹匣/套筒仍V4。6030曲线/实际PoseBlend与177实际adapter host检查0；fake broker/Core不冒充live。
- 弹匣原index/middle短交线以小范围index MCP修正收口，实际C++40过渡与C0稳态/进入/后进入/释放主几何检查0；小于阈值最大0.269758mm交线仍保留，不当完整实体/自然动态证明。原free-fist detour窄修仍仅实际Snub左authored magazine精确profile，腕120ms/20rad/s不变。
- 首次V5成对编译仍有4index/4pinky原枪体短交线，拒绝；有限交点累积约束后的局部两指修正保留腕/其他9关节，press指腹移动1.116mm、小指0.320mm。196实际C++样本与独立math oracle0差，全部3045原枪面保留，食指>1mm枪面交线/强inside采样/纯指间均0；最大速率≤20rad/s。3实际C0稳态（rest/press/0.6）每侧完整1891手面/3146整臂面/双手互交>1mm均0，3实际C0前态限速回放也0；115局部T/S固定。真实四MCP方向腕角LH16.063°/RH25.232°，图中旧axis角16.016°/20°不当实际腕舒适角。
- 完整Native `4114cc83` 与最终PG `251d86b4`编译链接0；Native92/PG55 BOOL保留，Native旧69导出加唯一V5成70，PG22不变，7共同wire header一致；最终仅PG数据变更重编译，不重复Native。源快照1627/1801不是实际MSVC依赖数量。首个PG9f986与旧dc320/8fee保留，未stage、未制成可运行候选。
- 已亲看本轮20张整臂/接触三视图，数值与离线可视证据单独记录；原guard穿越及失败实验和测试host编译失败均保留，不称11036持枪间歇失效已修复。当前 `analysis/snub-20261109/snub-curl-contact-current.json`，最终配对 `snub-curl-contact-pair-build.json`。
- 已保存源码检查点 `313474828b9cd104e544afd2dd0e306bc3b1976b`（独立Git index，公共HEAD/index不变）。新弹匣输入逆解81例通过；5个独立smoke driver已AST解析，首发前须见本轮主手cosmetic发布，主手hold_failed首次即停，无12发前奏。尚缺新外层运行入口/依赖清单/独占claim准入绑定，故未制成正式运行申请，也未借旧221733/Cover授权。
- 下一步先保存Git与准备唯一冷启动短程候选，再经已有路线获真实交窗；只主手/原弹匣自然保持、Snap进出、新Grip拉出归还、守恒及实际返射，主手首败停止，不射空12发前奏。后续空仓/套筒/奖励/中断/反馈/头显仍属完整目标。

# Snub 手枪开发与验证

## 2026-11-10 原匣掌指连接注册、真实过渡消费者窄修与配套数据

本轮有新数据和一处Native消费者窄修，仍无新增玩家动作通过。未stage／运行游戏／XR、未申窗、未调用CUA或助手；Publicv71与共享HEAD／index保持。完整已编译模块仍Native dc320612／PG8fee3bcd，里面仍是旧指节表，不能把本轮host编译当成新整对DLL。新草稿保存在`paired-contact-draft`及`magazine-motion-native-draft`，当前整枪、自然动态、头显仍未接受，评分41.25%不变。

原匣修复不再盲卷手指。最深源顶点10515是58.0%食指MCP＋42.0%手腕，原匣49三角有限最近点显示掌指连接处进入约5.462mm；按真实最近点方向外移并加0.5mm间隙，得到单个固定腕注册偏移 `[0.0145325053,0.2232972653,-0.5526668396]` raw，约5.96mm。腕方向保留原key22；中／无名指采用从原参考重建的小范围接触修正，食／拇指在两侧原匣面196／168制作。最初把四指统一贴近时有13组纯指间交线，拒绝；加硬指间约束后食指过卷且指腹更远的版本也拒绝。最终仅保留有效中／无名指分支，再修食／拇指；小指保持原较松姿态，不为了五个中心点都贴面强塞入下缘。

实际同一115骨C0重建：新原匣完整1891手面及3146前臂／roll／袖口／手面均0组超过1mm的匣体交线，强内部顶点／面中心0，纯指间0，腕角约16.016°。右mock raw对照差0，左HOST合成目标随新注册改变；不声称左物理跟踪已验证。纯手子树预测与实际差约1.033e-4raw（约0.00103mm），低于显式float世界坐标8ULP预算；首次1e-4raw断言略超的host失败保留，只修数值对照预算，没有改产品阈值。混合袖口预测与真实C0最大差0.571raw，说明不能把移动手腕的简化预测当完整手臂；真实袖口仍单独核查为0匣体交线。四组整臂／近距图和最终无剔面的8个八分体视图均亲看。原动画key18／22直接复用分别178／75组匣体交线和7／57组纯指间，key18腕角56.37°，均拒绝，原版关键帧不等于自然VR手型。

新增实际过渡首败：腕已对齐且静止的CPU条件下，原120ms shape混合先走向普通free闭拳，再交给原20rad/s逐指节限速，接近第2帧产生约13.601mm擦入。只对当前Snub＋左magazine＋authored＋精确profile ID，跳过这个普通闭拳中间目标，仍从真实上一局部指节状态经过原PoseBlend限速；原120ms腕部混合、raw输入、Frame／100ms／权限和唯一写者均不改。该C0草稿实际W4/WX/UNDEBUG编译，原几何＋9范围检查7836/0。

更强的真实C0前后消费者对照使用相同新请求／目标、相同0.27777777685腕混合权重，并各自提供其正确上一帧CPU手指状态：父本24顶点／36面中心、13.601322mm、59组匣体交线；窄修0／0／0mm／0组。输出局部指节对实际函数的误差约2.83e-8／2.96e-8，115局部translation／scale保持。它证明此拥有缓冲区中间目标绕行的修复，不证明实时Core、GPU、运动控制器或游戏。接近／松开各20帧的对齐腕检查匣体全清，但接近仍有2组食／中指约1.053／1.797mm表面交线，不能当穿透深度。它们在原拓扑相隔3边，近距图已亲看，仍保留待修，不能仅靠邻近拓扑判通过。额外把index PIP限速缩至75%的实验延长了小交叉，拒绝，未写入Native草稿；Native仍原20rad/s。中指单点下移2mm的草稿受约束无有效收益，也未替代有效数据。

主握三根握持指和拇指改用同一有限Root面／实际交线重心约束、原参考生成和纯指间约束；原固定腕／挂点／食指不动。再与上述原匣和现有真实食指rest／press在窄修C0中组合：静息主手4组、扣压8组、食指缓扣最差14组，原匣三态均0；各姿态纯指间0，主手静息／扣压强内部样本0。主握掌指面明显较原33组改善，但仍不称自然动态通过：既有右食指路径最大1.313mm仍在，三组整臂／近距图已亲看；不把8个端点交线或0强内部当完整封闭体积证书。

主握表和原匣腕位移／15表已制成配套补丁，request函数和原参考表不改；主握触发只改变食指3节，其余12节严格同表。主握51格式检查0，原匣完全同SHA的116检查复用；首次主握数据保留原参考float范数微差导致旧严格unit检查退出12，现仅在生成表时显式单位化，旋转几何不变，不放宽检查，旧表及失败日志保留。Native窄修、两份表尚未进完整生产DLL，已知动态问题解决前不为各姿态反复打包。

工具失败保留：过渡host同时引用两份路径下的公共头导致重复定义，改为已核逐字相同的同一include树；消费者host复用了旧`previous`变量名及原可选roll参数槽，修正HOST命名与第6参数，生产代码未改变以迎合这些失败。离线约束导数的单坐标拒绝／中指修正未生效记录也保留，不作不可行证明。当前`snub-paired-contact-current.json`，独立Git保存`paired-contact-source-checkpoint.json`。下一点只修剩余右食指护圈路径和左指间接近小交叉，再同一兼容构成编译／短验Snap进退、fresh Grip原匣拔回、守恒与返射；11036连续握持首因和后续空枪／套筒／奖励／中断仍保留完整任务范围。

## 2026-11-10 真实指节限速过渡首败与原匣有限接触见证

本轮没有新的玩家动作通过，仍在主握／原匣自然接触制作；无stage、运行窗申请或游戏／XR执行。生产Native dc320612／PG8fee3bcd未改、未重编，PG仍旧指节表，不能交付。Publicv71与共享HEAD／index保持。评分沿用41.25%，不能因新拟合和检查涨分。

新反例明确了静态插值检查的缺口：仅食指两端有界联合制作，把11点均匀NLERP最大强内部迹象从约1.193mm降到0.170mm，但指腹离真实扳机仍远。实际冻结V4＋`hand_finger_pose::PoseBlend`独立编译／执行0，在已经稳定握持的起点，快速扣下的逐指节限速重新制造2.594mm擦入。制作侧逐分量复现与实际C++135样本最大误差1.11e-16；这只验证限速模型，不能当真实控制器／Core授权／GPU证明。

把同一限速加入食指原参考10参数（其他12关节、腕框架不改，静息末两节仅小范围屈曲）后，制作抽样峰值0.569mm；完整135点真实函数检查仍发现90Hz缓扣、trigger=2/3处1.313mm，故拒绝。端点指腹表面间隙3.394mm、原扳机目标误差6.271mm也未接受。四个代表姿态各自复用原已核hash的C0 host，原右mock raw保持、115骨局部translation／scale保持，预测与实际同一C0皮肤差0。四组整臂＋近距三视图已亲看：主手33／37／43／33对大于1mm表面交线、原匣43，纯指间0，完整自然手型仍不通过。历史文件名`rate-fit-digital-worst`实际包含缓扣最差帧，已增加准确别名`rate-fit-path-worst`，不能把它解释成快速输入最差。

有限原表面见证进一步排除了盲目统一加深握拳：原匣食指／中指分别4／7个强内部顶点、峰值约5.46／3.10mm，掌侧2点最高2.59mm；无名／小指没有强内部顶点，却有4.28／7.13mm指腹间隙，拇指5.18mm。原镜像bind指腹对应误差不超过0.062mm，完整15关节world frame回放差1.72e-5raw；使用1891混合手面，原匣24条边界边仍不是封闭实体。首次host误用输入17..77而不是10..70，得到的失真记录以`first-host-failed`保留；现独立unit与实际C0 palette断言，生产代码未放宽。

只做0／3／6mm横向注册见证：3mm可缩短扳机指腹间隙，但主手扣压交线37增至60；6mm新增掌侧穿插，均未采用。其间单个2.5mm注册假设的有界食指制作仍1.264mm擦入且指腹间隙4.480mm，直接拒绝，不再为已拒绝方案做固定骨长C0复跑或打包。这是离线改变腕相对枪位置的假设，未改产品挂点／腕目标／原始跟踪，不称保留原控制器的已实现新注册。

下一点是按已定位的护圈上缘／前壁和真实限速共同修食指进入，再按原匣实际有限表面分开处理食／中指避让与无名／小指贴近；保留完整掌／袖口和其余指约束，不再以静态端点或零纯指间计数代替自然动作。仍优先一份兼容构成的Snap进退、fresh Grip同一原匣拔回、守恒／返射最小闭环，不恢复12发空枪长前奏；旧11036持枪首因、全枪／头显仍未完成。当前`snub-index-rate-contact-current.json`，独立源码保存`index-rate-contact-source-checkpoint.json`；本轮新增的是执行证据与拒绝见证，不是可玩完成。

## 2026-11-10 手枪专属腕框架实现、双模块完整编译

已在独立配套源码实现Snub专属中性腕框架：Native从既有、已闭合核对的ClassPath识别`Weap_COGPistol`，无额外读取；原外层held资格／单一C0写者保持，只在内部ModelBinding增加`held_snub`校准数据类型。对应PG握把挂点使用同一中性校准和原Root2526／保留掌面制作标记。旧Lancer35°profile逐字不改，其他武器follower正文未改，没有新导出或手部协议布局。新类型仍严格核完整32字节模型签名、右侧、比例、units和原绑定／时效；把35°数据伪装成Snub或反向伪装均拒绝。

Native完整生产编译／链接0，SHA `dc320612cf83ac24bdce8b9e1e7b5d99e72b5d844faba744daaa3d28891aa0e6`；PG完整生产编译／链接0，SHA `8fee3bcdffc96e39c6c6e77d8f3100c0b346c84cc4ea3b41f7cc516ccae17117`。实际92／55 BOOL和69／22导出分别与父本一致，六个公共协议头相同；源快照1626／1800项不是MSVC依赖数量。Publicv71和固定手动入口未动。IMPORTANT：本对模块保留旧PG指节表，新手型数据仍在离线制作中，所以明确`runtime_candidate=false`、不可交付，未stage／运行／占队列。不能只拿新Native配旧PG，后续仅成对冷启动。

实际挂点数学1581/0；适配器原376＋新252检查0，涵盖同一raw Core字节／原时间、掌心marker、原bore、不同units/model/mesh比例，以及未知/错误类型、签名、左右侧和过期输入拒绝。Scene实际采样742/0，Snub/Lancer热读取数都3957；类型来自原捕获ClassPath。`current_actor`继续只检查身份，任意修改派生bool不是它的API契约；`same_key`和已闭合Scene类型负责类型关联，未为测试加读或放宽权限。原C0几何7827/0。

检查失败保留：首次host把`detail::target_valid`当公开namespace函数导致编译失败；首次Scene测试错误地要求身份函数检查任意派生bool；旧held测试仍预期Lancer腕旋转为I，在父Native5977上独立复现2失败后，仅host夹具更正为既有35°，生产Lancer未改。其他已过检查复用，只重编受修正影响的host。

使用同一新Native源码C0、原115骨fixture，保留旧35°参考生成的同一右侧模拟raw输入，再由真实Snub校准求新腕目标。三份静息／半程／扣压输出的右raw位置差0、四元数差约1.11e-16，115骨局部translation／scale保持；这仍是owned-buffer＋mock，不是实际跟踪、Broker授权或GPU证明。整臂／完整手面及三场景各两张三视图已亲看：主手33／67／57对交线，弹匣43，纯指间0，整体仍拒绝。后续手型制作已切到这份真实新C0 arm，不再用抵消旧校准的旧对照palettes。

静息食指整体避让现为0交线、0强内部顶点／面中心，指腹悬在接收机外约5.87mm；外观仍待整手确认。接着按真实V4两端NLERP制作侧向进入；单看0.4或0.7会漏掉0.6处碰撞，11点完整复核仍拒绝。剩余见证是原Root1562／1563、Y约18/X约-0.728的护圈前壁，不是翼片或弹匣。联合调整原参考端点把峰值约9.48mm降至1.63mm，再依前壁见证改变静息指平面最小到约1.193mm；仍有表面穿插／接触间隙，不能按数值更小接受。开口219边界的winding仍只是拒绝诊断。无新曲线协议、输入重放或动作权限。

下一点联合收敛静息／扣压端点和护圈侧入口，再闭合掌根／拇指与原匣自然手姿；更新匹配的PG数据时复用未改Native，之后才准备一份兼容构成做Snap接近／退出、fresh Grip原匣拔回／守恒／返射最小闭环。不打空枪前置、不为姿态逐个打包。源／CPU／完整编译／游戏／头显分别记账，目标active，完整手枪／自然手型／头显FALSE。

当前记录`snub-neutral-wrist-current.json`，源码／实际构建／保护核对分别为`snub-neutral-wrist-source.json`、`snub-neutral-wrist-pair-build.json`、`snub-neutral-wrist-preservation-check.json`；独立Git保存见`neutral-wrist-source-checkpoint.json`。本轮所有拥有的工具作业已终，无游戏／XR／录像／窗口申请或助手。

## 2026-11-10 原扳机定位与过渡首败保存

本轮生产Native `5977955a…735498`／PG `3b0ef9af…e6aa1`未改、未重编、未安装，无运行窗请求。上一轮源码保存a40a257保留。本轮新增的是原模型接触制作、实际V4旋转函数／同一C0的离线过渡证据；不能称新生产手型或实跑通过。

固定上一草稿的掌根／thumb CMC后，仅用从原参考生成的末两节变换，将拇指指腹间隙4.42mm降至约2.48mm，纯指间交叉0、主手交线12对保持，但完整贴合仍未接受。随后亲看原Root在X0／-0.6／-1.5切面，确认2571／2572是扳机前侧窄曲面，不把护圈外壳当扳机。原35°构成的食指根部在枪内Z约-1.01，实际扳机Z约3.16／3.53；仅扩大指节范围虽可接近表面，却产生新的护圈穿插，因此拒绝。

为联合验证腕框架和扳机可达性，使用原实际C0做了合成枪内腕方向0°的有界几何对照。IMPORTANT：host mock跟踪方向抵消仍未改变的Native35°校准，不能声明保留了真实控制器方向，不能当已实现Snub独立腕校准或原枪口跟随证明。此对照所有115骨局部translation／scale保持，食指根部Z约2.36，指腹在较小指节范围可到原扳机目标约0.1—0.3mm内；整臂腕角约20°。最初整手仍164对枪面／75对纯指间交叉，全部保留，不能以腕角或单指接近通过。

制作纠正：三根握持指不强塞到握把远侧X正面的任意三角中心。依据原Root正Y前弧，用预期指列高度向真实三角表面作精确最近点投影，再加0.8mm局部接触间隙；这样中／无名／小指末端间隙约0.80／0.50／0.25mm、纯指间交叉0，但仍有少量皮肤内部样本。拇指另在同一中性对照和原后侧面制作，强内部顶点降至7左右，仍不接受。三角中心高度造成强迫挤指的版本保留。首次整数初值让0.1°有限差分被截断、Jacobian非有限，现明确float并assert有限；该失败三份产物保留，不算求解通过。另保留一次int64 JSON写出失败。

过渡首败：实际冻结V4 `rotations` 函数W4/WX/UNDEBUG编译／执行0，11个0..1参数输出再逐一输入原C0，全部115骨局部translation／scale保持、仅三个食指关节变化。静息端点已经失败（6顶点、最大内部迹象4.475mm）；40%处扩大为13顶点＋19面中心、9.483mm，50%仍约9.14mm，扣压端点剩2顶点约0.154mm。这不是只靠调端点能宣称通过的自然动作。整臂3146面／完整1891混合手面及静息、半程、扣压各两张三视图均亲看；主手55／72／57对枪面交线，原弹匣43对未解决，均拒绝。插值是现有真实NLERP，不误写SLERP。

快速原蒙皮／指节FK与11份实际C0皮肤另逐一对比，避免把不同采样范围（顶点与面中心）的最大值差异误判成消费者错误；该数值一致性不代表GPU绘制。当前只收敛自然静息位置、护圈侧开口进入路径和掌根／拇指接触，再决定是否需要匹配的Snub生产腕／挂点适配。原输入、100ms、身份、唯一写者与旧11036原因未证明的结论保持，暂不扩套筒、空枪前置或逐姿态运行包。

当前证据`snub-neutral-frame-current.json`、`snub-neutral-trigger-transition-check.json`、`snub-neutral-skin-oracle-check.json`、`snub-neutral-front-pads-authoring.json`。独立源码保存`neutral-frame-source-checkpoint.json`。完整手枪／自然手型／头显仍FALSE，目标active；本轮有新证据改变下一制作动作，没有游戏／XR／录像或窗口占用。

## 2026-11-10 参考中性腕修正编译与完整接触约束

最新独立编译构成为 Native `5977955a…735498`＋原PG `3b0ef9af…e6aa1`。Native只把已批准的通用C0两处参考中性扭转修正移入自己的180bc冻结树：前臂helper29/53回到原参考局部旋转；求解器中性腕从当前肘框架乘参考腕局部旋转取得，目标不变。与原批准patch `c4f2f385…a89dd`逐字归一化对比一致，没有导入其他武器二进制或整棵当前源码。完整编译／链接0，实际92 BOOL、69导出和六个公共协议头保持；PG原文件复用未重建。

原C0几何检查7827/0。自己的原115骨fixture＋同一Snub目标反例中，只给原动画腕增加0.8rad，旧代码使左右前臂／helper漂移约49.28°／50.74°，新代码为0°；所有局部translation／scale保持。该结果是拥有的离线缓冲区证据，不能解释成11036原因修复、GPU或实跑通过。唯一C0写入者、原100ms／identity／anti-replay、输入及物理腕／枪口关系保留；未stage、未申请或预占运行窗。

指节authoring改成每次从原模型参考姿态生成，并硬限MCP相对参考不超过80°，不再叠加种子四元数。搜索域只是艺术边界，不是测得人体限位。原始枪2881顶点／3045面已按真实刚性骨权重分类；现有指腹目标均为Root，不是弹匣或套筒。曾按任意指腹中心强制贴近，即使距离降至毫米以下，仍引入36—102对纯指间交叉；完整保留并拒绝。直接替换成原动画腕框架也在反例中恶化，未改生产腕基准。

在正确35°构成上，拇指对侧接触、静息食指和小范围枪身注册的独立草稿已通过实际C0重建。完整1891混合手面／3146前臂、roll、袖口和手面及三方向整臂、六近距图已亲看。主手交线从参考草稿116对降至静息草稿32对，再到最新thumb-active-v2的12对；纯指间交叉0。最新草稿全部手顶点和1891面中心的强内部winding样本为0，但原枪仍有219边界边，不能当封闭实体通过。拇指指腹仍约4.42mm间隙，完整自交99对（同指19／混合掌蹼80）仍明确保留，零纯指间或更少枪体交线不等于自然手型。弹匣43对未解决；套筒未扩新迭代。

这些新指节／注册数据仍是未编译艺术草稿，没有混称PG3b0内的生产表。实际C0检查保持全部115骨局部translation／scale，右腕位置和枪口朝向对照原参考输出差值均0。局部表面避让曾把最深内部迹象5.33mm降到2.54mm，但扩大偏移后在其他面制造新内部点，已拒绝。全手活跃约束刷新所有原顶点／面中心，再加入实际交线重心约束的末轮把枪体交线降至2对，但指腹间隙反增至6.36mm、自交101对且一处局部方向证据含糊，因此也拒绝，不因计数下降换用它。源证据与完整构建分开记录，不为每个姿态重打运行包。

当前仍按主握持＋原匣最小闭环推进。先解决剩余拇指／掌蹼交叉、真实扳机端点过渡及原匣手姿，再用一份兼容构成在明确交窗后短验Snap接近／退出、fresh Grip拔回、库存守恒和实际返射；不打12发空枪作前置。目标保持active，完整手枪／实体头显FALSE。当前记录`snub-reference-twist-current.json`，独立Git保存见`reference-twist-source-checkpoint.json`；旧失败不覆盖。


## 2026-11-10 手枪原部件挂点接线与完整PG编译

最新工作已从单纯调指节推进到原手枪握把挂点接线。独立PG `3b0ef9af…e6aa1`完整编译／链接0，实际55个BOOL和22导出与父本一致，六个手部协议头与复用Native `180bc253…e79e55`逐字节一致；Native未重建，未stage、未实跑、未占队列或修改Publicv71。手型仍明确未通过，不是可交付／整枪验收。

生产适配修正：`weapon_snub_hand_mount.h`用原Snub握把三角表面与原Marcus掌面点定义专用刚体挂点，沿现有 `weapon_native_lancer_follow.inl` 只增加 `Weap_COGPistol` 分支。继续使用原basic_hold的COG_Pistol身份、原socket基准与freshness检查、同一固定挂载和唯一组件写入路径；不再保存拾取动画中的临时位置偏移。保留当前Native35°腕校准和同一枪口方向，不改原始控制器／物理腕／骨长，也不新增右腕协议或骨骼写入者。非手枪分支移除新增段后与父本原文完全相同。

依据与纠正：原动画首关键帧／独立SpaceBases给出的腕框架只是离线参考。保存11036的35条同帧accepted Plan结合真实Native校准显示逻辑上的持枪关系含35°腕旋转，不能把原版动画腕框架当当前渲染手。相同日志的mesh-in-grip最大偏差约6.97e-5，支持保留现有枪口基准；这不是GPU／固定臂长投影后的实际腕测量，也不证明11036首败已修复。前一轮300等数值保留其原始合成authoring范围，不推广为生产手型通过。

实际原WeaponMeshFollow＋当前Native校准的挂载检查编译／执行0，覆盖不同初始拾取姿态、刚体运动、比例／units及无效签名／基准拒绝；枪口方向与物理腕关系保持、起始动画不再决定最终挂点。新PG主手／弹匣三场景真实V4→C0局部消费者编译／执行0，固定全部115骨translation/scale及非手指骨。首个测试夹具把初始枪放到250UE保护界外，原guard正确拒绝；改host夹具未放宽guard。跨两份快照一起编译的host曾重复include公共头，现只把host include重定向到已核对相同的Native头，生产模块未因此改动。

手型拒绝记录继续保留：仅把主手交线300降到252的食指方向，在原开口三角网格的winding诊断中强内部顶点反从62增至107，明确拒绝。原枪219条边界边不是封闭实体，winding仅用于发现掩埋迹象，不能认证完整体积通过。掌面换到凸出处的新接触数据使同一正确35°构成的主手交线205→116，但它是后续未编译草稿；仍有拇指／中指内部迹象、混合掌蹼自交，不能以更少交线或0纯指间计数接受。新草稿完整1891手面和3146前臂／袖口／手面保持核查，主手腕角约31.86°、未称自然全角度通过。弹匣仍43对未通过，套筒没有扩新迭代。

主人最新顺序已纳入：先接通目标完整玩家动作逻辑并完成必要编译，之后在既有唯一窗口用现有脚本双路录制游戏／模拟器并实际看录像，再按真实输入、部件、状态／库存／射击时刻修正；禁止Computer Use，不另造录制平台。当前已知手型拒绝不占运行窗、不制作逐姿态运行包。下一点在正确校准下联合修主手拇指／中指／掌根及扳机过渡，再完成原匣抓姿；用一份兼容构成短验Snap／fresh Grip拔回／守恒／返射。若最先丢主握持，保留首败，不先打12发空枪前置。

证据：`snub-authored-mount-current.json`、`snub-authored-mount-build.json`、`snub-authored-mount-check.json`、`snub-authored-mount-preservation.json`、`snub-held-mount-derivation.json`、`snub-calibrated-full-self-check.json`。源码检查点见`authored-mount-source-checkpoint.json`。目标仍active、全手枪／头显FALSE，本轮progress。



## 2026-11-09 主握持与原弹匣自然手型修复检查点

当前只推进主握持＋原弹匣最小闭环。221733 authored-inspection因已知手型互穿及独立准入缺口已完整撤回，未stage、未实跑、不占后继窗口。11036持枪链首败及原因未证实的结论保持。公开v71和固定手动入口不变。

本轮纠正左右手MCP张指镜像方向，再在固定腕位下调整局部屈指；两份可审查header草稿位于`analysis/snub-20261109/contact-repair-draft-source/`。除main_rest/main_trigger/closed_left三张局部15节表外，header全部正文与已编译7f24父本逐字相同，主手非食指两个端点一致。没有加入右腕新接口或改物理接触点、枪姿、输入、骨长、比例或权限。草稿仍因剩余枪体交叉而拒绝，未覆盖既有冻结源码、未构建新的完整产品或运行包。

使用准确Native180bc253源码的现有唯一C0进行离线完整整臂authoring，MSVC `/W4 /WX /UNDEBUG`编译、原115骨fixture和合成稳态可达目标输出0；真实V4请求→原C0局部消费者三场景也编译/执行0。所有115骨局部translation/scale保持。该证据不代表实跑许可、真实控制器、GPU蒙皮、动态或头显验收。

| 相同完整C0稳态构成 | 原姿态纯指间交线对 | 本草稿纯指间交线对 | 原手／部件交线对 | 本草稿手／部件交线对 | 腕角 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 主握持静息端点 | 17 | 0 | 385 | 300 | 20.00° |
| 原弹匣抓握 | 57 | 0 | 75 | 43 | 13.19° |

上述交线统计阈值均为跨度超过1mm，不是穿透深度或完整体积内外判定；零纯指间计数不等于自然手型通过。每侧完整1891手面、3146前臂／roll／袖口／手面均核查，4组各3整臂视图及最终6个近距接触视图已亲看，仍明确拒绝完整贴合。两手间本稳态未发现超过1mm交线，不能推广到拔插或所有动作。

被拒绝尝试全部保留：弹匣31对方案把中指最近三点平均间隙增至约6.9mm；主手拇指仅追数量方案扩大最长交线；毫米级刚体注册仍穿插。最终草稿筛掉接触间隙恶化和最长交线增长的这些改动。最大跨度工具旧字段曾误读为0，现明确UE乘10为mm，旧记录不追改；旧2703选面遗漏roll，不称完整3146。首个整臂host工具误把render_base引用传进source，现有disjoint guard正确拒绝；改为host独立副本后0，未放宽生产保护。

下一点是主手静息食指／护圈、拇指／掌根及弹匣剩余接触的联合修复。主手247个完全无指节权重面中仍有7对交线，不能靠改指节表解释成消失。完成自然贴合后再准备一份兼容构成，明确交窗后短测Snap接近／退出、真实fresh Grip原匣拔回／守恒／返射；若最先丢失主握持，停在首败，不先打12发空枪前置。套筒、空枪、奖励、中断和全部件画面随后在同一兼容构成集中验证，不为每张姿态单独打包。

证据：`snub-contact-repair-current.json`、`snub-full-arm-review.json`、`snub-c0-finger-curl-refinement.json`、`snub-contact-repair-draft-source.json`及同目录失败/拒绝记录。本轮为progress，完整手枪和实体头显仍FALSE；目标保持active。独立源码Git保存见`contact-repair-source-checkpoint.json`，共享HEAD和索引保持。


最新状态：11036运行原174315 idle-route候选，取枪／首发与准备射击0有实际弹道，准备射击1缺弹道首败停止，弹药10/36；未射空、未进入换匣／套筒／返射。2页双路和8原图亲审、编码、8项精确恢复及一次retention均全终，最终唯一交Coverr54。帧408持枪链观察不可用、409 hold_failed发生在失败射击快照之前；两个取枪后的Native过期记录在其后，不能倒置为原因。原100ms规则保持，完整手型／Snap／整枪及头显未完成。独立V4主手／弹匣生产接线已编译ac7/c89并保存，未安装／实跑。

来源计数说明：下文／既有receipt所称1789或1791项PG“编译源”，是传给独立构建的完整冻结源码快照集合，含AGENTS、检查源码和辅助脚本；并非MSVC实际使用依赖去重计数。Native同名compiled_source_sha256集合也按源码快照理解。完整编译／链接退出0及改变的生产文件在实际include链内分别有证据，不以集合条目数量代替编译或游戏验收。

更新至2026-11-09，目标仍在进行。唯一检出 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`，分支 `codex/6dof-single`，公开v71未替换。17548/PGf854d4b9实际通过空枪新匣0/36→12/24、未上膛R2阻断、原部件54.35784mm满行程、真实release/两帧闭合、原事务完成与实际返射12→11/reserve24/bonus0、pause/B。备用采样前缀观察修正有游戏因果链支持；不把它推广为其他输入/旧失败修复。原匣无补弹返射36696、战术新匣守恒返射46708分别保留有效证据，无需不变重跑。全跨度/原图审查和全部恢复/编码/retention已终，唯一交HB110627。完整手姿、自然接触/可见套筒、反馈和头显仍未接受，目标不complete；Snub专用V3皮肤已沿唯一C0接入，当前144724候选待验证原物理观察与当前许可分离后的弹匣／套筒动作及实际画面。全部旧失败及54740异常保留，准确冻结由`analysis/snub-20261108/prepared-current.json`指向。

## 设计参考和本项目取舍

参考 [Valve 的 Half-Life: Alyx 官方介绍](https://www.half-life.com/en/alyx/)所强调的直接物体交互，以及 [Pavlov 官方产品介绍](https://www.playstation.com/en-us/games/pavlov/)的 VR 武器操作方向。上述资料支持设计方向，不证明具体版本的每个按键、弹膛规则或交互半径。以下是本项目的实现/验收目标，并非照抄上述产品的规格。

2026-11-10 补充核对了 VR Expansion 与 Unity XR 官方的附着点、手模型摆放和动态接触规则，取舍及证据边界见 [Snub 接触点设计记录](weapon-development-20261109/snub-contact-design-20261110.md)。下表保留早期各版本的局部证据，不能替代最新候选的完整流程验收；当前状态以本文顶部 Game17844 记录为准，修正后的套筒输入规划仍未游戏验证。

沿用原版 Snub 模型、弹匣、套筒和联动翼片，复用现有右手持枪、原版射击、单一手部仲裁和库存事务。主手控制完整枪姿态，头部提供观察。物理动作需要可见部件运动和原版玩法结果相符。

| 环节 | 本项目目标 | 当前证据 |
| --- | --- | --- |
| 口袋取放 | 右手从既有左口袋取出/收回手枪，姿态连续 | 49012 实际取出/收回/同一枪再取通过；自然贴合与完整运动视觉仍待验 |
| 单手瞄准与射击 | 当前枪口位置/方向与原版实际射击对应 | 49012 两次实际同源枪口单发与弹药 12→11→10 对应；非完整瞄准角度/舒适性验收 |
| 弹匣拔出/原匣插回 | 真实手部接触、物品随手旋转、方向正确；原匣插回不制造弹药或奖励 | 36696实际通过取出/放回及库存11/36/bonus0不变、回射11→10；自然贴合及全角度仍未接受。此前失败保留 |
| 有余弹换匣 | 左肩取新匣，保持抓握插入；闭合后可射击 | 46708新匣stage4→5→6→8、11/36→12/35守恒及真实返射12→11通过；完整手姿未接受 |
| 空枪换匣 | 新匣入座后仍阻止射击；拉动套筒并释放，确认原版部件回到闭合后才解除欠上膛状态 | 17548实际新匣0/36→12/24、阻断、原bone54.35784mm满行程/松手两帧闭合、事务完成及返射12→11通过；完整手姿和可见闭合仍不接受 |
| 动作反馈 | 接触区域、持握和插入阶段可辨识，换弹结果可读且及时消失 | 54740所选原图可见黄色弹匣提示；其他阶段/UI/完整手姿未验收 |
| 暂停/失踪/切枪 | 没有旧输入重放、虚构行程、重复奖励或多余弹药；退出时归还原版所有权 | 有事务离线覆盖，待游戏中断路径 |
| 手姿/双手辅助 | 主握把、弹匣和套筒分别贴合；评估副手托握的独立接触区和独占权 | 尚未完成，不把现有套筒或弹匣抓取当作副手控枪 |

当前 Snub 的抓取入口以既有左 Grip 为准；食指替代、被动 Snap、完整 GrabPose 和双手托握需各自接通与验证后才能标记完成。弹药沿用当前 Snub 规则，没有直接把 Lancer 最近的余弹回收规则扩展到手枪。

## 本轮确定的源码问题和修正

`snub_runtime::local_left` 用当前原始手姿和已验证的刚性握持关系计算左手枪局部坐标。旧套筒候选却将后续 `contact_world` 与初次 `bind_proposal` 保存的 `proposal_gun` 混合转换。主手平移/旋转后两者不在同一个枪坐标系，合法接触可能超出原 6.5 cm 候选半径。

现改为在正常 GT 读取套筒部件时，配对读取当前渲染枪姿态，将接触点转为同一枪局部坐标，候选仅读取该局部接触。部件读取开始先撤销旧接触有效标志；读取、比例或当前身份失败不发布新接触。删除不再使用的初始世界枪姿态缓存。原几何已含对象比例，局部接触不重复缩放。

此修正不改变抓取半径、100/40/50 ms 跟踪规则、观察间隔、真实输入、反重放、Native 身份验证、库存或部件写入者。它是源码坐标问题修正，尚未证明旧实跑换弹失败的完整根因。

## 验证记录

- 修改前以 MSVC C++17 `/W4 /WX /UNDEBUG` 编译并运行现有 Snub 检查：换弹分支/恢复/迟到调用/重放及 114 组守恒组合通过；套筒 39 项通过；弹匣挂载 12 项通过。
- 新接触检查覆盖 12 组平移/旋转组合和无效输入，包含旧转换会将合法接触算到 6.5 cm 外的明确反例。纯 CPU 通过不代表游戏抓取已通过。
- 首次完整产品编译暴露头文件被包含于匿名命名空间，造成嵌套命名空间错误；失败日志保存。已将新头放到现有全局 Snub 头依赖处包含，随后独立 Playground 完整编译/链接通过。
- 独立源码来自准确 Git 基线加本轮 Snub overlay，位于 `analysis/snub-20261108/source`；没有将工作区其他武器的未提交改动混入。具体基线、源码哈希和 DLL 哈希见同目录 `build-source.json`。
- 本轮尚无新游戏/模拟器运行、录像或实体头显证据；旧 `snub_reload_complete` 超时等失败不撤销。原版缓存来源和几何记录保留。

## 运行窗口交还后的顺序

先核实窗口明确交还、运行/编码/清理终止和现有启动保护；沿用受控后台、静音、实体控制器隔离路径，倍率服从当前受控配置。先做短手枪基本动作：口袋取出、一次实际枪口射击、原匣拔出/插回。若有具体失败，停在首个失败定位。

基本动作可用后才检查有余弹换新匣和空枪套筒闭合。套筒专项增加主手平移/旋转后的再次接触，核对真实输入、候选、租约、原版部件和回射，验证本次修正。继续覆盖半拉释放、跟踪中断、切枪/暂停和正常清理；复用已过且未变的前置证据，不重跑无关套件。

保存实际游戏和模拟器画面，检查完整保存跨度及关键接触帧。源码、CPU、编译、游戏动作、画面及实体头显分开记；整体手枪开发和验证尚未完成。

## 2026-11-08 21:14 私有实跑前准备

新增手枪专用 `run_snub_check.py`、`run_snub_operator.py` 和 `snub_core_chain.py`，均在既有私有分析目录。复用现有启动身份/资源检查、原版 Playground 进入、平滑 Source1 控制器输入、双路录像、暂停/B 返回及正常清理。仅在既有装备检查入口分派到 Snub，不执行旧 `give_arena_snub.py` 的 `GiveTo` 前置，也不改其他武器驱动。

首段动作检查口袋槽 0 的当前真实 `held` 回执及武器身份，右扳机的实际同源枪口/trace与库存消耗，随后同槽 `stowed`、同一武器重新取出和再射击。任一段失败即保留首个失败，不自动重试/补发输入。该段尚未覆盖弹匣、套筒、UI、自然手姿或完整手枪验收。

独立候选 `snub-prepared-20261108-211406` 已创建但未 stage：PG `4093f1812d049a1e50335c41d2b067bac7083e4ece1ad310e495fe7c543b622e`；复用冻结 Native `c98568f0…b12a59`、XR `16840b27…a7fc`。归档包含 2,277 个源码/辅助文件和准确的编译来源；运行入口对当前需要的 16 个驱动/辅助文件做哈希核对，其他会话的无关脚本变化不作为阻断条件。语法、注入锚点、保存的二进制和驱动哈希核对通过，未启动游戏。

只读进程检查未见 game/XR/recorder/encoder，但 Hammerburst 任务仍在活动且窗口尚未明确交还，因此没有将空闲进程列表当作交窗许可。已向其请求确认既有排队接收者，继续服从用户“等待他完成了再进去测试”的要求。

随后 Hammerburst 明确回报：本轮 cleanup、编码和 retention 已全部 terminal，运行窗口已唯一交给手雷会话 `01a1bb34-1592-7e72-8e1a-ab03d9dd6553`，手枪为其后下一位；HB 自己仅继续离线定位。手枪据此等待手雷明确交还，没有同时 stage。当前目标回合为 progress（增加并校验实际输入驱动、冻结候选），不是游戏通过或整体完成。

## Steam 启动前置条件阻断

手雷的两次受控启动均在原版“需要执行 Steam 才能游玩”对话退出，未进入 XR；相关 wrapper 报 `launcher_exit_code=1`、`remaining_owned_game=null`。后一次原版游戏 70452、驱动 50888 的运行及退出曾由手枪会话只读核实。后续进程清单没有 game/XR/录制/编码进程，Steam 本身为 51320；Steam 进程存在不证明账号/授权/游戏启动条件可用，也未确定报错根因。

手雷会话已向用户请求确认当前账号能否正常启动购买版游戏，并结束本轮、不继续重复启动。手枪没有接管其会话或为同一失败重启游戏，也未 stage 自己的候选。手枪候选的 16 个运行辅助文件及保存二进制再次哈希核对一致。当前仍需明确窗口交还和 Steam 前置条件恢复后，才能进行手枪游戏动作验证；源码/CPU/编译不能替代该证据。

## 2026-11-08 21:29 换弹驱动与明确交窗

手雷会话随后明确确认两次游戏按正常路径退出、没有 XR/录制/编码/retention 作业且 idle，并将共享运行窗口唯一交给手枪。手枪已在现有 idle 保护下 stage 新的 `snub-prepared-20261108-212858-reload`，原私有文字与数值记录保存于 `prior-private-text-20261108-212909`。公开 v71 未变。Steam 前置条件尚未确认恢复，因此没有启动第三次不变的正版游戏探测。

本轮新增 `snub_reload_chain.py`，接入专用启动/动作驱动，可选择 inspection、tactical、empty 或 all 模式；默认仍为已准备的基本动作。原匣拔出/插回检查库存不增加，战术换匣检查实际补充量守恒，空枪分支检查插匣后真实 R2 仍禁射、主手平移/转枪后按当前原版套筒位置抓取、拉到后端释放闭合并回射。有限弹药准备使用实际射击，所有动作仅经现有 Source1 控制器；没有 GiveTo、弹药 setter、伪造上膛回执或忽略首个失败。

接触规划从哈希固定的原 279 顶点缓存取得弹匣中心，从当前原版 bone4 取得拔出方向。原套筒拉动的规划按当前 root 比例将局部行程转换到世界行程；未改变生产行程/半径/时间阈值。规划逆变换最初使用近似正交矩阵的转置，保存 fixture 的严格往返检查以约 1.1e-6 UE 单位误差失败；现用实际矩阵求解逆变换，9 组往返最大误差约 8.9e-16 UE。3 组 root 比例、非均匀比例拒绝和破损缓存拒绝通过。上述为离线规划证据，不能称游戏/手姿通过。

新候选是仅驱动变化的派生冻结，复用同一已编译 PG `4093f181…3b622e`、Native `c98568f0…b12a59`、XR `16840b27…a7fc`，没有重新编译不变产品。21 个运行辅助文件已绑定哈希，5 种启动模式的生成代码语法检查通过，stage 后私有二进制与冻结文件哈希一致。记录：`analysis/snub-20261108/reload-planning-check.json`、`preflight-current.json`、`stage-reload.log`。当前回合有驱动实现、离线反例修正及私有 stage，为 progress；整体开发与验证仍未完成。

## Steam 只读诊断与受阻审计

后续没有再次启动游戏。检查现有 `tools/debug-launch/game_install.py` 与 `quiet_launch.py`：已经在子进程环境设置 `SteamAppId/SteamGameId=2523720`，以游戏 EXE 目录为工作目录；原版目录已有 App ID 文件。Steam 51320 与工具的 TokenUser 相同，完整性级别均为 8192，没有改变用户或权限令牌。

从当前 `J:/steam/steamapps/libraryfolders.vdf` 读取到五个已配置库根：J:/steam、E:/SteamLibrary、F:/SteamLibrary、J:/SteamLibrary、C:/SteamLibrary。它们的 steamapps 目录均未见 `appmanifest_2523720.acf`。该结果是当前安装登记观测，不证明账号没有许可，也不是原版报错的完整因果解释。未新建/伪造清单、修改 Steam 或游戏鉴权、切换账号、下载或改写原版游戏。

Steamworks 官方 `SteamAPI_Init` 文档列举客户端、App ID、相同 OS 用户上下文和当前账号许可等初始化条件；原版“需要执行 Steam”对话不唯一指向其中一个。参考：https://partner.steamgames.com/doc/api/steam_api#SteamAPI_Init 。本地观测：`analysis/snub-20261108/steam-readonly-diagnostics.json`。

同一个正版启动阻断已在至少三个连续目标回合中保持，期间完成了可独立执行的驱动、规划检查和候选准备。下一必要步骤是实际 Snub 口袋取枪/枪口射击，不能以更多重复离线检查代替；当前源码没有被证据证明的启动遗漏可修。因此将目标标记 blocked，等待用户确认游戏可从 Steam 库正常启动到菜单并退出，或提供库中缺失/启动失败的具体状态。完整手枪功能、游戏动作、画面/手姿和实体头显仍未完成，不标记 complete。

## 用户指定现有副本继续验证

用户随后在本手枪会话直接指定 `J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P` 为游戏目录。这是本次验证目标路径的新指示；实际 EXE 位于其 `Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe`。EXE SHA256 `bdc6705d…a8fa4` 与解析器支持身份一致，配置 SHA256 `79972362…70de` 在已知支持范围。记录 `analysis/snub-20261108/user-selected-game.json`。此副本的运行结果单独记来源，不能作为之前 Steam 启动失败已修复的证据。

手枪把当前空闲运行窗口明确暂还手雷一次有限 Grip 流程，等待其正常结束、编码/清理终止和明确归还后才 stage/run。手枪自己的新增 `snub-prepared-20261108-213700-selected-copy` 仅更新驱动：绑定用户指定的路径与哈希、运行前 idle 校验、保存/恢复私有 config 原字节并记录副本来源；口袋目标使用当前 body_slots 的中心 z=.02，避免把既有边缘点 z=-.18 当中心。产品 DLL 未改，未因路径变更重建不变 Native/XR/PG。该新候选此时未 stage。

## 用户副本首次运行 16292：驱动错误，未触达取枪

手雷正常运行、编码、媒体清理 terminal 后明确归还窗口。手枪 freeze `214052-ground-pocket` / PG4093f181 在用户指定副本正常进入游戏与 XR；只读实际库存槽 0 已有 `Weap_COGPistol`，无需地面拾取。驱动仍保留空槽时的原版 X 地面拾取路径，但本次没有执行该分支，不把它报为通过。

第一处 `ProjectionRecording.event('snub_pocket_begin', label=...)` 重复传入 label 参数，导致 TypeError，取枪动作尚未开始。这是本会话新增 Python 驱动错误，不是手枪玩法失败，也没有任何基本动作通过。另两处同类 event 调用已一起改为 action/phase 字段，当前 AST 检查不再包含 event 的 label 关键字；修正后的仅驱动派生 freeze `214542-event-repair` 准备做一次定向重查，产品 DLL 未改。

16292 的 Native cleanup、入口恢复、虚拟手柄恢复、hand_stop5/drainedtrue、XR0、launcher0、game 退出均正常，外层 config 原字节恢复。完整捕获跨度的一页双路概览已查看，录像编码正常结束；画面只证明启动/第一人称自由手，不包含 Snub 交互。保留 startup 第三人称曝光与旧失败边界。记录 `analysis/snub-20261108/core-16292-driver-failure-review.json`。本次严格区分 transport report passed 与动作驱动失败，不称基本动作通过。

## 49012 基本动作实际限定通过

修正事件关键字后的 `snub-prepared-20261108-214542-event-repair`、PG4093f181、Nativec98568f0、XR16840b27，在用户指定现有副本的游戏 49012 / creation134386192816424265 中完成：实际槽0 Snub取枪；一条当前同源枪口/trace对应12→11、备弹36不变；同槽收回；同一武器身份重新取出；第二条同源枪口/trace对应11→10、备弹36不变；原版暂停/B返回。所有上述 checks TRUE，操作报告没有 error。初始库存已含Snub，地面X拾取备用分支未执行、不报通过。

游戏、Native cleanup/entry restore、虚拟手柄恢复、hand_stop5/drainedtrue、XR0/launcher0均正常；外层私有 config 原字节恢复。73.6秒双路视频编码完成且ffprobe可读，全部3页完整捕获跨度配对概览及实际game00525/00746/00907已亲自查看：取出/再取后Snub可见，收回后消失。所选原图来自相邻保存墙钟时间，不冒充原子玩法/像素匹配。完整手部/握把表面贴合未接受；启动第三人称曝光、原版TORQUE BOW拾取提示等保留。捕获有实际间隔，不能据此声称逐渲染帧或FPS通过。

既有retention正常结束，删766文件/118068890bytes，保留49012/16292/40280及48h，源码/数值/日志保留。此后确认idle，按先申请顺序将运行窗口唯一交给Longshot `01a1bb22-8f4a-78f3-bc7d-676c2f3b2fd7`，并同步HB/Frag排队；本会话不同时stage/run。

证据 `analysis/snub-20261108/core-49012-review.json`，实录 `analysis/solo-m1-20261028/menu-firstperson-49012-startup-20261108-214751/dual-output/paired-output.mp4`。该次没有进入弹匣/套筒流程，不能验证本轮套筒接触修正、原匣归还、战术/空仓换弹、结果UI、完整GrabPose或实体头显。下一目标为协调窗口后的原匣取下/插回与换弹，只复用已过基本动作证据，不重复未改变的收回/再取套件。完整目标保持active，不标记complete。

## Snub 下一部件引导接线，仅编译

源码核查确认 Snub 的下一步提示尚未连接实际绘制；新增专用 `tools/playground/weapon_snub_guide.inl`，从 Snub 自己的既有正常GT tick调用。闲置/无欠上膛时使用当前原版弹匣几何中心，等待供弹时使用当前左肩中心，待上膛时使用同一武器的套筒局部接触点配当前枪姿态。原版 Actor/Engine DrawDebugSphere 的函数身份、native调用地址、7个输入字段类型/尺寸/不重叠和Vector结构均校验；使用黄色、非持久单帧提示。绘制返回成功不代表用户可见，实际原版/双眼画面仍待确认。

提示不创建部件/碰撞/手租约，不改库存、部件行程、真实输入或任何100/40/50ms等准入规则。握住部件、套筒仍处于拉动/等开/回位阶段、按住R2、忙/动画拥有手或当前身份失效时不绘制，以免可选反射绘制与关键动作争用当前预算。提示仅为视觉目标，不当成Snap/GrabVolume或完整GrabPose已完成。

独立低优先级构建沿用49012的准确冻结基础，仅叠加本会话Snub runtime/guide两个文件，没有混入共享目录其他武器的未提交改动。最终PG完整编译/链接成功，1,789个编译源文件哈希与冻结核对一致。新二进制未stage/未实跑，不能继承49012为它的游戏回归通过。具体当前候选与哈希见 `analysis/snub-20261108/prepared-current.json`、`guide-build-check.json`。已登记HB/Lancer/Frag队列之后的原匣取下/插回与回射未过单点，等待明确交窗；不启动额外运行器。

## 原匣实跑前的实际接触判据

下一段 Snub 驱动在中立手到达规划点之后、实际 Grip 之前，增加现有只读 helper 的实际左腕（骨11）和实际枪网格读取。将实际左腕变换到当前枪体，按原哈希固定279顶点缓存的边界计算到弹匣表面的世界距离，再与生产 capture 原有4.5cm条件核对。保存实际骨/网格、距离和是否在范围内；若实际腕尚未到位，则保留首个失败，不自动搬枪、扩大范围、补按键或用目标手/目标枪矩阵代替输出证据。

该判据是顺序读取的观察，不是原子帧证明或手租约；真正抓取仍走生产的当帧身份/几何/输入/独占验证。保留Snub当前实际网格规划，没有未经证明地套用其他枪的模型目标修正。已有9组规划往返、3组root比例及拒绝检查在本次驱动变更后通过，未跑游戏。

仅驱动派生冻结 `snub-prepared-20261108-221201-pregrasp` 继续使用同一已编译PG `7fb20418…b9b0010`；运行辅助哈希核对一致，未stage。已修正候选元数据里继承旧“DLL未变4093”措辞，明确49012仅为旧基本段证据，新guide/换弹/手姿没有实跑通过。窗口仍按既有队列协调，当前回合为驱动实现和离线检查的progress，不称整体完成。

## 54740 原匣实跑：按 Grip 前接触失败，清理也未通过

明确交窗后运行 `snub-prepared-20261108-223223-inspection`，PG `7fb20418…b9b0010`、Native/XR保持冻结。在用户指定副本的54740 / creation134386222613507097中，实际口袋取枪及第一发同源枪口射击通过；左手中立移动后、按Grip之前，实际骨11到原弹匣表面为0.219881237944305m，大于既有0.045m抓取范围，驱动停止。没有补按Grip，没有进入原匣事务，不能报告原匣取回、奖励守恒或换弹通过。

顺序读取中，规划弹匣中心距左肩75.6824UE，实际固定臂两段合计46.9950UE；枪网格前后平移仅0.000095UE、轴最大差1.49e-7。该证据支持准备更近的测试持枪姿势，但骨SpaceBases/component到模型世界的对应仍未完全证明，不能据此定论22cm偏差的全部原因或生产修复。完整2页配对概览及game00475原图已查看；所选原图黄色弹匣提示可见，仅接受这一提示可见性，不接受全部引导阶段或完整手姿。双路编码36.0667秒、ffprobe可读；捕获间隔不冒充逐帧证明。

本轮launcher退出1、Native cleanup verified=false（cleanup incomplete/deadline expired），输入恢复在tmp→json回执写入遇WinError5并回滚；入口/虚拟手柄恢复没有成功回执，保留失败。hand_stop5/drainedtrue、XR退出0，游戏及本轮发布/录制进程退出；进程退出不能代替恢复验证。外层config原字节、settings原SHA、head原始状态核对均通过，详见restore-54740.json。输入补丁仅在特定pid/creation进程内存，已退出进程没有跨进程残留；不将这一事实改写为本轮输入恢复成功。retention52464终止0，保留最新54740/26496/59800及48h，源/文/数值记录不删。编码/清理终止后唯一交窗HB；其独立复核备份恢复，本会话不并行修改私有配置或stage。

证据：`analysis/snub-20261108/reload-54740-review.json`。未证明本轮清理异常由guide引起，不撤销49012旧PG基本段的有限通过，也不给新PG继承完整清理通过。

## 定向重查准备：真实主手收近，尚未实跑

仅修改私有输入驱动，在首次原匣接触前从当时实际枪中心、肩/肘/腕和当前原始控制器映射，规划一次右控制器平移，将目标收至固定臂长减4cm的位置。沿现有平滑Source1手势发出，最多40cm，检查原始主手输入未漂移；不改变枪旋转、骨长、生产抓取半径或身份/时效/租约验证。随后重新读取实际枪和骨，仍需实际接触检查通过才按Grip，不重复补偿或以预测目标替代输出。

54740保存数据的离线回归预计原始右控制器平移[-0.153398,+0.108672,+0.267406]m；预计目标肩距42.995UE，仅是规划结果。9组原规划往返、3组root尺度及破损/非均匀拒绝仍通过；新增已近目标不动、过大动作拒绝、漂移输入拒绝和姿态角不变检查通过。尚无该姿势的游戏证据，不称接触修复或原匣通过。公开v71未改变，完整手枪开发、换弹/套筒/引导/手姿和实体头显验收继续未完成。

## 原版骨 getter 与外读换算同时保存

进一步核查私有接触观察：54740使用`SpaceBases × Component`的外读世界腕；产品捕获使用同票C0腕或原版`GetBoneMatrix`。两种读法是否一致尚无直接游戏对照，不把外读距离单独当作最终渲染/像素结论，也不撤销54740按其既定检查停止的事实。

手枪只读helper新增显式`--body-bones`，通过既有Bridge在GT调用原版GetBoneMatrix读取实际左肩/肘/腕（8/10/11），校验同一pawn mesh、115骨数量、组件/数组头稳定和身份。下一轮规划与按Grip前核对固定使用原版getter，不按哪个结果更易通过来选择；同时保留独立SpaceBases读数及逐骨距离差，原getter不可用即失败、不回退。仍是顺序观察，不冒充原子帧、硬件或完整手姿证明。

新增三骨getter坐标解析与独立读数分离、错误mesh/game拒绝检查通过，原有规划检查继续通过；没有游戏执行或产品DLL变化。候选以`prepared-current.json`的新冻结标识为准，队列仍为原先登记的一次原匣单点。完整GrabPose还需Snub专用部件姿态向既有C0发布，源码缺口记录于`analysis/snub-20261108/grasp-source-gap.json`，未接通不称完成。

## 私有腕逆规划修正：与冻结生产 C++ 校准函数对照

源码对照发现私有`wrist_target`直接使用原标记[-5.42556,0.456734,0.202015]，遗漏冻结生产`marcus115-openxr-grip-authored-v3-wrist-down20`对标记的20°旋转。冻结候选与当前校准头SHA同为772e6ffe…bd5f97；未修改该生产设定。

新建仅离线C++观察程序，直接包含该冻结头并调用实际`marcus115`及`native_wrist_target`，用27组（3比例×3单位×3控制器旋转）检查私有逆规划。修改前最大误差3.299772UE（1.75倍模型；1倍约1.886UE），明确失败已保存；私有规划补上同一已存在的标记旋转后，27组最大误差7.38e-15UE，通过。9组既有世界规划往返、3组行程尺度及身份/异常拒绝检查继续通过。旧Python往返只检验自身数学一致，不能替代这次与生产函数的交叉核对。

只修私有输入规划，未改Native、PG、骨长、腕设定、接触范围、租约、库存或实际动作。该差异约1.9cm，不能解释54740全部21.99cm外读偏差，不称抓取根因或游戏修复。后续原匣单点沿用原排队优先级，准确新候选由`prepared-current.json`指向；旧225618/231437冻结保存但不再作为下一启动候选。证据`wrist-oracle-before-repair.json`、`wrist-oracle-check.json`及可重编译离线程序保留，未增加游戏实验。

## 3208 定向实跑：接触/授予通过，获取阶段生命周期丢失

收到Frag的明确唯一交窗后，准确stage并运行232252-wrist-conformance，沿用PG7fb20418、Nativec98568f0、XR16840b27、用户指定副本及受控0.3。游戏3208 / creation134386269978150144完成原有口袋取枪与第一发同源枪口射击。右手经实际平滑Source1输入收近后，左手到达弹匣；原版GetBoneMatrix腕到原弹匣边界距离0m，独立SpaceBases读数与三骨getter相差0.00027～0.00043UE。该样本支持本轮读数一致；不把它倒推为54740全部原因已证实，也不当作原子帧或像素贴合验收。

frame604/token1实际候选通过、new grant=1、lease=3、accepted_intent=1，进入acquiring1/serial1，原自动动画停止已记。之后没有old_magazine阶段；frame633后读取显示thread_or_unarmed，随后opening/current lifecycle5→6、其他对象/hold/world/session/input未变，触发恢复。驱动等待目标阶段超时，保留真实失败。没有退匣/插回、守恒、回射或全部换弹通过，transport报告passed不能代替动作结果。

已亲看2页完整捕获跨度总览和game707/715/745、sim983四张原图；枪/原匣仍在位，手部/袖口遮挡不接受完整自然贴合，启动第三人称和原版拾取/HUD问题仍保留。双路视频56.0667s，编码73818终止0、ffprobe可读，双路结尾覆盖通过；游戏捕获最大间隔2.067s、模拟器350ms，不声称逐渲染帧或FPS通过。

Native cleanup/entry/input恢复均TRUE，hand_stop5/drainedtrue、XR0、launcher0、game退出；config原bytes、settings原SHA、head原缺失状态均精确恢复。媒体retention96573实际终止0，源码/数值/日志不删，核idle后已唯一交HB。54740旧异常不撤回。证据`analysis/snub-20261108/reload-3208-review.json`、`restore-3208.json`及原日志保存。

主人最新等待要求已接受：正常排队不结束本轮或标blocked，保持有界等待，明确交到自己再运行，不抢占/终止持有者。当前共享窗口归HB，本会话离线定位。

## 获取失效原因的有界日志候选，仅编译

冻结编译源码核对后，现有3208日志只能确认client失去有效状态和生命周期变化，未保存Snub当时的last_disarm_reason。新增的唯一产品改动是在既有最多16行的Snub contact日志追加已有atomic原因、client phase/armed及Snub stage；不新增原子写入、回调、追踪器或输入，不恢复拒绝的Frame/租约，不更改时效、反重放和身份边界。各值是独立观察，不能单凭最后原因认定完整因果。

在准确的原7fb编译源/同配置上只叠加该Snub日志文件，独立低优先级/m2配置、完整编译链接均exit0；1,789个编译源哈希核对，除该文件外一致。输出PG4d5ba6de…2c94e9，Native/XR不变，尚未stage或实跑。它是有明确新增信息的下一次定向观察准备，不是获取问题修复声明。构建证据`analysis/snub-20261109/acquire-observation-build.json`；继续推进原匣流程和完整手枪目标。

随后同一冻结客户端源码又确认：`clear_gt`会清除发布但不更新last_disarm_reason；只看atomic最后原因可能漏掉已保存的game_changed等撤回。故在同一有界日志中补入现有只读`withdrawal_on_gt`回执的failure/difference/frame/sequence/token，标明其独立关联，不拿旧回执当本帧权限。该getter只复制GT现有POD，没有gate、Native调用或写入，不改变撤回语义。

增强版独立配置/完整编译链接exit0，仍为原7fb准确源码的同一Snub日志文件变化，PG80f692b1…578ec9；其前版4d5仅保存、不曾运行。增强版未stage/实跑，准确冻结见prepared-current。构建`analysis/snub-20261109/acquire-withdrawal-build.json`。HB已确认下一队列为Boltok→Frag伤害→Lancer HUD→掩体→HB原匣→Snub观察，本会话保持等待，不抢占或把正常排队记blocked。

## 模块边界与 Snub 候选独占回归

主人要求模块化/解耦后，在实际生产观察器与人工对象fixture中复现：合法已持有Snub仍运行自身一次及四个无关候选，外部producer的pending可污染本枪。该CPU依赖隔离缺口有明确反例，但不是3208生命周期原因证明。

共享组合层由Boltok会话统一精确分派，本会话不再修改共享分派点或复制dispatcher。新增Snub七项回归在当前共享组合层和私有冻结观察器的最小Snub独占分支均通过，覆盖source-only不执行候选、伪包/未持有/坏身份，以及正常Lancer路由；夹具不授予游戏租约。公共HandClient头、Native、输入追踪和仲裁不变。材质读取的公共提取由Boltok负责，Snub原同签名适配入口保留。

为不混入其他枪的新控制与Native改动，私有观察包仅将同一Snub独占规则回移原7fb基线，连同已有有界失效日志，共两份编译文件变化。低优先级/m2完整编译链接exit0，PG497a0bdd…30578f，1,789源哈希核对；Nativec985/XR1684复用。尚未stage/实跑，不称原匣修复。构建routed-observation-build.json，准确下一冻结仍见prepared-current。共享组合层不被该私有回移覆盖。

## 70440 独占候选实跑：授予通过，动画获取未推进

HB14312正常收尾并明确唯一交窗后运行010736/PG497a0bdd，Nativec985/XR1684复用，私有0.3、用户指定副本。游戏70440 / creation134386319909302745口袋取枪、首发同源枪口12→11/spare36通过；实际原GetBoneMatrix腕到原弹匣距离0m，三骨getter与独立SpaceBases差0.00037～0.00046UE。frame490/token1/new grant/lease3/accepted通过，进入acquiring1/serial1，原自动动画停止已记，但始终没有old_magazine3，动作失败。

frame491客户端仍armed/phase2；last_disarm256及withdrawal frame342/token0/reason5是早前独立回执，不解释本次获取。原先快读误将line218清理当下一帧Grip丢失，已明确撤回：完整日志中清理位于frame614/sequence2828，约8.7秒后，是等待超时后的释放收尾，不能当初始失败原因。下一定位为动画所有权/闭合恢复/部件隐藏前的具体拒绝点，不延长时效或绕过严格身份/释放/反重放。

两页全跨度配对总览及game587/593/605、sim894四张实际原图亲看，枪和弹匣仍在位；手/袖口遮挡，完整手姿未验收。启动第三人称、原版拾取/HUD仍保留。双路50.966667s编码79495终止0、ffprobe可读；游戏捕获最大间隔2233.365ms，不当逐渲染帧/FPS或实体头显证明。Native cleanup/entry/input restored均true、XR0/launcher0/game absent；config原字节、settings原SHA、head原缺失状态精确恢复。retention62875终止0，删2042媒体/318277381bytes，源/文/数值不删。核idle后唯一交窗Boltok，后续Frag→Longshot→Cover reader，本会话离线定位，后续新检查排队尾。证据analysis/snub-20261109/reload-70440-review.json、restore-70440.json、70440-selected-contacts.json。公开v71与完整手枪未完成状态不变。

## 动画获取拒绝点候选013007，仅编译

Snub专用动画函数记录原有读取/curve/控制/获取的拒绝阶段及curve拒绝源码行；Snub运行阶段最多16条、仅状态变化时记录frame/sequence/token及实际known/down/tracked、animation active/stopped。既有判据、短路调用次序、输入/身份/时效/租约不变，不在拒绝后继续执行。共享源码保留Boltok添加的service_unselected和recover_serial(serial)，私有包仍用准确旧基线，不覆盖共享组合层。独立完整编译exit0，1789源SHA核对，PG0c904af9…62cbfd；Nativec985/XR1684复用，21运行helper一致。冻结snub-prepared-20261109-013007-animation-gates，未stage/实跑。独立Git恢复ref refs/codex/snub-private-animation-20261109-013007 / ec68dbe386e32c97ba2df136ab6b0af7d34804be，shared index/branch未改变，不发布或cherry-pick旧基线到共享当前树。

另从已存原版AR_Reload导出（SHA3c5885f4…fba70c）取5148字节原流，执行准确生产read_curve函数；只有Engine对象/属性/身份/时效接口为固定fixture，内容验证和SnubSlideCurve仍为产品实现。结果接受、curve_failure0、open47/closed49，说明该已存原片段的结构/数值在这些fixture前提下可通过；不证明游戏所选节点就是此片段，也不证明实时metadata/current及时效或取匣通过。证据saved-curve-reader.json和可重编译程序保留。Boltok已将本次有新定位信息的检查登记在既有HB之后队尾；等待明确交窗，不抢占。

## 48800 原动画观察拒绝实证

Lancer31424全收尾后明确唯一交窗，准确stage/run013007/PG0c904af9。游戏48800 / creation134386365230610926，实际口袋与首发同源枪口12→11/spare36通过；原getter腕接触0m、frame522 token1 grant/lease3/accepted通过。frame523/sequence2422日志显示acquire gate1，即hammerburst_animation::observe_kind(pistol)拒绝；curve未执行、animation active0/stopped0，left known/down/tracked全1，client当时armed/phase2。早期withdrawal frame228/token0/diff8不解释本次；后续withdrawal位于释放收尾之后，也不倒置为初始原因。等待8秒未到stage3后正常释放，line227清理。具体observe拒绝条件未证，不称完整抓取/换弹通过。

两页完整跨度双路及game609/616/630、sim894四张原图已亲看，原匣仍在位，手/袖口遮挡不接受完整贴合；startup第三人称/原HUD拾取仍保留。50.766667秒配对编码23202终止0、ffprobe可读。Native cleanup/entry/input restore均true、drain1、XR0/launcher0/game absent，config原bytes/settings原SHA/head原缺失精确恢复。retention95733终止0删2002媒体/353895235bytes，源/文/数值不删；核idle后唯一交Boltok013903。后续Frag020326→Cover→Longshot021930→HB023148按既有队列；本会话离线定位，不追加本窗。证据analysis/snub-20261109/reload-48800-review.json及restore/contacts/原log。主人要求不主动抄送统筹已接受，仅必要直接窗口/源码依赖协调继续。

## 024436 原观察器拒绝行，仅编译

与Longshot确认该函数无并行编辑后，原observe_kind增加默认nullptr的unsigned* rejection；仅Snub两个调用显式传地址。第一段现有kind/current/asset/attachment四条件按原短路顺序分开拒绝，其余条件保持，仅原false返回保存拒绝源码行；原输出成功规则不变，拒绝时不返Frame/许可。Snub原最多16条状态变化日志追加该行。共享当前树保留longshot/engaged_for/HB新seek；准确私有旧基线不覆盖共享当前树。

独立完整编译exit0、1789源SHA和21运行helper核对，PG483903ed…aa131109，Nativec985/XR1684复用。冻结snub-prepared-20261109-024436-observe-rejection未stage/实跑；独立Git refs/codex/snub-private-observe-20261109-024436 / 48ead7f5e8dd79e7f288ee7332cbb962037175bb，shared index/branch未动。准备脚本曾因保留断言误写HB helper名称而停止，实际peer函数未被覆盖；纠正实际名称后完成，其失败记录保留，不冒充产品/游戏失败。已向当前直接持窗者申请排现有尾项之后，等待明确唯一交接。完整原匣/有余弹/空枪/套筒/反馈/自然手姿和头显验收仍未完成。

## 18760 观察拒绝块定位，正常交窗

Lancer32408最后全收尾后明确唯一交窗，准确stage/run024436/PG483903ed。18760完成口袋、同源首发12→11/spare36，原getter接触0m及frame512/token1新租约通过；frame513/sequence2246记录observe_line70，位于原动画名称和finite/time范围块。前置武器/挂接/节点/字段读取已通过；左known/down/tracked全1，client有效，curve尚未到。尚不能单独区分名称与时间子条件，原匣仍未取出，真实失败保留。

两页全48.333333秒跨度双路及game591/598/612、sim840四张原图亲看，原匣在位且手袖遮挡，完整贴合未验收。Native cleanup/entry/input TRUE、drain1、XR0/launcher0/game absent，config/settings原字节和head原缺精确恢复；encode39524与retention5717实际exit0，媒体删除1922/318606116bytes。核idle后唯一交Frag030808，其后Boltok→Cover→Longshot等既有队列，不追加本窗。证据analysis/snub-20261109/reload-18760-review.json、restore/contacts/ffprobe及原log。

## Native 租约内显式选取 Snub 原片段，仅编译候选

复用保存的原版脚本和包数据（使用reverse-engineer-anything技能区分静态、推断与运行证据）：WarWeapon.PlayWeaponReloading只有WeaponReloadAnim非None才调用PlayWeaponAnimByDuration；Snub转调该父实现。读取准确原包CDO三个继承层的完整tag流并核各exportSHA，WarWeapon/Weap_PistolBase/Weap_COGPistol都没有序列化WeaponReloadAnim，Lancer则显式AR_Reload。该缺省差异支持原代码假定Snub自动选中AR_Reload有误的推断，但不把缺失tag单独等同本局实时属性。REA当前未开native目标，UE3已有专用有界tag读器和保存脚本足够本次静态对照，没有重做全EXE分析。

新增Snub专用受限路径：现场以NameProperty严格读取WeaponReloadAnim，确认实际None且前后值一致、当前精确Native reload serial/identity、部件租约及输入时效都有效，才复用同一个snub_visual_clip/animation journal选择原AR_Reload。Journal在Play之前持久化；成功后同GT StopAnim、禁notifies的闭合SetPosition，且Native journal始终绑定同一serial/identity/owned_state。没有改WeaponReloadAnim属性、ForceReload库存、计时/奖励或仲裁，不接受任意旧片段或绕过命名检查；非None继续原有严格获取路径。原独立visual-only路径保持，Native路径仍需先退出Native再按原journal交还。

准确18760编译源上仅Snub runtime/visual_clip两文件变化，完整PG编译exit0，PG59192d9f…9ebff34a，Native/XR未变。抽取实际生产native_clip_receipt_current/no_configured_gun_reload函数执行26个CPU fixture用例，旧serial/foreign weapon/visual-only/missing ownership/非None/坏字段/前后值变化/晚期所有权和current失效均拒绝。只有Engine读接口为fixture，未调游戏，不是原动画/取匣成功证据。original-reload-defaults.json、native-clip-authority.json及可重编译程序保留；下一候选准确标识见prepared-current，仍需排队明确交窗后实跑。

## 36696 原匣流程通过，整体手枪未完成

HB7712全收尾后明确唯一交窗，准确stage/run042943-native-clip/PG59192d9f。36696 / creation134386454985572646实际口袋与首发12→11/spare36通过，原getter接触0m/新grant通过；原始记录现场typed WeaponReloadAnim=None，Native serial1内原AR_Reload只成功选取一次、停止并闭合。stage1→2→3，可移动原匣实际拔出、同物插回3→6→8，complete result1，loaded11/reserve36/bonus0不变。随后同源实际shot2/真实弹量11→10，正常暂停/B返回。该受限片段选择分支live通过，不把它推广为全部旧失败根因或完整手枪完成。

两页55.6秒全跨度配对概览与game612/624/635/647/659、sim901六张原图亲看；弹匣部分在手下可见，掌/袖口遮挡，不接受完整表面、自然手型/全角度。startup第三人称与旧HUD/拾取在此旧私有基线仍保留，后续其他包的UI通过不倒推本包。Native cleanup/entry/pad TRUE、drain1、XR0/launcher0/game absent；config/settings原bytes/head原缺精确恢复。encode83311/retention89915实际exit0，媒体删除917/125276048bytes，源/文/数值不删；核idle后唯一交Frag更新050613-tag-and-empty，其后Cover由其维护。本会话不追加游戏，证据reload-36696-review.json、restore/contacts/ffprobe及原log。

下一段仅私有驱动新增replacement模式，组合已有tactical与empty两条尚未实跑路径，不重复inspection；相同产品PG59192d9f/Nativec985/XR1684，不重编译未变Native或PG。元数据scope同时改为实际reload分支，不再误写stow/redraw覆盖；历史原receipt不重写。空枪前提仍用必要真实扳机逐发射空；无ammo setter，插匣未拉套筒时需真实射击阻断，主手平移/旋转后再接触当前套筒并完成原部件回闭/回射。首失败停，不展开不相关矩阵。代码语法通过，动作本身仍待唯一交窗实跑。

## 48924 新匣段失败，原匣通过记录独立保留

HB53224全部收尾后明确唯一交给Snub051415。48924 / creation134386479355640063，PG59192d9f、Nativec98568、XR16840b，口袋和实际首发12→11/spare36通过；live None与Native原片段选择再次成立，原getter接触0m，stage1→2→3→4完成原匣拔出和松手。等待肩部新匣时Native disarm257（input_lost），随后同对象/hold/world/session/input/registration下life5→6，Snub按原边界cleanup。未到stage5，新匣插入、返射及空枪/套筒段未通过或执行。game_difference8的新withdrawal发生在失效之后，不倒置为根因。

末尾shared_hand_rejected/blocked/aborted均0，714次完成含34次absent；stereo gate总计stale19、bridge_read1。只有汇总数，没有该次拒绝来源时间，不能据此断言过期或桥接争用触发了本次输入失效。XR CSV按约1秒抽样，不能将相邻CSV行的published_ns差当作发布停顿。输入失效先于生命周期恢复这一顺序成立，精确来源仍未证实；不放宽100ms、身份、Native事务或真实release检查。

55.666667秒双窗两页全跨度总览及game615/627/639/653/666、sim906/928七张原图亲审；掌/袖口遮住大部分部件接触面，完整贴合仍未接受。startup第三人称、原拾取字样/横条仍可见。runner0、encode/review0；Native/entry/pad恢复通过，drain1/XR0/launcher0/game absent，config/settings原bytes/head原缺/wrapper恢复通过。retention20656实际exit0，删除1107媒体138160159bytes，最新48924/53224/71272；源/文字/数值保留。CIM确认空闲后唯一交Boltok055114-resource-group，后继Frag053206-empty-debit，不追加游戏，不抄送统筹。证据reload-48924-review.json及restore/contacts/ffprobe、原log。

后续仅私有观察候选：在准确c98568冻结Native源的capture.cpp现有两处stereo read/gate拒绝分支加入最多64条write-once atomic/POD来源、QPC、received、sequence、bridge contention记录，通过原summary输出；同一失败返回、invalidate及所有输入/身份/时效规则不变，无新控制链/引擎writer。Native完整编译0，1149cb00fdca9014a8b2a7411745c7c8c633a43179a748c9e2601ef8e59095c9。配置首次缺快照未收录的辅助源码，按记录base6db24c6补齐原构建/测试文件和原MinHook路径后配置0；只构建DLL目标，没有运行游戏或旧测试套件。新增记录器的实际函数在CPU上以并发写/读检查64条上限、发布一致性和不覆盖，/W4/WX通过。私有驱动使用既有identity_boundary/cleanup日志首败即停，不再在事务已失效后继续肩部Grip；以48924实存日志核对边界识别。PG59192与XR1684复用，未称修复或实跑通过；共享Native文件未编辑，publicv71保持。


## 46708 战术新匣与空枪阻断通过，套筒首败保留

Cover r13/61864全收尾后明确唯一交窗，独立idle/060545 prepared SHA29c478d0/21helper/制品核对通过后stage。游戏46708 / creation134386522595166066，PG59192d9f复用，Native1149cb00仅原拒绝观察、XR1684不变。战术stage1→2→3→4→5→6→8，complete result2；真实11/36→12/35，返射12→11。必要逐发实射到0，再用新匣真实0/35→12/23，stage7未上膛时实际R2无轨迹/不扣弹。主手平移4cm并yaw+.12后读取当前枪与套筒再规划接触；首次抓取附近snub_charging返回recovery_required，runtime cleanup261，未闭合/未空枪返射。驱动记录cleanup后立即停后续动作，未追加本窗case。

Native19条观察均为动作开始前startup seq1010 stale，首次年龄109.9849ms，末次286.0545ms；bridge_read0，动作期间无capture拒绝行。动作空闲段出现的input_lost与最终shared_hand blocked2/aborted2不能证明48924的源因；本次未复现其stage4生命周期失效，不称修复。套筒恢复精确子分支仍未证明，不归因于新Native、时效、机械几何或手势。

134.166667秒H2641920x660双路，5页全跨度总览与game601/625/652/711/1474/1608/1738/1776、sim929/2776/2849共11原图亲审。弹匣在手下部分可见；掌/袖遮住接触面，末段左前臂遮住大部分枪/套筒，不接受完整贴合或可见闭合。startup第三人称及旧拾取/横条保留，黄色套筒引导可见。runner40522/encode19822/retention71593均实际terminal0；transport passed与operator真实失败分开。Native/entry/pad恢复TRUE、drain1/XR0/launcher0/game absent，config/settings原bytes/head原缺/wrapper恢复、21helper结束hash通过。retention删4177媒体758935656bytes，保留46708/61864/8092，源/文/数值不删。CIM与idle复核后唯一交Boltok063023/plan74623e62，再由其交Frag064829；本会话仅离线续查，不抄统筹。

下一候选只在Snub charging八处已有恢复返回记录源码行、Stroke拒绝/阶段、真实hand/Decision/serial与动画状态的现值；最多8条，失败才输出，无新增Engine查询或permission。原恢复分支和判据保持。后续动作使用已有empty模式，不重做战术新匣。未实跑的诊断不能称套筒修复或全手枪通过。


## 27104 明确套筒租约丢失，incumbent定点修正待实跑

Cover reader-r16/32760全终后明确唯一交窗，独立071103 prepared SHA5ef3a024、21helper/制品/idle核对后运行empty-only。游戏27104 / creation134386564938037234，PG6c0b46e0、Native1149、XR1684。实际射空及新匣0/36→12/24、未上膛R2不射/不扣弹通过。当前套筒初次成功捕获后，frame1420/seq5490/token3/life5在charging source98拒绝：Stroke pulling/rejection0、actual约-3.31e-7m/wanted0、tracked/known/down均1、input age51.9902ms、same_key1；Decision lease_lost12/release_required1/leaseNone。原动画active/stopped/serial1、Native acquired1/serial1、seek_pending0。因此首恢复是要求的手部租约已撤回，不是本记录证明的行程、动画或100ms失败；后到的withdrawal game_difference2不倒置为触发源。未闭合/未空枪返射，首败停止并正常退出。

四页完整110.9秒双路及game1270/1294/1322/1364/1486/1514/1528/1534、sim2334/2360十张原图亲审；末段左前臂挡住套筒和大部分枪，掌/袖遮接触面，完整fit仍未接受；startup和旧HUD仍保留。runner39518/encode73091/retention4113实际terminal0，Native/entry/pad恢复TRUE/drain1/XR0/launcher0/game absent；config/settings原bytes/head原缺/wrapper恢复、21helper结束hash通过。retention删2404媒体407383224bytes，保留27104/32760/72536。idle/CIM复核后唯一交Boltok072100/plan858b7380，后继由其维护；不追加游戏，不抄统筹。证据reload-27104-review.json及原log/restore/contacts/ffprobe。

检查确认Snub进入charging时清空旧magazine held；新Stroke.begin成功后却没有记录实际获得的新Decision lease，下一帧propose只重复candidate，未给原仲裁器要求的incumbent。定点修正仅在charging、前idle→实际pulling、Result waiting、known/新grant和原visual::granted全成立时，把该精确d.lease及本frame存入既有held/held_after_frame。既有propose仍只在后续frame、同token、同lifecycle有效租约下回报；不产生/续期租约，真实release、Native/input/身份/时效、几何和行程规则不改，不取消其他槽。共享peer的service_unselected/recover_serial保留，私有准确编译源只backport这段，不覆盖共享组合层。

提取实际visual::granted、新helper、原incumbent生产片段，连接实际SnubSlideStroke和原LeftHandInteractionOwner。旧路径首retained帧真实返回12，RED退出1；修正路径18项GREEN退出0（/W4/WX），覆盖下一帧retained且非newgrant、同帧不得补incumbent、错token/生命周期、真实release后旧token重放、外部incumbent冲突不得移除，以及无新grant/坏key/失踪/松手/错误阶段或结果不得记录。当前源码与准确冻结编译源分别验证；Engine和Native元数据准入在该宿主测试之外，不替代游戏验证。完整PG编译0：2732b6b26af37feb4fd5242eced530c86689804032a27e451be42124f8886815，Native1149/XR1684复用。下一仍empty-only单段，不重复战术/inspection，不称完整手枪完成。

## 58472 无即时租约恢复，但套筒完成仍超时

Cover r19/60336全收尾明确交窗后，核对082247 prepared SHA1d767e84、21helper、PG2732b6b2/Native1149/XR1684与idle，运行empty单段。58472 / creation134386599856401326实际射空、新匣0/36→12/24及未上膛真实R2阻断通过，stage1→2→3→4→5→6→7。此次没有snub_charging_recovery或27104 source98的即时lease_lost恢复；但8秒等待snub_reload_complete超时，没有完成闭合或空枪返射。stage7 runtime cleanup345在operator停用native_lancer_r2之后，不能倒置为首败；gate1接触回执本身也不足以证明整个行程的精确租约持续有效。

4页119.566667秒H2641920×660双路完整概览及game01248/01272/01300/01330/01446/01477/01492/01505/01579、sim02313/02340/02495共12原图亲审。掌/袖遮住弹匣接触面，套筒段左前臂遮住大部分枪；完整表面贴合、可见闭合不接受。startup第三人称/短暂显示切换、拾取和白/黄引导仍保留。runner2246、encode75188、retention60568均terminal0；Native/entry/pad恢复TRUE、drain1/XR0/launcher0/game absent，config/settings原bytes、head原缺、wrapper及21helper最终哈希通过。retention删2704媒体482407164bytes，保留58472/60336/65324，源/文/数值不删。独立idle/CIM核实后明确唯一交Boltok085057-phase-timeline（原084022同一slot），后继由其维护，无追加本窗动作。完整记录reload-58472-review.json及restore/contacts/ffprobe。

静态原getter记录的左臂总骨段46.9949328273UE；该次套筒起点到肩45.634795924UE、请求后端43.397440467UE，均在总长度内。这是顺序静态读取，不能证明动态到达，也不支持“请求端点必不可达”的诊断。continuous-hand-plans仅32条尾环、fresh-pose-producer容量8，读取时已是8秒超时后的尾部，不覆盖手势，不能据此判实际腕部行程。依据：58472-slide-reach.json和冻结Native现有容量定义，不改IK/固定骨长。

下一私有候选仅在已有Snub套筒处理点记录最多32条状态改变POD：等待分支、实际/目标/已发出的距离、原曲线和部件失败行、已有Stroke/动画/Decision状态及最近已读取手样本与对应frame/sequence。无新增Engine查询，不延长超时或放宽满行程/真实release/时效/身份/库存规则。移除观察增量后全文与上版处理逻辑相等，原Engine依赖调用数量相同；完整PG配置/编译0，bc2cff99039a1db14e55a87522aa3e1f8a87c982585e1c43a0181c50831f1b25。准确1789源仅weapon_snub_charging.inl变化，Native1149/XR1684和21helper不变。尚未stage或实跑；只在准备完成并重新获唯一交窗后进行一次empty段观察，不称修复通过。

## 61796 新匣退回，套筒观察未被执行

Cover r24/33420所有作业终态且明确唯一交窗，核b4e41518/21helper/3948归档源及4制品、独立idle后stage093107。首次stage误把完整相对路径传给只接受目录名的脚本，assert在任何stage修改前拒绝；按原接口仅传目录名后正常stage，失败日志保留。61796 / creation134386639108580272实际取枪/首发、逐发射空0/36、原匣拔出/松开stage1→2→3→4和肩部新匣4→5成立；插入尚未成功即5→4，驱动等待stage7超时并停。未到插匣/欠上膛阻断/套筒，charging_progress0条，不把这次当观察成功或套筒修复证据。

token2持新匣阶段出现read-consumer stale_before5回撤，age约100.38/101.19/100.74/99.91/100.36/105.48ms；之后hand_disarm256即context_lost，而非input_lost257。末端真实release后stage5→4，runtime cleanup345在operator停用之后。Native stereo拒绝14条全是startup seq1075，没有动作期间的capture拒绝，不能拿它解释本次消费端撤回，也不能倒推48924已解决。需核对同一帧原部件工作量与读取时效，不放宽100ms或跨帧续许可。

109.533333秒H2641920×660，4页全跨度概览与13原图亲审（game01227/01239/01250/01263/01277/01291/01317/01338/01354，sim01942/01963/02048/02112）。手和匣局部可见，掌/袖遮实际接触面，不接受完整fit；startup第三人称/等待卡切换、旧拾取和白色reload HUD仍保留。runner60710、encode60871、retention2598均terminal0；Native/entry/pad恢复TRUE、drain1/XR0/launcher0/game absent，config/settings原bytes、head原缺、wrapper及21helper最终hash全通过。retention删2439媒体419570353bytes，保留61796/33420/58316最新3局48h；源/文/数值保留，Native原status另外保存。独立idle/CIM复核后明确唯一交Longshot095812（同已受理094103槽，准确prepared9f772390/PG913725d8），本会话不再占窗或追加case。

离线发现Snub每个正常持匣tick的show和feed_sample重复读取同一f的整套poses（实际腕和当前枪、多个current）。正在把同一次同步读取的几何同时供显示及feed使用，值只留在调用栈并携带精确FrameKey/sequence/input时间/item等，显示后及插入消费前仍检查当前许可，不缓存或更新时间。这是减少重复工作的源码方向，尚无CPU/编译/游戏通过；不会混入Boltok未实跑的共享Scope优化，也不改共享Native/handclient/100ms/仲裁。

该窄修改随后通过抽取实际生产函数的46项检查（/W4/WX）：旧show/feed两份poses、新流程一份，枪/腕/插入端口几何与原结果一致；错frame/lifecycle/weapon/item/world/space/sequence/input时间、跟踪/持握差异、写后过期、不可见/坏port/失败pose或setter均拒绝，失败不输出旧值。Engine接口是fixture，不代表实时骨骼或游戏性能证明。初始capture仍原show和独立feed路径。完整编译首次因整份共享Snub visual已被peer迁移到新material接口、旧私有asset基线无该成员而失败；另发现局部observed遮蔽既有全局。保留原日志，改用准确旧私有源只backport本轮几何尾段/runtime两处，并更名feed_observation；共享peer接口和组合层未覆盖。最终精确1789源仅Snub runtime/visual变化，配置/完整PG编译0，PGc1c425f5793082fab2aa457319f369280cec3a291e9ee8dbfc5bce50e9c955f0。Native1149/XR1684与21helper复用，原charging进度观察保留；尚未stage/run，不称时效或套筒修复通过。

## 27132 实际行程／动画采样不符，观察前缀拒绝定点修正

Frag103500-v2/54784全终明确唯一交窗，核102805 prepared9029ba7f/3959归档源/1789编译输入/4制品/21helper及idle后stage。27132 / creation134386663928487486实际空枪新匣0/36→12/24、stage6→7及未上膛R2不射/不扣弹再次通过，不能仅据此称61796时效问题已因果解决。套筒真实token3/life5/frame1428成功begin/new_grant，1429及后续实际Decision retained/new_grant0/lease3、tracked/known/down成立，无charging recovery。手在枪局部从x=.0708723651到.00587325179，wanted达完整.0543574941m，full位置已发出/issued_full1；原部件actual稳在.0348558628m。真实release后phase waiting_open/full0，8秒完成等待超时。cleanup351发生于operator stop之后，不是首因；闭合/返射不通过。

5页120.966667秒H2641920×660全跨度配对总览与15原图亲审（game01257/01270/01282/01297/01311/01480/01491/01502/01513/01535/01572，sim01955/01976/02329/02372）。捕获前可见枪与黄套筒guide，真实拉放段左前臂遮盖大部分枪和套筒；掌/袖遮匣接触，完整fit和可见全行程/闭合不接受。startup第三人称/等待卡与旧拾取白HUD仍保留。runner74034/encoder77133/retention43002全terminal0，Native/entry/pad恢复TRUE/drain1/XR0/launcher0/game absent，config/settings原bytes/head原缺/wrapper精确恢复、21helper最终hash通过。retention删2793媒体477564558bytes，留27132/54784/45640最新3局48h，源/文/数值不删；Native21条来源记录及原status另存。独立idle/CIM后唯一交Longshot104905/133b986c/PGe9453009，其后Bol105222/Coverr29按真实尾维护。没有本窗追加case。

原1475bec4 stream认证的AR_Reload共60键/2s，最大pull在key47。九组“上一条已发出animation_time→当前实际部件”按59区间计算均与实际相差不足1微米，最大5.327855985e-7m；按60区间则可差约28mm。full请求time1.56666672按59区间对应key46.21666824，预测34.8558454mm，吻合实录；原distance_at_time却拒绝open_key47以前，导致已有备用law识别永远不能积累两次稳定匹配。证据27132-sampling-analysis.json，属于该局原getter/已发出时间的配对，不是所有Draw或头显证明。

定点新增observe_distance_at_time仅读原已认证解码键前缀，原distance_at_time和time_for_distance全文保持；Snub charging仅备用观测改调新函数。动作位置写入仍仅原最大pull至闭合的尾段；原实际两次稳定匹配、.8mm观察容差、1.5mm满行程、真实release/actual_full/两帧闭合/输入与所有权规则都保持。以实际生产adapter块、原60键和真实SnubSlideStroke执行28项检查，旧路径RED1复现不能切law，修正GREEN0；只有实际全行程骨回执才能从waiting_open进入returning，半拉回闭仍idle而非complete。无Engine执行，未称修复实跑通过；完整独立PG编译另记。

精确上版1789编译源中仅Snub charging与slide_curve两文件改变，完整配置/PG编译0：f854d4b9cefa3d76c22640c38da4e11b346d13435ad6107a0dc4911aef4aa367。冻结源28项红绿再次通过；Native1149/XR1684、同源magazine几何与21helper复用。后续仍一次empty-only，排到当前所有先序之后，未stage或实跑。不复跑不变旧计时器，不把CPU曲线匹配当完整闭合通过。

## 17548 空枪满行程、闭合和真实返射通过

Cover r31/30188全终最终唯一交窗后，核110649 prepared35fa0494/3971归档源/1789编译输入/4制品/21helper和idle，stage精确PGf854d4b9/Native1149/XR1684。17548 / creation134386685804124830，口袋/实际首发及必要射空通过，新匣stage1→2→3→4→5→6→7、真实0/36→12/24，未上膛R2不射/不扣弹。charging token3/life5首grant/retained成立，wanted完整.0543574941m；实际原部件初仍停.03485586m，两次稳定实际匹配后frame1441 sampling0→1，新的满行程动画时间1.59322035。frame1442 actual=.0543578415m/full1，真实release后phase returning，原部件回闭，frame1448/1449连续两帧actual约-3.4e-7m，complete/result2/stage7→8。真实同源返射12→11/reserve24/bonus0，pause/B resume通过。观察范围修正这条链在游戏中成立，不降低满行程/闭合/释放确认，不推广成所有输入过期根因。

snub_reload_sound bit1/2/4/8/16均applied1，说明原声音事件调用成功；受控测试静音，不证明听感/音量验收，也无触觉反馈实证。16条charging进度原样保留，完整原log和Native21条输入来源记录保存。原匣检查36696与战术新匣46708的有效证据保持独立，不把本empty-only谎报为重跑全部分支。

5页123.833333秒H2641920×660全跨度配对概览、15实际原图亲审（game01244/01256/01267/01281/01295/01464/01475/01487/01498/01520/01559，sim01970/01991/02346/02389）。掌/袖遮匣接触，拉放和返射段左前臂遮住大部分枪/slide，完整fit/可见全行程闭合不接受；startup第三人称/等待卡、旧拾取白HUD仍保留。数值/原事务/射击通过不替代可见手型或头显。runner83484/encoder91040/retention29199实际terminal0，Native/entry/pad恢复TRUE/drain1/XR0/launcher0/game absent，config/settings原bytes/head原缺/wrapper精确恢复、21helper最终hash全通过。retention删1995媒体337080558bytes，留17548/30188/47536最新3局48h，源/文/数值不删。独立idle/CIM后明确唯一交HB110627/5e9efb76/PG0cb1248a，无追加本窗case。最终回执reload-17548-review.json及restore/contacts/ffprobe/原Native status。

下一项是Snub专用主握把/弹匣/套筒皮肤手姿、可读反馈及中断条件的完整验证；不新增骨writer或控制/输入通路。纠正前述接口推断：实际1149与9839二进制均没有PartGraspV3，不能把当前共享源码当作已编入9839的证据。9839静态70项exports仅含旧V2。Lancer loading owner绑定不能冒充；一般part皮肤必须使用明确新增且准确编译的V3，不自动混其他武器改动。物理头显接受另记，整体目标未完成。


## Snub 专用皮肤候选：仅源码／CPU／编译，尚未游戏验证

在17548准确通过的PG源码上，只backport Snub physical-wrist选择、弹匣/套筒皮肤producer、原套筒getter同票观察值及两个可选调用；保留旧私有material接口，不覆盖共享peer迁移或recover_serial/service_unselected。V3存在时机械几何只接受精确stereo/input对应的committed physical_left_grip，经原Marcus115校准获得腕部，缺失即等待，不从已贴合骨骼回读、不重标旧时间。旧V2路径保持。实际生产选择器18项检查通过。

新的snub_grasp模块使用现有Marcus115 authored pad和现有圆柱／精捏手型：实际私有弹匣LocalToWorld及实际套筒getter位置为皮肤目标。只有当前真实magazine/ammo grant/retained、相同press/key/identity/current和完整原Frame才能提交V3一般part槽；不冒充Lancer loading owner。缓存只留成功发布后的圆柱轴符号，不缓存世界目标或权限。精确源码263项producer/尺度/旋转/拒绝检查通过；Native和Engine边界为fixture，不能当作手指表面贴合或运行性能证据。

独立Native以实际1149源码为基底，仅加入可选V3 export、preset与既有C0的一处shape选择。92个BOOL开关、旧V2 wire、原输入/committed frame和手指profile保持；静态67→68个exports仅新增GearsVrPublishPartGraspV3，原exports无删除。移除新增include/export和一处C0选择后全文等于原版。三项实际Native/手型CPU检查及完整编译通过，Native076e45841c921edb99f09cc7d90e7283a1d62769120e73c5123e71fc60fbeb38；没有新增骨writer/hook。PG完整配置/编译通过，ff7772880e8eb7e5bc87175766689bfbed0c8f9e6c8be46264dd90a5d8a24c85。准确1791项PG编译源和Native单独来源归档，不将它们谎称同一整树编译。

下一单段仍empty前置至真实套筒／回射，另通过原Source1头部路径在真实持匣和满拉段加入小幅侧观，双手按钮/机械规则保持；松手闭合后放低副手再观察回射。只有双路实际完整图像才能支持贴合接受。新候选尚未stage/run，公开v71未变、实体头显未验收。

## 56176 新候选在腕部准入前停止，未执行皮肤／套筒

Frag122000/28120全部终态后明确唯一交窗，独立idle/CIM与b4d7742a/3999归档源/4制品/21driver+AST核对后stage PGff777288/Native076e/XR1684。56176 / creation134386738664288820实际口袋取枪、首发和必要逐发射空0/36通过。原getter预接触腕部在现有capture范围内；初次真实Grip token1得到grant/status2/lease3、accepted1/current1，但snub_reload_capture_deferred gate21，8秒未见任何reload_transition而停。gate21是wrist(f,left)返回false；距离检查在gate24，不能称距离/半径拒绝。没有snub_skin、charging_progress或reload_complete，未执行持匣侧观、套筒、返射或本局pause/B；既有17548机械证据不撤销。

原Native计数interaction_publish0=982、read0=1，其余read拒绝计数为0，说明有一次到达原API最后safe_copy之前的分支；read0在最终复制之前递增，不能把它当作最终返回accepted或Playground成功的证明。尚不能从总计数证明Playground哪一项reader/matches/stereo/input/current/physical校准检查失败。下一只将已有短路判定分开，失败后最多12条状态改变POD，记录实际返回值和原始source时间、frame/sequence/stereo/identity；不添加Engine、rig、current或clock读取，不更新时间或放宽同票判断，不从已贴合动画骨回读。28项实际腕部选择及精确拒绝子门CPU检查通过，尚非游戏修复。

97.633333秒H2641920×660，4页完整双路概览和7张原图亲审（game01234/01247/01261/01275/01289，sim01931/01952）。枪与主手可见，副手到枪下但未拔匣，腕/掌/袖遮住接触，完整fit仍未接受；startup第三人称/等待卡及旧拾取HUD保留。runner工具会话92440=0、encode1032=0、唯一retention60559=0均实际终态，operator真实失败保留。Native/entry/pad恢复TRUE/drain1/XR0/launcher0/gamegone，config/settings原bytes/head原缺/wrapper精确恢复，21helper最终hash通过。保存Native22条来源及原capture/session status；retention删1486媒体244523987bytes、留56176/28120/34836最新3局48h，源码/文本/数值不删。独立idle/CIM后最终唯一交Coverr34/59770ded/70ee042c，后继由其沿真实尾维护，无追加本窗case、不抄统筹。完整reload-56176-review.json、restore、ffprobe及原log保留。

腕部子门观察随后完整PG编译0，19c9a54d8b0d1a716a862d08adb25dab38956980e66d7a606306130e8d576442。准确1791项编译源仅Snub visual改变，两个生产函数已有read/current/clock/physical/eligible/animated调用数量逐项相等；冻结源28+263项检查通过。Native076e/XR1684/runtime6aea/21driver不变。新snub-prepared-20261109-131050-wrist-observe，prepared97a3b1e38ee1ab4c06fac3b629f571886dac737a4a219d75590b772e111ba2b0，私有747518f6/ref refs/codex/snub-wrist-observe-20261109-131050，Git共享index/分支保持。下一仅inspection首抓取诊断，不重复射空；只有原准入自然成功才原匣拔回／守恒／返射。另排当前全部已受理项之后的新尾，未stage/run，不称修复通过。

## 46592 明确原物理观察与当前输入的帧差

HB125528/64316全终后明确唯一交窗，独立idle/CIM及131050准确归档／制品／21driver核对后运行短inspection。46592 / creation134386768744613245实际口袋／首发11/36通过，原匣stage1→2→3、extracted1及持匣侧观执行，但回插未complete，stage3→4后8秒等待超时，首败停；无本局返射、套筒或pause/B。至少一条snub_skin accepted状态回执成立，状态去重不能当发布调用总数，更不能当C0应用或皮肤证明。

唯一wrist_rejected gate3：当前f.frame514/seq2438，实际Native parent513/seq2437，武器／lifecycle／reference相同、modelscale1、units100；原Native read及matches已通过，随后新增exact-stereo相等才拒绝。原物理sync比当前许可输入早21.1923ms，原物理年龄81.4949ms、当前许可输入60.3026ms，都保持原时戳。本记录证明一处物理读取被过强的输入票相等要求拒绝，尚非回插失败全部因果证明，不将其写成已修复。

56.2秒H2641920×660，2页完整配对概览与13张原图亲审（game00614/00627/00639/00652/00666/00651/00665/00678，sim00916/00937/00936/00957/00979）。枪／主手与侧观可见，掌袖／前臂仍遮住弹匣接触；部分接触区域低于原水平视野，不接受完整fit。startup第三人称／等待卡、旧拾取和白色reload HUD保留。runner会话29446=0、encode34851=0、唯一retention84237=0全实际终态，operator真实失败保留。Native/entry/padTRUE/drain1/XR0/launcher0/gamegone，config/settings原bytes/head原缺/wrapper精确恢复，21helper最终hash通过；Native20条来源和原capture/session状态保存。retention删2272媒体391047775bytes、留46592/64316/9816最新3局48h，源文数值不删。独立idle/CIM后最终唯一交Frag131651/cead6c2e/c0a6e1e9，由其沿真实尾Longshot133536/Coverr39维护，无追加本窗case、不抄统筹。

## 原物理观察归属修正：当前仅CPU／完整编译

新的WristObservation只在调用栈保留当前Frame的精确归属和原Native物理观察，绝不储存或续租Decision。V3机械数据使用原fixed-reach physical_left_grip及原观察sequence/sync；当前Native Frame、Decision、身份／输入／持握许可继续独立校验。原Native matches/current_result的100ms保留，并在部件显示后和插入消费前用原时戳再次检查；当前许可仍新鲜也不能更新已过期物理观察。未来样本、跨Frame／身份／epoch／reference、先前控制按下／释放计数均拒绝，渲染后拟合left_wrist不参与机械计算。旧V2严格路径及animated fallback仍保持。

新握持若当前粗接触已成立，但真实物理观察尚未进入同一Grip/index控制计数，仅发布既有magazine_observation_pending否决候选；不在观察回调内调用HandClient current，不发新租约或捏造incumbent。复用原LeftHandInteractionOwner的同一未派发press100ms窗口；100ms+1ns即使新姿势到了也不能续机会，仍需真实release。这里不复用先前press的物理动作。

Feed和套筒样本现在带原物理sequence/time，真实当前Grip token仅标识仍有效的当前机械租约；重复同一物理样本不重置插入接近状态，也不制造新速度／行程。原套筒tick、采样law、最大速度／间隔、满行程／真实release／两帧实际闭合和库存事务规则不改。可选皮肤发布放到机械消费之后，避免占用原观察剩余时效；插入／完成时优先冻结原事务。测试侧观保留原路径，仅增加约14度向下看、取消原4cm抬头，帮助接触区域进入视野，不改碰撞／抓取几何。

用实际Native endpoint/matches/current_result、实际Snub选择器／proposal helper和真实LeftHandInteractionOwner执行旧路径RED1→新路径62项GREEN0；涵盖81.4949ms原样本、按下前姿势拒绝、原100ms等待／超时、当前许可仍新鲜时原观察过期、未来／跨上下文数据和旧V2。实际feed／charging adapter加真实FeedInsertionStroke/SnubSlideStroke另43项通过：原stamps、一份几何、重复不重置接近、显示后到期拒绝、真实原时差运动，手势全程不等于原部件full ACK。实际skin producer原263项仍通过。Native/Engine读者边界是fixture，不代表游戏／像素通过。完整PG编译0：07dc2a27cdfac80204b91a34f1b60b9c25d4c5cdde94c03f6488be79d3f8f441；准确1791源仅四个Snub模块改变，Native076e/XR1684/runtime6aea复用，公共v71未变。初次编译发现observed/pending局部名遮蔽工程全局名；observed原失败日志和pending失败工具回执分别保留（后者完整临时日志已被成功重编覆盖），/W4/WX未降低。下一一次empty段验证受影响的弹匣和套筒原时间数据流，不重复战术／原匣矩阵；未stage/run，不称修复通过。

新冻结snub-prepared-20261109-144724-physical-observation，prepared027d28d6140751651b410cabb9c1cd541cb077404991badc3e5caf148868acbb，源码ab37a80e/ref refs/codex/snub-physical-observation-20261109-144724，共享index／分支保持。4043归档项／4保存制品／21driver，唯一driver变化为原侧观向下看。Frag145754已直接独立复核并接受本候选为其唯一后继，旧Frag144552未运行候选已撤销；仍等待前序全部收尾后的明确唯一交窗，未stage/run。Snub之后现唯一受理已独立核验的Bol153322-motion-dispatch（prepared5c2b2820／PG7fda6e06／plan8d90663b），核验记录successor-bol153322-check.json；尚未最终交窗。新Coverr44及尚未冻结的Lancer原版手姿请求已指向真实新尾Boltok，由其维护后续，不在Snub登记第二后继。


## 71348 新匣入座与欠上膛阻断通过，原输入丢失边界取消事务

Frag145754/60288全终明确唯一交窗后，独立核144724 prepared027d28d6/4043归档/4bin/21helper+AST/1791PG+1138Native源码快照、idle/CIM均通过。71348 / creation134386841198344413真实口袋／首发和必要射空0/36通过，原匣1→2→3→4、新匣4→5→6→7、库存12/24/bonus0及未上膛真实R2不射不扣弹成立。随后stage7发生life6→7，其余hold/world/session/space/input/registration及对象相同，driver按身份边界首败停止；未完成本局套筒／返射／pause，不称新版本机械整链或皮肤通过。

原Native26条输入丢失记录中，25条属于startup seq1035过期；唯一动作期记录reason8、bridge_contended1、observed1018808619309500，原read零等待mutex竞争返回false。原capture据此input_lost、broker首次失效递增lifecycle；新baseline5352/sync1018808625142300在其后5.8328ms，PG同时disarm257，后续life7。此对应链支持本次取消源于桥读竞争，尚未测得持锁者／实际持续时间，不把它称为控制器真的过期或修改身份保护。证据71348-lifecycle-input-loss.json与原状态／冻结源码。

109.1秒H2641920×660、4页全跨度双路概览及20原图亲审：下视侧观真实发生，但掌袖／前臂仍遮弹匣接触和部分主握把，完整fit不接受；黄色上膛接触提示／白reload bar、原pickup/startupTPS/wait保留。runner99513=0（operator失败）、encode94559=0、唯一retention60522=0实际终态；Native/entry/padTRUE/drain1/XR0/launcher0/gamegone，PG统一cleanup fire_disable0/hand_stop5/drainedtrue，config原bytes/settingsSHA/head原缺/wrapper／21helper最终hash通过。Native状态已独立保存；retention删509媒体66553309bytes，留71348/60288/62728最新3局48h，源文数值保留。独立idle/CIM后最终唯一交Bol153322/5c2b2820/7fda6e06，由其维护后尾；无本窗追加。完整reload-71348-review.json。


## 原桥同步窄修正已编译封存，尚无新游戏结果

在准确Native076e源上只改stereo_bridge.h和capture.cpp帧构造的一处读取：新read_after_writer请求等待现有mutex 1ms，取得完整packet后仍用原packet/source/PID/frequency验证和原post-read clock／100ms时效门。旧read默认零等待、publish和wire布局不改；等待超时、abandoned、失败仍拒绝并沿原input_lost／lifecycle取消。不借先前packet，不更新时间，不跨life接续事务；共享Root Native／公开v71未编辑。

真实生产桥／gate旧路径RED1，新路径62项GREEN0（/W4/WX）；短写者调度边界注入，实际私有PID的Win32 mapping/mutex用于传输，另用真实线程长期持锁验证请求超时和恢复。Windows调度可超出请求1ms，host实际测5.1952ms，不能说严格1ms执行上限，原时效门必须在等待后重新判定。测试初稿把future样本的拒绝分类预期成invalid；原实现实际为stale，仅修预期，生产规则不动。原stereo_frame/gate/matrix/bridge完整程序另0，含64世界边界检查。

完整Native配置／编译0，DLL61991090a8f0edaab303f0cb893c7224c1e3d8b21a448e630099481dffe01bc8；68exports／92BOOL不变，移除这处选择后capture全文等于076e，桥读取核心除等待参数外全文相等，其他源不变。PG07dc、Source1／21driver、XR1684／runtime6aea复用，无不变PG重编。私有Git da79fbb0144b2df432b3c2baa4a3d2072086ef3f / refs/codex/snub-bridge-read-20261109-161628，共享index／分支不动。

新冻结snub-prepared-20261109-161628-bridge-read，prepared730534146bba585e6476025679aa4fdf74cc87ad2ee29164c6e002fe71e2fd60，4065归档／4saved bins／21helper+AST、1791PG／1138Native冻结源码快照全核0。ready9d47e1db、plan dc7c332b、inputsfc88ea0d记录于bridge-read-runtime-*.json；check-bridge-read-prepared.py只读不调用Game、不部署，run-bridge-read.py要求新的真实前驱全终最终唯一交窗。已向旧窗唯一收者Boltok请求现有全部项目之后的新尾，由真实尾核验；无继承旧窗、未stage/run。下一只跑原empty单段，首严失败停，不重复战术／原匣矩阵，不称同步修复／完整手姿／头显通过。


## 2026-11-09 追加吸附／逐部件GrabPose合同的Snub落实清单

按docs/weapon-development-20261109/grasp-and-snap.md及common.md，逐件皮肤接受需要掌面、拇指、四指、食指、腕、袖口及进入／操作／释放过渡的实际双眼画面；现有generic cylinder／precision只是可复用的authoring基础，不能作为Snub专用手型完成。单一C0、公共仲裁、物理腕与皮肤目标分离继续保持。当前161628同步候选保持冻结，不把后续手型工作混入其待验身份。

| Snub实际抓点 | 手与职责 | 当前生产入口／缺口 | 画面／吸附接受 |
| --- | --- | --- | --- |
| 主握把及扳机指 | 右手点按保持枪，食指随真实R2，其余三指及拇指包握实际握把，腕与袖口避枪体 | 既有持枪唯一C0／通用weapon手型；尚需Snub独立握把表面／逐指标定，不能将通用闭拳当接受 | 主握可见但完整表面／袖口未接受；不因拿枪成功而通过 |
| 副手托握 | 左手接触主手／握把前侧，真实新Grip才取得独立support；退出后右手继续控枪 | 尚未定义Snub专用支撑区、方向和逐指姿势；不得用弹匣或套筒租约冒充support | 未实现／未接受，不让托握抢已持弹匣 |
| 旧匣／肩部新匣／回插 | 左手掌贴实际匣表面，拇指与四指围绕直径，食指可参与包握；同一件跨拔出／持有／入座过渡 | snub_grasp::magazine，原实际组件矩阵／几何centre+axis及V3 cylinder基础；需Snub逐件表面和指型标定、稳定Snap获取／保持／退出 | 71348原匣取出及新匣入座通过；掌袖遮接触，皮肤和被动Snap未接受 |
| 原套筒抓点及联动翼片 | 左手拇指／食指夹持真实滑套接触面，其余三指避开枪身，腕随滑套行程；翼片随原机构归组 | snub_grasp::slide，原actual部件位置和V3 precision基础；不能从被拟合左腕读机械行程 | 历史17548原机械满行程／释放／闭合通过；新V3下套筒画面尚未执行，逐指及Snap未接受 |

只为实际可操作件定义交互。Snub使用弹匣，不为独立散装子弹凭空增加操作；当前无独立可抓弹匣卡扣／枪口／联动翼片权限，未来若新增可操作件必须同步加入对应GrabPose，不能借本清单虚报已有功能。被动Snap不得生成新按下、租约、插入行程或上膛；真实持件优先于邻近支撑区。参数按原模型自行标定，无需逐项问主人。


71348证据限定补充：empty-inserted读回ns1018808477502400，原输入丢失ns1018808619309500，empty-blocked读回ns1018810264569800；丢失落在两次读回之间。driver真实R2脉冲后无新弹道／无扣弹是观察事实，原operator检查true保留，但不能将本局无射击单独归因于欠上膛守卫；17548的独立机械／欠上膛证据保持。后续当前同步候选必须在有效输入下重新取得受影响段证据。161628冻结归档中的先前观察记录不改字节，本补充及更新后的71348-lifecycle-input-loss.json约束其解释，不冒充新一次运行。


161628排队确认：Frag161511作为LS160159已受理后继，直接独立全核4065源／4bin／21helper+AST／1791PG+1138Native源码快照及prepared73053414／plan dc7c332b／ready9d47e1db／inputsfc88ea0d后，正式唯一受理Snub161628为其后继；记录analysis/frag-grip-20261108/stop-trace-successor-Snub161628-check.json。当前链HB153818→LS160159→Frag161511→Snub161628，仅排队非授窗，未stage/run。Snub随后全核并唯一受理Bol162411（prepared0afa4c37／PG75510226／plan19256458／source79495b68），successor-bol162411-check.json；不存在第二后继或自循环。仍需前驱所有运行／原准备实际drain结果／恢复／亲审／encode／retention终态后的最终明确交窗，不能据idle或登记启动。


## 31420 射空前缺弹道，未触及换弹／套筒

Frag161511/35344全部收尾后明确唯一交窗，独立核161628 prepared73053414／4065归档／4bin／21helper+AST／1791PG+1138Native源码快照及idle/CIM，运行31420 creation134386896475608583。取枪和首发、准备射击0..7实际弹道成立；empty_preparation_fire_8缺实际trajectory即首败停止。没有射空、reload_transition、套筒、返射或本局pause，不称同步修复或新整链通过。8条Native loss都为startup seq1106过期，动作期无bridge_contended记录；此负观察不证明原mutex原因已因果解决。射击尾部有copied_input_age101.1796／105.6066／103.5524ms和preparation_budget0，真实新press10首个可见摘要已released10；需对齐实际输入／生产阶段，不能补发或扩大100ms。

76.4秒H2641920×660，3完整双路概览和8实际原图亲审：主枪／手可见，部分game画面短时缺手而sim仍有，startupTPS/wait/pickupHUD保留；未进入部件操作，完整fit不接受。runner53568=0（operator失败）、encode31839=0、唯一retention24668=0全实际终态。Native/entry/pad恢复，PGfire_disable0/hand_stop5/drainedtrue，XR0/launcher0/gamegone。原输入恢复因idle门暂缓，所有作业结束后独立idle/CIM，再按原SHA匹配settings含BOM的精确bytes／head原缺恢复；wrapper config原bytes及21helper结束hash通过。首次手工预检未含BOM的候选hash不符，在任何写入前拒绝，之后匹配原SHA唯一bytes完成；没有以强杀替代。retention删362媒体37793855bytes，留31420/35344/17072最新3局48h，源文数值不删。原Native状态、owned-done、restore／ffprobe及reload-31420-review.json独立保存。最终明确唯一交Bol162411／0afa4c37，后尾由其维护，无本窗追加；下一只离线查首败。


## 31420之后：空闲外枪探测窄调度已编译，尚未封存／实跑

准确PG07dc源中lancerreload_runtime::tick先调用HB完整tick，才进入Snub；HB idle但手枪持有时仍先copy完整Frame／game再拒绝。不将此直接当作31420唯一耗时原因。私有idle-route-source仅让持枪类型作空闲调度提示；有旧工作仍调用原消费者并保留原优先顺序，选中消费者仍完整fresh Frame／身份／租约准入；无新输入、时效延长、机械或库存规则变化。没有改共享Root或当前他人运行区。

实际生产调度前缀旧RED1→新50项GREEN0（/W4/WX），model/class/消费者边界注入，不代表游戏或性能证明。初次完整编译因组合点include顺序尚看不到HB/Snub私有stage类型而失败；改用已声明的只读pending_work()及两个精确stage!=idle getter，原动作体不改。fixture曾有selected全局名与生产局部名遮蔽，仅改fixture命名；/WX保留。失败编译日志／receipt分别保存。最终4文件（playground.cpp前置声明、HB/Snub getter、Lancer组合点）完整PG编译0，DLL14355de4d57bdf307158be730919c924e5e82d904438baff391ccc579d909160；Native6199／XR1684与Source1沿用，不重编不变部分。当前仅源码／CPU／编译，尚未新封存或申请新尾，不据31420旧窗或idle运行，完整Snap／逐件手型／头显仍待完成。


174315封存：snub-prepared-20261109-174315-idle-route，prepared2db231f1292ba61747bc323c188686ed3791327d75da788abe42c02b7f79136b，PG14355de4／Native61991090／XR1684／runtime6aea；源码d5f58f10a0511ec3984f8071c2e961456b6f446e / refs/codex/snub-idle-route-20261109-174315，共享index／分支保持。4083归档／4savedbins／21直接driver+AST／1791PG+1138Native冻结快照全核；prepared/plan另显式pin当前weapon_part_cache.py一个runtime_support哈希，不能称完整Python环境封存。checker增加saved DLL必须对应真实编译receipt的验证。最初174239错误选旧PG07dc，在任何请求／stage／run前发现并撤销；原错误候选／Git记录保留，禁止部署，不伪称其通过。新174315独立校验实际PG14355de4通过，尚未stage/run。

plan idle-route-runtime-plan.json ab545fbf1e37e8dc33d9b272bda7cca55643da081f8ab5e2782cec62804f8871；ready idle-route-runtime-ready.json de0a6cafb7db9a3f64c34af4f7812ad7217264c8a9ebe978cac40ff4923242b4；inputsfc88ea0d直接helper保持，check-idle-route-prepared.py只读、不调用Game。run-idle-route.py要求新的全终最终唯一交窗receipt。已向31420最终唯一收窗者Boltok申请全部更早项目之后的真实新尾，未继承旧窗、尚未直接受理，Snub无已受理新后继。scope仍empty单段原动作／首败停，完整fit／Snap／头显未完成。


排队期间独立的手型准备：原私有Snub弹匣缓存279顶点／314三角，vertex87f757bb与indexdd54ab47按原source.json全SHA核过，生成snub-magazine-geometry-authoring.png三个枪局部正交视图并亲审。原几何是薄的多边形板状轮廓；PCA包围跨度约12.29×10.30×3.14cm，PCA不是握轴。实际三角167／197宽面交点位于x约±1.499cm，y22.956cm／z0.583cm，法向约±X，带原三角索引／重心坐标保存在snub-magazine-surface-candidates.json。它们只供下一阶段掌／拇指表面位置标定，不推导腕／逐指／吸附范围，不改变机械接触／已排174315；无手型或像素接受。generic cylinder／precision只能作为手型基础，不能替代Snub逐部件标定。

174315同一申请路由：Bol已全终交Coverr47后指向其；Cover按唯一Bow164457继续单次转送；Bow明确其已受理唯一后继为Lancer165800，现已向Lancer核实际尾，仍未有真实前驱直接受理，不依据转述或idle部署。


Snub原手型离线基础已读回核验：snub_rig_authoring.py仅读取已有Marcus115原导出SHA cc2db56c及原参考canonical3b3c978c，解码22338个LOD0顶点／每chunk BoneMap与四权重／115参考骨，不调用Game。左右真实腕索引11/35、原Weapon_R_End52，两手各1395个全手部三角。固定参考FK再逆绑的独立skin identity残差5.6843e-14，权重归一检查通过；原平移／骨长不变。snub-rig-authoring-check.json保存来源和边界。此为后续专用GrabPose的蒙皮几何基础，不把参考臂／袖当当前C0 IK，不把步枪动画或握点当Snub姿势，也不代表双眼／头显接受；174315冻结身份不动。

174315继续沿既有链：Lancer直接确认其后继HB171233；HB直接确认Boom173042（最新plan d0df138f，旧6376作废）；Boom直接确认HODcore2-aim且其后有LS172406。更早请求原序Frag172809／Gnasher173101／Troika174900由实际尾确认，现向HOD核已确知真实尾，不自造第二槽／不插位／不授窗。尚无直接受理。


## 弹匣通用圆柱握型的离线相交基线（非当前C0画面）

inspect-snub-magazine-cylinder-baseline.py以71348已保存pregrasp gun/body getter方向、原115蒙皮及Native6199准确finger header02c96c0b，按现有cylinder最短轴对齐／10UE anchor／弹匣AABB centre重建满权重静态预测，生成三向图并亲审。手1395三角／原匣314三角，经2967个AABB候选对，627对存在非共面交线，其中414对交线跨度>1mm，最大1.6518UE（16.52mm交线长度，不是穿入深度）；三角对计数不能当独立穿模区域数。独立几何核5个解析／刚体变换／缩放例检查通过，点／共面／退化情形显式另记，尚未覆盖纯内部包含、厚度或全部接触标准。

这份基线支持继续标定专用Snub手型，不能当实际游戏穿模因果结论：pregrasp body getter方向只近似unsnapped物理腕，缺少同票Native physical／实际blend／动态C0 IK，参考臂袖不代表当前画面。保存snub-magazine-cylinder-baseline.json／png与snub_skin_geometry.py，运行区和已排174315均不变，不用skinned wrist反推机械行程，不靠改抓取半径掩盖几何。

实际新尾：Frag通知正在按旧Gnasher先序纠正为Frag172809→Gnasher173101→Troika174900；随后Troika直接确认自己是Gnasher唯一正式后继且暂未接后继。已向Troika明确174315未在别处受理，提交同一完整prepared／plan／ready／inputs申请，待其独立核接；没有运行授权。


Troika174900现已直接独立全核prepared2db231f1／planab545fbf／readyde0a6caf／inputsfc88ea0d、4083源／4bin／21current helper+1support／1791PG+1138Native保存源码快照及1274归档／当前Python AST，正式接受Snub174315为唯一后继，记录analysis/troika-development-20261109/successor-snub174315-check.json。既有非法转义SyntaxWarning不是语法失败。前驱明确仍等Gnasher173101最终交窗，尚未stage/run；本Snub仍未stage/run，队列受理不是授权。须其自身原scope／所有job／双路亲审／精确恢复／encode／retention全终、重核同一身份并最终唯一交窗后，Snub独立预检／idle才部署。Snub当前无已受理新后继，后到冻结申请沿这个实际尾单一维护。

## 排队期间的独立抓姿准备与后继核验（未进入174315）

Coverr54-array-witness现为Snub174315之后唯一正式受理后继：prepared6ec774ef／plan915215be／Native5cf4adea／PGf8e6f3c2／XR1684，源码9aeb97e0。check-successor-coverr54.py独立核过24声明Native源（另3项变更manifest按前缀规范化对应）、37当前与37归档driver／37AST／3保存bin、EXE／PhysX／reviewPython身份，successor-coverr54-check.json记录0。其范围是原A和同一闭合查询的只读array rejection记录、0新RPM，原控制／写入许可不扩。Cover已确认等Snub自身最终全终唯一交窗，不提前stage/run；后来Bol180947-main-grasp按先序指向Cover维护，不是Snub第二后继。Troika174900仍是Snub唯一前驱。

author-snub-magazine-pose.py对固定旧腕方向和有限手指／thumb／平移组合做24786个纯静态试型，8种种子用原手1395／匣314三角及精确顶点到三角距离复核，全部拒绝。一个试型没有>1mm交线，但最近3顶点均值仍有食指11.09mm／小指15.15mm／拇指7.29mm间隙；零交线不等于握稳。snub-magazine-pose-authoring.json保存实际距离／交线与所有拒绝状态，snub-magazine-rejected-trial.png亲审。距离核新增解析点面、刚体不变及退化例，snub_skin_geometry.selfcheck现8例；先前基线报告5例为当时真实结果，未重写历史。

新离线工具另经实际冻结Native头文件独立对照：rig-production-oracle.cpp直接调用hand_finger_pose.h rotation和cover_skeleton_ik.h forward，/W4/WX编译与运行0；参考、双手starter、双手free三组各115骨／22338顶点，Python列向量矩阵与实际C++行向量转置后最大矩阵差1.25043e-5、最大顶点差1.35987e-5 UE（约0.000136mm），双手分别记录。骨平移／scale保持，rig-production-oracle-check.json保留头文件SHA和边界。此只验证离线FK约定与生产函数相符，不是实际GPU／C0 IK／袖口／皮肤接受，也不单独验证Python蒙皮公式与游戏硬件一致。

旧圆柱方向的掌面转向是待检假设，不是已证原因。author-snub-magazine-palmar-frame.py新增±90度腕向、六种手型、两侧掌面共4200个静态组合，24个短名单精确复核；最小距离总和的box_thumb_flexed/+90试型虽掌部最近3顶点均值0.185mm，但四指约12.72／15.21／13.10／24.45mm、thumb10.88mm，且20对>1mm交线，仍拒绝。snub-magazine-palmar-trial.png三向亲审；不把腕转向或扩大接触半径当修复。

原始角色动画新增准确手枪组索引original-marcus-pistol-animation-index.json：AnimSetMarcus_CamSkel_Pistol，130序列／75轨，所有导出按原inventory SHA核；组内仍使用AR前缀名称，不能按序列名前缀认作步枪。read-snub-original-hand-pose.py只取本组AR_Idle_Ready_Aim第一key，单key手指Float96、其余多keyFloat96原24字节range header与跨度／范围均核；非root W转换沿既有原格式约定，保留固定参考平移。原RightHand→Weapon_R_End socket导出0ba933a9及Snub原mesh1f1c5a14全SHA独立核过，原Snub／WarWeapon／WeaponMesh默认属性也只读保存。该原始握姿按直接socket映射绘图后，主手明显偏在握把外侧，snub-original-pistol-right-hand.png亲审，仍未接受；默认属性未见显式旋转／平移覆盖，不能据此证明完整原运行附着链没有额外变换。第一key动画平移相对参考最大67.27UE被明确忽略以保持骨长，故这也不是原stock完整姿态重放。下一步应核实际同帧右腕—枪component／原附件变换，不能为了图面贴合猜90度补偿或把原姿势直接部署。

以上均在独立analysis文件内，未改变冻结174315／Native6199／公共v71，未启动Game／XR／模拟器。新候选运行、逐件Snap／GrabPose、完整画面与物理头显仍未完成。

随后复用17548／27104的已保存current_moved_gun_slide-body/gun，不新增采样：各局四个body样本右腕相对各自gun component的关系，与直接原socket假设约差90.00003度、位置差3.2847UE；各四帧body位置变化分别6.4e-5／1.7e-4UE。gun读比body分别早约1.879秒／18.745秒，明确不是原子同帧，不据此证明完整stock附件链有错。snub-saved-right-wrist-comparison.json记录原时间和SHA。用17548保存的腕映射与原手枪完整15局部指骨第一key渲染，主手在三向图能包围握把；这仍是离线组合假设，非测得的游戏手指或新像素接受。snub-original-pistol-saved-wrist.png亲审。

进一步把原手枪指骨旋转投影到现有单轴flex／双轴thumb参数：左右最大顶点偏差11.02／26.47mm，snub-original-finger-axis-projection.json记录，不能仅靠替换12个弯曲角表达完整参考姿态。当前Root已存在通用hand_authored_grasp.h V4和同一C0 authored消费，故本Snub不新增API或writer。analysis/snub-20261109/main-grasp-source/weapon_snub_grasp_pose.h是独立本枪主手draft（SHA2d85b267）：原Pistol AnimSet导出a153fbbe完整thumb／三指，rest食指独立8／8／5度，trigger端点只变index前三骨；严格Marcus115 reference/signature、原Frame原样携带、identity grip，只有局部显示数据，无grant／库存／腕位。

snub-main-pose-draft-test.cpp直接用现有V4 structural/reference_matches/rotations执行51项定向检查，/W4/WX编译与host运行0；错误reference／signature、非index trigger变化、越界trigger拒绝。首稿误要求归一化混合端点byte相等而exit3，改成四元数等价，仅测试修正，保留first-failed.log；另首次cmd路径正斜杠被cmd解析为analysis命令失败，未编译，receipt明确保留。snub-main-pose-draft-check.json只pin四个直接公共头，不冒称完整编译依赖封存。

rest／trigger三向图均亲审，但精确几何复核仍拒绝：保存17548腕映射下，generic weapon rest有376对>1mm交线，draft rest357／trigger342对，最大交线约10.14／9.21mm；这些是三角对和交线长度，不是穿入深度或改善验收。各手区最邻近顶点距离很小也不能排除穿模。snub-main-pose-draft-geometry.json保存实际全枪3045三角对手1395三角结果。原始参考只是起点，主手draft未集成／未接受，排队Native6199没有V4导出；不得把该draft偷换进174315，后续需独立原生制品和新冻结／新尾。

## 原导入变换补全与弹匣专用姿势草稿（后续离线工作）

主手草稿／拒绝试型与工具已先保存Git a620c08283f63c0f00ce02e4dcdf5fc1df0dceb9，ref refs/codex/snub-main-pose-draft-20261109-191011，父d5f58f10；共享HEAD／index保持。后续按手区分类，draft rest中食指111对>1mm交线（319个含混合腕权重的index相关三角则122对），仅改index MCP局部Y/X的21个试型最少37对，但手指下垂至护圈下方，亲审拒绝。按枪局部向前／上／外侧最短旋转的12个方向试型最少43对；姿势方向更合理，但仍未接受。author-snub-rest-index-clearance.py与author-snub-index-surface-direction.py均只改离线样本，未改已保存主手header、其他14关节／平移／候选。

此前直接socket重建漏了一层真实导入变换，现已修正离线工具而非运行版：原Snub mesh导出1f1c5a14在bounds offset286／1个material之后，原生MeshOrigin=(0,0,0)、RotOrigin=(0,-16384,0)，不是默认属性标签。参考[固定revision UEViewer原格式](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMesh3.cpp#L1943)，沿现有Native model_to_component_rotation约定对model→socket应用一次−90°yaw。原先“直接socket姿势偏握把外”的图是离线重建缺项，不能作为原运行附件链错误证据。

read-snub-original-hand-pose.py现用原RightHand socket+原mesh Origin/RotOrigin独立重建，修正后主手图snub-original-pistol-origin-corrected.png亲审，显示包住握把，与以前用保存腕位拼出的参考一致。对17548／27104保存数据，位置误差分别8.3147e-5／1.3231e-4UE（约0.00083／0.00132mm）、经微小float非正交SVD归一后的方向误差3.9514e-5／4.6214e-5度；这些数据仍有原1.879／18.745秒读间隔，不升级成同帧C0／像素证明。初次用非正交原float trace直接算角得到0.023度，已改为带1e-4 scale残差限制的proper rotation提取，原矩阵最大差仍单独保存，不隐去数值边界。之前JSON／NPZ保存在before-origin文件，原错误图和a620检查点保留。运行候选／原Native转换逻辑未修改。

另从原Pistol角色AR_Reload（60帧／2秒）与原Snub枪AR_Reload（60帧／2秒）解码换弹参考。角色75轨与枪5轨逐一核原导出SHA；single rotation使用Float96，角色multi Fixed48按[原固定点格式](https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMeshTypes.h#L136)，实际多key块均精确24-byte旧range header+60个key，translation精确1／60个Vector，无追加time array；5个解析／正反向／截断／非法norm检查通过。角色全部参考平移保持，枪的真实原动画translation用于显示该匣。相同离线key index只是原作者参考，不能声称实际游戏两个clip已同步。

decode-snub-reload-authoring.py全部60帧额外验证“part局部腕位+15局部finger q”可独立复现原全手1395三角皮肤，最大重建差<1e-9UE，不需要重放身体／手臂。第18帧各手区最近3顶点均值最远1.79mm、19帧2.06mm，但仍178／186对>1mm交线；22帧最远4.05mm、75对交线。18／22两张三向图亲审，只作为比通用圆柱更适合的原几何起点，仍拒绝完整fit；不得按最小距离自动接受。

独立新增analysis/snub-20261109/main-grasp-source/weapon_snub_magazine_grasp_pose.h弹匣draft，选原22帧、独立profile 0x534e554200000002：固定原part-local wrist+15指骨，原匣／补充匣同实际形状共用这一部件定义，与main/slide分开。纯函数从当前part pose转换腕位，再通过现有Marcus115 left校准反解V4 grip；模型scale只应用一次，原Frame／sequence／sync原样携带。proximity仅格式化显示请求，Native仍需独立socket admission，当前没有被动Snap状态机、抓取许可或机械动作接线。

snub-magazine-pose-draft-test.cpp在实际V4 formatter/rotations与Native wrist calibration上编译／host运行0（/W4/WX），初93项后加入独立+90°Z解析坐标/scale-once oracle，共111项通过，覆盖不同模型scale／tracking units、held/proximity格式、错误scale/units/quaternion以及失败输出不变。不是Engine/Broker/C0集成测试或游戏通过。snub-magazine-pose-draft-check.json保留4个直接公共头SHA和真实范围。主手原51项不因未改header而重复运行；两个draft均未进入174315，Native6199仍不含V4。

窗口仍等Troika174900全终最终唯一交接。更晚HB185428及Boom191450均已直接告知其owner去Snub唯一后继Coverr54维护真实后尾，未在Snub受理第二位，未替其核新候选或授窗。

## 专用姿势的隔离 Native 接入已编译，尚未接 Playground／运行

以上导入坐标与magazine draft源码另保存2ed9ddf5e2ea54e8844ba6178a723869117b5f60 / refs/codex/snub-main-pose-draft-20261109-193512，父a620c082，共享HEAD/index均保持。随后在准确Native6199私有源码上移入现有通用V4，保留原单C0消费，不改Root共享Native，也不导入Root mounted／其他枪功能。

新隔离源native-authored-grasp-source共1625源快照项（含configure/test输入，不是实际MSVC依赖或1625个编译单元）。14项delta包含通用hand_authored_grasp／live／owner policy、对应测试、原hand_finger_pose的authored可选分支、内部Target/Blend的局部手指数据、原body_c0_hands单写入点、capture少量include/export、原C0调用段和已闭合Scene ClassPath的缓存分类。Scene分类仅从已经读入的Identity.name区分实际Lancer，原C/E owner检查保留；没有新RPM或原游戏调用。V4消费仍需Broker、当前Core/原输入100ms、原按下释放计数、真实模型reference/signature与最终C0许可，不更新时间、不跨lifecycle。

内部Preset扩展后，对旧publish_value/publish_part显式保留原0/1/2范围，避免旧V2/V3因新内部enum而扩大接收。原V2/V3 wire大小1496/1512保持，Shape旧数字与所有原designer角度／算式可逆回原文件；V4 only传局部显示旋转，不包含机械许可。原bridge6199/read_after_writer源全SHA保持，post-read clock／100ms／失败取消仍原样。

实际定向结果：wire41、Native adapter49（Broker／Core／extra-owner为fixture）、owner policy44、真实closing Scene reader fixture717、原C0几何程序7827均0。另新增native-authored-c0-consumer-test.cpp，将本Snub main rest/trigger和magazine V4数据经真实rotations函数送入真实body_c0_hands::detail::fingers，以原115 reference执行711项0：15指骨输出等价、所有平移/scale不变、其余骨逐byte保持、实际LOD跳过骨不被写、无authored数据拒绝。未把这个私有buffer消费或fixture当实际Native写权限／GPU／游戏通过。

Native完整configure0/build0，工具会话77082已实际终态0，新DLL analysis/snub-20261109/build-native-authored-grasp/Release/GearsCaptureSnubAuthoredGrasp.dll SHA c89dff3e7029c723282f5d5994cf1b2ca3f89fb96914ba08fb12b31d7a212d15。check-native-authored-preservation.py实际dumpbin确认原68 exports全保留，仅新增GearsVrPublishAuthoredGraspV4；92显式BOOL与实际CMakeCache一致，MAIN_GRIP_AB仍OFF。capture逆去新增include/export即原6199；C0调用段之外精确相同；所有旧finger profiles精确回原。未stage/install/load，没有新runtime候选、公共v71/manual入口未动。编译原有shadow warnings保留，不称零warning，host /W4/WX未降低。

失败记录：首次准备工具把旧C0段误数7行，实际准确baseline为6行，assert在移入该段／编译前停止，first-incomplete目录与JSON保留；仅修工具行数守卫。初host Scene编译漏MinHook include，诊断输出又触发Python默认gbk解码失败；补正确原vendor include并保存原始bytes日志，wire/adapter/owner先前成功结果复用。新增C0测试初稿把115 std::array赋给512原生scratch数组，编译拒绝，改测试std::copy，失败raw log/receipt保留。原生产路径未为这些工具／测试错误改规则。

接Playground前另修本枪magazine draft量纲：部件object scale与Marcus hand model scale现在独立传入，前者仅缩放part-local接触，后者仅用于原腕校准；此前共用一个参数只覆盖相等情况。追加独立scale解析验证后本格式化检查116项0，保留原111项receipt；改变的本枪调用经711项真实C0消费重新核0，其他未变Native检查复用，无Native重编。该改动只在draft formatter，无运行行为变化。

下一实际接入：从准确PG14355私有source起，加入本枪可选V4主手／弹匣publisher；main在原机械消费之后使用本帧原Frame，magazine复用原scene-matrix与physical committed-pair，分别传visual.object_scale／actual.model_scale。不新增HandClient Frame读取，不从皮肤腕回读机构，不把缺接口／被拒绝显示当机械成功；套筒原V3路径保留直到其专用姿势完成。先编译和验证这条生产接线，再单独冻结/协调运行，不能用未接线的c89 Native代替已排174315的6199。

## Playground 专用姿势发布接线已编译，原174315交窗已收到

准确PG14355上隔离加入V4主手／弹匣publisher，source authored-grasp-pg-source共1795快照项、9项delta（不是1795实际MSVC输入）。主手最终接在原equipment backend已有Frame范围退出处，晚于该范围原处理，不依赖手动换弹开关；最初未编译的Snubruntime tail方案已移除，最终Snubruntime完整文件与14355精确一致。弹匣在原read_scene_matrix及generation／pose lease检查后复用原physical committed_pair，独立传object_scale／actual.model_scale和原units；Source原Frame/sequence/sync不变。仅V4接口缺失才沿原V3；V4当前性／租约／Native拒绝不转旧路径。原slide全段保持。

check-authored-skin-producer.py抽取真实granted／magazine函数及真实equipment scope tail，直接include生产publisher，用Engine/current/pair/class/held/Native resolver与receiver fixture执行43项0（/W4/WX）：过期scene/pair后不发、错误lease／release_required／token不发、原Frame/Decision逐byte保持、Native拒绝不fallback、无V4才旧V3、真实部件scale2/hand scale.75校准解析坐标、普通/早return均在原工作之后回调。首编译fixture漏weapon_support_grip.h，修include；首运行fixture未把Decision默认release_required1改为真实有效grant的0，因此原保护正确拒绝，已仅修fixture，两个失败receipt/raw log保留。新增post-mechanics检查以单独pre_mechanics_calls计数确认顺序，不仅看回调次数。

PG完整configure0/build0，工具会话66338实际终态0。DLL build-authored-grasp-pg/Release/GearsPlaygroundSnubAuthoredGrasp.dll SHA ac7f2638b95a8d8ebb46eb2ee8bf0bed58e89394a0aca087263c2323a89c02b6，与Nativec89配套但尚未stage/run。check-authored-grasp-pg-preservation.py确认原22exports、55BOOL及实际CMakeCache相同；playground.cpp除include/forward declaration、equipment除尾部回调、grasp除可选发布段均精确原文，全部其余源／Snubruntime／机械／router精确14355；新增Frame reader=0，PG/Native五个wire／mount头精确匹配。该版本仍未完成Snap、slide专用姿势和像素接受，不作为完整手枪交付。

Troika已于其18952/creation134387002721045557全部runtime／三编码／恢复／retention61012=0后，明确最终唯一交原Snub174315（prepared2db231f1、planab545fbf、Native6199／PG14355）。已直接确认只按原174315 scope，在本方fresh artifact／idle/CIM后运行，绝不拿本轮V4替换或追加本窗case。Troika目标查询前测试Bridge缺name_pointers，未到接触，保留该失败层级，不当产品Troika通过/失败。闭环18952-review.json与successor-snub174315-final-check.json另核SHA；待本方独立空闲核验后记录接管。Frag200216撤回不占位，GN202146后到申请已指向Cover实际尾，无此处新受理。

## 原174315实跑11036：准备射击中断，窗口已交Coverr54

收到明确最终交窗后，独立核4083保存source、4bins、21helpers+1support、AST和游戏身份／idle／CIM，按原174315 Native6199／PG14355／XR1684／Source1runtime6aea实跑。game11036／creation134387010817316315，用户指定bdc6705d游戏，0.3／behind-windows／observe-only／mute／物理后台输入隔离。取Snub、首发12→11、empty_preparation_fire_0 11→10通过；随后fire1无实际弹道，弹药停10/36，首失败立即停止。未到打空、换匣、套筒、returnfire或pause，不称idle-route性能修复或新整枪完成。

两页全程paired总览和取枪／首发／prep0／失败prep1四组原始game+sim图亲审：双眼枪与主手可见，但部分腕／枪被裁切或遮挡，完整fit不接受；总览后段个别game帧缺手而sim仍有，原因未定。启动title／TPS／等待卡和TorqueBow旧pickupHUD保留。视频39.733333秒，h264 1920×660，ffprobe有效；game捕获571帧、sim840帧，startup game gap1816.66ms不作为游戏卡顿因果证明。

runner4864=0、编码39127=0、一次既有retention20146=0均实际终态；operator仍失败，保存AssertionError原文。Native cleanup／entry restore true、XR0、pad恢复、launcher0、游戏已消失；8项原基线精确恢复，settings保留原BOM，public未动。保留11036／18952／48832与48h，删2009个既定旧媒体339539753bytes，文本／source／numeric不删。最终独立idle／CIM无相关进程，21helper末次SHA匹配。19个Native input_loss均reason4 stale、bridge_contended0，其中2个在pocket之后，不能说全在startup；下一步只对自身已保存时间线定位，不做已证原因声明。自身Native header/owner及源副本字节匹配保留，但collector identity_verified=false／acceptance_evidence=false仍原样，不当GPU或权限证据。

reload-11036-review.json SHA9836f6f9cda6188ca430492c16ee9f4708bac80a6b6db0950fc718fd1820777e。封存工具首次误取retention无errors键，KeyError发生在写review／交接前；改为对既有schema读取可选空errors，未重跑retention或改其政策。8项恢复有独立restore-11036.json。交接前Coverr54原prepared／plan、24声明source、37Root+archive+AST及3bins／game／PhysX／reviewPython全精确核对，successor-coverr54-check.json SHAe58e6fb2df832e0390bbb6ce2b12218d3b29b63a6afe877366f81d4bb86b4105，随后明确最终唯一交Coverr54。交接后不再读当前done／配置或perPID live helpers，只用自身11036保存证据；新V4 c89/ac7从未stage／run。

## 逐部件姿势、被动吸附和物理接触适配的离线完成状态

11036失败及闭环已另存Git a2fd70fa8d4b9f2f21d3a4580d152e8ff50f889b／refs/codex/snub-11036-close-20261109-205225；共享HEAD／index保持。保存时间线进一步确认frame407跟随有效，408 held_chain_observation unavailable，409 hold_failed；所用输入时间早于失败fire1前快照1465.1838ms。Native两条取枪后reason4发生在其后，age139.6601／154.2644ms、bridge_contended0；不是这两个记录导致更早hold_failed的证据。具体内部拒绝层与最初性能原因仍未定，未放宽100ms或自动重新抓枪。

| 实际抓点 | 独立手型及腕部 | 吸附／操作边界 | 验证状态 |
| --- | --- | --- | --- |
| 右主握把／扳机 | profile1，原Pistol Ready为掌／拇指及中无名小指基础，rest与真实trigger仅切食指三骨；原握把腕位不另写 | 原口袋／真实主握授权，已有equipment Frame退出发布，不新增Frame reader | 已编译／host51及实际C0消费覆盖；像素完整fit未接受 |
| 左原匣／替换匣 | profile2，原Pistol reload key22的part局部腕位与15指骨；同真实原匣几何共用定义 | 原scene矩阵／可见generation／真实part lease；被动时必须双控制中立，真实握匣不被邻件抢走 | 旧原匣／新匣机械证据各自保留；新profile未游戏验收 |
| 左套筒及联动翼片 | profile3，原reload key46的thumb对置，index／middle／ring越过套筒，小指在宽度外卷曲；腕横跨顶部 | 使用当前b_HG_Slide WORLD骨坐标，先逆原参考骨；两翼仍只联动，不新增抓点；原serial/current-bone/真实release／实闭合规则保留 | 原511顶点／445三角刚性骨组已独立解出，45／46两张三向图亲审；仍168／174对>1mm交线，不能把交线长度当深度或完整fit |

通用hand_cosmetic_snap仅保存化妆性身份／序号／时间和获取状态，没有lease／库存／输入边沿输出。Snub在已有guide调用点复用同一个Frame和既有一次gun读取：进入≤25mm／25°、保持≤40mm／35°，输入原时效≤100ms；超界、时序倒退、角色／枪／世界／输入／参考／owner／部件变化和非中立均退出／重取。实际无左lease／当前模型／输入仲裁仍由Native independently admit_socket_visibility判定，Native拒绝不沿旧V3回退，未成功提交不保留扩大后的hover状态。未给idle追加五骨读取或Frame读取；为比较未吸附物理腕新增一次既有committed-pair POD读取，尚无运行性能通过声明。生产默认不再逐帧反射DrawDebugSphere，只有明确GEARSVR_LANCER_REVIEW_VOLUMES=1保留旧诊断球。实际物体与手型承担正常接触提示，未由诊断球授予机构动作。

实际publisher／原magazine、slide caller和equipment scope tail的71项host检查通过，Engine／Native receiver／current／pair／物理腕仍为fixture；原模型／Broker／GPU权限未由这些测试证明。独立slide formatter327项覆盖原参考骨完整旋转、物体／手scale独立及units变换，修正了测试准备曾假设参考Slide旋转identity而被守卫拒绝的情况；测试读取真实非identity原参考骨，生产转换未因测试改动。Snap49项检查保留时效／迟滞／身份／边界及控制退出。该检查首轮末项错误比较两次重新构造Frame的padding，已改为同一输入调用前后比较；失败receipt／raw log保留，未改产品来通过该项。

通用唯一C0已有120ms acquisition／release上追加逐authored profile、真实grasp token和hover／grasp阶段的局部过渡；只混合“当前手相对offset＋15局部指骨”，不保存旧世界腕位当跟踪。先前重置混合权重的原型虽8项通过，但可能切回free再闭合，未stage，保留Native9b4620／PG3e2bfc及source；最终平滑版37项确认切换首帧连续、逐步到新pose、真实token／阶段独立以及跟随当前手释放。Native actual adapter51项和实际V4→唯一C0指骨消费948项（原115 reference、固定所有T/scale、其余骨不动、LOD跳过骨不写）均0；旧C0 geometry7827项0，旧context grasp检查也通过。旧V2/V3直赋值在mix1保留原算式；Frame／V2／V3／V4输入wire不改、Native原admission／target guards精确原文。

随后交叉核对发现新自然抓握与旧腕原点接触判据冲突：真实比例1、units100时，profile2腕距原匣AABB80.174mm；profile3腕距Slide骨73.967mm，均不满足既有45mm。原模板固定掌部点(10,0,0)与precision点(9.75,-4.6,-2.1)从未吸附的物理腕变换后，分别距对应既有gate16.772／43.543mm；其原controller距proposal分别39.551／53.853mm，原65mm proposal仍容纳。此为原几何／固定模板的离线兼容性，不是11036射击前握持丢失原因，不是新游戏接触或完整表面验收。

因此新增通用hand_physical_contact只接受Marcus115签名、原physical wrist及真实model scale，并让Snub capture用掌部点、slide hand_sample用precision点。实际item的relative mount仍基于原物理腕，feed／slide原行程内核、原时间／sequence／token／库存／输入与release规则保持，45／65mm不扩；机构不读已拟合left_wrist。非physical旧Native路径保持旧腕行为，physical模式不因签名／scale错误转拟合骨。capture以局部proposed_mount通过原输入和物理观察末次时效后才提交，避免失败留下半捕获mount。抽取真实capture／granted／nearest／physical_contact／slide hand_sample、真实mount／stroke内核执行40项0；Engine／physical observer／current／freshness为fixture。包含wrong signature／scale、远点拒绝、物理观察独立过期、末次current拒绝后无mount残留、旧非skin兼容、源Frame／Decision不变及未见actual骨不算满行程。新纯头首次缺stereo_frame.h而standalone编译拒绝，补自身include，保留失败，未降/WX。

目前最新实际完整编译／链接为Native180bc253bde99de288380e5a8c8f719bf106a7af362bb8934b92b1b218e79e55和PG7f24255db1756d205d03eeb206a99a854a277871254857e7ff696da1dc525636；Native69／PG22exports、实际92／55BOOL及六个wire/mount公共头核对匹配。最终平滑编译工具会话9647已实际终态0；PG掌部接触编译38815亦实际终态0，第一轮guide enabled命名遮挡全局而/WX拒绝，已仅改局部变量名并保留失败。原7f69（Snap/Slide）、5d3f（平滑PG）均另存，不覆盖其source。Native1625／最终PG1798为构建源码快照条目，非MSVC实际依赖闭包。未stage／load／游戏验证，publicv71／manual入口不动，完整自然fit、吸附实际动作、套筒可见操作、双手稳定射击与头显仍待完成。下一窗口必须重新冻结本版及匹配自然腕／掌部规划的有限inspection检查，再向实际队尾提出同一候选申请；不能拿新版本复用旧174315授权或访问Cover当前helpers。

上述逐部件／Snap／物理接触源码及编译／失败证据已保存Git b937592e941573b632414914a8e7d48543a512d9／refs/codex/snub-parts-grasp-20261109-215051，private index完成10582个源／文本项保存，共享分支／HEAD／index保持。另准备了仅供下一有限inspection的四文件私有operator overlay，不改共享helper：从本次实际编译的profile2 header读取腕部定义，按当前原组件LocalToWorld＋真实body scale规划原Source1控制器位置与朝向。81种mapping／物体scale／hand scale／units的输入计划经真正Native Marcus115腕校准正向oracle，max position1.369e-12UE、basis7.633e-16；不是固定臂长IK、Native许可或游戏接触证明。

私有检查只允许inspection：原口袋／首发后，做同一magazine中立进入、35mm保持画面、50mm严格退出、远处真实旧Grip按住返回不触发机构、真实release后新Grip原匣拔出插回，库存／bonus不变和一次返射。35mm保持仍要求后续原图审查，不能用旧状态日志当新位置证据。watcher遇到首个main hold_failed立即停止、不自动重新抓枪。移除这个新检查中旧的“已动画／已拟合腕读数＝物理接触证书”调用；原生产物理palm gate和真实事务仍独立判定。检查器只运行受白名单限制的原字符串修改／AST解析，确认最终Popen仅改到一次私有operator，原startup／cleanup与Source1输入路径不替换。准备阶段引号AST及检查器Windows路径unparse表示比较两次错误均在运行前拒绝，已修工具并保留失败，未改产品通过。冻结、精确文件／AST核验及新候选自身的最终唯一交窗记录仍是实跑前条件，idle和旧174315交接都不授权新候选。
