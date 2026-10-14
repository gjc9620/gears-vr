# 最终输入绑定与实际绘制路径

更新至 2026-10-14。同帧双视图、原生投影响应、真实 XR→同帧双场景的桌面头动，以及 Meta XR Simulator 中 120 秒静态游戏 projection 已通过受限验证。PID 33860 完成 6,746 对场景，发布并接收 6,732 张游戏图像，提交 8,637 帧 projection，停止后恢复 mono。PID 40776 的 30 秒显式模拟头动游戏 projection 及独立矩阵审计也已通过；raw 模拟来源不等于物理追踪。PID 43724 持续模式实际输出跨过 180 秒且明确停止正常，关闭尾部发布/接收差 1 张尚未通过严格计数验收。真实头显游戏显示、物理尺度、活跃 TAA 尚待验收。接收端游戏图像已回读，引擎中间纹理内容仍未逐项证明。下文按实验顺序保留证据。

## 本轮改变了什么判断

PID 48772 的十秒试验中，两眼最终 quad 前的第一次 CPU 输入状态采样均成功：四个纹理绑定槽非空，两眼绑定同一组对象。按引擎当前视图归属的同线程直接 Draw 两侧都有大量记录，首个主绘制栈可以回溯到 `11908c → 27e580`。

因此，不应再围绕 `13cfcc` 的空列表推断右眼没有世界绘制。现有证据更支持继续检查共享纹理内容、其写入范围和后续处理。共享对象可以承载正常 SBS，绑定相同本身不构成错误证据。

## 定向反编译结果

新增 `DecompileSnapshotTargets.java` 在独立 runtime 项目中按显式入口导出，不执行全程序自动分析，使用 headless `-readOnly`，项目变更被丢弃。前三个目标及五个补充目标均反编译成功，并导出了汇编。

| RVA | 观察到的逻辑 |
| --- | --- |
| `1665dc` | 按 view `+3e8` 与另一模式参数从 `1c3b410/1c3b488` 等候选选择输入，交给 `1b4ca0`；最终调用者的该模式参数为 0 |
| `1b4ca0` | 第六参数非空时先减 `13c`，读取 wrapper `+138` 的索引，再取 `+38 + index*8` 对象；写入 context `(slot+ed)*8`，标记 `+a1` 脏状态；另一路参数写入 `+ce8 + index*8` |
| `1b36f4` | 对象槽赋值及引用计数增减，不是 GPU 复制 |
| `2754f4` | 参数计算后也调用 `1b4ca0`，不能把它整体称为单一 SceneColor 绑定函数 |
| `166960/1b0b14/275638/da2588` | 惰性初始化并返回全局状态候选，准确类型未恢复 |

关键汇编点：`1b4cfb` 读取 wrapper 索引，`1b4d01` 取对象，`1b4d33` 计算 `slot+ed`，`1b4d47` 更新对象槽，`1b4d60` 标记脏状态。这些支持 CPU 绑定槽判断，尚未证明对象的 COM 接口或 D3D12 SRV。

输出：`analysis/ghidra/render-16812-verified/final-inputs/`、`input-resources/`。动态栈中的地址是返回点；把 `27eb6c` 当入口导出时，脚本发现它位于已有函数内并拒绝，没有错误创建新函数。其所属函数为已导出的 `27e580`；`119342` 直接调用它，返回点 `119347` 与实测栈一致。

## 最小观察改动与验证

`final_input_observer.h` 从现有最终 quad Hook 读取当前视图、绑定槽和候选 wrapper。没有增加新 Hook，没有调用对象虚方法，没有修改 GPU 状态。每眼只保存第一次有效样本，之后只累计调用数；槽读取最多 32 个，wrapper 索引只接受 0–7。未读取的更大索引不能自动标成无效资源。

新增解码自测覆盖槽数上限、wrapper 索引越界、无效地址拒绝、数据不变。真实 MinHook 几何/quad 自测通过，原型参数转发和入口恢复检查通过。构建 `build-finalinputs/GearsCaptureFinalInputs.dll` 成功。

运行方式：新启动时只预留入口，进入已观察确认的关卡后启用 25 秒采集，双视图限十秒，背景试验、原有输出 bounds 和 viewport diagnostics 保持启用；未 prime 描述符追踪。

## 动态结果

| 项目 | 左眼 | 右眼 |
| --- | --- | --- |
| 最终输入观察次数 | 1,744 | 1,744 |
| 首样本 selector | 0 | 0 |
| 首样本候选全局地址 | `141c3b410` | `141c3b410` |
| 首样本 selected | `931f2abc` | `931f2abc` |
| 首样本 wrapper 索引 | 0 | 0 |
| 首样本对应绑定对象 | `80d8fe00` | `80d8fe00` |
| 首样本槽 0–3 | `80d8fe00,80d8f380,3e85f5300,1622c51380` | 相同 |
| 当前视图归属的直接 Draw 总数 | 691,001 | 684,931 |
| 其中最终 quad Draw | 1,744 | 1,744 |

Draw 归属来自 context 当前 view，经 renderer 数组与 `D30` 步幅校验；unassigned=0、归属聚合 overflow=0。它不覆盖其他线程、ExecuteIndirect，也不能排除共享处理时残留当前 view。首个非最终 indexed 路径两眼均包含 `1acf07 → 1778fb → 17651b → 1a84d6 → 16b4d1 → 1697d2 → f18da → 27eb6c → 119347`。`27e580` 的静态代码确有逐视图绑定与 `D30` 遍历，但这些 Draw 的 pass 类型、颜色写掩码和实际像素仍未确认。

1,744 对对象提交、消费、准备及销毁；十秒跨度 9.999275 秒，生产关闭、pending/retained/skipped 均 0，恢复单视图。持续双视图专用分析器 `analyze_submit_duration.py` 通过，停止后 23 个入口匹配。

误用普通单视图 `analyze_lifetime.py` 会因双视图数据不满足单视图 owned-view 条件而失败；该结果保留在 `finalinputs-48772-generic-lifetime-inapplicable.json`，不能混为双视图专用分析成功，也不能用它宣布本轮对象泄漏。

本轮窗口截图实际发生在试验结束后，不能作为试验期间像素证据。后续必须把截图/回读请求与激活放在同一个脚本中，避免对话与工具调用间隔错过十秒窗口。

## 只读资源对象检查

新增 `inspect_bound_inputs.py` 校验 EXE 版本和相同 PID，从记录的最多 64 个对象各读取 256 字节，比较两次读取。仅枚举指针与模块归属，不注入或调用 COM。

四个绑定对象均可读且两次读取相同；`+18`、`+60` 等字段指向使用游戏模块 vtable 的候选。直接一层扫描没有确认 D3D12 系统模块 vtable，说明还需追踪引擎包装，不能直接将 `80d8fe00` 当 ID3D12Resource 指针调用。试验后对象也可能复用，读取稳定不证明它仍属于同一帧。

本地证据均以 `analysis/runtime/finalinputs-48772` 为前缀：`.status.txt`、`.csv`、`-restored.json`、`-lifecycle.json`、`-observation.json`、`-objects.json`。游戏已恢复普通单视图，观察 DLL 保留到进程退出。

## 下一步

继续定向解析绑定对象的引擎包装与实际 D3D12 资源关系，并确认 `27e580` 及后续 pass 对共享纹理的写入。先建立资源身份和状态，再选择同帧定点回读；不把“右眼绑定非空”“两眼 Draw 数接近”或“共享输入”当成双目成功或确定根因。

## 后续：资源接口已确认（PID 11580，20:11）

定向分析析构函数与包装成员后发现本版本含未按 8 字节对齐的指针。只读工具原先的 8 字节步幅会漏掉 `+c/+14/+24`，现改为 4 字节步幅，最多跟踪 128 个节点。根绑定对象 `+60` 指向引擎包装，包装 `+c` 指向系统 D3D12 对象。相关导出在 `texture-wrappers/` 与 `texture-wrapper-members/`，只读树在 `analysis/runtime/finalinputs-48772-packed-tree.json`。

新版 `resource_identity_observer.h` 在最终 quad 前、引擎仍持有绑定对象时校验两级引擎 vtable 及底层前三个方法所属系统模块，再调用 QueryInterface(ID3D12Resource)、GetDesc、Release。它区别于上述 PID 48772 的纯内存采样，不读取 GPU 像素或更改资源状态。独立 17×9 D3D12 纹理的接口与尺寸自测、无效地址拒绝、既有几何/quad 自测均通过。

新鲜 PID 11580 的首次左右眼采样均返回 S_OK，四个槽的 ID3D12Resource 对应一致：

| 槽 | 尺寸 | DXGI 格式数值 | mip / array / samples | flags |
| --- | --- | --- | --- | --- |
| 0 | 1920×1080 | 26 | 1 / 1 / 1 | 1 |
| 1 | 480×270 | 10 | 1 / 1 / 1 | 1 |
| 2 | 2048×1024 | 72 | 1 / 1 / 1 | 0 |
| 3 | 1024×32 | 24 | 1 / 1 / 1 | 1 |

接口查询确认了资源身份和描述，尚未确认实际 SRV 子资源、资源状态、着色器作用或像素。记录地址在 Release 后只是历史身份，不可作为后续 GPU 回读持有引用。

十秒试验 1,137 对视图，持续提交分析与 23 个入口恢复检查通过，pending/retained/skipped 均 0，恢复普通单视图。本次在启动采集的同一脚本里等待两秒后截图，捕获时间 20:11:21；图片明确显示左半场景、右半黑，修正了此前截图错过激活窗口的问题。数据前缀为 `analysis/runtime/resourceidentity-11580`；图片路径记录在 `analysis/runtime/resourceidentity-active-11580.json`。

下一项最小判别实验：在调用点、右视图、目标/采样矩形和尺寸全部匹配时，仅将右眼最终 quad 的 u 从 960 改为 0，保持右半目标不变，短时 A→B→A 并同步截图。若右侧出现左图，证明该路径能把可见内容写到右侧并保留到显示，故障依赖原右侧采样范围或相关输入；若仍黑，仍需区分输入/常量、写入状态和后续覆盖。此对照尚未实施，也不作为双目修复。

20:13 前后重新核对 Ghidra MCP：Java GUI 进程存在，但默认 8080 无监听，原生工具握手失败、独立协议探针返回连接拒绝。记录在 `analysis/first-person/ghidra-mcp-20261013-recheck.json`。GUI 打开不等于 MCP Server 已启动；独立 headless 项目仍可用于定向分析。

## A→B→A 采样对照已完成（PID 8760，20:33）

20:23 用户重新启用后 MCP 协议已连通。当前打开的仍是原始 EXE；两个绑定函数的 64 字节与已验证运行时快照不同，不能直接在原始项目中推断这条渲染路径。目标切换步骤见文档 13。

新增默认关闭的 `--quad-left-source-control`，复用原 quad Hook。仅接受十秒双视图、原输出 bounds、viewport diagnostics；排除 mono/duplicate 对照。运行时要求 caller=`d83b1c`、当前 view 为已跟踪 renderer 的第二项、数组 count=2 且容量合法、全部矩形及尺寸精确匹配。只有 B 阶段的右眼 `u` 从 960 改成 0，其余参数原样转发。首次合格右眼调用建立 QPC 起点：0–3 秒 A1，3–6 秒 B，6–10 秒 A2。没有新增 Hook 或 GPU 复制。

阶段事件在合格右 quad 返回后置位，表示 CPU 阶段。`run_quad_source_control.py` 激活采集后等待各阶段，延迟 0.6 秒截图，并检查前后阶段和 PID；事后又核对截图起止 QPC 被对应右眼调用区间覆盖。三张截图均通过，不是试验结束后的单视图截图。

| 阶段 | 右眼调用 | 实际改 u 次数 | 右眼拒绝 | 可见结果 |
| --- | ---: | ---: | ---: | --- |
| A1，u=960 | 329 | 0 | 0 | 左半场景，右半黑 |
| B，u=0 | 345 | 345 | 0 | 右半出现与左半相同的可辨识场景 |
| A2，u=960 | 450 | 0 | 0 | 右半重新变黑 |

左眼、未知归属及所有非 B 阶段修改数均为 0。实际 quad 聚合匹配：左眼原参数 1,124 次；右眼原参数 779 次；右眼 u=0 共 345 次。其他 quad 因作用域或 caller 不匹配被拒绝，是预期行为。排除边界和顶部 FPS 字样后的右半截图灰度均值分别为 0.145、31.962、0.145；这是窗口 JPEG 的测量，不是 HDR 中间资源回读。

**结论：当前路径能够将可见场景写入右侧并保留到显示；原故障依赖采样坐标或关联着色器输入。** 固定右侧禁写和无条件后续清黑的解释被削弱。仍未证明主颜色资源右半为空，也没有证明第二眼几何颜色已渲染正确；B 是同源图像对照，不是双目修复。

构建 `build-quadcontrol/GearsCaptureQuadControl.dll` 成功。`--quad` 包含阶段边界、各项 gate、十二参数保持和真实 ABI 转发自测；几何与资源接口自测也通过。独立审查未发现阻断项。进入关卡由截图确认；菜单自动助手只确认前两项，后续顺序不匹配并到期退出，未被当成成功导航依据。

十秒共有 1,124 对视图，跨度 9.9969041 秒，提交/消费/准备/销毁一致，pending/retained/skipped 均 0；恢复单视图及 23 个入口恢复核验通过。观察 DLL 保留到游戏退出。

证据：`analysis/runtime/quad-source-control-8760` 前缀的 `.status.txt`、`.csv`、`-shots.json`、`-control-validation.json`、`-restored.json`、`-lifecycle.json`。截图为 `analysis/game-io/frames/quad-source-control-8760-A1.jpg`、`-B.jpg`、`-A2.jpg`。

下一轮应向最终输入的生产端追踪：依据 `1665dc → 1b4ca0` 已确认的资源选择，检查该资源及其右侧范围在哪个世界颜色/后处理 pass 写入、是否清除或按错误区域处理；用运行时快照反编译与定点像素证据核对。优先找出首次丢失右侧内容的位置，再修复同帧多视图；不要把本次 u=0 对照留作正式双目实现。路线仍不采用 AER。

## 反向采样对照与颜色资源关系（PID 35904，23:44）

