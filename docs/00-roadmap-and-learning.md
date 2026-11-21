# GearsVR: Development of processes and learning along projects

2026-10-19 Resolution route update:**0.0.1 The VR render driven by the resolution must be replaced with an OpenXR recommended per-eye resolution x starter multiplier, which is above DLS.** 1.0 Use the recommended size for running, and a player can reduce the multiplier; the game window resolution does not determine VR per eye dimension. This is not an optional candidate or 0.0.2 function. Current high-multiplier actual images are still customised and must be repaired and validated before release; configuration is defaulting does not mean that the image has passed. See latest evidence [Resolution Achievement and Validation](runtime-render-size-candidate.md). The old date status below is kept as a historical record.

2026-10-18 Please see the current status.[Main line progress and V1 acceptance](39-mainline-progress-and-acceptance.md). Default Native 70/Borderless 1 And Feedback 3 Development package not upgraded; third person referred to as Consembling Biscades and 6 The foundation of the DPF and previous real feedback has been drawn down. Low/High is withdrawn.83 The actual eye output has been restored, but the switch is still marked with a lack of compatibility. The black screens of machine guns, the outer circle of pistols, the UI fork and long/real-time stability remain open and cannot be used as an old percentage.[Document 154](154-dlss-sr-startup-and-output-route.md)、[Document 155](155-path-choice-position-and-docking.md)。

The following is a record of the previous stages of study and experiments, in which “current” “latest” and percentages are on corresponding historical dates and do not represent the status quo.

Update:2026-10-15. For developers with a little front end and programming base who do not know about UE.

Current Player Test Orientation 2026-10-15 Adjustment: Remove the Low/High fixed resolution as required by the user, return to the frame SBS driven by the output size of the game and deliver the Chinese text starter.1920×1080 Game output for eyes 960×1080. Default list toggle to GameResolution 1 native＋MenuRecovery 1 XR, save the start-up menu, DLSS/DLAA protection and original HUD/astrometric corrections; no automatic navigation or test countdown. Build and offline check pass, this new combination has not yet been activated until the user has made a self-measurement.[Current Build & Starter](62-game-resolution-launcher.md)。

Low/High Every Eye 2K/4 K has withdrawn from the current delivery route and does not continue to include High as a first-end threshold.1 Still to be addressed or validated for flashing/eye continuous updates, menus and UI, cross-gate, cleanup, real headline 6 The DDF/FOV experience and the release of a cover.65%It is only the old scope that estimates that this round does not reduce the extent of the gap in scope.

The latest completed actual operation remains the previous fixed target Mirror 2：1920×1080 Desktop displays left glasses, downed output for each eye 2048²，111 Run complete in seconds 10,106 Sub-image,5,052 Copy that.63 The source map shows a clear dimness of the right eye and little subsequent change, which has not yet proved causal link to the user's left eye blinking. This experiment or feedback from the old header can not be used as a substitute for GameResoltion 1 Receiving and Inspection. Independent Canvas/Material and Subtitle pixel evidence of the old route is kept in place[Show Record](57-ui-position-and-subtitle-route.md)。

Below 1、Menu 1、Reload 1 The paragraph and its subsequent paragraphs are kept as historical experiments, and cannot be considered “the next step” as the current state.1(Submitted)`0a2c464`See more on this.[Independent Double-Eye Record](61-owned-render-latency.md): Independent Low Every Eyes 2048×2048 Cancel 120 Frame test limit, actual game+ emulator continuous 90 Second complete.3106 A downed ticket,3105 Sub-issue,3101 Sub-XR copy with 8084 Sub-proceed level. First frame and 1800 The frame GPU images are presented in full eye; the detailed diagnostic history is maintained 120 Article.49 The return of the item,7 compatibility check,59 The resource sizes are being rebuilt, the right eye mask is being rejected, and there are no changes or observations on how Trial and GPU follow-up has been made.100 The limitation period for ms has not been relaxed.

Menu 1 / PID 39376 Re-enacting the same freeze Sustain 1 Build, actual game+simulator running 240.91 Seconds, finished.8309 A downed ticket,8307 Sub-issue,8304 Sub-XR copying and 21669 Sub-projection level. Finish a pause game by key to read-only menu status and orientation window; pause scene`DeltaPause-Campaign-Host`and`LBtnResumeSingle`The focus was on actual confirmation that three of the recovered samples showed a pause in the scene and continued to grow in the number of releases.121 Sub-run state sampled down_error 0；59 This time, no paused menu GPU pixels was read and no level reloading or real header tests were performed. The receipt and inspection cannot be shown as a menu.`analysis/ui-position-runtime/owned-menu1-39376.audit.json`。

Reload 1 / PID 34448 A physical check point reload was completed: the Pawn address and the registry index replacement were maintained with the class/outer, Controller/Camera/Level; original scene/family/renderer accepted reconnection and continued to be published on both sides.299.99 within seconds 9896 A downed ticket,9894 Sub-issue,9891 Sub-XR copy,26745 Sub-proceed layer submission, maximum submission interval 2133.89 ms, not the whole trip without interruption.59 Access restored and normal exit passed. Full eye map after replacement or reloading across the Lever is not covered; a simulator preview factor size 1734×14 Not for visual authentication. Evidence.`analysis/ui-position-runtime/owned-reload1-34448.audit.json`。

The above-mentioned early experiments cover only continuous output, one pause/restoration and Pawn replacement within the same level. Subsequently, access to urban Canadians/Material and independent recording periods has been secured, with current status at the beginning of this paper and document 57. The old headline version of the "good effect" cannot be used directly for new paths.

The following is a historical record of the XR-ZO findings from the OwnedXr/OnedCost phase, which are only described as being constructed at that time and have been updated by the above-mentioned follow-up evidence.

OwnedXr 1/2: Add the default closed`GEARS_NATIVE_OWNED_XR_PROBE`, top 120 Independent scene picket, which prohibits the seizure of a public power. Independent CPU store keeps complete freezes at the actual consumption, and must have consistent eyes; consumption is based on certified rectangle, allocation generation and state call`capture_resource`, copy with DIRECT and restore the source state, clearly aligning the busy to retirement CPU picket. Owner still stays until process exits, and Canvas is not reoriented.

Actual PID 46980 and 42588 All completed 120 A stand-alone ticket,119 Sub-shared texture release (no consumer skips the first frame);**Both XR submissions 0 New Independent XR display not passed**。Xr 1 Read back ticket on the receiving side 50/serial 24 It's...4096×2048 Two eyes, paper sequences consistent with the issuance of receipts, not black for each eye.4194304 pixels;only completed 1 An XR image is copied and rejected before submission because of the expiry of time.2 Copy As 0. The receiving end continues to use 100 This results in the sharing and receiving of resources, which does not prove to be available.

Xr 2 It's...120 Grouping same monotonic clock measurements: pose received mid-point of the selected target 6.29 ms;select to start with consumer 77.66 ms (including native build and queue, not yet separately detected Queue presence); consumer starts to left eyefinal draw 62.09 ms;eye-final 0.125 ms;right-eye drew back to real eyes Execute 0.074 ms;Execute returns to a replicable border 0.0002 ms;sharing copy calls 0.036 ms. post to medium distribution 147.10ms/p 95 155.33 ms, has exceeded the threshold at the producer end. Consumer returns/destroys a little earlier than it is submitted and awaiting destruction; the CPU time mentioned above does not equal the execution time of the GPU. Next step is to split the native build and queue, measure scene recording costs, do not blindly replicate the interface or expand the inverse translation range.

Both rounds passed.39 Experiment returns; new check covers real WARP shared pixels/freeze ticket, eyes sample conflict, state mismatches and an urban mode rejecting legacy binding. Another closed down experiment is built through 6(c) The compatibility check.58 Access is restored and normal shut-down passes; the starter remains incomplete because Owner retains it.`analysis/ui-position-runtime/owned-xr1-46980.audit.json`、`analysis/ui-position-runtime/owned-xr2-42588.audit.json`and`owned-xr2-42588-latency.json`;freezing list`tools/vr-launcher/manifest-owned-xr1.json`、`manifest-owned-xr2.json`. The default player path is not switched to this experiment and the first-end display/performance acceptance is incomplete.

OwnedSequence 3 / PID 45772: Independence Low Targets First Success 16 scene ticket()2、4 to 32) Same as 4096×2048 Resources, first and second GPU readings; every eye 2048×2048、4194304 A non-Black Pixels, full end of the line.8090295 A pixel change. Manual reading of GPU images can be seen by people with two different eyes, columns, ground and remote entrances without HUD.14 Frame is only submitted/state evidence and cannot be considered frame-by-frame pixels or continuously processed. Target is RT (%)4) does not cover the side state conversion.

This round of amendments clears all list history, maintains real Reset and submitted tags and observes realCreateCommandList/CreateCommandList 1(SDK slot)12/51 and 88 The life cycle of a creation object. Even if the combination of unknown/duplicite batches fails, the true submission of the known member is recorded after the original Execute returns; overcapacity/non-readable members permanently stop the experiment and cannot be restored through Present. No DXGI white list added, no fake Reset.32 The SDK claims,17420 FrameLink,179 Trial and 34 The combination is amended and successive experiments are passed, but not attributable to all historical black screens.58 The entrance is independently restored and the exit is normal; Owner and the creation object refers to the process being intentionally retained for exit, while the starter remains inline.

Evidence:`analysis/ui-position-runtime/owned-sequence3-45772.audit.json`;freezing list:`tools/vr-launcher/manifest-owned-sequence3.json`Next step is to access the visible resource state, XR issued with the same ticket attitude and ownership constraints, and then deal with Canvas/UI, High, continuous play and normal retirement. Canvas/XR still uses old targets and cannot claim that players have gained independence.2K/4 K, overall progress estimate for the first edition is not adjusted.

OwnedSequence 1/2: Accessed to default shutdown,16(a) Continuous scene experiment with a ticket ceiling, using the same Owner, perticket receipt and first reading back;179 Trial and 32 A regression order is passed. Both rounds of actual games complete only the first owned ticket (first frame PPM saved) and do not produce a continuous independent output.2 / PID 53952 The first time I lost my license.`dxgi.dll+0x20da2`The DIRECT list submitted within the original Present: Currently, Tracker does not have valid list generational status, device has the same goal as owned, Reset achieves the same goal as it already has on a HOOK, and thus stops releasing the next frame. This evidence does not mean that the list actually visited the town texture or can not be attributed to history black screen; it is further checked for its real Execute return/Reset and holding period before resource creation, without the Caleb White List or forging of Reset.1 It's...`state_certified`Used to show only initial caches 2 Changed to current authentication status as 0. Both rounds of independent verification 56 The entrance is restored, the game is properly closed; Owner keeps it to exit and the starter remains inline. `analysis/ui-position-runtime/owned-sequence2-53952.audit.json`List `tools/vr-launcher/manifest-owned-sequence2.json`The current budget is still in the past, and the first edition of the report is estimated to be on track.

OwnedPixels 5 / PID 40520: First read full from the actual game independent Low Target 4096×2048 GPU image, two.2048×2048 The visions can see people and scenes, with different perspectives; each eye 4194304 A non-Black Pixels,4186986 Different matching pixels. Actual 1 Creates the status of RENDER_TARGET (%1)4) , individual ticket 2 Two final draws submitted by the same DIRECT queue, renderer returns and destroys, read back to fence complete, return status to 4, no unknown operation or failure of the daapter. This is evidence of pixels from a single scene ticket that is not continuous independent resolution, complete middle scene-color/depth size validation or real head count acceptance. Canvas remains undirected and XR still follows old Backbuffer path; this old wheel path has 23 Secondary file ticket, maximum submission interval 267.3858 ms, not to call stability pass. Owner intentionally keeps the process out and the starter returns incomplete as it should;56 The entrance is independently restored and the game is closed. `analysis/ui-position-runtime/owned-pixels5-40520.audit.json`;by default closing the experiment, list `tools/vr-launcher/manifest-owned-pixels5.json`。

Important correction: Early down-output monitor received Vtable slots from Research Barrier 24(actually OMSetStencilref) correct slot 26。OwnedPixels 4 Will 9 This error was detected after the next prebird unknown operation classification; these records cannot be used as evidence of a game barrier error.1–4"and the following history."13 The GPU status test is "not able to prove a real Rource Barrier conversion, the relevant state conclusion has been replaced by this paragraph and original records are kept. Production portals and two group integration checks have been amended; add 30 The SDK compilation slot claims as a DLL build-up dependency, modified version 13 Downed-output and 8 The scene-GPU mode and WARP readback. Debug layer is not available, the assertion that it does not support the WRapper test with direct call or independently prove post-installed intercept coverage.5 Targets do not have game sidetamping, state proof that they use observed initial RT status and no contamination submission, and cannot claim that it has run the validation sidedover branch.

Next step: Promote read-back single-time scenario output into a continuous, independent target output of the Tichet, maintaining real Reset and cross-frame GPU; re-entering Canvas/UI, XR, desktop mirroring and complete retirement. Prioritize existing back-compilation databases with only targeted analysis of specific gaps.

Continuous follow-up preparation: new`consume_owned_output`Borders, consumption is submitted with a genuine list of names and data, the non-submission of records of the resource is rejected and no re-application of creation status is permitted after the false identification.17351 FrameLink Checking Entries 100 WARP mode rejects frame continuity and duplicate submission/consumption, future ticket, pollution and incomplete records.13 Check all RGB pixels by actually two frames clear/barrier/readback; eye associated metadata are synthesized and cannot replace the game scene.14 Six COMs were held to be retained for process exit when the original submission failed earlier.32 All returns are approved, and the results are in `analysis/ui-position-runtime/owned-continuation1-tests.json`. The border has not yet been connected to the frame-by-framerender lease and release of the schedule, and only one owned ticket is still exported from the existing experiment.

OwnedScene 1 / PID 37624: For the first time, a physically managed-family target was changed to permanent independence and used to fix the Low border.2 Two. 2048×2048 View construction, consumption and destruction of renderer are all passed;4096×2048 Target received two verified finals of the draw, linked to a real Execute return in the same DIRECT queue, unknown drew 0,adapter failed 0. This only shows the original path and D 3D 12 Command submitted, not yet read texture pixels or authenticate GPU completed. Canvas/ subtitles are not reoriented and XR continues the backbuffer route; current two-eye running on the same wheel 51.46 seconds,4625 Sub-proceeding,3802 New chart, period 9 Second string ticket, maximum submission interval 111.481 ms, not to be called a continuous zero break. Experiment intentionally saves native Owner to exit and the original starter returns incomplete; independent verification 40 All entrances are restored and the game is closed.102 Experimental status check,299 Owner,16201 FrameLink and 13 The GPU status mode is passed. `analysis/ui-position-runtime/owned-scene1-37624.audit.json`, the default build closes the experiment. The list of experiments is: `tools/vr-launcher/manifest-owned-scene1.json`。

Independent resolution next: authenticate the creation/actual submission status of the resource and read back the eye map, then access Canvas, XR releases and mirrors, complete retirement. This frame cannot replace every eye.2K/4 K-rays or release acceptances.

OwnedMetadata 1 / PID 49524: Independent target adapter has been removed from the long-term possession of the original viewer. After re-entrying the database and checking the compilation, complete validation of the eight runtime codes and thread query import for metadata; original routines read current settings via the adapter gamma method. Real game RHI returns with Low/ High targets having all created, wide/survey queries,13 Bytes show parameters against and releases, without reserved resources.299 CPU inspection,13 The line GPU status connection test is passed. Existing binary sets run simultaneously 24.57 seconds,2212 Sub-projection,1825 New map. Start the plane to switch.40 The entrance is restored, the game is out. The scene/canvas has not been used as an adapter, or the game is back to normal.`target_redirected=0`、`gpu_target_rendered=0`;it's not everything. 2K/4 K shows acceptance. Evidence:`analysis/ui-position-runtime/owned-metadata1-49524.audit.json`;list of experiments `tools/vr-launcher/manifest-owned-metadata1.json`。

The same wheel joins the default closed owned-output legacy resource barier tracking and Execute preparation/relay connection, WARP actual submission and sync pass; only installation, bypass and restoration in real games is verified, not independently exported. Next step is targeted-family/Canvas, complete GPS status and final output border, retirement of queue holders. The air retirement conditions for synchronized metada probes cannot be used directly in scene/ UI queue.

GraphicsMenu 1 / PID 51304 has stopped the DLSS cache in real AA list selection; index for other controls 3 Still, it's a normal choice. It's a different kind of filter.8x/16 x-application and reopening both pass, eventually recover 4 x. The same process connects the two eyes from the start-up menu to the level 29.10 seconds,2620 Sub-proceeding,2134 A new map. Stop normal.39 The game is closed. Preset batch values are not covered and cannot be equated with the normal path to release locks. The state navigation continues through a new reader when it encounters changes in object lists; the controller supplements the orientation restoration of PID/Creation time check.57 The field offline menu test is passed and the autorecover patch has not been validated.[Set the call chain](61-graphics-runtime-lock.md)。

GraphicsStartup 2 / PID 52996 Completed start-up menu protection for double-eye interface:2813 Enter the propjection with ticket after starting the plane.167.36 seconds,15063 Sub-proceeding,12438 New map, stop and resume normal.38 Enter. Previously on Startup 1 / PID 50260 The main menu actually asks for DLSQuality, which is a very important tool for the community.3 Startup 2 This round does not re-select DLSS; return of menu cache tags and other settings after lock is on is still to be completed. Posture has returned by self-measurement before startup 33, the protection expired when running after the fresh samples were released to Hook installation; old failures do not have an age log and cannot be declared as having been proven.[Set the call chain](61-graphics-runtime-lock.md)。

Auto-opt the game state: The existing menu reflector takes scene, Focus, input block and confirmation box text, the navigator checks the status before and after input, and the level is confirmed by Pawn/Controller/Camera objects. Unknown states are checked first for knowledge base and available evidence, only to complete the necessary directional reverses. The screenshot is reserved for visual validation such as blackscreen, UI position and subtitle actual pixels; menu status, original settings take effect, GPU images and real header experience are recorded separately, avoiding "normal control display" being considered "VR image correct".

