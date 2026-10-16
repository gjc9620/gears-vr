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

## 第二批：已验证 live prepare，尚未接入 commit

2026-10-16新增，不覆盖上段的历史测试范围：

- `world_rebind.py`复用完整相机、anchor/UI/cinematic builder，保存不可变候选，绑定game/owner的PID与creation、稳定LocalPlayer六字段及连续world generation。新路径独占发布UTF-16包；文件存在本身不是请求。
- 启用Hook后，仅一次封存自有入口的原32字节、补丁32字节和relay14字节。严格验证普通x64 E9→FF25→指定本DLL detour、原入口后27字节一致、前后复读；不支持的形式拒绝证明。owner另行RPM复核、核对已加载DLL路径/base和预期文件SHA；前后重验同一manifest。原签名常量不变，未登记代码读取不补写，BOOL_CODE全函数哈希仍读实际内存。
- UObject、registry、属性、脚本和link检查始终读实际内存；只有明确的入口检查使用认证原字节，未修改通用Resolver.read/safe_copy。native候选UI验证也将数据检查与四个入口检查分开。
- `WorldRebindPrepare`事件让native worker读取固定下一代新路径；读取期间持有拒绝写入/删除的文件句柄。核验进程、owner、代次、module、LocalPlayer及两套证书的controller/camera/pawn关系，通过后仅保存pending。不会替换live anchor/UI、关闭Lifetime、推进world epoch或改变OpenXR。
- `frame_link.advance_world_epoch`保留真实Create/Reset世代及容量故障，拒绝旧代list和迟到票据；该接口尚未由live恢复流程调用。
- GPU协议升至ABI7，新增world generation/quiescing/minimum sample。begin阻止新发布，commit要求同锁检查生产完成及消费ACK；资源generation与serial保持。XR每帧poll，quiescing清显示资格并通过原receiver只acquire/ACK；已有GPU工作和XR ownership保留至原完成路径。native尚未调用begin/commit。ABI7构建必须搭配ABI7 probe，默认冻结manifest未切换。

实际游戏测试：PID15180 / creation134365677951377851，owner39208 / creation134365677886315103；`continuous-stereo-15180-20261016-035647-89ba8802.json`。Native WorldRecovery2 SHA `124949f9015a86568c211d23bc33f9d6e0bf69875f42183aca2d9014122139b7`；配套probe SHA `7af3b8a2ed38b0c76c69fbc224745e2142e45960547c54c9bf02300068c238a0`。保留冻结目录，未覆盖Native9/XR7。

游戏关卡内保持Hook开启，owner在约3.14秒内通过原20处入口签名、42个封存入口及完整anchor/UI/cinematic证书，生成generation2包。native日志实际记录 `world_rebind_prepare accepted=1 pending=1 generation=1 next_generation=2 committed=0 live_config_changed=0`。本轮对象未替换，不能把这一结果当成新世界已恢复。ignored证据：`world-recovery2-prepare-receipt.json`、构建目录的`hook-entry-manifest.json/world-rebind-2.ini/capture-status.txt`。

随后35秒状态观察未采到0 layer，保持gameplay/projection；这是有界采样，不证明不存在短暂闪烁。停止状态：pending0、release/destructor均1、active0、drain/close/entries_restored均1、resources_retained0、cleanup_timeout0，XR退出0；之后身份校验WM_CLOSE正常关闭游戏，没有请求前台焦点。证据`world-recovery2-passive.json`和`world-recovery2-close.json`。未进行RenderDoc或真头显测试。

新增接口测试：HookEntryAttestationTest 80检查，FrameLinkTest 17,695检查，Metadata身份门禁、Anchor loader 124检查、Lifetime/Gate及DLL continuous/frame-link自测通过；OwnedGpuShareTest三种idle实际GPU handoff与两眼像素通过。Python相关53项测试通过，包含入口读取期间补丁改变、默认启动及真实UI模拟fixture。GPU debug layer仍不可用。

最终源码另加入session-status中的world generation、candidate generation、prepare请求/拒绝计数，便于owner在不停止游戏的情况下读取prepare回执；该增量在独立WorldRecovery3构建验证，未冒充本轮WorldRecovery2实际游戏证据。

下一步仍是接线：通过非等待Scope admission阻止配置交换与在途旧回调重叠；原生线程完成旧renderer/ViewState退休；GPU quiesce/ACK排空；一次提交新anchor/UI/相机与world generation并退休旧UI/票据；再由新代已完成图像恢复projection。主菜单的独立认证画面源同样仍缺。不能把当前pending接收、GPU接口测试或同关卡运行当成完整主菜单→继续游戏修复。

## 第三批：实际退休、提交与跨世界恢复通过，自动调度未接入

WorldRecovery5（未提升默认）Native SHA `a98aab5665022ec02cfa28b39358ae160f57666f947c82749f46dd61510c272d`，配套probe仍为上一节的ABI7。游戏PID38312 / creation134365691575052088，owner43332 / creation134365691509830360，XR PID4968。会话 `continuous-stereo-38312-20261016-041929-31851f18.json`。WorldRecovery4仅构建/CPU验证，未用作本节live证据；WorldRecovery5实际启动后保持产物冻结。

实现边界：

