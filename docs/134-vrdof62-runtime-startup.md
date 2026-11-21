# VRDof62 runtime startup regression

The user authorizes unattended background game and simulator tests at any time, whether present or away. Keep game/simulator startup nonactivating and physical-controller background input disabled. Do not repeat the permission question. The simulator may remain visible. This authorization was persisted in local AGENTS.md.

## First actual attempt: PID17640

Used explicit `analysis/runtime/manifest-vrdof62.json`, the quiet2 simulator runtime, and `--auto-continue`, without `--foreground`. All 22 same-invocation startup self-tests passed. The state-based navigator dispatched five verified menu actions and a stable level camera was observed. No scene stereo activation was completed.

The native preparation failed with `IndexError: tuple index out of range` in `Resolver.name()`, called from UI owner-class discovery. The anchor sampler refreshes the live name count/header; the discovery pointer tuple remains the original snapshot. A name index can pass the live count bound while exceeding that tuple. This is a demonstrated preparation failure, not evidence that this explains mounted-weapon black output or every previous startup failure.

The owned prime was cancelled, entry restoration independently verified, the owned game normally closed, and all four saved profile files restored with hashes checked. The report correctly retains `native_started=false` and `native_cleanup_verified=false`, rather than claiming a successful VR lifecycle.

Evidence (ignored): `analysis/runtime/continuous-stereo-17640-20261017-180938-edade379.*`, `vrdof108-audit.json`, `vrdof108-normal-close.json`, `vrdof108-save-restored.json`; backup `analysis/savebackups/vrdof108-before-chapter/manifest.json`.

## Source correction

`Resolver.name()` now bounds snapshot reads using the tuple length. For a newly added name outside that snapshot, it validates a bounded current registry header and slot, the entry index and strict UTF-16, and verifies the header before caching. An observed header change raises typed `DiscoveryRegistryChanged`; UI discovery propagates it and preparation discards all certificates within its existing retry/deadline budget. Corrupt names, unexpected exceptions and target identities remain rejected. No native DLL, rendering route, deadline or retry count changed.

Validation: 14 resolver name/discovery tests, 31 UI tests and 29 startup anchor tests passed (74 total). The actual follow-up is recorded separately below; these CPU checks do not establish GPU/headset behavior.

## Follow-up: PID7124

After the Python correction, actual native preparation completed (certificate acquisition 2286.5424 ms; activation 517.9055 ms). No name indexing exception recurred. This supports the corrected preparation path, not a successful stereo display: XR received zero scene copies/submitted frames and the scheduled death-menu/checkpoint test timed out.

The final native report records 37358 accepted stereo tickets but zero publication/capture attempts. All 37368 final-transfer attempts were policy-rejected; source texture was 2560x1080 while presented buffers were 2534x1009, and the transfer arguments included a horizontal crop. The current strict final-transfer policy supports equal widths and at most one extra destination row, so this geometry cannot pass. This is a demonstrated reason for this run's no-publication; a subsequent controlled geometry test is required before assigning a broader root cause.

`frame_link_compare kind=first anchor=1 reason_mask=8` is an accepted initial reanchor, not a binding fault: binding rejected=0, changed=0 and anchors=1. The nonactivating tiny-output recovery completed before activation.

Native cleanup, entry restoration and XR object destruction were verified; the owned game was normally closed and all four profiles restored. Evidence: `continuous-stereo-7124-20261017-181337-3ae20545.*`, `vrdof109-{audit,live-status,normal-close,save-restored}.json` under ignored `analysis/runtime/`. The existing development ZIP predates the Python fix and has not been promoted.

## Geometry experiment blocked before activation: PID10756

A follow-up using the same candidate acquired certificates in 2239.7163 ms, then native startup rejected `crosshair observer requires compiled support and fresh assignment/tick bindings`. No geometry adjustment or GPU snapshot was performed. The existing crosshair safety check was not disabled. The generic native rejection still needs separation of compiled support, assignment and tick validation before attributing this intermittent startup failure. Prime cancellation/entry restoration, normal owned-game exit and four-profile restoration were verified. Evidence: `continuous-stereo-10756-20261017-182746-6ba84fa0.*`, `vrdof110-startup-{audit,normal-close,save-restored}.json`.

Next: isolate that rejection with existing binding evidence; complete the reversible, nonactivating window-geometry experiment without weakening transfer or UI admission checks. Mounted black output, fork UI placement, recoil ring alignment and new headset acceptance remain open.

---

## 中文

# VRDof62 运行时间启动回归

