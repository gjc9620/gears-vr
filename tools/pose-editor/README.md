# Lancer live editor — integrated development candidate (2026-11-11)

The current package is `J:/gearsVr/candidates/integrated-editor-20261111`.
`Open Desktop Editor.cmd` opens the normal launcher in packaged-simulator mode, the five-part pose editor, and the mouse-operated left/right controller panel. `Open Live Editor.cmd` opens the headset/editor workflow. The candidate's `先看这里.txt` has the complete Chinese steps. Public v71 and the fixed previous manual entry are preserved.

The desktop panel supplies human-controlled position, yaw/pitch/roll, held Grip/trigger, and A/B/X/Y/menu pulses. It uses this launch's isolated simulator directory, with serialized consume-once commands. No global runtime registry or LOCALAPPDATA changes. A separate heartbeat makes the simulator release buttons after the control application stops. Physical headset runtimes ignore this private simulator interface. Moving a controller is separate from editing a wrist/finger offset; real skin applies the edited profile only during the existing actual current grip.

Current editor protocol is **GVRPOSE2**, with five static part banks and display flags. Native/Playground use the matching V2 exports. Main grip adjustments change the weapon/muzzle mount; left adjustments are wrist-local changes to the admitted left visual pose. All original identity/current-source/DesiredBones/fixed-length checks remain. Yellow in-game lines preview the selected Grab Pose using the original rig translations, while red/cyan/purple ranges display current acquisition/insertion/retention geometry. There is no standalone skinned viewport or direct three-dimensional gizmo yet.

All five production components compile/link. Focused CPU/host/protocol/launcher-order and package checks pass; actual gameplay, visible pose changes in this integrated version, and headset acceptance are still pending. Source provenance, hashes, failures and results are saved in `J:/gearsVr/analysis/integrated-editor-20261111`. No game was launched for this integration round. Earlier development-version notes below describe the initial main-grip implementation and its historical evidence; their V1-only scope is superseded here.

---

# Lancer Pose Editor — development version 1

Windows desktop panel for the existing Lancer right main grip. Open `Open Pose Editor.cmd` on a machine with Python 3 and Tk installed. The current development machine has Python 3.14 with Tk. The panel can edit and save profiles offline; the installed public v71 does not expose the new runtime interface.

## Implemented

- Main-grip contact translation (millimetres) and rotation (degrees).
- Fifteen right-hand joints, each with three local rotation corrections; finger-wide incremental adjustment.
- Live preview, explicit send, undo/redo, save/load JSON, restore baseline.
- Separate received-configuration, completed C0 finger transaction, and completed weapon-component-write receipts. Stale runtime output is shown as stale; receipts are not final GPU/skin verification.
- Optional Native worker file reload, default OFF. No file parsing or file writes in C0/render callbacks. Callbacks take a nonblocking copy of static authoring data; transient contention can retain the previous static configuration, never old input or permission.

## Integration

Build Native and Playground together with the new optional exports. For a newly prepared PRIVATE capture session, add the following to that session's `capture.ini` **before Native starts**. The channel must be a separate, existing absolute directory for this session:

```ini
[PoseEditor]
enabled=1
channel=J:\gearsVr\analysis\pose-editor-20261111\session
```

Open the editor with `--channel` pointing to the same directory. Do not attach it by replacing DLLs in an already-running game. No public launcher, manifest or installed candidate was modified in this change.

The editor atomically replaces `preview.bin`; Native checks the fixed size, magic, FNV-1a corruption checksum, geometry signature, finite values, bounds and monotonically increasing revision. FNV is a corruption check, not an authentication mechanism. This is local authoring data and grants no action rights. Native retains the last valid configuration after an invalid, missing, older or revision-reused file and writes `status.json` from its existing collection worker. Poll cadence is 100 ms, not a measured end-to-end latency guarantee.

Each right main-hand C0 use still requires the actual admitted Lancer authored profile, current source, model, rig reference and all fifteen bones in the captured LOD DesiredBones. Only its existing finger writer postmultiplies the correction onto the current admitted pose. No second bone writer, controller sample, timestamp, grant, or relaxed freshness interval is added.

The main contact delta uses the original grip's forward/right/up axes, in metres. If the baseline hand-to-mesh transform is `B` and the edited palm contact is `D`, the new relation is `inverse(D) * B`. Derive it anew from the original mount on every update. The same adjusted mount feeds model matrices, weapon following, contact consumers and the existing muzzle calculation. **This alignment edit can change the gun and muzzle direction.** It is not a visual-only wrist correction. Existing identity, endpoint, source and component lease checks still gate every write and action.

