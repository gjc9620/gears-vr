# Ammunition model and 6 DIF prefill survey (DIF)2026-10-21）

This round will first complete the resource search, real geometry export and original reloading. Body sensor filling is not achieved, nor is it modified for normal starter, first person or game reloading. The resources are a copy of analysis from the current Reloaded installation package; these results cannot be extended to 2006 Full resource conclusion for all original versions.

## User-recognized targets

2026-10-22 Supplemented[Weapons 6DoF Interactive Specification](weapon-6dof-interaction-design-20261022.md), R-R-R-R-A-T-E-S.3 Start, shoot-by-gun yellow steps, insufficient number processing and time calibres selected by the user: from effective start to final necessary action complete, a perfect end of the original perfect window is perfect and valid before the beginning of the window. The starting point below “defended” is based on new documents; actual thresholds and mechanical actions for each gun still need to be checked.

The left hand removes the full cartridge, revolver or shotgun ammunition of the corresponding weapon from the left shoulder and loads it into a right hand-held gun, then executes the final bolt/loading. Complete the entire set of actions within the specified time frame triggers a perfect reload; complete the original reload without triggering failure/crash punishment. Time length, starting point and final mechanical action of each weapon are determined when the interactive design and actual measurements are made.

The current priority is to find the original complete munition in hand and not to use empty shell or tracer models as full ammunition that has been found.

## Exported resources

All products are left behind. `analysis/ammo-reload-20261021/` , does not add Mod release packages. Each provides OBJ, GLB, original package export Hash, Bohashi, offset, parsing layout and classification description.

| Resources | Conclusion for the current round | Body-loading use |
| --- | --- | --- |
| `COG_AssaultRifle.Assault_Rifle_Magazine` | Lancer's separate cartridge,40 Triangle;old weapon `MagazineMesh` Component quotes this resource | Source geometry as capture/insert prototypes, close-range details, stickers and interactive points still need to be addressed |
| `Locust_Hammerburst.Hammerburst_Magazine` | The video shows the number of people in the country.158 Triangle;old weapon `MagazineMesh` Component quotes this resource | Ibid. |
| `Locust_Boomshot.Mesh.Locust_Boomshot_Magazine` | The blog also highlights the impact of the project on the country's population.124 Triangle | specible shape; current round does not prove running time load binding |
| `Locust_Boomshot.locust_boomshot_ammo2` | The independent ammunition naming resource,268 Triangle, exported physical shape | Unaccompanied handheld filler not yet recognized as applicable |
| `Locust_Hammerburst.Mesh.S_Hammerburst_Shell` | Independent Shell Naming Resource,72 Triangle | Use is not nuclear, and can not be considered a Boltok munition. |
| `Delta_Assets.shell_spent_single_02b_SMesh` | Visually confirmed open-mouth empty shell; Gnasher shell particle module directly referenced | It's not a complete unlaunched shotgun. |
| Four. `bullet_spent_single*` Resources | Visual confirmation shell, citation of Lancer/Snub/ Hammerburst/ Longshot shell module | Not a full warhead at this time. |
| Three. `AmmoPickups` Resources | Pick-up of Magazine, Case, Box; source geometry display/ground level | It can't be directly equivalent to a magazine or a shotgun. |
| Four surface shell casings scattered resources | Environmental grid of multiple empty shells | Not as a single full-fired munition |

Total 17 A real resource. This round covers the main resource package `WarfareGame.u` and saved `MP_WarMachine`、`MP_Mansion`、`MP_Rooftops` Three map lists; currently unrecognized **Boltok Single, Independent and Complete Ammunition** and **Gnasher complete unlaunched shotgun**And that's not proof of the whole game.

Then we'll check all the machines. **1,034 Delocalize `.u/.upk/.war` Name list of the package**, the full decompression of the piece that is attached to the name list is successful. Only the compressions are unpressed and not all maps or game packages are fully developed. The names of ammunition, cartridges, warheads, magazines, cartridge/roll/slug and both target weapons are retrieved without a single dedicated resource name. The title sheet is only a clue and does not prove that there is no complete ammunition in the randomly named grid; the full result is stored with the name chart Hash. `analysis/ammo-reload-20261021/global-name-scan.json`。

The name scan also found Comando Dom's skin. `Bullet_Strap`. The role package has been further analysed for copying, bone reading and physical examination of a role grid: the name appears in the role skeleton and is not exported from the StaticMesh; no complete release is currently identified from it that can be removed directly. Role checking files are also left in the caractor subdirectories of anallysis, which are not mixed 17 A model ammunition delivery package.

