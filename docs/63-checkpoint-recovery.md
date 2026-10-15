# 检查点恢复修正记录

2026-10-15。优先顺序保持：①同进程检查点恢复与首次接入延迟；②手枪准星；③HUD尺寸/位置；④CG与游玩黑屏。当前未完成第一版验收。

## 已实施

- LOCAL参考空间变化不再发布空样本并退出持续会话。按OpenXR `changeTime`生效后更新空间代次；原生相机重新建立参考，接收端拒绝旧代次图像。只有新代次成功`xrEndFrame`才记录恢复。
- 相机就绪检查与原生anchor检查统一；激活前相机暂时切换时只重取完整读取证书，不重试已发生的注入或激活。保留取消、身份、签名和时间上限。
- 启动报告区分XR预检、相机等待、原生准备、running状态和实际第一张projection。
- 协议配套版本：StereoBridge 3、GPU共享包4。新二进制必须配套使用；`manifest-game-recovery1.json`是本轮实验，玩家默认仍保留原版。

## 本轮证据

本地16个验证命令通过（原生门控/桥接、生命周期、OpenXR CPU与共享映射检查、Python launcher/runner/menu监测）；准备重取专项12项通过。无真头显验收。

实际游戏PID10940，运行记录`analysis/runtime/continuous-stereo-10940-20261015-171139-11827aa4.*`：两次相机非Pawn拒绝后自动恢复准备；XR预检0.817秒、菜单/相机等待17.682秒、原生准备15.278秒、首次双眼提交距runner开始36.776秒。累计10099次图像复制、19813次projection提交，之后停更。

已用游戏状态读取和定向窗口消息打开暂停菜单，再确认`ConfirmCheckPointLoad`，在同一PID重载检查点。测试未通过：native继续running、anchor拒绝0，但新图像没有恢复。

冻结日志确认另一个直接缺陷：swapchain首次从3840×2160变1920×1080时，buffer对象也重建。库存观察器永久rejected，`changed=10918`是后续拒绝次数，非resize次数；共享纹理和XR眼纹理仍只支持初始大小。不能以进程运行或模拟器FOCUSED宣称画面恢复。退出时native资源排空、hook恢复、XR退出完成。

## Resize恢复增量（GameRecovery2 / SpaceRecovery2）

DXGI ResizeBuffers/ResizeBuffers1边界现已更新库存代次、退休旧票据，并以consumer显式释放握手重建GPU共享资源和两条XR眼交换链。GPU包版本5。失败resize重新验证旧库存；连续resize保留同一publisher的重建保留权。实际GPU测试覆盖升降尺寸、失败调用、未消费末帧和连续重建；17个验证命令全部通过，报告`analysis/runtime/game-recovery2-validation.json`。

实际PID12832，`continuous-stereo-12832-20261015-175419-4048fc7e.*`：1080p同进程确认重载检查点后继续提交。精确client尺寸1920×1080→1280×720→1920×1080→3840×2160分别出现资源代次3/5/7的成功xrEndFrame；前两次新代首次提交距窗口操作约121/150毫秒，4K约1.112秒。8次底层resize成功，库存changed/rejected均0，所有GPU重建prepare成功。原生排空、hook恢复、XR退出通过。此记录仍不是稳定性通过：后续VISIBLE状态被错误当作inactive导致持续不提交；用户同时报告反复短闪黑。

闪黑的独立证据：首个短缺帧期间，旧ticket年龄由90毫秒增长到590毫秒，被100毫秒年龄门槛逐帧拒绝；新图像到达立即恢复。持续黑屏期间CSV为XR_SESSION_STATE_VISIBLE(4)、shouldRender=1，现有双目门控却仅接受FOCUSED(5)。不得把这些当作resize失败，亦不得宣称第①或第④项已全部解决。

另一个限制：PID23276用非标准窗口client尺寸5094×2809、1894×1009时，游戏原生双目仍按1920×1080输出，新增最终缩放draw在consume之外且无ticket。重建成功但没有认证的最终输出，需定向追踪该缩放输入；不能给它补最新票据。

首入延迟目前仍约17秒native准备；已新增自测、首次resolve、证书获取、激活四段一次性计时，尚未优化。玩家默认与GUI仍未切换到本实验构建。

## 可见状态与短缺帧修复（GameRecovery3 / SpaceRecovery3）

