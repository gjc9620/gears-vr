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

## 2026-10-23 — 保留实时校验的局部属性查询合并

将已独立验证的局部 property coalesce 最小 patch 应用到计时源，保留计时改动；私有反向还原在 CRLF 标准化后与应用前原件完全一致。仅新增 field_at 并在 boolean、fast absolute_storage、fast read 的无 engine 调用局部区段复用解析的 property；Translation／Rotation 句柄块在末 current 前结束。所有 registration／类型／维数／大小／offset／mask／live value 检查、首尾 current、所有 getter、VM 顺序、Plan／100ms、setter 及恢复保留。

原／新真实 helper 的 36 个稳定 metadata fixture、547 项检查通过；相关 property 遍历 boolean 2→1、absolute_storage 6→3、read 局部区段 10→5，不包含 current 内部。首次严格编译仅因测试 fixture 全局 mesh 同名失败，改 fixture 名后保持 /W4 /WX 通过，生产 stage 未因该错误改变。Astra 窄审 9f6d3716…a96a 通过；CPU／旧套件未重跑。root 私有反向证明先被 Windows 反斜杠拒绝，再因 git autocrlf 的 LF→CRLF 未逐字相等；标准化后完全一致，两项证明错误保留，不涉及代码失败或游戏重试。真实提速及 100ms 授权仍待新候选实测；不会据此宣称自由手／枪已完成。stage 和集成证明位于忽略的 analysis/hand-mesh-local-property-coalesce-20261023/ 及对应 integration 目录。

Astra 后续核对 n 的拒绝位 1024 为 commit 时的 certificate_recent：原 stereo 实际年龄 109.0797ms、input 109.0562ms；authorize 采样年龄 109.0792ms，tick 票据 94ms 不能覆盖源姿态过期。current 扣除四个 slot 分项后 34.7356ms 仍包含其他身份／链接／类型检查及计时开销，不能全部归因给某一 property。

## 2026-10-23 — current 热点的局部删除属性复用

按 n 实测与 Astra 源码诊断，在一次无 engine 调用的 fast object_key 内复用第一次非零 bDeleteMe property 及其解析 class。保留两次实时 registration／offset／mask／删除位读取、所有 header／outer／class 检查；加入 header 与末次 live class 不一致时拒绝。首次 property 为零仍完整走原 deleted 再解析，没有负缓存；句柄随该次函数返回失效。按 n 路径最多可少 234 次 property 遍历，零命中会保留重试，因此不宣称实际全部减少。计时、字段 coalesce、所有 current／getter、原 Plan／100ms、setter／journal／rollback 保持。

25 个稳定及失败等价场景、2 个额外 class 变化拒绝场景共 160 项 CPU 检查在 /W4 /WX 下通过；新增 class 变化拒绝不是对任意并发 metadata 修改的原／新等价保证。为明确加入 missing 解析计数断言，窄 target 两次构建／运行都通过，旧套件未重跑。Astra 窄审 f8e666cc…04b9 无阻断，root 读完整 diff 并核对原件／stage／报告后按字节应用 0aa6294b…d5d0；root 首次误读短报告顶层 passed 字段，在源码写入前失败，随后按实际 CPU 结果结构核对，错误保留。非零查找 2→1，missing 保持 2→2，两个 live 位读保留；没有新 RE。来源在忽略的 analysis/hand-mesh-object-key-property-20261023/。

此前小字段 coalesce 的 PG 已首次构建并冻结 e9c1b79b…93ea（774／27／34），但 o 未 prepare 或进入游戏；因 read 字段段包络较小，下一实际会话将同时验证本热点优化。不得把 o 的编译／冻结称为运行通过。组合候选仍需首次真实交接、运动／取放覆盖及完整清理验证。

## 2026-10-23 — 组合热点优化的实际后台结果

以 d171f6e 首次构建 PG 44884632…0091，仍为原配置与 23 导出，仅相对上一私有候选改变 phase1 一个源文件；冻结新 bundle e1cad4d1…e7cb，manifest ef232989…d314（774 源／27 依赖／37 工件）。p 入口只绑定常量／专用路径并继承全部原时限、source0、物理后台手柄关闭及 behind／observe-only／mute；准备后 root 核对 209 依赖、25 复制和 24 实际模块，空闲后只运行一次。

p 的 Native／XR 短会话通过，71 projection layer、43 scene copy、约 0.767 秒有效提交；component／whole 仍失败、0 hand lease，Rotation authorize 再拒绝 bit10 certificate_recent。commit 时原始 stereo 102.8284ms、input 102.8032ms；tick 票据 93ms，截止余 7ms 不覆盖源过期。原时限未放宽，旋转调用未执行。

与 n 相同调用计数下，本轮 28 次 current 总计 33.8867ms（n 41.8484），13 次 read 总计 31.9711ms（n 40.6419），112 次 slot args／preflight／run-invoke／post 分项 0.1670／0.0385／0.1149／6.4742ms。嵌套总段不可相加；这是两个功能会话的具体观测，不是稳定性能或 FPS 比较。原 initial＋三 snapshot 合计 14.5073ms、跨度 53.1420ms；仍需继续减少重复遍历，不能将局部提速称为交互通过。

Stop／Shutdown 均 0；8 控制、44 字节挂接、3 组件组与两原生入口精确恢复，Native drain／entries restored、XR exit 0。确切创建时间独立核对 owner／game／XR 已退出，game exit 1；8 个音频 session 全部恢复，pending 0。工件／环境取证通过，15 项交互覆盖失败保留。p 共 122 份文件加 seal，seal b6af1935…bc31、root result 02c192ee…aafe，未重试本会话、未改 n 或默认启动器。下一候选只评估有界字符串匹配等不缓存 game metadata／成功结果的解析优化。

## 2026-10-23 — 有界属性名称匹配候选

后续候选仅缩小 fast lookup::named 为判断名称相等所需的读取上限：纯 query 字符循环最多 127 个 wchar_t，field／derives 每次遍历外计算长度，name 的上限为 query 长度＋真实 NUL 且最多 128。其他 name 调用仍默认 128；耗尽后补写的 NUL 仍返回 false，保留 _wcsicmp 的原大小写／locale 语义、FName 表／entry 身份、page／copy 失败检查。未缓存 name 索引、metadata 或校验成功；所有 current／getter／VM、原始 Plan／100ms、setter 和恢复保持。named 局部缓冲区仍清零，没有混入可选微改。

25 个有界字符串／页模型场景、137 检查首次 /W4 /WX 编译和运行通过；覆盖前缀／长名／空／大小写／Unicode／UTF16／126–128／缺 NUL／跨页及保护页。Astra 窄审 3ee7d303…2050 无阻断，root 读完整实际差异并核对报告后应用 a505df6b…1339。短查询 copy 请求 256→8 字节、示例 field 1280→94 字节仅为 fixture 观测，尚非真实提速。较长不匹配名称会提前拒绝，可能少触碰后续 PAGE_GUARD 页；这会改变一次性 guard 消耗／访问轨迹，不声称这种状态下完整执行可观察等价，也没有补读或重试。证据位于忽略的 analysis/hand-mesh-bounded-name-match-20261023/，组合游戏验证待新候选。


## 2026-10-23 — 有界名称候选完成三组枪体取得，末次票据复核仍超时

PG 以 48e67ee 首次构建 efa21855…b850，23 导出及原配置保持；Native／XR 沿用前候选。冻结 bundle 2b8f3dea…180c，manifest 6f74966b…0a1f（774 源／27 依赖／40 工件）。q 只完成准备、没有运行；准备期间另一会话将共享默认启动器更新到 0.0.4 Dev，旧空闲 GUI 哈希因而拒绝。root 核实新 GUI 与该会话存档完全一致，再建 r，只改输出路径、tag、专用 build 路径和已核实的 GUI 哈希四项常量，函数／audit 与时限不变。该 GUI 随后实际启动了其他游戏，r 等其结束并连续三次空闲观察后才自动执行一次，没有中断其他进程。r 的 Native 仍为原私有 003 候选，不能将识别新 GUI 称为已验证 004 Native。

r 的 Native／XR 短会话通过，50 个 projection layer、29 次 scene copy、约 0.534 秒；component／whole 仍失败，0 hand lease。此次三个组件组均已取得并临时设置，末次 group0 authorize 之前置条件拒绝，auth_rejected=2、owned／pending=7。原 ticket 年龄 110ms、原 Plan 年龄 94ms，write_until 来自原 ticket 的 100ms 期限，超出 10ms；authorize_ns／commit_ns 均未采样，不能以零值或另一时钟域推算原 stereo／input 年龄，也不把本次失败写成此前 group4 的 certificate_recent 拒绝。没有重锚时间或放宽原期限。

固定桶记录 30 次 current 共 25.8583ms、14 次 read 共 23.6947ms，二者嵌套不可相加；120 次 slot 的 args／preflight／完整 run-invoke／post 分项为 0.1735／0.0388／0.1071／7.1174ms。initial＋三 snapshot 共 13.9590ms，首段开始至末 snapshot 跨度 44.2078ms。调用计数比 p 多，亦非受控 FPS 比较，只记录当前功能试验的实际数据。

Stop／Shutdown 均 0，8 控制、44 字节挂接、3 组件组与两入口精确恢复；Native drain／entries restored、XR exit 0。root 独立核实 owner／game／XR 确切身份已退出，game exit 1；8 个音频 session 均已恢复，pending 0。工件／环境取证通过，15 项真实交互覆盖失败保留。r 的 105 份文件加 seal 封存，seal 0ff71057…9b17，root result f7a07f06…a39a；原 p 及默认启动器未改。

另在忽略的 analysis/weapon-6dof-left-history-004-stage-20261023/ 保存 004 已完成左眼 history 修复的七文件三方 stage：base 为已发布 9ffa710，ours 为独立武器源，theirs 为另一会话已冻结源码。机械合并零冲突，双向逆合并恢复双方增量，Astra 20328f83…a59 窄审无阻断；保留独立手部发布／continuity 和 renderer 代码。影子目录按原配置加 GEARS_INDEPENDENT_LEFT_VIEW_HISTORY=ON 首次 configure／build 均通过（低优先级、parallel1），新 Native d54ecf18…82c9 保持 34 导出；九个相关 CPU 程序各一次全部通过，没有运行 GPU 自检或旧整套。root 逐项核实 774 原 WORK 输入、776 shadow 输入、18 MinHook、stage 七文件与九个 CPU 程序／日志哈希后，将七文件按字节应用 WORK，776 项均与已编译影子源相同。构建报告 9796510d…cb02、集成证明 921af848…1790。PG／XR 未重编，新的组合 bundle／实际运行尚待完成，不继承另一会话的画面验收；共享 J 与默认启动器未改。

## 2026-10-24 — 最终手部复核的固定尾段计时

Astra 对 r 的原源码链复核确认，after_component Snapshot.ms 采样时距原 cutoff 尚余 6ms，其后 identity／baseline／same-controls／slots／phase gate 及 component_owned 的完整实时 read 继续执行，到最终采样又前进 16ms。component_owned 默认 fast=false，位于原 acquisition 桶之外，不能将 16ms 全归于某项。为区分它们，仅添加固定 8 个 attempt × 5 stage：snapshot、原 guards、条件 owned、内嵌 owned_live_read 和原 final authorize；保持调用／短路顺序、原时间实参、100ms、全部 getter／VM 与恢复，不扩 parsing scope、不改 fast=false。容量满仅丢诊断，父／子段不可相加，仅 clean 且没有打开的 attempt 时从既有结束 dump 输出。

原／新真实片段抽取的 24 场景／276 项 CPU 检查在 /W4 /WX /Od 下通过，覆盖拒绝点、跳过、异常展开、时钟失败／倒退、容量和饱和；Astra b85ee92f…779e 源审无阻断，producer 0a4afedb…a6be3。两项编译前 harness 设置错误及首次测试 wrapper 的未使用参数告警保留，修正不涉及 stage。root 核实原件、stage、全部报告／日志及完整 776 源，再按字节应用两份 PG 源；其余 774 源保持。尚未进行新的 PG 链接或游戏运行，不能据此声称超时已修复。

## 2026-10-24 — 左眼 history 与尾段计时的组合后台实测

以 94fe3aa 的完整 776 源首次 configure／PG build 通过（低优先级、parallel1），新 PG ba73729a…8d45 保持原 26 选项及 23 导出。与已验证的 Native d54ecf18…82c9 合并冻结 bundle 65d0c39d…43a2，manifest 373228f2…8c45（776 源／27 依赖／58 工件／862 实际文件）。八个既有 CPU 产物换成本次 Native 的实际版本，新增 IndependentViewHistoryTest；九份既有 CPU 日志与来源纳入，没有重跑。XR／loader／其余 helper 沿用。root 独立核对完整源／产物／配置、构建日志和专用 PG 三份相同副本，s 准备绑定 209 依赖、26 复制和 24 模块；入口／audit 函数和 class 的 AST 与 r 相同，仅候选等常量更新。三次空闲检查后单次静音、置后、observe-only/source0 进入。

s 实际 Native／XR 通过，65 projection layer、43 scene copy、约 0.712 秒有效提交。新 left_history 的 attempted／created／release_started／release_returned／delete_started／destroyed／delete_flags 均为 1、failure 为 0，drain／entries restored、XR exit 0；这是组合候选的实际短会话及生命周期证据，不是原会话闪烁图像结果的继承，也不是 FPS 或实体头显验收。

component acquired=1、restored=1、setter_calls=6，三组都已设置并恢复；手部 lease 仍为 0。末次 group0 authorize 此次通过毫秒 cutoff 后在 check10 certificate_recent 拒绝（1024）：原 ticket／Plan 年龄均 94ms、截止尚余 6ms，原 stereo 在 authorize 为 112.1465ms、commit 为 112.1470ms，input 为 112.1228ms。区别于 r 的 check1 毫秒超时，不能混写。全流程原 100ms、不可变原始源及调用顺序保持。

一条完整有效 tail row 无丢弃／饱和：snapshot 3.2835ms，原 guards 8.2262ms，component_owned 4.4244ms（其中 owned_live_read 4.4240ms），authorize 0.0011ms。owned 与子段不可相加；四主段合计 15.9352ms。开始 tail 时原源已 96.2114ms，guards 结束已 107.7214ms，因此只缩短 owned 不足以让本次通过。acquisition 桶 30 次 current 26.9683ms、14 次 read 24.4605ms，嵌套不可相加；120 次 slot args／preflight／完整 run-invoke／post 分别 0.1773／0.0403／0.1268／7.4737ms。initial＋三 snapshot 合计 14.1815ms、跨度 44.4303ms；均为单局诊断观测。

Stop／Shutdown 均 0；8 控制、44 字节挂接、3 组件组和两个入口逐字恢复，root 独立确切身份核实 owner／game／XR 终止，game exit 1；8 个音频 session 全部恢复、pending 0。工件／环境取证通过，15 项真实交互覆盖仍失败。协调者收尾首次仅因 Windows relative path 分隔符匹配失败，复用并逐字核对已复制证据后完成封存，没有重试游戏或替换原始证据。s 的 124 文件加 seal 保留，seal 5a631abe…0969，root result f58b1b8a…e18e。下一步评估现有 scoped 名称解析与最终只向前复核的局部算法降本，不能重锚证书或删减校验。

## 2026-10-24 — 持枪事务内的组合名称解析候选

根据 s 实际尾段，在同一候选合并有界降本：仅私有 HAND_WEAPON 宏且 component_names::enabled 时，global named／derives／field 使用 query 长度＋真实 NUL 上限（最多128），walk 外只计算一次纯 query 长度；默认 scope／宏关闭和直接 name 保持。after_component 的既有 Attempt 内仅 phase1/right-mask 开启 Scope，在失败 drain 和手 lease 前析构。component_owned 默认 forward=false，原 fault 位置只采样一次，显式 forward 与该故障位共同选择已有 fast read；只有原 phase1/right 调用传 true，全部 current／slot getter／exact-owned／relinquish 保持。invoke 原 opt-out 不改，直接 drain_journals／stow restore 入口明确 Scope(false)，保留回滚正文和次序。Scope 只选解析算法，原 Plan／100ms／采样位置和 s 计时保持。

六文件 patch ec0bb35a…6682、producer feb7bf53…8702，Astra 9b6e63a9…20f9 窄审无阻断。真实 helper 抽取的24类／227项严格CPU检查通过，覆盖默认／宏关闭、短路、嵌套及默认 owned 遮蔽父TLS。两次编译／执行中首轮仅测试事件字符串指针比较误判，改为文本值比较后通过，候选源未改，原日志保留。Root读完整patch并核实报告／日志与全部776源，首次核对仅误设 producer 原件目录名称、在任何源码写入前停止；改用既有不可变parent源核对原件后，应用六份原字节，其余770项保持。PAGE_GUARD后缀访问轨迹差异及有效内部query前提继续明确；fixture读取量减少不是实机节省。新的PG构建及100ms实际结果待验证，不把候选称为已修复超时。

## 2026-10-24 — 首次联合写入及两个后续失败边界

以 6a976d2 的 776 源首次 configure／PG build 通过，PG 4d9605db…2925 保持原 26 选项及 23 导出；Native d54ecf18…82c9、XR 0463e75f…2eff、loader 及已通过的 CPU 沿用。新 bundle 0b52ed15…cda0、manifest 0708d06b…1a8c 为 776 源／27 依赖／61 工件／865 实际文件。父 bundle 的 57 份其余工件逐字不变，仅替换 PG 并新增三份来源证明。三次空闲后 t 单次静音、置后、observe-only/source0 进入，禁用物理后台手柄输入。

t 首次取得并恢复一次联合 hand lease；双手 setter_calls=8、target_writes=8、restored=1，component acquired=1／restored=1／setter_calls=6，两边 dirty／faulted 均为 0。最终尾段四主段为 snapshot 2.1426ms、guards 4.0528ms、component_owned 1.5520ms（子段 owned_live_read 1.5516ms）、authorize 0.0007ms，合计 7.7481ms；父／子段不可相加。授权结果为 1，原 stereo 源龄在该授权 span 为 91.4009–91.4016ms，精确 commit 时刻没有保存在本次固定诊断中。written 诊断观测源龄 101.9439ms、毫秒票龄 109ms，不能当作 setter 授权时刻或下一包读取失败原因，写后新鲜度验收仍未通过。

随后 `WorldBridge::read` 返回 rejected，终止原因 xr_certified_pose_rejected；下一份 ticket／Plan／policy 检查尚未执行。此读取器对相同 serial 返回 none，所以若属于新包过龄，须是 serial 大于 20 的新包；不能把旧 sequence 942 自然变旧写成确定原因。现有日志缺拒绝分支和被拒包，lock／header／serial／shape／源龄等子因仍未知。Astra 只读诊断 91ca3202…8b84 建议仅补固定首失败记录，保留原短路、100ms 和清理，不做猜测性性能修改。

独立按 t 的冻结 parser／math 复算：下一 Tick 双手位置残差均为 7.629e-6 原始单位，完整旋转右手 0.019797°、左手 0.005469°，均在原 0.1 阈值内。枪体同 Tick written 对目标为位置 0／完整旋转 0.002990°，到下一 Tick pre_restore 为位置 0.000492／完整旋转 **90.010196°**；约等于局部轴变为 [-written.Y, written.X, written.Z]，该模型余差 0.014516°。两次组件属性字节相同，首 bind 输入矩阵未单独记录，同 Tick LocalToWorld 可能仍是旧值；故不能宣称首绑定正确、持续几何通过，亦不能把此旋转问题归因为 reader 拒绝。只读报告 5ea9ccce…b405 位于 t 目录之外，原封存工件哈希保持。

Native／XR 实际通过 60 projection layer、33 scene copy、约 0.658 秒有效提交，left_history 创建／释放／销毁均 1、failure 0，drain／entries restored 和 XR exit 0。这是短会话功能与生命周期证据，不能替代闪烁图像、FPS 或头显验收。Stop／Shutdown 均 0；8 控件、44 字节挂接、3 组件组、两入口逐字恢复，8 音频 session 恢复／pending 0。精确身份 owner 9000／game 23536／XR 29924 均终止，game exit 1。13 项实际交互／覆盖断言仍失败。t 的 107 文件加 seal 封存，seal 07fb240d…2c35、root result 235746d0…52e0；未重跑游戏或修改原失败审计。默认产物与公开包未安装候选。

## 2026-10-24 — 读取拒绝与首次枪体绑定的有界诊断

