# 12 Weapons: offline interactive delivery

Date:2026-10-28 Review v 2- Take it.[Approved programmes](../all-weapons-interaction-plan-20261028.md). Default to cancel the full weapon R 3 Switching bullets: Start with the first clear physical action of this gun, R.3 Only optional release aids. Mainstream VR cross-checks, Boltok institutional assembly checks, the wrinkle bow off-launch design and two grenade fuse static surveys have been completed, and complete replacements are delivered. This time no changes to run source code, no buildup candidate package, no startup game/XR, or operational connection are established.

| Delivery | Contents |
| --- | --- |
| [The direct start is on the handing over of the order.](implementation-handoff.md) | Interface fields, gun-by-gun code differences, original inventory/receipt, loading responsibility, order of implementation and key inspection cases; code accordingly, as ordered |
| [Code Change Location](code-locations.md) | 33 relevant documents,78 Function Positions and Files SHA 256;indicating in symbols to allow main lines to continue advancing |
| [Interactive review and findings v2](interaction-review-v2.md) | R 3 Judgment, full-gun review, Boltok original parts/design joints, twist bow cancellation sequence, different trigger paths for Frag/Smoke |
| [Offline model produced](../../assets/weapon-6dof/offline-v2/README.md) | 3 (a) Full live ammunition, six-hole nests, reusable starters;5 GLB, preview, LOD, materials, tags and generators |
| [12 weapons-specific cards](weapon-cards.md) | Access per gun, air/silo/post-fired action, ammunition submission point, tip, interruption, model and code map |
| [Model worksheet](model-work-orders.md) | (a) Originals are duplicated, bone-driven, to be removed, and additional; including 12 The result of the widget check for a source grid |
| [Original code portal](original-entry-map.md) | Public bomb exchange services, original gun-by-gun function, grenade generation and initial replacement, call conditions to be certified before access |
| [Offline Specification Index](authoring-manifest.json) | 12 Division stability ID, specific class, original grid Hash, static capacity, model tasks and existing source maps; not operational configuration |

Cross-arms/shelter input cross-repeated, active cancellation, initial speed and life cycle[Common rules for total handover](../offline-tech-prep-20261028/shared-contracts.md)Yes; check against identification[Total handover](../offline-tech-prep-20261028/README.md)。

## Scheduled Interaction

| ID | Weapons | The core action of this round is set. | Original package static capacity reference |
| --- | --- | --- | --- |
| W 01 | Lancer | Grip grabs, implicitly releases,  ⁇  change boxes,  ⁇  natural thrust/lock lock | 60 Reserve ceiling 600 |
| W 02 | Hammerburst | Hand-opening locks, twirl boxes, twigs, twirls. | 78 Reserve ceiling 702 |
| W 03 | Gnasher | Start when you're missing the bullet, and maximum per round. 3 Fire, less rear locks; after firing, the lever cycle must be completed | 8 Reserve ceiling 31 |
| W 04 | Longshot | Open the shell, poach it, load a full live bullet, poach it, close the lock. | 1 Reserve ceiling 23 |
| W 05 | Snub | Pull out the grab and implicitly release  ⁇  Change box; empty tank pulls a slider, half-port free | 12 Reserve ceiling 60 |
| W 06 | Boltok | The frame is open with a bullet nest, the joint repulsion shell, the hair-to-heap filling, the closed and partially filled. | 6 Reserve ceiling 24 |
| W 07 | Boomshot | Open the barrel loading unit  ⁇  Plug-in filling component  ⁇  Close | 1 Reserve ceiling 11 |
| W 08 | Torque Bow | Load an arrow, Zip it. | 1 Reserve ceiling 11 |
| W 09 | Hammer of Dawn | Weapons point to target, Zoom, lock and beam. | Original function reports unlimited ammunition; unconventional reloading |
| W 10 | Troika | Get into the gun position, boom, double-share turn/fire, boom, it's automatic recovery, boom, exit. | Do not design manual change boxes |
| W 11 | Bolo / Frag | (a) Continuous Grip Access/ Hold Pine Grip Bump-Putting;2 kg（2026-11-08 User Overwrite) | Base Category MagSize 1、MaxSpareAmmo 3;not directly considered as a total HUD;[Current Authentication Status](../frag-grip-release-20261108.md) |
| W 12 | Smoke | Same gestures, separate smoke stocks, cartridges and effects. | Idem, read this example of physical inventory |

All capacity is checked by the default object of the saver and cannot override the actual values of running-time Getter, difficulty, mode or specific variants. Snub's silo branch, Boltok's reloading bullet etc. are VR design options for this project; it is not claimed that these physical operations already exist in the original version.

