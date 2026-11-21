# 独立游戏／模拟器实例（2026-11-10）

> **当前状态：按用户最新“查清并修好后继续跑”指令，使用合成隐藏版本恢复。** 旧的仅后台／不抢焦点结论仍撤回。当前唯一入口为本文最后一节的CLOAK模式；旧prepare实例不能直接重启。暂停记录已归档，继续独立开发和运行，不再等待统筹批准。

用户明确要求：去掉本机游戏单实例限制，成功后各开发会话自行管理运行，不再互相等待同一个模拟器。保留先完整逻辑→吸附／Volume→GrabPose的顺序；不主动通知统筹。

## 已验证的边界

- 游戏仍使用用户指定的 `J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe`，原EXE未修改。
- 实测启动调用在游戏RVA `0x294FB0`，返回地址 `0x294FB6`，创建 `Guardian_UnrealEngine3_1`。运行时解密后的局部代码及IDA反汇编已保存；磁盘原EXE这部分为加密字节，不能拿原磁盘伪代码假装运行时代码。
- 原 `tools/quiet-startup/build-instance5/GearsQuietStartup.dll` 的制品缺少当前源码的实例命名隔离逻辑，二进制中也无相关Guardian／实例环境变量常量。不能仅根据build目录名认为隔离已生效。
- 初版多开制品（现已由下述后台修复版替代）`tools/quiet-startup/build-instance-multi1110/` 从当前源码独立构建，保留旧目录。仅显式私有实例把指定游戏互斥名映射为 `Guardian_UnrealEngine3_1.GearsVR.<instance>`。正常启动和其他系统／驱动互斥对象不变。原隔离检查实际通过。
- 干净制品实测 A game55508/XR57304 与 B game56556/XR29068 同时进入Playground、真实epoch1投影成功，独立头部位移±0.12m分别反映在各自XR CSV；两手独立命令均被自己实例消费并ack成功。两份session passed=true；launcher均0；Native正常恢复、XR正常退出、游戏均正常关闭；11项公共配置／存档字节不变。
- 证据：`analysis/coordinator-multi-20261110/report.json`；早期失败及诊断版通过结果分别保留在同目录attempt文件中。这里只证明短时双实例运行和输入隔离基础，不代表各武器流程或头显验收通过。
- 本机约32GiB内存。已验证双实例，不能推断12实例可同时运行。一次出现其他会话加两测试游戏时可用物理内存不足0.5GiB。新入口启动前检查实际可用内存；不足就等资源，不抢占／结束其他实例。

## 入口

工作区始终为 `C:/Users/11/.codex/worktrees/6dof-single/gearsVr`。

新增独立工具：`tools/multi-instance/isolated_sim.py`。复用原 `start.py.prepare`、现有Source1 `sim_entry.py`、原生产Native/Playground和正常停止；不再经过硬编码solo全局空闲检查或8720空闲断言。当前实际输入走实例私有文件，不能通过全局Operator/MCP接口去控制不确定的进程。

各会话使用自己的前缀加时间，例如 `lancer-1110-183000`、`boom-1110-183000`。每次新运行使用新名字，保留旧证据。

```powershell
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B tools/multi-instance/isolated_sim.py prepare --name <本次唯一名字> --manifest <自己已编译候选的manifest绝对路径> --config <自己的配置绝对路径>
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B tools/multi-instance/isolated_sim.py start --name <本次唯一名字>
& 'C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 -B tools/multi-instance/isolated_sim.py stop --name <本次唯一名字>
```

`prepare`复制并核对各自Native、Playground、XR制品，以及私有配置／存档；`--seed`可指定自己的稳定私有实例，默认已通过的冻结实例。不要用默认历史武器制品替代本枪新候选；传入候选manifest并逐项确认SHA。入口生成：

- `analysis/instances/<name>/launcher-session.json`：仅本次的owner/game/session身份。
- 同目录 `manifest.json`、`config.json`、`native/`、`LocalAppData/`、`GameLogs/`。
- `xr-input/LocalAppData/OpenXR-Simulator/`：自己的head/controller命令、ack、settings。
- `isolated-worker.json`：启动回执和准确stop命令；`isolated-launcher.log`、`isolated-done.json`：自己的启动及收尾证据。

## 各会话需要做的小范围接线

1. 把自己动作／录像脚本中写死的`solo-m1-1028d`、共享`stop-basic`、共享launcher日志，改为传入自己的实例根目录、自己的停止入口和输出目录。录制按自己的game/XR PID及creation绑定，不使用“机器上唯一游戏”枚举。
2. 每枪当前候选Native/PG和XR都用各自冻结副本。不得继续stage/restore `solo-m1-1028d`、共享PG输出或别人的实例。需要运行互斥时只用自己实例名；不要复用默认solo的StageToRestore锁。
3. 保留全部身份、时间戳、库存、真实机构终点、Source1标识、后台／静音／物理输入隔离和正常清理规则。把旧的全机唯一game/XR/8720检查替换为对自己实例身份的严格检查，不是直接删掉保护。
4. 原共用实例尚欠的恢复责任先闭合。之后各自独立运行、分析录像和清理，不需要向统筹签收、汇报或发送占用／释放消息。
5. 内存不足时保持其他实例原样，继续自己的离线工作，稍后再检查资源。不是等待“共享模拟器窗口”，不新增排队／票据／审批链，不标blocked。