根据 t 缺失的证据，只新增三个私有源的诊断。GHW1 reader 固定保存第一次 rejected 的原分支、实际执行／通过掩码、cursor、原锁与时钟结果及可用性；仅对已有本地 packet 保存完整 336 字节，清理后输出一次。宏 OFF 的原 read 签名／正文逐字相同、整类非空行相同，packet 和 open／publish／close 保持。首次实际 mount.bind 单独锁定既有 matrix、完整 Key、右 grip、pawn mesh TickTag 和已有 hand snapshot tick；同次 acquire 的 bind／solve／initial_read_after_bind 返回及量化目标分别标 known。不同阶段不作原子同时值，记录不能授予写权。两记录在原成功 reset 后清零、沿既有清理结束出口输出，保留原短路、时钟调用、100ms、VM 与恢复次序；新增固定复制仍会占用 CPU，不声称性能改善或失败原因已修复。

reader 首次严格 CPU 编译／运行通过 34 场景／366 项对照检查，首 bind 首次通过 11 场景／60 项；没有重跑原 suite 或构建主 DLL。producer c191a821…5f32／d4ebaa06…6728，Astra 组合窄审 463a2d5b…d6e3 无阻断。协调者核对两报告的 50 份工件、完整 776 源及两个 patch，按字节应用三份源码，其余 773 项保持，应用证明 843e50c1…8aff 位于忽略的 analysis/hand-mesh-diagnostics-integration-20261024/。u 入口另在 Native 启动与手部 Start 之前，通过既有 typed reader 独立观察资产 Origin／RotOrigin，保存身份、schema、原字节与错误；缺失或不合法保持 unknown，不填零、不作为验收条件或 bind 当刻值。此入口新增 9 项有限离线检查通过，Astra b89a37ff…5552 无阻断。下一步 PG-only 构建和实际读取仍待执行，尚未新增游戏或几何通过结果。

## 2026-10-24 — 两次联合接管、实际模型旋转与末次时限边界

上述诊断完成一次 PG-only 严格构建和组合封存，PG 720bd5c7…f20c，组合 manifest 263727f2…57d4；776 源、27 依赖、67 工件／871 实体文件经 root 核对，Native d54ecf18…82c9、XR 0463e75f…2eff 与 loader 沿用已冻结来源。旧 60 份非 PG 工件逐字保留，没有重建或安装默认入口。入口第一次绑定因 CRLF 匹配失败，UNBOUND 检查在运行前拒绝；修正后才准备并进入 u，没有重试游戏。不能将整条准备链表述为零失败。

购买版后台 u 完成双手 lease=2、restored=2、setter=16；枪体 acquired=2、restored=2、setter=12，均无 dirty／fault／relinquish。独立手部跨 Tick 几何和枪体协议通过，枪体实际完整旋转及移动／收回／返回等整局覆盖仍失败，保留 9 项真实失败。Native／XR 实际提交 80 层、38 次场景拷贝、跨度 0.8662066 秒，最大提交间隔 15.6608ms；left history 创建、释放和销毁各 1 次、失败 0。该结果只覆盖此局的生命周期和提交，不构成闪烁图像、FPS 或头显验收。8 个音频 session 均恢复；8 控制、44 字节挂接、3 组件组与入口恢复通过，自有 owner／game／XR 的确切身份均已退出。u 的 105 文件加 seal 保留，seal 1279a452…0593，root result 9439d95c…63b。

u 的封存 root result.failure 将历史首次授权拒绝误写成末次停止原因，原件不修改；外部 correction 1ece16dc…76e9 和独立诊断 28dd8e68…25c9 明确区分：ordinal 276 的 certificate_recent／1024 拒绝发生在第二次成功 lease 之前；真正末次 ordinal 554 在毫秒检查 2 被拒，原 ticket 年龄 109ms、超出原截止 9ms，后面的 NS 与 policy 阶段未执行，零值不能解释为采样时间。此前 ordinal 553 授权通过时原 stereo 已 98.3052ms。文本证据段最大仅 0.1218ms；末次 postflush 的原实时设备检查耗时只能界定为 5.4870–6.7864ms，第二次 pre_restore 到第三次 before 的 85.0009ms 尚缺同帧分段，不能据此删除检查或声称微优化足以修复。u reader 未留拒绝记录，也不能反推 t 的原因已解决。

实际 typed 资产读取取得 Origin=[0,0,0]、RotOrigin=[0,-16384,0]，完整资产身份与首个真实 bind 记录一致；预检值并不冒充 bind 当刻值。首 bind 原始 matrix、右 grip 与未量化 solve 均已记录。两次后续 Tick 的枪体残差约 90.015°；按模型旋转叠加到组件旋转的完整矩阵关系预测，残差降到约 0.02474°。这支持下一步在当前资产认证下按列轴 C=D×transpose(O) 求组件旋转，不能只加 yaw：后者仍约 12° 错误。非零 Origin 的平移关系尚未建立，不作推测。

纯旋转 helper stage 的独立物理向量 oracle 单次执行通过 33 场景／107 检查；u 未量化目标只在最终量化一次得到 [-1218,-5134,1001]，提前量化再补偿的反例得到不同结果。首次严格编译因 fixture 注释 Unicode 在 CP936 下触发 C4819，仅改 ASCII 注释后编译通过，生产 helper 未变。此时仍未接入实际 setter。正在接线实时资产 schema／身份复核并补充同 PlayerTick 的三段 QPC 诊断，保留原 100ms、原票据／Plan、raw 三组所有权和恢复顺序；新审计须分别验证 raw 组件控制量与物理模型目标，原 0.1 阈值和运动覆盖要求保持。

## 2026-10-24 — 实时资产旋转补偿与同帧时间诊断接线

将纯旋转 helper、实时 Origin／RotOrigin 接线和同 PlayerTick 诊断合并到独立武器源码。初始 raw read 成功后认证当前资产与 typed 字段，Origin 必须已知、有限且数值为零；以未量化模型目标 D 和实际 O 求 C 后只量化一次。每次 forward setter 在原时钟前复核资产，tail 在原 raw exact 成功后复核；几何变化拒绝继续写入但保留仍拥有的 raw 字段供原 drain 恢复。内部 faulted rollback 和显式 restore 不加新几何门，原 Backend、lease 和原 bind／solve／translation 路径保持。

同帧诊断保留末尾 8 条 completion-order 行，完整路径只在 pre 进入、原双 journal 恢复后和 post 进入新增 3 次 QPC；成功 read 复制原 now／now_ns 与同一 out，原票据／Plan 和所有权限时钟保持。当前终止行及其实际授权单独冻结，短路未执行的授权与 NS 阶段保持 unknown，不引用历史 first_failure。合法游戏线程与 Shutdown callback drain 前提明确，默认宏 OFF 的原 read／class 与源码在 8 对预处理比对中保持。

纯 helper 33／107、实时资产接线 19／43、同帧诊断 29／119 及新几何审计 34／37 的有限验证均通过；各自实际首次 fixture 编译或 patch 包装失败照实保留。新 schema2 审计将 raw C 的字节配方与物理 D 的跨 Tick 方向及刚体 mount 分开，原 0.1 和两个移动阶段各至少 8 次响应等门限保持。Astra 独立复核资产接线、Sol 独立复核诊断均无阻断；root 核对 61 份 producer 工件和全部原 776 源，仅修改 4 源并新增 1 header，其余 772 项保持。应用证明 ec4f5fa1…c140，源树 44a43c9c…d6b9。root 首次合并前 guard 因 include 锚点出现两次而拒绝，未写生产文件；改用唯一的完整私有 include 块后完成应用。PG-only 构建及补偿后的实际几何、持续运动和整局覆盖仍待本轮后续执行，没有安装默认入口。

首次完整 PG TU 严格构建在 configure 通过后失败：新 geometry_log 参数与 log 局部变量 observed 遮蔽主文件原有全局 observed，/WX 将 C4459 视为错误。原 source／build／日志 3f6be687…a705 全部保留，尚未启动游戏。仅将两处局部作用域内共 10 个标识符改为 asset_observation，字符串与 observed 日志前缀保持；逆向标识符替换可逐字还原原源码，不降低警告等级。下一次使用全新构建目录，有限 CPU 不因此重复执行。

## 2026-10-24 — 九次联合接管的枪体几何通过，恢复阶段仍挤占原票据

仅修正两个局部名称后，在全新 b 目录第二次完整 PG TU 严格 configure／build 通过，23 个导出保持。新 PG 5ce968c6…b082，组合 manifest e6a66bb8…d1a6，源提交 367cc463…2f9a；777 源、27 依赖、87 工件／892 实体经 root 核对，父候选的 66 份非 PG 工件逐字保留，Native／XR／loader 未重建，默认入口未安装。第一次严格 TU 失败的 source、配置和日志保留，未降低警告级别或重复旧 CPU suite。root 首次核对组合 closure 只因 Windows 依赖路径分隔符与 as_posix 不同而拒绝，统一比较键后全部通过，未改清单或工件字节。

全新 v 一次性入口在三次空闲观察后进行购买版后台测试，仍为静音、窗口置后、tiny-output 只观察、实体手柄后台输入关闭。实际完成双手 lease／restore 各 9 次、72 次手部 setter，枪体 acquire／restore 各 9 次、54 次 setter，dirty／fault／relinquish 均 0。新 schema2 原 raw 协议、实时资产输入及 9 个 later-Tick 枪体矩阵通过；相对未量化物理 D 的最大完整旋转误差 0.02611015°、最大位置误差 0.00817397 原始单位，均在原 0.1 门限内，u 中约 90° 的旋转偏差在这些实测样本中消除。此结果只覆盖当前 Lancer 资产和有限姿态，模拟器没有独立控制器 roll 输入，不等于全部姿态、其他武器或头显可见性验收。

基线阶段完成 4 个 later-Tick 样本，随后发送了 4 条双手移动命令；原每阶段至少 8 次运动响应、收枪和取回等覆盖仍未完成，整局保留 7 项真实失败。第 10 次写入前，实际 terminal row 为 tick2394／site1／serial43／sequence958；这次原授权 checked=3、rejected=2，ticket 年龄 110ms、超出截止 10ms，NS 与 policy 后续阶段未执行。新三边界记录将该帧 pre 到原双 journal drain 后的区间测为 69.3372ms；这是整个恢复分支的观测，尚不能直接归因于单个 setter 或将嵌套桶相加。下一步核对恢复和预恢复证据中的名称解析开销，保留原 getter、guards、恢复次序及原 100ms，不刷新票据或用新时钟代替原始源。

Native／XR 此局通过 219 层／66 次场景拷贝、跨度 2.4120402 秒、最大提交间隔 13.3935ms；left history 完整创建／释放／销毁各 1、失败 0。该提交记录不作 FPS 或闪烁图像验收。Stop／Shutdown 均返回 0，8 控制、44 字节挂接、3 raw 组件组和两个入口逐字恢复；root 以确切出生身份独立确认 owner／game／XR 退出，game exit1。8 个音频 session 全恢复、pending／read_failed／mute_failed 为 0。v 的 128 文件加 seal 已核对，seal dc73fa08…a8bb，root result 7b8da069…08e0；新的 root collector 将历史 first_failure 与真正 terminal ring 分开，未重用 u 的错误归因。补偿后的持续运动、暂收返回和全武器功能仍未通过。

## 2026-10-24 — 新增单双手射击伤害规则

用户确认单手和双手持枪均可射击，单手伤害为相同条件双手的 0.7 倍，双手以原版 1.0 倍为基准。已补入设计正文、弹道约束和验收表：按开火瞬间有效副握点抓握状态确定倍率，倍率随该发子弹／投射物冻结，同发霰弹共用，每个伤害包只应用一次；整数伤害的原版舍入规则在接入时核对。该要求尚未接入实际伤害链，不将当前几何试验当作伤害实现。原显式恢复名称解析候选继续并行开发，此次只修改文档，没有运行测试或游戏。

## 2026-10-24 — 显式恢复的名称解析候选

根据 v 同帧记录，只在枪体显式恢复与 dirty pre_restore 日志中启用现有、实时且无 metadata 缓存的 scoped 名称解析算法。Backend 新增默认 false 的 restore_names；两处名称 scope 按 forward ? !faulted : restore_names 选择，仅显式 nullptr restore 单点传 true。原 Owner 与 read-fast 表达式保持，因此显式恢复仍走完整 fast=false；内部 faulted-forward rollback、drain／stow／invoke 及递归回调继续 opt-out。所有 current／getter／schema／VM 次数、原字段所有权和三组恢复次序保持，written 日志、手恢复、几何与原 100ms 时钟不改。

实际抽取新旧 Backend／read／lease／invoke／Scope／恢复 wrappers 的有限 CPU 对照通过 18 场景、28634 次逐事件检查（不是同等数量独立场景），覆盖父 TLS 两态、完整／部分／撤权／失败写后清理和内部故障回滚。首次 strict 编译因 fixture if constexpr stub 缺 else 触发 C4702，保留原件，仅修 stub 后第二编译及唯一执行通过；生产 stage 不变，旧 24／227 算法 suite 未重跑。producer 813317d8…30f6，Astra 1602f5a0…d7a 无阻断；报告原句把 ticket110ms 误称 Plan 年龄已修正，原报告保留，实际 Plan context 32ms。root 核对 42 份 producer 工件和原 777 源后仅按原始 stage 字节替换 phase1，其余 776 源保持。原件 CRLF 与新增 3 处 lone LF 明确区分，精确局部逆变换可还原原 raw 文件。候选尚未完成 PG 构建或实测，不能据此宣称节省量或 TTL 通过。

## 2026-10-24 — 显式恢复候选完成构建，第二次应用仍因源姿态超时退出

基于提交 656bf77d…40ab3，新目录首次 configure／完整 PG TU build／freeze 均通过，保留 /W4 /WX、C++17、原 26 项配置和 23 个导出。PG bc1f7fa5…e335，组合 manifest eadd3a44…50a；777 源、27 依赖、99 工件／904 实体逐项核对，其余 86 份父工件逐字保留，新增 12 份来源证据。Native d54ecf18…82c9、XR 0463e75f…2eff、loader 66946ea6…2df 未重建；父 OFF 证明仅按原 SHA 继承，没有重复预处理或 CPU suite。未安装默认入口。

全新 w 入口绑定 209 依赖、26 复制和 24 个实际模块，在连续三次空闲观察后进行一次购买版后台运行，保持 mute／behind／observe-only 和实体手柄后台输入关闭。双手 lease／restore 各 1、setter 8；枪体 acquire／restore 各 1、setter 6，第二次尝试在第一个组件 setter 之前被拒。唯一 later-Tick 枪体相对物理 D 的位置误差 0.00342669 原始单位、完整旋转误差 0.00067336°，raw 配方与资产协议通过。这不构成持续运动；原计划的基线 4 次、每移动阶段至少 8 次、收枪与取回均未完成，整局保留 9 项失败。

实际 terminal tick2445／site0／attempt2／sequence827 的 postflush 授权是通过的：全部 127 已知字段、checked8191／rejected0。随后同原 ticket／stereo／input 时间的组件 group1 授权 ordinal339 才在 certificate_recent／1024 被拒；commit 时 stereo 源龄 107.8788ms、input 源龄 107.8628ms，MS ticket 年龄 94ms、距其截止尚有 6ms。因此此次是 NS 源姿态过期，不是把 postflush 通过误记为拒绝，亦不能用历史 first_failure 代替关联。该帧 pre→drain 40.7516ms、drain→post 7.3301ms；postflush commit 时原 stereo 已 89.6541ms，到组件 group1 commit 又经过 18.2247ms。各值属于相应观测区间，不能相加嵌套桶，或把相对 v 的跨局差值当作确定节省。100ms、原始时钟与 Plan、全部 getter／guards／恢复次序保持。

Native／XR 子会话通过 67 层、42 次场景拷贝、跨度 0.7163702 秒、最大提交间隔 16.0716ms；left history 创建／释放／销毁各 1、失败 0。Stop／Shutdown 均 0；8 控制、44 字节挂接、3 raw 组件组和原入口恢复相符。组件 faulted=1 如实保留，dirty／owned／revoked 均 0，不把安全卸载写成无故障整局通过。root 以创建时间和终止状态核实确切 owner／game／XR 均退出，game exit1；8 个音频 session 恢复，pending／read_failed／mute_failed 均 0。w 共 126 证据加 seal，经 root 重哈希闭包通过，seal c5b02126…2d8b、result 07845c6b…da5e；原 v／u 封存保持。新的 collector 分别保留 terminal callback 的 postflush 结果和按原时间戳关联的组件拒绝，明确 gate 行自身没有 tick ID。后续只定位剩余开销，尚未证明持续双手／枪体、完整暂收返回或全武器功能。

## 2026-10-24 — 同帧前置检查的名称解析接线

独立几何复核 5d8d1a4a…607c 确认 w 仅一个 later-Tick 样本通过，连续运动／收回／取回均未覆盖；Astra 的真实时序诊断 53fda8d4…97b9 明确区分 postflush 通过与后续组件拒绝。w 未开名称 scope 的 post→原 reader 时钟为 12.0372ms、reader→postflush 为 21.0325ms；这是可定位的原检查窗口，不能把全部 33.0697ms 当作可省时间。第一笔成功的组件 initial→tail authorize 仍需 54.6277ms，因此下一候选也不能预先声称足够完成整局。

只在 post_tick 原 active／session／clock／deadline 检查之后、首次 Snapshot 之前增加 phase1 名称 Scope，在 component_apply 之前用第二只 Scope 恢复入口保存的 TLS。非 phase1 继承原入口值；两声明都只在 HAND／CERTIFIED／COMBINED 同时开启时存在。原 Plan／时间戳／声明位置、read-fast、全部 getter／guard／VM 顺序、100ms、现有 tail Scope 和 invoke／drain／stow opt-out 逐字保持，只有两处共六行插入，无新缓存、权限或时钟。

实际 Scope／InvokePause／invoke 和两处 literal 声明的有限 CPU 生命周期 fixture 首次严格编译、首次执行通过 46 场景／5680 检查；8 宏组合是同一 fixture 中的声明 guard probe，不是八个完整 PG 构建。它验证父 TLS 两态、phase0／1／2、早返、临时 opt-out、递归 callback、component 入口和两 Scope 逆序析构，不运行整个 post_tick 或真实 getter／engine，不能证明性能或 TTL。两次准备脚本因 CRLF 假设及残留 marker 在任何源码副本写入前拒绝，原件保留；root 初次只读 extraction guard 误将 fixture 独立 namespace 闭括号当作生产连续片段而拒绝，修正比较边界后通过，未重跑 CPU。producer f2f895cf…aa80、Astra b2951336…c225 无阻断，root 验证 36 工件及原 777 源，仅按 raw stage cad39e77…d7cf 替换 stream 文件，其余 776 源保持。原 LF 换行保留，删除两块可逐字还原旧源。本轮完整 PG 构建与实际后台复测仍待执行，默认入口保持原发布组件。

## 2026-10-24 — 前置名称候选完成完整构建，第二次枪体写入通过但手部交还仍超时

以提交 35f1b19a…1f27 首次完整 PG configure／build／freeze 通过，保持 /W4 /WX、C++17、/MT 和 23 个导出。PG 41684207…2f50、组合 manifest 85ec227f…c24a；777 源、27 依赖、114 工件／919 实体经 root 三方字节及精确闭包核对，保留父 98 份非 PG 工件，新增 15 份来源证据。Native／XR／loader、CPU helpers 与原配置未重建；OFF 证明仅继承原件，未重复预处理。新 x 入口只变 tag／构建来源等顶层常量，原 54 个 runner 和 25 个 audit 函数／class AST 均相同；209 依赖、26 复制、24 实际模块和独立 prep 核对通过。

连续三次空闲后完成一次静音／behind／observe-only 的购买版后台 x。双手 lease／restore 各 1、setter8；枪体 acquire／restore 各 2、setter12，第二次三组组件设置均成功，随后 after-component 的手部再次授权拒绝，reason=hand_reauthorize_after_component。dirty／faulted／relinquish 均 0，日志安全卸载成立。原计划的基线4次、两移动阶段各至少8次、收枪和取回未完成，正式审计保留12项失败。

actual terminal 为 tick2544／site0／attempt2／serial31／sequence811。ring 中较早的 postflush 是 PASS；末次 tail authorize 的 QPC 区间 [437956789156,437956789161] 精确夹住完整授权 ordinal449／group0 的 authorize_ns 与 commit_ns，且 frozen ticket／plan／stereo／input 与 terminal 一致。因此这次不是用历史首次失败替代原因：已知字段127、checked2047、rejected1024，commit 时原 stereo 源龄105.6435ms、input105.6284ms；MS ticket年龄94、其截止尚余6ms。该 Tick pre→drain36.0678ms，其中手恢复总段10.2637ms；余下25.8041ms只是剩余窗口，不能直接称作单个组件函数耗时。post→原 reader 时钟5.6105ms、reader→postflush8.3708ms，总前缀13.9813ms；对比 w 的33.0697ms仅作跨局观测。postflush时原源龄59.1361ms、尚余40.8639ms，而该次组件 initial→tail commit 又用46.5072ms；原100ms门与所有时钟保持。

