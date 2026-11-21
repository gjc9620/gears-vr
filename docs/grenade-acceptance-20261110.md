# Frag / Smoke 当前验收边界

2026-11-11 最新用户范围与顺序确认：破片手雷与烟雾手雷均由当前会话负责，先完成破片，再继续烟雾专项开发。两者共用持续按住右Grip握持、真实松手投出、无需食指准备、2kg、直接跟手和保留原链条运动的交互约定；枪械按一下握持的规则不套用于手雷。烟雾保留原烟雾弹体、独立库存、效果与声音。既有Smoke源码和历史实测保留，不作为破片已完成或烟雾已验收的证明；完成状态仍以各自实际证据记录。


## 2026-11-11 破片040402当前结果

actual57788 / creation134388146964922144，PG0f41/Nativece9d：首次持续Grip准备、一次真实释放、原版once生成/通知/扣量4→3及completion通过，准备债务0；等待原回补失败。原continuation最初建立，随后gate36撤回，尚未执行第二次抓取或释放。缺少具体拒绝条件，不能把它归因于旧epoch、原版计时、库存或新cleanup anchor，也不能声称第二枚已投掷失败。本次实际捕获anchor但未走负向后备分支，310/311因果修复仍未证。Native/入口/XR/launcher/game正常退出，无强杀；77帧/38.972秒编码0，三页双眼时间线亲看，完整手型/回补视觉交接未接受。详57788-return-retention-failure-closeout.json。

已在同一return-retention函数的七个原拒绝出口增加最多8行原因记录，条件、调用顺序、时效及动作权限保持；去掉返回记录包装可逐字还原原函数。首次编译诊断计数器遮蔽触发C4459，重命名后Release编译链接0，原警告作为错误设置保留。新诊断DLL尚未冻结选用/游戏实跑；旧0f41冻结全部源码与三模块哈希核验保持。下一步冻结该有界观察增量，再做首次回补定点检查，不原样重跑双投。

首次名额检测与子进程重新加载之间发生过一次竞争，未创建游戏；本方paired runner将原资源等待移至同进程、分配名字/prepare之前，正式入口仍再次准入，600秒上限与暂停检查保留。实际等待块六项离线正反检查通过，无资源绕过。

## 2026-11-11 当前范围与034757补充

用户再次明确：本会话负责破片与烟雾，先完成破片，再开发完成烟雾；两者共用持续Grip握持、松开投掷、2kg、直接跟手和原链条运动。已有烟雾实现和证据保留，后续优先关闭破片连续投掷/准备问题。

PG负向cleanup anchor已接入两类手雷，正向准备/投出仍保持原身份与时效检查；仅实际派发过的原准备可通过同一实例观察偿还release/target/stop。实际客户端与负向函数离线Frag621、Smoke621检查通过，Native观察适配器359检查通过。候选PG0f41b318 / Nativece9dc35b已编译冻结。

新候选actual14044 / creation134388138685472388（034757）完成Smoke tracking-held六项检查，0投出/扣量，恢复后原mode8/neutral及新Grip准备通过。Game实际捕获anchor；本次未走cleanup_incarnation负向后备分支，不能据此宣称310/311原失败已修复。Native/入口/XR/launcher/game正常收尾，无强杀。73帧/36.894秒录像编码0、三页双眼时间线亲看；完整手型、全角度接触及物理头显仍未接受。证据：analysis/frag-grip-20261108/smoke-phase1/14044-tracking-held-closeout.json。下面Native尚未选用/PG尚未接入的段落是较早阶段记录，由本补充更新。

## 2026-11-11 最新记录

actual47612 / creation134388091773056386 在71b完成首次快投对照6项：原版once投出4→3、原回补、松开Grip无额外投出。驱动3m/s限制/12m/s²不是实测速度；2241条只读Source1中释放窗口101条/相邻100对序号连续、无>60ms间隔，原握点最高相邻速度约2.457m/s；整段另漏4序号，不能声称全帧覆盖或等同PG实际历史/弹心OLS。76帧/38.498秒GPU录像无错/编码0，三页双眼时间线亲看；回补后若干采样仍见手中手雷，完整空手显示交接/接触未接受。Native/入口/XR/launcher/game及观察器正常终态。记录 `47612-first-fast-closeout.json`。

随后actual55448 / creation134388097496589882 的双快投对照在FIRST抓取准备就失败，尚无释放命令/弹体/扣量，不是第二次释放测试。原蓝色轨迹预览仍可见；原准备债务1，stop-source先310（当前HandFrame不可得）、随后311（当前父级比较不匹配），具体不匹配字段未捕获，不能擅称与Smoke完全同因。72帧/36.443秒GPU录像无错/编码0、三页时间线亲看，末页可见蓝轨迹；父独立运行器Native/入口/XR/launcher/game及观察器正常退出，无强杀，但不能将退出成功当作运行中取消已修复。记录 `55448-preparation-stop-failure-closeout.json`。

共同停止路径现在优先。以精确a21冻结源制作最小只读Native增量已完整编译链接0：`ce9dc35bb89bb39c65d97e0be12e01061afc8a4d3ae5fa18f04585326938422e`。145配置/编译选项保持、实际编译输入只差capture.cpp和3个观察文件；移除两include及export可逐字恢复原capture。它仅观察注册/GT/世界代/实际对象身份，无新Engine调用、钩子、骨骼/输入写者或姿态权限。自有内存+注入运行/世界gate的实际适配器359检查0；Native与私有PG ABI布局同为336字节、8对齐及相同偏移。第一次完整编译因API头首次落入匿名namespace失败，已修正全局包含位置，失败保留。该DLL尚未选用/实跑，PG已派发准备的负向cleanup anchor尚未接入，所以不称修复。详 `smoke-phase1/cleanup-context-native-draft/build-result.json`、`context-test-result.json`、`abi-layout-result.json`，下一实现边界见 `cleanup-context-integration-next.md`。

共享首次准入新增single-use start-request后，本方5个独立runner将该回执纳入不可重用检查，实际predicate6例通过；不修改共享源码，不为入口变更额外跑游戏或复用旧名字。所有原失败、完整耗尽/中断/贴附/碰墙/最终手型及头显范围保留，公共包不变；本轮为progress，目标未完成。

首次快投隔离驱动已准备：沿保留finite的同一抓取/释放/原回补路径，首枚改用原第二枚的3m/s速度限制与12m/s²加速度，最多1次抓取/1次真实释放，回补后检查无额外投出并正常收尾。该数值是驱动轨迹限制，不是已测得的实际释放速度；不得替代完整耗尽验收。结果消费器26个正反检查通过，8模块dry-run编译通过，PG仍为71b，不重编Native/XR。增加的只读Source1保存原包预测时间/握持位置/朝向，线格式fixture通过，无新增读入口或输入写入。证据 `analysis/frag-grip-20261108/smoke-phase1/frag-first-fast-provenance.json`、`source1-motion-observer-check.json`。

当前运行准入等待持续遭两个活动实例门槛拒绝，尚未创建该对照游戏。此前57948投掷解失败与21072样本75.6042ms过期仍独立保留，不能以驱动/离线检查代替新玩法结果。另核对到私有已验证候选含原Frag authored hook及两支持文件，根源码没有对应文件/调用；最终整合/手动交付前须按当时单C0接口核对，不能用一次宽泛根构建冒称本候选或最终手型。现阶段保留精确私有源/二进制，不修改同伴共享源码或整体替换Native。详 `authored-source-delivery-boundary-1111.json`。

当前Frag私有候选 `equipment-controls-prepared-20261111-020542-frag-solution-observation` / PG `71b158a47c6a156e7067e64b7e769d0d1ff94aa92704f30aa9bcee14703d6034` 已完整PG编译0，新增可选查询局部的throw receipt（拒绝阶段/有效样本数/跨度/序号/样本清空原因），并区分16无解与18坐标转换拒绝。没有放宽输入、时效、4样本/55ms、原映射或反重放条件；Native/XR未改，Smoke运行计划仍选9bea。完整Bolo2016检查0，原throw套件及同一有效结果等价、50ms短跨度、旧释放序号、无效历史receipt检查通过。

该候选 actual21072 / creation134388075559612120 首枚原版once4→3并回补通过，第二枚尚持有时在reason2取消：实际当前输入age75.6042ms，违反原tracked()的60ms门槛。这足以解释拒绝，但不证明其他组合字段全通过。未进入tracker，throw receipt全0，因此没有复现/解决57948的reason16；耗尽仍失败。没有错误恢复输入；Native/入口/XR/launcher/game正常退出、无强杀。117帧/59.522秒GPU录像无错/编码0，三页双眼时间线亲看；取消后部分采样双手缺失，完整显示交接和贴合未验收。记录 `analysis/frag-grip-20261108/smoke-phase1/21072-frag-age-refusal-closeout.json`。

下一点保留两条独立失败，按同一快投动作对比首次释放与回补后释放，必要时区分继承输入年龄和本次取样耗时；不重复无改动耗尽，不把较早时效拒绝解释成投掷解问题已修复。Smoke310和完整其余范围仍保留。当前本方运行均已终态，目标active/未完成。

诊断候选248c actual57948 / creation134388068836541854 再现第二次释放取消，未通过耗尽：首枚原版once4→3及回补通过，第二枚准备后 `frag_update_safety reason=16 event=10`，取消后准备债务0，未再生成/扣量。原因16只定位“无投掷解或游戏坐标转换失败”合并分支；实际当前输入age20.7803ms、history_count5、edge6175/current6177，不能声称已定位具体历史样本/边沿/转换子条件或修复。修正后的驱动实际没有错误恢复菜单输入，cleanup_deferred_to_owned_runner明确；Native/入口/XR/launcher/game正常退出、无强杀。110帧/55.843秒GPU录像无错/编码0，三页完整双眼时间线亲看；后段采样仍见手中手雷/链条，取消后的完整显示交接和贴合未验收。记录 `analysis/frag-grip-20261108/smoke-phase1/57948-frag-solution-refusal-closeout.json`。

下一点只细分已有投掷解分支及原tracker的样本数量/跨度/顺序证据，保持60ms、4样本/55ms、源身份及反重放条件。不要在无变化下重复耗尽。Smoke310为另一已保留问题，当前没有自动运行中的本方实例；目标未完成，公共包不变。

Frag耗尽 actual55308 / creation134388061626761290 在f500失败：首枚原版once投出4→3、真实原点/速度匹配并回补；第二次新Grip准备后释放进入BoloEvent::safety_required，取消后preparation_owed=0，无第二弹体/扣量。之前一次gate102 rig拒绝后恢复，不能据先后关系确定因果。未到末枚/空槽。旧finite驱动错误收尾还额外发送菜单输入并报恢复失败；该额外输入和原失败均保留，不冒称严格首败无后续输入。父独立运行器最终Native/入口/XR/launcher/game正常退出、无强杀。124帧/63.117秒录像无录制错误/编码0，三页双眼时间线已亲看；末段可见旧驱动失败收尾菜单。详 `analysis/frag-grip-20261108/smoke-phase1/55308-frag-finite-failure-closeout.json`。

随后适配器已将empty_cue加入“失败后不发恢复输入”分支，8模块dry-run通过。新Frag私有候选 `equipment-controls-prepared-20261111-015120-frag-update-refusal` / PG `248c2f52420ff2a8cc089298512ca1377f08bb838d36f1c28d46ebdafdd04301` 完整PG编译0，增加17处模型实际安全取消原因的限量日志；去除诊断后的原检查条件文本逐字一致。现有完整Bolo模型2016检查0，另原私有focused116检查0，不能混称后者是全套。Native/XR未改，Smoke运行计划仍选已验9bea。新候选耗尽专项准备名 `grenade-smoke-1111-015258-frag`，结果另记，不推定已修复第二次释放或Smoke310。

当前Smoke9bea新增 actual42236 / creation134388058147586387 暂停专项6项通过：右Grip保持按住，经原菜单暂停/B恢复，原squeeze press/release计数及输入cohort不变，不重装/不投出/不扣量；旧Grip松开安全，新裤兜Grip再次准备。库存始终2，Frag不变，零弹体生成。实际79帧/39.949秒GPU录像、编码0，三页完整双眼时间线亲看，暂停UI/恢复可见；Native/入口/XR/launcher/game及自有进程正常退出、无强杀。日志gate60/10且preparation_owed=0，未出现stop-refusal/310/当前父级恢复行，故不关闭偶发取消问题。记录 `analysis/frag-grip-20261108/smoke-phase1/42236-smoke-pause-closeout.json`。

