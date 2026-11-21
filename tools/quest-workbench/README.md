## Adopt real left-hand captures into Lancer source (2026-11-12)

`adopt_lancer_captures.py` is a separate offline source-authoring path, distinct from the older fingers-only `transfer.py` V2 editor export. It selects ONLY each archive's capture.part/capture.side, requires four calibrated, unobstructed, fully tracked left snapshots for the exact original Lancer/Marcus model, and preserves the right main grip. It converts all15 finger quaternions AND the weapon-local wrist, including reflected object/wrist bases. Magazine wrists become item-local using the exact detached-magazine baked basis; C is unposed from its captured deployment before following the actual current angle/stroke; E follows its actual slide. Capture volumes are retained in the raw archives, not imported by this operation.

Example (choose a new output folder; raw archives are never overwritten):

```powershell
python tools/quest-workbench/adopt_lancer_captures.py --captures <capture-directory> --output <new-review-directory> --apply
```

The generated `weapon_lancer_owner_grabposes.h`, paired left finger tables and the Lancer support/magazine/E/C consumers form one source change. The operation preserves the right-hand SHA and refreshes source mod-baselines.json after adoption. Do not apply an old baseline's finger-delta export on top of the new compiled baseline. This does not install an APK, start a game, replace an active editor, or prove collision-free full arms/gameplay/headset fitting. Original independent capture IDs/SHA and previous source tables are kept in the review directory.

For0.3 captures carrying `part_delta_known`, use `adopt_moving_lancer_captures.py` with the same
arguments. It removes the full captured moving-part transform, then calls the unchanged legacy
selection/tracking validation and conversion. The original adopter remains frozen for existing
source-adoption consumers; calling it directly on a0.3 moving-part archive is unsupported.

# Quest weapon workbench

Private standalone Quest3/3S OpenXR authoring and controller practice application.
Current candidate: J:/gearsVr/candidates/quest-workbench-20261111, version0.4.0-dev, build13.
The Windows companion is Quest Workbench.exe. See 使用说明.txt for the complete Chinese guide.

0.3 removes runtime recording/history/replay. Legacy core.cpp/legacy_recording.h and desktop .take
readers are archival compatibility only, excluded from Android and host build targets. Single real
hand capture, independent capture archives, retarget/FK, current-pose overview and finger/wrist edits
remain. No controller-derived or authored preview is labelled a real26-joint hand observation.

The edit/practice loop uses A/X to return without restarting. Five colored acquisition regions and
part controls remain; the inspector defaults pinned60cm to the side and can be set45..120cm away.
A library preview can be explicitly adopted for testing. Stored real captures override authored defaults.
Each untouched default is identified as authored_default, with no tracked flags/source/time claim.

Controller practice implements main pickup, two-hand support, gun-owned25m sight zero and10m target
hits, installed-magazine extraction, old-magazine reinsertion, left-shoulder replacement supply,
assisted insertion, empty reload/observed E return and original C rail/full-travel/held motor/first-real-
release3s tail. Focus, tracking, time/space discontinuities clear held ownership and require a fresh
press. Grip requires actual OpenXR position/orientation tracked+valid flags15; aim remains display-only.
Menu hover suppresses trigger shooting and requires release before resuming fire.

practice.h is a standalone scene adapter around exact frozen portable Mod headers in game_rules/.
freeze_rules.py pins their transitive include bytes; assets/game-rules.json lists37 header hashes (the prior36 plus the Gnasher lever kernel).
These are the same geometry/state kernels, not the original engine, hooks, live actor identities,
inventory writer, animations, physics, recoil or audio. The shoulder uses the default body_slot layout
with stable initial/workspace yaw and head translation as a body-position proxy, adjustable center/scale.
The no-body-tracker limit and need to recenter after turning remain explicit.

Shooting cadence, visual recoil, vibration gain and quiet synthesized AAudio sounds are adjustable
PREVIEW parameters. The firing simulation does not prove the original game has fired. C uses simulated
scene acknowledgments paired with actual sandbox render submissions; those never claim game engine
execution receipts. E waits for visible mechanism acknowledgments and the shared return sequence.
Shared stroke speed, travel, timing, identity and fresh input rules stay unmodified.

