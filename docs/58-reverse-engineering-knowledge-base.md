# IDA Reverse Knowledge Bank and First Test

2026-10-15. This round completes the basic structure and 1,000 Function to run on a test, and then expand to comply with VR Mod mainline and Token budgets without automatically scanning the full game.

## 2026-10-15: Extension after user-authorized full database

The follow-up goal has been clearly expanded to full index, batch cache code and is still understood only for VR-related systems. No new sub-agents, either Agent or AI, are available at this stage. `tools/re-kb/ida_corpus.py`In the new `analysis/re-kb/full-native/snapshot.i64` executes in copy; initial index backup is as `full-native/index-before-expansion.sqlite`。

Fully indexed in the current IDA code section 113,924 function, all functions conform to historical source blib verification; deposit 1,512,528 IDA xref records and 342,569 bar directly calls records. xref contains the in- and out-sides saved by function, which may be repeated, not the only number calling the side of the map. Indirect calls to target, unrecognized code, and snapshot external modules are still incomplete.

Also recheck all six sections of the original section with SHA 256,extract 359,642 ASCII/UTF-16 LE ASCII subset string candidate. `snapshot_literals` Independent Table, Save Section SHA 256、manifest SHA 256, VA/RVA, code and capture source. Bytes scan candidate may be misidentified, not an IDA type definition, much less a running use; other codes are not yet covered.256 Yes `cae9aa5ca275e0094dc6c0944a643101fea7226235cb1a28f65df0cfcdecc76f`。

The pseudocode cache continues to be strung up in real time. `kb.py stats`、`full-native/status.json` and `runs.jsonl` Yes.`function_coverage=1` Only means that the name/ address count of this section is complete; it must be read simultaneously `pseudocode_coverage` and `reference_index_progress`- I'm sorry. 999 A successful result continues to be reused. The reason for failure is persistent and does not automatically retrace.

Retrieval examples:`py -3 tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite search Camera --limit 5`;known target RVA is used after conversion to VA `refs 0x140110878 --artifact-id 1 --limit 10` Checks the logs of the side and function. First check the database, then read the corresponding pseudocode by actually developing a suspect.

This is a full historical code section of mechanical index, not complete game database. The generation of pseudocodes, the incorporation of existing orientation exports, refinement of string/reference links and data sections analysis, and deepening structure by render/UI/ Camera/Input demand; actual games and RenderDoc validation are not replaced by static coverage.

## Verified environment

- Independent IDA MCP available: IDA 9.4、Hex-Rays 9.4.0.260915、IDAPython Python 3.12.15, the dalib backstage runs without having to open the GUI manually.
- REA 6.4.0 Connected, but IDA report `not_configured`- No, I don't. `REA_IDA_MCP_CONFIG`. This round does not install, migrate or change the global configuration. Existing MCP is `open_database/execute_python` interface;upstream of REA local fit documentation requirements `idb_open/idb_list/idb_close`The ULA is still an incomplete integration item.
- The list of examples before opening the IDA is empty. Copy `analysis/ida/camera-110878-20261014/section_1000.bin.i64` Present. `analysis/re-kb/pilot-1000/snapshot.i64`, only on copy; closing case successfully. Back and forth of the library SHA 256 Both. `66d17edbf21bf81f0fe5cf740111e738a0308ac9228f7576fe39f460baad1813`。

## Data identity and boundaries

This is PID.16812 Runtime code section, SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`. Source EXE 's SHA 256 Yes `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. The module base is `0x140000000`,blob offset zero `0x140001000`, no Zero image base of IDA Raw loader can be used to calculate RVA.

Just one. 64 BitCode `0x140001000..0x14131a94f`, the library is defined. 113,919 function, export follow-up run as 113,924 individual, indicating that the status of the copy analysis changes with the Hex-Rays job; these numbers do not represent the true total full game function. The final index denominator is used 113,924,selected 1,000 Address list remains unchanged. The snapshot is read in the historical section, not the current process atomic snapshot. All known chunks of each export function are checked against source blob bytes; pseudocodes are not semanticly correct.

