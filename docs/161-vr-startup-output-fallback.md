# 载入关卡后 VR 不激活、短暂出现后退出

2026-10-20。默认 native 已更新为 build-startup-output5/GearsCaptureStartupOutput5.dll，SHA256 `28884fa68c55a1536430b3e0e9c7a596a65f7ef32a5e7adadd6103aa34707536`。保留 MountedRecovery6 两步输出和首帧恢复保护、SeatedReticle1 准星修复及 MenuSmooth1 XR。正常通过根目录启动器重启游戏即可加载。

## 故障与最终修改

最早会话 continuous-stereo-38116-20261020-154557-e75db760 在 native 激活前报 crosshair_assignment_activation reason=schema。该校验包含动态 Pawn/Weapon/Instigator 链接，不能只据总错误码认定脚本布局损坏。激活前现在复用既有同 Pawn 武器重绑定证明，纯读取准备阶段最多等待约1秒；始终从原证书重新核验，保留 execLet 入口、对象身份和布局检查。实际 PID37044 已记录 accepted=1、weapon_rebound=1。旧38116日志不足以证明具体是哪条链接变化。

后续 native 已激活，却在 ticket2 的 HUD 绘制报 draw_state、flags258、pair0，随后 proof_failure 停止。此时 HUD FOV/大小计算尚未执行，不能归因为用户设置的 FOV51。

动态模糊开启时，普通 Output 的 d83cbf 绘制可以先落到中间资源，再经2b11a0复制到当前 owned 目标。只补第一步调用识别仍不能建立目标上的世界双眼记录：PID19892 的诊断证明两眼几何、viewport/scissor 和绘制命令均有效，但实际目标与 owned 资源不同。MountedRecovery6 补齐第二步复制识别，具体代码身份、实际调用和两次关卡恢复见[恢复记录](mounted-transition-recovery-20261020.md)。最终版本沿用此实现，无需持久关闭动态模糊或降低画质。

两步识别必须满足当前 owned renderer、冻结 world/ticket、代码完整摘要、左右半幅原像素矩形和 UV、有限且足够大的源尺寸。目标资源身份、命令列表、实际 draw、双眼数量、队列执行、GPU 发布与 HUD 完成证明继续执行。不是任意调用点白名单，也不把中间目标的绘制直接视为 owned 输出。完整代码范围校验缓冲区修正为0x1000，仍检查长度上限和全部摘要。

高画质启动另有一次实际命令列表跟踪表达到512项，首次失败 subtype6、after_active512、overflow1。最终将 frame association 与 RS/OM 共用的固定容量提高到1024，不引入驱逐，不放宽未知或失效列表拒绝策略。StartupOutput5 相对 MountedRecovery6 冻结源码的唯一新增生产差异是这个容量值，未混入工作目录其他 DLSS 实验代码。

## 最终实际游戏验证

回执 analysis/runtime/fps-resolution-720p-startup-output5-full1-20261020.json，会话 continuous-stereo-14812-20261020-173548-d44e2302：

- 启动前实际配置 MotionBlur=True、QualityBloom=True、AADefaultQualityMode=4、DLSS=False；用户 render_scale=1、HUD FOV51、camera_closer=0.4 保持原值。
- 匹配 VDXR 推荐尺寸的认证模拟器，实际 GPU 源6144×3264，两条 XR 眼链各3072×3264，像素裁切无缩放。桌面1280×720，与 VR 尺寸分开记录。
- 12个有效遥测样本，持续立体投影13.8567525秒；791对新图像，1656次成功投影提交。最长提交间隔96.4445ms，不能据此声称物理显示达到120FPS。
- native 首次 UI fault 为 none、owned_error0、pipeline_failed0；原生清理通过，79处独立入口字节全部还原匹配，XR/owner正常结束，游戏正常关闭，设置逐字节恢复。
- 接收端实际 GPU 回读 ticket552、serial551，6144×3264 PPM SHA256 `fcd5dafbe844258f986ae9d42537ed2c13c66e93807e5db39bb96b1726972bd0`。左右眼非黑像素99.9867%/99.9839%，两眼不同像素99.9448%；已目视检查两眼均有完整关卡、角色和字幕。这是该张 GPU 图像证据，不代替全部效果或物理头显验收。
- 最终 DLL 的39项原生检查通过，包括22项启动自检、普通两步输出、准星投影、启动武器变化与拒绝、world/pipeline/UI退休、容量边界与图形菜单检查。构建依赖解析和冻结源码验证通过。

此前关闭动态模糊的对照成功出图；改抗锯齿质量或单独关闭 UPSCALESCREENPERCENTAGE 没有解决问题。临时设置均已恢复，最终验收使用动态模糊开启的原设置。中间 Output2/3/4 不作为最终验收版本；Output3 只识别中间输出的假设已被实际目标诊断修正。

用户授权结束另一项保留测试并恢复现场后，本轮确认 PID12448 已由原协调任务正常关闭并恢复，随后继续验证。后台测试使用认证的置后窗口构建、只读 tiny-output 观察、临时静音和禁用物理手柄后台输入，没有切换到前台或调整用户窗口。游戏上次未成功启动的原生恢复对话框，仅在精确测试进程、标题/正文与按钮身份匹配后后台选择“不恢复默认设置”，不重置玩家画质。

物理头显尚未在本轮复测，headset_verified=false。没有生成发布包。最终清单/构建回执为 analysis/runtime/startup-output5-stage.json、startup-output5-build-tests.json，旧默认清单保存在 startup-output5-previous-manifest.json。

## 证据身份

后续用户反馈“我刚刚启动真机测试似乎已经没问题了”。核对最近手动 VDXR 会话 continuous-stereo-38848-20261020-184757-c3832acd，实际加载 StartupOutput5，相同 native SHA256，累计33475对接收图像、99190次投影提交。这是用户对本次基本可用性的反馈，未授予全部DLSS开关、画质、跨世界或帧率验收；性能候选 NgxManual3 尚未启用。原始反馈及会话身份另存 ignored analysis/runtime/startup-output5-user-headset-feedback.json。

本轮先检索 tools/re-kb/kb.py 和 analysis/re-kb/knowledge.sqlite，读取已有定向证据与 docs/58-reverse-engineering-knowledge-base.md，没有打开 IDA 数据库或扩展全游戏缓存。历史运行时代码 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、模块基址0x140000000，与磁盘 EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4` 分开保存；实际运行仍须字节匹配，相同 RVA 不代表相同版本。快照、衍生代码、SQLite、私有构建与图像均留在 ignored analysis/。
