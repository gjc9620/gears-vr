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
