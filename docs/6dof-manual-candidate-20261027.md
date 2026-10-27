# 第一人称与自由双手：手动候选 6dof-manual-20261027-1

本候选按用户 2026-10-27 最新要求交付。状态为**源码已实现、Native／XR 已编译和链接、已打包，等待用户物理头显反馈**。本轮没有运行游戏、XR、模拟器、自测程序、测试用例、回放或离线验收。早期原型的运行结果不算本候选通过证据。

## 启动与操作

本机候选目录：`J:/gearsVr/candidates/6dof-manual-20261027-1`。打开目录中的 `GearsVR Launcher.exe`，使用物理头显的系统 OpenXR 或 VDXR。已配置购买版游戏路径；默认渲染倍率 **0.4**，相机拉近 **0**，第一人称已勾选。不要另外叠加相机前移。进入普通可控场景后直接启用第一人称，不需要先发送测试事件。候选不含模拟器和自测 EXE。

只需检查三件事：

1. 在普通站立场景看视野是否清晰，再小幅转头、左右移动头部和真实起身／弯腰；观察旋转与真实平移是否分别正常。
2. 分别移动、旋转左右手，特别看手腕翻转、肘部弯曲及左右手能否独立活动。
3. 点击启动器“停止 VR”，等待停止完成，再退出游戏。反馈视野、双手及停止时的实际表现即可。

需要日志时使用同一启动器的“导出日志”，自行选择 ZIP 保存位置。原始会话日志在候选目录的 `analysis/runtime/`，界面日志在 `analysis/vr-launcher-ui/`。这不是要求另跑诊断或测试。

回退：先停止本候选并退出游戏，然后使用 `J:/gearsVr/GearsVR Launcher.exe`。主目录默认 manifest、已发布包、游戏文件和存档均未被本候选替换。

## 源码行为

- 第一人称：增加显式初始请求，候选启动时实际选择第一人称。身体模式移除旧预览固定前移 0.35m／抬高 0.10m；普通旧入口保留其既有行为。E+R／原有切换路径仍可交还视角；身体视角和写入也随之退出。
- 共同基准：初次写入前从已验证的 Ref115 捕获 eye、root、pelvis，保留初始实际脚部接触。之后只随 actor component 变换移动，不以持枪动画头／骨盆姿态刷新基准。真实 HMD 平移、旋转各组合一次。
- 近距离遮挡：候选第一人称使用 0.12m 近裁剪，保留原远裁剪及深度方向，不移动眼点、不隐藏整个角色。它会裁掉近裁剪内的所有几何，包括贴脸的手或场景物体；不是已证实的“仅隐藏头部网格”。此前遮挡的确切几何来源仍未获本候选真机确认。
- 双手：接入武器会话的纯变换模块 `free_hand_eye_targets.h`，使用同次 `prepared.desired_eye` 和原始匹配的 stereo／controller tick，完整位置和四元数含 roll。Key 关联进程、世界、骨架、tick／compose、会话、空间、参考修订及手柄序列，保留现有身份和新鲜度保护。
- 手臂：维持原固定骨长和既有双骨求解，当前肘部作为弯曲参考。真实手柄超出角色臂展时只把腕目标投影到可达范围，保留腕旋转，不拉长骨骼或移动身体。左右手分别记录 solved mask；失去跟踪的一侧不取得动画控制权，原版接管仍走已有单写入与恢复路径。
- XR：在最新 focus1 源码上合并已存在的 RuntimeTracked controller／continuity producer，并由正常手动入口明确打开 `--controller-input`。保留焦点诊断、菜单修复、原颜色策略。没有使用 simulator candidate 或模拟输入覆盖。
- 交付入口：只在显式 `sixdof_manual` 候选中省略自动自测，正常进程身份、目标字节检查、早期预留、生命周期、音频恢复及停止保护保留。无自动玩法脚本。

## 标识与编译

| 组件 | SHA256 |
| --- | --- |
| Native `GearsCapture6DofManual1.dll` | `322d8733291660a26445d8646d18a39d1aa32dc4f499d8d66f8b624fdd3f209b` |
| XR `OpenXRPoseProbe.exe` | `9356fff38724d42141723285cf3c90445163aedfe2a6c51fd058b504ad3666cf` |
| 原样保留的 Junker Playground | `75e54302ad97e5e39e4dc9e77e74d89a90278bdbc9b8c07e38d2f6f2aaf61581` |

Native 以最新 Junker／Support3 冻结源码为底，合入已有身体原型，保留独立左眼历史。A 先编译和链接 exit0，再接 B；最终 Native exit0。XR 在同 menu-modal1 父版上与 focus1 三方合并，物理 producer 五文件字节与已有 BQ 源一致，Release x64 编译和链接 exit0。仅构建 GearsCapture／OpenXRPoseProbe 目标，没有运行它们或任何测试目标。

本地构建记录位于工作树 `analysis/body-manual-candidate1/`：`integration.json`、`build-defines.json`、`build-a2/build-a-corrected.log`、`build-a2/build-b1.log`、`build-a2/build-final1.log`、`xr-build1/corrected-0.log` 和 `corrected-1.log`。保留首次配置／编译失败记录。`final-source/` 保留实际候选源码，包内 `inventory.json` 列出全部交付文件。

## 待反馈与范围

尚未确认本候选的实际启动、清晰双眼画面、持续双手表现、物理头显或正常停止；编译成功不代表这些通过。后续射击、换弹、LGrip 掩体拖拽和完整跑步稳定性验收保持待办。原完整交互需求没有取消。

本候选交付后暂停功能推进，等待用户真机反馈。先前首失败诊断保留但冻结，不作为交付门槛，也不增加完成度百分比。
