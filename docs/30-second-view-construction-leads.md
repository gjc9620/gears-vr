# Object boundary before second view construction

2026-10-12, Current Process PID 12624 . There is no call to construct functions, assign objects or modify the set.

## New discoveries

Player scene function 0x15cf08 The blogger adds:0x15d830 E.C.X=0x430、EDX=8，0x15d83a Call 0x1c03c8 Distribute the candidate source view. The result is passed through RCX 0x15e428, return value then in 0x15d9ae Pass. 0xca360 Adds the input set.0xca360 is the additional candidate for the needle array, which is processed at a specific capacity 0xca394。

It's in the rendering machine. 0xD30 The step-long consecutive copy is two types of objects of different sizes. The source view structure is not directly related 0xD30 Byte distribution followed by copying: The original structure calls also prepares a large number of stack parameters, including multiple points of reference, four float-point parameters, etc. All parameter synonyms and pairing contour are not confirmed and it is inappropriate to call the complete source view directly following the entry for copies of both known parameters.

Before 0x194454 The basic reproduction assistant also calls the container and quotes the assistant. 0x15cb7c、0x19483c、0x1948d8;All members cannot be considered to be normal values that can be copied independently. Rendering copy resolution 0x15c27c Dealing with large numbers of extended members, which are beyond the scope 0x430, not to use directly to clean the candidate view.

## Source View Clear Path

Keep chasing Draw's normal code and find it. 0x15ca0c: Press the group count through every non-empty pointer, call 0x15ca7c, and then EDX=0x430 Here. 0x53ea50 Release the object. Then count zero, by 0xdd200 Clears the pointer array. It appears at the same time at the end of normal Draw and at another submission branch. This is static evidence that the source pool owns and cleans up these views, without adding dynamic cleanup to Hook.

0x15ca7c Process Source View +0x418 the shared object (by -0x13C Call Later 0x1753a0) Cleanup +0x400、+0x3EC、+0x380 / +0x390 and +0x140 . Base copy 0x194454 Send Source +0x418 Copy to target and add references; the last value field observed includes +0x420. The reproduction and resolution are resource management syntax, not only the value of the value or the concoction of the rendering extension object.

Additional local evidence:`input-view-family-lifetime-12624.json`、`input-family-cleanup-12624.json`、`source-view-cleanup-12624.json`. The above is still only read-only evidence, no dynamic creation of additional source views and no proof that ownership can be safely transferred to the new view.

## Relationships that must be confirmed afterward

1. Source view complete construction parameters, or a controlled dynamic validation of base copying and pairing clean-up.
2. The dynamic time series of the cluster clean-up, if added to cover all normal/unusual exit branches.
3. Each eye needs an independent historical state, and a normal creation and release interface.
4. The actual support of multiple view view view, output target and chart path.

Only the normal way to construct a secondary view, copy a source view or submit it in two stages is to be selected after these relationships have been completed. Currently only the tectonic threads are confirmed and no secure second view interface is claimed.

Local evidence:`analysis/native/view-construction-ownership-12624.json`、`source-view-constructor-12624.json`. The preceding CPU left-and-right eye calculation has been validated by means of object management.

---

## 中文

# 第二视图构造前的对象边界

2026-10-12，当前进程 PID 12624 的只读普通代码调查。没有调用构造函数、分配游戏对象或修改集合。

## 新发现

玩家场景函数 0x15cf08 内，0x15d830 将 ECX=0x430、EDX=8，0x15d83a 调用 0x1c03c8 分配候选源视图。结果通过 RCX 传入 0x15e428，返回值随后在 0x15d9ae 通过 0xca360 加入输入集合。0xca360 是指针数组追加候选，具体容量处理位于 0xca394。

这与渲染器里 0xD30 步长的连续副本是两类不同大小的对象。源视图构造不是直接对 0xD30 字节分配后复制：原构造调用还准备大量栈参数，包括多个指针、四个浮点参数等。所有参数语义和配对析构未确认，不宜仿照已知两参数副本入口直接调用完整源视图构造。

此前 0x194454 的基础复制助手还调用容器和引用相关助手 0x15cb7c、0x19483c、0x1948d8；不能认为所有成员都是可独立复制的普通数值。渲染副本析构 0x15c27c 处理大量扩展成员，其范围超过 0x430，不可直接拿来清理候选源视图。

## 源视图清理路径

继续追 Draw 的普通代码后找到 0x15ca0c：按集合 count 遍历每个非空指针，调用 0x15ca7c，再将 EDX=0x430 交给 0x53ea50 释放对象。随后 count 清零，并由 0xdd200 清理指针数组。它同时出现在普通 Draw 结束和另一提交分支的清理位置。这是源集合拥有并清理这些视图的静态证据，尚未增加动态清理 Hook。

0x15ca7c 处理源视图 +0x418 的共享对象（通过 -0x13C 后调用 0x1753a0），清理 +0x400、+0x3EC、+0x380 / +0x390 等容器，以及 +0x140 的关联容器。基础复制 0x194454 将源 +0x418 复制到目标并增加引用；其已观察的尾部数值字段包括 +0x420。复制与析构都具有资源管理语义，不能仅复制数值或用渲染扩展对象的析构代替。

本地补充证据：`input-view-family-lifetime-12624.json`、`input-family-cleanup-12624.json`、`source-view-cleanup-12624.json`。以上仍是只读代码证据，未动态创建额外源视图，未证明所有权可安全转移给新视图。

## 后续必须确认的关系

1. 源视图完整构造参数，或基础复制与配对清理的受控动态验证。
2. 集合清理的动态时序，追加后是否覆盖所有正常/异常退出分支。
3. 每只眼是否需要独立历史状态，以及状态的正常创建和释放接口。
4. 多视图视口、输出目标和绘制路径的实际支持情况。

完成这些关系后才能选择正常构造第二源视图、复制源视图或分两次提交的方式。当前只确认构造线索，不宣称已具备安全的第二视图接口。

本地证据：`analysis/native/view-construction-ownership-12624.json`、`source-view-constructor-12624.json`。前面的 CPU 左右眼计算通过不等于对象管理已经验证。
