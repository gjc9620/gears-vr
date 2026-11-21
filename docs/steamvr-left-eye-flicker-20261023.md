# Left eye clarity jump:0.0.4 Accessed to independent historical state restoration

**Current Conclusion (Preliminary)2026-10-23）：** Already purchased version of the game/backstage simulator 7200×3200 Recreates the user's left eye path in the original stereo source. When ordinary rendering frames are interpolated with VR frames, old versions of VR left eye re-enactment of the player's historical state and right eye independence; when an independent primary state is created for VR left eye, the co-conforming changes removed the captured clear/ fuzzy jump. The default native has been switched to `build-left-history1`, normal starter marked as `0.0.4 Dev`;Colour policy maintained. Physical head start still pending acceptance by the user.[0.0.4 Development records](releases/0.0.4/PROGRESS.md)。

The following is a historical record of the corresponding phase of the survey, which was followed by original feedback and findings; expressions such as “causes unknown/not repaired”.

The user actually confirms that SteamVR appears to be clearer than VDXR under its current equipment and settings. The user then clarifies that the anomaly only occurs on the edge or in part of the left eye, and further corrects it as “should be luminous.” Based on this subsequent description, the previous "Flick/Slip" survey assumption is no longer a symptomal conclusion. It is currently observed only in this Mod; other applications are not independently screened and clarity differences are not the proven cause of flashing.

Previous feedback: User recheck confirmed that “is the problem of colour” is much clearer, while describing “left eye blinks as if a frame was white.” This complements the flash view of the moment by retaining previous margin/local observation and not treating “like a glass white” as the whole white evidence of what has been captured, nor from which sRGB is presumed to be the cause of the flash.

**2026-10-23 Recent corrections: User feedback "as if it all flashes, not colour problems." Both logs of original colour/UNORM and sRGB paths appear to be scintillated, with the color retreating unresolved; withdrawal of determination that colour format is a scintillating root cause or that the original colour is considered as non-scramble path, the root causes remain unknown.** Keeps user feedback on sRGB that is clearly clearer; clarity and scintillation are recorded separately, without completely excluding any unverified association.

**Recent image corrections: Users provide three simulators screenshots to clarify that “not flashing, but blurling into clarity” and confirm that desktop games are also flashing. Left eye clarity/verbal alternation has been captured in the current complete stereo source of manual sessions, right eye clarity remains clear, see end of text.** The results of the first three rounds of unrecovered white frame/light anomaly cannot be used as an exclusionary evidence for the clarity jump.

Users then requested that the "scrutinizing of this flash problem with an emulator" be resurveyed; previously, it was a historical decision to suspend as "preliminaryly recorded", which was replaced by requests for resumption. The current default maintenance of the restored source format/UNORM priority policy and retention of SRGR back-up paths required for SteamVR; no color switching or delivery experiment repairing due to this survey. Previously, the "no scintillation" had been retained as historical feedback and replaced by subsequent new observations, not being accepted without flashback.

**The subsequent users reconfirmed that “it is true that there is a flash of light only with the left eye” and replied, “It also happens when the head remains intact”.** The current symptoms are recorded as only left eyes, static periods and possibly instantaneous whites, keeping the previous edge/local description; turn-over is not a necessary trigger in user observations. There is no exact time frame for abnormality, which has yet to be shown to be the same as the outer dots of continuous frames in simulator scans.

## There is evidence of actual session

Use a live session started manually by the user `continuous-stereo-37508-20261023-180608-a0caee0c`, no extra game started for this investigation.

- Default native is `7571fee6…bab9`XR is `5f8d6769…fe3f5`;activated original stereo output.
- The actual source image is 6144×3264 Two-eye XR images, one each 3072×3264。
- I'll take it from the right to the left. 11,316 Number of times `xrWaitSwapchainImage`, no time lapses or errors; this is the application-end waiting result and cannot prove that there was no local white out.
- SteamVR synthesiser exists in the same session `WaitForAcquire timed out`But the first two appear. 18:05:56, while the Mod XR client is 18:06:09 Only when you connect. So these warnings are not direct evidence of Mod's left eye leak.
- No correspondence between a specific original image, XR submission and synth warning at the time of occurrence of the user anomaly can be determined which step in generating, converting, synthesizing or VD displaying local flash white.