- Scope使用线程自有RecursiveAdmission，外层持lease，嵌套继承成功/拒绝。普通open读者CAS争用重试，不把争用当作丢弃instrumentation的理由；看到closed立即拒绝，不等待writer。world敏感钩子拒入后透传原调用。GPU拒入在frame锁下标记unknown并覆盖原调用，使其要么先于commit完成，要么污染commit后的窗口。factory、descriptor和resize等不读取world配置的原有观测保留自身锁与资源握手。
- worker请求GPU quiesce并阻止新pair；scene/family及原有native停止服务边界执行可恢复Lifetime关闭。只有原生线程取得release claim，最后renderer原析构返回后退休，release前CAS记录started。提交要求旧renderer pending0、无retained sources、真实state release返回及deleting destructor完成；未创建state须完全未尝试创建。不能用清容器模拟释放。
- writer只在零admitted读者时try-close，再try-lock原native/frame/stereo锁；短提交内不做IO、engine调用、GPU等待。重新验证候选，只有精确timestamp-only变化允许暂态重试；失效候选清除并留在安全quiesce，允许新请求重取。每个候选请求单独获得Commit授权，旧授权不能沿用。新包文件名含game creation、下一代及请求序号，拒绝覆盖旧证据。owned实验模式在prepare前拒绝，避免进入不支持的永久等待。
- GpuPublisher在同一idle锁下执行有界native提交callback，成功后才向XR公布新world；资源generation/serial及XR session/pose时钟不重置。旧tickets、command-list关联和UI资格退休，新ViewState由factory重新创建；累计诊断与本代submit计数分开。TLS仅由所属线程下一次新world外层入口清除，anchor序列保持单调。绝对试验期限不因重建延长。
- 旧Material及payload保留拒绝墓碑，旧Execute消费后才释放key，binding tokens全退休；已借用Claim不被清空。尚无allocation epoch证据时，不主动解除永不消费的旧墓碑，理论上存在有界槽占满风险；本轮payload末态active/poisoned/capacity_rejected均0，不能推广为无限次世界切换安全。

实际测试按顺序完成：

1. 初次准备时角色已死亡，Pawn为null，完整builder正确拒绝；通过已认证死亡菜单重读检查点并暂停后再测，没有修改认证规则。
2. 同一关卡：实际prepare/commit将generation1→2、commits1，约4.84秒含完整证书重取后恢复新pair；本代submit20、published1406时确认成功，随后持续增长。证据 `world5-sameworld-success.json`。
3. 在旧世界先prepare generation3候选，退出到主菜单后才请求Commit。native实际完成旧state释放/析构与GPU排空后拒绝失效候选：generation仍2、phase4、candidate0、closed1、pending0、release/destroyed均1、publisher_stopped0。证据 `world5-old-world-candidate.json`、`world5-stale-reject.json`。
4. 同一进程从主菜单三次已认证确认进入继续游戏，再暂停。controller从`0x8cfd2ae0`换为`0x3f68f5580`，camera从`0x93d1b380`换为`0x219f60a0`。重新prepare request3/generation3；先观察至少2秒证明Prepare不自动提交，仍保持generation2/phase4/commits1，然后明确Commit。约5.27秒后generation3、commits2、本代submit16、published7023。证据 `world5-new-world-candidate.json`、`world5-new-world-success.json`及相应菜单动作记录。
5. Resume实际游戏后，模拟器projectionSubmissions从2814增至3478、consecutiveMissingProjection0、layer1；当前新场景左右均值约0.335/0.285。只查看自动incident中一个320×90 preview，确认两眼均有场景；这不是头显舒适度或全部UI验收。自动检测另报15次亮点不对称（最长13采样），仍需后续时序分析，不能宣布所有闪烁已消失。暂停共同quad不计作projection；20秒暂停观察无0 layer，但同样只是有界采样。
6. 正常停止：generation3、commits2、published15905，pending/active/retained0，release返回/destructor/flags、drain/close/entries_restored均1，resources_retained0、cleanup_timeout0；XR退出0。身份校验后WM_CLOSE正常关闭游戏，未请求前台焦点。证据 `world-recovery5-close.json` 与最终session/capture状态。

测试：Native5启动按现有完整reserve/self-test链执行，continuous自测新增closed GPU Execute仍须等待frame锁的反例；连续停止及frame-link自测通过。RecursiveAdmission覆盖继承拒绝、独立TLS、深度/foreign gate和无writer时八线程全部进入；UI retirement单测通过；FrameLinkTest17,700检查通过；实际GPU三种idle覆盖native callback拒绝时world保持quiescing。debug layer与真头显验收仍缺。

当前剩余的直接工作是owner自动检测真实对象换代、后台重取证书、等待prepare回执并发起对应Commit，以及失败重试/取消和长期循环测试。主菜单、死亡/无有效anchor时的独立画面源也仍须完成。上述显式触发路径与安全交接已经实际运行，但第一版整体、自动恢复和全部黑屏问题仍未完成。

收尾时保留graphics helper的原独立策略，closed窗口也不能绕过已有DLSS锁。它们不读world配置；策略故障造成的并发永久stop按终止竞争处理：Lifetime拒绝reopen或禁止下一pair，提交不恢复session stop状态。此增量在独立WorldRecovery6构建及continuous/frame-link/Lifetime自测通过，Native SHA `a0d31de8c7bbcd1954144d0ce8e68d8657f7f71991187c6e7d6435427b56435b`；未将Native5 live测试冒充Native6的运行证据。后续自动恢复联调采用新私有构建，默认冻结入口继续保持。

## 第四批：owner自动调度与两轮真实世界切换通过

WorldRecovery6保留上一节SHA，配套ABI7 probe SHA仍为`7af3b8a2ed38b0c76c69fbc224745e2142e45960547c54c9bf02300068c238a0`。本轮使用ignored `analysis/runtime/world-recovery6-auto-manifest.json` 显式启用`world_recovery`；默认manifest未修改。游戏PID29076 / creation134365706165849066，owner36996 / creation134365706101596446，XR29296。会话`continuous-stereo-29076-20261016-044349-775fe236.json`。

