# 固定ready输入的同entry观察与消费合同

当前唯一目标是解释HJ固定右手输入下的腕／枪姿态差异。原三phase和恢复通过继续保留；现有PG9与Native first32的精确join是0，不能靠相邻epoch继续定位。新观察必须在同一次现有骨骼entry内区分目标、实际提交、原consumer调用前与原调用返回后的结果。

逻辑合同封存在ignored `analysis/lancer-ready-same-entry-20261027/consumer-contract-v1.json`，SHA256 `b860b295ee1a35e86d89adf931260074c533a8281bf77d5b3c34e4366eaa8e32`。它是字段／坐标请求，不是提前假定的binary ABI。Body唯一实现共享Native与采集，武器会话负责合同和最小file-only消费；双方不并行新增writer／采集器或游戏。

观察独立默认OFF，仅Source1自动fixture启用。ready选择用当前Tick已复制的raw XR head／right Grip，声明full inverse(head)*grip，位置约`(.18,-.30,0)`米、姿态`(-sin(.15),0,0,cos(.15))`；1mm／1e-3rad只是测试观察识别容差，不改变游戏freshness／动作资格。raw XR数据是LOCAL、+X右／+Y上／−Z前，不能使用已映射的UE world_head替代。记录实际选择来源、cookie／owner和source标识，cookie也不能独自证明当前武器资格。

明确ready启动后，在有限64条不轮转窗口保留同entry，包括capture／plan／prepare／commit失败。资格或复制暂未知时保留UNKNOWN，不当作known-not-ready直接漏掉；明确非ready、owner／cookie／source变化或容量到达才按声明政策结束。Header分别保留unknown／not-ready／selected／drop与实际起止，64容量不等于完整phase。此选择只控制诊断，原writer／admission不依赖它，旧first32保留。

| 字段组 | 必须含义 |
| --- | --- |
| 身份／来源 | 同PID／创建时间／Native-start／GT、entry地址＋tick＋compose、mesh／asset／geometry、原source seq／session／space／publication等；entry地址单独会复用 |
| 当前目标 | 只有本entry的hands::compose实际成功才可发布computed_wrist[1]及当前planned Local；持久workspace在失败后不能冒充新目标 |
| 实际提交 | prepare／commit是否调用与真实Kernel code／receipt；计划本身不等于安装，code0默认也不代表成功 |
| consumer_pre | 原before_original／observe_hand_trace(false)，位于原调用之前；实际consumer_marked及Local读回单独有效 |
| returned_end | exact original_called且returned之后的状态、实际epoch、Local35／51／52、Space35／52与component-to-world；安全认证不足则UNKNOWN |
| scope_close | 正常finish／原异常／debt结果，不能把finish前读回的数据当成scope成功 |
| 枪体／Muzzle | 本轮不新增同步读取，明确UNKNOWN；旧PG几何不回填成Native同entry证据 |

已有hand9骨序为`8,10,11,32,34,35,51,52,55`，右腕35／weapon_r51／weapon_end52分别是槽5／6／7，优先复用已有Local before／planned／consumer-pre／after及end Space。无需增加全115矩阵。现有observe_hand_trace需要prepared；capture成功但prepare失败若另作被动读，只能由Body保留当前entry／thread／owner与metadata前后认证，不改原helper、不追加current()/solver或借旧model，无法证明安全时不读。

computed_wrist是已映射的UE game-world pose，原游戏单位、+X前／+Y右／+Z上，不再乘units_per_metre。Atom32是parent-local的float4 quaternion＋float3 translation＋float scale，不能与世界位置直接比较。Space为Avatar component空间；16float矩阵按row-major／行向量，`world = Space * component_to_world`，平移在索引12–14。Quaternion使用已有body::d::pose／rotation／angle约定，q和−q同姿；不拿raw XR quaternion直接对UE矩阵轴。

每组有独立presence／status／UNKNOWN。矩阵零、identity quaternion、kernel code0不能在缺组时解释成通过。仅保留已有真实时间及clock domain／frequency／point或interval类型，不把后继返回时间当目标产生时间。跨record的raw右手相同不代替head／left／reference／view相同；优先复用BMS已有source/basis，缺失时明确不能归因目标映射。

消费先报告同表示Local是否逐bits一致，以及世界位置／角度误差；若引用原`.002`游戏单位／`1e-4`弧度政策，必须确认producer使用该政策，否则只报数值。目标变化可能来自上游输入或basis，不直接叫solver错误；提交未执行、原返回或读取缺证则边界未知。只有准确同案mesh／实际epoch／end来源链才能和后继PG关联，不能用近邻或区间包含推断。