GraphicsLock 1 / PID 53840 Runtime lock for the active DLS/DLAA phase of consultation: actual menu selection NVIDIA DLSS 3.5 The government has also been able to provide information on the situation in the country.3 Submode 3 Request blocked, normal/alternate maintained 0，6685 The second colloctor/committee does not have a tactical malfunction.95.20 seconds,8566 Sub-proceeding,6320 A new map. Normal stop and...38 The portal is back through. Reopen the menu to display cache DLSS, menu tag correction and level protection are still incomplete; no release locks can be declared complete.[Set the call chain](61-graphics-runtime-lock.md)。

Subtitles have been given with non-empty geometry: PID 53760/46520 Separately 559/65 Subtitle text is called, but the original screenshot is not available in white and can not be declared as showing pass.1 Subtitles to maintain actual indexing status of source/root and GPU ticket, list establishment and command completion; CPU authentication pass, PID 53840 This round is a sample of the unhit subtitles GPU. The map for the given subtitleticket remains to be accessed, and the normal snapshot of vent only requests the next one, which cannot be used as proof of concoction.

Recent UiReticle 1 / PID 47092: The center-to-light stars are projected in each eye in the same direction as the ticket game.458 Subfixes are successful. Original GPU pixels confirm that the two-eye light stars move from image center to computational position, and white graphic boundaries remain open 60×104 Pixels;10775 Sub-proceeding,8012 A new map. Normal stop and...35 Access is restored. Use the experimental list. `tools/vr-launcher/manifest-ui-reticle1.json`, the default build closes this option. The current solution is asymmetric FOV directional deviation; near-real hit point, subtitle/more UI and real header experience are still not passed. This replaces the current status of "Aquar yet modified" in the next history record.

UiPixels 3 / PID 424: Support has been provided for a needs-based resolution GPS snapshot, and the actual sizet/serial of both screenshots with the cut-off and retention time check. The gun-exchange icon/number and ammunition HUD are visible, the two eye-level shifts are consistent with the HUD plane projections; the quasi-star is still at the centre of the various eyes, and asymmetric FOV deviation has not yet been corrected.5667 Sub-proceeding,4137 A new map. Normal stop and...35 Access is restored. The Material monitor is not hit by the current round and cannot be declared accepted; subtitles are not covered.[UI positioning progress](57-ui-position-and-subtitle-route.md). The independent target holds/adapter and Tracker birth contract has been added to the CPU test, but is not tied to the game scene; each eye 2K/4 K is still to be achieved.

NativeSurface 1 / PID 34052: First actual call to the game's own color target distributor, created sequentially in verified RHI thread/TLS 4096×2048 and 8192×4096 Target. Both sets of real GetDesc are RBBA.8/DXGI 28, mip/slice/slample, allOW_RENDER_TARGET and evac by reference to native; first ticket is successful, unreserved or undefined. Independence target is not tied up, so this is not a game original 2K/4 K shows tests. The original binary runs simultaneously.32.268 seconds,2907 Sub-proceeding,2416 Sub-copy, maximum submission interval 29.5934ms，34 Access is restored; simulators and games are logged out.54d 54b 6 evidence `analysis/ui-position-runtime/native-surface-live-34052.json`Self-measured 164 Item, macro-closure is ineffective and continuous self-measured.

c 5f 7262 The visible Low/High boundary has been added to the eye source structure, and the original source/family is maintained and returned through CPU.e This parameter has not yet been selected for operation.80aad 0 The next step is to have an independent native target, access the COPE-family/Canvas goal and publish it in actual GPU status and queue; new Tracker must configure the receiving list before emptying old Tracker's count for initial activation. The original release must still be on a legitimate RHI line and wait for the related renderers/UI/GPU to retire.

The first edition has been re-matched: third person is a two-eyed, reliable person.6 DIF, Start/menu, UI package, every eye 2K/4 K Independent Rendering, DLSS/DLAA lock-in, stability and player deployment, project estimate 45–50%, for specific weights/deficits[Target for deployment](16-deployment-target.md). This is not the rate of acceptance and approvals issued. The UI script will be exported in bulk 2709 The blog is a good one.1093 The only declaration is a native; it is also confirmed that the Material UI exclusive consumption route and the World Markers cannot be directly applied to the normal HUD plane.[UI Cover Audit](57-ui-position-and-subtitle-route.md). This round does not start the game or add a new visual acceptance.

TargetSize 2 / PID 29184: new diagnostic editions are launched,220.366582 The second double-eye and normal stop pass,19830 Sub-proceeding,15033 Sub-image copy, maximum submission interval 91.1762ms，34 One entrance is back. First time.4 The same actual target is captured by a valid family call.40 Method prefixes 4 A Vtable bytes is all consistent with historical running codes; it is not a guess from an expired stack address. The round started the guard successfully at one time, and cannot prove PID based on this.39208 It's a cross-frame competition.3d 88101 Clear failure classification for start-up has been added, with limited readback of time stamp changes only for the remaining identity/reading checks (maximum)5 Sub-paragraph 200(ms) Rendering is still a single check.`analysis/ui-position-runtime/native-target-size2-binding.json`;The game is normally closed.

Specific interfaces for stand-alone targets are located: actual target vtable RVA 1706880，+8 returns the name of the reference, +10/+18 Read width height,+20/+28 To display metadata. The original target is 0x 510 Bytesview object+8 Sub-interface, the original configuration releases the entire view and cannot be copied to a small adapter. World general final output and postproces will use a family-based target; native FCanvas' target in +8，48 bytesqueuedcommand's target in +20, the two are not the same layout. The active history reference and the COM texture extraction path have been identified, but the complete interface boundary, creation parameters and final actual GPU status of the output resource still need to be checked; no changes in primary resolution of the game are currently being made.

Latest (independent resolution preparation):37d 9047 Changed to left-and-right eye independence XR swapchain and check runtime ceiling. Game PID 46572 In the original eye 1920×2160 Run Down 99.279 Seconds,8938 Number of submissions, maximum interval 20.622ms，34 Access is restored and exited. Independent test diagram 4096×2048 Two.2048×2048 It's been connected through the actual emulator.29.955 Seconds, maximum interval 15.2904ms；High 8192×4096 Two.4096×4096 It's passed.29.976 Seconds, maximum interval 12.2045 ms, production end and XR exit normal. GPU pixel testing covers Low/High, copy/BGRA/sRGB 300 Samples are not different. The game's stand-alone scene is still not achieved and the test chart cannot be called a game 2K/4 K. Target definition is in the deployment document 16。

67 Ifdba adds a fixed texture GPS release interface, the source must not be desktopbackbuffer. Independent GPU tests cover COMMON/RENDER_TARGET/PIXEL_RESOURCE in three specified states, left- and right-pixel reading back, identity/decided, occupancy and stop; original swapchain path test passed.3692761 Saves the Low/High non-variable slotting rule, and the unrealized resolution options are not yet open to current starter.

TargetSize 1 / PID 39208 The diagnosis was not successful: Python has generated the current reflection configuration, but native has refused at load_archor_config, the scene is not active and XR only shows the startup plane. Keep the hook withdrawn and the portal validated; this is not a double-faced test. The current record does not distinguish between identity changes and time of transcam cameras, and cannot declare root cause. It is adding precise failure to classify with limited frame reading for start-up only, recapsulating actual retrofitting targets.

UiHudPlane 1 HUD view has been changed for the first time, but the visible location is not yet confirmed:791.65 Two seconds, two eyes.2984 Passed by sub-platform correction draw; screenshot used 1734×257 Simulator preview is then condensed to 1280×189 This is not the basis for judging that the original eye map HUD has disappeared. The star, real subtitles and final UI acceptance are still incomplete.

2026-10-14 UiObserver 7 / PID 37184: Index members are physically linked to the Ticket eye matrix and HUD text is also accessed.2650 All consumers, index sequences and eye overlays completed without rejection;1112 Paragraph Texts 13344 The vertex is full range, and the current sample is no longer missing text candidates. Steady pairs 206.034605 Seconds, maximum submission interval 17.8515ms，34 Portal restored. Subtitles are running.14497 The white scene is still pending after the time but not actually glyph; UI coordinates have not been modified.6 The government has also been able to provide information on the situation of the people in the country.3840×2160 And become 3842×2005 Post-fixed size protection rejects the new buffer, displaying failure and not attributable to Win+0 window. The window zooms back to the distribution gap. Code check point 341a 800、6aaff 64 For more details.[UI positioning progress](57-ui-position-and-subtitle-route.md)。

2026-10-14 UiObserver 5 / PID 45216: The Tile topline candidate for the Star-Astronomy, Gun-Embassed UI and Normal HUD has been associated to actual eye-drawing tipt.5840 Additional observation,1693 No consumer content match has been refused;139.80 Double seconds, maximum submission interval 18.10 Ms, open-eye/gun-to-eye screen is normal.33 Access is restored. The path from the insertion of the monitor into the call frame has been corrected and the route rejected and the 44/48 Byte step error; this round uses the independent simulator focus test option, which was not replaced. UI coordinates have not been changed and next is the alignment of the Draw Index and the eye-to-eye correction, with text/subtitles still to be accessed separately.[UI positioning progress](57-ui-position-and-subtitle-route.md)。

2026-10-14 UI Follow-up: PID 44988 The automatic opening of the mirror, cutting and taking of a picture confirm that UI repeats two eyes along the same layout; under asymmetric FOV, the centre is not in the same space.2 / PID 36892 The Scripts were repaired by bypassing the leak classification caused by shared VM.7894 Sub-Cavas Semantic Classification, Rejection 0 The distinction is made between a pro-star/gun exchange/other HUD.32 The entrance is restored; the projection of the two-eye anchor has been tested by CPU, but no actual coordinates have yet been modified. The next item is semantic vertex to rendering batches and correlations with the ticket.[UI positioning progress](57-ui-position-and-subtitle-route.md)。

2026-10-14 UiObserver 1 / PID 43652 Semantic UI Monitor has been physically accessed and is running in two static functions 230.21 Seconds, maximum submission interval 22.45ms，32 Access is restored. WarHD. PostRender accepts 16874 Number of times; no open-scope/slice gun execution, both for the target scope and Canvas call due to Windows refusal to activate the front desk 0 No coordinates were obtained, and the UI position has not been repaired. Subtitles have been confirmed to follow the independent original text route after PostRender's return, which cannot be verified with current HUD monitors instead of subtitles.[UI positioning progress](57-ui-position-and-subtitle-route.md)。

The current target is**Release 1 release support 6 The third person on the D.F. head track is a double-eye, Mod.**. The first person, the armed man and other depths of interaction are targeted for subsequent versions, without prefixing the release of the first edition.

2026-10-14 StartupMenu 1 / PID 34900 Run-through startup shows the VR plane menu, and the same XR session continues with the frame. The menu stops above 180 Seconds still visible, both menu and level eye real-time screenshots kept; stereo output 252.746474 sec, copy 18008 Eyechart, maximum submission interval 18.5689 Use the test list. `tools/vr-launcher/manifest-startup-menu1.json`, the default list has not been upgraded. The real headline menu experience, and then re-entering the level after returning to the main menu, is still not covered.

UI First Edition requires further clarity: User describes**Position deviation point, graphic shape normal.**. Static scripts reflect on the performance of read-only, and position the protometric star, reset, distribution of scripts and drawing entrances to Canvas in support of assumptions that the original canvas coordinates are not consistent with the headline view; no connection or repair has been made by actual mapping coordinates. Subtitles are normal when running: opend/forced-off is marked and white and chart path evidence is required. Next step is to observe coordinates within the precise UI function, processing the protoplanet targeting projection and regular HUD layout respectively.

2026-10-14 Real Rendering 2 / VDXR / PID 10920: User feedback is generally good, and suspicious 3 DoF, remote man-to-person lenses, star-scene and gun-changing UI, not yet seen subtitles, and request to start the game by accessing VR to operate the menu in a headline.889.022952 Seconds, submit 106572 Frame, Copy 81834 An eye map, maximum submission interval 125.2073 milliseconds; average of four primary stages 82967 Right and matrix mismatch 0 Event capacity overflow 0. End command normal stop with user test, XR exit 0、28 The entrance is back through and the game is normally closed. The wheel covers real play observation, which does not mean that all HD or menu paths are passed.`analysis/runtime/continuous-stereo-10920-20261014-145200-9440ece3.json`。

Location tracking is not closed in code: sample position spans approximately X 0.518 m/Y 0.293 m/Z 0.721 Rice, the eye matrix contains a change of position relative to the center point. The third person-size lens is still used as the baseline and no role-point origin has been enabled. User feedback on location and distance remains an experience problem without matrix validation for the other person, nor does it directly switch to first person.

Add First Edit to User 6 D.D.F. Objective: To confirm that head-to-head, front-to-face, up-to-down movement causes the correct direction and ratio of vision to horizontal and close viewing, while keeping normal turn-overs and back-to-backs. Priority is given to checking coordinates for change and world proportions to repair actual problems; the next real machine will need only natural side, front-to-side and down slightly, without extending into heavy harvests.6 DoF, does not add a movement controller or room scale role move commitment. Current code access and old logs cannot alone prove that the experience has passed.

The remaining main line in the first version is: amend ADS-associated and gun-exchange UI with a white scene confirming subtitles; re-enactment of the real experience of the startup menu and necessary life cycle validation; locking down DLSS; and collating copies of deployment packages and installation/dismount instructions. Menu, HUD and parastar are subject to separate authentication, not all UIs are deduced correctly by suspending menus. DLS has increased the start-up mandatory shutdown and recoverable backup, PID 34900 Both fields are valid 0; the stop-to-go menu is not yet available and cannot be declared locked. The lens distance/world ratio can be adjusted to optimize experience, keeping current available baselines.

2026-10-14 PauseUi 1 / PID 14128: Pause final quad return point by calling the stack on run-time d 808d 8, current function numeric section and historical running quickshot etc. quad parameter load with original d 83b 1 c The path is the same bytes. The new version retains geometry/ticket check and identifies the path, draws the whole UI into two eyes for a proven Canvas call chain, explicitly authenticating overlaying links, and continues to reject unknown writing. Automatically starts, enters the level, suspends 10 seconds, restored, normal stop complete; suspended 724→1484, the simulator screens two-eye menu is fully readable.60 Maximum interval of seconds XR 19.7603ms/stale 0 Phase four.4430 Yeah.0 Mismatch, overlay 25974 Group/0 Rejected,28 Access is restored.4423/ Copy 4422 End of the line.1 If you keep it, do not declare strict parity. The simulator previews the freeze construction, old SourceMap 2 Keeps; repeat switching, main menu/reload and real machine new construction is still incomplete. Evidence:`analysis/runtime/pause-ui1-14128.audit.json`。

2026-10-14 Discrepancies identified in the life-cycle suspension (h/PID)43408: The memory and restoration operation of the Start Switch menu is passed, but the simulator's eye amplifier is completely blacked and posted in 590, re-enactment after recovery, maximum XR submission interval 15.644 seconds. Still build new eyesticket,archor in the pause window 4541/0 Phase four.4541 Yes.0;not attributable to the time stamping of the game or only XR expiration door limits. Missing chart window 256 A sample of the existing outage shows new bults continuing, eventually left-and-right eyed draw missing and a whole other 312 Unknown-write contamination is rejected.28 The entry is restored to the sub-pass. Targeting is suspending output paths without relaxing expired or unknown writing protections.`sim-source-map2h-43408.audit.json`Read only UI reflections and confirm that the reference to DOF And BloomEfft in the DefaultUIPostProces is not a cause-and-effect certificate.

2026-10-14 Latest results (g / PID)39464）：**The gap is closed when the non-empty source Map real binary cover passes.** When the official simulator portal is activated, the handle is recognized and the camera is being cleaned.109.369°And then back off.76.393°, no hidden list or game parameters. Original non-empty candidate 4055 Accept 4055- No, no.0 Max 13 Node, six Map checks are all passed; phase four 4944 Right and matrix 0 About.60 GPU release in seconds 4930 XR copy 4930、projection 5414♪ Stop normal, stop normal ♪28 Portal restored, XR/entry/driver/ camerahelperrealexit 0. Proof prefix:`analysis/runtime/continuous-stereo-39464-20261014-115843-3f9a9a76`;actions:`analysis/runtime/sim-source-map2g-camera.json`. This confirms that the real conditions that would have triggered non-empty protection can be rendered on a continuous basis without proving old 44892 The pointer is valid or all historical black screens are resolved. Stop digging deep into the collection and then go back to the life cycle, clarity and current build range of the real machine.

2026-10-14 Natural non-empty condition found: PID 34688 One turn to the camera at yaw about 69–88°The blog also shows how the government is doing this:9 A consistent sample of HiddenActors contains Pawn_COGMinh, each of which 13 A suitably qualified component, back to empty.167, which is therefore the condition for another player to enter a hidden field and does not prove that he touched the wall. The whole process does not contain a list or parameters of hidden operations. This gives a normal operating recovery path for the next double-eye run; single-watching does not yet prove that real source Map copy has been successful.`analysis/runtime/camera-positive-orbit-34688.audit.json`。

2026-10-14 Enter path to follow-up validation: PID 34688 the virtual handle is connected, but the original device entry player assigns value to -1；19ab 10 . A normal Back button makes the allocation into a 0, the only right swing that changes Controll and POV yaw 20.484°, input is back. No game memory written; the UI inn hasn't changed, LastPlayerIndex from -1 And become 0 is expected. Previous catch of marking assumptions have been ruled out (in original View+)4 ec The real value is 1. This explains why the auto-scenes test was previously inactive, which does not amount to black screen root cause. Next step is to trigger HiddenActors with normal camera changes and then check SourceMap 2 The true copy and continuous output.`analysis/runtime/input-assignment-34688-once.json`、`input-assignment-34688-right-continuation.json` and `analysis/ida/input-routing-20261014/`。

2026-10-14 Subsequent orientation reverse confirmed 0x140 A normal production path: a native view construction 15cf 08 PlayerController. HiddenActors, then collect the eligible PrimitiveComponent from Actor. AllCommponses; bIgnoreidenActorsMembership will exclude components. Camera function 110878 The current PID should not be used as an illegal entry.34688 Reflection and 10 Sector run code/crack-byte match, HiddenActors empty; self-role horizontal radius 40 Current point distance role 167, it is explained that the stationary test did not cover the branch.44892 The complete collection was not maintained to prove the validity of the finger; sourceMap 2 Non-empty measurements are not completed. A normal roller command has not changed the POV and is searching for a path; only 640 The pixel image confirms the normal level, and presence at the PenderCrosplayInvitePopup does not amount to visible blocking windows. Next step is to prove that the input actually changes the camera, then triggers a hidden assembly by approaching the role/handing wall with the normal camera, and checks non-empty count, copy independence and GPU output successively.`analysis/ida/source-map-producer-20261014/README.md`、`analysis/runtime/source-map-producer-34688.json`。

