# 主菜单显示与世界重建恢复

2026-10-16，V1未完成项。目标是同一游戏进程中关卡→主菜单→继续游戏，VR会话持续存在，菜单可读，加载后恢复新世界的真实左右眼。当前9w实际证明这条路径尚未实现，不能把已通过的同关卡检查点重载等同于主菜单重建。

## 已有运行证据

见文档69的9w末段、ignored `analysis/runtime/menu9w-anchor-difference.json`、`menu9w-return-main-native-status.txt`。localplayer保持身份，而controller/camera/pawn/level被销毁和替换；旧level地址复用为CoverLink。旧证书严格拒绝，发布计数不再增长。正常停止清理通过，不能重新激活同一个已加载Native9；`start_capture.py`保留禁止重复注入的限制。

## 拟采用的恢复边界，尚未实现

一次Astra Medium只读审查后，优先由owner重建完整证书包，native在可恢复静止状态下两阶段提交，保留永久stop语义。避免把反射解析和所有builder直接搬入渲染回调。

1. owner以PID/creation和原localplayer完整身份为起点，重新发现controller/camera/pawn/level/WorldInfo，复用现有anchor/UI builder、属性及代码字节校验。包包含owner身份、单调world generation、前代和摘要；native解析到pending，不能直接覆盖全局配置。
2. 暂停新双目提交，在自然scene/family边界退休旧CPU scope/renderer及GPU槽；不持有回调需要的锁等待。native复核证书、strict_guard及新相互关系后，再原子切换整套anchor、expected_pc/camera/local和camera_cache。失败不启用新绑定。
3. receipt、图像票据、UI身份与world generation关联；旧代渲染、迟到线程、UI及保留图像不能复活。TLS在所属线程下次入口失效。新world提交完成图像后才恢复projection；不能清空生命周期容器假装对象已释放。

实现前先确认现有锁、renderer释放与GPU排空能支持可恢复静止状态；缺少条件时保持拒绝，不以身份失败重试冒充恢复。必要反例包括地址复用、pending期间再次换world、旧线程迟到、旧UI/票据、重复commit及失败rollback。

## 主菜单画面源

现有Pause共同quad依赖有效双目票据，不能覆盖无玩家anchor的主菜单。已有WGC捕获整幅窗口，但缺双目票据不证明图像是mono；frame_link只提供有界CPU关联，也不覆盖全部clear/copy/indirect/bundle。

拟沿同一GPU槽/receiver增加正面认证的菜单布局与完成图像，不另开争用consumer。必须证明主菜单完整输出实际完成，并绑定菜单epoch、Present、chain/queue/backbuffer身份与resource generation。未知写入、旧buffer、延迟Execute、TEST/失败Present和resize均不得误认为mono。若采用mono类型，它不可进入双目projection验证，返回游戏仍走新world的新票据。此处尚需实际最终合成路径证据，不能仅依据截图或 `!linked` 发布mono。

主菜单可见与新world双目恢复是两个必要条件；当前只修退出确认框，不宣称这两项已经完成。

## 第一批恢复基础接口，尚未接入live换代

2026-10-16新增以下边界，仍不把主菜单恢复标为完成：

- `anchor_config_loader.h`把schema1/2解析到局部Config，全部字段成功才赋out；启动时仍需expected身份和原bounded_guard全部通过才更新全局。补充拒绝数字溢出与截断。UI解析同样先使用局部候选、完整validate后才赋out/全局，未改变绑定允许范围。
- `world_epoch_gate.h`提供不排队的单次CAS读者进入及零读者关闭；调用方负责TLS最外层/递归管理。关闭时不得自旋阻塞引擎回调。owner与admission epoch拒绝重复/旧token；该epoch不自动等于已提交world ID。尚未接到Scope和配置交换。
- `Lifetime`增加独立quiesce_close、单锁snapshot、销毁确认及下一代reopen。原close和request_stop仍永久不可重开。pending清零不构成销毁证据，调用方必须核实native release返回及destructor完成后才确认token；接口本身不执行engine释放。
- `GpuPublisher::try_idle`非阻塞读取同一锁下的generation/serial和ready/done fences，拒绝生产端in-flight、重建、失败/停止、device-removal哨兵及未ack的消费端。它只是快照，正式commit还须先禁止新发布并重验；不能作为独占资源的替代。

新编译目录 `tools/native-capture/build-world-recovery1`采用与Native9一致的游戏分辨率、UI及graphics-lock开关，产物名GearsCaptureWorldRecovery1；未替换冻结Native9，也未接入默认或候选manifest。完整DLL与目标测试编译通过。AnchorConfigLoaderTest的124检查、WorldEpochGateTest、SubmittedViewLifetimeTest，以及DLL ABI/continuous/frame-link自测均退出0。

OwnedGpuShareTest在实际D3D12队列上验证三个受支持资源状态、两帧完整左右图像：ready复制完成但done未ack时try_idle拒绝，消费端ack后通过；另以快照反例拒绝device-removal哨兵、重建和in-flight。debug layer在本次环境不可用，未宣称debug-layer验收。GPU slot协议版本未改变，尚未加入world generation、XR旧图退休/只ack路径或旧command-list tombstone；owner重取与native两阶段commit仍待接线。
