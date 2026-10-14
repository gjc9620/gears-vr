# 准星、换枪 UI 和字幕的定位进展

2026-10-14。首版要求是三项均可用；用户明确“歪”指位置偏离瞄准点，图形形状正常。本页记录诊断结果，不表示位置修复已经完成。

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

## 准星目标的最小可实现边界

静态解码确认 `UpdateHUDActions` 的命中缓存赋值占 Script内存位置 `+359..+37f`；包内存储偏移不同，不能直接用于进程。当前 Canvas 链不包含已经退出的 writer 帧，共享VM入口又可能被嵌套脚本绕过，因此现有样本不能证明该tick执行了赋值。若使用有限深度命中点，需认证实际赋值及owner、tick、scene ticket，不以旧缓存非零或变化替代。

较小的方向方案是将同 ticket 的原游戏视图中心射线分别投影到双眼，作为“原游戏视图方向准星”。这能避免不对称FOV下硬放两眼图像中心导致的方向分离，但不提供近处收敛、枪口偏移或辅助瞄准后的真实弹着点；原视图方向与实际瞄准仍须核对。普通HUD应独立使用共同布局平面，不跟随命中缓存。

## 下一步

已取得开镜和切枪语义样本。下一步验证游戏现有瞄准命中点的更新条件，以及语义顶点区间到渲染 batch、ticket 的归属；若一个 batch 混合准星和其他 HUD，不能统一改该 batch。实现按对应帧两眼投影的准星位置与普通 HUD 布局后，以同一场景做视觉对照。字幕另选有对白的场景验证。
