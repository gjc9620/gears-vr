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

## 2026-10-25 — 实际动作入口接线、递归撤权与异常闭合

新增默认关闭的 ENTRY 试验选项，把 PE 和 owned invoke 接到实际 invocation-local controller；continuous forward 的源码入口也接到同一路由，最终 authorize_once 与 setter 仍紧邻。相关 PE 在旧 scope 构造前完成许可判定及一次恢复，随后裸转发原版；递归 PE 包括无关 Tick 直接走原版，不重复进入旧 pre／post／队列工作。真实 CF 的 whole-gap 拒绝标记复用已有 Gate linked depth，相关 CF 仅撤权、不新开恢复 scope。NoAuth 不调用旧 Action／hold／观察器析构来暗中恢复。

owned 初始组件 token 在任何 Revoke 前单次消费；收枪新增绑定 exact Request 地址、object、function 的初始 token，在最终 Args::run 前才武装。消费只豁免对应 participant，不豁免另一侧。mutating／releasing 保留为观察信息，不能单独抹掉动作义务。backend 的 Request／Receipt 副本必须与 active pool cell 一致，begin 还核对真实 active_stage 地址；换一个 Journal 无法复用父 stage。

全部相关 PE／bridge／CF 在 Native Revoke 前先以 context_lost 撤销本地 HandClient publication；Native generation 与 PG published 两条撤权分别闭合。已消费 token 之后才采样 lexical Snapshot。准备阶段的自有 C++／SEH 屏障不包住原版，准备失败仍转发真实 PE；原版异常类型、code 和 opaque 参数继续传播。private cleanup、已捕获 TLS 恢复、transport retirement 与 callback 释放分开闭合；cleanup 自身故障保留固定 State、隔离通道并精确退还本地 lease，不覆盖原异常，不补造正常 post。

本次 ENTRY 运行配置仍明确排除 STREAM／STOW。旧 normal_tick／PostTick 的引擎恢复没有获得整段 Tick 的新许可，ExternalBoundary 仍只授权本次 pre-action 恢复；不能把 Native cached authorized 状态当安全执行点。该禁止组合的 CMake 负例按预期拒绝。连续双手真实写入、实际恢复 helper 与新 Native provider 联调尚未通过，因此候选未集成默认启动器。

完整 ENTRY PG 以 `/W4 /WX /EHsc` 编译通过，SHA 为b74c8fed…8462。实际 controller／无 participant backend／真实 client、subscribers、transport 的39个 host 场景通过；包含 cleanup SEH、原版异常、CF整段递归、不同实际 Journal 及被改过的 backend 副本。另有28个 stage／active-cell 绑定场景。local client prepare／commit 真正发布后，clean NoScope／CFrelated 在 Native Revoke 调用前已撤 publication；旧 snapshot 尚未超过100ms也被拒绝。旧 helper、Native API、original／legacy 和游戏对象分类仍分别是明确的 host seam，不能据此认定真实动画交还。

初始收枪 token 使用实际源码1018字节精确片段，五配置的61个独立进程通过；foreign-thread 的 PAGE_NOACCESS Request 在解引用前拒绝。实际纯 handoff 的同时 dirty／pending／fault 状态保持，engine_reads／engine_writes／logs 均0。旧 handoff 测试同步修正宽泛回声豁免期望，原五配置56类通过。最终 CMake 复算117个作业通过，包含入口39、绑定28、完整纯交接16、core12、adapter21和旧完整handoff一个作业；adapter与handoff各自的外层case计数单独保留，不相加充当游戏覆盖。Astra对最终生产源码限定复核无剩余 must-fix；之后仅增加了测试目标／精确片段生成及选项说明。

证据位于忽略的 `analysis/weapon-entry-routes-20261025bo/`、`analysis/weapon-entry-binding-20261025bo/`、`analysis/weapon-entry-initial-handoff-20261025bo/`。回归编译误复用了BN私有构建目录，该目录中的未发布PG DLL现在对应BO源码，历史BN哈希不能再按该路径当前文件复核；原manifest与8c1e9bc源码保留，本轮另行封存BO产物和此差异。冻结support3 Native／menu-modal1 XR已分别复核为4752801a…ae14／37f46312…a4d，未改动。未启动游戏／XR／RE；后续与Native16模拟器候选配对须继续独立0.4、静音、置后和observe-only，完整武器目标继续进行。

## 2026-10-25 — 按用户要求优先完成双方共用的自由双手

