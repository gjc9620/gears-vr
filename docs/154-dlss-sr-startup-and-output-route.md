# V1 DLSS SR：隔离启动与输出分支

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
