# 12 张武器交互规格卡

版本：设计规格 2，2026-10-28。已按[交互复核](interaction-review-v2.md)取消统一 R3 前置、加入部分装填退出，并修正 Boltok、扭力弓和两种手雷的机制。动作规则供实现使用；模型和原函数的依据分别见[模型工作单](model-work-orders.md)和[原版接入口](original-entry-map.md)。本文件中的动作“完成”是未来验收要求，不代表本次运行结果。

## C0 — 全枪共同契约

**输入与握持。** 沿用既定身体槽位：右 Grip 取主武器／手枪／手雷，左 Grip 按住抓支撑点、机构或弹药；左肩供给当前枪对应弹药。右扳机按原武器方式开火。首个明确换弹动作请求原 ForceReload，并在原接受时计时；不再要求先按 R3。R3 只作可选释放／兼容辅助，与物理路径共用一个事务，长按或重复输入不重启。握前护木的 Grip 必须释放后重新抓机构，扫过和失追跳变不能拆件。右肩武器 1、左肩武器 2、左裤兜手枪、右裤兜手雷；Troika 只在炮位操作。枪口方向来自武器姿态，HMD 只影响观察。

**数量。** 原实例容量 C、已装弹 L、可用备用 S 为依据。普通弹匣的新增量 q=min(C−L,S)，保留原有 L，不引入“真实丢弃半匣”的惩罚。取出新件先保留 q，插入确认才提交 q；持有但未插入的 q 仍属于原备用，禁止再次保留。逐发件每次提交 1。提交必须由原库存读回确认，不由手部动画宣布成功。所有枪不额外增加“膛内 +1”。

**旧匣与部分装填。** 旧匣重新插回仅恢复其原数量和可用状态，不补弹、不授奖励；松手回收旧匣显示不丢原剩弹。旧／新件具有不同 item ID，不能同手叠加或反复插拔重复记账。Gnasher／Boltok 允许在本轮目标全部完成前闭锁退出，已插数量保留；未满计划只作普通完成。完全没装新弹的开合是检查／取消，无奖励。

**几何与计时。** 物件类型、连续接近、朝向、有效插入行程全部满足才吸附；仅碰到装填口不计完成。入口固定在枪体上，不跟随被取走的部件。机械动作必须到达要求端点／回位；原动画角度不是玩家手柄的直接阈值。黄色只显示当前可操作区域，成功到位震动一次。最后必要动作的有效时间不晚于本实例原完美窗口终点，即提交一次完美结果；更早完成也允许。迟到普通完成，不自动补满、不强制卡壳。用原游戏时间；追踪丢失不能延长期限。未取得可靠窗口时不能授予完美结果。

**中断。** 换枪、读档、死亡、失追、剧情接管均阻止新动作请求。未提交物件释放保留量；已提交数量不得回退／重加。短失追保持机构状态但暂停手势推进；追踪恢复后重新抓取，不利用跳变补完。切枪保存该武器实例已提交数量及未闭合机构；再取出先恢复机械状态。读档／重生按新实例重新读库存，废弃旧事务与回执。原退出函数若会补弹，必须先接好本实例的防重复记账，不能直接调用“Abort”假定安全。

**射击门控。** 装填件不在位、机构未闭合、逐枪射后循环未结束时不能发射，即使原计数仍有弹。射速、伤害／既定单双手策略、散布、弹体及效果沿用原链。已满或零备用时允许物理检查／取回原件和闭合，但不创建补弹或奖励事务。不得为了视觉检查强迫原 ForceReload 接受本来无效的补弹。

<a id="w01"></a>

## W01 — Lancer 电锯步枪

2026-11-05 当前Lancer覆盖：Grab区域只用于新抓取竞争；已持有同一部件时保留实际起始Grip／食指扳机所有权、部件专用手腕附着与手指姿态，不因普通横向／反向小调整退回张手。抓取／手部显示贴合／弹匣井渐进辅助／真实机构行程分开处理；不能用显示吸附或12厘米统一脱离阈值代替真实动作。E是前端小拉柄，沿原长槽移动；长槽提供运动轴和端点，不是另一整条可抓件。B不供玩家抓取，C平时折叠，真实抓住后展开、向枪口推启动、向枪托拉停止。弹匣／E／C／护木接受Grip或食指扳机，已有归属时不借同次输入触发其他动作。此覆盖替代下方历史Grip-only／机匣交互措辞；尚不代表实体手感验收。

