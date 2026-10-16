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

## XR13：首次场景前的独立启动等待提示

新增STARTING VR / Waiting for the game image的纯CPU像素variant，并使用第二个独立TransitionCard实例/STATIC_IMAGE交换链。正常WGC、菜单和projection优先；仅首次成功实际projection之前、原startup show_panel许可、最终没有层、XR可见且shouldRender/running、没有停止/loss/结构错/producer stopped时，普通等待才可显示。startup白名单复用旧等待集合，额外允许menu_requested以覆盖首次场景前已认证菜单的WGC等待；返回游戏卡的白名单不变。上传前后核对既有停止/owner控制，上传后重读菜单交给policy，再复核。首次成功projection后handed_off永久禁用启动卡，后续world/resize/menu不会重启它。

卡片独立记录startup_wait_card层及计数，不增加projection、ticket或WGC成功数，不更改新票据门槛、帧有效期或runner超时。正常和异常路径均在session cleanup前释放两个独立实例；不确定GPU所有权按既有unsafe保留规则处理。12项XR测试、扩充的receiver全status白名单自检和独立policy/lifecycle审查通过。

冻结XR13 SHA256 `3576ac6f9b44e8fd6197c8496ec35c694adf1870017b269462d080b89c60e740`，Native23不变。会话 `continuous-stereo-35372-20261016-114128-9cf8b252` 实际启动卡34层、一次上传，然后完成真实projection交接。XR记录2170次projection、936次场景复制；outage complete=true，未出现超过100ms的合成间隔，最长34.89ms。四个原生阶段各946对视图矩阵无偏差；ticket289的1920×1080 GPU源两眼均有非黑场景。原生清理、43入口恢复、XR shutdown及身份校验WM_CLOSE退出通过，无焦点激活。

原始日志、PPM和 `analysis/runtime/startup-card-verification.json` 保持ignored。此轮是一次自然启动回归，不能拿不同运行的453ms与35ms直接当作严格性能对照；证明的是启动卡实际被提交且此轮未出现阈值空窗。不宣称已修复WGC底层短暂不可用、全部菜单/CG/游玩黑屏、真头显文字像素或长期稳定性。玩家默认清单尚未提升。

### XR13 / Native23 的4K完整循环及剩余空窗

会话 `continuous-stereo-23988-20261016-114631-ec71ca35` 实际源3840×2160、每眼1920×2160。完成两次取消检查点确认、实际重载world1→2、新Pawn手枪开镜及切回步枪，再经精确Surrender确认返回主菜单、Campaign/Single/Continue进入world3。三张GPU源图index0/1/2、ticket295/5823/7877保存回执有效，world3左右眼有场景。四个原生阶段各7885对视图零错配；准星finite应用91次只作代码路径计数，不等于头显融合或全部UI像素验收。projection9308次、场景复制7841次。停止、43入口恢复、XR销毁和正常WM_CLOSE通过，测试前后仅将用户分辨率1920×1080↔3840×2160并恢复，保留原配置备份，无焦点激活。证据 `xr13-4k-events.json` / `xr13-4k-audit.json` 及同会话原始文件均ignored。

**此轮不能作为无闪黑通过。** 最长合成空窗4.6447721秒，边界62701975943200–62706620715300。Continue后的首次缺层WGC仍fresh/rendered，上传后菜单授权复查拒绝；随后PenderCrossplayInvitePopUp被分类UNKNOWN，show_panel=false，无render尝试，最后GAMEPLAY允许返回卡才恢复非空合成。world3提交更晚，不能用最终恢复掩盖此区间。outage complete=false/later_windows=4，其他区间未全部保留；首窗口另有约255ms的world2 source_reconfiguration→waiting_image空窗。image-ticket保留4096、丢弃3745，也不能声称后半程所有图像票据完整保留。

目前只知道UNKNOWN的tag，不能断言是input_block_count或player_index哪项失配。Python监控新增仅在原模式/原因变化行输出已读sample的紧凑state_evidence（top/active/focus及分类门禁），不改变64字节packet、分类、时间戳、重试、行率或显示权限；36项相关测试通过。需要下一次Continue的UNKNOWN/恢复对照，之后再确定仅允许通用等待提示的精确过渡状态。默认未提升。

