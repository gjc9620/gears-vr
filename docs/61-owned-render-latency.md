# 独立双目输出延迟：CPU 采样与下一步

最新进展为文末的Sustain1和Menu1：独立Low路径已通过90秒持续输出及240.91秒含一次暂停/恢复的实际游戏模拟器运行。下列Cost阶段XR零提交属于历史结果，不代表当前状态；菜单像素、关卡重载、High及新路径真头显验收仍未完成。

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

## Reuse2：容量复用与双眼遮罩协同通过120帧对照

遮罩修正增加默认关闭的高度留白许可，仅由开启容量复用且正在消费已登记owned ticket的调用传入。仍要求两个视图同一高度、顶部为0、正尺寸且不超过资源，source/target严格一致，水平相邻且并集不越界；保留实际全资源viewport、调用者、命令表、时钟及输入浮点位模式校验。默认调用仍只接受满高视图。42项回归包括新高度留白和越界/不等高/错viewport/错source拒绝测试。关闭owned/resize功能的最终独立兼容构建通过6项既有检查及额外的view-union-mask检查，共7项；记录`owned-reuse-compat1-all-tests.json`。

同一冻结构建的第二次运行PID10100（creation `134364957955025158`）完成120个owned ticket和120次发布；XR复制118次、投影层提交333次，仍要求匹配render ticket和100ms。姿态接收到发布中位33.489ms、p95 38.649ms、最大155.709ms（启动不能保证全部及时）。仅首次扩容创建172次资源，其后owned和legacy请求不再经过本路径创建资源。调用本体中位0.0003ms。

日志确认所有240次4096×2048双眼区域/4096×2160资源的遮罩都接受并转发`t=1`，`rectangles`拒绝为0；剩余19次scope拒绝发生在未获授权的消费范围，不改参数。首尾原生GPU图像均显示完整左右场景，右眼近黑回归消失。编码RGB加权亮度右/左平均比为0.898/0.872，与Resize1基线0.889/0.892接近；Reuse1回归样本为0.008/0.257。这些统计不是线性光亮度测量，也不是几何或真实头显验收。首尾图像已经直接查看，未以非零像素数代替图像检查。

该改动同时减少尺寸往返重建并恢复对应的双眼遮罩修正，支持当前独立Low通道的两个相连问题已在本次有界对照中解决。仍不是持续运行、High、完整UI、切关/重连或真头显验收。默认玩家清单仍未切换，资源/命令表诊断引用仍保留到进程退出。

首次运行PID31640提前保护停止，没有有效GPU发布；最后Trial error3、两次draw均未形成有效提交。首次stop位置已记录为`capture.cpp:1637`的`native_owned_scene_begin`失败，但可能晚于最初错误状态写入，不能据此认定根因。该类GPU跟踪初始失败仍是后续稳定性任务，不能被第二次成功掩盖。记录`owned-reuse2-31640-early-stop.json`及原始状态；两次均独立验证59个入口恢复并正常退出。

成功对照证据为`owned-reuse2-10100.audit.json`、`owned-reuse2-10100-resize.json`、`owned-reuse2-10100-eye-quality.json`及`owned-reuse2-10100-eyes-preview.png`；都在ignored的`analysis/ui-position-runtime/`。新版不再重复请求已知不可用的XR快照控制，只使用成功的原生首尾读回。清单为`tools/vr-launcher/manifest-owned-reuse2.json`。

## Refs1：统一场景提交与GPU状态证明的命令表代次

源代码确认一个生命周期冲突：`native_owned_scene`的Draw、prepare、commit原先使用全局`frame_link.current`，而owned GPU证明使用自己的`tracker`。桌面Present清空前者的全部命令表槽；连续owned模式的prebirth Present只清理负载，保留真实Reset/创建代次。实际CreateCommandList观察也只登记到owned tracker。于是合法的owned命令表可以在Trial中成为缺失引用，触发error3。PID31640的两次精确Draw、draw_mask=0/error3与此一致，但当次未记录最初缺失引用，不能把它的全部失败路径追认为已证实。

三处Trial查找现统一通过`owned_scene_gpu::list_ref`使用owned tracker；编译关闭owned pixels时仍使用旧记录。没有制造Reset、放宽GPU状态或时效检查。新增WARP模式13–17覆盖真实Reset后跨Present保留、首次创建的打开列表、关闭创建拒绝、失败Reset拒绝，以及prepare/commit之间代次变化拒绝。测试直接调用生产Draw/prepare/commit入口，使用合成场景元数据；没有native Owner，因此成功CPU receipt后的copy guard应单独报error10且不排队读回。它证明接线与代次约束，不证明GPU场景完成。

