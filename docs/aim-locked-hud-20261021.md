# 瞄准状态的准星与 HUD 锚定

2026-10-21 用户后续验收反馈：准星问题已经解决，授权进入 0.0.1 打包流程。此反馈记录为用户实际游玩验收；下文保留当时自动测试的范围与限制，不将用户确认扩展为每发子弹、所有武器或所有章节的一致性证明。

2026-10-21。用户报告抬头时准星和 UI 一起跟随头部，要求瞄准状态固定到瞄准线上。此次修改显示锚点与状态传递，直接合入普通启动器；不改变枪械射击、散布、辅助瞄准和伤害。

## 定位与修改

普通 HUD / 武器 UI 的既有 `ui_hud_plan.h` 明确构造头部跟随面板。owned Canvas 路径在准星定位失败后也可能使用该面板，导致丢失原来的瞄准基准。

新版本通过每次启动/世界恢复的只读反射证书认证当前 WarPC 继承链中的 `bIsTargeting` BoolProperty。认证对象身份、属性所属类、维度、大小、offset、单 bit mask、Bool 原生分发表及访问器关系，并复用既有 Bool 代码 SHA 证据。没有复用旧进程地址或固定照抄此次字段值。HUD 查询赋值前后复制/核对这个状态，将其与同一玩家、武器、tick、时钟及图像 ticket 一起冻结，传入普通与 owned 两个几何存储。

瞄准时，`ui_aim_locked_plan.h` 将 HUD 中心放在与准星相同的世界目标投影上；有限点不可用时，两者共用原始游戏视图方向，方向不从最新头部朝向重建。UI 保持可读的面板大小及形状，围绕这个瞄准锚点布局；这里实现的是世界点锚定的 billboard，并非带真实遮挡的刚性三维面板。头部旋转/平移只改变锚点在左右眼中的投影。

普通 Canvas、owned Canvas 和 Material HUD 绘制均进入相同面板选择函数。非瞄准状态保持原 HUD 方式。已认证的纯准星分段在 ADS 时共享同一瞄准映射；几何缺失、瞄准方向位于眼后或其他无法投影的情况使用空 scissor 隐藏该次绘制，避免回到头部面板。裁切重新从保存的源 scissor 计算，不因平移前面板已裁切而丢失本应显示的区域。

瞄准点仍来自既有 HUD 查询 / 原游戏方向。这次证明显示采用相同基准，不能将其称为每发子弹最终 Impact 的追踪，也未完成辅助瞄准、随机散布与实际命中之间的全部一致性验收。

## 验证

- 43 项必要启动及原生检查通过，包括瞄准状态前后变化拒绝、Bool 元数据/访问器错误、冻结 ticket 状态冲突和已有729组坐姿投影。
- 新增1,215组头部位置、pitch、yaw、roll及目标距离组合。pitch覆盖约±34°，yaw约±23°，roll约±17°；检查 HUD 中心与独立世界点投影、准星中心一致。另覆盖细小源 scissor、有限目标位于眼后、无有限点的方向回退及退出瞄准后恢复原行为。
- Python配置25项、启动30项、世界恢复12项检查通过，共67项。
- 后台会话 `continuous-stereo-16004-20261021-001834-e0b2c352` 使用 AdsAnchor2 native SHA256 `94b73bb04b6f36d4a4bc16f84f1e95d49a1a3144792e0e41d004ef3b802b4304`、原有 MountedLatency1 XR及认证置后窗口模拟器。开镜/FOV、开火/弹药响应、暂停恢复、普通检查点重建及新世界出图均通过；记录76个状态样本、15次动作。`ui_ads_anchor planned=180 finite=70 direction=110 head_locked_fallback=0`；这是绘制计划次数，不是图像帧数或像素验收。
- 测试记录两次观察器对象身份拒绝，详情保留于 playtest report；没有将它们隐去或把全报告称为零问题。owner/XR退出码0，原生清理和入口还原通过，游戏正常退出，五份保护文件及测试静音状态恢复。

在该会话结束边界曾提交非中性模拟头姿命令，只取得 queued 回执，未观察到其在活跃运行时生效；随后提交中性姿态恢复。该尝试不算实际头动覆盖。头动回归证据为上述CPU几何测试，`headset_verified=false`、`gpu_pixel_alignment_verified=false`。用户稍后集中进行真实头显与玩法验收。

## 构建与入口

从当前默认 IntegratedFixes1 冻结源码构建 AdsAnchor2，保留已集成的黑屏、菜单、机枪切换与相机时序修复，XR继续使用 MountedLatency1。仅叠加此次 UI/准星/瞄准状态改动，没有把其他 DLSS 实验源码带入。早期 AdsAnchor1 及测试失败保留为历史记录。

普通 `GearsVR Launcher.exe` 经默认 `tools/vr-launcher/manifest.json` 选择 `tools/native-capture/build-ads-anchor2/GearsCaptureAdsAnchor2.dll`；不新增手动测试入口。运行中的旧会话不会热换 DLL，下次正常重启游戏与VR生效。配置、源码、构建、依赖、自测、游戏测试和默认集成回执保存在 ignored `analysis/ads-anchor2-source/`、`analysis/native-build-ads-anchor2/`、`analysis/runtime/ads-anchor*` 与 `analysis/playtest/ads-anchor2-20261021/`，不进入发布包。
