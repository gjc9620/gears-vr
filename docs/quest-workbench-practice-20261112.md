# Quest workbench 0.3.0 controller practice and left-shoulder supply

Owner scope: remove runtime motion recording/replay, move the component inspector farther away,
and create an edit → controller practice → edit loop for Lancer acquisition, GrabPose, regions,
snap, magazine removal/replacement, left-shoulder supply, charging, C starter and sights/firing.
The later direct request explicitly includes left-shoulder ammunition pickup. This supersedes
the previous instruction to leave the0.2.0 workbench unchanged; it does not authorize PC game runs.

Source: `J:/codexWorktree/gearsVr/tools/quest-workbench`, branch `codex/6dof-single`.
No helpers, original game, PC XR/simulator or shared Mod runtime writer/build was used.
No public package/tag or game maturity/headset acceptance changed.

## Delivery

- Private candidate: `J:/gearsVr/candidates/quest-workbench-20261111/Quest Workbench.exe`.
- APK: `GearsVR-Quest-Workbench.apk`,0.3.0-dev, code9,16434539 bytes.
- SHA256: `d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c`.
- Build: `J:/gearsVr/analysis/quest-workbench-20261112/apk-build-v9-final`.
- Rollback: candidate `rollback/0.2.0-build8`, original APK75d28bb3…e8c1ff.
- Current delivery status: compiled/signed/aligned16K/packaged; **not installed or run on Quest**.
  ADB returned no devices during this change. A concise reconnection question is pending.
  Do not describe0.2.0 device receipts or historical UI PNGs as0.3.0 validation.
- Review/source hashes: `analysis/quest-workbench-20261112/delivery-v030-review.json`
  and `quest-source-v030-manifest.json`. Full Chinese instructions accompany the candidate.

## Implemented behavior

`practice.h` adapts the same portable Mod geometry/state kernels through a frozen36-header
dependency snapshot in `game_rules/`; each source hash is in `assets/game-rules.json`.
The snapshot is a defined source version, not a promise of every later private Mod change.
Source game modules, strict kernels and active game test artifacts were not edited.

Right squeeze acquires the main grip and right trigger fires into a10m target. Main-hand
acquisition uses the existing35-degree authored wrist mount; free left/right use20-degree
calibrations. Startup-held controls cannot acquire or fire, and menu aiming suppresses firing
until a real trigger release. Focus/tracking/space/time discontinuities clear ownership.
Controller grips require valid AND tracked position/orientation flags15. The gun, not HMD,
owns shot direction and the shared25m sight zero. Acquisition transition blends only on grab;
ordinary tracking is direct after it completes, without continuous smoothing latency.

Left squeeze or trigger selects the nearest admissible part. Support uses the shared two-hand
solver and the configured release boundary. Magazine extraction must observe the real4.5cm
outward stroke; the same old item can be restored. After removal, the left shoulder uses the
shared body-slot default to grant a replacement from the same pure inventory model. Insertion
uses the shared fitting/6cm guide/2cm real approach/180ms seat policy. Empty replacement requires
a subsequent E full pull and observed release/return before firing. A short12cm E pull cannot
finish; the original long slot and bounded10cm endpoint assistance remain. E waiting-open and
return gates consume actual sandbox render acknowledgments, not elapsed time alone.

C uses the shared local rail,8cm stroke,98% endpoint, real motion/minimum hold and observed
part constraints. The motor can remain held indefinitely; reverse stops it, and first actual
release starts a3s tail. It cannot start from stationary proximity or stale presses. Original
engine acknowledgment fields in this standalone adapter explicitly describe simulated scene
side effects. They are not game execution/actor proof.

Shoulder center and size are visible in its setup page and optional practice hints. The default
body frame preserves workspace/initial yaw while translating with the head; it does not rotate
the shoulder region when only the head turns. A real left controller can place the supply
center, and sliders adjust XYZ/scale. Q3 supplies no torso tracker, so this remains a body
position proxy and authored armspan layout; B/Y recenters after turning.