可复用入口是已有hand/BMS字段及其严格reader、`weapon_component_observation.py::mul/inverse_affine`、`weapon_lancer_muzzle_pose.h::location`和Body纯数学例程。旧BMS reader只读其原格式，PG saved-case分析也不能冒充新Native ABI。Body冻结新magic／version／Header／Record／offsets／valid位／enum／clock及Host布局后，武器会话只做所需解析与窄fixture，不重做旧v3解码、Game或Native构建。


## RWENT001 最小消费者交付（2026-10-27）

Body最终source3合同已冻结为 `27493ce7a286cb69ef87dcd1bfec354e59043fcfe913370e7e4f385c051e3520`，Header128／Record2024／Key240／Stage328、容量64。武器会话的纯文件解析器保存在 ignored `analysis/lancer-ready-same-entry-20261027/read_ready_wrist_v1.py`，SHA256 `821d0167bc054ec7ca4169b2fbfbd376f7c6bff464fe92f714d3b3b81bd8ddea`；交付回执 `consumer-delivery-v1.json` SHA256 `26a8c3aa6bee317ceee243aa791687380a0c78d5444b465055869a8e4ed92940`。调用为 `python read_ready_wrist_v1.py INPUT.bin NEW_OUTPUT.json`；输出CREATE_NEW语义，不覆盖旧结果。输入合同及原始header／compiler-layout文件必须通过精确哈希。

消费者逐组应用valid位，未尝试／失败阶段保留状态但不解释partial数组，拒绝坏magic／版本／大小／截断／尾随数据、未知enum与矛盾旗标。保留选择窗口内失败调用、不同mount／prepare／commit和缺少target的记录。Local依35／51／52顺序比较before→planned→commit→consumer_pre→returned_end的逐bit差异、位置／旋转／scale数值；q与−q等价。returned_end独有Space35／52，用row-vector的Space×CTW计算world，再与已为UE世界单位的目标比较，不重复乘units_per_metre。跨阶段epoch允许前进。

相邻entry只在原始head／right／reference、实际base、component、units、mount可用且cohort匹配时报告target数值变化，并同时报告已记录映射输入是否完全相同；不以rawRight相同推出目标必须相同，也不输出自动因果结论。raw XR位移误差单位为米，world／target位移为游戏单位。源ns保留原值，Stage QPC仅用Header频率计算读取时长。没有套用未经确认的误差通过阈值，没有新增gun／Muzzle或借用旧PG数据。合同文字有“six contiguous double scalars”笔误；实际源码五个double及编译器offset552..584一致，解析按准确布局。

必要验证：独立ctypes定义的Header／Key／Record／Stage每个字段与冻结的编译器布局比较；12项方法全部通过（0.020秒），覆盖有效位／UNKNOWN／失败partial、阶段变化、q符号、世界矩阵与旋转、跨entry映射缺失／cohort改变、空记录／64条溢出和坏输入。CLI合成解析exit0，已存在输出及截断输入exit2且截断不生成输出。未重跑旧suite，未改建Native／PG／XR或启动Game。已交给Body以准确67b513候选运行唯一K案例；实际K解析和定位结果仍待该局完成，以上不是跟手修复或玩法／GPU／真机验收。


## K：同次更新实跑结果（2026-10-27）

Body唯一K已正常完成原8动作、三个门、松手后的身体续更、双控制器还原和正常清理；本会话未启动第二实例。身份为Game74284／creation134375340335681979／Native3200498446623，准确Native67b513候选。新原件 `body-ready-wrist.bin` 36560B、SHA256 `b3563225378946723d82e7f91bd15062bc54a02216bba330f04f3dfe648ab28b`，Header128／Record2024，共18条，selected18／drop0／nested0／unmatched0。冻结解析器仅对该新原件运行一次exit0，原始输出 `k-ready-wrist-parsed-v1.json` SHA256 `f0c0d74befd7a5d9bdc18989e23669c9d424d2f27810daf61cfcc520ade4723b`；随后从已解析JSON整理 `k-ready-wrist-review-v1.json` SHA256 `fc7cf1c50e34e9ec76e2e15b7b59ac23c111fb287c979292d862218dace9e7a5`，均在ignored同目录。未重解旧BHELD003或PG Muzzle。