用户明确要求先实现双手自由移动，掩体会话也需要同一输入。执行优先级据此调整：先打通同一Scene的左右手追踪、完整世界变换、回中／转向及epoch，再用实际双手骨骼位移证明独立跟手和动画后重新接管；武器与掩体共用该结果。b16cfbe入口候选保留，新的writerless boundary互通探针后置；完整12武器、换弹、瞄准镜、电锯、投掷、UI等目标没有缩减。

源审确认旧GHW1世界转换与Cover点映射在相同基准下相容；正确基准是双眼实际采用的最终pre-HMD base_view，不能一概换成原source_view。Cover当前Scene已冻结一次matched controllers并选出最终view，但没有旧GHW1发布段。双方协调由Native唯一owner发布同票RigTracking数据，明确原始输入／reference／source times、world/context identities、最终基准、units与失效纪元；数据不能充当骨架setter许可，不改旧hand V1，不另读一次“更新”的输入或重盖时间戳。

AX的0 movementcommands已定位为试验顺序问题：runner先等枪体joint receipt，初次等待失败，未到第一条手部命令。现有clean waiting的hand_only路径允许先验证左右手各4cm纯平移及回位，之后再验动画重入；实际手骨世界矩阵、另一只手不串动和原有0.1单位／0.1度、100ms限制继续保留。RuntimeTracked0与MetaOperatorSimulated1必须按实际来源分别核对，模拟器身份不等于certificate source1。Support3标准冻结本身没有GHW1 producer，后续需要最小、可核对的共源发布候选，不能直接复用AX旧renderer或其scale1配置。当前仍在离线准备，运行窗口已留给Cover完成AR14；所有实际试验继续0.4、静音、置后和observe-only。

## 2026-10-25 — 独立手部运动候选，先于枪体接管发送手部命令

新增默认关闭的 `GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL`。该私有配置保留原 TAIL、certified、persistent hand-only 依赖、304字节 Start、原15秒期限和逐次 Plan／对象／时效／journal 检查，固定不进入枪体 acquire／update。normal_tick 先执行原手部恢复并退出 Operation，再要求 clean waiting 或带成功 drain 证明的 clean suspended；不恢复枪体区间，也不清除 closed、drain 或故障记录。普通配置继续原路径。编译时来源只能明确选择 RuntimeTracked0／MetaOperatorSimulated1，请求还需与相同的单字符来源环境值对应；不借可选 R2 模拟器开关决定手部来源。

Astra 对三文件限定源码复核无剩余 must-fix。新的 `analysis/hand-independent-motion-20261025bp/root-builds/attempt1/` 独立目录完成 source0、source1 和 feature-off TAIL 三种完整 PG 的 `/W4 /WX /EHsc` 构建，分别为83e90e24…82b1、ee61f1d0…c807、58371cf5…b854；没有复用或重建此前封存候选目录。这只是编译结果，未运行游戏／XR，尚无四条实际移动命令或动画恢复的运行收据。

新增 `test_hand_independent_motion_profile.py`，机械抽取实际 normal_tick、hand-only readiness、模式选择与 component dispatch guard；三种严格 CL 配置、15个独立进程通过。两个来源下均验证 clean suspended 连续两次手 journal acquire／restore，component resume／apply／acquire／update 均0，原 closed／成功 drain 记录保持；dirty／failed-drain／revoked 拒绝且保留原记录。实际 Policy 的100ms边界、任一原始来源超时1ns、MS期限及错source拒绝保持。完整结果为 `lifecycle-cpu-attempt5/result.json`，此前宿主构建和 fixture 构造失败分别保留；引擎 restore／finish／end 是明确 host stub，纯 primary journal 不证明 tail writer、实际手骨或游戏动画。

与 Cover 协调的共享 `rig_tracking_math.h` 已按 d52efa9d…7502 原字节采用，使用最终 pre-HMD base_view；完整姿态输出是 UE world forward/right/up，不能重复做 XR 轴转换。Native 的新只读 Rig 帧仍在准备，原始追踪数据本身不能冒充旧 GHW1 certificate；现有 XR companion 仅提供实际 stream／tracking 纪元，旧 reference_epoch 则来自 Native StereoGate 的独立连续性计数。后续须以匹配当前冻结渲染来源的真实 producer 候选接入，缺失信息保持不可用。本条未改变默认 Native／XR／启动器，也不代表自由双手完成。

## 2026-10-25 — 共用 Rig 读取、最新 XR 同源输入与独立试验收尾

