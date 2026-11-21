# Object boundary of the double viewer renderer

2026-10-12，PID 20420, read only the normal code survey. Continue the document 38 , the result of which is to de-cipher the view and sort it around the next double view check.

## Observed Reproduction and Destruction Relationship

`0x116250` Yes. `0x1162df` Call `0x3a4c54`, copy input collections to rendering object `+8`. The group copying is written to the last 32 Bit Fields `+5c`, therefore the current observation covers 0x60 bytes, not just copying 16 The byte array header is assembled in full.

`0x3a4cd8` Allocating your own storage to the array of cluster view pointers, copying them again `count * 8` bytes. It copys needle arrays, not every source view object at this step.

Rendering Structures set aside by the input pool `root+68` Storage, in `0x11643f` Loop Render View Copy Construct `0x193d50`and implement:

- `0x116447` Write every one of them. `+d24` is the current serial number.
- `0x116451` Replaces the pointer in the pool with a new rendering view.
- `0x116455` Write Render View owner as `root+8`。
- `0x11646b` Compares the circulation serial number with the number of input pools.

This means that the construction code has multiple paths, which does not mean that the entire rendering process has been dynamically passed on two levels.

Parsing `0x15aab8` Yes. `0x15ab74` to `0x15ab8e` Press `root+70` Number of times, `0xd30` Step Call `0x15bda4`and then released `root+68` Quantum, zero and release. `root+8` A group of indicator arrays. Source view 0x430 & Render View 0xd30 Different clearance paths must continue to be used.

## Next Experiment Limit

Double view input must keep the full set context, specify a separate state for each eye and check that both source views are copied into two independent rendering views. The original collection cannot leave a pointer pointing to a local temporary array; the association status cannot be released while the rendering view is still in use.

The verification of the non-submission of a double view object is only used as a border check for the object before it is actually submitted. The final acceptance is still to render two different points of view into two image areas, and receive the left or right OpenXR eye instead of considering the tectonic cycle count as a stereo success.

Evidence:`analysis/native/two-view-renderer-boundary-20420.json`、`family-copy-boundary-20420.json`、`family-array-copy-20420.json`. Three files save only the machine, without modifying the running-time entry. This round of DLLs is disabled and the game is still running.

---

## 中文

# 双视图渲染器的对象边界

2026-10-12，PID 20420，只读普通代码调查。接续文档 38 的偏移视图结果，围绕下一次双视图验收整理，尚未调用两视图渲染器。

## 已观察的复制与销毁关系

`0x116250` 在 `0x1162df` 调用 `0x3a4c54`，把输入集合复制到渲染对象 `+8`。集合复制写入最后一个 32 位字段 `+5c`，因此当前观察覆盖 0x60 字节，不能只复制 16 字节数组头就当完整集合。

`0x3a4cd8` 为集合视图指针数组分配自己的存储，再复制 `count * 8` 字节。它复制的是指针数组，不在这一步复制每个源视图对象。

渲染构造另按输入集合数量预留 `root+68` 存储，在 `0x11643f` 循环调用渲染视图复制构造 `0x193d50`，并执行：

- `0x116447` 写每项 `+d24` 为当前序号。
- `0x116451` 把集合中的该指针替换为新渲染视图。
- `0x116455` 把该渲染视图 owner 写为 `root+8`。
- `0x11646b` 比较循环序号与输入集合数量。

这说明构造代码具有多项路径，不等于整个渲染流程已经在两项上动态通过。

析构 `0x15aab8` 在 `0x15ab74` 至 `0x15ab8e` 按 `root+70` 数量遍历、以 `0xd30` 步长调用 `0x15bda4`，随后释放 `root+68` 数组，再清零和释放 `root+8` 集合的指针数组。源视图 0x430 与渲染视图 0xd30 必须继续使用不同的清理路径。

## 下一实验约束

双视图输入必须保留完整集合上下文、为每只眼指定独立状态，并核对两份源视图被复制为两份独立渲染视图。原集合不能留有指向局部临时数组的指针；关联状态不能在渲染视图仍使用时释放。

未提交双视图对象的验证只作为实际提交前的对象边界检查。最终验收仍是分别渲染两个不同视点到两个画面区域，再接到 OpenXR 左右眼，而不是把构造循环计数当作立体成功。

证据：`analysis/native/two-view-renderer-boundary-20420.json`、`family-copy-boundary-20420.json`、`family-array-copy-20420.json`。三个文件只保存本机，未修改运行时入口。本轮诊断 DLL 已停用，游戏仍运行。
