# VRDof63 final-transfer geometry investigation

## Candidate identity and scope

Source revision `e095c5e` adds one bounded crosshair activation failure-stage report. It changes neither admission checks nor retries. Current-source MSVC assignment, weapon-binding and INI-loader CPU tests passed. The normal feature profile is retained; scene-root/call-boundary diagnostics stay off. DLL `tools/native-capture/build-vrdof63/GearsCaptureVrdof63.dll` SHA256 `70035760f30b406c24cba0d41b456e961238b52333679e691611b7815db6b337`. Frozen source/options/vendor receipt: ignored `analysis/runtime/vrdof63-build-source.json`. Startup uses the full 22-test same-invocation suite. Root default and existing release ZIP remain unchanged.

## Actual PID41364: physical window correction did not change DXGI output

Session `continuous-stereo-41364-20261017-185342-b585024b`. Native preparation and activation succeeded. An identity-checked, per-monitor-v2-aware helper measured client 2534x1009, changed only owned window geometry to client 2560x1080 without activation, observed for 25 seconds, then restored the original outer rectangle and observed another 10 seconds. Foreground HWND stayed `789028` at both actions. No input helper was enabled.

All phase output samples remained 2534x1009 and publication remained zero. Two actual ResizeBuffers calls occurred near restoration; both requested and returned 2534x1009, not the temporary client size. Thus this experiment did not establish a successful matched-size DXGI control. A Win32 client rectangle is not proof of render-target size.

Native final report: 2048 transfer attempts, all policy-rejected, no paired publication. The late quad reports destination 2534x1009, source 2560x1080 and source rectangle approximately `(-76.154541, 0, 2712.30908, 1080)`. That apparent sampling extent exceeds source width. Exact padding/cropping/sampler behavior has not been proven by pixels; the earlier shorthand “crop” must not be treated as a decoded shader conclusion.

Native cleanup, entry restoration, XR destruction, normal owned-game exit and four-profile restore were verified. Evidence: ignored `vrdof111-{audit,normal-close,save-restored}.json`, `vrdof111-geometry-experiment.py` and the session files. The geometry test completed with no helper error, but display acceptance failed.

## Actual PID41244: saved resolution was not an effective control

Session `continuous-stereo-41244-20261017-190009-645e35fa`. Before launch, both user-settings resolution keys were backed up and temporarily set to 1920x1080. Native activation succeeded; 30 seconds of guarded status sampling still showed actual DXGI 2534x1009 and zero publication. Final native report records 1892 transfer attempts, all rejected; source and late quad geometry stayed the same as above. No GPU snapshot was requested because publication never began. This is not a passed 1080p rendering test.

After native/entry/XR cleanup and normal game close, all four saves and the two original resolution keys were restored, preserving other current settings. A transient process-exit check initially refused the settings write; the subsequent authoritative process check found no game, and restoration then succeeded. Evidence: ignored `vrdof112-{audit,normal-close,save-restored,settings-backup,settings-restored}.json` and raw before/after settings backups. These were temporary diagnostics, not a return of the removed Low/High resolution policy.

## Required next proof

An independent source review confirmed that accepting arbitrary native padding/cropping is unsafe: `frame_link::draw_transfer` synthesizes full-height destination eye halves and the frozen ticket retains the original projection. A permissive size predicate would label altered sampling as full eyes. Capturing a transient intermediate also requires state, resource/allocation lifetime, queue ordering and complete UI coverage proofs.

The smaller candidate route intentionally normalizes only an authenticated final transfer to full-source SBS sampling. Existing native TLS access can obtain the current command list before `original_quad`, but lazy target/list/SRV changes inside that call remain unproven. Next collect a bounded observation-only entry-to-actual-draw bracket (same list/generation, source, target, rectangles, complete current-ticket pair and one expected draw). Do not modify sampling or enable broader publication until that evidence is available. GPU pixel and headset checks remain separate.

Still open for V1: general output sizing, intermittent startup binding rejection, mounted black output, fork/UI placement, recoil ring alignment, continuity/performance and updated headset acceptance.

---

## 中文

# VRDof63 最后转让几何学调查

## 候选人身份和范围

