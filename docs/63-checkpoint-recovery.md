# Checkpoint Restores modified log

2026-10-15. Prioritization: 1 with process check point restored and initial access delayed; 2 pistols are on the same planet; 3 HUD size/position; 4CG and play black screens.

## Implemented

- LOCAL does not publish empty samples and exits from continuous sessions. Press OpenXR `changeTime`Update space generation after entry into force; original camera re-creates reference and the receiving side rejects old generation images. Only new generation succeeds`xrEndFrame`The record is restored.
- Camera readiness check is consistent with the original anchor check; only read certificates are retrieved when temporarily switching from the pre-activated camera, without retrying the injection or activation that has taken place.
- The initial report distinguishes between XR pre-screening, camera waiting, birth preparation, running state and actual first intervention.
- Protocol companion version: Stereo Bridge 3 GPU share 4. The new binary must be used in conjunction with the new binary;`manifest-game-recovery1.json`It's the current round of experiments, and the player defaults to keep the original version.

## This is the evidence.

Local 16 Passed certification orders (in-gen door/bridge, life cycle, OpenXR CPU and shared map checks, Python launcher/runner/menu monitoring); prepared for re-targeting 12 Item passed. No real headshot acceptance.

Practical game PID 10940,Runbook`analysis/runtime/continuous-stereo-10940-20261015-171139-11827aa4.*`: Autorecovering readiness after two cameras are not Pawn rejected; XR pre-screening 0.817 sec, menu/cam waiting 17.682 Seconds, originals.15.278 Seconds, first eye submission range. Runner.36.776 sec. Cumulative 10099 Sub-image copy,19813 Sub-submission, then stop.

Opened the pause menu with game status read and orientation window messages to confirm`ConfirmCheckPointLoad`, same PID reload check point. Test failed: native continues running, anchor refuses 0, but the new image was not restored.

Freezing log identifies another direct defect: swapchain first taken from 3840×2160 Change 1920×1080 The inventory monitor is permanently re-projected, and the target of the computer is reconstructed.`changed=10918`This is the number of subsequent rejections, non-resideze; shared texture and XR eye textures that support only initial size. The image cannot be restored with process running or simulator FOCUSED. At exit, native resources are drained, hook restoration, XR exit completed.

## Resize Recovery Increase (GameRecovery)2 / SpaceRecovery 2）

DXGI ResizeBuffers/ResizeBuffers 1 The border has now been upgraded with inventory and retirement old bills, and the GPU shared resources and two XR eye exchange chains have been re-established by consumer visible release.5 Failed reisize to re-validate old stocks; successive reisze reserves the same restoration rights as publicsher. Actual GPU tests cover lift and drop size, failed call, unconsumptioned end frame and continuous reconstruction;17 All authentication orders passed. Report.`analysis/runtime/game-recovery2-validation.json`。

Actual PID 12832，`continuous-stereo-12832-20261015-175419-4048fc7e.*`：1080 p Continue with the process to confirm the reload check point.1920×1080→1280×720→1920×1080→3840×2160 Resource successions 3/5/7 Successful xrEndFrame; first time two generations submitted a distance window operation 121/150 msec,4 K-Choose 1.112 sec.8 Sub-bottom resize succeeded, inventory was made/reprojected 0 All GPUs are re-establishedprepare successfully. Original emptiness, restoration of the book, XR exit pass. This record is still not stable: subsequent VISIBLE status is mistook as inactivated and continues to be withheld; users report repeatedly short and black.

