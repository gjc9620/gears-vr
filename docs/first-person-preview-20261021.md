# 第一人称预览、右摇杆死区与武器模型

2026-10-24 后续需求：[6DoF 掩体规格](6dof-cover-interaction-design-20261024.md)要求进入后身体面朝掩体、矮掩体中立眼点受遮蔽、头部自由观察，并以真实头动／左手抓边替换按键探头。本文记录的统一前移0.35米、抬高0.10米以及沿用原版掩体姿态属于现有预览实现，不能作为新掩体交互的最终方案。本次已定位源码入口，尚未实施掩体专用眼点修复或完成其运行验证。

2026-10-21。用户要求先体验可切换的第一人称，为以后的 6DoF 体感持枪准备。按键为手柄 L3＋R3、键盘 E＋R；再次组合按下返回第三人称。该请求取代文档163中先做“按住瞄准切换”的原型顺序。本轮仍不实现双手持枪、机械瞄具与弹道对齐或新的飞扑动作。

## 预览实现

启动器新增“第一人称预览”选项，启用切换能力；每次新游戏进程最初仍是第三人称。组合键同时按住80毫秒后切换，必须全部释放才能再次切换；失去焦点、剧情或非普通人物状态需要重新释放，避免恢复时误切。物理键盘、手柄只在游戏前台轮询。测试使用当前游戏进程独占的命名事件，不开启后台物理手柄输入。

在原有双目构造之前替换渲染相机，眼点取已认证的 Pawn.Location＋EyeHeight，再水平前移0.35米、升高0.10米；第一人称时不再叠加“镜头拉近”。前移方向使用原游戏相机的水平朝向，升高量不依赖俯仰。基础视图去掉滚转和位置动画，保留原视线方向；HMD 位姿仍在后续现有双目流程应用。CPU 45组俯仰、偏航和滚转组合验证无俯仰升高联动、前向不变和可恢复的刚性视图。

这一策略参考旧 Mod 去掉肩后偏移和稳定观看翻滚的方向，没有安装或发布其修改包；没有设置 bIgnoreControllers，没有修改武器/人物骨骼控制、射击起点、弹道、散布或枪械后坐力参数。沿用原游戏动作与持枪，因此“保留动画代码”不等于枪身反冲、近墙遮挡和掩体姿势已通过实际画面或头显验收。手柄/键盘组合当前是旁路轮询，原游戏的单键绑定仍可能响应。

普通受控人物才使用预览；剧情、机枪和未认证对象使用原视点。初版曾对整个 Weap_SniperRifle 类族保守回退；2026-10-22 的 [狙击枪第一人称修复](sniper-first-person-20261022.md) 已改为仅在实际开镜时保留原版镜内相机，未开镜持枪可正常使用预览，收镜恢复所选模式。分类从本次人物当前武器及已注册的类链读取，复核对象槽、头部、父类和武器指针，避免使用游戏线程 TLS 中的历史武器。换枪竞态、循环父类、失效槽和带编号的实例有独立 CPU 测试。

“进入掩体面朝掩体”仍沿用原游戏姿态，没有照搬旧包未证实的角色转向修改；预览视点在掩体、冲刺、翻滚和换弹时的模型遮挡需要实际逐项确认。固定水平前移也未建立墙面碰撞约束，不能宣称已经解决所有穿墙场景。抬头不再额外改变此相机锚点高度；人物自身 EyeHeight/姿势变化和真实头显平移是不同来源。

## 右摇杆死区

启动器新增0–50%的右摇杆死区，缺省2%。每次会话通过哈希限定的反射布局定位当前 PlayerInput：原 LookStickDeadZone 约0.30，改为用户选值；右轴 XboxTypeS_RightX/RightY 的 Axis aTurn/aLookUp 命令中原 DeadZone 改为0，保留同长 UTF-16 字符串分配，避免两层阈值叠加。

不改左轴或掩体/移动共用的控制器阈值。写前复核 UObject、属性布局、数组/字符串头和旧值；退出只恢复仍属于该对象且仍等于本会话安装值的字节，不覆盖游戏或其他代码后来的修改。切换世界后重新定位。事务测试包括中途写失败回滚、外部值修改和对象复用后的禁止恢复。

多次实际后台会话已经验证原值、应用和退出恢复5处数据，未出现写入/恢复错误；这证明运行时字段和生命周期，不替代实体手柄低幅输入阈值与手感验收。

## 默认构建基线和验证

候选从最新冻结 `analysis/fps-copy-allthreads-prefix-source` 增量加入，基线 DLL 为 GearsCaptureFpsCopyAllthreadsPrefix，SHA256 `ca66cdeac91915be17d30a5b1ff5008ae575acb6f7e60d82ff4ffb6f5f8a18c0`。没有用混有其他实验的工作区 capture.cpp 整体替换基线。每次默认集成前必须复核清单是否前进。

候选2的 verified-build 验证实际编译依赖，包括 first_person_view.h、first_person_weapon.h、first_person_live.inl 以及原双目/准星依赖。7项原生构建后测试、97项相关 Python 生命周期/相机/死区测试、79项启动器 Python 测试通过；另有打包与兼容性测试。新增两项 C++ 测试显式 /UNDEBUG，在独立新目录重新编译后实际执行断言，避免 Release 的 NDEBUG 把检查消除。C# 启动器编译及内置自测通过，离屏创建隐藏句柄检查选项布局，没有显示或激活窗口。CPU 与生命周期测试不能代替游戏图像、RenderDoc 或真实头显。

