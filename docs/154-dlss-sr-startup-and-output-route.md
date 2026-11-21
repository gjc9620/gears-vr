# V1 DLSS SR：隔离启动与输出分支

2026-10-20 最新范围：用户将 DLSS 确认为首版可选功能，允许独立 CMD 入口；普通启动器无需为它解除保护。
其他 Bug 修复完成必要验证后直接集成正常入口。本条覆盖本文历史段落中 DLSS 必须纳入 V1 的约定。

2026-10-20 后续头显反馈：独立测试入口可以运行，但帧率很低。手动 NgxManual2 会话
`continuous-stereo-8492-20261020-184444-d9cb27ab` 的正常游玩样本约13.73个新双眼帧/秒；
接收端实际6144×3264、每眼3072×3264，不能把120Hz XR循环当作游戏帧率。
关闭DLSS的匹配尺寸后台基线也仅17.14 FPS；旧构建开启时拒绝MV证明，未取得有效SDK开启段FPS。
没有以失败轮补零或把启动成功追认为完整DLSS头显验收。

另一个受控A/B/A中的单描述符复制快路径得到稳定约22.00→25.79 FPS，但不构成完整DLSS开关验收。
私有 `GearsCaptureNgxDeviceCache1.dll`（SHA256 `a00536b049b0157c5281eadb19a437e32b061cd4995966c032b085f64b2dd4a8`）
进一步缓存持有的typed设备接口，保留首次/不同接口的canonical身份核验，另处理有界较大上传区中的稳定160B前缀。
完整Shader哈希和其他渲染/发布要求保留；CPU、启动及真实D3D12描述符fixture通过。
其首次FPS窗口与独立autoplay53实例重叠，已取消自有测试，不能据13.48 FPS评价候选。
自有游戏正常退出，native清理、独立入口还原和5文件恢复均通过；未关闭其他实例。
候选未切换玩家入口，SDK开启及性能收益仍待独占实测。

用户授权等其他会话结束再测。待办与精确证据为 ignored
`analysis/runtime/dlss-performance-queue-20261020.json`。性能测量需等待所有游戏与自动流程worker退出，
不能只检查普通启动器互斥锁，因为隔离实例有独立锁。资料和私有候选继续留在ignored analysis/。

2026-10-20 20:12：后续私有候选 `GearsCaptureNgxBatchScratch2.dll`（SHA256
`906bf7cb36abcbe6b7c2d2406ebb7f7ea1385c51a1c3938622b32de178f75019`）保留上述设备缓存，
把批量描述符复制的128格默认构造改为仅构造实际快照；全部源快照仍在目的写入前冻结并持有资源。
全新构建、完整启动自检、针对检查及真实D3D12 fixture通过；没有实际游戏收益或SDK开启通过结论。
`performance_guard.py` 的5项检查通过，覆盖隔离游戏、轮次间worker、PID复用和精确owner身份。
新协调器先等待全部其他游戏/worker结束及五分钟空闲，再用独立实例的5个配置/存档文件做实际对照；
真实用户文件不参与写入或恢复。生成驱动的实际导入、私有路径、自检和每眼3072×3264计划预检通过，
预检没有启动游戏。保持用户MotionBlur及渲染倍率。运行中遇到其他游戏会取消自有测试并排除数据。
队列记录协调器精确创建身份，继续前先核验它，避免重复启动；玩家DLSS入口尚未切换此候选。

## 当前结果：2026-10-20 owned68 无人值守完整开关验证通过

隔离实验构建已在实际游戏、运行时尺寸的原生 owned RGBA8 场景/UI 路径完成
“开启 DLSS → 关闭 → 再开启 → 再关闭”。首次开启、关闭和再次开启后的真实 XR 接收端
GPU 图像均显示完整左右眼场景。正常退出、原生清理和独立字节还原校验通过。
这是后台模拟器验证；默认玩家启动器仍使用原构建并锁定关闭 DLSS，尚未接入此实验构建。
物理 VDXR/头显、运动中的历史画质、帧率、跨世界和预设 DLSS 启动均不在本轮通过范围。
精确证据在 ignored `analysis/runtime/dlss-owned68-evidence.json`，本页末尾记录本轮身份及边界。

## VDXR 手动测试入口

2026-10-20：手动入口现为 NgxManual3，保留 NgxManual2 的普通 Output/owned DOF 证明隔离修复，并减少单描述符复制的CPU开销。同场景A/B/A在每眼3072×3264、DLSS关闭时提高17.2%，三轮清理和设置恢复通过。没有据此宣称开启DLSS后的帧率或物理头显验收；重新打开测试启动器后生效。见[启动故障修复](162-dlss-manual-startup-proof-isolation.md)及[性能对照](dlss-descriptor-copy-performance-20261020.md)。

根目录 `GearsVR DLSS Test.cmd` 打开独立测试启动器。选择 Quest / Virtual Desktop (VDXR)，
点击启动 VR；进入关卡并确认双眼正常后，在游戏画质菜单选择 DLSS，先使用 Quality（质量）。
这个入口解除启动时和菜单中的运行时锁；启动仍先将保存的 DLSS 设为关闭，供场景挂钩就绪后
手动开启，不尝试未经验证的预设 DLSS 启动。普通启动器及其默认清单保持原有行为。

首次测试保持当前分辨率和渲染倍率，在同一关卡内观察左右眼、转头、移动和 UI，再做一次
开启/关闭/再开启切换。跨关卡、读检查点和改变布局仍未验收。结束时停止 VR 并正常退出游戏；
重新测试请关闭测试启动器，再次双击该入口，使日志和 DLL 工作目录重新隔离。

手动构建 `GearsCaptureNgxManual3.dll` SHA256
`7eda76dbcdc4cea004914840f969c895c1c5ed4bb1c20a5e0aea634b66ccd657`
通过全新 verified build、完整启动自检、DLSS/owned-scene 针对检查和真实 D3D12 描述符 fixture。
这是为用户头显测试准备的新构建，其头显通过状态不能由之前 owned68 模拟器结果代替。
新增默认关闭的编译选项 `GEARS_NGX_MANUAL_OWNED_TEST`，仅适用于原生 owned-scene join；
运行还需显式 `GEARSVR_TEST_NGX_MANUAL_OWNED=1`。所有实际渲染、SDK、命令队列及发布证明
保持原要求。手动模式不设置静音/behind 标志，不允许把最终 R10 staging 路径升级为头显入口。
原后台三标志要求保留。启动策略单元测试和4项脚本测试通过。

脚本校验构建收据、9个启动 DLL/EXE 和 VDXR 库；每次打开GUI把认证产物复制到新的 ignored
`analysis/native-dlss-manual/<id>`，不覆盖旧事件证据，也不安装/修改系统 OpenXR Runtime。
独立清单为 `tools/vr-launcher/manifest-dlss-manual1.json`。当前游戏若仍在运行，入口要求先正常退出，
不会终止已有会话。源码编译前后哈希一致性及CPU证据在 ignored
`analysis/runtime/ngx-manual3-source-audit.json`、`ngx-manual3-tests.json`。

## 2026-10-19: independent source ledger reaches actual MV

Fresh SourceLedger1 DLL `602116a77c48f158271265a7292434677f9ea98a9d00d989288f82313ca2765a` passed457 dependencies, CPU source policy checks and real D3D12 fixture proving source continuation after intentional legacy loss. Actual session35632 firstEval world1/ticket604 retained a typed MV and found exactly1 current nonvisible source descriptor. ledger_ready1/device_match1/legacy_failed_at_eval1, version47/origin47, no copied-source predecessor, explicit view bytes retained. No integer-to-COM cast or legacy failure recovery.

At teardown source ledger remained healthy:58heaps,40cells (32known UAV,8nonUAV,88unused),75observed UAV writes,7420289invalidations,11055932copies, zero otherdevice/missingbirth/uncertainty/loss. This establishes current source provenance and solves the instrumentation saturation obstruction; it does not establish a Dispatch binding or MV write domain. Next work can target only this retained resource with an explicitly separate bounded observation epoch.

Native/entry cleanup, normal close and settings restoration passed. Session incomplete/ownerexit1, so no playable-DLSS claim. Exact identities/statushash and original scope are in ignored `analysis/runtime/dlss-source-ledger1-evidence.json`. Default player DLL/manifest unchanged.


## 2026-10-19: targeted source-ledger design review

Astra review identified an important boundary: legacy Call admission closes after bank failure, so19 UAV writes is only a pre-failure prefix, not a complete creation census. The independent heap census survives but typed descriptor observations do not. The current frozen pair also stops after its selected ticket, so a future-ticket experiment cannot silently reuse its completed state.

First implementation stage is an independently admitted source ledger, not more global replica storage. Preserve typed CreateUAV destinations and current non-shader-visible CPU aliases. Suppress allocation of ordinary shader-visible copy replicas; still update/erase every already tracked destination, including visible direct-UAV births. Unknown sources and other descriptor writes erase provenance. Validate ranges and snapshot sources before writes. The ledger has its own bounded capacity, failure state and drained COM ownership; legacy failure stays immutable. Accepted Eval may match only a retained typed MV lease against current healthy source cells, preserving original scope/version.

This narrow stage tests whether usable MV source descriptors exist. It does not prove root3 binding, Dispatch, GPU execution or per-eye MV writes. Only after source matching works should a separate explicit targeted observation capture a subsequent complete ticket, bind current heap/root signature/table to Dispatch, and inspect the specific shader/constants. Player DLSS remains disabled until actual rendering acceptance.


## 2026-10-19: stop whole-device capacity escalation

Fresh5121 DLL `97da3228d5601bf40696b64bec1e1bbed0043df6eeb94107f7d3c4eeb3bdfe74` passed457 dependencies, startup/descriptor fixtures and512/513 occupancy,128/129 copy boundary tests. Real COM copy scratch remains46080 bytes via separate128 copy limit. Actual session22752 again failed before acceptedEval join:44heaps,512 known UAV cells,0invalid/free,2529 reuses,19 UAV writes,18528 invalidations,27402 copies. Two Evals had retained MV, but bank_ready0. No missingbirth/device/recursion/uncertainty.

This falsifies erased-cell reuse or modest persistent-capacity increases as sufficient fixes for the whole-device observer. Do not keep enlarging it blindly. Next review targets retained original UAV births and subsequent selected-MV descriptor copies, with distinct observation scope and no claim to repair lost history. Astra review requested; design is not yet implemented. This remains diagnostic work, not proven DLSS root cause.

Native cleanup/entry restoration, normalclose and settings restoration passed; sessionincomplete/ownerexit1. Curated ignored receipt: `analysis/runtime/dlss-descriptor-5121-evidence.json`. Player default unchanged.


## 2026-10-19: descriptor reuse counterfactual

Reuse1 DLL `ebec5cfbbbb36b81de7d0d6c11afd3db7ac8d7197fd961a00de2f228d68efd08` passed fresh457-dependency build, focused CPU and native descriptor fixture/startup tests. Same-address ABA, retained snapshots, source copying before reuse and all-known capacity failure are covered. Actual session32900 accepted two Eval observations with retained MV. Eight erased records were reused, but capacity still stopped at128 known UAV cells, zero invalid/free cells and43 heaps (19 UAV writes,2144 copies). Tombstones therefore contribute but do not explain remaining saturation; live copied descriptors exceed the bound. No known UAV was evicted and no failed bank recovered.

Session remains incomplete/owner exit1, not playable DLSS evidence. Native/entry cleanup, normal close and settings restoration passed. Exact occupancy and identity evidence is under ignored `analysis/runtime/dlss-descriptor-reuse1-evidence.json`. Next candidate may increase the explicit production cell bound while retaining generic limits and rejecting overflow; it must account for range-array types and hook stack use.


## 2026-10-19: typed MV join candidate actual boundary

Fresh `build-ngx-descriptor-join1` SHA256 `d28c9d00e99d14fcf03e9cb7443efbc72e50d1011b7cac32aff7bf02eaaebda1` passed457 dependency checks, continuous startup suite, provenance/census/epoch CPU tests and real D3D12 descriptor fixture. Production heap cap96 is based on the previous60 creation census; generic test default remains8. Typed MV joins never cast NGX integer values to COM pointers.

Native-menu session32716, world1/ticket1403 accepted two Evals with the same retained typed MV/device. The bank failed at43 heaps and128 cells (capacity loss3), with19 UAV writes,2259 invalidations and2067 copies; missing births, other devices, recursion and uncertainties were zero. Therefore both joins correctly report bank_ready0 and no matches. This removes the prior8-heap obstruction but does not establish the MV producer. Inspect invalidated cell retention before arbitrarily increasing cell capacity.

Session phase incomplete, owner exit1; native cleanup/entry restoration, normal close and five-file restoration passed. No playable-DLSS or successful display claim. Native status archive now uses the actual prefix and succeeds: SHA256 `41a61e561ff9d38477e38a79fac822c6b2be3ee450732dfb0698994a1bb9ecce`; curated receipt ignored `analysis/runtime/dlss-descriptor-join1-evidence.json`. Default player manifest unaffected.


## 2026-10-19：早期 heap census 实际结果

独立诊断 `build-ngx-descriptor-census1` DLL SHA256
`c546d426bdfe07535cae3e47cf8d70f1bf93007f48c9d38438655defd8a2dfa9`
通过全新编译、CPU census/provenance/epoch 测试、真实 D3D12 描述符 fixture 和启动自测。
后台小窗口、静音运行 session38876，通过游戏原生菜单启用 SR 后，同一 world1/ticket1434
接受两个 Eval。每个冻结点累计创建60个 CBV/SRV/UAV heaps：19个 shader-visible
（描述符累计75776、单heap最大4096）和41个非shader-visible（累计11352、最大2048）。
另有20个其他类型 heap；API失败、读取失败、饱和和重入排除均为0。
这是跨设备累计创建量，不是存活heap或显存字节数，也不是MV producer认证。

原8-heap provenance bank仍在第9个创建处因容量失败；新的独立scalar census继续统计，
因此现在可依据实际数量设置下一轮有限容量，而不是猜测。当前仍未关联MV资源与真实writer。
GPU completion/domain仍为0。这轮XR场景提交为0、会话incomplete/owner exit1；不能算DLSS显示通过。
native cleanup和入口恢复通过，游戏正常关闭，五份设置/profile恢复。

私有驱动沿用了旧固定归档名，归档步骤报FileExistsError且未覆盖旧证据；协调者随后从本session
专属native-status保存了新证据：`analysis/runtime/dlss-descriptor-census1-evidence.json`
和`dlss-descriptor-census1-native-status.txt`。后续驱动须使用本轮prefix归档。
默认玩家版本未改动，DLSS仍锁定关闭。

## 2026-10-19：Compute Dispatch 边界汇编核对

KB查询后，在已有私有IDA副本只读导出 `1ac6e4` 与 `17b420` 的全部热/冷chunks；导出后关闭lease。证据为 ignored `analysis/ida/ngx-copy-contract-20261019/compute-chunks.json` 及同目录两份汇编。所有字节与历史blob `911ab9…` 一致，组合hash分别 `151372ef327a20c9c6d56d227d371601a80ea61e7f9d572858a41b4d60cd6761`、`c84b4ac0e66f5f17bd7465a26fd54a87017bbd0162f1b2be98b895e599721e43`。原始资料库hash仍为 `324248ac7cd0388a05907c29436fccf804eb82e5cb8433190806342c1cfcbb26`。

`1ac6e4` 保存入口R8D/R9D，调用 `17b420(context+0x40)` 后，从 `[[context+0x127c]+0x14]` 取得命令列表，在 `1ac830` 调用vtable+0x70，返回 `1ac833`；三个Dispatch参数来自入口R8D、R9D和第五参数。`17ba1e` 则调用vtable+0xd0，`17ba24` 清空累计barrier数量。这证实后者不是Dispatch位置，不能从其附近MV的UAV屏障推断实际writer。

绑定助手还在 `17b982` 调用device vtable+0xb8，在 `17b994` 调用command list+0xf8（EDX=3），随后处理其他根参数分支。需要将实际UAV描述符和常量与同一ticket的MV及view对应；当前仅确认调用位置和参数来源，不确认这个通用Dispatch就是MV生成pass。本轮未增加hook、未运行游戏，也未证明当前进程字节与历史快照相同。

## 2026-10-19：原生 Copy 保持接线已集成，未启用 DLSS

将上述静态契约接入控制器，移除未实现的 `copy_states` 回调：现有启用检查新增六个完整函数块哈希；选中的 Output→Color 复制要求返回 RVA 1c6093、原始零 XYZ/null box/subresource0、固定的 typed 四资源、当前 world/ticket/renderer/eye 及真实 command/device/generation。仅调整复制区域和目的偏移，保持游戏原有状态设置、barrier 提交和恢复顺序。无关复制仍转发；选中复制验证失败不会回退全图复制。日志 chunks 改为实际表长度。

协调者运行 `NgxNativeCopyPreservationTest` 32项 CPU 检查、`NgxTypedQuartetTest` 253项及 `NgxStereoHostLiveTest` 均通过；host 开/关两个完整 capture 构建成功，Ninja 依赖分别447/462且有效。构建日志在 ignored `analysis/runtime/ngx-copy-integration-{on,off}-build.log`。默认 Integration1 DLL SHA256 仍为 `41a5b882ab1202940c0590d88a6c732609fd0db9511157e19f51f802064cc065`。

本轮无游戏或 GPU 验证，没有激活 producer，也未改变发布默认。Color/Depth/MV 实际生成区域、共享参数串行化和实际 DLSS 连续游玩仍需证据；这些 CPU 检查不能证明 DLSS 已可用。

## 2026-10-19：原生Copy契约静态核对

先查询KB artifact1，再在原IDA库私有副本导出 `1c5dd8`、`1c60f0`、`1c617c` 完整chunks汇编；每段字节与历史运行时blob `911ab9…` 对比一致。复制函数含热/冷两段789+569字节，组合哈希与KB一致。另机械核对其状态帮助函数 `17c0b4`/`17ab94` 的完整范围和哈希。证据 `analysis/ida/ngx-copy-contract-20261019/copy-chunks-with-state-helpers.json`，不进入发布包。原IDA库前后SHA256 `324248ac7cd0388a05907c29436fccf804eb82e5cb8433190806342c1cfcbb26` 一致，私有lease已关闭。

汇编支持的精确分支：`1c5f54`/`1c5f79` 向状态helper传入0x400/0x800；`1c605e` 通过command list +0xd0提交积累barriers；`1c608d` 经+0x80复制，返回点1c6093。原始XYZ为0、box为null；后续两次1c617c按scope保存值恢复状态。完整函数还含CopyResource及其他分支，不能按函数名把它们一并允许。缓存伪代码的参数位置及部分常量与汇编不符，不作为ABI接线依据。

据此可走“保持原生状态转换、同资源/列表/顺序，仅限制复制box和目的偏移”的契约，无需伪造状态tracker。但历史代码相同不证明当前资源、实际生产区域或GPU完成；生产端必须验证当前完整chunks、确切返回地址和typed参数，且仍等待Color/Depth/MV生成域证据，不能据此启用DLSS。

## 2026-10-19：控制器持有 typed 四资源

Context现在只从当前有效消费ticket内的typed Barrier/Copy实参获取资源引用，覆盖UAV与alias参数；NGX整数只与已持有对象匹配，不转换成COM指针。首次有效Eval固定Create epoch/world/device及Color/Depth/MV/Output四资源，后续替换拒绝。绑定前的候选有界64项、不同ticket不混用，绑定后仅保留四资源；满容量时非SR或异设备资源仍按类型筛选忽略，新合格纹理溢出才拒绝。资源身份持有不构成producer区域、资源状态或GPU内容证明。

停止、失败、world变化不清空已绑定资源；仅现有detach检查确认wrapper/active/GPU/feature均退休后释放Context持有。frame锁与活动计数保护采集和退出边界。真实首次Eval之前是否能在此窗口取得全部四资源尚未验证；缺项拒绝，不从诊断bank补齐。

协调者实际运行新CMake目标NgxTypedQuartetTest（253项CPU检查）和HostLive回归通过；私有提取的实际退出保护28项检查通过，退休事实使用测试替身。host开启完整capture重新编译通过，Ninja447依赖包含新增header，私有DLL SHA256 `98d29546682c388d595f230afdf2f6367c91755b04654d29500c8720b1abc792`。另用build_verified新建默认功能组合但host关闭的完整构建，462依赖验证通过，未覆盖已验收默认DLL或修改其manifest。此轮未运行游戏、未启用DLSS。

仍缺：真实Color/Depth/MV生产域、完整认证的原生Copy路线及共享参数串行性，之后才可做实际DLSS双目与连续游玩验证。推荐尺寸默认模式不受本次源码准备影响。

## 2026-10-19：实际界值确认整幅宽度下限冲突

首pair完成接线已加入源码：真实Create、两次Eval、两次boxed Copy的提交义务全部在匹配device/queue/fence上完成后，才能接受传输并允许后续ticket。该API不构造像素观察，不声称画质正确。Execute后先非阻塞检查；同步原生调用到达下一ticket而首pair仍pending时，在frame/adapter锁外最多等待2000ms，只等已提交且Signal已返回的工作；stop/world变化或超时停止并保留未退休资源，不重放SDK。私有完整调用链锁审查及CPU回归通过；协调端在修复header依赖的目录完成capture重新编译，Adapter/HostLive测试直接运行通过。真实producer域、原生Copy契约、参数串行性及实际GPU连续画面仍未接通或验证，默认未激活。原始日志 `analysis/runtime/build-ngx-transport-integrated.log`。

后续Ninja构建使用 `tools/native-capture/build_verified.py`：指定新 `--build-dir`、原有 `--define NAME=VALUE` 和 `--target`，保留候选功能组合。工具实际探测编译器showIncludes前缀，校验生成规则与编译器身份，并在构建后拒绝0依赖或缺少指定 `.inl` 依赖的结果；不复用有旧解析缓存的目录。5项离线检查及协调端实际小工程运行通过，另有修改私有header后重编译的回执。它只证明构建依赖被正确解析，不替代游戏/GPU验收。全量原生构建的446项依赖证据见下文。

完整私有构建随后完成于 `build-game-resolution-ngx-optimal`，DLL SHA256 `ce9ef2f8a954c4740105f086392917bc988b017f9764838d8a9c100c34f458ea`，未替换默认清单。期间发现旧Ninja构建的header依赖为0：当前中文MSVC输出UTF-8 showIncludes前缀，被CMake误解码并写入规则；VSLANG=1033不能修复仅有2052资源的安装。使用实测前缀的project-include override后fresh完整构建通过，capture.cpp依赖446项，明确包含本次修改的host/native_api/control_capture。不能将先前“ninja no work”当新源码编译证据。原始构建日志在ignored `build-game-ngx-optimal-prefix-fixed.log`；实际每眼GetOptimal/GPU画面仍未验证。

后续源码已接入每眼 GetOptimal 查询与双feature创建前的参数检查：按每眼输出尺寸和原quality查询，校验输入落入实际返回范围，并在查询前后重新验证world/revision/参数对象/handle/原Create字段。未知或漂移时拒绝创建，不将整幅最小宽度硬除以二。完整helper字节认证也已加入。共享源码重新编译的 NativeBridge/HostLive 两个测试程序直接运行通过；最初CTest名称过滤未选中测试，不计作通过。查询回执仍由测试替身提供，真实每眼GetOptimal尚未运行，完整capture构建及GPU验证未在本轮完成。该接口未激活，真实producer域、Copy状态、GPU fence退休及所有共享parameter调用串行性仍为接线前提。

`dlss-native-menu11` 使用只读界值观察候选 DLL `1c0e6c21e5b1c71c24fb190ad19ab4b76d109860fe36f51c9081d98e4428c1db`，session `8935e971` / PID23756。Create 后与两次接受的 Eval 入口均记录相同参数身份，界值双读稳定：maxW/maxH=3840/2160，minW/minH=1920/1080；Eval ticket1483、revision1/world1，两眼实际输入仍为1920×1440。结合已保存且按完整chunk认证的 `be65cc` 路径（读取view宽，再按 `1ad1960` 族界值夹取），确认1280的每眼构造宽被整幅配置的1920最小宽度抬高。不能据此将所有DLSS问题归结于此，也不能仅改Eval宽度绕过有效范围。

下一步应让每眼输出尺寸、GetOptimal结果、独立feature与输入域一致，再处理真实生产资源和输出生命周期；不修改发布默认以迁就旧整幅路径。此轮没有观察GetOptimal回调，不能宣称其调用已验证。运行时推荐尺寸×倍率仍是0.0.1正式默认，优先于DLSS。

协调端独立确认 session stopped、native_cleanup_verified/entry_restore_verified=true，游戏已退出，5个受保护文件逐哈希恢复。原始状态 `analysis/runtime/dlss-native-menu11-native-status.txt` SHA256 `5523c7c6c383a575d5f80930aa91b21f7cc6d876262f480d93d9d49c057b5d7d`。候选3项CTest及22项启动验证通过；这些结果不等于正确双眼GPU图像或DLSS可玩验收。

## 2026-10-19：输入矩形实验实跑，Eval 宽度仍不一致

当前发布开发基线已切为推荐每眼尺寸×倍率（见主进度文档），优先级高于DLSS。本节为私有game-resolution诊断，不替换默认模式；以下Native70等默认描述均属历史。

`dlss-native-menu10` 在正常游戏菜单启用DLSS Quality并返回关卡，实际加载输入构造实验DLL SHA256 `7ce3dd337fc9fe91604bbb523d4b1cd0a5c080431d47b6737130edd7acd40edc`。session `5672a25a`，PID46600，完整出生身份在ignored运行回执。菜单helper成功、观察期结束，native清理、入口恢复、游戏正常退出以及5文件恢复均通过，协调端再次核验无游戏进程和全部原文件哈希。

冻结epoch revision1/world1/ticket1492：candidate_complete=1、drained=1、dropped=0、evals=2。Create参数为2560×1440→3840×2160。构造两眼source/target为1280×1440，x分别0/1920；prepare前后及Eval时对应view矩形保持不变。但两次NGX Eval实际输入值仍是 **1920×1440**，而不是预期1280×1440。两次typed_before/final均f，dispatch_match=1、result=1。这证明只修构造输入矩形未同步Eval宽度；需继续追踪调用前宽度来源，不可把局部构造结果算作DLSS输入域整体通过。

原始状态 `analysis/runtime/dlss-native-menu10-native-status.txt` SHA256 `1bd23d54c4a0e91250e5b056dfb7a74419fecea9aabd1ef3ee6a802fa7aca588`，关键行188–202、244–252。gpu_completion_verified=0、producer_domain_verified=0；共享历史、输出回写及正确双眼图像仍未证明。本轮不支持“DLSS已可玩”的结论，下一步仅按这个明确宽度差异查保存反编译／代码。

2026-10-18。用户已把 DLSS 超分辨率列为 V1 必做项。当前玩家默认仍为 Native70，DLSS/DLAA 保护仍开启；本记录中的独立候选没有提升默认，也未加入发布包。DLAA、帧生成不在新增范围。

## 已实现的实验入口

`b701095` 增加独立清单字段 `experimental_preserve_upscaler=true`，经 Launcher、reservation 和 debug command 显式传到启动脚本。它只读检查唯一 `DLSS=True`、FSR2/FSR3/MFSR 关闭和唯一质量值3；质量4及其他值拒绝。质量3的 Ultra Performance 名称来自此前相同磁盘版本的配置证据，不推广成完整质量枚举。实验标志与启动或菜单图形锁并用时提前拒绝。默认缺省行为仍强制 OFF。相关92项测试及 PowerShell 语法检查通过。

此入口只保留测试配置，不能证明 NGX 执行、超分辨率生效、每眼历史隔离或图像正确；运行中也不是排除所有 DLAA 切换路径的产品策略。

Native73 从冻结 Native70 原生源独立构建，仅关闭 graphics runtime/startup/menu 三个保护宏，保留只读 graphics observer。DLL SHA256 为 `ad3c543ea9fc40e084d3aa557ae61e572b104aa50967b3c915ebb19dfecf7232`。

## 实际游戏结果：场景接受，输出未发布

- `dlss73-gameplay-20261018` 首轮在 UI 初始化校验处拒绝，native 未激活、没有场景提交；native cleanup 未验证，随后游戏正常退出。不能据此认定 DLSS GPU 故障或正常清理通过。
- `dlss73-scene2-20261018` 暂停 HUD 修正和 world recovery，仅隔离普通场景。原生 normal/alternate DLSS 字段均为1；左右场景分别接受686次，最终 GPU 发布与 XR scene copy 均为0。
- `dlss73-fullui3-20261018` 恢复全部 UI 功能后 native 激活成功，仍然0发布、0 scene copy。因此首轮 UI 拒绝未稳定复现，不能将它认定为 DLSS 输出失败的统一根因。

后两轮 native cleanup 验证通过。三轮均正常关闭自有游戏，完整设置与四个受保护 profile 按测试前内容恢复。均未观察到自有游戏进入实际前台；没有移动或调整窗口、启用实体手柄后台响应或修改全局 OpenXR Runtime。测试使用 quiet2 子进程 Runtime 和 tiny-output observe-only。

原始数据及完整身份留在 ignored `analysis/runtime/`。设置布尔、场景调用和提交计数不代替 GPU 图像或真头显验收。请求的 GPU 快照没有生成场景图片。

## 已缩小的代码位置

新路径实际 quad return RVA `d83cbf`，此前已认证的路径为 `d83b1c`、`d808d8` 等。Native73 记录左目标x约为2³²、每眼目标及source宽2534、右source从1280开始；不是两个合法互不重叠半宽矩形。

已有历史代码节 SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、基址 `0x140000000` 的完整缓存汇编显示：该分支将32位目标原点零扩展后转成浮点，并复用生成的目标宽高作为source宽高。它可以解释负原点变成巨大正数的机制，但日志中的舍入浮点不能反推出精确原整数。需要记录实际生成路径及shader常量后才能确定修正。

现有 final-eye 认证未覆盖该分支；final transfer 无有效源票据，`pair_rejection=16` 对应 ticket_conflict（票据为0也会产生），不是GPU资源地址冲突的证明。正常化在 entry 预检拒绝，符合现有门禁。不能仅加入caller白名单或单独改符号。

`ui-layout-details5-20261018` 在另一正常游戏进程中对 `d82f58` 主chunk3192B、continuation489B、setter `1760b4` 205B进行了完整当前字节双读及SHA核对，全部匹配上述历史chunks；这不是任意后续进程的自动认证。主函数R8是传入view，是否等于TLS current-view仍需运行观察。相关shader setter与DOF保护使用的setter不同，不能复制DOF标量归零策略。

## 有界只读观察器

`a0b90ec` 增加默认关闭的 `GEARS_DLSS_ROUTE_OBSERVER`。独立构建在启用前重新认证上述三个完整chunks，观察输出函数与常量setter，最多保存8条Output、每条最多2个setter和quad。记录传入view、TLS view、原始矩形、采样向量、常量提交与消费bank；不修改参数、资源或任何票据认证。

实际wrapper离线测试覆盖原调用一次、返回值、嵌套/异常恢复、额度耗尽、payload保存及缺读；跨线程try_lock验证原函数执行期间不持观察器全局锁。清理超时且仍有在途owner时禁止读取未完成Row，仅输出有界摘要。测试通过不代表实际游戏或DLSS图像通过。

Native74 独立候选已完成编译与wrapper检查，DLL SHA256 `72941b77148f1647de77389365d586cf80c63d6692f005d82be4dd2a9cae070b`。实际观察结果见下；首发 DLSS 支持仍未完成。

随后实际会话 `continuous-stereo-39168-20261018-023738-83b69260` 完成独立观察：三个完整当前chunks认证通过，8条记录覆盖4张ticket的双眼，传入view始终等于TLS current-view和对应renderer eye。每个Output均恰好一次被观察器筛选的目标setter `d83b69` 及quad；这不是整个Output只调用一次setter，正常先行的 `d8388e`、`d8397d` 不在该计数内。进入/退出view字段一致，没有嵌套或溢出。view的两组矩形均为合法半宽，而 `d83cbf` 收到的仍是上述巨大原点与全窗口范围。

setter `d83b69` 的当前描述符均为 `{65535,65535,0}`、index0、返回0。已核对实现先比较byte_count与index×16，当前是无操作，不访问bank；因此不能将缺少bank记录当故障，也不需要伪造shader常量消费证明。这一轮只读观察仍为0发布、0 scene copy、没有保存到场景GPU图片。native cleanup、正常游戏退出和完整设置/profile恢复通过，未记录自有游戏进入前台。

下一隔离实验仅复制直接分支的矩形语义，同时建立每次scope/draw完成及publication门禁；不能靠宏白名单放行。即使恢复双眼输出，也仍需单独证明实际NGX输入缩小、时域历史与SR效果。

私有 `GEARS_DLSS_OUTPUT_CONTROL` 候选已实现，默认关闭，独立于8条观察额度保存逐票据证明。审查修正了两处问题：仅统计目标setter，正常先行调用另计；实际SRV查询前后核对context当前命令列表的raw/canonical身份及generation，避免把不同列表的资源拼成凭据。实际wrapper的正常路径、先行setter、列表不匹配/漂移、重复/缺失/异常和旧caller拒绝测试通过。完整DLL构建、实际游戏和GPU输出仍待验证，不代表DLSS支持完成。

Native75 已完整编译，三项wrapper/policy测试通过；启动前 `GearsCaptureFrameLinkCheck` 在GPU接收fixture返回179，尚未启动游戏。该旧fixture不执行Consume，新publication门禁因而没有对应完成记录，正确拒绝；已在专用fixture增加Missing/Incomplete必须拒绝及显式完成NoSite后才能通过的断言。未放宽游戏门禁。设置与四个profile恢复通过，下一独立构建验证该修正。

Native76（SHA256 `96c96f0deb058b9e08a0762e8d3c6b7d51225df6ce084e647a605604cb7f3e0b`）的GPU启动自测通过，随后实际会话 `continuous-stereo-916-20261018-025942-2e064f2f` 正常激活，但forwarded/draw/source-query/publication仍为0。native cleanup通过，独立entry恢复验证失败，游戏正常关闭，设置和四个profile恢复，无自有前台采样。审查确认恢复验证器缺少新Output函数分支，已补完整主chunk和continuation固定SHA比较及七项漂移拒绝测试；不能把尚未重跑的验证器修正写成该会话恢复已通过。

另外，完整缓存调用链证明Output可在最终quad前经 `194950` 调用 `194c93` 的1024×32预处理quad。控制曾在caller筛选前计入所有quad，现改为仅计 `d83cbf`，其他调用单列诊断；实际wrapper补了该先行调用用例。旧日志未逐次记录该预处理执行，尚不能宣布它是本轮唯一根因。

Native76实际绑定高水位为10；缓存 `1b4ca0` 将context+65d设为 `max(old,slot+1)`，因此它不表示shader采样10张纹理。主输入绑定候选调用位于 `d831c3`（return `d831c8`），descriptor决定写入context绑定数组的槽。下一步观察实际descriptor、写入对象及draw前同槽是否被覆盖，不通过槽0或唯一尺寸猜源。

针对NGX的有界历史静态分析进一步定位Create/Evaluate共用handle槽RVA `1c20bd0` 和参数对象槽 `1c20bd8`；所读游戏侧完整函数没有view/eye索引。Create尺寸来自第六参数结构的0/4/8/c，Evaluate渲染尺寸来自入口R9结构+30/+34，Reset为+38。这是历史代码的数据流证据，尚未认证当前这些函数或观察两眼handle值，也不能据此断言DLL内部历史结构。恢复矩形输出之后仍要验证实际尺寸、双眼handle及历史状态，首发DLSS支持继续未完成。

Native78 在关闭UI试验的组合中暴露了头文件被包含在匿名namespace内的编译问题，未构建成功或启动游戏。纯helper头现放到全局include区。Native79（SHA256 `411770d1c3112eb66058a565cf8fe03a282e1ab85b271288c858446cd50ec281`）完整构建、输入wrapper和GPU frame-link自测通过。

Native79实际会话 `continuous-stereo-9388-20261018-032303-3e4e9656` 得到541次局部矩形转发；此前预处理quad误计已被消除。541次source查询均被旧单槽/格式24假设拒绝，实际draw证明和publication仍为0。完整命令列表关联通过，native cleanup、所有新旧entry独立恢复、正常关闭及设置/profile恢复通过，无自有前台采样。

新增默认关闭的输入绑定观察器完成 `1665dc` 全部503B的独立当前认证；8条记录覆盖4张ticket的两眼。`d831c8` 的descriptor均 `{0,1,0}`，由真实descriptor选定槽，而非按尺寸搜索。绑定后、quad及actual draw时，选中槽对象始终等于公共资源链解析对象，真实canonical资源为2560×1080、format26、单mip/sample/layer；descriptor/context未改变，draw命令列表身份及generation匹配。绑定前槽内640×270、format10资源是之前的状态，不当作NGX输入尺寸或SR生效证据。

下一私有修正建立独立于8条诊断的BindScope完成凭证，重新核对上述关系，并只扩展已观察的源format26；目标format24和最终传输契约保持。Astra对该CPU绑定凭证复核通过，可用于像素待验的矩形修正实验；它不证明shader采样用途、NGX降分辨率或独立history。

后续默认关闭的控制已接入独立BindScope；它不借用8条观察记录作为许可。启用时另外完整认证1665dc的503B；绑定必须由本次descriptor选择槽，正常完成原始五参数调用，公共对象链与真实format26资源一致，并在quad、实际draw前后及Output完成时复核。缺失、重复（包括draw之后重复）、异常、对象/描述符/列表generation漂移和双眼来源不一致均拒绝publication。目标format24及final transfer未改。实际wrapper三组`/W4 /WX`测试与10项独立恢复验证测试通过；完整DLL、实际GPU及NGX时域验证仍待执行。

Native81从`e758929`冻结，DLL SHA256 `8478dac38c9f96519418ba189f43d0af2a198df773c69a001abef63ea54af2cd`。完整构建、三组wrapper、final-transfer policy及实际GPU frame-link ABI自测通过。实际会话`continuous-stereo-45048-20261018-033857-a8e9d2e7`的完整当前代码认证通过，549次矩形转发和549次actual draw证明，3843次绑定复核，source/list拒绝均0；Output完成仍0，没有GPU图片或XR场景发布。XR以1退出，会话为incomplete；native cleanup、所有entry恢复、正常关闭和完整设置/四profile恢复通过，无自有前台采样。这是绘制凭证的进展，不是DLSS输出修复完成。

有界KB查询及原已缓存完整continuation显示：`d83d53`读取view的family并检查其`+4c` marker，非零时`d83d5b`将`view+3e8`写为1，然后cleanup/正常返回1。Bind/quad/draw样本selector均0，而`binding_current`在Output退出按当前selector重新选择公共bank并要求与原值完全一致。此机制可以导致拒绝，但本轮没有退出处样本，不能认定它是实际唯一失败条件。下一轮只增加有界首次失败记录，分别保存完整性谓词及退出绑定状态，维持现有拒绝；证实正常转换后才能分离draw源凭证与Output生命周期契约。

另增默认关闭的`GEARS_NGX_ROUTE_OBSERVER`，只在完整认证当前Create/Evaluate包装器249/1377B后安装两处只读hook，固定最多4条Create、8条Evaluate。记录原始六/四GP参数、64位RAX、前后公共handle/参数对象槽、输入输出尺寸、Reset、raw资源指针及可认证的world/ticket/eye关联；不解释SDK返回语义、不强转未知资源接口、不参与publication。实际wrapper、默认OFF及13项独立恢复验证测试通过；尚无本观察器实际游戏数据。入口/出口槽值不是中途实际消费handle的证明，历史隔离、GPU与SR标志保持未验证。

Output退出首次失败记录已实现：逐项调用原stable/descriptor/binding谓词并保留原AND条件，保存前后选择状态、字段mask、原槽/实际对象和列表generation；只发布第一条完整记录，cleanup以release/acquire读取，最多四行。正常、仅selector切换、槽变更、缺读、generation漂移及禁止后续覆盖用例通过。记录只用于定位，不授权publication。

Native82（`c7faf64`，SHA256 `a991b62b941a97f17e222c6895f719a826e761fa31b693d54a80bccc2dd11b8e`）完整构建、五组wrapper/policy及实际GPU frame-link ABI自测通过。实际`continuous-stereo-41880-20261018-034815-8ed1daee`得567次绘制证明，0 Output完成和0图片/XR场景发布，XR退出1/incomplete。首次失败receipt表明：所有其他退出谓词通过，input mask1023，selector确为0→1；原绑定槽对象、真实format26资源及list raw/canonical/generation596均保持。当前publicglobal转为新bank后对象不匹配，恰是binding_current拒绝点。该首条记录只覆盖ticket1左Output，后续修正仍须实际验证双眼及后续帧。

同轮当前NGX完整认证通过，记录0 Create和8 Evaluate，后续1132次只正常旁路不写行。未观察到Create只说明激活之后没有记录。row1..6覆盖ticket1..3的三对调用：提交结构render853×360，source base分别0/1280，output raw相同且output base均0；公共handle槽前后相同。只有row2 Reset=1，其余相关行0。两个不同source base调用的执行TLS都指向eye1；它不能用于识别逻辑左/右眼，日志后续改为tls_associated/tls_eye并显式logical_eye_verified=0。相同输出区域可能被覆盖，也可能是暂存后搬运；仍需实际写入/复制与最终像素证据。公共槽快照不证明下游消费handle或独立时域历史。

该轮native cleanup、所有entry独立恢复、正常关闭和设置/四profile恢复通过，无自有前台采样。下一私有修正保留绘制全程严格源绑定，只在Output正常尾部接受已认证marker1的0→1转换，并从原已证明publicglobal0重新核对完整源链；不借新bank替换源或放宽其他身份检查。Astra复核认可进入GPU像素实验，DLSS支持仍未完成。

Native83从`8e2dd6b`冻结（SHA256 `e61542a0e963cf020bcde2ca5b1fb0af2cb1937517dc63d42c3eb1155062918a`），完整构建、五组wrapper/policy与实际GPU frame-link ABI通过。实际`continuous-stereo-43064-20261018-040411-30561c20`完成1108次转发/实际draw/Output；390次完整scope publication，XR scene copy384次、projection layer1411次。允许样本的左右眼tail均单独通过并保留原global0，source/list/pub拒绝为0。3张GPU图像两半均非全黑，已保存原始图和统计；末图左右非黑占比约83.9%/90.2%，平均luma约10.44/16.14。样图近景占比大且两眼亮度不同，需要固定相机的DLSS开关对照，不能只凭非黑宣称像素、SR或history正确。

该轮会话stopped、XR退出0，native cleanup、全部entry独立恢复、正常关闭及完整设置/四profile恢复通过，无自有前台采样。默认Native70及现有开发ZIP未改；本结果只表明该隔离SR配置下已恢复实际双眼输出，不是全部质量模式、CG/机枪、长时间或头显兼容验收。

随后同一Native83进行DLSS关闭对照`continuous-stereo-9000-20261018-040759-2a575dde`：两轮测试前完整设置与四profile的SHA逐项相同，无额外游戏输入，XR CSV的头部位置/旋转及左右眼偏移也一致；未单独保存同步游戏相机POV。OFF图像显示正常人物和全景，ON图像明显只呈现放大的近处墙柱/地面，不能作为正确立体画面验收。OFF有634次scene copy、1416次projection，正常退出/全部清理与设置恢复通过。此对照不是同步姿态/固定GPU负载性能基准。

按Native83实际QPC排序，ticket1..3均为两次NGX Evaluate返回后才进入左/右Output及quad，不能将d83cbf认定在上采样调用之前。quad绑定的真实资源等于NGX Colorraw，Outputraw则不同；这仍不能排除中间复制或原位回写。提交的853×360与view的1280×1080不是同一语义。下一实验应观察一个ticket内的实际资源写入/复制位置及quad源像素/真实SRV，先核实有效区域和输出去向，不继续凭最终矩形猜UV。CPU排序报告和原始图片仅在ignored analysis/runtime。

进一步核对同轮已有原生复制日志：`caller=1c6093` 共2770次完整子资源复制，实际source正是NGX Outputraw，destination正是quad绑定的Colorraw，box为空、目标偏移为0。它支持Output整图回写到最终源的路径，因此不能把“quad绑定Color”单独认定为绑错输入。现有聚合记录没有逐次Evaluate关联与GPU像素；下一私有观察器拟在已存在的真实复制源旁最多读取两个GPU样本，核实实际有效区域与最终画面的关系，不新增资源状态转换或额外Execute。未取得样本之前，DLSS取景异常、逻辑眼/history/SR验证仍未关闭。

Native85从`768813a`冻结（DLL SHA256 `e44db519075ba88922c81d64dd33ab3d92d47814dc91e840711db415e15ece9b`），新增默认关闭、最多两份的相邻CopySource读回；六项实际wrapper/policy检查及实际GPU frame-link ABI自测通过。独立审查发现并修正跨线程Eval/Create可绕过TLS未完成区间的歧义门禁，实际不满足关联时不追加采样。`dlss85-output-pixels-20261018`成功采到同一world1/ticket32的Evaluate65/66：source base分别0/1280，Render.Subrect均853×360、output base均0；每份读回都位于对应Eval正常返回之后、下一Eval之前的原生Output→Color完整复制旁，使用独立列表generation、同一DIRECT queue，无新增barrier或Execute。

两份2560×1080 format26原始数据均在成功物理Reset和fence完成后保存，failure=0、持有引用随后释放。GPU预览在此Output复制点就呈现放大的近墙柱画面，因此最终SBS矩形不能作为唯一原因；尚未区分输入已异常与NGX参数/feature行为。原始GPU数据与解码结果已校验哈希并归档ignored `analysis/runtime/dlss85-output-pixels/`。XR有437次scene copy、1405次projection，但当前Borderless1二进制没有输出hdr_match记录，发布匹配请求不等于取得同ticket最终XR图，不能宣称像素闭环或真机通过。正常关闭、native/entry独立恢复、设置/四profile恢复通过，无自有前台采样，默认包未改。

下一实验先补激活前遗漏的真实Create输入/输出尺寸、quality/flags/subrects及后续handle关联；853×360与整图宽高比相近只是假设，不直接改尺寸或UV。原生复制中的Color是COPY_DEST，不能把它当输入读回的COPY_SOURCE；预Eval输入像素还缺真实状态和执行顺序证据。DLSS SR、逻辑眼及独立history仍未通过。

正常菜单重建试验`dlss85-menu-create-d-20261018`从DLSS关闭状态启动，保留Native85现有观察器，通过实际UI状态导航至Resolution Scaling并发送一次原生窗口Right。前后CPU DLSS/alternate DWORD仍均为0，Create/Evaluate记录均0；因此该轮没有证明启用，也没有取得创建尺寸。返回时实际出现`WarfareConfirmation / UIScene_Confirmation`，正文为`ConfirmVideoSettings`，自动路由因未知确认页停止。后续将只对该精确本地视频确认页补有界处理，并记录反射选项值；不把一次Right、历史脚本的Enter分支或CPU标志当SDK实际生效证明。该轮正常关闭、native/entry独立恢复及设置/四profile恢复通过，无实际自有前台采样；完整状态和哈希已归档ignored analysis/runtime。

后续E/G/H补充只读反射和精确视频确认：SelectedResolutionScaling的3/ULTRA PERFORMANCE到0/QUALITY是质量档变化，并不是启用开关。当前AA数组实际为NONE、FXAA、AMD FidelityFX Super Resolution 3.1、NVIDIA DLSS 3.5；H在AA行两次普通Right后观察到0→2→0，路由因重复停止，仍无Create/Evaluate。这与按键重复或原生跳过选项均相容，尚未区分；编译与对应实际状态均证实三项graphics锁OFF、lock_effective=0，不能直接归咎现有禁用hook。下一实验只记录一次原按键对期间的控件CurrentIndex/SkippedItemIndex及选项索引轨迹，不盲目循环或声明DLSS已启用。各轮均正常关闭并恢复native/entry、设置/四profile，无实际自有前台采样。Create尺寸、输入图像与正确SR输出仍是未关闭项。

I轮只发送一次原80ms按键对，实际反射轨迹为：按下后约13.60ms，CurrentIndex和SelectedAntialiasingSetting从0到1；释放后约23.68ms再从1到2，SkippedItemIndex一直为0。因此FXAA并未被直接跳过；第二次推进发生在释放之后，但尚不能单凭时间顺序区分释放处理与排队事件。该轮仍未启用DLSS、Create/Evaluate均0，正常关闭和native/entry、设置/四profile恢复通过，无自有前台采样。精简轨迹为ignored `analysis/runtime/dlss85-menu-create-i-aa-transitions-20261018.json`。下一轮只向同一精确AA控件发送一次原WM_KEYUP，保留身份、反射字段和稳定菜单认证；不改变普通导航或盲目循环选项。

J轮实际发送且仅发送一次WM_KEYUP，未发送WM_KEYDOWN；CurrentIndex和SelectedAntialiasingSetting从0到1，并在随后200ms采样中保持1，SkippedItemIndex保持0。这证明本控件会响应单独的释放事件，结合I轮可解释普通按键对推进两格；不据此改变所有菜单输入。下一步仅对已认证的Graphics AA/SR选项引入显式单释放步进，以实际当前数组核对QUALITY和NVIDIA DLSS 3.5，再观察Create/Evaluate；本轮仍为FXAA且Create/Evaluate均0，不能算DLSS支持通过。正常关闭、native/entry独立恢复、设置/四profile恢复通过，无自有前台采样；完整helper哈希与状态已归档，精简轨迹为ignored `analysis/runtime/dlss85-menu-create-j-aa-transitions-20261018.json`。

K轮完成上述正常菜单路径：Resolution Scaling逐次3→4→0（QUALITY），AA逐次0→1→2→3（NVIDIA DLSS 3.5），每步均核对控件/资源当前索引及实际数组。菜单输入只向精确自有HWND发送，普通导航未改变，实体后台手柄保持original。Native85实际记录一次Create和八次Evaluate，当前完整函数认证通过：ticket3488创建字段为1706×720→2560×1080、quality_raw2、flags_raw67、output_subrects_byte_raw0；随后ticket3488–3491的四对Evaluate均为1280×1080，Color/Depth/MV base交替0/1280、Output base均0，共用公共handle/parameters和资源；仅第一行Reset为1。该数据证明创建与每view区域的尺寸域不同，不证明SDK接受范围、实际消费handle或异常根因，不能把旧startup-ON/Ultra的853×360当同一轮变化。

两份原始Output读回均为创建当帧ticket3488的Eval1/2，GPU完成、物理Reset后释放及原始复制点契约通过，failure0；原始数据、解码和哈希保存在ignored `dlss85-menu-create-k-20261018-artifacts/`。根协调检查了两张完整Output及后续XR预览，房间与人物可辨，但当前XR图不与同ticket对应，既不能据此宣布完整像素正确，也不能把旧轮近墙取景直接套用本轮。现有绝对ticket>=32门禁在创建3488前已经满足，故本轮只读到了最初一对，未覆盖菜单恢复后的晚期输出。下一步先取得同handle的晚期pair、view矩形/缩放来源与对应最终图，区分尺寸域、取样区域和转换过程，暂不猜改尺寸或UV。正常关闭、native/entry独立恢复和设置/四profile恢复通过，无自有前台采样；默认保护与玩家包未改变，DLSS SR仍未验收。

L轮保持Native85不变，换用只启用现有匹配快照代码的私有XR match1（提交3b7396b，SHA256 `30d4d978a9613dd0bca665fd37560baf682c2c0e7c0f11e3b1fff688fe760ca5`）。完整来源与四项离线检查在ignored `xr-match1-build-receipt.json`；新旧Packet/Publisher前缀相同，不能把旧Borderless1缺少接收功能解释为Native没有出版请求。实际接收器选中了world1/ticket3535/copy_seq2，但这是画质菜单内创建当帧；恢复场景时实际新图已到ticket4504，明确记录`hdr_match_missed/ticket_passed`。因此本轮仍无对应最终图，验证了接收链和采样时机缺口。正常关闭、native/entry、设置/四profile恢复通过，无自有前台采样，默认XR未替换。

`5e66fb8`新增默认关闭的`GEARS_NGX_OUTPUT_DELAYED_CAPTURE`。它独立于四条Create/八条Eval观察额度，以完整前后Create括号建立epoch；worker仅接受当前进程/会话/world/epoch的一次性Resume请求。等待32个完整稳定pair后才选择下一pair，jitter可正常变化，其他签名漂移或不完整段重计且不延长30秒期限。已选中段失败只quarantine，保留原GPU退休契约；在`frame_link.present`后、同ticket最终capture前才按精确两次Eval闭合并出版。没有新增原生hook或修改尺寸/UV。独立审查发现的入口缺读/参数变化缺口已修复并补用例；原函数保持恰好一次。

Native92从该提交冻结，完整DLL构建及十项相关检查（含实际DLL frame-link自测）通过，SHA256 `1869b93ded9a5e5e60f42287283679efd563e9e28fa88fd5e1536e712a563ec2`。这是采样控制候选，不是DLSS兼容修复。M驱动在正常Resume后核对projection恢复及新入场图的保存记录，再写当前身份绑定的私有arm文件；姿态CSV更新、请求已发或Native出版均不冒充最终匹配。默认Native70、Borderless1和玩家包未改变。

实际`dlss92-menu-pair-m-20261018`正常完成Quality/DLSS菜单路径并接受一次Resume请求；选中world1/ticket5070后，最终状态为selected1、pair_closed0、gpu_saved0、terminal_reason13（selected_invalid），resets13。未产生该进程的原始Output文件，也未取得对应最终图，不能算晚期像素闭环。当前日志保留两条正常selected Eval/copy与该ticket的Present拒绝记录；通用selected_invalid会覆盖最初原因，尚不能仅凭最终码唯一断言最早失效边界，更不能视为DLSS取景问题根因。

本轮正常关闭游戏、native/entry独立恢复、设置/四profile恢复与自有arm文件删除均通过，XR退出0、无自有前台采样。精简proof和完整状态及已核对源码已归档ignored `dlss92-menu-pair-m-20261018-proof.json`、`dlss92-menu-pair-m-20261018-artifacts/`；构建receipt已如实标记game_tested=true、matched_pair_capture_passed=false。后续先定位选中段的首个失效边界，再决定最小修正或下一采样，不放宽成对关联、GPU完成或图像验收条件。

## 2026-10-18：晚期同帧闭环及输出反馈证据

`dd7cbe1`保留首个选中段失效的原始原因、具体入口、QPC、pair状态、incoming owner和实际Present结果，不改变选择、拒绝或GPU退休规则；所有新增状态键使用`invalid_`前缀。Native93从该提交冻结，DLL SHA256 `fd921e95bad0d6faed6faf4460a6d9cb1c554ec0fc358a6a0793880f931caee7`，完整构建及十项相关检查通过。该版本仍是诊断候选，默认玩家保护未变。

实际O轮正常菜单启用Quality后恢复游戏，一次Resume请求、32个完整稳定pair、零reset，选中world1/ticket4844。两份原始Output均正常复制/执行/Signal，成功物理Reset及fence完成后保存并释放，failure0；最后pair_closed1/gpu_saved1。私有XR match1实际保存uncached snapshot2，actual/selected ticket均4844、world均1，source_serial3849。此轮首次取得晚期两份Output与最终双眼图的精确同帧闭环，不只是发布请求或后续邻帧图。正常关闭、native/entry独立恢复、设置/四profile及自有arm清理均通过，无自有前台采样；原始数据仅保留在ignored `dlss93-menu-pair-o-20261018-artifacts/`。

根协调查看完整三图后，使用显式区域比较进一步核对：Output2与Output1右半横向放大后的亮度相关0.99908、XY梯度相关0.99327；整幅Output1及其左半的梯度相关仅0.04306/0.03952。最终左眼与Output2左半的梯度相关0.95035，最终右眼与Output2右半为0.94066，其余候选最高0.08041/0.05275，最佳平移均零。比较使用同一HDR预览曲线、640×270网格与小范围平移，不把曝光/后处理差异当作像素字节相同；完整方法和哈希在ignored `dlss93-o-region-comparison.py/.json`。

这些像素证据表明本轮最终双眼取的是第二份输出的不同半区，第二份输出又高度对应第一份的右半放大，不能作为两份正确独立眼图验收。结合原生完整Output→Color回写和第二次Eval的Color base1280，强烈支持首眼整图回写破坏后续眼输入的反馈假设；但仍需核对实际Evaluate命令列表/代次及原生调用合同，或做一次严格隔离实验，才能把外观对应提升为确证的GPU依赖。不能据此只改最终UV或解除默认保护；输入/输出尺寸域、每眼历史、正确SR、场景回归和真机验收仍开放。

## 其他主线进展

`50cbfcd` 为 MenuReader 增加可选的 Position/DockTargets 原始布局采集，70项离线测试通过。普通采样不增加这些读取，未改变UI位置。实际普通双目回归 `ui-layout-details4-20261018` 提交1359次、scene copy723次，约15.08秒，最大提交间隔28.7181ms；正常清理和配置恢复通过。岔路详细采集初次遇到导航碰撞、脚本结果字段及注册表变化；后续 `ui-layout-details7-20261018` 已取得完整13节点及布局详情，见[文档155](155-path-choice-position-and-docking.md)。这仍不是UI修复验收。

机枪方面，实际函数发现已把 `PlayerUpdateCameraNative` 的exec映射到 `3b3af4`，其拆包后直接调用已有 `110878`；炮塔 `GetBaseViewOffsets` 的exec为 `1248238`，实际实现经模式对象虚表调用。发现不等于全部当前函数字节或实际挂载验收。现有准星配对证据是 AssaultRifle，不是手枪，不能用于宣布用户报告的手枪外圈问题已修复。
### Native95/P：一次首眼回写省略的实际因果对照

从提交`5dd8a82`冻结Native95，DLL SHA256为`385fb4b323cf5e97976bc076edf8055145da047bff2ce859c358ba5144041b57`。11项相关检查通过。该私有默认关闭实验只在完整认证的稳定pair中省略第一次原始`Output→Color`整图复制，仍通过未过滤的原函数追加Output读回；第二次原始复制、Create/Evaluate参数、视图、UV及资源状态转换均保持原样。不是玩家修复开关。

实际P轮完成正常菜单Quality/DLSS选择及Resume，选中world1/ticket5349。首眼omission_count=1、original_forwarded=0、readback_started/returned=1；两份2560×1080、format26原始Output均在fence完成和物理Reset退休后保存。最终XR源图通过actual/selected world与ticket精确匹配，snapshot_index2、cached_image=false；不以请求成功或序号相等代替实际匹配。原始数据、独立清理与哈希证据归档于ignored `analysis/runtime/dlss95-menu-pair-p-20261018-proof.json`及同名前缀artifacts目录；归档仅复制，未删除构建目录原件。

复用O轮同一NumPy/PIL区域比较方法，Output2与本pair的Output1右半放大图的最佳梯度相关从O轮0.993269降至P轮0.039281。协调者查看完整读回与同帧最终图，确认P轮Output2不再只是Output1右半放大。此单次干预支持首眼整图回写对立即输入反馈有因果参与；没有直接取得Evaluate2之前的Color输入像素，也没有证明vendor内部history。

最终左、右面板仍分别与Output2的左右半图高度对应（梯度相关0.944376/0.909643），因此省略首眼复制并没有实现正确双眼合成，不能作为DLSS SR修复发布。下一步须保留各眼独立输出并正确缩放/组合，继续核对Quality的创建/渲染尺寸和时域history。对照仅一pair，不是性能、所有画质或真头显验收。

P轮诊断完成，正常关闭、native及所有entry独立恢复、设置及四profile恢复、arm精确清理均通过，XR退出0，无自有前台采样。默认Native70/Borderless1及Feedback3开发ZIP未变。

### 分眼布局与feature寿命策略已写入源码，尚未接入SDK

提交`88a6875`增加纯CPU的分眼布局/单box复制计划及两个feature的创建、逐眼使用、失效和GPU退休策略。输入尺寸必须由已验证生产端明确提供，不从Quality枚举猜比例；每眼输出、物理纹理和padded输入槽保持不同语义。完整输出复制的两个正常边界将分别只复制有效眼图到对应槽，仍使用原Color资源与最终shader。策略本身不调用SDK、不修改视图、不提交GPU命令，不能算DLSS支持已经实现。

协调审查以P轮实际sidecar修正了初稿“共用list/generation”的错误假设：两眼实际分别为list161b4d9b30/gen1988765和1150860d0/gen1988767，在同一queue595086d0依序执行。CPU策略现保留每眼各自的list/generation；每个feature提交必须匹配自己的记录，GPU顺序仍须由实际adapter的执行/fence证据认证。未被接纳的私有Create返回句柄有明确caller-quarantine归属，不静默遗失。

两个CMake策略测试以`/W4 /WX /UNDEBUG`构建并通过，覆盖边界、跨眼输入覆盖、实际两list模式、部分Create失败、独立handle、过期epoch、未提交/未完成GPU工作、错误queue/fence及重复释放。实现尚未接入活体SDK；源构造的实际低分辨率生产、物理Color输出尺寸、两个feature的正常重建/退出以及图像/时域正确性继续保持未验证。具体接入设计和完整历史chunk身份在ignored `dlss95-p-correct-output-plan-20261018.md`，不打包知识库或反编译资料。

## 2026-10-18：当前运行代码合同认证与隔离组件

从历史完整chunk恢复Create、四参数Evaluate（callback为0）、Release与32位BAD码判定，分别保留完整来源哈希；Create的0x19仅代表已认证封装的完整消费范围，不冒称结构体sizeof，Evaluate保留调用者初始化的完整0x168字节。独立adapter与输出绘制关联组件已加入严格CPU测试，仍未接入真实SDK Hook。

实际Native101后台游戏3432中，只读采集对Create/Eval/Release dispatcher、callback helper、结果格式化器及相关原生封装/owner完整chunks逐字节核验并末端复读，all_selected_chunks_matched=true。三个vendor函数指针均落在实际加载的_nvngx.dll（文件SHA256 66767018c36b3bab46398dade3adf173daa3730fda75965689ea848c9bc4e79b）。精确session与全部函数哈希保留在ignored `analysis/runtime/pistol101a-20261018.dlss.json`。这认证所选代码和指针归属，不证明vendor内部行为、资源同步或DLSS图像正确；preset helper等未选代码不自动取得认证。

输出关联组件明确区分记录位置、真实Execute/Signal顺序及完成条件；同一次Execute中的不同list成员顺序不当作资源依赖。尚需实际生产域、整个参数封装的排他、Create/Eval GPU退休、两眼独立history及原生Release返回政策。默认Native70与DLSS/DLAA保护均未变，DLSS仍为首发未完成项。


## 2026-10-18 原生分眼桥接组件（尚未接通真实SDK）

新增NativeWrapperBridge与NativeDispatchBackend：完整wrapper期间参数排他、独立双Create句柄、逐眼Evaluate及history reset、按真实完成凭据释放双句柄；关闭路径保持原始参数和返回值。新增output receipt到quad几何转换，遵循destination→source参数顺序。FeaturePair明确拒绝D3D12 UINT64_MAX完成哨兵。六组MSVC /W4 /WX /UNDEBUG组件测试通过；UiGroup105整合构建中的桥接与geometry测试再次通过。

这不是DLSS已可用：真实Color/Depth/MV生产区域、typed CopyTextureRegion接线、Execute/Signal完成、首次双眼图像隔离和安全Release仍未闭环；默认DLSS保护保持。完整接线说明与测试日志存于ignored analysis/runtime/dlss-integration.txt与ngx-stereo-native-bridge-test。

## 2026-10-18 默认关闭的原生生产链观察器

新增 `GEARS_NGX_STEREO_HOST_OBSERVER`，将完整wrapper上下文与Create/Evaluate/Release dispatcher、带类型的D3D12资源观察接通。仅采样，不激活独立SR，不修改参数、GPU状态或全局句柄。完整历史代码块认证后才安装；资源生命周期、DSV解析与GPU退休仍显式标为未证明，Execute返回不等于GPU完成。默认发布清单不变。

宿主组件测试和启用选项的capture编译通过；协调端再次构建并运行宿主测试。三个dispatcher恢复验证使用固定历史完整块哈希，独立于Hook自报状态；17项Python验证测试通过，包括末端字节改变与短读拒绝。没有启动游戏验证此观察器，也没有DLSS双眼图像验收。接线、证据与限制见ignored `analysis/runtime/dlss-host-handoff-20261018.txt`。

## 2026-10-18: Capture integration and bounded command ownership

Capture now forwards the real constructor, CreateCommandList, Reset, Copy and Execute boundaries to the NGX control integration. No activation call is installed: the default owner remains absent and the ordinary rendering path remains in use. Producer-resource/domain evidence, exact copy-state/dependency evidence and final-output/retirement integration are still required before enabling game DLSS.

The initial integration retained every direct command list in a 128-entry cache. Historical game observations exceeded that capacity, so it was replaced before runtime deployment. Only actual NGX recording and in-flight obligations retain lists. Reclamation requires both an observed Reset and completion of the relevant GPU fence; unrelated command lists occupy no persistent slots. A prepared context failing before owner activation does not stop normal stereo rendering.

Root applied the integration and lifetime patch, built the fresh host-OFF `build-runtime-resolution1-ngx-wiring` candidate and passed the DLL continuous self-test, 3,652 real GPU queue checks and 66 typed-resource/boxed-copy pixel checks. The queue tests use synthetic SDK calls. The agent also built private host-ON/native-owned and host-OFF capture configurations. These are integration evidence, not proof that game-menu DLSS produces correct stereo or survives quality changes. No player manifest or release package was changed.

### 2026-10-18: bounded real epoch/pair evidence candidate

`eba6be2` and `ba312c5` add a default-OFF epoch/pair observer at the existing constructor/prepare, typed Barrier/Copy, NGX Create/Evaluate/Release and Execute/Reset boundaries. It preserves up to three summaries of wholly empty epochs before rearming; any pair evidence freezes its epoch. Candidate completeness explicitly does not certify producer coverage, logical eyes, resource state, GPU completion or queue dependencies. SDK parameters and GPU work remain unchanged.

Root built and linked `build-runtime-resolution1-ngx-pair/GearsCaptureRuntimeNgxPair.dll`, SHA256 `65933b1e69e061417b71e72867389fc2cfff645d7d6596feca82f51970fda0e4`, with host and epoch observers ON and no control activation. Both registered probe tests and the DLL continuous self-test passed. The new actual-callback test uses real D3D12 Barrier/Copy/Execute/Reset with synthetic SDK scopes. No game run has yet used this candidate. The next bounded game sample must provide the missing typed Depth/MotionVectors identities, dimensions and actual pair transitions; these remain absent from historical Color/Output-only evidence. No player manifest/package was changed.

### 2026-10-19: prevent unrelated command traffic exhausting epoch evidence

The actual `dlss-native-menu4` sample selected NVIDIA DLSS 3.5 / Quality in the
reflected game menu. Native Create reported 2560x1440 input and 3840x2160 output,
but 994 of 1024 recorded events were global Reset/Execute traffic. Capture ended
at the unchanged event limit before any Eval. Menu navigation also hit its
90-second parent deadline; return to gameplay and stereo output were not proved.

Execute/Reset records now require a list recording previously introduced by a
selected Barrier, Eval or Copy. Pending Eval/Copy wrong-generation submissions,
failed Reset, duplicates and all selected barriers remain observable. Mixed
batches preserve original member/count and CPU call order; return membership is
frozen at begin. Excluded counters and `earlier_history_complete=0` make the
missing earlier history explicit. No SDK mutation, control activation or extra
GPU work is introduced; relevant-event overflow still stops at 1024.

Root rebuilt the game-resolution sampling DLL, SHA256
`26bac014cbd99ca98ee89578e5fb6a6b0e5a0387670a26f3e52506bae128a0aa`.
Both registered epoch tests and all 22 startup checks passed. The focused tests
include 747 unrelated prefix events, mixed batches, pending generation failures,
duplicate submissions and genuine capacity exhaustion. This remains an observer
candidate, not released DLSS support. No game/simulator launch or player package
change was performed for this revision; actual menu pair evidence is still due.

### Actual scoped menu6 sample

The game-menu DLSS Quality route now completes and returns through the expected
UI route. The probe retains 99 events and 13 typed resources, excluding 324
unrelated Execute members and 569 resets. It no longer hits event capacity, but
stops at the first selected Eval input rejection (`stop=3`, zero accepted Evals).
Create again reports 2560x1440 input and 3840x2160 output. Separate native route
samples show Evaluate calls, including different Color/Depth/MV base offsets;
these are not sufficient to establish valid per-eye producer dimensions or
history. The combined input rejection currently lacks a per-condition reason;
that must be resolved before changing SR parameters. GPU publish remains zero.
Native cleanup, entry restoration, normal exit and settings restoration passed.
Evidence is archived under ignored `analysis/runtime/dlss-native-menu6-*`.

### Typed command-list identity join candidate

Menu6 selected Eval raw command-list addresses did not occur among its 99 recorded events. The previous event-only join therefore could not establish canonical identity/generation. The observer now keeps at most 64 typed Create/Reset candidates and two pinned Eval leases. Eviction loses evidence rather than manufacturing identity; failed Reset, closed creation, world/generation mismatch and tracker capacity loss reject. There is no conversion of an NGX integer into a COM pointer, no event fallback, and no GPU/state/producer certification. Leases release after callback drain; the first rejection records its specific mask and tracker diagnostics.

Root rebuilt the sampling DLL, SHA256 `a969235e2f63681e86986cb334bf4550bfc7869b1d3770431b298e74202fadb6`. Three registered NGX tests and all 22 startup checks passed. The new real-D3D lifetime test exercises 600 candidate creations with synthetic tracker generations. No actual game run has used this revision; defaults/packages remain unchanged. Evidence: ignored `analysis/runtime/ngx-sampling-startup-suite-typed.json` and `ngx-typed-list-join/`.

### Actual menu7 sample and Copy caller-unit correction

Menu7 accepted two Evaluate observations with typed command lists and resources,
one Execute each, but recorded no copies. It retained 237 events and 41 resources;
six dropped observations remain unexplained. The UI route stopped after a sent
key when its post-key snapshot exceeded the timeout; no uncertain input was
replayed. Normal exit, entry restoration and five settings/profile restores
passed. This is not playable DLSS or GPU output acceptance. Evidence remains in
ignored `analysis/runtime/dlss-native-menu7-*`.

Source inspection identified a definite copy-observer error: the original copy
hook passes `_ReturnAddress() - base` through unchanged, while CopyScope compared
that RVA with `base + 0x1c6093`. CopyScope now compares with RVA `0x1c6093`.
The real-D3D regression sends a correct RVA, an absolute address and a foreign
RVA, checking that only the correct observation is retained and all original
CopyTextureRegion calls still execute. It does not attribute the six drops to
this filter or prove that menu7's selected TLS scope contained the missing copy.

Root rebuilt the candidate (SHA256
`219fa9e072ac96013ee8ec0c09849e33011011f957f2e50de152757e4f22d60b`)
and passed all three registered NGX tests. The prior live fixture incorrectly
supplied an absolute address; the corrected fixture now follows the real hook
contract. No game or simulator was launched for this revision. A fresh actual
sample is still required; release defaults and packages remain unchanged.

### Actual menu9: complete CPU-side Eval/copy sequence

The rebuilt RVA-corrected candidate completed the actual game-menu DLSS route
and returned to gameplay (game PID 24188; exact identity in ignored
`dlss-native-menu9-20261019.json`). The selected ticket 1528 recorded two Evals,
one returned copy per Eval and one queue execution for each Eval/copy recording.
The probe reports `candidate_complete=1`, `dropped=0`, `stop=1`; all four resource
identities were typed before and after both Evals. This validates this bounded
CPU observation only: GPU completion and producer-domain verification remain 0.

Create input/output is 2560x1440 / 3840x2160. Both Evals report 1920x2160, with
Color/Depth/MV base X 0 then 1920; both use the same raw CODM resources. Both
recorded output-to-color copies use subresource 0, no source box and destination
0,0,0. These shared resources and copy extents require domain/history analysis;
they do not establish correct per-eye pixels or playable DLSS. The observer
published zero GPU pairs and the overall session remains incomplete.

There were 389 valid occlusion samples, no occlusion/error records and no game
foreground sample. Native cleanup and independent entry restoration passed;
the game closed normally and all five settings/profile backups were restored.
The earlier menu8 attempt cancelled before native activation on a changing
desktop snapshot and supplied no DLSS evidence. Full native status is preserved
under ignored `analysis/runtime/dlss-native-menu9-native-status.txt`.

### Menu9 source/domain review and next implementation boundary

The typed Color and Output descriptors are both 3840x2160, one mip/layer/sample,
format 26. With subresource zero, null source box and zero destination, the
recorded Copy commands request full-subresource replacement, not eye-local
composition. Copy0 serial 134, Color/Output transitions 136/137 and Eval1
138/139 share list/generation `3d7fc1a20/416010`; Eval1 reads the same Color.
This establishes recorded command ordering and the input feedback path, not
GPU completion. Both Evals also use the same public feature handle; the mod
has not supplied independent per-eye feature histories in this run.

The next implementation boundary is the existing NativeCreateSource split-input
selection feeding RenderInputBounds and the constructor/ticket path. This
sample's Create-derived split is 1280x1440 per eye, stride 1920, output
1920x2160 per eye. Merely replacing Evaluate dimensions would leave producer
rendering inconsistent. Constructor inputs must carry the matching immutable
Create epoch through deferred prepare/consume; Create metadata alone cannot
certify actual Color/Depth/MV producer bounds or GPU ownership. Existing
independent-feature and boxed-copy code remains inactive until those facts
are connected and verified. No new runtime acceptance is claimed here.

### Create-derived constructor experiment integrated

`GEARS_NGX_INPUT_RECT_EXPERIMENT` remains default OFF and requires the epoch
observer plus the game-resolution diagnostic configuration. It reuses the native
Create split-input path for real constructor arguments; the full Create identity
is copied into the submitted frame and checked across constructor, prepare,
consume and wrapper boundaries. Observed identity loss stops the experiment
without skipping the original native callbacks. This is not SDK activation or
GPU producer certification. The diagnostic uses game-resolution output to
isolate this change; runtime-recommended resolution times launcher scale remains
a required 0.0.1 delivery feature, not a deferred feature.

Root's full DLL build exposed a header namespace collision not covered by the
private test harness. Moving the shared value type to `gears_ngx_create`, with
an alias in the host namespace, fixed the integrated build. Both experiment ON
and OFF full DLL configurations compiled. Seven targeted tests and all 22
continuous-startup checks passed. Candidate:
`build-game-resolution-ngx-input/GearsCaptureGameNgxInput.dll`, SHA256
`7ce3dd337fc9fe91604bbb523d4b1cd0a5c080431d47b6737130edd7acd40edc`.
Root receipts are ignored `ngx-input-root-tests.json` and
`ngx-input-startup-suite.json`. Actual reduced-input producer rectangles remain
unverified until the next game sample; shared history and whole-output copying
are still unresolved.

### Descriptor provenance foundation (2026-10-19, CPU only)

Added `ngx_mv_descriptor_provenance.h` and its isolated CPU fixture target.
The bounded bank retains typed resource/heap identities, tracks descriptor
copies and overwrites, and rejects stale snapshots, scope/device mismatches,
overlapping heap ranges and exhausted capacity. Unknown source descriptors
remain unknown after a copy. `producer_authorized` is always false.

Root compiled with MSVC `/W4 /WX /EHsc /UNDEBUG` and ran successfully.
Ignored evidence: `analysis/runtime/mv-descriptor-provenance1/receipt.json`,
`compile.log` and `test.log`. Tested header SHA256:
`8c1ee90a57918d689f7881a652c7040aa686f735989568a8b5874914653c0f8f`;
test source SHA256:
`b0d186191917dc4abb7650ba1be9788e2b4b034564f274e5b1fb7afe61d04519`.
These are CPU fixtures, not real COM/GPU or playable-DLSS acceptance.
The helper is not wired into runtime hooks and does not change release behavior.

Source inspection of `capture.cpp::begin_target_prime` confirms the current
early descriptor hooks cover CreateRTV and descriptor copies, not CreateUAV or
CreateDescriptorHeap. Starting a bank at a selected Eval ticket therefore does
not recover earlier UAV creation. Integration must retain actual early typed
creation evidence with its original lifetime/scope, or demonstrate actual
recreation; it must not relabel historical creation as the selected ticket.

Independent architecture review leaves the two-Eval/per-eye-feature route as
the supported candidate, not a proven cure. The next bounded experiment must
join the selected MV resource to copied UAV descriptors, root table 3 and the
actual Dispatch with list generation and region-defining data. Bound UAVs alone
do not prove shader writes. Producer validation must establish coverage and
mapping of the consumed region; an exact equal-sized generating rectangle may
be unnecessarily restrictive if real shader evidence establishes a larger
valid domain. No guard is relaxed on that hypothesis alone. Runtime-recommended
resolution times launcher scale remains the higher-priority 0.0.1 default.

The subsequent CPU helper revision adds an explicit device-lifetime scope:
its epoch is an observation capture generation and world/ticket remain zero.
Selected-ticket scope is a separate kind; lookups cannot relabel early creation
as a later Eval. `invalidate_if_tracked` clears a tracked UAV on SRV/CBV writes
without allocating cells for unrelated descriptors. Fresh MSVC compilation and
all CPU fixtures passed (`analysis/runtime/mv-descriptor-provenance2/receipt.json`).
Header SHA256 `50264634408baf7ccf1f872df4489a68634438778bae9c5e48675fc09dfb6343`;
test SHA256 `3890d62e32052b411af8a7ff8b753afd2fa78e1b6a4e06302bafa012d772ee35`.
Early hook installation, sparse descriptor-copy handling, selected-use binding
and Dispatch evidence are still outstanding; this revision is not in the game
capture path. The existing prime injection also occurs after loader dependency
discovery, so even early hooks must report missing births instead of assuming
that every descriptor was created after installation.

Sparse copy APIs now propagate known UAV provenance, erase tracked destinations
when the source is unknown, and skip allocation for unknown-to-untracked copies.
They share the original range/overlap/version validation; original copy behavior
is unchanged. Root's fresh CPU test run passed, including full-capacity overwrite
and skip cases (`analysis/runtime/mv-descriptor-provenance3/receipt.json`).
Header SHA256 `93ade51057bce6f7035644812310d09b6d2527d7b87fd4ffc7e06dfc2a8ed35b`;
test SHA256 `0cf4221d0d6b257f32dedc0261ca7c7da293a57faa4082f05444f51ff7f2a9be`.

### Early descriptor hook candidate, default OFF

`GEARS_NGX_MV_DESCRIPTOR_OBSERVER` appends CreateDescriptorHeap/CBV/SRV/UAV
observation to the existing prime and viewport hooks, and shares the existing
descriptor-copy callbacks. The first observed CBV/SRV/UAV heap selects one
bounded, explicitly unattested device candidate. Other devices are counted;
unknown births and capacity/order uncertainty invalidate the observation.
Original calls still run once. History survives prime handoff and world changes;
closed admission and drained callbacks precede retained-lease release.
The status explicitly reports no selected-pair or producer proof.

Root built `build-ngx-descriptor-early4/GearsCaptureNgxDescriptorEarly4.dll`,
SHA256 `37b4f2849ca726346b6a3ba063198f8c12aafec58d4a42d193c6ec5b493e9c5c`.
Fresh build recorded469 dependencies; CPU provenance test and the existing
continuous startup suite passed. Separate `--prime`, `--prime-handoff`,
`--prime-owned-cancel`, `--prime-owned-exit`, `--prime-owned-handoff` and
`--continuous` invocations all returned0. These prime fixtures did not create
a CBV heap, so they verify hook lifecycle/restoration, not the new typed
descriptor callback data path. A dedicated real-device fixture and selected
MV/Dispatch association remain required before an actual-game producer claim.
No default manifest or player DLL changed. Startup-trace ON remains unverified:
that option requires a different game-resolution diagnostic configuration.
The first three fresh configure attempts rejected missing NGX dependencies or
the incompatible source-pixel observer, before any binary was produced.
The observer-OFF configuration also passed a fresh build, CPU test and continuous
startup suite: `build-ngx-descriptor-off1/GearsCaptureNgxDescriptorOff1.dll`,
SHA256 `61fff9f378ac0f349c6a7faa8459176b5fa3089b464cd74dce8705fce6a44db8`.

### Real-device descriptor callback fixture

`CaptureSelfTest --ngx-mv-descriptors` now creates real CPU/shader-visible heaps
and UAV resources through the prime-patched D3D12 device methods. It checks
typed identity, both descriptor-copy APIs, overwrite versions, unknown-source
invalidation, SRV/CBV replacement, retained historical snapshots and drained
teardown with original entry-byte restoration. It issues no Dispatch and proves
no shader coverage. The observer-OFF build explicitly returns unsupported.

Root's fresh game-resolution diagnostic configuration with startup trace ON
passed this fixture, CPU provenance tests, the live epoch fixture, continuous
startup suite and all five prime modes plus continuous stop. Artifact:
`build-ngx-descriptor-game1/GearsCaptureNgxDescriptorGame1.dll`, SHA256
`47ad57ea19d74219eeedce555d89aaf31e0575deffce4642f43ab8efba41a1ae`.
Evidence: `analysis/runtime/ngx-descriptor-game1-tests.json` and selftest log;
build driver `analysis/runtime/build-ngx-descriptor-game1.py`.
Constructor-input experimentation is OFF to isolate descriptor observation.
Game-resolution output is private diagnostic isolation, not a change to the
required runtime-recommended-resolution release default.

### Actual early-observer run: heap bound exhausted, not a producer failure

`dlss-descriptor-menu1-20261019`, session36016, used the above exact Game1 DLL.
The native-menu helper enabled SR and returned to gameplay. The selected
world1/ticket1334 recorded two Evals with typed-before/final masks15 and completed
the existing CPU pair probe (no GPU completion or producer-domain claim).

The independent early descriptor observer failed at its ninth observed heap:
heaps8, cells2, heap_calls9, UAV calls2, copies138, other devices0, missing births0,
recursion0 and loss3 (capacity). Thus the heap limit, not the UAV cell limit,
prevented meaningful selected-MV provenance. This does not show missing motion
vectors or a vendor/runtime defect. Blindly interpreting the incomplete map
would be incorrect; a bounded heap census or justified observation-capacity
adjustment is needed before the selected-MV join experiment.

Native cleanup, entry restoration, normal game close, owner exit0 and all five
protected-file restores passed. Final observer status reports released1.
Archived raw evidence: `analysis/runtime/dlss-descriptor-menu1-native-status.txt`
and `dlss-descriptor-menu1-prime-status.txt`; lifecycle receipt:
`analysis/runtime/dlss-descriptor-menu1-20261019.json`. No public default changed.

## 2026-10-19: selected MV seed and future-consume core

Fresh build `build-ngx-target-core1` passed dependency verification (457 includes), continuous startup suite, descriptor provenance/census tests, target-window CPU tests, live epoch fixture and native descriptor/prime/handoff/continuous fixtures. DLL SHA256 `a5229ce02566dd3c33352784060e260378847f55db060370a1bc7539347e7f92`. Receipt: ignored `analysis/runtime/ngx-target-core1-tests.json`; full output `ngx-target-core1-selftests.log`.

The seed API imports retained typed heap leases and re-reads a current source cell before retaining its selected MV resource, counter and view. Target versions remain local; immutable source provenance survives alias copies. Tests cover stale/forged/foreign observations, invalid imports, and resource ownership after source retirement.

The independent CPU window excludes the selection ticket, permits a different renderer on the next newer consume, and requires stable world/Create revision/MV plus one or two successfully returned Eval calls. Deadline, nesting, identity changes and missing/unfinished Eval terminate the observation. A frozen window is scheduling evidence, not stereo or GPU producer proof.

These cores are not yet wired into runtime command-list/Dispatch observation. No game was launched for this step; player default and release package remain unchanged. Next: integrate bounded selected-source descriptor capture and command-list state with attested runtime callers, then collect actual-game evidence.

### Target runtime integration constraints

Review of current WrapperScope and hook_consume confirms that the independent target tap must precede the diagnostic Eval quota return and must not depend on the completed epoch-pair probe. Selection retains the authenticated NativeCreateSource and typed MV lease. Later structure+0x20 values are compared only to that held lease; they never become COM pointers.

Consume begin uses the installed frozen ticket and checked current ownership. Consume end uses saved identities, because original_consume can free the renderer. Wrapper checks must use native_create_matches: native_create_state deliberately rejects an active wrapper. Independent SDK receipts must require exactly one matching command/handle/parameter invocation, no callback, normal return, native_result_accepted, and matching low32 wrapper/SDK results. Merely returning from a wrapper is insufficient.

All cross-layer checks keep frame_hook_mutex before descriptor mutex. Descriptor code must not acquire the frame lock. Waiting/capturing requires deadline polling outside render callbacks; Create/release/world changes invalidate it. Closing admission precedes hook drain and retained resource release. These are integration requirements, not evidence that runtime wiring is complete.

### Selected descriptor callbacks validated

Fresh `build-ngx-target-capture1` SHA256 `95fea68e3ee1b0ebbfbd6dc067e5300f14e9a2cc4668f57437edea8d25080025` passed457 dependency checks, startup suite, focused CPU tests and patched real-D3D12 descriptor fixture. Receipt `analysis/runtime/ngx-target-capture1-tests.json`; full log `ngx-target-capture1-selftests.log`.

First accepted Eval now imports and seeds an independent selected-MV bank once. Complete seeding freezes source updates; later source/legacy failures cannot reopen or relabel the target. Native callbacks preserve selected-MV aliases, erase overwritten descriptors and skip unrelated allocations. stop_target freezes mutation while retaining ownership; drained shutdown releases typed leases outside the observer lock. Fixture verified late heap import, selected native rewrite, unrelated replacement, alias lineage, stopped admission and cleanup.

This build has no consume-window/Dispatch wiring and was not launched in game. Its status explicitly identifies collection until explicit stop/close, not bounded complete-render evidence. Next implement authenticated consume/SDK return hooks and deadline polling before actual-game use. Player default unchanged.

## 2026-10-19: future consume and real SDK returns captured in game

Fresh target-window4 DLL SHA256 `06f28bede2ecfa74e28e28ee3dfe11105e65c9ec35ec7eda67227c4bd0dbb3d1` passed460 dependency verification, startup/native descriptor fixtures, SDK receipt boundaries and the actual scheduling-inl CPU fixture. Window1 failed a test shadow warning; window2 exposed a late macro gate that disabled real wiring despite fixture success. Corrected to the early EPOCH_PAIR_OBSERVER gate with explicit compile assertions; window3/4 compiled active wiring. Window4 also corrects descriptor status to reference the separate scheduling receipt.

Actual `dlss-target-window4-20261019.json`, session `continuous-stereo-30148-20261019-112100-f6049182`: first selected source ticket617, next complete consume618, changed renderer permitted, same Create revision1/world1/MV identity. Window phase3 frozen, reason0, two independently returned SDK calls, both raw1 and wrapper1. Sampling lasted63ms from selection to completion. Selected bank retained59 heaps/3 MV cells, zero loss/uncertainty/saturation, stopped admission and drained bank release. This is CPU scheduling/SDK success evidence; Dispatch binding, GPU producer domain and playable stereo DLSS remain unverified.

Driver error/interruption null, normal game close true, five settings/profile files restored true. Owner exited1, so this is not overall successful VR-session acceptance. Native evidence saved `analysis/runtime/dlss-target-window4-20261019.native-status.txt`. Player manifest remains RuntimeMirrorFix1. Next collect attested command-list binding/Dispatch evidence inside this now-verified bounded window, without increasing global descriptor storage.

## 2026-10-19: compute-binding prerequisites verified

Added bounded pure CPU model `ngx_mv_compute_binding.h` (16 canonical command lists, independently attested recording generations). Different heap/root signature, Reset, ClearState, ExecuteBundle and generation changes invalidate root3 table evidence; redundant same heap/root preserves arguments. Invalid/missing generation attestation and capacity loss are terminal. Dispatch returns only scalar binding candidates and explicitly does not infer descriptor-table membership from GPU handle subtraction.

Compiled and ran NgxMvComputeBindingTest and D3D12AbiTest successfully. The Windows SDK C-vtable offsets now additionally certify Dispatch14, SetDescriptorHeaps28, SetComputeRootSignature29 and SetComputeRootDescriptorTable31 (38 total assertions). No DLL rebuild or game launch was needed for these standalone tests; player DLL unchanged.

Integration review found window4 has DEATH_PSO_CACHE OFF, so its existing ClearState/ExecuteBundle wrappers are absent. Runtime compute observation needs six added slots in that configuration (four binding/Dispatch plus ClearState11/ExecuteBundle27), or four when reusing already-installed PSO wrappers. Reset invalidation must include bypass paths and failed Reset. Retain typed callback list/heap identities, never reconstruct COM from NGX integers; use fresh frame_link generations and frame-before-descriptor lock order. Scope ends with the verified target window. These hooks are not yet installed by this commit.

## 2026-10-19: compute callbacks integrated; generation rejection remains

Added bounded typed compute interception (six hooks with PSO cache OFF; four with existing clear/bundle wrappers), Reset/ClearState/ExecuteBundle invalidation, current selected-MV lookup before/after Dispatch, and window-stop/drained-lease integration. Waiting records binding state; only selected-window MV-related dispatch candidates consume the32 row limit. Offsets are candidates, not descriptor-table membership or shader-write proof.

Historical compute chunks rechecked against the complete artifact1 blob (`compute-caller-historical-contract-check.json`); gated host contracts include four complete hot/cold chunks. First actual compute1 run was rejected because existing verify buffer was sized to the smaller Eval wrapper:2554-byte17b420 produced no hash, not a verified byte mismatch. Bounded4096-byte buffer fixes the verifier; no contract truncation or relaxed comparison.

Fresh compute2 SHA256 `26bb94918d11d2c7bbc540c6422bc19652bcc78acad51451477d2b4e6b6e26ec` passed463 dependency verification and startup/descriptor/window/SDK/compute-core/ABI tests. Actual session `continuous-stereo-43600-20261019-115555-64ce2a9b` confirmed all21 host chunks match, selected ticket630 -> consume631 with two successful SDK returns. Compute stopped early: failure10 invalid_generation, core_loss1,16held lists,14invalidations,6waiting+40capture bindings, zero Dispatch rows. This is a failed compute observation, not proof of absent MV generation. Next collect exact first failed list/generation/site; do not raise capacity or accept stale state speculatively.

Driver error/interruption null, normal close and all five settings/profile restores passed; owner exit1 means not playable VR-session acceptance. Evidence `analysis/runtime/dlss-compute-capture2-20261019.native-status.txt`. Player default remains unchanged.

First-failure instrumentation candidate compute3 now freshly builds and passes the same suite (463 dependencies), SHA256 `4baaa22c3de92f52f6eca1062c305cf4b7db9bdf747c0f7a38fc380849b34f79`. It records the first rejection site, typed list identity, stored/current generations, reset result, ownership/bypass, window/ticket/world and pre-failure core loss; it does not change rejection rules. Actual compute3 sampling remains next. Actual compute2 native_cleanup_verified and entry_restore_verified were both true. No game process remains.

## 2026-10-19: destructive invalidation incorrectly required old generation

Actual compute3 session `continuous-stereo-14892-20261019-120819-e65b7496` pinned the first failure: reset_begin -> invalidate_reset_or_unobserved, held generation164869, fresh tracker identity/generation0, calling thread ticket/frozen ticket/renderer0 while global target capturing. Window601->602 still completed two SDK returns. This is evidence of overly strict observer revocation, not a determined game-rendering fault or an explanation of why the tracker invalidated that list.

Changed revocation to erase-only `forget_binding(canonical)`, allocating no row and clearing no sticky loss. Reset begin zeros held generation. Only the actual ListRef returned by this invocation's successful frame_link.reset_success may restore it, and only while a fresh current reference still matches. Bypass/unowned successful Reset without that receipt leaves unknown. Native failure/unwind and strict unknown/stale binding/Dispatch rejection stay intact; no cap increase.

Fresh compute-reset1 SHA256 `a352096f93d713837e74f3d3de778a75fad71c844c9566dc7b87473cdcc02f0a` passed463 dependency checks and full diagnostic suite. Added CPU tests cover nonallocating erase, unrelated list preservation, sticky prior loss, and later missing/mismatched generation rejection. Actual retest pending below.

Actual reset1 session `continuous-stereo-38636-20261019-121441-0f5a9e7b`: previous reset_begin rejection no longer occurred. Window602->603 completed two SDK returns. Compute advanced to42 capture binding calls, then stopped with genuine list_capacity at track_capacity/root on a17th list (16retained), selected ticket603. No Dispatch rows yet. This does not justify claiming MV producer discovery. Next narrow admission or measure relevant list population; do not conflate this capacity loss with the repaired destructive-invalidation bug. Error/interruption null, normal close and settings/profile restore true, owner exit1. Native trace is `analysis/runtime/dlss-compute-reset1-20261019.native-status.txt`.

## 2026-10-19: bounded compute-row reuse, actual negative result

Fresh ComputeReuse1 DLL SHA256 `3747425b19b8f65c1fcc457d9949eef3ce86b93c3f9a1fdb09d7dd692a31d8f1` verified 463 dependencies and passed the startup suite, descriptor fixture, compute binding and ABI tests. The 16-list limit remains unchanged. Reclamation requires an explicitly revoked empty binding, matching current recording knowledge, no active callback/Reset and no saved-row reference. Typed incoming device and recording identity are checked before capacity. Detached owners release outside the frame lock. Tests cover partial bindings, pinned/busy records, stale generations and unknown-to-known generation rejection.

Actual `dlss-compute-reuse1-20261019`, session `continuous-stereo-2120-20261019-122815-003ec9e9`, selected ticket597 and observed consume598. Both native SDK calls returned accepted result1. The compute observer reused four empty rows in five attempts, then stopped at capacity: all16 entries had partial/nonempty knowledge, none eligible, none busy or history-pinned, one stale. Failure site `track_capacity`, operation root, had a verified same-device current incoming recording. Zero selected-MV Dispatch rows were captured. Target descriptor collection itself retained61 heaps/3 selected-MV cells with zero loss.

This rules out safe empty-row reclamation as a sufficient solution for this case; it does not justify discarding partial bindings or merely enlarging the bank. Next investigation should bound admission around the already byte-attested native compute helper before its heap/root/table setup. The late root-table return alone cannot establish prior bindings. No producer/GPU or playable-DLSS claim follows from these SDK returns. Owner exit1 remains incomplete VR acceptance. Normal game close and restoration of all five protected settings/profile files passed. Default launcher and release DLL were not changed.

## 2026-10-19: independent typed caller census identifies missed compute work

ComputeCallers1 SHA256 `e41e322f7a142edc0c9b9dd3728226e3940d90d688e9f446d56a59afebc4be83` passed fresh verified build and existing suites. A32-group scalar census in the existing typed detours remains bounded by the owned target consume window independently of compute-bank failure. It retains no COM/list state; full buckets count omitted observations. Caller RVAs require a guarded loaded-image bounds check. No new native entry ABI was assumed.

Actual `dlss-compute-callers1-20261019`, session `continuous-stereo-46408-20261019-123616-5eece70e`, selected604/consume605, captured248 calls in16 groups with zero overflow. During selected consume, root return138124 and heap return13813a each occurred87 times. Root return22018d and heap return2200ed occurred once each, after bank failure. Table return17b99a and Dispatch return1ac833 each occurred14 times, all after bank failure. Thus zero recorded Dispatch rows in the earlier bank was an observation loss, not absence of native Dispatch. These scalar groups do not join calls by command-list identity and do not prove that any Dispatch writes selected MV.

KB-first bounded offline decoding of artifact1 function220134 verified both complete chunks against source section SHA911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 and combined function hash a0d3715ba3368d42f377146c18713e6f4534df023a23d0eec7dcc84d425f4288. Assembly has compute-root virtual call220187 returning22018d, followed by descriptor-heaps tail jump2201b5; therefore heap return belongs to its caller, consistent with observed2200ed. This is historical/current-location correspondence, not a full current-byte attestation of220134. Saved mechanical evidence: `analysis/runtime/dlss-compute-setup-saved-1-220134-20261019.json`. The cached pseudo signature is corrupted and must not define a new native hook ABI.

Next: inspect and authenticate setup/initialization callers138124/13813a and2200ed, then establish bounded per-recording setup-to-Dispatch correlation without retaining unrelated initialized lists. Conditional/cached state means a scope opened only at1ac6e4 is insufficient. Original calls continue unchanged. Actual run closed normally and restored all five protected files; owner exit1 is not playable-DLSS acceptance. Default/release remains unchanged.

## 2026-10-19: setup-scoped admission exposes an earlier-phase dependency

Complete historical138020 (438 bytes) and2200a8 (140 bytes) were mechanically decoded and hash checked after KB lookup, saved in `dlss-compute-init-saved-2-2200a8-20261019.json`.138020 directly sets compute root at13811e (return138124), then descriptor heaps at138134 (return13813a).2200a8 calls220134 at2200e8; its tail heap call returns2200ed. Four complete setup chunks were added to the existing current-process host attestation, including220134 cold code. No inferred native entry signature is used.

The private narrowed observer admits new lists only at typed root callback22018d during capture. Existing exact typed-list arguments still receive strict generation, binding and Reset handling. Unknown callbacks skip COM admission and are counted; interface-alias coverage is explicitly incomplete. A duplicate canonical setup alias fails rather than creating two records. This is a bounded experiment, not a claim that unobserved initialization state is irrelevant.

Initial scoped1 build failed because the coordinator inserted contract rows at the wrong preprocessor block; corrected before any game launch. Fresh scoped2 SHA256 `41480d5029657e0944512cfc096afc0f0319a8bc05b4611f8a35c287aa048eb6` passed build/selftests. Actual `dlss-compute-scoped2-20261019`, session `continuous-stereo-1908-20261019-125318-85e02884`, verified all25 current chunks. Zero bank loss, but zero admitted lists: setup root22018d/heap2200ed each occurred once in waiting ticket595, while14 Dispatch1ac833 calls occurred in capture596. Caller census250 calls/16groups/zero overflow. Thus capture-only admission is too late for this run; setup placement varies across the selection boundary. No missing state was fabricated. Next change must permit the observed waiting-stage setup and preserve its genuine recording generation through capture, rejecting any intervening Reset mismatch. CPU association still cannot prove GPU MV contents.

Game closed normally and all protected files restored; owner exit1 remains incomplete VR acceptance. Default/release package unchanged.

## 2026-10-19: early setup admission does not yet identify the Dispatch list

Admission at the attested typed22018d callback now accepts both waiting and capture phases; all ownership/world/recording/Reset checks remain. Fresh EarlySetup1 SHA256 `d943eb1a73ee43f1002eb8c1c0c52f796d157455a30774e9c6ed496ddc33d168` passed verified build and suites.

Actual `dlss-compute-earlysetup1-20261019`, session `continuous-stereo-45080-20261019-125818-4fc9fdc2`, retained3 lists with15 captured binding events, no bank failure or capacity loss. However all14 Dispatch calls were unknown to the exact retained typed-argument lookup; zero selected-MV Dispatch rows. This run's admitted bindings occurred during capture, so it does not itself demonstrate successful waiting-to-capture retention. The earlier run establishes why waiting admission is necessary, not that it is sufficient.

Next distinction is different native lists versus interface aliases of retained canonical identities. Do not claim all14 are different COM objects from pointer inequality. A bounded canonical comparison at typed Dispatch can resolve this without admitting the broad initialization stream; generation and alias Reset handling must stay explicit. Normal close and five-file restoration passed; owner exit1 remains incomplete VR acceptance. Default/release unchanged.

## 2026-10-19: canonical comparison rejects the alias explanation

Typed callbacks now resolve raw matches first, then temporary IUnknown identities against retained canonical rows. No alias bank or broader canonical admission is added. Matching alias mutations revoke/update the same row, including out-of-consume Reset/ClearState/bundle paths. Temporary identity references are held through callbacks and released after local frame locks; resolver reentry/QI failure rejects coverage. Eight bounded Dispatch identity groups expose unmatched canonical identities. Shared PSO wrappers use the same revocation scope; their include moved after the compute observer to expose its type, with a disabled no-op implementation.

Alias1 SHA256 `32f4e86ac73f670245ddf76a7a019e20d55888a786f8302729c235e4a47a398f` passed verified build and suites. An explicit PSO-enabled build passed after disabling mutually exclusive VR_DOF_COMPAT; the initial incompatible configuration was rejected before compilation. Actual `dlss-compute-alias1-20261019`, session `continuous-stereo-43700-20261019-130812-ec17afd8`, retained one setup list, no observer failure. All14 Dispatches were canonically unmatched:12 on16180da610/current generation159059,2 on3d3eb9800/current generation159166. Both have no retained-row match. Resolution counters:4 raw matches,0 aliases,988 unmatched,0 QI failures,0 reentry rejections; identity groups2/8, no overflow. This rules out interface aliases as the cause in this sample and rejects the22018d setup route as sufficient for the observed Dispatch lists. Next experiment should begin with the selected MV descriptor at the authenticated root3 table callback, keeping unknown prior root/heap state explicit.

Review found Reset's new pre-Scope QI/final Release lifetime was not fully covered by active-based teardown. A separate activity guard now encloses compute ResetScope on bypass/unowned/normal and exception paths without changing world admission. Fresh Alias2 SHA256 `1fc8e6977d66ee488d115121c8876c20fbefdfcd47a20c21df65adf9ad0fefd0` passed build and suites after this correction; Alias2 has not yet run in-game. The PSO-enabled compile predates only this activity guard. No targeted blocked-QI/Release concurrency test is claimed.

Alias1 game closed normally, all five protected files restored; owner exit1 is not playable-DLSS acceptance. Default/release unchanged. The actual identity evidence and final source build are deliberately recorded separately.

## 2026-10-19: resource-first exact MV association reaches a Dispatch candidate

The independent probe resolves the authenticated root3 table argument17b99a directly through the retained target descriptor directory. It requires an exact base selected-MV match, unchanged descriptor version/origin across the original call, selected device and genuine recording identity. One actual typed list and canonical IUnknown remain retained until drained release. Any observed heap/root/Reset/Clear/bundle or root3 overwrite after arming stops the probe conservatively; it adds no GPU commands. Authenticated Dispatch1ac833 requires the same canonical/generation/window and descriptor before/after. Up to8 scalar rows explicitly withhold bound-heap/root-layout/shader-write/GPU proof.

The real descriptor fixture now tests exact selected MV, wrong/zero/misaligned handles and erased descriptors. Initial resource1 compile exposed a declaration-order issue (descriptor implementation is included after compute); shared scalar definitions moved to snapshot.h with a forward declaration beside the existing lookup. Fresh Resource2 SHA256 `25b8098c40ada125f671eb4d8f3ac26b86fda5b3193bc8c2fcac6297cc9356d7` passed build and suites.

Actual `dlss-compute-resource2-20261019`, session `continuous-stereo-45528-20261019-131947-a0e04572`, selected625/consume626, captured one candidate Dispatch: canonical3d3dfd9b0, generation169407, heap1890a900, MV16186653b0, GPU handle5678a01009e40, version2/origin1, groups80x90x1. Original Dispatch returned and descriptor remained matched/version2. One table attempt, zero misses, one row, no overflow; a later root3 overwrite stopped observation. No shader threadgroup size or pixel coverage can be inferred from dispatch group count alone. Normal close and all five protected files restored; owner exit1 is not playable-DLSS acceptance.

Review identified a nested known-raw fast path that could avoid exact-probe reentrancy rejection. The check now occurs before both raw and alias resolution. Fresh Resource3 SHA256 `ba90d411256e63491d99a0f1b3f16cf5da8cabf12ec0844f43e54eb53254fef2` passed build/suites; no game run yet. The Resource2 row is a promising historical candidate that must be repeated with Resource3 before stronger association claims; zero reported reentrant rejections alone cannot rule out the prior fast-path gap. Dedicated blocked/nested callback fault injection is not claimed. Next: repeat this candidate with corrected guard, then identify its shader/constants and verify actual MV pixel coverage. Default/release unchanged.

## 2026-10-19: corrected resource-first guard reproduces the association

Resource3 (`ba90d411256e63491d99a0f1b3f16cf5da8cabf12ec0844f43e54eb53254fef2`) actual `dlss-compute-resource3-20261019`, session `continuous-stereo-46744-20261019-132452-ee948f75`, repeats the exact association after the nested raw/alias guard correction. One exact-MV table and one normally returned stable Dispatch, groups80x90x1, ticket626/world1, canonical161752afb0/generation169581, descriptor version2/origin1 before and after. Later root3 overwrite ends the one-association probe as designed; no overflow. Normal close and five-file restore passed. Owner exit1 remains incomplete VR acceptance.

This confirms the narrow CPU association under the corrected diagnostic; bound heap, root-layout, shader write domain and GPU execution remain unproved. Next bounded implementation is a rolling typed PSO witness at attested SetPipelineState return17b514, promoted only for the exact table's same canonical recording/window. Missing cached-state history must remain unknown and must not suppress the existing resource association. Only after a stable PSO association should a single deferred GetCachedBlob attempt inspect its artifact; a driver cache blob is not automatically shader bytecode. No constant dump or threadgroup-size inference is warranted yet.

## 2026-10-19: bounded typed PSO witness candidate

The private ComputePso1 diagnostic adds one pending and one promoted typed PSO witness at attested SetPipelineState return 17b514. Promotion requires the exact selected-MV table's canonical list, genuine generation, world, ticket and renderer. Later Set/Reset/Clear/Bundle invalidates current validity while preserving a completed stable Dispatch receipt. Missing PSO evidence does not prevent the existing exact-MV association. No cache blob is read and no shader execution or MV correctness is established by this change.

Fresh PSO-off build and the full diagnostic self-test suite passed (463 captured dependencies); DLL SHA256 `49196b4067766fe8856c219490958c4e0f8e8f0e0097886d92ad55894489ce60`. The independent PSO-cache-enabled configuration also compiled successfully with its shared wrapper, avoiding duplicate Set hooks. Tests cover canonical alias matching, generation/context/sequence mismatch and preservation of historical validity after revocation. Runtime sampling is separate from these CPU tests. The player default manifest remains RuntimeMirrorFix1, with DLSS locked off.

Actual ComputePso1 run `dlss-compute-pso1-20261019`, session `continuous-stereo-16172-20261019-133535-05372ab1`, observed one candidate, one promotion and one returned stable exact Dispatch with `pso_stable=1`; no PSO identity-query failures. Consume604/world1/generation163441, canonical list `3d1601540`, PSO `13ba08f90`, sequence775, attested Dispatch return `1401ac833`, groups80,90,1. The exact descriptor remained version2/origin1 across the call. Later mutations revoked current validity while retaining the historical stable witness as intended. This identifies a typed PSO associated with that selected-MV call, not its shader bytecode or GPU writes. No blob call was made.

The test used the game's native DLSS menu route. Normal game closure and all protected settings/profile restoration passed; owner exit1 remains a diagnostic/non-playability outcome, not DLSS acceptance. Next bounded analysis step is one post-freeze cached-PSO blob retrieval and format validation; opaque cache data must not be treated as shader bytecode. Player default unchanged.

## 2026-10-19: one worker-side selected PSO cache retrieval

Private ComputeBlob1 adds one post-freeze GetCachedBlob attempt on the retained historical-stable PSO. The actual worker thread is registered explicitly; RHI and self-test callers of the shared tick cannot trigger it. A native activity guard encloses typed ownership acquisition, cache call, saving and all COM Releases. Native calls and file IO occur outside the frame lock. CREATE_NEW and a 4MiB save cap bound the artifact; driver internal allocation itself is not bounded by that cap. The receipt includes HRESULT, byte count, path/error and the immutable witness key. This is opaque cache evidence until separately validated, not shader or GPU execution proof.

Fresh build and diagnostic self-tests passed, DLL SHA256 `b33e11f50cd770600dfa8b3a5328a1d2273979062c9b04b8268b433d0a39f397`, 463 captured dependencies. Actual-game collection is a separate pending step. Player default unchanged.

Actual ComputeBlob1 session `continuous-stereo-18900-20261019-135347-338316af` saved exactly one4662-byte cache (HRESULT0), world1/consume602/generation162870, stable exact PSO `14683a9c0`, sequence772. Cache SHA256 `522e5cb888f90606b1dad1eb52df6826a5aadf589ab7d683a5e61a989c984db8`. Normal game closure and protected settings/profile restoration passed; owner exit1 is not playable DLSS acceptance.

The existing bounded extractor found a2360-byte compute6.0 DXIL container at cache offset954, SHA256 `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`. System D3DDisassemble returned80004005; the already installed Windows SDK `D:/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe -dumpbin` succeeded. Saved dump: ignored `analysis/runtime/ngx-picked-compute-blob1-dxc.txt`. Metadata declares8x8x1 threads; observed80x90x1 Dispatch thus launches640x720 thread positions, which is not alone proof of target coverage. Shader declares CB0(160bytes), T0/T1 and U0. CB0 register5.zw offsets the integer thread coordinates for both loads and store; register5.xy scales normalized coordinates, registers6–9 participate in fallback transformation and register4.xy is subtracted before output. These are decoded operations, not authenticated semantic names for the constants. Root-layout provenance, actual values per eye, submission execution and resulting pixels remain unverified. Next analysis can target these specific bindings/constant ranges instead of dumping arbitrary buffers.

## 2026-10-19: selected MV root layout joined in the actual game

Private Root1 DLL `47f32e3c6247e676bea0235d97ba6837e033c361a267f43401783034fe7aef0f` passed a fresh build (467 captured dependencies), the continuous startup suite, descriptor/root fixtures and CPU identity/revocation checks. A first launch was rejected before starting a game because the current player configuration contained `render_scale` while this diagnostic uses game resolution. The repeat used a separate configuration with that field omitted; the player configuration was not edited.

Actual Root1b session `continuous-stereo-9404-20261019-191847-a7c2d089` associated the selected MV, PSO and retained serialized root with one returned stable Dispatch: world1/ticket483, canonical `161a09f030`, generation127294, renderer `86aed440`, root `71c5c0b0`, device `18c73790`, provenance row3. Root history remained stable through Dispatch; later revocation did not rewrite the saved receipt. The independent root bank captured four roots/712 bytes with no loss, conflict or read failure, and released all owners during shutdown.

The current 228-byte compute root blob SHA256 is `5c0dd75ff306c87e61ed38dbf1733480a40419c7c5f22ba06a00cf75a5ff5717`. The existing SDK deserializer reports root0=SRV t0..31, root1=CBV b0..7, root2=sampler s0..15, root3=UAV u0..7, all space0/all visibility. The separately cached PSO again contains the 2360-byte DXIL hash `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`. Thus this specific actual call's root layout can be joined to the already decoded shader. These are binding observations, not proof of GPU execution or pixel coverage.

Normal game close, native cleanup, independent entry restoration and all five protected settings/profile restores passed. Owner exit1/session incomplete remains a non-playability outcome; no DLSS release claim. Curated receipt: ignored `analysis/runtime/dlss-mv-root1-evidence.json`; complete native log: `dlss-compute-root1b-20261019.native-status.txt`.

## 2026-10-19: bounded CB0 upload witness candidate

After KB lookup, the coordinator exported only relevant complete assembly chunks from a new private IDA copy and closed both owned leases. All exported bytes match historical runtime artifact1 (`911ab9...`), base `0x140000000`; the source private database remained hash `a840be95505d1ae9f2e776fa695925b11f23f92aab387951832a006782f0ae0c`. Evidence is under ignored `analysis/ida/dlss-mv-constants-20261019/`.

The already authenticated compute dispatcher uses `context+0x13cc` for its seven constant banks. The new candidate observes only bank0's 160-byte prefix, matching its CPU source with the native mapped upload range, actual typed CBV creation at return1ad2d9, native descriptor copy at return17bea9 and root1 table binding at return17bed7. It retains scalar addresses/bytes, never constructs COM from integers, maps no resource and changes no constants or GPU commands. Input memory, ring ownership, offsets, dimensions, current recording and descriptor overwrites are checked conservatively. Complete current-byte contracts for the upload/CBV/table helpers are required. Shader/root interpretation and GPU execution remain separate checks.

Fresh Constants3 DLL `340075ea41ad8867781f14c90321a05813593ed73697a7546763e99e4aa25d4d` passed build (469 captured dependencies), startup/descriptor/root fixtures and overflow/range/identity checks. An earlier build exposed the concurrent frame-mutex type change in two private test harnesses; their recursive-mutex aliases were updated without changing test behavior. The user subsequently authorized normal closure of the manual VDXR session and continuation of unattended testing. No player manifest was advanced for this DLSS work.

The first Constants3 run stopped at the menu notification guard before enabling DLSS. A private menu-route copy now accepts the already known exact world notification state and opens pause only from the verified HUD/toast stack; six guard cases passed. Constants3b session `continuous-stereo-1552-20261019-194618-bbd69ef6` completed the native menu transition and two native SDK evaluations. It captured four matching CB0 uploads and four native descriptor copies, but zero final CB0 table bindings. The selected MV Dispatch/root/PSO remained stable; this did not supply a constant-domain or playable-DLSS result. Native cleanup, independent entry restoration, normal close and all five protected-file restores passed. Raw status is ignored `analysis/runtime/dlss-compute-constants3b-20261019.native-status.txt`.

Constants4 adds first-failure stages without relaxing identity/range checks. Its initial launch was rejected by the repository launcher's singleton before game loading; protected files were restored. The private repeat preflight checks both a game and the repository launcher, and waits for the other test's full parent process to finish restoration. This rejected startup is not a DLSS runtime test.

Source review found the candidate's view-membership bug: `renderer+0x68` points to contiguous 0xd30-byte native views, as checked by the existing constructor and ownership code. Reading its first two machine words as view pointers was incorrect. Constants5 reuses `native_eye_index` and retains the original typed-command/upload/copy checks. Fresh DLL SHA256 `6a522a97301b34ee129aaadf5eb0dd8e055035f307c976b943d53ab0e6bcdbde` passed build (470 dependencies), startup and descriptor/root/CPU fixtures. Runtime results remain required before accepting the association.

## 2026-10-19: both selected MV constant bindings observed

Constants4b confirmed four table attempts, all rejected only at view-membership stage5. Constants5c then recorded four valid table bindings with no stage failures and one stable selected-MV Dispatch/CB0 association. Two earlier Constants5 attempts stopped during changing menu value reads; private navigation now retries only bounded, known read acquisition changes and never repeats the posted option key. These attempts closed normally and restored their protected files.

The single-association observer used to stop on the next root3 overwrite. Constants6 keeps at most eight immutable association/owner histories in the same selected consume window; a mutation inside an unfinished setter/Dispatch still rejects. Typed owners release only after callback drain. Fresh DLL `a1a0fbf446ee48d20a43caf55c5a87ac486f943c030aa9149cd9e5ca2c30b851` passed startup and descriptor/root/CPU fixtures. Actual session `continuous-stereo-36448-20261019-201128-8253fe51` captured two stable selected-MV Dispatches on world1/ticket576, canonical `1617af7770`/generation151689. Both used root `71f3a1f0`, PSO `3dc3567a0`, and groups80x90x1; CB0 registers5 were respectively `(640,720,0,0)` and `(640,720,640,0)`. The cached shader again has DXIL SHA256 `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`, so its decoded operations request distinct 640x720 regions at x0 and x640. This is recorded shader-domain evidence, not GPU execution or pixel correctness.

Both calls had TLS current-view index1. That index therefore cannot identify the logical producing eye; logical domains must come from the shader constants and authenticated construction geometry. Constants7 labels it `context_view_index`, explicitly withholds logical-eye verification, and reads native target rectangles as integers rather than floats. The private input-rectangle experiment now keeps current Create-derived input geometry after the read-only epoch sample finishes; each constructed ticket still validates fresh Create identity. It remains default OFF and requires actual lower-input-domain testing.

Constants4b, 5c and 6 all completed native cleanup, independent entry restoration, normal game closure and all five protected-file restores. Owner exit1/incomplete publication remains outside playable-DLSS acceptance. Curated receipt: ignored `analysis/runtime/dlss-mv-constants4-to6-evidence.json`.

## 2026-10-19: actual per-eye GetOptimal and GPU completion

Constants7 exposed an invalid assumption: the native family Quality input is853x480 for1280x720 output, so dividing an even family input cannot define the eyes. Constants8b queries the authenticated native GetOptimal helper for the actual640x720 eye output before the complete Create wrapper consumes the shared parameters. It returns427x480 optimal,320x360 minimum and640x720 maximum, result1/quality2. The complete original Create setters follow the query. The constructor now uses this receipt and checks current Create identity on every ticket. It never divides the family's min/max or rounds853 to make a gate pass.

Actual Constants8b session `continuous-stereo-39140-20261019-203316-80b3d4de` rendered source rectangles `(0,0,427,480)` and `(640,0,427,480)`. The selected shader's CB0 register5.xy matched427x480 and5.zw selected x0/x640; both Dispatches were54x60x1. The original SDK Eval still consumed640x720 and MVScale640/720. Independent-feature shadows now patch both logical input dimensions and floating-point MVScale together. These shadows were still inactive in this diagnostic.

The actual DSV resource differs from the NGX Depth input. Constants11 joined writable per-eye DSV draw regions to a returned full `CopyResource` into the typed NGX Depth allocation, caller `base+0x1c5fe0`. Constants12 retires current raster bindings after their real Execute returns and aggregates repeated depth copies, preserving immutable evidence rather than exhausting current-binding slots. Constants14 also excludes the three native Output-to-Color copies before the first selected Eval from the two post-Eval copy receipts; ambiguous later copies remain rejected.

Actual Constants14 session `continuous-stereo-39332-20261019-213900-8d3a1710` (DLL `d0dbfb46d8ec612e32446d4a3d90023e5ab15a289ebf852e8b9a3cc32316cff6`) recorded both Evals and both following copies on one direct queue. Selected Color/Depth/MV records were joined to real submitted list generations, with no raster/depth capacity loss. One observer-owned queue Signal returned S_OK, and its real fence reached1 without device removal. This proves completion of the selected submitted work; it does not prove intermediate resource dependencies, shader depth semantics, pixel correctness or playable DLSS. Native cleanup, independent entry restoration, normal game closure and all five protected-file restores passed; owner exit1 remains outside playability acceptance. Receipt: ignored `analysis/runtime/dlss-mv-input-and-gpu-constants8-to14-evidence.json`.

NVIDIA's [DLSS programming guide](https://raw.githubusercontent.com/NVIDIA/DLSS/main/doc/DLSS_Programming_Guide_Release.pdf), section5.2.8.1, specifies creation at the recommended input size and rendering within the returned inclusive min/max interval. The adapter now accepts per-ticket actual input sizes only inside the same observed eye query's interval, with equal Color/Depth/MV domains, unchanged resource identities and output geometry. A size transition resets both histories; a right-eye-only size change, out-of-range input or absent query is rejected. CPU tests exercise full-size startup, reduced input, stable history and a further allowed reduction. Actual per-eye SDK integration and resulting pixels remain separate acceptance steps.

## 2026-10-19: isolated two-feature integration, not yet accepted

`GEARS_NGX_ISOLATED_STEREO_TRIAL` is default OFF, requires the behind-windows/muted/observe-only child environment, and refuses builds with XR publication or owned-scene/final-transfer replacement. It connects the existing complete-wrapper adapter, two independent SDK features, typed quartet, boxed copies and native submission retirement to a current-consume producer collector. The test DLL retains quarantined owners through its disposable process lifetime; no such artifact is a player release.

Constants18b,20 and21 each made two actual original SDK Create calls, returning1 with different nonzero handles. This is SDK creation evidence, not Evaluate or image acceptance. Constants18b rejected the initial Evaluate before dispatch because its quartet observation began too late. The collector now retains typed full-buffer resources from barriers earlier in the same consume, and the MV descriptor bank can seed from the actual retained quartet without waiting for the diagnostic's deliberately skipped initial pair. Constants20/21 reached that input callback but rejected on bounded binding-table saturation. Constants22 adjusts exact-generation retirement to work when queue submission runs outside consume TLS. Its actual-game result is pending.

The isolated adapter can run one explicitly unpublished transition pair when fresh MV production is unavailable, using actual typed inputs and native requested bases; it does not invent a rendered MV rectangle. This pair cannot yield a normal output receipt. Its real SDK/copy work must complete, both histories reset again, and a new fully observed pair must pass before transport acceptance. CPU tests check the hold, reset, denial of output receipts and subsequent full-input pair. This opt-in does not establish first-pair image correctness.

Each eye writes a subregion of the larger native Output allocation. Owned Create shadows now set `InEnableOutputSubrects` while preserving quality and feature flags; [NVIDIA's multi-view guidance](https://raw.githubusercontent.com/NVIDIA/DLSS/main/doc/DLSS_Programming_Guide_Release.pdf), section3.17, requires this capability for output subregions. Input bases remain x0/xstride, output base remains zero, and each returned native copy is boxed into only that eye's Color region.

Constants15c produced a driver access violation during process teardown. A bounded offline dump unwind places this secondary failure in graphics-resource release after game exit; it does not identify the upstream cause. The exact crashed test process and its WER helper were closed after birth/image checks, and all five protected files restored. Later failed trials also restored their protected files; no failing trial is playable acceptance. Stage-level SDK receipts are summarized in ignored `analysis/runtime/dlss-owned-pair-create-evidence.json`.

## 2026-10-19: sustained isolated per-eye SDK transport

Constants22 first reached both real SDK Evaluates with separate handles and result1. Constants23 then identified an incorrect copy selection: the native graph reuses the same Output/Color allocations for preprocessing before the next ticket's first Eval. The controller now boxes only a pending post-Eval copy belonging to the current ticket/renderer; other native copies remain unselected. Constants25 showed real compute calls and constant uploads but no valid CB0 tables because the diagnostic list resolver deliberately sleeps outside its selected window. Constants26 resolves the actual typed list independently for live CB0 table/Dispatch joins. The early source descriptor ledger intentionally omits visible copy fanout, so the initial pair still cannot inherit retrospective GPU-table proof; later native copies are tracked live in the selected typed MV bank.

Actual Constants27 session `continuous-stereo-37960-20261019-231630-8613217d` (DLL `9dfa254af501274a88b266ed8ada6406cf70d68b12a94f471511acf420c4a9f6`) completed native menu enable, a15-second hold and menu disable. Stage receipts show distinct left/right SDK handles, real accepted Evaluates,427x480 fully observed inputs, two eligible MV regions per mature ticket and one current cached shader verified against the known DXIL SHA256. The final collector reports1226 accepted input callbacks, no context/bank failure, adapter phase accepted, no GPU transport failure, no retained recordings and no inflight batches. These are per-eye SDK/boxed-copy transport observations, not image-quality or normal XR publication acceptance.

Normal game closure, native cleanup, independent entry restoration, no owned foreground samples and all five protected-file restores passed. Disabling AA completed in the native menu; the engine kept its cached feature handle, so this is not proof of two SDK Releases. Pixel readback and final presentation remain required. Constants28 adds a private Output CopySource readback only after accepted transport and at least32 full-input pairs; this capture has not yet been accepted.

### 2026-10-19: first owned-feature pixel readback (Constants28d)

Private DLL SHA256 `40ababb43d2d4f3d3fd4a26f4d833d5ea157f1590e310ab8bfdb04aab564d731`, session `continuous-stereo-28724-20261019-233029-1462231e`, read back two scratch outputs for world1/ticket571 after actual SDK evaluations and inherited native COPY_SOURCE copies. Both readbacks completed and their command recordings physically retired. The owned 640x720 region is at scratch origin0 for each feature. Decoding the R11G11B10_FLOAT data showed finite, nonzero scene content for the first eye and an entirely zero second output; the unused physical right half was zero in both captures. `analysis/runtime/dlss-output28d-pixels/inspection.json` retains raw hashes, exact footprint and deterministic Reinhard/gamma inspection views. Original-wrapper metadata reports its unchanged 640x480 input clamp; the separately controlled shadow uses the authenticated producer's427x480, so those values must not be conflated. TLS eye remains unverified by this observer; source bases0 and640 join the controlled per-eye order. This is a concrete pixel failure despite SDK success, not playable DLSS acceptance. Native disable menu, normal game close, hook restoration and all five protected-file restorations completed; no owned foreground samples. Constants29 adds two bounded Color destination readbacks after the actual boxed copies, with explicit COPY_DEST-to-COPY_SOURCE-and-back transitions, to locate the lost right-eye content.

Constants29 SHA256 `bda5ceee53fef050ddd11dfe45bc31d56230314252e9d819b8eff0ab4cdefe93`, session `continuous-stereo-14420-20261019-233711-2edb3ed0`, captured world1/ticket565. First owned Output and Color-after-left-boxed-copy are byte-identical over the full1280x720 buffer, with all-zero right half. After the second SDK call Output is entirely zero; Color-after-right-boxed-copy retains the same first-eye image. This proves right Color is already zero at the first post-Eval copy; it does not alone identify which earlier pass erased it. The supplementary Color readbacks used two explicit state transitions per copy and restored COPY_DEST. Original Output metadata's `barriers_added=0` describes the original Output probe only; the separate Color diagnostic added2. The game then exited unexpectedly (Application Error at23:38:30, process14420, nvwgf2umx+1268f8/0xc0000005), so this run has no normal-close/hook-restoration acceptance. This offset also appeared in an earlier secondary teardown fault; no new causal attribution is made without a dump. Game gone, all five protected files restored, no owned foreground samples. Extra Color readback is now a distinct default-off private option. Constants30 tests a native-preprocessing region-preservation hypothesis using the current typed source, actual last RTV draw clip and constructed per-eye rectangles, with SDK writes invalidating the previous native-draw region; Output pixel readback remains enabled.

Constants30 SHA256 `00a345c203bd40db73bea96fa06572d0a7d6bbae16aa6e3deb092dac8db845fb` completed15s, disable menu, normal close and all five file restores. Native region experiment boxed1,150 preprocessing copies and left575 unclassified full copies unchanged; owned SDK transport remained accepted with no input/queue failure. Same-ticket563 output still has a nonblack first eye and all-zero second eye. This hypothesis failed pixel acceptance and is not promoted. Constants31 SHA256 `81975d70b77d4d004744071970fb5aa134fa2df12a26743657af2ea565e19ac2`, session `continuous-stereo-37956-20261019-234946-3d9ab246`, instead left native preprocessing unchanged and read its first two Output COPY_SOURCE images for ticket575. Both show scene only at physical origin, approximately427x480; the full right half is zero. This moves the observed failure before the SDK. It does not prove earlier scene Color lacked the right eye, because the probe sampled the copy source, not destination-before-copy. Constants31 completed15s, disable menu, normal close and all five restores. Constants32b adds that exact destination-before-copy readback. The output probe now quarantines leases before a potentially recording pre-copy callback/native original can throw; an offline regression checks no premature release, including an exception before the original copy. All checked tests passed in fresh32b; game launch waited for another owner's existing game and changed no protected files while waiting.

### 2026-10-20: submission serialization before continuing pixel diagnosis

Constants32b (SHA256 `8d52be074009ba94166f84de87f08a040fe47c3aa0941941b40bcca630e2becc`) exited before the64-input readback threshold, so no Color readback executed. The new bounded failure trace records context invalidation at the NativeGpuSubmission::execute failure path on a submission thread, outside consume TLS; this does not identify the internal validation reason. All five protected files restored after game exit. Source review found complete Create/Eval wrapper adapter Entries were protected by the parameter mutex, while Execute/Reset/Copy used only the frame mutex: legitimate RHI recording and queue submission could enter the adapter concurrently. Fresh Constants33b adds a separate recursive API-order mutex acquired before frame bookkeeping at those outer hooks; ParameterScope includes it, while original SDK forwarding still leaves frame_hook_mutex free. The old read-only observer forwarding guarantee remains checked. An actual host-scope CPU regression verifies nested same-thread entry, exclusion of another thread from the API-order lock, free frame mutex during forwarding, and release afterward. NativeGpuSubmission now retains its first failure source line for diagnosis instead of only a boolean. Fresh33b SHA256 `ca3a8432788a7c52b26052636fc976fc24788d90c39ce949af9e2dcd3493df61` passed existing build/selftests and this new check. It retains the before-copy/source readback experiment; no playable SR or GPU/pixel success is claimed from the lock fix. Tests33 (frame mutex held through SDK) were rejected by the existing forwarding regression before any game launch, and that approach was replaced rather than weakening the regression.

Constants33b actual session `continuous-stereo-23028-20261020-000951-175ef904` completed15s, native AA disable, normal close, hook restoration and all five protected-file restores, with no owned foreground sample or transport failure. World1/ticket562 first preprocessing source contains only an origin427x480 scene, while destination Color BEFORE that first copy already has a full640x720 first-eye image and all-zero right half. Before the second native copy, Color contains only the reduced first-eye region; right remains zero. Thus these two full copies are not established as the earliest right-eye loss. Bounded current stacks distinguish original copy paths through game return11aca0 (callers119deb/11a5e3),2216d9 (caller11a302), and owned post-SDK pathbe69af (caller11a6e0). Saved historical pseudocode has ABI/decompilation defects and was not treated as an authenticated live semantic contract. `analysis/runtime/dlss-preprocess33b-evidence.json` retains hashes, exact typed readback metadata and this acceptance boundary. Constants34 moves the observation earlier: two readbacks at Color's actual RENDER_TARGET exit transition, using that native before-state contract and restoring it before forwarding the unchanged original barrier. Source Color and paired Output interfaces come from the typed barrier/quartet leases. This diagnostic adds two source transitions per selected recording, remains background-only/unpublished, and allows separate tickets without labelling them a stereo pair.

Constants34 completed15s, native disable and normal close/restores but selected no Color RTV exit: the actual 4-to192 transition runs on a submission thread outside consume TLS. Constants35 used the actual typed epoch lease outside that scope and a separately labelled physical-recording nonce, never fabricating a game FrameLink generation/ticket. It observed both appended readbacks execute/signal/complete and their physical Reset, but falsely rejected a later use of the reset COM list as a duplicate; no bytes were saved. That observer bug was fixed in36 by ignoring subsequent executions only AFTER actual Reset retired the capture recording. A CPU regression covers unknown FrameLink metadata with later list reuse, duplicate-before-Reset rejection, and Reset-before-Execute rejection.

Constants36 SHA256 `f4c943294a83c329521abc179d0e7ee4425d0ce40e98952a421eae3d10abcc5f` completed15s, disable, normal close, hook restoration and five-file restores. Two actual Color RTV-exit snapshots were saved (world/ticket0 = unassociated, physical nonce1/2, source state4 restored, actual queue signal/completion/reset verified). Both have reduced origin427x480 content and zero right half. They are distinct physical snapshots, not an associated stereo pair. The first right-eye loss point remains earlier/unresolved. The same run's current view snapshots show source427x480 AND integer target427x480 at each eye, while the intended feature output is640x720. Sampling remains tied to427x480 physical input. Constants37 tests a separate source/output target seam: only unpublished, independently allocated source-view clones may have their integer target rectangle changed to640x720, with full object readback proving every other byte unchanged. A distinct opt-in test environment flag gates this experiment; normal construction and original source/family bytes are retained. The unsuccessful native preprocessing-copy preservation experiment is now opt-in separately and defaults off.

## 2026-10-20: input geometry reference and packed-input pixel failure

The integer-target experiment was rejected: Constants38b changed the native MV dispatch domain to640x720 as well, so the existing producer check correctly refused a427x480 input claim. Integer targets are an input domain, not an independently editable final-output rectangle. This experiment remains off.

Constants39b (`4d2724ef41345b554e4ef327896d81812ee19a292367c297d6dc01d8710f9305`) established an actual unmodified mono reference: native Create853x480→1280x720, stable float viewport213,120,853,480, integer target0,0,853,480 and SDK bases0. SDK shadows and extra per-eye queries were disabled. It completed native disable, normal close and all five protected-file restores. The centred float viewport and actual input coordinates must be distinguished.

Constants40c (`8b1650ee2788339f4b8122f0dda1cea4a0f8a29dcb4e00eb7a1832ba662eeead`) tested packed426x480 inputs at integer x0/426, with float x213/639. Actual typed GPU copies back up right input before overlapping640-wide left output, restore right input before its Eval, and restore saved left output after right output. CPU pixel/state tests and the fresh existing suites passed. Actual same-ticket HDR readbacks also verify bit-for-bit that final Color left equals saved left SDK output and final Color right equals saved right SDK output. However right SDK output AND restored right input are all zero. Packing and preservation therefore do not fix the earlier right-eye rendering failure. Evidence and raw-file hashes are in ignored `analysis/runtime/dlss-packed40c-evidence.json` and its linked pixel inspection.

The40c hold completed but native disable encountered a post-key menu snapshot timeout; cleanup was incomplete. The process ended and all five protected files were restored. Do not label this normal-close acceptance. No final draw, XR image, playable DLSS or physical-headset acceptance follows. Packed adapter output publication remains explicitly blocked pending an actual final-restoration lineage join. Constants41 adds only two bounded post-draw pixel probes on actual right-region Color/Output RTV draws to distinguish an earlier scene-rendering failure from later postprocessing loss; its pixel result is not assumed here. Release/default DLLs remain unchanged.

## 2026-10-20: reduced-input union mask restores both SDK eye images

Constants41 completed493 pairs and normal close/restores but its overly strict viewport/scissor equality selected no draw snapshot. Constants42 checked their intersection and captured one right-region Output/scene-color draw; the right region was zero. Constants43 then captured the same ticket's scene/scratch Output immediately BEFORE each SDK Eval; right was already zero before the first Eval. These are bounded observation points, not proof of the earliest loss. Both runs closed normally and restored all five protected files. Constants44, with SR disabled and no SDK owner, read two actual native source copies: both640x720 eye regions contained finite scene pixels, including a99.9996% nonzero right region in the later snapshot. This confirms the underlying private two-view path can render both eyes without SR.

The existing `view_union_mask_fix` rejected reduced SR rectangles: its original contract required source=target, adjacency and full physical height. Actual41 status records1288 rectangle rejections, while only ordinary640x720 frames were corrected. The native first-view mask therefore retained its reduced first-eye boundary. Constants45 freezes the verified constructor float rectangles into the existing frame ticket. A separate private gate joins current Create/query identity, world/ticket, verified native renderer storage and the exact frozen rectangles before allowing translated/padded reduced input. The incoming float ABI, physical viewport, nonoverlap, query range and stale/mismatched geometry checks remain strict. CPU regressions cover both427x480 padded and426x480 centred packed geometry, missing receipts and nine mismatches each; ordinary behavior remains unchanged.

Constants45 SHA256 `bd32cc5801db3d98a77a64e943e64e6172f0ed09752da9c0b22492072744570d` used the simpler padded427x480 layout, with no packed backup/restore operations. Actual mask status:2268 accepted/modified, zero rejected; SR union right1067 changes incoming-0.332812488 to0.667187452. All948 input calls were accepted, with distinct SDK features and no transport failure. Same-ticket549 HDR SDK outputs each have100% finite and100% nonzero pixels, visibly different stereo views; left output remains bit-exact in Color before right copy. This is actual SDK pixel evidence that the mask correction resolves the right-eye black input in this trial, not just successful API returns. Native disable, normal close and five-file restoration passed with no owned foreground sample. Ignored `analysis/runtime/dlss-input-union45-evidence.json` preserves identities, file hashes, baseline, status and pixel evidence.

Final Color AFTER both copies, the final native quad/SRV join, XR publication, temporal quality, runtime render size and physical-headset acceptance remain separate. The old native generated quad still derives invalid/reduced rectangles for these inputs. Constants46 retains the45 correction and adds post-copy Color readback plus a bounded read-only final-output binding sample after actual SDK admission. Neither its result nor playable VR support is assumed here. Default/release artifacts remain unchanged.

Constants46 (`83c362fc1f8be303c856d06ca53ac18c36d02e54fd1bdd06d8e3d3f2d8bf55b8`) then verified final Color after both real boxed copies: each640x720 half is bit-exact to its same-ticket542 SDK result, and both SDK images are100% finite/nonzero. Native final Output samples at later ticket615 bind that same Color canonical allocation (`3dc969740`) at actual DrawIndexed6,1. Importantly, explicit view and TLS eye DO match at final Output for both eyes; the earlier stale SDK-wrapper TLS observation must not be generalized here. The generated d83cbf quad still supplies invalid unsigned-wrapped destination coordinates and full1280x720 source rectangles. Normal disable/close and all protected-file restores passed. Ignored `analysis/runtime/dlss-final-color-and-binding46-evidence.json` retains exact identities and bounded native binding rows.

Constants47b adds a separately compiled private adapter-recorded Color/quad geometry seam, retaining existing view/TLS, descriptor, named SRV, native list and draw checks. A narrow CMake exception allows its typed barrier observer together with final-output control only in the isolated, XR-disabled trial. Fresh CPU suites and reduced-output geometry mismatch tests pass; the first47 build exposed a stale offline mutex fixture and never launched the game. Actual47b failed at ticket528 admission during native menu application, before stable pixel sampling; settings were restored after process exit, but normal close/cleanup was not established. No final-output success follows. Further diagnostics retain the adapter's first failure line and admission phase/completion. A pre-entry guard also refuses sampling during an active SDK bridge scope, pending copy, transition pair or stale input ticket; these intervals must never enter the non-reentrant adapter merely to ask for a snapshot.

Constants48c adds bounded actual native R10 target readback AFTER accepted generated-quad draws, with real typed target leases acquired at ResourceBarrier callbacks, source state restored and actual Execute/Signal/completion/Reset retirement. This is a diagnostic path only; its game result remains pending. Default/release and XR-publication gates remain unchanged.

Constants48c subsequently hung during native menu enable and did not establish normal close; protected files were restored after the test process ended. Constants48d removes adapter/API entry from final-output observers. The actual right boxed-copy callback freezes `RecordedOutput` using its already-owned adapter Entry. Final-output scopes read that immutable receipt under frame ordering and independently revalidate current construction, SDK Color, descriptor-selected SRV, native view/TLS eye, command-list generation and actual draw. No SDK lock is acquired by the final-output getter.

Actual48d session `continuous-stereo-1652-20261020-031409-731bf5be`, DLL `70ad188e727f7b97bc8daae92505da48f275d73cdd6cce141b1feb737903fb30`, completed244 guarded native final draws and244 Output exits, without source/list rejection. World1/ticket613 readbacks show the actual1280x720 R10 target after each native draw; after the right draw both640x720 halves have100% nonzero RGB, with distinct stereo views, and the left half is bit-exact to the first snapshot. Both readbacks were submitted in actual Execute23547/member1 on canonical list `161b4a0280`, queue `18667be0`, and retired through real Signals/fence completion/physical Reset. Readback `generation=1/2` values are physical observer nonces, not different FrameLink generations. The saved inspection PNG decodes R10 UNORM directly; its color space has not been independently certified. Normal SR disable, normal game close, five-file restoration and no owned foreground samples passed. Ignored `analysis/runtime/dlss-final-target48d-evidence.json` preserves hashes, exact resource identities and pixel statistics. This verifies actual final GPU pixels; it still does not establish XR projection, temporal quality or physical-headset acceptance.

The default-off `GEARS_NGX_FINAL_XR_TRIAL` adds a separately guarded simulator-only staging route. It requires isolated SDK ownership, the final-output join, game resolution and the three background test flags. It copies the complete final R10 image after the independently guarded right native draw and restores the native RTV state, keeping a private COMMON-idle texture. Publication requires both same-ticket final eye proofs, an immutable matching pose, exact actual Execute membership/generation and the same native DIRECT queue/device. Mixed-frame batches, resets, missing output completion and mismatched poses cannot publish. Normal tiny-window swapchain binding is suppressed only for this private trial. Default/release artifacts and native owned-scene/final-transfer controls remain unchanged; actual simulator results are pending.

Actual49b session `continuous-stereo-31952-20261020-033722-73e94713`, DLL `56ebd815ea7023176101d1c4a59d041fb9f4a1091e2d06fcc6de247b48811c5c`, passed the continuous owner check. The private route requires the selected simulator manifest and keeps the continuous runner's strict `RuntimeTracked` pose profile (`simulator_operator=0`); it does not relax tracking checks to start.327 same-ticket native stereo pairs produced327 final R10 copies, all matched to actual Execute membership, and326 published successfully (one worker-preparation skip). There were no pair, output-proof, source, list or ambiguous-batch rejections and no SDK/GPU transport failure. Both eye Output scopes completed654 times. The XR receiver made325 actual scene copies and3076 projection submissions over34.162044 seconds; submissions include retained-image reprojection and are not the source frame rate. The recorded40.108256-second maximum submission gap includes initial startup/menu waiting and supplies no smoothness or performance acceptance.

Four receiver-side GPU snapshots at tickets710,749,760 and857 have exact1280x720 R10 sources, distinct640x720 stereo images and100% nonzero RGB in both halves after the probe's RGB10-to-RGB8 conversion. These pixels come from the XR shared texture, not a desktop/window capture. Native AA NONE navigation, normal game closure, all five protected-file restores, publisher drain/close, independent hook restoration and XR shutdown passed; no owned game foreground sample was recorded. AA NONE still does not prove native cache feature Release. Ignored `analysis/runtime/dlss-final-xr49b-evidence.json` preserves hashes, raw counts, pose/ticket snapshots, cleanup evidence and limitations. Default/release, physical-headset, temporal-quality, longer-duration and native runtime-render-size/owned-scene acceptance remain separate. A fresh-process1536x816/off-on-cycle check uses the identical verified49b DLL and test executables in another ignored runtime directory.

That follow-up requested1536x816 but the game actually selected1366x768. Only the actual dimensions count: both receiver GPU snapshots before disable contain distinct683x768 eyes, each100% nonzero RGB, from455x512 inputs. The off/on cycle then failed during re-enable; game process30088 exited and native cleanup/normal close were not established. All five protected files were restored after the process ended, with no owned foreground sample. The first runtime-directory attempt never launched a game because its copied startup test executables were absent; the second directory preserved the identical DLL and all required executables with matching hashes.

The actual exception-stream dump (`150d30d24eaaed49ff78c492c625ab2d2d1ac8d75e7b446b6ddf7300b6ad623c`) places thread4628's read access violation at `nvwgf2umx.dll+0x1268f8`, reading address-1. Offline DbgHelp unwind uses captured memory and timestamp/size-checked local images, excludes disk GOW code fallback, and contains `RtlExitUserProcess`/`LdrShutdownProcess` plus DLL cleanup. The driver exception therefore occurred during process teardown and does not identify the primary exit trigger. Export-nearest symbol names without local PDBs are not function identities. Before that exit, the queue hook first rejected `complete_candidate_transport` at capture line397;49b did not record the adapter's original failure line or release lifecycle. Constants50 adds bounded Create/Release lifecycle receipts and the actual callback completion value, without changing their retirement predicates, for a1280x720 off/on reproduction. Ignored `analysis/runtime/dlss-cycle1366-failure-evidence.json` separates initial-image success from re-enable and cleanup failure. Repeated enable, full native feature retirement and player-release acceptance remain unverified.

Actual50 (`16c1aaf06f5da43751d45a11d24e82af6207d06e5fe1ff3fce76e2fb81b6ce74`) confirmed both original SDK Release calls returned1 on thread35852, using actual completion device `18824fb0`, queue `71ecc9b0`, fence `3a7fb1d20`, value1563. The public handle then became0. An ordinary later Execute classified the deliberately retired pair as a transport rejection; the cached adapter failure457 is the normal `invalidate` call used by release. This set `Context.failed`, and the subsequent Create at ticket1599/revision3 was refused at stage20 before any replacement could be made. Re-enable and normal cleanup failed again; protected files restored after process exit. Ignored `analysis/runtime/dlss-release50-lifecycle-evidence.json` preserves exact ordering and native results.

Constants51 separates fully retired transport from rejection only when both real feature releases, creation and recorded GPU uses have drained. That state cannot produce an output or admit another Eval. A later complete native Create may start a fresh bounded quartet only with the same complete native fields/layout, world and device, and a newer source revision; the old typed leases stay pinned. The independently owned staging texture retains its own generation while its feature identity changes, with no pending old stereo packet and no world/extent/device change. The private route explicitly accepts initial world1 only. CPU regression checks a partially released pair remains rejected, a fully released pair reports retired without output, and the next valid epoch resets diagnostics. Fresh full suites passed (`e02b19bafdd3ab778e46b22539bde3433db52f0988ea176d3b31d87e0dbc6a19`); actual off/on-cycle results are pending. Default/release remain unchanged.

Actual51 advanced through successful old-pair releases and `create_wrapper_rebind` into epoch3 at ticket1647, without the erroneous transport rejection. Its first replacement Eval was nevertheless refused because the session had already entered stopping. The constructor had continued using the old reduced-input Create token during the no-SDK interval; existing source-identity checks correctly reject that token when a later Create replaces it. This trial therefore did not accept re-enable or normal cleanup; protected files were restored. The independent default-off build also compiled and passed selected baseline checks (`b890d9131c32018aeb2b028965e631c0149848b68bcf7dd21443ac21682e580b`).

Constants52b adds constructor activity policy, separate from raw Create identity. After a registered stereo consume uses no SDK pair, later constructors use the ordinary geometry until a newly accepted real pair is recorded. Old cached feature existence no longer authorizes reduced input during that gap. Recreate holds that policy until its replacement pair is accepted; stale older receipts cannot reopen it. Raw native identity, late-Create, view-mask and per-frame GPU publication guards remain unchanged. The gap observation uses the immutable frame saved before original consume, without looking up an already destroyed renderer afterward. New CPU tests cover inactive cached features, candidate holds, stale receipts and recovery.52 and52b passed fresh full suites;52 was not launched, and52b adds first-stop-source diagnostics and the immutable consume observation. Actual52b (`178d011335f93da40f761de3f62464aedb20cb41174e9885e1df5edf89de5c9d`) off/on results are pending.

Actual52b advanced to its replacement input callback and refused stage37 at retained-MV/selected-descriptor admission. It no longer stopped on the stale reduced-input constructor token. Re-enable and normal cleanup still failed; all protected files were restored after exit. Ignored `analysis/runtime/dlss-reenable51-evidence.json` and `dlss-reenable52b-evidence.json` retain the distinct outcomes. The default-off52b build and baseline check passed (`c8a0eeb15e1cb51ccc724af38a113662a8891d4f6622a77584e0936fc3468012`).

Constants53b keeps the independent source descriptor ledger live only for this private controlled-epoch mode, narrowing new UAV admission to full-size single-mip/sample R16G16 motion candidates after native output geometry is known. Neither source nor selected-target capacity/structural failure may be repaired. An explicitly retired feature epoch may select a different typed MV only from current observed source births/copies, imported strong heap leases and a fresh bounded CPU observation scope; an unobserved visible copy is not inferred. Old target banks and MV leases stay pinned, and historical first-Eval summaries cannot be reused as the replacement's SDK identity. CPU regression verifies replacement resource lineage, loss of old aliases, explicit subsequent visible-copy observation and rejection from a failed source.53 never launched because updated forward declarations were missing; fresh53b passed all suites (`daf181b15e2270a2ef15e2b7d94ecdf5371ab275228fb2e213cb7deef114bd08`). Its actual game cycle is pending. Default/release remain unchanged.

Actual53b session `continuous-stereo-35548-20261020-045038-cc6fda91` passed initial enable, AA NONE, same-process re-enable, another AA NONE, native hook cleanup and normal game close. The old real feature pair returned1 from both original Releases, replacement feature epoch3 was accepted, one fresh typed quartet and one MV descriptor scope were retained, and the independent staging generation stayed1. The live source ledger held18 cells without loss; the replacement selected target also reported no structural or capacity failure.222 native stereo pairs and222 actual Execute matches produced220 GPU publications (two explicit skips), with no pair/output-proof/ambiguous-batch rejection and no SDK/GPU transport failure. Both native Output scopes completed444 times.

Receiver-side R10 snapshots before and after re-enable have1280x720 full size and distinct640x720 eyes, each100% nonzero RGB. The post-reenable snapshot is ticket1544/serial159, saved at43276945028000ns after the second enable completed at43271752615300ns; its ticket is later than the new Create/rebind at1403. It is a fresh replacement-epoch image, not a retained old frame. Visual inspection found both complete views without a black eye or wrapped/cropped source. The XR owner recorded216 actual scene copies and2425 projection submissions, succeeded in the startup-to-projection handoff and shut down cleanly. The121.424344-second owner span and94.1740351-second maximum gap include graphics menus and no-SDK intervals; neither count nor span is a performance/smoothness result.

Native cleanup, independent entry restoration, publisher drain/close, normal process exit and all five protected-file restores passed, with no owned foreground sample. Ignored `analysis/runtime/dlss-reenable53b-evidence.json` preserves exact binary/file hashes, lifecycle receipts, actual Execute and GPU-image evidence. This completes the unattended simulator verification for the tested1280x720/Quality2/world1 enable-off-on scenario. The private trial remains simulator-manifest-only and process pinned, with at most eight identical-layout recreations. Default/player-launcher replacement, physical headset, UI completeness, temporal artifacts, long-duration performance, other quality/size changes, native runtime-render-size integration and world handoff are not accepted by this result.


### 2026-10-20: runtime-sized native owned scene integration candidate

`GEARS_NGX_OWNED_SCENE_TRIAL` is a separate default-off candidate, mutually exclusive with the private R10 staging/XR route. It requires the normal runtime-sized native-owned scene/UI pipeline and the isolated SDK/final-output join. The RGBA8 final RTV is admitted only when it is the actual active native Owner resource, renderer, world, ticket and planned size; an arbitrary matching-size RGBA8 allocation is insufficient. Both complete native Output scopes must additionally authorize the owned scene/UI publication. This does not promote the candidate into the player manifest.

The first54 build compiled but failed the actual system hook installation self-test before any game launch: owned output and NGX independently installed CopyResource and ExecuteBundle detours.54b uses one hook per ABI while preserving both observers, original-call count, bypass and unknown-use semantics. Its full startup and targeted suites passed (`e8d47c9e91a3feb159db411d3a00edfd2c2f6d9269c06c00e2c5a34e9831835f`); it never launched a game, and its waiting helper was stopped before protected-file writes. The normal player build options also passed compilation/full startup/independent CPU tests (`ebc6d1ee7321d61ca519cd652f767bd1d8bc5be91a7a256cf8645c1d11ca5e92`). Its disabled NGX descriptor mode correctly returns ERROR_NOT_SUPPORTED; this is not counted as a descriptor-test pass.

54d explicitly finishes the DLSS consume proof after original native consume returns and before owned scene end may publish. The destructor remains a fallback, with exactly-once finish and active-scope restoration. An actual wrapper regression requires incomplete proof to refuse publication, accepts the completed proof, and verifies explicit finish/repeated finish/destruction produce one finish. The producer collector uses the independent render-size plan only after admission of the actual owned renderer; Create parameters and all typed GPU resources still have to agree.54d runtime acceptance is pending. An existing game/launcher remains untouched while the exclusive background-test waiter waits; no current session is closed by that waiter.


Actual54d (`1fa13c246b21cf39df8fff1368d4aa80d665da4175ee3b1ad7b1fcb893030e7e`), session `continuous-stereo-21376-20261020-051401-323ae91b`, used the actual runtime plan1792x980 (896x980 each eye, scale0.7). Before DLSS, the owned scene/UI path consumed489 tickets and published488 images. Both real SDK features then created successfully with597x653 per-eye optimal input, and post-Create input/copy callbacks ran. At ticket499 admission observed an earlier UI proof stop; normal close/native cleanup and post-DLSS image acceptance failed. All five files restored, independently confirmed by the next exclusive55 backup snapshot. The last periodic status predates the refusal and must not be presented as a clean final sample. Ignored `analysis/runtime/dlss-owned54d-evidence.json` preserves this separation. The normal player-option54e build/full suites passed (`c61a2f9b913530a54ac0ccf105bad935270b0aac4ecfb46e16f575abea7533d3`).

Actual55 (`598b9beced93f65356fe98d8cfa47695a5b7da5acb38b085f8ca92759f6dff6e`) reproduced the failure and captured the first exact refusal: owned UI ticket514, reason9/draw_state, native draw caller172741, actual and expected resource199badb0,1792x980. Flags258 mean a correct single target and observed overlay chain, but no linked scene pair (pair ticket0). The HUD plan was not attempted, so this is not a HUD projection rejection. The DLSS output controller recorded zero admitted final quads/draws/scopes. All five protected writes/readback checks reported restored and no owned foreground sample; normal close/native cleanup and DLSS pixels remain failed. A later separate game owner changed live settings/profile files after that cleanup, so no corrective write or independent later comparison was attempted. Ignored `analysis/runtime/dlss-owned55-evidence.json` contains exact diagnostic receipts.

56 (`5f5d1dcd646212997e6ef68bbc04df5542d10e453df964839977e771c5c25604`) passed fresh full suites. It changes diagnostics only for the guarded owned candidate: sample the first complete post-bootstrap Output route after two accepted inputs, and write each bounded row only from its owning TLS callback after native Output returns. No concurrent live recorder read, geometry authorization or GPU proof is added. The background waiter preserves the current separate game/launcher and starts only after exclusive idle checks. Actual56 route evidence is pending; the player manifest remains unchanged.


57 (`01fc59254e739abdf3f48ef87b0f6c214e46f538ed8a72629b3548bf5f34626f`) supersedes56 before game launch. It additionally exports the bounded completed row's actual parameter descriptors/payloads and named SRV/list observations from the same TLS owner. The full startup and targeted suites passed.56's waiting helper31068 was stopped only after verifying no driver log/start, so56 never wrote protected files or launched a game. The current57 waiter is process31944, created2026-10-20T05:28:46.108843+08:00; its live tool session62324 remains waiting for the separate game37372/owner19380. State files alone are not used to treat these processes as stopped.

The pending integration audit also requires an explicit owned-publication treatment of the SDK's unpublished transition pair. The private final-XR route already requires an accepted RecordedOutput, whereas owned publication currently also accepts Output store no_site for normal AA-off frames. That no_site decision alone cannot authorize an SDK bootstrap image; this must remain distinct from an actual no-SDK gap when the engine keeps cached features. This is a source-level acceptance gap, not proof that a particular bootstrap image was submitted. Do not promote the owned candidate until this distinction and the actual final branch/UI images are verified.


Actual57 (`01fc59254e739abdf3f48ef87b0f6c214e46f538ed8a72629b3548bf5f34626f`) reproduced UI draw_state refusal at ticket514 and captured no ready-gated Output row. It therefore supplies no final-branch/UV conclusion. The protected-file write/read checks passed after its own process ended; no owned foreground sample. Ignored `analysis/runtime/dlss-owned57-evidence.json` retains the exact receipts.

58 (`087381c54bce24a67c96d2da83cde65365315b9eeb3e401ff121c4818a81e6c0`) added explicit SDK publication policy and passed fresh full suites; it never launched. CPU regression covers baseline no-SDK frames, cached-feature gaps, unpublished first pairs, pending copies, wrong tickets and replacement epochs. Controlled SDK frames additionally require the final Output store's complete site proof; no_site alone remains valid only for ordinary no-SDK frames. The player-option/default-off58b build/full suites also passed (`efecbf69638a0589a1ef440a09937cbeb13b8a6870dbcaf668c41082a5834504`).

59 (`7e837430c500558256f1e1caeb0694789237856149c383e00a313063f8dd8753`) extends discard to the first real candidate pair while GPU acceptance is pending, and synchronizes the outer transport cache if the existing actual-fence admit callback enters accepted phase. It also admits early diagnostic observation without waiting for transport acceptance; observation grants no publication permission. Full suites passed. Actual session `continuous-stereo-24280-20261020-054253-1256015e` recorded exactly two candidate discards (tickets545,546), each explicitly submitted_to_publisher=0, then UI draw_state refusal at547. No completed eligible Output row or admit-cache synchronization marker appeared. The cache timing change is therefore not established as the primary cause. Both SDK calls of ticket547 still returned1 before the later failure. Five protected files restored, no owned foreground sample, no normal cleanup/close acceptance. Ignored `analysis/runtime/dlss-owned59-evidence.json` preserves these distinctions.

60 compiled/passed but never launched.61 adds a bounded actual-draw diagnostic at the shared owned-output observation seam, covering R10/RGBA8 targets and up to eight unknown binding states, not only expected owned targets. It records actual target versus Owner, current versus last SDK ticket, final/quad metadata, tracking failure and a bounded native stack. This can distinguish wrong geometry/caller, a different output target, missing command-list state and lost thread-local ticket association without widening any drawing/publication predicate. Its build/runtime evidence is pending.


61 fresh full startup/targeted suites passed (`e1f5d018ea4df26af7d141e3b81feacef28efc6941fc6fa910a68e85c126d719`), including the updated candidate-publication policy regression. It has not launched. Waiting helper39860, created2026-10-20T05:48:33.814298+08:00, is live in tool session15244 and waits for separate game34948/owner32544. Its driver log does not exist yet; no61 protected-file writes have started.60 never launched. The pending trace is an observation only; current geometry/publication predicates stay unchanged.


## 2026-10-20: owned RGBA8 final output follows the DOF renderer

62 superseded61 before launch;61's waiter was stopped before driver logs or protected-file writes.62 passed full startup/targeted suites (`402a036be5860e734c059b569c0e347a86138d157987211f1249b6242e51f7b1`). Actual session `continuous-stereo-39852-20261020-060121-5640c6df` again discarded two unpublished SDK candidate pairs, then failed the native-menu transition. Five protected files restored after the test game ended; no owned foreground sample, normal native cleanup or post-DLSS pixel acceptance. The launcher-level movie-reader identity error is downstream of the renderer failure and does not establish its cause.

The actual draw observer captured both transition eyes drawing to the exact active owned RGBA8 target at1792x980, using native DrawIndexed6,1 caller1af92b and quad callerd808d8. The native stack includesd808d8/2b0bdb. These are bootstrap draws, not mature SDK output acceptance; the48-row diagnostic bank subsequently filled on intermediate RGBA8 targets. No actual owned-target copy was observed by the bounded hooks. Ignored `analysis/runtime/dlss-owned62-evidence.json` preserves the exact receipts and limits.

A bounded KB-first IDA export on a private historical-runtime database identifies the DOF renderer atd7fd14..d80b75, native bind returnd7ff84 (callee1665dc), and vertex setter/quad pairsd8084e/d808d8 andd80925/d80a7b (setter1760b4). Full function hash is `62ceae022db3ea1d7fd0994520e2cbff6c1b1dca288023d1a56bd957fbca7d5c`, source artifact1 SHA `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, base140000000. These are historical-runtime facts, not disk-EXE or live-byte equivalence. The private IDA lease is closed and the source database hash stayed unchanged.

The pending private63 candidate authenticates the complete live DOF function before installing its wrapper, preserves the full64-bit RAX return, and routes its named Color binding/vertex setter/final quad through the existing actual SDK RecordedOutput proof. It additionally requires the same-ticket existing VR DOF zero-constant proof at the actual quad. Missing SDK receipts remain ordinary no-site passthrough; unsupported vertex descriptors remain rejected. Actual UI publication still requires both completed native scopes. Diagnostics reserve quota for the owned target rather than exhausting all rows on intermediate targets. CPU wrappers and real-game/GPU validation remain required; no player manifest or release DLL has been promoted.


63c fresh full suites passed (`24bca123ceb835f7dbe079a571f62b7bed5d935f4cf9d4b2c3b232d867776ebc`). Earlier63/63b builds failed compile-time wiring/fixture definitions and never launched. The normal player-option/default-off63 build/full suites passed (`b0bda82664ea836fe177baa3154d563989aa8b6a0b786155941a048bfae65d35`). A separate verified offline DOF controller harness then passed both setter/quad pairs, reduced-input to full-output geometry, completed two-eye publication, and nine refusals including nonzero/lost DOF proof, wrong binding/site/target, stale/missing SDK receipt, wrong Color and unwind. Its SDK and DOF seams are synthetic; it does not establish game/GPU behavior. See ignored `analysis/runtime/owned-dof-control-tests.json`. The immutable63c game DLL is awaiting exclusive idle; a later separate game is preserved.


Actual63c session `continuous-stereo-34712-20261020-063031-663331ff` passed live whole-function DOF attestation. Both observed DOF routes use the exact no-op vertex descriptorffff,ffff,0 and actual named input callerd7ff84 with selection0/alternate0. Mature SDK ticket540 has accepted transport and RecordedOutput; the forwarded actual d808d8 native draws use896x980 eyes instead of597x653. Ticket541 switches to the second native paird80925/d80a7b: both forwarded actual draw rectangles are again correct, but final_eye_valid remains0 because the shared classifier/hook gate only recognized the first DOF quad. UI publication then fails; the last periodic native status is stale. All five files restored, no owned foreground, no post-SDK pixels or normal cleanup accepted. Ignored `analysis/runtime/dlss-owned63c-evidence.json` retains actual targets/list generations and the exact failure boundary.

64 additionally admitsd80a7b to final-eye classification only with a current controlled quad. Its arbitrary/uncontrolled calls remain ineligible. The common actual DLL self-test covers left/right classification, missing control, invalid ticket and full-width rejection for both controlled sites. The existing owned target/SDK binding/list/draw/output/UI proof is retained; this is not an unconditional caller whitelist. Actual64 acceptance remains pending.


64 full startup/targeted suites, including the real capture classifier regression and the separate owned DOF control harness, passed (`aefb0617c31b8db54f94097e4f76f74726e553450354b8e4b831afaa93d87962`). The normal player-option/default-off64 build/full suites also passed (`28921a4ba65d7e2b1973f97b784fffd37cceb4ccccaa022252052466cd822726`). The synchronous63c first-fault line independently confirms owned UI draw_state reason9 at541, missing linked pair0, actual=expected owned target; native periodic counters remain stale. Runtime64 still awaits the separate game/owner. No production manifest promotion is implied by these CPU results.


Actual64 session `continuous-stereo-33720-20261020-064432-18049c13` stopped before DLSS enable because native menu dispatch changed the registry and post-dispatch observation raised Object outside registry. Normal close, native stop and five-file restoration completed; no SR acceptance. Independent entry verification initially refused its91 unique hooks at the old87-entry inventory bound. The verifier now uses the102-slot maximum derived from current capture capacities, still independently checks every entry, and authenticates the full3681B owned DOF body using the fixed historical hash. All24 verifier tests pass, including corruption at either end and short reads. A successful hook inventory count is not a byte exemption.

The first same-DLL retry, session `continuous-stereo-33424-20261020-064758-7c981c44`, passed all native identity checks but prepare_trial refused the existing ngx-isolated-trial-events.txt before installing scene hooks. It did not test SR. Its owned game closed normally and all five files restored; incomplete activation/cleanup is preserved as such. The next queued retry archives that one prior event file only after all actual game processes and launcher ownership are absent, preserving its hash. It retains the native refuse-overwrite guard and the same immutable64 DLL. Runtime validation remains pending.


Actual64 retry2, session `continuous-stereo-37488-20261020-065312-d4a544dd`, completed native DLSS menu enable, returned to gameplay and held. The actual second DOF branch now records both correct final-eye identities and896x980 rectangles.822 completed Output scopes/draws represent411 SDK stereo pairs. XR receiver snapshot1, ticket678/serial654, is an actual post-enable1792x980 RGBA8 frame; both eyes have100% nonzero RGB, with99.9019% differing eye pixels, and inspection shows complete distinct gameplay views. PPM SHA `fb4f8d88eb38285b678d9329a1859bbf1f6edbc6a632bdaf63cbbd5ea5899905`; ignored `analysis/runtime/dlss-owned64retry2-pixels/inspection.json` records exact timing and limits. This is first-enable GPU evidence, not complete-cycle/headset acceptance.

Native disable returned0, but owned UI draw_state reason9/pair0 at ticket947 stopped production before reenable. No DLSS Output publication rejection was recorded; source/target and branch metadata for that gap are not yet captured because the initial diagnostic bank was exhausted. Normal game close, all five file restores and no owned foreground passed; native cleanup/entry restoration acceptance did not.65 adds a separate eight-draw diagnostic bank for actual owned-target draws whose ticket follows the last observed SDK input, retaining the current Output entry's POD view rectangles. It grants no gap/publication permission and does not infer full-ticket SDK absence from a draw-time observation.

The private menu probe also now rebuilds a stale reader only when exact Object outside registry occurs in the after-dispatch read. It never resends the key; before/dispatch_before, identity/window/unknown failures remain strict. Five tests cover recovery, exhaustion and changed identity. These harness changes do not establish GPU success.


65 full suites passed (`bc1edc8d87e6030e2cde3e7ee6858d82c8e240cb23635ad8d681b7c4118fb80b`). Actual session `continuous-stereo-30212-20261020-070403-af3bc66e` again passed first enable/hold, then reproduced the disable transition failure. The reserved bank captures953 and954: last SDK input952, no current SDK output receipt, live native source=target597x653 with padded right origin896, actual finald808d8 draws still597x653, final_eye_valid0. Constructor eligibility changes1 to0, but a second already-built renderer still has reduced input. Epoch remains active/cached. These are draw-time observations; full-ticket absence is established separately at consume completion. Game close/five-file restoration/no foreground passed; full cycle/native cleanup did not. Ignored `analysis/runtime/dlss-owned65-evidence.json` retains the exact rows.

The pending66 implementation collects a separate native-input receipt for current registered renderer/views after an earlier accepted SDK input. It freezes both source regions from that frame's constructor metadata, or requires the full normal native layout after suppression. The actual named HDR source, same-ticket zero-DOF proof, owned target, list generation, draw, native return and UI checks remain required. Native gap quads map those current input regions to the complete owned eyes; they never reuse an old SDK output as the current image. After consume returns, owned publication must independently classify actual no-SDK use. Each eye also records whether its geometry used a current SDK receipt, so native/mixed eye proofs cannot authorize an SDK-owned DOF publication. Pure/wrapper tests cover reduced/full input on both routes, changed current native Color, stale receipt, zero proof, caller/target failures, malformed geometry and mixed SDK/native refusal.66 passed full suites (`4780d06fecda816fa155cb0db0a1dce712b04f2043eb3b270728b5cbebbf60d6`) and never launched;66b refreshes a diagnostic-only print and additionally requests an actual off-state receiver snapshot before reenable. Runtime acceptance remains pending.


66b fresh full startup/targeted suites passed (`6ecdac955b55718177eb0b51ca507814a1b29c0c9902c2838e338da8a66f885d`). The normal player-option/default-off66 build/full suites passed (`a4737a6cbdf38abf0d9559a6b40e697cee21932528491e14493448557d2fa4c0`). Both include the expanded actual-controller fixtures for native reduced/full inputs and SDK-proof separation. The player manifest remains unchanged.66b's owned background waiter is process29320, created2026-10-20T07:26:06.997439+08:00, tool session19686; current game33516 is a separate session and remains untouched. Actual66b acceptance is pending.


Actual66b session `continuous-stereo-39764-20261020-073018-4e81804c` passed first enable but again refused disable-frame946 (last SDK945). The native-gap receipt and named binding are valid, while the quad fails before actual-draw certification. Source review identifies an implementation error: Create-only Layout intentionally leaves all physical extents zero until Eval; native_output_ticket read those zero fields. The fix derives expected physical topology from the already retained typed Color descriptor and checks it against the last accepted producer's Color identity/device. The current native shader binding is still independently read at Bind/quad/draw and may be a new native Color; no old SDK pixels are reused.67b's tests explicitly require unknown physical extent to refuse geometry, then admit the retained descriptor, and reject seven topology corruptions. A separate bounded immediate native exit receipt preserves any subsequent failure without extra queries in its formatter.67 compile failed only on a test assert macro/template comma and never launched; fresh67b full suites passed (`f2bc88d46badc9e65d3ac6e79efe6c81702c5b8d168b4d8a17324c0512aa79d7`). Full-cycle game/GPU acceptance remains pending.66b closed its own game and restored all five files; native cleanup/entry restoration acceptance remains incomplete.


Actual67b session `continuous-stereo-35424-20261020-074648-02e8c47f` verifies first enable AND disable rendering. Disable-frame983/984 retain597x653 constructor regions; their native source and physical1792x980 receipt now produce both complete896x980 final eye draws, binding/setter/draw proved and failed0.985/986 restore full native input. No UI refusal occurs in this gap. Receiver snapshot2 ticket1117/serial1071 (saved53792995433900ns) shows complete distinct native eyes after disable,100% nonzero RGB; PPM SHA `09056531de673bcdedeeeeaeac3186875c167241d556f51b0e17b1c2ecee3dd8`. Snapshot1 ticket706 verifies first SDK-enable output. Ignored pixel/evidence files preserve exact timing/hashes.

Reenable still fails at UI1422. Actual native Release of the old pair and Create rebind to revision3/MV rebind succeed; the first current SDK input is an unpublished transition. Its final route has no accepted SDK output receipt, while the earlier native-gap collector requires a prior input ticket. The resulting missing scene pair causes UI refusal before the existing later discard policy can retire this transition. The admission refusal at1423 is downstream (stopping1, source owned_ui_observer:61), not the initial producer/SDK failure. Five protected files restored and no owned foreground, but game/cleanup acceptance remains failed.

68 collects current unpublished SDK constructor regions separately from accepted SDK output, lets the real scene/UI/queue proof complete, and then always discards that image. Its proof remains unpublished even if GPU acceptance advances during the scope; native or SDK publication cannot upgrade it. Per-eye owned epoch identity also prevents mixing epochs. The complete render-candidate witness joins the current SDK ticket/epoch in publication policy, so a late cache update cannot publish a frame drawn with candidate geometry. Rebind diagnostics retain the first eight owned draws per bounded recreation. CPU fixtures cover full/reduced candidates, late mixed SDK readiness, wrong epoch and missing copy/ticket refusal. Fresh full experimental68 suites passed (`2496bc56e8f87c34ceb1632d79cf40ce4739ecf4c9e2036508ddc8beb47f7669`); normal player-option/default-off68 suites passed (`b06b1e0de270f2585915d370b658a902cd812f6cdfc995cc735ab19382ae35bf`). Runtime68 acceptance remains pending. Player manifest unchanged.

## 2026-10-20：owned68 实际完整开关、GPU 图像及退出通过

上述待验证项由 session `continuous-stereo-39412-20261020-080820-5ddf8f7d`
完成。游戏 PID39412，创建 FILETIME `134369284771875483`；实验 DLL SHA256
`2496bc56e8f87c34ceb1632d79cf40ce4739ecf4c9e2036508ddc8beb47f7669`。
磁盘 EXE SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`
与历史运行时快照 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`
继续分别记账，基址 `0x140000000`。四次原生菜单操作均返回0，完整开启/关闭/再开启/再关闭完成。

首次 SDK pair 在 ticket532 建立。关闭后的942/943仍保留597x653构造区域，两个真实最终绘制
均有有效 native-gap、绑定、setter/draw 证明，failed0；随后恢复完整原生输入。
再次开启在1341完成新的 SDK Create：两个新 handle 为 `1614d79370` 和 `1614d81330`，
与初始 `1614d89940`、`3dc2e58a0` 不同。原生 Create revision3 与观察器 epoch2
是不同计数域，不混同。1341/1342以 unpublished_sdk1完成场景/UI证明并被丢弃；
1343/1344的左右眼均恢复 sdk_output1/native_gap0、实际draw1、binding_proved1、failed0。
首次启动532/533与重开1341/1342共4张过渡图像均明确 `submitted_to_publisher=0`。
最终2450个 Output scope全部具有实际draw及return完成证明，publication_rejected0，
UI faults0、owned_error0，SDK owner/gpu failure均0。

XR接收端保存的是实际1792x980 RGBA8图像，每眼896x980：

| 状态 | ticket / serial | 保存 monotonic ns | PPM SHA256 |
| --- | --- | --- | --- |
| 首次开启 | 682 / 660 | 55032645770800 | `68d86c9e3a7b9f039c001bb40f026e806e1ae0a9f52e7dc3323d8ff203d2c46b` |
| 关闭后 | 1056 / 1014 | 55083205789000 | `425b56afbfdc5ce926c686e5d243b0873972d55781a9a3ae1f04b5d9952b72c2` |
| 再次开启 | 1472 / 1404 | 55138535361900 | `b439143bb1191e23a205337f163e586f622f70ff1aa7c543be816c1ab43e7cf3` |

三张左右眼均100%非零RGB，逐像素左右差异约99.90%；这些数值只是辅助检查。
协调者逐张查看PNG，确认完整庭院/建筑/人物双眼画面，没有单眼黑屏或低分辨率画面只占一角。
关闭状态边缘锯齿更明显；静态图不能评价运动拖影、历史稳定性或真实性能。
时间戳明确位于对应操作之后，重开图1472也晚于SDK恢复的1343/1344。

会话正常停止，native_cleanup_verified1、entry_restore_verified1、XR exit0。
独立验证100项范围/入口字节全部匹配，其中包括 owned DOF完整3681B历史范围的固定哈希，
没有扩大可接受字节或跳过不匹配项。随后正常关闭游戏。驱动在08:12:22写最终报告前，
逐一恢复并回读校验全部5个设置/profile文件；没有焦点请求、窗口移动/缩放或物理控制器输入，
自有游戏实际前台样本为空。08:12:36启动的另一个游戏PID36544不属于此测试；
后续文件比较中的变化属于更晚的会话，未覆盖其文件或关闭该进程。

精确身份、原始证据哈希、菜单操作、丢弃和恢复记录在 ignored
`analysis/runtime/dlss-owned68-evidence.json`，图像及统计在
`analysis/runtime/dlss-owned68-pixels/inspection.json`，选定源码的事后哈希审计在
`analysis/runtime/dlss-owned68-selected-source-audit.json`。事后审计不是完整的编译时源码快照。
原生/SDK历史观察器的零值诊断并未被追认为全局画质认证；本轮接收端图像及实际scope独立记账。

本结果完成隔离构建的无人值守游戏/GPU开关验证。默认玩家manifest仍指向原
`build-mounted-fps1`，DLSS锁定关闭；没有把要求后台测试标志的DLL直接替换为玩家版本。
实验限制仍包括world1、最多8次同布局重建、SDK资源/feature保留至正常测试进程退出。
物理VDXR/头显、跨世界、预设DLSS启动、运动历史画质、Frame Generation及性能验收仍独立待做。
会话216.38秒，1706次场景复制；18026次投影提交包括旧图复用，不能用作渲染FPS。

---

## 中文

# V 1 DLSS SR：隔离启动与输出分支

2026-10-20 最新范围：用户将 DLSS 确认为首版可选功能，允许独立 CMD 入口；普通启动器无需为它解除保护。
其他 Bug 修复完成必要验证后直接集成正常入口。本条覆盖本文历史段落中 DLSS 必须纳入 V 1 的约定。

2026-10-20 后续头显反馈：独立测试入口可以运行，但帧率很低。手动 NgxManual 2 会话
`continuous-stereo-8492-20261020-184444-d9cb27ab` 的正常游玩样本约 13.73个新双眼帧/秒；
接收端实际 6144×3264、每眼 3072×3264，不能把 120Hz XR循环当作游戏帧率。
关闭DLSS的匹配尺寸后台基线也仅 17.14 FPS；旧构建开启时拒绝MV证明，未取得有效SDK开启段FPS。
没有以失败轮补零或把启动成功追认为完整DLSS头显验收。

另一个受控A/B/A中的单描述符复制快路径得到稳定约 22.00→25.79 FPS，但不构成完整DLSS开关验收。
私有 `GearsCaptureNgxDeviceCache1.dll`（SHA 256 `a00536b049b0157c5281eadb19a437e32b061cd4995966c032b085f64b2dd4a8`）
进一步缓存持有的typed设备接口，保留首次/不同接口的canonical身份核验，另处理有界较大上传区中的稳定 160B前缀。
完整Shader哈希和其他渲染/发布要求保留；CPU、启动及真实D 3D 12描述符fixture通过。
其首次FPS窗口与独立autoplay 53实例重叠，已取消自有测试，不能据 13.48 FPS评价候选。
自有游戏正常退出，native清理、独立入口还原和 5文件恢复均通过；未关闭其他实例。
候选未切换玩家入口，SDK开启及性能收益仍待独占实测。

用户授权等其他会话结束再测。待办与精确证据为 ignored
`analysis/runtime/dlss-performance-queue-20261020.json`。性能测量需等待所有游戏与自动流程worker退出，
不能只检查普通启动器互斥锁，因为隔离实例有独立锁。资料和私有候选继续留在ignored analysis/。

2026-10-20 20:12：后续私有候选 `GearsCaptureNgxBatchScratch2.dll`（SHA 256
`906bf7cb36abcbe6b7c2d2406ebb7f7ea1385c51a1c3938622b32de178f75019`）保留上述设备缓存，
把批量描述符复制的 128格默认构造改为仅构造实际快照；全部源快照仍在目的写入前冻结并持有资源。
全新构建、完整启动自检、针对检查及真实D 3D 12 fixture通过；没有实际游戏收益或SDK开启通过结论。
`performance_guard.py` 的 5项检查通过，覆盖隔离游戏、轮次间worker、PID复用和精确owner身份。
新协调器先等待全部其他游戏/worker结束及五分钟空闲，再用独立实例的 5个配置/存档文件做实际对照；
真实用户文件不参与写入或恢复。生成驱动的实际导入、私有路径、自检和每眼 3072×3264计划预检通过，
预检没有启动游戏。保持用户MotionBlur及渲染倍率。运行中遇到其他游戏会取消自有测试并排除数据。
队列记录协调器精确创建身份，继续前先核验它，避免重复启动；玩家DLSS入口尚未切换此候选。

## 当前结果：2026-10-20 owned 68 无人值守完整开关验证通过

隔离实验构建已在实际游戏、运行时尺寸的原生 owned RGBA 8 场景/UI 路径完成
“开启 DLSS → 关闭 → 再开启 → 再关闭”。首次开启、关闭和再次开启后的真实 XR 接收端
GPU 图像均显示完整左右眼场景。正常退出、原生清理和独立字节还原校验通过。
这是后台模拟器验证；默认玩家启动器仍使用原构建并锁定关闭 DLSS，尚未接入此实验构建。
物理 VDXR/头显、运动中的历史画质、帧率、跨世界和预设 DLSS 启动均不在本轮通过范围。
精确证据在 ignored `analysis/runtime/dlss-owned68-evidence.json`，本页末尾记录本轮身份及边界。

## VDXR 手动测试入口

2026-10-20：手动入口现为 NgxManual 3，保留 NgxManual 2 的普通 Output/owned DOF 证明隔离修复，并减少单描述符复制的CPU开销。同场景A/B/A在每眼 3072×3264、DLSS关闭时提高 17.2%，三轮清理和设置恢复通过。没有据此宣称开启DLSS后的帧率或物理头显验收；重新打开测试启动器后生效。见[启动故障修复](162-dlss-manual-startup-proof-isolation.md)及[性能对照](dlss-descriptor-copy-performance-20261020.md)。

根目录 `GearsVR DLSS Test.cmd` 打开独立测试启动器。选择 Quest / Virtual Desktop (VDXR)，
点击启动 VR；进入关卡并确认双眼正常后，在游戏画质菜单选择 DLSS，先使用 Quality（质量）。
这个入口解除启动时和菜单中的运行时锁；启动仍先将保存的 DLSS 设为关闭，供场景挂钩就绪后
手动开启，不尝试未经验证的预设 DLSS 启动。普通启动器及其默认清单保持原有行为。

首次测试保持当前分辨率和渲染倍率，在同一关卡内观察左右眼、转头、移动和 UI，再做一次
开启/关闭/再开启切换。跨关卡、读检查点和改变布局仍未验收。结束时停止 VR 并正常退出游戏；
重新测试请关闭测试启动器，再次双击该入口，使日志和 DLL 工作目录重新隔离。

手动构建 `GearsCaptureNgxManual3.dll` SHA 256
`7eda76dbcdc4cea004914840f969c895c1c5ed4bb1c20a5e0aea634b66ccd657`
通过全新 verified build、完整启动自检、DLSS/owned-scene 针对检查和真实 D 3D 12 描述符 fixture。
这是为用户头显测试准备的新构建，其头显通过状态不能由之前 owned 68 模拟器结果代替。
新增默认关闭的编译选项 `GEARS_NGX_MANUAL_OWNED_TEST`，仅适用于原生 owned-scene join；
运行还需显式 `GEARSVR_TEST_NGX_MANUAL_OWNED=1`。所有实际渲染、SDK、命令队列及发布证明
保持原要求。手动模式不设置静音/behind 标志，不允许把最终 R 10 staging 路径升级为头显入口。
原后台三标志要求保留。启动策略单元测试和 4项脚本测试通过。

脚本校验构建收据、9个启动 DLL/EXE 和 VDXR 库；每次打开GUI把认证产物复制到新的 ignored
`analysis/native-dlss-manual/<id>`，不覆盖旧事件证据，也不安装/修改系统 OpenXR Runtime。
独立清单为 `tools/vr-launcher/manifest-dlss-manual1.json`。当前游戏若仍在运行，入口要求先正常退出，
不会终止已有会话。源码编译前后哈希一致性及CPU证据在 ignored
`analysis/runtime/ngx-manual3-source-audit.json`、`ngx-manual3-tests.json`。

## 2026-10-19: 独立源分类账达到实际MV

新鲜源代码1 DLL `602116a77c48f158271265a7292434677f9ea98a9d00d989288f82313ca2765a` passed457 依赖性、CPU源政策检查和真实 D3D12 证明在故意遗留损失后源源源源源源的延续。35632 第一Eval世界组织1/ticket604 保留了打字的MV,并找到准确的1 当前不可见源描述符。 分类账_ ready1/device_match1/legacy_failed_at_eval1, 版本47/origin47,没有复制源前身,保留了明确的视图字节。没有整数到COM的铸造或遗留故障恢复。

拆卸源分类账仍然健康:58heaps,40 单元格( E)32 已知无人驾驶航空器,8nonUAV,88unused),75 无人机记录着,7420289invalidations,11055932 复制, 0 其它设备/ 丢失/ 不确定/ 丢失。 这可以确定当前源代码, 并解决仪器饱和障碍; 它没有建立调度绑定或 MV 写入域。 下一步只能针对保留 resou rce带有明确独立的定界观察时代.

本地/ 输入清理、 正常关闭和设置恢复通过。 会话不完整/ 所有人退出1,因此没有可播放的 DLSS 索赔。 `analysis/runtime/dlss-source-ledger1-evidence.json`。默认玩家 DLL/显示不变。


## 2026-10-19:目标源头引导设计审查

Astra评论确定了一个重要的界限: 遗产 银行倒闭后,Call 录取关闭,所以19 UAV 写作只是前缀,不是完整的创建普查。独立的堆积普查存活下来,但打字的描述性观测却没有。当前被冻结的对子在它选中的票之后也停止了,所以未来计时器实验无法默默无闻 重新使用其已完成状态。

第一个执行阶段是独立被接受的源分类账,而不是更全局的复制存储. CreatUAV 目的地和当前非阴影可见的CPU别名. 禁止分配普通的遮荫可见副本复制件; 仍然更新/ 将每个已跟踪到的目的地, 包括可见的直流UAV 诞生。 未知来源和其他描述符会写出擦除出处。 在写前验证范围并抓取来源 。 分类账有它自己的极限容量、 故障状态和排水量 。 COM所有权;遗留故障保持不变. 接受 Eval可能只匹配保留型MV租赁与当前健康源细胞,保留原范围/版本.

这种狭义的阶段测试是否存在可用的 MV 源描述符。 它不能证明根3 绑定、调度、 GPU 执行或每眼 MV 写。只有在源头匹配工作完成后, 才能有单独的明确目标观察, 抓取随后的完整票, 将当前堆积/根部签名/表绑定到调度中, 检查特定的阴影/ c 显示器。玩家 DLSS 仍然禁用,直到实际渲染接受。


## 2026-10-19:停止整个装置能力升级

Fresh5121 DLL `97da3228d5601bf40696b64bec1e1bbed0043df6eeb94107f7d3c4eeb3bdfe74` passed457 依赖、启动/描述定型和512/513 occupancy,128/129 复制边界测试。 真正的 COM 复制抓取 。46080 通过单独的字节128 副本限制。实际会话22752 在接受Eval加入前再次失败 :44heaps,512 已知的无人驾驶飞行器电池,0invalid/free,2529 reuses,19 无人机写道:18528 invalidations,27402 两名Evals保留了MV,但银行已经准备就绪0. 没有失产/失产/失产/失学/不确定。

将被抹除的细胞再利用或适度的持久性容量增加作为整个设备观察器的充分固定。不要盲目地扩大它。下一次审查的目标保留了原始未爆炸航空器的诞生和随后选定的MV描述文本,并附有 di 静态观测范围, 没有修复历史损失的索赔。 请求 Astra 检查; 设计尚未实施。 这仍然是诊断性工作, 未经证明 DLSS 根源 。

原生清理/入口恢复、正常关闭和设置恢复通过;会话不完全/所有者退出1。被解禁的忽略收件 : `analysis/runtime/dlss-descriptor-5121-evidence.json`。玩家默认值不变。


## 2026-10-19: 描述符再利用反事实

Reuse1 DLL `ebec5cfbbbb36b81de7d0d6c11afd3db7ac8d7197fd961a00de2f228d68efd08` 刚通过457—— 依赖性建设、 重点CPU 和本地描述式固定/启动测试。 相同的地址 ABA、 保留快照、 重复使用前的源复制和所有已知容量失败都包括在内。 实际会话32900 接受了2次Eval观测,并保留了MV。128 已知的无人驾驶飞行器单元格,零无效/无空单元格和43 堆积( 堆积)19 无人机写道:2144 因此,墓碑有助于解释剩余饱和度;活版复制的描述符超过约束。没有已知无人驾驶航空器被逐出,也没有找到失效的银行。

会话仍不完整/ 所有人退出1。没有可播放的 DLSS 证据。本地/输入清理、正常关闭和设置恢复通过。精确的占用和身份证据被忽略 `analysis/runtime/dlss-descriptor-reuse1-evidence.json`下一个候选选项可能会增加明确的生产电池约束,同时保留通用限制并拒绝溢出;它必须说明范围类型和钩子堆栈的使用。


## 2026-10-19: 输入 MV 加入候选实际边界

Fresh `build-ngx-descriptor-join1` SHA256 `d28c9d00e99d14fcf03e9cb7443efbc72e50d1011b7cac32aff7bf02eaaebda1` passed457 依赖性检查、连续启动套件、出处/普查/时间CPU测试和真实D3D12 生产大头盖96 以上一个60 创建普查; 通用默认测试8。输入的MV 从未将 NGX 整数投放到 COM 指针上。

土著菜单会议32716,世界1/ticket1403 接受两个Eval, 并保留相同的MV/设备。43 堆积和128 单元格(容量损失)3),与19 无人机写道:2259 无效和2067 副本; 缺少出生、 其他装置、 重复和不确定因素为零。 因此, 两者都正确合并报告银行_ 准备0 和无匹配。此选项将删除前一个8 检查无效的细胞保留后,才会任意增加细胞容量。

会话阶段不完整, 主人退出1; 本地清理/ 输入恢复, 正常的关闭和五文件恢复通过。 没有可播放的 DLSS 或成功显示请求。 本地状态档案现在使用实际的前缀和成功 : SHA256 `41a61e561ff9d38477e38a79fac822c6b2be3ee450732dfb0698994a1bb9ecce`; 忽略了管理收件 `analysis/runtime/dlss-descriptor-join1-evidence.json`。默认玩家不受影响。


## 2026-10-19：早期 heap census 实际结果

独立诊断 `build-ngx-descriptor-census1` 沙发 256
`c546d426bdfe07535cae3e47cf8d70f1bf93007f48c9d38438655defd8a2dfa9`
通过全新编译、CPU census/provenance/epoch 测试、真实 D 3D 12 描述符 fixture 和启动自测。
后台小窗口、静音运行 session 38876，通过游戏原生菜单启用 SR 后，同一 world 1/ticket 1434
接受两个 Eval。每个冻结点累计创建 60个 CBV/SRV/UAV heaps：19个 shader-visible
（描述符累计 75776、单heap最大 4096）和 41个非shader-visible（累计 11352、最大 2048）。
另有 20个其他类型 heap；API失败、读取失败、饱和和重入排除均为 0。
这是跨设备累计创建量，不是存活heap或显存字节数，也不是MV producer认证。

原 8-heap provenance bank仍在第 9个创建处因容量失败；新的独立scalar census继续统计，
因此现在可依据实际数量设置下一轮有限容量，而不是猜测。当前仍未关联MV资源与真实writer。
GPU completion/domain仍为 0。这轮XR场景提交为 0、会话incomplete/owner exit 1；不能算DLSS显示通过。
native cleanup和入口恢复通过，游戏正常关闭，五份设置/profile恢复。

私有驱动沿用了旧固定归档名，归档步骤报FileExistsError且未覆盖旧证据；协调者随后从本session
专属native-status保存了新证据：`analysis/runtime/dlss-descriptor-census1-evidence.json`
和`dlss-descriptor-census1-native-status.txt`。后续驱动须使用本轮prefix归档。
默认玩家版本未改动，DLSS仍锁定关闭。

## 2026-10-19：Compute Dispatch 边界汇编核对

KB查询后，在已有私有IDA副本只读导出 `1ac6e4` 与 `17b420` 的全部热/冷chunks；导出后关闭lease。证据为 ignored `analysis/ida/ngx-copy-contract-20261019/compute-chunks.json` 及同目录两份汇编。所有字节与历史blob `911ab9…` 一致，组合hash分别 `151372ef327a20c9c6d56d227d371601a80ea61e7f9d572858a41b4d60cd6761`、`c84b4ac0e66f5f17bd7465a26fd54a87017bbd0162f1b2be98b895e599721e43`。原始资料库hash仍为 `324248ac7cd0388a05907c29436fccf804eb82e5cb8433190806342c1cfcbb26`。

`1ac6e4` 保存入口R 8D/R 9D，调用 `17b420(context+0x40)` 后，从 `[[context+0x127c]+0x14]` 取得命令列表，在 `1ac830` 调用vtable+0x70，返回 `1ac833`；三个Dispatch参数来自入口R 8D、R 9D和第五参数。`17ba1e` 则调用vtable+0xd0，`17ba24` 清空累计barrier数量。这证实后者不是Dispatch位置，不能从其附近MV的UAV屏障推断实际writer。

绑定助手还在 `17b982` 调用device vtable+0xb8，在 `17b994` 调用command list+0xf8（EDX=3），随后处理其他根参数分支。需要将实际UAV描述符和常量与同一ticket的MV及view对应；当前仅确认调用位置和参数来源，不确认这个通用Dispatch就是MV生成pass。本轮未增加hook、未运行游戏，也未证明当前进程字节与历史快照相同。

## 2026-10-19：原生 Copy 保持接线已集成，未启用 DLSS

将上述静态契约接入控制器，移除未实现的 `copy_states` 回调：现有启用检查新增六个完整函数块哈希；选中的 Output→Color 复制要求返回 RVA 1c 6093、原始零 XYZ/null box/subresource 0、固定的 typed 四资源、当前 world/ticket/renderer/eye 及真实 command/device/generation。仅调整复制区域和目的偏移，保持游戏原有状态设置、barrier 提交和恢复顺序。无关复制仍转发；选中复制验证失败不会回退全图复制。日志 chunks 改为实际表长度。

协调者运行 `NgxNativeCopyPreservationTest` 32项 CPU 检查、`NgxTypedQuartetTest` 253项及 `NgxStereoHostLiveTest` 均通过；host 开/关两个完整 capture 构建成功，Ninja 依赖分别 447/462且有效。构建日志在 ignored `analysis/runtime/ngx-copy-integration-{on,off}-build.log`。默认 Integration 1 沙发 256 仍为 `41a5b882ab1202940c0590d88a6c732609fd0db9511157e19f51f802064cc065`。

本轮无游戏或 GPU 验证，没有激活 producer，也未改变发布默认。Color/Depth/MV 实际生成区域、共享参数串行化和实际 DLSS 连续游玩仍需证据；这些 CPU 检查不能证明 DLSS 已可用。

## 2026-10-19：原生Copy契约静态核对

先查询KB artifact 1，再在原IDA库私有副本导出 `1c5dd8`、`1c60f0`、`1c617c` 完整chunks汇编；每段字节与历史运行时blob `911ab9…` 对比一致。复制函数含热/冷两段 789+569字节，组合哈希与KB一致。另机械核对其状态帮助函数 `17c0b4`/`17ab94` 的完整范围和哈希。证据 `analysis/ida/ngx-copy-contract-20261019/copy-chunks-with-state-helpers.json`，不进入发布包。原IDA库前后SHA 256 `324248ac7cd0388a05907c29436fccf804eb82e5cb8433190806342c1cfcbb26` 一致，私有lease已关闭。

汇编支持的精确分支：`1c5f54`/`1c5f79` 向状态helper传入 0x 400/0x 800；`1c605e` 通过command list +0xd 0提交积累barriers；`1c608d` 经+0x 80复制，返回点 1c 6093。原始XYZ为 0、box为null；后续两次 1c 617c按scope保存值恢复状态。完整函数还含CopyResource及其他分支，不能按函数名把它们一并允许。缓存伪代码的参数位置及部分常量与汇编不符，不作为ABI接线依据。

据此可走“保持原生状态转换、同资源/列表/顺序，仅限制复制box和目的偏移”的契约，无需伪造状态tracker。但历史代码相同不证明当前资源、实际生产区域或GPU完成；生产端必须验证当前完整chunks、确切返回地址和typed参数，且仍等待Color/Depth/MV生成域证据，不能据此启用DLSS。

## 2026-10-19：控制器持有 typed 四资源

Context现在只从当前有效消费ticket内的typed Barrier/Copy实参获取资源引用，覆盖UAV与alias参数；NGX整数只与已持有对象匹配，不转换成COM指针。首次有效Eval固定Create epoch/world/device及Color/Depth/MV/Output四资源，后续替换拒绝。绑定前的候选有界 64项、不同ticket不混用，绑定后仅保留四资源；满容量时非SR或异设备资源仍按类型筛选忽略，新合格纹理溢出才拒绝。资源身份持有不构成producer区域、资源状态或GPU内容证明。

停止、失败、world变化不清空已绑定资源；仅现有detach检查确认wrapper/active/GPU/feature均退休后释放Context持有。frame锁与活动计数保护采集和退出边界。真实首次Eval之前是否能在此窗口取得全部四资源尚未验证；缺项拒绝，不从诊断bank补齐。

协调者实际运行新CMake目标NgxTypedQuartetTest（253项CPU检查）和HostLive回归通过；私有提取的实际退出保护 28项检查通过，退休事实使用测试替身。host开启完整capture重新编译通过，Ninja 447依赖包含新增header，私有DLL SHA 256 `98d29546682c388d595f230afdf2f6367c91755b04654d29500c8720b1abc792`。另用build_verified新建默认功能组合但host关闭的完整构建，462依赖验证通过，未覆盖已验收默认DLL或修改其manifest。此轮未运行游戏、未启用DLSS。

仍缺：真实Color/Depth/MV生产域、完整认证的原生Copy路线及共享参数串行性，之后才可做实际DLSS双目与连续游玩验证。推荐尺寸默认模式不受本次源码准备影响。

## 2026-10-19：实际界值确认整幅宽度下限冲突

首pair完成接线已加入源码：真实Create、两次Eval、两次boxed Copy的提交义务全部在匹配device/queue/fence上完成后，才能接受传输并允许后续ticket。该API不构造像素观察，不声称画质正确。Execute后先非阻塞检查；同步原生调用到达下一ticket而首pair仍pending时，在frame/adapter锁外最多等待 2000ms，只等已提交且Signal已返回的工作；stop/world变化或超时停止并保留未退休资源，不重放SDK。私有完整调用链锁审查及CPU回归通过；协调端在修复header依赖的目录完成capture重新编译，Adapter/HostLive测试直接运行通过。真实producer域、原生Copy契约、参数串行性及实际GPU连续画面仍未接通或验证，默认未激活。原始日志 `analysis/runtime/build-ngx-transport-integrated.log`。

后续Ninja构建使用 `tools/native-capture/build_verified.py`：指定新 `--build-dir`、原有 `--define NAME=VALUE` 和 `--target`，保留候选功能组合。工具实际探测编译器showIncludes前缀，校验生成规则与编译器身份，并在构建后拒绝 0依赖或缺少指定 `.inl` 依赖的结果；不复用有旧解析缓存的目录。5项离线检查及协调端实际小工程运行通过，另有修改私有header后重编译的回执。它只证明构建依赖被正确解析，不替代游戏/GPU验收。全量原生构建的 446项依赖证据见下文。

完整私有构建随后完成于 `build-game-resolution-ngx-optimal`,DLLSHA(英语:256 `ce9ef2f8a954c4740105f086392917bc988b017f9764838d8a9c100c34f458ea`，未替换默认清单。期间发现旧Ninja构建的header依赖为 0：当前中文MSVC输出UTF-8 showIncludes前缀，被CMake误解码并写入规则；VSLANG=1033不能修复仅有 2052资源的安装。使用实测前缀的project-include override后fresh完整构建通过，capture.cpp依赖 446项，明确包含本次修改的host/native_api/control_capture。不能将先前“ninja no work”当新源码编译证据。原始构建日志在ignored `build-game-ngx-optimal-prefix-fixed.log`；实际每眼GetOptimal/GPU画面仍未验证。

后续源码已接入每眼 GetOptimal 查询与双feature创建前的参数检查：按每眼输出尺寸和原quality查询，校验输入落入实际返回范围，并在查询前后重新验证world/revision/参数对象/handle/原Create字段。未知或漂移时拒绝创建，不将整幅最小宽度硬除以二。完整helper字节认证也已加入。共享源码重新编译的 NativeBridge/HostLive 两个测试程序直接运行通过；最初CTest名称过滤未选中测试，不计作通过。查询回执仍由测试替身提供，真实每眼GetOptimal尚未运行，完整capture构建及GPU验证未在本轮完成。该接口未激活，真实producer域、Copy状态、GPU fence退休及所有共享parameter调用串行性仍为接线前提。

`dlss-native-menu11` 使用只读界值观察候选 DLL `1c0e6c21e5b1c71c24fb190ad19ab4b76d109860fe36f51c9081d98e4428c1db`，session `8935e971` / 密码 23756。Create 后与两次接受的 Eval 入口均记录相同参数身份，界值双读稳定：maxW/maxH=3840/2160，minW/minH=1920/1080; 爱瓦尔票 1483、revision 1/world 1，两眼实际输入仍为 1920×1440。结合已保存且按完整chunk认证的 `be65cc` 路径（读取view宽，再按 `1ad1960` 族界值夹取），确认 1280的每眼构造宽被整幅配置的 1920最小宽度抬高。不能据此将所有DLSS问题归结于此，也不能仅改Eval宽度绕过有效范围。

下一步应让每眼输出尺寸、GetOptimal结果、独立feature与输入域一致，再处理真实生产资源和输出生命周期；不修改发布默认以迁就旧整幅路径。此轮没有观察GetOptimal回调，不能宣称其调用已验证。运行时推荐尺寸×倍率仍是 0.0.1正式默认，优先于DLSS。

协调端独立确认 session stopped、native_cleanup_verified/entry_restore_verified=true，游戏已退出，5个受保护文件逐哈希恢复。原始状态 `analysis/runtime/dlss-native-menu11-native-status.txt` SHA 256 `5523c7c6c383a575d5f80930aa91b21f7cc6d876262f480d93d9d49c057b5d7d`。候选 3项CTest及 22项启动验证通过；这些结果不等于正确双眼GPU图像或DLSS可玩验收。

## 2026-10-19：输入矩形实验实跑，Eval 宽度仍不一致

当前发布开发基线已切为推荐每眼尺寸×倍率（见主进度文档），优先级高于DLSS。本节为私有game-resolution诊断，不替换默认模式；以下Native 70等默认描述均属历史。

`dlss-native-menu10` 在正常游戏菜单启用DLSS Quality并返回关卡，实际加载输入构造实验DLL SHA 256 `7ce3dd337fc9fe91604bbb523d4b1cd0a5c080431d47b6737130edd7acd40edc`。session `5672a25a`，PID 46600，完整出生身份在ignored运行回执。菜单helper成功、观察期结束，native清理、入口恢复、游戏正常退出以及 5文件恢复均通过，协调端再次核验无游戏进程和全部原文件哈希。

冻结epoch revision 1/world 1/ticket 1492：candidate_complete=1、drained=1、dropped=0、evals=2。Create参数为 2560×1440→3840×2160。构造两眼source/target为 1280×1440，x分别 0/1920；prepare前后及Eval时对应view矩形保持不变。但两次NGX Eval实际输入值仍是 **1920×1440**，而不是预期 1280×1440。两次typed_before/final均f，dispatch_match=1、result=1。这证明只修构造输入矩形未同步Eval宽度；需继续追踪调用前宽度来源，不可把局部构造结果算作DLSS输入域整体通过。

原始状态 `analysis/runtime/dlss-native-menu10-native-status.txt` SHA 256 `1bd23d54c4a0e91250e5b056dfb7a74419fecea9aabd1ef3ee6a802fa7aca588`，关键行 188–202、244–252。gpu_completion_verified=0、producer_domain_verified=0；共享历史、输出回写及正确双眼图像仍未证明。本轮不支持“DLSS已可玩”的结论，下一步仅按这个明确宽度差异查保存反编译／代码。

2026-10-18。用户已把 DLSS 超分辨率列为 V 1 必做项。当前玩家默认仍为 Native 70，DLSS/DLAA 保护仍开启；本记录中的独立候选没有提升默认，也未加入发布包。DLAA、帧生成不在新增范围。

## 已实现的实验入口

`b701095` 增加独立清单字段 `experimental_preserve_upscaler=true`，经 Launcher、reservation 和 debug command 显式传到启动脚本。它只读检查唯一 `DLSS=True`、FSR 2/FSR 3/MFSR 关闭和唯一质量值 3；质量 4及其他值拒绝。质量 3的 Ultra Performance 名称来自此前相同磁盘版本的配置证据，不推广成完整质量枚举。实验标志与启动或菜单图形锁并用时提前拒绝。默认缺省行为仍强制 OFF。相关 92项测试及 PowerShell 语法检查通过。

此入口只保留测试配置，不能证明 NGX 执行、超分辨率生效、每眼历史隔离或图像正确；运行中也不是排除所有 DLAA 切换路径的产品策略。

Native 73 从冻结 Native 70 原生源独立构建，仅关闭 graphics runtime/startup/menu 三个保护宏，保留只读 graphics observer。DLL SHA 256 为 `ad3c543ea9fc40e084d3aa557ae61e572b104aa50967b3c915ebb19dfecf7232`。

## 实际游戏结果：场景接受，输出未发布

- `dlss73-gameplay-20261018` 首轮在 UI 初始化校验处拒绝，native 未激活、没有场景提交；native cleanup 未验证，随后游戏正常退出。不能据此认定 DLSS GPU 故障或正常清理通过。
- `dlss73-scene2-20261018` 暂停 HUD 修正和 world recovery，仅隔离普通场景。原生 normal/alternate DLSS 字段均为 1；左右场景分别接受 686次，最终 GPU 发布与 XR scene copy 均为 0。
- `dlss73-fullui3-20261018` 恢复全部 UI 功能后 native 激活成功，仍然 0发布、0 scene copy。因此首轮 UI 拒绝未稳定复现，不能将它认定为 DLSS 输出失败的统一根因。

后两轮 native cleanup 验证通过。三轮均正常关闭自有游戏，完整设置与四个受保护 profile 按测试前内容恢复。均未观察到自有游戏进入实际前台；没有移动或调整窗口、启用实体手柄后台响应或修改全局 OpenXR Runtime。测试使用 quiet 2 子进程 Runtime 和 tiny-output observe-only。

原始数据及完整身份留在 ignored `analysis/runtime/`。设置布尔、场景调用和提交计数不代替 GPU 图像或真头显验收。请求的 GPU 快照没有生成场景图片。

## 已缩小的代码位置

新路径实际 quad return RVA `d83cbf`，此前已认证的路径为 `d83b1c`、`d808d8` 等。Native 73 记录左目标x约为 2³²、每眼目标及source宽 2534、右source从 1280开始；不是两个合法互不重叠半宽矩形。

已有历史代码节 SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、基址 `0x140000000` 的完整缓存汇编显示：该分支将 32位目标原点零扩展后转成浮点，并复用生成的目标宽高作为source宽高。它可以解释负原点变成巨大正数的机制，但日志中的舍入浮点不能反推出精确原整数。需要记录实际生成路径及shader常量后才能确定修正。

现有 final-eye 认证未覆盖该分支；final transfer 无有效源票据，`pair_rejection=16` 对应 ticket_conflict（票据为 0也会产生），不是GPU资源地址冲突的证明。正常化在 entry 预检拒绝，符合现有门禁。不能仅加入caller白名单或单独改符号。

`ui-layout-details5-20261018` 在另一正常游戏进程中对 `d82f58` 主chunk 3192B、continuation 489B、setter `1760b4` 205B进行了完整当前字节双读及SHA核对，全部匹配上述历史chunks；这不是任意后续进程的自动认证。主函数R 8是传入view，是否等于TLS current-view仍需运行观察。相关shader setter与DOF保护使用的setter不同，不能复制DOF标量归零策略。

## 有界只读观察器

`a0b90ec` 增加默认关闭的 `GEARS_DLSS_ROUTE_OBSERVER`。独立构建在启用前重新认证上述三个完整chunks，观察输出函数与常量setter，最多保存 8条Output、每条最多 2个setter和quad。记录传入view、TLS view、原始矩形、采样向量、常量提交与消费bank；不修改参数、资源或任何票据认证。

实际wrapper离线测试覆盖原调用一次、返回值、嵌套/异常恢复、额度耗尽、payload保存及缺读；跨线程try_lock验证原函数执行期间不持观察器全局锁。清理超时且仍有在途owner时禁止读取未完成Row，仅输出有界摘要。测试通过不代表实际游戏或DLSS图像通过。

Native 74 独立候选已完成编译与wrapper检查，DLL SHA 256 `72941b77148f1647de77389365d586cf80c63d6692f005d82be4dd2a9cae070b`。实际观察结果见下；首发 DLSS 支持仍未完成。

随后实际会话 `continuous-stereo-39168-20261018-023738-83b69260` 完成独立观察：三个完整当前chunks认证通过，8条记录覆盖 4张ticket的双眼，传入view始终等于TLS current-view和对应renderer eye。每个Output均恰好一次被观察器筛选的目标setter `d83b69` 及quad；这不是整个Output只调用一次setter，正常先行的 `d8388e`、`d8397d` 不在该计数内。进入/退出view字段一致，没有嵌套或溢出。view的两组矩形均为合法半宽，而 `d83cbf` 收到的仍是上述巨大原点与全窗口范围。

setter `d83b69` 的当前描述符均为 `{65535,65535,0}`、index 0、返回 0。已核对实现先比较byte_count与index×16，当前是无操作，不访问bank；因此不能将缺少bank记录当故障，也不需要伪造shader常量消费证明。这一轮只读观察仍为 0发布、0 scene copy、没有保存到场景GPU图片。native cleanup、正常游戏退出和完整设置/profile恢复通过，未记录自有游戏进入前台。

下一隔离实验仅复制直接分支的矩形语义，同时建立每次scope/draw完成及publication门禁；不能靠宏白名单放行。即使恢复双眼输出，也仍需单独证明实际NGX输入缩小、时域历史与SR效果。

私有 `GEARS_DLSS_OUTPUT_CONTROL` 候选已实现，默认关闭，独立于 8条观察额度保存逐票据证明。审查修正了两处问题：仅统计目标setter，正常先行调用另计；实际SRV查询前后核对context当前命令列表的raw/canonical身份及generation，避免把不同列表的资源拼成凭据。实际wrapper的正常路径、先行setter、列表不匹配/漂移、重复/缺失/异常和旧caller拒绝测试通过。完整DLL构建、实际游戏和GPU输出仍待验证，不代表DLSS支持完成。

Native 75 已完整编译，三项wrapper/policy测试通过；启动前 `GearsCaptureFrameLinkCheck` 在GPU接收fixture返回 179，尚未启动游戏。该旧fixture不执行Consume，新publication门禁因而没有对应完成记录，正确拒绝；已在专用fixture增加Missing/Incomplete必须拒绝及显式完成NoSite后才能通过的断言。未放宽游戏门禁。设置与四个profile恢复通过，下一独立构建验证该修正。

Native 76（SHA 256 `96c96f0deb058b9e08a0762e8d3c6b7d51225df6ce084e647a605604cb7f3e0b`）的GPU启动自测通过，随后实际会话 `continuous-stereo-916-20261018-025942-2e064f2f` 正常激活，但forwarded/draw/source-query/publication仍为 0。native cleanup通过，独立entry恢复验证失败，游戏正常关闭，设置和四个profile恢复，无自有前台采样。审查确认恢复验证器缺少新Output函数分支，已补完整主chunk和continuation固定SHA比较及七项漂移拒绝测试；不能把尚未重跑的验证器修正写成该会话恢复已通过。

另外，完整缓存调用链证明Output可在最终quad前经 `194950` 调用 `194c93` 的 1024×32预处理quad。控制曾在caller筛选前计入所有quad，现改为仅计 `d83cbf`，其他调用单列诊断；实际wrapper补了该先行调用用例。旧日志未逐次记录该预处理执行，尚不能宣布它是本轮唯一根因。

Native 76实际绑定高水位为 10；缓存 `1b4ca0` 将context+65d设为 `max(old,slot+1)`，因此它不表示shader采样 10张纹理。主输入绑定候选调用位于 `d831c3`（return `d831c8`），descriptor决定写入context绑定数组的槽。下一步观察实际descriptor、写入对象及draw前同槽是否被覆盖，不通过槽 0或唯一尺寸猜源。

针对NGX的有界历史静态分析进一步定位Create/Evaluate共用handle槽RVA `1c20bd0` 和参数对象槽 `1c20bd8`；所读游戏侧完整函数没有view/eye索引。Create尺寸来自第六参数结构的 0/4/8/c，Evaluate渲染尺寸来自入口R 9结构+30/+34，Reset为+38。这是历史代码的数据流证据，尚未认证当前这些函数或观察两眼handle值，也不能据此断言DLL内部历史结构。恢复矩形输出之后仍要验证实际尺寸、双眼handle及历史状态，首发DLSS支持继续未完成。

Native 78 在关闭UI试验的组合中暴露了头文件被包含在匿名namespace内的编译问题，未构建成功或启动游戏。纯helper头现放到全局include区。Native 79（SHA 256 `411770d1c3112eb66058a565cf8fe03a282e1ab85b271288c858446cd50ec281`）完整构建、输入wrapper和GPU frame-link自测通过。

Native 79实际会话 `continuous-stereo-9388-20261018-032303-3e4e9656` 得到 541次局部矩形转发；此前预处理quad误计已被消除。541次source查询均被旧单槽/格式 24假设拒绝，实际draw证明和publication仍为 0。完整命令列表关联通过，native cleanup、所有新旧entry独立恢复、正常关闭及设置/profile恢复通过，无自有前台采样。

新增默认关闭的输入绑定观察器完成 `1665dc` 全部 503B的独立当前认证；8条记录覆盖 4张ticket的两眼。`d831c8` 的descriptor均 `{0,1,0}`，由真实descriptor选定槽，而非按尺寸搜索。绑定后、quad及actual draw时，选中槽对象始终等于公共资源链解析对象，真实canonical资源为 2560×1080、format 26、单mip/sample/layer；descriptor/context未改变，draw命令列表身份及generation匹配。绑定前槽内 640×270、format 10资源是之前的状态，不当作NGX输入尺寸或SR生效证据。

下一私有修正建立独立于 8条诊断的BindScope完成凭证，重新核对上述关系，并只扩展已观察的源format 26；目标format 24和最终传输契约保持。Astra对该CPU绑定凭证复核通过，可用于像素待验的矩形修正实验；它不证明shader采样用途、NGX降分辨率或独立history。

后续默认关闭的控制已接入独立BindScope；它不借用 8条观察记录作为许可。启用时另外完整认证 1665dc的 503B；绑定必须由本次descriptor选择槽，正常完成原始五参数调用，公共对象链与真实format 26资源一致，并在quad、实际draw前后及Output完成时复核。缺失、重复（包括draw之后重复）、异常、对象/描述符/列表generation漂移和双眼来源不一致均拒绝publication。目标format 24及final transfer未改。实际wrapper三组`/W4 /WX`测试与 10项独立恢复验证测试通过；完整DLL、实际GPU及NGX时域验证仍待执行。

Native 81从`e758929`冻结，DLL SHA 256 `8478dac38c9f96519418ba189f43d0af2a198df773c69a001abef63ea54af2cd`。完整构建、三组wrapper、final-transfer policy及实际GPU frame-link ABI自测通过。实际会话`continuous-stereo-45048-20261018-033857-a8e9d2e7`的完整当前代码认证通过，549次矩形转发和 549次actual draw证明，3843次绑定复核，source/list拒绝均 0；Output完成仍 0，没有GPU图片或XR场景发布。XR以 1退出，会话为incomplete；native cleanup、所有entry恢复、正常关闭和完整设置/四profile恢复通过，无自有前台采样。这是绘制凭证的进展，不是DLSS输出修复完成。

有界KB查询及原已缓存完整continuation显示：`d83d53`读取view的family并检查其`+4c` marker，非零时`d83d5b`将`view+3e8`写为 1，然后cleanup/正常返回 1。Bind/quad/draw样本selector均 0，而`binding_current`在Output退出按当前selector重新选择公共bank并要求与原值完全一致。此机制可以导致拒绝，但本轮没有退出处样本，不能认定它是实际唯一失败条件。下一轮只增加有界首次失败记录，分别保存完整性谓词及退出绑定状态，维持现有拒绝；证实正常转换后才能分离draw源凭证与Output生命周期契约。

另增默认关闭的`GEARS_NGX_ROUTE_OBSERVER`，只在完整认证当前Create/Evaluate包装器 249/1377B后安装两处只读hook，固定最多 4条Create、8条Evaluate。记录原始六/四GP参数、64位RAX、前后公共handle/参数对象槽、输入输出尺寸、Reset、raw资源指针及可认证的world/ticket/eye关联；不解释SDK返回语义、不强转未知资源接口、不参与publication。实际wrapper、默认OFF及 13项独立恢复验证测试通过；尚无本观察器实际游戏数据。入口/出口槽值不是中途实际消费handle的证明，历史隔离、GPU与SR标志保持未验证。

Output退出首次失败记录已实现：逐项调用原stable/descriptor/binding谓词并保留原AND条件，保存前后选择状态、字段mask、原槽/实际对象和列表generation；只发布第一条完整记录，cleanup以release/acquire读取，最多四行。正常、仅selector切换、槽变更、缺读、generation漂移及禁止后续覆盖用例通过。记录只用于定位，不授权publication。

Native 82（`c7faf64`，SHA 256 `a991b62b941a97f17e222c6895f719a826e761fa31b693d54a80bccc2dd11b8e`）完整构建、五组wrapper/policy及实际GPU frame-link ABI自测通过。实际`continuous-stereo-41880-20261018-034815-8ed1daee`得 567次绘制证明，0 Output完成和 0图片/XR场景发布，XR退出 1/incomplete。首次失败receipt表明：所有其他退出谓词通过，input mask 1023，selector确为 0→1；原绑定槽对象、真实format 26资源及list raw/canonical/generation 596均保持。当前publicglobal转为新bank后对象不匹配，恰是binding_current拒绝点。该首条记录只覆盖ticket 1左Output，后续修正仍须实际验证双眼及后续帧。

同轮当前NGX完整认证通过，记录 0 Create和 8 Evaluate，后续 1132次只正常旁路不写行。未观察到Create只说明激活之后没有记录。row 1..6覆盖ticket 1..3的三对调用：提交结构render 853×360，source base分别 0/1280，output raw相同且output base均 0；公共handle槽前后相同。只有row 2 Reset=1，其余相关行 0。两个不同source base调用的执行TLS都指向eye 1；它不能用于识别逻辑左/右眼，日志后续改为tls_associated/tls_eye并显式logical_eye_verified=0。相同输出区域可能被覆盖，也可能是暂存后搬运；仍需实际写入/复制与最终像素证据。公共槽快照不证明下游消费handle或独立时域历史。

该轮native cleanup、所有entry独立恢复、正常关闭和设置/四profile恢复通过，无自有前台采样。下一私有修正保留绘制全程严格源绑定，只在Output正常尾部接受已认证marker 1的 0→1转换，并从原已证明publicglobal 0重新核对完整源链；不借新bank替换源或放宽其他身份检查。Astra复核认可进入GPU像素实验，DLSS支持仍未完成。

Native 83从`8e2dd6b`冻结（SHA 256 `e61542a0e963cf020bcde2ca5b1fb0af2cb1937517dc63d42c3eb1155062918a`），完整构建、五组wrapper/policy与实际GPU frame-link ABI通过。实际`continuous-stereo-43064-20261018-040411-30561c20`完成 1108次转发/实际draw/Output；390次完整scope publication，XR scene copy 384次、projection layer 1411次。允许样本的左右眼tail均单独通过并保留原global 0，source/list/pub拒绝为 0。3张GPU图像两半均非全黑，已保存原始图和统计；末图左右非黑占比约 83.9%/90.2%，平均luma约 10.44/16.14。样图近景占比大且两眼亮度不同，需要固定相机的DLSS开关对照，不能只凭非黑宣称像素、SR或history正确。

该轮会话stopped、XR退出 0，native cleanup、全部entry独立恢复、正常关闭及完整设置/四profile恢复通过，无自有前台采样。默认Native 70及现有开发ZIP未改；本结果只表明该隔离SR配置下已恢复实际双眼输出，不是全部质量模式、CG/机枪、长时间或头显兼容验收。

随后同一Native 83进行DLSS关闭对照`continuous-stereo-9000-20261018-040759-2a575dde`：两轮测试前完整设置与四profile的SHA逐项相同，无额外游戏输入，XR CSV的头部位置/旋转及左右眼偏移也一致；未单独保存同步游戏相机POV。OFF图像显示正常人物和全景，ON图像明显只呈现放大的近处墙柱/地面，不能作为正确立体画面验收。OFF有 634次scene copy、1416次projection，正常退出/全部清理与设置恢复通过。此对照不是同步姿态/固定GPU负载性能基准。

按Native 83实际QPC排序，ticket 1..3均为两次NGX Evaluate返回后才进入左/右Output及quad，不能将d 83cbf认定在上采样调用之前。quad绑定的真实资源等于NGX Colorraw，Outputraw则不同；这仍不能排除中间复制或原位回写。提交的 853×360与view的 1280×1080不是同一语义。下一实验应观察一个ticket内的实际资源写入/复制位置及quad源像素/真实SRV，先核实有效区域和输出去向，不继续凭最终矩形猜UV。CPU排序报告和原始图片仅在ignored analysis/runtime。

进一步核对同轮已有原生复制日志：`caller=1c6093` 共 2770次完整子资源复制，实际source正是NGX Outputraw，destination正是quad绑定的Colorraw，box为空、目标偏移为 0。它支持Output整图回写到最终源的路径，因此不能把“quad绑定Color”单独认定为绑错输入。现有聚合记录没有逐次Evaluate关联与GPU像素；下一私有观察器拟在已存在的真实复制源旁最多读取两个GPU样本，核实实际有效区域与最终画面的关系，不新增资源状态转换或额外Execute。未取得样本之前，DLSS取景异常、逻辑眼/history/SR验证仍未关闭。

Native 85从`768813a`冻结（DLL SHA 256 `e44db519075ba88922c81d64dd33ab3d92d47814dc91e840711db415e15ece9b`），新增默认关闭、最多两份的相邻CopySource读回；六项实际wrapper/policy检查及实际GPU frame-link ABI自测通过。独立审查发现并修正跨线程Eval/Create可绕过TLS未完成区间的歧义门禁，实际不满足关联时不追加采样。`dlss85-output-pixels-20261018`成功采到同一world 1/ticket 32的Evaluate 65/66：source base分别 0/1280，Render.Subrect均 853×360、output base均 0；每份读回都位于对应Eval正常返回之后、下一Eval之前的原生Output→Color完整复制旁，使用独立列表generation、同一DIRECT queue，无新增barrier或Execute。

两份 2560×1080 format 26原始数据均在成功物理Reset和fence完成后保存，failure=0、持有引用随后释放。GPU预览在此Output复制点就呈现放大的近墙柱画面，因此最终SBS矩形不能作为唯一原因；尚未区分输入已异常与NGX参数/feature行为。原始GPU数据与解码结果已校验哈希并归档ignored `analysis/runtime/dlss85-output-pixels/`。XR有 437次scene copy、1405次projection，但当前Borderless 1二进制没有输出hdr_match记录，发布匹配请求不等于取得同ticket最终XR图，不能宣称像素闭环或真机通过。正常关闭、native/entry独立恢复、设置/四profile恢复通过，无自有前台采样，默认包未改。

下一实验先补激活前遗漏的真实Create输入/输出尺寸、quality/flags/subrects及后续handle关联；853×360与整图宽高比相近只是假设，不直接改尺寸或UV。原生复制中的Color是COPY_DEST，不能把它当输入读回的COPY_SOURCE；预Eval输入像素还缺真实状态和执行顺序证据。DLSS SR、逻辑眼及独立history仍未通过。

正常菜单重建试验`dlss85-menu-create-d-20261018`从DLSS关闭状态启动，保留Native 85现有观察器，通过实际UI状态导航至Resolution Scaling并发送一次原生窗口Right。前后CPU DLSS/alternate DWORD仍均为 0，Create/Evaluate记录均 0；因此该轮没有证明启用，也没有取得创建尺寸。返回时实际出现`WarfareConfirmation / UIScene_Confirmation`，正文为`ConfirmVideoSettings`，自动路由因未知确认页停止。后续将只对该精确本地视频确认页补有界处理，并记录反射选项值；不把一次Right、历史脚本的Enter分支或CPU标志当SDK实际生效证明。该轮正常关闭、native/entry独立恢复及设置/四profile恢复通过，无实际自有前台采样；完整状态和哈希已归档ignored analysis/runtime。

后续E/G/H补充只读反射和精确视频确认：SelectedResolutionScaling的 3/ULTRA PERFORMANCE到 0/QUALITY是质量档变化，并不是启用开关。当前AA数组实际为NONE、FXAA、AMD FidelityFX Super Resolution 3.1. 导航系统 3.5；H在AA行两次普通Right后观察到 0→2→0，路由因重复停止，仍无Create/Evaluate。这与按键重复或原生跳过选项均相容，尚未区分；编译与对应实际状态均证实三项graphics锁OFF、lock_effective=0，不能直接归咎现有禁用hook。下一实验只记录一次原按键对期间的控件CurrentIndex/SkippedItemIndex及选项索引轨迹，不盲目循环或声明DLSS已启用。各轮均正常关闭并恢复native/entry、设置/四profile，无实际自有前台采样。Create尺寸、输入图像与正确SR输出仍是未关闭项。

I轮只发送一次原 80ms按键对，实际反射轨迹为：按下后约 13.60ms，CurrentIndex和SelectedAntialiasingSetting从 0到 1；释放后约 23.68ms再从 1到 2，SkippedItemIndex一直为 0。因此FXAA并未被直接跳过；第二次推进发生在释放之后，但尚不能单凭时间顺序区分释放处理与排队事件。该轮仍未启用DLSS、Create/Evaluate均 0，正常关闭和native/entry、设置/四profile恢复通过，无自有前台采样。精简轨迹为ignored `analysis/runtime/dlss85-menu-create-i-aa-transitions-20261018.json`。下一轮只向同一精确AA控件发送一次原WM_KEYUP，保留身份、反射字段和稳定菜单认证；不改变普通导航或盲目循环选项。

J轮实际发送且仅发送一次WM_KEYUP，未发送WM_KEYDOWN；CurrentIndex和SelectedAntialiasingSetting从 0到 1，并在随后 200ms采样中保持 1，SkippedItemIndex保持 0。这证明本控件会响应单独的释放事件，结合I轮可解释普通按键对推进两格；不据此改变所有菜单输入。下一步仅对已认证的Graphics AA/SR选项引入显式单释放步进，以实际当前数组核对QUALITY和NVIDIA DLSS 3.5，再观察Create/Evaluate；本轮仍为FXAA且Create/Evaluate均 0，不能算DLSS支持通过。正常关闭、native/entry独立恢复、设置/四profile恢复通过，无自有前台采样；完整helper哈希与状态已归档，精简轨迹为ignored `analysis/runtime/dlss85-menu-create-j-aa-transitions-20261018.json`。

K轮完成上述正常菜单路径：Resolution Scaling逐次 3→4→0（QUALITY），AA逐次 0→1→2→3(NVIDIA 软件) 3.5），每步均核对控件/资源当前索引及实际数组。菜单输入只向精确自有HWND发送，普通导航未改变，实体后台手柄保持original。Native 85实际记录一次Create和八次Evaluate，当前完整函数认证通过：ticket 3488创建字段为 1706×720→2560×1080、quality_raw 2、flags_raw 67、output_subrects_byte_raw 0；随后ticket 3488–3491的四对Evaluate均为 1280×1080，Color/Depth/MV base交替 0/1280、Output base均 0，共用公共handle/parameters和资源；仅第一行Reset为 1。该数据证明创建与每view区域的尺寸域不同，不证明SDK接受范围、实际消费handle或异常根因，不能把旧startup-ON/Ultra的 853×360当同一轮变化。

两份原始Output读回均为创建当帧ticket 3488的Eval 1/2，GPU完成、物理Reset后释放及原始复制点契约通过，failure 0；原始数据、解码和哈希保存在ignored `dlss85-menu-create-k-20261018-artifacts/`。根协调检查了两张完整Output及后续XR预览，房间与人物可辨，但当前XR图不与同ticket对应，既不能据此宣布完整像素正确，也不能把旧轮近墙取景直接套用本轮。现有绝对ticket>=32门禁在创建 3488前已经满足，故本轮只读到了最初一对，未覆盖菜单恢复后的晚期输出。下一步先取得同handle的晚期pair、view矩形/缩放来源与对应最终图，区分尺寸域、取样区域和转换过程，暂不猜改尺寸或UV。正常关闭、native/entry独立恢复和设置/四profile恢复通过，无自有前台采样；默认保护与玩家包未改变，DLSS SR仍未验收。

L轮保持Native 85不变，换用只启用现有匹配快照代码的私有XR match 1（提交 3b 7396b，SHA 256 `30d4d978a9613dd0bca665fd37560baf682c2c0e7c0f11e3b1fff688fe760ca5`）。完整来源与四项离线检查在ignored `xr-match1-build-receipt.json`；新旧Packet/Publisher前缀相同，不能把旧Borderless 1缺少接收功能解释为Native没有出版请求。实际接收器选中了world 1/ticket 3535/copy_seq 2，但这是画质菜单内创建当帧；恢复场景时实际新图已到ticket 4504，明确记录`hdr_match_missed/ticket_passed`。因此本轮仍无对应最终图，验证了接收链和采样时机缺口。正常关闭、native/entry、设置/四profile恢复通过，无自有前台采样，默认XR未替换。

`5e66fb8`新增默认关闭的`GEARS_NGX_OUTPUT_DELAYED_CAPTURE`。它独立于四条Create/八条Eval观察额度，以完整前后Create括号建立epoch；worker仅接受当前进程/会话/world/epoch的一次性Resume请求。等待 32个完整稳定pair后才选择下一pair，jitter可正常变化，其他签名漂移或不完整段重计且不延长 30秒期限。已选中段失败只quarantine，保留原GPU退休契约；在`frame_link.present`后、同ticket最终capture前才按精确两次Eval闭合并出版。没有新增原生hook或修改尺寸/UV。独立审查发现的入口缺读/参数变化缺口已修复并补用例；原函数保持恰好一次。

Native 92从该提交冻结，完整DLL构建及十项相关检查（含实际DLL frame-link自测）通过，SHA 256 `1869b93ded9a5e5e60f42287283679efd563e9e28fa88fd5e1536e712a563ec2`。这是采样控制候选，不是DLSS兼容修复。M驱动在正常Resume后核对projection恢复及新入场图的保存记录，再写当前身份绑定的私有arm文件；姿态CSV更新、请求已发或Native出版均不冒充最终匹配。默认Native 70、Borderless 1和玩家包未改变。

实际`dlss92-menu-pair-m-20261018`正常完成Quality/DLSS菜单路径并接受一次Resume请求；选中world 1/ticket 5070后，最终状态为selected 1、pair_closed 0、gpu_saved 0、terminal_reason 13（selected_invalid），resets 13。未产生该进程的原始Output文件，也未取得对应最终图，不能算晚期像素闭环。当前日志保留两条正常selected Eval/copy与该ticket的Present拒绝记录；通用selected_invalid会覆盖最初原因，尚不能仅凭最终码唯一断言最早失效边界，更不能视为DLSS取景问题根因。

本轮正常关闭游戏、native/entry独立恢复、设置/四profile恢复与自有arm文件删除均通过，XR退出 0、无自有前台采样。精简proof和完整状态及已核对源码已归档ignored `dlss92-menu-pair-m-20261018-proof.json`、`dlss92-menu-pair-m-20261018-artifacts/`；构建receipt已如实标记game_tested=true、matched_pair_capture_passed=false。后续先定位选中段的首个失效边界，再决定最小修正或下一采样，不放宽成对关联、GPU完成或图像验收条件。

## 2026-10-18：晚期同帧闭环及输出反馈证据

`dd7cbe1`保留首个选中段失效的原始原因、具体入口、QPC、pair状态、incoming owner和实际Present结果，不改变选择、拒绝或GPU退休规则；所有新增状态键使用`invalid_`前缀。Native 93从该提交冻结，DLL SHA 256 `fd921e95bad0d6faed6faf4460a6d9cb1c554ec0fc358a6a0793880f931caee7`，完整构建及十项相关检查通过。该版本仍是诊断候选，默认玩家保护未变。

实际O轮正常菜单启用Quality后恢复游戏，一次Resume请求、32个完整稳定pair、零reset，选中world 1/ticket 4844。两份原始Output均正常复制/执行/Signal，成功物理Reset及fence完成后保存并释放，failure 0；最后pair_closed 1/gpu_saved 1。私有XR match 1实际保存uncached snapshot 2，actual/selected ticket均 4844、world均 1，source_serial 3849。此轮首次取得晚期两份Output与最终双眼图的精确同帧闭环，不只是发布请求或后续邻帧图。正常关闭、native/entry独立恢复、设置/四profile及自有arm清理均通过，无自有前台采样；原始数据仅保留在ignored `dlss93-menu-pair-o-20261018-artifacts/`。

根协调查看完整三图后，使用显式区域比较进一步核对：Output 2与Output 1右半横向放大后的亮度相关 0.99908、XY梯度相关 0.99327；整幅Output 1及其左半的梯度相关仅 0.04306/0.03952。最终左眼与Output 2左半的梯度相关 0.95035，最终右眼与Output 2右半为 0.94066，其余候选最高 0.08041/0.05275，最佳平移均零。比较使用同一HDR预览曲线、640×270网格与小范围平移，不把曝光/后处理差异当作像素字节相同；完整方法和哈希在ignored `dlss93-o-region-comparison.py/.json`。

这些像素证据表明本轮最终双眼取的是第二份输出的不同半区，第二份输出又高度对应第一份的右半放大，不能作为两份正确独立眼图验收。结合原生完整Output→Color回写和第二次Eval的Color base 1280，强烈支持首眼整图回写破坏后续眼输入的反馈假设；但仍需核对实际Evaluate命令列表/代次及原生调用合同，或做一次严格隔离实验，才能把外观对应提升为确证的GPU依赖。不能据此只改最终UV或解除默认保护；输入/输出尺寸域、每眼历史、正确SR、场景回归和真机验收仍开放。

## 其他主线进展

`50cbfcd` 为 MenuReader 增加可选的 Position/DockTargets 原始布局采集，70项离线测试通过。普通采样不增加这些读取，未改变UI位置。实际普通双目回归 `ui-layout-details4-20261018` 提交 1359次、scene copy 723次，约 15.08秒，最大提交间隔 28.7181ms；正常清理和配置恢复通过。岔路详细采集初次遇到导航碰撞、脚本结果字段及注册表变化；后续 `ui-layout-details7-20261018` 已取得完整 13节点及布局详情，见[文档155](155-path-choice-position-and-docking.md)。这仍不是UI修复验收。

机枪方面，实际函数发现已把 `PlayerUpdateCameraNative` 的exec映射到 `3b3af4`，其拆包后直接调用已有 `110878`；炮塔 `GetBaseViewOffsets` 的exec为 `1248238`，实际实现经模式对象虚表调用。发现不等于全部当前函数字节或实际挂载验收。现有准星配对证据是 AssaultRifle，不是手枪，不能用于宣布用户报告的手枪外圈问题已修复。
### Native 95/P：一次首眼回写省略的实际因果对照

从提交`5dd8a82`冻结Native 95,DLLSHA(英语:256为`385fb4b323cf5e97976bc076edf8055145da047bff2ce859c358ba5144041b57`。11项相关检查通过。该私有默认关闭实验只在完整认证的稳定pair中省略第一次原始`Output→Color`整图复制，仍通过未过滤的原函数追加Output读回；第二次原始复制、Create/Evaluate参数、视图、UV及资源状态转换均保持原样。不是玩家修复开关。

实际P轮完成正常菜单Quality/DLSS选择及Resume，选中world 1/ticket 5349。首眼omission_count=1、original_forwarded=0、readback_started/returned=1；两份 2560×1080、format 26原始Output均在fence完成和物理Reset退休后保存。最终XR源图通过actual/selected world与ticket精确匹配，snapshot_index 2、cached_image=false；不以请求成功或序号相等代替实际匹配。原始数据、独立清理与哈希证据归档于ignored `analysis/runtime/dlss95-menu-pair-p-20261018-proof.json`及同名前缀artifacts目录；归档仅复制，未删除构建目录原件。

复用O轮同一NumPy/PIL区域比较方法，Output 2与本pair的Output 1右半放大图的最佳梯度相关从O轮 0.993269降至P轮 0.039281。协调者查看完整读回与同帧最终图，确认P轮Output 2不再只是Output 1右半放大。此单次干预支持首眼整图回写对立即输入反馈有因果参与；没有直接取得Evaluate 2之前的Color输入像素，也没有证明vendor内部history。

最终左、右面板仍分别与Output 2的左右半图高度对应（梯度相关 0.944376/0.909643），因此省略首眼复制并没有实现正确双眼合成，不能作为DLSS SR修复发布。下一步须保留各眼独立输出并正确缩放/组合，继续核对Quality的创建/渲染尺寸和时域history。对照仅一pair，不是性能、所有画质或真头显验收。

P轮诊断完成，正常关闭、native及所有entry独立恢复、设置及四profile恢复、arm精确清理均通过，XR退出 0，无自有前台采样。默认Native 70/Borderless 1及Feedback 3开发ZIP未变。

### 分眼布局与feature寿命策略已写入源码，尚未接入SDK

提交`88a6875`增加纯CPU的分眼布局/单box复制计划及两个feature的创建、逐眼使用、失效和GPU退休策略。输入尺寸必须由已验证生产端明确提供，不从Quality枚举猜比例；每眼输出、物理纹理和padded输入槽保持不同语义。完整输出复制的两个正常边界将分别只复制有效眼图到对应槽，仍使用原Color资源与最终shader。策略本身不调用SDK、不修改视图、不提交GPU命令，不能算DLSS支持已经实现。

协调审查以P轮实际sidecar修正了初稿“共用list/generation”的错误假设：两眼实际分别为list 161b 4d 9b 30/gen 1988765和 1150860d 0/gen 1988767，在同一queue 595086d 0依序执行。CPU策略现保留每眼各自的list/generation；每个feature提交必须匹配自己的记录，GPU顺序仍须由实际adapter的执行/fence证据认证。未被接纳的私有Create返回句柄有明确caller-quarantine归属，不静默遗失。

两个CMake策略测试以`/W4 /WX /UNDEBUG`构建并通过，覆盖边界、跨眼输入覆盖、实际两list模式、部分Create失败、独立handle、过期epoch、未提交/未完成GPU工作、错误queue/fence及重复释放。实现尚未接入活体SDK；源构造的实际低分辨率生产、物理Color输出尺寸、两个feature的正常重建/退出以及图像/时域正确性继续保持未验证。具体接入设计和完整历史chunk身份在ignored `dlss95-p-correct-output-plan-20261018.md`，不打包知识库或反编译资料。

## 2026-10-18：当前运行代码合同认证与隔离组件

从历史完整chunk恢复Create、四参数Evaluate（callback为 0）、Release与 32位BAD码判定，分别保留完整来源哈希；Create的 0x 19仅代表已认证封装的完整消费范围，不冒称结构体sizeof，Evaluate保留调用者初始化的完整 0x 168字节。独立adapter与输出绘制关联组件已加入严格CPU测试，仍未接入真实SDK Hook。

实际Native 101后台游戏 3432中，只读采集对Create/Eval/Release dispatcher、callback helper、结果格式化器及相关原生封装/owner完整chunks逐字节核验并末端复读，all_selected_chunks_matched=true。三个vendor函数指针均落在实际加载的_nvngx.dll（文件SHA 256 66767018c36b3bab46398dade3adf173daa3730fda75965689ea848c9bc4e79b）。精确session与全部函数哈希保留在ignored `analysis/runtime/pistol101a-20261018.dlss.json`。这认证所选代码和指针归属，不证明vendor内部行为、资源同步或DLSS图像正确；preset helper等未选代码不自动取得认证。

输出关联组件明确区分记录位置、真实Execute/Signal顺序及完成条件；同一次Execute中的不同list成员顺序不当作资源依赖。尚需实际生产域、整个参数封装的排他、Create/Eval GPU退休、两眼独立history及原生Release返回政策。默认Native 70与DLSS/DLAA保护均未变，DLSS仍为首发未完成项。


## 2026-10-18 原生分眼桥接组件（尚未接通真实SDK）

新增NativeWrapperBridge与NativeDispatchBackend：完整wrapper期间参数排他、独立双Create句柄、逐眼Evaluate及history reset、按真实完成凭据释放双句柄；关闭路径保持原始参数和返回值。新增output receipt到quad几何转换，遵循destination→source参数顺序。FeaturePair明确拒绝D 3D 12 UINT 64_MAX完成哨兵。六组MSVC /W 4 /WX /UNDEBUG组件测试通过；UiGroup 105整合构建中的桥接与geometry测试再次通过。

这不是DLSS已可用：真实Color/Depth/MV生产区域、typed CopyTextureRegion接线、Execute/Signal完成、首次双眼图像隔离和安全Release仍未闭环；默认DLSS保护保持。完整接线说明与测试日志存于ignored analysis/runtime/dlss-integration.txt与ngx-stereo-native-bridge-test。

## 2026-10-18 默认关闭的原生生产链观察器

新增 `GEARS_NGX_STEREO_HOST_OBSERVER`，将完整wrapper上下文与Create/Evaluate/Release dispatcher、带类型的D 3D 12资源观察接通。仅采样，不激活独立SR，不修改参数、GPU状态或全局句柄。完整历史代码块认证后才安装；资源生命周期、DSV解析与GPU退休仍显式标为未证明，Execute返回不等于GPU完成。默认发布清单不变。

宿主组件测试和启用选项的capture编译通过；协调端再次构建并运行宿主测试。三个dispatcher恢复验证使用固定历史完整块哈希，独立于Hook自报状态；17项Python验证测试通过，包括末端字节改变与短读拒绝。没有启动游戏验证此观察器，也没有DLSS双眼图像验收。接线、证据与限制见ignored `analysis/runtime/dlss-host-handoff-20261018.txt`。

## 2026-10-18: 抓取集成和有界限的命令所有权

抓取当前将真正的构建器 CreateCommandList, 重置, 复制和执行边界转发到 NGX 控制集成。 没有安装激活调用: 默认所有者仍然缺席, 普通的渲染路径仍在使用中 。 Productor- resource/域证据,准确的复制状态/依赖性证据和最终产出/退休整合,仍然是启用游戏DLSS之前的必备条件.

初始集成保留了 a 中的每个直接命令列表 128 历史游戏观测数据超过了这个容量, 因此在运行时间部署前就被替换了。 只有实际的 NGX 记录和飞行中的义务保留列表。 重新开发需要同时观察到重置和完成相关的 GPU fe ce; 不相关的命令列表没有占用持久槽。 在启动前未启动的上下文不会停止正常的立体渲染 。

Root应用了集成和寿命补丁, 新建了主机- OFF `build-runtime-resolution1-ngx-wiring` 候选人通过连续的DLL自我测试, 3,652 真实的 GPU 队列检查和 66 键入资源/盒式像素检查。队列测试使用合成 SDK 调用。代理机还构建了私有主机ON/内控主机和主机OFF捕获配置。这些是集成证据,而不是游戏菜单DLSS产生正确立体声或su的证明 rvives 质量变化。 没有更改玩家显示或发布包 。

### 2026-10-18: 限定范围的真正划时代/空白证据候选人

`eba6be2` and `ba312c5` 在现有构造器/预置器、键入障碍/复制、NGX创建/评估/释放和执行/重置边界处添加默认-OFF时空/空观察器。在重新配置前,最多保存三个完全空的时空时空摘要;任何对齐证据都会冻结其e poch. 候选人完整性明确不证明生产者覆盖,逻辑眼,资源状态,GPU完成或队列依赖性. SDK参数和GPU工作保持不变.

根已建立和连接 `build-runtime-resolution1-ngx-pair/GearsCaptureRuntimeNgxPair.dll`, 莎莎 (英语).256 `65933b1e69e061417b71e72867389fc2cfff645d7d6596feca82f51970fda0e4`,有主机和时空观察器,没有控制激活。注册探测器测试和DLL连续自试都通过。新的实际召回测试使用真实的D3D12 屏障/ 复制/ 执行/ 重置合成 SDK 瞄准镜。 目前还没有游戏运行使用此候选。 下一个限定值的游戏样本必须提供缺失的打字深度/ 移动维器身份、 尺寸和实际对切换; 这些功能仍然不在直径中 。Cal Color/ Output 仅限证据。 没有更改玩家显示/ 包 。

### 2026-10-19: 防止无关的命令交通耗尽划时代的证据

实际 `dlss-native-menu4` 所选 NVIDIA DLSS 样本 3.5 / 质量
已报告原生创建 2560x1440 输入和 3840x2160 output,
but 994 of 1024 记录的事件是全球重置/执行流量。
菜单导航也击中它。
90- 第二次母程序截止日期; 返回游戏播放和立体输出没有被证明 。

执行/重置记录现在需要以前由一个单位提出的清单记录。
选中的屏障、 Eval 或复制件。 等待 Eval/ Copy 错误生成提交件,
失败的重置、 重复和所有选定的障碍都仍然可以观察到。 混合
批次保留原成员/伯爵和CPU呼叫顺序;返回成员为
不包括柜台和 `earlier_history_complete=0` 做一个
缺少早期历史的清晰信息。没有 SDK 突变、控制激活或额外
引入了GPU工作;相关活动溢出仍然停留在 1024.

Root重建了游戏分辨率采样 DLL, SHA256
`26bac014cbd99ca98ee89578e5fb6a6b0e5a0387670a26f3e52506bae128a0aa`.
两次登记时间试验和所有 22 启动检查通过。
include 747 无关的前缀事件,混合批次,待生成失败,
重复提交和真正耗尽能力。
选项, 不发布 DLSS 支持。 没有游戏/模拟器启动或玩家软件包
修改时作了改动;实际的菜单配对证据仍应提交。

### 实际范围菜单6 sample

游戏菜单 DLSS 质量路线现在完成,并通过预期返回
用户界面路径。探测器保留 99 事件和 13 类型资源,不包括 324
执行成员和 569 重新来过,它不再能击中事件容量,但是
在第一个选定的 Eval 输入拒绝时停止(`stop=3`,0 接受Evals).
重创建报表 2560x1440 输入和 3840x2160 输出。 单独的本地路径
样本显示评价呼叫,包括不同的颜色/深度/MV基数抵消;
光是这些技术不足以确定有效的每眼生产者维度,或
历史。 合并输入拒绝目前缺乏有条件的理由;
,在更改 SR 参数之前必须先解决。GPU 发布时仍为零。
原生清理,入口恢复,正常退出和设置恢复通过.
证据存档于忽略之下 `analysis/runtime/dlss-native-menu6-*`.

### 输入的命令列表身份加入候选人

Menu6 选中的 Eval 原始命令列表地址没有在其间发生 99 因此,先前的事件只连接无法建立犬科特征/生成。现在观察员最多只能保留 64 键入 Create/Restet 候选和两个被绑定的 Eval 租赁。 Eviction 丢失了证据而不是制造身份; 失败了 Reseat, 关闭创建, 世界/ 世代不匹配以及 tracker 容量损失。 没有将 NGX 整数转换为 CO 。M指针,不发生事件倒计时,也没有GPU/状态/生产者认证. 租赁在召回排水后释放;第一次拒绝记录其特定的面具和跟踪器诊断.

根重建了取样 DLL, SHA256 `a969235e2f63681e86986cb334bf4550bfc7869b1d3770431b298e74202fadb6`3项已登记的NGX测试和全部 22 启动检查通过。新真D3 D 终身试验练习 600 包含合成跟踪器世代的候选创建。 没有实际游戏运行使用过此修订; 默认/ 包件保持不变。 证据: 忽略 `analysis/runtime/ngx-sampling-startup-suite-typed.json` and `ngx-typed-list-join/`.

### 实际菜单7 样本和复制呼叫器单元更正

Menu7 接受两个带有输入命令列表和资源的评价观察,
执行各一份,但没有记录副本。 237 事件和 41 resources;
6个投放的观察仍然无法解释。UI 路线在发送后停止
当其后键快照超过超时时键; 输入不确定
重播。 正常退出、 恢复和5个设置/ 配置恢复
。此选项不能播放 DLSS 或 GPU 输出接受。证据仍在
ignored `analysis/runtime/dlss-native-menu7-*`.

源码检查发现一个明确的复制-观测器错误:原件
钩子传球 `_ReturnAddress() - base` 进行修改,同时比较复制范围
和RVA一起的飞机 `base + 0x1c6093`。现在复制范围与 RVA 进行比较 `0x1c6093`.
真实的D3 D回归发送正确的RVA,绝对地址和外国
自动自动取款机,检查是否只保留正确的观察和所有原始
CopyTextureRegion 呼叫仍然执行。 它不将六滴归为
此过滤器或证明菜单7'所选的 TLS 范围包含缺失的副本 。

根重建候选人( SHA)256
`219fa9e072ac96013ee8ec0c09849e33011011f957f2e50de152757e4f22d60b`)
并全部通过了所有三次注册的 NGX 测试。前一次的活固定错误
提供了绝对地址; 纠正的固定位置现在遵循真实的钩子
合同。没有为本修订版推出游戏或模拟器。
仍需要样本;发布默认值和包保持不变.

### 实际菜单9: 完整的CPU侧Eval/复制序列

重建的 RVA 修正候选人完成了游戏菜单 DLSS 实际路线
返回到游戏游戏(游戏 PID) 24188; 忽略中的确切身份
`dlss-native-menu9-20261019.json`。选中的票 1528 记录了两件易瓦尔,
Eval每张回执一份,每张Eval/复制记录一份队列执行一份。
探测器报告 `candidate_complete=1`, `dropped=0`, `stop=1`; 所有四个资源
身份在 Evals 前后输入。此选项验证了此边框
仅CPU观察: GPU 完成和制片-域验证仍保留 0.

创建输入/输出为 2560x1440 / 3840x2160。Evals的两份报告 1920x2160,与
颜色/ 深度/ MV 基准 X 0 then 1920; 两者使用相同的原始CODM资源。
使用子资源录制输出到颜色副本 0,没有源框和目的地
0,0,0 这些共享资源和复制范围需要域/历史分析;
它们不能建立正确的每眼像素或可播放的 DLSS 。
已发布零 GPU 对子,总体会话仍然不完整。

曾经 389 有效的封存样本, 没有封存/ 错误记录和游戏
前景样本:通过原生清理和独立入境修复;
游戏正常关闭,所有五个设置/配置备份都恢复。
较早的菜单8 尝试在本地激活前在更改时取消
桌面快照和未提供 DLSS 证据。 完整本地状态保留
忽略未计 `analysis/runtime/dlss-native-menu9-native-status.txt`.

### Menu9 源/域审查与下一个执行界限

输入的颜色和输出描述符都是 3840x2160,一个mip/layer/sample,
format 26。如果子资源为零,则无源框和零目的地,则
复制命令请求全子资源替换, 而非眼- 本地
组成。复制0 serial 134,颜色/输出转换 136/137 和 Eval 键1
138/139 共享列表/生成 `3d7fc1a20/416010`; Eval 语句1 读取相同的颜色。
以此确定已记录的命令命令和输入反馈路径, 而不是
GPU 补全。 Evals 都使用相同的公共特性控件; mod
尚未提供此运行中独立的每眼特征历史 。

下一个执行边界是现有的本地创建源分割输入
选择输入 Render 输入框和构造器/ 切换路径。 此选项
样本的创建- 衍生的分割符是 1280x1440 每只眼睛,脚步 1920,产出
1920x2160 每只眼睛。 仅仅替换评价尺寸就会让生产者离开
渲染不一致。构造器输入必须带有匹配的不可变异性
通过推迟准备/假设创建时代; 单独创建元数据无法
验证实际的颜色/ 深度/ MV 生产者界限或 GPU 所有权。 现有
独立版和盒式复制码在这些事实发生之前一直没有作用
连接和验证。此处不要求新的运行时间接受。

### 创建生成的构造器实验集成

`GEARS_NGX_INPUT_RECT_EXPERIMENT` 仍然默认关闭,并需要时间
观察器加游戏分辨率诊断配置。它重新使用原生设备
为真正的构建器参数创建分割输入路径; 完整的创建身份
复制到提交的框架,并检查整个构造器,准备,
使用和包装边界。观察身份丢失会停止实验
而不跳过原原主调用。这不是 SDK 激活或
GPU 制作人认证。 诊断使用游戏解析输出到
隔离此更改; 运行时间推荐分辨率发射器比例表仍保留
所需员额 0.0.1 交付特性,而不是延后特性。

根的全 DLL 构建 暴露了一个没有覆盖的页眉命名空间碰撞
私有测试工具。 将共享值类型移动到 `gears_ngx_create`,与
用于主机命名空间中的别名, 固定了集成构建。 两次实验均在 On
并编译了完整的DLL配置。 22
连续启动检查通过。
`build-game-resolution-ngx-input/GearsCaptureGameNgxInput.dll`, 莎莎 (英语).256
`7ce3dd337fc9fe91604bbb523d4b1cd0a5c080431d47b6737130edd7acd40edc`.
根收据被忽略 `ngx-input-root-tests.json` and
`ngx-input-startup-suite.json`实际投入减少的生产者矩形依然存在
直到下个游戏样本尚未验证; 共享历史和全输出复制
尚未解决。

### 说明来源基础(2026-10-19,仅CPU)

Added `ngx_mv_descriptor_provenance.h` 以及它的孤立的CPU固定目标.
边框银行保留了输入的资源/堆积身份、音轨描述符
复制和覆盖,并拒绝 Stale shaptris, 范围/装置不匹配,
重叠堆积范围与耗尽的能力。未知的源描述符
副本后仍为未知。 `producer_authorized` 总是不实的

根编译为 MSVC `/W4 /WX /EHsc /UNDEBUG` 成功运行。
被忽略的证据 : `analysis/runtime/mv-descriptor-provenance1/receipt.json`,
`compile.log` and `test.log`. 测试的SHA头256:
`8c1ee90a57918d689f7881a652c7040aa686f735989568a8b5874914653c0f8f`;
测试源 SHA256:
`b0d186191917dc4abb7650ba1be9788e2b4b034564f274e5b1fb7afe61d04519`.
这些是CPU的固定装置,不是真正的COM/GPU或可播放的DLSS接受.
帮助器不连接到运行时钩,也不改变释放行为.

来源检查 `capture.cpp::begin_target_prime` 确认当前
早期描述钩覆盖 CreateRTV 和描述盘副本,而不是 CreateUAV 或
创建描述器Heap。 因此, 以 Eval 选中的票创建银行是
无法恢复早期的无人驾驶航空器创建。 集成必须保留实际的早期类型
具有原寿命/范围或证明实际存在的
娱乐;它不得将历史创作重新标注为所选票.

独立架构审查留下双Eval/每眼-feature路径为
支持的候选者,而不是经证明的治愈方法。下一个受限实验必须
加入选中的 MV 资源复制 UAV 描述符, root table 3 编辑
仅限无人驾驶航空器
无法证明阴暗的文字。制作人验证必须建立覆盖范围,
绘制消费区域图;精确的等大小生成矩形
如果真正的遮蔽证据证实存在较大范围,则限制不必要。
有效的域名。仅此假设无法放松任何警戒。运行时推荐
发射装置规模仍然是最高优先事项 0.0.1 default.

随后的CPU帮助修改增加了一个明确的设备寿命范围:
它的时代是观测捕获生成,世界/盘子保持零。
选中的 ticket 范围是单独的类型; 查看无法重新标记早期创建
以Eval为后期人物. `invalidate_if_tracked` 清除跟踪无人驾驶航空器在SRV/CBV上写到的
而不为无关的描述符分配单元格。
所有CPU固定器都通过(`analysis/runtime/mv-descriptor-provenance2/receipt.json`).
标题 SHA256 `50264634408baf7ccf1f872df4489a68634438778bae9c5e48675fc09dfb6343`;
测试 SHA256 `3890d62e32052b411af8a7ff8b753afd2fa78e1b6a4e06302bafa012d772ee35`.
早期安装钩子, 稀疏的描述式复制处理, 选择使用绑定
和调度证据尚未确定;此修订不在游戏中
抓取路径。 现有的质注入也发生在加载器依赖之后
早早的钩子必须报告失踪的出生 而不是假设
说明每个描述符都是在安装后创建的。

复制API,现在传播已知的无人驾驶航空器来源,删除跟踪目的地
当来源未知时,并跳过未知的未跟踪副本的分配。
他们共享原始范围/重叠/版本验证; 原始复制行为
根的新鲜 CPU 测试运行通过,包括全容量覆盖
并跳过案件(`analysis/runtime/mv-descriptor-provenance3/receipt.json`).
标题 SHA256 `93ade51057bce6f7035644812310d09b6d2527d7b87fd4ffc7e06dfc2a8ed35b`;
测试 SHA256 `0cf4221d0d6b257f32dedc0261ca7c7da293a57faa4082f05444f51ff7f2a9be`.

### 提前描述钩选手, 默认关闭

`GEARS_NGX_MV_DESCRIPTOR_OBSERVER` 附加 创建描述器Heap/CBV/SRV/UAV
观测到现有的质点和视图钩,并共享现有的
首个观测到的 CBV/SRV/UAV 堆积
明确未测试的有界装置候选。
不明出生和产能/顺序的不确定性使观察无效。
原始电话还开一次 历史在源头传递中存活 并且世界在变化
关闭收监和排水回调在保留租赁释放之前。
状态明确报告没有选定派或制作人证明。

根已构建 `build-ngx-descriptor-early4/GearsCaptureNgxDescriptorEarly4.dll`,
SHA256 `37b4f2849ca726346b6a3ba063198f8c12aafec58d4a42d193c6ec5b493e9c5c`.
记录的新鲜建筑469 依赖关系; CPU 来源测试和现有
连续启动套件通过。 分离 `--prime`, `--prime-handoff`,
`--prime-owned-cancel`, `--prime-owned-exit`, `--prime-owned-handoff` and
`--continuous` 引用全部返回0。这些主定着没有创建
一种CBV堆积,因此它们可以验证钩子生命周期/恢复,而不是新打的
解写器调回数据路径。一个专用的真设备固定并选中
MV/Distatch协会在实际游戏制作人索赔之前仍须进行。
没有更改默认的列表或玩家 DLL。 启动跟踪 ON 仍然未验证 :
选择需要不同的游戏分辨率诊断配置。
前三次新配置尝试被拒绝 缺少 NGX 依赖性或
无法兼容的源像素观察器, 在任何二进制之前。
观察者- OFF 配置也通过了一个新鲜的构建, CPU 测试和连续
启动套件 : `build-ngx-descriptor-off1/GearsCaptureNgxDescriptorOff1.dll`,
SHA256 `61fff9f378ac0f349c6a7faa8459176b5fa3089b464cd74dce8705fce6a44db8`.

### 真实描述符调用回调固定

`CaptureSelfTest --ngx-mv-descriptors` 现在创建真实的 CPU/ 阴影可见的堆
和无人驾驶航空器资源,通过原版D3D12 设备方法。它检查
键入身份, 两种描述文件复制的 API, 覆盖版本, 未知源
无效,SRV/CBV替换,保留历史快照并排水
以原输入字节修复的拆卸。 它没有发布调度并证明
无法覆盖阴影。观察员- OFF 构建了明确的返回, 不支持 。

Root的新游戏解析诊断配置, 并带有启动跟踪 ON
通过了这个固定,CPU的源代码测试,活的时空固定,连续
启动套件和所有五个质点模式加连续停止。 Artifact :
`build-ngx-descriptor-game1/GearsCaptureNgxDescriptorGame1.dll`, 莎莎 (英语).256
`47ad57ea19d74219eeedce555d89aaf31e0575deffce4642f43ab8efba41a1ae`.
Evidence: `analysis/runtime/ngx-descriptor-game1-tests.json` 和自试记录;
构建驱动程序 `analysis/runtime/build-ngx-descriptor-game1.py`.
构造-输入实验被关闭,以隔离描述式观测。
游戏解析输出是私人诊断隔离,而不是改变
需要运行时间推荐解析解析默认。

### 实际早期观测员运行: 捆绑累人,而不是生产者失败

`dlss-descriptor-menu1-20261019`届会36016,使用上述精确的游戏1 DLL.
本地菜单帮助器启用了 SR 并返回游戏播放。 选中的
world1/ticket1334 记录了两张Eval,打入前/最后口罩15 已完成 并已完成
现有的CPU对探测器(没有GPU完成或生产者-域要求)。

独立早期描述者观察员在第九批观察中失败:
heaps8 单元格2,重声调用9,无人驾驶航空器呼叫2, 复制本138,其他设备0 失踪的婴儿0,
recursion0 损失3 因此,堆积限制,而不是无人驾驶航空器的细胞限制,
无法防止有意义的 MV 选择出处。 这不会显示缺少运动
向量或供应商/运行时间缺陷。盲目解释不完整的地图
范围有限的普查或合理的观察能力
在选定MV加入试验之前需要调整。

原生清理、 条目恢复、 普通游戏关闭、 主人公退出0 和所有五个
保护文件恢复通过。最后观察员地位报告发布1.
已归档的原始证据 : `analysis/runtime/dlss-descriptor-menu1-native-status.txt`
and `dlss-descriptor-menu1-prime-status.txt`; 生命周期接收:
`analysis/runtime/dlss-descriptor-menu1-20261019.json`。没有改变公开默认值。

## 2026-10-19: 选定的MV种子和未来组合核心

新建建筑 `build-ngx-target-core1` 通过了依赖性核查(457 包括),连续启动套件,描述性出处/检查测试,目标窗口CPU测试,活期定点和本地描述/prime/handoff/持续定点. DLL SHA256 `a5229ce02566dd3c33352784060e260378847f55db060370a1bc7539347e7f92`。收件者:忽略 `analysis/runtime/ngx-target-core1-tests.json`; 全部输出 `ngx-target-core1-selftests.log`.

种子 API 进口保留了打字机租赁,并在保留其选定的 MV 资源、 计数器和视图之前重新读取了当前源单元格。目标版本仍然为本地版本; 不可移动源源代码还存有别名副本。 测试覆盖 stale/ forged/ Formerign ob 。保税,无效进口,源头退役后的资源所有制.

独立的CPU窗口排除选择票,允许在下一个新用户消费时使用不同的渲染器,并且需要稳定的世界/创造修订/MV+一两个成功返回的Eval呼叫. endin, nocking, 身份更改和缺失/unf 已丢弃 Eval 终止了观测。一个冷冻窗口是排程证据,而不是立体声或GPU 生产者证据。

这些核心尚未被连接到运行时命令列表/ 发送观察。 没有为此步骤启动游戏; 玩家默认和发布包保持不变 。 下一步: 将已设定的选定源描述符捕获和命令列表状态与证明合并 ed运行时调用器,然后收集实际游戏证据.

### 目标运行时间整合制约因素

对当前SrapperScope和shook_consume的审查证实,独立的目标水龙头必须在诊断Eval配额返回之前,并且不得依赖于已完成的划痕-pair探测器。选择保留了经认证的本地Create源并输入了MV le ase. 后期结构+0x20 价值只与持有的租约相比较;它们从未成为COM指针。

控制器开始使用已安装的冻结票并检查当前所有者。 控制端使用保存的身份, 因为原版_ 控制器可以释放渲染器。 包装检查必须使用本地_ create_ matches: 本地_ create_ state 故意拒绝活动 w 说唱歌手。独立的 SDK 收据必须要求精确的匹配命令/手/参数引用,不回调,正常返回,本地_结果_接受,以及匹配低32 包装/ SDK 结果。 仅仅从包装器返回是不够的 。

所有跨层检查在描述符 mutex 之前保留帧_ hook_ mutex。描述符代码不能获取帧锁。等待/抓取需要外部的截止投票才能调回; 创建/释放/世界更改使其无效。 关闭录用前会先关闭 钩子排水和保留资源释放。 这些是集成要求, 不是运行时间线条完成的证据 。

### 已验证选中的描述符调用

Fresh `build-ngx-target-capture1` SHA256 `95fea68e3ee1b0ebbfbd6dc067e5300f14e9a2cc4668f57437edea8d25080025` passed457 依赖性检查、启动套件、重点式CPU测试和补丁实-D3D12 描述固定。 `analysis/runtime/ngx-target-capture1-tests.json`; 完整日志 `ngx-target-capture1-selftests.log`.

最初接受的Eval现在导入并种子了一次独立的选定MV库。 完成种子冻结源更新; 后期源/ 遗留失败无法重开或重新给目标贴标签。 原生回调保存选定的MV别名, 擦除上写描述 或 并跳过不相关的分配。 stop_ targets studys 时保留所有权; 排空关闭释放观察员锁外的打字租赁。 Fixture 已核实后堆导入、 选定本地重写、 无关替换、 化名分界线、 停止 便便入门清净.

此构建没有消耗窗口/ 发送线条, 也没有在游戏中发布。 它的状态明确识别收藏直到明确的停止/ 关闭, 没有设定界限的完整发送证据。 下一步执行已验证的消耗/ SDK 回钩和截止日期 poli ng 实际游戏使用前的 。 玩家默认不变 。

## 2026-10-19: 未来消耗和游戏中捕获到的 SDK 真实返回

新的目标窗口4 沙发256 `06f28bede2ecfa74e28e28ee3dfe11105e65c9ec35ec7eda67227c4bd0dbb3d1` passed460 依赖性验证、启动/内置描述定着、SDK接收边界和实际调度-inl CPU固定。窗口1 测试阴影警告失败; 窗口2 曝光一个延迟的宏门, 尽管修复成功, 却禁用真线。 纠正到早期的 EPOCH_ PAIR_ ObSERVER 门, 并明确编译断言; 窗口3/4 编译活动线条。窗口4 并更正描述式状态,以参考单独的列表收件。

Actual `dlss-target-window4-20261019.json`届会 `continuous-stereo-30148-20261019-112100-f6049182`: 首张选中的源票617,下一个完整消耗618 创建修订1/world1/MV身份。窗口阶段3 冻死,原因0,两个独立返回的SDK电话,两个都是原始的1 和包装1. 取样持续63 从选择到完成的 ms。 保留了选中的银行59 heaps/3 MV 单元格, 零损失/ 不确定/ 饱和度, 停止输入和排水的银行释放。 这是 CPU 调度/ SDK 成功的证据; 调度绑定、 GPU 生产域和可播放的立体 DLSS 仍然未经验证 。

驱动程序错误/ 中断无效, 正常游戏关闭真实, 5个设置/ 配置文件恢复真实。 所有人退出 。1,因此这在总体上不是成功的 VR 部分接受。 `analysis/runtime/dlss-target-window4-20261019.native-status.txt`。玩家列表中仍然保留了 runtimeMirorFix1。下一次收集到已验证的命令列表绑定/发送证据,但不会增加全局描述符存储。

## 2026-10-19: 计算具有约束力的先决条件已核实

添加了 QPU 纯 模型 `ngx_mv_compute_binding.h` (16 条形命令列表, 独立验证的记录代号 。 不同的堆积/ 根签名、 重置、 清除状态、 执行Bundle 和生成更改无效根3 表格证据; 冗余的同一堆/ 根保留参数。 无效/ 丢失的生成验证和容量损失是终端。 调度返回的只是有标尺约束的候选者, 并且明确不推断GPU 处理子程序下的成员列表raction.

编译并运行了 NgxMvCompute Binding Test and D3D12 AbiTest 成功。 Windows SDK C- vable 正在额外认证调度14,设置描述符28,设置计算机 Root 签名29 并设置计算 Root 描述表31 (38 共声明 。 这些独立测试不需要 DLL 重建或游戏发布; 玩家 DLL 不变 。

找到整合审查窗口4 已存在 DETH_ PSO_ CACHE 关闭, 因此其现有的 ClearState/ ExecuteBundle 包件已不存在。 运行时计算观测需要在该配置中添加六个插槽( 四个绑定/ 发送加 ClearState)11/ExecuteBundle27),或当重用已安装的 PSO 包装器时使用4。重置无效必须包括绕行路径和失败的 重置。保留打入的回调列表/重置身份,绝不从 NGX 整数重建 COM; 使用新帧_链接生成和帧- 默认 ptor 锁定命令。 范围以已验证的目标窗口结束。 这些钩子尚未被此任务安装 。

## 2026-10-19: 计算回调集成; 生成拒绝保留

添加了有线型的计算截取(六个钩子带有PSO缓存关闭;四个钩子带有现有的清/捆包装),重置/清/ExecuteBundle无效,当前在调度前/之后选择的MV搜索,以及窗口停止/排练的租赁集成。等待 记录绑定状态; 只有选中的窗口 MV 相关发送候选人使用32 排位限制。 偏移是候选人, 而不是描述表的会员或阴影写作的证明 。

历史计算块与完整的文物重排1 双球( Blob) (`compute-caller-historical-contract-check.json`));门式主机合同包括四个完整的热/冷块。1 运行被拒绝, 因为现有的验证缓冲器大小为小 Eval 包 :2554-byte17b420 生产无散列,没有经核实的字节不匹配。4096-字节缓冲器可以修正核查者;没有合同截断或比较放松。

新鲜计算2 SHA256 `26bb94918d11d2c7bbc540c6422bc19652bcc78acad51451477d2b4e6b6e26ec` passed463 依赖性验证和启动/描述/窗口/SDK/计算核心/ABI测试。 `continuous-stereo-43600-20261019-115555-64ce2a9b` 全部确认21 主机块匹配, 选中的票630 - > 消耗631 有两个 SDK 返回成功。计算提前停止:失败10 无效的生成, 核心_ 丢失1,16 持有的清单,14invalidations,6waiting+40 抓取绑定, 0 调度行。 这是一次失败的计算, 而不是没有 MV 生成的证明。 下一次收集精确的第一个失败列表/ 生成/ 站点; 不要提高容量或以推测方式接受 stale状态 。

驱动程序错误/中断无效, 正常关闭, 所有五个设置/ 配置恢复通过; 所有人退出1 表示不可播放的 VR 值值值接受。 `analysis/runtime/dlss-compute-capture2-20261019.native-status.txt`。玩家默认值保持不变。

第一次故障仪器候选人计算3 现在新建和通过同一个套房(第1款)463 附属关系,SHA256 `4baaa22c3de92f52f6eca1062c305cf4b7db9bdf747c0f7a38fc380849b34f79`。它记录第一个拒绝站点、打入的列表身份、存储/当前世代、重置结果、所有权/旁路、窗口/盘/世界和失败前的核心损失;它不改变拒绝规则。3 尚在下一个取样。实际计算2 本地_ 清理_ 验证和条目_ 恢复_ 验证都是真的。 没有游戏进程 。

## 2026-10-19: 破坏性无效化错误地需要旧的一代

实际计算3 session `continuous-stereo-14892-20261019-120819-e65b7496` 标注第一个失败: 重置_ begin - > 无效_ 重置_ 或未观察, 重置的生成164869,新跟踪器身份/生成0,呼叫线形票/冻结票/退票0 窗口601->602 仍然完成了两个 SDK 返回。 这证明观察者被过度严格地撤销, 而不是一个确定的游戏提交错误, 或解释追踪者为什么将列表作废 。

已更改的撤销为只删除 `forget_binding(canonical)`,不分配行,也不清除粘滞损失。重置开始为零控值生成。只有此引用成功框架返回的实际列表参考文件_link.reset_success 才能恢复它,并且只有在当前新引用仍然匹配时才恢复它。 成功重置而无此收据, 未知。 原生失败/ unwind 和严格的未知/ 封装/ 发送拒绝 。 保留不变; 没有增加上限 。

新的计算重置1 SHA256 `a352096f93d713837e74f3d3de778a75fad71c844c9566dc7b87473cdcc02f0a` passed463 依赖性检查和完整的诊断套件。添加的CPU测试包括不分配擦除、不相关的列表保存、粘滞的先前丢失以及后来缺失/匹配的生成拒绝。下面将进行实际的重试。

实际重置1 session `continuous-stereo-38636-20261019-121441-0f5a9e7b`: 上次重置_ 开始拒绝已不存在。 窗口602->603 已完成两个 SDK 返回。计算已提前到42 抓取绑定的呼叫,然后在轨迹中以真列表_容量/根部停止17 列表( ưμ㼯A16 保留),选中的票603 中,尚未发送行。这不能成为声称MV生产商发现的理由。接下来的收录范围或测量相关的列表群;不要将容量损失与修复的破坏无效错误相混淆。错误/中断无效,正常关闭 。d 设置/ 配置恢复真实, 主人退出1。原始的痕迹是 `analysis/runtime/dlss-compute-reset1-20261019.native-status.txt`.

## 2026-10-19: 边距计算- 排位再利用, 实际负结果

新鲜计算1 沙发256 `3747425b19b8f65c1fcc457d9949eef3ce86b93c3f9a1fdb09d7dd692a31d8f1` verified 463 依赖性并通过了启动套件、描述器固定、计算绑定和ABI测试。 16- 列表限制保持不变。 重新使用需要明确取消空绑定, 匹配当前记录知识, 没有主动回调/ 重置和保存的行引用 。 输入的设备和记录身份在容量之前检查 。 Detach 版面所有者在帧锁外发布. 测试内容包括部分绑定,绑定/繁忙记录,沉滞的世代和未知的一代拒绝.

Actual `dlss-compute-reuse1-20261019`届会 `continuous-stereo-2120-20261019-122815-003ec9e9`,选中的票597 和观察的消耗,598。两个本地SDK电话都返回了接受的结果1。计算观察者在五次尝试中重复使用4行空行,然后按容量停止:全部16 条目有部分/非空知识,没有符合条件,没有繁忙或历史固定,只有一个僵化。 `track_capacity`, 操作根, 有经过核实的同一设备当前录制。 0 已选中 MV 调度行。 目标描述器收藏本身被保留61 heaps/3 零损失的选定 MV 单元格。

接下来的调查应该将承认绑定在已经经过字节测试的本地计算帮助者bef周围。矿石的堆肥/ 根根/ 表格设置。 仅靠根表返回后期无法建立先前的绑定值。 这些 SDK 返回中没有生产者/ GPU 或可播放的 DLSS 索赔。 所有人退出1 VR 接受度仍然不完整。正常游戏关闭并恢复所有五个受保护的设置/配置文件。默认启动器和发布 DLL 并未更改 。

## 2026-10-19: 独立打字员普查确定误选计算工作

ComputeCallers1 SHA256 `e41e322f7a142edc0c9b9dd3728226e3940d90d688e9f446d56a59afebc4be83` 通过了经核实的新建筑和现有套房。32- 现有类型绕行的群划板普查仍然由自有目标消耗窗口进行,独立于计算-银行故障。它没有保留COM/List状态;完整的桶数省略了观察。调用器 RVA 需要加载图像的加护约束 s check. 没有假设新的本地条目 ABI 。

Actual `dlss-compute-callers1-20261019`届会 `continuous-stereo-46408-20261019-123616-5eece70e`选中604/consume605,捕获248 呼叫中,请16 组,零溢出。在选定的消耗中,返回根138124 和堆积的归宿,13813 a 发生87 根返回时22018 d 和堆积返回2200 ed 银行倒闭后, 每一段都发生一次。 表格返回17b99 a 和调度返回1ac833 每一个都发生了14 。所以,在银行倒闭后,记录的零行是观察损失,而不是没有本地调度。这些分级组不通过命令列表身份加入呼叫,也不证明任何调度组会写入选定的MV。

KB - 首个对文物进行线下解码1 function220134 对照源代码SHA 区块验证了两个完整的块911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 和合并函数散列 a0d3715ba3368d42f377146c18713e6f4534df023a23d0eec7dcc84d425f4288. 议会有计算- 根基虚拟呼叫220187 returning22018 d, 后面是描述式- 堆尾跳2201b5 故该归其召者所有,与所观察的一致。2200 编。这是历史/当前位置的函证,不是完整现字节的证明。220134。保存的机械证据: `analysis/runtime/dlss-compute-setup-saved-1-220134-20261019.json`。缓存的伪签名已损坏,不得定义新的本地钩子 ABI。

下一步:检查和认证设置/初始化呼叫者138124/13813 a 包括:2200 ed,然后在不保留无关的初始列表的情况下,确定每条记录的设定到发送关联。1ac6e4 。原始电话继续维持不变。实际运行正常关闭,并恢复所有五个受保护文件;所有者退出1 默认/释放保持不变。

## 2026-10-19: 设置范围化的录取暴露了较早阶段的依赖性

完全历史138020 (438 字节)和2200a8 (140 字节)在 KB 检查后被机械解码和 hash 检查,保存在 `dlss-compute-init-saved-2-2200a8-20261019.json`.138020 直接设置计算根 :13811 e(返回时)138124),然后是描述语句的堆积,138134 (return13813a).2200a8 calls220134 at2200e8; 其尾部堆积调用返回2200 编。 现有的当前进程主机证明中增加了四个完整的设置块,包括220134 冷码。 没有使用本地输入签名 。

私人缩小的观察者只按输入的根回调输入新列表22018 d 在抓取时。 现有的精确的输入列表参数仍然得到严格的生成、 绑定和重置处理。 未知的调回符跳过 COM 的录入并被计算; 界面别名覆盖明显不完整。 重复的空格设置别名失败 。 而不是创建两个记录。这是一个有界限的实验,而不是一个不观察初始状态无关紧要的说法。

初始范围d1 构建失败, 因为协调员在错误的预处理器块中插入了合同行; 在游戏发布前改正。 fresh scoped2 SHA256 `41480d5029657e0944512cfc096afc0f0319a8bc05b4611f8a35c287aa048eb6` 实际数 `dlss-compute-scoped2-20261019`届会 `continuous-stereo-1908-20261019-125318-85e02884`,所有已核实25 当前块。 0 银行损失, 但被接受的列表为零: 设置根22018d/heap2200$ 每一个在等待票中发生一次595,则14 Dispatch1ac833 抓取时发生呼叫596. 征召者普查250 calls/16 组合/ 0 溢出。 因此, 仅捕获录入为时已晚; 设置位置在选择边界之间不尽相同。 没有编造缺失状态。 下一次更改必须允许观察到的等待阶段设置并保存其真实记录基因组 通过捕获进行配位,拒绝任何干扰的重置不匹配. CPU关联仍然无法证明GPU MV内容.

游戏正常关闭,所有受保护文件都恢复;所有者退出1 默认/释放包不变。

## 2026-10-19: 早期设置的录取尚未确定调度列表

输入已验证类型22018 d 现在接受等待和捕捉两个阶段;所有所有权/世界/记录/重置检查都保留。1 SHA256 `d943eb1a73ee43f1002eb8c1c0c52f796d157455a30774e9c6ed496ddc33d168` 通过了经核实的建筑和套房。

Actual `dlss-compute-earlysetup1-20261019`届会 `continuous-stereo-45080-20261019-125818-4fc9fdc2`,保留3 列表中15 包含约束性事件,没有银行故障或能力损失。14 发送呼叫在精确保留键盘的搜索中未知; 零 所选 MV 调度行。 此运行被承认的绑定在抓取时发生, 因此它本身并不显示成功保存等待到抓取。 先前运行已建立 为何需要等待录取,但并非足够。

下一个区别是不同本地列表和保留犬科特征的界面别名。14 COM 对象与指针不平等不同。在输入时进行边框的条形比较,可以解决这个问题,而不会允许宽的初始化流;生成和别名重置处理必须明确。普通的近似和五文件重置 离子通过; 所有人退出1 默认/释放不变。

## 2026-10-19: 犬类比较拒绝别名解释

已打入的调用符首先解决原始匹配, 然后针对保留了的空洞行临时 IUn known 身份。 没有添加别名 银行或更广泛的空洞录用 。 匹配别名后撤销/更新同一行, 包括退出的 Consume Reset/ ClearS tate/bundle路径。临时身份参考通过调用方式保存,并在本地帧锁定后发布;解析器再录/QI故障拒绝覆盖。八个有边界的调度身份组暴露出无与伦比的空洞身份。共享 PSO 包罗我们 e 相同的撤销范围;其内容在计算观察员之后移动,以披露其类型,并取消执行。

Alias1 SHA256 `32f4e86ac73f670245ddf76a7a019e20d55888a786f8302729c235e4a47a398f` 。一个明确的 PSO 启用的构建在禁用相互排斥的 VR_DOF_COMPAT 之后通过; 初始不兼容的配置在编译前被拒绝。 `dlss-compute-alias1-20261019`届会 `continuous-stereo-43700-20261019-130812-ec17afd8`,保留一个设置列表,没有显示失败。14 调度是无可比拟的:12 on16180da610/当前生成159059,2 on3d3eb9800/当前生成159166。两者没有保留行匹配。分辨率计数器:4 原始火柴, 和0 aliases,988 unmatched,0 QI失败,0 拒绝重返;身份群体2/8,不溢出。此选项排除了接口别名作为本样本中的原因,并拒绝22018 d 设置路径足以显示已观测到的调度列表。下一次试验应从所选MV描述符开始,该描述符位于经认证的根部3 表格调用, 保留未知的 root/ hiap 状态 。

评论发现, 重置的新预设QI/ 最终发布寿命没有完全被基于主动的拆解覆盖。 一个单独的活动守护符现在在绕行/ 未拥有/ 正常和例外路径上附加计算重置程序, 而不会改变世界的录入。 Fresh Alis2 SHA256 `1fc8e6977d66ee488d115121c8876c20fbefdfcd47a20c21df65adf9ad0fefd0` 更正后经过的建筑和套房;2 尚未运行游戏中。启用的 PSO 编译只比此活动守护器早。没有声称目标被封禁的QI/释放货币测试。

Alias1 游戏正常关闭, 恢复所有五个受保护文件; 主人公退出1 默认/释放。实际身份证据和最终源构建被分别刻意记录。

## 2026-10-19:资源-第一准确MV协会联系到调度候选人

独立探测器解决了认证的根3 表格参数17b99 直接通过保留的目标描述目录。它需要精确的基数选择MV匹配,没有改变的描述版本/原生于原始调用、选中的设备和真实记录身份。一个实际的打字列表和单词Iunknown 任何观察到的堆积/根部/重置/清除/捆绑或根部3 解除后将保护探测器的正常状态停止; 它没有添加 GPU 命令。 认证调度1ac833 需要相同的条条/生成/窗口和描述。8 scalar 行明确保留约束-厚度/根-铺设/阴影-写/GPU 证明.

真正的描述符固定在现在测试精确的 MV,错误/零/密索中连接的手柄和被擦除的描述符。初始资源1 汇编一个已曝光的宣布命令问题( 描述性执行在计算后包含); 共享的标尺定义移动到shraw. h , 并在当前浏览之外添加一个前置声明。 FreshResource2 SHA256 `25b8098c40ada125f671eb4d8f3ac26b86fda5b3193bc8c2fcac6297cc9356d7` 通过了建筑和套房。

Actual `dlss-compute-resource2-20261019`届会 `continuous-stereo-45528-20261019-131947-a0e04572`选中625/consume626,俘虏一名候选人 调度: 警犬3d3dfd9b0, 生成169407,堆积1890a900,MV 时16186653b0,GPU 控件5678a01009e40, 版本2/origin1 组合80x90x1 原发件人返回,描述语保持匹配/版本2。一个表格尝试,零漏失,一行,不溢出;一个后面的根3 覆盖已停止的观察。 仅从发送组数无法推断出阴影线组大小或像素覆盖。 正常关闭并恢复所有五个受保护文件; 所有人退出1 无法播放- DLSS 接受。

复检发现一条可避免准确重复拒绝的嵌入式已知快速路径。 检查现在既发生在原始解析度, 也出现在化名解析度之前。 Fresh Resource3 SHA256 `ba90d411256e63491d99a0f1b3f16cf5da8cabf12ec0844f43e54eb53254fef2` 已通过构建/ 套期; 尚未运行游戏。 资源2 排行是一个很有希望的历史候选人,必须和资源一起重复3 ; 仅报告零次拒绝请求无法排除先前的快速路径缺口。没有要求指定被阻断/取消的回调断层。接下来:用正确的防守重复这位候选人,然后识别其阴影/恒定并验证实际的MV像素覆盖。默认/释放不变。

## 2026-10-19: 纠正资源第一警卫复制协会

Resource3 (`ba90d411256e63491d99a0f1b3f16cf5da8cabf12ec0844f43e54eb53254fef2`实际 `dlss-compute-resource3-20261019`届会 `continuous-stereo-46744-20261019-132452-ee948f75`,重复在嵌入式原始/异形后精确的关联。80x90x1 票号626/world1, 犬类161752afb0/generation169581,描述文本2/origin1 前后。 后继根3 覆盖一个关联探测器的结束, 以设计为目的; 没有溢出。 正常的关闭和五个文件恢复通过。 所有人退出1 仍然未完全接受《公约》。

这证实了在校正诊断下窄的CPU关联; 绑定堆、 根铺设、 阴影写域和 GPU 执行仍未被证实。 下一个绑定执行是 SetPipeline State 返回时滚动键入的 PSO 证人17b514,只为精确表的同一条条框记录/窗口而推动。缺少缓存状态历史必须仍然未知,并且不能压制现有的资源关联。只有在稳定的PSO协会之后,一个推迟的 GetCachedBlob 尝试才能被取消。 检查它的文物; 驱动缓存blob 不是一个自动阴影字节码。 目前尚不需要常数的倾卸或线组大小的推断 。

## 2026-10-19: 限定键入的PSO证人候选人

私人计算Pso1 诊断结果增加一名待决证人和一名经证实的SetPipeline状态返回时被推举的PSO证人 17b514。促进需要精确的选定 MV 表的条形列表、真实生成、世界、票面和渲染器。之后的Set/Reset/Clear/Bundle在保存一个完成的稳定的调度收据的同时,会使当前的有效性失效。缺少的PSO证据不会预先被复制。释放现有的精确MV关联。不读取缓存blob,也不通过此更改确定阴影执行或MV正确性。

新建PSO,通过完整的诊断自试套件(463 已捕获的依赖关系; DLL SHA256 `49196b4067766fe8856c219490958c4e0f8e8f0e0097886d92ad55894489ce60`。独立的 PSO 缓存配置也用其共享的包装器成功编译,避免重复 Set 钩。测试覆盖了空格化名匹配、生成/文本/序列不匹配以及历史有效性在撤销后的保护 离子。运行时采样与这些 CPU 测试是分开的。 玩家默认的列表仍然是运行时的MirrorFix1 关闭了DLSS

实际计算Pso1 run `dlss-compute-pso1-20261019`届会 `continuous-stereo-16172-20261019-133535-05372ab1`,观察到一名候选人、一名晋升和一名返回的稳定的准确调度人员与 `pso_stable=1`; 没有 PSO 身份清空失败。 假设604/world1/generation163441,条形列表 `3d1601540`,支持和平行动 `13ba08f90`,顺序775,证明已发送返回 `1401ac833`组合80,90,1。精确的描述符仍为版本2/origin1 。在调用整个调用中, 之后的变异取消了当前的有效性, 同时保留了历史稳定的证人。 这可以识别一个与选中的 MV 调用相关的输入 PSO , 而不是它的荫影字节码或 GPU 写。 没有调用 blob 。

测试使用了游戏的本地 DLSS 菜单路径。 正常的游戏关闭和所有受保护的设置/ 配置恢复都通过; 主人公退出1 仍然是诊断/ 非播放结果, 而不是 DLSS 接受。 下一个边框分析步骤是一个后冻结缓存- PSO blob 检索和格式验证; 不透明的缓存数据不能作为阴影字节码处理。 玩家默认值不变 。

## 2026-10-19: 一个工人端选定的 PSO 缓存检索

私人计算布洛布1 在保留的历史稳定 PSO 上添加一次后冻结的 GetCachedBlob 尝试。 实际工人线程被明确注册; RHI 和共享勾选的自试呼叫器无法触发它。 本地活动守护符会附加输入的所有权获取 。 缓存呼叫、保存和所有 COM 释放。 本地呼叫和文件 IO 发生在帧锁之外。 CREATE_ NEW and a 4 MiB 保存封顶绑定了文物; 驱动程序内部分配本身不受该封顶的束缚。 收件包括 HRESULT、 字节计数、 路径/ 错误和不可改变的目击键。 在单独验证前, 这是不透明的缓存证据, 而不是阴影或 GPU execu 电磁证明。

新鲜的建筑和诊断自试通过,DLL SHA256 `b33e11f50cd770600dfa8b3a5328a1d2273979062c9b04b8268b433d0a39f397`, 463 已捕获的依赖。 实际游戏收藏是单独的待决步骤。 玩家默认默认不变 。

实际计算布洛布1 session `continuous-stereo-18900-20261019-135347-338316af` 保存了完全一个4662-字节缓存( HRESULT)0),世界1/consume602/generation162870,稳定精确的PSO `14683a9c0`,顺序772 缓存SHA256 `522e5cb888f90606b1dad1eb52df6826a5aadf589ab7d683a5e61a989c984db8`。正常游戏关闭和受保护的设置/配置恢复通过;所有者退出1 无法播放 DLSS 接受。

现有边框提取器发现2360- 字节计算6.0 缓存 DXIL 容器 中 键 偏移954, 莎莎 (英语).256 `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`D系统3 拆卸返回80004005; 已经安装的 Windows SDK `D:/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe -dumpbin` 成功。 已保存的垃圾堆: 忽略 `analysis/runtime/ngx-picked-compute-blob1-dxc.txt`元数据声明8x8x1 线程; 已观测80x90x1 从而发射640x720 线状位置, 不仅仅是目标覆盖的证明。 Shader 声明 CB0(160 字节, T0/T1 和 U 键0. 能力建设0 register5 zw 抵消负载和存储的整数线程坐标; 寄存器5.xy 坐标正常化,登记6–9 参与倒转转换和登记4.xy 在输出前被减去。 这些操作是解码的操作, 不是对常数进行认证的语义名称。 根放出源、 实际值/ 眼睛、 提交执行和产生的像素仍然未经验证。 接下来的分析可以针对这些光谱 ic绑定/固定幅度,而不是倾销任意缓冲。

## 2026-10-19: 已选择的 MV 根版式在实际游戏中加入

私家根号1 DLL `47f32e3c6247e676bea0235d97ba6837e033c361a267f43401783034fe7aef0f` 通过了新的建筑( Name467 , 连续启动套件、 描述器/ 根套件和 CPU 身份/ 撤销检查。 在开始游戏前第一次发布被拒绝, 因为当前玩家配置包含 `render_scale` ,而此诊断使用游戏分辨率。重复使用了与该字段相关的单独配置;玩家配置没有编辑。

实际根1 b 届会 `continuous-stereo-9404-20261019-191847-a7c2d089` 将选定的MV、PSO和保留的序列化根与返回的稳定调度:世界1/ticket483, 犬类 `161a09f030`, 生成127294, 渲染器 `86aed440`根号 `71c5c0b0`,设备 `18c73790`来源行3。根历史通过调度保持稳定;后来的撤销没有重写保存的收件。独立根库捕获了四个根/712 字节没有丢失、冲突或读取失败,并在关闭时释放所有所有者。

当前的状况 228-字节计算根块的SHA256 is `5c0dd75ff306c87e61ed38dbf1733480a40419c7c5f22ba06a00cf75a5ff5717`。现有的 SDK 解析器报告根0=SRV t = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =0..31 根号1=CBVb = 数字0..7 根号2= 样本 s0..15 根号3 无人驾驶飞行器u0..7,所有空间0/ 所有可见度。 单独的缓存 PSO 中再次包含 2360- 字节 DXIL 散列 `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`。因此,这个特定的实际调用根版式可以与已经解码的阴影连接。这些是绑定的观察,而不是GPU执行或像素覆盖的证明。

普通游戏关闭,本土清理,独立条目恢复以及所有五个受保护的设置/配置恢复都通过。所有者退出1/ 部分不完整仍为不可玩性结果;没有DLSS发布索赔。 `analysis/runtime/dlss-mv-root1-evidence.json`; 完整的本地日志 : `dlss-compute-root1b-20261019.native-status.txt`.

## 2026-10-19: 带界限的 CB0 上传证人候选人

KB 搜索后, 协调员只从新的私人开发协会副本中输出相关的完整组装块, 并关闭了两个自有的租赁。 所有输出的字节都符合历史运行时间的文物 。1 (`911ab9...`),基数 `0x140000000`;来源私人数据库仍为散列 `a840be95505d1ae9f2e776fa695925b11f23f92aab387951832a006782f0ae0c`. 证据被忽视 `analysis/ida/dlss-mv-constants-20261019/`.

已经认证的计算调度器使用 `context+0x13cc` 新候选人只观察银行0's 160-字节前缀,将其CPU源头与本地映射的上传范围匹配,返回时实际键入 CBV 创建1ad2d9,返回时本地描述副本17bea9 根和根1 返回时绑定表格17bed7。它保留了scalar地址/字节,从不从整数构造COM,没有映射资源,也没有改变常数或GPU命令。输入内存、环控、偏移、维度、当前记录和描述符上方被保守地检查。需要为上传/CBV/表列帮助者签订 ete 时字节合同。 阴影/ 根解析和 GPU 执行仍然分开检查 。

新鲜常量3 DLL `340075ea41ad8867781f14c90321a05813593ed73697a7546763e99e4aa25d4d` 已通过构建( E)469 已捕获的依赖性、启动/描述/根定着和溢出/距离/识别检查。早先的建筑暴露了两个私人测试带中同时发生的帧-mutex类型变化;它们的递归-mutex别名在不改变测试行为的情况下进行了更新。或。用户随后授权正常关闭手动 VDXR 会话,并继续进行无人出席的测试。此 DLSS 工作没有推进玩家列表。

初常数3 运行在菜单通知守护器上停止,然后启用 DLSS 。 一个私有菜单路径复制器现在接受已知的准确世界通知状态, 并且只打开已核实的 HUD/ 托拉斯堆栈的暂停; 六个守护器已过 。3 b 届会 `continuous-stereo-1552-20261019-194618-bbd69ef6` 完成了本地菜单转换和两次本地SDK评价。它捕捉了4个匹配的 CB0 上传和4份本地描述文本,但最终CB为零0 表格绑定。所选的 MV Dispend/root/PSO 仍然稳定; 这不能提供恒域或可播放的 DLSS 结果。 本地清理、 独立输入恢复、 正常关闭和所有五个保护文件都通过。 Raw 状态被忽略 。 `analysis/runtime/dlss-compute-constants3b-20261019.native-status.txt`.

Constants4 添加首个失败阶段而不放松身份/ 距离检查。 它的初始发布在游戏加载前被寄存器的单ton拒绝; 受保护的文件被恢复。 私人重复的飞行前检查既包括游戏, 也包括寄存器lau ncher, 并等待另一个测试的完整父进程完成恢复。 此拒绝的启动不是 DLSS 运行时间测试 。

来源评论发现候选人的视图会员制错误: `renderer+0x68` 指向毗连点 0xd30-字节本地视图, 由已有的构造器和所有者代码检查。 读取其前两个机器单词作为视图指针是不正确的。 常数5 reuses `native_eye_index` 并保留原始的打字命令/上载/复制检查。新鲜 DLL SHA256 `6a522a97301b34ee129aaadf5eb0dd8e055035f307c976b943d53ab0e6bcdbde` 已通过构建( E)470 依赖、启动和描述/根基/CPU定着。接受关联前仍然需要运行时间结果。

## 2026-10-19: 观察到了两个选定的MV常数绑定

Constants4 b 确认了四次表试,但都仅在观众席阶段被拒绝5 常数5 c 然后记录了四个有效表格装订,没有阶段故障,并记录了一个稳定的选定MV调度/CB0 先前的两个常数5 尝试在更改菜单值时停止; 私有导航现在只重复了已设定的、已知的获取更改, 并且从未重复张贴的选项密钥。 这些尝试通常关闭并恢复其受保护的文件 。

单联观察员曾经停止在下一个根上3 覆盖。常数6 将最多八个不可变的关联/所有者历史保存在同一选定的消耗窗口中; 未完成的设置/ 发送中的一个变异仍然拒绝。 输入的所有人仅在回调排水后释放。 fresh DLL `a1a0fbf446ee48d20a43caf55c5a87ac486f943c030aa9149cd9e5ca2c30b851` 已通过启动和描述符/根/CPU定着。 `continuous-stereo-36448-20261019-201128-8253fe51` 世界上有两支稳定的机动车1/ticket576, 犬类 `1617af7770`/generation151689。两个都使用了根 `71f3a1f0`,支持和平行动 `3dc3567a0`组合80x90x1; CB 语言0 registers5 分别是: `(640,720,0,0)` and `(640,720,640,0)`。缓存的阴影器又有 DXIL SHA256 `6d5c211bdf1bc2f5671a841aa1cb02fc49686e1f52420284932b22d396583c01`,因此其解码操作要求区分 640x720 区域数x0 和 x 键640。这是记录的阴影域证据,而不是 GPU 执行或像素正确性。

两个电话都有 TLS 时景索引1。因此,该索引无法识别逻辑生成的眼;逻辑域必须来自阴影常数和经认证的构造几何。7 贴上标签 `context_view_index`,明确不进行逻辑-眼校验,并将本地目标矩读作整数而非浮点。私有输入-矩形实验现在保留当前 Creat-Educ 输入几何学,完成仅读的 Epoc 样本后;每个构造 ed 票仍然验证新鲜的 Create 身份。 它仍然是默认关闭, 需要实际进行输入率较低的域测试 。

Constants4b, 5 和(c) 6 所有已完成的本地清理、独立条目恢复、游戏正常关闭和所有五个保护文件恢复。所有者退出1/ 不完整的出版仍然在可玩性- DLSS 接受范围之外。 已校验的收件: 忽略 `analysis/runtime/dlss-mv-constants4-to6-evidence.json`.

## 2026-10-19: 实际每眼完成 GetOptimal 和 GPU

Constants7 曝光无效的假设: 本地家庭质量输入为853x480 for1280x720 输出,所以将一个偶数家庭输入进行分割不能定义眼睛。8 b 查询经认证的本地 GetOptimal 帮助器,以了解实际640x720 整个 Creative 包件之前的 眼输出会消耗共享参数。 它返回427x480 optimal,320x360 最低和640x720 最大值, 结果1/quality2。完整的原始创建设置符会跟踪查询。构建器现在使用此收据,并检查当前每个票上的身份。它从不分割家庭的分/分或回合853 来让一个门通过。

实际常数8 b 届会 `continuous-stereo-39140-20261019-203316-80b3d4de` 已制成源矩形 `(0,0,427,480)` and `(640,0,427,480)`。选定阴影的 CB0 register5.xy 匹配427x480 and5 选定 x 的 .zw0/x640;两个调度员都是54x60x1 原始的SDK Eval仍在消耗中640x720 和MV比例表640/720。独立特性阴影现在将逻辑输入维度和浮点MV比例图都补齐。这些阴影在这种诊断中仍然不活动。

实际DSV资源与NGX深度输入不同。常数11 将可写入的 DSV 绘图区域完全返回 `CopyResource` 输入已输入的 NGX 深度分配,调用器 `base+0x1c5fe0`常数12 保留当前光栅绑定在真实执行返回后,并汇总重复的深度副本,保存不可变的证据,而不是耗尽当前绑定的空档。14 还将第一批Eval之前的3份原产地输出至彩色副本排除在Eval后两份副本收据之外;后来模棱两可的副本仍然被拒绝。

实际常数14 session `continuous-stereo-39332-20261019-213900-8d3a1710` (DLL `d0dbfb46d8ec612e32446d4a3d90023e5ab15a289ebf852e8b9a3cc32316cff6`) 记录 Evals 和 复制后都直接排在一条队列上。所选的颜色/深度/ MV 记录被加入到真实提交的列表中,没有光栅/ 深度容量损失。一个观察员拥有的队列 Signal 返回 S_OK , 并且它的真正围栏已到达 。1 。这证明所选已递交的工作已经完成;它不证明中间资源依赖性、阴影深度语义、像素正确性或可播放的 DLSS。原生清除、独立条目恢复、普通游戏关闭 并全部恢复所有五个保护文件; 所有人退出1 仍然为可玩性接受。接收:忽略 `analysis/runtime/dlss-mv-input-and-gpu-constants8-to14-evidence.json`.

NVIDIA's [DLSS 编程指南](https://raw.githubusercontent.com/NVIDIA/DLSS/main/doc/DLSS_Programming_Guide_Release.pdf),节5.2.8.1,指定在返回的包含分/最大间隔内推荐输入大小和渲染。适配器现在只接受同一观察眼查询间隔内按每盘实际输入大小,且颜色/深度/MV域相等,不切 ged 资源身份和输出几何。 大小转换重置两个历史; 拒绝右眼大小变化、 超出范围输入或缺失查询。 CPU 测试练习全尺寸启动、 减少输入、 稳定历史和允许的进一步还原 离子. 实际的每眼SDK集成和产生的像素仍然是单独的接受步骤.

## 2026-10-19:孤立的两个特点整合,尚未接受

`GEARS_NGX_ISOLATED_STEREO_TRIAL` 默认关闭,需要后窗/混合/观察唯一的儿童环境,并拒绝使用 XR 出版物或自有- scene/ 最终转让替换来构建。它连接了现有的完全的 Wrapper 适配器,两个独立的 SDK 特性, 键入四重奏, 框式副本和本地提交到当前相容生产者收藏器的退休。测试 DLL 保留了被隔离的所有人,直到其一次性流程寿命;没有此类文物是玩家的释放。

Constants18b,20 and21 每一个都做了两个实际的 SDK 创建,返回1 具有不同的非零控件。这是 SDK 创建证据,而不是评估或图像接受。常数18 b 拒绝在发送前进行初始评估,因为其四重奏观察开始得太晚。现在,收集器在同一消耗量中较早时保留了来自屏障的排入式全部缓冲资源,MV描述库可以从实际保留的夸尔特种子中种子 而不等待诊断结果故意跳过初始对。20/21 已到达输入回调, 但拒绝在绑定的表饱和度上。 常数22 调整精确生成的退出状态, 当队列提交运行在外部时会消耗 TLS。 它的实际游戏结果尚未确定 。

孤立的适配器在无法使用新MV生产时可以运行一个明确未发布的转换对,使用实际输入和本地请求的基数;它不会发明一个已渲染的MV矩形。这对无法生成正常输出收据 。 I ts 真实的 SDK/ 复制工作必须完成, 两种历史都重置, 并且一个新的完全观察到的对子必须在运输接受前通过。 CPU 测试检查持有、 重置、 拒绝输出接收和后续的全输入对子。 此选择输入不会建立 Firs t-pair 图像正确性.

每个眼睛都写着一个更大的本地产出分配的次区域。 `InEnableOutputSubrects` 保持质量和特色旗帜; [NVIDIA的多视图指导](https://raw.githubusercontent.com/NVIDIA/DLSS/main/doc/DLSS_Programming_Guide_Release.pdf),节3.17,则需要产出次区域的这种能力。0/xstride,输出基数保持为0,每个返回的本地副本被框输入到只该眼的颜色区域.

Constants15 c 在进程撕毁时产生了驱动程序访问权限的违反。 设定界限的离线垃圾将这一二次失败放在游戏退出后图形资源发布中; 它没有识别上游原因。 准确的崩溃测试过程及其WER 帮助器 w 出生/图像检查后关闭, 并且所有五个受保护的文件都恢复。 后来的失败的试验也恢复了受保护的文件; 没有失败的试验是可玩的接受。 阶段级 SDK 接收摘要被忽略 。 `analysis/runtime/dlss-owned-pair-create-evidence.json`.

## 2026-10-19: 持续隔离的每眼SDK运输

Constants22 第一次到达真正的 SDK 评估器和结果1 常数23 然后识别出错误的复制选择: 本地图重用相同的输出/ 颜色分配, 在下一张票的首张Eval之前进行预处理。 控制器现在只输入属于当前票/ 递归器的待发的Eval后复制件; othe r 原生副本未选中。 常数25 显示真实的计算调用和不断上传, 但无有效的 CB0 表格,因为诊断列表解析器会故意睡在它选定的窗口之外。26 独立解决实际输入的活式 CB 列表0 表格/ Distapatch 加入。 早期源代码分类账会故意省略可见的复制扇形, 因此初始对子仍然无法继承追溯性 GPU 表格证明; 之后本地副本会被跟踪到选定的键入的 MV 库中。

实际常数27 session `continuous-stereo-37960-20261019-231630-8613217d` (DLL `9dfa254af501274a88b266ed8ada6406cf70d68b12a94f471511acf420c4a9f6`完成本地菜单启用,a15 阶段性收据显示不同的左/右 SDK 手柄,真实接受的评价,427x480 完全观察输入,每个成熟票次有两个合格的MV区和一个当前缓存的遮阳器,对照已知的DXIL SHA进行核查256。最终收集器报告1226 已接受输入调回, 没有上下文/ 银行失效, 适配器相位被接受, 没有 GPU 运输失败, 没有保留录音和飞行批次。 这些是每眼 SDK/ 盒式复制运输观测, 不是图像质量或 XR 正常的出版接受.

普通游戏关闭,本土清理,独立条目恢复,没有拥有前台样本,所有五个保护文件都通过。本地菜单中完成的禁用AA;引擎保留了缓存功能控件,所以这不能证明有两个SDK R eleases. 像素回读和最后显示仍需要。28 仅在接受运输后, 并至少添加一个私有输出复制源重读32 完全输入对; 此捕获尚未被接受 。

### 2026-10-19: 第一个自有的像素回读( Constants)28d)

私人 DLL 莎士比亚256 `40ababb43d2d4f3d3fd4a26f4d833d5ea157f1590e310ab8bfdb04aab564d731`届会 `continuous-stereo-28724-20261019-233029-1462231e`,将世界的两个抓痕输出读回1/ticket571 已保存了 COPY_ UNVERCE 的原始副本。 640x720 区域处于抓取源0 解码 R11G11B10_FLOAT数据显示,第一眼的场景内容有限,非零,第二只输出完全为零;未使用的物理右半部在两个抓取中均为0. `analysis/runtime/dlss-output28d-pixels/inspection.json` 原始的散列物、精确的足迹和决定性的Reinhard/gamma检查视图。 640x480 输入夹; 单独控制的阴影使用认证的制片人的427x480,因此这些值不能被拼凑。TLS眼仍然没有得到这个观察者的验证;源底0 and640 加入可控的每眼命令。 尽管 SDK 成功, 但这是混凝土像素失败, 无法播放 DLSS 接受。 本地禁用菜单、 普通游戏关闭、 钩子恢复以及所有五个保护文件恢复完成; 没有自有的前端样本 。 Constants29 在实际的框式副本后添加两个有边框的色彩目的地回读,并带有明显的COPY_DEST-to-COPY_SOURCE-和背部过渡,以定位丢失的右眼内容.

Constants29 SHA256 `bda5ceee53fef050ddd11dfe45bc31d56230314252e9d819b8eff0ab4cdefe93`届会 `continuous-stereo-14420-20261019-233711-2edb3ed0`,被俘虏的世界1/ticket565。首个自带输出和彩色后左框复制为字节-相同,覆盖全版1280x720 缓冲器, 右半部分为全零。 在第二次 SDK 调用输出后是零; 彩色后右框复制件保留相同的一眼图像。 这证明右侧的颜色在 Eval 后的第一个副本中已经是零; 它不单是识别哪个更早的 p 屁股擦除它。 补充的 Color readback 使用每个副本的两个明确状态转换并恢复了 COPY_DEST。 原始输出元数据 `barriers_added=0` 只描述原始输出探测器; 添加了单独的颜色诊断2。然后,游戏意外退出(应用程序错误, at23:38:30,进程14420, nvwgf 数据2umx+1268f8/0xc0000005),因此本次运行没有正常的关闭/ hook- rerestoration接受。此抵消也出现在较早的二次撕裂断层中;没有倾销,就无法产生新的因果关系。游戏已消失,所有五个受保护文件都已恢复,没有拥有的前台样本。Extra extra 色彩回读现在是单独的默认私密选项。 constants30 使用当前打入的源头、实际的最后一个RTV绘图剪辑和构造的每眼矩形测试本地预处理区域-保存假说, SDK 写入无效之前的本地拖网区域; 输出像素回读仍然启用 。

Constants30 SHA256 `00a345c203bd40db73bea96fa06572d0a7d6bbae16aa6e3deb092dac8db845fb` completed15 s, 禁用菜单, 正常关闭和全部五个文件恢复。 原生区域实验框1,150 预处理副本并左575 未分类的完整副本不变; 拥有的SDK运输仍然被接受, 没有输入/排队失败。 same-ticet563 输出仍然有一个非黑第一眼和全零第二眼。这个假说没有像素接受,也没有被促进。31 SHA256 `81975d70b77d4d004744071970fb5aa134fa2df12a26743657af2ea565e19ac2`届会 `continuous-stereo-37956-20261019-234946-3d9ab246`,而不是原版预处理不变,而是读取其前两张输出式COPY_Source图像作为票面575。两个场景仅在物理起源时显示,大约427x480; 右半部分是零。 这可以移动SDK 之前的观测失败。 它不能证明早先的场景 Color 缺少右眼, 因为探测器对复制源进行了取样, 而不是目标复制前的复制。 常数31 completed15 s, 禁用菜单, 正常关闭和全部五个恢复。 常数32 b 添加了准确的目的地在复制前的回读。输出探测器现在在可能录制的复制前回调/原原作之前租赁隔离;离线回归检查不过早释放,包括来源地之前的例外a 收到,所有检查结果都以新鲜的32 b;游戏发布等待另一个所有者的现有游戏,在等待时没有更改保护文件.

### 2026-10-20: 连续像素诊断前提交序列

Constants32 b (撒哈拉)256 `8d52be074009ba94166f84de87f08a040fe47c3aa0941941b40bcca630e2becc`)在64-输入回读阈值, 因此没有执行 Color readback 。 新的已绑定的失败微量记录上下文无效 。 NAME OF Submission: 执行提交线程上的失败路径, 外部消耗 TLS; 这不能表示内部验证重 ason. 游戏退出后恢复了所有五个受保护文件。 源审查发现完整的 Creat/ Eval 包装适配器条目被参数 mutex 保护, 而Execute/ Reset/Copy 只使用帧 mutex: 合法的 RHI 记录和队列提交 c 也可以同时输入适配器。新鲜常数33 b 添加一个在外钩的帧记账前获得的单独的递归式 API 命令 mutex; 参数MetrometerScope 包括它, 而原始 SDK 转发仍然保留帧_ hook_ mutex 。 旧的只读观察器转发保证仍然被选中 。 A n 实际主机镜 CPU 回归 验证嵌入的同条线项, 将另一个线程排除在 API 命令锁之外, 转发时可自由帧变换, 并释放后。 NAMEGPUMission 现在保留了第一个用于诊断的失败源线 。而不是只穿布丁 新鲜的33 b 萨摩亚256 `ca3a8432788a7c52b26052636fc976fc24788d90c39ce949af9e2dcd3493df61` 已通过 已存在的构建/ 自测试和此新检查。 它保留了复制前/ 源重置实验; 锁定中不声称可播放的 SR 或 GPU/ 像素成功 。 测试33 (帧Mutex通过SDK持有)被任何游戏发布前现有的转发回归所拒绝,这种方法被取代而不是削弱回归.

Constants33 b 实际届会 `continuous-stereo-23028-20261020-000951-175ef904` completed15 s,本地AA禁用,正常的近身,钩子修复以及所有五个保护文件恢复,没有拥有的前地样本或运输失败. World.1/ticket562 第一个预处理源仅包含一个源427x480 现场,而目标颜色在第一个副本之前已经有完整640x720 图像和右半角。在第二个本地副本之前, Color 仅包含已缩小的第一眼区域; 右侧为零。 因此这两个完整的副本没有被确定为最早的右眼丢失。 绑定的当前堆栈区分 ori 通过游戏返回的 Ginal 复制路径11aca0 (callers119deb/11a5e3),2216d9 (caller11a302),并拥有后 SDK 路径be69 af(电话员)11a6e0),保存的历史伪码有ABI/deccological缺陷,没有作为经认证的活语义合同处理. `analysis/runtime/dlss-preprocess33b-evidence.json` 保留散列、 精确的重排读取元数据以及此接受边界。 常数34 移动较早的观察: 在 Color 实际 RENDER_TARGET 退出过渡时有两个回读, 使用该本地状态前合同, 在转发未更改的原始屏障之前恢复它。 源色和对齐的输出界面来自 ty 。隔板/四重奏租赁。 这种诊断每段选中的录音中增加了两个源转换, 仍然只使用背景/ 未公布, 并且允许单独开票, 而不给其贴上立体对 。

Constants34 completed15 s, 本地禁用和正常的关闭/ 恢复, 但选中没有 Color RTV 退出: 实际 4-to192 转换运行在外部的提交线程上 消耗 TLS。 constants35 使用该范围外的实际打入的时代租赁,以及单独标注的物理记录非ce, 从未编造游戏 FramleLink 生成/ ticet。 它观察到了附加的背面执行/ 符号/ 完整及其物理重置, 但错误的重置 退出了以后对 COM 列表的重复使用; 没有保存字节。 该观察器被固定在36 重置后只忽略了 AFTER 实际重置已退出捕获记录。一个CPU回归覆盖未知的FramleLink元数据,并使用后期列表重用,重复重置前拒绝,以及重置前拒绝。

Constants36 SHA256 `f4c943294a83c329521abc179d0e7ee4425d0ce40e98952a421eae3d10abcc5f` completed15 s, 禁用, 普通的关闭、 钩子修复和五文件恢复。 已保存两个实际的彩色 RTV- exit 快照( world/ ticket)0 = 与实体无关的无1/2,源状态4 恢复, 实际队列信号/ 完成/ 重置已验证。 两者都减少了来源427x480 内容和零右半边。 它们都是不同的物理快照, 而不是关联的立体对。 第一个右眼丢失点仍然在较早/ 未解析。 同一运行当前视图快照显示源427x480 和整数目标427x480 ,而预期的特性输出为640x720 取样工作仍与427x480 物理输入。常数37 测试一个单独的源/输出目标接合:只有未公布的、独立分配的源视图克隆人可能将其整数目标矩形改为640x720, 完全对象的回读证明其他字节不变。一个不同的选择输入环境标记门 。 此实验将保留正常构造和原始源/ 家族字节。 本地预处理保存实验失败 。 ,现在将选择单独进入,并默认关闭。

## 2026-10-20: 输入几何参考和装入像素失败

整数目标实验被拒绝:常数38 b 将本地 MV 发送域名更改为640x720 也因此,现有的制片人检查 正确地拒绝了427x480 输入声明。整数目标是一个输入域,而不是一个可独立编辑的最终输出矩。这个实验仍未结束。

Constants39b (`4d2724ef41345b554e4ef327896d81812ee19a292367c297d6dc01d8710f9305`) 建立了实际的未修改单引用: 原生创建853x480→1280x720,稳定浮点视门213,120,853,480,整数目标0,0,853,480 和 SDK 基数0. SDK 阴影和额外的每眼查询被禁用。它完成了本地的禁用、正常的关闭和所有五个保护文件的恢复。必须区分中心浮点视门和实际输入坐标。

Constants40c (`8b1650ee2788339f4b8122f0dda1cea4a0f8a29dcb4e00eb7a1832ba662eeead`测试包装426x480 输入值为整数 x0/426,带有浮点x213/639。在重叠前,将实际输入的GPU副本备份到右侧输入640- 宽左输出, 在 Eval 之前恢复右输入, 在右输出之后恢复保存的左输出。 CPU 像素/ 状态测试和刚刚通过的现有套件。 实际的同位素 Ket HDR 重置还验证了最终 Color 左键等于保存 le 的位元 ft SDK 输出和最终颜色右侧等于保存的右 SDK 输出。 然而, 正确的 SDK 输出和恢复的右侧输入都是零 。 因此, 包装和保存不会纠正较早的右眼渲染失败 。 证据和原始文件散列被忽略 。d `analysis/runtime/dlss-packed40c-evidence.json` 及其链接的像素检查。

The40 c 控件已完成但本地禁用遇到后键菜单快照超时; 清理不完整。 进程结束, 所有五个受保护的文件都恢复了。 不要给此正常的默认接受标签 。 没有最终绘图、 XR 图像、 可播放的 DLSS 或 phisica 接续 l- 头集接受。 包装适配器输出出版物在实际最终恢复线条连接之前仍然被明确封杀。41 在实际的右区域颜色/输出 RTV 绘图上只添加两个边框的后拖动像素探测器,以区分较早的场景落空失败和以后的后处理丢失;其像素结果在此不假设. Release/default DLLs保持不变.

## 2026-10-20: 减少输入的联盟罩可以恢复SDK双眼图像

Constants41 completed493 双对和正常的近距/重置,但其过于严格的视图/剪切平等没有选择绘图快照。42 检查它们的交叉点,并抓取一个右区域输出/色-色图;右区域为零。43 然后在每张SDK Eval 之前立即捕获同一票的场景/切换输出; 在第一个 Eval 之前, 右侧已经是零。 这些是有界限的观察点, 而不是最早丢失的证据 。 二者均正常关闭并恢复所有五个保护符 。d 文件。常数44,在SR已禁用且没有SDK所有者的情况下,读取两本实际的本地源代码: 两者均640x720 眼区域含有有限的场景像素,包括 a99.9996% 后期的非零右侧区域。这证实了潜在的私有双视路径可以使双眼没有SR。

现有 `view_union_mask_fix` 拒绝减少SR矩:其原始合同要求源=目标、辅助性和全物理高度。41 状态记录1288 矩形拒绝, 而仅是普通640x720 框被修正。因此,原生第一视图遮罩保留了缩小的第一眼界限。45 将已验证的构造器向现有框架票中浮动矩形。一个单独的私人门会加入当前 Create/query 身份、世界/台式计算机、已验证的本地渲染器存储器和确切的冷冻矩形,然后允许翻译/添加红色 uced 输入。 输入的浮点数 ABI、 物理视图、 非重叠、 查询范围以及 Stale/ mism 几何检查仍然严格。 CPU 回归既涵盖427x480 添加和426x480 中心包装的几何,缺失的收据和每个错配9个;普通行为不变.

Constants45 SHA256 `bd32cc5801db3d98a77a64e943e64e6172f0ed09752da9c0b22492072744570d` 使用简单的加法427x480 ,没有组合备份/存储操作。2268 接受/修改,0项拒绝;SR工会权利1067 即将发生的变动0.332812488 to0.667187452 全部948 输入调用被接受,具有明显的SDK特性,没有传输失败。同位计549 HDR SDK产出100% 有限和100% 非零像素, 明显不同的立体视图; 左输出在右复制前的颜色中仍然是位精确的。 这是 SDK 像素的实证, 即面具校正能解决本次试验中的右眼黑输入, 而不仅仅是成功的 API 返回。 原生 disa ble, 正常的近距离和5个文件的修复通过, 没有自有的前缘样本。 忽略了 `analysis/runtime/dlss-input-union45-evidence.json` 保存身份、文件散列、基线、状态和像素证据。

最终颜色 ATER 两种副本, 最终本地四重奏/ SRV 加入, XR 发布, 时间质量, 运行时间使大小和物理头集接受仍然分开。 旧本地生成的四重奏仍然为这些输入产生无效/ 减少的矩形 。ants46 保留45 校正并添加复制后彩色回读, 加上一个在 SDK 实际录入后绑定的限定读最后输出的样本。 这里既不假定其结果, 也不假设可播放 VR 支持 。 默认/ 释放的文物保持不变 。

Constants46 (`83c362fc1f8be303c856d06ca53ac18c36d02e54fd1bdd06d8e3d3f2d8bf55b8`),然后在两个真实的框式副本之后验证最终颜色:每个640x720 半个是比特精确到其相同的线程542 SDK结果,SDK两个图像都是100% 有限/非零。 以后票价为原始最后产出样本615 绑定相同的颜色条形分配( Name`3dc969740`实际绘图6,1。重要的是,在最后输出时,两只眼睛的清晰视图和TLS眼光Do匹配;在此不能概括早先的SDK- Wrapper TLS观测。生成的d83 cbf 四方仍然提供无效的未签名的目的地坐标和全数1280x720 源矩形。 正常禁用/ 关闭, 所有受保护文件都会被恢复。 忽略 `analysis/runtime/dlss-final-color-and-binding46-evidence.json` 保留确切的身份和有界限的本地约束行。

Constants47 b 添加一个单独编译的自适应器记录的颜色/方位几何缝隙,保留现有的视图/TLS、描述符、命名为SRV、本地列表和绘图检查。一个狭窄的CMake例外允许其输入屏障观察器与最终输出控制 新的CPU套件和减输出几何错配测试通过;47 Build曝光了断线变种固定功能,从未推出过游戏. Actually47 b 票价失败528 本地菜单应用期间, 在稳定像素采样之前, 允许使用; 设置在进程退出后恢复, 但正常的关闭/ 清理没有建立。 之后没有最终输出成功。 进一步的诊断结果会保留适配器的第一个失败行和 a dmission 相/ 完成. 进入前的守护人员在SDK桥架有效瞄准镜中也拒绝采样,等待复制,过渡对或 stale 输入票;这些间隔决不能仅仅为了要求快照而进入非留守适配器.

Constants48 c 加上实际本地R10 目标重回 ATER 已接受生成方程式绘图, 资源barrier 回调器获得真正的打字目标租赁, 源状态恢复和实际 Execute/ Signal/ 完成/ 重置退出 。 这只是一个诊断路径; 它的游戏结果仍然被消耗 ing. 默认/释放和XR-出版门保持不变。

Constants48 c 后在本地菜单启用时挂起, 且未建立正常的关闭; 保护文件在测试过程结束后被恢复。 constants48 d 从最终输出观察者中删除适配器/ API 条目。 实际右侧的框式回调冻结 `RecordedOutput` 使用已拥有的适配器条目。最后输出范围读取了在帧命令下且独立重验证当前构造、SDK Color、描述器选择的SRV、本地视图/TLS眼、命令列表生成和实际绘图下的不可变收件。没有 SDK锁被最终输出获取器收购.

Actual48 d 会 议 `continuous-stereo-1652-20261020-031409-731bf5be`, DLL 键 `70ad188e727f7b97bc8daae92505da48f275d73cdd6cce141b1feb737903fb30`,已完成244 守护着本土最终图纸244 输出退出, 不拒绝源/ 列表。 World1/ticket613 重新读取显示实际1280x720 R10 目标在每次本地绘图后; 在右边绘图后同时绘制640x720 一半的人拥有100% 非零 RGB, 带有明显的立体视图, 左半部分是比特精确到第一个快照。 这两个回读都是以实际执行方式提交的 。23547/member1 犬科列表 `161b4a0280`队列 `18667be0`,然后通过真实信号/栅栏完成/物理重置而退役。 `generation=1/2` 值是物理观察非ces,而不是不同的FramleLink世代。保存的检查PNG解码为 R10 直接使用 UPRM ; 其色彩空间尚未独立认证。 正常的 SR 禁用、 正常的游戏关闭、 五文件恢复以及没有自有的前端样本通过 。 忽略了 `analysis/runtime/dlss-final-target48d-evidence.json` 保存散列、 精确的资源身份和像素统计。 这验证了实际的最终 GPU 像素; 它仍然不能建立 XR 投影、 时间质量或物理头集接受度 。

默认关闭 `GEARS_NGX_FINAL_XR_TRIAL` 添加一个单独守护的模拟器唯一的中转路径。 它需要隔离 SDK 所有权、 最终输出加入、 游戏解析度和三个背景测试标记。 它复制了完整的最终 R10 图像在独立守护的右本地图后恢复本地RTV状态,保持私有的COMMON-idle纹理。出版需要两种相同的图案最终眼部证明、不可改变的匹配姿势、准确的实际执行成员/生成。d 相同的本地 DIRECT 队列/ 设备。 混合框批次、 重置、 缺少输出完成和不匹配的外观无法发布。 正常的微窗口互换链绑定只为私人试验而压制 。 默认/ 释放的文物和本地所有- 场/最后转让控制保持不变;实际模拟结果有待于作出。

Actual49 b 届会 `continuous-stereo-31952-20261020-033722-73e94713`, DLL 键 `56ebd815ea7023176101d1c4a59d041fb9f4a1091e2d06fcc6de247b48811c5c`,通过连续所有者检查。私有路线需要选定模拟器的显示,并保持连续跑者严格的操作 `RuntimeTracked` 配置( 位置) (`simulator_operator=0`),它不会放松启动跟踪检查.327 生产同样台式原生立体声片对327 最终R10 副本,所有副本均与实际执行成员相符,326 已成功发布( 一次工人准备跳过)。 没有对对、 输出防控、 源、 列表或模棱两可的批次拒绝, 没有SDK/ GPU 传输失败。 双眼输出范围完成654 XR 接收器已创建325 实际现场复制件和3076 预测提交数34.162044 秒; 提交材料包括保留图像重投影,而不是源帧率。40.108256- 第二个最大提交缺口包括初始启动/菜单等待和供应不畅通或无法接受性能。

4张接收方GPU快照的票710,749,760 and857 准确1280x720 R10 来源, 不同640x720 立体图像和100% 探测器RGB后, 两边的无零 RGB10-to-RGB8 转换。这些像素来自 XR 共享纹理,而不是桌面/窗口抓取。本地 AA NOE 导航、正常游戏关闭、所有五个保护文件恢复、发布者排水/关闭、独立钩子恢复和XR 关闭通过;没有自有游戏 预览样本被记录。 AA NOE 仍然无法证明本地缓存特性释放 。 忽略了 `analysis/runtime/dlss-final-xr49b-evidence.json` 默认/释放、物理头套、时间质量、较长期限和本地运行时间大小/自有场景的接受情况仍然分开。1536x816/ 周期外检查使用相同的核实49 b DLL 和测试另一个被忽略的运行时目录中的可执行文件.

要求采取后续行动1536x816 但游戏实际选中1366x768 中。只有实际的尺寸计数:在禁用前,两个接收器GPU快照都包含不同的683x768 双眼,双眼100% 非零 RGB,从455x512 输入。然后在重新启用时关闭/运行周期失败; 游戏进程30088 退出和本地清理/ 正常关闭没有建立。 进程结束后, 全部恢复了所有五个受保护的文件, 没有拥有的前台样本。 第一次运行时指令尝试从未推出游戏, 因为它复制了启动测试可执行文件 不存在;第二个目录保留了相同的 DLL 和所有需要的可执行文件,并配对散列。

实际例外流程垃圾堆(`150d30d24eaaed49ff78c492c625ab2d2d1ac8d75e7b446b6ddf7300b6ad623c`位置线程4628'读取访问权限的侵犯 。 `nvwgf2umx.dll+0x1268f8`,读取地址 --1。离线 DbgHelp 解风时使用捕获的内存和时间戳/大小检查的本地图像,排除磁盘 GOW 代码回落,并包含 `RtlExitUserProcess`/`LdrShutdownProcess` 加上 DLL 清理。 因此, 驱动程序例外发生在进程拆卸时, 并且没有识别主退出触发。 没有本地 PDB 的导出- 最接近的符号名称不是函数身份 。 在退出前, 队列钩头首先被拒绝 。 `complete_candidate_transport` 捕获线397;49 b 没有记录适配器的原始故障行或释放生命周期。50 添加限定的创建/释放生命周期收据和实际召回完成值,而不改变其退休前提,用于1280x720 关闭/复制。 `analysis/runtime/dlss-cycle1366-failure-evidence.json` 将初始图像成功与可重现和清理失败区分开来。重复的允许、完全本地特性的退役和玩家的释放接受仍未核实。

Actual50 (`16c1aaf06f5da43751d45a11d24e82af6207d06e5fe1ff3fce76e2fb81b6ce74`)确认原SDK发布电话已返回1 线程35852,使用实际完成设备 `18824fb0`队列 `71ecc9b0`栅栏 `3a7fb1d20`, 数值1563 公用把手随即变成0。一个普通的后期执行将故意退役的对子归类为运输拒绝;缓存适配器故障457 属于普通 `invalidate` 调用此发件人。 此集 `Context.failed`创建1599/revision3 被拒绝的阶段20 。在程序退出后恢复受保护文件。忽略了 `analysis/runtime/dlss-release50-lifecycle-evidence.json` 保持精确的顺序和原生结果。

Constants51 完全退出的运输只有在真实特性释放、创建和记录的GPU用途耗尽时才能被取消。该状态不能输出或允许另一个 Eval。一个后来完全本地创建可能启动一个新的四重奏 y 具有相同的完整原生田地/ 铺设、 世界和装置, 以及较新的源码修订; 旧的打字租约仍被固定。 独立拥有的中继纹理保留了自己的一代, 而其特征则发生变化, 没有待决的旧立体声 pa cket 和 无世界/ 外部/ 设备变化。 私有路线明确接受初始世界1 CPU回归检查 部分释放的对子 拒绝, 完全释放的对子报告 已退休而未输出, 以及下一个有效的时代重排诊断 。 新鲜的完整套件已经通过( )`e02b19bafdd3ab778e46b22539bde3433db52f0988ea176d3b31d87e0dbc6a19`默认/释放情况保持不变。

Actual51 通过成功释放旧的和 `create_wrapper_rebind` 进入时区3 罚单1647,没有错误的传输拒绝。但它的第一个替换 Eval 被拒绝了,因为会话已经进入停止。构造器在无 SDK 间隔期间继续使用旧的减输入 创建符; 已有的酸性 电子身份检查在后置替换时正确拒绝该符号。 因此, 此测试不接受可重设或正常清理; 保护的文件被恢复。 独立的默认关闭构建也编译和通过了选定的基线检查( Creative) 。`b890d9131c32018aeb2b028965e631c0149848b68bcf7dd21443ac21682e580b`).

Constants52 b 添加构造器活动策略, 与原始创建身份分开。 在注册立体消耗后, 以后的构造器使用普通几何学, 直到记录新接受的真对。 旧缓存特性已不存在 experienti jes 减少了在空白期间的投入。 重构此政策直到其替换对被接受; 旧的旧收据无法重新打开。 原始本地身份、 创建后期、 视图和每帧 GPU 出版守护符保持不变 。 空白观察 使用在原消耗前保存的不可变帧, 而不检查一个已经销毁的后置渲染器。 新的 CPU 测试覆盖了不活动缓存特性、 候选控件、 呆滞的收据和回收 。52 and52 b 通过了新鲜的全套套房;52 未发射,而且52 b 增加了第一站式诊断和不可改变消耗观察。52b (`178d011335f93da40f761de3f62464aedb20cb41174e9885e1df5edf89de5c9d`尚未结果。

Actual52 b 提前到替换输入回调和拒绝阶段37 。它不再停止在 Stale 减输入的构建符上。重新启用和正常清理仍然失败;所有受保护的文件退出后都恢复。忽略了 `analysis/runtime/dlss-reenable51-evidence.json` and `dlss-reenable52b-evidence.json` 保留不同的结果。默认关闭52 b 建筑物和基准检查通过(`c8a0eeb15e1cb51ccc724af38a113662a8891d4f6622a77584e0936fc3468012`).

Constants53 b 保持独立源描述分类账仅为这种私人控制时代模式的活性,将新的无人驾驶航空器录入范围缩小为全尺寸单米/样R16G16 本地输出几何后的运动对象。既不能修复源或选定目标容量/结构故障。明确退役的特性时代只能从当前观察到的源生成/复制中选择不同的输入MV, 复制器 Ted 强烈的重磅租赁和新的限值CPU观测范围;无法推断出一个未观察到的可见副本。旧目标库和MV租赁仍被固定,历史的第一-Eval摘要不能作为替换者SDK身份重新使用。 CPU regressive ver 表示替换资源序列、旧别名丢失、随后明显的可见副本观察和来自失败来源的拒绝。53 从未启动,因为缺少更新的转发声明;新鲜53 b 通过了所有套房(`daf181b15e2270a2ef15e2b7d94ecdf5371ab275228fb2e213cb7deef114bd08`)它的实际游戏周期待定。默认/释放保持不变。

Actual53 b 届会 `continuous-stereo-35548-20261020-045038-cc6fda91` 已通过初始启用, AA 无功能, 同进程可重用, 另一种 AA 无功能, 本地钩子清理和普通游戏关闭。 旧的真功能对返回1 原版,替换特性时代3 接受了新的四重奏和MV描述范围,独立中转组则保留1。现场源分类账18 单元格没有丢失;所选替换目标也报告没有结构或容量故障。222 原生立体声对和222 实际执行匹配220 GPU 出版物( 2 个明确跳过), 没有对/输出防/ 矛盾- 批次拒绝, 没有 SDK/ GPU 运输失败。 两种本地输出范围都完成 。444 times.

接收方 R10 重新启用前后的快照1280x720 大小和独特640x720 双眼,双眼100% 非零 RGB 。 可复制的快照是票1544/serial159,保存于43276945028000 第二次启用完成后ns43271752615300 ns;其票价晚于新创建/重置时间1403。这是一个新的替换时代图像,而不是保留下来的旧框。视觉检查发现既没有黑眼圈又没有包裹/封存源头的完整视图。XR 所有人录制了216 实际现场复制件和2425 投影提交,成功启动到投影交接并清洁关闭。121.424344- 第二次拥有者范围和94.1740351- 秒最大间隙包括图形菜单和无 SDK 间隔;既不计数也不计间距,都不是性能/平滑结果.

原生清理、独立输入恢复、出版商排水/关闭、正常流程退出和所有五个保护文件恢复通过,没有拥有的前置样本。忽略了 `analysis/runtime/dlss-reenable53b-evidence.json` 保存精确的二进制/ 文件散列、 生命周期收据、 实际执行和 GPU 图像证据。 这可以完成对测试的未处理模拟器的校验1280x720/Quality2/world1 私人审判仍只使用模拟器-操纵器和工艺,最多可进行8次相同放电的娱乐。默认/玩家-发射器更换、物理耳机、UI完整性、时间文物、长期表演c e. 此项结果不接受其他质量/大小变化、本土运行时间-移交规模一体化和世界交接。


### 2026-10-20: 运行时大小的本地拥有场景整合候选人

`GEARS_NGX_OWNED_SCENE_TRIAL` 单独缺省候选人,与私人R公司互不相干10 中转/XR 路径。 它需要正常运行大小的本地拥有场景/ UI 管道和孤立的 SDK/ 最终输出加入。 RGBA8 最终RTV只有在实际活动本地所有者资源、渲染器、世界、票和计划大小的情况下才被允许;任意匹配大小的RGBA8 分配不足。两个完整的本地输出范围必须额外授权自有场景/UI 发布。这不会推动候选人进入玩家的显示。

第一次 第一次 第一次54 构建编译但失败了任何游戏发布前实际的系统钩子安装自试:自有输出和NGX独立安装了复制资源和执行Bundle detours.54 b 使用每个ABI一个钩子,同时保留观察者,原声计数,绕行和未知用途的语义。它的全部启动和定向套房通过(b)`e8d47c9e91a3feb159db411d3a00edfd2c2f6d9269c06c00e2c5a34e9831835f`; 它从未推出过游戏, 且它的等待帮助器在保护文件写入前被停止了。 一般玩家构建选项也通过编译/ 完整启动/ 独立的 CPU 测试( ) 。`ebc6d1ee7321d61ca519cd652f767bd1d8bc5be91a7a256cf8645c1d11ca5e92`。其已禁用的 NGX 描述符模式正确返回 EROR_NOT_SUPPORTED;这不计算为描述符测试的通过 。

54 d 明确完成 DLSS 的消耗证明,在原本地消耗返回后, 在自有的场景结束可能发布之前。 毁灭器仍然是倒置, 并精确完成并主动恢复。 实际的包装回归需要完成 。e 拒绝公布、接受已完成的证明并核实明确完成/重复完成/销毁生产完成一次的证明。3米,所有输入的GPU资源仍须同意.54 d 运行时间接收等待。 现有的游戏/ 启动器在唯一背景测试服务员等待时仍未被激活; 该服务员不会关闭当前会议。


Actual54d (`1fa13c246b21cf39df8fff1368d4aa80d665da4175ee3b1ad7b1fcb893030e7e`),会话 `continuous-stereo-21376-20261020-051401-323ae91b`,使用实际运行时间计划1792x980 (896x980 双眼,双尺0.7。在 DLSS 之前, 已消耗的自有场景/ UI 路径489 门票和已出版488 图像。两个真实的 SDK 特性随后都成功创建597x653 每眼最佳输入,创建后输入/复制回调运行。在票位上499 默认时已观察到较早的UI 校验停止; 正常的近距离/ 本地清理和 DLSS 图像后接收失败。 所有五个文件都恢复了, 并由下一个独家确认 。55 备份快照。上一周期状态在拒绝之前,不能作为干净的最后样本显示。忽略了 `analysis/runtime/dlss-owned54d-evidence.json` 保存此分隔。 普通玩家选项54 e 建造/通过全套套房(`c61a2f9b913530a54ac0ccf105bad935270b0aac4ecfb46e16f575abea7533d3`).

Actual55 (`598b9beced93f65356fe98d8cfa47695a5b7da5acb38b085f8ca92759f6dff6e`)转载失败,并捕捉到第一次确切拒绝:拥有UI票514,原因9/draw_ state, 本地绘图调用器172741 实际和预期资源199badb0,1792x980 旗帜258 指正确单一目标并观察到的覆盖链,但没有连接的场景对(pair picket)0。HUD计划没有尝试,所以这不是HUD投影拒绝。DLSS输出控制器记录了0个已接受的最终四重唱/画幅/镜像。所有5个受保护的写/读回检查都报告恢复,没有拥有前缘样本;正常的关闭/n tive 清除和 DLSS 像素仍然失败。 稍后的游戏所有者在清理后更改了活的设置/ 配置文件, 因此没有尝试纠正写入或独立进行后续比较。 忽略了 `analysis/runtime/dlss-owned55-evidence.json` 包含准确的诊断收据。

56 (`5f5d1dcd646212997e6ef68bbc04df5542d10e453df964839977e771c5c25604`)通过新鲜的全套房。它只修改了被守护的自有候选者的诊断:在两个被接受的输入后,对第一个完整的后波斯特拉普输出路径进行抽样,并且每个边框行仅在本地输出返回后从自己的TLS调回中写入. 没有同时添加同步的直播记录器读取,几何授权或GPU证明. 背景服务员保留当前单独的游戏/发射机,仅在独家闲置检查后开始.56 路径证据待决;玩家名单保持不变。


57 (`01fc59254e739abdf3f48ef87b0f6c214e46f538ed8a72629b3548bf5f34626f`替换56 。它从同一 TLS 所有人处另外导出已定义的已完成行的实际参数描述符/负载,并命名为 SRV/ list 观察。完整的启动和定向套件通过。56 正在等待帮助31068 仅在验证没有驱动日志/ 启动后停止, 所以56 从未写过受保护的文件或发布过游戏。当前57 服务员是进程31944,创建2026-10-20T05:28:46.108843+08:00; 其实时工具会话62324 仍然等待单独的游戏37372/owner19380. 仅国家档案不被用来将这些过程视为停止。

有待进行的整合审计还要求对SDK未发布的转换对进行明确的自有出版处理。 私人最终-XR路径已经要求接受记录输出,而目前拥有的出版物也接受输出存储。 no_site for real AA-off frames. no_site 单凭此决定不能授权 SDK 靴子陷阱图像; 当引擎保存缓存特性时, 这必须与实际的无 SDK 空隙区分开来。 这是源级接受漏洞, 而不是证明某个特定 提交拖曳图像。 在验证此区分和实际的分支/ UI 图像之前, 不推动候选人 。


Actual57 (`01fc59254e739abdf3f48ef87b0f6c214e46f538ed8a72629b3548bf5f34626f`) 复制UI 绘图_ 状态拒绝票514 而不抓取已准备好的输出行。因此它不会提供最终分支/UV结论。保护文件的写/读检查在程序结束后通过;没有自有的前端样本。忽略了 `analysis/runtime/dlss-owned57-evidence.json` 保留确切的收据。

58 (`087381c54bce24a67c96d2da83cde65365315b9eeb3e401ff121c4818a81e6c0`)添加了明确的SDK出版政策,并通过了新鲜的全套件;它从未推出. CPU回归覆盖基线无SDK框架,缓存版空格,未发布的第一对,待发副本,错误的票和替换时代. 控制下的SDK框架添加了 线性要求最终输出商店的完整站点证明; 仅对普通的无 SDK 框架来说, 仅靠网站仍然有效。 player- option/ default- off58 b 建造/全套套房也通过(`efecbf69638a0589a1ef440a09937cbeb13b8a6870dbcaf668c41082a5834504`).

59 (`7e837430c500558256f1e1caeb0694789237856149c383e00a313063f8dd8753`) 将丢弃扩展至GPU接受等待时的第一个真正的候选对, 如果已有的实际栅栏允许调回进入接受的阶段, 则同步外传缓存。 它还允许不等待 tr 的早期诊断观察 。接受体育;观察不允许出版。通过了完整的套房。实际会话 `continuous-stereo-24280-20261020-054253-1256015e` 记录了两个候选弃物(表)545,546),每个明确提交_to_publisher=0,然后 UI 绘制_状态拒绝 。547。没有完成合格的输出行或录入-cache同步标记。因此,缓存时间变化没有被确定为主要原因。SDK两次呼叫票547 仍然返回1 。恢复了5个保护文件,没有自有的前端样本,没有正常的清理/关闭接受。忽略了 `analysis/runtime/dlss-owned59-evidence.json` 维护这些区别。

60 编译/通过但从未发射。61 在共享自有产出观测接合点添加一个范围限定的实际拖动诊断,涵盖R10/RGBA8 目标,以及多达8个未知的绑定状态, 不仅记录了预期拥有的目标。 它记录了实际目标相对于所有者, 当前相对于最后一张SDK票, 最终/ 季度元数据, 跟踪失败和有界限的本地堆栈。 这可以区分错误的几何/ 调用 r, 不同的输出目标, 缺少命令列表状态和丢失的线程本地票务关联, 而未扩展任何绘图/ 出版物上游。 它的构建/ 运行时间证据有待 。


61 新的全新启动/目标套房通过(`e1f5d018ea4df26af7d141e3b81feacef28efc6941fc6fa910a68e85c126d719`),包括更新的候选出版物政策回归。它尚未启动。等待帮助者39860,创建2026-10-20T05:48:33.814298+08:00,在工具会话中实时15244 并等待单独的游戏34948/owner32544。其驱动日志尚不存在;否61 保护文件写入工作已经开始 。60 无法发射。待发的跟踪只是观测;当前的几何/出版物上游保持不变。


## 2026-10-20: 拥有的 RGBA8 最终输出遵循 DOF 渲染器

62 superseded61 (a) 发射前;61 服务员在司机日志或保护文件写作前被截住。62 通过了全启动/目标套房(`402a036be5860e734c059b569c0e347a86138d157987211f1249b6242e51f7b1`实际会议 `continuous-stereo-39852-20261020-060121-5640c6df` 重新丢弃了两对未发布的 SDK 候选对, 然后失败了本地菜单的转换 。 测试游戏结束后恢复了五个受保护文件; 没有自有的前置样本, 正常本地清理或 DLSS 像素接受后。 启动器级的电影内容 。der身份误差位于渲染器故障的下游,不能确定其原因.

实际绘制的观察者捕捉到两个过渡眼 绘制到精确的主动拥有的 RGBA8 目标在1792x980,使用本地绘图6,1 caller1af92 b 和四位呼叫者d808d8。本地堆栈包括808d8/2b0 bdb. 这些是靴子图案,不是成熟的SDK输出接受;48- 排行诊断库随后在中间RGBA上填充8 目标。没有看到边框钩子实际自有目标副本。忽略了 `analysis/runtime/dlss-owned62-evidence.json` 保留准确的收据和限额。

在一个私有历史运行时间数据库中,一个有界 KB- 第一个 IDA 输出可识别 DOF 渲染器 atd7fd14..d80b75,原生绑定返回7ff84 (callee1665 dc)和顶点固定/整对8084e/d808d8 andd80925/d80a7 b (较佳)1760b4。全函数散列是 `62ceae022db3ea1d7fd0994520e2cbff6c1b1dca288023d1a56bd957fbca7d5c`,源代码1 SHA `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`基数140000000。这些是历史运行时间的事实,而不是磁盘-EXE或活字节等同。私人开发协会的租约已经关闭,源数据库散列保持不变。

有待处理的私人案件63 候选者在安装其包装器之前认证完整的DOF活性函数,保存完整64-bit RAX返回,并通过现有的SDK记录输出证明来引导其命名的颜色约束/顶点设置/最终四位。它还需要在实际四位中提供与现有VR DOF零恒定证明相同的盘点。丢失的SDK收据仍然为 Ordi 无效的无站点通道; 支持的顶点描述仍然被拒绝。 实际的UI 发布仍然需要两种完成的本地范围。 诊断为自有目标保留配额, 而不是在中间目标上耗尽所有行。 CPU 包装 an d 仍需要真游戏/GPU验证;没有玩家显示或发布DLL的功能被推广.


63 c 刚通过的全套新套房(`24bca123ceb835f7dbe079a571f62b7bed5d935f4cf9d4b2c3b232d867776ebc`更早点63/63 b 构建失败的编译时间线程/ 固定定义, 并且从未启动。 普通的玩家- 选项/ 默认关闭63 建造/通过的全部套房(`b0bda82664ea836fe177baa3154d563989aa8b6a0b786155941a048bfae65d35`) 单独核实的离线DOF控制器,然后通过双对固定/方对,减少投入到全输出几何,完成双眼出版,以及九次拒绝,包括非零/丢失DOF证明,错误绑定/场地/目标, Stale/ missing SDK 矩 eipt, 错误的颜色和未调色调。 它的 SDK 和 DOF 的连接是合成的; 它没有建立游戏/ GPU 行为。 请参见忽略 `analysis/runtime/owned-dof-control-tests.json`。不可改变63 c 游戏 DLL 正在等待独家闲置;后期的单独游戏保存.


Actual63 c 届会 `continuous-stereo-34712-20261020-063031-663331ff` 通过了全程实时的DOF验证。两个观察到的DOF线路都使用精确的无顶点描述ff,ffff,0 和实际命名的输入调用器d7ff84 带有选择的0/alternate0. 成熟SDK票540 已接受运输和记录产出; 实际发送d808d8 本地绘图使用896x980 双眼代替597x653 车票541 切换到第二个本地对齐80925/d80a7 b: 两种转发的实际绘图矩再次正确, 但最终_ eye_ valid remain0 因为共享分类器/ hook 闸门只承认第一个 DOF 二次曲线。 UI 发布失败; 最后一个周期本地状态是 stale 。 所有五个文件恢复, 没有自有的前景, 没有后 SDK 像素或正常清理被接受。 忽略了 `analysis/runtime/dlss-owned63c-evidence.json` 保留实际目标/列表代数和准确的失败边界。

64 额外承认80a7 b 至 最终视线分类, 只能使用当前控制四位。 它的任意/ 未控制的呼叫仍然不合格 。 常见的 DLL 自我测试涵盖左/右的分类、 缺失控制、 无效的票和双控的全宽拒绝 d 站点。 保留了现有的自有目标/ SDK 绑定/ 列表/ draw/output/ UI 证明; 这不是一个无条件的呼叫者白名单。 实际64 验收仍未进行。


64 完整的启动/目标套件,包括真实的捕获分类器回归和单独的自有DOF控制带,通过(`aefb0617c31b8db54f94097e4f76f74726e553450354b8e4b831afaa93d87962`。普通玩家选项/默认关闭64 建造/全套套房也通过(`28921a4ba65d7e2b1973f97b784fffd37cceb4ccccaa022252052466cd822726`。同步键63 c 首条错误线独立确认自有 UI 绘图_ 状态理由9 at541,缺少链接对0,实际=预期拥有的目标;本地定期计数器仍然呆滞。64 仍在等待单独的游戏/所有者。这些CPU结果没有暗示会进行生产表征升级。


Actual64 session `continuous-stereo-33720-20261020-064432-18049c13` DLSS 启用前已停止, 因为本地菜单发送更改了注册, 发送后观察在注册外提升对象。 正常关闭、 本地停止和五文件恢复完成; 没有 SR 接受。 独立条目验证初始化 y拒绝91 独有的钩子在旧的87 核查人现在使用102- 最大距离来自当前捕获能力,仍然独立检查每个条目,并验证全部3681 B 拥有的国防部机构,使用固定的历史散列。24 验证测试合格, 包括端点和短读的腐败。 成功的钩子盘点不是一个字节豁免 。

第一次相同的 DLL 重试, 会话 `continuous-stereo-33424-20261020-064758-7c981c44`,通过所有本地身份检查,但 prepare_trial 在安装场景钩之前拒绝使用现有的 ngx- 同化-trial- events.txt。它没有测试 SR。 它的自有游戏正常关闭, 并且恢复了所有五个文件; 不完全的激活/清理被保存了 。sse. 下一个队列重试的档案, 在一个前事件文件只在所有实际游戏进程和启动器所有权缺失之后, 保存其散列。 它保留了本地的拒绝覆盖守护符和相同的永久保存符64 DLL. 运行时间验证尚未完成 。


Actual64 retry2 届会 `continuous-stereo-37488-20261020-065312-d4a544dd`, 已完成本地 DLSS 菜单启用, 返回到游戏游戏并保存。 实际的第二个 DOF 分支现在记录了正确的最终眼特性和896x980 rectangles.822 输出范围/绘图411 SDK 立体声对。 XR 接收器快照1 票号678/serial654,是实际可启用的1792x980 RGBA8 框;两只眼睛都有100% 非零 RGB, 以99.9019% 不同的眼睛像素,检查显示完全不同的游戏玩法视图. PPM SHA `fb4f8d88eb38285b678d9329a1859bbf1f6edbc6a632bdaf63cbbd5ea5899905`; 忽略 `analysis/runtime/dlss-owned64retry2-pixels/inspection.json` 记录准确的时间和限制。这是第一个可启用的GPU证据,而不是完整周期/头集接受。

返回原已禁用0,但拥有 UI 绘图_状态理由9/pair0 罚单947 无法重现前停止生产。 没有记录 DLSS 输出发布拒绝; 由于初始诊断库已耗尽, 尚未捕获该缺口的源/ 目标和分支元数据。 正常游戏关闭, 所有五个文件恢复, 没有自有 d 事前经过;未接受原地清理/入口恢复。65 添加一个单独的八长诊断库,用于实际自有目标绘图,其票面遵循最后观察到的 SDK 输入,保留当前 Outlook 条目的 POD 视图矩形。它不给漏洞/ 发布权限,也不推断 SDK 空格 由抽取时间观察。

私人菜单探测器现在也只当发送后读取的准确对象在注册外时重建一个 stale 读取器。 它从不重发密钥; 在/ 发送前, 身份/ 窗口/ 未知的失败仍然严格。 有五个测试涵盖恢复, exha 使用和改变身份。这些管理器的改变不能确定 GPU 的成功 。


65 通过的全套套房(`bc1edc8d87e6030e2cde3e7ee6858d82c8e240cb23635ad8d681b7c4118fb80b`实际会议 `continuous-stereo-30212-20261020-070403-af3bc66e` 重新通过启用/ 持有, 然后复制禁用转换失败。 保留银行抓取953 and954: 上次 SDK 输入952,没有当前 SDK 输出接收, live 本地源=目标597x653 带有加法源896,实际最终808d8 画完就动597x653, 最终_ eye_ valid0。构造者资格的改变1 to0,但已建成的第二个渲染器仍然减少了输入。Epoch 仍然活跃/已锁定。这些是绘图观测;在消耗完成时将全程计数符的缺失单独确定。游戏关闭/五文件恢复/没有通过前缘;全周期/ 内部清理没有。 忽略 `analysis/runtime/dlss-owned65-evidence.json` 保留准确的行。

有待处理的案件66 执行为当前已注册的渲染器/视图收集一个单独的本地输入接收,这是在较早接受的 SDK 输入后。它冻结了框架构造器元数据中的两个源区域,或者需要完整正常的本地布局,然后是压制。实际命名的 HDR 源、 同位目录 零 DOF 证明、 拥有目标、 列表生成、 绘图、 本地返回和用户界面检查 。 原生的空档区将当前输入区域映射到完全拥有的眼睛中; 它们从未以 th 重新使用过旧的 SDK 输出 。e 当前图像。 消耗返回后, 拥有的出版必须独立分类实际的无 SDK 使用。 每个眼睛还记录其几何是否使用了当前 SDK 接收, 因此本地/混合眼证不能授权 SDK 拥有的 DOF 出版物 。 pure/ wrap er测试覆盖了两种路线的减少/全部输入,改变了当前本地的颜色, stale receive, 0 proof, caller/目标故障,形状不整齐的几何和混合SDK/内置拒绝.66 通过了全套套房(`4780d06fecda816fa155cb0db0a1dce712b04f2043eb3b270728b5cbebbf60d6`从未发射;66 b 刷新只诊断的打印,并在重新启用前额外请求实际的状态外接收器快照。运行时间尚未接收。


66 b 新的全新启动/通过的目标套房(`6ecdac955b55718177eb0b51ca507814a1b29c0c9902c2838e338da8a66f885d`。普通玩家选项/默认关闭66 建造/通过的全部套房(`a4737a6cbdf38abf0d9559a6b40e697cee21932528491e14493448557d2fa4c0`) 两者都包括了本地减/全输入和SDK防分离的扩展实际控制器固定装置。玩家的列表保持不变。66 b 所属的背景服务员是进程29320,创建2026-10-20T07:26:06.997439+08:00,工具会话19686; 当前游戏33516 是一个单独的会话,并且没有变化。66 b 尚待接受。


Actual66 b 届会 `continuous-stereo-39764-20261020-073018-4e81804c` 已先通过但再次被拒绝的禁用框架946 (最后一张SDK945) 本地获取和命名绑定是有效的, 而四重奏在实际绘制认证前失败。 源审查确认一个执行错误: 只创建布局会故意将所有物理范围都留为零, 直到 Eval; 本地_ 输出_ ticket r ead that 0 字段。 固定从已保留的打字的颜色描述符中得出了预期物理地形, 并对照最后被接受的制作人颜色身份/ 标记符进行检查。 当前本地阴影器绑定仍然独立读取 。二次/拖曳并可能是新的本地颜色;没有旧的SDK像素被重复使用.67 b 测试明确要求物理范围不明,拒绝几何,然后承认保留描述符,并拒绝7种地形腐败。 单独的限定直线本地退出收据保存了随后发生的任何失败,而无需在f中提出额外询问。ormatter.67 仅在测试中编译失败, 无法断言宏/ 模板逗号, 从未启用; 新鲜67 b 已通过的全套套房(`f2bc88d46badc9e65d3ac6e79efe6c81702c5b8d168b4d8a17324c0512aa79d7`),全周期游戏/GPU接受程序仍未完成.66 b 关闭了自己的游戏,并恢复了所有五个文件;本土清理/输入恢复接受度仍然不完整。


Actual67 b 届会 `continuous-stereo-35424-20261020-074648-02e8c47f` 验证先启用和禁用渲染。禁用框架983/984 retain597x653 建构区;其原生源和物质1792x980 收货现在都完成896x980 最终的眼绘、装订/修补/画图已证实且失败0.985/986 恢复全部本地输入。 在此空白中不会发生拒绝用户界面的情况。 接收者快照2 ticket1117/serial1071 (saved53792995433900 ns)在禁用后显示完整的本土眼睛,100% 非零 RGB; PPM 空间和空间 `09056531de673bcdedeeeeaeac3186875c167241d556f51b0e17b1c2ecee3dd8`。快照1 ticket706 验证第一个 SDK 可启用输出。 忽略像素/ 证据文件保存准确的时间/ 间隔 。

无法在UI 中重现1422 实际原生的旧配对释放,并创建重置为修订版3/MV 重置成功; 第一次当前 SDK 输入是未发布的转换。 它的最终路径没有接受 SDK 输出接收, 而早期的本地采集器需要事先输入票。 由此导致的空景对在 th 之前导致 UI 拒绝 。e 现有的后弃置政策可以使这一过渡退休。1423 下游( 停止)1,源代码为自有_ui_观测者:61),而不是初始的制作/SDK失败。恢复了5个受保护的文件,没有拥有前台,但游戏/清理接受度仍然失败。

68 收集当前未发布的 SDK 构造器区域与接受的 SDK 输出分开, 让真实的场景/ UI/ 队列验证完成, 然后总是丢弃该图像。 它的证明仍然未公布, 即使 GPU 接受在范围中取得进展; 本地 或 SDK 出版物无法升级。 Peree 拥有的时代身份也防止了混合时代。完整的渲染可验证证人加入当前 SDK 的票/时代的出版政策,因此缓存更新后不能发布与候选者 g 绘制的帧 几何学。 重新组合诊断法保留了每节限值娱乐的前八幅自有图。 CPU固定装置覆盖了全部/减少的候选者、 延迟混合 SDK 准备状态、 错误的时代和缺失的复制/ 盘片拒绝 。 新鲜的全实验性68 通过的套房(`2496bc56e8f87c34ceb1632d79cf40ce4739ecf4c9e2036508ddc8beb47f7669`; 普通玩家选项/默认关闭68 通过的套房(`b06b1e0de270f2585915d370b658a902cd812f6cdfc995cc735ab19382ae35bf`运行时间68 接受程序尚未确定。玩家显示的状态不变。

## 2026-10-20：owned 68 实际完整开关、GPU 图像及退出通过

上述待验证项由 session `continuous-stereo-39412-20261020-080820-5ddf8f7d`
完成。游戏 PID 39412，创建 FILETIME `134369284771875483`；实验 DLL SHA 256
`2496bc56e8f87c34ceb1632d79cf40ce4739ecf4c9e2036508ddc8beb47f7669`。
磁盘 EXE SHA 256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`
与历史运行时快照 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`
继续分别记账，基址 `0x140000000`。四次原生菜单操作均返回 0，完整开启/关闭/再开启/再关闭完成。

首次 SDK pair 在 ticket 532 建立。关闭后的 942/943仍保留 597x 653构造区域，两个真实最终绘制
均有有效 native-gap、绑定、setter/draw 证明，failed 0；随后恢复完整原生输入。
再次开启在 1341完成新的 SDK Create：两个新 handle 为 `1614d79370` 和 `1614d81330`，
与初始 `1614d89940`、`3dc2e58a0` 不同。原生 Create revision 3 与观察器 epoch 2
是不同计数域，不混同。1341/1342以 unpublished_sdk 1完成场景/UI证明并被丢弃；
1343/1344的左右眼均恢复 sdk_output 1/native_gap 0、实际draw 1、binding_proved 1、failed 0。
首次启动 532/533与重开 1341/1342共 4张过渡图像均明确 `submitted_to_publisher=0`。
最终 2450个 Output scope全部具有实际draw及return完成证明，publication_rejected 0，
用户界面缺陷 0、owned_error 0，SDK owner/gpu failure均 0。

XR接收端保存的是实际 1792x 980 RGBA 8图像，每眼 896x 980：

| 状态 | 票/序号 | 保存 monotonic ns | PPM 诗人协会 256 |
| --- | --- | --- | --- |
| 首次开启 | 682 / 660 | 55032645770800 | `68d86c9e3a7b9f039c001bb40f026e806e1ae0a9f52e7dc3323d8ff203d2c46b` |
| 关闭后 | 1056 / 1014 | 55083205789000 | `425b56afbfdc5ce926c686e5d243b0873972d55781a9a3ae1f04b5d9952b72c2` |
| 再次开启 | 1472 / 1404 | 55138535361900 | `b439143bb1191e23a205337f163e586f622f70ff1aa7c543be816c1ab43e7cf3` |

三张左右眼均 100%非零RGB，逐像素左右差异约 99.90%；这些数值只是辅助检查。
协调者逐张查看PNG，确认完整庭院/建筑/人物双眼画面，没有单眼黑屏或低分辨率画面只占一角。
关闭状态边缘锯齿更明显；静态图不能评价运动拖影、历史稳定性或真实性能。
时间戳明确位于对应操作之后，重开图 1472也晚于SDK恢复的 1343/1344。

会话正常停止，native_cleanup_verified 1、entry_restore_verified 1 QQR 退出 0。
独立验证 100项范围/入口字节全部匹配，其中包括 owned DOF完整 3681B历史范围的固定哈希，
没有扩大可接受字节或跳过不匹配项。随后正常关闭游戏。驱动在 08:12:22写最终报告前，
逐一恢复并回读校验全部 5个设置/profile文件；没有焦点请求、窗口移动/缩放或物理控制器输入，
自有游戏实际前台样本为空。08:12:36启动的另一个游戏PID 36544不属于此测试；
后续文件比较中的变化属于更晚的会话，未覆盖其文件或关闭该进程。

精确身份、原始证据哈希、菜单操作、丢弃和恢复记录在 ignored
`analysis/runtime/dlss-owned68-evidence.json`，图像及统计在
`analysis/runtime/dlss-owned68-pixels/inspection.json`，选定源码的事后哈希审计在
`analysis/runtime/dlss-owned68-selected-source-audit.json`。事后审计不是完整的编译时源码快照。
原生/SDK历史观察器的零值诊断并未被追认为全局画质认证；本轮接收端图像及实际scope独立记账。

本结果完成隔离构建的无人值守游戏/GPU开关验证。默认玩家manifest仍指向原
`build-mounted-fps1`，DLSS锁定关闭；没有把要求后台测试标志的DLL直接替换为玩家版本。
实验限制仍包括world 1、最多 8次同布局重建、SDK资源/feature保留至正常测试进程退出。
物理VDXR/头显、跨世界、预设DLSS启动、运动历史画质、Frame Generation及性能验收仍独立待做。
会话 216.38秒，1706次场景复制；18026次投影提交包括旧图复用，不能用作渲染FPS。