Finger corrections use `current_local_rotation * delta`, with `delta = Rz * Ry * Rx`. The interface's index/middle/ring/little/thumb order matches existing Marcus115 bones 36..50. Translation norm is limited to 50 mm, palm rotation to 45 degrees, and each joint correction to 90 degrees; these are authoring bounds, not a claim of anatomical validity. The original joint translations, scales and bone lengths remain fixed.

Zero deltas select the existing compiled authored posture. Disabling preview restores the existing baseline through normal current-frame processing. Closing the panel normally sends a disabled revision. An editor crash does not send a disable: Native keeps its last valid static configuration until a new command or the end of the Native session. One editor should own a session directory.

## Evidence and remaining work

The first isolated production build is based on frozen natural-hand V144, with only this editor's changes overlaid. The live shared checkout contains unrelated in-progress weapon/cover changes; an attempted broad build failed there and was not used as an editor candidate. The isolated sources, exact diff, binaries, hashes, and logs are retained under `J:/gearsVr/analysis/pose-editor-20261111`.

Completed checks: Python profile validation/save/atomic transport; Python-to-C++ wire interoperability; independent inverse-contact transform and nonaccumulation; real Native worker reload/rejection/restoration in a host fixture; original Marcus115 owned-buffer finger rotation and fixed-bone preservation; desktop button/undo/send/normal-close inspection. Native and Playground production DLLs compile/link.

**Not yet game-recorded:** changing position plus one joint while holding Lancer, observing both actual rendered changes and same-version receipts, restoring and reloading a saved profile in the same game, and normal game/XR cleanup with this build. No headset acceptance or complete natural-fitting claim. The first version also does not include a skinned 3D viewport, VR point recording, left-hand editing, mirroring, collision fitting or an IK visualizer.

## Focused checks

Run Python checks with `py -3 -m unittest discover -s tools/pose-editor -v`. The CMake project in this directory builds three focused host/CPU executables. `PoseEditorTest` optionally accepts the generated Python wire fixture; `PoseEditorRuntimeTest` takes a private test channel directory; `PoseEditorC0Test` takes a retained original-rig fixture directory containing `reference.bin`, `C0-local.bin` and `C0-component.bin`. These executables never start a game.

---

## 中文

# 蓝瑟实况编辑——综合发展候选人(2026-11-11)

当前软件包是 `J:/gearsVr/candidates/integrated-editor-20261111`.
`Open Desktop Editor.cmd` 打开普通的发射装置,即包模拟器模式、五部分的姿势编辑器和鼠标操作的左/右控制面板。 `Open Live Editor.cmd` 打开耳机/编辑工作流程。 `先看这里.txt` 拥有完整的中国步骤。71 ,并保留了上一个固定的手工条目。

桌面面板提供人控位置, yaw/pitch/roll, 持有 Grip/ trigger, 以及 A/B/ X/Y/ 菜单脉冲。 它使用这个发射的孤立模拟器目录, 并有序列化的消耗- ce命令。 没有全局运行时间注册或 LOCALAPPDATA 更改 。 A s eparate single 使控制应用程序停止后模拟器释放按钮。物理头盔运行时间忽略了这个私有的模拟器接口。移动一个控制器与编辑手腕/手指偏移是分开的;真实的皮肤应用了编辑的 profi le 只在当前实际抓住时才使用。

当前编辑器协议是 **GVRPOSE2**,有五个静态部分库并显示旗帜。2 导出。主要抓控调整改变武器/口罩挂载;左侧调整是手腕-局部更改已接受的左侧视觉姿势。所有原始身份/当前-源/DesiredBones/固定长度检查都保留。黄色的游戏中线预览选中的 G rab Pose 使用原始的钻机翻译,而红/青/普尔范围显示当前获取/插入/保留几何。还没有独立的剥离视港或直接三维视差。

所有五个制作组件都编译/链接. 焦点CPU/host/protocol/launder-order和包检查通过; 实际游戏,可见的这个集成版本的变换, 头盔接受仍然有待完成. source exitance, hashes, fails and r 苏尔特被保存在 `J:/gearsVr/analysis/integrated-editor-20261111`。没有为这一回合的整合推出游戏。以下早期开发版说明描述最初的主食执行及其历史证据;其V1-这里只能取代范围

---

# 兰瑟·波斯编辑器——开发版 1

用于当前 Lancer 右主抓的 Windows 桌面面板。 打开 `Open Pose Editor.cmd` 在带Python的机器上 3 并安装了 Tk。当前开发机器有 Python 3.14 使用 Tk。 面板可以编辑和保存脱机配置文件; 已安装的公用 v71 不显示新的运行时界面。

## Implemented

