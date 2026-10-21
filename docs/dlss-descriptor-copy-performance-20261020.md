# DLSS 手动入口的描述符跟踪 CPU 优化

2026-10-20。`GearsVR DLSS Test.cmd` 的独立清单更新为 NgxManual3，保留 NgxManual2 的启动证明隔离修复。DLL 为 `analysis/native-build-ngx-manual3/GearsCaptureNgxManual3.dll`，SHA256 `7eda76dbcdc4cea004914840f969c895c1c5ed4bb1c20a5e0aea634b66ccd657`。重新打开测试启动器后生效；已有 GUI 保留它创建时的私有副本。

## 修改和检查

旧描述符复制路径即使只复制一个描述符，也构造、清零和销毁最多128项的临时 Cell 数组，生产实例约48 KB。实际 NGX 观察每秒有大量此类回调。现在将单描述符复制分离，只冻结一个源 Cell；其余范围仍走原实现。保留 heap/范围/可见源拒绝/重叠/容量/版本/资源持有和来源证明，不增加驱逐或授权，不修改渲染命令或分辨率。

NgxManual3 冻结源码以 NgxManual2 为基础，生产差异仅 `ngx_mv_descriptor_provenance.h`，另增加同语义两种范围表示的144组差分检查。22项启动、31项针对检查、8项补充检查以及真实 D3D12 描述符 fixture 通过。合成 CPU 测试中20万次复制约182 ms降至5 ms，不能直接当成游戏帧率。

## 实际游戏 A/B/A

相同存档场景、相同游戏配置字节、render_scale=1、每眼3072×3264、接收源6144×3264、桌面1280×720。保留动态模糊、QualityBloom 和 AA quality3，启动阶段 DLSS 关闭。使用120 Hz认证模拟器；每轮45个有效样本，稳定段取最后30个样本。统计新发布的双眼画面对数，重复投影不计为新帧。

| 顺序 | 构建 | 全段均值 | 稳定段均值 |
| --- | --- | ---: | ---: |
| A1 | NgxManual2 | 21.80 | 22.02 |
| B | NgxManual3 | 23.53 | 25.79 |
| A2 | NgxManual2 | 21.41 | 21.97 |

单位为对/秒。相对两次旧版稳定段均值21.9956，新版提高17.235%；旧版前后漂移0.262%。全段包含启动后回读等扰动，完整样本保留在回执中。三轮均正常停止 native/XR、恢复入口字节、正常关闭游戏并逐字节恢复游戏设置；候选音频 guard 显示 restored1、restore_pending0。候选 owned_error0、UI first fault none、DLSS output publication rejected0。

模拟器设置文件 SHA 随自身启动保存的预览窗口高度变化。机械重建得到三次原始 SHA，只改变 window_height=1304/1265/1226，其余字段逐字节相同；每眼尺寸、FOV/IPD、full_render=false、show_stats=false 和 preview_fps90 均相同。没有移动或调整玩家窗口。该差异和完整回执在 ignored `analysis/runtime/ngx-copy-fast-simulator-settings-audit.json`，不隐去设置文件摘要差异。

候选会话 `continuous-stereo-11728-20261020-191707-6d13842b`：1148对接收图像、5877次投影。实际 GPU 回读6144×3264，SHA256 `60b9b4ff42353cab07c445949e6fefd953c86396b6d08c079e3eb3b793f1db9e`，已目视检查完整左右眼关卡和角色；旧版同场景也有对应回读。此证据不代替开启 DLSS 后的功能/帧率、跨世界或物理头显验收。

## 入口和证据

汇总回执 `analysis/runtime/ngx-copy-fast-comparison-private.json`；源码、构建和检查为 `ngx-manual3-source-audit.json`、`ngx-manual3-tests.json`。包装脚本校验及实际 prepare_run 的9项私有复制校验通过。仅更新独立 DLSS 清单；普通入口的其他任务更新保留。

最初两次对照因直接复用已有事件文件的构建目录而在激活前拒绝，未取得帧率样本。测试脚本已改为每轮通过 prepare_run 创建独立认证目录，保留旧日志。无样本不解释为零帧率。这两次失败回执仍保存在 ignored analysis/，未用于上表。

此前用户手动真机反馈“似乎已经没问题了”对应普通 StartupOutput5 会话，而非本次 NgxManual3；该反馈和33475对历史接收记录见[启动修复](161-vr-startup-output-fallback.md)。本轮性能优化未降低渲染比例、画质或改变用户菜单锁策略，未生成发布包。