原C0已满／无备用弹的检查契约继续有效：允许取出并放回同一原弹匣，保留实际弹量，不创建补弹或奖励事务。v63实际录到满匣60／备用360中断后重新抓取、放回再开火；无备用弹20／0仅编译约束通过，运行与实体手感待证。

2026-10-31 用户覆盖：旧匣／未插入的新匣松手后消失，不实现落地拾取。起始 L>0 的战术换弹在插匣／闭合后恢复射击；起始 L=0 的空仓换弹另需真实上膛动作及机构复位，该动作是完美计时的终点。上膛部件和姿态须基于实际原件接入，不能把按键、等待计时或电锯启动件当作拉栓通过。以下早期“不额外增加机构”条款以此覆盖。代码／原件接入／编译／录像／头显确认分别记录。

完美计时由 `tools/vr-launcher/6dof-reload.ini` 配置，便携候选交付到启动器旁的 `6dof-reload.ini`。`perfect_reload=vr/original/off`，按2026-10-31用户最新简化要求，VR 初值 `tactical_seconds=4.0`、`empty_seconds=4.0`，有效范围0.1～60秒。空仓仍需实际拉动及释放回位，统一4秒不取消这一步。下次启动生效，每轮接受开始时固定该轮期限；取消不奖励、同轮不重置、奖励只一次。原模式仍读取该实例原窗口；普通完成和弹药守恒不依赖奖励模式。更新候选时保留用户已修改的文件。

用户补充：枪械换弹 UI 的进度与完美区必须使用同一轮配置时间，不能保留旧2.5秒显示。当前手动换弹取消原版“错过时机→失败／卡壳／延长惩罚”路径，超过期限仍可普通完成。进度到头不能代替真实插入、空仓上膛和复位。UI／状态原接入未核对前不得标记已完成。

类 `WarfareGame.Weap_AssaultRifle`；静态参考 C=60、备用上限 600、原普通 ReloadDuration=2.5 s。容量和时间均不是硬编码运行参数。

1. 左手在弹匣／释放区重新按 Grip，沿取出方向明确拉出；抓取隐含解锁，锁扣／机匣随原联动开放。越过有效脱离阈值时请求原换弹并取得计时器／动画所有权；不先按 R3。旧匣拿在手里，剩弹保留。
2. 松手回收旧匣显示后，从左肩抓取本枪新匣；也可把旧匣插回取消。右手持续握枪，新匣对准固定装填口并完成插入行程；提交 q=min(C−L,S)。已插入后原枪上匣可见、手持匣不可见，不能同时显示两份。
3. 将卡扣／机匣的联动锁定合并为一次自然推入／合盖动作，不强迫分别抓两个同步运动件。若实际原结构要求独立闭合接触，再提示一次收尾。最后必要锁定是完美／普通结果的结束时间点，不额外虚构直线枪栓。
4. 普通射击不要求每发手动拉机柄。电锯独立：左手拉原创启动件，满足已有启动动作后通过原电锯行为接触目标；处决交原版接管，结束后恢复握持。

模型任务 M01：原 `MagazineMesh`、`b_AR_Magazine`、`b_AR_Latch`、`b_AR_Receiver`；Receiver_Spin 作为联动件；电锯区域保留原蒙皮。启动件 M15 为独立可后接项目。

代码：复用 `weapon_reload_model.h`、`weapon_lancer_feed_anchor.h`、`weapon_lancer_insertion.h`、`weapon_lancer_magazine_pose.h`。原入口为 C1 公共换弹、`Weap_AssaultRifle.Reloading.BeginState`、`Notify_AmmoGrab/Release/Reload`。Playground 两个 Lancer `.inl` 仅提供历史适配依据，正式流程去除 8 秒自动补满兜底。

未来核对：历史卡扣约 114.5°、机匣约 28.5°只能证明运动存在；需同一原实例的联动姿态和固定入口标定。验收例：L=20、S=5，插匣后只到 25，闭合后可射；放置 8 秒仍不能自行变 60；切枪再取不能重复加 5。

<a id="w02"></a>

## W02 — Hammerburst 锤爆步枪

类 `WarfareGame.Weap_LocustAssaultRifle`；静态参考 C=78、备用上限 702、ReloadDuration=2.5 s。