实现：

- `world_rebind_monitor.py`使用一个异步只读worker，不在owner菜单轮询中等待证书。稳定LocalPlayer保持一致，必须有当前可认证Pawn；controller/camera/level/WorldInfo任一完整身份变化才准备新包。单纯Pawn换代继续使用已有native同世界重新关联路径。死亡、null Pawn、拒绝计数不构成世界重建授权。native缺少supported字段时不工作。
- `world_rebind_runtime.py`复用原Resolver及完整anchor/UI/cinematic builder，关闭所有RPM/attestation句柄；原入口签名改为共享函数，常量不变。expected DLL SHA来自已验证启动manifest。包名绑定game creation、下一代及native请求计数，独占发布。
- owner检查完整进程身份、started_qpc和native回执，只对自己的request发送Prepare/Commit。成功发送的Prepare不盲目重发；超时保留事务以核对迟到回执。发布后必须返回receipt，取消不能丢失已存在包。accepted Prepare随后Commit，最终新鲜度由native实际退休之后验证；phase4允许重新认证当前世界，即使等于旧baseline。无法证明归属的冲突停止自动调度并保留证据，不猜测代次或覆盖包。
- 可选manifest/CLI入口要求anchor/UI/cinematic及可信native SHA；清理先取消并join worker，再走原native停止/入口恢复和XR退出。自动调度未加入输入或窗口激活动作。测试菜单动作仍按既有精确状态认证后PostMessage，不请求焦点。

实际测试：初次双目运行后暂停，经已认证退出流程返回主菜单，再以既有三次确认继续游戏；自动request1将generation1→2。再次完整退出/继续，自动request2将generation2→3。两轮没有手工发送WorldRebind事件，也未重启owner/game/XR。controller依次为`0x8df92ae0`、`0x24e92ae0`、`0x1617dd2ae0`；camera依次为`0x124ef6d60`、`0x3e368a6c0`、`0x152542740`。

日志Prepare→generation提交的owner观察耗时约3.04秒、3.29秒，不含此前扫描、加载及完整证书获取。第二轮本代submit至少241；模拟器projectionSubmissions从2710增至3750，consecutiveMissingProjection0、layer1，两眼均值约0.345/0.307。证据为会话`.world.jsonl`、`world6-main-continue{,2}.jsonl`、`world6-{confirm-return,confirm2-return,new-world-pause,second-world-pause}.json`、`world6-xr-cycles.json`与构建目录两份世界包。

最终正常停止：generation3、commits2、published7602、本代submit1572，pending/active/retained0，release/destructor/flags、drain/close/entries_restored均1，resources_retained0、cleanup_timeout0；会话passed、native_cleanup_verified、entry_restore_verified均true，XR exit0，无worker清理异常。身份校验WM_CLOSE正常退出游戏，证据`world-recovery6-close.json`。

测试：native-capture相关Python103项通过，含自动调度16项、生产adapter4项、启用前依赖与SHA门禁；launcher40项通过并检查manifest SHA传递。Astra Medium定向复查事务归属、超时与迟到回执；静态审查不当作runtime验收。

仍缺主菜单/死亡/无有效anchor的独立正确图像源、全部黑屏/CG/闪烁覆盖、全部UI/字幕/准星和真头显验收、长期多世界循环及发布默认提升。自动亮点不对称累计248次、最长29采样，不能宣称闪烁已解决；主菜单/加载期0 layer和暂停非projection不作为世界projection失败。当前定期只读扫描复用完整Resolver，后续可降低稳定场景开销，实际提交证书不能放宽。

## 第五批：主菜单使用独立窗口镜像面板

新增私有XR MainMenu8，SHA `22053c9590f570b86a09d49fb73a6e898272febdc54f1c42d766f8b2336b790f`，配套冻结Native6。ignored清单`analysis/runtime/main-menu8-manifest.json`；没有提升仓库默认。

此前拟新增native mono图像认证。此次采用明确的完整客户端窗口镜像合约：只对正面认证的主菜单状态使用WGC，作为双眼共同2D quad；不猜测原生mono、不由缺失双目票据推导画面布局，也不计作stereo/projection。若实际窗口仍有SBS污染或不可读菜单，仍算失败。现有唯一native GPU receiver继续poll、ACK及资源退休，不增加争用消费者。

owner Mode4只认证DeltaMainMenu，以及已有实际证据的相邻有序MainMenu→CampaignType→ContinueGame链。要求完整top/stack身份匹配、只读一致性及player0；最终源码允许这些精确状态在整数block0/1时显示，输入控制仍要求block0。其他主菜单子页面和未知状态不放行。每次进入/重新进入清旧WGC邮箱，捕获时间须严格晚于新epoch且不超过一秒；进程creation、HWND、客户端裁剪、已完成GPU上传及XR swapchain所有权保持原有检查。池或客户端尺寸变化立即推进source generation并清资格，等待新匹配帧；上传等待后再次验证owner租约，发现变化立即观察并退休旧模式，避免ABA重入漏掉epoch。

MainMenu8实际试验A：游戏37608 / creation134365718566835821，owner36768 / creation134365718504571872，XR7620；会话`continuous-stereo-37608-20261016-050429-d592c665.json`。退出至主菜单后first_frame的capture_epoch100ns=384099998181、captured100ns=384100383472，尺寸1920×1080，xrEndFrame成功。保存640px实际WGC源及一次模拟器共同面板图像检查：完整菜单，无持续SBS污染；准确键盘状态记录确认Campaign→Versus→Campaign及进入两层战役选择。最终主菜单quad累计21648，不当作世界projection。

