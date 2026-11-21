# Scoped walking and mounted access limit

2026-10-17. Actual123 and Actual124 used the existing Native67 DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`; no native rendering change was made in these runs. Python includes the discovery and failure-evidence changes in `2bb3cc3` and `7f74a50`. This is test-access progress, not a mounted black-screen fix.

## Input evidence

Actual123 (`continuous-stereo-23252-20261017-205924-dd35ddea`) checked the live WASD bindings, then posted each key only to the owned game HWND for 0.55 seconds. All four measured displacements were zero. During a separate A-key probe, the bounded live PressedKeys/axis observations remained empty/zero; the mouse comparison admitted RightMouseButton. Cached historical input-loop assembly suggests a keyboard cleanup boundary, but no exact imported key-query identity or causal runtime explanation was established. A failed read-only import assertion is retained rather than converted into such proof.

Actual124 (`continuous-stereo-38880-20261017-211141-64dddf4a`) temporarily used the game's own keyboard-binding menu and normal profile-save path to bind MiddleMouseButton to secondary backward movement and ThumbMouseButton1 to left turn. The capture/duplicate dialogs were checked through reflected callback, sender, owner and reserved-key evidence because their rich text was outside the existing strict string decoder. This test-specific reader did not relax production menu input checks.

A 0.65-second middle-button pulse moved the pawn 144.42 units; subsequent neutral observations had zero displacement. A 0.15-second side-button pulse changed controller yaw by 17.52 degrees; its neutral yaw change was zero. Later bounded navigation also recorded releases and neutral checks. No foreground activation, global input, controller emulation, native function invocation or game-memory write was used. Physical background controller response remained in the original disabled state. Foreground HWND was unchanged across these recorded navigation actions; this is not a claim that the foreground never changed during the entire session.

## Mounted reproduction remains blocked by access

Fresh same-WorldInfo discovery identified two non-template Troika actors, both without Driver/Controller. The chosen nearby actor's origin was only a navigation hint. Direct and short detour attempts stopped at a wall, with collisions near y=-11851.8 at distinct x positions. The final player-to-actor distance was 489.63 units, versus an observed EntryRadius of 100. No Use input or possession occurred.

Target-facing GPU crops show masonry and a projecting column, not a verified passage. A bounded independent route review recommended retracing previously traversed floor before further reconnaissance; it did not establish a shortcut or justify a checkpoint change. Saved artifact2 `GetEntryLocation` declarations also compute entry from skeletal references and an offset, so Actor.Location is not an established exact entry point. Attempts to inspect the relevant current reflected fields stopped at prerequisite guards and produced no field values.

The latest admitted menu observation changed to `DeltaMainMenu`. The subsequent read-only field query rejected its gameplay guard, without sending input. Neither the main-menu transition's cause nor mounted rendering behavior is established by these observations. Mounted target admission must not be widened on this evidence.

## Lifecycle and limits

Both initial 90-second read-only monitors passed. Actual124's final lifecycle report records 2242.97 seconds and 130234 projection submissions, but includes menus/binding work and a maximum submission gap of 583377.968 ms. It is **not** a continuous-play, pacing or headset acceptance result.

Both sessions verified orderly XR shutdown, native cleanup, entry restoration, normal owned-game closure and all owned-process exits. All four protected profile files and the full original game-settings file were restored and hash-verified after each run, including removal of the temporary test bindings. Actual124 settings restored SHA256 is `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`.

Ignored receipts include `vrdof123-window-walk.json`, `vrdof123-key-state.json`, `vrdof124-mousewalk-window-walk.json`, `vrdof124-thumb-turn.json`, `vrdof124-troika-approach2.json`, the `vrdof124-obstacle-*`, `vrdof124-detour-*`, `vrdof124-wallfront-*` and `vrdof124-entry-fields-*` records, and both sessions' normal-close/profile/settings restoration receipts. No proprietary derivative was added to Git or a release package. Mounted output, PathChoice placement, the reported recoil-ring appearance, startup latency and headset pacing/extended play remain open; `headset_verified=false`, `release_ready=false`.

---

## 中文

# 范围在步行和挂载出入限制

2026-10-17 实际123 实际数124 使用已有的土著67 沙发256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`;在这些运行中未发生本地渲染变化。 Python 包括发现和失败证据的变化 `2bb3cc3` and `7f74a50`。这是测试访问进度,不是安装的黑幕固定器。