## Current round of supplementary inspections

Add `EyeChainRotationGpuTest`Freezing with default delivered XR `GpuBlit` , check for RGBA not previously covered by GPU tests 8 sRGB（DXGI 29) Continuous conversion path. The test uses an independent production transformation object for each eye, three target resources for each eye, rotating in different order and changing the left-and-right eye source pixels by frame.

Runs on WARP and NVIDIA hardware adapter, respectively, covering typed 29 with typeles 27 Resources. Each combination is continuous 96 Frames, eyes. 257×129 Pixels, read-in-through-the-post comparisons, allowing maximum 1 Rounded error. All four combinations are passed and checked together 25,461,504 No missing text, left-and-right eye-string or old frame residue from target rotation was found in pixels.

Compile a new Ninja directory,`/W4 /WX`, saves actual MSVC showIncluds prefixes and certificates of dependency. Test does not create games, windows or OpenXR sessions. Small-scale offscreen tests do not cover the original stereo production, the real XR exchange chain, SteamVR synthesis or VD transmission; they cannot be used as actual flash fixes or head-over acceptances by results.

## First round of inspections and subsequent corrections

**The first round of flashing root causes were not identified and repaired, and the treatment was suspended at the user ' s request.** The first round of surveys only adds tests and records, without modifying the default binary or settings. The user then explicitly requests that all XR outputs be replaced with sRGB; this independent request is then integrated by default `build-srgb-output1`and confirmed by user retrospect that clarity has improved significantly, as detailed [Harmonize SRGB output records](xr-srgb-output-20261023.md). This clarity feedback does not constitute a flash white fix or full head viewing; the current default colour policy is based on a subsequent retreat from the header.

If the user resumes this work later, it is necessary to match the flash with the actual image of the left and right eye and the submission stage, then modify the production path based on evidence. Full manual session log, SteamVR log, two GPU test returns and first round default file has been saved in ignore directory `analysis/steamvr-left-eye-20261023/`♪ The Lord will answer ♪ `investigation.json`. The reply retains the description of the symptoms, and the subsequent correction is made `analysis/xr-srgb-output-20261023/symptom-correction.json`, this user confirms that the suspension is saved in the same directory `user-feedback.json`;no actual images captured at abnormal moments.

## 2026-10-23 - Recover simulator screens as required by the user

Use published 0.0.3 Default native `7571fee6…bab9` and XR `58ef6c39…539a`, complete three-wheeled formal game/ OpenXR emulator backstage testing. Each round uses a stand-alone configuration and current archived copy to test the behind-windows starter and simulator, read-output observation only, test silent sound, and close the physical handle background input; no changes were made to the global OpenXR runtime. 3072×1632♪ Every eye ♪ 1536×1632, is a semi-scale survey that does not cover full resolution and display links as indicated by the user.

- A wheel for automatic header, simulator preview detector maintained 17 Number of times `ASYMMETRIC_BRIGHT_BREAKTHROUGH` The alarm and a manual capture. Looking back at the continuous picture, the white dots on the upper left eye go into the image with multiple frames; this is only suspicious local brightness that has not yet been proven to be instantaneous as indicated by the user or normal scene light source. `ASYMMETRIC_EYE_FLASH` or `VISIBLE_TO_BLANK_FRAME` Call the police.
- B Round fixed view and sent a boundary sight/shooting input, keeping four game stereographs; no similar alarm was captured. The initial fixed value is taken from the head angle in state, which is actually zero, and cannot be considered as matching retrometry for the A wheel anomaly angle.
- C wheel is changed to calculate fixed direction from A Round CSV double-eye attitude, and saves three stereo source maps and a two-eye preview of the simulator. Last saved status over 4,520 Preview of sample,6,349 The following successful drawing failed to produce zero and automatic abnormalities zero. The source map and preview do not see the dot or instant white; however, the scene is not consistent with A-wheelshots, which does not preclude A-wheel phenomena.

