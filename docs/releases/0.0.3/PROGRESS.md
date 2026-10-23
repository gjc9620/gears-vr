# GearsVR 0.0.3 发布记录

## 2026-10-23 — 版本调整及分支切换

用户要求把原计划0.1.0的同一套功能改以0.0.3发布，创建并切换至 `codex/0.0.3`，随后删除 `codex/0.1.0`。分支切换前后HEAD均为 `315473dca5008e753011ae905783079de6f04770`，154个已有工作文件逐文件SHA256保持，旧检查点 `963d965b8590555bae31d047bef6ccc27d3e6469` 和两条提交历史保留。原0.0.1、0.0.2标签／发布包及 `codex/0.0.2` 不变。机器回执在忽略的 `analysis/github-publish-0.0.3/branch-rename.json`。

既有功能与验证沿用[原开发记录](../0.1.0/PROGRESS.md)，保留其中日期、版本名称和证据边界，不将历史0.1.0名称当作已发布版本。启动器标题、启动诊断版本及便携打包支持改为0.0.3。

## 2026-10-23 — 发布范围及等待颜色路径回滚

发布内容包含第一人称预览、右摇杆死区、happyotaku playground、路径历史、瞄准／狙击镜修复、机枪／世界恢复、双目帧传输优化及Steam在线／离线配置兼容。用户确认当前上下机枪延迟已可接受；完全无延迟仍为后续优化目标。其他来源版本采用用户确认的措辞：“也顺手照顾了一下玩家从‘其他地方’找来的兼容版本。”

用户明确要求等待另一个会话完成闪烁相关回滚后再发版。已找到正在执行该工作的「排查启动器未进入游戏」，其目标是恢复原有颜色路径。最终native／XR清单、源码冻结及打包须以该会话完成后的默认值为准；本次不自行更改其XR代码或回滚方案，不将此前统一sRGB试验写为已发布功能或闪白修复。当前仅准备版本标识、发布说明和工具支持，尚未创建线上0.0.3标签或Release。


## 2026-10-23 — 回滚完成、源码冻结与最终包验证

已等待「排查启动器未进入游戏」完成颜色回滚。最终XR为build-runtime-colors1，SHA256 58ef6c397af8facc5ae50c8adba659005a13efd603e7bcfee73ba890d1e2539a；VDXR恢复UNORM，SteamVR保留sRGB兼容。原生7571fee6…bab9和Playground a7b4f3fb…0535保持。左眼局部闪白按用户要求作为已知问题暂缓处理，不宣称回滚已证实修复根因。

以最终默认组件新建不可变输入副本和干净stage。最终ZIP共725成员、14,551,352字节，SHA256 `b9068e926db17a3f95f323e3e8c05ebd0c91c405483cf126d784eaac19cdd0e2`。98项debug-launch、217项vr-launcher检查通过，共315项；公开源码同一组检查及GUI编译／隐藏自检也通过。最终中文／空格路径解压、随包Python -I/-B隔离预检、GUI实际选用随包Python、带下载标记脚本探测、23项原生自检和无会话／预期失败日志导出均通过。没有另启游戏或扩大头显验收。

公开源码从0.0.2的main创建独立发布提交39d35bcbc0705b1310b1d37a38bb2578c8407b19，799个文件；native／XR来自实际默认构建的冻结源码，58个运行Python模块与ZIP语义逐一相同。源码不包含本地开发历史、个人设置、第三方依赖、分析／逆向衍生物。公开源码准备完成，尚待上传。

本地releases/0.0.3整包就位，根目录GUI更新为0.0.3，SHA256 45cf2da24a390e31b5956d8c1d8ae75e1b6bd8158b93aeaa68a547cebe487fb6；旧GUI保存在忽略的发布工作目录，未结束用户旧窗口／进程，个人配置保持。当前提交同时归档原开发工作区源码；发行二进制对应的严格来源以独立公开源码及冻结清单为准。


