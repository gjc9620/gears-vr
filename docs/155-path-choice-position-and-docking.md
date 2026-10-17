# 岔路 UI 的实际位置与停靠关系

2026-10-18。`50cbfcd` 增加默认不读取的布局细节选项。本轮在冻结 Native70 的实际游戏中取得 `DeltaChoosePath / UIScene_PathChoice` 的完整13节点树及 Position、DockTargets 元数据和值，保留 `read_only=true`、`consistent_reread=true`、23,799项复读验证。它是顺序CPU采样，不是原子快照或GPU裁剪证明。

证据保存在 ignored `analysis/runtime/ui-layout-details7-20261018.json` 的 `path_layout_details`。导航只向自有 HWND 发送已确认绑定的鼠标消息；达到碰撞后停止移动，等待关卡脚本显示岔路菜单。测试结束正常关闭自有游戏，native cleanup、设置与四个profile恢复通过。未启用旧FCanvas平移实验或修改UI位置。

## 当前数据支持的下一步

- 场景下有 SafeRegionPanel 和另一个直接子 `UIImage`；10个已识别岔路内容节点均是 SafeRegionPanel 的直接子。因此移动整个场景会同时影响额外图片，移动SafePanel是范围更准确的候选。
- SafePanel四面的存储单位均为脚本枚举值4（PercentageViewport），无停靠目标。Top约0.0374、Bottom约0.9626，对应实际bounds y约40.392..1039.608。四面比例共同支持UI坐标域1920×1080，不能用Windows窗口的2534×1009替代。
- 10个内容节点的Y位置/高度存储单位均为5（PercentageOwner）。BottomRightBackground的X停靠左背景的Right；Y无停靠目标。该关系支持父级平移的实验假设，仍不证明实际布局及裁剪会随之移动。
- Right/Bottom的原始Value在不同单位下可能表示相对尺寸或端点；不能把所有节点的四个原始数一律当作绝对坐标。右箭头还有有意保留的负宽度，不能归一化后回写。

最小下一实验是在准确认证的游戏线程调用原生 `UIScreenObject.SetPosition`，仅对SafePanel的Top/Bottom作等量小位移，跨正常布局帧观察父/子bounds，再恢复；不在Render调用返回时立即恢复。已有历史实现支持InputType=0保留原存储单位并触发正常布局失效通知，可避免先猜测Getter的原点/单位转换。但调用前仍须认证当前完整函数、实际对象和相关虚函数，实验之后还要核对GPU裁剪及输入区域。

本轮没有调用GetPosition/SetPosition、写游戏内存或交付UI修复。正常双目、额外布局读取、父级平移后的视觉结果及真头显体验是不同验收项。

后续 `ui-layout-details8-20261018` 在当前进程核对了SetPosition完整356B、实际SafePanel两个虚表槽指向及其全部冷热chunks：`2988d8` 的696+114B和 `297338` 的92+47B，均与保存的历史完整函数匹配。新私有位置试验默认关闭，逐次重新认证全部五段，只调用原生SetPosition改变Top/Bottom各 `16.f/1080.f`，跨正常布局帧保留；后续在身份与自有数值仍匹配时通过同一接口恢复。试验仅验证布局传播路径，不是最终居中量。

审查后增加固定owner线程、部分恢复失败后的后续帧重试，以及停止前最多2秒的恢复等待；超时保留unresolved和pending记录，正在执行的回调始终阻止卸钩。核心、实际wrapper及双线程/恢复用例通过。Native77完整构建通过，原生UI bypass及GPU frame-link启动自测通过；Python配置认证13项测试通过。

首轮启动前发现缺少GraphicsMenuLockTest测试可执行文件，补建后再开始游戏；`path-position77b` 实际到达岔路场景，但布局采样因UI变化而拒绝，未写入试验配置或修改位置。正常清理、游戏退出与设置/profile恢复通过。采集器现仅对带明确SnapshotChanged原因的失败丢弃整个样本并有界重新采集，不放宽身份认证。父子bounds变化、GPU文字/裁剪与输入仍待实际验证，尚未交付岔路UI修复。

`path-position77e` 实际取得认证配置并调用原生接口，立即读回成功，`applied=1`；之后位置读取失败或数值不再匹配自有值，`ownership_lost=1`，未通过同一接口恢复，`unresolved=1`。当时同一个标志覆盖这两种情况，不能仅凭它认定游戏改写了坐标。没有取得位移后的GPU图片，不能证明子布局移动。试验未强行恢复，随后停止并正常关闭游戏；native cleanup、entry恢复、设置及四个profile恢复通过，自有前台采样为空。普通双目在该轮仍有1153次XR提交、572次scene copy；这不是位置修正通过。

