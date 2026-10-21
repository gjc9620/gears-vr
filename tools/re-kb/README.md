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