Original geometry is preserved: body10798 + receiver1300 + C1089 + E54 + magazine156 =13397 triangles.
The detached magazine is the original bone8 partition. Full Lancer.glb remains byte-identical to0.2.0.
Original vertices are transformed by A=(-X,Z,-Y)*.01 with reversed winding. Marcus wrist geometry/
reference uses S=diag(-1,1,1); wrist transforms use A*R*S. Current poses follow moving-part deltas,
including C deployment/pull, E travel and detached magazine. Legacy capture fields remain supported.
General part_delta_in_object permits offline adoption to remove the actual captured part transform.

Build Android: py tools/quest-workbench/build.py --dependencies PINNED_DEPS --output FRESH_OUTPUT.
Isolated pinned NDKr27c, Android35 packaging tools/platform, Android29 minimum/32 target and Khronos
1.1.63 loader remain. No Java/Gradle application layer or development keystore is shipped.
prepare_assets.py uses retained hash-checked retail geometry and numpy/Pillow. Materials remain clay.
Rendering retains1.15 recommended dimensions,2xMSAA, batched text and instanced debug primitives.

Host checks: CMake/MSVC builds WorkbenchCoreTest and WorkbenchPracticeTest. Run the former with a
fresh fixture directory, the latter with assets/Lancer.parts.json. Practice trajectories cover cold
press, menu suppression, source loss, volume/support release, stable shoulder, tactical and empty
reload/replacement/insertion/fire, old magazine restore, short/full/observed E, C no-gesture/no-render
rejection, indefinite hold, release tail, reverse stop/restart and tracking loss. They are CPU fixtures,
not physical headset acceptance. Python conversion checks: py -m unittest discover -s tools/quest-workbench
-p test_transfer.py -v. Four retained real captures also round-trip through arbitrary moving-part deltas.

Saving project.json/model.project.json preserves poses/regions, shoulder settings, practice feedback
parameters and panel placement; interaction-settings.json is a companion settings copy. Exports keep
all archives. The existing desktop V2 editor conversion remains finger-only. The separate offline
adopt_lancer_captures.py source-authoring flow described above handles selected full wrists/fingers.
New region/snap/shoulder/preview values are not automatically installed into the Mod. A later explicit
Mod configuration integration and real-game comparison are required for complete parity.

No PC game or simulator is started for this workbench change. No full-arm IK, generic imported model
rig/animation retargeting, textures, enemies, executions, simulated weight or collision acceptance is
claimed. A successful compile/host check is separate from Quest startup, UI visibility and owner feel.


0.5.0/build14 extends the twelve-model catalog with curated interactive roles, editable starting
poses and standalone per-weapon practice. armory_practice.h implements the requested video/user
reload policies, including Boltok group deficit loading + fresh Grip closure, Gnasher right-down/
return, Hammerburst no charge, Longshot lift/pull/push/lock, Snub empty slide, Boomshot carrier and
Bow release fire. Beam, turret and grenade previews remain distinct local approximations.
Yellow next-step guides default on and persist with project data. Main UI is weapon picker -> part
chips -> two capture buttons + large test button; advanced controls collapse. Part chips preview
current stored poses. Switching preserves workspace heading and grip contact. No recording returns.
Generic profiles are exported as workshop_profile; Mod import is not automatic.

Asset pipeline: prepare_catalog.py -> prepare_workshop.py -> prepare_assets.py, using the pinned
numpy/Pillow Python runtime. Original surfaces/bone inventories remain conserved in the retained
meshes. Boltok rendering uses the documented authored open-cylinder replacement; arrow is an
explicit authored preview. Cached Boomshot vtx60 positions are ALREADY METRES; apply .75 player
scale, not another .01. test_catalog.py checks geometry, curated roles, ammo bounds and atlas.
WorkbenchArmoryTest adds eleven non-Lancer routes and yellow-stage/input-loss checks. Native UI
and physical hand fit still require actual Quest inspection, distinct from these CPU fixtures.


0.5.1/build15 removes revolver/pistol fore-end editor entries. Both sides use the same main-grip
pose/volume; support does not steer aim via the tiny clasp baseline. Internal legacy support slots
remain for archive compatibility but cannot be selected as independent editor parts. Gnasher lever
is not independently grabbed and its editor entry is hidden. No quickloader plate is rendered;
revolver supply remains grouped loose rounds with the unchanged C-L-S deficit accounting.
Ammo entries are labeled 子弹: selecting hides all gun meshes, all other acquisition regions and
non-interactive aliases. It displays only the actual reload item (box magazines for magazine-fed
guns), keeping its original object frame and capture schema. Captures feed the existing ammo
wrist/finger route. Gun/parts return on selecting a different part. Data is not deleted or relabeled.
preview_captures.py is read-only: reconstructs Marcus LBS from stored finger quaternions and wrist,
with actual captured component transforms. Inline preview uses explicitly simplified connected
surfaces; raw26 tracked joints can be compared. It is not a headset screenshot/contact acceptance.


