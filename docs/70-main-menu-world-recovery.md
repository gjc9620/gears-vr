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
