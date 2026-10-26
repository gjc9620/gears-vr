# EN：PG 原来源过期与下一最小修改边界

本次只核对EN保存的日志、三份原binary及PG0ec1现有源码；不改PG／Native／门槛、不派生case、不构建或运行Game。完整需求和原持续held／移动／交还验收保持。

## 已核实的行为与覆盖

实际Game20148／Native2447539276000，typed4424、v32120和ARMALP01的28条记录逐条关联，552个原事件逐字段等Body严格reader且无drop。27条完整Local／FK／consumer-end通过，四角色各有27次actualApplied；不是仅看owner grant。异常准确为sequence5／query30，四角色tokenRejected且未Applied，仍保留待定位。query30自身PG ready，这条异常不能归入PG最终source_mismatch。原first-held40／6秒失败，未到后续移手；本轮freehand继续与正常106清理通过，不覆盖EM旧失败。

## PG拒绝在哪里发生

PG只保留first32／总111次callback（truncated1），原Native预算也正好query1–32；Root逐条核对Scene／publication／cookie／Native实例和原sync一致。以下只描述该前缀，不能据此分类全部63次stale_broker：

| 前缀类别 | 条数 | 原sync在PG入口年龄 | PG耗时 | PG结束年龄 |
| --- | ---: | --- | --- | --- |
| unheld |6|独立早退样本|未执行held全路径|其中2次Native source_after，不能计成held资产失败|
| held且ready |10|36.2374–41.8290ms|6.3443–8.1438ms|42.7718–49.7524ms|
| held最终response12 |16|43.7524–49.1403ms，中位46.6176|6.3998–7.3612ms，中位6.80845|50.9208–56.1447ms，中位53.2709|

16条失败均为初始context ready、retained complete、独立Game／typed资产／附着／bind检查ready，最终sender::response里的context返回source_mismatch。**在response入口时16条原sync都已经超过50ms**，不是仅在最后打印时越界。`weapon_main_mount_sender.h::context`要求Rig有效且原controllers.sync≤50ms，response再次调用它；这是足够解释本组拒绝的实际年龄条件。末次Native failure_query110／mask1025／旧tracking约53.76ms与last_query111／raw0不是同query，不能强配。

仅使这16条PG回复赶在50ms以内，就分别需要省0.9208–6.1447ms，中位3.2709ms；尚未包含后续Native current／Kernel逐写复核。first32中10条PG ready只有6条出现在成功held记录，query10／13／21／27没有成功held记录；不能把budget commit_end当成功或精确CAS时点。当前C0→query约1ms，把查询小幅提前不足以覆盖上述缺口。新Tick40消费与原Scene50资格必须继续分开，不能用新Tick时标更新旧包。

## 推荐的最小可改范围

下一份有实质变化的候选应只动**PG claimed同步callback内的纯解析工作区**，复用Native46／PG wire／原动作。保留两次真实Game观察、独立末读、两次typed资产、所有class/outer/registry、首次同名字段／类型、generation／binding／原50ms检查，不缓存Frame／资格／字段值或跨query的证明。

现有源码可见两个具体算法边界：`fresh_name`对每个重复FName逐wchar fresh读取；三个field walker（`weaponhand::field`、`PawnObserveFields::resolve`、`Reader::pointer`）对最长4096链逐节点线性扫描seen，最坏O(n²)。可在一个固定有界、无堆分配的query工作区内做：只保存(entry,index)的长度界限，重复时仍重新读slot/index及全部实际字符和NUL、拒绝内部提前NUL／长度变化／不可读；以及用精确地址哈希集合替代seen查重，仍保留全部真实metadata读和原cycle／4096／首个同名语义。工作区仅现有entered成功后启用，在原finally恢复并清零，拒绝重入不得动外层；嵌套／容量不足走原实现，默认其他路径仍原实现。不得启用旧legacy component_names解析器。

这是一份可实施的PG内部候选边界，**尚无零售墙时收益证据**。先前owned fixture中的33%／41%只是理想copy调用减少，不能换算为能省3.27ms，也不能保证最晚49ms入场样本通过。建议将这两个纯解析成本作为同一有界候选评估，避免只做微小名称改动后反复实跑；定向等价／突变／边界与实际wrapper finally检查后，才完整严格构建一份PG，再用原动作一次测真实耗时及held。若实际成本不足，回Body评估真正生产／消费时序；不续时、不删除末次观察、不放宽50或40／6秒门，不增加遥测平台或并行旧矩阵。

本阶段已按要求止于分析与边界交付，未实施该候选。独立回执在ignored `analysis/lancer-native46-review-20261026/`：en-root-review.json、en-pg-source-boundary.json。六份所审PG源码逐项与0ec1实际构建来源一致；reader结果、原日志与EN现场保留。
