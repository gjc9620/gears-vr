# 武器消费者对 Body Tick 双来源合同的窄审阅

审阅对象：身体工作树 `docs/6dof-body-tick-input-contract-20261026.md`，commit `c7115f3`，实际文件与该提交 blob 的 SHA256 均为 `c0d24e735c3aa65b759e791ec4368e9fd6f515b32e301a6bd3198a0479577242`。仅审该合同对已核过的 PG0ec1 消费者的影响；未重复14份源码／5份共享头基线检查，未增加助手、构建、Game 或共享源码改动。

**结论：PG 零改可行，但以 Native 实现下列明确关联为前提；当前合同不是新写入路径的接受证据。** 先实施真实 outer Tick 只读探针与此结论兼容，不需要先修改 PG。

## 保留原消费者的 Native 前提

1. **Scene 校准链完全保留。** parent Hand 仍提交真正 Scene 的原 Frame；request.retained 仍是对应的原 Rig，BodyContext 保留其 Scene／publication／bindingGeneration／referenceGeneration。PG 初次及独立末次 Game 观察、typed 资产／附着、hold generation、最终 sender 检查和原50ms上限全部留在每次同步查询中。Tick 输入不进入旧 request，也不改变 response.key。
2. **一次 query 对应一次实际 C0。** Native 将非零 query id、实际 response key、当前 parent cookie／incarnation／lifecycle 与该 C0 的 entry／Compose／真实 outer Tick 身份绑定。response.key 必须仍等于该查询使用的已提交 Hand key；原 Hand 与原 Rig 继续满足 Scene 身份和原输入的精确一致，不能拿 Tick id 或 Tick 样本满足旧 Scene 比较。响应只为本次选择固定校准，不跨 C0 或 query 复用 ready。
3. **两条来源只在明确的共同身份上关联。** 原 Scene 资格与新 Tick 输入必须关联同一个当前 world／controller／pawn／mesh／weapon、Native/world generation 和稳定 reference／producer continuity。Tick 的 raw sequence 可以比 Scene 更新；两者不能强制原输入逐字相同，否则没有新采样收益，也不能改写旧输入使其看起来相同。每条来源内部的头显／双手／continuity 则必须各自精确成组。
4. **当前模型与武器资产分别核实。** C0 当前 Avatar asset／reference header／模型代次、geometry／scale／units／basis 与查询 BodyGeometry 必须匹配；response 的 weapon component／asset／hold generation 是另一组身份，不能用 Avatar 身份代替。保留已有每写 current／validate／exchange／CAS／回读和 parent 撤销、动作、换枪、reference 失效检查；不能把同步查询成功缓存为后续 current 成功。
5. **年龄按实际依赖分别验证。** 原 Scene 校准依赖及原时标继续完整50ms；真正使用的新 Tick 整包原 sync／receive、真实取得时间及本次 C0 基准 capture 开始时间按40ms，并在末次使用前复核。目标计算使用该次当前 CTW／actor-local neutral 所构造的统一 base/reference，不读取旧 Scene 的 world target。新包不可在 C0 中替换或续时，关联失败走原无写入／原动画让位和清理路径。

## 具体发现与后续接受边界

- **需修正文档的一处年龄矛盾。** 原合同第51行“每写身份／时间／元数据”的新路径列写“新Tick输入不可变与50ms检查”，与其余 Tick40／Scene50 规则冲突。应分别写明新Tick仍40ms、Scene依赖仍50ms，避免后继实现误用。已交身体侧修正，不阻止当前只读探针。
- **后继写入仍缺实现证据。** 合同列出了必要字段，但尚未有可核验的 `BodyTickTrackingInput`／同C0组合回执类型、不可变范围、join validator 和最终使用点接线。它们必须表达上述身份／代次／reference／双时限，并拒绝错query、错parent、错模型／资产、失效reference与过期任一来源；这是未来新消费的接受前提，不是要求在只读探针中提前实现 writer。
- **当前消费者无新增接口缺口。** 若 Native 保持上述边界，PG0ec1及 V1 request／response 可原字节复用。探针先证明真实 Tick 能读到更新且成组的原包、原时标和来源关联，并通过正常恢复；随后才接受新 typed 姿态消费。下一真正行为门仍为持续 held、原小移转和交还，合同审阅不能代替这些结果。
