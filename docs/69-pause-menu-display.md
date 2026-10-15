# Pause 菜单显示：共同菜单面板与恢复边界

2026-10-16。SpaceRecovery7 已实现严格识别的 Pause 菜单共同面板；CPU 检查通过，实际游戏验证另记于下。未实现或宣称原生 mono 输出边界。

## 实际观察

quiet9k 的 game6680 出现暗画面时，实际显示 Pause / Resume Game，而非已证实的场景全黑。只读 MenuReader 一致复读得到 `DeltaPause-Campaign-Host / WarUIScene`、input_mode=1、player_index=0、input_block_count=0，焦点为 `LBtnResumeSingle / UILabelButton`（Enabled、Focused）。现有 reader 不输出独立 scene_flags。

对应 menu.bin 读取时为 schema1、mode0（UNKNOWN）、sequence927、年龄约225ms；game/owner 的 PID与创建时间匹配本会话。monitor 日志同样记录 `mode0 / DeltaPause-Campaign-Host`。会话证据前缀：`analysis/runtime/continuous-stereo-6680-20261016-001324-ce0a104f`。

协调线程记录一次严格认证后的后台 Enter，见 `analysis/runtime/quiet9k-resume.json`。随后本次只读复读保存于 `analysis/runtime/quiet9k-after-resume-menu.json`：仍为相同 Pause tag/class、player0及 Resume 焦点，但 top 已从 `0x3f5333180 / number1` 变成 `0x162b87a180 / number2`。不能据此宣称已持续恢复 gameplay，也不能仅据此断定是未退出还是退出后重新进入 Pause。本次复读没有发送输入。

## 改动前的源码链路

- `tools/native-capture/menu_display_monitor.py:23–30` 只识别 DeltaGameover 菜单及已观察的非模态 gameplay 场景；Pause 落为 UNKNOWN。
- `tools/openxr-probe/startup_panel_policy.h:19–23` 在首次交接后允许 UNKNOWN 继续 projection；只有现有 death_menu 模式显示菜单quad并禁止projection。
- `tools/native-capture/run_stereo_continuous.py:442–453` 每250ms发布菜单状态。Native9没有消费该菜单协议。`tools/openxr-probe/pose_probe.cpp:455–460` 按XR session状态发布 focused/visible/should_render，472行才读取菜单mode；现有菜单模式不会暂停native SBS。
- `tools/openxr-probe/game_screen.h:220–226` 抓取完整客户区，388–390行把整图作为双眼共同quad；没有单眼crop。若native仍在生成SBS，仅把Pause分类成菜单会把两个菜单一起放进quad。

## 曾考虑的原生 mono 路线，未接线

可复用的临时回退是：在严格认证的菜单显示意图下发布新序列、`should_render=0` 的sample。`tools/vr-common/stereo_frame.h:109` 会返回inactive；`tools/native-capture/capture.cpp:1909–1913` 不因inactive永久停止连续会话，2100行回退原始renderer。仅清focused不足，因为visible也允许渲染。启动诊断项 submit_mono_control 与当前view_union/output_bounds组合互斥（capture.cpp:3131–3133），不应直接动态切换。

**inactive只是拒绝本次双目候选，不是原生ack，更不是原始renderer完成、GPU完成或全幅mono Present的证明。** 必须先设计并验证同一菜单epoch下的真实mono输出完成边界，再让WGC开始接受该边界之后的菜单帧；单纯重置WGC时间戳仍可能采到队列中的旧SBS。恢复gameplay还需现有身份、时间边界及新双目票据守卫。未建立这套边界前，不把Pause直接映射为DEATH_MENU，也不将所有UNKNOWN视作菜单。

## SpaceRecovery7 实现

采用已经完成 GPU 复制并释放的左眼交换链作为双眼共同的 2D quad。沿用原接收器，原生 SBS 继续工作；不引入第二个接收器，不依赖 WGC 截取可能仍为 SBS 的窗口。此面板不计入 stereo/projection 成功，也不代表已实现菜单背景的双目深度。

