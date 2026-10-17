# 开场 CG 后双眼图像发布中断

2026-10-17，Reticle46／Borderless1，实际游戏会话 `continuous-stereo-24660-20261017-092744-7bfc80e7`。此问题与 docs/100 的启动证书变化不同。本轮准备在 1.992 秒内完成，未触发新的受控角色链接重试分支。

## 实际证据

从实际菜单选择 Prison-Chap1（sp_prison_p，LoadChapter=0）。MovieProgressReader 观察到 89 帧短片段和 1674 帧开场片段；匹配 XR PID 20568 的运行时诊断及模拟器截图证实电影内容和字幕出现在两眼共享平面。这不是几何立体渲染证明。

电影结束后，世界场景双眼构建、票据消费和准备累计 5535 次，mismatch=0，但 published=0、capture_attempts=0。左右眼各 5532 次 final-eye failure reason128 表示没有跟踪到 OM 渲染目标。仅 3 次 ticket retain／duplicate，FrameLink linked=0。

同轮 `frame_link_capacity` 记录 list_highwater=296、list_limit=512、list_overflow=0、event_overflow=0、descriptor_evictions=0；另一层 `frame_target_stats` 的固定表只有 256 项，实际 target_binding_overflow=408510。Present 清理检测到该溢出会调用 FrameLink.invalidate()。容量契约不一致及溢出后的关联失效是已确认事实；尚不能把每一次缺目标都归因于同一个溢出列表。

`frame_target_stats` 是默认非 strict tracker，`owned_target_stats` 才是 strict。非 strict 可以通过新 OM bind 建立项。Present 前后清 bindings 和 FrameLink 清 recording generation 还可能构成独立跨边界问题；本次没有证据支持保留旧绑定绕过校验。

## 修复与验证要求

先统一两层固定命令列表容量，保留超限拒绝与 Present 失效门禁；加入活动数、高水位和首个拒绝列表的有界诊断。CPU 测试覆盖超过旧 256 上限仍保持各列表绑定、超过新上限仍拒绝、retire／clear 后恢复且诊断历史不被擦除。

实际同路线复测必须同时检查：容量压力实际超过 256、零溢出、双眼票据正常、reason128 不再按每帧增长、linked→capture_attempts→published→OpenXR projection 贯通。仅溢出归零不能算修复成功。

本轮最终 phase=incomplete、passed=false，但 native_cleanup_verified=true，拥有独立停止恢复证据；所有者／XR 已退出后普通关闭实际游戏。测试后的四个存档先保存副本，再恢复基线并核对全部哈希。证据位于 ignored `analysis/runtime/prison81-*` 及会话同名文件。原玩家章节进度没有被本次测试覆盖。
