# UI 知识库首批检索与未完成路线

2026-10-15。本轮仅整理已保留文件，没有调用 IDA、重新反编译、启动游戏或修改 Mod。机器可检索结果见 `analysis/re-kb/ui-findings.json`。后续开发应先查询这些证据，再对确实缺失的函数做定向调查。

## 来源与可信度

`analysis/ui-v1-script-audit/manifest.json` 是两个解压脚本包的候选清单：1705 + 1004 = 2709 个导出文件；1093 个仅为 native 声明，1010 个含未解析 native token，888 个含类型占位。文件导出成功率不能当作原生反编译成功率或功能理解覆盖率，包对象索引也不能当作 RVA。所有衍生内容继续仅留本地。

原生静态证据与脚本声明分开。Material 原始证据明确标记 frozen runtime code SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，不是磁盘 EXE 身份。本轮没有独立核验该快照文件或确定捕获进程；其他证据未给出的版本、快照路径和 PID 均保留未知。RVA 只能在匹配二进制身份后复用。现有记录指出部分 IDA RIP 符号显示偏低 0x1000，应采用已保留的原始指令位移结论，不能按显示符号机械重定位。

可信度按结论分别判断：静态路线可高可信，但运行调用、精确目标资源、实际 ticket 关联和视觉正确性是独立证据。此处运行结果是对既有报告的归纳，本轮未重跑。旧 UiObserver3/4 的 44 字节步长结论已被 48 字节修正取代，禁止将旧字节匹配升级为实际顶点证据。

## 实施顺序

| 优先级 | 公共路线 / 缺口 | 已有依据 | 下一步最小验收 |
| --- | --- | --- | --- |
| P0 | 屏幕 HUD 与世界标记分类 | `WarHUD.UpdateHUDLocators` 明确读取相机并更新 locator / 可见性；PostRender 父 scope 不是屏幕固定性证明 | 明确认证屏幕布局 scope，排除未知和世界分支；逐个绑定 subject / anchor / scene ticket |
| P1 | Tile/Icon 与整段 Text 消费 | `1d52b4 → 1bc05c → 19e64c → 172741`；`2482b8 → 1bd880 → 19e64c`。UiObserver7 报告 2650 个消费者/索引序列/overlay，且文字有 1112 段 | 复用已认证 receipt 和冻结矩阵；获取正常比例或原始 GPU 像素核对 HUD 可见性、位置和长文字。HUD 平面 applied 计数不是视觉验收 |
| P1 | Material 独立消费与 GPU 路线 | `3a4d3c → 3a5548 → 3a514c → 15a420 → 159fcc`；独立 112 字节 payload / 36 字节记录 / 32 字节 GPU 顶点；GPU 返回点 `1af345` 或 `177c4a` | 先只认证 count=1、纯屏幕 scope、mode=0，关联实际 IA、OM target、ticket 与源变换；保留多 pass 和透明行为。consumer 只调用一次，禁止释放后读 payload |
| P1 | 真实对白字幕 | `2a4370 → 2470ac → 246c58 → 1bd880` 独立于 PostRender 内 DrawText | 在真实对白场景取得非空 glyph、对应 GPU/ticket 和两眼像素。现有 14497 / 55393 次入口均空 delta，不能认定字幕完成 |
| P2 | 暂停 / 设置 UIScene | 现有类型和状态，以及 `2c9481` 的 virtual +238 调用线索 | 先认证对象/vtable/实际 render 函数、目标和合成顺序。不能把未明调用命名为 RenderUI，启动 WGC 不能直接覆盖双目接续后的菜单 |
| P2 | 准星有限深度目标 | `execLet 1d7ad0`、StructMember `1d6750` 与赋值 Script+359..37f 已有契约 | 认证 fresh GNatives / owner / armed writer 分支、实际 PlayerTick 覆盖和一次性 writer→scene→ticket 传递。缓存变化、相同时间戳或 latest receipt 都不能证明本帧新鲜度 |

Material 路线的旧 `ui-coverage-audit.json` 尚称 `159fcc` GPU 路线未反编译；较晚的 `material-ui-gpu-route-contract.json` 与原始窗口已补静态路线，因此该旧 unknown 应标为部分被取代。仍然缺少实际运行的 Material 调用及源投影、资源和 ticket 证据；没有因此完成双眼修正。

## 共用层的边界

`19e64c` 与精确 `172741` 回调可复用经过认证的 Tile/Text/字幕批次；它不是全 UI 通道。Material 使用另一种 payload 与 draw 路线，不能按 Tile float4 或索引 segment 布局解释。底层 D3D draw 同时服务世界网格，必须有上层语义和实际资源证据才能接纳。World HUD、准星、整屏受伤后处理也不能因为处在父 HUD scope 就统一搬到显示平面。

已有脚本 `Engine.Canvas.DrawMaterialTile` 文件只是 native 声明，不能据此补造 native 地址。`WarHUD.DrawHUDMessages` 中的 Canvas 调用支持继续定向分类，但含未解析操作符，不构成所有消息分支的视觉认证。`WarHUD.UpdateHUDLocators` 的相机依赖支持保守排除，却不独立证明某一次 Canvas.Project 的来源眼睛。

验证场景至少覆盖开镜/切枪、弹药与主动换弹、交互提示、世界目标/队友、暂停/设置、死亡/受伤、真实对白及长文本。每次分别记载：路径命中、成员覆盖、资源代次/ticket、正确像素、退出恢复；模拟器通过不替代真头显验收。当前没有新的 RenderDoc 捕获或运行验收。本轮 AI 深入分析只复用现有材料，新增反编译函数数为 0，Token 实耗由平台统计，未获得计费数据不得填估算值。
