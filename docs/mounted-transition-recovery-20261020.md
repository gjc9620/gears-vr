# 2026-10-20 机枪上下机黑屏与退出：第二轮处理

用户真机复测仍黑屏，下机枪后出现约三秒 Waiting for image，随后退出 VR 模式。本条目不能作为机枪修复已通过的验收记录。

## 本次失败证据

会话 `continuous-stereo-30768-20261020-153714-f929944f`：游戏 PID30768、creation134369554215945533；native MountedFps1 SHA256 `9505245bd8ba18632b6faac1d1c39232be7a069cffbd3164ae0c5e0e9151c07e`；XR PauseStereo1 SHA256 `afcde6794e004852de0f2965c54cd0a6522136e28a2db3d979d636b783238e1e`。

正常步行的 generation2 曾提交并出图。恢复器随后生成了 Turret_TroikaCabal 的 mounted 证书，但提交到 native 前用户已经下机枪，`anchor_before` 拒绝；这不能当作 mounted 提交成功。随后普通 Pawn_COGMarcus 的 generation3 提交，早期票据14739在 owned UI `command_context` 校验中停止。最终状态还包含 GPU tracker `prepare_execute` 错误12、pipeline 原因4；旧版没有足够先后时间与分支细节，不能把后两项认定为最初故障。XR 记录 `producer_stopped` 并正常结束，符合画面来源停止后退出的反馈。

原始会话、native 配置/状态、world证书和压缩分析保留在 ignored `analysis/runtime/mounted-black-30768-20261020/`。辅助只读采集器在真正上机枪前结束，且检查点切换后仍沿用旧相机身份；其失败采样不能代替实际 mounted 角色证据。

## 候选改动

- RuntimePreparation 仅将已发现相机位置作为提示。每次使用新 Resolver，重新认证模块、注册表及 controller/camera/local身份，并完整运行当前拓扑与 mounted guard。身份改变即关闭该 reader、重新全局发现；提示不缓存成功证明。
- 每次 owned 资源重建清除并行许可，累计完成帧计数保留。至少一帧完成 CPU 证明并经过当前资源的真实输出边界后才恢复 pending 帧。GPU边界先于 CPU尾部时，在已满足 `complete && can_advance` 的推进点确认。原有 target、ticket、双眼、UI、队列及资源退休检查继续生效。
- 记录首次 command_context 拒绝的具体子项、receipt/active/pending、资源/world及GPU错误。GPU prepare_execute 首次拒绝另存 QPC，通过发布标志保护并在周期状态保存，方便区分最初故障与停止后的连带失败。

候选使用 StartupOutput3 的冻结源码作基线，保留其普通输出分支修复、校验缓冲区修正、武器启动重绑定和 SeatedReticle1 准星修改，详见 [启动输出修复](161-vr-startup-output-fallback.md)。未纳入工作目录中无关的 DLSS 实验改动。

## 普通输出的第二步复制

Recovery5 的实际 session31700 已通过代码身份与 native 激活，但仍在 ticket2 的 HUD 绘制拒绝，flags258、pair0、owned_error0。其有界 `late_quad` 记录证明：`d83cbf` 首先绘制到3840×2160中间资源 `16608fdaf0`；随后 `2b11a0` 将同一资源作为输入槽2，绘制到当前1792×980 owned目标 `229fab0`。左右眼均使用896×980的原像素矩形，复制源纹理为3840×2160；它不是把全幅纹理缩放为一只眼。原规则只承认第一步，最终 owned目标上的复制仍被计为未知绘制，因而没有可供HUD使用的双眼记录。这是两步输出，不是已证明的嵌套hook。

先以 `kb.py refs/search` 查询 artifact1 的 `0x1402b0c04`，再读已保存定向伪代码。历史函数含主段`2b0c04..2b1255`与冷段`8ee78c..8eeb7a`，其缓存伪代码SHA256为`a53d03613b2ded23d13b329bb4a3e1a934335b83e41617e3a14c280ebfa0332d`。本轮没有打开新的IDA数据库或重做反编译。历史blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、module base `0x140000000`，与游戏磁盘 EXE身份分开保存。

Recovery6增加对这个最终复制调用的识别：必须为当前owned renderer、同一冻结world/ticket、已认证调用代码、原像素左右半幅映射，源纹理不得小于输出且双方尺寸均有上限。真正目标资源、viewport/scissor、列表、双眼数量、队列与GPU输出检查仍执行。启用时额外核对完整主段SHA256 `5088f5f7fb8c718769016b98194413e42aa466a30bfdfeab01d081c07075515d`和冷段SHA256 `024e4777801ff3236360d5ead3b10a2a02717438020dd634537a47723453e138`；相同RVA不代表匹配代码。回归直接重放上述实际几何，并拒绝尺寸不足、越界、UV偏移、错误角色/代码/world和控制输出。普通`d83cbf`的源目标同尺寸条件继续保留。

