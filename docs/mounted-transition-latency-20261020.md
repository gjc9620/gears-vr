# 上、下机枪的短暂黑屏延迟

2026-10-20。用户确认选关菜单黑屏已经修复，另报告上机枪约2秒、下机枪约3秒的黑屏，随后正常。此项处理切换时延，实际头显时延仍须用新版本复测。

## 已定位的等待

真机会话 `continuous-stereo-38848-20261020-184757-c3832acd` 使用 StartupOutput5 native `28884fa68c55a1536430b3e0e9c7a596a65f7ef32a5e7adadd6103aa34707536`、MenuSmooth1 XR `5b7051b756725b84545c96494d7af38deeecb12fab32c24b843a35e4db8cff7d`、游戏磁盘 EXE `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。日志显示 generation 1→2→3，两个重绑定均提交并恢复新帧，没有把这两次暂时空白归为永久停止。

发现三处可消除的等待：

- owner 的完整世界发现默认每2秒检查一次。上、下机枪更换受控 Pawn 后，下一次检查前旧锚点已不能用于新帧。
- 实际只读分析中，旧候选准备约2秒；其中相机全注册表发现约0.68秒，UI对象发现约1.08秒。它们属于CPU墙钟区间，不是GPU时间。
- native 的普通状态发布间隔为1000毫秒。Prepare、进入quiescing、Commit等进度原本可能等待下一次普通状态输出，owner 即使已经发送事件也要等待回执。

## 修改

1. 每100毫秒最多读取两个已认证字段位置的指针，提前提示受控 Pawn 或相机变化。提示只安排完整只读发现，不是锚点、显示或输入证明。空值、读取失败、无效会话和错误身份不授予权限；原有完整发现周期仍保留，失败退避不被提示绕过。每次读取关闭自己的只读句柄。
2. 准备候选时复用相机发现位置提示。新 Resolver 仍认证磁盘哈希、模块、注册表及 controller/camera/local 完整身份；提示失效会丢弃 reader，通过既有有界重试重新全局发现。Anchor、UI、准星、代码入口及提交前 native guard 均重新检查，不缓存通过结果。
3. UI仍遍历同一份完整对象指针表。先用同一轮的页提示读取对象 class，筛掉非Class实例，再解码候选的名字；所有选中的类、函数、属性及最终注册表复核继续使用独立 fresh 读取。没有取消类重复检查、减少发现范围或跨轮保存页提示。
4. `world_status_cadence.h` 仅在 session worker 中观察 world generation、request、reject、candidate、phase。握手进度变化立即发布状态，平时保持既有频率；调用发生在 world gate 重开之后，不增加渲染钩子内IO或改变GPU退休条件。
5. world Prepare事件记录 observe/prepare/worker耗时，后续区分发现、CPU准备和native握手等待。

## 构建和检查

候选 MountedLatency1 DLL SHA256 `a2d52a9804cb0c3e37e40e749ff3a5dda7c517d99c7cfb188c9b3caa5c798d19`，路径 `tools/native-capture/build-mounted-latency1/GearsCaptureMountedLatency1.dll`；独立清单 `tools/vr-launcher/manifest-mounted-latency1.json`。

原生代码从已交付 StartupOutput5 的冻结源码复制，只修改状态进度发布、增加对应头文件/测试/CMake目标；没有带入工作树其他DLSS实验。依赖验证构建、40项原生/启动检查通过。Python世界恢复、提示失效、源身份、UI与连续runner共143项测试通过，注册表发现/名称读取16项测试通过。新的回归覆盖提示不能创建证书、失败退避、句柄关闭、同地址实例代际变化、筛选与原发现得到相同配置，以及错误属性继续拒绝。

实际VDXR进程的只读测量中，新版在已上机枪状态准备一个不发布、不发送事件的候选约761毫秒。此前约2秒的测量在步行状态；两者角色不同，不能当作严格配对收益，更不能当作头显切换黑屏已缩短到761毫秒。

私有源码与构建：`analysis/mounted-latency1-source/`、`analysis/native-build-mounted-latency1/`。CPU构建回执、分阶段只读测量及实际游戏回归结果均在 ignored `analysis/runtime/mounted-latency*`。没有新增游戏反编译或操作IDA数据库，没有发布发行包。

后台回归沿用认证的游戏/模拟器置后窗口、只观察微小输出、静音及禁用物理后台手柄输入，运行前备份并在自有游戏退出后还原五份设置/存档。此测试覆盖普通世界重建与检查点，不替代真机上、下机枪的端到端时延验收。结果待追加。

## 实际回归和本地启用

第一次后台回归中，新帧和GPU复制持续、native无UI故障，Prepare到owner确认Commit完成约0.315秒。但是旧测试只接受 `world_handoff_card_ended` 作为新世界投影证据。该次切换在菜单/电影回退期间完成，没有显示world等待卡，因此缺少该日志，90秒后严格判失败。完整失败报告为 `analysis/runtime/mounted-latency1-actual-20261020.json`；退出和五份文件还原均完成。没有把它改记为通过。

XR新增独立 `world_handoff_projection` 回执：仅在原有 `handoff_projection_completed()` 已确认当前world、copied generation、有效图像和成功的双眼projection `xrEndFrame` 后写一条记录，保留ticket、采样时钟及generation。与是否显示提示卡无关，没有改变渲染或准入条件。候选XR为 `tools/openxr-probe/build-mounted-latency1/OpenXRPoseProbe.exe`，SHA256 `0a2b7f7dc6a65f8ee23c255423e5fac841726ac6f2ef9bc249ac8e5f4902d56c`，依赖验证构建与5项现有策略检查通过。相关源文件快照在 `analysis/xr-mounted-latency1-source/`。

第二次实际会话 `continuous-stereo-11480-20261020-191327-e865a47d` 完成两次检查点重建，generation 1→2→3，各自取得新world的真实XR投影回执和1792×980 GPU源图，`saved_verified=true`、`stage_verified=true`。两次owner握手（Prepare事件到committed回执）分别约0.3123秒、0.3131秒；worker总时长约1.377秒、1.163秒。这是检查点场景的CPU/握手测量，不是头显机枪切换黑屏时长。原生/UI检查无故障，正常关闭、owner/XR退出和五份设置/存档还原均通过。

最终回执 `analysis/runtime/mounted-latency1b-actual-20261020.json` 的 `passed=true`、`two_reloads_verified=true`、`settings_and_profiles_restored=true`，error/cleanup_error为空。通过后只更新默认清单的native目录/DLL/摘要及XR目录/摘要；个人设置不变。原清单备份、提升回执为 `mounted-latency1-previous-manifest.json`、`mounted-latency1-promotion.json`。正常退出游戏并从根目录启动器重开后使用新版本。`headset_latency_verified=false`，还须重复上、下机枪，才能报告实际切换时长或宣称黑屏完全消失。