The inspector defaults pinned60cm to the side,30cm above and25cm farther behind the selected
anchor, with45..120cm adjustable side distance. The user can fix/follow or close it. Practice
hides large editor cards. A/X returns to editing; pressing enter again preserves scene ammo
and mechanism state, while30/360 and0/360 reset options begin explicit fixtures. Previewed
library poses can be adopted and tested directly. Authored defaults are labelled游戏默认;
real captures and their provenance remain preserved.

The original bone8 magazine is a separate mesh. Geometry sum remains13397 triangles:
body10798, receiver1300, C1089, E54, magazine156. Full Lancer.glb stays identical to0.2.0.
Moving-part capture transforms now include arbitrary part deltas, C angle/pull, E travel and
detached magazine position. Legacy scalar captures remain readable. The frozen legacy
`adopt_lancer_captures.py` was preserved; new0.3 compatibility is isolated in
`adopt_moving_lancer_captures.py`, leaving existing source-adoption gates unchanged.

Runtime Recorder/history/replay is removed from Android and host build targets. Archive
compatibility codecs remain unused by the app; existing old.take files are not deleted.
Single real hand capture, calibration, independent archives and pose editing remain. No
synthetic26-joint samples or source timestamps are created from controller/preview hands.

## Verification and practical limits

- Android arm64 NativeActivity/GLES/AAudio compiled without warnings in final compile log;
  signing, manifest version9 and16K library/zip alignment verified.
- Windows companion compiled. APK packaged asset hashes and36 frozen header hashes verified.
- Host core checks passed: bases, real-capture flags, shape/snap math, fixed finger lengths,
  input edges, controller object following, selectors/rings and independent atomic archives.
- New host practice trajectories passed: cold presses, shots/ammo, menu suppression, stale
  input/epoch/tracking withdrawal, support release and volume rejection, stable shoulder,
  tactical and empty shoulder replacement→insertion→charge→fire, old magazine restore,
  short/full/render-waited E, C no-gesture/no-render denial, sustained motor, release tail,
  reverse stop/restart and tracking-loss stop. These are CPU fixtures, not real tracking.
- Four existing Python editor/library conversion tests passed. Four retained actual0.2.0
  left snapshots round-tripped through arbitrary moving-part fixture deltas, worst position
  error7.11e-15 original units and rotation-matrix error4.45e-16. Those rebases are explicitly
  offline fixtures, not newly captured poses. See check-practice-capture-conversion.json.
- Two concrete adapter failures were reproduced/fixed: exact seated floating-point endpoint
  could start slightly negative, blocking extraction; E attempted move on its begin sequence
  and immediately cancelled. A test expectation about inventory reserve was corrected to the
  shared model's returned-old-rounds/reserved-replacement semantics. No kernel was relaxed.

Cadence, visual recoil and synthesized audio are labelled previews and editable. The original
game engine, original audio, animation, physics/mass, recoil, enemies and executions are absent.
The app cannot establish complete game feel parity, collision-free surface fitting or physical
comfort by itself. Main/body full-arm IK is not run here. Rendering retains1.15 recommended
eye dimensions and2xMSAA, but new-device clarity/performance remain unobserved.

Full projects save wrist/fingers, region/snap/shoulder/practice settings and panel placement.
Desktop V2 export remains fingers-only. The existing separate source-adoption flow handles
full wrists/fingers; new region/snap/shoulder/feedback parameters are **not automatically
consumed by the Mod**. Later explicit game-side configuration integration and a real-game
comparison remain required. Do not infer full Lancer or original C defect acceptance from this
standalone workbench. No Git commit was requested or made for this turn.


## Installation follow-up (2026-11-12)

Owner reconnected Quest2G0YC1ZF9H0MWD and authorized installation.13 external application files
(22733604 bytes) backed up to analysis/quest-workbench-20261112/pre-v9-device-backup.
Coverage installation succeeded; package reports0.3.0-dev/code9. Installed APK pulled back and
SHA256 exactly matches d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c.
PID4922 initialized Touch actions, simultaneous input, both1931x2023 eye targets and AAudio;
ready logged, OpenXR stayed IDLE/state1 and Android reported Asleep. No projection/frame/GL
receipt or controller practice action observed. Owner explicitly chose to keep installation and
try later. Stop further launches/checks; do not treat initialization as headset acceptance.