后续1080p会话 `continuous-stereo-11428-20261016-120425-bc269c4e` 取得对照：sequence233（63624141326500）block=2/UNKNOWN，sequence249（63628297074400）block=0/GAMEPLAY，相隔4.156秒。两者read_only/consistent均true、player0；top仍为同一PenderCrossplayInvitePopUp对象，活动栈精确为HUD_Toast→MessageToast→Pender，前两者mode0、top mode2；焦点链仅top自身且states=[UIState_Enabled]。尚未放宽分类，下一步单独认证仅通用提示的窄状态，继续禁止旧画面、投影和自动输入。主菜单Continue→world2及请求图像完成，正常停止/43入口恢复/XR shutdown/WM_CLOSE退出通过，证据 `xr13-loading3-events.json` 及同会话日志。

复现中的另两轮单独保留：40736在原生激活前因CrosshairRegistryChanged校验拒绝，runner报告incomplete；预留日志已取消且active0，另外只读核验6个系统入口及4个图形设置入口后正常WM_CLOSE，未将其改写成通过。40428的精简测试脚本漏定义world_path，在菜单操作前报错，已正常停止/恢复并退出；补变量后增加全局引用检查，后续11428才是有效复现。它们不是新的已证实游戏闪退，也未在原进程上重复注入。

## XR14：精确阻塞状态仅授权通用等待提示

Python新增LOADING_DISPLAY=5，单独认证上轮实际block2/player0的完整三项活动栈、单一Enabled焦点、同outer及top/末项/焦点身份一致；所有计数和mode必须严格整数，tag_number精确0，拒绝截断标记、额外场景、block1/3、错误玩家和不一致读取。不硬编码运行地址或对象实例number。原GAMEPLAY的block0条件不变，没有新增输入许可。C++仅扩充mode5，packet仍64字节/schema1，身份、序列与1.5秒lease不变；旧XR会把5拒绝为UNKNOWN。

loading显式禁止WGC、eye-menu及projection，只更新恢复时间边界，不产生菜单witness。已有真实菜单witness时可显示独立返回卡；上传后重读状态，仍为loading或合法GAMEPLAY恢复才可排层，UNKNOWN/过期/停止/结构错撤销。首次projection前可显示独立启动卡，并在loading→GAMEPLAY等待新票据时继续；真实projection成功后该首次分支永久关闭。world monitor不以mode5授权重绑或输入，实际该参数未参与其判定。

冻结XR14 SHA256 `45e6cf45f996cf09706303ab3da634a5e46f742459da46044dd704b7334e6511`，Native23不变。58项Python monitor/world-monitor测试、12项XR测试及receiver自检通过，独立审查通过。

实际会话 `continuous-stereo-41216-20261016-121254-c523f414` 完成主菜单Continue→world2、不同Pawn和GPU源index1双眼非黑场景。mode5→GAMEPLAY持续4349.3252ms；对应成功返回卡episode自64135991191800至64149669323500，持续13.678秒，跨过loading并等到新projection。首条卡比分类采集起点晚28.1192ms（采集/发布和XR观察的端点差，不等于28ms空层）；内部区间有连续成功卡episode覆盖。此期间image-ticket没有新记录，未以“没有记录”冒充零投影证明。两段返回卡共1327层、一次上传；原生四阶段各2221对视图零错配，projection1853次、场景复制2207次。正常停止、43入口恢复、XR shutdown及身份核验WM_CLOSE通过，未抢焦点。ignored证据 `xr14-loading-events.json`、`xr14-loading-audit.json`、原始日志/图像及冻结原生状态。

仍未通过完整无空窗验收：最长合成间隔493.3288ms，完整outage记录（later0/dropped0）显示它位于更早的返回主菜单阶段：SnapshotChanged导致短时UNKNOWN，随后MAIN_MENU等待新的合格WGC捕获；与已认证的Continue block2区间不同。source_reconfiguration拒绝23帧也不能直接算23个空层，本轮对应返回卡一直持续到新projection。默认玩家清单仍未提升；还需收敛该主菜单交接、完整4K候选回归、UI/CG、真头显及便携部署。


