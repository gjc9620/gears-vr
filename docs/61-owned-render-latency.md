# 独立双目输出延迟：CPU 采样与下一步

2026-10-15。OwnedCost1 / PID49468、OwnedCost2 / PID48432。默认玩家清单没有切换到实验版本。

两轮均完成120个独立 Low scene ticket；前者119次、后者120次共享纹理发布，首尾读回每眼2048×2048且非黑。**两轮 XR 图像复制及 projection 提交仍为0**，独立分辨率通道尚未达到可用状态。未放宽100ms姿态过期门限，也未把旧图像改标为新姿态。

## 有效样本

| CPU 侧区间/统计 | Cost1 中位数 | Cost2 中位数 |
| --- | ---: | ---: |
| 选定目标到构建验证完成 | 0.443ms | 0.404ms |
| 构建验证完成到消费开始 | 76.366ms | 74.815ms |
| 消费开始到左眼 final draw | 62.782ms | 61.950ms |
| 姿态接收到共享发布结束 | 146.809ms | 144.683ms |
| 共享发布调用 | 0.0369ms | 0.0384ms |
| safe_copy 累积 | 2.000ms | 2.158ms |
| engine-view draw 观察累积 | 1.533ms | 1.701ms |
| frame_lock 获取路径累积 | 未测 | 4.134ms |
| 原始 Draw/DrawIndexed 转发累积 | 未测 | 0.102ms |

默认关闭的 `GEARS_NATIVE_RENDER_COST_PROBE` 只在 owned renderer 的 `original_consume` 当前线程内记录有限计数，不分配内存、不输出逐 draw 日志。类别有嵌套，尤其 safe_copy 包含于 engine-view 观察中，不能把它们相加。QPC 观察是 CPU 墙钟时间，不是 GPU 执行时间。构建后到消费前还包括少量构建收尾，不应称为纯队列驻留时间。

Cost2 同时用 `GetThreadTimes` 记录该线程的用户态和内核态累计时间。120个有效窗口合计墙钟7566.6424ms、线程CPU2578.125ms，即每帧平均63.055ms与21.484ms。当前返回值以15.625ms为粒度，单帧中位数不能作为精细CPU剖析；聚合结果支持窗口内有大量未在该线程执行CPU代码的时间。它不能区分引擎同步、驱动/GPU等待、其他线程依赖或调度等待。

因此，本轮不关闭逐 draw 观察、不修改 Present 锁范围、不调整引擎队列深度。测得的这些观察项不足以解释主要延迟，需要先定位具体等待调用。源码中 Present 跨原始调用持有排序锁，是可检查的候选；当前只有4.134ms的获取路径统计，尚不能把它认定为约62ms的根因。

## 启动与失败样本

- Cost1首次 PID53792 在 UI 证书构建阶段拒绝激活：两次完整0x60字节 Pawn 头不同，具体变化字段未记录。prime恢复验证通过，正常退出；同一冻结DLL在新PID49468成功完成采样。后续提交`603c98d`只比较真正的对象身份字段，保留注册槽、名称、类型/outer闭包、角色引用和末尾复验，16项测试通过。
- Cost2首次 PID48296 在首个owned帧附近以`proof_failure`停止，最终Trial error3、无提交。首个停止调用点未知。stop_tick为GetTickCount64，渲染计时为QPC，未记录同钟对齐，不能直接用绝对值排序；也不能把最终error3直接视作首因。该样本不进入稳定性能统计。同一冻结DLL在新PID48432完成120帧，失败原因仍待定位。

两次完成采样及Cost2失败样本均独立核验58个入口恢复，随后正常关闭游戏。Owner/list引用仍按实验设计保留至进程退出，启动器仍报告incomplete；这不等于完整生命周期验收通过。未进行真头显、High或完整UI验收。

## 复用资料库与下一轮

已通过`tools/re-kb/kb.py`检索历史快照artifact1中的RVA `15a80c`（消费入口）和`11908c`（缓存伪代码中的下游调用候选），没有开启IDA、重新导出或改写资料库。快照代码节SHA256为`911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，模块基址`0x140000000`；磁盘EXE SHA256为`bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。缓存的多参数原型明显不能直接替代既有运行时调用约定，后续只作为定向检查线索。

下一轮先保留首次停止的同QPC时间和调用点，定位偶发启动后停止；再对渲染线程等待调用做有界采样，用真实调用栈将约42ms平均非CPU执行时间关联到具体分支。仅在有对应证据后修改同步或队列策略，并复测同ticket图像/姿态与100ms限制。缓存函数必须与汇编、调用上下文及当前运行字节交叉验证，不从静态文本宣称GPU等待根因。

每个实验构建通过39条既有回归；关闭owned/profiling的独立兼容构建通过6项检查。原始状态、时间序列、像素统计、失败记录与哈希审计位于被忽略的`analysis/ui-position-runtime/owned-cost1-49468.audit.json`、`owned-cost2-48432.audit.json`、`owned-cost1-53792-startup-failure.json`、`owned-cost2-48296-failure.json`。清单为`tools/vr-launcher/manifest-owned-cost1.json`和`manifest-owned-cost2.json`。这些结果没有完成首版发布目标。

## Wait1：停止来源与关闭菜单捕获的对照

首次成功的停止请求现在保存同一QPC时钟的时间、线程和源码位置，后续请求不能覆盖；状态输出以release/acquire标志区分记录是否完整。连续会话自测覆盖首次记录和拒绝覆盖。可选成本记录增加消费线程ID及线程变化计数，默认关闭成本探针时不采样。

Wait1 / PID51032临时关闭startup_menu及对应启动/菜单graphics保护（运行时graphics保护仍在），用于排查启动面板WGC捕获是否造成延迟。实验与正常启动还存在prime和XR启动顺序差异，不能当成严格单变量因果证明。默认玩家清单未变。

120个owned ticket、119次发布；首尾每眼2048×2048均非黑，XR copies/projection仍为0。构建完成→消费开始中位75.092ms，消费开始→左眼final draw为61.674ms，姿态接收→发布为143.336ms。与菜单开启的Cost1/2接近，本轮未支持菜单捕获是主要延迟来源。frame_lock累计中位3.780ms，不能解释整个消费窗口。

该轮首次停止明确来自测试控制事件，QPC=`1012316415632`、频率10000000、线程28352、冻结Wait1源码`capture.cpp:2985`。58个入口恢复通过，游戏正常退出。因Owner/list资源保留策略，启动器仍报告incomplete；不把主动停止或首尾像素检查当完整生命周期/头显验收。审计：`analysis/ui-position-runtime/owned-wait1-menuoff-51032.audit.json`。

系统WPR录制尝试因Windows权限失败（`0xc5585011`, “Failed to enable the policy to profile system performance.”）；前后均确认无录制会话，未生成ETL，也未改权限。接下来使用有界、进程内线程身份匹配的外部线程栈采样，并以QPC对应已有owned消费窗口；诊断暂停造成的时间扰动必须单独报告。
