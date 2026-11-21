# Native View Call Validation:2026-10-11

## Conclusions

Confirmed via temporary Hook in current game process: RVA `0x15cf08` The current WarLocalPlayer constructs and returns objects containing view/ projector matrices. The location, rotation and matrix of the original output changes with the player; the same round is also recorded as DXGI Presten. This is further than static candidates but has not yet been rewritten or rendered any view.

Player reports that the "normal, no apparent anomalies" during testing. The original code entry patch has been withdrawn, collecting DLLs remains in process and is released as the game exits. After stopping, the camera locator is successfully run again, and the original entry bytes of both games functions are restored and verified.

## Experimental conditions

Target is SHA.256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，PID 32532. The collection starts with the Hook enabled, all 120 sec, which contains chats, focus pauses, reset games and players moving / turns lenses, not complete 120 Seconds continue the game.

Use this machine 2026 MSVC x 64 and Windows SDK compiled. Third party MinHook v 1.3.4 . The DLL loads the process through a starter and temporarily rewriting the function entry using MinHook. This round is not a read-only memory survey, but does not modify location, direction, matrix or original function parameters/ return values.

The independent test program before access validates the alignment of parameters and returns with real trampoline, event nesting records and matrix copy. Additional enhancements are made in `build-next` Compile, self-measured: add entry byte after stoppage, re-call the original function and keep a static CRT thread notification. Enhancement is not loaded for the current game, and the run-time result is the original `build` .

## Call Results

| Category | All records | Record with game focus | Main Object / Thread |
| --- | --- | --- | --- |
| scene view 0x15cf08 | 4115 | 1071 | WarLocalPlayer 0x914b8bc0; threads 4960 |
| Player Viewer Event 0x15c968 | 10068 | 3978 | PlayerController 0x8d782ae0; threads 4960 |
| DXGI Present | 4165 | 1120 | Same swapchain 0x107229ad0; threads 17824、31100,Slight 4960 |
| DXGI Present 1 | 0 | 0 | Method Hook was created successfully and no call has been recorded in this round |

The above address and thread ID is used only to describe the experiment and cannot be a positioning configuration for the new process.

All 4115 The secondary scene view calls match the LocalPlayer independently located before startup;4115 We've read two candidate matrices successfully. 32 Individual elements, all of which are limited. 351 It's different.57 A different rotation and 352 Groups of different matrices; Cameron Cache time is also advancing. The small matrix differences at stationary times are not attributable separately.

The caller for the scene view returns all positions to `0x2c7bfa`..with static investigations. `0x2c7bf5` Call command corresponds. Player point event caller includes `0x15d0ca`, directly validates the path it is built from a scene view; there are calls for other uses, so the player point-of-view event count cannot be used as a frame rendering.

Median number of scene views to call interval for focus periods is 16.65905 ms; two main Precent workspaces are each approximately 16.6634 and 16.6645 ms. The interval between the unfocused view and the main Present caller is approximately 33.33 ms. The different groups contain focus switching, thread switching and preparation times, do not extrapolate the continuous frame rate using large intervals or aggregates, nor does it prove that view corresponds to Precent. All Presents are not DXGI_PRESS_TEST and have failed HRESUT.

Queue to be abandoned 1 Logs are not no-loss tracks, and counting should not be used to strictly prove that each frame is called exactly once. View original functions take about the time of focus (excluding back-to-back expenses). 0.0085 ms。

## Observation when camera cache stops updating

There's a continuous agreement in the record. 5.5866 Seconds, game has focus but CameraCache time holds 129.227142 The footage of the event. 286 Subscenes View Call and 335 Second.

This proves that the primary path can continue at this observation when the cache is still running. However, users may have focused on the pause menu, not yet restored to Esc or there are other reasons for the pause; no menu status and VM remote sensing so this**No, it's not.**Evidence of “repeated and resolved script VM pauses”. Players also reported no apparent freeze.

## Stop and restore evidence

Status file termination act:`disable_status=0 active=0 dropped=1`. Revalidation of RVA with read-only locator after stop `0x15cf08` and `0x15c968` Original entrance bytes are restored in both locations; the current camera relationship is still valid and the game process responds. Present Hook's revocation is confirmed by MinHook returning to successful status, without saving and checking its original entry by byte.

Starts scripts to throw out reading errors because the captured status file is locked in CRT; DLL and the collection thread are actually started. Confirming operational status by naming a collection event created only after Hook is enabled, successfully reading the full log after collecting. The starter has joined the processing of this file lock situation, does not equate failure with failure of injection or blindly overloading.

Local original evidence:`analysis/runtime/native-capture-1.csv`、`.json`、`.status.txt`. These do not enter Git; repository save code, validation conclusion and version candidate configuration. The current DLL is still in process and the original entrance has been restored; a new round or build needs to exit and reset.

## Next

The next step is a very small, reversible view change test: the field view takes back the player's view point and before generating the matrix, verify that a controlled angle changes affect the image while retaining original person input. It must be limited to the current rotation user and current player identity, avoiding any modification of other GetPlayerViewPoint calls such as Aim, Audio or other.

This step is used to control the entry by using a primary lens. Then you can take OpenXR position and study left-and-right eye independent view and projection; already existing matrix reading capacity does not equal to having two eyes or 6DoF。

---

## 中文

# 原生视图调用验证：2026-10-11

## 结论

已通过当前游戏进程中的临时 Hook 确认：RVA `0x15cf08` 会为当前 WarLocalPlayer 构建并返回包含视图 / 投影矩阵的对象。原函数输出的位置、旋转和矩阵随玩家操作变化；同一轮也记录到了 DXGI Present。这比静态候选更进一步，但还没有进行任何视图改写或 VR 渲染。

