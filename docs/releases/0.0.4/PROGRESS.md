# GearsVR 0.0.4 开发记录

## 2026-10-24 — 明确真实站蹲／物理探身与抓边并存

按用户纠正，改写[交互规格](../../6dof-cover-interaction-design-20261024.md)第4节：真实站起、蹲下、弯腰和左右物理探身均直接影响视点及掩体暴露，无需LGrip；左手抓边提供额外虚拟位移，可以单独或叠加使用。仅对“位置完全不变的纯旋转”禁止额外程序化探身，不能丢弃自然转头／抬头／侧倾伴随的真实位移。虚拟中立眼点不能将最终视点锁在顶沿以下，松手保持也不能锁住真实头部运动。

同步操作表、实现P4、AGENTS与早期文档的解释，新增C19–C21，当前为21项掩体验收及4项跑动稳定验收，共25项。此轮为需求修正，未修改运行时代码或默认构建。

## 2026-10-24 — 追加跑步／冲刺镜头稳定要求

用户反馈当前跑步镜头晃动过大，要求跑步时镜头不要晃动。已在 `6dof-cover` 的[交互规格](../../6dof-cover-interaction-design-20261024.md)第4.1节加入必做相机修复，覆盖普通跑动、冲刺、游戏允许的沿掩体跑动及起停／进出掩体过渡。去掉游戏额外跑动颠簸、摆动与角度晃动，保留真实头显运动、正常位移、地形／姿态变化、主动转向和武器行为。

源码复核确认现有第一人称预览仍采用原视图forward与Pawn／EyeHeight输入，不能因已经去掉部分位置动画和滚转就认定跑步稳定完成；本轮未认证运行时根因。规格新增独立实现项及R01–R04四项验收，连同原C01–C18共22项。已同步分支AGENTS要求；此轮仅补充规格，没有修改运行时代码或默认构建。

## 2026-10-24 — 独立 6DoF 掩体分支与交互规格

按用户要求建立独立托管 worktree `C:\Users\11\.codex\worktrees\6dof-cover\gearsVr`，分支精确命名为 `6dof-cover`。从 `codex/0.0.4` 的 `9ffa710` 出发，在该分支提交根目录尚未提交的29项相关0.0.4源码、诊断和记录，形成基线 `85fc2b3`；根目录分支、源码和用户配置未改变。与本功能无关的未跟踪内容未纳入基线。

新增[6DoF 掩体交互与实现规格](../../6dof-cover-interaction-design-20261024.md)，覆盖左右／上探、真实头动、LGrip抓边与反向拉拽、掩体朝向与眼点、原版吸附／退出、输入互斥、射击／受击／碰撞一致性、生命周期与18项验收场景。用户已明确松手保持身体位置、重抓反向移动才缩回。旧交互文档的摇杆探身、松手回位及不保留进入／退出状态已标注被替代。

源码检查确认第一人称预览仍统一水平前移0.35m、抬高0.10m，没有按掩体高度建立眼点；只定位待改入口，不将其称作经实测确认的唯一原因。已按知识库流程保存8项限定词查询，得到掩体动作／槽位／相机的脚本对象及历史字符串候选，未把声明当成当前进程可调用接口。只读核对了独立武器分支的双手输入桥及LGrip复用需求，没有合并或修改该分支。

实际support1 native／XR冻结源、二进制与来源回执在新worktree内按字节复制并核对，连同限定查询共1,196个文件；默认native仍为 `5e78c2a7…fe7a`，XR仍为 `df54d17c…1b48`。记录保存在 ignored `analysis/6dof-cover-planning-20261024/baseline-copy.json`，同时保留support1与left-history1冻结回执。当前工作区源码快照不冒充实际默认构建源；后续仍从实际冻结源最小增量开发。

文档检查确认新增规格的6个相对链接可解析、18项验收编号完整，Git差异格式检查通过；1,196个私有副本再次核对哈希一致，根工作区分支／HEAD／状态及已复制来源文件哈希保持不变。检查回执为 ignored `analysis/6dof-cover-planning-20261024/document-validation.json`。

本轮完成规格与实现准备，未改掩体运行行为、未重编默认DLL／XR、未启动游戏或新增IDA导出。下一步是只读认证原版掩体状态、合法边缘、输入分派和相机来源，再进入纯逻辑与游戏接线；自动化、真实游戏和实体头显验收均按新功能单独记录。

## 2026-10-24 — 运行时支持观测集成默认入口

按用户要求补齐五类运行证据：XR运行时实际提供的系统名称、vendor ID、追踪支持和尺寸上限；游戏交换链所用队列设备与XR实际设备的适配器信息／LUID对应关系；游戏与XR进程实际加载的图形模块文件版本，以及已安装／运行中VR组件版本；匿名交换链编号下创建、绑定接受、resize边界／返回值、Present进展和原有捕获／owned管线计数；以及首次故障及时落盘与运行中native状态保存。头显连接方式、厂商不提供的型号／版本保持不可用，不用注册清单或已安装显卡列表代替实际运行证据。

原生回调仅写有界内存并使用try-lock，文件和适配器描述查询在现有worker完成；累计全入口Present和原有受限绑定Present分别记录，不能当作实体头显FPS。使用匿名chain ID保留关联，未导出地址。原生日志两份轮转、单文件预算，首次负HRESULT另存并flush／commit；Windows并发读取测试发现安全CRT默认独占打开，已改为允许读取。启动器守护线程以进程创建时间核对game／XR身份，持续保存状态；记录采集失败原因，读不到不记作零。首次观察到的故障经fsync及原子创建保留，后续清理错误不覆盖；启动阶段另有独立首次故障回执，即使普通时间线满也保留。原生QPC、频率和观察时间保留，跨进程需依时钟字段关联；最早失败返回不自动等于根因。

导出修订为 `support-evidence-3`：增加support轮转历史、首次故障、运行中native快照、VR软件补充清单及逐项runtime覆盖状态；支持包仍筛除私人路径、账号、凭据、指针、姿态与原生指令。当前配置标明为导出时信息，后续会话另记录本次请求参数。新增源码模块已加入portable载荷白名单。正常会话未出现故障时没有首次故障文件；缺失值、截断和采集失败仍可区分。

构建严格基于已验证冻结源：先核对left-history1的582项源码哈希，复制私有源后仅加入3个native源／构建文件及共享支持header／test；XR仅对原色彩冻结pose源加入观测。保留全部原构建开关、FPS两槽与独立左眼历史，未将当前源的其他实验整体重编。默认native现为 `build-support1/GearsCaptureSupport1.dll`／`5e78c2a736526b3bf22a87023959d1635d82d8bcec80ddd05ed24fede4d6fe7a`，XR为 `build-support1/OpenXRPoseProbe.exe`／`df54d17c03068f46cad60f43c5b17114dcc61bf612321efc85611822cd181b48`。原left-history1与原色彩包保留，根GUI仍为0.0.4 Dev。冻结回执在忽略目录 `analysis/runtime/support1-frozen.json`，完整源哈希和构建记录在 `analysis/support-instrumentation-20261024/`。

最终购买版／私有配置／behind-windows模拟器测试 `continuous-stereo-30188-20261024-020809-bad3a100` 正常启动，约22.0秒有效段提交1983次场景projection、复制1914次图像（含重复投影，不能等同独立新帧数）。真实日志包含system、native／XR adapter、按chain匹配结果、双方模块快照、交换链快照及live状态，观察器无错误并正常join。native清理、入口还原通过；原10份配置／存档哈希不变，未启用截图。此前初版运行测试为 `continuous-stereo-52204-20261024-014313-c36b1934`，记录保留。最初一轮仅因测试清单仍指向不存在的旧工作副本而在启动前拒绝，随后改为当前已选正版Steam副本，未改用户选择或重启旧P2P调查。

对最终真实会话导出21项，ZIP **1,035,984 bytes**、展开 **10,386,072 bytes**。逐项确认系统／选卡／模块版本／交换链／live状态进入ZIP，私人路径／邮箱／账号格式／64位指针扫描为零；正常运行没有伪造首次故障。启动器229项测试、运行控制／支持诊断67项测试、39672项FrameLink断言、独立历史状态测试、DLL ABI及连续停止自测通过；默认GUI自检、85项portable载荷文件及默认清单哈希验证通过。回执为同目录 `final-validation.json`／`export-audit-final.json`。仅支持观测和生命周期验证，不宣称实体Pimax／Sboys验收或Chtiblue根因已解决。尚未创建0.0.4公开发布、tag或上传此本地支持包。

最终复核另修正长会话保留：原生256项事件缓冲使用循环索引，flush后持续接收后续事件，超出预算保留最近事件并显式累计丢弃数，首次失败另存不受影响；轮转重新记录已有适配器。启动器日志轮转时重放有界设备／软件上下文，保留最初观察时间，避免多次轮转后仅剩后半程计数。三次轮转上下文保持、缓冲超限后后续stop保留、首次故障不变及已满启动时间线仍能导出首次失败均有回归验证；最终native／XR采用新空build-c目录，重新核验依赖前缀及493／532项依赖后构建。

## 2026-10-24 — 一次导出保留故障定位所需的现有证据

Chtiblue的已收两份0.0.3支持包分别保留native激活失败与启动后零双目产出的摘要，但旧导出器没有带上同次native-start、native-status、session-status及完整停止原因；增加XR CSV字段还会导致旧白名单丢弃整份CSV。这是证据导出缺口，尚未证明该玩家故障的具体根因，不能从摘要推断SteamVR崩溃、相机距离或某个API layer为根因。

`diagnostics_bundle.py` 导出修订为 `support-evidence-2`，已接入正常GUI调用的同一 `launcher.py export-logs` 路径，无需专用入口或重编GUI。最近三次已识别会话各增加 `debug/session-N.json`：保留native激活错误／Python回溯文件名和行号、native生命周期／捕获／绑定计数、停止请求来源、XR错误及数值输出、CSV和菜单／世界／遥测历史。当前XR新增数值列已纳入，未知列不会使已知列全部丢失；姿态列不导出。已有启动时间线和组件哈希继续保留，0.0.4-dev版本后缀也可正确导出。

系统清单保留CPU、GPU、显卡驱动、RAM及Windows；新增只读OpenXR运行时与API layer注册清单，仅保存名称、版本、DLL文件名、清单哈希、注册状态及已声明的环境禁用标志。此清单属于导出时环境，不是旧会话实际加载证据。没有启动额外XR会话、读取游戏进程或改注册表。HMD型号、连接方式仍需已有运行记录或用户反馈；现有冻结XR尚不记录这些信息，也未增加驱动／GPU崩溃转储。

每个附件上限4 MiB和8192条记录，长文件保留开头64 KiB与末尾，记录预算优先保留首尾与错误；ZIP总展开预算32 MiB，优先缩减长历史，保留核心激活／状态内容。manifest逐附件记录读取范围、缺失／截断／变化状态、过滤／非法行数、保留数与错误遗漏数，避免将不可用当作零。历史候选池现保留较新的候选，不再在目录前512个候选后停止；目录遍历仍有200000项上限及显式告警。所有输入保持固定同前缀附件、拒绝链接／重解析点、拒绝跟随报告提供的路径；支持包过滤私人路径／账号／凭据、指针／姿态／原生指令、存档及二进制。

验证：45项导出器测试与226项启动器Python测试通过，包括激活原始错误、零出图捕获计数、停止来源、当前／未知CSV列、错误位于长历史中间、总大小预算、较新候选、隐私／重解析点与原子写出检查。复核额外修复扩展历史中过滤掉来源状态的问题：摘要与扩展记录均保留首次采样、数据推进／不变、计数重置、身份不匹配／不可用状态及XR／尺寸读取错误类型，未知私人字段继续过滤。真实三会话离线导出20个文件，ZIP **1,120,082 bytes**，展开 **11,856,449 bytes**；最新三会话与三次启动时间线核对通过，native-start／native-status／session-status及CSV均有记录，扩展遥测中fresh状态保留通过，私人路径／邮箱／账号格式／64位指针扫描为零。私有审计回执在忽略目录 `analysis/support-evidence-20261024/offline-export-audit.json`，并未上传此本地包。

默认native仍为 `a637128c…d758`，XR仍为 `58ef6c39…539a`，根GUI仍为既有0.0.4 Dev；本次只改Python导出和测试／说明，没有重建或替换冻结运行时。已发布0.0.3包／标签保留，未创建0.0.4发布。旧支持包未收集的文件无法由此更新追溯恢复；当前改动提升一次反馈的证据完整度，不表示所有未来故障均能仅凭一包确定根因。

完整度复核：当前已补齐“采集到但没导出”的重要缺口，仍不足以认证“一次反馈必能确定底层根因”。后续诊断构建应优先补足实际XR系统／头显信息、游戏与XR各自选择的GPU适配器及匹配关系、故障时实际加载的图形模块／层、swapchain创建／选择／resize／Present与捕获拒绝的关联进展，以及首次故障先于清理错误的持久记录。注册清单不能替代实际加载证据，已安装GPU清单不能替代实际设备选择，正常会话导出成功也不能替代对激活失败／零发布／运行时退出等故障包的逐项验收。本次未把这些待补项写成已实现，也未因此更换冻结XR／native默认构建。

## 2026-10-23 — 从0.0.3创建闪烁修复分支

按用户要求丢弃0.0.2分支临时未提交的启动账号识别修改和本地配置改动，回到 `codex/0.0.3`，从 `9ffa710` 创建 `codex/0.0.4`。恢复此前保留的0.0.3调查记录和用户配置；原stash仍保留，未删除其他来源文件或忽略的调查证据。根目录GUI恢复0.0.3已发布文件 `45cf2da2…7fb6` 并通过隐藏自检；默认native `7571fee6…bab9`、XR `58ef6c39…539a` 仍为0.0.3基线。尚未创建0.0.4发布或标签。

用户在0.0.2也观察到闪烁，不能据此将0.0.3新增的FPS优化认定为根因。0.0.2隔离测试已成功注入，但动态模糊开启的当前画质组合在ticket2出现 `draw_state`／flags258／pair0，双眼投影和复制均为零；不能冒充有效的无闪对照。原始配置与存档未变，两个后台测试及XR进程均已退出。0.0.2临时启动改动已按要求丢弃，不移入0.0.4。

0.0.3实际手动会话已在7200×3200完整原生SBS接收源中捕获左眼清晰／强烈模糊交替、右眼清晰，详见[既有调查](../../steamvr-left-eye-flicker-20261023.md)。本轮优先核对动态模糊和左右眼历史状态。源码存在左眼复用原玩家ViewState、右眼使用独立工厂状态的非对称性，这是待验证假设，不是已证实根因。修复尚未实施，未取得运行／头显验收。

私有回执和图像继续保存在忽略目录 `analysis/flicker-simulator-20261023/`；分支／GUI切换回执为 `branch-004-baseline.json`。

## 2026-10-23 — 左眼历史状态隔离已接入默认构建

已在同一购买版游戏、每眼3600×3200、原色彩策略、动态模糊开启的静止场景中复现用户截图的左眼强烈径向拖影。普通游戏帧与VR帧穿插时，旧版VR左眼复用原玩家的历史状态，右眼为独立状态。现将VR左眼也交给原生工厂创建的独立持久状态，保留原玩家状态供普通渲染；不通过关闭动态模糊或更换颜色规避问题。修复限定正常连续VR会话，保留诊断控制模式原行为。

| 配对测试 | 完整SBS源图明显左眼模糊 | 预览模糊采样 | 普通帧回退次数 |
| --- | --- | --- | --- |
| 0.0.3基线 C | 5 / 7 | 17 / 88 | 1325 |
| 独立左眼状态候选 C | 0 / 7 | 0 / 88 | 1463 |
| 最终默认构建 E | 0 / 7 | 0 / 220 | 750 |

完整源图为7200×3200，位于XR转换／合成之前；右眼始终保持清晰。基线左眼梯度RMS约3.61–11.69，候选C约11.69，右眼均约15.26。指标只用于这个匹配场景；预览为若干连续采样窗口，不能换算成整个会话的闪烁频率。最终E包含瞄准、暂停和重载，因此其预览与C并非同一静止阶段的频率对照。

早期A/B未复现的隔离测试不能充当无问题对照：私有启动脚本将无BOM配置写成UTF-8 BOM，游戏随后重写部分画质为低档。保留原始编码后，配置保持原高画质，基线自然重现问题。只修正忽略目录内的测试脚本；用户配置／存档没有被替换。

候选D与最终E均通过普通瞄准／射击、暂停恢复、检查点1→2代重建；瞄准／射击证据限于FOV和弹药响应，不是命中精度验收。新状态只在工厂身份、虚表和与原玩家／右眼不别名检查通过后使用；停止及换关沿原生release／延迟删除路径清理，等删除完成后才允许重建。两个会话均正常停止，双眼状态释放、钩子入口还原、XR退出通过，资源保留和原生UI／材质故障计数为零，用户原10份配置／存档哈希不变。全部测试进程已退出。

默认native现为 `build-left-history1/GearsCaptureLeftHistory1.dll`，SHA256 `a637128c5604a7c88d9dc7658a2e5248a3a5c6b76559d6ff3a0d57ab7710d758`；从实际默认冻结源而非含其他实验的当前源整体重编译，保留原构建开关与FPS两槽修复，额外启用独立左眼状态。XR仍为 `build-runtime-colors1`／`58ef6c39…539a`，未更改颜色策略。最小源码增量已按唯一上下文合并至当前源码，保留其其他已有内容。

验证包括独立状态、双眼构造、提交生命周期等9项针对性原生检查，当前源码新增状态／双眼构造2项编译运行检查，实际启动自测，217项启动器Python测试，以及GUI隐藏自检。根目录 `GearsVR Launcher.exe` 已更新为 **0.0.4 Dev**，SHA256 `4c864dc26d8376c8ebb29aacc2f4cb5a749864b3cf84d74e2048218cc570e913`；启动诊断版本为 `0.0.4-dev`。无需使用独立实验入口，正常重启即可加载默认修复。

冻结回执为忽略目录 `analysis/runtime/left-history1-frozen.json`，配对图像和完整会话证据在 `analysis/flicker-simulator-20261023/`。原生／模拟器像素与生命周期回归通过；**物理头显、完整战役及FPS效果未验收**。未创建0.0.4发布包／标签，已发布历史包保持。