退出后的独立三组组件快照在 preflight._identity 的两次0x60对象头读取不一致时拒绝，未取得最终快照，restored_component3_exact 缺失。没有保存具体变化对象／字节，原因保持未知，不推断成组件未恢复或忽略该错误。正式 component_protocol／geometry 因独立快照缺失失败，component_rows 未生成；collector 将已审计行数与几何最大值均保留 null。外部只读数学诊断 d9420286…4889 从首枪 written→pre_restore 的 Tick2545→2546 重算：位置0.00855038原始单位、完整旋转对D为0.01546852°，raw C=[-1660,-4910,398]符合从未量化D逆组合后只量化一次。第二枪仅有pre_restore、没有对应written或后续独立Tick，不能计为第二个几何样本。该诊断不提升正式协议／几何／整局结论，2/2 journal totals 也不替代最终 raw 恢复证明。

Native／XR 子会话通过59层、42次拷贝、跨度0.6471454秒、最大提交间隔13.9737ms；left history创建／释放／销毁各1、失败0。Stop／Shutdown均0，8控制、44字节挂接和原入口恢复通过；确切owner／game／XR已退出，game exit1，8音频session恢复且pending／read_failed／mute_failed为0。x共108证据加seal，root逐项重哈希闭包通过，seal ca377c6f…73e7、result49c77c48…1f26。首次collector因失败审计根本未生成component_rows而KeyError拒绝，其脚本与说明保留；只修正缺失键表达并保持unknown后封存，receipt／audit／游戏未重跑或改写。下一步继续定位手恢复中的实时名称解析和后置快照拒绝，未放宽任何期限、原始身份或恢复校验；默认入口和公开包保持。

Astra 独立诊断 8d0bbf1f…884c 以完整授权 NS 与 tail QPC 夹取确认末次原因，并分别记录 postflush→tail commit46.5074ms 和 component initial→tail commit46.5072ms。唯一建议是仅在 pre_tick 显式 drain 的手恢复词法区间选择现有名称算法，其他动作／终止入口默认关闭，原外层 false、递归 opt-out、全套恢复顺序与100ms保持。手恢复10.2637ms是整个观测段，尚未分测可省比例；不能预先承诺覆盖5.6435ms缺口。当前没有该候选 stage 或新一轮构建／游戏，14项诊断输入重哈希保持，外部报告不修改x封存。

## 2026-10-24 — 显式 pre_tick 手部恢复的名称解析接线

仅在 drain_journals 增加默认 false 的手恢复名称参数，并给原手 restore 调用套入局部 Scope；GT 检查后的唯一 pre_tick 调用显式传 true，其他五处动作／失败清理调用保持默认 false。原最外 Scope(false)、dual_draining guard、枪 prelog→手恢复→枪恢复的顺序与返回式保持；手恢复失败或结束仍先退出局部 Scope 再尝试枪恢复。Stop／ended 不成为恢复准入门，实际 restore 整函数、Backend／lease、getter／read-fast／guards、原100ms、invoke及递归 opt-out逐字保持。手 Scope 内可能执行原 end/dump 日志，其真实闭包未增加引擎写入或权限。异常只恢复 TLS，不新增原协议没有的异常清理。

实际旧／新 wrapper、原手 restore 与 GT admission 片段的有限 CPU 对照通过34场景／13604次逐事件检查，默认与显式false均等价于旧 wrapper。首次编译因提取头CRCRLF触发 /WX C4335，确切输入和失败日志保留；仅修三份提取头换行后第二编译／唯一执行通过，生产stage不变，旧算法及宽suite未重跑。producer0acc9930…bdeb、Astra ffd654e0…cb5c无阻断；root核43工件及原777源后应用两份raw stage，其余775源保持。原LF及三处精确逆变换可还原旧源，不宣称实测节省或TTL通过。本轮完整PG构建和实际运行仍待执行。

另作有界架构复核1af597ef…7420：逐Tick完整恢复／重新acquire及正常三组恢复中的96次slot getter是当前一帧试验协议，尚非引擎必须如此的证据；现有clean-only policy与lease也不能直接跳过。持续owner、逐帧owned→owned更新是后续候选方向，每次仍须新Plan、live guard及100ms；跨Tick持有的原动画反馈、字段覆盖和动作边界需先做受控实证。x中240次slot Args::run/invoke累计0.162ms属于嵌套forward桶，不能把主要成本归为getter执行或叠加桶。通过100ms和有限几何仍不等于流畅、完整的6DoF交互。本轮开始发现另一局游戏与音频守护，离线工作未干预它们；后台运行仍需新的空闲核验。

## 2026-10-24 — 手恢复候选构建完成，增加严格后置快照观察并等待空闲

以提交b8943521…97aa首次完整PG configure／build／freeze通过，/W4 /WX、C++17、/MT及23个导出保持。PG ea35935b…0838、组合manifest295883c3…9557；777源、27依赖、126工件／931实体经root三方字节与精确闭包核对，父113份非PG工件逐字保留，Native／XR／loader及CPU helpers未重建，旧OFF证据只继承未重跑。两份被修改的PG include以原raw stage冻结；普通启动器及公开包未安装此候选。

新的y测试入口在原Shutdown后的唯一组件观察位置，内联最多三次完整快照尝试，仅snapshot阶段的精确PreflightError及原header错误文案允许追加。每次新建MeshPreflight并前后核原Bridge／Reader／identity／profile／source guard，仍使用原Resolver与缓存；失败partial不返回或拼接。只有完整成功结果才进入原三组raw比较与正式audit，其他类型／身份／来源错误立即停止。首次观察保留旧预算行为；追加尝试用同一个原Budget作准入、读前与接受检查，不创建新deadline或sleep，不能抢占同步读取，晚结果保持unknown。原preflight c827942d…67d及x封存不改，旧x的独立raw恢复仍未知。

实际原完整MeshPreflight.snapshot与既有内存fixture的首轮17类／60检查通过，覆盖churn后成功、持续失败、typed／identity／link拒绝、预算不足、晚结果与partial丢弃。producer078a3a31…0c3c、Astra177f5b74…1634通过；root独立AST核对新增一个inline helper、仅run函数改变，其余53既有定义保持，包括Budget创建点、prepare／verify、原raw比较及清理helpers。root第一次重核历史输入因刚应用的独立XR C++源码增量而拒绝，已保留该只读guard失败；改按旧冻结源与已批准应用证明核对该项，所有Python／helper／runner实际输入仍匹配，没有重跑旧suite或改写producer历史。

y的runner e50712bc…40dc、audit4e2be1f6…fde1、prep69c06d71…355e已绑定并核209依赖／26复制／24实际模块；绑定阶段55个runner与25个audit定义逐一保持stage AST。新的后台等待程序已启动，要求三次连续空闲后才进入一次性测试，并再次核原prep；保持mute／behind／observe-only、禁实体后台手柄和无自动重试。实际观察到另一组game／launcher／XR／audio仍活动，本轮准备核验时尚未启动自己的游戏，未干预外部进程。此处记录的是准备与活等待，不是y运行通过；之后以新receipt和封存为准。
## 2026-10-24 — 手恢复候选实际运行结束，持续持枪协议转入离线实现

已准备的 y 一次性入口在连续三次空闲观察后运行一次购买版，waiter 正常结束并返回真实失败码 1。保持静音、窗口置后、tiny-output 只观察和实体手柄后台输入关闭。双手完整 lease／restore 各 1、setter8；枪体完整 acquired=1、restored=2、setter10。第二次尝试只写入 Absolute 与 Translation，在 Rotation/group4 实际调用前拒绝，随后恢复两组，故恢复次数多于完整接管次数；不得以 acquired==restored 为原协议不变量。终局 hand faulted=0、component faulted=1，dirty／owned／revoked 均0、safe_to_unload=1，故障与清理结果分别保留。

actual terminal 为 tick2485／site0／attempt2／serial35／sequence829／revision28。其 postflush 完整授权仍为 PASS，checked8191／rejected0；随后组件 group4 ordinal441 以相同原 ticket、Plan、stereo／input 时间关联到该帧，而非仅凭 pinned first_failure 标签归因。失败授权 known127／checked2047／rejected1024，commit 时 stereo 源龄102.1423ms、input102.1273ms；MS ticket年龄94ms、截止仍余6ms，后续 policy 未执行。该 Tick pre→drain34.6887ms，其中本局唯一手恢复span5.7297ms，其余28.9590ms只是剩余恢复窗口；drain→post1.9459ms、post→reader时钟6.0702ms。postflush commit 时原 stereo源龄59.5865ms，至 group4 commit 又过42.5558ms。保留原始100ms、时钟、Plan、getter、guards及恢复顺序，不以跨局差值宣称确定节省。

退出后新观察 helper 在第1次完整快照即成功，三组raw组件、8控制、44字节挂接和两个入口独立恢复相符；没有执行第2／3次快照，不将本局写成瞬态重试恢复实证，也不回填x的未知快照。正式审计仍有11项失败，component_protocol先因未完成第二lease的样本不满足完整录制协议拒绝，component_rows未生成，几何样本数及误差仍unknown。一次成功的later-Tick日志pair可供独立数学诊断，但不能升级正式几何、持续运动、收枪与取回或全武器验收。

Native／XR子会话通过81层、58次场景拷贝、跨度0.8871504秒、最大提交间隔14.7458ms；left history各阶段1、失败0，仅记录提交与生命周期，不作FPS／GPU图像／头显验收。Stop／Shutdown均0，确切owner23108／game43000／XR48368以出生身份独立检查退出，game Wait0／exit1；8音频session恢复，pending／read_failed／mute_failed均0。root collector首次执行通过并封存105证据加seal，逐项hash／size／集合闭包核对，seal e1975344…b05b、result79a0f8f4…5e7eb、原receipt7711fb45…25ab。x及此前封存不改写，未重复游戏或旧suite，默认入口未安装。

Astra独立时序报告d8a7fefb…1640核对封存及9份冻结源码，明确第二次tail／第二次hand lease均未执行，checks11／12短路保持unknown。几何只读复核901aad27…27e3仅从第一组written→pre_restore的Tick2486→2487重算：位置残差0.000305182原始单位、对未量化D的完整旋转残差0.01398066°；actual raw C=[-1367,-5062,988]，资产O=[0,-16384,0]。这不是正式审计通过；第二次只有两组写入的partial行，枪矩阵与第一组pre_restore逐字相同，不能算新几何响应。两份外部报告均核105成员及seal，未改封存或重跑正式审计。

当前继续实现独立的continuous component owner：首次保存原始journal，逐帧用新的不可变frame授权owned→owned更新，取消／失追／动作时恢复。它不会跳过旧clean-only policy伪造干净状态；forward Backend必须在实际setter前调用新的四参permit，不能用旧三参setter的提前授权替代。部分写入、混合／未知字节、ABA、递归取消与撤权均需有限CPU验证，之后再做受控跨Tick持有实证。此时仍只有离线stage，尚未接入生产、构建新PG或运行持有试验。

## 2026-10-24 — 持续组件协议及真实类测试入库，运行适配仍待验证

新增 `tools/vr-common/weapon_component_continuous_lease.h`，首次保存原始字节后允许owned→owned更新，每次独立复制输入frame和目标值；单独记录pending／revoked与forward停止状态。每次更新有group0准入，实际变更的组分别使用四参Backend和单次final permit，三参接口仅用于恢复。部分setter返回false时按真实回读区分未写、完整写或混合／未知，撤权后旧值回现不会恢复写权限；cancel保留可认证的恢复权，relinquish保留未清journal，reset拒绝dirty／busy。不会把仍持有的组件伪装成旧policy的clean。

真实新类的有限CPU fixture首次strict MSVC／C++17编译和唯一执行通过30场景／1326检查；输入别名变更、逐组拒绝、三种部分写、未知身份／读取、已观察ABA、取消／撤权／递归、permit缺失／重复和恢复均有实际类场景。producer3f9924cd…0054，header3b1d9bf3…60b2，test343c0d57…6568，run.log d5e906f1…09f8。Astra独立窄审00e8e39d…5a00通过、无阻断，核对4份编译输入逐字一致，没有复跑CPU。generic C++不能证明真实setter物理位置或深拷贝输入，尚无engine Backend；未观察的write-and-return ABA仍不可仅凭key／value检出，no-change准入亦不证明返回全过程满足100ms或持有字段安全。

root核producer21份支持工件、Astra pins及原冻结777源，按原raw字节加入header与同字节测试，CMake只新增对应target和strict选项两行，原776源保持。工作源现779项，before6cf1b603…5435、applied0c6583c2…9dd1；新增header未被运行代码include，旧lease／policy／PG／Native／XR／默认入口保持，没有重建或重复旧suite。该次入库只提供可供后续适配的协议代码，不等于游戏内持续持枪、性能或头显通过。

同时对已有全量KB作有界查询，核对4份保存脚本：instant hit存在GetFireModeDamage→原int转换→TakeDamage的候选边界，Gnasher有自己的transient damage覆盖，投射物另走spawn／Init。来源注记4d8fcef1…718a仅是历史导出候选，含未解决token／type标记；未打开IDA或新增反编译，没有认证当前Result ABI、舍入、投射物伤害入口或实际0.7倍结算。单双手新规则继续标为设计确认、运行接线待做。


## 2026-10-24 — 默认关闭的两 Tick 持枪观察接线与有限测试

新增 `GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL`，CMake 默认 OFF，仅用于独立私有候选。第一次正常、完整的 phase1 枪体与双手应用及 written 回读完成后，分支关闭普通 reader／Plan／acquire 通路，保留真实 dirty 的枪 journal；下一 pre_tick 按原流程恢复手部，仅观察后续两次完整的最外层原版 PlayerTick。没有第三次、重新装载期限、新目标或新 forward setter；保留的证书仅作历史标签，首次应用的100ms门和原始source clocks保持。原版动作、Stop／Shutdown、身份／字段／schema／epoch异常和原15秒watchdog均进入原手→枪清理，不阻止原动作。

两个源码复核问题已在未运行的stage内修正：最终dump等待Token链退出，避免原动作尚未返回时把returned状态提前冻结；status5另行要求首／末清理结果已知且成功、两个journal无fault，raw恢复成原值不能掩盖false-after-complete。复制记录增加可解析raw3、完整Key／slots、矩阵、typed asset schema及计数；written未保存的专用比较标为unknown，不伪造mismatch。观察自身通常跨12个slot getter引擎边界，记录非原子；两Tick字段保持也不证明引擎重新消费了矩阵或连续姿态更新。

首次严格MSVC编译通过，首次执行完成32场景／175检查后，在递归终止测试的旧预期status7处失败。真实完整end在枪journal仍dirty时按原规则选择status8，随后枪清理成功，外层ended guard保留8。仅该fixture改为严格要求8、真实FILE sink的内层reason、无status5及clean/safe闭包；不改生产源或实际函数片段。首次失败b0af8848…b764及原输入／日志保留，第二次编译／执行通过39场景／202检查，结果4aa833b4…757a；未重跑已有宽suite。

一组实际MSVC `/EP` 对比通过：以原26项配置、HOLD未定义，在原位置展开四份PG源码的三条include，其余依赖include两侧同规则剥离；所有非空行字节一致。该证据b799a053…8ede仅证明这组源文本预处理，不能当完整TU／布局／ABI或运行证明。producer7648069…c328、Astra e9bae3ca…bad4及root9cca3058…3f0e通过；root应用五份raw stage，其余774份已跟踪源保持，共779源。新持续组件header仍未被runtime引用。完整PG构建、冻结和新的两Tick后台实测待执行，默认启动器及发布包未安装此试验，单手0.7／双手1.0的实际伤害链也仍待接入。


## 2026-10-24 — 两 Tick 私有候选完成构建，z 在试验前预检拒绝

以8c523b1b…9fa源码首次完整PG configure／build／freeze通过，保留strict编译及23个导出。PG a60c17ac…7094、manifest25eae851…0487；779源、27依赖、171工件／978实体经root三方字节及完整闭包核对，父125份非PG工件逐字保留，Native／XR／loader不重建。新增HOLD的源文本OFF证明单独绑定，不扩大为完整TU或运行证明。runner新增四个限定HOLD观察及关闭helper，原启动、15秒预算、100ms与清理路径保持；有限真实helper测试14类／132检查、独立审计39方法首次执行通过。候选未安装到默认启动器。

z 一次性入口在连续三次空闲后运行购买版，保持mute／behind／observe-only及实体后台输入关闭，真实返回失败码1。在初始component_baseline的owner-link读取中，原preflight._identity的两次0x60对象头一致性检查拒绝：object header changed during fresh read。失败owner及两份raw未保存，不能认定具体对象或字节、坏asset或HOLD缺陷。此前Native预留与图形hooks已真实启用；baseline_latch、Start304、15秒Budget、后续continuous Native/XR session及HOLD均尚未进入，五类HOLD日志为0行。

Shutdown返回0；8控制器、44字节挂接及两处hook入口前后原字节一致。post-Shutdown完整快照第一次成功，但初始component_baseline缺失，因此receipt的restored_component3_exact=false表示无法证明，collector明确保持known=false／passed=null，不能写成观察到未恢复，也不能补造基准。正式receipt和HOLD审计失败原样保留。音频8个session恢复、pending／read_failed／mute_failed为0；root以PID加creation time独立验证owner44692已不存在、game41796确切对象signaled且exit1，未启动后续XR进程。

z共84份证据加seal，seal edf7f29f…5d5e、result e2c82a33…ef60、receipt bbeaaf25…f0a1、独立诊断e6b4a0c5…e555。root collector没有改写既有证据或重跑游戏。下一步仅在新aa入口为这一初始只读位置增加最多3次完整快照重读；每次保留原身份、typed schema与来源guard，只有snapshot阶段的精确原异常可追加，不重做任何准备或写入，也不新建期限。两Tick、运动、图像和头显结论仍待实测。

单双手伤害规则的Astra有界复核5bce308d…4a8c确认候选位置：即时命中在原版伤害修饰后、原整数转换前；投射物在首次可能致伤前关联开火快照。现有回调的Result ABI、实际射击边界与投射物生命周期仍未认证，不能仅靠扳机状态或弹体返回后打标实现正确倍率；实际伤害接线继续待做。


## 2026-10-24 — aa 完成两 Tick 持有，独立修正审计后完整重审通过

新aa入口仅新增initial_component_snapshot并替换初始一处调用，OUT／TAG独立；原58定义AST保持，整个run逆变换一致，旧post-Shutdown helper不变。最多3次完整snapshot，只在snapshot阶段的精确PreflightError与精确header文案相符时重读；每次使用同Bridge／Reader／identity／profile的新helper，保留before guards，完整返回后才执行after guards并发布全值。失败partial不拼接、不作基准，不重做准备或写入；此时原Budget与Native启动deadline尚未创建，只有同步调用次数上限，不宣称硬抢占。实际preflight及旧内存fixture定义的新14类／114检查首轮通过，CPU00efd3bb…900d；source63c14b61…084a、peer144127fe…df52。producer总结5419d40a…424c保留一次因root已合法推进prepare而触发的旧metadata前提拒绝，不属于CPU或运行失败。

相同D66d冻结候选、PGa60c17ac…7094未重建。aa prep62452dbf…00df6及207依赖／26复制／24实际模块经root核对；三次连续空闲后运行一次静音／behind／observe-only购买版，实体后台输入关闭。初始快照第1次完整成功，未触发追加重读，不回填z缺失基准。Start／Stop／Shutdown均0，terminal5／lease1约在原外部Budget开始0.456秒后被观察。手部lease／restore各1、setter8；枪体acquire／restore各1、setter6、fault0。记录两对最外原版PlayerTick完整返回，枪体在观察期间追加setter0，完整owned raw3／Key／asset schema保持；前后同一对的mesh_tick与矩阵未变化，两对分别为2382／2383。这支持有限字段持有，不证明原PlayerTick消费了矩阵或持续新目标更新。

三组枪组件、8控制器、44字节挂接和两个入口独立恢复一致；8音频session恢复且pending／read_failed／mute_failed为0。Native／XR独立子会话通过61层、51次拷贝、跨度0.6560021秒，XR exit0；不作FPS／GPU／头显结论。root以creation time独立确认owner／game／XR退出，game exit1按实记录。aa共114证据加seal，seal1b991cc4…7fb1、resulta421a17f…3530b、receipt5538e971…8a3f。

