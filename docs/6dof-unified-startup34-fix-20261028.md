# 统一候选 3：修复手动启动的 Playground error 34

版本：**6dof-unified-20261028-3**。固定入口仍是 `J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe`。

## 用户实际失败与源码修正

用户 2026-10-28 两次手动启动均在 `playground_reservation` 阶段返回 `Playground initialization failed: 34`。Steam EXE 验证已通过；Native 未激活，XR 未启动。旧候选及两次 first-failure 日志已完整保存到工作树忽略目录 `analysis/6dof-unified-startup34-20261028/previous-candidate/`，不把含个人路径／账户配置的原始日志放进新分发包。

`reserve_playground_hook()` 在启用 `HAND_ANIMATION_CALL_PROBE` 时，无条件调用 `handanim::test_environment()`，要求三个自动测试环境变量。这阻止了正常手动启动，属于本次已定位的明确拒绝条件。

生产源码仅修改 `tools/playground/playground.cpp` 这一处：只有明确编译了 `GEARS_PLAYGROUND_UNIFIED_MANUAL_PROFILE=1` **且**使用 `GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL` 单一 Native 调用所有者的档位，才不受这道自动测试环境门限制。旧实验档仍保留原限制。不是给启动器注入测试环境变量，也没有绕过整个 Playground。

原 EXE／进程身份、模块／注册、64 字节入口核对、relay 距离、hook 创建／激活、生命周期和正常恢复保护保留。相邻手动路径已做源码顺查：Reserve、Initialize、Enable、WeaponHandStart、VrFireEnable；R2 已有显式手动档分支，动画 snapshot／事件采集与其他实验按钮上的测试限制保留。

## 编译与配置保留

只编译和链接受影响的 `GearsPlayground` 目标，exit0。没有运行 Game、XR、模拟器、测试、自测、回放或验收预检。实际编译配置已确认包含统一手动、Native subscriber、统一 muzzle consumer 三个定义。保留原构建和修正后构建记录；尚未宣称下一次手动启动或玩法已通过。

| 组件 | SHA256 |
| --- | --- |
| 新 Playground | `4f8a5dfaab0d191d45f2867fb084a6f414172933d43d1f1487844a14620b1036` |
| 复用 Native | `458cdf929da1df7d6263c75f2adcac042cd7f4dbce077eb347b668e8322ba36b` |
| 复用 XR | `6db1b9bc72a42c4ef5a2b3d0fe99858c794e16f7206620ec4e25fe35ca53d727` |

当前用户 `config.json` 以原始字节保存并带入新候选，保留全部设置与路径历史。实际所选游戏是 `J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe`，FOV 为 65；不再用不存在的原 J 工作副本路径覆盖它。发布前再次读取当前配置，若用户期间修改，以最新文件为准。发布记录单独保留配置摘要校验值。

旧候选、日志、旧 Native／XR／PG 和回退包保留。更新不停止用户手动进程；若仍在使用，先留版本化新候选，待安全更新固定入口。

## 当前范围

上一版的第一人称、自由双手、肩部／裤兜取枪、Lancer 起点与基础方向接线保持。既有限制仍在：HUD 准星独立查询未接新射线；左手跟踪未知时暂拒开火；socket 局部 +X、实际弹着、手感及正常停止仍待用户真机反馈。本次没有新增换弹、掩体或其他武器功能。

下一步由用户从同一入口正常启动，确认本次拒绝是否消失；若出现新的实际故障，继续根据新日志修正，不要求额外自动测试。