## 2026-10-24 — 6DoF 掩体分支开始实现

工作限定独立 `6dof-cover` 分支／worktree。需求以 `docs/6dof-cover-interaction-design-20261024.md` 为准：真实站起、蹲下和左右探身直接生效；左手 LGrip 拉拽提供额外连续偏移，松手保持，再抓反向缩回。保留原生进入、吸附、沿边移动与退出，替换 LT／RT 自动探头。用户已授权自主后台测试及多个 agent；测试保持显式 behind-window 构建、静音、tiny-output 只观察、真实控制器后台输入关闭，所有运行均使用隔离配置／存档。

已实现纯 `CoverInteraction`、`CoverControllerAdapter` 与 `LeftHandInteractionOwner`。控制器桥的三份源码从武器分支提交 `da775ed7d12118910b140c156b9750a0c678a9bd` 按 blob 原样引入，未编辑武器 worktree。抓边测试覆盖三方向、冻结局部轴、松手保持、重抓、20000 帧重复／漂移、总范围、所有权、追踪／生命周期中断与独立头显位移；适配器 629 项检查，左手决定接口 203 项检查通过。它们是可接线的 CPU 逻辑，尚不代表原生掩体或跨 DLL 的互斥已经生效。

新增只读 `cover_state.py` 及原生调用元数据准备／FFrame 一致性捕获／有界 wire packet。当前购买版进程已实际导出两套 State 各自的 PlayerMove／UpdatePlayerPosture 和两个 setter，共 6 个函数、162 个身份及 13956 项源检查；两个 PlayerMove 的 locals size 为 20，CT／CA／CD 是三个独立 byte。暂未把 out 参数 flags、alias／copyback 或动作枚举解释为写权限。被动 observer 复用现有唯一 dispatch hook，原函数调用一次，保留返回寄存器，不读取 Result 作为 outparam，不改 gameplay。

跑动相机候选只修改当前玩家已认证 CameraBone modifier 的 `bDisableAmbientCameraMotion` 和 `RoadieRunShakyCamDampeningFactor`，保留其他 bool 位与自定义／开火动画。实际应用、读回、条件恢复已通过，2 次写入及 2 次恢复，无待恢复项。走路 A/B/A 受控输入对照中，运动帧的 CameraBone Alpha 由基线约 0.0034–0.0735、恢复后约 0.0036–0.0878，变为候选阶段 0；候选运动样本相对控制器的额外 pitch／yaw／roll 变化为 0，camera Z−Pawn Z 恒为 76 native units。基线与恢复阶段对应角度变化约 0.077°–0.253°、高度仍有扰动；角色继续真实移动。这是普通相机缓存读回，尚非 GPU／头显证明，也尚未证明疾跑与掩体进出。

一轮初始输入检查因测试窗口盖住副屏任务栏而拒绝，没有绕过保护或运行时调整窗口；后续在隔离副本启动前使用 1280×720 窗口配置，后台检查通过。另一轮请求实际触发翻滚与剧情通话，明确排除出疾跑验证。用户正常 10 份配置／存档在已结束的隔离会话前后哈希不变。

尚待连接并验证：有效掩体中的姿态结果→setter 同调用回执、合法边缘与连续身体／碰撞、面朝掩体与遮蔽眼点、实际武器／左手消费者、疾跑及过渡、头显验收。`CoverMovementPct` 的现有脚本用法是速度归一化，不能凭名字当探身权重；单人原生掩体／盲射免伤分支也必须纳入身体暴露验证。默认 support1 native／XR、独立左眼历史与原色彩策略保持；未发布新的版本或标签。私有证据位于本 worktree 忽略目录 `analysis/cover-implementation-20261024/`。

## 2026-10-24 — 正版矮掩体端点与相机会话恢复

在隔离副本加载已有医院测试进度，四份来源存档逐项核对 SHA256，并只映射到实例内已有的 profile 文件。该进度保留 assisted／既有 health assistance 标记，不构成无辅助战役验证；正常存档与设置没有替换。医院最近的可上探矮掩体实际进入成功，`CoverType=2`、slot 0、原生 stationary 标记成立；原版瞄准产生 `CoverAction=7 / CoverDirection=3`，释放后恢复 0。后退沿原逻辑清除 cover、退出吸附；输入驱动在特殊状态转换时中止继续施加移动。测试用户原 10 份文件的哈希仍保持。

新增只读动画图 sampler，验证当前控制关系、Mesh Owner、Animations 实例与模板区别、SkelComponent 回链，以及选中节点的 Children／MaskList 元数据和结束一致性。在真实角色上读取到 177 个节点，含 2 个 CoverBlend 和 18 个 PerBone blend。该矮掩体中实际有权重的 CoverBlend 从中立 child 0 切到原生上探 child 4，收回后恢复；提前读取的动画过渡样本因值变化被拒绝，稳定后通过。13 组新增 synthetic 测试与已有 cover／world 测试合计 41 项通过。SpaceBases 为 115 个 Matrix、stride 64，RefSkeleton 反射仍为 Int4，未把它猜成骨名表或擅自解码 opaque 数据。

被动调用候选从 support1 实际冻结源做最小增量，保留独立左眼历史和原色彩。注册表只允许在数组地址／容量不变时增加 count，选中身份、脚本、函数、类型及其他源字节仍逐项复验；回退、搬迁与已选源变化拒绝。新候选用显式 event 开启有限观测窗，不轮询文件、不发送 gameplay 命令。两次窗实际开始／封存 64 个 PlayerMove 调用，拒绝与 dropped 均为 0；192 条 updater／动作 setter／方向 setter 回执保留同调用的 frame、locals、原始字节及代码位置。两窗分别覆盖稳定中立与稳定上探；由于每窗开始时动作已稳定，这批数据尚不证明输出从旧值改变到新值的 alias／copyback，下一轮在按下／释放前开窗取证。绝不把 Result 地址或不透明 RAX 当作 outparam。

候选 native 为 `build-cover-observe4/GearsCaptureCoverObserve4.dll`，SHA256 `3bd0c30ca1dbc49a6d2628073c7e67f987bcc1258f1983e2a7efb484f9d7b8f0`，仅用于私有实验清单。原生依赖解析 498 项；实际会话正常停止，native drain、入口还原及 XR 退出全部通过。它没有替换正常启动器的 support1。

相机设置现由可选 `RunCameraSession / RuntimePolicy` 管理，只有显式清单能力才启用，默认能力尚未打开。准备完成、首写之前独立打开并核对原进程创建时间与镜像的 wait handle；身份查询失败不当作退出。退出时恢复仍待确认的 journal 会保留 reader／writer 与 owner 锁，由只执行清理的 non-daemon guardian 继续处理；专属 sidecar 逐次核对 session，不覆盖主报告。会话／wrapper／原 Policy／right-stick 检查合计 134 项通过，另通过真实 CloseHandle 失败路径的针对性检查。实际候选会话自动应用 2 个字段、正常关闭时恢复 2 个字段，released／closed 均为 true，pending 为空，无 policy 错误。启动器能力、portable 模块清单及诊断已接线，尚未宣称疾跑、GPU 或头显通过。

身体实现继续沿原生 cover 混合姿态与必要的局部骨链修正推进，保持逻辑 Pawn／cover anchor 稳定。没有采用每 tick 把整个 Pawn 根位置来回移动的方案。只读 native 查询的两个 getter 及其非连续冷分支、实现函数已在私有 IDA 导出，并在当前正版进程逐块验证完全一致；证书保存在 ignored analysis，不把源码散列或相同 RVA 当作当前字节证明。连续身体权重、骨姿态发布、真实射击命中消费者与掩体／盲射免伤两个分支仍待联接和验证。

## 2026-10-24 — 掩体姿态实际变化与骨骼查询接通

上一轮 T 测试于本地 05:13 结束，随后没有自动接续施工；用户询问停滞和进度后，已如实报告，并恢复实现。T 的完整日志现已解码：64 个完整调用组、0 拒绝，其中两组实际记录 updater 的 caller-local 从 CT2/CA0/CD0 变为 CT2/CA7/CD3，以及反向恢复；后续两个 setter 均在同一 caller/locals 保留新值。其余62组只证明稳定调用连续，未将两个独立窗口的数值差异误记为同次调用变化。会话停止、native 清理、80项入口还原和 XR 退出通过，原10份配置／存档再次哈希匹配。

新增限定骨骼查询 adapter，只经已加载且哈希已知的 Playground 游戏线程队列调用 `MatchRefBone`／`GetBoneMatrix`。只使用当前受控 Pawn 的 Mesh，类型、参数布局、返回宽度与所有8段 getter／实现代码在各次调用前后核对；不改 Actor、Mesh 或碰撞字段。队列已发布完成才释放请求分配，未知／超时不重试、不擅自释放。模块检查区分磁盘和当前进程：实际启动后改变的是 PE Resource 目录，Import 保持原值；只允许该购买版散列与已保存的确切 Resource 元组，其他文件头与 section 布局变化仍拒绝。修复属性查询提前扫描无关祖先全 children 链造成的读取预算超限，仅认证声明 owner；首次见证与结束复读保留，没有提高全局预算。35项针对性测试通过。

V 隔离正版会话已取得普通姿态、矮掩体收拢、原版上探和缩回的头／骨盆矩阵，各次查询完成且临时分配释放。当前角色的头骨索引55、骨盆1；该次收拢 head Z约-40.44，上探约17.75，缩回约-37.87，骨盆约-71.80→-31.91，Pawn 根位置基本不变。采样不是原子同帧姿态，端点会有待机动画变化；不能据此假设 blend权重与世界高度线性，也不证明碰撞／渲染或枪口。上探两次 getter 队列查询在有界瞄准输入中取得，随后已释放输入。

实际 CoverBlend 更新、父 blend Tick 和子权重传播函数的全部已导出代码段均与当前进程一致；反射确认 `bDoingCoverActionAnimTransition` 为原生 post 读取／写入的字段。连续半探可能被误判为长期动画过渡，需在姿态原型中单独验证新发射请求，尚未宣称瞄准／开火闭环完成。新增两套只在原函数前调用 helper、随后透明跳到原函数的 x64 thunk；合成测试121项通过，覆盖指定 volatile 寄存器、浮点／栈参数、返回通道、标志位和 unwind。真实身体姿态钩仍在接线，默认能力关闭。

用户追加要求后台游戏测试采用 `render_scale=0.4`。新 V 会话已从启动配置使用0.4，每眼1440×1280、接收源2880×1280（推荐3600×3200的该次模拟器尺寸），继续仅一份测试游戏、置后、静音、只观察输出，不改正常游戏画质或用户窗口。完整私有回执位于 ignored `analysis/cover-implementation-20261024/`。新原型尚未并入正常启动器，武器 worktree 未修改。

## 2026-10-24 — 连续上探身体原型完成接线，等待实际姿态检查

新增默认关闭的 `GEARS_COVER_BODY_PROTOTYPE`，已接入现有 native 提前 reservation、统一 install、入口认证与停止流程；运行时还需测试子进程显式环境变量，普通启动不生成原型 INI。两个透明入口在 CoverBlend tick 前恢复上轮自有 Weight，随后在原生父 Tick 完成权重计算、首个非空 child 传播前，写入 neutral/top 的 `1-u/u`。原生传播与动画求值继续执行，没有修改 Pawn 根位置、DesiredWeights、TotalWeight、CA/CD 或动作过渡位。当前仅支持实际受控角色的稳定 CT2 矮掩体、SpecialMove0 与正的 incoming weight，不代表左右探身已接通。

测试事件按 `0→.25→.5→.75→1→.5→0` 推进，10秒无新事件撤回。它是验证连续身体姿态的私有原型控制口，尚未接到 LGrip 或真实头动。反射布局从实际 receiver／当前受控 Pawn 的相关类链取得，首次写入前可随有效上下文更新；写入前逐项复验所选属性、对象和存储关系。日志按各步骤保留首轮中立／上探 child 的传播前后样本，避免第一步耗尽后续日志空间。

每个字段条件恢复仍等于本模块安装值的内容，外部改变不覆盖；world、当前 Pawn 或存储关系变化并不等于旧实例销毁，未知或仍存活的旧实例保持 pending。只有对象槽不再指向原地址或 fresh 完整身份已不同，才可退休义务。停止前给游戏线程最多5秒恢复，逾期明确未完成并保留钩子／资源，不能报成功。新 `CoverBodyJournalTest` 将68项合成检查纳入版本控制，覆盖存活旧实例、身份失效、部分写入回滚和准入；两套 thunk 的121项检查、原型 ON/OFF 编译与33项启动配置测试通过。

实际候选从已验证的 observe4 冻结源叠加上述最小增量，保留原构建开关、独立左眼历史和原色彩。body2 DLL 为 `build-cover-body2/GearsCaptureCoverBody2.dll`，SHA256 `6ae83dad2d22cc91d3d302e79a5752aa7e41dd7f12ff4b1bc83fd4db0c0779b0`，依赖及完整构建回执保留在 ignored analysis。第一轮 W 在游戏启动前因少一份 preflight helper 被拒绝，未注入；补齐后检测到另一会话的游戏／XR 测试，后续 X 在实例建立前拒绝，未叠开第二份游戏。当前等待独占实际测试窗口，没有把编译／合成检查写成身体姿态或头显通过。

## 2026-10-24 — 连续上探七档实际身体校准与独立清理

body3 修正普通 NPC 的 CoverBlend entry 提前恢复玩家 Weight 的时序问题：正常 entry 只恢复当前节点，只有 Reset、超时和停止才在已绑定游戏线程撤回全部自有字段；撤回未完成时不消费下一档输入。非目标 child 在进入 Scope／mutex 前拒绝。独立 journal 检查为 99 项通过。候选仍从 observe4 的实际冻结源叠加最小增量，body3 DLL SHA256 为 `ea3dcade1e68fb8cedba3fd298a6ba551d818dff2122d8226fa4e578ae7be8e9`；本分支没有把该实验写入正常默认启动器。

ab 隔离正版会话使用 render scale 0.4、后台置后、静音、tiny-output 只观察。当前角色稳定 CT2／SM0，活动 CoverBlend #4；七档均取得 Head、Pelvis、Foot_L、Foot_R 四个原生骨矩阵，查询全部完成并释放临时分配。各档耗时 4.64–5.45 秒，在 10 秒 TTL 内，但未达到 2 秒目标；矩阵按顺序获取，不能声称同帧原子姿态。

| 上探权重 u | child 0／4 | Head Z | Pelvis Z | 原生动作过渡位 |
| --- | --- | --- | --- | --- |
| 0 | 1／0 | -39.16 | -72.11 | false |
| .25 | .75／.25 | -22.95 | -60.46 | true |
| .5 | .5／.5 | -3.20 | -48.70 | true |
| .75 | .25／.75 | 13.30 | -37.00 | true |
| 1 | 0／1 | 28.54 | -25.56 | true |
| .5 | .5／.5 | -3.87 | -48.86 | true |
| 0 | 1／0 | -38.09 | -71.95 | false |

坐标为游戏原生单位。七档采样的 Pawn root 完全相同；最终中立相对首次中立，头／骨盆 Z 相差约 1.07／0.16。双脚有原版姿态的水平移动，Z 变化约 4.7 单位，尚未证明碰撞、受击或 GPU 对齐。实测证明身体姿态连续上探并能收回，未接 LGrip、真实头部到身体的驱动或掩体专用眼点。fresh BoolProperty 的 mask 从 `+0xd0` 取得，确认 `bDoingCoverActionAnimTransition` 在 `+0x540 / 0x20`；u>0 时持续为 true，包括 u=1。下一步需使原版 post 判断读取原生 Weight、后续 pose 求值读取连续 Weight，不能直接清除过渡位，也不能据现有结果声称已经能开始射击。

停止认证新增两个固定安装入口 `0x2a85a8`／`0xfa680` 的完整代码段 SHA256 检查，同时检查 CoverBlend 非连续冷段；它不接受 native 自报还原作为替代，未知 RVA 仍拒绝。32 项停止认证与 4 项路径关闭恢复检查通过。ab 先 Reset 至 pending=0，再正常停止：session passed、native cleanup、独立 entry restore／all_match 均为 true，XR exit 0，原 10 份配置／存档哈希一致，游戏／owner／XR 三个自有进程均已退出。此前 aa 的独立 verifier 异常仍保留为未完整验证，没有追认为通过。

完整证据保留在 ignored `analysis/cover-implementation-20261024/body3-ab-calibration-review.json` 及 `analysis/runtime/continuous-stereo-53688-20261024-150032-c1505489.restored.json`。测试存档保留原 assisted／health-assistance 标记，本轮不构成无辅助战役、射击、疾跑或实体头显验收。用户明确授权各会话直接协调测试；当前按菜单→武器→掩体排队，完整清理后移交窗口，自动测试都使用私有 0.4 配置。最终集成需基于届时最新默认 support2／菜单修复，保留其他会话已完成的内容。

## 2026-10-24 — 连续 pose 与原生动作判断分离

body4 改为固定两个寄存器参数 ABI 的 call／post 包装：每轮首 child 前安装连续 Weight，最后非空 child 的原传播返回后恢复本轮 native Weight；原 CoverBlend post 返回后再安装连续 Weight，供 pose evaluation 使用。TotalWeight 仍只由原生传播计算，未增加 CA/CD、过渡标记或 Pawn 根写入。新增跨原函数的 Scope／wrapper／事务责任、嵌套 frame、异常 finally 和条件恢复；临时 native 阶段不等于清理完成。自有 helper 的普通 C++ 故障在 RAII 解锁后停止新修改，原函数异常继续穿透；未知内部 SEH 不作可恢复承诺。Scope 构造失败也会归还自身 active 增量。

独立 ASM 319 项、生产 journal／生命周期 705 项通过。冻结候选严格从 source3 叠加五文件和一个唯一 Scope constructor hunk，反向替换整份 capture.cpp 等于 source3；CMake 与其他文件不变，实际 flags 保持，完整 DLL＋9 helper 通过。body4 DLL SHA256 为 `c3da142950b1a5967011c772661242c24bf63dfd7a60f5cfc668ce950b81d4e1`。

