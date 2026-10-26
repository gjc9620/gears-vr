# PG Tick／C0装备观察实现与验证边界

武器侧已经实现独立的query-bound装备观察及typed provider／sender。该实现等待Body的Native Source47后进行一次联合构建和原动作验证，尚未生成新的PG DLL、建立新case或运行Game／XR。旧f85／EO保持冻结，旧V1检查与行为保持。

共享头来自Body唯一封存的`tools/vr-common/body_main_mount_provider.h`，逐字复制SHA256 `6ad0a08fb3633bb0cc3d39841634a69c2a65771ed29886e7c32ceeb9c03b4b3d`。新namespace为`gears_body_main_mount_provider`，新export为`GearsVrAttachBodyMainMountProviderV1`，旧wire不改。PG新开关`GEARS_PLAYGROUND_BODY_MAIN_MOUNT_PROVIDER_TRIAL`默认OFF，要求既有main-mount provider；新旧孩子共用原parent cookie／image pin／callback计数／entered／stop与drain，新增attach失败仍沿原parent断开流程，不能自行卸载或另造registrar。

## 观察与来源的具体分工

`HandClient::observe_equipment_on_gt`在同一Client Gate／本地epoch内匹配实际已提交key、world／PC／Pawn／mesh／weapon完整身份、reference与generation。旧Frame只用于历史关联，不更新时间、不输出Frame、不改publication／participant／held。对旧Frame的结构和既有注册资格仍核对，但当前跟踪资格由独立新raw提供；不调用旧`fresh`、`copy_frame_on_gt`或`source_game_on_gt`来伪称旧跟踪仍新鲜。SEH finally清失败输出并释放Gate；跨线程invalidate改变epoch后，即使状态恢复也不能沿用这次观察。

固定metadata continuation顺序为完整Game-before、asset-first、唯一右手attachment、asset-final、完整Game-after与`same_game`，然后检查实际holdGeneration／held_binding／domain／Client epoch。每个metadata reader内部原有selector、对象、schema、property、link、flag与table末读均保留。专用typed入口直接接受`EquipmentIdentity`，不临时拼Hand Frame。真实metadata begin在首次读前从Client clock取，end在全部读取与末检后取；50ms从begin计，拒绝未来／逆序／跨期，不能用end续期。

sender在入口、metadata中各current围栏和最终response重复检查：原Scene observation／publication50ms、新raw acquisition／stereo／controller receive／sync40ms、C0 capture40ms、当前右Grip usable／focus、source／session／profile／input／space／producer continuity、Native reference稳定形状与同源关联，以及固定Lancer profile的geometry／scale／units／basis。旧tracking保持原值，可以对V1过期；typed路径没有把它续为新数据。

两个reference字段明确不同：`Tracking.reference_revision`是Native私有reference snapshot seqlock/config revision，PG仅检查非0偶数形状并回显；`original_scene.reference_generation`才与Client实际committed Frame的generation硬关联。publication／binding／ticket／cache／Native actionGeneration也只由Native认证，PG不能声称自行观察。SourceKey是echo子集，不能替代Native对完整immutable Source、当前reference/configuration／basis及逐写barrier的验证。

返回值只含同query的真实equipment时间范围、holdGeneration、完整component／asset身份及固定profile metadata；内外query编号相同、mount.key仍原Scene。PG不返回世界姿态或装备／射击／骨骼写入许可。Native必须独立验证同query、父注册及当前source／basis，转换新raw并重建固定calibration，在每次实际写前重新核对40／50／50与原CAS／Kernel条件。

## 本轮已经执行的检查

以下均为本地owned数据或Host seam，不是零售游戏行为接受。断言与场景分别写明，不合并成“功能完成数”。

| 检查 | 结果与实际覆盖 |
|---|---|
| 新sender | 272 checks；old tracking>50／Scene<50／新raw和capture<40时新路径可过、原V1仍拒绝；各时间／focus／右Grip／epochs／reference关联与响应清空 |
| 实际HandClient新操作 | 80 checks；真实Gate／epoch，150ms旧Frame ancestry、完整身份／reference、GT、revoke／reentry、时间边界与SEH后Gate释放及下一调用 |
| 实际typed Game／asset读取 | 24 checks；owned人工reflection内存、原重复读和mutation拒绝；没有把metadata reader本身当跟踪资格 |
| 实际typed attachment读取 | 9 cases；owned Attachments数组与metadata解析seams，唯一右骨／可见性／结构／NaN／末读变化；不代表引擎或Client资格 |
| 实际新provider／sender／equipment编排 | 271 checks；实际callback与两Game／双asset／attachment调用顺序、generation／domain／same_game拒绝、共享entered重入、SEH／TLS／scratch清理、attach收据与失联；Game／asset／attachment／Client本身用明确owned seams |
| 原HandClient回归 | 15 existing＋30 Frame-copy＋17 source-current及8 game-state输出cases通过；旧freshness／same_game等原路径保持 |
| 实际PG完整TU | 同f85生产defines／flags，加唯一新flag，MSVC `/Zs /O2 /W4 /WX /EHsc /MT`通过；语法检查无obj／DLL／link／运行结果 |

实现前后准确差分证明另外确认：删去本次纯新增操作后，原Client文件逐字还原；删去新flag围栏后，原start／playground源逐字还原；新typed三个reader除入口签名／identity carrier外，与原函数体逐字一致；共享头、f85 DLL和EO private输入保持。所有比较以正常文本换行读取，不伪称Git换行处理后的文件字节等于原二进制。

一轮Astra只读增量审查未发现具体P1/P2；审查未运行测试、DLL或Game，也未审核整个Native，依赖已约定的Native完整source／实际GT／barrier before／after／逐写责任。失败的编译尝试与修正后的通过回执一并保留，只复跑受变化影响的检查，未反复重测未变的metadata／attachment／Client路径。

证据位于ignored `analysis/body-mount-pg-candidate-20261026/`：`source-delta-proof.json`、`helper-tests-receipt.json`、各`owned-*`目录的逐step结果及`syntax-1792991366200026300/receipt.json`。早期owned receipt可能整体exit1但包含已通过且后续依赖未变的单项结果，不能把它整体写作通过；最终集合按单项和其实际源依赖关联。未完成的联合DLL链接、零售动作、GPU／双眼与头显接受明确保持pending，整体36%／W2 40%不变。
