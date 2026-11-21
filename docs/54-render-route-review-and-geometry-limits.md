# Rendering the structure review and local geometry observation boundary

2026-10-13. Leave the current chat as requested and complete the architecture review first. The right black screen has not been repaired, because root is not yet established. The second round of observation codes have been compiled but not enabled and cannot be used as operational evidence.

## Diagnosis

The current double view does not simply repeat Precent or the final quadrilateral: two original structures have entered into the consumption, preparation and cleaning process of the rendering object. However, CPU multi-view processes do not create the right world geometric pixels with both eyes.

`13cfcc` This function is not complete, Scene Render. The functions are entered around the latest observations, the step-by-step dynamic list is empty and there are a large number of Ds in both right and left rectangles in the same experiment.3D 12 Draw. This means that local observations are not sufficient to cover the mapping of the scene, and cannot be interpreted as “no object in the right eye” or “no Draw in the right eye”.

The next most valuable evidence is the same middle scene colour, final processing of input and backbuffer content and reading/ writing relationships. Continue changing or increasing the ultimate quad parameter `13cfcc` Access logs, not answering directly which border the right colour is missing.

## Levels and Key Functions

The following are all working names; the address is the current validated version of RVA, not an official symbol.

| Level | Access | Evidence available and restrictions |
| --- | --- | --- |
| Source View and Pool Submission | `3a148c`、`116250` | Construct two views to enter the original rendering object; not only copy the final image |
| Render object consumption and destruction | `15a7f4`、`15a80c` | The object is cleaned after consumption and cannot be called simply repeatedly to the same object |
| Phase of the scenery | `11908c` | Preparation of view, selection to draw branch, post-implementation processing; 0–3 Not the right-and-right eye index. |
| Optional Drawing Branch | `170948`、`e1d004`、`e1e0f8`、`e1ff64` | Selected by sign of renderer; no current developments have confirmed the actual performance of branches |
| Current View Bind | `173170` | Writing Context `+14c0` and process matrix-related data; does not prove the GPU constant to be ultimately correct |
| Local Object Drawing | `13cfcc` | List of static candidates and phased dynamic; observation ranges only partially mapped |
| Final processing per view | `2b0a98 → 虚表+8 → d82f58 → 1af42c` | Existing running stacks and parameters evidence established; source pixels still unverified |
| Present / OpenXR | D 3D 12 / XR Submit | The right-hand malfunction of the desktop occurred before XR; XR test diagrams successfully do not prove a double-eye success |

Document 53 The road map is only open. `170948`, does not mean all drawing branches. Run a snapshot back-compilation `e1ff64` And all over the place, renderer. `+68/+70` View array (stepping) `D30`) set the rectangle, call `173170`Let's go. `13e49c`、`13e3ac`、`13e340` . The view data it uses contains `+480` Area. The actual branch needs to be identified, and it can't be assumed that all the worlds have been drawn. `13cfcc`。

The source view structure adjusts the position and re-uses the original tectonic process, but the eye deviation is still experimental in the game unit and maintains the original projection. Even if it is restored on the right side, it requires a correct width ratio, IPD/unit conversion, per eye projection and geometric visual differential verification.

## Completed observations

PID 48208：10 Two-second view test,2,014 Life cycle analysis is passed and stopped. 23 A single view is restored by checking the entry match, without reservation or skipping. These are the result of the object ' s life cycle and not the acceptance of a stereo image.

| Observation | Left View | Right View |
| --- | --- | --- |
| `13cfcc` Phase 1/2, Mode 0/1 Total Call | 8,056 | 8,056 |
| Number of phased dynamic lists observed | Both. 0 | Both. 0 |
| Line during Function Execution Directd | 0 | 1 I'm not sure.69 An element |
| View Rectangle | `0,0,960,1080` | `960,0,960,1080` |

The exact right-and-right rectangle of the same test. Draw count is for each of the two 729,735 / 728,418; rectangles are only the drawing of regional evidence, not the world geometry classification and do not prove that the drawing results are not black. `+1a0` It's... 64 The byte matrix summary is the same, and cannot be used to determine that the full view or actual GPU projection is the same; the exact semantics of the field are still to be verified. Visibility summaries do not support the validity of visible data either.

This time no prime descriptionor tracks, so the target in the record cannot be used 0 The judgement game binds empty RT. Observations only count the same thread `DrawInstanced/DrawIndexedInstanced`, does not cover ExecuteIndirect, other threads or complete resource dependence.

