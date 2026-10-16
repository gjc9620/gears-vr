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

## 启动前台切换的隔离：9n / 9o / 9p

9m通过正常停止流程结束，连续会话记录约898秒、79369次投影提交；最大提交间隔包含主动Pause实验，不能作为连续帧率结论。最后检测器未新增非对称明暗或预览绘制失败，唯一事件仍是主动Pause的无projection阶段。未扩大为所有CG/检查点/真实头显通过。

SpaceRecovery8仅在已有 `--shutdown-diagnostics` 开启时增加初始化边界的GetTickCount64及前台HWND/PID，不修改渲染或焦点行为；SHA256 `96765985d77410af2ccefed89622506a914c0f63a7f7616d49b4f1e12a8b4a62`。9n的game34216在D3D12CreateDevice调用区间内进入前台（事件tick25660781），早于XR session与WGC创建。但这只是时间相关，后续对照未支持该API就是原因。

9o在测试子进程环境中通过各层manifest声明的disable_environment请求关闭5层隐式OpenXR插件；没有改注册表或全局设置，未独立保存该轮加载模块复核。game25576仍进入前台（tick25856343），这次处于WGC初始化区间。独立、不加载OpenXR的D3D12小程序没有改变前台。二者均不能单独定位调用方，也不据此禁用玩家的插件。

更小的9p只使用quiet6启动游戏和裸NoPauseUnfocus：没有启动VR、native capture、后台手柄helper，也没有发送菜单输入。game11148（创建FILETIME `134365595413258232`）仍在启动后约21秒进入前台（tick26033531）。因此自动激活不依赖本Mod的双目、OpenXR会话或WGC路径；后续用这个轻量场景定位现有静默策略遗漏，不继续从XR时间相关性推断根因。

证据在ignored `analysis/runtime/quiet-game9n.json`、`quiet-game9o.json`、`quiet-game-only9p.json`、`device-focus9n.txt`及对应连续会话stderr。9n/9o均正常停止VR后以WM_CLOSE退出已认证游戏。诊断构建不提升默认发布清单。

### quiet7 / quiet8：有界调用与消息记录

9p实际检查确认8个user32入口仍指向已加载的quiet策略，不能解释成Hook被移除。quiet7新增显式环境开关 `GEARSVR_QUIET_TRACE=1`，只在内存保留首128条API记录；包括放行、NULL、参数、前后前台/tick及24帧裸栈。快照导出只复制release/acquire已完成槽，不等待、不覆盖旧槽，原Status ABI保持8个计数。quiet8在同一显式开关下增加DefWindowProcW/A的5类激活消息观察，原样转发，默认不开这些诊断Hook。编译、CTest、追踪开/关两次离屏fixture通过；fixture还检查饱和、NULL和历史槽不变。未把fixture等同于游戏通过。

9q（game40344）得到22条完整记录、overflow0/busy0；9r（game40384）得到25条完整记录、overflow0/busy0。前台事件前，ShowWindow、SetWindowPos与SetForegroundWindow均按策略处理，返回时仍为原前台。9r的WM_NCACTIVATE和WM_ACTIVATE抵达默认过程时游戏已经在前台；没有观察到WM_SYSCOMMAND，因此尚不支持“拦截默认恢复消息即可修复”。

9r实际消息栈为游戏RVA `0x27ffa4` 调用 `0x1f9d74` 消息泵，在 `PeekMessageW` 内交付激活消息。64字节消息泵与历史快照完全匹配SHA256 `59e99684e64b44e882a105296235dd855920241b1528ec8a3b33dc17e26a3b24`；通过实际RIP位移读取IAT并匹配user32导出确认PeekMessageW。9q窗口过程RVA `0x1995d0` 的1866字节同样匹配历史SHA256 `f8aaedc4525ad22bcae4c49a237b777f1742f218bc1c53d3ba0803dd12b0a617`。当前Windows的DefWindowProcW转发到NTDLL.NtdllDefWindowProc_W，不能按user32模块范围误认未知函数。

这证明激活消息在取消息时被交付，尚未定位请求来源；不通过过滤WM_ACTIVATE或循环抢回焦点掩盖问题。证据：`quiet9p-policy-query.json`、`focus-trace9q.json`、`focus-trace9r.json`、`focus9q-wndproc-evidence.json`、`focus9r-pump-evidence.json`、`focus9r-startup-imports.json`。quiet8 DLL SHA256 `d24bf2da8bde8017326f540c782eaf2fba119cec4504758b291a3d9246ec17cc`。未修改游戏磁盘代码、未打开新的IDA数据库。