原d291审计在时间门早停：host Start前／返回标签均50540984，而GT began50541000，晚16ms；原deadline50556000正好是began+15000。Start冻结实现仅设置requested，GT post_tick随后消费，原304请求由同一start_tick+1000给出armdeadline。因此要求began不晚于同步返回时刻没有producer契约依据。原正式receipt／383650bb…c4b审计保持失败，不能只凭terminal5跳过后续项目。

新外部auditor4b5d0b62…70fe0f只改该门：request<=return，request<=GT begin<=request+1000，拒绝溢出／bool，不从return刷新原armdeadline；其余30定义AST保持，原15秒和100ms不变。新增11场景／24检查首轮通过，含1000／1001边界、晚返回不刷新及下游journal损坏拒绝；未复跑原39方法。源审27f3922d…6b92、root33de52cd…af59通过；源文本逆变换只对规范化LF成立，新auditor CRLF不冒称raw相同。

随后对已封存aa只执行一次外部离线完整重审：e42febd5…c038的scalar／独立component／two_dispatch_retention／terminal_journals／controls-attachment-hooks／owned_cleanup六项全部通过，retention与cleanup为true。重审前后原115项完整闭包保持，没有再开游戏或改写旧结果；新解释只在外部工件。完整运动、全武器、引擎矩阵重消费、头显与实际0.7伤害仍未完成。

Astra201127a4…a378澄清generic Permit只禁止成功返回后到setter之间再有引擎边界，允许callback内部保留完整VM检查；最后getter后的direct身份／expected／schema／asset／实际callee-params与owner状态必须重核，原MS／NS授权最后执行。后续最小接线计划226e1cb2…b867保留组件先更新、手部后应用的真实clean语义、busy动作的journal与原ABI转发；已授权单独默认OFF源码stage，尚未应用、测试、构建或运行，不将HOLD结果扩大为连续owner通过。

## 2026-10-24 — 连续枪体更新接线通过主机检查并加入武器分支

CONT源码经root逐项审查后原字节复制五份文件，779源中其余774份保持，源树cb90d655…1ba3。新开关默认OFF，与HOLD互斥；四参Backend在实际PE／Native dispatch前执行显式Permit，最后一次引擎检查后重新核对完整身份、原始字段、手部状态、资产schema和实际12字节参数，再执行原MS／NS授权。每帧冻结完整输入，保留首次原值，动作／停止即时关闭写入权，busy期间延后完整恢复；恢复仍先手部再枪体，失败不自动反复写入或重开区间。

实际源码片段的dispatch测试11类／138检查通过，包括原值A→B→C后恢复A、晚到状态变化、schema和参数重编码、原100ms、真实SEH与递归dispatch。lifecycle测试10类／149检查通过，包括动作交还、setter内关闭、内部回滚、递归结束、恢复次序及只完成一次drain／finalize。合计21类／287检查，aggregate8ef6296a…bf8；root集成审查309e2a91…992a、复制回执cb1f2d3d…1d39。

所有早期失败保留：attempt1的命令包装在编译器前失败；attempt2的CASE6测试替身过早触发状态变化，修正为实际读取顺序后，attempt3的dispatch完整通过。attempt3的lifecycle在/WX下因std::find的有符号字面量产生C4389编译失败；仅改五个unsigned后缀，在attempt4单独编译和执行通过，没有重跑已通过的dispatch。生产五源、提取片段和断言语义没有因这项修正变化。

默认OFF预处理首组仅三个格式差异导致严格对比失败，原件保留；修正格式后新一组两次/EP均0，完整复合输入的非空行字节一致，proof d7fd0c49…020。该证据只覆盖源文本，非完整TU／ABI或运行证明。CONT仍是一个有限Lancer owner区间，完整PG构建、实际更新与原生消费、全武器、0.7真实伤害和头显未验收，未集成默认启动器。

随后以e30e9a30…ce49源码完成一次完整PG configure／build／exports及私有冻结，原W4／WX、C++17、EHsc与MT保持，23导出相同。PG9c86ef31…c021、manifest9ffbdec0…a4d、bundle0b9b8910…cb80；779源、27依赖、333工件／1140实体经root完整核对，父170份非PG工件逐字保留，Native／XR／loader与CPU工具没有重建。新候选没有继承HOLD运行验收，也没有安装到默认入口。新的AB单区间更新测试源码及8类fixture已准备并独立源审通过；实际游戏测试尚未启动。

## 2026-10-24 — ab 首次CONT运行在初始装备交接停止，原值与进程清理核对通过

AB新流程经独立源审与root核对，实际8类Python helper／parser检查完成：首组7类通过，test05的ACK fixture设置20ns文件时间戳后等待失败；未记录实际stat返回值，时间戳量化仅为源码支持的解释。仅把该fixture改为完整2秒后，单独执行test05通过，其他7类未重跑，aggregate f25c3900…1420；runner／auditor不变。候选与208依赖、26复制、24实际模块绑定通过，prep965c4d72…d33，保持mute／behind／observe-only、实体后台输入关闭，连续三次空闲后只运行一局。

实际Start返回0，初始设备阶段0→1的yield日志dirty／fault均0、restored=1、helper_clean=1；CONT的PostTick此时operation_depth=1，最终同步交还判断因此返回false，外层以equipment_phase_handback_failed结束，terminal7／lease0。component acquire_attempts／update_attempts／commits均0，没有执行P0／P1命令。此时!dirty分支没有调用drain_journals；末尾drain_attempted=1／result=1来自clean no-op，不表示发生过枪体恢复。原有限片段测试未覆盖这条完整初始阶段路径，不能把21类通过扩大为运行通过。

失败Stop0、Shutdown0；三组组件raw、8控制器、44字节挂接和入口独立核对一致，8音频session恢复且pending／read_failed／mute_failed均0。root按PID与creation time只读核对owner／game／XR确切对象均退出。Native／XR独立子会话35层／34次拷贝、跨度0.3514457秒、最大提交间隔15.1906ms，通过不代表CONT或FPS／头显验收。AB正式scope／update／cleanup审计仍false，原receipt8a8e2f4f…6d77和失败不改写；外部root只确认原字节与进程清理，未替代计划Stop6的验收。

AB共142项含seal87067093…3109，root-result dd795275…6e7，全部闭包核对。下一步在独立stage区分未曾持有且全部clean的初始设备交接与真实busy持有，保留epoch撤销、原时限、已持有后的立即关闭和延后恢复；不重跑本局、不放宽drain guard或实际写入校验。

## 2026-10-24 — 修正CONT首次装备交接，受影响生命周期测试首轮通过

仅stream与XRstream两处源码变化，其余777源保持，源树cc7595a9…dc40。CONT私有双参helper只在观测到previous0／current1时收到初始提示；原单参入口仍传false，unused-clean早返回与同步depth0行为保持。新增depth1分支必须从未begun、lease0、无component快照、全部clean／无fault，且无action／closed／terminal／deferred／finishing／Stop／Shutdown／releasing／dual，保持原active与phase。提示仅表示初始干净交还，epoch推进、blocked观察和binding reset仍执行，当帧仍由equipment_changed与tick_epoch阻写；后续新票与真正Permit不变。begun后的动作与装备变化仍立即关闭，busy drain和一次恢复标志均未放宽。

root与独立源码审查通过；实际边界、资格条件和原恢复片段的生命周期测试14类／426检查首轮strict compile／run均0，含18种拒绝变体、snapshot与dirty独立、旧单参语义、后续纯Policy新票及begun阶段一次交还。CPU44109b61…1a38；未变dispatch11类／138不重跑，明确只沿用旧attempt3中通过的dispatch项，aggregate cd55d5ed…33a4为25类／564检查。纯Policy与component snapshot／restore等host seam不当作真实certified XR或引擎资格。

新OFF唯一pair两次/EP均0，26配置／15ON／11OFF、CONT与HOLD都undefined，完整非空行字节相等，proof35b32c68…1105。两侧同一受CONT保护include及全部移除签名清单相同；不是完整TU／ABI证明。root源审845ea1d7…a8aa7、独立ab3bf94b…b6a7、集成ba24778d…748b及原字节复制9bd15bcb…f1f2通过。源码已应用，修后完整PG构建与新AC实际测试尚待进行；AB失败保持封存，默认入口未安装试验。

## 2026-10-24 — ac记录到同一持枪区间的新姿态更新，计划停止验收仍失败

以修后提交f9145bfe…8ba完成一次完整PG严格构建与23导出核对，PG868b898d…9b6；父332份非PG工件原字节保留，Native／XR／loader没有重建。私有bundle2ffd36c1…e14a、manifest30f9fc8b…50a，779源／27依赖／401工件／1208实体经root闭包核对，报告486dd6df…99。默认入口未安装。AC运行与审计逻辑保持AB，仅绑定新候选和输出身份；已通过的8类helper结果沿用，未重复执行。208依赖、26复制、24实际模块准备通过，prep63ee41af…953；三次空闲后唯一一局继续mute／behind／observe-only，关闭物理后台手柄。

实际Start0，初始0→1的clean交接成功。同一个owner记录acquire_attempts=1、update_attempts=4、commits=5、component restore=1、setter=10；双手lease／restore各5、setter与target write各40。P0与P1各发送一次并取得新ACK，后续新源样本、双手目标和枪体回读属于同一完整Key、首次原值与资产身份，changed_owner_update子断言通过，记录到两组相邻变化。5次提交包含没有变化的提交，不能称为5个不同姿态，也不构成原生消费或画面流畅度证明。

主机在phase2／lease5时请求计划停止，Stop0、Shutdown0；最终raw327687为5个lease／phase7，原因continuous_post_tick_without_fresh_joint_commit，first_failure_known=1／first_failure=7、dirty=0／faulted=0／safe=1。原scope／update／cleanup整体判断均保持false。终止tick2449／attempt6实际上已有新serial33／sequence840、Plan revision33／accept_mask3，final_auth_called=0；源码与日志推断相关出口是首次prewrite授权返回0后静默返回，再由PostTick析构保守记7。该次授权的准确拒绝位已被恢复阶段环形日志覆盖；Stop store时刻未记录，不能凭主机调用相近就认定停止竞态，也不能断言TTL过期。窄审394010e0…7632仅提出后续局部捕获本调用原拒绝分类，不重标AC，不按晚到Stop抹掉其他失败。

root独立核对三组组件raw、8控制器、44字节挂接和入口恢复，8音频session已恢复且pending／read_failed／mute_failed均0；按精确PID与creation time只读确认owner／game／XR全部退出。Native／XR独立子会话97层／54拷贝、跨度1.0544409秒、最大提交间隔14.5587ms通过，XR exit0；不作为FPS或头显验收。AC原receipt f7648519…5e6、正式失败审计和110项封存保持，seal1720be63…cc26、root-result e15c1004…f1a；没有自动重放。下一步只修正有直接证据的写前取消分类，保留未知、时钟／Policy拒绝和既有失败的7／8，以及原100ms、15秒／256和恢复顺序。全武器、实际0.7伤害与头显仍待完成。

## 2026-10-24 — 写前授权取消分类通过受影响测试并接入独立分支

仅XRstream一份源变化，其余778源原字节保持，源树ead5b55c…62f3。CONT在原有首次prewrite授权调用外使用全新局部AuthObservation，调用次数、参数、时钟与原检查顺序不变，返回后先恢复上层observer再记录或结束。只有本调用确切早期Stop／Shutdown拒绝，或已具备当前Plan内有效右手授权的晚期Stop／Shutdown拒绝，才请求状态6；缺失观察、cutoff／时钟／年龄／证书失败、Policy0或缺右手权限均请求7。原PostTick保守兜底、failure sticky和恢复失败7／8升级保持，没有读取晚到Stop重标旧失败。

真实授权、capture／classifier与原生命周期片段组成13类／350项主机检查，首次严格MSVC编译和运行均0，CPU e3e67d3b…1c79；覆盖原检查顺序、父observer恢复、当前调用包、有效权限子集、已有7与清理7／8及原手部恢复。7个旧片段原字节沿用、5个新片段核对实际源边界，Policy返回／时钟调度／快照与组件恢复仍是声明过的host seam；不是完整certified XR、实际引擎动作或消费证明，未重跑旧AC／dispatch测试。

运行前发现新增纯诊断行名落在旧auditor保留的continuous命名空间内，会被unknown-prefix检查拒绝；保留48个未执行准备文件后，仅将源、提取片段和fixture读行前缀同步改为hand_weapon_component_prewrite_rejection，审计器原字节保持。修正前没有执行CPU或游戏，不称为一次运行失败。原五pin检查器另保留未执行版，AD新OFF适配为单个已审XR delta加四份两侧同一冻结父上下文；唯一一组两次/EP均0，严格完整非空行字节相等，proof083cc697…660a，仍非完整TU／ABI。root新源审314c1725…fd4a、fixture审185367d6…fa66、集成7032c751…189d、原字节应用e6559d32…98b均通过。完整新PG构建与AD实际局尚待进行，AC110项封存和失败不改写，默认入口未安装。

## 2026-10-24 — ad单个持枪区间的连续更新、正常停止与恢复审计通过

提交95705a8b…2180随后完成唯一完整PG严格构建，23导出保持；PG28ce0a16…73f7，私有bundle d2ad1c9a…20af／manifest9739413d…3521冻结779源、27依赖、479工件／1286实体，其中新增78份必要证据、父400份非PG工件原字节保留。Native／XR／loader与原配置没有重建或替换，root闭包报告99b94d92…8d5a通过。root元数据核对的Windows路径分隔符、派生manifest来源标签解释错误在游戏前保留并修正；没有重跑通过的CPU／构建，候选本身未因此改写。

AD runner仅变更AC的OUT／TAG，auditor原字节2be21ee5…1470；已通过的AB helper8类沿用，不重复执行。prep3fd3adc3…4881绑定208依赖、26复制（含一份按固定私有DLL路径派生的模拟器manifest）、24实际模块，root复核af883884…92dc；quiet三项仍为mute／behind／observe-only、物理后台输入关闭。三次新鲜空闲后只运行一局，没有自动重放。

实际Start0，同一个owner记录acquire_attempts1／update_attempts3／commits4，枪体setter10／restore1；双手lease4／restore4。P0和P1各一次且后续新源与同身份枪体字段确认两组相邻变化，正式changed_owner_update通过；4次提交不表示4种姿态。计划Stop0／Shutdown0、最终raw262150为lease4／phase6，结束原因xr_stopped_after_restore；无first_failure、dirty或fault，safe_to_unload1。原正式scope、update、cleanup审计均通过，receipt0e10eb95…2faf保持其受限scope，不称整局／全功能验收。

本局新prewrite_rejection行数为0，因此不能称新写前取消分支已在游戏中演练；它仍由13类／350主机检查覆盖，AC原准确拒绝原因也仍未知。此次实际成功证明当前有界Lancer owner可以跟随新输入、正常关闭并恢复；不是生产会话重新开区间、原生重新消费、完整动作交还、流畅度、全武器、0.7真实伤害或头显结论。

root另外比较原组件Key／links／资产身份、三组值、完整8控制器与44字节挂接，入口证书审计通过；8音频session恢复，pending／read_failed／mute_failed均0。用Win32只读PID＋精确creation time核对owner／game／XR确切对象均退出，无终止或窗口动作。Native／XR独立子会话passed与XR exit0保留，不扩展CONT范围。AD共87项含seal5a7c27c0…bc90，root-result35ceafb8…db5b，外部Native／waiter证据已复制入封存并逐字核对；AC110项及其失败不改写。普通启动器未安装未完成的6DoF候选。

## 2026-10-24 — 持续组件会话源码、关闭路径和严格构建

新默认关闭开关 `GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL` 依赖原 CONT。四文件新增211行，把一次 Start 的 session 与可结束／重开的组件 interval 分开；正常可同步交还的动作先撤销旧 Plan、恢复手和组件，再按原 ABI 执行动作。下一个真实 pre_tick 仅在零操作深度、双方已干净且没有故障／撤权／Stop 时重置该 interval；等待原稳定条件和新票据后取得新的原值，不重启会话或清空累计计数。

可恢复的年龄暂停限无 receipt、历史 binding_age 且证书没有失败的路径，同 Key／资产／动作代次和有效绑定继续核对；实际收到的拒绝证书、写前授权过期、unknown／foreign 和部分写失败仍终止。原100ms写前门、getter、typed schema、四参数 Permit、原动作转发和 sticky failure 保持。15秒／256次只结束诊断录制，寿命继续由显式停止等条件控制。此轮仍为同固定Lancer，phase2／无右手Plan会暂停整组writer，尚未在组件暂停期间独立驱动自由双手；空Weapon／更换实例也没有开放。

root已读新增路径并核对其余775份作者源原字节不变。新OFF唯一一组两次MSVC /EP均0，严格完整非空行源文本相同，proof07bd2b6a…e6f4；它不证明完整TU／ABI。首次完整PG构建遇到新局部pending隐藏已有请求队列的C4459，失败目录保留。仅将新开关内部两个标识符改为pause_deferred，源码提交014cebec…b0ba／修正3fb9ae4f…eef5；完整逆向两token比较与其余三源原字节相同支持沿用已执行的OFF结果，没有重跑该通过项，reuse ce960489…266a。最终源树037f8837…b5d0。

修正后独立目录完整PG严格构建通过，C++17／W4／WX／EHsc／MT与实际vcxproj核对，23导出保持；DLL2ff0ef1a…7b42。新模式缺少CONT依赖的配置按预期拒绝。此构建与受影响CPU准备并行，CPU结果和实际游戏尚待记录；没有重建Native／XR／loader、启动游戏或安装默认入口。构建器位于忽略的analysis/weapon-component-persistent-session-build-r2-20261024ae/，首次失败与元数据修正均保留。

本轮另完成12种武器的接线边界与伤害属性静态复核。扩武器还受库存挂接、固定Marcus八控件、同实例Start及非零Origin等条件限制，不能只放宽枪名；Hammerburst作为下一种普通手持枪仍须自己的实际资源和恢复证据。伤害表确认base／Gnasher getter返回为浮点属性，表内序列化Offset不等于运行时ABI；Gnasher实际脚本候选先汇总一发的命中再结算。单双手0.7／1.0仍属已确认需求，实际返回值与逐发／投射物归属尚待接线，不宣称真实伤害已改变。

随后受影响生命周期主机检查首编首跑均0：7类／505项，CPU result a1bfe680…5162，fixture root审a0f908cd…364f。17个实际源码片段与38份完整输入逐字绑定，使用实际hand Lease／Backend、certified Policy和Session；覆盖正常action交还／fresh rearm、busy原动作撤权、PostTick延后单次drain、真实no-receipt历史年龄、fault不可复活、Stop及15秒／256录制与寿命分离。组件物理恢复、asset／stability与原动作正文仍为声明的host服务，admission_probe没有执行真实A2 acquire／setter，也不是完整诊断ring或引擎消费证明。没有重跑旧suite／通过的OFF对比，没有启动游戏。组件暂停时的独立自由双手已形成原三inl内的最小接线计划；后续必须处理手部新journal不能被旧组件drain成功记录跳过，再做实际联调。另将设计表中武器1与原库存编号分清：VR右肩武器1对应原库存槽3，手枪为0、手雷为1、左肩武器2为2，与已有body_slots实现一致。


## 2026-10-24 — 组件暂停时的独立双手接线及 AF 实测边界

源码提交 `8d1675561c5df0a1bbbf1cfb56ecc382b7948611` 在原三份 inl 中区分 joint／hand_only。已持有的组件须先完整交还；健康 waiting／suspended 状态下的新认证 Plan 可只更新双手，不调用组件 apply／owned 或伪增组件 commit。完整八控件写后核对、原100ms及同一Plan重验保留。手部新清理义务在 acquire 前登记，两个旧 drain 成功记忆均不能覆盖它；pre_tick 完成手恢复并退出 Operation 后，才在零深度重新准备组件。真正失败／撤权仍为 sticky，未知或失败的 joint 不会降级为 hand_only。

AF 三源 root 审查、原字节片段提取通过，CMake沿AE不变。关闭新开关的一组两次MSVC /EP均0，完整非空行源文本一致，proof `97be603d…d901`；完整私有PG严格构建通过，23导出保持，DLL `b5d1f458…c62d0`。受影响7类／539项主机检查通过，实际26片段／51输入，CPU `2f9c5a9b…b28a9f`。首次host编译因case内 next 隐藏同main外层 next 的C4456被WX拒绝，未运行main；原失败保留，新目录只改两个局部标识符后编跑均0。组件实际引擎操作、反射、资产和原动作正文仍有明示host服务，不能将该结果当真实再次取得枪体的证明。未重跑旧宽suite、AE录制循环或通过的OFF／build。

