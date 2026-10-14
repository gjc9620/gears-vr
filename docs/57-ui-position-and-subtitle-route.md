# 准星、换枪 UI 和字幕的定位进展

2026-10-14。首版要求是三项均可用；用户明确“歪”指位置偏离瞄准点，图形形状正常。本页记录诊断结果，不表示位置修复已经完成。

## 2026-10-15：整套 UI 盘点与批量修正范围

首版 UI 范围已扩展到菜单/暂停、弹药/主动换弹、交互/教程提示、任务/队友/救援标记、死亡重试、章节检查点和过场文字，仍包括准星、换枪与真实对白字幕。完整交付清单和工程进度见[部署目标](16-deployment-target.md)。

复用现有 `Inspect.exe` 对解压副本批量导出两个脚本包中的UI候选函数：WarfareGame 1705个、156个owner；Engine 1004个、129个owner。2709个候选全部产生文件，导出异常日志为空；其中1093个只是native声明，1010个文件包含未解析的 `__NFUN_*` token，888个含 `@NULL` 类型占位，故不能称为完整原始源码或2709个已理解功能。选择范围由精确正则保存，不覆盖全部游戏包/界面资源，也不以名字证明实际执行。全部衍生代码仅留本地，不加入发布包或Git。

证据：`analysis/ui-v1-script-audit/manifest.json` 保存包与逐文件SHA256、选择范围和限制；`selection-regex.txt`、`summarize.py` 和包导出目录可复核。原生入口审计为 `analysis/ui-position-runtime/ui-coverage-audit.json`。

| 路线 | 当前证据 | 批量修正边界 |
| --- | --- | --- |
| 启动菜单 | WGC窗口画面→XR平面 | 当前只覆盖首次双目接续前；暂停/返回菜单不能自动沿用 |
| Tile/Icon | `1d52b4 → 1bc05c → 19e64c → 172741`，已有真实索引/ticket证据 | 普通屏幕HUD可共用布局平面；不能据PostRender父scope覆盖所有子项 |
| Canvas文字 | `2482b8 → 1bd880 → 19e64c`，已有整段、多atlas范围 | 与父UI语义一致；世界标记中的文字不直接变成固定屏幕文字 |
| 对白字幕 | `2a4370 → 2470ac → 246c58 → 1bd880` | 原生入口已采集，实测尚无真实glyph；仍需对白场景 |
| MaterialTile | `3a4d3c → 3a5548 → 3a514c → 15a420 → 159fcc` | **独立通道**，现有Material包装只有观察，没有Tile/Text receipt覆盖 |
| 任务/队友等世界标记 | `UpdateHUDLocators`依赖相机位置和对象可见性；部分投影入口已定位 | 世界锚点、绘制scope、对应帧和逐眼投影仍须认证 |
| 暂停/设置UIScene | 类型和状态已知 | 实际native绘制入口、目标与合成顺序未认证 |
| 受伤等后处理 | 世界最终输出路线已知 | 不能将整段后处理按平面HUD处理 |

Material新证据：20字节render item、112字节payload、36字节绘制记录，queued consumer为 `15a420`（command vtable RVA136c5d8的+8槽，返回36）。它与Tile/Text的208字节payload、48字节顶点不同，不得复用后者布局解析。`159fcc` 的实际GPU绘制/矩阵仍待跟进，本轮没有启用新的运行时修改。

后续定向反编译已追通 `159fcc` 到GPU：每条记录构造四个32字节顶点，位置为float3；非索引路径DrawInstanced返回 `1af345`，索引路径DrawIndexedInstanced返回 `177c4a`，均可复用现有D3D detour，但必须新增精确Material语义/command/target/ticket关联。首次可接单条纯屏幕HUD记录，原consumer调用一次，逐眼重放实际GPU draw；不能将会销毁payload的 `15a420` 调用两次。初始depth state为不测试/不写深度，透明分支仍保留原材质行为。详细指令和限制见 `material-ui-gpu-route-contract.json` / `material-ui-gpu-route-static.json`。目前只有路径证据，尚未启用Material双眼修正。