An emulator restriction was found: automatic head turner updates eye attitude, while VIEW ' s head position and status bar remains original; the relationship changes when turning off and static fixes. The current scan cannot therefore be a rigorous match to the A/B test. The preview detector also has sampling and threshold limits, and single-spectrum anomalies in a physical submission cannot be ruled out without warning.

The tricycle sessions have been properly completed without structural failure. A wheel sample shows that the transition to a stereo level has not lost any more; this only indicates that layers are submitted continuously and cannot prove image pixels correct. 1.2 The second mouse request was rejected by the entered guard, which did not send it; the monitor ended when the stop was placed at a border and the fixed request after the stop was denied by the identity guard, which was not recorded as passing the test. 195 The number of irregularities in the sub-window audit was zero for both the testing window front desk and the existing window; the start time was not within that audit.

All three rounds review the user ' s original configuration/archive source file Hash maintained, test games, XR client and start workmaker quit. Default list and native/XR binary Hashi maintained; no default colour changed, no flash fixes submitted, no package reissued. All original anomalies, source charts, session logs and survey returns are stored in ignore directory `analysis/flicker-simulator-20261023/`♪ And I'm gonna write it down ♪ `investigation.json`. The screenshot does not correspond to the same source, which does not allow for a location of the period in which the anomaly occurs in the original rendering, reproduction, synthesis or real head appearance.

## 2026-10-23 - User confirms that only left eye flashes when static, manual session log reviews

Reconfirms that only the left eye flashes when the head is still moving. Read-only saves the finished `continuous-stereo-18524-20261023-200301-e2a97080` And was running. `continuous-stereo-10516-20261023-201851-4d4e206f` Manual session report/stdout, no extra games, send input, stop user games or change configurations started. Both sessions record the Stereo source format 28 XR selection 28、`gpu_conversion:false`, the original UNORM directly submits the path.

Ended session recording left and right.45,541 Sub-exchange chain waiting, timeout/ error zero; image ticket diagnostic records 72,779 The session is stopped after the game has quit normal and no structural failure; the outer report is marked because the game exits is observed `incomplete`, not rewritten as a whole. The above is only API/ID and submission records, without authentication of each eye pixel or user flashing accuracy time, which does not allow identification or exclusion of left eye rendering, copying, reuse of resources or actual delivery of anomalies. Read only evidence and user words to save the ignore directory `analysis/flicker-simulator-20261023/manual-feedback-2019/`. The root cause was not located and the repair was not delivered.

## 2026-10-23 - Clear/obscurant jump captured in original stereogram

Users add three other simulators of the same scene:1、3 There's a strong drag blur in the left eye, no.2 Open left eye and clear right eye. Users correct to "black to clear" while confirming that desktop games are flashing; update symptoms with this direct image evidence, no longer simply searching for white frames or bright mutations. History flash alert does not have a relationship with this blur.

Still running on user `continuous-stereo-10516-20261023-201851-4d4e206f` , after checking the full identity of the game, owner and XR, request four new complete GPS source maps using existing active snapshot events. No extra games are started, input sent, attitude/set/color changed or end user sessions.7200×3200♪ Every eye ♪3600×3200,Format 28 XR chooses 28, no format conversion.

| Quick-Impression | ticket | source serial | Left eye. | Right eye. |
| --- | --- | --- | --- | --- |
| 1 | 24700 | 20031 | Clear. | Clear. |
| 2 | 24760 | 20074 | Strong Blur | Clear. |
| 3 | 24821 | 20115 | Clear. | Clear. |
| 4 | 24883 | 20158 | Strong Blur | Clear. |