## 2026-11-12 Quest工作台反馈与后续游戏接入

玩家认可工作台整体与C区域进入展开/离开收起，要求补齐运行结束后的滑柄退回和收起。工作台0.3.1已修复并安装，游戏侧没有修改；核心便携规则同源，但接入层及原引擎反馈未整体等效，不能声称手感几乎一致。后续负责Lancer的会话请先读 [工作台交接](lancer-workbench-handoff-20261112.md)，核对现有C恢复路径与配置消费者；保留已有严格保护及历史失败，未做游戏验收。


## 2026-11-12 Quest Touch水平握持枪口上翘反馈 / 0.3.2

玩家再次报告手水平时枪口约上翘45度，并提醒Mod阶段已提过相同问题。核对冻结
weapon_lancer_wrist_mount.h，35度配置明确针对旧PSVR Aim/Grip观察，不是通用Quest
物理标定。不能把35度直接称为此次45度的实测因果：离线Grip中立计算反而给出向下35度，
实际Touch Grip与物理手水平之间的角度未采样。本次修复的是不适用的方向接入方式。

工作台0.3.2/code11持枪时改用真实Touch Aim方向确定枪朝向，Grip位置/真实侧握和追踪
仍负责位置及获取；手腕由枪和当前主握GrabPose反解，并保持原掌心锚点。手腕/手指
修正不会再额外改变枪口方向。双手护木仍可参与已有方向求解，HMD不控制瞄准。
Aim与Grip均要求15有效/追踪标志；已用Aim获取后丢失Aim会释放，重新握持需新按键。
不使用HMD水平锁枪，不盲加固定-45度，不修改原始XR数据或游戏侧PSVR标定。

CPU回归通过水平、上下俯仰、不一致Grip俯仰以及主手腕修正情况下枪方向与Aim一致；
已有换弹/C/E/输入流程检查通过。Android编译/签名/16K对齐通过。尚无新版实机确认，
当前USB无Q3，安装连接问题已提出；最后安装仍0.3.1/code10。

后续Lancer会话应按实际输入设备区分Aim/Grip与握姿标定，保留既有PSVR证据；不要直接
删除游戏35度配置或声称工作台与游戏完全等效。此改动仅工作台。


## 2026-11-12 连续满匣拔出与肩部供弹 / 0.3.3

玩家报告拔出弹匣后不消失，背后无法取新匣。代码复现：已装满60发后再拔匣，
ReloadModel.begin返回full，工作台仍把外观设为移除，但库存事务停在idle，肩部只
允许feed_port，故第二轮卡住。另一个明确遗漏是松手后的旧匣一直留在场景。

0.3.3/code12仅修复独立工作台接入：拔出满匣/无备弹分支将实际旧匣弹量记录保存，
再以移除后的弹池开启本地换弹事务；原匣仍握着插回时恢复原数量，新匣从同一总弹池
领取，不额外造弹。总弹池为零时仍不能凭空供弹。底层共享ReloadModel没有放宽或修改，
不代表游戏侧库存提交已经验证。原匣在手中可见、可插回；松手后隐藏并释放部件占用，
肩部重新按键可领新匣。隐藏旧匣不再保留可交互的残留模型。

CPU新增连续三轮拔匣→松手隐藏→左肩取新匣→插满匣→再次拔出通过，已有旧匣归还、
空仓拉栓、C和Touch Aim方向检查亦通过。APK编译、签名/16K对齐通过。
包含0.3.1 C复位和0.3.2 Touch角度修复。当前USB无设备，尚未安装，最后实装仍code10。
后续游戏侧会话应核对原游戏满匣检查与正常取新匣的事务策略，不盲套本地reset/prime。

---

## 中文

# 查询工作台 0.3.0 控制器练习和左肩供应