♪ The first time I was in a shortage, the old ticket age was by 90 milliseconds have increased to 590 msec, by 100 millisecond age threshold rejected frame by frame; new images are returned immediately. CSV is XR_SESSION_STATE_VISIBLE () during continuous black screen 4)、shouldRender=1, the existing dual-purpose control only accepts COCUSED (5 This shall not be considered a failure of reesize, nor may it be declared that item 1 or item 4 has been fully resolved.

Another limitation: PID 23276 Use non-standard window client size 5094×2809、1894×1009 And then the game was still set.1920×1080 Output, add the final scaling of draw outside consume and no ticket. Rebuilds a successful but uncertified final output, which requires orientation to track the zoom input; it cannot be supplemented with an updated instrument.

Initial delay still around 17 Second-native preparation; four one-time count times have been added for self-measurement, initial resolve, certificate acquisition and activation. The player defaults with GUI are not yet in the process of building this experiment.

## GameRecovery 3 / SpaceRecovery 3）

Double-eye rendering now accepts VISIBLE and FOCUSED; Focus still indicates input focus, add a new visible field to display permission, should render and track check keep. Protocol is supported by StereoBride 4 GPU share 6。

Allow maximum number of continuous receipt of completed new images 1 Second age, complete copying of images can be preserved to the maximum 2 Seconds. Each submission continues to use the image ' s original rendering/FOV and rechecks the session, spatial generational, menu recovery time and eye area; no new head attitude may be posted on old pixels. New images can continue using valid old maps only if rejected for expiry; structural errors are still invalid.100 The millisecond threshold remains unchanged. This only alleviates the frame of shortage and does not resolve the lack of new scene output for a long time.

17 Passed local authentication command (`analysis/runtime/game-recovery3-validation.json`; the additional VISIBLE test is passed. 39 Item, preparatory phase 12 The project is adopted, the GUI is re-structured and self-measured.256 `ce00b4e1c3b682c64e9d198b0e34cba850a1b1fe73589f3ffd9b7c54bda7d983`，XR SHA 256 `86d0b092a6f32e97f52dfb44f814a06cbb53c0bf7b203ff605bd2e38a611868f`。

Practical game PID 11236，`continuous-stereo-11236-20261015-183342-4e82a424.*`: directed WM_ACTIVATE to actuated XR CSV after the VISIBLE preview report =4、shouldRender=1, native continues to publish and fully inactivate=0. Stabilization segment cumulative 5856 Repeat, no startup without player records.1080 p checkpoint reloads to continue the figure but there is a lack of player; no blackness can be claimed at all.4 K and back 1080 p After new resources are in place 3/5 First time distance engagement successfully submitted 160/159 milliseconds.4# The reisize is successful,5 Subprepare succeeded, inventory was made/reprojected 0。

4 K still has an unresolved gap: last new ticket 6497♪ I'm gonna be a little bit more than a little bit too late for that 679615352700 Next, new ticket.6977 Yes 690448683100 I'm sorry, sir.10.8 No new images are available for seconds. The maximum XR submission gap is available after the time is spent 8.878 sec, then recover. Generous cumulative contamination frame 1836、missing-pair 495、metadata-lost 27、unknown-list 13; only the reasons for the candidacy are such that existing early stocks cannot link specific HUD drew to this gap.7410 Shortfall frame submission 3113、late-images-rejected=0. Resources are drained at normal stop, the book recovery and XR exit pass; the overall test was not passed due to long gaps.

First access to runner.37.839 sec. native ready 14.931 In seconds, self-measurement 4.235 Second, first call 0.852 seconds, certificate acquisition 9.369 sec (4- A few attempts, of which 3 Second non-Pawn, activated 0.365 seconds. This delay has not been optimized.

Current`manifest.json`and`manifest-game-recovery3.json`Unanimously, GUI and SimulatorVR.ps 1 Use it uniformly; keep the game-resolutory list rolling back. Still locking down DLSS/DLAA at game resolution. It is an experimental check point, not a complete releaseable version. No real header acceptance.

## GameRecovery 4 / SpaceRecovery 3）

Add a fixed capacity window that works only when theoutage diagnosis is open:500 nms no new release triggers recording unknown draw, material release, existing overlay query results and inter-zone left-and-right eyeticket; no additional image admission rules, no rendering path allocation or documentation.9 Targeted inspection passed, list`manifest-game-recovery4.json`Freezing Independently, still default to Recovery 3。

Actual PID 14480，`continuous-stereo-14480-20261015-184923-98084e66.*`: Automatic 1080p→4K、4 Reloading of K Checkpoint 1080p。4 Subreside/5 subprepare all successful; XR copy 3540 Zhang, cumulative 8821 Sub-proceeding, maximum submission gap 59.055 Seconds, whole failed.4 K-to-K, first time you submit a resource until the pause menu is open 3 The cause and effect mechanism is not yet defined.41 All accesses are back through and the game is closed.

The last three short gaps provided direct source evidence: first_unknown and Trigger share of left-and-right ticket, Precent refused the position 128(pollotted); first unknown draw from RVA 1af 345，4 vertex non-indexed Materal, then 172741 The normal UI query becomes missing_pair.4486 All the Materal candidates/consumer/quad have been completed; metoral_kind=0、invocation=0 This means that no specific script HUD classification can be obtained and that it cannot be declared a quasi-star or a new gun UI. The current branch of the game-resolution only observes this type of material without being exported by eye, so these frames are conservatively rejected.

Current round 19 Early records in each gap are the last of the rings.4 Entries are covered, the longest is lost 4 K window cannot directly attribute the specific draw identity of short gap to long jap. Additional maximum windows are retained and directed to complete the upload, IA, active projection, eyeticket and consumer authentication.`analysis/runtime/game-recovery4-audit.json`。

Another follow-up UI recovery gap: the operating anchor has accepted new Pawn (family/renderer)2065(a) but the cumulative rejection of PostRender UI scope is 2065- Second.`ui_observer.h::quick_environment`The Pawn and Inventory addresses are still required to be activated. They are highly relevant, not yet called for matching; the level of the UI certificate needs to be re-linked and cannot be restored by de-identifying it. It may result in the loss of HUD classification after loading and fall within the scope of subsequent UI restoration.

## Material-Element Incremental Output (GameRecovery)5 / SpaceRecovery 3）

Independent Default Closed`GEARS_GAME_UI_MATERIAL`Requires a game-resolution and Materal observation that still prohibits fixed downed targets. Only the full verified functions of Hash are added 1af 054 Uploads the book, reuses actual non-indexed quad, native list, stride 32/topology 5 The original consumer and uploading are performed once; only the certified GPU drew is replayed, restoring the video/scissor.0 Keep it ordinary overlay, not claiming it's HUD;kind 2/3 with HUD;kind 1 The star is not supported. Present refuses to be a window when consumer has not finished, and if he fails, the window is contaminated without blocking; registration failed after re-playing does not replace the original.

GameRecovery 5 Freezing of compilers and 13 Check passed, list.`manifest-game-recovery5.json`;doing not replace defaultRecauver 3. Add a maximum gap to the independent reservation, which will not be covered by subsequent short gap. The actual first round of PIDs 17836 Completed 505 Minor eye re-painting (averagekind)3) Upload 508、3 No, I'm not.1505 Contamination.5= stocked/subjected 0、pending Present=0. The stabilizer simulator does not have a startup and defaults the player event. Test in cut 4 K was stopped before reading MenuReader found changes in the object list and therefore not complete 4 K/reload pass; normal closing of game after exit. Diagnostic control scripts are allowed to read the competition at most before any input is sent 3 New reader re-sampling, without retrying.

### Full scene retrospect and current entrance

Same GameRecovery 5 Run PID again 30400，`continuous-stereo-30400-20261015-190433-c56864c6.*`,Auto complete 1080p→4 K (staying)55 s) Reload with process check point (suspend)25 sec 1080 p. native release 6724 XR copy 6723, submission 13427 Range of scene submission 150.613 Seconds. Materal redraws the eyes.2305 Number of times (kind)3 HUD 1925、kind 0 Normal Overlay 380），21 Once a pair does not reject, pending Precent =0;contaminated frame from the top 3483 Down to current cycle 15(The two rounds of work are different and not directly considered as a percentage of performance).4 Subresize and 5 Subprepare was successful, inventory changed/projected=0，4 K and back 1080 p New generation first time submitting distance engagement 126.6/193.5 milliseconds.

Previous 59.055 The second-long submission gap has not been repeated, with the maximum XR submission interval down to 733.834 milliseconds; continuing criteria are not met and the report is clearly incomplete, which cannot be said to have all black screens repaired. The maximum primary release interval is approximately 2.023 Seconds occur after confirmation of check point, containing load interval with temporary eyeless pair; old diagram is kept short 186 No date-image rejections occurred.42 The entry point is independently restored for clearance, the native resource is drained, XR object destroyed and then the game is closed.

First time in the cycle, double-eye runner.36.484 sec. Natural self-measurement 4.300 Second, first call 0.773 Seconds, certificates ready.11.704 sec (5 A few attempts, waiting explicitly for Camera.ViewTarget to resume as controlled Pawn, active 0.364 sec. The initial delay cannot therefore simply be attributed to the GPU slow; support/show strategy for non-Pawn camera phases still needs to be investigated and anchord verification cannot be cancelled.`analysis/runtime/game-recovery5b-audit.json`。

Current`manifest.json`Upgrade to`manifest-game-recovery5.json`, GUI/Script read it directly;3 With original GameResolution 1 Keeps the list rolling back.256 `eebe351a8de54c363446caa1b0dbfc8f81eabcecf3c8ed3c3633320b2aa98127`XR is still SpaceRecovery 3. Item 1 still needs to address initial access delays and transition shortfalls; Division 2/3 is not complete with the HUD and Block 4 CG.

## UI Re-locking Level (GameRecovery)6/7）

The UI static function/ attribute/class certificate remains unchanged. Each matching script UI entry takes the current Pawn, HUD, Inventory binding of the same level, verifys the full object/class identity and two-way relationship, freezing the private configuration to that call. Embedded UI reuses the frozen value, and then rechecks after.512 The slot fixed receipt book retains the original call binding; the queuing of the draw consumption only keeps HUD/Astrostar classification when the original binding has been completed and is still valid, and sends, overwrite or change roles down to certified normal overlay and does not remark old records. The consumption end check again, the semantic window that has not yet been completed is not published; no new hook is added.

GameRecovery 6 / PID 29844 Reload and stopover of two check points completed with the same process 4 K and back 1080 p. UI calls for access 5469 Number of refusals 0 New Pawn reconnection 4108 Number of times; queued 28915 Adopted, pending/retified =0、changed=1 End of rejection 3 Time-tamp competition 4. Materal draws HUD-by-Eye 1842 No degradation tokind 0 Normal overlay. XR submits 11943 Sub-reproduction 5185 Zhang, the horizon 134.453 Seconds, maximum interval of submission 867.687 milliseconds; still incomplete.42 The entrance is restored and XR exit completed, followed by normal closure of the game.`analysis/runtime/game-recovery6-audit.json`。

The review and the round record found that new binding failed to be merged`ui_hud_sequence_fault`, causes the temporary change of role/time stamp to permanently close the Tile HUD route (the round is divided_fault=)1）。GameRecovery 7 Amend to allow permanent fault to be triggered by only the original GPU sequence/return value structure error; binding failed only by rejecting the current window and retaining subsequent recovery. Legacy owned route also passed binding to its original strict cessation mechanism; this project configuration does not enable downed route, nor claims that it is supported.

Freezing of the data.6、Recovery 7 Separately adopted 12 local check of items;Recovery 6 First time misuse self-test parameters`--owned-prime-handoff`Back 2, to actually support it.`--prime-owned-handoff`, the original failure record is kept and not considered to be a malfunction.7 DLL SHA 256 `964ec52a8a23c3b9b22e2a13dd5845e3ba78c667eead6ec1a8f03757f5127aa7`XR maintains SpaceRecovery 3. This does not claim that the star depth, HUD size or subtitles have been completed.

Recovery 7 / PID 26920 Reloading, reloading and reloading of two additional check points with the process 1080p→4K→1080 p. New Pawn UI reconnect 4339 Access 5637 Repeated refusal 0; queued and tied 28430 Adopted, pending/retired all 0、changed 9 End of rejection 4,timestemp.11. Tile HUD 10571 Second`sequence_fault=0`, Materal HUD-Eye Drawing 1259 Number of times, not down tokind 0. Four reeses were successful, stocked =0 Contamination.18 Frame. XR submits 11866 Sub-reproduction 5338 Zhang, the horizon 133.540 sec, maximum submission interval 866.434 milliseconds; shortage frame still in, report maintains incomplete. Clean-up, clean-up,42 The entrance is restored and XR exited, the game is normally closed.`analysis/runtime/game-recovery7-audit.json`。

Current Player`manifest.json`Update to Recover 7，Recovery 5/6 The list is separate and the DLL reservation is frozen.70°/2 The rice parameters have not been declared integrated or supported by the CG. Two successive rounds of physical testing cover repeated reloading and restoration of UI binding; the remaining opening field, CameraActor, continues to operate with a start-up plane transition and short black screens continue along the subsequent stability/CG route.

## Next

The follow-up check found that the old Runner's incomplete may also have been from a succession mode error: it required one_way=true, while the XR that enabled the menu to be restored was clearly false. The exit reporting condition has been modified according to actual configuration. See[Notes to the withdrawal report](64-reticle-and-ui-capacity.md). The original running report and spacing measurement on this page is retained; the black frame cannot be determined by the old incomplete label alone, nor can the report correct the measured interval.

### Initial waiting camera identity (PID)4776）

GameRecovery 5 Keep binary and only keep the full object/class identity of non-Pawn objects and the last reread result in a read-only record of anchor failure.`continuous-stereo-4776-20261015-191911-0fc4a160.*`The first and last refusals are the same.`CameraActor_1`(Category`CameraActor`) Camera time stamp 8.85 to 13.73 Seconds,`consistent_read=true`. The wheel is therefore waiting for the transition of independent CameraActor, which cannot be explained as an overlap of left and right eyes or a GPU outlay, nor can Pawn constraints be lifted without certification.

The launch plane has been submitted, about 36.275 sec first successful projection; short observation segment after continuous dual-eye 13.247 seconds,1195 Second submission, maximum interval 24.13 milliseconds. Natural cleaning,42 The short observation does not cover CG switching and complete stability check.25 Enochor and 12 The block is ready to be tested.

The 2nd physical pistol opener permanently ceased to function after the UI batch was fully positioned, resulting in a quasi-star correction and the HUD layout being disabled.8 Repair and re-detection, current player list upgraded; evidence and remaining scope available[Pistols are on the star side with UI capacity.](64-reticle-and-ui-capacity.md). Existing w=0 The third re-alignment of guns UI, subtitles and the fourth CG/load display and shortfall frame are confirmed. First reloading has been repeated several times; the waiting CameraActor identity for first access has been confirmed without eliminating all start delays. The unknown Delaw acceptance or indefinite extension of the old map remains unrestricted. The VDXR event has not yet been proven to be back on track.

References:[OpenXR Sessions Visible and Input Focus Status](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSessionState.html)、[OpenXR Reference Space Change Event](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrEventDataReferenceSpaceChangePending.html)、[DXGI ResizeBuffers Resource Release Requirements](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)。

---

## 中文

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

本轮首次双目距runner开始36.484秒。原生自测4.300秒、首次resolve0.773秒、证书准备11.704秒（5次尝试，明确等待Camera.ViewTarget恢复为受控Pawn）、激活0.364秒。因此首入延迟不能简单归于GPU慢；非Pawn相机阶段的支持/显示策略仍需调查，不能取消anchor关系校验。运行摘要`analysis/runtime/game-recovery5b-audit.json`。

当前`manifest.json`升级为`manifest-game-recovery5.json`，GUI/脚本直接读取它；Recovery3与原GameResolution1清单保留回滚。native SHA256 `eebe351a8de54c363446caa1b0dbfc8f81eabcecf3c8ed3c3633320b2aa98127`，XR仍为SpaceRecovery3。第①项仍需处理首次接入延迟和切换短缺帧；第②/③项准星与HUD以及第④项CG未完成。

## UI同关卡重绑定（GameRecovery6/7）

UI静态函数/属性/类证书保留不变。每个匹配的脚本UI入口获取同关卡当前Pawn、HUD、Inventory绑定，校验完整对象/类身份和双向关系，把私有配置冻结到该次调用。嵌套UI复用冻结值，结束后重验。512槽固定receipt账本保留原调用绑定；排队draw消费时只有原绑定已完成且仍有效才保留HUD/准星分类，pending、覆盖或换角色降为已认证普通overlay，不重新标注旧记录。消费结束再次检查，尚未完成的语义窗口不发布；无新增hook。

GameRecovery6 / PID29844完成同进程两次检查点重载、中途4K及回1080p。UI调用获取5469次、拒绝0、新Pawn重关联4108次；排队绑定28915次通过、pending/retired=0、changed=1，结束拒绝3、时间戳竞态4。Material逐眼HUD绘制1842次，未退化为kind0普通overlay。XR提交11943次、复制5185张，跨度134.453秒、最长提交间隔867.687毫秒；仍为incomplete。原生清理、42个入口恢复和XR退出完成，随后正常关闭游戏。证据`analysis/runtime/game-recovery6-audit.json`。

审查及该轮记录发现新增binding失败被合并到`ui_hud_sequence_fault`，造成瞬时换角色/时间戳竞态永久关闭Tile HUD路线（该轮sequence_fault=1）。GameRecovery7修正为只让原有GPU序列/返回值结构错误触发永久fault；binding失败只拒绝当前窗口并保留后续恢复。legacy owned路线同时传递binding失败到其原有严格停止机制；本项目game-resolution配置不启用owned路线，不宣称owned可恢复性已支持。

独立UI dev编译及针对性CPU检查通过。冻结Recovery6、Recovery7分别通过12项本地检查；Recovery6首次误用self-test参数`--owned-prime-handoff`返回2，改用实际支持的`--prime-owned-handoff`后通过，原失败记录保留，未当作实现故障。Recovery7 DLL SHA256 `964ec52a8a23c3b9b22e2a13dd5845e3ba78c667eead6ec1a8f03757f5127aa7`，XR保持SpaceRecovery3。此处不宣称准星深度、HUD尺寸或字幕实机验收已完成。

Recovery7 / PID26920再次完成两次同进程检查点重载、1080p→4K→1080p。新Pawn UI重关联4339次、scope获取5637次拒绝0；排队绑定28430次通过，pending/retired均0、changed9，结束拒绝4、timestamp竞态11。Tile HUD实际调整10571次且`sequence_fault=0`，Material HUD双眼绘制1259次，未降为kind0。四次resize均成功，库存changed=0，污染18帧。XR提交11866次、复制5338张，跨度133.540秒，最长提交间隔866.434毫秒；短缺帧仍在，report保持incomplete。清理、42入口恢复及XR退出通过，游戏正常关闭。摘要`analysis/runtime/game-recovery7-audit.json`。

当前玩家`manifest.json`更新到Recovery7，Recovery5/6独立清单与冻结DLL保留。没有更改HUD的70°/2米参数，也没有宣布准星已融合或CG已支持。已在连续两轮实际测试覆盖同进程重复重载及UI绑定恢复；剩余开场CameraActor仍用启动平面过渡，短黑屏按后续稳定性/CG路线继续处理。

## 下一步

后续核对发现，旧runner的incomplete还可能来自接续模式判定错误：它要求one_way=true，而启用菜单恢复的XR明确为false。该退出报告条件已按实际配置修正，见[退出报告说明](64-reticle-and-ui-capacity.md)。本页原始运行报告与间隔测量保留；不能仅凭旧incomplete标签认定发生了黑帧，也不能用报告修正否定实测长间隔。

### 首入等待的相机身份（PID4776）

GameRecovery5保持二进制不变，只在只读anchor失败记录中保存非Pawn目标的完整对象/类身份与末次重读结果。实际`continuous-stereo-4776-20261015-191911-0fc4a160.*`，首次和末次拒绝均为同一`CameraActor_1`（类`CameraActor`），相机时间戳约8.85至13.73秒，`consistent_read=true`。因此该轮确实在等待独立CameraActor过渡，不能解释成左右眼重叠或GPU出图故障，也不能未经验证直接取消Pawn约束。

启动平面已经提交，约36.275秒首次成功projection；进入持续双目后的短观察段13.247秒、1195次提交，最大间隔24.13毫秒。原生清理、42个入口恢复和XR退出通过，游戏正常关闭。该短观察不覆盖CG切换及完整稳定性验收。失败证据仅作诊断，未改变接纳条件；25项anchor及12项启动准备测试通过。

第②项实际手枪开镜已定位UI批次表满后永久停用，导致准星方向校正和HUD布局失效。Recovery8修复并复测，当前玩家清单已提升；证据及剩余范围见[手枪准星与UI容量](64-reticle-and-ui-capacity.md)。现有w=0无限远方向仍不等于真实命中点。接下来确认第③项换枪UI、字幕，及第④项CG/加载显示和短缺帧。第①项同进程重载已多次恢复；首次接入等待的CameraActor身份已确认，未消除全部启动延迟。不放宽未知draw接纳或无限延长旧图保留。尚未证明VDXR原点事件在真实头显上恢复。

参考：[OpenXR会话可见与输入焦点状态](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSessionState.html)、[OpenXR参考空间变化事件](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrEventDataReferenceSpaceChangePending.html)、[DXGI ResizeBuffers资源释放要求](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)。
