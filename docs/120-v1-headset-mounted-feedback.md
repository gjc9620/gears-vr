# 真机反馈：机枪、岔路界面与英文默认

2026-10-17。用户已结束一次短时间 Quest 3 / Virtual Desktop 测试：上机枪黑屏，下机枪恢复；岔路选择界面位置过高，需要抬头；希望第一版保留完整 6DoF。短测没有建立长时间稳定性结论。发布渠道计划为 Discord，启动器首次默认英文，保留中文选择。DLSS 兼容不阻塞第一版，现有禁用策略暂不解除。

用户现在使用电脑玩其他游戏。本轮仅离线分析、修改与测试，不启动游戏或模拟器，不抢焦点，不改变后台实体手柄策略。

## 机枪：证据与调查边界

既有脚本资料 `Engine.Vehicle.DriverEnter`、`WarfareGame.Turret.DriverEnter` 和 `Turret_TroikaCabal_base.DriverEnter` 显示进入流程经 UnPossess / Possess 切换控制角色；这些是导出的脚本资料，包含未解析 native token 和参数占位，不视作完整原生实现或本次运行证明。现有 `LiveRegistryAnchorSampler.validate_controlled` 已允许 Turret，不能简单归因于只允许 WarPawn。

用户会话 `analysis/runtime/continuous-stereo-24696-20261017-150718-1be1dbc1` 使用 VRDof60。保存的 world.jsonl 含四次注册表变化失败及两次采样截止失败，但缺少上机枪事件时间与确切内部拒绝原因，不能认定这些记录就是机枪黑屏的原因。会话报告为 incomplete，游戏退出后 native cleanup 未验证，不据此宣称本轮正常清理通过。

离线审查重点是角色切换后的认证和重绑定共同机制：mounted 采集重复解析反射信息，严格时间窗口内包含大量重复身份与父类读取；mounted 时间戳变化的错误分类也与普通相机不同。任何优化必须保留完整前后时间戳、POV、身份、反射字段和互反关系校验，不用旧画面或旧认证掩盖失败。现有实际机枪验收仍未通过。

## 岔路 UI

既有脚本 `WarPC.OnDisplayPathChoice` 经 UIController.OpenScene 打开 `UIScene_PathChoice`，并保存 `OpenedUISceneChoosePath`；`WarPC.IsPathChoiceOpen` 查询该对象。它不是单凭 DrawWeaponSlots 或准星 scope 就能完整识别的界面。当前 HUD 元素缩放只覆盖已有来源认证的绘制，尚无本次岔路画面的逐绘制归属、布局边界与双眼验证。下一步沿 UIScene 公共绘制及裁剪流程定位，避免全局下移整个 HUD 影响字幕、准星和菜单。

## 6DoF 与发布默认

`tools/vr-common/stereo_frame.h::stereo_matrices` 已将每眼位置相对追踪参考点的三轴位移按世界单位比例写入视图矩阵，旋转也独立计算；这说明实现包含位置追踪，不是新加一个 3DoF 标签即可解决。用户此前有 6DoF 正面反馈，但本次未区分旋转与平移的体验，因此保留当前实现，继续检查切换角色后的参考点和追踪有效性，不宣称本轮重新验收通过。

GUI 默认及缺失配置回退已改为英文，新便携包显式生成 `launcher_language=en`；显式保存的 `zh-CN` 仍保留。84 项 Python 检查、C# 编译与隐藏自测通过。新 EXE 位于 `analysis/vr-launcher-ui/GearsVrLauncher.exe`；根目录启动器正在被进程占用，未关闭该进程或强行替换，旧 ZIP 也未改写。此项尚未成为用户当前正在使用的可执行文件。