历史虚表候选 `138a558+488 → 298bdc` 会计算位置并以return `298c46/298c7b` 调用SetPosition的Left/Top。尚未认证当前这个虚表槽或证明本轮执行了该函数。下一步保存首次失去所有权时的实际值，观察该panel的原生SetPosition来源，再决定在哪个正常布局入口调整；不在Render层反复覆盖。

新增一次性loss记录区分Top/Bottom可读mask、原始/自有/实际bits、units及scene/panel/world；只发布首条完整记录，worker通过release/acquire读取。缺读、变回原始、单face改变和禁止第二条覆盖用例通过；实际数据待采集。

Native80（`364800f`，SHA256 `986fbad49dabc61da5a79319e78f6df793ea9f62c384bc428685a033508a7723`）完整构建及实际wrapper/GPU frame-link启动自测通过。80首轮配置被拒绝、未apply；80b则由旧试验INI阻止，未执行位置API。已修正ignored驱动的会话配置生命周期：只清理由完整历史receipt和进程失效证明关联的旧配置，未知内容不覆盖；本轮配置等待正常退出和native/entry恢复验证后清理。

`path-position80c-20261018` 取得实际首次loss：`applied=1`，read mask=3且complete，Top/Bottom观察bits分别`1025061054/1064725749`，精确等于原始bits；自有位移bits为`1029037876/1064974300`。units可读且四项均4，world/current world均1、identity lost=0。因此这轮确认数值恢复到原值，已排除该记录中的读取失败；仍未定位具体writer，不能仅凭相等宣布298bdc执行。试验以proof_failure停止，没有位移后GPU图像或最终UI修复验收。下一步围绕SafePanel正常布局入口取得有界调用证据，避免在Render层反复覆盖。

该轮正常关闭自有游戏，native cleanup与entry恢复、完整设置/四profile恢复均通过，本轮自有配置在停止后完整比对并清理；无自有前台采样。原Top/Bottom约`0.0374/0.96260005`，自有值约`0.05221482/0.97741485`。历史`298bdc`完整161B读取已核对缓存SHA；其入口使用对象与12B输入，条件调用Left/Top SetPosition，尚不能命名为SafeRegion更新或证明它恢复Bottom。下一条证据需要当前完整161B与实际panel的vtable+488，以及精确对象调用内的位置前后变化。

只读`path-position80d-20261018`随后在实际PathChoice panel上双读vtable+488及目标完整161B：指针为当前base+298bdc，SHA256 `b5dae9f89c8182b26eef27e87ca2ae57804899ed313cb9acdc630280a03b6fb5`，身份、指针和字节复读一致。未写位置INI、未调用位置API；这只补齐当前静态认证，仍没有该函数实际执行/改写位置的证据。正常关闭、native/entry恢复和完整设置/四profile恢复均通过，无自有前台采样。下一步可围绕这一精确对象做有界入口/出口观察。

新增默认关闭的`GEARS_PATH_CHOICE_LAYOUT_OBSERVER`，仅在完整当前161B及全局虚表槽双读认证后观察该方法。它要求位置试验的固定owner线程、成功apply的同一nonce、精确panel/root/client/world及实际虚表槽；最多保存4条入口/出口快照，以release/acquire逐条发布，不新增跨original锁、不改参数或坐标。累计apply次数不认证后续未成功的新nonce。新wrapper、原位置core/live的`/W4 /WX`检查及18项Python检查通过，完整DLL及实际writer命中尚待验证。该观察器不是UI修复。

Native84从`228f82a`冻结（DLL SHA256 `baf522a98cabdfad9ef24b0ef748ca06e7cbb6523f517ca4c86b19bbd913f281`），完整构建、原位置core/live、布局wrapper、graphics锁及实际UI bypass/GPU frame-link启动自测通过。`path-layout84-20261018`实际到达岔路，位置apply一次后再次完整读到原值，ownership_lost=1并以proof_failure停止；观察器claimed=0。它只有通过门禁后的行数，没有各项bypass计数，因此零行不证明298bdc没有执行。没有取得位移修复验收；正常关闭、native/entry独立恢复及设置/四profile恢复通过，无自有前台采样。