`geometry-active-48208.jpg` The actual photo taken after the end of the two-view window cannot be used as pixel evidence during the test. The analyser output `engine_view_draw_summary` empty;context drew value 0 This means that the observation data are not available and not actually not drawn.

Local evidence:

- `analysis/runtime/geometry-48208.status.txt`: Original polymerization record.
- `analysis/runtime/geometry-48208-lifecycle.json`、`geometry-48208-restored.json`: Life cycle and access recovery.
- `analysis/runtime/geometry-48208-observation.json`: This read-only analysis is adopted, clear `pixels_verified=false`、`stereo_verified=false`。
- `analysis/ghidra/render-16812-verified/decompiled/`: Scattered transcribed and compiled while running in direct direction.

Another prime start PID 25604 The exit is pending for a reason, and no clean stop evidence exists. It cannot be recorded as success or directly attributed to the new observation function. 44960 Only pre-encumbered versions were loaded and the second round of capture was not initiated; the game process still exists at the time of review.

## Disk means different from running time code

The original Ghidra MCP project analyses disk images. The run-time snapshot obtained after normal start is different from the byte of the drive entry; this means that the object must be aligned first, and not simply by the bytes in which " a lot of Hook" is written. The operation of the protection code may cause differences, loader processing or patches to be validated by address. This round of independent headless projects can analyse the snapshots, not equal to the fact that the original MCP project has been switched to the run-time code.

## API and the failure of the frame

Actual device, queue and Draw path support D 3D 12 Judge. Historical device import calls include D 3D 12 CreateDevice Reference `373dd7/3742ef/417dd7`，CreateDXGIFactory 1 References `417e3e`; import references do not in themselves equal the number of branches that have been run.

RinderDoc smoke successfully only validates the tool base. The game does not record successful registration of API, and the old way to replace imported pointers collapses without repeating the program. There is insufficient evidence to distinguish between creation times, object packaging or injection conflict.

PIX has a home-grown smoke capture; the game log failed foreground framer and is not sufficient to prove that the game is incompatible. The start, time of device creation and effective capture files should be verified in the process of closing the original Mod injection before double viewing.

## Assumptions with the next minimum experiment

| To verify assumptions | Support or weaken its next piece of evidence |
| --- | --- |
| Right world geometry does not produce a valid color | The main colour of the same frame is missing on the right; continuing to locate Draw, constant and visibility that generated the resource |
| The scene colour is valid, but the post-processing input, screening or shared resource coverage is incorrect | The scene stage is valid on the right side, and one subsequent writing or reading is invalid |
| Final sampling binding error | Final input resource is valid per se, but final output is still dark; actual SSV, subresource and shadow constants need to be checked |

The current priority range is between the scene colour generation and final processing input, for which it is not possible to state the cause.

The next round will only answer "Where did the first blackening of the right side of the same frame":

1. The original process performs a controlled PIX capture validation to confirm that you can access files with real game GPU events. Record the specific failure phase; do not try the old RenderDoc hand replacement.
2. When captured, saves the scene colour, final shadow input and backbuffer, and links resources ID, subresource, video/scissor and event sequence in a two-dimensional view experiment that keeps cameras, visions and quad parameters constant.
3. If the frame is still not available, first specify the queue, status and fence of the identified resource and then design a fixed-point reading back; do not copy texture directly in unknown condition. The existing Draw context/first call stack observation can help select specific functions but cannot replace GPU pixels.

Receiving and inspection requires that the frame-to-peer image contains correct world geometry, with a credible mask and visual difference in its position; copying the left chart, the final call equals or Hook does not have enough self-measurement. Desktop SBS is then reconnected to the OpenXR output after the game has been passed.

## Code Status

Geometric observation versions have been compiled, self-measured and completed the above-mentioned in a defined game by real MinHook parameters/return values/entry. The current Draw version of the current engine view has been compiled, is measured by geometry but no new TLS attribution logic or enabled in the game has yet been validated. The current view may remain during the shared and reprocessing phase, which cannot automatically be equated with the true owner of Draw.

This review only re-analyses and supplements the files, does not enable the second round of probes to continue changing the rendering behaviour. All changes are still pending and interactive design modifications already available to users are retained.

---

## 中文

# 渲染架构复审与局部几何观察的边界

2026-10-13。按用户要求留在当前聊天，先完成架构审查。右侧黑屏尚未修复，根因尚未确定。第二轮观察代码已编译但未启用，不能作为运行证据。