These four are samples taken at approximately one second intervals, not every frame submitted, and cannot be used to claim fixed frame-by-frame rotation or extrapolation of blinking frequency. Visual review is consistent with the RMS gradient: left eye 11.68→3.62→11.68→3.61 Right eye.15.26 Maintain; average values do not rise in white frame. The indicator describes only the changes in edge details of these same scene samples, and is not a generic quality or comfort check.

Freezing a stereo snapshot of XR source code directly from `receiver->texture` Read the full SBS source, confirm it with local GPU fence and keep its original size; not XR eye-link targets or simulator synthesis previews. This time has shown that clarity anomalies exist in the incoming home-grown sources, and the problem is not limited to subsequent XR conversion/synthesis. Specifically, the back-to-back processing of raw rendering, the time history status or the source release are still unpositioned; dynamic fuzzy/time-related reprocessing is only a test hypothesis and no single variable closed-discretion contrast has been made.

Check also the default freeze `DesktopMirror` pixelshader: by `uv.x * 0.5` Samples the left half of SBS, where desktop monoscopes are shown with the left eye by default. The desktop synced blur corresponds to this left eye map phenomenon, but mirror observations themselves can no longer break down the failure link.

3 original user maps, full Hashi/Preview of 5 source charts, 4 snapshotticket and source security identity and frozen source code identities are stored in ignore directory `analysis/flicker-simulator-20261023/blur-evidence/`I'm coming back. `investigation.json`. No default build, user drawing or distribution package, no claim of repair or physical header acceptance.

## 2026-10-23 — 0.0.4 Recover, restore and default integration

From `codex/0.0.3` It's... `9ffa710` Create `codex/0.0.4` After, use actual default `7571fee6…bab9` . The normal frame is still used for the original player status.

The C-tests are the same scene, static posture, original graphics, dynamic blur open, every eye.3600×3200 XR Source Format 28/ Choose 28/ No conversion. Baseline 7 Full source map available 5 The left eye is clearly blurry.88 A preview sample has 17 Blur; candidates for 0/7 and 0/88. Normal frame retreats still occur 1325 Minor/1463 The RMS margin of the baseline left eye is approximately 3.61–11.69 Candidates 11.69 Right eye.15.26 Stable. Source view matches user's cut-off. These are the sampled in a fixed scene, which does not constitute a flash frequency or header comfort check for the entire session.

Early low-level A/B replicating results are not used to repair validity determinations: additional UTF-to-test script added 8 BOM, the game then rewrites part of the painting; after retaining the original code in the private configuration, the original high quality is correctly preserved and the baseline is repeated. The user's original configuration and archive remain Hashi at all times. The dynamic blurry or colour changes are not closed.

Candidate D and E were both targeted/demunition response, moratoriums on recovery and site world 1→2 Rebuild. Finally, native `a637128c…0d758` Test at E.7 Source graphs and 220 The serious ambiguity was not found in the preview sample, raw UI/material failure zero, independent status original release/delegation completed, hook entry restored, XR exit normal, user 10 The original configuration/archives remain unchanged. The dynamic operation and preview window during the reloading period is different from the C static phase, without mixing the two with frequencies. Both tests use authorized backstage, window placement, tiny-output read-only and test silent policy, no physical handle background input enabled; all testing processes have been withdrawn.

Fix the consolidated default list and root directory `GearsVR Launcher.exe`（0.0.4 This is a very interesting example of the "XR" approach. `58ef6c39…539a`, the FPS slots and other original functional switches are retained.217 The starter for a block Python test, target protostate/construct/life cycle check, actual start self-measurement and GUI hides the self-check. Freezing the original from the actual default source plus minimum increment construction; current source code with other experiments only merges this increment and does not replace the default binary as a whole. Full evidence is saved in ignore directory `analysis/flicker-simulator-20261023/` and `analysis/runtime/left-history1-frozen.json`。

No physical appearance, complete campaign or FPS change has been verified in this round, nor has the historical scrutinizing dots been shown to be the same as this blurry; no release 0.0.4 Package/label.

---

## 中文

# 左眼清晰度跳变：0.0.4 已接入独立历史状态修复