- 主食接触翻译(毫米)和旋转(度).
- 右关节十五个,每处有3个局部旋转的修饰;指向全向递增调整.
- 直播预览,明确发送,撤销/重做,保存/装入JSON,恢复基线.
- 单独的接收配置,完成 C0 指向交易,以及已完成的武器组件-写入的收据。 Stale 运行时输出显示为 stale; 收据不是最后的 GPU/ 皮肤校验 。
- 可选的本地工人文件重新装入, 默认关闭。 没有在 C 中写入文件解析或文件0/ 放弃调用。 回调会获取静态作者数据的非屏蔽副本; 瞬态的争论可以保留先前的静态配置, 永远不要旧的输入或权限 。

## Integration

新建本地和游戏场, 加上新的可选导出。 对于新准备的 PriVATE 抓取会话, 请在会话中添加以下内容 : `capture.ini` **本地开始前**。频道必须是本届会议单独的、现有的绝对目录:

```ini
[PoseEditor]
enabled=1
channel=J:\gearsVr\analysis\pose-editor-20261111\session
```

打开编辑器 `--channel` 指向同一目录。 在此更改中, 没有修改过公共发射装置、 显示或安装的候选选项 。

编辑器在解剖学上取代 `preview.bin`原生检查固定大小,魔法,FNV...1 腐败校验和、 几何签名、 有限值、 界限和单调增加的修订。 FNV 是腐败校验, 不是认证机制。 这是本地授权数据, 并且不授予任何动作权限。 土著保留了最后一个有效的配置 。无效、 缺失、 旧的或重用过的修订文件后进行配方并写入 `status.json` 收集工作, Poll cadence是 100 ms,不是量度的端到端延迟性保证.

每个右手主手C0 使用时仍需要实际被承认的 Lancer 作者的配置、当前来源、模型、钻机参考和所有15个在所捕获的 LOD DespedBones 中的骨头。只有它现有的指头写法将修正值乘以当前被承认的姿势。没有secon d 添加了骨写器、控制器样本、时间戳、赠与或放松新鲜度间隔。

主接触三角洲使用原握柄的前/右/上轴,以米计。如果基线手对头变换为 `B` 和编辑的掌上型联系人是 `D`,新的关系是 `inverse(D) * B`。每次更新时,从原始挂载中重新获取。同样的调整挂载的种子模型矩阵、武器跟踪、联系消费者和现有的口号计算。 **这种对齐编辑可以改变枪口和口角的方向.** 它并非只用视觉的手腕矫正,现有身份、终点、来源和组件租赁检查仍然为每张文字和行动打开大门。

指向校正使用 `current_local_rotation * delta`,与 `delta = Rz * Ry * Rx`。界面的索引/中/环/小/通令与现有的Marcus匹配115 bones 36..50。翻译规范仅限于 50 毫米, 棕榈旋转至 45 ,每个联合校正改为 90 度; 这些是写法的界限,而不是解剖有效性的主张。原始的联译、 鳞片和骨长仍然固定。

Zero deltas 选择了已编译的作者姿态。 禁用预览会通过正常的当前帧处理恢复已有基线。 关闭面板通常会发送禁用的修订。 编辑器崩溃不会发送禁用: 原生保留其基准 。 最后一个有效的静态配置, 直到新命令或本地会话结束。 一个编辑器应该拥有会话目录 。

## 证据和剩余工作

最早的孤立生产建筑是基于冷冻的天然手V144,只有此编辑器的修改覆盖。共享的直播检查输出包含不相关的在建武器/封面更改;尝试的宽建失败,没有用作编辑候选人。孤立的源、精确的diff、二进制、散列和日志 s 保留在 `J:/gearsVr/analysis/pose-editor-20261111`.

已完成的检查: Python profile 校验/保存/原子迁移; Python-to-C++ 线互操作性; 独立的反接触变换和不累积; 真正的本地工人在主机固定装置中重新装入/拒绝/恢复; 原Marcus115 自有缓冲手指旋转和固定骨保存;桌面按钮/undo/send/ 正常-关闭检查. 原生和 Playground 制作 DLLs 编译/链接.

**尚未录制游戏 :** 正在改变位置加一个关节,同时持有 Lancer, 观察实际已变换的版本和同版本的接收, 恢复并重新装入同一游戏中保存的配置文件, 以及正常的游戏/ XR 清理此构件。 没有耳机接受或完整 na 符合壁画要求的第一版中也不包括剥皮 3 D视图端口,VR点录制,左手编辑,镜像,碰撞装配或IK可视化器.

## 重点检查

运行 Python 检查用 `py -3 -m unittest discover -s tools/pose-editor -v`。本目录中的CMake项目构建了三个焦点主机/CPU可执行文件。 `PoseEditorTest` 可选择接受生成的 Python 线固定; `PoseEditorRuntimeTest` 使用一个私有测试频道目录; `PoseEditorC0Test` 使用保留原正则固定目录,其中包含 `reference.bin`, `C0-local.bin` and `C0-component.bin`。这些可执行文件从不启动游戏。