源码订正 `e095c5e` 添加一个有限制的十字气动故障级报告。 它既不改变录入检查, 也不重复。 当前源的 MSVC 任务、 装订武器和 INI 装填器 CPU 测试通过。 正常特性描述保存; 场景根/ 呼叫边界 diagn 骨骼别乱动,掉下来 `tools/native-capture/build-vrdof63/GearsCaptureVrdof63.dll` SHA256 `70035760f30b406c24cba0d41b456e961238b52333679e691611b7815db6b337`. 冻结源/选项/收到报文:忽略 `analysis/runtime/vrdof63-build-source.json`。启动时使用全 22-测试同一调用套件。根默认值和现有的版本 ZIP 保持不变。

## 实际的 PID41364: 物理窗口校正没有更改 DXGI 输出

Session `continuous-stereo-41364-20261017-185342-b585024b`. 原生准备和激活成功。2- 意识到帮助对象测量客户端 2534x1009,只将自有窗口几何修改为客户端 2560x1080 ,不激活,观测到 25 秒,然后恢复原外矩形,并观察到另一个 10 秒。预览 HWND 停留 `789028` 。没有启用输入帮助。

所有阶段产出样本都保留下来 2534x1009 两次实际的调整车站呼叫接近恢复;既要求恢复,又要求恢复 2534x1009,而不是临时客户端大小。因此,此实验没有建立成功的匹配大小 DXGI 控制。 A Win32 客户端矩形不是渲染目标大小的证据。

本地最后报告 : 2048 传输尝试,所有政策拒绝,没有对齐的发布。 2534x1009 源代码 2560x1080 和源矩形约 `(-76.154541, 0, 2712.30908, 1080)`。这种明显的采样范围超过了源宽度。精确的粘贴/剪切/取样行为尚未用像素来证明;早期的简写“剪切”不得作为解码的遮蔽结论。

原生清理、入口恢复、XR销毁、正常自有游戏退出和四大显示恢复等都得到核实。 `vrdof111-{audit,normal-close,save-restored}.json`, `vrdof111-geometry-experiment.py` 和会话文件。几何测试没有帮助错误,但显示接受失败。

## 实际的 PID41244: 保存分辨率不是有效的控制

Session `continuous-stereo-41244-20261017-190009-645e35fa`。在启动前,两个用户设置解析键都备份并临时设置到 1920x1080 当地激活成功; 30 观察状态取样秒数仍显示为实际 DXGI 2534x1009 最终本土报告记录 1892 传输尝试, 全部被拒绝; 源和后四几何保持了上述状态。 没有请求 GPU 抓图, 因为发布从未开始 。 这不是一个通过 。 1080 p渲染测试.

在本地/ 输入/ XR 清理和普通游戏关闭后, 全部四个保存和两个原始解析键被恢复, 保留其他当前设置。 一次瞬态进程- 退出检查最初拒绝设置写入; 后续的权威进程 检查找不到游戏, 然后恢复成功。 证据: 忽略 `vrdof112-{audit,normal-close,save-restored,settings-backup,settings-restored}.json` 和设置后原始备份。这些是临时诊断,而不是返回已删除的低分辨率/高分辨率策略。

## 需要下一个证明

独立来源审查证实,接受任意的本土栽培/作物是不安全的: `frame_link::draw_transfer` 将全高的目的地眼量减半,冷冻票保留原投影。允许大小的上游会将改变的采样标记为全眼。 队列命令并完成 UI 覆盖证明 。

较小的候选路径故意使只有经过认证的最终传输到全源 SBS 采样中。 现有的本地 TLS 访问权限可以在当前命令列表之前获得 。 `original_quad`,但该调用内部懒惰的目标/列表/SRV更改仍未证实。接下来收集一个限定的仅观察条目到实际绘制的括号(相同的列表/生成、源、目标、矩形、完整的电流计对和一个预期的绘图)。运行或允许更广泛的发布,直到有证据为止。 GPU 像素和头盔检查仍然分开。

仍然开放V1:一般输出大小,间歇启动装订拒绝,挂载黑输出,叉/UI布置,后坐力环对齐,连续性/性能和更新耳机接受.