新私有候选bundle `d8b429cf…500f9`、manifest `75047f92…3c567` 冻结779源、27依赖、489工件／1296实体；Native／XR／loader保持AD原字节，未安装默认入口。后台runner新增三阶段：原持枪右手P0/P1、收枪后左右手各P0/P1、取回后的新epoch持枪P0/P1，仍是一套原15秒预算、两次在局tap及一次计划Stop／Shutdown。ACK后以原QPC整数ns公式建立固定源时序界限，input_ns／stereo_ns必须均越过它，排除旧输入在ACK后才打印。14项新离线helper检查通过，仅含合成验证器数据，不是新游戏证据。

三次新鲜空闲观察后唯一AF购买版后台局使用mute／behind／observe-only、物理后台手柄关闭。实际Start0，原持枪interval取得1次、更新4次、提交5次、恢复1次；双手应用／恢复16／16，其中hand_only提交11次。原持枪两条命令和收枪阶段四条命令各有独占fresh ACK及更晚源帧；当前槽stow10和return11实际ACK均有。取回后的joint等待被auditor对unused equipment_stow_state中合法空 original_hand_pose= 的过度解析拒绝，未完成后两条命令和计划正常结束。原scope／正式审计保持false；异常路径Stop0、Shutdown0、日志终态6且无failure／dirty／fault。原回执、失败审计与84项封存保持，seal `a293f407…35e73`、root-result `77dcad21…bd3f`。

root独立核对组件三组raw、完整8控件、44字节挂接和入口恢复；8音频session恢复、pending为0。Win32按精确PID／creation time只读确认owner／游戏／XR均退出，未干预外部进程或窗口。Native／XR独立子会话通过、XR exit0，不扩大为6DoF或头显通过。AF失败原因是host读取诊断的异常；这不表示实体手骨已经达到目标。

对原AF日志再算11个hand-only样本、22个手部配对，6个位置配对超过原0.1单位门限：左P0连续四Tick误差约3.13–3.27，右P1连续两Tick约2.92–3.07；最大3.268784原始单位，本局100单位／米即约3.27厘米。旋转最大0.020772°，bind／initial和另一些姿态接近浮点误差。selected6子集的最大3.181473不能替代整段最大值。当前同epoch8／后续bind_mask0、pre_restore比written晚一mesh Tick、另一手正确，支持可达范围限制的假设，尚缺同帧肩／肘数据，不能定为已证IK根因。派生只读摘要 `cbbc7b52…07d5` 位于 ignored `analysis/weapon-component-persistent-hand-only-root-review-20261024af/af-hand-reach-boundary.json`。

AG审计器仅收集本scope实际使用的phase／tap／end三类额外行，忽略未用于证明的stow调试行；PERSIST精确字段及原检查保持。新8项窄回归重现原拒绝、确认修复、继续拒绝关键字段缺失／重复／空值，并确认AF不能冒充完整往返，旧84项原字节保持。AG仍authorized=false、未prepare／游戏，先处理物理位置偏差，不靠缩小P0/P1或放宽0.1门限刷通过。下一步用现有合法GetBoneMatrix只补同Tick肩—肘—手链，检验目标半径、两段臂长与实际伸直方向；实际再次取得枪体、完整动画接管、全12武器、0.7伤害与头显均未完成。


## 2026-10-24 — AH 同帧肩肘诊断确认手部可达边界，物理精度仍失败

源码提交 `353f989016e69df26eddd322562d97d89123f39e` 只在原 stream／phase1 中新增默认关闭 PERSISTENT 诊断，193 行插入，其他 777 份作者源保持。整会话最多 16 条 POD，在实际成功 pre_restore 样本之后，通过现有 MatchRefBone／GetBoneMatrix 读取肩肘，逐次核对实际索引、七项身份、lease／sequence／epoch 和首尾 Tick；unknown、失败或重入只放弃诊断，原 lease.restore 必然继续。原动作 hook、100ms、15s、动作、目标、门限和 writer 不变。静态父链与非原子读取限制明示，未猜测伸长字段或改手臂。

原包中当前 Marcus 的 115 条 RefSkeleton 记录已离线严格验证，左右手分别经前臂连到上臂，Armroll 为旁支。首次读器因四个眉毛骨带 FName instance number 而拒绝，原失败／源码保留；私有 R2 按现有 tag reader 的编号规则解码，所有姿态、父索引、实际直接子数与完整名称唯一性通过。没有修改生产武器模型读器、导出几何或等同静态与实时资产字节，派生数据仅留在忽略的 analysis。

root 源审 `06bf5166…b4a29`、关闭开关的一组两次 /EP 完整非空行字节对比 `56c86a29…f123` 通过；完整 PG 严格构建通过，C++17／W4／WX／EHsc／MT 与实际工程核对，23 导出保持，DLL `bbf89d05…b2642`。未重跑 AF 已通过的 7 类／539 项生命周期 CPU，也不把旧结果称为新诊断的 CPU 证明。Native／XR／loader未重建，默认启动器未安装。最终私有 bundle `063fa1dd…01c6f`／manifest `aa3a85ad…9cef9` 冻结 779 源、27 依赖和 498 工件；第一次 manifest 把 DLL 输出名混入功能开关表，被 prepare 拒绝，原候选保留。修正后缺少 test-plan 的第二次 prepare 拒绝也保留；补齐后 prep `25d55384…83910` 通过，两次均未启动游戏，未重复编译。

三次新鲜空闲后只运行一局购买版，mute／behind／observe-only、物理后台输入关闭。原 8 条姿态命令、收枪10／取回11及独立 preStart latch 完成；双手 lease21／restore21，hand_only11，枪体两区间累计10提交／2恢复；计划 Stop0、独立 Shutdown0、终态6，无 first_failure／dirty／fault。修后的空诊断字段解析通过，本次正式审计仍为 false：部分实际 later-Tick 手骨未达到原0.1位置门限，不因命令流程完成而提升验收。

新增11行均 status10／known63、四个 per-bone mask均15，和原pre_restore手矩阵、目标、身份与Tick关联。22个手部结果中6个超限，6个均与实测肩中心的臂链最大长度径向投影吻合：最大位置偏差 `4.1072294549` 原始单位（本局约4.11厘米），最大径向预测偏差 `0.0001223167` 单位。支持手臂可达限制这一几何解释；实时父子边和原生求解器内部截断未因此证明，物理修复尚未实现。保留原P0／P1、0.1位置／0.1°旋转与原时限，不能缩动作或放宽门限取通过。

正式 cleanup_passed=true；三组组件、完整8控件、44字节挂接、入口恢复，8音频会话恢复且pending0。原Native／XR独立子会话passed与XR exit0分开保留，root只读Win32核对确切owner／game／XR均不存在。后处理记录器曾把 acquired 错当区间数，首个断言失败保留；按现有计数语义核对 acquired=10提交、restored=2区间后记录成功，不改原游戏审计。AH93文件封存 `2eac19f5…3952b`，root-result `4600defc…b373`，实际几何在忽略的 `analysis/hand-reach-root-review-20261024ah/actual-reach-result.json`；旧AF84项封存和AG离线结果均不改。

后续最小候选为单独默认关闭的尾部 SingleBone 世界位置实验，先取得实际平移字段、空间／加法模式与 Bool mask 的 typed schema，再把原值、owned值与部分写 pending 纳入完整扩展 journal；不能只放宽 bApplyTranslation 检查，也不能替换原 Limb 位置通道。必须验证动作前恢复、原Plan、精确写后值、手—枪关系、前臂蒙皮及后坐力，再决定臂长校准。全部12种武器、体感装填与真实0.7／1.0伤害入口仍未完成，本记录不构成发布或头显验收。

## 2026-10-24 — AI 平移类型实测与扩展事务 CPU 检查

只运行一局静音、置后、observe-only 的购买版，沿用 AH 私有 DLL，新增外部只读类型观察。左右末端 SingleBone 的 BoneTranslation 均为 12 字节 Vector，X／Y／Z 为实际 FloatProperty；BoneTranslationSpace 为 1 字节 ByteProperty，并与 BoneRotationSpace 指向同一个已认证 EBoneControlSpace。原值均为零向量、World 空间和 None 空间骨名。bApplyTranslation／bAddTranslation 为同一 4 字节字中的不重叠 mask 1／2，原字仍含旋转位 4，故写入不能覆盖整个字。此处为实际类型和值观察，偏移没有作为生产 writer 常量；尚未证明原生平移消费与画面效果。

AI 在原有 hand_reauthorize_after_component 步骤以终态8停止：初始／收枪六条姿态命令完成，双手16次应用／16次恢复，含hand_only11；枪体两区间累计6次提交／2次恢复。first_failure=8，有限诊断环只保存前8条、丢弃9条，没有取得失败重授权的完整子条件，不能猜定原因。正式 passed=false、cleanup_passed=false 保留：原审计拒绝失败状态，旧严格结构也不接受附加的 translation_observation 元数据，失败路径没有正常计划 Stop 的完整次序。独立比较确认8个控制器（连同新增只读平移观察）、三组枪组件與44字节挂接前后相等；failureStop0／Shutdown0、入口恢复、8音频会话恢复且pending0，自有owner／game／XR均已退出。独立Native／XR子会话通过不替代整局结论。AI63文件封存 `85b33978…0475f`，typed报告 `21664f58…ba37b`；不修改 AH／AF 原回执。

新增 `hand_tail_translation_lease.h` 保存两个手的原值、owned、单步暂态与部分写 pending，在精确+0强度下按字段准备与反向恢复；未知或外部值永久撤销该手 journal 的恢复写权限，另一手仍须尝试清理。`hand_ik_tail_lease.h` 将它与原四通道 Lease 组合，先准备扩展字段再激活，始终先恢复原通道再恢复扩展；最后一个setter中的重入／上下文关闭也不能被正确数值读回掩盖。真实 backend 仍须逐次认证对象／schema／原Plan／mask和expected字节，不能用aggregate fault借用primary内部回滚的写入旁路。

root与Astra完成源码审查后，仅构建并运行新增实际CMake目标 HandTailTranslationLeaseTest 一次，C++17／W4／WX／EHsc／MT 与实际Release工程核对，28类／370项检查通过。测试直接使用三个实际头文件及声明过的内存服务，覆盖false-before／after、四字段未知／外部／torn、精确+0、共享字保留、输入冻结、单双手、嵌套恢复、最后setter和双层清理。冻结782源／27依赖；结果 `ef3c3b38…0a01` 保存在忽略的 `analysis/hand-tail-policy-build-20261024ai/`。旧539项和已通过OFF／PG构建未重复执行。当前只新增两个策略、单一测试和CMake注册，没有游戏写入接线、Native／XR重建或默认安装；物理位置修复、0.7伤害和完整12武器仍未完成。

## 2026-10-24 — AJ 末端位置覆盖接线与严格构建，运行验证待做

新 `GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL` 默认关闭，仅允许已认证的固定实例 PERSISTENT 组合。实际 reader 逐次解析11个属性、Vector三成员与共同Enum，冻结对象和属性身份、大小／维数／偏移／mask，并校验字段边界与交叠；三个强度地址也属于schema。实际偏移与位值来自当前metadata，没有写死AI所见偏移或mask。末端真实平移值独立保存在Control中，只有同一journal整tuple被接受后才归一化原fixed标志；原Limb位置通道和四通道setter／target计数保持。新增字段另行计数，最终逐手检查原值／owned／未选中值和完整fixed。

实际 backend 先在零强度准备位置和位，再激活原通道；恢复始终先原通道后扩展。每次forward继续认证同一Plan／context／mask、原100ms、epoch与Stop，恢复不以过期forward权限阻断，也不跳过typed identity和expected检查。最终SEH写入立即从三个实际强度地址重读精确+0，只比较本字段，packedword只写两个选定位并保留当前其他位。原动作入口、hand→component清理和正常interval保留；两个完整Trial重建入口拒绝dirty／busy／恢复中与活跃operation。第一次源审发现普通snapshot外部拒绝没有锁存，失败报告保留；修后同一pending hand的早返、类型／tuple／完整fixed拒绝以及Backend后置拒绝都会锁存撤权，晚到相同值不能复权，另一健康手仍尝试恢复。Astra修后源码审查 `558a3a3d…0082` 通过，这不是运行证明。

冻结786作者源／27依赖后，首次新开关OFF五份实际运行源的两次/EP非空行字节比较通过；首次完整严格构建因新局部名遮蔽已有original／observed而被/WX拒绝，字段测试未执行。原源码、日志和失败保留。R2仅改三文件局部标识符，逐行逆向替换与首次冻结文本完全一致；新OFF对比再次通过，完整PG及HandTailFieldsTest严格构建通过，C++17／W4／WX／EHsc／MT与实际两个Release工程核对，23个导出保持，DLL `459e0f67…934bf`。

实际reader和最后compare-write直接由独立内存测试include，15类／198项检查通过；覆盖非历史偏移／mask、schema和身份变化、坏Enum／Vector／Bool／字段交叠、未知结果不发布、四字段精确写入、三个强度晚到非零／-0／NaN、stale expected、共享字新变化位保留，以及本进程PAGE_NOACCESS／空地址拒绝。registry／名称／field／copy-denial是明确的模拟服务，未证明游戏UObject ABI、完整Backend生命周期或物理显示。R2结果 `0e57ad3e…3a9a2` 在忽略的 `analysis/hand-tail-engine-build-20261024aj-r2/`。原370项与旧539项未重复；未启动新游戏、重建Native／XR／loader或安装默认入口。后续必须以新候选验证后续Tick手掌、原动作恢复、手臂蒙皮、手—枪与后坐力；完整12武器和真实0.7伤害仍未完成。

## 2026-10-24 — AK 新候选首次运行：后置授权拒绝，手骨零写入

冻结 AJ 严格 DLL `459e0f67…934bf`，786 作者源／27 依赖／509 候选文件，保留旧 Native／XR／loader。新审计器逐包验证位置目标、选手 mask、部分写 journal、独立计数和共享字其他位；先完整校验八份实际类型观察，才在内存副本中精确移除八个新键交给原严格核心，其他键及全部旧断言保持。46 项离线检查通过，其中实际已存类型观察与合成审计包不构成游戏证据。root 首次检查脚本误计函数数量而失败：实际是 16 函数及 1 类，13 函数及该类原样保留、另两正文保留为别名；首份失败保留，仅修正计数再做 AST 比较，45 项已通过的实际审计检查没有重复执行。

三次新鲜空闲清单后，只运行一次静音、behind、observe-only 的 Steam 购买版，物理后台手柄输入关闭。首个 component interval 提交一次后，尾部 snapshot、guards、component_owned 与其 live_read 均返回真，随后同一 Plan 的 authorize 返回零，以 `hand_reauthorize_after_component`／终态8结束；没有执行8条姿态命令，hand leases／setter／target／新增平移写入均为0。同一 QPC 来源的记录显示，尾 snapshot 入口证书源龄约101.3501ms、authorize入口约110.6981ms，原100ms条件已不可能满足；精确 Auth 包被后续恢复事件覆盖，不能将时间推导冒充实际第一个 reject bit，也没有证明是新 reader 的特定操作造成超时。

独立比较确认完整8控件连同八份类型观察前后相等，3组component与44字节挂接恢复、无入口恢复错误；component累计1提交／1恢复、failure Stop0／Shutdown0、音频8恢复且pending0。确切owner／game／XR只读核对均不存在，独立Native／XR子会话通过且XR exit0。正式 passed=false、cleanup_passed=false 原样保留：失败fallback没有正常计划Stop的完整顺序，零完成手部lease不通过新增采样审计。AK75文件封存 `8ada9ae7…93e40`，root-result `a8a60e82…fde82`，私有记录在忽略的 `analysis/hand-mesh-runtime-1024ak/`。

后续检查原输入过期后的有序恢复／新票据重接与拒绝原因保留，不能放宽100ms、把新票据塞入旧Plan或重启原15s预算。本局没有验证末端平移的原生消费或解决伸手位置；未安装默认启动器，全部12武器、真实0.7／1.0伤害与头显验收仍未完成。

## 2026-10-24 — AL 限定后置截止交还，修后运行待做

仅在现有 TAIL 实验开关内加入原授权观察与交还分支。捕获原调用一次的 Auth，TLS 立即还原；只分类实际 `known67/checked3/rejected2`、零返回且后续时钟尚未执行的早期 cutoff 过期，同原 Plan／context／证书时间和最早三戳+100 截止精确匹配，拒绝未来／倒退时间。它只允许有序交还，不代表未执行的 policy 条件通过。实际 snapshot／guards／component_owned 均先通过、joint phase1、原 owner／mount／epoch／ready／绑定／Stop 等仍健康，本次手骨和扩展写入／恢复／lease计数均未变，才请求既有 age suspend；只有 pause_pending 成立才 suspending 返回，不标 committed。原 PostTick 零depth 后才按原顺序恢复，失败仍 sticky；新 Tick 仍须新 Plan、稳定门及原身份／资产检查。

Astra 源审发现并修正两点：原 restore 保留历史 applied_mask，因此只比较前后相同，不要求零；后置 context 不得早于 Plan context。独立首拒绝包保存在八行计时容量之外，记录来源、原字段和是否请求暂停，仅经原清理后的 dump 输出；不把它误称最后一次终止原因。没有 reader 优化、增加授权调用／getters／时钟、票据替换、记录期重启或默认行为变更。源审 `3a2d55b7…a31b` 通过。此前15s／256在persistent模式中是记录封顶；实际runner仍保留原15s外部预算与3s清理预留，不扩大任何一项。

一次新受影响 fixture 严格编译运行通过12类／315项，22个实际源码片段及6份完整输入逐字绑定，包含真实 Policy首绑3、真实原授权短路、历史applied3、context倒退、其他Auth拒绝、手部计数／owner撤权、动作／Stop、原 suspend／PostTick／finish 的零depth与失败sticky。时钟／lease方法／受控drain／计数finalize仍是声明过的主机服务，未覆盖原生writer恢复、完整finalize或整个XR回调。CPU结果 `f7c2c414…a0f8c` 位于忽略的 `analysis/hand-tail-post-cutoff-cpu-20261024al/`；原198／370／539通过项未重跑。

冻结786源／27依赖，唯一新OFF两次/EP的五份实际运行源非空行相等；完整PG严格构建与23导出均通过，DLL `884d9f0b…9356a`。root构建记录器在最后源树复核处误用路径下标而报错，四条实际构建／OFF／导出命令均已返回0；首次错误保留，仅修正记录环节并核对冻结与实际源字节，不重复构建／预处理／CPU。最终结果 `d35b1905…85c95` 在忽略的 `analysis/hand-tail-post-cutoff-build-20261024al/result-r2.json`。修后候选尚未运行游戏，不声称解决100ms耗时来源、手掌位置或实际平移消费；无Native／XR／loader重建或默认安装。

并行有界伤害证据盘点仅使用33份已存直接输入，定位瞬发原 getter 经原修改／整数转换到 TakeDamage，以及 Gnasher按命中对象汇总和投射物返回前Init／爆炸多目标的关联边界；实际 ReturnParm／Result ABI与逐发身份未证实，未写0.7 hook。报告 `846c15a8…68e29` 位于忽略的 `analysis/weapon-grip-damage-evidence-20261024ak/`，不是实际伤害验证。

AL实际运行补记：源码提交 `b8bd6abb…eb98` 后，唯一修后候选冻结786源／27依赖／523工件，PG884d9f0b…9356a，原Native／XR／loader保持。runner全部函数／类AST与已封存AK相同，仅隔离目录／tag及新诊断前缀变化；原审计器与已通过46项检查原样沿用，未重复执行。新freezer生成首轮曾因误改继承的AK证据名称断言停止，未生成候选／prepare／启动游戏；保留错误，仅完成尚未生成的后半段。冻结、prepare核对和三次新鲜空闲后，静音／behind／observe-only的Steam购买版运行一次，物理后台手柄输入关闭。

这局在更早的 `component_acquire_failed` 终态8停止：组件1次acquire尝试、0次提交、5次setter尝试、恢复计数1，faulted与cleanup_failed保持1，最终dirty／owned／revoked均0；不能把恢复计数称作成功提交。hand leases／setter／target和扩展字段写入均0、姿态命令0，没有进入after-component授权，新early cutoff交还分支与独立首包都未被实际触发。最后保留的prewrite Auth为3／无拒绝，实际失败事件被后续恢复日志覆盖；中间pure_acquire_value差异记录也不能用作具体失败原因。目前不将失败归因于某个时钟或reader步骤。

独立8控件（含完整新增typed观察）、3组component、44字节挂接前后相同，入口无恢复错误；failure Stop0／Shutdown0，音频8会话恢复且pending0，确切owner／game／XR均不存在。正式passed／cleanup_passed仍false；独立Native／XR子会话通过且XR exit0不替代该结论。AL79文件封存 `ad31db33…a6c3ee`，root-result `f04b25a7…3986d`，回执位于忽略的 `analysis/hand-mesh-runtime-1024al/`。下一步先保留组件内部实际授权拒绝及完整成本，保留门限，不直接扩展暂停分类；无默认安装、物理位置、后坐力或完整12武器验收。

