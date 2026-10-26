# Lancer原枪组件读回与骨骼配对观察

HH阴性与修正候选（2026-10-27）：PG80b826实跑只取得肩部3行，3／3原component／Avatar配对有效，Muzzle前后均未录，没有derived world；socket0／bonecount0等默认值不能解成场景原点或合法枪骨。两个样本早于原phase起点179.369／70.1742ms，第三个开始只晚0.2454ms，不能外推整段。原40／6秒门0→35／6.0179205s且Stop续更超时，正常106／音频9／9恢复仅cleanup；原失败与Body交付保留。Root只解此局21648B PG日志，没有重解held或调查非阻断Native拒绝，回执`hh-muzzle-negative-saved.json` SHA `df7d7cacc300a7871781330796e62bbb92c8a1d3a1efeab8ccec56a176381636`。

随后确认新增reader存在具体schema错误：实际保存反射／Core盘点显示Vector、Rotator的owner链是`Core.Object.Vector/Rotator`，旧reader及合成正例却要求直接Core package。限定修正为精确ScriptStruct→真实Core.Class的Object→无outer的Core，保留extent12、所有selector／身份／动态值复核；旧直接Core或假owner不接受。这是真实过严条件的源码修复，HH没有实际分支记录，尚不能把它认定为该局首个／唯一拒绝，亦不归因身体协议失败。仅追加已有实际返回的step／pass／metadata Reason，诊断不增加Game读／clock／guard，未创建Reader保持UNKNOWN、实际none保留0；失败Socket不发布。

修正后PG SHA `207cb5373f5ac675a24efb64a6f08237f17c72b2662fb6d046b80b0855f8f231`，相对80b826只变4个观察文件，原compiler flags与16导出保持，OFF probe仍精确HG父源。完整PG构建与42个受影响Socket owned案例通过；未改的枪骨位置数学及原测试源逐hash相同，12例复用不重跑。ignored `analysis/lancer-muzzle-observation-20261027/attempt-1793054247587862700/receipt.json` SHA `be549385365f5f786b94f7fb11832ff1b9d5025aa7e5fb7a7851ebfa6ee4f012`，`artifact-audit.json` SHA `90be72f306dbe4c7715960d9aeeb5a8f78917c6b489ed9827e5152412d481d17`封存历史资料hash、四文件增量、旧阴性及诊断限制。尚无修正后的实际Muzzle／玩法或默认整合结论。

当前Muzzle观察候选（2026-10-27，尚未实跑）：新增默认OFF的`GEARS_PLAYGROUND_LANCER_MUZZLE_PROBE`，必须复用已有component probe。普通PlayerTick在同一个有界采样中认证实际weapon.MuzzleSocketName、asset.Sockets及唯一SkeletalMeshSocket、真实Core.Vector／Rotator描述符和相对值，并读取当前9骨Ref／Local／Space／epoch前后副本；再按实际BoneName唯一匹配骨index，以UE3行向量关系计算挂点位置。名称、指针、字段selector／descriptor、动态值、完整身份／结构／table均复核，失败输出不变；非Actor对象只认证当前注册身份，不声称未知pending-kill flags已验。没有引擎getter、骨骼写入或新的持续观察平台。

新`muzzle_pair_accepted`独立于原component／Avatar配对，只有原观察、socket前后与枪骨前后均一致才给出derived world位置；默认值／未知不当成功。raw配置名、socket名、BoneName、相对location／rotation、当前骨index／count／Space矩阵和原枪CTW一起记录。旋转只保留原始Rotator，未擅自设定bore；getter等价／权威枪轴／sameC0／最终renderer仍明确未证明，mount_known=false与R2门不变。