`WarHUD.DrawHUD` 已反编译到 `UpdateHUDLocators`、`DrawObjectives`、`DrawHealth`、`DrawActions`、`DrawDownedButNotOutTeammates` 等分支。`UpdateHUDLocators` 读取玩家相机、调用locator更新和可见性更新，说明父级PostRender中的内容并非全部固定屏幕UI。当前kind3父scope继承不足以证明适用共同平面；后续须在批量认证屏幕布局scope时排除或单独处理世界相关分支。既有HUD平面试验不等于这项风险已修复。

下一批实施按公共通道推进：先为已知屏幕HUD、准星、世界标记建立明确分类；复用Tile/Text消费端并补Material消费端；补UIScene显示切换；对同一场景帧做逐眼投影，保留文字排版、裁剪与提示时序。没有已认证的一处Hook能直接修正所有UI。全局索引可复用，原生实现只对上述缺口继续定向反编译。

## 已确认的绘制层次

原 `172741 → 19e753 → 19f2f3` Canvas GPU 路线将同一几何分别绘制到左右半宽 viewport/scissor，不使用两眼视图矩阵或 FOV 重算 UI 坐标。它让 UI 出现在两眼，并未解决准星与实际游戏瞄准方向的对应。

脚本准星使用 Canvas.ClipX/Y 的中心，换枪布局使用 PostRender 设置的 SafeZoneFriendlyCenterX/Y。普通 HUD 与准星应分别处理：准星需要可靠的游戏瞄准目标和对应帧的两眼投影；换枪 UI 需要保留组内布局，选择一致的显示参考面。不能把全部 HUD 都平移到命中点。

保留运行时快照中验证的入口：

| RVA | 已验证用途 | 当前限制 |
| --- | --- | --- |
| `1d6ca0` | 共享脚本 VM 执行入口，RCX=this、RDX=FFrame、R8=result | 必须以当前 UFunction/对象身份筛选语义 |
| `1d52b4` | Canvas.DrawTile 包装入口 | 入口处 Canvas 状态不是完整解码参数 |
| `3a4d3c` | Canvas.DrawMaterialTile 包装入口 | 同上 |
| `2482b8` | Canvas.DrawText 包装入口 | 不涵盖对白字幕的独立原生路线 |
| `cb3f00` | Canvas.Project | 使用 Canvas+`a4` 的单一 SceneView，尚未证明对应哪只眼 |
| `1bc05c` | DrawTile 解码后的提交边界 | x/y 已为 Org+Cur，尚未建立语义到 GPU batch 的关联 |

FFrame 的 Node/Object/Code/Locals 分别在 `18/20/28/30`；Locals 不能当作解码后的 Canvas 原生参数。Canvas.Project 经 `ca71f8/31f6b0` 使用 SceneView+`2b0` 的矩阵并除以 w，再映射到 ClipX/Y。该事实不能替代 Canvas.SceneView 与原视图/眼视图的运行时关联证据。

## UiObserver1：可选的语义观察器

实验清单 `tools/vr-launcher/manifest-ui-observer1.json` 保持默认发布/预览清单不变。新进程启动时预留四个 trampoline，进入关卡后重新反射取得 controller、camera、pawn、HUD、inventory、Canvas class、函数脚本范围和字段身份，验证后才启用观察。七个精确 scope 包括准星、武器额外准星、DrawWeaponSlots 和 WarHUD.PostRender。

TLS scope 支持嵌套恢复；未知脚本保留父级语义，已知但认证失败的子级不冒充父级 HUD。三个 Canvas 包装保持原参数、调用次数和返回行为，不修改位置。每个 kind×draw 桶最多 48 行，前 12 次调用后最多每秒两行。停止、禁用 hooks 且在途调用归零后才导出记录。

`latest_built_ticket_hint` 只是全局最近生成序号，显式标记 `ticket_association_verified=0`；不得用它把 Canvas 调用与 GPU 帧或眼睛关联。scope 调用数不是帧数；采样拒绝数也不是所有绘制调用的失败率。高频 VM scope 身份读取仍有开销，观察器不作为发布默认功能。

## 首次实际运行：PID43652

采用 OpenXR Simulator 子进程 runtime override，未更改全局 runtime。启动菜单与关卡共用 XR PID4712；静止场景投影 230.212654 秒、提交 20722 帧、复制 16458 张新场景图，最大提交间隔 22.4514ms。提交帧包含复用，不等于游戏渲染帧率。

