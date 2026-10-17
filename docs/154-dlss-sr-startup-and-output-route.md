# V1 DLSS SR：隔离启动与输出分支

2026-10-18。用户已把 DLSS 超分辨率列为 V1 必做项。当前玩家默认仍为 Native70，DLSS/DLAA 保护仍开启；本记录中的独立候选没有提升默认，也未加入发布包。DLAA、帧生成不在新增范围。

## 已实现的实验入口

`b701095` 增加独立清单字段 `experimental_preserve_upscaler=true`，经 Launcher、reservation 和 debug command 显式传到启动脚本。它只读检查唯一 `DLSS=True`、FSR2/FSR3/MFSR 关闭和唯一质量值3；质量4及其他值拒绝。质量3的 Ultra Performance 名称来自此前相同磁盘版本的配置证据，不推广成完整质量枚举。实验标志与启动或菜单图形锁并用时提前拒绝。默认缺省行为仍强制 OFF。相关92项测试及 PowerShell 语法检查通过。

此入口只保留测试配置，不能证明 NGX 执行、超分辨率生效、每眼历史隔离或图像正确；运行中也不是排除所有 DLAA 切换路径的产品策略。

Native73 从冻结 Native70 原生源独立构建，仅关闭 graphics runtime/startup/menu 三个保护宏，保留只读 graphics observer。DLL SHA256 为 `ad3c543ea9fc40e084d3aa557ae61e572b104aa50967b3c915ebb19dfecf7232`。

## 实际游戏结果：场景接受，输出未发布

- `dlss73-gameplay-20261018` 首轮在 UI 初始化校验处拒绝，native 未激活、没有场景提交；native cleanup 未验证，随后游戏正常退出。不能据此认定 DLSS GPU 故障或正常清理通过。
- `dlss73-scene2-20261018` 暂停 HUD 修正和 world recovery，仅隔离普通场景。原生 normal/alternate DLSS 字段均为1；左右场景分别接受686次，最终 GPU 发布与 XR scene copy 均为0。
- `dlss73-fullui3-20261018` 恢复全部 UI 功能后 native 激活成功，仍然0发布、0 scene copy。因此首轮 UI 拒绝未稳定复现，不能将它认定为 DLSS 输出失败的统一根因。

后两轮 native cleanup 验证通过。三轮均正常关闭自有游戏，完整设置与四个受保护 profile 按测试前内容恢复。均未观察到自有游戏进入实际前台；没有移动或调整窗口、启用实体手柄后台响应或修改全局 OpenXR Runtime。测试使用 quiet2 子进程 Runtime 和 tiny-output observe-only。

原始数据及完整身份留在 ignored `analysis/runtime/`。设置布尔、场景调用和提交计数不代替 GPU 图像或真头显验收。请求的 GPU 快照没有生成场景图片。

## 已缩小的代码位置

新路径实际 quad return RVA `d83cbf`，此前已认证的路径为 `d83b1c`、`d808d8` 等。Native73 记录左目标x约为2³²、每眼目标及source宽2534、右source从1280开始；不是两个合法互不重叠半宽矩形。

已有历史代码节 SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、基址 `0x140000000` 的完整缓存汇编显示：该分支将32位目标原点零扩展后转成浮点，并复用生成的目标宽高作为source宽高。它可以解释负原点变成巨大正数的机制，但日志中的舍入浮点不能反推出精确原整数。需要记录实际生成路径及shader常量后才能确定修正。

现有 final-eye 认证未覆盖该分支；final transfer 无有效源票据，`pair_rejection=16` 对应 ticket_conflict（票据为0也会产生），不是GPU资源地址冲突的证明。正常化在 entry 预检拒绝，符合现有门禁。不能仅加入caller白名单或单独改符号。

`ui-layout-details5-20261018` 在另一正常游戏进程中对 `d82f58` 主chunk3192B、continuation489B、setter `1760b4` 205B进行了完整当前字节双读及SHA核对，全部匹配上述历史chunks；这不是任意后续进程的自动认证。主函数R8是传入view，是否等于TLS current-view仍需运行观察。相关shader setter与DOF保护使用的setter不同，不能复制DOF标量归零策略。

## 有界只读观察器

`a0b90ec` 增加默认关闭的 `GEARS_DLSS_ROUTE_OBSERVER`。独立构建在启用前重新认证上述三个完整chunks，观察输出函数与常量setter，最多保存8条Output、每条最多2个setter和quad。记录传入view、TLS view、原始矩形、采样向量、常量提交与消费bank；不修改参数、资源或任何票据认证。

实际wrapper离线测试覆盖原调用一次、返回值、嵌套/异常恢复、额度耗尽、payload保存及缺读；跨线程try_lock验证原函数执行期间不持观察器全局锁。清理超时且仍有在途owner时禁止读取未完成Row，仅输出有界摘要。测试通过不代表实际游戏或DLSS图像通过。