候选直接从HG failclosed PG `dfd995e4…904669`的实际frozen source2及原17份vendor源派生，只变2既有／加5文件，811份闭包稳定；原FLAGS保持，DEFINES只增Muzzle观察。完整PG严格构建、12个9骨／位置owned-memory案例及31个真实typed socket-reader案例通过，16导出保持。PG SHA `80b826235774f6b9126db0017fdc24f1867cc2127e99b68bb6ddfb436a3db150`，回执ignored `analysis/lancer-muzzle-observation-20261027/attempt-1793053485363798500/receipt.json` SHA `63bde1d5b8f4de45c1dcd344af6461977b65a5102dfcda6e23a5d7464c662809`。预处理去除新OFF块后的probe文本精确等于实际HG父源；未重跑旧suite或重建Native／XR。首次stage遗漏manifest独列vendor导致configure失败，下一stage的路径规范化断言也拒绝，原失败目录保留，均未到编译／Game；只修build driver后才得到本候选。上述CPU产物尚无实际Muzzle观察或玩法通过，不替换默认。

HG更新（2026-10-27）：Body已通过原持续持枪／Stop续更／正常恢复协议，Root仅消费该局现成32715B PG日志，不重解128条held journal。9／9组件观察与骨骼前后配对接受，间隔4.9722–5.6081ms；gun相对bone52继续维持原固定-90°关系，最大矩阵元素残差0.00024615（世界坐标文本9位有效数字）。同raw输入相邻样本最大枪位移32.43099原单位／旋转32.77917°，腕35／骨52与枪一起变化，角色CTW相同；仍不支持枪组件单独脱离，也不能把协议通过当最终稳定可见跟随。新证据为ignored `analysis/lancer-component-readback-20261026/hg-saved-component-review.json`，SHA `bbe78ada3f355fad5990b13b0aad000ab949a1aa59dd612c2fad9c246a38d01b`。原v1／v2导出、局部非阻断拒绝及旧EQ／ES结果保持。

Body从既有HG hand-v3／BMS前缀确认：PG epoch1295／1297分别可与同mesh的Native seq1／2有序关联，实际consumer／end已验证，PG读回发生在对应Native end之后；其余ready／move六个epoch1395..1420均超出32条前缀末1363。该匹配不是原子同C0或最终renderer证明，当前没有以非原子summary推定后继writer原因。下一最小观察缺口为当前实际Muzzle socket对象／BoneName／RelativeLocation／Rotation及骨index/count和对应枪骨姿态，继续复用唯一现有PG有界读路径，不另造骨骼writer。

枪口资料边界另收紧：历史实际武器仅确认MuzzleSocketName=Muzzle；静态9骨盘点、作者+Y前向或旧Magazine socket getter均不足证明当前Muzzle的骨绑定／相对几何，更不授权威bore／最终射线。先闭合实际元数据／姿态与原getter，再沿现有CallFunction所有者接逐发观察。当前mount_known=false和逐发输入资格不因组件观察而放开，R2／散布／回缩／前推／命中仍未验收。

ES更新：在现有PG采样点取得9／9稳定骨骼／枪配对（约5.05–5.79ms），实际重算枪相对bone52矩阵在9条中恒定，最大元素差0.000029427。固定手柄输入下，腕35、bone51／52和枪姿态一起切换，Avatar CTW一致；当前观察不支持枪组件单独偏离骨骼。最终writer／动画更新阶段与GPU原因仍未知，已把保存样本交Body唯一共享writer所有者继续定位。ES动作整体仍失败：Stop quiescent后12条采样没有新的hands提交／尝试，连续tick却552→630，原coordinator返回1。正常finalizer一次及106／十文件／audio／入口恢复通过仅属cleanup。详细证据见`es-saved-pose-review.json`与PROGRESS；未重跑或放宽门。

Body返回保存数据对照：rows4–6附近joint观察的MainMount提交计数保持45，rows7–9附近保持54，hands／Native提交计数也未变；前32条journal已在epoch1612结束，早这些PG样本约1.75–2.93秒，不覆盖当前姿态。回执`analysis/cover-implementation-20261024/es-native47-pose-timing-review.json`（Body worktree，SHA256 `3054388d…3af81a`）及其三份输入hash已由Root核对。约0.2秒的非原子摘要括号不能证明精确PG采样点没有实际写入；不能用它给动画／另一writer定责。这里只追加已有证据，不重复分析或新开游戏。