Recent developments (sessionExit)1 / sim-source-map 2 d: failed to locate the previous round exit as application was waiting for STOPPING, but the emulator installed is in PID 3852 The new XR build records EndSearch successfully and finally. Do not forge standard handshakes. Only destroyed objects are passed safely after GPU closes down, timeout/lost is still unsuccessful.5 No game in seconds precheck exit 0，7 C++/ABI self-measured, runner 28 The test passed. Follow-up PID 38688 Autostart and run 120.003 Seconds, phase four. 8816 Yeah, it's a little bit of a chorus.0;publication 8808 Copy 8807、XR projection 10810 Maximum submission interval 24.3774 milliseconds, shale 0;owner/XR/stopaverageexit 0、28 The entry is restored. The simulator MCP six fields actually read back and the XR CSV independently match yaw ⁇15°、pitch 8°、roll 8°, XYZ changes; does not consider the queued return applied. The right swinger is successful but the telemetry is only moving and position/rotation remains unchanged without proof of actual transject. Non-empty source Map is still not covered and image instruments are incomplete.`analysis/openxr/sim-exit1-preflight.json`、`analysis/runtime/sim-source-map2d-38688.audit.json`。

Start delivery new `tools/vr-launcher/SimulatorVR.ps1` Independence `manifest-simulator-preview.json`, automenu into regular starter `--auto-continue`Common 120 Second-time and independent player ready to judge;`--runtime`The new entry PID is available.34688 Independent measurement of passage: regular scripts when automatic five-step menus are used 21.469 seconds;observation thereafter 60.003 Seconds, release = copy 4622、projection 5415 Maximum submission interval 14.2133 milliseconds; phase IV 4630♪ And it's a little bit more like a "middler" ♪0,owner/XR/stopaverageexit 0、28 The entrance is restored. No encapsulation navigation, no extra games or attitude input for the entire journey. Core C++ test 7 Entries, Runner Tests 28 Division, regular launcher/navigation test 31 Item, Menu Test 40 The items are adopted.`analysis/runtime/continuous-stereo-34688-20261014-105921-a1581368.*`、`sim-source-map2e-single-run-control.json`. Target is not yet complete: old source+0x 140 The condition coverage, real header current construction and more complete life-cycle ranges are yet to be verified; this entry is a preview of the simulator that has validated the current level and not a statement of compatibility for all game states.

2026-10-14 The first validation of the actual machine has been completed for menu automation: add `tools/game-io/menu_state.py`(Read only reflectors/focuss/blast windows text) and `menu_control.py`(Send window enter as accurate).16336 Online Service Error, then 5.313 Automatically complete Campaign & SinglePlayerContine in seconds, without a stopshot of the navigation. The player then independently validates the backlink, WarPahn type, Level ' s attribution and advance camera time to confirm entry; this round is not injected into VR. 1 milliseconds, window-sharp 1.6 msec;35 The item is tested offline. The title page contains AttracLauncher without input overlay, matching with a dedicated scenery; unknown bullet windows or layouts are not automatically confirmed.`analysis/runtime/ui-control-16336-online.jsonl`、`ui-control-16336-to-level.jsonl`、`ui-control-16336-level-ready.json`. Next use this script to start validation and run SourceMap 2 High resolution double-eye; automatic entry level is not equal to 4 The K black screen has been repaired.

Follow-up full chain PID 33964（sim-source-map 2 c: Initialization of menus 13 In seconds, finish title by status, identified online error, Campaign, SinglePlayer, Continue five times confirmed, controller time used 21.297 seconds; the original starter independently confirms player ready. Add initializing waiting to de-link test 40 Item Map adopted.2 Completed in user-specified OpenXR Simulator Runtime 180.0067 Second observation; a real-time MCP screenshot confirms that both eyes have game scenes. 13320 ♪ And the Matrix ♪0 Source Rectangle 3840×2160,Press 13315 Copy 13314 XR Projection 16211 Maximum submission interval 19.9289 milliseconds. Native 28 Access is back on track, but XR exit=1, shutdown_completed=false, the whole round has failed; destroy calls do not successfully equal normal session exit process passed. Non-empty Map branch candidates=0/accepted=0, therefore the repair is not validated as effective under the original malfunction; image document records also have a volume missing item that cannot be asserted to be complete.0 The next survey stop-state machine, with controlled scene/portal changes covering non-empty Map conditions; no change to production method, is available.`analysis/runtime/sim-source-map2c-33964.menu-control.jsonl`、`sim-source-map2c-33964-mcp-live.json`、`continuous-stereo-33964-20261014-103320-e635db57.*`。

2026-10-14 Follow-up operation switch: VDXR 7 Stopped pre-screening when xrGetSystem returns FORM_FACTOR_UNAVAILABLE, no new rendering is activated; prime VI is independently restored. Users are then explicitly re-used `OpenXR-Simulator-v1.5.0-win64`. The naked relative DLL path for its installation failed in the current loading environment and has been built `analysis/runtime/sim-absolute-runtime1.json`, only write the DLL path as absolute and for subprocess environments. The original installation list and global runtime have not been changed;264 Frame post-only precheck valid, exit normal 0. This is a simulation of attitude.2 Supplementing the review of ownership of the re-constructed source pool, which prohibits release and isolation if it is stored or read out unsuccessful;14 Local check pass, to be validated by the game. Evidence:analysis/runtime/source-map2-native-ready.json、analysis/openxr/sim-absolute-runtime1-preflight.*。

VDXR 6 PID 44892: MotionBlur is the only one that will be able to change to False.4 K source map started to be released, actually every eye 1920×2160,Press 17 Receiving 16、projection 43, Overwrite only 0.3504181 sec. then source+0x140 Non-empty trigger old input protection (mask=32(i) Actively stopping the sighting;28 The entry is restored and the XR is properly closed. The user exits VR after seeing a short double view of the desktop, with its head was always black screened, so the round shows no acceptance and submission of frames is not a substitute for showing proof.analysis/runtime/vdxr6-44892.audit.json。

In response to this suspension, targeted counter-edition and PID 44892 . Five-part code bytes confirmation: Numerical view construction function is copied item by item +0x140 Gather, assign separate nodes and barrels; the resolution releases only these storages, payload being quoted as borrowing values. Adds a border reading, chain-to-bunk coverage check, eye/source storage independence, content consistency and source unchanging checks; reserves other input protections.1 Build and 14 CPU/ABI is approved, covering normal reproduction, bad chain, aliases isolation and tectonic change. Evidence:analysis/runtime/source-map1-native-ready.json、analysis/runtime/collection-code-44892-20261014/bytecheck.json. The new game process has yet to confirm the real and non-empty assembly and continuous headline output; no production startup list has been updated.

## What are we delivering?

First edition: Single PCVR Mod of War: with real eye vision and head rotation, smoothing 6 DoF. Keep the original mouse or handle moving, aiming; independent VR hand operation is a subsequent milestone.

The third person is now the target for a staged acceptance: same eye, same eye.6 The first person says that the eyepoint adjustment is kept as a follow-up experiment, not a prefix to the third person's term phase. 75% It is an estimate of the work; real head-to-head chain, level and breakup recovery, historical output gap and exit anomaly remain substantial gaps that cannot be replaced by simulators or CPU self-measurement.

2026-10-14 User supplements: FOV, menu and level automatic switch requirements belonging to VR Mod. Phase acceptances are increased: menu panels with visible stability after startup; automatically cut to the same frame when a valid level player is identified; keep XR sessions while suspended, loaded, reloading and present interface correctly, restore view attributions and continue to be double-eyed. Each eye is now projected but headline edged, HUD readability and special lenses are not yet fully captured; existing entry points are still open after entry of the passboard, and full menu processes have not been achieved. The menu board shows no world appreciation for two independent views.

Real VDXR First Wheel PID 40060 The user confirms that "the right and the left eye see the game scene" and gives feedback on the first experience.366 The four-stage matrix matches.357 Game Image Receiver and 874 Sub-project Commit, Overwrite 7.274807 seconds;slip source 0/rejection=1) Trigger proof_failure, unfinished target 120 seconds. View attribution check 367/rejected=0;XR exits normally,28 The entrance is restored, the game remains at the level. Starting anchores of buffer zone differences are not malfunctions: binding changed=0/rejected=0, no first_failure. Specific failure fields are to be recorded in the next round; users confirm that there is a map of the two eyes which does not replace depth, scale, head start and long-term stabilization.analysis/runtime/vdxr1-40060.audit.json。