## 诊断结论

当前双视图并非只重复 Present 或最终四边形：两份原生构造的视图已经进入渲染对象的消费、准备与清理流程。但是，CPU 多视图流程成立不等于两眼都产生了正确的世界几何像素。

`13cfcc` 不能代表完整 Scene Render。最新观察中左右都进入该函数，左右分阶段动态列表都为空，而同一试验中左右矩形都有大量 D3D12 Draw。这说明局部观察不足以覆盖场景绘制，不能把它解释为“右眼没有场景对象”或“右眼没有任何 Draw”。

最有价值的下一项证据是同一帧中场景颜色、最终处理输入与 backbuffer 的内容及读写关系。继续改变最终 quad 参数，或继续增加 `13cfcc` 入口日志，都不能直接回答右侧颜色在哪个边界丢失。

## 层次与关键函数

以下均为工作名称；地址是当前已校验版本的 RVA，不是官方符号。

| 层次 | 入口 | 已有证据及限制 |
| --- | --- | --- |
| 源视图与集合提交 | `3a148c`、`116250` | 构造两份视图，进入原生渲染对象；不是只复制最终图像 |
| 渲染对象消费与销毁 | `15a7f4`、`15a80c` | 消费后会清理对象，不能对同一对象简单重复调用 |
| 场景阶段编排 | `11908c` | 准备视图、选择绘制分支、执行后处理；阶段 0–3 不是左右眼索引 |
| 可选绘制分支 | `170948`、`e1d004`、`e1e0f8`、`e1ff64` | 由 renderer 标志选择；当前尚未动态确认各分支的实际执行情况 |
| 当前视图绑定 | `173170` | 写入上下文 `+14c0` 并处理矩阵相关数据；不证明 GPU 常量最终正确 |
| 局部对象绘制 | `13cfcc` | 含静态候选及分阶段动态列表；观察范围只覆盖部分绘制 |
| 每视图最终处理 | `2b0a98 → 虚表+8 → d82f58 → 1af42c` | 既有运行栈与参数证据成立；源纹理像素仍未验证 |
| Present / OpenXR | D3D12 / XR 提交 | 桌面右侧故障已在 XR 之前出现；XR 测试图成功不能证明游戏双目成功 |

文档 53 的路径图只展开了 `170948`，不足以表示全部绘制分支。运行时快照反编译中的 `e1ff64` 也遍历 renderer `+68/+70` 的视图数组（步幅 `D30`），设置矩形、调用 `173170`，再走 `13e49c`、`13e3ac`、`13e340` 等绘制候选。它使用的视图数据包含 `+480` 区域。需要先确认实际分支，不能假定所有世界绘制都经过 `13cfcc`。

源视图构造会调整位置并重新调用原生构造流程，但当前眼偏移仍是实验性的游戏单位偏移，保留原投影。即使右侧恢复，也还需要正确的宽高比、IPD/单位换算、每眼投影及几何视差验证。

## 已完成的有界观察

PID 48208：10 秒双视图试验，2,014 次渲染对象提交、消费、准备和销毁。生命周期分析通过，停止后 23 个入口校验匹配，无保留或跳过对象，恢复单视图。这些是对象生命周期结果，不是立体画面验收。

| 观察项 | 左视图 | 右视图 |
| --- | --- | --- |
| `13cfcc` 阶段 1/2、模式 0/1 的总调用 | 8,056 | 8,056 |
| 所观察分阶段动态列表数量 | 均为 0 | 均为 0 |
| 函数执行期间同线程直接 Draw | 0 | 1 次 indexed，69 个元素 |
| 视图矩形 | `0,0,960,1080` | `960,0,960,1080` |

同一试验的精确左右矩形 Draw 计数分别为 729,735 / 728,418；矩形仅是绘制区域证据，不是世界几何分类，也不证明绘制结果非黑。两侧 `+1a0` 的 64 字节矩阵摘要相同，不能据此认定完整视图或实际 GPU 投影相同；该字段的准确语义仍待验证。可见性摘要不同也不证明可见性数据正确。

本次未 prime 描述符追踪，因此不能用记录中的目标 0 判断游戏绑定了空 RT。观察只统计同线程 `DrawInstanced/DrawIndexedInstanced`，不覆盖 ExecuteIndirect、其他线程或完整资源依赖。