ER实际运行更新：原动作45／8／8次held提交与正常恢复通过，39条原coherent姿态均保留；新增外部pair 0／39接受，38次前后骨骼比较变化、1次Query结束字段／registry变化。配对区间中位35.4808ms（28.7729–55.0061ms）；拒绝记录不含after字段，不能倒推具体变化项。原PG组件9条均接受，但没有同骨骼稳定配对。ER保留原件，不重跑；下一候选收窄到既有PG组件采样点内的只读骨骼数据，保留sameC0／GPU／枪口／射击边界。结果在ignored `er-saved-pair-review.json`及PROGRESS。

EQ实际运行更新：Root在Body明确交出空窗口后完成原动作一次并正常finalize，Game20012、原持枪门与后续ready／移转提交通过。9条组件观察各自有效，包含约10°对应旋转；同small-move输入和epochs的后继样本却又偏移14.93523游戏单位／24.06287°，距离阶段结束尚640.34ms。角色CTW相同；这些数据既不能宣布稳定跟手，也不能把分叉直接解释为GPU闪烁、动画覆盖或Native回退。正常清理与恢复通过，窗口已交回。原件和分析在ignored `analysis/lancer-component-readback-20261026/eq-saved-component-review.json`。下文“待运行”保留为先前封存时的历史说明。

当前唯一后继为外部只读配对观察。生产入口`tools/game-playtest/weapon_component_observation.py::wrap_probe`包住既有私有`body_observation.probe`，不改原函数体、原host调用和成功条件。两次原骨骼观察包围实际枪组件、Avatar矩阵、typed资产、唯一实际weapon_end attachment读回；再对属性选择、反射布局、身份、动态字段、Ref115、骨骼epoch／Space／Local／CTW及全局注册表做结束重复检查。成功记录原枪矩阵、weapon_end世界矩阵及枪相对骨骼矩阵；失败仅附加拒绝原因，返回原成功姿态。有限顺序读取只能筛掉观察到的变化，显式保留`same_C0_not_proved`与`final_render_phase_not_proved`，没有调用引擎getter或写游戏，不生成射击权限。

16项owned定向测试使用实际pair reader和原`world_state.Query`、假的memory／resolver／pose callback验证正常、读取中变化拒绝和错误隔离等边界；原live骨骼算法和游戏／GPU不在此测试范围。命令为`python -B tools/game-playtest/weapon_component_observation_test.py`。不为这次Python观察器重建未改Native／PG／XR；ER只复制该helper并在既有私有入口追加wrapper，再运行原取枪／移转动作。详细结果持续记录在`docs/releases/0.0.4/PROGRESS.md`。

ER已完成该接线并封存，准确源码`59e2cca`，pins `4618c3bd9ac62e0ad4d32f50cb7fe24b55aaf5465eee817a2ceb99465e5a4f3b`；实际import验到私有helper／Query和保留的原pose函数。Body后继共享query变化使首次clone被guard拒绝，ER转而私有pin完全相同的原EQ旧字节，保留拒绝记录且不修改共享代码。所有旧保护、8命令、40／6秒、DLL及finalizer保持；无Game／XR新运行。等FPS及Body按既有路线交接独占窗口后再执行，不能把封存当作分叉已定位。

EP已经通过原40／6秒持枪门及right-ready、小幅平移＋10°转枪的后续提交。Root复核保存的BMS／v3／alpha原件头和32条sequence／tick／Compose关联，独立重算v3腕FK位置与Local精确一致，并直接计到四角色各32次Applied、640事件无drop。BMS完整来源与新source目标派生复用Body严格reader，未冒充又做一次独立来源数学证明；前32条不能扩充成全部62条会话提交。Root没有为此重跑游戏。记录在ignored `analysis/body-mount-pg-candidate-20261026/ep-saved-root-review.json`。