- VM 记录 1473639 次，WarHUD.PostRender 接受 16874 次、拒绝 0。
- 准星与换枪 scope 均未触发；所有 Canvas 桶调用数为 0，没有坐标样本。
- Windows 拒绝正常前台激活，未发送开镜/切枪输入；截图显示两眼场景正常且没有活动 HUD。此轮仅证明观察器接入和静止输出，不能排除其他 UI 路线或证明 UI 修复。
- native/XR 正常停止，32 个入口独立核验全部恢复，游戏正常关闭；模拟器输入不计为真实位置追踪验收。

证据保留于 `analysis/runtime/continuous-stereo-43652-20261014-165604-62446fa0.*`、`ui-observer1-43652.audit.json` 和 `ui-observer1-43652-idle-mcp*`。会话报告的 passed 表示现有双目和收尾检查通过，不表示 UI 功能通过。

本地检查：UI 配置 Python 12 项、launcher/navigation 34 项、continuous runner 31 项；UiObserverTest 和 CaptureSelfTest 的常规、continuous、frame-link、owned-prime-handoff 检查通过。实际启动还执行现有 reservation 的生命周期检查。

## 字幕独立路线

静态证据确认 viewport `2c78f4` 调用 PostRender (`3dc864`) 返回后清空 HUD.Canvas，再调用 GetSubtitleRegion (`3d0f0c`) 和字幕管理器绘制 `2a4370`。当前 cue 经 `2470ac → 246c58 → 1bd880` 进入原生文字批次，不经过当前 PostRender scope 内的 Canvas.DrawText。

GetSubtitleRegion 属于 WarGameViewportClient/Engine.GameViewportClient。SpeakLine 排队、subtitle-enabled 标志及 GetSubtitleSize 测量只能作为前置条件，不能证明有对白 glyph 显示。后续需在真实对白时观察该原生路径，并核对两眼位置；当前四个观察 hook 不宣称覆盖字幕。

最小反编译/指令证据保留在 `analysis/subtitle-route-native/`、`analysis/subtitle-route-static/`、`analysis/ui-position-runtime/canvas-project-submission-static.json`。这些本地衍生文件不进入发布包。

## 已有瞄准目标缓存

`WarPC.UpdateHUDActions` 在符合条件的本地 Pawn PlayerTick 中调用，沿原游戏 CamRot 的方向执行已有 8000 单位 trace，并将 Impact.HitLocation 写入反射字段 LastCrosshairHitLocation（当前版本偏移 `136c`）。这条路径用于准星颜色/标记，不能称为最终随机散布或辅助瞄准后的弹着点。

HUD 缺失、特定 ActiveAction、烟雾遮挡均可能在赋值前返回而保留旧值；无武器分支也不能作为可靠目标。后续若复用缓存，必须证明该次调用经过实际赋值，并与场景 ticket 关联。函数返回、缓存非零或数值发生变化均不足以证明本帧新鲜度。此时不额外调用带随机散布状态的 GetAdjustedAim。

证据：`analysis/ui-position-runtime/crosshair-writer-summary.json`、`crosshair-writer-script.json`、`crosshair-field-43652.json`。

## 开镜、切枪及嵌套调用的实际验证

UiObserver1 / PID44988 已成功各执行3秒开镜和DPad右切枪，两眼截图均看见对应UI。该轮投影73.7911015秒、6644次提交、5196张新图，最大间隔19.0163ms，32入口恢复通过。HUD下记录5607次Tile、1175次Text，分别保留33/32行，无采样拒绝；准星和换枪精确scope仍为0。截图已有UI，因此不能再把0解释成“没有实际绘制”。

定向汇编与运行时函数身份随后确认：`1d6f30` 的嵌套脚本分支直接建立FFrame并执行opcode循环，绕过共享VM `1d6ca0`。它把调用者帧存于 `PreviousFrame+38`。实际Canvas当前Frame.Node包含Canvas.DrawIcon或Weap_AssaultRifle.DrawAdditionalCrosshair，其Code都位于相应脚本内。

UiObserver2 用既有三个Canvas包装入口追溯此链，不增加Hook。最多12帧，只有回到已认证TLS根帧才接受最近的精确语义scope；已知但无效的子scope、读失败、循环和未到根均拒绝。根Code允许正常推进，不要求等于入场Code。记录明确区分root_invocation、matched_frame和matched_depth；仍不建立GPU ticket关联。

