# VDXR startup anchor reacquisition

2026-10-16: actual Quest 3 / VDXR run, game PID27748, session `43c31988e9004f16b4a3dc5d7fc24f89`, Native23 / XR18-vd. Source report: ignored `analysis/runtime/continuous-stereo-27748-20261016-180049-43c31988.json`; native-start log and failure receipts retained alongside the build evidence.

## Observed failure

The level camera was accepted 16.662 seconds after runner start. Native preparation ran its self-tests (4.982 seconds), retried a cross-frame sample, then reacquired after UI registry churn. Its final anchor assembly guard rejected `Object registry changed since resolution`. Native activation did not complete: zero stereo copies and zero projection layers; 6740 startup quad layers remained visible. The runner subsequently requested XR stop, and XR stopped and destroyed its objects. This explains the flat-screen persistence and subsequent VR exit in this run. It does not establish the cause of game stuttering.

The user separately reports foreground game startup avoids the desktop black image; moving the window had also restored the image. Window presentation/redraw remains a hypothesis, not a proven cause.

## Change

The exact strict registry comparison now emits a typed mismatch with the expected and observed valid header. Anchor wrappers preserve this evidence. Before activation, the owner discards that Resolver and all partial anchor/UI certificates and repeats complete acquisition using the existing bounded attempt/deadline policy. It never activates with the rejected certificate. Process/module/player/code/identity failures and untyped matching error text are not admitted as registry churn. The shared outer preparation deadline still applies.

## Validation and remaining work

60 targeted native-capture tests, 8 base anchor tests and 12 cinematic-anchor tests pass. Coverage includes successful fresh reacquisition, exhaustion/deadlines and continued rejection of identity failures. No game/headset rerun is claimed for this change. Next real run must demonstrate native activation plus nonzero stereo copies/projection layers. Startup delay and stuttering remain to measure after this failure is removed.

---

## 中文

# VDXR 启动锚重购

2026-10-16: 实际查询 3 / VDXR 运行, 游戏 PID27748 届会 `43c31988e9004f16b4a3dc5d7fc24f89`原生23 / XR 函数18-vd. 源报告: 忽略 `analysis/runtime/continuous-stereo-27748-20261016-180049-43c31988.json`; 与构建证据一起保留原生启动记录和故障收据。

## 已观察到失败

摄像头被接受 16.662 启动后秒。 原生准备会进行自我测试( NAME OF TRANSLATORS) 。4.982 ,以二舍五入计数,再测试一个跨帧样本,然后在UI注册churn之后重新获得。 `Object registry changed since resolution`原生激活未完成:零立体复制和零投影层; 6740 启动四层仍然可见。 跑者随后请求 XR 停止, XR 停止并摧毁了它的对象。 这解释了平屏持续和随后 VR 退出在此运行的原因。 它不能确定游戏扭曲的原因 。

用户单独报告前景游戏启动可以避免桌面黑图像;移动窗口也恢复了图像. Window 演示/重排仍然是假说,而不是一个经过证明的原因.

## Change

精确的注册比较现在显示与预期和观察到的有效头目不符。 Anchor 包件保存了此证据。 在激活前, 所有人丢弃了解析器和所有部分锚/UI 证书并重复完成 使用已存在的边框的尝试/ 死线策略获取。 它从不与被拒绝的证书一起激活。 进程/ 模块/ 播放器/ 代码/ 身份识别失败和未打字匹配错误文本不被录入为注册 churn。 共享的外部准备dea dline 仍然适用。

## 鉴定和剩余工作

60 有针对性的本土采集测试, 8 基准锚试验和 12 电影- 锁定测试通过。 覆盖范围包括成功重新获取、 疲惫/ 死线和持续拒绝身份失败。 没有声称游戏/ 头套重播会改变。 下一次真实运行必须显示本地激活加非零 s 立方体复制/投影层。 启动延迟和断层在故障被移除后仍需要测量 。