1. 左手重新抓 `b_HB_MagLatch` 锁扣并沿原铰链明确打开，触发本轮换弹；原接受时计时并接管自动完成。尚未解锁不能直接拔／插匣。
2. 旧匣进入移除表现，保留剩弹。左肩取本枪独立弹匣，沿自身入口轴插入，确认 q 后把视觉交回枪上 `b_HB_Mag`。
3. 左手关锁扣到位后完成换弹。2026-11-12 用户明确覆盖：兽族步枪暂按没有可拉枪栓处理，空仓、半仓均不追加拉栓，上膛计数不增加膛内 +1。
4. 射击保持原 `FireAmmunition` 节奏，不因握持层每帧请求而重复发射。

模型任务 M02：锁扣 2232 三角、匣 272 三角均可刚性归属；使用原独立匣，锁扣直接驱动原骨。`HB_Reload` 的匣相对父骨为常量，不等于枪体空间不移动。接入前要复合父骨变换来定入口，不能照抄 Lancer 抓点。

代码：`weapon_hammerburst_reload.h`；原 C1、`Notify_AmmoGrab/Release/Reload/Throw` 统一纳入同一事务，`Throw` 不能再次销毁已转交的手持匣。插入未关扣时禁止射击；松开左 Grip 不自动关扣。

验收例：先关扣再靠近弹匣不计装填；L=70、S=3 只到 73；通知重复触发不丢掉新匣。待定量项为锁扣真实铰链、开合端点、手持匣姿态。

<a id="w03"></a>

## W03 — Gnasher 霰弹枪

类 `WarfareGame.Weap_Shotgun`；静态参考 C=8、备用上限 31、ReloadDuration=3 s。

1. 缺弹时首次左肩取弹开始本轮，目标 q=min(3,C−L,S)；已满或 q=0 不创建取弹步骤。左手一次取一发封口实弹，插入实际装填入口，每次记账 1，再取下一发，最多本轮 q。
2. 左手抓握并固定护木，右手握把大幅向下压，再抬回完成杠杆上弹。2026-11-12 视频覆盖：不实现末尾装饰性转枪复位。可在装 1／2 发后提前闭锁，只保留实际新增数、普通完成；达到计划 q 后才可按期限授完美奖励。不是沿前护木直线来回泵动。
3. 每次原射击确认后进入待循环；同样完成杠杆动作才允许下一发。不能因原 `FinishedCockingShotgun` 计时器先到而跳过手势。
4. 换弹开始前已有待循环状态时，将最终杠杆回位统一为一次闭锁确认，不能要求连续做两次等价动作。中断保留已插入发数；重新开始时重新计算缺额。

模型任务 M03+M13：原 `b_Shotgun_Lever` 1260 三角，直接原骨控制；[v2 完整弹药](../../assets/weapon-6dof/offline-v2/gnasher-round.glb)已制作，960／320 三角双 LOD，米制、+Z 指向封口。其制作尺寸来自空壳包络参考，不能直接定案为游戏中最终尺寸。

代码：`weapon_gnasher_mechanics.h`、C1；原射击保留 `CustomFire/FireAllPellets`，射后适配 `ShotgunCocking.BeginState`、状态内 `FinishedCockingShotgun`、`PlayShotgunCocking`。`BeginState` 原本设置约 0.75×发射间隔的完成计时器，必须接管当前实例这个计时器；只挡扳机不等于机构状态完成。

验收例：L=2、S=10 一轮只能到 5；第二发已提交后失追不会回到 2，也不会自动到 8。历史约 32.43° 杠杆幅度用于姿态标定，不作为手柄阈值。待核射后状态退出回执与最终闭锁去重。

<a id="w04"></a>

## W04 — Longshot 狙击枪

类 `WarfareGame.Weap_SniperRifle`；静态参考 C=1、备用上限 23、ReloadDuration=3 s。正常规则没有独立“半仓”分支；已装 1 发时不为动画额外取弹。

1. 空仓时左手抓原枪栓并明确开膛，触发本轮换弹。退壳是开膛到位的自动视觉步骤，不再要求手捏空壳。
2. 左肩取一发完整实弹，沿膛口轴插入；类型、方向、行程通过后 q=1 记账。
3. 左手闭合并锁定枪栓后可射。插入后未闭锁不能射；射后不另做一次独立装填前循环，下一次开膛属于本轮换弹。
4. 镜内视图作为原枪瞄准配套项，射线沿枪轴；不以整屏 FOV 放大替代真实镜内画面。

