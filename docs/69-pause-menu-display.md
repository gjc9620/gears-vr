# Pause 菜单显示：已定位问题与下一步边界

2026-10-16。以下方案尚未实现，原生 mono 输出边界尚未实际验证。

## 实际观察

quiet9k 的 game6680 出现暗画面时，实际显示 Pause / Resume Game，而非已证实的场景全黑。只读 MenuReader 一致复读得到 `DeltaPause-Campaign-Host / WarUIScene`、input_mode=1、player_index=0、input_block_count=0，焦点为 `LBtnResumeSingle / UILabelButton`（Enabled、Focused）。现有 reader 不输出独立 scene_flags。

对应 menu.bin 读取时为 schema1、mode0（UNKNOWN）、sequence927、年龄约225ms；game/owner 的 PID与创建时间匹配本会话。monitor 日志同样记录 `mode0 / DeltaPause-Campaign-Host`。会话证据前缀：`analysis/runtime/continuous-stereo-6680-20261016-001324-ce0a104f`。

协调线程记录一次严格认证后的后台 Enter，见 `analysis/runtime/quiet9k-resume.json`。随后本次只读复读保存于 `analysis/runtime/quiet9k-after-resume-menu.json`：仍为相同 Pause tag/class、player0及 Resume 焦点，但 top 已从 `0x3f5333180 / number1` 变成 `0x162b87a180 / number2`。不能据此宣称已持续恢复 gameplay，也不能仅据此断定是未退出还是退出后重新进入 Pause。本次复读没有发送输入。

## 当前源码链路

- `tools/native-capture/menu_display_monitor.py:23–30` 只识别 DeltaGameover 菜单及已观察的非模态 gameplay 场景；Pause 落为 UNKNOWN。
- `tools/openxr-probe/startup_panel_policy.h:19–23` 在首次交接后允许 UNKNOWN 继续 projection；只有现有 death_menu 模式显示菜单quad并禁止projection。
- `tools/native-capture/run_stereo_continuous.py:442–453` 每250ms发布菜单状态。Native9没有消费该菜单协议。`tools/openxr-probe/pose_probe.cpp:455–460` 按XR session状态发布 focused/visible/should_render，472行才读取菜单mode；现有菜单模式不会暂停native SBS。
- `tools/openxr-probe/game_screen.h:220–226` 抓取完整客户区，388–390行把整图作为双眼共同quad；没有单眼crop。若native仍在生成SBS，仅把Pause分类成菜单会把两个菜单一起放进quad。

## 下一步方案，尚未接线

可复用的临时回退是：在严格认证的菜单显示意图下发布新序列、`should_render=0` 的sample。`tools/vr-common/stereo_frame.h:109` 会返回inactive；`tools/native-capture/capture.cpp:1909–1913` 不因inactive永久停止连续会话，2100行回退原始renderer。仅清focused不足，因为visible也允许渲染。启动诊断项 submit_mono_control 与当前view_union/output_bounds组合互斥（capture.cpp:3131–3133），不应直接动态切换。

**inactive只是拒绝本次双目候选，不是原生ack，更不是原始renderer完成、GPU完成或全幅mono Present的证明。** 必须先设计并验证同一菜单epoch下的真实mono输出完成边界，再让WGC开始接受该边界之后的菜单帧；单纯重置WGC时间戳仍可能采到队列中的旧SBS。恢复gameplay还需现有身份、时间边界及新双目票据守卫。未建立这套边界前，不把Pause直接映射为DEATH_MENU，也不将所有UNKNOWN视作菜单。