用户打开运行时程序并完成自动分析后，MCP 已可直接反编译验证快照。绑定函数的字节比对通过，后续分析不再使用不匹配的磁盘 EXE 代码。

新增与原试验互斥的 `--quad-right-source-control`：B 阶段只把左眼最终 quad 的 u 从 0 改成 960，右眼参数不变。仍由第一个合格右眼 quad 启动同一十秒 A1/B/A2 时钟，原十二参数门限、调用点、视图数组和生命周期检查保留。三个窗口截图的 QPC 起止均在对应阶段左右眼调用区间内。

| 阶段 | 左眼调用 | 修改数 | 左侧可见结果 |
| --- | ---: | ---: | --- |
| A1，u=0 | 489 | 0 | 场景 |
| B，u=960 | 475 | 475 | 黑色；两侧都黑 |
| A2，u=0 | 652 | 0 | 场景恢复 |

时钟建立之前另有一个未修改左眼调用。右眼、未知归属及所有非 B 左眼调用修改数均为 0，合格左右眼的拒绝数均为 0。实际转发 quad 聚合为左眼原参数 1,142 次、左眼 u=960 共 475 次、右眼原参数 1,617 次。去除边缘和 FPS 后，左半 JPEG 灰度均值为 31.428、0.109、27.989。A1/A2 的镜头方向有变化，因此不作逐像素相等结论；可见场景在 B 消失、恢复 u 后重新出现。

**两次互补对照说明：最终显示结果跟随采样半区，不能再把故障简单归因于右侧输出不能显示。** 仍需区分原始颜色纹理、复制/解析范围及与采样坐标相关的着色器处理；没有获得原始 HDR 像素，也没有完成真正双眼立体。

同次首次左右眼输入样本确认：

- 最终 slot 0：QI 成功，`ID3D12Resource=11682f380`，1920×1080、格式 26、单采样。
- `global 1c3b420` 颜色目标：QI 成功，`ID3D12Resource=16111bb320`，相同尺寸/格式，但与最终 slot 0 **不是同一资源**。
- 颜色目标默认解析目标 wrapper `907a5380` 与最终 selected wrapper 相等。此项只证明包装关系，实际调用可以通过参数覆盖目标，仍需采集复制参数。
- slot 0 引擎包装 `+80=FFFFFFFF`、`+84=169`。`1c60f0` 在 `+84` bit 0 为 1 时走另一状态路径，不能将 `FFFFFFFF` 当作实际 D3D12 状态并直接插入 barrier。引擎缓存状态也不等于 GPU 已执行的状态。

`build-quadcomplement/GearsCaptureQuadComplement.dll` 构建及 quad/geometry/input-resource 自测通过。十秒 1,617 对提交、消费、准备、销毁一致，跨度 9.9991836 秒，pending/retained/skipped 均 0；23 个 Hook 入口恢复、普通单视图恢复。游戏窗口确认已进入关卡后才激活，本轮没有发送菜单输入。

证据前缀：`analysis/runtime/quad-right-source-control-35904` 的 `.status.txt`、`.csv`、`-shots.json`、`-control-validation.json`、`-restored.json`、`-lifecycle.json`；截图在 `analysis/game-io/frames/quad-right-source-control-35904-{A1,B,A2}.jpg`。

## 生产端调用链：新证据与尚未验证的分支

`11908c` 是阶段调度；它的 0–3 阶段不是眼睛编号。早先大量 Draw 的 `27e580` 路径绑定空颜色目标与深度候选，颜色写掩码置 0 后恢复，更像深度/预处理。**这些 Draw 不足以证明右眼世界颜色已生成。**

实际颜色边界是 `11a800`：绑定 `1c3b420` 及深度候选 `1c3b510`。`11aad0` 在第二参数非零时调用 `1c5dd8`，源是 `1c3b420`，默认目的 wrapper 取源 public 指针减 `48` 处。`1c5dd8` 最终可走 CopyResource、CopyTextureRegion 或 ResolveSubresource。其有效矩形分支使用传入 LTRB；无效/负矩形走全资源或全子资源复制。`11908c` 初始共享矩形是 `{face=0,left=-1,top=-1,right=-1,bottom=-1,mip=0,override=0}`，并非默认仅左半，后续写入和实际复制范围仍待核实。

`224ca8` 创建名为 DefaultColor 的采样与渲染包装；传入已有采样纹理作为关联参数不等于底层 GPU 资源相同，本次动态身份已经区分二者。最终输出来自 `11908c → 2b0a98(stage 4) → d82f58 → 1af42c`（quad 返回点 `d83b1c`）。`223ab4` 在该输出之后，不能列为此前最终输入的生产函数。可选 `be65cc` 为 DLSS 分支，当前设置已关闭 DLSS，不能因静态存在就声称本次执行。

另有具体候选：`119bdd → 2a7e48` 只根据第一个 view 的宽度计算绘制范围。当前 960/1920 会传入 0；该函数确实发出 quad，但深度/颜色写入状态及实际影响未验证，暂不命名为“清除右眼的根因”，也未跳过它。

CPU 坐标审查没有发现左右矩形重复偏移：构造器按源矩形并集建立局部目标坐标，当前并集原点 0；最终 quad 分别计算目标 clip 位置与源 UV。右眼 `x=960,u=960` 属于不同坐标域，不能据此判断重复加偏移。

下一最小实验：在已跟踪双视图消费范围内，仅观察与上述颜色资源有关的实际 `CopyTextureRegion` 参数，确认是否覆盖右半。若复制已全幅，继续检查源颜色的写入及后处理；若复制只覆盖左半，再追踪是谁构造了该矩形。不要通过猜测资源状态直接增加 HDR 回读。

## 实际复制观察（PID 18388，2026-10-14 00:06）

用户手动退出旧进程，`build-colorcopy/GearsCaptureColorCopy.dll` 在新进程中预留入口，用户继续关卡后才启用十秒双视图。截图的完整 QPC 区间落在 CSV 中双视图构造区间，画面仍为左侧场景、右侧黑。未修改最终 quad 的采样。

首次两眼确认目标 `129b02640` 与最终输入 `129b01cb0` 均为 1920×1080、格式 26、单采样的不同资源。实际 `CopyTextureRegion` 的已保存 623 次调用来自返回点 `1c6093`，源/目的与这两个资源一致，均为 subresource 0 → 0、目的 `(0,0,0)`、源 box 为 null：这些调用复制整个源子资源，不受 viewport 限制。

本轮记录存在明确覆盖限制：6,140 次 scope 内观察中，1,535 次与指定资源无关，623 次存入聚合，3,982 次因 32 项容量溢出未存。原因是将大量轮换 command-list 地址也作为聚合 key。**只能说已保存调用全幅复制，不能据此排除未保存调用中的局部复制。** 下一版本按复制语义聚合，保留首个 list 地址并增加独立分类计数；不通过无限扩大日志处理。

资源/参数读取拒绝均为 0。1,535 对视图全部消费、准备、销毁，跨度 9.9994043 秒，pending/retained/skipped 均 0；24 个入口恢复、普通单视图恢复检查通过。证据：`analysis/runtime/color-copy-probe-18388` 的 `.status.txt`、`.csv`、`-shots.json`、`-copy-validation.json`、`-restored.json`、`-lifecycle.json`。窗口截图为 `analysis/game-io/frames/color-copy-probe-18388-copy.jpg`。

## 首视图宽度分支的最小因果实验

补充反编译/汇编核对：`11a800 → 1741d0` 先将 viewport 恢复为整个颜色资源尺寸。`119bdd` 调用 `2a7e48(float)`，返回点 `119be2`，float 参数通过 XMM0 传入；该函数构造四个 CPU 顶点，x 为 `{1,1,t,t}`，其中 `t = 2 * firstViewWidth / resourceWidth - 1`。当前双视图时 t=0，覆盖全资源右半区域。

`2a7e48` 保留 shader/纹理绑定后调用 `1af054`，提交四顶点 triangle strip。`11a800` 和 shader 绑定函数 `0ea9f0` 不禁用深度；前面的深度 pass 有启用深度写入的预设，但仍缺这一调用点的实际状态与 shader 深度输出，不能将其直接定性为清黑或深度遮挡。

对照设计：仅在已跟踪双视图、精确返回点/矩形/资源尺寸/viewport、原 t=0 全部匹配时，把 B 阶段 t 改成 1，A1/A2 保持 0。仍调用原函数，保留其他绑定、上传与提交；四个 CPU 顶点 x 均为 1 的退化几何是否影响右侧场景，以实际截图验证。该实验已执行，结果如下。

## 遮罩覆盖的因果关系已验证（PID 34724，00:19）

用户进入关卡后执行 `run_color_probe.py --mode mask`。本轮没有修改最终 quad 采样，没有将左眼图复制给右眼。仅在上述原生遮罩调用点执行 A→B→A。

| 阶段 | 原生调用数 | 修改 t 次数 | 拒绝数 | 窗口图像 |
| --- | ---: | ---: | ---: | --- |
| A1，t=0 | 999 | 0 | 0 | 左侧场景，右侧黑 |
| B，t=1 | 1,005 | 1,005 | 0 | 两侧均有场景 |
| A2，t=0 | 1,358 | 0 | 0 | 左侧场景，右侧恢复黑色 |

三张截图均在对应 CPU 阶段内，完整截图 QPC 区间由合格调用起止包围。三阶段首个入口状态一致：viewport 1920×1080、scissor 为 `(0,0,16384,16384)`；CPU 深度描述前三 DWORD 为 `1,1,4`（启用、写入、LESS_EQUAL）；请求及缓存颜色写掩码全部为 15。入口描述尚不等于最终 GPU PSO，更不能单独证明 shader 的深度输出。

**已确认：在当前双视图路径中，`119bdd → 2a7e48` 的几何覆盖对右侧黑屏具有因果作用。** 原代码按第一视图宽度计算 t=0，在整个目标的右半提交遮罩；令该几何退化后右侧恢复，恢复原覆盖后再次变黑。具体通过颜色写入、深度遮挡或后续依赖产生黑屏，尚未完全区分。修正应围绕整个视图集合的有效覆盖范围，不能全局跳过此函数或长期保留诊断硬编码。

复制观察的聚合已经修正。本轮 6,724 次 scope 内 CopyTextureRegion 中，1,681 次无关，5,043 次匹配 source420→destination410，全部为完整 subresource 0 复制；partial/source-only/destination-only/overflow 均为 0，参数和资源读取拒绝均为 0。该结论覆盖本次同线程 CopyTextureRegion 观察范围，不代表覆盖其他线程或所有 GPU 操作。

十秒 1,681 对原生视图全部消费、准备、销毁；pending/retained/skipped 均为 0。25 个入口恢复及持续双视图生命周期检查通过。两眼来自同一原生 family 的两个视图，仍使用合成 ±1 游戏单位位置与原投影；尚未完成 IPD、非对称投影、不同深度视差或头显验收。

证据：`analysis/runtime/color-mask-probe-34724` 的 `.status.txt`、`.csv`、`-shots.json`、`-restored.json`、`-lifecycle.json`。图像：`analysis/game-io/frames/color-mask-probe-34724-{A1,B,A2}.jpg`。这三张图应一起审阅，不能只凭 B 宣布双目 VR 完成。

### B 阶段立体图像的离线核对

`analyze_stereo_pair.py` 使用亮度零均值 NCC、纹理/峰值歧义过滤与左右互逆匹配；搜索允许正负水平位移及 ±3 像素垂直位移。图像分半后的每眼为 480×540，以下数值是保存截图中的像素，不是原始 GPU 纹理像素或物理深度。

| 人工选取区域 | 合格匹配数 | 水平视差中位数 |
| --- | ---: | ---: |
| 前景角色 | 90 | 3.98 px |
| 近处柱子 | 42 | 1.87 px |
| 中景板面 | 57 | 1.03 px |
| 远处门洞 | 11 | 0.057 px |

全图共 835 个合格匹配；提高阈值、缩小 patch 后保留 341 点，各区域中位数仍约 3.98/1.85/1.04/0.057。各区域垂直位移中位数及绝对值 95 分位均为 0。**该场景有随景物深度变化的水平视差，支持不同视点的立体渲染，排除简单复制或整张恒定平移的解释。** 人工标记的近远关系不是深度测量；匹配 patch 有重叠，JPEG/缩放/遮挡/时序均可能影响数值，小数位不代表相应精度。单张窗口图不能证明头显投影、IPD、GPU 眼间时间同步或所有关卡正确。

统计在 `analysis/runtime/color-mask-probe-34724-stereo.json`、`-stereo-strict.json` 及对应 CSV；已做已知平移/零位移匹配检查，亮度与检查结果在 `-stereo-checks.json`。右侧灰度均值 A1/B/A2 约 0.225/31.375/0.223，与肉眼结果一致。

下一步：默认关闭、严格限定当前双视图调用点的 view-union 范围修正，进行 30 秒持续 SBS；复核视差、对象生命周期和自动恢复，再校准双眼投影与 OpenXR 接入。不采用 AER。

## 持续修正的首次运行及未通过项（PID 12420，00:33）

新增 `--view-union-mask-fix`，复用同一个原生遮罩入口。要求当前 tracked family 有两个源/目标一致、相邻且全高的整数矩形，左边界为 0，union 不越界，viewport 为全目标；原 t 必须与首视图宽度的浮点推导逐位一致。以整个 union 的右边界重新推导 t，保留原函数一次调用。默认关闭，排除 mono、duplicate、最终采样对照和旧 mask A/B/A。不同目标尺寸、不等宽双视图及部分宽度 union 有离线覆盖，不代表均已经游戏内验证。

`build-viewunion/GearsCaptureViewUnion.dll` 七项自测通过。真实运行 30 秒产生 4,620 对视图，全部消费、准备及销毁，pending/retained/skipped 为 0，恢复单视图及 25 入口恢复通过。13,860 次相关 CopyTextureRegion 均为完整子资源复制，聚合无溢出。

本轮不能作为持续图像验收，有两项明确问题：

