# Gears VR 0.1.0 开发记录

## 武器 6DoF 独立分支（2026-10-22）

2026-10-23：PE入口到现transport的只读测量已接线：最外层TLS scope先核GT，只保存const快照／时钟，本地指针仅沿当前调用配对；独立1024行缓冲，same_state比较原始stamp，零覆盖或clock错误不影响原权限与恢复。Astra静态复核无阻断，相关五种构建和65项独立entry审计自检通过；新候选尚未运行。late created_ms≤entry_ms仅作年龄条件，未在入口读取bridge，不能证明那张票据当时已发布／可读。另明确手柄→手骨刚体校准本身无100ms数学期限，之后可单独评估age-only suspend，但当前latest-only会遗漏中间loss且GVE6未包含Native reference_generation，不因此直接放宽现有候选。详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：bf95959／d456 连续性候选 f 购买版 PID56488 完整 25写／25恢复、11→10→11→10、Shutdown0 和原8控件／44附件／两Hook恢复；FPP累计86，静音9会话无错误／待恢复，精确游戏退出后Guard结束，自有进程全部退出。三份独立 timing／shadow／transport 审计有效，65条 transport 中24次调用、21次双手续龄；普通持枪／取回 phase1 的14次均续龄且零超龄，收枪 phase2 的10次为7续龄＋3早期过期（旧109／110／109ms，新15／16／16ms）。**组件仍仅持续运动门失败**，部分重绑发生在新姿态到达时，不能据此称跟手通过。原failed receipt `560457d5…330dfc2` 和原审计保留；Native entry verifier false／XR1／整局false不改。下一步仅测PE入口到现transport点的前缀时间、同Tick状态与早期票据可用性；Astra指出现有计时在transport之后才开始，不能直接证明前移能追回余量。100ms及三道装备／最终写入门均保留，详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：按 e 的早期新帧证据实现独立 transport continuity 候选，开关默认关闭且必须显式联合试验。最早入口以第三个独立 cursor 读取，只给尚未超龄且仍 tracked 的既有手绑定保存真实 created_ms；旧 101ms／future 先撤销，不能由新帧复活。调用必撤旧 Plan，不生成 Plan、不创建绑定或消费 baseline、不改历史 Context／ready 或 writer 游标；后置三次装备认证及最终 authorize 均保留。8 个固定 stream fence 阻止跨路径重放，合法完整同票据只允许正常 writer 重验一次，身份／action／ready 变化清例外。25 组 Policy 边界、九项组件检查／四拒配、连续性开启／关闭两种联合及默认／独立 stow 构建通过。Astra 发现的 bridge 消费后拒绝而未返回 Ticket 的跨 reader 缺口已补：首次建基以外的拒读先封写、按原路径恢复、再清计划／绑定并失败结束，不改 ABI；5 个编译期分类断言通过，最终只读复核无剩余确定阻断。独立 transport 审计 75 自检通过。尚未跑新候选，不能据此宣布持续跟手修复或完整 6DoF 完成；后台联调继续 Mute1／Behind1／ObserveOnly1 与精确退出后音频恢复。见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：只读shadow63317ca／ad22候选的e实际PID35740取得74条完整配对、0异常；phase2的9次双手age撤销在最早入口旧78–94ms／新0–16ms，晚期旧125–141ms但早期新仍46–63ms，全部same_state且无其他撤权；phase1另7次也全部early可用。54项shadow审计和实际结构检查通过，证据支持独立transport-liveness设计，尚未实现续龄或修复motion。23写23恢复／完整交接／Shutdown0及原字段恢复通过，component仍仅motion失败；FPP99、Native entry verifier false／XR1整局false保留，9会话静音正确收尾，所有自有进程退出。原failed receipt`0464570b…23be51`／shadow`ec76cc17…18af5db`保留，83项历史文件未改；详见 [早期连续性证据](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：restore前的独立cursor只读shadow已接线，最多1024条内存记录、结束后一次输出；常规observe后配对真实binding_age，并检查动作代次／phase／历史状态未变及早期新票据到晚期仍≤100ms。Policy新增const按值continuity_snapshot只查询，不续龄／消费游标或改计划；15纯场景、五种相关构建、九检查／三拒配通过。尚未跑新候选，也未实现liveness接口，不把诊断候选当作跟手修复。详见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：诊断提交6467b1d／ccda候选的实际d局 PID31544 完整23写23恢复／11→10→11→10／Shutdown0，独立QPC块通过（23开始22完成1尾部pending）。phase2旧binding票据在observe平均126.667ms／最大140ms，9次明确age撤销；新票据平均48.556ms。pre_restore→observe平均35.275ms，restore9.806、单次三处装备认证约7.1、dirty stow2.149、原Tick8.241ms；这些inclusive分项样本不同、不可相加推FPS。写前证据约0.085ms，主成本不是该flush。组件唯一motion门仍false；8控件／44附件／两Hook、game_absent、9会话静音恢复均通过，Native entry verifier／XR1整局失败保留。原receipt`c867d64b…b1034c`、独立计时`7772c813…3e6306`不改写；尚未声称优化或完整6DoF完成。见 [测量详情](../../hand-ik-xr-stream-trial-20261023.md)。

Astra Max指出认证内部去重不足以独自补上26.7／40ms余量；下一步先在restore前用独立shadow cursor只读测传输连续性并配对稍后撤权，不改Policy／writer／Context。仅证实最早时刻仍≤100ms后才评估单独liveness接口，始终保留真实created_ms与后置完整写入授权；该shadow与接口当前均未实现。

2026-10-23：联合候选新增仅观测的15项内存QPC聚合和Policy逐原因／手别年龄诊断，终端一次输出，覆盖写前／写后证据flush与实际lease acquire；不改100ms created_ms续龄、15秒／256预算、Plan／authorize或任何装备门。12组Policy边界扩展断言、五种相关构建、九检查／三拒配通过，尚无新成本测量或跟手修复结论。后台测试继续显式Mute1＋置后／窗口只读，Guard保留至独立确认精确游戏退出；普通启动音频不变。见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：新候选64b的联合c实际 PID38884 完整11→10→11→10，23写／23恢复，取回新绑定、最终stowDirty0x10005直接Shutdown0及8控件／44附件／Call32＋Event64恢复均通过；FPP累计88，9音频会话无写入／错误／待恢复且Guard在game_absent后结束。**组件仍false：唯一失败是收枪phase2移动响应**，lease6–14全bind3，尚未保持同一绑定连续跟手；不得把完整交接流程称完整6DoF通过。receipt`a4336048…75ea83`／audit`eec68346…96c63c`保留。strict Native年龄等待实际两次约11.64／23.15ms取得≤1秒新状态，预算不延；23 runner／38 audit／10 pure检查与Popen0准备通过。Native clean但独立entry verifier false、GPU0／XR exit1和整局failed继续保留。下一步测量剩余认证／stow tick成本，100ms绑定语义不改；见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：联合b购买版 PID47248 Start0、槽11→10，原手4＋owned-detached4共8次双手写入／跨Tick采样并全部恢复，FPP累计63；但每次bind_mask3，移动被重绑吸收，未达持续运动／取回／dirty联合Shutdown验收。host Native复合新鲜度检查停止，失败四数缺失，不能唯一归因；原failed receipt `95a42eb4…3e18fa`保留。Shutdown0、8控件／44附件／Call32＋Event64恢复、自有game_absent和9会话静音收尾通过；Native clean但entry verifier false、图像8×8／GPU0／XR exit1均保留。Astra Max确认旧票据125／125／109ms触发按设计撤权；`f8ec02f`仅将Context变为纯构造，将六次装备认证去重为三个显式门。新候选`64b965a9…ca6798`（160源码、manifest`5e15c276…b2f40e`）strict构建／九检查／三拒配通过，100ms／15s／256不改，尚未跑新局。

2026-10-23：其他实例自然退出后，联合a实际购买版 PID46048 在双手Start前因运行器误要求全部8 strength为+0停止；冻结C++要求0／1／4／5为(+1,+1,+0)、尾2／3／6／7为(+0,+0,+0)，真实读数完全满足原生门。未Start、未槽请求、未Native／XR session，原失败receipt `d845dbf9…399fad`和audit保留。Shutdown0，8控件全字段／44附件精确恢复，Call32／Event64独立匹配、game_absent；9音频会话无mute／restore写入或错误，Guard在游戏退出后结束。正在新b目录仅修运行器／审计位模式，候选159947不变，尚无联合交互结论。

2026-10-23：联合源码已提交 `e8b3cf5`，候选160项 live／保存源码再次匹配。私有 runner 17检查、独立审计30自检、禁止Popen的默认prepare均通过，固定182依赖／23实际模块；Astra Max窄复核的FPP硬门、Stop异常仍Shutdown及静音Guard异常保活问题已修。三项后台标志显式开启，只有精确game_absent后才恢复原静音状态。准备回执 `a34c4706…31d96`；首次run在注入／私有实例创建前被另会话活动游戏的idle门拒绝，**尚无联合游戏结果**，不干预其他实例。详情见 [联合记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：双手／暂收联合候选已接线并封存 `159947f1…97229d`（160源码），默认关闭，须显式 combined＋XR stream＋stow，其他 writer 仍禁配。稳定 owned-detached 可重新取得双手资格，pending／切换先清旧计划与绑定；助手前要求手journal clean，Shutdown 先封两方。foreign 撤权先于可重入 handback，已回原手路径新增只读 relinquish，旧收枪写权不随再detached复活；新显式acquire才可开新lease。strict联合／独立stow／默认构建、九检查、三拒配和导出隔离通过，Astra Max最终源码复核无剩余确定阻断。私有联合runner正在准备，**未跑联合游戏**，未变更冻结Native／XR、普通启动器或公开ownership；详见 [联合接线记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：同一 `121f116`／dc870候选的购买版 PID 18776 按钮联调 **passed=true**，36 正式读数＋6 pending 样本。实际 owned R2／L2 各自 Release 成功并变 up，随后原 BurstDelaying EndState→Active BeginState，自然完成 pending，再原助手暂收；4 槽确认 `11→10→11→10` 均有本请求唯一 ack/API。持续 both 的收枪／取回阶段各 0.6913／0.6744 秒，原键 up／AmmoUsedCount=6 不变；松开后新 both 才再次 pressed／耗弹。最终 dirty0x10005 Shutdown0，先恢复原44字节挂点再卸Hook，两入口独立还原、publisher/game/owner正常退出；九个原已静音会话无错，Guard 持续到 game_absent。修正的 private runner 28检查通过，fresh discovery首次0.2224秒成功，未实际覆盖churn重取。原 failed a/b 不改写；c receipt SHA `e9c1b0e6…07a1e4d`。候选仍默认关闭，开始双手驱动与稳定 owned-detached 的联合接线；Quest／最终画面与完整6DoF未验收。

2026-10-23：原开火结束等待与原子请求仲裁已提交 `121f116`，候选 `dc870cd1…1111f2` 封存 160 源码、七项纯检查及默认导出隔离通过。实际新局 PID 42312 在首个 Pawn.Controller 只读字段发现中因 Name registry changed 中止，Publisher／Enable／tap／helper／pending 均未开始；原收据 failed 保留，不能判定新功能结果或附件往返。Shutdown0、两入口原字节匹配、精确游戏退出、owner0和九个已静音会话无错清理通过；未抢占之前轮换的其他游戏实例。正在修正私有 runner 的启动期完整只读发现重取，不修改共享 Resolver 或重放在途请求，之后继续同一候选联调。

2026-10-23：真实原 R2／L2 联调 PID 57152 成功按下两键并消耗 Hammerburst 弹药，但开火中的身体槽请求被旧候选返回 12，整局收据保持失败；当时为 BurstDelaying，缺少 yield 分阶段日志，尚不能唯一归因。清理成功，静音保护持续至精确游戏退出。新修订增加逐手 Release 回执、最多 1 秒的原开火状态自然结束等待；等待先封新 press、没有附件写权，身份／挂点／原状态／按键认证失败即取消，Stop／Shutdown 排空后才卸钩。外部与 XR intent 统一原子 claim，等待保持 processing，终值只在读回与日志后公布。严格候选／默认关闭构建和新增 mailbox／pending 纯检查通过，尚待封存后实机联调；详见 [暂收试验](../../equipment-stow-trial-20261023.md)。

2026-10-23：请求确认竞态已由单请求 mailbox 修复，reserved／queued／processing 期间始终返回 0，原调用／读回／模型／完成日志结束才公开结果；真实跨线程暂停和 producer 竞争回归通过。新候选 SHA256 `81601d4b…80cdec` 的购买版 PID 36000 补测 **passed=true**：Hammerburst 槽 3 完整往返，外部 true 交还后 35 份样本持续 11.5607865 秒 detached／dirty=0、无旧 deadline 拔回；再取回／暂收后在 `0x10005` dirty 状态直接 Shutdown。日志先恢复原 44 字节挂点、clean／fault=0／revoked=0，再还原 Hook；Shutdown=0，两入口原字节独立匹配、精确游戏退出、owner0、九个已静音会话无错误／无待恢复。源封存 158 份，严格候选／默认隔离及六项纯检查通过；启动前私有 ini 原字节快照保存，未补 resize／前台恢复。回执在 `analysis/equipment-stow-runtime-1023b/`，前局失败不改写。两局均仅外部槽请求且 R2／L2 mode off；下一项为原按钮联调。XR intent 与外部请求还需原子仲裁，实际身体命中、肩部图像、与手骨 writer 共存及头显仍未验收，详见 [暂收试验](../../equipment-stow-trial-20261023.md)。

2026-10-23：暂收候选首局购买版 PID 47872 的真实 Hammerburst 槽日志 `11→10→11→10→11`，原手附件往返 44 字节一致；外部 true 进入前按内部 false／附件读回／clean／action 的顺序交还，随后 35 份样本持续 11.698669 秒保持 detached、dirty=0，跨过旧 10 秒截止且无内部 helper 拔回。49 份外部读数的库存／Active／八控件位模式保持。**原收据仍失败**：取走队列后、写回本次结果前的确认窗口暴露旧状态 10，runner 在本次真实 11 完成前提前退出；当时 journal 已 clean，不能当 dirty Shutdown 通过。Shutdown=0、原附件及两入口独立还原、精确进程退出与静音清理通过；失败收据／机械 audit 保持在 `analysis/equipment-stow-runtime-1023a/`。正在修复请求全处理期的 mailbox 回执，并补本请求新日志确认；不以同局组件证据改写整局状态。

2026-10-23：身体槽暂收新增默认关闭的有限候选，先在模型副本提议，再由原 `SetWeaponTemporarilyHolstered` 与当前附件读回决定是否提交。独立 journal 记录八对象完整身份、四槽和原手骨名／相对变换，原动作前在反射请求／ProcessEvent／CallFunction 三处同步恢复并结束本 lease；未知或 foreign 状态永久撤销写权限，之后再 detached 也不补 false。R2／L2 新增同步 yield，停用时不等新 XR 帧；取回后封存旧边沿，持续按压不会自动开火，明确松开的新 baseline 允许后续正常单次按压。Shutdown 等待游戏线程恢复和在途回调，锁顺序已由 Astra Max 复核修正。严格候选／默认关闭构建、导出隔离、缺 CallFunction 的配置拒绝及五项纯检查通过，候选 SHA256 `016b587c…a95ef7`；尚待单局零售版有限联调，未替换启动器，详见 [暂收与按键交还试验](../../equipment-stow-trial-20261023.md)。另初始 arena PID 28452 的三组读数及 20 帧观察均已持 Hammerburst，旧 1,003 帧也都非零 Weapon；没有取得真正 `Weapon=None` 基线，不因此开放零武器手骨写入。该观察局 Shutdown／两入口还原、精确 game_absent 与静音恢复通过。

2026-10-23：实际双手 XR 数据链新增两局购买版模拟器记录，Native 的 FREE_HAND／CROSSHAIR 开启、CALL_BOUNDARY 关闭可与 Playground 动画入口共存。PID 40872／53564 各完成 43 次 GVE6 新姿态驱动和 43 次精确恢复；前一局有一次 dirty 原近战入口立即交还，后一局请求在干净 Tick 边界进入，均观察到原近战／槽播放和退出后新票据驱动。前局未启用第一人称，临时偏移过大，一次左手位置残差约 9.76075 原始单位；其余 85／86 手样本小于 `6.87e-5`。后一局实际切到第一人称（Native applied=173），86 个跨网格 Tick 手样本位置残差最大 `6.57e-5`、完整旋转角差最大 `0.02348°`，另有一个 Stop 到达后的 planned 尝试未写入、不计 lease。两局 dirty／fault 为零，PlaygroundShutdown=0、两原入口独立匹配、九个已静音专属会话无修改／无待恢复且自有游戏退出。**两份整局 receipt 仍为失败**：1280×720 源与 8×8 backbuffer 不匹配，GPU 未捕获／XR 无图像并 exit1；保留 tiny-output 只读规则，没有自动 resize／前台补测。Native 干净关闭，五项 DXGI 磁盘不匹配的独立关闭后 hash 均等于同局 manifest 安装前 original、不同于 Native patched；原 verifier 基准接入缺口与原 `all_match=false` 保留。原始回执和组件／入口复算只在 `analysis/hand-xr-1023c/`、`1023d/`；这支持有限手骨数据链，尚非 Quest、掌心标定或图像／头显验收，见 [XR 双手记录](../../hand-ik-xr-stream-trial-20261023.md)。

2026-10-23：原版收枪助手的购买版只读观察取得当前实例证据。自有置后静音 PID 18068 调用人物 `SetWeaponTemporarilyHolstered(true)` 后，当前 Lancer 的同一 Weapon.Mesh 从人物 Mesh 脱离；finally 调用 false 后恢复到原右手 `b_MF_Weapon_R_End`。四槽、库存链、当前 Weapon／slot 2、Active 与 `bSwitchingWeapons=false` 保持，稳定基线后 419 帧的八个 cached IK 控件对象／强度位模式不变；观察器共 441 帧，无其覆盖的 watched 动作事件。原 `bTemporaryHolster` 始终 false，说明不能把人物助手与武器的暂收旗标混用。私有 runner 错误等待该旗标 true 而超时，原失败 receipt 保持未通过；独立机械 audit 只总结已保存的字段、附件及 slot 样本，不重写运行结论。前期三次准备／签名拒绝均在实际暂收调用前停止，未作通过证据。自有 PID 的实际退出、PlaygroundShutdown=0、两个原入口匹配及九个本来已静音会话的零修改／零待恢复均核实。回执／复算位于忽略的 `analysis/hand-stow-observe-1023d/`；尚未证明肩部代理渲染，也未接入身体槽与 R2 持枪许可。

2026-10-23：实际 GVE6 grip 到双手 IK 的默认关闭候选已接线并完成严格编译／离线检查。每手保存完整刚体局部绑定，独立核验新票据、身份／动作／XR 各代次和 100 ms 新鲜度，失去一只手不要求另一只手重绑；原动作入口同步恢复 journal，重新安静后等待新 baseline 再接管。Lease 新增选中通道接口，未选手不读写；日志刷新后重验新鲜度，拒绝第二次 Start 不覆盖排队请求，256 次写入预算不可被原动作重开。完整绑定、12 组票据边界、选择通道／失败恢复及 304 字节私有 Start 检查通过，XR／合成／默认禁用构建及导出隔离通过。只允许独立 status 冻结的新 Native 无 handoff 测试场，不能把 frame-link epoch 冒充 committed 同步 getter，也不能把票据自身代次当外部基准；后续购买版模拟器数据链与整局失败分开记录于上文。详见 [XR 双手有限试验候选](../../hand-ik-xr-stream-trial-20261023.md)。原公开手部 ownership／apply 与 GVE6 ABI 不变，普通启动器及冻结包未替换；本轮子任务不启用 Fast。

2026-10-23：自由双手新增默认关闭的四通道逐帧驱动，复用原左右手链尾的世界位置／整数旋转；每次先保存四组目标与强度，下一 PlayerTick 在原版及桥接请求前精确恢复，原动作入口可在同帧先恢复再交给原版。购买版 PID 15952／20624 两局各完成 32 次位置／yaw 驱动、96 份完整矩阵样本及 32 次恢复；PID 56972／37196／43348 三局再完成 X／Y／Z 与 pitch／yaw／roll 的三轴组合目标和反向目标。五局各有 256 次 Setter 和 256 次目标写入／还原，第 16 次写入后立即触发原版 Lancer 近战，真实动作／槽播放、同步交还及结束后重新合格接管均通过，dirty／fault 为零。三轴三局各 31 个跨 Tick 样本中两手位置目标残差均小于 `6.25e-5` 原始单位，三轴旋转段完整目标角差均小于 `0.022°`；同帧第 16 次不计效果样本。这是小幅合成目标的当前手骨窗口证据，实际 grip 标定、XR 持续驱动、其他固定动作和最终渲染／头显仍待完成，公开 `ownership_proved`／apply 保持零。

前两局 PID 29424／51004 各完成 16 次恢复和立即交接后干净超时。诊断确认 Lancer `AR_Melee_Saw_Idle` 在父槽退回 base、三种子权重全零后仍保留子 `bPlaying=1`；不再用该标志单独否决当前零姿态贡献，非零退混合、未知字段、动作和身份门仍拒绝，也未写原动画字段。权重和剩余混合按 binary32 位检查零，DAZ／FTZ 模式下的 volatile 次正规数拒绝测试通过。Astra Max 复核已保存脚本、原日志与恢复／调用顺序；本轮 subagent 均未启用 Fast。四通道部分写入、helper 失败后已写入、foreign 值不覆盖、精确恢复和零权重条件的 C++ 检查、旧受控 IK／CallFunction Hook 回归及 30 项 Python 运行器测试通过。所有七局的两入口活体还原、九个静音会话恢复及自有游戏退出通过；默认禁用构建和导出隔离通过，冻结 Native／XR、默认启动器及包未替换。回执 `analysis/hand-ik-stream-runtime-1023a.json` 至 `1023g.json`，当前 DLL SHA256 `87308482f6e60502361f81032e96b28f9b2d2971b02b9f2962d1046643090563`；详细协议、数值和限制见 [双手连续 IK 试验](../../hand-ik-stream-trial-20261023.md)。

自由手动画内部调用缺口已有购买版证据：默认关闭的 CallFunction 候选在自有置后静音 PID 59828 取得 44 对调用、11 次真实槽播放、274 个快照、零 caller frame 拒绝。拾取伴随槽 3／10／14 在配对调用 18／19／20 中，把同一子节点从 `AR_*Holster_Rt` 改为 `AR_*Equip_Lt`；此前未观察到的槽复用现有同步边界。主 Kickup 和普通近战 A／C 的槽播放也已观察。候选不消费 caller 参数、不改返回值，RVA `0x1d6f30` 入口已被其他所有者改写时拒绝安装；停机后独立读取 CallFunction32／ProcessEvent64 均与原始证书一致，游戏退出及 11 个静音会话恢复通过。私有／默认关闭构建、转发与还原离线测试、观察器／Hook 测试及 23 项 Python 检查通过。回执 `analysis/hand-animation-call-runtime-1023a.json`，详细字段和证据见 [动画观察记录](../../hand-animation-probe-20261022.md)。该只读观察局本身不写手骨，也未覆盖取消／处决或头显画面；后续有限驱动见上文，原所有权门不会由新日志自动解锁。

自由手新增独立纯逻辑 `hand_animation_ownership.h` 与测试：默认拒绝，仅对完整配对的拾取 53／普通近战 A、C 起止、稳定实例／控制链、槽实际／目标权重与剩余混合退出及后续新 Tick 输出 `normal_path_ready` 必要条件；`ownership_proved` 始终为 false。缺边界、取消／受伤、其他固定动作、身份变化、槽复用、丢样和超时锁闭，重置后空闲也不自动获权。认证的空数组元素可保留，未读成功的零值不能代替空洞证明；`PendingBlendOutTime` 保留时长及精确 -1 哨兵均不当作剩余混合。MSVC Release `/W4 /WX /EHsc` 的新门、既有观察／自由手回归及私有保存日志回放共 4/4 通过。PID 57480 的 19 个槽数组元素含两个空洞；伴随收枪槽 3／10／14 在 snapshot 26 无配对 Play／Stop 边界复用为 Equip，严格门因此拒绝，不能由主 Kickup 已安静宣称可交还。该前 26 快照回放人为补足了空洞读取成功、Tick 代理值和预绑定拾取目标等字段假设，**仅为首次拒绝位置的诊断性离线复核，不认证当前运行适配器**；派生源码和回执均留在忽略目录，见 `analysis/hand-animation-ownership-offline-20261022.json`。没有启动游戏、IDA、接入运行桥或改变默认构建；持续双手写入与头显验收仍未完成。

自由双手原 IK 受控复测：默认关闭的六窗试验在两局自有置后静音购买版进程 PID 53904／60116 各取得完整 `S,+X,−X,−X,+X,S`、307／317 条原始采样、12 次 Setter 和 12 次 owned 目标写入／还原；字段恢复、Hook 关闭、自有游戏退出及各 11 个音频会话恢复通过。离线脚本按原定前后不同网格 Tick 夹取及同跨度无写入对照重算，两局四个活动窗右手 X 残差约 `±0.096–0.106`／`±0.096–0.105` 原始单位，均超过未放宽的五倍噪声门槛 `0.035896`／`0.042369`；假操作上界 `0.000825`／`0.004228`。首个恢复新 Tick 并非全部在噪声内，第二个新 Tick（回滚后约 18–22 ms）全部在噪声内。初版仅用脉冲前两 Tick 外推的分析在第二局因较大 sham 偏差未通过，旧回执保留；原采集终行仍为 `effect_evidence=0,inconclusive=1`。这只支持**当前实例短时右手骨 X 响应候选**，尚非 XR 持续双手、动作接管、最终渲染或头显验收。复算工具、私有回执与方法见 [IK 对照记录](../../hand-ik-controlled-trial-20261022.md)。默认启动器及 `apply_mask=0` 不变。

启动前臂展校准现有一个源码工作树候选：新清单布尔门 `armspan_startup_calibration` 必须与手柄观察、owned startup menu 及既有 crosshair／UI／anchor 前置能力同时启用；冻结的普通清单、安装版启动器、Native／XR 构建及便携包白名单均未改变。缺失或无效记录时，在 Native 激活前由自有 XR 子进程发布同票据的双手姿态，只读消费者核对版本、进程身份、频率、跟踪与 100 毫秒新鲜度；双手张开并连续按住两个 Grip 至少两秒且取得 60 个不同样本才原子保存到本机配置。有效记录自动复用，启动器可选重测；测量或保存成功前失败保留旧记录。保存成功后若 XR 清理失败，则保留新记录但阻止 Native 激活，不能回滚覆盖并发更新。Python 新增 32 项、既有 launcher 78 项、原校准 2 项、C++ ABI 对照、C# 候选构建与隐藏自检通过；未跑真机 OpenXR／头显。双语桌面提示与进度已接线，头显内专用引导、肩／裤兜试抓确认和真实玩家验收仍待完成；详见 `tools/vr-launcher/armspan_candidate.md`。

原版拾取／普通近战新增默认关闭、严格只读的 `GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` 候选，位于独立武器分支。它在原自动 Hook 的 PlayerTick 过滤前记录真实 `source=hook` 命中，并将桥接直接调用原函数的请求单独记为 `source=bridge_invoke`；有界记录身体槽子节点实际／目标权重与剩余混合，以及八个原手部控制器身份、强度和当前树链成员。每次采样复核测试场和 PC／Pawn／World／Mesh／AnimTree，全部记录保持 `ownership_proved=0 writes=0`。MSVC Release `/W4 /WX /EHsc` 候选、默认关闭构建、调用关联／缺边界／容量单测及 Hook 回滚测试通过；候选有三个专用导出，默认关闭 DLL 无这些导出。离线回执 `analysis/hand-animation-probe-build-1022a.json`，字段说明 `docs/hand-animation-probe-20261022.md`。

购买版自有置后静音测试场 PID 57480 的有效序列中，测试前不拥有的 Longshot 由原版 `ServerUse` 拾取，`SpecialMove` 进入 53，人物 `Custom_FullBody` 槽播放 `AR_Idle_Ready_Kickup`；约 1.34 秒后原版 `OnAnimEnd` 的序列节点与该槽相同，返回时 `SpecialMove` 已归零，该子节点的实际／目标权重均为零。随后原版 `StartMeleeAttack` 请求使当前武器进入 `MeleeAttacking`；请求维持约 1.4 秒，观察到两次 BeginState／EndState，人物 `Custom_UpStand` 与 `Slot_Std_Idle_Lower` 播放 `AR_Melee_Smack_A`／`C`，各次 EndState 后相应子节点权重归零。189 个边界／周期快照均读到 19 个身体槽；原数组预留容量为 42，先行探针误将预留容量也限制在 32 而漏读槽，之后已把活动槽上限 32 与可接受预留容量分开并复测通过。八个手部控制器在全部快照保持同一对象、当前网格和动画树链身份；链尾 VR 可用目标强度仍为零。终行 `calls=9`、`event_lines=18`、`overflow=0`、`ownership_proved=0`、`writes=0`，`PlaygroundShutdown=0`，11 个静音会话恢复，游戏进程退出；私有回执为 `analysis/hand-animation-probe-runtime-1022e.json`。这局证明正常拾取与普通近战的这些边界和相关槽变化；VM 内部直接调用、取消／受伤交接、其他固定动作、XR 双手驱动和头显可见性仍未证明，**不能由本局把全局手部所有权置真**。两局先行脚本分别因属性类型读取写错和测试人物已持有 Hammerburst 在探针启动前拒绝，均正常清理，不计动画证据。

右手原版 IK 的默认关闭单帧试验已完成一局购买版置后静音运行：新 `GEARS_PLAYGROUND_HAND_IK_TRIAL` 仅在自有测试场、八控制器与原动画稳定后，将当前右手骨世界位置沿 X 移 1 原始单位，用原 `SetSkelControlStrength(0.1, 0)` 激活一帧；下一次原 `PlayerTick` 前归零强度、确认三强度值为零，再恢复原 `EffectorLocation` 的 12 字节，继续观察至少 0.25 秒。PID 36904 的最终回执为状态 5 `inconclusive`、Setter 2 次、目标写入／还原 2 次、`restored=1`、`safe_to_unload=1`、`PlaygroundShutdown=0`；11 个静音会话恢复，自有游戏退出。渲染时间推进，角色 `TickTag` 从 420 到 421，右手在脉冲期间沿目标 X 方向约移动 `0.093` 单位，但左手及原待机动画同时漂移，恢复后又有持续动画位移，**不能把这局判成 IK 骨骼求值或可见双手跟随证明**。旧探针把 `Engine.SkelControlBase.ControlTickTag` 错按 `IntProperty` 读取，保存的包对象盘点实为 `BoolProperty`（artifact 3，object 24790）；所以旧日志的 `tick_known=0` 是类型门控失败，不是未 Tick 证据。随后私有单脉冲候选已改用布尔读取、独立记录网格整数 Tick 与控制器布尔标签，并禁止没有 sham／自然动画基线的单脉冲返回效果通过；离线 Release 构建和 Hook／策略测试通过，修正版单脉冲未再运行；后续六窗对照见本节开头与专项记录。私有回执 `analysis/hand-ik-trial-runtime-1022d.json`。先行零售局因把 `SkelComponent` 错认成 `ObjectProperty` 而在写入前状态 7 拒绝，诊断确定它实际是 `ComponentProperty`；修正后保留了对当前角色网格的身份校验。误输 DLL 哈希的一局在注入前拒绝，不计功能试验。正式默认构建、只读双手候选及 `apply_mask=0` 未改变；拾取／近战交接和真实 XR 双手仍待验证。

自由手动画交接边界已用保存的原版脚本作窄范围复核：拾取可能先 `GrabPickup` 改变库存再进入 `SM_WeaponKickup`／`SM_WeaponPickup`，正常结束须由本动作身体动画的结束通知退出特殊动作，取消可直接替换下一特殊动作；近战从武器 `MeleeAttacking` 状态进入／退出，Lancer 结束后还会有约 0.2 秒身体槽退混合，受伤可能转入踉跄。仅见 `SpecialMove=0`、`bDoingMeleeAttack=false`、武器 `Active` 或 `CanSwitchWeapons=true` 均不能证明双手已交还。购买版现已验证一局正常拾取 53 与普通近战的起止及槽变化，见本节前段；取消、受伤、Lancer 处决、其他固定动作及跨动作槽复用仍缺完整运行序列。丢失任一边界继续保持 `ownership_proved=false`。

当前人物双手 IK 边界先经只读取证：购买版自有置后静音 PID 60744 的当前 WarPawn 缓存了左右各四个手部控制器，均指向当前角色网格且都在其六条 AnimTree 骨骼控制列表中；左手骨 11 对应列表 0，右手骨 35 对应列表 2。每侧链顺序为 `IKBone_*Hand → IKBoneRot_*Hand → IK_*Hand → IKRot_*Hand`。末端 `IKCtrl_*Hand` 位置及 `IKRotCtrl_*Hand` 旋转均以 World 空间为目标，当前强度／目标强度／剩余混合时间都是 0；两侧链首控制器仍受原版动画驱动，左侧取样时尚有约 0.05 秒混合。角色网格未全局忽略控制器。资料在忽略的 `analysis/pawn-ik-runtime-1022b.json`，探针 `PlaygroundShutdown=0`、11 个音频会话恢复和自有游戏退出通过。它认证当前实例的可复用入口；后续单帧右手写入／回滚试验见本节前段，原版动作争用与双手持续驱动仍未验证；默认自由双手候选继续 `apply_mask=0`，不能称可见双手已自由移动。

并行候选已纳入本独立工作树：`GEARS_PLAYGROUND_FREE_HANDS_PROBE` 默认关闭，消费 GVE6 同票据左右手完整世界位姿，在原版 `PlayerTick` 后只读记录原角色双手骨骼矩阵和固定动作状态；离线状态机覆盖双手独立失追、拾取／近战／切枪动画接管、新票据恢复、角色／会话／空间变化。候选与默认 Playground Release 构建、`FreeHandControlTest`、`PlaygroundHookTest` 和五项 Python 日志解析测试通过；导出只存在于候选 DLL。随后自有购买版置后静音 PID 56756 的原角色手骨探针完成 160/160 帧：左手 `b_MF_Hand_L=11`、右手 `b_MF_Hand_R=35`，当前角色 `SkelControlIndex` 长 115、动画树控制列表 6 条；160 帧双手骨矩阵均有效，因没有 XR 姿态 `xr_samples=0`，写入与 `apply_mask` 均为 0。忽略回执 `analysis/free-hands-probe-runtime-1022a.json`，`PlaygroundShutdown=0`、11 个音频会话恢复、自有游戏退出通过。当前 `ownership_proved=false`，**游戏内可见双手并未被驱动**。原版拾取完整生命周期、可恢复的双手骨骼写入、掌心标定和头显运行仍待解决。

空手姿态发布增量：另一个默认关闭的 `GEARS_FREE_HAND_POSE_OBSERVER` native 候选在同一已认证 XR 票据内允许当前武器为空，或原版固定动画暂时接管手部时，继续发布只读双手世界姿态；当前 PC↔Pawn、世界、字段布局及存在武器时的实例身份仍须通过校验。装备意图依然要求非空武器，失追可单手独立撤销，动画结束后只接受新票据。`HandPosePublicationTest`、`EquipmentIntentBridgeTest`、`FreeHandControlTest` 和默认／候选构建通过；候选 DLL 哈希 `65c67d957fab3b28362d9ac296c4da4c34c11657a4ed59a11928b59833f7c8ed`。XR 失焦时发布端继续拒绝有效输入；此候选未做购买版或头显测试，`apply_mask=0` 及默认启动器均未改变。

枪上弹数卡片新增默认关闭的独立 OpenXR 候选：从经认证的原版 `GetMagazineSize − AmmoUsedCount` 与 `SpareAmmoCount` 生成“枪内／备用”纹理，在当前枪体局部挂点姿态上构造双眼 Quad，前后向及过期／错票据均拒绝；手雷独立显示可用数，不放完美换弹条。MSVC `/W4 /WX` 候选测试 3/3 通过，默认 `OpenXRPoseProbe` 构建通过，CPU 预览已核对。当前没有经认证的游戏线程弹药快照跨进程桥、同票据枪模位姿和逐枪挂点，也没有原 HUD 弹数／完美条各自绘制归属的证据；所以卡片尚未进入生产 XR 循环，右上角原 HUD 尚未移除，不能声称头显可读。

Lancer 控制索引继续只读核验：自有购买版置后静音 PID 58808 的当前枪模 `SkelControlIndex` 长度 9，骨骼 0–7 均为原始字节 `255`，骨骼 8（弹匣）为 `0`；`RequiredBones` 为 0–8，`AnimTickArray` 正好是当前 `AnimTree` 与其 `AnimNodeSequence`，原 `AmmoControl.SkelComponent` 指向该枪模组件。实时 UFunction 元数据确认 `SetAnimTreeTemplate(NewTemplate)`、`UpdateAnimations()`、`SetSkelControlStrength(NewStrength, InBlendTime)` 的参数结构，但没有调用它们。这个索引使“仅追加卡扣 7、机匣 5 的控制链就生效”的假设不成立；仍须引擎管理的对象创建、数组维护、索引重建与 GC／恢复证明。忽略回执 `analysis/lancer-anim-tree-runtime-1022c.json`，`PlaygroundShutdown=0`、11 个音频会话恢复及游戏退出通过。

完美换弹期限从推导推进到当前实例实测：自有购买版置后静音 PID 57472 在一次原版 Lancer 换弹中，经游戏线程调用原 HUD 本来使用的 `GetActiveReloadValues`，两次分别于换弹约 0.201／0.463 秒取得相同四值：起点约 `0.700000`、预反应 `0.25`、完美宽度约 `0.100000`、普通成功宽度 `0.25` 秒。按已匹配原版判定函数的 `elapsed ≤ start + super`，这次 Lancer 的完美**终点**为约 `0.800000` 秒，自 `ReloadStartTime` 起算；这不是所有枪或所有游戏状态的常数。两次 Getter 调用前后主动换弹尝试标志均未变化；原版换弹随后自行结束、已耗弹从 1 归零。数值回执在忽略的 `analysis/lancer-reload-getter-runtime-1022a.json`，`PlaygroundShutdown=0`、11 个音频会话恢复、自有游戏退出均已核对。没有用实体动作结算完美奖励，也没有头显验收。

Lancer 动画树当前实例只读核验：购买版自有置后静音 PID 46552 的 `Weapon.Mesh.AnimTreeTemplate` 为 `AT_COG_AssaultRifle`，`Animations` 是另一份当前实例 `AnimTree`，仅有一个 `AnimNodeSequence` 子节点；其 `SkelControlLists` 只有 `b_AR_Magazine → AmmoControl`，`bIgnoreControllers=0`、`bIgnoreControllersWhenNotRendered=false`。这为保留原动画通知并另加卡扣／机匣骨骼控制提供真实拓扑，但**没有创建或挂接新控制器**，也未证明动画计算顺序／GC。只读回执在忽略的 `analysis/lancer-anim-tree-runtime-1022b.json`，私有 `PlaygroundShutdown=0`、11 个音频会话恢复、游戏退出通过；先行试验把模板字段误查到武器而非其 `Weapon.Mesh`，在图读取前停止，没有作为通过证据。后续仅在引擎对象创建、列表所有权和恢复路径认证后做默认关闭的单控制器试验；不能直接改骨数组或把现有 `AmmoControl` 换掉。

控制器接线审查确认当前证据还不能安全实现新增 `SkelControlSingleBone`：缺少当前构建的引擎对象创建入口、`SkelControlLists` 的引擎管理扩容、`SkelControlIndex` 重建和新对象的 GC／解绑证明。现有 `SetAnimTreeTemplate`、`UpdateAnimations`、`SetSkelControlStrength` 声明不等于这些路径已经验证。下一步只读记录当前实例控制索引、相关数组和实时接口签名，之后才考虑默认关闭、零强度的单控制器试验；不直接串改 `AmmoControl.NextControl` 或用 DLL 内存替代引擎数组。

Lancer 原版换弹机构新增同帧只读核对：自有购买版置后静音 PID 46348 中，对一次原版 `ForceReload` 取得 321 组枪体／卡扣／机匣世界矩阵，160 组处于 `Reloading`；卡扣 `b_AR_Latch` 为骨骼 7，相对枪体绕局部 +X 最大转 114.5°，机匣 `b_AR_Receiver` 为骨骼 5、绕 −X 最大转 28.5°，两者枢轴位移均低于 0.00031 游戏原始单位。卡扣从约 313 ms 起运动、约 2,282 ms 回位；机匣从约 360 ms 到 2,266 ms，说明原版序列里两件大部分时间同时处于打开状态，不能凭名称把卡扣直接当作玩家插弹后才拉动的“枪栓”。只读原始回执与确定性运动摘要分别保存在忽略的 `analysis/lancer-reload-bone-runtime-1022c.json` 和 `analysis/lancer-reload-bone-motion-1022c.json`；新分析脚本可复算矩阵。`PlaygroundShutdown=0`、11 个静音会话恢复、游戏退出均已核对。一次先行启动没有可用菜单视口，在采样前停止；没有被用作通过证据。这些矩阵不确定实际可抓表面、黄色高亮或头显中的动作，也没有完成手动换弹。

逐枪离线模块进展：Hammerburst 新增专用弹匣／闩锁换弹事务与黄色步骤，固定动作顺序必须先经当前原枪实例认证；Gnasher 新增每轮最多三发、每发库存提交、射后双手杠杆循环与原版动画交接；Snub 新增左裤兜手枪身份、弹匣守恒、空仓／半仓逐实例认证步骤、失追恢复与一次性原版提交；Longshot 新增单发物理换弹认证计划，以及双眼独立相机、离轴投影、虚像焦平面、镜筒遮挡和真实瞄准交点投影接口；Boomshot 新增无默认弹数／机械顺序的认证计划、原版库存提交与射击回执门禁。五者均接入独立 CMake 单测并在 MSVC Release 构建与运行通过；Snub 合成 114 组弹药守恒／重试、Longshot 换弹 178 项和光学 136 项、Boomshot 611 项通过。Gnasher 当前步骤在取弹时指向左肩，握住子弹后才转装填口。这些是纯逻辑／数学基础，握点、装填口、时限和镜片光学参数仍需零售游戏实例标定，尚无真实手柄、GPU 瞄准镜或默认启动器接线，不能算逐枪完成。

Torque Bow 逐枪纯逻辑另已接入 CMake 并通过合成测试：R3 左肩取箭、认证步骤高亮、原版 EndFire 请求和实际弹体／弹耗回执分离；失追和姿态过期不能提出放箭。保存脚本显示持续蓄力到上限也可能调用发射，所以当前零售版没有证明“不发箭中止”能力时，生产配置的蓄力门禁保持关闭。装填顺序、原版完美终点及安全中止入口都待购买版实例验证；它不代表游戏里已可用。

Boltok 专用纯逻辑已接入 CMake、MSVC Release 单测通过：左裤兜取枪、左肩获取经认证的实弹、当前实例提供机构移动／逐次装入计划、每步黄色提示、原版库存一次性回执及原版射击弹道／后坐力保留门禁。测试中 7 发容量仅是合成夹具，未写作游戏容量。原包网格通用骨名与默认 Snub 动画轨道不匹配，因此没有默认装填动作、部件语义或弹药模型；游戏线程和手柄尚未接入。Hammer of Dawn 另有目标／资源／蓄力／照射与双手动画交接协议，合成 500 项在 Release 构建通过；原版 `IsAValidHODTarget` 等原生签名、使用条件和非发射取消入口未实证，故不启用实际使用，也不虚构弹匣或普通 R3 换弹。

Bolo 手雷专用纯逻辑已接入 CMake、MSVC Release 1,144 项合成检查通过：右裤兜无弹时只产生绑定当前实例与原版音效身份的一次性提示意图；有弹时保留一枚并通过原版准备／发射票据，最近 140 毫秒手部与手腕运动给原版投掷物提供 3 kg 模型的初始位置／速度，库存只认原版一次性回执。扳机按下与松开必须属于同一次持有，释放票据匹配手部帧序列、输入纪元和同步时间，失追、过期或恢复造成的伪边沿不能投掷。原版引信状态在中断后不能免费重置，旧投掷不会缓发；原链条、爆炸和音效必须由原版保留。释放输入、引信归属、原生初速覆盖及安全取消入口尚未在购买版当前实例验证，配置默认闭锁；没有游戏内真实投出或头显手感结论。

Troika 与 Smoke Grenade 各有独立纯逻辑模块并接入 CMake，MSVC Release 合成测试分别通过 350／1,730 项。Troika 按固定机枪处理，验证固定枢轴、双手握把与两枪口身份，原版上下机枪动画、射击、资源和热量各依回执；不假定双枪口交替，也不默认身体槽、左肩弹匣或 R3。Smoke 单列原版烟雾武器／投射物／弹药／发射器／音效类，右裤兜无弹音效带一次性令牌；释放票据必须匹配同帧手部序列，投出仍走 3 kg 手势轨迹及原版一次生成／一次耗弹，原烟雾和音效路径保留。两者缺当前实例的原生调用、机械／引信／安全停火与渲染证明，生产配置保持闭锁，尚无游戏内实际交互。

Lancer 锯刃候选新增仅只读的同帧探针：在购买版自有置后静音 PID 53896 中，以已核原枪网格的同一 `PlayerTick` 矩阵取得 16 帧枪体和原两枢轴矩阵，并记录由 12 点可见锯链外缘近似变换的世界位置；离线复算各点与对应枪矩阵最大差约 `5.15×10⁻⁵` 原始单位。完整坐标、日志与来源哈希留在忽略的 `analysis/lancer-blade-edge-runtime-1022p.json`；游戏退出，测试音频守护对 11 个会话恢复成功。此回执只证明候选外缘随当前枪矩阵同帧变换，不证明锯齿准确碰撞、敌人接触或原版处决。两次先行尝试分别因工作区 Python 库路径和私有 DLL 未编入探针在采样前停止，均关闭各自游戏并恢复音频，不作通过证据。

枪口射线复核：保存的普通 `InstantFire` 链在原版随机散布之前可观察 `GetAdjustedAimFor` 基础方向，但最终 `PushedForwardStartTrace` 可再改动起点；Gnasher 有独立 `CustomFire/FireAllPellets` 和使用玩家视角方向的近距离分支，不在普通步枪观察器覆盖内。原版后坐力还会经 `SetWeaponRecoil → ProcessViewRotation` 改变瞄准基底；若直接替换为手柄轴可能丢失实际反冲。先对单发与短连发做只读关联，再决定逐枪安全改向入口；此项是静态证据，未声称当前零售版枪口开火通过。

Lancer 锯刃接触几何更正：同一哈希已核原模型 LOD0 的锯链模糊材质构成枪体局部 Y `26.30–75.86`、Z `-7.21–10.31` 的环，原先两根骨枢轴只在环内部，不能直接当接触线。独立分支新增基于可见环外缘 12 点的稀疏候选及 `LocalToWorld` 刚性检查接口；原枪身份与同帧性仍由未来游戏线程适配器负责；接触模型改要求原版权威、持锯、目标存活与控制器、双方资格、遮挡及粘附目标一致性门禁。纯状态／姿态测试、私有 XR 候选与默认关闭构建通过。材质／骨骼二维审计图只留在忽略的 `analysis/lancer-blade-geometry-1022/`。购买版自有置后静音 PID 61076 再通过原版 `Spawn`／`SpawnDefaultController()` 创建一名 `WarAI_Locust` 控制的独立敌人，其 `Controller.Pawn`／`MyWP` 回指一致，两项原版近战资格函数均为 true；常规无控制器靶被原版特殊近战资格函数拒绝。只读资格测试没有持锯、实体接触或处决；`PlaygroundShutdown=0`，游戏退出且音频恢复，回执仅在忽略的 `analysis/lancer-saw-target-runtime-1022m.json`。边缘坐标仍是静态近似，XR 适配器没有真实敌人接触或原版 `AcquiredVictim` 调用，后续须验证同帧物理接触、遮挡及双方动画自然完成。

Lancer 电锯动画交接修正：已保存的原版脚本显示，处决特殊动作 34 刚进入时松开 B 可能经近战 `EndFire` 强制结束动作。独立状态机改为在原版自行退出攻击后才请求松 B，并待原版按键字段清零才恢复双手 6DoF；处决进入与 XR 失追同帧的竞态也由原版动画优先。私有原版 B 适配器增加特殊动作 34 的释放保护。默认关闭的新 `GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST` 把认证 GVE6 真实手姿、当前 Lancer 枪矩阵、临时拉柄点和连续拉程接到原版 B 待机，须显式 Arm；15 秒试验看门狗和失追清理仅供开发，未实现实际锯刃目标获取。状态机／姿态纯测试、私有与默认 Playground 构建通过。购买版自有置后静音 PID 44484 验证显式 Arm 后没有 XR 手姿不会误按 B：约 0.45 秒均维持待命 2，Stop 后状态 8，原版近战／锯链／受害者为空、关闭码 0、自有游戏退出且音频恢复，回执仅在忽略的 `analysis/lancer-saw-xr-no-pose-runtime-1022k.json`。最后一次释放门更新后，PID 51708 取得 9 个原版 B 保持、锯链运转、自动寻敌计时器关闭的样本；显式停止、关闭码 0、自有进程退出及音频恢复通过，回执仅在忽略的 `analysis/lancer-saw-xr-final-hold-runtime-1022l.json`。两次运行都没有真实 XR 拉动或敌人处决，不能作为头显验收或启用默认启动器。

体感动作单位比例接线：私有双手姿态桥从 GVE5 升至 GVE6，原生发布端把与手部世界姿态**同一 XR 票据**使用的游戏单位／米比例写入帧，桥接边界只接受有限的 10–500 范围，旧协议的映射名隔离。Lancer 电锯新增世界手部姿态到米制手拉样本的转换，核对同一控制器／人物／武器／世界代次及 100 ms 新鲜度；纯测试证明 100 或 200 单位／米时同样的实际 19 cm 拉程都提出原版待机请求，失效比例、过期或身份变化拒绝。桥单测、电锯状态机与转换单测、私有 native 与 Playground 候选／默认构建通过。这是原版电锯按键的**输入适配准备**：拉柄尚未在原枪定位或渲染，游戏线程尚未调用原版 B／抑制寻敌计时器，未在游戏中启动锯链，也未合入默认启动器。

Gnasher 原版持枪手骨增量：购买版自有置后静音测试场 PID 8720 按原版拾取／装备流程取得当前霰弹枪，先确认人物 `ShouldLeftHandIKBeOn=true`，再于待机姿态读取 24 组枪主骨、杠杆、左右手骨及左手 IK 骨世界矩阵。换算到枪主骨，右手骨原点中位数约 `(-2.3163, -17.5396, -9.9111)`、左手约 `(8.5540, 25.9871, -9.7627)` 原始单位；最大位置漂移分别 `0.0925`／`0.2972`，选定样本的朝向跨帧最大漂移分别约 `0.28°`／`0.36°`。右手骨到已测杠杆枢轴的垂轴距离约 38.8 原始单位，故测试几何现用实测手骨原点和枢轴构造下掰圆弧，纯测试通过。**手骨原点不等于握把网格接触面或 Quest 控制器握点**；仍需实枪接触、控制器到掌心标定及头显验证。本次完整回执与 24 组矩阵只在忽略的 `analysis/gnasher-held-runtime-1022i.json`，可由 `tools/weapon-models/analyze_gnasher_grips.py` 重算摘要；`PlaygroundShutdown=0`、游戏 PID 退出、音频守护恢复。前三次诊断分别遇到资料表刷新、不可读名称及探针提前返回，均未用作通过证据且各自已清理；一份未记录 IK 状态的先行 24 帧样本仅作交叉检查。默认 DLL 未启用该机构。

Gnasher 杠杆手势校准：复用购买版 PID 43448 的 35 帧 `SG_Chamber` 实例矩阵，修正原纯逻辑样例要求下掰 0.8 rad（约 46°）、超过原版最大 32.43° 行程的问题。新候选采用实测局部 −X 转轴与枢轴 `(0.01479, 20.62221, −2.92057)` 原始单位；完整下掰阈值 0.44 rad（约 25.2°）、拒绝超过 0.70 rad（约 40.1°），后两者是待头显校准的 VR 容差而非原版时限。双手几何和循环单测已覆盖完整下掰回位、渐进越程、方向、半径、追踪失效和头部运动不改变杠杆角度。右手握把实际接触点尚未从原版资源标定，游戏线程、原骨骼运动、射后禁射及黄色引导仍待接入；本条没有新零售版运行，也未启用默认 DLL。

Lancer 原枪网格跟手的购买版组件验证：在自有置后静音测试场 PID 40068，先通过私有右手在左肩槽取武器 2意图让 VR 装备模型确认持枪，再让游戏线程以 12 个短行程合成右手姿态驱动**同一个**原版 `COGAssaultRifle` 组件。世界矩阵跟随前后约 `+16／−8` 游戏原始单位并发生小角度旋转，逐姿态稳定后的最大 X 误差 `0.00977` 单位；候选退出后原相对变换与绝对标志恢复，`PlaygroundShutdown=0`。第一次未提交槽位取枪时模型正确拒绝；第二次立即读取 `LocalToWorld` 得到上一帧值，因此最终试验保持每个位姿 32 ms 后核对。原始运行回执只在忽略的 `analysis/lancer-support-probe-20261022/capture-mesh-follow-trial-retail-3.json`；原生入口还原、XR 退出 0、游戏静音恢复、准确自有 PID 退出均确认。此结果证明私有候选能移动和恢复原组件，**不证明 Quest 追踪、当前帧渲染、枪口弹道或后坐力**；默认 DLL 未启用。为下一步实际握点标定，`WeaponMeshFollow` 新增显式的原版手掌局部姿态与手柄到掌心朝向接口。已保存的原版 24 帧样本中右掌局部中位点约 `(-2.322616,0,0)`、手骨坐标相对枪体约偏航 `+90°`，但 Quest 手柄到掌心标定还未取得；新接口只经数学测试，尚未接入运行时。

Lancer 启动件原创模型原型：新增 `assets/weapon-6dof/lancer-starter.glb` 与确定性生成器，将固定插座、跟手 T 形拉柄和可延长绳分成三个节点；704 三角形，不含原游戏模型或贴图。忽略目录内的静态预览已经检查。尚未在游戏中定位、渲染、拉伸绳或用头显验证，不能把资产原型称为体感电锯完成。

购买版自有置后静音测试场 PID 51388 复核原版电锯边界：对已装备 Lancer 调用一次原版 `ButtonPress('B')`，按住约 0.85 秒期间控制器 B 与人物近战意图为真、`bDoingMeleeAttack=true`、`CurrentFireMode=2`、`SpecialMove=33`、锯链运转，`CheckForMeleeTarget` 计时器有效且没有受害者。清除该原版寻敌计时器后再保持 0.45 秒，近战／锯链继续，计时器没有自行重建；调用原版 `ButtonRelease('B')` 后近战、锯链、计时器和意图均恢复。回执只在忽略的 `analysis/lancer-support-probe-20261022/capture-chainsaw-semantics-retail-3.json`。反射直接调用类方法 `IsMeleeing` 即使处在上述状态也返回 false，不能拿它作为状态判据。这只证明无目标场景中的按键与计时器边界，尚未验证接触受害者后的原版处决、VR 拉柄或头显画面。Playground 入口／静音恢复通过，停止命令后仍留存该自有测试进程，已核对路径和创建时间再结束；完整 VR 会话未完成。

新增 Lancer 体感电锯目标：左手抓拉柄并完成连续拉程后启动，锯刃实际接触有效敌人时调用原版双方处决，原版动画结束后恢复双手 6DoF。原包静态脚本核对了 `WarPC.PreButtonPress('B')`、`Weap_AssaultRifleBase.MeleeAttacking.BeginState/CheckForMeleeTarget/AcquiredVictim` 及原版清理链；原版前方寻敌计时器不能在 VR 交互期间抢先取得目标。原枪 `b_AR_Chainsaw_Handle` 是 685 顶点／1,089 面的既有独立刚性部位，尚无证据可作为启动拉柄，检查 OBJ 仅在忽略的 `analysis/weapon-partitions/Lancer-chainsaw-handle-audit/`。独立分支新增 `weapon_lancer_chainsaw.h`：按握持边沿、拉程、方向、连续性和身份做启动候选，原枪锯刃线段／敌方胶囊接触候选必须再经原版有效目标与遮挡确认，明确在处决期间让原版动画持有双手、结束后再交还。合成测试通过；游戏线程 B 状态适配、原版寻敌计时器抑制、可见拉柄／拉绳、真实接触与头显测试仍未完成，不接入默认启动器。

后续私有 Lancer 电锯待机适配已补齐原版 B 键保持、每个自有游戏帧清除 `CheckForMeleeTarget` 自动寻敌计时器、身份丢失时释放，以及退出时等待原版 B 按住字段清零。购买版自有置后静音测试 PID 39340 的 9 个持续样本均为原版 B 按住／近战／锯链运转／模式 2，计时器关闭且没有受害者；明确停止后原版状态恢复。PID 61196 在 B 仍按住时执行 `PlaygroundShutdown`，关闭码 0，原版 B、近战、锯链与模式均恢复空闲。两次自有进程退出且音频守护恢复；证据只在忽略的 `analysis/lancer-saw-hold-runtime-1022a.json`、`analysis/lancer-saw-shutdown-runtime-1022b.json`。此结果不覆盖 XR 手拉启动、可见拉柄、实体锯刃接触、原版双方处决或动画后 6DoF 恢复；默认 DLL 未启用该测试选项。

另一次购买版自有置后静音只读探针 PID 61168 核对了原版 Lancer 电锯函数签名：`AcquiredVictim(VictimPawn)` 的参数区为 8 字节对象引用；`IsTargetValidForSpecialMeleeAttack(P)` 为对象入参加布尔返回。`MeleeAttackImpact` 有伤害和命中输出参数，不能当成实体锯刃接触入口。本次未调用目标获取或处决；回执仅在忽略的 `analysis/lancer-saw-api-runtime-1022c.json`，自有进程退出、音频恢复。下一步仍须逐帧定位原枪锯刃骨骼、验证敌方碰撞和遮挡后，才允许对同一有效受害者做一次原版动画移交。

原枪锯刃骨骼只读采样改用同一游戏线程帧，排除第一次跨反射调用时枪组件在两次读之间更新的无效尝试。购买版 PID 53752 普通持枪及 PID 36516 原版锯链运转各采 16 帧，`b_AR_Chainsaw`／`b_AR_ChainsawEnd` 索引均为 1／2；枪体局部枢轴约 `(0.008,34.127,5.651)`、`(0.008,56.562,4.476)`，距离约 22.466 原始单位，两组漂移均小于 0.001。PID 52172 的增强采样逐帧复核原版 B、锯链和人物近战确实持续为真，仍得到相同枢轴。启动电锯没有改变枢轴相对枪体的静态位置。原始矩阵仅留在忽略的 `analysis/lancer-saw-bones-runtime-1022e.json`、`analysis/lancer-saw-bones-held-runtime-1022f.json`、`analysis/lancer-saw-bones-held-runtime-1022g.json`；`tools/weapon-models/analyze_lancer_blade_bones.py` 可重算摘要。三次有效试验 Playground 关闭码 0、精确自有游戏进程退出，音频守护恢复。枢轴不等于可见锯齿外缘，不能凭此认定敌方碰撞或处决已实现；默认 DLL 未启用探针。

合成左手拉绳至原版电锯待机的私有端到端路径通过购买版 PID 32496 验证：临时枪体局部拉柄点 `(6,26,10.65)` 随原枪矩阵变换；同一世界单位比例／身份检查后的连续拉动在 297 ms、18.81 个原始单位时仅请求一次原版 B。其后 9 个样本中原版 B、近战和锯链运转，前方自动寻敌计时器均关闭、受害者为空；显式停止后状态恢复，Playground 关闭码 0，自有进程退出、音频恢复。首次 PID 41264 已达原版请求但漏发运行状态码，验证器超时；现已修复，以忽略的 `analysis/lancer-saw-gesture-runtime-1022i.json` 为通过证据。`weapon_lancer_chainsaw.h` 也修正为只在启动前要求 `CanDoSpecialMeleeAttack`，进入原版持有状态后依原版 B 状态持续，纯测试通过。**这仍是模拟手部轨迹**，临时点位未可视化、未校准真实 Quest 掌心；真实锯刃接触、原版双方处决及动画后恢复双手 6DoF 未完成。默认 DLL 仍不包含该候选。

范围更正：玩家确认先前清单误列 Mulcher 和 Ink Grenade，本轮只实施交互设计中剩余的 12 件武器／投掷物。此前“14 种”与这两件待查模型的条目属于当时的历史记录，已由本条和更新后的设计清单取代；无需再为它们追索模型或开发交互。

Lancer 下一机构边界审核：已核验原包的 `AR_Reload` 中 `b_AR_Latch` 与 `b_AR_Receiver` 各有 38 个旋转键、仅一个平移键，`AR_Reload_Fumble` 各有 60 个旋转键；`b_AR_Latch` 的原 LOD0 是 298 个刚性顶点／276 面，位于枪下方，源骨枢轴候选在枪体局部约 `(0.008,21.391,-7.540)`。有界解码显示 Latch 的存储局部姿态经历约 114.5° 相对旋转，但尚未验证动画键时间、游戏运行骨矩阵和可见操作面，因此不能只凭名字把它叫作枪栓。解码回执和检查用 OBJ 只留在忽略的 `analysis/lancer-support-probe-20261022/`、`analysis/weapon-partitions/Lancer-latch-uv/`，没有接入发行模型。

Lancer 插入判定候选：独立分支新增原枪固定弹匣井锚点、原版左手携弹匣姿态与当前枪组件矩阵的同帧几何检查；须先在井外合理深度且轴向、横向对齐，之后连续向内移动至到位。相同输入序号、空间／输入代次变化、跟踪间断、瞬移、背离和错向均不能记作插入。原版采集末段弹匣约在井口外深度 1.5、横向 1.8 个原始单位，当前私有容差只作候选，尚无 Quest 握把到掌骨标定。纯几何测试覆盖正常、旋转枪体、枪体移动和拒绝路径；默认关闭的 Playground 只记录插入候选，不转移弹药、隐藏手中件或完成装填。候选与默认禁用构建通过。购买版置后静音 PID 48504 复测原有 1.8 秒模拟左手轨迹，弹匣跟随、原版通知后的重挂、松握卸下与原版一次性结算均通过，未出现虚假的插入候选。新增测试专用的对准路径在购买版 PID 2348 上从井外深度 12 连续移到 2.34 个原始单位，横向 1.8、轴向及朝向对齐值均为 1，只产生一次插入完成候选；松握后原组件卸下，原版装填仍一次性结算。原始回执在忽略的 `analysis/lancer-support-probe-20261022/capture-insertion-observer-retail.json` 与 `capture-insertion-aligned-retail.json`。两次 Playground 入口和八个音频会话恢复通过，但启动器停止命令返回 1、各自原测试进程仍在，核对创建时间与路径后单独结束；完整 VR 会话仍为 `incomplete`。这些都是模拟手轨和候选记录，枪栓、黄色提示、完美奖励与头显验收均未完成。

Lancer 连体模型处理推进到原版独立组件运行验证：购买版自有置后静音测试场 PID 39128 只读反射确认 `SetPawnAmmoAttachment(bAttach)`、`SetWeaponAmmoBoneDisplay(bShow)` 与 `MagazineMesh` 的 `SetAbsolute`／`SetTranslation`／`SetRotation` 参数签名；该组件使用原版 `Assault_Rifle_Magazine`。PID 53212 将它挂到原角色、世界 X 平移 5 个原始单位并卸下回位，实测组件矩阵到目标误差约 0.015，接口调用与恢复通过。独立分支新增默认关闭的 Lancer 左手跟随候选，复用这个原版组件和枪上 `AmmoControl`，并用已测原版左手骨骼相对姿态构造**暂未标定 Quest 掌心**的握持候选。PID 38068 的测试专用 1.8 秒左手轨迹在原版 R3 装填保持期间取得四个同一组件的游戏线程样本：枪上控制器目标始终为 1、独立组件始终挂在角色上，原版通知将其卸下后候选重新挂接；跳过首次 `LocalToWorld` 尚未更新的样本后，组件世界位移 8.644 单位，手到弹匣相对偏移最大漂移 0.0015 单位。松握后组件卸下，随后原版装填完成函数一次性结算弹药。原始回执仅在独立工作树忽略的 `analysis/lancer-support-probe-20261022/`；三次游戏、Playground Hook、静音和入口均正常收尾，整局 VR 会话仍为 `incomplete`。这证明原件可被接管和移动，**不等于实体插入、枪栓、头显画面或任何其他武器已完成**。正式默认 DLL 与启动器未启用该候选；非 Lancer 部件继续优先核对原骨骼与原独立组件，只有缺失可用原件才制作可发布的独立覆盖模型。

Lancer 换弹结算边界新增零售版隔离验证：在购买版自有置后静音测试场 PID 44552，先由原版 `ForceReload` 进入 `Reloading`，确认 `EndOfReloadTimer` 计时器存在，清除后游戏时间越过原版 2.5 秒至 3.221 秒，已耗弹量仍为 1 且仍在装填；随后精确调用原版 `Reloading.EndOfReloadTimer` 状态函数，已耗弹量变为 0 并退出装填。证据在忽略的 `analysis/lancer-support-probe-20261022/capture-44552-reload-hold.json`。据此在独立分支增加 GVE5 认证 R3 意图（会话、空间、输入代次、世界和当前枪身份），以及默认关闭的 Lancer 游戏线程计时器移交试验。它只接受私有测试场、当前原版 Lancer 和可装填弹药，将原版自动完成改为 8 秒原版计时兜底；明确完成仍走原版状态函数，不自行改库存或发完美奖励。购买版 PID 53584 的**测试专用完成入口**验证了 3.270 秒时仍为已耗 1 发，完成后已耗归零、退出装填；回执在忽略的 `analysis/lancer-support-probe-20261022/capture-53584-reload-adapter.json`。这一运行样本未由真实 R3 或实体插弹触发，未验证 8 秒兜底实际到点、弹匣/枪栓动作、黄色引导、完美奖励或头显画面。最终加固了新输入代次与新鲜手部姿态匹配、空弹提示音和完成失败时重建兜底计时；这些后续改动编译及桥单测通过，尚未重复零售版运行。默认启动器与正式 DLL 未替换。两次自有游戏均退出，Playground 入口恢复、音频恢复各自通过；完整 VR 会话仍为 `incomplete`。

Lancer 左手副握点运行样本：购买版私有置后静音测试场 PID 47456 在原版装备动画结束、`ShouldLeftHandIKBeOn` 为真后，完成 24 次原版 `PlayerTick` 后的同线程人物左右掌骨／IK 骨及枪组件矩阵采集。枪体局部左掌心中位数为 `(8.556242, 43.525180, 0.165157)` 游戏原始单位，跨帧最大漂移 `0.000231`，左右掌心距离约 `44.86`。原始数据与分析留在忽略的 `analysis/lancer-support-probe-20261022/`；探针关闭、游戏退出和音频恢复完成，完整 VR 会话仍为 `incomplete`。据此将实测点接入**私有、Lancer 限定**的原枪组件双手跟随候选：左手必须在接触半径内新按 LGrip，松键、失踪或离开范围回退右手；绑定时已按住不能误抓。纯数学／边沿测试、候选和默认禁用构建通过。尚无实际手柄带动枪模、后坐力、射线或头显验收；未换正式启动器／DLL。

模型运行时只读增量：第二个购买版私有置后静音测试场 PID 37984 再次完成 24 帧原版持枪样本，并记录当前 Lancer 的独立 `MagazineMesh` 确实指向 `Assault_Rifle_Magazine`；`HiddenGame=false`、未挂在人物骨架，枪上 `AmmoControl` 存在且 `ControlStrength`／`StrengthTarget`／`BoneScale` 都为 0。一次原版静止持枪不能说明弹匣最终渲染可见性或装填交接时刻；下一步在原版换弹的同一条时间线上观察两份组件。首轮对象扫描遇到已失效对象类引用后已改为跳过该槽，复跑成功。私有探针退出码 0，游戏进程退出、Playground 入口恢复及游戏音频恢复均核对；完整 VR 会话仍是 `incomplete`，默认构建和发行包未替换。

Lancer 原版换弹视觉交接实测：购买版私有置后静音测试场 PID 34536 在测试场把原枪已耗弹量临时设为 1，调用原版 `ForceReload`，私有游戏线程只读采集 318 个顺序样本。相对原版 `ReloadStartTime` 的采样间隔：枪上 `AmmoControl` 由 0 到 1 为 `0.258–0.275 s`；独立 `MagazineMesh` 挂上角色骨架为 `0.774–0.791 s`；独立件卸下且枪上控制器回到 0 为 `1.479–1.500 s`；原版退出 `Reloading`、已耗弹量由 1 归 0 为 `2.497–2.510 s`。这些是连续两帧之间的事件范围，不是动画通知的精确时间，也不是 VR 完美期限。`HiddenGame` 全程为 false，但挂接和渲染可见性不同；正常 VR 弹匣仍需绑定左手控制器并验证画面。原始逐帧数据与派生摘要留在忽略的 `analysis/lancer-support-probe-20261022/`。此次游戏进程、Playground 入口和音频均清理，完整 VR 会话仍为 `incomplete`。默认启动器／DLL 未更换。

购买版实例补充：私有置后静音测试场 PID 47308 的当前 Lancer 实例，只读反射验证了装填时长 `2.5 s`、起点约 `0.7 s`、三个未覆盖宽度字段 `-1`、完美宽度数组 `0.1／0.075／0.05 s` 和角色索引 0。据已匹配的 Getter 主块与历史尾块推导此实例候选终点约 `0.800000012 s`；未调用 Getter，也未触发装填奖励，不把候选值配置为正式期限。回执在忽略的 `analysis/weapon-reload-probe-20261022/reload-window-instance-47308.json`。私有组件编译、入口还原、原生清理、音频恢复和游戏正常退出通过；完整 VR 会话仍为 `incomplete`。正常启动器及正式 DLL 未更换。

完美边界原生核验增量：仅分析已与购买版当前 Lancer 虚表目标逐字节匹配的判定函数主代码块，确认完美条件为 `elapsed ≤ start + super`，包含窗口右边界；独立分支纯装填事务已按此修正，恰好到点／晚一步测试通过。Getter 主块使用默认对象上的难度索引数组作为未显式设置时的完美宽度；其分离尾块目前仅为历史快照。哈希核对的购买版 `WarfareGame.u` 基类数组为索引 0／1／2 对应 `0.100／0.075／0.050 s`，六把枪及中间基类未显式覆盖。报告在忽略的 `analysis/weapon-reload-probe-20261022/active-reload-defaults-with-difficulty.json`。逐枪当前游戏实例输出和索引尚未读取，这些默认值推导的候选期限不写入正式配置。数组解码单测与 `WeaponReloadModelTest` 通过；未替换默认 DLL 或启动器。

模型与装填原生实测增量：从当前 0.1.0 工作区选择性移植早期 Playground 预留及菜单定位修复到独立分支的测试组件，保留本分支私有观察器；`PlaygroundHookTest` 的邻近窗口／全窗口占用正反例、装备意图和默认禁用构建通过，未替换正式默认 DLL。购买版私有置后静音测试场 PID 52656 读到原版换弹包装入口 RVA `0x3e7ffc`／`0x39bfcc`；前者前 256 字节、后者完整 47 字节与历史运行时代码快照相同。两者分别转经武器虚表 `0x638`／`0x630`；随后 PID 44876 的当前 Lancer 实例验证了虚目标 RVA `0x3e81b0`／`0x3e7d28`，各 4 KiB 与历史运行时代码快照逐字节一致，Getter 的分离尾块不在比较范围内。这只证明当前代码身份，尚未算出逐枪完美期限。PID 43448 成功取得 Gnasher 原版 `SG_Chamber` 的 35 组真实骨骼矩阵，相对枪身最大转角约 32.43°、局部 −X 转轴，骨原点对应原参考枢轴。原杠杆骨骼可直接作为模型运动源，仍未接手柄、黄色引导或装填奖励。上述原始字节与矩阵仅在忽略的 `analysis/weapon-reload-probe-20261022/`；完整 VR 会话保持 `incomplete`，Playground 入口还原、原生清理、XR 退出 0、音频恢复和测试游戏正常退出分别核对，不当作头显验收。

原生换弹入口定位接线：私有 Playground 在已认证武器和自有置后静音测试场读取 `WarWeapon.GetActiveReloadValues`／`DetermineActiveReloadSuccessOrFailure` 的 UFunction 入口，验证当前游戏映像可执行页后仅记录 RVA、native 索引及参数大小；不调用或修改装填函数，也不依赖实体手柄。装备意图候选和默认禁用构建通过；此时尚无零售版入口地址或完美期限运行证据。模型方面继续按已核验的骨骼／独立弹匣组件处理，不将检查用拆件 OBJ 直接覆盖原游戏资源。

枪上弹匣控制器观察增量：只在私有认证手部／装备测试中沿原版 `FindSkelControl(AmmoControl)` 查询当前枪模，只读记录 `ControlStrength`、`StrengthTarget`、`BoneScale` 与是否存在。`Engine.u` 中该函数的参数和返回类型分别为 `NameProperty`／`ObjectProperty`，调用前还按运行时反射复核。私有装备意图和默认禁用构建通过；尚无零售版有效运行样本，控制器数值也不能代替实际画面可见性。

模型原件观察增量：在只限私有认证手部／装备测试的游戏线程回调里，加入对原 `MagazineMesh` 的只读检查，记录当前武器是否有独立弹匣组件、其静态模型名、显隐与是否挂在角色骨架上。它与已有原枪组件姿态观察配对，用于避免未来实体换弹时同时显示枪上和左手两份弹匣。私有装备意图、枪模跟手及默认禁用构建通过；没有新的零售运行样本，不能据此认定换弹视觉交接完成。

Snub 静态拆件复核：在已验证的原包上用同一刚性骨骼提取器分出滑套 445 面、左右翼片各 60 面，三个目标各自与剩余枪体恰好覆盖原 LOD0 的 3,045 面；UV0／法线与来源哈希记录在忽略的 `analysis/weapon-partitions/snub-b_HG_*/`。这补齐原先只拆出 Snub 弹匣的检查预览，不是游戏内可抓取、动画驱动或发行资源。

Boltok 静态拆件增量：哈希匹配原包后对 `Object01`–`Object11` 每根骨骼导出活动件／其余枪体配对预览，保持 UV0、法线和原材质槽；11 件共 2,417 面、主体 4,325 面，合计原 LOD0 的 6,742 面。检查件留在忽略的 `analysis/weapon-partitions/boltok-verified-Object*/`，不作为可发布或已接入的游戏模型。

Boltok 模型／动画绑定复核：SHA256 核验的原版 `Weap_LocustPistol.WeaponMesh` 显式引用 `Geist_Pistol.Boltock_Pistol` 网格与 `COG_Pistol.COG_Pistol` 动画集；前者 12 根通用命名骨骼，后者 `HG_Fire`／`AR_Reload`／`AR_Reload_Fail` 只有五根 `b_HG_*` 轨道且无同名项。暂不把 Snub 活动件方案移植到 Boltok；先查运行时实际动画和可见部件，必要时逐件控制或做覆盖件。包内轨道回执位于忽略的 `analysis/boltok-pistol-animation-audit-20261022.json`，并非运行时验证。

完美窗口静态复核增量：新增只读、哈希约束的原版默认对象字段审计，重新确认 WarWeapon 基类 `AR_SweetSpotDuration`／`AR_SuperSweetSpotDuration`／`AR_PreReactionWindowDuration` 为 `-1`，Lancer、Hammerburst、Gnasher、Longshot、Snub、Boltok 具体类未显式覆盖这三项；已有 `AR_PossibleSuccessStartPoint` 和 `ReloadDuration` 数值重现。结果位于忽略的 `analysis/active-reload-explicit-defaults-20261022.json`。这仍未确定原版 SuperSuccess 窗口终点，不给逐枪体感期限填猜测数值。

继续按保存的原版 HUD 脚本核对出显示层的完美区终点候选为 `ActiveReloadStartTime + SuperSweetSpotDuration`（均为 `GetActiveReloadValues` 的实际输出），普通成功区另加 `SweetSpotDuration`；历史真实 SuperSuccess 样本仍见对象上的三项时长为 `-1`。因此下一步只分析两个原生函数的有界 getter／判定路径，或在受控零售会话只读截取其真实输出，不用显示层公式冒充原生严格边界，也不主动调用未知副作用的原生函数来声称只读。

完美窗口原生定位增量：先查 `knowledge.sqlite` 后在私有 IDA 库中补入并 SHA256 核验历史运行时的代码与五个数据节，确认 `GetActiveReloadValues`／`DetermineActiveReloadSuccessOrFailure` 名称字面量分别位于 `0x14174ed60`／`0x14174edb0`。同 RVA 的零售磁盘 EXE 是 Themida 保护字节，不能与历史运行时内容等同。当前字面量没有直接 IDA xref、绝对指针或指向精确字符串的 RIP 相对 LEA 命中，尚未定位两个原生函数体或证明完美截止值。私有报告 `analysis/ida-reload-window-20261022/lookup.json` 保存来源与失败边界；IDA 租约已关闭。

从 `codex/0.1.0` 提交 `315473dca5008e753011ae905783079de6f04770` 创建独立工作树与 `codex/weapon-6dof` 分支；原工作目录仍由其他会话用于 0.1.0 开发。仅将本次[武器 6DoF 交互设计与验收规格](../../weapon-6dof-interaction-design-20261022.md)及相关文档索引迁入新分支，未复制原目录中其他会话尚未提交的 native、Playground 或启动器改动。后续完成功能并验证时，以届时最新默认版本为集成基线。

规格记录自由双手、身体槽位取枪取弹、R3 物理装填、逐枪黄色引导、霰弹数量不足与射后循环、枪口弹道及原版后坐力、武器弹药卡片、手雷手势投掷、动画接管、臂展校准、可配置转向与普通行走去惯性。用户确认的完美规则为原版该武器完美窗口结束前完成全部动作；提前完成也算，超时为普通装填且不加卡壳惩罚。

分支建立时只迁移需求文档。随后开始实现：OpenXR 探针增加显式 `--controller-input` 路径，在同一会话内读取双手 grip／aim 姿态、扳机、握持、摇杆及 ABXY，并在立体图像发布前写入独立的 v3 手柄共享帧。原立体帧 v4 格式没有变动。采集端为按钮、扳机和握持键保留按下／松开累计次数，握持按下还保留当时的手部位置，避免 Native 帧率较低漏掉短按或用延迟后的手位判错身体槽。失焦、交互配置或 LOCAL 空间代次变化会换代，重获焦点的首帧只做基线。Native 侧仅在 `controller_input_observer=1` 时按会话、序号、LOCAL 空间代次和预测时间读取同一帧；原有认证 PlayerTick 所在构建还可只读冻结最近有效输入，识别双手肩部／裤兜、R3、L3 和扳机意图，记录首次基线、丢失、过期与世界代次拒绝。当前没有向游戏注入手柄动作。

纯逻辑模块已加入两秒双握持稳定臂展测量、按左右手区分的肩部／裤兜槽位命中，以及装填弹药事务：弹匣和逐发装填、霰弹一轮最多三发且库存不足缩量、取弹保留与松手归还、取消时守恒、未完成机构阻止射击、射后循环、原版完美窗口**终点前**完成的严格判定及超时普通结果。这里的槽位半径和站姿偏移是待头显调整的初值；装填模块尚未接原版库存、动画、计时器、奖励或音效，不能据单元测试称实际游戏已经完成体感换弹。

验证：私有 Release 构建 `OpenXRPoseProbe`、默认和已启用原有认证 Tick／嵌套调用观察器的 `GearsCapture` 均成功；手柄共享帧与按键边缘、身体槽位意图、臂展／槽位、装填事务、既有第一人称视图和当前武器身份用例通过。编译沿用现有 Windows SDK 的警告，未启动游戏或模拟器，未更换默认启动器、DLL、XR 可执行文件或发布清单。下一步在原版认证游戏线程上关联真实开火与武器身份，再接实际手模、枪模、库存和逐枪机械动作；各枪完美窗口数值、弹药模型与枪口最终发射线仍待验证。

后续独立分支增量：私有连续会话增加 `--controller-input-observer`，只有认证的 UI／anchor／crosshair 观察器齐备才同时给 XR 生产者和 Native 消费者开启只读手柄帧；普通启动仍不启用。有效双握持两秒臂展测量可在会话完整、原生入口恢复确认后写入本机 `%LOCALAPPDATA%/GearsVR/weapon-calibration.json`，下一次私有会话复用；`--remeasure-armspan` 忽略旧值进行本次重测，失败不覆盖原记录。后台置后测试和已知模拟输入不写入该记录。校准引导界面和启动器开关尚未加入，不能称首次校准体验已完成。玩家补充 Gnasher 是左手稳握前端、右手下掰握把并回位的杠杆式霰弹枪，射后高亮应是握把／杠杆而非泵动前护木；纯换弹模型现在要求与武器匹配的机构动作，真实双手轨迹判定仍待接入。Longshot 需要镜片内独立远景渲染，单纯放大画面不可作为完成；最初 14 种武器／手雷清单后经玩家更正为 12 种，Mulcher／Ink 不纳入开发。此增量仅完成私有命令与离线逻辑，未在游戏／头显验收。

离线复查：Native 默认及认证 Tick 私有 DLL 构建、换弹模型、校准格式与私有命令接线通过；连续会话 60 项测试及便携文件清单 21 项测试通过。`docs/weapon-aim-hit-chain.md` 和 RE 知识库再次核实 `InstantFire`／`GetAdjustedAimFor` 是脚本链，现有共享 VM 入口已有 Hook，不能把函数名当独立机器码入口直接叠加新 Detour；真正开火链仍缺一次原版实际调用轨迹，下一步先在原 Hook 所有者下只读关联开火范围、返回 ABI 和当前枪身份，再改枪口方向与起点。

Gnasher 杠杆手势增量：新增纯逻辑的双手行程识别器，要求左手持续抓住前端、右手握把有效追踪，在指定武器／世界／追踪空间代次下完成原位→下掰→回位；一帧跳角、采样中断、松手和回中会重置，成功后才把 `lever_down_return` 事件交给换弹／射后开火门。该角度必须由未来经原版动画核对的杠杆枢轴计算，当前尚无游戏枪模几何接入，阈值也未按头显调定。独立测试构建与轨迹／射后门控用例通过。

枪姿态增量：加入只消费同一 XR 双目 ticket 的双手枪体求解器。右手握把决定枪体锚点；左手确实握住副握点且追踪与臂距合理时参与方向修正；枪口位置及轴线由逐枪局部挂点计算，并能用冻结的回中参考转换为游戏世界射线。纯逻辑测试覆盖单／双手、仅头动不改变枪线、错帧和失去追踪后的拒绝或单手回退。当前没有已核对的逐枪握点／枪口局部坐标，也未写入游戏枪模或开火函数；测试只证明数学与数据门控，不证明实际弹道改变。

模型拆件核验增量：原版六件已定位的武器 `SkeletalMesh` 通过包内哈希和骨骼层级审核；Gnasher 确有独立命名的杠杆骨骼，Lancer／Hammerburst 的弹匣、Longshot 的枪栓／镜体、Snub 的滑套／弹匣也有相应骨骼。查看用 GLB 丢失蒙皮，不能由其外观看出原版部件是否独立。新增可复现的只读审核脚本，报告放在忽略的 `analysis/weapon-bone-audit.json`。动画行程和游戏内控制未验证；Boltok 骨骼命名不具部件语义，其余武器仍待逐件调查。

武器几何增量：只读审核从六件原始模型中提取参考骨架的父骨和局部姿态，Gnasher 杠杆的父骨确认为枪身。新增左手稳住枪身、右手沿杠杆圆弧下掰并回位的几何适配器，合成姿态和顺序识别测试通过。枢轴方向、右手握点和行程尚需原动画标定，仍未接入原版射击或奖励，不能视为可玩功能。

装填几何增量：增加弹药件从装填口外沿指定轴向连续插入的判定及装填口／枪栓／杠杆单步引导状态；合成姿态验证了对准、行程、瞬移、误触和回中失效。原模型局部坐标、高亮渲染及实际弹药事务仍未接入。

模型覆盖增量：在已定位的另外四件地图包网格及 Frag／Smoke 手雷上复用来源哈希、骨骼参考姿态与 LOD0 蒙皮审核。清单中已有 12 件模型获静态核验；其中 10 件可刚性分出检查用部件，Lancer 的 36 个混合权重顶点和 Torque Bow 的 48 个跨骨骼三角形阻止**整件网格**无缝逐骨拆分。后续逐区段复核确认 Lancer 的混合权重只在电锯模糊特效，不在弹匣。Boomshot 弹匣、Hammer of Dawn 扳机、Troika 多个活动部件及 Bolo 手雷分节链条已有原骨骼证据。Mulcher、Ink 的网格与所有武器的动画行程／游戏内操控仍待核验。

零售包身份增量：只读解压购买版工作副本的 `WarfareGame.u` 与 `MP_WarMachine.war` 后，两个解压包的完整 SHA256 均与既有模型分析来源逐字节一致。临时解压文件已清除；这是静态包核验，不是运行或头显验收。

原版 R2 输入实测：独立、静音、窗口置后的零售版测试场中，Hammerburst 仅一次 `WarPC.ButtonPress('R2')` 就建立原版持续开火状态，约 1.2 秒弹药消耗 15 发；仅一次对应 Release 即清除控制器开火／按住状态。此前诊断脚本为机枪显式双调 Pre+ButtonPress，不能当作最小调用证明；本轮独立证据支持后续 VR 扳机采用单次 ButtonPress/Release 语义。测试停止后独立游戏进程退出，菜单桥接 Shutdown 返回 0，游戏音频恢复。其他武器、追踪丢失和输入来源去重仍待实现与验证。

模型蒙皮增量：继续解析经哈希验证的 LOD0 活动骨骼表及 64 字节顶点权重，发现 Gnasher 杠杆对应 1,192 顶点、1,260 三角形，与主体三角形无交叉。已用保守工具把 Gnasher、Hammerburst、Longshot、Snub、Boltok 的可刚性分组灰模导出到忽略目录 `analysis/weapon-partitions/`，并逐件核对三角形总数和输出哈希。Lancer 整件有 36 个混合权重顶点，保守工具拒绝整件刚性拆分；后续已查明这些顶点属于电锯模糊特效，弹匣本身可刚性分出。Longshot 镜体／镜盖骨骼不在当前 LOD0 活动骨骼表内。导出仅有几何，不含贴图、骨骼动画或运行时替换；实际动画与 VR 操作仍待验证。

开火链只读观察增量：为现有脚本身份凭证增加可选的六函数序列（`WarWeapon`／`Engine.Weapon.InstantFire`、`WarPawn.GetAdjustedAimFor`、`WarWeapon.GetAdjustedAim`、`Engine.Weapon.CalcWeaponFire`、`WarWeapon.ProcessInstantHit`）。仅在私有手柄观察会话申请；原始角色、包、脚本地址／大小与入口每次由当前进程核验。复用已有 VM 调用边界，不设第二个 detour，不调用武器函数，也不读取结果缓冲或参数。普通 `WarPC.PlayerTick` 的同步调用栈中，记录原始调用前后元数据；子调用必须有实际 `Engine.Weapon.InstantFire` 函数体祖先，才标作射击体内观察。完整 Tick 结束后再次验证身份和世界代次才提交有限行数；世界重绑定也申请同样凭证。纯内存测试通过；尚无实际游戏开火轨迹，因此不证明该 Tick 覆盖连发计时器，也未取得射线数值、更改弹道或接通枪口。

开火方向只读采样续作：现有共享 VM Hook 的同一次同步返回中，只有经当前进程 `ReturnValue` 结构身份、12 字节大小、`0x580` 标志、`Rotator` 目标与准确调用链重新认证的 `WarPawn.GetAdjustedAimFor`／`WarWeapon.GetAdjustedAim`，才读取原入口 R8 所指的 12 字节并记录三个原版 Rotator 整数；RAX 保持原样透传。为两次调用加直接父帧及 `parent_aim_id` 限制，不能把 HUD 查询或另一次射击混入。Python 26 项配置测试、C++ INI 解析变异测试、私有观察器和默认关闭构建通过。未实测购买版返回值；`CalcWeaponFire` 射线参数、碰撞和实际枪口弹道仍未读取／修改。

首发射线输入只读采样：继续在同一个共享 VM Hook 中认证 `Engine.Weapon.InstantFire` 的原始 `CalcWeaponFire` 调用点（字节码 `0x95` 起）、`StartTrace`／`EndTrace`／`PushedForwardStartTrace` 的 12 字节 `Vector` 结构和局部区边界。仅在 role4 的直接调用者执行帧代码指针是 `script+0x9e`、当前枪与父帧匹配且前后帧／局部区指针稳定时，复制这三个原版输入向量；`ImpactList` 只核对令牌身份而不读取，递归穿透不作为首发射线。变异测试拒绝令牌、字段大小、局部区、父帧、非有限坐标；私有构建与解析单测通过。尚未取得购买版开火样本，实际碰撞／伤害与 6DoF 枪口一致性仍未验证。

实验性游戏线程 R2 接入：独立的 `OriginalR2Fire` 状态机只消费精确匹配的 XR 帧与累计边沿，核对原版 PC／Pawn／当前枪身份；处理漏帧内的短按、已有游戏按键占用、追踪失效和 100 ms 过期。只有已实测持续开火的 Hammerburst 被私有 `Playground` 候选允许调用原版 `ButtonPress/Release('R2')`，Torque Bow 等松开可能发射的武器仍禁止。候选入口需要显式启用，默认构建编译选项为关闭。最终候选的隔离购买版测试场以模拟来源完整经过共享帧、游戏线程及原版开火链：按住约 1.2 秒使用 15 发，55 ms 输入空档未误松开，正常 Release 清空 `bFire`／`bHoldingButton_R2`；再次按住后终止发布端，过期恢复清空两项且弹药停在 24 发。`analysis/playground-20261021/vr-fire-bridge-1022f/vr-fire-bridge-receipt.json` 标记通过，游戏、工作者退出和八个音频会话恢复已核实。中间一轮因测试控制文件共享错误在开火前停止，另一轮测试发布器把单次共享锁竞争当作永久故障提前退出；均完成桥接停机和进程清理，修复测试发布器后最终回执通过。离线状态机及候选／默认两种 DLL 构建通过；这尚非真机、全部武器、枪口弹道或默认启动器验收。

原版 L2 与候选接入：单独零售版实测 `WarPC.ButtonPress/Release('L2')` 在 `bToggleTargeting=false` 的 Hammerburst 上建立／退出 `bPressingTargetingKey` 与 `bIsTargeting`，不需要额外调用 Pre；这一样本的 `FOVAngle` 未变化，不代表真实狙击镜已处理。隔离回执 `analysis/playground-20261021/l2-semantic-1022a/l2-button-receipt.json` 通过，游戏退出、桥接 Shutdown、八个音频会话恢复已核实。新增可选 `GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2`，在现有候选共享帧上处理左扳机边沿，原版切换开镜设置会被阻止；短按、共用输入状态、追踪丢失和身份改变的纯逻辑测试通过，默认 DLL 无相关导出。合并候选的后台联测在菜单视口尚未出现时超时，未投送模拟输入，故没有游戏内 L2 桥接通过结论；`analysis/playground-20261021/vr-l2-bridge-1022a/` 记录关闭与音频守护，独立游戏进程随后按身份清理。Gnasher 的两次 `SG_Chamber` 运行骨骼采样也未到达菜单入口，仍需在正常启动后读取主骨／杠杆矩阵；此时静态分件证据仍有效，但没有动画行程或逐实例骨骼控制证据。

Gnasher／Longshot 原版动画静态核验：新增 `tools/weapon-models/audit_animation_tracks.py`，用源包与导出各自的 SHA256 检查有界读取 `AnimSet` 和 `AnimSequence`，选定一根骨骼才解码采样，衍生结果留在忽略的 `analysis/`。购买版身份已核对的 `WarfareGame.u` 中，Gnasher `SG_Chamber` 的杠杆骨骼有 18 个旋转键，枪身仅 1 个；压缩键的杠杆父局部峰值约 32.40°，先转到约 24.02°、再回位。`SG_Reload`／失败／成功序列另有 45／60／27 个杠杆旋转键。Longshot `LS_Reload`／失败序列的枪栓各有 50／160 个旋转键，镜体和镜盖各只有 1 个。Python 语法检查与两枪完整轨道审核通过，Gnasher 18 键解码曲线复核通过。动画存储四元数还需转换到网格／世界坐标，游戏播放速率与完美窗口也须另证；零售游戏启动未进入菜单视口，因此本轮没有新增运行时骨骼或头显验收。此时无需真的裁切原版 Gnasher 枪模；优先复用独立蒙皮骨骼。Lancer 混合权重位置已在后续复核中更正为电锯模糊特效，而非弹匣交界。

扩展动画审核：允许某些序列省略压缩格式属性时只读取明确写入的轨道偏移／键数，不推断未写的默认格式，也不做该轨道的四元数采样。Lancer、Hammerburst、Snub、Bolo 四组 AnimSet 的完整有界审计通过。Lancer 原版装填同时驱动弹匣、机匣、卡扣；Hammerburst 装填活动锁扣而弹匣骨骼常量；Snub 开火时滑套有 6 个平移键；Bolo 准备动作驱动多节链条。相应哈希与计数回执位于忽略的 `analysis/{lancer,hammerburst,snub,bolo}-animation-tracks.json`。这进一步限定了哪些部件应复用原动画／蒙皮、哪些独立手持弹药需另建交互，但不构成游戏内操作通过。

地图包动画审核：已与购买版逐字节比对的 `MP_WarMachine.war` 另有 Boomshot、Torque Bow、Troika 的 AnimSet。工具在该源包哈希下完整读取三组轨道；Boomshot 装填枪管有 86 个平移键、弹匣骨骼为常量，Torque Bow 张弓在八根弓臂骨骼上各有 54 个旋转键，Troika 双旋转件各有 10 个旋转键。结果在忽略的 `analysis/{boomshot,torquebow,troika}-animation-tracks.json`；Torque Bow 的跨骨三角形继续使用蒙皮，不能通过逐骨刚性切割代替。

Gnasher 实例骨骼入口复核：Astra Max 只读核对已有脚本／对象证据，确认默认 `WeaponMesh.Animations` 直接为 `AnimNodeSequence`；`WarWeapon.SetWeaponAnim`、`FreezeWeaponAnim`、`SetWeaponAnimPosition` 为先验证原 `SG_Chamber` 姿态映射的最小原脚本路径。它接管整条序列且 `SetPosition(..., false)` 不触发通知，不能冒充单骨覆盖或完整音效流程。真正叠加单骨的 `AnimTree`＋`SkelControlSingleBone` 仅有声明和原脚本兼容路径，实例创建、`bIgnoreControllers`、GC 与更新顺序尚未验证；现有 Playground 请求在原 Tick 前执行，不是已证实的动画后修改点。下一次能稳定进入菜单的隔离零售版测试，先读当前枪实例与原序列主骨／杠杆相对矩阵，再决定最小控制候选。

模型运动测试入口已落地为 `tools/playground/gnasher_sequence_probe.py`：使用已拥有的置后、静音私有购买版进程，通过原版拾取／装备与 `SetWeaponAnim`、`FreezeWeaponAnim`、`SetWeaponAnimPosition` 在 35 个位置取主骨／杠杆矩阵，运行时按骨名校验索引，停用 Playground 后通知测试启动器收尾；失败回执与衍生骨骼矩阵都只写忽略的 `analysis/`。脚本语法和命令入口通过，**尚无新的游戏样本**；此前两次菜单未就绪的结果仍不构成动画控制证据。Gnasher 本体不需预先切割，真正交互接线还要先拿到有效实例矩阵与手柄方向映射。

该探针两次隔离购买版运行的收尾结果已核对：首次在原版 UE3 名称／对象表可用前建桥失败，因此只对此类瞬时表头错误增加最长 90 秒重试；第二次建桥成功但 `PlaygroundEnable` 等待菜单视口 60 秒超时，未拾取 Gnasher、未取骨骼矩阵，回执 `analysis/gnasher-sequence-runtime-1022d.json` 的样本数为 0。`PlaygroundShutdown=0`，被拥有的游戏进程已退出，音频守护 `restored=true`；不再无条件重复同一开局。运行时动画行程与手柄驱动仍无有效样本。

Gnasher 完整霰弹模型原型：已查原版资源只有抛出的开口空壳，故新增纯程序化 `assets/weapon-6dof/gnasher-round.glb`，封口、底座、底火及四色材质均为新几何／材质，参考尺寸 33.7×113.5 mm，544 个三角形。`tools/weapon-models/build_gnasher_round.py --check` 逐字节复现提交的 GLB，结构与尺寸检查、本地侧面／斜视预览通过。该原型**尚未加载进游戏**，装填口配合、抓点、碰撞、纹理和头显尺度仍待验证；原空壳网格／检查图仍留在忽略的 `analysis/`。

启动器校准入口增量：`launcher.py start --remeasure-armspan` 现在只在清单启用已认证的 controller／crosshair／UI／anchor 观察器时传给连续会话。双语 WinForms 界面在该清单下显示臂展行：无有效本机记录时首次测量必选且不可取消，有记录时默认复用并可选重测；测量仍由已有 OpenXR 双握持两秒逻辑执行，失败不覆盖旧值。普通冻结清单未启用该能力，界面不会误导玩家以为当前默认版已有 6DoF。启动器 Python 78 项测试、连续会话及校准 62 项测试、C# 编译与隐藏自检通过；私有 GUI 候选留在忽略的 `analysis/vr-launcher-ui/`，未安装到玩家默认入口。头显内进度／槽位确认与真机测量待完成。

完美换弹期限复核：按项目规则先查逆向知识库与已保存的 UnrealScript 类默认值，Astra Max 只读复查了七把已定位武器。Lancer／Hammerburst 的 `AR_PossibleSuccessStartPoint` 默认 0.70 秒、`ReloadDuration` 2.50 秒；Gnasher 为 0.85／3.00，Longshot 为 1.00／3.00，Snub／Boltok 为 0.50／2.00；Boomshot 这两个值未取得。现有保存证据没有这些枪的**完美窗口终点**，开始可能点和普通时长均不得代用。体感换弹的逐枪截止值仍未配置，原版奖励／音效接入仍待完成；详见交互设计中的计时表。

本轮离线验证还覆盖 `OriginalR2FireTest`、`OriginalL2AimTest`，并成功构建默认、仅 R2、R2+L2 三种私有 Playground DLL；导出表分别为无 VR 入口、仅 R2 入口和额外的 L2 状态入口。模拟发布器增加左扳机及双扳机测试动作，仍只用于置后静音测试，不进入默认启动清单。

L3 回中接线增量：认证的游戏 Tick 仅将已接受的 L3 边沿写入一次性请求，包含当前世界、XR 会话和 LOCAL 空间代次。渲染线程先验证实际冻结的立体帧，再把匹配的请求应用到原有 `StereoGate` 回中参考；首帧已以当前头姿建参考时不重复回中，拒绝帧、换世界／会话／空间或重放均不能消费为有效动作。私有认证 Tick Native DLL 构建、立体参考测试和新增请求边界测试通过；普通冻结清单及玩家默认 DLL 未更换，真实手柄、游戏内身体槽与枪体同步效果仍待验收。

Lancer 弹匣拆件复核：原先分析工具错误地把整件网格的活动骨骼表用作每个区段的顶点骨骼映射，由此得出的“10 个弹匣接缝三角形、218 面弹匣侧”结论**已撤回**。经原包哈希、LOD0 区段索引、各 chunk 的 `BoneMap` 及 CPU／GPU 顶点逐项核对，13,397 个原始三角形中真正归属弹匣骨的是 156 面，另外 13,241 面归属枪身／其他部件；弹匣边界无需权重裁切。更正后检查用 OBJ 与报告在忽略的 `analysis/weapon-partitions/Lancer-corrected/`。这仍不是可直接游戏装卸的模型，原包和默认版本未改。

Lancer 拆件错误来源已定位：先前 62 面的 `X≈0` 片状块位于材质 `BlurringBlades_Delta_MAT` 的电锯模糊区段，不能归入弹匣。正确弹匣是 120 个刚性顶点／156 面的单一连通块；整枪其余 36 个混合顶点、40 个关联面和 8 个跨骨面都在电锯特效区段。审核器与两个拆件工具现按 chunk 自身的 `BoneMap` 取骨骼，并逐顶点校验源 CPU／GPU 几何和影响；12 件已定位网格重新审核通过，只有 Lancer 的旧骨骼归属计数变化。旧错误派生预览已移至 `analysis/weapon-partitions/Lancer-weighted-invalid-20261022/` 并标记无效，新工具 5 项单测通过。要做可装卸件，仍需原 UV／材质、碰撞和运行时动画／显示核对；已有 `MagazineMesh` 可优先复用。

刚性部件首次导出增量：`tools/weapon-models/extract_rigid_bone_lod0.py` 仅接受目标骨骼所有有效顶点权重为 255 且目标三角形不跨界的模型，记录源材质槽、源哈希与派生 OBJ 哈希。已离线导出 Lancer 弹匣 120 顶点／156 面、Gnasher 杠杆 1,192／1,260、Hammerburst 弹匣 271／272、Snub 弹匣 279／314、Longshot 枪栓 687／894、Boomshot 弹匣 700／1,236、Hammer of Dawn 扳机 164／156、Troika 两个旋转件各 488／600；Lancer 弹匣在两角度灰模预览中位于枪身下方。跨 chunk 相同局部骨骼索引、无关部位混合权重、目标自身混合权重和跨界面均有合成单测，7 项通过。当时声称“保留源主 UV”**已撤回**：首次导出读取了错误偏移，几何归属和面数有效，旧 OBJ 的贴图坐标无效；更正如下。

刚性部件 UV／法线更正：购买版包内三件不同骨骼部件的面法线核对及 [UEViewer 的 GoWU 专用 64 字节顶点读取分支](https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealMesh/UnMesh3.cpp#L3743-L3778) 均指向此 GPU 顶点的两组打包方向位于偏移 12、16，UV0 始于偏移 20；旧工具从 24 读取两个浮点，实际是 UV0.v 与 UV1.u。工具现按 20–27 读取 UV0、按 16–19 解码法线并反转 OBJ 面序。九个刚性部件重新导出到忽略的 `analysis/weapon-partitions/uv-normal-fixed-20261022/`，源哈希、顶点与三角形数量逐件保持一致，导出面与法线平均点积为 0.698–0.984；旧 `*-uv/` 目录有失效说明，不能用于贴图。提取工具的 8 项合成检查通过；新的元数据保存原骨骼父级、原始父局部姿态及逐级组合的静态参考枢轴候选。九件候选距各自几何包围盒中心均小于对角线的 0.43 倍，但原版运行时骨矩阵、铰链轴和手柄行程仍未证实。新 OBJ 仍不含原材质贴图、蒙皮、碰撞或游戏内显隐控制，不能直接装入 Mod 或公开包。

成对模型拆分增量：同一来源校验工具现在同时导出活动骨骼件与不含该件的枪体剩余部分，保留源 UV0、法线及材质槽索引，要求目标骨骼无混合权重和跨界三角形；剩余部分可含原版混合蒙皮，OBJ 仅作静态检查。九组新导出在忽略的 `analysis/weapon-partitions/paired-20261022/`，逐组核对两份 OBJ 的面数、哈希和合计原始 LOD0 面数，均无少面或重复面；例如 Gnasher 杠杆 1,260＋枪体 5,060＝原网格 6,320 面，Lancer 弹匣 156＋其余 13,241＝13,397 面。提取工具的目标／剩余面覆盖与拒绝跨界单测通过；`paired-preview.png` 的侧面和斜面预览也显示杠杆与弹匣落在正确局部。原版脚本另确认 Lancer、Hammerburst、Boomshot 的装填通知使用 `AmmoControl` 隐藏／显示枪上弹匣，`SetPawnAmmoAttachment` 可把独立 `MagazineMesh` 接到左手，Lancer／Boomshot 弹出时还有物理弹匣；VR 运行时应先接管这些现成对象和显隐，避免复制弹匣。当前只有离线几何和脚本证据，手柄附着、原版通知时序、材质及实机渲染未验证。

原版库存槽位映射增量：经知识库索引与已保存脚本核对 `WarPC.WeaponSlotFromDpadDirection`、`WarPawn.SetSlotAttachment`，将 VR 左裤兜手枪、右裤兜手雷、左肩武器 2、右肩武器 1 分别映射到原版 0／1／2／3 号槽；取弹位置无库存武器槽。`BodySlotsTest` 编译和校准／位置／编号断言通过。镜像角色的视觉肩位会交换，且没有调用原版切枪；游戏内装备动作仍待单独验证。

取放枪所有权状态增量：新增 `WeaponEquipmentModel`，将右手 RGrip 槽位点按与原版库存快照绑定。空槽只返回一次反馈请求；非空槽先请求原版切枪，待当前武器身份和动画结束同时确认才锁定持有；原槽再次点按请求收枪，RGrip 松开不改变持有。原版拾取动画期间双手由动画占用，结束后按实际槽位和当前武器交还；世界／角色切换、武器移除及原版拒绝切换会清除挂起状态。独立 MSVC `/W4 /WX` 用例通过，覆盖空槽、切枪等待、收枪、拾取交接与身份突变。此模块尚未接入游戏线程库存读取、原版切枪、枪模显隐或音效，不能当成游戏内取枪已完成。

取枪动画边界复核：Astra Max 仅用已保存的 UnrealScript 和现有 VM 观察文档确认，原版 `SetCurrentWeapon`／`ChangedWeapon`／`WeaponPuttingDown`／`Active` 负责复制、挂接及计时，不能跳过；身体取放枪姿态分别在 `TimeWeaponPutDown/TimeWeaponEquipping` 内调用。姿态函数还被手雷重装与其他动作复用；现有 CallFunction 入口的参数尚由 caller frame 在函数内求值，直接吞掉调用会错过参数消费。未新增危险的全局动画抑制或同步完成装备 Hook，下一步先做原游戏 caller／callee 边界与参数消费的只读验证。

库存只读候选增量：新增默认关闭的 `GEARS_PLAYGROUND_EQUIPMENT_OBSERVE`，只在本地 `WarPC.PlayerTick` 游戏线程上每 500 ms 调原版 `GetWeaponInSlot`、`CanSwitchWeapons` 与手雷 `HasAnyAmmo`，并核验 `WarPC`／`WarPawn`／`WarInventoryManager`、`CharacterSlot`、当前枪和 `bSwitchingWeapons`。手雷数量取 `SpareAmmoCount`，按原版人类玩家 `HasAnyAmmo` 判定是否可取；若原版无限备用弹药允许使用，则标记为无上限。之前考虑的 `GetTotalAmmoInGun` 包含当前弹仓，不适合手雷选择门槛，已弃用。查询或类型不符即丢弃快照；`WeaponEquipmentModel` 对未知手雷数量改为按空槽处理。独立状态单测、私有候选 DLL 和默认关闭的 DLL 均完成 MSVC Release 构建。尚未拿到实际购买版运行时快照，且只读候选不执行取枪、收枪、黄色提示或音效，默认启动器未改。

库存观察运行尝试：独立购买版测试子进程 PID `31700` 按置后、静音、禁后台物理手柄输入策略启动，私有候选 DLL `b607bb7e…16d35b` 注入后 `PlaygroundInitialize` 成功，但 120 秒内 `PlaygroundEnable` 一直找不到菜单视口，未进入场景，日志也没有有效 `equipment_snapshot`。`PlaygroundShutdown=0`、测试游戏退出、音频守护恢复，回执在忽略的 `analysis/weapon-equipment-snapshot/owned-probe-receipt.json`。这与先前 Gnasher 探针的菜单未就绪同类，不能推断库存查询成功或失败；在拿到启动原因／不同的启动路径之前不再按同样条件重复。默认 DLL、启动器及用户配置均未更换。

Gnasher 首发杠杆门槛补齐：`ReloadModel::prime` 可用同一原版库存快照的当前弹仓／备用数量，先把已经装备且装弹的枪纳入状态机；第一次射击后即要求右手握把下掰回位，未完成时 `can_fire` 为假。重复初始化不能清除待操作杠杆，切枪／换世界必须由适配器显式重置。完美窗口终点尚未从原版取得时，`prime` 仍能启用射后杠杆，但 `begin` 继续拒绝未知期限，避免假造完美换弹时间。`WeaponReloadModelTest` 与 `GnasherLeverTest` 重新编译运行通过。此逻辑尚未接到原版射击事件或游戏内禁射／骨骼驱动，不等于实枪已要求每发手动循环。

库存观察启动诊断：第二轮改用此前成功测试所用的捆绑 Python 3.12，仍在 120 秒内 277 次收到 `PlaygroundEnable=2`，未创建菜单视口；候选正常关闭、测试进程退出、音频守护恢复。第三轮改用完整私有 VR 启动器并在私有清单关闭默认 Playground，原生预留器 240 次轮询后明确记录缺少 `d3d12.dll`／`dxgi.dll`，`injection_attempted=false`，故候选甚至未到加载阶段。其关闭消息未让测试子进程退出；按启动记录 PID／创建时间／路径核对后只终止该测试 PID `12912`，确认已消失。两轮原始回执都在忽略的 `analysis/weapon-equipment-snapshot/` 与对应 `J:\gearsVr\analysis\instances\equip-snapshot-1022b/1022c/`；不能据此判断库存反射正确与否。需要先定位当前购买版启动停在图形模块之前的原因，或使用已证明可进入场景的不同运行条件；正式启动器及用户配置未改。

手雷抛掷数学原型：新增 `GrenadeThrowTracker`，用同一 XR 会话、空间／输入纪元及冻结回中参考下的最近 140 ms 手部轨迹作最小二乘初速度估计；短于 55 ms、静止、追踪失效、回中改变、跨世界／武器或瞬间跳位都不会产生投掷。出手后给出 3 kg 对应冲量、9.81 m/s² 抛物线及经同一参考帧转换的游戏世界位置／速度；质量不被错误地当作减速系数。握点到手雷质心的局部偏移现可逐帧随手旋转，速度拟合包含手腕甩动；世界速度还可叠加原版角色平移速度。`GrenadeThrowTest` 以 `/W4 /WX` 编译并通过轨迹、手腕旋转、角色速度、冲量、世界坐标及断续输入测试。实际各手雷握点偏移／角色速度来源仍待游戏适配器量测接入；它尚未替换原版投掷曲线、生成／扣除手雷、处理碰撞引信或加载手持模型，也没有真机手感验收。

启动记录只读对照：前次成功进入战斗场景的 `vr-fire-bridge-1022f` 与本次 `equip-snapshot-1022b` 使用同一购买版工作副本、同样的置后／静音启动策略、相同的 `-windowed -ResX=1280 -ResY=720 -NoMovieStartup -NoPauseUnfocus` 参数及相同私有配置内容；当前没有证据将菜单未就绪归因于这些参数。第三轮管理式启动 `1022c` 只带 `-NoPauseUnfocus`，是一个可解释的条件差异，但前两轮同参结果不同，仍须另找启动状态或时序证据；本轮未再次启动游戏。

入口返回码勘误：源码中 `PlaygroundEnable=2` 既可能是 `WarGameViewportClient` 未找到，也可能是 `PlayerController.PlayerTick` 函数未找到。此前将 `1022a/1022b` 的返回码直接写成“未创建菜单视口”超出了现有回执，应改读为**测试入口未就绪，原因未区分**。这不影响“没有取得有效库存或骨骼运行样本”的结论；下一次在确有不同启动条件时，应分别记录这两个寻址结果，再决定是否进一步诊断游戏启动。

私有库存入口细分验证：仅在 `GEARS_PLAYGROUND_EQUIPMENT_OBSERVE` 候选中新增只读 `PlaygroundEquipmentReadiness` 位码，分别观察 Hook、视口、`PlayerController` 类、`PlayerTick` 和 `WarLocalPlayer`；默认关闭的 DLL 不导出它。候选与默认 DLL 均重新编译，导出隔离核对通过。一次不同条件的购买版置后静音测试 `equip-snapshot-1022d` 将候选注入延后 20 秒，120 秒观察后仍仅得位码 `1`（Hook 已启用，其余四项均未出现）；`PlaygroundEnable=2` 计 278 次，没有有效库存快照。回执在忽略的 `analysis/weapon-equipment-snapshot/equip-snapshot-1022d-receipt.json`；`PlaygroundShutdown=0`，测试 PID `52392` 已退出，音频守护已恢复。延后注入未解决本次初始化未就绪，不能归咎于库存查询，也不能据此否定过去成功进入场景的独立样本。相同条件不再重跑；下次须先找到游戏初始化条件变化的证据。

库存切换私有候选：核对已保存的 `WarInventoryManager.SetWeaponFromSlot` 脚本，确认原版槽位选择由它处理无弹药反馈和 `SetCurrentWeapon`，而非直接改当前枪指针。新增默认关闭、必须同时启用库存观察的 `GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST`：外部只提交 0–3 号槽；游戏线程重新读取有效快照，仅限已创建的私有 Playground 场，才调用原版 `SetWeaponFromSlot`，并观察装备完成、身份中断或 5 秒超时。测试子进程还须带置后和静音标记。私有测试 DLL 和默认关闭版均编译通过，导出表证明测试入口仅在前者，`WeaponEquipmentModelTest` 通过。当前购买版启动尚未达到游戏线程库存边界，**此候选没有游戏内调用证据**；不接入默认启动器，不视为可玩的取枪实现。

右手抓取到原版槽位的私有接线：原生已认证游戏 Tick 中的 XR 输入路由现在可把右肩／左肩／左右裤兜四种抓握意图，经新的进程绑定、8 事件环形桥发布给 Playground。桥按序号消费并拒绝溢出、重放、超过 250 ms 的事件；Playground 在原版游戏线程再次比对 PC、Pawn、当前武器及四槽库存，仅在置后静音的私有场里调用已有 `SetWeaponFromSlot` 候选。`EquipmentIntentBridgeTest`、启用与关闭候选的 Playground 构建、启用与关闭认证 Tick 的 native 构建均通过；默认 Playground 导出表仍无测试入口。当前未取得购买版游戏中的有效库存 Tick，不把此接线视为真实 RGrip 切枪验收。原版切换动画、收枪显隐、空槽提示音和持枪 6DoF 姿态尚未完成；原版脚本的无弹药反馈只覆盖有武器对象的槽。

空槽音效私有候选再利用原枪音源：库存模型判空时，在同一有效游戏线程快照里优先取目标槽武器、其次取当前武器的 `FireNoAmmoSound`，用原版 `WeaponPlaySoundLocal` 播放；没有有效 `SoundCue` 则回报状态 9，不伪称已发声。候选编译通过，默认编译排除这条路径；由于购买版本轮未到有效游戏 Tick，**没有音频播放实测**。左肩无备用弹药以及完全无当前武器的通用提示音仍待接入。

同一库存快照现在逐个已占槽调用原版 `HasAnyAmmo`。枪对象仍在但已无任何弹药时立即判无货，不再进入无法完成的五秒切枪等待；先判断“把已持枪放回原位”，所以弹药见底仍可收枪。私有 Playground 候选构建、纯状态机测试均通过，尚无运行时库存查询／声音验收。

左肩取弹意图已接入同一认证事件桥。私有适配器在有效游戏线程快照里核对当前枪、角色和测试场，按原版 `SpareAmmoCount` 与 `HasInfiniteSpareAmmo` 给出可供给／无弹药反馈；无弹药时尝试原版空仓 Cue。状态接口区分可供给、Cue 已调用、无 Cue 和验证失败，**有弹药时仍未生成或扣除实体弹／弹匣**。更新后的事件桥测试、两侧私有候选编译及默认 Playground 导出隔离通过，购买版真实手柄与声音仍待验证。

双手连续世界姿态接线：native 从同一已认证 XR 双眼 ticket 取左右手抓握姿态，经冻结的回中参考和实际用于双眼生成的游戏相机视图转换到游戏世界，在 GVE3 进程绑定桥的独立最新姿态槽发布；游戏线程私有 Playground 重验 PC／Pawn／当前武器及测试场身份，拒绝过期姿态并限速记录。新增 `xr_pose_in_game_world` 的平移、水平转向、俯仰／侧倾与无效矩阵测试；`WeaponPoseSolverTest`、`EquipmentIntentBridgeTest`、native／Playground 候选、默认 Playground 构建及导出隔离均通过。未运行新的购买版游戏测试，枪模、枪口射线和动画还未受这条姿态控制；此前私有测试没有进入库存／视口边界，本轮静态通过不改变该结论。

原枪组件只读探针：私有 Playground 在同一身份已验明的手部姿态旁每 500 ms 记录 `Weapon.Mesh` 的组件／资源名、相对变换、`LocalToWorld` 原点和绝对变换／显隐标志，供下一次有效购买版场景确定枪模控制入口。候选编译通过；未取得游戏读数，也未移动枪模或替换模型资源。

Lancer 原枪模跟手私有候选：`WeaponMeshFollow` 从有效原 `LocalToWorld` 与右手世界姿态锁定一个首帧不跳位的临时偏移；仅在测试场已持有 Lancer 且 XR 姿态、PC／Pawn／武器／世界身份持续有效时，于原 `PlayerTick` 后调用游戏已有组件变换函数。候选保存原相对变换和绝对标志，断追、切枪、收枪和身份变化时尝试恢复，私有 Hook 关闭前必须已恢复。`WeaponMeshFollowTest` 通过平移、转动和不合格矩阵测试；启用、关闭及默认 Playground 构建通过，默认导出无该入口。没有新的有效购买版游戏运行样本，因此**不能称为已经可玩或后坐力已保留**；临时偏移仍须替换为逐枪握点校准。

双手输入接线增量：GVE4 私有桥在同一已认证 XR ticket 中加入 `input_epoch` 与两手 `grip_down_mask`；只给有效追踪的手设置按住位，失焦／配置换代后的基线不产生新的抓握。游戏线程日志可区分左手姿态与实际握持，Lancer 跟手候选在输入代次改变时恢复原组件。`EquipmentIntentBridgeTest` 覆盖左右手有效位、无效组合和换代数据，私有 native／Playground 与默认 Playground 构建通过。副握点尚无原版逐枪标定，双手持枪纠偏仍未启用；本轮没有新的购买版运行验收。

Lancer 副握点取证：已有 `AR_Fire` 枪械序列没有人物手轨，已盘点武器 socket 只有 `Muzzle`／`Magazine`；人物动画树的左手 IK 则指向 `b_MF_IK_Hand_L`。这些静态线索不足以给出副握点或吸附半径。私有 Playground 候选新增仅在显式请求后执行的游戏线程采样：原版 `PlayerTick` 返回后、枪模候选移动前，以同一世界／角色／原枪身份读取三根人物骨骼和前后不变的人物／枪组件矩阵，最多 24 帧；`lancer_support_probe.py` 读取日志并要求 24 行连续完整才签发回执。无写入原版骨骼／武器，输出仍在忽略的 `analysis/`。候选和默认 Playground 构建、两项日志解析测试通过；购买版测试尚未取得样本，渲染线程同步、握点与吸附半径仍未验证。

Lancer 弹匣 socket 同帧零售版验证：私有只读游戏线程探针确认原版 `Magazine` socket 位于 `b_AR_Magazine`，待机 16 帧的枪体局部中位点为 `(0.007997, 28.145977, -6.484180)` 原始单位，最大漂移 `0.000182`；与静态参考枢轴加原版 socket 偏移的预测点相差 `0.000078`。另一局原版换弹 169 帧中，socket 开始后约 `0.271 s` 跳离待机点，约 `0.738 s` 时远离超过 `120` 原始单位，结束后回位。因此 VR 弹匣井提示／插入判定不能跟随换弹中的活动 socket，需以枪体坐标冻结参考挂点。`tools/playground/vr_lancer_socket_probe.inl`、采集器与离线分析器可复核，原始回执仅在忽略的 `analysis/lancer-support-probe-20261022/`；两次测试的私有 Hook、游戏子进程和静音均已收尾。私有／默认 Playground 构建、3 项解析测试、离线摘要复算与差异检查通过。当前仍没有确认插入轴、手柄抓点或真实黄色渲染，正式默认 DLL 未加入该探针或实体换弹。

Lancer 弹匣挂点纠正：先前待机稳定的 `Magazine` socket 实际靠近弹匣下缘，不能当弹匣井中心。新购买版私有测试将 socket、`b_AR_Magazine` 骨矩阵与枪组件矩阵在同帧采集 16 次；骨枢轴局部中位点 `(0.007966, 28.146028, 4.396064)`、最大漂移 `0.000230`，距静态参考枢轴 `0.000026` 原始单位。枢轴到 socket 为 `10.880222` 单位、方向约局部 `−Z`；原模型弹匣 Z 范围 `[-6.950642, 4.325475]`，支持枢轴靠近上缘、socket 靠近下缘。插入提示应从枪体上缘附近的固定挂点开始标定，不能把下缘 socket 当目标。采样与复算回执留在忽略的 `analysis/lancer-support-probe-20261022/`；私有 Hook 停机码 0、入口还原、静音恢复、该局零售游戏子进程退出均已核对。手持弹匣相对左手的朝向、实际口面及容差仍未验证，未启用实体换弹或正式版黄色引导。

固定挂点接入私有 Lancer R3 保持适配器：新增枪体局部上缘参考点和外向轴到原枪组件世界坐标的投影，非法／镜像／剪切矩阵会被拒绝；每次保持期间重验 `COGAssaultRifle` 及同一网格身份。纯数学测试与私有、默认 Playground 构建通过。购买版测试场记录 7 个同一网格挂点姿态，原版换弹开始状态 1、明确完成状态 6，库存由原版一次性结算；私有回执 `analysis/lancer-support-probe-20261022/capture-49940-fixed-feed.json` 已通过有界解析。游戏、Hook 和静音均正常收尾，整局 VR 回执 `incomplete`，不能视为头显验收。尚未取得左手手持弹匣姿态、玩家插入行程或黄色渲染；正式版没有启用实体换弹。

独立弹匣的原版左手姿态取证：购买版私有换弹观察器在 `Assault_Rifle_Magazine` 实际挂到人物骨架期间，同帧读取它与 `b_MF_Hand_L`、枪体矩阵，得到 43 帧。连续 21 帧的中段近似刚性握持位于换弹后约 `0.889–1.206 s`，弹匣原点相对左手骨骼中位点 `(19.036035, -5.566177, 0.139646)` 原始单位，最大漂移 `0.000227`；方向矩阵漂移 `0.0000019`。整段挂接前后仍有明显相对动作，最大位置偏差约 `8.147` 单位，不能将原版送回动画作为玩家插入判定。`tools/weapon-models/analyze_lancer_magazine_grip_runtime.py` 和合成测试可复算稳定区及全段差异；原始矩阵和摘要只在忽略的 `analysis/lancer-support-probe-20261022/`。本次私有 Hook 停机码 0、入口还原、静音恢复，零售子进程退出；整局 VR 回执仍 `incomplete`。手柄掌心偏移、实体弹匣随手显示与玩家插入行程待实现。

无弹取枪规则校正：静态脚本证实 `WarInventoryManager.SetWeaponFromSlot` 仅在 `HasAnyAmmo || bCanSelectWithoutAmmo` 时请求原版切枪。私有游戏线程库存快照现读取逐槽 `bCanSelectWithoutAmmo`，纯取枪状态机在无弹但原版允许时仍请求切出，反之给空槽反馈；已持枪收回原槽仍不受无弹限制。增加合成库存分支测试及日志位，实际游戏内字段读取和切换待测。

对既存测试启动失败再做只读归因界限：`equip-snapshot-1022c` 的完整启动器回执显示购买版游戏进程已建立，但在有界等待后仍未加载 `d3d12.dll`／`dxgi.dll`，故启动器在注入前退出；它不是库存观察器、网格跟手候选或过期清单的运行失败。此前直接游戏启动样本 `1022d` 只证明 Hook 就绪而未找到视口／`PlayerTick`。本轮没有再次启动游戏；下一次测试须先确认启动条件有实质变化，并分别记录图形依赖、视口和游戏线程状态。

## 合并0.0.2启动修复（2026-10-22）

按用户要求切回 `codex/0.1.0`，将 `codex/0.0.2` 的完整修复与发布记录并入。合并前开发检查点为 `963d965b8590555bae31d047bef6ccc27d3e6469`，热修复源为 `a0a39b0`。公开0.0.1是独立根提交，两分支没有共同祖先；以公开源码 `4b621fc430fa5f96eba3bf70399810d3fd51e202` 为明确的内容基准进行三方合并，并保留两个原始父历史，不改写既有提交或发布标签。

已保留0.1.0第一人称、Playground、右摇杆死区和渲染源码／默认清单；便携模块白名单同时包含Playground与新增startup_diagnostics。接入0.0.2的UTF-8解释器探测、真实Steam子进程环境回执、有界启动分阶段日志、失败原因和Export logs摘要。根README与AGENTS更新为0.1.0开发上下文，公开版本的构建指南不覆盖本分支已有的删除决定。合并前本地配置仅有格式差异；其全部参数值保留，并恢复本分支的第一人称和死区选项。原始配置与合并回执保存在忽略的 `analysis/git-integration-0.0.2-to-0.1.0/`。

离线验证：88项debug-launch、204项vr-launcher检查通过；GUI编译与隐藏自检通过，涵盖第一人称、死区、配置保留、路径发现及Python探测。消除了内容合并重复引入的相同Steam环境函数和测试定义。新增Python文件语法、冲突标记、差异空白及源文件范围检查通过；默认native／XR／Playground文件SHA256与0.1.0原清单一致。

附加的13项预留生命周期／预启动回归中10项通过、3项旧夹具报错。用合并前 `963d965` 的启动预留源码回放同组测试，复现相同3项错误：两处mock返回值缺stdout／stderr，一处旧mock仍等待PowerShell `-Command`，随后解析了占位DLL。这些失败保留为既有测试限制；两个回放日志位于上述忽略目录。本次没有启动游戏或模拟器，不新增GPU／头显验收，不生成或发布新包。发布版0.0.1／0.0.2、热修复分支和既有本地二进制保留。

2026-10-21 分支保存：按用户要求，将当前第一人称、Playground、性能／恢复优化、正版迁移及启动修复等项目源码保存到 `codex/0.1.0`，随后从公开0.0.1基线分出0.0.2，只做启动兼容修复。此检查点不代表0.1.0已经发布或新增全量玩法验收。

解压0.0.1本机失败已定位到下载脚本的PowerShell执行策略拦截，并独立确认旧版多账号设置检查会拒绝。仅对子进程指定执行策略、保留两路错误输出／UTF-8，配合Steam环境和账号选择后，后台实际游戏到达AttractLauncher并提交启动菜单；正常退出、prime还原、音频及11个保护文件恢复通过。49项启动层检查和107项启动器／诊断检查通过；0.0.2版本与包的检查将在新分支继续。详见[本机复现记录](../../164-launcher-steam-install-feedback.md)。

## 工作副本直接启动入口（2026-10-21）

用户反馈直接双击工作副本 `Binaries_x64/GOWDE-Steam.exe` 仍会进入反作弊启动画面。已有正版启动证据确认，缺少 Steam App ID 上下文会请求 `steam://run/2523720`，转由 Steam 库中的 `Launch_GOWDE.exe` 启动。按 [Valve 官方 API 文档](https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary)，现仅在工作副本 EXE 同目录新增本地 `steam_appid.txt`，内容为 ASCII `2523720`，使直接运行时 Steam API 不再要求通过库中入口重启。Steam 仍需运行并登录拥有游戏的账号；正常 VR 启动器继续使用已有的子进程 App ID 环境。

已回读确认文件为7字节、无BOM且内容正确，位于忽略的游戏目录中；没有修改 EXE、Steam DLL、EAC 组件、Steam 库内原始安装或发布包。删除此新增文件即可撤销。本次遵守停止追加兼容性游戏运行测试的要求，未启动游戏；尚未实测本文件方案下的启动画面或耗时，也不声称卸载或全面关闭游戏内反作弊。

## 正版基线、启动兼容与存档迁移（2026-10-21）

按用户要求，今后开发和测试改用购买的 Steam 正版，默认目录为 `J:\gearsVr\Gears of War Reloaded`，Steam 库内原始安装保留。完整读取并计算三份安装的 SHA256：两份正版3,609文件完全一致；与历史旧目录共有的3,609文件中3,607一致，差异仅为原版／替换的 `steam_api64.dll` 及已有可恢复启动补丁 `Coalesced.ini`。核心EXE均为1.0.381.6947、26,278,928字节，SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`；2,518个CookedPC文件一致，现有核心逆向成果可继续使用。

首次正版启动复现 `Target priming failed (1)`，Steam日志确认原进程请求 `steam://run/2523720`，Steam另起库内启动包装程序，原预留PID退出。启动层已在子进程环境中传入本游戏 `SteamAppId`／`SteamGameId`，保留真实Steam DLL、登录及授权检查；Windows环境变量名按大小写不敏感去重，不改变父进程。修复后正版预留成功、13处入口字节核对通过，保持选定正版目录。首次启动生成第二套用户设置后，新增按当前Steam活动账号精确选择设置，避免多文件拒绝和改错账号。85项debug-launch及76项launcher离线检查通过。

用户随后明确停止继续兼容性运行测试，并要求迁移存档。所有自建游戏／XR进程已停止，owned prime、菜单组件和测试音频恢复通过；最后一轮在新闻欢迎页停止，未进入关卡、未激活双目native，不作正版GPU／头显验收。因关闭过程中次要窗口先消失产生WinError1400，单独复核原预留清理后完成正常进程退出及10个受保护文件恢复；保留原失败回执。

当前旧账号4个存档伴随文件按正版活动账号命名复制到共用的 `%LOCALAPPDATA%\Coalition\WarGame\SaveData`。原旧档和正版覆盖前副本均已备份；目标四文件逐一完整SHA256与源相同，主档和local档zlib校验及声明长度有效，未改序列化内容。按用户要求没有再启动游戏验证读取／继续章节。普通／DLSS清单、用户配置和开发工具默认路径已更新；native、XR、playground二进制保留原冻结哈希，历史实验清单、公开v0.0.1标签和已发布包不变。见[完整记录](../../retail-steam-compatibility-20261021.md)，本地回执在忽略目录 `analysis/retail-compatibility-20261021/`。

## happyotaku playground 测试场（2026-10-21，已接入默认）

按用户要求新增主菜单 `happyotaku playground` 入口，已向 Astra Max 咨询并采用包含现成 Troika 的 `MP_WarMachine`。11种枪械／投掷物拾取点、6个兽人静止靶和原有机枪生成通过，失败计数0；靶子死亡后三秒重生，枪库使用后补回。无限备用弹药保留正常弹匣消耗、装填及主动装填。

首次完整 VR 输出失败已定位并修复为 FXAA 最终输出调用的缺失分类：增加完整函数字节认证及现有 owned world／ticket／左右眼像素映射约束，不放宽通用绘制、HUD 或 GPU 提交检查。三轮完整启动器／模拟器流程正常停止通过，分别发布18,008、37,157、6,248帧；前两轮测试场实际射击击杀、换弹／备用弹药不减少和靶子重生通过，上下机枪通过，后两轮使用每眼1536×1632。GPU读回确认地图、枪械、机枪与靶子可见。普通战役回归没有生成测试场或启用测试场弹药维护。三轮 native 清理／入口还原、XR退出0、菜单桥接独立入口还原均通过。

启动器189项检查、受限输出几何正反例、原生启动检查及721文件的隔离便携目录依赖检查通过。当前默认 native 为 `build-playground-output1/GearsCapturePlayground1.dll`（SHA256 `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`），菜单组件为 `build-playground2/Release/GearsPlayground.dll`（`e1d07405d0b8c3393783d5de549c44d7e20647efcf5bcc2e82b0646e6cd038ee`）。仅更新当前默认清单的7个相关字段，基于原 `045ca20a…b9d` 冻结源码保留已集成修复，用户配置不变。失败回执继续保留，不作玩法／VR通过证据；0.0.1 标签和已发布包不变。实体头显、VR手柄与整体游玩体验仍由用户之后统一验收，本次不是新版本发布。见[实现与验证记录](../../happyotaku-playground-20261021.md)。

20:55 完成当前默认构建的补验：11种武器逐项原版拾取／库存和无限备用弹药通过；枪库补回16次，11拾取物／6靶／失败0。原版退出主菜单后，Down／Enter 再次进入新世界通过。按原版 R2 按住流程，Troika 经过0.5秒预热实际发射28发，松开停止、上下机枪通过；此前缺少控制器按住状态及脚本参数／名称扫描错误的诊断回执保留，不作为发射证据。合并检查再次通过，无需更换已集成构建。测试入口64字节还原、游戏音频恢复、独立游戏进程退出均确认，当前状态完成审计 `analysis/playground-20261021/completion-audit.json` 通过。

## 玩家启动失败修复（2026-10-21）

用户反馈普通入口进不了 VR。已从其 PID44044 日志定位：UI 组装的256条可选身份缓存满后直接拒绝配置，native 尚未激活。修复为缓存满后执行原有实时读取，保留最终96条证书身份限制及全部结束复核。新增超过256个扫描成员的回归用例，247项相关检查通过；标准后台正常启动完整复测通过，实际256条缓存满后发生1743次回退，约90秒8136次双目提交，native/XR/owner exit0、Hook 校验及音频/死区/存档/设置恢复均通过。保留第一人称、2%死区与现有恢复优化，见[故障与验证记录](../../vr-startup-ui-cache-regression-20261021.md)。

2026-10-21 玩家启动日志跟进：22 项自检及游戏创建成功，失败点为 `Target priming failed (1)`；排除前述 DLSS 保存设置检查拦截。核对 0.0.1 冻结源码后确认 1 是远程线程退出码，尚不能认定为某个 Hook 返回值或游戏崩溃。待玩家提供 `target-prime-status.txt` 及报错后游戏存活情况，见[证据与边界](../../164-launcher-steam-install-feedback.md)。

## 体感装填前期：弹药与弹匣模型（2026-10-21）

按用户要求先找可手持装填的完整弹药。已从主包及三张地图的保存清单中导出17个真实弹药相关网格，提供 OBJ、GLB、独立本地预览页与静态多角度图。Lancer、Hammerburst 独立弹匣及其原有 MagazineMesh 引用已确认；另有 Boomshot 弹匣命名资源。Gnasher 霰弹与四种单发步枪／手枪候选经实物检查及抛壳模块引用复核，属于空壳，不能当成完整未发射弹药；Boltok、Gnasher 的独立完整弹药仍未确认，结论限于已检查范围。

17份源导出哈希、OBJ／GLB／网页数组对应检查及 JavaScript 语法检查通过。补查全部1,034个非本地化资源包名称表，全部成功；另核对 Commando Dom 的 Bullet_Strap 骨骼／角色几何线索，未确认可直接使用的完整单发。名称扫描不能证明任意网格内不存在完整弹药。自动打开 `file://` 页面被浏览器安全策略拦截，未绕过；浏览器交互未验收。原版普通换弹和主动换弹成功／失败入口已从保存脚本定位，native 精确时序及卡壳连接待核。记录用户方案：左肩取弹、插入、最终上膛动作，限时内完美换弹，超时普通装填且无卡壳惩罚；本轮没有修改启动器或游戏装填逻辑。见[资源与逻辑记录](../../ammo-and-physical-reload-20261021.md)。所有专有派生资源继续留在忽略的 analysis 中。

## 版本切换基线

2026-10-21 按用户要求，将本地当前源码、工具、资源和文档统一提交为 `5787a48`，完成 0.0.1 阶段的本地归档；后续工作在 `codex/0.1.0` 分支继续，目标版本为 0.1.0。

该归档也包含下文已有的开发中改动，不等同于 GitHub 已发布 `v0.0.1` 的精确源码或新增玩法验收。已发布的标签与下载包保持原样；运行记录、逆向分析数据和编译产物继续排除在源码提交之外。

2026-10-21 玩家安装反馈：已修复 Mod 与游戏 EXE 同目录时无法自动识别的问题，补齐一层 Mod 子目录；启动预留错误直接显示具体原因。修复合入本地正常启动器，保留当前 native/XR 默认构建。玩家截图中的启动失败仅有汇总错误，对方尚未提供详细日志，根因未确认。见[反馈处理与验证](../../164-launcher-steam-install-feedback.md)。

2026-10-21，用户确认开始 0.1.0 工作，首项为继续缩短上下机枪时的黑屏／画面恢复等待。完成代码和必要自动验证后接入正常启动器；头显体验由用户有空时统一验证。

## 首项：上下机枪恢复延迟

首轮缩短等待的实现及必要自动验证已完成，已接入正常启动器；头显上下机枪体验待用户之后统一验收。优化包含一次完整采集路径、有界快速握手轮询、同次属性链复用及 UI 类发现的批量读取。保留最新默认 native 中已集成的 FPS、ADS 和瞄准修复。

同进程四次交替对照中，成功准备的中位数从约 756 ms 降到 643 ms，节省约 113 ms（14.9%），四份权威配置内容一致。该结果衡量 CPU 配置准备，头显黑屏总时长尚未实测；严格跨帧保护引起的重采次数和总耗时保留在技术记录中。

289 项 Python 检查全部通过。对照轮和合入默认源码后的回归轮均完成 checkpoint 新世界恢复与独立 XR 提交，正常退出、79 项入口还原及五份保护文件还原均通过。测试按用户要求等待其他 FPS 工作者，不跨任务发送消息、不打断现有测试。

完整结果见 [首项完成记录](../../mounted-transition-latency4-20261021.md)，前序改动及失败回执见 [第一轮优化](../../mounted-transition-latency2-20261021.md) 和 [属性链优化](../../mounted-transition-latency3-20261021.md)。本记录表示开发进度，不表示 0.1.0 已经发布。

## 继续朝无延迟切换推进

当前完整目标为上下机枪持续出图、没有额外恢复等待，尚未达成。新增静态证书准备、认证模块选择、菜单观察后台化及到期探测的单次采集路径。第一轮只读对照中，成功准备中位数约 643 → 513 ms，但整轮遇到共享源码变动，保留为失败，正在改用固定源码运行目录复验。CPU 配置准备改善不能代替实际机枪切换与头显连续出图证明，见 [后续开发与验证记录](../../mounted-transition-latency5-20261021.md)。

后续固定副本第三轮的新路径已通过恢复、独立 XR 出图及退出还原，完整记录仍区分 checkpoint 与实际机枪。继续减少 Anchor/UI/Crosshair 单次组装中的重复身份读取；当前工作区相关 433 项 Python 检查全部通过，正常启动器已使用该代码。最新固定副本在 native 激活前失败，未获得本轮计时。用户约 16:03 明确由自己真机测试，本任务自动开局、采样及排队工作已全部结束，后续不为本任务寻找机枪或自动开局；实际收益等待用户反馈。见 [身份组装优化记录](../../mounted-transition-latency6-20261021.md)。

## 帧率目标继续验证

默认 native 仍为 CopySimple 全线程分支和 frame-link 有效前缀版（DLL SHA256 `ca66cdeac91915be17d30a5b1ff5008ae575acb6f7e60d82ff4ffb6f5f8a18c0`）。正式启动器模拟器关卡回归在 4K 游戏输出、VR 缩放 0.25、120 Hz 匹配尺寸条件下取得约 63 秒窗口 **111.098 个新立体帧/秒**，尚未达到 120，也不能作为新版头显 FPS。viewport 槽位前缀实验的 A/候选/B 绘制负载不相近，未集成。私有帧事件延迟清零实验已得到默认 A 114.412、候选 111.043 FPS，两段各 60 有效玩法样本，末段默认 B 因另一项自动试玩工作进程仍活跃而取消；候选未集成。下一私有实验准备测正式构建中细粒度 QPC 计时自身的开销，只完成源码预检，尚无编译或性能结论。完整证据见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

补充：帧事件候选在较轻绘制负载下仍慢，未集成；其末段默认 B 因旧空闲门槛没有排除活跃独立自动试玩工作进程而安全取消。旧守护器停止时只在等待，没有本轮游戏子进程。现有 FPS 空闲检查已加入工作进程枚举，实测能拒绝 4 个并行试玩 PID。后续优先在所有试玩工作进程退出后构建并夹测细粒度计时开销候选。

自动试玩退出后，细粒度计时轻量候选已完成私有构建（DLL SHA256 `773a19e6588d0c52b56d21f610a502c09c9e6c6687ac4f7060ffdf8aaeb36b3b`）、481 项依赖核对与 24/24 原生自检。新版空闲门槛下的默认 A → 候选 → 默认 B 三段模拟器关卡测试已排队，当前尚无 FPS 结果；默认启动器未更换。参见 [详细证据](../../157-vr-fps-hot-path.md)。

随后默认启动器合入第一人称预览与右摇杆死区控制，默认清单 SHA256 变为 `b902bfa3…f6a3b`、native DLL 为 `045ca20a…b9d`。上述旧基线计时轻量三段测试在启动游戏前检测到默认清单变化，`runs=[]`，安全终止。旧候选没有玩法 FPS 结论。计时轻量改动已复制到新默认冻结源码的私有副本，500 个文件只改 `render_cost.h`；新构建和测试脚本的只读预检通过，仍需等并行自动试玩结束后构建、夹测。之前的 111.098 FPS 是前一默认版本的模拟器回归，不能写作新默认版的 FPS 或真机实测。

新版计时轻量私有构建与三段对照已交给一次性隐藏排队器 `analysis/runtime/idle-fps-render-cost-lite-preview2-queue-b-20261021.status.json`，PID 21512；先等约五分钟稳定空闲，不占用正在运行的游戏。构建前将重核默认清单哈希，构建后要求 26 项原生检查通过；候选尚未有新基线玩法 FPS，也未集成。旧排队器的 UTF-8 命令引号问题在尚未构建和开局时发现并修正，旧状态保留。

新版计时轻量 DLL `e0cd2215…14c6` 已完成私有构建、484 项依赖核对和包含第一人称检查的 26/26 原生自检。基线采用第一人称预览提升回执的实际文件哈希，修正了冻结源码附带哈希表滞后问题；物理源码只改 `render_cost.h`。A→候选→B 的一次性后台守护 PID 40704 已排队、等待稳定空闲，暂无关卡 FPS 结论，默认版仍未替换。

17:10–17:23 根据用户要求补测高 VR 分辨率。当前默认第一人称预览 DLL `045ca20a…b9d` 在 4K 游戏输出、120 Hz 匹配尺寸模拟器的同一室外街景，Render scale **0.7 / 0.25** 分别测得 **60.487 / 61.426 个新立体帧/秒**，各 60 个有效玩法样本、约 63 秒窗口；0.7 的实际双眼尺寸为 4302×2285。绘制调用中位数约 1629/1548，截图场景一致，用户设置均已恢复、游戏退出、无并行游戏和试玩工作进程。单眼像素数相差约 7.84 倍而 FPS 仅差约 1.5%，故此前室内场景 0.25 的 118.213 FPS 不能用来评估升至 0.7 的损失；室外的低帧率更应追查游戏基础渲染与 CPU/同步/提交路径。计时轻量候选因 UI 证书暂存身份上限连续三次激活前失败，没有有效 FPS，未集成。详见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

17:45 的 720p 游戏输出、0.7 VR 缩放、同室外街景补测为 **59.422 新立体 FPS**，60 有效样本、draw 中位 1622；对照 4K 游戏输出、0.7 的 60.487 FPS/draw 1629，游戏桌面像素数降九倍仍无收益。双眼会话、游戏退出、设置恢复和无并行负载检查通过。当前约 60 FPS 不主要随这两处像素数变化，继续量化高绘制量下的主线程、帧锁与提交成本；尚不能宣称 CPU 根因，也不能将模拟器视为真机结果。

已把完成 26/26 自检的私有计时轻量候选重新排入 0.7 缩放、4K 输出的默认 A→候选→默认 B 夹测；UI 身份缓存满的现行修复通过 11 项 Python 针对性检查。新守护状态 `analysis/runtime/idle-fps-render-cost-lite-preview2-scale07-20261021-180345.status.json` 当前因用户游戏运行保持 `runs=[]` 等待，不干扰游戏，候选仍未集成。

复用 0.7 室外与 0.25 室内原生状态各前 120 个 ticket 的阶段时间：原始消费区间中位 **18.374 / 7.292 ms**，消费返回后到 ready 和实际发布各约 0.1–0.2 ms、0.03 ms。较大场景差异位于包含原游戏、Mod 钩子、驱动和等待的消费区间；不能由此把责任归于某一锁或 GPU。Astra Max 只读复核修正了 `frame_lock()` 短路条件的误读，建议待现有计时轻量对照完成后，低频抽样实际入锁等待和最外层持锁时间，按调用点与线程归因，再选局部候选。默认 DLL 仍未更换。

为在重场景定位消费者 CPU 栈，已准备私有 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07.py`，只读预检、实时状态合成 fixture 与旧室外/室内绘制负载识别通过。该诊断尚未运行；当前 QPC-lite 三段守护进程仍在等另一局游戏退出，保持互斥，不叠加采样。采样器若运行，其暂停线程的诊断开销使该局 FPS 只用于场景有效性，不作为优化收益。

匹配的私有离线分析器 `analysis/runtime/analyze-fps-consumer-stack-preview2-scale07.py` 也已准备，锁定当前默认 DLL 和采样器哈希，并要求堆栈时间戳落在 4K/0.7 室外玩法窗口内。语法和历史回执场景门槛检查通过；仍待取得本轮真实堆栈后才能做 CPU/锁归因。

18:39 检查到另一项 `analysis/playground-20261021/driver.py` 自动试玩工作进程在两局之间仍活跃，但原私有 FPS 空闲门槛只拦到了游戏映像。补齐工作进程识别，实际读到 PID `50276`；旧 QPC-lite 等待器确认 `runs=[]`、无子进程后安全替换。新隐藏等待 PID `38404` 已核实同时拦截该工作进程与正在运行的游戏，仍未启动任何一段 FPS 对照。未中断并行游戏，正式 DLL、用户配置未变。

另准备一个仅在单次 draw 内合并 owned target tracker 重复只读查询的私有源码候选，500 个冻结文件中仅三份实现文件及相应测试不同，见 `analysis/runtime/fps-tracker-snapshot-preview2-source.json`。它尚未构建、自检或测 FPS，默认版未改；先等现有 QPC-lite 对照结果，且该局部改动没有单独达到室外 120 FPS 的时间预算证据。

同一正式 DLL 的室外/室内各前 120 ticket 重算：消费 CPU 均值 **11.979/4.427 ms**，消费墙钟中位 **18.371/7.289 ms**，draw 中位 **1629/353.5**，`frame_lock` 入口计时中位 **5.640/2.383 ms**。类别有嵌套、样本只在会话开头，不能把差值直接当成某项可删除成本；下一步仍是室外玩法窗口的调用栈/等待归因，而不是依据局部查询候选承诺 120 FPS。

18:53 从正式 DLL 的首 120 ticket 跳过前 10 条后核对供帧间隔：室外 4K/0.7 与 4K/0.25 各 109 个间隔均分布为 72 个约 16.67 ms、37 个约 25 ms；720p/0.7 为 1/90/18 个约 8.33/16.67/25 ms，轻室内则 92/109 个约 8.33 ms。室外降像素后仍每两到三个 120 Hz 周期才来新帧，支持优先查游戏/Mod 消费路径与同步；这段开头采样不能单独确定具体根因。详见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

4K/0.7 室外前 120 ticket 的九类成本 Scope 调用合计中位 20,279 次/ticket，正式版每次两次 QPC，约 40,558 次 QPC/ticket；计时轻量候选是对此的端到端检验，仍在等待无并行游戏的 A/B/A 结果，不能预判收益。

19:12–19:33 的 4K/0.7 室外默认 A→计时轻量候选→默认 B 三段对照已完成，各 60 个有效新立体玩法样本，完整窗口 FPS **62.330 / 62.068 / 64.062**；首 120 ticket draw 中位 **1635.5 / 1599 / 1591**，双眼会话、游戏退出、设置恢复、无并行游戏均通过。候选比两端默认分别低 **0.42% / 3.11%**，默认漂移 2.78%，无端到端 FPS 收益，**不集成**。三段消费线程首 120 ticket CPU 平均 **12.500 / 10.156 / 10.677 ms**，基线本身明显漂移，不能将候选与 A 的差额当稳定 CPU 节省。默认 DLL/清单及用户配置 SHA256 不变。下一步仍需重场景消费线程等待位置和锁持有者证据；完整回执与限制见 [帧率分析记录](../../157-vr-fps-hot-path.md)。

19:34 私有重场景消费者堆栈诊断预检通过，隐藏等待器 PID `9728` 已启动；状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-20261021-1935.status.json` 检测到另一项游戏与试玩工作者，仍 `runs=[]` 等稳定空闲。它使用当前默认 DLL、4K/0.7/120 Hz 模拟器，一局中有界外部栈采样，诊断 FPS 不判优化收益，不改发布版或用户设置。

19:46 发现另一试玩启动者 `analysis/playground-20261021/vr_driver.py` PID `43952` 未被空闲门槛识别；补入工作者白名单，新的只读进程检查返回该 PID。原堆栈等待器确认 `runs=[]`、无子进程后安全替换，新隐藏 PID `42464` 状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-vrdriver-gated-20261021-1946.status.json` 同时挡住试玩游戏与工作进程，未启动我方诊断游戏。默认清单和用户配置未改。

19:47 为覆盖同一 `analysis/playground-*` 目录其他 `.py` 试玩脚本，将空闲门槛扩展为该目录内 Python 工作进程。PID `42464` 仍无游戏/子进程时安全替换为最终隐藏等待器 PID `32956`，状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-playground-gated-20261021-1948.status.json` 首次空闲检查已通过、`runs=[]`，发布版和用户设置不变。

19:51 等待器 PID `32956` 在 `runs=[]` 阶段遇瞬时 `PermissionError(13)` 退出，未启动游戏；来源缺少 traceback，不能确定。为恢复安全等待，状态文件原子替换对访问拒绝有界重试，空闲观察错误使稳定计数归零并记录错误，终止错误保留 traceback；语法和模拟一次拒绝的重试检查通过。新隐藏 PID `48656`、状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-resilient-20261021-1954.status.json` 已预检并核实存活，仍 `runs=[]`。默认 DLL/清单及用户设置不变。

19:58–20:01 的正式 DLL 重场景 4K/0.7 堆栈诊断取得 60 个有效玩法样本、draw 中位 1618、266/266 个恢复成功的外部栈快照；但全局监视器在同局发现另一 Python 试玩 PID `48396`，状态为 `profile_incomplete`，**不作为清洁验收或性能收益证据**。探索性分类有 69 次帧锁等待、56 次游戏 WaitOnAddress，不能确定并行工作者是否覆盖 8 秒采样，快照比例也不是每帧耗时。正式离线分析器已加整局无并行工作者门槛并实际拒绝该回执。后台监视器与采样器新增首次见到工作者及采样墙钟时间，便于下次确定重叠；语法/哈希预检通过，新隐藏等待 PID `46348`、状态 `analysis/runtime/idle-fps-consumer-stack-preview2-scale07-repeat-20261021-2003.status.json`，尚 `runs=[]`。

20:25 旧第一人称预览堆栈等待器 PID `46348` 在达到空闲门槛后的开局前校验发现默认清单已由其他开发于 20:21 变更为 `fc54cd22…54e3bd`、新 DLL `bf0a8f73…978855`，安全退出且 `runs=[]`。模拟器 profile、外部采样器及用户配置未变；不能把旧 DLL 诊断当当前默认版。新 4K/0.7 单轮基线脚本已用当前清单/DLL 哈希预检通过，隐藏等待 PID `40480`、状态 `analysis/runtime/idle-fps-current-default-scale07-playground-output1-20261021-2028.status.json` 已核实 `waiting_for_idle`、`runs=[]`；若清单再变会安全拒绝，默认发布文件和用户设置未被本项改动。

20:33–20:35 当前默认 DLL `bf0a8f73…978855` 的 4K 桌面、Render scale 0.7、120 Hz 模拟器室外重场景测试完成：60 个有效玩法样本，约 63.986 秒完整窗口 **66.920 个新立体帧/秒**，样本均值 **66.456**、前后 30 样本 **66.411/66.501**，draw 中位 **1646**。双眼会话通过、游戏退出、设置恢复、无并行游戏或试玩工作进程，清单/DLL/模拟器配置哈希匹配。该结果离 120 仍远，也不是真机实测；旧构建 FPS 不是此版的受控 A/B 对照。回执见 `analysis/runtime/idle-fps-current-default-scale07-playground-output1-20261021-2028.status.json`、`fps-resolution-4k-current-default-scale07-203317-20261021.json`；继续定位当前构建的帧关键路径。

20:48–20:50 同一当前默认 DLL 的 0.7 室外重场景消费线程外部堆栈诊断通过：60 个有效玩法样本、draw 中位 **1614**，**258/258** 次有界采样成功且线程均恢复，暂停最长 **0.909 ms**；整局无并行游戏/试玩工作者、用户设置恢复。瞬时样本中 **90/258** 次顶层栈显示等 Mod 全局帧锁、**44/258** 次在游戏 `WaitOnAddress`，其余为执行或其他等待。每条栈最多16帧，均标记部分展开；这些比例既非每帧耗时，也不能证明谁持锁。该局受外部采样器扰动，FPS 不作提帧验收。严格分析回执为 `analysis/runtime/fps-consumer-stack-current-scale07-analysis-20261021.json`，下一步用私有锁等待与持有计时按调用点/线程归因，正式默认版未改。

当前默认构建的私有锁等待/持有计时源码在 495 个文件中只改 5 个、标注 48 个 `frame_lock()` 调用点；仍保留原始锁及调用顺序。另一项游戏退出后，隐藏构建器在连续 20 次空闲检查后完成 DLL `4d4de7a0…977b2`，484 项依赖与 26 项原生自检通过，未覆盖默认版。诊断版 4K/0.7/120 Hz 室外关卡的一次性隐藏等待 PID `43540`，状态 `analysis/runtime/idle-fps-lock-profile-current-scale07-20261021-2104.status.json`；先等约五分钟无其他游戏、启动器空闲和低 CPU，再测实际关卡。诊断插桩影响 FPS，只用来找锁竞争热点。

私有锁计时局随后完成，60 个有效玩法样本、draw 中位 **1631**，双眼会话、游戏退出、设置恢复及无并行游戏/试玩工作者通过；清单/DLL 哈希匹配，默认版未替换。整段原生会话消费 **4979** 个 ticket，消费线程最外层锁等待累计按 ticket 粗归一化 **4.627 ms**；其中 Reset **2.038 ms/约75.7次**、DrawIndexed **1.135 ms/约1546次**、Viewports **0.650 ms/约94.3次**。非消费线程的 Execute 钩子最外层持锁累计约 **1.673 ms/ticket**，多个非消费线程也频繁 Reset。计时覆盖启动与玩法、增加 QPC 开销，归一化数值不是纯玩法每帧因果成本；诊断局完整窗口约 **65.098 FPS**，不得用于优化收益。它支持先缩短真实持锁工作并保持 Reset/Execute/Present 顺序，再做当前默认版 A/B/A 对照。见 `analysis/runtime/idle-fps-lock-profile-current-scale07-20261021-2104.status.json`。

当前默认版在相同 4K/0.7 重场景首 120 个 ticket 跳过前 10 个后的逐帧原生记录，消费线程平均约 **11.364 ms CPU**、**16.148 ms 墙钟**、约 **1644 次绘制转发**；内部计时类别互有重叠，不能相加。私有 128-ticket 锁时间线保持原锁及 Reset/Execute/Present 顺序，DLL `218ba72a…961cbc5` 的 26/26 项自检及一局游戏通过；原生状态 **325,322** 条事件、128 个完整 ticket、零溢出，60 个有效玩法 FPS 样本、draw 中位 **1597.5**，游戏退出、设置恢复、无并行负载。观察器各有一次 `render=unavailable` 与 `native=unchanged` 瞬时读数，严格按两次缺口、原生发布计数单调和其余会话条件恢复诊断资格；该局约 **59.254 FPS 受诊断开销影响，不作提帧比较**。同一批 128 帧中消费线程平均每帧等锁约 **6.807 ms**，与其他线程 Reset 跟踪更新、原始 Execute、原始 Reset 持锁阶段分别重叠约 **1.290 / 1.169 / 1.062 ms**；余约 **3.01 ms** 包含未记录的其他钩子持锁和调度唤醒，不能称为可节省时间。正确分析见 `analysis/runtime/fps-lock-timeline-current-scale07-analysis2-20261021.json`。下一私有诊断为所有非消费钩子在消费线程等待期间的锁持有重叠计时，源码已固定、只读预检通过，隐藏构建等待 PID `37460` 尚未更换默认版。

补充进展：该全钩子重叠诊断 DLL `2e149532…36c934` 已完成私有构建、484 项依赖核对和 **26/26** 原生自检；Reset 更新细分为附件退休、HDR/PSO 状态、owned scene、目标与视口、frame-link 等十段。一次性隐藏测试 PID `31288` 于 `idle-fps-lock-overlap-current-scale07-20261021-2208.status.json` 等约五分钟稳定无游戏，当前因另一局真机/游戏运行保持 `runs=[]`，没有抢占或覆盖。正式默认版 0.7 的只读整卡 NVML 诊断 PID `45672` 排在共享互斥锁之后，状态 `idle-fps-current-default-gpu-scale07-20261021-2217.status.json` 为等待前项；GPU 百分比只作瓶颈线索。正式默认 DLL/清单和用户配置哈希仍与 20:33 基线一致。两项尚无新 FPS 或 GPU 结果。

22:25 旧重叠诊断达到空闲门槛后，在**开局前**发现正式清单从 `fc54cd22…54e3bd` 变为 `050319a7…bfb86`，状态 `runs=[]`、安全结束；旧 GPU 排队器也在 `runs=[]`、仅等待时按 PID/命令行核对后停止并记录 `replaced_before_probe_due_default_change`。新旧清单除游戏相对路径外相同，正式 native DLL SHA256 `bf0a8f73…978855` 未变，新路径 EXE SHA256 仍为 `bdc6705d…18bb7`，用户配置改为新游戏绝对路径并保持 Render scale 0.5。本任务没有覆盖这些用户改动。只将私有运行清单的游戏路径重新固定，私有诊断 DLL 不重编、不替换默认版；回执 `analysis/runtime/fps-lock-overlap-current-runtime-rebase.json`。新隐藏重叠测试 PID `50556` 状态 `idle-fps-lock-overlap-current-scale07-rebased-20261021-2230.status.json` 为 `waiting_for_idle`、`runs=[]`，正式默认 GPU 诊断 PID `33812` 状态 `idle-fps-current-default-gpu-scale07-rebased-20261021-2230.status.json` 排队等待共享互斥锁。两轮仍各要求稳定空闲及正确加载哈希，未有新运行结果。

22:36–22:47 两项排队诊断已顺序完成采样，但**均未通过设置恢复验收**。私有全钩子锁时间线取得 128 个 ticket、326,582 条事件，消费线程等锁 7.207 ms/ticket 中与其他线程持锁重叠 6.166 ms；Reset 跟踪十段合计 1.706 ms/ticket，未发现可保持排序证明且单独回收 ≥2 ms 的局部工作。该局设置文件的 `ALLOWD3D10`、`StoredPPQuality` 在运行中变化，约 60.975 FPS 不进入优化对照，原生数据只用于探索性热点排序。正式 DLL 的独立 0.7 GPU 观察局有 63 个匹配玩法的 NVML 样本，整卡使用率中位 50%、最高 53%；该局 `QualityBloom=True→False`，约 62.805 FPS 同样不能用于正式对照。两局游戏均退出、无并行游戏，用户设置未被回写；默认 DLL 仍为 `bf0a8f73…978855`。详细回执与限制见 `docs/157-vr-fps-hot-path.md`。此前**完整通过**的 0.7 同关卡正式版基线仍是 66.920 新立体 FPS，距 120 目标较远。接下来优先归因正式版的 on-CPU 消费线程调用栈；现有 WPR 权限不足，既有 118 条非等待瞬时栈只可导航，不能当 CPU 耗时占比。

22:56 已用当前正式清单 `050319a7…bfb86` 和 DLL `bf0a8f73…978855` 通过只读预检，排队一次私有 0.7 重场景消费线程外部堆栈诊断 `analysis/runtime/idle-fps-consumer-stack-current-scale07-triple-20261021-2256.status.json`，隐藏等待 PID `46376`，最初状态 `waiting_for_idle`、`runs=[]`。守护器要求约五分钟连续无游戏、无试玩工作者、启动器空闲和低 CPU；游戏窗口置后、静音、只观察输出，禁后台物理手柄输入。它用现有已自检的采样器顺序取三段各最多 8 秒的瞬时栈，每段独立核实线程挂起恢复，整局仍只作诊断，不把受采样干扰的 FPS 当优化收益；若用户设置再次变化，严格会话仍拒绝 FPS 验收。默认 DLL 和用户设置未由此准备步骤改动。

23:02–23:04 三段栈诊断已完成，游戏/守护进程退出、画质设置恢复、无并行游戏/试玩工作者、正式 DLL/清单哈希一致；每段采样器都恢复了消费线程，合计 **783/783** 条栈，最长暂停 **0.587 ms**。XR 关闭缺 EndSession，严格会话 `passed=false`，本轮约 59.805 FPS 不作为性能对照；原生双眼提交和清理正常，故只把栈当探索性热点导航。380 条非等待快照的栈顶为游戏113、Mod102、NVIDIA驱动86、`ntdll`52、其他27，分布没有指向单一可安全删除 ≥3 ms 的函数；快照占比不是 CPU 毫秒。离线回执 `analysis/runtime/fps-consumer-stack-current-scale07-triple-exploratory-20261021.json`。未集成新的 FPS 候选，正式版 0.7 完整通过的 66.920 FPS 基线仍有效，120 FPS 未达成。

在用户明确要求提高到 0.7 且上述后台探针全部结束后，将 `tools/vr-launcher/config.json` 的 `render_scale` 从 **0.5** 更新为 **0.7**，其余当前配置字段保持相同；新配置 SHA256 `4c2c008a9f94a1c87a4fd25d881f317613bf69460ffd9d3da0589a541db08b6a`，当前没有运行中的游戏或启动器。之前的性能探针均通过临时覆盖使用 0.7，没有依赖本次持久配置修改。新默认画质只代表启动器选项，不代表达到 120 FPS。

为直接比较用户要求的 0.7 与低缩放，准备同一正式 DLL 的 4K 桌面、120 Hz 匹配模拟器室外关卡 **0.7→0.25→0.7** 三段夹测，测试只用临时配置，用户启动器保持 0.7。首段 23:23–23:26 取得 60 个玩法样本、设置恢复和游戏退出，但严格会话因独立恢复核验的 7 个 `dxgi.dll` 入口与磁盘字节不一致而未通过；它们在 Mod 安装前已有 `E9` 入口字节，原生停机自检显示恢复到安装前字节。58.637 FPS 不用于正式比较。真正的失败项是 `entry_restore_verified=false`；`terminal_exit_without_end` 在先前通过的基线也存在，不是本项的失败原因。旧独立回执未记录实际停机字节，不能追认这局。

`tools/native-capture/verify_stopped.py` 现仅在系统图形模块入口的安装前/停机自检快照唯一、一致且原生报告恢复成功、独立远程读到的停机字节与安装前 **32 字节完全相同**时，允许“恢复到安装前状态”；其他情形仍用磁盘 PE 字节验证，游戏入口认证不变。输出记录三方哈希及匹配类别，25 项 verifier 测试全过，未改正式 DLL。新一次性隐藏夹测 `analysis/runtime/idle-fps-current-default-resolution-aba-20261021-2332.status.json` 已排队；因为用户启动器当前打开，空闲门槛保持 0，尚未启动下一局，不干扰手动测试。

等待夹测期间，针对 120 FPS 的下一候选方向准备了私有 128-ticket 双眼绘制阶段诊断，源码在忽略目录 `analysis/fps-eye-phase-current-source/`。它仅在每次原始 renderer 消费调用内记录同线程直接 Draw 的普通左/右眼与最终左/右眼阶段切换、QPC 和相对线程 cycles；不新增游戏钩子、不变更锁或正式 DLL。当前正式 0.7 室外回执聚合的普通直接绘制约为每 ticket 左眼 963、右眼 654 次，表明两眼不完全相同。495 文件逐项哈希仅两文件不同，`build-fps-eye-phase-current.py --preflight-only` 通过；**未构建/未运行**，不会把该探针 FPS 当收益。待分辨率对照结果和用户窗口空闲后再构建，先判断有没有足够大的可共用阶段。

该私有诊断补充已有 `hook_geometry` 的按眼/stage/mode 包围计时与线程 cycles，直接判断这一有限逐眼函数是否够大；计时包含现有 Draw hook 观察开销，不代表原游戏纯函数或完整双眼渲染。修改后私有源码回执重新封存，495 文件预检再通过，正式版及其哈希未改。

逐眼诊断的离线校验器 `analysis/runtime/analyze-fps-eye-phase-current.py` 已准备：要求 2–129 共 128 个唯一 ticket、各段 QPC/cycles 连续、分段耗时及绘制数与原生汇总一致、零溢出，并把原始消费总墙钟、首次绘制前和四类绘制关联区间分开报告；`hook_geometry` 的整局聚合另列，不与首 128 帧直接相减。语法检查和合成完整/篡改回执拒绝检查通过。它尚未读过真实逐眼运行数据，探针仍未构建或用于性能结论；0.7→0.25→0.7 对照仍因用户启动器打开而等待，后台进程存活且未启动游戏。

23:55 用户从启动器运行游戏时保存了配置文件，Render scale 仍为 0.7，但完整配置 SHA256 从 `4c2c008a…8b6a` 变为 `ed66c7fd…6257`；旧 0.7→0.25→0.7 守护器 PID 49200 尚 `runs=[]`，若照旧运行会在空闲后因旧配置哈希退出。核对其命令行、零子进程、等待阶段后仅停止该守护器，将旧状态标记 `replaced_before_probe_due_user_config_change`，未碰用户游戏/启动器。私有守护脚本现到连续稳定空闲之后才锁定当前用户配置哈希，之后每段前后要求哈希一致；三段只在子进程临时覆盖缩放，不写用户配置。语法与清单/DLL/模拟器预检通过，新隐藏 PID 41524 状态 `analysis/runtime/idle-fps-current-default-resolution-aba-20261021-2356.status.json` 已核实存活、`waiting_for_idle`、`runs=[]`；当前启动器仍打开，尚无新 FPS 对照。

逐眼私有探针再补每个首 128 ticket 内 `hook_geometry` 按眼原函数包围耗时和调用次数，以便和**同一 ticket**的 `original_consume` 墙钟对照；这是包含现有 Draw hooks 的区间，若调用嵌套还可能重叠，不解释为独占游戏 CPU。离线校验器同步检查这两眼字段及完整/篡改合成记录。495 文件哈希回执重新封存、构建预检通过，原回执单独保留在 `analysis/runtime/fps-eye-phase-current-source-before-ticket-geometry.json`；尚未编译或测游戏，正式版不变。0.7→0.25→0.7 等待 PID 41524 仍存活，因用户启动器打开而保持零轮。