### 默认入口与辅助窗口补齐（2026-10-16）

`bdc323f` 将 `background_launch.py` 的普通启动默认接入冻结的 quiet8；校验启动器与策略 DLL 的 SHA256，缺失或变化时拒绝启动，不降级为普通前台启动。显式 `--foreground` 仍可选择普通启动。16 项启动测试通过。这是默认接线修正，不表示上面的 9q/9r 激活遗漏已解决。

`2112cd4` 为主 launcher 的 reservation、进程查询、native preparation/verification 与 native 自测/PowerShell 辅助调用补齐 `CREATE_NO_WINDOW`；不会隐藏游戏或模拟器的 GUI。XR 子进程环境固定 `XR_SIMULATOR_NO_ACTIVATE=1`，保留父进程环境和 `XR_RUNTIME_JSON`。相关 144 项 Python 测试通过。连续会话没有默认 duration 截止，启动/清理超时不等于显示时长。用户要求模拟器持续可见；目前预览依附 XR 进程，会话停止或重启时仍会关闭，不能表述为跨进程重启常驻。


### 2026-10-16 后续实际观察与当前默认

`8412253` 将默认构建更新为冻结的 quiet11，并对子进程显式启用主窗口 NOACTIVATE 策略；保留 APPWINDOW、原窗口位置/分辨率，foreground 显式启动仍绕过。36 项 debug-launch 测试通过。quiet11 是 `2dc0271` 源码阶段的冻结构建，后续 quiet12/13 仅为实验，不覆盖该构建。仍未达到“所有自动启动均不抢焦点”的实际验收，不能把默认接入等同于根因已解决。

- `quiet-game-only9t.json` 的游戏前台事件，用户已明确承认手动切换；不作为自动抢焦点失败。
- `quiet-game-only9u.json`：输入时间在整轮未变化，实际 extension-point mitigation flags=1，游戏仍自动激活。AppHook64 与 nvspcap64 仍存在，模块差集主要是 IME；没有实现对 DisplayFusion 的完整隔离，也未修改其设置或注册表。
- `quiet-persistent11.json`：自动导航遇到视频设置确认/选项页后超时。已停止 XR，owned prime 取消并验证入口恢复，随后正常关闭该轮游戏；该轮不是持续运行通过。
- `quiet-persistent13.json` / `continuous-stereo-16104-20261016-150650-701eca87.json`：取消自动导航、保持菜单等待，无默认显示时长。模拟器窗口可见，运行时帧计数递增；阶段为 awaiting_level，尚未激活关卡原生双目。前台事件混有大量人工输入，不能单凭目标事件归因。

只读查询 `focus-persistent13.json` 显示，新增 NtUserSetForegroundWindow 实验入口首次调用发生在游戏已经前台之后，并未捕获到最初激活请求。其余四个相关 win32u 入口的运行字节与磁盘相同，见 `focus-persistent13-win32u-bytes.json`。不继续沿“该入口被绕过导致初次激活”作无证据修改。

复用知识库后，已将 `RVA 0x197178 / 0x27f740 / 0x2815f0` 的全部 chunks 与当前游戏逐段读取、拼接 SHA256 比较，全部匹配，证据 `focus-persistent13-kb-functions.json`。前者是修饰键清理，不能跳过；主动窗口显示/置前块已被现有策略覆盖。下一处定向分析为已有运行栈上的视口/RHI 更新与 restore 路径，仍是待验证假设。仅打开了私有 Windows user32 副本，确认 SetForegroundWindow 是转发到 win32u 的跳转；IDA lease 已关闭，原件未改，衍生数据保留在忽略的 analysis 下。


知识库后续定向结果：`452ADC / 452BB0` 与当前运行代码 SHA256 同样匹配；后者负责已观察到的 `ShowWindow(SW_RESTORE)`，该调用仍经过现有 guard。尾部 IAT `base+0x131c9a8` 实际解析为 `user32!PostMessageW`，参数15对应绘制消息，不能视为激活，见 `focus-persistent13-rhi-constructor.json`。历史缓存 `453468` 呈现 CreateSwapChainForHwnd / Windowed=1 / MakeWindowAssociation(1) 的布局，尚未运行验证接口与实参，未支持“安装默认全屏就是根因”。