## Model conclusions

12 The gun/mine subject has a verifiable source and does not need to be re-doed 12 Put the whole gun. Lancer, Hammerburst, Boomshot's hand-held filling component and Torque Bow's hand-held arrow priority re-use the original independent resource; Snub's hand-held cartridge by original `Magazine` The other activity agencies prioritize the driving of the bone.

3 complete munitions have now been generated v 2 GLB, which contains two tiers of LOD, UV, PBR material and capture/insert reference nodes. Boltok's original bomb nest is closed on both ends and replaced with six real cross-sections. Lancer's original startup is now in the main line directory; this round checks its generation results and only copys it into an independent offline directory. Snub's original cartridge is now available. 314 The triangle has also been removed in private anallysis, keeping the UV/modeline/material belonging. There is no re-loading of the whole gun.

This time, we're rechecking. 2 Save resource packages,12 original grid,18 Default object and export from the package orientation 41 The new evidence identifies the original Boomshot fill, Torque Bow holding arrows, the specific type of dawn hammer and the chain to stop exposure. Bones audit identifies 71 A single, self-extractable hardcore group with a gun; it's not. 71 (c) Work on the completion of the modelling.

## Completion of border and residual matters

| Level | Current Status |
| --- | --- |
| Disparities between player action and gun-by-gun | 12/12 Specifications developed; rules for input, quota filling, hint and interruption are stated |
| Source and treatment decision-making | 12/12 Subject checked; stand-alone/generic/refill route established |
| Original semantic portal | 12/12 Map specific class, save function and existing module |
| Bones to mechanical parts. | Boltok Object 03 Stamp, Object 02 Blast, Object 08 Seter, Object 01 Fixed backdisk checked by level/assembly; original animation not checked 21 The first of these is the "ArnamSet" (the "AnimSet").80° Shake out for design start |
| Capture points, axes, filling axes, process values | Need to be marked against the exact example and unified body baseline; no guess values given this time |
| native call ABI, return value replacement and running ownership | Historical tests on public switchovers; special access points not all certified for call, item by item |
| New modelling and document checking | 4 New geometry,8 LOD, plus reuse startup, total 5 GLB delivered; is not a certified game ratio or running asset |
| Model loading, physical connection, game and header validation | Not implemented; not counted as completed |

Subsequent code reuse `tools/vr-common/` The first is to resolve the public quota and original timer/notification of ownership, followed by a gun-by-gun machine; the complete ammunition loads only select one common pipe line; the grenade share speed estimate, with two sets of atomic bombs retained and effects.

Implementation of the handover order has already indicated exact behaviour required to modify the old module, including R 3 Fields, old box-back pools, partial filling exit and minimum speed/place transfer threshold for hand grenades. The original GLB engine resource creation/loading backend remains a bottom line that must be prepared, with fixed input, component life cycle and first validation steps; it cannot be claimed that it already has a usable native call. Interactive design is not left to be reopened next time.

The round is out of line and the following sets of points, scales and status are required to be identified for a single call, without using the main line construction or game running window. `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`、`codex/6dof-single` and fix manual access without creating a separate running package. First edition starts checking with the source code HEAD as `10510241b620b2960d79b9d1abc517e42c9c59b9`;This is not the version number of this set of specifications that has been integrated.

## Local evidence

Source extraction, default values, bone group geometry and source Hash kept in neglect [analysis/weapon-specs-20261028](../../analysis/weapon-specs-20261028/)。`sources.json` (b) Recording of two packages of identity;`all-12-mesh-parts.json` (b) Recording bone group attribution;`explicit-defaults.json` That's right. 18 object resolution;`map-script-provenance.json` Records 41 The export source and the reconciliation limit of the item. `invalid-tuple-order` The document is a failed intermediate of the reservation and is not evidence.

Still unresolved static script;5 An export object has an array of interlinking warnings. Only the control stream, cursor and object references can be identified are quoted, and not exported `.uc` As a retail of abI, which is either compiled or certified. Original package, extraction geometry and back-compilation are not entered in the list of issueable assets.

v 2 New evidence is focused on `analysis/weapon-specs-20261028/review-v2/`：44 Article orientation functions reference,3 A Bow Status Export,21 A Boltok bone name matching scan, in situ assembly map, frame reference data, Longshot original shell references, Snub private shredding and final file validation records. Model viewer prevents local file URLs from being completed because the embedded browser has not completed UI measurements; preview maps have been rendered.

---

## 中文

# 12 种武器：离线交互规格交付

