# 0.1.0 首项：继续缩短上下机枪恢复等待

2026-10-21，用户要求以此作为 0.1.0 第一项功能，完成实现及自动验证后接入正常启动器，头显体验由用户之后统一验证。用户要求继续等待其他 FPS 工作者，不向其他任务发消息。

## 同次采集中的属性链复用

之前的一次准备会反复遍历相同类的反射字段链。部分函数插桩数据记录到 `fields()` 约 1.96 万次迭代、总计约 3.3 万次进程读取，单个 anchor 构建阶段约 220 ms。插桩同时干扰严格跨帧保护，故这份失败采样只用于定位热点，不作为有效完整准备耗时。

`Resolver` 新增显式、限次采集的字段链提示：保留已经访问的 `struct_children` 和 `field_next` 值，其他字段、对象身份、代码和关系证明继续走原有读取与校验。在所有证书完成后，逐条重新读取这些链值；任何变化都抛出 `DiscoveryRegistryChanged`，本次证书和 reader 一起丢弃。提示在验证前清空，不跨 reader、尝试或世界保留。

提示只由 world recovery 的 `_preparation_camera()` 启用，`acquire_observer_preparation()` 返回前负责完成校验。每次最多保留 65,536 条链值，超出部分使用普通读取。原有循环、字段 owner、属性类型/偏移、完整身份、代码和 native 最终重验仍生效；候选发布仍保留原子替换、独占路径和同步落盘。

## 已完成检查

263 项 native-capture Python 检查、16 项 package-inspector 检查通过，共 279 项。新增检查覆盖链头、中间链接、链尾变化，最终读失败、容量回退、下一次采集不复用旧值，以及完整独立 UI 证书内容相同。启动准备流程还验证字段链最终校验先于返回与 reader 关闭，失败时整次采集及其部分证书均被丢弃。

第一次实际新代码回归完成了 checkpoint 后的新世界恢复，owner worker 为 899.8533 ms，Prepare 到 committed 为 211.9200 ms；新世界 XR projection 有独立回执，native 清理及 79 项入口还原完成。这与之前 1378.9826 ms 的准备样本并非配对控制，不能作为稳定改善比例或头显黑屏时长。

该轮后续只读控制采集完整通过，用时 850.9441 ms；测试随后因“没有新场景 ticket”条件停止，整体报告保留为失败。诊断确认 XR 正在使用 movie fallback，未出现 owned error 或世界 quiescence。CPU 证书测量现在记录场景 ticket 是否增长，另行保留首次新世界投影证明，不把持续场景出帧作为只读 CPU 试验的通过条件。

同轮首次正常关闭等待超时；协调端重新确认原 game creation identity 后，对当前所属窗口再次发送正常关闭，游戏退出。独立 `mounted-latency3-stages-close-retry.json` 记录五份保护文件还原；原失败报告没有改记通过。后续测试加入相同身份下重新枚举窗口、再次正常关闭的有界清理。

## 配对测量

`mounted-latency3-stages2` 使用同一进程、世界和输入状态，按 control / field hints / field hints / control 顺序采集四份完整候选；每份都在发布前明确拒绝。控制仅禁用新增字段链提示，其余实现相同。通过条件包括完整身份一致、四份权威 INI 内容摘要一致、native generation/request/commit 不变、无 owned error/quiescence，以及退出/还原完成。

采用阶段函数计时，避免逐次读取/函数插桩扰动严格时间戳保护。阶段耗时包含嵌套，不可相加；成功采集与包含重采的总耗时分别记录。本轮在保护并备份的 ClientSettings 中临时请求 TargetFramerate=60，结束还原。它不是 FPS 对比或头显延迟测量。

四次成功采集分别为 control 786.3622 ms、hints 749.5833 ms、hints 812.5276 ms、control 803.2533 ms。成功采集中位数由 794.80775 降到 781.05545 ms，仅节省 13.7523 ms，不能据此报告稳定的体验改善。第二次 hints 发生 9 次严格保护重采，包含重采总耗时为 8522.0221 ms；它是发布前拒绝的只读试验，不能解释为一次头显黑屏实测，也不能从结果中删除。本轮四份证书内容摘要相同，首次 checkpoint 新世界投影、native 清理、79 项入口恢复及五份保护文件还原均通过，完整自动报告为 passed。

接下来继续测量占时较多的 UI 类发现扫描；字段链复用只是这一轮的局部优化。源码与证据保存在 ignored `analysis/mounted-latency3-*`，不进入发行包。本轮沿用最新默认自旋优化 native SHA256 `fe928d7bab650b7672bd92b0d9ce7193ee2c9762a36250319ebcae4971729e8a` 和 MountedLatency1 XR SHA256 `0a2b7f7dc6a65f8ee23c255423e5fac841726ac6f2ef9bc249ac8e5f4902d56c`。

后续批量 UI 发现、四次配对测量及默认源码回归已完成，见 [0.1.0 首项完成记录](mounted-transition-latency4-20261021.md)。