ae 正版实际日志已记录 u=.5：native post 的 child0／4 Weight 为 1／0，而 child TotalWeight 为 .5／.5；pose rearm 的 Weight 为 .5／.5，TotalWeight 保持 .5／.5。它证明两阶段分离，尚未证明实际半抬骨骼、过渡标志持续正确或新开火响应。ad／ae 完成中立新几何和四骨查询，实反射过渡位为 false；两轮均正常停止、独立83项入口匹配、XR0、原10文件一致、所有自有进程退出。两次校准中断来自本地文件共享竞争与过严的状态发布时间判断，已保留失败并修正只读等待。af 则为真实 startup panel `no_eligible_window` 拒绝，未 continuous activation／未射击；prime、Playground、音频与身份清理均通过，没有前台替代或调整用户窗口。证据为 ignored `body4-owned-runtime-review1.json` 与各实例回执。

ac 有界疾跑夹具通过原版 CanDoSpecialMove(4,false)、held getter 和唯一 TryToRoadieRun(false)，条件 End 分支在同 Pawn SM4 时执行，结束恢复 SM0。完整元数据检查耗尽采样窗口，没有形成 during-SM4 相机多时间戳；3.578 秒实际租借超过原1秒／后3秒目标，均明确记录未达标，不以缺失样本的零速度当作稳定结果。相机字段2写入／2恢复、held／latch、timer、native／独立入口／XR／原10文件和进程清理均通过。疾跑相机稳定验收仍待补齐。

## 2026-10-24 — 半探新开火通过，接入私有中立眼点预览

ag 同一隔离正版实例完成 body4 主验证。固定 .5 权重下，只执行一次新的 LT＋Shoot 输入动作，原武器 AmmoUsedCount 4→8；射击时 CA／LastCA=7／7、Weight=.5／.5，起始的原生过渡随后完成，射击样本 `0x540 & 0x20=false`。双层释放已确认，没有第二次发射动作。此结果证明发射响应，不代表命中精度、枪口遮挡或伤害校准。

同实例的中立／半探／全探 Head 相对 Pawn Z 为 0.708492／35.724415／67.756161，半探位于两端的 0.522254，三次 Pawn root 相同。射击后的首次快照因动画变化在 getter 前拒绝，随后保留原 owner，以新的 pose-only Reset→Advance 轮次取得两骨 half 与四骨 full，没有重复射击。最终 fields／nodes／in-flight／transactions 为0、failed=0；native、独立83项入口、XR0、Playground、音频、原10文件与全部自有身份清理通过。完整证据保留于 ignored `body4-owned-runtime-review2.json` 和 `cover-host-1024ag-body4-final.json`。

新增纯 CoverView 与 CoverInputOwner 接线逻辑：基础相机只接受冻结中立眼点和虚拟拉拽，真实 HMD 在原 stereo_matrices 中只叠加一次；身体目标单独计算虚拟偏移＋真实平移，使用调用者提供的测量曲线，不假设权重和头部世界高度线性。唯一共享 LeftHandInteractionOwner 接受 cover 与真实已提交的武器租约；cover 只优先新请求，松手身体偏移不占手，失去左手输入不冻结真实头动。新增552项视点、48项 owner 组合检查通过，既有 kernel／adapter／lease 检查保持。

新增完整8段 native getter 哈希与 ABI 适配，用动态 FName→bone index 读取头矩阵，拒绝未加载 SpaceBases、越界索引和 MasterPose 路径，38项独立检查通过。只读 neutral provider 在当前 GT／场景／Pawn／Mesh／World／Link／slot 与中立姿态认证后，冻结 HeadWorld−PawnLocation；201项检查通过。CoverCameraPreview 将它接在 first-person prepare 后、stereo_matrices 前，成功时跳过 cameraCloser；只在测试子进程精确环境变量启用，普通路径默认关闭。首次冻结 PawnYaw+π 为本场景面墙的预览候选，尚未旋转身体 Mesh；100ms校准等待和未知上下文保持原有路径，进入过渡仍需实际调校。1236项合成接线、body ON/OFF 单TU编译与配置拒绝检查通过。body5候选从 source4 最小5文件叠加，DLL SHA256 `156085d845021507be77e0a56da34aa19d3f82f8f3820ed57362455e1730d0b5`，完整9 helper通过；实际眼点／朝向与直接 getter 验证待执行。

只读 CoverLink geometry sampler 已实际取得 Link／Pawn typed变换、slot flags／offset及 live Stand130／Mid70、StandingLean[0,78,69]／CrouchLean[0,70,19]，未将这些本场景值硬编码为所有掩体。Python世界组合仍标记 candidate，未等同碰撞表面。左右分支与伤害保护继续独立验证；没有以本段结果宣称 LGrip、左右／上抓边或受击已完整接通。

## 2026-10-24 — 流送关卡校验修复与低眼点实际验证

ah 的 body5 相机未应用：provider停在 current_link，applied=0。当前只读几何成功通过 Link／Pawn／PC 的 WorldInfo 相同检查，但 Link.outer 与 Pawn.outer 不同；原 provider 错把同一世界的流送关卡掩体拒绝。修复分别认证三个角色的 WorldInfo 属性类型、owner／extent、继承路径与引用类，在相同游戏世界内允许不同 Level.outer。没有仅删除身份验证。neutral 320 项与 camera 1257 项通过，body ON／OFF 编译通过。body5b 从冻结 source5 只替换 neutral 一个文件，其他源码、renderer flags、body4事务、CMake不变；DLL SHA256 为 `e4893743f7563fd1cb934c104506ef4bba4e74c26f08192e1bc77caaf74b5400`，完整 DLL＋9 helper检查通过。

ai 在0.4后台正版实例中，原生 getter实际校准Head55，固定绑定一次，最终camera_applied=679。中立、first-person开、first-person关、唯一一次原LT均accepted；LT期间CA0→7，没有射击。四阶段保留的基准眼点与固定朝向、相对Pawn位置在日志精度下均未变化。中立眼点为[-381.723053,-3132.8313,-29.4882965]，相对Pawn高度9.59703；first-person事件确实消费两次，最后requested=0。它确认新眼点路径实际运行及对旧相机抬高的覆盖，尚未证明身体Mesh面墙或枪口／受击对齐。

最后after_aim的只读动画快照因“Cover value changed during observation”拒绝，整轮complete仍为false；没有重发动作，也未把该快照当作相机故障。ai没有重新采集碰撞几何。与ah同fixture、同Link188的candidate顶部比较，眼点约低43.49单位；这不是已认证碰撞面。现存PNG属于启动普通视角，不能替代后续掩体画面或头显验收。后续诊断增加实际每次base view的origin／从矩阵反算的origin／Scene invocation，避免将初次校准日志误作当前渲染矩阵证明。

ah和ai均正常Reset／Stop，native及独立83项入口检查、Playground、XR exit0、音频9／9恢复、原10配置／存档哈希、自有进程退出全部通过。证据保留在ignored的`body5-ah-sameworld-evidence1.json`、`body5b-ai-runtime-review1.json`及两轮final报告；默认support2／menu-modal1未替换。

## 2026-10-24 — 连续实时目标与几何准备

新增独立side profile，仅在当前原生镜像下将child5作为单侧连续端点，尚未写镜像或混合二维姿态。live target入口按当前GT、完整Pawn／Mesh／World身份、world generation、严格递增frame／QPC和100ms时限接收任意u。每轮冻结自己的u与endpoint，新的输入仅供下一轮使用；传播、native-post、pose-rearm及停止恢复沿用既有事务。重复frame不刷新时限，未知身份继续保留恢复责任。中立判断使用实际u，不能因小幅探身落入诊断bucket0就当作中立。1717项事务／live检查、24项关闭构建检查和64组环境组合通过；此时尚未完成新的实时输入游戏验证。

新的相机／输入组合从同一份StereoResult读取精确匹配的controller-v3 frame，raw追踪坐标映射与双眼使用同一reference；真实HMD平移只给身体目标，基础相机只接虚拟偏移。手部失踪不应冻结真实头动，头显固定位置旋转不生成姿态位移。私有top实验显式使用ag测得的三点高度曲线，不把该角色／场景标定当成通用常数。合法表面与可达性未认证前，runtime的LGrip获取仍关闭；没有把只读候选平面升级成抓握许可。

Native只读geometry模块认证当前Link／Pawn变换、live高度、slot结构212字节与选定Bool的D0 mask，655项启用和2项关闭检查通过；所有组合位置／顶沿均标记candidate，未建立原生getter或碰撞证明。动画诊断补充选定child的bMirrorSkeleton反射offset、原始word、D0mask与解析值，不写字段；新增镜像类型／范围／竞争检查与既有动画、几何共27项Python测试通过。接下来在同一隔离实例验证真实模拟头显驱动，并只读捕获两个原生slot getter与Actor.Trace的当前证书，继续闭合合法抓边。

## 2026-10-24 — 连续物理上探实测与统一左手接线

AN在正版隔离实例完成9段模拟HMD运动，render scale=0.4，窗口置后、静音、tiny-output只观察。每段都将Native选择的精确sequence与全量XR CSV关联，全部保留source1；不属于真实头显验收。同位置yaw15°／pitch10°的physical up与top weight均为0；上移0.15／0.30／0.55米得到u=0.214188／0.428376／0.811942。后两段Head相对中立Z增加33.289／59.440，Pelvis增加20.093／37.839；归零后child0/4恢复1/0，四个骨查询阶段PawnRoot逐值不变。骨查询非原子限制保留。左右各0.20米仅证明不误触发top，尚未证明侧探身体。

九段camera base及HMD应用前matrix origin保持不变；它们不是最终双眼矩阵或GPU画面的验收。原姿态已恢复，Reset后pending／fields／nodes／in-flight为0；Native、独立83项入口、XR exit0、原十文件哈希、音频9/9、PG关闭及准确自有进程退出均通过。证据为ignored的`body7-an-motion-summary1.json`及`cover-host-1024an-camera-final1.json`。AM先取得当前GetSlotLocation／Rotation入口，AN再将两个wrapper与implementation的7个完整chunks同当前内存两次稳定读取逐字核对；尚未调用这两个getter或认证墙面碰撞。

统一左手接线现在覆盖所有有效Scene，包含未进入掩体的ordinary CT0。角色绑定独立认证当前PC／Pawn／Mesh／World／Weapon及typed Instigator，不依赖body布局曾经初始化，支持已知空武器和换枪。Native只解码一次逻辑LGrip，冻结帧后执行weapon prepare→唯一owner分配→weapon commit→cover complete；未结束的prepare事务、callback与模块引用在停止／unregister期间继续保留。迟到unregister仅对精确已退休cookie返回transport quiescence，不把它当作PG资源恢复证明。

PG忙碌或提交失败时，当前新Scene可以重新认证上一次已提交的偏移并组合当前真实HMD；不发布本次未提交拉拽或租约，避免视点跳回中立或让真实起身依赖左手。事务745项、shared组合30场景568项、runtime ON/OFF 424项及独立binding579+1+1项检查通过。Native8选定source8c：只将19组输入／生命周期hunks和两段CPU targets移植到frozen7，均能反向逐字恢复母体，不携带当前树的其他NGX实验；DLL SHA256为`3a05b91fa0453dda9e8a32916f05330c681f9c9291a901bd291e4e4fad93ed55`，23原DLL/helper和85个focused CTest通过，实际依赖516项。V1注册／注销导出已存在，两ABI头与武器5565eec相同；实际PG注册联调尚未完成。

LGrip runtime获取仍关闭，候选slot几何不能授予抓握。两侧镜像、完整上／侧组合、Mesh朝墙、碰撞／受击／枪口一致性、跑动SM4实测和最新默认集成仍待完成。固定child0/4/5只有三角形姿态域，不能把归一化三权重称为完整二维跟随。默认support2／menu-modal1及其他工作树未替换，无新release/tag。

## 2026-10-24 — 原版左右姿态、原生掩体坐标与LGrip输入实测

AO使用隔离正版、0.4 scale及原有后台保护，关闭body／camera／input三个原型，取得完整七阶段原版数据：CA依次为0、3、0、4、0、7、0；左侧CD1、右侧CD2，始终同一Link／slot0／CT2。三条原生LT租约全部确认释放，五条平移租约都确认归零及writer关闭，没有Shoot。四个已知骨骼各阶段查询通过，PawnRoot逐值不变；这些getter仍非同一原子姿态快照。原版左右child5 Weight均为1，top为child4=1；child0／4／5边的bMirrorSkeleton全部false，Pawn.bWantsToBeMirrored则左true、右false。Mirror_Master与TransitionBlend祖先尚缺typed权重／镜像值，不能据child5边值就认定原版左右的翻转位置，更不能把child5=0当作全局镜像切换不会改变中立姿态的证明。

新增`cover_surface_getters.py`通过现有已附着PG队列执行原生GetSlotLocation／GetSlotRotation，不另建hook、不注入新模块。先后认证当前UFunction签名、Link／World／Pawn／slot、7个完整代码chunks，任一身份／代码／数组头变化即拒绝。12秒总界限与单次1.5秒队列界限内，未知pending保留回执，不重试。AO实际getter pair返回Location=(-388.7710266,-3114.7421875,-58.9570160)、Rotation=(0,-31860,0)；现有候选位置相距0.0060243游戏单位，只说明该slot位置接近，不认证顶沿、墙面或可达性。另7个RotatorMatrix／MirrorAxes／mirror consumer chunks与当前游戏匹配。新增14项和相关13／35项共62个Python检查通过。

AO全量CSV有11332个连续published/source1帧。Native清理、独立本次实际80/80入口、XR exit0、原10文件哈希、音频9/9恢复、PG关闭、tiny worker及三个准确自有进程退出均通过；原型关闭时不把本次80项写成83。证据保留于ignored的`native-pose-ao-summary1.json`、`cover-host-1024ao-camera-final1.json`和`signed-side-ao-evidence-review-astra.json`。

独立测试模拟器grip1仅将私有文件通道的optional squeeze接到已有XR action状态，保留source1和raw追踪标记，原运行时与正常配置不变。真实v3 receiver在8秒无游戏bench回读0→1→保持→0→1→完整pose恢复，保持不重复rising，最终累计press/release各2；原10文件和前景窗口不变，端口空且两个自有进程退出。XR exit1为dummy接收端没有游戏GPU提交的已知结果，objects_destroyed／shutdown_completed另有成功回执，不称为游戏渲染通过。第一次bench因Operator端口早于实际模块receipt而提前拒绝，0输入，失败与后续自然退出检查保留；第二次增加有界receipt等待并按实际raw head_flags=15核验后通过。证据为ignored `simulator-grip-source1/bridge-bench2-receipt.json`，它验证输入链，未验证真实抓边获取或身体拉拽。

Native9统一CallFunction入口正在离线接入。最外层UI hook改为POD placement storage与finally逆序恢复，游戏SEH／RAX／四参数不变；接入辅助桥后的生产outer hook已通过763项检查。辅助通知与模块pin必须覆盖pre→original→end整个空隙，通知fault或不确定结束保留sticky debt；完整冻结DLL构建与真实PG联调仍待完成。局部child5镜像仅准备显式opt-in A/B候选，不作为已认证的原版左右开关。所有未完成的完整玩法与默认集成范围继续保留。

Native9随后完成fresh冻结构建。normal DLL SHA256为`50601ea6bf86ffcd6b0532e62975ed80704198846ba4d02ae660ff5f2cf6d2dd`；以frozen8c的628文件为母体，仅12文件变化、7个capture hunks与CPU CMake块，capture／CMake反向逐字恢复，其他文件与渲染flags保持。三个注册／注销／辅助attach导出存在，hand V1与补充ABI分别保持5565eec／32b416a。23个原DLL/helper和94个focused CTest通过，包含41个runtime独立场景、30个shared场景；独立Broker1348、runtime ON516／OFF85和outer763项检查也通过。Attach只接受已由Native安装的唯一wrapper、同cookie／native incarnation／context与原已pin PG image；原调用空隙即使callback计数为0仍阻止unpin，未知结束不可由后续closed抹除。

Native9sim仅改变既有一个sim连续模式guard，其余633文件／CMake／flags逐字相同，整体反向恢复normal；DLL SHA256为`e8109e819c63e105637178fc9e33213ed0d81be46ac38efd0b7f78702f8c5727`，23项helper重跑通过，CPU94因源码逐字相同明确复用。证据保留于ignored `native-body9-build-result.json`、`native-body9sim-build-result.json`及两份source provenance。候选已交武器会话配合其冻结c41a65c subscriber，继承旧PG pin仅为reference，实际测试必须换成该subscriber的真实hash；当前仍未完成真实跨DLL注册／开火／清理验收，没有替换默认组件。

## 2026-10-24 — 上层镜像定位及原生几何接入

AP关闭三个原型，在0.4后台正版实例完成七阶段原版对照。Master#2 child0的镜像位依次为false、true、true、false、false、false、false，对应CA0、3、0、4、0、7、0；左探收回后仍保留上层镜像。Transition#7两条边始终为false／Weight1和true／Weight0。四骨前后14个祖先区间accepted，身份、布局、mask和采样值保持；独立读区间仍不等于同一pose epoch。清理80/80、XR0、音频9/9、原十文件及全部自有进程退出通过，证据为ignored `native-pose-ap-summary1.json`与`cover-host-1024ap-camera-final1.json`。

Native10仅在frozen9上覆盖三个body文件，保留默认关闭的`side_mirror_trial`局部child5 A/B能力。typed BoolProperty和masked CAS与Weight共用租约；原生post可暂见原值，但pending责任保留；零round实际进入node cache且进入fresh mesh epoch后才允许一次固定方向试验，回零／Reset／失效后不能在同cohort二次激活。Journal2682项、MASM319项及OFF分支通过，normal23个helper与94个CTest重跑；sim23项重跑，CPU94明确复用。normal／sim DLL SHA256分别为`1e1af1a824e09d9066ce2b4524ffefd443770b897824c0266a450a739ca1319b`／`4ccc24cd6f286a2f73edc18de63fb81a3efd62cb114dcce1dcf981cf8e0448b3`。这不是已认证的原版左右开关或完整二维控制；实际骨骼效果、最终输出独占及pose并发仍待验证。