双目渲染现在接受VISIBLE和FOCUSED；focused仍表示输入焦点，新增visible字段用于显示许可，shouldRender和追踪校验继续保留。协议配套为StereoBridge 4、GPU共享包6。

持续模式接收完成的新图像时允许最多1秒年龄，已完整复制的图像可保留最多2秒。每次提交仍使用该图像原始渲染姿态/FOV，并重新检查session、空间代次、菜单恢复时间和眼区；不得把新头部姿态贴到旧像素上。新图像仅因过期被拒绝时可继续使用有效旧图；结构错误仍失效。原生新相机样本的100毫秒门槛不变。这只缓解短缺帧，不解决长时间没有新场景输出。

17个本地验证命令通过（`analysis/runtime/game-recovery3-validation.json`）；随后补充的VISIBLE ticket-store测试通过。Launcher 39项、准备阶段12项通过，GUI重新构建并通过自测。冻结DLL SHA256 `ce00b4e1c3b682c64e9d198b0e34cba850a1b1fe73589f3ffd9b7c54bda7d983`，XR SHA256 `86d0b092a6f32e97f52dfb44f814a06cbb53c0bf7b203ff605bd2e38a611868f`。

实际游戏PID11236，`continuous-stereo-11236-20261015-183342-4e82a424.*`：定向WM_ACTIVATE让自有模拟器预览报告VISIBLE后，actual XR CSV为state=4、shouldRender=1，native持续发布且全程inactive=0。稳定段累计5856次projection，无启动后缺layer记录。同进程1080p检查点重载继续出图，但存在一次缺layer；不能宣称完全无闪黑。精确client切4K和回1080p后，新资源代次3/5首次成功提交距操作约160/159毫秒。4次resize成功、5次prepare成功，库存changed/rejected均0。

4K仍有未解决缺口：最后新ticket6497的样本时间679615352700ns，下一新ticket6977为690448683100ns，约10.8秒没有可用新图像。保留时间耗尽后，XR最长提交缺口8.878秒，随后恢复。原生累计污染帧1836、missing-pair495、metadata-lost27、unknown-list13；只是候选原因，现有早期库存无法把具体HUD draw与该缺口关联。全程发布7410，短缺帧保留提交3113、late-images-rejected=0。正常停止时资源排空、hook恢复和XR退出通过；整体测试因长缺口未通过。

首次接入距runner开始37.839秒。native准备14.931秒中，自测4.235秒、首次resolve0.852秒、证书获取9.369秒（4次尝试，其中3次非Pawn）、激活0.365秒。尚未优化此延迟。

当前`manifest.json`与`manifest-game-recovery3.json`一致，GUI及SimulatorVR.ps1统一使用它；保留game-resolution清单回滚。仍按游戏分辨率、锁定DLSS/DLAA。它是实验检查点，不是可发布的完成版。无真头显验收。

## 断帧来源实测（GameRecovery4 / SpaceRecovery3）

新增仅在outage诊断开启时工作的固定容量窗口：500毫秒无新发布触发，记录未知draw、Material receipt、既有overlay查询结果和同区间左右眼ticket；不增加图像接纳规则、不在渲染路径分配或写文件。9项针对性检查通过，清单`manifest-game-recovery4.json`独立冻结，默认仍为Recovery3。

实际PID14480，`continuous-stereo-14480-20261015-184923-98084e66.*`：自动1080p→4K、4K检查点重载→1080p。4次resize/5次prepare全部成功；XR复制3540张、累计8821次projection，最长提交缺口59.055秒，整体失败。4K切换后直到打开暂停菜单才首次提交资源代次3；此因果机制尚未确定。正常停止完成，41个入口恢复核验全部通过，游戏正常关闭。

最后保留的三个短gap提供直接来源证据：first_unknown与trigger均有相同的左右眼ticket，Present拒绝位为128（polluted）；首条未知draw来自RVA1af345，4顶点非索引Material，随后172741普通UI查询变成missing_pair。4486个Material producer/consumer/quad候选全部完成；material_kind=0、invocation=0意味着未获得具体脚本HUD分类，不能宣称是准星或换枪UI。当前game-resolution分支只观察这类Material而未逐眼输出，因此这些帧被保守拒绝。

