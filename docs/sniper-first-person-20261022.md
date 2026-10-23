# 狙击枪持枪时的第一人称切换修复

2026-10-22。用户明确报告：在第一人称换到狙击枪会退回第三人称；第三人称持狙击枪时，E＋R 无法进入第一人称。已修复并通过自动检查，合入普通启动器的 SniperFirstPerson1 构建。新游戏进程生效；本次没有新增实际游戏或实体头显验收。

## 原因与证据

原 `first_person_live` 在识别到 `Weap_SniperRifle` 类族后，无条件跳过第一人称预览，没有区分持枪与实际开镜。这会让第一人称开关仍为启用时，狙击枪的渲染相机退回原版第三人称，也解释了持枪按 E＋R 没有效果。问题在当前预览的相机选择规则。

工作副本和未修改 Steam 副本的磁盘 `Engine.u`、`WarfareGame.u` 分别逐字节哈希一致，SHA256 为 `8d6ad508162609482656fd892cdadc30e0438c8ce2c44fc87163a99c0601666a`、`cd6d9a98c809d5520783a732f955916111e20bb6bf3155fb8b15dea86d7d0538`，没有在这两个包中发现旧第三方第一人称替换包的残留。

先检索知识库并复用 artifact 2 已保存的 `Weap_SniperRifle.UseFirstPersonCamera`、`SetZoomed` 脚本。前者返回 `bZoomed`；这支持按武器实际开镜状态区分相机，而不是用 FOV 阈值或武器类别代替开镜状态。保存脚本的 SHA256 为 `e096866710f95fbb1a798dfe921c8dea19581bb9743fe90c519fc6b44b78f67d`。该来源是已有解包证据，与本次磁盘压缩包哈希、实时内存读取分开记录，没有新增 IDA 分析。

已有用户会话 `continuous-stereo-32628-20261022-033835-3d51cc56` 使用 MountedResume2，记录了 `owned_material_reticle_draws=5211`、`owned_material_reticle_suppressed=0`、`owned_error=0`、`owned_ui_first_fault=0`。这仅表明此前镜框路径已经运行且未记录相应故障；会话最终为 incomplete，记录游戏进程退出／身份失效及清理未完成，不能作为整局或本次新第一人称代码的验收。

## 实现

- 未开镜的狙击枪与普通枪械使用相同的第一人称预览，支持换枪时保留选择，也支持持枪时 E＋R / L3＋R3 切换。
- 只有已认证的 `bZoomed` 为真时保留原版镜内相机，同时阻止“镜头拉近”再叠加。第一人称开关仍保持选择，收镜后恢复预览。
- 从当前 Pawn 的当前武器读取类链，在已验证的反射布局中发现 `bZoomed`，验证其名称、所属类、BoolProperty 类型、数组维度、元素大小、字段位置、单比特掩码以及匹配 EXE 的 Bool 访问器和分发地址。
- 保存的属性地址仅用于减少重复遍历。每次使用都重新核对对象槽、名称、类链、属性布局和实际标志；读取前后检查开镜位与当前武器指针。换枪、失效对象或元数据变化时拒绝本次预览选择，保留原相机与用户切换偏好。
- 没有写入开镜状态、武器瞄准、角色动画或 FOV；HMD 仍由原有双眼流程应用。这里不建立机械瞄具、弹道或狙击镜放大画面的新验证结论。

## 构建、验证与默认集成

从最新 MountedResume2 冻结源码制作候选，基线 native SHA256 为 `e89ff571364c6afde938d558ab2860405b6bb13ad813c95e91546abed7c774fe`。只改变六份文件：五份第一人称实现／测试文件和 `capture.cpp` 中对应相机选择变量名；没有整体复制含其他实验的工作区源文件。既有机枪恢复、狙击镜材质、掩体瞄准、renderer、XR 与 Playground 设置保留。

37 项原生／启动前检查通过，其中包含第一人称视图、武器状态、镜头拉近、相机观察器、准星、狙击镜材质、图像帧存储和世界恢复。新增用例覆盖换入狙击枪、持狙击枪按 E＋R、开镜／收镜保留选择、狙击枪子类、错误属性归属／布局／类型、失效槽、循环发现链、开镜位变化、元数据变化及检查末尾换枪。允许无关的同字标志位变化，不把它误判为开镜切换。断言保持启用，既有 45 组视图组合检查继续通过。离屏共享输出回归不等于本次狙击枪实际 GPU 画面验收。

485 项编译依赖核对、冻结源码构建后哈希复核、109 项启动器 Python 回归检查通过。启动器文件验证确认候选与受支持零售 EXE 匹配。

普通启动器选择 `tools/native-capture/build-sniper-first-person1/GearsCaptureSniperFirstPerson1.dll`，SHA256 为 `9a43860f0adaa047314f4b676683c15ada1ef5027662a75f241361c37bbdde75`。具名清单为 `tools/vr-launcher/manifest-sniper-first-person1.json`。集成只更改三个 native 字段，用户配置哈希保持不变，没有更新发布标签或发行包。

没有启动或操作新游戏进程、改动存档、进行 RenderDoc 或实体头显验收。实际持枪画面、开关镜连续性和头显体验仍属于累计待验收项，无需单独实验入口。

审计、冻结源、构建和测试回执保存在 ignored `analysis/sniper-first-person-20261022/`、`analysis/sniper-first-person1-20261022/`、`analysis/sniper-first-person1-source/` 与 `analysis/native-build-sniper-first-person1/`；专有脚本和运行日志不进入发行包。