Native11在frozen10上接入原生几何读路径。当前Link／Pawn／World／slot及字段布局在一次body mutex内认证，native getter Guard反复检查冻结的Scene Receipt、Context和原始Values；独立存储原生slot位置／旋转，原候选平面和顶沿仍不认证。新的显式`CoverGeometryObserver`默认关闭，只在有效Scene取最多16条成功样本，间隔至少100ms；没有view、body target或抓握写入。几何ON953／OFF2、原生getter helper208项通过，normal与sim两版均实际重跑23个helper和94个CTest，body2682仍过。normal／sim DLL SHA256分别为`d83775bdde24298279946b27b6c93e4701f90d4d653ecce2c768086186d42405`／`b3d72dc8ed274c52e9c2daee30258df175a1d34f34a57f6f50883e8efbceaca0`。此前9／10候选不变；运行时几何、镜像A/B和c41a65c跨DLL检查继续排队，没有替换默认。

补充发现并修正Python诊断字段：`UFunction.FunctionFlags`应读`+0xf4`，旧`+0xfc`实际与函数name index一致。已由保存的ProcessEvent完整chunks及CallFunction的`R9+F4`、Native bit10消费者ASM确认；首次大dispatcher伪码导出超出预算的失败保留，第二次仅机械导出ASM成功。相关Python64项检查通过，新增用例确保name index里的0x400不能授予Native许可。AP历史`function_flags`字段作废并保留原记录，独立NativeIndex／Parms／Return／入口／实际getter值未重写，也未重新执行游戏动作。更正及私有metadata／名称读取helper2在ignored `function-flags-offset-errata1.json`。它不改变hand／supplementary ABI、Native11或既有身体驱动。

最新默认已前移至support3 Native `4752801a…ae14`与menu-modal1 XR `37f46312…a4d`。本分支候选保留原冻结渲染来源，后续正式集成需叠加最新support3被动诊断及菜单修复，不能拿旧候选覆盖默认。LGrip合法表面／碰撞、独立上下左右组合身体求解、Mesh朝墙、受击／枪口一致性与疾跑实际A/B验收仍为未完成范围。

## 2026-10-24 — 冻结完整三维目标与实际几何读回

新增 `CoverBodyTarget`，将同一已提交虚拟偏移、原始 HMD 平移、视点基准和完整 World／Pawn／Mesh／Link／slot／Scene／XR epoch 绑定冻结为一个求解输入。左右、上下和前后独立保留，支持真实站起、下蹲和身体侧探；没有三角形归一化或高度饱和，固定位置的旋转不产生平移目标。源追踪和目标各自超过100ms即失效。当前模块只存储目标眼点，头骨到眼点校准、骨骼求解、同epoch发布和碰撞均明确未通过，未添加骨骼写入。

Native12严格由frozen11叠加五个文件、六个capture hunks及三行CPU CMake，反向恢复逐字相同，其他文件、冻结ABI和渲染flags保持。normal／sim SHA256分别为`40173f6973da047e2ff80dd98b614ef9f7c0acba751e28f820f3b739f8e25598`／`27baf0bf9225171ce0c6fd0124a0a367b66738a7fba3c737cd60a7c6c07e328a`。两版均实际重跑23个DLL／helper及95个focused CTest；新增目标包316项，既有body journal2682、geometry953／OFF2、surface getter208项均通过。证据为ignored `native-body12-build-result.json`、`native-body12sim-build-result.json`与各自冻结source provenance。未替换默认，也未将这些离线结果计为实际身体求解通过。

AR-A／A2的Native11sim实际正版会话均在稳定CT2／CA0／slot0下取得16条原生表面观测：slot位置为(-388.771027,-3114.74219,-58.957016)，旋转为(0,-31860,0)，原候选位置相差0.00602429416游戏单位。它只验证该slot的原生getter与候选组合接近，尚未认证墙面、顶沿、抓取可达性或碰撞。两轮均在任何Advance前被测试脚本拒绝：A误把inactive Transition child1的合法mirror=true／Weight0当成false；A2等待了仅在停止日志输出的target_u字段。失败保留，没有镜像效果或左右姿态通过结论。A2清理独立83项、Native入口、XR0、音频9/9、原十文件和准确自有进程退出全部通过。后续脚本必须将运行时步号对应的请求目标与实际typed Weight／Total／骨骼证据分开。

AQ疾跑夹具在进入SM4前的PE身份预热拒绝，未执行Try／End或持有跑动输入，没有during-SM4样本。完整清理独立80项、XR0、音频9/9、原十文件和自有进程退出通过。为定位实际差异，`cover_bone_query._module`仅增加失败字段、section差异及头部hash诊断，原拒绝谓词保持；相关64项Python检查通过。疾跑相机稳定仍未验收，不能将无样本写成零晃动。

## 2026-10-24 — 显式Scene与stereo锁上下文

Native13新增内部context ledger及真实`StereoMutex`归属记录，所有11处stereo lock／try-lock使用同一类型；保留原底层互斥行为。完整Scene入口以POD helper及finally包住原有body，原六个参数、返回值和异常继续转发。嵌套、复制／错误地址／错误ledger、溢出和非LIFO结束不能被误记为上下文清空；未知状态保留poison。它目前只提供上下文记录，不授予引擎调用或字段写权；普通callback、ActionGate及前置PE／bridge许可还未连接，不能据此称完整动画交接通过。

候选由实际frozen12的640文件最小叠加为643文件，严格9文件变化、10个capture hunks、3行CPU CMake及四个inl的5处guard替换；capture／CMake／inl均反向逐字恢复，保留各母体原换行。normal DLL为`build-cover-body13/GearsCaptureCoverBody13.dll`，SHA256 `82dbf0779730ce48603ab0ebd198a8f56808e7faae687acd7ac7a47d02802693`。23个原helper与96个focused CTest全部重跑通过，其中context102项涵盖实际mutex所有权、try-lock、跨线程隔离、Scene嵌套及真实SEH穿透；body2682、geometry953／OFF2、surface208继续通过。正式capture对象的实际Ninja依赖525项包含两个新头，37个exports和原3个participant导出、冻结ABI及渲染flags均保持。仅normal版，没有13sim或实际游戏／实体头显通过结论，默认与旧9／10／11／12 normal、sim产物均未替换。证据在ignored `native-body13-build-result.json`和`native-body-source13-provenance.json`。

AR-A3继续在任何正权重前被脚本拒绝：它读取了清理时才复制到runtime报告旁的status文件。原生运行时实际写入各隔离实例`native/session-status.txt`，AR4已改为认证instance_root、native_command build-dir、game／owner incarnation及started_qpc后读取该LIVE路径。A3清理独立83项、Native／入口、XR0、音频12/12、原十文件与自有进程退出均通过。AR-A4／A5则在continuous激活前被原有startup窗口门槛拒绝，没有probe或Advance，不记为身体失败或连续83项通过；窗口诊断为一个visible但client0×0的候选及一个hidden候选，tiny-output曾观察1280×720，但不能将它等同实时帧。未切前台、调整窗口或放宽门槛。A4完整prime／entry、reservation13/13、PG、tiny、音频12/12、原十文件与当前准确进程清理已核；A5保留独立后续审查。

## 2026-10-25 — 完整Support3基线合并与测试阻断定位

按实际源清单复核后，明确本分支冻结渲染源的共同母体为Support1（584文件）；source13仍缺Support2的完整启动timeline，不能只叠加Support3最近三个文件。真实Support1→Support3恰7文件。以source13为ours做三方合并，root审核三处capture冲突，保留cover／hand retained、完整stereo summary scope及hand初始化／失败顺序，再加入原Support3 trace。source14为645文件，恰7文件增量，其余source13内容逐字保持；旧默认及9–13候选保持。

Native14 normal DLL SHA256为`438ac963d8ebbf40427ba1a5245594badcad38eab638560c5550494a35a79651`。23个原helper、97个CTest（原96加AtomicStatusFileTest）及独立SupportStartupTraceTest实际通过；context102、body2682、geometry953／OFF2、surface208保持。76项defines、实际capture编译flags、37个exports及冻结hand／call ABI与13保持，实际依赖526项含新trace／context头。证据为ignored `native-body14-build-result.json`、`native-body-source14-provenance.json`及原始／规范换行的三方冲突记录。随后将相同增量合入公开源码，保留公开树已有实验和测试取舍，没有用混有实验的整份源码替换冻结构建；公开同步记录在`public-support3-rebase1-applied.json`。当前没有14sim、实际游戏或头显验收，也没有替换默认启动组件或加入新ActionGate／Boundary provider。

A5独立启动清理已补齐。A6的有界只读窗口采样实际记录主窗口IsIconic=true、showCmd=2、client0×0；只能确认采样时处于最小化，不能确定触发者。该局随后进入continuous，未进行恢复窗口／前台替代；在两次已完成转向之后，脚本直接索引暂缺的rotation样本而停止。A7改为最多3秒等待新鲜合法姿态，保持原轴向租约与八次上限，进入稳定CT2／CA0／slot0后，被祖先只读区间的registry closing检查拒绝。A8仅首次Advance前允许该精确纯读失败重采一次，每次原始结果独立保留，原closing谓词和后续正权重／骨骼阶段不放宽。A6／A7／A8均完成独立83项、Native／入口、XR0、音频12/12、原十文件及准确自有进程清理。

A8通过中立基线、零档请求、typed child0／5 Weight及cache观测，首次骨骼查询在分配／提交前因PE校验拒绝。新诊断指出同一正版EXE的index12 `.themida` 节raw_size=0，PointerToRawData磁盘11354624、运行时0，其余节字段相同。离线以原1536字节头仅改此字段并叠加已复核Resource目录布局，重建hash与A8实际live header hash完全一致。兼容只接受该正版SHA256、精确index／整条节记录和0目标值；其他头字节、节布局、Import、全部函数代码及每次查询的完整原始live header closing复读保持。兼容记录不识别改写者，不修改游戏文件／内存，也不认证映射节的运行时内容。新增6组含多字段变异、未知hash、closing变化和代码变化拒绝检查，相关Python70项通过；新兼容的当前游戏读回仍待执行。依据为[Microsoft PE字段说明](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)，私有证据在`bone-empty-raw-pointer-layout-ar8.json`。正权重侧探A/B、实际抓边、完整三维身体求解、碰撞／受击／枪口一致性、原LT/RT自动探头移除和疾跑稳定验收继续实现，未计为完成。

## 2026-10-25 — 掩体与武器动作边界的兼容接线

新增 Native ActionGate 与外部动作边界 provider，仍复用唯一 CallFunction owner、既有 hand pin 和同一个左手仲裁器。CallFunction 顺序为 Gate begin、V1 begin、原函数、V1 end、Gate end；所有已接纳责任跨越完整原函数并在 POD finally 收尾。恢复引擎调用只允许认证 GT 上、本次入口的显式许可；Scene、stereo、普通回调、嵌套及未知上下文拒绝许可。外部 State 只保存描述符，TLS／责任节点留在 Native 固定池；停止、异常、不确定状态与未完成责任保留模块，worker 不提前卸载。`WeaponActionGate.engine_restore` 默认关闭，没有启用 STREAM／STOW。

Fast Revoke 仅短 POD copy 和原子撤销，旧 Frame commit 不能覆盖新 generation。Action 引起的 PG invalidate 延后到下一合法 Scene Frame prepare 或停止后 owner worker；original 前即时撤 Native 授权，但不增加回调或取得 stereo 锁。Shared Scene 在同帧缓存之前检查 generation／barrier，撤销后只可重用完整相同且仍新鲜的已认证视角，并清除手部授权；不重复提交同序号掩体观测或新身体输出。新鲜 Scene 失败时继续认证已提交虚拟偏移及当前 HMD 平移，未提交拖动不外泄。released／no_candidate／needs_release 的 accepted 提交可以没有手部授权。

Core 独立 MSVC 测试 4,443 项通过（旧 Broker／Runtime 1,949、原 CF outer 763、新 provider 48 进程场景 1,731）；另 35 个生产 Shared consumer 组合场景通过，覆盖同帧撤销、revision 变化、整个外部 scope 阻挡和 prepare／commit 间撤销。证据为 ignored `action-provider-core1/run-20261025-020201-782171/result.json` 和 `shared-action1-20261025-020340-477559/result.json`。这些是 CPU／生命周期证明；完整候选构建、PG ENTRY 互操作和真实游戏动作验收分开记录。

AR-A9 的初始纯读结果中 optional posture 拒绝 closing，但脚本错误索引其 roles，未执行正权重。A10 改正 schema 后，在任何 Advance 前被 guard_cover 拒绝；Native 的16条原生几何观测证实本局曾处 CT2／CA0、同 Pawn／Link／slot0，最后 host 为 SM0。拒绝瞬间脚本未保留完整 sample／cover rejection，因此不能断言未进掩体、closing 错误或特定瞬态。A9／A10 均完整清理；A10 独立83项、Native／入口、XR0、音频12/12、原十文件和准确自有进程退出通过。后续只补失败原始记录，不放松状态守卫。

AQ3 实际通过 PE／VM 预热，三次原查询分别为 CanDoSpecialMove(4,false)=true、ControlScheme=0、IsHoldingRoadieRunButton=false。随后属性准备累计触到既有32,768 source-check上限，未发送 Try、未持有跑步输入、无 SM4 样本。两个相机策略字段暂设并全部恢复，不计为镜头稳定。独立80项清理、Native／入口、XR0、音频12/12、原十文件和准确进程退出通过，证据为 ignored `cover-host-1025aq3/sprint-1.json`、`sm4-aq3-summary1.json`。后续修复须缩小重复元数据认证范围并保留原上限、归属、类型和 closing 检查。

Native16 normal已完成完整冻结与编译：DLL SHA256 `07837b14539ee81c42eb87aafe56819afc7b8a564533032841db86b072584641`，source16 provenance `121066957d119eb58eb7e939517ac551a80a039f279cf1475e840134e64c4872`。实际source14仅13文件增量、650文件，反向逐字节恢复；原76defines、正式Capture flags、Support3被动诊断、原色及独立左眼history保持。23 helpers、151 focused CTest、独立SupportStartupTrace全部通过；41exports为原37加4，provider48场景1731、boundary328、context102、body2682、geometry953/OFF2、surface208通过。证据为ignored `native-body16-build-result.json` 与 `candidate-cover-body16-manifest.json`。失败Native15是capture forward声明未同步，原source15／provenance／build及失败记录完整保留；16另外修正声明，两个新CPU target显式静态CRT。构建记录诚实保留：同一payload先以遗留CoverBody15.dll basename测试，再逐字节复制为CoverBody16.dll；frame-link随机临时目录只记模板，未伪造原命令。尚无16sim、实际PG ENTRY互操作、Game／XR／头显验收或默认替换。

AQ4独立证书修补后，正版游戏完成一次Try及0.703秒held租约、恢复held后conditional End回SM0，取得107个accepted SM4样本和47个不同相机时间戳；相交完整CSV中61帧为固定source1模拟HMD。所有新增证书保持32,768上限及完整归属／closing，fast sampler字节保持；97项CPU通过。两个相机字段保持候选值并恢复2/2；游戏清理独立80项、Native／入口、XR0、音频10/10、原十文件及准确进程退出通过。实际Pawn根位移269.063单位、Z范围-41.350至1.699，不能将地形位移算晃动；相机相对Pawn的三个局部坐标范围约25.058／0.306／62.071单位，旋转相对PC范围17／0／0 RU。尚未有FP开启、无步态残差、最终眼矩阵或A/B证明，不能宣称稳定已修复。证据在ignored `sm4-aq4-summary1.json` 与本局 `sprint-1.json`。

## 2026-10-25 — 跑步稳定视角候选与共用追踪坐标

Native17只在actual frozen16上叠加7文件（654文件）：独立跑步kernel/live及组合测试，加Capture/CMake/first_person glue。SM0校准必须为CT0、未蹲伏／无蹲伏请求、无掩体／镜像过渡；SM4以当前Pawn根位置和已校准眼高构造基础视角，方向来自PC yaw/pitch，HMD仍只在既有下游组合一次。持续新鲜同context的跑动不设任意10秒切回，支持正负90度pitch；失去有效Scene／身份／epoch／时效时本次可选覆盖不可用，不停止stereo。配置默认关闭，仅在第一人称实际preview生效且非原生狙击scope时覆盖，普通Scene在FP关闭时只读预热。Root另以ordinary规则排除cinematic／mounted／unpossessed。该候选尚未解决第三人称跑动，也未完成实际FP开关／SM4衔接及最终眼验收。

Normal DLL SHA256 `ec3e12f62aa3eee2c8226feb4cd743baf70b76d4050b0c34566c8427d0efc11a`，sim `cf8787ee6947cc944d3095ee24c8ab358719b62e6d1fb480ff0442057628a12d`。Normal实际23 helpers、154 CTest及trace/OFF2/surface208通过，新增kernel1524、live195、OFF2；76defines/mainFLAGS/41exports/既有wire及Support3／原颜色／独立左历史保持。17sim仅一处旧sim guard差异，23helpers以正确17sim文件名实际通过，154CPU依赖闭包不含capture而明确复用normal。首次私有builder遗留16sim名的attempt1和重新以正确名运行的证据分别保存。ignored证据为 `native-body17-ready.json`、两份build-result及manifest；未替换默认。

新增共用 `rig_tracking_math.h`，547项严格MSVC检查通过。它以最终pre-HMD base_view、原reference和原tracking pose映射head／左右手到同一游戏世界，使用完整basis与显式单位；不再叠加HMD位移，姿态转换到UE Xforward/Yright/Zup。level cover+已提交virtual offset与原cover_tracking_point一致，也验证了真实stereo_matrices的两眼中心、双手平移、扒墙抵消、rotation-only、rebase和正负90度。它不包含手骨校准、authoritative aim证明或setter许可。已将精确header交武器会话复用，共享原始输入来源及新Rig数据快照接口另行接线，未扩展冻结hand V1。

