# 6DoF 掩体碰撞接入顺序

本文件把[交互规格](6dof-cover-interaction-design-20261024.md)中的抓边与探身约束落实到 Native 查询和 Scene 提交事务。它是实施门槛，不代表功能已开放。当前 `cover_shared_input_live::prepare` 仍向输入 owner 传入零范围和空边缘；CT17 在同一局、同一槽位以零尺寸射线将一处矮墙挡面的顶沿夹在地面以上96单位命中与112单位有效空返回之间。该结果仍不能认证抓取边缘或身体碰撞体积。

## 1. 调用与事务边界

沿用现有的唯一 GT Scene 作为左手仲裁和身体提交点：`cover_shared_input_live::prepare` 在 `participant.begin` 前取得已认证的边缘和活动范围，在 `participant.update_with_decoded` 后、武器 broker `commit` 前认证已提交身体位置到候选位置的连续扫掠。使用该 Scene 已冻结的 Pawn、CoverLink、slot、双眼、头显和左手样本，不增加 Tick 或 ProcessEvent 写入阶段。

Native 中只实现绑定当前零售版本的 Actor.Trace 查询。2026-10-27审阅将下一次 **observation-only** 探针改为现有外层 meshTick 的普通 TickLatch 路径：`continuous_hand_owner_live::tick_begin` 在 `begin_tick/read_tick/owner_current` 成功后、原 meshTick 调用前，以真实 Entry、invocation、owner、Core 和 world lease 为根；独立 sampler 复核 `binding::current_actor`，只在短 binding mutex 内复制并检查 metadata，释放局部锁后才允许引擎调用。不得通过启用依赖身体writer的 `BodyTickInput`／`BodyMainMountSource` 来取得读权限，也不得伪造Scene Receipt。该插入点目前只有冻结源码可达性审阅，没有本次运行证明；原Scene原型的 `scene_lock_certified=false` 保持关闭。

探针配置保留现有continuous/stereo/anchor前提，启用 `ContinuousHandOwner`、`RigTracking`、`WeaponHandParticipant` 的传输路径；FullBodyC0Native及hands/arm/mount、Goal、两个BodyTick来源、旧body/input/camera和engine_restore均关闭。另核对没有会写入的注册参与者；无关FreshPose/Atom观察器保持关闭。六个现有hook的认证、预留和安装仍有开销，不能把该配置称为无hook或完全无副作用。

查询前后核对同一真实Tick入口、owner/source当前性、无冲突引擎调用上下文及原50ms来源鲜度；记录同步调用耗时、回调重入、嵌套和正常清理。Actor.Trace的NativeIndex277、116字节参数结构及原始字段链、GNatives277、完整代码字节和所有被用对象的注册槽仍须认证，区分命中、有效空返回、证书失败与调用结果不明。Python／Playground继续用于取证，不能成为生产帧的异步授权者。

私有观察原型candidate1的现场入口仍有已确认缺口：它先调用旧 `neutral::sample` 和 `geometry::sample_with_impl`，要求旧body的metadata/layout/GT与hook所有权；旧 `CoverBodyPrototype.enabled=0` 时这些条件不会建立，而且neutral路径还可能调用原生头骨getter。不能只跳过 `body_ready` 或打开旧姿态writer来取得读权限。后继独立 `CoverTraceContext` 应从 `weapon_hand_scene_binding` 成功的真实Scene绑定取得调用者持有的metadata快照，另行认证少量CoverType/Link/slot/世界/位置字段；完全绕开neutral校准、geometry变换和骨getter。共享binding mutex只用于采样/快照，不应被带入未来引擎查询调用；Scene/stereo锁的调用安全仍需独立验证。

candidate2已实现这条独立被动路径，完整假Scene在旧body metadata/layout/GT均为初始且三个旧运行时开关关闭时可达 `scene_lock_unproven/called=0`；20次CT0等待也不会耗掉进入CT2后的证书尝试。但它尚有已发现的注册表观察历史缺口：identity内部每次header只两两比较，没有把全部实际读取纳入统一count历史，可能掩盖已见增长后的回退。该已编译候选保留为待修正证据。另有静态预算问题：历史WarfareGame包中的WarPawn直接声明为933项，而candidate2的find会保存全部遍历身份，容量为512，并为多个目标重复扫描；历史规模不等于当前runtime身份，但已足以要求先优化有界遍历与证据存储，不能仅凭小假Scene宣称真实证书可达。后继私有candidate3先修观察历史并保留初始依赖范围，实际规模与成本仍须验证。

