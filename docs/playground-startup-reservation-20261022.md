# Playground menu and VR start fix

2026-10-22. User reports that the main menu of the game does not have a test field, and VR cannot be started when it is developed. This round has been restored to 0.1.0 Default list, actual real game is `J:\gearsVr\Gears of War Reloaded`。

## Observations and causes

Initial PID 41780 Start from the public package of the subdirectories, with a stand-alone list using AdsAnchor 2, no Playground field or DLL. Then root directory development PID 52544 Report `Playground initialization failed: 5`;component logs further record MinHook `create_hook_failed=9`. This initialization error occurs before the menu is added and results in the original startup process being withdrawn earlier.

Only expand the candidate for adjacent distribution window in the positive PID 43508 Still failed, the new diagnosis limits position to trampoline distribution and excludes MinHook hook-table distribution branch. Error tag 8 The failure of the candidate to separate branches cannot be considered a real window distribution failure. The candidate's failure to isolate the original entry and continue to start the process, but does not generate testing grounds that can be recorded as pass-through. The subsequent address-only window records contain available areas, which do not allow for an assertion that the entire window was depleted when it failed; the exact floor allocation limit has not yet been attributed.

## Modify

- Start loading early-loading locked Playground DLL, using `PlaygroundReserve` Creates but does not enable Hook. Full 64 Byte-entry authentication and original entry check remain in place.
- The engine registration form is ready.`PlaygroundInitialize` Re-use the same DLL preset springboard and enable event distribution; add `playground_early_reserve` List field controls the path. MinHook and native/XR binary of the renderings are not recompiled with the changes.
- Expand rel for current Playground construction only 32 Distribution window, keep 64 KiB excess and check complete before commissioning 64 Byte jumpability for trampoline slot. Current authentication entrance 6 bytes are push commands, and this range conclusion is not extended to other functions.
- Resource/enabled failure only to read independently confirming original 64 The byte is restored to the test site. Identity, certificate, unknown state or failure path cannot be confirmed.
- Records the TRampoline/book-table allocation, distribution attempts and actual Windows errors; enable failed recovery of created Hook. Releases the preset resource before normal stoppage and cancellation; cleanup mode only attachments already loaded DLL without injection or reinitiation.

## Validation and Borders

Independence `PlaygroundHookTest` I'm just trying to get old.1.5 GiB window, actual Hook/Preliminary call from new window 64 Byte is restored; the intended distribution error returns when the full accessible window is filled, and the entrance remains unchanged.

PID 49328 Use the latest CoverAim 2 native and backstage emulator: early pre-allocation success, relay delta `-1998365`; the component was successfully enabled. Selecting precise text through the actual main menu Down/Enter `happyotaku playground` And get into the map.11 A pick-up/6 Targets/Troika, failed 0 Actual shooting, normal ammunition exchange and spare ammunition are not reduced, target passes again; XR actually submits 14,031 The second game of projection,13,150 The test script is once encountered with a blank record of the history, corrects the private script and then passes.

Round Playground Stop/Opport 64 Byte back through, native self-cleaning through, XR exit 0. But independent master check 79- Of the items.7 One. `dxgi.dll` The entry is not compatible with disk and there is no pre-installation evidence, and the overall report is maintained `incomplete/passed=false`, cannot be said to have been fully recovered; this time the verification has not been relaxed or attributed to other components. `analysis/runtime/continuous-stereo-49328-20261022-021413-040e69f6.json`。

PID 49544 Cancel before preset completion and component enabled: Playground `reservation_only=true` Cleaning and 64 The byte was restored, the native prime cancelled/entry cancelled; the independent validation test process was properly withdrawn. The pre-cancellation competition for private oversight scripts was also amended and no killing process was carried out.

Eventually.213 The item trigger is checked through,722 Documentation,41,961,304 The byte isolation is subject to inspection.8 In the old pre-encumbrance life cycle check 2 Error reporting entry folders, all mock returns values missing stdout/stderr; replaying unmodified HEAD source code duplicates the same 2 Errors, record-keeping, do not count as the current pass item.

The current default menu component is: `build-playground4/Release/GearsPlayground.dll`，SHA 256 `b6c32ebfd84cbaa068c6943bfc2a178caaea3bb9009c3a709d6288fbc096608a`. Only changes based on the latest default 3 Playground field, retain CoverAim 2 native SHA 256 `04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0`, XR and user configuration. Current development portal is the repository root directory `GearsVR Launcher.exe`. This is not a public release or entity header acceptance; analytical, original error and validation checkout left behind `analysis/playground-entry-20261022/`。

---

## 中文

