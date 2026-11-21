# New model to original engine resources: already identified implementation route

2026-10-28. This round is completed N 01 and the static technology route:**The original engine created the independent temporary StaticMesh  ⁇  native LOD/toppoint initialization of original components binding the original rendering resource release and GC.** The data in input format is prepared to be matched. The loader has not been achieved and no game functions have been performed; the current process ABI, actual image and destruction are still part of the access validation.

## Evidence and scope of application

Exact Functions, RVA, Block SHA 256, command evidence and structure fields are kept in private [native-resource-entry-map.json](../../analysis/technical-prep-20261028/native-resource-entry-map.json)Check to read [native-resource-audit.json](../../analysis/technical-prep-20261028/native-resource-audit.json)。30 Selected entry points for functional roles, and total 39 function,5,201 The command records are consistent with the same historical bytes.256 No change.

History `render-16812-verified`, module base `0x140000000`，EXE SHA 256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. It is not the current running process; it cannot simply skip a version, byte, call an agreement and object identity check on the same RVA. The old cache compiler/part base metadata have mistrued the pseudocode repository with constant interpretation, the latest export only amends these metadata in the second private database, all conclusions being based on actual instructions. The first failed copy and failure are exported without using them as evidence of success.

## Unify Backends with First

Add `tools/playground/weapon_part_assets.inl` As the only back end, along [Weapons hand-over](../weapon-specs-20261028/implementation-handoff.md) I 05 Connect. Two types of input share the command handle,pose, visible, destroy, owner/world creation and failed recovery:

- `OriginalResource`: Reuse of the original cartridge, Boomshot filling assembly, Torque Bow arrows along existing engine object/bones.
- `AuthoredGeometry`Four pieces of geometry supplement, Lancer starter and private Snub dismantlings follow the following original resource routes. GLB is open as a source; Snub remains the original game's private extraction and does not enter this package.

The first fixed to Gnasher's full live ammunition, with only one example of a LOD. The first piece is created, held, inserted, visible, released and re-used backend without another loader for each gun. The resource cache key is Hashi+Paint Version + Unit Conversion; the component handle is bound separately by the task/owner configuration. Default does not cross the world save new resources, release them normally and rebuild them.

## Access sequence and real entry

Below is the map of coding responsibility and existing access; full address/byte and call repository is based on a private machine schedule. All UObject, array and status changes are entered to the only available game line execution end; resources upload and release to the original engine queue.

| Steps | Accessories detected/evidence | Realization of requirements |
| --- | --- | --- |
| 1 Create new resources | `StaticClass` → `sub_140193078` (a) General object construction;`sub_14014CF38` Examples of packagings for existing temporary objects | Use the actual StaticMesh class, temporary Outer, unique identity and engine tectonic process. The memory cannot be disguised as UObject or call nudity instead of initialisation/default properties |
| 2...to protect life. | RootSet Name Map, Original Program Direct Location/Search and GC Consumption Three Cross-Efforts | Only the new resource at this backend is given its root reference lease; it was created and established in the same game line execution, before being released or queued. The Outer cannot be used as a root reference. The original resource is never marked. |
| 3 Create LOD | `sub_140153D08` Allocation/additional, then `sub_140153F7C` Initialize | The memory returned by the partitioner has not been constructed, and must be constructed in order of the original caller. Saves the original buffer object, a false table and the default values for the selected array |
| 4 Filling in vertex points | `sub_140BC9830` tangent/UV、`sub_140BC9698` position、`sub_140BC9590` color | Temporary input 60 bytes vertex array; internal generation of the first two 2×N storage, input numbers and section index remain N. No more than double it on its own |
| 5 Material clustering/indexing | Original 56 Bytes section allocation/default values/sequencing;`sub_1401CF460` Original Numeric Capability | Use the array assigned by the engine to fill out first arrays, triangle count, Min/ max vertex, motor index and real physical Internetface references. Empty support arrays keep their original construction default; do not copy any original array ownership pointers |
| 6. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . | Original Sequencing and original Build Consumption Co-accordation Center, Half Size, Radius | Calculates from the actual vertex of the local coordinates that have been moved to the engine. N= input numbers, all index numbers are smaller than N; no zero encirclement boxes can be used or otherwise cut |
| 7 Initialization and binding | `sub_140153580` → `sub_140153D58` • Former BeginInitResource/vertex factory; components `SetStaticMesh` | The data is complete before initialization. The creation period component is not displayed/not registered to the scene; release as originally bound/updated. Returning "Called" does not count GPU completion; first must check actual display |
| 8 Release | Mesh BeginDestroy  ⁇  ReleaseResource Resources > render fence; Is LadyForFinishDestroy waiting for fence | First, remove the interactive handle, hide and normally untie components, clear the root lease held only at this backend, allow the original engine to destroy resources and wait for a fence. No old pointer can be de-referenced after the roots are cleared, no UObject/GPU memory is manually released, no hot frame etc. |

