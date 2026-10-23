# 枪体组件变换的可恢复租约候选

此记录属于 `codex/weapon-6dof`。当前包含纯C++组件及默认关闭的同GHW1枪体适配器。R2两局未进入枪体写入；R3的c局取得一次部分absolute标志更新及交还，完整枪体跟随仍未通过。公开手部或枪体写权限仍未授予，普通启动器未启用。

## R4首失败诊断候选

R4只改三份私有适配源码，保持原100ms、同Plan最终认证、逐setter schema／完整Key／expected检查、原返回值、短路顺序、真实hook转发及unsafe卸载规则。它不修复c的运行根因，仅增加固定首失败和63条有序尾部记录；字段来自原有game读取、原有clock／atomic与授权调用，不额外重采样或续龄。输出为独立gate前缀，原written／pre_restore／end schema不变。

Astra复核修正两项诊断缺口。纯lease组前或组后值不符可能发生在read返回true之后；现仅对ok／known／完整Key相等、group0／forward／已有snapshot／无旧fault的既有读值比较owned，先固定失败，排除setter内部正常旧值及内部／外部回滚。dump同时等待组件depth与dual_draining归零；第三源只在双journal均已尝试、发布状态后增加终端重试，覆盖非Tick动作及clean-ended后立即Shutdown，不依赖下一个Tick。DumpAtExit在旧terminal重复提前返回前构造，不需要改旧terminal标志。

实际函数提取使用合成backend：Auth274项／16case、Backend69项／12case、Drain10项通过。原R3与R4的时钟、Policy、Stop／Shutdown及backend调用序列相同；覆盖Rotation返回成功但无效果、超过63行的两组清理、次生回滚认证失败、组前foreign、false有／无写、skip组与正常original清账。这些结果不是实际游戏调用证明。

ON实际发生两次编译尝试。首次在严格W4／WX下因新增SetScope参数original遮蔽现有全局变量而C4459失败，无DLL；首次源码、命令、输入与失败日志保留在on-attempt-1。仅将该参数及其绑定使用改名wanted_original，机械替回可重建Astra已审原文件完整SHA；第二次成功。成功DLL SHA `a8276f74e247454a98a695d57d0a5817524c8f164b8d7e9657a1fa36167c1af8`，实际项目为Level4／WX／EHsc／C++17，23导出与R3相同。default的MSVC预处理输出原字节相同；combined ON／phase1 OFF仅空行及同一个write_until失败return花括号差异，窄归一化后相同，没有重复这两种构建。末次改名在被移除的私有块中，不重复预处理。

报告 `analysis/hand-mesh-first-failure-20261023/report.json` SHA `62c328320c5c8bc7e67b600e0eed750a6cfe84bee6ec377c12f1138fef29475e`；协调者核对742项目源、16vendor、76份支持工件、真实PE／编译项目、旧R3封存及57个c证据文件后按字节应用三源，应用复核SHA `09c77441d00113e9758c87e355477bd0c04ae9b988962e6d4e7478c89eeb206e`。live742与成功staged编译输入全部相同，可复用该DLL；没有应用后重编，构建base为 `17709c39fc62991351eb1261965c6158b4a4091d`，三处staged差异和后续应用commit必须分开记录。common pure lease未改。

R4尚无实际游戏结果。下一局继续购买版、静音／置后／只读输出，独立保留实际失败及清理回执；不能以本诊断CPU／编译通过替代枪体跨Tick运动、最终图像或头显验收。

## R3分类前置候选

R3只修改 `vr_hand_ik_stream_trial.inl` 和 `vr_hand_weapon_mesh_phase1.inl`：共用原46个动作名，只有注册有效、名称读取成功且明确不在该集合中的调用才跳过组件关联查询。GT检查和owned invoke的一次性self token仍在最前；真实PE／CallFunction不消费token，名称未知保留原处理，InvManager切槽／完整八对象关联、失败双drain和转发前撤权均不减少。未缓存跨回调的关联结果，也未改Policy／WorldBridge／Session／100ms或任何同Plan最后写入门。

Astra逐集合及两文件差异复核无阻断。实际提取入口的合成backend回归548项通过；机械换回R2两段admission后，“被动getter不产生fallback属性查询”的断言失败，rc1。staged ON Release只构建一次，实际项目为Level4／WX／EHsc／C++17，23导出与R2相同，DLL SHA `64956c118f5b36131f664016f8df8f81e2c68b4b48c4c69731580a3114c47689`。default的实际MSVC预处理输出原字节相同；combined ON／phase1 OFF只有空行及同一个失败return的花括号差异，明确窄归一化后相同，没有重复OFF/default构建。