Frag耗尽专项已从保留的原finite驱动接入独立入口，复用f500生产二进制；新适配仅选择empty_cue并关闭本专项后的无关菜单/姿势输入，原身份、CLOAK、静音、首败停止和正常收尾不变。结果消费器87个正反检查通过、8适配模块dry-run编译通过，均非游戏结果。actual准备名 `grenade-smoke-1111-014038-frag` 正在原子资源准入等待，尚不可称已验证耗尽。

更新：资源释放后，新CLOAK actual55104 / creation134388048153351663 已在Frag f500候选完成核心10项：持续Grip/无扳机准备、一次真实松手、库存4→3、原生成/扣量/通知/Init/原点/速度各一次、原引信及停下未爆阶段、原爆炸与目标血量下降、回补后新裤兜Grip准备且无额外投出。163帧/82.888秒GPU录像录制无错/编码0，三页完整双眼时间线已逐页亲看；敌人遮挡多个持有角度，完整手型接触/所有方向仍未验收。Native/入口/XR/launcher/game正常收尾、全部自有进程退出，无强杀。该次未证明负向恢复helper实际分支；不关闭Smoke310或完整中断/耗尽/贴附/特殊碰墙/体验/最终GrabPose/头显范围。精确记录 `analysis/frag-grip-20261108/smoke-phase1/55104-frag-core-closeout.json`。下文“Frag尚无最新候选游戏结果”为此次运行前状态。

本会话负责两种手雷，优先完成破片，再完成烟雾。两者均持续按住右 Grip 持有、松开投出，无需扳机准备；2 kg、直接跟手，保留各自原版资产、链条、弹体和效果。以下按精确候选分别记录，历史结果不替代最新候选验收。

当前 Smoke 私有候选 `equipment-controls-prepared-20261111-001443-smoke-cleanup-source-diagnostic`，PG `9bea0866394a49aa98b131f08ef02c5e768b4c4c14560b4ba0ae4213a29b1981`。actual53416 / creation134388016872440280 耗尽专项14项通过：两次真实释放、库存2→1→0、原版末枚自动切枪、两次空槽Grip各一次提示，无第三次准备或投出。88条效果观察中86条取得原粒子及循环音频组件；这是静音运行的组件证据，不是实际可听验收。157帧私有GPU录像、79.829秒，录制编码正常；三页双眼时间线已逐页亲看，烟雾增长、遮挡及清散可见，完整接触/各角度/头显仍未验收。Native、入口、XR、launcher及自有游戏正常退出，无强杀。

前一候选1c6e的 actual53420 / creation134387997058495514 首枚2→1及52次原粒子/音频观察通过，但第二次准备失败：停止路径310表示真实当前HandClient Frame不可得，仍处于mode7并有预览。158帧/83.003秒录像编码0，三页已亲看；失败原样保留。53416没有触发103/310或当前父级恢复日志，不能据成功认定该偶发取消问题已修复。actual37116只证明其记录候选的右手跟踪丢失/恢复功能流程，亦未触发恢复helper。

最新 Frag `f5001fce43fcecf7dcc3c2a68b58e413227d5f1763a4fa8a93c0c5272459fee5` 已编译，真实生产停止/释放与HandClient离线43检查通过；独立frag-core适配器已准备，尚无该候选游戏结果。本次启动申请在准备前被两活动实例门槛拒绝，未创建新游戏/worker；随后证据盘低于4GiB。继续等待资源并做离线收尾，不放宽门槛或占用他人实例。旧Frag2956成功、Smoke失败和全部剩余16组范围继续保留；公共v71及固定手动入口未改变。

精确记录见 `analysis/frag-grip-20261108/smoke-phase1/53416-finite-closeout.json`、`53420-cleanup-source-failure-closeout.json`；其后的旧“当前候选”描述均为历史检查点。

2026-11-10。目标仍是完成两种手雷的完整生产流程、体验、最终 GrabPose 和分别记录的头显验收。此前自动 Game / XR 禁令已按主人“修好后继续跑”指令解除；当前仅准用新 CLOAK 入口及全新实例；不以编译、离线检查或旧候选通过替代当前实际验证。

当前私有候选：`equipment-controls-prepared-20261110-230413-smoke-tracking-driver`，Playground `1c6e92895e36e331f24fa251325b5160577d59238f083eb3015c43289ea87d18`。Smoke新增仅停止用的真实当前父级寿命，原source/lifetime和全部投掷检查保留；35个直接生产helper正反检查、完整PG构建0。actual57756首枚投出／双眼烟雾可见，但效果观察器属性失败，未到取消分支。当前tracking-held驱动0投掷／2抓取，离线7例和5模块dry-run通过，尚待实际容量／磁盘门槛允许。未交付公共v71或最终手动包。

| 要求范围 | 已有证据 | 仍缺少的证明 |
| --- | --- | --- |
| Frag 基本闭环 | actual2956：持续 Grip、无扳机准备、真实释放、原生成/扣量各一次 4→3、原爆炸/伤害、空手返回和新裤兜抓取 | 最新候选及完整中断、耗尽和动作变体 |
| Smoke 基本闭环 | actual2320：原 Smoke 覆盖/base/生成/扣量各一次 2→1，Frag 库存不变；原烟雾增长／遮挡／淡出及新裤兜 Grip 再取通过 | 当前3ef候选尚无普通投掷完整闭环 |
| 运动与原点 | 原始手部轨迹和一次基底速度合成；新原点平移逻辑 2518 项离线检查 | 新逻辑实际移动投掷；平移间隔中的历史释放缺少历史身体原点，仍安全取消；零速、近墙、方向及头转组合的完整实际覆盖 |
| 库存与中断 | 两轮 2→1→0 模型循环、同 Grip 空库提示去重；暂停保留负例和两个实际验证脚本已接好 | 08b候选actual37860耗尽／原自动切枪／空槽提示和55720暂停／恢复已通过；当前3ef须确认，原效果不中断／不刷新仍缺 |
| 原碰撞与贴附 | Frag 普通爆炸和历史原贴附记录；Smoke 原专属效果已观察 | 特殊 HitWall 必须单独实录；Smoke 贴附必须有自己的证据；历史 Frag 贴附的时间段和清理限制继续保留 |
| 显示、链条、2 kg、手型与头显 | 两者原独立资产/链条；2 kg 配置、直接跟手；部分双眼图像未见蓝线 | 完整链条/皮肤/所有方向、最终握姿/腕袖接触、连续显示交接和实体头显手感 |
| 稳定性与运行保护 | 多次自有实例正常清理；原件和公共包保护保留 | actual38928 未派发原因未证；actual39580 当前输入 67.2344 ms 超过原 60 ms，未放宽门槛；主人再次反馈前台，短测和采样零命中不能作为后台通过 |

后续允许运行时先补最新 Smoke 核心闭环，再按实际结果执行耗尽或暂停专项；继续遵守首失败正常停止。特殊碰墙/贴附、体验和最终 GrabPose 仍在完整范围内，不因基本投掷已通过而取消。

完整 16 组要求、逐项缺口、原始证据 SHA 与审计副本见 [requirements-current-20261110.json](../analysis/frag-grip-20261108/requirements-current-20261110.json)。既有成功、失败、离线结果及头显未接受状态分别保留；整体完成未获证明。

## 本会话录像滚动保留

已完成 10 次自有 Smoke 会话录像的身份、进程退出与编码终态核对，并按最近 3 次规则保留 actual39580、47128、38928。其余 7 次已审阅录像的 11,379 个媒体文件已删除，共 1,857,789,377 字节；源码、原始日志、数值、失败记录和审阅记录保留。53440 的最后编码正常退出；完整配对页仅覆盖启动失败/等待画面，没有手雷动作。旧审阅记录中被滚动淘汰的媒体路径不再保证可打开，保留的文字结论不因此升级或撤销。范围只含本会话已认证的 Smoke 录像目录。详见 `analysis/frag-grip-20261108/smoke-phase1/owned-media-retention-result.json`。

## CLOAK 恢复后的实际结果

主人明确恢复运行后，本会话使用当前候选和 `build-instance-cloak1110` 重新准备全新实例，没有复用旧后台实例。actual56780 已进入 Playground：549 次本方窗口读数均为隐藏、1130 次前台采样无本方命中；315 帧真实 GPU 合成双眼画面录制并编码正常，2 页完整时间线已审阅。这些采样有间隔，不代表所有时刻的绝对证明。第一轮 Grip 在输入年龄 63.6719 ms 超过 60 ms 时取消；采样前年龄 31.9710 ms，采样至 stamp 耗时 31.7009 ms。库存仍为 2，没有投出。Native、入口、XR 和游戏正常清理。

最新诊断候选为 `equipment-controls-prepared-20261110-194038-smoke-sample-cost`，PG `08b701fdddedbcf2536d060447ec1054a0b3309897e302eed9b1e3fca5a995fd`，仅新增限量采样分段计时（最多 8 行），不改变身份、输入、原调用、机械或新鲜度条件。首次编译遇到局部变量遮蔽警告，改名后编译通过。该候选 actual15032 在 XR source0 预检失败，原错误 `xrCreateInstance (-6)`、有效行 0；Native 未激活，未进入手雷动作，故采样诊断没有实际数据。原 worker 收尾时游戏尚未消失，随后精确身份复查已确认游戏退出、未强杀；保留原 worker 的失败结果，不改写为正常生命周期通过。未进行不变重试。

分段耗时的实际原因和 XR 初始化失败原因均未证实。最新候选完整投掷／重新抓取验收仍未完成；下一步先查 XR 初始化原始失败，再执行针对性的实际检查。记录：`analysis/frag-grip-20261108/smoke-phase1/cloak-resume-actual-20261110.json`。

actual15032 最终四项精确进程身份（game／owner／XR／worker）复核均已退出，未强杀。XR 边界只定位到 loader/API-layer 链创建实例失败：该进程 runtime 日志有协商与扩展枚举，未记录进入 runtime 的 xrCreateInstance。不能仅据此将并发或 Operator 层判为根因；未删除现有 Source1 层或修改统筹维护的共用启动工具。

## XR 独立对照结果（19:54）

相同 XR 二进制和 CLOAK runtime 的短时对照：启用 Operator API 层退出 0、CSV 265 行；不启用层退出 0、CSV 269 行。两次启动前均记录外部进程 23924 监听 8720。因此固定端口被占用本身不足以解释 actual15032 的初始化失败；原失败根因仍未证实，不能据此删除生产层。对照未启动游戏或发送输入命令，两个子进程正常结束。初次脚本在创建 XR 子进程之前遇到本地化 netstat 的 UTF8 解码错误，改为保留原始字节并按 Windows 代码页解码后才完成对照。

之后两次最新手雷候选运行申请都在准备／启动前被两个实际活动实例的容量检查拒绝；没有新游戏进程、没有武器验证进展。源日志和结论见 `analysis/frag-grip-20261108/smoke-phase1/xr-create-layer-conclusion.json`。最新采样计时候选继续保留，等待实际容量后执行有界核心流程。

## 最新 Smoke 核心闭环实际通过（actual2320，20:00）

PG `08b701fdddedbcf2536d060447ec1054a0b3309897e302eed9b1e3fca5a995fd` 的全新 CLOAK 实例完成持续 Grip／无扳机准备、真实松手投出、原 Smoke override／base／生成／扣量／通知／Init／原点与速度替换各一次，库存 2→1；初始位置速度匹配且方向恢复。原烟雾 emitter／粒子／循环音频组件取得 88 次观察；释放后不重装／不再投，Frag 库存不变，原回补后新的裤兜 Grip 再次准备成功且库存仍为 1。该有界核心 case 实际通过；Native、入口、XR、launcher、game 全部正常收尾，未强杀。

547 帧真实 GPU 双眼合成录像无录制错误，编码退出 0；完整两页时间线已审阅，两眼均见烟雾增长、遮挡与淡出。选定画面可见手雷和原链条，完整链条／各角度贴合、再次抓取的手部接触和实体头显均未验收；静音运行也不证明实际可听。900 次本方窗口检查无非隐藏记录，1417 次前台采样无本方命中，仍保留采样覆盖限制。

本轮未输出超过 20 ms 的限量采样计时行；不能据此说此前 63.6719 ms 新鲜度失败已被修复，也不能将本轮启动成功当作 XR 初始化失败的根因修复。随后 finite-empty 申请在任何新实例启动前被两实例容量限制拒绝；耗尽与暂停专项继续待实际验证。详细记录：`analysis/frag-grip-20261108/smoke-phase1/2320-core-closeout.json`。

