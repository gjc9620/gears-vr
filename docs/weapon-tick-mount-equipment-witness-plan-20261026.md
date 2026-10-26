# Tick／C0挂点查询的PG装备观察方案

这是PG／Client侧的最小接口建议，等待身体侧共享typed头及准确hash后再实现。现有V1、`copy_frame_on_gt`、`source_game_on_gt`、PG f85和EO封存文件均不改。本次只读源码和整理文档，没有构建、case或Game／XR。方案依据身体侧`analysis/body-c0-producer-after-eo-review1.md`，不自行定义或发布新ABI。

## 实际读取依赖

| 当前入口 | 实际依赖 | 新路径的处理 |
|---|---|---|
| `HandClient::copy_frame_on_gt`／`source_game_on_gt` | actualGT、Client Gate、active epoch／published、key、`fresh(frame)`、adapter Game观察；前者还比对committed_game | 完整保留V1；新装备操作只在同一个Gate内取exact committed key／五个对象／reference generation作为历史关联，不输出Frame，不调用旧fresh，也不把历史committed_game当当前观察 |
| `weaponhand::observe` | enabled、actualGT、restore健康与互斥trial；对象registry／完整身份／类型／父包；PC↔Pawn／World、Pawn↔Mesh／Weapon、Weapon.Instigator；删除／切枪／禁止射击标志；实际软件hold与held_binding；原schema／property／link／flag／table末读 | 新typed对象上下文直接供给等价metadata读取，保留所有原实际读和末读；不用临时拼Frame调用旧observer。其当前实现没有读HMD／controller pose，但其V1调用资格确实依赖这些数据 |
| `lancermain_asset::read` | key.thread、weapon／pawn／mesh身份、WarWeapon与Lancer类型、Instigator、Mesh typed descriptor、可见性、SkeletalMesh完整身份及COGAssaultRifle；全部selector／schema／property／table重复读 | 增加typed metadata入口，保留两次完整asset读及每次自身全部重复读，比较component与asset完整身份；不能只比地址或跳过内部重复 |
| `read_attached_frame` | weapon／pawn／mesh地址、expected component、当前Instigator、唯一Attachments成员及右手骨、可见性、成员结构、有限位置／scale、数组与entry末读、weapon.Mesh末读 | 为新typed对象上下文提供同一只读核心；其外层`attached_frame/current`仍保留原V1 source资格。新路径在新Client观察内调用metadata核心，不调用会重入Client Gate的外层 |
| `eligibility`／`source::bind`／`weapon_rig_binding::match` | 旧Frame active／reference转变、旧右Grip usable、完全相同旧Rig／Hand和world_grip重算、当前held／generation／busy／animation／firearm、C0 profile | V1原样保留。新路径另验认证raw Tick的右Grip资格和40ms；PG仅验metadata与Native认证的当前body-profile身份。world_grip重算／转换归Native当前C0 basis，PG不输出世界姿态 |

精确源码位置：`tools/vr-common/weapon_hand_client.h:214`、`:234`，`tools/playground/weapon_hand_client.inl:114`，`weapon_lancer_asset_read.inl:23`，`weapon_equipment_backend.inl:90`，`weapon_equipment_main_mount.inl:17`／`:87`，`weapon_main_mount_candidate.h:35`，`weapon_rig_binding.h:34`。这些读不调用引擎、不做装备动作、不改变held、不申请ownership。后续实现只可复用已经存在的metadata primitives／提供独立typed入口；不以改掉旧V1资格取得新行为。

## 建议的本地接口边界

以下是职责名字，不是冻结的类型名、字节布局或函数签名。共享头由Body唯一制定。Client本地只需要一个同步操作，概念上为：

`observe_equipment_on_gt(expected_parent_association, query_correlation, dedicated_metadata_reader, out_equipment_witness)`。

`expected_parent_association`来自已经被Broker准入的typed请求：完整原committed FrameKey、五个完整对象身份、reference generation、同注册cookie／Native incarnation／session／actualGT及parent lifecycle。publication／binding generation由请求原Scene关联给出；Hand的committed Frame本身不含这两个字段，PG不能声称从Client取到了它们，须由Broker匹配其实际保留来源。原Scene与原观察时间保持历史原值，单独执行仍必需的Scene-observation50。

`query_correlation`只关联本次调用、真实Tick source／当前C0 basis与查询id；它不是资格。原Client私有epoch由本次Gate操作自己捕获并检查，不接受请求端任填epoch当证明。

`dedicated_metadata_reader`是PG编译时固定的只读实现，在Gate内同步执行。只收到本次提取的key／对象identity视图及本地current检查，不接收旧Frame、Native写入器、engine action或外部可变callback。它可使用现有reflection primitives和query-local解析scratch，但不得回调`copy_frame_on_gt`、`source_game_on_gt`、`snapshot_on_gt`或外层`attached_frame`而递归取得同一个Gate。不要给请求增加任意reader指针。

