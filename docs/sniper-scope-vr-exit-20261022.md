# 狙击枪开镜退出 VR：材质准星绘制支持

2026-10-22。针对用户报告的狙击枪一开镜就退出 VR，补齐全屏准星材质的双眼绘制路径，通过自动检查后合入普通启动器。实体头显中的镜框、放大效果和开关镜连续性仍待实际使用验证。

## 故障证据与边界

用户现场会话 `continuous-stereo-18812-20261022-030528-8f374063` 加载 CoverAim2，native SHA256 为 `04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0`。首个 UI 错误是 ticket 96429 的 `material_overlay_plan`（28），随后出现 `owned_error=24`、`proof_failure`。该绘制的 ticket 与双眼资源关联一致，实际与预期目标相同，上传 stride=32、topology=5。这里没有发生世界恢复提交，与前一项机枪恢复边界旧姿态错误不同。

旧日志把图像姿态查找、材质类型及投影规划合并为错误 28，没有保存材质 kind 或内部拒绝原因。因此现场记录确定的是材质规划失败的位置，不单独证明内部某一个条件。

按知识库流程检索后，核验 artifact 2 的两份 `WarHUD.DrawWeaponCrosshair` / `DrawWeaponCrosshairGOW3` 保存脚本 SHA256。脚本导出显示：狙击枪处于 zoomed 状态时，在准星语义范围内绘制全 Canvas 的狙击镜材质，再绘制独立准星图标。项目将这些范围标为 kind 1；原 `material_overlay_plan` 对 kind 1 无条件拒绝。这是与症状、现场失败位置一致且可离线复现的确定性支持遗漏。脚本来源是解压 `WarfareGame.u`（SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`），仍含未解析 token，不能当作本次 native 调用轨迹。没有新增 IDA 会话或全游戏分析。

## 实现

- 在 owned Material 绘制中增加 kind 1 专用路径；普通材质及 HUD 材质的旧策略保持。
- 只支持经过原上传、命令列表、资源目标、帧编号和原生 Canvas 投影检查的全 Canvas 准星材质。新规划器要求四个不同的源角点；允许原投影的半像素偏差，不据此修改原始顶点或重新居中。
- 从当前 `owned_ticket` 的 `image_tickets` 取得同帧场景几何，使用准星图标已有的有限目标／方向投影。头显变化影响眼矩阵，不写入游戏武器瞄准状态；本项不新增武器真实射线或命中对齐证明。
- 镜框不使用普通 HUD 的元素缩放和中心位移。出视野时保留该帧目标并裁剪，重新进入视野可正常绘制；不以新的头部朝向替换目标。
- 每眼各执行一次原绘制，恢复 viewport、scissor 和深度范围，仍提交原有完成回执。类型不匹配、非全屏几何、无效资源／帧编号或坏矩阵继续拒绝。
- 增加镜框绘制／裁剪计数，以及首次失败的材质 kind、规划阶段和几何拒绝类别，便于后续现场区分错误 28 的内部原因。

## 验证与默认集成

新增 `MaterialReticlePlanTest` 的 54 组独立投影与双眼回放用例通过：三种源尺寸、半像素偏差、有限目标／无限方向、左右转头与平移、双眼中心位置、与图标共用映射、源顶点不变、正常及异常退出的绘制状态恢复。附加负例覆盖未知语义、重复／偏离角点、非零 Z、非有限矩阵、错误宽高比、多 viewport、缺失／错误几何；并验证出视野裁剪后恢复、关闭方向修正和奇数输出高度。

总计 40 项原生及启动前检查通过，其中包括既有材质规划、UI 回执、帧几何存储、第一人称、掩体瞄准与恢复检查。`OwnedGpuShareTest` 实际离屏 D3D12 双眼像素及三种资源状态恢复通过，但这是共享输出回归，不是狙击镜材质的游戏 GPU 像素验收。109 项启动器 Python 检查通过，默认文件哈希验证通过。

从 MountedResume1 的冻结源码制作候选，仅五份文件不同：新增规划器、对应测试、两份 owned Material 接线文件及 CMake 测试登记。485 项编译依赖核对及冻结源码构建后哈希再核通过，原 renderer 开关、机枪旧姿态丢帧修复、掩体瞄准和第一人称均保留。首轮构建误复制工作区完整 CMake，遇到冻结源不存在的独立实验目标，在生成阶段失败；失败目录和回执保留。最终 r2 使用原冻结 CMake，仅登记新测试，重新从空构建目录编译，没有沿用失败缓存。

默认启动器现在选择 `tools/native-capture/build-sniper-scope1/GearsCaptureSniperScope1.dll`，SHA256 `7f8e100e3144750354951e01b46348fe7c682ec080614cc78c1804d90c7ba693`。只更新 `native_build`、`native_dll`、`native_sha256`，并保存命名候选 `manifest-sniper-scope1.json`。XR、Playground 和用户配置未改。启动器首次检查发现前一默认缺少完全匹配的命名候选；本次保存正式命名候选后完整相关检查通过，没有放宽断言。

新游戏进程会加载此版本。没有启动或操作游戏、修改存档、进行 RenderDoc 或实体头显验收；没有更新发布标签／包。后续现场使用可通过 `owned_material_reticle_draws` 确认实际进入新路径。

证据、原始失败、构建日志和冻结源码保存在 ignored `analysis/sniper-scope-20261022/`、`analysis/sniper-scope1r2-20261022/`、`analysis/sniper-scope1r2-source/`、`analysis/native-build-sniper-scope1r2/`；专有脚本和日志不进入发行包。