0.5.2/build16 owner correction: support remains an independent Second Hand entry, including
revolver and Snub. Its volume, wrist, fingers and archive identity remain support, never aliasing
main_grip. Existing support captures are used directly; automatic copying into main-left is removed.
shared_grip now only prevents short-baseline two-hand steering for pistols. Starting support points
for pistols are near the grip; other weapon support points remain unchanged. Original capture labels
are retained as history. Previously rejected three archives stay excluded from the22-entry curated
dataset; no deleted files are restored. Ammo-only editing and no quickloader plate remain unchanged.


0.5.3/build17 uses the whole six-chamber cylinder as the revolver reload item, with its own
feed-relative mesh exactly aligned to the mounted replacement cylinder. No single rounds/loader
plate are rendered as the revolver item. The mounted cylinder is hidden while the replacement is
held; insertion restores it. C-L-S accounting still transfers only the actual missing rounds.
The editor exposes 装填弹巢 separately from the mounted 弹巢 operation pose.
Isolated ammo can be picked up with either valid controller Grip, translated/rotated, released to
fix placement, summoned in front of the operating hand or reset. Editor pickup uses padded actual
mesh bounds independent of authored gameplay volumes. The hidden object frame follows the item,
so stored wrists/raw joints/fingers stay object-local. The gun world placement is restored before
leaving the item editor, switching weapons or entering practice. Held-item capture is rejected.
Real captures on all other weapons remain unchanged. CPU global rigid-transform capture invariance
and cylinder-seat geometry/visibility/deficit tests cover this change; Quest feel remains separate.

---

## 中文

## 将真实的左手抓取输入兰瑟源(2026-11-12)

`adopt_lancer_captures.py` 是一个单独的离线源作者路径, 不同于老的手指 `transfer.py` V2 编辑器导出。 它只选择每个归档的抓取. part/ capture. side, 需要四个校准, 无障碍, 完全跟踪的左快照, 用于精确的原始 Lancer/ Marcus 模型, 并保存右主抓取。 它转换全部 。15 指尖和武器局部腕,包括反射物体/手腕。 E 遵循其实际幻灯片。捕获量保留在原始档案中,而不是由这一操作导入。

示例( 选择新输出文件夹; 原始档案从不覆盖) :

```powershell
python tools/quest-workbench/adopt_lancer_captures.py --captures <capture-directory> --output <new-review-directory> --apply
```

生成的 `weapon_lancer_owner_grabposes.h`,对齐的左指表和Lancer支持/magazine/E/C消费者构成一个源变化。操作保留右手的 SHA ,并在通过后刷新源 Mod-碱.json 。不要在顶端应用旧基线的指代导出 。e 已编译新基线。 这不会安装 APK 、 启动游戏、 替换活动编辑器, 或证明没有碰撞的全臂/ 游戏/ 头集配置。 原始的独立捕获 ID/ SHA 和以前的源表会保存在审查目录中 。

For0.3 携带的捕获 `part_delta_known`,使用 `adopt_moving_lancer_captures.py` 同样的
参数。它删除了全部抓取的移动部分转换,然后将未变的遗留物称为
选择/跟踪验证和转换。原采用者仍然被冻结,用于现有的
源代消费者;直接使用0.3 移动部分存档不支持 。

# 搜索武器工作台

私人独立查询3/3 S OpenXR 编写和控制器练习应用程序.
现任候选人: J:/gearsVr/candidates/quest-workbench-20261111, 版本0.4.0- 德夫,建楼13.
The Windows companion is Quest Workbench.exe. See 使用说明.txt for the complete Chinese guide.

0.3 删除运行时间记录/ 历史/ 重播。 legacy core. cpp/ legacy_ recording. h 和桌面 . take
读取器仅是档案兼容性,不包括在Android和主机构建目标中。
手抓、独立抓取档案、重新瞄准/FK、当前拥有的概览和手指/手抓编辑
。没有从控制器中生成或编写的预览标记为真实26- 手相观察