**当前结论（2026-10-23）：** 已在购买版游戏／后台模拟器的7200×3200原始立体源中重现用户的左眼径向拖影。普通渲染帧与VR帧穿插时，旧版VR左眼复用原玩家历史状态，而右眼独立；为VR左眼建立独立原生状态后，同配置对照消除了已捕获的清晰／模糊跳变。默认native已切换为 `build-left-history1`，正常启动器标识为 `0.0.4 Dev`；颜色策略保持。物理头显效果仍待用户验收。详见[0.0.4开发记录](releases/0.0.4/PROGRESS.md)。

以下保留调查过程中的原始反馈和当时结论；“根因未知／尚未修复”等表述属于对应阶段的历史记录。

用户实际对比后确认，在其当前设备和设置下 SteamVR 看起来比 VDXR 更清晰。用户随后澄清异常仅出现在左眼边缘或局部，并进一步更正为“应该说是闪白”。以这份后续描述为准，先前“整帧闪黑／漏一帧”的调查假设不再作为症状结论。目前只在本 Mod 中观察到；其他应用未独立排查，清晰度差异也不是已证实的闪白原因。

此前反馈：用户复测确认“就是颜色的问题 srgb 清晰很多”，同时描述“左眼会闪烁，就好像有一帧是白色的”。以此补充闪白的瞬时观感，保留此前边缘／局部的观察，不将“像一帧白色”当作已捕获的整帧全白证据，也不据此推断 sRGB 是闪白的根因。

**2026-10-23 最新更正：用户反馈“好像都会闪，不是颜色的问题”。记录为原颜色／UNORM 与 sRGB 路径似乎都可能闪烁，颜色回退未消除该现象；撤回将颜色格式作为闪烁根因或将原颜色视为无闪路径的判断，根因仍未知。** 保留用户关于 sRGB 明显更清晰的观感反馈；清晰度与闪烁分别记录，不据此完全排除任何尚未验证的关联。

**最新图像更正：用户提供三张模拟器截图，澄清“不是闪白，而是模糊到清晰”，并确认桌面游戏画面也在闪。已在当前手动会话的完整立体源图中抓到左眼清晰／模糊交替、右眼保持清晰，见文末。** 前三轮未复现白帧／亮度异常的结果不能作为该清晰度跳变的排除证据。具体根因仍未定位、未修复。

用户随后要求“用模拟器排查下这个闪的问题”，已恢复调查；此前按“先不修，记录下来”暂停属于历史决定，已被恢复调查的请求取代。当前默认维持已恢复的源格式／UNORM 优先策略，保留 SteamVR 所需的 sRGB 后备路径；没有因本次调查切换颜色或交付实验修复。此前“原颜色没有闪烁”保留为历史反馈，已被后续新观察取代，不能作为无闪验收。

**后续用户再次确认：“确实是有闪白只有左眼才有”，并回答“头不动时也会出现”。** 当前症状记录为仅左眼、静止时也可能瞬时闪白，保留此前边缘／局部描述；转头不是用户观察中的必要触发条件。没有异常帧的精确时间，尚未证明与模拟器扫视中持续多帧的边缘圆斑属于同一现象。

## 已有实际会话证据

使用用户手动启动的正版会话 `continuous-stereo-37508-20261023-180608-a0caee0c`，没有为本调查额外启动游戏。

- 默认 native 为 `7571fee6…bab9`，XR 为 `5f8d6769…fe3f5`；已激活原生立体输出。
- 实际源图像为 6144×3264，两眼 XR 图像各 3072×3264。
- 左右眼各记录 11,316 次 `xrWaitSwapchainImage`，均无超时或错误；这是应用端等待结果，不能证明头显没有局部闪白。
- SteamVR 合成器在同一会话存在 `WaitForAcquire timed out`，但最早两条已出现在 18:05:56，而 Mod XR 客户端在 18:06:09 才连接。因此这些警告不是 Mod 左眼漏写的直接证据。
- 没有用户异常发生时刻与某一具体原生图像、XR 提交、合成器警告的对应关系，不能认定局部闪白发生在生成、转换、合成或 VD 送显中的哪一步。