## Smoke 有限库存专项实际通过（actual37860）

同一当前 PG `08b701fdddedbcf2536d060447ec1054a0b3309897e302eed9b1e3fca5a995fd` 完成两次新 Grip／真实释放、原生成和扣量各一次，库存 2→1→0。末枚完成回执 capacity=used=1、reserve=0、infinite=0、next_expected=0、continuation=0，原完成调用一次且实际自动切换。两次独立空裤兜 Grip 的原提示各派发一次，持续按住／移动不重复，没有第三次准备或生成，Frag 库存未变。Native、入口、XR、launcher、游戏和观察器全部正常结束、未强杀。

749 帧真实 GPU 双眼画面、3 页完整时间线均已审阅，录制无错误、编码退出 0；可见两次烟雾，末枚效果完整自然寿命未作为本专项结论。1006 次窗口读取全部隐藏，1458 次前台采样无本方命中。静音专用回读 initially_muted_sessions=1、verified_muted_observations=531、verify_failed=0、target_shared_rejected=0；muted=0 表示没有新增静音写入，不是未静音。全局 shared_rejected=5841 不是本枪被拒绝数量。

首次绑定的两条慢采样记录：22.1887 ms（配置16.7896 ms）及20.1438 ms（配置14.8746 ms），未改变任何新鲜度条件，不能宣称此前超时已修复。详情：`analysis/frag-grip-20261108/smoke-phase1/37860-finite-closeout.json`。暂停专项当前另用新实例验证中；完整目标仍未完成。

## Smoke 持续 Grip 暂停／恢复实际通过（actual55720）

同一当前 PG 完成右手 Grip 始终按住、原菜单暂停、原准备状态停止、B 恢复而原 squeeze press/release 计数不变且不重装／不投出、旧 Grip 松开不扣量、新裤兜 Grip 再次准备。全过程库存 2，Frag 不变，零生成、零扣量。此为功能性暂停覆盖，不声称某一 guard 分支独占触发。原效果在暂停后继续、真实跟踪丢失／重定位／换世界仍须分别验证。Native、入口、XR、launcher、game 与观察器正常退出，无强杀。

391 帧 GPU 双眼录像无错误，编码退出 0，2 页完整时间线已审阅，原暂停 UI 与恢复实际可见。746 次本方窗口检查无非隐藏记录，1570 次前台采样无本方命中；561 次目标静音回读成功，verify_failed=0、target_shared_rejected=0。中断记录当前输入年龄 71.1706 ms，其中采样前已68.6188 ms，采样至 stamp2.5518 ms；保留现有60 ms门槛，不能把此现象归结为配置读取耗时或称延迟已修复。详情：`analysis/frag-grip-20261108/smoke-phase1/55720-pause-closeout.json`。

## 换种类驱动与 actual59020 首败

已准备 `equipment-controls-prepared-20261110-203043-grenade-kind-switch`：PG／Native 不变，只增加原 Frag→Smoke→Frag 的两次 X 拾取／三次新 Grip 驱动。每次拾取前准备旧种类且右 Grip 保持按住，以同源／同 squeeze 计数证明未偷偷松手；旧释放不得生成或扣新种类库存，末次新 Grip 应再次准备。实际 replacement 分支离线正常例和三个负例通过，六个适配模块 dry-run 编译通过。该场景明确不覆盖已派发投掷 token 与换种类竞态。

actual59020 启动／预检通过后，在到达 Smoke 拾取物以前的第一路段中止：四次位置记录始终为(-3900,0,583.9485)，距离首 waypoint250不变，原3次 stall 保护触发；尚未执行旧 Frag 准备 hook，没有任何种类切换／投掷结果可计入。62 帧 GPU 录像编码0、完整1页时间线及末帧已审阅；不是之前走至石栏边缘的失败，不能据此改路线。同期 rig source unavailable、较多输入 stale 拒绝和 consumer jitter 保留，但根因未证实。Native、入口、XR、launcher、game 和观察器均正常收尾，未强杀；没有不变重试或放宽输入门槛。完整记录：`analysis/frag-grip-20261108/smoke-phase1/59020-kind-switch-prepickup-failure.json`。

## 换种类部分进展及准备栅栏修补

actual13552 首个方向通过：原 Frag 准备时持续 Grip，X 换为 Smoke，同源／同 press/release 计数，旧释放零投掷／零扣量；返回 Frag 的西后侧路线滑撞石栏，末段111条Source1记录中26条非中性左摇杆，连续序号最大sync间隔72.6225ms。它不复现actual59020的起点完全无位移，也不证明消费端每次采样均通过。427帧录像编码0，2页完整时间线已审阅。

actual36268 在原 X 换种类后保留旧 Frag preparation_owed=1，gate11反复；驱动旧通用错误文字不能证明新Smoke spinning。296帧录像编码0、2页已审阅，选定画面出现蓝线／蓝色标记，其手雷预览归因尚未单独证明。保留此实际失败。

源码修补使用现有 Native 原按钮栅栏：准备前先臂起；sync保留Frag／Smoke准备和现有可见性租约所有权；既有pre-original PlayerTick在原按钮待处理时调用各自cancel／原stop，严格证明原停止终态后才释放该准备的栅栏份额。Native、输入发布器、身份／原动作寿命／反重放／新鲜度检查未改变。直接提取生产函数的离线夹具：旧版32检查3失败，新版117检查0失败；私有PG编译0，新SHA `ed61f10eee2a0da549bb0f4f53066f6bcaacf127e5b4b254e5d11cf38f97ad76`。actual35768已见gate60、preparation_owed=0，替换后PC key／PC targeting／Pawn targeting／新Smoke spinning四项false；首方向通过，返回仍撞障碍，331帧录像编码0、2页已审阅。两个实例Native／入口／XR／game／观察器正常结束，未强杀。完整往返和新PG的普通投掷、耗尽、暂停及原B贴附仍需当前候选证据。

路线另外修正：80cm退后曾被默认85cm到达半径跳过，直接向东又受阻。当前驱动只沿本次已实际完成的Smoke接近段终点以35cm半径返回，再进入Frag2956实际东侧通道，未走完／非有限记录拒绝使用。不是传送、碰撞放行或失败后自动换路；当前新实例验证中。记录见 `13552-kind-switch-partial.json`、`36268-kind-switch-closeout.json`、`35768-kind-switch-closeout.json` 和 `pre-input-fence-candidate.json`。

## 最新 actual41460 启动失败边界

重新准备的 retrace 路线尚未进入动作：`PlaygroundWeaponHandStart`返回2，协调代码报Unified equipment connection rejected。当前ABI中2为Reply::invalid，不是Windows错误码。父注册或任一子attach均可传播该值；尚未定位具体子步，也不能仅凭PG日志未见普通tick就断言GT=0。保留原身份／ABI拒绝，不放宽或无条件重试。Native／入口清理true、XR0、游戏／owner／观察器全部退出，未强杀；launcher1是原失败结果，不能改为生命周期通过。详情：`analysis/frag-grip-20261108/smoke-phase1/41460-startup-connection-failure.json`。完整目标仍未完成。


## 当前准备态换种类往返与最新耗尽首败

actual46040（creation134387913700716933）在当前3ef候选通过两次原X换种类、三次新Grip准备：Frag→Smoke和Smoke→Frag均保持同Source1／同squeeze press-release计数；两种pre-original gate60均实际到达，替换后PC key／PC targeting／Pawn targeting／新种类spinning均false。旧Grip释放零生成／零扣量，最后新Frag Grip可准备。201帧真实GPU双眼录像覆盖102.874秒，3页完整时间线已逐页审阅，录制errors为空、编码0；该记录证明准备态换种类，未覆盖已派发投掷token竞态。手部选定可见，完整皮肤／链条／全角度接触和头显未验收。Native／入口／XR／launcher／game／观察器正常收尾、无强杀；1225次窗口采样无非隐藏，1651次前台采样无本方命中，保留采样间隔限制。记录：`analysis/frag-grip-20261108/smoke-phase1/46040-kind-switch-closeout.json`。

前次actual30264（本次creation134387906076823155，与旧有限库存同PID不同实例）机械JSON检查均通过，但录制在56.047秒后因No complete fresh projection frame失败，动作继续至178.878秒才停止。整体case=false，不能作为完整通过；已编码的片段仅2页partial时间线已审阅。此后本会话私有驱动在录制首错误后拒绝继续输入，私有GPU请求间隔从0.10秒调为0.50秒；共享录制器、游戏机械和新鲜度条件未变。后一完整成功不证明已查明该录制失败根因。记录：`30264-recording-failure-closeout.json`。

actual25656（creation134387922102389275）当前finite-empty未完成第一次Smoke准备，未进入投掷／耗尽。最后Smoke gate102／rig_mismatch1对应handrig读取未接受；Smoke行未记录具体ReadResult，根因未证实，不能归为原按钮栅栏回归。60帧GPU记录编码0、两页时间线已审阅，但存在长帧间隔及多张黑帧；errors为空只证明录制器未报告错误，连续可见性未接受。Native／入口／XR／launcher／game／观察器均正常结束，无强杀。下一步按已有Native来源／发布／消费时序定位拒绝，保留原rig、身份和新鲜度保护；不作不变重跑。记录：`25656-finite-preparation-failure-closeout.json`。

一次性状态按A20/B30/C20/D15/E15保守记录Frag沿用45%，Smoke由未开工0%重新评估为45%，两者均为.5/.5/.25/.5/.5；不是头显或完整交付接受。报告保存在本会话`one-time-status-20261110-2200.json`，未主动跨会话发送。


本会话录像保留已滚动至最新3次完整身份：25656／46040／30264（本次creation134387906076823155，片段失败结论保留）。21个已退出且已审阅的自有录像目录复核后，删除旧媒体7,478个、1,327,584,526字节，无错误；源码、JSON、原始日志及数值未删。详见`owned-media-retention-result-2200.json`。


## actual53724 首枚闭环与第二枚元数据拒绝

前一25656原Native统计：rig发布231，读取879／拒绝794，仅说明读取拒绝较多，不能定位那一次Smoke拒绝的具体原因。针对原handrig失败返回补最多8条只读日志，PG72773287编译0；首次prepare脚本在旧solo薄入口名字断言停止，未改薄入口或运行游戏。随后明确复用独立isolated入口及认证冻结构建，保留旧入口退休状态。

actual53724（creation134387931654760705）当前finite驱动首枚核心10项均通过：持续Grip准备、真实释放一次2→1、原Smoke override/base/生成/扣量/通知/初始化/原点/速度替换各一次，原烟雾对象与粒子／循环音频组件寿命观察、新Grip再准备库存仍1，Frag不变。第二枚在原准备后gate107（Smoke元数据read或与certified比较）失败，随后gate10正常取消，没有第二枚弹体；finite整体case=false。该局没有新rig-refusal行，不能称前一rig问题已修复。

122帧真实GPU双眼录像无录制errors、编码0、三页完整时间线逐页审阅，两眼见原烟雾增长、遮挡与淡出；录像有明显长帧间隔，连续时间覆盖不接受，整手／全链条贴合未验收。Native、入口、XR、launcher、游戏和观察器正常退出，未强杀。详见`analysis/frag-grip-20261108/smoke-phase1/53724-finite-partial-closeout.json`。

新134c候选只把原Smoke metadata失败分为initial-current、class/ammo/emitter、原script指纹、ammo再读／retained-metadata、final-current和certified比较六类，成功路径仍相同次序、相同原读取和检查，最多8条负例日志；未重读／刷新Frame或放宽期限。完整PG构建0，尚无Game证据。下一点区分实际声明变化与末尾当前帧失败，再做必要修补；不据成功重试倒推根因。


## actual20452：producer拒绝与原取消首败

当前134c候选actual20452（creation134387938633865346）首次原准备成功，实际读取拒绝留下`reply=1 failure=4`、observed105153381115100、input_sync105153360983700。按当前reader，failure4为producer连续性flags不足：provider及envelope检查已通过；输入年龄20.1314ms，不能归结为age超限。后来恢复过一次普通采样，随后gate11／stage6／preparation_owed1反复；驱动在计划释放前检查原bDisplayingArc仍true而停止，零投掷。该局没有metadata-refusal行，不证明前53724的元数据问题已修复。