下一项只验证实际Lancer组件的`LocalToWorld`读回是否随同一组右手移转变化，继续复用Native47／BQ／CY和原动作。它不改枪体、骨骼、相机、准星或射线，不开火，也不把当前仅有静态腕标定的profile标成完整WeaponMount。屏幕视觉、真实枪口／散布／回缩／前推及最终命中保持待验。

新私有开关`GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE`默认OFF，依赖typed Body provider，排斥另一个原bone-getter observer同时运行。普通PG PlayerTick只将现有`copy_frame_on_gt`已实际获取的Game观察保留给只读延续，不增加第一次Game读取、不改原Frame方法；读取后以原`source_game_on_gt`和`same_game`、holdGeneration、domain和Frame有效期结束复核。旧V1 Frame继续按自己的资格验证，过期就拒绝，不借新typed metadata witness创建正常GT引擎调用权。

采样直接读反射结构、原枪`MeshPoseProbe`、两个typed资产与唯一右手attachment：枪与角色的`LocalToWorld`各自验证StructProperty／实际ScriptStruct Matrix／64字节、身份和descriptor／inner／extent／矩阵重复读取。记录前再比对第二次component／asset及枪可见性、absolute设置、Translation／Rotation与矩阵。没有调用MatchRefBone／GetBoneMatrix／socket getter，没有第二次bridge采样、Native重入或任何写入。两次Game包住这些字段读取；异常／改变／过期记录拒绝，不把部分矩阵当成功。

采样只存限流策略，不存许可：最多8个实际parent／对象／hold／reference／input epoch身份，每个24次尝试。每次明显平移（5mm）或旋转（2°）后取3次，间隔至少50ms；静止输入不会随Frame推进重置预算，q和−q不当旋转。这样保留后继ready和移动阶段的观察机会，不把全部样本提前花在固定肩部。碰到身份容量上限直接停，不轮换或无限重试。

输出为有界`lancer_component_observation`行，包含完整parent时间／进程／注册／Frame关联、实际component／asset地址、源raw right Grip、source／session／space／profile／input／reference代数、原枪与角色矩阵、时间和拒绝原因。`accepted`只表示这次metadata重复读取及原Frame复核通过；每行明确`observation_only=1 source_is_parent_V1=1 same_C0_not_proved=1`。它不是同C0原子姿态、GPU已画出该矩阵、真实枪口位置或射击许可。分析必须按输入阶段及真实时间关联，不能仅凭source_sequence相同认定同步。

定向验证：采样预算47个断言通过（静止Frame推进、q符号、微小抖动、纯旋转、24／8硬上限、换reference／input epoch及非法pose）；实际typed矩阵reader的8个owned metadata案例覆盖正常读取、错误尺寸／属性类型／struct类型、不可读、NaN和读取中descriptor／矩阵变化。实际PG完整TU加唯一新probe flag严格`/Zs /W4 /WX`通过；本轮没有重复未改的Body／Client／source tests。准确最终回执在ignored `analysis/lancer-component-readback-20261026/`；完整PG构建和实际原动作结果应另行追加。

后继交付：源码`28babdd`已完整编译／链接为PG `38d897b4ff4f355b958874235327a1851a9acb5fc9c7bfa05be91774d3a68912`，783源与vendor稳定，实际生产只增加上述只读probe flag。EQ以原EP动作封存交Body，pins `2e8591fa29b289d62d572cdcdf7c5a3c4957b61e18dff0e00bc98951947986ac`。Native47／BQ／CY／INI、原40／6秒门和106保护均不变；Root未启动Game。现阶段仍等待实际读回，不能因组件采样已交付就把枪体／枪口／视觉／射击标作通过。
