# 启动器 EXE 同目录识别与启动错误反馈

## 解压包本机复现：下载脚本与多账号设置

用户随后在项目内正版目录直接运行解压的 0.0.1 包，重现无法启动。旧日志丢失了 PowerShell 子进程错误；显式捕获两路输出后确认，这一轮最早被带下载来源标记的未签名 `Start-DebugGame.ps1` 拦截，游戏尚未创建。单独运行旧版画质设置检查还确认多份账号设置会触发 `Expected exactly one user-settings file`；它是后续独立缺陷，不能替代最先失败点。

修复为仅向启动器拥有的 PowerShell 子进程传入 `-ExecutionPolicy Bypass`，不改系统／用户执行策略，不移除下载标记；脚本和Python输出统一UTF-8，并将启动脚本的标准输出、错误显式写回本地预留日志。解压包同时补入已验证的Steam进程环境与当前账号设置选择。游戏EXE、正版Steam DLL、native/XR及玩家启动配置逐文件哈希保留。

后台受保护验证在项目内正版PID47884到达 `AttractLauncher`，当前玩家菜单只读样本一致，35帧启动预检、229层启动菜单平面提交；未进入关卡或激活原生场景。owner退出0，owned-prime入口还原、游戏正常关闭、音频恢复和11个受保护文件还原通过。启动前取消时XR退出码为1，同时终止证据为有序停止、无结构错误、对象销毁完成；不把该结果写成完整XR／头显验收。原始失败和成功回执在 `analysis/retail-portable-startup-fix-20261021/`。用户要求当前开发先提交保存至0.1.0，再在独立0.0.2分支处理和交付该修复。

## 同日正版本机复现与修复

2026-10-21 用户另购Steam正版后，本机复现相同 `Target priming failed (1)`。这次有Steam侧日志：直接运行的项目内EXE请求 `steam://run/2523720`，Steam转而创建库内 `Launch_GOWDE.exe`，原预留PID退出。子进程明确传入正确Steam App ID后，本机正版保持原路径／进程，预留及13处入口字节核对通过；已接入当前开发启动层，原版Steam DLL和账号授权流程保留。首次运行产生第二套设置时也已修复当前Steam账号的精确选择。用户随后停止追加运行测试，本轮不宣称正版关卡双目或头显通过。见[正版迁移与完整证据](retail-steam-compatibility-20261021.md)。

该证据解释本机复现，不直接认定远端玩家缺少的prime状态和进程退出证据。已发布0.0.1包尚未包含此启动环境修复；以下原始反馈与当时的证据边界保留。

2026-10-21，玩家 Notagameaddict 反馈：Mod 已放在游戏 EXE 旁，启动器没有自动填入游戏路径。截图中手动选择的文件为 Steam 游戏目录下的 `Binaries_x64/GOWDE-Steam.exe`；VDXR 已返回每眼 2496×2688 的推荐尺寸，随后出现 `Fresh reservation failed; no retry`，启动退出码 1。

初次反馈时用户确认对方尚未发送详细日志，因此没有把截图中的启动失败归因于路径、DLSS、VDXR 或游戏版本。后续收到的预留日志已将故障缩小到目标预备调用，见下节；自动识别修复不能当作该玩家启动问题已经解决。

## 修改

GUI 原先只查找启动器根目录下的 `Binaries_x64/GOWDE-Steam.exe`，漏掉启动器本身就在 `Binaries_x64` 的安装方式。现在保留已保存且仍存在的路径，否则依次查找启动器目录中的 `GOWDE-Steam.exe`、`Binaries_x64/GOWDE-Steam.exe`，再在父目录查找同样两个位置。这样也覆盖解压到一层 `GearsVR` 子目录的情况。查找不依赖当前工作目录，最多上查一层；真正启动时继续执行既有 EXE、配置与构建哈希验证。

Python 启动预留失败时，优先显示子进程原始 Python 异常，避免末尾的 `CalledProcessError` 包装盖住根因；没有异常行时显示有界的错误尾部。提示包含子进程退出码和详细日志路径，完整输出仍保存在本地 `.reserve.log`。错误文本最多 1500 字符；现有会话状态与脱敏诊断导出继续保留其摘要。子进程输出解码替换无效字符，避免解码失败遮住原始启动错误。保留单次启动和按进程身份清理的既有行为。

