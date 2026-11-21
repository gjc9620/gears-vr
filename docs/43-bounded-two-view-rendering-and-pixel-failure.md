# BRT: Life cycle passed, image not yet passed

2026-10-12. The original single-person level, the first known as Mod, is maintained. The original livelihood number of this document cannot be accepted as desktop or head-to-head.

## Ongoing submission and release

Add Fixed 64 The caller's rendering is registered before entry and the original destruction returns. After closing production, all registered objects are released from service for an additional status Release right once. The cross-rest lock protects only tanks and numbers and does not surround their original calls.

Right state is reused throughout the cycle, and left side maintains the game status. Each frame of two source views is destroyed after the original renderinger has its own copy. The insufficient capacity or duplicate address refuses to register and returns; does not release any status that has been submitted for use earlier. Unknown structures or unempted objects are kept to process exits and the analysis cannot pass.

Most continuation mode 60 Seconds, more time to collect than test. 10 seconds. Start when you get the focus; lose focus, F 8 If the connection is stopped, the clean-up cannot be declared complete.

## Observed records

- PID 25528：5 Seconds,974 Yes; consumption, preparation, destruction 974;pool closing, pending decommissioning 0 Reservations 0 Skip 0 Original CSV 973 Two-view tectonic records, with the rest of the life cycle recorded separately 974. The focus of the two-view construction of this cycle is on 0, the user indicates that the image appears to be vague and has not been received through pixels.
- PID 44980 Focus Control 5 Seconds,932 Yes; equal 932;CSV clean-up and release. 932 Each of the structures has a game focus. Prepare for recording. 931. Users consider observation time too short to require one minute.
- PID 312: Autostart and continue from the save level,60 Seconds,10,241 Yes; consumption, preparation, destruction 10,241. CSV Construct Range 59.9918364 Seconds,10,237 All of the structures have a focus. The pool is closed and awaiting decommissioning. 0 Reservations 0 Skip 0;state new to release and delete observations.13,032 The first failed without a record,11 One link entrance is restored.

Local evidence is the same. `analysis/runtime/submit-duration-25528.*`、`submit-focus-44980.*`、`submit-minute-312.*`; continuous submission of analyser passes. CSV reports precise coverage, as the ring buffer may be lost in small quantities, and does not declare each record to be complete.

## Actual pixel failed

WCC binding game HWD, collecting 1920×1080 Client area and shrink to 640×360 Check. Normal images exist before and after testing; intermediate images are clearly magnified/cuted, narrow bars appear on the left side, and the subject does not display two full level views as expected.**See the failure of double view acceptance.**

Local picture:`analysis/game-io/frames/20261012-152954-449bbf1c.jpg`(testing started off with normal footage)`20261012-153010-3208c603.jpg`(Abnormal in test)`20261012-153102-17d2a969.jpg`(taken after testing). Pictures are not submitted to Git.

The source four floating point fields recorded during the duration are `(640,360,640,360)`. The game window collection source is 1920×1080, the values cannot be interpreted as a direct result of showing the final area or extrapolating the resolution change based on background focus. The original structure is still initiated. `+58/+5c/+60/+64` ; the field changes from the original view to the rendering copy need to be tracked and the target area, zoom in and reprocessing.

## Validation tool and next steps

`SubmittedViewLifetimeTest` The submitted analyser has six tests covering unreleased status, lack of life cycle, error PID, failure of Present, refusal not to be considered a success and unempted/reserved/early ending continuous rotation.

One minute for collection. `collect_submit_capture.py` Fluent statistics;`analyze_submit_duration.py` flow through CSV, loading long test records into memory at no time.

Next, fix the area and sample link to re-validize two complete images. Then calibrate the left and right eye projection, world unit/IPD, access OpenXR double eye texture. This is still not a fully deployable whole. 6DoF VR Mod。

---

## 中文

# 有界持续双视图：生命周期通过，画面尚未通过

