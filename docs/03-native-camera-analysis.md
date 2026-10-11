# 原生摄像机调查：2026-10-11

## 目的和当前结论

我们需要找到游戏真正用于绘制画面的视图入口。可以把脚本摄像机理解成前端的业务状态，把渲染视图理解成浏览器本帧实际绘制时使用的数据。改业务状态未必能及时改变已经送往渲染器的数据，因此最终需要追踪状态到绘制之间的传递。

目前找到了 `PlayerUpdateCameraNative` 对应的原生计算函数，并确认其输出结构字段与运行时反射信息吻合。这是原生代码证据，但仍是由脚本 VM 调用的摄像机计算阶段，尚未找到最终渲染视图入口。

## 版本和定位方式

目标 EXE SHA256 为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。所有下述代码位置都是相对模块基址的 RVA，不能直接当作其他版本的地址。

使用当前摄像机类的反射字段找到 `PlayerUpdateCameraNative` 的 UFunction，再读取其 `+0x114` 执行指针。这一偏移来自本版本调查，不是通用 UE 偏移。包装函数 RVA 为 `0x3b3af4`，其直接调用的原生主体 RVA 为 `0x110878`。

包装函数读取 `FFrame.Code +0x28` 所指的字节码参数，随后设置 RCX、RDX、XMM2、R9 并调用主体。根据反汇编及脚本声明推测参数分别为摄像机对象、Pawn、DeltaTime、输出 ViewTarget；尚未通过动态断点验证完整 ABI。

## 输出结构证据

当前运行时反射给出的 TViewTarget 布局：

| 字段 | 相对 TViewTarget 的偏移 |
| --- | --- |
| Target | 0x00 |
| Controller | 0x08 |
| POV | 0x10 |
| AspectRatio | 0x2c |
| PRI | 0x30 |

已验证 TPOV 内 Location 在 0、Rotation 在 12、FOV 在 24。因此相对 ViewTarget，位置是 `+0x10`，旋转是 `+0x1c`，FOV 是 `+0x28`。

原生主体一开始读取 `[r9+0x28]`。可达指令中出现对输出结构旋转和 FOV 字段的存储，以及把位置字段地址传给其他函数的操作，与上述结构布局吻合。后续仍需核对寄存器别名和各分支含义，不能凭单个数值偏移就认定全部数据流。

## 工具和可复现性

`tools/package-inspector/analyze_native_camera.py` 只以查询和读取权限打开进程，先重新解析当前摄像机，再反汇编包装函数及其直接调用目标。它不写内存、不注入 DLL、不设置断点。

使用方式（PID 应替换为当前进程）：

```powershell
& 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' 'J:\gearsVr\tools\package-inspector\analyze_native_camera.py' --pid 32532 --output 'J:\gearsVr\analysis\native\camera-graph.json'
```

本次得到包装函数 75 条可达指令，主体 895 条，均未触及设置的指令上限。工具跟随普通可执行区的直接跳转，不跟随被调用函数；这不是完整函数边界恢复，间接跳转可能造成遗漏。`.themida` 和 `.boot` 区段排除在遍历范围之外。

详细 JSON 和汇编保存在本地 `analysis/native/`，Git 仅保存我们的工具和文字结论。

## 下一步和验收条件

追踪 LocalPlayer / PlayerController 的原生视点读取，以及位置、旋转、投影参数进入渲染视图结构的路径。候选位置必须先证明读取的是当前玩家视图，再验证调用频率与实际渲染帧的关系。游戏失去焦点会自动暂停，因此停更不能单独作为脚本 VM 卡顿证据。

只读证据充分后才开展可回退的小角度修改测试；成功标准是画面按预期改变，同时角色控制、瞄准和 HUD 没有异常。双目和 6DoF 尚未进入实现验证。
