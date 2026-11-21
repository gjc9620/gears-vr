# Actual UI layout sampling: scene root versus widget bounds

2026-10-17. Actual125 exposed a defect in the optional CPU layout reader: it requested `RenderBounds` on the root UIScene, before visiting any controls. The actual reflection lookup rejected that field. Existing KB artifact3 identifies `Engine.UIObject.RenderBounds`; the saved UIScene declaration traverses its Children for bounds. UIScene and UIObject are distinct UIScreenObject branches.

`MenuReader.sample(include_layout=True)` now authenticates the current object, registered Class metaclass, bounded complete superclass chain and Engine-owned UIScene/UIObject base using the existing snapshot read ledger. An authenticated scene root explicitly has `render_bounds=null` with the absence reason and ancestry proof. Every descendant must authenticate as a UIObject and retains the original strict reflected field/layout/value checks. Unknown roots, invalid or changing ancestry and missing/invalid widget bounds still reject. The default sample and ordinary tree option do not acquire this extra ancestry evidence.

The synthetic fixture now models distinct scene/widget branches rather than inventing root bounds. The 64 menu-state tests pass, including class/owner/registration/cycle/churn rejection and preserved numeric/layout failures. Menu-state, menu-control and menu-display-monitor suites together passed 143 tests. Actual125 subsequently captured a consistent ten-node `PenderCrossplayInvitePopUp` subtree: one scene root with explicitly absent bounds and nine descendants with actual reflected bounds. This confirms the sampler correction; it does not establish PathChoice placement, GPU clipping or headset acceptance.

Actual125 used the same Native67 DLL during reservation, but the XR preflight failed before native activation because this test supplied the simulator's original manifest with a bare relative DLL name. The existing ignored absolute-path manifest remains available. The owned launcher exited with code1; its prime cancellation and entry restoration were verified. The same identity-checked game remained alive for normal menu inspection. No retry/restart was inferred merely from a timeout, and the global OpenXR runtime was unchanged.

The game itself supplied eight Act1 chapter rows. The normal menu selected `FORK IN THE ROAD`, `sp_embry_p`, `LoadChapter=3`, after verifying the current provider values and the four protected profile backups. This establishes a reproducible chapter selection, not that its initial spawn immediately exposes PathChoice or a usable turret. A temporary in-game secondary forward binding permitted measured background movement; a bounded probe stopped on collision without triggering PathChoice. Gameplay input stayed scoped to the owned game HWND, with physical background controller response disabled.

Evidence is ignored under `analysis/runtime/vrdof125-*`, including the failed layout log, corrected root layout JSON, chapter provider/selection receipts, entry-field query, menu checks and forward probe. The ongoing test's original profiles/settings are protected by `analysis/savebackups/vrdof125-before-chapter/manifest.json` and `vrdof125-settings-backup.json`; temporary bindings and test progress must be restored only after the owned game is closed. No scene transform, default DLL or release ZIP changed in this commit. V1 remains incomplete.

---

## 中文

# 实际的 UI 布局采样: 场景根对部件边框

2026-10-17 实际125 暴露了可选 CPU 排版阅读器中的缺陷: 它请求 `RenderBounds` 。在访问任何控件之前,请先查看根 UIScene 。实际反射检查拒绝了该字段。3 identifies `Engine.UIObject.RenderBounds`; 保存的 UIScene 声明会穿越其子孙的界限。 UIScene 和 UI Object 是不同的 UIScreen Object 分支 。

`MenuReader.sample(include_layout=True)` 现在使用现有的快照读分类账验证当前对象、 注册的类元类、 被绑定的完整超类链条和引擎拥有的 UIScene/ UIObject 基础。 认证的场景根显然有 `render_bounds=null` 。每个后代必须认证为 UI 对象,并保留原已严格反映的字段/显示/值检查。未知的根、无效或正在更改的祖先和缺失/无效部件的界限仍然拒绝。d efault样本和普通树种选项不能获取这种额外的祖先证据.

合成固定器现在模拟了不同的场景/部件分支,而不是发明根边框。 64 菜单状态测试合格,包括类/所有者/注册/周期/churn拒绝和保存的数值/显示失败。菜单状态、菜单控制和菜单发布-监视套件一起通过 143 实际125 之后,我们抓住了一个十号线 `PenderCrossplayInvitePopUp` 子树: 一个明显没有界限的场景根, 以及九个实际反射界限的后代。 这证实了采样器的校正; 它没有建立 PathChoice 位置、 GPU 剪切或耳机接受度 。

Actual125 使用相同的原生语言67 保留期间的 DLL, 但是在本地激活前 XR 飞行前失败了, 因为测试提供了模拟器的原始显示, 上面有一个赤裸的相对 DLL 名称。 现有的被忽略的绝对路径显示仍然可用 。 拥有的发射装置退出 w 其代码1; 其质值取消和录入恢复已被验证。 同一身份检查的游戏在正常菜单检查时仍然有效。 没有仅仅从超时推断出重试/重开, 全球 OpenXR 运行时间没有变化 。

游戏本身提供了八幕1 章节行。所选的普通菜单 `FORK IN THE ROAD`, `sp_embry_p`, `LoadChapter=3`,在验证当前提供者值和四个受保护的配置文件备份后。这确定了可复制的章节选择,而不是说其初始产物立即暴露PathChoice或可用的炮塔。一个临时的游戏中二级前置装订 允许测量背景运动; 边框探测器在碰撞时停止, 没有触发 PathChoice 。 Gameplay 输入仍被限制在自有游戏 HWND 的范围内, 物理背景控制器响应被禁用 。

证据在下文中被忽略 `analysis/runtime/vrdof125-*`,包括失败的布局日志、更正的根版式 JSON、章节提供者/选择的收据、条目-域查询、菜单检查和前置探针。正在进行的测试的原始配置/设置由 `analysis/savebackups/vrdof125-before-chapter/manifest.json` and `vrdof125-settings-backup.json`; 临时绑定和测试进度必须在自有游戏关闭后恢复。 在此承诺中, 没有改变场景转换、 默认 DLL 或释放 ZIP 。 V1 仍为不完整。
