# 手枪准星与 UI 批次容量

## 复现：GameRecovery7，2026-10-15

实际游戏 PID24112（`continuous-stereo-24112-20261015-195606-390abba0.*`）通过对象/类身份与双向关系复读确认从 `Weap_AssaultRifle` 切到 `Weap_COGPistol`。两次正常手柄开镜各3秒，首轮采样的相机FOV由80变为45。没有修改游戏武器或相机内存。游戏EXE SHA256沿用清单中的 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。

- Canvas观察到303次kind1准星绘制；GPU准星分类与校正均为0。
- `ui_payload_store`：capacity_rejected=1、groups_poisoned=16、disabled=1；批次表满后永久停止接纳，后续语义标签丢失。
- 首轮开镜保存GPU原始图ticket368，1920×1080，两眼准星均落在各自矩形中心。双眼使用非对称FOV，矩形中心不代表同一空间方向。
- XR提交1822次、复制896张、跨度20.233秒、最大间隔24.269毫秒。原生清理、42处入口恢复及XR退出完成，游戏正常关闭。该短段不能复验检查点/CG或真头显。

原始图和脚本保存在ignored的`analysis/runtime/`；不得加入发布包。旧样本47092的矩阵审计显示，方向修正后的双眼中心射线误差约0.000000214°，而未修正矩形中心约相差29.44567°；它是几何校验，不能替代本次手枪图像或有限距离命中验证。

## 修复：GameRecovery8

UI批次表容量从16组增到64组（顶点副本仍按需分配，每组最多1024顶点）。表满时只拒绝尚未入表的新payload，不驱逐已有记录，不自动reset，也不使整表永久停用。稍后接纳的非零起始范围把漏记前缀保留为unknown；消费者仍逐索引验证归属、原始顶点字节、原调用绑定及当前场景票据，不宣称拥有整个payload的语义。

已有group发生metadata冲突、回退/重叠、单组容量或分配失败时，仍保留poison tombstone直到消费/显式退休边界；generation耗尽仍永久拒绝。多atlas文字的原有整次撤销逻辑不变。没有新增hook，也没有启用有限距离瞄准缓存。

15项检查通过，覆盖满表保留原组、持续追加、空槽释放后恢复、漏记前缀/混合索引不获得纯标签，以及原有HUD、准星、索引、Material、FrameLink和DLL生命周期。扩大容量使准星测试中多个内联局部Store触发栈溢出，测试fixture改为堆分配后通过；生产Store为静态实例，没有改变游戏线程栈上的分配。首次失败记录保留在`game-recovery8-validation-pre-fixture-fix.json`。

冻结DLL SHA256：`f5cb96399eb5e6ca7d0932cb5a761d9892afc984fd06a69f48a29f95cd942558`；清单`manifest-game-recovery8.json`，XR继续使用SpaceRecovery3。

## 同操作复测与当前入口

GameRecovery8 / PID29060（`continuous-stereo-29060-20261015-200432-eee3ef86.*`）再次通过身份复读确认COG手枪及开镜FOV45，两次各3秒。批次published=7008、claimed=3302，容量拒绝/poison/disabled均0。准星considered/planned/applied均254，center/geometry/projection拒绝均0；HUD面板应用2147次、sequence_fault=0，原生字幕索引绘制65次。字幕计数不等于本轮已目视验收字幕。

开镜ticket376的1920×1080 GPU图显示准星已离开两眼矩形中心，位于各眼对应空间方向；右上弹药HUD恢复为较小的面板布局。三个矩阵齐全的实际准星样本，中心射线夹角约0.000000413°（原矩形中心约29.44567°）。只证明无限远方向修正与实际图像位置，不宣称有限距离命中、所有武器或真头显融合已验收。

XR提交1864次、复制868张、跨度20.680秒、最大间隔22.159毫秒。原生清理、42处入口恢复及XR退出完成，正常关闭游戏。report仍为incomplete，不借短段结果宣布CG/加载黑屏修复。早先PID17932在只读菜单复读变化时测试主动中止，未执行切枪；脚本现仅对这类临时读竞争重试，不重复输入操作。错误日志保留。

玩家`manifest.json`提升为Recovery8；Recovery7与更早冻结清单保留回滚。源码、清单与结论提交Git；测试图像、对象地址和日志仅留在ignored的analysis。下一步确认换枪菜单和字幕在持续运行/重载后仍保持UI布局，并继续有限距离瞄准与CG/加载显示缺口。