日期：2026-10-28，复核版 v2。承接[已批准方案](../all-weapons-interaction-plan-20261028.md)。默认取消全武器的 R3 换弹前置：由本枪首个明确物理动作开始，R3 仅为可选释放辅助。已完成主流 VR 交互对照、Boltok 机构装配核对、扭力弓非发射取消设计与两种手雷引信静态调查，并实际交付补件。本次未修改运行源码、未构建候选包、未启动游戏／XR，也未建立运行时接线。

| 交付 | 内容 |
| --- | --- |
| [直接开工的实施交接单](implementation-handoff.md) | 已定接口字段、逐枪代码差额、原库存／回执、加载责任、实施次序及关键检查例；下令后据此编码 |
| [代码修改位置](code-locations.md) | 33 个相关文件、78 个函数位置及文件 SHA256；以符号定位，兼容主线继续推进 |
| [交互复核与调查结论 v2](interaction-review-v2.md) | R3 判断、全枪复核、Boltok 原部件／设计关节、扭力弓取消顺序、Frag/Smoke 不同触发路径 |
| [已制作的离线模型](../../assets/weapon-6dof/offline-v2/README.md) | 3 款完整实弹、六孔弹巢、复用的启动件；5 个 GLB、预览、LOD、材质、标记和生成器 |
| [12 张武器规格卡](weapon-cards.md) | 每枪的取用、空仓／半仓／射后动作、弹药提交点、提示、中断、模型和代码映射 |
| [模型工作单](model-work-orders.md) | 原件复用、骨骼驱动、需拆出的件、需新增的件；包括 12 个源网格的部件核对结果 |
| [原版代码接入口](original-entry-map.md) | 公共换弹事务、逐枪原函数、手雷生成与初速替换、接入前尚需认证的调用条件 |
| [离线规格索引](authoring-manifest.json) | 12 项稳定 ID、具体类、原网格哈希、静态容量、模型任务及现有源码映射；不是运行配置 |

跨武器／掩体的输入互斥、主动取消、初速和生命周期以[总交接共同规则](../offline-tech-prep-20261028/shared-contracts.md)为准；标定与要求核对见[总交接](../offline-tech-prep-20261028/README.md)。

## 已定下的交互

| ID | 武器 | 本轮定下的核心动作 | 原包静态容量参考 |
| --- | --- | --- | --- |
| W01 | Lancer | Grip 抓匣拉出、隐含释放 → 换匣 → 一组自然推入／合盖锁定 | 60，备用上限 600 |
| W02 | Hammerburst | 手开锁扣 → 换匣 → 手关锁扣 | 78，备用上限 702 |
| W03 | Gnasher | 缺弹时取弹开始，每轮最多补 3 发，可少装后闭锁；射后必须完成杠杆循环 | 8，备用上限 31 |
| W04 | Longshot | 开膛退壳 → 装一发完整实弹 → 闭锁 | 1，备用上限 23 |
| W05 | Snub | 抓匣拉出并隐含释放 → 换匣；空仓另拉放滑套，半仓免拉滑套 | 12，备用上限 60 |
| W06 | Boltok | 支架带弹巢开放 → 联动退空壳 → 逐发补缺 → 闭合，可部分装填 | 6，备用上限 24 |
| W07 | Boomshot | 开放枪管装填机构 → 插原装填组件 → 闭合 | 1，备用上限 11 |
| W08 | Torque Bow | 装一支原箭 → 扳机蓄力／松开发射 | 1，备用上限 11 |
| W09 | Hammer of Dawn | 武器指向目标 → 原条件锁定照射 → 松键停止 | 原函数报告无限弹药；无常规换弹 |
| W10 | Troika | 进入炮位 → 双握把转向／开火 → 原自动恢复 → 退出 | 不设计手动换匣 |
| W11 | Bolo / Frag | 持续 Grip 取用/持有 → 松 Grip 按手运动投出；2 kg（2026-11-08 用户覆盖） | 基类 MagSize 1、MaxSpareAmmo 3；不可直接当作 HUD 总数；[当前验证状态](../frag-grip-release-20261108.md) |
| W12 | Smoke | 同样手势投掷，独立烟雾库存、弹体及效果 | 同上，读取本实例实际库存 |

所有容量均由保存包的默认对象核对，不能覆盖运行时 Getter、难度、模式或具体变体的实际值。Snub 的空仓分支、Boltok 的逐发补弹等是本项目的 VR 设计选择；不是声称原版已经存在这些物理操作。

## 模型结论