Native74 独立候选已完成编译与wrapper检查，DLL SHA256 `72941b77148f1647de77389365d586cf80c63d6692f005d82be4dd2a9cae070b`。实际观察结果见下；首发 DLSS 支持仍未完成。

随后实际会话 `continuous-stereo-39168-20261018-023738-83b69260` 完成独立观察：三个完整当前chunks认证通过，8条记录覆盖4张ticket的双眼，传入view始终等于TLS current-view和对应renderer eye。每个Output均恰好一次被观察器筛选的目标setter `d83b69` 及quad；这不是整个Output只调用一次setter，正常先行的 `d8388e`、`d8397d` 不在该计数内。进入/退出view字段一致，没有嵌套或溢出。view的两组矩形均为合法半宽，而 `d83cbf` 收到的仍是上述巨大原点与全窗口范围。

setter `d83b69` 的当前描述符均为 `{65535,65535,0}`、index0、返回0。已核对实现先比较byte_count与index×16，当前是无操作，不访问bank；因此不能将缺少bank记录当故障，也不需要伪造shader常量消费证明。这一轮只读观察仍为0发布、0 scene copy、没有保存到场景GPU图片。native cleanup、正常游戏退出和完整设置/profile恢复通过，未记录自有游戏进入前台。

下一隔离实验仅复制直接分支的矩形语义，同时建立每次scope/draw完成及publication门禁；不能靠宏白名单放行。即使恢复双眼输出，也仍需单独证明实际NGX输入缩小、时域历史与SR效果。

私有 `GEARS_DLSS_OUTPUT_CONTROL` 候选已实现，默认关闭，独立于8条观察额度保存逐票据证明。审查修正了两处问题：仅统计目标setter，正常先行调用另计；实际SRV查询前后核对context当前命令列表的raw/canonical身份及generation，避免把不同列表的资源拼成凭据。实际wrapper的正常路径、先行setter、列表不匹配/漂移、重复/缺失/异常和旧caller拒绝测试通过。完整DLL构建、实际游戏和GPU输出仍待验证，不代表DLSS支持完成。

Native75 已完整编译，三项wrapper/policy测试通过；启动前 `GearsCaptureFrameLinkCheck` 在GPU接收fixture返回179，尚未启动游戏。该旧fixture不执行Consume，新publication门禁因而没有对应完成记录，正确拒绝；已在专用fixture增加Missing/Incomplete必须拒绝及显式完成NoSite后才能通过的断言。未放宽游戏门禁。设置与四个profile恢复通过，下一独立构建验证该修正。

Native76（SHA256 `96c96f0deb058b9e08a0762e8d3c6b7d51225df6ce084e647a605604cb7f3e0b`）的GPU启动自测通过，随后实际会话 `continuous-stereo-916-20261018-025942-2e064f2f` 正常激活，但forwarded/draw/source-query/publication仍为0。native cleanup通过，独立entry恢复验证失败，游戏正常关闭，设置和四个profile恢复，无自有前台采样。审查确认恢复验证器缺少新Output函数分支，已补完整主chunk和continuation固定SHA比较及七项漂移拒绝测试；不能把尚未重跑的验证器修正写成该会话恢复已通过。

另外，完整缓存调用链证明Output可在最终quad前经 `194950` 调用 `194c93` 的1024×32预处理quad。控制曾在caller筛选前计入所有quad，现改为仅计 `d83cbf`，其他调用单列诊断；实际wrapper补了该先行调用用例。旧日志未逐次记录该预处理执行，尚不能宣布它是本轮唯一根因。

Native76实际绑定高水位为10；缓存 `1b4ca0` 将context+65d设为 `max(old,slot+1)`，因此它不表示shader采样10张纹理。主输入绑定候选调用位于 `d831c3`（return `d831c8`），descriptor决定写入context绑定数组的槽。下一步观察实际descriptor、写入对象及draw前同槽是否被覆盖，不通过槽0或唯一尺寸猜源。

针对NGX的有界历史静态分析进一步定位Create/Evaluate共用handle槽RVA `1c20bd0` 和参数对象槽 `1c20bd8`；所读游戏侧完整函数没有view/eye索引。Create尺寸来自第六参数结构的0/4/8/c，Evaluate渲染尺寸来自入口R9结构+30/+34，Reset为+38。这是历史代码的数据流证据，尚未认证当前这些函数或观察两眼handle值，也不能据此断言DLL内部历史结构。恢复矩形输出之后仍要验证实际尺寸、双眼handle及历史状态，首发DLSS支持继续未完成。

Native78 在关闭UI试验的组合中暴露了头文件被包含在匿名namespace内的编译问题，未构建成功或启动游戏。纯helper头现放到全局include区。Native79（SHA256 `411770d1c3112eb66058a565cf8fe03a282e1ab85b271288c858446cd50ec281`）完整构建、输入wrapper和GPU frame-link自测通过。

