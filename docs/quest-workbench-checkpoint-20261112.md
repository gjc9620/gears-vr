# Quest workbench 0.1.4 checkpoint — 2026-11-12

Owner scope: repair the standalone Q3 workbench's unusable/controller menu feedback, correct the Lancer component side/visibility, and reduce accidental default gestures. This does not resume game tests or the seven paused weapons. Sole chat; no helpers. Canonical source remains J:/codexWorktree/gearsVr. Public v71 and the old game candidates/launcher remain unchanged.

Implemented and built:
- Real OpenXR Touch action set, attached before begin-session; synchronized current aim/grip poses, trigger/A/X clicks, squeeze pickup, B/Y workspace reset, independent hand/controller pointers. No controller pose manufactures finger joints.
- Held input must release after activation/focus loss. Missing tracking releases object ownership. Invalid pose extraction no longer freezes the application before validation. Live extraction has a three-second countdown. Saved pose previews no longer replace trajectory playback frames.
- Bare-hand application gesture controls default OFF; tracking/retarget/recording stay enabled. Either hand's SYSTEM_GESTURE/MENU_PRESSED flags suppress application hand gestures when enabled. A three-second bare-right-hand pickup binding supports raw hand movement while menu gestures stay off. This cannot turn off the reserved Horizon OS system menu itself; no device system setting was changed. Official guidance: https://developers.meta.com/vr/documentation/native/android/mobile-hand-tracking/ and https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrHandTrackingAimFlagBitsFB.html .
- Optional unobstructed-hand source and simultaneous hand/controller extensions only when advertised. Q3 reported both supported; v4 resume call returned0. User-presence events and B/Y reset address workspaces placed while the headset was on a table. Presence unsupported/absent is not inferred headset state.
- Corrected original weapon basis (-X,Z,-Y)*.01 with reversed winding. Wrist-local Marcus mesh/reference has matching X reflection; finger adapter reverses it before raw Mod baseline deltas. Explicit project joint basis + model byte identity rejects old mirrored profiles. Packaged reserved Lancer model wins over the old external copied default, without deleting user files; custom imports must use another name.
- Original component partition: body10954, C1089, E54, Receiver1300 triangles =13397. Exact triangle multiset matches the full original model; original C/E geometry is retained. Orange C preview0/45/90deg around original pivot; yellow E preview0..230.414mm along original rearward travel. Grab volumes follow previews. QWBTAKE2 preserves those scalars and source metadata; reader supports QWBTAKE1. This is an offline authoring component preview, not a full game-mechanics simulation.

Checks and delivery:
- Native Android arm64 compile/link, APK signature and16K alignment pass. Windows companion compiles. Portable core checks cover controller object following without finger data, lost tracking, held-input rearming, rotated panel ray intersections, binary v2 roundtrip, v1 compatibility, discontinuity and calibrated fixed-length retargeting. Three Python adapter tests include reflected correction sign, actual existing Mod encoder compatibility, bounds and wrong basis/model/rig rejection.
- Existing candidate updated in place: J:/gearsVr/candidates/quest-workbench-20261111. Version0.1.4-dev, versionCode5; APK SHA256 5ab66d76aa998488f9e4ff77733e75bbc6ab40f2cbc0ad384ce53d45704a5c94,13059294 bytes. Device install-r succeeded; pulling installed base.apk returned the same SHA. User data was backed up before update. No saved user project/take was found at that backup, only the prior model and own diagnostic image.
- V4 actual Q3 reached focused session, two1931x2023 eyes/2xMSAA and GL0. Its own rendered eye showed the fixed workspace outside the current view, motivating B/Y reset. V4 logs had no active hands/controllers or actual UI click; continuous frames are not input acceptance.
- V5 process17712 subsequently produced active focused hand/controller input and rendering updates. The retained active-use ring no longer contains its first GL-error/resume receipts; v5 GL0 is not separately claimed. Actual right /interaction_profiles/oculus/touch_controller aim/grip, hover and many controller UI click edges are recorded, including live return, timed capture request, recording start/stop and stepping an82-frame take. Frames remained live after invalid uncalibrated capture. Technical right-controller menu interaction is now observed, not merely compiled. Left-controller clicks, B/Y reset, complete calibrated capture/save/reload, subjective clarity/comfort and full surface fit remain unaccepted. Current calibration was00. The owner is manually using the app; do not reinstall, terminate, inject input or claim full acceptance. Async subjective feedback remains pending.
- Evidence under J:/gearsVr/analysis/quest-workbench-20261112: apk-build-v5/build and sign logs, device-installed-v5.apk, device-v4.log, device-v5.log, model-verification.json, lancer-parts-offline.png, core-v5-fixture, source manifest and delivery-review.json. Offline image is explicitly not a headset screenshot. Previous v3 APK remains in 20261111/apk-build-v3-final.

Remaining scope: generic imports are static clay; no general external rig retarget, magazine removal/full reload/firing simulation, or complete hand surface/comfort acceptance. The Mod converter still imports bounded finger deltas only. Full wrists/volumes/snap/mechanical state remain in the workbench project and are not yet consumed by the Mod. Source checkpoint is not a Git commit or public release.

---

## 中文

# 查询工作台 0.1.4 检查站 -- -- 2026-11-12