针对历史运行时代码的9个窄函数机械导出保留在 ignored `analysis/cover-native-trace-lock-audit1/`。包装函数完整chunks与CT17证书相符；导出仍有21处未解析间接调用，并明确包含引擎临时全局写入。查询语义不能证明整个调用无副作用或无回调；这些静态记录不构成当前进程helper身份、Scene锁安全或异常恢复证明。

未来位移管线须让Scene产生纯数据请求，下一Tick查询，再由新的Scene使用当前ticket重新仲裁和认证结果；不得跨Tick保留旧ticket、broker nonce或手lease。独立query ID绑定原Scene、已提交offset版本、起终点、体积、轴、边缘、对象cohort及时间。新Scene只接受从同一已提交位置沿当前候选方向的连续前缀，并复核World、Controller/Pawn/Mesh、Link/slot、动作和XR连续性。只有身份和鲜度仍不足以证明Tick至Scene之间的几何／动态占用不变；碰撞版本或执行阶段证明尚待解决。当前真实头显变化导致身体体积变化时，旧查询不能继续授权；保留旧虚拟位置也须有当前占用证据。等待查询需要同ticket的零增量接受路径，不能用会释放grab的 `complete(false)` 冒充普通等待。

限制每帧射线数、距离、搜索层数和总耗时；同步引擎调用一旦卡住，普通超时计时无法中断它，因此时间预算只限制后续调用和许可。上述Tick管线尚未接入，不能据此开放抓边。

## 2. 几何与位移许可

CT17 已在同一局、同一个 Link／slot／墙面 Actor 与组件下，用下方命中和上方有效空返回夹逼一个上沿候选。接下来结合原生 `CanPopUp`、低掩体资格、手到边缘的抓取距离、接近侧及手部路径，形成单一边缘身份；多个候选或任何身份变化均拒绝。边缘身份绑定 World generation、完整 Link 身份、slot、实体 Actor／组件及几何版本。96～112单位的零尺寸射线范围不能直接授予 LGrip。

先只开放经连续体积验证的一条轴；另一轴固定为零。现有 `CoverObservation.range` 合同要求整个二维范围有效且包含中立点，稀疏射线和只查四角不能证明它。以后开放左右与上下同时拉拽时，要证明整个组合范围的身体体积合法，或先明确修改并测试该合同。

抓取只在新按下沿且手处于 acquisition reach 内成立。抓住后锁定该边缘与操作轴，手离开初始抓取距离属于合法拉拽，不应每帧自动释放。每帧从上一次已提交身体位置向候选位置做非零 extent 连续扫掠；只在有可靠接触距离和余量时提交已证明的连续前缀，否则拒绝本次虚拟位移。静态墙面、动态 Actor、可破坏物、起点穿透及枪口／眼点遮挡分别验证。真实头显平移继续直接生效，虚拟位移限速不能裁剪它。

`CoverInputOwnerCandidate` 需按当前 ticket 暴露不可变 pending offset／view，并允许同一 ticket 接受已认证的裁剪结果，使内部交互状态、视点和身体目标共同提交。`complete(false)` 的旧偏移 fallback 也必须有本 Scene 当前占用的独立碰撞证书；原有范围再认证不等于无碰撞。证书失败、命中歧义、预算用尽或身份变化时，不授予新的抓取或位移，不把失败的手租约带入下一帧。

## 3. 开放前证据

1. 在现有外层meshTick、局部Mod锁已释放的真实普通TickLatch路径验证只读Trace，记录实际入口／来源、重入、调用耗时和正常清理；另证明查询结果至新Scene提交之间的几何当前性。
2. CT14 已补所选名称槽指针见证，CT17 已在同局完成上沿候选命中／有效空返回夹逼；旧 CT12 记录不追认。下一步需认证抓取边缘的实际身份。
3. 验证合法与非法手部接近、手到边缘路径对照。
4. 对实际选定的身体保守体积验证非零 extent：有效阻挡、完整空扫、起点重叠和接触点语义，并确认动态阻挡范围。
5. CPU 回归覆盖拒绝扫掠时 lease／offset 无泄漏、裁剪后交互／视点／身体一致、失败 fallback 不绕过碰撞门、同 Scene 重试不重复调用，以及 slot／epoch／action barrier 撤销旧证书。

这些证据仍不能替代实体头显、受击区域、权威枪口遮挡和完整左右／上探玩法验收。
