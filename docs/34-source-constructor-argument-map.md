# Draft parameter map for full source view construction

2026-10-12. Call from the observed player scene 0x15d989 And construction 0x15e428 . This draft has not been used to call the game construction function.

## Windows x 64 Parameter Position

The first four target objects, aggregations, correlation status and one pointer parameter; subsequent parameters by 8 byte slots. Construct clearly to read the sections from these slots 32 Bits or integers, the entire slot cannot be used as a pointer.

| Parameters | Sources or uses of current observations |
| --- | --- |
| 1 | Objective 0x430 Bytes Objects |
| 2 | Gather, write objects +0 |
| 3 | Association Status, Write +8 |
| 4—10 | Seven pointer values, write to +0x10 to +0x40 |
| 11—14 | Four floats, write to +0x48、+0x4C、+0x50、+0x54 |
| 15 | Point 64 Byte Matrix, copy to +0x70 |
| 16 | Point to the other group. 64 Byte Matrix, copy to +0xB0 |
| 17—19 | Three. 16 Byte Input, Copy to +0xF0、+0x100、+0x110 |
| 20 | associated packagings;participating +0x140 structural reproduction of packagings |
| 21 | float, write +0x3E0 |
| 22 | 32 Bit Value, Write +0x3E4 |

0x15f083 Puts the target object in the Rax back. The table here only records binary transmission; most pointer parameters and the formal semantic of the latter two values are still unknown.

## Derivative Count

Construct to read +0x70 and +0xB0 , write with SIMD multiplied by +0x2B0 Starts the matrix block. After that, generate multiple groups of derivative data and move +0x380 Containers and +0x2B0 Give me the matrix. 0x1CD784。

0x1CD784 From regular RCX, RDX, R 8 Parameters start, press 6 Item, each 16 Byte prepare packagings and extract several four-point coefficients from the matrix. This corresponds to the cropping plane generation, which is still a candidate syntax. It cannot be moved only +0x70 , and leave the data with old vision points.

Same Construct pair 0x137A60 Call with XML 0—XMM 3 Enter and return multiple SIMD repositories, setting ECX. It cannot be called by the normal " two matrix fingers " C++ function. Use normal complete construction interface to recalculate or restore its internal ABI accurately, which is more valid than guessing the signature of a function.

## Local interface scaffolding

`native_source_view_args.h` Save 0x430 Byte Input Snapshot and Use 22 Parameter function type. It does not have a game address, distribution, Hook or start logic. The external resource pointer in the snapshot is still a loan value and cannot be held across frames or lines.

`SourceViewArgsTest.exe` Validates the transmission of seven points, four floats, input matrix/block addresses, tail fload/integer and returned values by local function. The entry address is also verified after copying the snapshot and the original input bytes are not changed. Construction and testing have passed.

These tests only validate local compilers using scaffolders, and do not replace semantic authentication of the internal configuration of the game. `build-source-args` No game injected; still running is finished and no sampling `GearsCaptureStateShadow.dll`。

The next round will recreate an unsubmitted view with the original matrix, size and scale, then try to shift extra eye positions against the raw derivative data and clean path. Do not use scaffolding directly for both eyes.

---

## 中文

# 完整源视图构造的参数映射草案

2026-10-12。根据已观察的玩家场景调用 0x15d989 与构造 0x15e428 的参数读取、成员写入整理。尚未用此草案调用游戏构造函数。

## Windows x64 参数位置

前四项为目标对象、集合、关联状态和一个指针参数；后续参数按 8 字节栈槽传递。构造明确从这些槽读取部分 32 位浮点或整数，不能把全部栈槽当成指针。

| 参数 | 当前观察的来源或用途 |
| --- | --- |
| 1 | 目标 0x430 字节对象 |
| 2 | 集合，写入对象 +0 |
| 3 | 关联状态，写入 +8 |
| 4—10 | 七个指针值，写入 +0x10 至 +0x40 |
| 11—14 | 四个 float，写入 +0x48、+0x4C、+0x50、+0x54 |
| 15 | 指向 64 字节矩阵，复制到 +0x70 |
| 16 | 指向另一组 64 字节矩阵，复制到 +0xB0 |
| 17—19 | 三个 16 字节输入，复制到 +0xF0、+0x100、+0x110 |
| 20 | 关联容器输入；参与 +0x140 容器的构造复制 |
| 21 | float，写入 +0x3E0 |
| 22 | 32 位值，写入 +0x3E4 |

0x15f083 将目标对象放进 RAX 返回。这里的表只记录二进制传递关系；多数指针参数与后两个数值的正式语义仍未知。

## 派生计算

构造读取 +0x70 和 +0xB0 的四行矩阵，用 SIMD 乘加写入 +0x2B0 开始的矩阵块。之后还生成多组派生数据，并把 +0x380 容器和 +0x2B0 矩阵交给 0x1CD784。

0x1CD784 从普通 RCX、RDX、R8 参数开始，按 6 项、每项 16 字节准备容器，并从矩阵提取、归一化若干四分量系数。这与裁剪平面生成相符，仍是候选语义。不能只移动 +0x70 的镜头矩阵，而让这些数据保留旧视点。

同一构造对 0x137A60 的调用使用 XMM0—XMM3 输入及返回多个 SIMD 寄存器，同时设置 ECX。不能把它按普通“两个矩阵指针”的 C++ 函数调用。使用正常完整构造接口重算，或先准确还原其内部 ABI，比猜测函数签名更有依据。

## 本地接口脚手架

`native_source_view_args.h` 保存 0x430 字节输入快照，并用 22 参数函数类型表达上述映射。它没有游戏地址、分配、Hook 或启动逻辑。快照中的外部资源指针仍是借用值，不能跨帧或跨线程持有。

`SourceViewArgsTest.exe` 通过本地函数指针验证七个指针、四个 float、输入矩阵/块地址、尾部 float/整数和返回值的传递。另验证快照复制后输入地址指向新存储，原输入字节未被改变。构建与测试已通过。

这些测试只验证本地编译器调用脚手架，不能代替游戏内部构造的语义验证。新构建目录 `build-source-args` 未注入游戏；运行中的仍是完成并停用采样的 `GearsCaptureStateShadow.dll`。

下一轮先用原矩阵、原尺寸重建一个未提交视图，对照原生派生数据与清理路径，再尝试额外眼位偏移。不要直接把脚手架用于双眼提交。
