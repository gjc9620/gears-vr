## 2026-11-18 部件关系修正交付（contact02）

已按Quest本轮只读回拉的Boomshot ammo/left档案1794499171798-4恢复完整手/弹巢关系：逆转工作台A=(-X,Z,-Y)、.75缩放、centroid和feed，得到原MagazineMesh source-local手腕位置(.12446883397,-.10665401194,-.10103154195)m和旋转；同一档案的15指姿势同步采用。导入脚本校验model/hash/part/side/source time，拒绝误用Gnasher/Boltock来源。主手恢复本轮Boomshot项目authored模板，明确不是实录主手，不再冒称跨武器采集已导入。

contact01左手外沿接触、一次换弹和两发成功，但主手模型到Aim基变换漏项使枪横向偏90°，该候选被拒绝。恢复既有mesh_in_aim后，contact02原速画面已亲看持枪、持弹巢、插入、编码后字幕；左手包住弹巢外沿，枪恢复朝前，实际渲染枪管俯仰0°。普通换弹14.810691秒包含主动持弹观察停留与对准，原组件入座后实际返射，两发Proj_Boomer。无上膛，Second Hands未恢复，未重跑完美换弹矩阵。

PG/Native编译均exit0；本次实例boom-contact02-1118-014243 driver/stop exit0；isolated-done确认所属game/children正常退出，forced_termination=false。Quest只读拉取，未安装或启动应用。本轮未做实体头显手感验收。

25秒原速字幕视频：analysis/boomshot-20261118-contact/Boomshot-按工作台部件关系抓握.mp4。接触定帧：video02/check-contact.jpg；证据：delivery-review.json；不可变DLL和manifest：candidate。旧17轮跨武器数据结论已撤回，下面保留历史。公共v71/固定入口未更新，无新Git提交。

---
## 2026-11-18 用户接触截图后的重要纠正

撤回下方17轮“真实Boomshot主持枪1794486454128-2及ammo1794500066511-3”的结论：前者顶层model为Gnasher.glb，后者为Boltock.glb。只按部件标签/左右手筛选而漏检武器model/content hash，导致跨武器档案误用。运行次数、入座/奖励/炮弹回执不能证明抓握几何正确，先前goal完成状态过早。

本轮通过ADB只读拉取Quest实际Boomshot项目和28档案到analysis/boomshot-20261118-contact/quest-readback，未启动、安装或修改头显应用。Boomshot项目实际采用ammo1794499171798-4；main-right仍authored_default，无本轮已回读的真实Boomshot主手采集。必须移除跨武器主手，不能再称其为已采集导入。

更重要的关系链：ammo编辑显示centered boomshot-carrier.mesh，生成时把原M0缓存的米制顶点经反射轴变换和0.75玩家比例后减去质心，显示时再叠加part_delta和profile.feed。采集wrist/fingers必须相对于这份实际可见弹巢恢复，不能拿游戏gunRoot/seated骨代替它的部件坐标。独立三视图已按真实Marcus skin与实际ammo mesh/feed重建，手掌/手指确实包住外沿；图为offline workbench reconstruction，不是新游戏截图。源码修复与新基本运行状态见本轮contact目录后续报告。

---
## 最终补录17（2026-11-18）

交付视频：analysis/boomshot-20261117/delivery/Boomshot-真实抓点与完美换弹.mp4，39.24秒，1280×840，原速、中文三行字幕、受控Source1实际游戏渲染。复用build14，未改变枪/手/弹道代码。真实主持枪1794486454128-2、弹巢1794500066511-3关系及手指已生效；水平输入下实际渲染枪管模型+Y俯仰0°；三次原版炮弹起点与本次枪口记录相同，原散布和重力保持。已查看主手侧面、换弹接触、前方首发爆炸、换弹后收手及编码后字幕。

普通换弹8,327,396us，完美换弹3,260,090us，原奖励applied=1，三次实际Proj_Boomer与两次返射成功。无上膛动作。Second Hands继续暂缓。driver_exit=0、stop_exit=0，isolated-done确认owned_game_absent=true、无所属子进程、forced_termination=false。无新物理头显验收。