## 2026-10-24 — AM 首次实际授权拒绝与完整成本保留

仅在默认关闭的TAIL候选内修正诊断：`Backend::read` 实际仍返回其 `ok`，额外 `pure_acquire_value` 比较的是generic journal尚未发布的新owned tuple，可能在自身setter读回中间态出现差异。原诊断将该比较直接当成首个失败并冻结成本，导致后续真正拒绝被恢复环覆盖。新版本仍把比较原样放入63行环，但首个实际边界失败和成本冻结只采用真实gate／调用／授权失败；实际返回值、原授权和字段写入逻辑不变。另加独立capacity1的首个Auth拒绝完整副本，保存原ordinal、setter上下文、journal mask与全部原时间／结果字段；环覆盖和晚到拒绝不替换它，原清理后dump新增明确capacity／known标记，没有额外采钟或授权调用。

两份完整实际cost／trace namespace与真实组件值类型直接进入有限fixture。新4类／225项检查通过，覆盖中间比较仍可见但不提前冻结、实际AuthEnd与既有开span关闭后冻结、不同schema首故障／首Auth、400多条后续记录覆盖、计数饱和、disabled／begin replay／dumped／reset、原wide dump与新增零采钟。QPC、mask提供器及无关诊断reset／dump是明确host服务，不是游戏ownership或writer证据。首轮fixture严格编译因host变量ticks遮蔽原源码局部被/WX拒绝，未执行检查；保留错误，R2仅改host变量三个同名token并逆向逐字核对，实际片段未改。结果 `c879838c…fc441` 在忽略的 `analysis/hand-auth-pin-cpu-20261024am-r2/`，旧315／198／370／539项未重跑。

冻结786源／27依赖，唯一完整PG严格构建、23导出及新OFF五实际运行源/EP非空行比较通过，DLL `f0433810…90c1`，build结果 `47e66fca…e75e` 在忽略的 `analysis/hand-auth-pin-build-20261024am/`。本补记时尚未运行AM购买版局，没有证明AL拒绝的具体原因或解决手部位置；不放宽100ms、不更新旧Plan、不扩大暂停分类，不重建Native／XR／loader或安装默认入口。

AM实际运行补记：提交 `d3285c90…b3f8` 的786源／27依赖／535工件私有候选 `02a2598f…44d2`，在三次新鲜空闲确认后仅启动一次静音／behind／observe-only的Steam购买版测试，物理后台手柄输入关闭，原15s外部预算、3s清理预留、8命令及0.1单位／0.1°保留。首个枪体提交后，原后置授权实际记录 known67／checked3／rejected2，seq752／rev24，now=live68706015、write_until68706006，返回0；新限定分支请求age交还并完成clean_close1。其后实际双手hand_only应用7次／恢复7次，setter／target均56；末端字段46行／23对／7个完整lease审计通过，prepare写28次、最后总写56次，8份外部typed观察均通过。

第二次枪体取得控制仍以 `component_acquire_failed` 终态8停止：intervals2、acquire尝试2、update尝试0、commit1，累计component acquired1／restored2／setter尝试9，faulted与cleanup_failed均1，最终dirty／owned／revoked0。首次Auth pin为ordinal198／seq752／rev24，是前一个已经恢复的拒绝；终止发生于seq821／rev32，最后postflush授权通过，不可借首pin代替后一次具体原因。原8条移动命令均未开始。直接调用未改的原hand_samples审计器，一次离线读取原日志得7组同Tick之后样本、14个手部结果，全部在原门限内，最大位置0.0000247956单位、旋转0.0299503°；这些只有初始未变化手势，原Limb和末端位置同时工作，不能隔离新平移消费或证明此前大幅伸手问题、蒙皮／画面与完整移动通过。

原正式passed／cleanup_passed均false。独立完整8控件、3组件和44字节挂接前后相等，入口恢复无错误；failure Stop0／Shutdown0，8音频会话恢复且pending0，确切owner／game／XR均退出。Native／XR子会话通过不替代整体结论。77文件封存 `95598b2b…13c`、root结果 `1629c18a…f216`，在忽略的 `analysis/hand-mesh-runtime-1024am/`，后续没有改写该目录。hand样本补证在独立 `analysis/hand-auth-pin-runtime-root-20261024am/hand-sample-result.json`。

原initial两个实际时段4.6996／4.9053ms；旧group0的current／read聚合是嵌套跨attempt数据，不能相加或当作末端reader独占成本。Astra有界只读源审 `cf7d3f28…e2cb` 确认实际ContinuousAuthorize位于原invoke_paused保护内，因此旧计时未覆盖原prepare和最终hand_direct_matches。建议保留每次真实acquire／update的失败上下文，在这两个原表达式单独计时，均在原授权时钟之前结束；不解除全局invoke暂停、不重授权、不在成功授权至setter之间新增计时。源审未写代码或运行测试。单双手0.7／1.0仍只有规格与静态伤害路径证据，无默认安装、全武器或头显验收。

## 2026-10-24 — AN 本次枪体失败记录与原回调定点计时

仅在TAIL默认关闭分支内增加一个固定失败操作副本及每操作最多16条callback记录：实际acquire／update前清空并绑定原sequence／revision／lease／interval，false返回后冻结该次首个真实失败和callback；首次全session Auth pin仍单独保留。callback的Auth只在本次原authorization表达式实际执行时复制，前置失败明确unknown，授权PASS后的dispatch失败独立保留，恢复环与后续调用不能替换失败副本。原prepare与hand_direct_matches各增加一个显式QPC对，含原VM的prepare不标为纯reader；两段计时均在原最终授权时钟前关闭。Scope结束、原SEH finally和dump不采钟，异常保留未完成字段并恢复index；不解除invoke_paused、增添getter／授权、缓存类型、改变写入／恢复／Plan或放宽门限。OFF callback保留原完整定义。

新6类／55项主机检查首次严格编译运行通过，直接使用完整实际cost／trace namespace和实际ForwardDiagnostics／component_callback_permit。覆盖先前可恢复拒绝与后次失败隔离、两段恰好4次QPC、旧pause保持、前置拒绝不借旧Auth、授权通过后dispatch失败、真实Windows RaiseException穿过实际SEH finally后的unknown与下次callback、容量／嵌套／reset及真实wide dump。时钟／mask／无关诊断服务和只供finally使用的Permit／两成员ContinuousAuthorize是显式host服务，不是实际引擎callback执行。CPU结果 `49ac51e2…1870` 位于忽略的 `analysis/hand-component-attempt-cpu-20261024an/`，旧225／315／198／370／539项未重跑。

冻结786源／27依赖，唯一新完整PG严格构建／23导出及OFF五实际运行源两次/EP非空行相等均通过，DLL `4d50dff8…8604`，build结果 `ce0be602…3523` 位于忽略的 `analysis/hand-component-attempt-build-20261024an/`。本条记录时尚未实际运行该候选；未知终止原因与100ms成本仍待新局验证，没有reader优化、Native／XR／loader重建或默认安装。

AN实测补记：源码 `e5fbcb1b…384a` 的786源／27依赖／547工件私有候选 `aea8eff8…75a1`，按三次新鲜空闲后只进行一次原8命令／15s预算／3s清理预留的静音置后购买版测试。准备脚本生成器首轮仅因一段说明文字的大小写断言停止，未写候选／prepare或启动游戏；保留错误，修正生成器后执行首次实际候选构建与prepare。runner全部函数／类AST与AM相同，原审计器及此前46项检查原样沿用，只有目录／tag与保留诊断前缀不同。Astra源码窄审 `cdaf6895…cef8e` 对两个源码pin通过，实测前root复核与实际源／冻结786份逐一相同。

新增失败副本这次实际捕获了停止原因：首个acquire的seq755／rev23／lease1／interval1，第0／1／2组三次原最终授权返回3，最后group4旋转的原授权在ordinal192返回0，known67／checked3／rejected2，now=live69612484大于write_until69612475（原ticket69612375＋100ms）；没有进入后续NS／Policy阶段。四次prepare实际分别7.7695／8.2188／7.6974／7.8021ms，最终hand_direct分别2.4990／2.5722／2.5055／2.6687ms，原initial为4.9035ms。prepare含原VM，不能将其全算末端reader；这些阶段不与旧嵌套聚合相加。此次明确是实际部分acquire最后旋转前授权截止，并不回填AL／AM当时未保存的具体原因。

该局组件1尝试／0提交／1恢复、setter尝试5，fault与cleanup_failed保留1，最终dirty／owned／revoked0；双手／新增字段零写、0移动命令，正式passed／cleanup与tail审计均false。独立8控件含typed观察、3组件和44字节挂接前后相同，入口恢复无错误，failure Stop0／Shutdown0；8音频会话恢复且pending0，确切owner／game／XR均不存在。独立Native／XR子会话通过不替代正式失败。97文件封存 `cd40f4a9…858fd`、root结果 `11d59a63…bc655` 在忽略的 `analysis/hand-mesh-runtime-1024an/`，不再改写。下一步只优化单次末端观察的重复first-match查找，保留所有实时identity／deleted／typed检查和原100ms，不增加暂停／重试授权分类，不清除真实fault或降低动作／精度。0.7伤害、全武器与头显仍未完成，无默认安装。

## 2026-10-24 — AO 单次末端观察的 first-match 地址批查

默认关闭TAIL内，新增只返回本次局部地址的 `playground::fields<N>`：原子类优先／链内首同名顺序、40层／每层4096项与大小写规则保持，不按kind跳过错误首匹配，缺项／null查询时输出全零。候选名一次解析后匹配尚未找到的标签；优化名称scope未启用时沿用128字读取，启用时取本组最长查询上限，实际page-aware NUL读取不变。原11个control descriptor与3个Vector成员逐项交给原 typed validator（只移出地址定位），36次live identity／deleted及布局、掩码、重叠、Enum、raw值和末尾owner／class／extent均保留。原始值观察后再独立遍历两组字段，必须与前次地址一致；不跨observe／invoke／Tick缓存成功、缺失或权限，仍非原子观察。Astra计划源审 `359c39fa…7ca6` 和实际三文件窄审 `0f62b951…5e54` 明确绑定各自基线／活动源，未将后改稿冒充早先已审。

真实原／新resolver与named／page text的5段逐字源码进入8类／53项新主机检查，首轮严格编译运行通过，结果 `27705d8e…f76d` 在忽略的 `analysis/hand-tail-batch-cpu-20261024ao/`。覆盖子类／链内遮蔽、重复查询、错误首类型不能跳过、缺失／null／不可读／无NUL名字、scope=false、真实PAGE_NOACCESS边界、原40／4096边界与循环、不同调用的新链／名称及无部分发布。有限示例的名称解析由逐项9次减少为批查4次，只说明工作量减少，未证明毫秒节省。UObject到名称的catalogue和本地节点是明确host服务，不能证明真实FName注册ABI。

冻结786源／27依赖，唯一完整PG严格构建／23导出通过；新OFF一对/EP将controller也按真实include边加入，六份实际运行源的非空行相等。DLL `a96aedd0…cdc2`，build结果 `4f2a278a…7746` 在忽略的 `analysis/hand-tail-batch-build-20261024ao/`。实际reader／最终SEH writer目标因受影响而重做一次回归，新16类／207项通过，新增原raw读取期间control／Vector首匹配改变或消失后的拒绝；fields仍是明确catalogue seam，实际批查由独立53项覆盖。未重复未改的225／315／370／539项。此时尚未游戏验证，不能承诺100ms内完成、自由伸手或实际画面通过；原AN失败封存不改，无Native／XR／loader重建、默认安装或0.7真实伤害接线。

AO实测补记：提交 `64bbe76c…9003` 的786源／27依赖／559工件候选 `85c79d60…dfec`，按原三次新鲜空闲、静音／behind／observe-only、物理后台手柄输入关闭及原15s／8命令／0.1门限运行一次购买版测试。首枪体提交后按原67/3/2截止拒绝有序age交还，clean_close1；其后hand_only7应用／7恢复，setter／target56。末端46行／23对／7个完整lease审计通过，prepare写28、总写56，外部8份typed观察有效；14个初始未变化手势结果均在原门限内，最大位置0.0000171661单位、旋转0.0157499°。仍无移动命令，不证明大幅伸手、末端平移独占贡献或画面／蒙皮完成。

第二次枪体acquire在seq817／rev31／lease8／interval2的group2平移前停止；实际失败副本ordinal674记录known127／checked2047／rejected1024。前两组原授权返回3，此次MS now=live=commit70094765≤write_until70094771，原后置check10 `!certificate_recent(...,commit_ns)`实际拒绝；commit_ns70094930496400−stereo_ns70094820316700=110.1797ms，input证书也超过100ms。故本局是明确的最终NS新鲜度拒绝；不能仅凭MS未越界判作原授权错误，也不回填旧局。三次prepare实测7.5114／7.5241／7.4684ms、最终hand_direct2.4691／2.3467／2.4381ms；这是不同运行中的小样本，没有受控比较或字段／删除检查独占计时，不能声称批查已解决性能。

累计组件2次acquire尝试／1提交／2恢复／setter尝试9，fault与cleanup_failed保持1，最终dirty／owned／revoked0；8移动命令未开始，正式passed／cleanup仍false。独立8控件／3组件／44挂接恢复，入口无恢复错误，failure Stop0／Shutdown0，音频8会话恢复／pending0，确切自有owner／game／XR均不存在。独立Native／XR通过不替代正式失败。59文件封存 `952075aa…2f08a`、root结果 `f9f40e2e…0c1fc` 在忽略的 `analysis/hand-mesh-runtime-1024ao/`，额外初始手样本一次离线审计保存在独立root目录。下一步由Astra就同次观察中重复live删除／身份反射的具体等价优化作有界审查；不加TTL、不续旧Plan、不清fault，不扩大暂停分类。无默认安装、0.7伤害接线或头显通过。

Astra后续只读建议 `48286dbb…8512f` 已给出一个尚未实现的候选：仅在单次observe栈内，对重复同类 `bDeleteMe` 查找保存有界完整路径比较材料，每次仍实读class／children／super、field next／FName、当前GNames entry与必要原始名称字节，重新证明当前不存在；正匹配保留原live descriptor／mask／对象bit路径。现有field()==0混合未命中与失败，不能直接缓存为absence；未知／路径变化必须拒绝本次观察，不能重建再准入。原Snapshot缺少首匹配完整路径，不能跨VM借旧地址跳过查找。该报告未改源码、运行测试／游戏或承诺收益，位于忽略的 `analysis/hand-tail-next-cost-review-20261024ao/review.json`。root另核对AM／AN／AO共77／97／59份封存文件未变、当前786源与最新严格构建相同；不把候选方案算已完成修复。

## 2026-10-24 — AP 同次观察的实时删除查找路径校验

TAIL默认关闭路径新增一个observe栈内Lookup，显式传入原36次identity，原注册／header／typed／schema与末尾重新查找保持。它仅对完整已知且再次校验的 `bDeleteMe` 未命中记录比较材料；每次复用实读全部root身份、class children／super、field next／FName、GNames Array／entry指针／index与实际名称字节。第10个原始wchar保留，避免将长名末尾人工清零误作真实NUL；未知或任一字节变化锁存failed，本次不能fallback重建。正匹配以及未发布凭据前的资源／层级上限走原deleted实时descriptor／mask／对象bit路径；不跨observe／VM／Tick保存状态、删除结果或权限，不改变原100ms／原动作／精度。Astra三源窄审 `98aec17a…75f95` 通过，并明确Win64栈空间约131,344字节、完整Array和已复制NUL后字节变化会额外保守拒绝，不称完全等价或已提速。

9段完整实际name／registered／field／property／deleted／Lookup等源码在独立本地GObjects／GNames数组和对齐节点上执行，新8类／405项检查首轮严格编译运行通过，结果 `d4b4de2d…26566a` 在忽略的 `analysis/hand-tail-delete-cpu-20261024ap/`。覆盖完整实时负路径、当前正匹配offset／mask／对象bit和早期遮蔽、层级／root／FName／entry／文字改变后完整恢复仍拒绝、首读与warm未知、真实第10wchar变NUL、实际PAGE_NOACCESS后缀、容量／循环／disabled原路径，以及当前registry／class与独立新observe寿命。实际算法未替换为catalogue，但本地数据布局及计数／拒读copy是明确host服务，真实SEH／VirtualAlloc页面不等于真实游戏注册表或性能证据。有限同一fixture仅核对实时复用读数少于原逐项查找，没有给出毫秒承诺。

冻结786源／27依赖，唯一完整PG严格构建／23导出、六实际运行源的OFF一组两次/EP非空行相等，以及受影响actual reader／最终SEH目标16类／207项回归均通过。DLL `cfd25b0c…8bf5c`、build结果 `09f0b3aa…36bc` 在忽略的 `analysis/hand-tail-delete-build-20261024ap/`；reader目标的删除／地址查找仍是显式catalogue服务，不能代替405项新路径检查。未重做未变的53／55／225／315／370／539项。此记录时尚未游戏验证，实际节省量与完整8命令／大幅伸手仍未证明；旧AO失败封存不改，无Native／XR／loader重建、默认安装或真实0.7伤害接线。

AP实测补记：提交 `bb542b15…48bed` 的786源／27依赖／571工件候选 `372235cd…68cf8`，按原三次新鲜空闲、静音置后／observe-only、物理后台输入关闭及原15s／8命令／0.1门限运行一次购买版测试。首次acquire的四组prepare和hand_direct均返回通过；原授权第0／1／2组返回3，最后group4旋转的ordinal192实际记录known127／checked2047／rejected1024，最终check10 NS新鲜度拒绝。MS now=live=commit71090640≤write_until71090646，票龄94ms；实际commit的stereo／input源龄分别104.0706／104.0464ms。四次prepare7.5043／7.3323／7.4895／7.8116ms，最终hand_direct2.5122／2.4755／2.4778／2.4913ms，没有逐Lookup启用／命中计数，且不是与AO受控比较，不能认为该候选已提速或定位了独占瓶颈。

组件1次acquire尝试／0提交／1恢复／setter尝试5，fault和cleanup_failed保留，最终dirty／owned／revoked0；双手与新增字段零写、0移动命令，正式passed／cleanup／tail审计仍false。独立完整8控件typed观察、3组件和44字节挂接恢复，入口无恢复错误，failure Stop0／Shutdown0，8音频会话恢复且pending0，确切自有owner／game／XR均不存在；独立Native／XR子会话通过不替代正式失败。73文件封存 `e0fae948…177ad`、root结果 `359506f7…def1e` 在忽略的 `analysis/hand-mesh-runtime-1024ap/`，不再改写。准备与最终校验通过不能证明每条新Lookup路径都实际启用，当前证据只确认整段未达到原目标。

下一候选收敛补记：Astra七输入有界源审 `0f9d42ae…2c5b1` 建议只批量定位每次read_control的17个SingleBone或19个Limb字段，再按原顺序进行每项注册／类型／offset／size／值／bool mask／结构类型检查，末尾独立重查地址与当前class后才完成原TailRead。静态成功八控件约200–208次字段名称遍历可改为16次批查，但没有毫秒收益证明；暂不合并pawn cache八项。报告在忽略的 `analysis/hand-control-batch-next-review-20261024ap/review.json`，未写源码／测试或运行游戏。

为控制候选复杂度，root已撤下AP新增约131KB删除查找工作区及其reader／fixture接线，保留AO地址批查、原live deleted和AN失败／计时诊断。撤回依据是收益尚未确立且存在额外保守拒绝与栈成本，不是宣称已证明零收益。三份生产源按已冻结AO原字节恢复，786源逐一与AO已通过的严格构建输入相等；没有重做旧构建／检查，没有修改AP候选、73份失败封存或任何公开包。恢复证明 `c50cde8a…5e83f` 在忽略的 `analysis/hand-control-batch-prepare-20261024aq/restore-source-result.json`。原控制器17／19字段的新接线尚未编写／验证；完整6DoF、真实0.7伤害、默认安装和头显仍未完成。

## 2026-10-24 — AR 同次原控制器读取的 17／19 项地址批查

TAIL 默认关闭路径内新增小型 `ControlFields<N>`，仅保存一次非调用读取的初始 class 与首匹配地址。八个 pawn cache 仍原样读取，原 instance／owner／identity 先执行；SingleBone 17 项、Limb 19 项各批查一次后，原顺序逐项 registered、实时 kind／size／offset／class extent、copy、finite／range／Name／space、bool bits 后实时 mask、struct inner 注册／类型及 ObjectProperty→ComponentProperty 回退均保持。尾部再次完整解析并比对地址与当前 class，成功后才进入原 tail_layout_allowed 和 TailRead.complete；失败仍触发原撤权析构。无跨 control／调用／VM／Tick 状态、getter／写入／时钟／授权变化。缺项可能更早拒绝、名称读取顺序变化、地址相等不是全量描述符快照／原子性证明等限制明确保留。

