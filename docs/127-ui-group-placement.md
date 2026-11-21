# Shared UI group placement preparation

This is offline preparation, not an enabled PathChoice UI fix or headset acceptance.

`ui_group_placement.h` applies one source-normalized uniform scale and translation to a complete admitted group. Each eye retains its independent panel viewport. The saved source scissor follows the same affine transform and intersects that eye's bounds. Empty or invalid results reject atomically; the helper cannot recover geometry already culled by the game.

The existing HUD layout delegates its established affine calculation to this helper. An actual identity HUD policy preserves the panel unchanged. A nonidentity HUD policy whose resulting affine happens to be identity still recomputes the saved source clip, preserving the previous behavior.

## Verification

Both standalone CPU executables passed on 2026-10-17:

- `UiGroupPlacementTest`: common movement of arbitrary group members, asymmetric eyes, independent expected pixel clips, scaling, partial/outside clipping, numeric limits and unchanged output on rejection.
- `UiHudLayoutTest`: existing HUD behavior and the centered-bounds identity-affine regression case.

The HUD test uses a 16 MiB executable stack for its existing large CPU fixtures; the CMake link option affects this test only. Private executables are under ignored `analysis/ui-group-placement-cpu/`.

## Remaining integration

The native scene route and matrix evidence are recorded in docs 122 and 124. Production use still requires an admitted root scene, one immutable group receipt shared by text/tile/material payloads, complete collection across relevant sort keys, and verification against actual GPU viewport/scissor state. Ordinary per-entry HUD layout must not apply a second transform to that group.

No root hook, PathChoice translation, new DLL, game launch, simulator launch or runtime test is included in this change. Mounted-camera sampling changes in `d803216` remain separately unverified in an actual mounted session. The latest development ZIP predates that commit and this helper.

## 2026-10-18: Shared fit and Native105 buffered HUD baseline

The shared helper now computes one normalized fit from a complete logical group to an explicit target rectangle. It preserves the requested uniform size when it fits and reduces uniformly otherwise; it has no page-specific offset or default target. HUD bounds use the shared type without changing existing player layout. This is preparation, not an enabled general UI correction.

The root coordinator built private UiGroup105, passed22 startup self-checks, and ran a background baseline. The game had an8×8 background output and the VR scene submitted/copies remained0, so there is no visual acceptance. The logical observer's separate CPU counter published175 samples and retained the latest8 authenticated PostRender groups; it must not be confused with VR publication. All8 retained groups have before/return/root/delta/bounds checks passing,20–22 Canvas calls,12–13 simple ranges and3–4 material ranges, with no call overflow, unclassified Canvas calls or synchronous simple/material Execute. There are no PathChoice root samples. The coordinator stopped normally and verified Hook restoration, game exit and settings/profile restoration.

Runtime frame/name evidence includes WarHUD.PostRender, DrawHUD, DrawHealth, UpdateWeaponIndicator, DrawWeaponInfo, DrawWeaponIcon, DrawEmptyAmmo, DrawAmmo and teammate/locator routines. At the actually observed DrawWeaponInfo UFunction node0x37e51de0, the decoder finds copied geometry candidates in7 of8 retained groups; row2 rejects an incomplete selected-call member proof. Filtering by a UFunction ancestor alone can combine repeated invocations, so these are diagnostic unions, not proven single logical prompts. DrawActionInfo, the Hammer pickup event and Choose Path were not covered. The result does not establish the reported pickup route, GPU clipping, input hitboxes or headset layout.

Evidence remains ignored: `analysis/runtime/continuous-stereo-23004-20261018-180334-03ce8195.native-status.txt`, `uigroup105-logical-group-bounded-report.json`, `uigroup105-weaponinfo-ancestor-report.json` and `uigroup105-logical-group-bounded-proof.json`. The proof saves the source status SHA256. The10 relevant UI files matched the frozen `analysis/native-source-uigroup105/` copy before the next extension; per-file SHA256 pairs are saved in `uigroup105-ui-source-comparison.json`. Subsequent unique-token source differences are explicitly saved in `uigroup105-ui-source-after-unique-token.json`, not attributed to the105 run.