另由 WINDOWED 字面量引用定位到 `14D844` 初始化函数，缓存支持命令行 `-windowed` 确实被解析、FULLSCREEN 可优先覆盖；参数反编译错位，不能直接按结构偏移补丁。可在下一次正常重启时进行仅增加 `-windowed` 的对照，不改 INI 或扩展 Hook。当前常开菜单会话继续保留，不为这项静态分析重启。

### 默认 quiet11 的实际失败与窗口化对照（2026-10-16）

`quiet-default11-final1.json` 使用正式启动链、默认冻结 quiet11、显式主窗口 NOACTIVATE 与 Native23/XR16。70秒观察记录一次 game24640 前台事件，tick77508781；初始输入 tick77391093，首个后续输入 tick77530484，因此这次自动激活不能归因于人工切换。辅助进程无所属前台事件，模拟器可见；观察结束保留 owner/game/XR。36项 debug-launch 测试通过，但不覆盖此实际遗漏。

`focus-default11-final1.json` 保存27条完整记录、无溢出：ShowWindow、两次 SetWindowPos 和 SetForegroundWindow 均经过 guard，返回时前台仍为原应用；随后 PeekMessage 内交付 CBT/激活消息。这确认 quiet11 尚未完成不抢焦点目标，不能把历史人工切换解释用于这轮。

只增加 `-windowed` 的启动对照见 `quiet-default11-windowed1.json`、`background-launch-35236.json`、`focus-default11-windowed1.json`。真实命令行包含 `-windowed -NoPauseUnfocus`，仍于 tick136484 自动进入前台；此前最后输入126718、此后首次输入139359，调用顺序同上。故显式窗口化未解决本机启动激活；实验性 PS 参数已撤回，未保留为默认修复。两轮间系统 tick 重新计数，不跨轮比较绝对时戳。窗口化轮保留游戏35236与XR34992、等待关卡，无自动菜单输入。下一步需定位现有 guard 之外的激活请求来源，尚无证据确定为 DisplayFusion、显卡覆盖层或某个 RHI 调用。

### quiet14：排除本进程 ShowWindowAsync 入口

仅在显式 TRACE 开启时增加 `ShowWindowAsync` 原样转发记录（ID11）；`extra_argument` 保存窗口所属线程，已有8个Status计数和快照大小保持不变。未修改首次最小化行为：首次 ShowWindow 受 STARTUPINFO 影响，命令2被改7并不证明实际 IsIconic。未提升默认 quiet11。

quiet14 构建、命令行CTest通过；实际hook fixture的 trace_valid 检查通过，覆盖异步显示原参数、窗口线程和返回值。两次fixture整体不通过，因为各有一个短暂前台事件；旧冻结quiet11同条件对照也出现，不能将此结果写为无焦点回归通过。证据 `quiet14-on[-trace]-fixture.json`、`quiet11-control-after14.json`。

实际 `quiet-async14.json` / `focus-async14.json`：game34536仍出现前台事件tick517078；策略已安装，27条完整记录、无溢出，ShowWindowAsync记录为0。该结果排除了这轮激活前通过被监测进程内该入口发起的请求，不能排除跨进程调用或其他API。会话 `continuous-stereo-34536-20261016-162343-825736d9.json` 在观察结束时继续等待关卡，owner30912、XR2492持续运行，模拟器可见；不关闭会话或通过抢回焦点掩盖事件。

### Win32k 系统跟踪能力检查

本机 WPR/xperf 已安装。非管理员启动 WPR 实际返回 `0x80070005`；用户随后手动运行管理员采集脚本。`Record-Focus.ps1` 使用独立实例名、5–180秒时限及 stop 文件，正常停止保存 ETL；停止失败时只取消自己创建的实例。不会启动/关闭游戏，也不会设置前台窗口。`-CheckOnly` 和 WPR profile 解析通过；非管理员及越界时长均在采集前拒绝。

首轮管理员实例 `GearsVRFocus-bcca9daf7cb4413bb6e6b09a66b6c62e` 经 logman 确认实际运行（非管理员 WPR status 未显示该实例）。记录覆盖 game13348 的实际启动，随后主动请求保存，stop exit0，ETL 524288字节。xperf导出显示133秒、事件/缓冲丢失均0，但没有 Win32k 焦点事件，仅系统元数据；因此不能用于来源归因，也不能宣称ETW路线已验证可用。

