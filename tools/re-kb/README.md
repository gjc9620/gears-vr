# RE knowledge base

## Full corpus continuation

Final recovery overlays must be preserved: import `class-recovery/manifest.json` after `classes-v2/manifest.json`. `RecoverClasses.cs` / `recover_classes.py` preload constants and enums before retrying only failed classes; original exports and errors remain historical evidence. All 23,641 selected script objects currently have verified files. Build/search their separate FTS index with `pseudocode_search.py --db analysis/re-kb/knowledge.sqlite build-scripts` / `search-scripts QUERY --limit 5`.

`ida_data_recovery.py` works only on the private `data-recovery/snapshot.i64`: load and verify the six same-capture sections, then retry only prior failures once per recovery profile. The completed profile recovered 30 of 382 functions; 352 retain errors and addressed assembly. Do not rerun successful functions. `link_snapshot_data.py --db analysis/re-kb/knowledge.sqlite --manifest analysis/ghidra/render-16812-verified/manifest.json --output analysis/re-kb/data-links.json` validates capture hashes and links existing data references to exact-start literal candidates. These links do not establish string semantics. `kb.py refs` exposes bounded results.

Current corpus scope and active manifests: `docs/re-kb-corpus-status.md`. Script expansion uses `export_scripts.py`, `export_other_packages.py`, `export_classes.py`, then `kb.py import-manifest` for their active manifests and `index_package_objects.py`. These tools preserve source copies and verify hashes; generated class failures remain explicit. The v2 extra-package/class folders preserve original package filenames so UELib emits correct package-qualified names. Earlier folders remain historical only.

Full-text cache search uses SQLite FTS5. After an export window, run `py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite build`, then `py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite search 'sub_140110878' --limit 5`. Build verifies new/changed file hashes and updates each document atomically; unchanged documents are reused. Search returns only short excerpts and source paths, and excludes documents whose hash no longer matches the current function record. FTS text is a local derived index, not semantic understanding. Run the full cache verifier separately to detect external file changes whose recorded hash has not been updated. Do not run another SQLite writer during index construction.

