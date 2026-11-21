# Background native viewport and output sizing

2026-10-18. This continues the background-black investigation without window activation or automated window movement. The user's observation that moving the game window restores its picture remains the main trigger for investigating engine resize processing. It does not establish that focus itself is required.

## Bounded static and current-byte evidence

Existing KB artifact 1 chunks were exported mechanically with the existing vendored Capstone. Historical module base is `0x140000000`, code-section SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`. The four functions (`199d1c`, `1995d0`, `50ae6c`, `27f740`) total 4,645 bytes including recorded cold chunks. Both successful live probes compared every saved byte twice and matched each full-function hash. Source chunks, hashes and the read-only probe remain ignored in `analysis/ida/background-size-20261018/`.

The dispatcher maps HWND through the native viewport list at RVAs `1c5eb38/1c5eb40`. A unique listed viewport's `+98` field matched the owned HWND. Its WM_SIZE handler can bypass resizing for several state flags; `+d0` is set/cleared by the window resize routine. The size helper reads the actual client rectangle rather than WM_SIZE's lParam, checks another virtual condition and compares cached width/height before invoking the engine resize method. Current getter instructions establish cached width/height at viewport `+80/+84`; the independent window-client measurement used a temporary DPI context on the probe thread only. No target function was invoked or game memory written.

## Background runs

`background-viewport-state-20261018.json` stopped when a real game-foreground sample appeared, before useful capture. It cannot identify automatic activation versus manual switching. A later post-close lookup correctly rejected the unmatched HWND; its errors are retained. The probe's initial missing Resolver.prepare call was corrected before the successful runs, not treated as a game defect. Its owned prime restored entries; after normal close timed out, only the identity-checked owned game was terminated and settings restored.

`background-viewport-state2-20261018.json` captured 16 consistent viewport samples. Output observations changed from 2534×1009 to 8×8 for approximately 2.5 seconds, then returned to 2534×1009 without this test issuing a size notification or other window mutation. No game or owned-XR PID appeared in actual foreground samples. It stayed in startup/menu, so it is not gameplay acceptance. Normal shutdown and settings restoration passed.

`background-viewport-gameplay-20261018.json` used the normal Native70 launcher with auto-Continue and observe-only tiny-output monitoring. The same short 8×8 transition was observed. Nearby native-window samples showed cached size 2560×1080, briefly 2534×1009 with `+d0=1`, then 2560×1080 with `+d0=0`; physical client size was 2534×1009. These sequential observations are not an atomic proof of which message or thread changed the DXGI output.

The run reached a possessed level player and active native stereo, then recorded 1,127 simulator projection submissions and 517 scene copies over 12.5087 seconds. Final maximum submission gap was 24.103 ms. Neither the game 17788 nor XR 8500 appeared in the foreground samples. Normal owner/XR stop, native cleanup and 44 independently restored entries passed; normal game closure, four profile files and the full INI were restored. The session is `continuous-stereo-17788-20261018-014043-0bfadccd`.

This establishes one complete background-start/auto-Continue/stereo-session path with the current Python and frozen Native70 combination. It does not prove image correctness, physical-headset performance, mounted play or absence of every intermittent startup fault. The repeated short 8×8 transitions show why the existing 2-second recovery threshold can act during ordinary initialization; they do not establish that the recovery action caused the earlier persistent black screens. The prior WM_SIZE-only experiment remains a negative result.

The recovery policy now requires five seconds of distinct, fresh tiny-output observations, retaining the existing two-second freshness/gap limits and once-per-session rule. This avoids acting on the observed short startup transition; it is not a repair of a demonstrated persistent-black root cause. All 28 focused tests passed, including a normal→2.5-second tiny→normal regression that never inspects or changes the window, an exact five-second advancing-QPC boundary, and unchanged observe-only behavior. Autonomous tests continue using observe-only mode. The Feedback3 ZIP predates this threshold change and retains its original digest.

## Private runtime-size zero-extent candidate wiring (2026-10-19)

The saved `analysis/runtime/background-zero-extent-actual-20261018-proof.json`
isolates one allocation boundary: untreated `ResizeBuffers(0,0)` changed a
freshly verified 2534×1009 buffer into 8×8 while the background window was iconic
with a 0×0 client. The separately gated control forwarded 2534×1009 at the
matching boundary and retained that physical extent, without a window operation.
This is allocation evidence; the proof explicitly leaves pixel acceptance pending.

The current runtime-render-size default did not compile these DXGI resize
wrappers. The candidate now permits startup trace with `GEARS_RUNTIME_RENDER_SIZE`
while keeping `GEARS_GAME_RESOLUTION` disabled. Its prime lease installs both
ResizeBuffers entrypoints and its session lease accounts for both hooks. Only
the game-resolution mode runs the existing transport begin/finish, publisher
drain and inventory replacement logic. The runtime-size candidate records the
boundary, releases its fresh GetBuffer/QueryInterface references, optionally
substitutes certified physical dimensions for zero axes, and forwards the
original API exactly once with all other arguments preserved. It does not
modify the runtime render-size plan or issue a window API.

This remains a bounded private experiment, not a production-complete repair.
The shared startup recorder has 64 slots across create and resize calls. Once
full, both recording and the control stop: subsequent calls forward their
original dimensions without querying COM or the window. It cannot repair an
already tiny physical buffer or certify later resize/queue migrations. Default
trace/control options remain OFF. Added mode-gate and saturation assertions
require a fresh build and execution; source preparation alone is not acceptance.

Desktop mirror success waits for its draw fence and releases the destination
reference before returning. Its timeout path deliberately retains that reference
until a later completed fence is observed; the existing worker polls it. The
new runtime wrapper does not add a drain, release an unfinished GPU resource,
or invoke the game-resolution lifecycle. A later ResizeBuffers can therefore
still fail while the pre-existing timed-out mirror owns a buffer. Startup
allocation prevention does not establish timeout or runtime resize recovery.


## 2026-10-19: runtime-size startup correction verified

Fresh RuntimeMirrorFix1 SHA256 `7b9f0b02f41af35a479f44647f9ce4d82d1fca1d8e8b8d9b91f586b0a5e5a862` combines the runtime extent fix with bounded zero-axis startup control. GAME_RESOLUTION remains OFF. Fresh build verified470 dependencies; policy/control/mode/provenance and continuous startup tests passed.

Actual runs `runtime-mirror-fix1-actual1-20261019` (41592) and `runtime-mirror-fix1-actual2-20261019` (5492) each completed two checkpoint reloads, normal close, native/entry cleanup and settings restoration. Run1 caller4533ea requested0x0 with iconic background owned window/client0x0. Fresh GetDesc/GetBuffer agreed1280x720; substitution preserved actual post-call1280x720. Mirror1948 presents, zero timeouts/error/unsafe retirement. No window movement/activation was requested.

Independent desktop-mirror WARP pixel/state/handoff fixture passed10659 checks; debug layer unavailable. Actual-game screenshot acceptance remains outstanding: first request failed; second returned occluding desktop content. Neither is game pixel evidence; clear VR does not prove the desktop mirror.

Default manifest and named manifest-runtime-mirror-fix1 now select this tested startup correction; previous extent-fix manifest remains rollback. Trace capacity64 is still bounded; exhaustion disables subsequent control. This fixes the demonstrated startup shrink path, not every later resize anomaly. Old ZIPs do not contain this update.

Portable candidate now regenerated: `GearsVR-0.0.1-runtime-mirror-fixed-20261019.zip`, SHA256 `287d82d4decf6d24c1cf45d238680e86d42a117b3f71370d75c2b0a9d5f06b36`. Fresh stage and extraction into a path with spaces both passed isolated preflight;717 ZIP entries. This package includes the new default native DLL, recommended resolution/scale launcher and diagnostic export. It remains a candidate, not final0.0.1 acceptance. Receipt: ignored `analysis/runtime/runtime-mirror-fixed-receipt.json`.

## Private post-mirror pixel readback seam

A compile-gated GEARS_DESKTOP_MIRROR_READBACK seam now supports one explicitly armed copy of the actual destination immediately after the mirror draw on the same command list and fence. It retains pending GPU storage through timeout, hands off only after confirmed completion, and preserves completed evidence across source retirement without retaining the swapchain destination. No worker file output or production arming is wired yet; default behavior remains unchanged.

The expanded GpuMirrorTest WARP fixture passed12896 checks, including all supported formats/RGB10, padded rows, destination pixel comparison, single handoff, source retirement and timeout recovery. Debug layer was unavailable. This is fixture acceptance only: actual-game pixel capture remains pending integration of the private worker writer.

## 2026-10-19: actual desktop destination pixels accepted

RuntimeMirrorPixels2 private diagnostic build SHA256 `b190203dcc8f2f1f6f384dbff084ed8f8edacf8f51ca6e534c75ade5f92c9463` enabled the one-shot writer while retaining RuntimeMirrorFix1 startup control and runtime recommended-size rendering. Fresh build recorded473 dependencies and passed the mode/policy and continuous self-tests. Initial Pixels1 compilation exposed an unused disabled compute-snapshot stub referencing a gated type; removing that unused stub fixed the fresh build. No active compute diagnostic behavior changed.

Session `continuous-stereo-2504-20261019` is identified fully in ignored `runtime-mirror-pixels2-actual1-20261019.json`. The actual post-draw destination readback was1280x720, format24(R10G10B10A2), generation1/ticket3/draw serial1; source2560x1400. Mirror rectangle311,0,658,720 preserves the1280x1400 single-eye aspect ratio with side bars. PPM SHA256 `0e2991f0890e8365fd456915caf058d654bb737507aac42678fa3bc301205f3e`; preview `analysis/runtime/runtime-mirror-pixels2-desktop.png`. Visual inspection confirms distinct character, stone-wall and floor details, not an8x8 enlarged color field. This proves the completed desktop destination pixels for this frame independently of window occlusion; compositor visibility and all future frames are not claimed.

The same background, muted run passed both checkpoint reloads with new-world XR projections, normal game closure and exact protected settings/profile restoration. Final mirror counters:1995 presents,0 timeouts,0 error,0 unsafe. Worker image and metadata writes both succeeded. Diagnostic readback remains defaultOFF and the player manifest remains RuntimeMirrorFix1; no recurring readback/file IO is added to release playback. The observed startup8x8 degradation now has both allocation and actual pixel evidence of correction. Later unrelated resize anomalies remain outside this bounded claim.

---

## 中文

# 背景本地视图和输出大小

2026-10-18。这继续背景黑调查,而无需窗口激活或自动窗口移动。用户观察,移动游戏窗口恢复其画面仍然是调查引擎大小调整处理的主要触发器。它不会 确定需要的是重点本身。

## 固定字节和当前字节证据

现存 KB 文物 1 块块与现有的销售商Capstone一起被机械地出口。 `0x140000000`,代码区 SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`四项职能(`199d1c`, `1995d0`, `50ae6c`, `27f740`共计 4,645 字节包括记录的冷块。两个成功的活探测器都比对了每个保存的字节两次,并匹配了每个全功能散列。源块、散列和只读探测器仍然忽略在 `analysis/ida/background-size-20261018/`.

调度器通过本地视图列表在 RVAs 绘制 HWND `1c5eb38/1c5eb40`。一个独特的上市视图 `+98` 字段匹配自有的 HWND。它的 WM_SIZE 处理器可以绕过重塑数个州旗的大小; `+d0` 由窗口调整大小的常规设置/ 清除。 大小帮助器读取客户端的矩形而非 WM_ SIZE 的 lParam, 检查另一个虚拟条件, 并比较缓存宽度/ 高度后, 引用引擎调整大小的方法。 当前获取器指令 s 在视图端口建立缓存宽度/ 高度 `+80/+84`;独立窗口客户端的测量只在探针线程上使用了临时的DPI上下文。没有引用目标函数或写入游戏内存 。

## 背景运行

`background-viewport-state-20261018.json` 当真正的游戏前样出现时, 在有用的抓取之前, 停止了 。 它无法识别自动激活相对于手动切换。 稍后的关闭后搜索会正确拒绝未匹配的 HWND; 它的错误被保留。 探测器初始缺失 commerder.prepare呼叫在成功运行前被更正,没有作为游戏缺陷处理。它拥有的质值恢复条目;在正常的关闭超时后,只有身份检查的自有游戏终止,并恢复设置。

`background-viewport-state2-20261018.json` captured 16 一致的视图样本。输出观测结果从 2534×1009 to 8×8 大约 2.5 秒,然后返回到 2534×1009 测试时会发出大小通知或其他窗口突变。在实际的前景样本中没有出现游戏或自有的 XR PID。它停留在启动/菜单中,所以它不是游戏游戏接受。正常关闭和设置恢复通过。

`background-viewport-gameplay-20261018.json` 使用普通的本地语70 具有自动持续和观测性能微小输出监测的发射器。 8×8 已观察到过渡。 附近的本地窗口样本显示缓存大小 2560×1080,简略 2534×1009 with `+d0=1`,则 2560×1080 with `+d0=0`; 实际客户端大小 2534×1009。这些顺序观测不是哪个消息或线程改变 DXGI 输出的原子证据。

运行到一个已拥有的关卡播放器和活跃的原生立体声,然后录制 1,127 模拟预测提交和 517 现场复制完毕 12.5087 秒。最后最大提交缺口为 24.103 ms. 游戏也没有 17788 或 XR 键 8500 显示在前台样本中。正常所有者/ XR 停止、本地清理和 44 独立恢复的条目通过; 正常游戏关闭, 四个配置文件以及完整的 INI 被恢复。 会话是 `continuous-stereo-17788-20261018-014043-0bfadccd`.

由此建立了一条完整的背景启动/自动持续/定型会话路径,与当前 Python 和 冰冻的土著70 组合。它不能证明图像正确性、物理头集性能、挂载播放或不存在每个间歇启动故障。重复的短片 8×8 过渡显示为什么存在 2- 第二次恢复阈值可以在普通初始化期间起作用;它们没有确定恢复动作导致早期的持久性黑幕. 先前的WM_SIZE-仅限实验仍然是负结果.

回收政策现在需要5秒钟的单独、新鲜的微量产出观察,保留现有的2秒新鲜度/差距限制和每会一次的规则。3⁄4 ̄ ̧漯B 28 通过的重点测试,包括正常测试 +2.5- 第二小 – 正常回归, 从未检查或更改窗口, 精确的五秒推进- QPC 边界, 并且没有改变只观察行为。 自动测试继续使用只观察模式。 反馈3 ZIP早于此阈值变化,并保留其原始文摘.

## 私人运行时大小为零的候选线路(2026-10-19)

拯救的人类 `analysis/runtime/background-zero-extent-actual-20261018-proof.json`
隔离一个分配边界: 未处理 `ResizeBuffers(0,0)` 更改a
最新核实 2534×1009 缓冲到 8×8 当背景窗口是图标时
带一个 0×0 客户端。发送的单独门控 2534×1009 现场
匹配边界并保留该物理范围,不进行窗口操作。
这是分配证据;证据明确将像素接受问题留待处理。

当前运行时的递归大小默认未编译这些 DXGI 调整大小
包头。候选人现在允许启动跟踪 `GEARS_RUNTIME_RENDER_SIZE`
保持时 `GEARS_GAME_RESOLUTION` 已禁用。它的主租借安装了两种设备
调整缓冲点大小及其会话租赁均包含两个钩。只有
游戏解析模式运行现有的传输开始/完成,发布器
排水沟和库存替换逻辑。运行时大小的候选人记录
边框,可选择发布其新的 GetBafer/查询界面引用
将经认证的物理维度替换为零轴,并转发
原API 与所有其他参数完全相同一次保留。 它没有
修改运行时间渲染大小计划或发布窗口 API。

这仍然是一次有限度的私人实验,而不是生产完成的修复。
共享启动记录器有 64 跨创建和更改呼叫大小的插槽。一次
完全录音和监控停止:随后的呼叫
原始维度不询问 COM 或窗口。 它无法修复
已存在微小物理缓冲或验证后期的调整大小/队列迁移。默认
跟踪/控制选项仍然关闭。添加模式门和饱和度断言
需要新建和执行;仅源头准备不能被接受。

桌面镜像成功等待其绘图栅栏并释放目的地
返回前的引用。其超时路径有意保留该引用
直至观察后来完成的围栏; 现有工人投票。
新的运行时间包装器不会添加排水槽,释放未完成的GPU资源,
或引用游戏分辨率生命周期。因此,后期的调整缓冲器可以
仍然失败, 而先前存在的超时镜像拥有缓冲器。 启动
拨款预防不能确定停工或运行时间调整恢复规模。


## 2026-10-19: 运行时大小启动校正已校验

新鲜运行时间MirorFix1 SHA256 `7b9f0b02f41af35a479f44647f9ce4d82d1fca1d8e8b8d9b91f586b0a5e5a862` 将运行时间范围修复与限值零轴启动控制相结合。 GAME_RESOLUTION 仍然关闭。 已验证新构建470 依赖性;政策/控制/模式/证明和连续启动测试通过。

实际运行 `runtime-mirror-fix1-actual1-20261019` (41592 和 `runtime-mirror-fix1-actual2-20261019` (5492)每个检查点完成了两个重新装入,正常关闭,本土/入口清理和设置恢复。运行1 caller4533 ea 请求0x0 带有图标背景的窗口/客户端0x0 同意新鲜的GetDesc/GetBuffer1280x720;替代保留了实际的召用后1280x720 镜像1948 提交、零超时/超时/不安全的退休。没有要求窗口移动/激活。

独立的桌面镜像 WARP 像素/状态/手动固定器通过10659 检查; 调试层无法使用。 实际游戏截图接收仍未完成 : 第一个请求失败; 第二个返回的 occluding 桌面内容。 游戏像素证据也不存在; 清除 VR 无法证明桌面镜像 。

默认的列表和命名的 Express- runtime- Mirror- fix 定义1 现在选择这个测试的启动修正; 先前的限定范围表仍然回滚。 Track capany64 仍然被绑定; 穷竭会禁用后续控制。 这可以修正已显示的启动缩进路径, 而不是以后每次调整异常大小。 旧 ZIPs 并不包含此更新 。

手提式候选人现在重新生成: `GearsVR-0.0.1-runtime-mirror-fixed-20261019.zip`, 莎莎 (英语).256 `287d82d4decf6d24c1cf45d238680e86d42a117b3f71370d75c2b0a9d5f06b36`新鲜的舞台和带空间的提取道都经过了孤立的飞行前;717 ZIP 条目。 此包包括新的默认本地 DLL、 建议的分辨率/ 比例启动器和诊断导出。 它仍然是候选的, 不是最终的 。0.0.1 接收: 忽略 `analysis/runtime/runtime-mirror-fixed-receipt.json`.

## 私人后镜像素回读接合

编译的 GEARS_DESKTOP_MIROR_REABACK 接合器支持镜像绘制在同一命令列表和栅栏上后立即装有明确武装的实际目的地。它保留了待处理的GPU存储器,通过超时保存,仅在关闭后手动关闭。rmed 完成,并保存完整证据,同时不保留互换链目的地。 尚未对工人文件输出或生产武装进行窃听;默认行为保持不变。

扩展的 GpuMiror Test WARP 固定器通过12896 检查,包括所有支持的格式/RGB10, 添加行, 目的地像素比较, 单向交接, 源退休和超时恢复。 Debug 层没有可用。 这只是固定接受: 实际游戏像素捕获仍然有待私营工人编剧的整合。

## 2026-10-19: 接受实际桌面目的像素

RuntimeMirrorPixels2 私人诊断楼 SHA256 `b190203dcc8f2f1f6f384dbff084ed8f8edacf8f51ca6e534c75ade5f92c9463` 启用了一发写器,同时保留运行时的MirrorFix1 启动控制和运行时间推荐大小渲染。新编组已记录473 依赖性并传递模式/政策和连续自我测试。初始像素1 编译 显示未使用的 compute-snapshot 树枝, 引用门形类型; 删除未使用的树枝, 将新建筑固定。 没有改变主动的计算诊断行为 。

Session `continuous-stereo-2504-20261019` 完全忽略其中的识别 `runtime-mirror-pixels2-actual1-20261019.json`。实际绘制后目的地重新读取为1280x720,格式24(R10G10B10A2),生成1/ticket3 画序1; 来源2560x1400 镜像矩形311,0,658,720 保留1280x1400 单眼侧边栏的视宽比。 PPM SHA256 `0e2991f0890e8365fd456915caf058d654bb737507aac42678fa3bc301205f3e`预览 `analysis/runtime/runtime-mirror-pixels2-desktop.png`视觉检查证实其特征、石墙和地板细节,而不是8x8 扩展色彩字段。 这可以证明已完成的桌面目标像素独立于窗口封存; compositor 可见度和所有未来框架不要求 。

同样的背景,哑巴运行通过两个检查点的重新装入,并带有新世界XR投影,正常游戏关闭和精确的保护设置/外观恢复. Final 镜像计数器:1995 presents,0 timeouts,0 error,0 不安全。 工作者图像和元数据都成功。 诊断回读仍然为默认的OFF, 玩家的列表仍然是 runtimeMirorFix1; 没有添加重复的 read back/ file IO 以释放播放。 已观察到的启动8x8 223. 索赔人说,由于伊拉克入侵和占领科威特,它无法在索赔中找到任何证据。
