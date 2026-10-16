# Reticle39便携包实际运行与手枪采样

2026-10-17。使用新ZIP的独立解压目录 `analysis/release-staging/reticle39-zip-extracted-20261017`，包内Python以-I/-B启动包内launcher、Reticle39 DLL和Borderless1 XR程序；游戏安装位于包外，显式使用外部模拟器运行时，无全局XR注册表修改。诊断输入／弹药读取是仓库外部测试脚本，不是玩家包依赖。

实际会话 `continuous-stereo-11080-20261017-071905-b80e2365`，游戏PID11080／creation134366663324791980，owner PID5700／creation134366663236488050。准备3816毫秒，相机就绪至实际首投影6770毫秒。正常停止、原生清理、XR退出、游戏普通关闭通过。700个清单文件哈希均未改变。证据 `pistol55-audit.json`、`pistol55-normal-close.json`、portable55-launch.log。新包已证明本机独立实际启动；仍非干净玩家机器、包内GUI或真头显验收。

## 手枪证据与采样失败

先等待实际首投影、published及epoch提交至少10次，然后一次数字方向键切到经反射确认的Weap_COGPistol。LT／RT同一次请求保持3秒，没有另一次中性状态间隔；命令成功仅证明输入提交。只读采样保留16个有效样本，AmmoUsedCount从5到6，SpareAmmoCount保持60。随后第17项因RegistryHeaderChanged致命拒绝，completed=false；没有继续借旧证书读取。

父测试脚本没有检查watch子进程退出码，故其error=null不能表示8秒弹药观察通过。采样器自己的error／completed字段为权威。这个限制与先前步枪采样类似，后续模板必须检查子结果；本次原始回执保持不变。

GPU四份3840×2160回执，相对射击请求起点1.412／2.118／2.777／4.484秒，ticket142／162／185／248，source139／158／180／242。首图可见两眼手枪白色外圈，之后采样图没有同样可见的外圈，末图可见受击特效；不能把缺失直接归因于Mod，也没有可识别的独立内圈／外圈同帧配对。

原生记录considered／planned／applied均59，center／geometry／projection拒绝均0，finite计划及应用59、direction_fallback=0。新空scissor修正在这批记录中没有被证明触发，因此不是用户外圈错位的实际根因证据。准星融合、形状／位置及真实瞄准点关系仍须继续验证。

原图、640尺度诊断裁剪、私有运行日志全部位于ignored analysis，不进入ZIP；旧运行回执不被新分析改写成成功。