Native79实际会话 `continuous-stereo-9388-20261018-032303-3e4e9656` 得到541次局部矩形转发；此前预处理quad误计已被消除。541次source查询均被旧单槽/格式24假设拒绝，实际draw证明和publication仍为0。完整命令列表关联通过，native cleanup、所有新旧entry独立恢复、正常关闭及设置/profile恢复通过，无自有前台采样。

新增默认关闭的输入绑定观察器完成 `1665dc` 全部503B的独立当前认证；8条记录覆盖4张ticket的两眼。`d831c8` 的descriptor均 `{0,1,0}`，由真实descriptor选定槽，而非按尺寸搜索。绑定后、quad及actual draw时，选中槽对象始终等于公共资源链解析对象，真实canonical资源为2560×1080、format26、单mip/sample/layer；descriptor/context未改变，draw命令列表身份及generation匹配。绑定前槽内640×270、format10资源是之前的状态，不当作NGX输入尺寸或SR生效证据。

下一私有修正建立独立于8条诊断的BindScope完成凭证，重新核对上述关系，并只扩展已观察的源format26；目标format24和最终传输契约保持。Astra对该CPU绑定凭证复核通过，可用于像素待验的矩形修正实验；它不证明shader采样用途、NGX降分辨率或独立history。

后续默认关闭的控制已接入独立BindScope；它不借用8条观察记录作为许可。启用时另外完整认证1665dc的503B；绑定必须由本次descriptor选择槽，正常完成原始五参数调用，公共对象链与真实format26资源一致，并在quad、实际draw前后及Output完成时复核。缺失、重复（包括draw之后重复）、异常、对象/描述符/列表generation漂移和双眼来源不一致均拒绝publication。目标format24及final transfer未改。实际wrapper三组`/W4 /WX`测试与10项独立恢复验证测试通过；完整DLL、实际GPU及NGX时域验证仍待执行。

Native81从`e758929`冻结，DLL SHA256 `8478dac38c9f96519418ba189f43d0af2a198df773c69a001abef63ea54af2cd`。完整构建、三组wrapper、final-transfer policy及实际GPU frame-link ABI自测通过。实际会话`continuous-stereo-45048-20261018-033857-a8e9d2e7`的完整当前代码认证通过，549次矩形转发和549次actual draw证明，3843次绑定复核，source/list拒绝均0；Output完成仍0，没有GPU图片或XR场景发布。XR以1退出，会话为incomplete；native cleanup、所有entry恢复、正常关闭和完整设置/四profile恢复通过，无自有前台采样。这是绘制凭证的进展，不是DLSS输出修复完成。

有界KB查询及原已缓存完整continuation显示：`d83d53`读取view的family并检查其`+4c` marker，非零时`d83d5b`将`view+3e8`写为1，然后cleanup/正常返回1。Bind/quad/draw样本selector均0，而`binding_current`在Output退出按当前selector重新选择公共bank并要求与原值完全一致。此机制可以导致拒绝，但本轮没有退出处样本，不能认定它是实际唯一失败条件。下一轮只增加有界首次失败记录，分别保存完整性谓词及退出绑定状态，维持现有拒绝；证实正常转换后才能分离draw源凭证与Output生命周期契约。

另增默认关闭的`GEARS_NGX_ROUTE_OBSERVER`，只在完整认证当前Create/Evaluate包装器249/1377B后安装两处只读hook，固定最多4条Create、8条Evaluate。记录原始六/四GP参数、64位RAX、前后公共handle/参数对象槽、输入输出尺寸、Reset、raw资源指针及可认证的world/ticket/eye关联；不解释SDK返回语义、不强转未知资源接口、不参与publication。实际wrapper、默认OFF及13项独立恢复验证测试通过；尚无本观察器实际游戏数据。入口/出口槽值不是中途实际消费handle的证明，历史隔离、GPU与SR标志保持未验证。

## 其他主线进展

`50cbfcd` 为 MenuReader 增加可选的 Position/DockTargets 原始布局采集，70项离线测试通过。普通采样不增加这些读取，未改变UI位置。实际普通双目回归 `ui-layout-details4-20261018` 提交1359次、scene copy723次，约15.08秒，最大提交间隔28.7181ms；正常清理和配置恢复通过。岔路详细采集初次遇到导航碰撞、脚本结果字段及注册表变化；后续 `ui-layout-details7-20261018` 已取得完整13节点及布局详情，见[文档155](155-path-choice-position-and-docking.md)。这仍不是UI修复验收。

机枪方面，实际函数发现已把 `PlayerUpdateCameraNative` 的exec映射到 `3b3af4`，其拆包后直接调用已有 `110878`；炮塔 `GetBaseViewOffsets` 的exec为 `1248238`，实际实现经模式对象虚表调用。发现不等于全部当前函数字节或实际挂载验收。现有准星配对证据是 AssaultRifle，不是手枪，不能用于宣布用户报告的手枪外圈问题已修复。