第二版 profile 使用 logman 查询确认的 GUID `8c416c79-d49b-4f01-a467-e56d3aa8234c`、Focus/UserActivity mask `0x22000`、NonPagedMemory及Strict启用。仍需重新采样验证事件实际出现；名称解析或buffer选项是否解释首轮空数据尚未证明。采集结果全部保存在ignored analysis，不进发布包。窗口操作对照和调用方归因尚未完成。

### Win32k 有效采样与归因边界

后续实例 `79dae3652d7d439e85da851fdabab248` 成功采到 Focus/FocusedProcessChange 及调用栈，正常保存退出0；该轮仅验证采集能力，没有重启游戏。实际启动实例 `a2a877fd0b8948a1a3c8c365d25c6ff9` 也正常保存退出0，导出18条焦点相关事件。新前台进程15296的事件位于49210913微秒，事件头PID15296/TID30436，栈39帧。与 `focus-etw14-valid.json` 的模块范围对照，用户态栈为 win32u+0x12e4、user32+0x119bf/+0x11948、游戏+0x1f9d93/+0x27ffa9，延续已观察到的消息泵路径。

该证据确认目标线程处理激活的位置，不能将事件头PID当成最初发起异步请求的进程。继续解析这份内核栈不能可靠还原此前排队请求的发起者，因此停止这条归因路线，不再要求用户重复管理员采集。当前没有证据确认 DisplayFusion 或显卡覆盖层是根因。

下一项候选是外部组件来源隔离。只读检查确认 DisplayFusion 正在运行，游戏加载 AppHook64；本机 Compatibility/Triggers 键未读到规则。官方支持说明单应用 Disable Application Hooks 并非完全禁用该应用的所有 DisplayFusion 功能，不能用该设置冒充完整隔离。尚未修改其配置或关闭该软件；短暂退出整个 DisplayFusion 会影响用户桌面功能，需要先取得该范围的同意。当前游戏15296保留运行。

### DisplayFusion 临时退出对照：隔离成立，焦点归因未定

用户明确授权临时退出并恢复后，正常停止旧会话，验证 prime 入口恢复及 XR shutdown，再向身份核验后的 game15296 发送 WM_CLOSE。通过 DisplayFusionCommand -CloseAll 正常退出，确认主进程与32/64位hook辅助进程均不存在，再沿同一 quiet14/Native23/XR16 链启动 game15648，观察70秒且无自动导航。恢复前模块列表不含 AppHook/DisplayFusion，仍含 nvspcap64；恢复前 DisplayFusion 进程仍为空。finally 中重新启动 DisplayFusion，并确认主进程存在，未修改其配置。游戏和模拟器保留运行。

`quiet-df-isolation.json` 记录 game15648 前台tick1874546/1877109、XR14796前台tick1883390；此前最后输入分别1874515/1877000/1883343，距离仅31/109/47毫秒。用户回答记不清是否手动切换。因此本轮不能判为自动激活失败，也不能判为不抢焦点通过，更不能据此认定或排除 DisplayFusion。`focus-df-isolation.json` 保存27条完整记录、无溢出。控制及模块证据为 `quiet-df-isolation-control.json`，全部位于ignored analysis。若继续做因果对照，需要能区分人工切换的观察窗口。

### quiet15 首次显示参数单变量实验准备

有界专家复核发现：default11-final1 主窗口创建时不带 WS_VISIBLE，首个 ShowWindow 请求2（guard改7），而启动器 STARTF_USESHOWWINDOW 指定4；约19.313秒后才有恢复请求。首次显示参数可能覆盖游戏初始化期间的最小化状态。此为待验证假设，不能据此宣布根因。现有fixture先创建可见dummy，未覆盖这一首次显示顺序；其SC_MINIMIZE阶段的ANSI激活事件也不能替代游戏实际抢焦点证据。

early_launch新增显式诊断环境变量 `GEARSVR_QUIET_START_MINIMIZED=1`，仅把启动hint从4改7；未设置或其他值仍为4，默认冻结quiet11不变。`build-quiet15-minstart` 编译及命令行CTest通过。无窗口探针经真实early-launch启动、沿用冻结quiet14策略DLL，验证默认/1/无效值三种实际STARTUPINFO分别4/7/4且flags含STARTF_USESHOWWINDOW，证据 `quiet15-startup-contract.json`。尚未运行游戏对照，不代表焦点修复通过。下一轮需同一DisplayFusion状态与无人工输入窗口，外部记录IsIconic确认最小化/恢复实际成立；不能只相信传入参数。

### 无人工输入的 DisplayFusion 隔离：仍自动抢焦点

