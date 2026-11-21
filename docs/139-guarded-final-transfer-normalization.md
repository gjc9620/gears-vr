# Guarded final-transfer normalization

2026-10-17. Native66 source revision `4fcd1af`; frozen DLL SHA256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`. This candidate follows the bounded pending-state evidence in [137](137-pending-final-transfer-target.md) and the [Native65 runtime comparison](138-pending-final-target-runtime.md). `GEARS_FINAL_TRANSFER_NORMALIZE` remains **OFF by default**. No launcher default or release package was promoted.

## Guarded behavior

For the attested final quad, the candidate validates a fresh complete source pair, source/list generation, pending destination and descriptor provenance, world/inventory, and consumed OM/viewport/scissor binds. It holds the existing recursive frame mutex across the original call and completion. Only sampling UV changes: the observed native mapping `-76.154541,0,2712.30908,1080` becomes `0,0,2560,1080`; destination geometry and declared dimensions remain unchanged. No game or window resolution is forced.

Identity plans use the existing legacy path. Failed preflight leaves original arguments untouched. A mismatch after mutation, extra/nested draw, bypass, exception, or reentrant Execute/Present disables further normalization and publication for the session. The legacy transfer predicate remains unchanged.

## Actual115 result

Game PID32848, session `continuous-stereo-32848-20261017-193347-21d52549`, rendered a 2560×1080 source into a 2534×1009 backbuffer. The 30-second monitor recorded 61 samples and publication increasing from 0 to 1305. Final native summaries recorded 2976 planned, linked and completed normalized transfers, zero normalization failures, and 2968 published frames. Seven entry preflight rejections did not mutate arguments. Frame-link inventory changes and contamination were zero.

Two original-resolution GPU snapshots, tickets 49 and 457, were saved at 2534×1009 in format 24. Their pixel receipt records different eye content (mean absolute byte differences about 18.30 and 18.80). The coordinating review saw two complete SBS gameplay images with no obvious black half. This verifies GPU image output for this ordinary scene and this nonstandard mapping; eye differences and visual review do not establish correct projection.

Native cleanup and entry restoration passed. XR reported orderly shutdown and 2968 scene copies over approximately 73.34 seconds; the owned game closed normally. All owned processes exited and the four saved profiles were restored. Ignored receipts: `analysis/runtime/vrdof115-result.json`, `vrdof115-pixels.json`, `vrdof115-normal-close.json`, and `vrdof115-save-restored.json`.

## Validation limits

This run does **not** accept UI coverage, projection accuracy, physical tracking, headset presentation, mounted gameplay, or long-session continuity. It does not decode the pixel shader or establish correctness for arbitrary resizing or scenes. The result receipts explicitly retain `headset_verified=false` and `release_ready=false`.

`FinalTransferNormalizeTest` is now a CMake CPU target with `/UNDEBUG` so Release builds retain assertions. It exercises guarded plans, identity passthrough, invalid geometry/modes, changed post-call proof, and completion ambiguity. CPU tests complement the runtime receipts; they do not replace GPU or headset acceptance.

---

## 中文

# 守卫最后移交正常化

2026-10-17 原生66 源码修订 `4fcd1af`冰冻的DLLSHA256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`候选人遵循下列各项中已限定的待决状态证据: [137](137-pending-final-transfer-target.md) 编辑 [本地65运行时间比较](138-pending-final-target-runtime.md). `GEARS_FINAL_TRANSFER_NORMALIZE` remains **默认关闭**。没有推广发射装置默认或释放包。

## 守法行为

对于经认证的最后四重奏,候选人验证了一个新的完整的源对、源/列表生成、待目的地和描述来源、世界/库存以及消耗的OM/viewport/剪切器结合。它持有现有的递归框架变异星 s 原始调用和补全。仅取样紫外线变化:观察到的本地映射 `-76.154541,0,2712.30908,1080` becomes `0,0,2560,1080`; 目标几何和已宣布的维度保持不变。没有强制游戏或窗口解析。

身份计划使用现有的遗留路径。 失败的飞行前未将原始参数保留在未动中。 突变、 超常/ 取消绘图、 绕行、 例外或再入执行/ 执行后不匹配, 会使会话的进一步正常化和发布失效 。 遗产转移上游不变。

## Actual115 result

游戏 PID32848 届会 `continuous-stereo-32848-20261017-193347-21d52549`,并做了一个 2560×1080 来源到 a 2534×1009 后缓冲器 30- 记录第二台显示器 61 样本和出版物增加 0 to 1305. 最后记录的本土摘要 2976 计划、联系和完成的正常化转让、零正常化失败和 2968 已公布的帧。有七个条目的飞行前拒绝没有改变参数。帧链接的目录变化和污染为零。

两份原版GPU快照,票 49 and 457,保存于 2534×1009 格式化的 24。它们的像素接收记录了不同的眼部内容(关于 18.30 and 18.80)协调审查看到两个完整的SBS游戏播放图像,没有明显的黑色半身。这验证了这个普通场景和这个非标准映射的GPU图像输出;眼差和视觉审查不能确定正确的投影.

原生清理和入口恢复工作通过。 2968 现场复制约 73.34 秒; 普通关闭自有游戏。 所有自有进程退出, 保存的4个配置文件被恢复。 忽略了收件 : `analysis/runtime/vrdof115-result.json`, `vrdof115-pixels.json`, `vrdof115-normal-close.json`,以及 `vrdof115-save-restored.json`.

## 验证限度

运行会 **not** 接受UI 覆盖、投影精度、物理跟踪、耳机演示、挂载游戏播放或长片段连续性。它不会解码像素遮蔽器,也不会确定任意重定大小或场景的正确性。结果接收会明确重写n `headset_verified=false` and `release_ready=false`.

`FinalTransferNormalizeTest` 现在是一个 CMake CPU 目标,使用 `/UNDEBUG` CPU 测试可以补充运行时间的接收,它们不会取代 GPU 或 头盔接受。