`geometry-active-48208.jpg` 实际在双视图窗口结束后拍摄，不能当作试验期间像素证据。分析器输出中的 `engine_view_draw_summary` 为空；其 context draw 数值 0 表示这版没有该观察数据，不是实际没有绘制。

本地证据：

- `analysis/runtime/geometry-48208.status.txt`：原始聚合记录。
- `analysis/runtime/geometry-48208-lifecycle.json`、`geometry-48208-restored.json`：生命周期与入口恢复。
- `analysis/runtime/geometry-48208-observation.json`：本次只读重分析通过，明确 `pixels_verified=false`、`stereo_verified=false`。
- `analysis/ghidra/render-16812-verified/decompiled/`：定向运行时快照反编译及汇编。

另一次 prime 启动 PID 25604 在进入关卡前退出，原因未定，没有干净停止证据。不能把它记为成功或直接归因于新观察函数。PID 44960 只加载预留版本，未启动第二轮捕获；复审时游戏进程仍存在。

## 磁盘与运行时代码不同的含义

原 Ghidra MCP 项目分析的是磁盘映像。正常启动后取得的运行时快照与磁盘入口字节不同；这说明必须先对齐分析对象，不能仅凭字节不同推断“写了很多 Hook”。保护代码的运行时展开、加载器处理或补丁都可能造成差异，具体来源需逐地址验证。本轮独立 headless 项目可分析快照，不等于原 MCP 项目已切换到运行时代码。

## API 与抓帧失败的判断

实际设备、队列及 Draw 路径支持 D3D12 判断。历史设备导入调用点包括 D3D12CreateDevice 引用 `373dd7/3742ef/417dd7`，CreateDXGIFactory1 引用 `417e3e`；导入引用本身不等于每个分支都运行过。

RenderDoc smoke 成功只验证了工具基础能力。游戏内没有成功注册 API 的记录，替换导入指针的旧方式发生崩溃，不能重复该方案。尚无足够证据区分创建时机、对象包装或注入冲突。

PIX 有本机 smoke 捕获；游戏日志中的 foreground privilege 失败不是成功抓帧，也不足以证明游戏不兼容。应先在关闭 Mod 注入的原版进程中验证捕获启动、设备创建时机及有效捕获文件，再进行双视图对照。

## 假设与下一轮最小实验

| 待验证假设 | 支持或削弱它的下一项证据 |
| --- | --- |
| 右侧世界几何没有生成有效颜色 | 同帧主场景颜色已经缺右侧；继续定位产生该资源的 Draw、常量和可见性 |
| 场景颜色有效，但后处理输入、清屏或共享资源覆盖有误 | 场景阶段右侧有效，某次后续写入或读取后失效 |
| 最终采样绑定错误 | 最终输入资源本身有效，但最终输出仍黑；需检查实际 SRV、子资源与 shader 常量 |

当前优先检查范围是场景颜色生成到最终处理输入之间，尚不能对其中某一项宣布根因。

下一轮只回答“同一帧右侧第一次在哪里变黑”：

1. 原版进程进行一次受控 PIX 捕获验证，确认获得可打开且有实际游戏 GPU 事件的文件。记录具体失败阶段；不重试旧 RenderDoc 指针替换方式。
2. 捕获可用后，在保持相机、视区和 quad 参数不变的有界双视图试验中，保存场景颜色、最终 shader 输入和 backbuffer，并关联资源 ID、子资源、viewport/scissor 和事件顺序。
3. 若抓帧仍不可用，先明确已识别资源的队列、状态与 fence，再设计定点回读；不要在未知状态下直接复制纹理。现有待启用的 Draw 上下文/首个调用栈观察可帮助选具体函数，但不能替代 GPU 像素。

验收要求：同帧左右图像包含正确世界几何，并随眼位置呈现可信遮挡和视差；复制左图、最终调用次数相等或 Hook 自测通过均不够。桌面 SBS 通过后再接游戏双眼 OpenXR 输出。

## 代码状态

几何观察版本已编译，通过真实 MinHook 参数/返回值/入口恢复自测并完成上述一次有界游戏试验。更广的引擎当前视图 Draw 归属版本已编译、通过原几何自测，但尚未验证新 TLS 归属逻辑或在游戏中启用。当前视图可能残留在共享和后处理阶段，不能把它自动等同于 Draw 的真实所有者。

本次复审只重分析和补文档，未启用第二轮探针，未继续改变渲染行为。所有修改仍未提交，用户已有的交互设计修改保留。