Gnasher has only Root, Main, and Lever reference bones; Boltok uses several generic Object names. It is not proven from this information that the individual munition can be removed directly; nor does the skeletal naming itself prove that the Gorge did not have ammunition.

If the original version of the single-shot ammunition is not found afterward, it can be based on existing shell casings and weapon fillings with a closed-locked shotgun and a warhead revolver. Such models should be clearly marked as new/modified models and cannot be manufactured directly into the original package.

## Identified original reloading process

Use saved UnrealScript/ Class Export Evidence to locate the entry, without adding an EXE indent or running time. See Hashi `analysis/ammo-reload-20261021/reload-evidence.json`。

- `WarWeapon.Reloading.BeginState` Records `ReloadStartTime`, clear the active reloading attempt sign and set the reloading fire mode;`PlayWeaponReloading` Starts the original aerobic re-alignment and end time.
- Normal Path is `EndOfReloadTimer → PerformReload → Active`。`PerformReload` Handle the ammunition back-up into weapons and not just play animation.
- Original Second Press by `Reloading.HandleButtonPress` Enter `DetermineActiveReloadSuccessOrFailure`. The latter only holds evidence **Native declaration**The precise determination of the inter-area and threshold values has not yet been verified.
- `WarWeapon.ActiveReloadSuccess` Including success awards and fire Mode 10／11 . A single call function cannot be used as a whole body sensor swap access at this time.
- `FailedActiveReload` Clear the active bullet incentive and choose failed fire mode 9; there are also local failures of feedback, failed animation and timing.
- `WarPawn.PistolJammed` It'll be transferred. `Weap_PistolBase.Jammed`. Save scripts have not proven that the active switch failed to complete the connection of the event, and it cannot be asserted that all card cases are eliminated by skipping a single failure function.
- Pawn Exists `Notify_AmmoGrab`、`Notify_AmmoRelease`、`Notify_AmmoThrow`、`Notify_AmmoReload` Forward entry; the saved script corresponding to WarWeapon base class is empty and cannot be declared as providing full generic capture/plug interaction.

User vision is achieved by the independent perception phase: after the taking, insertion, final mechanical action has been completed, then selection of a successful reward or regular filling submission based on time-consuming hours. Timeout branches do not enter the original active re-loading failure decision. This is done only if they verify their primary state, normal automatic completion of timing and recording of ammunition, so as to avoid automatically loading the original clock before they are loaded; this round records only the design without modifying these paths.

## Export and Authentication

Use existing UE 3 Label Reader and press UEVIewer `GAME_GoWU` The static grid sequenced branch produces a border reader; unknown structures fail directly and do not scan geometry. The physics package version 835/76, with a valid version of GoWU 614 Path; KDOP, section, position flow, UV stream and two index streams are read by structure.17 Keep all entries 56 Byte does not explain the end, and does not count it as geometry. The format is as follows: [UEVIewer Official Warehouse](https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealMesh/UnMesh3.cpp), downloading sha 256 Yes `404f6f5f11e56f8d8cdaa7d85131bd98d161f155214d18a78a73b1c83a1211cd`. No analytical tool has been installed or replaced.

Validation results:17/17 Source Export SHA 256 Matching;17/17 OBJ, GLB aligns to the embedded location/index within the web page; triangle index range, section face number, limited coordinates, GLB length and buffer border pass; Node of JavaScript `--check` The page is open for external requests.

Auto Open in Browser `file://` The preview page was rejected by the URL security policy and did not be bypassed by other browsers, service forwarders or indirects. HTML is delivered as a local file, actual WebGL operations in the browser have not been completed. All models are shown with neutral ash, no posters are exported; original dimensions are not calibrated to VR hands, guns or plug-in mouths.

Main products:

- `analysis/ammo-reload-20261021/ammo-viewer.html`
- `analysis/ammo-reload-20261021/ammo-priority-previews.png`
- `analysis/ammo-reload-20261021/ammo-previews.png`
- `analysis/ammo-reload-20261021/export-manifest.json`
- `analysis/ammo-reload-20261021/particle-mesh-references.json`
- `analysis/ammo-reload-20261021/resource-coverage.json`
- `analysis/ammo-reload-20261021/global-name-scan.json`
- `analysis/ammo-reload-20261021/verification.json`
- `analysis/ammo-reload-20261021/ammo-models-17-local.zip`

---

## 中文

# 弹药模型与 6DoF 装填前期调查（2026-10-21）

本轮先完成资源查找、真实几何导出和原有换弹入口定位。尚未实现体感装填，也未修改正常启动器、第一人称或游戏换弹行为。资源来自本机 Reloaded 安装包的分析副本；不能把这些结果扩展为 2006 原版所有版本的完整资源结论。