随后仅查询KB并检查已存历史代码/rdata，定位更有针对性的路径：SafePanel虚表`+98 → d44918`在条件成立后调用`+528 → 2983f8`；后者读取对象内百分比数组与范围，计算四边并经`+2c8 → 298694 → SetPosition`重设全部四边。2983f8完整两chunks共1101B，SHA256 `9df4472db8e3ebaca59f6c14356c8de8c685a2aac1a1ff060ca0f7603d945d97`；298694完整240B，SHA256 `c455b118525aa5c957e5eea29cd8a46479aa2a95ef633693d4aa04da0e15ac28`。历史来源身份与完整证据保存在ignored报告。此静态链仍不能证明它是本轮实际writer，也不能把对称边距百分比当作平移接口。下一轮先认证当前完整代码及实际两槽，再有界观察目标panel调用前后四边，补齐bypass分类，暂不改百分比或持续覆写布局。

Native86从`7c387e9`冻结，DLL SHA256 `a0f3937432378d6e4de96236d5a3b7921cf83797c47e87986a10b5ec5a364c50`。完整DLL、四项wrapper/core/graphics检查及实际UI bypass/GPU frame-link自测通过。`path-layout86-20261018`在同一目标panel上捕获正常reflow调用：caller为当前base+2982a8，入口与出口身份和全部2047 mask一致，Top/Bottom从自有`0.052214816/0.977414846`变回原始`0.0374/0.962600052`，Left/Right保持`0.15305008/0.846949875`。数组Num/Max均2，raw索引0、selected值0.9252、units四项4保持；不把未经反射命名的raw偏移当已还原字段。全入口2次、承认1行，另一入口因外线程旁路。该实际样本已证明2983f8调用子树会重设目标位置，但未追踪具体store，也不证明所有UI症状有相同原因。

本轮仍因试验位置被覆盖而停止，未完成UI修正。正常关闭、native cleanup、所有entry完整独立恢复及设置/四profile恢复通过，无自有前台采样。精简proof和原始receipt保存在ignored `analysis/runtime/path-layout86-reflow-proof-20261018.json`。后续修正应顺应这条原生布局路径，先验证其下游SetPosition参数域，避免在Render阶段持续覆写或重复调用SetPosition造成布局失效循环。

后续已加入默认关闭的298694八参数只读观察，限定在同一已认证reflow/owner/nonce/panel/world作用域，最多四行，保留四个float原bits、inputType/origin/sort及原始RAX。它不hook或修改SetPosition的356B认证，不改坐标/百分比；旁路嵌套parent不会借用外层作用域。实际wrapper和原位置回归`/W4 /WX`、20项Python配置/恢复检查通过（含parent/child/双hook库存及完整chunks/两槽的39种恢复子用例）。完整DLL和实际实参采样尚待运行，不能把历史PixelViewport声明当作这轮已观察值。

## 2026-10-18：实际实参与原生重排修正候选

Native87未取得child实参：一次apply后认证被拒绝，parent/child均未承认记录。不能据此认定对象注册表数量变化或放宽认证。`7e5ce90`随后只增加首次认证失败的原始字节诊断，不改变接受条件；Native88本轮未重现该失败，`auth_failure_ready=0`。

Native88从`7e5ce90`冻结，DLL SHA256 `1e4ee00345426cb683715fbf56e21d5d20910953f57b5a8fabe665b1b299f60e`。实际`path-layout88-20261018`在同一已认证panel/nonce/world中捕获298694正常调用，caller为base+2985a5，八实参为Left293.8561401、Top40.3919983、Right1626.1437988、Bottom1039.6080322、inputType1、origin0、sort0；保留了float原bits及原始RAX。float加法所得宽/高为1920/1080，支持UI视口坐标域，不能替换为Windows窗口尺寸。child入口为自有位移值，正常返回后再次精确恢复原始百分比坐标，X和全部元数据保持。该轮仍以ownership loss停止，没有交付位置修复；正常退出、native/entry恢复、设置及四profile恢复通过，无自有前台采样。精简原始证据为ignored `analysis/runtime/path-layout88-child-proof-20261018.json`。

`c4ed52b`新增默认关闭的`GEARS_PATH_CHOICE_REFLOW_CONTROL`：仅在上述完整认证和活跃位置租约下，对原生child的Top/Bottom实参各加`(Top+Bottom)*(16.f/1080.f)`，原调用仍只执行一次，其他实参保持。控制作用域独立于四条诊断配额；首先验证原输入归一化后仍是已知安全区，且入口两Y精确等于当前owned。原生单位换算会产生1ULP差异，因此仅本次已修改、正常返回、完整元数据/X稳定的调用可在预期值2ULP以内保存实际精确bits为owned；后续holding/restore仍严格逐bit比较。失败、异常或身份不明时停止私有实验，不猜测恢复。到期/drain的正常未改参原调用仅在同一租约、已知owned入口与精确原始出口下可证明自然恢复。

