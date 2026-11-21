# Quest workbench interaction redesign — 2026-11-12

Owner scope and feedback

The owner confirmed the previous capture functions were usable, but requested a simpler interface: different colors for volumes, controls attached to a selected component, single-hand posing while the other hand clicks, and an all-part current-pose overview with editing. The earlier empty disk project did not disprove their in-headset capture report. They explicitly said the old data was unnecessary before this update. No game/other-weapon work, helpers or public package changes were started.

Delivered implementation

- Five direct tabs: capture, all-part current-pose overview, replay, capture library and settings. Blue main grip, green support, purple magazine, yellow E, orange C are stable volume/chip identities. Buttons and readiness status were enlarged.
- Default immediate single-hand capture. The posing hand is excluded from hand-menu input; the other bare hand can aim/pinch or use real Touch actions. Both-hand capture is secondary with3s countdown. Capture does not freeze live tracking or stop a running take. Two-second calibration is specific to the selected hand; bare pickup binds that hand after3s.
- Capture snapshots are independent captures/<UTCms>-<sequence>.json files. A bounded single storage worker publishes immutable archive + latest project copies. UI marks ready after actual completion, reports failures, and does not count pending entries as saved. Library scans actual archive files on startup, filters by part and supports multiple same-part captures.
- Five overview cards show actual stored Marcus hand meshes (independent left/right availability) and cropped original component geometry. Empty slots explicitly show unrecorded. Card selection previews a stored pose on the full weapon. Thumb caches are skinned only when local rotations change, using the same fixed-length FK/skinning path.
- Component selection opens a viewer-facing inspector whose position is attached to that component. C/E have mechanical sliders/presets; region controls have attached XYZ rings, drag rotation, position, per-axis box sizes, capsule length, snap distance/angle and release distance. These controls are hidden before selection and may be collapsed.
- Hand editing directly selects thumb/index/middle/ring/little, root/middle/distal, axis and a rotation slider; 3D joint markers can be selected by ray. Wrist translation/rotation has direct controls. Preview edits are archived as a new variant on save or return to live; previous entries are retained. Saved preview changes update the current in-memory slot too.
- Changing the only model no longer clears current poses. Switching real models restores that model's project. Drag callback failures remain UI errors rather than escaping the render loop.
- Desktop export retains captures and converts each capture's chosen part/hand into its own mod-captures directory. Existing Mod limits remain: bounded finger deltas only, canonical main-right/other-left consumer; full wrists/volumes/snap and mechanical state remain in the authoring project.

Validation and exact package

Current build:0.2.0-dev/versionCode8, APK SHA256 75d28bb397f7ed1cb5919b7a1e756a226f39227ec21b940467122e3a72e8c1ff,15189214 bytes. Candidate updated at J:/gearsVr/candidates/quest-workbench-20261111; Windows companion and guide updated. Installed base.apk was pulled back and matched. v5/v6/v7 build artifacts remain rollback records; no public tag or Git commit was created for this work.

Android arm64 compile/link, signature and16K alignment passed. Windows companion compiled. Portable core checks passed rotated sliders, source/held input, rotation-ring drag, posing-hand menu exclusion, independent archive ordering/atomic latest update/overwrite rejection, binary v1/v2, fixed-length retargeting and invalid controller-source finger capture rejection. Four Python checks passed existing Mod encoder roundtrip, reflected correction/bounds, wrong model/rig/basis and independent same-part library conversion with invalid part rejection. These remain offline checks.

Actual v7 UI offscreen rendering produced the two layout images personally reviewed: ui-v7-capture.png and ui-v7-overview.png. They render current application UI into a private framebuffer and do not inject input, alter tracking or create poses. The initial slots were empty, so those pictures validate component thumbnails/layout, not stored hand fitting. The ordinary eye capture had the world workspace outside the view while the headset was tilted; it is preserved and not called a passing layout view.

Owner explicitly replied “能看清、能点到” to the0.2.0 layout question. Actual v7 and final v8 right-controller tab/component/hand-selector/calibration clicks were observed. Final v8 process23653 reached session5, both1931x2023 eyes,2xMSAA, GL0 and simultaneous resume0; original tracking flags/calibration are retained. Owner's complete physical multi-capture/archive-reload/edited-thumb/rotation-drag confirmation remains pending. At the last bounded inspection there was no new captures directory and the visible actions were calibration/navigation, so do not claim two independent physical captures already passed. An async two-capture check is pending; do not repeat installations or stop the owner's current session.

Evidence: apk-build-v8-final, device-v8-start.log, device-v8-owner.log, device-installed-v8.apk, core-v6-fixture, quest-source-v020-manifest.json, delivery-v020-review.json, the two UI previews, and device-before-v6/device-before-v8 backups under J:/gearsVr/analysis/quest-workbench-20261112. Current installed package is left for manual use. No new game/XR simulator/other-weapon acceptance is implied.


