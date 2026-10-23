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
