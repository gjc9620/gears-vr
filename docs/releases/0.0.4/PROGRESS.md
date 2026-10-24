# 0.0.4 开发进度：武器 6DoF 独立分支

本文件记录 `codex/weapon-6dof` 的后续工作。已发布的 0.0.3、既有分支历史和冻结产物继续保留；早期武器记录见 [0.0.3 进度](../0.0.3/PROGRESS.md)。完整需求见 [武器 6DoF 交互设计](../../weapon-6dof-interaction-design-20261022.md)。这里的私有候选和自动测试不代表新版本发布或头显验收。

## 2026-10-24 — 唯一开火调用入口的真实联调准备

武器候选保持 `c41a65c` 的 PG subscriber（6157a180…5c69），与掩体会话交付的 Native9sim（e8109e81…5727）及 XR5sim（6e0f44c7…4839）进行一次私有互通验证。Native 独占 CallFunction，PG 只管理 ProcessEvent；保持正常 UI，不再使用关闭 UI 后并列安装第二个 CallFunction hook 的路径。原 hand v1 和补充调用 ABI 字节保持不变。此组合只用于模拟器联调，不覆盖最新默认启动器；今后正式集成须以用户指定的 support3 Native／menu-modal1 XR 冻结基线为起点，保留左眼独立历史、菜单修复和诊断功能。

私有测试使用购买版、独立配置与存档，每边渲染比例 0.4，静音、窗口置后、tiny-output 只读，保持物理手柄后台输入关闭。一次真实原版 R2 开火前，由游戏线程对当前 Lancer 的实际槽位提交一次模型拿枪，再连接真实 Native hand／call 注册；不设置 single fixture、不直接调用伤害 getter，也不把模型提交当成身体 RGrip 或双手抓握。测试须分别证明 PG exact-cookie 排空、PG 的 PE 恢复且 Native 调用入口保持、Native 正常停止后的两个原始入口恢复、XR 退出、原音频与原配置／存档恢复。

离线预检查已通过并固定实际工具来源。历史 AP 的 Cover 工具路径只作参考，实际运行使用武器工作树的 1221 项工具源码；派生平台固定 Source1、原始 flags、真实 XR runtime 名称／版本／模块路径和 8720 监听进程身份，未放宽正常头显路径。新增的 passive SupportJournal 单独固定来源，未改已冻结平台源。第一次预检查因历史 Cover 诊断 helper 已变化而拒绝，第二次发现缺失的 passive helper；原失败均保留。

输入脚本采用私有 SafeBridge：提交前持有 request 与实际 Submit thread，原绝对期限覆盖反射、提交和完成等待。Submit 仍在执行时即使 GT 已写完成也不回收；仅已知拒绝码 1／2 可按未受理处理，未知线程退出保留不确定状态。超时只观察同一请求，不重发开火；补发释放以实际 dispatch 记录为准。静音由 root 的唯一 Guard 管理，启动器复用精确身份的 ready 凭据，避免两个 Guard 相互记录已静音状态。晚到的 owned game 和清理本身抛错均继续进入确切游戏／音频收尾。

9 项人工输入边界检查和 45 项 schema2 日志规则检查通过；Astra 对原先四项收尾问题及最后两处修正完成限定源码复核。人工检查不等于跨 DLL、实际伤害、GPU 或头显结果。准备回执为 002d5573…ba556，私有 bridge 检查 4898bbae…ae66；证据位于忽略的 `analysis/weapon-call-runtime-1024bi*` 目录。本条落盘时尚未创建 BI 游戏实例或启动游戏／XR，实际结果须另行记录。

真实副握 mount、全武器装填与黄色引导、自由手动画交还、投射物／延迟伤害、枪口弹道与准星、瞄准镜、体感电锯和投掷、默认合流及整局／Quest 验收仍未完成。完整武器 6DoF 目标继续进行。

## 2026-10-24 — BI-r3 真实共存启动与清理，测试场顺序修正