新增入口是运行隔离工具，没有修改任何武器机制或生产游戏验收结论；公共v71和原游戏EXE保持不变。

## 前台反馈后的后台修复（本节覆盖上文初版启动制品）

用户反馈仍有窗口到前台，全部会话已先暂停新启动并正常结束已有实例。历史弹窗所属实例无法从事后快照确定，不归责某一武器。

发现并修正：原guard在本进程已获前台后不再拦截；主窗口／内部启动弹窗的禁止激活保护不完整；模拟器有两条预览创建路径，初次补丁仅覆盖一条。新私有启动组件在自动后台模式下始终保持保护，对主GUI线程的创建／激活加CBT保护，顶层窗口禁止激活和置顶，并显式启用原有底层前台请求拦截。只有已知私有设置恢复提示自动选择“保留设置”，不显示该弹窗；其他错误没有伪造成功或吞掉。模拟器的两条创建路径和旧窗口采用路径均保留NOACTIVATE。

当前入口仍为 `tools/multi-instance/isolated_sim.py`，但已改用 `tools/quiet-startup/build-instance-background1110b/` 和 `tools/multi-instance/background-runtime.json` 中哈希绑定的私有模拟器。固定强制 `GEARSVR_QUIET_NATIVE_FOREGROUND=1`。旧prepare产物不得直接启动，入口会拒绝旧quiet_build；每会话用当前候选manifest/config重新prepare新唯一名字。不替换运行中的二进制，不改公共v71或游戏EXE。

实际验证 `background-1110-1794306775`：game3052/XR37952进入原Playground并成功输出epoch1投影；5755次前台采样、579次可见窗口检查，foreground/z-order/style违规均0，采样失效0；session passed=true，launcher0，Native恢复、XR退出、game消失，未强杀。静音guard enabled=true、mute_failed=0、正常cleanup0。专用窗口／弹窗fixture在启用底层拦截后也通过。此结论限本轮受控短流程，保留此前失败记录，不能称所有长期／多实例焦点组合完全验收。

验证记录 `analysis/coordinator-multi-20261110/background-check.json`，源与制品身份 `background-source-provenance.json`，旧失败结果同目录保留。各会话仅按此修复版恢复独立后台运行；如再次前台显示，停止本方动作并正常停止本方实例，保留实际窗口／身份记录，不通过抢回用户焦点或移动用户窗口掩盖。

## 当前正式测试入口：合成隐藏窗口，并隔离异常同名子进程

用户最新指令要求继续排查、修复后恢复各会话，不是长期停留离线。用户确认干扰对象为游戏画面窗口。

**已抓到的缺陷：** 原检查只比较当前foreground窗口，不能推出所有显示器都不显示游戏。将窗口放在屏外的实际尝试中，游戏启动布局又把窗口带回`[-1023,-2160,2821,60]`，已有真实屏幕相交反例。后台／NOACTIVATE／位置控制都不足以保证不显示。另Windows应用日志确认实际48756（0xBE74）在18:38:50发生c0000005/RVA45357c崩溃，同期曾观察同名子进程55420。异常路径是原监控盲区；没有历史完整窗口事件，不能声称该崩溃就是每次用户报告的唯一原因。

**当前实现：** `tools/quiet-startup/build-instance-cloak1110/`在自动模式下从窗口创建起使用DWM CLOAK；模拟器所有显示／采用窗口路径同样CLOAK，窗口保留尺寸、渲染及GPU录制能力，桌面不合成它的像素。原不激活／不置顶保护仍保留。worker只对身份一致的本方game/XR窗口维持CLOAK，不移动／激活／最小化用户窗口。Windows错误报告仅本测试进程设NO_UI；同EXE子进程定向放入`GearsVR.HiddenChildren.<parentPID>`非活动桌面，不切换用户桌面，不改其他应用或授权流程。记录子进程身份，原父流程结束后按精确身份正常关闭子窗口；不强杀。

`isolated_sim.py`现指向上述制品及`background-runtime.json`哈希绑定的私有模拟器。各会话必须传自己的当前manifest/config重新prepare新名字，不复用旧background1110b/offscreen实例。旧独立动作脚本的前台暂停标记只解除本次“等待后台修复”这一原因，不改变机械／身份／新鲜度检查。

**实际证据：** `cloak-check-initial54frames.json`对应game57684/XR40496已进真实Playground并输出投影；6833次前台采样、675次可见自有窗口的CLOAKED状态读取，失效0、uncloaked0、可见桌面相交0。54帧实际投影录制成功、无录制错误；已查看frame00030，真实场景及双手可见，不是黑屏／等待卡。原launcher0、session passed=true、game/XR/Native正常收尾。不能因此称某枪动作或物理头显通过。