六次重排与四条记录、外部1ULP改写、嵌套、原调用异常、restore递归旁路、到期/drain及original执行期间撤销请求等离线检查通过。独立只读实现审查未发现阻碍本次实验的缺陷。Native89完整构建及八项wrapper/graphics/UI bypass/GPU frame-link自测通过，DLL SHA256 `5efa24ce70161c5447364a6baa85ca5389adc3657504b5eb2a5cd7f81b3c8352`。实际游戏的持续位移、恢复、子布局、GPU裁剪和输入结果仍待验证；16px只是实验量，不是最终玩家居中设置，默认Native70和开发包未改变。

Native89实际`path-layout89-20261018`捕获一次正常受控重排：modified/adopted均1、控制failure0，Top经原生单位换算后比最初fraction setter结果低1ULP，并已保存真实精确owned。随后hold认证失败，首次诊断明确指向check552、base+1c24730的全局对象表完整16B头：Data94946852864、Max309766保持，Num241760增加到241761。该记录支持“旧证书将无关对象表数量变化视为失效”的具体问题，不能据此认定目标panel已被替换。没有取得shifted GPU图或完成恢复验收；实验拒绝继续写入并以proof_failure停止。正常关闭、native/entry独立恢复及完整设置/四profile恢复通过，无自有前台采样，精简证据在ignored `analysis/runtime/path-layout89-reflow-proof-20261018.json`。下一私有修正将只允许已知注册表固定前缀内的证书在Data/Max稳定且Num不低于初始值时继续使用，固定对象槽与全部身份/成员字段仍保持精确认证；采集边界仍完整复读，不引入通用字节mask。

`55f83f2`实现显式schema2 `registry_header_policy=fixed_prefix_v1`。仅base+1c24730与base+1c34800的完整16B头适用，两个头都必须存在且合法，不允许部分/跨头/重复别名覆盖；Data/Max保持，实时Num可在初始Num至固定Max之间变化。具体对象/名称槽仍来自首次有效范围，所有依赖字节继续exact。此策略允许无关后缀变化，不声称整个表单调追加。schema1保持原逐字节规则，Python采集边界和owner serializer未改变；core与诊断共享同一matcher，接受计数来自原有同次Read。四项`/W4 /WX`检查及31项Python检查通过。

Native90从该提交冻结，DLL SHA256 `e5805eea57654a02075e8b7ef4146924da315d821540db2823f399a77586dbcc`，完整构建与八项自测通过。实际`path-layout90-20261018`完成两次apply和两次restore，涵盖6秒到期及owned状态主动停止；identity/ownership lost、pending、unresolved及控制failure均0。记录346次注册表后缀变化的接受读取，首条对象Num241814到241826，Data/Max保持，目标身份与原生代码认证仍通过。两次正常重排均修改并承认实际owned，未把原值当自有位移。

该轮GPU保存baseline、shifted16、restored三张2534×1009图像。上方中央文字区逐眼比较，位移最佳匹配均为向下15输出像素，与16×1009/1080≈14.948一致，IoU分别0.9824/0.9819；恢复两眼均dy0且IoU1.0。根协调另检查了左眼三图预览，标题、左右选择及提示均可见。此结果验证该幅度下的实际双眼文字平移与恢复，不证明全部裁剪、输入命中或真头显体验，也未接受16px为最终居中量。完整证据在ignored `path-layout90-reflow-proof-20261018.json`、`path-layout90-gpu-translation-20261018.json`。正常关闭、native/entry独立恢复、设置/四profile恢复通过，无自有前台采样；默认Native70和开发包未改变。

现有静态与实际实参还确认：安全区宽约`1080×0.9252×4/3`、高约`1080×0.9252`并居中；安全区居中不表示其顶部内容已居中。下一候选保持宽高和L/R，对确认范围内的PathChoice内容作一次更大的Y平移，检查完整文字、间距、裁剪与正常选择。不能直接缩小百分比并假设字形和停靠关系不变，也不推广为所有UI的共同根因。