编辑/练习循环使用 A/X 返回而不重新启动。
部分控件仍保留; 检查员默认值被锁定60 cm 娩杆45..120 离这里很近
库预览可以被明确采用用于测试。 stocked real caption over authored default.
每个未触碰的默认被确定为作者_默认,没有跟踪标记/源/时间要求.

主计长的做法主要负责两手支援,枪械拥有25 视零和10 m 目标
恢复使用、安装磁带、旧磁带再安置、左肩替换供应、
协助插入、空载重载/观察 E返回和原始C型铁路/全程/持有的发动机/第一-实载
release3 重点、跟踪、时间/空间不连续,明确拥有所有权,需要新鲜的
。 Grip 需要实际的 OpenXR 位置/方向跟踪+有效旗15;目标仍然只显示。
菜单徘徊抑制了触发射击,要求恢复射击前释放.

practice.h是游戏_rules/中围绕精确冷冻的便携式Mod头的独立的场景适配器.
rois_rules.py 标注其过渡性包括字节; 资产/游戏规则.json 列表37 头部散列( 前一个36 加上Gnasher的杠杆内核。
这些是相同的几何/状态内核,而不是原始引擎,钩子,活演员身份,
库存写入器、动画、物理、后坐力或音频。肩部使用默认的正文_slot布局
具有稳定的初始/工作空间yaw和头翻译作为体位代理,可调整的中心/尺度.
无人跟踪器的限制和转弯后需要更新,这一点仍然明确。

射击腔,视觉后坐力,振动增益和静静合成的AODIO声音是可调节的
PREVIEW 参数。 射击模拟不能证明原游戏已发射。 C 使用模拟
场景确认与实际沙盒对齐的提交; 那些从未声称游戏引擎
E 等待可见的机制确认和共享返回序列。
共享中风速度,旅行,计时,身份和新输入规则保持不变.

原几何保存: 体10798 + 接收器1300 + C1089 + E54 + 杂志156 =13397 triangles.
脱落的杂志是原始的骨骼8 分区。完整 Lancer.glb 仍然与0.2.0.
原顶点由A=(-X,Z,-Y)*转化而成.01 马库斯手腕几何/
引用使用 S=diag( -)1,1,1);腕部变换使用A*R*S. 电流的姿势跟随移动段三角洲,
包括C部署/推力、E旅行和脱落杂志。
常规部分_delta_in_object允许离线通过,以删除实际捕获的部分变换.

构建 Android: py tools/quest-workbench/build.py -- 依赖性 PINNED_DEPS -- 输出 FRESH_UTPUT。
隔离式NDKr27 c, Android 软件35 packaging tools/platform, Android 软件29 minimum/32 目标与赫罗诺斯
1.1.63 装入器仍保留。 没有 Java/ Gradle 应用程序层或开发密钥托尔 。
准备_资产.py 用途 保留经hash检查的零售几何和数字/平面。材料仍然是粘土。
保留了渲染1.15 建议的层面,2 xMSAA,批量文本和实例调试原始.

主机检查: CMake/ MSVC 构建 Workbench Core test and Workbench 实践测试。 运行前一个测试时使用
新的固定目录,后者有资产/Lancer.parts.json。
压力、菜单压制、源头损失、量/支持释放、稳肩、战术和空
重新装入/更换/插入/火力,旧杂志恢复,短/全/观察E、C无年龄/不投降
拒绝、无限期搁置、释放尾巴、逆向停止/重新启动和跟踪丢失。它们是CPU固定装置,
物理耳机接受。 Python 转换检查: py - m unitest 发现 -s tools/quest-workbench
-p test_ transfer.py -v. 四条保留了真实的捕捉,也通过任意移动的段三角洲进行圆盘捕捉.

保存工程.json/model.project.json保留了姿势/区域,肩部设置,练习反馈
参数和面板位置; 交互设置.json是伴奏设置副本。导出保存
全部归档。现有的桌面 V2 编辑器转换仍只使用手指。 单独离线
采用上描述的_lancer_captures.py源授权流,将选中的全腕/指针。
新的区域/snap/肩/审查值不会自动安装到模块中。
完全对等需要Mod配置集成和真游戏比较.