用户明确提供无输入窗口后，重复原quiet14参数的隔离测试，未启用quiet15最小化实验。`quiet-df-idle.json` 整个70秒的 input_changes 为空；game4464于tick2316546进入前台，观察结束仍在前台。`quiet-df-idle-control.json` 确认启动前与恢复前DisplayFusion进程不存在，游戏模块列表无AppHook/DisplayFusion。finally恢复DisplayFusion成功，游戏和XR继续运行等待关卡。`focus-df-idle.json` 保存25条完整记录、无溢出。

该轮证明本机抢焦点在DisplayFusion缺席时仍会发生，排除它作为此问题的必要条件；不再以修改其设置作为当前修复方向。当前目标仍未完成。quiet15实验观察器 `quiet-minstart15.py` 已准备，将记录窗口IsIconic状态变化；使用新启动器配冻结quiet14 DLL，未运行、未升级默认发布构建。

### quiet15 实测失败；quiet16 定向 MINMAX 观察

`quiet-minstart15.json`：game15760，全程无输入，起始IsIconic=true，tick2481671恢复为false，2481937再次true，2481968恢复为false并进入前台。`focus-minstart15.json` 25条记录无溢出，第二次最小化没有对应的ShowWindow最小化请求。首次显示hint改7不是充分修复，默认仍为4；窗口style差值恰好WS_MINIMIZE，不能据此猜测API来源。

quiet16仅扩展现有TRACE线程CBT观察器接收HCBT_MINMAX，原样CallNextHookEx、不阻断。保持记录ABI：该事件argument为原show command，related/result_window分别为调用下一hook前后的IsIconic，extra_argument低/高32位为对应style；导出器增加明确命名字段。注意“after_next_hook”不是最终操作完成，最终状态仍需外部采样确认。构建和命令行CTest通过，`quiet16-fixture.json`整体passed=true、trace_valid/cbt_valid=1、7条CBT记录。未提升默认发布构建。

用户随后明确自己退出DisplayFusion，进程枚举确认已退出；之后不再自动恢复它。下一轮沿默认首次显示hint4使用quiet16，聚焦第二次最小化通知，避免把quiet15行为改动混入观察。

quiet16实际game34672：70秒无输入，仍在tick2748296自动进入前台。`focus-minmax16.json`记录首个command7、同步恢复command4（游戏452DDA栈），随后同一tick2748296在PeekMessage调用栈交付command7、command9、HCBT_ACTIVATE。因此额外最小化/恢复确为延迟处理的窗口操作，不是外部20ms采样误判或单纯style读数变化。仍未恢复最初排队者身份，不据此认定显卡覆盖层、DXGI或Steam。证据 `quiet-minmax16.json` / `focus-minmax16.json`；无溢出，游戏与模拟器保持等待关卡，DisplayFusion保持用户退出状态。下一步应查这对延迟操作的排队来源，不能靠阻断所有恢复/激活破坏手动切换。

### FancyZones 单游戏排除对照

只读确认本机 PowerToys FancyZones 0.101.2362 开启 appLastZone_moveWindows，游戏与OpenXRPoseProbe都有区域历史；日志确认此前启动时应用了历史区域。上游源码 WindowUtils.cpp 的 SizeWindowToRect 使用 WPF_ASYNCWINDOWPLACEMENT，并可能选择SW_RESTORE（https://github.com/microsoft/PowerToys/blob/main/src/modules/fancyzones/FancyZonesLib/WindowUtils.cpp）。上游main并非本机二进制版本证据。

用户授权临时仅排除GOWDE-Steam.exe并测试后恢复。`quiet-fz-exclude16-control.json`确认原排除值为空、临时单项、最终恢复成功；FancyZones日志确认两次LoadSettings，测试窗口内不再查询游戏历史，但仍查询XR历史。未关闭PowerToys，DisplayFusion保持退出。

`quiet-fz-exclude16.json` game32572整轮无输入，初始/结束前台均ChatGPT17708；旧PeekMessage中的command9及紧随激活消失，command7仍存在。仍收到一次目标游戏的EVENT_SYSTEM_FOREGROUND，tick3000937，CBT栈变为游戏31B24F/31ACE0/4EDFB3→win32u+2ca4；其回调前后前台均外部67396。不能只用事件目标就认定实际抢焦点，也不能仅凭结束前台不变宣称全程通过。现有观察器未保存事件回调瞬间的实际前台，需要补充该字段及连续前台采样，再验证。临时排除不是当前持久修复。
