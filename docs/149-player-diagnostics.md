# Player diagnostics for V1

The bilingual launcher includes **Export logs / 导出日志** beside Open logs. It opens a ZIP save dialog and runs the exporter independently of the VR owner. Export is available during a session, after a normal exit and after an incomplete startup. It does not launch, stop, focus or attach to the game and does not upload anything.

Ask players to save the ZIP soon after an issue and attach it with a description, chapter/checkpoint, headset and connection method. The archive contains a short issue template. A screenshot or headset observation remains useful when the issue is visual; telemetry cannot establish what the player actually saw.

The exporter selects recent owned session metadata, lifecycle/startup/world/menu transitions, bounded recent numeric telemetry, artifact fingerprints, graphics INI settings and selected Windows hardware inventory. It excludes game binaries, native instruction dumps, saves/profiles, screenshots, analysis databases and pseudocode. It filters personal paths, account identifiers and email addresses. Unsafe links, corrupt/partial records and unavailable sources produce omissions/warnings; the remaining report can still be exported.

Native diagnostic timestamps use QPC. Existing bound, top-level DXGI Present observations exclude TEST calls; their rate is not engine-update or physical display FPS. Stereo-pair publication counts new native image publications. XR loop frequency uses sampled frame counts and the runtime's predicted display timestamps; a 90 Hz loop can repeatedly submit a scene produced at a lower rate. Final successful projection averages/gaps are reported separately only when final session evidence exists. First, unchanged, reset or unavailable samples have null rates; measured unchanged counters over an advancing clock can establish zero.

The session recorder samples at most once per second, records state changes, and rotates two backups with a 2 MiB bound per file. It reuses existing counters and introduces no rendering hooks. Sampling write failures disable diagnostics rather than stopping VR. Exports are bounded and atomically written to the selected destination; replacing an existing ZIP requires the save-dialog confirmation or explicit CLI `--overwrite`.

CLI equivalent:

```powershell
python tools/vr-launcher/launcher.py export-logs --output C:\Feedback\GearsVR-Diagnostics.zip
```

Validation receipts, actual-game export samples and portable package verification are kept under ignored `analysis/runtime/` and `analysis/release-zips/`. Simulator tests do not establish physical headset acceptance or readiness of the other V1 features.

## Mounted-camera rejection summaries

World-rebind failures now retain a bounded `guard_failure` receipt through the local event log and exported ZIP. Closed categories distinguish a mounted camera target mismatch, camera timestamp change, registry change, read failure, deadline and unknown cause. The receipt keeps strict-attempt count/timing and separately records whether the observed target was the controlled object, Driver, null or another object, plus the consistency of timestamp and registry reads. Missing observations remain unavailable; a target mismatch alongside changed registry/timestamp is not presented as a stable root cause.

The exporter independently validates the receipt and omits object names/addresses, raw memory, pointer values and free-form exception detail. Integration tests exercise actual mounted and registry rejection exceptions through the monitor, plus malformed evidence and ZIP filtering. No new reads, hooks, retries or admission permissions are introduced. This improves feedback diagnosis; it does not prove the reported mounted black screen is fixed.

---

## 中文

# V 的玩家诊断1

双语发射器包括: **Export logs / 导出日志** 打开日志。 它打开 ZIP 保存对话框, 并独立于 VR 所有人运行导出。 导出在会话中、 正常退出后和未完成的启动后可用。 它不启动、 停止、 聚焦或附加到游戏中, 也不向上l 什么都行,你喜欢什么?

要求玩家在某期发行后很快保存 ZIP, 并附上描述、 章节/ 检查点、 耳机和连接方法。 存档包含一个短的发行模板。 当该期发行时, 截图或耳机观察仍然有用; telem erry无法确定玩家实际看到的东西.

出口商选择最近拥有的会话元数据、生命周期/启动/世界/菜单转换、最近的数字遥测、文物指纹、图形 INI 设置和选定的 Windows 硬件目录。它不包括游戏二进制、本地指令 垃圾堆、保存/配置、截图、分析数据库和伪码。它过滤个人路径、账户标识符和电子邮件地址。不安全链接、腐败/部分记录和无法获取的源头会产生遗漏/警告;其余报告仍然可以 。导出。

原生诊断时间戳使用QPC. 现有绑定,顶级的DXGI 现成观测排除了TEST调用;其速率不是引擎更新或物理显示 FPS. Stereo-pair 出版数新原生图像出版物. XR 循环频率使用样本. 帧计数和运行时间预测显示时间戳; a 90 Hz 环路可以反复提交一个以较低速率生成的场景。 只有在最终会话证据存在时, 才会单独报告最终成功投影平均值/ 差距。 首先, 不变、 重置或无法使用的样本有无效速率; 测量的数值不变 。超时钟可以建立零。

会话记录器样本最多每秒一次,记录状态变化,并用一个旋转的备份旋转两个 2 MiB 绑定每个文件。 它重新使用已有的计数器, 并且不引入渲染钩。 抽样写入失败会禁用诊断而不是停止 VR 。 导出将绑定, 并在解剖时写入到选定的目的地; 替换现有的 ZIP 需要 保存对话框确认或明确的 CLI `--overwrite`.

等效的CLI :

```powershell
python tools/vr-launcher/launcher.py export-logs --output C:\Feedback\GearsVR-Diagnostics.zip
```

验证收据、实际游戏出口样本和便携式包件核查被忽略 `analysis/runtime/` and `analysis/release-zips/`模拟测试不能确定其他V的物理耳机接受或准备状态1 features.

## 挂载的相机拒绝摘要

世界危机的失败现在仍然有限度 `guard_failure` 通过本地事件日志接收并导出 ZIP. 闭合类区分挂载相机目标不匹配,相机时间戳更改,注册更改,读取失败,截止日期和未知原因。收件会严格计数/计时和分隔 e 记录观察到的目标是否为受控对象、驱动器、无效对象或另一个对象,以及时间戳和注册符的一致性。缺少的观测数据仍然无法获取;目标不匹配与更改的注册符/时间戳不是预设的 被作为稳定的根源而缠绕

出口商独立验证收件,并省略对象名称/地址、原始内存、指针值和自由格式例外细节。整合测试通过显示器进行实际挂载和注册拒绝例外,加上错误的证明 ce和 ZIP 过滤。 没有引入新的读取、 钩子、 复写或录用权限。 这可以改善反馈诊断; 它不能证明所报挂载的黑屏幕是固定的 。
