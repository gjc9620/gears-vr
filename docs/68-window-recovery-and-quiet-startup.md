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

## 提前安装策略：小程序验证

新增 `tools/quiet-startup/` 独立实验启动器。它只创建自己的新进程，在恢复 EXE 入口断点并脱离调试后，暂时保留主线程的暂停计数，加载进程内窗口策略并读取安装状态，再释放主线程；失败时结束未释放的新实例，不回退普通启动。磁盘 EXE 不变，已有游戏不受此工具操作。图形/相机 Hook 和默认 VR 构建均未修改。

最初仅设置临时 WS_EX_NOACTIVATE 的创建方式仍改变线程活动窗口，产生前台事件；隐藏和最小化还暴露间接激活路线。保留这些失败证据，没有放宽通过条件。quiet5 改为后台顶层窗口先隐藏创建、再非激活显示，并使用无激活隐藏/最小化操作。八类自动窗口调用只在所属进程位于后台时受限；用户主动激活后的调用继续原路径。

quiet5 小程序实际检查通过：ANSI/Unicode创建、父子/owned窗口、隐藏/最小化/恢复、延后激活和置顶请求，窗口可见且未保留新增的 NOACTIVATE 位；全过程未记录所属前台事件，首尾前台 HWND 相同。缺失 DLL 在创建前拒绝；无效策略的已创建进程实际退出。适配器和后台控制相关27项 Python 测试通过。该结果尚不是游戏、人工点击或 Alt-Tab 验收；当前只通过显式 `--quiet-build` 或子进程环境 `GEARSVR_QUIET_STARTUP_BUILD` 启用。

冻结构建 `build-quiet5`：启动器 SHA256 `2f1f125ebd50ea30de22b45c0542dd635fecc493913daf8d51f57a91850a7e60`，策略 DLL SHA256 `74a5ff4abab225035e3cfcd55ba2f7160ed7b1e9f458a98d778ac0c62e951782`。实际事件/时间线与失败清理证据在 ignored `analysis/runtime/quiet-startup5-*.json`。游戏启动观察须覆盖 EXE 入口之前的 TLS/DLL 初始化，不将“安装成功”当成“从未抢焦点”。

## 9j实际启动失败与交换链编号冲突

2026-10-16复核：PID34196、创建FILETIME `134365531792435533` 的quiet5启动报告确认入口已恢复、调试器已脱离、策略已安装且入口前可见窗口为0。但90秒观察记录到2次游戏前台事件、2880次采样中1482次游戏位于前台，不能通过不抢焦点验收。观察器没有停止游戏；前台事件尚未区分人工切换与自动激活。后续查询显示八个user32入口仍跳转到该策略DLL，确实拦截过ShowWindow 3次、SetForegroundWindow 1次、SetWindowPos 3次、CreateWindowExW 1次；“Hook安装失效”暂无证据。

这一会话的VR失败已有单独的运行时原因：模拟器日志 `openxr_simulator.32756.log` 依次记录菜单交换链2（1920×764）、替换菜单交换链3（1920×1080）、销毁2，随后两个960×1080眼交换链均再次返回3。模拟器按 `g_swapchains.size()+2` 分配编号，并未检查emplace结果；删除较小编号后发生冲突，枚举返回了仍存活的菜单纹理。Native源1920×1080且resize计数全部0，published=1；XR因尺寸检查失败而停止，submitted_frames=0。该证据解释本轮退出，不能泛化成之前所有黑屏的原因。

SpaceRecovery5在枚举交换链时就检查所有实际D3D12纹理尺寸，并在异常时输出眼/图像索引、交换链、请求与实际尺寸；取得图像后的原检查仍保留。没有修改渲染尺寸、缩放策略或放宽保护。模拟器编号修正另存quiet2，并保留quiet1作为失败对照；实际集成结果另行记录。默认原生/桥接清单仍未提升。

证据：ignored `analysis/runtime/quiet-game9j.json`、`quiet9j-policy-query.json`、`continuous-stereo-34196-20261015-235320-d2080fda.*`。查询仅调用已加载策略的状态导出，没有重新安装Hook或改动焦点。

用户随后确认9j期间曾手动切换游戏，因此上述前台事件不能直接认定为自动抢焦点。该轮缺少明确的事件归因，不判为自动激活失败，也不据此新增焦点Hook；实际静默启动仍待观察。

## quiet2对照测试与9k实际运行

同一独立D3D12测试对照quiet1/quiet2：quiet1的eye_left再次得到handle3，实际三张纹理均与菜单相同（1920×1080、format87），退出码1；quiet2的眼链4/5返回独立960×1080、format24纹理，销毁/重建眼链后编号6/7与新尺寸也正确，退出码0。测试没有BeginSession/EndFrame或预览窗口。另有4项CTest通过。可重放补丁、来源及边界记录在 `tools/openxr-probe/simulator-quiet/swapchain-handles-provenance.json`。

9k使用Native9、SpaceRecovery5、quiet2与quiet5启动策略。PID6680（创建FILETIME `134365543832524611`）、XR35036实际再次走菜单2→替换菜单3→销毁2→眼链4/5；本次成功提交投影。90秒观察结束时owner/game/XR均存活，观察器未停止任何进程。首个实际投影标记在XR运行器开始后31.654秒，初始化延迟仍存在。00:16时记录13349次投影提交，native另一次采样published7641、anchor matched7654/rejected0，说明会话持续，不代表每个画面内容都正确。