Existing 2,709 The UI candidate is UnrealScript Export,1,093 The article only states that the token and type are undefended. They enter a separate script table without increasing the native coverage.

## Achievement and piloting results

`tools/re-kb/ida_export.py` Batch Call in IDAAPython `ida_hexrays.decompile_many()`, read function-by-function results and the reason for failure. Hypothetical code and JSON record independent drops, MCP returns only summary. The first round of samples by address space certainty excludes those identified in export from existing names 100 An address; this is not a full historical exercise, but an index and historical evidence will be examined in the future before expanding.

| Indicators | Measure |
| --- | --- |
| Selected/Record Functions | 1,000 / 1,000 |
| Hypocrite success/failure | 999 / 1 |
| Success rate | 99.9%(this sample only) |
| Recorded/IDA definition function coverage | About 0.878% |
| Four batches of cumulative wall clock time | 39.094 sec, does not open database |
| Where native bat calls cumulative | 22.936 sec |
| xref records / direct call records | 12,220 / 3,192(Salve by function, repeat side) |
| String Record | 0; missing data sections that cannot be interpreted as playing with no strings |
| Rerun | 0 A new export, Jo. 0.281 sec |
| Add AI In-depth Analysis Functions to this batch | 0 |
| AI Token Consumption | unknown; interface is not task-by-task measure, not filled 0 or estimate |

Failed function RVA `0x11a2bd4`Hex-Rays `1411a2c17: call analysis failed`. The failure has been prolonged and the default run-back does not repeat; the correction analysis is retried with a new program/output to preserve old evidence.

See SQLite CLI [Tool Description](../tools/re-kb/README.md). Key is artifact SHA 256 + Source type + source base, original VA saves at the same time as RVA; script, native is separated from cross-source discovery. Import submitted by a function service and can be re-exported; pseudocode file validates Hashi. Exporter's monopolistic lock, profile Hash and atomic records support recovery; type/analysis of environmental changes requires new output directories that cannot follow the old check point falsely updated results. 1,000 Native function,2,709 Script entries,17 The article is well established; the number of repeated imports does not increase,5 Tests for identity isolation, failure recovery, Hash authentication, declaration separation and cut-off input are passed. The additional depth analysis is still as follows: 0；17 Article is organized in accordance with the conclusions and cannot be considered a function for new analysis.

## Follow-up rules for collaboration

The main Agent holds the sole IDA writing responsibility; subAgent searchs for local indexes and saved evidence without contesting databases. Mechanical export does not occupy AI analysis; light classification prioritizes individual low strength items Agent, only critical call chains or risk review upgrades. Up to three do not require three permanent residences. No full analysis is added after the end of the current three subtasks.

New Development First Search `analysis/re-kb/knowledge.sqlite`, recheck the original and version of evidence before you can finally perform a minimal new reverse. No matching does not represent that function does not exist. The conclusion must include functions/declarations of identity, artifact or clear unknown, evidence, credible level, static/dynamic distinction and to be verified.

UI, I'll see you next time. [UI Priority](59-re-kb-ui-priorities.md)See Mechanism [Mechanism priorities](60-re-kb-mechanics-priorities.md). RenderDoc, actual game and real head start validation are not yet replaced by static indexing. This round does not start games, install tools, modify Hook or execute release.

Local evidence:`analysis/re-kb/capabilities.json`、`pilot-1000/config.json`、`profile.json`、`selection.json`、`runs.jsonl`、`functions.jsonl`、`records/`、`pseudocode/`. Original derivative data continues to be ignored by Git, not entering the Mod release package.

---

## 中文

# IDA 逆向知识库与首轮试运行

2026-10-15。本轮完成基础架构与 1,000 函数试运行，后续扩展服从 VR Mod 主线和 Token 预算，不自动扫描全游戏。

