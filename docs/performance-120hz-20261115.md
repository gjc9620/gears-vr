# 实际120Hz性能调查（2026-11-15）

目标是实际游戏新双眼帧120Hz（8.33ms），目前未达到。XR循环120Hz单独成立，不能作为游戏120FPS证据。

本次工作区 `J:/codexWorktree/gearsVr`。所有运行复用 `tools/multi-instance/isolated_sim.py`，新名字、visible、lowest、render-scale0.5、wait-resources-seconds0；保留私有 `-nomoviestartup` 和180秒玩家等待。无录像，不接管或停止其他会话。只以运行中的 `startup_status.startup_confirmed=true` 确认启动。

## 当前调查：Native 批量校验否决、原 worker 侧栏核实与直接校验复查

唯一原worker `01a1de77-a71a-7f93-b229-e3ecd8cf315e` 名称「GearsVR｜DeepSeek 开发 worker」。本轮实际工具核实：第一次使用旧目标item排序被拒绝，刷新当前分组item后移动成功，随后list_threads返回同一原ID为「GearsVR · 6DoF 开发」itemKeys首项；没有新建会话或修改会话数据库。此前只读accessibility也观察到唯一同名运行worker在gearsVr项目与Recent。回执`analysis/lancer/workers/reflection-contract-deepseek-01/sidebar-verified.json`。

Native来源批量方案不推广：现有BatchedProof在实际填充3640 ordered/scattered和1027 ordered的source-only成本分别为直接4/8基线的1.8873/8.4509/1.9419倍。完整actor记录原报告off/on标签反了，代码k0启用批量却记为off，故真实direct6223.8ns、batch6516.2ns（约1.047x），不是4.5%收益。打印的4checks属于旧fixture全局计数，并非本地逐witness断言计数；历史735检查不能证明新计划生命周期。另发现begin_prepare重填sources不失效p.plan，故生产生命周期从未合格。早期手写mask清空已有witness错误已排除。没有Root/完整Native构建/Game，见`native-source-batching/main-review.json`。

直接校验复查已结束且不推广。worker因夹具编译组合错误交回，Main用独立main_validation.cpp修复测试：3168检查/0失败，包含1..31尾宽逐字节和1024B批次逐字节、掩码padding变值、NoAccess/Guard不重试、第二遍变值/故障、outer Capture全部回调与故障、有效unbatched/invalid回退和观察者所有权变化。显式伪造跨页batch只用于证明前方差异不能跳过后方不可读数据；生产planner仍不跨页。/O2 seam-off汇编实际有两次pass回跳和每批Game movdqu/volatile尾读，不外推portable C++鲜度。五配对反序round组件成本：8065/8070混合shape快约8..10%，纯1024B批次反慢约26%，组件不是FPS。

私有PG SHA `bb0a70767f9f8efcdb3693f890b2d7b0cbc7821aec80d8f6a4645b9868eda1d3` 编译通过；只增加独立header、include和scope的私有4B A/B选择，原witness、mask、Capture、两遍与fallback保留。AY同Game原/新/原实际新双眼中位30.372198/32.541725/31.830404Hz，候选/两原均值1.046314。为排除暖机顺序，AZ反向新/原/新26.758248/29.172566/31.802247Hz，比值1.003691且随时间强烈漂移，不能证明稳定收益。每段15或16有效样本，去两端2秒、QPC同域去重；约3成功查询/admitted frame、qualifications/rejected/unavailable增量均0。两次都startup_confirmed，无录像或额外输入/姿态采集、正常stop0、Native/entry/audio恢复、XR0、精确Game/launcher/XR/sessionowner均退出，控制字恢复1。启动/phase资源快照只有自身登记实例；不声称外部负载恒定。首次反向prepare名称过长被入口拒绝，换新短名后才创建运行实例，无直接EXE启动。证据`direct-validation-review/main-review.json`及AY/AZ-ab-summary.json。

Actor成功路径源码审查已完成。Main纠正worker初稿：max_units116，不是231；230为pose-half地址槽。成功普通事务Backend.validate为N+3次、backend Context.current为2N+3次；exchange仅permit/context/CAS，不重复整Local读回。初始/末尾capture与apply再加3次，另有条件门。完整actor在weapon.address!=0时base+instigator前后共4次proof验证；无武器地址则只有base前后2次，不可无条件说4。没有以旧AG计数推断当前FPS。validate结束和preCAS context是不同保护边界，不批准删除；Native形状缓存和visual合并窗口均缺等价性/所有权证据，未实施。见`actor-validation-call-audit/main-review.json`。

Native固定4/8小读组件已由Main接手验证：worker夹具多次编译/保护地址/初始化失败，原日志保留；Main从生产读函数重新组合常量POD helper，去除无条件to_reader/counter，按编译宏保留reader与私有AB入口。实际direct252检查+6actorfixture检查、reader735检查均0失败。Guard首次调用只尝试一次并失败，第二次独立调用成功；NoAccess、跨页/非对齐、null/overflow及实际actor身份/拥有关系/来源变化/线程/生命周期守卫保持。seam-off计数关闭的优化汇编4/8分别只有DWORD/QWORD Game load/store与SEH，无CRT调用。

五配对反序轮完整组件，真排序中位：identity79.74→70.62ns（0.885628）；actor合成3640/1027sources为13640.0→13387.8ns（0.981510），1027/1027为9258.9→8746.6ns（0.944669）。同一进程同Native内存，只改small reader选择；fixture identity15/8不是零售22/9，不外推FPS，未完整Native构建或Game。证据`native-fixed-small-reads/main-review.json`。无固定20%门槛，但先检查identity另外16/80/146常量copy，避免直接为~2%actor组件收益重复游戏。唯一原worker只实现`native-fixed-wide-reads/brief.md`的三个helper snippet及极小汇编编译，Main复用实际fixture，禁止再重组整套actor测试。

Native固定16/80/146 helper由原worker独立交付，/O2汇编为1/5/9组movups、146再读写末尾word，无CRT调用。Main复用实际fixture扩展后566direct检查+12辅助actorfixture检查、735reader回归均0失败，明确保留跨NoAccess页的短名称完整block失败→原scalar fallback。所有Game观察/次序/比较/identity/current保持，SourceLoop未改。短组件3640组4.1%回退保留；加长十倍iterations五配对轮、记录core位置（有迁移、不改affinity/priority）后identity0.6751、actor3640/1027为0.9121/0.9348，仅组件成本。

完整私有Native SHA `8848d25688c5eb55d26cfb8753b14e850d13e7f23715708e1f59f62eb6c87d5c` 编译通过，PG保持6a79。首次build输出默认GearsCapture.dll而脚本找IntegratedEditor失败，恢复基线CAPTURE_OUTPUT_NAME后重新链接target0，实际最后DLL/PDB匹配，其他GEARS/CAPTURE选项逐项一致。唯一私有4B模式字经PDB/RW非exec绑定；Main控制器抽取真实函数用fakeprocess做11检查0失败，Nativeflag和PGcounter基址分离，无真实进程写入。运行时只写该Native字13次52B，校验NativeScene/body与PG同线程。

BA `perf120-ba-native-fixed-1115` 按既有visible/lowest/0.5/wait0/nomovie入口、实际双眼确认后测85.407秒，六组交错反序5秒配对，QPC同域去重并去每段两端1秒，各18有效新双眼样本/模式，Game与stereo一致。原/候选raw合并中位33.057249/33.302104Hz，1.007407；raw4/6wins、配对中位1.021644。首组资源有其他boltok实例且随后消失，负载不受控，首组不用于收益结论；其余五组3wins、中位1.011267，仍不证明稳定收益，不能把全轮当clean A/B。约3成功查询/frame（非原子边界快照），qualification/reject/frameunavailable与telemetry unavailable均0。其他实例未操作。控制字恢复1，正常stop0、Native/entry/audio恢复、XR0，精确Game/launcher/XR/sessionowner均退出。无录像/额外输入或姿态采集。证据`native-fixed-wide-reads/main-review.json`和BA-ab-summary.json；不推广、不继续原样重跑。

下一步唯一原worker实现`native-source-shape/brief.md`组件：仅审计/预验证由Native自身构建后不变的Source地址/大小描述符形状，在所有builder/reset生命周期失效；热循环仍逐条按原顺序实时读取并比较全部Game见证，保留两次closing/身份/freshness/noRetry，不能缓存Game当前性/可读性。先证明Native所有权与测试，再由Main决定完整构建/Game。

生产6a79与手动入口保持，实际120Hz仍未达。
## 两项必需字段前缀合并 AX 未证明游戏收益（未推广）

原worker因反复shell/组合错误交回，Main在独立task目录重组真实读取器：两臂使用当前生产semantic_name_success和fresh_name、开启实际MAIN_MOUNT_PROVIDER宏，并在每次double-capture安装TickScope；原capture/strict-fields/非武器/bDeleteMe路径文本相同，删除旧AS helper遗留。12shape/5轮配对完整double-capture基准：全武器候选/原版0.7351..0.8450，混合0.8622..0.8918，非武器同路径0.9725..1.0298。仅组件成本，无VM/Frame外层或真实布局保证。

Worker定向夹具交回后，Main修复大Proof栈溢出、测试对象注册表计数未更新、旧场景地址复用及页保护未真正触发问题；增加正向地址断言、真正跨保护页及双方初始Guard重置。旧崩溃/6失败记录保留。最终36检查/0失败，含派生优先/错误类型遮蔽、大小写/Greek、缺失/重复/null/输出别名、页尾NUL/未终止/PAGE_NOACCESS/PAGE_GUARD；源proof失败后仍无permit。SourceProof.count/used仅unique witness ranges/captured witness bytes，非raw read calls。完整capture两臂472ranges/11639B。证据`analysis/lancer/workers/reflection-contract-deepseek-01/multi-prefix-fields/main-review.json`。

独立冻结树只改query_local_fields.inl和weapon_equipment_inventory_fields.inl。私有A/B PG编译目标GearsPlayground exit0，SHA`ff043c480ad11d5277503cb3e3e82db7d3b7a882da8dd1eb6fdce8aaf4067153`；仅Mod-owned4B模式字，旧typed分支逐字保留。首次all-target构建日志GBK解码失败，未据此声称full-build通过；随后显式PG目标完成。无Root/公共入口改动。

AX `perf120-ax-required-prefix-1115` 沿原visible/lowest/0.5/wait0入口、实际startup_confirmed后测85秒，原→候选→原三段各20秒，去头尾2秒后新双眼中位 **28.962220→23.881302→25.493085Hz**，有效15/16/16样本，ON/两OFF均值0.877097，本次不证明收益，故不推广。三段约3次成功库存查询/admitted frame，资格重建/拒绝增量0；首OFF一帧unavailable。资源快照均有同一peer`lancer-merged-1115-v`，未操作它；两段OFF也漂移，不外推固定12.3%回退。Mode恢复1，正常stop0，无强退，Native/入口/音频恢复、XRexit0，Game/launcher/XR/sessionowner身份已复查退出。回执`analysis/lancer/perf-runtime-20261115/perf120-ax-required-prefix-1115-ab-summary.json`与同名`-summary.json`。

保持原production6a79/Nativebf595手动候选。唯一worker继续同一会话，下一项仅Native来源校验批量读取成本spike，保留完整原witness、两个鲜读遍次和原身份/输入保护；成本未通过前不扩展完整构建或Game。任务`analysis/lancer/workers/reflection-contract-deepseek-01/native-source-batching/brief.md`。

## AW 生产热点与唯一 worker 字段实验

AW `perf120-aw-production-hotspots-1115` 使用未变生产PG6a79/Nativebf595，沿既有visible/lowest/0.5/wait0入口确认实际双眼后观察60.282秒，无录像/输入/姿态采集。最后50秒按telemetry自身QPC去重后49有效样本，Game/新双眼中位30.735347Hz，XR120。启动另有`lancer-merged-1115-v`，不是A/B提升证据。正常stop0，无强退，Native/入口/音频恢复true、XRexit0，Game/launcher/XR/sessionowner精确身份已复查退出。

复用现有RIP-only采样器，精确Scene线程20.000194秒、998accepted/998attempts，线程CPU19.46875秒。暂停合计25.8769ms、最大0.8784ms，restored/success/module_set_coherent均true。Native430、PG453、Game62、其他53个指令位置；不是完整栈或inclusive CPU百分比。匹配本地PDB，Native source_loop_exact172（8B实际读取/比较行116）；PG memcpy111、固定4/8B读取138、BatchedProof.validate67。ICF折叠同宽模板，不能由array4符号断言实际类型。回执`analysis/lancer/perf-runtime-20261115/perf120-aw-production-hotspots-1115-rip-summary.json`和同名`-summary.json`。

复核旧AO后停止事件分派实验：event_hook名称读取估算累计14.133ms，字段名称11487.622ms、raw_identity名称7963.472ms。它们是历史instrumented估算，不直接外推当前生产CPU/FPS，但足以降低小路径优先级。保留未编译候选，不追加矩阵/整DLL/Game，见`event-dispatch/main-priority-decision.json`，不能声称候选通过验证。

唯一真实worker仍为`01a1de77-a71a-7f93-b229-e3ecd8cf315e`，稳定名称「GearsVR｜DeepSeek 开发 worker」，Kaku/deepseek-v4.1-flash/high。额外02已按用户要求Ctrl-C取消、CLI exit1，并用正式应用工具归档。原会话由当前桌面拥有者续接，未抢占active writer或新建替代。当前任务`analysis/lancer/workers/reflection-contract-deepseek-01/multi-prefix-fields/brief.md`：仅武器Inventory+CharacterSlot两项必需标签的一次鲜读多前缀排除；非武器、bDeleteMe、类型和闭合保护不改，无跨查询缓存。先测真实完整double-capture成本。首次helper变量重名编译失败，旧基准缺生产semantic_name_events的组合问题也在修正；尚无合格候选成本结果，无Root/生产改动。

Ghidra名称表xref两次查询均超过300秒等待上限，已关闭自有目标，不再重试。IDA batch有界解码已得到名称池/字段遍历候选，但名称池有扩表及延迟初始化，字段候选比较完整8B Name且类型迭代helper未核实，与Mod文本契约不同；不作为可调用替换。运行函数表的packer尾未排序且重叠，仅主代码区有序唯一范围用于定位；展开载荷缺失、范围可能为fragment。见`analysis/lancer/engine-reflection-20261115/runtime-code-av/main-decision.json`。

## 历史调查：真实 CLI worker 语义验证与 AV 运行代码

真实持久 Codex CLI 会话 `01a1de77-a71a-7f93-b229-e3ecd8cf315e` 使用 Kaku / deepseek-v4.1-flash / high，命名「GearsVR｜反射查找验证｜DeepSeek｜01」，通过正式应用工具移入「GearsVR · 6DoF 开发」并打开。默认 npm CLI 曾因 package identity 不能执行命令；改用本机 packaged Codex CLI 后，原会话 Get-Location 和后续源码/编译命令实际成功。未修改全局 provider、权限或凭据，未使用 OpenCode。

Worker 直接包含旧/生产文本读取器并构建合成类/字段/名称表。Main review 修正场景回执时间点、Greek 无分歧标记、空槽/越界/头不匹配分离；最后修正头不匹配查询目标并增加修改前成功对照。最终 `/W4 /WX /O2` 编译成功，63 检查/0失败、12行，实验 exact-index 解析在不同索引同名、ASCII case、Greek locale 三类场景不同于原文本选择器。因此不推广此替换，不外推任何未知引擎 FName API。证据 `analysis/lancer/workers/reflection-contract-deepseek-01/main-review.json`。原会话保留，当前轮已结束。

现有 RE KB 未提供原生反射地址。磁盘 PE 打包，历史 IDA 缓存虽 input SHA 匹配，但 `0x141c34800` 为全 FF、无相应符号/xref，不足以证明 ABI。IDA MCP 缺 ida_nexus；现有 IDA batch CLI 在私有缓存副本导出并正常退出，未改全局插件或其他数据库。

全新 `perf120-av-runtime-code-1115` 沿既有 visible/lowest/0.5/wait0 入口启动，startup_confirmed=true 后测60.265秒；最后50秒按 telemetry QPC 同时钟域、去重复观测，49有效样本，Game/新双眼中位30.798449Hz、XR120。生产Mod未变，启动另有 `hb-empty-reload-1115-r14`，且本轮做静态代码提取，不作为优化A/B。无录像或额外输入/姿态采集。stop0，无强退，Native清理/入口恢复/音频恢复正常、XRexit0；Game/launcher/XR/sessionowner精确身份均已退出。回执 `analysis/lancer/perf-runtime-20261115/perf120-av-runtime-code-1115-summary.json`。

仅自有Game主模块的PE可执行区、当前MEM_IMAGE可执行页只读提取，每页双读相等、身份和磁盘SHA前后复核；原名称/对象/输入数据区不在提取范围。生成仅分析用PE载体（59937615B执行区，其他数据置零、头已改写，非可运行原文件）；Mod hook可能存在，不假定全字节原始。代码载体SHA `193391fdbe82d24cd739a1be0c313dd79566b767c725a6a265d1be7410408074`。Ghidra离线名称表xref分析仍在运行，尚无原生lookup ABI结论。文件仅在私有analysis，不进入交付包。

## 当前交付：生产跨回调校验组合（AU）

生产两文件已最小合入Root；修改前两文件均与审查基线完全一致，先备份，未覆盖其他会话内容。完整Root PG编译通过，Root完整二进制本身未进行Game验收；交付使用冻结、已测算法对应的production DLL `6a79d1a60c52e48d887b1c18141371102151c296a5d70c0e69efce6016b0e3e0`，无A/B模式旗标。沿用原allowlist stager，通用依赖预检和恢复个人0.5/游戏路径后的最终inventory/hash通过。新独立入口：`analysis/lancer/perf-persistent-prefix-20261115/manual/GearsVR Launcher.exe`；原入口及前一候选保留。

全新AU通过原visible/lowest/0.5/wait0入口取得startup_confirmed，测60.266秒；去启动后10秒，51有效样本的新双眼/Present中位26.642995Hz，XR120。启动有其他实例`lancer-recoil-1115-p`，不得与AT顺序数据作因果提升/回退比较。正常stop0，Native清理、入口与输入恢复true，XRexit0，四个自有进程身份复查已退出、无强退。证明生产DLL实际启动/出双眼/清理，不是120Hz或全玩法/实体头显验收。

交付回执：`analysis/lancer/perf-persistent-prefix-20261115/artifacts.json`；运行：`analysis/lancer/perf-runtime-20261115/perf120-au-production-persistent-1115-summary.json`；合入：`persistent-prefix-root-integration.json`。历史外部worker已停止；当前仅使用上节真实Codex CLI/Kaku/DeepSeek会话，不恢复已删除的bridge技能。

## 跨回调布尔校验同实例 A/B/A（AT）

实际120Hz仍未达到。当前前缀候选组合已审查AN的两文件方案，私有production DLL编译为`6a79d1a60c52e48d887b1c18141371102151c296a5d70c0e69efce6016b0e3e0`。另建私有同DLL A/B：最外Scope锁定原/跨回调模式、嵌套继承，原模式使旧persistent记录失效；原observe函数体逐字保留。实际Scope路由43、macroOFF3定向检查通过，未重跑旧1275矩阵。

全新AT按既有visible/lowest/0.5/wait0入口启动，实际startup_confirmed后测85.453秒，无录像。原→跨回调→原三段新双眼中位31.739076→32.688180→30.898734Hz；ON比OFF均值高4.372%。各段去头尾2秒，有效14/16/16条，首OFF另1不可用率排除。XR仍约120，资源快照均仅本实例；Mod已有计数每admitted frame约3次成功库存查询，qualification/rejected/frame_unavailable增量均0。正常stop0、mode恢复1、Native/入口/输入恢复、XRexit0，四个自有进程身份已复查退出，无强退。

这只是短测算法收益，两模式共用AN semantic-success epoch/null-observer开销，不能当旧生产二进制布局严格相同或固定FPS保证。A/B PG为`f6f9a777f5b2abacc4967991dda5734bb22300e9a1135d81fd13c02d60337d5f`，不交付；仅16B对精确PDB/PE绑定的Mod自有4B模式字写入，原Game双遍新鲜校验及NativeNames前提、输入/身份/生命周期保护均保留。无Game对象/姿态/输入写入。

证据：`analysis/lancer/perf-runtime-20261115/perf120-at-persistent-samegame-1115-ab-summary.json`及同名前缀`-cleanup-recheck.json`；`analysis/lancer/build/persistent-prefix-ab-components/log/review.json`。下一步最小合入生产两文件、Root编译和独立候选打包，保留其他会话改动；生产二进制启动验收与120Hz仍分别待完成。

## 库存节点批量查询同实例 A/B/A（AS，未推广）

实际120Hz仍未达到，当前交付继续保留下节的字段前缀候选。本轮没有新增性能代码到Root或交付包。

复用现有多开入口启动全新 `perf120-as-inventory-samegame-1115`，visible/lowest/0.5、wait0、无录像。实际 `startup_confirmed=true` 后测85.422秒，窗口和任务栏可见回执成立。三段各20秒，原查询→批量查询→原查询；同一Game、同一PG、同一NativeScene/PG线程65248。每段资源快照均有同一个其他登记实例 `lancer-rank-1115-o`，未操作它；这些快照不能证明外部负载完全恒定。

去掉各段头尾2秒并排除不可用率后，新双眼中位为 **28.688120→27.047587→29.734749Hz**，15/16/16有效样本、无不可用率。ON/两段OFF均值为0.925925，本次未证明收益，故不推广；不能将短测7.4%差异外推为所有负载下的固定回退。XR各段中位仍120，与实际游戏新帧分别记录。

为排除“查询失败所以少做工作”的假提升，只读精确PDB/PE绑定的六个现存Mod自有计数器。各段frames为568/551/599、idle hits为1704/1653/1794，约每帧3次成功查询；三段qualification/rejected/frame_unavailable增量均0。它们是边界快照，不是逐帧原子日志。A/B只写Mod自有、对齐4B、RW非执行数据模式字，共16B写入并恢复初始1；没有Game对象、输入、姿态、代码或页权限写入。PG为`047b74829108006639e87da1fc6ac174aa1a9c4386a59d8d15d46a287f162bb8`，仅用于此私有诊断，不交付。

两次完整组件方案保留：初三标签查询因非武器缺CharacterSlot而明显退化；最后按已识别class选择2/3标签，成功路径多数合成形状改善6–26%，部分接近无收益，不能由组件推断整机FPS。实际抽取包含完整identify的tables/raw_identity两遍/tables关闭、instance/registered/typed_at，每次做capture两遍与same；未把原VM和最终Frame链闭包成本计入组件。安全审查发现Partial模式在component_names开启时不能用短名字上限，已仅在最终私有A/B树强制完整128字符，避免将无关长字段误判为fault。普通fields/strict保持原清零；新helpers留在idle_inventory，原pair header逐字不变。

最终私有路由89、component开启173、macro关闭3、旧pair47项定向检查通过，包括可选缺失、首同名错误类型、真实NOACCESS/单次Guard失败无重试。外层before/after capture、same、function_current、4次原VM资格和最终actor/link/current闭包保持原文。没有为了FPS关闭玩法或放宽身份/输入/生命周期检查。

另一个16B固定复制候选的1806项故障/输出检查通过，但完整field形状收益不稳定、部分回退，已停在私有组件；没有集成或为它开Game。

AS正常stop0、无强退，Native清理/入口恢复true，输入恢复true、XRexit0；自有Game/launcher/worker/XR四个PID+creation+path身份均已复查不存活，控制句柄和exec会话关闭。没有停止其他会话。新负结果后不继续调库存批量方案，也不重跑copy16。

证据：`analysis/lancer/perf-runtime-20261115/perf120-as-inventory-samegame-1115-ab-summary.json`、同名前缀`-startup-confirmation.json`与`-cleanup-recheck.json`；源码/检查位于`analysis/lancer/source/perf-inventory-node-ab-20261115`和`analysis/lancer/build/inventory-node-ab-components/review.json`，16B组件为`analysis/lancer/build/pg-copy16-components/review.json`。`tools/multi-instance/README.md`仅把当前推荐命令明确为新GUID名字、visible/lowest/0.5/wait0，保留原入口与旧可选full-hd说明。

下一步仍针对typed raw_identity/fresh_name与Native/PG proof成本。既有persistent名字观察器局部查询更快但先前顺序测量整体FPS不明确；应先审查与当前前缀候选的最小组合及同实例控制能否回答这一具体不确定性，不直接推广缓存或重复原顺序跑法。

## 当前交付：字段前缀否定与同实例A/B/A（AR）

实际120Hz仍未达到。普通查找→优化查找→普通查找在**同一隔离Game、同一PG二进制**的短测新双眼中位为27.793144→28.800845→26.845826Hz；本次ON比两段OFF中位的均值高约5.42%。每段20秒，去掉头尾各2秒，15/15/16有效样本，ON另1条不可用率单独保留、不当作0。XR各段中位120，三段资源快照均仅本实例。它支持短测小幅收益，不证明稳定120、全玩法/实体头显或生产二进制固定帧率。

真实来源定位：AO完整DLL寿命302,953,570次反射调用，ordinary name从field约201,190,962；不能把启动期混成稳定帧。AP在确认双眼后+10..40秒读取两次**Mod自有计数器**，窗口30.0621493秒、读取0.2409/0.3029ms；精确DLL与PDB证明NativeScene/Bodyowner/PGgt均TID45472。窗口field触发83,980,925次ordinary name，property触发97,988次field；pointer/deleted各24,352/23,181次property。raw_identity触发37,283,636次fresh_name。相邻cost层级嵌套，计时受探针和抢占影响，不相加当CPU selftime。诊断PG不是交付版。

AO抽样4,520,497个反射起点，4,466,359（98.8024%）在系统报告的高EfficiencyClass1性能核；bootstrap power control/state均0，不能由此推断自动QoS。没有支持E核主导执行的证据，未改affinity/priority/QoS/电源计划。CPU端点不是连续驻核证据。

生产优化只改global name可选negative_prefix与field默认路径的named_field：原header/slot/index仍一次、原次序；仅当原完整最大字符span有效且在同页时，精确读4wchar做case-insensitive否定；可能匹配继续完整原text/NUL与最终比较，跨页/非法span保留原路径，失败不retry。不保存名称或Game字段缓存；专用输出只有name成功才消费。component_names开启分支与普通named原文不改。69default/72component定向检查、3arg旧seam与8组完整field长链benchmark通过；这些检查数不代表独立游戏场景。

AQ生产二进制eb366f44…f2664d实际启动确认并测60.359秒，新双眼22.297991Hz，有lancer-support-1115-k并发；未改follow/remainder阶段也更慢，不能从跨实例结果归因。因此另做AR同实例控制：29个模式路由检查及8个自身fixture控制安全检查通过后，仅切换精确PDB/PE/RW非执行区证明的Mod自有4B模式字，OFF20→ON20→OFF20；无Game对象/输入/姿态、代码或保护权限写入，没有新线程/hook。共16B模式数据写入，恢复初始1并读回；NativeScene/PGgt相同TID33676。85.469秒测量后正常stop0，无强退，自有Game已退出、Native清理/入口恢复true、XRexit0。阶段边界与telemetry均为QPC/perf_counter_ns，未与GetTickCount64观察等待的不同epoch混算。

Root最小合入playground.cpp和playground_controller.inl；完整Root PG编译通过，改动函数与生产AQ源码一致，其他会话改动保留。Root和包均**不含A/B旗标或计时探针**。最新独立手动候选：`analysis/lancer/perf-field-prefix-20261115/manual/GearsVR Launcher.exe`，Native bf595fa5…d3d7d /PG eb366f44…f2664d，753文件/57015488B；原allowlist stager generic依赖预检及恢复个人0.5配置后的inventory/哈希校验通过。包未自动运行，原公共/用户手动入口不替换。

证据：`perf120-ap-field-callers-1115-steady-callers.json`、`perf120-ar-prefix-samegame-1115-ab-summary.json`、`field-prefix-root-integration.json`，均在`analysis/lancer/perf-runtime-20261115`；组件`analysis/lancer/build/field-prefix-components/review.json`；包`analysis/lancer/perf-field-prefix-20261115/artifacts.json`。下一步调查仍重复的pointer/deleted字段选择与typed raw_identity，优先复用已有query-local多字段查找，保留首匹配/类型/closing检查，不做未有证据的全局缓存或调度调整。

## AM后续采样与AN组合复核（未推广）

AM沿用已选fixed-copy候选，在0其他登记实例时启动确认，测60.235秒后正常stop0、无强退、自有Game已退出。实际新双眼中位27.746112Hz，XR120，说明顺序测量仍有波动，不能把单次AL29.7838当成固定保证。

优化后精确Scene线程的20秒采样：999accepted，19.375CPU秒/20.000455wall秒；PG529、Native343、Game71、其他56。暂停合计26.0208ms、最大1.8261ms，restored/success/module_coherent均true。主要Native source_loop_exact141样本；PG memcpy118、fixed-copy code135、BatchedProof.validate55。ICF可能把同宽模板折叠到array4符号，不能由该符号断言实际读取类型。它是单线程指令位置分布，不是完整栈或inclusive百分比。

两项固定lookahead8预取组件均已停，不集成、不新增Game：Native原3shape/4mode完整actor测试基本回退3–5%；PG原4shape完整validate wall改善1.3–14.5%，未达稳定收益门槛。只有smoke/汇编检查，没有为未通过性能门槛的方案追加Guard深测。证据位于`build/native-prefetch8-components`、`build/pg-validator-prefetch8-components`。

AN把已审Y布尔witness复用方案重放到fixed-copy，并沿用Native config-revision。只改两个私有PG文件；原Scope逐字相同，旧1275/32/compare/cost证据按未变源码复用，新增fixed-copy与name observer交点12checks通过，完整PG编译通过。实际startup_confirmed后测60.297秒，0其他登记实例，实际新双眼29.103519Hz，XR120；正常stop0，Native清理/入口恢复true、XRexit0。

AN12条admitted装备stage early样本中位7.2106ms（AL8.63325）；资格capture/seal中位0，仍14次验证、每遍8070reads、30052ranges/435162Bwitness。posttick日志含较多未接纳/不可用帧，不能拿其2.8813ms总中位当成功的装备耗时。整体FPS没有证明收益，故Root与手动候选继续保持AL，不替换为AN。

新增只读硬件核对：i5-12600KF、10核16线程，Windows11 build26200；GetSystemCpuSetInformation报告Group0逻辑0..11为EfficiencyClass1，12..15为Class0。尚未测得Game执行核分布，未更改affinity、priority、QoS或电源计划。下一私有探针仅采样反射caller成本、代码RVA和执行CPU桶，不保存Game名字、对象、输入、点位或栈。CPU拓扑回执：`analysis/lancer/perf-runtime-20261115/cpu-topology-20261115.json`。

## 最新状态：AL固定宽度读取候选

实际游戏/新双眼中位 **29.783826Hz**（51个排除启动前10秒后的样本），XR120；目标实际120Hz仍未达到。AL经原isolated_sim入口、全新名字、visible/lowest/0.5、no-video启动，startup_confirmed=true后测60.219秒，启动时0其他登记实例；正常stop0、无强退、Native清理/入口恢复true、XRexit0，自有Game已退出。其他会话未操作。

PG只把get/read_at中编译期已知4/8B的memcpy放进同型SEH模板，省CRT运行时长度分派；unknown-size copy、16B等聚合、读取范围/数量/observer及unreadable顺序不变。实际内存84检查涵盖跨页destination partial-output/get忽略false、PAGE_GUARD不retry、capture范围/字节/顺序、overflow拒绝；原reader seam46320检查也通过，但这套旧fixture默认不走新模板，语义证据分开记录。完整query合成5/30/95字符cold/warm中位改善18..36%，不能当作实际FPS收益。

AL装备阶段12个admitted early样本中位8.63325ms，AK9.1714ms；actual新双眼29.7838 vs28.7777Hz。AK启动有1peer、AL0peer，是顺序运行，不能给出严格单变量整体FPS提升结论。Native保持bf595fa5…d3d7d，PG新f8144f1cbd84c0f095cc872a28ca3ea5e49bccbe0d8090afb057a1a85359f165。

Root最小合入playground.cpp与weapon_grip_damage_metadata.inl，其他会话内容保留。首次多hunk合并漏endif，C1004编译阻止；修正后完整RootPG编译成功，两个改动函数文本与实际测量源码相同。未把初次失败写成成功，未改fault/identity/permission以过测试。

