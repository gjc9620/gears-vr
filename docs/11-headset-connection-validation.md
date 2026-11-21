# Headline connection and position validation log

2026-10-11 22:45–22:48, KRVR XR Runtime, Apple Vision Pro.
User reports connected and conducts a turn-over peace transfer test as hint; no game is started or modified during this round.

| Test | Total frame | Complete VALID/TRACKED | Frames for non-default and complete tracking | The only position. |
| --- | ---: | ---: | ---: | ---: |
| 3 Second connection check | 254 | 254 | Not separate | 11 |
| 30 Second head motion | 2681 | 304 | 48 | 9 |
| 20 Second-Eye Test Map | 1796 | 1796 | 0 | 1 |

30 The first non-default position appears in the second test. 2.839 Second, last position changes.
3.072 sec.3.560 Only VALID sign after seconds, TRACKED disappears; maximum continuous untraceed
About 26.428 sec. The appointment is still received while running 90 Hz shows feedback, but the number of position samples stops 37。
Therefore, a small amount of non-default data cannot be considered as continuous head-to-head validation.

20 Second test diagram submitted 1796 A program player, GPU back-reading and checking consistent with previous tests.
Head data is all default zero position/four-digit, and feedback is shown on run-time 0/s, headline cadence is 0Hz。
Users then clearly responded “sleeping out”. No confirmation of head-over visible test maps was received.
TRACKED signs remain intact on this wheel, indicating that the default attitude report will be fully tracked when it is running;
Producer access must be accompanied by a separate recognition of the true connection and cannot rely solely on these markers.

Original evidence is in Git ignored analysis/openxr：

- pose-20261011-224557-063389...csv and corresponding logs;
- pose-20261011-224624-591855...csv and corresponding logs;
- pose-20261011-224755-535444...csv, left-eye GPU back and corresponding log.

Add non-default and tracked frames to the packaging, with maximum untraceed time periods; no automatic physical head validation will be claimed.
User then chooses to change to Quest 3; revalidation when using a combination of the connection and readiness.
Found VDXR and J:/Steam Next.
The subsequent test subprocess -runtime parameter selection does not require modification of the global OpenXR registration form.

## Quest 3 / Virgin Desktop: validated

2026-10-11 22:59–23:03, with the test subprocess XR_RUNTIME_JSON
VirtualDesktopXR 1.0.10. The system is still running at KRVR, without changing the registration form.
Query returns XR_ERRO_FORM_FACTOR_UNAVAILABLE before connection; user connects in headline
System query successful after Virtual Desktop, name Meta Quest 3。
I recommend every eye. 3072×3264, rotation/location tracking support is true; testing remains only 512×512。

First round 30 sec:3547 Frame, all tracked and all positions different but should Render only 971 Frame,
FOCUSED 970 frame. User requests to retest, not using this round as a display of acceptance and inspection.

Second round 30 sec:3585 Frames, all positions limited, complete VALID/TRACKED, non-default and different;
3585 The frame should be Render,3584 The FOCUSED framework,3585 Submits a project player.
Pressed predicted DisplayTime 30.021 seconds,119.38 Subposition sample/sec.
This is not a real head start showing frame rate measurements, nor is it an original sensor sampling rate.