模型任务 M04+M14：`b_LS_Bolt` 894 三角保留原骨；`LensCap/Scope` 骨在本 LOD0 没有自有完整三角面，不能宣称已有可独立镜盖。[完整 Longshot 实弹](../../assets/weapon-6dof/offline-v2/longshot-round.glb)已制作，1248／416 三角双 LOD；已核对原粒子用 `bullet_spent_single_clean_03b_SMesh`，该空壳只作参考。

代码：`weapon_longshot_reload.h`；原 C1 加 `PlayWeaponReloading`、`Reloading.EndState/ForceReload/TargetingModeChanged`、`ShouldAutoReload`。需要保持瞄准／退镜清理，不能只调用公共基类退出。步骤映射 `open_breech → eject_case → take_round → insert_round → close_breech`，eject_case 由原动作回执驱动。

验收例：S=1 时插入只消耗 1；开膛途中换枪后恢复开放状态；原自动换弹不能在未装弹时补 1。待标定枪栓完整复合运动，不能先假定纯直线。

<a id="w05"></a>

## W05 — Snub 手枪

类 `WarfareGame.Weap_COGPistol`；静态参考 C=12、备用上限 60、ReloadDuration=2 s。

1. 左手抓匣并明确拉出，抓取隐含释放；可选按钮释放进入同一事务。松手回收旧匣显示后，从左肩取本枪弹匣并插入，保留旧剩弹，提交 q；旧匣原样插回则取消、不补弹。
2. **本项目固定的分支：** 开始时 L=0 为“空仓流程”，插匣后左手拉滑套到后端再释放回位，才完成；开始时 L>0 为“半仓流程”，插匣到位即完成。分支在事务开始时锁定，不因插匣后 L 变大而跳过。
3. 不增加独立膛内弹计数。滑套与侧翼按原机械联动表现，不要求逐个抓两片侧翼。射击时保留原滑套动作，不能被上次换弹手势覆盖。

模型任务 M05：原 `Magazine` 314 三角已从单骨组拆出并保留 UV、法线与材质索引，私有 OBJ 仅为制作输入，仍需创建运行组件；`Slide` 445 三角、`Wing_L/R` 各 60，直接原骨控制。无需新建整枪或假造外形不同的弹匣。

代码：`weapon_snub_reload.h` 的空仓／战术计划，原 C1、`Weap_COGPistol.PlayWeaponReloading/StartFire`。`EjectClip/InsertClip/CockOpen/CockClose/CockCloseFinish` 保存函数只播放声音，分别在对应有效动作完成时触发一次；它们不是弹药提交或运动控制 API。

验收例：L=0、S=4 插匣后已装 4 但滑套未回位不得射；L=4、S=2 插匣后到 6 且不追加拉套。半仓免拉套是 VR 设计，不是已认证的原版膛内机制。待标定滑套轴及空仓视觉姿态。

<a id="w06"></a>

## W06 — Boltok 左轮手枪

类 `WarfareGame.Weap_LocustPistol`；静态参考 C=6、备用上限 24、ReloadDuration=2 s。

2026-11-13 最新用户覆盖：左轮装填操作使用整只弹巢作为手持件，不使用单颗子弹；工作台装填物入口改为“装填弹巢”，枪上开合机构仍是“弹巢”。装填物可在独立编辑页拿起、移动、旋转后采集。记账仍按原容量缺额与备用计算，不能按整巢外形无条件补满；下方旧散装/快速装填表述以此覆盖。

2026-11-17 最新口述覆盖此前同日规则：左手像开门一样转开机构，到实际开启端点后旧弹巢消失；从左肩拿新整只弹巢，插入到位后自动合膛，不再要求第二次 Grip 或手推。完美奖励从新弹巢首次对准装填口开始计时，600ms 内连续插入到位可获奖励；实际插入、实际合膛与原奖励回执仍必须确认。松手或失去本次对准连续性取消资格，不通过重抓/重新对准重置窗口。此前“首次关膛抓握600ms”已被此规则替代。

2026-11-12 用户追加：左轮双手持枪时两手均握握把附近；随后用户澄清保留独立“副手握持（Second Hand）”抓点，护木支撑是第二只手的统称，并非限定护木位置；除弹巢外不开放其他枪体活动件。子弹作为单独的装填物可采集姿势，不增加实体快速装弹器。

