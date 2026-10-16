# 第一版真机反馈与候选状态

更新：2026-10-17。第一版仍未完成，不以单次正常退出或合成测试替代真机表现。

| 用户反馈／要求 | 当前证据与状态 | 剩余工作 |
| --- | --- | --- |
| 机枪、载具与实时过场保持双目 | Native37进一步认证同一WorldInfo下不同Level的Turret／Driver等角色；完整构建、合成上／下机枪及普通关卡回归通过 | 实际机枪验证；当前只认证 Turret，不能推广为全部 Vehicle 或全部实时过场 |
| CG 不黑屏 | Native31／XR23 已覆盖首轮与一次检查点电影播放和原生双目交回 | 更多 CG 与头显回归；预渲染电影双眼共用二维内容，不宣称真实几何立体 |
| 游玩、换弹偶发黑屏 | 已修复中间纹理上材质绘制破坏双目标记；Native37另取得新完美换弹周期结果2及两眼GPU场景图（docs77） | 完美换弹全程连续性、其他黑屏触发和长期运行；本轮检查点仍出现约0.46秒世界重绑定过渡卡，不能称全部黑屏已修复 |
| FPS 开关导致黑屏 | Native33 开启 FPS 冷启动已有两眼场景和 FPS 文字，资源清理通过 | 游戏内即时切换 FPS 尚未复现验证 |
| 启动双目延迟、偶尔完全不启动或不进入VR | 已前移自测，并修复受保护的空Pawn过渡重试；UI发现均值降低约27%，最新Native37完整启动准备约6.5秒、相机就绪至首投影约9.5秒（docs78） | 延迟仍存在；最新捕获HUD证书准备时Controller.Pawn变空导致启动退出，见docs77；不能只依据进入关卡判断修好 |
| 头显卡顿但桌面尚可 | 旧真机保留样本的新图年龄中位约52毫秒，日志混有菜单与缺帧 | 分离活跃游玩帧节奏、渲染与传输；未认定为VD网络问题 |
| UI 更大、整体靠近视野中心 | Native37包含元素缩放／中心收拢、完整单范围字幕布局和分片准星共同中心；实验清单1.2／0.8，保持用户角宽 | Native36已有实际双目开火和外圈GPU图；新准星双眼融合、实际字幕与更多UI仍需头显检查，多范围和未知分组保持回退 |
| 中英文启动器 | 3c83336／3229b4d，GUI构建及配置串联测试通过 | 新候选真机使用时继续观察 |
| 不抢焦点、不响应后台实体手柄 | e810eec／4678d2f 默认关闭启动链后台手柄；4678d2f为未注入的确切新进程增加失败清理 | 抢焦点全路径尚未证明消除；用户现已离开并授权无人值守前台测试，后台实体手柄仍默认关闭 |
| 按游戏分辨率、DLSS关闭、6DoF | 独立Low／High已移除；DLSS锁保持；6DoF已有用户真机反馈 | 新候选完整回归；不把旧版本反馈扩大到新改动 |

## 当前可用版本与测试边界

本地试用默认已切到 Native37／Borderless1，清单 `manifest-mounted37-borderless-vd.json`；增加分别认证流式Level的机枪绑定修正，已完成4K普通关卡／检查点回归，尚未实际占有机枪或做新版本真头显验收。Native36／35／32命名清单保留回退。启动器新增实验性镜头拉近0–0.5米，本机按用户希望拉近的要求设为0.3米，其他用户配置保留；便携包仍从0开始。FPS对照临时设置已恢复。最新证据见[Native36修复记录](74-native36-ultra-reticle-camera.md)。

Native34首次进关前失败的PID39900：实际交换链8×8；状态文件偶发PermissionError让旧监视器反复清零，237a888已修复有界读取间隙，仍须下一份新鲜采样才可动作。之后PID38836在加载器依赖12秒截止前未就绪，原启动链留下进程；4678d2f补确切身份回执与未注入时的普通关闭。未用新代码重新运行，不能记为实际抢焦点问题验收通过。

当前运行测试暂停仅限制启动游戏／模拟器及输入操作，离线编译、单元测试、日志分析和候选打包继续。状态记录 `analysis/runtime/runtime-test-hold.json`，不能因目标自动续跑而忽略。

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