## 本轮补充检查

新增 `EyeChainRotationGpuTest`，使用默认已交付 XR 的冻结 `GpuBlit` 及其依赖，检查此前 GPU 测试未覆盖的 RGBA8 sRGB（DXGI 29）连续转换路径。测试使用每眼独立的生产转换对象，每眼三个目标资源，以不同次序轮换，逐帧改变左右眼源像素。

分别在 WARP 和本机 NVIDIA 硬件适配器上运行，均覆盖 typed 29 与 typeless 27 资源。每个组合连续 96 帧、每眼 257×129 像素，逐像素四通道读回比较，容许最大 1 级舍入误差。四个组合均通过，共检查 25,461,504 个像素，没有发现目标轮换导致的漏写、左右眼串图或旧帧残留。

编译使用全新 Ninja 目录、`/W4 /WX`，保存实际 MSVC showIncludes 前缀及依赖证明。测试不创建游戏、窗口或 OpenXR 会话。小尺寸离屏测试没有覆盖原生立体生产、实际 XR 交换链、SteamVR 合成或 VD 传输；通过结果不能用作实际闪黑修复或头显验收。

## 首轮检查与后续更正

**首轮闪白根因未确定、未修复，当时按用户要求暂缓处理。** 首轮调查只增加测试与记录，没有修改默认二进制或设置。用户之后明确要求将所有 XR 输出改为 sRGB；这一独立请求当时集成默认 `build-srgb-output1`，且已由用户复测确认清晰度明显改善，详见 [统一 sRGB 输出记录](xr-srgb-output-20261023.md)。这项清晰度反馈不构成闪白修复或完整头显验收；当前默认颜色策略以文首的后续回退为准。

若用户之后恢复此项工作，需要将闪白瞬间与左右眼实际图像及提交阶段对应，再依据证据修改生产路径。完整手动会话日志、SteamVR 日志、两份 GPU 测试回执和首轮默认文件哈希已保存在忽略目录 `analysis/steamvr-left-eye-20261023/`，主回执为 `investigation.json`。原回执保留先前症状描述，后续更正在 `analysis/xr-srgb-output-20261023/symptom-correction.json`，本次用户确认与暂缓决定另存同目录 `user-feedback.json`；没有异常瞬间的实际图像捕获。

## 2026-10-23 — 按用户要求恢复模拟器排查

使用已发布 0.0.3 默认 native `7571fee6…bab9` 和 XR `58ef6c39…539a`，完成三轮有界正版游戏／OpenXR 模拟器后台测试。每轮使用独立配置和当前存档副本，测试专用 behind-windows 启动和模拟器、只读 tiny-output 观察、测试静音，关闭物理手柄后台输入；未修改全局 OpenXR runtime。源立体图为 3072×1632、每眼 1536×1632，属于半比例调查，未覆盖用户头显的完整分辨率和送显链路。

- A 轮进行自动转头，模拟器预览检测器保存 17 次 `ASYMMETRIC_BRIGHT_BREAKTHROUGH` 报警及一次手动捕获。回看连续图片可见左眼左上边缘的白色圆斑随视角进入画面，持续多帧；这只是可疑局部高亮，尚未证明是用户所述瞬间闪白，也未证实是正常场景光源。没有保存到 `ASYMMETRIC_EYE_FLASH` 或 `VISIBLE_TO_BLANK_FRAME` 报警。
- B 轮固定视角并发送有界瞄准／射击输入，保存四张游戏立体源图；未捕到同样报警。最初固定值取自状态中的头部角度，实际为零，不能当作 A 轮异常角度的匹配复测。
- C 轮改为从 A 轮 CSV 双眼姿态计算固定方向，保存三张立体源图和模拟器双眼预览。最后保存状态覆盖 4,520 个预览样本、6,349 次成功绘制，失败绘制为零、自动异常为零。源图和预览没有看到该圆斑或瞬间闪白；但场景朝向与 A 轮截图不匹配，不能据此排除 A 轮现象。