The root lease is held only after it has been established that the resource was newly created and not originally rooted; the cache internal reference count is zero. Saves “this bit overside” and does not restore the entire flags. Failure occurs before release, as well as following the same engine clean-up path; failure occurs when a component is cancelled after release. Reporting on specific failures and stopping reuse if clean-up is unverifiable, without forcing free or regenerating duplicate resources.

## Created Input Protocol

Complement [engine-input/manifest.json](../../assets/weapon-6dof/offline-v2/engine-input/manifest.json) Save SHA for each data file and source GLB 256, clusters, materials sources and quantity. 11 Grid input: four new models each have two LODs, with three independent motor components for start-up. 9,872 triangles,29,616 vertex. They are not cookies or UStaticMesh that can be loaded directly.

| 60 Byte Input Record | Offset | Rule |
| --- | --- | --- |
| Position float 3 | 0 | GLB local rice;backend only converted once |
| Packed Tangent X / Y / Normal | 12 / 16 / 20 | Each group 4 bytes with byte/127.5−1;oldinitializer calculates final handworthiness |
| 4 UV float 2 | 24 / 32 / 40 / 48 | Current Use UV 0, the rest is 0;starter unsituated UV, fixed to 0, texture normal materials only |
| Color uint 32 | 56 | Current Full White; original Initializer can omit all white alone |

Triangle angles are spread, and the GLB is maintained with each angle line;16 Bit index numbers by material group. Maximum single-part input 10,944 Point, native. 2×N storage is not beyond 65,535. Enter the generator and independent reader to check source Hash, position/UV corner point equal, index range, grouping over and methodical quantification error without calling an engine.

The conversion allows only the certified arc to change and add up the normal flat scale. Positions are converted to local units of existing engines, where the legal/trigger rotation is followed by repackaging; if the axis changes hand, with a triangle twisting and cutting-linearity. Unmarked, non-balanced or tilted, not rated in unit matrix. Capture point/fill node is converted along the same transformation, so geometry and crosspoint cannot be scaled separately.

The materials are bound by the input list groups and actual Internet game; GLB PBR parameters are not used as original engine parameter names. The grouping of metals, cartridges, warheads, primers, handles has been defined, and the raw resource material and available parameters are read back to authentic objects at the time of access; no shadow parameters are assumed. The first acceptance check on missing materials and legal lines failed to keep a specific slot report.

## Why not use RawTriangles Build?

Former Mesh.Build and 252 Byte RawTriangle layouts have also been found, containing package marking checks, bulk lock/reese/unlock, resource release/reconstruction process. It recalculates the method lines; this round is unable to confirm all the angled lines that have been created when trying to load them with smoothing groups. The freeze is therefore for direct calls to original vertex initializers and no longer uses the original triangle as a default path. Failed input and research evidence are stored only in private anallysis without mixing into delivery inputs.

It's not the announcement that the engine is all about expression. 32 A smooth area; failure is the current smoothing of the wheel, and recalculation does not in itself guarantee consistency of origin. The new route retains its original resource life cycle while avoiding reliance on this unproven reconstruction equivalence.

## Validation responsibilities on first access

The sequence of code implementation and data formats are fixed, and the technical route is no longer required. The current bytes/ABI, object fields, original array distribution, correct threads, material references, eyes/deepness, GC and switch exits must be verified after access has been granted. These items cannot be replaced by offline data or historical snapshots.

Additional boundary: New components close game collisions/harms, capture and load independent interactive volumes; the bomb nest maintains six separate slots that cannot be sealed with a whole convection. Load only one LOD, not two scene at the same time; start three parts of the package move by their respective nodes. Access failure only disables the corresponding new resources and roll back to the business without returning the main line source or changing the default playpack or showing the route.

---

## 中文

# 新模型到原引擎资源：已确定的实施路线