独立手部核心已提交为 `eda686d`。按 Native owner 交付的58cbb26f…76b0原字节采用 `rig_tracking_frame.h`：真实 producer stream／逐手 tracking 与实际 NativeReference 分开，Frame1824、ReadRequest40。Root 在自己的依赖下严格编译并核对全部给定 offset／align8；新只读 `rig_tracking_reader.h` 的18个 fake-provider 检查通过，验证 exact NativeKey／source／Scene 下界、读后时钟、原始双来源时效以及缺 producer 拒绝。此 helper 只复制数据，尚未接真实 Native 调用，不授予骨架写入许可。typed RigPolicy／完整 receipt 的前向再授权接线仍在进行，不从 raw Rig 拼造旧 GHW1 certificate，也不增加新的 GHW1 publisher。

XR 改动从准确 menu-modal1 源另建私有副本：provenance为c089c4d8…390e，586个原文件和两份封存 JSON 构建后均保持原 hash。normal 候选只增加7个文件／13个 hunk 的 GVC3＋GHC1 发布接线；同一原始 actions sample 及时间先进入实际 SourceTracker／两桥，再尝试发布 eyes。未发布、失追、失焦均保留真实源历史，不二次采样，不增加 producer reference。原颜色、菜单修复与原 WAIT ON／SIXDOF OFF／HDR OFF／编译 flags 保留。四个原菜单 CPU 和三个严格桥／实际 loop 抽取 CPU 通过。normal XR为4174d0e3…1ead，manifest为056682a7…6b4，位于忽略的 `analysis/hand-xr-continuity-20261025bq/`；额外完整 `/WX` 试验的四处原 SDK C4459 失败另行保留，正式候选按母体原 flags 构建。它尚未运行，Source1sim 是独立后续候选，不能重标这份 normal。

新的 `hand_independent_motion_run.py` 默认 UNBOUND，在项目 helper／prepare 前拒绝。四条手部命令先于任何枪体 joint 等待；即时 typed control／tail 字节、后续 Tick 的实际 pre_restore 手骨、restore 回执分别核对，缺同 lease 的 post-restore 世界手骨仍 pending。复核发现的晚到 reservation child 和 raw Start 在途参数释放问题已修：先收拢严格身份的原 worker／birth，再清理 owned game；实际 retained adapter 分别保留 request、Submit／export 原线程和 GT 结果，未知时不重派发、不 free、不卸载 Native，直到原线程或确切进程实例退出。修后限定源码复核无剩余 must-fix；34个 runner 离线场景和22个 fake Win32／thread adapter 场景通过，分别保留在 `runner-final-attempt5/` 与 `bridge-cpu-attempt2/`，不是实际进程／DLL／游戏收尾证明。

本节没有启动游戏／XR／SDK session，未改冻结默认、原已发布包或其他会话 checkout。Native18b／新XR配对、真实左右手4厘米运动、完整旋转与大伸手、动画交还及头显验收仍待完成；后续自动测试继续每边0.4、静音、置后、tiny-output只读和关闭物理手柄后台输入。

## 2026-10-25 — typed Rig 手部接线完成编译，执行阶段仍待合流

新增独立 `RigPolicy/RigPlan`，完整1824字节 Native Frame进入 Plan；Scene publication 与 raw XR sequence 独立检查，首帧 baseline、新 Scene 同 raw 不重复写入、逐手 loss、实际 NativeReference 与完整身份／source／cookie／epoch／base 约束保持。两份原始 source 时间按100ms严格授权，局部 receipt 的 MS 时间仅作诊断，不伪称 Native uptime。新策略1261项检查／18组场景、旧 stream25组／certified12组回归通过，旧 policy 源未修改。

PG 私有 Rig 分支已从真实 Native getter 读取，完整 typed Plan贯穿 tail backend，写前及 written 后均重新取得 live Rig，并在该读取返回后重读本地 action／对象／输入代际。恢复不依赖 fresh Rig。原配置8组／16份选中代码 token 等价检查通过，实际 helper全文及授权路径提取146项检查／15组 host场景通过。新增数据 getter 的 closing-bit/count 生命周期与24字节 owned Connect capsule分别复核；它们保留真实模块与登记，Connect不加载 Native，不改变原 Start304。

