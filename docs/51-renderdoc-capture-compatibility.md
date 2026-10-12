# RenderDoc 接入验证

日期：2026-10-12。目标是直接查看双视图的右侧输入纹理与最终绘制；本次没有拿到游戏帧。

## 实际结果

- 官方 RenderDoc 1.46 便携版已解压在 J 盘，DLL 签名 Valid。
- 新增官方 replay API 单帧控制工具和独立最小 D3D12 清屏程序，均编译成功。
- 游戏旧 PID 16996 正常退出，从 RenderDoc 启动新的 PID 33108；模块枚举确认官方路径的 renderdoc.dll 已载入。
- 按用户确认的办法连续回车约 10 秒，之后遥测显示镜头时间更新。没有逐页截图。
- RenderDoc 控制连接 ID 38920 对应游戏 PID 33108，但没有注册图形 API。一次请求等待结束，未生成 .rdc；随后加强工具检查，无 API 时不再请求捕获。
- 最小程序 PID 47536 注册 D3D12，presenting=1、supported=1。单帧 783 捕获成功，文件 1,524,351 字节。
- 从该 .rdc 成功导出 320×240 缩略图，中心 RGB=(20,64,127)，与程序清屏色 (0.08,0.25,0.5) 的 8 位结果一致。
- 游戏静态导入表的只读检查没有找到 D3D/DXGI 项。它不覆盖动态设备创建，不能单独定位接入失败原因。
- DLSS/FSR2/FSR3/MFSR 的实际设置仍全部为 False。

## 推论及下一步

最小程序证明此安装、官方控制接口和基本 D3D12 帧捕获可工作。游戏只出现“DLL 已载入”，没有图形 API 注册，因此不能宣称 RenderDoc 已接管游戏设备。
日志有 Hook 注册信息但没有游戏 D3D12 设备注册证据；动态创建方式或运行时导入重建可能影响接入，这是待验证假设。

下一步检查游戏设备创建入口和 RenderDoc 接入时机；若不能可靠接管，改用另一种 GPU 调试接入方式。不要反复对无 API 的连接发送捕获请求。
本次没有启用原生双视图诊断，不能改变第 50 份记录中“最终调用参数正确、右侧图像仍不正确”的结论。

## 本机证据

- `analysis/renderdoc/smoke_frame783.rdc`
- `analysis/renderdoc/smoke_frame783.png`
- `analysis/renderdoc/game-33108-renderdoc.log`
- `analysis/renderdoc/game-33108-imports.json`

清理了路径已核验的模拟器 synthetic_env_server PID 40820，它占用约 24.15GB 私有内存。
它与本轮游戏帧捕获无关；未重新启动模拟器。