冻结构建`build-owned-refs1/GearsCaptureOwnedRefs1.dll`通过47项回归。另以scene开启、pixels关闭编译`build-owned-refs-compat1`，7项兼容检查通过，覆盖旧记录回退分支的编译及既有功能。结果保存在`analysis/ui-position-runtime/owned-refs1-tests.json`和`owned-refs-compat1-tests.json`；实际游戏结果另记。

Refs1实测PID42992（creation `134364973661445312`）完成120个ticket和120次发布，XR copies118、projection layers339，Trial/GPU error均0，59入口恢复通过并正常退出。接收姿态→发布中位33.791ms、p95 40.343ms；首次扩容172次资源创建，其后本路径不再重建。首尾编码亮度右/左比0.902/0.859，无近黑眼；这里只检查像素统计，没有新增真头显或完整UI验收。证据`owned-refs1-42992.audit.json`、`owned-refs1-42992-resize.json`、`owned-refs1-42992-eye-quality.json`。

停止后的状态又暴露一个边界：120帧观察上限达到后，owned tracker不再接收Reset，但仍保留旧代次，Trial继续以它们计数，最终同一个Trial的submissions变为181。这不是181次新增VR场景或有效GPU提交证明。Refs2进一步要求`list_ref`只在`watching()`时返回引用，finished、disabled或error均返回空；新增模式18直接验证三种停用状态不能从生产prepare生成receipt。此改动保留真实Reset约束，避免用历史记录冒充当前命令表证据。

Refs2冻结构建通过48项回归。实测PID28552（creation `134364976714124190`）完成120个ticket，119次发布，首个ticket因`no_consumer`明确跳过，XR copies118、projection layers342；Trial/GPU error均0。停止后的最后Trial仍为draws2/submissions1，旧代次重复计数未再出现。姿态接收→发布中位34.478ms、p95 39.828ms、最大162.194ms；首尾编码亮度右/左比0.908/0.866，无近黑眼。59入口恢复全部通过，随后正常关闭游戏。证据`owned-refs2-28552.audit.json`、`owned-refs2-28552-eye-quality.json`，冻结清单`tools/vr-launcher/manifest-owned-refs2.json`。

两次修正后的启动通过不能替代长时间/多次启动稳定性结论，也不能追认为历史所有黑屏的根因。仍为120帧实验，资源与命令表引用保留至退出，launcher状态按设计为incomplete，默认玩家清单未改。下一步优先把连续渲染与有界诊断记录分开，解决当前实验上限和资源生命周期，再推进High与UI；不再扩展全游戏资料库。

## Sustain1：渲染继续，诊断记录保持有界

新增默认关闭的`GEARS_NATIVE_OWNED_SUSTAINED`，要求原有owned XR条件。持续模式取消发布记录满、消费记录满和第120帧finished这三个停止条件。消费/发布总数使用64位计数，另存最新ticket；详细历史仍只保留前120项，最新时间戳单独覆盖，不扩展vector或逐帧分配。旧有界模式保持帧数限制。

性能计时的`Window`和嵌套`Scope`持有原始指针，因此前120个cost记录不移动、不循环覆盖；最新时间戳记录明确不作为cost指针返回。资源尺寸复用和双眼遮罩现在根据已匹配Trial的线程局部owned消费状态判断，不依赖计时指针。嵌套消费者保存/恢复该状态。否则第121帧计时结束会同时令尺寸复用/遮罩失效，重新引入等待或右眼回归。

49项回归通过。新增WARP模式19执行257次真实清屏、命令表Reset/提交、共享纹理复制、接收与确认，跨越记录上限后仍继续；检查计数、最新ticket、120条存储上限、图像ticket退休及首个cost指针稳定。合成的左右Draw元数据不能证明游戏双眼几何。resize真实detour自测还覆盖观察槽4096条耗尽且没有cost记录时，授权owned调用仍正确复用容量。scene开启、pixels关闭的7项兼容检查通过。

为验证持续后的图像，在第1800帧增加一次固定GPU读回，文件后缀`-late.ppm`；原来的`-last.ppm`在持续模式中仅代表第120帧检查点。三个读回对象保持固定数量，仍保留至进程退出。会话状态新增owned消费/发布计数，外部观察脚本可核对进程身份及持续增长，无需截图控制。

本次尚未解决512个累计命令表身份上限与native Owner释放协议；遇到创建抖动仍会停止，不能靠增大数组冒充生命周期修复。默认玩家清单未切换，真实游戏长时测试另记。