## 2026-10-15：用户授权全量资料库后的扩展

后续目标已明确扩展为全量索引、分批缓存伪代码，仍只对 VR 相关系统深入理解。本阶段没有新增子 Agent 或逐函数 AI 阅读。使用 `tools/re-kb/ida_corpus.py`，在新的 `analysis/re-kb/full-native/snapshot.i64` 副本中执行；初始索引备份为 `full-native/index-before-expansion.sqlite`。

已完整索引当前 IDA 代码节中的 113,924 个函数，所有函数 chunks 均与历史源 blob 校验一致；存入 1,512,528 条 IDA xref 记录和 342,569 条直接 call 记录。xref 包含按函数保存的入边/出边，可能重复，不是唯一调用图边数。间接调用目标、未识别代码及快照外模块仍不完整。

另外对原 manifest 的六个节全部重新核验大小与 SHA256，提取 359,642 条 ASCII/UTF-16LE ASCII 子集字符串候选。存入 `snapshot_literals` 独立表，保存节 SHA256、manifest SHA256、VA/RVA、编码和捕获来源。字节扫描候选可能误识别，不是 IDA 类型定义，更不表示运行使用；其他编码尚未覆盖。manifest SHA256 为 `cae9aa5ca275e0094dc6c0944a643101fea7226235cb1a28f65df0cfcdecc76f`。

伪代码缓存继续分批推进，实时数量以 `kb.py stats`、`full-native/status.json` 和 `runs.jsonl` 为准。`function_coverage=1` 仅表示本代码节的名称/地址盘点完成；必须同时看 `pseudocode_coverage` 和 `reference_index_progress`。原先 999 份成功结果继续复用。失败原因持久化，不自动循环重试。

检索例子：`py -3 tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite search Camera --limit 5`；已知目标 RVA 转成 VA 后用 `refs 0x140110878 --artifact-id 1 --limit 10` 检查入边与函数所属记录。先查资料库，再按实际开发疑点读取相关伪代码。

当前是一个完整历史代码节的机械索引，不是完整游戏资料库已完成。仍需继续生成伪代码、纳入既有定向导出、完善字符串/引用关联与数据节分析，并按渲染/UI/相机/输入需求深化结构；实际游戏和 RenderDoc 验证不由静态覆盖率替代。

## 已核实的环境

- 独立 IDA MCP 可用：IDA 9.4、Hex-Rays 9.4.0.260915、IDAPython Python 3.12.15、idalib 后台运行，无需手动打开 GUI。
- REA 6.4.0 已连接，但 IDA provider 报 `not_configured`，缺 `REA_IDA_MCP_CONFIG`。本轮不安装、不迁移、不改全局配置。现有 MCP 是 `open_database/execute_python` 接口；REA 本地适配文档要求的上游 supervisor 是 `idb_open/idb_list/idb_close`，不能仅填同一地址便宣称兼容。统一入口仍是未完成的集成项。
- 打开前 IDA 实例列表为空。复制 `analysis/ida/camera-110878-20261014/section_1000.bin.i64` 到 `analysis/re-kb/pilot-1000/snapshot.i64`，只在副本上操作；关闭 lease 成功。原库前后 SHA256 均为 `66d17edbf21bf81f0fe5cf740111e738a0308ac9228f7576fe39f460baad1813`。

## 数据身份与边界

