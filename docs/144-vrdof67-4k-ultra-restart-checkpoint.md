# Native67 4K Ultra restart and checkpoint regression

2026-10-17. Actual119 and Actual120 used the unchanged Native67 development candidate from [docs142](142-vrdof67-development-candidate.md): source `f55f71313df32c96e588f658b7d4caf76eea2777`, DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. The tested settings/restart/checkpoint sequence succeeded in the simulator. This does not accept mounted gameplay, general UI, headset pacing or extended play.

## Settings sequence and actual output

Actual119 (PID43328, session suffix `9cf5026c`) selected 3840×2160 through the game's Display menu, accepted the timed video confirmation, and obtained a 3840×2160 GPU image containing both scene halves. The image SHA256 is `c9e38c6814f0786f157334ef47bb04159c235bad24c34d49a593c2678a3b51d6`. This image preceded the Ultra selection and is not Ultra evidence.

The same run selected the game's **Ultra** graphics preset, accepted the video confirmation and acknowledged the separate restart-required notice. Persisted settings were preserved for a normal restart; the prelaunch INI SHA256 was `e56d4a4a2f35b9ba07bf401de75f52922f8976f545dbcae2b49afa94269c520d`. Its ClientSettings selected 3840×2160/windowed; DLSS remained False and the graphics UI reported anti-aliasing None / resolution scaling Native. Preset labels and persistence do not independently attest each shader's live quality setting.

Actual120 (PID20316, session `continuous-stereo-20316-20261017-203219-ed8800b7`) restarted from that INI. The captured final GPU output was **3840×2161**, with a 3840×2160 engine source and two 1920×2161 XR images. The cause of the final one-row difference is not established. This is a test of the selected 4K mode, not evidence that each eye rendered at 3840×2160.

The gameplay GPU image SHA256 is `d0b75ef3d0229f16226c09af63b200650de2ad41e406f8d0fdea537f9fcd48ab`. After checkpoint recovery it is `7b40626b3747ba652769707075c2f78b377080acf608b77f124f2705381a464c`. Coordinating visual inspection found complete scene content in both halves. These images establish their sampled output, not uninterrupted image continuity or projection accuracy.

## Restart, pause and checkpoint evidence

Actual120 reached a successful first XR projection approximately 24.47 seconds after runner start, including automatic menu/level loading. The interval from the guarded camera-ready observation to that submission was approximately 4.31 seconds. The initial VR delay therefore remains a performance/experience issue, even though this startup succeeded.

The read-only monitor sampled approximately 90.21 seconds, with native publication advancing 2645→5696 and no monitor errors. It included pause entry, so it is not 90 seconds of uninterrupted gameplay. A fresh successful `eye_menu_first_frame` receipt was recorded after pause entry. That common panel is not stereo gameplay.

A state-checked checkpoint confirmation then committed world generation **1→2**. Polling observed generation2 with 33 new epoch submissions approximately **6.65 seconds after confirmation**, followed by the GPU image above. This is an observed recovery bound after at least ten submissions, not first-display latency. The runner recorded a `world_handoff_card_ended` event with replacement projection. An earlier `AnchorRegistryChanged` rebind failure remains in the log; it did not prevent the later committed recovery and has not been erased from evidence.

Native120 final publication reached 8417. Final-transfer normalization recorded 8421 planned/linked/completed transfers, 97 preflight rejections, zero post-mutation failures and `faulted=0`. The rejected preflights are retained, not counted as successful transfers. DOF publication rejection was zero; `d80a7b` produced 2698 valid calls/proofs per eye. Actual119 additionally exercised both certified sites (`d808d8`: 41909 per eye; `d80a7b`: 11506 per eye), with zero DOF publication rejections. This expands caller coverage; it does not identify a mounted-machine-gun path.

Projection-only shutdown counters omit paused eye-menu composition. Their approximately 64.24-second maximum projection gap must not be presented as 64 seconds of black output. Physical tracking and headset display remain unverified in these runs.

## Input scope and restoration

All navigation used fresh game menu state and messages posted only to the identity-checked game HWND. No foreground activation, global controller input or game-memory writes were used; the checkpoint operation's foreground HWND remained 789028 before and after. Physical controller background response remained disabled.

Native cleanup, entry restoration, orderly XR shutdown and normal owned-game closure were verified for both runs. Four protected save/profile files were restored after each run. After Actual120, the complete original settings INI was also restored and verified against SHA256 `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`; after-test settings copies were retained. All owned processes exited.

Ignored evidence is summarized by `analysis/runtime/vrdof120-result.json`, with the `vrdof119-*` and `vrdof120-*` receipts and original session logs retained alongside it. No implementation/default/package changes were made by these two tests. `headset_verified=false` and `release_ready=false` remain.

The user's current persistent authorization permits background game/simulator tests at any time, whether present or away, without repeated permission requests. Tests must still avoid stealing focus and keep physical background controller response off. This supersedes earlier requests to wait before starting background tests.

---

## 中文

# Native67 4 K 超重开和检查站回归