Sustain1实际游戏PID38544（creation `134364984029291547`）按计划运行90秒后停止，完成3106个owned ticket、3105次发布、3101次XR复制及8084次projection层提交。最后一次尝试明确为`native_stopped`，没有把停止期间的未发布图像算作成功。Trial/GPU error均0，GPU状态仍有效，最后Trial为draws2/submissions1。47个带真实进程身份核对的状态样本显示计数持续增长；详细历史始终为120条。创建列表身份总数87，未碰到512上限，不能据此证明切关后的生命周期也成立。

resize观察共6594次，4096槽之后溢出2498次，渲染继续。第1800帧（ticket3583）GPU读回完成，首帧和该帧的小尺寸直接像素图均已查看，两眼场景完整、没有右眼遮罩近黑回归；首/120/1800帧编码亮度右/左比为0.901/0.859/0.861。此为静止角色/模拟器的持续输出验证，不包括High、完整UI、重连、切关或真实头显体验。最新时间戳属于停止跳过的尝试，其36.154ms不能当作最后一次成功显示延迟。

59个Hook入口独立恢复核验通过，游戏正常退出。审计为`analysis/ui-position-runtime/owned-sustain1-38544.audit.json`，状态时间线`analysis/runtime/owned-sustain1-38544-watch.json`，首/1800帧预览`owned-sustain1-38544-eyes-preview.png`，冻结清单`tools/vr-launcher/manifest-owned-sustain1.json`。默认玩家清单仍未切换，状态仍因实验资源保留为incomplete；本轮证明移除人为帧数上限后能持续工作，并未宣称第一版完成。

## Menu1：用状态控制暂停与恢复

本轮复用Sustain1冻结DLL（SHA256 `885794b7a012ace1db2cb374f39c0ec7bb622ea95e8f93d306a950f4af6ef9bb`），没有新增渲染Hook或重新编译。游戏PID39376、creation `134364988128243081`，owner PID34692、creation `134364988052852308`；清单为`tools/vr-launcher/manifest-owned-menu1.json`。仅对该进程使用OpenXR Simulator，不切换全局runtime。

先检索资料库，复用`WarHUD.PauseGame/UnPauseGame`导出。脚本资料提示战役主机的暂停资产是通用`WarUIScene`类型的`DeltaPause-Campaign-Host`，关闭委托会走UnPauseGame；这些内容来自WarfareGame.u（SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`）的UnrealScript导出，不等于原生函数或GPU执行证明。未打开IDA、未做全库扫描，具体菜单标签和焦点由实际进程读取确认。

关卡中的已观察场景栈为HUD_Toast、MessageToast、PenderCrossplayInvitePopUp，后者没有已聚焦按钮，不能把场景名当成需要确认的邀请。核对对象归属、一致重读、PID/创建时间、HWND及运行中的owned发布后，仅发送一次定向Escape。首次读取因新增UI对象导致`Object registry identity changed`拒绝，重新创建同进程reader后读到暂停场景和`LBtnResumeSingle`；没有重复Escape。该焦点链各节点均为Enabled+Focused，输入阻塞为0、玩家索引为0。稳定重读后仅发送一次Enter，三次后续采样均确认暂停场景移出场景栈，发布计数7381→7477→7575→7674。

模拟器实际会话240.9133秒，8309个owned消费ticket、8307次发布、8304次XR复制、21669次projection层提交。121次有效运行状态采样均匹配进程身份且owned_error为0，最终Trial/GPU error为0；92个命令表身份仍低于512上限。最大XR提交间隔100.3443ms，因此不称为全程无间断。按计划停止后59个入口恢复通过，游戏正常关闭；Owner和命令表引用仍保留到退出，launcher按设计为incomplete。

本轮确认一次暂停/恢复没有使观察到的双目发布停止，不证明菜单已经正确显示在VR中。没有采集暂停菜单GPU像素，没有重载检查点、修改游戏存档或做真头显测试。下一步先取得重载按钮和确认框的实际状态，再验证重载期间对象替换及图像恢复；菜单、字幕与其他HUD仍需接入独立输出并验证像素。

原始资料及操作脚本留在ignored的`analysis/`：`ui-position-runtime/owned-menu1-39376.audit.json`包含输入前后状态、进程身份、发布计数、恢复/退出与文件哈希；对应`*-level.json`、`*-pause.json`、`*-paused.json`、`*-resume.json`保存状态证据，`runtime/owned-menu1-39376-watch.json`保存带身份的时间线。本轮运行未使用截图或Computer Use。