- 截图脚本请求 1920 宽，而现有 WGC helper 只接受 160–1280。首张截图失败，无有效阶段图；原始失败报告保留。脚本已改 1280，并在激活前增加同 PID 的截图预检，避免再因截图参数消耗试验。
- 遮罩调用 9,240 次，其中 9,239 次接受并修改，最后 1 次被独立 30 秒时钟判为 expired。即已提交双视图在消费中途跨过了另一个截止时钟。不能把它作为无遗漏修正成功；下一版沿用 30 秒生产截止，让已进入队列的 tracked consumer 完整执行，由总采集停止流程收尾。

记录在 `analysis/runtime/color-union-probe-12420` 的 status、CSV、shots、lifecycle、restored 文件。`timing_valid=false`、`correction_valid=false`、`cleanup_valid=true` 是各自检查的实际结果，不能合并成成功。

## 30 秒持续修正通过（PID 31668，00:41）

`build-viewunion2/GearsCaptureViewUnion2.dll` 将有效期改为 tracked consumer 的完整生命周期：30 秒关闭双视图生产，已入队 family 继续完成，45 秒总采集停止流程保留。真实 Lifetime 模型的跨截止、已排队消费、销毁后拒绝测试及独立审查通过。没有放宽几何门限，也没有增加 Hook。

本轮 4,850 对视图全部消费、准备、销毁；9,700 次遮罩全部接受并修改，拒绝及几何聚合溢出均为 0。构造跨度 29.99794 秒；所有生命周期、25 入口恢复及回单视图检查通过。14,550 次相关 CopyTextureRegion 全部完整子资源复制，无溢出。

截图改为 1280×720，并在激活前预检。前、中、后段均看到两个场景视图，恢复截图为普通单视图；四张完整截图 QPC 均处于对应有效区间，`correction_valid/timing_valid/cleanup_valid` 全为 true。这是稀疏截图加连续 CPU 计数的验收，不是逐 GPU 帧回读。

Sol Medium 独立复核并运行已有离线匹配器，每眼保存尺寸 640×720。前/中/后角色视差中位数约 5.18 px、柱面 2.41 px、板面 1.38–1.39 px、远门洞 0.12 px；合格匹配数 1,228/1,224/1,225，趋势稳定。相同图像在三个时点支持持续存在不同深度的水平视差；未据此推断物理 IPD 或头显校准。

证据前缀 `analysis/runtime/color-union-probe-31668`，包括 shots/status/CSV/restored/lifecycle 及 `{early,middle,late}-stereo` JSON/CSV。图片为 `analysis/game-io/frames/color-union-probe-31668-{early,middle,late,restored}.jpg`。

下一项为原生构造参数 `+b0` 的投影响应验证，再接实际 OpenXR 两眼 pose/FOV。当前投影对应原 16:9 视野，置于每眼 960×1080 后为横向压缩的 half-SBS；不能直接把这些像素作为已校准的头显眼图。桥包仍只传 head，GPU 输出仍是二维 quad，需加入与图像一致的每眼元数据和 projection layer。

## 原生投影响应验证通过（PID 20804，00:56）

独立构建 `build-projection/GearsCaptureProjection.dll`，默认关闭的 `--projection-control` 仅与三十秒同 family 双视图、output bounds、viewport diagnostics 和 `--view-union-mask-fix` 联用，复用原有 Hook。每次尝试创建一对源输入前只选择一次阶段，首次成功创建并登记 tracked renderer 的 pair QPC 建立起点；两个眼睛共享该阶段。仅修改源快照的投影 `+b0`，原 source 不变，±1 游戏单位眼位、其余矩阵字段和深度参数保持相同；未知形状、非有限值及无有效数值变化均拒绝。

| 阶段 | 时间 | 两眼投影输入 | tracked pair 数 |
| --- | --- | --- | ---: |
| A1 | 0–5 秒 | 原投影 | 830 |
| B | 5–10 秒 | P00 × 2 | 890 |
| A2 | 10–15 秒 | 原投影 | 836 |
| C | 15–20 秒 | P8 + 0.1 | 833 |
| A3 | 20–30 秒 | 原投影 | 1,684 |

五阶段 × built / consume_before / prepare_before / prepare_after 共二十项聚合均有记录、mismatch=0，projection rejected=0。预期与有效投影保存在有界 tracked root 元数据中，销毁时清除；消费/准备记录证明原生 renderer 在这些位置持有对应输入，不单独证明 GPU 执行。prepare_after 检查其余字段与深度保持，并记录允许变化的 P8/P9 残差。**本次所有阶段、两眼的 prepare P8/P9 residual 都是 0，因此未验证活跃 TAA 抖动时的行为。** B/C 残差相对 A1/A2/A3 实测基线的检查通过，未假设固定引擎抖动上限。

同步保存的 1280×720 窗口截图每眼为 640×720，五张完整截图 QPC 均被对应 tracked consume_before 区间包围。`20804-response.json` 的有界水平仿射匹配显示：B 两眼横向 scale 约 2（拟合平移左 0、右 1 px），C 两眼均同方向 +32 px、scale=1、垂直位移=0。A1→A2、A2→A3 及 A1→A3 两眼均恢复 scale=1、shift=0；支持 `+b0` 的缩放和非对称项能传到可见图像。匹配来自窗口截图，受动画、JPEG、缩放和遮挡影响；不等于物理 IPD、头显校准或逐帧 GPU 同步证明。

三十秒共 5,073 对视图，全部消费、准备及销毁；10,146 次遮罩全部接受并修改，拒绝和几何溢出均为 0。pending/retained/skipped 均为 0，原 source 保持、独立状态释放、普通单视图恢复和 25 个入口恢复检查通过。CSV 有 214 条 ring dropped，双视图生命周期事件仍满足分析器的覆盖门限；精确总数由连续计数提供。`correction_valid/projection_valid/timing_valid/cleanup_valid` 均为 true。

证据：`analysis/runtime/color-projection-probe-20804-{shots,lifecycle,restored}.json`、同前缀 `.status.txt/.csv`，以及 `analysis/runtime/20804-response.json`。截图为 `analysis/game-io/frames/color-projection-probe-20804-{A1,B,A2,C,A3,restored}.jpg`。

当前 OpenXR registry 对应 SteamVR 2.17.10、system 名称 `oculus`，推荐两眼各 4096×3288；此次只确认 system 可用，没有提交游戏的 projection layer。下一步将真实每眼 pose/FOV 转换到原生视图与投影，并将每个图像 ticket 绑定对应的两眼元数据和预测显示时刻，再提交 OpenXR projection layer，验证尺度、动态头动和持续运行。当前仍是合成眼位的桌面 half-SBS，不能宣布 VR 完成。

## 真实 XR 静止参数路径验证（PID 39436）

`build-stereoxr/GearsCaptureStereoXR.dll` 的三十秒桌面 pair 试验以真实 XR 样本替换合成眼位和游戏 FOV。每个原生 family 冻结一次样本及两眼矩阵，独立 ticket ID 与 sample sequence 分开；无效样本回退普通 mono。当前 scale 为 100 游戏单位/米，尚未完成物理尺度校准，没有发布 GPU 图像或提交游戏 projection layer。

共 4,465 对视图全部消费、准备及销毁，8,930 次遮罩全部接受并修改、无拒绝或几何溢出。built、consume_before、prepare_before、prepare_after 四处均记录 4,465 次，mismatch=0、view_error_max=0，两眼 P8/P9 residual=0。pending/retained/skipped 均为 0；原 source 保持、独立状态释放、普通单视图恢复及 25 个入口恢复检查通过。shots 的 `stereo_valid/correction_valid/timing_valid/cleanup_valid` 均为 true；原始 CSV 有 134 条 dropped，计数和生命周期覆盖检查通过。

原生首末样本 sequence=1110/4706、ticket=1/4465。`39436-xr-matrix-audit.json` 与发布的 XR 记录逐项核对 session、sequence、received_ns、predicted_time、flags、head pose 和两眼 pose/FOV，首末均完全对应；边界样本年龄约 8.27/7.57 ms。实测 IPD 首末为 0.0665900205/0.0665899868 米，约 66.59 mm。左右 FOV 非对称，按 left/right/up/down（弧度）分别为 `-0.942477763/0.698131680/0.767944872/-0.959931076` 和 `-0.698131680/0.942477763/0.767944872/-0.959931076`。左眼 CSV 的源到 built 矩阵差为 0，P00/P5/P8/P9 与原 mono 不同，深度块保持；两眼一致性由上述原生聚合检查提供。

**本轮只验证静止时的真实 XR 参数路径，不是明显头动验收。** 用户确认第二轮没怎么动；试验期间相对 reference 的最大位移约 0.212 mm，最大旋转约 0.02848°，属于追踪噪声量级。完整记录期间最大位移仍不足 0.32 mm、旋转不足 0.03°。小输入伴随消费矩阵变化，但窗口截图不能分辨这种运动，更不能据此建立最终 GPU 图像响应。CSV 没有逐记录的 frozen XR sequence/base-matrix ticket，因此仅首末可与发布样本精确核对，中间 pose 不能逐条精确关联。prepare residual 全为 0，也未验证活跃 TAA 行为。

执行中两项失败均保留：第一次 preflight 因现有 CSV 被独占而失败，没有原生 activation，不能计作一次有效试验；第二次 runner 在 65 秒收尾等待时 timeout，之后通过 `finalize-only` 收齐最终 shots、lifecycle、restored/status 证据并核验恢复，不能把 runner 原始超时记录改称无错误完成。

证据：`analysis/runtime/color-stereo-probe-39436-{shots,lifecycle,restored}.json`、同前缀 `.status.txt/.csv`，以及 `analysis/runtime/39436-xr-matrix-audit.json`。本轮支持真实两眼参数进入原生 renderer 的 CPU 路径，不证明游戏眼图已映射到头显。

Frame link 模块另有 316 项离线检查通过，但尚未接入 Hook 或 GPU publish，不能称为实机图像映射成功。下一步用有声音提示的实际动作试验核对旋转、平移与恢复，再把 ticket、两眼 pose/FOV 和预测显示时刻绑定到实际 GPU 图像，提交 OpenXR projection layer；该后续动作试验的结果尚未写入本节。

## 真实动作的桌面头动阶段通过（PID 39232）

新进程沿用 `build-stereoxr/GearsCaptureStereoXR.dll`，通过声音提示完成实际头部旋转和平移，验证真实 XR→同帧两眼原生场景→桌面图像的头动路径。`stereo-xr-39232-attempt1.json` 记录 preflight 60 帧通过、native_started=true，native 与 XR 进程 exit_code 均为 0；本轮运行、收尾和恢复成功，前述 PID 39436 的静止结论和两次脚本失败仍保留。

三十秒共 4,226 对视图全部消费、准备及销毁；8,452 次遮罩全部接受并修改，无拒绝或几何溢出。built / consume_before / prepare_before / prepare_after 四处均记录 4,226 次，mismatch=0、view_error_max=0，两眼 P8/P9 residual=0。原 source 保持，独立状态释放，pending/retained/skipped 均为 0；普通 mono 恢复和 25 个入口恢复检查通过。`stereo_valid/correction_valid/timing_valid/cleanup_valid` 均为 true。CSV dropped=167；保存的 4,221 条 constructor 源到 built 矩阵逐项精确一致，完整计数来自原生聚合。

首末 native sequence=1118/4717、ticket=1/4226，与发布的 XR 样本按 session、pose、两眼 FOV、received_ns、predicted_time 和有效性 flags 全部匹配，边界样本年龄约 5.55/4.97 ms。试验期间相对 reference 的头部最大位移约 9.34 cm、最大旋转约 53.85°，明显超过上一轮追踪噪声。由左眼消费 V 矩阵恢复的 camera position 最大位移约 9.47 游戏单位；当前 scale=100 单位/米，这个数值关联尚不等于独立物理尺度标定。

前、中、后截图显示两眼场景明显响应实际动作，恢复截图回到普通 mono；完整截图 QPC 均在对应观察区间内。**真实 XR→同帧双场景的桌面头动阶段通过；游戏眼图的头显 OpenXR projection layer 尚未完成。** CPU 矩阵匹配和稀疏窗口图像不能证明每个 GPU 帧同步，也未建立精确 GPU frame ticket 映射。中间 native CSV 仍缺逐记录 frozen XR sequence/base-matrix ticket，无法逐条精确关联所有 pose；prepare residual=0，活跃 TAA 仍待验证。

证据：`analysis/runtime/color-stereo-probe-39232-{shots,lifecycle,restored}.json`、同前缀 `.status.txt/.csv`、`analysis/runtime/39232-xr-matrix-audit.json`、`analysis/runtime/stereo-xr-39232-attempt1.json` 及对应 XR CSV。图像为 `analysis/game-io/frames/color-stereo-probe-39232-{early,middle,late,restored}.jpg`。

下一步把已离线验证的 Frame link 接入实际图像 Hook/GPU publish，将每个 ticket 与对应两眼 pose/FOV、预测显示时刻和 GPU 图像绑定，再提交 OpenXR projection layer，验证物理尺度、头显显示、动态同步和持续运行。316 项离线检查不替代实机图像映射验收。

## 首轮 GPU 帧关联诊断（PID 19076）

独立构建 `build-framelink/GearsCaptureFrameLink.dll`（SHA256 `02BF4EF045892026B113B1352BC043D3F026DC1F64BBF29F3E4DA40D94F17869`）接入现有 Reset、RTV/OM、Draw、Execute、Present 观察路径，并在启动时记录实际 factory 输入的 swapchain/queue。354 项 CPU 检查和 frame-link / prime / prime-handoff / viewport / quad / view-union-mask ABI 检查通过。测试仍只观察关联，不发布游戏 GPU 图像。

PID 7680 第一次启动未进入双目试验：游戏 prime watchdog 以禁止其他写入的方式持有 `target-prime-status.txt`，额外 ABI 自检再次写该文件失败，返回 142。不是游戏渲染失败。脚本现把相同字节的 DLL 和自检程序复制到独立临时目录运行；原始错误日志保留，prime 到期后六个入口独立核验恢复，再重新启动 PID 19076。

PID 19076 三十秒合成 SBS 共 2,522 对视图，全部消费、准备及销毁；5,044 次遮罩修正全部接受。早、中、晚均为两眼场景，恢复截图回 mono；timing / correction / cleanup 和生命周期检查通过，28 个入口全部恢复。CSV dropped=46，不影响内存内连续聚合的计数。