Head coordinates range (m): X [-]0.07862,0.07314]、Y [-0.12833,0.05069]、
Z [-0.05759,0.13615. The maximum rotation relative to the front frame 69.36°The distance between the eyes and the right 66.61mm。
GPU, read back and check left and right images as the test described above.

User confirmed " See, finish, picture continuous" and added "no problem".
Therefore, acceptance is shown by entering and head-higher test maps for independent head rotation/diversion.
No axle-by-axis symbols, game scales or virtual scene depths strictly verified; positional access to games, first person location,
The real eye viewing remains to be realized.

Second round of original evidence:analysis/openxr/pose-20261011-230207-719422.csv The blog, and its corresponding logs,
Left-and-right eye GPU back reading; confirming statistics as Quest 3-confird-validation.json. Both keep their machines, Git ignores them.

---

## 中文

# 头显连接与位姿验证记录

2026-10-11 22:45–22:48，默认运行时 KRVR XR Runtime，Apple Vision Pro。
用户报告已连接，并按提示进行转头和平移测试；本轮没有启动或修改游戏。

| 测试 | 总帧数 | 完整 VALID/TRACKED | 非默认且完整跟踪的帧 | 唯一位姿 |
| --- | ---: | ---: | ---: | ---: |
| 3 秒连接检查 | 254 | 254 | 未单列 | 11 |
| 30 秒头部运动 | 2681 | 304 | 48 | 9 |
| 20 秒双眼测试图 | 1796 | 1796 | 0 | 1 |

30 秒测试中，首个非默认位姿出现在约 2.839 秒，最后一次位姿变化在约
3.072 秒。3.560 秒后只剩 VALID 标志，TRACKED 消失；最长连续未跟踪
约 26.428 秒。运行时仍收到约 90Hz 的显示反馈，但位姿样本数停在 37。
因此不能将少量非默认数据视为持续头动验证通过。

20 秒测试图提交 1796 个 projection layer，GPU 回读校验和与之前测试一致。
头部数据全为默认零位置/单位四元数，运行时显示反馈为 0/s、头显 cadence 为 0Hz。
用户随后明确反馈“头显睡眠了”。没有收到头显端可见测试图的确认。
TRACKED 标志在这轮仍完整，说明本机运行时会对默认姿态报告完整跟踪标志；
生产者接入时必须另行确认真实连接，不能仅依赖这些标志。

原始证据位于 Git 忽略的 analysis/openxr：

- pose-20261011-224557-063389.csv 及对应日志；
- pose-20261011-224624-591855.csv 及对应日志；
- pose-20261011-224755-535444.csv、左右眼 GPU 回读和对应日志。

包装器新增非默认且跟踪的帧数、最长未跟踪时段统计；仍不会自动宣称物理头动验证通过。
用户随后选择改用 Quest 3；等待连接方式和准备状态后，用其对应运行时重新验证。
已找到本机 Virtual Desktop 的 VDXR 和 J:/Steam 下的 SteamVR manifest。
后续可用测试子进程的 --runtime 参数选择，无需修改全局 OpenXR 注册表。

## Quest 3 / Virtual Desktop：已验证

2026-10-11 22:59–23:03，通过测试子进程的 XR_RUNTIME_JSON 使用
VirtualDesktopXR 1.0.10。系统默认运行时仍为 KRVR，未改注册表。
连接前查询返回 XR_ERROR_FORM_FACTOR_UNAVAILABLE；用户在头显里连接
Virtual Desktop 后，系统查询成功，名称 Meta Quest 3。
推荐每眼 3072×3264，旋转/位置跟踪支持均为 true；测试仍仅用 512×512。

第一轮 30 秒：3547 帧，全部跟踪且全部位姿不同，但 shouldRender 仅 971 帧，
FOCUSED 970 帧。用户要求再测，未把这轮作为显示验收。

第二轮 30 秒：3585 帧，全部位姿有限、完整 VALID/TRACKED、非默认且不同；
3585 帧 shouldRender，3584 帧 FOCUSED，3585 个 projection layer 提交。
按 predictedDisplayTime 统计约 30.021 秒、119.38 次位姿采样/秒。
这不是实际头显显示帧率测量，也不是原始传感器采样率。

头部坐标范围（米）：X [-0.07862,0.07314]、Y [-0.12833,0.05069]、
Z [-0.05759,0.13615]。相对首帧最大旋转约 69.36°，左右眼间距约 66.61mm。
GPU 回读左右图像校验和与前述测试相同。

用户确认“看到了，完成动作，画面连续”，随后补充“没有问题”。
因此通过独立头部旋转/平移输入与头显端测试图显示验收。
未严格验证逐轴符号、游戏尺度或虚拟场景深度；游戏位姿接入、第一人称位置、
真实双眼场景渲染仍待实现。

第二轮原始证据：analysis/openxr/pose-20261011-230207-719422.csv 及对应日志、
左右眼 GPU 回读；确认统计为 quest3-confirmed-validation.json。均保留本机，Git 忽略。