协调者核对报告、实际742项目源及16 vendor、25份支持工件、原live742和旧证据后按字节应用两源，应用后live742全部匹配真实staged编译输入；因此可复用该DLL，未再编译相同输入。报告 `analysis/hand-mesh-passive-classification-20261023/report.json` SHA `bfcecb70a96ca565a0b79db601eaa2afdab679e39007546609ff05aa35b9e94c`，应用复核 `coordinator-apply-review.json` SHA `1d94db80ffa0e06fd92b878fb1d39089a913237e32bb00c018122d5d7a002515`。构建时base为 `a8532ce12724a972730b0f56e83aca095b607fd6`，当时两份staged源与base不同；后续提交／封存必须保留这一来源事实。

R3封存目录为 `analysis/hand-mesh-phase1-r3-candidates/7e027f31…12bccdf/`，source.json SHA `cf760058406f7ae935b6146b28c521a957c5886a7bc876828bb3c3d06d488550`；742源／146工件，Native／XR／loader原字节继承R2。manifest分开记录旧构建base、两份staged差异和后续应用commit，未把它改写成新commit上的live build。

购买版置后静音c局PID8120以Popen0准备 `c815cc19…85a0570` 启动。Start0，首次authorize和postflush已通过，产生一张planned pose，之后 `component_acquire_failed`；没有手部written样本或完成lease。组件终态为acquired0／restored1／setter_calls2／faulted1，最终dirty／owned／revoked均0、safe1。setter_calls在实际调用前递增，还混合forward、内部fail回滚和外层null rollback尝试，不能解释成“两个setter成功”。

唯一component pre_restore记录dirty=owned=1，absolute逻辑标志由000变为110，Translation／Rotation的12字节仍与原值相同；外层交还后，独立raw快照确认三组原值和九个对象Key均与基线一致。这支持本次部分absolute更新后的交还，不证明三个setter完整执行或跨Tick跟随。日志中没有本局related action／yield打断，当前仍缺失败gate与时刻，不能在超时、expected差异或call结果之间唯一归因。

Stop／Shutdown均0，原8控件／44附件／两PG入口保持；FPP累计61，Native GHW1发布50。9个会话的静音守护read／mute错误与pending均0，精确游戏退出后Guard exit0，独立CIM未见owner／game／XR／Guard残留。协调者21项只读收尾通过，scope=`partial_component_acquire_failure_and_rollback`；原component／whole=false及Native／XR画面失败均保留。

原c receipt SHA `be31b13bd4c9ca8c217ed0a74fe01317f5be45478a42643de6dc2e729adcd367`、log `9eb88756cfb6b93271d86a3f52a1399de53b6c7c91bb7628e151fab50e9faaf0`、独立摘要 `2944a33ddf3f3f7423d2bd5b88e1fc43f04951852710ab1ef5d9dbb343b19edf`。证据位于ignored `analysis/hand-mesh-runtime-1023c/`，54文件原字节清单另存；a／b全部原证据不改。下一步仅记录首个组件失败gate、既有时钟和已知比较结果，不增加写权限、重读、续龄或扩大100ms。

## R2购买版测试结果

两次均使用原封存 DLL `25c9b9ed…7086ce2` 和742份源码，强制 Mute1／Behind1／ObserveOnly1，物理手柄后台输入关闭。每局有独立目录和一次性入口，没有覆盖失败回执或自动重复原版拾取／装备调用。

| 运行 | 已取得的证据 | 中止位置 |
|---|---|---|
| a，PID17996 | 原版 GiveTo／SetCurrentWeapon 各完成一次，原枪 Lancer 已装备 | 独立读取器额外查询不存在的 `Weapon.WeaponMesh`；第九个 asset Key 及 setter schema 尚未读取，未执行 Start 或启动 Native／XR 会话 |
| b，PID27712 | 真正 `Weapon.Mesh`、八条 links、九个 Key、资源 `COGAssaultRifle`，以及原组件三组和三个 setter schema 的完整独立基线 | Start返回0；38次 prewrite 后未取得最后写入权限，最终 WorldBridge read rejected，以 `xr_certified_pose_rejected` 结束；手与枪均0 leases／0 setter calls |

b的 `SetAbsolute` 参数为三个4字节 Bool、偏移0／4／8、flags `0x90`；`SetTranslation`／`SetRotation` 各为一个12字节 Struct、flags `0x80`，三个参数区均12字节、无返回值。独立脚本重新解码保存的函数头／参数元数据和身份数值，并核对基线、Start前、Shutdown后三份快照；这不证明 setter 被实际调用。b的原组件三组值保持，不能用零写入清理代替写后恢复测试。

a 的读取器修补只在新b副本删除多余字段，真实游戏适配器一直使用正确的 `Weapon.Mesh`，无须为此重编DLL。此前另修正了读取器的bound receiver验证与runner/audit范围匹配；原版本和回执均保留。b的 helper／runner／audit CPU检查为38／35／56项，尚不能代替枪体运行结果。

