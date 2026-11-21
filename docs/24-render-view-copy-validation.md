# Dynamic validation of rendering view copy

2026-10-12 Game version SHA 256 Same as existing profile. First person maintains the Mod installation, minimum setup/ 1080 p, single person level. This round is only for viewing, no second eye, and no change in the status of a person.

## Rationale and scope

Previous round of confirmations for original submission 0x3a148c Receives player SceneView. Runtime command extrapolates: collect access 0x116250 Render object construction, repeat 0x193d50 Creates each rendering view. This time add two parameters to the latter entry, return the pointer's Hook, and verify the actual input output of this step.

The assembly Hook identifies the current submission range with a linear local variable, and copys Hook only to record calls within that range. The original function is always executed as it is. Read source view before copying +0x70 It's... 128 byte matrix regions;repeated and read the same area after returning and +0 . Read-only is not equal to zero cost, and the diagnostic records still have operating costs.

No new Hook is used for head-starting or stereo rewrite. It only observes the time of structural return; the subsequent group rewriting, rendering thread consumption and destruction are not verified dynamically.

## Tools and Independent Validation

- `family-diagnostics` Add Kind=8 Render View Copy Record, CSV Add lines_source, line_owner and 32 A number of source matrices.
- valid 128= A collection of arrays that read successfully.256=The source matrix was read successfully.512= Target matrix reading succeeded.1024= The target is the subject of a successful group reading; these definitions apply only to duplicate records.
- New entry version signature, start early springboard preset, pre-injection and DLL internal verification extend to 0x193d50. New build must use a new game process, which cannot replace the already existing DLL.
- `CaptureSelfTest.exe <DLL> --clone` The normal camera and assembly ABI self-measures are also passed.
- `analyze_clone.py` Check source view from current collections, target using independent storage, return values equal to targets, matrix values consistent and limited, initial builder owner, caller, ignore point rewrite, Precent return value and access restoration.

Build As `tools/native-capture/build-clone/GearsCaptureClone.dll`, only diagnostic products, not VR Mod deployable.

## Actual Game Results

PID 32968 Autostart and enter level, sample 20 sec:

| Item | Result |
| --- | --- |
| Render View Copy Record | 5,297 |
| Caller | All 0x116444 |
| Copy Threads | All 34392,Systems with surrender |
| Different source/ target address | 231 / 165 |
| Different Matrix Groups | 545 |
| Source, target matrix region match | All records |
| Source belongs to the submitting array, with different addresses | All records |
| Return value equals target address | All records |
| Construct the initial owner equals entering a collective address | All records |
| scene view / group submission / Present | 5,298 / 5,297 / 5,296 |
| Failed to discard record / Present | 11 / 0 |
| Activate / Six Entry Check After Stop | 0 / Restor All |

The number of addresses indicates a change in the reuse and distribution during observation, which does not mean that there are objects who survive at the same time. 128 bytes, does not represent that the entire object can be directly memcpy or confirm the coordinates of the matrix.

Local original evidence:`analysis/runtime/reserve-32968.json`、`clone-32968.{csv,json,status.txt}`、`clone-32968-restored.json`、`clone-32968-correlation.json`. The original evidence is left on J disk, ignored by warehouse rules; source code and this report are submitted to Git.

## Follow-up access location

The dynamic evidence now supports the path of " Player SceneView Quest Pool to submit a copy of the  ⁇  Render View". A copy is still pointed at the input pool after it has been constructed; the running time command shows 0x116455 Then re-direct the rendering to its own collection. Next, look at the whole thing. 0x116250 The assembly, view array and homeer at the end of the construction check actual consumption and destruction.

The eye requires each other’s point of view, asymmetric projection and corresponding state. It cannot be achieved by pressing two identical points into a collection as stereo. This wheel has not produced eyes or integrated the GPU display and head-motion prototype.

We've also encountered short-lived non-combustion. 1080 p window size; menu tool does not send any input when it does not match resolution, waiting to return to the verified dimensions and menu images, still subject to total time limits. The current game's five-step navigation is actually passed; no matching resolution waits for branches to be covered separately through second start.

---

## 中文

# 渲染视图副本的动态验证

2026-10-12，游戏版本 SHA256 与既有 profile 一致。第一人称 Mod 保持安装，最低设置 / 1080p，单人关卡。此轮只观察，不改视图、不新增第二只眼、不改变人物状态。

