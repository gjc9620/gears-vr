# DLSS 手动入口：关卡启动后立即结束 VR

2026-10-20。本页记录启动修复版本 `GearsCaptureNgxManual2.dll`，SHA256 `25c414e8ca46da30340f907367956bb87f8142161c279133450c58fd0fb1b66e`。后续独立入口已更新为保留此修复的 NgxManual3，见[性能优化与对照](dlss-descriptor-copy-performance-20261020.md)。使用前关闭旧测试启动器，重新双击根目录 `GearsVR DLSS Test.cmd`；旧 GUI 保留创建时的私有 DLL/清单副本。启动修复验证时普通默认入口为 StartupOutput5。

## 这次实际失败

用户粘贴的18:21日志对应 `continuous-stereo-36508-20261020-182152-42feda7d`，实际 DLL 为 NgxManual1，SHA256 `0cbe380e5cd222926bd42472ccad6506e3533adf33253ee826da2d579bdc8983`。不能用普通入口 StartupOutput5 的通过结论代替此测试版的验收。

native 成功激活，但 published0、projection0、owned_error24；首次 UI fault 为 none，DLSS output publication rejected1。Output 退出诊断记录 flag4、无有效绑定和完成证明。两个 native 输出步骤 d83cbf/2b11a0 已出现，实际目标分别为中间 R10 和 owned RGBA8，几何均为当前6144×3264半幅；本次拒绝点是额外 DLSS 输出证明，不是上一轮 HUD 的 draw_state。

## 修复

owned-scene DLSS join 的 SDK/native-gap 输出证明来自 owned DOF pass。普通 Output 仍会在原生启动及后续渲染中执行，使用另一种绑定，不能参与同一证明记录。

`dlss_output_control.inl` 只在编译启用 owned-scene join 时，为普通 Output 建立没有证明 key 的局部 scope。它正常调用原函数，内部 setter/quad 不再污染 SDK/native-gap 记录，也不能借用外层证明。离开时恢复先前 TLS scope。owned DOF 的身份、绑定、setter、quad、双眼完整性与 SDK 发布检查不变。普通旧传输的控制逻辑仍由独立测试覆盖。

回归直接覆盖普通 flag4/flag8 pass：无几何改动、无 setter/bind 接管、原生可发布而 SDK 不可凭空发布；完整 owned SDK 两眼后的普通 pass 不得污染证明，不完整 SDK 单眼也不能被它补齐。原有 native-gap、错绑定、目标/epoch/代码拒绝与异常检查继续通过。

## 验证

冻结源码相对 NgxManual1 的生产变化仅上述 scope 隔离，另有一份回归测试变化。全新 verified build 通过22项启动自检、31项针对检查、8项附加检查，以及4项独立启动脚本测试。9个启动 DLL/EXE、GUI与VDXR文件预检通过；未打开或操作 IDA 数据库，没有安装工具。

实际后台游戏会话 `continuous-stereo-10880-20261020-182914-c6411533`，回执 `analysis/runtime/fps-resolution-720p-ngx-manual2-startup1-20261020.json`：

- 保持用户渲染比例1.0，匹配 VDXR 推荐尺寸的模拟器，每眼3072×3264、GPU源6144×3264，桌面1280×720。
- 30个有效遥测样本，32.6159099秒立体投影；515对新画面，3908次投影提交，最长提交间隔85.3554ms。
- owned_error0、首次 UI fault none、DLSS output publication rejected0；517次普通原生发布检查均为 no_site，不伪造 SDK 完成证明。
- 接收端GPU回读 ticket155、serial154，左右眼均有完整关卡与角色，两眼不同像素99.9410%。PPM SHA256 `8b47937476f9b278ecb11544d8c8d8f81b63fb120d6aed8a99faf160946f18ef`，已目视检查。
- native清理、XR/owner退出0、游戏正常退出、设置逐字节恢复通过。后台使用已认证置后窗口、只读 tiny-output、临时静音与禁用物理手柄后台输入。

本轮验证范围是 **DLSS测试入口先以 SR 关闭状态进入关卡并持续出图**。没有在此次启动回归中再次实际开启 DLSS，也未做物理头显显示或跨世界验收；这些不能由上述结果推定。已有 owned68 开关实验与本轮验收分开记录。

详细冻结源码、构建、图片和测试资料均在 ignored analysis/。汇总回执 `analysis/runtime/ngx-manual2-startup-acceptance.json`；旧模板保存为 `ngx-manual2-previous-template.json`。没有生成发布包。