A另做最小化/不激活恢复，layer0→layer1。尺寸试验工具最初处于DPI虚拟化环境，所谓逻辑1600×900实际使backbuffer变为3200×1801，逻辑原尺寸恢复实际为1920×1081；已改用DPI aware读回并恢复1920×1080。WGC最终恢复菜单，但之后新world代次2虽提交、场景计数增长，GPU发布仍停在菜单前1709，XR投影未恢复。Astra定向复核确认：最后成功GPU发布QPC384087046098早于菜单384099997336，不能把累计published或新epoch_submit当作新GPU图像。capture_attempts/linked/ticket_retains同样未再前进；bad_quad1827缺少时序，尚不能直接指认根因。

A停止：native_cleanup_verified、entry_restore_verified均true，XR资源销毁/shutdown完成，但XR exit1、会话incomplete，不能写成全流程通过。随后B不再resize仍以1081高度启动且无第一张GPU发布；查实游戏把StartupResolutionY=1081持久化到用户配置。正常停止/关闭B后，完整备份到ignored `mainmenu8-before-resolution-restore.ini`，仅恢复该值1080，保留其他用户设置。A/B关闭证据`mainmenu8-close.json`、`mainmenu8b-close.json`。这不是奇数高度问题已修复。

最小下一项native实验：在classify_final_eye完整半屏覆盖和observe_frame_draw目标尺寸相等门禁处记录有界首批拒绝，包含world/resource代次、quad caller/矩形、RTV/实际buffer/final_eye尺寸和ticket。精确physical1920×1080→1920×1081→1920×1080对照；先要求linked/capture_attempts/published推进，再判断XR。不得放宽完整画面门禁或继续盲改纹理参数。

源码测试：native-capture相关Python110项通过，含菜单29项及真实状态fixture；MainMenu8的StartupPanelPolicyTest、MenuDisplayStateTest、MenuCaptureEpochTest、CaptureFrameMailboxTest、StartupCaptureSizeTest分别退出0。图像证据、状态和异常均留在ignored analysis；未知菜单/加载、全部CG/UI、任意窗口尺寸、长期循环和真头显验收仍缺。

恢复1080p后的C/D同进程对照：游戏44016 / creation134365725804385987，owner36360 / creation134365725741140634，XR42020；会话`continuous-stereo-44016-20261016-051632-ed74bfd9.json`。加载最终block0/1分类器，不resize，连续两轮Pause→MainMenu→CampaignType→ContinueGame→gameplay→Pause，自动world1→2→3。每轮均有新GPU发布推进和成功的`menu_display_recovered`/ticketed projection handoff，epoch分别2、4；第二轮恢复时published4335。owner日志在这些已认证菜单转换中维持Mode4，没有此前仅block1触发的UNKNOWN；仍有其他场景读取SnapshotChanged暂态，不代表所有闪烁解决。

C/D证据`mainmenu8{c,d}-cycle.json`、`mainmenu8-two-cycles-evidence.json`及完整menu/stdout/world日志。最终main_menu_quad_layers2496，菜单recoveries2；native_cleanup_verified、entry_restore_verified均true，XR exit0、会话stopped/passed；`mainmenu8c-close.json`记录身份校验后正常关闭游戏。退出后确认用户配置StartupResolutionX/Y仍为1920/1080。此次正常路径通过不能覆盖A/B已复现的1081尺寸失败，也不等于所有菜单和真头显验收。

## 第六批：定位尺寸变化后的图像关联拒绝点

私有Native7 SHA `bae42b29e373de6e0d534ad7681cc2f5117f2adc799f411b0eb818c8e85ad8e3`，继续使用XR MainMenu8；默认入口未提升。新增诊断只记录既有门禁的拒绝，不改变图像接受策略、Hook入口或资源所有权。32行总上限，每个world/resize/inventory epoch最多四种正ticket失败几何；无ticket另限一行，并为正ticket保留容量。相同几何合并计数及首末时间/ticket，避免逐帧日志。

会话`continuous-stereo-31028-20261016-053718-1d6d0c2f.json`，game31028 / creation134365738258728593，owner39884 / creation134365738195811210。使用DPI-aware物理尺寸确认1920×1080→1920×1081→1920×1080，再实际MainMenu→Continue切换至world2。baseline published220/projection480；变化后published274/projection550，恢复客户端尺寸及世界切换后仍停在该值，XR layer0；原生场景提交继续增长。

实际拒绝为stage2/reason512 `not_known_buffer`：带正ticket的左右quad有效，矩形分别(0,0,960,1080)/(960,0,960,1080)，RTV资源`0x3db5906a0`、尺寸1920×1080、单target/mip0/format24；当时缓存的实际Present尺寸1920×1081。world1和world2均复现，最终10行、dropped0。证据`world7-rect-experiment.json`及会话`.native-status.txt`。这是中间RTV不属于交换链后缓冲而被拒绝的运行证据；不证明该资源像素正确、后续缩放传递成立、viewport/scissor验证通过，亦不证明所有黑屏共有此根因。后面的验证器尚未执行，不能把有效quad等同完整GPU认证。