AR-A11实际初次neutral父CoverBlend缓存完成（node／mesh tag同2400）；后续保存的freshness读值同3121，未满足“旧node／新mesh”。该额外镜像写入条件不适用于没有mirror写入的A control，A12只对A保留completed cache而不请求fresh-cache，B仍拒绝。A12到0.25请求后因baseline变化停止，但拒绝数据未先保存，不能归因某一字段。A13没有启动coordinator，空跑到期；独立83项清理、XR0、audio10、原十文件及准确进程审计通过，原缺review finalizer失败保留。A14将host与coordinator连续启动，零位四骨前的child5 typed interval拒绝，未进入正位移；完整83清理、XR0、audio10及原十文件通过。后续不将这些计为侧姿态通过，同次原生pose相位观测用于替代跨多次远程getter的姿态推断。

AQ6已在正版游戏实际运行Native17sim的第一人称／跑步候选。一次0.703秒原SM4 held租约取得112个accepted样本、54个不同相机时间戳；Native实际751次prepare，697次校准、54次应用、0拒绝，校准眼高60单位，FP应用609次且无sniper scope。相交CSV的62帧均为固定source1模拟HMD；这证明候选确实在实际SM4期间应用，尚不证明最终眼矩阵无步态残差或实体头显舒适度。保留camera_stability_accepted=false、final_rendered_eyes_verified=false，未做A/B或默认替换。独立80项、Native清理、入口还原、XR exit0、音频10/10及原十文件保持通过；停止后无自有Game/XR存活。证据为ignored `sm4-aq6-summary1.json`、`cover-host-1025aq6-camera-final1.json`。final1末尾“camera OFF”是旧模板文字，与实际配置／计数不符，原文件保留，另以`sm4-aq6-acceptance-boundaries1.json`明确本轮边界，不据此扩大验收。

`world_state.Query.finish()`现在保存首个closing拒绝的identity／metadata／registry前后值，registry另保存pointer／count／capacity；原拒绝谓词、短路顺序、读取序列／次数、异常字符串及growth语义不变，`CoverQuery.finish()`未改。20项测试对照旧实现通过。AQ6仅在首次FP toggle前，依据明确记录的registry count增长允许最多两次、三秒内的全新只读认证；第二次必须完整通过原strict gate及完整cohort／allocation核对。其他失败立即拒绝，toggle后不重发或重采；这项夹具处理不赋予生产runtime新权限。

共用Rig接口的provider与Shared consumer已完成16＋36个CPU场景、1,450项检查，MSVC x64 Frame为1824字节、ReadRequest为40字节；冻结hand／call／action V1不变。原匹配GVC3只读一次，额外continuity companion必须与该输入及publisher完全匹配；缺失时producer计数为0，不伪装GHW1证书。新Native reference observer在重置／重新居中／追踪失效的状态改变前原子撤销旧快照，4,888项observer检查及既有stereo测试通过。Root已接入最终pre-HMD基础视角和只读export，正准备Native18组合构建，尚无该组合的真实跟手验证。

新增只读pose阶段观测器：在原Compose／Physics／外层Tick保持原调用与异常转发，事件触发后最多8个episode、每个最多7阶段，记录LocalAtoms／SpaceBases及相关矩阵到ignored二进制。实际编译代码先通过完整hash证书；默认关闭、不写骨骼、不授予独占或setter。core116、真实MASM/ABI/SEH492、live147项检查通过。该观测器与Rig的完整Native18构建和游戏采集仍待验证，不能将静态阶段关系或无观测并发等同安全最终写入点。

Native18b normal完整组合已通过：实际frozen17的654文件加21文件最小闭包为670文件，所有变化逆向逐字节还原；保留原76项渲染defines、actualCaptureFlags、Support3、原颜色及独立左眼history。新test-only fixture cache单列，未改渲染配置。DLL SHA256 `bc588e78c8c7cc4927f6ba4cd3fcd3a8c26ea27f9732cd6dc2007b1a134eced8`，42exports；实际fresh编译、23helpers、176focused CTest、原StereoFrame独立实跑、trace/OFF2/surface208通过。Pose core116／thunks492／live149、Rig720／Shared730、math547、observer4888。最初18的编译及23helpers已经通过，但一项CTest漏传私有fixture，保留175/176负例；18b只改CMake测试参数和测试输出唯一目录，production capture与18完全相同。构建脚本旧source变量与typed-cache参数的前置失败也原样保留，没有放宽测试。证据为ignored `native-body18b-build-result.json`、manifest及source provenance。

18bsim也已READY，DLL SHA256 `cf6bcecd98dcfdb76ba9c9798082af08c490d94e369f1657dff3a1437664c3fe`；仅历史operator guard一处差异，670文件及逆向核对通过，实际fresh编译并以正确basename运行23helpers。176CPU／trace／OFF2／surface检查经实际源码依赖闭包证明后明确复用normal，没有冒称重跑。最终`native-body18b-ready.json` SHA256 `adb69c39faa0481aea775a77423418d21ed53cedc16c30c47b4c4eb79f14cee4`。继承的旧报告文案另有修正回执及原备份，DLL和原日志不变。只读Pose文件parser通过离线检查，首轮收敛为一次事件；等待最新菜单同源XR source1sim后再进行新游戏采集。无18b Game/XR、真实跟手、完整身体或头显验收，未替换默认。

上段首次交接之后，构建agent又删除了报告继承的gait-only／excluded-Pose-Rig矛盾字段，连带更新sim manifest／provenance及READY pins。武器会话据旧pin正确拒绝接收；原`adb69…cee4`字节完整保存在`native-body18b-ready-pre-semantic-metadata-audit.json`。Root重新核对9个关联文件后，用CREATE_NEW另存固定选择`native-body18b-ready-semantic-final1.json`，SHA256 `943fea2b3e0def7899c9ead53c47fc2bcc8a3ead4e11e01c82a49348ac51c304`；最终sim manifest为`77f94b9447f9cb091f491509122b72ea9025be9fb6738c67887c974e6b741c01`。交接回执`native-body18b-handoff-final1.json` SHA256 `f47c9bc04e4dad1d9117d37d6f3253a3bcb35bb7c4b711859628977769855664`。两个DLL与原源码／运行日志保持，未重建或重测；后续采用新命名文件，不继续覆盖交接选择。

配套XR source1sim已交回，基于最新menu-modal1同源normal制作，仅私有sim选项及9个限定hunk；原菜单、颜色、CSV尾部与normal输入采样loop保持。EXE SHA256 `1a059d2d3b6aed6b4f8911ee1790f5051a1908b0d91515f3451655e535a2b4e9`，manifest `c189bcca43d4a41420359fc36acb39100a00cbe0cb3e0174f091948539befa80`。Root独立核验7个EXE／loader／源与CPU回执／外部runtime及operator文件，均匹配；证据为ignored `pose-phase-ro1/xr-receiver-preflight1.json`。实际runtime身份、完整模块路径及本次XR进程的fresh8720监听仍须在拥有的会话启动后核对，文件校验不替代实际运行验收。只读Pose单次采集夹具准备中，此处尚未启动新Game/XR。

## 2026-10-25 — 实际姿态阶段、骨名与原生数据适配

Pose RO1首次实跑在Native worker启动前因私有child的`prime_owner`导入路径错误退出，未发送事件；原失败／incomplete、XR exit1及finalizer正常停止timeout均保留。独立事后核对无准确自有Game/XR进程、音频10/10已恢复、PG／early prime恢复通过，host原十文件比较保持；没有补造完整Native独立清理通过。RO2只修三个具体child导入路径并验证真实CLI闭包，保持所有Native/XR／动作／权限和原只读协议。

RO2正版实跑成功，0.4 scale、behind／observe-only／mute、后台物理输入关闭；仅一次SetEvent，取得1 episode、594792字节，SHA256 `29ca02928e54a7b42711a4e3adbc11cd801b5df2ca9db8603d0a21c3f6b36110`。C0／C1／Plate0／Plate1／L1五阶段有效，main physics两阶段未经过；无exception／nested／duplicate，观测concurrent为0不等于独占。状态receipt为CT0／CA0／SM0、link null／slot -1，且不是与pose原子同epoch的掩体认证。正常停止、Native／entry、独立88项（含三入口完整8个已知code chunks）、XR0、音频10/10、原十文件、PG0／stopped、tiny monitor和准确自有PID清空全部通过。证据在ignored `pose-phase-ro2/runtime-final.json`及`runtime-binary-parse.json`。

实际115骨C1满足行向量`Space=Local(q,t,s)*ParentSpace`、`World=Space*component`，重建最大平移误差3.668e-5游戏单位、旋转元素误差9.664e-7；C0不能当作当前local已组合完成的space。C0→C1只有骨号8／10／11／32／34／35的local改变；C1至L1的四组原始数组在记录端点相同，不能据此称callee不写、GPU已消费或physics碰撞体已更新。后续静态证据已具体收敛到DD91C／真实vtable方法、12E330尾调用和physics接收者；F20F4是矩阵归一化、F2610是48字节转换，不能再当body getter。定向新ASM与原历史blob全chunks核验，原IDB保持；IDA连接插件缺模块时仅用既有本地CLI私有副本，不安装或迁移工具。

新增`cover_skeleton_ik.h`的FK、world pose提取及world→local／后代适配，统一复用武器会话`hand_two_bone_pose.h`，没有第二套长期双骨solver。固定长度不可达明确失败；adaptive完整目标通过显式lower/end local translation调整实现，保留scale及其他local原字节。数据只写调用方私有数组，不赋予游戏内写入许可。严格MSVC synthetic486项、含真实C1 fixture共2040项通过；4单位位移／返回／完整roll和超出原长度的adaptive目标最大位置误差约2.71e-5游戏单位。CMake两个focused test通过，未重跑无关176 suite或重建Native DLL。独立构建使用frozen18b的最小四文件增量、673文件；初始worktree缺ignored MinHook、复制文件只读及日志解码失败均保留，未装依赖或改旧冻结树。证据为ignored `skeleton-ik-cpu1/delivery-shared.json`和`skeleton-ik-cmake2/result.json`。

武器会话新bv只读会话进一步完成同session骨名／Ref raw／本次Pose parent闭环，Root实际核验join SHA256 `00511f9aace8ba9dfa16ba7906edd18179b6af8dd8a1a2ce50df814383df245e`及final SHA256 `fe2b7dad240bce2aec9e63f168cfcea39f244a6ffee42fb3631697ec96203d91`。当前115骨模型的左链为`b_MF_UpperArm_L` 8→`b_MF_Forearm_L` 10→`b_MF_Hand_L` 11，右链32→34→35；113／114是另外的IK helper，不能混用。仅按该asset／Ref签名绑定，不能全模型硬编码。该轮Native／entry／88独立checks／XR0／原十文件／音频10／PG及准确PID清理通过，所有writer关闭。下一步是另做默认关闭、有界且可恢复的Native C1姿态写入试验；当前仍没有真实自由双手、完整掩体身体／碰撞或最终发布验收。

有限姿态写入纯IO草案尚未接入Native或进行Game/XR写入。武器会话独立审计冻结草案`c2ecc4c3…9e`，以合法原生Compose修改Local的新增回归确认旧C0==C1前提会拒绝实测行为，原失败保留。另发现C0逐字段归还失败可能留下混合姿态、C1／L1失败阶段不能正常收尾、Space责任提前清除、非空desired数组未纳入别名检查。修订中将原生C0输入、本轮原生C1输出、本次overlay值和后续归还条件分别保留；C1原始输出是即时回滚的精确目标，新动画输入不得由旧姿态覆盖。任何不确定或部分写入不能当作消费者可以继续运行；实际Native入口、并发消费者顺序与故障出口仍待实现／验证。审计与回归在武器worktree ignored `analysis/hand-c1-overlay-review-20261025bx/`，此处不把host测试或caller proof位提升为实机写入许可。两会话共用solver，grip到wrist完整位置／旋转标定由武器会话独立小模块推进，未修改另一worktree。

共用`hand_wrist_calibration.h`已按字节接收，SHA256 `d73d075c7b854c14addb5fa919292d6301f4116dff0b864987009b32ccdad224`，与武器会话原件及`pose_math.h`依赖完全匹配。接口需要显式已对齐的grip／wrist pair，保存grip局部米制位置和完整四元数偏移；默认unknown拒绝、失败不改输出，任意原版动画姿态不能自动当成物理标定。Root以实际本地header严格CL编译并通过10组CPU检查，包括非交换旋转、完整roll、单位只换算一次、刚体协变及溢出拒绝；证据为ignored `wrist-calibration-cpu1/result.json`。加入未来组合构建的focused CMake target，本轮没有重建Native DLL或重跑176 suite；实际资产／左右手绑定、标定来源、存储交互与跟手验收仍待实现。

有限overlay C3的纯IO修订已冻结为header SHA256 `2bce0f10b7fa8cb0517ecbe821236cc6e5c9c1019b001b8fb39800ecda87941b`，strict CL synthetic7031 checks／39 scenarios、含真实RO2 C0／C1配对的8455 checks／43 scenarios通过。真实六骨变化保留，即时失败精确回本轮C1原像，新动画输入不由旧姿态覆盖；有债务的失败明确fatal／保留责任，Space不再凭近似FK归零，desired与metadata/control别名检查补齐。Root实际重核源码与新回执hash，并交武器会话独立窄审；回执为ignored `overlay-trial-cpu1/delivery-native-c1-c3.json`（SHA256 `64d76d6efddc705a661c5df55e522f67df1831a39a5a5b2be05ada8aa12e0189`）。没有Native接线、游戏内写入或真实读者同步许可，这个草案不作为实机READY。

定向ASM审查进一步确认Compose按有效AnimTree `mesh+230`、分轮标签和control链选择输出；`master=0`／desired全覆盖不足以保证全部Space本轮重写。Root据Astra建议优先定位原生手臂SkelControl实际目标输入，让原Compose自行产出Local／Space并沿既有physics／render传播；C1直接数组overlay保留私有验证。最小只读采集沿用已有18bsim／source1sim／0.4后台协议，新增受控mesh实际AnimTree、逐骨control map、链头和真实虚目标inventory，随后才针对实际计算实现确认effector／弯曲方向／目标空间／完整旋转来源。未修改strength／LOD来强制启用，未增加setter、重复Compose或PG前向写权限。当前仍处于采集准备，不把inventory等同控制器已执行或碰撞体已更新。

RO3在event前因实际AnimTree class不可用拒绝。Root复算已保存指令`1289B1: 48 8B 0D 98 8E AF 01`，确认`next RIP 1289B8 + signed disp 1AF8E98 = runtime RVA 1C21850`；IDA显示的数据符号`1C20850`是raw-loader注释，不能直接当运行时RVA。原失败和无event保留，Native／entry及88独立项（8个完整chunks）、XR0、音频10、原十文件、PG0及准确PID清理全部通过。RO4使用新实例，只对当前未hook的7B指令逐字认证后解码signed RIP，不在活进程修改采集器或重试。

RO4前置inventory独立accepted，取得受控mesh的12个control节点；骨号11、35分别映射到Limb→SingleBone→Limb→SingleBone链，原strength均为1／1／0／0。这里的侧别仍需本次ControlName／Ref FName归属认证，不能凭跨进程相同骨号补名。实际Limb虚槽230／238为RVA `3A4908`／`1D2A04`，SingleBone为`19E2FC`／`1D396C`，root虚270为`F7940`。标签count115、取值0／1，原始标签与parents保留；inventory的逐骨分轮计算显式假设全部骨在desired，不能当本次真实Compose覆盖证明。前置文件SHA256 `bc93485fca5ae6236dccdb173f18e24b15605fa2f653affd31d2e805e4de5bf1`。

RO4唯一event已发；后置严格CoverQuery因值在采样中变化而拒绝，整轮`accepted=false`保留，没有把前置独立窗口拼成原子Pose或后置通过。完整清理88项／8chunks、XR0、音频10、原十文件、PG0／tiny和准确自有PID退出通过，事后Game／XR／8720空；final SHA256 `d556c81c8732af7b4c6b7382806e1cb390930e6e01ca85583905c3b13dd6349e`。Root已检索主知识库并只在新私有IDB导出这5个实际函数ASM，全部chunks匹配历史blob且前64B匹配本次实机读取；完整函数当前运行时字节仍未认证。原IDB hash保持，CLI exit0／2.156秒；衍生证据留ignored `ida-body1/native-controls1/`，不进发布包。下一步审实际目标字段、空间转换、完整旋转及长度限制；尚未进行control目标写入或自由跟手验收。

武器会话独立C3复审仍指出两项P1：Local按整个16B／12B字段仅因部分字节不同就判native-replaced，会丢失仍含overlay分量的责任；Space各分量不同也不能证明来自完整原生输入重新生成，1ULP变化不等于恢复。这些不是host已有checks覆盖充分的证明。C3保持未接线、未启用和私有回归用途，原冻结源码不再变动，不以再开C4替代当前原生control主线。复审为武器worktree ignored `analysis/hand-c1-overlay-review-20261025bz/astra-review-c3.txt`，SHA256 `775c53f4c4bedbf92617442b7f99675ed53ef30eb1170a1b4c89551724828d8c`；后置动态读取拒绝的具体字段也仍未定位，不隐去失败或认定它只是无关变化。

实际原生Compute合同现已收敛到临时输出：Limb v238给3个component-space Matrix64，按upper／elbow／wrist排列，Single给1个wrist；原Compose负责从这些矩阵生成Local／Space且不再次限长。`cover_control_output_pose.h`复用exact shared solver，默认preserve、不可达拒绝，显式adaptive可达完整目标；Single直接替换3×3完整旋转并逐字保留translation／affine bits。仅接受proper unit-scale。strict CL synthetic417 checks／24 cases、含RO2 C1编码样本475 checks／27 cases通过；C1样本不是v238实际输出采集。header SHA256 `67109c6d79698a6759e156c5747c66bfe6254f4a760214ccd81f55788ca45b36`，回执`control-output-pose-cpu1/delivery-unit-scale-v1.json` SHA256 `553dff2cc86f46bada9635533b3412ee5bbc3ce768a8213166c24ddb5edcfd76`。尚无运行时写入或真实跟手验收。