## 未确定的启动错误

### 后续收到的预留日志

用户随后提供的日志显示：22 项启动前自检成功，用时 9505.2117 ms；创建了 PID 37432、创建时间 134370432031460723 的游戏进程；随后 `launch_reserve.py` 报 `Target priming failed (1); do not activate`。这证明启动前 DLSS 配置检查已通过，之前怀疑的保存设置拦截不是这次失败点。按该版调用顺序，Mod DLL 加载确认、入口预留、mask 预留及独立入口字节核对也已完成。

`reservation_birth` 是游戏刚创建时的收据，因此其中 `injection_attempted=false` 不代表后续没有注入；失败清理给出的 `injection_state_unknown_or_attempted` 也只是拒绝把该进程当作“确定尚未注入”来自动关闭，并非第二个故障原因。

核对的是 0.0.1 的冻结源码 `analysis/ads-anchor2-source/native-capture/capture.cpp`，文件 SHA256 为 `0c1e87f6ece214ed7f10aa14848990659848589e0154e4ece05c2826c27ac670`，与发布源码快照相同。构建回执 `analysis/native-build-ads-anchor2/verified-build.json` 指向该源码；发布 DLL SHA256 为 `94b73bb04b6f36d4a4bc16f84f1e95d49a1a3144792e0e41d004ef3b802b4304`。没有以当前开发 DLL 替代玩家版本作结论。

该版 `GearsCaptureTargetPrimeOwned` / `begin_target_prime` 没有显式返回 1 的分支。Python 的 `remote_call` 读取的是 `GetExitCodeThread`，所以这里的 1 是远程线程退出码，不能直接解释成 Win32 `GetLastError=1` 或某一个 Hook 的失败编号。游戏进程在远程调用途中退出是需要核对的候选解释；尚无该玩家进程退出状态或原生预备日志，不能认定为游戏崩溃、驱动故障或第三方 Hook 冲突。

下一份需要的证据为玩家 Mod 目录下 `tools/native-capture/build-ads-anchor2/target-prime-status.txt`，以及报错后游戏是否仍在运行。预备例程会创建 D3D12/DXGI 探测对象并准备、启用图形入口 Hook；状态文件可进一步缩小失败位置。此文件包含本机原始诊断，作为私下问题排查材料，不加入公开发布包。

### 初始截图阶段的候选原因（历史）

截图里的 `Fresh reservation failed` 是父进程汇总，不能确定失败发生在自检、启动前策略、进程创建、依赖等待还是注入阶段。`Check eye size`/运行时尺寸查询成功也不证明实际 XR 会话或 GPU 出图已通过。

源代码中另有一条可核对的可能原因：`Start-DebugGame.ps1` 对外部安装执行 `graphics_policy.py check`；如果用户已保存 `DLSS=True`，会以 `Persisted DLSS must be OFF before release startup` 拒绝启动。此处仅是代码条件，不是对该玩家现场的判断。本次不改变该策略。可请玩家先在普通游戏内关闭 DLSS、正常退出游戏后重试。

若仍失败，需要该 Mod 目录下 `analysis/runtime/vr-launcher-session.reserve.log` 末尾 20–30 行。原版诊断包不保证包含这份原始子进程日志；更新后的启动器会把提取到的原因保留在界面和导出摘要里。

## 验证与交付边界

GUI 隐藏自检覆盖 EXE 同目录、游戏根目录、一层 Mod 子目录、已有有效绝对/相对路径优先、无效路径回退、中文与空格目录、无关工作目录和最多一层父目录的边界。Python 回归覆盖具体错误展示、包装异常剥离、缺失输出与长度限制、完整日志保留、单次调用、失败清理以及尚未建立立体会话时的脱敏导出。

142 项 Python 检查及 GUI 编译、隐藏自检通过，覆盖启动器、诊断导出、便携暂存和打包四个模块。正常 GUI 已同步更新，当前默认 native/XR 清单和用户配置的 SHA256 与修改前一致。未覆盖已发布的 0.0.1 ZIP 或线上资产。本次验证不启动游戏，不新增实际游戏、GPU 或物理头显验收结论。验证回执保存在忽略目录 `analysis/feedback-launcher-20261021/result.json`。