The next observation-only extension uses the already existing, byte-checked1d6f30 script-call boundary; it installs no additional Hook and changes no configured semantic scope. Under a sampled authenticated HUD parent, live Function/WarHUD identity, script range, original VM entry and caller ancestry must pass before a unique child token is assigned. Canvas calls retain a token only when their actual body frame matches that callee/self/caller and code range. Normal return revalidates the function identity and script/entry. Repeated use of the same UFunction and stack address receives distinct tokens. Original arguments, return bits, exceptions and exactly-one original call are preserved. The decoder's `--invocation` checks the token, normal completion, body-call count, ancestry and typed member ranges before emitting a copied group candidate; it still never authorizes replay. This extension has offline checks, not a runtime claim from105. Default player manifest and private PathChoice positioning policy remain unchanged.

## 2026-10-18: Pickup applicability boundary and one-invocation fit candidate

Bounded KB lookup and seven existing script exports establish a static pickup data-flow candidate. `WarPickupFactory.GetPickupAction` explicitly tests `WEAP_HammerOfDawn` and fills action icons and item-name tooltip; `WarDroppedPickup.GetPickupAction` also has an explicit Hammer branch. `WarPC.UpdateHUDActions` passes successful pickup actions to `MyWarHud.SetActionInfo(13, Action)`. That setter writes `ActiveAction`; `DrawActions` can draw both `ActiveAction` and fading `PreviousAction` through separate `DrawActionInfo` calls in one frame. The latter emits background text and Canvas tiles. Thus neither a function-name filter nor a union of every `DrawActionInfo` ancestor identifies one prompt. These exports contain unresolved native tokens/types; this is static data flow, not proof of the user's actual Hammer instance or of every branch condition.

