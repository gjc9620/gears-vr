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

## Wait2：实际消费线程栈

PID51812完成120个owned ticket、119次发布，XR copies/projection仍为0；姿态接收→发布中位149.355ms。58个入口恢复通过，随后正常关闭。采样器只操作状态中公布的消费线程53460，持有匹配PID/creation的进程与线程句柄，先冻结有限栈字节，恢复线程后再用StackWalk64展开，禁止扫描原始栈猜调用链。

8秒记录257次有效快照，全部恢复；101次采集区间完整落在同线程的owned begin→returned窗口内，其中60次栈顶为`win32u!NtGdiDdDDIWaitForSynchronizationObjectFromCpu`，向上连续四个D3D12Core帧。另有分配、驻留和引擎/Mod锁等待样本。此证据支持主要消费等待候选是图形同步；它不能单独确定等待的fence、GPU负载或游戏调用者，也不能把采样次数直接当GPU执行时长。相应[官方DDI文档](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/nf-d3dkmthk-d3dkmtwaitforsynchronizationobjectfromcpu)描述CPU等待monitored fence的接口语义，未提供本游戏的根因。

Suspend→复制→Resume的调用区间总计10.6284ms，中位0.0324ms、最大0.505ms；这仍会扰动时序。最初128次元数据读取预算截断了上游栈，不能把其尾部当作真正调用链起点。后续采样器扩大恢复线程后的元数据读取预算至1024次/2MiB，并记录符号位移；无PDB时最近的导出名不等于内部函数名，模块RVA才是后续资料库定位依据。没有安装工具、提权或网络下载符号。

独立采样器通过Sleep线程调用链、错误身份拒绝、连续恢复和已有暂停计数保持自测。原始帧、同钟关联和像素审计：`owned-wait2-51812-thread.jsonl`、`owned-wait2-51812-stacks.json`、`owned-wait2-51812.audit.json`，均在ignored的`analysis/ui-position-runtime/`。

该轮之前PID41632在主scene激活前遇到`UI registry changed during assembly`。已保存失败记录、确认prime入口恢复并正常退出。仅该最终注册表并发变化现在允许最多3次、10秒预算的完整read-only准备重采集，每次使用新Resolver和一致的camera/anchor/UI/INI；其他身份、字节或anchor拒绝不重试，超时结果不接纳。7项专用测试及既有UI/启动策略共31项通过。PID51812成功经过新准备流程，但没有声称该次实际触发了重试。记录：`owned-wait2-41632-startup-failure.json`。

## 深栈：定位到尺寸比较后的资源重建

同一冻结Wait2 DLL的PID50924再次完成120个ticket、120次发布，XR提交仍为0。新的8秒采样257次全部恢复，102次完整位于owned消费窗口内；58次停在`win32u+6084`，距真实导出`NtGdiDdDDIWaitForSynchronizationObjectFromCpu`仅0x14。采集调用区间总计10.0152ms、中位0.0322ms、最大0.1921ms。58个入口恢复通过，游戏正常退出。

18个完全相同的已展开等待栈包含：`21bc96 → 21da6b → 21d412 → 21c747 → 1598bd → 159572 → 227507 → 2b643e → 2b5b13 → 119137 → 15a943`（列的是返回地址，方向为内层到调用者）。这些等待并非只凭静态代码猜测；它们实际落在owned消费窗口。较浅的驱动分配栈也经过同一个资源创建返回点，但尚未测量每次调用的资源描述符或分辨率。

资料库映射上述关键返回点到函数`1593c4/15974c/21bbd0/224ca8/2b59f4/2b6408`，在PID50924停止Hook后读取其全部已知chunks，六个函数SHA256均与历史快照一致。保持PID/creation/module base/快照SHA分开记录。原始证据：`owned-wait2-deep-50924-callers.json`、`owned-wait2-deep-50924-stacks.json`、`owned-wait2-deep-50924.audit.json`。

只在原IDA库的私有副本中导出`2b59f4`和`2b6408`共107条汇编。`2b59f4`的入口将RCX保存为对象、EDX保存为请求宽度、R8D保存为请求高度，比较对象`+F8/+FC`。普通分支尺寸不同会经过`2b5234`再调用`2b6408`；后者执行若干虚调用，实际栈的`2b643e`对应vtable+8调用返回，与`224ca8`大规模资源初始化路径相连。此函数还有配置变化比较，不能把所有重建都归因于尺寸变化。

IDA库字节与快照一致，但显示文本中的部分RIP全局地址不等于原始指令计算值。必须使用已保存原始字节的`next RIP + signed disp32`；例如模式标志正确RVA为`1b98b6c`。缓存伪代码、IDA显示地址和该标志的名称/用途不能直接当作可写配置，本轮没有修改该全局变量。汇编与计算结果：`owned-wait2-resize-disasm.json`。私有库lease已关闭；原库SHA仍为`66d17edbf21bf81f0fe5cf740111e738a0308ac9228f7576fe39f460baad1813`。

**下一轮最小实验**：仅在已验证入口有界记录对象身份、旧/请求/返回后的宽高、owned或legacy来源、配置比较状态、重建用时及资源创建计数，确认是否在不同目标尺寸之间反复切换。若成立，再设计保持足够大资源或按视图复用资源的最小修改；保持原始视图矩形、投影、同ticket姿态和100ms规则，并复测两眼像素。当前只能将“分辨率切换造成资源重建抖动”列为有依据的假设，尚未通过尺寸序列或修改后对照确认。