`child-desktop-result.json`另证明同名子进程在非活动桌面创建窗口成功，桌面名为`GearsVR.HiddenChildren.56100`，父启动仍未切换桌面。新子进程路径是控制夹具验证，非再次诱发真实游戏崩溃。

本机32GiB，每次已有两个自动实例时停止新资源申请，继续离线工作等资源释放。资源检查／worker身份登记使用毫秒级同锁事务，**运行期间不持有共同模拟器锁**，两个实例可独立同时运行；防止12个同时预检内存充足后一起启动造成争用／崩溃。此限制是资源边界，不是重新引入候选签收／前后继审批。

继续先完整逻辑→吸附／Volume→GrabPose，静音，不使用全局MCP或Computer Use，不向统筹主动发启动／占用／释放消息。各自保留原录制方式；若某种HWND录制在CLOAK下不可用，使用现有私有GPU投影录制，不得解CLOAK来录制，不用桌面截图替代。

恢复后的实际观察：Smoke grenade-smoke-1110-193549 与Boom boom-1110-193550-55800均进入running，CSV持续增长，1818次自有窗口CLOAKED检查解除隐藏0、前台0；同时worker最多2。证据resumed-observation.json与cloak-final-summary.json。随后统筹额外重复检查因两实例已占资源而在启动前被拒绝，该次未生game，不当新的渲染失败；初始54帧通过证据已单独保留。

最新收尾复核：上述Smoke与Boom两份原session均native_started=true、phase=stopped、passed=true，非启动等待卡通过。观察器的running_observed对Boom为false是因为采样窗口先结束，不能把它改写成当时已观测；之后的终态另存cloak-final-summary.json/resumed_terminals。窗口观测最终2042次，解除隐藏0、前台0。

## 静音统计解释与明确读回证据

HB47004反馈的matched1/muted0/read_failed0/mute_failed0不能断言未静音：旧Policy在GetMute本来TRUE时直接保留，不增加muted写入计数。shared_rejected扫描所有会话，并非目标专属。只读现场Bow39312、Gnasher56500均读到1个目标独占会话且GetMute=TRUE，目标共享拒绝0，其他全局共享拒绝11。没有任何音量/静音写入。

新增测试专属helper与GEARSVR_TEST_AUDIO_EVIDENCE=1；记录initially_muted_sessions、verified_muted_observations、verify_failed和target_shared_rejected，成功设置后重复GetMute确认，读取也检查同一session/PID。旧helper没有明确证据时mute_verification标未知。91项C++策略与11项Python生命周期/证据检查通过，保留设置返回成功但读回未静音的拒绝回归。系统及其他应用音量、原Engine Music/FX/Dialogue和SoundCue功能不改，共享会话仍拒绝变更。证据audio-evidence-review.json；本项不是各枪玩法或完整时段音频验收。


## 后台 worker 与音频 ready 的独立修复

用户确认此前到前台的是游戏画面窗口。窗口隐藏结论沿用 cloak-final-summary.json 的实际投影、前台采样与 CLOAK 检查；不能用 focus=0 代替不可见。全 12 会话已经通知继续独立后台验证，未恢复全局暂停。

HB 实例 hb-cloak-1110-203109 另有具体启动故障：worker 读取正在更新的 birth JSON 时 PermissionError 未捕获，worker 退出且没有 finally，正常收尾未执行。新增 runtime_receipts.py 对锁占用/不完整 JSON 有界重读，缓存首次私有记录中的精确 PID/创建时间/镜像，后续变更拒绝；所有窗口写入仍要求当前身份严格一致。完全不可读返回未知，不冒充游戏已退出。worker finally 保留资源所有权，请求自己的 launcher 正常恢复并等待其退出，再重复正常 WM_CLOSE 自己的游戏和记录的私有子进程；没有强杀。尚未获得身份/收尾未完成时不写成功回执。

音频原 3 秒 ready 预算和期限边界未再读存在缺陷；现改为默认 15 秒、最大 30 秒的有界等待，期限处仍读取一次；错误目标/owner 和已退出 helper 拒绝。helper 新增 boot/service-ready/guard-ready 分段记录供下一次真实故障定位，不据此宣称 COM 已证实为根因。新 build-mute-evidence1110b 已单独构建并写入来源散列，后续新实例使用，旧运行进程不替换。ready 仍不代表已静音，必须用目标会话 mute readback；私有游戏内各音量及系统音量不改。

离线验证：6 项记录/正常 stop 身份检查、15 项音频生命周期测试、91 项 C++ 音频规则检查通过，Python 解析通过、新 helper 编译成功。新 worker finally 异常分支尚未进行新游戏运行验收，不冒充完整运行通过；既有 CLOAK 实际运行证据保持独立。详见 analysis/coordinator-multi-20261110/startup-worker-repair.json。