2026-10-28。本轮已补齐 N01 的静态技术路线：**原引擎创建独立临时 StaticMesh → 原生 LOD／顶点初始化 → 原组件绑定 → 原渲染资源释放及 GC。** 已准备匹配输入格式的数据。没有实现加载器，也没有执行任何游戏函数；当前进程 ABI、实际画面和销毁效果仍属于接入验证。

## 证据与适用范围

精确函数、RVA、代码块 SHA256、指令证据和结构字段保存在私有 [native-resource-entry-map.json](../../analysis/technical-prep-20261028/native-resource-entry-map.json)，核验结果为 [native-resource-audit.json](../../analysis/technical-prep-20261028/native-resource-audit.json)。30 个功能角色对应的选定入口，以及共 39 个函数、5,201 条指令记录，均与同一历史快照字节一致。原 IDA 数据库 SHA256 未改变。

历史源为 `render-16812-verified`，模块基址 `0x140000000`，EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。它不是当前运行进程；不得仅凭同一 RVA 跳过版本、字节、调用约定和对象身份检查。旧缓存的编译器／段基址元数据曾使伪代码寄存器与常量解释失真，最新导出只在第二份私有数据库修正这些元数据，所有结论以实际指令为准。第一份失败副本及失败导出保留，不采用它们作为成功证据。

## 统一后端与首件

新增 `tools/playground/weapon_part_assets.inl` 作为唯一后端，沿 [武器交接](../weapon-specs-20261028/implementation-handoff.md) I05 接线。两类输入共享 component 句柄、pose、visible、destroy、owner/world generation 和失败恢复：

- `OriginalResource`：原弹匣、Boomshot 装填组件、Torque Bow 箭等沿已有引擎对象／骨骼路线复用。
- `AuthoredGeometry`：四件补做几何、Lancer 启动件及私有 Snub 拆件采用下面的原生资源路线。公开 GLB 为制作源；Snub 仍是原游戏私有提取物，不进入本包。

首件固定为 Gnasher 完整实弹，只实例化一个 LOD。完成首件的创建、持握、插入、显隐、释放读回后复用后端，不为每枪另写加载器。资源缓存键为内容哈希＋协议版本＋单位转换版本；组件句柄另外绑定 world/owner generation。默认不跨 world 保留新资源，先正常释放再重建。

## 接入顺序与真实入口

下面是编码责任和已有入口的映射；完整地址／字节与调用寄存器以私有机器表为准。所有 UObject、数组和状态更改进入现有唯一游戏线程执行端；资源上传与释放交给原引擎队列。

| 步骤 | 已查到的入口／证据 | 实现要求 |
| --- | --- | --- |
| 1. 创建新资源 | `StaticClass` → `sub_140193078` 通用对象构造；`sub_14014CF38` 为已有临时对象构造包装实例 | 使用实际 StaticMesh class、临时 Outer、唯一身份和引擎构造流程。不能把堆内存伪装成 UObject，也不调用裸类构造代替注册／默认属性初始化 |
| 2. 保活 | RootSet 名称映射、原程序直接置位／清位和 GC 消费三处交叉证据 | 只给本后端新建资源取得自己的根引用租约；创建后在同一次游戏线程执行中建立，发布或排队前完成。不能把 Outer 当根引用。原资源绝不改标志 |
| 3. 创建 LOD | `sub_140153D08` 分配／追加，随后 `sub_140153F7C` 初始化 | 分配器返回的内存尚未构造，必须按原调用者顺序构造。保留原缓冲对象、虚表和可选数组默认值 |
| 4. 填顶点 | `sub_140BC9830` tangent/UV、`sub_140BC9698` position、`sub_140BC9590` color | 输入临时 60 字节顶点数组；前两者内部生成 2×N 存储，输入数与 section 索引仍为 N。不能再自行翻倍 |
| 5. 材质分组／索引 | 原 56 字节 section 分配／默认值／序列化；`sub_1401CF460` 原数组扩容 | 使用引擎分配的数组，填 first index、triangle count、min/max vertex、material index 和真实 MaterialInterface 引用。空辅助数组保持原构造默认值；不复制任何原数组所有权指针 |
| 6. 包围盒 | 原序列化和原 Build 消费共同确认中心、半尺寸、半径 | 从已转到引擎局部坐标的实际顶点计算。N=输入数，索引都小于 N；不能使用零包围盒，否则可能被裁剪 |
| 7. 初始化和绑定 | `sub_140153580` → `sub_140153D58` → 原 BeginInitResource／vertex factory；组件 `SetStaticMesh` | 数据完整之后才初始化。创建期组件先不显示／不注册到场景；按原绑定／更新路径发布。返回“已排队”不算 GPU 完成；首件必须检查实际显示 |
| 8. 释放 | Mesh BeginDestroy → ReleaseResources → render fence；IsReadyForFinishDestroy 等待 fence | 先撤回交互句柄、隐藏并正常解除组件绑定，清掉仅由本后端持有的根租约，让原引擎销毁资源并等待 fence。不得在清根后继续解引用旧指针，不手动释放 UObject／GPU 内存，不在热帧忙等 |