1. 左手抓握转轮区域向外拉出，开始本轮；Object03 支架带动转轮，支架带子件 Object02 弹巢、Object08 摇出，Object01 固定后盘留在枪体。退空壳作为开仓后的视觉／部件联动，不再加证据不足的独立退壳杆步骤。摇出是本项目依据装配确定的设计，原动画角度未认证。
2. 2026-11-12 用户视频与明确覆盖：左肩一次取本轮需要的整组实弹（2026-11-12 后续覆盖：不用实体快速装弹器），送入打开的转轮后一次提交 q=min(C−L,S)。保留原有实弹，不丢弃半巢、不增加容量。此规则取代此前逐发装填方案。
3. 整组插入只提交一次，把 q 发分配到空槽，重复接触不得二次入账。成功插入后沿本次真实事务自动合膛，不要求另一次 Grip。外形上旧弹巢在开启后消失，新弹巢从左肩取得并插入；原有实弹仍保留记账，空仓最多补 6，半仓只补缺额，没有备用时仍可关闭已开放机构。游戏侧的原库存回执和奖励仍由原事务处理，工作台模拟不能代替这些回执。
4. 每发保持原射击方式，不要求额外捏击锤。弹巢索引和已装实弹分布由本地机械显示记录映射到原总数，不能声称原库存本来提供六个物理弹室。

模型任务 M06+M14+M16：保留 `alow_poly` 主体和原活动支架。Object02 两端实际封闭，已制作[六孔弹巢替换件](../../assets/weapon-6dof/offline-v2/boltok-cylinder.glb)及[完整 Boltok 实弹](../../assets/weapon-6dof/offline-v2/boltok-round.glb)。Object09 位于枪管下方，不能当已证实的退壳杆；Object01 是 root 子件，不随支架摆出。原 12 组均可刚性分离，层级／参考坐标详见 v2 调查和私有装配图。

代码：`weapon_boltok_reload.h` 的 `move_original_parts/insert_ammunition` 计划、C1。装填阶段映射多个原部件集合，不能为适配器便利硬填单个 Object 骨。`Cock*/EjectClip/InsertClip` 也仅为音效；默认引用 Snub 动画不能证明 Boltok 已有相同的运动轨道。

验收例：L=2、S=3 最多新增 3，全部补入关上为 5；备用仅 1 时只补 1 发就关闭为 3、普通完成；六槽不能多出第 7 发。支架／弹巢层级与孔位模型已交付，仍保留 `current_instance_verified=false`。已查 21 个 AnimSet 无匹配原轨道，离线 80° 展开仅为设计初值；实际实例、比例和行程留待接线认证。

<a id="w07"></a>

## W07 — Boomshot 榴弹发射器

具体类 `warfaregamecontentweapons.Weap_Boomer`，父类 `WarfareGame.Weap_Boomer_base`；静态参考 C=1、备用上限 11、ReloadDuration=2.5 s。

1. 空仓时左手抓原 `Boomshot_Barrel` 对应装填机构并明确开放，开始本轮，到达开放端点后才能插入。
2. 左肩取得原 `Locust_Boomshot.Mesh.Locust_Boomshot_Magazine` 装填组件，沿实际装填口插入；一个组件只转移一发。当前设计采用已证实的原载具件，不另让玩家抓飞行中的爆炸弹体。
3. 左手关闭枪管／装填机构至锁定，结束本轮。正常容量 1 不存在半仓换弹；射后开放动作归下一轮换弹，不另要求空射后循环。

模型任务 M07：枪管 1802 三角、枪上 Magazine 1236 三角都可原骨控制；地图类的 `MagazineMesh0` 明确绑定上述独立静态网格。`ammo2` 不作为默认装填资源。枪管静态动画有平移，不凭名称编造铰链翻开角。

代码：`weapon_boomshot_interaction.h` 选择 verified carrier 类型，原 C1、`Notify_AmmoGrab/Release/Reload`。保留 `GetProjectileClass/GetProjectileFirePosition/ProjectileFire` 的原弹体及奖励属性；本地玩家沿用对应玩家弹体，不误用 AI 类。

验收例：S=1 时只补一发，重复 Notify 不产生第二个件；枪口贴墙不能从墙外生成弹体。待核枪管开合曲线、实际插入轴、原生成入口参数。补模结论由“未知弹体”收紧为“复用原装填组件”。