Astra 对原／新实际源的窄审 `d62d118c…b6fb` 未发现具体阻断。18 段实际原／新 read_control、registered／name／字段解析及逐项校验源码在本地 GObjects／GNames 布局上执行，新 8 类／659 项检查首次严格编译运行通过，结果 `82ce1597…7e2` 位于忽略的 `analysis/hand-control-batch-cpu-20261024ar/`。覆盖全部八控制器输出／原拒绝字段、继承／错误首匹配／ComponentProperty 回退、缺项、真实 PAGE_NOACCESS 名称边界、copy 未知、类型／大小／offset／mask／inner／值／身份错误、初始批查后实时描述符改变、末尾地址／class 变化先于 tail 布局拒绝，以及新调用不借旧成功。原实际尾观察／lease 策略在本 fixture 是显式有限服务；copy 只增加拒读／计数／单次变更注入并经过真实 SEH，不能称原生引擎执行。有限 Limb 例字段名解析从 295 次降为 60 次，仅为工作量计数，非耗时结果。

786 源／27 依赖冻结，唯一完整 PG 严格构建、23 导出及六实际运行源 OFF 一组两次 /EP 非空行相等均通过。DLL `589cc44b…14c5d`，构建结果 `482253f5…6626` 位于忽略的 `analysis/hand-control-batch-build-20261024ar/`；未变的 reader／最终 SEH 16／207、resolver 8／53 及其他旧检查直接沿用，未重复执行。此记录时尚未运行实际游戏；保留原 8 命令、15s 外部预算／3s 清理、100ms 和 0.1 单位／0.1°，收益与完整伸手仍待验证。无 Native／XR／loader 重建或默认安装，真实 0.7 伤害、其余全武器与头显仍未完成。

AR 实测补记：提交 `2407537e…819c1` 的786源／27依赖／571工件候选 `98344503…ec749`，runner 全部函数／类 AST 与 AP 相同，原 audit46 项复用，三次新鲜空闲后唯一静音置后购买版运行。原 component acquire 完成一次提交，随后原 post-component snapshot／guards／owned 均通过；实际再次授权在 ordinal198、seq755／rev24／lease1 的最终 check10 拒绝，known127／checked2047／rejected1024。NS stereo／input 源龄107.0825／107.0575ms，MS票龄94ms仍距原截止6ms。checked2047只证明0–10项已执行；check11未执行，policy_mask0的具体内部原因未证明，不能称全Policy通过或自动扩大可恢复分类。

原 post-component 分段实测 snapshot3.5781ms、guards4.3191ms、component_owned1.9017ms，后者内含 owned_live_read1.4641ms；authorize0.0021ms，嵌套不重复相加。acquire实际返回成功，因此新 failed-attempt 记录保持unknown0且没有四组callback失败副本是预期；本局没有成功操作定点段的副本，不能补造prepare／hand_direct耗时或从跨局数据宣称优化收益。现有仅MS早期过期的交还分支未命中，正式reason为hand_reauthorize_after_component。

组件1提交／1恢复、setter6，fault／dirty／owned／revoked／cleanup_failed均0；双手零写、0移动命令，原正式passed／cleanup／tail审计仍false。独立8控件typed观察／3组件／44挂接前后相同，入口恢复无错，failure Stop0／Shutdown0，音频8会话恢复／pending0，确切自有owner／game／XR均不存在；独立Native／XR通过不替代正式失败。59文件封存 `6c75cd25…33d96`、root结果 `22977e10…fae57` 位于忽略的 `analysis/hand-mesh-runtime-1024ar/`，额外解释留在独立root目录，不改原始证据。已把具体post-component NS过期与未完成Policy检查问题交给Astra有界审查，尚无新恢复接线；无默认安装、真实0.7伤害／全武器／头显完成。

## 2026-10-24 — AS 已证实 post-component NS 源超龄的有序交还

AR 的明确 check10 拒绝之后，Astra 十输入有界建议 `1ff48d71…6feaf` 与实际实现八输入窄审 `eea19057…9a0bb` 支持一个仅供交还的分支。新增独立 `post_component_ns_source_expired`：只接收实际 known127／checked2047／rejected1024、mask0／result0，逐项比对原Plan／context／截止与源时间，检查证书形状、正数／无未来／无倒退、原三项MS新鲜度和精确截止，且至少一个源在原authorize_ns时已经严格超过100ms。它不接收只在commit间隙超龄或非零Policy mask的其他情况。NS条件与原完整pause_ready联用；当前Stop／Shutdown、真实线程、epoch、绑定、健康owner／journal与手／extra计数未变仍必须通过，只能suspend_interval(true)，pause_pending成立后返回原PostTick零depth恢复。原早期MS分类、实际授权／手写／committed／录制起点不改变，不能称原Policy内部已通过，也不清fault或让旧Plan重试。

固定诊断新增ns_source_expired，保留早期MS字段和policy_checks_complete=0／grants_write_authority=0。原XR混合换行被统一CRLF，旧函数只在归一化换行后完全相同，不声称raw源码字节相同。实际两authorizer／AuthEnd、新旧classifier／pause_ready、完整原失败分支及PostTick／finish等23段源码进入16类／593项受影响检查，首次严格编译运行通过。覆盖原MS／健康／lifecycle12类和新增NS源单侧或双侧超龄、100ms／+1ns、未来／倒退／形状／packet／Plan错配、current Stop／部分手写、实际分支延后恢复、无新时钟／授权调用、失败sticky及旧Plan仍拒绝。Policy／Plan／Certificate／Session实际头与代码执行；lease／mount／trace sink、失败诊断收集和drain／finalize是明确host服务，不冒称引擎恢复或全Policy原因已知。准备报告在首次编译前仅修正host中两个不同array类型的auto声明，原报告保留，最终root-review与runner绑定新fixture，没有编译失败重试。结果 `7847fd8b…eac85` 位于忽略的 `analysis/hand-post-ns-cpu-20261024as/`。

冻结786源／27依赖，唯一完整PG严格构建／23导出与六实际运行源OFF一组两次/EP非空行相等通过；DLL `86b5f64b…45f26`、构建结果 `a3b5f87f…733a` 在忽略的 `analysis/hand-post-ns-build-20261024as/`。未变的原tail reader／SEH、control8／659与其余旧检查不重复。本条记录时尚未实际游戏验证，交还成功不保证下一输入能完成联合手写，也不解决读取成本或大伸手几何；原8命令／15s预算／100ms／0.1门限仍保持。无默认安装、Native／XR／loader重建、实际0.7伤害或完整全武器／头显通过。

AS实测补记：提交 `c7ad8ac1…e1b8f` 的786源／27依赖／583工件候选 `c5a6a11b…73221`，三次新鲜空闲后唯一原协议静音置后购买版运行，runner函数／类AST与AR相同。实际seq755／rev25首次post-component的原最终check10仍拒绝，stereo源龄109.2292ms；新增ns_source_expired=1／pause_requested=1，早期MS标记0，原Policy／写权限诊断仍0。组件1提交／1恢复、clean_close1、fault0，随后新输入完成5个hand-only lease／5恢复、40原setter／target写；32末端行／16对样本、5完整末端lease、20 prepare字段写／最后观察36，tail审计通过。10份未移动初始手势位置／旋转结果均在原0.1单位／0.1°内，最大位置0.00006115425原始单位、旋转0.01231177°，不证明大伸手／蒙皮／最终画面或末端独立贡献。

该局正式失败由测试host的Path.read_bytes(session-status.txt)抛PermissionError errno13触发，原freshness fail-closed策略停止并清理；未记录底层Win32错误，不能声称已证实sharing lock。Stop后游戏status6／xr_stopped_after_restore，component／hand无sticky故障，0移动命令，正式passed／cleanup仍false。独立8控件／3组件／44挂接恢复、Stop0／Shutdown0、音频8会话恢复pending0及确切自有owner／game／XR消失核对通过，Native／XR子会话独立通过。70文件封存 `09bb303a…f2460`、root结果 `15da86dc…696bb` 在忽略的 `analysis/hand-mesh-runtime-1024as/`，不改写。

下一AT只修测试读取路径：复用现已固定哈希的run_stereo_continuous.open_session_status（Windows share read/write/delete，同一个完整发布代次），通过buffer返回原始bytes以保留严格UTF8检查，原worker／进程身份／report／audio与freshness／预算／异常拒绝全部保留，不增加重试或缓存。实际提取read_status与真实Windows文件句柄／原freshness的8项检查通过，结果 `bf2235f3…21bbf` 在忽略的 `analysis/hand-status-shared-read-20261024at/`。首轮fixture误用MoveFileEx替换被持有文件，2项WinError5失败；原fixture／错误保留，R2仅修publisher为实际AtomicFile的SetFileInformationByHandle(FileRenameInfoEx,REPLACE_IF_EXISTS|POSIX_SEMANTICS)，8项首次R2通过。未改候选binary／Native／XR／源786或旧freshness政策，没有以测试变更冒称原PermissionError根因已证实；尚未实际运行AT。

## 2026-10-24 — AT 原共享状态读取器接入后的实际再测试

仅更换host的read_status文件打开表达式，使用原固定哈希run_stereo_continuous.open_session_status并从buffer取原bytes，反向还原该表达式后runner函数／类AST与AS完全相同；未改原freshness／重试／命令／预算／身份／清理或任何候选binary。源786／依赖27／Native／XR／PG直接复用AS已冻结构建，没有重复构建或重跑593／659等旧检查。786源／27依赖／599工件候选 `0dd4cbed…6a9af`，manifest `a80dd31b…1aa0`，DLL仍 `86b5f64b…45f26`，三次新鲜空闲后唯一静音置后购买版运行。新reader8项实际文件与原strict freshness检查的fixture修正事实保持前条记录，不当作首次全绿。

本局未出现PermissionError，但不能据此证明AS底层I/O原因或普遍修复。原post-component NS交还再次实际命中并clean_close1，之后7次hand-only应用／7恢复，原setter／target56；46末端行／23对样本、7完整末端lease、28 prepare字段写／最后观察56，tail审计通过。14份未移动初始手势原0.1门限内，最大位置0.00006394598原始单位、旋转0.02719499°；依然0命令，无大伸手／渲染／头显或末端独立贡献证明。

第二次acquire的seq822／rev33／lease8／interval2在group2平移之前失败，实际ordinal674 known127／checked2047／rejected1024、mask0。MS票龄93ms距原cutoff7ms，commit的stereo／input源龄104.1106／104.0945ms。此前group0／1授权返回3；该组prepare及hand_direct也通过，但实际最终check10拒绝，check11／12与Policy内部首因不作补证。三次prepare7.1589／6.9913／6.9017ms、hand_direct2.2561／2.1890／2.3512ms，包含原VM且不属于受控提速对比。部分component失败仍按原合同latch fault，没有事后清除；新增post-component交还并未覆盖这条callback内失败。

累计组件2次尝试／1提交／2恢复、setter9，component fault／cleanup_failed1、最终dirty／owned／revoked0；正式passed／cleanup仍false，Stop0／Shutdown0。独立8控件typed观察／3组件／44挂接与入口恢复、音频8会话恢复pending0和确切自有进程退出通过；Native／XR子会话独立通过。77文件封存 `976d820e…6ba3`、root结果 `30d1c96a…ac948` 在忽略的 `analysis/hand-mesh-runtime-1024at/`，不改写。已将这次实际callback前NS拒绝与原generic fault合同交给Astra有界审查，要求不能事后清fault／放宽写权限；若无安全取消前提则只选具体读成本优化，尚无新接线。完整6DoF／0.7真实伤害／全武器／头显仍未完成，无默认安装。

## 2026-10-24 — AU 连续八控制器的 pawn 缓存地址批查

Astra对AT具体callback NS失败和generic Permit／lease合同的12输入审查 `a116871c…bdda` 明确：Permit拒绝之后原update_impl仍读回／settle，再因valid_once=false调用reject锁stopped＋faulted；callback内cancel／suspend或busy时递归yield均不能安全绕开，假报success也无效。因此不把已锁存失败改成可恢复暂停，下一步只减重复读取。报告位于忽略的 `analysis/hand-component-ns-next-review-20261024at/`。

TAIL内新增PawnControlFields，复用已有ControlFields<8>仅保存八个pawn cache的首匹配地址。snapshot在两个MatchRefBone调用之后建立局部块，八次read_control结束即重新解析并销毁，早于read_lists和GetBoneMatrix；hand_direct_matches有独立局部块。每项仍实时注册／class／kind／size／offset／存储范围及指针读取，原control身份／布局／值／比较顺序不删；独立Backend等调用不传该局部指针，继续原查找。batch begin／finish失败发生在原TailRead之外，显式对3／7调用既有tail_observation_failed，实际anchor／pending过滤仍执行；正常单项失败仍只原index撤权，正常比较失败不发明新的TailRead失败。无跨VM／callback／Tick缓存或Plan／Permit／setter／100ms／恢复合同更动。Astra实际8输入窄审 `bfca93c2…8079` 通过；IK保持CRLF，phase1由LF统一为CRLF，不称后者raw字节未变。

22段实际读取／名称／注册／两调用块／tail_observation_failed源码与实际尾Value头进入7类／182项受影响检查，首次严格编译运行通过，结果 `de380939…bdd8` 位于忽略的 `analysis/hand-pawn-cache-cpu-20261024au/`。覆盖八控制器原新完整输出、初始缺项／未知／失注册导致匹配pending尾部撤权、所有cache实时类型／大小／offset／指针读失败、首同名错误类型、批查后描述符变更、末尾class／地址／注册变化、原单项与比较短路、当前anchor／pending过滤、恢复字节不能抹撤权及新调用不借旧成功。tail观察和layout／value、pending／anchor存储是显式host服务；实际撤权函数体执行并只作机械重命名以便统计，不冒称原引擎或完整ownership。fixture新函数有条件签名，首次机械brace提取未涵盖互斥头；在任何编译前保留初稿并改为连续完整#if/#else源码区间，未动生产。有限8字段fixture名称解析468→448只是工作量，不是耗时承诺。

冻结786源／27依赖，唯一完整PG严格构建／23导出与六实际运行源OFF一组两次/EP非空行相等通过。DLL `b3fbdd54…e3df5`、build结果 `28a2287f…d2b55` 在忽略的 `analysis/hand-pawn-cache-build-20261024au/`；未重跑未变的593／659／207等旧套件。本条记录时尚未购买版运行，完整8命令／大伸手仍未证明；原时间／精度保持、未清fault，无默认安装／Native／XR／loader重建、0.7真实伤害或全武器／头显通过。

AU实测补记：提交 `0f13a451…327f8` 的786源／27依赖／611工件候选 `36c043b2…1732e`，manifest `463d520d…0030b`，runner函数／类AST与AT完全相同，原shared-status reader保持。三次新鲜空闲后唯一原8命令／15s／3s清理／0.1门限静音置后购买版局，首post-component按原MS早期超时67／3／2交还，clean_close1；之后7次hand-only应用／7恢复、原setter／target56，46末端行／23对样本、7完整末端lease、28 prepare字段写／最后观察56，tail审计通过。14个未移动初始手势结果全部在门限内，最大位置0.00002670288原始单位、旋转0.01890266°；依然0命令，不证明大伸手、蒙皮、GPU／头显或末端独立贡献。

第二次acquire的seq820／rev33／lease8／interval2，group0／1原授权3，group2平移前ordinal674最终check10 NS拒绝，known127／checked2047／rejected1024／mask0。MS票龄94ms距原截止6ms，commit stereo／input源龄105.7948／105.7696ms，原后续Stop／Shutdown和Policy内部首因不补证。prepare实际7.1184／7.3903／6.6690ms、hand_direct2.1116／2.0113／1.9956ms，三个段均返回true；不是受控比较，没有独占字段扫描计时，不能认为AU已证明提速或足够解决100ms。原generic拒绝锁存保持，累计组件2尝试／1提交／2恢复、setter9、fault／cleanup_failed1，最终dirty／owned／revoked0，正式passed／cleanup仍false。

独立8控件typed观察／3组件／44挂接和入口恢复、Stop0／Shutdown0、音频8会话恢复pending0、自有owner／game／XR退出均核对通过，Native／XR子会话独立通过。65文件封存 `12159b1f…479b9`、root结果 `28318a7b…b3d5d` 在忽略的 `analysis/hand-mesh-runtime-1024au/`，保持原失败与前序封存。当前新增批查仍保留于私有候选，没有默认安装。下一步只审查固定骨名FName参数取得的成本：name_value当前每次完整扫描GNames，snapshot左右各调用一次，但现有计时未隔离该段，不能称已测最大瓶颈。已将“只缓存当前验证过的参数值、保持原MatchRefBone调用与11／35结果检查”的语义／首匹配／未知读问题交给Astra短审；尚未接线，无新RE／测试或100ms改动。完整功能、真实0.7伤害与头显仍未完成。

## 2026-10-24 — AV 实时动画列表的骨骼名称参数

Astra 对固定 FName 缓存的审查 `94b2864a…4f617` 给出反例：较早的同名项变更或恢复可读时，表头及缓存项均可不变，原全表扫描却会选另一个索引。未实施跨调用缓存。TAIL 内改为在每侧原 MatchRefBone 前，从当前 typed SkelControlLists[0／2].BoneName 取得参数；保留六列表／115索引、11→0／35→2映射、inner／字段类型、大小／stride／offset及当前 pawn→mesh→tree 和注册 cache head 相等检查。Name 使用同一份 copy-known GNames 做原严格 bounds、number0、index／entry header及完整含NUL精确文本校验；左右不借用描述符或结果，各保留一次原 typed VM 和11／35结果检查，后续完整八控制器／read_lists／GetBoneMatrix不动。这是明确的实时列表参数来源变更，不声称与全局首次同文匹配语义等价；原字符串探针仍用原全表扫描。

初稿严格表头后又调用较弱 name_is 重读表的缺口由 Astra 指出，并在任何编译前改成同表 known-copy校验，初稿与修正记录均保留。最初生成器因 freehands 实际为LF而不是假定CRLF，在生产写入前断言失败；R2保留 freehands LF、IK CRLF完成。最终窄审 `b9138a84…2779` 无剩余阻断，可精确局部逆变换恢复两原文件。24段实际 Name／typed Args／Request／list／两查询调用块在本地 GObjects／GNames和真实SEH copy上执行，9类／88项检查首次严格编译运行通过，结果 `a7e9afb0…c5ee` 位于忽略的 `analysis/hand-live-bone-cpu-20261024av/`。覆盖数组／布局／类型／map／head错误、未知和index0 header、大小写／Unicode／NUL、左VM期间变更后右侧重读、原11／35与失败不重试、全局早同名反例及后续完整链仍拒绝。native invoke为显式Name→返回值host服务，未宣称真实引擎ABI；同表读取不是原子快照。

冻结786源／27依赖，唯一完整PG严格构建、23导出及七实际运行源OFF一组两次/EP非空行相等通过；DLL `9a57a3dc…b843`、build结果 `03f83482…c8a2` 在忽略的 `analysis/hand-live-bone-build-20261024av/`。未重跑未变旧套件，未修改时钟、Plan、Permit、恢复合同或100ms／0.1门限。本条记录时购买版尚未测试，名称扫描在AU未被独立计时，不承诺性能收益；尚无默认安装、Native／XR／loader重建、实际0.7伤害或完整全武器／头显通过。

AV实测补记：提交 `f68be0d3…715d` 的786源／27依赖／623工件候选 `5e3cdfac…ebf0`，manifest `29810c55…1995`，runner全部函数／类AST与AU相同，保留shared-status reader。三次新鲜空闲后唯一原协议静音置后购买版局，首次post-component实际NS交还clean_close1，之后7次hand-only应用／7恢复、56原setter／target写；46末端行／23对样本、7完整末端lease、28 prepare字段写／最后观察56，末端审计通过。14个未移动初始手势结果在原0.1单位／0.1°内，最大位置0.00006209877原始单位、旋转0.01979825°；移动命令仍0，不证明大伸手、蒙皮、GPU／头显或末端覆盖的独立贡献。

第二次acquire seq822／rev34／lease8／interval2的group0／1／2原授权均通过，group4在ordinal724最终check10 NS拒绝，known127／checked2047／rejected1024／mask0；MS票龄93ms、距原截止7ms，commit stereo／input源龄116.2897／116.2737ms。四段prepare6.9852／6.5731／6.5220／6.6998ms，hand_direct2.0578／2.1360／1.9898／1.9756ms，均返回true；它们不是受控提速比较，未隔离Name读取。后续checks11／12和Policy内部完整原因不补证。原generic fault保持，累计组件2尝试／1提交／2恢复、setter11、fault／cleanup_failed1，最终dirty／owned／revoked0；正式passed／cleanup仍false。无访问文件错误不能证明普遍I/O修复。

