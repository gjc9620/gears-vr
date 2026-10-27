# 统一候选 4：修复 Capture 配置重复键导致的会话退出

版本 **6dof-unified-20261028-4**，固定入口保持 `J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe`。

## 用户实际记录

v3 手动记录 `continuous-stereo-60532-20261028-021200-e9bfcdb8.json` 的顶层错误是：`Capture` 节的 `first_person_preview` 已存在。UTF16 `capture.ini` 第 22、55 行各写了一次 `first_person_preview=1`。`world_rebind_runtime.RuntimePreparation` 使用默认严格 `ConfigParser` 读取时抛异常，控制器随后清理会话。

这次 Native 已激活、PG 已启用并读到 `level_player`，XR 随后正常停机，Native cleanup 已确认；说明上一版的初始化 34 已越过。它不构成画面或头显通过证据，也不是当前已证实的游戏原生 crash。

修改前，完整 v3 候选、用户这次日志、实际失败 INI 和用户配置已保存至忽略目录 `analysis/6dof-unified-duplicate-ini-20261028/previous-candidate/`。个人原始日志不放入新分发包。

## 唯一配置来源

两个生产 Python 文件有窄修正：

- `start_capture.hud_size_ini` 继续作为 `first_person_preview` 的唯一写入方，手动档明确使用 True，仍通过原有连续 stereo、anchor、cinematic、weapon observer 条件检查。
- `sixdof_manual_profile.CAPTURE` 移除重复的该键，只追加 `first_person_initial_requested`、`first_person_body_basis`、`first_person_near_clip_mm` 和 `sixdof_unified` 四个专属键。

第一人称仍为 1，同次眼／身基准和现有交互配置保留。没有设置 `strict=False`，没有忽略矛盾值，也没有跳过世界恢复。`world_rebind_runtime.py` 未改。

对现存实际失败 INI 的键／节逐行检查，只发现上述重复项，无其他重复键或重复节。对两个源文件作 AST／常量检查，专属键及附加节没有内部重复；没有执行配置生成器或运行测试用例。这不是对所有未来参数组合的运行验收。

## 打包与保留

纯 Python 修正，三个已存在的二进制完全复用，**没有无故重编译**：

| 组件 | SHA256 |
| --- | --- |
| Native | `458cdf929da1df7d6263c75f2adcac042cd7f4dbce077eb347b668e8322ba36b` |
| XR | `6db1b9bc72a42c4ef5a2b3d0fe99858c794e16f7206620ec4e25fe35ca53d727` |
| Playground，含 init34 修正 | `4f8a5dfaab0d191d45f2867fb084a6f414172933d43d1f1487844a14620b1036` |

用户当前 `config.json` 全部原样带入，保留实际 SteamLibrary 路径、路径历史、FOV 65 及其余设置；发布前再次读取最新文件。包的 manifest、portable metadata、inventory 和版本身份同步更新。旧候选和日志保留；不停止用户进程，在用时先保留版本化候选。

未调用 Game、XR、模拟器、测试、自测、回放或 preflight。新版本下一次实际启动、出图、手感和停止仍由用户确认。原四步实现及其限制保持：HUD 准星未接新射线、左手 unknown 时暂拒开火、socket 局部 +X 等待方向反馈。本次未扩展新功能。