## XR15：区分快照采集竞争与普通未知状态

仅 reader.sample() 阶段抛出的精确 SnapshotChanged 发布 ACQUISITION_WAIT=6；构造/身份/分类错误、派生异常、其他采集错误及退避仍为 UNKNOWN。失败不携带局部样本，原采集起点、64字节包和1.5秒有效期不变。普通游玩遇到6时保留 UNKNOWN 原有的严格投影资格，不创建恢复边界或菜单 witness。已有真实菜单 witness 的恢复阶段，6或新鲜 MAIN_MENU 可在正常图层尚未排入时复用独立等待卡；menu_requested 仅在这两个窄状态下新增许可，上传后重新核验状态与停止条件。普通 UNKNOWN/过期包不授权该卡。等待卡默认文字改为中性的 WAITING FOR GAME IMAGE，首次启动文案保持原样。

XR15 SHA256 `166a697dbd80dbfe77f7261741263638a0c7de04611f6784d956855ec78714e0`，Native23不变。59项 Python monitor/world-monitor 测试、相关 C++ 测试与 receiver 自检通过；Astra Medium 独立审查未发现阻断问题。精确异常分支由故障注入验证；实际主菜单往返验证结果另记，不以静态审查代替运行证据。

实际会话 `continuous-stereo-36080-20261016-123605-75feb4ab` 完成1080p主菜单往返、world2新Pawn及GPU index1双眼非黑图像；四原生阶段各2235对零错配，projection2141次/copies2223次。MAIN_MENU新捕获交接有成功等待卡episode，首段897.1091ms；本轮没有精确SnapshotChanged，因此mode6运行分支仍未覆盖。正常停止、43入口恢复、XR shutdown和身份核验WM_CLOSE退出通过；无焦点操作。

审计 `xr15-loading-audit.json` 明确保留 bounded_checks_passed=false：原block2加载状态持续4138.8894ms后出现完整一致但block1的同名三项场景状态，约256.8745ms后才到block0 GAMEPLAY，故“直接到GAMEPLAY”检查未通过。最长成功非空合成间隔344.5089ms（complete=true、dropped0、later0）恰在该block1 UNKNOWN区间，不能声称黑屏已全部修复。block1仍未授权卡/投影/输入；下一步仅评估是否给该精确实证结构增加通用等待卡许可，禁止直接放宽全部UNKNOWN。三段返回卡分别约897ms/4078ms/7401ms，仅最后一段交给projection。原始事件、图像、冻结原生状态与审计均留在ignored analysis/runtime。


## XR15b：同一精确加载结构的block1衔接与4K联合回归

依据XR15原始第9行，只把loading_display严格整数条件扩为block1/2，完整三栈、焦点、身份和一致复读条件不变；block0才走既有GAMEPLAY，block1/2均不授权输入。仅mode5的日志去重增加block计数，保留实际2→1变化，不增加重复状态行。60项monitor/world-monitor测试通过，两种block均执行完整结构破坏负例；无需改XR15二进制或原生Hook。

实际会话 `continuous-stereo-15660-20261016-124127-c2a2430e` 在3840×2160源SBS/单眼1920×2160完成两次确认框取消、真实检查点world1→2、新Pawn手枪开镜、还原步枪、主菜单Continue→world3。GPU index0/1/2三张图均有独立票据且双眼非黑，接触图目视有场景、角色、开镜与字幕；这不证明全部UI或真头显融合。原生四阶段各6265对零错配，projection5102/copies6236。

加载实测记录blocks=[2,1]后直接到block0，总计4402.8728ms，成功等待卡episode覆盖内部区间；完整结构与加载审计通过。独立4K审计通过的范围仅脚本路径、源GPU图像及原生一致性，明确不包含无空窗验收。最长非空合成间隔仍为222.4131ms，complete=true/dropped0/later0，位于GAMEPLAY下source_reconfiguration→waiting_image→submitted，需独立处理。日志另见精确Registry末尾复读竞争导致普通RuntimeError/reader_retry，尚未放宽保护，下一步分析专属异常与原client校验。旧XR15失败审计保留原样。

