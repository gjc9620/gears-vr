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