Source identity is KB artifact2, `WarfareGame.u`, SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`, module base0 for the package. `DrawActionInfo` export SHA256 is `1a1fb2e41b64012bd834815609812e2a1a0e7355edc527998af00a939a8f1085`; factory pickup export SHA256 is `5031b1a09fec3a0568f84a7925a59e7215b0ea1984b26414886c5657e7e4c36e`. Full per-export object identities, hashes, unresolved markers and bounded line excerpts are saved in ignored `analysis/ui-group-placement-cpu/layout-applicability-20261018.json`, along with the unchanged105 negative coverage proof. No new native analysis, IDA or game execution occurred.

The shared implementation boundary is complete normalized content bounds plus `gears_ui_group::fit`, not a common proven scene parent. PathChoice has its separately authenticated13-node tree and10 direct SafePanel content children, native layout invalidation and restoration. The pickup candidate uses Canvas text/tile groups; no SafePanel route has been established for it. A UIScene root has no UIObject `RenderBounds` (docs148), so a scene rectangle or a whole PostRender rectangle must not substitute for content bounds.

`ui_scene_logical_layout.h::fit_invocation` now produces a CPU candidate from the existing authenticated HUD observation's unique child ID. It checks normal root/child return and identity proofs, body ancestry/counts, complete typed member mapping across sort keys, interval alignment and absence of consumer flush/overflow. Exact duplicate references are deduplicated; partial overlap or shared geometry from another invocation rejects. It copies the selected complete text-atlas/material geometry, unions its original-matrix bounds and computes one shared fit recipe. It never moves individual quads. Rejection preserves the previous output. The API deliberately rejects structural PathChoice rows: that route continues to use its actual SafePanel adapter and the same fit math.

`UiSceneLogicalLayoutTest` passes MSVC `/O2 /W4 /WX /UNDEBUG`, including two invocations with identical function/frame addresses, split text atlases, material geometry, foreign overlap, incomplete/changed proof and atomic rejection. The existing observer test instantiates the adapter against its real Row type and passes; shared placement/HUD tests and seven decoder tests also pass. CMake test wiring is supplied separately to the coordinator; no runtime Hook, admission scope, layout policy or default manifest changes.

The minimum remaining evidence is one actual Hammer event linked to its pickup action identity and unique completed invocation, all of that invocation's text/tile/material members without an intervening flush, and the independent world/ticket/resource plus GPU viewport/scissor proof. Those facts must then authorize one immutable recipe for every member and bypass ordinary per-entry layout for that group. Current105 evidence has no Hammer or PathChoice coverage and predates runtime verification of the unique-token extension. This candidate has no GPU replay, hitbox or headset acceptance; enabling it for all HUD or writing a guessed Hammer offset remains unsupported.

## 2026-10-19 — complete-group sampling candidate prepared

Root built a separate `build-runtime-resolution1-ui-group` candidate with both
`GEARS_UI_CALL_BOUNDARY_OBSERVER` and `GEARS_UI_LOGICAL_GROUP_OBSERVER` enabled.
It includes the current PathChoice close/position renewal fixes. DLL SHA256:
`7066f6c2faebc7b5a8a706c75de57c9cba7cbe7d64d531e178f9bb4624413366`.
All 22 startup checks plus PositionService and CloseRestore tests passed.
The private build command, manifest and checks are under ignored analysis/runtime
(`build-runtime-ui-group.cmd`, `manifest-runtime-ui-group.json`,
`ui-group-startup-checks.json`). No default player manifest/package was changed.

The missing evidence remains an actual Hammer pickup/action invocation with its
complete text/icon/material membership and corresponding GPU output. Historical
chapter providers must be read again in the live process; old PID/address/index
values cannot be replayed. Current fit_invocation is not yet wired into shared
text/material replay. This build prepares that evidence collection only; it does
not claim Hammer placement or common UI layout is fixed. No game was started for
this candidate's checks.

---

## 中文

# 共享UI组的安插准备

这是离线准备, 不是一个启用的 PathChoice UI 修正或头盔接受 。

`ui_group_placement.h` 将一个源态统一缩放和翻译应用到一个完整的被承认的组。每个眼都保留其独立的面板视图。保存的源剪刀遵循相同的折叠转换并交叉该眼的界限。空的或无效的结果 re 弹出式解剖;帮助者无法恢复已经被游戏所困的几何.

现有的HUD布局将它既定的亲和计算方式交给了这个帮助者。一个实际身份 HUD 政策保留了面板不变。一个非身份的HUD 政策,其产生的亲和性恰好是身份,但它仍然在重算保存的源片段,预 服务于前一个行为。

## Verification

两种独立的CPU可执行文件都流传 2026-10-17:

- `UiGroupPlacementTest`:任意团体成员常见运动,不对称眼,独立预期像素剪辑,缩放,部分/外剪辑,数字限制和拒绝时输出不变.
- `UiHudLayoutTest`: 现有的HUD行为和中心圈式身份-美因回归案例.

高频测试使用 16 MiB 已存在的大型 CPU 固定装置的可执行文件堆栈; CMake 链接选项只影响此测试。 私有可执行文件被忽略 `analysis/ui-group-placement-cpu/`.

## 其余的一体化

原始场景路线和矩阵证据记录在文档中 122 and 124 生产用途仍需要经过认可的根场景、由文本/瓷砖/材料有效载荷共享的不可变组接收、跨越相关类型密钥的完整收集以及对照实际GPU视图/剪切状态进行核查。 不得对该组进行第二次改造。

此更改中不包括根钩、 PathChoice 翻译、 新的 DLL、 游戏发布、 模拟器发布或运行时间测试。 挂载的相机样本更改 。 `d803216` 在一个实际挂载的会话中, 保持单独未验证。 最新的开发 ZIP 时间比执行时和此帮助时早 。

## 2026-10-18: 共同适应和土著105 高频基准

共享帮助器现在计算一个从一个完全逻辑组到一个明确的目标矩形的正常化匹配。 当它适合和缩小了其它的一致时, 它会保留所要求的统一大小; 它没有特定页面的抵消或默认目标。 HUD 绑定 使用共享类型而不改变已有的玩家布局。这是准备,不是启用的通用 UI 校正。

根协调员创建了私有的 UiGroup105,通过22 启动自查,并运行了背景基线。游戏有一个8×8 背景产出和提交的VR场景/复制件仍保留0,所以没有视觉接受。逻辑观察者单独发布的CPU计数器175 样本和保留最新8 认证的后发货人组;不得将其与VR出版物混淆。8 保留组别通过检查前/返回/根/底/边界,20–22 彩虹的电话12–13 简单范围及3–4 物质范围, 没有呼叫溢出, 没有解密的 Canvas 呼叫或同步的简单/ 材料执行。 没有 PathChoice 根样本。 协调员正常停止并核实 Hook 恢复、 游戏退出和设置/ 常规恢复 。

运行时帧/名称证据包括 WarHUD. PostRender, DrawHUD, DrawHealth, 更新武器指示器, DrawWorldInfo, DrawWorldIcon, DrawEmptyAmmo, DrawAmmo 和队友/定位程序。 在实际观测到的 DrawWorldInfo UFunction 节点上0x37e51de0,解码器在7 of8 保留组; 行2 拒绝不完整的选定调用成员证明。 仅由UFunction 祖先过滤就可以合并重复引用, 因此这些是诊断结合, 不是被证明为单一的逻辑提示。 DrawActionInfo、 锤子拾取事件和选择路径没有被 cov ered. 结果不能确定报告的接车路线、 GPU 剪接、输入重击框或耳机布局 。

证据仍然被忽视: `analysis/runtime/continuous-stereo-23004-20261018-180334-03ce8195.native-status.txt`, `uigroup105-logical-group-bounded-report.json`, `uigroup105-weaponinfo-ancestor-report.json` and `uigroup105-logical-group-bounded-proof.json`。该证据保存源状态 SHA256。该词10 相关的 UI 文件匹配已冻结 `analysis/native-source-uigroup105/` 复制到下一个扩展名之前; 每个文件 SHA256 双对保存在 `uigroup105-ui-source-comparison.json`中,将明确保存随后的独到源码差异。 `uigroup105-ui-source-after-unique-token.json`,不归属于105 run.

下一个仅限观察的扩展使用已有的, 字节检查1d6f30 脚本调用边界; 它不安装附加的 Hook 并更改没有配置的语义范围。 在被抽样认证的 HUD 父程序下, 活函数/ WarHUD 身份、 脚本范围、 原始 VM 条目和调用者祖先必须在独有的孩子符号 i 之前通过 。s 指定。 空格调用只保留一个符号, 只有当其实际的机身框架与被调用人/ 自调用人和代码范围相匹配时。 正常返回会重新验证函数身份和脚本/ 输入。 重复使用相同的UFununction和堆栈地址会得到不同的 t OKens。原参数、返回位、例外和完全一个原调用被保留。解码器的 `--invocation` 在释放复制的组候选人之前, 请检查符号、 正常完成、 机身计数、 祖先和打字成员范围; 它仍然从未授权重玩 。 此扩展有离线检查, 而不是运行时的 要求 。105。默认玩家列表和私人路径选择定位策略保持不变。

## 2026-10-18: 采信适用性界限和一次性使用合适候选人

被捆绑的 KB 搜索和7个现有的脚本输出可以建立静态的皮卡数据流候选. `WarPickupFactory.GetPickupAction` 明确测试 `WEAP_HammerOfDawn` 并填充动作图标和项目名称工具提示; `WarDroppedPickup.GetPickupAction` 也有明显的锤子分支. `WarPC.UpdateHUDActions` 将成功的接车动作传递至 `MyWarHud.SetActionInfo(13, Action)`。该设置器写 `ActiveAction`; `DrawActions` 既可以画又可以画 `ActiveAction` 和渐渐消逝 `PreviousAction` 通过单独 `DrawActionInfo` 。后一框架会发出背景文本和 Canvas 牌。因此,既不是一个函数名过滤器,也不是每个字符的结合 `DrawActionInfo` 祖先识别一个提示。 这些导出包含未解决的本地符号/ 类型; 这是静态数据流, 不是用户实际锤子实例或每个分支条件的证据 。

源码身份是 KB 文物2, `WarfareGame.u`, 莎莎 (英语).256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`模块基数0 包的,你给我的 `DrawActionInfo` 导出 SHA256 is `1a1fb2e41b64012bd834815609812e2a1a0e7355edc527998af00a939a8f1085`;工厂皮卡出口SHA256 is `5031b1a09fec3a0568f84a7925a59e7215b0ea1984b26414886c5657e7e4c36e`。每个导出对象身份、散列、未解决的标记和界限线节选全部保存在忽略中 `analysis/ui-group-placement-cpu/layout-applicability-20261018.json`,加上未变的105 负覆盖证明。 没有发生新的本地分析、 IDA 或游戏执行 。

