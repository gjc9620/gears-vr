# 切换关卡时选关菜单黑屏

2026-10-20。用户在 VDXR 真机游玩后返回主菜单，选择另一关卡时头显黑屏；进入关卡后恢复。定位和修复发生在 Python 菜单显示观察器，不需要修改 native DLL 或 XR EXE。

## 原因与现场

会话 `continuous-stereo-30768-20261020-153714-f929944f`：游戏磁盘 EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，native `9505245bd8ba18632b6faac1d1c39232be7a069cffbd3164ae0c5e0e9151c07e`，XR `afcde6794e004852de0f2965c54cd0a6522136e28a2db3d979d636b783238e1e`。没有新增原生逆向或内存代码快照。

原观察器只接受主菜单、战役类型和继续战役三层。在同一新鲜、只读、一致性复核通过的场景栈内，以下后续页面被发布为 `UNKNOWN`：

| 原日志 sequence | 页面 | class | input_mode |
| --- | --- | --- | --- |
| 461 | DeltaSelectAct | WarUIScene | 1 |
| 575 | DeltaSelectMap | WarUIScene | 1 |
| 1252 | DeltaSelectDifficulty | WarUIScene | 1 |
| 1259 | WarfareConfirmation | UIScene_Confirmation | 1 |

这些页面保留 `DeltaMainMenu → DeltaCampaignType → DeltaCampaignMenu_ContinueGame` 父链，父级 input_mode 为4，子级为1。原日志从选幕开始，到选关确认后离开菜单的区间约89.539秒；这是未知页面区间，不是头显逐帧测得的黑屏时长。

XR 在已完成第一次关卡投影后，遇到菜单恢复期间的 `UNKNOWN` 会撤回窗口菜单显示，也不会提交旧关卡图像。因此选关子页面期间可能没有图层。进入新关卡后 generation 1→2、handoff commit 完成，原生双目发布和 XR projection 恢复，符合用户观察。原主菜单第一张保存的 WGC 图像有正常菜单内容；不能将问题归为所有菜单捕获都失败。

## 修复

`tools/native-capture/menu_display_monitor.py` 将上述四层加入完整、按顺序匹配的战役菜单显示链，统一发布 `MAIN_MENU`，沿用已有整窗口 WGC 菜单面板。它检查每层名称、class、input_mode，及新增路径的非零、不重复场景地址、实例号、相同 scene client outer 和未截断的采集。top 必须与栈末身份匹配。

缺失/乱序父链、不同 client、无效身份、错误 player/block、采集变化和未知页面继续拒绝。此处只授予显示选择，没有新增游戏输入、确认按钮或存档操作规则。离开菜单后仍须新鲜玩法状态和原有双目票据才能恢复投影。

## 验证

105项 Python 测试通过（菜单显示观察器与连续会话 runner），包括四层页面、0/1 block状态、每层身份/顺序/类型错误、无输入权限、连续 packet sequence 与新鲜时间戳，以及保存的实际真机菜单记录回放。现有 `StartupPanelPolicyTest` 和 `WindowFallbackPolicyTest` 可执行文件退出0。

ignored `analysis/runtime/chapter-menu-black-replay-20261020.json` 保存旧 `HEAD` 与修复代码的四页回放、源码与现场文件读取哈希及版本身份：旧分类均为0，修复后均为4。原始记录和回放工具仅留在 ignored `analysis/`，不进入发布包。

以上是实际记录的离线回放与 CPU/策略测试，不是修复后 GPU 或头显验收。用户已同意正常重启并重复“进入关卡 → 返回主菜单 → 选幕/章节/难度”的真机流程，结果待追加。正在运行的旧 Python owner 不会自动加载改动；正常退出游戏并从启动器重新启动后生效。

2026-10-20 后续用户确认“确实已经修复了”。该确认覆盖此次选关菜单黑屏。用户随后提出上机枪约2秒、下机枪约3秒的暂时黑屏，进入独立的切换延迟处理；不再将选关问题列为待确认。