57帧实际GPU双眼录像、编码0、两页完整时间线已逐页审阅；末段可见蓝线，原preview字段失败保留，完整接触未接受。Native／入口／XR／launcher／游戏／观察器正常退出，未强杀；原准备债务在shutdown以前是否结清没有证明，不把正常进程退出充作该机械清理通过。

下一点查smokepreparation::stop及smoke_original::release_existing的实际拒绝。源码保留ExistingActionLifetime的epoch／owner／reference检查，不把过期Frame刷新成新动作许可；现有failure参数可定位lifetime拒绝，但本次未记录该值，不能声称已定位到某一个epoch或对象。先捕获或直接重现精确取消失败，再修补，不作不变finite重跑。完整记录`analysis/frag-grip-20261108/smoke-phase1/20452-producer-cancel-failure-closeout.json`。


## actual34260 精确停止拒绝及当前取消修补

原d320诊断候选actual34260（creation134387950666109916）首枚原调用2→1和78次烟雾对象观察通过；第二次抓取在原preview字段检查失败。三种原停止调用均拒绝103，对应ExistingActionLifetime初始检查failure3（捕获epoch／phase不再active）；原mode7未结束。terminal_guard3在mode!=8时短路，四个打印为0的布尔值尚未读取，不能当作中立状态证明。122帧双眼GPU录像编码0，三页完整时间线逐页审阅；正常Native／入口／XR／launcher／游戏／观察器退出，无强杀，原准备债务在shutdown前是否结清未证明。记录`34260-stop-epoch-failure-closeout.json`。

停止诊断本身的原生产release函数及原mode8证明，与改动后1,296个成功／逐guard拒绝用例对比，返回、调用顺序／次数及原Request完全一致；这些是mock Engine/client边界的离线证据，不是游戏成功。

实际修补只在已有、确已派发的Smoke准备取消路径使用：优先原动作寿命；只有原failure3、注册仍active且restoration quiet，才尝试原客户端真实copy_frame_on_gt。新帧必须满足原EquipmentIdentity的全部Native/key/world/controller/pawn/mesh/weapon/reference比较，仅允许真实更晚frame号。随后由原capture_existing_action_lifetime_on_gt获取当前真实寿命并复核；单独cleanup_source/cleanup_lifetime只送往原release／target(false)／stop三方法。journal.source/lifetime、投掷运动与原launch／completion绝不替换。没有新帧、错误父级／参考／注册或获取失败仍拒绝；mode8／四状态中立双读及原fence清理保持。35个实际helper+真实equipment_matches正反检查、PG构建0；新增最多8条成功分支日志以区别实际执行与推测。Frag尚未套用，须在Smoke实际验证后针对同构问题处理。证据`cleanup-current-source-repair.json`和`cleanup-current-source-regression/result.json`。

## actual57756 组件观察失败与独立中断驱动

actual57756（creation134387961745057349）使用1c6e，首枚原投掷一次2→1；95帧双眼GPU、三页完整时间线审阅，烟雾增长／遮挡／淡出可见，录制无errors、编码0。87次effect读数中85次Property Template not found，旧观察器在同一个try内先读粒子属性，失败后未尝试SmokeSpewer，最终报loop audio未观察。它不能证明音频组件不存在，实际粒子组件身份当时未留存，stale class hint只在离线夹具复现，不能称为实机根因。此局没有current-parent成功日志，修补分支未实到；整体case=false，正常收尾、无强杀。记录`57756-component-observer-failure-closeout.json`。

待后续完整core使用的观察器已先fresh认证粒子／音频组件，并独立捕获失败；只有组件身份及owner指针关闭核对后才写成功证据。7种直接helper正反例通过，错资产／组件复用不会留下阳性字段；原emitter退休检查和粒子／音频必须实际观察的断言未删。该观察器改动尚未冻结进当前230413tracking专用候选。

当前tracking-held复用现有Source1 tracking参数，右手失追0.6秒后恢复，Grip不松；记录失追时是否已停止，恢复后严格要求mode8／全部target/spin中立，同source cohort／同squeeze计数，旧释放零生成／零扣量，再新Grip准备。独立case不生成弹体、不运行烟雾效果观察，也不覆盖耗尽；7个真实branch提取mock用例和5适配模块dry-run通过，尚未Game。失追与恢复都是预定输入，任何实际失败后不补救输入。完整Frag／Smoke范围、特殊碰墙／贴附、运动／体验／最终GrabPose／交付／头显不变。

## 本轮证据卷空间与冻结记录

C盘一度free0，首次全复制223007候选失败，未更新当前计划、未开Game；无效片段树保留且不被选用。本会话仅移除19个旧构建可重建.obj中间文件146,724,143字节，源码／成品／日志保留；随后新冻结使用不可变相同文件硬链接，改动文件先解除链接再独立复制。22个自身previous_prepared冻结的1,642份相同内容已按声明SHA验证并原子去重，所有路径／字节不变，未改根源码或他人实例。共享磁盘准入最低4GiB保持，准备／启动不足只等待，不视为武器通过或blocked。新运行会独立记录并归档实际共享isolated_sim/runtime_receipts/hidden_children/start源码哈希；中央入口未由本会话修改。当前对自身analysis/frag-grip-20261108做Windows透明压缩，保留字节与路径；不含他人或根源码。

2026-11-11 04:13补充：return-refusal候选已冻结为equipment-controls-prepared-20261111-041143-frag-return-refusal，PG8c9dc566 / Nativece9dc35b，实际编译输入只差weapon_frag_runtime.inl；1105份旧冻结源码及三模块哈希保持。只切换Frag定点检查计划，尚无新游戏证据。新检查任务已提交，当前首次prepare前等待名额，继续同一任务不得重复启动。37次自有终态媒体按最近3次57788/14044/55448及48小时保留，清理201个旧媒体文件约53MB，错误0，源码/数值/日志/失败记录保留。

2026-11-11 actual30828 / creation134388154468557597，PG8c9d/Nativece9d：两次相同3m/s限制、12m/s²驱动轨迹投掷与原回补通过，库存4→3→2，原生成/扣量/通知/初始状态各once匹配，六项检查通过，松手无额外投出。驱动限制不是实测释放速度。57788 gate36未重现，旧失败原因未证，负向cleanup fallback未走，不能据通过宣称因果修复。95帧/54.395秒录像编码0，三页双眼时间线亲看：启动有一黑样本，回补后部分样本仍见手雷/链条，最终空手交接/全角度贴合未接受，末尾爆炸像素不等于伤害读数。正常完整收尾，无强杀。新增原因记录的8行被启动空闲调用耗尽，已修正为只记录活动回补并编译0；后续推进完整耗尽/最后自动切换/空袋原提示，不原样追跑双投。详30828-paired-fast-closeout.json。

2026-11-11 30828视觉细查纠正：回补输入sync映射约38.729/52.538秒，分别亲看frame64/65（39.018/39.530秒）及91/92（52.826/53.351秒）原双眼图。所选样本右掌张开、掌中未见握持手雷，手雷/链条像素位于双手下方的靶场武器物件区域。撤回仅据总览缩略图的手雷/链条像素推断空手交接失败；这些选定样本支持空右手，不等于连续/全角度手型验收，也未证明每个物件归属。输入sync早于消费回执、截图request/observed不是GPU精确渲染时间，保留对齐边界。详grenade-smoke-1111-041558-frag/handoff-timeline.json。

2026-11-11 actual46360 / creation134388162498346824，PGfd27/Nativece9d：finite在首次准备后、Grip仍按住时以reason10运动边界中断，后续旧释放未投出；0弹体/扣量，库存4，原准备债务0。未进入第二抓取、最后自动切换或空袋；不是57788回补故障重现。附近4647→4648源sync间隔100.941ms，但该次未启用独立Source1观察，不由此断言具体边界原因。正常完整收尾，无强杀；80帧/40.518秒编码0，三页双眼时间线亲看，整体手型/头显未接受。详46360-first-held-boundary-failure-closeout.json。

随后仅增加七个原boundary谓词的被动bit记录，去掉记录字段/表达式可逐字恢复fd27模型；原boundary表达式、条件、时效和动作权限不变。实际Frag模型2059、Smoke2518检查通过，覆盖独立stream/tracking与既有姿态/参考/比例/间隔边界，Release完整编译链接0。新候选冻结，实际编译输入只差共享手雷模型头和Frag runtime；原冻结全部源码/模块哈希保持。有限驱动启用已有只读Source1观察器，无输入/Engine写入。候选尚未Game验证，完整耗尽及异常恢复/贴附/碰墙/体验/手型等范围保持。

2026-11-11 04:41状态：边界观察候选equipment-controls-prepared-20261111-043956-frag-boundary-observation，PGb85cef65 / Nativece9dc35b已冻结。带只读Source1观察的finite任务11057已提交，首次prepare前等待名额，尚无新Game证据；继续同一任务，不重复启动。自有39次终态媒体按最新46360/30828/57788及48小时保留，77旧媒体文件约18.6MB清理0错误；源码/模块/日志/数值/失败记录不变。

2026-11-11 actual54300 / creation134388172331699011，PGb85c/Nativece9d：在首次裤兜Grip取得前失败，无Frag准备/runtime/弹体/扣量，新boundary与负向fallback未走。只读5647..5653共7包连续、最大相邻35.084ms、同cohort/焦点/有效右手；Native两端sync差140.491ms不是无生产包证明。原按压5649在原frame时间年龄82.655ms，Native观察时110.413ms，已超过100ms；不续时或重放。CPU/GPU调度原因未证明，不能归咎于Frag模型或生产断流。Native/入口/XR/launcher/game正常完整收尾，无强杀；71帧35.927秒编码0，三页双眼时间线亲看，完整手型/头显未验。详54300-expired-pocket-press-closeout.json。

同轮补上根源码交付缺口：仅恢复/接入5个Frag自有文件，包括已在私有候选使用的root-shaft几何/参考指姿/请求生成及发布hook；公开V4 ABI与Mount profile规范化文本一致。共享Native、主playground.cpp、CMake和公共/手动入口未编辑。root/private各38项真实请求生成/拒绝/输出不变检查通过，global公共ABI加local模块include顺序编译通过，既有mesh-follow几何检查通过；私有生产PG完整编译0，新候选冻结。首个测试fixture误把counter变化当作cosmetic模拟量拒绝，改为精确.35边界后通过，生产条件未变，首败日志保留。完整根集成构建、完整接触/全角度、Game与实体头显仍不作接受；不把私有编译说成整个根构建。详root-grenade-integration-54300/source-result.json及candidate.json。后续不原样追跑完整耗尽，准备短Frag中断/旧释放不投/新Grip恢复检查。

2026-11-11 actual19168 / creation134388188642941511，PG1bbd/Nativece9d：新短Frag中断驱动在原地面X拾取后库存未变时停止，尚无裤兜Grip、Frag准备/失追/投出；不能计为中断验证。原Frag同实例spare1保持，拾取失败原因未证。31帧15.418秒录像无错/编码0，三页双眼时间线亲看，包含沙袋接近/身体遮挡和原拾取现场；正常完整Native/入口/XR/launcher/game收尾，无强杀。原receipt未进入Frag模块所以没有case_directory，现用独立sidecar核实唯一startup result的确切Game身份后编码，不改旧receipt或伪造Frag结果。详19168-ground-pickup-failure-closeout.json。

短检查沿Smoke已有tracking契约但用原Frag路线，0投出/最多2抓取；8模块dry编译与45项结果消费正反检查通过。针对19168暴露的路线观测薄弱点，独立适配器最终接近改用实际当前掉落物位置、真实走到35单位内，并在一次原X前记录水平/高度距离、原物件弹量、前后选中武器；不改数量/坐标，不重复X，不续时或重放过期输入。新适配尚未Game验证，不能说旧失败由缓存位置导致或已修复。启动前接入共享birth-stage发布窄修复的当前SHA核实，未改共享入口、未为该入口另跑游戏或复用旧名字。

资源清理：身份逐一核实34个本会话已结束私有实例，删除其可重建PSOCache1282文件/10620591882字节，0错误。共享缓存、存档、配置、源码、日志、媒体和冻结模块未触及。详owned-terminal-pso-plan-0518.json/result。

2026-11-11 052607首次请求：600秒ResourceUnavailable在first_admission probe中超时，归档SHA一致的入口源码仅在commit内Popen；没有commit创建的worker.log/worker.json，也无launcher/game回执。该名字已消耗，不复用，不计Game或Native生命周期通过。详052607-resource-timeout-closeout.json。