18条分为9个成功提交、3个guard_rejected、3个实际mount0但无target／prepare／commit、3个capture seed_model失败；17条known-ready和1条nonready closing都保留。成功seq1／3／5／8／10／12／14／16／18中，planned→commit→consumer_pre→returned_end的Local35／51／52全部逐bit一致，最终world腕到target的位置误差为2.90473e−5至2.35253e−4游戏单位，角度2.07531e−7至7.46121e−7弧度。这是该9份样本的数值，未套未认证的通过阈值。

提交拒绝为seq7／epoch1295／unit17、seq13／epoch1301／unit3、seq15／epoch1303／unit9，均code12；这三份consumer_pre的Local35／51／52与本次before逐bit相同，而pre到end发生变化。返回腕目标误差分别15.77156／16.08712／16.17715游戏单位、0.603902／0.599980／0.596448弧度。另seq9／11／17实际mount0，seq2／4／6 capture reason4；其未知字段未填补。11份known-ready有效target位置和角度差均0；相邻seq7→8、12→13→14→15→16具有完整且相同的已记录mapping输入、base、CTW、units、mount，但存在成功／拒绝分流。

因此当前优先定位提交前的capture／mount／commit准入分叉。成功路径未观察到原流程将已提交的这三根Local改回；拒绝路径的后续变化不能作为成功写后动画覆盖的证据。准确seed_model子条件、guard unit对应的失败判定及mount0原因由Native owner继续定位，不为增加通过率放宽保护。尚不能宣称完整故障根因或跟手修复。

覆盖按原capture_ns与原phase ns比较，不从QPC猜branch时钟：原ready区间320060142564400..320060994053000内只有seq2..16的15个capture点，其中7成功；seq1的capture点约早于ready marker13.549微秒，该点不等于整次entry区间；seq17／18在ready结束与move开始之间，18为nonready closing。肩和移动phase没有此新观测的capture点，不声称其阶段覆盖。Header unavailable_input293是整个启用期的尝试统计，不能当293条ready遗漏或失败。106清理、XR0和原文件／音频恢复沿Body原handoff证据，不能替代玩法或真机验收。


## RWENT002 增量消费者（2026-10-27）

按统筹明确窄交付，新建 `analysis/lancer-ready-same-entry-20261027/read_ready_wrist_v2.py`，SHA256 `9923573d99c72408c56b8f59570ec7dd599b6f429c60dd80d8a12fae157dfa00`；交付 `consumer-delivery-v2.json` SHA256 `feddc65f9e445172a75af3936996183791f182aed45a9c85eae447d2f76fb039`。准确delta合同SHA256 `fca9c717500f2abe2b7bbb5da41fe4c2c46cd178182f1cbafcf24a8a0562fbfb`。新文件显式RWENT002／version2／Header128／Record2040，原2024前缀和Key／Stage offsets全部保持，尾2024／2032为commit_context_verdict／causes。旧reader821d016和旧合同／K parsed／review均保留且核hash未变；未重解K或补值。

适配器先核冻结v1 reader、合同、source header与compiler-layout哈希，在自有内存中投影原前缀并复用原数学／有效位逻辑；输出原文件SHA、version2／R2040及明确的内存投影标识，不把投影当新原件。v1严格分派仍返回原结果，不增加拒绝字段，原v1 reader继续拒绝v2。调用 `python read_ready_wrist_v2.py INPUT NEW_OUTPUT`，同目录保留原v1 reader，输出不覆盖。

两尾字段全UNKNOWN仅表示本次特定return未观察，不能推出成功或hard guard；非未知仅接受verdict3与causes1／2／3且实际prepare0／commit12，半缺或非法对直接拒绝。事实mask1表示已取得的body source verdict为source-age-only，mask2表示已取得的equipment_fresh为false；只解释该entry在Context stagecommit首个既有BodyMainMountSource source-age返回。不能推出事实先后、失败branch时间、validate／exchange边界或更深metadata-age／raw-age子条件，不能与capture verdict或rollback后最终值混用。

6项新增必要方法／0失败（0.007秒）：独立ctypes原前缀与尾布局、三个合法mask、UNKNOWN边界、非法／半缺对及实际prepare／commit矛盾、v1兼容和严格旧reader拒绝新格式、版本／长度／前缀验证。CLI新合成成功0，覆盖旧输出及半缺对拒绝2，v1合成结果与原结果完全一致。旧数学套件复用未重跑，无Game／Native／PG／XR构建或运行。已交Body和统筹准确哈希；新DLL必要检查与新验证安排由其负责，此增量交付不是新的实跑或根因结论。
