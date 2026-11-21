# Native67 alternate DOF quad and pause recovery

2026-10-17. Actual117 used Native67 source `f55f71313df32c96e588f658b7d4caf76eea2777`, DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. Game PID10656, session `continuous-stereo-10656-20261017-195136-53668504`. The tested pause pattern now works in the simulator; this does not accept all menus or headset presentation.

## Evidence behind the caller correction

In [Actual116](140-vrdof66-checkpoint-pause-regression.md), actual `gpu_output_quad` counts at return RVA `d808d8` were 5698 per eye, matching the missing pre-quad proofs. Saved historical function `140d7fd14` contains a common setter followed by mutually exclusive quad calls returning at `d808d8` and `d80a7b`. This is not a resolution-specific branch, nor evidence that it is a mounted-gameplay branch.

The ignored `alternate-dof-branch-attestation.json` binds that saved analysis to historical artifact1 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, module base `0x140000000`, and pseudocode SHA256 `a5cf75e88d0d204574958c55bb6158364c664d7594293caab7629c0c7b6eddcb`. No IDA database was opened. Those historical identities alone do not attest current code.

Native67 explicitly accepts both exact quad callers using the unchanged fresh pre-quad certificate and one shared per-eye reserve. There is no setter-only or NoSite bypass. Caller attestation was strengthened from 96 bytes to the full 1819-byte range at `d80360`, SHA256 `e7bdb0107d1243e27ccdfc2c24e2a477b209ca1e31c3a78ec9e3489ee5423997`; the 200-byte setter check remains. Actual117 verified that live full range with `match=1`. Tests preserve unknown-site, duplicate/cross-site duplicate, one-eye and missing-quad rejection.

## Actual117 observations

The `d808d8` site produced 3943 calls and successful proofs per eye; `d80a7b` was unused in this run. Publication rejection was zero. The saved allowed receipt preserves world1/ticket4616, setter and quad masks3, and successful caller pair `d808d8,d808d8`. Normalization completed 4614 transfers with zero faults; native publication reached 4609.

Both pause entries produced successful fresh XR `eye_menu_first_frame` receipts: epoch1/ticket287 and epoch2/ticket1333, each with sample time later than its menu-entry minimum. Resume1 recovered gameplay projection. GPU snapshot2 (`c7ae7d21320144bf81df618cef960545d8bafd78c9e83c60094902be0582ed08`) contains complete duplicated pause UI in both halves according to the coordinating visual review. Snapshot3 (`cda4af5b1754ed03f70d2598cabf6cb07c305867bce985bccee5588ea8033a0f`) records resumed gameplay. Both are 2534×1009.

The pause image's approximately 93–94% black fraction reflects its black menu background; aggregate darkness is not evidence that an entire eye is blank. The common eye-menu panel is explicitly not stereo gameplay. Likewise, XR's approximately 62.8-second maximum projection-submission gap includes paused eye-menu composition: projection-only submitted counters omit that panel. It must not be reported as 62 seconds of black display.

## Cleanup and remaining acceptance

Native cleanup, entry restoration, orderly XR shutdown, normal game closure, all owned-process exits and restoration of four saved profiles were verified. Receipts remain ignored under `analysis/runtime/vrdof117-{result,pixels,scenario,normal-close,save-restored}.json` and `vrdof117-native-status.txt`.

This fixes the observed simulator pause pattern. General menu coverage, projection accuracy, physical tracking/headset presentation, mounted gameplay and long-duration continuity remain unaccepted. Defaults and release packages have not been promoted; `headset_verified=false` and `release_ready=false` remain in the result.

---

## 中文

# Native67 替代 DOF 四进制并暂停恢复

2026-10-17 实际117 已使用原生67 source `f55f71313df32c96e588f658b7d4caf76eea2777`,DLLSHA(英语:DLLSHA)256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`游戏 PID10656 届会 `continuous-stereo-10656-20261017-195136-53668504`。测试的暂停模式现在在模拟器中起作用;这不接受所有的菜单或耳机演示文稿。

## 调用人更正背后的证据

In [Actual116](140-vrdof66-checkpoint-pause-regression.md)实际数 `gpu_output_quad` 返回时计数 `d808d8` were 5698 每只眼睛, 匹配缺失的方块前证明。 保存的历史函数 `140d7fd14` 包含一个常见的设置器,然后是相互排斥的四重调用 `d808d8` and `d80a7b`。这不是一个特定分辨率分支,也不是它是一个挂载游戏分支的证据。

被忽略的( 被忽略的) `alternate-dof-branch-attestation.json` 将分析保存在历史文物上的束缚1 双胞胎沙拉256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`模块基数 `0x140000000`,和伪码 SHA256 `a5cf75e88d0d204574958c55bb6158364c664d7594293caab7629c0c7b6eddcb`。没有打开开发协会的数据库,仅凭这些历史身份并不能证明当前的代码。

Native67 明确接受使用未修改的预方证书和一个共享的每眼保留区来准确的四位呼叫者。没有仅设置或NoSite绕行。调用者验证从 96 字节到全 1819- 字节范围在 `d80360`, 莎莎 (英语).256 `e7bdb0107d1243e27ccdfc2c24e2a477b209ca1e31c3a78ec9e3489ee5423997`; 编号 200- 字节设置检查还剩,实际117 已校验全程运行 `match=1`测试保留未知地点、重复/跨地点重复、单眼和缺方拒绝。

## Actual117 observations

The `d808d8` 生产场地 3943 电话和每只眼睛的成功证明; `d80a7b` 已保存的允许收件保存世界1/ticket4616、安装器和四面罩3,和成功的呼叫对 `d808d8,d808d8`完成正常化 4614 转让时无错误; 已到达本地出版物 4609.

两个暂停条目都产生了成功的 XR `eye_menu_first_frame` 收件日期:时代1/ticket287 和时代2/ticket1333,每个样本时间都比菜单输入的最小时间晚。1 恢复游戏播放投影。 GPU short2 (`c7ae7d21320144bf81df618cef960545d8bafd78c9e83c60094902be0582ed08`)在两半边都包含完全重复的暂停 UI,根据协调的视觉审查。Snapshot3 (`cda4af5b1754ed03f70d2598cabf6cb07c305867bce985bccee5588ea8033a0f`)记录恢复游戏。 2534×1009.

暂停图像大约是 93–94% 黑色分数反映了其黑色菜单背景; 集合黑暗并不是整个眼睛是空白的证据。 常见的眼菜单面板显然不是立体游戏。 同样, XR 大约 62.8- 第二大投影-提交缺口包括暂停的目镜:只提交投影计数器省略了该面板。 62 黑色显示秒。

## 清理和剩余验收

原生清理、条目恢复、有序XR关闭、正常游戏关闭、所有自有进程退出和恢复四个保存的配置文件都得到核实。 `analysis/runtime/vrdof117-{result,pixels,scenario,normal-close,save-restored}.json` and `vrdof117-native-status.txt`.

这可以修正所观察到的模拟暂停模式。 一般菜单覆盖、投影精度、物理跟踪/头集演示、挂载游戏播放和长期持续性仍未被接受。默认和发布包尚未推广 ; `headset_verified=false` and `release_ready=false` 仍为结果。