v278权重和v248额外scale明确是两项调用：分别return12911F和129474，v240还可能返回per-bone scale数组。首个输出候选必须分别核对真实alpha、g、scale数组和Single的ApplyTranslation；矩阵轴长不能证明LocalScale保持。Root已准备默认关闭的三入口被动观测：Limb／Single原Compute与原weight getter，依附已有Pose event，记录真实栈header、临时矩阵、节点参数、vtable目标及原weight返回；v248只读目标，不额外调用getter。5个完整历史chunks共5891B证书已由原blob及机械导出生成，运行时仍须在hook前认证；getter确为6B `vmovss xmm0,[rcx+70]; ret`，本轮该6B也与RO4读取吻合。观测接线／测试正在实现，没有将WIP编成新候选或覆盖旧默认。

Native内部Rig数据源与public registration耦合另行拆分。已按字节接收武器会话的独立`CoreSnapshot`1832B／`NativeSessionKey`24B，不含public Frame或假cookie；public V1仍1824B／ReadRequest40B且真实cookie检查保持。Root第一次receiver因本地`stereo_frame.h`依赖差异在复制前拒绝，未复用不相同依赖的CPU结论；第二次用实际本地headers和冻结旧V1严格CL，8组204581 checks通过，包括原行为差分和有效payload全bytes投影。Source／实际include和前后hash确认，证据为ignored `native-rig-core-receiver2/actual-local-result.json`。这是纯数据层交接，Scene builder／Native-session cache／getter／outerTick一次latch仍未接入；不重读bridge、不刷新源时间、不把Scene改名为Tick，也不赋骨骼写入许可。后续Native19只读观测先从实际frozen18b最小增量构建，维持其原V1 header与全部76旧defines；Core live迁移另作明确增量。

Native19只读control观测现已完成最小源闭包构建：从实际frozen18b的670文件派生为682文件、15项增量，保留76个原defines和原capture flags，仅增加observer capability。normal DLL SHA256 `4f601096265f02af844eec11ee6b07c17ee360657a4effe8a10276465f10aeb4`，23个实际DLL helper与183个focused CTest通过；sim DLL SHA256 `8b7cc0eb276de778864e5988208c070670e9a6cdfd148bef201afe5c70b7f8fc`只保留既有私有simulator兼容谓词增量，23个实际DLL helper通过，183项CPU结果按真实依赖闭包复用并明确未重跑。来源和CREATE_NEW选择固定在ignored `native-body19-ready-final1.json`，旧18b与19来源、二进制、结果均保持不变。这些结果不覆盖真实MinHook安装、控制器实机采样或跟手写入。

`control-output-ro1`使用19sim／source1sim／0.4后台隔离协议，在派发唯一event之前被control observer的configure/reserve共用保护拒绝，未产生CCTRL或CPOSE。该错误文字尚不能区分完整代码hash、入口prefix或实际MinHook失败；getter使用VEX是待验证假设。整轮accepted=false，原失败与finalizer正常退出超时保留。session为incomplete，Native cleanup=false、entry恢复未验证、XR exit1，不能称完整清理通过；PG恢复通过、音频10项恢复、原十个设置／存档hash未变，00:40 UTC复查准确自有PID及Game／XR进程为空。补充回执为ignored `control-output-ro1/runtime-failure-audit1.json`。接下来先补足拒绝分段证据与真实本地MinHook试验，不能绕过代码匹配保护或将缺失样本补成通过；最新默认未替换。

Native20仅从实际frozen19派生三文件拒绝日志增量，Row1944／Header72与所有动作保持。normal SHA256 `b7c2ac308f825d711166f001af0295b41227eaf5b17920e1f14c943185f080c0`，23个实际DLL helper和183个focused CTest通过；sim SHA256 `98fa60907c636fd5e43a7725e1fb8e39f0fabe632536e2b2f473585e157deb55`，23个实际DLL helper通过、183项按相同依赖闭包复用。stage／RVA／MH_STATUS区分代码hash、入口读取／匹配、MinHook创建与创建后入口变化；strictCL实际thunk528／live690／两个OFF组合各11通过，源和新回执另存，原19与旧失败不改。

`control-output-ro2`实际0.4后台启动已定位同一预激活拒绝：`minhook_create / RVA515BB0 / status8 / certified1 / reserved_mask3 / calls0`。这表明五个完整known chunks已在Native先后两次匹配，两个Compute成功预留，随后exact VEX getter被MinHook以MH_ERROR_UNSUPPORTED_FUNCTION拒绝；代码证书仍不是独立还原证明。隔离自有VirtualAlloc试验直接链接19和19sim各自实际库，两次各172 checks通过：原VEX getter可CPU执行，HDE却len1／F_ERROR_OPCODE；经典SSE getter的真实trampoline／detour／禁用还原完整通过，两个Compute只Create／Remove且不执行。冻结报告为ignored `minhook-control-cpu1/delivery.json`（SHA256 `628b386f120c4e962a2c1d26324185de8ce6ab17a445e6dd2cc98bf690a79144`）。仅在取得20的实际错误后才确定它是本轮激活阻塞点，未反推其他历史故障。

RO2未发event、无CCTRL／CPOSE，accepted=false。正常stop尝试及finalizer超时保留，Native cleanup=false、entry未验证、XR exit1；PG恢复、音频10项与原十文件通过，00:50 UTC后准确自有PID及Game／XR进程空，补充回执为ignored `control-output-ro2/runtime-failure-audit1.json`，不能将进程退出改称完整Native清理通过。当前推进仅首指令exact六字节getter的最小decoder兼容候选，须实际验证trampoline调用、相似编码拒绝和原像还原后另建21，不换SSE或广泛开启VEX、不改vendor／旧冻结源、不绕过代码匹配、不替换默认。

Native21从frozen20派生为684文件，只改CMake、增加build-local trampoline生成器和真实MinHook测试，不改vendor原件、capture逻辑、控制参数或目标数组。生成器要求原trampoline完整SHA256 `824ed887ec9da606960962af8973205d1f97cd03241e75517eba485280fb6377`、唯一原decoder guard及修复后SHA256 `825edb0f2e75fc4a72d2f1f374040aa672badfd997aed27ded78ae7b86bdd4b3`，仅observer ON时接受首条exact `C5 FA 10 41 70 C3`。错误来源明确拒绝，其他解码错误仍保留；normal／sim／OFF四个MinHook C对象实际defines和flags与20逐项相同。隔离fixed实际242 checks、baseline206通过，正例真实trampoline／原返回／禁用原像恢复，四个相似VEX反例仍拒绝。OFF的实际单独CMake库＋测试再次baseline206通过、未生成修复源。仅Root第一版21构建脚本错误残留source20，Ninja以新测试target不存在拒绝，未接受DLL／manifest；失败目录和脚本保留，正确构建使用`build-cover-body21a`。

正确21 normal DLL SHA256 `0a3db8c0a48bceaf8b3dea6f1bac160099438e4d90acfb191ac714a70083ac27`，23个实际DLL helper与184个focused CTest通过；sim SHA256 `841f88cba1b4dde010ffb3bbb2e4b0facc543edbec5f1830425f6ad457153f8e`，23个实际DLL helper通过、184项CPU按实际源／Ninja依赖闭包复用。原76+observer defines、42导出、原色、独立左眼history和冻结支持／菜单来源保持；选择固定为ignored `native-body21-ready-final1.json`，不改旧19／20和正常默认。

`control-output-ro3`首次成功完成新通路实机观测：购买版、source1sim、0.4、behind／tiny只观察／测试静音，Body／Input／Camera／Run／FP／Rig／Weapon／engine_restore全部OFF。场景为普通CT0／CA0，非掩体附着证明。唯一event取得同一episode的8条CCTRL（15624B，SHA256 `597b2d01a9abe82aaff898b67d505ad770a4a16150cdfb018c9bbffaa923f5b7`）和CPOSE（594792B，SHA256 `103e331a0c1c79e2c6eeddb40e57a38fc279027851c5d6db6d0bf80f3ebedfc0`）。control reserve mask7／MH0、rows8、rejected／nested／protocol／failed均0；Native／entry完整恢复，96项独立检查全匹配（control8＋原Pose8包含其中）、XR0、音频10恢复、原十文件hash未变、PG／tiny／准确自有PID退出全通过。final accepted仅指既定只读采样与清理合同，非writer或完整玩法接受；证据为ignored `control-output-ro3/runtime-final.json`。

武器会话冻结v2 decoder按真实MSVC Row1944／Header72实际解出8行，4条Compute与4条真实weight完整对应。首次join因Root finalizer直接parse函数的报告缺layout_sha256被正确拒绝，原失败保留；随后以同一个冻结Pose parser的原CLI重解同一binary生成新报告，核心字段与旧报告逐项相同，原输入hash不变，真实layout SHA由原CLI写出。新join qualified_compute_rows4／global_issues为空，记录在`offline-receiver2.json`和`runtime-control-pose-relation.json`。两条当次骨链8／10／11与32／34／35各产出Limb三矩阵和Single一矩阵，真实alpha均1，Single+C8均4／ApplyTranslation=false；+AC字段前后1、v240目标C3F20为RET imm0空函数（不是数值0返回）、v248目标516100为+AC float getter。scaleHeader和未来g返回仍未采样，端点相等不证明未来值、reader fence、最终渲染或physics；CPOSE的Pmain0／Pmain1仍缺失。

Root再以这次真正v238临时矩阵和同Row component-to-world生成两个264B私有数学夹具，调用实际21构建的`CoverControlOutputPoseTest.exe`，两次各475 checks／27 cases通过，最大preserve／adaptive位置误差分别2.081e-6／3.201e-6原单位。测试程序stdout继承旧C1样本标签，原日志保留并在回执明确真实输入是本次v238，未冒称物理grip输入。证据为ignored `control-output-v238-math-cpu1/attempt-1792890843986105300/receipt.json`。数学合同现在覆盖真实输出编码，仍没有Native手臂／身体写入或碰撞验收。

后续提交点的定向审计建议采用原v278返回后、12911F返回前，此时已取得实际alpha且v240已完成，仍可更新未消费的原scratch输出。关键ABI：weight入口R9是v240遗留scaleHeader=原RSP0+70，不能当pose out；必须取与关联Compute相同的原RSP0+80，affected=+90，hasScale=+48。现有weight行的三个header／矩阵是未采初始化值，不能据此开始writer；需要先在该阶段补齐真实读取／复读、scale与未来g条件、当前目标／code／owner／cohort／age与故障处理。原v238路线只读结果和这个更窄提交阶段不授writer／物理fence权限；审计保存在ignored `control-output-ro3/native-control-output-post-weight-audit1.md`。独立Native Rig core的Scene发布／store／native getter候选由武器会话在其私有analysis继续推进，两边现有live源码无交叉改写。

Native Rig provider v2按武器会话冻结交付接收，37项实际本地依赖全部吻合，仅约定6项源码／fixture变化；Root另以真实本地headers严格CL重编跑完legacy16、Core26及真实Broker guard注册窗口，共43次执行，来源前后hash稳定。内部缓存不依赖public注册cookie，公开V1仍以真正已发布的registration_current校验，不以hand授权代替注册；新增非copy TickLatch保留原Scene，四node用同一key／快照，失效后sticky、错end不清原key。接收回执为ignored `native-rig-live-receiver1/attempt-1792891869053074900/receipt.json`。共享provider独立提交`d051cfe`，包含全局Core头接入，便于后续与武器会话合并；没有修改另一worktree。

Native22从21最小派生691文件／16项变化，保留21的77个defines与实际capture flags，只增加Core event observer capability。Source／default／vendor旧件保持；normal DLL SHA256 `c9c9341085299b3d255a8235935caec0ee32d8c1d41ea33b7cf2f4f55d6d9837`，23个实际DLL helper与239个focused CTest通过，含旧公开V1的204581差分检查、Core26／Broker注册guard及新observer的24个ON模式／3个OFF组合。首次收尾脚本把JSON ABI误当普通文本，正确拦住报告；原失败保留，第二份finish脚本读取真实verified-build及已成功的23／239回执，验证JSON Header112／Record1920／Snapshot1832后只执行未跑的剩余检查，没有重编或冒称重复测试。sim DLL SHA256 `05c7e44d9d46971bc3b855d4e82fee89add23e1d7d18ec44140cea3b1f6663b7`，23个实际DLL helper通过，239项按实际源／Ninja依赖闭包复用。42导出和公开V1 wire布局保持。

新增`cover_native_rig_observer_live.inl`默认关闭，只在第一个已认证event outerTick持有真实全局Scope的world admission，独立TLS Frame保存非copy latch，四个Compute回调在CCTRL锁外读取同一latch。原tick finally在Pose finish／CCTRL结束后、release前执行exact end；stale／异常／drain仍可释放，错end／未知责任保留pending。新增pending进入stop、native cleanup、资源retained和entry-restored检查。局部strict MSVC182 checks为Rig／Phase／CCTRL／World stub，独立Astra窄审没有可落实P1／P2；不把它们当真实连续Owner或执行权。真实Native生命周期由下述试验另证。

`native-core-ro1`使用0.4／behind／tiny只观察／测试静音，Rig和WeaponHandParticipant仅数据runtime启用，没有注册、cookie为0；Body／Input／Camera／Run／FP／engine_restore仍OFF，source1sim仍head-only。真实一次event得到Core sidecar7792B（SHA256 `24ab74c4fac560ace18e16e92b0b81c7259ae469ce38d3b4c496918d319c662c`），1次Core getter、1次latch、4次read成功／0拒绝、4记录／1 exact end／0pending／0failed；Rig发布336、拒绝20，启动／场景拒绝未进一步归因，不能写成整段零拒绝。Scene264与outerTick27090／Compose27091保持分开。C++检查器直接使用22实际Core／observer类型，四份1832B快照逐字相同，publication262／sourceTicket263／refRevision6／published_ns不刷新，world_pose_mask=1；纯payload在ComputeEnd时刻有效，不等当前liveness或写权限。另从实际文件构造8种私有损坏副本，截断／尾字节／writer／count／session／ordinal／无效payload／有效但不同ticket都按预期拒绝，原文件不改。

同轮CCTRL8行、4组Compute／weight关联及CPOSE解析通过；Core的4条记录与CCTRL的线程、session、Scene／QPC／generation、outerTick、Compose、ordinal、node、kind、end bone和前后QPC逐项同。联合回执为ignored `native-core-ro1/runtime-core-control-join1.json`，检查器／快照回执为`runtime-core-parse1.json`。Native／entry完整清理、96个独立范围检查、XR0、音频10恢复、原十文件hash、PG／tiny和准确自有PID退出全部通过。CPOSE依然没有Pmain0／Pmain1，这一普通CT0场景不证明掩体、手追踪、后续Compose成功、physics／render fence或连续跟手。首次只读真实调用通路已打通；独立连续Owner、已知模型／完整手腕目标、实际临时输出写入及掩体身体动作继续实现。

21的ready-final1误继承logging_delta_only=true已由独立源码审计发现。旧frozen、pins和RO3原报告不改，新增`native-body21-ready-clarified1.json`（SHA256 `96e957bd7c760793c5c01529b40d802d7ef25e54bd7bdc777e830056f5417c62`）明确21包含exact getter兼容修复／测试，所选8个实际产物逐字相同；无需因元数据描述重编DLL。22的选择没有沿用这一标签，默认清单没有替换。

共享hand target包已接收：`hand_authored_mount_profile.h` SHA256 `63bdc29eae4ad9cd49c3ae9a17242e680a9acaa9785a1dd9aaefe32f44f11ec7`、`hand_core_target_adapter.h` SHA256 `5bd88f2c983e7c17d592eff9b8aacccc6de445ca8252ec0672ae883c002ca477`和Single数学最小修复`cover_control_output_pose.h` SHA256 `2bc3f16d80a64f821507869750f9fdfe5bc4e1bd29727dfd63507c76141620f8`。实际本地17依赖逐项匹配，仅上述3生产项／2测试变化，旧math与22冻结源保持；Root严格CL重编profile699、adapter7组603、旧math417检查均通过，实际include与前后hash固定在ignored `hand-core-adapter-receiver1/attempt-1792896688960933200/receipt.json`。Host Core／binding和实际RO3矩阵的混合fixture不等同实际tracked手或执行权。

默认`marcus115-openxr-grip-authored-v1`明确为designer_authored：静态骨架的四指根几何定义掌心marker与握持轴，不以任意动画腕和实际grip自动校准。选择依据为115骨index／parent／qT28构成4140B canonical SHA256 `3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193`，不是骨数或跨进程FName整数；RO3和当前core1五个有效参考阶段均与之逐项匹配，其他模板仍拒绝。每手一次prepare生成完整腕目标，Limb和Single复用；单位按model scale和Core.units换算。当前owner／asset／source／stop及输出寿命与执行许可继续由Native端认证，物理掌面、手型、模型外观仍待验证。

实际Single row2／6的m15为0.9999998807907104／1.0000001192092896，旧严格affine验证会拒绝合法实机输入。修复只在Single的验证副本允许finite abs(m15-1)<=2*float epsilon后置1再走原rigid检查，实际输出七个非3x3字段（含m15）逐bits保留；不是把真实矩阵normalize，也没有放宽Limb／component-to-world、shear、reflection或scale条件。原失败397和真实Single正例／超过阈值等负例由交付包保留，两个原生Single现在真实通过full-rotation规划。尚未进行live姿态写入。

`native-core-ro2`在同一Native22sim和冻结XR source1sim上，仅让私有XR factory增加`--controller-input`，没有启用旧Native controller observer或任何writer。0.4／behind／tiny只观察／静音的正版实跑取得双手grip_flags15／active1、world_pose_mask7；同一outerTick24515／Compose24516的四个Core副本逐bytes相同，publication237／sourceTicket238／refRevision6，保持原Scene239及时间而未重标。只调用1次getter，4次cached read均成功，exact end后pending0；没有public注册，cookie0。实际7792B文件SHA256 `34ad0b06e6135544310cd17f7613a64d70a87235219a8a362b795ef210366cf6`。typed parse及与同轮四组CCTRL的node／ordinal／Scene／QPC关联通过，保存于ignored `native-core-ro2/runtime-core-parse1.json`、`grip-flags-inspection1.json`和`runtime-core-control-join1.json`。完整Native／entry96项独立恢复、XR0、音频10恢复、原十文件、PG／tiny及自有PID退出通过。此轮是默认模拟controller pose数据接通，不是翻腕、物理掌面、连续跟手或写骨骼验收。