所有人范围:删除运行时的运动记录/重播,将组件检查员移到更远的地方,
并创建编辑 – 控制器操作 – 编辑循环用于 Lancer 获取, GrabPose, 区域,
扣,杂志除/更换,左肩供应,充电,C起动和视线/火.
以后的直接请求明确包括左肩弹药的接车。
上一个离开0.2.0 工作本位不变;它不授权PC游戏运行.

Source: `J:/codexWorktree/gearsVr/tools/quest-workbench`,分支 `codex/6dof-single`.
没有使用帮助器,原版游戏,PC XR/模拟器或共享 Mod 运行时写作机/构建.
公共软件包/tag或游戏成熟度/头集接受度没有改变.

## Delivery

- 私人候选人: `J:/gearsVr/candidates/quest-workbench-20261111/Quest Workbench.exe`.
- APK: `GearsVR-Quest-Workbench.apk`,0.3.0- 德夫,代码9,16434539 bytes.
- SHA256: `d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c`.
- Build: `J:/gearsVr/analysis/quest-workbench-20261112/apk-build-v9-final`.
- 退后:候选人 `rollback/0.2.0-build8`,原始APK 数据75d28bb3…e8c1ff.
- 目前交付情况:汇编/签署/对齐16K/packaged; **未安装或运行于查询**.
  亚行在此更改期间没有返回设备, 简洁的重联问题有待解决 。
  不描述0.2.0 设备接收或历史的UI PNG 以0.3.0 validation.
- 评论/源码散列: `analysis/quest-workbench-20261112/delivery-v030-review.json`
  and `quest-source-v030-manifest.json`候选人随身携带完整的中文说明书.

## 已实施的行为

`practice.h` 通过冷冻调整相同的便携式模具几何/状态内核36-header
依赖快照在 `game_rules/`; 每一个源散列在 `assets/game-rules.json`.
shatch是一个定义的源版本,而不是以后每次私人Mod更改的许诺.
源游戏模块,严格的内核和活跃的游戏测试文物没有编辑.

右挤得到主握 右触发火成10 目标,主手
获取使用现有的35- 度作者手腕挂载; 自由左/右使用20-degree
校准。 启动控件不能获取或开火, 菜单瞄准压制射击
以至真正启动。焦点/跟踪/空间/时间断续续,明确所有权。
主计长握手需要有效的跟踪位置/定向旗帜15 枪,不是HMD,
拥有射击方向和共享25 零视距 获取转换只用于抓取;
普通跟踪在完成后是直接的,不连续平滑的空心.

左键或触发选择最近的可允许部分。支持使用共享的双手
解析器和配置的释放边界。 Magazine 提取必须观察真实4.5cm
向外中风; 同一旧项可以恢复。 删除后, 左肩使用
共享的机身槽默认允许从同一纯目录模型中替换。插入
使用共享配置/6 cm 向导/2 cm 实际办法/180 ms 座位策略。 需要空置替换
E型后部全拉力,在开火前观察到释放/返回。12 cm E 无法拉动
完成; 原长槽和边框10 cm终点援助仍然存在。
返回门消耗了真正的沙盒 使承认,而不是仅仅用时间。

C使用共享的本地铁路,8 中风,98% 终点、实际运动/最小状态和观察
发动机可以无限期地被扣住;反向停止,然后首先实际停止。
释放开始 a3 s尾部。它不能从固定的近距离或 Steal 压入开始。
引擎识别字段在此独立的适配器中明确描述模拟场景
副作用。它们不是游戏执行/代理证据。

肩部中和大小可见于其设置页面和可选练习提示。默认
弹框在与头翻译时保留工作空间/初始线程; 它不旋转
。只有头部转动时,真正的左侧控制器可以放置供应
中,滑动器调整 XYZ/ 比例。Q3 补给没有躯干跟踪器,所以这仍然是一具尸体
位置代理和作者的臂章布局; B/Y 转弯后最近者。