`dce60a3`为schema2增加有界可选`vertical_offset_reference_pixels`，范围1..400、默认16；省略或显式16仍输出完全相同的旧INI字节，schema1拒绝新增字段。apply和正常reflow使用同一租约偏移，续用请求也必须匹配该值。原16px检查保留，新增374px的多次原生单位换算、精确hold/restore、边界及非法配置检查；四项原生`/W4 /WX`与32项Python检查通过。

Native91从该提交冻结，DLL SHA256 `24475036a5bc339fba19283ed508d7c8335f8f48e3f1c404eccaa55296658bdf`，完整构建和八项自测通过。候选在当前13节点原始几何与Native90基线逐项一致时才允许试验374参考像素的Y平移；它是待验的内容居中量。首轮`path-layout91-20261018`在首次移动释放后的只读姿态采样发生AnchorRegistryChanged而停止，尚未到达位置试验，未写位置配置、没有新的居中GPU证据。游戏正常关闭，native/entry、设置及四profile恢复通过，无自有前台采样；后续只针对该明确采样失败做有界完整Resolver重新采集，身份或角色变化仍停止，不放宽生产路径认证。

`path-layout91b-20261018`复用同一冻结DLL，通过实际基线几何认证后完成374参考像素候选。两眼文字最佳位移均349输出像素，与374×1009/1080≈349.413一致，恢复均dy0且IoU1.0。根协调查看了baseline/centered/restored的完整双眼六图，标题、左右箭头、两项选择及Mouse提示均完整且已靠近中央；没有把上方35%ROI当作居中后的裁剪范围。受亚像素取样影响，中间图文字mask IoU约0.7873/0.7950；它不是全图或输入验收。两次apply/restore均完成，包含owned状态主动停止；identity/ownership/setter/control失败、pending、unresolved均0，native自然restore计数仍0，不冒充该分支实际覆盖。正常关闭、native/entry独立恢复及设置/四profile恢复通过，无自有前台采样。精简proof、GPU对比和完整状态分别归档ignored `path-layout91b-reflow-proof-20261018.json`、`path-layout91b-gpu-translation-20261018.json`及`path-layout91b-20261018-artifacts/`。

当前幅度通过本轮图像实验，尚未接入玩家自动流程。原有自动monitor服务的是FCanvas试验，不能直接复用其换nonce续期来反复采集已移动的Position作为原值；正常选择使ActiveScenes变化时，现有租约是否仍能认证与恢复也还缺实际证据。下一轮先用现有私有开关和首次失败记录观察正常选择关闭，保留未知值不写和完整清理，之后再实现自动生命周期；默认候选与开发包继续未改变。

91c在写入前的输入绑定采集中遇到完整读集末端变化，零apply/零选择输入后停止。91d仅在写sidecar前允许最多三次完整新reader采集，边界变化按明确类型/消息丢弃整组，其余身份、角色、反射及未知失败仍停止；写入后没有请求或输入重试。九项新采集检查和原十项输入合同检查通过。

实际`path-layout91d-20261018`第一次完整采集即通过：运行时认证左/右/Start的NameProperty数组、本scene的CustomInputKey delegate、原13节点几何及全部owner证明，申请374px/6000ms租约后只向自有HWND发送一次左键down/up。两次均发送成功且all_up=true。约340ms后PathChoice离开ActiveScenes；随后32次样本中原panel仍注册、X/units保持、Y精确等于受控owned值，最后一次约在release后5.048秒。已有观察最终因UI registry快照变化而停止，不能把它解释为目标失效或追加增长。

停止时主VR原因是驱动cancel的event，非UI proof_failure；identity/ownership/auth/control失败均0。但私有位置统计为applied1/restored0/pending1/unresolved1，表明普通关闭后现有匹配绘制回调未完成恢复。native/entry释放、正常游戏退出及设置/四profile恢复均通过，**不代表隐藏widget位置已恢复**。精简证据为ignored `path-layout91d-close-proof-20261018.json`，原状态在`path-layout91d-20261018-artifacts/`，无自有前台采样。

下一步优先确认原生CloseScene的自然入口是否早于ActiveScenes移除：若实际同owner线程、目标scene仍活跃且现有完整认证通过，可先恢复再执行原关闭一次，避免给inactive对象猜测liveness或放宽通用检查。当前只有脚本native声明与历史字符串，尚无此版本的exec/实现ABI或当前字节认证，因此尚未增加CloseScene hook。先只读解析精确运行时UFunction入口和直接实现，再观察实际关闭顺序；该缺口仍阻止玩家自动启用。