新增默认OFF的`hand_goal_observer_live.inl`在同一次event的四个原Compute之后，只读规划双手完整腕目标及Limb／Single输出；不写Game／node／Local／Space／input，不追加Core getter或bridge read。C0仅提供owned Ref／metadata，live asset身份、Ref header及内容前后复核，已知Marcus115 signature才选authored profile；每手prepare一次，Limb与Single复用同腕目标／同Core。Single只规划旋转，保留原七个affine字段，不能据此声称平移已经到达目标。子agent实际519项CPU检查通过；Astra对Root接线的窄审无当前只读范围P1／P2，明确冻结快照收尾检查不代表返回时source仍有效、`planned`不授writer许可。Root另建typed二进制解码／数学重放工具，实际受控fixture正例和10个损坏副本拒绝通过，原生Game数据仍待Native23组合构建及采集。所有fixture与派生证据留在ignored analysis，未替换默认。

Native23第一次组合编译发现Windows RPC的`small`宏污染shared solver的局部变量；原23失败源码／构建保持，只在capture的adapter include周围push／undef／pop该宏，未改shared算法。Native23b以697文件、4既有项／6新文件从22的实际冻结源派生，并保留逆patch；78旧defines及实际渲染flags保持，只增默认OFF的HandGoal capability。Normal DLL SHA256 `0131375505ac8a881151de31aa2fc89bed1a204315175e7318b96c89a238e212`，269项focused CTest及profile699／adapter603、原Stereo／trace／surface／编码检查通过；sim DLL SHA256 `795a432fa60691189703db4fe3b9bfe57d253aca6fc0add812f81f387ef5c7a2`，仅旧simulator guard差异，实际编译与预检通过，CPU按实际源／Ninja依赖及flags审计复用。42exports、Support3、原颜色和独立左眼history保持。两次报告／路径规范化断言的失败原样保存，finish只继续未完成的验证，没有为元数据错误重编DLL或重复预检。

预检计数口径更正：本构建系列继承的23项preflight实际上包含15项加载所选DLL的CaptureSelfTest和8项独立component执行；历史“23 DLL helpers”的标签不准确，保留旧frozen报告并以本说明限定，不能按23次DLL加载理解。Root的hand parser另对actual frozen23b headers严格CL编译，正例与10个损坏副本拒绝通过，选择保存在ignored `hand-goal-parser-cpu1/frozen23b-parser-selection1.json`。这些解码／数学结果尚不等同Native实际规划、连续Owner、writer、physics或头显接受；下一步按`hand-goal-ro1`实际采集。

`hand-goal-ro1`已以冻结Native23bsim完成0.4／behind／observe-only／mute正版实跑，保持Body／Input／Camera／Run／FP／engine_restore OFF，仅数据Rig／Weapon／Core／HandGoal ON。唯一event取得真实10208B `hand-target-plans.bin`，SHA256 `b418f1b7f2b7f97623a0412beddc6eb91160377fc08e5ad48d8375db35767166`；每手prepare1、planned4、reject0／failed0／pending0。Core getter1、cached read4／reject0，outerTick28017／Scene272／GT21456、Native session1588117652527。typed parser对四个实际完整腕目标及规划矩阵重放逐bits一致；原始矩阵、component-to-world、1832B Core副本逐bytes与同轮CCTRL／Core文件关联，四组Pose／control关系通过。Limb腕位置数值误差3.174e-6／1.763e-6原单位；Single仅旋转，原平移仍保留，绝不将它的目标距离误写为已达到位置。完整Native／entry96项恢复、XR0、音频10、原十文件hash、PG／tiny及准确自有PID退出通过，窗口已释放。证据为ignored `hand-goal-ro1/runtime-final.json`、`runtime-hand-plan-parse1.json`、`runtime-hand-core-control-join1.json`。

该轮明确选adaptive只读数学政策，不能计正常臂长跟手通过。武器会话对前一Core2实际数据独立重放发现：原左右总臂长各约46.995单位，肩到腕目标约144.455／126.489单位，preserve均unreachable且失败输出不变。当前FP OFF的测试将完整手姿映射到第三人称pre-HMD相机基准；这是真实空间偏差线索，不是solver故障证明。源码还表明旧FP preview有前移0.35m／上移0.10m，故仅打开FP亦不能预设身体对齐。接下来先以实际world_head／参考头骨／肩与视点基准核对并修正手与身体的坐标关系，再做正常臂长和实际写入验证；不得靠三倍伸臂或任意静态姿态自动校准掩盖偏差。持续只读owner正在独立私有candidate实现，尚未合入当前代码，也没有原生手骨writer或碰撞验收。

`hand-goal-fp-ro1`首次FP对照在ready前被INI配置保护拒绝：私有manifest仍为first_person_preview=false，GUI选择器因此未启用Native能力；没有发送FP toggle或pose event，原失败及缺event回执导致的finalizer解析错误保留，独立96项／XR0／原十文件及准确PID清理通过。`hand-goal-fp-ro2`只补私有manifest的已存在FP能力并离线执行实际gameplay_controls函数确认选择为true，原Native／XR／runtime及固定追踪姿态不改；通过现有PID FirstPersonToggle event一次切入实际preview，再发送唯一pose event。Native最终enabled1／requested1／transitions1／applied246／rejected0，完整Core metadata另证实际view_policy=1。原普通清单／旧preview算法及用户配置保持。

FP2真实数据的4组Core／CCTRL／CPOSE关联通过，但HandGoal只计划左Limb／Single两项：右Limb原始末矩阵第三basis长度0.999979923355763，偏离1约2.0076644e-5，超过原2e-5输入门槛，故invalid_matrix并sticky拒绝最后Single；原矩阵精确affine、有限、CTW严格通过。这次计划接受明确false，完整Native／entry96／XR0／音频10／原十文件／PG／tiny／准确自有PID退出仍通过。武器会话以同三原件独立参数化重放（receipt SHA256 `b60a09e70b2579f87034b801a9574b230aaf837707d4649022431ed46c3fa63e`）确认FP2视点距本局C1骨55约0.26913m，而FP OFF为约1.74273m；实际左右肩到controller派生腕目标仍为61.900／85.687单位，原臂长46.995单位。左preserve确为unreachable；右只可报告有限原始row-product的几何距离及invalid_matrix，不能冒称已接纳刚性姿态。打开旧FP不能直接完成身体空间对齐。

独立Astra矩阵审计给出私有显式opt-in输入政策候选：仅original Limb可用Gram Frobenius总预算1e-4，CTW／模型scale／编码输出与Single尾部保留原规则；默认仍strict。61项针对检查与原417项／24例通过。正例目标是原始Native末端，绝非实际controller目标；此候选不会解决上述实际reach偏差。预算是工程近似，微小真实scale／shear与漂移不可仅靠矩阵区分，现阶段仍缺control LocalScale作用域证据，未合入shared header／adapter或writer。

连续只读owner candidate1在独立548文件私有树编译并通过1111项CPU检查、40 Tick／160 Compute，已经脱离event／C0／诊断容量；不能把它当从23b697实际冻结源派生的runtime产物直接运行。Root独审无只读P1，发现child Compose／Control相同depth错Entry地址仍消耗计数的P2，已要求candidate2以exact地址保留责任并补两条回归；完整caller字节仍留ignored证据，candidate2改为双读完整SHA256认证与小callsite校验，避免将专有完整chunks写进tracked源码。待该小修及独审完成，再从实际23b最小迁移与采集；当前没有连续实机、原生骨骼writer、受击／枪口／碰撞或头显验收。

Root按与武器会话共同确认的单一接口接收矩阵政策候选：`LimbInput`与`LimbData`末尾新增OriginalMatrixPolicy，默认strict_unit；adapter每次显式复制data政策到work.limb，坏enum走实际solve的invalid_policy，CTW、model-scale、Single尾部和编码输出仍用旧规则。Root实际本地严格CL验证adapter8组626、原有math417／24例、近刚性专项61通过；包括bounded→strict重复Workspace、失败output不变、坏enum、CTW保持严格和近似政策会接纳微小真实scale的边界。只改共享math／adapter／其测试3项，旧CF／Native23b／已冻结Native24的2bc3源与二进制保持，不把新政策偷带入连续runtime构建。未开放live opt-in或任何writer，实际controller目标仍不可达。来源及实际include回执保存在ignored `shared-original-matrix-policy1/attempt-1792902299784375500/receipt.json`。

连续只读owner的candidate2已修正Compose／Control相同depth错误Entry地址消耗责任的问题，两条exact-address回归通过。调用认证的完整798／26B历史代码仍在ignored fixture；tracked certificate只保留RVA、长度、SHA256和16B callsite，不包含完整游戏代码。Native24首次组合构建的cap-OFF helper因引用cap-ON声明失败，原24保留；24b在新接线周围添加真实编译guard，关闭能力仍走原路径，没有no-op替身。24b从实际23b最小派生702文件（6既有／5新增），80个defines保留旧渲染政策，284项focused CTest、15项实际DLL helper及8项component预检通过。Normal DLL SHA256 `b6d4fff5b7ec56ed64882097bf808ea0ca89a949dd911b8ed1f72647ac22f5b6`，sim DLL `fdc9da253163bfb3ddbd1704f31b527ed37fd116a683db2463667c8dd39a0584`；ready选择为ignored `native-body24b-ready-final1.json`，SHA256 `84dc7495a9d2b2f6c1b1a2943515ffecfd728a4b55ac1aa265034a1a7f43ac0b`。42导出、原renderer／MinHook来源保持。

`continuous-hand-ro1`以24bsim完成0.4／behind／tiny只观察／测试静音正版实跑。Rig／Weapon／Continuous数据能力开启，旧Pose／CCTRL／Core／HandGoal事件观察关闭，Body／Input／Camera／Run／engine_restore保持OFF；通过既有FP toggle切入preview，没有发送Pose事件。411个真实outerTick各调用一次Core getter，Core读取拒绝0；394 Tick完成四份计划，左右prepare分别411／405，总拒绝计数45不是失败Tick数。最终pending0／failed0，完整Native／entry96项独立恢复、XR0、音频10、原十文件hash、PG／tiny与准确自有PID退出全部通过。二进制`continuous-hand-plans.bin`为132944B，SHA256 `891b36404852cce6e039decf97b1c36ac4eaf75f205c9401651d56249670da62`；容量32的最后32条均complete且failure0，独立ABI parser无issue。总计算不受32条环形存储容量限制。

该轮明确使用adaptive只读诊断及24b的旧strict矩阵算法；`runtime-final.json`的accepted只表示连续采集和生命周期通过，不表示正常臂长、FP对齐、骨骼写入或physics／render fence通过。当前tracked已选择接入24b的6既有／5新文件，capture／CMake仅应用对应hunk，逆patch完全回到当前前态，并保留已提交的新shared政策。Root以当前实际headers严格CL编译owner／actor两个helper，13个owner模式加caller SHA认证／actor检查全部通过，回执为ignored `continuous-hand-tracked-integration1/cpu-1792904583612796200/receipt.json`；第一次缺vendor include路径的失败保留。此CPU结果不是当前shared政策与continuous组合后的完整DLL编译，已运行的冻结24b与当前tracked来源继续分开记录。

武器会话对FP2同局眼部参考骨57／60的进一步核对给出：world_head距双眼骨pivot中点约18.1519cm；之前26.9132cm是距head55，不应等同真实眼点误差。该参考仍不是实体眼睛或IPD标定。肩线相对相机右方向约偏42.8655度，单独移动视点不足以解决完整双手reach。现有骨4的原生Recoil控制可位于双肩／头的祖先链，但必须保留它自己的自然状态gate、状态更新和原FK路线；正在补齐只读实际调用与post-weight scale数据，不强制激活原控制，不以endpoint数据推定未来调用许可。LGrip抓边／保持与缩回、躯干最终朝墙、低位中立视点、受击／枪口／碰撞及跑步最终视角仍未完成，不能将持续数据接通描述为掩体玩法可用。

Native25在实际24b上只改CCTRL／CMake并加4个文件，706文件整树逆patch回到702基线；原80defines、renderer／MinHook来源、strict2bc和42导出保持。9个新增CPU场景463 checks、BodyOff／ControlOff各11 checks及ABI检查通过，累计296项focused CTest，normal与sim各实际15 DLL+8 component预检通过；sim的296项CPU按真实Ninja／源码／flags闭包复用，不冒称重跑。Normal DLL SHA256 `b2ae70ba1d99c4c17ca1ec7703b1f54f46f91d2abb964239caf7e38254cf4400`，sim `4139c64a46f8bbba999ffd144bbd1999d9408c66994835ca8bbd323eb0dd473f`；选择为ignored `native-body25-ready-final1.json`，SHA256 `19663a3f7b0a1658e96448c0ae260056f6a64a93f4ca7b0655fd9688a629aa75`。该模块默认OFF，仅复用旧v278返回观察，没有新增hook／getter／engine调用或pose写入；cap-OFF显式enabled请求会拒绝，不静默忽略。

`post-weight-scale-ro1`在发送事件前被Root测试guard误拒绝：新模块只有capture-status的启动／最终print声明，periodic session-status未含该字段，实际INI及启动声明均为ON。未发Pose事件、空88B sidecar；原失败及finalizer缺event错误保留，96项清理独立通过。`ro2`仅修正私有guard从当前capture-status取真实启动声明，核对当前launcher PID／创建时间和Native PID；对旧真实失败轮读回通过，缺声明／错owner／错PID三反例均拒绝，未改DLL或削弱原身份／输入／窗口保护。两轮均0.4／behind／observe-only／mute，Continuous与HandGoal OFF，Pose／CCTRL／Core数据ON。

`post-weight-scale-ro2`实际session1650050334510／PID54836／Compose44873取得四组`CPSCL001`，24984B，SHA256 `f7cf3b3ee85517bd90a2e286883d81af30d0dd340fd008437a29189d79f61260`。Root使用25实际ABI和独立parser再次解析，4/4只读观察qualified，骨序分别8→10→11、11、32→34→35、35；前后read_mask均127，cached flag为实际DWORD0，scale header的data／count／capacity全0，真实alpha bits均3f800000，完整返回packet保持，Compose正常关闭且无nested／concurrent。嵌入8个Compute／weight Row与原CCTRL逐字关联，唯一变化为实际close更新offset1938的compose_returned 0→1，其余包括padding逐字相同，不做伪零归一化。完整96项恢复、XR0、音频10、原十文件与准确PID退出通过，窗口释放。

独立parser SHA256 `f789d7d76e2eeea06ac7eafb998172a957f7c5a43f820f78487e40df1df0d8f8`，13项格式／cohort／claims／截断／未读值等正负检查通过；Root实际结果为ignored `post-weight-scale-ro2/runtime-scale-parse1.json`。它只排除本轮四组v240额外缩放，未来v248/g、其他状态、正常reach、骨骼writer／物理传播未获证明。Scale25专属2旧／4新文件已最小接入tracked，CMake保留当前其他内容、逆patch精确，当前headers的ON九场景／ABI和两个OFF helper严格CL通过，回执为`post-weight-scale-tracked-integration1/cpu-1792905767363545400/receipt.json`；当前combined完整DLL编译仍与已运行冻结25分开记录。

独立接收武器会话的null-link身份复读修复`ab2ec3818b7e1ec0d7e3094a9e1bcfe13e88b8a2`，本分支为`fe4f90e`：只将UI组装期间判定可重试null前的controller／field身份检查恢复到真正fresh_read，避免assembly hint cache掩盖变化，既有typed-null／启动分类保持。Root本地两个修改测试模块实际7项通过；初次错误目录的0项不计，收尾误期待对方31项而失败的元数据断言保留，随后仅依据原exit0／7项日志与源码hash澄清，未重跑或冒称31项。实际范围和来源见ignored `analysis/ui-null-fresh-receiver1/receiver-final1.json`；未改根默认清单或扩展为新启动兼容性测试。

固定个人臂长接口按武器会话独立提交`35a44974575a22fd4ca8cc25ad07f4f60f58ed92`接收，本分支为`6affa4b`。`LimbInput`／`LimbData`的upper_length_scale／lower_length_scale默认均1，adapter每次显式复制到底层既有LengthPolicy，不从target推导、不改变输入矩阵／头手／IPD／world units。preserve只保留原始输入段长×固定比例；adaptive仍可伸长／重分配，不能把calibrated状态标签当作已完成玩家测量。私有C++ Workspace／输入布局变大，消费者必须同步重编，public Frame／Read／Core wire未变；玩家测量／持久化和live opt-in未接。Root首次raw hash预检遇Git checkout CRLF差异而在编译前拒绝，随后对peer实际原件hash、双方LF canonical和已提交Git blob逐项认证，仅换行不同，实际本地raw hash独立保留。严格CL adapter9组820、math24例417通过，当前连续owner／actor全部模式另以新headers重编通过，回执分别为`fixed-length-receiver1/attempt-1792907100878442400/receipt.json`和`continuous-hand-tracked-integration1/cpu-1792907047716080400/receipt.json`。本地helper不冒称Root完整TU；武器会话727源组合TU／198相关CTest及14个continuous模式为另一份明确来源的证据。

Native26初始Recoil候选只读窄审修正opaque scale的stride4假设：只保存稳定双读的真实三header／DWORD，新增headers_read512，非空scale明确拒绝output_read64，不读取payload。Root再发现nested且selected=false的复制WeightCall仍可能消耗depth责任，26在已编译后保留为superseded，26b对所有counted call加exact top／previous地址链并补反例；错误end保债，真实nested／outer顺序可排空。26b在实际25上4既有／4新增，710文件的inverse精确回到706；81defines只增BodyRecoil能力，42导出、renderer／MinHook／strict2bc与25 Scale接线保持。normal DLL SHA256 `90fa333909401783a91fae512bff878df71923c8261e48d4467254199dfc1bae`，sim `7e7323639cf56a3ef6a27fa040455ce1a977dbce285b3cc7b9bfdefd8eba723b`；ready为`native-body26b-ready-final1.json`，SHA256 `29cb851a8b50b358fa47fa46a24b4e1b5176e5848a1972c5909019edb4dbad10`。normal307项focused、Recoil205 checks（含opaque25／nested-end24／OFF7），两版各15 DLL+8 component通过，sim307按实际闭包复用。旧26、构建前参数失败和basename旧回执均保留。