两局 Shutdown均0，原8控件／44字节附件／两PG入口保持；b另核对完整组件身份和三组原值。静音守护各匹配9个专属会话，未发生静音写入或恢复写入，read_failed／mute_failed／restore_pending均0，精确游戏退出后Guard exit0；独立CIM未见自有owner／game／XR／Guard残留。a／b协调者独立收尾分别20／19项通过，scope分别为 `pretrial_baseline_failure_and_cleanup` 与 `actual_lancer_schema_zero_write_rejection_and_cleanup`；原component／whole均false，b的Native entry verifier false和XR exit1保留。

原始a／b receipt SHA分别 `26cc48523649edd093ecb0420221493e05e40e60f62764a333872ec6f341ceec`／`9e894c074468aa3cb014908a3f2be78b44a83a918ec5a98a5b96b4e071ef6775`，独立摘要分别 `cd6e54c2074e9a3ed0256c7e5fbc02f1299e17d448a21e7ab0901e04eea3bfbf`／`22dfb6e4429e6f08d55e36958a9bf75b0bc999a1795371d8d3e83c5317c60e39`。全部原始证据留在ignored `analysis/hand-mesh-runtime-1023a/` 和 `…1023b/`；两目录另有48／54文件原字节清单。

下一步只缩减新增hook在确认函数名之前对被动getter反复做的组件关联读取。Policy／WorldBridge／Session与C1的源码哈希相同；不能把37次binding_age诊断误认成撤销绑定，38次prewrite也不是“没有Plan”。b没有记录最终read rejected的分支或包时刻，因此不能确定其唯一原因。两局武器／非空槽数量不同，分段耗时只支持热点假设，不能作同条件性能对比。100ms、动作名集合、InvManager边界、同Plan最后授权及双journal交还均须保留，再用新候选实测。

## 同GHW1 phase1 Lancer适配器候选

新增 `GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL`，要求certified＋combined，排斥旧mesh-follow／magazine writer。当前只处理固定Lancer实例、phase1右手单握；phase0／phase2不写枪体，左手副握和射击仍在后续范围。它消费与手部同一张Plan：完整八对象与资源身份、四槽及expected字段重验后取得组件租约，再重新读取手部状态和组件owned值，最后认证同一Plan才写IK；三个setter也各自重验实际实例Args schema和最终授权。恢复不依赖新鲜姿态或phase1，不把trial／stow bookkeeping epoch当对象身份。

恢复顺序为手IK、组件，再允许装备附件助手；两个journal均尝试，首个失败不会跳过第二个。恢复入口位于active／ended门之前，Stop不再把status8视为已清，每次drain重新发布safe_to_unload。纯lease增加 `relinquish()`：真实原动作必须转发而未能交还时，永久撤销pending组写权并保留dirty，后来只能在同Key／精确原值下只读清账；自有相关Args请求则可在未清时拒绝。自身setter使用一次性的精确object／UFunction token，真实递归hook保持原ABI和边界观察。

Astra复核发现初版只关联Weapon.Mesh，遗漏phase1／stow clean时由InvManager发起的SetWeaponFromSlot。R2已关联冻结完整八对象，并在建Key前由已认证Pawn取得InvManager；实际提取的关联入口回归覆盖该动作先交还、失败时自有请求拒绝／真实hook先撤权再转发。Astra最小复核确认缺口闭合。旧R1源码／报告／DLL保持原字节，机械替回旧关联实现的反例被新回归检出。

staged ON／OFF／默认三构建及278项pure检查通过，实际提取生命周期41项检查通过；这些检查使用合成backend，不是游戏结果。R2报告 `analysis/hand-mesh-component-adapter-20261023/report-r2.json` SHA `fb0f1d2df07445d6c0dea30552302a312bb98655dcf43f5116048e0805df5e48`。协调者另核对7份源码、8份回执记录、R1工件及16份独立vendor来源后按字节接入独立分支。live新目录只构建一次ON候选，23个导出与C1一致，四种错误组合均在配置阶段拒绝；live报告 `analysis/hand-mesh-phase1-live-20261023/report.json` SHA `e32fb274e778be42eb26cbccf816e44a158094a7483fb2d5404bdf7efbf2d917`，DLL `25c9b9edf8aeadf5eac959c8eaadc08bbef6a731306e6bab3121e1f567086ce2`。742份源before／after不变，没有重跑已通过的CPU／OFF／默认构建或Native／XR，也未替换另一会话的默认文件。

后续每局仍须Start前通过原版pickup准备Lancer，重做完整身份、8控件、附件44字节、组件三组和真实SkeletalMeshComponent的setter schema基线，不能复用b的实例地址。日志接口提供同lease／证书的written与pre_restore枪矩阵及全局journal计数；当前尚无实际枪体写入／跨Tick位置、完整旋转或写后恢复结果。同步setter读回不是最终可见跟随证明，C1的双手通过也不能作为本枪体候选验收。