下一步先定向追踪中间双目纹理到真实backbuffer的后续传递，记录源身份、映射、实际提交顺序和UI位置。若可证明完整无污染传递，可保留既有backbuffer发布与生命周期；不能直接移除known-buffer门禁，也不能伪造两次draw_final跨资源复用同ticket。直接采用中间资源则另需状态、生命周期、替换和UI证据。既有RenderDoc游戏注入失败路线不重复盲试。

FinalEyeDiagnosticsTest、SubmittedViewLifetimeTest、WorldEpochGateTest及FrameLinkTest（17,700检查）通过；continuous/frame-link/quad自测通过。最终native_cleanup_verified、entry_restore_verified均true，XR shutdown与资源销毁完成，但显示未恢复、XR exit1，会话incomplete；不是全流程通过。身份核验后正常WM_CLOSE退出游戏，没有获取前台焦点。退出后备份配置为`world7-config-after-test.ini`，仅将持久化的StartupResolutionY从1081恢复1080。未进行真头显或GPU抓帧验收。

## 第七批：后处理候选未执行，目标身份需核对描述符来源

Native8 SHA `5658892fd2ff84d4b82584b23fe10840c267ad40d690c4852472b275083647f5`，XR仍为MainMenu8。增加已有quad Hook内caller `2b11a0` 的有界观察：每world/inventory八种几何、全局32行，只有新行查询当前引擎持有的输入资源；没有新Hook、渲染策略或接受条件变化。Execute记录明确只是CPU原调用前观察，不计为GPU完成。

会话`continuous-stereo-12940-20261016-055010-0a86a3a8.json`，game12940 / creation134365745978921889，owner33804 / creation134365745915269729。精确物理1920×1080→1081→1080后再回主菜单继续游戏，world1→2自动提交；published219→278后保持，projection251后保持，layer0。此候选caller的calls/rows均0，因而不能按旧VDXR5调用栈推导本轮存在该缩放传递。实时代码范围RVA2b0c04长度1452、2b12c7长度144与历史PID16812 blob匹配；只认证这些范围，不是全部函数或shader语义。证据`world8-live-byte-attestation.json`。

更关键的反证：拒绝记录中的资源`3dfe88dd0`在较早的实际Present outage记录中也出现过。故第六批称其“中间RTV”只可视为当时假设，尚未确证。RTV宽高来自CreateRTV时保存的映射，不是拒绝时重新查询；同地址不能证明同资源生命周期。两次ResizeBuffers均成功并更新当前inventory，后续Present changed0，而map未记录创建/复制epoch。下一项先记录resize请求/实际尺寸、实时buffer描述符及OM所消费的RTV映射来源，再区分旧映射、地址复用与真正的独立渲染目标；暂停传递路径实现。

LateQuadDiagnosticsTest及continuous/quad/input-resource自测通过，完整启动reserve自测链通过。最终native_cleanup_verified、entry_restore_verified及XR shutdown/objects_destroyed均true，但XR exit1、会话incomplete；显示缺口未修复。正常WM_CLOSE关闭游戏并备份`world8-config-after-test.ini`，恢复StartupResolutionY=1080；没有获取前台焦点。未提升默认，也没有真头显或GPU抓帧验收。

## 第八批：实时resize尺寸与RTV来源

Native9 SHA `a89d967280d6bd95e1c628cd663c55dbb3eef6f1d96dab145ca49aaf970c6160`，XR仍为MainMenu8。增加八笔resize实时请求/结果和临时GetBuffer/GetDesc记录，不保留资源引用；RTV映射独立记录创建/复制serial、来源及OM绑定时快照，不改变Target比较或接受策略。descriptor epoch来自缓存Present，明确可能落后于resize；重复复制按创建来源合并，不因递增copy serial占满首批采样。

会话`continuous-stereo-43032-20261016-055824-1cb552b0.json`，game43032 / creation134365750921580875，owner6896 / creation134365750859423196。原对照再次world1→2，published217→266后保持，projection239后保持。两笔实际ResizeBuffers请求均为1920×1081，并非第二笔恢复1080；两笔S_OK，实时buffer GetDesc也均1920×1081。客户端恢复1080不能等同游戏请求或GPU尺寸恢复。两笔新buffer canonical分别为`3ddae0f60/3ddae05d0`，第二笔index交换，不能据相同地址推断跨resize生命周期不变。

被拒绝RTV为`3ddadf2b0`、1920×1080，handle`79d38b0`；来源是观察到的CreateRTV serial369、cached inventory epoch2、live接口`3ddadf2e0`，无copy lineage，之后在inventory3持续用于两眼。说明不是仅沿用启动期创建记录；它在第一次resize后的观察期被实际传给CreateRTV，且不同于两张已核对的1081后缓冲。仍未证明第二次resize后的存活、像素或最终传递。下一步扩展已有CopyTextureRegion观察，绕开旧global-color-pair筛选，针对当前后缓冲或该拒绝目标记录实际调用参数与实时描述符；不得查询已保存的旧COM地址，不新增捕获注入或放宽门禁。

GpuViewportStatsTest、FinalEyeDiagnosticsTest、frame-link/continuous自测及实际reserve链通过。最终native清理、入口恢复、XR shutdown/对象销毁通过，但XR exit1、会话incomplete，不能称显示通过；正常WM_CLOSE退出，备份`world9-config-after-test.ini`后恢复用户StartupResolutionY=1080。原始证据为`world9-rect-experiment.json`、会话native-status及`world9-close.json`。默认清单未改，真头显验收未做。

## 第九批：排除已观察CopyTextureRegion路径，转查真实backbuffer绘制

