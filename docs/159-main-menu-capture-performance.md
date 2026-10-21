# 主菜单窗口捕获卡顿修复

2026-10-20。主菜单使用独立的 WGC 全客户区捕获和 OpenXR 共同面板。旧路径每 33 ms 才尝试一次捕获；没有新帧或捕获时间戳略在未来时，同样要等待下一轮 33 ms。每次更新还在 XR 线程上执行逐像素整数除法、整幅图像的串行逐字节 FNV，以及标量颜色转换。该问题属于现有 Mod 源码，不需要新增游戏原生逆向。

## 默认修复

`game_screen.h` 让启动和菜单恢复面板每个 XR 周期读取最新可用 WGC 帧，保留 mailbox、future-frame 延迟、菜单 epoch、HWND/进程身份、窗口代次、图像新鲜度及提交后的菜单授权检查。非启动用途的旧诊断面板保留原 67 ms 节流。

`menu_pixels.h` 缓存缩放列地址，同宽图像逐行 memcpy；输出仍与原最近邻缩放逐字节一致。上传采用 x64 基线 SSE2，保留 BGRA/RGBA 两种格式及 alpha=255，处理非对齐地址、行间 padding 和不足四像素的尾部。诊断指纹改成覆盖全部像素的四路 FNV，主菜单日志明确标记 `source_hash_algorithm=pixel_fnv4`；它与旧 byte-FNV 数值不同，不能作跨算法身份比较，也不是密码学哈希。

启用现有 outage diagnostics 时，每秒输出一条 `menu_capture_performance`，记录 WGC 上传数、render 调用和成功数、路径墙钟区间及尺寸。字段 `render_cpu_ms` 包含 D3D11 Map 和 GPU fence 等待，是 CPU 侧观察到的墙钟时间，不能解释为纯 CPU 执行时间或 GPU 耗时。新纹理上传率与 XR 面板提交率分别统计。

默认 XR 为 `tools/openxr-probe/build-menu-smooth1/OpenXRPoseProbe.exe`，SHA256 `5b7051b756725b84545c96494d7af38deeecb12fab32c24b843a35e4db8cff7d`。`tools/vr-launcher/manifest.json` 只更新 XR 路径和哈希。集成时其他任务已把默认 Native 更新为 `build-seated-reticle1/GearsCaptureSeatedReticle1.dll`、SHA256 `adde3b7484fde70dca516c068a108cc7111237389b44da3ce39f77777f228410`，本修复保留该选择；下列实际对照使用的是明确记录的另一 Native 版本，不能写成新默认组合已联合运行。

## 实际游戏对照

使用实际 `GOWDE-Steam.exe`、1280×720 菜单、90 Hz 私有 behind-windows 模拟器，同样的用户设置和 render_scale=1。游戏 EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，两轮 Native 均为 `9505245bd8ba18632b6faac1d1c39232be7a069cffbd3164ae0c5e0e9151c07e`。旧对照仅添加相同计数诊断，保留旧捕获与像素路径，XR SHA256 `7506f88c939608f52480fb6dc5f2f666e1f2a9a8dd84baf1f3d96cd079708f56`。

测试通过现有受限导航仅到达 `DeltaMainMenu`，观察 24 秒，排除最初两秒后聚合完整的一秒区间，再取消菜单等待、恢复入口并正常关闭所属游戏。没有进入关卡或确认存档操作。

| 实测项 | 旧对照 | 默认修复 |
| --- | ---: | ---: |
| 新菜单纹理更新率 | 22.31 FPS | 41.27 FPS |
| 面板 render 成功率对应频率 | 90.00 Hz | 89.99 Hz |
| 每秒 render 路径墙钟区间合计 | 223.80 ms | 73.71 ms |

有效新菜单更新率提高约 85%。纹理更新次数来自完成上传的 WGC 帧；这不是游戏内 Present FPS、原生双目新帧率或头显刷新率。两轮会话 `xrEndFrame` 和 orderly shutdown 正常，原生场景捕获未激活，故不构成玩法性能测试。

有效收据：

- `analysis/runtime/menu-smooth-baseline-b-1792483752-20261020.json`，会话 `continuous-stereo-30980-20261020-160951-e1a84f22`。
- `analysis/runtime/menu-smooth-candidate-a-1792483823-20261020.json`，会话 `continuous-stereo-18196-20261020-161100-1c1b29df`。

两轮均使用已核验 behind-windows 游戏构建和模拟器，设置 `GEARSVR_TEST_WINDOWS_BEHIND=1`、`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`、`GEARSVR_TEST_MUTE_GAME=1`，禁用物理手柄后台输入。观察期间每约 200 ms 采样前台 PID，未发现测试游戏成为前台；这是离散采样，不是完整窗口遮挡验证。没有调整用户窗口。结束后所属游戏退出，用户 INI 和 Coalesced 原始字节恢复，音频 guard 的恢复记录保存在对应 index/audio 日志中。

## 其他验证及边界

10 项相关 C++ 可执行检查及 XR 两项无会话自检通过；菜单观察器 46 项、连续会话 runner 59 项 Python 测试通过。实际启动的 22 项 Native 预检通过。新增 `MenuPixelsTest` 对照旧标量结果检查尺寸变化、最近邻缩放、两种颜色格式、padding、非对齐、尾部和指纹覆盖。没有更改 GPU 命令、资源屏障或 fence 所有权。

独立 CPU 像素处理微基准在 4K→1920×1080 和 1280×720 两个输入条件下逐字节核对上传结果，均无差异。它支持局部处理成本下降，不能代替实际游戏或 GPU 测试；原始区间见 `analysis/runtime/menu-pixel-bench-4k.jsonl` 与 `menu-pixel-bench-720p.jsonl`。

16:12:02 后另一款 `GoWEDay-Steam.exe` 启动并持续高负载；其后的第二轮旧版重复值、候选窗口就绪超时均不作有效性能对照。后续预取候选仅留在 ignored `analysis/` 和独立测试构建中，没有进入默认源码或默认 EXE，未宣称收益。

本次验证是实际游戏菜单捕获加模拟器提交。真实头显的菜单流畅度与舒适度、新默认 Native+XR 联合会话和 4K 实际菜单仍需分别实测。正常退出并从启动器重新启动后加载默认 XR；正在运行的旧进程不会热更新。原始日志、派生数据、快照和实验均留在 ignored `analysis/`，不进入发布包。