检查员默认被钉住60 cm到边上,30 cm以上和25 cm 更远地在所选位置后面
锚, 与45..120 cm 可调整的侧距离。 用户可以修正/ 跟踪或关闭它。 练习
隐藏大编辑卡。 A/ X 返回编辑; 按下输入再次保留场景弹药
和机制状态,同时30/360 and0/360 重置选项开始明确固定。 预览
library poses can be adopted and tested directly. Authored defaults are labelled游戏默认;
真实的捕捉及其来源仍然保留着。

原骨头8 杂志是单独的网格。几何和尚存13397 triangles:
body10798 接收器1300, C1089, E54 杂志156。完整的Lancer.glb 与0.2.0.
移动部分的捕获转换现在包括任意部分三角洲、C角/推力、E旅行和
离散的杂志位置。 遗留的标尺捕获仍然可以读取。 冷冻的遗产 。
`adopt_lancer_captures.py` 保存; 新建0.3 兼容性被隔离在
`adopt_moving_lancer_captures.py`,使现有的源选择门保持不变。

运行时记录器/历史/重播从Android和主机构建目标中移除. Archive.
兼容性编码器仍然被应用程序使用;现有的旧. takes文件不删除.
仍保留单张真实的手抓、校准、独立档案和姿势编辑。
synthetic26- 联合样品或源时间戳由控制/审查手创建。

## 核查和实际限制

- 机器人手臂64 汇编的土著活动/GLES/Audio在最后汇编日志中未加警告;
  签名, 显示版本9 and16 K 库/ zip 校正已验证 。
- 编译 Windows 伴奏。 APK 包装资产散列和36 冻结头部的散列物已经核实。
- 主机核心检查通过: 基座,真抓旗,形状/snap数学,固定手指长度,
  输入边,控制对象跟随,选择器/环以及独立的原子档案.
- 新的主机练习赛道:冷压、射击/射击、菜单压制、 Stale
  输入/划时代/跟踪退出、支持释放和数量拒绝、稳定的肩部、
  战术和空肩替换 插入 充电 火 旧杂志恢复
  短/全/等待E、C无位/无位拒绝、持续运动、释放尾巴,
  逆向停止/ 重新启动和跟踪丢失停止。 这些是 CPU 固定装置, 不是真实的跟踪 。
- 通过了4项现有的Python编辑器/图书馆转换测试。0.2.0
  左侧快照通过任意移动的段固定三角洲进行圆形绘制,最差的位置
  error7.11e-15 原始单位和旋转矩阵错误4.45e-16 这些重新基地是明确的
  脱机装置,不是新抓的姿势。请参看检查练习-抓捕-转换。json。
- 复制/修复了两个混凝土适配器故障:准确的座椅浮点端点
  可能开始略微负, 阻断提取; E 尝试按其起始顺序移动
  库存储备的测试预期值已更正至
  共享模型的回归旧轮/保留置换语义,没有放松内核.

带宽、视觉后坐力和合成音频是标签预览和可编辑的。
游戏引擎,原声,动画,物理/质量,后坐力,敌人和处决都不存在.
应用程序无法建立完整的游戏感觉平价,无碰撞的表面配置或物理
由自己来安慰。主/全臂IK不在此运行。1.15 recommended
眼尺寸和2 xMSAA,但新设备清晰度/性能仍然未见.

完整的项目可节省手腕/指针、区域/指针/肩/练习设置和面板放置。
桌面五2 导出时仍只使用手指。 现有的单独的源入选项流程控件
全腕/指针;新区域/指针/肩/背针参数为: **不自动
被摩德消耗**。随后明确游戏侧配置集成和真游戏
无需进行对比。不要推断出全部Lancer或C缺陷接受率
独立工作台。没有要求或要求进行 Git 任务。


## 安装后续2026-11-12)

重新连接的所有人查询2G0YC1ZF9H0 妇女发展部和经授权的安装。13 外部应用程序文件
(22733604 字节) 备份到 analysis/quest-workbench-20261112/pre-v9-device-backup.
覆盖安装成功; 包报告0.3.0-dev/code9 安装的APK 后撤并
SHA256 完全匹配( E) d1667acea691bfcaa59b4143871b7c73834c3d09b2fd451b91986d9ce50c1f7c.
PID4922 触摸动作,同时输入,两者1931x2023 眼球目标与奥迪奥;
已准备好登录, OpenXR 已保留 IDLE/ state1 无人投影/帧/GL
观察到了收货或控制器操作动作。
。停止进一步发射/检查;不要将初始化视为耳机接受。


