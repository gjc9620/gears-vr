# 连续双目启动：用就绪屏障替代固定三秒

2026-10-17。候选61，源码 `88cfae66d0f16fd0aea90283470c9cdf4fcfdc20`，DLL SHA256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`。仍默认VRDof60，61没有因两次游玩成功就直接提升。

## 改动

旧持续诊断提交75149ba遗留了初始化后固定三秒的submit_not_before。候选60三次新启动的状态中分别有229／267／231次该门拒绝，且确认原生running后到首投影均约2.98–2.99秒。

连续模式改用Initializing／Ready／Closed单个原子状态。worker只可CAS Initializing→Ready，所有停止请求与worker退出关闭，Closed不可重开。仅在原有Hook应用成功、seal已执行、停止事件创建及首次状态通道可用后发布Ready；提交入口acquire检查，保留owner、相机／字节、anchor和Lifetime的原有认证及停止屏障。失败不因时间过去就允许提交。非连续有界诊断仍保持原三秒策略。

没有将运行中状态文件的偶发打开／写入失败新增为整个VR会话的立即终止；首次写失败仍由原worker条件拒绝。新增有限Ready QPC标记，未添加GPU命令或场景Hook。

14项冻结CPU／SDK检查、22项完整启动检查通过。ContinuousCheck实际覆盖私有门初始化／重复发布／终态关闭、32次publish与close竞争以及生产停止后不可重新发布；完整startup suite没有启动游戏。

## 两次实际新启动

| 会话 | 相机就绪→首个实际projection | running观察→首个实际projection | Ready→首个projection |
| --- | --- | --- | --- |
| 候选60，101 | 6098.714ms | 2981.428ms | 无标记 |
| 候选60，102 | 6128.781ms | 2990.087ms | 无标记 |
| 候选61，105 | 6117.196ms | 129.168ms | 230.388ms |
| 候选61，106 | 2947.415ms | 86.409ms | 170.069ms |

105因UI证书读取变化重新获取，原生准备5985.277ms，抵消了取消固定等待的总收益；没有隐藏该样本。106无该重取，原生准备2857.661ms。表中均不包含相机就绪前的菜单／加载时间，也不是头显显示延迟或网络延迟。first projection使用XR实际xrEndFrame成功标记；日志读取观察可能晚约3秒，已单列、不混入实际提交时刻。

105／106均完成死亡双眼场景及一次world1→2检查点恢复（派发至观察7488.115／7484.838ms，含加载）。两眼景深修改各557／3034次，发布校验拒绝均0；正常停止、独立恢复、普通关游戏及每轮四存档恢复通过。协调线程复核两轮死亡图。ignored证据 `analysis/runtime/vrdof61-startup-comparison.json`、`vrdof61-two-starts-regression.json` 及对应会话状态／图像哈希；原始新启动会话为35244-20261017-141725-fd564c08、10772-20261017-141934-67093e31。

## 107电影复测：未通过，保持问题开放

会话 `continuous-stereo-32580-20261017-142217-7e3ae609` 在原生启用前认证超时，异常为UI acquisition exceeded retry deadline before activation。先有跨帧相机时间戳拒绝；没有完整分阶段耗时，不能认定UI构建自身就是耗时根因，更不能将失败归给尚未启用的就绪屏障。

该轮native_started=false，连续会话清理没有成功回执，不冒记native_cleanup_verified=true。启动器随后独立取消owned prime并确认入口恢复，普通关闭游戏、XR退出及四存档恢复通过；相应`prison107-normal-close.json`和`prison107-save-restored.json`保留此区别。没有绕过认证或原样盲目重启。

下一步需有界计时定位认证中耗时的具体阶段，再复测电影与便携包。候选61的ZIP已离线生成但尚未实际运行，默认仍为已验证的60。新门的两次成功不能替代全部CG、机枪、长期或真头显验收。


## 109阶段计时：不是UI构建卡住

独立私有副本的start_capture实际子进程只包原有acquire／resolve／anchor／UI／crosshair调用，原参数、55秒期限与重试策略不变，没有额外resolve。278个有限事件记录：46次Resolver.resolve全部成功，共28.387秒；46次build_anchor_config全部因跨帧相机时间戳拒绝，共4.329秒；UI与Crosshair调用均为0。其余主要是原有每轮0.5秒等待。因此本轮的UI acquisition超时文案是通用截止检查，不能当作UI构建缓慢的证据。

原始trace位于ignored `analysis/release-staging/vrdof61-profile109-20261017/analysis/runtime/startup-profile-32692-28471d83ba354e5795a050fb36f87cb3.jsonl`，源码及身份／哈希、阶段汇总见 `analysis/runtime/profile109-stage-summary.json`。首个测试monitor错误读取仓库旧会话索引，证据保留为attempt1；修正私有索引后在同一拥有的游戏、再次认证章节警告下继续，未重启或绕过状态检查。诊断副本修改不属于700文件便携包验收。

该轮仍未启用新原生逻辑，启动器独立prime取消／入口恢复、普通关闭和四存档恢复通过；不把native_cleanup_verified=false改为true。进一步检查发现两个同文案时间戳边界，现有build总耗时不能区分它们。下一轮仅在私有guard内标记边界与阶段时刻，保留全部读取和比较，再判断优化；不增加期限或复用旧认证。
