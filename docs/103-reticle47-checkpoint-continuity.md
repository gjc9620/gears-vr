# Reticle47 三次检查点恢复与过渡显示缺口

2026-10-17。实际会话 `continuous-stereo-10640-20261017-095902-3ef8fd8a`，游戏PID10640／creation134366759290115025，XR PID22572／creation134366759420633581。使用当前默认 Reticle47／Borderless1、显式模拟器运行时、原存档继续流程。后台实体手柄未启用。没有修改游戏内存或强制角色位置。

## 恢复证据

按实际死亡菜单及焦点认证，连续三次选择检查点；world generation 1→2→3→4，恢复后分别观测到新代43、42、40次提交。派发至观测耗时5521、6576、6578毫秒，包含加载及采样，不能当作全程黑屏时间。

最终发布4215帧，capture_attempts4224、busy3、capture_skipped9，ticket／rect拒绝0。FrameLink及绑定表高水位均337，超过旧256上限；容量溢出、descriptor_evictions均0。四份3840×2160 GPU快照，每份两眼均有非黑内容；逐文件哈希和每眼均值保存在 `analysis/runtime/reticle84-summary.json`。

正常停止phase=stopped、passed=true、native_cleanup_verified=true，独立恢复与游戏普通关闭通过。四个玩家存档在启动前备份，测试后保留副本，全部进程退出后恢复基线并核验哈希。证据 `reticle84-audit.json`、`reticle84-normal-close.json`、`reticle84-save-restored.json`。

## 尚未解决的连续性问题

模拟器最终合成检测保留7个完整事件：5个MISSING_PROJECTION_LAYER、2个ASYMMETRIC_BRIGHT_BREAKTHROUGH。另有退出时未完成incident13397，缺incident.json，明确排除，不当作已分析事件。完整事件记录和选定原图保存在ignored `analysis/preview-evidence/reticle84-complete`，复核图与亮度摘要见 `reticle84-preview-audit.json`／`reticle84-preview-contact.jpg`。

两个亮度事件的选定图像可见枪火、受击效果；没有据此确认整眼黑屏。缺投影事件的触发时刻有3次layer_count=0、2次layer_count=1。各事件保留图像并未覆盖所有缺层时刻，不能由这些非黑图像推断整个窗口没有黑屏。

部分后续合成图显示黑色矩形提示区。CPU提示卡源图有文字，但 `transition_card_source_preview` 明确gpu_readback=false，不能证明上传后的GPU内容。需要区分静态卡片上传／OpenXR提交与模拟器预览的独立问题，尚未证明真头显同样显示黑矩形。

非空xrEndFrame诊断记录最长缺口632.427毫秒且恢复：first_missing为stale_ticket、menu_mode1、layer_count0；threshold_crossing为waiting_image、mode1；last_missing变为menu_requested、mode2；随后startup_panel提交成功。时序与进入死亡菜单相符，仍不能声称已完成逐帧像素归因。该诊断complete=false，另有7个后续窗口未逐行保留。图像票据记录4096行、丢弃115行，retention_complete=false。保留这些限制，不把有限记录当作长期零黑屏证明。

## 机枪覆盖

查询资料库和已有实际记录只确认原庭院有两台无Driver的Troika。此前接近停在掩体，尚无正常占用、Controller／Driver关系及上机枪后的GPU图像证据。没有可靠“章节菜单→最近机枪”映射，因此未盲目跳章或重复无目标移动。本轮检查点恢复不能替代机枪验收。