<a id="w08"></a>

## W08 — Torque Bow 扭力弓

具体类 `warfaregamecontentweapons.Weap_Bow`，父类 `WarfareGame.Weap_Bow_base`；静态参考 C=1、备用上限 11、ReloadDuration=3 s。

1. 缺箭时首次左肩取原 `Locust_Torquebow.Locust_Torquebow_Rocket` 开始本轮。沿弓体固定槽位完整送入并到位锁定，提交 1，结束装填；不增加没有证据的手拉弦步骤。
2. 右扳机按下启动原蓄力，松开进入 CheckFireShot。CanReleaseFire 判断 Pawn 延迟开火条件，并不是最低蓄力门槛；松键可能立即 Fire，或置 bPendingEndFire 等待。弓臂保留原蓄力联动。
3. 正常握持保留原疲劳行为。ChargeTimeCount 从 MinChargeTime 开始，默认上限是 MaxChargeTime+HoldTimeUntilTired=3+3.5；完美换弹奖励可让计数以 2 倍推进。不是固定现实时间 6.5 s，也不能把 3.5 s 当全部持有上限。
4. 蓄力中 fresh R3 短按主动取消；同一按下不再分派换弹。新增非故障语义的公开 `cancel_charge(stamp, edge)`，复用私有 request_abort。失追、切枪、死亡、剧情接管也采用非发射取消：停止注入 → 清 ClearPendingFire 与 bPendingEndFire → ChargeEnded → 正常离开 Charge，自动执行 Charge.EndState；切枪走正常 PutDownWeapon。读回无待发射且未消耗弹药，等实际松扳机后才能再按。禁止用 EndFire／StopFire 取消，禁止手调 EndState 再重复转状态。已进 Fire 无法回滚为未发射。输入互斥见[共同规则](../offline-tech-prep-20261028/shared-contracts.md)。

模型任务 M08：原箭已被 `MagazineMesh0` 明确用于手持装填，可以复用；弓臂保留原骨骼和蒙皮，Bow05–08 涉及跨骨三角面，不能整体拆刚体。无需新增传统弦或另一支箭。

代码：`weapon_torque_bow_interaction.h`、C1，`Notify_AmmoGrab/Reload`、`PlayWeaponReloading`；发射接 `Charge.BeginState/EndFire/Tick/EndState → CheckFireShot → Fire`。取消组合已由保存脚本定位，详见[复核中的完整顺序](interaction-review-v2.md)；ChargeEnded 单独调用清理不全，Active.BeginState 还可能重启 PendingFire。native ABI 和游戏线程状态切换尚未认证，不等于已可直接调用。

验收例：只装一箭；头转但手不转，发射线不变；蓄力失追取消后不发箭、不重启、不扣弹；正常超时仍是原规则。剩余是原箭姿态、槽位标定及已设计取消组合的实际绑定验证。

<a id="w09"></a>

## W09 — Hammer of Dawn 黎明之锤

具体类 `warfaregamecontentweapons.WEAP_HammerOfDawn`，继承 WarWeapon。原 `HasAmmo/HasInfiniteSpareAmmo` 返回 true；常规换弹不适用，R3 不取弹、不触发普通换弹奖励。

1. 使用武器姿态提供目标射线，按右扳机进入原 BeginFire。先检查原 BeamManager 启用及目标条件，不能强制在室内／禁止区域呼叫光束。
2. 保持枪对准合法目标，沿用原目标保持、容错、预热／冷却、照射过程与反馈；移开目标走原 InvalidTarget，不能只把激光视觉移走。
3. 松键调用原 EndFire，完整执行 NotifyInvalidTarget、NotifyStopFire 和 TurnOffTargetingLaser。失追或离手采用同一已核对的停止语义，停止后再交还原动作。

模型任务 M09：原 `cog_hod_mesh` 主体和 Trigger（156 三角）；目标指示和光束引用原 ParticleSystem。不新增弹匣、充电电池或能量补充动作。

代码：`weapon_hammer_of_dawn_interaction.h`；本次已补到具体类 `BeginFire/EndFire/GetTraceImpact/Tick/TurnOnTargetingLaser/TurnOffTargetingLaser/UpdateFX`；BeamManager 的 `NotifyStopFire` 调到当前 Beam.AbortFire。`GetTraceImpact` 消费武器起点及 GetAdjustedAim，须改实际目标查询而非仅画枪口激光。默认 paint time=1.2 s、grace=.25 s 是静态参考。