用户授权在任何时间, 无论是在现场还是不在场, 都进行未入读的背景游戏和模拟测试。 保留游戏/ 模拟器启动时不激活, 并禁止物理控制器背景输入。 不要重复权限问题。 模拟器可能重新出镜 。可见。这种授权在当地的AGENTS.md中一直存在。

## 第一次实际尝试: PID17640

使用明 `analysis/runtime/manifest-vrdof62.json`安静2 模拟运行时间,以及 `--auto-continue`,不包含 `--foreground`全部 22 同一调用启动自测试通过。基于状态的导航器发送了5个经核实的菜单动作,并观察到一个稳定的电平相机。没有完成场景立体激活。

本地准备失败 `IndexError: tuple index out of range` in `Resolver.name()`,从 UI 主人级发现调用。锚点取样器刷新了活名计数/头数;发现指针拖曳仍然是原始快照。一个名称索引可以在超过拖曳时通过活名计数。这是演示的预留 n 失败,而不是证明这解释了挂载武器黑输出或之前每次启动失败的原因.

取消了自有质素, 独立核查了条目恢复, 通常关闭自有游戏, 并检查了所有四个保存的配置文件。 报告正确保留了 。 `native_started=false` and `native_cleanup_verified=false`而不是声称一个成功的 VR 生命周期。

证据(已忽略): `analysis/runtime/continuous-stereo-17640-20261017-180938-edade379.*`, `vrdof108-audit.json`, `vrdof108-normal-close.json`, `vrdof108-save-restored.json`备份 `analysis/savebackups/vrdof108-before-chapter/manifest.json`.

## 源更正

`Resolver.name()` 现在, 绑定快照读取使用 Tuple 长度。 对于在快照之外新添加的名称, 它验证一个有边框的当前注册页头和槽、 条目索引和严格的 UTF-16,并在缓存前验证头部。一个观察到头部变化会提高键入 `DiscoveryRegistryChanged`; 用户界面发现传播并准备丢弃其现有重试/死线预算范围内的所有证书。 腐败名称、 意外例外和目标身份仍然被拒绝。 没有更改本地 DLL 、 使路径、 截止日期或重试计数 。

Validation: 14 解析器名称/发现测试, 31 UI测试和 29 启动锚测试通过( Q)74 ) 实际后续记录如下;这些CPU检查没有建立GPU/headset行为.

## 后续行动:PID7124

Python校正后,实际本土准备完成(证书的获取) 2286.5424 ms; 激活 517.9055 ms). 没有名称索引例外重现。此选项支持校正的准备路径, 而不是成功的立体显示 : XR 收到零场景副本/ 提交帧, 以及预定的死亡菜单/ 检查点测试超时 。

最终本土报告记录 37358 已接受立体声罚单,但未进行出版/抓取。 37368 最后转让尝试被政策拒绝;源纹理是 2560x1080 缓冲器是 2534x1009,而传输参数包括横向切换。当前严格的最终传输策略支持等宽度,最多可多一个目标行,因此此几何学无法通过。这是该切换不公开的明显原因;子序列 在分配更广泛的根源之前,需要进行nt控制几何测试。

`frame_link_compare kind=first anchor=1 reason_mask=8` 被接受的初始重排者,而不是具有约束力的过错: 具有约束力的拒绝=0,更改=0 和锚=1. 启动前已完成的不激活微量输出回收。

原生清理、条目恢复和XR对象销毁都经过了核实;自有游戏通常关闭,所有四个配置图都恢复。 `continuous-stereo-7124-20261017-181337-3ae20545.*`, `vrdof109-{audit,live-status,normal-close,save-restored}.json` 忽略未计 `analysis/runtime/`. 现有开发的ZIP早于Python定律,没有被推广.

## 激活前几何实验被封杀: PID10756

利用同一候选人在下列地点获得证书的后续行动 2239.7163 ms, 然后拒绝本地启动 `crosshair observer requires compiled support and fresh assignment/tick bindings`。没有进行几何调整或 GPU shraw。现有的横线安全检查没有被禁用。一般本地拒绝在判定此间歇启动f 之前,仍然需要将已编译的支援、任务和勾选验证分开。已核实了主机取消/进入恢复、正常自有游戏退出和四大功能恢复。 `continuous-stereo-10756-20261017-182746-6ba84fa0.*`, `vrdof110-startup-{audit,normal-close,save-restored}.json`.

下一步: 用现有约束性证据隔离拒绝; 在不削弱传输或UI 输入检查的情况下完成可逆、不可激活的窗口几何实验。 挂载了黑输出、 叉 UI 放置、 后座环对齐和新头盔 cc 保持开放。