无法启动此工作台更改的 PC 游戏或模拟器。 没有全臂 IK , 通用导入模式
钻机/动能重新瞄准目标、纹理、敌人、处决、模拟重量或碰撞接受度
。成功编译/主机检查与查询启动、用户界面可见度和所有者感觉不同。


0.5.0/build14 扩展十二式目录,并设置可编辑的互动角色,开始
配置并单独进行每件武器练习。
重新装入策略,包括 Boltok 组的负载不足 + 新鲜 Grip 关闭, Gnasher 右下/
返回、锤子无电、长射升降/推力/推力/锁、Snub空滑动、Boomshot载体和
弓释放火. 束,炮塔和榴弹预览仍然是独特的局部近似.
黄色的下一步导线默认并持续使用项目数据。 主UI 是武器拾取器 - > 部分
芯片 - > 2 捕获按钮 + 大测试按钮; 高级控制崩溃。 部分芯片预览
当前存储的姿势。切换会保留工作空间的向导和抓住联系人。没有录制返回。
一般配置文件以工作坊_ profile 输出; Mod导入不是自动的 。

资产管道: 准备_ catalog.py - > 准备_workshop.py - > 准备_资产.py, 使用绑定
数字/平面 Python 运行时间。原始表面/骨骼清单在保留期内仍然保存
meshes. Boltok 渲染使用文件所记录的开圆筒替换;箭头是
已明确作者预览。 Cached roomshot vtx60 职位是 ALHEADY METRES; 应用。75 player
缩放,不是另一个。01. test_catalog.py检查几何,校准角色,弹药界限和地图集.
工作室武器试验增加了11条非蓝皮路线和黄相/输入-损失检查。
和物理手合体仍然需要实际的查询检查,不同于这些CPU固定装置。


0.5.1/build15 删除左轮/手枪前端编辑器条目。 双方使用相同的主语句
状态/数量;支持不通过小的空间基准引导目标。
保留存档兼容性,但不能选择独立编辑器部件。 Gnasher 杠杆
无法独立抓取,其编辑器条目被隐藏。没有提供快速装填机的盘片;
左轮手枪供应仍然按散弹分组,C-L-S赤字核算不变。
Ammo entries are labeled 子弹: selecting hides all gun meshes, all other acquisition regions and
非交互化名。它只显示实际重装项目(供杂志编辑的盒式杂志)
枪,保留其原始对象框架并捕获图。
手腕/ 指针路由。 枪/ 部件在选择不同部分时返回。 数据不会被删除或重新标记 。
预览_抓取.py是只读的:从存储的手指四角和手腕重建Marcus LBS,
内置预览使用明确简化的连接
表面; 生质26 可比较跟踪关节。 这不是一个耳机截图/接触接受度 。


0.5.2/build16 主人更正:支持仍然是独立的第二手条目,包括
左轮手枪和Snub. 其体积、手腕、手指和档案身份仍然支持,从未变名
主格子。 直接使用现有的支持抓取; 自动复制到主左侧被删除 。
共享_ grip 现在只防止手枪的短线双手方向盘。 开始支持点
手枪的握手位置; 其他武器支援点保持不变。
三个档案被否决后,仍被排除在《公约》之外。22- 入院验收
数据集; 没有恢复删除的文件。 仅编辑 Ammo 和没有快速装填机板块不变 。


0.5.3/build17 将整台六室圆筒作为左轮转装弹器,并带有其自己的
与安装的更换气瓶完全一致的网格。无单发/装弹机
板块作为左轮手枪项目进行渲染。挂载的圆柱是隐藏的,替换时是隐藏的。
已保存;插入会恢复。 C-L-S 会计仍然只传输实际缺失的回合。
The editor exposes 装填弹巢 separately from the mounted 弹巢 operation pose.
隔离弹药可以使用有效的控制器Grip、翻译/旋转、释放到
固定放置,在操作手前传呼或重置。编辑器的皮卡使用加法实际
网格边界独立于作者游戏游戏卷。隐藏对象框跟随项目,
因此,枪手/拉动关节/枪枝的手腕保持物体局部。枪械世界放置在枪械世界放置之前恢复。
离开项目编辑器、切换武器或进入练习。拒绝捕获已持有的项目。
所有其他武器的实际捕获情况保持不变。
和圆柱座几何/可见/缺陷测试覆盖这一变化; 查询感觉仍然分开。