## 用户确认的目标

2026-10-22 已补充[武器 6DoF 交互规格](weapon-6dof-interaction-design-20261022.md)，明确 R3 启动、逐枪黄色步骤、数量不足处理及用户选定的计时口径：从有效启动至最后必要动作完成，早于原版完美窗口终点即可完美，提前于窗口起点也有效。下文“留待确定”的起点与时间口径以新文档为准；每枪实际阈值及机械动作仍需核对。

左手在左肩取出对应武器的弹匣、左轮完整弹药或霰弹枪完整弹药，装入右手持枪，再执行最终拉栓／上膛动作。规定时间内完成整套动作触发完美换弹；超过规定时间仍正常装填，不触发原版换弹失败／卡壳惩罚。计时长度、起点和各武器最终机械动作留到交互设计与实测时确定。

当前优先级是找到可拿在手里的原版完整弹药，不能把抛出的空壳或曳光模型作为已找到的完整弹药。

## 已导出的资源

全部产物保留在忽略的 `analysis/ammo-reload-20261021/` 中，未加入 Mod 发布包。每项提供 OBJ、GLB、原包导出哈希、包哈希、偏移、解析布局和分类说明。

| 资源 | 本轮结论 | 体感装填用途 |
| --- | --- | --- |
| `COG_AssaultRifle.Assault_Rifle_Magazine` | Lancer 独立弹匣，40 三角形；原武器 `MagazineMesh` 组件引用该资源 | 可作为抓握／插入原型的源几何，近距离细节、贴图与交互点仍需处理 |
| `Locust_Hammerburst.Hammerburst_Magazine` | Hammerburst 独立弹匣，158 三角形；原武器 `MagazineMesh` 组件引用该资源 | 同上 |
| `Locust_Boomshot.Mesh.Locust_Boomshot_Magazine` | Boomshot 名下独立弹匣资源，124 三角形 | 可检查形状；本轮未证明运行时装填绑定 |
| `Locust_Boomshot.locust_boomshot_ammo2` | 独立弹药命名资源，268 三角形，已导出实物形状 | 尚未认定为适用的单发手持装填件 |
| `Locust_Hammerburst.Mesh.S_Hammerburst_Shell` | 独立 Shell 命名资源，72 三角形 | 用途待核，不能认作 Boltok 完整弹药 |
| `Delta_Assets.shell_spent_single_02b_SMesh` | 已目视确认开口空壳；Gnasher 抛壳粒子模块直接引用 | 可作霰弹壳外形参考，当前不是完整未发射霰弹 |
| 四个 `bullet_spent_single*` 资源 | 已目视确认空壳，Lancer／Snub／Hammerburst／Longshot 抛壳模块分别引用 | 当前不是带弹头的完整弹药 |
| 三个 `AmmoPickups` 资源 | 拾取用 Magazine、Case、Box；源几何含展示／地面平面 | 不能直接等同于枪上对应的弹匣或散弹 |
| 四个地面弹壳散落资源 | 多枚空壳组成的环境网格 | 不作为单发完整弹药 |

共 17 个真实资源。本轮覆盖主资源包 `WarfareGame.u` 及已保存的 `MP_WarMachine`、`MP_Mansion`、`MP_Rooftops` 三份地图清单；目前未确认 **Boltok 独立完整单发弹药** 与 **Gnasher 完整未发射霰弹**。这不是全游戏绝对没有这些模型的证明。

随后补查本机全部 **1,034 个非本地化 `.u/.upk/.war` 包的名称表**，全部解析成功。只解压与名称表相交的压缩块，没有完整展开所有地图或修改游戏包。检索弹药、弹壳、弹头、弹匣、cartridge／round／slug 及两种目标武器相关名称，未发现明确的两种武器完整单发专用资源名。名称表只是线索，不能证明任意命名的网格中没有完整弹药；全量结果与名称表哈希保存在 `analysis/ammo-reload-20261021/global-name-scan.json`。

名称扫描还发现 Commando Dom 皮肤的 `Bullet_Strap`。已进一步对该角色包做分析副本清单、骨骼读取及一份角色网格的实物检查：该名称出现于角色骨骼，不是独立 StaticMesh 弹药导出；目前没有从它确认可直接拆出的完整一发。角色检查文件另留在 analysis 的 character 子目录，没有混入 17 个弹药模型交付包。

另外检查了已导出的 Gnasher、Boltok 武器骨骼及焊接后的连通部件。Gnasher 只有 Root、Main、Lever 三个参考骨；Boltok 使用多个通用 Object 名。未从这些信息中证明可以直接拆出独立完整弹药；骨骼命名本身也不能证明网格里没有弹药。