PID36892 实测：7894次Canvas链分类，拒绝0；GOW3准星892次、步枪额外准星223次、换枪2884次、其他HUD3895次。准星Tile保留18行；换枪Tile/Text各18行；其他HUD Tile/Text各35行，采样拒绝0。已验证DrawIcon到GOW3祖先的depth=1及正确scope归属。长时间等待窗口焦点后实际完成开镜和切枪；两眼截图保存，整轮投影685.649956秒、61693次提交、47553张新图，最大间隔168.9184ms；此数值不是全程无卡顿承诺。正常停止及32入口恢复通过。**坐标仍未修改，不能把分类成功当作UI修复完成。**

证据分别位于 `analysis/runtime/continuous-stereo-44988-20261014-184007-ea0a225a.*`、`continuous-stereo-36892-20261014-185144-e8c30c45.*` 及相应 `ui-observer*-mcp*`；嵌套分发证据为 `analysis/ui-position-runtime/nested-script-dispatch-44988.json`、`frame-nodes-44988.json`。新实验清单为 `tools/vr-launcher/manifest-ui-observer2.json`。

## 同屏幕坐标的双眼投影错误

PID44988 的348个CSV采样记录恒定不对称FOV和两眼单位朝向。截图准星均约处于各眼图像中心。按该轮参数反投影 `(u,v)=(.5,.5)`，左右射线水平向外约15.037°，三维夹角29.44567°，不存在前方共同目标。共同头部正前方向对应左眼 `(.62126,.40975)`、右眼 `(.37874,.40975)`。这些值仅是本轮模拟器数据，不能写死为真实头显修正参数。

计算和像素检测方法、源文件hash、阈值与局限见 `analysis/ui-position-runtime/ui-fov-audit-44988.json` 及 `audit_ui_fov.py`。不依赖latest ticket hint，也没有据此认定瞄准缓存的新鲜度或真实弹着点。

`tools/vr-common/ui_projection.h` 已实现纯数学的 `project_ui_anchor`：将同一个调用方认证的世界点，用冻结的双眼矩阵投到各眼像素坐标。CPU检查覆盖上述FOV反例、不同深度视差、旋转平移、视口外点及非法输入。它尚未改变游戏绘制；目标来源、HUD平面深度和批次/ticket归属由后续接入负责。

## UiObserver3：语义顶点到实际绘制的候选关联

在既有 DrawTile 包装调用前后读取 UCanvas→FCanvas→当前排序元素→BatchedElements 的链，只有确认新增恰好4个顶点时记录区间及字节副本。真实顶点步长为48字节，其中44字节由Tile生产代码赋值，尾4字节不参与内容比较。排序元素按 key 查找；顶点数组重分配后只读取新地址。同 payload 必须仍属于同 item，读取失败、裁剪后未新增、布局变化均拒绝。它不增加 Tile 绘制次数，也不改顶点。

新增唯一 consumer 入口 `19e64c`，经实时字节与命令 vtable 验证，ABI 为 `uint32_t Execute(void* command)`。该函数在内部绘制后销毁 payload；观察器在原调用前取出固定容量候选，逐区间比较当前顶点内容，并用嵌套 TLS 将匹配结果带到既有 `172741` GPU 回调。记录的 ticket 来自该次实际 `frame_link.overlay_pair`，不是最近生成的 ticket 提示。原函数仅调用一次并原样返回，返回后不读取已释放 payload。

候选容器最多16组、每组32区间。重复/冲突/组内溢出保留无效标记直到消费者到达；无法记录新组时整容器停止接受，不静默淘汰。记录包含未知顶点数量和语义混合掩码，不能把缺少区间解释为整批次属于某类 UI。候选 generation 不是分配器身份；字节一致也不证明每次 indexed draw 使用这些区间。当前仍为只读诊断，不能直接据此授权改写顶点。

新增入口使恢复检查上限为33。UiCanvasBatchTest、UiPayloadReceiptsTest、UiObserverTest、UiProjectionTest 以及 continuous ABI 自测通过；默认预览清单未升级。

UiObserver3 / PID9232 在实际各3秒开镜/切枪时记录5742个追加区间、1668个消费者内容匹配，拒绝/溢出0。但新增 hook 在调用栈中插入了一个自有函数帧，旧的三帧相邻检查拒绝了所有 overlay，GPU关联0，最大提交间隔7178.84ms。该轮不能算UI通过；常规会话报告 passed 只覆盖现有收尾条件，33入口确实恢复。证据 `analysis/runtime/continuous-stereo-9232-20261014-191940-51e21904.*` 保留了实际四帧栈。