发现一项模拟器限制：自动转头会更新眼睛姿态，而 VIEW 空间头部姿态和状态栏仍为原值；关闭转头再静态固定时，两者关系改变。因此本次扫视与固定视角不能作为严格匹配的 A/B 试验。预览检测器还存在采样和阈值限制，未报警不能排除某一实际提交中的单帧局部异常。

三轮原生会话均正常结束、无结构性失败。A 轮采样显示启动阶段切换到立体层后没有再失去 projection layer；这只能说明层提交连续，不能证明图像像素正确。A 轮 1.2 秒鼠标请求被输入守卫拒绝，未发送；监视器到有界关停时结束、关停后固定请求被身份守卫拒绝，这些不记为测试通过。A 轮启动之后的 195 次窗口审计中，测试窗口抢前台和遮挡既有窗口的违规数均为零；启动瞬间不在该审计范围内。

三轮均复核用户原始配置／存档源文件哈希保持，测试游戏、XR 客户端及启动 worker 已退出。默认清单及 native／XR 二进制哈希保持；未改默认颜色、未提交闪烁修复、未改发布包。全部原始异常图、源图、会话日志和调查回执保存在忽略目录 `analysis/flicker-simulator-20261023/`，总回执为 `investigation.json`。截图未对应同一源 ticket，不能据此定位异常发生在原生渲染、复制、合成或真实头显送显中的哪一阶段。

## 2026-10-23 — 用户确认静止时仅左眼闪白，手动会话日志复核

用户自行复现后再次确认仅左眼闪白，头不动时也会出现。只读保存已结束的 `continuous-stereo-18524-20261023-200301-e2a97080` 和当时运行中的 `continuous-stereo-10516-20261023-201851-4d4e206f` 手动会话报告／stdout，未启动额外游戏、发送输入、停止用户游戏或更改配置。两个会话均记录立体源格式28、XR选择28、`gpu_conversion:false`，即原UNORM直接提交路径。

已结束会话记录左右眼各45,541次交换链等待，超时／错误均为零；图像ticket诊断记录72,779次提交、无无效ticket提交行。会话随游戏退出正常关停，无结构性失败；外层报告因观察到游戏退出而标为 `incomplete`，不改写为整体验收通过。以上仅是API／身份和提交记录，不验证每眼像素，也没有用户闪白精确时刻，不能据此确认或排除左眼原生渲染、复制、资源复用或实际送显异常。只读证据与用户原话另存忽略目录 `analysis/flicker-simulator-20261023/manual-feedback-2019/`。根因仍未定位，未交付修复。

## 2026-10-23 — 清晰／模糊跳变已在原生立体源图捕获

用户补充三张同场景模拟器截图：第1、3张左眼出现强烈拖影式模糊，第2张左眼清晰，三张右眼均清晰。用户更正为“模糊到清晰”，同时确认桌面游戏画面也闪；以本次直接图像证据更新症状，不再只按白帧或亮度突变检索。历史亮斑报警与这次模糊的关系没有建立。

在用户仍运行的 `continuous-stereo-10516-20261023-201851-4d4e206f` 中，核验游戏、owner及XR完整身份后，使用现有有界快照事件请求四张新的完整GPU源图。没有启动额外游戏、发送输入、改变姿态／设置／颜色或结束用户会话。实际源为7200×3200，每眼3600×3200，格式28，XR选择28、无格式转换。

| 快照索引 | ticket | source serial | 左眼 | 右眼 |
| --- | --- | --- | --- | --- |
| 1 | 24700 | 20031 | 清晰 | 清晰 |
| 2 | 24760 | 20074 | 强烈模糊 | 清晰 |
| 3 | 24821 | 20115 | 清晰 | 清晰 |
| 4 | 24883 | 20158 | 强烈模糊 | 清晰 |