所有人范围:修复独立的Q3 工作本库的无法使用/控制菜单反馈, 纠正 Lancer 组件侧/ 可见度, 并减少意外默认手势。 这不会恢复游戏测试或七个暂停的武器。 单独聊天; 没有帮助者。 Canonical source still J:/codexWorktree/gearsVr. 公开 v71 和旧游戏 candidates/launcher 保持不变。

已执行并建成:
- 真正的 OpenXR 触摸动作集,在开始会话前附加;同步当前目标/grip 姿势,触发/A/X点击,挤压皮卡,B/Y工作空间重置,独立的手/控制器指针. 无控制器会摆出制造手指关节.
- 已保存的输入必须在激活/ 焦点丢失后释放。 缺少跟踪对象所有权。 无效的 profile 提取将不再冻结应用程序, 无法在验证前进行。 Live defry 已进行三秒倒计时。 已保存的预览已不再替换 tr 弹出式回放框。
- 空手应用程序手势控制默认关闭; 跟踪/ 重定向/ 记录保留启用。 任意手的 System_ GESTURE/ MENU_ PRESSED 旗在启用时会压制应用程序手势。 一个三秒的赤手右手皮卡绑定支持原始手势移动 菜单手势时 ent 。 这无法关闭保留 Horizon OS 系统菜单本身; 没有更改设备系统设置 。 官方指南 : https://developers.meta.com/vr/documentation/native/android/mobile-hand-tracking/ and https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrHandTrackingAimFlagBitsFB.html .
- 可选无障碍的手源和同时的手/控制器扩展仅在广告发布时才使用. Q3 报告两者均得到支持;v4 恢复呼叫返回0。用户存在事件和B/Y重置地址工作空间,在耳机位于表格中。不支持/缺席的存在不推定耳机状态。
- 修正原武器基础(-X,Z,-Y)*.01 。 Wrist-local Marcus mesh/Reference 具有匹配 X 反射; 手指适配器在原始 Mod 基线三角洲之前反射。 明确项目联合基础 + 模型字节身份 拒绝旧的镜像配置 。 包式保留兰瑟模型获胜 s 覆盖了旧的外部复制默认,而不删除用户文件;自定义导入必须使用另一个名称.
- 原始组件分区: 正文10954, C1089, E54,接收者1300 三角形= 三角形13397。精确三角形多集匹配完整的原始模型;保留了原始的 C/E几何。橙色 C预览0/45/90 绕原主轴的 deg; 黄色 E 预览0..230.414 沿原往后行 mm。 抓取预览后的卷数 。 QWBTAKE2 保存这些标尺和源元数据; 阅读器支持 QWBTAKE1。这是离线作者组件预览,而不是游戏-机械模拟。

检查和交货:
- 原生安卓臂(英语:Android arm)64 编译/链接、APK签名和16 K 校正通过。 Windows 同伴编译。 可移植核心检查覆盖控制器对象跟踪, 没有指头数据, 丢失跟踪, 被扣重装, 旋转面板射线交叉, 二进制 v2 往返旅行, v1 兼容性,不连续和校准的固定长度重定向. 三Python适配器测试包括反射的校正标志,实际存在的Mod编码器兼容性,界限和错误的基础/模型/rig拒绝.
- 现有候选人更新情况: J:/gearsVr/candidates/quest-workbench-20261111. Version0.1.4-dev, 版本代码5; APK SHA 软件256 5ab66d76aa998488f9e4ff77733e75bbc6ab40f2cbc0ad384ce53d45704a5c94,13059294 字节。 设备安装- r 成功; 拖动已安装的基数. apk 返回了相同的 SHA。 用户数据在更新前已被备份。 在该备份中找不到保存的用户工程/ 取用, 只有先前的模型和自己的诊断图像 。
- V4 实际数3 已达成重点会议,2次1931x2023 eyes/2 通用自动取款机和通用调制器0。它自己做成的眼睛显示当前视图以外的固定工作空间,激发了B/Y重置。4 logs没有活动手/控制器或实际的UI点击;连续帧不是输入接受.
- V5 process17712 后生成了主动聚焦手/控制器输入和渲染更新。保留的活动使用环不再包含其第一个GL-error/resume收款; v5 GL0 实际右/ 交互_ profile/ oculus/touch_ controller 瞄准/ grip, 悬浮和许多控制器UI点击边被记录, 包括活回、定时捕获请求、记录开始/停止和踩82-frame take. 帧在无效的未校准的捕获后仍然活下来。 现在观察到技术的右控菜单交互, 而不仅仅是编译。 左控键点击, B/Y 重置, 完整校准的捕获/ 保存/ 重载, 主观清晰度/ 舒适度和 f 表面完全没有被接受。00。所有者手动使用该应用程序;不重新安装、终止、注入输入或声称完全接受。Async主观反馈仍有待处理。
- 证据 J:/gearsVr/analysis/quest-workbench-20261112: apk-build-v5/ 建立和签署日志,设备安装-v5.apk,设备-v4.log,设备-v5.log,模型验证.json, lancer-parts-offline.png, 核心-v5-fixture, source express and excess-review.json. 离线图像显然不是耳机截图。上一个 v3 APK 仍在内 20261111/apk-build-v3-final.

剩余范围: 普通进口为静态粘土; 没有一般外部钻机重定向、 杂志清除/ 完全重载/ 火药模拟, 或完全接受手表/ 舒适。 Mod 转换器仍然只导入有界指三角体。 完整手腕/ 体积/ 斯纳 。p/机械状态仍留在工作台项目中,尚未被Mod消耗. Source检查站不是Git承诺或公开发布.
