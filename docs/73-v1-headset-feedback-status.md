# 第一版真机反馈与候选状态

更新：2026-10-17。第一版仍未完成，不以单次正常退出或合成测试替代真机表现。

当前主线补充：开场CG后的双目发布容量阻断已在Reticle47修复；死亡时Controller.Pawn为空造成新票据中断的原因已确认。独立无占有候选可继续生成死亡画面，但右眼仍近黑，尚未提升为玩家默认。最新Hdrpixels54实际读回在死亡票据901的关键全幅copy源中看到了左右完整场景，将后续排查收敛到该copy之后的链路；最终黑图为相邻票据，尚缺同一票据的前后像素对照。见[HDR源像素证据与边界](110-death-hdr-source-pixels.md)。这仍是未完成的黑屏问题，不能视为修复通过。

| 用户反馈／要求 | 当前证据与状态 | 剩余工作 |
| --- | --- | --- |
| 机枪、载具与实时过场保持双目 | Native37进一步认证同一WorldInfo下不同Level的Turret／Driver等角色；完整构建、合成上／下机枪及普通关卡回归通过 | 实际机枪验证；当前只认证 Turret，不能推广为全部 Vehicle 或全部实时过场 |
| CG 不黑屏 | Native31／XR23 已覆盖首轮与一次检查点电影播放和原生双目交回 | 更多 CG 与头显回归；预渲染电影双眼共用二维内容，不宣称真实几何立体 |
| 游玩、换弹偶发黑屏 | 已修复中间纹理上材质绘制破坏双目标记；Native37另取得新完美换弹周期结果2及两眼GPU场景图（docs77） | 完美换弹全程连续性、其他黑屏触发和长期运行；本轮检查点仍出现约0.46秒世界重绑定过渡卡，不能称全部黑屏已修复 |
| FPS 开关导致黑屏 | Caption38独立包运行中经显示菜单开启并应用FPS，FrameCtr=True；新GPU帧两眼场景及38.87 FPS文字，清理与设置恢复通过（docs82） | 应用瞬间连续性、反复开关、其他画质及真机回归未完成 |
| 启动双目延迟、偶尔完全不启动或不进入VR | 已前移自测、增加空Pawn过渡重试、优化UI发现，并区分Crosshair注册表追加与身份失效；Caption38最新启动准备约4秒、相机就绪至首投影约7秒，连续三次检查点取得新代图像（docs78／80） | 启动和检查点恢复仍有数秒等待；单次样本不代表稳定提速，跨场景与真机回归未完成 |
| 头显卡顿但桌面尚可 | 旧真机保留样本的新图年龄中位约52毫秒，日志混有菜单与缺帧 | 分离活跃游玩帧节奏、渲染与传输；未认定为VD网络问题 |
| UI 更大、整体靠近视野中心 | Caption38包含元素缩放／中心收拢及多片段字幕布局；Reticle46修复新Canvas批次首个外圈四边形缺失来源的问题，已有同ticket完整外圈与附加图标共同校正证据（docs98） | 新准星双眼融合、实际字幕与更多UI仍需头显检查；保持未知索引拒绝规则 |
| 中英文启动器 | 3c83336／3229b4d，GUI构建及配置串联测试通过 | 新候选真机使用时继续观察 |
| 不抢焦点、不响应后台实体手柄 | e810eec／4678d2f 默认关闭启动链后台手柄；4678d2f为未注入的确切新进程增加失败清理 | 抢焦点全路径尚未证明消除；用户现已离开并授权无人值守前台测试，后台实体手柄仍默认关闭 |
| 按游戏分辨率、DLSS关闭、6DoF | 独立Low／High已移除；DLSS锁保持；6DoF已有用户真机反馈 | 新候选完整回归；不把旧版本反馈扩大到新改动 |

## 当前可用版本与测试边界

本地试用默认已切到 Reticle47／Borderless1，清单 `manifest-reticle47-borderless-vd.json`。Reticle46已有手枪和两次检查点证据；Reticle47新增开场CG后容量阻断修复、同章节实际双眼图像及全新ZIP独立运行通过。Reticle46及更早命名清单保留回退。仍未实际占有机枪或做新版本真头显验收。镜头拉近范围0–0.5米，本机用户0.3米设置未覆盖，便携包从0开始。最新证据和包哈希见[Reticle47候选版](102-reticle47-player-candidate.md)。

Native34首次进关前失败的PID39900：实际交换链8×8；状态文件偶发PermissionError让旧监视器反复清零，237a888已修复有界读取间隙，仍须下一份新鲜采样才可动作。之后PID38836在加载器依赖12秒截止前未就绪，原启动链留下进程；4678d2f补确切身份回执与未注入时的普通关闭。未用新代码重新运行，不能记为实际抢焦点问题验收通过。

此前运行测试暂停记录在 `analysis/runtime/runtime-test-hold.json`。用户之后明确授权睡眠期间继续无人值守修复与测试，当前记录active=false；允许测试时前台启动，仍保持后台实体手柄关闭。用户回来后恢复不抢焦点的协作约束。

## 部署准备

最新补充：独立候选包已完成一次本机对包外游戏安装的实际启动、进入双目及正常清理；发现并修复菜单导航两处固定仓库路径依赖。详情与失败边界见[独立包实际检查](75-v1-portable-runtime-check.md)。这仍不是干净玩家机器／真头显验收。

