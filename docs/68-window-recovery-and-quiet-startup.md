# 窗口重绑与可见的后台预览

2026-10-15。用户要求游戏和模拟器尽量在后台启动，预览可以一直显示并由用户移到第二屏；不得通过 Win+0、主动激活、最小化或持续挪动窗口打断其他电脑操作。后台手柄复用另一聊天实现的共用启动入口，见文档67。

## VR 菜单捕获窗口恢复

SpaceRecovery4 将菜单捕获所绑定的进程和 HWND 分开。只有带启动器认证的进程创建 FILETIME 的菜单恢复模式允许重新绑定窗口；旧诊断模式继续严格绑定原 HWND。保留进程 HANDLE 并验证存活及创建时间，进程退出仍终止会话。

每250ms最多枚举一次同 PID 的可见、未最小化、至少640×360客户区窗口。没有唯一候选时暂停菜单层，保留 XR 会话；不激活窗口。每个 WGC 绑定使用独立帧邮箱，退休时先关闭邮箱再在锁外关闭 WGC；迟到回调只能关闭旧帧，不能写入新绑定。绑定代号变化会清除已有显示状态，即使尺寸相同也必须等新 epoch 的合格图像完成 GPU 上传。提交菜单层前再检查窗口、绑定代号与图像新鲜度，覆盖 GPU 等待期间的窗口关闭。检查仍非原子，不声称绝对消除 HWND 重用或最后检查之后的竞争。

12个离线 native 检查程序通过，包含新增帧邮箱迟到回调、关闭、旧图像与新 epoch 检查。SpaceRecovery4编译通过。尚未实际切换游戏窗口验证恢复，不将 CPU 检查等同于 WGC/GPU 验收。GameRecovery9与SpaceRecovery4通过独立实验清单组合，默认玩家清单仍保持GameRecovery8 / SpaceRecovery3。

## 可见且不抢焦点的模拟器

现有模拟器自动创建预览时直接调用 `SetForegroundWindow`。本地 `XR_SIMULATOR_NO_ACTIVATE=1` 选项改用非激活显示，自动尺寸调整保留位置与Z序，手动点击仍正常。连续会话没有自动关闭计时。用户在第二屏摆放窗口后，本聊天不通过脚本把它挪回主屏；进程重启后原模拟器默认位置策略尚未加入跨进程保存。

修改以可重放补丁、源码版本、依赖补丁和DLL哈希记录在 `tools/openxr-probe/simulator-quiet/`。使用私有源码副本编译，原安装DLL及focus1副本不变。GUI与SimulatorVR入口默认选择quiet1；连续运行器及独立pose工具仅给子进程设置静默环境并隐藏辅助控制台。

模拟器原有3项CTest通过；补丁正向与反向核验通过；连续运行器40项测试通过；GUI编译及参数/身份自测通过。PID3136独立测试约5.516秒，445次测试图层提交，532次前台采样与WinEvent监听均未发现该进程获取前台，首尾前台HWND相同；正常退出，未启动或修改游戏。日志在 ignored `analysis/runtime/simulator-quiet1-check.*`。这只证明一次模拟器启动不抢焦点，游戏自身后续激活行为需另外观察。

原生实验EXE SHA256：`77beb9c151a8254bf6a3b91c2b9789e6f05aaefcd84db414f264e63e26e78d96`。静默模拟器DLL SHA256：`86cc8c42a8534c976fbfe347044d6e123b5b30c9a8b106d093058a41a4428a27`。后续修改采用新构建目录，保留本轮证据。

## 游戏仍抢焦点：已核验的候选路径

用户明确反馈 PID36340 本轮游戏启动仍出现在前台。因此不能把模拟器独立测试的通过写成“游戏和模拟器均不抢焦点”。启动记录确认游戏收到 `STARTF_USESHOWWINDOW / SW_SHOWNOACTIVATE`；它是初始显示提示，后续显式激活仍未处理。当前游戏会话保持运行；本次调查没有切换焦点、发送输入或重启进程。

先查 SQLite artifact1 的 IAT 引用，再只读分析缓存与私有 IDA 副本。窗口创建/重配置函数 RVA `0x27f740` 包含 ShowWindow 调用（`0x27fefb`），特定模式分支中还包含 SetWindowPos → SetForegroundWindow（`0x27ff60`）→ SetWindowPos。调用者 RVA `0x1ef024` 在调用该函数后又调用 SetActiveWindow（`0x1ef188`）。缓存伪代码存在参数错位，具体调用以 IDA 汇编和当前 IAT 映射交叉验证；未找到这两个函数内已有的禁止激活命令行选项。

历史来源为 PID16812 代码节 SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，模块基址 `0x140000000`。PID36340 的磁盘 EXE SHA256 已核验为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`；只读比较结果如下：

- `0x27f740..0x280053` 全部2323字节匹配，SHA256 `fbc7feab3a84dec4a8bec9bec37fae754c5953aa036b6bfa9d70e01ea1959815`。
- `0x1ef024..0x1ef250` 的556字节节选匹配，SHA256 `2cbe7c887e1c50d496f68a0c7558f6c6730fd0f8b5aeed9f3edea1eb88e41923`；不声称覆盖该调用者完整函数。
- 当前 ShowWindow、SetForegroundWindow、SetActiveWindow、SetWindowPos 四个 IAT 槽均匹配该进程加载的 user32.dll 导出地址。

这些结果证明当前版本仍有相应自动激活路径，但本轮没有记录启动时的实际调用序列，尚不能断言用户看到的置前台具体来自哪一次调用。后续修正需在首次窗口显示前生效，覆盖自动显示/激活，并保留用户主动点击、切换窗口的能力；不能用反复抢回焦点或全局系统设置掩盖问题。修改后应在一次正常重启中记录前台事件验证，当前尚未实现或验收该游戏修正。

证据保存在 ignored `analysis/ida/window-startup-20261015/` 和 `analysis/runtime/window-activation-36340.json`。仅打开私有 IDA 副本，已关闭所持 lease；原数据库未改动。