UiObserver4 对观察模式增加精确四帧认证：`172741 → 19e753 → hook_ui_execute 的 PE unwind 函数范围 → 19f2f3`，并要求同线程 Execute 入场保存的真实返回地址也是 `19f2f3`。原有 target/ticket/viewport 条件继续生效。路由身份与语义候选分开，因此未知图元和文字不会因缺少候选而失去原来的双眼复制。CPU测试覆盖错误函数范围、入口返回地址及顺序；清单为 `tools/vr-launcher/manifest-ui-observer4.json`。

UiObserver4 / PID26980 未完成该路由的实测验证：Win+0实际切回游戏后，模拟器XR状态变成VISIBLE；桥接记录失去focused，native记录6682次inactive，仅构建198个ticket。开镜截图全黑，GPU关联仍为0。该轮应按显示失败保存，不能以退出正常宣称通过。

后续对上传/分配器的交叉核对还纠正了 UiObserver3/4 的44字节步长误判：`333de8` 使用48字节分配，`1720a0` 按48字节偏移/上传，并设置IA stride=48。旧两轮的生产/消费比较都使用了相同错误步长，因此“字节匹配”不能证明实际顶点内容。UiObserver5改为按48字节步进、逐个比较前44字节，并增加带独立padding的测试；旧实验数据不用于坐标修正。

### UiObserver5 实测：PID45216

使用冻结的新构建和模拟器自动测试焦点选项，各执行3秒开镜与切枪，两眼截图均正常。连续投影139.801223秒，提交12585帧、复制10463张新图，最大提交间隔18.103ms；native inactive拒绝0。正常停止、XR退出及33个入口恢复通过，游戏随后正常关闭。

- 5840次Tile追加观察全部取得四顶点区间，前后读取/追加/发布拒绝0。
- 1693个消费候选全部内容匹配，header/内容/返回值拒绝0；容器无溢出、无遗留组。
- 2916组overlay实际记录并提交，拒绝0；语义候选关联到实际ticket：准星464次、换枪699次、其他HUD527次。其余1226次没有Tile候选，不能当作该类UI或据此移动。
- 保留的样本均为单一语义的完整顶点覆盖；该场景没有观测到混合候选，不证明所有武器/菜单都不会混合。单次indexed draw的索引成员及分配器身份尚不由本观察器证明。

证据：`analysis/runtime/continuous-stereo-45216-20261014-193621-c0dcdcff.*`、`ui-observer5-45216.audit.json`、同前缀ADS/weapon截图。实验清单为 `tools/vr-launcher/manifest-ui-observer5.json`。**实际UI坐标仍未修改。**

模拟器原版在预览失焦时发送VISIBLE，现有Mod的focused门限会停止构建新眼图。新增本地测试选项 `XR_SIMULATOR_FORCE_FOCUS=1` 仅改变模拟器VISIBLE/FOCUSED策略，默认关闭；实际OS焦点日志和鼠标释放保留，退出状态不被唤醒。补丁与依赖/构建hash记录在 `tools/openxr-probe/simulator-focus/`。原部署DLL和全局runtime未替换，只给该次XR子进程指定独立manifest。CPU焦点策略82项、CTest3项和5秒446帧预检通过；本轮实际日志未出现OS失焦事件，切换到预览的单次后台请求被Windows拒绝，所以仍不能声称该失焦分支已在实机事件中覆盖。

## 准星目标的最小可实现边界

静态解码确认 `UpdateHUDActions` 的命中缓存赋值占 Script内存位置 `+359..+37f`；包内存储偏移不同，不能直接用于进程。当前 Canvas 链不包含已经退出的 writer 帧，共享VM入口又可能被嵌套脚本绕过，因此现有样本不能证明该tick执行了赋值。若使用有限深度命中点，需认证实际赋值及owner、tick、scene ticket，不以旧缓存非零或变化替代。

较小的方向方案是将同 ticket 的原游戏视图中心射线分别投影到双眼，作为“原游戏视图方向准星”。这能避免不对称FOV下硬放两眼图像中心导致的方向分离，但不提供近处收敛、枪口偏移或辅助瞄准后的真实弹着点；原视图方向与实际瞄准仍须核对。普通HUD应独立使用共同布局平面，不跟随命中缓存。

