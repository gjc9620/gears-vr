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