2026-10-12。原版单人关卡，第一人称 Mod 保持恢复。不能把本文的原生计数通过当作桌面或头显双眼验收通过。

## 持续提交与释放

新增固定 64 槽跟踪器：调用者提供的渲染对象在入队前登记，原生销毁返回后退役。关闭生产后，所有已登记对象退役才授予一次新增状态 Release 权利。互斥锁仅保护槽与计数，不包围原生调用。

右侧状态在整轮中复用，左侧保留游戏原状态。每帧两份源视图在原生渲染器持有自己的副本后销毁。容量不足或重复地址拒绝登记并回退；不提前释放任何仍由已提交对象使用的状态。未知结构或未清空对象保留至进程退出，分析不能通过。

持续模式最多 60 秒，采集时长至少比测试多 10 秒。获得游戏焦点后开始；失焦、F8 或到期关闭新增提交。停止挂钩时若还有待退役对象，不能宣称清理完成。

## 已观察记录

- PID 25528：5 秒，974 对；消费、准备、销毁均 974；池关闭、待退役 0、保留 0、跳过 0。原始 CSV 973 个双视图构造记录，其余生命周期记录各 974。该轮双视图构造的焦点均为 0，用户表示画面似乎模糊，未通过像素验收。
- PID 44980：焦点控制版 5 秒，932 对；计数均 932；池清空、状态释放。CSV 932 个构造均有游戏焦点，准备记录 931。用户认为观察时间太短，要求一分钟。
- PID 312：自动启动并从保存关卡继续，60 秒，10,241 对；消费、准备、销毁各 10,241。CSV 构造跨度 59.9918364 秒，10,237 个构造全部有焦点。池关闭、待退役 0、保留 0、跳过 0；新增状态完成 Release 和删除观察。13,032 次 Present 无记录到的失败，11 个挂钩入口恢复。

本地证据分别为 `analysis/runtime/submit-duration-25528.*`、`submit-focus-44980.*`、`submit-minute-312.*`；持续提交分析器通过。CSV 因环形缓冲可能少量丢失，报告精确覆盖数，不宣称每条记录完整。

## 实際像素失败

用户授权助手自行查看窗口后，WGC 绑定游戏 HWND、采集 1920×1080 客户区并缩至 640×360 检查。正常画面在测试前后存在；测试中途图像明显放大／裁切，左侧出现窄条，主体没有按期望显示两个完整的关卡视图。**可见双视图验收失败。**

本地图片：`analysis/game-io/frames/20261012-152954-449bbf1c.jpg`（测试开始附近正常画面）、`20261012-153010-3208c603.jpg`（测试中异常）、`20261012-153102-17d2a969.jpg`（测试后采集）。图片不提交 Git。

持续期间记录的源四浮点字段为 `(640,360,640,360)`。游戏窗口采集源是 1920×1080，不能把这四个值直接解释为最终显示区域，也不能仅根据背景焦点推断分辨率变化。原生完整构造还初始化 `+58/+5c/+60/+64` 的整数目标区域，并计算相关采样比例；需要追踪原视图到渲染副本的区域变化，核对目标区域、缩放与后处理。

## 验证工具与下一步

`SubmittedViewLifetimeTest` 覆盖容量、重复/复用地址、关闭时仍有对象、并发退役和一次释放权利。提交分析器共有六项测试，覆盖未释放状态、缺少生命周期、错误 PID、Present 失败、拒绝不得当作成功，以及未清空／保留／提前结束的持续轮次。

一分钟采集使用 `collect_submit_capture.py` 流式统计；`analyze_submit_duration.py` 流式遍历 CSV，不一次把长测试记录加载到内存。

下一步修正区域与采样链路，重新验证两个完整画面。其后再校准左右眼投影、世界单位/IPD，接入 OpenXR 双眼纹理。当前仍不是可部署的完整 6DoF VR Mod。
