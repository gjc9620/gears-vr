# 初始启动解析合并

2026-10-17，源码提交 `7b5420d`。本轮只改Python启动准备，DLL保持Reticle39，不重新构建原生Hook。

## 改动与边界

原始start_capture.main先用resolve_camera_once完整解析相机并关闭Resolver，再进入acquire_observer_preparation新建Resolver、完整解析一次。前一结果仅提供local_player_address／module_base比较值；Anchor已接收prepared_camera，没有额外隐藏的resolve。

现在初始启动由证书获取中的首次成功完整解析建立比较基准，且在任何证书构建之前锁定这两个值。该基准跨内部注册表重试及外层wait_camera_preparation重试保留，每次尝试仍新建Resolver、完整解析、重建全部证书、校验入口与最终读取。显式传入camera字典的其他调用者保持原有比较行为；没有拿UI证书替代Crosshair证书，也没有跳过全局类唯一性扫描。

此变化移除了单独的首次观察，基准从原第二次解析开始。因此并发拒绝时序不完全相同，不能称与旧版本逐观察等价。模块／预留DLL／priming／active-event检查先于新的首次解析；它们不写入或激活游戏。所有配置写入与远程激活仍在新鲜证书成功之后。

新增测试覆盖首次解析失败不锁基准、内部三次注册表失败及外层重试、玩家／模块变化硬拒绝、取消／截止／迟到成功拒绝、每次Resolver关闭、main没有前置resolve且Capture INI使用实际新鲜字段。主执行代理报告189项准备／证书／启动自测／重绑定相关测试通过，另8项sample_player_anchor通过；协调器独立重跑33项聚焦测试通过，记录startup56-focused-tests.log。

## 实际运行

会话 `continuous-stereo-39424-20261017-072454-bca850b9`，新游戏PID39424，完整创建身份见startup56-normal-close.json。首轮新鲜准备3279毫秒，相机就绪至实际首投影6282毫秒；正常停止、原生清理、XR退出、游戏普通关闭通过，保存两份双目GPU图像。审计startup56-audit.json无错误。

此前Reticle39普通启动准备3795毫秒、独立包启动3816毫秒，对应相机就绪至首投影约6771／6770毫秒。新样本少约0.5秒，但进程／加载条件不同且只有一轮，不能宣称稳定收益或按比例推广到整个启动过程。检查点重绑定不调用该初始启动路径，不能把本改动当作检查点恢复提速。

现有Reticle39测试ZIP仍是改动前Python快照；默认仓库启动已使用新代码。新包更新、独立部署验证及更广泛场景仍须分别处理。
