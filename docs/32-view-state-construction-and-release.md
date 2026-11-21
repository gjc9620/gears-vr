# Linkage of each eye: creating and releasing clues

2026-10-12,Preliminary Process PID 17824, read only. No state plants are called and no additional status created or released.

## From Player to View

Normal Player scene function 0x15cf08 Yes. 0x15d974 Place Player Object +0xCC As R 8 Pass to Source View Full Construct 0x15e428. Constructing 0x15e495 Put R in.8 Write Source View +8。

Player Initialise Path 0x3ec5a8 Yes. 0x3ec63c Call the factory. 0x33bdd4，0x3ec641 Write return value to player+0xCC. The factory is... 0x280 bytes,8 Byte Alignment Call 0x1c03c8, and then call the single parameter to construct 0x33be04。

Current Player +0xCC Yes 0x8d3c2900, with the current round 6,910 The link pointer in the secondary render preparation log is consistent. 0x1353918, also appears in the table position of plant construction and resolution. The stacking address only applies to this process.

This is more than just taking view +8 As unknown pointer more specifically, but with no symbolic information, it is still called "Copient View State".

## Historical field check

The + of the current object 0x210 Location 16 Float point values, maximum absolute difference with last set of source view matrices in the sample 6.15e-5；+0xE0 Maximum difference with the last source projection matrix 0.0021605. Read the whole of it twice instantaneously 0x280 Bytes are consistent, and all comparative fields are limited.

Previous read-only survey of preparatory functions found view matrix orientation +0x210/+0x230, Projective data orientation +0xE0/+0x100 . The current result corresponds to the candidate for saving historical data. Old CSV is not the same frame as current reading, and the margin cannot be used as a simultaneous check or as an official name for all fields.

## Create and render resource registration

Construct 0x33be04 Create header, +0x14 and +0x1C . The tail ends 0x33c074 Call 0xF007C, received by +0x1C Sub-object address.

0xF007C Thread/ Mode Branch: A dash can be called directly to resources +0x28, and can also be written 16 Byte Rendering Command. Command Form RVA 0x1339468 +8 Access 0x19fc50 Read the resource pointer saved by the command and then call the resource table +0x28. This means that the plant contains a replica of resource registration, which cannot be modelled on the normal data structure and distributed only 0x280 And clean up.

## Why can't you just release?

First Table Entry 0x4ce80c Separately handle non-empty objects +0x1C and +0x14: The former is handed over to 0x282930 The latter is to be handed over. 0xCB198。

0x282930 Directly call the resource table +0x30 or via a command sheet RVA 0x133CEB0 +8 Access 0x1590f4 Delay call.0xCB198 You can put +0x14 Pointer adds a global array of to-dos, and the other branch calls the first false function of the sub-object.

+0x14 First entrance to sub-object list 0x33BC54 Deduct the pointer 0x14, and then pass the object header +8 Call packing with demarked 0x33BC70. Packaging first 0x33BCA4, the minimum mark is 1 Time-Crite 0x280 Size release storage.

The above is static control stream evidence. No dynamic confirmation has been made of which branch the current operating mode takes, when the array to be processed is emptied, and whether the GPU resource has been released. It cannot be called resolution or free directly after the factory returns, nor can the false header entry simply be called immediate removal function.

## I'm gonna need a visual.

Source View Construct to Write From Parameters +0x48、+0x4C、+0x50、+0x54 Four floating point values; complete construction subsequent branch also takes integer sizes through the output target's default function of the pool and writes +0x60/+0x64, and calculate with these values +0x3D0 to +0x3DC . The source view does not have two arrays, and the adjustment of the left-and-right eye-view is accompanied by a synchronized spin-off parameter, which cannot be changed to just one width field.

## Next wave of controlled authentication

First, you look at the independent status plant and its normal resource release interface, confirm that the refilling thread has been completed and then assigns independence to a copy of the unsubmitted source view. Then you verify the matrix, visual mouth and state independence of both sources' views before you try to submit two views.

Helper `find_native_rip_refs.py` Press 1 RIP relative to LEA/MOV references in the MiB block scan normal code; approximate of current cycle scan 19.1 MiB, excludes the protection segment. The function starting point given by nearby fill bytes is a candidate and continues to check key entry using a bounded traffic control tool.`inspect_view_state.py` Fluidly read old CSV and read only 0x280 Byte state, without whole sampling to memory.

Local evidence is here. `analysis/native/`：`view-state-construction-17824.json`、`view-state-factory-callers-17824.json`、`view-state-registration-17824.json`、`view-state-command-consumers-17824.json`、`view-state-destruction-17824.json`、`view-state-history-comparison-17824.json` Wait.