# Playground 菜单与 VR 启动修复

2026-10-22。用户报告游戏主菜单没有测试场，改用开发版后 VR 也无法启动。本轮修复已集成到0.1.0默认清单，实际正版游戏为 `J:\gearsVr\Gears of War Reloaded`。

## 观察与原因

最初的 PID41780 从游戏子目录的公开包启动，其独立清单使用 AdsAnchor2，没有 Playground 字段或 DLL。随后根目录开发版的 PID52544 报 `Playground initialization failed: 5`；组件日志进一步记录 MinHook `create_hook_failed=9`。这次初始化错误发生在菜单添加之前，并使原启动流程提前退出。

仅扩大邻近分配窗口的候选在正版 PID43508 仍失败，新诊断将位置限定为 trampoline 分配，排除 MinHook hook-table 堆分配分支。错误标记8用于区分分支，不能当作真实 Windows 分配失败原因。该候选的失败隔离验证了原入口未变并继续启动流程，但没有生成测试场，不能记为场景通过。后续只读地址窗口记录包含可用区域，不能据此断言失败时整个窗口耗尽；确切底层分配限制尚未归属。

## 修改

- 启动早期加载版本锁定的 Playground DLL，用 `PlaygroundReserve` 创建但不启用 Hook。完整64字节入口认证与原入口不变检查仍保留。
- 引擎注册表就绪后，`PlaygroundInitialize` 复用同一 DLL 的预留跳板并启用事件分发；新增 `playground_early_reserve` 清单字段控制该路径。渲染器的 MinHook 和 native/XR 二进制不随本修改重编译。
- 仅对当前 Playground 构建扩大 rel32 分配窗口，保留64KiB余量，并在启用前核对完整64字节 trampoline slot 的跳转可达性。当前认证入口前6字节为 push 指令，不把这个范围结论推广到其他函数。
- 资源／启用失败只有在独立重读确认原64字节完整还原后，才可使测试场暂不可用而继续 VR。身份、证书、未知状态或无法确认还原仍沿用失败路径。
- 记录 trampoline／hook-table 分配位置、分配尝试及实际 Windows 错误；启用失败回收已创建的 Hook。正常停止及启用前取消都释放预留资源；清理模式只附着已加载 DLL，不注入或重新初始化。

## 验证与边界

独立 `PlaygroundHookTest` 故意占满旧1.5GiB窗口，新窗口下实际 Hook／原函数调用及64字节还原通过；整个可达窗口占满时返回预期分配错误，入口保持不变。

正版 PID49328 使用最新 CoverAim2 native 和后台模拟器：早期预留一次分配成功，relay delta `-1998365`；随后组件启用成功。通过实际主菜单 Down／Enter 选中精确文字 `happyotaku playground` 并进入地图，11个拾取物／6个靶子／Troika、失败0。实际射击击杀、正常换弹且备用弹药不减少、靶子重生通过；XR实际提交14,031次游戏投影、13,150次场景复制。测试脚本一度遇到遍历中的空类记录，修正该私有脚本后通过，错误回执保留。

该轮 Playground 停止／原入口64字节还原通过，native自身清理状态通过，XR退出0。但独立总校验79项中有7个 `dxgi.dll` 入口与磁盘不匹配且缺少安装前证据，整体报告保持 `incomplete/passed=false`，不能称整轮全部还原验收通过；本次未放宽该校验或归因于其他组件。原始报告为 `analysis/runtime/continuous-stereo-49328-20261022-021413-040e69f6.json`。

正版 PID49544 在预留完成、组件启用前取消：Playground `reservation_only=true` 清理及64字节还原通过，native prime取消／入口还原通过；独立确认测试进程正常退出。私有监督脚本的提前取消身份采集竞态也已修正，未强杀进程。

最终213项启动器检查通过，722文件、41,961,304字节的隔离便携依赖检查通过。另有8项旧预留生命周期检查中的2项夹具报错，均为模拟返回值缺少stdout／stderr；回放未修改的HEAD源码复现相同2项错误，保持记录，不算本次通过项。

当前默认菜单组件为 `build-playground4/Release/GearsPlayground.dll`，SHA256 `b6c32ebfd84cbaa068c6943bfc2a178caaea3bb9009c3a709d6288fbc096608a`。基于最新默认只变更3个Playground字段，保留 CoverAim2 native SHA256 `04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0`、XR和用户配置。当前开发入口为仓库根目录 `GearsVR Launcher.exe`。本次不是公开版本发布或实体头显验收；分析、原始错误和验证回执留在忽略的 `analysis/playground-entry-20261022/`。