候选1的真实运行已发现并记录“按键状态切换但视点应用为0”，原因是历史武器证书；没有把该次截图当作第一人称验收。候选2a/2b在启用 VR 前被原有相机采样跨帧校验拒绝，自有游戏正常关闭，5个保护文件均通过独立恢复脚本恢复；失败记录保留，没有降低校验条件。候选2c通过私有测试入口，在存档过场结束后再启用 VR，原校验和启动保护仍执行。

实际游戏 PID36468 的候选2c会话成功：约120秒普通场景，第一人称切换2次、应用1607帧，weapon_unknown=0、rejected=0，双目发布10792张；保存六张模拟器 GPU 眼图，已检查第三人称→第一人称→第三人称的画面，持枪模型可见，没有这两张第一人称截图中的头部/肩部遮挡。运行反射记录确认当前武器为 Weap_LocustAssaultRifle（Hammerburst），最初看小图时误认成手枪，已更正。不同帧含有普通行走转向，截图不作为静态瞄具精度或后坐力对照。1项移动响应不足属于导航障碍，没有归因为 Mod 缺陷。native/XR/owner exit0，Hook 独立恢复校验、音频、5处死区数据、存档及设置恢复均通过。实体组合键、翻滚、进入掩体朝向、近墙及头显舒适度仍需继续验证，不能由该轮普通行走截图扩大确认。

补充候选2e / PID43256 在同一版普通入口通过第一人称瞄准和开火：Hammerburst 的游戏 FOV 从80变到约40.047，武器身份相同，AmmoUsedCount 从0到6；GPU 图像可见抬枪、枪口火焰、准星和回第三人称。此数值为游戏相机状态，不表示强制改变头显运行时的眼投影 FOV。快照是异步请求，记录的请求标签不保证最终读回图正好处于该时刻；本轮按实际像素检查图像，不把标签当状态证据。画面未见自身头肩遮挡；机械瞄具对线、后坐力/受击体感与命中射线精度没有由此建立。该轮也通过 native/XR/owner exit0、Hook 校验、死区5处、音频、存档和设置恢复；证据在 `analysis/playtest/first-person-preview2e-20261021/` 及 `analysis/runtime/continuous-stereo-43256-20261021-150609-c5abbcd7.*`。

已合入普通启动器：`build-first-person-preview2/GearsCaptureFirstPersonPreview2.dll`，SHA256 `045ca20af3ec7a97b40320219dbcf9ae99d880008ee3ed53e15c4191a8b49b9d`；GUI SHA256 `17777fa99938985f3c228f384aeb473c375a9ccc8dba1f666de8c10639c3c696`。具名清单 `tools/vr-launcher/manifest-first-person-preview2.json` 与默认一致，配置保留原渲染倍率等选项，新增预览启用和死区2%。独立 DLSS 旧冻结构建可以继续读取共享配置，未支持的新功能偏好不会发送给旧 DLL，`dlss_manual.py --check` 通过。回退副本、完整冻结源码差异及哈希在 ignored `analysis/runtime/first-person-preview2-promotion.json`；最终冻结源相对 ca66 基线只改 capture.cpp 和 CMakeLists，新增5个 first_person 文件。

实际证据：`analysis/playtest/first-person-preview2c-20261021/report.json`、`analysis/runtime/continuous-stereo-36468-20261021-144024-9083646d.json`、`analysis/runtime/first-person-preview2c-snapshot-actions.json`、`analysis/runtime/first-person-preview2-assertion-tests.json`。

自动运行使用经过验证的 behind-windows 游戏启动和模拟器、observe-only tiny-output、测试静音，保存并恢复当前设置及存档；不打断其他游戏实例。显式 `--gpu-snapshots` 仅获取模拟器眼图，用于本功能画面验证；普通自主测试默认继续不采图。

## 武器模型交付

本地 `analysis/weapon-models-20261021/weapon-viewer.html` 内嵌12个真实 LOD0 网格，支持旋转、缩放、侧视、俯视、瞄准轴和斜视；同目录提供 OBJ、GLB 和多角度总览。包括 Lancer、Hammerburst、Gnasher、Longshot、Snub/COG Pistol、Boltok（资源拼写 Boltock）、Torque Bow、Boomshot、Hammer of Dawn、Troika、破片手雷和烟雾手雷。

导出为灰模、计算法线，不含贴图、骨骼动画、握持变换或 VR 尺度标定。“瞄准轴”只辅助观察模型，不证明其与权威射线一致。顶点、索引、各 section 三角形数量、边界及 GLB 结构有机械校验；export-manifest、mesh-source-manifest 和 requested-asset-coverage 保存包/对象/导出哈希与检索范围。

Mulcher、Mortar、Ink 在 WarfareGame 和选定的 WarMachine、Mansion、Rooftops 三个地图清单中未找到对应网格；不能扩大为全部安装资源不存在。另外，Epic 设计总监 Cliff Bleszinski 在[2008年《Gears of War 2》开发者访谈](https://worthplaying.com/article/2008/10/20/interviews/55897-gears-of-war-2-x360-developer-interview/)的新增武器问答中列出这三种，用户清单混入了二代武器，不把它们当作本机初代必有资源继续无界查找。其余跨武器玩法数值未逐项验证，不能作为本项目实现或瞄准设计的已确认参数。游戏包及模型衍生内容均留在 ignored analysis/，不进入 VR 发行包。