短中断契约修正：0投出/2抓取只要求实际非空原库存，无须先地面补满4枚。现直接在原World/Pawn绑定后调用Frag检查，原Grip/失追/旧释放/新Grip验证与数量不变均保留；inventory_baseline来自第一次原抓取后的双读typed计数，允许1..4并要求全程一致。50项消费器正反检查、8模块dry编译通过；完整四颗耗尽和地面拾取范围未因此接受。新任务59426等待首次prepare前资源，无重复旧请求。共享新预启动NoStart源已按新SHA纳入核对；parent仅保留canonical no_game_started字段，不把unknown owned_game_absent改绿。

独立物理开发预览已生成：J:/gearsVr/candidates/frag-grip-2kg-20261111-1bbd-preview/GearsVR Launcher.exe，PG1bbd/Nativece9d，physical XR a18b保持，6个输入/追踪/公共Frame与Mount接口规范化文本同物理基线。753文件、约15.7MB ZIP，依赖/清单预检及全成员大小/SHA/CRC回读通过。包内采用原sixdof_manual物理输入边界，不含模拟器；打包没有启动Game/XR/GUI，公共v71和旧预览保持。README/metadata明确本版完整Game/最终贴合/声音/实体头显未接受，旧30828两投和68512四投只按相应版本证据保留。Readme文字更正时首次预检因输出文件exclusive-create已有而拒绝发布，原报告/初始ZIP保留，最终报告使用新路径并完成全量回读；没有依赖失败或假造Game通过。详analysis/frag-grip-20261108/manual-preview-1bbd-delivery.json。

2026-11-11 59426终态：原库存短检查在prepare前完整600秒仍为既有两实例占满，require_memory超时；没有分配名字/prepare/worker/Game。没有本方游戏任务在运行，不反复提交同一资源拒绝。新的1bbd独立物理预览已交付并请求3点实体反馈；未收到的反馈不作通过，等待不阻止独立源码工作。


## 2026-11-11 破片后续集成差异与失追实证补齐

本轮只读准入查询通过实际worker PID及creation身份确认，hb-cloak-1111-052600与lancer-natural-v139-225928仍占满两实例名额；未提交新启动，未触碰现有进程。上轮范围文档及检查点保存属于进展；本轮取得新的源码级反例并改进定向驱动，不以等待代替进展。

用同一现有真实reference/local/component骨骼夹具和相同controlled-wrist compose_common测试，对当前工作树头文件闭包与当前预览ce9d冻结闭包做独立只读副本对照。两者MSVC /W4 /WX编译成功；当前根源码678检查中15项专属Frag指骨旋转失败，ce9d冻结源码678项全过。根源码当前authored_main只选择held_lancer，且C0的authored target调用也仅在held_lancer分支；不据此推断当前ce9d预览失败。源码快照与结果保留在smoke-phase1/frag-c0-integration-1111；没有修改共享Native或新增Native观察器/写者。此为后续集成缺口，不能直接把当前根Native重编成等价预览。完整实机接触仍未通过。最初误选本机没有的VS17生成器，配置失败保留；随后明确使用实际VS18 E:/vs，两检查目标编译成功。

现有零投掷/至多两抓取的Frag失追驱动新增during-loss只读Source1快照，要求真实追踪失效、同Grip边沿计数、同会话/空间/输入/配置；恢复快照必须真实追踪有效，四次序号/同步时间递增并绑定相同publisher。结果判定器拒绝缺失失追证据、未实际失追、错publisher、旧快照、错误边沿和NaN等无效squeeze。122个聚焦正反例及8个实际适配模块dry-run通过，零输入/零Game启动。旧三快照驱动与检查记录保留在tracking-loss-witness-before；现有历史结果不重判为新检查通过。下一步资源实际空闲后跑这一个定向中断检查，再根据真实结果推进当前破片重复/耗尽流程；最终精细GrabPose仍在完整生产流程之后。


## 2026-11-11 独立C0指骨选择增量验证

在上轮当前根源码的独立副本中，只给authored_main增加held_frag分支，原Lancer及左手部分选择保持。相同实际compose_common／真实骨骼夹具／受控wrist检查下，Frag由678检查15失败变为678检查0失败；同样受控姿态的Lancer对照在修补前后各679检查0失败。没有测试原游戏Lancer行为，也没有因此宣称完整Lancer通过。原根源码闭包哈希复核未变，ce9d成品哈希未变；没有编辑共享Native、stage、启动Game或增加Native诊断。

可审阅最小补丁：smoke-phase1/frag-c0-integration-1111/repair-candidate/frag-finger-consumer.patch。只修补消费者选择；当前根full_body的authored target读取仍只对held_lancer调用，完整Frag Native集成尚未完成，不能直接使用此单文件补丁宣称Game修复。保持当前已交付ce9d预览，生产流程/体验/最终GrabPose顺序不变。

首次Lancer测试适配错误地全文件替换Shape::grenade，连原先的grenade-vs-rifle区分测试也改成weapon，导致前后各相同6项fixture失败；该源文件、二进制、输出已保留first-invalid-lancer-fixture。修正为只改变受控compose尾部的测试mount及预期fallback，生产修补不变，重新编译运行得到上述结果。完整记录frag-consumer-repair-1111.json和result-final.json。

本轮准入仍为hb-cloak-1111-052600与lancer-natural-v139-225928两个真实worker占用；未重复提交启动。下一实际运行仍是已经准备的零投掷、真实Source1失追／同Grip恢复／新Grip检查；不以新增离线项替代该Game证据。


## 2026-11-11 独立三文件集成补丁：目标读取与持续Grip所有权

延续上轮独立消费者修补，本轮在当前根源码只读副本中补齐Frag的authored target入口，保留原Lancer分支逐字不动；新else-if只在held_frag、已有eye/BodyTick输入下，当前焦点、Source焦点、持续Grip和有效追踪均通过后读取现有适配器。只绑定当前模型右手15个参考旋转、只接right_main_fingers，不消费Lancer左手/支撑/part输出，不改变腕部、骨长、投掷或输入许可。现有owner回调另对Frag限制right/main/no-trigger-endpoint、原请求焦点/Grip/追踪；仍走原policy的实际actor/世界代身份关闭核对，不将Frame更新或过期动作复活。

直接提取未改根分支与修补后的实际dispatch源码，配合未改生产authored_grasp_live.inl、真实Frag request生成和模型参考匹配执行，注入state/Core/准入边界：162检查0失败。原分支确实不读Frag，修补分支读当前15旋转，旧known每tick清除；失焦/失追/输入关闭/无eye/过期/旧sequence/epoch或松手边沿/错对象/错模型/拒绝owner均不选择。实际owner callback配合注入admitted actor/mount及原Lancer专属owner边界：46检查0失败，拒绝Frag食指端点等错误请求，保留Lancer toggle握持及原loading/starter所有权判断。它们不证明实际Game身份或C0写入；MSVC /W4 /WX编译的是聚焦检查，不是完整Native DLL。首次owner fixture误用ObjectIdentity.index导致C2039，改为实际registry_index；生产补丁未因此改动，首败源与说明保留。

完整可审阅补丁在smoke-phase1/frag-c0-integration-1111/target-repair/frag-authored-integration.patch，含body_c0_hands.h、full_body_c0_native_live.inl、authored_grasp_owner_live.inl三处增量；共享根Native源码、现有ce9d预览及Public均未改变，没有新增诊断/钩子/写者或Game运行。当前根源码与初始哈希一致。仍需后续合适的Native集成编译及实际验证，不能把源码补丁和208个定向检查称作整体验收。末次准入仍确认相同两worker占满，不提交新启动；下一Game仍是已经准备好的零投掷失追检查。详frag-target-owner-repair-1111.json、target-repair/result-final.json。


2026-11-11 11:48 新实例grenade-smoke-1111-114820-frag经正式准入启动actual46956/creation134388425286563600（PG1bbd/Nativece9d）。旧worker资源由用户另行明确授权结束后已释放，不将其记为原正常生命周期。本局场景玩家/相机就绪后Native激活在anchor_startup_guard initial_binding拒绝，attempts1/全部stamp0；尚未进入严格timestamp校验，不据此诊断为时钟超限，具体binding失败字段未知。零手雷operator/抓取/失追/发射证据。原运行器未重试激活；terminal launcher1、Game与owned children均absent、forced_termination=false；PG shutdown0/entry恢复及prime entry_restore有证明，但Native_cleanup_verified=false、XR exit1，生命周期整体不通过。1164次只读前台采样无owned_foreground，不证明采样间隙从未抢焦。父result的game_started仍false来自startup-ready前赋值位置，真实Game身份及canonical no_game_started=false单独保存，不把本局称为未启动Game。详smoke-phase1/46956-startup-initial-binding-closeout.json。下一步检查冻结证书初始pawn binding，不放宽保护或原样重跑。


2026-11-11 actual17676/creation134388431092293652，PG1bbd/诊断Native71058a0f：零投掷失追专项7项通过，原库存2始终不变，Source1四快照右手flags15→0→15→15；同Grip恢复保持原边沿计数，原mode8/四状态中立，旧松手零发射/扣量，新Grip再次准备。失追期间已停止。实际捕获cleanup anchor，但negative fallback和current-parent分支均未实到，不称旧取消首败已因果修复。Native/入口/XR/launcher/Game正常收尾，exit0、无强杀，928次前台采样无owned_foreground。42帧21.035秒双眼录像无errors、编码0，3页全跨度总览已逐页亲看；中段下方可见手/手雷，选样无显著蓝弧，不接受完整接触/全角度/连续画面或头显验收。此前46956初始binding拒绝未重现；只增加诊断不可能据此称修复。诊断候选冻结保留，Frag普通计划已恢复原ce9d/1bbd，手动/Public未动。详smoke-phase1/17676-frag-tracking-held-closeout.json。下一步继续当前Frag重复/耗尽完整流程，不重复已通过同一失追用例。


2026-11-11 actual46584/creation134388435149596044，普通PG1bbd/Nativece9d有限库存：采用原有2枚库存的直接有限流程，保留2..4逐发/耗尽/切枪/空槽全部断言，未写库存；地面拾取仍独立待验。首次once投出2→1，生成/扣量/通知/初始化/原点/初速匹配各一次。第二枚准备后、实际松手前sequence4078安全取消reason4/history_count8/boundary_flags0/sample_age46.4897ms，零第二发，最终preparation_owed0；reason4对应history末输入匹配或motion_history_valid，具体哪项尚未证明，不能归因为motion epoch或先前gap。finite整体失败，耗尽/切枪/空槽未达。Native/入口/XR/launcher/Game正常收尾，均0/无强杀；72帧36.713秒双眼录像编码0，三页全跨度已亲看，首枚爆炸可见，完整接触未验收。下一步分析保存Source1 ring及冻结history验证，不原样重跑。详smoke-phase1/46584-finite-history-refusal-closeout.json。


actual46584后续只读审计：实际Source1序号4071..4078完整，4077→4078 sync间隔63.6937ms、predicted间隔63.5735ms，均超过冻结grenade_throw.h的60ms；motion_pair任一超过即可拒绝，因此这一个实际时间条件已足以触发reason4，不能据此归因于Frame组装不一致。观察器缺aim/部分packet字段及Native tracking epochs，不声称完整PG历史字节重放或排除其他同时失败。调度根因未证，不放宽60ms、不补造样本、不恢复旧Grip。下一步分析producer节奏与可重新抓取的安全中断行为，未再启动Game。详smoke-phase1/46584-source-history-gap-audit.json。


actual35844取消路径补充审计：实际日志frag_cleanup_incarnation serial3/hold1/weapon162c69d030，terminal_mode8_neutral1/old_epoch_unchanged1，随后gate10/preparation_owed0，说明既有负向anchor清理后备分支确已Game实到；此前17676未走该分支的结论保持。该日志只在原mode8/四中立双读、同步fence和anchor退休后输出，不是仅Engine调用返回成功；不推广为所有旧取消首败已因果修复。本次gate10源于unavailable保留失败后取消，与46584的模型history reason4路径不同；具体入口/保留哪项拒绝未留，不能擅称lifetime failure3。下一增量仅对已有准备保留路径记录原有lifetime失败输出/拒绝组，保留读次数和短路顺序；负向anchor不可用于积极保留或新动作许可。详smoke-phase1/35844-negative-cleanup-path-audit.json。


