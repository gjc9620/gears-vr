# 不戴头显的 OpenXR 自动验证

2026-10-12。用户休息期间使用 Meta XR Simulator 验证显示通路；工具保存在 J 盘。模拟器不能替代 Quest 3 的延迟、舒适度、真实跟踪、颜色和最终兼容性测试。

## 本机工具

- Meta CLI `1.8.0.17.10`：`J:\gearsVr\tools\third-party\metavr\bin\metavr.exe`。
- Meta XR Simulator `v207.0`：`J:\gearsVr\tools\third-party\meta-xr-simulator\PFiles\MetaXRSimulator\v207.0`。
- 通过官方 CLI 下载 MSI，验证 Authenticode 为 Valid，签名主体 Meta Platforms, Inc.；使用 MSI administrative extraction 解压到 J 盘，没有全局安装、注册系统 OpenXR runtime 或修改 PATH。
- 原先 CLI 默认下载至 C 盘的同一 MSI，核验与 J 盘副本 SHA256 一致后已删除。后续 CLI 使用 `METAVR_CONFIG_DIR=J:\gearsVr\tools\third-party\metavr\state`，安装临时目录也设在 J 盘。
- 软件包、第三方程序、生成配置和原始证据由 `.gitignore` 排除，不进入我们的源代码仓库。

## 机制和复现

`run_simulator.py` 读取原配置，在同目录生成 `gears_automation.json`，开启 batch mode，保留原相对资源路径；仅给测试子进程设置 `META_XRSIM_CONFIG_JSON` 和 `XR_RUNTIME_JSON`。系统默认运行时保持原样。

```powershell
$pythonExe = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 12
# 先正常退出旧游戏，启动新共享通道并进入关卡；一个通道仅允许一个接收端：
& $pythonExe tools/native-capture/launch_binding.py --seconds 180 --gpu-share --build-dir tools/native-capture/build-share-verify
& $pythonExe tools/openxr-probe/run_simulator.py --seconds 30 --gpu-game --gpu-snapshot
```

菜单按现有半自动方案操作，每次确认前读取低清游戏窗口截图。`--gpu-snapshot` 在 OpenXR 接收端运行至少 10 秒后回读一次实际 XR swapchain 图像，保存 640 像素宽 PPM；不经过 WGC 或桌面，正常显示没有每帧 CPU 图像回读。这个快照证明提交纹理内容，不等同于 compositor 最终图像。

## 已完成记录

模拟器实际枚举格式为 `[29,91,10,34,26,40,55,45,20]`，没有游戏的 R10G10B10A2_UNORM（24）。因此增加 GPU shader 转换到 R8G8B8A8_UNORM_SRGB（29）。在已编码 SDR RGB 假设下，shader 先解码 sRGB，RTV 再编码，保持输入 RGB；没有 HDR tone mapping。跨进程已知颜色的 12 帧逐像素核验通过，容差一个 8-bit 数值。此机器没有 D3D12 debug layer，不能声称经过该层验证。

8 秒左右眼测试：565 个有效位姿记录，564 次双眼图层提交；程序退出 0。保存 GPU 图像检查为左红、右蓝和白十字。默认模拟位姿始终静止（unique poses=1），不作为实时头动证据。

首轮游戏 GPU → OpenXR：游戏 PID 34508，30 秒采样，2,130 次 quad 提交、1,112 次纹理复制，退出 0，无每帧 CPU 图像回读。游戏记录 43,236 次 Present，失败 0；发布 1,113 帧，停止后 active=0、资源释放成功，四处 Hook 入口外部读取均恢复。游戏已有第一人称 Mod，1920 × 1080 / 最低设置。该轮包括关卡载入阶段，未保存最终 compositor 图像，不宣称每一帧都是关卡。

证据：`analysis/openxr/pose-20261012-030424-060822.*`、`pose-20261012-031249-996766.*`、`analysis/runtime/gpu-share-34508.{json,jsonl}`。

第二轮独立游戏 PID 17584，进入关卡后 30 秒：2,157 次 quad 提交、1,200 次 GPU 复制，正常退出。一次 XR swapchain 回读保存成功，人工检查为石柱、走廊、烛光，方向正常。游戏发布 1,201 帧，43,877 次 Present、失败 0，停止后四处入口恢复、资源释放成功。快照不是 compositor 最终图像，未经过头显观察。证据：`pose-20261012-031640-107823.*`、`native-gpu-xr-17584.png`、`analysis/runtime/gpu-share-17584.{json,jsonl}`。

封装入口 `run_simulator.py --seconds 8` 已独立运行通过：574 条位姿记录，退出 0。

## 自动位姿与合成图接口

Meta XR Operator v207 已通过官方 CLI 装到 `tools/third-party/metavr/state/tools/meta-xr-operator`。仅在 `run_simulator.py --operator` 子进程开启 OpenXR API layer；没有注册 Codex 全局 MCP。测试脚本通过 localhost:8720 MCP 初始化、发送 initialized 通知，并确认应用名为 GearsVR Pose Probe 后调用接口。拒绝操作已存在的其他 XR 服务。

`simulator_motion_test.py` 会运行 30 秒，依次设置 ±10 cm 左右 / 前后、±5 cm 上下和 ±10° yaw，回中，保存左右眼最终 compositor 图，核对 CSV 的三轴平移和 yaw 范围。全部是人工设置的模拟位姿；XR Operator 返回 valid=true、tracked=false，现有 PoseGate 不会将其视作真实跟踪，不为测试绕过游戏头动保护。

第一次自动测试捕获到服务先于会话就绪，首次设置被拒绝；模拟器清理也异常退出，因此测试失败，没有计为通过。脚本现等待有效会话，识别顶层及内容块错误，结束时关闭 MCP 会话。修正后 `sim-motion-20261012-032340.json` 测试通过：220 个不同姿态、全部目标范围出现、左右眼合成图保存成功、程序退出 0。不将这一次成功认定为已修复所有 runtime 清理异常。

游戏面板组合测试 `sim-motion-20261012-032640.json`（PID 40888）也通过：1,194 次 GPU 复制、2,149 次 quad 提交、位姿范围核对通过、退出 0。人工检查最终左右眼 compositor 图，黑底中的二维面板确实为石柱、走廊和烛光关卡。面板偏小且当前为诊断布局，不评价 VR 视野或舒适度。此测试的头部变化仍为模拟输入，未控制游戏镜头。

```powershell
& $pythonExe tools/openxr-probe/simulator_motion_test.py
# 新游戏 GPU 通道进入关卡后，同样检查游戏二维面板：
& $pythonExe tools/openxr-probe/simulator_motion_test.py --gpu-game
```

## 尚未证明

当前是双眼可见的同一二维面板，完整立体场景尚未渲染；实时头动与显示通路仍为分开的原型。模拟器 CLI 的 MP4 录制尝试返回当前 runtime 不支持，未产出录像。早期无图层测试曾在 runtime 清理阶段异常退出，随后 batch 配置的图层测试正常结束；不把配置改变单独认定为已证明根因。

参考：[Meta XR Simulator 文档](https://developers.meta.com/vr/documentation/native/xrsim-intro/)、[Meta CLI XR 环境命令](https://developers.meta.com/vr/essentials/metavr-environment/)、[官方 Windows 下载](https://developers.meta.com/vr/downloads/package/meta-xr-simulator-windows/)。
