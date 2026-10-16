# 完美换弹与启动过渡回归

2026-10-17，Native37 / Borderless1。用户离开期间继续模拟器测试，后台实体手柄响应保持关闭。未进行新一轮头显验收。

## 独立打包检查

`analysis/release-staging/mounted37-offline-20261017` 共700个文件、43,416,766字节；独立Python的隔离导入、文件清单与依赖检查通过。包内22项启动自测通过，9个自测相关文件的前后哈希一致。原生DLL SHA256为 `66a5bba9c1de4dab5bc66c17adb6dd267b1552619255bfbc3c64fbbd40c280f0`。回执：`analysis/runtime/mounted37-packed-preflight.json`、`mounted37-packed-selftests.json`。这不是发布完成或干净电脑安装验收；此快照早于下述启动过渡修正。

## 本轮捕获的启动失败

会话 `continuous-stereo-18152-20261017-043824-862c8b97` 自动进入已有检查点后，证书准备先遇到注册表增长并重新获取，随后玩家死亡，HUD证书中的 `Controller.Pawn` 变为空。`UIConfigRejected: UI reference mismatch: controller_pawn` 被当成永久错误，原生未激活，未产生双目场景提交。

这是本轮失败的直接证据，不证明以前所有无法进入VR或黑屏都由此引起。启动预留入口已恢复，之后仅向确切游戏进程发送正常关闭并确认退出，见 `active-reload37-failed-start-close.json`。

修正提交 `87c771d`：只有已注册控制器及其反射Pawn属性的身份、偏移重查一致，且两次读到空Pawn时，才抛出专用的过渡异常并丢弃本次全部证书，进入已有的三次局部重取与共享截止时间。不会接受空Pawn证书；其他角色为空、字段／身份／代码不匹配仍直接拒绝。81项相关Python测试通过，包含死亡跨四轮准备、第五轮检查点恢复、持续死亡超时、取消及错误身份／属性／入口拒绝。无需重新编译原生DLL；实际回归另行记录。

## 完美换弹的判定依据

先用现有知识库检索，再检查离线WarfareGame脚本缓存。HUD的 `EActiveReloadResultType` 是 None=0、Success=1、SuperSuccess=2、Failed=3；`ActiveReloadSuccess` 为完美结果赋2，`DrawActionReloadBar` 在2时绘制 `PerfectGrade`。Pawn的同名结果枚举取值不同，不能混用。源包SHA256为 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`。

只读脚本 `analysis/runtime/watch-active-reload37.py` 通过当前反射属性定位 `Controller.myHUD -> WarHUD.ARBarData -> ActionReloadBarData.ARResult`，保留进程创建身份、注册表、对象／属性／类型、前后括号和双读检查；不调用游戏原生函数、不写内存、不猜Bool位。还读取现有弹药整数和可用的换弹时间浮点字段。

第一次实际只读采样取得158个有效样本，HUD结果始终0、ReloadStartTime始终0，见 `active-reload37-baseline-watch.json`；仅证明读取链可用，不是发生了换弹。需要观察新周期／复位和实际结果2，并关联有效双目输出，才可记录本触发点经过测试。50毫秒是采样目标，不保证覆盖每个游戏帧；字段名和时间值不自动证明第二次按键时机。

## 实际双目与完美换弹样本

修正后的模拟器会话 `continuous-stereo-8328-20261017-044533-c26a66cb` 实际激活原生并提交双目。准备阶段11.175秒，相机就绪至首个成功投影14.220秒，含本轮注册表增长重取；仍有明显启动延迟。这轮没有重现专用空Pawn分支，不能声称实际验证了该分支恢复。

第一次R/R尝试因整个UI栈指纹变化拒绝第二次输入，之后普通换弹完成、玩家死亡；未计为完美换弹。测试助手随后改为每次输入前两份最新菜单状态必须完整一致，且保持初始顶层游戏UI身份与所有进程、前台、反射状态保护，允许已经结束的非阻塞Toast发生变化。

第二次读取检查点后，短时采样160/160有效、无错误：`ReloadStartTime` 从0升至28.473846，HUD结果从0变为2，随后弹药从Used=4／Spare=600变为0／596。两次R实际间隔719.6004毫秒。结果2之后保存GPU票据369、源序号364、3840×2160，两眼均有场景；预览 `active-reload37-perfect-preview.png`。图中没有直接可辨的PerfectGrade文字，不以图像单独证明换弹等级；等级判断来自上述新周期反射结果与缓存脚本语义。

此后约0.46秒发生了检查点世界重绑定的 `source_reconfiguration` 过渡卡，约0.456秒后交回投影；日志明确标为world handoff，而不是仅凭时间相近归因于完美换弹。单张GPU图和抽样CSV均不能排除短暂黑帧，完美换弹黑屏不得标记为全面修复。

整轮共2126次原生发布、4843次XR投影、2124次场景复制，world generation=2。最大投影间隔79.064秒包含死亡菜单等待，不能用作游玩帧率。停止后原生清理、入口恢复、XR正常退出通过，游戏仅正常关闭并确认退出。回执 `active-reload37-runtime-audit.json`、`active-reload37-normal-close.json`。

包含Python启动修正的新独立快照 `analysis/release-staging/mounted37-startup2-20261017` 为700文件、43,418,174字节，隔离预检通过；原生／XR文件沿用此前已测试哈希。没有重复运行相同原生自测，也没有将此离线快照称为已发布版本。
