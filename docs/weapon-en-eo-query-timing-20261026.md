# EN／EO查询耗时与原来源年龄复核

EO的纯解析候选在已保存的握枪查询样本中耗时较短，但原firstheld40／6秒仍失败，未进入后续移手。保持候选与原40／50ms规则，下一步由身体会话回查生产到消费时序，不继续叠加同类小优化或新建运行case。

本次只独立读取已有EN／EO PG查询日志和Native预算原件，按query、Scene、publication、cookie、Native incarnation与原sync时间关联，检查PG起止在对应Native query起止内。没有重新执行游戏、构建、测试或修改封存case。复核脚本、每条数据与输入SHA256在ignored `analysis/weapon-parser-candidate-20261026/compare_en_eo.py`及`en-eo-saved-query-comparison.json`。

## 实际观察

PG均只保留最初32次查询。EN总111次，EO总115次，均truncated；下面“握枪查询”仅指PG status非unheld的样本，并不等于最终成功提交。EN有26条，EO有27条。两个运行采样不同，表中差值不构成同输入配对实验或全场景性能保证。

| 指标／单位毫秒 | EN最小／中位／最大 | EO最小／中位／最大 |
|---|---|---|
| PG握枪callback总耗时 | 6.3443／6.5356／8.1438 | 3.6503／3.7464／4.4699 |
| context观察完成至retained观察完成 | 6.3430／6.53395／8.1422 | 3.6423／3.7375／4.4583 |
| PG进入时原sync年龄 | 36.2374／44.87425／49.1403 | 38.7161／47.0972／49.2830 |
| 最终response进入时原sync年龄 | 42.7707／51.78135／56.1443 | 42.3955／50.7641／53.4922 |
| PG完成时原sync年龄 | 42.7718／51.7818／56.1447 | 42.3966／50.7644／53.4926 |

已保存样本的callback中位数减少2.7892ms，主要体现在retained观察段；实际时长支持候选缩短该段的观察结论，不能由算法比较次数推导相同数值。EO进入查询时原sync年龄的中位数反而比EN高2.22295ms，因此callback减少并没有等量变成最终年龄裕量。不能把两次运行的ready41／held19与EN ready43／held28直接解释为优化退化或改善。

EN前缀为6 unheld、10 ready、16最终response12；EO前缀为5 unheld、11 ready、16最终response12。两边这些最终12都在初始context、retained、完整Game／typed资产／bind通过后，进入最终response时已经超过原50ms。EO这16条的原sync年龄如下：

- PG进入46.6415–49.2830ms，中位47.5599ms。
- PG callback耗时3.6503–4.4699ms，中位3.7389ms。
- 最终response进入50.3525–53.4922ms，中位51.2579ms；每条都已过期，而非仅日志收尾后才过期。

EN预算前缀能关联全部32条PG；EO预算前缀中有一个query0，因此只关联PG1–31，PG32没有对应Native预算明细。另一个query0为原no-query-age记录，不能按行号错配。EO失败样本中可关联的15条在compose进入时，原sync已经45.6674–48.0854ms，中位46.5109；compose进入至query开始仅0.9455–1.2952ms，中位0.9830。具体旧来源为何以这个年龄到达compose，仍待身体侧依据生产消费链核对，不能由本次年龄观察宣称某个线程或生产函数已确诊。

原commit_end是返回观察，不是成功或精确CAS时刻。首32条不能解释全程EN的63次、EO的65次stale_broker；其余查询明细未保存，不补造分类。PG ready、held日志成员关系、四角色最终应用、姿态通过和持续动作门槛继续分别记录。

## 玩法与清理边界

身体侧本轮报告：Game1500／Native2480512716131，19条held全部pose通过，四角色mask15／count4及actualApplied各19，380事件无drop；原firstheld40／6秒失败，未移手。HandStop5／quiescent、freehand继续、输入恢复、bridgeclose与106／Native／entry／XR0／PG0／十文件／audio10／准确PID恢复通过，host瞬时false保留。本复核对读取的清理回执及freehand字段做关联核对，没有重新计算这19条Local／FK；姿态与实际应用结论引用身体侧`eo-pose1`／`eo-alpha1`／`eo-runtime1`，不会冒充又一次独立姿态验算。

候选PG `f85a00dda92f09c0c7548151b676c7cb37dc5ec0879472201855ad0248615650`保留；Native46与EO原pins不变。尚无连续握枪／后续动作／实体头显接受，整体36%、W2 40%保持。此次形成了实际查询耗时边界，尚未解决持续玩法。