首次 BI 在创建游戏前因多传 `remeasure_armspan` 配置字段被拒绝；新的 BI-r2 在同阶段因隔离 Python 未显式提供既有 pefile 库路径被拒绝。两个已终止 worker、无 birth 的原始失败、各自一次性 marker 和私有配置保留，没有覆盖或因观测超时重启同一测试。BI-r3 去掉无关字段并仅向私有 wrapper／后代补充既有库路径，未安装依赖、改正常启动器或重建 Native／PG／XR。冷子进程导入检查通过后才启动。

BI-r3 在购买版实际完成正常 UI、Source1 启动及 9025 次立体 projection 提交。实际 XR 名称、版本 281474976710683、runtime／operator 模块绝对路径及 IPv4／IPv6 的 8720 监听 PID 均与确切 owned XR 对上。请求比例为每边 0.4；启动游戏图像资源实测 1536×864。提交计数与尺寸不是画质、可见手部、瞄准对齐或头显验收。

功能试验未通过：测试脚本先自动进入战役，再调用 `PlaygroundTestEnter`，状态在 100 秒观察期内保持 `travel_requested`。源码明确仅在 `WarPC_Menu` 分支发送 ClientTravel；该顺序无法进入测试地图。没有执行 hand／call 注册、观察器或 R2，Native 的 register／attach／admitted 计数均为零，因此没有真实握持或 0.7 倍伤害结果，也没有将其解释成 Native 卡死或已定位的渲染缺陷。

PG Shutdown0，PG 停止后 PE 原64字节恢复；Native 正常停止后独立读取 CF 原32字节及 PE 原64字节均匹配。Native cleanup／entry restoration 为真，XR exit0，hand callback／transaction／aux scope／sticky debt／pin／cookie／retained 均0。确切 owned game、launcher、XR、root 与音频 helper 已退出，9个音频会话恢复、pending0，原10份配置／存档 hash 不变。失败发生在 Native wrapper 基线采集前，不能用“与缺失基线比较为false”推断 wrapper 被改变，也未声称已证明该中间阶段。实际收据 d990fdc3…cf0d，31文件封存与限定汇总 e96184a4…91c9 位于忽略的 `analysis/weapon-call-runtime-1024bi-closeout/`；平台生命周期通过与功能失败分别记录。

BI-r4 仅修私有脚本顺序：不自动进入战役；启动器已启用 PG、仍在 Native 激活前等待 level 时，从主菜单提交一次 TestEnter。随后仍须真实 player／camera 就绪才允许 Native 激活，实际 hand 注册／R2／排空流程保持。Native／PG／XR 产物及 SafeBridge 字节不变，新一次性准备 d84494e2…da7e 已通过。测试窗口已明确归还掩体，等待其完成当前 AQ 后再协调一局，不并发启动；本条不预先声明 BI-r4 运行通过。

## 2026-10-24 — BI-r5 真实共享握持到 Lancer 返回值的有限验证

BI-r4 实际从主菜单进入测试地图，11个拾取物／6个目标就绪，原版 Lancer 的 GiveTo／装备／左手 IK 查询完成。随后启动器可选 right-stick dead-zone 策略在该现场 binding 上以 `Not a right-stick look-axis command` 拒绝并正常停机；无 hand 注册／开火，清理和原10份文件校验通过。具体 binding 字符串未独立采集，不能仅据此认定由测试场改写或归因给当前正常默认版本。BI-r5 只在私有 manifest／config 中不请求这项无关的可选修改，策略 guard 和普通启动器不变。

BI-r5 保持原 Native9sim、PG subscriber 与 XR5sim 产物，实际完成同槽位模型拿枪 status5、Native hand register accepted1／call attach accepted1，PG Start0 并获得 active／armed 状态258。随后只调用一次原版 R2 press 和匹配 release，总时长0.3048秒，原持键 Bool 依次为 true／false；没有直接调用 damage getter 或设置握持 fixture。真实 Native owner 的登记 cookie／incarnation 与四条 `native_shared_owner / fire_invocation / fixture_ignored=1` packet 一致，各自保留 owner frame、input sequence 和 world generation。