## 2026-10-23 — 0.0.3正式发布

GitHub [v0.0.3](https://github.com/gjc9620/gears-vr/releases/tag/v0.0.3) 已公开并设为latest，附完整ZIP与SHA256SUMS。新包SHA256 `b9068e926db17a3f95f323e3e8c05ebd0c91c405483cf126d784eaac19cdd0e2`，725成员、14,551,352字节。上传摘要及公开重新下载校验均通过，远端799个源码文件与审计库存一致。原0.0.1/0.0.2标签和包未变，未创建0.1.0发布。当前codex/0.0.3保留全部开发内容，原0.1.0分支已删除且历史保留。详情见[打包记录](PACKAGE_BUILD.txt)；机器回执在忽略的analysis/github-publish-0.0.3/publication-receipt.json。

## 2026-10-23 — 武器 6DoF 分支合并发布基线

用户确认 0.0.3 已发布并要求合并后恢复测试，取代此前等待 0.1.0 发布的临时安排。将已发布分支提交 `9ffa710d5428f270a2ee7e6d7c8e9a930dbb7b81` 合并到独立的 `codex/weapon-6dof`；共同基线为 `315473dc…04770`，合并前武器分支为 `2e3e2a4f…38a2`。共享的 `J:\gearsVr` 仍在 `codex/0.0.3`，其已有配置及未跟踪文件保持原哈希，未切换另一会话分支、替换已发布组件或更改公开包。

解决 9 个文件的冲突：证书构造同时保留私有 weapon-call 元数据和发布版合并读取；世界恢复同时保留私有观察器及发布版相机重试／churn 检查；启动器同时保留臂展、早期预留、路径历史和模拟器专用配置。Playground 保留已有 64 字节入口认证、提前 Reserve、MinHook 分配／失败清理与全部默认关闭的武器私有路径。立体门合并 world sample 阈值与手部连续性，推进世界时立即使旧证书失效，旧 sample 不消耗待执行 recenter。原 0.1.0 进度文件的双方新增历史均保留，后续记录写入本文件。

合并验证通过：Python 317 项（其中 5 项按原条件跳过）；严格 MSVC/C++17 的 StereoFrame、HandReferenceContinuity、HandWorldContinuityBridge、EquipmentIntentBridge、FreeHandControl 五个 CPU target 退出 0。新增回归沿用真实 Gate 的 64 次世界恢复循环，覆盖立即失效、重复／倒退阈值、阈值前旧 sample 和新证书恢复。GUI 编译及隐藏自检通过，候选未安装。第一次 Python 原生／启动器组缺少现有 pefile 搜索路径的失败保留；绑定项目已有 python-libs 后通过，未安装新依赖。CPU 编译前的路径分隔符 guard 失败及原验证器也保留，未重写为所有步骤首轮全过。

Astra 对 Native／XR 源码与两份 CMake 的双向增量保留窄审通过，报告 `d522c162…1dc4a`；协调者核对实际输入、工件与共享文件，报告 `8eed4eb7…b0728`，均在忽略的 `analysis/weapon-6dof-merge-003-20261023/`。本轮合并检查尚未启动游戏或模拟器。后续从新合并源重建并冻结私有候选，保留发布版 Native/XR 配置与颜色回滚；旧 R8 的 742 源／DLL／Native／XR 证明保留，不能冒充新合并版本的验证。实际后台测试继续使用 behind、observe-only、mute，并禁用物理后台手柄输入。

## 2026-10-23 — 暂停期间的 Longshot 离线增量收尾

私有单眼矩阵转换及单 source 准备部件通过独立窄审，具体行为见[武器交互设计](../../weapon-6dof-interaction-design-20261022.md)。单眼转换首个 CPU 构建／运行完成 114 场景、22,204 检查、3,040 个独立世界点，并对实际旧 pair 实现作 143 次返回／全输出对照；单 source 的合成构造／所有权／回收检查为 30 场景、978 检查。前者报告 `df15bed3…d02f`／Astra `fc412c68…7f02`／root `c7032782…6426`，后者 `95235b23…7145`／Astra `62bec571…dc49`／root `6eff862a…74fa`，分别保存在忽略的 `analysis/longshot-single-eye-matrices-20261023/` 和 `analysis/longshot-single-source-20261023/`。

单眼补丁私有重建的 CRLF 字节证明失败、协调者把虚拟输入清单的根目录误读导致的只读核对失败、单 source 构建后的 PowerShell 元数据写入错误均保留，成功 CPU 程序未因此重编／重跑。两部件均未应用到 Native；真实 state／family／目标生命周期、原生间接资源析构语义、单 view 提交、GPU 与镜片画面仍待实现和验证，不构成真实瞄准镜完成。

## 2026-10-23 — 合并后私有候选重建及测试排队

以合并提交 `b83909f5929d236a01ac6d770e15545b34dcc30c` 新冻结 774 个 authored 输入，另存 18 个 MinHook 与 9 个 OpenXR SDK 依赖。Native 继承发布版全部 72 项 GEARS 配置后叠加 free-hand observer／continuity，XR 继承发布版 3 项后叠加 continuity，Playground 保留原 R8 的 26 项；没有带入 Longshot 私有 stage。三个独立组件各首次构建通过，Native `08212497…76e4f`、XR `0463e75f…32eff`、PG `88173598…aaf79`，loader 保持 `66946ea6…32df`。14 个必要纯 CPU 程序各运行一次通过，PG 23 个实际导出与旧 R8 名称一致；尚未执行完整 CaptureSelfTest／GPU 或游戏会话。封尾路径表示检查及旧导出 JSON 格式比较的修正记录保留，没有重编成功产物或重复 CPU。

协调者核对 774 源／27 依赖、实际编译项目和缓存、19 个直接构建产物及 PE 导出后，按字节复制为新私有 bundle `47f9ef19…26495`，manifest `fb9e8b54…1821a`，共 26 份运行／来源工件。只为受限预留接口另置专用 `tools/playground/build-hand-mesh-post003/Release` DLL，原构建到保存名称仅改 basename。普通启动器、默认 Native／XR／PG 和公开包未安装或替换。原始构建、root 冻结核对在忽略的 `analysis/hand-mesh-post003-candidate-20261023/`。

新的 k 入口经 Astra 发现并修正两项接线问题：完整 reservation 回执应从生产者实际 `WORK/analysis/runtime` 读取，并在得到确切 birth identity 后就确定真实 PG 日志路径，以便 attach／Enable／arena 早期失败也能在游戏退出后归档。修前失败报告 `c25643c4…780b1` 保留，修后窄审 `4aa728c5…09067` 通过。root 只绑定常量且逐函数／class AST 不变；最终 runner `b53e8b71…4f18b`、audit `00a9feef…14131`，绑定后的 runner＋idle 52 项及 audit 58 个纯 mock 检查通过。这些 mock 结果不构成实际手／枪或整局通过。

新 `analysis/hand-mesh-runtime-1023k/prep.json` 摘要 `738f5fde…096d1` 已复核，绑定 210 依赖、24 份复制、24 个实际加载模块和 8 个 helper 来源，保留 source0、R2／L2 模式关闭、禁物理后台手柄、behind／observe-only／mute。当前发现另一局游戏及其 launcher／XR／audio worker 正在运行，未干预。已排队一项自有后台等待（初始 owner PID 44968），要求连续三次空闲、再次校验同一 prep 后才进入单次运行；源码或配置漂移会拒绝，不自动重试已进入的会话。排队时本分支尚未启动新游戏／模拟器；实际结果以后续 receipt 为准。等待状态及可取消标记仅在该忽略目录，未形成新的发布或头显验收。

## 2026-10-23 — 合并后首次后台运行及入口参数修正

现有会话正常结束、连续三次空闲后，k 实际启动了自有 Steam 正版游戏。启动经过本次所要求的原生自检、DLL／mask 预留及入口字节检查，到 owned target priming 后失败：runner 的 reservation 命令遗漏 `--graphics-startup-lock` 和 `--graphics-menu-lock`，检查器因此期望 0 项图形钩子，而与 0.0.3 一致的 Native 候选正确启用了 4 项，scene 钩子为 0。Astra 只读核对 CLI、manifest、构建选项和实际 status，确认应在下一独立入口补齐两参数，并加入其要求的 `GraphicsMenuLockTest.exe`；保留检查器和已发布的图形保护。后续 continuous session 只接管现有预留，不需要增加不受消费的 graphics 参数。

本次没有进入手／枪 trial，也未启动 XR 主程序，component／whole session 均为失败。Native 取消状态为 `disable_status=0 active=0 entries_match=1`。协调者用确切创建时间及进程等待状态确认原游戏已终止；退出码为 `0xc0000005`，不能记为正常退出。原 owner PID 已被其他进程复用，未对复用进程执行操作。音频守护正常收尾，`matched=1 muted=0 restore_pending=0 read_failed=0 mute_failed=0`；本次未发生静音写入，无待恢复音量状态。生产者在预留完全成功后才写完整 reserve 回执，所以本次没有该完整回执，保留实际 birth、原始 stderr／stdout 和 PG 日志。

失败证据封存为 k 的 139 个文件，seal `866e9347…1e99`，协调者结果 `ff884bd6…3c8`；原 j 的 51 份封存文件仍逐一匹配。独立退出核对第一次误把仍被句柄引用的已退出进程对象当作仍在运行，已改用 Windows 的 signaled 终止状态核实，并保留这项核对错误说明，没有因此重启游戏。私有候选的 774 源／26 工件保持，未重编已成功的三个组件。

运行结束后另一个会话切换了共享 `J:\gearsVr` 的分支，原 k 准备时依赖的五份共享配置／Python 输入发生变化或移除。没有恢复或改写该会话的文件。后续 l 改用本独立 worktree 的 Bridge／Resolver 和从已发布提交逐字冻结的 manifest，配置也从 k 实际使用的私有副本冻结，以免测试再次依赖其他会话的分支切换。l 是新的单次入口，k 的失败记录不复用、不重写。

## 2026-10-23 — 独立入口完成预留与 Lancer 基线，继续修正激活参数

仅补建缺少的 `GraphicsMenuLockTest` target，首次成功，原 Native／XR／PG 和 14 个已通过 CPU 程序未重编或重跑。新私有 bundle `5ae4697c…82101`，manifest `6572c317…475ba`，保留 774 源／27 依赖及原 26 工件，新增该自检程序和来源报告两项。l 的两 graphics 参数与 WORK 模块／冻结配置接线通过 54 项必要 runner／idle 检查；协调者确认仅五个既有函数和一个新 helper 改动，清理、15 秒时限、256 次上限及 audit 函数保持。实际准备绑定 209 依赖、25 复制、24 个加载模块和 15 个来源，未再读取共享 J 的可变 Python、manifest 或配置。root 的后续绑定仅改常量，函数／class AST 不变。

l 后台实测已成功通过原生预留，完整回执确认 4 项图形保护、0 项 scene 钩子及确切 PG 预留；进入场景后完成原版 Lancer 准备、库存和八个控制器／武器组件的基线。随后 Native 正式激活失败：`vr_dof_identity configuration=0 runtime_owned_transport=0`，状态记录 `hook installation failed disable_status=0 active=0`。这次实际启动了 XR 并发布 3,935 个 hand-continuity 样本，但没有提交游戏场景层，也没有进入 hand trial；component 和 whole session 都保持失败，不能用 XR 样本数替代手部应用证明。

退出时 `PlaygroundShutdown=0`，八控制器、44 字节挂接和三组武器组件原始状态逐一相符，原生 CallFunction 入口恢复记录保留。8 个游戏音频会话已静音并恢复，`restore_pending=0`，未修改普通启动器音量。协调者核实确切 game／owner／XR 均不在运行；游戏退出码 1，本次没有把它写成正常 exit 0 或完整 Native cleanup 通过。l 的 96 份文件封存，seal `4c2300ba…0142a`，结果 `9275a7d2…6b02b`，原始外部 Native／XR 状态、CSV、stdout、stderr 及回执均复制保存。

直接原因由原源码链确认：测试的 session 参数遗漏 `render_scale`，XR 因而没有 `--render-scale`，不会发布 `RenderSizeBridge`；新 Native 的 owned transport 必须取得当前分辨率信息，故在代码段哈希校验之前就短路拒绝。`runtime_ranges_match=0` 在这里不能当作游戏二进制哈希不匹配。下一独立 m 入口从已冻结配置传入该参数，沿用相同候选与检查器，不关闭发布版 VR DOF 或运行时分辨率机制。此前对已结束 agent 使用普通消息没有启动新的复核任务，root 已纠正为正式 follow-up；l 的实际放行依据是协调者源码与实物复核，没有虚构一次未执行的 Astra 通过报告。

## 2026-10-23 — 双目实际提交恢复，定位首个手／枪姿态事务超时

m 仅从已校验的冻结配置补传 `render_scale=1`；两个针对性检查调用真实 XR／Native command builder，确认 XR 只增加该参数对、Native 命令逐项保持，非法比例仍拒绝。协调者核对新 helper 和这一 keyword 之外的全部函数／class、finally 与 audit 保持。Astra 独立源码诊断确认缺失参数的完整发布／读取链，报告 `8f94ad69…a27ff`。本轮未重编主要组件或重跑已通过的 54／58 组。

m 实际启动、原版 Lancer 准备、Native 激活和第一帧双目提交均通过。Native／XR 子会话退出时自身为 passed，短窗口实际提交 59 个 projection layers、完成 32 次 scene copies，跨度约 0.635 秒；不是长时帧率、实体头显或完整交互验收。Native 同会话 FPP 累计 applied=42、hand publication=45、continuity publication=39，原生入口恢复、drain、状态资源回收和 XR exit 0 均得到回执。

手／枪 trial Start 返回 0，但首个枪体 acquisition 在 Rotation setter 认证前结束，完成 hand lease 为 0。Absolute 和 Translation 曾暂时设置，Rotation 尚未调用；原计划 `created_ms=21246656`，`write_until=21246756`，认证时 `now=21246765`，超过原 100 ms 时限 9 ms。事务已回退，独立原始字节比较确认八控制器、44 字节挂接和三组枪组件恢复，32／64 字节原入口匹配，8 个音频会话恢复。component 和 whole-session 仍是失败。m 的 105 证据加 seal 均保留，seal `ab3a48a5…5f910`，协调者结果 `83a2a519…5cfc9`；确切 game／owner／XR 都已退出。

另修正了一处独立审计接线错误：旧 collector 仍要求 146 工件，而新固定 manifest 为 28。只在新副本把两个数量断言绑定至显式 `ARTIFACT_COUNT=28`，保留数量唯一性、全部 hash／flags／路径／依赖和 Guard 检查；28 项含重复及 27／29 项仍拒绝。对已退出 m 的只读重算不再有 source 采集异常，artifacts／environment 通过，15 项真实 trial 断言继续失败，没有改写 m。原 106 个文件前后逐一相同；副本及结果在忽略的 `analysis/hand-mesh-audit-count-fix-20261023/`，报告 `ed81ee0e…d72a8`。

Astra 对超时调用顺序的诊断 `f9ef058c…d49f` 表明，不能给同一不可变 Plan 或 certificate 重置原始时间：现有代码本来就在每次 setter 前重新认证，末次 snapshot 完成时原 stereo 已约 98.534 ms。尤其 `component::read` 首尾的 `current()` 会各调用四次真实 `GetWeaponInSlot.run()`；按成功分支，到本次 Rotation 认证前已有 96 次该 getter，不能视为纯内存读取而跨 engine 调用复用“校验通过”。下一步只在私有候选补有界分项计时，区分 metadata 与真实 invoke，再决定最小降本；100 ms、原始源时间、现有调用顺序与恢复逻辑保持。

## 2026-10-23 — 首次组件超时的私有分段计时

根据 m 实际首次 acquisition 的严格 100ms 超时，在现有 component_cost 诊断中追加 6 类 × 4 组的固定 24 桶，覆盖 current/read 总段及原有 GetWeaponInSlot 的 Args、preflight、run/invoke 和结果检查。保留原 64 行、所有真实 getter 次数／短路／顺序、原始 Plan 与证书时间、setter 授权及恢复；没有热路径文本和额外游戏调用。统计累计到首次故障或原有 stop dump，可能跨多个 acquisition，没有独立 lease 标记；嵌套统计不可相加，run/invoke 包含现有 guards/wrappers。

独立 stage 的真实 PG TU 在同 26 项配置和 /W4 /WX 下首次 object 编译通过，实际抽取计时器的 15 场景／91 检查通过。Astra 窄审 5bf582db…3e76 无阻断，root 核对三份原件／stage／复核哈希后应用。本次尚未链接新主 DLL 或采集真实引擎分段耗时；不能据此判定瓶颈或认为 100ms 问题已修。原 m、父 bundle 和默认启动器保持。证据保存在忽略的 analysis/hand-mesh-component-cost-next-20261023/。

## 2026-10-23 — 分段计时的实际后台结果

计时版 PG 首次链接通过（f02e9fb0…6c04），保持原 26 个配置、23 个导出；Native／XR／loader／helper 沿用。以提交 1ca7404 冻结 774 源／27 依赖／31 工件的新私有 bundle 0ee39924…0408，manifest e53eb246…06c5，未替换默认启动器。n 入口仅绑定常量和专用 build 路径，audit 同步固定实际 31 工件且保留唯一性／SHA／构建开关校验；旧整套 mock／CPU 未重跑。root 准备核对首次误把结构化 source_profile 当作整数，纠正为候选的 hand source 字段后通过；入口、prep 和旧证据未改，未因此重试游戏。

n 实际 Steam 后台会话的 Native／XR 再次通过：57 个 projection layer、30 次 scene copy、约 0.621 秒有效提交；这仅是短会话功能证据。component／whole 仍失败，零 hand lease：Rotation 的 authorize 拒绝位为 1024；tick Plan 年龄 94ms，原始 stereo 源在 authorize 时已 109.0792ms。没有延长 100ms 或重锚原始证书时间。

新增固定桶均有有效时钟且未饱和：28 次 current 总计 41.8484ms，13 次 read 总计 40.6419ms；二者嵌套不可相加。112 次 slot getter 的 Args／preflight／完整 run-invoke／postresult 分项分别为 0.1714／0.0401／0.1050／6.7963ms。原始 initial＋三个 snapshot 合计 15.8540ms，初段开始至末 snapshot 跨度 62.8192ms。数据支持继续减少身份／属性反复遍历，不能把真实 getter 执行体当作主要瓶颈，亦不构成 FPS 结论。

失败 Stop／Shutdown 均为 0，原始 8 控制、44 字节 attachment、3 组件组逐字恢复，CallFunction／ProcessEvent 入口精确恢复；Native drain／entries restored、XR exit 0，独立确切创建时间核对原 owner／game／XR 已退出，game exit 1。8 个音频 session 静音并全部恢复，pending 0。修正后的 audit 工件／环境取证通过，保留 15 项真实交互覆盖失败。n 共 108 份证据加 seal，seal 2aea7640…ed40，root result e08025e3…5249；未重开或改写 m。下一步仅对确定的局部 metadata 重复查询做等价优化，不跳过 current/getter 或放宽授权。