旧 `vr_weapon_mesh_follow.inl` 会修改 `Weapon.Mesh` 的absolute标志、Translation及Rotation；stow journal则管理人物Mesh.Attachments中的44字节附件姿态。两个位置不是同一份状态。h的双手与附件恢复通过，不能证明旧mesh-follow的组件变换也恢复；旧adapter的地址存活检查也不能证明完整对象代次。因此暂不解除combined与旧mesh-follow的互斥。

## 接口与边界

`tools/vr-common/weapon_mesh_component_lease.h` 提供 `WeaponMeshComponentLease<Key>`，操作序列为 `acquire(expected_key, wanted, backend)`、`restore(backend)`，以及仅clean、非重入时显式 `reset()`。它精确保存三组字段：一次SetAbsolute所使用的三个flags整体、Translation原始12字节、Rotation原始12字节。Absolute不是三个可分别重写的字段，以免恢复某个flag时覆盖同次调用中的foreign flag。

backend必须用完整对象代次及调用上下文认证Key；每个setter在实际调用前再次核对Key和完整expected Value，只能改变自己的组，必须以bool报告错误。false仍可能已经写入，journal在调用前记pending，调用后重新认证读回。它不自行验证游戏线程、动作、GHW1或源时间；未来adapter须在这些字段读写之外提供当前姿态与动作授权。backend若会抛异常、改变未声明的其它字段，或无法进行写前重验，不能直接接入。

组内撕裂或foreign值会撤销该组写权、保留dirty；其它仍owned组可以独立恢复，不覆盖foreign组。Key未知、地址复用、known=false或读失败会撤销全部剩余写权；旧owned值后来重现也不会重新授予写权。撤权组只有在同完整Key下只读观察到精确original时才能清账。`restore()==true` 只表示journaled组已清，不证明never-owned的foreign字段，也不清除sticky fault。父session遇任意fault必须结束，不能用reset静默重开。不可观察的foreign写入后又改回的ABA仍不在本组件证明范围内。

## CPU验证与审阅

独立VS18 x64 Release以 `/W4 /WX /EHsc /UNDEBUG` 构建，修正版268项断言检查和CTest1/1通过，涵盖false前后产生效果、无效成功返回、组内撕裂、foreign组独立恢复、身份丢失、地址复用、写前漂移、重入及精确字节恢复。Astra发现初版在复制wanted前清理自身journal，导致把 `owned_value()` 或 `original_value()` 传回acquire时目标变零；现已先复制desired再清理。新增两getter自别名回归使用不同的非零新backend基线，核对目标、journal及最终原值；在ignored副本恢复旧顺序后，两条回归分别rc1捕获旧bug。Astra再次只读复核问题闭合。

修正版header SHA256 `7116b49ac7442a3288f06083313ba3bbf78762eda6fa92e3ce10294748588bdd`，test SHA256 `7ad9ef71675b16c68453be24d1f5157d8317fd74905f31bd32d11643bb6f3a94`。私有回执 `analysis/weapon-mesh-component-lease-20261023/getter-alias-fix/verification.json`，SHA256 `c3454567b689f8ad65d0e6b43a2b396b78befdf82e50ee11116fff0b5295154d`。初版256检查的72个历史文件及原回执保留，没有把修正结果覆盖到初版。

项目CMake另加 `WeaponMeshComponentLeaseTest` 两行目标定义，在全新目录重建持续ON DLL和该测试目标；flags与C1 trial逐项相同，新目标只执行一次，268 checks通过。9份源码before／after快照及10份记录hash已独立核对，旧C1三DLL和旧报告未变；没有重跑旧CPU或OFF／default构建。集成报告为 `analysis/hand-continuous-session-integrated-20261023/report.json`，SHA256 `ac7f10c2c2a6c386b343b9730ccc769d9be9f59b970fe2a0b72bb4efe70b4f59`；当前CMake `de450edc…` 机械移除新增目标两行可精确重构旧 `86dd65f2…`。新持续候选DLL为 `425f6868aef6d05b24ce153172386c8bb15f658348ad6950f4867f23443e8a0a`；它尚未跑游戏，不替换既有h验收身份。

纯组件阶段确定的最小运行切片是固定Lancer实例、phase1右手枪体：消费与handstream同一张认证GHW1票据，先取得组件租约再重新认证手部写入；原Tick、动作和stow helper前必须同步交还手部及组件两个journal。phase0／稳定phase2不写枪体。当前adapter已实现并进入上述R2测试，实际SetAbsolute／Translation／Rotation行为及跨Tick跟随仍待证明；左手副握、射线和正式武器切换尚未接入，CPU检查不能作为游戏／GPU／头显验收。