Unattended operation: use the existing owned IDA MCP worker and bounded execution windows (prefer 45–180 seconds; this client times out after 300 seconds even if IDAPython's requested timeout is longer). On a transport timeout, inspect the original worker and its server log for the matching `tool_result` before restarting or closing anything. A lock/status file alone is not proof of liveness or termination. Keep one writer, save the private IDB at checkpoints, and release its lease when ending a work window. Reopen that saved copy to continue.

Run `py -3 tools/re-kb/verify_corpus.py --db analysis/re-kb/knowledge.sqlite --output analysis/re-kb/full-native/audit.json` for a consistent SQLite read-snapshot audit of integrity, foreign keys, cached file hashes and remaining work. Pending functions and failures stay explicit; a successful audit is not proof of whole-game coverage. No AI function-body review is involved.

`ida_corpus.py` runs only in the owned `analysis/re-kb/full-native/snapshot.i64` IDA worker. `run('census')` inserts every IDA-defined function without replacing existing pseudocode. `run('index',45)` verifies function chunks against the historical code snapshot and checkpoints typed incoming/outgoing references plus direct outgoing calls. `run('decompile',45,100)` caches bounded batches, preserving all prior success/failure rows. The time budget is checked between functions/batches, so a complex function can exceed it. Resume the same stage; do not open a second writer.

The corpus uses SQLite WAL with synchronous NORMAL: transactions remain consistent, but power loss may lose recent commits; retained pseudocode/records allow recovery. Each completed function is checkpointed. `full-native/status.json` and `runs.jsonl` provide progress, while `corpus_work` distinguishes pending (0), indexed (1), and indexing failures (-1). A pending decompilation has status `skipped`, not success. `function_coverage` measures the name/address census; `pseudocode_coverage` and reference-index progress are separate.

`snapshot_strings.py --db analysis/re-kb/knowledge.sqlite --manifest analysis/ghidra/render-16812-verified/manifest.json` validates all six snapshot sections before indexing printable ASCII and UTF-16LE ASCII-subset candidates. These are explicitly historical byte-scanning candidates, not IDA-defined strings. They remain separately searchable with manifest/section hashes and addresses. Other encodings, semantic ownership and actual runtime use remain unverified.

Python standard library only. IDA remains the native analysis authority. The exporter is the single IDA reader/coordinator; this CLI never opens or modifies an IDA database. On this Windows machine use `py -3` in place of `python` (the latter is a WindowsApps alias). Run commands from the repository root:

```powershell
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite import-jsonl analysis/re-kb/pilot/export.jsonl
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite import-manifest analysis/ui-v1-script-audit/manifest.json
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite search Draw --limit 20
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite stats
python -m unittest discover -s tools/re-kb -p test_kb.py -v
```

## Export contract

UTF-8 JSONL starts with one artifact header:

```json
{"type":"artifact","sha256":"<64 hex characters>","kind":"runtime_snapshot","module_base":"0x140000000","path":"snapshot.exe","version":"optional build identifier","total_functions":12345,"metadata":{"source":"IDA","identity_basis":"hash of actual snapshot file"}}
```

Then one record per function:

```json
{"type":"function","va":"0x140001000","rva":"0x1000","name":"sub_140001000","bytes_sha256":null,"status":"success","error":null,"pseudocode_path":"pseudocode/140001000.c","pseudocode_sha256":null,"xrefs":[{"from":"0x140001010","to":"0x140002000","type":"fl_CN","iscode":true}],"calls":[{"from":"0x140001010","to":"0x140002000","type":"direct"}],"strings":[{"address":"0x140010000","value":"HUD"}],"metadata":{}}
```

Artifact identity is **SHA-256 + kind + module base**. Supported kinds are `disk_exe`, `runtime_snapshot`, and `unrealscript_package`. Hash the actual artifact being analyzed; do not label runtime evidence with the disk EXE hash. Record source snapshot, IDB provenance, selection strategy and exporter version in metadata. Integers or hexadecimal strings are accepted for addresses. VA/RVA/base consistency is validated. Separate bases intentionally remain separate identities.

Function statuses: `success` requires an existing independent pseudocode file; `failed` requires the actual error; `skipped` denotes no attempt and is excluded from decompilation success rate. Paths are relative to the JSONL directory or absolute. Provided pseudocode hashes are verified; absent hashes are computed. Byte hashes are optional and must reflect actual IDA function bytes when supplied.

Each function and its edge/string replacement commits atomically. Reimporting is idempotent; exports can be appended/resumed and imported again. Earlier completed functions survive a later malformed record. `--allow-truncated` explicitly accepts only an incomplete final line without a newline. Old records outside the new selection remain indexed. Only one importer should run at a time. Historical exports and content-addressed pseudocode should be retained by the exporter; current index rows are replaced by newer observations for the same identity and VA.

UI manifests populate `script_entries`, preserving package hashes, object indices, declaration flags and complete manifest entry metadata. They never inflate native coverage or prove a native implementation/runtime call. Package hashes are reported manifest provenance, not independently rehashed by this importer.

## Evidence and statistics

`add-finding finding.json` accepts optional `artifact_id` (null for unbound historical/mixed evidence), optional `function_va`, `claim`, `confidence` (`low`, `medium`, `high`), nonempty `evidence` (original file/record/address references), nonempty `validation` (performed checks and remaining limits), optional stable `id`, optional `metadata`, and optional externally measured `ai_tokens`. Omitted token values remain SQL NULL. Do not estimate tokens from file sizes or invent session usage. A function VA must exist under that artifact. The artifact row carries the binary version and hash. Preserve unknown or mixed provenance in metadata and leave both identity fields null rather than binding to an adjacent snapshot.

Search returns bounded name/address, string, declaration and finding matches, optionally restricted with `--artifact-id`. Pseudocode remains in separate files; use returned paths for focused inspection. SQL queries over `edges` support call-chain and cross-reference analysis without loading all pseudocode into a model.

Stats distinguish native coverage over the IDA-reported function count, success over attempted decompilations, script declarations, distinct native functions with findings, known finding-token sum and unknown token counts. Coverage measures the IDA census, not guaranteed whole-game code discovery. Session token totals stay unknown unless measured by another authoritative system. Static findings need runtime/RenderDoc evidence before claims about live GPU or game-state behavior.
# IDAPython batch export

`ida_export.py` runs inside an already-open private IDA database copy. The pilot configuration and immutable selection are in `analysis/re-kb/pilot-1000/`. Load the module with `importlib.util.spec_from_file_location` and call `export(config_path, max_new=25)`. The return value is a small progress summary; pseudocode remains on disk. Never pass an empty function list to Hex-Rays: it means all functions.

Config requires `output`, `limit`, `load_address`, and `artifact` (`sha256`, `kind`, `module_base`, `path`); optional `exclude_rvas`, `seed_rvas`, and `source_idb_sha256` preserve selection/provenance. The pilot uses a verified runtime section, not the disk EXE. Use a new output directory after changing source database analysis/types or exporter/configuration. Resume skips completed records, including failures, and does not infer that unchanged machine code means unchanged decompiler analysis. Inspect an abandoned `export.lock` before removing it; never take over a live writer.

The native batch output is retained in `batches/`; subsequent individual extraction retrieves per-function output/error information locally. Xrefs omit ordinary fallthrough, calls include only IDA-resolved direct call references, and strings include only referenced defined strings. Missing data segments and indirect targets remain unknown. See `docs/58-reverse-engineering-knowledge-base.md` for measured scope and limitations.

---

## 中文

# 可再生能源知识库

## 完全继续

最后回收的叠加必须保留:进口 `class-recovery/manifest.json` after `classes-v2/manifest.json`. `RecoverClasses.cs` / `recover_classes.py` 预装常数和enum后, 重试时只选择失败的类别; 原始导出和错误仍然是历史证据。 All 23,641 选中的脚本对象目前已经校验文件。 用 FTS 索引构建/搜索它们独立的 FTS 索引 `pseudocode_search.py --db analysis/re-kb/knowledge.sqlite build-scripts` / `search-scripts QUERY --limit 5`.

`ida_data_recovery.py` 只能私下工作 只能私下工作 `data-recovery/snapshot.i64`: 加载并验证六个相同的抓取部分,然后每个回收配置文件只重试一次以前的失败。完成的配置文件已恢复 30 of 382 functions; 352 保留错误和地址组装。 不要重运行成功函数 。 `link_snapshot_data.py --db analysis/re-kb/knowledge.sqlite --manifest analysis/ghidra/render-16812-verified/manifest.json --output analysis/re-kb/data-links.json` 验证捕获散列并将已有的数据引用链接到精确启动的字面对象。这些链接不会建立字符串语义学。 `kb.py refs` 暴露了有限度的结果。

目前主体范围和活性清单: `docs/re-kb-corpus-status.md`。脚本扩展用途 `export_scripts.py`, `export_other_packages.py`, `export_classes.py`,则 `kb.py import-manifest` 他们的表率, `index_package_objects.py`。这些工具保存源副本并验证散列;生成的类失败仍然明显。2 外包装/类文件夹保存原始软件包文件名, 因此 UELib 发出正确的软件包合格名称。 先前的文件夹仅保留历史 。

全文缓存搜索使用 SQLite FTS5。在导出窗口后,运行 `py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite build`,则 `py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite search 'sub_140110878' --limit 5`。构建验证新/更改的文件散列,并在解剖上更新每个文档;未修改的文档被重新使用。查找返回的只是简短的摘录和源路径,不包括其散列不再符合当前函数记录的文档。FTS文本是 l OCAL 衍生索引,而不是语义理解。 单独运行全部缓存验证器以检测记录的散列未更新的外部文件更改。 在索引构建过程中不要运行另一个 SQLite 写入器 。

未参与操作:使用现有的开发协会MCP工人和有界限的执行窗口(偏好) 45–180 秒; 此客户端在之后的时数 300 秒, 即使IDAPython请求的超时时间更长。 在传输超时时时, 检查原始工人及其服务器日志以匹配 `tool_result` 。仅一个锁定/状态文件并不能证明存在或终止。保留一个写手,在检查站保存私人的IDB,并在关闭工作窗口时解除其租赁。重新打开保存副本的文本以继续。

Run `py -3 tools/re-kb/verify_corpus.py --db analysis/re-kb/knowledge.sqlite --output analysis/re-kb/full-native/audit.json` 用于对完整性、外国密钥、缓存文件散列和剩余工作进行一致的 SQLite 读取和shot 审计。待决功能和失败保持清晰;成功审计不是整个游戏覆盖的证据。没有涉及AI函数机构审查。

`ida_corpus.py` 仅在自有状态下运行 `analysis/re-kb/full-native/snapshot.i64` 开发协会工人。 `run('census')` 插入每个开发协会定义的函数,而不替换现有的伪代码。 `run('index',45)` 对照历史代码快照和检查站输入的进出参考文献以及直接发出的电话,核实功能块。 `run('decompile',45,100)` 缓存 , 保存所有先前的成功/ 失败行。 时间预算在函数/ 夹之间检查, 因此复杂的函数可以超过它。 恢复相同的阶段; 不打开第二个写入 。

程序组使用 SQLite WAL 与同步的 NORMAL : 交易保持一致, 但失去电源可能会失去最近的承诺; 保留伪代码/ 记录允许回收。 每个已完成的功能都会被检查 。 `full-native/status.json` and `runs.jsonl` 提供进展,同时 `corpus_work` 区分待决(0),索引(1),以及索引失败(-)1 有待汇编的汇编具有地位。 `skipped`而不是成功 而不是成功 而不是成功 而不是成功 而不是成功 而不是成功 `function_coverage` 衡量名称/地址普查; `pseudocode_coverage` 参考索引的进展是分开的。

`snapshot_strings.py --db analysis/re-kb/knowledge.sqlite --manifest analysis/ghidra/render-16812-verified/manifest.json` 在索引可打印的 ASCII 和 UTF- 之前验证所有六个快照部分16 LE ASCII 子集候选人。 这些是明确的历史字节扫描候选人, 而不是开发协会定义的字符串。 它们仍然可以单独查找, 并带有显示/ 节散列和地址。 其他编码、 语义所有权和实际运行时间使用都未生效 。ified.

Python 标准库。 IDA 仍然是本地分析权限。 输出方是 IDA 的单一读取器/ 协调员; 此 CLI 从未打开或修改 IDA 数据库。 在 Windows 机器上使用 `py -3` 替换 `python` (后者是 WindowsApps 的别名) 从寄存器根运行命令 :

```powershell
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite import-jsonl analysis/re-kb/pilot/export.jsonl
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite import-manifest analysis/ui-v1-script-audit/manifest.json
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite search Draw --limit 20
python tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite stats
python -m unittest discover -s tools/re-kb -p test_kb.py -v
```

## 出口合同

UTF-8 JSONL首先用一个文物标题:

```json
{"type":"artifact","sha256":"<64 hex characters>","kind":"runtime_snapshot","module_base":"0x140000000","path":"snapshot.exe","version":"optional build identifier","total_functions":12345,"metadata":{"source":"IDA","identity_basis":"hash of actual snapshot file"}}
```

然后每个功能一个记录:

```json
{"type":"function","va":"0x140001000","rva":"0x1000","name":"sub_140001000","bytes_sha256":null,"status":"success","error":null,"pseudocode_path":"pseudocode/140001000.c","pseudocode_sha256":null,"xrefs":[{"from":"0x140001010","to":"0x140002000","type":"fl_CN","iscode":true}],"calls":[{"from":"0x140001010","to":"0x140002000","type":"direct"}],"strings":[{"address":"0x140010000","value":"HUD"}],"metadata":{}}
```

人工身份为 **SHA-256 + 种类+ 模块基数**。支持的种类是: `disk_exe`, `runtime_snapshot`,以及 `unrealscript_package`。将分析中的实际文物挂起;不要将运行时的证据与磁盘 EXE 散列标签。记录源快照、 IDB 出处、 选择策略和元数据中的导出版本。 地址接受整数或十六进制字符串。 VA/ RVA/ bas e 一致性得到验证。 单独的基础是有意的,仍然是单独的特性。

函数状态 : `success` 需要现有的独立伪码文件; `failed` 需要实际错误; `skipped` 表示不尝试, 并且被排除在编译成功率之外。 路径相对于 JSONL 目录或绝对。 只要验证伪代码散列, 则不包含散列是计算。 字节散列是可选的, 必须反映实际的 IDA 函数字节 。 提供时。

每种函数及其边缘/字符串替换在原子上都具有二元性; 重新导入是可附加/ 恢复和再次导入的。 较早完成的函数在后来的不良记录中存在。 `--allow-truncated` 明确接受不新建线的不完整的最后一行。新选择以外的旧记录仍为索引。每次只应运行一个进口商。历史出口和内容地址的伪编码应由出口商保留;当前 i ndex行被更新的观测器取代,用于同一身份和VA.

用户界面显示居中 `script_entries`,保留包散列、对象索引、声明旗帜和完整的显示条目元数据。它们从未充斥本地覆盖或证明本地执行/运行时间调用。包散列报告是明显的出处,而不是独立重排 b y 这个进口商。

## 证据和统计

`add-finding finding.json` 接受可选 `artifact_id` (无约束历史/混合证据的无效),可选 `function_va`, `claim`, `confidence` (`low`, `medium`, `high`),非空(无空) `evidence` (原始文件/记录/地址参考),非空置 `validation` (已执行的检查和剩余限额),可选的稳定 `id`,可选 `metadata`,和外部计量的可选 `ai_tokens`。Ondimited 令牌值仍然是 SQL NULL。不要从文件大小或发明会话使用中估计令牌。该文物下必须存在函数 VA。文物行带有二进制版本和散列。在元数据和lea中保存未知或混合的出处 将两个身份字段都无效,而不是对相邻的快照具有约束力。

搜索返回的有边框的名称/地址、字符串、声明和查找匹配,可选择限制使用 `--artifact-id`。Pseudicode 仍然保存在单独的文件中;使用返回路径进行重点检查。SQL 查询结束 `edges` 支持调用链和交叉引用分析,而不将所有伪代码装入模型。

数据区分本地覆盖度,而不是开发协会报告的函数计数、成功率而不是尝试的编译、脚本声明、具有发现的独特的本地功能、已知的查找数据总和和和未知的符号计数。 保证整个游戏代码的发现。除非用另一个权威系统测量,否则会话符号总数仍然未知。静态结果需要运行时间/RenderDoc证据,然后才能声称存在GPU或游戏状态行为。
# IDAPython 批量导出

`ida_export.py` 运行在已打开的 IDA 私人数据库副本中。试运行配置和不可变的选中项已进入 `analysis/re-kb/pilot-1000/`。加载模块 `importlib.util.spec_from_file_location` 呼叫( C) `export(config_path, max_new=25)`。返回值是一个小的进展摘要;伪代码仍然保留在磁盘上。从不将空函数列表传递给Hex-Rays:它意味着所有函数。

配置需要 `output`, `limit`, `load_address`,以及 `artifact` (`sha256`, `kind`, `module_base`, `path`; 可选 `exclude_rvas`, `seed_rvas`,以及 `source_idb_sha256` 保存选择/ 验证。 试运行时使用经核实的运行时间段, 而不是磁盘 EXE。 在更改源数据库后使用新输出目录 analysis/types 或导出/配置。重置已保存的记录,包括失败,并且不推断未修改的机器代码意味着未修改的解编器分析。检查一个已废弃的记录。 `export.lock` ; 永远不要接管一个现场作家。

原产批量输出保留在 `batches/`; 之后的个人提取本地获取每个函数输出/ error 信息。 Xrefs省略了普通的下传, 调用只包含 IDA- 解析的直接调用引用, 字符串只包含引用的定义字符串。 缺少数据段和 间接目标仍然不明。 `docs/58-reverse-engineering-knowledge-base.md` 以衡量范围和限制。