Final owner capture readback and handoff

The owner subsequently saved4 actual independent left-hand captures. All four files were copied read-only to owner-v020-captures and parsed: support#1, magazine#2, charging#3 and starter#4. Each selected capture contains15 normalized local finger quaternions and26 raw joints, retargeted=true, fully_tracked=true, hand_data_source=1 (unobstructed). Exact IDs:1794418169059-1,1794418220181-2,1794418385098-3,1794418426664-4. New capture files coexist, rather than overwriting the previous part. The final snapshot was copied before any further change and no new installation followed.

This supersedes the earlier “no captures directory” observation. The main right-hand slot is not in this batch. The owner chose “暂时不试，先保留这版” for the proposed same-part two-capture check; do not run that check, reinstall, capture more diagnostic UI, terminate the current session or continue changes until asked. Preserve0.2.0/code8. Actual multi-file capture/write/readback is now verified; same-part variant physical selection, full edited-pose reload and complete fit remain unperformed. User's latest question asks whether these4 captures are visible; answer yes with the four exact parts/sides, not a claim of four motion takes.


Offline game-import question

The owner asked whether the4 accurately positioned captures can be used in the game. All4 converted successfully through the current finger-only adapter and passed the actual existing desktop editor validator/600-byte encoder. Prepared output: owner-v020-mod-import-review, with actual-editor-format-review.json and import-scope.json. No game, build, runtime input, install or live profile application occurred. Do not claim original hand placement was applied: current conversion deliberately emits zero palm translation/rotation. The captured weapon-relative wrist transforms remain intact in the original files. Complete positional import still requires converting the reflected object/wrist bases and attaching to each current moving component; C capture is90deg deployed, E0 travel, magazine must follow its item frame. Existing V2 adjustments are bounded50mm/45deg deltas. Answer with this current boundary; old raw data does not need recapture merely because the importer is incomplete.

---

## 中文

# 查询工作台互动重新设计—— 2026-11-12

拥有者范围和反馈

主人公确认先前的抓取功能是可用的,但要求有一个更简单的界面:卷的不同颜色,附加在选定组件上的控制,单手摆放而另一手点击,以及全部分的当前所有器概览 wi th 编辑。 先前的空盘项目并没有反驳他们的内置套件捕获报告。 他们明确表示, 在更新之前, 旧数据是不必要的 。 没有开始游戏/ 其他武器工作、 助手或公共软件包的更改 。

已交付执行

- 五个直接标签:抓取,全段当前所有控件概览,重播,抓取库和设置. 蓝色主抓取,绿色支持,紫色杂志,黄色E,橙C是稳定的音量/芯片身份. 按钮和准备状态有所扩大.
- 默认即时单手抓取。 假手从手菜单输入中排除; 另一只赤手可以瞄准/ 平板或使用真实的触摸动作。 双手抓取是次要的 。3 s 倒计时。 抓取不会冻结实时跟踪或停止运行。 2秒校准是特定于选定手的; 光接点绑定手后3s.
- 抓获快照是独立的抓获/<UTCms>-<sequence>.json 文件。 一个有边框的单存储工作人会发布不可移动的存档 + 最新工程副本。 UI 标记在实际完成后可以准备,报告失败,并且不计算待保存条目为保存。 库扫描启动时的实际归档文件, 过滤器由 p 艺术并支持多个同段抓取.
- 五张概览卡显示实际存储的Marcus手头的夹片(独立左/右可用)和裁剪的原始组件几何。空插槽明确显示未记录。卡片选择预览了全武器上存储的姿势。缩略图缓存被剥离 只有在本地旋转发生变化时,使用相同的固定长度FK/皮肤路径。
- 组件选择打开一个查看器显示检查器,其位置与该组件相连。 C/E 具有机械滑动/预设;区域控制器已附加 XYZ 环、拖动旋转、位置、每轴框大小、胶囊长度、断裂距离/角和 释放距离。这些控件在选择前隐藏,可能会崩溃。
- 手部直接编辑拇指/索引/中度/环/小度、根/中度/底部、轴和一个旋转滑动器; 3 D 联合标记可以由射线选择. 手持翻译/旋转有直接控制。 预览编辑器作为保存或返回到生命的新变体存档; 保留先前的条目。 已保存的预览更改也更新了当前内存槽 。
- 更改唯一的模型不再清除当前设置。 切换真实的模型会恢复该模型的工程。 拖动调用失败仍然是UI错误, 而不是逃避渲染循环 。
- 桌面导出保留抓取, 并将每个抓取所选的部分/ 手转换为自己的 Mod 抓取目录。 现有的模式限制仍然存在: 仅限指三角形、 犬形主右/ 其他左侧消费者; 完整腕/ 卷/ 锯齿和机械状态 。主要在作者项目中。

