# VR 景深兼容候选：启动与实际回归记录

2026-10-17。候选60在实际101中恢复了死亡场景的双眼画面，并完成一次检查点恢复。默认玩家候选仍为 Reticle47；尚未进行真头显验收，也未宣称解决全部黑屏。

## 候选与启动检查

冻结源码为 `4fd6884a4d1b2d3812ce2696943177bf4effaafd`，`GearsCaptureVrdof60.dll` SHA256 为 `599dd838f08b832b2fbd4265bf4251ed90f17535f0af43f97edd8d17a0bfdc66`。14项冻结 CPU／SDK 检查、22项完整 startup suite 均通过；startup suite 本身没有启动游戏。见[构建记录](../analysis/runtime/hdrpixels60-build-proof.json)与[完整启动检查](../analysis/runtime/vrdof60-full-startup-suite.json)。

独立默认关闭的 `GEARS_VR_DOF_COMPAT` 对 possessed／unpossessed 两类 owned VR 视图使用相同认证 setter，将有限布尔参数 X=1 的局部副本改为0，保留 YZW、调用参数、返回值及调用方内存。普通二维绘制不在该作用域内。首版兼容暂时舍弃这一路景深模糊效果；没有修复底层模糊图集布局。

128项票据存储按冻结 `(world,id)` 记录值证明。完成且未经过死亡 site 的票据可沿原路径发布；参与 site 的票据必须具有两眼 setter 后及 quad 前的零值证明。缺失、未完成、重复、读取失败和容量溢出均拒绝发布。判断位于真实 Present 的关联结果之后、GPU capture 之前；不重写原关联结果，也不增加 GPU 命令。

候选59的试次100在游戏启动前，因 legacy GPU 自检夹具没有登记 completed NoSite 票据而以179退出。`4fd6884` 仅为该夹具补齐受 `self_testing` 限制的完成记录，并保留 missing／incomplete 拒绝检查；真实 production gate 未加无条件 bypass。候选60随后通过完整检查，候选59冻结物保留。

## 实际101

会话为 `continuous-stereo-8636-20261017-133838-24955675`，游戏 PID8636。最终[原生状态](../analysis/runtime/continuous-stereo-8636-20261017-133838-24955675.native-status.txt)记录：

| 项目 | 结果 |
| --- | --- |
| begun／completed | 1609／1609 |
| 两眼 modified | 各266 |
| 两眼 setter／prequad 证明 | 各266／266 |
| NoSite／零值证明发布门放行 | 1339／265 |
| 发布拒绝、overflow、late | 均0 |

证明计数与发布门调用次数不是 GPU 帧数，不能直接等同。

最终 GPU 快照 `.gpu.ppm.2.ppm` 为票据500、3840×2160，SHA256 `b3314264f2f9c75c507fc7fb722bb3499ad4253574e44b9d8c9f951e862b6b59`。两眼均可见死亡角色、地面及周围场景；RGB各通道均≤8的像素占比为左眼0.0146%、右眼3.4140%，本次未再呈现原有的近乎全黑右眼。原始图及预览留在 ignored `analysis/runtime/`，不进入发布包。票据记录见[运行审计](../analysis/runtime/vrdof101-audit.json)，四图完整哈希、各眼均值与近黑比例见[联合摘要](../analysis/runtime/vrdof101-summary.json)。

一次检查点恢复从 world1 进入 world2，输入派发到观察到恢复为7507.1635ms，包含加载时间，不能作为纯渲染恢复耗时。[正常关闭记录](../analysis/runtime/vrdof101-normal-close.json)确认会话通过、原生清理完成及普通游戏关闭；[存档恢复记录](../analysis/runtime/vrdof101-save-restored.json)记录全部相关进程退出后四份存档文件的恢复哈希。

## 边界与后续补充

本次支持该死亡路径的双眼兼容及一次检查点恢复；不构成真头显舒适度、长期稳定性、全部 CG／武器／设置或所有黑屏场景的验收。两眼失去此处景深效果是当前兼容取舍，底层 T4／T5 图集问题仍未完整修复。

实际102连续完成三次检查点恢复，world1→2→3→4，派发至观测分别6275.7747、4757.2937、7936.8749ms（含加载）。4584份消费全部完成，两眼各724次修改及setter／prequad证明；NoSite／零值门放行3857／724，拒绝、overflow、late均0。7份3840×2160 GPU图中，三份死亡图左右均保留场景，右眼近黑占比依次4.4580%、3.1182%、4.2842%，不再重现原基线约97.5%的近黑右眼。恢复图两眼均非黑。

会话为 `continuous-stereo-1732-20261017-135105-c8031a4e`。逐图哈希、近黑比例和恢复事件见ignored `analysis/runtime/vrdof102-summary.json`；审计、正常关闭及四份存档恢复回执分别为同目录`vrdof102-audit.json`、`vrdof102-normal-close.json`、`vrdof102-save-restored.json`。协调线程复核三份死亡图；本轮支持跨多次world恢复的该路径兼容，不能扩大为任意章节或长时间零黑屏保证。