正常停止、43入口恢复、XR shutdown、身份核验WM_CLOSE通过，CIM确认游戏与XR进程退出。临时4K仅修改分辨率X/Y，游戏退出后已恢复1920×1080，其他设置保留；无激活焦点操作。ignored证据：`xr15b-4k-events.json`、`xr15b-4k-audit.json`、`xr15b-loading-audit.json`、原始会话日志/图像/冻结原生状态、`xr15b-4k-config-state.json`。默认玩家清单未提升。


## XR16：独立世界交接提示及Registry末尾竞争恢复

XR15b的222ms间隔发生在菜单恢复成功约3.19秒之后的world交接，不能归因为菜单卡被不合法旧票据提前替换。新增独立WorldHandoffDisplay：仅有效world与opened receiver身份共同认证的quiescing或已有baseline后的generation增长建立pending；初始稳定代、无效/回退generation、wrongPID/resource均不能制造pending。只在continuous会话、新鲜GAMEPLAY、正常world重配置/等待图像、无其他图层且无停止/结构错误时复用原中性静态卡。上传后重新核验world、receiver资源身份、owner/stop和菜单lease；不改变projection票据、时效或资源获取规则。只有实际成功提交当前有效代次projection才清pending；独立world_handoff_card证据与layer_kind不算双目场景。

XR16最终SHA256 `29d7e2e651104868bde3931939670eb5461bf6496ff289b901c0a7b3d2bba1b0`，Native23未重建且ABI布局未变。完整build16、16项CPU测试与receiver自检通过，补充wrongPID/resource quiescing→匹配同代和回退代次负例后重建复核。

另将MenuReader最后一次objects header比较的唯一竞争分支区分为RegistrySnapshotChanged，失败快照仍丢弃并标记dirty；monitor仅精确异常保留reader，继续UNKNOWN/null证据，不授mode6。下次完整采集前沿用既有强刷，完整names/objects库存、原client身份、零名和两个header复核通过才清dirty；强刷共享一次预算，整轮_sample_once最多两次，I/O/header/client失败保留原拒绝/重建语义。52项menu_state、32项menu_control、41项menu monitor、19项world monitor共144项测试通过。

实际1080p会话 `continuous-stereo-16312-20261016-130305-7e8c534c` 完成真实检查点world1→2、新Pawn手枪开镜/还原步枪、主菜单Continue→world3，GPU index0/1/2双眼非黑，四原生阶段各4051对零错配；projection4310/copies4032。独立world等待卡14层，成功episode从67125169039300至67125324784300持续155.745ms，由quiescing代1转到有效稳定代2成功projection而结束。两份离线审计通过，范围是本轮脚本、源GPU图像、原生一致性与已审查的成功卡episode契约，不代表头显像素验收。

本轮outage完整记录complete=true/later0/dropped0，最长间隔89.319ms，未出现超过100ms窗口；最长位置在Pause面板初始衔接。mode6精确SnapshotChanged已实际出现，但本轮未再次复现RegistrySnapshotChanged，后者恢复由故障注入测试证明，不能宣称实际竞争路径已运行覆盖。正常停止、43入口恢复、XR shutdown、身份核验WM_CLOSE及CIM进程退出通过，未抢焦点，分辨率保持1080p。ignored证据 `xr16-world-events.json`、`xr16-world-audit.json`、`xr16-handoff-audit.json`、原始日志/图像与冻结原生状态。

XR16尚未进行完整4K回归和真头显验证；默认玩家清单未提升。下一重点转向实际CG/视频与整套UI覆盖：cinematic标志和Continue加载不能代替播放区间/实际画面证据，章节入口尚待精确只读状态确认；不得盲目开始新游戏覆盖存档。


## XR16 完整4K回归与本地GUI候选更新

