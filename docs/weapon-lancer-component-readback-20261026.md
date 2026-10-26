# Lancer原枪组件读回：下一最小验证

EP已经通过原40／6秒持枪门及right-ready、小幅平移＋10°转枪的后续提交。Root复核保存的BMS／v3／alpha原件头和32条sequence／tick／Compose关联，独立重算v3腕FK位置与Local精确一致，并直接计到四角色各32次Applied、640事件无drop。BMS完整来源与新source目标派生复用Body严格reader，未冒充又做一次独立来源数学证明；前32条不能扩充成全部62条会话提交。Root没有为此重跑游戏。记录在ignored `analysis/body-mount-pg-candidate-20261026/ep-saved-root-review.json`。

下一项只验证实际Lancer组件的`LocalToWorld`读回是否随同一组右手移转变化，继续复用Native47／BQ／CY和原动作。它不改枪体、骨骼、相机、准星或射线，不开火，也不把当前仅有静态腕标定的profile标成完整WeaponMount。屏幕视觉、真实枪口／散布／回缩／前推及最终命中保持待验。

新私有开关`GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE`默认OFF，依赖typed Body provider，排斥另一个原bone-getter observer同时运行。普通PG PlayerTick只将现有`copy_frame_on_gt`已实际获取的Game观察保留给只读延续，不增加第一次Game读取、不改原Frame方法；读取后以原`source_game_on_gt`和`same_game`、holdGeneration、domain和Frame有效期结束复核。旧V1 Frame继续按自己的资格验证，过期就拒绝，不借新typed metadata witness创建正常GT引擎调用权。

采样直接读反射结构、原枪`MeshPoseProbe`、两个typed资产与唯一右手attachment：枪与角色的`LocalToWorld`各自验证StructProperty／实际ScriptStruct Matrix／64字节、身份和descriptor／inner／extent／矩阵重复读取。记录前再比对第二次component／asset及枪可见性、absolute设置、Translation／Rotation与矩阵。没有调用MatchRefBone／GetBoneMatrix／socket getter，没有第二次bridge采样、Native重入或任何写入。两次Game包住这些字段读取；异常／改变／过期记录拒绝，不把部分矩阵当成功。

采样只存限流策略，不存许可：最多8个实际parent／对象／hold／reference／input epoch身份，每个24次尝试。每次明显平移（5mm）或旋转（2°）后取3次，间隔至少50ms；静止输入不会随Frame推进重置预算，q和−q不当旋转。这样保留后继ready和移动阶段的观察机会，不把全部样本提前花在固定肩部。碰到身份容量上限直接停，不轮换或无限重试。

输出为有界`lancer_component_observation`行，包含完整parent时间／进程／注册／Frame关联、实际component／asset地址、源raw right Grip、source／session／space／profile／input／reference代数、原枪与角色矩阵、时间和拒绝原因。`accepted`只表示这次metadata重复读取及原Frame复核通过；每行明确`observation_only=1 source_is_parent_V1=1 same_C0_not_proved=1`。它不是同C0原子姿态、GPU已画出该矩阵、真实枪口位置或射击许可。分析必须按输入阶段及真实时间关联，不能仅凭source_sequence相同认定同步。

定向验证：采样预算47个断言通过（静止Frame推进、q符号、微小抖动、纯旋转、24／8硬上限、换reference／input epoch及非法pose）；实际typed矩阵reader的8个owned metadata案例覆盖正常读取、错误尺寸／属性类型／struct类型、不可读、NaN和读取中descriptor／矩阵变化。实际PG完整TU加唯一新probe flag严格`/Zs /W4 /WX`通过；本轮没有重复未改的Body／Client／source tests。准确最终回执在ignored `analysis/lancer-component-readback-20261026/`；完整PG构建和实际原动作结果应另行追加。