12 件原枪／手雷主体均有可核对的来源，不需要重做 12 把整枪。Lancer、Hammerburst、Boomshot 的手持装填组件和 Torque Bow 的手持箭优先复用原独立资源；Snub 手持弹匣按原 `Magazine` 骨组建立独立组件方案。其他活动机构优先驱动原骨。

三款完整弹药现已生成 v2 GLB，均含两级 LOD、UV、PBR 材质与抓取／插入参考节点。Boltok 原弹巢两端封闭，已补做带六个真实贯通孔的替换件。Lancer 原创启动件现已在主线目录出现；本轮核对其生成结果，只复制进独立离线目录。Snub 原弹匣 314 三角也已在私有 analysis 拆出，保留 UV／法线／材质归属。没有重做整枪。

本次重核了 2 个保存资源包、12 个原网格、18 个默认对象，并从地图包定向导出 41 个类／函数对象。新增证据明确了 Boomshot 原装填件、Torque Bow 原手持箭、黎明之锤具体类与停止照射链。骨骼审计识别到 71 个可独立提取的刚性组，包含枪体；这不是 71 项补模工作。

## 完成边界与剩余事项

| 层级 | 本次状态 |
| --- | --- |
| 玩家动作与逐枪差异 | 12/12 已形成规格；输入、定额装填、提示和中断规则已写明 |
| 原模型来源与处理决策 | 12/12 已核对主体；独立件／原骨／补件路线已定 |
| 原版语义接入口 | 12/12 已映射具体类、保存函数和现有模块 |
| 骨名到机械部件 | Boltok Object03 支架、Object02 弹巢、Object08 支架子件、Object01 固定后盘已由层级／装配核对；原开合动画未在已查 21 个 AnimSet 中找到，80° 摇出仅为设计初值 |
| 抓点、枢轴、装填轴、行程数值 | 需要在确切实例与统一身体基准下标定；本次不给猜测数值 |
| native 调用 ABI、返回值替换与运行时所有权 | 公共换弹已有历史试验；各特殊入口不全具备可调用认证，逐项列在接入口表 |
| 新模型制作与文件检查 | 4 件新几何、8 个 LOD，加复用启动件，共 5 个 GLB 已交付；不是已认证的游戏比例或运行资产 |
| 模型加载、实际接线、游戏与头显验证 | 本次未执行；不计作完成 |

后续代码复用 `tools/vr-common/` 的逐枪状态机，新增薄的原游戏适配。先解决公共定额记账和原计时器／通知所有权，再接逐枪机械；完整弹药加载只选择一条共用管线；手雷共用速度估计，保留两套原弹体和效果。

实施交接单已经标明旧模块需要改的确切行为，包括 R3 字段、旧匣回池、部分装填退出以及手雷的最低速度／位移门槛。原创 GLB 的引擎资源创建／加载后端仍是必须编写的底层工作，已固定输入、组件生命周期与首件验证步骤；不能声称它已经具备可用 native 调用。交互设计不因此留待下次重开。

本轮离线范围已交付，后续需要实机才能确定的抓点、比例和状态调用单独保留，不占用主线构建或游戏运行窗口。实际合入仍交当前唯一协调执行者，沿用 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`、`codex/6dof-single` 及固定手动入口，不另建运行包。首版开始核对时源码 HEAD 为 `10510241b620b2960d79b9d1abc517e42c9c59b9`；这不是本套规格已经集成的版本号。

## 本地证据

源提取、默认值明细、骨组几何和来源哈希保存在忽略的 [analysis/weapon-specs-20261028](../../analysis/weapon-specs-20261028/)。`sources.json` 记录两包身份；`all-12-mesh-parts.json` 记录骨组归属；`explicit-defaults.json` 是正确的 18 对象解析；`map-script-provenance.json` 记录 41 项导出的来源和解析局限。带 `invalid-tuple-order` 的文件是保留的失败中间产物，不作为证据。

静态导出的脚本仍有 unresolved native token；5 个导出对象有数组等内联解析警告。只引用可确认的控制流、标量及对象引用，不把导出 `.uc` 当作可编译源码或认证过的零售 ABI。原包、提取几何和反编译内容不进入可发行资产目录。

v2 新证据集中在 `analysis/weapon-specs-20261028/review-v2/`：44 条定向函数引用、3 个 Bow 状态导出、21 个 AnimSet 的 Boltok 骨名匹配扫描、原位装配图、支架参考数据、Longshot 原空壳引用、Snub 私有拆件和最终文件验证记录。模型查看页因内置浏览器禁止本地 file URL 未完成 UI 实测；预览图已渲染检查。