补录15/16失败保留：15新弹巢已取得但未入座；16左肩取得失败。日志可见15插入授权曾因100ms输入年龄越界撤销，但不把它当唯一根因或放宽时效；16拒绝分支没有进一步细因日志。演示原肩部点在现有区域偏前，调整到默认区域中心(-.2295,-.115,+.05)，插入后停留从0.1秒增加为0.35秒，实际收手后返射；17流程通过。仅声明本轮基本流程通过，不宣称间歇问题在所有负载/物理操作条件下都已排除。旧09视频的手部/方向结论已撤回，14/15/16和所有原始记录未覆盖或拼接。

完整结果：analysis/boomshot-20261117/delivery/review17.json；原始帧/时间：video17/projection；字幕/制作记录：delivery/revised17；对应DLL快照：delivery/revised-build14。离线pythonw编码console_hwnd=0、own_visible_windows=[]。

---
## 2026-11-18 抓点、水平与弹道修正（取代下方09轮视觉结论）

真实主持枪档案已找回：owner-cylinder-captured/captures/1794486454128-2.json；主手采用该档案的手腕/枪体关系与15节手指姿势。弹巢采用1794500066511-3的手腕和同一份手指姿势。此前“未采集主手”结论错误，不再要求重新采集。

枪管模型+Y与当前Aim+X统一；Grip锚定位置。Native现有C0写入点区分Boomshot校准，不再套用Lancer腕角。原弹巢组件使用同一C0左腕与采集关系随手渲染，动作仍读取新鲜原始Grip。恢复原组件的Translation、Rotation和全部Absolute标志；不制造替代弹巢。左手Grip闭合时在同一手指写入点采用真实采集姿势，无额外世界手腕目标。Second Hands继续暂缓。

build14已编译，video14完整基本流程通过：普通换弹9.693035秒、完美换弹3.073090秒、原奖励applied=1、三次实际Proj_Boomer。水平持枪实际延迟读回gun_basis的模型+Y z=0；不是只看输入Euler。原版炮弹生成位置与本次记录枪口位置一致，方向使用同一枪线；原版GetPlayerAimError散布、重力和SafeSpawnLoc碰撞保护保持。原速画面首发约1.1秒显示前方同侧目标爆炸。未测物理头显手臂，也未声称所有角度表面贴合验收。

一次目标性修正：video13完美结束后立即按扳机没有发射；等待原版退出动作结束后，video14实际第三发成功。账本完成不等于原版CanFire已恢复；演示明确保留等待时间。video14仍有换弹后空左手挡枪的演示问题，video15只调整收手动作，复用同一build14，不新增功能或测试矩阵。

代码/构建快照：analysis/boomshot-20261117/delivery/revised-build14；新录像和最终复核见同目录后续交付记录。video14所属实例正常退出，owned_game_absent=true，forced_termination=false。以下旧09轮内容仅保留历史，不作为修正版本证据。

---
## 最新补录与完美换弹（2026-11-17）

最新主人范围：暂不继续实现或演示 Second Hands；本次为右手持枪、左手仅换弹。以下历史双手记录不代表本轮重新验收。此段为旧09轮记录：当时未找到真实主手档案的判断已撤回，见2026-11-18修正记录。

完美换弹在真实原弹巢入座回执处停止计时。修复了库存 ACK 提早结束计时、结束请求被提前取走的问题；当前实际普通换弹 9,160,892 us、完美换弹 3,680,652 us，原奖励 applied=1，三次原版 Proj_Boomer 发射和最终返射均成功。没有上膛、开关枪管动作。

补录发现并修复：Boomshot 原定时枪体换弹动画未停止；当前帧过期不应抹掉已认证持有记录；插入完成后的库存提交阶段继续保留路由；几何确认与原库存提交分到新鲜帧；已经认证入座后松开 Grip 允许该次请求收尾。仍检查原对象、同一事务与 Native serial、当前源、库存和一次性 token；输入 100 ms 门未放宽。

枪方向使用原枪管 +Y 对齐 Aim +X、上方 +Z，Grip 保持位置锚点；不再将拿枪瞬间的动画方向保存为水平基准。中立模拟姿态下实际应用 target_rot 俯仰为 0。【已撤回】此前把 restored=1 的 gun_basis 当恢复后矩阵的解释错误；Backend::read 实际先读取延迟渲染矩阵，再恢复。旧约 -29 度是真实缺陷。已检查中立持枪画面，实体 Touch 手臂水平对应与主人感觉未测。