## 检查与限制

候选 MountedRecovery6 SHA256 `31db7c69c75c11abae6de90970be26cde5c4fb0284ef30be9693b721188660e1`，构建依赖479项。13项组件检查及22次启动自检通过；之前相同 Python 提示逻辑的 world/mounted/anchor等138项回归通过。私有源码、构建和测试回执分别在 ignored `analysis/mounted-recovery6-source/`、`analysis/native-build-mounted-recovery6/`、`analysis/runtime/mounted-recovery6-tests.json`。

较早的Recovery3 session36156在同一类普通Output分支拒绝。Recovery4 session12448发现旧校验缓冲区不足，在安装显示hook前拒绝。Recovery5 session31700补充上述第二步复制证据。这些版本均未提升为默认，五份受保护设置/存档文件已恢复，音频守护记录恢复原静音状态。Recovery6首轮后台启动在可用窗口出现前超时，没有形成native渲染验收；游戏已退出并恢复设置。随后保留相同DLL、VR尺寸及后台策略，以1280×720桌面输出重新执行检查点回归，回执前缀为`mounted-recovery6b-actual-20261020`。后台使用认证的置后窗口构建、静音、只观察输出，保留禁用物理手柄后台输入。

物理头显 mounted 输出尚未验证，`headset_verified=false`、`mounted_runtime_verified=false`。新增只读采集器支持检查点身份替换并观察1200秒，避免上一轮过早结束；采集不授予显示或输入权限。没有生成发布包，没有新增游戏反编译或操作 IDA 数据库。

截至17:03，第二次后台回归仍在等另一任务遗留的游戏PID21640（creation time对应16:57:04）退出，尚未启动本轮游戏。该启动器已结束；本轮没有操作这个外部进程，已请求用户确认是否正常关闭它。Recovery6仍为独立候选，没有改成默认。`promote-mounted-recovery6.py`只在实际回归、两次检查点恢复和设置还原都通过时才允许更新默认的三个native字段；只读真机采集器尚未启动。

## 17:30：后台游戏回归通过，切换默认

用户随后明确授权正常关闭PID21640并继续验证。重新核对时该进程已经退出，无需再操作。此前只等待游戏空位的6b监督器已在十分钟限额到达后结束，原超时日志保留为`mounted-recovery6b-wait-expired.log`；检查无人占用后重新运行同一有界回归。

本轮实际会话`continuous-stereo-26524-20261020-172829-06615995`使用上述相同Recovery6 DLL、MenuSmooth1 XR、标准置后模拟器，桌面1280×720、VR scale0.7、每眼896×980。首次立体投影实际`xrEndFrame`成功，native最终发布4418个画面对，generation从1升至3，两个handoff commit完成。两次检查点恢复均取得新world的XR投影回执与1792×980的当前GPU源图，`saved_verified=true`、`stage_verified=true`。

最终UI首次故障为none、owned_error0、pipeline_failed0；pending选择4923次，证明首帧保护没有使整局长期停留在串行模式。该累计值不是FPS或物理显示流畅度的测量。原生清理通过，独立入口还原79项全部匹配，XR/owner正常结束，游戏正常关闭；五份设置/存档均逐字节还原。音频守护`restored=true`、`sessions_restored=1`、`restore_pending=0`。完整回执`analysis/runtime/mounted-recovery6b-actual-20261020.json`的`passed=true`。

通过受控提升脚本仅更新默认清单的native目录、DLL名与摘要，`tools/vr-launcher/manifest.json`现指向`build-mounted-recovery6/GearsCaptureMountedRecovery6.dll`，XR继续使用已测MenuSmooth1。原清单与提升回执分别保留在`mounted-recovery6-previous-default.json`和`mounted-recovery6-promotion.json`。

本轮没有实际控制机枪，故`mounted_runtime_verified=false`、`headset_verified=false`继续保留。随后尝试启动额外只读角色采集时会话已经结束，采集器在身份检查处拒绝，未生成角色采样，不能作为采集器运行验收。下一次手动真机会话的1200秒只读采集器已准备并启动等待；不会启动游戏、发输入或改变窗口。物理上/下机枪的画面与退出行为仍待用户复测。