2026-10-17 实际119 实际数120 使用未改变的原生67 发展候选人 [docs142](142-vrdof67-development-candidate.md)来源 `f55f71313df32c96e588f658b7d4caf76eea2777`,DLLSHA(英语:DLLSHA)256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`。测试的设置/重起/检查点序列在模拟器中成功。这不接受挂载的游戏播放、一般UI、头盔间隔或扩展播放。

## 设置序列和实际输出

Actual119 (PID43328,会话后缀 `9cf5026c`选中的 3840×2160 通过游戏的"显示菜单",接受定时视频确认,并获得一个 3840×2160 包含两幕的 GPU 图像 半身。 图像 SHA256 is `c9e38c6814f0786f157334ef47bb04159c235bad24c34d49a593c2678a3b51d6`。此图像在 Ultra 选择之前,不是 Ultra 证据。

同一运行选择了游戏的 **Ultra** 图形预设, 接受视频确认并承认单独的重启要求通知。 已设置为正常重启而保留; 发射前的 INI SHA256 was `e56d4a4a2f35b9ba07bf401de75f52922f8976f545dbcae2b49afa94269c520d`选择客户端 3840×2160/ 窗口; DLSS 仍然为 False, 图形 UI 报告反变异 None / 分辨率缩放本地。 预设标签和持久性不能独立证明每个阴影的活质设置 。

Actual120 (PID20316 届会 `continuous-stereo-20316-20261017-203219-ed8800b7`)从该 INI 重新开始。所捕获的最后GPU输出是 **3840×2161**,带有 3840×2160 引擎源和两个 1920×2161 XR 图像。 最终一行差数的原因尚未确定。 这是对选中的测试 4 K 模式, 不是每个眼睛在 3840×2160.

游戏游戏 GPU 图像 SHA256 is `d0b75ef3d0229f16226c09af63b200650de2ad41e406f8d0fdea537f9fcd48ab`检查站恢复后, `7b40626b3747ba652769707075c2f78b377080acf608b77f124f2705381a464c`。协调视觉检查在两个半边处都发现了完整的场景内容。这些图像确定了其抽样输出,而不是不间断的图像连续性或投影精度。

## 重新启动、暂停和检查点证据

Actual120 大约达到第一个 XR 成功预测 24.47 运行后秒, 包括自动菜单/ 级别加载。 从监视摄像头的观察到提交的时间间隔约为 4.31 秒。因此,初始的 VR 延迟仍然是一个性能/经验问题,尽管启动成功。

仅读显示器的样本约为 90.21 秒, 本地出版物正在推进 2645→5696 且没有显示器错误。它包括暂停条目,所以不是 90 秒不间断的游戏播放。新成功 `eye_menu_first_frame` 暂停输入后记录了接收。 常见的面板不是立体游戏 。

国家检查的检查站确认,然后承诺世界一代人 **1→2**. 投票观测生成2 with 33 大约是新提交的时间 **6.65 确认后秒**,然后是上面的 GPU 图像。这是至少10个提交后、而不是第一播放间隔后观察到的恢复。 `world_handoff_card_ended` 替换投影。 `AnchorRegistryChanged` 复盖故障仍留在日志中;它没有阻止后来承诺的恢复,也没有从证据中抹去。

Native120 最终出版物已达 8417 记录的最后移交正常化 8421 计划/联系/完成的转让, 97 飞行前拒绝、零起爆后故障和 `faulted=0`。拒绝的飞行前记录被保留,不算作成功转让。 `d80a7b` produced 2698 每只眼睛的电话/防伪有效。实际119 额外行使了两个经认证的地点(`d808d8`: 41909 每只眼睛; `d80a7b`: 11506 ),其中没有DOF的发布拒绝。这扩大了呼叫者覆盖面;它没有识别挂载的机关枪路径。

仅投影关闭计数器省略了暂停的目菜单成分。它们大约 64.24- 第二条最大预测差距不得显示为 64 秒黑输出。物理跟踪和耳机显示在这些运行中仍未验证。

## 投入范围和恢复

所有导航都使用仅张贴到身份检查游戏HWND的新鲜游戏菜单状态和消息。没有使用前置激活,全球控制器输入或游戏记忆写入;检查点操作的前置HWND仍然保留 789028 前后。物理控制器背景响应仍然禁用。

已对两个运行进行本地清理、恢复输入、有序XR关闭和正常自有游戏关闭。每次运行后恢复了四个受保护的保存/配置文件。在实际运行后120,完整原始设置 INI 也被恢复,并与 SHA 对照进行验证256 `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`; 测试后设置副本被保留。所有自有进程退出。

忽略的证据摘要如下: `analysis/runtime/vrdof120-result.json`,与 `vrdof119-*` and `vrdof120-*` 接收和原始会话记录与它一起保留。这两个测试没有修改执行/默认/包。 `headset_verified=false` and `release_ready=false` remain.

用户当前持续的授权允许在不重复许可请求的情况下,在任何时间,无论是在现场还是外进行背景游戏/模拟测试。测试必须避免偷取焦点,并保持物理背景控制器响应。Sersed 先前要求等待开始背景测试前.