## UiObserver6/7：索引、冻结矩阵与整段文字

`ui_index_membership.h` 在 Execute 入场保存当前36字节segment和uint32索引副本。成员由 `minimum + index` 判定，不用上传区间的交集替代。最多64个segment、8192个索引；零draw也按原生循环推进，核对完整五个DrawIndexed参数，原调用结束后确认数量。原生1720a0的静态循环已核实每segment按序产生一个172741回调；相同参数本身不是可区分的身份，不搜索相同数量的后续segment。大数组使用有界heap，失败只丢弃观察。所有完成检查只读自己的副本，不读原调用已经析构的payload。

场景构建时保存原游戏view/projection和实际双眼矩阵，随后在同一Draw/Present顺序锁内随ticket原子保留、退休。UI只查询实际overlay_pair的ticket，不取latest pose；缺失/冲突不会升级为成功。`ui_plane_projection.h` 的纯数学测试涵盖原相机姿态、非对称FOV、深度视差与转头后的projective mapping。一般情况下viewport缩放平移不能精确表达整个平面；这个helper尚未修改GPU。

UiObserver6 / PID14964取得1736个完整索引序列，但用户随后说明自己用FancyZones调整了窗口，缓冲从3840×2160变为3842×2005且换了资源。固定尺寸检查因此拒绝，overlay和UI矩阵样本均为0，最长提交间隔185455.2ms。常规收尾passed不代表显示通过。焦点一直有效、33入口恢复且正常关闭。最初把尺寸变化归因于Win+0不准确，用户说明为此次根因证据；报告保留为负实验 `analysis/runtime/ui-observer6-14964.audit.json`。

UiObserver7为文字新增整段range receipt，保留原quad适配。每payload最多32段、1024顶点，使用独立有界heap副本；长文字不会按32个glyph截断。现有DrawText包装函数围绕一次原调用捕获全部item的新增范围，支持多字体纹理和重分配；嵌套Tile/Text不重复记录。字幕在精确入口2a4370、caller return2c9375单独收集direct FCanvas，原RAX、rectangle与排版不变。多atlas调用在共同锁内全部发布，否则整次丢弃对应payload候选；失败不改变原文字。

PID37184在稳定尺寸下完成3秒开镜和3秒切枪，双眼截图均有场景、图标和换枪文字。实际结果：

- 持续206.034605秒，18546次投影提交、14081张场景新图，最大提交间隔17.8515ms；本轮无尺寸变化、inactive拒绝0。
- 5301个Tile追加区间，以及1112段文字、13344个文字顶点，生产/读取/发布拒绝0，容器无溢出或遗留。
- 2650个消费者全部内容匹配，2650个索引序列完成，2650组overlay实际执行，均无拒绝；gpu_missing_context=0。保留的UI行全部关联到对应ticket的冻结几何。
- 字幕入口14497次均得到空delta；该场景没有字幕glyph，不能以入口命中声明字幕显示已验证。
- 正常停止、XR收尾与34个入口恢复通过，游戏正常关闭。12组相关CPU测试及continuous ABI自测通过。

清单为 `tools/vr-launcher/manifest-ui-observer7.json`，DLL SHA256 `58438c20230469061974701ee97266a74a6c364c6bd3c69e4235734df13121c0`；证据前缀 `analysis/runtime/continuous-stereo-37184-20261014-213300-8d5d594c`，摘要 `ui-observer7-37184.audit.json`。代码检查点341a800（索引/矩阵）、6aaff64（整段文字/字幕）。本轮仍未修改坐标，也不是物理头显测试；一个场景没有unknown/mixed不证明全游戏均如此。

## 下一步

实际GPU回调发生在顶点上传之后。完整位置修正需要每眼独立VB副本或已认证的GPU变换；不能在回调中改原CPU数组并期待已上传顶点变化。顶点stride48，位置float4在0..0f，UV/颜色/hitproxy与padding需保留；每眼VB需跟踪并恢复原IA绑定，GPU fence完成前不能释放。普通HUD与整组文字使用共同平面；准星需关联原游戏实际目标/赋值时序，不用过期命中缓存。