## 输入证据

Actual123 (`continuous-stereo-23252-20261017-205924-dd35ddea`)检查了活的WASD绑定,然后将每个密钥只发布到自有游戏 HWND 上 0.55 秒。所有四个测量的移位均为零。在单独的 A 键探测器中, 被绑定的活按键/轴观测值仍然为空/ 0;鼠标比较录入 RightMouseButton。 Cached 历史输入- loop 组装建议键盘 Cl 移出边界, 但没有确定精确的导入密钥清空身份或因果运行时间解释。 失败的只读导入断言被保留, 而不是转换成这样的证明 。

Actual124 (`continuous-stereo-38880-20261017-211141-64dddf4a`)临时使用游戏自己的键盘绑定菜单和普通配置保存路径将MiddleMouse按钮绑定为二次后向移动和缩略图Mouse按钮1 。通过反射回调、发送者、所有者和保留键证据检查了抓取/复制对话框,因为其丰富的文本不在现有的严格字符串解码器之外。此测试特定的阅读器没有放松生产菜单输入 checks.

A 0.65-第二中键脉冲移动了卒子 144.42 单位;随后的中性观测结果无移动。 0.15-第二副按钮脉冲改变控制器 yaw by 17.52 度; 其中性 yaw 更改为 0。 后期的导航也记录了释放和中性检查。 没有使用前置激活、全局输入、控制器仿真、本地函数引用或游戏记忆写入。 物理背景控制器 r 响应仍然处于原始的禁用状态。 这些记录的导航动作中, 前置 HWND 都没有变化; 这不是一个预置在全场始终不变的说法 。

## 载运的复制仍被访问屏蔽

新鲜的同源世界信息发现发现了两个没有模拟的三驾马车演员,都无司机/主计长。选中的附近演员的源头只是导航提示。直接和短路尝试在墙上停止,碰撞接近Y=-11851.8 。在不同的 x 位置上。最后的玩家对演员距离是 489.63 单位,相对于观察到的 100。没有使用输入或拥有。

目标显示的GPU作物显示的是泥石流和投影柱,而不是经过核实的通道。 边界独立路线审查建议在进一步侦察之前重新追踪先前经过的地面;它没有建立捷径或证明检查车是正当的。已保存的文物2 `GetEntryLocation` 声明还计算出从骨骼参考和抵消中输入的条目,因此Actor.Location不是一个既定的确切切入点。检查当前相关反映的字段的尝试在先决条件的卫士中停止,没有产生字段值。

最新已接受的菜单观察改为 `DeltaMainMenu`。随后的只读字段查询拒绝了它的游戏游戏守护,但没有发送输入。这些观察既不确定了主菜单转换的原因,也不确立了挂载渲染行为。在此eviden上,不能扩展挂载目标录入范围。ce.

## 寿命周期和限制

两份初次 90- 仅读第二台显示器通过。124 最终生命周期报告记录 2242.97 秒和秒数 130234 包括菜单/约束性工作以及最大提交差距 583377.968 女士,这是 **not** 连续播放、节奏或耳机接受结果。

两会均验证有序的 XR 关闭、本土清理、条目恢复、正常自有游戏关闭和所有自有进程退出。每次运行后,全部恢复了四个受保护的配置文件及完整的原游戏设置文件并进行了散列核查,包括 拆除临时试验装订。124 设置已恢复 SHA256 is `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`.

忽略的收据包括 `vrdof123-window-walk.json`, `vrdof123-key-state.json`, `vrdof124-mousewalk-window-walk.json`, `vrdof124-thumb-turn.json`, `vrdof124-troika-approach2.json`,该 `vrdof124-obstacle-*`, `vrdof124-detour-*`, `vrdof124-wallfront-*` and `vrdof124-entry-fields-*` 记录,以及两个会话的正常关闭/配置/设置恢复收据。没有在 Git 或一个发布包中添加专有衍生工具。挂载输出、PathChoice 位置、报告的后坐力环外观、启动时的空档和头盔间隔/展出 裸体游戏仍开放; `headset_verified=false`, `release_ready=false`.