**实机关联未通过。** Factory inventory 接受一个游戏 chain、记录两个 buffer；第一次 Present 的 `unchanged` 检查失败，binding 被永久标记无效。此后 4,939 次 Present 为 unbound，linked=0，GPU published=0。114,341 次 Execute 均被关联器拒绝；首次窗口含 unknown-list，此后没有有效 Present 边界，不能据此断言游戏所有提交都错误。当前日志未保存首个差异的尺寸/逐 buffer 详情，无法仅凭汇总区分启动后的 Resize 与其他 inventory 问题。下一轮需要在已加载关卡的单目基线建立固定尺寸快照，并保留首次差异细节；之后仍拒绝变更。

当前观察还没有覆盖所有 Clear / Copy / Resolve / Indirect / Bundle / UAV / RenderPass 写入及生命周期，故即使出现 linked，也只表示条件性的 CPU 调用关联。接入发布前须保留完整 frozen sample、确认实际输出和队列、处理自己的 GPU 复制及未知写入，并以 GPU fence 把图像与元数据一起交给接收端。

证据：`analysis/runtime/color-union-probe-19076.{status.txt,csv}`、同前缀 `-shots.json / -lifecycle.json / -restored.json`；失败的自检为 `color-union-probe-7680-start.log`，恢复证据为 `frame-link-7680-prime-restored.json`。

## 关卡缓冲区边界已修正，仍需定位额外提交（PID 21604）

`build-framelink2/GearsCaptureFrameLink2.dll`，SHA256 `58A2C945DAB02728B7C706BE65DA5BA77C32DEEC467DE8BE423A697B53DF40B2`，7313 项 CPU 检查及启动 ABI 检查通过。首次关卡内 Present 建立固定快照；持有 chain/queue 身份引用，backbuffer 仅临时访问；Present 回收旧 list/OM 槽并要求新的 Reset，容量丢失拒绝关联。

PID 21604 完成 1,728 对合成 SBS，全部消费、准备和销毁；窗口截图 timing 与 cleanup 通过，28 个入口恢复。首次快照确认尺寸始终为 1920×1080，但两个 backbuffer 身份已不同于创建 swapchain 时的记录。anchor 后 changed=0、unbound=0、inventory rejected=0；list 高水位 148/512，OM/list/event/target overflow 均为 0。这解决了上一轮最先遇到的 inventory 和观察表回收问题。

两个实际 backbuffer 各观察到左眼 864 次、右眼 864 次，ticket/queue/target 冲突及重复 eye 均为 0。但 linked 仍为 0：所有 3,657 次 Present 都有 unknown-list，所有 1,728 个 pair 都有最终绘制后的未分类写入（polluted）。只观察到一个 DIRECT queue，共 82,709 次 Execute、354,776 个成员，其中 3,728 个成员没有本窗口 Reset 记录。5,584 次未知 backbuffer Draw 都在没有 pair ticket 的作用域中。不能把这些数量直接解释为 GPU 错误；下一轮用少量调用栈及 Present 嵌套深度区分 UI 绘制、系统内部提交与窗口边界遗漏，不能直接忽略。