共享执行边界是完全正常化的内容界限加 `gears_ui_group::fit`,而不是一个普通的已验证的场景父。PathChoice有其单独认证的场景13- 节点树和10 直接安全板内容子, 本地版式无效及恢复。 pick 候选选项使用 Canvas 文本/ 瓷砖组; 没有为此设置安全板路径 。 UIScene root 没有 UI Object `RenderBounds` (docs148),因此场景矩形或整个后Render矩形不能取代内容界限.

`ui_scene_logical_layout.h::fit_invocation` 现在从现有的认证的 HUD 观测器中生成一个CPU 候选儿童身份。它检查正常的根/子返回和身份证明、身体祖先/计数、完整的按键间输入成员映射、间隔校正和没有假名 缩略语 : 缩略语 : 缩略语 : 缩略语 : 缩略语 : 缩略语 : 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩略语: 缩写: 缩略语: 缩略语: 缩写: 缩写: 缩写: 缩略语: 缩略语: 缩略语: 缩写: 。ecipe。 它从不移动单个四位数。 拒绝保留了先前的输出。 API 故意拒绝结构 PathChoice 行: 该路由继续使用其实际的安全板适配器和相同的适配数学 。

`UiSceneLogicalLayoutTest` 通行证 `/O2 /W4 /WX /UNDEBUG`,包括两个引用,功能/框架地址相同,分解文本图集,材料几何,异形重叠,不完整/更改的证明和原子拒绝。现有的观察者测试将适配器与其真实的行类型比对并经过;共享的放置/HUD测试和7个解码测试也通过. CMake测试线程单独提供给协调器;没有运行时的Hook,录用范围,布局政策或默认的表单变化.