最新独立手动候选：`analysis/lancer/perf-fixed-copy-value-20261115/manual/GearsVR Launcher.exe`。原allowlist stager打包，753文件/57116463字节，generic依赖预检通过；个人0.5/游戏路径映射恢复后，最终inventory及DLL哈希核对通过。原公共/用户手动入口未替换，打包未开游戏；实体头显验收未做。精确回执：`analysis/lancer/perf-fixed-copy-value-20261115/artifacts.json`。

下一步应重新测剩余主线程热点，再决定算法调整；不重复推广已经无净收益的page-gap、runtime4/8分支或未有真实batch比例的tiny validator组件。

## 最新状态：主线程 Mod 热点（AI/AJ）

AI 实际新双眼中位27.9081Hz，AJ26.2347Hz；XR仍120。两次均启动确认、60秒无录像、正常停止0，Native入口恢复与XR退出正常。AJ有其他会话，跨轮FPS不能作严格单变量收益证明；实际120Hz尚未达到。

AI 对精确身份的自有Scene线程做20秒、1000次RIP位置采样：19.980秒wall内耗19.594CPU秒，Playground516、Native367、Game73、system44样本。每次立即恢复后才记录；暂停合计21.13ms、最大0.252ms，最终恢复及模块身份检查通过。这是指令位置分布，不是完整调用栈或每个函数inclusive耗时。

AJ仅在六段已逐字节核对、零栈展开的CRT叶函数命中时读取一个8B返回代码地址。1000次采样中367次命中，实际读取2936B，全部映射至模块代码；没有完整栈、通用寄存器或姿态快照。暂停合计24.58ms、最大0.290ms，正常恢复。PG memcpy238次的返回位置为playground.cpp copy；Native38次memcmp返回位置对应重复自有Config比较。Game mesh/pose当前性比较保持原路径。

Root已经集成两项定向PG小修：未进入的Gnasher invocation token提前返回；metadata scratch激活保留epoch完整清零，只省去不会被读取的旧地址数组清零。Receipt原功能/空token读取数量回归通过，metadata46320检查通过；AI/AJ使用这两项修改，但未证明整段FPS净收益。当前手动交付仍为X，尚未替换。

下一候选把Native-owned约22KBConfig改为只读视图及两个原发布点维护的revision；发布时仍完整比较，世界门和原锁保持，Game逐字节证明不变。735启用、18耗尽后retained cleanup、10store及原717legacy mutable检查通过；完整Native Release编译通过，AK实际验证及Root最小集成已完成。它明确不保证检测绕过publisher的任意raw patch或RAM损坏。

证据：`analysis/lancer/perf-runtime-20261115/perf120-ai-idle-hooks-1115-rip-samples.json`、`perf120-aj-leaf-callers-1115-leaf-samples.json`、`idle-hooks-root-integration.json`；兼容候选审查：`analysis/lancer/build/anchor-config-compatible-components/review.json`。

AK结果：startup_confirmed=true，60.219秒，actualgame/新双眼中位28.777738Hz，XR120；启动时其他会话hb-left-loss-1115-r2，未操作该实例。stop0、无强退、自有Game已退出，Native清理/入口恢复true、XRexit0。原C0调用214context/122owner未减少；28个earlyquery中位total6.6415ms/context4.71825ms，较AG7.32635/5.11905为不同轮观察，不能据此保证整场同等收益。

AK固定PG copy帧采样1000次，231次memcpy命中；226次匹配已验证copy返回位置并映射第二返回代码，实际读取3656B、无完整栈，恢复/模块一致性成功。归属：完整BatchedProof validate43、ordinary name56、fresh_name55、get<unsigned/Name>46、registered_known20等。下一项只评估编译期4/8B的SEH memcpy模板；不重复推广此前失败的运行时宽度分支。

Root集成仅Native config store/capture/binding/world发布点，另新增单独revision/store测试并登记CMake目标；原legacy fixture不改，其他会话掩体改动保留。完整Root Native构建及6个相关测试目标通过。独立手动候选：`analysis/lancer/perf-anchor-revision-20261115/manual/GearsVR Launcher.exe`，Native bf595fa5…d3d7d/PG c630d4a8…959a，753文件/57100073字节；原allowlist stager generic依赖预检通过，恢复个人0.5配置后的最终清单及DLL哈希通过。Full generic预检要求空game_path，因此个人化后只核对清单/配置/哈希，不伪称再次全generic通过。包未自动运行，原public/owner入口未替换。无Game、PDB或逆向资料进入包。

## 已有实际证据

- I同一场景正常卸下Playground后：完整Mod游戏/双眼中位17.899Hz；FireDisable0→HandStop5→PGShutdown0且原ProcessEvent恢复后，Native+XR为26.765Hz。只是定位实验，不以关闭功能交付。
- L精确PID/创建时间/路径验证的线程CPU观察：Native Scene线程37100在20.015秒内消耗19.469CPU秒，接近占满单核。没有内核栈采样证据。WPR因系统策略0xc5585011拒绝；未更改策略，没有遗留录制会话。
- M的Playground布尔liveness读取证明：每帧13次验证，30052个唯一范围、440810字节，原13210读取/遍。N只改读取合并为8065次/遍，完整字节及两次复读保留；实际FPS仍约18.2，不能声称大幅改善。

## C0元数据热点

| 运行 | 目的 | C0总中位ms | 提交中位ms | 实际新双眼中位Hz | 清理 |
| --- | --- | ---: | ---: | ---: | --- |
| R `perf120-r-c0-budget-1115` | 原路径分阶段计时 | 13.6608 | 5.6590 | 18.9333 | 正常0 |
| S `perf120-s-actor-witness-1115` | 整角色字节证明复用候选 | 25.6425 | 15.73035 | 15.8488 | 正常0 |
| T `perf120-t-actor-cost-1115` | 撤下S，原解析器详细计时 | 15.8394 | 5.98595 | 17.8820 | 正常0 |
| U `perf120-u-gpu-cost-1115` | 原路径，关闭C0时钟并测CPU/GPU | 未开计时 | 未开计时 | 19.1931 | 正常0 |
| V `perf120-v-source-loop-1115` | 原顺序source整段SEH读取 | 7.70875 | 2.9157 | 24.7423 | 正常0 |

这些运行均保存实际启动确认，XR中位120Hz。C0统计为固定32个早期query记录，排除前4个；它不是整场Mod总耗时。各操作计时有嵌套，禁止相加。R并行会话为lancer-controls-1115-d；S/T为lancer-contact-1115-e，跨运行FPS不能作严格单变量因果比较。

T共16个原角色解析器样本，排除前4个后12个成功样本：单次校验中位42.2μs，元数据proof37.05μs，capture2.15μs，identity7.6μs（identity嵌在proof内）。实际base sources3640/identities22，weapon sources1027/identities9。旧synthetic fixture仅29+16个source，不能据其读数预测游戏性能。

S虽通过18767组件检查和一次源码审查，实际变慢，已从Root撤下其绑定替换、prepare/commit scope、report和新增头文件。私有源码/二进制/评审/失败数据全部保留。未将S发布到公共或用户手动入口。

cinematic lookup独立实验也未集成：10764个输出/读取序列/变异/故障parity案例通过，但常见浅链40/128身份性能混合，128身份同Pawn回退约41%。真实capture只占小部分，故不再追这一方向。

## 当前下一步

针对已经保存的4667个metadata source编译固定读取计划：只省重复的小块调用与数据编排；每次current继续真实读和比较原字段，保留原identities检查及各current调用边界。该固定计划在3640/1027 source组件基准回退17..79%，已否决，未集成。后续单个POD SEH的原顺序精确读取循环在组件中减少68..75%的source-only耗时；真实内存149689检查及旧reader717检查通过，独立增量审查未见阻断。Native整段读取方案已集成Root、编译并完成V实际游戏测量：C0总7.70875ms/提交2.9157ms、新双眼中位24.7423Hz。214次Context及122次owner检查数量未减少。新回归测试加入未对齐/跨页/最后可读宽度后150447检查通过，Root reader717/disabled1/independent1也通过。所有性能收益必须再经实际游戏测量。

机器记录与原始结果：`analysis/lancer/perf-runtime-20261115/runs-summary.json`、`actor-witness-outcome.json`、各运行`-startup-confirmation.json`/`-body-budget.json`、T的`-actor-cost.json`。当前状态：`analysis/lancer/performance-current.json`。

实体头显、完整玩法、多实例长期稳定和实际120Hz均未通过本次验收。未创建Git提交。已按用户要求删除 `C:/Users/11/.agents/skills/codex-opencode-bridge`，技能不再用于工作。

U只读CPU/GPU计数：精确游戏PID50656/Native Scene线程32396，在20.018秒内使用19.422CPU秒（单核97.02%）。11个有效GPU Engine样本3D中位4.606%，最大4.779%，其余该进程引擎为0。这支持继续优先游戏线程CPU路径，不是GPU帧时间或完整硬件瓶颈证明。U启动确认且正常清理0，无其他会话操作。


已打包独立手动候选：`analysis/lancer/perf-source-loop-20261115/manual/GearsVR Launcher.exe`。复用现有allowlist stager，753文件generic preflight通过，个人配置与candidate-state补齐后754文件、57097317字节；不包含游戏资源、模拟器、存档和旧运行图像/日志。Native SHA256 `3a7ba01012e4d6577704d34020c8adc87e90f8996901de4ecc3a68777b170f57`；PG `9d43a073be131f2f4e8d0332b00c465bdc46c22565e98b12475871f5ca2bdad0`。公共/原手动入口未替换，包未自动启动。

PG对应的volatile-copy整段SEH实验在8065batch/两遍读取组件中377→517μs，回退37%，未集成，未作深度测试。V仍有Playground游戏线程成本，120Hz目标保持未完成。


## Playground排序复用（W/X后续）

W新增的只是既有16条计时记录：13次验证/帧、每次1次资格；首次捕获2.1192ms、seal1.80145ms、验证累计3.41525ms。W有两套其他会话，测得游戏/新双眼中位24.49295Hz，正常清理0。

X在完整fresh capture之后，只有当前原始Range描述符的count/address/bytes/capturedOffset/记录顺序与前次完全相同，才重放以前的排序后、合并前描述符。原overlap检查、compact拷贝、expected/mask与双遍验证全部保留。任何变化回std::sort，失败使缓存无效。仅SourceProof启用，额外1,048,584字节位于原heap owner，不存Game字节或许可。

X实际：startup_confirmed=true，seal中位0.49685ms，capture1.9371ms，13次验证累计3.47985ms；末条采样1521hit/16miss。新双眼中位25.70139Hz、XR120，正常退出0且Native钩子恢复。X只剩一套其他会话；与W的FPS不是严格单变量对照，但实际seal成本和命中证明修改生效。

Root已集成，33个描述符/新字节/失败/容量回退测试与既有32个SourceValidation案例通过。Runtime header/default路径编译与一次增量源码审查完成。新手动候选 `analysis/lancer/perf-proof-order-20261115/manual/GearsVR Launcher.exe`，沿用比例0.5与本机配置映射。原allowlist stager的generic预检通过，最终754文件/57098947字节已打包。Native仍3a7ba010…f57；PG716b625b0468838834199e0e9a29f735da6336091754fa4dc91a88edd71b68a2。

PG direct SIMD的两种compiler-barrier实验只改善12.35%/8.18%，未达到推进门槛，仅做smoke/汇编检查后停止，未集成或做游戏验收。下一项是私有Native metadata identity小字段调用开销基准，尚无生产修改。实际120Hz仍未达到。


## 跨回调布尔 witness 私有原型（Y/Z，未推广）

私有原型保存验证字节与布尔资格，跨Scope首次命中仍执行原完整Game两遍读取。Native名字长度workspace是额外前提，逐entry/index记录实际semantic length和成功cold覆盖，命中前后均检查兼容；source/domain/software变化、unreadable和原parser失败使持久entry失效。完整Game、Frame时钟、pose、input和动作权限不复用。临时foreign/NULL observer吞事件再恢复的真实接口反例已用全success事件计数与collector自身计数核对修复。一次增量审查完成。

组件1275项assertion全通过，其中1000项属于500回调人工基准，不能写成1275个独立场景；另有原32cases、compare1与计时3项通过。原型PG2dda4563…0011、Native保持3a7ba010…f57。人工基准不是游戏收益。

| 实例 | 实际Game/新双眼中位Hz | 装备阶段中位ms | 资格capture/seal中位ms | 验证累计中位ms | 启动时其他登记实例 | 退出 |
| --- | ---: | ---: | --- | ---: | ---: | --- |
| Y perf120-y-persistent-1115 | 25.8205 | 7.95585 | 0 / 0 | 4.0571（14次） | 0 | 正常0 |
| Z perf120-z-order-control-1115（旧候选对照） | 27.2576 | 9.244 | 1.93935 / 0.4789 | 3.3328（13次） | 0 | 正常0 |

两次均以实际startup_confirmed=true进入60秒无录像测量，统计排除最初10秒，各51samples，XR中位120。Native清理及入口恢复true、XR退出0，无强退。相同visible/lowest/0.5配置、新实例名字；顺序独立运行，不是同进程交叉试验，无法控制所有其他工作负载。Y先前即时约29Hz不是最终结果。虽然局部装备阶段下降，整段新帧率没有证明收益，故本轮**不集成Root、不替换当前手动候选**。Y最后bounded行1294hit/292qual/290invalid、4014names；没有reason counters，不归因这290次失效。

现有RHI原consume bounded22个ticket>=100记录：Y总8.32925ms、frame_lock2.433ms；Z总8.22425ms、frame_lock2.49745ms。frame_lock只包条件判断和获取锁，不是整个持锁作用域；目前不能把它叫成已证实的锁等待。它与PlayerTick/C0不是同一span，禁止相加冒充全Mod耗时。下一步区分这个predicate与acquire成本，保持现有锁、身份和资源生命周期保护。

结果：analysis/lancer/perf-runtime-20261115/persistent-witness-outcome.json；渲染成本：persistent-order-render-cost-comparison.json。当前手动入口仍perf-proof-order-20261115/manual/GearsVR Launcher.exe；真实120Hz仍未达到。


## 渲染frame锁成本拆分（AA/AB，仅私有诊断）

在原frame_lock总计时内追加predicate、原短路实际执行的retained helper和原lock.lock获取三项；无新增锁、try-lock、跳锁或权限变更，unique_lock返回/持锁生命周期保持。三文件增量审查通过，Native私有诊断1d4d9ef1…565a；PG保持716b625b…68a2。

AA（spin0）startup_confirmed=true，60秒测量后正常清理0、Native/入口恢复true、XR退出0。22个ticket>=100的bounded记录：frame_lock1007calls/2.2269ms；predicate0.01295ms；retained0calls；outer acquire2.1768ms。acquire包括正常API开销、可能等待及调度，不是纯等待，也不是持锁作用域。

另用逐字节相同的私有render_cost.h在独立组件进程做一次5轮校准，共150000个真实Scope calls、4.727ms：每轮均值中位empty10.48ns、无竞争Enter16.05ns、同线程recursiveEnter16.55ns。QPC10MHz/100ns量化；不是游戏线程、不能直接相减，也不能单独证明争锁，但没有支持每次微秒级是QPC固有开销。

AB只把同一CriticalSection初始化spin0改4000，有限迭代后仍走原OSwait，保留同锁及所有Enter/TryEnter/Leave。单行及双向完整文件SHA审查确认无额外修改。其Native779c6c45…36ae仅私有。AB同样启动确认并正常清理，actual game/新双眼中位27.6627Hz，XR120；22个early记录1077calls、acquire2.3698ms、predicate0.01985ms、retained0。AA实际25.8859Hz但有1其他登记实例，AB没有，FPS不能作严格因果比较；acquire成本未下降，故**不推广spin4000、不改Root或手动候选**。

本轮新增Y/Z/AA/AB所有自有实例均正常停止，未操作其他会话实例。spin官方语义依据Microsoft SetCriticalSectionSpinCount文档；不能从有限自旋保证游戏收益。下一项仅为现有acquire时间分布（max及rawticks阈值计数），复用原stamp，不先假设持锁线程。完整记录：analysis/lancer/perf-runtime-20261115/render-lock-outcome.json。实际120Hz仍未完成。


## 获取锁分布与worker来源（AC/AD/AE，未推广）

AC新增max和累计rawticks阈值计数，复用Scope原一次结束stamp。22个ticket>=100：acquire中位2.9731ms/1038calls，最大单次中位0.1746ms，占每帧累计约6.7%；>=128ticks(12.8us)33.5calls、>=1024ticks(102.4us)7calls、>=8192ticks(819.2us)0calls。不是单个很长卡顿。独立只读进程快照Normal优先级、affinityFFFF，没有改变调度策略；不是整个测量期调度证明。

AD仅profilingON给原FrameHookMutex增加atomic owner和CS内depth，原Enter/Try/Leave/spin0保持。真实提取class组件ON28/OFF1检查通过，8行+overflow和nestedWindow归属通过，增量源码审查通过。22个early记录23743calls/538354ticks，两种汇总精确相等。RHI22004、Scene38872、Nativeworker47632；获取前的owner快照来自另外7threads，其中42912对应48.85%获取elapsed，六个其他owner各6..9%，unknown0约8.12%。这些只是before-acquire快照，不证明其持有整个等待，也不把0当无竞争。

AE将owner与outer acquire caller Native RVA打包为单atomic64，profileON noinline锁wrapper保留_ReturnAddress；真实class组件ON44/OFF1通过。只为私有构建加Zi/DEBUG:FULL并显式OPT:REF/ICF/INCREMENTAL:NO，仍O2/Ob2/NDEBUG。本地DbgHelp helper用精确DLL/PDB且pdb_match=1；22个early记录的7worker同site0x197cff，符号是frame_lock()、capture.cpp:1724，约91.86%获取elapsed。说明这个公共wrapper仍遮住了具体D3D hook；下一步只增加profile-only caller-origin TLS/RAII，把wrapper的调用者记录下来，继续保留原锁/时钟次数。

AC实际新双眼中位23.7110Hz，AD26.7570Hz；两者均实际启动确认、60秒无录像、0其他登记实例启动、正常清理0、Native及入口恢复true、XR退出0。这是诊断，不是FPS收益对比。AE同样启动确认并正常清理，结果在runs-summary.json；均未改Root或当前手动候选。相关结果：render-lock-distribution-owner-outcome.json、各实例-distribution.json/-owners.json/-sites.json及同批-symbols.tsv。

独立PG外层SEH+原memcpy对照保留完整batch与原比较，比原8065batch人工shape仅改善6.36%，未达推进门槛，smoke7通过后停止，未做深度故障/双遍变异/汇编安全验收，不集成。另发现shared BaseEngine.ini有smooth62配置，但它只是基配置；private active settings TargetFramerate0，实际引擎覆盖/上限仍unknown，未修改任何共享游戏配置。


## 当前 actor 与渲染原始调用计时（AF/AG，私有诊断）

AF 在原 frame_lock 调用处只加 profile-only TLS caller origin，保持原锁、spin0及 prepare→original→commit 全顺序。精确私有 DLL/PDB 匹配后，22 个 early ticket 的获取前 owner-origin 快照对应 hook_execute 49.293%、hook_reset_list 25.095%、hook_draw_indexed 10.091%，unknown 8.386%。这是获取耗时按 owner 快照归类，不是函数 CPU selftime，也不证明该 owner 持有整个等待。AF 实际 game/stereo 中位27.7604Hz，XR120，正常清理0。

AG 从已交付 source-loop Native 冻结树合并两项既有 scalar 诊断方法，Native1078c86a…97a2d，PG仍716b625b…68a2；只改私有 capture/binding 加两个 helper，不改Root或手动候选。Execute每线程1/16、Reset1/64记录原锁获取后的阶段，typed forward精确测原始D3D12调用；driver仍原参数/原返回/每次单次调用。组件22checks/0fail；actor Timing ON/OFF及4reader目标compile通过，未重跑未变reader矩阵。

AG startup_confirmed=true，60.219秒无录像测量，visible/lowest/0.5/私有120Hz，启动时0其他登记实例。排除前10秒后51样本：actualgame27.7691Hz、新双眼27.7696Hz；XR50有效样本中位120。正常stop0、Native清理/入口恢复true、XR退出0，无强退，其他会话未操作。

NativeQPC频率/进程/启动身份匹配后，48个保守完整Gameplay秒均在ring保留覆盖内，6320行未evict；matched Execute1829样本、Reset2868样本。Execute原driver占已测held区间72.99%，前+后26.71%；driver均值42.669us/中位35.3us，前9.093/7.7us，后6.522/1.4us。Reset driver77.11%，前+后21.51%；driver7.245/6.0us，后1.969/1.8us。整个passthrough约99.60%/98.25%来自driver。不能把跨线程inclusive区间直接相加为帧关键路径，也不能把driver当可删除Mod开销；end仍在局部析构及frame_guard释放前。附件clear在整个Native生命周期实际calls=0/ticks0，暂不为未触发代码做优化。

Actor全程模式0/1/2/3调用244754/7132/167295/464904，总884085，全部成功；32个first/stride样本：模式0/2 total中位24.9/24.3us、proof19.55/18.9us、capture2.1/2.1us、identity7.8/7.4us；模式3 total17.1us/proof12.4us/identity5.4us。identity嵌在proof内不可相加。槽在各模式ordinal0..36864期间用满，属于早期样本，不能外推晚期每帧CPU耗时。

当前选择：不缩小渲染锁的事务保护，不优化零调用clear，不推广纯计时DLL。下一步只在组件中检查旧S witness的大量Native数组比较能否由完整闭合的Native mutation revision取代；Game全部字节仍每次两遍fresh检查，Config/anchor/metadata当前性和原Scope不变。实际120Hz仍未达到。

结构化证据：analysis/lancer/build/render-work-components/perf120-ag-current-costs-1115-render-work.json；analysis/lancer/perf-runtime-20261115/perf120-ag-current-costs-1115-actor.json；runs-summary.json。


## 实际 proof 分页形态与组件取舍（AH）

AG早期C0预算28有效query记录：total7.32635ms、context5.11905ms/214calls、owner_actor2.44115ms；均为嵌套wall区间，不相加为全帧CPU。

组件阶段已停止四条未证实净收益的方案，没有为它们新开Game或修改Root：S的proof revision虽降低Native matches，完整fresh witness synthetic warm仍慢于V；direct metadata identity在现实数量fullactor synthetic回退8.3..9.7%；last-record capture缓存对非连续/无重复回退；packed20 Source仅两行pragma，首次加权约4.9%收益，但唯一一次反序确认mode0回退且加权约0.5%回退，未深测、未交付。各receipt分别在build/actor-revision-components、metadata-identity-deep-components、proof-last-record-components、source-pack20-components。它们是组件结论，不是实际Game回退证明。

AH仅给已有boundedPG成本日志增加已封存Native descriptor的page/span标量：不读取额外Game字节，不保存地址/输入/点位；phase端点在统计之前。PG8c8fcbd1…1ba1b编译成功，Native保持已选3a7ba010…170f57。经原入口visible/lowest/0.5/私有120启动确认后测60.235秒，无录像；排除前10秒后51样本game26.8592Hz、新双眼26.9003Hz、XR120，0其他登记实例启动；stop0/无强退/Native清理与入口恢复true/XR退出0，其他会话未操作。PG仅诊断，不替换手动候选。

16条bounded形态记录最后稳定为1317pages、444218B真正witness、whole-page5394432B、同页最小span2919028B；span额外2474810B超过原1MiB预算，不能整页/全span强合并。下一步只在组件评估同页较小gap优先、预算不变的合并；全部原witness及两遍fresh检查保持，读取失败不重试。同时只审查cold name一次读取中未被解析器消费的NUL后尾部是否应进入witness；未批准跳过身份、输入或权限检查。

实际Game120Hz仍未达到，当前手动候选及Root继续保持既有已验证source-loop+sort-order缓存。AH结果见analysis/lancer/perf-runtime-20261115/perf120-ah-page-shape-1115-page-shape.json及runs-summary.json。


## Native source-shape 实际激活与 CPU 复核（BB／BC）

BB 六组同进程交替比较没有稳定收益：原路径双眼21.0158Hz，候选20.8137Hz，pooled ratio0.99039，pair median0.99317，3/6胜出。候选不推广；生产 bf595fa5／6a79 保持。组件18747+7实际内存检查及735 reader检查通过，仅证明相应组件行为。

BC 正常双眼启动后测60.297秒并正常停止，Native清理、入口／音频恢复、XR退出0、四个精确身份均已不存在。只读Mod描述40次均证明base3640／instigator1027项形状可用；匹配PDB执行位置1000次中sealed循环129次，证明新循环确实运行。Scene线程CPU19.296875秒／wall19.897405秒（约96.98%单核）；Native356、PG484、Game67个执行位置样本。样本不等于inclusive CPU份额，不能据此给出全调用栈百分比。

BC刷新率元数据赋值因字段缺失失败，PowerShell继续启动，实际XR为30Hz。因此本次只验激活／CPU，不作为120Hz比较；Game32.7099Hz、双眼29.7580Hz不能宣称优化收益。系统忙碌／进程CPU另见activation-summary，不能解释BB历史波动或证明具体核调度。下一步审查PG完整反射观察是否能用完整fresh proof读回重建少量未持握输出；不缓存GameState、输入、姿态、时间或权限，不减少任何新鲜度、身份和两遍实际字节检查。worker接口挂起，原ID保留，无新worker或接管。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/native-source-shape/main-review.json；analysis/lancer/perf-runtime-20261115/perf120-bc-shape-activation-1115-activation-summary.json。


## 完整观察实际次数与耗时（BD）

只在冻结PG树增加完整观察包装：原函数体逐字保留（仅重命名、默认参数移至包装），每次原参数调用一次；记录累计标量与每16次一次wall耗时，无额外Game读取。仅两个私有文件不同，完整PG编译通过，诊断SHA25650bfa27a016c1ca2c88e64038da9780815fbb0a47776134e8e7ee238f8827ee5；Native保持生产bf595fa5。未改Root或手动候选。

新实例BD经原入口visible/lowest/0.5准备，以Python字典写入并回读sim_refresh_hz=120。startup_confirmed=true后测60.328秒；终末50秒49有效样本，实际Game／新双眼均30.6141Hz，XR120.0000Hz。正常stop0、无强退，Native／入口／音频恢复、XR退出0，四个精确身份均已不存在。

中间19.5314秒累计2951次完整观察，全为成功未持握，无held／失败；577设备成功回调，约5.1144次观察／设备回调。185次wall采样共182.5139ms，均值0.986562ms；按次数估计约5.0455ms／设备回调、区间wall约14.91%。这是每16次采样推估，可能受相位及调度影响，不能称为exclusive CPU，设备回调也不等于双眼呈现。已证实该路径值得优化，但尚未获得帧率收益。

Main源码复核发现现有boolean Scope在copy_frame_on_gt完整观察之后才建立，单纯在既有Scope内加输出重建覆盖不到前导路径。下一步仅允许私有组件评估：保留完整原witness及两遍fresh实际读取，从fresh copy缓冲区组装真实flag字节，复用Native解析布局与类型描述，重新计算live Native held／动画／切换状态；不缓存GameState／输入／姿态／时间／权限。所有held、诊断、观察器、outerCapture、未知／失败路径保留原观察。完整报告main-readonly-audit.md，尚未批准Root整合或宣称120Hz。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-full-observer-cost/main-review.json；analysis/lancer/perf-runtime-20261115/perf120-bd-full-observer-cost-1115-full-observer-cost.json。


## 未持握布局原型的游戏回退与后续组件修复（BE）

BE 以生产Nativebf595fa5＋私有PG3b687ee8运行，同Game六组交替比较，原路径新双眼24.3042Hz、原型23.4668Hz；pooled ratio0.96555，paired median0.93453，2/6组更快，不推广。正常启动与清理、入口／音频恢复、精确四身份已退出，模式4B写入共52B并恢复1，无其他会话操作。

真实日志显示既有boolean Scope failure4（byte_capacity），捕获1048566B接近原1MiB上限，Scope disabled，导致重复完整观察。组件单独测快没有覆盖“前导观察为后续解析工作区提供名称长度读取界限”的跨模块作用。后续私有组件在完整fresh proof成功后，用验证过的Native名称布局补回缺失的长度读取界限；未来读取仍重验真实字符及NUL，不存GameState或权限、不放宽字节容量。

后续853检查、61既有完整观察及32既有校验案例通过；跨回调回归中原路径与原型后续boolean proof占用同为2418B／230ranges／相同readcount。尚未完整DLL编译或Game复验，保持未交付。详见pg-unheld-projection-prototype/main-review.json。

按用户最新要求，性能结论仅报告实际Game／新双眼画面；XR循环及同一旧图重复提交不作为FPS或120Hz达标依据。不把降低协议循环当作性能修复。


## 名称界限修正版的实际比较（BF）

BF 使用独立PG677cb9ef、生产Nativebf595fa5，同Game六组交替，原路径实际新双眼31.6264帧，修正版30.5746帧，pooled ratio0.966745、pair median0.963938、2/6更快，不推广。正常stop0、模式恢复1、无强退，Native／入口／音频恢复、XR正常退出，四个精确身份均已不存在；其他会话未操作。

16条既有有界Scope成本日志全部disabled0／sealed1／failure0，原1MiB容量不变，witness约444KB；额外完整观察回退已消失，约5次／设备回调。仍保留每回调14次左右fresh boolean校验，其wall成本数毫秒；原型还需重复资格建立，组件单独快不等于整段Game快。停止未变化的原型FPS重跑，下一方向是审查未持握／无动作时真正必要的工作与生命周期／输入／清理副作用，不能缓存权限或无依据删guard。

原DeepSeek worker接口已恢复并审查PASS（仅私有Game比较），随后同一worker补名称witness显式测试。Main发现helper的名字槽地址／WarPC地址匹配错误，已交原worker修正并编译单项，额外结果尚未验收。原853组件检查与61／32案例仍作为对应旧版本证据。

结构化证据：pg-unheld-projection-prototype/main-bounds-review.json及perf120-bf-projection-bounds-1115-ab-summary.json。实际120帧未达成，手动候选保持bf595fa5／6a79。

2026-11-16：按用户最新要求，FPS 只表示实际新游戏画面。头显新增 GAME FPS 小面板，以成功 xrEndFrame 中不同有效游戏双眼图像计数，直接数过去完整一秒事件，不以 XR 循环、重复旧图或短间隔外推代替。暖机/计数不完整显示 --，一整秒无新图显示 0；菜单和等待卡不参与。游戏 GPU 复制原本已经只发生于新生产者图像，无需降低 XR 生命周期频率。

Root 源：tools/openxr-probe/headset_fps_state.h、headset_fps_pixels.h、pose_probe.cpp 与两项聚焦测试。复用同一 DeepSeek worker 01a1de77-a71a-7f93-b229-e3ecd8cf315e；Main 修正最终计数数学、主层接入和退出保护，worker 只读审查通过私有运行。30→30、120→120、1→1、重复不增加、停帧0、时间/代际/计数跳跃/容量边界测试通过；完整物理与专用模拟器单目标均编译。

私有 perf120-bi-real-fps-1116 确认 startup_confirmed=true，测量45.25秒，浮层成功提交、disabled=0；后30秒显示样本中位47 FPS，范围39..54。51个显示更新与现有CSV累计计数上下界相容；CSV启动后每秒一行，不具备独立逐图时间戳，因此没有宣称逐帧精确交叉核验。实际窗口算术由聚焦事件测试覆盖。正常stop=0，Native/入口/音频恢复，XR退出0，4个自有身份均不再存活，无录像/额外点位采集。详细 main-runtime-review.json。

首次 BH 使用物理版错误进入模拟器入口，参数守卫拒绝、退出2、未启动Native；正常清理，失败保留。BI复用原 sync_hotfix_xr_source.py 的既有Source1构建变换，独立输出；未放宽物理版守卫。专用CMake仅保留所需XR目标，排除无关测试目标依赖。

本机手动候选：analysis/lancer/actual-fps-headset-20261116/manual/GearsVR Launcher.exe。物理XR SHA256 7a9848335d4228540f87aceae8b82a4ff57136d80e7873966b62490ca1cccc7d；Native/PG沿用原稳定版本。753项负载清单哈希核验通过。本机候选保留已有游戏选择，通用便携预检因要求空游戏路径而拒绝；没有便携预检PASS/公共可分发声明。实体头显可见性、舒适度尚未验收。该次系统外部负载不同，47FPS不是优化提速证据，120FPS目标仍未达成。compound identity 候选只编译/审查，尚未Game比较。

