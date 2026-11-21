# The star-limited target is back and back when the position is off.

2026-10-20 User feedback is sometimes inconsistent with the bullet position and suspects a sit-in bias. This round fixes an indication defect that has been identified by code and can be recreated mathematically: when a limited world target exceeds any eye view, the protometric projection is rejected, then retraces in endless directions, loss of head liding and close to target vision. It cannot be assumed that all deviations from user feedback are caused by this branch.

## Modify

`ui_reticle_direction_plan.h` Allows the active, immediate target to project beyond sight and continues to be mapped peacefully at a limited world point by cutting each eye through a scissor. One-eye crossing does not change another eye to different aim benchmarks. Target is behind eyes, values are invalid or above D.3 The Dview range is a new one.`display_plan` Draws two empty scissor for this parastar, keeps the drawing and status recovery process and avoids displaying an infinity replacement of a protostar that is unrelated to the limited target.

`ui_payload_observer.inl` Use this display policy to empty after removing failed `finite_aim` Re-scan branch. Logs keep compatible fields `direction_fallback=0`, other `finite_suppressed`. The original direction strategy is followed when no limited target exists; this round does not claim to resolve all time-series problems between the missing HUD query or cache and the current camera.

The change only handles astrometric mapping, without modifying the gun, shooting, collision, injury, dispersal or auxiliary targeting. The limit point is still from a certified HUD query, not the final Impact of a real fire. [Weapons are on the line.](weapon-aim-hit-chain.md)。

## Authentication

Add independent world point projection, using `project_ui_anchor` Calculates the expected pixels per eye, and compares the actual star viewing map of the source centre:

- 729 group position, rotation and target distance combination;horizontal ±0.4 m, vertical ±0.15 m, front and back ±0.25m，yaw ±0.5rad、roll ±0.35 Rad, target depth. 60/200/800 Game units.
- Asymmetrical Eyes FOV,64mm IPD、20 (a) Close the game unit;192 The second one-eyed target is still on the right map of the world.18 The group targets are hidden behind your eyes, avoiding a retreat.
- Single-eyes are passed across the edge, limited input anomalies, old direction patterns and invalid mapping; former scissor, sub-star, world origin and wide-highby-turn continue to pass.
- 22 Self-testing is initiated as necessary; there are also limited inquiries, camera binding, D 3 Focus tests for D ABI, world rebind/epoch, owned scene peteline, Mounted are passed. 26,608 Check.

The test results are CPU/Mathematics and Hometop DLL self-measured without proof of real bullet drop, GPU edge pixels or physical header experience. Backstage games return to the PID that already exists before startup 10784, exit with the protection already in place; no changes to the game, no restarts, no input sent, no test settings written.`actual_game_regression=false`、`headset_verified=false`、`release_ready=false`。

## This institution is built and the portal

Independent Build `build-seated-reticle1/GearsCaptureSeatedReticle1.dll`，SHA 256 `adde3b7484fde70dca516c068a108cc7111237389b44da3ce39f77777f228410`. From default MountedFps 1 . Compiling records to confirm the DLL use of modified files.

`tools/vr-launcher/manifest.json` & With Named List `manifest-seated-reticle1.json` It's pointing to the new edition, XR. Continue with the old PauseStereo 1;from repository root directory after normal closing of current game `GearsVR Launcher.exe` Reactivate game with VR. Current loaded DLLs keep running. Old MountedFps 1 Build and name the list to be maintained.

The logs for build, freeze source code, test and early failure checkback, original list and unexecuted game return are all in signed `analysis/`：`seated-reticle1-source/`、`native-build-seated-reticle1/`、`runtime/seated-reticle1-tests.json`、`seated-reticle1-stage.json`、`seated-reticle1-previous-manifest.json`、`fps-resolution-4k-seated-reticle1-20261020.json`. No original translation added, no IDA opened, no ZIP published.

---

## 中文

# 坐姿偏移时的准星有限目标回退修复