---

## 中文

# 每只眼的关联状态：创建与释放线索

2026-10-12，原版进程 PID 17824，只读调查。未调用状态工厂，未创建或释放额外状态。

## 从玩家到视图

普通玩家场景函数 0x15cf08 在 0x15d974 将玩家对象 +0xCC 作为 R8 传给源视图完整构造 0x15e428。构造中的 0x15e495 把 R8 写入源视图 +8。

玩家初始化路径 0x3ec5a8，在 0x3ec63c 调用工厂 0x33bdd4，0x3ec641 把返回值写到玩家 +0xCC。工厂以 0x280 字节、8 字节对齐调用 0x1c03c8，再调用单参数构造 0x33be04。

当前玩家 +0xCC 为 0x8d3c2900，与本轮 6,910 次渲染准备记录中的关联指针一致。对象首表地址 RVA 0x1353918，也出现在工厂构造和析构的写表位置。堆地址仅适用于这个进程。

这比仅把视图 +8 当作未知指针更具体，但没有符号信息，仍称为“候选视图状态”。

## 历史字段核对

当前对象的 +0x210 处 16 个浮点值，与采样中最后一组源视图矩阵最大绝对差约 6.15e-5；+0xE0 与最后的源投影矩阵最大差约 0.0021605。两次即时读取整个 0x280 字节一致，所有比较字段有限。

以前的准备函数只读调查已发现视图矩阵向 +0x210/+0x230、投影相关数据向 +0xE0/+0x100 的复制。当前结果与保存历史数据的候选用途一致。旧 CSV 与当前读取并非同一帧，不能把差值当成同步校验或据此确定所有字段的正式名称。

## 创建与渲染资源注册

构造 0x33be04 建立首表、+0x14 和 +0x1C 的子对象表，初始化多个矩阵与容器。尾部 0x33c074 调用 0xF007C，接收的是 +0x1C 子对象地址。

0xF007C 有线程/模式分支：可以直接调用资源虚表 +0x28，也可以写入 16 字节渲染命令。命令表 RVA 0x1339468 的 +8 入口 0x19fc50 读取命令保存的资源指针，再调用该资源表 +0x28。这说明工厂包含渲染资源注册行为，不能仿照普通数据结构只分配 0x280 并清零。

## 释放为何不能直接 free

首表入口 0x4ce80c 对非空对象分别处理 +0x1C 和 +0x14：前者交给 0x282930，后者交给 0xCB198。

0x282930 可以直接调用资源表 +0x30，或通过命令表 RVA 0x133CEB0 的 +8 入口 0x1590f4 延后调用。0xCB198 可以把 +0x14 指针加入全局待处理数组，另一分支则调用该子对象的首虚函数。

+0x14 子对象表的首入口 0x33BC54 将指针减去 0x14，再通过对象首表 +8 调用带删除标志的包装 0x33BC70。包装先执行 0x33BCA4，标志最低位为 1 时按 0x280 大小释放存储。

以上为静态控制流证据。尚未动态确认当前运行模式走哪个分支、待处理数组何时清空，以及是否已经完成 GPU 资源释放。不能直接在工厂返回后调用析构或 free，也不能把虚表首入口简单叫作立即删除函数。

## 视口补充线索

源视图构造从参数写入 +0x48、+0x4C、+0x50、+0x54 四个浮点值；完整构造的后续分支还通过集合的输出目标虚函数取得整数尺寸，写入 +0x60/+0x64，并使用这些值计算 +0x3D0 至 +0x3DC 的相关参数。源视图不只有两组矩阵，调整左右眼视口时需要同步其派生参数，不能只改一个宽度字段。

## 下一轮受控验证

先观察独立状态工厂及其正常资源释放接口，确认渲染线程完成配对清理，再把独立状态赋给未提交的源视图副本。随后验证两个源视图的矩阵、视口及状态独立性，最后才尝试提交两视图。

辅助工具 `find_native_rip_refs.py` 按 1 MiB 分块扫描普通代码中的 RIP 相对 LEA/MOV 引用；本轮扫描约 19.1 MiB，排除保护段。附近填充字节给出的函数起点只是候选，已继续用有界控制流工具核对关键入口。`inspect_view_state.py` 流式读取旧 CSV 并只读取 0x280 字节状态，不加载整个采样到内存。

本地证据均在 `analysis/native/`：`view-state-construction-17824.json`、`view-state-factory-callers-17824.json`、`view-state-registration-17824.json`、`view-state-command-consumers-17824.json`、`view-state-destruction-17824.json`、`view-state-history-comparison-17824.json` 等。