如果后续仍没有找到完整原版单发弹药，可以基于已有弹壳外形和武器装填口补建封口霰弹、带弹头左轮弹。此类应明确标为新建／修改模型，不能冒充原包直接导出。本轮没有制作这些替代模型。

## 已定位的原有换弹流程

使用保存的 UnrealScript／类导出证据完成入口定位，没有新增 EXE 反编译或运行时验证。文件哈希见 `analysis/ammo-reload-20261021/reload-evidence.json`。

- `WarWeapon.Reloading.BeginState` 记录 `ReloadStartTime`，清除主动换弹尝试标志，设置换弹 fire mode；`PlayWeaponReloading` 启动原有换弹动画与结束计时。
- 普通路径为 `EndOfReloadTimer → PerformReload → Active`。`PerformReload` 处理备用弹药转入武器，不只是播放动画。
- 原版二次按键由 `Reloading.HandleButtonPress` 进入 `DetermineActiveReloadSuccessOrFailure`。后者在保存证据中只有 **native 声明**，精确判定区间和阈值尚未核实。
- `WarWeapon.ActiveReloadSuccess` 包含成功奖励及成功 fire mode 10／11 的选择，Pawn 侧另有 HUD、统计和反馈处理。当前不能把单独调用某个函数当作完整体感换弹接入已成立。
- `FailedActiveReload` 清除主动换弹奖励并选择失败 fire mode 9；另有本地失败反馈、失败动画和计时处理。
- `WarPawn.PistolJammed` 会转给 `Weap_PistolBase.Jammed`。保存脚本尚未证明主动换弹失败到该事件的完整连接，不能断言只跳过某一个失败函数就消除了所有卡壳状态。
- Pawn 存在 `Notify_AmmoGrab`、`Notify_AmmoRelease`、`Notify_AmmoThrow`、`Notify_AmmoReload` 转发入口；WarWeapon 基类对应的已保存脚本是空实现，不能据此宣称原版已提供完整通用抓弹／插弹交互。

用户设想可以按独立体感阶段实现：取弹、插入、最终机械动作完成后，再根据耗时选择成功奖励或普通装填提交。超时分支不进入原有主动换弹失败判定。实现时仍须核实原生状态、普通自动完成计时和弹药记账的配合，避免手还没装入就被原计时器自动加弹；本轮只记录设计，未修改这些路径。

## 导出与验证

使用现有 UE3 标签读取器，并按 UEViewer 的 `GAME_GoWU` 静态网格序列化分支制作有界读取器；未知结构直接失败，不扫描猜测几何。物理包版本 835/76，采用既有 GoWU 有效版本 614 路径；KDOP、section、位置流、UV 流和两份索引流均按结构读取，17 项都保留 56 字节未解释尾部，没有把它算作几何。格式参考为 [UEViewer 官方仓库 UnMesh3.cpp](https://github.com/gildor2/UEViewer/blob/master/Unreal/UnrealMesh/UnMesh3.cpp)，下载参考 SHA256 为 `404f6f5f11e56f8d8cdaa7d85131bd98d161f155214d18a78a73b1c83a1211cd`。未安装或替换分析工具。

验证结果：17/17 源导出 SHA256 匹配；17/17 OBJ、GLB 与网页内嵌位置／索引逐项一致；三角形索引范围、section 面数、有限坐标、GLB 长度与 buffer 边界通过；JavaScript 的 Node `--check` 语法检查通过。预览页没有外部请求。实际源几何的静态多角度图已经目视检查。

自动在浏览器打开 `file://` 预览页被 URL 安全策略拒绝，没有改用其他浏览器、服务转发或间接方式绕过。HTML 作为本地文件交付，浏览器内实际 WebGL 操作验收尚未完成。所有模型使用中性灰显示，未导出贴图；原始尺寸未校准到 VR 手、枪或插入口。

主要产物：

- `analysis/ammo-reload-20261021/ammo-viewer.html`
- `analysis/ammo-reload-20261021/ammo-priority-previews.png`
- `analysis/ammo-reload-20261021/ammo-previews.png`
- `analysis/ammo-reload-20261021/export-manifest.json`
- `analysis/ammo-reload-20261021/particle-mesh-references.json`
- `analysis/ammo-reload-20261021/resource-coverage.json`
- `analysis/ammo-reload-20261021/global-name-scan.json`
- `analysis/ammo-reload-20261021/verification.json`
- `analysis/ammo-reload-20261021/ammo-models-17-local.zip`