另需在有对白的场景验证真实字幕glyph和双眼位置。窗口resize恢复不能只清掉拒绝标志：绑定、共享纹理、接收端图像与frame ticket都需要新的资源代次及有序交接。现有固定尺寸版本会停止新图，当前不宣称具备缩放恢复能力。定向计划见 `analysis/resize-recovery/architecture-plan.json`。DLSS/DLAA的三个引擎拦截点与菜单Apply返回规则已核对，仍未启用运行时锁定，不能把启动INI关闭当作完整锁。

## HUD 平面控制首次运行（2026-10-14）

e5ede89 / UiHudPlane1 / PID46800 首次启用独立 70°、2 米 head-locked HUD 平面。模拟器运行 791.65065 秒，71243 次 projection、54971 次复制，最大提交间隔 68.5128ms；2984 次计划和实际双眼 overlay，sequence_fault=0，34 个入口恢复且游戏正常关闭。1492 段文字被覆盖；原生字幕 55393 次均无 glyph，准星没有样本。

两张操作期间截图均能看见双眼场景，但没有清楚可见的换枪 UI/文字；不能仅凭 applied 计数宣布视觉修正成功。已记录 0.1 米水平/0.08 米前后平移及 yaw/pitch/roll 非零的模拟器姿态，场景随之变化；HUD 可见性、平面位置及实际字幕仍需定向调查。证据 `analysis/runtime/ui-hud-plane1-46800.audit.json`，不计为真头显验收。

截图限制已查明：MCP D3D12截图读取模拟器窗口预览RT（本轮1734×257），随后压到1280×189；Fill Window模式将完整眼图压扁进预览，并非原始1920×2160眼图读取。截图不足以证明HUD消失；不能将这种预览变形归因于游戏投影。单眼MCP也只裁切预览，不能恢复丢失像素。后续需尺寸正常的预览或原始GPU读回才能判断小字体位置。

准星缓存赋值的纯辅助模块已加入 `crosshair_assignment.h`，验证 execLet 的精确字节码、属性/owner 链、原调用后完成的 Vector 复制，包括同值赋值。独立 CPU 测试通过；尚未安装赋值 hook，也没有与玩家 tick/实际场景 ticket 关联，不能据此声称瞄准点已经修复。

## 原始 GPU 像素与 Material 路由（2026-10-15）

`manifest-ui-material1.json` / PID51800 增加默认关闭的 Material UI 消费观察器（第35个入口）。只检查有界追加、不可变payload副本和实际消费回调，不修改材质顶点或GPU状态。本轮已发送开镜、切枪，6574次projection、5036张新图，正常停止及35入口恢复通过；Tile/Text路径有准星、换枪、HUD样本，Material计数全部为0。因此当前样本应沿Tile/Text处理，不能宣称整个游戏不使用Material，或已完成该分支的运行时验证。

原始GPU快照现在保留完整3840×2160 SBS像素，可在明确开启诊断的会话内按需请求，每次会话最多8张。PID22904 / Pixels2 首次在操作期间成功保存独立文件，清楚看见开镜准星、弹药HUD及换枪图标/数字。换枪与弹药两个区域的亮色像素匹配均在右眼局部偏移(-493,0)附近最佳，与本次HUD平面预测(-493.356,0)一致；这只是当前场景的像素证据，不代表所有UI/真头显体验已通过。图像与量化记录：`analysis/runtime/ui-pixels2-ui-alignment.json`、同轮`.gpu.ppm.1.ppm`。准星仍处于每眼图像中心，未应用非对称FOV下原游戏方向的投影修正。

Pixels2 的快照日志在acknowledge之前读取serial，记录的是上一次已确认图像；图像及ticket有效，serial不能用于同帧关联。Pixels3修正为ack成功、GPU fence完成后读取，并增加保存完成的单调时钟时间。PID424 / `manifest-ui-pixels3.json` 实际采到3张完整图，均与image-ticket日志的ticket及serial匹配；其中两张保存时间落在开镜、切枪动作期间。该轮5667次projection、4137张新图，正常停止及35入口恢复通过。首张Pixels1原实现是在保存时读取serial，未受Pixels2这一回归影响。三轮审核文件为各session前缀加`.ui-pixels-audit.json`。

下一项先校正纯准星segment的原游戏屏幕中心方向，使用同ticket冻结矩阵，以无限远方向投射到每眼后平移viewport；保持原图形尺寸。该方案不等同于近处命中点、枪口偏移或真实弹着点修复。字幕仍需有对白的实际glyph样本，普通PostRender语义也不能自动推广到所有世界标记。