四条合格 getter 返回均完成一次 compare-exchange 和读回：三条12→8.399999618530273（`41400000→41066666`），一条20→14（`41a00000→41600000`）。后面三条复用同一 Result 地址，但对应独立 getter／row／scope-call，重新取得原版值20、12、12，没有按旧写入值反复乘0.7。实际 schema2 raw／context／writer 审计通过，Astra 有界复核 c2970a86…2a07 未发现推翻此限定结论的矛盾。

停止时 exact HandStop 首次返回 quiescent5，client off0，PG Shutdown0，PE 原64字节恢复且 Native CF detour 与基线相同；Native 停止后 CF 原32／PE 原64再次独立匹配。Native 已入场的37392个 begin／finish相等，最终 callback／transaction／aux scope／sticky debt／pin／cookie／retained 均0。Native cleanup／entry restoration 为真，XR exit0，确切 owned game／launcher／XR／root／音频 helper 已退出。实际 viewport1536×864符合每边0.4，物理后台输入为 original，无测试窗口遮挡；9个音频会话恢复、pending0，原10份配置／存档 hash 不变。测试窗口已明确归还掩体。

本次21项限定交叉检查通过，33文件封存及汇总4c9a3edd…c23b位于忽略的 `analysis/weapon-call-runtime-1024bi-r5-closeout/`。原 r5 的 `passed=false` 保留，汇总单独记录“共享 owner 到同调用存储已验证”，不改成完整功能验收。`shot_entry_runtime_verified=0`、审计器 `runtime_binary_provenance_verified=false` 及所有早期失败保持；4个 getter 不构成独立证实的4发，也没有测量最终敌人 HP／TakeDamage、投射物或延迟伤害。

拿枪来自真实游戏线程的模型提交，仍不是身体 RGrip；mount／副握几何尚未认证，不能外推 Two、手模型运动、动画交还、全武器或 Quest。该 writer 仍为 Lancer／私有 arena／显式 opt-in／64行的受控版本，未安装到默认启动器。下一步继续接投射物与多目标／延迟结算，并完成真实副握 mount、自由手动画交还及全武器交互；完整武器 6DoF 目标继续进行。

## 2026-10-24 — 手部组件局部字段读取合并

在默认关闭的 TAIL 候选中，同一次 mesh 读取的五个字段改为一次批量定位，读取后再核对 mesh class 和首个匹配的五个字段地址。每个值仍经过原类型、维度、大小、范围、单 bit mask 和 Vector／Rotator 布局检查；字段地址不跨本次非 VM 读取保存。前后身份检查、slot VM 次数、每个 setter 的最后准入、100 ms 原始源龄、0.1 原始单位／0.1° 精度要求及 rollback 路径保持。此项减少重复查找，尚无实际耗时收益或 AX 超限已解决的结论。

新增实际 helper 的 CPU seam 检查56项通过，既有字段检查207项及连续组件 lease 检查1326项通过；完整 TAIL PG 以 MSVC `/W4 /WX /EHsc` 编译成功，私有 DLL 为 a799ecbf…2591。TAIL 关闭时实际 phase1 文件的预处理结果与本次基线相同（仅规范换行和空行；不是完整 TU／ABI 证明），记录 b3f10658…db2f。Astra 有界复核没有发现需修复的保证丢失；CPU fixture 的 resolver／typed reader／schema 使用桩，真实元数据行为的保持来自源码对照，不能据检查数量推断运行验证。

证据位于忽略的 `analysis/hand-component-read-20261024bj/`、`analysis/hand-component-read-build-20261024bj/` 和 `analysis/hand-component-live-build-20261024bj/`。本阶段未运行游戏／XR、未改 Native／XR／默认启动器；掩体会话继续独占已经协调的实际测试窗口。后续自动测试继续使用每边0.4、静音、窗口置后和 tiny-output 只读的独立实例。拾取／近战的同步动画交还仍待独立 ActionGate 补充通道实现，现有 hand V1／call V1 保持不变。

## 2026-10-24 — 共享握持 observe 的 pawn 字段局部合并