Native10 SHA `3a47376665306802c4f167f7361143a1dd03b416d57f64ce98da9a267926e03e`，XR仍为MainMenu8。已有CopyTextureRegion Hook在旧consumer/global-color筛选之前增加有界观察，只对当前API实际资源参数查询canonical身份；匹配当前buffer或已有拒绝目标时才在新行查询GetDesc，全部接口在转发前释放。候选ticket只作上下文，Execute仍明确是CPU原调用前记录。无新Hook或接受条件变化。

会话`continuous-stereo-27784-20261016-060633-8a09973b.json`，game27784 / creation134365755813869714，owner36844。相同尺寸及world1→2对照仍失败：published216→280后保持，projection260后保持。新观察total84063、unmatched84063、invalid0、rows0、dropped0；不能推导通过此API将拒绝目标传到当前后缓冲。它只排除已观察的CopyTextureRegion关联，不排除其他API或未知shader路径。

现有outage_sources已有更直接的后续线索：gap0 first_unknown draw在QPC421018854368写入实际当前backbuffer `16033795e0`、1920×1081，caller`1401af92b`、indexed6/instance1，list`160302b090`、generation109262；无ticket或已认证Material。该图元生成函数也用于已知quad，但本轮只筛选历史caller2b11a0的late观察仍为0。因此下一步应把已有quad作用域扩展为“实际当前backbuffer上的未认证quad”，记录真实quad caller及当前绑定输入；不是继续猜新caller或放宽门禁。若没有quad作用域则再查相应直接调用路径。源内容、采样及同帧归属仍须验证。

BackbufferCopyDiagnosticsTest及color-copy/color-copy-system/continuous自测、完整reserve链通过。最终native_cleanup_verified、entry_restore_verified及XR shutdown/objects_destroyed均true，但XR exit1、会话incomplete；显示未通过。正常WM_CLOSE关闭游戏，备份`world10-config-after-test.ini`后恢复StartupResolutionY=1080；证据`world10-rect-experiment.json`、会话native-status、`world10-close.json`。未提升默认或宣布所有黑屏根因。

## 第十批：定位实际后缓冲quad与实时输入身份

Native11 SHA `aaeeba4499732d54128e4e72c91f573f2b5282dfeb11b5424c3aaf46ef4ccca7`，XR仍为MainMenu8。所有已有quad Hook作用域携带真实caller，但只对当前已认证buffer上的未分类quad（及旧历史caller例外）读取TLS/查询输入。8行/epoch、32行总限额和新行才查询资源保持不变，不改变渲染或接受条件。

会话`continuous-stereo-37612-20261016-061336-9309a636.json`，game37612 / creation134365760038375735，owner38716。此次缩短为baseline→物理1081→恢复物理1080，不再重复世界切换。published219→283后保持、projection242后保持；最终脚本多余的“再暂停”因已在DeltaPause页面被状态校验拒绝，未发送该键，不把脚本标作完整通过。尺寸三阶段证据有效，保存于`world11-rect-experiment.json`及stderr。

late row8/9定位真实caller `9a7377`：目标quad(0,0,1920,1081)，source(-0,-0.5,1920,1081)，target dimensions1920×1081、texture dimensions1920×1080；当前backbuffer单RTV、indexed6/instance1、全尺寸viewport。slot0当前绑定资源native接口`164e6a4b70`、canonical`164e6a4b40`，实时GetDesc1920×1080，恰好等于positive-eye拒绝记录中的RTV。首个左眼拒绝QPC425224789976，后缓冲quad425224793269；两者同raw list`164e1a3b70`、generation105995。另一buffer也重复该源。明确找到同一记录窗口的源/目标传递候选，不再是猜测地址或保存的旧COM值；仍不是shader语义、提交完成或GPU像素认证。

下一步只围绕caller9a7377查资料库/具体函数及其采样、shader设置，验证当前版本字节后设计显式source→destination关联；不能用同ticket的两次普通draw_final冒充跨资源传递。CPU pre-original Execute字段、ticket0及尚未验证内容保持原标注。

LateQuadDiagnosticsTest、quad/continuous自测及实际reserve链通过；最终native清理/入口恢复、XR shutdown/对象销毁通过，但XR exit1、会话incomplete。正常WM_CLOSE关闭游戏，备份`world11-config-after-test.ini`后恢复用户StartupResolutionY=1080。默认未提升，未作真头显或GPU抓帧验收。

## 第十一批：显式呈现关联恢复新画面

Native12 SHA `6475080bb23e0e45f9d989a2c2a2d7c8f382a84b0dd1c2d37346c9e16f56fe94`，XR仍为MainMenu8。私有构建开启`GEARS_FINAL_OUTPUT_TRANSFER`，CMake默认OFF；没有提升玩家默认清单。

现已定位函数RVA9a6d88及lookup helper9a6aa8/9a6adc。启动时逐段实时核验1827/49/49字节SHA，三者均匹配历史artifact1。缓存伪代码中的数据地址比native xref低0x1000，未据其字面值读取全局shader字段。静态及实际几何支持居中图像呈现，但未解码shader bytecode或sampler；此项限制保留，不将CPU门禁当作完整GPU证明。