稳定源码的 `full-pg-attempt2/result.json` 中 source0／source1／Rig-off TAIL 三个完整 `/W4 /WX /EHsc` 构建通过且 source pins 全程不变。attempt1 三个编译也 exit0，但代理在构建窗口补了 GT 和读后 action 复核，因此一致性检查按原样保留 `passed=false`，不将其当封存候选。Native18b采用 owner 的不可覆盖 `native-body18b-ready-semantic-final1.json`，943fea2b…c304；首次 adb69…旧元数据 pin 拒绝及原件保留，两个 DLL未变。新XR Source1sim为1a059d2d…b4e9、manifest c189bcca…efa80；实际 operator模式、0.4与三个后台环境门槛明确，normal与menu-modal1母体保持原 hash。Root本轮没有启动游戏／XR。

**这些 PG 目前仍只是编译与限定 CPU 候选，不能按 Native18b 互通 READY 使用。** 继续源码核对发现原 handanim CF hook 承担同步 hands/stow 交还，不能只迁移其观察或假置 reserved。Native18b是唯一 CF owner；现 CallV1禁止该同步恢复，CMake的 subscriber＋stream/stow 以及 ENTRY＋normal Tick禁配继续保留。现 Native ActionGate已有受限恢复权限，PG还需接真实 Gate-bound恢复stage；正常 Tick写入也需要 Native实际安全阶段，RigRead和原PE返回均不授这项许可。双方正比较实际阶段接线与 Native最终骨架执行路径，既不安装第二CF hook，也不把四条idle命令当完整交还证明。审查回执为 `pg-call-owner-coexistence-review.json`（ed078cdf…1b17）。完整自由双手、与掩体的真实运行和动画交还仍未完成。

独立脚本随后完成实际 RigV1 recipe：固定真实 MSVC layout／header，完整1824字节 sidecar 与 pose、Native/game/XR、五对象及原始双 source 时间交叉核对；四条4厘米命令、原残差和后续 pre_restore 骨骼检查保留。Connect从同一确切游戏 handle 枚举已加载 Native，按精确路径发送一次 owned24请求，使用原 startup期限；Hand15秒仍从HandStart前开始。新脚本737fbe7c…39a5与旧 retained adapter源码经限定复核无 must-fix，85个离线场景通过，证据位于 `runner-rigv1-attempt7/`。三项 `sole_native_cf_owner/cf_handoff_ready/normal_frame_stage_ready` 默认false，真实 pinned source manifest也须逐项证明；当前编译候选不满足，仍在任何artifact／SDK导入前拒绝运行。该 preflight声明不是运行时 Grant，尚无实际4厘米移动或最终世界手骨恢复证明。

## 2026-10-25 — 自由双手优先，共用 Native 骨架执行方向

依用户最新要求，自由双手先于依赖它的武器与掩体交互。双方选择 Native 的 C1 最终骨架工作区，由唯一 owner 按身体、手臂与武器顺序计算和发布。只迁移旧 PG 观察入口无法解决同步动画交还，因此不再为它新增 normal Tick 前向授权；现有 subscriber＋stream/stow、ENTRY＋normal Tick 禁配继续保留。C1 工作区的实际发布与消费、下游物理／附件关系和动画恢复仍在由 Native owner 核对，尚未宣布可运行的最终骨架 writer。

新增共用 `hand_two_bone_pose.h` 与实际 header 测试，分别固定为015d06ac…922f与2aabe8d4…4d67。输入是同一坐标空间的肩、肘、腕原姿态及完整腕目标，不包含骨号、引擎调用或写入许可。保留长度模式对不可达目标明确失败；自适应模式调整两段长度并精确保持腕目标，原 packed Atom scale 由 Native adapter 保留。完整腕旋转保留 roll；上臂／前臂按原始骨轴施加摆动旋转，不假定模型骨轴。关节极限、碰撞和奇异姿态的唯一解不在这份纯数学能力内。

严格 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译与16组 CPU 场景通过，涵盖内外可达边界、伸长／折叠、镜像、128次刚体变换的三类场景、极端单位量级、完整腕 roll、4厘米平移及回位、失败输出不变。最终回执为忽略的 `analysis/hand-two-bone-20261025bs/attempt2/result.json`，较早尝试原样保留。这是数学验证；实际角色手骨、武器附件、最终骨架发布与 GPU／头显验收均未完成。当前不改默认 Native／XR 或普通启动器。

## 2026-10-25 — Native Gate 绑定的单次恢复实验

新增默认关闭的 `GEARS_PLAYGROUND_WEAPON_GATE_RESTORE_TRIAL`。实际 Native Gate.begin journal 链接后、observer 与原版调用之前，adapter 核对完整 Invocation、Native State 地址、GT、cookie／Native 实例、当前 Entry、backend 与 stage 地址；在第一个 helper 前持久记录一次性 claim。重复、拷贝、嵌套、原版间隙或返回后的尝试不能重放恢复。沿用现有 hands → finish → stow → finish／verdict／POD cleanup，撤销前向计划但保留私有 rollback；不生成 external Open 回执，不授 normal Tick／前向写入权限。State128、hand／call／Gate V1 和原版调用位置保持。