本轮屏幕内容检查处于暂停菜单，存在左右背景差异；菜单路由继续单独调查，不把成功投影写成场景/菜单完整验收。前台记录仍可能包含人工操作，不由最后输入时戳推断具体输入内容或宣称完全静默。GUI及SimulatorVR入口已改用quiet2，GUI编译、自测通过；默认原生/桥接清单保持不变。

SpaceRecovery6另增加两眼交换链handle重复拒绝，构造清理只保留一次所有权；编译及CPU检查通过，9k实际运行的仍为SpaceRecovery5，未热替换。该实验构建SHA256 `5ad83e2be9ee708358e152480f428d5673e685414506abc2efd11f4322733251`。所有运行记录与截图仍留在ignored analysis。

## quiet6：修正 NoPauseUnfocus 的引号

用户确认前台记录曾包含手动切换，因此不由9j/9k事件直接推断自动抢焦点；随后明确实测失焦仍会暂停。后台手柄补丁只处理控制器焦点过滤，不能代替自动暂停策略。

9l（PID35456/创建FILETIME `134365563390630203`）确实收到 `-NoPauseUnfocus` 启动参数，但只读验证游戏内部命令行是 `"-NoPauseUnfocus"`，包含首尾引号。quiet5的 `early_launch.cpp` 无条件给每个参数包引号。知识库找到tick函数RVA `0x1e3f28` 的开关检查与 `ConditionalOutOfFocusPauseGame` 调用；整个3064字节函数在历史快照和本次进程均匹配SHA256 `f31e4dc62da150887df414dda7028e8ac4177295438d559e0d57a8afdcbe26e9`。

解析函数RVA `0xc2f44`（含独立chunk `0x899626..0x89967c`）只接受关键字后为NUL、空格或tab；末尾双引号不满足条件。`0x245124` 的汇编确认只识别空格/tab。条件跳转 `0x1e41ec` 在开关有效时跳过专用失焦暂停路径，未涉及主动暂停入口。原始快照的伪代码/汇编文本有raw-loader数据地址偏差，因此运行期地址由指令的RIP相对位移计算：内部命令行 `base+0x1cdc190`、开关字面值 `base+0x15ace58`，不直接照搬伪代码常数。

quiet6仅修正命令行构造：非空且没有空格/tab/双引号的参数保持原样，需引用的参数继续正确转义。不修改游戏参数解析器、暂停分支、全局焦点或磁盘EXE。新 `QuietCommandLineTest` 对照原始开关文本并通过Windows argv往返边界测试；编译、CTest和非激活窗口fixture均通过，fixture没有所属前台事件。游戏实际对照另记，不将fixture当成游戏验收。

证据：ignored `analysis/ida/unfocus-pause-20261016/`、`analysis/runtime/unfocus-pause-35456-read.json`、`quiet-startup6-fixture.json`。IDA只使用原库的私有副本，持有lease已关闭。

### quiet6 实际游戏对照

9m使用同一Native9/XR7/quiet2，仅启动器改为quiet6。game35136（创建FILETIME `134365581446475980`）、owner41452、XR41552。内部命令行变为裸 `-NoPauseUnfocus`；tick、参数解析函数的两个chunks及空白判断函数均与历史快照字节完全相同，证明未通过暂停代码补丁改变行为。新启动器SHA256 `15cffb4832a473f621125f0057be87eb59ae2357eaaea3b347cf91c9aec4c5bd`，策略DLL SHA256 `3bcc353553deeb2f5245bd2e6e9b193160f2e97956e9b3573e4cd6dfad89ec19`。

初始30秒观察游戏始终在前台，故不算后台通过。协调线程随后只将认证游戏窗口最小化、再无激活恢复，未调用激活API；前台实际从game35136移至XR41552，游戏窗口位置/尺寸恢复原值。随后30秒60次观察全部为后台GAMEPLAY，Pause/UNKNOWN/错误均0，native published6660→8159。

同一后台进程经认证状态后发送一次Escape，正常打开Pause并提交共同菜单quad；随后在认证Resume按钮上发送一次Enter，正常恢复投影。恢复后再观察30秒，60次全部后台GAMEPLAY，Pause/UNKNOWN/错误均0，published11848→13348。额外只读反射WorldInfo：Pauser持续为空，TimeSeconds在约1秒内从335.571716推进到336.571808；未仅以渲染帧增加认定游戏时间在运行。

这支持quiet5错误引用开关是本轮失焦暂停的原因，quiet6修正有效；不扩大到所有版本/模式。无人值守9m启动观察仍记录一次游戏进入前台，静默启动问题没有因此解决，quiet策略仍为显式实验启用。观察器结束不停止连续会话。

证据：ignored `unfocus-pause-35136-read.json`、`unfocus9m-observe.json`、`unfocus9m-focus-loss.json`、`unfocus9m-background.json`、`pause-menu9m-open.json`、`pause-menu9m-resume.json`、`unfocus9m-after-resume.json`、`unfocus9m-world-tick.json`，均位于 `analysis/runtime/`；会话前缀 `continuous-stereo-35136-20261016-011605-c5356ffd`。