审查同时发现观察器 Draw 时重新解析 RTV handle 的语义问题：RTV 描述符在 OMSetRenderTargets 返回后可立即复用，目标必须在绑定时冻结。[Microsoft 官方说明](https://learn.microsoft.com/en-us/windows/win32/direct3d12/non-shader-visible-descriptor-heaps)。还需排除零数量 Draw，并将 descriptor eviction 纳入明确的丢失处理。当前仍没有 GPU 游戏图像发布，不以这些条件性的 CPU 事件宣称最终像素或头显双目通过。

证据：`analysis/runtime/color-union-probe-21604.{status.txt,csv}` 及同前缀 `-shots.json / -lifecycle.json / -restored.json`。

## 已定位 Present 内部提交及额外绘制（PID 35864）

`build-framelink3/GearsCaptureFrameLink3.dll`，SHA256 `FA0AAC8C16E6D37AC98745B80A3CD6DC4B14866AB21E8A31E607C4565A1C9A9A`。本版在 OM 绑定时冻结 RTV 目标，描述符淘汰使关联失效，忽略零工作量 Draw，并添加有界调用栈采样。7335 项 CPU 检查、完整构建及 ABI 检查通过。实际运行完成 1,686 对视图，3,372 次遮罩全部接受修改；生命周期、时间、恢复检查通过，28 个入口恢复。`linked=0`，没有发布游戏 GPU 图像。

已观察到每个原始 Present 内都有一次未知 Execute，共 3,457 次，调用者属于 `dxgi.dll+0x20da2`，`inside_original_present=1`。观察器在调用原始 Present **之前**封口，却把随后的 DXGI 内部提交计入下一个窗口；这是明确的观察边界问题。下一版应明确隔开该区间，保留对区间外未知游戏提交的拒绝，不能把所有未知 list 放行。另有区间外未知 Execute，尚未完全归因。

另一组真实 backbuffer Draw 在 consume 之外，调用栈含游戏 RVA `0x172741 → 0x19e753 → 0x19f2f3`。样本记录为 120 个 index、全屏 viewport；其用途可能是文字/UI，但尚未反编译确认，不能据此允许最终眼图后的任意写入。64 个聚合槽和总计 16 条栈已达到采样上限；未记录到的类别不代表不存在。

证据：`analysis/runtime/color-union-probe-35864.status.txt` 的 `frame_link_work` 与 `frame_link_work_stack`；同前缀 CSV、shots、lifecycle、restored。当前阻塞点是图像与 frozen ticket 的可靠关联；本轮没有推翻已经验证的桌面同帧双场景结果。

## Meta XR Simulator 独立双眼测试通过（2026-10-14 03:05）

用户将系统 ActiveRuntime 切换为 `tools/third-party/metavr/state/tools/xrsim/meta_openxr_simulator.json`。未修改此设置。使用既有 `build-stereogpu/OpenXRPoseProbe.exe` 执行 10 秒独立 `--render` 测试，不连接游戏桥。程序 SHA256 为 `8595C47117D2809E1D4D0EF449912C65E061794EDDBC8C7B2003C87C5F3236DA`，正常退出。

取得 705 帧有效模拟姿态、704 帧 focused/should-render，提交 704 帧双眼测试图案。两眼各 512×512 GPU 回读：左红底、右蓝底，均有白十字，校验值分别为 `11226527874467850021` / `15146484442681159973`。模拟 IPD 约 60 mm，头部姿态全程相同，因此没有验证模拟运动或真实头显追踪。

运行时实际启用 `XR_KHR_D3D12_enable`，日志后端为 `RenderingD3D12OnVulkan`，原生 `RenderingD3D12` 未启用。枚举格式为 `[29,91,10,34,26,40,55,45,20]`；当前测试图的 SRGB 候选满足要求。该结果验证模拟器接受当前程序的 D3D12 双眼 projection 测试图，不等于游戏图像、跨进程 GPU 共享、物理尺度或真实头显体验通过。

证据：`analysis/openxr/pose-20261014-030533-288859` 的 JSON、stdout、stderr、CSV、两眼 PPM 及 `-validation.json`。下一步先修正上述 frame-link 边界并查明额外 Draw，再将带完整 frozen ticket 的游戏图像交给模拟器 projection 接收端。

## 帧关联通过，额外 FPS 绘制已隔离（PID 37304）

`build-framelink4/GearsCaptureFrameLink4.dll`，SHA256 `1491F1613B89E24AF99F566F7B454AA9A114D5D9AB7C48AF209E60C4B96A96E8`，7357 项模型检查及真实 Hook 路径自测通过。外层 original Present 返回后，在原有排序锁中退休该独立观察区间；nested 不重复清理，failed/TEST 同样处理，容量故障仍保持拒绝。跨线程握手自测确认其他线程的 Execute 只能在区间清理后进入下一窗口。

只读 Ghidra 定向导出确认：`[ClientSettings] FrameCtr` 写入 `0x141c205bc`，FPS 函数检查它并格式化 `%2.2f FPS`。文字先画偏移阴影、再画原文字，逐字生成顶点和索引，经 `140117430 → 14019e64c → 1401720a0` 提交。最后两者为共用批次函数，不是 FPS 专用入口，未禁用它们。证据在 `analysis/ghidra/runtime-ui-{stack,fps,text,glyph,batch,producer}` 及 `runtime-ui-settings-near.txt`。

关闭游戏并确认上一轮入口恢复后，备份实际用户配置为 `analysis/runtime/frame-counter-20261014-031349.ini.bak`，仅将 `FrameCtr=True` 改为 `False`。下一次启动 PID 37304 的窗口截图中 FPS 消失。30 秒试验完成 2,599 对视图，2,594 对成功关联（99.8076%），contaminated/polluted=0；此前的 `caller_rva=172741` 绘制样本消失。未知 Draw 共 2,609 次，均为普通 mono 时期无 pair ticket 的 final quad；总数 `7807 = 2599×2 + 2609`，与分类相符。

Present 共 5,208 次；metadata_lost=6（包括首个 anchor 窗口）、unknown_list=5，其余容量、queue/ticket/target 冲突、重复眼、坏矩形、过期 ticket 均为 0。5 个缺命令列表证据的窗口仍被拒绝。生命周期、截图时间、回单视图及 28 入口恢复全部通过。此轮仍只验证 CPU 命令关联，`gpu_published=0`，没有提交游戏 projection layer。

实际 final Draw 使用 **全屏 viewport**，半屏区域来自 quad 顶点目的坐标；不能误要求 viewport 等于半眼矩形。PID 35864 sample31/34 和 PID 37304 sample3/4 都为 DrawIndexed 6、instance 1、单目标 1920×1080、viewport `(0,0,1920,1080)`、scissor `(0,0,16384,16384)`；quad 目的区间分别为左/右 960×1080。后续 publisher 应验证此已观察到的组合，并要求 scissor 覆盖相应眼图区域。

证据：`analysis/runtime/color-union-probe-37304` 的 status、CSV、shots、lifecycle、restored 及 `-frame-link-audit.json`。下一步接入完整 frozen sample、真实队列 GPU 复制与 fence，再运行模拟器游戏 projection 测试。

## 首次游戏 OpenXR projection 端到端通过（PID 15336，attempt2）

`build-stereopublish/GearsCaptureStereoPublish.dll`，SHA256 `E2A013A910DCBD418F09DA1E942AC9BED04777D354F36989851A56D1B76118E1`。默认关闭的 `stereo_gpu_publish` 仅与真实 paired XR、frame-link 和 30 秒 union 修正联用。consume 保存完整 frozen sample 到 TLS，已验证 final Draw 在排序锁下保留值；Present 按 linked ticket 查回样本并复制当前 backbuffer。只使用 factory 捕获且 anchor 验证的实际 queue/chain，GPU 资源由 worker 创建；自己的 GPU 命令以 TLS 隔离，避免再次计入观察器。单槽 ready/done fence 使图像与票据共同传递；截止后先通知 stopped，callback 排空后 drain/close。

真实 publisher→receiver ABI 集成测试、48 帧独立跨进程 GPU 测试及独立代码审查通过。本机 D3D12 debug layer 不可用，未以它提供验证。

第一次 attempt1 在 native 激活前被启动器拒绝：XR 实际产生了有效姿态，但预检把未写完的 CSV 尾行纳入最后 20 行，且只有 8 秒启动等待。修复为仅读取最后完整换行之前的数据，保留完整记录的严格 flags 检查，等待上限 20 秒；半行回归及既有 mock 共 10 项通过。保留 `stereo-xr-15336-attempt1.*` 失败证据，没有将测试图计为游戏 projection。

attempt2 正常完成：

- Native 生成并完整消费/准备/销毁 1,613 对视图；built、consume_before、prepare_before、prepare_after 的 mismatch、view_error 和 P8/P9 residual 均为 0。物理 scale 仍为临时 100 units/m，本轮模拟姿态静止、IPD 约 60 mm。
- 1,609 次 frame link 成功；容量、ticket/target/queue 冲突及污染均为 0。5 次未知 list 的窗口继续拒绝。
- 发布并接收 **1,111 张游戏 GPU 图像**，没有丢失/冲突 ticket 或被拒绝的实际 viewport/scissor；GPU 接收端通过格式转换将 R10 游戏源写入模拟器支持的格式。
- 提交 **2,158 帧双眼 projection**，覆盖 **29.9561931 秒**，最大提交间隔 **18.9437 ms**；session、ticket flags、geometry、stale、image rectangle、structural failure 均无错误。开始前 producer_unavailable/waiting_image 与结束后的 producer_stopped 分别记录，未计为游戏帧。
- native 与 XR 均 exit0，截图时间、生命周期、active=0、28 入口恢复、GPU drain/close、pending_tickets=0 均通过。游戏已恢复普通 mono。

接收端保存的 `stereo-xr-15336-attempt2.csv-gpu.ppm` 为 640×360，两眼均有游戏场景且不同；同名 PNG 是无损格式转换供查看。稀疏匹配得到 1,732 个合格点，角色/柱面/远门分别约 83.90 / 80.13 / 78.02 px，支持深度相关差异。大部分共同横移来自模拟器两眼非对称 FOV，不能把约 78 px 都当成深度视差。初次匹配器将搜索半径误用作取样边距，排除了指定的柱面和远门区域；已修正为使用现有边界裁切的搜索，保留旧报告并另存 `-gpu-disparity-fullgrid.json/.csv`。

本次通过的是当前关卡、静止模拟器姿态、约 30 秒的实际游戏投影链路。新游戏图像平均约 37 Hz，projection 使用复用图像达到约 72 Hz；两者不能混称场景渲染帧率。此版仍有旧 mono 通道的固定 16 ms 发布节流，下一版将仅对 stereo 去除该节流，保留消费 fence。自动模拟头动、长时间运行、真实头显显示/舒适度、物理尺度和其他关卡仍待验证，目标尚未完成。

证据：`analysis/runtime/stereo-xr-15336-attempt2.*`、`color-stereo-probe-15336-attempt2.{status.txt,csv}` 及 shots/lifecycle/restored；XR executable SHA256 `8595C47117D2809E1D4D0EF449912C65E061794EDDBC8C7B2003C87C5F3236DA`。

## 120 秒静态游戏 projection 通过（PID 33860，attempt1）

`build-stereopublish120/GearsCaptureStereoPublish120.dll`（SHA256 `284047BD10C65CA03AD6CC996516A8D3070B5BCBF62124BFD6606400DA0A1486`）与 `build-stereogpu120/OpenXRPoseProbe.exe` 完成新的有界长时测试。本轮不启用 Operator、不发送模拟头动；stereo 通道去除旧 mono 的固定 16 ms 发布节流，仍以单槽消费 ack 和 GPU fence 控制复用。默认严格 head/view flags 门限保持不变。

- Native 构造、消费、准备及销毁 **6,746 对视图**；四个 stereo stage 的 count 均为 6,746，mismatch、view_error 和两眼 P8/P9 residual 均为 0。构造跨度 **119.9999157 秒**，13,492 次 union 修正全部接受并修改，无拒绝和几何溢出。
- frame link 成功 **6,741 次**，contaminated、changed、unbound 均为 0；不满足关联的 Execute 继续拒绝。发布端产生 **6,732 张新游戏图像**，接收端 copies 同为 6,732；busy=7、capture_skipped=9、rate_limited=0，ticket_missing/rejected 和 rect_rejected 均为 0。
- 提交 **8,637 帧双眼 projection**，覆盖 **119.942687 秒**，最大间隔 **19.2561 ms**。required_span=119、producer_seconds=120，sustained_submission_passed=true、structural_failure=false。ticket flags、session、geometry、age、rectangle、reference-space、session-loss 和结构拒绝均为 0；启动前 unavailable/waiting、首帧 inactive 和截止后 stopped 未计入游戏提交。
- Native/XR 均 exit0；shots 的 stereo/correction/timing/cleanup/GPU-publish 检查均为 true，前、中、后和恢复截图完整 QPC 均被对应区间覆盖。lifecycle 全项通过，pending/retained/skipped=0，普通 mono 恢复；独立 restored 检查 **28 个入口全部匹配**，active=0、GPU drain/close=1、pending_tickets=0。

XR CSV 共 10,790 行，raw head_flags/view_flags 全为 15/15。接收端保存 `stereo-xr-33860-attempt1.csv-gpu.ppm`。CSV ring dropped=49，consume_before 明细少 1 行；连续原生聚合及生命周期覆盖检查通过，不能把有限 CSV 当成无丢行的逐 GPU 帧日志。旧 union status 的 `production_seconds=30` 是 `view_union_mask_fix.h` 中的固定打印字段，本轮实际期限以 lifetime/trial_seconds=120、完整构造跨度及 XR coverage 证据为准。

按新图像数除以实际 projection 覆盖跨度，本轮约 **56.1 张新图像/秒**，高于 PID 15336 的约 37 张/秒；8,637 次提交约为 72 次/秒，包含图像复用，不能称作约 72 fps 的新游戏场景渲染。不同进程和测试跨度也不是严格单变量性能基准。**本轮证明当前关卡、静止模拟器姿态下的 120 秒有界 projection 与恢复，尚不证明常驻运行、动态同步或物理头显显示。**

证据：`analysis/runtime/stereo-xr-33860-attempt1.{json,csv,stdout.txt,stderr.txt}`、接收端 GPU PPM；`color-stereo-probe-33860.{status.txt,csv}` 及 `-shots.json/-lifecycle.json/-restored.json`。独立端点、矩阵、coverage、图像与恢复审计为 `analysis/runtime/stereo-xr-33860-attempt1-audit.json`，passed=true。上述结论读取原始报告和 status 后核验，不覆盖 PID 15336 的历史结果。

## Operator 的 raw flags 与显式模拟来源准备

已安装官方 Meta XR Operator standalone API layer 和 stdio MCP proxy；只向自有 XR 子进程添加 `XR_API_LAYER_PATH` / `XR_ENABLE_API_LAYERS`，不改全局注册或环境。真实 backend 的 initialize、tools/list 和 get/set schema 已发现并保存。独立 probe 使用 LOCAL 空间，每段 3.5 秒，依次 yaw +15°、−15°、x +0.06 m、−0.06 m、回初始，约 17.6 秒完成。

初试 `operator-motion-validation-20261014-034037-479658` 显示 raw view_flags 始终 15，override 后 head_flags 为 3；数值回到初始 pose 后 tracked 两位仍 false。位置/方向有效不等于 actively tracked，不能把 3 改写为 15，也不能用两眼的 tracked 标志冒充物理 head tracking。后续 release smoke `operator-release-smoke-20261014-034608-231128` 调用已发布的 `openxr_release_input_devices`，原始返回 success=true，随后 get 的四个 flags 全 true、pose 回到初始；CSV 中间 1,206 帧 head3/view15，release 后 410 帧恢复 head15/view15。该 smoke 的 Probe 收尾超出 harness 有界等待、exit1，所以只确认动作与 release 证据，不称整轮无错误通过。

为此新增默认关闭的 `--simulator-operator` 来源模式，外层 runner 只允许它与 `--operator-motion --stereo-game` 联用。`RuntimeTracked=0` 与 `MetaOperatorSimulated=1` 按模式精确匹配；strict0 仍要求 head15/view15，sim1 保留 raw head 有效位 3、要求 view15，并标记 `simulation_not_physical_tracking`。bridge v2、GPU packet v3 和 frozen ticket 传递来源，双端校验 policy/version/size/PID；projection 使用图像对应 ticket 内的眼姿态/FOV，拒绝换源、未知 source 和 mode/source 不一致。Python 核对 accepted window 的 XR source 与 native ticketed CSV raw source/flags；旧严格 CSV 缺 source 列兼容为 0，新 sim 缺字段必须失败。

Helper 在 native paired-render 记录出现后启动有界动作；initial 与 release 后 restored get 都仍要求四个 validity/tracked flags 全 true，并校验数值恢复。工具失败也在 finally 尽力 release；工具、恢复、deadline 或清理错误均使 trial 失败。明确来源不等于放宽真实头显主线。

最终 `GearsCaptureStereoSim.dll` SHA256 `3F50E60DF4B14CC640A2E1843A891366F4670336CF9409B7194E98EA058A245F` 已完成 real/sim production FrameLink ABI（均 result=0）和 8 个 CLI 组合检查，执行归属记录在 `analysis/runtime/stereosim-native-ready.status.txt`。共享头负责人实跑的 CPU gate/store 与六组双格式 GPU legacy/real/sim 结果保存在 `analysis/runtime/source-policy-selftests-final.json`，passed=true；每组发布 12 帧、consumer exit0、resources_released=true，D3D12 debug layer 不可用的限制保留。Python 17 项 mock/来源门限测试通过，独立代码审查已消除 sim profile 接受 real source 的不一致。运行时身份另以 `xrGetInstanceProperties` 实测为 `Meta XR Simulator`，证据 `analysis/openxr/stereogpusim-runtime-info.txt`；不能用 system 名称 `Meta Quest 3` 判断当前是否模拟器。**此准备阶段尚未运行动态游戏；后续 PID 40776 结果见下节，自测不能替代实际游戏。**

用户另报告 IDA headless 已可用，后续可作为定向原生调用调查工具；本轮未据此新增分析结论，也无需重复计算器示例。本节记录动态游戏启动前的准备证据；后续实际结果见下节。

## 显式模拟头动游戏 projection 通过（PID 40776，attempt1）

先保留失败的 PID 38112 attempt1：native 与 XR 虽正常退出，但实时读取 capture CSV 遇到 `PermissionError(13, 'Permission denied')`，paired-render 等待未完成，Operator 没有执行 set 动作，yaw/x 和原生矩阵跨度为零，整轮 passed=false。native 写端改用 `_wfsopen(csv, L"w", _SH_DENYWR)`，允许只读共享、拒绝第二写者，并完成跨进程读/拒写验证；不是 Operator 路径增加 Python 重试。使用新进程 PID 40776 再测通过，没有把旧静态输出当作动态通过。

PID 40776 采用显式 `--simulator-operator --operator-motion --stereo-game`，runtime 为 Meta XR Simulator；这是模拟来源验证。Operator 在新 native paired-render 证据出现之后，依次慢速 yaw +15°/−15°、x +0.06 m/−0.06 m、返回初始，实际动作跨度 20.032 秒。accepted window 里 yaw 跨度约 30°、head x 跨度 0.12 m；2,161 个 XR 样本 source 均为 1，其中 1,432 行 raw head3/view15，729 行 head15/view15。release 成功且末尾 get 恢复全部 tracked/valid 和初始数值，没有改写 raw flags。

- 四个 native stage 各 **1,601 对**，mismatch/view_error/residual 均为零；GPU 发布与接收新图均为 **1,591 张**。
- XR 提交 **2,158 帧 projection**，覆盖 **29.9563728 秒**，最大间隔 **17.7506 ms**，结构拒绝为零；native/XR 均 exit0。
- 独立审计按 XR 时序与值重建 3,202 条 consume/prepare 左眼矩阵，最大误差 **0.0038875 native units**，小于量化 CSV 容差 0.006；左眼 FOV 投影也匹配。native CSV 不逐行包含完整 sequence，因此此项是时序/数值对应证据，不宣称逐行唯一 sequence 关联。
- 截图时序、四阶段、frozen endpoint 两眼 pose/FOV、生命周期、GPU drain/close 和全部入口恢复均通过；接收端保存的两眼图像都有不同的游戏场景。`simulation_not_physical_tracking=true`，不当作真实头显游戏显示、物理尺度或舒适度验收。

证据：`analysis/runtime/stereo-xr-40776-attempt1.*`、`color-stereo-probe-40776` 的 status/CSV/shots/lifecycle/restored；独立审计 `analysis/runtime/stereo-xr-40776-attempt1-audit.json` 全项 passed=true。失败实验保留 `stereo-xr-38112-attempt1.*`。该动态测试仍是 30 秒有界测试，不代表常驻模式通过。

## 持续运行入口与明确停止契约

新增 `tools/native-capture/run_stereo_continuous.py` 的前台会话 owner。游戏须先以对应 DLL 保留方式启动、进入关卡并完成 target prime；此入口不自动启动游戏或操作菜单。以下命令已更新为带 owner 身份监测的 Continuous2；Continuous1 仅保留为历史实验二进制。针对已经准备好的 PID，一条命令启动同帧双目、GPU 发布与 OpenXR：

```powershell
python tools/native-capture/run_stereo_continuous.py start --pid <PID> --build-dir tools/native-capture/build-continuous2 --dll-name GearsCaptureContinuous2.dll --xr-build-dir tools/openxr-probe/build-continuous2
```

owner 打印唯一 `analysis/runtime/continuous-stereo-<PID>-<时间>-<ID>.json` 和准确停止命令，并在 native 确认 running 后打印 `continuous_running=true`。在另一终端执行：

```powershell
python tools/native-capture/run_stereo_continuous.py stop --session <上述JSON完整路径>
```

stop 端核验 owner/game/XR 的 PID、创建时间及映像身份，只发 `Local\GearsVR.Capture.Stop.<PID>`，不重新注入。owner 先等 native closed/pending0、state release/destroy、active0、publisher stop、drain/close、无 retained/timeout，并独立核验游戏入口恢复；之后等 XR 因 producer_stopped 退出，必要时只对自有 XR 发 `Local\GearsVR.XR.Stop.<PID>`。XR 依 STOPPING→xrEndSession 正常收尾；依据 [OpenXR session state 定义](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSessionState.html)。有界清理失败或强制终止均报告 incomplete/exit1，不杀其他模拟器或游戏进程。Ctrl+C 也进入同一收尾路径。

持续模式为显式 `continuous_session=1`，pair_seconds=0；原有 30/120 秒诊断入口及期限保持不变。默认严格 source0/head15/view15，不添加 Operator layer 或自动头动；simulator-operator 动态测试仍通过独立有界诊断入口执行。即使严格 flags 在模拟器为 15，报告也明确 `physical_tracking_verified=false`。native 连续模式关闭逐帧 CSV，保留聚合/停止状态；XR 前 240 帧完整落盘、随后约 1 Hz 采样，但每帧仍检查门限。不会通过不断扩展 CSV 的方式占用无界内存。

独立会话互斥、唯一报告、fresh native started_qpc 防止第二 owner 或旧 status 被误接受。JSON 原子替换仅针对短暂 Windows PermissionError 做至多 100 ms 重试；其他错误失败。Python 9 项连续模式 mock 与原诊断 17 项测试通过（26 项），覆盖身份复用、清理状态、先 native 后 XR、XR 早退/超时和旧诊断参数。native CPU/ABI 与 XR 独立停止/异常路径测试由对应实现负责人完成；尚未据此宣告持续游戏通过。

XR 最终程序 `tools/openxr-probe/build-continuous1/OpenXRPoseProbe.exe` SHA256 为 `D8E0DE33AD66420B96172842F462DB9BECDF4BA91494046E983B755B4A83D46F`。独立无 native 的 Meta XR stop smoke 显示 orderly_stop/shutdown_completed=true，但因没有游戏图像 exit1；异常 runtime 路径为 probe_failure/orderly=false/shutdown=false/exit1，非法 CLI 拒绝。证据 `analysis/openxr/continuous-stop-smoke-final-40776.json` 与 `continuous-failure-smoke-40776.json`。持续 owner exit0 只确认有游戏输出且结构和联合收尾正常；持续输出质量与真实头显验收仍需独立检查。首次实际持续输出结果如下。

## 持续输出跨 180 秒，联合停止通过；尾帧计数待查（PID 43724）

使用最终 `GearsCaptureContinuous1.dll` SHA256 `01BF00A0870C4D912D719B034822E30C6B93011E2C24F26CF9C90BF5564A2CF0`，只运行一个 strict source0 会话，不启用 Operator、不发模拟动作。owner 从本地 04:28:59 开始；有图后独立计时起点为 UTC `2026-10-13T20:29:32.937012Z`，180 秒后于 `20:32:32.939721Z` 请求 stop，`20:32:39.448225Z` 确认联合完成。快照均为相同 started_qpc、running/source0：

| 有图后计时 | published |
| --- | ---: |
| 0 秒 | 990 |
| 60 秒 | 4,221 |
| 125 秒 | 7,668 |
| 180 秒 | 10,571 |

这直接证明跨过旧 120 秒期限后仍产生新游戏图像。最终四阶段各 10,632 对，mismatch/view_error/两眼 residual 均为零；XR 14,401 帧 projection 覆盖 200.010565 秒、最大间隔 28.5519 ms，structural_failure=false。1 次 stale_ticket 被正常拒绝，不宣称没有暂停或拒绝。445 行采样 CSV 全为 raw head15/view15/source0，逐帧门限仍由程序执行，不能把稀疏日志当成逐帧完整证据。

native phase=stopped/reason=event，release/destroy、pending0/active0、publisher stop、drain/close、无 retained/timeout 和 entries_restored 均满足；独立 28 入口恢复全部匹配，stop CLI、owner 与 XR 均 exit0，XR 原因 producer_stopped/orderly_stop/shutdown_completed=true。所有自有进程已正常退出。早段和跨 120 秒后的截图分别为 `analysis/game-io/frames/20261014-042941-77986e62.jpg` 与 `20261014-043207-df27fd06.jpg`，均有正常双眼游戏场景。停止后的 `analysis/game-io/frames/20261014-043404-4b850a75.jpg` 已直接查看，显示恢复为全宽普通 mono 场景。

**独立严格等数审计仍为 passed=false：native 最终 published=10,615，而 XR scene_copies=10,614，相差 1 张；持续 coverage 与联合生命周期检查分别通过。** `tools/vr-common/gpu_share.h` 的单槽 producer 在 done<serial 时拒绝新发布；mark_stopped 先置 stopped，receiver 的 acquire 见 stopped 后拒绝未领取的 slot。producer drain 等待自身 GPU in-flight，receiver 的共享资源由自己的 copy/ack fence 管理。因此差 1 与关闭时最多一张未消费 slot 的代码语义一致，但缺逐 packet 尾部证据，只作根因推断。保留等数检查 false 和 1 次 stale 拒绝，不为追零改 stop/age 门限，不宣称全项无异常或真实物理追踪。真实头显、其他关卡和长期稳定性仍待验证。

证据前缀 `analysis/runtime/continuous-stereo-43724-20261014-042859-b876e567`：owner `.json`、`.snapshots.json`（UTC/原始 running 状态/stop）、`.session-status.txt`、`.native-status.txt`、`.restored.json`、XR stdout/stderr/采样 CSV，独立 `.audit.json` 记录所有检查及未通过的 published_equals_received，没有覆盖原始计数。

## 完整六轴模拟运动的 120 秒验证（PID 45440）

新增显式 `--operator-profile sixdof`，仅允许 `--operator-motion --simulator-operator --stereo-game --trial-seconds 120` 的有界试验。复用已冻结的 `build-continuous1/GearsCaptureContinuous1.dll`，以 bounded120/source1 工作；XR 使用 `build-stereogpusim/OpenXRPoseProbe.exe`，未增加渲染 Hook。原 30 秒 yaw/x profile 不变。

在 native paired-render 出现后，13 个动作依次执行固定 LOCAL 轴的 yaw ±15°、pitch/roll ±10°、XYZ ±0.06/0.04/0.05 m 和回初始；每个目标由初始姿态计算，避免累积偏移。实际动作历时 59.625 秒。release 后 raw head 恢复 15，位置误差 0、四元数绝对点积 1。accepted window 的 8,641 个 XR 样本全为 source1，其中 head3/view15 为 4,281 帧，head15/view15 为 4,360 帧，未合成 tracked 标志。

- 四个 native 阶段各 **6,418 对**，两眼 mismatch/view_error/投影 residual 均为 0；独立逐段审计确认十二个轴向目标都在实际 XR 样本中到达，误差 ≤1°/2 mm 且至少三帧，并关联到 native consume 冻结 ticket。
- consume/prepare 的 **12,834 行左眼矩阵**由相应 XR 位姿重新构造，0 行无法匹配；最大 CSV 量化误差 0.00391875 native units，FOV 投影误差约 2.17e-8。跨阶段冻结 ticket 和首末完整姿态/FOV/raw flags 核对通过。CSV 未逐行记录右眼矩阵或 sample sequence，右眼仍属于四阶段仪表比较证据；GPU 也没有逐 packet 完整 dump，不夸大为每张图片都独立重构验证。
- GPU 发布/接收均 **6,412 张**；XR 提交 **8,637 帧**，覆盖 **119.943487 秒**，最大间隔 **17.9084 ms**，stale、结构及追踪拒绝均为 0。新图约 **53.44 Hz**，projection 约 **72.01 Hz**，后者含复用。
- runner/native/XR exit0，GPU drain/close、生命周期和独立 28 入口恢复通过；两眼场景与恢复后的全宽 mono 已查看，随后游戏正常关闭。

动作审计按每次 set 发出至下一次 set/release 发出的实际 override 持有区间检查，不假定工具返回时刻就是 XR 插值完成时刻。初版以工具 finished/duration 截止，因实际目标晚约 50–100 ms 出现而失败；该初版输出在纠正方法时被覆盖，已由未变原始数据确定性复算并保存为 `stereo-xr-45440-attempt1.six-axis-initial-window-audit.json`，明确标注 reproduction。最终审计不借用下一动作发出后的样本，目标/矩阵容差没有放宽。

证据：`analysis/runtime/stereo-xr-45440-attempt1-audit.json`、同前缀 `.six-axis-independent-audit.json`、`.execution.json`、`.operator.json` 和原始 CSV；native 前缀 `color-stereo-probe-45440`。完整及六轴审计均通过。这只证明当前关卡的模拟器六轴软件链路，仍是第三人称基准视点，不代替真头显、物理尺度、稳定第一人称或其他关卡验收。

**后续证据更正（PID 38784 回中试验时发现）：** 本机 Python 3.12 的 `time.monotonic_ns()` 与 `perf_counter_ns()`/原生 QPC 并非相同时间基准，现场差约 160 ms。旧 Operator 动作记录用前者，而上述逐段审计直接与 XR QPC 时间相比，因此其“严格位于下一次 set/release 之前”的动作窗口结论需要撤回；历史原始报告不覆盖。原始姿态跨度、使用同一 QPC 域的 native↔XR 矩阵重建、四阶段比较及清理结果不受这个动作窗口问题影响。不能用当前测得的偏移回填过去的精确时刻；新实验将额外记录 QPC 对应的 perf_counter 时间后重新验证动作边界。

## Continuous2：启动器意外退出后的独立清理（PID 33264）

之前依赖 Python finally 收尾，强制结束 owner 会跳过它。Continuous2 在 native 与 XR 两端保留 owner 进程 handle，并核对 PID 与 FILETIME 创建时间；owner 消失或 Wait 失败只触发停止请求，继续原来的 native 回调释放、GPU drain、入口恢复和 OpenXR STOPPING 流程，不强制释放在用资源。

continuous 模式现必需 native INI `continuous_owner_pid` / `continuous_owner_creation_time`，XR 必需 `--owner-pid` / `--owner-creation-time`；Python owner 自动向两端传自己的同一身份，核对 native 状态。旧 bounded30/120 不接受也不需要这些字段。XR 连续模式仍拒绝模拟 Operator。PID 复用由 retained handle 和创建时间校验排除；native 独立记录 `owner_exit_observed` / `owner_error_observed`，保持首次 stop_reason，避免把同时发生的追踪失效改写为另一首因。

冻结二进制：

- `tools/native-capture/build-continuous2/GearsCaptureContinuous2.dll`：SHA256 `0976D6F255F3F1D42DE5EEA031C276CD2144907273B14AF42F46FB8E217B4551`。
- `tools/openxr-probe/build-continuous2/OpenXRPoseProbe.exe`：SHA256 `B57A16ACAD047824F85CAAE0AA98613855F1C6FE6A162C59A85EF972908FE492`。

5 项 native CPU ABI、Lifetime、FrameLink 7,357 检查，2 项 XR CPU 自测、非法/不匹配身份拒绝和 Python 35 项测试通过。PID 33264 实际进入关卡后，控制器等待 owner running，再观察 10.063 秒 published 从 0 增至 389；随后仅 `Popen.terminate()` 自创 owner 34528，**没有发送 native/XR stop 事件**。4.328 秒内两端完成清理：native owner_exit_observed=1、error=0、reason=owner_exited，pending/active/retained/timeout=0，release/destroy/drain/close/入口恢复成立；独立验证 28 入口全部匹配。

XR 提交 522 帧、接收 399 张新图，native 最后 published=400，故本次也保留停止边界差 1，不宣称全帧等数。XR 明确 owner_lost=true、stop_reason=owner_exited、structural_failure=false、shutdown_completed=true，**exit1/orderly_stop=false 是预期的 owner 故障结果**，不伪装成正常用户 stop。异常恢复测试 passed=true 只表示故障注入后的清理条件成立。

原 owner JSON 保留被杀时的 running 状态与原始字节；控制器另写 `analysis/runtime/owner-death-33264-8f5a5a4d5a.audit.json`，没有补写正常完成。游戏仍存活并恢复全宽 mono，截图 `analysis/game-io/frames/20261014-045348-4ccc1071.jpg` 已查看，随后核对游戏路径与创建时间，正常关闭游戏。本轮无 game/probe/proxy 残留，runtime 仍为 Meta XR Simulator，最终记录 `owner-death-33264-final-cleanup.json`。

该测试没有覆盖 runtime 阻塞在 xrWaitFrame、断连重建、同进程重新激活或重载关卡；这些情况不能因本次 4.328 秒成功就保证有界恢复。最终交付仍须补单一启动入口、回中、稳定第一人称以及真头显验收。

## 仓库统一启动与停止通过（PID 44640）

`tools/vr-launcher/launcher.py start/status/stop` 复用冻结的 Continuous2 native/XR，由同一个前台 Python 进程持有会话。start 不再要求手填 PID；校验三个二进制哈希，拒绝已有游戏，执行一次 reserve/prime，再验证 Controller↔WarPawn、Level 归属与连续推进的 CameraCache。入口等待玩家进入关卡；此次菜单由助手通过已有 game_io 导航，不是入口自动导航功能。120 秒玩家等待从预留调用之前计时，不保证所有 XR/激活耗时必然赶上 prime watchdog；超时保持失败现场、不重新注入。

PID 44640 中，launcher owner 与 continuous runner owner 均为 12652。四阶段各 2,343 对，mismatch、view_error 和两眼投影 residual 均为零。GPU 发布和接收均 2,334 张，XR 提交 3,166 帧、覆盖 43.9590107 秒、最大间隔 15.7937 ms。无 PID 的 stop 完成联合收尾，owner/XR/stop CLI 均 exit0，独立 28 入口全部恢复，SBS 和恢复后的普通 mono 已直接查看。核对游戏身份后正常关闭，无本轮游戏/XR/proxy 残留。完整证据与当时 launcher index 快照在 `analysis/runtime/vr-launcher-smoke-44640-audit.json`；后续覆盖工作索引不会覆盖这份历史快照。

配置提供有限数值尺度与本机游戏路径，范围 10..500 不等于物理校准通过。它仍依赖本仓库和固定底层启动路径，不是可复制部署包。取消启动只证明协作取消请求，未激活时的 prime watchdog 恢复另行核验；同一游戏进程停止后重新激活暂不支持。

## 第一人称锚点的只读证据（PID 44640）

新的 `tools/package-inspector/sample_player_anchor.py` 每次核对注册对象、Pawn/Controller 双向关系、Level、ViewTarget 和反射类型，再以 CameraCache 时间戳前后包围只读测量。时间戳相等仍不等于引擎内的原子渲染快照。首次启动期 490 次中只接受 6 次，484 次因注册表数量改变而拒绝，原结果保留；恢复 mono 后独立采样 482 次，接受 448 次，34 次跨帧拒绝，448 个不同时间戳覆盖约 9.973 秒。

静态样本 Pawn.Location=(9879,-35097,-708.8499756)，EyeHeight/BaseEyeHeight=60；POV.Location=(9719,-35049,-632.8499756)，三种 Rotation 均为零。POV 相对 Pawn 差 (-160,+48,+76)，候选 `Pawn.Location+(0,0,EyeHeight)` 为 (9879,-35097,-648.8499756)。此值只是候选眼点；不能把相对 POV 的 (+160,-48,-16) 写成固定偏移，也不能以零旋转样本证明 Controller、Pawn 和相机旋转在运动时相同。

下一项最小实验需要把已核验眼点与当前原生 renderer 对齐，并检查角色遮挡；稳定朝向仍需非零视角输入和动作时的动态证据。当前尚未修改第一人称渲染基准。证据 `player-anchor-44640-launcher.*`、`player-anchor-44640-mono.*` 和 `player-anchor-44640-fp-plan.json`。

## IDA 定向相机分析

使用已校验 PID 16812 运行时代码快照 `analysis/ghidra/render-16812-verified/section_1000.bin`，以 0x140001000 加载并确认 64 位，独立 headless IDA 分析了相机主体 RVA 0x110878、包装函数内 0x3b3bd9 的调用点及四个必要直接 callee。证据、字节核对和带地址汇编保存于 `analysis/ida/camera-110878-20261014/README.md`；自有数据库已保存并关闭。

汇编与既有反射一致：主体使用 Pawn 的 Location+0x130/Rotation+0x13c，经 CurrentCamMode（camera+0x558）调用和局部位置插值，写入 ViewTarget.POV 的位置、旋转、FOV。存在后续旋转重写，保留第三人称原始 basis 不能作为稳定第一人称验收。主体未见直接读取 EyeHeight+0x310/BaseEyeHeight+0x30c，但不能排除经模式/脚本取得眼点。vector dispatcher RVA 0x3d4fbc 的真实函数名仍未知，不能直接命名为 GetPawnViewLocation。原始 blob 不包含解析外部数据所需的完整上下文，Hex-Rays 的额外参数、AVX 内联和 jumpout 只保留为不可靠推断；本结论以字节和汇编为主，未声称这些分支在本轮实际执行。

## Continuous3 回中准备与失败记录

native SHA256 `F15F375E647B451858376183D7B9C4198CB931DB801DFE7841264BC06FB237B0` 新增 F9 边沿及命名事件回中，仅在下一对合格视图冻结之前处理。参考姿态取当前 LOCAL 位置与 Y-up 水平 heading，保留真实俯仰和侧倾；近乎竖直、不能稳定确定水平朝向时延后。原有 source、session、age、flags 门限不变。每张原生 FrameTicket 按值保留 sample/reference/reference_generation，旧票不修改；日志最多保存前 32 次参考变化和最新一次，超出时不宣称完整历史。CPU oracle、生命周期和独立代码审查通过，实际功能以以下游戏试验为准。

PID 38784 的回中控制器因跨时钟域比较而未发请求，native 为 requests0/generation1，因此回中未测。该轮还存在独立的输出失败：XR 最大提交间隔 3000.6125 ms、67 次 stale 拒绝，exit1；native 四阶段各 6,461 对、GPU 发布/接收均 6,299 张，正常清理和 28 入口恢复通过，随后游戏正常关闭。间隔内 XR/raw pose 与 native pairs 仍持续且处于有效前台状态，不能归因于失焦。聚合记录有 147 次 Present 污染，但缺逐次时间和调用者，尚不能据此确定缺图原因。另有一行 stereo_published0 导致严格来源窗口检查失败，不能把所有 tracking flags 有效当成桥发布必定成功。证据 `stereo-xr-38784-attempt1.recenter-failed-audit.json`、`.failure-audit.json`、`.failure-audit-addendum.json`。

修正时钟后 PID 21208 控制器仍未发请求；实测其每次解析 66 MB 的完整 native CSV 耗约 1.878 秒，导致读完即超过 100 ms。这是测试控制器问题，不能宣称回中 native 功能失败或成功。修复方向是只读有界日志尾部并使用原生 QPC 时钟，不放宽样本时效门限；原控制器源码、失败报告和原始数据保留，后续新进程重新验证。

PID 21208 的 projection 覆盖 119.956264 秒、最大间隔 28.8403 ms，通过输出覆盖分项；但 XR 实际 PID 43420 以 0xC0000409 退出，Windows 事件和 minidump 的参数 7 表示 fatal application exit，不能直接宣称栈缓冲区越界。事件匹配当前冻结 EXE，异常点为 EXE+0x18885；输出统计发生在清理之前，不能作为退出成功的证据。日志已到 Operator 的 DestroyInstance succeeded 后发生异常，具体调用链仍需定位。另有一行桥发布失败使严格 source 窗口检查失败。native 清理及 28 入口恢复通过，整轮仍为失败。证据 `.crash-metadata.json`、`.shutdown-events.json` 与原始 runner 报告。

随后在同一 PID 恢复 mono 后单独进行 10 秒动态眼点采样：437 条接受、44 条跨帧拒绝。仅短时右摇杆 yaw/pitch 正反输入，没有移动角色或启动 XR；Pawn 位置及 EyeHeight/BaseEyeHeight=60 保持不变。Pawn pitch 始终零，Controller/POV pitch 范围 -6..232 RU；290 条样本的 Pawn/Controller pitch 不同，23 条 Controller/POV 旋转不同。后者可能含非原子读与相机插值，不能据此确立延迟模型。该结果说明不能把三个旋转来源视为等价，仍不证明渲染对齐或稳定第一人称。证据 `player-anchor-21208-dynamic.*`，随后核对游戏身份正常退出，无游戏/XR/proxy 残留。

## Continuous3 六轴与两次回中通过（PID 36212）

保持上述 native3 SHA256，配合独立 XR `build-shutdown3/OpenXRPoseProbe.exe`（SHA256 `FA1DAD6A707C3C767695498F7988F6FEB8263ADC09DCD518548F7388E3C33EA3`）。XR 新版仅增加默认关闭的收尾阶段/terminate 诊断并保留 PDB；本轮显式启用诊断。先前 12 秒独立 pattern+Operator 生命周期烟测 exit0，但该烟测不作为游戏图像证明。

36212 使用修正后的 QPC、有界尾部控制器，在 yaw+15、pitch+10 目标运动中分别发一次请求，native 实际 requests2/committed_generation3，三个参考事件完整保存。第二代在非零偏航提交，第三代在非零俯仰提交；各参考只含 LOCAL Y-up heading，真实俯仰未被归零。旧帧按其冻结代际检查，不用最新参考回算旧帧。12,192 条消费/准备左眼矩阵全部匹配各自参考和已发布 XR 候选，最大误差 0.00395 native units，FOV 系数最大误差约 2.17e-8；四阶段各 6,096 对、两眼矩阵和投影残差均零。动作时间窗口使用实际记录的 perf_counter/QPC，不推算旧 monotonic 偏移。位置回中及竖直 heading 延后仍只有 CPU 覆盖，本次没有在非零平移时请求回中。

新游戏图像发布/接收均 6,090 张，projection 8,639 帧覆盖 119.968899 秒、最大间隔 17.2533 ms，无 stale 或结构失败；XR bridge 本轮零发布跳过。完整六轴范围、来源/raw flags、GPU drain/close、生命周期与独立 28 入口恢复通过。runner/controller/native/XR 实际均 exit0；所有收尾标记完成，没有 terminate。恢复普通 mono 已直接查看，核对 PID/path/creation identity 后正常关闭游戏，无本轮 game/XR/proxy 残留。

证据为 `stereo-xr-36212-attempt1.json`、`.recenter-six-axis-independent-audit.json`、`.execution.json` 及对应 native CSV/status、lifecycle/restored 文件。execution 文件创建时的 `reference_application_independent_audit_pending` 是当时状态，最终回中结论取独立审计，不回写其历史字段。当前结果不抹去 38784 输出缺口或 21208 退出异常；仅证明本轮未重现。仍不是物理头显显示或稳定第一人称验收。

### 来源验证契约与第一人称下一步

`analysis/runtime/stereo-source-publication-consumption-contract.json` 区分 XR 采样、成功发布、native 接受/消费和 GPU 发布。现 runner 在 native 首末 sequence 的整个区间要求 `stereo_published=1`，这是生产者零跳过策略；未发布的有效采样并不等于 native 消费了坏姿态。历史严格失败不改写；当前 CSV 缺逐票冻结 session/sequence/received_ns，不能由区间或矩阵候选匹配宣称完整精确消费集关联。若以后调整策略，先增加逐票证据并保留发布跳过统计，原 source/session/100ms/geometry/fence 门限不变。

IDA 进一步分析 SceneView 后续位置消费者 RVA 0x2718c8 及两条空间查询。已识别汇编没有直接改写传入位置，但会更新 LocalPlayer 的位置相关渲染/插值状态；因此只恢复临时坐标不等于消除所有副作用。记录见 `analysis/ida/camera-110878-20261014/position-consumers-note.md`。下一项可隔离为修改原生重建视图的 base origin 的几何实验，先证明同次 Pawn 眼点与 source view 的关联、再检查身体遮挡；它仍保留原相机选择的其他渲染状态，不能称完整第一人称。

2026-10-14 的 IDA 定点跟进确认了主路径的静态联系：上层 RVA 0x2c78f4 在 0x2c7bf5 调用 SceneView，随后 0x2c8904 以同一栈上 family 调用提交函数；后者在 0x3a1590 把该 family 传入 renderer 构造。导出的两段函数共 1,828 条已识别指令均与 PID 16812 的已验证运行时快照字节一致。Scene 返回后存在直接改写 source+B0..E0 投影的条件分支（0x2c80d8..0x2c80f6）；不能用整个 128 字节矩阵始终相等作为关联条件。视图的前 64 字节仍严格比较，三次投影另存观察值，不改变已有 stereo gate。静态导出不覆盖所有冷分支或间接调用，也不证明这一分支在新测试中执行。

为验证玩家眼点与实际 source 的关联，新增默认关闭的 `--anchor-observer`。它在原 Scene 返回后读取当前 Pawn，而非倒推此前 viewpoint callback 的输入；通过反射证书、对象注册槽、双向关系、ViewTarget、身份及时间戳括号核对候选 `Pawn.Location + (0,0,EyeHeight)`，用 TLS 单次回执关联 exact source/family 与 renderer 构造。嵌套、重复消费、来源不符及跨线程无回执均拒绝。只记录首 4 次和其后每秒一次的 bounded 回执，另外保留拒绝计数和少量首例；不修改原相机、Pawn 或视图，不证明原子快照、眼骨位置或第一人称效果。静态证据见 `analysis/ida/anchor-scene-family-20261014/association-review.md`，构建及 CPU 检查见 `analysis/runtime/fp-anchor5-native-ready.json` 与 `anchor-observer-python-evidence.json`；运行结果单独记录。

第一次实测 PID 43296 在 `GearsCaptureStart` 调用前失败：Python 证书取样遇到 `Cross-frame camera timestamp during identity checks`。未生成证书/INI、未启用完整 capture、没有游戏 projection 或摇杆测量输入；owner/XR 均 exit1，XR 按 stop event 正常清理。priming watchdog 恢复了实际启用的 6 个 descriptor/factory 入口，独立字节核验均匹配；不能写成 28 个完整 capture 入口恢复。游戏随后正常关闭，旧 driver 自然超时结束，预存 Simulator 未关闭。原始失败保留在 `analysis/runtime/anchor5-43296.audit.json`。后续修正仅允许在固定时间和次数内重新取得独立、严格通过的样本，不接受跨帧样本，也不重用失败值。

修正后的 PID 44200 在首次尝试即取得严格样本（0.743 ms），没有实际触发重试路径。Scene/family/renderer 各接受 3,727 次、拒绝计数零；64 条保留回执的身份、单次调用关联、时序和 view64 字节检查均通过，其中 58 条关联了非零 registered ticket。其余 3,660 条因采样策略不保留，另有容量满后的 3 条丢弃，不能宣称取得全部逐帧原始证明。两次 right-stick 0.25 / 0.25 秒请求成功并归零，但 64 条记录只有一种相机 basis，实际转向未验证。原视点与候选眼点的差约为 (+160,-48,-16) native units；候选仍未应用。证据为 `continuous-stereo-44200-20261014-065454-782a5b98.independent-anchor-audit.json` 及同前缀原始 status、证书、driver 和执行审计。

该轮双眼输出覆盖 61.1024666 秒，4,401 次 projection，最大提交间隔 16.4588 ms；四阶段各 3,346 对且 mismatch=0。GPU 发布 3,330、XR 复制 3,329，严格等数仍为 false。owner/driver/stop/XR 均 exit0，native drain/close、独立 28 入口恢复及普通 mono 核验通过；正常关闭游戏，无本轮进程残留，预存 Simulator 保留。上述结果只证明静态关联及这一轮模拟器输出，不能由输入请求成功推导动态关联、第一人称或物理头显通过。完整执行审计为 `analysis/runtime/anchor5b-44200.audit.json`，生命周期独立审计另存。

随后准备以历史产生过相机转向的 0.35 输入做 30 秒补测，但 PID 15204 在进入关卡前触发 120 秒 player readiness deadline，未激活 native capture，未启动 XR，未发送测量输入。没有延长门限或沿用旧证书；prime watchdog 正常恢复实际启用的 6 个入口，独立检查匹配，full28 为不适用。owner 与 driver exit1，游戏正常关闭且无自有进程残留。原始失败与独立复核分别见 `analysis/runtime/anchor5c-15204.audit.json`、`anchor5c-15204.independent-startup-audit.json`。这是启动流程失败，不计为动态测试通过或双目渲染失败；下一步先缩短菜单操作的外部等待，再进行新的有界动态关联测试。

PID 38012 改用独立的菜单协同脚本与本轮已查看的截图模板：保持 distance<12、margin>12、两次连续识别、进程身份和分辨率检查，使用较窄的文字区域排除背景干扰。离线复核通过后，实际 5 次菜单确认在约 45.35 秒内完成，coordinator exit0，player gate 通过；没有延长 120/180 秒期限。原始记录为 `analysis/runtime/anchor5d-menu.json` 与 `menu-anchor5-offline-replay.json`，生产菜单模板未改。

但该轮在 `anchor_config.py` 的证书组装后 `after=sampler.guard()` 再次遇到跨帧时间戳拒绝；此前的有界重取仅覆盖首 sample，未覆盖这个复核边界。native capture 未激活，没有 first publication 或测量输入；XR 已启动并发布 3,077 个 pose，但游戏 copies/projection 均零。停止后 XR 日志到达 `destroy_instance_returned=0`、`cleanup_return=1`、`main_return_pending=1`，实际进程退出却为 `0xC0000409`，不能写成自然 exit1 或正常退出成功。owner/driver 均 exit1；XR cleanup actions 为 stop_signaled/wait，无强制终止。实际启用的 6 个 prime 入口独立恢复通过，full28 不适用；游戏正常关闭，无自有进程残留。原始执行审计为 `analysis/runtime/anchor5d-38012.audit.json`，同前缀 `.sources-manifest.json` 和 `.source-*.py` 冻结本轮实际 Python 源码；旧 PID 44200 证书另存并标注来源，不作本轮证据。新异常转储 `OpenXRPoseProbe.exe.45492.dmp` 须离线分析；尚不能因异常码相同就认定与旧 PID 43420 同一根因。

新转储的 DbgHelp `StackWalk64` 得到 32 帧真实展开栈，异常线程 24296 与主线程日志一致。匹配 PDB 将 RIP 定位到 probe 的 `abort+0x35`；调用链为 `main` 返回后的 CRT exit → `LdrShutdownProcess` → SIMULATOR 的 onexit → `SimRpcManager` 析构。IDA 定点分析该析构函数，结合捕获栈里的零线程句柄及原始 C++ 异常对象，确定 `_beginthreadex` 返回 0 后执行 `_Throw_Cpp_error(6)`，抛出 `std::system_error` 并进入 noexcept terminate。对象中的 11 是 C++ generic error_code（`resource_unavailable_try_again`），不是 GetLastError。尚未证明线程创建失败的操作系统层原因，也不将最近导出名的大偏移当作实际函数名。此处定位的是本轮模拟器退出故障，与 Scene Render/黑屏根因分开；完整证据及磁盘指令覆盖限制见 `analysis/runtime/xr-crash-45492/findings.json`，自有 IDA lease 均已关闭。下一项可用冻结 probe 的 3 秒默认 pose 模式独立验证正常进程退出，不必加载游戏或增加 Hook。

唯一一次独立隔离测试 PID 20656 使用冻结 probe 的 `3 <新CSV> --shutdown-diagnostics`，没有游戏、native Hook、stereo/Operator 或配置变更。实际取得 209 帧有效且 tracked pose；程序记录 `main_return_pending=0`，进程仍以 `0xC0000409` 退出。总体进程观察跨度 15.2671 秒（包括初始化、清理及异常退出），未超时或强制结束。新转储独立展开的 32 帧模块/RVA 序列与 PID 45492 完全相同，异常线程 26588 同样与主线程日志一致；保存的线程返回值为 0，原始异常对象仍为 generic 11 的 `std::system_error`。因此同一直接退出故障已在完全没有游戏 Hook 的路径复现，不需要继续增加游戏侧日志定位它。底层线程创建失败原因仍未知，不能把异常抑制或强制退出作为修复。原始审计为 `analysis/openxr/xr-exit-isolation-20261014-072950-578351.json`，同路径核验为 `analysis/openxr/xr-crash-20656/same-path-audit.json`；本轮 probe 无残留，预存 Simulator 14472 未动。

随后 Python 修正覆盖了 initial、sample_before/after、assembly_after 和 final_boundary 的每次严格 guard；guard 层仅对明确的跨帧时间戳异常最多重取 5 次、200 ms，其他身份、类型、registry 或未知失败立即拒绝，超时后才完成的好样本也拒绝。guard 耗尽不会被 sample 层吞掉后重新循环，原 sample 的 before==after、重取分类及自身预算保持。证据保留每层实际次数、时间、阶段及可取得的 partial guards/raw values，无法取得的值明确为 null，不声称总共只有 5 次内存读。49 项 CPU 测试通过；采用已存 PID 44200 profile 的最大重试结构离线序列化为 158,888 字节，低于 driver 的 256 KiB 限制，但不是任意更大证书的普遍上界。最终修正与 CPU 证据见 `analysis/runtime/anchor-observer-guard-acquisition-fix-evidence-v2.json`；尚未再次进行实机激活，不把测试通过等同于启动故障已实测消除。

第一人称 A1/B/A2 的纯 CPU 辅助模块 `tools/native-capture/origin_trial.h` 已独立编译测试，但未接入 capture 或更新运行二进制。阶段由构造前的 QPC 选择并随票冻结，首个成功 admission 才提交 epoch；A1/A2 各用当前原 view，B 对局部副本设置候选原点。独立矩阵反算的最大误差为 0.00476638 native unit，低于既有 0.02 限值。边界、失败不推进、旧阶段不随晚消费改写及非法输入测试见 `analysis/runtime/origin-trial-helper-20261014/`。连续模式关闭普通 native CSV，后续实测需要少量按阶段保留的完整冻结矩阵和 XR 接收票证据，不能把 pose CSV 当作逐票渲染证明。

## 统一入口回中通过、整体失败（PID 41704）

同一组冻结 native3 / XR shutdown3，统一入口 strict source0，默认关闭诊断且没有 Operator。两次无 PID 回中命令均成功，status 核对当前会话后观察到代际 1→2→3；原生四阶段各 3840 对，矩阵/投影残差为零。3748 张发布图像全部由 XR 接收；4981 次 projection 覆盖 70.7055067 秒，但最大间隔 1431.7773 ms、stale_ticket 73 次，不能视为稳定输出通过。

runner 另记录 `xr_cleanup_error=Session process identity changed or exited: 9232`。它先 poll 自有 Popen，随后重新按 PID 查询运行状态，两次调用间子进程退出会令查询失败；旧异常路径可能立即 terminate 尚未结束的自有子进程。本次没有强制操作记录，故只是已识别的代码竞态，不能证明实际执行了 terminate，也不能把输出间隔作为 XR exit1 的原因：continuous XR 返回码没有使用 maximum_gap 条件。owner/stop/XR 实际均 exit1，原始结果保持 false。

native drain/close、生命周期、28 入口恢复及 SBS/mono 图像核验通过，游戏正常关闭，无本轮 game/XR/proxy 残留。不可变证据为 `vr-launcher-smoke3-41704.audit.json`、`.index-initial.json`、`.index-final.json` 及 `continuous-stereo-41704-20261014-055446-f3ced5d3.*`。连续 runner 的 passed 为既有生命周期契约，输出健康仍须单独审计，不把前者等同于 100ms 无缺图。

缺图累计量尚不能定位时间：3840 张票到 3760 次 linked 差 80，与 76 次污染及少数 metadata 拒绝相容，但没有逐次 QPC；3760 次 capture 到 3748 次发布差 12，busy9 不能解释所有支路。4254 次 unknown execute 中 4209 次来自已隔离的 original-Present 内部，不能全部视为拒图原因。下一项限定为现有 Present/capture 路径上的有界缺图窗口记录，关联拒绝原因、票号、fence 和 XR 缓存图像年龄，不增加 Hook 或放宽门限。第一人称集成暂缓。

复核旧 PID 38784 的完整 native CSV：既定缺图包络内 151 次已记录 Present 的最大调用耗时 2.7362 ms、相邻起点间隔最大 27.9438 ms，151 次 renderer consume 也持续出现。这些调用的耗时包含 pre-capture、original Present 和 retire，是 original Present 耗时的上界，不支持已记录调用阻塞三秒。全局 dropped97 缺少时段/类型分类，frame mutex 的入锁等待不计在内，仍不能排除漏记；CSV 没有最终 Draw 或逐 Present 拒绝字段。证据 `stereo-xr-38784-attempt1.failure-audit-addendum2.json`，不改原失败结论。

退出清理已改为使用自创 Popen 保留的进程句柄，不再在 poll 后重新按 PID 打开查询；停止事件已关闭时仍等待自然退出，仅真实 wait 超时才允许 terminate/kill 自有子进程，强制原因和操作明确记录。外部 stop/recenter 的身份核验保留。22 项 continuous 与 16 项 launcher 离线测试通过；这是代码修正和离线证据，不证明 41704 实际执行过旧 terminate 分支。

定向诊断版本 `build-outage4` 默认关闭记录，游戏端仅在现有外层 Present 中保存首个超过 100 ms 的发布间隔，最多 256 条；built 使用一致原子快照，Draw 沿用既有区间隔离，收尾在无活动回调且 Hook 已停用后打印。XR 保存首窗口及最长间隔边界，总计不超过 256 条，区分新图/缓存复用和未知票据。XR 行时间在 xrEndFrame 之后，所列 age 不是更早的 projection gate 精确年龄。GPU 票据 ABI、单槽 ack、拒绝条件和立体矩阵未变。CPU、单格式 D3D12 trace/null/busy/票保护与关键 ABI 检查通过；D3D12 debug layer 不可用，尚不能将这些自测当作游戏运行证明。诊断构建和哈希见 `native-outage4-evidence.json`、`native-outage4-ready-addendum.json`、`outage4-postproducer-cpu-build-checks.json`。

## 动态来源关联与第三人称阶段优先级（2026-10-14，PID 42228）

continuous-stereo-42228-20261014-073900-9ae09592 使用冻结 anchor5 / outage4 和证书全边界有限重取版 Python。自动菜单五步完成，从首次 published 开始测量 30.0212 秒；+8/+18 秒各一次幅度 .35、持续 .5 秒的右摇杆输入，两次均归零。5 个证书 guard 全部首试成功，本轮未触发实际重试。

Scene/family/renderer 各接受 1,912 次，所有拒绝理由为零。36 条保留记录加 1,876 条 sampled-out 等于 1,912，没有 capacity drop；三阶段 view64 全等、身份与 QPC 顺序通过。30 条正 ticket 关联至已登记 renderer，6 条 ticket0 为登记前样本。4 种原始 source basis 独立推导 yaw 跨度 4.279175°、pitch 跨度 2.675171°，补足 44200 静态样本无法证明的动态变化。277 条 sampled XR pose 同时出现 5 种 head quaternion，相对首姿态最大 13.411491°，所以不能把所有双目变化仅归因游戏输入；仍不是物理头显或原子引擎快照证明。

两眼四阶段各 1,551 次，矩阵和投影残差为零。XR 2,172 次 projection，覆盖 30.1476066 秒、最大间隔 16.5231 ms；native 发布 1,541 张、XR 复制 1,540 张，严格等数仍为 false。owner、driver、menu、stop CLI、XR 实际退出均为 0，28 个入口恢复，mono 截图核验后正常关闭游戏，无自有残留；预存 MetaXRSimulator 14472 保留。原始执行、动态关联及最终退出分别见 anchor5e-42228.audit.json、该前缀 .independent-dynamic-lifecycle-audit.json、.independent-execution-supplement.json。

默认关闭的第一人称 origin_trial 已实现 A1/B/A2 各 10 秒选择：精确本次调用回执、128 字节 renderer 矩阵一致才生成局部基准视图；B 使用候选 Pawn+EyeHeight，A1/A2 使用各自当前原始视图。选择在构造前冻结，首次 lifetime admission 成功才提交 epoch，元数据按值跟随 renderer。每阶段最多保留 12 个 ticket 的两眼四阶段矩阵、原始视图、实际基准及完整 pose/reference/FOV。第一版 frame-link 自测因局部 64 槽 Lifetime 增大而发生栈溢出；将该自测对象移至堆后，最终 13 组原生检查通过。生产 Lifetime 始终在静态区。详见 fp-origin1-native-ready.json、fp-origin1-code-review.json。此实验尚未在游戏运行，不能宣称第一人称成立。

按用户提出的第三人称阶段目标，当前约 75% 是工程估计，不是量化验收比例。优先持续输出、停止边界、恢复和真实头显整链路；第一人称不是该阶段的前置条件。XR 新增显式、默认关闭的 image-ticket-diagnostics，仅保存实际新复制或首次成功提交新 session/ticket 的接收回执，堆上固定容量 4096 条，停止后输出并报告溢出。每行保留真实 receiver serial、ticket 与来源，不能用 producer CSV 代替。共享 ABI、GPU 等待与渲染门限不变；8 项 XR CPU/CLI 检查通过。第三人称 receipt1 将使用旧 anchor5 native 配合新 XR，检查已发布 serial 接收连续性和停止尾部；原有差 1 和历史长缺图在取得新证据前仍保留。

receipt1 已实际完成，前缀 continuous-stereo-24396-20261014-075809-a309b0bc。driver 从首次观察 published46 开始测量 60.0000479 秒，无测量输入、回中或截图；实际 XR 输出覆盖 61.0634224 秒、4,398 次 projection、最大间隔 16.7439 ms。3,101 条接收回执无容量丢项，serial 严格从 1 连续至 3,101，copy_count 逐条等于 serial，每个 serial 均有成功提交回执；header submitted_frames 与 projection 总数同为 4,398。native published3102 / last_ticket3108，XR 最后 serial3101 / ticket3107，terminal producer_stopped 仍为同一已接收 ticket；ticket3108 未见于任何 receiver 行。本轮差 1 因而定位到最终未接收发布，不是已接收序列内部的编号缺口。尚未保存最终共享 packet 的完整 sample、done fence 或精确停止竞态时间，因此不扩展为完整逐包停止时序证明。

owner、driver、menu、stop CLI、XR 均实际退出 0，无强制清理；native GPU drain/close、全部 28 入口恢复、mono 图像核验和游戏正常关闭通过，无自有残留，预存 Simulator 14472 与 game_io 服务保留。严格发布/接收等数仍为 false；历史长缺图和模拟器退出异常未因本轮正常而改判。证据为 receipt1-24396.audit.json 和该前缀 .independent-xr-tail-audit-v2.json。现有 mark_stopped 阻止后续 acquire，而 producer drain 只等待自身 GPU 拷贝完成，源码与观测相容。若未来允许已有接收者消费正常停止时的最后一张图，需要单独证明正常停止身份、资源寿命、完整元数据及过期 pose 拒绝；现有 stopped 布尔不区分故障停止，不能全局绕过。风险评审见 xr-tail-receipt-prerun-review-v2.json。

## 单次缺图诊断复测（PID 1756）

隔离入口复用原 launcher 的版本、priming 和关卡玩家判定，仅选择已冻结的 outage4 构建并显式开启两个诊断选项；生产 manifest 不变。driver 在启动前挂好，从首次观察到本轮 published>0 开始，60.0017217 秒后调用 stop，期间没有截图或移动。两次回中命令成功且实际参考代际 1→2→3。原生四阶段各 3408 对、矩阵/投影残差零；XR 提交 4331 帧，覆盖 60.1400841 秒，最大间隔 18.0283 ms，未出现超过 100 ms 的提交间隔。这仅表示本轮未重现旧间断。

owner、stop CLI、XR 实际均 exit0；收尾 actions 只有 wait，forced reason 为 null。DestroyInstance、cleanup 和 main return 标记完整，native drain/close、28 入口恢复及 mono 核验通过；游戏正常关闭，无本轮进程残留。306 条采样 CSV 的 source0/head15/view15/published1 均通过，但连续模式仅采样记录，不能声称验证了所有 XR 采样行。生命周期独立审计见 `continuous-stereo-1756-20261014-062542-7b2fcbc1.independent-lifecycle-audit.json`。

native 最后发布 3386 张，XR 复制 3385 张，严格等数仍为 false，不能由 runner 的生命周期 passed=true 覆盖。native 有界窗口在停止请求后触发并留下两行未恢复记录，XR 的缺图窗口未触发；这两种窗口不能混称为运行期间故障。停止尾部的精确时序另行审计，不追零改写统计。总证据及不可变索引见 `outage4-1756.audit.json`，原始前缀 `continuous-stereo-1756-20261014-062542-7b2fcbc1`。

独立尾部审计确认：两条 native 行分别在 XR terminal 之后约 67.9966/90.8114 ms，原 Present 耗时仅 0.2518/0.4179 ms；Draw 和 capture 字段无有效位，不能把所列零值当作 fence 实测。最后发布发生在 driver 开始调用 stop 后 88.9568 ms，XR terminal 再晚 9.2083 ms。terminal 实际保留 serial3385/ticket3406/session13211648185900/sequence5072，而 publisher 的最后票为3407、总发布3386；结合单槽源码支持最后一次发布未被本 receiver 复制/ack。最终 packet、done fence 和票3407的完整 sample 未保存，不能称逐包直证；native event barrier 的精确 QPC 也未记录。详见 `.independent-outage-tail-audit.json`，严格等数和全项结果保持 false。