独立8控制器typed观察／3组件／44挂接、入口恢复、Stop0／Shutdown0、音频8会话恢复pending0及确切自有owner／game／XR退出均核对通过；Native／XR子会话独立通过。59文件封存 `627b8fa5…2cdd`、root结果 `bebcc4f8…630a` 位于忽略的 `analysis/hand-mesh-runtime-1024av/`，原失败及前序封存不改写。候选没有默认安装。下一步向Astra提出一个有界整段输入预算问题：只在无写preflight后的合法位置消费真正更新的完整packet并新建Plan是否成立，必须保留pose／证书／目标一致，不跨VM借metadata、不在部分写中换Plan、不续期旧Plan、不改100ms或generic拒绝。尚未接线，完整目标、真实0.7伤害、其余全武器和头显仍未完成。

## 2026-10-24 — AW 两次原 component 回调读取的固定计时

Astra整段预算审查 `7b5b6341…856` 否决仅后读packet或后采钟重建Plan：同serial返回none，证书源时间不变；当前发布依赖接受的renderer/family提交，没有同PlayerTick内必有新完整包的证据；callback中Plan／pose／target／journal已属同一事务。AV postflush至最终拒绝68.4411ms，扣除已测initial／prepare／hand_direct尚余28.8042ms，只是窗口差值，不能当作某函数成本。因此本次只给原ContinuousAuthorize的两次checks.read增加各一对QPC，不变调用／gate／比较／短路／VM pause。CallbackRow新增两个固定span，measure显式区分read_before／prepare／read_after／hand_direct，其他site不采钟、不借旧span。最终原authorize之后、析构和SEH finally不添时钟，dump追加八字段，旧字段相对顺序保持。Astra四输入窄审 `e55a6f14…8b93` 通过；不称性能修复。

实际cost／完整trace／finally和两读取调用块进入10类／137项检查。首次fixture编译因host的State与导入State歧义及checks计数器遮蔽失败，原代码／日志保留，未执行该exe；R2只把host读取类型放入内层namespace并改host计数器名，四段实际源码字节不变，严格编译运行通过。覆盖四span独立、未知site／inactive不添时钟、原调用和短路次数、真假读与后续比较分离、未知Auth不借旧值、Windows SEH保持未闭span、原finally恢复index以及实际wide dump字段值。QPC和read／prepare为显式host服务，不是原引擎成本；结果 `7b603ac1…f52d` 在忽略的 `analysis/hand-callback-read-cpu-20261024aw-r2/`，首次失败另在无r2目录。

冻结786源／27依赖，唯一完整PG严格构建、23导出及七实际运行源OFF一组两次/EP非空行相等通过，DLL `b10adbd5…86ba`、build结果 `1345e877…d59c` 在忽略的 `analysis/hand-callback-read-build-20261024aw/`。其余未变旧套件不重跑。本条记录时尚未实际购买版计时，没有新包／旧Plan续期、权限／setter／恢复／100ms／0.1门限改动，无默认安装或真实0.7伤害、完整运动／全武器／头显通过。

AW实测补记：提交 `1448030` 的786源／27依赖／635工件候选 `5f6994fd…4344`，manifest `1309ad5f…fbc3`；runner全部函数／类AST与AV相同。三次新鲜空闲后唯一原协议静音置后购买版局，首次原MS交还clean_close1，之后7手应用／7恢复、原setter／target56，末端46行／23对／7完整lease、prepare字段28／最后观察56，末端审计通过。14份未移动初始手势原0.1门限内，最大位置0.00002861023原始单位、旋转0.02157564°；仍0移动命令，无大伸手／蒙皮／GPU或头显证明。

第二次acquire seq815／rev33／lease8／interval2，group0／1／2通过，group4 ordinal724在原早期MS check1拒绝：known67／checked3／rejected2、mask0；live_now距ticket109ms、比原cutoff晚9ms。commit_now／authorize_ns／commit_ns仍0，后续Policy和NS检查未执行，不能再称本局最终NS拒绝。四组新增read_before实测1.4473／1.5416／1.4049／1.4802ms，read_after1.4532／1.4610／1.4014／1.5402ms，均closed和原bool true，合计11.7298ms；prepare6.9330／6.9164／6.7226／6.8304ms、hand_direct2.3653／2.1035／2.0869／2.1511ms。仅是该局单次真实测量，无受控提速结论。原generic故障锁存，组件2尝试／1提交／2恢复、setter11、fault／cleanup_failed1，最终dirty／owned／revoked0；正式passed／cleanup仍false。

独立8控制器／3组件／44挂接／入口恢复、Stop0／Shutdown0、音频8恢复pending0、确切自有owner／game／XR退出均通过，Native／XR子会话独立通过。77文件封存 `6c0eb53b…965f`、root结果 `a0e8aaec…28e3` 在忽略的 `analysis/hand-mesh-runtime-1024aw/`，保留原失败。下一最小候选的Astra静态建议 `3ca7d9f7…1db2d` 只合并pawn的Controller／InvManager／Mesh／Weapon四个地址查找，scope在PC两项后结束于weapon／slot VM前；保留lookup::field_at全部严格检查、精确ComponentProperty及原非fast／未启用names路径，不承诺耗时收益。本条记录时该优化尚未接线，完整目标、真实0.7伤害和默认／全武器／头显仍未完成。

## 2026-10-24 — AX 同次 pawn 四个关联字段的地址批查

只在TAIL且fast／names启用时，current及current_direct把Controller／InvManager／Mesh／Weapon四项地址定位合为局部PawnLinkFields。每项仍调用原lookup::field_at，保留当前owner／class、property／class注册、dimension1、bytes8、精确属性类型、offset／class extent和copy；Mesh严格ComponentProperty，无ObjectProperty回退。首尾按原derived→base首同名规则重解析并比较地址／class，四项值仍按原顺序读取比较；scope在PC两项后建立，返回销毁后才进入weapon／asset关联和原slot VM。fast=false、names未启用和OFF原路保持，原engine／slot调用数、Plan／Permit／100ms／恢复不变，无新撤权或析构读取。首批遍历可能更早拒绝，末次增加保守拒绝；地址相等不是全metadata末次复核／原子性证明。Astra四输入窄审 `261b7212…48ab` 通过，raw inverse恢复原件7f2af93d…ba48，CRLF保持。

mono只准备5份原始输入／27实际raw片段／36pins，root审查后唯一严格编译运行首次通过12类／131项；实际结果与准备时未执行报告分开保留，在忽略的 `analysis/hand-pawn-link-cpu-20261024ax/`。包括真实lookup／playground批查／stow fallback／SEH copy和原新九关联后缀，覆盖继承／大小写／首错误类型、精确component类型、维度／注册／边界／未知／错值、begin后metadata变化、finish前class或地址替换、当前新调用重读、PC／pawn／weapon短路及真实保护页Name边界。宏只机械改copy／pointer函数名以统计，实际body执行；完整GT／身份前门、slot VM与engine不在此fixture范围，不作假成功服务或性能证明。

冻结786源／27依赖，唯一完整PG严格构建、23导出与七实际运行源OFF一组两次/EP非空行相等通过，DLL `14555b80…e5105`、build结果 `a11ff380…f3c1d` 在忽略的 `analysis/hand-pawn-link-build-20261024ax/`。构建与CPU独立进行，运行前须两者通过；未重跑未变旧套件。本条记录时购买版尚未运行，AW的11.7298ms是整个两read总成本，未隔离pawn4，不承诺本候选可省多少或完整8动作通过。无默认安装／Native／XR／loader重建，真实0.7伤害和完整全武器／头显仍待接线与验收。

AX实测补记：提交 `fc92520` 的786源／27依赖／647工件候选 `4d3d0f3b…9afc`，manifest `7f3ef2ca…35f5`，runner全部函数／类AST与AW相同，保留原shared-status reader。三次新鲜空闲后唯一原协议静音置后购买版局，首次原NS交还clean_close1，随后7次hand-only应用／7恢复、原setter／target56，末端46行／23对／7完整lease、prepare字段28／最后观察56，末端审计通过。14份未移动初始手势均在原0.1单位／0.1°内，最大位置0.00006151015原始单位、旋转0.02463807°；仍0移动命令，不证明大伸手／蒙皮／末端覆盖独立贡献或GPU／头显通过。

第二次acquire seq814／rev34／lease8／interval2，group0／1通过，group2 ordinal674在平移最终原NS check10拒绝：known127／checked2047／rejected1024、mask0；MS票龄78ms、距其截止仍有22ms，commit时stereo／input源龄100.4253／100.4112ms。三组回调的两原read均closed且返回true：read_before1.3524／1.3800／1.3260ms、read_after1.3402／1.3198／1.3621ms；prepare6.5683／6.7105／6.5050ms，hand_direct3.3145／2.1886／2.0588ms。各值属于该局单次观测，跨局没有受控比较，也未隔离pawn4独占成本；不能称性能修复。后续checks11／12和Policy内部原因仍未证明。原generic故障锁存，组件2尝试／1提交／2恢复、setter9、fault／cleanup_failed1，最终dirty／owned／revoked0；正式passed／cleanup仍false。

独立8控制器typed观察／3组件／44挂接和入口恢复前后相符，Stop0／Shutdown0、音频8恢复pending0、确切自有owner／game／XR均退出；Native／XR子会话独立通过。78文件封存 `3b4e5fce…dd7f9`、root结果 `b0e227ce…b544` 位于忽略的 `analysis/hand-mesh-runtime-1024ax/`，保留原失败，不为获取通过而重复启动。完整运动仍未解决，无默认安装或全武器／头显通过。

并行推进用户新增的单双手伤害要求：已有静态证据和本轮有界KB查询仍不能证明当前零售版Result ABI、逐发／弹丸／投射物归属。Astra只读探针方案 `d260e44e…11b1c` 建议复用同一次四参数CallFunction原调用，在固定POD中保存完整身份、实时唯一Float ReturnParm布局、原Result地址的前后4字节与同代调用配对；不使用caller参数区、RAX或Result+ReturnOffset，不写游戏内存。方案本条记录时尚未实现或运行，不代表0.7／1.0倍率生效；枪体时序与完整功能继续保留为未完成项。

## 2026-10-24 — AY 原伤害结果的默认关闭只读采样

为接用户确认的单手0.7／有效双手1.0规则，新增独立默认OFF开关 `GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE`，依赖现有两个动画观察开关。只在原四参数CallFunction同一次调用周围采样，原before-action／handanim顺序、四参数、调用次数和返回位保持。完整有界copy-known身份／Name外链、receiver继承和实时pawn.Weapon／weapon.Instigator、函数子字段链及唯一ReturnParm的flags／dimension／width／offset／ParamSize／extent进入固定POD；局部首尾复核后只发布完整布局，失败只给阶段／原因。Result只取原地址4字节，不加返回offset、不借caller／RAX；这一层没有乘0.7、引擎调用或游戏内存写入。

Astra发现初稿仅凭leaf FloatProperty便标typed的缺口：现有保存证据及有界KB查询没有绑定完整原生类型身份，不能猜Core包路径。因此修后只保留完整身份与任意raw4位型；`stable_layout_candidate`只表示同代session、前后元数据和读取稳定，`typed_candidate`固定false，输出 `native_return_type_proved=0`，不把NaN／Infinity位型解成浮点。真正Result ABI、getter实际是否经过边界、最终TakeDamage与单发／同发弹丸／投射物归属仍待购买版验证。最终源审 `4cfdcb7f…cc83` 通过，只是有界只读源审，不是伤害实现或零售类型证明。

新记录器64行／8层，同代call／parent／depth配对，容量不覆盖open行。POD-only `__try/__finally` helper让异常只记incomplete并收尾新栈，不抑制异常、不伪造post；原外层Active／旧handanim的SEH生命周期没有因此修复。Stop／limit仅冻结录入，已进入token照常收尾；GT pump在inactive早退前，queued Start在旧记录drain／dump前不exchange／reset，Shutdown在原禁钩且全局callbacks为零后无条件处理pending。候选getter仅新增一次OS采钟，与原observer截止比较，不新建期限，不参与100ms写权限。四旧源只加新guard；首次生成器误统一旧LF／mixed EOL，原raw逆变换失败保留，恢复原行EOL后四件raw inverse全等（`306eff09…c0c0`）。

两个独立本地CPU fixture均由助手只准备、root审查后首次严格编译运行通过：记录器33类／727项、metadata reader18类／594项。前者直接使用实际完整POD／dump、原hook／SEH helper、finish／post_tick和实际Ledger，candidate／read／same与无关snapshot／指针为明确host服务；后者include完整实际reader和原SEH copy，只用本进程GObjects／GNames／元数据及声明的after-copy变更，覆盖身份／首同名拒绝、完整布局／extent、fresh变更／同代配对、未知／边界／真实PAGE_NOACCESS。容量内部子场景不冒充完整read，Core名字只是构造模型。Snapshot实际33208字节；不证明零售ABI、Result、全局Shutdown或性能。R1输入保持，R2准备器一次文本替换断言失败单独保留；未发生编译或CPU失败。实际结果在忽略的 `analysis/weapon-grip-damage-recorder-cpu-20261024ay-r2/attempt1/` 与 `analysis/weapon-grip-damage-metadata-cpu-20261024ay/attempt1/`，准备报告保留其生成时未执行事实。

冻结788源／27依赖，唯一完整PG严格构建、23导出及新开关OFF一组两次/EP对比通过：展开9份原运行源与11份新运行源，覆盖实际handanim／call及两新文件，仅额外两项正guard标准头include，同条件非空行相等。DLL `3e2c6035…5ab6`、build结果 `a375078f…74ac8`；不重跑未变旧套件、不重建Native／XR／loader。root复核788当前源与冻结源、27依赖及两套实际结果一致，总记录 `c0fe1e37…5e77d` 在忽略的 `analysis/weapon-grip-damage-closeout-20261024ay/`。本阶段没有启动新游戏或安装普通启动器；AX枪体时序、完整伸手、全12武器、真实0.7伤害和GPU／Quest／头显仍未完成，完整目标继续active。

## 2026-10-24 — AZ 原版 R2 的真实伤害查询观察与 BA 字段链修正

AZ 冻结 AY 原探针为独立788源／27依赖／654工件候选 a7ec72cb…353be，manifest 7e8eeda7…a0662，DLL保持3e2c6035…5ab6；Native／XR／loader未重建。新增纯日志审计器的16个人工合成方法首次通过，完整字段表／外链／flags／布局与unknown均按实际格式核对；只声称日志完整性，不解码float、不证明Result所有权或逐发身份。结果位于忽略的 analysis/weapon-grip-damage-audit-root-20261024az/，没有重跑未变旧套件。

两次全新私有存档／配置、静音置后购买版运行各先通过三次新鲜空闲检查。首局在按R2和启动观察器之前因误向PC查bNoWeaponFiring而停止，未伪装成字段false；保存的Pawn／Weapon脚本说明该字段属于Pawn。AZ-r2只把这一处对象参数pc改为pawn，raw逆变换可恢复原脚本，旧局23文件封存03c12d42…00a17保持。修正局原版Lancer R2经ButtonPress／ButtonRelease，typed held实际false→true→false，整个触发／释放约0.2663s；只调用这一种入口，没有直接调用getter／伤害函数或启动VrFire、handstream、Native／XR会话。原SetRotation与同世界Trace仅证明命中准备，不证明实际射线或逐发命中。

AZ-r2自然经过4次GetFireModeDamage原调用，原返回均完成、同代session保持，记录无gap；8个pre／post元数据均为stage6 pawn_weapon、reason19 field_limit、ordinal512，因此0个有效raw4对，审计为protocol_valid但incomplete，正式passed仍false。它证明这次观察点可达，不能解释为4发子弹或已知零伤害。原朝向精确恢复、Stop／Shutdown0、两入口原字节、自有游戏退出和8音频会话恢复pending0均独立通过；19文件封存c202517a…74503。具体回执、原日志和失败保留在忽略的 analysis/weapon-grip-damage-runtime-1024az/ 与其-r2目录。

BA仅修改读取器的单类兄弟字段查找上限512→4096，参照既有反射发现的4096项规模；原function字段64／parent40／固定Snapshot33208字节、身份／注册／首同名／类型／范围／循环与最终复核全部不变，没有写游戏伤害或输入。raw逆变换与CRLF保持。实际完整reader和原SEH copy在本地合成元数据上新增8类／42项首次严格编译运行通过，覆盖长链600、恰好4096／4097边界、长链循环、晚到的错误同名类型、失注册及最后指针变更；未重跑旧18类。结果在 analysis/weapon-grip-damage-search-cpu-20261024ba/。唯一完整PG严格构建、23导出与11实际运行源OFF一组两次/EP对比通过，DLL87ad5e25…59f17、build结果37d22a60…3a743；此条记录时修订读取器尚未购买版运行，没有性能或原生ABI结论。

用户进一步确认：0.7／1.0只用于玩家枪械射击，包括Boomshot／Torque Bow等发射的投射物；手抛Bolo／Smoke保持原版伤害与效果。设计与验收项已同步。实际倍率仍未接线；双手／枪体完整运动、全武器装填／交互、普通启动器集成与GPU／Quest／头显验收均未完成，完整目标继续active。

BA实测补记：d71ac22源码冻结为788源／27依赖／659工件候选4a7ec186…84663，manifest cc9d5a91…50c17，DLL87ad5e25…59f17；沿用原Native／XR／loader，只替换PG并增加5份BA证明。新runner与AZ-r2反向替换候选／路径／hash／工件数后原字节相同，原R2协议、时限、Stop／Shutdown和恢复均未改变；日志审计器只更新reader源pin，解析／判定代码与已通过16项的原件完全相同，不重跑旧逻辑检查。

三次新鲜空闲后唯一新候选静音置后购买版局，原版Lancer held再次false→true→false，触发／释放约0.2796s，自然记录4次getter、4对完整raw4，前后session／schema保持、无gap／unknown，正式只读采样及独立cleanup均通过。实际每份metadata为35身份／32名字／14结构／5函数字段，接收者继承7级；返回属性完整路径Core.FloatProperty，ReturnValue flags0x580、dim1／width4，函数parameter_bytes24／return_offset20／extent36。两入参为HitLocation（结构大小12，offset0）和HitActor（Object大小8，offset12）；offset20只记录，不加到Result，不从RAX或caller参数区取值。后值按任意raw32保留，尚未认证原生类型／Result所有权、调用与实际射击／伤害包归属，更没有乘0.7。

原朝向精确恢复、Stop／Shutdown0、原32／64字节入口、确切自有游戏退出与音频8会话恢复pending0通过；19文件封存24f3435a…33b28，汇总c969653a…1fffd位于忽略的 analysis/weapon-grip-damage-result-evidence-20261024ba/。后续另有非自有游戏活动，未干预或借用其内存，不再启动额外实机局。没有跨局性能结论、Native／XR图像或头显验收；下一步绑定真实caller／开火／结算身份，再实现枪械0.7／1.0且只应用一次，手抛Bolo／Smoke继续排除。完整双手、全武器与默认集成仍未完成。

## 2026-10-24 — 后台测试固定 0.4 渲染比例

按用户要求，后续自主后台测试采用每边0.4的渲染比例，继续静音、窗口置后和tiny-output只读观察。新增纯规划器 `tools/test-support/background_render_policy.py`，并接入 `tools/multi-instance/start.py` 的 `prepare(..., background_test=True)`；`--playtest` 自动启用。仅在新私有实例内设置render_scale并调整EngineUserSettings的8个ClientSettings启动尺寸，先验证所有计划、保留原始INI，再写私有副本；缺失、重复或未知尺寸语法会中止启动。普通实例、用户配置及已封存历史输入保持原样。XR测试仍须传递 `--render-scale 0.4`，不通过移动或缩放现有窗口实现。

精确启动器比例校验与两个真实历史INI副本的字节逆变换通过；实际prepare函数的本地文件模型覆盖普通路径不变、后台0.4与两份原INI完整备份、第二份非法INI在任何计划写入和manifest发布前拒绝，三类检查通过。游戏验证／置后构建检查在该模型中是明确host服务，不算实机验证。记录位于忽略的 `analysis/background-test-scale-20261024bb/` 和 `analysis/background-test-scale-integration-20261024bb/`。本次没有启动游戏，实际渲染尺寸尚待下次空闲运行核对；不据此声称FPS、GPU或头显通过，完整6DoF目标仍未完成。