最新私有 Playground：A3B70BFF1E221489396E04C027B31CD21D7D11219CE2A354E7CD10D19DB46860。构建成功；使用配套冻结基线的 compatible-source，未把主目录其他任务的改动整体打包。真实实例 boom-video09-1117-191938 / game90628，driver_exit=0；最终 owned_game_absent=true、无所属子进程、forced_termination=false。停止后未增加游戏测试。

视频：analysis/boomshot-20261117/delivery/Boomshot-完整流程与完美换弹.mp4，42.92 秒、1280×840、6001071 字节。异步 exact-HWND WindowsGraphicsCapture，受控模拟输入，实际时间播放；只裁去前面的导航和模拟器窗口标题菜单，未加速。三行中文流程/操作/对象结果字幕。pythonw 编码，console_hwnd=0、own_visible_windows=[]，FFmpeg CREATE_NO_WINDOW；已抽看举枪、普通取弹、快速插入与完美返射画面。原帧、ASS、字幕 JSON、结果及构建快照均保留在 delivery/full-demo09-* 和 video09-192010。

---
# 榴弹发射器直接换弹巢（2026-11-17）

按主人的简化交互要求实现：拿枪、举枪、射击，随后拔下旧弹巢、左肩取新弹巢、吸附插入后直接射击。正常路径没有上膛、开枪管或关枪管动作。旧机构检查代码仍保留为历史兼容路径，不是正常交互步骤；正常流程不再显示旧开闭枪管引导。

【历史版本】此前只导入左手 support/ammo，主右手使用默认值；2026-11-18已经改为真实主手1794486454128-2。原版模型、Marcus115 骨架、手腕挂接和原版弹巢组件继续复用。副手 Grip 可握持、调整枪口、保持射击并释放。

直接换弹使用原游戏的重装和弹药提交路径。旧弹巢以当前实际组件跟随左手，拔出后释放隐藏；新弹巢从左肩补给取得，沿插入口方向进入，在末段吸附入座。暂时读不到有效输入时不产生入座或扣弹，持有物可在入口外重新对准。当前输入时效仍为 100 ms；直接换弹的相邻有效采样允许最多 250 ms 间隔，以适应受控演示较低帧率，其他武器和旧机构路径未改。

修正了实际运行中发现的 ComponentProperty 读取类型错误，以及原版 IsComponentAttached 返回参数合法的输出标记被拒绝的问题。减少同帧重复的游戏状态、几何和手指显示资产读取；没有更改共享 Native、输入时间戳或原版投射物。

真实 DeepSeek CLI worker：Kaku / deepseek-v4.1-flash，持久会话 `01a1e554-b7c0-7293-816e-a307a5d40cf1`，侧栏标题 `Boomshot｜DeepSeek 开发 worker`。简单实现、姿势导入、构建和脚本准备交由该 worker，主会话负责集成、审阅和实际运行。

私有构建使用 `analysis/boomshot-20261117/compatible-source` 的配套冻结基线加窄范围覆盖，避免混入当前主目录其他任务的未完成改动。Playground SHA256 为 `849589934444ecf9ea2c71a828dc8127c193c53c15be0eb14179c87d016c0364`。交付快照和约 47 秒字幕录像在 `analysis/boomshot-20261117/delivery`。

基础演示实例 `boom-basic-1117-050039` 的实际结果：原版地面拾取和肩部取枪、双手握持/调整瞄准/射击/释放、旧弹巢拔出释放、左肩新弹巢取得、吸附入座、再次射击均已发生；两次原版 Proj_Boomer 投射物回执分别为 call 404965 和 498649。新弹巢入座后 AmmoUsedCount 为 0，再次射击后为 1。场景原生 HasInfiniteSpareAmmo=true，SpareAmmoCount=2 保持不变符合该规则。脚本原本错误要求备用弹数减一而退出 1，原记录保留，修正解释和实际动作结果见 basic-050144/basic-review.json。没有以断言修正冒充新的游戏运行。

实例正常退出：launcher_exit=0、owned_game_absent=true、无残留所属子进程、forced_termination=false。按主人要求，基础动作跑通后未追加全面测试。实体头显手感、完整手部贴合及有限备用弹药场景不在本次实际演示范围。公共 v71、固定日常入口、Quest 安装和其他武器未更新。