## 2026-11-12 Quest工作台反馈与后续游戏接入

玩家认可工作台整体与C区域进入展开/离开收起，要求补齐运行结束后的滑柄退回和收起。工作台 0.3.1已修复并安装，游戏侧没有修改；核心便携规则同源，但接入层及原引擎反馈未整体等效，不能声称手感几乎一致。后续负责Lancer的会话请先读 [工作台交接](lancer-workbench-handoff-20261112.md)，核对现有C恢复路径与配置消费者；保留已有严格保护及历史失败，未做游戏验收。


## 2026-11-12 Quest Touch水平握持枪口上翘反馈 / 0.3.2

玩家再次报告手水平时枪口约上翘 45度，并提醒Mod阶段已提过相同问题。核对冻结
weapon_lancer_wrist_mount.h，35度配置明确针对旧PSVR Aim/Grip观察，不是通用Quest
物理标定。不能把 35度直接称为此次 45度的实测因果：离线Grip中立计算反而给出向下 35度，
实际Touch Grip与物理手水平之间的角度未采样。本次修复的是不适用的方向接入方式。

工作台 0.3.2/code 11持枪时改用真实Touch Aim方向确定枪朝向，Grip位置/真实侧握和追踪
仍负责位置及获取；手腕由枪和当前主握GrabPose反解，并保持原掌心锚点。手腕/手指
修正不会再额外改变枪口方向。双手护木仍可参与已有方向求解，HMD不控制瞄准。
Aim与Grip均要求 15有效/追踪标志；已用Aim获取后丢失Aim会释放，重新握持需新按键。
不使用HMD水平锁枪，不盲加固定-45度，不修改原始XR数据或游戏侧PSVR标定。

CPU回归通过水平、上下俯仰、不一致Grip俯仰以及主手腕修正情况下枪方向与Aim一致；
已有换弹/C/E/输入流程检查通过。Android编译/签名/16K对齐通过。尚无新版实机确认，
当前USB无Q 3，安装连接问题已提出；最后安装仍 0.3.1/code 10。

后续Lancer会话应按实际输入设备区分Aim/Grip与握姿标定，保留既有PSVR证据；不要直接
删除游戏 35度配置或声称工作台与游戏完全等效。此改动仅工作台。


## 2026-11-12 连续满匣拔出与肩部供弹 / 0.3.3

玩家报告拔出弹匣后不消失，背后无法取新匣。代码复现：已装满 60发后再拔匣，
ReloadModel.begin返回full，工作台仍把外观设为移除，但库存事务停在idle，肩部只
允许feed_port，故第二轮卡住。另一个明确遗漏是松手后的旧匣一直留在场景。

0.3.3/code 12仅修复独立工作台接入：拔出满匣/无备弹分支将实际旧匣弹量记录保存，
再以移除后的弹池开启本地换弹事务；原匣仍握着插回时恢复原数量，新匣从同一总弹池
领取，不额外造弹。总弹池为零时仍不能凭空供弹。底层共享ReloadModel没有放宽或修改，
不代表游戏侧库存提交已经验证。原匣在手中可见、可插回；松手后隐藏并释放部件占用，
肩部重新按键可领新匣。隐藏旧匣不再保留可交互的残留模型。

CPU新增连续三轮拔匣→松手隐藏→左肩取新匣→插满匣→再次拔出通过，已有旧匣归还、
空仓拉栓、C和Touch Aim方向检查亦通过。APK编译、签名/16K对齐通过。
包含 0.3.1 C复位和 0.3.2 Touch角度修复。当前USB无设备，尚未安装，最后实装仍code 10。
后续游戏侧会话应核对原游戏满匣检查与正常取新匣的事务策略，不盲套本地reset/prime。