玩家报告测试期间“正常，没有明显异常”。原始代码入口补丁已撤销，采集 DLL 留在进程内，随游戏退出释放。停止后再次运行摄像机定位器成功，两个游戏函数的原始入口字节恢复并核验通过。

## 实验条件

目标为 SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，PID 32532。采集从 Hook 启用时开始，共 120 秒，包含聊天、焦点暂停、恢复游戏和玩家走动 / 转镜头，不是完整 120 秒持续游戏。

使用本机 Visual Studio 2026 的 MSVC x64 和 Windows SDK 编译。第三方 MinHook v1.3.4 的源码下载到忽略目录，不修改游戏文件或复制 DLL 到游戏安装目录。DLL 通过启动器加载进程，并使用 MinHook 临时改写函数入口。这轮不是纯只读内存调查，但不修改位置、朝向、矩阵或原函数参数 / 返回值。

接入前独立测试程序通过真实 trampoline 验证参数和返回值保持一致、事件嵌套记录及矩阵复制。后续增强版另在 `build-next` 编译，自测也通过：增加停止后的入口字节恢复、原函数再次调用检查，并保留静态 CRT 的线程通知。增强版尚未加载当前游戏，运行期结果属于最初 `build` 中的版本。

## 调用结果

| 类别 | 全部记录 | 有游戏焦点的记录 | 主要对象 / 线程 |
| --- | --- | --- | --- |
| 场景视图 0x15cf08 | 4115 | 1071 | WarLocalPlayer 0x914b8bc0；线程 4960 |
| 玩家视点事件 0x15c968 | 10068 | 3978 | PlayerController 0x8d782ae0；线程 4960 |
| DXGI Present | 4165 | 1120 | 同一 swapchain 0x107229ad0；线程 17824、31100，少量 4960 |
| DXGI Present1 | 0 | 0 | 方法 Hook 创建成功，本轮没有记录到调用 |

上述地址和线程 ID 仅用于描述本次实验，不得作为新进程的定位配置。

全部 4115 次场景视图调用都匹配启动前独立定位出的 LocalPlayer；4115 次都成功读取两个候选矩阵，共 32 个元素，所有元素有限。焦点期间观察到 351 个不同位置、57 个不同旋转和 352 组不同矩阵；CameraCache 时间也在推进。静止时的微小矩阵差异没有单独归因。

场景视图的调用者返回位置全部为 `0x2c7bfa`，与静态调查的 `0x2c7bf5` 调用指令对应。玩家视点事件的调用者包含 `0x15d0ca`，直接验证了它由场景视图构建路径调用；也有其他用途的调用，所以玩家视点事件计数不能当作渲染帧数。

焦点期间场景视图调用间隔中位数为 16.65905 ms；两个主要 Present 工作线程分别约 16.6634 和 16.6645 ms。无焦点时视图与主要 Present 调用间隔约 33.33 ms。不同分组包含焦点切换、线程切换和准备时间，不使用大间隔或总计数推断持续帧率，也未证明视图和 Present 一一对应。所有 Present 都不是 DXGI_PRESENT_TEST，且没有失败 HRESULT。

队列共丢弃 1 条记录。日志不是无损追踪，计数不应用于严格证明每帧恰好调用一次。视图原函数耗时中位数（不含回调后续采集开销）焦点期间约 0.0085 ms。

## 摄像机缓存停止更新时的观察

记录中存在一个持续约 5.5866 秒、游戏拥有焦点但 CameraCache 时间保持 129.227142 的片段。在这段时间里仍观察到 286 次场景视图调用和 335 次 Present。

这证明本次观察中该原生路径可以在缓存时间不变时继续执行。不过用户可能已聚焦暂停菜单、尚未按 Esc 恢复，或存在其他暂停原因；没有菜单状态和 VM 执行遥测，所以这**不是**“已复现并解决脚本 VM 停顿”的证据。玩家也未报告明显冻结。

## 停止与恢复证据

状态文件结束行为：`disable_status=0 active=0 dropped=1`。停止后用只读定位器重新验证 RVA `0x15cf08` 与 `0x15c968` 原始入口字节，两处均恢复；当前摄像机关系仍有效，游戏进程有响应。Present Hook 的撤销由 MinHook 返回成功状态确认，未保存并逐字节核验其原始入口。

启动脚本最初因采集中状态文件被 CRT 锁定而抛出读取错误；DLL 和采集线程实际已启动。通过仅在 Hook 启用后创建的命名采集事件确认运行状态，采集结束后成功读取完整日志。启动器已加入这一文件锁情形的处理，不将文件读失败等同于注入失败，也不会盲目重复加载。

本地原始证据：`analysis/runtime/native-capture-1.csv`、`.json`、`.status.txt`。这些不进入 Git；仓库保存代码、验证结论和版本候选配置。当前 DLL 仍在进程内，原入口已恢复；新一轮或新构建需要先退出并重开游戏。

## 下一步

下一步是极小幅度、可回退的视图变化测试：在场景视图取回玩家视点之后、生成矩阵之前，验证一个受控角度变化能影响画面，同时保留原有人物输入。必须根据本轮调用者和当前玩家身份限定作用范围，避免把瞄准、音频等其他 GetPlayerViewPoint 调用一起修改。

这一步通过才算可用的原生镜头控制入口。之后接 OpenXR 位姿，再研究左右眼独立视图与投影；已有矩阵读取能力不等于已有双目或 6DoF。
