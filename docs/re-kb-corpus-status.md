# 反编译资料库状态

2026-10-15。资料库位于 `analysis/re-kb/knowledge.sqlite`，所有游戏衍生文件保留在被 Git 忽略的 `analysis/` 内。工具与使用说明见 [re-kb README](../tools/re-kb/README.md)。本报告记录已完成的机械覆盖，不等同于所有函数已被 AI 理解或实际游戏验证。

## 原生运行时代码

对 PID16812 历史代码节 SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，使用原 IDA 库的独立副本完成批量处理。反编译中新发现 2,404 个函数，补入后再次枚举稳定为 **116,328 个**。没有遗漏待处理条目；所有函数的已知 chunks 均经过源快照字节核验，最终函数边界检查无差异。

- 115,976 份成功伪代码，全部独立缓存并建立全文索引，成功率约 99.70%。
- 初始 382 个失败均保留带地址/字节的汇编后备文件，共 52,222 条 IDA function item。加载同一快照数据节后，仅重试这 382 个，恢复 30 个，剩余 352 个保留明确错误；未重复反编译成功缓存。
- 342,844 条直接 call 记录，1,527,426 条按函数保存的 xref 记录。引用可能按所属函数重复；不宣称唯一边数或完整间接调用图。
- 六个快照节校验后生成 359,642 条字符串候选，保留编码、地址、节和 manifest 哈希。候选不等同于已认证字符串对象。

六个历史快照节已在独立的 `data-recovery/snapshot.i64` 中装载并逐字节核验，函数枚举和边界保持稳定。建立 151,092 条数据引用关联及 48,073 条精确起始地址字符串候选关联，均绑定同一 manifest 和节哈希。类型、动态虚调用、字符串内部指针与快照外代码仍有限。磁盘 EXE 和运行时快照身份不同，不能交换地址证据。快照不是原子化的完整进程捕获，不包含所有 DLL、堆或后续动态代码；这里没有声称整个游戏运行时已被完整还原。

## 脚本与包

盘点本地 CookedPC 的全部 **22 个 `.u` 包**，对象索引 **203,764 条**。非 `.u` 资产包、地图、模型和纹理不在本次脚本反编译范围。

- 15,764 个 Function 条目，其中 5,659 个标记为 native 声明；不是 5,659 个已定位的原生实现。
- 777 个 ScriptStruct、479 个 Enum、4,175 个 Const；上述与 Function 合计 21,195 条，全部成功导出。
- 2,446 个 Class 条目全部有类文本。最初 75 个失败通过预先反序列化常量与枚举后恢复；其中 1,106 条直接子常量与独立导出文本交叉检查无差异。
- 总计 23,641 个选定脚本对象全部有文件，逐文件哈希核验无错，并建立脚本全文索引。原有 2,709 个 UI 文件直接复用。

活跃 manifest 按导入顺序为 `scripts/manifest.json`、`other-packages-v2/manifest.json`、`classes-v2/manifest.json`、`class-recovery/manifest.json`，最后一个覆盖 75 个类的失败记录。早期 extra-package 导出因临时文件名 `decompressed.u` 导致逻辑包根名错误，已保留旧结果并以原包名重新导出到 v2；当前 SQLite 对象索引没有 `decompressed.*` 名称。不要把旧 `other-packages/`、`classes/` 当成当前权威导出。

类文本包含 UELib 未解析 token 和类型占位的可能性，不保证源码还原；类名、属性声明也不证明当前运行时偏移。相关来源、缺失和错误见 `script-audit.json` 及逐包 `errors.txt`。

## 检索与继续

```powershell
py -3 tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite search WarPlayerCamera --limit 5
py -3 tools/re-kb/kb.py --db analysis/re-kb/knowledge.sqlite refs 0x140110878 --artifact-id 1 --limit 10
py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite search sub_140110878 --limit 5
py -3 tools/re-kb/pseudocode_search.py --db analysis/re-kb/knowledge.sqlite search-scripts WarPlayerCamera --limit 5
```

`full-native/audit.json` 为最近的完整性、外键、缓存和汇编哈希检查；`full-native/fulltext-build.json` 记录增量索引结果；`script-audit.json` 为脚本文件检查。9 项工具测试通过。所有这些检查仅证明相应资料一致性，不代替语义审阅、RenderDoc 或实际游戏验收。

本次可用输入的自动导出与有限恢复已完成，原生待处理数为 0。剩余 352 个反编译失败需要具体类型、调用约定或 switch 证据，不能靠无限重试保证恢复。新快照、新版本或开发中的具体问题出现后再增量扩展；不重新全量反编译成功缓存，不逐函数提交给 AI。

最低成本策略已写入项目 AGENTS.md：机械任务默认不启用子 Agent；确有需求时先用一个范围受限的轻量 Agent，只对明确难点提高强度，最多三个。当前 17 条已保存语义结论来自已有记录，自动导出数量不计为 AI 深入理解数量。`token-usage.json` 保存宿主实际累计计数，不能据此推导单函数费用；本次协调累计消耗较高，不能宣称实现了理论最低消耗。后续以本地增量脚本和按需检索避免重复开销。