2026-11-11 actual39356/creation134388460952125538，诊断PG1df5/Nativece9d/诊断XR603fc373：当前有限流程9项通过，原实际库存2→1→0，两枚不同原弹体逐枚once生成/扣量/通知/初始化/原点初速匹配。最后原切到Weap_LocustAssaultRifle，空槽Grip一次原cue dispatch被接受，零额外弹体，前后当前原枪同一身份；静音条件下不接受实体可听性。此次无preparation retention refusal，不称35844/46584间歇首败已修复；历史失败保留。Native/入口/XR/launcher/Game正常0收尾，无强杀。76帧38.450秒双眼录像无errors/编码0，三页全跨度亲看，两次爆炸可见，末段空手；全角度贴合/腕袖/头显仍未验收。普通计划恢复，诊断候选独立保留，未改手动/Public。详smoke-phase1/39356-finite-depletion-closeout.json。下一步不重复相同finite，继续剩余Frag实际运动/轻放/近墙/贴附与中断体验要求，随后Smoke，完整目标不缩减。


2026-11-11 actual9628/creation134388465074555463，诊断PG1df5/Nativece9d/XR603fc373：单枚静止松Grip7项通过，实际原初速向量(-2.30498e-13,-1.15249e-13,-9.21991e-13)游戏单位/秒，原初始位置/速度匹配，各一次生成/扣量/通知/初始化，库存2→1，原返回、松手无额外投掷。未注入速度或伪装无效追踪；Source1模拟非实体追踪。原Native/入口/XR/launcher/Game正常0退出、无强杀。27帧13.338秒双眼录像编码0，两页全跨度亲看，手雷/链条下方可见及末样本爆炸，不计详细伤害/引信或全角度贴合。普通计划恢复、手动/Public未改；先前间歇失败保留。详smoke-phase1/9628-stationary-release-closeout.json。下一项继续不同方向/转头/身体位移/近墙/独立贴附和中断体验，不重跑相同静止用例。


2026-11-11 actual32676/creation134388468643485154，PG1df5/Nativece9d/XR603fc373：独立上抛7项通过，原弹体初速(-2.288e-13,-1.144e-13,168.688843)游戏单位/秒，竖直比例约1；原释放位置/速度匹配、once生成/扣量/通知/初始化，库存2→1，原返回且无额外投掷。驱动3m/s上限/20m/s²不是实测值。Native/入口/XR/launcher/Game正常0收尾，无强杀。28帧13.859秒双眼录像编码0，两页全跨度亲看，手雷/链条及上举可见，不证明连续轨迹/全角度贴合。没有转头/身体位移，不据此验收不随头瞄准或移动继承，也不计伤害/引信/贴附。普通计划恢复，手动/Public未改。详smoke-phase1/32676-upward-release-closeout.json。后续不重复相同上抛/静止/finite。


2026-11-11 actual18948/creation134388473412944988转头定向检查：operator0，原弹体一次2→1，实际初速接近数值零，原返回/无追加投掷等动作断言通过；最终case=false，因为头部证据缺失。释放Source1 seq3986，但CSV邻近仅3964/4009；冻结XR源码连续会话首240帧后降为每秒一行，无法证明释放及相邻帧真实头部运动。原严格join未放宽，不将发过头部命令或视频视角转动计为精确头部独立性通过，也不判产品失败。Native/入口/XR/launcher/Game正常0退出，无强杀。30帧14.885秒双眼编码0，两页全跨度亲看，末段转头可见，不替代逐帧证据或贴合。普通计划恢复、手动/Public未改。下一增量仅修观察覆盖：独立Source1固定容量头部采样journal，退出导出，不增加逐帧CSV I/O或改输入/渲染保护。详smoke-phase1/18948-headturn-evidence-gap-closeout.json。


2026-11-11 actual54492/creation134388478068494781，PG1df5/Nativece9d/密集头部XRa08af7c5：静止右手转头释放专项通过。释放Source1 seq3926与前后帧同会话/预测时间准确join，头部三帧变化0.022216rad（1.273°），右手位置/姿态保持，执行晚于真实松手57.796ms；原弹体初速(2.29677e-13,-1.14839e-13,0)，once生成/扣量/初始化、2→1、原返回/无额外投掷。1024密集记录导出，无插值或放宽关联。仅证明静止右手未从头动获得额外速度，不计非零投向/身体移动继承。此前18948仍为证据不足。Native/入口/XR/launcher/Game正常0收尾，无强杀；27帧13.369秒双眼编码0，两页全跨度亲看，转头/爆炸可见，不接受完整贴合/实体头显。普通计划恢复，手动/Public未改。详smoke-phase1/54492-headturn-release-closeout.json。


2026-11-11 actual12900/creation134388481619884882，PG1df5/Nativece9d/XRa08af7c5：上抛同时yaw/pitch转头检查通过。释放seq3715及前后帧准确join，原始右手向上跨帧位移0.0237144m，头部角变0.0213576rad、俯仰变0.0133420rad，执行晚于实际释放67.0595ms；原弹体初速(2.27233e-13,1.11234e-20,168.197922)游戏单位/秒，horizontal/vertical约1.35e-15<.02，once原生成/扣量/初始化/初始状态匹配，2→1，原返回/无追加。standing body，不能据此接受walking或身体速度继承、任意方向、近墙、贴附。Native/入口/XR/launcher/Game正常0退出、无强杀；29帧14.367秒双眼编码0，两页全跨度亲看，不接受完整贴合/实体头显。普通计划恢复，手动/Public未改。详smoke-phase1/12900-moving-headturn-release-closeout.json。


2026-11-11 Frag身体基准边界修正（源码/离线，尚无本补丁Game验证）：旧模型独立复现旋转基准继续旧轨迹、平移后历史松手仍套当前原点两项失败。现保留当前认证身体平移，仅以原始XR手轨迹拟合手速、认证角色速度叠加一次；游戏基准旋转中断旧轨迹，已平移且缺释放时刻身体原点的历史松手取消且不扣量。原boundary bits1..32不变，64历史原点不可认证、128旋转；2181项模型检查通过，私有PG编译链接0/SHA256 574cf90698dfec604eeb5dbb30726e591adaca5d002ea1c9cc77eaf44c706433。本次未启动Game/XR，未改Native、公开或手动候选；移动投掷、完整Frag/Smoke及头显仍未验收。范围保持先完成破片再开发烟雾。证据analysis/frag-grip-20261108/smoke-phase1/frag-body-origin/source-build-result.json。


2026-11-11 身体原点限制更正：39512在首次接管处64.4255ms>60ms，未准备/投出；46700（PG6506b122）准备及实际移动松Grip已到达，但新增flag64因current3960/release3959而拒绝，零生成/扣量。两轮Native/入口/XR/Game正常0，29/34帧双眼编码0，各两页全跨度亲看。首次接管现仅在无hold/token/准备债务时，针对同一合法cohort/pose/当前装备及60..100ms输入等待独立新鲜帧，60ms准备/投掷门槛及已准备中断不变；2236检查通过，尚无live等待分支证明。

本人重新核对implementation-handoff.md第14行及ThrowPlan协议：此前要求每个XR历史采样具备精确历史游戏身体原点，是额外限制，会阻断正常移动投掷，撤回该限制正确性的旧结论。Frag现以真实producer释放姿态/手速，通过当前认证Game身体变换合成一次原点/基础速度，保留同一参考/运动代、旋转边界、时效/身份/防重放；不伪造过去Game原点。2242检查、PG92f8e16e编译0；本补丁Game验证待独立运行结果，Smoke旧限制仍待破片后同规则修正。全部历史首败保留，完整目标/头显未完成。详smoke-phase1/frag-body-origin/current-game-frame-mapping-correction.json。


2026-11-11 actual14668/creation134388496975167718（PG92f8e16e/Nativece9d/密集头XRa08）移动释放专项7项通过。真实摇杆移动使Pawn位移236.7168游戏单位；实际释放seq4759±1原始右手位置/姿态完全相同、有效flags15，左stickY=-.35仍持续；模型手速约1e-14m/s，认证角色基础速度(-224.999130249,0,0)，实际原弹体初速(-224.99913,-1.146e-13,-9.168e-13)，差约2.49e-7游戏单位/秒，未重复合成。执行晚于实际释放57.9554ms，once生成/扣量/通知/初始化/原点初速匹配，库存2→1，原返回且无额外投掷。Native/入口/XR/launcher/game正常0，无强杀，监测未见本方前台；28帧13.8707秒双眼编码0，两页全跨度亲看。
本次release/current均4759，不能说历史边沿移动分支live通过；初次延迟等待分支亦无直接live标记，不声称39512原因全面修复。手速非零+身体、近墙、独立贴附、完整中断/皮肤贴合及Smoke自身证据/头显仍待完成。普通候选计划恢复，手动/Public未改。详smoke-phase1/14668-moving-body-release-closeout.json。


2026-11-11 actual1724/creation134388500289018027（PG92f8e16e/Nativece9d/XRa08）真实上甩＋后退组合8项通过。释放seq3704±1原始右手竖直位移0.04013359546661377m，左stick持续-.35；认证手速Y1.6844871889m/s、基础速度X-224.999160767游戏单位/秒，原弹体实际(-224.999161,-5.718e-14,168.448715)，减基础速度后的模长与实测手速×100差3.89e-6游戏单位/秒。once生成/扣量/通知/初始化、2→1、原返回/无追加；Native/入口/XR/launcher/game正常0，无强杀。30帧14.875秒编码0，两页全跨度亲看。release=current3704，历史移动边沿仍未live覆盖；完整贴合/近墙/贴附/Smoke/头显未完成。详smoke-phase1/1724-hand-body-release-closeout.json。


2026-11-11 actual20088/creation134388502979140946，PG92f8/Nativece9d/XRa08：独立原B近战实际生成认证Proj_FragGrenade附着在真实目标Mesh/Attachment/Base/MeleeVictim上，库存2→1，零普通VR投掷；旧reader仅接受gate14，因此在实际pre_original_tick的gate60/event9/stage7/fault0/owed0路径错误中止，后续原引信属性观察未完成，整项FAIL保留。源码证明gate60是原B前安全停止，不是放宽旧债务；本轮仅改私有reader，须本次B后新的同binding唯一60或14，61拒绝，14消费检查与8模块dry-run通过，正常生产二进制不变。Native/入口/XR/launcher/game正常0、无强杀；38帧18.971秒双眼编码0，两页全跨度亲看，不据图接受完整贴附计时。详smoke-phase1/20088-tag-reader-retirement-miss-closeout.json。


2026-11-11 actual55652/creation134388505481039086（同PG92f8/Nativece9d/XRa08）reader修正复核仍FAIL：retention reason1/lifetime_failure3/frame605/lastinput3612/observed161809946050200，随后负向anchor3/mode8neutral/旧epoch未更改，gate10/event9/stage7/fault0/owed0；此取消早于原B press161812167223000达2.2211728秒，不能用旧gate10充当本次B停止。原B仍实际认证附着Proj_FragGrenade、库存2→1、零普通VR投掷；引信属性观察因assert未继续，不接受完整贴附。
精确编译freeze内Client头SHA4834fa5b701a150f5cffddd6fb15a975d7d9102206b9204686c25bfbaf7822b2与当前根不同，已核对其failure3为!active(oldEpoch)，可能phase/terminal/epoch，不声称已唯一证明epoch推进或producer根因。邻近Source1 seq3611..3615持续focus/flags15/Grip1/同代，只是输入证据，不能替代Native action lifetime。Native/入口/XR/launcher/game正常0、无强杀；49帧24.588秒双眼编码0，两页全跨度亲看。下一步按精确freeze定位preparation生命周期撤销原因，不放宽复活旧动作、不重复未改tag。详smoke-phase1/55652-tag-pre-B-lifetime-refusal-closeout.json。


2026-11-11 lifetime3窄观察已实现并私有编译0：PG53e397e1/Nativece9d/XRa08，相对实测92f8唯一编译输入差异weapon_frag_runtime.inl，所有动作/返回/许可谓词inverse完整还原。现有最多8条准备保持失败记录补出client_phase、已保存disarm_reason、registration_alive、原动作epoch和当前metadata_generation；只读已有客户端状态，无新Engine读取、时钟查询、writer或Native修改。256/257分别Native context/input_lost，258..260为终结类，>=512须按精确freeze行号；未实际运行新观察，不称找到了55652根因。私有tag reader现在即使退休证明缺失也先保存first_failure，继续合法原贴附后的有界引信观察，末尾仍assert退休证明，完整FAIL不被抹掉；8模块dry-run通过。
14:10:31一次协调让行期间无本方存活game/XR/worker，构建已结束，暂不新增启动，最迟14:20:31自动恢复原独立准入；不改全局锁/并发上限。详smoke-phase1/frag-lifetime3-observation/source-result.json。