- MenuDisplayMonitor 增加数值3 `PAUSE_MENU`，只接受实际观察过的 `DeltaPause-Campaign-Host / WarUIScene / input_mode1 / player0`，保留一致复读、只读、无输入阻塞与进程创建身份校验。不同焦点选项仍属于同一 Pause 场景；未推广到未知子菜单。64字节 schema1 不变，旧 XR validator 会将3拒绝为UNKNOWN。
- 菜单 quad 取左眼完整图像 `(0,0,source_width/2,source_height)`，双眼共同显示，宽1.6米、距离2米，高按原游戏窗口宽高比计算。普通 Canvas 的X坐标在 native replay 中缩为一半，因此用 source aspect 还原其形状，而非眼纹理的宽高比。认证HUD有另一套映射，不将此比例规则推广到所有UI。
- 入菜单与恢复均要求新边界之后的已完成图像票据，同时保留 session、origin、年龄、尺寸和资源重建校验。UNKNOWN 不越过待恢复状态；仅认证GAMEPLAY加新票据允许恢复投影。
- `eye_menu_first_frame` 只在 `xrEndFrame` 成功后记录，共同面板计数与投影计数分开。原生构建仍为 GameRecovery9；新增实验清单 `manifest-game-recovery9-window3.json`，默认清单未提升。

冻结 XR7 SHA256：`59220f534b389d51667fa988b6cd236a7ee35261114af11b2ad4537d5198cf2c`。

验证：全量编译成功；直接执行 MenuDisplayStateTest、StartupPanelPolicyTest、MenuEyePanelTest 和 `--projection-receiver-self-test` 均退出0，Python menu monitor 12项通过。CMake 当前未注册 CTest，因此没有把 CTest 的“无测试”当成通过。另完成一次独立窄范围静态审查，未发现确定阻塞问题。以上不等于真实菜单视觉或头显验收。

## 9l 实际运行

会话前缀 `analysis/runtime/continuous-stereo-35456-20261016-004600-51750857`；game35456/创建FILETIME `134365563390630203`，owner34892/创建 `134365563322509862`，Native9 + XR7 + quiet2 + quiet5。首个投影在运行器开始后32.603秒，初始化延迟尚未解决。90秒观察结束后仍持续运行，观察器不会自动停止游戏。

实际记录3次 Pause 菜单进入、3次共同面板成功提交、2次返回投影。每次首面板均为960×1080左眼纹理、1.6×0.9米quad，票据sample_ns晚于该菜单边界；实际模拟器截图显示双眼相同完整Pause菜单、恢复16:9外形。人工头显舒适度及所有菜单尚未验收。

协调线程尝试打开菜单前的状态守卫发现游戏已经Pause，因此没有发送Escape；随后只在认证Resume按钮后发送一次后台Enter（`pause-menu9l-open.json`、`pause-menu9l-resume.json`）。日志确认返回投影，但很快再次Pause；临时采样器的旧registry失效，生产monitor重建reader后识别到新Pause对象，未把这一段当成持续gameplay通过。用户随后明确反馈失焦仍自动暂停，下一项针对失焦暂停路径调查。

模拟器检测器的 `MISSING_PROJECTION_LAYER` 在本轮包含有意提交菜单quad的阶段，不能将累计值直接当成黑屏次数。上述画面检查只有一次截图；其余检查依据menu状态、票据、xrEndFrame及进程身份日志。

## 9m 暂停/恢复复验

修正quiet启动器的开关引用（见文档68）后，game35136的同一后台运行中：认证GAMEPLAY→一次Escape→认证Pause/共同菜单quad→认证Resume的一次Enter→新票据投影。恢复后30秒60次状态均为后台GAMEPLAY，未再自动进入Pause；WorldInfo.Pauser为空且游戏时间推进。此轮无需截图或抢回焦点。证据见 `analysis/runtime/pause-menu9m-{open,resume}.json`、`unfocus9m-after-resume.json` 与 `continuous-stereo-35136-20261016-011605-c5356ffd.stdout.txt`。