`body-recoil-ro1`已以26bsim完成0.4／behind／observe-only／mute正版连续观察。旧event／post-weight诊断OFF、Continuous／BodyRecoil只读ON，通过一次既有FP toggle，无Pose event，不强bit／strength或调用Compute。实际408 Compose观察、自然调用计数0、拒绝16；32槽末尾记录均failure0／returned1／no_natural_call1／self-qualified1、mask399（context／model／map／begin／end／code），没有headers或output读位。二进制165464B，SHA256 `83e7fce686f08a7546d129a527ba5921af1af402fe810dcb0c24941da7358a51`。Root用外部receipt的session1663201348914／PID50808／base140000000运行独立parser，结构／claims／cohort均通过，32条独立一致的输出为0，不能把该静止场景、环外记录或16个拒绝窗口推定为整段inactive。完整96项独立恢复、XR0、音频10、原十文件和准确自有PID清理通过；这不是身体姿态或writer验收。

Recoil parser SHA256 `252abcfd24784da8f7c1899eeae63c4bdb7afd8bedf5ba2afbfe43f419f520c1`，与26b机械完整ABI一致：Header88／Record5168，19项格式／readmask／cohort／opaque／异常／环／CREATE_NEW检查通过。Root实际结果为`body-recoil-ro1/runtime-body-parse1.json`。已把26b的4既有／4新增选择接入tracked，capture／CMake仅应用对应hunk并精确inverse，当前新shared固定长度headers下的Recoil ON九场景／ABI／OFF严格CL通过，回执`body-recoil-tracked-integration1/cpu-1792907275905002200/receipt.json`。编译和运行默认仍OFF，不授writer、独占、v248或physics fence。

限定静态审计进一步确认：原Compose在1293F5／1293FA已消费scratch到SpaceBases，129419已写转换后的Atom，之后才12946E→129474取v248/g；等真实g返回再只改旧scratch已晚。下一最小观察为旁观原调用并关联现有post-weight四组，不额外调用getter，不将端点+AC或多次g=1当区间恒定。审计为ignored `analysis/post-weight-future-g-audit1.md`；躯干自然动作覆盖和g旁观候选继续私有实现，当前未开放LGrip抓边或最终骨骼写入。

Native26c增加默认0的`freeze_on_first_natural_output`，只有合格自然输出已保存才冻结环写入，原调用／Core／累计计数继续，Header88／Record5168保持。首次312项检查中一项新测试因WinProfile相对INI路径失败，原source26c／build保留；source26c2只修测试绝对路径，生产字节相同，normal312项与Recoil1596 checks通过、sim依实际闭包复用，两版各15 DLL+8 component通过。ready SHA256 `1f55b584598b2e0b62e3a7547eee3e0824104d6963194e0684221dd2d0730340`；normal DLL `d5a25d6559d8f280305ff778aca4c03a13af3d2a21f2ae84a265ad95e1eba99c`、sim `18704cef03f4664414b28cc6c131136e657a278b3206f5d9fdf76dcc01b700fc`。并未改正常默认。

`body-recoil-fire-ro1`以26csim完成一次原生owned HWND `Game_Shoot`输入：请求80ms，无aim／retry，现成pulse在finally释放，同身份UP后请求正常stop。输入释放验证通过，未取得post-shot ammo，不能声称ammo／damage验收。实际Header sequence491／composes509／natural_calls12／rejections21，32条保留记录中有1份独立一致的自然输出，mask1023／alpha1／hasScale0；冻结阻止后18次更新覆盖。165464B SHA256 `a0a0c7b933764c426a51fe0e5470e2a9594d9faa255b77407549fbb5f554b18a`。实际输出QPC区间在派发DOWN之后约11.952ms开始、在UP之前约72.502ms结束；这只是时间关联，不替代弹药因果或伤害证明。完整96项独立恢复、XR0、音频10、原十文件与准确PID清理通过。

freeze parser2原本假定最终状态只有一行，实际固定producer的`write_session_status`另写`complete=1`末行，maker因此正确保留拒绝回执。Root新增独立parser3，仅对固定native26c产物接受“唯一状态行＋精确complete=1末行”，仍拒多个snapshot、缺失／错误／多余末行；旧parser2／全部原文件保持。实际freeze receipt与typed解析通过，另6个真实格式损坏副本均拒绝，结果为`body-recoil-fire-ro1/runtime-body-freeze-parse2.json`和`analysis/body-recoil-parser3/validation-final1.json`。producer／session1704512707745／PID17792／base140000000／创建时间／清理状态仍逐项核对，未放宽身份或计数合同。

Native27从26c2的710文件最小派生718文件（5既有／8新增），82defines只增FutureG能力，339项focused实跑、sim按实际依赖闭包复用、两版各15 DLL+8 component通过；42导出、原渲染／颜色／左眼history与旧ABI保持。新9B getter需要的精确VMOVSS搬运仅cap ON生成，vendor与cap OFF源保持，ON新源hash明确为`24cc672eb7d20d9c38763a282e7c2abd90d3eb177a2834b0d8d79a8ca1d47984`。真实call独立exact-self计数及饱和pending已修复复制end导致计数回绕的P2，失败版本保留。ready SHA256 `4b131ba4efdadc82eba4f4adcef960e8b6748df5506b8d6d56387685dcc1c8b7`，normal DLL `8934b07d97f0b7f60abb4f305eef71dcce9ac7bdb10d6bf695997ada4bc51123`、sim `a62d6f9f73c5f99c59b003af5736ed009772c935458deba016a45aab97d192ea`。

`future-g-ro1`实际一次FP toggle与一次Pose事件取得四组原生g返回，全部1.0，Scope=session1732915906423／PID7868／Compose45116，Header88／Record7728共31000B，SHA256 `51511acbd5169dc1a3fd1dc490f1f6e8f6dcc3053c432ae08630877e12661b53`。独立parser按真实ABI与外部event cohort解析，4/4 qualified，旧Post／CCTRL逐字关联通过，完整98项独立恢复（原96＋新getter完整9B／固定32B）、XR0、音频10、原十文件和准确PID退出通过。解析器未预读新g参数块，旧Post原件的参数不变性没有被推为当前或未来写资格。实际结果为`future-g-ro1/runtime-future-g-parse1.json`；writer／g区间／producer→C0／physics fence仍为false。

连续body正式收敛到原动画输出进入基础FK前的C0事务，Recoil不再作为持续carrier。Root按KB先查，在私有IDA副本机械导出F7940、109970及F6D8C／577E5F／10AEB0，原IDB SHA404ae保持。已有RO4 F7940仅盘点，不是本帧producer执行认证；F9341后还可能root-motion及109970处理，最终提交边界应为F9487前。IAT thunk的实际raw6解出131CA18，与旧IDA表达131BA18不同；磁盘PE无匹配不冒称memcpy身份。唯一producer→C0定点样本仍在准备，不扩整棵动画树，证据在`ida-body1/fresh-local-output1/`及`fresh-local-helpers1/`。

新增真正完整的纯C0 body planner，当前Root源码为`full_body_c0_planner.h`／测试／CMake。以完整115 canonical签名和真实C0 Atom32＋CTW构模，输出115个Local Atom、changed mask、更新头／肩／骨盆／脚与残差；用原有保段长双骨solver和有界脊柱FABRIK，所有非Root LocalT与scale逐字保留。独立物理位移和已提交Grip偏移各合成一次，Root heading与头旋转分开，两种posture的纯转头不移动Root／骨盆；包含Ref中立上身。失败保持整个Output旧字节，不缩不可达目标。首支持固定脚、scale1、身体yaw±60度；不等完整cover180、脚步切换或运动验收，head-bone world目标的HMD／眼作者映射仍由上游提供。

该planner独立审查无当前纯数组范围P1／P2，Root接收只改三include，原件／fixture保持。当前实际tools共享headers严格CL重编，9456 checks／0fail、20份完整115×32B候选，最大头残差0.000254662u、脚0.000570187u、段长0.000314261u；主要计数是逐骨验证，不能称9456个独立玩法场景。Root tracked编译与实际include回执为`full-body-c0-tracked-receiver1/cpu-1792914362233462300/receipt.json`。CMake缺私有fixture时不注册该fixture用例，不把skip当通过。尚未把planner结果写入Game，没有宣称C0所有权、reader或physics fence、LGrip实际抓边与最终玩法完成。

Native28在实际冻结27上只迁入4既有／9新文件，727文件／83defines，保留原82项定义、42导出、26c freeze、FutureG、strict2bc与原渲染。Optional三参入口不再读取caller第五槽，Root五参保留；返回packet在全部观察复读后比较，失配拒绝并只恢复Mod自有packet。normal真实356项定向CTest及15 DLL／8 component预检通过；sim真实TU／预检通过，356项按实际依赖和flags闭包复用。ready SHA256 `651e2b37464423d21c5ffb314a03746cc2449119f71fbc96c3e82118c9a9af51`；normal DLL `43476a3837f47f1d8e6cb96b5d5186aeb7c3701c0c1080eae3ac9ba134d7c41d`，sim `25c3658573bf1da8d1cf2ed030511863188752436e0ce6861931118b81756f81`。sim脚本的父源计数在执行前由718修为727，缺失独审报告补齐，旧冻结源保持。

`fresh-producer-ro1`以0.4／behind／observe-only／mute完成一次既有FP toggle，无Pose event／射击；实际session1765175332446／PID51640／GT20748，tick282／Compose283得到五stage全部31的真实Root→Optional→C0样本。FPOSE001共66480B，SHA256 `d3278f14cf526a8e25af868c86d5cf7c9954ab8882d2da8d7006a974c4b4e4ee`；Root第五参数真实读取，Optional第五为零，packet／真实闭合通过。外部身份／模块base和session独立关联，完整99项恢复（旧96＋F7940 hot/cold及109970三完整chunk）、Native／entry、XR0、音频10、原十文件及准确PID退出通过。最终实际typed复核为`fresh-producer-ro1/runtime-fresh-review1.json`，SHA256 `661a6d365d8bc946199b3eacd67c470ebb355e24eac929a0d3ec008c25ab3801`。root70=0／cache空，首child weight1／flags0的v270仍为已挂钩F7940；不能由这一层或端点相等推成fresh全覆盖。只补实际执行递归分支所需的有限call树，不扩整个离线动画树。

保存的F7940还存在零贡献时直接把旧Local送入F6D8C缓存的路径，cache-miss返回不授退役receipt。静态审查与真实样本继续分开。阈值常量由原始RIP字节解为实际RVA17070D8／17070D4（raw `acc52737`／`58ff7f3f`），旧IDA全局表达均少1000；机械证据为ignored `analysis/fresh-producer-branch-constants1.json`。新测试不得采用旧表达地址或把weight==1作为全部branch条件。

全身C0事务内核接入tracked：完整Source／Output／Desired及115骨FK、changed mask、段长和七个marker复核之后，仅对变化的Q16及Root T＋原scale16使用CAS；最多116单元。失败逆序exact ours→before撤销，未知值保留责任。消费通知在任何身份拒绝前闩锁，原返回／异常的typed finally也兜底；原Compose开始使用后绝不回写旧C0。后来同owner／旧存储且真实覆盖全部欠责字段的已认证producer才能退役，值相等、错end或部分覆盖不能清债。最初错误通知后仍可能回滚的P1已修复，最终两份独审及冻结delivery保持。Root仅适配一个include，以当前真实共享headers严格CL重编，原RO3和本次新采集的精确C0各24741 checks／0fail，真实本进程CAS128及PAGE_NOACCESS SEH通过；计数是13个实际单元逐字段／故障矩阵断言，不是24741个玩法场景。接收回执为`full-body-c0-transaction-tracked-receiver1/cpu-1792918283854668100/receipt.json`，SHA256 `76d59ebcb247876b40aebc383f5728f1fe0fd8b7a10c1a712cb786f12d2cbce7`。Native backend仍明确未实现／拒绝，Host通过不等同Game写入、独占区间或物理验收；真实接线继续进行。

新增纯`body_head_authored_mount.h`，从完整已签名的静态Ref与独立骨名关联选择Head55、LtEye57／RtEye60，以两眼中点构造固定Head→Eye marker，中立HMD方向显式采用作者指定的UE actor +X／+Y／+Z，不读取当前动画Local／CTW或旧FP camera自动校准。Eye/HMD UE-world pose与Head骨pose使用不同类型；转换保原Eye输入，每次复核完整canonical／Ref，未把脸骨间距作为玩家IPD。固定eye位置纯旋转时，Head骨因固定offset走弧，该骨delta不能再次计入physical／Root／Pelvis。原世界参考下的固定Head局部offset约(4.86286695,-8.56014083,-0.00011918)原单位；这只是designer_authored模板，不是物理校准或相机高度修复已生效。

Root逐项验冻结交付后原字节接入，当前真实共享headers严格CL重编；原RO3及最新fresh-ro1完整Ref各1013 checks／0fail，包含独立quaternion FK与192组固定eye位置旋转往返、错模型／Ref／输入和失败输出不变。实际最大往返位置误差约8.7e-15u，独立FK差约1.17e-6u。回执`body-head-authored-tracked-receiver1/cpu-1792919063575648600/receipt.json`，SHA256 `50c664222ebada11c599b0dcfdad3fee1602f6f61e5259848e9ddae8b131d7ec`。真实protected cover neutral、Rig与renderer共用base和Native body writer仍待接线，未替换默认或宣称HMD视觉接受。

Native29仅在实际28上4旧／1新增加有界实际F7940调用记录，728文件／83旧defines完全不变，42exports和旧FPOSE ABI保持。Normal真实374项定向CTest及15 DLL／8 component通过，sim真实TU／预检与实际闭包复用374通过；ready SHA256 `0b8d75e46aebe2635fae90be70478edb7b080d43037a077398c9a5494ec14cd7`，normal DLL `ddc16beb34047e5e095a95f692ebe12115360dd1053f98403db2f111c979b8b3`、sim `cdb75328edf4ec21cc8d7d65adab14640c3757677b5ae96d5d77fbe94058450c`。派生脚本的recipe键／source token及sim旧输出名错误均在编译前保留失败记录；source29sim已stage后只继续未完成的编译／复用审核，没有为元数据重复已通过的normal374。

`fresh-tree-ro1`实际session1796769383690／PID30400／GT55932，tick300／Compose301取得13次真实F7940入场，全部原返回／exact end／packet保全；树86064B，SHA256 `6d176e046e6ac8d986fdb9349b237bb583d932dd6134b55754be39d126b17747`。99项恢复、Native／entry、XR0、音频10、原十文件及自有PID退出通过。`transport=true`但`accepted=false`原样保留：7个unknown边分别为F5C1D、10B561、385817及四个FADC4；out／Desired／R9／fifth没有变化，是未观察的override层缺口，未把最近F7940 parent冒称完整原生父。实际树分析在ignored `analysis/fresh-tree-runtime-branch-review1.json`。Root只对实际6目标及当前leaf／cache helper做私有IDA机械导出，原404ae IDB保持，所有Owned CLI exit0；F8AF4 cache-miss进入E08A0，故不能只凭返回或copy库模块名推全frame洁净。

全身planner增加显式±π heading，以及可选`world_absolute`目标。骨盆始终追上游稳定neutral world＋physical＋committed Grip各一次，Root使用显式世界Q；绝对模式拒非零relative yaw，Head／feet仍是实际绝对目标。默认旧relative行为保持，脚不会暗移，不可达完整Output不变。Root接收合并heading与absolute，仅对测试include作两个准确替换、将旧越界反例更新到新π上限之外；当前真实共享headers严格CL编译与365012 checks／0fail通过，其中13504为旧计划／heading，351508为1000帧回灌逐骨不变量，并非365012个玩法场景。Root／Pelvis回灌漂移0，head／feet残差约6.63e-5／6.26e-4u，48份继承产物逐bytes一致。回执`full-body-c0-absolute-tracked-receiver1/cpu-1792922639039692300/receipt.json`，SHA256 `53fde47e52cb0d183b05cb0ad0e8b2c0a901f04dd3c7dea1f53860038477acfc`。私有Goal从232增至296B，消费者需同步重编；Frame／Output及公开wire未变。稳定neutral、朝墙方向和脚步策略仍由上游actor／Cover提供，不能从含旧偏移的动画或上次Output重新校准。

私有实际Native Backend已实现真实read／CAS128及同latch C0接点，c2修复了结束路径错误依赖新工作admission的P2：stop／Core过期／hand sticky不再阻止原World lease仍有效的exact finally，错Entry／metadata／冲突仍保债。实际live＋actor reader严格638检查和Native ON／OFF编译／42exports通过；最终独审为ignored `analysis/full-body-c0-native-backend-final-audit2.md`。当前Goal策略尚未调用，字段覆写receipt正在实现，未在Game提交body pose。后续以原生真实Q16／T12＋scale4写入事件退役Local责任，区分字段覆盖与全frame BaseClean；时间更新、相同数值或未知解码格式不能代替store证据。同步C0入口契约仅作为工程推断，不宣称全程序reader／physics fence。

真实Native内存核心的header／65项Host测试现已原字节接入tracked；只接内存后端，不夹带旧28 capture／CMake或尚未组合的live。Root以当前Goal296、事务及实际共享headers严格CL重编通过65／0，覆盖真实进程read／CAS128／PAGE_NOACCESS SEH、混合source／ours、非owned字段／Ref／Desired／header／CTW改变、单次地址permit及消费后的保债。实际Root includes和前后hash见`full-body-c0-native-core-tracked-receiver1/cpu-1792923404915454200/receipt.json`，SHA256 `db77cb73410a6d6d692d75170d3dbe45184697ff6f385fc9f669a040ab240d58`；它不代替638项实际live审查，也没有声称已在Game写入。