2026-11-16 continued120FPS：私有 BG compound identity 同游戏同DLL original→candidate→original20s比较已完成。按实际成功接受的新双眼图像计数、各段两端裁掉2秒：33.4621 / 33.7731 / 32.1240 FPS，候选对两段原版平均约+2.99%；Game Present各段中位33.8090 / 33.7264 / 32.5769，候选未胜过第一段原版。证据偏弱/有段间漂移，不推广、不重复同一实验。自有4B诊断选择值恢复、startup confirmed、正常stop0、Native/入口/音频恢复、XR退出0均通过，无录像/输入点位新增。见 workers/reflection-contract-deepseek-01/pg-compound-identity/main-bg-decision.json。

下一目标不是删掉idle校验，而是测当前stow_native::identities的剩余泛反射查询成本。旧AP调用方记录只能指路，不能作为当前成本；同一个DeepSeek worker正准备只包装原函数、每17次抽样的私有计数诊断。Root/手动候选不包含此诊断。

2026-11-16 BJ当前收纳身份成本已核实：25.057489s窗口1284个设备帧；certified=false桶窗口0调用，certified=true桶2568调用/151抽样/85401600ns，平均0.56557351ms每抽样调用、2调用/设备帧，估算1.13114702ms/设备帧。包含原身份与嵌套库存链校验，不是独占CPU，也不等同全部可省成本。CSV同窗口内部24.1167746s计1232个实际新双眼画面，约51.0848 FPS；这是当前归因运行值，不是提速对照。所有原Game读/检查/返回保持，Main补齐volatile与溢出失效并验证包装，完整私有PG079d2fba编译，正常清理/入口/音频/XR0/4身份不存活通过。见 stow-identity-current-cost/main-current-cost-result.json。

Native source-order-cost索引plan组件成本负面1.38..1.90x，未集成；其baseline并非完整逐字原生函数、guard只测原臂，已明确限制，不作Native真实性能或完整安全证明。新的direct-record方向经只读审计仅允许组件试验：准备最后current成功之后、ready之前重排原有Native Source记录，保留热循环原精确读取；记录集合/预期值不变但失败读取顺序变化仍待验证。当前同一DeepSeek worker只做该窄成本实验，Root/手动候选未改Native/PG。120FPS仍未达成。

2026-11-16 direct record sort成本门关闭：Main修正fixture低地址写、分配边界fatal检查、unique_ptr数组索引和scattered地址别名，并使用heap大对象；最终同一逐字冻结source_loop_exact读两臂，14654检查/0失败，配对结果 ascending1.040169、interleaved1.023461、scattered0.905519（cycle1.024227/1.009036/0.915955）。混合/密集无收益，极端页分散仅约9.45%，未过成本门，不全DLL/不Game/不推广。初版崩溃与Main第一版别名失败日志均保留，非有效成本。此组件用Identity/Property无用成员最小桩，不是完整current_actor/C0证明。sort_ns仅计base permutation，不含全部分配/索引/另一proof。见native-source-direct-record-cost/main-result.json。后续转保留原Game读序的Native自有元数据布局/对齐方向，尚无新的生产修改，120FPS仍未达成。


2026-11-16 BK：稳定 Native/PG，既有独立可见入口、0.5/lowest/无录像；startup_confirmed 后45.203秒，Native自有Source.size两次只读统计一致：base 4B=23、8B=3617，总3640；instigator 4B=8、8B=1019，总1027。不读取Source地址/期望字节、游戏值、输入或姿态。原1/7宽度夹具不代表实际分布；对齐组件原结论仍不合入，不声称FPS收益。正常停止0，Native/入口恢复、XR退出0、所属游戏消失。证据 analysis/lancer/workers/reflection-contract-deepseek-01/native-source-width-audit/main-runtime-review.json。头显只计成功提交的实际新双眼画面，XR循环不作为性能指标。

2026-11-16 实测宽度复查：Native Source 对齐仍不合入。两个宽度排列×三种地址布局，85349检查0失败，原exact reader函数体一致；四组慢6.06%/7.65%/21.17%/6.89%，仅near/far两组快8.85%/12.19%。宽度数量来自BK，顺序/地址仍为合成，不是假实测trace。不构建DLL、不重复游戏、不宣称FPS收益。主会话接手worker反复shell错误后的编译/运行，证据 native-source-alignment-measured-width/main-result.json。

2026-11-16 stow grouped query组件：worker玩具读取/注册/name/slot模型不等价，拒绝；Main复用原actual-memory SEH/registry/FName/pagebounded/metadata identity/真实inventory capture和原identities函数，三组prefix查找、SkeletalMesh和最终InvManager保留。原TickScope内两次独立validation，8组合5轮反序计时，130检查0失败；median wall ratio=0.9840115，多数微小改善且有回退，不合入、不构建DLL/不重复Game。VM/script资格检查离线未复现；共同adapter使用真实inventory capture，不能称全函数生产成本或已完成故障矩阵。证据 stow-query-group-component/main-composition/main-review.json。BK已存CSV新双眼2153/45.2207337s=47.6109FPS，既有抽样source validation13次3.513ms、设备阶段4.329ms；区间嵌套不可相加，无性能A/B声明。

2026-11-16 PG fixed Table16：仅Array/Table窄分支，跨页NOACCESS正确失败/partial表头拒绝/恢复完整header通过。修复fixture只写8B问题后长测64次每臂×5反序，131检查0失败，median wall ratio=0.9898405，3/8回退；不合入/不Game、不称FPS收益。旧worker result残留boundary失败文字，以run-long.log与Main main-review.json为准。引擎FName旧反编译仍缺比较函数/冷路径写入契约，不缓存Name索引替换文本选择。下一步补实际Native自有Source地址拓扑证据，仅元数据，不读期望/实际Game字节或输入姿态。

2026-11-16 BL/BM topology：BL startup确认但reader遗留policy断言导致0样本，正常45秒停止，保留失败；修复并用全新BM入口重查，3/18秒两份相对拓扑一致。4667记录、485页、4320起始cacheline、78.1397%相邻同页、39跨proof共享页、虚拟span1596612608B，所有4/8读取不跨页；Native proof起始RVA mod64=base0/instigator48。只读Native metadata地址/size，未解引用Game地址或读取期望值/实际游戏值/输入姿态。BM45.375秒正常停止/Native与入口恢复/XR0；他人实例不操作，不能与BK比较收益。证据 native-source-topology/main-runtime-review.json。

2026-11-16 actual topology alignment：Main接手worker生成器失败，逐项核对4667 offsets/width与BM JSON相同，保留exactreader字面体、Native缓存行offset0/48；只reserve1596612608B、commit485页1986560B。7反序轮×4096次，15001检查0失败，expected/actual变化和NOACCESS拒绝/恢复通过。original4771.17ns vsaligned4683.89ns，wall0.981707、cycles0.996925；只87.28ns差，无Game收益结论，不合入、不重复Game，关闭字段对齐方向。已实测所有地址按宽度对齐，下一步只做16B Native tagged记录plan组件，非packed20，不改Game读序/量。

2026-11-16 重要纠正：BI/BJ/BK/BL/BM新FPS XR运行中body_native/hands commits全部0，body_source acquire0、unavailable=attempts；旧AU/BG commits1676/2967。已存counter与冻结headers比对：Native bf读取continuity v3/cap8/map v3，新FPS XR源取current root已升v4/cap32/map v4。双眼/FPS面板检查不能代表完整Mod；47.6109FPS不得作为完整body/hands性能基线。旧BG完整链实际original33.4621/32.1240FPS（不同外部负载，不能与新run直接做收益比较）。优先修复私有FPS兼容bundle，root v4及其他会话不回滚。证据 actual-fps-v3-compat/main-regression-evidence.json。

2026-11-16 兼容修复已验证：私有FPS XR替换exact v3/cap8两个header，root v4不动；physical仅关闭既有私有sim-admission定义，其余FPS/pose source哈希与旧FPS快照完全相同。sim SHA6481600807eb025c2faa9b3247623072bb896f5806a1fbf21f9bc23766a242d5，physical db7727e74f9c8c6b08593c3495c46f81e8983f348137bf244d6689f4996d0e31。原协议suite使用新sim头编译运行0，Native/sim/physical头哈希相同。BN startup确认45.157s、body获取1617/1618、Native/body/hands/goal commits1615、PGcontinuity matched1589 unavailable0、49个FPS显示更新计数边界检查通过，最后30s实际新双眼显示median33/min14/max37FPS。正常停止、Native/entry/audio恢复、XR0、4个exact owner身份不存活。新本地manual候选753文件hash/size全匹配；旧FPS手动入口原子更新同physicalXR，配置和Native/PG保留。实体头显/全部玩法/120FPS仍未验收，portable preflight不称通过。

2026-11-16 owner FPS clarification: performance and headset display use only distinct accepted new game stereo images/second. Main verified Native consumed-ticket dedup, GPU receiver new-sequence acquisition, eye GPU work within successful new acquisition, and repeat-key exclusion in FPS counter. Existing compatible candidate already follows this behavior; no render-loop cap or unnecessary source rebuild. Retained projection keeps the prior image visible and is not new game rendering or FPS. OpenXR synchronization/tracking cadence remains separate. Main audit: analysis/lancer/workers/reflection-contract-deepseek-01/game-frame-cadence/main-review.json. No new game run or headset acceptance in this audit.

2026-11-16 Native tagged16 lifecycle/build/Game: Main repaired private integration,42unit/791lifecycle/735macro-off checks pass; exact original reader and ClassPath onward byte-identical, Nativeonlytwofileschanged. Build43f2716c...235e19. BO original33.349/candidate28.776/original26.850FPS, but peerinstance appeared after firstphase: invalidgaincomparison. BP targeted alternating3pairs10sec, actualdistinctnewstereo endpointFPS original30.745/35.764/35.071 vs candidate36.158/35.166/34.435.1/3wins,medianratio0.983279; no stablegain, no promotion, close tagged16 layout-only direction. All6BPphases body/hands continuous and >98% acquisition/commithealth; actualcandidatepath used~1552calls/bodyframe, fallback16/bodyframe retainedoriginal. Bothownedinstances normalstop0, Native/entry restoredXR0 and exactownidentitiesabsent. Existingmanualcandidateunchanged. Worker boundedcalleraudit shows repeatedfull proof stages through context/owner/storage/standing/cleanup; next directcallsiteattribution withoutremovingfreshness/identity/preCAS checks. Evidence analysis/lancer/workers/reflection-contract-deepseek-01/native-source-tagged16-integration/main-paired-runtime-review.json and main-cleanup-review.json. Actual120FPSstillunachieved.

2026-11-16 Native 调用归因（诊断，不是提速版）：Main 接手原 worker 的执行器失败，仅在稳定私有 Native 中增加 TLS root/leaf 与 Native 原子计数。剥离插桩后，原源码非空行完全相同；585 计数检查及宏开/关各735原绑定检查通过。Native SHA95a930d9...82a20e。BQ 在 Native 加载前因已有 FrozenThreads/SuspendThread 拒绝访问而启动失败，保留失败；新 BR 经同入口成功，60秒后正常停止。30秒计数窗口 body/hands各743次，实际新双眼24.2813FPS；窗口内硬拒绝1次，整段会话硬拒绝2次、年龄拒绝6次，不能称零拒绝或完整功能验收。

每个 body commit 进入1569.31次 Source 遍历，按进入时source_count累计4333380.87条记录；不是逐项实际完成读取的事件追踪。普通actor入口552.56次/帧（35.21%调用），storage536.09（34.16%调用、45.03%记录量），retained379.81（24.20%调用），其余约6.21%。这些是调用量，不能当排他CPU占比。静态路径指向 full_body_c0_native_live.inl RetirementContext::current 的522/527双storage检查；原覆盖回执在 native_atom_overwrite_receipt.h 的begin/finish/blend/verify会反复请求该上下文，仍须保留真实覆盖、身份和即时写入检查。下一步研究用绑定到原游戏二进制的布局证书减少反射发现依赖的重复遍历；尚无替代实现，不缓存游戏授权。BR Native/入口恢复、XR0、已检查精确自有身份均退出；手动候选未改，120FPS仍未达到。证据 analysis/lancer/workers/reflection-contract-deepseek-01/native-proof-caller-counts/main-runtime-review.json。

2026-11-16 固定宽度批次候选：字段二进制布局证书尚不成立。REA IDA未配置，现有IDA无数据库；REA Ghidra核对原文件SHA，字段名均不在已定义字符串清单，旧全局位置在打包磁盘视图为0。既有AV代码样本hash193391fd...8074匹配，但只含RX区，不能建立字段语义。停止猜偏移、不删除反射witness；本轮REA会话已正常关闭。

原DeepSeek worker直接交付Native source-runs header，Main落盘并review：Native16B entry不带tag，按连续4/8B宽度形成29个base run与12个instigator run，固定宽度循环每4项顺序短路，不预取、不扩宽Game读，不缓存校验成功。197组件检查、实际4667地址拓扑15001检查、791接入生命周期与735宏关原绑定检查均0失败。原reader与ClassPath之后原代码保持不变。7反序cost轮全部更快，组件wall ratio0.615426、cycles0.622062；合成物理内容，不是FPS收益。私有Native294f5baf...214e0b编译通过。

BS三对实际新双眼FPS原25.430/35.561/35.820，对应候选35.560/37.001/37.613。第一对有明显启动波动，排除其收益归因；后两对+4.05/+5.00%仅暂定。针对启动波动补BT：确认后等待25秒，3对10秒，原33.381/26.282/30.596，对应候选28.105/33.718/31.399；2/3更快、中位ratio1.026238，但帧率波动大且其他登记实例持续存在，不能确认稳定收益，也不能仅以实例名相同认定其他负载恒定。六段body/hands均持续健康、实际run路径调用约1552/body帧、开关恢复。两自有实例正常stop0、Native/入口恢复XR0、已检查精确自有身份退出；其他会话不操作。候选保留但未promote，手动入口不变，stager因证据不足未执行。下一步读取精确自有Scene线程CPU时间与cycles，区别调度等待；仍以实际新画面FPS为结果，不再重复不变的wall-FPS测量。证据 native-source-runs-integration/main-warm-runtime-review.json。120FPS仍未达到。

2026-11-16 BU线程会计：原worker交付只读单线程helper，Main补closing身份检查；24接口检查+11原控制器检查通过。仅OpenThread0x100800对精确自有NativeScene/body/PG同TID，读取FILETIME与QueryThreadCycleTime，不挂起/不改优先级或亲和性。25秒预热后3对10秒，实际新双眼原37.216/35.013/34.767，对应runs36.417/37.676/30.701FPS，1/3更快；CPU毫秒/body帧原25.739/27.138/27.661，对应26.065/24.877/31.503，1/3降低，medianCPUratio1.012663、cycles1.018435。最后一对有peer加入；前两对也未重复确认收益。Scene线程忙碌94.4..96.9%，说明需要减少实际执行工作。关闭source-runs继续比较，不promote。Native/body/hands六段健康，模式与原入口恢复，XR0，自有线程handle已关闭，精确自有实例身份退出。

结构性证据：BV首次因打包PE节名为空而错误按.rdata名称筛选，未读数据；修正读取器后旧实例已退出，身份检查拒绝读取，失败保留。新BW按已核对Game hash中的第一主只读节RVA0x131b000/8037812B及OS PAGE_READONLY/MEM_IMAGE/allocationBase三重门控，double read总16075624B，保存38个指定字段/类名字常量锚点的小上下文，不保存整块，不读heap/input/pose。28纯函数/区域门控检查通过。复用AV历史RX样本，在私有IDA数据库叠加4818B只读常量上下文，8个原始RIP候选、6个函数组；WorldInfo消费点是创建/设置路径，Mesh消费点涉及名字比较/材质事件，Instigator/helper有检查与可能日志/懒初始化，不能冒充Actor/Pawn字段getter。原数据库hash不变，IDA正常退出。实际布局证书仍未建立，不删除witness、不调用未知Engine API。下一步有源码依据：现有PG invoke_engine与组件setter header读者已使用UFunction native pointer0x114、native index0xf8、参数大小0x106、return0x108；可据此从已知Native类identity定向查实际GetWorldInfo/GetMesh函数元数据，限定schema/code，不读取对象姿态或调用函数。BV/BW均正常清理，手动候选不变，120FPS未达到。证据 native-source-runs-thread-cost/main-thread-runtime-review.json 与 native-layout-rodata-anchors/main-review.json。


### 2026-11-16：已知类 UFunction 定向检查，关闭指定 getter 路线

沿用唯一 DeepSeek worker（01a1de77-a71a-7f93-b229-e3ecd8cf315e）的检查器，从稳定 Native 中的 Actor、Pawn、Pawn_COGMarcus、WarPC 类身份出发，只遍历有界类声明和父类。BZ 独立实例实际启动确认通过；双读共 8,290,608 字节，没有输入、普通实例姿态读取、引擎函数调用或录像。

GetWorldInfo、GetActorWorldInfo、GetMesh、GetPawnMesh、GetSkeletalMeshComponent、GetInstigator、GetWeapon、GetSpecialMove 八个精确名称均无匹配。这个结果仅适用于已查类声明和这些名称，不能推出整个游戏没有读取函数；它也没有建立字段布局证书。因此关闭这条具体优化路线，不删除反射与 currentness 检查，不修改现有候选，不宣称帧率提升或 120 FPS 达成。

检查工具先后修正两处具体问题：私有实例没有 PDB，改为校验原始 DLL/PDB 与已加载 DLL 的同一构建身份；固定长度名称读取跨区，worker 改为逐 UTF16 单元读取并在 NUL 停止。27 项原检查、4 项名称边界检查通过。BX 初次路径失败、结束后身份拒绝、BY 名称边界拒绝记录均保留，BZ 检查完成。三个自有实例均正常停止、Native/入口恢复、XR 正常退出；逐身份核对自有游戏/启动器/XR 均已不存在，其他会话未操作。

证据：`analysis/lancer/workers/reflection-contract-deepseek-01/native-accessor-metadata/main-review.json`、`main-capture-result.json`、`main-cleanup-review.json`。这是定位证据，不是性能 A/B；后续回到已测出的验证调用成本，先提出有生产代表性的减耗方案再开性能比较。


### 2026-11-16：完整覆盖回执先被内层逐骨骼退休，私有诊断验证通过

在 BZ 的完整 Native 状态中发现：3270 次完整 blend ticket 已 admitted，但 finish_blend 全部 exact failure bit4（绑定已被退休）；内层成功逐骨骼回执 212550 次，正好 65 次/提交。这个事实改变后续方向：先验证能否让已有完整覆盖回执承担最终退休，而不是继续压缩底层读循环。

同一持久 DeepSeek worker 交付三文件最小诊断 diff。独立源码 perf-native-blend-retirement-probe-20261116 只新增宏控制的父调用链判断和延后通知计数，Registry、原逐骨骼 begin/finish 校验、覆盖集合、原游戏调用、part/handle 观察及原完整 stores/currentness 校验均未改变。332 项真实 Registry 检查、17 项实际 helper 边界/宏组合检查通过；全量继承原 GEARS/CAPTURE 与 CXX_FLAGS，私有 Native 编译成功，SHA256 4638c3ed3140c145028cecbfe5477e43e7bfc77a064f8de712e094321c86f456。CA prepare 因清单误写 build 根目录而非 Release，在启动前失败，已记录并修正；未复用实例名。

CB 独立实例 startup_confirmed=true，30 秒无录像：完整 blend admitted/qualified 均1155；原先 exact failure 与 root rejected7 均0；body/hands commits 均1155；逐骨骼 qualified/deferred 均86625、rejected0、pending0。全部真实校验仍执行，因此这只证明完整覆盖回执真实可通过，不宣称 FPS 收益，也不作为120Hz或实体头显/完整玩法验收。CB 正常停止、Native/入口恢复、XR0，逐身份核对自有进程均已不存在。普通手动候选、PG与XR、公共版本均未更新。

下一步是隔离实现精确完整 witness 的回执所有权：在同一真实 admitted 完整覆盖作用域内减少内层重复 body receipt 工作，同时保留原游戏执行、独立部件观察、最终完整写入与身份/世界检查；必须覆盖嵌套、异常、错线程/版本、停止与清理路径。证据目录 analysis/lancer/workers/reflection-contract-deepseek-01/native-blend-receipt-owner。


### 2026-11-16：完整回执选择实际 FPS 改善

CC 单实例25秒预热后三组10秒交替比较：原路径36.327/36.522/34.473 FPS，新路径40.267/42.121/38.739 FPS；三组均更快，配对提升中位数12.375%。Scene CPU/身体帧配对比中位数0.88506。六段身体/手部链路健康、回执路径均验证、atom拒绝0、最终pending0，模式恢复、正常退出及Native/入口恢复、逐身份进程退出均通过。短模拟器比较，不是完整玩法或实体头显验收；120 FPS仍未达到。Native b262a4f0fddd873d31a00e09d5d773f8b58498fee7d2b33740e5b0e98bb3fd44，PG/XR保持原版。独立手动候选 analysis/lancer/native-blend-owner-perf-20261116/manual/GearsVR Launcher.exe，753文件哈希/大小核对通过，旧候选保留。详细证据 native-blend-receipt-owner/component/main-runtime-review.json。


### 2026-11-16：已优化版本再定位

CD 使用相同 Native b262/PG6a 重新做20秒单Scene RIP采样，997次有效，采样器正常恢复、模块集合一致；Native316、PG520、Game79。Native source_loop_exact118；PG固定小字段copy_value合计150、memcpy88、BatchedProof.validate87。这里只是指令位置样本分布，不是精确独占CPU比例。已优化版本仍主要卡在Mod反射/验证；先不推进尚无热点支持的提前owner选择，转向PG固定小读和批量验证的调用与复用边界。原worker只读审查确认提前选择必须保留part/handle分支、原函数与finally、父Scope/最终stores检查，未实施该变体。CD实际双眼启动确认、采样正常恢复、普通stop、Native/入口恢复及XR0，逐身份进程核对全部退出。没有增加录像/输入或姿态采集，没有修改已交付候选。详见 component/main-hotspot-review.json。


### 2026-11-16：PG 本地掩码比较组件原型

原worker交付4x16B SSE2合并比较，仅处理成功read后的now/expected/mask本地数组。两遍Game读取、范围、批次顺序、fault路径均未改变；不同于已关闭的direct Game SIMD路线。Main修正std::array指针表达式后，在实际原/候选BatchedProof.validate上完成36864项等价检查，0失败。单批次7轮各50000次计时，新/原中位比：64B0.82503、96B0.96673、128B0.81904、256B0.73081、1024B0.66028。这只是小组件原型，不是完整validation或FPS收益；尚未私有PG集成/运行游戏，现有候选未改变。下一步做接近现有见证规模的混合多批次计时及第二遍变化测试，再决定是否推进。证据 pg-local-masked64/main-review.json。


### 2026-11-16：参考会话绘制快速分类的隔离移植已编译

复用查明真机调试帧率会话的原zeroRTV/HDR26分类器、Counters及两项组合快照API。拒绝worker首份误将RGBA8纳入fast的重写草稿；最终原分类器保持，只删除当前没有的SR矩形修正依赖并增加私有模式开关。当前startup分支、flat theater UI、Scope/world、frame lock回退路径及Reset/Execute/Present原逻辑保持；逆变换逐字验证capture.cpp等于基线。原跟踪/并发计数/交换链清单fixture exit0，并显式覆盖RGBA8等非候选格式拒绝。保留低频聚合命中统计，无逐draw计时。私有Native890d255d2560f2e27e08a757dcd5644ecfa5807f75ed5e85322c2effbdbda527编译通过，继承已验证身体回执优化及全部基线编译选项。尚未运行游戏或证明FPS收益，未更新手动候选。下一步精确PDB绑定模式变量并复用同实例受控A/B；证据 native-worker-indexed-fast-port/main-integration-review.json。


### 2026-11-16：参考绘制快路实际A/B无稳定收益，不推广

CE独立实例启动确认，通过90秒观察、25秒预热后三组各10秒交替比较。原路径41.521/42.916/41.725 FPS，快路41.448/44.307/41.273 FPS。配对FPS比中位数0.998248，只有1/3更快；Scene CPU/身体帧比中位数0.993344，只有2/3下降。快路确实生效：zeroRTV2716638、HDR26139188次命中，supported1、overflow0，非未命中。六段身体/手部健康，原身体owner优化均生效。正常stop、Native/入口恢复、XR0、精确自有身份退出、flag恢复和线程句柄关闭均通过。当前固定空手场景不支持性能收益结论，不推广、不重复原样测试；旧会话70FPS不适用于本次完整Mod负载。继续PG验证热点；现有手动候选不变。证据 native-worker-indexed-fast-port/main-runtime-review.json。


### 2026-11-16：PG local64短对照有小幅正向信号，未推广

CF沿用Native身体回执b262与XR648，仅PG切换复制后本地比较。精确PDB绑定PG自有4字节模式，11项实际controller fake API检查通过；25秒预热后三组10秒同进程交替比较。原42.247/42.200/43.194 FPS，新43.384/43.711/43.978 FPS；3/3更快，配对比中位数1.026906。Scene CPU/身体帧比中位数0.993448、cycles比0.969382，三组下降但CPU幅度不一致。模式已受控选择，无新增逐batch命中计数，不能将Native计数称PG命中。六段body/hands健康、Native pending0；普通退出、Native/入口恢复、XR0、自有身份不存在、PG模式恢复和线程句柄关闭均通过。仅固定空手短模拟器比较，实际玩法与实体头显未验收。保留小幅正向实验但不推广，不把2.7%称稳定收益；下一步回到高层重复PG工作而非继续SIMD变体。现有手动候选保持不变，120FPS未达成。证据 pg-local-masked64/main-runtime-review.json。


### 2026-11-16：Native/PG字段契约交集核对

已核对Native Binding/Proof/values与PG observe_game/PawnObserveFields。PC.WorldInfo、Pawn.Mesh/Weapon及持枪Instigator均有Native typed proof与真实值读取；但PG的PC.Pawn、Pawn.Controller、四对象删除位、切换/禁火bool字段及武器特定状态不能由现有五个Native基础属性覆盖。Main纠正worker关于Instigator覆盖的保守遗漏。Native descriptor仍在私有Proof中，Frame/Binding不是可直接替换PG reader的完整schema或调用域证明。MeshPointer与ComponentProperty选择语义还需等价验证。禁止直接删PG校验；下一步只有定义带callback/thread/world/version/时效和失效边界的窄descriptor复用契约、保留额外字段与真实值新读/失败回退，才可做整体query成本组件。此轮仅契约证据，未改运行代码/未开游戏/未宣称性能收益。证据 native-pg-schema-contract/main-review.json。


### 2026-11-16：实际选择器证明Native/PG描述符不能直接互换

原worker提供两份现有fixture wrapper，Main补PG空logger seam并执行原生产函数。Native16项、PG4项检查0失败。Native MeshPointer接受ComponentProperty及ObjectProperty，PG只接受Core.ComponentProperty；两边选择规则不等价。另在自有合成Native元数据中构造前置Ignored字段，暖起保留proof后将其改名Mesh：原保留proof.current仍true，而fresh property lookup拒绝重复Mesh。证明该Native保留proof不能充当PG首次同名查找的等价证书。此现象仅在合成metadata中复现，不声称真实游戏发生过该变更或它导致了当前低帧率。没有更改运行代码、现有候选或开游戏。关闭直接借Native current标记的路线；未来任何schema复用都须保留PG类型/Core限定、前置非匹配名称/链接证据和真实值/调用域校验，先比较已关闭projection原型，避免重复实验。证据 native-pg-schema-contract/main-review.json及selector-tests日志。


### 2026-11-16：PG高层调用范围核对

比对已关闭multi-prefix/projection实验与当前Scope调用点：Lancer/known-unheld outer Scope仅!active时创建；Lancer部件与starter已复用外层，Longshot有显式同Frame匹配复用。Boltok局部Scope虽直接创建，但完整frame读取位于武器分派路径，不能据源码断言空手时它反复重建。observe_source是完整payload，observe_hold_source是有限payload，observe_liveness才走布尔SourceProof；旧5.1full observer/frame不能简单归因nested作用域。暂不实现无到达频率证据的nested优化；下一步复用已有observer-cost思路按payload/hold/liveness回退分类定位。未开游戏或修改运行代码。证据 pg-call-scope-reachability/main-review.json。


## PG 完整观察来源实测（CG，2026-11-16）

私有PG仅修改weapon_hand_client.inl并加入统计inl，反向移除插桩后与原源码逐字一致；原观察原参数调用一次。每类独立stride16采样，TLS标签嵌套恢复，短sequence与inflight过滤RPM交叠。111项CPU检查通过，PG完整编译通过；worker草案存在分类覆盖、声明顺序及计量混淆，review拒绝后Main接手关键实现。未修改共享运行版本。

CG经现有入口visible/lowest/0.5启动，startup_confirmed=true。34.054秒成本区间、1238设备更新；实际新双眼36.483FPS。完整观察6191次（5.0008/设备更新），387采样均值0.820850ms，推估4.1049ms/设备更新。显式source、hold_source、other均0。布尔liveness 17591次（14.2092/更新），1100采样均值0.318070ms，推估4.5195ms/更新；其中仅37次完整回退（0.2103%）。liveness_total包含回退，禁止重复相加。

身体／双手连续更新且无拒绝；本实例正常关闭、Native／入口／音频恢复、XR退出0，全部精确自有身份已消失。数据排除了本场景显式source／hold接口或大量liveness回退为主因的解释；两项实质开销分别是完整payload反射约4.10ms／设备更新，以及成功boolean witness验证约4.52ms／设备更新；后续同时检查完整读取和成功验证路径，而不是把高频boolean调用误当作廉价或大量回退。每16次墙钟采样含调度，设备回调不是渲染帧，固定空手场景不代表战斗或实体头显，未宣称诊断带来收益或达到120FPS。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-observer-callers/main-review.json。


## 查询内身份查找筛选（2026-11-16）

原worker复用同一正式会话实现append-only查询内身份lookup。Main修正未命中不应重建、换数组重建、超容量原线性回退及地址类型契约；只在初次发现reuse_initial_identities时替代已有本地线性查找，原Game读取、名称文本读取和所有closing原样保留。未采用可能绕过名称变化的文本复用方案。两个私有文件变化，未修改交付候选。

真实metadata测试46320项、lookup／实际观察读取trace／closing突变58777项通过（共105097）；实际完整observe与source适配器的读取地址／字节数／顺序及输出一致，身份class／outer／name.number／object slot改变仍被closing拒绝。7轮各模式2000次真实完整观察（人工拥有内存）耗时比1.045134、1.018456、0.994683、1.022802、0.908476、0.984337、0.980935，中位0.994683，无稳定组件收益。已在离线阶段关闭，不编译完整PG、不为其启动Game、不推广。此结果不能证明生产大元数据树绝无收益，但不支持把该小优化当主瓶颈解决方案；后续返回CG已测完整反射和成功boolean验证。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-query-identity-lookup/main-review.json。


## PG 批量规划参数筛选（2026-11-16）

对现有实际BatchedProof进行有界gap64／256／512及padding1／2MiB组件筛选，全部record／seal真实执行，保留全部见证字节、mask、两遍fresh、同页1024上限和失败不重试；读取规划不同，不宣称任意并发突变trace等价。人工6144节点、stride256／512／1024；覆盖15421939及10224426个逐字节／边界／变动／pass2／故障检查，零失败。这些不是实际Game布局。

大gap512在stride512布局提前耗尽填充预算，使后续批次反而由6144增为14883，耗时中位1.6975倍。只增padding到2MiB时stride256批次3503→1537、计划字节1417204→1492940，耗时中位0.9784；其他布局计划不变、无稳定收益。没有证据实际Game计划触及预算，故不改生产参数、不完整构建、不为其运行Game。原worker草案因类型／空witness／错误fault／伪pass2／错误AB等问题未应用，Main接手关键fixture。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-proof-gap/main-review.json。


## PG local64 独立复核（CH，2026-11-16）

