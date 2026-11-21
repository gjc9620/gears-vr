# Native66 checkpoint recovery and pause-menu regression

2026-10-17. Actual116 used unchanged Native66 source `4fcd1af`, DLL SHA256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`. Game PID43000, session `continuous-stereo-43000-20261017-193848-446ca183`. This extends [the ordinary-scene normalization result](139-guarded-final-transfer-normalization.md); it does not establish menu acceptance.

## Recovery succeeded; pause presentation failed

Two checkpoint reloads committed world generations **1→2→3**, and gameplay GPU images were obtained after recovery. The two recorded images are 2534×1009 with nonblank content in both halves. Native normalization recorded 9856 planned, linked and completed transfers, zero failures, and `faulted=0`. Final native publication reached 4152; frame-link inventory changes and contamination were zero.

Recorded XR composition shows long pause intervals with **zero submitted layers**, rather than a visible pause panel. Pause/menu acceptance therefore failed. A single `eye_menu_first_frame` receipt exists for epoch2, but occurred after the second confirm-reload command. It cannot establish that the preceding paused menu was visible. A common menu panel also does not count as stereo gameplay.

## Publication evidence and unresolved cause

`vr_dof_compat` recorded 5698 setter proofs per eye but **zero pre-quad proofs per eye**. It rejected 5698 publications. The saved rejected example is a completed record with `setters=3`, `quads=0`, `failure=0`; successful transfer normalization alone does not satisfy this later publication gate.

The XR eye-menu path requires a fresh native image. Missing publication is consistent with the observed blank pause display, but the general explanation that pre-quad hook coverage is incomplete remains a **hypothesis**. It requires bounded static/current-runtime trace evidence; these counters do not identify the missing native path or prove a normalizer defect.

Movie fallback separately recorded 352 submissions, zero end-frame failures, and two entries/two exits. Those receipts concern movie fallback, not successful pause-menu rendering.

## Timing and cleanup limits

The XR lifecycle spanned approximately 214.92 seconds, with a maximum submission gap of 102379.168 ms that includes deliberate paused-menu dwell. This is **not** evidence of 214 seconds of uninterrupted presentation. The first recovery receipt's approximately 14.8 seconds is delayed sampling to an observed recovery, not first-display latency; no display-latency claim is made here.

Native cleanup, entry restoration, orderly XR shutdown and normal owned-game closure were verified. All owned processes exited and all four saved profiles were restored. The lifecycle receipt's `session_passed=true` does not override the failed pause/menu observation.

Evidence remains ignored under `analysis/runtime/`: `vrdof116-audit.json`, `vrdof116-recovery.json`, `vrdof116-recovery2.json`, `vrdof116-pixels.json`, `vrdof116-native-status.txt`, `vrdof116-normal-close.json`, `vrdof116-save-restored.json`, and the session's stdout/world/menu logs. No default or release promotion follows. UI, projection accuracy, headset, mounted gameplay and long-session continuity remain unaccepted.

---

## 中文

# Native66 恢复检查站和暂停菜单回归

2026-10-17 实际116 原生语言66 source `4fcd1af`,DLLSHA(英语:DLLSHA)256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`游戏 PID43000 届会 `continuous-stereo-43000-20261017-193848-446ca183`。此扩展 [普通场景正常化的结果](139-guarded-final-transfer-normalization.md);它没有确定菜单接受。

## 恢复成功; 暂停演示文稿失败

两次检查站重装,世世代代都承诺 **1→2→3**,而游戏游戏 GPU 图像是在恢复后获得的。所录入的两种图像是 2534×1009 记录了土著正常化情况。 9856 计划、连接和完成的转让,零失败, `faulted=0`. 最终获得的本地出版物 4152;帧链接库存变化和污染为零。

记录的 XR 组成显示长的暂停间隔 **零 提交层**,而不是一个可见的暂停面板。因此,暂停/菜单接受失败。 `eye_menu_first_frame` 时代的收件已存在2,但发生在第二个确认-重新加载命令之后。它不能确定前面的暂停菜单是否可见。一个常见的菜单面板也不算作立体游戏。

## 公布证据和未解决的理由

`vr_dof_compat` recorded 5698 每只眼睛的设置证明,但 **每只眼睛0个方格前的证明**。它拒绝了 5698 出版物。保存的被否决的例子是完成的记录,其中包含 `setters=3`, `quads=0`, `failure=0`光是成功转让正常化并不能满足后来出版的这一大门。

XR 眼菜单路径需要一个新的本土图像。 缺少的发布与观察到的空白暂停显示一致, 但一般解释为 平方钩覆盖不完整 仍然是 **hypothesis**。它需要有限度的静态/当前运行时间的跟踪证据;这些计数器不能识别缺失的本地路径,也不能证明是正常化缺陷。

电影倒计时 352 提交、 零末端框失败和两个条目/ 2 退出。 这些收件涉及电影倒计时, 而非成功的暂停菜单渲染 。

## 时间和清理限度

XR 寿命周期大约为 214.92 秒,最大提交缺口为 102379.168 包括故意暂停菜单的 ms。 **not** 证据: 214 第一次回收收据大约是 14.8 秒被延迟取样到观测到的恢复,而不是第一显示延迟;在此没有显示相关性的主张。

已验证了原生清理、输入恢复、有序XR关闭和正常自有游戏关闭。 所有自有进程都退出, 保存的4个配置文件都恢复。 寿命周期收件 `session_passed=true` 中不覆盖失败的暂停/菜单观察。

证据在下文中仍然被忽视 `analysis/runtime/`: `vrdof116-audit.json`, `vrdof116-recovery.json`, `vrdof116-recovery2.json`, `vrdof116-pixels.json`, `vrdof116-native-status.txt`, `vrdof116-normal-close.json`, `vrdof116-save-restored.json`,以及会话的 stdout/world/菜单日志。没有默认或发布促进。UI、投影精度、耳机、挂载游戏播放和长会话连续性仍未被接受。