`out_equipment_witness`是清零开始、成功后才赋值的caller-owned专用数据：本query关联、原parent关联、两次真实Game观察的时间范围、稳定holdGeneration、完整weapon/component/asset身份、当前held／非busy／非animation／firearm结果、当前profile与basis版本关联。只返回真实观察时间，不以返回时刻重置开始时刻或旧Scene／raw tracking时间；50ms从实际最早必需metadata观察计龄，now倒退／未来／溢出均拒绝。是否在wire包含两次时间与哪些fixed-profile标识，由Body头收口。不得返回Hand Frame、world pose、任意calibration、装备／射击许可或缓存permit。

## 同步操作与复核顺序

1. 进入原parent supplemental callback lease／PG callbacks计数并取得新provider独占scratch；继续使用parent pin、unregister／drain。验证typed请求shape／version／reserved、parent、actualGT、当前registered session和domain。新raw40与C0 capture40、Scene-observation50各自按对应真实时标验证，不拿一种替代另一种。
2. Client单次非阻塞取得原Gate；捕获active epoch，要求published恰为同epoch，cookie／Native／session／actualGT／完整committed key及对象与请求exact一致。只提取历史身份，不copy／制造／发布Frame，不修改committed_／committed_game_／participant_／held状态。Gate busy按原fail-closed规则处理，不等待。
3. 确认domain／同epoch／同注册当前，冻结本次实际非零holdGeneration；执行Game-before完整只读观察，要求当前held_binding与同world／PC／Pawn／weapon／cookie／Native／world_generation、模型held状态和generation一致，且非busy／animation、确为firearm。
4. 执行asset-first、实际唯一右手attachment、asset-final完整读并比较完整component／asset身份。保留每个reader内部所有独立末读。随后执行Game-after完整观察并`same_game(before,after)`，使两次Game观察包住asset／attachment工作；这是新路径的明确顺序，旧路径不改。若Body最终接口要求不同顺序，仍须保留全部原读并证明最终metadata没有借历史snapshot绕过。
5. 重新读holdGeneration及held_binding，domain仍ready、无action barrier变化；检查epoch／published／cookie／actualGT／committed exact关联未变，真实观察仍≤50ms；新raw／C0对应检查仍≤40ms且身份和reference/configuration不变。构造专用witness，退出Gate前再验epoch。provider最终发布前再次检查注册／domain／typed关联及时限；Native收到后独立验同query与当前source/basis／metadata依赖，并在每次实际写前重验。

当前`domain_ready()`实际只有weapon-call ready／shot stack depth，加可选action-gate ready／depth，不包含可导出的单调action generation。因此不能在新witness中凭空宣称“现有actionGeneration已核对”。Body需指定它认证的parent action-generation／barrier语义；PG使用原domain前后围栏，并在共享头要求时对实际存在的对应状态做exact关联。保持callback读取engine-free，避免中途引擎重入；跨线程invalidate通过原atomic epoch阻断，即便发生后立即恢复，epoch变化仍不能被前后bool掩盖。

新操作的hard revoke／terminal／disconnect仍必须拒绝，结束后的witness不能复用。仅“旧tracking已超50”不应在这个专用metadata操作里冒充新raw过期或凭此改写／清空V1状态；旧V1消费者照旧拒绝。若旧V1先前确因真实invalidate清掉published或parent，typed操作也须拒绝，不能重建它。这样不新注册、不补发旧Hand、不替换轨迹、不引入跨query许可缓存。

## 发给Body共享头的最小需求

- 能exact关联原parent Hand key／五对象与真实Scene publication／binding身份；另有认证的原Scene观察时刻。FrameKey.frame保持原Scene，不以Tick／Compose替换。
- 新typed raw来源的actual Tick／thread、不可重写的采样时间、active/focus／producer continuity／session／space／profile／input／reference epochs和右Grip原字段；PG可用40ms与usable资格进行独立检查。不为PG重复decode Grip、重开bridge或采样。
- 当前C0的Compose／capture-start／actor／mesh／reference-config／geometry／uniform scale／units及basis版本关联；PG验证fixed Lancer profile支持与对象关联，Native负责实际坐标转换与重建calibration。
- query与parent lifecycle／action barrier关联、响应真实metadata时间范围和holdGeneration／完整component/asset identity；使用原parent pin/drain的独立typed attach／callback ABI。状态区分tracking、parent、metadata或profile拒绝，避免把旧source50失败改名为新成功。

PG实现范围预计是一个独立typed provider／sender／candidate路径、HandClient的上述本地装备观察及metadata overload。V1 wire、旧方法、旧binding／eligibility及旧provider不能被新时标静默替代。共享头封存前不提交这些实现，也不先建立新case。

最低验证只做实际新边界：old tracking>50、原Scene observation<50、新raw／capture<40且同硬身份，V1仍拒绝而typed可凭fresh装备观察完成；query与写之间parent／source／reference／heldGeneration变化仍拒绝，stop仍按原drain。实际时标与每次写关联须来自真调用路径。暂不扩大武器／场景矩阵，之后再由Body独占原40／6秒动作验证。
