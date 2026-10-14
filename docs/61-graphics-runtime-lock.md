# DLSS/DLAA 运行时锁：实际设置调用链

2026-10-15。目标是首版运行期间禁止重新启用未经测试的DLSS/DLAA，同时保留其他设置的正常应用。启动INI关闭已存在；GraphicsLock1通过continuous阶段实际启用请求测试，GraphicsStartup1/2补齐关卡前保护及一次实际交接。菜单标签修正和锁启用后的其他设置回归仍未完成。

## 实际游戏证据

`manifest-graphics-observer1.json`，PID46520，游戏SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，模块base `0x140000000`。静态对照来自PID16812历史运行时代码节，SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`；启动时三个入口各32字节重新校验，不把相同RVA当作版本证明。

观察版在Reserve阶段只建立未启用trampoline，进入continuous阶段才启用三个入口。每个原函数仍调用一次；collector保留原始RAX，不宣称恢复了完整C++返回类型。只记录固定上限32行/入口；命令192字节、vtable及DLSS字段只在原调用前复制，原调用后不再访问可能已退休的命令。默认宏`GEARS_GRAPHICS_LOCK_OBSERVER=OFF`。

| 入口 | 本次实际调用 | 已确认内容 |
| --- | --- | --- |
| 菜单RVA280da4 | 7次，caller return28047a，TID51592，mode0，EAX1 | 正常菜单Apply经过已知setter；未测试mode3 |
| collector RVA2ac894 | 27374次，首32行caller return1f9a9b，TID51592，TLSdepth1 | singleton=base+1da57f0，normal/alternate均0，原RAX为singleton |
| commit RVAff6f4 | 27374次，首32行caller return19f2f3，TID52244，TLSdepth0 | command vtable=base+1343588、DLSS dword+78=0，原EAX=c0；queueflag=1且selector_thread=52244 |

排队调用点前后32字节已保存，栈回到19f2f3→282b8d→282de3。TID51592/52244仅属于本进程，不能写死到下一版。collector/commit保留样本在前32帧即已满，因此不能称它们直接记录了后续每次Apply的payload。direct调用分支、开启DLSS请求和未知caller仍未实际覆盖。

测试通过正常手柄菜单，把各向异性过滤4x调到8x，应用并确认，再调回4x并确认。首次确认超时后界面仍显示暂存8x，随后重新Apply；这说明暂存标签不能代替生效值。抗锯齿一直为NONE、缩放NATIVE；退出后INI确认MaxAnisotropy=4、DLSS=False。停止后独立只读检查确认normal/alternate=0、ScreenPercentage/FINAL=100、缩放因子1。

会话运行406.420698秒，36422次projection、26848张场景新图，最大提交间隔333.701ms；这是含菜单切换/设置应用的运行，不声明无卡顿。正常停止、XR销毁、38入口恢复及游戏正常关闭。DLL SHA256 `ca8aaa94cafb8e8311ff1563d8836006ca8123ba0db86bb061e116c535cc364c`。首次启动在自测阶段因新构建缺FrameLinkTest等exe中止，尚未启动游戏；补建仅所需测试目标后成功，冻结DLL未变化。

证据：`analysis/graphics-lock/graphics-observer1-46520.audit.json`、`analysis/runtime/continuous-stereo-46520-20261015-021439-a72cff7f.*`、同目录`graphics-observer1-options-*`和`graphics-observer1-after-stop.json`。源码后续把collector TLS恢复改为RAII，并增加嵌套/异常恢复CPU自测；冻结Observer1不包含此收尾改进。

## GraphicsLock1：实际启用请求测试

默认OFF的`GEARS_GRAPHICS_RUNTIME_LOCK`要求observer启用，复用相同三个入口及38入口恢复路径。在启用前校验游戏哈希、原入口、singleton/vtable、dispatcher四段字节、初始normal/alternate为0及normal可写页。queued回调逐次校验当前selector线程、queueflag和精确callsite，direct回调要求已认证collector TLS作用域。仅允许精确字段四字节CAS 1→0，不改变页保护；命令仍由原函数消费，保留原始返回值及192字节步长。故障请求既有安全停止并将effective置0。启用发生在continuous阶段，不能代表启动菜单保护。

PID53840，`manifest-graphics-lock1.json`，DLL SHA256 `611b5c2e6f9c58ed43486519e025918f80dba3710513241ed70187a47b550571`。实际菜单通过状态读取和普通手柄输入，从NONE选到NVIDIA DLSS 3.5、NATIVE (DLAA)，Apply并在确认框接受。3次mode3均被拦截，前后normal/alternate为0；collector/commit各6685次，faults=0，normalCleared/commandCleared均0，因为菜单边界已阻止置1。menu行的`original_rax=1`此时是Mod合成成功值，不是原函数执行结果。

重新进入图形菜单仍显示index3及DLAA标签，因此原测试脚本的“重开应回NONE”断言失败。停止VR后独立读回重新认证原入口字节，normal/alternate仍0，ScreenPercentage/FINAL=100，factor=1；AA quality=4只是质量字段，不能据此认定DLAA启用。这轮证明引擎效果锁关闭，同时暴露菜单暂存标签未刷新。不能仅以菜单显示判断锁是否生效，也不能将标签缺口隐去。

会话95.1983617秒、8566次projection、6320张新图，最大提交间隔73.6923ms，正常停止及38入口还原；XR销毁、游戏正常关闭。关闭前通过菜单恢复NONE并重新打开确认。全部构建目标及22组CPU测试通过，包括生产dispatch的mode3绕过、其他模式原调用一次、collector原始RAX、commit192返回、其他188字节不变和CAS边界；仍未实际覆盖normal/command从1清0或direct分支。

证据：`analysis/runtime/graphics-lock1-menu-aa3.json`、`graphics-lock1-after-stop.json`、`graphics-lock1-menu-restore.json`、`continuous-stereo-53840-20261015-023856-06129087.*`，测试日志`analysis/graphics-lock/graphics-lock1-test.txt`。本次操作未使用截图控制菜单。

## 剩余工作

菜单暂存值需要沿已认证选择/Apply事务与真实刷新接口恢复，不能盲写控件；现有setter成功返回不会自动重置该值。补测锁启用后其他设置仍可应用，以及尚未覆盖的缩放选择。启动取消和一次关卡交接已通过，但实际故障分支与direct分支尚未完整覆盖，尚不作为首版完整锁定。

## GraphicsStartup1/2：启动菜单保护与交接

默认OFF的`GEARS_GRAPHICS_STARTUP_LOCK`要求runtime lock及observer同时开启。启动器还要求owned startup-menu生命周期；普通未预留游戏入口的CPU prime fixture保持原有行为。真实启动在预留并认证三个原生入口后启用图形设置锁，scene Hook仍为0。prime watchdog负责交接前取消，continuous worker接管后继续使用相同三个Hook；原始字节进入38入口恢复记录，不把已经跳转的入口作为原始字节保存。

fault计数和session ownership以顺序一致原子操作交接，确保并发故障要么阻止交接，要么触发continuous停止。启动器在菜单导航及关卡等待时检查对应PID的prime event，防止保护已取消后仍无限等待。取消先禁用Hook，再最多等待10秒让活动回调返回；仍有活动回调时保留资源并拒绝报告清理成功。独立只读审查确认此前交接竞争已封闭，但这不是所有并发交错的实际游戏故障注入测试。

Startup1 DLL SHA256 `5c57fb45a55ad71f63a35a225909a80bd6394057e459662a35b19e4484e451c3`，PID50260，尚未进入关卡。通过菜单反射状态和普通手柄输入选择NVIDIA DLSS 3.5 / Quality并确认；3次mode3请求被阻止，faults=0。取消后disable_status=0、active=0、entries_match=1，独立认证原入口后normal/alternate=0、ScreenPercentage/FINAL=100、factor=1。XR只有26978次启动平面、没有projection或场景复制，因此这不是立体渲染测试。退出前通过菜单恢复NONE/NATIVE并重新打开确认。旧的取消后立即读回曾因异步清理尚未结束而拒绝入口校验；终态后读回成功，不能把早读失败记作恢复失败。

实际缩放列表为index0 Quality、1 Balanced、2 Performance、3 Ultra Performance、4 Native(DLAA)。试验最初把从4向左一次误当Quality，标签检查及时拒绝，随后按真实标签选择0才应用。这说明读取语义值比固定按键次数可靠；重新打开的AA缓存仍为3，也说明菜单暂存值不等于实际生效值。

同一Startup1的PID32568已进入关卡，但在native启动前ABI自测返回33，尚未激活场景Hook。启动保护取消、入口恢复通过。旧自测先发布带时间戳样本再安装Hook，存在安装耗时使样本过期的可能；旧日志没有status/age，未证明此次失败必然由过期造成。Startup2将初始样本发布移到Hook安装完成后，并增加失败时status/age/位置/旋转输出；生产100毫秒有效期和显式过期测试保持不变。

Startup2 DLL SHA256 `963efc35399824bdb4c1bbb6d14893e6d12faa31e16ba52acc818853c77560af`，PID52996。状态导航5次操作请求加载，Pawn/Controller/Camera确认就绪后ABI自测通过、prime handed_off。XR记录2813次启动平面后成功提交带ticket的projection。关卡双目运行167.357912秒、15063次projection、12438张新图，最大提交间隔54.0254ms；collector/commit分别18602/18601次、faults=0。该轮menuBlocked=0，没有重复DLSS选择试验。正常停止、38入口恢复、独立设置读回、XR对象销毁及游戏正常关闭全部通过；不代表真头显验收、无卡顿或字幕显示通过。

构建与验证：Startup1/2全目标构建成功；Startup2普通ABI及continuous CPU测试通过，实际启动又执行既有自测集合。Python prime-owner 4项、launcher 34项通过，覆盖Hook数量/归属、manifest约束、prime event缺失/已停止/查询错误。owned-prime CPU取消/owner退出/交接验证的是6个系统Hook，不能冒称它们直接测试了游戏三个图形入口。

证据：`analysis/runtime/graphics-startup1-50260-prime-status.txt`、`graphics-startup1-after-cancel-complete.json`、`graphics-startup1-32568-prime-status.txt`；`analysis/graphics-lock/graphics-startup2-52996.audit.json`、`analysis/runtime/continuous-stereo-52996-20261015-031630-9d9e33c2.*`、`graphics-startup2-after-stop.json`、`graphics-startup2-normal-close.json`。本轮没有用截图导航；已有启动配置自动保存一次原分辨率GPU图，本次未用它作视觉结论。

缓存修正的只读线索：PID50260活跃AA控件绑定`<WarfareGameResources:Antialiasing>`，BindingIndex=0；解析后的datastore中`SelectedAntialiasingSetting`为IntProperty、offset0x200、当前值3。实际反射发现RefreshSubscriberValue/SaveSubscriberValue NativeFunc分别RVA46a248/b72e8c，元数据及入口双读一致。证据保存在`analysis/graphics-lock/startup1-ui-reflection-50260.json`；调用ABI和正确刷新事务尚未证明，不能仅凭字段偏移直接写入或调用。