会话 `continuous-stereo-42956-20261016-130831-35243d38` 实际3840×2160源SBS/单眼1920×2160完成两轮确认框取消、检查点world1→2、新Pawn手枪开镜/还原步枪、主菜单Continue→world3；index0/1/2三张源GPU图像双眼非黑，接触图目视有对应场景/角色及字幕。四原生阶段各6214对零错配，projection5225/copies6166。独立world等待episode持续167.0246ms并由有效当前代projection替换。完整outage最长88.8906ms，complete=true/dropped0/later0，未出现超过100ms窗口；两份窄范围审计通过，仍不等同真头显或全部UI/CG验收。

正常停止/43入口恢复/XR shutdown后，另一次仅菜单检查以原game全身份、原HWND、已停止同session及43入口恢复证据为门禁，双次稳定只读采到Campaign Continue完整树，确认同parent下存在butLoadChapter、butContinue、butNewCampaign。没有按Continue/选择章节/开始新游戏，没有焦点操作；这轮菜单检查在VR停止后，不能算VR菜单像素验收。之后身份核验WM_CLOSE及CIM确认退出，临时4K设置恢复1920×1080。证据 `xr16-4k-events.json`、`xr16-4k-audit.json`、`xr16-4k-handoff-audit.json`、`inspect-chapter-menu1.json`及原始日志/图像。

在两种分辨率实际回归后，本地GUI的manifest.json切换到具名manifest-world-recovery23-xr16.json；实际game/profile/native/XR哈希和settings前置验证、204项相关Python测试及隐藏GUI构建自检通过。旧manifest-game-recovery8.json保持原样可回滚，新增HUD大小控件现随候选能力开启。仅本地研究候选更新，不宣称便携包或发布验收完成；未改用户config/其他对话的README变更，也没有修改全局OpenXR Runtime。

章节入口的静态查询已先用现有KB。artifact2的WarUIScene_Campaign类导出仅native/defaultproperties，没有按钮处理体；butLoadChapter在包对象清单中存在，与当前只读UI名相符。LoadChapter字面量仅历史运行时快照候选，不能证明当前按钮执行语义；ChapterSelect有界查询无匹配不代表不存在。原始查询保留在ignored kb-chapter-loadchapter/chapterselect.json。CG下一步仍需确认章节选择界面，并在任何实际换章节前保护现有存档；UI优先补当前布局selector与主动换弹图像。

## Native31/XR23：有进度证据的CG显示及原生交回（2026-10-16）

已提交 `28d3a89`：电影窗口fallback仅由精确身份绑定、只读movie_progress证据授权，校验singleton/vtable/handle及帧进度和复读一致性；UNKNOWN、普通gameplay或loading本身不授权任意窗口画面。电影采用双眼共用的2D窗口画面，保留时效、代次及停止保护；有效原生projection优先，并以成功提交的原生帧完成交回。这是CG可见性路径，不能算双目场景或立体CG重建。

XR22实测 `analysis/runtime/movie22-fallback-summary.json` 记录eligible2843、render_true2641、render_false202，202次原因均为capture_not_fresh；另有399次capture future。该失败不能被“CG模式已经打开”掩盖。已提交 `632e4a7`：WGC帧时间略晚于本次判定now时，单槽暂存后等待原始时间戳可接受再处理，最多持有250ms；epoch/generation改变、超时等仍丢弃，不改捕获时间戳或放宽future/freshness规则。

实际会话 `continuous-stereo-13096-20261016-223856-64596f25` 覆盖首次入关及一次死亡检查点重载。`analysis/runtime/movie23-validation.json` 与原stdout显示：首段render_false只有入口阶段6次；重载后累计17次，新增均在入口区间，已观察的中途播放段没有继续增加。该计数是render_false诊断次数，不是“丢17个视频源帧”的证明。末次快照eligible7138、render_true7121、submitted7119、end_failed0；post_context_rejected2单独保留。pending_reused940、pending_dropped1，已观察最大future ahead为13.0988ms。三条 `game_window_fallback_recovered/native_projection_end_succeeded` 标记支持电影2D公共画面后交回原生双目；不是所有CG、无空窗、声音同步或真头显像素验收。该validation是会话内快照，不是正常退出及长期稳定验收报告。

同一13096后台启动也实际触发一次tiny输出恢复，连同17708及已提交 `5fc7e9f` 的守卫/诊断边界见文档68末尾。菜单窗口重绑后成功确认的一次有界重试由 `0559132` 独立修复；失败或部分发送、butContinue不重放，身份/焦点树/稳定复读保护保持。

