# Tick／C 0 PG Equipment Observation Program for Hang-up Query

This is the minimum interface proposal for PG/Clit, which will be implemented after body side sharing typeped head and accurate Hash.1、`copy_frame_on_gt`、`source_game_on_gt`、PG f 85 This is a read-only source code and sorting document without building, case or Game/XR. The scheme is based on the side of your body.`analysis/body-c0-producer-after-eo-review1.md`, does not define or publish a new ABI.

## Actual Reading Dependence

| Current entrance | Actual dependency | Processing of new paths |
|---|---|---|
| `HandClient::copy_frame_on_gt`／`source_game_on_gt` | actualGT、Client Gate、active epoch／published、key、`fresh(frame)`, Aadapter Game Watch; the former is more than a pair of listed_games | Keep V whole 1; new equipment operations only take the exact selected key/five objects/reference generation as historical links in the same Gate, do not output Frame, do not call old fresh and do not use history committed_game as current observation |
| `weaponhand::observe` | Enabled, actualGT, restore health and mutual retrial; object registry/complete identity/type/fatherbag; PC ⁇ Pawn/World, Pawn ⁇ Mesh/Weapon, Weapon.Instigator; de-cut/shooting/prohibited shooting marking; practical software hold with help_binding; former schema/property/link/flag/table | New typed object context directly provides the same price metadata to read and keep all original actual and final readings; no ad hoc frame call old observer. Its current realization does not read HMD/controller pose, but its V 1 Call eligibility does depend on these data |
| `lancermain_asset::read` | Key.theread, weapon/pahn/mesh identity, WarWeapon and Lancer type, Instigator, Mesh typed descriptor, Visibility, SkeletalMesh full identity and COGASSaultRifle; all selfer/schema/policy/table | Add typed metadata portal, maintain two full asset readings and repeat them all for each time, comparing the complete identity of the coponent to the full identity of the asset; not just duplicate an address or skip an internal one |
| `read_attached_frame` | Webpon/pawn/mesh address, quoted campaign, current Instigator, sole Attachments members and right hand bones, visibility, membership structure, limited position/scale, array and entry last reading, Waapon.Mesh last reading | Provides the same read-only core for new typed object context; its outer layer`attached_frame/current`Keep original V 1 The new path calls the metadata core in a new client observation, and without that it will re-enter the outer layer of the client Gate. |
| `eligibility`／`source::bind`／`weapon_rig_binding::match` | Old Frame Active/Reference Transition, Old Right Grip Usable, Same old Rig/hand and World_grip Recalculation, Current Held/Generation/Busy/ation/Firearm, C 0 profile | V 1 Keeps as it is. New path verifies the right Grip qualification of Raw Tick and 40 ms; PG only validates the current body-profile identity of metadata and Native.0 Basis, PG doesn't export world gestures. |

Accurate source position:`tools/vr-common/weapon_hand_client.h:214`、`:234`，`tools/playground/weapon_hand_client.inl:114`，`weapon_lancer_asset_read.inl:23`，`weapon_equipment_backend.inl:90`，`weapon_equipment_main_mount.inl:17`／`:87`，`weapon_main_mount_candidate.h:35`，`weapon_rig_binding.h:34`. These read without calling engines, without equipment action, without changing held, without applying for downership. Follow-up will be possible only by reusing already existing metadata slides/providing independent typed entrance; no changes to old V 1 Qualification for new behaviour.

## Proposed local interface boundary

Below are the job names, which are not the type of name, byte layout or function signature that is frozen. The header is only created by Body.

`observe_equipment_on_gt(expected_parent_association, query_correlation, dedicated_metadata_reader, out_equipment_witness)`。

`expected_parent_association`This is a request from typed, which has been accessed by Broker: full original FrameKey, five complete object identities, access promotion, registration cookies/Native incarnation/session/realGT and parent lifecycle.(a) Publicization/binding promotion by the original requested Scene connection;Hand ' s company Frame does not contain these two fields, and PG cannot claim to have taken them from Clint, but must match the actual source of its reservation with Broker.Former Scene maintains historical values with original observation time and executes the remaining Scene-observation separately 50。

`query_correlation`Only this call, real Tick source/current C 0 Basis and queryid; it is not eligible. The original private espoch of Clent was captured and checked by this Gate, and the request for epoch proof was rejected.

`dedicated_metadata_reader`It is a fixed read-only realization for PG compilation, executed in the same time within Gate. Only the Key/objectability view and local current check are received from this extraction, and no old Frame, Native writer, engine action or external variable callback is accepted. It can use existing reflection profiles and query-local resolution scratch but cannot be returned`copy_frame_on_gt`、`source_game_on_gt`、`snapshot_on_gt`Or the outer layer.`attached_frame`The request is not given any additional pointer.