2026-11-11 actual50832/creation134388513581832473，诊断PG53e397/Nativece9d/XRa08：取雷前普通步行首败，未触达准备/B/投掷或新lifetime3观察。原路径点(-3725,0,651)距角色8.9978raw，角色实际(-3716.0022,0,651)，距目标(-3650,0,651)66.0022raw已在原60..82范围，却受人工路径点arrival5约束，三个无推进小步后正常stall中止。先拟70/arrival12方案在未Game前撤回；私有tag driver现复用navigate原until回调，仅实际60..82且Z差<=10时结束，原waypoint75/arrival5及末尾同范围/高度检查保留，无传送或生产改动。8模块dry-run0；Native/入口/XR/launcher/game正常0，无强杀；33帧16.412秒双眼编码0，两页全跨度亲看。详smoke-phase1/50832-tag-waypoint-stall-closeout.json。


2026-11-11 actual37736/creation134388516510393054（PG53e397e1/Nativece9d/XRa08）原独立B贴附3项通过。正常步行达原60..82范围，原B时Grip持续按住；新同binding gate60/event9/stage7/fault0/owed0，随后认证原Proj_FragGrenade的Mesh Attachment/Base/MeleeVictim、库存2→1。真实Grip在附着观察后释放，零普通VR launch；100次有界属性读中先nonexploded/simulating0/bounce0/base同目标，再原exploded1，剩余游戏时间1.9650917秒（不是完整2.5秒）。目标Health200→-200，玩家450→-195；原爆炸后失去输入许可即停止后续脚本输入，只读观察继续，未伪造恢复。Native/入口/XR/launcher/game正常0，无强杀；36帧17.922秒双眼编码0，两页全跨度亲看，爆炸/死亡菜单可见，不接受完整贴合或头显。新lifetime3诊断本轮未触发，不声称55652根因修复；旧20088/55652/50832失败保留。普通计划恢复、手动/Public未改。详smoke-phase1/37736-original-tag-fuse-closeout.json。


2026-11-11 actual36944/creation134388520649694716，PG53e397/Nativece9d/XRa08：12秒持续握持专项8项通过。61次观察覆盖12.6189墙钟/12.5951游戏秒，spinning/targeting持续TRUE，库存2且零VR launch；随后普通真实移动释放once2→1，基础速度只合成一次、原返回/无追加。无retention refusal，不能据成功证明55652原因已修。Native/入口/XR/launcher/game正常0、无强杀；52帧26.162秒双眼编码0，3页全跨度亲看；释放附近模型不能仅凭缩略图辨别是实际弹体还是补雷，完整贴合/显示过渡未验收。详smoke-phase1/36944-long-hold-release-closeout.json。

actual39012/creation134388524299945291同候选近墙FAIL：旧4Trace选中x=-4330的BlockingVolume31（5extent），实际一次VR投掷库存2→1，出生x=-4328.30469近侧1.6953、速度朝墙-67.3047；原LastHitLocation落在x约-4348/Z513并最终原爆炸，未观察所选平面命中，不接受墙/特殊分支。此为误复用旧几何入口，既有60288已保存0/5extent墙不同，未改碰撞体。现私有driver恢复既有8Trace typed HitInfo组件/BlockZeroExtent过滤，10模块dry-run通过，首dry-run因执行期绑定写在纯compile路径而失败已修为正常exec时绑定，无Game由该脚本错误启动。Bridge正常close、无pending；Native/入口/XR/launcher/game正常0，无强杀；56帧28.186秒编码0，媒体待本轮后续亲审。详smoke-phase1/39012-near-wall-obsolete-geometry-closeout.json。


2026-11-11 actual21584/creation134388528310018620（PG53e397/Nativece9d/XRa08）近墙基础4项通过，原特殊分支仍FALSE。8次原Trace/typed HitInfo认证零extent mp_warmachine_SMC11/StaticMeshComponent186901及原Owner，CollideActors/BlockActors/BlockZeroExtent/BlockNonZeroExtent均TRUE，选墙x=-4350.2461；一次真实松Grip投出，once生成/扣量/通知/初始化、2→1，无额外VR投掷。实际出生近侧20.1914raw、速度朝墙-67.8428；45次原LastHitLocation在所选平面8raw内，先bounce/physics6/nonexploded再physics0/bounce0/exploded。
原始请求出生(-4348.12783,-27.53933,627.15359)与实际(-4330.05469,-21.29342,627.15356)不同：已核对精确freeze的collision_plan在Spawn前保留原pawnXY→raw5unit短体积Trace，journal.plan先调整，再由read_launch对该计划验初始状态；不是生成后移动弹体，不能把日志planned_position误读成已调计划。Bridge正常close/无pending；Native/入口/XR/launcher/game正常0，无强杀；57帧28.760秒双眼编码0，3页全跨度亲看。未记录specialHitWall/普通StopBounce具体callsite，不以本基础通过替代原特殊路径、全角度贴合、完整中断/Smoke/头显；55652原因仍未修复证明。详smoke-phase1/21584-near-wall-contact-closeout.json。


2026-11-11 原特殊HitWall静态复核：4份原导出SHA匹配，非零Physics＋LastHitLocation严格等于Location＋Authority，非sim分支才直接BeginExplosionSequence(0,TimeAfterBeep)，否则普通HandleCollision；既有21584的45个样本无前两条件同时成立，不证明瞬间未发生。当前REA无UnrealScript解码入口，复核使用项目已保存原导出，不声称新native反编译。旧8记录被动CallFunction观察器复用到私有PG eac5e98c，仅frag_launch一个编译输入区别于实测53e397；移除观察块＋6调用后原非空生产语句完全恢复，默认OFF、新Native钩子/游戏写入0，不用于手动交付。

actual4368/creation134388540292663564定向观察FAIL，投前因旧许可失效取消，零launch/扣量，观察器未arm，不能计任何原碰撞call分支。失败reason1/lifetime3，clientphase2、registration_alive1、disarm_reason257=最近Native input_lost，oldaction112016/currentmetadata112619。真实Source1 seq4071→4072 sync84.1875ms/predicted84.0829ms，唯一跨样本EndFrame83.8365ms、WaitFrame0.0064ms；取消时旧4071年龄111.507ms，raw4073已新鲜5.202ms，不能用raw替代Native认证Frame或复活旧许可。精确Native内部input_lost分支/所有中间撤销序列仍未知，不把此一局归因扩到全部历史。Native/入口/XR/launcher/game正常0，无强杀；64帧36.687秒双眼编码0，2页全跨度亲看。详smoke-phase1/4368-input-lost-cadence-closeout.json。

独立实体预览frag-grip-2kg-20261111-53e3-preview已交付，普通PG53e397/Nativece9d/物理XRa18，无碰撞观察器或模拟器；753成员依赖/PE预检及ZIP逐项SHA/CRC通过，SHA7dd4529f24e3ce79187d94f28244d6efd150ed587820aad5aeba9e19a00e9286。首包元数据未转义中文遇嵌入Python默认GBK解码失败，保留失败日志后改为ASCII转义JSON并完成最终预检，未改产品parser/运行游戏。已异步请求3项物理控制器握持/轻放再投/Stop的文字反馈，尚未收到；不计头显或整份Frag通过。旧预览/Public入口保留。详analysis/frag-grip-20261108/manual-preview-53e3-delivery.json。


2026-11-11 当前A4手型来源与静态表面复核：已从全部历史pose中找到thumb-mount-fit/pose.json，与当前生产weapon_frag_grasp_pose.h的15个closed关节、mesh中心、手腕基准和握点逐值完全相同；重新按原固定骨长FK蒙皮，863个手部顶点最大误差4.2633e-14。1395个原LOD0手部三角面静态检查为29对手部自身相交、0对柄部/Counter相交（仅非共面相交线段>0.1原单位，排除焊接邻面）。因此不能把较早proposal差异当成当前源码缺陷，也不能以柄部零相交宣称完整手型通过；动态链条、袖口、全臂、各角度实际画面和头显仍未接受。ce9d Native草案目录虽然留有旧A1头文件，但387项实际编译输入不含Frag grasp头文件，不构成已编译Native使用旧手型的证据。本次只复核保存数据和当前源码，没有改产品、编译或启动Game。证据analysis/frag-grip-20261108/frag-fit-current-20261111/current-A4-static-surface-audit.json。范围顺序继续为本会话先完成破片，再开发烟雾，不把两者历史结果互相继承。


2026-11-11 A5私有手型候选：当前A4的29对原相交中，11对在原六视角双面遮挡检查至少有采样点可见。对完整1395面而非旧跨手指子集做协调求解，第一候选消除23对旧相交但引入12对新相交，总18；把新相交也加入约束后总15对、柄/Counter为0，六视角可见采样对5，五个原远端骨三角接触标记最大位移0.02097raw（约0.21mm），握点与骨长固定。标记不是已认证指腹，剩余相交未接受；另一次直接逼近柄面标记的尝试产生90对自身相交和4对柄部相交，明确拒绝。三组旧/新六视图已亲看，未将缩小计数当完整贴合。
只在独立冻结源码中把15关节和profile A4→A5编译，GearsPlayground配置/链接exit0；PG32e456124f2432b59dcd947390468c8c72bf06c743e66a6dfaa19ff10c7f5423，实际编译输入相对普通53e397仅weapon_frag_grasp_pose.h一项改变，57个GEARS选项及9项Release编译设置一致，实际CL读取了A5头文件。根生产头83d34ab1保持，普通53e397手动预览与当前运行计划保持，Native/XR未重编、未启动Game、未发布此A5。完整Frag/Smoke/头显仍未完成。下一步对私有A5做当前真实Source1持续握持多腕角度的实际双路画面检查，再决定剩余折痕和链条/袖口修正；不重复已过投掷或普通墙碰撞。证据analysis/frag-grip-20261108/frag-fit-current-20261111/fold-contact-closeout.json。


2026-11-11 A5实际59384/creation134388573920257453：PG32e45612/Nativece9d/XRa08，当前隔离config实际render_scale0.3，明确不记为0.4规范验收。Source1仅一次真实Grip/零投掷；正面、拇指侧、四指侧三段完成，45个spinning/targeting真样本、库存一直2，第四中心返回时取消，operator1/casefalse。实际reason7/lifetime_failure0，client_phase2、disarm_reason256、old/current epoch均30817。5182→5183 sync间隔102.7315ms，前EndFrame0.6467ms、WaitFrame0.0097ms；拒绝时旧5182龄140.6379ms，而raw5185龄1.4422ms且实际Grip pressed1/released0/focused1/flags15。raw新样本不证明Native当前Frame合格，reason7包含多个条件，具体失败谓词未证实，不说节奏问题已修复、不延长60/100ms、不重放旧Grip。
79帧39.954秒编码0，三张全跨度paired总览与实际帧51/57/63/66/68已亲看。正面可见手握柄；两侧袖口遮挡仍大，链条/弹体随转腕改变可见，但全角度自然手型、动态净空、残余15对源相交和头显仍未接受；帧63过渡重影不作根因诊断。Native/入口恢复正常，XR0、launcher0、Game/children正常退出无强杀、owned身份均不存活；1029次前台观察无owned窗口，亚采样空档仍不作保证。
已在本会话Frag模块补reason7失败专属10位掩码和原已读时间/输入/Grip边沿值，最多8行；原accepted表达式、原短路ExistingActionLifetime及返回逐字保留，逆移除日志块恢复全部旧源码，无新Engine读/时钟/动作/阈值更改。普通根手型A4保持；私有A5诊断PG04b4343b79c3c24310dcca7b401f77338ac1fc66fdee0cb7f80310ba82890320编译0/独立冻结160623，相对实际59384仅frag_runtime一项编译输入变更。尚未运行新诊断；下一私有runner显式配置0.4，不改共享配置；现有手动53e397/公共包和普通运行计划保持。
首次155051在启动前因C盘空间4,118,757,376<4,294,967,296被原磁盘门槛拒绝，无Game/worker；删除缓存与旧raw命令被自动审批blocked by policy拒绝，未执行。1123份归档压缩哈希一致但已压缩未释放空间；改为将58个已终止本会话output/1601文件完整转存J盘并建原路径junction，逐文件SHA/大小验证、旧精确进程身份重查不存活，全部材料保留，C盘恢复约9.09GB后才做上述实际检查。证据59384-held-view-cancel-closeout.json、retention-predicate-source-result.json、disk-admission-recovery.json均在analysis/frag-grip-20261108/frag-fit-current-20261111。完整目标active；Smoke仍在Frag完成后，不以这次收尾代替完整验收。