这四张是约一秒间隔的采样，不是每一提交帧，不能据此声称固定逐帧交替或推断闪烁频率。目视复核与同尺寸梯度RMS一致：左眼约11.68→3.62→11.68→3.61，右眼约15.26保持；均值没有白帧式跃升。该指标仅描述这些同场景样本的边缘细节变化，不是通用画质或舒适度验收。

冻结XR源码的立体快照直接从 `receiver->texture` 读取完整SBS源，使用本地GPU fence确认完成，保留原始尺寸；读到的不是XR眼链目标或模拟器合成预览。因此本次已经证明清晰度异常存在于收到的原生立体源中，问题不限于后续XR转换／合成。具体是原生渲染后处理、时间历史状态还是源发布等环节，仍未定位；动态模糊／时间相关后处理仅为待验证假设，尚未做单变量关闭动态模糊的对照。

另核对默认冻结 `DesktopMirror` 的像素shader：以 `uv.x * 0.5` 采样SBS左半图，即桌面单眼镜像默认显示左眼。因此桌面同步模糊符合这次左眼源图现象，但镜像观察本身不能再细分故障环节。

三张用户原图、五张源图的完整哈希／预览、四次快照ticket及source serial与冻结源码身份均保存于忽略目录 `analysis/flicker-simulator-20261023/blur-evidence/`，总回执 `investigation.json`。没有改默认构建、用户画质或发布包，未宣称修复或通过物理头显验收。

## 2026-10-23 — 0.0.4配对复现、修复及默认集成

从 `codex/0.0.3` 的 `9ffa710` 创建 `codex/0.0.4` 后，使用实际默认native `7571fee6…bab9` 的冻结源，核对左眼继承原玩家关联状态、右眼使用工厂状态的非对称路径。现为正常连续VR会话增加独立左眼状态，沿已验证的原生工厂／release／延迟删除路径管理；换关与清理等待其删除完成，不复制引擎对象或直接释放其资源。普通帧仍使用原玩家状态。

配对C测试保持同一场景、静止姿态、原始画质、动态模糊开启、每眼3600×3200、XR源格式28／选择28／无转换。基线7张完整源图有5张左眼明显模糊，88张预览采样有17张模糊；候选分别为0/7与0/88。普通帧回退仍发生1325次／1463次，未通过取消回退绕开条件。基线左眼梯度RMS约3.61–11.69、候选约11.69，右眼约15.26稳定。源图目视吻合用户截图。这些是固定场景的有界采样，不构成整个会话的闪烁频率或头显舒适度验收。

早期A/B的低画质无复现结果不用于修复有效性判断：测试脚本额外添加了UTF-8 BOM，游戏随后重写部分画质；在私有配置中保留原编码后，原高画质被正确保留，基线重现。用户原始配置与存档始终保持哈希。未关闭动态模糊或修改颜色。

候选D和最终构建E均通过瞄准／弹药响应、暂停恢复和检查点世界1→2代重建。最终native `a637128c…0d758` 在E测试7张源图和220张预览采样中未见该严重模糊，原生UI／材质故障为零，独立状态原生释放／删除完成、钩子入口还原、XR正常退出通过，用户10份配置／存档原件不变。动态操作及重载期间的预览窗口与C静止阶段不同，不将两者混算频率。测试均使用已授权的后台、窗口置后、tiny-output只读和测试静音策略，未启用物理手柄后台输入；全部测试进程已退出。

修复已集成默认清单和根目录 `GearsVR Launcher.exe`（0.0.4 Dev）。原XR `58ef6c39…539a`、FPS两槽及其他原有功能开关保留。217项启动器Python测试、针对性原生状态／构造／生命周期检查、实际启动自测和GUI隐藏自检通过。冻结原生从实际默认源加最小增量构建；当前含其他实验的源码只合并该增量，未整体替换默认二进制。完整证据保存在忽略目录 `analysis/flicker-simulator-20261023/` 和 `analysis/runtime/left-history1-frozen.json`。

本轮没有验证物理头显、完整战役或FPS变化，也没有证明历史扫视圆斑与本次模糊同因；没有发布0.0.4包／标签。