38个独立 host 进程场景、1225项检查通过，旧14场景回归及两份旧文件各三种 OFF 预处理输出去空白一致。完整 GateON／GateOFF PG 均以 `/W4 /WX /EHsc` 构建通过，DLL分别为1f981675…37ec与98733010…7c66，八份 source pins 和实际 DLL hash 在 Root 复核时均匹配。限定独立源码复核未发现新增 P0／P1。证据位于忽略的 `analysis/weapon-gate-restore-20261025bs/`，失败尝试与日志保留。

这些 full DLL 仍未包含 STREAM／STOW，配置禁配保持；实际 helper、Native 锁／阶段及 provider 在 CPU 场景中是 host seam。此项仍是恢复实验代码，不是自由双手、真实动画交还或运行 READY。未启动这两个 DLL 的游戏测试，也未改默认候选。

同轮首次私有只读骨名采样 `hand-reference-names-20261025bt` 在任何 game birth／Native session 前被 launcher 的 playground artifact 路径检查拒绝，未发送 Pose event。原10文件 hash 相同，事后 Game／XR／launcher worker／8720 均不存在；缺少 session 的 finalizer 如实保留 accepted=false，不能当 Native／entry／audio 清理通过。原因是派生 manifest 将要求相对路径的 playground_build 改成绝对路径，新的私有 bu case 将恢复正确精确 origin，额外离线核对实际 loader 与 launcher，保留 bt 全部失败原件。

## 2026-10-25 — 同局骨名与真实父链采样通过

bu 修正 PG 精确 origin 后仍在 game birth 前拒绝：私有 PowerShell 直接启动共享 `graphics_policy.py`，Python 的脚本目录优先级使它导入了旧 instance policy。失败的 bu 与 bt 均完整保留。新的 bv 用固定12个 target 的私有入口先载入严格私有 context，再保留实际脚本路径、参数、退出和清理行为；共享规则未改。全局配置修改、物理后台输入和 instance prepare 的入口仅允许离线 probe，实际路线不可执行。12个 fresh-child namespace probe、25个 Python 编译、实际 PG／driver 配置和 PowerShell 检查通过，限定独立复核未发现 P0／P1。

随后 bv 在购买版与 Source1 模拟器中完成一次实际只读采样。前后115个 FName 的 index／number／名称、完整 opaque RefSkeleton 原始字节及同一 game／owner／XR／Native session／mesh／asset 身份一致；本次唯一 PoseEpisode 的 C0／C1／Plate0／Plate1／L1 均与之匹配，父链只取该局实际 C1 parser 输出。已能确认当前模型左右真实手臂的直接父链，未将 IK helper 当作真实手骨，也不将本局骨号推广到其他模型。Native 二进制594792字节，SHA b5098d75…9e57；join为00511f9a…245e，final为fe2b7dad…3d91。完整原始数据仅保留在忽略的 `analysis/hand-reference-names-20261025bv/`。

该局 `runtime-final.json` accepted=true：Native 正常停止、入口及88项独立核对、8份完整 pose chunks 还原通过；XR exit0；原10文件 hash 相同；匹配的音频10次尝试均恢复、pending0；PG shutdown0／entry restored／stopped；tiny monitor joined；确切 owned PIDs为空。host较早记录的 `no_remaining_games=false` 原样保留，后续独立 finalizer 与全局 Game／XR／8720 查询均已为空，没有回写旧瞬时结果。

测试保持每边0.4、静音、窗口置后、tiny-output只读和物理后台手柄输入关闭。Body／Rig／hand writer 与 engine restore 全部关闭；本次是骨名、父链和生命周期证据，没有控制器运动命令，也没有新增骨架／游戏字段 writer。已向掩体 Native owner 交付，继续用于共用 kernel 与真实 Atom／父链适配；自由双手运动、动画交还、最终发布／物理／附件与头显验收仍未完成。

## 2026-10-25 — 本局真实手臂链的纯缓冲区求解验证

将上述 bv 的同局 C1 原始 Atom、父链、component 和已认证骨名作为输入，只复制唯一 shared kernel 与 Native owner 的纯缓冲区 adapter，未改双方生产源码。当前 session 的 requested configuration、实际 Native command／config／status共同确认100 units／metre。左右各自4厘米平移／回位、90度腕轴旋转及多轴完整旋转共8个正向场景，以及两侧 preserve 不可达拒绝通过；实际 header 以 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译运行均 exit0，12份原输入、源与 header 副本前后相同。