实现为Tracker专门的同命令列表`draw_transfer(source,destination)`事件：源须先有当前完整、无污染的双眼，Execute按事件顺序显式移到目标；普通draw_final仍不准同ticket跨资源。支持源上的既有overlay、目标后续overlay及已执行目标的UI延续；未知源/目标写入、跨队列、旧代次、重复/链式传递、partial pair和owned模式拒绝。只读endpoint查询覆盖另一命令列表对已传递源的写入。原生仅对已核验caller9a7377、indexed6/instance1、单输入、当前buffer、单RTV/mip0、实时R10G10B10A2单采样源及完整viewport/scissor启用；本轮范围为同宽、目标高度等于源或多一像素，严格核对居中UV。非当前buffer的final-eye也须通过原有矩形/调用者/票据校验；已记录端点的后续Draw及实际CopyTextureRegion目标写入会污染，其他场景RT保持原有快速过滤。clear/indirect等未完整观测的限制不被隐藏。

会话`continuous-stereo-15856-20261016-062734-392a32f6.json`，game15856 / creation134365768425995984，owner35536 / creation134365768361929342，XR20428。同一对照：baseline published221；物理1081阶段491；恢复物理1080阶段738；Resume后1072、projection251→593；主菜单继续进入world2后published2125、projection1203，所有阶段末layer1。两次实际resize仍为1081，已不靠“物理恢复即GPU恢复”的错误假设。自动世界交接generation1→2通过。

图像：`.gpu.ppm.g3`为1920×1081的新GPU资源输出，两份完整暂停菜单可见；缩略图`world12-g3-preview.png`。后续模拟器自动历史`session_20428/incident_16193/frame16182_preview.bmp`可见双眼游戏场景，低分辨率检查副本`world12-before-death-preview.png`；这不是高质量像素比较或完整UI/头显验收。额外MCP frame-info/stereo验证并发请求长期未返回，已终止该诊断cell，没有重启游戏；等待期间角色实际死亡，owner菜单记录DeltaGameover，后续共同面板不计projection，不将死亡阶段累计缺projection数误报为修复失败。没有取得该工具的立体验证PASS。

最终transfer accepted4566、source_draws10014，published4860、pending tickets0、rect_rejected0；pair_rejected50340包含无当前完整双目的菜单/死亡阶段，不等同有4566个已验证shader帧。list/event/target及descriptor容量故障均0。FinalTransferPolicyTest、FrameLinkTest17990检查、continuous/frame-link/color-copy及完整reserve链通过。正常停止passed=true，native清理/入口恢复true、XR exit0、shutdown/对象销毁完成；随后WM_CLOSE正常关闭，配置备份`world12-config-after-test.ini`并恢复1080。

该实际运行修复了此次额外呈现路径导致的发布停滞，不推广为任意aspect/resize、全部CG/视频、全部UI、6DoF或真头显验收完成。下一步验证常用4K路径回归、继续收敛其余V1缺口，再决定默认构建提升。

### Native12 的 4K 回归

同一冻结 Native12 / MainMenu8，会话 `continuous-stereo-14216-20261016-063841-411d73b0.json`，game14216、owner2784。物理客户区3840×2160→2161→2160，暂停菜单阶段published3578→4141持续增长，但共同菜单面板不计双目。恢复游玩的3秒采样published停在4267，模拟器末次layer0；之后日志记录新resource generation的首次projection并恢复，不能把这段短暂空层算作稳定性通过。主菜单→Continue后自动world1→2，新世界采样published5083→5170、projection8849→9108、layer1，说明并非持续无法恢复。

证据 `world12-4k-rect-experiment.json` 及同会话stdout/menu/world/native-status。实验未抢焦点；最终正常停止passed=true，native清理与入口恢复true，XR对象销毁及shutdown完成；身份校验后WM_CLOSE退出游戏。保留测试前后配置，并仅恢复StartupResolutionX/Y到原1920×1080。此轮仍为70° HUD，55°源码候选属于后续Native13；没有头显或完整无闪黑验收。

### Native13 同世界暂停恢复对照

Native13 SHA `f6f12ccf16015470bf7e0b19b7593fec7e8da606bd1a9a482309798d5945cd21`，XR仍MainMenu8；仅HUD55°变化。会话 `continuous-stereo-42972-20261016-064504-bcc8002c.json`，game42972、owner28540。`world13-control-rect-experiment.json` 每200ms采样（native状态自身约1秒更新）：无resize恢复游玩10秒50样本，published440→824、projection374→1261，零层样本0；同world1物理2160→2161→2160后恢复游玩10秒50样本，published1340→1646、projection1604→2490，零层样本0。尺寸变化中的暂停阶段10样本有1个零层，仍保留可见黑闪缺口；采样未发现不代表逐帧零层不存在。

显式transfer accepted1093、policy_rejected0。正常停止passed=true，native清理与入口恢复true，身份校验WM_CLOSE退出。配置前后备份world13-config-before/after-test.ini，仅恢复X/Y到1920×1080。此次未做world重绑，不把暂停或普通菜单共同quad计双目，不提升默认、不作全场景黑屏验收。

## XR9：区分实际合成空窗与正常菜单

此前outage诊断只把projection算作submitted，正常eye-menu/startup共同quad也会耗尽首个有限记录窗口。XR9现在将只读Row副本交给outage，以成功xrEndFrame且layerCount>0为合成提交依据，输出schema2、实际layer_count及拥有内容的layer_kind。image-ticket与projection证据继续使用原始Row及原有语义，不把二维面板算立体，也不把非空层当成源图非黑的证明。只在原有诊断开关内运行，没有新增Hook或改变显示策略。

冻结 `tools/openxr-probe/build-main-menu9/OpenXRPoseProbe.exe` SHA256 `0b3a2c6878ad01ec3707e18d43ecdc0f354549ebae38e590b69e212f9a43de12`，Native仍23。11项相关XR测试通过；新增覆盖连续projection→eye-menu→startup层不制造outage、零层/失败EndFrame确实制造gap及恢复、原Row不变、标签拥有性及schema输出。