2026-10-20。用户反馈准星有时与子弹位置不一致，并怀疑坐姿偏斜。本轮修复一个已从代码确认、可由数学回归复现的显示缺陷：有限世界目标超出任一眼的视野时，准星投影被拒绝，随后重试无穷远方向，丢失头部平移及近目标双眼视差。不能据此认定用户反馈的全部偏差均由这一分支造成。

## 修改

`ui_reticle_direction_plan.h` 允许有效、位于眼前的目标投影到视野外，继续使用同一有限世界点和平移映射，通过每眼 scissor 裁切。单眼越界不再使另一眼切换到不同的瞄准基准。目标位于眼后、数值无效或超出 D3D viewport 范围时，`display_plan` 为该次准星绘制生成两个空 scissor，保留绘制和状态恢复过程，避免显示一个与有限目标无关的无穷远替代准星。

`ui_payload_observer.inl` 使用这条显示策略，移除失败后清空 `finite_aim` 再投影的分支。日志保留兼容字段 `direction_fallback=0`，另计 `finite_suppressed`。没有有限目标时仍沿用原有方向策略；本轮未声称解决 HUD 查询缺失或缓存与当前相机之间的全部时序问题。

该修改只处理准星绘制，不修改枪口、射击、碰撞、伤害、散布或辅助瞄准。有限点仍来自已认证的 HUD 查询，而不是一次真实开火的最终 Impact。真实射击链与两者区别见 [武器瞄准与命中链](weapon-aim-hit-chain.md)。

## 验证

新增独立世界点投影对照，使用 `project_ui_anchor` 计算每眼预期像素，再比较实际准星 viewport 对源中心的映射：

- 729 组位置、旋转和目标距离组合；横向 ±0.4m、竖向 ±0.15m、前后 ±0.25m，yaw ±0.5rad、roll ±0.35rad，目标深度 60/200/800 游戏单位。
- 不对称双眼 FOV、64mm IPD、20 游戏单位镜头拉近；192 次单眼目标越界仍保持正确世界点映射，18 组目标在眼后时隐藏，避免方向回退。
- 单眼刚越过边缘、有限输入异常、原有方向模式和无效绘制状态拒绝均通过；原有 scissor、分段准星、世界原点及宽高比回归继续通过。
- 22 次必要启动自测通过；另有有限查询、相机绑定、D3D ABI、world rebind/epoch、owned scene pipeline、mounted 等聚焦测试通过。FrameLinkTest 为 26,608 项检查。

测试结果是 CPU/数学与本机 DLL 自测，未证明真实子弹落点、GPU 边缘像素或物理头显体验。后台游戏回归在启动前检测到已有游戏 PID10784，按既有保护退出；未更改该游戏、未重启、未发送输入，未进入测试设置写入阶段。`actual_game_regression=false`、`headset_verified=false`、`release_ready=false`。

## 本机构建与入口

独立构建 `build-seated-reticle1/GearsCaptureSeatedReticle1.dll`，SHA256 `adde3b7484fde70dca516c068a108cc7111237389b44da3ce39f77777f228410`。从默认 MountedFps1 的冻结源码复制，仅替换上述两份实现及准星测试，未混入其他正在开发的 native 功能。编译依赖记录确认 DLL 使用修改后的两个文件。

`tools/vr-launcher/manifest.json` 与命名清单 `manifest-seated-reticle1.json` 已指向新版，XR 继续使用原有 PauseStereo1；正常关闭当前游戏后，从仓库根目录 `GearsVR Launcher.exe` 重新启动游戏与 VR 生效。当前已加载的 DLL 保持运行。旧的 MountedFps1 构建及命名清单保留。

构建、冻结源码、测试与早期失败回执、原清单和未执行游戏回归的记录均在 ignored `analysis/`：`seated-reticle1-source/`、`native-build-seated-reticle1/`、`runtime/seated-reticle1-tests.json`、`seated-reticle1-stage.json`、`seated-reticle1-previous-manifest.json`、`fps-resolution-4k-seated-reticle1-20261020.json`。未新增原生反编译、未打开 IDA、未生成发布 ZIP。