验证和准确的包件

当前建筑 :0.2.0-dev/versionCode8,APK SHA (英语:256 75d28bb397f7ed1cb5919b7a1e756a226f39227ec21b940467122e3a72e8c1ff,15189214 字节。候选人更新于 J:/gearsVr/candidates/quest-workbench-20261111; Windows 伴奏和向导更新。安装的碱基.apk被拉回并匹配 v5/v6/v7 建造文物仍为回滚记录;没有为这项工作创建公共标记或Git承诺。

机器人手臂64 汇编/链接、签名和16 K 校正通过。 Windows 伴奏编辑。 便携式核心检查通过旋转滑动器、 源/ 持有输入、 旋转环拖动、 假名菜单排除、 独立存档命令/ 原子最新更新/ 覆盖拒绝、 二进制 v1/v2,固定长度重定向和无效控制器-源手指捕获拒绝。四张 Python 检查通过现有的 Mod 编码器圆盘,反映校正/绑定,错误的模型/rig/basis以及独立的同部分库转换,无效部分 rejec TV,这些是离线检查

实际 v7 UI 屏外渲染生成了两个布局图像 : ui-v7- 抓捕.png和ui -v(英语:7-overview.png.它们将当前应用程序 UI 转换为私人帧缓冲器,而不注入输入,改变跟踪或创建外观。初始插槽是空的,所以这些图片验证组件缩略图/显示,而不是存储手装。普通的ey 。e 抓取在耳机倾斜时,有世界工作空间在视图外;它被保留,不称为通过版式视图。

Owner explicitly replied “能看清、能点到” to the 0.2.0 实际问题 v 7 和最后决定 v 8 观察到右控件标签/组件/手选/校准点击。 Final v 8 process 23653 已到达会话 5,两者 1931x 2023 eyes,2 通用高频 0 并同时恢复 0; 原跟踪旗/校准保留。 所有者完整的物理多抓/存档重装/编辑-thumb/rotation-drag确认仍有待确定。在最后一次限值检查时,没有新的捕获目录,可见动作正在 校准/导航, 所以不要声称已经通过了两个独立的物理捕获。 正在等待一个Aync 2- capture检查; 不要重复安装或停止所有者的当前会话 。

证据: apk-building-v8- 最终,设备V8- 启动.log,设备V8- 拥有者.log,设备安装到V8.apk, 核心- v 数据6- 固定,探险-源 -v020- 明显。 Json,快递V020- Review.json,两个UI预览,和装置 - 之前 -v6/device-before-v8 备份在 J:/gearsVr/analysis/quest-workbench-20261112. 当前安装的软件包留待人工使用,不暗示新的游戏/XR模拟器/其他武器接受.


最后所有者抓取回读和交接

主人后来救了4 。所有四个文件都只被复制到所有者-v020- 抓获和解析:支持#1 杂志 #2 充电 #3 启动器 #4 中。每个选中的抓取包含15 当地普通指尖和26 原始关节,重定向=真,全_跟踪=真,手_数据_源=1 (未阻碍)精确的ID:1794418169059-1,1794418220181-2,1794418385098-3,1794418426664-4。新的抓取文件共存,而不是覆盖上一部分。在任何进一步更改之前,最后的快照已被复制,没有新的安装。

This supersedes the earlier “no captures directory” observation. The main right-hand slot is not in this batch. The owner chose “暂时不试，先保留这版” for the proposed same-part two-capture check; do not run that check, reinstall, capture more diagnostic UI, terminate t 在当前会话中,或继续更改,直到被询问。保留 0.2.0/code 8。现在已验证了实际的多文件抓取/写入/读回;同部分的变体物理选择、完整的编辑后所有重载和完整的匹配仍然未实现。用户的最新问题询问这些是否 4 捕捉器是可见的;回答是,用四个准确的部分/侧,而不是声称四个运动得到。


离线游戏- 导入问题

店主问,是否4 精确定位的捕获可以用于游戏。所有4 通过当前仅指的适配器成功转换,并通过实际存在的桌面编辑器验证器/600-字节编码器。已编写输出:所有者-v020-mod-import-review, 包含实际编辑器格式审查. json和导入镜. json。 没有发生游戏、 构建、 运行时间输入、 安装或活版配置文件应用程序。 不要声称使用了原始的手插入: 当前转换刻意释放零棕榈 tran 剪切/旋转:在原始文件中,俘获的武器相对腕部变换仍然完好无损。完整的位置导入仍然需要转换反射物体/手基,并附着在目前每个移动部件上;C捕获是90 已部署,E级0 旅行,杂志必须遵循其项目框架。2 调整为范围50mm/45 deg 三角洲。用当前边界进行回答;旧的原始数据不需要仅仅因为进口商不完整而重新获取。