单次 observe 中的 pawn 六字段改为同批解析：Controller／Mesh／Weapon／bDeleteMe／bSwitchingWeapons／bNoWeaponFiring。每次取值前核对对象、class、owner structure 和完整 Property，消费 binding 前重新匹配首个同名字段，再执行原有全部末次身份、结构、属性、链接、Bool 和 table 复读。保留派生层首个错误类型拒绝、Core 包、精确 ComponentProperty、维度／宽度、owner 与 pawn 双重 extent、链循环和容量限制；没有跨 prepare／commit／shot／其他 observe 缓存，没有更改注册、拿枪模型、100 ms 或写入授权。

包含实际 helper、原 selector 和生产 metadata Reader 的人工内存检查36类通过，原 hand client 检查21类通过，证据 d37650e6…71df 位于忽略的 `analysis/weapon-hands-transport-20261024bj/cpu-attempt2/`。root 使用冻结 BI subscriber 的13项开启选项编译完整当前 PG，私有 DLL 为7e237915…4395；实际 subscriber／client／shot／writer 的6个离线集成场景通过，包括 nested fire、复用 Result 和原 SEH 传播。它们使用人工内存及本地 Native Host 模型，不是新的跨 DLL／游戏运行结果；旧 BI-r5 工件和实际证据保持原样，也没有预先认定耗时收益。

完整构建位于忽略的 `analysis/weapon-hand-pawn-live-build-20261024bk/`。本次未运行游戏／XR、未安装默认产物。独立 ActionGate 的 draft wire／PG POD 语义已与 Native owner 对齐审查；未知 Scene／stereo 上下文的 NoAuth 通知需要独立无引擎撤销路径，不能复用有隐藏引擎调用的旧 before_action。独立 PE／bridge 前置许可和即时 cache 撤销仍待 Native provider 对齐；在覆盖和验证完成前，subscriber 与持续 STREAM／STOW 的组合继续禁止。

## 2026-10-24 — 动作交还传输与无引擎撤权路径

新增独立 `weapon_action_gate.h`（e8908d94…d361）及默认关闭的 PG Gate subscriber，保留原 hand V1／call V1 字节。Gate 在同一 hand／call 登记之后接入，使用相同 cookie／Native incarnation／context；128B journal 与 PG 本地事务覆盖 begin、原调用空档和 end，停止也须等待它们闭合。原 CallFunction hook／original／左手 owner 仍归 Native，PG 没有新增第二入口或仲裁器。目前 Gate 仅接原只读动画观察，尚未接引擎恢复，也没有实际 Native provider／跨 DLL 结果。

独立 `inspect／revoke_no_engine` 为不能调用引擎的入口提供撤权：GT 检查先于非原子状态读取，按 hands／stow 分别处理相关动作、恢复回声、dirty 和 pending；撤销旧 Plan／binding／readiness，保留未决 journal，禁止通过旧 before_action 间接调用 GetStateName／finish／yield。stow 回声不能掩盖 hands 的义务，真实 PE／CallFunction 不消费 component 的 owned bridge self-token。旧非 phase1 hand Backend 缺少在途纯写入 fence；related 仍只有 bool，pending 自然状态豁免仍需引擎查询。这些限制继续保留。

Gate 的14个离线场景通过，覆盖嵌套、原调用空档停止、错误身份／LIFO、正常容量耗尽、C++／SEH 故障、observer 无异常拒绝，以及清理计数不足后重试不得扣减其他新租约。源码复核发现并修正了 observer 的 void 返回无法证明 token 关闭、全局容量溢出误判、失败释放未记录 attempted 的问题；原动画观察仅新增本次 ledger 关闭的 bool 回执，旧 post／完成计数行为保留。Astra 对最终 subscriber 没有剩余 must-fix。

纯撤权 helper 的五配置人工 host 检查共56类通过；root 完整 TAIL PG 及其17类检查通过，仅将一个与 PG 全局变量冲突的形参改名。Gate 开／关的完整 PG 均以 `/W4 /WX /EHsc` 编译通过，关闭时既有 subscriber 的6个离线集成场景通过；完整 STREAM／STOW 与 subscriber 组合仍被实际 CMake 配置拒绝。保留首轮夹具骨矩阵错误、runner 负退出码识别问题和严格编译的变量遮蔽失败，不用后续通过覆盖它们。