此前CF三组成对实际新双眼均有小幅提升，但未推广；本轮使用相同已编译PG bcc1297e、Native b262a4f0、XR64816008，在全新实例CH做第二次25秒暖机、6×10秒交替对照，每段两端各裁2秒。运行前保存温和收益标准：三组均快、成对中位至少1%、CPU／cycles中位下降且CPU至少两组下降、身体双手全段健康、资源peer一致、正常恢复。不是新增SIMD变体，也不是只因旧小收益就自动推广。

组0: 原42.7160FPS，local64 42.5887FPS，ratio 0.997020。

组1: 原43.8503FPS，local64 43.3120FPS，ratio 0.987725。

组2: 原42.8586FPS，local64 43.1965FPS，ratio 1.007883。

CH成对中位0.997020，1/3更快；CPU中位比0.991914、cycles 0.998977。预定温和复核标准通过=False；Do not promote local64. Earlier small positive did not satisfy prospective replication criteria; close and do not rerun unchanged.

正常stop，Native／入口／音频恢复，XR退出0、PG模式恢复，精确自有Game／启动器／XR／session身份全部消失。无录像或额外输入／姿态采集，实际新双眼计数与复制计数交叉检查。仍限站立空手模拟器场景，不是完整玩法／头显／120FPS验收。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-local64-replication/main-runtime-review.json。


## 完整观察内部阶段实测（CI，2026-11-16）

仅在CG既有caller统计中加入同次采样phase meter：首次发现到reuse_initial_identities=false之前为discovery，原全部身份／结构／属性／flag／links／table最终复核为closing，后续support/candidates为extras。共享切换时间戳，早返回归当前阶段，TLS嵌套恢复，未采样不调用clock；16项小型边界检查和完整PG编译通过。原函数体移除两条phase标记后逐字等同CG；没有新增Game读取、writer、输出或校验变化。

CI实际新双眼33.7774FPS；成本区间34.570秒、1166设备更新、367个同次分段样本，nested／clock backwards均0。

discovery: 均值0.760269ms，占同次观察合计78.282%。

closing: 均值0.210689ms，占同次观察合计21.694%。

extras: 均值0.000239ms，占同次观察合计0.025%。

身体／双手健康、正常退出，Native／入口／音频恢复，XR退出0，精确自有身份均已消失。采样墙钟含调度，固定空手场景不是全玩法／头显验收，诊断未推广、未宣称优化收益或120FPS。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-observer-phases/main-review.json。

进一步源码定位：PlayerTick无条件调用lancerhandle_visual::tick，该视觉入口在确认无持握之前先完整copy_frame；拟仅对软件model未held且无outstanding、无control.handle的无owner路径提前返回，所有已有撤销／stop／owner流程保留。尚未修改或验证运行收益。中段closing身份复用仅设计review，因发现阶段占主导且有瞬时变化／observer风险，不实施。Native／PG计数为分离RPM端点，本次body／hands1165与acquisition／设备1166相差1，报告保留此边界限制。

已按原worker返回的gate在全新私有perf-pg-idle-handle-visual-20261116源树落盘：只在原GT／retail／stop／revoking处理之后，对model未held且无outstanding、无control.handle的无owner视觉提前return。加入私有A/B DWORD及skip/read累计计数，其他路径及原fresh校验未改；当前仅source，尚未编译／运行／推广或证明收益。下一步验证旧owner撤销路径及真实跳过计数，再做完整PG构建和实际对照。


## 无owner Lancer手柄视觉入口优化（CJ，2026-11-16）

只在原GT／retail／stop／revoking检查后，当软件model未held、无outstanding、无control.handle时提前返回，避免原本读取完整状态后必然unheld返回的视觉工作。旧owner、停止、撤销路径保留。实际原／新源码入口提取编译，2048控制流检查通过（model／client／resources为host seam，非真实资源生命周期验收），控制器11项通过；完整PG编译通过。

组0: 原31.6889FPS，候选32.7533FPS，ratio 1.033589。

组1: 原29.6037FPS，候选32.7882FPS，ratio 1.107571。

组2: 原29.0069FPS，候选28.6620FPS，ratio 0.988111。

成对中位1.033589，2/3更快；CPU中位比0.964467、cycles 0.972736。真实skip／read路径全段验证=True；预定温和收益标准通过=False。

Do not promote this trial. Preserve actual route/FPS evidence and inspect which criterion failed before further changes.

仅新产生并成功提交的双眼游戏画面计FPS，复制计数交叉检查，XR重复提交不计。无录像／额外输入姿态；暖机25秒、6×10秒，每段两端裁2秒。身体／双手全段健康，正常退出、Native／入口／音频恢复、XR退出0、PG开关恢复，精确自有身份消失。仍限空手静止模拟器场景，无完整玩法或实体头显验收，120FPS未达到。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-handle-visual/main-runtime-review.json。

CJ归因限制补记：前两组resource快照无peer，最后一组出现其他会话实例lancer-tick-l2-1116-n9f65。同组peer名称相同不能证明其负载稳定，更不能证明它导致最后一组下降；未操作／停止该实例。候选真实每更新约skip1、source_read0，原路径约source_read1；已证明省掉无用完整读取，但仅2/3帧率对照更快，不推广稳定收益。后续若复核需针对资源cohort变化这一具体未决因素，而非盲目重复。


## 无效Troika性能方向撤回（CK，2026-11-16）

源码候选和16400 host检查完成后，完整编译首次发现registered在外层namespace，已修正并复测；私有PG编译成功。但Main随后才核对冻结基线和候选CMakeCache，TROIKA_INTERACTION均OFF，目标工程不定义该宏；configured直接false，因此这条发现路径不是当前热点。该可达性检查本应在实现／构建／启动之前完成。没有打开开关或修改当前负载。

CK经原入口启动并确认出双眼后，Main立即正常stop；0个A/B阶段、0次开关写入，没有有意义的性能结果。观察器正常退出，控制器暖机后见自有身份已退出而终末1，是主动取消之后的预期读者错误，不是Game崩溃；没有写入所以不存在未恢复的开关。Native／入口／音频恢复、XR0、无强退，精确自有身份消失。原手动候选未改变。

同时否决idle reload新增gate：dispatcher已在selected为空时返回，完全idle／unheld通常进不到step，不做无收益改动。下一步已核实活动代码：HAND_IK_STREAM等OFF而NATIVE_EQUIPMENT_TRIAL ON，PlayerTick走controller_locomotion::dispatch，prepare在轴零判断前完整copy_frame。评估仅返回bool的无运动需求预检查：在GT／注册／epoch／key／fresh都成立且原始左右轴为零时拒绝无工作路径；未知／stale／非零仍原完整校验和写／恢复，不能返回Frame或动作许可。尚未实现该预检查。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-troika-discovery-negative/main-review.json。

已在全新私有perf-pg-idle-locomotion-20261116树落盘bool无运动需求预检查及PG入口gate：仅同GT／armed／Gate／epoch发布／key／controller／fresh成立，且左原始xy零或inactive、右原始x零或inactive时拒绝原本不会写移动轴的工作。任何非零、未知、陈旧或变更回原完整copy／observe／写／恢复；不返回Frame／输入数据／动作许可，不改类字段布局。worker草案Gate bool／不存在key.pc／发布epoch不相等／clock后原子时序已修正。当前仅source，未编译或运行，没有收益宣称。

空闲移动预检查已通过94项真实HandClient+原Host检查：已发布零轴不发起Game观察，非零（含死区内非零）回原full observer；错GT／控制器／未发布／陈旧／时钟回退／epoch变动／Gate忙拒绝优化，Gate忙不解除原调用；未知Game元数据仍由原完整消费者拒绝。首轮fixture把inactive但坐标非零输入当作可发布，原Frame契约本来拒绝，已改为明确验证发布拒绝并保留失败日志，未改运行实现迎合测试。完整PG构建与实际帧率尚未执行，没有新性能结果或交付候选替换。


## 空闲移动完整构建和实际对照（CL，2026-11-16）

在冻结目标工程核对NATIVE_EQUIPMENT_TRIAL=1、HAND_EQUIPMENT_COMBINED_TRIAL未定义，PlayerTick实际使用controller_locomotion::dispatch（不是仅依据源码猜测）。94实际HandClient检查、11控制器检查通过，完整PG编译，Native保持b262、XR648。no_locomotion_demand_on_gt仅返回拒绝无用工作的bool，GT／armed／Gate／epoch发布／key／controller／fresh均检查，原始零轴或inactive才跳过；非零／未知／陈旧回原完整路径。

组0: 原30.0852FPS，候选30.0966FPS，ratio 1.000379。

组1: 原31.1040FPS，候选28.8761FPS，ratio 0.928373。

组2: 原31.5427FPS，候选29.2085FPS，ratio 0.925998。

成对中位0.928373，1/3更快；CPU比1.027233、cycles比1.042304。实际skip/read路由通过=True；全6段peer cohort相同=True；预定收益标准通过=False。

Do not promote; retain exact work-reduction and FPS/CPU evidence. Review failed criterion and active-proof/name-observer qualification before another change.

仅实际新提交双眼游戏画面计FPS，保留原Game动作校验／轴写／恢复；无录像或额外输入姿态采集。全部正常stop、Native／入口／音频恢复、XR0、PG模式恢复、精确自有身份消失；身体双手健康。站立零输入结果，不是走动／完整玩法／头显／120FPS验收。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-locomotion/main-runtime-review.json。

CL结论补记：真实省掉该入口完整读取，但2/3帧率更低，成对中位ratio0.928373、CPU ratio1.027233；本轮全部peer列表为空。因此关闭本改动、不推广、不原样重跑。既有16条bounded source-validation日志只有首条qualifications=1，其余0、fallback均0、order_hits在早期升到7后维持；这些不是完整按phase对齐记录，不能证明本次回归来自name-bound失效或排除所有其他因素。下一步要用按phase对齐的观察／验证成本证据解释，而不是把调用次数减少直接当帧率收益。


## 空闲移动按phase对齐成本（CM，2026-11-16）

保持CL优化源码不变，加入已审核CG包装计数／采样；新stride17与4／5调用数互质，避免stride16只抽同一调用位置。111组件检查、40读取一致性／采样位置检查、11控制器检查和完整PG编译通过。312byte私有表＋8byte序列回读，inflight=0且序列偶数一致才接受，最多8次立即尝试，不sleep、不写进程；只读Mod累计标量，没有Game输入／姿态或调用栈采集。

mode0三段中位：新双眼36.4955FPS，CPU 25.207182ms／body更新，完整观察5.004525次／body更新，完整观察wall估计4.439107ms，liveness 14.222222次／body更新，liveness wall估计4.451461ms。

mode1三段中位：新双眼36.3548FPS，CPU 25.852273ms／body更新，完整观察4.000000次／body更新，完整观察wall估计3.854062ms，liveness 14.201835次／body更新，liveness wall估计4.760166ms。

选择各段裁2秒后的首末一致快照，相同端点对齐CPU／body／equipment与观察成本，CSV只取该区间新提交／复制计数；wall不是exclusive CPU，不能与CPU相减作精确剩余。liveness包含fallback不重复相加。所有身体双手健康、正常退出及恢复、精确自有身份消失。本诊断未推广、没有120FPS／走动／头显验收。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-axis-cost/main-cost-review.json。

CM归因结论：同端点完整观察5→4，三组成对估计耗时均下降；liveness仍约14.2次／body，fallback罕见、equipment/body约1，未出现调用数量暴增。最末mode1的liveness耗时却比配对mode0高约45%，主线程CPU同步大幅上升；不能据此认定具体缓存或模块因果。

资源口径修正：require_memory只枚举本ROOT下isolated-worker.json，空peer列表不是整机无其他游戏的证明。现只读系统process_snapshot确有另一Game，但其创建时间在CM正常结束文件之后178.509秒，不能用它解释CM末段。旧phase没有系统级历史名单，保留归因限制；后续增加每checkpoint的全机已知Game／compiler身份记录（不增加启动限制、不操作其他会话），并反转开关顺序验证末段时间漂移。全机cohort helper的6项过滤检查通过，尚未用于新运行。

CM已有数据的短窗口复核：末组mode0内部已约37→32FPS、CPU/body约24→28ms；随后mode1后半段出现约12FPS／38..44ms CPU/body的短暂低谷，再回到约33FPS／25ms。约2秒CPU窗口及其内部CSV窗口并非原子、不严格同宽，不能作精确CPU残差或单次因果证明。它提示末段存在时间变化／卡顿，不是均匀稳定的开关效应。下一步反转mode顺序，并在每checkpoint保存系统级Game／compiler身份及自有Game fault／memory计数；保持同诊断二进制，不改功能来追求通过。当前未执行该后续运行或推广。


## 反向顺序成本窗口（CN，2026-11-16）

复用CM完全相同的fe51 PG／b262 Native／6481 XR二进制及既有stride17统计；无需重复编译。原111组件及40统计读取检查继续有效，本轮增加10内存读取检查并运行11控制器检查。顺序改为1,0,0,1,1,0，并在每checkpoint只读全机Game／compiler身份及自有Game内存／总缺页。312byte私有表＋8byte序列回读，inflight=0且序列偶数一致才接受，最多8次立即尝试，不sleep、不写进程；只读Mod累计标量，没有Game输入／姿态或调用栈采集。

mode0三段中位：新双眼37.1123FPS，CPU 25.455298ms／body更新，完整观察5.010638次／body更新，完整观察wall估计3.903976ms，liveness 14.264901次／body更新，liveness wall估计4.893187ms。

mode1三段中位：新双眼38.6792FPS，CPU 24.872449ms／body更新，完整观察4.000000次／body更新，完整观察wall估计3.276761ms，liveness 14.204082次／body更新，liveness wall估计4.617262ms。

选择各段裁2秒后的首末一致快照，相同端点对齐CPU／body／equipment与观察成本，CSV只取该区间新提交／复制计数；wall不是exclusive CPU，不能与CPU相减作精确剩余。liveness包含fallback不重复相加。所有身体双手健康、正常退出及恢复、精确自有身份消失。本诊断未推广、没有120FPS／走动／头显验收。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-axis-reverse/main-cost-review.json。


## CN全机cohort及自有Game内存／缺页

只读复用已验证Game HANDLE的GetProcessMemoryInfo；结构80bytes、DWORD无符号缺页；软硬缺页合计，不是硬缺页／磁盘IO证明。每checkpoint使用已有performance_guard.process_snapshot，过滤已知Game／cl.exe并取精确创建时间／镜像身份，不操作其他进程或新增启动拦截。

全程外部Game身份集合稳定=True，未识别身份记录0；最后阶段mode0，内部CSV窗口最低36.6697FPS，CPU/body最高26.0417ms，缺页最高6.3905/s。

这些是相关性数据，不是卡顿因果证明；所有owned恢复及清理已核验，未推广诊断或宣称120FPS。

证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-axis-reverse/main-resources-review.json。


CN决策：反向配对FPS比1.04619／1.04440／1.00582，CPU比0.96144／0.95307／0.98849；与此前CL／CM存在不一致，不推广空闲轴门控。此前末尾严重卡顿未复现，原因仍未证明。下一步审查已有独立启动链的游戏并行渲染参数及真实模式读取，保留完整身体／双手。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-idle-axis-reverse/main-decision.json。


## 游戏并行渲染默认模式（CO，2026-11-16）

复用已交付b262 Native／6a79 PG／6481 XR，现有独立入口visible／lowest／0.5，无录像，实际启动确认。两次间隔10秒只读精确自有游戏，EXE与关键代码哈希、命令键及三布尔字段双读均通过，实际parallel_rendering=1。原模式读取器仅调整工作区／实际同哈希游戏路径，并允许None表示观察实际值；12离线检查通过。

60秒观察，排除前10秒后有效49.214931秒／1885组不同双眼新画面，实际38.3014FPS；复制差0。身体双手commits=2434、solved=2436，Native body fallback=0。正常stop0、Native清理／入口恢复／XR0及自有Game退出，无强退。

现有Start-DebugGame.ps1增加两处有界代码：显式GEARSVR_TEST_PARALLEL_RENDERING_MODE=0或1只允许独立、behind、observe-only、mute、无foreground且禁用后台物理输入；默认未设置不改启动参数。保留全部旧内容，逆向移除新增块后字节相等，未重写启动平台。完整PS语法＋14提取真实块检查通过；首次测试脚本绝对路径Join-Path错误已修正，未涉及游戏启动。模式0尚未游戏验证，不推广、不声称提升。下一步用全新实例，同完整Mod比较模式0，并核对实际模式及身体双手；不从一次跨运行差异认定稳定收益。证据：analysis/lancer/workers/reflection-contract-deepseek-01/engine-parallel-mode-audit/main-default-runtime-review.json。


## 模式0及默认恢复（CP／CQ，2026-11-16）

相同b262 Native／6a79 PG／6481 XR和配置，既有独立入口visible／lowest／0.5，无录像。CP启动回执确认为-PARALLELRENDERING=0／-NoPauseUnfocus／-nomoviestartup；两次只读实际引擎mode0。CQ无模式参数，两次实际mode1。各60秒观察裁前10秒，CO／CP／CQ实际新画面FPS为38.3014／39.8308／37.7615；只是跨启动描述，未宣称稳定收益。CP身体双手commits2558／solved2560，CQ2431／2431，Native body fallback0／非零失败状态空。两轮正常stop／Native清理／入口恢复／XR0，精确自有身份退出，无强退。

CP／CQ各14个全机已知Game／compiler身份快照；CP没有其他身份，CQ第55.480秒首次观察另一个Game，前一第50.463秒未观察到。没有操作该进程，也不假设它造成全部差异。事后另外比较各轮10..45秒窗口：39.0950FPS, 40.1174FPS, 38.2631FPS；mode0相对前后默认比1.026152／1.048462。该窗口只是描述分析，不是预先验收标准；CO全机身份未知，工作负载／缓存／GPU仍未完全控制。

Mode0 is a promising small startup option for this fixed scene, not a demonstrated stable gain or the main route to120FPS. Preserve explicit test-only option; do not change default delivery or repeat unchanged launches now. Return to measured PG liveness/observer overhead.

证据：analysis/lancer/workers/reflection-contract-deepseek-01/engine-parallel-mode-default-return/main-sequence-review.json。120FPS／走动／战斗／实体头显验收未完成。


## PG current真实调用来源（CR，2026-11-16）

复用唯一DeepSeek worker输出counter／读取器，主会话保存、review并在独立冻结树集成。current原正文重命名current_impl，noinline wrapper只调用一次；逆向移除新增代码后正文精确相同。只记录Mod代码位置及计数／每17次抽样wall；不读额外Game字节／输入／姿态，不改原校验、动作或权限。404实际C++检查／19只读快照检查通过，完整PG编译567819b7…103e86e；原Native b262／XR6481不变。

自有实例启动确认，65秒观察正常退出。成本窗口34.063932秒，身体／双手／acquisition／blend-qualified各1240，无hard reject；current总17622次、每body14.211290、failed=0，16个代码落点全部精确DLL／PDB解析。有效CSV窗口33.148186秒实际新画面36.3821FPS，仅归因观察，不是收益对比。