`c4cb379` 已将默认manifest切到 `manifest-background31-movie23-vd.json`：Native31 `build-background31` 配XR23 `build-main-menu23-vd`，启用movie_display。此前各节“默认未提升”均为当时记录，不应据此再次回到XR16/19重复定位。当前完成的是上述本机有限路径；整套UI、近距离准星、真头显、长期运行、更多CG/分辨率/硬件回归及便携发布和依赖许可仍待验证，不宣称V1完成。原始日志、图像、运行时派生物继续留在ignored analysis，不能进入发布包。


## Native32候选：真机反馈后的材质路径修复（尚未提升默认）

2026-10-16用户反馈机枪、换弹、FPS选项及连续游玩黑屏、启动延迟/不进入VR。真实会话40028的保存native-status显示3528次材质上传全部被om_target拒绝；目标是最终输出前的中间纹理，旧实现仅接受swapchain backbuffer。最终传输pair_rejected4117与ticket_conflict4117一致。提交5a19df0允许有完整当前命令列表双眼pair、格式/尺寸/最终传输契约均符合的中间目标进行既有材质双眼重放，保留未知写入失效和最终传输校验；不能据此认定所有新反馈同因。

冻结Native32位于build-material32，原始源来自提交归档，未纳入其他未提交UI实验。GameUiMaterial、FrameLink17990项、FinalTransferPolicy、UiHudPlan、UiMaterialBatch及连续/FrameLink ABI自测通过。模拟器会话continuous-stereo-19292-20261016-235026-f47744cd中，材质draws562（HUD560），仅两个空目标被拒绝，published946、XR copies944，证明新路径实际执行。随后观察到主菜单与空Pawn重绑定失败；换弹脚本在输入前因非关卡状态退出，不能称换弹回归通过。自动首帧GPU快照是黑色图像，不能把成功提交计数当像素画面通过；未取得后续人工请求图，暂不提升默认清单。停止时native_cleanup_verified=true，但整轮phase=incomplete，不称会话验收成功。原始证据保留在ignored analysis/runtime，含该会话native-status副本。

提交579d2a2修复已证明的注册表/时间戳临时变化导致启动永久放弃，保持共享截止时间、身份和代码变化拒绝、激活不重试。19292实际经历7次重新获取后成功激活，但准备耗时19.9秒，启动延迟仍未解决。提交3c83336/3229b4d增加GUI中英文切换和后端配置兼容，50项launcher测试及GUI自检通过。

余项继续保留：机枪/载具摄像机覆盖、所有黑屏路径、头显帧节奏、缩短启动、HUD元素尺寸与位置分离、真机回归。预渲染CG只能双眼共用二维电影，不能标为真实几何立体。当前默认仍Native31/XR23；Native32仅诊断候选。


### Native32后续有效图像与本地默认候选更新（2026-10-17）

第二轮continuous-stereo-12492-20261016-235900-c6b9bd9c补齐第一轮缺失的像素证据：换弹请求快照1/ticket349/source_serial341、快照2/ticket366/source_serial358，均1920×1081。目视两眼场景、武器HUD、换弹条及受伤效果可见；160个严格弹药样本完成，场景脚本完成普通换弹采样，未验证完美换弹。快照和原始状态在ignored analysis/runtime/material32b-reload*及会话PPM。

停止后材质consumers/uploads/draws各568，rejected/failed为0；最终传输accepted892。原生清理、入口恢复和XR退出完成，report stopped/passed=true；该passed仅既有生命周期标准，仍有8671次最终pair拒绝，提交最大间隔46.815秒，不能解释为连续稳定或所有黑屏均已修复。menu/死亡等区间必须结合模式区分。保存的native-status副本保持原始证据。

已将本地默认更新至manifest-material32-movie23-vd.json：Native32配原XR23，保留Native31具名清单回滚。这是已证实路径的修复候选，机枪/载具、完美换弹、FPS设置、启动延迟及头显卡顿仍待各自验证。未改变用户游戏分辨率或全局OpenXR设置。