## UiReticle1：原游戏方向修正已通过像素核对

`manifest-ui-reticle1.json` / PID47092 首次开启默认OFF的 `GEARS_UI_RETICLE_DIRECTION`。仅当当前原生segment索引全部属于同一次Tile准星调用、独立顶点副本位置有限且z=0/w=1、包围盒居中误差不超过1像素且宽高各小于画布1/3时启用；额外准星/整屏瞄准镜/混合/未知批次不自动接管。Extent在原consumer调用前按实际索引保存为值，之后不读取已销毁payload。使用实际overlay ticket的source view/projection及双眼矩阵，以w=0映射屏幕中心方向，只平移viewport，裁剪scissor并恢复原状态。

实际458次准星计划及双眼draw通过，居中/矩阵/投影拒绝均0，其他HUD 2134次平面修正，sequence_fault=0。10775次projection、8012张新图，正常停止、35入口恢复及游戏正常关闭。原图开镜ticket650/serial643与XR记录吻合；阈值提取的准星白色像素框中心由基线(959.5,1079.5)/(2879.5,1079.5)，移动到(1192.5,884.5)/(2646.5,884.5)，与计算位置(1192.812,885.062)/(2647.188,885.062)在像素栅格范围内一致。四个白色像素框均为60×104像素，原图同时能直接看见正常准星形状。

CPU测试以 `/W4 /WX /UNDEBUG` 编译，覆盖非对称FOV、旋转、平移不影响方向、索引归属、混合来源/多个root拒绝、失败不修改输出；HUD与索引回归、continuous ABI自测通过。代码已只读审查。DLL SHA256 `185161b70a9fa00ce755644f014382fef269d46ab00e558897d7053eafdd4ef4`，证据 `analysis/runtime/ui-reticle1-47092.audit.json` 和原始GPU图。这证实当前场景FOV方向偏移的修正，不证明近处命中收敛、枪口弹着点、所有武器/菜单/字幕或真实头显体验。

## 首次非空字幕生产记录（2026-10-15）

PID53760正常推进开始房间、走廊及庭院入口，运行727.19秒、65225次projection、45428张新图，正常停止与35入口恢复。字幕统计桶source=1（receipt枚举为NativeSubtitle=2）首次取得559个非空调用、1845段、110312顶点；生产没有delta或publish拒绝。这里是精确2a4370入口内的文字追加证据，不是文字像素已显示。

完整3840×2160的6张快照未截到对白。晚期只读队列观察器在源getter/selector字节与历史快照匹配后，通过getter的RIP相对寻址取得global RVA1c7dbc8；稳定的manager header为count=0/capacity=0。该观察比进入走廊晚约181秒，不能由此否定此前非空glyph。随后PID46520在移动前开启60秒watch，仍采到空队列；该轮停止日志却有65个非空字幕调用、129段、2820顶点，说明当前队列时序与截图覆盖仍不足。实际画面还显示玩家在庭院门边进入掩体，不能把发送前进键当作已经通过该位置。

当时GPU receipt上下文仅保留HUD kind，丢弃了原NativeSubtitle source，不能拿任意kind4行当作字幕ticket。证据：`analysis/runtime/continuous-stereo-53760-20261015-015852-f4f5fcfa.*`、`continuous-stereo-46520-20261015-021439-a72cff7f.*`、`subtitle1-queue-walk.jsonl`、`subtitle2-queue-walk.jsonl`。

GraphicsLock1已增加`ui_segment_sources.h`，按真实复制索引保留source/root范围，拒绝混合、未知、过期代次及索引不匹配；独立16条字幕GPU记录保留ticket、list generation、execute sequence、QPC和双眼replay/命令完成状态。`UiSegmentSourcesTest`以/W4 /WX编译并通过，未增加原生hook或修改顶点。PID53840本轮菜单测试字幕GPU calls/rows均0，尚未取得真实字幕GPU命中，不能据此宣称可见性通过。

现有快照event没有指定ticket，GPU done fence仅为复用ack。后续精准采图应将已提交的字幕list与Present发布的相同ticket关联，接收端只保存指定ticket/session；越过则明确miss，不能自动用下一帧冒充。最后仍需检查文字像素与双眼位置。