float Atom 打包后经实际 FK，目标腕位置最大误差3.289601e-5原生单位，旋转最大1.707830e-6弧度；每步保留115个 scale 字节、112个非链 Atom 及另一手 world 字节，19个相关后代的纯 FK 重组通过。输入、源码、依赖、结果和首轮记录保留于忽略的 `analysis/hand-arm-actual-20261025bw/`；最终 `result-attempt2.json` 为61f067c1…8659，已交 Native owner。

这些目标是 Native wrist pose，未把 Rig world_grip 旋转直接充作腕骨；grip→wrist 校准仍须真实接入。八个4厘米目标均未触发长度调整，因此没有新增大伸手模型验收。回位只表示腕目标位置／旋转回位，量化后链上的三个 Atom 不保证原字节；这不能清除运行时 rollback 债务或替代保存原始 preimage。本节没有启动 Game／XR／SDK，也不证明真实骨架写入、模型变形、最终渲染／物理／附件或头显效果。下一步由 Native owner 的默认关闭、有限次数 C1 overlay 验证实际写入与恢复，仍保持唯一执行端和18b／默认冻结版不变。

## 2026-10-25 — 原版合成变化回归与控制器到腕骨变换

核对 Native owner 的未接 live 草案时，发现其 C1／排债 L1 要求完整 C0 Local 与 C1 Local 相同。重新按 bv 的实际 parser 范围和 hash 比较，原版恰会修改左右两条真实臂链的六个 Atom，包括 quaternion 和少量 translation。原 host 的 compose 只更新 Space，没有覆盖这个实际性质。Root 对稳定私有 c2ecc4c3…259e 源快照补了合法原版 Local 变化回归：严格 CL 编译0，功能期望 applied，实际返回 context、writer_calls0，测试 exit9／functional_expectation_met=false 如实保留，未启动实机写入。证据为忽略的 `analysis/hand-c1-overlay-review-20261025bx/`。

Astra 的限定审查另指出 C0 部分归还后的 consumer 安全、失败后的 phase 可恢复性、Space journal 提前退休，以及 desired 数组／实际控制存储的别名保护问题。审计 ebc88d1f…71b7 与真实六骨变化、失败回归已交 Native owner；其正在分离 C0 输入、原版 C1 preimage、overlay after 和归还来源，并补对应 fixture。此处是对指定未冻结草案的发现，不是新 Native 候选的通过回执。Scope flags、C1／L1 样本和 host seam 均不授实际写入权限，仍待真实入口、恢复及 consumer 分支接线；18b、默认 Native／XR 和普通启动器不变。

并行新增 `hand_wrist_calibration.h` 及实际 header 测试，固定为d73d075c…d224与bcefadc2…dabe。`Calibration` 默认 unknown，保存米制 grip-local 偏移与完整相对 quaternion。`from_caller_aligned_world_pair` 只在调用者已另证对齐的前提下计算相对变换，`native_wrist_target` 由同 UE 世界 grip、单位倍率和已知 offset 产生完整腕目标；平移只换算一次，完整 roll 保留，失败不修改输出。known仅表示数学校准存在，不证明物理对齐、身份／时效或 Native 权限；旧临时 binding、solver、Rig、PG、Native 与 CMake 未改。

严格 `/std:c++17 /W4 /WX /EHsc /UNDEBUG /MT /O2` 编译和10组 CPU 检查通过，涵盖单位、90度及多轴旋转、完整 roll、回中／原位输出、世界刚体变换协变、左右独立 offset、合法 q 归一化、未知／非法／溢出输出不变。实际编译依赖、五份源及快照前后 hash 相同；最终回执位于忽略的 `analysis/hand-wrist-calibration-20261025by/attempt1/result.json`，c078001a…9810。实际标定流程、持久化和 live 连续跟手仍未实现。本节未启动任何 Game／XR／SDK／IDA，也未改其他会话源码。

## 2026-10-25 — 原生控制输出路线与独立追踪数据核心