根租约只有在确认资源是本事务新建且原先未被 root 后才能持有；缓存内部引用计数归零才清位。保存“本后端加过这一位”，不恢复整份旧 flags。失败发生在发布前也走同一引擎清理路径；失败发生在已发布后先撤销组件。清理不可证实时报告具体失败并停止复用，不强行 free 或重新生成重复资源。

## 已制作的输入协议

配套 [engine-input/manifest.json](../../assets/weapon-6dof/offline-v2/engine-input/manifest.json) 保存每个数据文件和源 GLB 的 SHA256、分组、材质来源及数量。共 11 个网格输入：四件新模型各两个 LOD，启动件三个可独立运动部件。共 9,872 个三角形、29,616 个展开顶点。它们不是 cooked 包或可直接加载的 UStaticMesh。

| 60 字节输入记录 | 偏移 | 规则 |
| --- | --- | --- |
| Position float3 | 0 | GLB 局部米制；后端仅转换一次 |
| Packed Tangent X / Y / Normal | 12 / 16 / 20 | 每组 4 字节，分量为 byte/127.5−1；原 initializer 计算最终手性 |
| 4 个 UV float2 | 24 / 32 / 40 / 48 | 当前使用 UV0，其余为 0；启动件无原 UV，固定为 0，仅适用无纹理常色材质 |
| Color uint32 | 56 | 当前全白；原 initializer 可省略单独全白 color buffer |

三角角点展开，保留 GLB 每个角点法线；16 位索引按材质分组连续编号。最大单件输入 10,944 点，原生 2×N 存储也未超过 65,535。输入生成器和独立读取验证器检查源哈希、位置／UV逐角点相等、索引范围、分组覆盖和法线量化误差，且不调用引擎。

转换只允许已认证的刚体轴变换加正的统一尺度。位置换算到现有引擎局部单位，法线／切线旋转后重新打包；若轴变换改变手性，同时换三角绕序和切线手性。拒绝未标定尺度、非均匀缩放或斜切，不以单位矩阵冒充标定。抓点／装填节点沿同一变换转换，不能让几何和交互点分别套一次缩放。

材质采用各输入清单的分组与实际游戏 MaterialInterface 绑定；GLB PBR 参数不被当成原引擎参数名。金属、弹壳、弹头、底火、握柄的分组已定，原资源材质与可用参数由接入时的真实对象读回验证；不臆造 shader 参数。首次可视验收检查材质缺失和法线方向，失败保留具体槽位报告。

## 为什么不采用 RawTriangles Build

原 Mesh.Build 与 252 字节 RawTriangle 布局也已找到，含包标志检查、bulk lock/resize/unlock、资源释放／重建流程。但它会重新计算法线；本轮尝试用平滑组承载已制作的所有角点法线时无法保真。因此冻结为直接调用原顶点初始化器，不再把原始三角形重建作为默认路径。失败输入及研究证据仅保存在私有 analysis，不混入交付输入。

这不是宣布该引擎最多只能表达 32 个光滑区域；失败的是本轮平滑组映射，且重新计算本身不保证原角点法线一致。新路线保留原资源生命周期，同时避免依赖这项未证明的重建等价性。

## 首件接入时的验证责任

代码实施顺序和数据格式已经固定，不再需要选择技术路线。仍须在获准接入后验证当前字节／ABI、对象字段、原数组分配、正确线程、材质引用、双眼／深度、GC 和换关退出。这些项目不能通过离线数据或历史快照替代。

附加边界：新件组件关闭游戏碰撞／伤害，抓取和装填使用独立交互体积；弹巢保持六个独立槽位，不能用整体凸包封孔。仅加载一个 LOD，不同时画两个 scene；启动件三部分按各自节点运动。接入失败只禁用对应新资源并回滚本事务，不退回主线源码、不改默认游戏包或显示路线。
