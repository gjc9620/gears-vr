# 暂停菜单保留原生双眼输出

2026-10-19。用户反馈暂停菜单呈现为扁平画面。

## 原因与修复

原 XR 暂停分支 `GameGpuScreen::render_eye_menu` 从已经完成的双眼图像中只选取 `eyes[0]`，作为 `XR_EYE_VISIBILITY_BOTH` 的 VIEW-space Quad 提交。菜单和游戏背景一同进入共同平面，原始右眼画面和场景视差在这一分支被丢弃。这是现有 Mod 源码确定的显示路径，无需新增游戏原生逆向。

用户进一步确认同时存在文字/图标比例压扁。当前原生 `capture.cpp` 的 owned Canvas 分支已经将菜单 glyph 绘到同一票据的头锁定 HUD 平面。旧 Quad 的 `menu_eye_panel.h` 仍按“普通 Canvas 半宽重放”还原 source aspect，对已经投影的菜单进行了第二次比例转换。在方形眼纹理、对称 FOV 的可解析例子中，原本等长的 UI X/Y 被变为 2:1；该问题与丢失右眼同属旧暂停分支。直接保留原生投影同时消除这次错误转换。

暂停分支改为 `render_pause_menu`，通过现有 `render_projection` 使用同一张合格票据的两个独立 eye swapchain、对应 pose、FOV 和裁切矩形。没有重新合成眼位或使用最新姿态替换图像票据内的姿态。后台原生发布、世界代次、资源代次、reference-space、图像时效及 GPU fence 门禁继续由原路径核验。

进入暂停时仍要求晚于菜单 epoch 的新图像；GPU 复制后再次读取菜单租约，状态过期、变化或 epoch 变化均拒绝提交。暂停成功显示只记录菜单 witness，不调用 gameplay handoff；恢复游戏时仍需晚于恢复边界的新票据。主菜单和死亡菜单继续遵循各自现有显示策略。

新增 `pause_menu_first_frame` 回执仅在 `xrEndFrame` 成功且实际提交两视图 Projection 后记录。`pause_menu_evidence.projection_layers` 单独计数；outage 日志将这一分支标记为 `pause_menu_projection`，避免共同面板与原生双目混淆。

## 验证

独立构建 `tools/openxr-probe/build-pause-stereo1/OpenXRPoseProbe.exe`，SHA256 `afcde6794e004852de0f2965c54cd0a6522136e28a2db3d979d636b783238e1e`。

- 8 项 C++ 检查通过：菜单状态、暂停/恢复策略、窗口 fallback、reference-space epoch、世界交接显示及身份、左右眼裁切策略和实际 D3D12 裁切 fixture。GPU fixture 在 2048/4096 每眼尺寸、三种格式下均无像素不匹配；它不代表实际游戏像素验收。
- 43 项 `test_menu_display_monitor.py` 检查通过。
- XR 自带的 `--projection-receiver-self-test` 与 `--continuous-control-self-test` 通过，未打开 XR 会话。
- 暂停策略新增检查：成功暂停投影不能消除进入菜单边界；失败投影同样保留边界；已显示的暂停菜单进入 gameplay 时必须等待新的恢复图像。
- 新增 `PauseMenuGeometryTest`：72 组源尺寸/源比例、对称/非对称 FOV、40/64/80mm 瞳距组合，通过独立逆视锥计算核验等长 glyph 与有限距离的双眼对齐；并明确断言旧共同 Quad 在解析例子中产生 2 倍横向拉伸。

实际游戏两轮均使用 Native `e6d598f6b90f0eeb9c9b82bc5d10233f2cb4b999b355d6f815104f3bdf92ca5a`、上述 XR、1920×1080 启动设置、render_scale=0.7、Quest 3 非对称 FOV。实际原生输出为 1792×980，每眼 896×980。

| 会话 | 实际结果 |
| --- | --- |
| `continuous-stereo-40340-20261019-195842-eb0b8c2a` | 两次 Pause→Resume，1454 次暂停投影，两个菜单 epoch 均有新票据首次提交和恢复回执。 |
| `continuous-stereo-36164-20261019-200725-1f1ebe71` | 两次 Pause→Resume，1492 次暂停投影；在两个暂停阶段分别取得 GPU fence 后的原始双眼快照。 |

两轮均包含暂停时 yaw12°/pitch4°/roll3°、X位移35mm及Y变化100mm，再恢复中立姿态。原生 cleanup、独立入口恢复及 XR 退出0均通过，未强杀自有游戏。游戏正常关闭消息发出后，进程退出存在延迟；第一轮在下一轮前、第二轮在其他测试释放游戏后完成设置与四个 profile 的原始字节恢复，五文件 SHA256 均核验通过。第二轮音频 guard 日志确认 muted=1、restore_attempted=1、restored=1、restore_pending=0。

第二轮两个原始 GPU 图像均为 1792×980，snapshot1/ticket560 和 snapshot2/ticket1181；配对回执、图像路径和 SHA256 在 `pause-stereo1-verify5-20261019.json`。从原图的红色齿轮图标像素质心、实际每眼 FOV 与64mm眼距独立三角测量，两个样本的 UI 深度均约1.992m，与原生2m平面一致。该测量受光栅化/低分辨率质心误差影响，不是精密距离标定。另生成按配置 UI 平面逆投影的检查图，近圆形图标不再呈2倍横向拉伸；检查图是派生图，不是头显截图。

验证请求显式认证的 behind-windows 游戏启动器与 Simulator Runtime，并设置 `GEARSVR_TEST_WINDOWS_BEHIND=1`、`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`、`GEARSVR_TEST_MUTE_GAME=1`。没有激活、移动、缩放或重排用户窗口，没有启用实体手柄后台输入。两轮各40个前台采样均无自有测试进程成为前台。第一轮窗口遮挡采样0；第二轮40个采样均报告自有窗口与 TaskManagerWindow 的矩形覆盖，故第二轮不作为 behind-windows OS 行为通过的证据，未通过调整/重排用户窗口来掩盖它。该窗口策略缺口与本次暂停图像修复分别保留。

默认 `tools/vr-launcher/manifest.json` 已将 XR 路径和 SHA256 更新为本构建，现有默认 Native 由其他工作更新为 MountedBlack2（`30bb2d31c31ea4b5d193cd55e847bf7e704235824205a0d686d594861a423e7f`），本修复保留该字段。默认所有文件哈希核验通过；上述两轮实际组合的 Native 版本明确为 StereoFPS1，不能把它写成 MountedBlack2 联合运行验证。

本地证据保存在被忽略的 `analysis/runtime/pause-stereo1-*`，包括修改源码及 SHA256、离线结果、实验 manifest、运行脚本与回执。游戏衍生图像和运行数据不进入发布包。真实头显的观感与舒适度仍需独立实测。