验收例：合法目标才能照射；松键／失追能终止光束及循环音；纯转头不换目标；原禁止状态保持禁止。目标条件与停止语义已定位，具体调用参数／所有权仍需绑定认证。

<a id="w10"></a>

## W10 — Troika 固定机枪

武器类 `WarfareGameContent.Weap_Troika`，父类 `WarfareGame.Weap_Troika_base`；炮位 `WarfareGameContent.Turret_TroikaCabal`。只走炮位领取，不进入肩槽。

1. 沿用主线交互键请求原 ClaimTurret/DriverEnter。占用权确认后，左右手 Grip 抓两个握把，围绕固定炮塔枢轴求俯仰和水平转角，遵守原转动限制。
2. 右扳机开火，维持两手握持才持续发送本项目开火请求；松扳机或任一握把失追／释放，停止原开火并解除手部驱动。重新抓住不会自动重新扣扳机。
3. R3 无手动弹匣流程。原 `CanReload` 返回 false，但原自动 Reloading 状态存在，须保留进入、计时和粒子反馈。`GetReloadDuration` 按 Loader 是否存在选择时长；不能统一改成自定义“过热条”。
4. 主线退出动作走 DriverLeave/UnclaimTurret；完成后恢复自由手和角色观察基准。不要把炮位相机旋转直接写进自由武器方向。

模型任务 M10：原 latitude、longitude、spinner_L/R、muzzle_L/R；握把与 playerref 骨在本 LOD0 无自有面，但可以作为参考点候选，并不意味着没有握把外观。转向驱动原父子层级，不重建双枪管。双枪口轮流还是同时发射由原 InstantFire/WeaponFired 决定。

代码：`weapon_troika_interaction.h`；原炮位 ClaimTurret/UnclaimTurret/DriverEnter/DriverLeave/UpdateTurret/GetPhysicalFireStartLoc；武器 Active.BeginFire、WarmingUp.EndFire、InstantFire、Reloading.BeginState/EndState、GetReloadDuration。默认自动时长 2.5 s、有 Loader 0.1 s 是包内参考。

验收例：占用失败不绑手；退出不留下开火或强制相机；自动恢复仍考虑 Loader；左右枪口保持原行为。待核炮位边界和真实握把轴，资源／热量语义不由 Reloading 名字推定。

<a id="w11"></a>

## W11 — Bolo / Frag 破片手雷

**2026-11-08 用户覆盖：** 持续右 Grip 取用/持有，松 Grip 投出，不再扳机准备；Frag 2 kg，保留去掉原版轨迹预览要求。以下交互条目按[最新规则](../frag-grip-release-20261108.md)表述；原版生成/碰撞/库存/引信与模型约束继续有效。

**2026-11-09 手感确认：** 用户选择保持直接跟手和原链条运动，不增加惯性拖拽。2 kg 继续作为 Frag 的虚拟质量／冲量配置，不缩放真实出手速度；实体头显手感验收另记。

类 `WarfareGame.Weap_FragGrenade`，父类 Weap_Grenade，弹体 Proj_FragGrenade。原基类 MagSize=1、MaxSpareAmmo=3，手持与备用计数按实例读取；不直接假设 HUD 等于 4。

1. 在右裤兜持续按住右 Grip 取出并持有，不需要右扳机准备；真实松开右 Grip 形成一次主动释放。它与枪械按一下 Grip 保持握持的规则不同。追踪消失、暂停、读档或旧计数恢复不能冒充一次真实松 Grip，也不能自动重新握持或投掷。
2. 有效姿态下的轻抛／原地低速释放也应成立，不沿用原型 0.5 m/s 与 4 cm 作为最低玩法门槛。无效追踪则拒绝生成并给重抓提示；低速不等于无效。
3. 出手位置来自手中原弹体参考点，速度来自释放前手部轨迹。由同一冻结身体基准转换到游戏世界，角色／载具基底速度只加一次。普通转头不参与速度。
4. 释放通过原生成链成功获得原弹体后只扣一枚，原反弹、爆炸与引信继续工作。碰墙不能把出生点推到墙后。原投掷动画通知不得在手势释放后再生成第二枚。
5. 不手动换弹；投后原自动补到可取用状态保留，但补雷不等于再次握持，下一枚仍须新的右裤兜 Grip 取用。原近战贴附若保留，必须独立选择模式并沿用原贴附链，不能把持续 Grip 持有或松 Grip 投掷解释成贴附。