2026-11-11 实际53524/creation134388580891842670，PG04b4343b/Nativece9d/XRa08：原四腕视角完成，60个实际spinning/targeting真样本，库存2保持、零VR投掷、Source1持有段最大sync间隔30.6509ms，真实Grip pressed1/released0。新拒绝细分日志未触发，上轮59384取消未重现，不作因果修复或无限持有声明。73帧36.855秒、3张完整paired总览及51/57/64/70细节已亲看；两侧仍以袖口/手背遮挡为主，不能接受全部手指贴合。Native/入口恢复正常、XR0、launcher0、Game/children退出且无强杀，所有本方身份不存活。
同时纠正渲染参数记录：私有runner输入虽然请求0.4，但统一isolated_sim.prepare明确固定实际0.3；实际config和launcher0.30日志为准，53524不能记为0.4验收。未更改共享渲染策略。相机观察方向已按精确冻结的down20握腕基准、XR→UE轴转换和头/手位置推算并往返验证，数学计划不是实际接受。
第一轮新视角实际43092/creation134388590454907073到达掌面旋转后，在截图请求前因本方脚本g未绑定而失败；该失败保留，55帧27.717秒两张全跨度总览已看，原样正常退出。已补g=c.g；准备器默认gpu_snapshot=False，因此仅本次私有review在prepare后显式启用现有受限8次XR原分辨率readback（只请求2次），其他manifest字段、默认小窗口960×540/实际0.3和全部身份/焦点/时效/后台保护保持。未构建新Native/XR、无新hooks/输入源。
162937在创建Game/worker前因磁盘free3,868,332,032<4,294,967,296被原门槛拒绝，不算Game检查。确认私有PSOCache接近每例0.9GB；精确旧game/owner/XR/worker均重查不存活后，将5个本会话历史PSOCache共4,425,246,590字节/525文件完整转存J盘，逐文件SHA/大小核对并保留原路径junction，未丢弃文件/未动存档或原配置，C盘回升8.06GB。证据53524-four-held-views-closeout.json、43092-surface-driver-error-closeout.json、camera-derived-held-views.json、cache-offload-result.json。完整Frag/Smoke/头显仍未完成。


2026-11-11 原分辨率观察实际11244/creation134388595820509400：PG04b4343b/Nativece9d/XRa08，原后台小窗口/实际0.3保持；仅本实例gpu_snapshot=True，其余managed manifest字段逐值保持。原XR限8次readback实际自动保存一张2458×987、source_format28、ticket131/source_serial127、saved_ns170856061188500且resizedfalse的原SBS；已亲看，内容是口袋尝试前的空手，不当握持贴合证据。PPM转PNG只为工具支持，解码RGB SHA逐字节一致。
随后真正右Grip按压在Source1 sequence4876/sync170873770329200，前间隔27.0386ms、focused1/flags15/pressed1/released0，真实raw位置(0.23,1.035,-0.18)。原口袋获取未完成，inventory观测为空、无Frag runtime/VR投掷、新四掌面视角和两次主动截图均未到达。自动截图比按压早17.709秒，不能把后续失败归因给截图。已有Native读失败8、game_difference0、frame382/sequence4869/pre-age99.991ms发生在按压前；冻结源码定义它合并了epoch撤回、publication撤回、observe后变陈三种，缺少真实子因，不据此修复/保持旧动作。61帧30.787秒、2张全跨度paired总览和自动原SBS已亲看；全部Native/入口恢复、XR0、launcher0、Game/children正常退出无强杀，当前owned任务均terminal。
针对这个缺口只在私有冻结HandClient/EquipmentBackend添加精确首个post-read拒绝项：保留原短路先后和返回，1实际epoch不活跃、2实际publication撤回、3observe后过期；无新Engine读、时钟、动作许可、重试或input observer保持逻辑。原3条件均照旧拒绝，正读路径不改。真实HandClient回归3项（pre-age99.991ms跨100ms读取后仍拒绝/读取中epoch撤回仍拒绝/新鲜读取仍成功）通过；首个编译因旧main重命名失去隐式return0报C4715，已仅在复制fixture main加显式return，未压制生产警告。PG编译链接0，冻结dbd027a5f48c81fdf2867f16cc1e7b975974d2d5f1a806c9d212f602b64eff6f，相对实际04b4仅上述两项私有编译输入；根共享HandClient/EquipmentBackend本方未编辑，Native/XR/普通手动53e397/正常运行计划保持。新私有分类器未跑Game，非修复/非完整接受。下一点是同一真实Frag口袋获取的精确拒绝项，不盲重跑已过四视角/投掷/普通墙碰撞。证据11244-pocket-acquisition-failure-closeout.json与post-read-diagnostic-source.json。完整Frag/Smoke/头显目标继续active。


2026-11-11 最新直接用户验证指令：“你自己写脚本 视觉检查”。接下来的取雷/转腕/静止松手/挥手投掷及显示由当前会话脚本执行并亲看实际图像，后续推进不以用户戴头显反馈为下一动作；模拟输入与物理头显触感仍明确区分，不伪称物理实测，也不缩减原16组功能要求。
165156启动前再次被原4GiB磁盘门槛拒绝，无Game。首轮历史空缓存转存为0字节，未当作释放空间；随后精确进程身份均重查不活跃的10个非空本会话缓存共8,850,493,180字节完整转存J并逐项校验。新增私有archive_prepared_cache.py/.ps1，在每个新实例尚无worker/session时将独立PSOCache完整转存J、建原路径junction并核对105文件/885,049,318字节SHA/大小；165731和170211实际都已使用该缓存正常启动，避免后续每例净占C盘0.9GB。未降低磁盘/内存门槛、未改shared entry、render0.3小窗口/输入/启动/清理保护，未丢文件、未动存档配置。
165731实际59384/creation134388610683040733（不同于早期同PID案例）成功取雷与掌面旋转；首图helper因-I排除脚本目录而ModuleNotFoundError，在事件请求前失败。固定仅snapshot辅助CLI的模块导入方式，先以已结束会话验证导入成功且运行态保护仍拒绝/无事件，再进入170211实际3932/creation134388613445365735。
3932 PGdbd027a5/Nativece9d/XRa08：同一真实Grip/零trigger，四个相机几何推导方向完成，60个spinning/targeting真样本、库存2→2、零VR投掷，持有原Source1最大sync间隔44.6549ms。两次原受限GPU请求成功，原SBS2458×987/source_format28，ticket561/628、serial556/623、saved_ns172623048655500/172627115882700，均在对应稳定握持窗口；最近Source1位置误差<1e-5m、归一后角误差9.42e-8/4.21e-8rad、Grip pressed1/released0/focus1。PPM→PNG解码RGB SHA完全一致，原图与不重采样矩形提取均已亲看。
86帧43.530秒/3张全跨度paired总览已看，另看opposite_side实际frame83。前两张虽为正确原分辨率/时序，第一张前景柱状几何遮手、第二张腕袖遮挡；不能从像素单独证明柱状物骨身份或归罪mount，也不能接受完整手型。frame83反侧确实露四指与柄，已将下一脚本的两张原图改为反掌面与该反侧，仍4方向/2受限请求、同一产品/同一保护，尚未启动下一版本。源15对相交和完整视觉贴合仍未接受；拒绝分类分支本轮未触发，11244/此前取消原因仍不作修复声明。Native/入口恢复正常、XR0/launcher0/Game children正常退出无强杀，当前本方runtime/encode/cache helper均terminal。普通53e397预览和正常运行计划保持。证据3932-scripted-held-source-review.json、3932-source-images.json、reverse-view-driver-preparation.json、59384-1657-snapshot-helper-failure-closeout.json。完整Frag/Smoke目标active，下一视觉姿态后继续脚本静止松手与挥手投掷/返回。

2026-11-11 用户要求当前会话自行脚本/视觉检查后的新增结果：A5实际3168四方向持有2→2/零投掷，59帧29.752秒3页全跨度paired及两张2458×987原图均已亲看；反侧可见指腹与柄仍有间隙。私有A6在同一固定骨长、权重、腕基准与mount上，仅调整四指12个closed关节，以注册的指腹表面靠近柄；拇指不变。候选指腹中心到柄距离约1.8–4.1mm，静态自交21对、物体交0、指间交0，保留掌指褶皱5对和全部原始相交记录，数值不是视觉接受。近/远侧失败优化72/109对等均保留。PG0d54899ee628b7b5c94a0b41f2a568721682c9a852546a6eb70ac9fb99314210仅此手型编译输入相对dbd027变化，Nativece9d/XRa08及所有动作/认证保护保持。根生产手型仍A4，普通53e397预览与Public入口未替换。

A6实际29204/creation134388647777425221：两次真实释放2→1→0和原版once全部发生，静止速度近0、挥手约1.21m/s；第二次实际release4890/current4891已覆盖修正后的历史释放分支。脚本错误假定动作局部token全程唯一，两次合法token4被误判，原失败exit1保留，未到空库存反馈。仅修reader：按每次动作前mapping日志基线取增量，以release_sequence/input_sync_ns分别关联，不改生产token/反重放。52帧26.137秒3页paired+两张原图亲看；精确Source1释放边沿、同cohort、原版once、库存与执行<100ms均离线认证。见29204-two-release-source-visual-closeout.json与two-release-reader-repair.json。

修正后A6实际45524/creation134388655405869067完整通过：同一真实Grip持续持有/零trigger、静止松手、新口袋Grip后挥手松手、库存2→1→0，最后原版自动切回，第三次空槽Grip原空库存反馈dispatch accepted且无额外弹体/切换当前武器。静止release4155/current4156，挥手release4738/current4739均真实历史边沿，Grip pressed/released分别1/1、2/2，同原始session/space/profile/input epoch，动作各仅一次原spawn/debit/init/notify/return。静止速度约0，挥手1.578m/s；原始手速映射加身体速度一次与实际初速度误差<0.05worldunit/s。77帧38.935秒3页paired和两张2458×987原图均亲看，持有闭手/链条可见，松手后张开且无持有手雷残留；蓝预览检查标记通过。空槽声音在既有测试静音保护下仅接受原dispatch，不冒称可听见。见45524-two-release-source-visual-closeout.json。

A6补四角度原图实际58716/creation134388657391949447失败：完成反掌/指正面，第三侧面32个持有检查中的末样本spinning/targeting变false，库存2、零VR投掷。精确日志reason1/lifetime_failure3/client_phase2/disarm_reason257，Native最近input_lost，old_action21347/currentmetadata21954；不是手动松Grip。取消附近45个原始样本始终focus1/squeeze1/pressed1/released0，最大观测gap28.559ms，取消时最新raw4555年龄9.3881ms，last_consumed4552年龄76.1313ms。第三原GPU保存后12.9543ms记录拒绝，不能仅凭相邻时间断言截图是原因，也不能把此前84/102ms断档套用本轮。Native具体input_lost发起分支/所有中间许可撤回仍未记录，不能让raw绕过Native复活动作。3张原2458×987、52帧26.151秒3页全跨度paired均亲看，指正面能看四指弯曲包柄，无该角度明显指间交叉；反掌仍腕袖遮挡，第三角度已经可见准备撤回和短暂菜单/等待画面，不计完成第四角度/完整贴合。见58716-held-view-input-invalidation-closeout.json。

三次新增Game均Native/入口正常恢复、XR/launcher正常0、Game/owned children正常退出无强杀/无前台激活观测，所有本方runtime/encode当前terminal。实际managed render0.3（非输入请求0.4）明确记录，既有小窗/静音/物理输入隔离/identity/freshness/save/cleanup保护均保留。下一项为冻结Native input_lost来源的有界被动证据，禁止不改证据重复同一角度流程或放宽时限。完整手型、连续握持可靠性、原特殊碰撞分支及Smoke独立闭环仍未整体完成；Frag先于Smoke，2kg直接跟手/原链条要求不变。物理触感未知，但不再要求用户戴头显作为当前推进前提。完整16组要求保留，whole_grenade_goal_complete=false。