本轮19个gap中的早期记录被环形最后4项覆盖，丢失了最长4K窗口，不能把短gap的具体draw身份直接归于长gap。后续额外保留最长窗口，同时定向补齐Material的上传、IA、正交投影、同目标双眼ticket和consumer完成认证。证据摘要`analysis/runtime/game-recovery4-audit.json`。

另一个后续UI恢复缺口：该运行中anchor已经接受新Pawn（family/renderer各2065次），但PostRender UI scope累计拒绝恰为2065次。`ui_observer.h::quick_environment`仍要求启动时Pawn和Inventory地址。两者高度相关，尚未逐调用配对；需为UI语义证书增加同关卡重绑定，不能用取消身份校验来恢复。它可能导致重载后HUD分类丢失，属于后续UI修复范围。

## 材质双眼输出增量（GameRecovery5 / SpaceRecovery3）

独立默认关闭的`GEARS_GAME_UI_MATERIAL`要求game-resolution和Material观察，仍禁止固定owned目标。只新增已校验完整函数哈希的1af054上传hook，复用实际非索引quad、native list、stride32/topology5、OM backbuffer、同目标完整双眼pair、冻结样本和正交Canvas投影认证。原consumer和上传各执行一次；只重放认证的GPU draw，恢复viewport/scissor。kind0保持普通逐眼overlay，不宣称它是HUD；kind2/3用HUD平面；kind1准星暂不支持。consumer尚未完成时Present拒绝当窗，失败则污染窗口，不阻塞等待；已重放后登记失败不再补原draw。

GameRecovery5冻结编译和13项检查通过，清单`manifest-game-recovery5.json`；不替换默认Recovery3。新增最长gap独立保留，后续短gap不会覆盖它。实际首轮PID17836完成505次Material双眼重绘（均kind3），上传508、3次pair不成立被拒绝；native发布1505，污染5、库存changed/rejected=0、pending Present=0。稳定段模拟器无启动后缺layer事件。测试在切4K前因只读MenuReader发现对象表变化而停止，因此不算完整4K/重载通过；退出完成后正常关闭游戏。诊断控制脚本只在发送任何输入之前，允许该具体读取竞态最多3次新建reader重采样，动作不重试。

### 完整场景复测与当前入口

同一冻结GameRecovery5再次运行PID30400，`continuous-stereo-30400-20261015-190433-c56864c6.*`，自动完成1080p→4K（停留55秒）→同进程检查点重载（停留25秒）→1080p。native发布6724，XR复制6723、projection提交13427，场景提交跨度150.613秒。Material逐眼重绘2305次（kind3 HUD 1925、kind0普通overlay 380），21次pair不成立拒绝，pending Present=0；污染帧从上轮3483降至本轮15（两轮工作量不同，不直接当作性能百分比）。4次resize与5次prepare均成功，库存changed/rejected=0，4K及回1080p新代首次提交距操作约126.6/193.5毫秒。

上一版59.055秒长提交缺口本轮未复现，最大XR提交间隔降为733.834毫秒；仍不满足连续性标准，最终report明确为incomplete，不能称所有黑屏已修复。最长原生发布间隔约2.023秒发生在确认检查点后，含暂时无双眼pair的加载区间；旧图短时保留186次，未发生late-image拒绝。停机时42个入口独立恢复核验通过、native资源排空、XR对象销毁完成，随后正常关闭游戏。无真头显或CG验收。

当前`manifest.json`升级为`manifest-game-recovery5.json`，GUI/脚本直接读取它；Recovery3与原GameResolution1清单保留回滚。native SHA256 `eebe351a8de54c363446caa1b0dbfc8f81eabcecf3c8ed3c3633320b2aa98127`，XR仍为SpaceRecovery3。第①项仍需处理首次接入延迟和切换短缺帧；第②/③项准星与HUD以及第④项CG未完成。

## 下一步

继续第①项首次接入延迟/切换短缺帧，随后第②项手枪准星和第③项HUD。UI语义证书在同关卡换Pawn后重绑定是后续覆盖所需；CG/加载状态仍需专门显示策略。不放宽未知draw接纳或无限延长旧图保留。尚未证明VDXR原点事件在真实头显上恢复。

参考：[OpenXR会话可见与输入焦点状态](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSessionState.html)、[OpenXR参考空间变化事件](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrEventDataReferenceSpaceChangePending.html)、[DXGI ResizeBuffers资源释放要求](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)。