## Resize1：尺寸往返已在真实调用序列中确认

PID4368（creation FILETIME `134364947388686134`）用默认关闭的 `GEARS_NATIVE_RESIZE_PROBE` 记录704次调用，无溢出。启动前重新验证当前进程中`2b59f4`的全部两个chunks共364字节，SHA与上文相同；没有仅凭RVA或磁盘字节认定版本匹配。使用整数返回传输保存原始RAX，不采用失真的缓存原型。未更改函数参数、全局配置或视图矩形。

同一对象`0x141c3b2c0`出现以下确定的循环：

| 调用上下文 | 调用前→请求→调用后尺寸 | 次数 | CPU中位耗时 | 调用内资源创建API总数 |
| --- | --- | ---: | ---: | ---: |
| owned ticket消费 | 3840×2160 → 4096×2048 → 4096×2048 | 120 | 50.101ms | 20,640 |
| legacy消费 | 4096×2048 → 3840×2160 → 3840×2160 | 120 | 51.489ms | 20,640 |
| legacy尺寸不变 | 3840×2160 → 同尺寸 → 同尺寸 | 464 | 0.0003ms | 0 |

每次尺寸变化恰有172次已观察的资源创建API调用，无失败返回；它们不是全局计数差，而是同线程、当前resize调用内的计数。记录的是API调用返回，不等同于172个持久资源或GPU完成。七个已知全局配置值前后未变，但没有读取所有实时设置或重算引擎配置比较，不能声称完整排除配置因素。owned标记来自只为已登记owned ticket建立的成本观察范围。

120个ticket均发布，XR复制/投影仍0；姿态到发布中位149.112ms，owned消费总时长中位63.785ms，其中resize本体约50ms。这已证实资源尺寸往返与主要消费等待相伴；接下来用保留足够大资源的最小对照修改验证因果及画面正确性，不放宽100ms规则。首尾每眼4194304像素均非黑且两眼不同，仍不代表正确投影/UI/头显体验完成。

探针与既有回归共41项通过，包括真实MinHook包裹的假函数参数/RAX、4096槽溢出后继续转发、线程隔离、嵌套计数及入口恢复。新增入口纳入独立停止验证（完整364字节SHA），59个入口全部通过，随后正常关闭游戏。默认玩家清单未改，状态仍incomplete（资源按实验设计保留至退出）。原始记录和审计：`analysis/ui-position-runtime/owned-resize1-4368-resize.json`、`owned-resize1-4368.audit.json`；清单`tools/vr-launcher/manifest-owned-resize1.json`。运行时数据及缓存继续留在ignored目录。

## Reuse1：等待下降，但画面回归，尚不可发布

默认关闭的`GEARS_NATIVE_RESIZE_REUSE`仅锁定首次已登记Low owned消费所用对象与线程，拒绝其他线程/对象、未知尺寸及非零原生mode。首次请求把资源保留为4096×2160，此后仅对原桌面3840×2160与Low 4096×2048请求取已有容量的逐维最大值。没有写全局mode、改视图矩形、改投影或放宽100ms。记录槽满后仍执行相同策略和原始调用；独立假函数测试覆盖该路径。41项回归与关闭功能的6项兼容检查通过。

PID51936（creation `134364951814739390`）首次owned调用创建172次资源、耗时48.069ms；后续119次owned和346次legacy调用均不再创建资源，resize中位0.0003ms。构建完成到消费开始中位从77.244ms降至12.287ms；姿态到发布从149.112ms降至33.500ms，p95为40.811ms，启动最大值仍165.855ms。120个ticket中119次发布，XR复制118次、投影层提交336次；重复提交同一有效图像不等于新增独立场景。保持原时效规则。

**本实验画面不合格。** 首尾GPU读回显示首帧右眼接近纯黑，末帧右眼明显偏暗；Resize1对照没有该现象。现有`nonblack`实际只统计任意RGB字节非零，即使非常暗或近乎常色也会满足，不能把4194304个非零像素当作画面通过。小尺寸直接像素预览已保存在`owned-reuse1-51936-eyes-preview.png`和对照`owned-resize1-4368-eyes-preview.png`。

现有遮罩日志提供具体线索：Reuse1出现240次`view_union_mask_rejected reason=rectangles`，正好对应两眼owned消费的遮罩次数；旧验证器要求视图高度等于资源高度，2048视图/2160资源被拒绝。Resize1相同位置240次均接受。下一步保留容量复用，只在已登记owned消费范围允许两眼相同、有效且不超过资源的高度，继续验证viewport、水平并集、调用者与原始浮点参数，并重新采样。不能仅凭这些拒绝就宣布回归已修好。

59个入口恢复通过并正常退出。XR快照控制请求失败，消息为`Snapshot control unavailable in this XR build`，未取得该项XR快照；原生首尾读回独立成功。模拟器MCP返回2048×2048的左右image rect和非对称FOV，不能替代真实头显。状态仍incomplete；连续投影仅覆盖约3.7秒，停止前的长间隔包含120ticket实验上限后的等待，尚不属于持续运行验收。证据`owned-reuse1-51936-resize.json`、`owned-reuse1-51936.audit.json`、`owned-reuse1-watch.json`，均在analysis下。