`stage_portable.py --manifest` 可单独打包实验候选，不替换默认清单或玩家配置。真实Native34离线候选第一次检查通过：693个清单文件、42,652,715字节；包内Python隔离模式导入33个项目模块，并核验清单、哈希及PE依赖，未启动GUI或游戏。原始回执 `analysis/runtime/hud34-portable-preflight.json`，目录 `analysis/release-staging/hud34-offline-20261017`。

这只证明本机的离线包依赖检查，`release_ready=false`；并非干净玩家机器、许可审查、实际安装或头显验收。包不包含游戏、模拟器、逆向数据库、伪代码或资料库。发布前仍须完成上表剩余项及实际部署验证。

补充：c61921e将底层Python函数／CLI也改为默认关闭后台手柄，只有显式参数才可启用。更新后重新生成的离线候选 `analysis/release-staging/hud34-offline2-20261017` 通过包内隔离预检（693文件、42653389字节），回执 `analysis/runtime/hud34-portable-preflight2.json`；未启动GUI／游戏，仍非发布验收。

启动准备补充：7b57d0e在有创建回执时固定同一PID／FILETIME／路径，只开一个进程句柄查询依赖模块，不反复启动PowerShell扫描。模块等待上限25秒，且受命令起始+50秒共同截止，依赖读取后、游戏哈希复核后及注入标记前后都再次核对时限与拥有者／取消／进程身份。8项定向mock测试通过，包括超过旧12秒后就绪、迟到结果和取消；没有实际启动验证。前述offline2目录在该提交前生成，仍是不可变旧快照，不包含此项Python修改；最终发布包须重新生成。

离线补充：038361c允许有完整单一范围证据的NativeSubtitle字幕整体使用元素缩放／中心收拢，字间距随同组缩放；多范围碎片和未知组仍回退。UiHudLayout／UiCanvasTextBatch／UiPayloadReceipts CPU测试通过，尚未构建进冻结Native34或做画面验收。新打包流程会生成中英文README.txt，并纳入文件清单；13项打包测试通过。既有offline2包仍保持原快照，不含这两项后续变化。

冻结候选补充（Native35）：从已提交源码7524ea3单独导出native-capture／vr-common，复用Native34冻结MinHook依赖并记录逐文件哈希，完成Release全量构建。DLL为`tools/native-capture/build-subtitle35/GearsCaptureSubtitle35.dll`，SHA256 `c6b40ed4b1f3ab45e5874f2bbf660d7a25c13c30d9d2919b8d96b6215a43511d`。它包含此前机枪／HUD候选以及038361c完整单范围字幕布局；不含工作区尚未提交的旁路修改。10项离线检查通过：HUD布局／平面、收据、Canvas文字、索引、材质批次、游戏材质、FrameLink及DLL continuous／frame-link ABI。记录位于`analysis/runtime/subtitle35-source.json`、`subtitle35-selftests.json`和`manifest-subtitle35.json`。未启动游戏、模拟器或输入辅助，未替换默认Native32清单，未生成新发布包。GPU、机枪、字幕实显及头显稳定性仍未验收；保持运行测试暂停与后台手柄默认关闭。

用户指定试用Native35：默认manifest.json现临时选择manifest-subtitle35-vd.json（游戏／DLL／XR哈希复核通过），原Native32清单保留回退；这只是试用选择，不是完成验收或发布。用户配置未覆盖。本轮显式VDXR运行时、普通前台游戏启动，后台手柄仍关闭，不启动模拟器；用户自行游玩并决定结束时间。试用日志前缀`analysis/runtime/subtitle35-user-trial`。

机枪最新进展与完整Native37构建／回归边界见[流式Level认证](76-mounted-streamed-level-certificates.md)。

最新独立打包自测、完美换弹判定与新发现的启动过渡失败见[回归记录](77-active-reload-startup-regression.md)。

UI发现优化的同进程测量、完整VR与检查点回归及最新独立快照见[启动准备耗时](78-startup-discovery-cost.md)。

最新字幕双眼图像、多片段布局修正、Caption38候选与剩余边界见[字幕回归](79-subtitle-multirange-layout.md)。

检查点重绑定重复解析已去除，78项测试及两轮实际恢复通过；耗时与边界见[重绑定准备耗时](81-rebind-discovery-latency.md)。默认仍为Caption38／Borderless1。

最新独立包实际启动及游戏内FPS开启证据见[Caption38独立包回归](82-portable-caption38-fps-runtime.md)。

手枪实际开镜／一次射击样本中外圈已落在计划位置，尚无独立内外圈配对，问题仍开放；详见[手枪准星检查](83-pistol-reticle-runtime.md)。

本地测试ZIP已生成并完成解压隔离预检；新步枪配对与采样器提前停止的边界见[本地交付与步枪检查](85-local-zip-and-rifle-pair-check.md)。仍不是正式发布。

实际接近机枪尚未成功；已排查一次转向校准失败，未注入对照同样失败而数字键后恢复响应，未修改VR yaw；详见[机枪导航与手柄对照](86-mounted-navigation-controller-baseline.md)。

Reticle39修正准星片段因空裁剪选择不同投影的问题；19项自测及两次实际检查点恢复、新双眼GPU图像和正常清理通过。尚未证明用户外圈偏移消失；旧Caption38测试ZIP仍是原快照，详见[准星裁剪一致性](87-mounted-approach-and-reticle-clipping.md)。