限定汇总1c0b0dfc…c418及14场景记录d2bdea3e…0072e在忽略的 `analysis/weapon-action-gate-20261024bl/`；三个完整构建分别为 `analysis/weapon-action-gate-build-20261024bl/`、`analysis/weapon-action-gate-off-build-20261024bl/` 与 `analysis/weapon-action-handoff-full-build-20261024bl/`。最终 Gate 私有 DLL 为5e8b49cb…1a48。它们证明离线传输／撤权构件，不证明原动画接管或恢复、实际双手运动、Two、投射物伤害或头显验收。本阶段未运行游戏／XR、未改冻结 Native／XR 或默认启动器。

Native owner 正在补显式 Scene／stereo-lock 跟踪和无需加锁的撤销 generation，并审独立 PE／owned-bridge 前置 boundary scope；GT 或普通 Scope.depth 不能代替该许可。持续 STREAM／STOW 的兼容开关继续关闭，直到前置恢复、同帧 cache 撤销与实际拾取／近战／收枪交还齐备。完整武器 6DoF 目标继续进行，后续实际测试仍使用购买版的独立0.4比例、静音、置后实例。

## 2026-10-25 — 原恢复函数的结果回执与前置边界本地排空

hands 原 before_action 实现移到独立文件，并增加 checked 回执；旧 void 入口保持。回执在旧 Action 析构及其可能的 finish／drain 结束后判定，记录实际 restore／yield 返回值，不能因字段恰好干净而把失败判为成功。机械逆变换确认原动作过滤、引擎调用及其顺序未改，母文件除原位 include 外相同。实际生产函数被直接包含的两配置 CPU 检查共62类／60次 checked 与 void 轨迹状态比较通过，覆盖晚期析构新出现 dirty／fault／cleanup_failed／relinquished。

stow 同样保留旧 void 入口，新增 checked 与 pending 完成回执；取消分支仍仅调用一次原 yield_hands，Stop 不充当成功代理。已 Stop 但本次确实交还成功可以 clean；未关闭 pending、无效 slot、restore false 或残留 dirty／fault／visibility 不会假 clean。原 Hammerburst 自然 firing 状态边界保持独立 pending_state_exempt，不能折叠为“已取消 pending”。37类／74次原版、checked、void wrapper 的 host 轨迹与末态比较通过，机械替回后完整原文件相同。

复制 Native owner 的前置 boundary 草案6dac28b3…6709到公共源码，保持它是未冻结、未启用的草案，hand／call／Gate V1 不变。默认关闭的 boundary client 只接导出预检查、同登记绑定及本地生命周期。单原子的 closing-bit＋count 在 worker 停止时先封门；即使 Native Open 没授 scope，已进入的 PG 调用也阻止 HandClient 清空 GT 状态和 FreeLibrary(Native)。Entry 地址／代际防复制及重连混用，清理 attempted 防重复扣减。8类本地 admission 检查和实际 PG stop 路径的 healthy／faulted 两种 fixture 通过：count0不抹掉 sticky fault；内部 GT 拒绝不会封门。原公开停止入口本已拒绝 GT，此处属于内部 helper 硬化。

完整 checked-hands TAIL PG 和 boundary-lifetime PG 均以 `/W4 /WX /EHsc` 编译通过，分别为f7d42842…2aa与68cb312a…af43。后者尚无实际 PE／bridge Open／Close 调用或恢复阶段；生命周期 fixture 使用本地 API 回执与持有的系统模块引用，不是 Native provider 验证。Astra 对目前 admission／client／stop 的限定源码复核没有剩余 must-fix。

证据位于忽略的 `analysis/weapon-action-restore-20261024bm/`、`analysis/weapon-hands-transport-20261024bj/stow-checked/` 和两个 `analysis/*-build-20261025bm/` 目录；限定汇总为 `result-r2.json`（442a77a7…a163）。原汇总仅将包含草案的 header pin 字典误命名为 frozen_headers，保留原件并用新汇总明确更正，没有新增冻结基线。恢复阶段仍须保持 rollback：不能提前用 NoAuth relinquish，也不能预增 stow epoch。还需显式 finish、不可被后续 finalize 清除的阶段故障门槛，以及 Open 前地址稳定的 State512 pool；不确定 Open／Close 必须保留原槽位。上述接线、Native provider、真实动画交还和双手运动仍待完成，持续 STREAM／STOW 组合继续禁止。本轮未启动游戏／XR，未改默认 Native／XR／启动器。