- gears_equipment_runtime::Runtime::tick: 4.000000次／body，抽样inclusive wall估计1.580986ms／body。
- `anonymous namespace'::nativeequipment::quiet_unheld_inventory: 2.000000次／body，抽样inclusive wall估计0.640970ms／body。
- `anonymous namespace'::nativeequipment::next: 1.000000次／body，抽样inclusive wall估计0.329489ms／body。
- `anonymous namespace'::nativeequipment::stow_native::idle: 1.000000次／body，抽样inclusive wall估计0.292059ms／body。
- `anonymous namespace'::nativeequipment::read_input_continuity: 1.998387次／body，抽样inclusive wall估计0.567248ms／body。
- `anonymous namespace'::lancer_grasp::proximity_tick: 1.000000次／body，抽样inclusive wall估计0.266634ms／body。
- `anonymous namespace'::lancerreload_manual::prepare_starter: 1.000000次／body，抽样inclusive wall估计0.260521ms／body。
- `anonymous namespace'::nativelancerfollow::tick: 1.000000次／body，抽样inclusive wall估计0.255951ms／body。
- `anonymous namespace'::lancersaw_starter::tick: 1.000000次／body，抽样inclusive wall估计0.250514ms／body。
- `anonymous namespace'::nativeequipment_rig::acquire: 0.212903次／body，抽样inclusive wall估计0.063175ms／body。

九次不一致快照已丢弃；仅接受3616byte表＋8byte序列一致、偶数及inflight0、合法唯一可执行区RVA和完成计数，最多8次立即尝试，故障不重读。两个全机已知Game／compiler身份端点只作记录，不代表全机负载隔离。Native／entry／XR0和四个精确自有身份退出均确认。next：Prioritize measured no-work preparation branches in Lancer follow/starter/preparation and existing stow lifecycle. Do not remove Runtime/input/inventory identity checks or infer all weapons are active. Confirm local cleanup/publication obligations before changing a branch.

诊断不推广；wall不是exclusiveCPU，不能相加冒充整帧Mod耗时；ReturnAddress/PDB落点可受尾调用影响，不是完整调用栈。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-current-callers/main-runtime-review.json、main-ranked-callers.json及main-sites.tsv。


CR后续review：唯一worker及Main核对两项拟议早退：无绑定空手follow、相同starter Binding准备。HandClient::source_observation_on_gt在key／freshness／observe失败时会clear_gt或withdraw_expired_pose_gt；即使本地没有visual lease，这个publication撤销副作用仍不能证明可省。两项均未实现、未为它们运行Game。不能用后续消费者仍current替代该证明。后续转向保持全部校验与撤销语义的算法成本优化，不再泛化为所有武器都在运行。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-current-callers/main-no-work-review.json。


## 预取8补齐故障检查及实际对照（CS，2026-11-16）

复查旧组件4shape预取8均正向1.3..14.5%，但当时仅因未稳定>=10%而停，未有深故障或Game证据。此次不重跑旧性能循环、不增加新距离变体；唯一worker提供测试，Main修正9/16页矛盾、故障read次数及缺失Capture／第二遍trace／padding／fallback检查。173项Windows自有内存检查通过：16页逐页变异／NOACCESS、未来Guard页不会被hint提前消耗、第二遍变异仍拒绝、实际callback地址宽度次数顺序及Capture记录一致。全程无外部Game测试于该组件。优化MSVC汇编有prefetcht0，实际两遍read不替代。/Fa冒号误生成任务目录ADS已转存普通main_test.asm并删掉自有ADS；后续命令已修正。

私有当前冻结PG仅改BatchedProof.validate默认false模板bool和原SourceScope选择分支，逆向删除新增块后原正文一致；只加cache hint、显式Mod自有DWORD与选路计数，原范围／顺序／两遍读取／比较／fault／撤销保持。完整PG2fd2ffbf…a54b61c编译，精确PDB mode RVA4440240／size4数据段绑定及11真实控制器fake API检查通过；Nativeb262／XR648不变。

CS现有多开visible／lowest／0.5／私有120设置、无录像。实际启动确认，90秒观察、25秒暖机、6×10秒同进程对照，两端裁2秒。三组原→prefetch新画面FPS：14.9135→20.6249、20.8862→15.2630、23.8424→20.6629；配对比中位0.866643，只有1/3更快。Scene CPU／body比中位1.100193、cycles1.123777，均不支持收益。每段选中计数>0、另一路0，约14验证／body；这不是硬件cache-hit计数。六段身体／双手健康，正常stop0／Native清理／entry恢复／XR0、Mode恢复、Scene thread handle关闭均通过。

全机已知Game／compiler cohort跨phase改变，并有1次他人Game退出导致身份无法解析，不能归因全部差异或断言hint内在回退。没有操作他人Game／compiler。派生控制器原始private_data_symbol和local64 phase标签为遗留文本；实际操作2fd2匹配PDB prefetch8_mode和选路计数已独立核对，未含local64实现。原始记录保留，解释见main-runtime-review。

Do not promote. This run did not establish gain and has changing global game/compiler cohorts; intrinsic regression is not proven either. Preserve tested binary and allow at most one bounded reverse-order confirmation after rechecking actual current cohort, without rebuilding or repeating component benchmarks.

补充：整段装备tick消融未实现，因为false会触发外层fire／reload补跑且pre-original生命周期存在；不能把功能／隐藏／渲染负载变化当原段CPU差。当前手动候选和Root运行模块未替换，120FPS未达。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-prefetch8-safety/main-runtime-review.json、main-safety-review.json、main-source-review.json。


## 预取8反向核验与方向关闭（CT，2026-11-16）

复用CS相同2fd2 PG／b262 Native／6481 XR，不重编译。启动前全机已知Game／compiler为空，只确认CS自有身份退出；现有独立visible／lowest／0.5／无录像入口。模式1,0,0,1,1,0；25秒暖机、6×10秒两端裁2秒。三组成对原→prefetch实际新画面FPS：28.2979→30.0390、28.8064→25.2741、21.6508→24.0582；配对比中位1.061526，但只有2/3更快。CPU／body比中位0.980769、cycles0.977230，各2/3下降。每段选中计数>0、未选中0，约14.2..14.4次验证／body，六段身体／双手健康。全程已知Game/compiler cohort稳定空，未解析身份0；仍非全机所有负载证明。模式恢复／线程句柄关闭／普通stop／Native清理／entry恢复／XR0及四个精确自有身份退出均验证。

CS受进程变化干扰、CT反向仍不一致，所以关闭该方向，不作第三次原样测试，不推广；该结果不证明hint内在回退。已有bounded日志末三条qualification0／fallback0、8060reads/pass，未见这些样本中反复资格失效爆炸；这些不是完整时间对齐证明。当前120FPS未达到。

下一方向仅只读审查Native Reset锁边界：实际配置下owned_source→ngx_output的OFF reset stub是直接original forward，compute ResetScope也是编译关闭stub；普通world路径仍把driver Reset包在global frame_lock内。FrameLink reset_success明确保留已提交candidate并只废弃未提交旧记录，为线性化审查提供具体依据；CPU Execute/Reset并发、Present旧记录、递归锁及failure/drain尚未证明，所以没有移锁或改Native。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-prefetch8-reverse/main-runtime-review.json、native-reset-lock-audit/main-audit.json。


2026-11-16 Reset 私有原型：仅当两套 recorder 均无未完成记录时，在原始驱动 Reset 周围释放当前全局帧锁租约；Reset 后跟踪仍持锁。实际钩子控制流 3305 项、真实 Tracker 模型 238 项、原 frame_link 39672 项均通过。完整 Native 正在独立目录编译，尚无实际游戏收益证据，不替换交付。证据 native-reset-deferred-lock/。

私有Reset锁范围原型已完整编译，Native1654c3ee…1b004e。CU同局三组实际新增双眼帧：17.149→16.490、18.734→17.119、18.780→18.531 FPS；安全快路实际命中，原路径释放计数0。两个优化时段未通过原身体/双手健康检查：159/163提交比与另一次hard_rejected1，拒绝交付，不放宽检查。另一游戏全程在场，不能证明单实例上限或因果退化；不重复未变化实验。Native/入口/XR和模式正常恢复，自己身份均退出。当前交付b262保持。证据native-reset-deferred-lock/main-runtime-review.json。

CV六段诊断：29项计时检查与完整PG编译通过，160个一致样本；实际新增双眼14.2892FPS。严格身体连续性验收失败，511提交/514获取，不修改门限。identities 0.020037ms/0.805%；fields 1.775139ms/71.355%；flags 0.130416ms/5.242%；state 0.003179ms/0.128%；closing 0.558451ms/22.448%；extras 0.000551ms/0.022%；Native使用原b262；另一游戏在场，墙钟含调度，非独占CPU归因或优化收益。自有4身份退出，Native/入口/音频恢复。下一步仅审查完整观察的字段resolve与既有prefix机制兼容性，保留所有最终复核。证据pg-discovery-phases/main-review.json。

字段query replay私有原型CW：仅Binding/Pawn begin包裹实际resolve；外Capture绕过、proof两遍验证、changed原查询、unreadable拒绝、不成功seal保留原resolve成功且不缓存；原全部closing及函数体逆变换逐字一致。61原观察器+35定向检查通过，长链人工内存3组组件耗时约降44%，短链混合；完整PG c6acb36d…97700f0编译。实际同局原→候选：27.036→23.725、21.276→18.692、24.480→26.867 FPS；身体双手六段均健康。只有1/3更快，Scene CPU每身体更新三组均增约14–21%，不推广。约9hits/6cold/3bypass每body，反复重建的原因尚未实测；不得把padding假说当结论。已正常恢复Native/入口/音频/开关，XR退出0、4自有身份消失。下一步定位实际key差异，不原样重跑。证据field-query-replay/main-runtime-review.json。

CY本地Snapshot差异诊断：完整PG编译、8项分类检查、实际双眼启动和自有正常清理通过。最多36行，仅在valid+同object+同phase且Snapshot字节不同时记录；实际0行。未同步采样hits/cold，不能据此归因为padding，下一步补齐各级miss门槛分类。CX辅助脚本启动前索引失败，未开游戏；自有孤立observer5644已按确切命令/启动身份清理。证据field-query-key-diff/main-runtime-review.json。

CY诊断解析修正：此前误匹配field_query_replay_key_miss，实际标记field_query_key_diff，共36条，撤回0条结论。三类query的首次差异均7828，Structurepadding差异均0。缓存键可能包含NameText未消费尾部；先进行名称语义键逐字节边界验证，不跳过原proof与closing。

名称尾部语义键私有PG71dbad3b…194e05：33210逐字节键检查、35复用边界、61原观察器案例及完整PG编译通过；保留proof/Capture/resolve/closing。CZ实际新双眼30.647→33.820、27.266→33.584、32.103→31.641；DA反向24.437→26.850、22.686→17.952、25.826→25.193FPS。两次身体双手均健康。优化时段cold均0、name_tail_hits实际>100，证明原先名称未消费尾部导致假失配；但6组仅3组FPS提高，收益未稳定，不推广、不原样第三轮。两次自有实例正常清理/模式恢复。证据field-query-name-tail-reverse/main-sequence-review.json。

DB单SEH注册读取私有PG79e9f167…45b7a3：571真实4/8B轨迹/输出/NOACCESS/GUARD/溢出检查、61原观察器案例与完整PG编译通过；原字段/名称/closing和Cache均未改。修正组件使用生产fixed_copy和MAIN_MOUNT宏、关闭不对称read计数、每TickScope5次observe；5组均更快，中位wall ratio0.9073，初版偏置结果另存不采用。实际新增双眼20.910→23.841、23.642→21.097、28.181→27.515FPS，身体双手均健康、路径稀疏采样证实，仅1组更快，未推广不原样重复。正常清理/模式恢复/4身份退出。原路径首末FPS相差34.77%，需量化基线负载稳定性，不能猜测流式载入/温度/外部进程。证据registered-seh-scope/main-runtime-review.json。

DC已接受基线只读稳定性观察：Nativeb262+PG6a不变，无模式写入，31项WinAPI假调用/自有Python检查；86秒1Hz进程/Scene CPU、内存和系统时间采样，与实际新增双眼CSV对齐8个10秒窗口。FPS22.994..24.845，中位24.172；Scene CPU每身体更新中位35.305ms，占0.825..0.888核；Game进程1.63..1.79核，全机16逻辑核总忙碌69.97..74.85%。身体双手8窗口健康，正常清理/4身份退出。
范围修正：旧global cohort只覆盖两个Gears EXE和cl.exe，不能排除所有其他游戏。基线结束后5秒只读CPU采样见GTA5约1.51核、pi_server约1.92核等；同一后续GPU快照见GTA5 3D22%%、pi_server 3D8%%。这是后续状态，不是先前各轮因果证明；未终止/调整任何其他进程。PageFaultCount不等于硬盘换页；系统CPU忙碌度依Windows kernel含Idle口径扣除Idle。下一步按原Scene/原调用边界归属实际模块CPU工作，不继续原样微优化AB。证据baseline-stability/main-runtime-review.json。


### PlayerTick线程周期分段，2026-11-16

私有dd实例正常停止，精确四个自有身份退出、Native/入口/静音恢复、XR0。136个stride17有效采样，无nested、周期失败或倒退。采样PlayerTick周期中装备入口67.8637%，均值墙钟8.2167ms；原调用（含嵌套Game/Native/PG）17.6472%/2.2384ms，调用前Mod8.4599%/1.0907ms，后续跟随/换弹/可视6.0094%/.7179ms。不是整帧或模块独占时间。8个窗口身体双手健康，实际新增双眼29.399–34.805FPS；存在编译负载，未证明优化收益。下一项拆分装备入口/各武器前置/库存Runtime/跟随恢复，保留所有功能与校验。记录：analysis/lancer/workers/reflection-contract-deepseek-01/tick-cpu-phases/main-runtime-review.json。


### 装备内部周期分段，2026-11-16

DE实例87个有效stride17采样、71个coherent观察，周期失败/倒退/nested均0，eligible/body=1。原锯齿所有权判断＋库存Runtime占所测装备周期72.4209%，均值墙钟10.3679ms；Frame读取13.8962%/1.9718ms，fire/follow6.7629%/.9243ms，starter/grasp/Longshot及可选observers和有界报告6.7069%/.9233ms。phase10原标签bounded_cost_reporting不完整，已更正starter_observers_cost_reporting，不得把整段归因于日志。其余各武器前置在该站立未持握场景占比很小，不外推到操作中。不是整帧/独占模块CPU或优化收益。8窗口身体双手健康，实际新增双眼18.548–22.284FPS；另有Gears进程，系统忙碌74.6–87.4%，不得作跨轮因果对比。自有实例正常stop，Native/入口/静音恢复，XR0，精确四身份退出。生产候选未变；下一步细分state.tick的currentness/input/inventory/idle visibility，保留新鲜度、撤销与动作处理。详见analysis/lancer/workers/reflection-contract-deepseek-01/equipment-cpu-phases/main-runtime-review.json。


### 库存Runtime内部周期分段，2026-11-16

DF私有实例133有效quiet-unheld采样，无nested/周期失败/倒退。入口current20.7267%/1.4600ms、next输入及收尾current21.5409%/1.4733ms、库存query22.5150%/1.5491ms、库存closing/model4.7548%/.3232ms、idle visibility23.9823%/1.6418ms、idleclosing6.4559%/.4414ms。均为所测Runtime周期比例/平均墙钟，不是整帧或各payload独占；backend内current校验仍包含在该backend区间。133个样本全部走quiet分支，不能外推到持枪/换弹。没有单一独占query，下一步区分重复source validation/native-name工作和backend实际读取，不删除current或idle清理。135用例/1743 actual Runtime+model检查和23读取器检查通过；私有命名空间计量适配器链接错误已在启动前修复。8窗口身体双手健康，实际新增双眼30.296–33.500FPS；其他Gears/编译负载存在，不能归因提速。正常stop，Native/入口/静音恢复，XR0，四自有身份均退出。生产候选未改、120未达到。详见analysis/lancer/workers/reflection-contract-deepseek-01/inventory-cpu-phases/main-runtime-review.json。


### 无padding批次整段比较，组件筛选关闭，2026-11-16

私有候选仅为sealed batch增加本地dense形状标志，保持原两遍读取及读取请求顺序/大小/首故障退出，只对dense且>=64字节改用memcmp，其他masked路径保留。105836检查通过，另一次定向安全复核强制覆盖计划容量耗尽fallback；包括见证/填充逐字节变化、第二遍变化、读取trace、NOACCESS/GUARD、begin重用。首版8065组超过SourceProof1MiB容量的性能数据作废，组件baseline/candidate均改2MiB后筛选，生产alias未改。1024/8065小批次shape3周期中位比为1.051452/1.054629，均未通过筛选；不编译游戏DLL、不运行游戏、不合入、不重复不变实验。详见analysis/lancer/workers/reflection-contract-deepseek-01/pg-dense-batch/main-review.json。


### 源校验周期归因，2026-11-16

DG完整窗口1888个stride17有效样本，周期失败/倒退/nested0，源检查/body14.2508。原软件准入之后的所测区间中，proof.validate两遍重读/比较占93.0209%周期，平均墙钟.34369ms/检查；名称前/后1.21596%/1.14014%，fallback capture3.81096%，seal/closing.81123%。原software_current与未admitted原observe路径不在分母，不得外推整Mod/整帧。实际新增双眼27.699–31.714FPS；有编译负载，已知Gears/cl采样不覆盖所有host应用，无优化收益归因。8窗口身体双手健康，正常stop，Native/入口/静音恢复，XR0，精确四身份退出。下一项由同一worker实现紧凑见证segment比较组件：保持原两遍整batch读取，包括padding故障语义，仅减少本地mask/expected比较数据。组件无稳定收益即关闭，不直接启动Game。详见analysis/lancer/workers/reflection-contract-deepseek-01/source-cpu-phases/main-runtime-review.json。


### 紧凑见证segment比较组件关闭，2026-11-16

原型保持原两遍整batch读取与fault语义，只在本地用见证segments/compact比较代替expected/mask；worker首稿seal回退语义已在编译前纠正。v1/v2各10213336检查0失败（覆盖重叠，不累加），含逐字节两遍变化、trace、读取失败、NOACCESS/GUARD、disabled-plan/容量fallback。v1小批次1024/8065周期中位比1.229846/1.289137；v2将局部plan bounds一次性验证移到seal，validate期间plan与Base同样要求不可变，不省Game读取/身份新鲜度校验，小批次仍1.225342/1.153244，未过预先筛选。组件都不进入Game，不合入，不重复不变试验。合成小批次只近似均值，不等同真实完整plan分布；下一项先读取有界本地plan聚合长度/见证/填充/segment直方图，不导出Game地址/数据/输入/pose，再选算法。详见analysis/lancer/workers/reflection-contract-deepseek-01/pg-segmented-batch/main-review.json。


### 真实sealed plan聚合，2026-11-16

DH仅在原有16行有界cost日志observed512位置一次读取本地proof计划/mask聚合，没有导出Game地址、值、输入或pose。46项golden/非法计划检查通过；原baseline6a加日志，不带前几轮CPU meter。8061批，见证439144B，计划899708B，填充460564B(51.1904%)，dense719(8.9195%)，FF runs25710，平均3.1894/batch；长度65..128有4910批(60.91%)。见证run长度<=8有16178段，17..32有6091段。此前组件仅近似均值，不能等同真实plan。尾数mod16分布未证明；下一项只组件测试本地比较buffer round16，Game每批读取仍原地址原size，禁宽读Game。8窗口身体双手健康，实际新增双眼30.931–36.536FPS，不作收益归因；正常stop/Native/入口/静音恢复/XR0，四精确身份退出。无已知Gears/cl干扰不排除其他host应用。详见analysis/lancer/workers/reflection-contract-deepseek-01/plan-shape/main-runtime-review.json。


### 本地round16缓冲组件关闭，2026-11-16

14743469检查0失败：长度1..1024逐见证/填充字节、两遍真实边界、相同read trace、外层Capture、故障/guard、精确Game原读取末端紧邻NOACCESS、disabled/capacity回退。只round本地expected/mask/now比较，不扩大Game读取。shape4主样本1024/8065周期中位比.978050/.910318，均仅3/5更快，未过<=.97且4/5门槛；8065对齐控制中位1.035901也超1.03。其他分布虽改善，不足支持Game试验。关闭，不合入，不启动Game，不重复不变微调。下一项检查冷Name文本第一个NUL后的非语义尾部是否被不必要纳入SourceProof，保持原物理读/故障与所有语义字节。详见analysis/lancer/workers/reflection-contract-deepseek-01/pg-rounded-batch/main-review.json。


### 冷名称语义witness私有原型，2026-11-16

仅source qualification的精确Capture/NameCapture token内，原page-bounded physical read完成后，将当前chunk第一个NUL及之前字节登记到原SourceProof.record，前序无NUL chunks全登记，其他读取及重叠必需字节不动。原物理读取、首故障不重试、warm/full-payload/身份/flags/closing/expiry不变。原worker两版存在范围登记/恢复/状态错误，main在运行前修复关键事务状态，保留拒绝记录。helper55检查、chunk宏开启实际SourceValidation32用例、46项名称/source公开结果及physical trace/尾变/字符与NUL变化/重叠/跨页NOACCESS-GUARD oracle通过。单名204→38B，源proof7032→2418B。实际observe产生的人工metadata proof，filler0/128/256周期中位比.816836/.790614/.905175，4/5、5/5、5/5更快，组件筛选通过；非游戏收益。下一项冻结Scope模式并在模式变化时重新qualify，再编译私有同Game AB候选，防止复用错误模式旧proof。尚未编译整PG/运行Game/交付，120未达。详见analysis/lancer/workers/reflection-contract-deepseek-01/pg-name-witness/main-offline-review.json。


### 冷名称语义证明候选：两轮真实游戏对照（2026-11-16）

私有名称读块保留原始页内读取、失败行为及公开结果，只在SourceProof中保留实际字符与首个NUL；原始slot/index/header/closing、名字域与两遍重读保持。新增root冻结模式、模式不匹配重新资格检查、非法模式完整回退与分模式root/成功资格计数。helper55、原source32、扩展实际类名尾部oracle55、mode38、控制器gate21检查通过；私有PG79a76b86编译通过，Nativeb262与XR6481保持。

首次DI因观察时长120超过现有90上限而未得到确认/测量，已正常取消，精确自有进程退出，无强退；失败记录保留。改用新名DJ及DK，visible/lowest/0.5，禁止录像，两次均startupconfirmed。DJ原始31.480/30.896/31.841，候选31.492/32.157/33.029FPS；DK反序原始26.675/18.664/29.898，候选30.399/22.339/25.514FPS。每组10秒、两端裁2秒，只数新增双眼画面。共5/6更高，但第一轮CPU中位比1.0013未一致降低；反序有大幅漂移与一组约14.66%下降。不得把5/6或整体均值作为稳定因果收益证明。全部accepted区间模式回执、身体双手健康；两次正常Native/入口/音频恢复、XR0、四个精确自有身份退出。已知Gears/cl抽样未见其他游戏；反序出现编译进程，具体影响未量化，不能排除其他宿主负载。

现有低频SourceProof日志出现436856B/8058reads与400882B/8122reads两种计划，日志不含显式mode标签，不能将其直接指派给某模式；减少witness字节也可能伴随更多batch，必须核对真实planned复制量。当前不替换交付版本。下一步只补私有本地qualification计划回执与已有WindowsMetrics，分辨真实减少的工作量和宿主漂移。详见analysis/lancer/workers/reflection-contract-deepseek-01/pg-name-witness/main-two-run-review.json。实际120FPS仍未达到。


### 名称证明按模式计划与宿主CPU核对，2026-11-16

私有PGf8530e2e在成功qualification后发布本地plan stats，单GT观测序号odd/even，非batched planned显式UINT64_MAX，reader拒绝部分/零/非法记录；不会增加Game对象读取或每次validate遍历。45个mode/publication检查、23个reader检查、32个原source用例及完整PG编译通过。复用已31项验证的WindowsMetrics，新增精确进程creation校验、process/system CPU与working-set/private-commit/pagefault区间；PageFaultCount不代表硬盘缺页。PDB匹配并验证模式4B/计数8B对齐与私有DLL可写非执行区域。唯一既有DeepSeek worker提供发布代码，Main核对实际planned_bytes成员、修正fallback/对齐/overflow/边界并执行检查。

DL新实例visible/lowest/0.5、无录像，startupconfirmed后同Game0/1/1/0/0/1六段，切换等新qualification和root回执。实际新增双眼FPS34.820→33.195、30.694→31.631、30.089→32.844，2/3高，CPU每body配对中位比0.990607，cycles0.983303，未证明稳定因果收益。身体双手六段健康；正常恢复模式、Native/入口/音频，XR0，四个精确自有身份退出，无强退。

实际mode0 witness438650B、planned899734B、reads8065/pass；mode1 witness400882B、planned865128B、reads8122/pass。witness减少8.61%，计划复制字节仅减少3.85%，读取反增0.71%。全部仍两pass。三段mode0成功重新qualification59/54/70次，mode1为1/0/0；这些是整10秒计数，FPS/CPU裁前后2秒，不能视为逐帧故障归因。单组计划稳定，但还没有记录每次旧proof不匹配的具体字段。

16逻辑处理器、单group，匹配CPU窗口systembusy39.8%..52.9%，出现编译进程；不能从这些采样量化外部负载对某组FPS的因果影响。进程指标API最长0.2904ms。当前不推广、不原样重复FPS实验。下一项组件候选把校验读调用的SEH边界提到一次完整validate，严格保持原batch地址/宽度/顺序、两pass、observed与首次fault不重试；只在实际memcpy/observed区域catch，防止把本地plan/compare缺陷误记成Game不可读。先离线故障/轨迹与组件成本验证，再决定是否构建游戏候选。证据analysis/lancer/workers/reflection-contract-deepseek-01/pg-name-plan-receipts/main-summary-review.json；实际120FPS未达到，交付候选保持。


### 完整校验单SEH边界：组件通过、实际游戏无收益，2026-11-16

复用唯一DeepSeek worker。首稿bool返回/不存在proof.unreadable/占位run接口不匹配，退回后改为调用原Proof::validate的Result模板；Main加compiler barriers和明确outer符号，原完整proof计划、两pass、successful observed、首fault一次unreadable保持，只在memcpy/observed读阶段catch，local compare/plan异常CONTINUE_SEARCH。安全测试接实际SourceProof算法，6304068检查零失败，含1..1024每字节两pass、padding、精确trace、Capture实际bytes/ranges、NoAccess/Guard首中末两pass、observed故障、实际源第二遍突变、读前/读后本地异常逃逸、空/非法/unbatched回退。扩展测试曾因未对齐夹具跨页而错误假定单batch，修正页内夹具后通过；候选代码未为通过该故障而改变。测试输出已限制20failure停止。

组件计时关闭观察seam，原一个共享plan和实际两遍validate，五组AB/BA。8065小块shape cycle中位0.805556/5of5更低；实际近似3runs/80..126B shape cycle0.952126/4of5更低，通过预设组件筛选。组件数字不是游戏FPS。独立PGf2e922dc基于已验证6a源，不混入未推广名称语义候选；Nativeb262/XR6481保持，原所有编译配置继承。11项实际controller函数自检通过，PDB/模块身份与private4Bmode精确绑定。

新实例DM visible/lowest/0.5，无录像，startupconfirmed后同Game三对。实际新增双眼35.558→30.332、35.130→34.633、35.937→33.575FPS，0/3更高，中位比0.934258；SceneCPU每body中位比1.02807，cycles1.06529。候选真实route命中，六段身体双手健康。正常恢复模式、Native/入口/音频，XR0，四个精确自有身份消失，无强退。结论关闭，不推广、不原样重测；组件改善不能代替游戏收益。

下一步转源代码审计每帧重复liveness检查的调用点及其间的真实引擎副作用，先明确Native所有权/currentness义务；不缓存旧bool、不跳过当前保护、不直接实现事务复用。证据analysis/lancer/workers/reflection-contract-deepseek-01/pg-validation-single-seh/main-runtime-review.json。实际120FPS仍未达到，已交付版本保持。


### 重复current副作用复核与可选profiling组件关闭，2026-11-16

唯一worker读片段审计指出next/continuity/inventory前后current守护实际读或输入处理；Main复用旧pg-current-callers/main-no-work-review.json，source_observation_on_gt失败会clear_gt/withdraw_expired_pose_gt，不能因空闲无操作省掉撤销。精确命名getter路线也已0hits关闭，本轮未重新启动Game查getter。未删任何current/freshness/identity/closing。

发现接受Nativeb262仍编译GEARS_NATIVE_RENDER_COST_PROBE=ON；初始固定窗口后TLScurrent为空，但safe_copy依旧含原Scope。复用原header与原guard，ON/OFF分别独立TU/namespace避免ODR冲突；Main修正worker错误构造参数、移除额外atomic及OFF不存在current写入。2091正常/NOACCESS/GUARD/页尾/activecounter检查通过。5轮AB/BA，hot cyclemedian0.932764且5/5更低；预设primary散页cyclemedian0.951351但仅3/5更低，未达4/5筛选。因此关闭该组件，不构建Native、不运行Game、不修改交付；不能从热缓存组件直接宣称FPS收益。证据native-render-probe-off/main-component-review.json。

### 原始FName intern路径新静态线索（未建立优化证书）

在本会话保存的历史RX/只读literal数据库副本分析，原DBhash保持51d39e3d。IDA连接缺ida_nexus失败，复用既有IDA批处理仅打开新私有副本，三次正常exit0；未安装插件/改全局环境或碰其他数据库。WorldInfo消费者调用1dfb70，其37B包装进入1dfba0。展开直接CFG跨unwind碎片，179条指令显示ASCII大小写规范hash、低16bit bucket141B9D080、node+0x14文字比较、命中返回node+0index、不等走node+0xC链；未命中mode0输出0，非0经ArrayAdd/allocator写GNames141C34800。此为intern候选的静态证据，不是getter/可调用ABI或唯一性证书。

比较器IAT14131D198、Unicode分支8982c0、mode2 replace分支898342、allocator及并发/唯一性均未证明；历史字节还需当前进程匹配。下一步有界补这些证据，评估能否在保持身份/number/closing义务下减少非匹配字段的重复名称解码。未实现NameID负筛选，未删fresh_name，未调用引擎API，本轮没有Game运行、没有新FPS结果，120FPS未达到。证据fname-intern-static/main-review.json。


### 原始名称代码绑定核对与冷读扫描关闭，2026-11-17

补全冷分支及Unicode selector的12个跳转目标。mode2可在原数字ID位置经wcscpy_s改写名称文字，所以不得假定ID不变即文字不变。DN独立实例仅用QUERY_INFORMATION|VM_READ读代码/函数指针，37+758+445B代码双读匹配历史；比较器实际绑定ucrtbase _wcsicmp，替换为wcscpy_s，数字后缀为_wtoi/_wtol，目标前32B匹配磁盘。未调用引擎函数。Native/入口/音频正常恢复，XR0，全部记录的所属身份均消失。此结果补充上一节尚未绑定的状态，但不证明唯一性/并发/负筛选等价；未删fresh_name，未实施NameID负筛选。证据fname-intern-static/main-review.json与main-dn-cleanup-review.json。

复用唯一DeepSeek worker生成本地UTF16 first-NUL SSE2辅助函数，562509项本地检查通过。接真实fresh_name的1774项输出/轨迹/observer检查通过；冷路径5/5 cycles更低，中位比0.846777，暖路径仅1/5更低、中位1.014172，因此仅保留冷读变体继续筛选，暖代码不变。完整实际observe_game每TickScope五次，0/128/256额外字段三形状各5组AB/BA，12项公开输出与精确读取轨迹检查通过；cycle中位比分别1.025800/0.983298/0.998659，低于原版分别2/5、5/5、3/5，均未达到预设每形状median<=0.97且4/5更低的标准。关闭，不做完整PG构建或Game测试，不推广。扫描函数收益不能外推完整装备观察，更不是游戏FPS收益。证据pg-name-nul-scan/main-whole-observer-review.json。

下一步定位persistent proof重新qualification的首次原因/变化字节。唯一worker生成仅操作已读取本地buffer的first-mismatch helper，Main编译并执行651365项零失败检查，覆盖全部长度/首次差异位置、256xor×256mask、高位、空值/溢出、NOACCESS页尾。当前仅辅助函数完成，尚未接入校验器或游戏日志；不能宣称原因已定位或性能已提高。计划仅在原changed分支扫描已有本地buffer，保留全部游戏读取、比较mask、两pass、失败返回和撤销语义，再用有界原因计数/样本识别原始触发字段。证据pg-proof-mismatch/main-local-review.json。120实际新画面FPS目标未达成，已交付组合保持。

### 参考“查明真机调试帧率”会话

核对原会话与实际文件，早期安全读取SEH memcpy和帧钩子CRITICAL_SECTION已经存在当前capture.cpp；菜单像素优化也已继承，菜单纹理更新不算关卡FPS。原会话70.408FPS为59.183秒4167对新增双眼画面，非重复率，但第三人称广场/旧功能组合、scale1/最高/FSR Quality不同，且原会话自身尚未完成因果ABA；不能直接和当前完整身体双手版本比较。当前已有CO/CP/CQ默认1/模式0/默认1的实际38.301/39.831/37.762FPS，保留小幅改善线索及宿主负载限制，无稳定收益结论、不改默认、不原样重复。参见engine-parallel-mode-default-return/main-sequence-review.json。


### 首次proof变化字节实际诊断，2026-11-17

在接受PG6a父源码新私有副本perf-pg-proof-mismatch-20261117，只于Base/Batched原changed分支调用本地first-mismatch helper，成功路径不额外扫描；原读取/两pass/mask/返回保留。唯一worker提供note头文件，Main用真实数组.data()/范围索引集成；对照6305798检查零失败，覆盖全部1..1024长度各首差异位置及两pass、read失败、masked padding、base回退和exact trace/result。原32项WeaponSourceValidationTest通过。完整PGcd43275d编译，非性能候选，未推广。

新唯一实例DO visible/lowest/0.5，无录像，startup_confirmed=true后观察60.484秒。18条有界首次changed样本都在同一cold名称ordinal4009附近，其length4，差异位于名称文字开始后140/172/176/180B，即NUL之后；mask均255。包含pass0/1、cross0/1。最近名称192B地址关联只是候选来源，不是完整read-origin证明，尚不能声称所有变化都是名称尾部。前1920scopes累计changed311、seal3、ineligible35；domain/observer/software/cross_names_domain/unreadable/closing等为0。计数记录到1920后停止，样本前16加每128一次，不是全程所有变化统计。诊断FPS不作收益结论。

正常Native/入口恢复，XR0，launcher0、owned_game_absent，未强退；逐creation/image查询3个去重所属身份均已退出。未操作其他会话。证据pg-proof-mismatch/main-runtime-review.json和main-source-review.json。后续检查名称长度表负载：至少4010个唯一名称对4096槽、16探测上限，可能使部分名称反复走cold大块读取；现有证据未记录实际length_bound miss，不能断言已证明碰撞。先评估更大长度表的实际缓存miss、重置成本和完整观察成本；保留全部slot/index/文字/终止符/身份/收尾验证。不原样重复已关闭的semantic witness候选，实际120FPS仍未达成。


### 名称长度表探测耗尽与真实插入轨迹，2026-11-17

唯一worker提供有界本地插入trace和组件计时代码，Main集成独立诊断副本、修正activation饱和/计时比值new÷old/文件大小上限/计数器失败退出。DP实际确认startup后60秒：前1920scope计数lookups100234477、no_scope27168000、hits53992696、empty14517233、16probe耗尽4556548；remembers18066357、inserted14517233、same0、插入耗尽3549124，插入请求19.645%耗尽。三种lookup路径与remember分解计数精确相加。首次第16activation为空，24B空trace拒绝，不用于重放或计时。原始失败保留。365074本地状态/返回/顺序检查，原46320MetadataParse及32SourceValidation检查通过；此版未推广。

修正版把一次分配延迟到activation16后的实际remember，复用缓冲最多64非空scope，occupied>=3500且count>=4000才选中；overflow无效。既有MetadataParse测试抓到restore(nullptr)时诊断workspace0误解引用，Main加active显式守护及本地反例，365076检查与原46320项重跑通过，完整PG01c0201f编译；失败前未启动此版Game。

DQ全新实例visible/lowest/0.5，无录像，startupconfirmed后30秒，正常Native/入口恢复，XR0、launcher0、无强退；3个去重creation/image所属身份确认均退出。选中6376个实际remember事件、4244不同键、actual occupied3811，未截断。离线按原hash和16probe重放4096容量精确得到3811占用：433个键未保留、2565次插入耗尽；8192容量仍3键未保留/14次耗尽；16384及32768在本轨迹全部保留，16384比32768清表内存少。一次轨迹不能代表所有场景，NativeNames独立4096容量不变。

复用实际原解析器，仅length表4096→16384的本地组件5AB/BA：reset_only cycles中位2.983112、wall3.010806；单次reset墙钟中位约1.412us→4.313us。按实际remember序列先lookup、miss才remember的重放cycles中位0.248187，5/5更低，wall中位0.239611；每次组件重放约100.538us→24.207us。汇编观察保留完整名称/epochs memset及循环。此组件不模拟Game读取、完整observe_game或全部实际lookup，不能宣称FPS提升。10类损坏trace均拒绝。下一步从接受生产源准备16384候选，先完整观察路径与切换验证，再决定实际游戏A/B；全部fresh slot/index/文字/NUL/收尾保护和作用域重置保留。实际120FPS未达到、交付组合未变。证据pg-name-bound-trace/main-runtime-review.json、pg-name-bound-trace2/main-replay-review.json和main-runtime-review.json。


### 16384长度表完整路径与实际Game对照，2026-11-17

从接受PG6a父源码独立构建，无前轮采样。唯一worker首稿通用regex未匹配真实合并constexpr/原hash表达式，退回后用精确anchor变换。4096/16384两模式每activate冻结mask，分别只清逻辑表64/256KiB，16probe不变，成功activation计数回执。Main在现有PersistentEntry增加模式标签，切换撤销旧证明，旧fresh_name/全部读取验证/二pass/mask/完整payload不动。50模式冻结/重置/失效检查通过。实际observe_game每TickScope五次，3800额外字段5AB/BA cyclemedian0.922649，5/5更低；0/256字段cyclemedian1.021023/1.053398，满足预设小场景<=1.10上限但存在回退。12公开结果/原故障结局检查通过。Fixture扩到8192nodes/16384slots仅组件用途；无Game来源offset证书。

完整PGff89938c编译，32SourceValidation通过。原MetadataParse测试捕获两项固定容量夹具假设：碰撞target在Query前按4095计算，以及要求Mode0未启用的后12K槽也清零。将碰撞target移到已有Query内，零值断言覆盖全部逻辑活动槽，保留16碰撞/coldfallback及stale拒绝断言；0模式46320、1模式58608检查通过。测试失败与修正单独留档。精确匹配PDB的模式4B和两项8Bactivation计数绑定，11实际控制器函数的fakeprocess检查通过。Mode0仍用较大Workspace物理布局，是实验限制；未声称与老二进制逐条等价。

DR全新独立visible/lowest/0.5，无录像，startupconfirmed后90秒，模式0/1/1/0/0/1各10秒，FPS/CPU裁两端2秒，只数新增双眼。实际三对25.431→23.861、26.225→18.129、29.853→33.690FPS，1/3更高，中位比0.938275；SceneCPU每body配对中位0.924960、cycles0.931317，均2/3更低。模式1真实activation全部命中，模式0未走large。

第二候选段body163/acquired168=0.970238未达0.98健康阈值，hands163等于body、9个分段均有正更新、hardreject0，未证明具体原因；健康用完整10秒而FPS/CPU裁窗，不能把这些计数归为逐帧原因。宿主systembusy39.17%..71.66%，前段有cover-1117-r397-hands-route游戏，后段无该已知peer，且负载变化大；不能断言差异由容量导致或由外部负载完全解释。最后一对更快不能单独代表稳定收益。采样sourceproof仍约439534B、14validation，容量并未消除该成本。

不推广容量候选，不原样重复。正常恢复私有mode、Native/入口，XR0/launcher0、无强退；3个去重PID/creation/image所属身份均退出，未操作其他会话。实际120FPS仍未达成，已交付组合保持。证据pg-name-capacity/main-runtime-review.json、main-source-review.json及main-test-fixture-failures.json。

后续只做有界设计：observe_liveness无显式Scope时会丢弃已有同域proof并完整扫描；评估仅借用现有显式Lancer/known-unheld调用者已建立的同域完整proof，在单次栈上Scope中仍做全部Game字节两pass新读。必须遵守adapter无分配/无引擎调用/无重入，先核对owner与view再解引用、无foreignobserver/Capture、当前Lancer或unheld资格、域/名称/软件/时间/撤销等义务，不创建新seed、不缓存bool/动作许可；完整payload路径不变。此方向尚未实现或证明安全/收益，不能把设计当优化交付。复用唯一worker风险审阅，证据main-next-design-review.json。


### 已有seed的跨回调布尔Scope候选实际未命中，2026-11-17

从接受PG6a独立副本实现：仅mode1、无activeScope、softwarecurrent、无foreignsemanticobserver/Capture、当前modelunheld或原Lancerpredicate、已有view与owner一致且valid同域时借用单次栈Scope；不得在adapter创建存储。所有旧Proof两pass/NativeNames/domain/software/changed重观察/unreadable撤销和客户端100ms不变。唯一worker首稿变量误当函数/缺namespace及写错return文件，执行前退回，修正脚本只改两处runtime文件。已有Lancerpredicate真实实现未改；follow-enabled旧offline fixture新增falseeligibility spy以链接旧geometry路由测试。

自有内存测试首轮54455检查，后补错误lengthbound上下文及真实PAGE_NOACCESS/PAGE_GUARD一次性失败，最终56014检查零失败；本轮实际6888witnessbytes逐字节原full与新路径结论一致，158个fault range均首失败即撤销；cold无seed/warm有效seed的adapter无新增C++operatornew。包括域/观察器/foreignCapture/nested/GT禁用/Modelheld非Lancer等。0/256额外字段、每tick显式seed+五次scope外回调，5AB/BA cycles中位0.058321/0.173372，均5/5低；是人工图组件，不是实际Game时序。完整PG6b106d8b编译，32SourceValidation和61HandClientOffline、11控制器检查通过。

DS全新visible/lowest/0.5，无录像，startupconfirmed后90秒，0/1/1/0/0/1各10秒，裁两端2秒只数新增双眼。FPS24.876→24.238、20.505→22.845、22.017→18.414，但新callback_scope_reused六段全0。原boolcallback每body约14.3；已采样显式Scope也约14validation。不能从未执行的新分支解释FPS或CPU差异，不能据此宣称有效或算法负收益。六段身体双手健康、持续更新；systembusy67.7%..82.3%，已知peer名字为空不代表全宿主隔离。私有mode恢复、Native/入口正常恢复，XR0/launcher0、无强退；3个去重所属身份确认退出。不推广、不原样重跑。实际120FPS未达成、交付保持。证据pg-seeded-callback-scope/main-runtime-review.json。

静态调用链核对：copy_frame_on_gt/snapshot_on_gt需要fulladapter.observe返回完整Game/poseconsumer比较，不能替换为bool。observe_hold_source主要在真实held/charging/magazine/starter/execution路径，不应再假定它是站立热点。下一项先确认Fullobserver调用数及原始main_hand_held为false的频率；再评估仅无持握fullobservation的metadata解析计划，所有原Game输入字节仍两pass鲜读，busy word从成功最后一遍readbuffer取，Modheld/binding/switch/animation每次现算；kind仅是已鲜读完整输入验证的类型派生结果。现有DSbodymount drained witness queries2161、model_held0/binding_known0，支持检查该方向但不是完整callsite归因。需原完整查询成功闭合后建计划、hook前可失败预分配、不偷foreignCapture/observer，held/geometry/contact/action路径保持。这个新方案未实现、输入覆盖/closing时序/生命周期/收益均未证明；先actualhit计数再谈FPS。证据main-next-design-review.json；唯一worker提供有限风险审阅。


### DT 实际完整观察调用与实时标志组件（2026-11-17）

计数诊断 e747d 已完成一次30秒无录像运行，startup_confirmed=true。稳态装备frame129→513共384次：完整observe完成1915次（4.98698/次装备更新），全部原返回known且unheld，无失败；liveness完成5451次（14.19531/次），source和hold专用入口均0。liveness输出不含真实持握语义，不能用其unheld计数解释角色。只确认实际调用频率，不是排他CPU时间或FPS提升。Native/入口恢复、XR退出0，3项唯一自有进程身份已退出，无强制终止。证据 pg-full-observer-counts/main-runtime-review.json。

唯一DeepSeek worker108生成了独立fresh flag辅助头和新增validate_observed接口；主会话review纠正reset返回值及完整地址区间溢出。辅助头11737项、观测校验8659项检查0失败，包含全部模型字节变更/每次读失败/实际NOACCESS/第二遍改变/跨块字段拒绝。删除两个新增重载可逐字恢复原header，两遍读取、掩码、顺序和旧API保持。代码仅在pg-fresh-flag-words私有组件目录；尚未接入完整观察、未构建游戏候选、未测FPS收益、未替换手动交付。后续需绑定原typed字段与完整4B witness、每轮reset且只在完整same后消费，重新计算当前Mod持握/忙碌/动画状态并保留失效及原持握几何路径。


### DU/DV 完整观察解析计划与名称提示副作用（2026-11-17）

完整adapter observe私有接入先通过46412项实际记录字节/故障范围/保护页/状态边界检查、Source32/Client61和完整构建（5bca60d）。实际Game分支ON命中88.7..100%，但来源布尔Scope的记录达1048574B，10个ON剖面全部byte_capacity/disabled，OFF约437790B且14次校验。完整观察跳过了原元数据解析提供的Mod-owned名称长度提示，造成另一路径容量退化；没有推广。

唯一worker113提供verified NativeName hint replay。仅在完整两遍proof same及软件/身份/域/key/名称上下文/未持握/完整flagword全部检查后，补回已验证名称长度的解析提示；所有Game实时读取仍执行，无GameState/Frame/输入/姿态/权限缓存。冷上下文本地3800字段组合复现旧版5/5容量失败（1048548B），修正版0失败（419864B），恢复69次实时验证；最初用预热上下文未复现，已改为与实际故障一致的冷上下文。修正版47872安全检查、32source、61client、完整构建c33fa1e通过。

DU/DV准备时遗漏新instance.json的sim_refresh_hz；尽管runtime目录名含120且manifest选择正确，入口仍默认30Hz，实际运行读数均约30。这两轮仅保留调用/容量/CPU证据，不作为120Hz FPS对照或推广依据。DV实际Game容量恢复435218B、14..16次验证，六段身体双手均健康；CPU中位比约0.939，但新增双眼帧受30Hz限制、末组较差，未推广。

DW使用全新实例perf120-dw-full-unheld-hints-120-ab-1117，同一c33fa1e DLL，显式instance.sim_refresh_hz=120、调用既有environment核实GEARSVR_SIM_REFRESH_HZ=120；控制器开测前必须确认实际运行读数110..130Hz。已观察119.991Hz通过前置确认，三组0/1、1/0、0/1对照进行中；只用新增双眼帧计FPS。无录像、没有修改全局默认或其他会话实例。证据目录pg-full-unheld-plan、pg-full-unheld-hints、pg-full-unheld-hints-120。


DW完成：实际运行频率前置119.991Hz，全部running样本110..130内。三组实际新增双眼FPS36.817→36.678、35.635→34.435、35.893→33.351（0/3更高）；SceneCPU中位比1.02981、周期1.03065。六段身体/双手均健康；ON命中91.66..99.62%，OFF0，hint replay与hit逐段相等、读取失败0。记录容量退化已修复，但完整观察proof方案总成本仍未改善，明确拒绝推广及原样重跑。Native/入口恢复、XR退出0、三项自有身份已退出，无强杀；手动交付及公开版未替换。下一步回到已接受Nativeb262/PG6a，对Native source_loop_exact/current_actor布局证明依赖做结构性审查；已有排序/批量/source-runs等失败结果保留，不再次微调同一路径。完整证据pg-full-unheld-hints-120/main-runtime-review.json。


### Native 字段布局证明结构审查（2026-11-17）

确认现有prepare_base/prepare_instigator已保留偏移，稳态费用主要是每次current_actor的前后来源链与对象验证；简单再缓存偏移不会解决。复用唯一worker生成有界IDA脚本，主会话修正nearoperand已经是绝对地址、range跨界、间接跳转、add_func布尔返回及原unwind顺序断言；只在独立292MB库副本分析新选中的lazyclassgetter两个构造callee0x500ba0/0x50a6c8（29/27条指令）。两函数实际字节SHA均与历史RX镜像相符、原DB SHA未变。前者分配560B并传548/2049等参数；后者写对象+0x60/+0x134/+0x50，有区间外跳转及间接虚调用。均不是已认证Pawn/Actor字段getter，548不得当作类型大小/字段偏移。两个只读指针RVA0x1582d8a/0x13654f0不在旧38anchors内，原打包磁盘数据分别缺失/不可读为文字，类型身份仍未知。不得据此删除长来源链或调用构造/引擎函数。本轮无Game、无运行源码修改、无FPS收益，120目标未完成；下一项仅识别这两个精确只读常量，保留此前getter名字/字符串扫描失败记录。证据native-class-layout-construction/main-review.json。


DX精确只读常量完成：已接受Nativeb262/PG6a，全新实例显式120Hz，实际启动确认；仅两处128B各读两遍，共512B，PAGE_READONLY/MEM_IMAGE/分配基址/进程身份/原EXE SHA前后匹配。0x1582d8a=Actor、0x13654f0=Engine，定位原类工厂注册信息。读取器23项纯guard检查通过；Native/入口恢复、XR退出0、3项自有身份退出，无录制/点位/输入读取或引擎函数调用。

注册回调后续静态分析：0x25f7c4为65条指令的类元数据构造路径，0x49cc20为6条指令（非空参数则写vptr=0x141346ff0及DWORD+0x80=0），0xc3f20为ret0。两个小回调均无.pdata覆盖，不把前一函数范围套上；修正IDA conditional nearoperand分支追踪后，前者存在jz，按有界assembly处理，不宣称直线完整函数。私有库叠加256B当前只读字节，原库SHA保持。尚没有WorldInfo/Mesh/Weapon字段getter或偏移证书，Native运行代码不改、不宣称FPS收益。下一证据点是Actor虚表只读函数地址及Native自己已选择字段偏移，通过PDB实际布局读取后寻找原getter机器码对应，禁止用548猜Actor字段偏移或调用构造函数。证据native-class-layout-construction/main-actor-review.json。


离线PDB布局工具已由原worker实现并经主会话修正/编译。独立编译器地址差值fixture的34项字段位置与DbgHelp类型结果全相等，数组长度4错误fixture被拒绝。实际b262 Native DLL/PDB精确匹配，base_proof/instigator_proof均位于可写非执行本模块节且完整符号范围受限；提取5个Property的offset/width/dimension及ready/source_count，不读取字段值。工具只SymLoadModuleEx离线调试元数据，未LoadLibrary输入DLL、不连接符号服务器。首次编译cvconst.h不在默认include，定位现有VS DIA SDK include后修复，无安装。下一次受控只读目标已能由PDB定位Native自有元数据，不猜结构偏移。证据native-class-layout-construction/main-pdb-layout-review.json。


### DY Actor虚表与Native选定元数据（2026-11-17）

已接受b262/6a全新独立实例，显式/实际120Hz；两整轮仅Actor虚表512B+Native PDB定位的82B布局元数据，共1188B/46次读取，前后相等。Native只读字段offset/width/dimension、ready/source_count，未读Proof来源数组/期望Game字节、玩家/武器字段值或姿态输入。当前base.ready1/sources3640：Pawn/PC.WorldInfo208宽8，Pawn.Weapon1084宽8，Pawn.Mesh1008宽8，Pawn.SpecialMove1139宽1；instigator.ready1/sources1027：Weapon.Instigator200宽8；所有维度1。Native/入口恢复、XR0、三项自有身份退出，无强杀。23项reader guard检查及17项getter模式/控制流/宽度测试通过。

64槽均指向Game RX，共48不同目标；严格RCX字段读取后ret模式0。进一步每目标至多256指令有界CFG检查，仅发现2个数值重合：call[rax+0xc8]/call[rax+0xd0]为虚表代码指针读取，不是Instigator/WorldInfo实例字段。已区分CF_USE内存读与control-target-read，并用已知mov加载/存储验证IDA标志；conditional nearbranch显式跟踪。47函数观察范围完结、1个部分/未知，不外推全Actor虚表或全游戏没有getter。原IDB SHA不变，运行代码及交付不改，无FPS收益声明。下一证据窗口是64槽之后的Actor方法，不再重读相同前64槽；必须有原字段消费及类型归属依据才替换长元数据来源链。证据native-actor-vtable-metadata/main-static-review.json。


### EA Actor后续方法原生字段读取（2026-11-17）

DZ准备脚本锚点断言失败导致manifest未生成，未启动Game，弃用该名；修正后用全新EA实例。仅新虚表槽64..191（RVA0x13471f0，1024B）+Native82B元数据，两轮2212B/46次读取相同；reader参数/slot归一化后AST等同DY，复用其23项guard检查。显式/实际120Hz，Native/入口恢复、XR0、三项自有身份退出，无强杀，无输入/姿态/游戏字段值采集。

83不同RX目标严格getter仍0，256指令/过程有界CFG发现19处数值位移操作，12过程有部分/未知。逐条排除栈槽、LEA、宽度不符、其他对象寄存器后，找到两处入口RCX字段读取：槽89入口0xae6b40先subrsp18，0xae6b44 movrax,[rcx+0xc8]；槽91入口0x12ec4c在RCX未改写/无调用的路径0x12ec58 movrdx,[rcx+0xd0]。宽8与Native选定Instigator200/WorldInfo208相符。完整观测到的函数分支范围内两者均完结，但虚表归属范围与当前RX字节尚未完整认证，不是可调用getter或布局权限。原DB SHA不变，未替换运行代码或宣称FPS提升。下一步绑定Actor singleton与Native选定字段的owner/id名称地址，再校验当前代码；暂不继续扩展虚表窗口。证据native-actor-vtable-tail/main-static-review.json。


### Native字段所属类的离线布局工具，2026-11-17

复用唯一DeepSeek worker扩展现有PDB工具，在独立native-property-owner-layout目录添加Property.owner.address、owner.name、id.name路径；所有偏移通过精确PDB成员推导。Main review补齐wchar_t基本类型检查及fields/_Elems直接容器范围，修正错误类型构建参数。Main实际执行独立编译地址fixture共64项全部相等；字段数组4、名称长度63、同宽非wchar_t三负例均exit11按具体原因拒绝。accepted Native b262与PDB e7e1精确匹配，成功提取真实元数据布局；其index0 owner.address相对Proof121024、owner.name121072、id.name120896，绝不能使用fixture偏移替代。

已核对历史RX两处字段读取指令，保存295B当前代码核验计划及Actor singleton RVA0x1c1fe68；尚未进行新的进程读取。字段所属类与当前RX绑定仍待核验，未颁发布局证书、未删来源/身份/生命周期检查、未改生产或交付、无新Game/FPS结果。下一步用扩展只读reader离线检查后，再进行全新自有实例的有界核验。证据native-property-owner-layout/main-pdb-layout-review.json与native-layout-authority-plan/main-offline-plan.json。120实际新双眼FPS目标仍未达成。


### EB字段所属类及当前代码绑定，2026-11-17

新自有实例perf120-eb-owner-binding-1117，既有多开visible/lowest/0.5、明确120Hz、nomoviestartup、无录像。唯一worker实现新只读reader，Main review修复manifest取值位置、读取类型到权限映射与固定代码/虚表span锁定；25离线检查通过。startup_confirmed=true后两pass共2366B/64reads完全一致，295B五段当前RX匹配历史，Actor/Engine两个RO名字匹配；Actor singleton0x18473e80与base index0/1 WorldInfo、instigator index0 Instigator的Native owner地址完全一致，名称Actor、偏移208/208/200、width8/dim1。slot89/91目标符合计划。此为当前实例正面绑定证据，不等于已实现替代校验或提高FPS；未读取Game字段值/堆/pose/input，未调用引擎API/写进程。

正常退出，Native/入口恢复，XR0，未强退，三个去重自有身份逐creation/image确认退出。其他会话实例未操作。下一步审阅原生布局替代发现链的语义，先在私有proof中精确区分搜索来源与必须来源并计量，保留现有检查，不原样重跑本次Game。证据native-owner-binding-run/main-runtime-review.json。120实际新双眼FPS仍未达成。


### 私有来源角色标记完成，2026-11-17

唯一DeepSeek worker在native-source-origin实现Proof平行search_roles/required数组，不改Source/Property布局；只标property children头及pre-match next，其他来源required。重复角色OR/required黏性，成功追加初始化复用槽，非法role/count提前拒绝。Main修复高位角色、无用成员、reset成本及夹具引用/位宽/读数断言；current/source_loop_exact逐字保持，begin_prepare仅换行不同。原accepted fixture逐字复制、showIncludes确认新inl。Main实际重跑3exe均exit0：原reader735检查0失败，来源组件79检查0失败，production直接loop sample/current/changed_rejected均1。小型fixture base29中mask3计3、mask31计7，instigator16中mask1计5，仅fixture shape非实际Game数量。

全部原读取仍执行，未建Native DLL/未跑Game/未改交付/无FPS收益声明。下一步私有Native实际来源计量与代码绑定替代方案。证据native-source-origin-review/main-review.json。120实际新双眼FPS仍未达成。


### 真实Native来源角色计量，2026-11-17

仅来源标记变化的1844文件私有副本编译Native42f02277/PDB7ce51181；111项编译选项与接受版完全相等。扩展PDB工具68偏移fixture全部相等，4095长度及两种2B元素负例exit11；真实新PDB精确匹配。reader首个EC在0字节时因错误比较私有DLL与build路径而失败；原结果/脚本保存native-origin-runtime/failed-ec-reader，正常停止。修复私有路径helper，正确本实例通过、build/otherinstance拒绝，18离线检查通过；全新ED visible/lowest/0.5/明确120Hz/nomoviestartup，无录像。

ED startup_confirmed=true，两pass16reads/32788B全部一致，仅Native-owned count/ready/role/required，无Game字段值/输入/pose/Source地址/bytes/引擎调用。Main从raw重新计算全部32mask统计相符：base3640来源中required36、仅两WorldInfo搜索角色2310（mask3）；所有5角色潜在3604，但后三字段尚无原生替代依据不得据此删除。Instigator1027中required13、搜索only1014（mask1）。当前已核验字段对应3324/4667约71.22%来源记录有替代潜力，不代表CPU/FPS改善，当前仍全部读取。

EC/ED均Native及入口恢复、XR0、正常stop无强退，逐creation/image核对各3个去重自有身份已退出。交付未改。下一步私有代码绑定资格与过滤读取实现，保留required/identity/生命周期/closing等义务，先离线故障及组件成本验证再实际Game AB。证据native-origin-runtime-ed/main-runtime-review.json。实际新双眼120FPS仍未达成。


### 原生代码资格过滤实现与有限离线行为验证，2026-11-17

在native-code-layout-filter私有副本实现默认mode0原路径；mode1仅ready base mask3/instigator mask1。每次开始与结束重读295B代码、26B名字、16B虚表指针、8B singleton共345B/10reads，校selected owner/name/outer/offset/width/dim；开始可读不符回原路径，首次不可读或closing变化/故障撤回不重试。过滤loop保持原顺序/宽度/SEH，只跳完整授权且非required的搜索来源，所有身份原循环保持。Main review修正错误全表拒绝、Instigator偏移矛盾、Property/Identity类型混淆、地址溢出及冗余标签全表扫描。worker连续文本写入失败后停止，Main窄接手测试与selected函数，未新建worker。

Main实际编译运行715检查0失败，4命中/343回退/4撤回：包含全部295代码字节、名字/虚表各字节变异回退，shared未授权/required仍读，selected不符/unknown/unready回原，真实NOACCESS/GUARD首次故障无retry及closing故障/变化。为本地32MiB代码区+合成Proof(identity_count0)集成，不能冒充Game。宏ON mode0及宏OFF原main各735检查0失败，cold5022/warm3957保持。仅专用TEST宏允许codebase测试seam；正常编译使用原globalbase。Main逆变换确认私有块外与origin基线忽略空白相等，295B常量逐字匹配已核验计划。

尚缺完整组件成本、额外4/8B未对齐/跨页及identity/selected失败复核、Native DLL编译与实际Game AB；未建DLL/未跑Game/未改交付/无FPS提升声明。证据native-filter-contract-review/main-implementation-review.json。120实际新双眼FPS仍未达成。


### 完整current组件：逐条标签扫描版本回退，2026-11-17

Main补充500项真实本地内存边界/身份/selected检查0失败：4/8B未对齐、跨页逐字节变异、最后可读宽度避开NOACCESS页、跨页NOACCESS/GUARD拒绝、保留原fixture identities的真实current命中与失效在witness前拒绝，以及两selected身份/owner变化回原路径。生产inl未改。

唯一worker生成组件benchmark及实际ED角色数组，连续两次输出语法错误后停止；Main窄修命名空间/输出括号、错误始终计量instigator proof、new/old比方向及每样本hits检查。两shape使用实际3640/1027 roles、合成地址/全部8B宽度/identity_count0，完整current包括两次fresh345B witness，mode0是真正direct原loop（未定义reader seam）。每样本1000调用，5组AB/BA交替；两shape均0/5更快。base wall中位1.513546、cycles1.476091（原2975ns/新4476.5ns中位）；instigator wall1.633852、cycles1.615989（原785.3ns/新1284.9ns中位）。未达预设<=0.97且4/5门槛。此为组件负面证据，不是Game FPS结论。

当前逐条标签扫描过滤实现关闭，不构建DLL/不跑Game/不推广。下一步在Native-owned Proof准备阶段或本地标签变更后生成原顺序保留索引，只缓存本地计划而非Game资格，每次仍前后fresh witness及全部保留来源读取；先重新完整组件验证。证据native-filter-component-cost/main-review.json。120实际新双眼FPS仍未达成。


### Native本地保留索引计划与首次实际A/B，2026-11-17

唯一worker执行器包装反复失败后仅交有限草稿；Main在独立native-layout-index-plan-main实现POD mutable keep index计划，不覆盖worker文件。仅缓存Native-owned原顺序索引，source成功追加/合并、begin_prepare、keys/config零重置及mask/count变化使失效或重建；每次Game资格两次345B witness、身份/selected及所有保留来源仍新鲜。sample_impl/current_actor_checked原mutex/GT边界保持。29索引检查+715行为+500边界/identity+宏ON mode0与OFF各735回归均0失败。新组件仅切include实现，actual role数组/时序/地址宽度假设同上轮：base5/5更快wall中位0.521055、cycles0.521989；instigator5/5更快wall0.230201、cycles0.236645，过预设门槛。仍为合成地址/全8B/identity0，不外推FPS。

私有Native0914b39d/PDB88c8624a编译，原选项加GEARS_PRIVATE_NATIVE_LAYOUT_FILTER=1，未启用TEST seam，默认mode0。本地PDB精确绑定模式与命中/回退/撤回counter；11controller fake-process守卫通过。EF新自有visible/lowest/0.5/明确120Hz/nomoviestartup，无录像，同Game三组0/1、1/0、0/1各10秒（两端trim2秒）：39.737→40.981、37.213→38.672、21.868→33.622实际新双眼FPS。3/3更快，配对中位1.039222；CPU/身体帧3/3下降中位0.912588，cycles0.937574。Mode1命中423450/413864/346762，fallback/withdraw均0，mode0命中0；六段身体/双手健康，旧身体owner优化保留。

第三段原路径明显掉速，不能把平均14.63%当稳定因果收益；严格旧10%FPS+10%CPU推广门槛未通过。全部原始数据保留，不推广交付。正常stop、模式恢复0、Native/入口恢复、XR0、线程句柄关闭，三个去重自有身份creation/image已消失。下一步因基线不稳定进行一次有界反序复核，同二进制与配置，不扩大输入或玩法；不是原样无理由重复。证据native-index-runtime/main-final-review.json。实际120FPS仍未达成。


### Native索引路径反向复核，2026-11-17

EG全新visible/lowest/0.5/明确120Hz/nomoviestartup，无录像，Native0914b39d与PG6a79保持。controller仅改name/阶段顺序，AST逆变换等于EF，guard复用；顺序1/0、0/1、1/0，其余25s预热/6x10s/trim各2s和严格旧推广标准保持。实际新双眼FPS原→候选：40.073→42.112（+5.09%）、39.139→41.867（+6.97%）、37.622→40.393（+7.37%）。第三组peer列表从空变hod-single-task47-20261117，独立保留，不用来确认稳定peer收益，不操作该实例。三组CPU/身体帧均下降中位0.932582，cycles0.929128；六段身体/双手健康，mode1命中437104/426866/424505且fallback/withdraw均0，mode0命中0。

两轮共12阶段健康，5个peer列表稳定配对均更快（包含EF第三基线异常，未隐藏）。方向重复支持小幅收益线索，但严格10%FPS/10%CPU推广门槛未达到，不能声称稳定提升幅度或14.6%因果收益；不改交付，不原样重复NativeAB。正常stop/模式恢复/Native入口恢复/XR0/线程句柄关闭，3个去重自有身份确认退出。下一步只读定位更大PGprefix发现链来源；唯一worker已复用派发窄分析，无新worker/无代码修改。证据native-index-reverse-runtime/main-combined-review.json。实际120FPS未达成。


### PG prefix读来源源码核对，2026-11-17

唯一worker两次读取包装失败停止，Main核对实际源码。observe_game仍包含controller WorldInfo，Instigator取决于f.weapon.address而非VR held；ControllerBindingSpec分组Pawn/WorldInfo/bDeleteMe，Weapon分组Instigator/bDeleteMe；resolve对全部未匹配selector遍历children/super/raw identity+name，current再读选中property/value。不能用Native3字段资格直接授权全部分组查找或全部布尔位。PGcopy已有SEH memcpy，非逐值RPM；不重做旧copy优化。下一步先检查既有Native hand registration/current查询API与boolean-prefix契约，判断能否共享新鲜校验或需要有界来源分类，不直接新增ABI或跳过完整payload路径。证据pg-prefix-origin-audit/main-review.json；本项无Game/代码改动。


### PG/Native现有查询契约核对，2026-11-17

Main核对weapon_hand_participant.h的Frame/Registration、weapon_hand_client.h的ClientAdapter/source_observation_on_gt，以及Native rig_tracking_live.read。source_current_on_gt是PG本地adapter入口，不是Native等价查询；RigTracking导出只校cookie/session/core projection并复制Frame，不覆盖PG类包/属性链/6布尔字段及closing。ClientAdapter明确禁止观察回调进入Native/engine/分配/等待，所以本次不增加Native重入回调、不把cached Native Frame当新鲜属性证明。保留完整epoch/key/input freshness及原PG所有读。

下一步仅PG本地分类已发生的读取：默认required，group剩余仅WorldInfo或Instigator时标候选发现尾链，重叠读取以required优先计算union；先量化范围，不新增Game读取、不跳校验、不作FPS声明。证据pg-native-source-contract-audit/main-review.json。本轮无Game/代码改动，120FPS仍未达到。


### PG读取来源精确重叠统计，本地验证（2026-11-17）

唯一DeepSeek worker交付纯代码，Main修正64位重复重载、两个address0测试及失败输出全零语义。MSVC C++17 /W4 /WX通过，31针对性检查和10004独立字节oracle/边界检查零失败；其中10000随机范围使用独立80字节覆盖格，不复用事件扫描算法。required来源对重叠字节占优，否则必须授权全部消费者rolebits才能计入eligible。固定容量收集仅地址/长度/角色，不解引用；区间排序分析在回调外。当前仅本地工具，未接入生产、无Game运行、无FPS收益结论。下一步单次捕获原有读取来源，只标注独占WorldInfo/Instigator查找尾部，保持原验证与所有Game读取。证据：analysis/lancer/workers/reflection-contract-deepseek-01/pg-read-origin-accounting/main-review.json。


### PG来源诊断EH：实际报告不完整，不作比例结论（2026-11-17）

私有PG f3c8adcb编译通过，原WeaponSourceValidationTest32项通过，另10项capture生命周期与12项实际selector函数真值表通过。只新增2头文件、改3文件，原Game读取/原验证结果不变；独占可选查找尾部标记、选中字段/类结构/closing全部required。只捕获一次原qualification，无日志/分配/额外Game数据读取。

EH既有独立入口visible/lowest/0.5、无录像，实际启动确认，运行60.281秒，正常stop，Native与入口恢复、XR0、精确3身份消失。外部reader先后修正PID/HANDLE与modules返回dict误用，同一实例继续读取；state2/count bounds通过但collector complete=false，故拒绝计算eligible比例，没有重新启动。完整失败原因未记录，不能猜测是容量；原始ranges未取回。下一步补首个失败原因/attempt数，并在语义断言前保存有界tail，先定位收集失败再定向复查。本轮不是性能候选，不声称FPS收益或120Hz。证据pg-read-origin-accounting/main-runtime-review.json。


### PG来源完整捕获EJ：独占尾部仅1.56%，关闭该窄路线（2026-11-17）

EI新增首个failure/attempt标量后，实际报告count65536、attempt91802、failure5=capacity，原observe成功、role恢复正确，无conflict；正常清理。不是无效地址/角色失败。按实测将私有collector上限增为131072，不增加Game读取或跳过验证。新PG9c768cf2编译，31+10004+10+12+7及原32检查有效。EJ一次性完整捕获91806records（required89715，WorldInfo1052，Instigator1039），原observe成功、complete1/state2、failure0。3MiB报告跨多个同DLL可写非执行MEM_IMAGE region，修正外部reader逐region验证，在同实例取回，没有重启。

独立C++事件扫描和Python离线结果一致：union438982B，required432122B，mask0/1/2 eligible均0，mask3 eligible6860B=1.562706%。这是保守独占查找尾部的字节比例，不是CPU时间或FPS收益；role1/2原始读取重叠，不能简单相加。该证据不支持把Native侧71%可替代record比例套到PG，也不能据此跳过其余必需校验。关闭未改变的窄尾部诊断路线，继续更大的required-prefix重复校验成本，任何替代需独立权威证据，保持NoNativeEntry契约。

EJ确认真实双眼启动、60秒正常stop，Native/入口恢复/XR0及3精确身份消失；无录像/额外姿态输入采集/强退。默认Native b262与PG6a79哈希复核未变，未推广诊断，无FPS收益或120Hz完成声明。EH/EI/EJ三轮是工具失败定位与容量修正，不是重复性能AB。证据pg-read-origin-accounting/main-final-review.json与main-offline-actual-crosscheck.json。


### 实际PG读取拓扑本地复现：gap256更慢，关闭（2026-11-17）

复用EJ完整91806条原始range，只在owned arena填确定性数据，绝不解引用原Game地址。1321个原page映射为6713344B：页内offset/连续页邻接/跨页range/重叠保留，原缺页gap保留至少一整页，不保留大段原虚拟距离或物理cache/TLB拓扑。actual SourceProof record/begin/seal执行；两方案均30055unique/25670merged，Python独立规划形态吻合。原gap64：8068reads/pass、899274plannedB、460292padding；gap256：4784reads/pass、1487554plannedB、1048572padding，均未fallback。

唯一worker交回草稿后Main修正缺失操作符、错误原始距离mapping、record签名/begin、clock错误检查和paired median，MSVC /O2 /W4 /WX通过。5组AB/BA各100次完整validate，实际两遍回调总数核对通过。候选/原wall中位1.168752，cycles1.176355，0/5更快。读次数降低40.7%但字节量增65.4%，不构建PG、不启动Game、不改交付、不重复未变gap扫参。本组件的计数回调成本及合成数据限制保留，不将其当FPS。下一项审查更大mandatory状态前缀的原游戏compiled依据，不绕过NoNativeEntry/撤销/身份/新鲜度保护。证据pg-actual-topology-replay/main-review.json。120Hz仍未完成。


### 原游戏布尔VM引用机制：当前绑定已核对，具体字段待绑定（2026-11-17）

纠正简称，源码准确字段bDeleteMe／bSwitchingWeapons／bNoWeaponFiring。只读真实历史脚本分别确认WarPawn／Engine.Pawn使用对应属性。IDA连接器因ida_nexus缺失退出1，未修全局；复用独立idat和自有privateI64。AV旧VMentry带Mod JMP，不能作原始函数依据；仅在privateDB恢复旧911ab9捕获中hash431c7345的完整648B VM body分析，原Game不变。真实机器LEA确认GNative RVA1b86280，旧伪码1b85280不能直接作RVA。

EK接受b262／6a79／6481，既有visible/lowest/0.5入口，无录像，startupconfirmed后30秒观察。外部只读两次slot1/45及代码窗口：slot45->1d6ea4完整127B，slot1->1d8570前256B，均与旧hash绑定代码一致。实际汇编：BoolVariable清零R8后调用下一opcode，InstanceVariable从Frame.Code取QWORD property并Code+=8，读取property+94signed offset、owner+offset地址；其null-output支路在已捕获256B内完整。BoolVariable之后读property+D0DWORDmask并test目标DWORD。94/D0为Property描述符布局，不是任何Actor/Pawn旗标的硬编码偏移。仍未取得三个特定字段的当前Script绑定。

唯一worker交付局部token解析器，Main修正缺失比较符/截断/对齐边界fixture及WX警告，MSVC /O2 C++17 /W4 /WX通过1604checks0failure。仅显式2D01+QWORD十字节shape解析，无搜索/解引用/身份校验/许可，不接入生产。EK正常stop、Native/入口恢复、XR0及3精确身份退出。下一步只读具名UFunction Script并与原lookup选中Property的身份/owner/type/offset/mask比对，保持PG NoNativeEntry和全部撤销/新鲜度保护。无FPS收益或120Hz声明。证据pg-bool-compiled-authority/main-review.json。


### 三个实际Script字段引用匹配EL（2026-11-17）

复用Resolver发现候选、原cover_call_config._Capture双读/slot/name/type/owner/完整closing；只读具名Class/Function/Script/BoolProperty元数据，不读角色flag值/输入/pose，不调用游戏函数。worker代码Main修正比较符及Class.outer0绕过，真实共用Capture的21项边界/错误引用/closing变化检查通过。EL acceptedb262/6a79/6481，既有visible/lowest/0.5，无录像，真实stereo确认。0.485秒完成：WarPawn.ShouldLeftHandIKBeOn token3的引用恰等于WarPawn.bSwitchingWeapons，offset1196/mask512，Script163B；Pawn.StartFire token3恰等于Pawn.bNoWeaponFiring，offset600/mask2048，Script68B；WarPawn.GoreAroundTooLong token4（NOT0x81）恰等于Actor.bDeleteMe，offset172/mask8，Script21B。源checks8532/3069/11664均closing通过。值只作本次已退出进程历史布局，不允许硬编码角色字段。全部cases由当前VM代码与slot校验前后包围。

三份实际Script字节交原C++局部parser再验证，52项全部通过，包括截断、错位、改变对齐property引用；shape解析接受变后的指针，调用者必须另行比较预期选中property，不能把parser当授权。EL正常stop、Native/入口恢复、XR0及3精确身份消失。默认交付无改动，无FPS/120声明。还未覆盖实际Frame派生类遮蔽；候选必须以原Frame-derived完整lookup初选并匹配Script引用，保留身份/撤销/新鲜度/NoNativeEntry/不可读不重试。下一步实施私有boolean-source候选，计划见pg-bool-script-links/main-candidate-plan.json；不是已实现候选。证据main-review.json。


### Script布尔绑定与VM代码见证实现编译（2026-11-17）

在acceptedPG6a独立副本perf-pg-script-bool-binding-20261117新增binding/code/set三个inl，仅原weapon_hand_client.inl加入include；其余逐文件SHA与parent相同，无删除。worker首版混淆U32slot/Identity与Field.next，未应用；返工后Main修正owner初值恒拒绝、Actor属性owner与WarPawn函数owner混淆、API签名/命名空间头文件/位移及signedoffset边界。新helpers使用真实meta::Reader，原Frame-derived选中BoolProperty与Script精确ref一致，当前描述符mask/offset保持读取；Script关闭复查、VM114/127/256B及两slot见证均为只读，无Native/engine调用。组合验证四actor删除字段同一Actor属性及三条Script引用，不输出任何权限。

575项真实Reader/原两遍Proof组合检查通过，含513逐字节code/slot变动、Script/offset/mask/活flag源变化、派生类成功与遮蔽拒绝、不可读一次即停/observer指针不变。完整PG编译7b0a2827通过；原SourceValidation32与HandClient61回归通过。默认observe_game尚未调用这些helpers，无source过滤/无运行优化/无Game。不是FPS提升或120完成，未推广。下一块是原record ordinal固定role ledger与已捕获expected bytes的保守投影，仍须保留必需重叠字节、完整首次资格、原失效撤销、双遍fresh和mode变更失效。计划main-projection-plan.json；证据pg-script-bool-binding/main-review.json。


### 2026-11-17：读取角色采集与必需字节投影组件离线验证

复用唯一DeepSeek worker实现，Main修正API/偏移/排他终点及大型栈对象问题。固定容量ledger记录成功读取的原始ordinal，必需角色覆盖优先；投影逐ordinal匹配原raw记录，从已捕获compact复制精确期望字节，检查连续offset、范围缺口和完整覆盖。任何失败使目标失效，不留下部分可用proof；原对象alias直接拒绝且保持原记录。成功record hook仅在私有编译宏开启时存在，同值重复与新读取均采集；fault路径不变，嵌套保守失效、TLS/role恢复。四个实际MSVC /O2 /W4 /WX检查终止0：ledger133136、真实SourceProof投影4119、hook13、串联4097，总141365项0失败。包含逐字节独立oracle、全部64角色允许mask、必需重叠、缺口、损坏元数据、两遍新读、变化拒绝及读失败单次退出。只实现组件，尚未给真实查询分配角色、没有过滤运行校验、没有新DLL/游戏运行/FPS收益，不改现有手动候选。下一步接入私有bool-binding源、保留所有selected/closing必需数据及完整路径回退，再编译和验证。证据 pg-source-role-projection/main-review.json。


### 2026-11-17：PG bool／Script 精简校验实际EM对照，无稳定收益

私有perf-pg-role-projection源接入成功读取ordinal、discovery-only角色、当前Script／VM资格与结束检查、完整proof回退和模式切换失效，默认0。Main修正worker Field/runs/shift误稿、证书original重名警告并补模式中途切换拒绝。完整PG编译；27集成、原32/61回归通过。实际DLL84a759f35c38eb708f2a9a07693d77224620309035d364a5144707cab0ac310f。EM沿用b262 Native和6481 XR，visible／lowest／0.5既有独立入口，无录像；startup_confirmed=true。三组同局完整→精简实际新画面FPS：34.176→35.367、35.099→35.051、36.954→38.939；前两组同名peer，最后组peer退出，不能归因提速。Scene CPU/身体帧配对比中位0.9297、cycles0.9584，只有2/3下降。六段body/hands健康。三次资格／投影实际命中，原438207B→415067B（仅5.28%），每遍8063→7774次（仅3.58%）；人工样本约30%不可外推。无稳定收益、不推广、不原样复测。Native／entry恢复、XR0、mode0恢复、精确自有身份退出及Scene handle关闭均核验。下一步复用既有Actor singleton/getter代码见证给PG的WorldInfo／Instigator资格，解除与bool共享的查找范围，再先量实际合并eligible范围；不另做原样bool测试。证据 pg-source-role-projection/main-integration-runtime-summary.json。


### 2026-11-17：Actor与bool合并投影EN实测，三成字节减少仍无稳定FPS收益

复用Native已验证295B代码／26B字串／16B虚表槽／8B Actor singleton，经PG原Reader fresh witness，校验原WorldInfo／Instigator选中描述、Actor owner／Engine package／Core type／offset208／200／宽度／extent，保留实时值和closing。Main修正worker槽号误用、const写入和self-compare，常量逐字对照Native源；383聚焦／原32+61回归及完整PG编译通过。私有DLLc17668339d8dc557064b093b224ef755eceea5c5d8623dd9832a69b0961982eb。EN既有入口visible／lowest／0.5、无录像、实际双眼startup确认。先验覆盖门槛10%字节／5%读取通过：439802→308708B（29.8075%），8084→6139读取／遍（24.06%），然后三组同局完整→投影实际FPS39.938→41.135、40.528→40.240、40.337→40.531；各组peer名单均空。CPU／身体帧配对中位比1.0309，只有1/3下降，不能称有效提速。六段body/hands健康，mode恢复0、Native／entry恢复、XR0、精确自有身份退出和线程handle关闭均核验。不推广、不原样复测，不继续凭字节节省外推帧率。下一步只加查询调用／Scope旁路原因／实际full与projected validation计数，先量该快路在全部查询中的覆盖率，再决定是否处理完整hold/source查询。证据 pg-combined-actor-bool-projection/main-review.json。


### 2026-11-17：EO实际调用覆盖计数，旁路假设被排除

唯一DeepSeek worker提供23项调用层原子计数与插入点，Main接入private query-accounting源，只改client/scope并加一个头，不计rawread/draw、不改权限或校验算法。完整PG编译，12实际observer计数／原32+61回归通过。DLL0f3bf05eae77f9b73fb4a782deede5a8462e8ba18d8f28e2d1f3f02bd048e4a3。EO现有visible／lowest／0.5入口、无录像，实际startup确认；full/projected各20秒，裁2秒边界。稳定窗口634／644身体帧：完整candidate观察4.998／4.992次每帧；liveness验证14.185／14.171次每帧，分别全走full／projected proof；所有nested/nameobserver/noScope/disabled/frameMismatch/compare旁路0，capture/change/fault均0，hold_requests与显式source_requests均0，所有hands==body、hardRejected0。不能再把快路未覆盖归因于Scope旁路，也不把持握专用路径当站立热点。调用次数不是独占CPU比例。核对旧DW完整unheld full-witness/hints方案已经0/3更慢，禁止原样重做。下一步剩余PC.Pawn／Pawn.Controller／Mesh／Weapon的compiled Script字段引用资格，已找到4份saved UC线索（GetPawnSide、DetachFromController、GetWeaponHandPosition、ShouldLeftHandIKBeOn），尚无新的当前token/属性绑定证明，不可直接删校验。Native／entry恢复、XR0、mode0恢复、自有身份退出及线程handle关闭均核验。不推广／不宣称FPS提升或120。证据pg-query-path-accounting/main-review.json。


### 2026-11-17：EP四个剩余pointer的当前Script线索及有界prefix解析

accepted b262／6a79／6481保持，EP既有visible／lowest／0.5入口、30秒无录像，实际startup确认；只读metadata/code，未读actor字段值／输入／pose，无Game调用或写入。复用Capture原first-match派生到基类／注册表／类型／package／descriptor／finish fresh closing，采集四函数约0.985s。PC.Pawn由Engine.Controller声明，GetPawnSide Script80B token候选19；Pawn.Controller Engine.Pawn，DetachFromController247B候选6/22/56/66/93/105/234；Mesh Engine.Pawn，GetWeaponHandPosition54B候选1；Weapon Engine.Pawn，ShouldLeftHandIKBeOn163B候选34。观察offset548／560／1008／1084仅本次metadata，禁止硬编码。Main修正worker父类查找／运算符／Core metaclass包；44 helper检查及408真实Script有界prefix decoder检查通过，覆盖截断／opcode／每个reference bit／NOACCESS边界，parser仅语法。尚未实现Frame-derived C++选中字段绑定／shadow处理与primary角色，因此不构成布局授权／运行优化／FPS收益。Native／entry恢复、XR0及自有身份退出均核验。下一步primary4项link0/2/3/4的当前引用逐一等于原Frame查询选中property，再与现有Bool／Actor资格合并，后量真正必需见证规模；不原样复测DW/EN。证据pg-pointer-script-links/main-review.json及main-next-binding-plan.json。


### 2026-11-17：Primary4字段Frame绑定及EQ实际正向信号，尚未推广

私有primary-pointer源将当前Frame原链接0/2/3/4选中property与GetPawnSide／DetachFromController／GetWeaponHandPosition／ShouldLeftHandIKBeOn固定prefix引用相等绑定；按当前Frame继承链验证Engine.Controller／Pawn与函数owner、Core.Class／property type、dim／width／signedoffset／extent，所有原selected／live value／closing保留。4项与Bool/Actor全部成功才允许role32 primary discovery，默认0；派生shadow完整回退、fault无重试。Main修正worker detail命名、祖先Core.Class及Set原子成功输出；284实际Reader及原32/61回归、完整PG编译通过，DLLd8a4c8f48d35c5416ebd9d8bbad5a1cbf5a24e72efd434c656040bf78565fb50。首个实例名超40字符入口拒绝，未开Game；已核对并结束两个自有等待Python，保留失败证据，换全新短名EQ。EQ既有visible／lowest／0.5无录像入口、startup确认。实际443623→7509B（98.31%）、8072→201读／遍（97.51%）；三组原→投影实际FPS38.820→41.626、35.444→42.321、38.144→41.323，3/3更高，配对中位+8.34%；SceneCPU／帧中位0.89942、cycles0.91262，3/3下降；peer名单各组空，六段body/hands健康。尚无跨次稳定／动作／头显验收，不推广、不称120。新增重要问题：mode1每10秒出现41..54次再资格／capture，原因未明，不能先认定脚本缓存或actor flag。下一步只从原validator已读buffer和Scope原因做失败归因，定位实际变化见证后再缩小；保留正向候选，不原样重测EQ。Mode0／Native／entry恢复、XR0、自有身份退出和线程handle关闭均核验。证据pg-primary-pointer-projection/main-review.json。


### 2026-11-17：ES失效诊断完成，未复现EQ精简阶段频繁重新资格

原validator仅在已发生的mismatch上记录已有buffer的首个差异位置；不增加Game读取、不改变判定或故障重试。ER因Mod记录区跨多个保护region被外部collector拒绝，已保留工具失败及正常清理证据。修正为预先逐region确认全部范围属于同一精确Mod的已提交可读非执行image页，再作一次完整读取；8项范围准入检查通过。ES实际startup确认，正常结束，Native／entry恢复／XR0／精确身份退出／模式恢复与Scene handle关闭核验。

成功取回6条差异，全部mode0完整proof/cross，地址5398218552、batch8046、byte_offset8，无malformed。单次30秒mode1未捕获投影失效；因此不把之前EQ每10秒41..54次重新资格归因于Script缓存或name tail，也不称修复。诊断本身不作FPS提升声明，accepted生产／手动候选未改。下一项复用旧full-unheld typed状态与fresh-flag消费者，接入本次新primary Script资格形成的小proof，处理已测每帧约5次完整query；旧DW full-witness负收益方案不原样重复。唯一DeepSeek worker已接收有界实现任务。证据pg-primary-churn-diagnostic/main-review.json。120FPS未达到。


### 2026-11-17：新primary小proof接入full-unheld typed查询，编译／本地通过，待实际测量

复用唯一DeepSeek worker的prep与适配草稿；执行器写入中断后改纯代码交付，Main补client写入、include依赖顺序、原observed-validator重载、atomic模式及mixed-mode拒绝。新私有源perf-pg-full-primary-projection保留EQ boolean快路，复用旧full-unheld即时输出消费器；capture走新Script／Actor／primary资格和role63投影，当前两遍read buffer提供busy bits，Mod动画／持枪状态每次新查，selected proof显式覆盖两个flag DWORD。持枪／接触／候选走原路径，unsupported有效shadow保留完整proof。默认fullmode0，旧DW full-witness不是本次方案。

68981项聚焦检查零失败、234故障range／真实NOACCESS与GUARD／回调C++分配0，原32／61回归通过；45个新增Script元数据故障为原始接受、新资格读失败后严格拒绝，无输出或entry许可。初次测试误要求所有新增故障与原结果相同、及未打开生产name-bound宏的失败证据已保留并修正oracle／fixture。DLL 15a1ea68e3d7b37f701b03249e3ece57286a97e6bd4bceaf72a0691afaa70074编译通过。生产Native／手动候选／公共文件不变，无Game运行或FPS提升声明。下一步同局控制source projection1，切fullmode0／1，检查实际帧率／命中／CPU和正常清理。证据pg-full-primary-projection/main-review.json。120Hz未达到。


### 2026-11-17：ET full-primary实际同局对照：命中高但无稳定收益，定位重新资格热点

私有15a1ea68 DLL精确PDB绑定两个DWORD及30个Mod计数，existing独立入口全新perf120-et-full-primary-1117，visible／lowest／scale0.5／nomoviestartup／无录像；仅自己新实例采用已有120Hz runtime及hash，不改其他实例。实际双眼startup确认，source projection1全程固定，fullmode0/1三对10秒，各端裁2秒。实际游戏新帧41.447→39.386、45.498→44.621、45.062→47.283FPS，1/3提高，配对中位0.98072；SceneCPU／bodyframe中位1.03939、cycles1.01861，均只有1/3降低。六段身体／双手健康；peer名单空，系统进程快照未见其他Game／编译器／未解析identity，系统busy仍有30%..45%变动，不宣称完全控制负载。

候选真实命中91.99%／95.43%／95.70%，probe12447B／200reads；hintreplay等于hits，但每10秒qualifications157／103／99、changed88／58／56、projected qualifications126／77／79，且unreadable0。不能再称快路没用到；高命中和小proof没有转成稳定FPS收益，也不能把重新资格原因猜成name tail或Script cache。正常stop／Native和entry恢复／XR0／两个mode恢复0／精确自有identity退出／Scenehandle关闭全核验。

不推广、不原样重跑ET；保留原EQ正向候选和accepted交付。下一项将既有失败buffer归因用于本次full query，并记录原短路reuse条件和证书Bool／Actor／primary／closing阶段；不增加Game读取或改权限。唯一DeepSeek worker已收到有界draft任务。证据pg-full-primary-projection/main-runtime-review.json及main-cleanup-review.json。120FPS未达到。


### 2026-11-17：EU full-query精确失效归因：同一名称尾部与Native提示缺失

新增仅失败buffer注释、原短路reuse原因及证书阶段诊断，普通／observed校验两套两遍trace/result/fault/padding与父版本对比12611590项0失败；完整payload72230项、ring4项及原32／61回归通过。逆移除后原validator/rolehook及证书源码完全恢复。完整DLL bb80d6056248680bace65d03188d3cd40374cf039b89a90561803c294e67e199编译通过。worker旧接口／虚构anchor草稿拒绝，Main按实际父源完成有界修正，原草稿保留。

EU全新自有visible／lowest／scale0.5／已有明确120Hz runtime／nomoviestartup／无录像，startup实际双眼确认。总体220 changed、82 reuse_names；固定ring保存28条，全部projected=1／kinds2 name-tail／malformed0，同一index2968、entry391876896、实际长度4，NUL含义到10B，变化在文字起始后140／141／144／152／156B；只是本进程历史定位，不硬编码名称index或地址，不把抽样当全部变化。没有Script／flag样本。30秒内full6170calls／5790hits／380qualifications／209changed／288projected qualifications，unreadable0。成功raw后的Bool／Actor／primary／closing失败阶段计数均0，所以不能将未projected的92次直接解释为绑定失败；其他raw失败或未完成分支仍未分类。

正常stop／Native及entry恢复／XR0／两mode恢复／精确自有身份退出／Scene handle关闭全核验。无FPS提升声明、不推广；ET原候选hash复核未变。下一项复用旧pg-name-witness已测试的ColdRead/Capture事务组件，接入新的full typed-query＋primary小proof；物理读取地址／长度／顺序／首故障保持，只登记首NUL及之前语义bytes，所有别的必需重叠记录继续保留。旧大proof裁尾路线曾无稳定收益，不原样重测。NativeNames提示规则暂不放宽。唯一DeepSeek worker已收到有界prep草稿任务。证据pg-full-churn-diagnostic/main-review.json。120FPS仍未达到。


### 2026-11-17：EV full-query＋semantic witness正向实测，重新资格1／0／1

原持久DeepSeek worker返回Kaku403余额用完，未另建替代worker；Main复用既有reviewed-helper字节及Capture事务，不重新实现。新私有源perf-pg-full-semantic-witness从ET复制，只在fullmode1资格内激活精确Capture／NameCapture／Frame／mode／unheld上下文；cold物理读取地址／长度／顺序／首故障不变，仅proof.record登记首NUL之前含NUL的字节。直接record走原source-role hook，其他必需重叠记录保留。SourceScope／NativeNames提示规则／ledger／projection／共享Client逐字未改。

完整typed32141项0失败、32个tail变动保持hot查询，旧helper55及physical trace／NUL／必需alias／真实NOACCESS-GUARD42项通过，原32／61回归通过；DLL2b71252779d7fd838695024534642ae5c85a57ab02d3f2b6a3ae640cec2097d4编译成功。EV全新自有visible／lowest／scale0.5／已有明确120Hz runtime／nomoviestartup／无录像，实际双眼startup确认。source projection1固定，full0/1三对10秒各端裁2秒；实际新画面FPS42.319→46.180、41.002→47.046、39.470→44.521，3/3提高，配对中位+12.7979%；SceneCPU／身体帧中位0.88467、cycles0.87348，3/3降低。六段身体双手健康，peer空、系统进程快照无其他Game／编译器／未解析身份；宿主负载仍非完全控制。

候选三个10秒区间分别2304／2372／2243calls、2303／2372／2242hits；重新资格1／0／1，changed0、unreadable0，hintreplay等于hits。probe6009B／201reads；没有为了通过而改NativeNames检查。这个测试区间内原重复建立问题消失；不外推所有场景。跨ET/EU差异只是不同进程描述，FPS收益依据EV同局对照；旧大proof裁尾的混合结果不重解释为成功。正常stop／Native与entry恢复／XR0／两个mode恢复0／精确自有身份退出／Scene handle关闭全部核验。

支持当前固定站立模拟器场景的正向收益，不等于跨启动稳定／移动战斗／头显验收或120FPS，正式手动候选未替换。保留这一正向私有版本；下一步在source1/full1固定的新优化路径测剩余Scene CPU位置，不能继续套用旧52%PG／34%Native指令位置比例。Kaku余额限制不阻断Main继续复用实现。证据pg-full-semantic-witness/main-runtime-review.json和main-cleanup-review.json。


### 2026-11-17：EW当前优化路径采样，布尔Scope仍332次捕获

保持EV2b712527 PG／accepted b262 Native，精确PDB/加载SHA匹配。自有新visible／lowest／scale0.5／明确120Hz／nomoviestartup／无录像，实际双眼startup确认；source1/full1固定，Native Scene／body owner／PGgt同TID，进程及线程创建身份绑定既有RIP-only sampler。20.00048秒997样本、线程CPU18.640625秒；采样总暂停22.6007ms、max0.4166ms并正常恢复，无stack/pose/input读取。Native435、PG307、Game100、ntdll140、其他15；仅指令落点，不是调用栈或独占CPU百分比。Native source_loop_exact169落点，主要209..212行，range()仅地址bounds；PG event排序19、Proof.record7及复制/名称仍有落点，ICF类型标签不能作输入类型证明。

45秒内full10604calls／10601hits／3qualifications／changed0，boolean source332attempts／324selected；45秒counts不等于20秒采样频率，不猜每次失效字节。身体双手健康，实际采样期FPS记入review作为扰动诊断，不作提帧声明。正常stop／Native和entry恢复／XR0／两个mode恢复／精确自有身份退出／Scene handle及sampler恢复全核验。下一项保留EV正向路径，给boolean projection资格复用相同semantic witness，新增默认0独立开关与持久模式失效，先验证别名/fault/Reader，再只切新开关测量；Native来源循环随后处理，不原样重跑旧5%过滤方案。Kaku余额403不新建替代worker，Main继续。证据pg-optimized-scene-profile/main-review.json。120FPS未达到。


### 2026-11-17：EX布尔语义见证独立开关实测，当前50..52FPS

新private3a312294 DLL仅改source Scope文件，独立atomic source_semantic_witness_mode默认0；Scope及PersistentEntry冻结有效模式，构造选proof之前撤销旧模式、validation/capture中途变化拒绝。只在root projection1资格内激活既有semantic Scope，全部原物理读取／角色hook／名称hint规则／完整typed查询不变。最终源码逆变换精确恢复EV父源。62定向Reader／32tail／完整physical trace／模式／fault一次／nested检查0失败，完整typed32144、原32／61回归通过；旧helper55与physical oracle42组件未变复用。初次并行测试共享log导致full命令拒绝，修正独立log后通过，失败证据保留；不影响Game。冷路armed计数补充后完整增量编译，最终PDB/SHA绑定。

EX全新自有visible／lowest／scale0.5／明确120Hz／nomoviestartup／无录像，实际双眼startup确认。full1/sourceprojection1两臂固定，只切新boolean0/1。三组同Game10秒裁各端2秒，实际新帧47.317→50.182、46.408→52.427、47.368→51.118FPS，3/3提高、配对中位+7.916%；SceneCPU/身体帧中位0.92731，但仅2/3降低，首组1.03379增加；cycles3/3降低中位0.92483。保留正向线索，不改严格稳定收益判定／不称跨次稳定。六段body/hands健康，peer空，已知其他Game／compiler／unresolved快照无记录。

模式0 source attempts56／128／122，mode1 attempts1／0／0、selected1／0／1；计数逐项读取，边界可能跨第一snapshot，不把attempt0/selected1解释为没有资格。armed路径probe1并实际选用，全程full查询hits持续>100，每段full资格≤1。三个mode均正常恢复0，Native／entry恢复、XR0、精确身份退出及Scene handle关闭核验；未录像、未动其他会话。

正式手动候选仍保留，EV父版和EX新私有版并存；没有把两次不同进程收益相加或外推120。下一项Native source_loop_exact来源／消费契约，当前EW169指令落点已有依据，先核对既有layout过滤和剩余pointer/flag查找能否复用已验证编译引用，不原样重做旧5%Native过滤或删实时值／身份／freshness。Kaku余额403仍由Main复用实现，不另建worker。证据pg-dual-semantic-witness/main-runtime-review.json及main-cleanup-review.json。120FPS未达到。


### 2026-11-17：EY Native剩余三字段当前Script引用与联合角色拓扑

不改Native/PG源码，使用b262 Native及EX3a312294 PG／v3 XR既有入口，全新自有visible／lowest／scale0.5／明确120Hz／nomoviestartup／30秒无录像，startup实际双眼确认。只读Class/Function/Property/Script与已验证VM代码和slot，不读actor值/input/pose，无Game调用或写入。复用原Capture完整类型/包/层级/slot/descriptor/结束复查，0.704秒读完：Mesh Engine.Pawn，GetWeaponHandPosition54B，EX1引用token1；Weapon Engine.Pawn，ShouldLeftHandIKBeOn163B，token34；SpecialMove WarfareGame.WarPawn，ByteProperty宽1，IsDoingASpecialMove29B，token4。观察offset1008/1084/1139只是本进程metadata，禁止硬编码。三组closing11364／11355／8539通过，VM代码/表前后包围。

SpecialMove局部shape049b383a01＋8B字段ref＋383a240016；新parser复用原Mesh/Weapon解码，仅新增18B有界SpecialMove前缀，解析不授予权限。34 helper和719真实Script截断/操作码/每reference-bit/零ref/超限/实际NOACCESS页尾检查0失败。尚无actual Native p.fields派生选中绑定或新的Native DLL/过滤/FPS收益。

独立重算保存的全部32mask历史拓扑一致：base3640sources、required36；mask3可选2310，mask7仅2323，mask11仍2310，mask15为3024，mask31为3604；Instigator1027中mask1为1014。记录重叠使单个Weapon增加13、单个Mesh增加0，联合两者才能再释放714，SpecialMove联合还多580；不是CPU比例或过滤许可。下一项Native optional sidecar逐一绑定原p.fields[3/2/4]及当前owner/type/width/shape/Script/VM，成功后扩大联合mask；保留所有required/identities/live values，optional失败不污染original proof，fault不重试、unsupported保留旧路径。原Actor mask3/1依据保留，不原样重跑旧5%试验。

EY正常stop／Native和entry恢复／XR0／精确自有身份退出均核验；正式候选未改，最新性能仍EX约50..52FPS，不能用本次metadata捕获宣称提帧。证据native-remaining-script-links/main-review.json与main-native-binding-plan.json。120FPS未达到。


### Native三条Script绑定原型：完整本地耗时回退，2026-11-17

上一轮参考会话回复仅复核既有结论，没有新增FPS收益。本轮在私有perf-native-script-layout-20261117实现Mesh/Weapon/SpecialMove精确Native描述符、WarPawn函数Script、当前VM代码/派发表和开闭验证；未硬编码三者actor字段偏移，保留原required来源/原fallback/实时值路径。7982项针对性检查及735项原回归通过，含全Script/VM字节变化、原current()接入、全部required来源、故障不重读及生命周期失效。

五组AB/BA完整current()本地组件对照，实际角色分布3640条缩为36条，但cycle中位比2.145236509（5/5更慢），wall中位比2.13129342。模型是本地连续地址、统一8B宽度、合成注册表与实际角色分布，不是Game帧率。新增逐Script开闭身份重复读取成本抵消精简来源收益，拒绝此实现，不做完整DLL构建/游戏启动/默认推广。保留代码与精确脚本证据，下一步先消除同一开闭阶段的共享身份重复观测并重测完整组件成本，不能削弱freshness、required或故障撤销。最新实际新双眼FPS仍引用EX约50–52；120目标未达。证据native-script-layout/main-review.json。


### Native三条Script绑定原型：完整本地耗时回退，2026-11-17

上一轮参考会话回复仅复核既有结论，没有新增FPS收益。本轮在私有perf-native-script-layout-20261117实现Mesh/Weapon/SpecialMove精确Native描述符、WarPawn函数Script、当前VM代码/派发表和开闭验证；未硬编码三者actor字段偏移，保留原required来源/原fallback/实时值路径。8171项针对性检查及735项原回归通过，含全Script/VM字节变化、原current()接入、全部required来源、故障不重读及生命周期失效。

五组AB/BA完整current()本地组件对照，实际角色分布3640条缩为36条，但cycle中位比1.204490144（5/5更慢），wall中位比1.181069839。模型是本地连续地址、统一8B宽度、合成注册表与实际角色分布，不是Game帧率。第二版已合并共享身份，完整新验证成本仍抵消精简来源收益，拒绝此实现，不做完整DLL构建/游戏启动/默认推广。保留代码与精确脚本证据，下一步先消除同一开闭阶段的共享身份重复观测并重测完整组件成本，不能削弱freshness、required或故障撤销。随后第二版合并各Script共享身份，同一开阶段仅复用current()刚完成的原身份检查，闭阶段仍全部重新读取；增加每个去重身份变化的检查后8171/735全通过。cycle中位比约1.204（仍5/5更慢）；第一版约2.145证据另存v1子目录。当前拒绝推广，下一步检查同阶段注册表/队列公共头的批量闭合验证。最新实际新双眼FPS仍引用EX约50–52；120目标未达。证据native-script-layout/main-review.json。


### Native Script批量fresh身份验证候选编译，2026-11-17

继续前述未推广原型，合并同一开闭阶段共享身份，并将canonical对象/名称的两次新鲜观测放在同一注册表cohort内；仅开阶段复用原current()刚完成的身份检查，闭阶段全部重读。最终对象slot、名称entry指针与index在第二次文字读取后仍复查，非canonical走原identity。9318项针对性检查及735原回归通过，新增第二pass各身份成员变化、同文新entry替换、表count收缩及文字后slot/entry/index变化等。

完整current()五组AB/BA本地周期中位比0.912889864，5/5更快；wall中位比0.928231062。这是同一本地连续地址/统一8B/实际角色分布组件模型，不是Game FPS。私有Native fe324f0ee022c67dabb102915fdc1b26a6ba9de2658c52b5ea8572b006e1ea82 已编译，PDB匹配，默认mode0；已绑定EX PG3a312294，尚未启动Game、未推广。初次编译C1041 PDB访问失败保留日志；新私有构建目录/FS/单构建job完成。下一步现有独立实例入口做当前PG优化固定开启的Native0/1实际新双眼FPS对照，并确认mask31真实使用与正常恢复清理。120仍未达到，最新Game约50–52沿用EX。证据native-script-layout/main-batch-review.json。


### EZ Native脚本/批量身份实际对照，2026-11-17

独立入口新实例perf120-ez-native-script-1117，visible/lowest/scale0.5/nomoviestartup/无录像，startup_confirmed=true；Native fe324f0e，PG沿用EX3a312294且三个优化mode固定1，仅Native mode0/1切换。三组10秒各端裁2秒实际新增双眼FPS：48.0776→53.7378、49.5548→52.5193、52.0522→55.4103，3/3更高，中位比1.0645148859101705。SceneCPU/身体帧3/3降低，中位比0.9049063709593053；cycles3/3降低，中位比0.9392451793933663。不将不同运行收益相加，不称跨次稳定/120完成。

精确DLL/PDB绑定的Mod自有plan标量显示所有候选phase快照基础mask31/kept36/ready1，Instigator mask1/kept13/ready1；Native hits分别556305/546006/574980，fallback与withdrawal均0。六段身体/双手计数健康；PG三个mode各checkpoint保持1；同入口peer为空，但未增加全局GPU/进程采样，不能排除全部外部负载。现有固定未持武器站立场景，不是移动、射击、换弹、长期或实体头显验收。

Native和三个PG开关全部恢复0、Scene句柄关闭、stop0、Native/入口恢复、XR0及精确自有进程退出均已核验。没有推广默认或改其他会话。下一项检查Native current()/binding每个实际新画面的高重复调用来源，先定位Native/PG请求消费边界，保留实时字段/身份/freshness/ownership，不原样重复这组对照。证据native-script-layout/main-runtime-review.json及main-cleanup-review.json。120未达到。


### FA最新Native+EX组合指令采样，2026-11-17

全新perf120-fa-native-profile-1117复用已验证fe324f0e Native+3a312294 PG，Native和三个PG模式固定1，visible/lowest/scale0.5/无录像。实际startup双眼确认；既有精确进程/Scene线程身份RIP sampler20.0001119秒993样本，线程CPU18.15625秒，总暂停22.8004ms/max0.1519ms，正常恢复。Native377、PG293、Game127、ntdll176，其余20；Native script_identities_current64（其中第172行26）、memcpy61、memcmp50、identity18、memset14。仅指令落点，不是调用栈或独占耗时；PG同宽模板ICF标签不作为输入类型证明。

Native/三个PGmode恢复、stop/Native/入口/XR0、精确自有进程退出、Scene句柄关闭及sampler恢复全部核验；诊断FPS留在review，不作为新收益。最新性能仍引用EZ三组约52.5–55.4FPS。下一项先减少批量身份临时构造/无关name尾部复制，保留两次对象/名称观察、最后slot/entry/index和registry cohort，不删实时许可。证据native-optimized-profile/main-review.json。120仍未达到。


### FA当前优化采样与名称语义比较候选

FA20秒993个Scene指令样本：Native377、PG293、Game127、ntdll176，Native script_identities_current64为明显单点，memcpy61/memcmp50；不是独占CPU归因。四mode固定1，暂停累计22.8004ms/max0.1519ms且正常恢复；诊断FPS48.0146不作收益，完整正常清理见native-optimized-profile/main-review.json。

新私有perf-native-semantic-identity-20261117不构造临时Identity，比较原对象头有效字段，并仅读取/比较名称至已验证NUL的语义前缀；两pass、最终slot/entry/index、registry cohort保留。12419项检查含逐UTF16字位置与原identity一致、第二pass变化和NOACCESS边界，735原回归通过。Native33bd4c10编译完成/default0，未Game-run。相同测试EXE内复制父函数直接对照identity group，5/5更快，cycle中位比0.746866755；完整current另与原未过滤mode0比较，不能说是相对EZ增益。下一步独立新开关对照父版批量方法，再测实际新双眼FPS。120未达，最新无采样扰动FPS仍EZ52.5–55.4。


### 独立semantic开关就绪，2026-11-17

私有a2e69131将新名称/头部比较放在默认0独立native_identity_semantic_mode；0为精确旧batch函数，1为新方法，Native layout1固定。本地同二进制完整current()五组均更快，周期中位比0.829017211；非FPS。新路径12429、旧路径9318、原回归735检查通过，PDB/数据区/默认0已绑定。尚无Game运行或推广。下一项固定Native layout和三个PG优化仅切semantic，验证实际新增双眼FPS并恢复五个DWORD和自有实例清理。证据native-semantic-toggle/main-review.json。120未达。


### FB独立Native semantic实际对照，2026-11-17

新实例perf120-fb-semantic-toggle-1117，visible/lowest/scale0.5/nomoviestartup/无录像；实际双眼startup确认。Native a2e69131与PG EX3a312294，Native layout和三个PG优化固定1，仅新semantic0/1三对10秒裁边对照。实际新增双眼FPS及SceneCPU结果：{"pairs": [[41.12639105547485, 37.418527624725606], [42.786543922486864, 45.25096413956777], [43.609686456111795, 45.3173194569971]], "pairs_faster": 2, "FPS_ratio": 1.0391571951016856, "CPU_ratio": 0.9478994547257105, "cycles_ratio": 0.9609226681505779}。所有checkpoint固定mode正确及基础mask31/kept36/ready1；五个DWORD恢复，stop0/Native/入口恢复/XR0/自有身份退出及Scene句柄关闭核验。仅一次固定场景同局，非跨次稳定/实体头显/完整玩法验收，不相加跨次收益，不推断120完成。证据native-semantic-toggle/main-runtime-review.json及main-cleanup-review.json。


FB判定补充：2/3FPS、CPU、cycles降低，首组FPS约-9.02%、CPU约+11.27%，不能称稳定新增收益；不推广、不原样重跑。该次基线41–44低于EZ历史52–55，跨次条件未完全控制，原因未证实，不能将历史候选帧数当成本次实测或归因新代码。保留EZ正向候选，下一项审查事务逐unit validate/exchange重复current的真实放大及freshness/rollback契约，不能直接删检查。


### 事务Local整块比较私有实现，2026-11-17

确认validate与exchange两次current各有写入许可边界职责，未删除。改动仅把逐230个16B local比较重构为构造expected完整buffer并一次memcmp；全部实际读取、metadata/immutable双检查、shape/重复/before/ours/state/rollback条件及CAS许可保持。397980项与精确parent旧循环对照一致，含六种状态/三阶段/0至116单元选定形状/3680字节逐一差异和畸形输入；真实observed-backend-fixture通过NativeBackend441项，actual_read/CAS128/SEH，Host context限定。尚无完整后端耗时、DLL或Game收益，不推广。下一项相同fixture/Context次数的完整validate耗时对照。证据native-transaction-bulk/main-review.json。120未达到。


事务bulk完整validate耗时补充：0/1/13/75/115单元各五对AB/BA，实际相同捕获fixture、每arm20000次validate且HostContext检查次数严格一致；周期中位比分别0.95903/0.95122/0.99546/0.95580/1.02263，更快3/4/3/3/2组。13单元既有动作近乎无收益，115反而回退，不能宣称提速或从局部算法外推Game。关闭该实现的性能推进，不完整DLL/不Game/不推广。下一项现有运行时budget的context/owner_actor/metadata分项耗时与调用阶段，只读Mod自身计时/次数，不采集姿态输入；保留validate/exchange双边界。证据native-transaction-bulk/main-cost-review.json。120未达。


### FC现有运行时C0计时，2026-11-17

自有fresh perf120-fc-runtime-budget-1117 visible/lowest/scale0.5/nomoviestartup/无录像；不改DLL，使用正向EZ fe324f0e+EX3a312294。精确PDB类型定位现有Recorder.enabled 1B，在Native/PG四个mode固定1后开启，未新增pose/input诊断；32query/0noquery，排前4剩28条。每条context214次，中位4.0506ms；嵌套owner_actor122次1.95645ms；metadata_repeat214次0.17435ms；total阶段6.10965ms、commit2.1909ms、query1.03645ms。跨度嵌套不能相加，不是全部Mod/GPU/整帧时间。

初始两个plan快照mask0/notready，随后mask31/kept36；计时记录没有原子附带plan状态，不宣称逐条优化资格已证实。模式固定与计时开启回执有证据，首四条排除仍不是同步plan证明。正常stop/Native入口恢复/XR0/四mode恢复/自有identity退出/Scene句柄关闭核验，budget状态随正常Native卸载和进程退出清除。记录仅已有Mod计时及次数，不作FPS提速结论。下一项在每个current_actor内合并PC/Pawn/World/Mesh/Weapon的注册表cohort及双pass强身份观察，保留双possession/实时字段/slot/name/index和validate/exchange边界，不跨调用缓存。证据native-runtime-budget/main-review.json与main-cleanup-review.json。120未达。


### current_actor registry cohort私有实现，2026-11-17

默认0独立native_actor_cohort_mode，在每次既有check内共享registry起止cohort，Native identity各对象两次观察/slot/name/index/Default__全部保留，双possession/实时字段/validate-exchange边界未删；成功/失败恢复TLS。214针对性、735原Scene及9318Script回归通过。完整current_actor合成场景五对10000calls，周期中位比0.982414138、wall0.995337145，收益过小不推进完整DLL/Game，不宣称FPS。下一项capture_pawn_binding的本地same_bytes无diagnostic路径能否用memcmp保留逐字节精确语义，先测完整观察器，不弱化96B内容或读取/guard。证据native-actor-cohort/main-review.json。120未达。


### anchor本地精确memcmp尝试，2026-11-17

私有anchor_observer::capture_pawn_binding的same_bytes仅diagnostic=nullptr时用memcmp，比较字节/全部读取/guard不变，diagnostic仍原逐字节首差异定位。baseline及96B头每字节在第二次读取变化，296结果/精确读取轨迹检查和735原回归通过。完整capture无diagnostic五组cycle中位0.87073、5/5更低；diagnostic控制中位1.04887混合。完整current_actor两exe五对只有2/5更低，中位cycle1.00602、wall1.00156，未产生完整路径收益，关闭推广/完整DLL/Game。不得把capture微基准当FPS。下一项核对proofs_current里base/Instigator的身份集合重复，先证明重叠及合并新鲜校验边界，不跨调用缓存/不删来源或闭检查。证据native-anchor-compare/main-review.json。120未达。


### 成对proof只读身份bracket原型，2026-11-17

合成注册表base15/Instigator8，5相同，union18，不冒充实际Game身份统计。默认0私有prototype：union第一观察→两套原source_loop_exact只读验证→union第二观察+最终slot/name/index/registry关闭，最终前不返回权限；保留两轮possession/livefield及validate/exchange。186针对性+735Scene+9318Script通过，含每条source、每个ID、第二pass变化及冲突alias。完整current_actor五组10000calls均更快，周期中位比0.749877062，wall0.746312256。仅原始unfiltered mode0，layout1故意回原路，尚未接入EZ正向compiled-layout源校验；不能作为EZ新增收益或GameFPS。下一项在同一只读bracket内接入现有current_sources并保留全部Actor/Script/required和closing，再测真实mode1完整路径。未完整DLL/Game/推广，120未达。证据native-paired-proof/main-review.json。


### 成对身份校验接入compiled路径，2026-11-17

将current拆为身份检查及内部只读current_sources；默认0仍完整旧流程。pair1在合并身份首观察/末观察之间执行既有base+Instigator current_sources，Actor代码/literal/vtable/singleton和Script/VM开闭及必需sources不删，双possession/实时值/validate-exchange不删，不跨调用缓存。新增130项compiled集成检查含baseScript闭合后、Instigator代码读取时身份变化由最终union拒绝；186pair、735Scene、9318Script原回归通过。Native layout1固定仅pair0/1，合成完整current_actor五组周期中位0.841646、5/5更低，非GameFPS。完整Native编译19c7c716，精确PDB、独立默认0pair DWORD及layout DWORD绑定。没有Game/推广。下一项固定Native layout和PG三mode1仅切pair测实际新双眼FPS，恢复五个DWORD和自有进程正常清理。证据native-paired-compiled/main-review.json。120未达。


### FD独立paired identity实际对照，2026-11-17

新实例perf120-fd-paired-proof-1117，visible/lowest/scale0.5/nomoviestartup/无录像；实际双眼startup确认。Native19c7c716与PG EX3a312294，Native layout和三个PG优化固定1，仅新pair0/1三对10秒裁边对照。实际新增双眼FPS及SceneCPU结果：{"pairs": [[49.47141285467576, 55.59723244549269], [55.917917591277394, 56.12653356796613], [55.84283703233835, 55.41381401800463]], "pairs_faster": 2, "FPS_ratio": 1.0037307536774451, "CPU_ratio": 1.0054945054945055, "cycles_ratio": 0.9823877203568405}。所有checkpoint固定mode正确及基础mask31/kept36/ready1；五个DWORD恢复，stop0/Native/入口恢复/XR0/自有身份退出及Scene句柄关闭核验。仅一次固定场景同局，非跨次稳定/实体头显/完整玩法验收，不相加跨次收益，不推断120完成。证据native-semantic-toggle/main-runtime-review.json及main-cleanup-review.json。


FD判定补充：49.4714→55.5972、55.9179→56.1265、55.8428→55.4138FPS，2/3更高，中位比1.00373075；CPU中位1.0054945略增，cycles0.9823877。没有稳定新增收益。更关键：native_layout_filter_hits/mask31仅证明旧compiled路径，不直接证明新paired分支成功；后补scratch读取因symbol输出header被误识别而assert，未发生该RPM，修正时自有Game已正常结束。新pair覆盖证据缺失，不能声称新paired性能已有效验收。修正report candidate_route_used为null，单列compiled_layout_route_used；actual_pair_route_proven=false。不推广，不原样重跑；下一项添加仅Mod自有paired attempts/successes直接计数并离线验证，再新名字定向测量。五开关恢复/正常Native入口XR和精确身份清理仍有效。120未达。


### FE带直接计数paired identity实际对照，2026-11-17

新实例perf120-fe-paired-counted-1117，visible/lowest/scale0.5/nomoviestartup/无录像；实际双眼startup确认。Native counted candidate与PG EX3a312294，Native layout和三个PG优化固定1，仅新pair0/1三对10秒裁边对照。实际新增双眼FPS及SceneCPU结果：{"pairs": [[48.08465792822721, 53.721098747847066], [54.52755199590143, 54.34322824653512], [54.63512369719815, 55.41693900869501]], "pairs_faster": 2, "FPS_ratio": 1.014309756409263, "CPU_ratio": 0.9797055845210101, "cycles_ratio": 0.9851907370983066}。所有checkpoint固定mode正确及基础mask31/kept36/ready1；五个DWORD恢复，stop0/Native/入口恢复/XR0/自有身份退出及Scene句柄关闭核验。仅一次固定场景同局，非跨次稳定/实体头显/完整玩法验收，不相加跨次收益，不推断120完成。证据native-semantic-toggle/main-runtime-review.json及main-cleanup-review.json。


FE判定补充：e5ce6090仅增加Mod自有paired attempts/success/rejection/ineligible，27计数语义+130集成+735Scene+9318Script通过。新分支真实success263087/266428/269500，均等于attempt、rejection0；关闭段四计数均0变化，覆盖证据已补齐。实际新双眼FPS48.0847→53.7211、54.5276→54.3432、54.6351→55.4169，2/3更高，中位+1.431%；CPU3/3低、中位-2.029%，cycles3/3低、中位-1.481%。保留小幅CPU改善线索，不宣称稳定提帧/120，不推广或原样重复。五个开关恢复，stop/Native/入口/XR/自有身份/Scene句柄正常。下一项current_actor已有检查内分段计时区分proof/possession/身份/字段链接，原读取/检查顺序与双写入边界不变，只采Mod耗时次数。证据native-paired-counted/main-runtime-review.json。


### FG current_actor真实分项计时，2026-11-17

私有actor-timing默认0、最多4096次，只包原调用不增Game读写/输入姿态。61项开关结果/精确read轨迹/分组/上限检查+735Scene+9318Script+130集成通过。独立fresh perf120-fg-actor-timing-1117，visible/lowest/scale0.5/nomoviestartup/无录像；匹配PDB384B自有统计区，固定layout/PG/pair1再开计时。实际4096次：live2357、retained1573、storage166。live均次15.7178us，opening proofs6.0181us、closing5.7247us，合计74.71%；两次possession合计15.05%，十次实例identity合计7.42%。retained均次15.9808us，proofs合计76.00%。所有分项false_returns0；受QPC及bounded early窗口扰动，不是整帧/全部Mod独占CPU或FPS收益。

计时开关及五个性能开关全部恢复，Native/入口/XR0/自有identity退出/Scene句柄关闭核验。下一项proof内部重复构造union预期集合：仅原Mod身份数组/计数逐字节相同时复用预期plan，游戏实际观察和原source/编译witness/closing每次仍执行；任何mode0重建先失效，避免旧快照匹配却scratch已被别的模式重建。先加hit/rebuild证明与生命周期测试、完整current_actor成本对照。证据native-actor-timing/main-review.json及main-cleanup-review.json。120未达。


### 预期union构造缓存私有尝试，2026-11-17

默认0的native_paired_plan_mode仅在Mod原identity数组/count逐字节未变时复用expected union；不缓存Game观察，命中仍双pass/原sources/compiled/closing。mode0任何重建先失效旧snapshot，避免scratch被覆盖后旧snapshot误命中。208针对性+735Scene+9318Script+130集成通过，覆盖精确读取轨迹、Game实际变化、expected变化、count溢出/模式和TLS。固定layout1/pair1、仅planmode0/1完整current_actor五对，周期中位0.9726725、wall0.9851535，3/5更快，前两对更慢，不推进DLL/Game或推广。下一项Script闭身份与外paired闭身份重复，只有明确映射outer union的原身份才可能延后到mandatory outer close；Script-only与所有VM/Script/source保留，直接current()保持旧闭合。先验证中途变化和无提前许可。证据native-paired-plan-cache/main-review.json。120未达。


### FH Script结束原始身份复用外层强校验，2026-11-17

只在外层两pass原始身份union bracket内，将Script结束时完全映射至原始清单的身份交给外层第二pass；Script-only身份、VM、Script全部字节、required sources和Actor witnesses保留，direct current读轨迹不变。2591定向+735Scene+9318Script+130集成零失败；完整本地current_actor五组cycles均更低，中位0.875627418，不能外推Game收益。Native838743ff编译通过。新实例perf120-fh-paired-close-1117，现有入口visible/lowest/0.5/无录像，startup实际双眼确认；PG三个优化、Native layout/pair固定1，仅close0/1三对。实际结果：{"pairs": [[46.299264976316934, 52.071695251476775], [54.73909377300411, 55.82595031817402], [46.048639962389224, 52.749665910488595]], "pairs_faster": 3, "FPS_ratio": 1.1246764992513933, "CPU_ratio": 0.9121663313212608, "cycles_ratio": 0.9340798725418756}。所有段pair success>100且reject0，只有mode1有close计数；checkpoints基础mask31/36/ready1。六个模式恢复、stop0、Native/入口恢复、XR0和精确自有身份退出，线程句柄关闭。固定场景短时结果，不代表移动战斗或物理头显验收，不推广默认、不宣称120完成。证据native-paired-close/main-runtime-review.json和main-cleanup-review.json。


### FI 完整Script身份纳入外层union，2026-11-17

保留FH候选，只在Script三项资格完整且原始expected清单匹配时，外层两个fresh pass覆盖原始及Script独有身份，内层保留VM、Script全部字节、字段shape、required sources和Actor witnesses。首次资格/不完整状态用原路径；外层opening语义不匹配可以退回完整原路径，实际读取失败不重试，middle开始后失败不回退。单独current读取轨迹不变。定向Script、旧close、Scene、Script、集成检查均通过；完整本地路径五组cycles都更低，中位0.904902633。独立实例perf120-fi-paired-script-1117，visible/lowest/0.5/无录像；实际双眼startup确认，已有六项优化固定1，仅Script union0/1。实际结果：{"pairs": [[28.46944822068338, 31.740530683818985], [49.86692979981964, 46.58180616175771], [31.78790411043627, 47.36272223415135]], "pairs_faster": 2, "FPS_ratio": 1.1148979930267537, "CPU_ratio": 0.885946793420103, "cycles_ratio": 0.8508058392147277}。直接success/无fallback或reject和mask31/36/ready1均核验；七项模式恢复，正常stop、Native/入口恢复、XR0和所属身份退出。短时固定场景，不代表移动战斗/实体头显，不推广默认，不宣称120完成。证据native-paired-script/main-runtime-review.json和main-cleanup-review.json。

FI结论补充：只有2/3更快，一对退步，FPS28..50且Scene CPU随时间显著变化；不得仅凭medianFPS1.1149宣传收益。pair每身体commit约490..491、layout fallback/withdraw及Script fallback/reject均0，实际Script success161700/224399/230278。现有观测不能将原因归给其他会话或调度/缓存/页面换入之一。关闭推广，保留候选；下一步仅扩展现有只读采集器的host CPU/本进程页故障/working set记录，避免继续拿未知波动判断优化，不新加资源门槛、不干扰其他会话。


### FJ 原FI候选附加只读OS负载记录，2026-11-17

复用原Native a8802564/PG3a312294，不重编译、不改变功能，只在现有4Hz计数采样加入GetSystemTimes/GetProcessTimes/K32GetProcessMemoryInfo；复用已验证自有句柄，每次核对PID/创建时间/存活，无新权限、输入、录像或启动资源门槛。46项检查及采样器自进程smoke通过。新实例perf120-fj-host-observe-1117，visible/lowest/0.5，实际双眼startup确认，已有六项优化固定1，仅Script union0/1。实际结果：{"pairs": [[45.769057573912406, 48.169410143478565], [51.18433888286417, 49.074621518438725], [51.756609368782364, 51.827727651605834]], "pairs_faster": 2, "FPS_ratio": 1.0013740908396207, "CPU_ratio": 1.0172649360327908, "cycles_ratio": 1.0009664649069998}。七项模式恢复、正常stop、Native/入口恢复、XR0及自有身份退出核验。OS负载只作解释证据，PageFaultCount包括软缺页，不能当磁盘换页或卡顿时长；仍非移动战斗/头显验收或120达成。证据native-paired-script-host-observation/main-runtime-review.json及main-cleanup-review.json。

FJ最终review：三对45.769→48.169、51.184→49.075、51.757→51.828FPS；median1.001374，Scene CPU1.017265、cycles1.000966，无端到端收益证据。host全机16逻辑CPU忙碌52.54..62.32%，各段pagefault22..312（含软缺页），working set4.956..4.987GB；API有最大27.47ms墙钟延迟，可能含采样器调度，不能归给游戏暂停时长/CPU竞争/磁盘之一。FI完整Script union退出下一候选组合，不继续原样复测。实际本轮v1采样器已按hash留档；后续adapter复用旧baseline-stability WindowsMetrics，48检查与self-smoke通过，未另开Game验证adapter。下一步复用现有RIP-only sampler检查FH正向候选优化后的残余落点，不能沿用优化前Native/PG比例。


### FK FH正向候选优化后Scene指令落点，2026-11-17

新实例perf120-fk-close-profile-1117，复用Native838743ff/PG3a312294，六个mode固定1，visible/lowest/0.5/nomoviestartup/无录像，实际双眼startup确认；direct close uses>100及pair success/reject0、基础mask31/36/ready1均核验。既有RIP-only sampler20.000429秒984/984，线程CPU18.8125秒；累计暂停34.3844ms/max0.8811ms、restored/success/modulecoherent均true。Native339、PG370、Game113、ntdll143、其余19；仅指令落点，非独占CPU比例或全栈归因。Native Script identities56、memcmp43/memcpy40；PG copy_value4/8合计201，精确ICF符号不能作语义输入类型证明。采样期实际新双眼FPS54.02980357099241只作扰动诊断，不是优化收益。

正常stop/Native/入口恢复/XR0、精确自有进程退出、六mode恢复、Scene句柄关闭与sampler正常恢复全部核验。完整Script union与旧Native semantic候选继续不推广。下一步沿现有bounded caller-origin诊断定位当前PG4/8B读取的调用来源，保留实际读取/observed/unreadable/SEH，不凭copy_value函数名优化memcpy。证据native-paired-close-profile/main-review.json及main-sites-review.json。120仍未达到。


### FL/FM/FN/FO 当前PG固定4/8读取来源，2026-11-17

从EX3a冻结源码派生诊断PG，仅重命名原copy_value实现+noinline单次调用wrapper；原读取/observed/unreadable/SEH正文精确保留。复用既有caller Meter，容量扩512，GT每509次抽样，默认关闭；852counter/22812wrapper/19reader/22region检查通过，完整PG编译。Native保留FH838743ff；六项优化+诊断固定1，独立入口visible/lowest/0.5/无录像。

前FL/FM未得到可接受表，早先“更新太快”只是未经确认的推测，冻结后仍失败；FN记录read rejected，未保存原表。离线真实DLL验证两个问题：尾调用可能产生其他模块返回地址（一般性，不证明这批失败原因）；28KB表跨RW/COW image分区而旧守卫要求单区（确定的守卫缺陷）。只读guard改为逐区同image/commit/非执行RW/COW验证后一次RPM，最终FO采集成功，旧失败保留。四实例startup均确认、正常Native/入口/音频及XR清理、所属身份退出和模式/线程句柄恢复均核验main-all-runs-cleanup.json。

FO10.02579秒计数窗口：4B66074353/8B210579912/ordinal276654209，约2759万次每秒；分开读取差-56，未强行修正。最终冻结表606385次选中事件/113代码位置/overflow0，另8个在线coherent快照。所有FO记录均PG模块；名称name414953、通用field104732、registered_known57799、fresh_name14922。这是系统抽样事件分布，非独占CPU/无偏概率或完整每调用频率。name源码645/647/648/660为名称ID/表slot/entry index/前缀；field控制器96为反复链表遍历。诊断实际新双眼43.6557FPS，wrapper/计数可能扰动，不能作收益对照，120未达/不推广。下一步复用已有field/property caller诊断定位当前重复发起字段查找的真实消费者，不能套用旧AP请求比例，保留首匹配/派生类顺序/类型/当前性/收尾和engine invoke边界。证据pg-copy-callers/main-runtime-review.json。


### FP当前field/property请求方，2026-11-17

从EX3a派生诊断PGcccaaf8c，复用原reflection_caller_cost helper随机1/64计时，仅在field/property入口加noinline Scope，原CPP/controller正文逆变换精确；22组件检查/完整编译通过。NativeFH838743ff，六mode固定1，现有独立入口visible/lowest/0.5/nomoviestartup/无录像，startup实际双眼确认。优化暖机3秒后两次Mod原子计数快照间27.060943秒、body/hands1210，overflow0/epoch不变。field入口来自property约128.199次/body、抽样inclusive wall估计4.64546ms/body；property上游pointer32.4769/1.83891ms、deleted29.8554/.94957ms、idle inventory node typed36.0198/.40391ms、车辆flag8.0132/.21181ms、vehicle_controls2.0033/.51141ms，均为嵌套墙钟，不能求和或称独占CPU。诊断实际新双眼45.0952FPS，不作收益。

已正常stop、Native/entry/音频恢复、XR0/所有记录所属身份退出、六mode恢复和Scene句柄关闭。下一步车辆无动作仍查询InputIsDisabledCount/Pauser的早退，只减少无动作查询，保持按钮边沿/driver/shadow/entry bookkeeping及所有动作许可；随后审查库存元数据查询复用，不弱化freshness/first-match/identity。证据pg-field-property-callers/main-runtime-review.json。120未达/不推广诊断版。


### FR 车辆无动作许可查询早退实际对照，2026-11-17

EX3a私有源码仅vehicle_controls添加默认0可选gate：按钮/driver/shadow/seat bookkeeping之后，walking无use_edge或非walking无leave_edge即返回，不再查InputIsDisabledCount/Pauser；实际动作所有原检查保留。65536状态组合/589828检查零失败，暂停按住恢复再松开重按序列一致，原动作/状态相同且只减少这两类读取。带skips计数PG4f42d6aa编译通过。先前FQ因主会话漏生成控制脚本未形成AB，已保留错误及正常清理记录；修复后FR全新实例visible/lowest/0.5/nomoviestartup/无录像、实际双眼startup确认，六优化固定1，仅新PG gate0/1三对。实际结果：{"pairs": [[45.67839133232378, 47.94146796337264], [51.55913018035384, 49.555033611248845], [51.382645489663126, 46.376536513869375]], "pairs_faster": 1, "FPS_ratio": 0.9611301322948106, "CPU_ratio": 1.0172790982238866, "cycles_ratio": 1.0393816621150724}。Mode1skips>100，mode0只允许极小边界差；Nativepair/close均活跃、metadata mask31/36/ready1；七mode恢复、stop/Native/entry/XR0和所属身份退出及句柄关闭。短时站立场景，不是车辆玩法/头显验收、不宣称120、不默认推广。证据pg-vehicle-idle-gate/main-runtime-review.json和main-cleanup-review.json。

FR判定：45.678→47.941、51.559→49.555、51.383→46.377FPS，仅1/3更快，不能宣称稳定收益或确定回退；第三对其他登记实例boom-video09-1117-184714由存在变为不存在，不作严格可比AB，其余两对也有未测外部负载。分支skips实际命中、Body/Hands健康，实际动作和车辆体验未测试。FQ先前因Main漏生成controller而无AB，正常cleanup，失败另存main-initial-run-failure.json，未伪称FQ测量通过。FR全部七mode/Native/entry/音频/XR和所属身份正常清理核验。下一步比对生产vcxproj发现HAND_TAIL_TRANSLATION_TRIAL未定义，库存现有two-field batching分支未编入；仅拟复用对应query helper的私有前缀感知batch路径，不启用其他手骨功能，不做全局裸指针缓存。


### 战役测量阻塞与加载崩溃，2026-11-18

用户要求优先测战役，比较是否仅Playground低帧。FH固定站立Playground实际双眼新帧52–56 FPS仍是当前正向结果，不能外推战役/移动战斗。FT/FV曾到玩家与相机，但没有合格双眼提交；FV定位战役最终画面调用者224baf，私有候选4eee已编译并通过1184个过滤检查，尚未实际启用并完成新双眼验收。后续FW/FX在加载、Native正式激活之前崩溃，不能给出战役FPS。

本轮只读已有OS崩溃文件，确认Game+285f91与+3dcd13均为空哈希桶基址读取；缓存头未保存在文件中，尚不证明内存不足、磁盘缓存损坏、竞态或Mod破坏。自制StackWalk64缺少当前映像/unwind页，结果弃用，不当成真实调用栈。启动几何修复源码只在精确启动四边形与自有swapchain资格下改局部参数，177e90在此仅字节核验；该关联不能证明它写坏图形缓存。

独立GA实例只改已有PARALLELRENDERING=0启动参数，确认parallel0但async prepass及rendering thread仍1；菜单初始化超时、动作0、Native未激活、双眼未确认，没有性能结果。正常关闭失败，PG shutdown5未ack；Native prime入口与音频恢复后，仅强制结束身份核验的自有Game75148，明确不算正常清理通过。最终status未确认启动且存活身份不匹配。保留失败，不继续原样重启。下一项是核对后台战役菜单的确认输入/就绪路径，键盘消息与玩家发现前Native gamepad路径的差异目前只是源码假设。证据campaign-serial-c91f/main-campaign-loading-review.json。120未达。


### 战役菜单已推进；两项具体缺陷修复，2026-11-18

私有模拟器布尔输入漏掉XR动作weapon_a/b/x/y，A命令虽ack却不会变成对应XR按钮。独立副本09c5只改runtime.cpp两行，48实际解析函数用例通过、8旧版失效案例复现并修复。GD在120Hz/lowest/scale0.5/visible/无录像下，经原MenuReader及最多5动作到Continue，稳定玩家/相机与Native running已观察；非FPS验收。Native4eee战役caller仍attested0：先前请求4553字节，原helper固定4096容量，hash前即拒绝。新Native4d99按4096+457连续两段完整核验，所有字节仍须匹配，helper限制不变；实际BCrypt10回归、完整编译、PDB及父featureflags一致性核验通过。

GE用新Native及按钮修复模拟器，启动前断言runtime hash、120Hz和0.5；5动作后加载在Native正式激活前退出。OS ApplicationError确认首个异常Game+285f91/c0000005，继发ntdll+7e907/c000000d；两段代码核验尚未live验证，战役FPS仍未知。GE自有Game/XR/launcher均退出，音频恢复，无强制终止；异常退出不是正常游戏验收。GD Native/入口/PG恢复，但XR退出1且无双眼提交，不能称整轮通过。

Main两次准备错误分别保留：GB漏私有runtime.json用默认可见模拟器，0动作，正常关闭失败后只强制精确自有Game；GC漏显式120Hz用默认30Hz，仅1次A菜单尝试、无FPS。均不作性能比较；后续runner增加hash/scale/120Hz断言。下一步复用既有稳定玩家后才启用PG的路径，结合已验证A导航；旧延后实验使用键盘停在标题，不能作新组合已测证明。证据campaign-final-chunks-1118/main-delivery-review.json。Playground短站立52–56FPS保留，120未达。


### GF 首次有效战役测量，2026-11-18

复用既有延后PG启动分支，并要求配合已修模拟器A确认；原stable_player连续两次同pawn/controller/camera/level且timestamp推进后只启用一次实际生产PG。离线生命周期检查覆盖缺失/stale/换相机/一次启用/失败不重试；默认入口不变。新实例perf120-gf-cam-defer-1118-b2f5使用Native4d99/PG3a/Sim09c5，visible/lowest/0.5/120Hz/nomoviestartup/无录像。5个原菜单确认47.484秒进入加载，稳定玩家后PG已启用，Native两段4096+457字节校验在实际Game均match1；startup_confirmed=true且实际双眼输出确认。此单次启动成功不能证明早启PG就是间歇加载崩溃原因。

六项优化全部固定1，40个checkpoints均正确，base metadata mask31/kept36/ready1。40秒阶段两端各裁2秒，实际35.1878845秒接收1423独立Game双眼新帧，copies同为1423，40.44005544 FPS。不是XR循环数字；不代表移动/战斗/完整战役/实体头显验收，也不是与早先PG地图的严格AB。整阶段身体/双手1624相等，Source attempts/acquired1625，paired成功795947/reject0，约490.115次paired checks/身体更新。对应CPU采样区间36.0617173秒线程CPU30328.125ms，1462身体/双手更新，约20.7443ms/身体更新、0.841个核心；是Game+Native+PG合计而非独占Mod成本。

六项mode恢复、Scene线程句柄关闭、stop/Native/入口/PG/虚拟手柄门/音频恢复及所属Game/XR/launcher退出均核验，XR退出0。战役同样低帧，不能再假设仅PG地图低帧；仍未区分共享Mod与战役自身开销。下一项集中评估只读current_actor_checked中两套proofs_current的完整括号合并：两次sources与两次actor读取保留，结束前校验完整身份，不跨函数/更新缓存动作权限。不以跳过写入前授权换帧。证据campaign-deferred-pad-gf-1118/main-runtime-review.json。120未达。


### GG 只读actor完整括号合并未证实整帧收益，2026-11-18

从已验证战役Native4d99派生独立720b候选，只改weapon_hand_scene_binding.inl/native_script_layout.inl。默认0；只在当前只读current_actor调用中，将两套成对身份阶段合并为一个前后完整括号，保留两次sources/VM阶段、两次actor读取、原mutex/ready/epoch和所有fallback，返回true前必须结束收尾，不跨调用或更新缓存许可。关闭时1050次读取的精确序列hash与父版本一致；新路径884次。6547actor检查、9318Script、2591旧close、735Scene、旧actor及130集成通过；完整actor五对cycles中位0.849488、wall0.853968，仅局部收益。原存储backend用例首次遗漏bcrypt链接、随后漏必需C0 fixture参数，未称通过；实际storage reader0/1精确读轨迹已验证相同。初版测试错误假设所有可选Script失配必须拒绝，现已对照父版本验证相同完整原source fallback，保留原失败日志。

独立战役GG复用已验证A输入和稳定玩家后启PG，120Hz/visible/lowest/0.5/无录像，实际双眼startup确认；六优化固定1，只切新actor mode0/1三组10秒。实际新双眼FPS35.603→42.664、40.816→29.011、38.282→34.769，仅1/3更高。线程CPU/身体更新比0.9284、1.6144、0.9642，不能宣称整帧收益或把最好的单对当结论。新分支成功97813/73252/87191，reject均0；paired checks/身体更新约490→256，功能状态与base mask31/kept36/ready1始终正确。全部7mode恢复、Scene句柄关闭、Native/入口/PG/虚拟手柄门/音频及所属Game/XR/launcher正常退出，XR0。

候选保留默认0，不推广、不继续原样AB。下一步复用既有RIP-only采样器及OS负载记录查看战役当前CPU落点；旧Playground采样不能当战役独占成本。SystemSettings实际Idle/Inactive energySaving为False，DefaultSettings同名True不能当已启用节流证据；render_cost只覆盖最初有界ticket，不能直接将其QPC开销归于暖机后阶段。详见native-actor-bracket-1118/main-next-protocol.json。120未达。


### GH 战役RIP采样未到启动确认，2026-11-18

已复用既有20秒RIP-only采样器、匹配PDB及只读OS计数适配器，目标是4d99/PG3a/Sim09c5六模式固定1的战役，不含GG未证收益actor候选。新GH在原60秒菜单导航中0动作超时，只有基础Transient UIScene；PG仅预留且未启用，Native未正式激活，没有RIP/有效FPS/host interval结果。不能把所有启动失败归给PG早启。Native prime/PG预留入口/音频均恢复，正常窗口关闭未退出后，仅核验并强制精确自有Game93788；强制收据单独记录，framework forced=false不能覆盖这次外部清理事实。Game/XR/launcher身份均已退出，无同行进程操作。保留失败，不原样重启；此前GF40.44FPS结果仍有效，120未达。检查点已保存，当前工具无真实压缩入口，运行时压缩未执行。证据campaign-rip-gh-1118/main-runtime-failure-review.json。