实际会话 `continuous-stereo-43368-20261016-103519-21448e5b` 完成检查点确认重载、world1→2及新Pawn手枪开镜/返回。新诊断首窗口9行、丢弃0行；后续另有2个窗口未完整保留，所以complete=false，不能声称全部逐帧区间均有完整日志。最长合成提交间隔1.8005819秒，已恢复：最后eye-menu为QPC58252332334500/ticket1241，首个零层58252343563600，恢复projection58254132916400/ticket1242。中间状态menu_recovery_pending，旧样本received58252187636100不满足恢复minimum_sample_ns；新样本received58254087644200后才恢复。这证实菜单→新场景交接存在实际空窗，不能再用正常菜单投影缺席解释它。

只读相机观察 `xr9-checkpoint-camera.jsonl` 有20个cinematic=true样本，但范围58215067942900–58217038168700，均早于首投影58221665926200；不把它们当成已运行VR中的剧情切入验收。Bink资料库查询仅找到已有脚本声明和历史FullScreenMovieBink路径字面量，尚无当前播放调用/画面证据，未据此启用视频回退。会话正常停止、原生清理/43入口恢复/XR shutdown通过，90秒只读相机任务完成后WM_CLOSE退出游戏，输入服务停止。下一项需解决真实交接空窗，同时保留新票据门槛、世界/资源/参考空间生命周期和二维/立体证据区分；不能把旧菜单帧冒充新场景投影。

## XR10：独立返回游戏过渡卡

新增session-owned static-image swapchain，使用纯CPU生成的768×256 RGBA卡片，显示RETURNING TO GAME与等待下一立体帧的说明。它不引用游戏、旧眼纹理或原生世界对象；只上传一次，严格成功wait、GPU fence完成和release后复用。异常时按既有unsafe生命周期策略保留不确定资源，正常停止先销毁自身chain再销毁session。源预览明确不是GPU回读。

policy要求菜单实际成功显示过、当前为新鲜认证GAMEPLAY、仍待新projection，且无层的原因属于普通等待。UNKNOWN/失焦不可渲染/结构失败/停止不能激活；上传后重读菜单lease并检查停止请求。新projection门槛和票据保持原状，卡片不清恢复状态、不计立体。审查还修正旧handoff将正值XR_SESSION_LOSS_PENDING误当成功的问题：清恢复状态、成功计数和日志现在明确要求XR_SUCCESS及实际projection层；接收器自测覆盖这一边界。

冻结XR10 SHA256 `2504156a90bfb9ec0774d6a974cc2dc5aeb65847751c536c468825c029a77bf4`；12项相关测试与projection receiver自测通过，独立审查及卡片源图检查完成。实际会话 `continuous-stereo-3600-20261016-104746-e7639997` 在59013959901900–59015804193000提交166层过渡卡（一次上传），随后成功新projection；world1→2、新Pawn手枪开镜/原步枪恢复及最终清理/43入口恢复/XR退出通过。该路径已有连续合成，等待本身未被伪称消失；没有真头显像素验收。

同轮另复现确认框阶段的3.9450159秒零层：59006159367900最后eye-menu，59010104383800恢复eye-menu，非过渡卡阶段。menu日志sequence89为原始RuntimeError `Object registry identity changed`，触发关闭reader、1秒退避和全扫描。该错误是缓存库存槽不匹配，区别于后续live-slot错误；当前仍是待修项，不能因为返回游戏过渡卡成功就宣布所有闪黑完成。原始源图、事件和私有清单均留在ignored analysis，玩家默认清单未提升。

## XR12：定位启动捕获空窗的直接门禁

在已有outage有限记录中增加值类型菜单捕获/渲染快照，schema3；记录最近捕获结果、尝试/无帧/接受计数、HWND/generation/epoch、freshness原判定时间，以及本帧面板请求、render调用、返回值和授权拒绝。未调用render明确标为cached_only/not_attempted，不把旧捕获结果当作当前撤层原因。快照在render返回时冻结，后续授权重读不能混入新epoch。未增加捕获、QPC、窗口探测或等待调用，显示门禁与TTL未变。独立审查、12项XR测试通过。

冻结XR12 SHA256 `18d656b5892642f80732ce2a193c13623b2e5a90f420790e82e9adbd727aded6`，Native23不变。实际会话 `continuous-stereo-41892-20261016-113223-9298c461` 正常进入关卡并完成非零位置回中；最长非空合成提交间隔453.4142ms，首窗口38行、丢弃0、后续窗口0，complete=true。最后成功捕获age991.0395ms；首次缺层age1002.1464ms，reason=capture_not_fresh。随后窗口不可用，generation1→2/HWND0→3/原HWND恢复，epoch重建；最后新捕获age27.7566ms时恢复startup_panel。首窗口37个缺层帧中19个capture_not_fresh、18个window_unavailable，没有授权拒绝；首个menu monitor live值晚于恢复25.13秒。

这证明此次空窗的直接显示门禁，未证明窗口为何不可用或WGC为何没有新帧；“静态内容导致无新帧”仍是假设。一次future捕获拒绝也不能证明持续时钟偏差。合成层日志不是头显黑像素验收。原始日志和分析留在ignored `analysis/runtime/xr12-startup-gap-analysis.json`。会话原生清理、43入口恢复、XR shutdown及WM_CLOSE退出通过。后续可在首次projection前的既有启动等待范围显示独立提示卡，不复用过龄游戏图像或放宽门禁；尚未实现于XR12。
