# Native camera survey:2026-10-11

This page records the previous phase of the survey-only and mobile-link experiment. A temporary Hook collection has since been completed, dynamically calling evidence, withdrawing patches and current progress is shown [Native Call Validation](04-native-capture-validation.md). The description below, such as “calls not yet collected”, is in the state of investigation at that time.

## Purpose and current conclusions

We need to find the view portal that the game really uses for drawing images. We can understand the script camera as a front-end business state, and the rendering view as data used in actual mapping of the browser frame. The change of business status does not necessarily change data already sent to the renderer in time, so ultimately we need to track the transmission between the status and the drawing.

I've found it. `PlayerUpdateCameraNative` The corresponding primary calculation function and confirm that the output structure field matches the reflect information on running. This is evidence of original code, but still the calculation phase of a script VM-calling camera, where the final rendering view entry has not been found.

## Version and positioning

Target EXE SHA 256 Yes `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. All the following code locations are RVAs of relative module base addresses and cannot be directly used as addresses for other versions.

Finds with reflection fields of the current camera type `PlayerUpdateCameraNative` UFunction, read it again `+0x114` Execute the pointer. This deviation is from this version of the survey, not the generic UE offset. The packaging function RVA is `0x3b3af4`, the direct caller of the original subject RVA is `0x110878`。

Packaging Functions Read `FFrame.Code +0x28` bytes parameters, then RCX, RDX, XMM 2、R 9 And call the subject. The assumptions based on the countercompilation and script statements are camera objects, Pawn, Delta Time, output ViewTarget; ABI has not yet been verified through dynamic breakpoints.

## Output Structure Evidence

The TViewTarget layout given by the current refraction on running:

| Fields | Offset relative to TViewTarget |
| --- | --- |
| Target | 0x00 |
| Controller | 0x08 |
| POV | 0x10 |
| AspectRatio | 0x2c |
| PRI | 0x30 |

Verifyed in TPOV 0 Rotation is here. 12 FOV is here. 24. So, relative to ViewTarget, the location is `+0x10`♪ The rotation is `+0x1c`- FOV. - Yes. `+0x28`。

Original subject begins reading `[r9+0x28]`. The operation of storing output structures rotated and FOV fields in commands, as well as passing location field addresses to other functions, corresponds to the above structure layout. The following is still required to check the register ' s aliases and branch meanings, and cannot be identified by a numerical deviation of vouchers for all data flows.

## Tools and replicability

`tools/package-inspector/analyze_native_camera.py` Opens the process with query and read permission only, first re-defining the current camera, then reverses the compilation of the packaging function and its direct caller. It does not write memory, does not inject DLL, does not set breakpoints.

Usage mode (PID should be replaced by the current process):

```powershell
& 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' 'J:\gearsVr\tools\package-inspector\analyze_native_camera.py' --pid 32532 --output 'J:\gearsVr\analysis\native\camera-graph.json'
```

This time you get the packaging function 75 A code of accessible instructions, subject 895 bar, none of the command ceilings set. Tools follow the direct jump in the normal enforceable area and do not follow the call function; this is not a full function boundary restoration, and indirect jump may cause omissions.`.themida` and `.boot` The sections are excluded from the scope of the history.

Detailed JSON and compilations saved locally `analysis/native/`Git only saves our tools and text conclusions.

## Next steps and acceptance conditions

Tracks the original viewer / PlayerController reading and the path to position, rotation, projector parameters into rendering view structures. Candidate positions must first prove that the current player view is read, then verify the call frequency's relationship with actual render frames. The loss of focus will automatically suspend the game, so it cannot be stopped as a separate script VM Cardon evidence.

Only read the evidence to do a small reversible modification test; success criteria are that the image changes as expected, while role control, targeting and HUD are not abnormal. 6 DoF has not yet entered the validation.

## Follow-up read-only survey: candidate for original view construction

This round follows the player viewer event to a lower primary path. The following function name is derived from data stream use and is not the name recovered from the official symbol.

| Location (RVA) | Static evidence | Current judgement |
| --- | --- | --- |
| 0x1c37018 | FName read by the current process is indexed 7384、Number 0;names are resolved as GetPlayerViewPoint | The name variable in the module of a player 's point-of-view event |
| 0x15c968 | Read the above name, query function, then call the non-existent object table +0x1e8; copying two after returning 12 Byte Output | GetPlayerViewPoint Primary Event Packing Function |
| 0x15cf08 | From this+0x68 Take player controllers and call the above-mentioned packaging functions; read FOV; calculate view mouths using Origin/Size; generate two 4×4 Matrix and Construct View Objects | Strong candidate: LocalPlayer scene view build function |
| 0x15dd7c | Accept FOV derivatives and magnanimous relationships, build 64 Byte Matrix | Projectile Build Candidate |
| 0x15e428 | Copy Matrix to New Object +0x70 and +0xb0, then calculate more derivative data | View Object Constructive Function Candidate |
| 0x2c78f4 | Run Players Through, Call 0x15cf08, copy the location to LocalPlayer. LastViewLocation; then send the same view together to 0x26858c Equal Functions | Draw path candidate, the overall control stream is not fully analysed |

Respond validation on running: Current WarLocalPlayer `Actor` Offset is `0x68`The pointer is exactly the PlayerController found by the locator;`Origin` Yes. `0x98`，`Size` Yes. `0xa0`，`LastViewLocation` Yes. `0xe4`. These layouts are consistent with original commands. Read LastViewLocation `(113.31996, 720.62537, 283.45770)`, which corresponds to the current CameraCache position. This static sample only confirms that the current data match and does not support dynamic updating frequency.

0x15cf08 The parameter stream display RCX saves as LocalPlayer, R 8 Save as Location Output Address, R 9 Saves as a rotating output address. `0x15d0c5` The next command after the player viewer event is `0x15d0ca`; then read and rotate, calculate the view matrix and projection array, and then `0x15d989` Call the construction function. This " event return, before matrix calculation" position is worth further validation: if head-over positions are applied here, subsequent matrices can be generated by the game itself in a consistent manner.

This is still not a valid HOOK: no threads, call times or correspondence with Present have been collected and there is no proof that the path continues when script cameras stop. There are also branches in code using global point of view data; no dynamic confirmation of which branch is currently used. To be reliable, there is no movement to confirm which branch is currently used. 6 DoF, which must cover the actual path and process the life cycle of the view object, cannot modify only one cache or single matrix.

## Current round of tools and result boundary

- `scan_native_views.py` Scan boundary windows near the false object list, track direct branches of the normal code and screen for relevant deviations; support `--entry-rva` Checks the selected entry further. The window may cross the adjacent false list and the offset may belong to another structure, so the initial sift does not prove a functional identity.
- `trace_view_events.py` Searches for running-time name variables, text and RIP relative LEA/MOV references. The player view point reference is confirmed by FName values and the data stream of the packaging function.
- `find_native_callers.py` Search for direct CalL/JMP with a nearby command that points to the specified RVA inside the normal code.3 Fills provide only candidate boundaries and do not guarantee the starting point of a true function; they cannot independently prove that they are data from the start of the call instruction itself.
- `native-view-candidates.json` Saves the RVA candidate and limitations for this version for subsequent investigations; does not participate in autowriting or patches.

In the branch analysis: Event packaging function subject can be viewed directly; scene view candidate 547 A rule to reach command, a view construction candidate 633 Box, projection candidate 33 bar, none of which touches the upper limit. Draws a path candidate to reach 1600 The cap must be treated as an incomplete result.

Next dynamic validation starts with the call of a candidate function, the LocalPlayer identity, input output point view, matrix and thread to confirm it in relation to actual rendering frame; no change is made at the collection stage. After validation, changes are added to the small angle before taking OpenXR position and left-eye projection. Current work uses process querying and reading privileges, and the game and original package are not modified.

## The Mobile Link Experiment.

2026-10-11，PID 32532. New tools `sample_resolved_camera.py` Automatically wait for the front-stage focus and CameronCache time to advance, then make an appointment 50 Hz Read On-Access 35 seconds; always using query, read permissions.

Got it. 1718 A sample of all the games recorded as having front-stage focus. CameraCache Time from 66.67715 Increase to 101.66108 sec, maximum time increment observed in adjacent samples 0.04975 seconds; this is not a rendering frame count or used to prove the call frequency of the original candidate.

| Indicators | Observation |
| --- | --- |
| Different positions | 1667 One. |
| Different rotation | 1097 One. |
| X Scope | -251.70 Present. 278.76 |
| Y Scope | 720.53 Present. 1220.37 |
| Z Scope | 270.65 Present. 296.49 |
| FOV | Always. 80 |
| CameraCache and LastViewLocation | 0.14441 Game coordinates |
| Two places are different than that. 1 Units | 1715 / 1718- Yeah, I'm sure. 99.83% |
| Maximum location difference | 3.77636 Units |

Conclusion: When people move and rotate, the location and rotation of the auto locator is actually changed; LocalPlayer. LastViewLocation corresponds to the height of the CameraCache position. The two addresses are not atom-synchronized reading and may be a different frame, and individual differences cannot be directly miscalculated.

This supports the connection between current player camera positioning and raw view candidates. No candidate function is set to hook, so no proof of threads, actions per frame call, script stop-up or modified images are available. The person is controlled by the player; tools do not control the person or apply VR position.

Original CSV and Abstract JSON stay locally `analysis/runtime/camera-movement-validation.*`, Git saves the sampling tool and this section's conclusions.

---

## 中文

# 原生摄像机调查：2026-10-11

本页记录前期只读调查与移动关联实验。其后已完成一次临时 Hook 采集，动态调用证据、撤销补丁核验和当前进展见 [原生调用验证](04-native-capture-validation.md)。下文“尚未采集调用”等描述属于当时的调查状态。

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

## 后续只读调查：原生视图构建候选

这一轮沿着玩家视点事件追到了更靠后的原生路径。下述函数名是根据数据流推断的用途，不是从官方符号恢复的名称。

| 位置（RVA） | 静态证据 | 当前判断 |
| --- | --- | --- |
| 0x1c37018 | 当前进程读取的 FName 为索引 7384、Number 0；名称解析为 GetPlayerViewPoint | 玩家视点事件的模块内名称变量 |
| 0x15c968 | 读取上述名称，查询函数，再调用对象虚表 +0x1e8；返回后复制两个 12 字节输出 | GetPlayerViewPoint 原生事件包装函数 |
| 0x15cf08 | 从 this+0x68 取玩家控制器，调用上述包装函数；读取 FOV；使用 Origin/Size 计算视口；生成两个 4×4 矩阵并构造视图对象 | 强候选：LocalPlayer 场景视图构建函数 |
| 0x15dd7c | 接受 FOV 派生参数和宽高关系，构建 64 字节矩阵 | 投影矩阵构建候选 |
| 0x15e428 | 复制矩阵到新对象 +0x70 和 +0xb0，随后计算更多派生数据 | 视图对象构造函数候选 |
| 0x2c78f4 | 遍历玩家，调用 0x15cf08，把位置复制到 LocalPlayer.LastViewLocation；之后把同一视图集合传给 0x26858c 等函数 | 绘制路径候选，整体控制流尚未分析完整 |

运行时反射验证：当前 WarLocalPlayer 的 `Actor` 偏移是 `0x68`，其指针确实等于定位器找到的 PlayerController；`Origin` 是 `0x98`，`Size` 是 `0xa0`，`LastViewLocation` 是 `0xe4`。这些布局与原生指令一致。读取 LastViewLocation 得到 `(113.31996, 720.62537, 283.45770)`，与当前 CameraCache 的位置一致。这个静止样本只确认当前数据相符，不能证明动态更新频率。

0x15cf08 的参数流显示 RCX 保存为 LocalPlayer、R8 保存为位置输出地址、R9 保存为旋转输出地址。在 `0x15d0c5` 调用玩家视点事件后，下一条指令是 `0x15d0ca`；之后读取位置和旋转，计算视图矩阵和投影矩阵，再在 `0x15d989` 调用构造函数。此“事件返回后、矩阵计算前”的位置值得进一步验证：如果在这里应用头显位姿，后续矩阵可以由游戏自身一致地生成。

这仍不是已可用的 hook：尚未采集执行线程、调用次数或与 Present 的对应关系，也未证明脚本摄像机停更时此路径是否继续。代码里还存在使用全局视点数据的分支；未动态确认当前使用哪个分支。要实现可靠 6DoF，必须覆盖实际路径并处理视图对象生命周期，不能只修改一个缓存或单个矩阵。

## 本轮工具和结果边界

- `scan_native_views.py` 扫描对象虚表附近的有界窗口，追踪普通代码的直接分支，筛选相关偏移；支持 `--entry-rva` 进一步检查已选入口。窗口可能跨到相邻虚表，同偏移可能属于别的结构，所以初筛不证明函数身份。
- `trace_view_events.py` 搜索运行时名称变量、文字及 RIP 相对 LEA/MOV 引用。通过 FName 值和包装函数数据流确认了玩家视点事件引用。
- `find_native_callers.py` 搜索普通代码内指向指定 RVA 的直接 CALL/JMP，附带附近指令。INT3 填充只提供候选边界，不保证真实函数起点；从调用指令本身开始反汇编也不能独立证明它不是数据。
- `native-view-candidates.json` 保存本版本的候选 RVA 和局限，供后续调查使用；不参与自动写入或补丁。

有界分支分析中：事件包装函数的主体可直接查看；场景视图候选 547 条可达指令，视图构造候选 633 条，投影候选 33 条，均未触及上限。绘制路径候选达到 1600 条上限，必须当作不完整结果。

下一次动态验证先记录候选函数的调用、LocalPlayer 身份、输入输出视点、矩阵和线程，结合实际渲染帧确认；采集阶段不改变视图。验证通过后再加小角度变换，之后才接 OpenXR 位姿与左右眼投影。当前工作使用进程查询和读取权限，游戏及原始包没有修改。

## 人物移动关联实验

2026-10-11，PID 32532，玩家自行恢复游戏并走动、转镜头。新工具 `sample_resolved_camera.py` 自动等待前台游戏焦点和 CameraCache 时间推进，再以约 50 Hz 连续读取 35 秒；始终使用查询、读取权限。

得到 1718 个样本，全部记录为游戏拥有前台焦点。CameraCache 时间从 66.67715 增加到 101.66108 秒，相邻采样观察到的最大时间增量约 0.04975 秒；这不是渲染帧计时，也不用于证明原生候选的调用频率。

| 指标 | 观察值 |
| --- | --- |
| 不同位置 | 1667 个 |
| 不同旋转 | 1097 个 |
| X 范围 | -251.70 到 278.76 |
| Y 范围 | 720.53 到 1220.37 |
| Z 范围 | 270.65 到 296.49 |
| FOV | 始终 80 |
| CameraCache 与 LastViewLocation 位置差的 RMS | 0.14441 个游戏坐标单位 |
| 两处位置差不超过 1 单位 | 1715 / 1718，约 99.83% |
| 最大位置差 | 3.77636 单位 |

结论：人物移动和转镜头时，自动定位器读取到的位置、旋转确实变化；LocalPlayer.LastViewLocation 与 CameraCache 位置高度吻合。两个地址并非原子同步读取，且可能属于不同帧，不能把个别差异直接判为错误。

这支持当前玩家摄像机定位和原生视图候选的数据关联。没有对候选函数设置 hook，因此没有验证其线程、每帧调用、脚本停更时的行为或修改后的画面。人物由玩家控制；工具没有控制人物，也没有应用 VR 位姿。

原始 CSV 和摘要 JSON 留在本地 `analysis/runtime/camera-movement-validation.*`，Git 保存采样工具与本节结论。
