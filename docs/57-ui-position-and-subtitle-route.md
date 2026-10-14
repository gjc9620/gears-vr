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

## 下一步

在正常焦点下触发开镜与切枪，取得实际 scope 和 Canvas 样本。再验证游戏现有瞄准命中点的更新条件，以及语义绘制到渲染 batch、ticket 的归属；若一个 batch 混合准星和其他 HUD，不能统一改该 batch。实现按对应帧两眼投影的准星位置与普通 HUD 布局后，以同一场景做视觉对照。字幕另选有对白的场景验证。
