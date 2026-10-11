# 游戏画面进入头显：显示通路原型

2026-10-12。短测、30 秒显示测试及用户头显端确认通过。

新增 run_pose.py --game-screen。Python 复用 game_io 的完整游戏路径核验和
HWND 选择，C++ 再核验窗口/PID/EXE 名；Windows Graphics Capture
CreateForWindow 只捕获目标游戏，不回退到桌面。持续采集复用会话，
只保留最新帧，不写连续截图，不调用电脑操作模型。

1920×1080 客户区裁切后下采样至 960×540，约 15Hz 更新。
原型经 CPU 读回 BGRA，再传入 D3D12 上传缓冲；这不是最终的低延迟 GPU 直连。
OpenXR swapchain 复制前后切换资源状态，GPU fence 完成后释放图像。
第一次上传额外 GPU 回读，逐像素核对 RGB 与不透明 alpha，并保存两份 PPM。

OpenXR 提交 XR_EYE_VISIBILITY_BOTH 的单个二维 quad，位于 VIEW 空间前方 2 米，
宽 1.6 米，保持游戏宽高比。面板跟随头部，本轮不驱动游戏摄像机、
不进行每眼场景绘制，也不把同图双眼称为立体渲染。
源窗口关闭、最小化或尺寸变化时报告失败；采集超过 1 秒没有新帧时暂不提交面板。

短测 1：3 秒、346 次 quad 提交、42 次纹理上传、41 次图像内容变化。
短测 2（加入 GPU 核验）：3 秒、344 次 quad 提交、41 次上传、40 次变化，
gpu_pixels_verified=true、正常退出。游戏 PID 28720，VDXR/Quest 3。
第二轮证据：analysis/openxr/pose-20261012-002210-205308.*；
源码截帧 .csv-game-source.ppm、实际 swapchain 回读 .csv-game-submitted.ppm。
这些证明捕获、上传、提交路径；是否看见、颜色和观感仍需用户确认。

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\openxr-probe\run_pose.py --seconds 30 --game-screen --runtime 'C:\Program Files\Virtual Desktop Streamer\OpenXR\virtualdesktop-openxr.json'
```

仅对子进程覆盖 OpenXR runtime，不改系统注册表。自动结束并关闭 WGC/OpenXR 会话。
下一步评估原生渲染图像共享、匹配视点/投影与图像时序，再进行真正双眼渲染。

## 用户确认的 30 秒测试

证据 analysis/openxr/pose-20261012-002408-134913.*。
3,572 次 quad 提交，411 次游戏纹理上传、410 次内容变化，
GPU 首帧像素核验通过，程序正常退出。原始 OpenXR 位姿 3,572 条均有限且被跟踪；
不能把 quad 提交数换算成游戏图像更新率。
用户明确确认：“看到关卡，画面连续，颜色正常”。
本轮由头显端观察和提交记录共同确认显示通路，未开启游戏内头动 Hook。
当前实现的上限和目标仍是二维诊断面板，不代表已完成沉浸式 VR 或双目场景。
