# VR Start Return: UI set caches were wrongly rejected

2026-10-21, the user feedback integration first person is not able to access VR after preview. Checking the real startup log, OpenXR prechecked and original self-check verified; UI configuration before active UI profile dropped out `UIConfigRejected: UI assembly identity limit exceeded`. Failed game PID 44044 Counterpart `analysis/runtime/continuous-stereo-44044-20261021-160309-9dd3ba96.native-start.log`. The old start log or other process count is not used for attribution.

The machine gun has been re-enacted to optimize. `ui_observer_config.py` Add subassembly cache, maximum 256 bar. The UI function finds that many members of the class who will not eventually write to a certificate can be more than scanned 256 And the final certificate is still far less than it was.96 The old code wrongly invalidates the optional cache as a certificate, thus suspending startup before first person is enabled, right-hand-bar strategy and VR rendering. The previous small-scale synthesis input did not cover this condition and cannot replace this player with its result.

Fixing to keep cache caps 256, the cached object continues to be reused; the new object is fully read in real time after full amount and no more dropping abnormality. Final object/register/name, parent, script, attribute and entry review is retained, final certificate 96 An identity restriction is also maintained. No changes to the native/XR construction, no cancellation of first person ' s name, dead area or other machine gun restoration changes are carried out; and the new Python subprocesses for ordinary starters are directly used for repair.

Add 300 Synthetic level structure of unselected functions, where the original error is accurately repeated in a private memory version; final after repair 52 The configuration and integrity of the bar identity is exactly the same as the disabled cache path. The cache remains 256 Article,347 Read back in real time; changed identity at the final border after full cache is still rejected.247 Item Python returns pass, as evidenced by `analysis/runtime/ui-cache-startup-regression-20261021.json`。

The actual normal startup of the repeat is using default lists, current user configuration and archive, run by standard backstage test portals without any private additions to the front wheel for the field 160 Second delay. Use validated bend-windows start/ emulator, observe-only, test silent sound and protect current archive and settings; no system OpenXR settings have been modified.43256 / creation_time 134370440511479300 New session via previously failed UI assembly, cache 256 Article and actual occurrence 1743 Repeatedly, native is activated.

The round is normal.180 Second observation; after the show 90.38 Seconds.8136 Second Bibliography,7948 A new image of the secondary is published.0,Hook Independent Access is back to pass. Audio, dead zone.5 The location data, archive and settings are restored; no legacy test games are available at the end.1 Sub-cam cross frame sampling failed, navigation appears 1 The movement response is inadequate, and the two are retained as test facilities and navigation problems respectively, not attributable to the start-up repair failure. This results in a physical game+ emulator start/submission/cleanup validation that does not replace physical header acceptance.

Full running record:`analysis/playtest/vr-startup-ui-cache-fix1-20261021/report.json`、`analysis/runtime/continuous-stereo-43256-20261021-161439-1446b6ef.json`. Final UI Source SHA 256 `a720e454b1150625d547edc323dcda88541cc99ab6f4fa48d9cb1ae9e7ff5708`Default native continues to be the first person that has been integrated `045ca20a…b9d`。

---

## 中文

# VR 启动回归：UI 组装缓存满后误拒绝

2026-10-21，用户反馈集成第一人称预览后无法进入 VR。检查该次真实启动的日志，OpenXR 预检通过，原生自检证明通过；native 激活前的 UI 配置组装抛出 `UIConfigRejected: UI assembly identity limit exceeded`。失败游戏 PID44044，对应 `analysis/runtime/continuous-stereo-44044-20261021-160309-9dd3ba96.native-start.log`。不是把旧启动日志或其他进程计数用于归因。

近期机枪恢复优化在 `ui_observer_config.py` 增加同次组装身份缓存，最多256条。UI 函数发现会遍历许多最终不会写进证书的类成员，因此扫描对象数可以超过256，而最终证书仍远少于原有96条身份限制。旧代码错误地把可选缓存装满当成证书无效，从而在启用第一人称、右摇杆策略和 VR 渲染之前中止启动。前一轮小规模合成输入未覆盖这个条件，不能用其通过结果替代此次玩家启动。

修复保持缓存上限256，已缓存对象继续复用；新对象在满额后执行原有完整实时双读，不再抛容量异常。最终对象/注册槽/名称、父类、脚本、属性和入口复核均保留，最终证书96条身份限制也保留。不更换 native/XR 构建，不撤销第一人称、死区或其他机枪恢复改动；普通启动器的新 Python 子进程直接使用修复。

新增300个未选中函数的合成关卡结构，原错误在私有内存版本中准确复现；修复后最终52条身份的配置及完整 INI 与禁用缓存路径完全相等。缓存保持256条，347次读取回退实时路径；缓存满后在最后边界改变已选身份仍会被拒绝。相关247项 Python 回归通过，证据为 `analysis/runtime/ui-cache-startup-regression-20261021.json`。

实际正常启动复测使用默认清单、当前用户配置与存档，由标准后台测试入口运行，没有前轮为过场添加的私有160秒延迟。使用已验证 behind-windows 启动/模拟器、observe-only、测试静音并保护当前存档和设置；没有改系统 OpenXR 设置。PID43256 / creation_time134370440511479300 的新会话通过此前失败的 UI 组装，缓存256条并实际发生1743次回退读取，native 已激活。

该轮正常完成180秒观察；过场后约90.38秒有8136次双目提交、7948次原生新图发布。native/XR/owner 均退出0，Hook 独立入口恢复验证通过，音频、死区5处数据、存档与设置均恢复；结束时没有遗留测试游戏。观察器出现1次相机跨帧采样失败、导航出现1项移动响应不足，两者分别保留为测试设施和导航问题，没有归因为本启动修复故障。该结果是实际游戏＋模拟器的启动/提交/清理验证，不代替物理头显验收。

完整运行记录：`analysis/playtest/vr-startup-ui-cache-fix1-20261021/report.json`、`analysis/runtime/continuous-stereo-43256-20261021-161439-1446b6ef.json`。最终 UI 源码 SHA256 `a720e454b1150625d547edc323dcda88541cc99ab6f4fa48d9cb1ae9e7ff5708`，默认 native 继续为已集成第一人称的 `045ca20a…b9d`。