Native owner 的 C3 私有修订已移除 C0／C1 相等假设，并改进部分失败、blocked 阶段及已知别名处理。Root固定六份源码，重新从本局 bv 提取完整 C0／C1／L1 原始 Snapshot，用原 C3 header／test 严格编译运行，8455项／43场景通过，16份源码／输入前后相同。证据位于忽略的 `analysis/hand-c1-overlay-review-20261025bz/bv-paired-attempt1/`，result为6a98b4f3…12ee7。实际执行的 IO／GT／Request／proofs仍是 host seam，paired overlay只验证默认左链。Astra继续指出 Local 任意分量变化即整字段退债，以及 Space 仅凭 different-after 判完整原生生产的未闭 P1，审计775c53f4…8d8c保留。C3不接 live，不因这份 CPU 回执变成写入 READY。

后续主路线改为复用原生手臂控制的临时输出，由原 Compose 负责正式骨架和后续传播。Native owner 的新 RO4 前置独立窗口取得12个控制节点，实际手链为活动的 Limb／SingleBone 与强度0的尾节点；旧 PG 四通道恰选尾节点，不能复用其强启用结果作为新路线证明。Root已读原始前后／final：前置 accepted=true，后置因观察期间值改变拒绝，整轮 accepted=false、cleanup verified=true，未拼成原子 Pose 或提升执行权限。原生节点实现、目标空间、完整旋转、最大伸手限制与临时输出传播由唯一 Native RE owner 继续核对；Root未运行新游戏或使用 IDA。

并行源码检查确认当前 Rig producer 在发布前后已依赖真实 hand 注册，不是只在 public read 上检查 cookie。现有同次 Scene 的 TrackingBundle／tracking_for、最终 pre-HMD base 与原始来源／reference 时间可以复用，但尚无供内部节点使用的独立完整快照入口。Root据此新增 `native_rig_tracking_frame.h`／实际测试，NativeSessionKey为24字节、无 cookie，CoreSnapshot为独立1832字节／align8；source_ticket与native_reference_revision仅作内部形状检查，current核对由 Native 外层实施。原 V1 Frame1824／ReadRequest40及全部成员布局保持，公共验证仍要求非零真实注册。

公共 `rig_tracking_frame.h` 只抽取同一份 payload validator及按成员原字节拷贝 helper；Core与public共用原数据策略。`project_for_authenticated_registration` 仅在调用者已另证真实 broker／receipt 后按同 Native session 投影，再跑原 public validator；不造 cookie／producer／时间／sequence，不授调用权限。默认50毫秒和100毫秒上限、head-only及无 companion 的原数据语义保留；手部 consumer仍需实际 producer、各手mask、当前 ticket／reference／stop和安全阶段。

最终两个生产 header 固定为d6b295eb…c2b2（Core）、af103284…d684（public）；测试83f2f7d8…b8cf。严格 CL actual headers／冻结旧 V1检查通过：Core8组204581项，含58484份 Frame及328份 Request行为差分、48220份合法变异的完整 V1输出字节比较；原 RigPolicy1261项／18场景回归通过。布局、原声明／request_valid原文、payload抽取、17份源码／依赖及11份只读输入均核对，attempt1原件保留；最终回执为忽略的 `analysis/hand-native-rig-core-20261025cb/attempt2/result.json`，f45580fd…fa35。Astra限定复核无 P0／P1，37a8296e…ebac。

本轮没有实现 Native store／Scene builder／getter／latch，也未修改其他会话 live／capture／shared源码、运行 Native DLL 或启动 Game／XR／SDK。数据模块已交 Native owner 接线，仍需真实原生临时输出、控制器持续跟手、动画交还、全部角色／装备状态与头显验证；默认冻结 renderer／启动器和18b保持不变。

## 2026-10-25 — 原生分支接收数据核心，准备控制输出解析

Native owner 已按最终三文件 hash 接收 Core。首次接收检查因其本地 `stereo_frame.h` 与原 CPU 依赖不同而正确拒绝，失败保留；后续明确重新用实际本地 headers 与冻结旧 V1 编译，而非复用 Root CPU 结论，8组204581项检查通过，实际依赖与三份源码前后相同。Root已读取 `native-rig-core-receiver2/actual-local-result.json`，passed=true，回执a490c99d…e488e；这只证明另一组实际依赖下的数据层接收，Native发布／缓存／getter／latch仍未接入。

下一观测候选由 Native owner 以冻结18b的最小增量准备，默认关闭，记录原生控制返回的临时矩阵、权重及相关状态；不在该轮混入 Core live 迁移或姿态 writer。Root初读了 register-only thunk／protected bridge，观测后仍调用原方法并在 finally 记录和清理；候选尚未冻结，因此不以本次初读作为最终ABI／运行通过结论。Root本轮没有启动游戏、XR或IDA，也未修改其他会话源码。