## 原理与范围

上一轮确认原生集合提交 0x3a148c 接收玩家 SceneView。运行时指令推断：集合进入 0x116250 渲染对象构造，再经 0x193d50 创建每个渲染视图。此次在后者入口增加两参数、返回指针的 Hook，验证这一步的实际输入输出。

集合 Hook 用线程局部变量标识当前提交范围，复制 Hook 只记录该范围内的调用。原函数始终原样执行。复制前读取源视图 +0x70 的 128 字节矩阵区域；复制返回后读取目标相同区域及 +0 的所属集合指针。只读并不等于零开销，诊断记录仍有运行成本。

没有将新增 Hook 用于头动或立体改写。它只观察构造返回时刻；后面的所属集合重写、渲染线程消费和销毁暂未动态验证。

## 工具与独立验证

- `family-diagnostics` 增加 kind=8 渲染视图复制记录，CSV 追加 clone_source、clone_owner 和 32 个源矩阵数值。
- valid 128=集合数组读成功，256=源矩阵读成功，512=目标矩阵读成功，1024=目标所属集合读成功；这些定义只适用于复制记录。
- 新入口版本签名、启动早期跳板预留、注入前与 DLL 内核验均扩展到 0x193d50。新构建必须使用新游戏进程，不能替换已驻留 DLL。
- `CaptureSelfTest.exe <DLL> --clone` 通过真实 trampoline 检查参数和返回值、矩阵采样、原函数转发两次、只记录提交范围内一次，以及停止后的入口恢复。普通相机和集合 ABI 自测也通过。
- `analyze_clone.py` 检查源视图来自当前集合、目标使用独立存储、返回值等于目标、矩阵数值一致且有限、构造初始 owner、调用者、无视点改写、Present 返回值和入口恢复。

构建为 `tools/native-capture/build-clone/GearsCaptureClone.dll`，仅诊断产物，不是可部署 VR Mod。

## 实际游戏结果

PID 32968，自动重新启动并进入关卡，采样 20 秒：

| 项目 | 结果 |
| --- | --- |
| 渲染视图复制记录 | 5,297 |
| 调用者 | 全部 0x116444 |
| 复制线程 | 全部 34392，与集合提交同线程 |
| 不同源地址 / 目标地址 | 231 / 165 |
| 不同矩阵组 | 545 |
| 源、目标矩阵区域相符 | 全部记录 |
| 源属于提交数组，目标地址不同 | 全部记录 |
| 返回值等于目标地址 | 全部记录 |
| 构造初始 owner 等于输入集合地址 | 全部记录 |
| 场景视图 / 集合提交 / Present | 5,298 / 5,297 / 5,296 |
| 丢弃记录 / Present 失败 | 11 / 0 |
| 停止后 active / 六处入口核验 | 0 / 全部恢复 |

地址数量表示观察期间的复用和分配变化，不代表同时存活对象数量。存在丢弃记录，不能宣称一对一无损追踪。矩阵一致只覆盖记录的 128 字节，不代表整个对象可直接 memcpy，也不确认矩阵的坐标约定。

本地原始证据：`analysis/runtime/reserve-32968.json`、`clone-32968.{csv,json,status.txt}`、`clone-32968-restored.json`、`clone-32968-correlation.json`。原始证据留在 J 盘、按仓库规则忽略；源代码和本报告提交 Git。

## 后续接入位置

现在动态证据支持“玩家 SceneView → 集合提交 → 渲染视图副本”这段路径。副本刚构造完仍指向输入集合；运行时指令显示 0x116455 后续改指向渲染器自己的集合。下一步观察整个 0x116250 构造结束时的集合、视图数组及 owner，再检查实际消费与销毁。

双眼需要各自的视点、非对称投影和相应状态，不能把两个相同指针塞进集合就当作立体实现。此轮尚未生成双眼，也未将 GPU 显示和头动原型整合。

启动过程中还遇到短暂的非 1080p 窗口尺寸；菜单工具在不匹配分辨率时不发送任何输入，等待回到已核对的尺寸和菜单图像，仍受总时限限制。当前游戏的五步导航实际通过；不匹配分辨率等待分支尚未通过第二次启动单独覆盖。