剩余的最低限度证据是与其小动作身份和独特的完整引用相关的一个实际锤子事件,所有引用的文本/瓷砖/材料成员没有插笔,以及独立世界/平板电脑/资源加GPU 视图/剪切证明。这些事实必须允许每个成员有一个不可改变的配方,并绕过该组的普通每进布局。105 证据没有锤子或路径选择覆盖,且运行时间已超过对唯一托肯扩展的校验。该候选人没有GPU重播、击打框或耳机接受;允许所有 HUD 重播或写入猜想的锤子抵消仍然不支持。

## 2026-10-19 ——编制完整组抽样候选品.

根建一个单独的 `build-runtime-resolution1-ui-group` 两者均具有候选人资格
`GEARS_UI_CALL_BOUNDARY_OBSERVER` and `GEARS_UI_LOGICAL_GROUP_OBSERVER` enabled.
它包括当前路径选择的关闭/位置更新修正。 DLL SHA256:
`7066f6c2faebc7b5a8a706c75de57c9cba7cbe7d64d531e178f9bb4624413366`.
All 22 启动检查加上位置服务测试和关闭Restore测试通过.
私人建筑指挥、清单和检查被忽略 analysis/runtime
(`build-runtime-ui-group.cmd`, `manifest-runtime-ui-group.json`,
`ui-group-startup-checks.json`。没有更改默认玩家列表/包装 。

缺失的证据仍然是实际的锤子小卡车/行动援引,并附有
完整文本/icon/材料成员及相应的GPU输出。
章节提供者必须在实时进程中再次阅读; 旧的 PID/地址/索引
无法重播值。当前 fit_引用尚未连接到共享
文本/材料重放。此构件只准备收集证据;它确实
无法声明锤子放置或通用的 UI 版式已固定。没有开始游戏
这个候选人的支票。