Real VDXR Second Round PID 40696: Completed 120 Second observation, user feedback "tried out" but the picture is rather biased and wants to increase resolution. 6545 Yes, the matrix is zero, both published and received 6276;belongs to the matied=6546/rejected=0,split enter checks=63/rejection=0, the old input failure is not repeated. XR exits normally,28 The entrance is restored. 4192.432 ms output interval: keep the middle scene construction and left-eye drawing continued, but frame-link polluted (128 The reception and absence of maps are both capable of being lost and cannot claim to be complete.analysis/runtime/vdxr2-40696.audit.json. Current SBS General 1920×1080♪ Every eye ♪ 960×1080;the next round of games where users select higher resolution, priority 3840×2160, the actual source map and XR dimensions still need to be run for record confirmation. Users work together mainly with normal play and brief experience feedback.

High Resolution Initial Measure: Users select in Game menu 3840×2160. The INI DLS=True is a functional switch, AA DebtQuality Mode=4 corresponds to AAOnly; cannot be misrecorded as DLSS hyperresolution. VDXR 3 PID 41952 Timeout during menu waiting period, no binary; then pime watchdog resumes and does not have high-resolution rendering failure. Keep settings restarted VDXR 4 PID 440 The game has entered the level and opened for capture, but before the front chart the self-responding (user confirmation) is zero, with no actual proof of eye size. XR finishes; game abnormally leads to native recovery unrecognized. The offline dump process exits from the cleanup chain without explaining why it was originally withdrawn or because of DLAA evidence:analysis/runtime/vdxr4-440.audit.json、analysis/runtime/game-crash-440/offline-unwind.json. Next item to keep user 4 K+DLAA settings to start a baseline comparison without injection of VR Mod.

Follow-up user screenshot and INI display settings changed to 3840×2160 Noone, Native, DLSS=False, Texture High, Dynamic Fuzzy On. No VR injection baseline PID 46028 Survive at the gate. 60.0118366 sec, no new WER; neither the initial nor level module list is available. Initial/end configuration changes are retained and therefore this result only supports the current 4 Short-time baseline set by K Native, not proven 4 K+DLAA is stable and not a single variable contrast for the previous round. The next round will be validated with the latest user configuration 4 K Native binary does not automatically change other drawings. Evidence:analysis/runtime/vdxr-baseline1.json。

VDXR 5 PID 39356：4 K Native High-quality Sub-Purpose Blackscreen, published/coped/projection zero; phase four 7367 Yeah, every eye. 1920×2160, the matrix is zero, submit input checks=63♪ And I'm gonna be a little bit more like you ♪7426/rejected=0. Normal stop and verify 28 Enter is restored, the game keeps a normal image. The direct thread is an output path change: original gpu_output_quad each eye from d 83 cbf, real backbuff Draw reservation call stack from 2b 11a 0→2b 0bdb→11a 6f 9;available Finnish_eye only d 83b 1 c, so all backbuffer Draw is recorded as unknown and the capture attempt is zero. The white list cannot simply be added without verifying rectangles and sampling attributions. Capture.cpp for observe_output_stack 1920×1080 Conditions are filtered only, not controlled for publication. Users can not use root cause. The game after the user exits the memory of MetaXRSim is still black screen; read remaining physical memory after exit 17.5 GiB, Visible Clan 23.4 GiB, without recording pre-exit peaks, resource competition and release failure are investigated separately. 4 K and other graphics, only changing MotionBlur from True to False; earlier 1080 Configure also closes the item, but it is not PID 40696 . Evidence:analysis/runtime/vdxr5-39356.audit.json;directed static back-coding is stored in analysis/ida/vdxr5-output-route/, to supplement the call stack only, preserve type and reposition extrapolation limits.

6 The six degrees of freedom for DoF: turn around to the left, watch up and down, roll back, and move right and left, up and down. When you sit still and just move your head next to your side, you should be able to see the side of the object from different positions; this is the same as only allowed to turn around. 3 DoF has a difference.

6 DDF tracking and stereo rendering are two separate conditions. The same game picture can only be debugged as a display, not complete with the binary stereo. Camera positioning is not equal to controlled cameras, let alone VR already exists.

This is an evidence-driven study path. Same frame double view, original impact response, real per eye/fOV desktop double-eye scene, and in Meta XR Simulator 120 Second game GPU image projection link is already verified through the current level. 36212 The complete six-axis simulation and two non-zero posturing returns were completed, with the independent matrix audit and normal exits. 41704 Come on. 1431.7773 ms output interval and independent exit cleanup error, round failure. After exiting competitive correction, diagnostic version PID 1756 It's... 60.1401 Maximum interval of second repeat 18.0283 ms, neutral exit through; failure to repeat old missing maps does not amount to a position root cause and stops tail release/receipt 3386/3385 The strict parity remains the false. The continuous mode 180 Second output and joint stop has been verified by sub-checking, history PID 43724 Strictly issued/receivable parity checks remain in false; longer, more critical life cycle, stable first person and real headline show still to be accepted.

Enter Link Supplement (PID)34688: Actual XInputGetSateQTQQ only slot 0 Connect, LocalPlayer. ContractorId=0;bIgnorreMoveInput/bIgnoreLookInput both Byte counts 0. Then the right-swipe sync sample is taken one second at a time 60 OS right axis, again.30 Number of times 18022 and ending back;playerInput 4float continues 0 The view is that the "Os" do receive, but no side response is observed.30 Hz sampling does not exclude short-lived axes, but does not yet locate internal filter points. This round does not include role moves or non-empty assembly overlay. Next game needs to be checked for input backend/routing and the pole cannot be blindly repeated.`analysis/runtime/input-sync-34688-once.json`and`.samples.csv`。

## What have you done?

- [x] Confirm actual game path, EXE SHA 256、x 64 Structure.
- [x] A copy of the depressure analysis, reading the list of objects and some scripts for Engineering / WarfareGame.
- [x] The candidates for the WarPlayerCamera, CameraCache.POV and the Native Calculator portal were found.
- [x] Validate the current camera cache by stationary and lens input.
- [x] Checks the camera as a local player relationship for WarPC / WarLocalPlayer / Marcus.
- [x] Writes an automatic location tool that does not depend on the old stack address and validates that the error game version is rejected.
- [x] Cross-relaunch authentication autoposition: PID 35088 → 32532, POV address changed, independently deciphered successfully.
- [x] Validation of automatic location after reloading checkpoints within the same process: PID 32532 No change. Independent analysis and player relationship verification is done.
- [x] Dynamic capture raw scene view build call, current player identity, matrix and DXGI Present; check the game function portal back after you have stopped.
- [x] User observation confirms controlled yaw swing impact images; six-axis microturbation logs confirm three-axis rotation/diversion into original view and restore to baseline.
- [x] Independent OpenXR sessions, left-eye test maps GPU read and submit; coordinates converted and position validity checked through offline testing.
- [x] Quest 3 / VDXR Rotation Peace Transfer Input, Test Chart Header Prominent Continuous Display: Samples are validated with user observation.
- [x] Quest 3 Positions enter the current level at a primary temporary view point: three axes of rotation/diversion, matrix changes and recovery of entry after stop.
- [x] Game window image displayed through OpenXR 2D panel to Quest 3, GPU pixels readback and user observation confirmed pass; this is not a bi-eye stereo.
- [ ] Game image submission, head tracking access, eye stereo and integrity 6DoF。

Current stage: GPU shared images with frozen pose/FOV  ⁇  Meta XR Simulator program player. Recent PID 36212 attempt 1 It's... 120 Second test is devious  ⁇15°⁇  Upside/sideside 10°XYZ is equal to  ⁇0.06/0.04/0.05 m and one in each of the turn-overs and head-up periods. 6,096 Yeah, release and receive. 6,090 A new game image, submit 8,639 Frame protection, overwrite 119.968899 Seconds, maximum interval 17.2533 ms。12,192 Consumption/preparation of left eye matrix matching with respective reference intergenerational independent reconstruction, maximum error 0.00395 {\cHFFFFFF}{\cH00FF00} native numbers; no disability compared to four stages of two eyes.28 Restore entrance and native/XR exit 0 All adopted. New tutus 50.75 Hz, protocol 72 Hz, which contains reuses, cannot be referred to as a new frame rate. The current cut-off is still the third person ' s reference point and has not yet been stabilized for first person.

Official Oberator independent test observed Raw head during override 3/view 15 returns the same value does not restore tracked; call releasing before returning head 15/view 15. Default source 0 Still demanding, head 15/view 15。PID 40776 Completed Visibility source 1 Dynamic game protection: yaw margin 30°x Range 0.12 m/ The four phases are 1,601 Yeah, new GPU. 1,591 Chang, prevention 2,158 Frame/29.95637 Seconds;release, portal restoration and independent matrix audit passed. Previous PID 38112 The simulation input cannot be considered a head start tracking acceptance because the real time CSV sharing lock failed, and no action was executed.

Ongoing running access has achieved visible start/stop, session identification and joint closing of native ⁇ XR. 43724 We've got a visual. 180 Seconds continue to output, cross-doesn't. 120 Seconds are still updated, and post stop is clear. 10,632 Yes, XR. 14,401 Frame/200.010565 Seconds, maximum interval 28.5519 ms. But final release 10,615 Chang, take over. 10,614 Chang, strict counting of equal numbers of audits is not possible to declare the entire acceptance pass on a continuous basis. Real head start-in PID drives desktop rotation/diversion 39232 Validation still valid. 1 The next checker menu/gate life cycle, breakback recovery, and real head-show and physical scale; scale=100 Active TAA, other levels and comforts are not yet accepted. See document 56。

Continuous 2 Unincidentally quit protection for refilled starter: PID 33264 Force the end of a self-creation owner only, not sending stop events;native/XR app 4.328 The government has been able to provide the necessary information for the event.28 The entrance is restored, the game still survives and returns to normal mono.1 is clearly documented downtime for the owner, which has been independently abnormally reinstated; old running report from the original owner was not rewritten.

Unified entrance in the warehouse `tools/vr-launcher/launcher.py start/status/stop` Already in PID 44640 The entry does not itself navigate the menu, and this round is supported by a game_io-assisted entry level. New charts are published/received. 2,334 Chang, projection 3,166 Frame Overwrite 43.959 seconds;stop, GPU dry/ close with 28 The entrance is restored and there are no remnants of the current cycle after normal game closes. It remains dependent on its own warehouse layout, not a copyable deployment package. `analysis/runtime/vr-launcher-smoke-44640-audit.json`。

First person said to read only samples obtained 448 A record of the difference between the original camera and Pawn (-)160,+48,+76# I'm not sure if you want to be a good girl # 60。`Pawn.Location + Z*EyeHeight` Still only candidate points, no renderer alignment or body mask verification; subsequent dynamic sampling has been observed that Pawn, Controller and camera rotation cannot be considered equivalent.3 Two simulators back to check the reference generations. 1→2→3, horizontally preserves the real pitch/roll back; the unified entrance command is also in PID 41704 and 1756 Measure, the whole round results and limits are kept in the document 56. Reload or breakup recovery, stabilize first person and real headline still show a delivery gap, runtime maintains Meta XR Simulator.

Add a default closed eye-linking monitor, PID 44200 It's... 3,727 Scene/family/renderer Linked Counts and 64 The article retains the endorsement through static consistency checks, but does not observe a physical camera shift and has not yet completed dynamic validation or application of first person position. The wheel emulator output is approximately 61.10 Seconds, maximum submission interval 16.46 ms, stop and restore everything. 28 entrances;distribution/reproduction 3,330/3,329 The strict parity still fails. 15204 End before activation because entry level is over; PID after correction of menu schedule 38012 Automatically enter the level, but not enabled at the cross-reference check at the end of the certificate and actually exit after XR cleanup `0xC0000409`certificate has been corrected for all border re-entry 49 A CPU test, pending validation. Exiting dump is located in Meta emulator `SimRpcManager` Failed to create clean-up threads at DLL unmounting; no game, no Hook independent PID 20656 The same test is repeated. 32 Frame abnormality calls chains, the reason for failure to create a lower-level thread has not yet been proven. The subsequent treatment of simulator exits and reconnecting dynamic eye points is required, with first person A/B/A. First person refers to the support module as only CPU tests, but has not yet reached the live machine; complete evidence and limitations are available in the document 56。

Follow-up PID 21208 After restoring amo 437 A dynamic accepted sample: Pawn pitch remains zero, Controller/POV pitch changes with short input time.23 The two do not rotate; this is a non-atomic observation, which cannot be interchangeable with the rotation sources. IDA has already applied to the camera subject of the running snapshot RVA 0x110878 Directed analysis to confirm mode calls, local position plugs and subsequent rotation writing is still unknown.

PID 42228 Added dynamic eye link:36 A reservation letter appears 4 Speculate original view towards the yaw horizon 4.279°,pitch Range 2.675°, Scene/family/renderer 1,912 The number of acceptances and rejections is zero. 1,551 Subsistence, matrix does not match zero; XR output 30.1476 sec, maximum interval 16.5231 ms, all current control processes actually exit 0，28 The entrance is restored and the game is properly closed. 1,541/1,540 The strict equivalence still fails; runtime head changes at the same time 13.411°, the entire change cannot be attributed to a game only.5 The certificate board was successful for the first time, so a limited retry itself is not yet triggered in the actual game. First person is called A/B/A by default completed 13 Group automagnosis and independent code review, but not yet in play. Next priority is given to receiving a new XR for the third person's checkout of the boundary instead of first person's startup.

**Failures and limitations of reservations:** PID 38784/21208 The controller did not really send a request for an answer to the problem of cross-clock domain and whole log resolution; it was amended to read QPC with a border tail.36212 Two requests and practical applications were adopted.38784 The appointment. 3 Second projection gap and 21208 The XR exit exception remains a failure;36212 Not repeated, does not mean that its root causes have been identified or repaired. 45440 The precise action time window conclusion is maintained by the drop in the clock field, original attitude, matrix evidence of the same QPC domain;36212 Use the actual record of perf_counter/QPC clock to verify all actions. Strict source windows now require producers to skip zero, which is not a precise link proof for individual consumer samples. Corrections, contractual boundaries and evidence of failure are available in documents 56。

Third person 1（PID 24396 Further confirmation of end-of-cycle differences: original release 3,102 Chang, XR receives serial 1–3,101 All successively, each has a successful response to the request.4096 No missing diagnostic capacity. Last received ticket 3107, native final release ticket 3108 Not accepted, final retain the same final receipt number. This round is different 1 is located in the end unreceivable release, has not changed the stop strategy and does not reverse all of the old rounds. 61.0634224 seconds,4,398 Sub-projection, maximum interval 16.7439 ms;XR exit 0、28 Access is restored, GPU closes and normal closing passes without any trace. See full evidence available at: http://www.un.org/en/g/wp/c/d/1-24396.udit.json and this prefix.indepent-xr-tail-audit-v 2.json. Next steps should prioritize level/recovery and real third-person acceptance, maintaining emergency stoppages, outdated postures and verification of all sources; if normal end frame consumption is stopped, then the distinction between normal stop and failure ends must be distinguished, and stopped images cannot simply be released to pursue equal numbers.

## Phase 1: Every time I find a player camera

**Question:** Memory address changes with object creation, level loading or process restart.

**Theory:** Search for the base of the current module, find player cameras by class name and object relationship using the engine log entry that was validated in Hashi; read the CameraCache/POV field deviation from the reflection metadata.

**Frontend analogy:** Do not remember the address of a temporary DOM object on the page, but re-discover the components and nodes with a stable identifier. Old references may fail when the page is remounted, and the camera is recreated similarly.

**I'm responsible for:** Address resolution, version fit, invalid object rejected, log.

**You're responsible for:** Reload the checkpoint, restart and enter the level on a hint, and rotate the lens to the right or the left, if necessary.

**Acceptance and acceptance:** The current local camera is found in all three situations, running, reloading and restarting. Output values match the actual operation. Addresses can be passed without change; it is critical that the new process is independently analysed successfully, not required to change.

Tools:`J:/gearsVr/tools/package-inspector/Resolve-Camera.ps1`。

## Phase 2: Control the first person's point of view at the right time

**Question:** Found CameraCache only a point of view to save the game; changes may be overridden too early, modified copies may not have visual effects and script updates may not be synchronized with rendering.

**Theory:** Tracks the path of "Clean Game Camera View  ⁇ GPU Draws" to determine where and when the actual reading of the follow-up image will take place. Hook can be understood as adding a packaging layer to the original function, retaining the performance of the original function, and then observing or adjusting the results at a verified moment.

**Frontend analogy:** Change state, call component render, browser to show the image as a different link. The game logic update, rendering view construction, and Present cannot be confused.

**Trial order:** Read-only tracking of small fixed rotation experiments restore original values  ⁇  first person viewpoint validation. Use protocol first 1° The correct position of the predictable deflection test is then visible.

**I'm responsible for:** Native function analysis, Ghidra/recompilation, call time sequence evidence, switchable experimental code.

**You're responsible for:** See if the whole scene changes as expected, switch switches, confirm point of view; keep focus.

**Acceptance and acceptance:** The deflection is repetitive, avoidable and does not accumulate; it really affects the whole scene view; it distinguishes between a pause in the game and an interface. The initial first person is considered reasonable, not inside the body of the role.

Ghidra is a tool to read original functions; it does not automatically generate the full C++ source code of the author, nor does it automatically find VR interfaces. The C++ compiler and JDK 21 Complete when required.

User reports that IDA headsles are available and can be used for subsequent reference games if required; this round does not need to repeat the calculator example or use tools as a game reverse or VR acceptance conclusion.

## Phase 3: OpenXR with a head start

**Question:** The game rendering and headline operating environment are two systems.

**Theory:** OpenXR provides a session, attitude, eye view and image submission interface. First, you check head-to-head path with an independent test scene, then you take the game images and avoid checking two unknown systems simultaneously.

**Frontend analogy:** OpenXR is an external SDK: Prove that API connections and the minimum examples are normal, then connect to business applications. Game adaptor level is responsible for inside the game, OpenXR layer is responsible for headline communication.

**Steps:** Confirm actual header/serial/OpenXR Runtme  ⁇  Independent square scene  ⁇  Show attitude values  ⁇  D 3D 12 Image capture  ⁇  temporary same image for eyes display.

**I'm responsible for:** Independent program, graphic resources synchronized, time sequence submitted, image colours, diagnostic data.

**You're responsible for:** Starts the physical headline connection, confirming that the test object can be seen, turned, crooked, smoothed and fed back.

**Acceptance and acceptance:** The independent scene is normal for the eye and six-axis tracking data are valid, and games can then be continuously displayed. Even if the game remains a same eye, it will only be marked as showing road passage.

References: Khronos Official Hello_xr includes D 3D 12 examples;no direct reference assumptions 119.559631° FOV or colour settings are suitable for the current header. Displays the parameters to be checked with the actual format and per eye view.

## Phase 4: Let the game follow its head with a point of view

**Question:** Images can go up and down, not to say that the video game moves when you turn around.

**Theory:** Read the header attitude relative to the center position, switch to the game coordinate system and then combine it with the original game view point. Test first to turn around, then look up and down, then turn over; do not add the four-digit fraction directly as an angle of the Ori.

**Frontend analogy:** Displays the state with raw status and input extrapolation, instead of adding the same increment to the old state every rendering.

**I'm responsible for:** Coordinate conversion, rotation combinations, centering, tracking validity, image and attitude correspondence.

**You're responsible for:** Sitting on a small three-axis rotation, verifying direction, scale, center and drift.

**Acceptance and acceptance:** Turning in the right direction, stable after stopping, repeatable and running continuously without cumulative deviation; no clearly different time-time image or attitude error pairing is made. This stage cannot be described as complete 6DoF。

## Phase 5: Prove a real eye stereo

**Question:** We need to get the game to draw scenes from two eyes.

**Theory:** The left-hand eye has a point of view and projection information, with different visions between the near-eye and the distant. The ultimate goal is to see the right-and-right view in the same game state.

**Experimental policy:** Users have explicitly accepted the same frame only, and AER does not act as a route of achievement. The main line is to submit two original views in the same game; if there is evidence that the path has an unrecoverable sharing problem, double-scoping with the frame order is assessed. The latter draw no forwarding game logic between the two drawings, and need to manage eye maps and historical resources separately, without directly repeating consumption functions that call on the object that destroys.

Current confirmation of birth P through submission, consumption, clean-up and thin image acceptance in the same frame view 00/P 8 Image response, desktop head start phase driven by real XR double-eye pose/FOV and visible rotation/diversion of the same frame. Image ticket has received a real GPU output, completed in Meta XR Simulator 120 secstatic with 30 Second-spectrum simulation game; dynamic matrix time series/value corresponding to audit passed. The ongoing mode actual output has been crossed 180 Seconds and normal stop, end release/receiver differential 1 Zhang's strict equality check still falls, matching code extrapolation of a single slot to close the frame. See document 56. Next to complete the menu/level/break-up life cycle and real head start display acceptance; physical scales and active TAAs are not yet accepted.

**I'm responsible for:** Two-eye view and projection, retrofitting target, image life cycle, depth and reprocessing history, eye and attitude attribution.

**You're responsible for:** Observation of door frames, peri-wall angles and visions, confirmation of depth, right or left eye order, reobservation of moving objects.

**Acceptance and acceptance:** The static scene is characterized by a real geometric difference, with both the left and right eye images continuously valid and proportionally and viewed in the correct range; dynamic tests record whether there is an eye time difference, a reflection or historical contamination.

This is a critical point of feasibility. If the interface is inadequate, resurveying the rendering path cannot be completed by copying or placing the same chart.

## Phase 6: Add head horizontal, done 6DoF

**Question:** The position of the game observation shall also change when the body moves to the right, the upper and the lower, the back and the left.

**Theory:** Enter the relative headal shift by coordinates and scales, using game view points as anchors; then combine each eye point. The world scale is calibrated by means of a visual calibration, without "100 Game unit = 1 The rice is directly considered as verified.

Concept:

```text
每眼最终视点 = 游戏基准变换 × 转换后的相对头部变换 × 每眼变换
```

The multiplication number here means a combination of three-dimensional variations, which cannot be understood as the direct application of all angles and positions to normal numerical multipliers. Ultimately, it also addresses reference space, centering and attitude validity.

**I'm responsible for:** (b) The movement of the place, scale, first person mooring, mid-term, non-cumulative application and behaviour in case of temporary tracking lapses.

**You're responsible for:** (c) Validation of the blockage relationship and movement ratio at slow, right or right location detection, up and down, and back and forth.

**Acceptance and acceptance:** Each of the six axes is independent in changing the point of view; only when it does not turn, a new side of the object can be seen; after mid-turn, repeatable; and the camera keeps synchronized with both eye results.

## Phase 7: Turn into a fun-able Mod

**Question:** Individual scene work does not represent normal play process work.

**Theory:** Defines behaviour for regular games, menus, pauses, over-the-counters, bunkers and other special shots to handle first person-name model masks, HUDs and visual changes.

**I'm responsible for:** Status switching, camera life cycle, special scene adaptation, performance measurement, logs and installation/disload description.

**You're responsible for:** Chapter I, reading of feedback menus, targeting, bunkering, roll-over, cross-field and pause recovery, was tested using the original input.

**Acceptance and acceptance:** Chapter I core processes can be repeated; reloading, autopositioning after restart is effective; known anomalies are clearly recorded; switches and restorations can be performed. Additional milestones are set for hand-to-hand operations, weapons IK, manual switchup, saw action etc.

## How do we cooperate with each round?

1. I will explain the specifics of this round.
2. I have explained the rationale in a few words, and only introduced the necessary terminology for this round.
3. I'll explain in advance what the back-office code will read, whether it will be written and how the experiment ends.
4. You only do one clear operation; I don't grab the game focus, try not to use the computer-use.
5. I report on the results of actual observations, distinguishing between what has been proven, extrapolated and what remains unknown.
6. Update the status of acceptance in the report and this route, with a further round to begin.

Changes one key variable at a time. For example, "Standing and turning right or left, just looking up and down, moving only," without running, turning head, aiming to verify a field.

The script is not suspended as a rendering pause. The game loses focus, so the interface is measured by recording the focus/play status and the rendering time order to avoid error.

## Learn what you want from the project.

| Development Time | Just learn the concept first. | Front-end counterpoint |
| --- | --- | --- |
| Location | Object, pointer, field deviation, object life cycle | Object Reference, Properties Access, Component Remount |
| UE Target Survey | Category, Examples, Reflections, Metadata | Class / Example, Runtime Type Information |
| Call Path | Scripts, native, Hook, Calling Inn | JS/BORD, Function Packaging, DevTools debug |
| OpenXR | Sessions, posturing, reference space, image submission | SDK Sessions and Interface Call |
| Head rotation | Point Euratom, Four-Datas, Variable Group | Group order for 3D transform |
| Two-eye drawings | View, Projection, Rendering Targets, Frame Sync | Camera and framebuffer; new graphics knowledge is required for the latter part |
| Head Pale | World/local coordinates, scales, relative shifts | Father and son transform, unit conversion |

You do not need to complete the UE, C++ or graphics first. The corresponding knowledge is added by a wheel; then you need to focus on C++ when writing performance-sensitive game codes.

## Next

The basic point reference orientation of the first person has been confirmed by the user. See [First Person camera reference log](12-first-person-mod-analysis.md#已采用的第一人称相机参考方向). Study its shoulder-side offset zero, split vision points and body display processing, then build our own first person anchor; head up and move with high altitude not to act as VR stand/ probe input. This is the record and follow-up direction, which has yet to be achieved.

Limited real head start, 2D display of game images, current level and frame view, original input effect, real XR-sync double screen desktop header, and emulator 120 Second static and PID 40776 Oberator source 1 Dynamic game program passed, see docs/14-live-head-camera-test.md、docs/15-game-screen-display.md and documents 56. Dynamic tests keep Raw flags, motion, freezet endpoint and consumption/preparation matrix time series corresponding to all of them, which cannot be validated as physical head start. 43724 Continuous output span 180 Seconds, definitive stop and joint clean-up pass but end release/receiving differential 1 The difference is consistent with the silo stopped boundary syntax and does not contain packet end-of-story evidence; continuous cutage and joint lifetime session sub-column without following up on zero-disturbation limits. The next step is to verify the life cycle of the menu/gate, breakback recovery, and the synchronization, physical scale and comfort of real head-on images. The actual runtime and eye configuration must be reconfirmed at the real head-to-eye stage, without using analogue identity or previous SteamVR parameters. Active TAA, live-in, active breakup, and tracking restoration and stabilization anchor points remain to be verified. docs/10-vr-interaction-design.md, deployable delivery requirements are available docs/16-deployment-target.md。

## Official reference and project evidence

- Camera evidence:`J:/gearsVr/docs/02-camera-analysis.md`。
- Positioning tool:`J:/gearsVr/tools/package-inspector/resolve_camera.py`。
- [Khronos hello_xr](https://github.com/KhronosGroup/OpenXR-SDK-Source/tree/main/src/tests/hello_xr): OpenXR Minimum Example, with D 3D 12 Backend.
- [OpenXR Code: XrView](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html#XrView): Attitude and visual information for each view.

---

## 中文

# GearsVR：开发流程与随项目学习

2026-10-19 分辨率路线更新：**0.0.1 必须以 OpenXR 推荐每眼分辨率 × 启动器渲染倍率取代游戏分辨率驱动的 VR 渲染，优先级高于 DLSS。** 1.0 使用运行时推荐尺寸，玩家可降低倍率；游戏窗口分辨率不决定 VR 每眼尺寸。此项不是可选候选或 0.0.2 功能。当前高倍率实际图像仍有裁切，必须修复并验证资源重建后才能发布；配置成为默认不等于图像验收通过。最新证据见 [分辨率实现与验证](runtime-render-size-candidate.md)。下方旧日期状态保留作历史记录。

2026-10-18 当前状态请先看[主线进度与V1验收](39-mainline-progress-and-acceptance.md)。默认Native70/Borderless1及Feedback3开发包未提升；第三人称同帧双目和6DoF已有基础及此前真机反馈，Low/High已撤下。首发新增必做DLSS SR，隔离Native83已恢复实际双眼输出，但开关对照仍有取景异常，兼容验收未完成。机枪黑屏、手枪外圈、岔路UI和长期/真机稳定性仍开放，不能沿用旧百分比。DLSS与UI最新证据分别见[文档154](154-dlss-sr-startup-and-output-route.md)、[文档155](155-path-choice-position-and-docking.md)。

以下保留此前阶段的学习及实验记录，其中“当前”“最新”和百分比属于相应历史日期，不代表上述现状。

更新时间：2026-10-15。面向有一点前端和编程基础、不了解 UE 的开发者。

当前玩家测试方向已于2026-10-15调整：按用户要求撤下Low/High固定分辨率，回到游戏输出尺寸驱动的同帧SBS，并交付中文本机启动器。1920×1080游戏输出对应各眼960×1080。默认清单切换为GameResolution1 native＋MenuRecovery1 XR，保留启动菜单、DLSS/DLAA保护及原有HUD/准星修正；没有自动导航或测试倒计时。构建与离线检查通过，尚未启动这份新组合，等待用户自行实测。详见[当前构建与启动器](62-game-resolution-launcher.md)。

Low/High每眼2K/4K已从当前交付路线撤下，不继续把High列为首版门槛。V1仍需解决或验证闪烁/双眼持续更新、菜单与UI、跨关卡、清理、真头显6DoF/FOV体验及发布封装。此前约65%只是旧范围的工程估计，本轮不以缩减范围虚增完成度。

最新已完成的实际运行仍是此前固定目标Mirror2：1920×1080桌面显示左眼镜像，owned输出每眼2048²，111秒运行完成10,106次镜像绘制、5,052张XR复制。63个入口恢复并正常关闭，Owner保留到进程退出。源图采样发现右眼明显偏暗且后续变化很少，尚未证明它与用户左眼闪烁的因果关系。不能用此实验或旧头显“效果不错”的反馈替代GameResolution1验收。旧路线已实现的独立Canvas/Material及字幕分项像素证据保留在[显示记录](57-ui-position-and-subtitle-route.md)。

以下Sustain1、Menu1、Reload1及后续段落保留为历史实验记录，不能将其中“下一步”当作当前状态。Sustain1（提交`0a2c464`）详情见[独立双目渲染记录](61-owned-render-latency.md)：独立Low每眼2048×2048取消120帧实验上限，实际游戏＋模拟器连续90秒完成3106个owned ticket、3105次发布、3101次XR复制与8084次projection层提交。首帧和第1800帧GPU图像均见完整双眼场景；诊断详细历史保持120条。49项回归、7项兼容检查、59个Hook入口恢复及正常退出通过。资源尺寸往返重建、扩容后的右眼遮罩拒绝、Trial与GPU跟踪代次不一致已分别作修正与实测，100ms时效门限没有放宽。

Menu1 / PID39376复用同一冻结Sustain1构建，实际游戏＋模拟器运行240.91秒，完成8309个owned ticket、8307次发布、8304次XR复制和21669次projection层提交。通过只读菜单状态和定向窗口按键完成一次暂停→继续游戏；暂停场景`DeltaPause-Campaign-Host`与`LBtnResumeSingle`焦点均实际确认，恢复后的三次采样显示暂停场景退出且发布计数持续增长。121次运行状态采样owned_error均为0；59个入口恢复、正常退出通过。这次没有读取暂停菜单GPU像素，也没有进行关卡重载或真头显测试，不能当作菜单显示验收。证据为`analysis/ui-position-runtime/owned-menu1-39376.audit.json`。

Reload1 / PID34448完成一次实际检查点重载：Pawn地址和注册索引替换，同class/outer，Controller/Camera/Level保持不变；原生scene/family/renderer均接受重新关联并继续双目发布。299.99秒内9896个owned ticket、9894次发布、9891次XR复制、26745次projection层提交，最大提交间隔2133.89ms，不是全程无中断。59入口恢复和正常退出通过。未覆盖跨Level替换或重载后完整眼图；一次模拟器预览因尺寸1734×14不能用于视觉验证。证据`analysis/ui-position-runtime/owned-reload1-34448.audit.json`。

上述早期实验仅覆盖持续输出、一次暂停/恢复和同关卡内Pawn替换。其后已接通owned Canvas/Material及独立录制期绑定，当前状态以本文开头和文档57末尾为准。旧头显版本的“效果不错”不能直接用于新路径。

以下为历史实验记录，其中OwnedXr/OwnedCost阶段的XR零提交结论仅描述当时构建，已由上述后续证据更新。

OwnedXr1/2：新增默认关闭的`GEARS_NATIVE_OWNED_XR_PROBE`，最多120个独立scene ticket，禁止legacy Present抢占同一个publisher。独立CPU store保存实际消费时的完整冻结sample，两眼必须一致；消费owned proof后按已认证矩形、allocation generation和状态调用`capture_resource`，同DIRECT队列复制并恢复源状态，明确busy跳帧后才退休CPU ticket。Owner仍保留到进程退出，Canvas没有重定向。

实际PID46980与42588均完成120个独立ticket、119次共享纹理发布（第一帧无consumer而跳过）；**两轮XR projection提交均为0，新独立XR显示尚未通过**。Xr1在接收端读回ticket50/serial24的4096×2048双眼场景，pose序列与发布收据一致，每眼非黑4194304像素；只完成1张XR图像复制，提交前又因时间过期被拒绝。Xr2复制为0。接收端继续使用100ms门限，未提高门限、未把旧图改标为新pose。此结果证明资源共享与接收读回，不证明显示可用。

Xr2的120组同一monotonic时钟测量：pose接收到选定目标中位6.29ms；选定到consumer开始77.66ms（包括native构建与排队，尚未单独测出队列驻留）；consumer开始到左眼final draw62.09ms；双眼final间0.125ms；右眼draw到真实双眼Execute返回0.074ms；Execute返回到可复制边界0.0002ms；共享复制调用0.036ms。pose到发布中位147.10ms/p95 155.33ms，已在producer端超过门限。consumer返回/销毁略早于提交，等待销毁不是本轮瓶颈；上述CPU时间不等于GPU执行时间。下一步拆分native构建和排队、测量scene录制开销，不盲目改复制接口或扩展反编译范围。

两轮均通过39条实验回归；新增检查覆盖真实WARP共享像素/冻结ticket、双眼sample冲突、状态错配，以及owned模式拒绝legacy绑定。另一个关闭owned实验的构建通过6项兼容检查。58入口恢复和正常关闭通过；启动器仍因Owner保留而incomplete。证据`analysis/ui-position-runtime/owned-xr1-46980.audit.json`、`analysis/ui-position-runtime/owned-xr2-42588.audit.json`及`owned-xr2-42588-latency.json`；冻结清单`tools/vr-launcher/manifest-owned-xr1.json`、`manifest-owned-xr2.json`。默认玩家路径未切换为此实验，首版显示/性能验收未完成。

OwnedSequence3 / PID45772：独立 Low 目标首次完成有界连续16个scene ticket（2、4至32），同一4096×2048资源，首尾两次GPU读回均成功；每眼2048×2048、4194304个非黑像素，首尾全图8090295个像素变化。人工读取GPU图像可见两眼不同视角的人物、柱子、地面与远处门口，没有HUD。中间14帧只有提交/状态证据，不能当成逐帧像素检查或持续游玩验收。目标全程为RT(4)，未覆盖游戏侧状态转换。

本轮修正prebirth Present清除全部list历史的问题，保留真实Reset代次和已提交标记，并观察真实CreateCommandList/CreateCommandList1（SDK槽12/51）及88个创建对象的生命周期。混合unknown/duplicate批次即使关联失败，也在原始Execute返回后记录已知成员的真实提交；超容量/不可读成员使实验永久停止，不能经Present恢复。未增加DXGI白名单、未伪造Reset。32项SDK槽断言、17420项FrameLink检查、179项Trial检查及34条回归命令通过。此组合修正后连续实验通过，但不归因于所有历史头显黑屏。58入口独立恢复核验、正常退出通过；Owner与创建对象引用有意保留到进程退出，启动器仍如实incomplete。

证据：`analysis/ui-position-runtime/owned-sequence3-45772.audit.json`；冻结清单：`tools/vr-launcher/manifest-owned-sequence3.json`。下一步接入显式资源状态、同ticket姿态与所有权约束的XR发布，再处理Canvas/UI、High、持续游玩和正常退休。Canvas/XR目前仍使用旧目标，不能宣称玩家已经获得独立每眼2K/4K，首版整体进度估计暂不调整。

OwnedSequence1/2：已接入默认关闭、16个ticket上限的连续scene实验，使用同一Owner、逐ticket收据及首尾读回；179项Trial检查和32条回归命令通过。两轮实际游戏均只完成第一个owned ticket（首帧PPM保存），尚未完成连续独立输出。Sequence2 / PID53952的首次失证来自`dxgi.dll+0x20da2`在原始Present内部提交的DIRECT list：当前tracker无该list有效代次，device与owned目标相同，Reset实现与已装hook相同，故停止放行下一帧。此证据不表示该list实际访问了owned纹理，也不能归因于历史头显黑屏；下一步核对其在资源创建前的真实Execute返回/Reset及持有期，不按caller放白名单或伪造Reset。Sequence1的`state_certified`曾仅显示首帧缓存proof，Sequence2已改为当前认证状态，实际为0。两轮均独立核验56入口恢复、正常关闭游戏；Owner保留到退出，启动器仍如实incomplete。证据 `analysis/ui-position-runtime/owned-sequence2-53952.audit.json`，清单 `tools/vr-launcher/manifest-owned-sequence2.json`。Canvas/XR仍走旧目标，首版进度估计不变。

OwnedPixels5 / PID40520：首次从实际游戏独立 Low 目标读回完整4096×2048 GPU图像，两张2048×2048眼图均可见人物和场景，视角不同；每眼4194304个非黑像素，4186986个对应像素不同。实际CreateCommittedResource1创建状态为RENDER_TARGET(4)，单个ticket2的两个最终draw由同一DIRECT队列提交，renderer返回并销毁，读回fence完成，状态恢复为4，无未知操作或adapter故障。这是单个场景ticket的像素证据，不是持续独立分辨率、完整中间scene-color/depth尺寸验证或真头显验收。Canvas仍未重定向，XR仍走旧backbuffer路径；本轮旧路径有23次stale ticket，最大提交间隔267.3858ms，不能称稳定性通过。Owner有意保留到进程退出，启动器如实返回incomplete；56入口独立恢复核验通过，游戏正常关闭。证据 `analysis/ui-position-runtime/owned-pixels5-40520.audit.json`；默认关闭该实验，清单 `tools/vr-launcher/manifest-owned-pixels5.json`。

重要更正：早期owned-output观察器把ResourceBarrier接到了vtable槽24（实际为OMSetStencilRef），正确槽为26。OwnedPixels4将9次prebirth未知操作分类后发现此错误；这些记录不能作为游戏屏障错误的证据。OwnedPixels1–4及以下历史“13项GPU状态接线测试通过”不能证明真实ResourceBarrier转换，相关状态结论已被本段替代，原始记录保留。生产入口及两组集成检查已修正；新增30项SDK编译期槽位断言作为DLL构建依赖，修正版13项owned-output与8项scene-GPU模式及WARP读回通过。调试层不可用，断言与直接调用wrapper测试也不单独证明安装后的拦截覆盖。实际OwnedPixels5目标没有游戏侧transition，状态证明使用观测到的初始RT状态及无污染提交，不能声称它已运行验证游戏侧转换分支。

下一步：把已读回的单次场景输出推进为有界连续ticket的独立目标输出，保持真实Reset代次与跨帧GPU状态；再接Canvas/UI、XR发布、桌面镜像及完整退休。优先复用现有反编译资料库，仅对具体缺口定向分析。

连续跟踪准备：新增`consume_owned_output`边界，消费已提交帧时保留真实list代次、executed标记及资源状态，拒绝该资源的未提交记录，失证后不允许重新套用创建状态。17351项FrameLink检查含100帧连续状态及重复提交/消费、未来ticket、污染和未完成记录拒绝。WARP模式13通过实际两帧clear/barrier/读回，检查所有RGB像素；eye关联元数据为合成，不能替代游戏场景。模式14验证原始提交后早期失败时六份COM持有保留到进程退出。32条回归命令全部通过，结果在 `analysis/ui-position-runtime/owned-continuation1-tests.json`。该边界尚未接入游戏中的逐帧renderer租约及发布调度，现有实验仍只输出一个owned ticket。

OwnedScene1 / PID37624：首次将一个实际 stereo ticket 的 copied-family target 改为持久独立 adapter，并使用固定 Low 边界。ticket2 的两个 2048×2048 视图构造、消费及 renderer 销毁均通过；4096×2048 目标收到两个已核验的最终 draw，关联到同一个 DIRECT 队列的一次真实 Execute 返回，unknown draw 为0，adapter 故障为0。这里只证明原生路径与 D3D12 命令提交，尚未读回纹理像素或认证 GPU 完成。Canvas/字幕没有重定向，XR 继续原有 backbuffer 路线；同轮现有双目运行51.46秒、4625次projection、3802张新图，期间9次stale ticket、最大提交间隔111.481ms，不能称连续零中断。实验有意将 native Owner 保留到游戏退出，原启动器如实返回 incomplete；独立核验40个入口全部恢复，游戏已正常关闭。102项实验状态检查、299项Owner、16201项FrameLink及13项GPU状态模式通过。证据 `analysis/ui-position-runtime/owned-scene1-37624.audit.json`，默认构建关闭该实验，实验清单为 `tools/vr-launcher/manifest-owned-scene1.json`。

独立分辨率下一步：认证该资源的创建/实际提交状态并读回眼图，然后接入 Canvas、XR 发布与镜像、完整退休。不能用这次单帧提交替代持续每眼2K/4K画面或发布验收。

OwnedMetadata1 / PID49524：独立目标 adapter 已摆脱原 viewport 指针的长期持有。复用资料库并核对汇编后，完整校验显示元数据的八段运行时代码及线程查询导入；原生例程通过 adapter 的 gamma 方法读取当前设置。实际游戏 RHI 回调中，Low/High 目标均完成创建、宽高/surface 查询、13 字节显示参数对照及释放，无保留资源。299 项 CPU 检查、13 项 GPU 状态接线测试通过。现有双目同时运行 24.57 秒、2212 次 projection、1825 张新图，启动平面正常交接，40 入口恢复，游戏正常退出。场景/Canvas 尚未使用 adapter，`target_redirected=0`、`gpu_target_rendered=0`；这不是原生每眼 2K/4K 显示验收。证据：`analysis/ui-position-runtime/owned-metadata1-49524.audit.json`；实验清单 `tools/vr-launcher/manifest-owned-metadata1.json`。

同轮加入默认关闭的 owned-output legacy ResourceBarrier 跟踪及 Execute 前准备/后提交接线，WARP 实际提交和同步通过；真实游戏中仅验证安装、旁路及恢复，尚未 birth 独立输出。下一步为 copied-family/Canvas 目标接线、完整 GPU 状态与最终输出边界、排队持有者退休。不可将同步 metadata 探针的空退休条件直接用于场景/UI 队列。

GraphicsMenu1 / PID51304 已在真实AA列表选择事务中阻止DLSS缓存值；其他控件的index3仍可正常选择，各向异性过滤8x/16x应用与重开均通过，最终恢复4x。同一进程从启动菜单接续关卡双目29.10秒、2620次projection、2134张新图，正常停止、39入口恢复及原生DLSS关闭读回通过，游戏已正常关闭。预设批量赋值仍未覆盖，不能将普通选择路径等同于完整发布锁。状态导航遇到对象表变动时已通过新reader继续；控制器补充同PID/创建时间核验的定向恢复，57项离线菜单测试通过，自动恢复补丁尚未实际运行验证。详见[设置调用链](61-graphics-runtime-lock.md)。

GraphicsStartup2 / PID52996 已完成启动菜单保护→关卡双目交接：2813次启动平面后进入带ticket的projection，关卡运行167.36秒、15063次projection、12438张新图，正常停止并恢复38入口。此前Startup1 / PID50260在主菜单实际请求DLSS Quality，3次请求被阻止，取消后入口及原生缩放读回正常。Startup2本轮没有重新选择DLSS；菜单缓存标签、锁开启后其他设置回归仍待完成。启动前姿态自测曾返回33，已把新鲜样本发布移到Hook安装后，运行时过期保护不变；旧失败没有年龄日志，不能宣称根因已证明。详见[设置调用链](61-graphics-runtime-lock.md)。

自动操作优先使用游戏状态：现有菜单反射读取器获取scene、focus、输入阻塞及确认框文字，导航器在输入前后检查状态，关卡就绪由Pawn/Controller/Camera等对象确认。未知状态先查知识库和已有证据，仅补必要的定向逆向。截图保留给黑屏、UI位置和字幕实际像素等视觉验证；菜单状态、原生设置生效、GPU图像和真头显体验分别记录，避免把“控件显示正常”当成“VR画面正确”。

GraphicsLock1 / PID53840 已启用continuous阶段DLSS/DLAA运行时锁：实际菜单选择NVIDIA DLSS 3.5、NATIVE (DLAA)并确认，3次mode3请求被阻止，normal/alternate保持0，6685次collector/commit无策略故障。95.20秒、8566次projection、6320张新图，正常停止及38入口恢复通过。重开菜单仍显示缓存DLSS，菜单标签修正及关卡前保护仍未完成；不能宣称发布锁定已全部完成。详见[设置调用链](61-graphics-runtime-lock.md)。

字幕已有非空几何证据：PID53760/46520分别发布559/65次字幕文字调用，但现有原始截图未截到对白，不能宣布字幕显示通过。GraphicsLock1已保留实际索引对应的字幕source/root与GPU ticket、list generation及命令完成状态；CPU验证通过，PID53840本轮未命中字幕GPU样本。指定字幕ticket的采图仍待接入，普通快照event只请求下一张图，不能当作同帧证明。

最新 UiReticle1 / PID47092：居中准星已按同ticket原游戏方向投影到每眼，458次修正均成功。原始GPU像素确认两眼准星由图像中心移动至计算位置，白色图形边界保持60×104像素；10775次projection、8012张新图，正常停止及35入口恢复通过。使用实验清单 `tools/vr-launcher/manifest-ui-reticle1.json`，默认构建关闭该选项。当前解决非对称FOV方向偏移；近处真实命中点、字幕/更多UI及真头显体验仍未通过。此条取代下方历史记录中“准星尚未修改”的当前状态。

UiPixels3 / PID424：已支持有界按需原分辨率GPU快照，开镜与切枪两张截图的实际ticket/serial及保存时间均核验。换枪图标/数字和弹药HUD可见，两眼平移与HUD平面预测一致；准星仍在各眼图像中心，非对称FOV方向偏移尚未修正。5667次projection、4137张新图，正常停止及35入口恢复通过。Material观察器本轮未命中，不能据此宣布该路线已验收；字幕亦未覆盖。详见[UI定位进展](57-ui-position-and-subtitle-route.md)。独立目标持有/adapter与tracker出生契约已加入CPU测试，尚未绑定到游戏场景；每眼2K/4K仍属待实现。

NativeSurface1 / PID34052：首次实际调用游戏自身的颜色目标分配器，在已核验RHI线程/TLS内依次创建4096×2048和8192×4096目标。两档真实GetDesc均为RGBA8/DXGI28、单mip/单slice/单sample、ALLOW_RENDER_TARGET，并通过同线程native引用清空释放；首次ticket即成功，无保留引用或cleanup不确定。独立目标尚未绑定绘制，因此这不是游戏原生2K/4K显示测试。原双目同时运行32.268秒、2907次projection、2416次复制，最大提交间隔29.5934ms，34入口恢复；模拟器及游戏正常退出。代码54d54b6；证据 `analysis/ui-position-runtime/native-surface-live-34052.json`，自测164项、宏关闭无效果和continuous自测通过。

c5f7262已为双眼源构造加入显式Low/High边界，保持原source/family不变并通过CPU回归，尚未在运行时选择该参数。e80aad0贯通玩家指定游戏安装路径的后端及身份检查，仍未完成便携打包。下一步是持有独立native目标、接入copied-family/Canvas目标，并以实际GPU状态和队列顺序发布；新tracker必须先配置再接收列表，不能清空旧tracker计数伪造首次启用。原生最后释放仍须在合法RHI线程，并等相关renderers/UI/GPU持有退休。

首版范围与进度已重新对齐：第三人称同帧双目、可靠6DoF、启动/菜单、整套UI、每眼2K/4K独立渲染、DLSS/DLAA运行时锁定、稳定性及玩家部署，工程估计约45–50%，具体权重/缺口见[部署目标](16-deployment-target.md)。这不是发布验收通过率。UI脚本本轮批量导出2709个候选条目，1093个仅为native声明；另外确认Material UI独立消费路线，以及世界标记不能直接套用普通HUD平面。详见[UI覆盖审计](57-ui-position-and-subtitle-route.md)。本轮未启动游戏或新增视觉验收。

TargetSize2 / PID29184：新诊断版启动、220.366582秒双目和正常停止通过，19830次projection、15033次图像复制，最大提交间隔91.1762ms，34个入口还原。首次4个有效family回调均捕获同一实际target，40个方法前缀及4份vtable字节全部与历史运行代码一致；不是从已过期栈地址猜测目标。该轮启动guard一次成功，不能据此证明PID39208是跨帧竞争。3d88101已增加启动阶段的明确失败分类，仅对其余身份/读取检查都通过的时间戳变化有限重读（最多5次、200ms），渲染回调仍单次检查。证据：`analysis/ui-position-runtime/native-target-size2-binding.json`；游戏已正常关闭。

独立目标的具体接口已定位：实际target vtable RVA1706880，+8返回surface引用地址，+10/+18读取宽高，+20/+28为显示元数据。原target是0x510字节viewport对象的+8子接口，原析构会释放整个viewport，不能复制到小型adapter。世界普通最终输出和postprocess会使用family中的target；nativeFCanvas的target在+8，48字节queued command的target在+20，两者不是同一布局。native surface强引用及COM纹理提取路径已查明，但完整接口边界、创建参数和输出资源最后的实际GPU状态仍须核对；当前仍未改变游戏原生渲染分辨率。

最新（独立分辨率准备）：37d9047 已改成左右眼独立 XR swapchain 并检查 runtime 上限。游戏 PID46572 在原始每眼1920×2160下运行99.279秒，8938次提交、最大间隔20.622ms，34入口恢复及退出通过。独立测试图 Low4096×2048→两张2048×2048 已通过实际模拟器链路29.955秒、最大间隔15.2904ms；High8192×4096→两张4096×4096也通过29.976秒、最大间隔12.2045ms，生产端和XR均正常退出。GPU像素测试同时覆盖Low/High、复制/BGRA/sRGB共300采样点无差异。游戏的独立场景目标仍未实现，不能把测试图称为游戏2K/4K。目标定义见部署文档16。

67afdba 增加固定自有纹理的GPU发布接口，不再要求来源必须是桌面backbuffer。独立GPU测试覆盖COMMON/RENDER_TARGET/PIXEL_SHADER_RESOURCE三种明确状态、左右像素读回、身份/代次/尺寸拒绝、占用和停止；原swapchain路径测试通过。引擎资源持有、实际状态与队列顺序仍须接入证明。3692761保存Low/High不可变档位规则，尚未向当前启动器开放未实现的分辨率选项。

TargetSize1 / PID39208诊断启动未通过：Python已生成当前反射配置，但native在load_anchor_config检查拒绝，场景hook未激活，XR只显示启动平面。预留hook撤销且入口还原核验通过；这不是双目场景测试。当前记录未区分身份不一致与跨帧相机时间变化，不能宣布根因。正在补充精确失败分类与仅限启动的有限跨帧重读，再采集实际渲染目标接口。

UiHudPlane1已首次实际修改HUD viewport，但可见位置尚未验明：791.65秒双目、2984次平面校正draw通过；截图使用1734×257模拟器预览再缩成1280×189，不能据此判断原始眼图HUD消失。准星、真实字幕和最终UI验收仍未完成。后续保留以下历史记录，不将旧“UI坐标尚未修改”描述当作最新状态。

2026-10-14 UiObserver7 / PID37184：索引成员与同ticket双眼矩阵已实测关联，HUD文字也已接入。2650个消费者、索引序列及双眼overlay全部完成，无拒绝；1112段文字共13344个顶点有完整范围，当前样本不再缺失文字候选。稳定双目206.034605秒、最大提交间隔17.8515ms，34入口恢复。字幕入口运行14497次但均无实际glyph，仍待对白场景；UI坐标尚未修改。前一轮UiObserver6受用户FancyZones调整窗口影响，3840×2160变为3842×2005后固定尺寸保护拒绝新缓冲，属于显示失败，不归因于Win+0。窗口缩放恢复列为发布缺口。代码检查点341a800、6aaff64，详见[UI定位进展](57-ui-position-and-subtitle-route.md)。

2026-10-14 UiObserver5 / PID45216：已将准星、换枪UI、普通HUD的Tile顶点候选关联到实际双眼绘制ticket。5840次追加观察、1693个消费者内容匹配均无拒绝；约139.80秒双目，最大提交间隔18.10ms，开镜/换枪两眼截图正常，33入口恢复通过。修正了观察器插入调用帧造成的路由拒绝和44/48字节步长误判；本轮使用独立模拟器焦点测试选项，原部署/runtime未替换。UI坐标尚未修改，下一步是draw索引归属及逐眼位置修正，文字/字幕仍需单独接入。详见[UI定位进展](57-ui-position-and-subtitle-route.md)。

2026-10-14 UI后续：PID44988已实际自动开镜、切枪并采图，确认UI沿同一画布布局重复到两眼；不对称FOV下，两眼图像中心并非同一空间方向。UiObserver2 / PID36892修复了嵌套脚本绕过共享VM导致的漏分类，7894次Canvas语义分类、拒绝0，准星/换枪/其他HUD均已区分。32入口恢复通过；双眼锚点投影数学已通过CPU测试，但尚未修改实际绘制坐标。下一项是语义顶点到渲染批次及对应ticket的关联。详见[UI定位进展](57-ui-position-and-subtitle-route.md)。

2026-10-14 UiObserver1 / PID43652：语义 UI 观察器已实际接入，静止双目运行230.21秒、最大提交间隔22.45ms，32入口恢复通过。WarHUD.PostRender接受16874次；由于Windows拒绝前台激活，未执行开镜/切枪，目标scope和Canvas调用均为0，没有取得坐标样本，UI位置尚未修复。字幕已证实走PostRender返回后的独立原生文字路线，不能以当前HUD观察器替代字幕验证。详见[UI定位进展](57-ui-position-and-subtitle-route.md)。

当前目标为**第一版发布支持6DoF头部追踪的第三人称同帧双目 Mod**，继续无人值守实现与模拟器诊断，不使用AER。真头显联调按用户指示开始，由用户说测试结束再停止。第一人称、持枪与其他深度交互作为后续版本目标，不作为首版发布前置条件。

2026-10-14 StartupMenu1 / PID34900 已跑通启动即显示VR平面菜单、进入关卡后同一XR会话接续同帧双目。菜单停留超过180秒仍可见，菜单与关卡双眼实时截图均保留；立体输出252.746474秒、复制18008张眼图、最大提交间隔18.5689ms，正常收尾及入口恢复通过。使用实验清单 `tools/vr-launcher/manifest-startup-menu1.json`，默认清单未升级。真头显菜单体验、返回主菜单后再进关卡仍未覆盖。

UI首版要求进一步明确：用户描述为**位置偏离瞄准点、图形形状正常**。静态脚本与只读运行时反射定位了准星、换枪布局、脚本分发和Canvas绘制入口，支持原画布坐标与头显视角不一致的假设；尚未用实际绘制坐标建立关联或实施修复。字幕enabled/forced-off运行时标志正常，需要对白与绘制路径证据。下一步在精确UI函数范围内观察坐标，分别处理准星瞄准投影和普通HUD布局。

2026-10-14 真头显反馈（Recovery2 / VDXR / PID10920）：用户反馈整体效果不错，提出疑似3DoF、人物距镜头偏远、开镜准星和换枪UI偏斜、尚未看到字幕，并要求启动游戏即接入VR以便在头显中操作菜单。实际XR运行889.022952秒，提交106572帧、复制81834张眼图，最大提交间隔125.2073毫秒；四个原生阶段均82967对且矩阵错配0，事件容量溢出0。按用户测试结束指令正常stop，XR退出0、28入口还原通过，游戏正常关闭。该轮覆盖真实游玩观察，不表示所有HUD或菜单路径通过。证据：`analysis/runtime/continuous-stereo-10920-20261014-145200-9440ece3.json`。

位置跟踪并非在代码中关闭：采样位置跨度约X0.518米/Y0.293米/Z0.721米，双眼矩阵包含相对回中原点的位置变化。当前仍使用原游戏第三人称镜头作为基准，未启用角色眼位原点。用户对位置感和距离的反馈保留为体验问题，不用矩阵校验替代体验验证，也不因此直接切换第一人称。

用户追加首版6DoF目标：在现有位置与旋转链路上，确认头部左右、前后、上下移动造成正确方向及比例的视点平移和近处视差，并保留正常转头与回中。优先检查坐标变换和世界比例，修复实际发现的问题；下一次真机只需自然侧身、前倾和轻微上下移动，不扩展成繁重验收。此处指头部6DoF，不新增运动控制器或房间尺度角色移动承诺。当前代码接入与旧日志不能单独证明该体验已通过。

首版剩余主线为：修正ADS准星与换枪UI，并在有对白场景确认字幕；补充启动菜单的真机体验及必要生命周期验证；锁定DLSS关闭；整理可复制部署包与安装/卸载说明。菜单、HUD和准星须分别验证，不以暂停菜单通过推断所有UI正确。DLSS已增加启动强制关闭和可恢复备份，PID34900两处生效字段均为0；运行时禁止菜单重新启用尚未实现，不能宣称已锁死。镜头距离/世界比例可调作为体验优化，保留当前可用基线。

2026-10-14 暂停输出修复（PauseUi1 / PID14128）：通过运行时调用栈定位暂停最终quad返回点d808d8，当前函数字节与历史运行快照相等，quad参数装载与原d83b1c路径逐字节相同。新版保留几何/ticket检查并识别该路径，针对已验证Canvas调用链将整屏UI分别绘入两眼，显式验证overlay关联，继续拒绝未知写入。自动启动、进入关卡、暂停10秒、恢复、正常停止完成；暂停发布724→1484，模拟器截图两眼菜单完整可读。约60秒XR最大间隔19.7603ms/stale0，四阶段4430对/0错配、overlay25974组/0拒绝、28入口恢复通过。发布4423/复制4422尾差1如实保留，不宣称严格等数通过。模拟器预览manifest已切到该冻结构建，旧SourceMap2保留；重复切换、主菜单/重载与真机新构建仍未完成。证据：`analysis/runtime/pause-ui1-14128.audit.json`。

2026-10-14 暂停生命周期发现明确缺口（h / PID43408）：Start开关菜单的内存状态与恢复操作通过，但暂停时模拟器双眼截图全黑，发布停在590，恢复后重新增长，XR最大提交间隔15.644秒。暂停窗口内仍构建新的双眼ticket，anchor4541/0，四阶段4541对/mismatch0；不能归因为游戏时间戳停止或仅仅XR过期门限。缺图窗口256条现有outage样本显示新built持续、最终左右眼draw缺失，整体另有312次unknown-write污染拒绝。整体显示验收失败，正常退出/28入口恢复分项通过。正在定向定位暂停输出路径，不放宽过期或未知写入保护。`sim-source-map2h-43408.audit.json`及同前缀原始记录为证据。只读UI反射另确认GameUISceneClient引用DefaultUIPostProcess中的DOFAndBloomEffect，此为线索，尚非调用因果证明。

2026-10-14 最新结果（g / PID39464）：**非空source map真实双目覆盖通过，此缺口关闭。** 正式模拟器入口启动后，正常手柄认领、相机正扫109.369°再反扫停76.393°，没有改隐藏列表或游戏参数。原生非空候选4055、接受4055、拒绝0、最大13节点、六项map检查全部通过；四阶段各4944对且矩阵mismatch0。约60秒内GPU发布4930、XR复制4930、projection5414，正常停止、28入口恢复、XR/入口/driver/相机helper真实exit0。证据前缀：`analysis/runtime/continuous-stereo-39464-20261014-115843-3f9a9a76`；动作：`analysis/runtime/sim-source-map2g-camera.json`。这验证了先前会触发非空保护的真实条件可以持续渲染，不证明旧44892指针有效或所有历史黑屏已解决。停止深挖该集合，后续回到菜单/关卡生命周期、清晰度与真机当前构建范围。

2026-10-14 自然非空条件已找到：PID34688 一次正向相机转动在 yaw 约69–88°期间，9条一致样本读到 HiddenActors 包含 Pawn_COGMinh，每条有13个合格组件，前后回到空态。自身角色到镜头距离仍约167，因此是另一角色进入隐藏条件，并非已经证明自身碰墙缩短镜头。全程没有写隐藏列表或参数。这给出了下一轮双目运行的正常操作复现路径；单目观察尚不证明实际source map复制成功。证据：`analysis/runtime/camera-positive-orbit-34688.audit.json`。

2026-10-14 输入路由后续验证：PID34688 的虚拟手柄已连接，但原生设备条目玩家分配值为 -1；19ab10 的按钮分配分支要求先按按钮，纯摇杆不会认领。一次正常 Back 短按使分配变为 0，随后唯一一次右摇杆使 Controller 与 POV yaw 均改变 20.484°，输入已回中。没有写游戏内存；驻留 UI 栈未改变，LastPlayerIndex 从 -1 变为 0 是预期认领。此前捕获标志假设已排除（原生 Viewport +4ec 实值为 1）。这解释自动镜头测试为何此前无动作，不等于黑屏根因。下一步用正常相机变化触发 HiddenActors 非空，再核对 SourceMap2 的真实复制与持续输出。证据：`analysis/runtime/input-assignment-34688-once.json`、`input-assignment-34688-right-continuation.json` 与 `analysis/ida/input-routing-20261014/`。

2026-10-14 后续定向逆向已确认 source+0x140 的一个正常生产路径：原生视图构建 15cf08 遍历 PlayerController.HiddenActors，再从 Actor.AllComponents 中收集合格 PrimitiveComponent；bIgnoreHiddenActorsMembership 位会排除组件。相机函数110878根据WarPawn.CameraNoRenderCylinder及其他资格条件增删HiddenActors，因此非空不应一概作为非法输入。当前PID34688反射与10段运行代码/快照字节相符，HiddenActors为空；自身角色水平入界半径40，当前视点距角色约167，解释了静止试验未覆盖该分支。旧44892没有保存完整集合，仍不能证明当时头指针有效；SourceMap2非空实测也尚未完成。一次正常摇杆命令未改变POV，正在查输入路由；唯一640像素画面确认正常关卡，驻留PenderCrossplayInvitePopUp不等于可见阻塞弹窗。下一步先证明输入确实改变相机，再通过正常相机靠近角色/碰墙触发隐藏集合，并核对非空计数、复制独立性与GPU连续输出。证据：`analysis/ida/source-map-producer-20261014/README.md`、`analysis/runtime/source-map-producer-34688.json`。

最新进展（SessionExit1 / sim-source-map2d）：已定位上轮退出失败为应用只等待 STOPPING，但安装中的模拟器在 PID3852 日志中实际发出 EXITING。新 XR 构建分别记录 EndSession 成功与终态 EXITING 后销毁；不伪造标准握手。只有已销毁对象且 GPU 收尾安全才通过，超时/丢失仍失败。5 秒无游戏注入预检 exit0，7 项 C++/ABI 自测、runner 28 项测试通过。后续 PID38688 自动启动并运行 120.003 秒，原生四阶段各 8816 对、mismatch=0；发布 8808、复制 8807、XR projection10810、最大提交间隔24.3774毫秒、stale0；owner/XR/stop均exit0、28入口恢复。模拟器MCP六字段实际读回及XR CSV独立匹配 yaw±15°、pitch8°、roll8°、XYZ变化；不把queued回复当已应用。可选右摇杆虽返回成功，但telemetry仅时间推进、位置/旋转不变，未证明实际转镜头。非空源map仍未覆盖，图像票据保留也不完整。证据：`analysis/openxr/sim-exit1-preflight.json`、`analysis/runtime/sim-source-map2d-38688.audit.json`。

启动交付新增 `tools/vr-launcher/SimulatorVR.ps1` 与独立 `manifest-simulator-preview.json`，自动菜单进入正规启动器 `--auto-continue`，共用原120秒期限与独立玩家就绪判断；`--runtime`仅影响当前进程及子进程。原manifest与受保护交互设计文件保持原哈希。新入口 PID34688 独立实测通过：正规脚本自动五步菜单用时21.469秒；随后观察60.003秒，发布=复制4622、projection5415、最大提交间隔14.2133毫秒；四阶段各4630对且mismatch=0，owner/XR/stop均exit0、28入口恢复。全程无截图导航、无额外游戏或姿态输入。核心C++测试7项、runner测试28项、正规launcher/navigation测试31项、菜单测试40项均通过。证据：`analysis/runtime/continuous-stereo-34688-20261014-105921-a1581368.*`、`sim-source-map2e-single-run-control.json`。目标仍未完成：旧source+0x140条件覆盖、真实头显当前构建及更完整生命周期范围尚待验证；此入口是已验证当前关卡的模拟器预览，不是所有游戏状态的兼容性声明。

2026-10-14 菜单自动化已完成首次实机验证：新增 `tools/game-io/menu_state.py`（只读反射场景/焦点/弹窗文字）和 `menu_control.py`（按精确状态发送窗口 Enter）。PID16336 先以一次确认关闭已识别的英文 Online Service Error，再在 5.313 秒内自动完成 Campaign→SinglePlayer→Continue，导航过程没有截图。随后独立校验玩家占有回链、WarPawn 类型、Level 归属和推进的摄像机时间，确认进入关卡；这轮没有注入 VR。读取器缓存后普通菜单约 1 毫秒、弹窗约 1.6 毫秒；35 项离线测试通过。标题页存在 AttractLauncher 无输入覆盖层，使用专用场景栈匹配；未知弹窗或布局不自动确认。证据：`analysis/runtime/ui-control-16336-online.jsonl`、`ui-control-16336-to-level.jsonl`、`ui-control-16336-level-ready.json`。下一步使用该脚本做 fresh 启动验证，再运行 SourceMap2 高分辨率双目；自动进关卡成功不等于 4K 黑屏已经修复。

后续 fresh 全链路 PID33964（sim-source-map2c）：菜单初始化约 13 秒后，按状态完成标题、已识别在线错误、Campaign、SinglePlayer、Continue 共五次确认，控制器总用时 21.297 秒；原启动器独立确认玩家就绪。加入初始化等待后离线测试共 40 项通过。SourceMap2 在用户指定 OpenXR Simulator Runtime 内完成 180.0067 秒观察；一张 MCP 实时截图确认两眼均有游戏场景。原生四阶段各 13320 对且矩阵 mismatch=0，源矩形 3840×2160，发布 13315、复制 13314，XR 投影 16211 次、最大提交间隔 19.9289 毫秒。原生 28 入口恢复成功，但 XR exit=1、shutdown_completed=false，整轮保留失败；destroy 调用返回成功不等于正常 session 退出流程已经通过。非空 map 分支 candidates=0/accepted=0，因此本轮未验证修复在原故障条件下有效；图像票据记录也有容量丢项，不能断言完整无缺帧。模拟器的 source=0 仅代表 runtime pose，仍不是物理头显验证。下一项调查停止状态机，并用受控场景/姿态变化覆盖非空 map 条件；不改生产 manifest。证据：`analysis/runtime/sim-source-map2c-33964.menu-control.jsonl`、`sim-source-map2c-33964-mcp-live.json`、`continuous-stereo-33964-20261014-103320-e635db57.*`。

2026-10-14 后续运行切换：VDXR7 在 xrGetSystem 返回 FORM_FACTOR_UNAVAILABLE 时停止于预检，未激活新的渲染修复；prime 六入口独立恢复。用户随后明确改用 `OpenXR-Simulator-v1.5.0-win64`。其安装 manifest 的裸相对 DLL 路径在当前加载环境下失败，已另建 `analysis/runtime/sim-absolute-runtime1.json`，只将 DLL 路径写为绝对路径并用于子进程环境。原安装清单和全局 runtime 未改；264 帧 pose-only 预检有效、正常退出 0。这属于模拟姿态验证。SourceMap2 补充构造后源集合所有权复核，源改持有眼图集合存储或读取失败时禁止释放、进入隔离；14 项本地检查通过，待游戏验证。证据：analysis/runtime/source-map2-native-ready.json、analysis/openxr/sim-absolute-runtime1-preflight.*。

VDXR6 PID 44892：只将 MotionBlur 改为 False 后，4K 源图开始发布，实际每眼 1920×2160，发布 17、接收 16、projection 43，覆盖仅 0.3504181 秒。随后 source+0x140 非空触发旧 split 输入保护（mask=32），主动停止双目；28 入口恢复及 XR 正常收尾均通过。用户看到桌面短暂双视图、头显始终黑屏后退出 VR，因此本轮显示验收失败，提交帧数不能替代显示证明。证据：analysis/runtime/vdxr6-44892.audit.json。

针对这次保护停止，定向反编译及 PID 44892 的五段代码逐字节比对确认：原生视图构造函数已经逐项复制 +0x140 集合，分配独立节点与桶；析构只释放这些存储，payload 为不解引用的借用值。新增有界读取、链与桶覆盖检查、两眼/源存储独立性、内容一致及源未变检查；保留其他输入保护。SourceMap1 构建及 14 项 CPU/ABI 检查通过，涵盖正常复制、坏链、别名隔离和构造期间源变化。证据：analysis/runtime/source-map1-native-ready.json、analysis/runtime/collection-code-44892-20261014/bytecheck.json。尚待新游戏进程确认真实非空集合及持续头显输出；未更新生产启动清单。

## 我们要交付什么

第一版目标：Gears of War: Reloaded 的单人 PCVR Mod，具有真正左右眼视差，以及头部旋转、平移的 6DoF。先保留原有键鼠或手柄移动、瞄准；独立 VR 手部武器操作属于后续里程碑。

当前先以第三人称 VR 为阶段性验收目标：同帧双眼、6DoF、回中、持续输出与可恢复的启动/停止，以及真实头显的双目显示和尺度验证。第一人称眼点调整保留为后续实验，不作为第三人称阶段的前置条件。对用户报告的约 75% 是工程估计；真实头显整链路、关卡与断连恢复、历史输出缺口和退出异常仍是实质缺口，不能由模拟器测试或 CPU 自测替代。

2026-10-14 用户补充：FOV、菜单及关卡自动切换属于 VR Mod 的必要条件。阶段验收增加：启动后头显可见稳定的菜单面板；识别有效关卡玩家后自动切到同帧双目场景；暂停、加载、重载期间保持 XR 会话并正确呈现界面，恢复后重新验证视图归属并继续双目。现有每眼 OpenXR FOV 已进入投影计算，但头显边缘裁切、HUD 可读性及特殊镜头尚未完整验收；现有入口仍在进入关卡后才启动 XR/双目，完整菜单流程尚未实现。菜单面板显示不计作两个独立视角的世界渲染。

真实 VDXR 首轮 PID 40060：用户确认“左右眼都看到了游戏场景”，并反馈初步体验还可以。实际 runtime_name=VirtualDesktopXR，366 对四阶段矩阵匹配，357 张游戏图像接收及 874 次 projection 提交，覆盖 7.274807 秒；随后 split source 输入检查拒绝（checks=0/rejection=1）触发 proof_failure，未完成目标 120 秒。视图归属检查 matched=367/rejected=0；XR 正常退出、28 入口恢复，游戏留在关卡。启动 anchor 的缓冲区差异不是故障：binding changed=0/rejected=0，没有 first_failure。具体失败字段待下一轮定向记录；用户确认两眼有图不替代深度、尺度、头动及长期稳定验收。证据：analysis/runtime/vdxr1-40060.audit.json。

真实 VDXR 第二轮 PID 40696：完成 120 秒观察，用户反馈“试了下没什么问题”，但头显内画面偏糊，希望提高分辨率。四阶段各 6545 对、矩阵残差为零，发布与接收均 6276；归属 matched=6546/rejected=0，split 输入 checks=63/rejection=0，旧输入故障未复现。XR 正常退出、28 入口恢复。仍记录到 4192.432 ms 输出间隔：保留窗口中场景构建与左右眼绘制继续，但 frame-link polluted（128）阻止发布，随后恢复；污染来源尚待定位，不放宽旧图门限。接收及缺图诊断均有容量丢项，不能声称轨迹完整。证据：analysis/runtime/vdxr2-40696.audit.json。当前 SBS 总幅 1920×1080、每眼 960×1080；下一轮由用户在游戏菜单选择更高分辨率，优先 3840×2160，实际源图与 XR 尺寸仍须运行记录确认。用户配合以正常游玩和简短体验反馈为主。

高分辨率初测：用户在游戏菜单选择 3840×2160，并明确当前是 DLAA。INI 的 DLSS=True 是功能开关，AADefaultQualityMode=4 对应 AAOnly；不能据此误记为 DLSS 超分辨率。VDXR3 PID 41952 在菜单等待期间超时，未开启双目；随后 prime watchdog 恢复，不属于高分辨率渲染失败。保留设置重启的 VDXR4 PID 440 已进入关卡并开启捕获，但首图前游戏自行闪退（用户确认），published/copied/projection 均为零，没有实际双眼尺寸证据。XR 收尾已完成；游戏异常导致 native 恢复未确认。离线 dump 的异常线程位于 ntdll/PlayFabMultiplayerWin 的进程退出清理链，尚不能解释最初为何退出，也不能归因 DLAA。证据：analysis/runtime/vdxr4-440.audit.json、analysis/runtime/game-crash-440/offline-unwind.json。下一项保持用户 4K＋DLAA 设置，启动不注入 VR Mod 的基线对照。

后续用户截图及 INI 显示设置已改为 3840×2160、抗锯齿 None、Native、DLSS=False，纹理 High、动态模糊 On。无 VR 注入基线 PID 46028 在关卡内存活 60.0118366 秒，无新增 WER；初始及关卡模块清单均无 GearsCapture。初始/结束配置变化已保留，因此该结果只支持当前 4K Native 设置的短时基线，不证明 4K＋DLAA 稳定，也不是上一轮故障的单变量对照。下一轮以用户最新配置验证 4K Native 双目，不自动更改其他画质项。证据：analysis/runtime/vdxr-baseline1.json。

VDXR5 PID 39356：4K Native 高画质下用户报告头显黑屏，published/copied/projection 均为零；四阶段各 7367 对、每眼目标 1920×2160、矩阵残差为零，提交输入 checks=63、归属 matched=7426/rejected=0。正常停止并核验 28 入口恢复，游戏保留普通画面。直接线索为输出路径变化：原始 gpu_output_quad 每眼来自 d83cbf，真正 backbuffer Draw 的保留调用栈来自 2b11a0→2b0bdb→11a6f9；现有 final_eye 只识别 d83b1c，因此所有 backbuffer Draw 被记为 unknown，捕获尝试数为零。不能简单新增白名单而不验证矩形及采样归属。capture.cpp 中 observe_output_stack 的 1920×1080 条件仅过滤诊断日志，不控制发布，不能作为根因。用户自行退出占用内存的 MetaXRSim 后游戏不卡顿但仍黑屏；退出后读取剩余物理内存约 17.5 GiB、显存约 23.4 GiB，未记录退出前峰值，资源竞争与发布失败分别调查。下一项备份配置、保持 4K 及其他画质，只将 MotionBlur 从 True 改 False 对照；较早 1080 配置也关闭该项，但它不是 PID 40696 的精确冻结配置。证据：analysis/runtime/vdxr5-39356.audit.json；定向静态反编译保存在 analysis/ida/vdxr5-output-route/，仅作为运行调用栈的补充，保留类型和重定位推断局限。

6DoF 的六个自由度：左右转头（Yaw）、上下看（Pitch）、歪头（Roll），加上左右、上下、前后移动。身体坐着不动、只把头向旁边移动时，应该能从不同位置看到物体侧面；这与只允许转头的 3DoF 有区别。

6DoF 跟踪与立体渲染是两个独立条件。把同一张游戏图片给两眼只能作为显示调试，不算完成双目立体。相机定位也不等于已控制相机，更不等于已经有 VR。

这是一份按证据推进的研究路线。同帧双视图、原生投影响应、真实每眼 pose/FOV 驱动桌面双眼场景，以及 Meta XR Simulator 中 120 秒游戏 GPU 图像投影链路已在当前关卡通过受限验证。PID 36212 完成完整六轴模拟和两次非零姿态回中，独立矩阵审计与正常退出均通过。随后 PID 41704 出现 1431.7773 ms 输出间隔及独立的退出清理错误，整轮失败。退出竞态修正后，诊断版 PID 1756 的 60.1401 秒复测最大间隔 18.0283 ms，回中和自然退出通过；未重现旧缺图不等于已定位根因，且停止尾部发布/接收 3386/3385 的严格等数仍为 false。持续模式的 180 秒输出与联合停止已通过分项核验，历史 PID 43724 严格发布/接收等数检查仍为 false；更长时间、关卡生命周期、稳定第一人称与真实头显显示尚待验收。

输入链路补充（PID34688）：实际XInputGetState四槽轮询仅slot0连接，LocalPlayer.ControllerId=0；bIgnoreMoveInput/bIgnoreLookInput两个Byte计数均0。随后单次一秒右摇杆同步采样60次，OS右轴30次为18022且结束回中；游戏PlayerInput四float持续0、Controller/POV角度和位置均不变，camera时间正常推进。支持“OS实际收到，但未观察到游戏侧响应”，30Hz采样不排除短暂轴值，尚未定位内部过滤点。此轮不算角色移动或非空集合覆盖。下一项需检查游戏输入后端/路由，不能盲目重复摇杆。证据：`analysis/runtime/input-sync-34688-once.json`及`.samples.csv`。

## 已经做了什么

- [x] 确认实际游戏路径、EXE SHA256、x64 架构。
- [x] 解压分析副本，读取 Engine / WarfareGame 的对象表及部分脚本。
- [x] 找到 WarPlayerCamera、CameraCache.POV 和原生计算入口候选。
- [x] 通过静止与镜头输入验证当前摄像机缓存。
- [x] 核对摄像机属于 WarPC / WarLocalPlayer / Marcus 的本地玩家关系。
- [x] 写出不依赖旧堆地址的自动定位工具，并验证错误游戏版本会被拒绝。
- [x] 跨重启验证自动定位：PID 35088 → 32532，POV 地址改变，独立解析成功。
- [x] 同一进程内重载检查点后再次验证自动定位：PID 32532 不变，独立解析及玩家关系核验通过。
- [x] 动态采集原生场景视图构建调用、当前玩家身份、矩阵及 DXGI Present；停止后核验游戏函数入口恢复。
- [x] 用户观察确认受控 yaw 摆动影响画面；六轴微扰日志确认三轴旋转/平移进入原生视图，恢复与基线一致。
- [x] 独立 OpenXR 会话、左右眼测试图 GPU 回读和提交；坐标转换及位姿有效性检查通过离线测试。
- [x] Quest 3 / VDXR 实时旋转和平移输入、测试图头显端连续显示：采样与用户观察共同验证。
- [x] 真实 Quest 3 位姿进入当前关卡的原生临时视点：受限三轴旋转/平移、矩阵变化及停止后的入口恢复已验证。
- [x] 游戏窗口图像通过 OpenXR 二维面板显示到 Quest 3，GPU 像素回读和用户观察确认通过；此项不算双目立体。
- [ ] 游戏图像提交、头部跟踪接入游戏、双眼立体及完整 6DoF。

当前阶段：已打通真实游戏同帧双眼 → 带 frozen pose/FOV 的 GPU 共享图像 → Meta XR Simulator projection layer。最新 PID 36212 attempt1 的 120 秒测试完成偏航 ±15°、俯仰/侧倾 ±10°，XYZ 分别 ±0.06/0.04/0.05 m，并在转头、抬头期间各回中一次。四阶段各 6,096 对，发布并接收 6,090 张新游戏图像，提交 8,639 帧 projection，覆盖 119.968899 秒、最大间隔 17.2533 ms。12,192 条消费/准备左眼矩阵与各自参考代际独立重建匹配，最大误差 0.00395 native units；两眼四阶段比较无残差。source/raw flags、GPU drain/close、普通 mono、28 入口恢复及 native/XR exit0 均通过。新图约 50.75 Hz，projection 约 72 Hz 且包含复用，不能混称新场景帧率。当前截图仍为第三人称基准视点，尚未完成稳定第一人称适配。

官方 Operator 独立试验观察到 override 期间 raw head3/view15，回到相同数值并不恢复 tracked；调用 release 后才恢复 head15/view15。默认 source0 仍严格要求 head15/view15。PID 40776 已完成显式 source1 动态游戏 projection：yaw 跨度约 30°、x 跨度 0.12 m，四阶段各 1,601 对、GPU 新图 1,591 张、projection 2,158 帧/29.95637 秒；release、入口恢复及独立矩阵审计通过。此前 PID 38112 因实时 CSV 共享锁失败，未执行动作，保留为失败实验。模拟输入不能当作真头显追踪验收。

持续运行入口已实现显式 start/stop、会话身份核验和 native→XR 联合收尾。PID 43724 已观察有图后的 180 秒持续输出，跨旧 120 秒仍更新，明确 stop 后 native/XR 和入口恢复正常；四阶段各 10,632 对、XR 14,401 帧/200.010565 秒、最大间隔 28.5519 ms。但最后发布 10,615 张、接收 10,614 张，严格计数相等审计未通过，不能宣告全项持续验收通过。真实头显输入驱动桌面旋转/平移的 PID 39232 验证仍有效。差 1 与单槽 stopped 边界的代码语义一致，但没有逐 packet 尾部证据，只作推断，不追零放宽门限。下一步验证菜单/关卡生命周期、断连恢复，以及真实头显显示和物理尺度；scale=100、活跃 TAA、其他关卡及舒适度尚未验收。详见文档 56。

Continuous2 已补启动器意外退出保护：PID 33264 仅强制结束自创 owner，未发送 stop 事件；native/XR 约 4.328 秒内自行清理，28 入口恢复，游戏仍存活并回到普通 mono。XR exit1 是明确记录的 owner 故障结果，独立异常恢复审计通过；原 owner 的陈旧 running 报告未改写。

仓库内统一入口 `tools/vr-launcher/launcher.py start/status/stop` 已在 PID 44640 实测：无需传 PID，自动启动并核对版本，等待注册的关卡玩家后激活双目。该入口本身不导航菜单，本轮由已有 game_io 辅助进入关卡。新图发布/接收均 2,334 张，projection 3,166 帧覆盖 43.959 秒；停止、GPU drain/close 与 28 入口恢复通过，正常关闭游戏后无本轮进程残留。它仍依赖本机仓库布局，不是可复制部署包。详见 `analysis/runtime/vr-launcher-smoke-44640-audit.json`。

第一人称只读采样取得 448 条接受记录，原摄像机相对 Pawn 的位置差为 (-160,+48,+76) native units，EyeHeight/BaseEyeHeight 均为 60。`Pawn.Location + Z*EyeHeight` 仍只是候选眼点，尚无 renderer 对齐或身体遮挡验证；后续动态采样已观察到 Pawn、Controller 与相机旋转不能视为等价。Continuous3 的两次模拟器回中验证了参考代际 1→2→3，水平朝向回中保留真实 pitch/roll；统一入口命令也已在 PID 41704 和 1756 实测，整轮结果和限制分别保留在文档 56。重载或断连恢复、稳定第一人称和真实头显显示仍是交付缺口，runtime 保持 Meta XR Simulator。

新增默认关闭的眼点关联观察器后，PID 44200 的 3,727 次 Scene/family/renderer 关联计数与 64 条保留回执通过静态一致性检查，但未观察到实际相机转向，尚未完成动态验证或应用第一人称位置。该轮模拟器输出约 61.10 秒、最大提交间隔 16.46 ms，正常停止并恢复全部 28 个入口；发布/复制 3,330/3,329 的严格等数仍失败。PID 15204 因进关卡超时在激活前结束；修正菜单调度后的 PID 38012 已完成自动进关卡，却在证书末尾跨帧检查处拒绝启用，且 XR 清理后实际退出 `0xC0000409`。证书全边界重取修正已通过 49 项 CPU 测试，尚待实机重验。退出转储已定位到 Meta 模拟器 `SimRpcManager` 在 DLL 卸载时创建清理线程失败；无游戏、无 Hook 的独立 PID 20656 测试复现了相同 32 帧异常调用链，底层线程创建失败原因尚未证明。后续需分别处理模拟器退出、补动态眼点关联，再做第一人称 A/B/A。第一人称辅助模块仅通过 CPU 测试，尚未接入实机；完整证据与限制见文档 56。

后续 PID 21208 在恢复 mono 后获得 437 条动态接受样本：Pawn pitch 保持零，Controller/POV pitch 随短时输入变化，23 条两者旋转不一致；这是非原子观测，不能把这些旋转来源互换。IDA 已对运行时快照的相机主体 RVA 0x110878 做定向分析，确认模式调用、局部位置插值及后续旋转写入，眼高的实际取用仍未知。

PID 42228 已补动态眼点关联：36 条保留回执出现 4 种原始视图朝向，推导 yaw 跨度 4.279°、pitch 跨度 2.675°，Scene/family/renderer 各 1,912 次接受、拒绝计数为零。两眼四阶段各 1,551 次、矩阵不匹配为零；XR 输出 30.1476 秒，最大间隔 16.5231 ms，所有本轮控制进程实际退出 0，28 入口恢复并正常关闭游戏。发布/接收 1,541/1,540 的严格等数仍失败；runtime head 同时变化约 13.411°，不能把全部变化仅归因游戏输入。5 个证书 guard 均首次成功，因此有限重试本身仍未在实际游戏中触发验证。第一人称 A/B/A 默认关闭的实现已完成 13 组原生自测和独立代码审查，但尚未运行于游戏。接下来优先用新 XR 接收回执诊断第三人称停止边界，而非先启用第一人称。

**保留的失败与限制：** PID 38784/21208 的控制器没有真正发出回中请求，分别暴露跨时钟域和整份日志解析问题；修正为 QPC 与有界尾部读取后，36212 两次请求及实际应用通过。38784 的约 3 秒投影缺口和 21208 的 XR 退出异常仍保留为失败；36212 未重现，不代表已确定或修复其根因。旧 PID 45440 的精确动作时间窗口结论因时钟域混用降级，原始姿态、同 QPC 域的矩阵证据保留；36212 使用实际记录的 perf_counter/QPC 时钟验证全部动作。严格来源窗口当前还要求生产者零发布跳过，它不是逐个 native 消费样本的精确关联证明。更正、契约边界及失败证据见文档 56。

第三人称 receipt1（PID 24396）进一步确认本轮停止尾差：原生发布 3,102 张，XR 接收 serial 1–3,101 全部连续，每张都有成功 projection 提交回执，4096 条诊断容量没有丢项。最后接收 ticket3107，native 最后发布 ticket3108 未被接收，terminal 保留相同的最后接收编号。本轮差 1 位于最终未接收发布，尚未修改停止策略，也不反推旧轮次全部相同。实际输出 61.0634224 秒、4,398 次 projection、最大间隔 16.7439 ms；XR 退出 0、28 入口恢复、GPU 收尾及正常关闭通过，无自有残留。完整证据见 receipt1-24396.audit.json 与该前缀 .independent-xr-tail-audit-v2.json。下一步应优先关卡/断连恢复及真实头显第三人称验收，保持紧急停止、过期姿态和所有来源校验；若改善正常停止末帧消费，需区别正常停止与故障停止，不能单纯放行 stopped 图像来追求计数相等。

## 阶段 1：每次都找得到玩家摄像机

**问题：** 内存地址会随对象创建、关卡加载或进程重启改变。

**原理：** 查询当前模块基址，利用经过版本哈希校验的引擎注册表入口，按类名和对象关系寻找玩家摄像机；从反射元数据读取 CameraCache / POV 的字段偏移。

**前端类比：** 不记住页面上某个临时 DOM 对象的地址，而是每次按稳定标识重新找到组件和节点。页面重新挂载时旧引用可能失效，摄像机重建也类似。

**我负责：** 地址解析、版本适配、无效对象拒绝、日志。

**你负责：** 根据提示重载检查点、重启并进入关卡，必要时左右转镜头。

**验收：** 原运行、重载、重启三种情况下均找到当前本地摄像机，输出数值符合实际操作。地址未发生变化也可以通过；关键是新进程中独立解析成功，而不是要求它必须变化。

工具：`J:/gearsVr/tools/package-inspector/Resolve-Camera.ps1`。

## 阶段 2：在正确时机控制第一人称视点

**问题：** 找到 CameraCache 只是找到了游戏保存的一份视点；修改太早可能被覆盖，修改了错误副本可能没有视觉效果，脚本更新还可能与渲染不同步。

**原理：** 追踪“干净游戏摄像机 → 场景视图 → GPU 绘制”这条路径，确定后续画面实际读取的位置和时机。Hook 可以理解为给原函数加一层包装，保留原函数的执行，再在已核实的时刻观察或调整结果。

**前端类比：** 更改 state、调用组件 render、浏览器把画面显示出来是不同环节。游戏逻辑更新、渲染视图构建、Present 也不能混为一谈。

**实验顺序：** 只读追踪 → 微小固定旋转实验 → 恢复原值 → 第一人称视点验证。先用约 1° 的可预测偏转测试位置是否正确，再接头显。

**我负责：** 原生函数分析、Ghidra/反汇编、调用时序证据、可开关的实验代码。

**你负责：** 看整个场景是否按预期改变，切换开关，确认视点位置；保持游戏焦点。

**验收：** 偏转可重复、可以撤销、不会不断累积；真正影响整个场景视图；能区分游戏暂停与接口停止执行。初步第一人称位置合理，不在角色身体内部。

Ghidra 是阅读原生函数的工具；它不会自动生成原作者完整 C++ 源码，也不会自动找到 VR 接口。C++ 编译工具与 JDK 21 在实际需要时补齐。

用户报告 IDA headless 已可用，后续需要时可用于定向调查游戏调用；本轮无需重复计算器示例，也不把工具可用当作游戏逆向或 VR 验收结论。

## 阶段 3：打通头显与 OpenXR

**问题：** 游戏渲染画面和头显运行环境是两个系统。

**原理：** OpenXR 提供会话、姿态、每眼视图及图像提交接口。先用独立测试场景确认头显路径，再接游戏画面，避免同时排查两个未知系统。

**前端类比：** OpenXR 类似外部 SDK：先证明 API 连接和最小示例正常，再接业务应用。游戏适配层负责游戏内部，OpenXR 层负责头显通信。

**步骤：** 确认实际头显/串流/OpenXR Runtime → 独立方块场景 → 显示姿态数值 → 游戏 D3D12 图像捕获 → 临时同图双眼显示。

**我负责：** 独立程序、图形资源同步、提交时序、画面颜色、诊断数据。

**你负责：** 启动实际头显连接，确认测试物体能看见，转头、歪头、平移并反馈。

**验收：** 独立场景左右眼正常、六轴跟踪数据有效，随后游戏图像能持续显示。此时即使游戏仍是同图双眼，也只标记为显示通路通过。

参考：Khronos 官方 hello_xr 包含 D3D12 示例；不直接假设参考项目的 119.559631° FOV 或色彩设置适合当前头显。显示相关参数要结合实际格式和每眼视图核对。

## 阶段 4：让游戏视点跟随头部旋转

**问题：** 图像能进头显，不代表转头时游戏摄像机会动。

**原理：** 读取头显相对于回中位置的姿态，转换到游戏坐标系，再与原始游戏视点组合。先测试左右转头，再上下看、歪头；不要直接把四元数分量当成欧拉角相加。

**前端类比：** 用原始状态与输入推导显示状态，而不是每次渲染都继续给旧状态加一遍相同增量。

**我负责：** 坐标转换、旋转组合、回中、追踪有效性、画面与姿态的对应关系。

**你负责：** 坐着做小幅三轴旋转，验证方向、比例、回中和漂移。

**验收：** 转动方向正确、停止后稳定、回中可重复，持续运行无累积偏移；没有把明显不同时间的画面和姿态错误配对。仍不能把这个阶段称为完整 6DoF。

## 阶段 5：证明真正的双眼立体

**问题：** 我们需要让游戏从两个眼睛的位置绘制场景。

**原理：** 左右眼各自具有视点和投影信息，近物与远物在两眼之间出现不同视差。最终目标是同一游戏状态下的左右眼场景视图。

**实验策略：** 用户已明确只接受同帧双目，AER 不作为实现路线。主线为同一游戏状态下提交两个原生视图；若证据表明该路径存在难以修复的共享状态问题，再评估同帧顺序双渲染。后者两次绘制之间不推进游戏逻辑，并需要分别管理眼图与历史资源，不能直接重复调用会销毁对象的消费函数。

当前已通过同帧双视图提交、消费、清理及稀疏图像验收，确认原生 P00/P8 图像响应，并通过真实 XR 两眼 pose/FOV 和明显旋转/平移驱动同帧双场景的桌面头动阶段。图像 ticket 已接到真实 GPU 输出，在 Meta XR Simulator 中完成 120 秒静态与 30 秒显式模拟头动的游戏 projection；动态矩阵时序/值对应审计通过。持续模式实际输出已跨 180 秒并正常停止，末尾发布/接收差 1 张的严格等数检查仍 false，符合单槽关闭尾帧的代码推断，见文档 56。下一步完成菜单/关卡/断连生命周期及真实头显显示验收；物理尺度和活跃 TAA 尚未验收。

**我负责：** 两眼视图与投影、渲染目标、图像生命周期、深度及后处理历史、眼睛与姿态归属。

**你负责：** 观察门框、近处墙角和远景，确认深度、左右眼顺序，再观察移动物体。

**验收：** 静态场景有真实几何视差，左右眼图像均持续有效，比例和视图范围正确；动态测试记录是否存在眼间时间差、重影或历史污染。

这是可行性关键关口。若接口不足，要重新调查渲染路径，不能靠复制或平移同一张图宣布完成。

## 阶段 6：加入头部平移，完成 6DoF

**问题：** 身体向左右、上下、前后移动时，游戏观察位置也应变化。

**原理：** 以游戏视点为锚点，加入经过坐标和尺度转换的相对头部位移；再组合每眼视点。世界尺度通过实测校准，不把“100 游戏单位 = 1 米”直接当成已验证事实。

概念式：

```text
每眼最终视点 = 游戏基准变换 × 转换后的相对头部变换 × 每眼变换
```

这里的乘号表示组合三维变换，不能理解成把所有角度和位置直接做普通数字乘法。最终实现还要处理参照空间、回中和姿态有效性。

**我负责：** 位移转换、尺度、第一人称锚点、回中、不累积的应用方式，以及临时跟踪失效时的行为。

**你负责：** 在原地缓慢左右探头、升降和前后移动，验证遮挡关系和移动比例。

**验收：** 六轴均能独立改变视点；只移头不转头时能看到物体新的侧面；回中后可重复；相机与两眼结果保持同步。

## 阶段 7：变成可以玩的一版 Mod

**问题：** 单个场景工作不代表正常游戏流程工作。

**原理：** 给常规游戏、菜单、暂停、过场、掩体及其他特殊镜头分别定义行为，处理第一人称模型遮挡、HUD 和视点变化。

**我负责：** 状态切换、摄像机生命周期、特殊场景适配、性能测量、日志与安装/卸载说明。

**你负责：** 用原有输入方式测试第一章，反馈菜单可读性、瞄准、掩体、翻滚、过场和暂停恢复情况。

**验收：** 第一章核心流程可重复运行；重载、重启后自动定位有效；已知异常明确记录；可开关与恢复。双手操作、武器 IK、手动换弹、电锯动作等另开后续里程碑。

## 每轮我们怎么配合

1. 我说明这一轮要证明的具体问题。
2. 我用几句话解释原理，只引入本轮必要的术语。
3. 我提前说明后台代码会读什么、是否会写入、实验如何结束。
4. 你只做一项明确的操作；我不抢游戏焦点，尽量不用 computer-use。
5. 我报告实际观测结果，区分已证明、推断及仍未知的内容。
6. 更新报告和本路线中的验收状态，再开始下一轮。

一次只改变一个关键变量。例如“站立不动 → 只左右转头 → 只上下看 → 只移动”，不用同时跑动、转头、瞄准来验证一个字段。

不会把脚本暂停等同于渲染停顿。游戏失去焦点会暂停，因此测接口时要记录焦点/游戏状态和渲染时序，避免误判。

## 随项目学什么

| 开发时刻 | 只需先学会的概念 | 前端对应思路 |
| --- | --- | --- |
| 地址定位 | 对象、指针、字段偏移、对象生命周期 | 对象引用、属性访问、组件重新挂载 |
| UE 对象调查 | 类、实例、反射、元数据 | class / 实例、运行时类型信息 |
| 调用路径 | 脚本、native、Hook、调用栈 | JS/原生边界、函数包装、DevTools 调试 |
| OpenXR | 会话、姿态、参照空间、图像提交 | SDK 会话和接口调用 |
| 头部旋转 | 欧拉角、四元数、变换组合 | 三维 transform 的组合顺序 |
| 双眼绘制 | 视图、投影、渲染目标、帧同步 | 相机与 framebuffer；后半部分需新学图形知识 |
| 头部平移 | 世界/局部坐标、尺度、相对变换 | 父子 transform、单位换算 |

不用先完整学会 UE、C++ 或图形学。跟着当轮实验补相应知识；需要写性能敏感的游戏内代码时再集中讲 C++。

## 下一步

第一人称基础视点参考方向已由用户确认，见 [First Person 相机参考记录](12-first-person-mod-analysis.md#已采用的第一人称相机参考方向)。研究它的肩后偏移清零、分状态视点和身体显示处理，再建立我们自己的第一人称锚点；抬头与高度联动不作为 VR 站蹲 / 探身输入。此项是记录和后续研究方向，尚未实现。

受限真实头动、游戏图像二维显示、当前关卡同帧双视图、原生投影响应、真实 XR→同帧双场景的桌面头动，以及模拟器 120 秒静态和 PID 40776 的显式 Operator source1 动态游戏 projection 已通过，见 docs/14-live-head-camera-test.md、docs/15-game-screen-display.md 和文档 56。动态试验保留 raw flags，运动、冻结 ticket 端点和消费/准备矩阵时序对应均已核验，不能当作物理头显验证。PID 43724 持续输出跨 180 秒、明确停止和联合清理通过，但末尾发布/接收差 1 张使严格持续计数验收未通过。差异与单槽 stopped 边界语义一致但没有逐 packet 尾部证据；持续 coverage 和联合 lifecycle 分项通过，不追零放宽门限。接下来分别验证菜单/关卡生命周期、断连恢复，以及真实头显图像同步、物理尺度和舒适度。真实头显阶段须重新确认实际 runtime 与每眼配置，不沿用模拟器身份或此前 SteamVR 的参数。活跃 TAA、实机回中、主动断连和跟踪恢复及稳定第一人称锚点也仍待验证。最终交互见 docs/10-vr-interaction-design.md，可部署交付要求见 docs/16-deployment-target.md。

## 官方参考与项目证据

- 摄像机证据：`J:/gearsVr/docs/02-camera-analysis.md`。
- 定位工具：`J:/gearsVr/tools/package-inspector/resolve_camera.py`。
- [Khronos hello_xr](https://github.com/KhronosGroup/OpenXR-SDK-Source/tree/main/src/tests/hello_xr)：OpenXR 最小示例，包含 D3D12 后端。
- [OpenXR 规范：XrView](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html#XrView)：每个视图的姿态和视场信息。
