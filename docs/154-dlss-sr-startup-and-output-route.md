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

随后实际会话 `continuous-stereo-39168-20261018-023738-83b69260` 完成独立观察：三个完整当前chunks认证通过，8条记录覆盖4张ticket的双眼，传入view始终等于TLS current-view和对应renderer eye。每个Output均恰好一次setter及quad，进入/退出view字段一致，没有嵌套或溢出。view的两组矩形均为合法半宽，而 `d83cbf` 收到的仍是上述巨大原点与全窗口范围。

setter `d83b69` 的当前描述符均为 `{65535,65535,0}`、index0、返回0。已核对实现先比较byte_count与index×16，当前是无操作，不访问bank；因此不能将缺少bank记录当故障，也不需要伪造shader常量消费证明。这一轮只读观察仍为0发布、0 scene copy、没有保存到场景GPU图片。native cleanup、正常游戏退出和完整设置/profile恢复通过，未记录自有游戏进入前台。

下一隔离实验仅复制直接分支的矩形语义，同时建立每次scope/draw完成及publication门禁；不能靠宏白名单放行。即使恢复双眼输出，也仍需单独证明实际NGX输入缩小、时域历史与SR效果。

## 其他主线进展

`50cbfcd` 为 MenuReader 增加可选的 Position/DockTargets 原始布局采集，70项离线测试通过。普通采样不增加这些读取，未改变UI位置。实际普通双目回归 `ui-layout-details4-20261018` 提交1359次、scene copy723次，约15.08秒，最大提交间隔28.7181ms；正常清理和配置恢复通过。岔路详细采集初次遇到导航碰撞、脚本结果字段及注册表变化；后续 `ui-layout-details7-20261018` 已取得完整13节点及布局详情，见[文档155](155-path-choice-position-and-docking.md)。这仍不是UI修复验收。

机枪方面，实际函数发现已把 `PlayerUpdateCameraNative` 的exec映射到 `3b3af4`，其拆包后直接调用已有 `110878`；炮塔 `GetBaseViewOffsets` 的exec为 `1248238`，实际实现经模式对象虚表调用。发现不等于全部当前函数字节或实际挂载验收。现有准星配对证据是 AssaultRifle，不是手枪，不能用于宣布用户报告的手枪外圈问题已修复。