模型任务 M11：原 root、Counter、9 节链、end 保持蒙皮／原动力学。第一阶段取一个明确的弹体参考点估速；刚性握点偏移不等于真实链端运动；不拆链条后静态拼回。

代码：`weapon_bolo_grenade_interaction.h`、`grenade_throw.h`。原 `AnimNotifyThrow → ThrowGrenade → GrenadeThrownNotification`；在本次 ThrowGrenade 内接 `GetPhysicalFireStartLoc/GetGrenadeTossVelocity` 的返回值及 Init 方向，具体契约见 T1。引信相关 BeginExplosionSequence/DoExplosion 保留原逻辑；LifeSpan=10 不是 10 秒引信。

原引信静态结论：准备／Spawn/Init 的已查函数没有启动爆炸 timer，不新增在手 cooking。普通 Bounce 停止后继承 StopBounce，默认 .1 s 提示延迟+1 s 后爆炸；Base 为 Pawn 时变 1.5+1 s，贴附 AttachInit 为 2.5 s。特殊 HitWall 另有 BeginExplosionSequence(0,TimeAfterBeep)，须保留。停止不等于第一次触地；低速或反弹计数也可触发。普通停止已有67780计时／伤害与28120原StopBounce调用返回实录，贴附54784记录了原附着／剩余引信／伤害和正常退出；整段2.5 s不冒称完整实测。特殊HitWall仍无实际执行记录。详见 v2 逐路径表及[最新证据](../frag-grip-release-20261108.md)。

验收例：上抛、平抛、下手投和轻放具有不同初速；重复松键只扣一枚；头转不改轨迹；暂停、回中和 Snap 转向不制造大速度；已启动引信不能靠失追刷新。分别检查普通停弹、特殊碰墙和贴附，不由其中一项代替全部。

<a id="w12"></a>

## W12 — Smoke 烟雾手雷

2026-11-09 用户最新指令：Smoke 也由当前破片手雷会话负责，先完成 Frag，再开始 Smoke。交互沿用 W11 的持续按住右 Grip 持有／准备、真实松手投出、直接跟手；保留 Smoke 自己的原主体和链条。此指令是后续开发授权，不代表 Smoke 已实现或已验收；开始前不扩展当前 Frag 的运行范围，不将 Frag 实录当作 Smoke 证据。

类 `WarfareGame.Weap_SmokeGrenade`，弹体 Proj_SmokeGrenade；独立弹药类和效果。取用、准备、释放、低速处理、墙边检查与 W11 相同，共用 T1 运动算法。

1. 每次取出必须绑定烟雾实例和它自己的可用数量。接收手势投掷请求时确认类型仍为 Smoke，不能复用上一个 Frag 的弹体类缓存。
2. 成功生成原 Smoke 弹体才提交一枚；移交原碰撞／引信／GrenadeExplosion，创建原 `Emit_SmokeGrenade` 并沿用其清理。不给破片爆炸伤害作为通用 fallback。
3. R3 无常规换弹。切换 Frag／Smoke 时废弃旧运动窗口与待投 token，重新准备；未生成的保留量返回各自库存。

模型任务 M12：原 Smoke 主体、Counter、9 节链和 end；与 Frag 的三角数、外形不同，不能只替换材质。保留烟雾 emitter，不新增“通用手雷”网格。

代码：`weapon_smoke_grenade_interaction.h`、`grenade_throw.h`，共用 Weap_Grenade 发射链；Smoke 专用 StopBounce 直接 SetPhysics(0) 并 DoExplosion，GrenadeExplosion 创建 Emit_SmokeGrenade；专用 AttachInit 直接生成附着烟雾，不套 Frag 的 2.5 s。继承的特殊 HitWall 路径仍保留。原 `PlayWeaponReloading` 的 .001 s 自动更新不作为用户换弹。

验收例：烟雾库存只减一次，破片库存不变；生成 Smoke 类并持续原效果；普通停止立即进入原烟雾流程，不套 Frag 的 .1+1 s；交换种类后旧请求不得生成 Frag。静态触发差异已查清，实际碰撞／效果清理仍需独立验证，不能以 Frag 的通过代替。