Root另交付了离线解析所需的MSVC布局字段清单，位于忽略的 `analysis/hand-native-control-output-20261025cc/plan-draft1/required-layout.json`，89e46f0c…1939c。它明确为DRAFT_NOT_FROZEN：所有size／align／offset仍为null，尚未实现decoder或接受实际binary。清单覆盖Header／Row／NodeState／Method及Pose依赖类型、数组stride／count、基础类型与bool表示、Failure枚举，并要求确认episode／ordinal／linked-row关系、矩阵空间及参数来源。最终解析必须绑定冻结源码、实际MSVC布局和同session CPOSE数据，保留异常／失败／缺失／嵌套／并发／非有限值；端点相同不证明读者排他、未来权重、最终渲染或物理结果。

## 2026-10-25 — 原生控制输出实际布局与精确阶段关联

用户再次确认先完成武器与掩体共用的自由双手，双方继续使用 Native 唯一身体／手部执行端。Native owner 已冻结控制观测 v1b：生产 inl 为a3276d4f…7725c，四源交付9f0dde8a…00d26。Root在自己的忽略目录独立编译布局 helper，直接 include 对方冻结的真实 test／inl，由实际生产类型导出，未重新手写结构或运行 test main／observer。`/std:c++17 /W4 /WX /O2 /MT /EHsc /UNDEBUG` build／run均0；实际编译依赖、四源及其他依赖前后相同。

实际 Header72／Row1944，九类全部成员的size／align／offset、数组data offset／stride／首末、基础类型与bool／float表示、Failure十五值及rows128均已导出。布局为忽略的 `analysis/hand-native-control-output-20261025cc/layout-v1b/attempt-1792887601705353300/layout.json`，2d0b064d…9529。CCTRL001是默认关闭的只读记录；本次ABI检查没有证明任何实际sidecar、节点姿态写入或跟手结果。

新增离线CPOSE精确关联器，要求同session、完整owner、thread、1-based episode、outer tick和Compose invocation相等。Astra首审指出缺少控制QPC区间矛盾检查，以及L1／physics pair调用错配；旧源码与报告5bc212a3…ab3c完整保留。修订后要求正整数QPC（排除bool）满足C0≤before≤after≤C1；主／late物理各自配对并核对原caller、kind／phase、invocation及physics_matrix；L1核对原outer tick。缺端或矛盾保持具体原因，不借用其他阶段的数据。Weight只有原parser已精确关联且时间位于对应区间时才列为可观察记录，仍不解释未知应用权重。

严格关联与反例42场景通过，六份源码／输入前后稳定；回执为同目录 `join-v1b/attempt-1792888199784113300/receipt.json`，e56f5fd6…2e3d。Astra仅复核两项原P1与新增weight过滤，均已关闭／未发现新P0或P1，报告d263715d…aa03。此处输入是此前真实bv CPOSE加明确标记的synthetic控制行，没有冒充新CCTRL实采；矩阵相等始终只是数值观察，reader fence、最终渲染／物理、writer权限和游戏验收均为false。Root本轮没有启动Game／XR／IDA；连续跟手、动画交还及Native内部追踪发布接线仍未完成。

同目录纯Python CCTRL001 decoder最终固定为4698aa2f…ff03，绑定实际布局的原始文件与canonical内容hash，即使传入已准备的可变Layout也重新核对。11组CPU／33份保留synthetic二进制及CLI成功／拒绝检查通过；原生failure=okay与额外比较过滤分别记录，完整坏行、float原位bits／非有限分类、参数差异及显式compute→weight关联均保留。最终CPU回执 `parser-v1b/cpu-attempt3/result.json` 为c444c953…c75e，交付回执3c1e1cb0…c09c；首次仅因两个fixture留存文件重名而拒绝的失败原件仍保留。

Root对最终decoder做三项端到端集成：旧bv真实CPOSE数据与明确synthetic控制bytes匹配时，一组compute／weight及完整矩阵逐bits对照通过；不同scene与非有限输出均保留且拒绝比较。七份源码／输入前后稳定，回执 `join-v1b/integration-1792888738648462900/receipt.json` 为f7ee80c9…df39；旧decoder的同类回执另留存。本轮实际Native19测试由Native owner执行，但其在启用前报告控制代码匹配检查拒绝，未派发capture event，也未产生新CCTRL／CPOSE；该失败不能用本节CPU结果代替，正在定位具体guard。离线工具已交Native owner继续使用，尚无新实采或自由双手运行通过结论。