`out_equipment_witness`It is a zero-start, successful and end-value-entrusted data set for the carler-downd: Benquery connection, original parentent association, time frame for two real Game observations, stabilization of holdoutGeneral, full-fledged webport/asset identity, current held/nonbusy/nonimation/firearm results, current profile link to the Basis version. Return only to real observation times, not to reset start or old Scene/raw tracking times at return times;50 Ms observe age from the earliest practical need, and now reverse/future/expiration. Whether or not to include two times with which fixed-profile is marked and wherebyBody ends up.

## Synchronize the sequence of operations and review

1. Enter the original file callback list/PG callbacks and obtain a new provider scratch; continue using parent pin, unregister/drain. Verify that typeed requests Shape/version/reserved, parent, actual GT, current registered session and domain.40 With C 0 capture 40、Scene-observation 50 Each of them is verified by the corresponding real timescale, without replacing the other.
2. Clinent single-time unblocked access to the original Gate; capture active epoch, requiring that publicised be identical to epoch, cookie/Native/session/actualGT/completed key and object. Only extracts historical identity, does not copy/manufacturing/publication of Frame, does not modify the condition of the committee_/committee_game_/participant_/hold. Gate bus handles it in accordance with the original fail-closed rules without waiting.
3. Confirms that this actual non-zero Generalization is frozen; implements the full read-only observation of Gamed-before and requires that held_binding be consistent with the world/PC/Pawn/weapon/cookie/Native/world_gender, model status and establishment, and notbusy/animation.
4. Execute the entire and more complete command/asset identity of aset-first, actual only right hand attachment, anset-final. Keep all independent final readings within each reader. Then execute the full Game-after observation and`same_game(before,after)`, make two Game observations to include aset/attachment; this is the clear sequence of the new path and the old path remains unchanged. If Body’s final interface requires different order, it must remain all original and prove that the metadata did not eventually bypass history snapshot.
5. Read the holdGeneral and hisd_binding, domain still ready, no action barrier changes; check epoch/publicized/cookie/actual GT/committee act unchanged, real observation still remains 50 ms;new raw/C 0 Countercheck still works.40 The identity and status/conversion remain unchanged. Constructively, the witness is rechecked before you exit Gate. Provider checks registration/domain/typed connections and time limits again before final release; Native receives independent verification of Query and current source/basis/metadata, and re-tests them before each actual writing.

Current`domain_ready()`Only the function of a plan is to provide for the use of the system.Therefore, the “existing actionGeneral has been checked” cannot be asserted in a blank in the new Witness.Body needs to specify the syntax of the parent action-generalization/barrier that it authenticates;PG uses the former domain fence and makes an exactive connection to the actual state of correspondence when sharing header requirements.Maintaining callback readings of engine-free and avoiding the reentry of midway engines;Cross-line invalidate was blocked through the original atomic epoch, and even if it recovered immediately after that event, the effect of epoch change could not be concealed by back-and-back bool.

The new operation has to be rejected. The end-of-service witness cannot be reused. Only 'old traching is over'50"not pretend to be a new raw expired or rewritten/empty V in this special metadata operation 1 status;old V 1 Consumers refuse to follow the usual pattern.1 This does not allow new registration, re-post old Hand, replace tracks and introduce crossquey caches.

## Minimum demand for headers sent to Body

- The original name of the original Hand Key/V is linked to the real Scene publication/binding identity; there are other authentications for the former Scene observation time.
- Practice Tick/thread, non-rewritten sampling time, active/focus/producer consultation/session/space/project/input/reference_epochs and original right-hand Grip fields; available for PG 40 These are independent checks on eligibility.
- Current C 0 The company/capture-start/actor/mesh/conference-config/geometry/uniform scale/units and basis version; PG validates the association of the fixed Lancer program with the object, Native is responsible for actual coordinate conversion and reconstruction.
- Query is associated with parent lifecycle/action barrier, responding to real metadata time frame and holdGeneral/completecomponent/asset education; using former parent pin/drain independent typeped attch/callback ABI. Status distinction between Tracking, parent, metadata or policy refuses to avoid old source 50 The failure was renamed a new success.

The PG realization range is expected to be an independent typed projecter/sender/candidate path, the above-mentioned local equipment observation and metadata overload for HandClitent.1 The old method, the old binding/eligiliity and the old provider cannot be replaced by a new time mark.

Minimum authentication only for actual new boundary: old tracking>50 Original Scene observation 50 New Raw/capture<40 And with the same hard identity, V.1 Still refusing, typed can be observed with the fresh equipment; changes between query and writing remain rejected, and stop is still in line. The actual timescale and each writing connection must come from a real call path. No weapons/situation matrix will be expanded until then taken by Body alone 40／6 Second action validation.

---

## 中文

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