本次目标为 PID16812 的运行时代码节，SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`。源磁盘 EXE 的 SHA256 为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。二者严格区分。模块基址是 `0x140000000`，blob 偏移零对应 `0x140001000`，不得使用 IDA raw loader 的零 image base 计算 RVA。

仅有一个 64 位代码节 `0x140001000..0x14131a94f`，打开时库中定义了 113,919 个函数，导出后续跑枚举为 113,924 个，表明副本分析状态会随 Hex-Rays 工作变化；这些数不代表真实全游戏函数总量。最终索引分母采用 113,924，选定的 1,000 地址清单保持不变。快照为历史分块读取，不是当前进程原子快照。每个导出函数的全部已知 chunks 均对照源 blob 验证字节；伪代码成功不等于语义正确。

已有 2,709 条 UI 候选是 UnrealScript 导出，1,093 条仅 native 声明，另有未解析 token 和类型占位。它们进入独立脚本表，不增加 native 覆盖率。

## 实现与试运行结果

`tools/re-kb/ida_export.py` 在 IDAPython 内批量调用 `ida_hexrays.decompile_many()`，再读取逐函数结果和失败原因。伪代码与 JSON 记录独立落盘，MCP 仅返回摘要。首轮按地址空间确定性抽样，排除从既有命名导出中识别的 100 个地址；这不是完整的历史工作去重，未来先查索引及历史证据再扩展。

| 指标 | 实测值 |
| --- | --- |
| 选定/记录函数 | 1,000 / 1,000 |
| 伪代码成功/失败 | 999 / 1 |
| 成功率 | 99.9%（仅本次样本） |
| 已记录/IDA 定义函数覆盖率 | 约 0.878% |
| 四批导出累计墙钟时间 | 39.094 秒，不含打开数据库 |
| 其中原生 batch 调用累计 | 22.936 秒 |
| xref 记录 / 直接 call 记录 | 12,220 / 3,192（按函数保存，可有重复边） |
| 字符串记录 | 0；缺数据节，不能解释为游戏无字符串 |
| 重新运行 | 0 个新导出，约 0.281 秒 |
| 本批新增 AI 深入分析函数 | 0 |
| AI Token 消耗 | unknown；接口没有逐任务计量，不填 0 或估算数 |

失败函数 RVA `0x11a2bd4`，Hex-Rays 报 `1411a2c17: call analysis failed`。失败已经持久化，默认续跑不会反复重试；修正分析后用新 profile/output 重试，保留旧证据。

SQLite CLI 见 [工具说明](../tools/re-kb/README.md)。键为 artifact SHA256 + 来源种类 + module base，原生 VA 与 RVA 同时保存；脚本、native 与跨来源发现分开。导入按函数事务提交，可重复导入；伪代码文件校验哈希。exporter 的独占锁、profile 哈希与原子记录支持中断恢复；类型/分析环境变化需新输出目录，不能沿用旧检查点假称最新结果。最终 SQLite 含 1,000 原生函数、2,709 脚本条目、17 条已有证据结论；重复导入数量不增长，5 项针对身份隔离、失败恢复、哈希验证、声明分离和截断输入的测试通过。新增深度分析仍为 0；17 条是已有结论整理，不能当作新分析的函数数。

## 后续协作规则

主 Agent 持有唯一 IDA 写入责任；子 Agent 查询本地索引和已保存证据，不争抢数据库。机械导出不占用 AI 分析；轻量分类优先单个低强度子 Agent，仅关键调用链或风险审查升级。最多三个，不要求保持三个常驻。当前三个子任务收尾后不追加全量分析。

新增开发先检索 `analysis/re-kb/knowledge.sqlite`，再查证据原文及版本，最后才开展最小新逆向。无匹配不代表功能不存在。结论必须包含函数/声明身份、artifact 或明确未知、证据、可信等级、静态/动态区分及待验证项。

UI 下一步见 [UI 优先级](59-re-kb-ui-priorities.md)，机制见 [机制优先级](60-re-kb-mechanics-priorities.md)。RenderDoc、实际游戏和真实头显验证仍不能由静态索引替代。本轮未启动游戏、安装工具、修改 Hook 或执行发布。

本地证据：`analysis/re-kb/capabilities.json`、`pilot-1000/config.json`、`profile.json`、`selection.json`、`runs.jsonl`、`functions.jsonl`、`records/`、`pseudocode/`。原始衍生数据继续被 Git 忽略，不进入 Mod 发布包。