## 2026-10-25 — 固定边界存储、单次恢复阶段与导出传输

新增地址稳定的128槽 State512 pool。每槽保留本次 Request、Open 输出、冻结 Receipt 和独立 Close 输入，票据绑定自身地址及不回绕代际；不确定 Open／Close 隔离原槽，不重置、不复用。正常 NoScope 只接受约定的五种返回，并核对零 State、无 active／engine 标志和无 scope。Native 后来撤销的全局 generation 不会替换本次 Open 的冻结回执。公共 boundary ABI 继续使用 owner 固定的6dac28b3…6709，既有 hand／call／Gate V1 字节不变；这没有建立新默认冻结基线。

恢复协调器记录本次单次消耗，显式执行 hands → finish → stow → finish。具体 adapter 调用已有 checked helper，保留精确 pending_state_exempt；先撤销前向计划，但不提前 relinquish rollback，也不预增 stow epoch。原 Action 析构在外层 hold 下不触发恢复。阶段内部的 C++／SEH 故障只归还公开词法守卫，不清掉私有 busy／restoring／dirty，并留下独立的永久故障门槛。后续写入、reset、正常 finalize 和 client 停止都不能用表面 clean 覆盖它。嵌套 poison 后，两个显式 finish 在调用旧 helper 前再次检查健康状态。

新增导出 transport：取得本地 Admission 后才能读取 API／cookie／context；先 Revoke，再按需 Open。无论是否获得 Native scope，本地 Entry 都跨越原版调用间隙，直到终结通知／Close 完成。未知导出历史、异常、回执损坏和失败 Close 先隔离／quarantine，再释放本地 Entry；正常重复 finish 不重试导出或重复扣减。未返回至完成位的 Revoke／Open 不能被 caller 的完整 Finish 假报 clean。该 transport 尚未挂入实际 PE／owned-bridge 入口，尚未证明同一 active cell 与唯一实际 stage 的入口绑定。

CPU 检查分开记录：pool22类；协调器12类；具体 adapter21个独立进程、37个外层场景；最终 transport38个进程场景，另跑实际 PG client 生命周期3种与 pool，可复算汇总共42个作业。最后补充了旧 NoScope entry 仍在原版间隙、新 entry 已复用同槽的场景：旧 finish／重复 finish 不触碰新代际，worker 停止也不能提前释放模块。adapter 的旧 helper／grant／client 是明确的 host seam；transport 的 Native API／原版 body／成功 Finish 是 host seam。实际 client stop 路径执行过，但 Native provider、真实动画、引擎控制器和 GPU／头显均不在这些 CPU 结论内。

完整 TAIL adapter PG 与独立 boundary PG 均以 `/W4 /WX /EHsc` 编译通过，分别为697bf4cf…19bf和18150ff8…79ca。Astra 对 coordinator／具体 adapter／故障门槛和最终 transport 的限定源码复核无剩余 must-fix；其后的测试源码只增加上述 NoScope 复用案例，生产代码未变。旧 review、历史 CPU 尝试与首两次 C4459 构建失败日志保留。最终 transport作业回执为ebbf61dd…2905，具体 adapter 的 CMake 回执为a5828698…c574；证据位于忽略的 `analysis/weapon-boundary-transport-20261025bn/`、`analysis/weapon-restore-stage-20261025bn/` 及对应构建目录。

本轮仍未启动游戏／XR，未改 Native／XR／默认启动器或启用 STREAM／STOW＋subscriber 组合。下一步是实际入口的递归 NoAuth 分流、初始 owned self token 的独立消费、POD finally 覆盖原版间隙，以及 Native provider 联调；真实自由双手与拾取／近战／收枪的动画交还继续待验。后续实际测试继续每边0.4、静音、窗口置后、tiny-output只读并禁用实体手柄后台输入；保持最新 support3 Native／menu-modal1 XR 与其他会话修复。
