# The starship, the gun changer, and the UI and subtitles are moving forward.

2026-10-14. The first version is available for three; the user clearly indicates a "wrong" position de-targeting point, with a normal graphic shape. This page records the diagnosis and does not mean that the location repair has been completed.

## 2026-10-15: Full UI inventory and batch range

The first version of UI has been extended to menu/suspension, ammunition/active switch-off, interactive/trainer alerts, mission/teammate/rescue markers, death retests, chapter checkpoints and crosswords, which still include acoustic, gun exchange and real-to-white subtitling. Full delivery lists and project progress are available.[Target for deployment](16-deployment-target.md)。

Reuse Existing `Inspect.exe` Export UI candidate function in two script packages to the batch of depressed copy: WarfareGame 1705 Yeah, yeah.156 owner;Engine 1004 Yeah, yeah.129 A homeer.2709 All candidates generate files and export abnormal logs to empty; of which 1093 The blogger says that the government is not a party.1010 File contains unsolved `__NFUN_*` token，888 Quantum `@NULL` Type is in place, so it cannot be called full original source code or 2709 The selection is saved by precise regulars, does not cover all game/interface resources or prove the actual execution by name. All derivatives are left locally and do not add to the release package or Git.

Evidence:`analysis/ui-v1-script-audit/manifest.json` Save Packages and File-by-File SHA 256(b) Scope and limits of selection;`selection-regex.txt`、`summarize.py` The entry is the original entry audit. `analysis/ui-position-runtime/ui-coverage-audit.json`。

| Route | Current evidence | Batch fixes of borders |
| --- | --- | --- |
| Start Menu | WGC Window Image XR plane | Current only covers the first double-eye renewal; pause/return menu cannot be automatically followed |
| Tile/Icon | `1d52b4 → 1bc05c → 19e64c → 172741`, real index/ticket evidence available | Normal screen HUD can share layout; not all sub-items can be covered by PostRender father |
| Canvas Text | `2482b8 → 1bd880 → 19e64c`There's a whole section, Dotlas range. | Aligns with parent UI; text in world tag does not directly change to fixed screen text |
| # And the white subtitles | `2a4370 → 2470ac → 246c58 → 1bd880` | The original entrance has been collected and no real glyph is known; the white scene still needs to be addressed |
| MaterialTile | `3a4d3c → 3a5548 → 3a514c → 15a420 → 159fcc` | **Independent Channel**Current Materal packaging is only observed and not covered by Tile/Text receipt |
| World markers for tasks/teammates, etc. | `UpdateHUDLocators`Dependence on camera location and object visibility; partial projection portal is located | World anchors, scope, frame-to-eye projection still need to be certified |
| Pause/Sett UIScene | Type and status known | Actual native drawing entry, target and synthesis sequence not certified |
| We'll wait for the wound. | The world's final export route is known. | Can not process entire post-sections by HD level |

The new evidence of Material:20 bytesrenderitem,112 Bytespayload,36 Byte drawing log, given consumer `15a420`（command vtable RVA 136c 5d 8+8 Slot, back 36 It's with Tile/Text.208 Bytespayload,48 The byte vertex is different and the latter layout cannot be used to resolve.`159fcc` The actual GPU drawing/matrix is still pending and no new running-time changes are enabled in this round.

Follow-up direct reverses are being tracked. `159fcc` To GPU: four for each record 32 Byte vertex, position is float 3;non-indexed path DrawInstanced returns `1af345`Index path DrawIndexenstead returned `177c4a`, can re-use existing Ds 3 D detour, but an exact Material/command/target/ticket connection must be added. First access to a single screen HUD log, formerly consumer, is re-opened to the actual GPU draw; no Payload will be destroyed `15a420` Call twice. The initial step state retains the raw material behaviour for non-test/not writing depths. Detailed instructions and limitations are available `material-ui-gpu-route-contract.json` / `material-ui-gpu-route-static.json`. Only path evidence is available, and the Material eye fixes have not been enabled.

`WarHUD.DrawHUD` _Other Organiser `UpdateHUDLocators`、`DrawObjectives`、`DrawHealth`、`DrawActions`、`DrawDownedButNotOutTeammates` - Like branch.`UpdateHUDLocators` Reads the player camera, calls the Locator update and visibility update to show that not all of the content in the parent PostRender is fixed on the screen UI.3 The father’s scope inheritance is not sufficient to justify the application of a common plane; subsequent mass authentication of screen layouts must exclude or separately process relevant branches of the world.

The next set of operations will be driven by public channels: first, a clear classification of the known screen HUD, a quasi-star, and world markers; re-use the Tile/Text consumption end and supplement the Material consumption end; complete UIScene displaying the switch; and project the same frame with an eye-to-eye projection, leaving text layout, tailoring and hinting time lines. There is no certified Hook that can directly amend all UIs. The global index can be reused, and originals can only continue to direct translation of the above gaps.

## %1 confirmed drawing level

Original `172741 → 19e753 → 19f2f3` The Canvas GPU route draws the same geometrically to the right and the left half width of the view/cissor, without using a two-eye view matrix or FOV to recalculate UI coordinates. It allows UI to appear in both eyes, and does not address the correlation between the star's direction and the actual game orientation.

The script is based on the centre of Canvas. ClipX/Y, and the gun layout is based on the SafeZoneFriendlyCenterX/Y set by PostRender. Normal HUD and the target star are treated separately: the target of a game needs two eyes projection; the gun UI needs to keep an inner layout and select a consistent display reference. All HUD cannot be moved evenly to the end of the hit point.

Keep the access to the cache on run:

| RVA | Validated use | Current Limit |
| --- | --- | --- |
| `1d6ca0` | Shared script VM Executing Entry, RCX=this, RDX=FFrame, R 8=result | Sieve semantics by current UFunction/ Object |
| `1d52b4` | Canvas. DrawTile Packing Entrance | Canvas state is not a complete decoded parameter |
| `3a4d3c` | Canvas. DrawMateralTile Packing Entrance | Ibid. |
| `2482b8` | Canvas. DrawText Packing Entrance | Not covering independent primary routes for white subtitles. |
| `cb3f00` | Canvas.Project | Use Canvas+`a4` The single SceneView has not yet proved which eye it is. |
| `1bc05c` | Submission boundary after DrawTile decoded | x/ y has not created syntax to GPU watch association for Org+Cur |

Node/Object/Code/Locals for FFrame `18/20/28/30`;Locals cannot be used as a decoded Canvas primary parameter. Canvas. Projects `ca71f8/31f6b0` Use SceneView+`2b0` . This fact cannot replace the running evidence of Canvas. SceneView with the original view/eye view.

## UiObserver 1: optional semantics observer

List of experiments `tools/vr-launcher/manifest-ui-observer1.json` Keeps the default release/preview list unchanged. Four trampoline are reserved for new processes, and re-reflection after entering the level will result in controller, camera, pawn, HUD, interventory, Canvas class, function script scope and field identification before you can start to verify the observation. Seven precise scopes include accurate stars, weapons superstars, DrawWeaponSlots and WarHUD. PostRender.

TLS scope supports nesting restoration; unknown scripts keep parent syntax and the known but failed subclass does not pass as parent HUD. Three Canvas packagings maintain original parameters, call times and return behavior without modifying location. Maximum number of kindxdraw drums 48 Okay, front 12 A maximum of two lines per second after call. Stop, disable and release records when calling to zero.

`latest_built_ticket_hint` It's just that the entire world recently generated serial numbers, visible tags. `ticket_association_verified=0`; it cannot be used to connect Canvas calls to GPU frames or eyes. Scope calls are not frame numbers; sample rejections are not all drawing call failure rates. HF VM scope identity reading is still expensive and the monitor does not function as a default for posting.

## First actual operation: PID 43652

OpenXR Simulator subprocess runtime override, without changing global runtime. Start menus share XR PID with level 4712;situation of static scenes 230.212654 Seconds, submit 20722 Frame, Copy 16458 New scene, maximum submission interval 22.4514 ms. The frame submission contains reuse, which does not equal the game render frame ratio.

- VM Records 1473639 Second time, WarHD. PostRender accepts 16874 Number of refusals 0。
- No star-marking or gun exchange scope triggers; all Canvas barrels call in 0 No coordinates.
- Windows refuses normal front-stage activation, without sending open/script input; the screenshot shows two eyes is normal and inactive HUD. This round only provides proof of observer access and static output, and cannot exclude other UI routes or UI repair.
- The narrate/XR stop normal,32 Independent validation of all entry points is restored and the game is properly closed; simulator input does not count as real location tracking acceptance.

Evidence reserved for `analysis/runtime/continuous-stereo-43652-20261014-165604-62446fa0.*`、`ui-observer1-43652.audit.json` and `ui-observer1-43652-idle-mcp*`. Session report passed by existing binary and end-of-pipe check does not mean that UI function passes.

Local check: UI Configuration Python 12 Project, launcher/navigation 34 Division, substantive runner 31 ; the normal, continuous, fame-link, owned-prime-handoff checks for UiobserverTest and CaptureSelftest. The actual startup also performs life cycle checks for existing reservation.

## Subtitles for the independent route

Static Evidence Confirms View `2c78f4` Call PostRender (Present)`3dc864`) Empty HUD. Canvas, call GetSubtitleRegion ()`3d0f0c`) and subtitle manager `2a4370`. Current custom `2470ac → 246c58 → 1bd880` Enter the original text batch without Canvas. DrawText in the current PostRender scope.

GetSubtitleRegion belongs to WarGameViewportClit/Engine.GameViewportClit. SpeakLine queue, subtitle-enabled sign and GetSubtiteleSize measurements can only be used as a prefix and cannot prove white gramph display. Following up is the need to observe the original path during real whitetime and check for two eyes; four observation hooks do not claim to cover subtitles.

Minimum inverted/directive evidence retained in `analysis/subtitle-route-native/`、`analysis/subtitle-route-static/`、`analysis/ui-position-runtime/canvas-project-submission-static.json`. These local derivatives do not enter the release package.

## Target cache already in place.

`WarPC.UpdateHUDActions` Call in local Pawn PlayerTick to execute the original CamRot 8000 Unit track, write Impact. HitLocation to reflect field LastCrosshairHitLocation (current version offset) `136c`. This path is used for a quasi-star colour/mark, which cannot be called the final random dispersal or auxiliary impact point after the aim.

The HUD missing, specific Activaction, smoke mask may all return before the value is given; no branch of weapon can be used as a reliable target. If the subsequent cache is used, it must prove that the call was actually granted and linked to the scene ticket. The function returns, caches are not zero or changes in values. There is no additional call for GetAdjusted Aim with random dispersal status.

Evidence:`analysis/ui-position-runtime/crosshair-writer-summary.json`、`crosshair-writer-script.json`、`crosshair-field-43652.json`。

## Physical verification of open mirrors, cut guns and embedded calls

UiObserver 1 / PID 44988 Successfully implemented 3 Second-scene and DAD right cut, both eyeshots are visible. The projection is active.73.7911015 seconds,6644 Sub-submitted,5196 New graph, maximum interval 19.0163ms，32 The entrance is restored.5607 Tile,1175 Next, keep separate 33/32 All right, no sample denied; the precision of star-and-gun change is still 0. The screenshot has UI, so can't put 0 Explanation is "no actual drawing".

Directed compilation and running time function identity is subsequently confirmed:`1d6f30` FFrame and opcode loops are created directly by embedded script branches, bypassing shared VM `1d6ca0`. It places the caller frame on `PreviousFrame+38`. Canvas currently contains the Canadians.DrawIcon or Weap_AssaultRifle.DrawAdditionalCrosshair, all of which are located in the corresponding script.

UiObserver 2 The chain is traced back to the three existing Canvas packaging portals without adding Hook.12 Frames, the latest precise semantic scope is accepted only if back to the certified TLS root frame; known but invalid subscopes, reading failures, loops and failure to reach their roots are rejected. Root allows normal advance, without requiring entry into the court. Record clearly distinguishes between root_institution, matched_frame and made_depth; no GPS link is created.

PID 36892 Actual:7894 Second Canvas chain classification, denied 0；GOW 3 Star-sharp.892 I. Additional star size of rifles 223 Second, change of gun.2884 Other HUDs 3895 Minor. Starless Tile 18 Lines; change of gun Tile/Text 18 rows;other HUD Tile/Text 35 Okay, sample denied.0. Validated DrawIcon to GOW 3 The ancient decth 1 And correct scope belongs. After long waiting for the focus of the window, actually finish opening and cutting; two eye amplifiers are saved, and the whole projection is made.685.649956 seconds,61693 Sub-submitted,47553 New graph, maximum interval 168.9184 ms; this value is not the full-time no-Carton commitment. Normal stop and 32 The entrance is restored.**The coordinates have not been modified and the classification cannot be successfully completed as UI restoration.**

The evidence is in the same place. `analysis/runtime/continuous-stereo-44988-20261014-184007-ea0a225a.*`、`continuous-stereo-36892-20261014-185144-e8c30c45.*` and accordingly `ui-observer*-mcp*`;slot distribution of evidence as `analysis/ui-position-runtime/nested-script-dispatch-44988.json`、`frame-nodes-44988.json`. The list of new experiments is: `tools/vr-launcher/manifest-ui-observer2.json`。

## Error projection of eyes with screen coordinates

PID 44988 It's...348 The CSV sample records a constant asymmetric FOV and two-eye unit orientation. The transecting of the star is approximately at the centre of each eye image. `(u,v)=(.5,.5)`Left- and right-ray horizontally approximated 15.037°Three-dimensional angle.29.44567°There is no common goal in the front. A common head forward direction corresponds to a left eye. `(.62126,.40975)`Right eye. `(.37874,.40975)`. These values are only the current round simulator data and cannot be written to be real headline correction parameters.

Calculation and pixel detection methods, source file Hash, thresholds and limitations `analysis/ui-position-runtime/ui-fov-audit-44988.json` and `audit_ui_fov.py`. No latest ticket hint is relied upon, nor is it determined that the target for the cache is fresh or real impact point.

`tools/vr-common/ui_projection.h` ♪ Have achieved pure mathematics `project_ui_anchor`: The world point certified by the same caller is placed in each eye pixel coordinates using a frozen eye matrix. CPU checks cover the above-mentioned FOV reverses, different depth visions, rotational smoothing, exterior points of the visual mouth and illegal input. It has not changed game drawing; target source, HUD plane depth and batch/ticket attribution is responsible for subsequent access.

## UiObserver 3 Semantic vertex to actual drawn candidate association

Read the chain of UCavas ⁇ FCánvas ⁇ the current sorted element BatchedElements before and after the call for an existing DrawTile package, only confirm that it is just new 4 Record between the ends and bytes of a file. Real vertex steps are as follows:48 bytes, where 44 Byte value assigned by Tile Production Code, tail 4 Bytes do not participate in content comparisons. Sorting elements are searched by key; only new addresses are read after the vertebrae is re-assigned. Payload must remain the same as item, reading failed, cutting unnewed and layout changes rejected. It does not increase the number of Tile drawings or change the toppoint.

Add the only consumer entrance `19e64c`, verified by real time bytes and command vtable, ABI is `uint32_t Execute(void* command)`. This function destroys payload after drawing internally; the monitor removes a fixed capacity candidate before calling, compares current toppoint content from area to region and uses embedded TLS to bring matching results to existing ones `172741` GPU Callback. Recorded ticket from actual `frame_link.overlay_pair`, not the recently generated ticket tip. The original function is called only once and returns as it is, and does not read released payload after returning.

Most candidate containers 16 Group, each 32 Intersection. Repeat/conflict/group spill invalids until the consumer arrives; block packagings are stopped and not silently phased out when new groups cannot be recorded. The records contain unknown vertex numbers and semantic composite masks, which cannot be interpreted as missing batches belonging to a certain group of UIs. Candidates for establishment are not distributor identities; bytes do not support each time indexed drawings are used. Currently, only read-only diagnostics are available, and no toppoints can be directly authorized.

The new entrance will restore the check to its maximum.33. UiCavasBatchTest, UiPayloadReceiptstest, UiObserverTest, UiProjectionTest and continous ABI self-censorship; default preview list not upgraded.

UiObserver 3 / PID 9232 In actual terms 3 Recording of second-in-sight/gun cut 5742 Additional space,1668 Consumer content matching, rejection/expiration 0. New Hook inserts a frame of function in the Call Inn, and the old three frames adjacent check rejects all overlay, GPU connections 0, Max. interval 7178.84 ms. The wheel cannot count as UI pass; regular session reports passed only to cover the existing closing conditions,33 The entrance did resume. `analysis/runtime/continuous-stereo-9232-20261014-191940-51e21904.*` The actual four-year-olds were retained.

UiObserver 4 Add precision four frames to the observation mode:`172741 → 19e753 → hook_ui_execute 的 PE unwind 函数范围 → 19f2f3`and require the same thread, Execute to save a real return address as well. `19f2f3`. The original conditions for target/ticket/view continue to apply. The route identity is separated from the semantic candidate, so unknown graphics and text do not lose their original eye copy because of lack of candidates. CPU tests overwrite error functions, return entry address and order; list is `tools/vr-launcher/manifest-ui-observer4.json`。

UiObserver 4 / PID 26980 Not completed for route validation: Win+0 After actually cutting back to the game, the simulator XR becomes VISIBIBLE; bridge logs lost 6682 Minor inactive, only constructed 198 A ticket. The open-view is black and the GPU still links to 0. The round should be saved as a display of failure and cannot be passed by withdrawal from normal claims.

The subsequent cross-check of upload/distributions also corrected Uiobserver 3/4 It's...44 Byte step error:`333de8` Use 48 Bytes allocated,`1720a0` Press 48 Byte Offset/Upload and Set IA stride=48. The same error steps were used in both rounds of production/consumption comparison, so the byte matching cannot prove actual toppoint content.5 Replace by 48 Byte step, before comparison 44 bytes and add tests with independent pedding; old experimental data are not used for coordinates correction.

### UiObserver 5 Actual: PID 45216

Use the freezed new build and emulator automatic test focus options, each 3 Second-opening mirrors and slicing guns, both eyeshots are normal.139.801223 Seconds, submit 12585 Frame, Copy 10463 New graph, maximum submission interval 18.103 ms;native inactivated refusal 0. Normal stop, XR exit and 33 The entrance was restored and the game was then regularly closed.

- 5840 All four vertexes are obtained for the next Tile additional observation, read/addition/dispatch 0。
- 1693 Full content match for each consumer candidate, header/content/return value rejected 0;containers are not overflowing and no legacy group.
- 2916 Group overlay actual records and submit, denied 0;syntax candidate associated with actual ticket: a quasi-star 464 Second, change of gun.699 Other HUDs 527 The rest.1226 No Tile candidate can be considered UI for or move on this basis.
- The sample is covered by a single semantic full vertex; the scene does not show mixed candidates and does not prove that all weapons/menus are not mixed.

Evidence:`analysis/runtime/continuous-stereo-45216-20261014-193621-c0dcdcff.*`、`ui-observer5-45216.audit.json`, as well as prefix ADS/weapon screenshot. `tools/vr-launcher/manifest-ui-observer5.json`。**The actual UI coordinates remain unchanged.**

The original emulator sends VISIIBLE on the preview loss of focus, and the existing Focused door limit for Mod will stop building new eye maps. Add local test options `XR_SIMULATOR_FORCE_FOCUS=1` Change only the emulator VISIBLE/FOCUSED policy, by default, to close; actual OS Focus Log and mouse release is reserved without a wake-up call. Patches and dependence/build-up Hash are recorded in `tools/openxr-probe/simulator-focus/`. The original DLL and global runtime were not replaced, only the XR sub-process was assigned independent manifest. CPU focus strategy 82 Item, CTST 3 and 5 sec 446 The frame pre-screened; the OS-minus incident did not appear in this round ' s actual log and the single backstage request to the preview was rejected by Windows, so it cannot be claimed that the defocal branch was covered by the live event.

## The minimum achievable boundary of a target star.

Static decode confirmed. `UpdateHUDActions` Cache value of hit in Script memory `+359..+37f`; the package contains different store offsets that cannot be used directly in the process. The current Canvas chain does not contain a quitted writer frame, and sharing VM access may be bypassed by embedded scripts, so the existing sample cannot prove that the Tick is granted value. If limited depth hits are used, the actual grant value and its owner, tick, scene tipet is not replaced with an old cache without zero or change.

The smaller direction scenario is to project the same original game view centre rays into two eyes as "the star in the direction of the original game views." This avoids the separation of directions caused by the hard-to-do image center under asymmetric FOV, but does not provide a real point of impact after closeness, shot-port deviation or auxiliary aim; the direction and actual sight of the original view must still be checked. Normal HUD should use the common layout on its own, without having to follow the command.

## UiObserver 6/7 Index, freeze matrix and whole text

`ui_index_membership.h` Save current entry on Execute 36 Bytessegment and unit 32. Members by `minimum + index` It was decided that no overlap between upload areas would be used to replace them.64 Segment,8192 Index; zerodraw also drives by primary cycle, checking the full five DrawIndex parameters and confirming the quantity after the original call.1720a 0 The static cycle verified that each segment produced one in sequence 172741 Retouch; the same parameter is not a distinguishing identity per se, and does not search for subsequent security in equal numbers. Large arrays use a bound heap, which fails to discard only observation. All completed check only read their own copies without reading the parload that was called by the parload.

The scene is built with a video/presentation and actual eye array, followed by the ticket atoms to be retained and retired in the same Draw/Prevent sequence lock. UI only search for the actual overlay_pair ticet without latest position; missing/conflict does not escalate to success.`ui_plane_projection.h` The pure math test covers the original camera attitude, asymmetric FOV, depth vision and projective motion after turning. In general, the videoport scaling does not accurately represent the entire plane; this helper has not changed GPU.

UiObserver 6 / PID 14964 Access 1736 A complete index sequence, but the user then explained that he had adjusted the window with a FancyZones buffer from 3840×2160 And become 3842×2005 And change the resources. Fixed-size checks are rejected, and overlay and UI matrix samples are all available.0, maximum submission interval 185455.2 ms. Regular end-of-service passes do not mean pass. Focus is always valid,33 The entrance is restored and closed.0 Inaccuracy, user describes the root cause of this case; the report retains negative experiments `analysis/runtime/ui-observer6-14964.audit.json`。

UiObserver 7 Adds the whole paragraph of the text, Range receipt, and preserves the original quad fit.32 Paragraphs,1024 vertex, use independent-based copy heap; long text does not press 32 glyph cut. The existing DrawText packing function supports multi-texting and redistribution by calling the new range of all the em at once; embedded Tile/Text does not repeat records. Subtitles are in the exact entry.2a 4370、caller return 2c 9375 Collects single-source FCavas, original Rax, rectangle and layout. Dotlas calls in a common lock to publish all of them or drops the corresponding Payload candidate altogether; failed to change the text.

PID 37184 Done at steady dimensions.3 Second open mirror and 3 Second-cut guns, eyeshots, scenes, icons and gun text. Actual results:

- Ongoing 206.034605 Seconds,18546 Sub-simultaneous submission,14081 New scene, maximum submission interval 17.8515 ms; current round without any change in size, inactivated rejection 0。
- 5301 Additional space for Tile, and 1112 Paragraph text,13344 Text caps, production/reading/publishing denied 0, the packaging is not spilled or left behind.
- 2650 The whole consumer content match, and the rest of it is not a good idea.2650 The index sequence is complete.2650 Group overlay actually performed, none rejected; gpu_missing_context=0. The reserved UI lines are all associated to the corresponding freezing geometry of the ticket.
- Subtitles portal 14497 The scene was empty and the view could not be shown as verified by a hit.
- Normal stop, XR close and 34 The entrance is back through and the game is closed.12 The group is linked to the CPU test and the self-testation of the continuous ABI.

List as `tools/vr-launcher/manifest-ui-observer7.json`，DLL SHA 256 `58438c20230469061974701ee97266a74a6c364c6bd3c69e4235734df13121c0`;evidence prefixes `analysis/runtime/continuous-stereo-37184-20261014-213300-8d5d594c`, Summary `ui-observer7-37184.audit.json`. Code Checkpoint 341a 800(Indicator/matrix),6aaff 64(The whole text/subtitles). The coordinates have not been changed or the physical head start test has not been done; a scene does not prove that this is all the game.

## Next

The actual GPU return occurs after the toppoint upload. Full location correction requires a separate VB copy or certified GPU conversion for each eye; cannot be modified in the backturn and looks forward to uploading the top point change.48 Position 4 Yes.0..0 f, UV/colour/hitproxy and paddy are to be kept; every eye VB is to track and restore the original IA binding, and cannot be released until GPU fence is completed. Normal HUD uses common plane with whole group text; acoustics need to relate the actual target/value of the original game to the time series without expiry of the Cache.

The window is not just about removing the rejection sign: binding, sharing texture, receiving end images and fame ticket require new resources to be institutionalized. The current fixed-sized version will stop with a new map and no scaling up recovery capability is currently declared. `analysis/resize-recovery/architecture-plan.json`. The three DLSS/DLAA engine intercept points are checked with the Apple return rules, and the running lock is still not enabled and INI off cannot be used as a full lock.

## HUD plane control first run (2026-10-14）

e 5ede 89 / UiHudPlane 1 / PID 46800 First time to start independence 70°、2 Mihead-locked HUD plane. Simulator runs 791.65065 Seconds,71243 Sub-projection,54971 Sub-copy, maximum submission interval 68.5128ms；2984 Sub-plan and actual eyes 0，34 The entrance is restored and the game is properly closed.1492 Text is overwritten; original subtitles 55393 No glyphs and no samples from the protostar.

Both screenshots of the operation can see a double eye view, but no clearly visible change UI/ text; visual correction cannot be declared successful on the basis of application count alone. 0.1 Met level/0.08 The scene changed with the shift in rice and the ow/pitch/roll non-zero simulator posture; HUD visibility, plane location and actual subtitles still require targeted investigation evidence. `analysis/runtime/ui-hud-plane1-46800.audit.json`Not counting the real head of acceptance.

Amplogram limits identified: MCP D 3D 12 Screenshot Read Simulator Window Preview RT (in current round)1734×257) Then presses 1280×189;Fill Windows mode flattens the full eye map into preview, not original 1920×2160 Eye-mapping. The cut-off diagram is not sufficient to prove that HUD has disappeared; it cannot be attributed to the game projection. One eye MCP is only tailored to preview and cannot restore lost pixels. Next, a normal size preview or original GPU reading back will be required to determine the location of small fonts.

The pure support module for the quasi-star cache value has been added. `crosshair_assignment.h`, validate the exact byte code of execLet, attribute/owner chain, and a Veter copy completed after the original call, including equivalent value. Independent CPU testing passed; no grant-making hook has been installed or associated with player tick/ actual scene ticket, which does not allow claims that the point of aim has been repaired.

## Original GPU pixels and Material route (Pretty)2026-10-15）

`manifest-ui-material1.json` / PID 51800 Add the Material UI consumer observer that is default closed (s)35 This round has been sent with an open mirror, a cut-off gun and a new one.6574 Sub-proceeding,5036 A new map. Normal stop and...35 Access is restored; Tile/Text path has a straight star, gun exchange, HUD sample, Material count 0. The current sample should therefore be processed along Tile/Text, and cannot claim that the whole game does not use Materal or that it has been run-time validation of this branch.

Original GPU snapshot is now complete 3840×2160 SBS pixels, requested on demand in session with clearly opened diagnosis, with the largest number of sessions per session 8 Chang. PID.22904 / Pixels 2 The first time you have successfully saved a separate file during your operation, you can see clearly the open-scope star, the ammunition HUD and the gun exchange icon/number.493,0 Best near, with this HUD plane forecast (-)493.356,0) is consistent; this is only the pixel evidence of the current scene, not representing all UI/real-heading experience. Images and quantitative records:`analysis/runtime/ui-pixels2-ui-alignment.json`Round, same round`.gpu.ppm.1.ppm`. The quasi-star is still at the centre of each eye image and does not apply asymmetrically astrometrically to the original game direction.

Pixels 2 The snapshot log reads serial before the backnowridge, and records the last confirmed image; images and ticket are valid and serial cannot be used for frame association. Pixels 3 Amend to read after the jack is successful, the GPU set is completed and add single-tang clock time for saving finished.424 / `manifest-ui-pixels3.json` Actual 3 A complete chart, all matching the size-ticket logs of the image-ticket log, and serial; two of these save time falls during an open mirror, a cut shot.5667 Sub-proceeding,4137 A new map. Normal stop and...35 The entrance is restored. First Pixels 1 Original realization is read serial while saving, not by Pixels 2 This regression effect. Three rounds of audit documents are prefixed to the session`.ui-pixels-audit.json`。

The next one is to correct the center of the original game screen for pure star-segment, using a homogeneity freeze matrix and projecting in an infinite distance to the later eye level of the videoport; maintain the original graphic size. This option is not equivalent to near-cut points, detachments or real impact restoration. Subtitles still need to have actual glyph samples of white, nor does the normal PostRender syntax automatically extend to all world signs.

## UiReticle 1: The original game direction correction has been checked in pixels

`manifest-ui-reticle1.json` / PID 47092 Starts the default OF OFFF for the first time `GEARS_UI_RETICLE_DIRECTION`. Only when the current raw index is all on the same Tile-associated call, with a limited location for standalone top-point copies and z=0/w=1. No more than the centered box error 1 Pixels and widths are smaller than canvass 1/3; extra-sync/screen scope/mix/unknown batches do not automatically take over. Extent saves the value by actual index before original consumer calls, and does not read destroyed payload. Use the actual overlaytice source view/projection and eye matrix in w=0 Map the centre direction of the screen, only horizontally shift the view, crop the scissor and restore it.

Actual 458 Sub-stellar plan and eye-draw pass, center/matrix/ projection rejection 0, other HUD 2134 Sublevel fix, sequence_faault=0。10775 Sub-proceeding,8012 A new map. Stop normal.35 The entrance is restored and the game is properly closed.650/serial 643 Matches with XR records; the centre of the quasi-Star white pixel frame extracted from the threshold is by baseline (in thousands of years)959.5,1079.5)/(2879.5,1079.5) Move to (1192.5,884.5)/(2646.5,884.5), with the calculation position (1192.812,885.062)/(2647.188,885.062) is consistent within the pixel grid. All four white pixel frames are 60×104 Pixels, original maps can also see normal star shapes directly.

CPU testing `/W4 /WX /UNDEBUG` Compiles, covering asymmetric FOV, rotation, horizontal without prejudice direction, index attribution, mixed source/multiple root rejection, failure to modify output; HUD and index return, continuous ABI self-measured. Code is read-only reviewed. DLL SHA 256 `185161b70a9fa00ce755644f014382fef269d46ab00e558897d7053eafdd4ef4`Evidence. `analysis/runtime/ui-reticle1-47092.audit.json` This confirms the current situation, which is a correction of FOV's direction, and does not prove that close-to-place hits are low, shot at any point, all weapons/menu/subtitles/real headline experience.

## First non-empty subtitle production record (in US$)2026-10-15）

PID 53760 Normal advance to start room, corridor and yard entrance.727.19 seconds,65225 Sub-proceeding,45428 A new map, normal stop with 35 Access restored. Subtitles statistics =1(receipt count as NativeSubtitle=)2 First time obtained 559 Non-air conditioner,1845 Paragraphs,110312 Toppoint; Production is not rejected by delta or publish. Here's the exact 2a 4370 The entry is not displayed in pixels.

Complete 3840×2160 It's...6 A snapshot is not intercepted. Last-read queue monitor gets global RVA via getter's RP relative location after matching sourcegetter/selector bytes with history snapshot 1c 7dbc 8;stablemanagerhead is count=0/capacity=0 The observation is later than entering the corridor.181 Seconds, this cannot negate the previous non-empty glyph. Then PID 46520 Open Before Moving 60 Secondwatch, still in empty queue; the stop log has 65 Non-empty subtitles calling,129 Paragraphs,2820 Toppoint, which indicates that the current queue time and screenshot coverage is still insufficient. The actual picture also shows players entering bunkers by the yard door, not sending forward keys as having passed through that position.

The GPU context was only HUD kind, abandoned the original NativeSubtitle source and could not be used for random scrapd 4 As a subtitle ticket.`analysis/runtime/continuous-stereo-53760-20261015-015852-f4f5fcfa.*`、`continuous-stereo-46520-20261015-021439-a72cff7f.*`、`subtitle1-queue-walk.jsonl`、`subtitle2-queue-walk.jsonl`。

GraphicsLock 1 Increased`ui_segment_sources.h`, maintain a source/root range as true copy index, reject mixing, unknown, expired and mismatches;16 Subtitles GPU record retention status of ticket, list promotion, execute security, QPC and eyes replay/command complete.`UiSegmentSourcesTest`/W 4 /WX compiled and adopted without adding a native book or modifying the top point.53840♪ This round of menu tests subtitles GPU calls/rows 0 The fact that the real subtitle GPU was hit cannot be claimed to have passed.

The current snapshot does not specify a ticket, and the GPU doe fence is only re-entry. The subsequent precision map should link the submitted subtitle list to the same ticket published by Present, with the receiving side saving only the ticket/session; overstepping clearly miss does not automatically use the next frame. Finally, you need to check text pixels for eyes position.

## 2026-10-15: Independent output not yet connected to Canvas/UI

Sustain 1、Menu 1、Reload 1 The independent Low scene continues to be exported and partially changed, but the old Backbuffer UI conclusion above cannot be followed. The current source code has three clear break points:

1. `capture.cpp::hook_family`Receiving FCavas;`try_submit_pair`Only the ones that did it.`+0x10`The scene target is changed to owned adapter. Follow-up compilation check confirms that the third parameter of renderer, although named Canvas, actually comes from FCanvas+0x 18 The load is not a FCavas object, and cannot be replaced with an individual. Canvas' target and stifling relationship requires independent verification.`native_owned_scene`Status specified as`canvas_redirected=0`。
2. `hook_draw_indexed`UI replays the target of a known swapchain buffer, to use`frame_link.overlay_pair`Take association. Downed scenes use independent Tracker and`image_tickets`; existing HUD/sync planning queries`stereo_ticket_store`. A simple relaxation of the buffer judgement would lack a succession, target, queue and freeze position/geometric link to the Ticket order and could not be considered repair.
3. `native_owned_scene_copy`The site consumption return, the destruction of the renderer and the submission of two world views are available. If Canvas consumes later, it must first obtain the end of the ticket's Canvas and the actual order to submit the border, or clearly have no UI evidence for the ticket, copying/pushing and advancing the ticket. Trial currently requests just two worlds to draw down; UI should be recorded as an overlay, not mixed in the world ' s draw count or write unknown as a success.

Family in next-directive reuse database `0x3a148c`、renderer `0x116250`UI observation `0x19e64c`This is not equivalent to new original back-to-back compilation conclusions or GPU evidence. First resolve the association/time sequence, then authenticate the star, change the gun UI, menu, subtitle pixels; maintain existing paper identification and attitude limits.

## 2026-10-15: Canvas-to-scenes document orientation validation

Re-use complete database and existing compilations, only fill in Flush and solvency call clips without re-programming. History snapshot SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, module base 0x 140000000; still distinguished from disk EXE.`analysis/ui-position-runtime/owned-ui1-ida/snapshot.i64`Copy Job, please is closed, original SHA 256 Hold on.`66d17edbf21bf81f0fe5cf740111e738a0308ac9228f7576fe39f460baad1813`。

Established ABI/field relationship: family entrance RCX is FCavas; FCavas+4 It's the target interface, +.0 xc is optional;batch programr `0x117430(item,FCanvas)`Put the partm+8 Give me the payload.48 Byte Command, Command+8 It's payload, plus 0x20/+0x 28 It's the goal.`0x19e64c(command)`Back 48 And release Payload. `0x1172c4(FCanvas)`Assign by theitem false list; the parsing of Flush returns to address as 0x 2a 6b 9f。Reload 1 Four of the running records confirm FCanvas+4 Equals the original family+0x10- The evidence is...`owned-ui1-static-contract.json`and the byte validation of the directory; offset is not a generic definition across the versions.

Add Optional`GEARS_OWNED_UI_ROUTE_PROBE`Only two access points to Flush/producer are observed, with existing Execute monitors; no change in target, no change in original call times or conditions for release. Fixed capacity ledger links on Canvas, threads, clearly selected ticket; payload registers before production, checks empty after transfer, and only uses value copies when Excelute returns. After closed, it remains open for execution/submission, repeats payload, wrong targets, old ID and volume depletion.

Ui 1 First installation failed: two new Hooks omitted to modify the capacity of the assembly branch at reisize, refused before the application queue and no proof of operation was obtained; repaired as base capacity plus 2, failed to build with list.2 SHA 256 `2f8c3c64537b9b6548150706093af76bd2e35869ea85eb65e49a9ff8601047d9`Two rounds of simulator testing were then conducted:

- PID 52888：40.092 seconds,1,364 A downed scene.1,362 Sub-issue,1,360 Sub-XR copy. Canvas links to the structure closed down.1,364 Number of production/implementation cases 0 First 32 A Flush sample is empty.32 The independent access review was interrupted by the early closure of the co-ordinated game, without independently returning to conclusions; the game exited normally.
- PID 38236: Confirm entry through memory status`DeltaPause-Campaign-Host`Focus`LBtnResumeSingle`Then Esc resumed.40.134 seconds,1,280 A downed scene.1,279 Sub-issue,1,277(a) Secondary XR copying;11,187 Production/transfer/implementation/completion of equal number of UI batches, failure, mistargeting, payload conflict, average volume spill 0;all 1,280 cycle closed and recycled.11,168 A producer is not selected for Canvas scope, and its Execute is unconnected and clearly does not count as a successful batch.61 The game exits normally after independent matching of entrances, without force.

This confirms that the CPU batch of this scenario pauses the menu, and is not independently exported UI pixels or position acceptance.2 Keep Only First 32 cycle, which is all empty and does not save a cycle time series for subsequent non-empty batches; they cannot be inferred from aggregation that they are all before/after release. The source code then adds the whole timescale count and independent head 32 Non-empty cycle save, Ui 3 Only compile/CPU tests, not yet live.1,569 Passed; FrameLink, Payload, index, owned scene with ABI-related return pass.

Operational audit:`analysis/ui-position-runtime/owned-ui2-52888.audit.json`、`owned-ui2-menu-38236.audit.json`. The static, running evidence and scripts are left in the signed anarysis. The default player list remains unchanged, the native resource is kept until process exits are completed, launcher's incomplete is not eliminated. The next achievement is to direct the associated batches to downed targets, access the urban tracker eye cover play and the same ticket/geometry, and close Canvas, fully associate Execute completion and real GPU submission as a pre-replicate condition; Material and other uncertified routes cannot be automatically included.

## 2026-10-15: Pause menu drawn into independent eye output

Add Default Closed `GEARS_OWNED_UI_BATCH`, rely on owned UI association and owned XR. First look for Execute in the knowledge base `0x19e64c`, reuse saved compilation without adding an IDA session. Check two complete functions of the current process before installation `19e64c..19e791`、`8bee1e..8beec0`, Merge SHA 256 I must. `1b804155ae4801e5c554227420cf988fb994138d305d6d11c349295410f8be25`. This limits the target replacement to the checked running code and cannot be used in other game versions.

Copy on the stack while executing associated batches 48 Byte command, replace only `+0x20` Target; original FCavas, original queue command and payload do not rewrite. Execute calls once and releases the old payload. Draws with a nicket, command list sub-present and resource connection from own tracker to execute left and right eye overlay; UI does not count as third world draw. Downed image instruments save both original lens arrays and eyes geometry, and protometric stars and common UI stop searching for old backbuffer document warehouses.

The release must be satisfied: this Canvas has been analysed for closed, complete and complete Flush list with all the supporting batches forwarded and Execute completed, the world eye is submitted, and related D 3 D recording is submitted. GPU waits for a query about non-consumption or destruction of tracker certification, and then performs the original resource/queuing/status check. On waiting, no more unrelated backbuffer binary copies are created. Currently only access is granted to the watch; Materal/unknown trigger path is stopped and not considered completed.

HUD increases the independent source width ratio, and square single-eye output remains the same as Canvas 16:9 shape; acoustic planning also supports the source pixel ratio and maintains the direction algorithm for the original screen centre. A menu with no semantic labels is available on the same instrument 70 Do, do not.2 Rice locks the panel. Limited distance hits, all weapons and caned-eye support are not verified as such.

- Route 1，PID 25320，DLL SHA 256 `67e4ccdbae25417f6bb4c5651ec61c6e9e16d81353e6b8e29aba50a2c6f615d4`：40.918 Seconds,1,307 A downed scene.1,306 Sub-issue,1,303 Sub-XR copy,3,675(a) Secondary prevention;10,963 Batchs to finish eye redrawing. Turn on the pause menu and return level;667 Non-empty cycle, advance release and route error 0. The builder `observed_only=1` It's an old log text error, actually. `owned_ui_batch enabled=1`; subsequently amended.
- Route 2，PID 47320，DLL SHA 256 `78973d4a5addeb0386d34a55afc1fed8eefae1bec097b7f03860e25ef0e1184f`: Do not create additional double-eye copies while joining the waiting period, keep the menu pause until it stops.70.978 Seconds,3,116 A downed scene.3,114 Sub-issue,3,111 Sub-XR copy,6,382 Sub-proceeding, maximum submission interval 55.694ms；48,279 The first time I was in the business, I had to go to school.2,414 Non-empty cycle, pre-dispatch, mistargeting, route failure and unknown world drew 0。

Both rounds pass.61 The entrance is independently restored and the game is closed without force.2 No. I'm not.1,800 A downed scene (ticket)1801) After the GPU set is saved 4096×2048 Original. Directly view the visible eye with a pause menu option, task title and bottom hint, without any significant stretching of fonts. The image and scene state are mutually confirmed; it only proves that the pause menu is visible, not representing full UI or real comfort.

Evidence:`analysis/ui-position-runtime/owned-ui-route1-menu-25320.audit.json`、`owned-ui-route2-menu-47320.audit.json`;original maps `tools/native-capture/build-owned-ui-route2/owned-scene-47320-late.ppm`, the thumbnails are reviewed `analysis/ui-position-runtime/owned-ui-route2-47320-menu-review.png`. All derivatives and construction are ignored and not included in the release package.

FrameLink 17,455 Item, ledger 1,665 Van, owned scene 179 The entry and HUD/Astrostar Ratio, the Viewport Restoration, Capture ABI/family test passed.2 After running source code to replace Execute 48 Immediately ceases upon return, previously blocked but only waiting for time out; Route 3 For compiling the difference, you cannot put Route 2 Run the records as Route 3 Truth check. The default player list remains unchanged, the resource is kept until process exits are completed and deployment exit processes are not completed. Next step is to use the same urban path for proof of open-scopeing, gun change and subtitles, respectively, and then handle other menu/Material paths.

## 2026-10-15: Independent output open-screening, gun change and subtitle test boundary

Route 3 / PID 50132 / `manifest-owned-ui-route3-ads.json` Actual 40.107 Seconds,3,605 Sub-proceeding,1,739 Copy the next XR new figure, maximum submission interval 55.429ms。1,743 All Canvas cycle recovery, of which 314(a) Non-empty;1,768 Double-eye redraw, plain-scale.1,480 Stars.288 Road failure and early consumption, average 0。3,540 All the settings of the index are completed, no security value.61 The game is closed normally after the entrance has been independently restored.

Confirm in the level by using primary menu status, specify Win+ as user 0 Focus, virtual handles open and display the gun-changing menu, each segment 3 Seconds, all end neutrals. The receiving side original GPU ticket 719、887(a) Falling over two operating periods;4096×2048 The original map is visible for both eyes, the gun-changing icon/text and ammunition HUD, with a preliminary normal shape. `analysis/ui-position-runtime/owned-ui-route3-ads-50132.audit.json`Thumbnail `owned-ui-route3-ads-50132-review.png`This round does not have subtitles and cannot be extended to white; the star is still a center-line correction of the screen, not a limited distance impact verification or header acceptance.

Add then to the default close `GEARS_OWNED_SUBTITLE_PIXELS`. Only the owned draw of NativeSubtitle will be registered; most of them are registered;3 This is a texture evidence that does not provide separate proof of XR acceptance or head-visibility.

The same cycle fixes two completeness gaps: if the unsubmitted own overlay is discarded by the command list Reset or reset, Tracker loses the release authentication; submitted lists and unrelated resources are not affected. The semantic index cursor must be finished first, owned ledger must be allowed to do so, and it cannot be published before subsequent sequence failure. There is no semantic consult watch that can still follow the authentication path.17,529 Item adopted, ledger 1,665 The checks are passed on the items, indexed membersship, owned scene and Capture ABI/family.

Route 4 / PID 51376 Subtitles are failed experiments: scripts move from menu status and camera telemetry to the bottom of the corridor 2- Protection stops;2,162 A scene that was consumed before, a subtitle candidate.0. The head of the construction 32 A Flush sample is empty and does not distinguish between failure to read the List, material/unknown type or ledger refusal. There is no evidence that this was attributed to subtitles themselves or all of the previously black screens.61 The entrance is restored and then it is normally closed. `analysis/ui-position-runtime/owned-ui-route4-subtitle-51376.audit.json`. The subsequent construction only adds the first list of failures and the specific reasons for rejection, continues to position itself effectively blocked and does not ease the release threshold.

Route 5 / PID 50488 Re-emerge and narrow down the cause: first rejection of ticket 2748、caller RVA 1167f 4, the list is read successfully, total=4、batch=3、material=1、other=0，reason=2 Unequivocally not supported, rather than read/unknown type/ledger rejected.2,746 The video is a video of the event.1,728(b) A general batch of eye redrawing;3,474 The index sequence is complete. Subtitles still available.0. The Materal monitor actually published=9、claims=8、content_rejected=0、bad_return=0♪ but gpu_candidates=0、gpu_rejected=8, the gap is in a quad or draw parameter authentication and cannot yet be called as having been correctly mapped by Materal.61 The entrance is back. The game is down. See you at negative. `owned-ui-route5-subtitle-50488.audit.json`。Route 5, and the default branch that is functionally closed is compiled.

Database hit ui-05 The path uses independent data.36 Byte Command,112 Bytes Payload and 32 Byte GPU vertex, not fit for regular Canvas batches 48 Byte command or top point explanation. Next item first goes to the existing one.8 Specific parameters/steps rejected by GPU and checking true IA, targets and relationship to ticket continue to be uncertified as a denial of release; subtitle visual acceptance is still incomplete.

## 2026-10-15: The landmark assumption for the Materal candidate has been amended

Route 6 / PID 36168 Keep the head under conditions that keep Material blocked 4 A copy of the GPU rejected value.15 All consumer claims returned successfully and normally, and all GPU candidates were rejected by the metadata check.4 All records are non-indexed DrawInstanced, carler RVA 1af 345，4 Top Point/1 Examples, beginnings 0；stride 32、first 0、primitives 2、min 0/max 3, proxy and expected_proxy are equal. The only thing that does not match the old judgment is flags=9237（0x2415 The old code requires the whole thing.16 Bit field equals 0x 411. The goal remains 3840×2160 Original Backbuffer, not as an urban-eyed UI.

Re-access to database`0x14015a248`Hypocrite and existing compilations from SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`Original history byte check: RVA 15a 3"Ca"`25 9d f5 ff ff`（AND EAX,0xfffff59d），15a 3d 6 Yes`0d 11 04 00 00`（OR EAX,0x411) The equivalent is simplified to`(old & 0xf18c) | 0x411`; saving other bits for initialization cannot require an equivalent of the whole field. The actual fixed slot condition is`(flags & 0x0e73)==0x411`. Two instructions are now also in the actual process of checking binding; additional tests cover field values and 16 Flip in place, sourceblob validates scripts for all 65,536 Low 16 Bitvalue check value equivalence. First hand count the AND address deviation 2 Bytes, captured by independent byte assertion and captured in Route 7, no error address is built.

Route 7 / PID 49384 Using the amended read-only conditions,10 The first time that the production of Materal/claim/Execute was completed, it was a normal event.10 Quad/draw candidate, GPU rejected 0；10 Lines of the Article are to be maintained as paragraphs 3 non-indexed RVA 1af 345、command_completed=1. All ticket_candidate is still 0，mutation_allowed=0 The list of names is still rejected.

Both rounds stopped and passed.61 Separately restore the entrance and then close the game.6/7 The list, source document and DLLHS are frozen; the CPU Material test is passed with DLL ABI, functionally closed for branch compilation and adoption. `analysis/ui-position-runtime/owned-ui-route6-material-36168-material.audit.json`、`owned-ui-route7-material-49384-material.audit.json`Byte evidence `material-flag-mask-bytes.json`. No new IDA library, full language re-establishment or amplification navigation.

Next around the Matericer `3a514c`、consumer `15a420`Contracts exist to complete current real targets/upload IA/Canvas completed connection with owned ticket and retain consumer for one call and original resource life cycle; Tile command cannot be followed`+0x20`Replace, because the Material offset is flags and the logical goal is in a family of independent SceneView. Once this path is completed, the white subtitles and other UIs will continue to be validated.

## 2026-10-15: Materal Life Cycle Base and Orientation Tectonic Validation

Downed UI ledger distinguishes between Batch and Material, checking the expected quantity, production, transfer, execution and completion separately. Materic must clearly set out the intended logical target when producer links, consumer cannot reverse the target; old `transferred(true)` Only prove that the Batch is empty, not for use in Material. `MaterialDispatched` Only means that the production path has been handed over, and not that GPU has been successfully drawn. The common payload uniqueness check between types does not allow a ticket to be published; allows sync consumer to finish first, and producer to return after. Existing regular batch calls are compatible.

After re-entry database hits, fill in private IDA copies only for producer, upload functions and associated compilation ranges of two tectonic functions, by command against history blib SHA 256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`Confirm. `11739b` Call original FCavas as RDX itsem producer;`3a514c` Read `+4` Logical objectives,`+18` Mode and `+20` Agent, Materian Payload did not have a cleanup agreement after the transfer.`3a574c→15e428` Write target wide-high `+50/+54`, copy the original office view matrix and the matrix saved by Payload to the original one as well `+70` and `+b0`. This is the construction of data stream evidence and does not amount to a final shadow/view or actual OM binding certified.

Original `3a52e3` Command bytes `48 8d 05 86 47 75 01` Press RIP to make it worth the matrix RVA `1af9a70`;the display label for the Raw IDB is `1af8a70`, the actual address cannot be changed by showing text. The evidence is saved in a ignore directory `analysis/ui-position-runtime/owned-material1-ida/material-producer-upload.json`、`material-view-ctor.json`. Working copy please close, copy the source of IDB SHA 256 Keep it back and forth. `b47742b1c7d0d97201432d395283c853571193f1fdaffd3ed9c6e290b71ae55fb`。

New `build-owned-material-ledger1` Compiled, ledger run 1,877 Checks, Materal tests and DLL ABI/family self-measures are passed. No game started, no previous logs built are used as this GPU validation.

This submission is only for a typed life cycle and has not yet enabled the Materal output path; the current list will still prevent unsupported Material releases. Next, the target/ instrument link of producer is connected to consumer, and the actual drawing of goals and matrices determines that changes are done with the smallest eye, then continue with subtitle paths. The static database coverage is not considered V 1 Completion rate.

## 2026-10-15: Materal has entered the actual binary output, and the next block is binding the tracking life cycle

Add Default Closed `GEARS_OWNED_UI_MATERIAL`, requested owned Batch and Material observer. producer `3a514c` Linking the original FCanvas/ Logic Objectives to typed target;consumer `15a420` Still executes the same document and actual OM bound 4096×2048 The texture's non-indexed texture performs eyeview/scissor repainting; the status quo is restored. Any indexed/world/projective Matherial is still not supported.

Independent upload screen check `1af054` ♪ The context, the pounce ♪1、2 primitives、32 Bytestride and Exact 128 Bytequad source;Draw check the same list of actual commands, stride 32 Taku-poo!5. Producer and upload to the Delaw range are performing historical bytes SHA before installation 256 Compare. ViewPlane only certifies the original SceneView’s opinion view, original matrix copying by bytes, limited standard Canvas XY/W map to most 1 The source pixel is offset without changing the source data. The GPU goal/command list is checked separately/inventory evidence.5 Parametersupload ABI and number of calls to original function, local correction `simulator_operator` Shade; same for the first round of actual global switches 0 The amendment therefore does not purport to change the first round of results.

- Route 1-a / PID 38532：40 The following Materal production, execution, completion, original upload and eye re-graving are approved.0。2,635 Downset consumption, regular Batch first_fault=9 End of follow-up release, subtitling candidate.0. The first independent recovery tool has not recognized two new ones, and refused 63 List of items; two complete areas of independent Hash to be completed 63 All matches, normal closings, no killing. Early Route 1 Starts the attempt to complete test targets for the starter because the new directory has failed due to the absence of a test package for EXE.
- Route 2 / PID 44700：20 Second full life cycle and eye-repeated passage of Materal =0；2,699 The government has been working on the issue of “black-handedness” in the country.1,589 A normal batch of eye redraws.2236 Read the same texture after UI is finished and GPU is submitted, save after fence is completed 4096×2048 The original map; direct view of the right and left eye is armed with HUD, without a visible stretch. It proves that the frame's eyes are visible, does not prove all Material branches, subtitles or head-over inspection.63 The inspection was regularly closed after the independent resumption.

Route 2 Also get a specific refusal from the regular UI: ticket 2701、caller 172741, correct call chain, but `tracked=0`Neither the actual objective nor the video/scissor were recorded;`frame_link_capacity` Medium `target_binding_overflow=96`，list highwater 268 Over and above the old binding table 256 Slot. Old `frame_target_stats` It also emptys the border with Precent, and the output is owned to cross it. The missing observation data cannot be interpreted as the actual GPU target as nuld or as the only reason for all previous black screens.

The following restoration: the downed path uses an independent recording period binding state, syncs reset/RS/OM and startup phase descriptor create/copy, maintains the source of descriptor without being emptied by Presten; only the CPU binding of the list of the CDlosed is recovered after submission, keeps the generated GPS tracker's execution/implementation evidence and other currently recorded lists. It must continue to reject real and distribution capacity, lack of Reset and unknown sources. `Subtitles=True`, but scripts are from file setting ID 40 Sets the running time switch; subsequent check from the actual menu/profile status cannot be assumed to have been opened by ILI alone.

Signed in this cycle, ledger 1,877 Checks, Materal/ViewPlane tests and DLL ABI/family checks passed, as did additional self-measures required by the starter; functionally closed branch compiled.`analysis/ui-position-runtime/owned-material-route1-a-38532.audit.json`、`owned-material-route2-44700.audit.json`Visual review `owned-material-route2-44700-review.png`. The experimental list is saved separately; the default player list is unchanged.1 Not yet completed.

## 2026-10-15: Independent recording bound, continuous UI and paired subtitles

Adds a strict recording period binding form to the owned pixels path.Only successful Reset or first opening creation can be observed to establish a record;The list is not filled with unknowns.Create/ Copy, maintain the database source without emptying the desktop by using canonical COM input.Only CPUs that were originally returned by Execute and submitted for registration will be retired from the actual membership of the group;Other lists that are being recorded and GPU submission/execution certificates are maintained.Scene, common UI, Materal, Clear and MRT checkes are all available for independent sources.Real and simultaneous. 256 The tank is depleted and continues to be refused explicitly. The old picnel-only route that did not open remains tracked.

Strict life-cycle test coverage 1024 Order list, parallel retention of another record, prohibition of hidden reconstruction after submission, Reset restoration, descriptor reservations and 257 The complete construction, associated CPU testing, DLL family/frame-link self-measurement and default closing branch compile is denied.

- Binding 1 / PID 33460：2913 Yes, owned scene, independently bound overflow=0; no UI triggered, input stopped because focus verification failed. XR exits waiting time out and cannot be considered complete for continuous or outgoing acceptance.
- Binding 1-b / PID 32724：3158 Yes, owned scenes;3565 A normal UI command and eye redraw,348 Material Production/Implementation/Fulling/uploading/Early Retweaving, UI bonds=0, bind overflow=0、descriptor loss=0. Not released before UI is finished. As planned 130 Second test window stopped but final count stalled, XR maximum submission interval 38188.837 ms; not full flow.
- Subtitles ticket 3024、3036 Saves the same instrument, glyph/submission/fence/readback record.3024 The images of the eyes are available for both eyes on the white “Kim: Who's next!” and the weapon HUD; this only proves that the subtitle is visible, without any proof of subtitles, comfort or real appearance.

Both rounds stopped and independent verification was conducted. 63 The entry is restored and the game is properly closed; native owner is kept until the game exits, launcher's cleanup status is incomplete. Second XR has itself closed without mandatory clean-up of XR. Users also report that two images are often interchangeed on the desktop and that the existing samples do not capture the phenomenon directly; the reason is uncertain and is not attributable to the entire historical black screen.

The freezing list is as follows: `manifest-owned-binding1.json`、`manifest-owned-binding1-b.json`♪ Share unrebuilded `build-owned-binding1`. Audit and source document/DLHAH is located in `analysis/ui-position-runtime/owned-binding1-33460.audit.json`、`owned-binding1-b-32724.audit.json`;images reviewed as `owned-binding1-b-subtitle-review.png`. Next, you can associate anchords to reject, stop publishing and actual state changes, then improve the exit of resources, menus, high-level, cross-gate and real header acceptance.1 Not yet completed.

## 2026-10-15: Reverted Death Menu resulting in the double-eye stopover

Binding 2 / PID 12880 Add optional diagnostics to existing Pawn binding, retain original reading order and all rejection conditions, before recording 16 The number of failures failed; no access to the game or new Hook was added. CPU tests cover the same/non-diagnostic reading sequence, empty Pawn, six associations, class/outer and changes during review; freezes the build self-measurement and default closure of branch compilation.

Actual game+ emulator completed 2868 The scenes are all over the city.2391 Sub-normal UI and 215 Once upon a time, Materal, both UI malfunctions and binding spills.0. First rejection and all 16"and keep a record of all the transactions `stage=pawn field=controller_pawn actual=0 actual_valid=1`. Independent reading of the same process confirms that the Controller.Pawn is 0 Camera.ViewTarget still points to the original Pawn; then recertified menu reads confirmation `DeltaGameover`Focus `btnCheckpoint`, CONSSistant_reread=true. This proves that after the role dies and the loss of possession, the two-eyed guard correctly rejected the old player scene, but the existing one-time startup panel does not restore the display of death menus. It cannot be attributed to all historical black screens or desktop alternation reported by users.

After Stop 63 The entrance was independently restored, the game was regularly closed and no killings were carried out; the original Owner remained in place until the process was withdrawn and normal retirement had not been resolved. `analysis/ui-position-runtime/owned-binding2-12880.audit.json` Saves the original record with DLL/L-Hashy. Next, add a visible and reversible scene/menu display to maintain the original scene identity and attitude time limit; desktop mirror gaps are handled separately. This submission is for diagnosis only, without declaring that menu restoration has been achieved.

## 2026-10-15: Reversible death menu shows realization, actual restoration branch to be covered

Add Optional `menu_recovery` List entries are only active in the life cycle of the startup_menu. The starter uses MenuReader to authenticate the same PID/create time, register UI tree and repeat it consistently.64 Byte atomic file. Initial authentication only `DeltaGameover/WarUIScene/input_mode=1` For the death menu, the observed permanent-based Crosplay scene is a candidate for restoration; unknown, failed sampling, expired and wrong identity does not authorize death panel. No changes were made to the native Pawn guard or relax 100 The masturbation is valid and there's no increase in Hook.

XR can save the window capture for return menus. Empty old maps and create epoch when entering the menu, using WCO SystemReleasiveTime to confirm that the actual catch occurred after epoch, with the highest cache 1 seconds. Menu status lease 1.5 Second, once you enter the Death menu, you must receive a valid game candidate again and get a new certified double-clicket later than the restoration of the border to complete the continuation after success. The screen plane and the program are only one frame; the old Starup-only list is still being captured by the first successful double-eye release window. The menu panel does not count as a stereo world presentation. Suspended, other menus and cross-Leve recertation remain subject to subsequent access.

Menu time/ protocol/status machine, XR reception and exit etc.8 C++ Passed, Python menu observation 4 Division, substantive runner 36 Project, launcher 35 Adopted. Actual `MenuRecovery1 / PID22156` Use freeze Binding 2 # Native and new `build-menu-recovery1` XR: Start Panel 2933 The blogger adds:261.105 within seconds 8097 A downed scene.7475 Sub-normal UI,499♪ The world is so different ♪0 UI/Material failure 0. XR copy 8089 Zhang, submission 23433 Number of times, maximum interval 112.4373 Ms, do not claim no carton. Old Backbuffer's binding forms are still on record.73 Overflow, not mixed with the new towned binder.

The current round of normal movements has not resulted in any deaths.`entries=0/quad_layers=0/recoveries=0`XR is closed.63 All entrances are restored and games are properly closed; the native resource is kept until the game exits, and the starter remains incomplete. `manifest-menu-recovery1.json`Audit `analysis/ui-position-runtime/menu-recovery1-22156.audit.json`; the default player list remains unchanged. The next round must actually cover the death menu and restore, while continuing to process desktop mirrors and release gaps.

## 2026-10-15 Desktop independent left eyeglasses and still-unsolved flash reports

Add Default Closed `GEARS_DESKTOP_MIRROR`. When the release conditions are met for downed scenes, associated UI and actual queue submissions, copy the full SBS to an independent snapshot and restore the source state; Present will only draw the current swapchain buffer before the left eye is compared to the certified current. Tie up the same DIRECT line, private GPU command bypasses the original observer without adding the Hook entrance.3D 12 Document specified PRESENT/ COMMON status contract, does not declare a new independent backbuffer status track.1 After success, the frame is updated.

Current version of sync waiting for mirrors to be finished, release backbuffer references before returning successfully. Waiting for a maximum of one second; close mirror drawing after time has elapsed, but workker continues to unblock fence and release target references after completion. Continouous stoppage is empty until then shut down; without confirmation the resources are kept open for retesting, no limit can be repeated once Detach is permanently tested. Independent testing actually block queues and overwhelms recovery paths.

Mirror 1 / PID 33548 Access 918 A mirror snapshot, but the desktop is drawn as 0, the target verification is denied. The run record desktop RTV format is as 24（R 10G 10B 10A 2_UNORM), four more than the first edition 8 bitformat;Mirror 2 Increase 10 Bit-end format, source still limited to supported 8 bit format. Only the SDR values are maintained here, which do not contain HDR/PQ or colour field conversion authentication. Mirror 1 And because of the early cessation of the owner's Mission Error,24.10 seconds,917 xR copy; monitoring scripts were then open with normal files.bin, there was a potential for shared deletion conflicts with anatomic replace and no abnormal call pads were obtained, as the reason remained extrapolated.

The menu monitor now captures OSError only at the release stage, keeps the original package and its original sample time, and next time the poll re-samp and replaces the atom; failure and rerecording, unincorporated cover or branding of old packages, and changes in game identity are still unusual.7 The Python test covers files that are occupied, restored, first published failed, cleanup and identity change. Formal XR reader is already using FILE_SHARE_DELETE; new monitoring scripts are rereading the change log to avoid blocking replacement.

Mirror 2 / PID 25668 Keep user settings 1920×1080 Desktop, owned resources remain 4096×2048♪ That's every eye ♪2048 2. Actual running 111.039 Seconds,5,056 Sub-image snapshot,10,106 Sub-desk mirror drawing, destination format 24；busy=0、timeout=0、unsafe=0. CPU fence waiting for average 0.1542 ms, max 2.7639 ms; does not equal the full GPU time-consuming. XR copy 5,052 Zhang, submission 9,988 Maximum submission interval 55.92ms。345 The secondary infinventory refused to concentrate on the start-up phase and no longer increased the number of operational samples; time limits denied 5 Number of times, retained 100 ms door limits. Neither the ordinary UI nor the Materal round appears on this cycle without using this to check UI.

Four.640×360 Window sampling is a single-eye image, left and right black side. The simulator saves 22 Frame rolling evidence, visual reading.6 The frame does not appear to be clearly interchangeable; this only indicates the sample, which does not negate the desktop and scintillation of the simulator reported by the user. Both the original GPU map and the subsequent simulator preview are clearly darker in right eye, for reasons that have yet to be determined, and the full set of eyes cannot be shown as approved. Next steps should be taken to compare continuous source maps, same-sight posture/FOV, rendering status with composite preview, locating exposure/reprocessing or other eye differentials; these are pending directions.

Independent WARP Final Run 5,330 Passed through, covering RGBA/BGRA, SRGB,10 Bit-purpose format, black side, snapshot independence, source recovery, busy skipping, timeout and restoration. Test Environment D 3D 12 The limitation has been clearly exported by Debug Layer, which is not available; the earlier supporting report "Debug Layer does not error" cannot be used as evidence for debugging layers. The relevant CPU, DLL ABI/family/frame-link tests are passed, and the functional branch compilations are closed and fame-link is self-tested.63 The game is closed without force after the entrance has been independently verified; native Owner remains inline until process exits, launcher still incomplete and default player lists are not switched. Audit:`analysis/ui-position-runtime/owned-mirror1-33548.audit.json`、`owned-mirror2-25668.audit.json`. The simulator event directory:`%LOCALAPPDATA%/OpenXR-Simulator/flicker_incidents/session_33872/incident_9026`。

Compared more with the same process, native-born readback: ticket 2 Mean greyscale of two eyes 27.16/24.97；ticket 121 and 1801 It's all about left.29.4 Right about 9.2 The second two, left eye.365,252 The pixel changes. Only the right eye.41/4,194,304 Pixels change and maximum channel differential 1. Simulator 22 The right eye pixels of frame sampling are identical, with minor changes in the left eye. This moves the question of "near-updated/light abnormality" to the original output stage without proving the specific cause or explaining the user report's left eye blinking.4096×2048 Two of the textures.2048 Area 2, no two complete images are simply stacked in the same area; a sample of the upstream eye, shared history/rendering status remains to be found. `analysis/ui-position-runtime/owned-mirror2-temporal-measurement.json`、`owned-mirror2-eye-change.json` and source chart Hashi. The next minimum experiment provides the same eye-eye FOV and zero IPD in simulators, which compares continuous eye maps after checking actual consumption matrices, distinguishing between perspective differences and right eye update paths; restore configuration when completed.

---

## 中文

# 准星、换枪 UI 和字幕的定位进展

2026-10-14。首版要求是三项均可用；用户明确“歪”指位置偏离瞄准点，图形形状正常。本页记录诊断结果，不表示位置修复已经完成。

## 2026-10-15：整套 UI 盘点与批量修正范围

首版 UI 范围已扩展到菜单/暂停、弹药/主动换弹、交互/教程提示、任务/队友/救援标记、死亡重试、章节检查点和过场文字，仍包括准星、换枪与真实对白字幕。完整交付清单和工程进度见[部署目标](16-deployment-target.md)。

复用现有 `Inspect.exe` 对解压副本批量导出两个脚本包中的UI候选函数：WarfareGame 1705个、156个owner；Engine 1004个、129个owner。2709个候选全部产生文件，导出异常日志为空；其中1093个只是native声明，1010个文件包含未解析的 `__NFUN_*` token，888个含 `@NULL` 类型占位，故不能称为完整原始源码或2709个已理解功能。选择范围由精确正则保存，不覆盖全部游戏包/界面资源，也不以名字证明实际执行。全部衍生代码仅留本地，不加入发布包或Git。

证据：`analysis/ui-v1-script-audit/manifest.json` 保存包与逐文件SHA256、选择范围和限制；`selection-regex.txt`、`summarize.py` 和包导出目录可复核。原生入口审计为 `analysis/ui-position-runtime/ui-coverage-audit.json`。

| 路线 | 当前证据 | 批量修正边界 |
| --- | --- | --- |
| 启动菜单 | WGC窗口画面→XR平面 | 当前只覆盖首次双目接续前；暂停/返回菜单不能自动沿用 |
| Tile/Icon | `1d52b4 → 1bc05c → 19e64c → 172741`，已有真实索引/ticket证据 | 普通屏幕HUD可共用布局平面；不能据PostRender父scope覆盖所有子项 |
| Canvas文字 | `2482b8 → 1bd880 → 19e64c`，已有整段、多atlas范围 | 与父UI语义一致；世界标记中的文字不直接变成固定屏幕文字 |
| 对白字幕 | `2a4370 → 2470ac → 246c58 → 1bd880` | 原生入口已采集，实测尚无真实glyph；仍需对白场景 |
| MaterialTile | `3a4d3c → 3a5548 → 3a514c → 15a420 → 159fcc` | **独立通道**，现有Material包装只有观察，没有Tile/Text receipt覆盖 |
| 任务/队友等世界标记 | `UpdateHUDLocators`依赖相机位置和对象可见性；部分投影入口已定位 | 世界锚点、绘制scope、对应帧和逐眼投影仍须认证 |
| 暂停/设置UIScene | 类型和状态已知 | 实际native绘制入口、目标与合成顺序未认证 |
| 受伤等后处理 | 世界最终输出路线已知 | 不能将整段后处理按平面HUD处理 |

Material新证据：20字节render item、112字节payload、36字节绘制记录，queued consumer为 `15a420`（command vtable RVA136c5d8的+8槽，返回36）。它与Tile/Text的208字节payload、48字节顶点不同，不得复用后者布局解析。`159fcc` 的实际GPU绘制/矩阵仍待跟进，本轮没有启用新的运行时修改。

后续定向反编译已追通 `159fcc` 到GPU：每条记录构造四个32字节顶点，位置为float3；非索引路径DrawInstanced返回 `1af345`，索引路径DrawIndexedInstanced返回 `177c4a`，均可复用现有D3D detour，但必须新增精确Material语义/command/target/ticket关联。首次可接单条纯屏幕HUD记录，原consumer调用一次，逐眼重放实际GPU draw；不能将会销毁payload的 `15a420` 调用两次。初始depth state为不测试/不写深度，透明分支仍保留原材质行为。详细指令和限制见 `material-ui-gpu-route-contract.json` / `material-ui-gpu-route-static.json`。目前只有路径证据，尚未启用Material双眼修正。

`WarHUD.DrawHUD` 已反编译到 `UpdateHUDLocators`、`DrawObjectives`、`DrawHealth`、`DrawActions`、`DrawDownedButNotOutTeammates` 等分支。`UpdateHUDLocators` 读取玩家相机、调用locator更新和可见性更新，说明父级PostRender中的内容并非全部固定屏幕UI。当前kind3父scope继承不足以证明适用共同平面；后续须在批量认证屏幕布局scope时排除或单独处理世界相关分支。既有HUD平面试验不等于这项风险已修复。

下一批实施按公共通道推进：先为已知屏幕HUD、准星、世界标记建立明确分类；复用Tile/Text消费端并补Material消费端；补UIScene显示切换；对同一场景帧做逐眼投影，保留文字排版、裁剪与提示时序。没有已认证的一处Hook能直接修正所有UI。全局索引可复用，原生实现只对上述缺口继续定向反编译。

## 已确认的绘制层次

原 `172741 → 19e753 → 19f2f3` Canvas GPU 路线将同一几何分别绘制到左右半宽 viewport/scissor，不使用两眼视图矩阵或 FOV 重算 UI 坐标。它让 UI 出现在两眼，并未解决准星与实际游戏瞄准方向的对应。

脚本准星使用 Canvas.ClipX/Y 的中心，换枪布局使用 PostRender 设置的 SafeZoneFriendlyCenterX/Y。普通 HUD 与准星应分别处理：准星需要可靠的游戏瞄准目标和对应帧的两眼投影；换枪 UI 需要保留组内布局，选择一致的显示参考面。不能把全部 HUD 都平移到命中点。

保留运行时快照中验证的入口：

| RVA | 已验证用途 | 当前限制 |
| --- | --- | --- |
| `1d6ca0` | 共享脚本 VM 执行入口，RCX=this、RDX=FFrame、R8=result | 必须以当前 UFunction/对象身份筛选语义 |
| `1d52b4` | Canvas.DrawTile 包装入口 | 入口处 Canvas 状态不是完整解码参数 |
| `3a4d3c` | Canvas.DrawMaterialTile 包装入口 | 同上 |
| `2482b8` | Canvas.DrawText 包装入口 | 不涵盖对白字幕的独立原生路线 |
| `cb3f00` | Canvas.Project | 使用 Canvas+`a4` 的单一 SceneView，尚未证明对应哪只眼 |
| `1bc05c` | DrawTile 解码后的提交边界 | x/y 已为 Org+Cur，尚未建立语义到 GPU batch 的关联 |

FFrame 的 Node/Object/Code/Locals 分别在 `18/20/28/30`；Locals 不能当作解码后的 Canvas 原生参数。Canvas.Project 经 `ca71f8/31f6b0` 使用 SceneView+`2b0` 的矩阵并除以 w，再映射到 ClipX/Y。该事实不能替代 Canvas.SceneView 与原视图/眼视图的运行时关联证据。

## UiObserver1：可选的语义观察器

实验清单 `tools/vr-launcher/manifest-ui-observer1.json` 保持默认发布/预览清单不变。新进程启动时预留四个 trampoline，进入关卡后重新反射取得 controller、camera、pawn、HUD、inventory、Canvas class、函数脚本范围和字段身份，验证后才启用观察。七个精确 scope 包括准星、武器额外准星、DrawWeaponSlots 和 WarHUD.PostRender。

TLS scope 支持嵌套恢复；未知脚本保留父级语义，已知但认证失败的子级不冒充父级 HUD。三个 Canvas 包装保持原参数、调用次数和返回行为，不修改位置。每个 kind×draw 桶最多 48 行，前 12 次调用后最多每秒两行。停止、禁用 hooks 且在途调用归零后才导出记录。

`latest_built_ticket_hint` 只是全局最近生成序号，显式标记 `ticket_association_verified=0`；不得用它把 Canvas 调用与 GPU 帧或眼睛关联。scope 调用数不是帧数；采样拒绝数也不是所有绘制调用的失败率。高频 VM scope 身份读取仍有开销，观察器不作为发布默认功能。

## 首次实际运行：PID43652

采用 OpenXR Simulator 子进程 runtime override，未更改全局 runtime。启动菜单与关卡共用 XR PID4712；静止场景投影 230.212654 秒、提交 20722 帧、复制 16458 张新场景图，最大提交间隔 22.4514ms。提交帧包含复用，不等于游戏渲染帧率。

- VM 记录 1473639 次，WarHUD.PostRender 接受 16874 次、拒绝 0。
- 准星与换枪 scope 均未触发；所有 Canvas 桶调用数为 0，没有坐标样本。
- Windows 拒绝正常前台激活，未发送开镜/切枪输入；截图显示两眼场景正常且没有活动 HUD。此轮仅证明观察器接入和静止输出，不能排除其他 UI 路线或证明 UI 修复。
- native/XR 正常停止，32 个入口独立核验全部恢复，游戏正常关闭；模拟器输入不计为真实位置追踪验收。

证据保留于 `analysis/runtime/continuous-stereo-43652-20261014-165604-62446fa0.*`、`ui-observer1-43652.audit.json` 和 `ui-observer1-43652-idle-mcp*`。会话报告的 passed 表示现有双目和收尾检查通过，不表示 UI 功能通过。

本地检查：UI 配置 Python 12 项、launcher/navigation 34 项、continuous runner 31 项；UiObserverTest 和 CaptureSelfTest 的常规、continuous、frame-link、owned-prime-handoff 检查通过。实际启动还执行现有 reservation 的生命周期检查。

## 字幕独立路线

静态证据确认 viewport `2c78f4` 调用 PostRender (`3dc864`) 返回后清空 HUD.Canvas，再调用 GetSubtitleRegion (`3d0f0c`) 和字幕管理器绘制 `2a4370`。当前 cue 经 `2470ac → 246c58 → 1bd880` 进入原生文字批次，不经过当前 PostRender scope 内的 Canvas.DrawText。

GetSubtitleRegion 属于 WarGameViewportClient/Engine.GameViewportClient。SpeakLine 排队、subtitle-enabled 标志及 GetSubtitleSize 测量只能作为前置条件，不能证明有对白 glyph 显示。后续需在真实对白时观察该原生路径，并核对两眼位置；当前四个观察 hook 不宣称覆盖字幕。

最小反编译/指令证据保留在 `analysis/subtitle-route-native/`、`analysis/subtitle-route-static/`、`analysis/ui-position-runtime/canvas-project-submission-static.json`。这些本地衍生文件不进入发布包。

## 已有瞄准目标缓存

`WarPC.UpdateHUDActions` 在符合条件的本地 Pawn PlayerTick 中调用，沿原游戏 CamRot 的方向执行已有 8000 单位 trace，并将 Impact.HitLocation 写入反射字段 LastCrosshairHitLocation（当前版本偏移 `136c`）。这条路径用于准星颜色/标记，不能称为最终随机散布或辅助瞄准后的弹着点。

HUD 缺失、特定 ActiveAction、烟雾遮挡均可能在赋值前返回而保留旧值；无武器分支也不能作为可靠目标。后续若复用缓存，必须证明该次调用经过实际赋值，并与场景 ticket 关联。函数返回、缓存非零或数值发生变化均不足以证明本帧新鲜度。此时不额外调用带随机散布状态的 GetAdjustedAim。

证据：`analysis/ui-position-runtime/crosshair-writer-summary.json`、`crosshair-writer-script.json`、`crosshair-field-43652.json`。

## 开镜、切枪及嵌套调用的实际验证

UiObserver1 / PID44988 已成功各执行3秒开镜和DPad右切枪，两眼截图均看见对应UI。该轮投影73.7911015秒、6644次提交、5196张新图，最大间隔19.0163ms，32入口恢复通过。HUD下记录5607次Tile、1175次Text，分别保留33/32行，无采样拒绝；准星和换枪精确scope仍为0。截图已有UI，因此不能再把0解释成“没有实际绘制”。

定向汇编与运行时函数身份随后确认：`1d6f30` 的嵌套脚本分支直接建立FFrame并执行opcode循环，绕过共享VM `1d6ca0`。它把调用者帧存于 `PreviousFrame+38`。实际Canvas当前Frame.Node包含Canvas.DrawIcon或Weap_AssaultRifle.DrawAdditionalCrosshair，其Code都位于相应脚本内。

UiObserver2 用既有三个Canvas包装入口追溯此链，不增加Hook。最多12帧，只有回到已认证TLS根帧才接受最近的精确语义scope；已知但无效的子scope、读失败、循环和未到根均拒绝。根Code允许正常推进，不要求等于入场Code。记录明确区分root_invocation、matched_frame和matched_depth；仍不建立GPU ticket关联。

PID36892 实测：7894次Canvas链分类，拒绝0；GOW3准星892次、步枪额外准星223次、换枪2884次、其他HUD3895次。准星Tile保留18行；换枪Tile/Text各18行；其他HUD Tile/Text各35行，采样拒绝0。已验证DrawIcon到GOW3祖先的depth=1及正确scope归属。长时间等待窗口焦点后实际完成开镜和切枪；两眼截图保存，整轮投影685.649956秒、61693次提交、47553张新图，最大间隔168.9184ms；此数值不是全程无卡顿承诺。正常停止及32入口恢复通过。**坐标仍未修改，不能把分类成功当作UI修复完成。**

证据分别位于 `analysis/runtime/continuous-stereo-44988-20261014-184007-ea0a225a.*`、`continuous-stereo-36892-20261014-185144-e8c30c45.*` 及相应 `ui-observer*-mcp*`；嵌套分发证据为 `analysis/ui-position-runtime/nested-script-dispatch-44988.json`、`frame-nodes-44988.json`。新实验清单为 `tools/vr-launcher/manifest-ui-observer2.json`。

## 同屏幕坐标的双眼投影错误

PID44988 的348个CSV采样记录恒定不对称FOV和两眼单位朝向。截图准星均约处于各眼图像中心。按该轮参数反投影 `(u,v)=(.5,.5)`，左右射线水平向外约15.037°，三维夹角29.44567°，不存在前方共同目标。共同头部正前方向对应左眼 `(.62126,.40975)`、右眼 `(.37874,.40975)`。这些值仅是本轮模拟器数据，不能写死为真实头显修正参数。

计算和像素检测方法、源文件hash、阈值与局限见 `analysis/ui-position-runtime/ui-fov-audit-44988.json` 及 `audit_ui_fov.py`。不依赖latest ticket hint，也没有据此认定瞄准缓存的新鲜度或真实弹着点。

`tools/vr-common/ui_projection.h` 已实现纯数学的 `project_ui_anchor`：将同一个调用方认证的世界点，用冻结的双眼矩阵投到各眼像素坐标。CPU检查覆盖上述FOV反例、不同深度视差、旋转平移、视口外点及非法输入。它尚未改变游戏绘制；目标来源、HUD平面深度和批次/ticket归属由后续接入负责。

## UiObserver3：语义顶点到实际绘制的候选关联

在既有 DrawTile 包装调用前后读取 UCanvas→FCanvas→当前排序元素→BatchedElements 的链，只有确认新增恰好4个顶点时记录区间及字节副本。真实顶点步长为48字节，其中44字节由Tile生产代码赋值，尾4字节不参与内容比较。排序元素按 key 查找；顶点数组重分配后只读取新地址。同 payload 必须仍属于同 item，读取失败、裁剪后未新增、布局变化均拒绝。它不增加 Tile 绘制次数，也不改顶点。

新增唯一 consumer 入口 `19e64c`，经实时字节与命令 vtable 验证，ABI 为 `uint32_t Execute(void* command)`。该函数在内部绘制后销毁 payload；观察器在原调用前取出固定容量候选，逐区间比较当前顶点内容，并用嵌套 TLS 将匹配结果带到既有 `172741` GPU 回调。记录的 ticket 来自该次实际 `frame_link.overlay_pair`，不是最近生成的 ticket 提示。原函数仅调用一次并原样返回，返回后不读取已释放 payload。

候选容器最多16组、每组32区间。重复/冲突/组内溢出保留无效标记直到消费者到达；无法记录新组时整容器停止接受，不静默淘汰。记录包含未知顶点数量和语义混合掩码，不能把缺少区间解释为整批次属于某类 UI。候选 generation 不是分配器身份；字节一致也不证明每次 indexed draw 使用这些区间。当前仍为只读诊断，不能直接据此授权改写顶点。

新增入口使恢复检查上限为33。UiCanvasBatchTest、UiPayloadReceiptsTest、UiObserverTest、UiProjectionTest 以及 continuous ABI 自测通过；默认预览清单未升级。

UiObserver3 / PID9232 在实际各3秒开镜/切枪时记录5742个追加区间、1668个消费者内容匹配，拒绝/溢出0。但新增 hook 在调用栈中插入了一个自有函数帧，旧的三帧相邻检查拒绝了所有 overlay，GPU关联0，最大提交间隔7178.84ms。该轮不能算UI通过；常规会话报告 passed 只覆盖现有收尾条件，33入口确实恢复。证据 `analysis/runtime/continuous-stereo-9232-20261014-191940-51e21904.*` 保留了实际四帧栈。

UiObserver4 对观察模式增加精确四帧认证：`172741 → 19e753 → hook_ui_execute 的 PE unwind 函数范围 → 19f2f3`，并要求同线程 Execute 入场保存的真实返回地址也是 `19f2f3`。原有 target/ticket/viewport 条件继续生效。路由身份与语义候选分开，因此未知图元和文字不会因缺少候选而失去原来的双眼复制。CPU测试覆盖错误函数范围、入口返回地址及顺序；清单为 `tools/vr-launcher/manifest-ui-observer4.json`。

UiObserver4 / PID26980 未完成该路由的实测验证：Win+0实际切回游戏后，模拟器XR状态变成VISIBLE；桥接记录失去focused，native记录6682次inactive，仅构建198个ticket。开镜截图全黑，GPU关联仍为0。该轮应按显示失败保存，不能以退出正常宣称通过。

后续对上传/分配器的交叉核对还纠正了 UiObserver3/4 的44字节步长误判：`333de8` 使用48字节分配，`1720a0` 按48字节偏移/上传，并设置IA stride=48。旧两轮的生产/消费比较都使用了相同错误步长，因此“字节匹配”不能证明实际顶点内容。UiObserver5改为按48字节步进、逐个比较前44字节，并增加带独立padding的测试；旧实验数据不用于坐标修正。

### UiObserver5 实测：PID45216

使用冻结的新构建和模拟器自动测试焦点选项，各执行3秒开镜与切枪，两眼截图均正常。连续投影139.801223秒，提交12585帧、复制10463张新图，最大提交间隔18.103ms；native inactive拒绝0。正常停止、XR退出及33个入口恢复通过，游戏随后正常关闭。

- 5840次Tile追加观察全部取得四顶点区间，前后读取/追加/发布拒绝0。
- 1693个消费候选全部内容匹配，header/内容/返回值拒绝0；容器无溢出、无遗留组。
- 2916组overlay实际记录并提交，拒绝0；语义候选关联到实际ticket：准星464次、换枪699次、其他HUD527次。其余1226次没有Tile候选，不能当作该类UI或据此移动。
- 保留的样本均为单一语义的完整顶点覆盖；该场景没有观测到混合候选，不证明所有武器/菜单都不会混合。单次indexed draw的索引成员及分配器身份尚不由本观察器证明。

证据：`analysis/runtime/continuous-stereo-45216-20261014-193621-c0dcdcff.*`、`ui-observer5-45216.audit.json`、同前缀ADS/weapon截图。实验清单为 `tools/vr-launcher/manifest-ui-observer5.json`。**实际UI坐标仍未修改。**

模拟器原版在预览失焦时发送VISIBLE，现有Mod的focused门限会停止构建新眼图。新增本地测试选项 `XR_SIMULATOR_FORCE_FOCUS=1` 仅改变模拟器VISIBLE/FOCUSED策略，默认关闭；实际OS焦点日志和鼠标释放保留，退出状态不被唤醒。补丁与依赖/构建hash记录在 `tools/openxr-probe/simulator-focus/`。原部署DLL和全局runtime未替换，只给该次XR子进程指定独立manifest。CPU焦点策略82项、CTest3项和5秒446帧预检通过；本轮实际日志未出现OS失焦事件，切换到预览的单次后台请求被Windows拒绝，所以仍不能声称该失焦分支已在实机事件中覆盖。

## 准星目标的最小可实现边界

静态解码确认 `UpdateHUDActions` 的命中缓存赋值占 Script内存位置 `+359..+37f`；包内存储偏移不同，不能直接用于进程。当前 Canvas 链不包含已经退出的 writer 帧，共享VM入口又可能被嵌套脚本绕过，因此现有样本不能证明该tick执行了赋值。若使用有限深度命中点，需认证实际赋值及owner、tick、scene ticket，不以旧缓存非零或变化替代。

较小的方向方案是将同 ticket 的原游戏视图中心射线分别投影到双眼，作为“原游戏视图方向准星”。这能避免不对称FOV下硬放两眼图像中心导致的方向分离，但不提供近处收敛、枪口偏移或辅助瞄准后的真实弹着点；原视图方向与实际瞄准仍须核对。普通HUD应独立使用共同布局平面，不跟随命中缓存。

## UiObserver6/7：索引、冻结矩阵与整段文字

`ui_index_membership.h` 在 Execute 入场保存当前36字节segment和uint32索引副本。成员由 `minimum + index` 判定，不用上传区间的交集替代。最多64个segment、8192个索引；零draw也按原生循环推进，核对完整五个DrawIndexed参数，原调用结束后确认数量。原生1720a0的静态循环已核实每segment按序产生一个172741回调；相同参数本身不是可区分的身份，不搜索相同数量的后续segment。大数组使用有界heap，失败只丢弃观察。所有完成检查只读自己的副本，不读原调用已经析构的payload。

场景构建时保存原游戏view/projection和实际双眼矩阵，随后在同一Draw/Present顺序锁内随ticket原子保留、退休。UI只查询实际overlay_pair的ticket，不取latest pose；缺失/冲突不会升级为成功。`ui_plane_projection.h` 的纯数学测试涵盖原相机姿态、非对称FOV、深度视差与转头后的projective mapping。一般情况下viewport缩放平移不能精确表达整个平面；这个helper尚未修改GPU。

UiObserver6 / PID14964取得1736个完整索引序列，但用户随后说明自己用FancyZones调整了窗口，缓冲从3840×2160变为3842×2005且换了资源。固定尺寸检查因此拒绝，overlay和UI矩阵样本均为0，最长提交间隔185455.2ms。常规收尾passed不代表显示通过。焦点一直有效、33入口恢复且正常关闭。最初把尺寸变化归因于Win+0不准确，用户说明为此次根因证据；报告保留为负实验 `analysis/runtime/ui-observer6-14964.audit.json`。

UiObserver7为文字新增整段range receipt，保留原quad适配。每payload最多32段、1024顶点，使用独立有界heap副本；长文字不会按32个glyph截断。现有DrawText包装函数围绕一次原调用捕获全部item的新增范围，支持多字体纹理和重分配；嵌套Tile/Text不重复记录。字幕在精确入口2a4370、caller return2c9375单独收集direct FCanvas，原RAX、rectangle与排版不变。多atlas调用在共同锁内全部发布，否则整次丢弃对应payload候选；失败不改变原文字。

PID37184在稳定尺寸下完成3秒开镜和3秒切枪，双眼截图均有场景、图标和换枪文字。实际结果：

- 持续206.034605秒，18546次投影提交、14081张场景新图，最大提交间隔17.8515ms；本轮无尺寸变化、inactive拒绝0。
- 5301个Tile追加区间，以及1112段文字、13344个文字顶点，生产/读取/发布拒绝0，容器无溢出或遗留。
- 2650个消费者全部内容匹配，2650个索引序列完成，2650组overlay实际执行，均无拒绝；gpu_missing_context=0。保留的UI行全部关联到对应ticket的冻结几何。
- 字幕入口14497次均得到空delta；该场景没有字幕glyph，不能以入口命中声明字幕显示已验证。
- 正常停止、XR收尾与34个入口恢复通过，游戏正常关闭。12组相关CPU测试及continuous ABI自测通过。

清单为 `tools/vr-launcher/manifest-ui-observer7.json`，DLL SHA256 `58438c20230469061974701ee97266a74a6c364c6bd3c69e4235734df13121c0`；证据前缀 `analysis/runtime/continuous-stereo-37184-20261014-213300-8d5d594c`，摘要 `ui-observer7-37184.audit.json`。代码检查点341a800（索引/矩阵）、6aaff64（整段文字/字幕）。本轮仍未修改坐标，也不是物理头显测试；一个场景没有unknown/mixed不证明全游戏均如此。

## 下一步

实际GPU回调发生在顶点上传之后。完整位置修正需要每眼独立VB副本或已认证的GPU变换；不能在回调中改原CPU数组并期待已上传顶点变化。顶点stride48，位置float4在0..0f，UV/颜色/hitproxy与padding需保留；每眼VB需跟踪并恢复原IA绑定，GPU fence完成前不能释放。普通HUD与整组文字使用共同平面；准星需关联原游戏实际目标/赋值时序，不用过期命中缓存。

另需在有对白的场景验证真实字幕glyph和双眼位置。窗口resize恢复不能只清掉拒绝标志：绑定、共享纹理、接收端图像与frame ticket都需要新的资源代次及有序交接。现有固定尺寸版本会停止新图，当前不宣称具备缩放恢复能力。定向计划见 `analysis/resize-recovery/architecture-plan.json`。DLSS/DLAA的三个引擎拦截点与菜单Apply返回规则已核对，仍未启用运行时锁定，不能把启动INI关闭当作完整锁。

## HUD 平面控制首次运行（2026-10-14）

e5ede89 / UiHudPlane1 / PID46800 首次启用独立 70°、2 米 head-locked HUD 平面。模拟器运行 791.65065 秒，71243 次 projection、54971 次复制，最大提交间隔 68.5128ms；2984 次计划和实际双眼 overlay，sequence_fault=0，34 个入口恢复且游戏正常关闭。1492 段文字被覆盖；原生字幕 55393 次均无 glyph，准星没有样本。

两张操作期间截图均能看见双眼场景，但没有清楚可见的换枪 UI/文字；不能仅凭 applied 计数宣布视觉修正成功。已记录 0.1 米水平/0.08 米前后平移及 yaw/pitch/roll 非零的模拟器姿态，场景随之变化；HUD 可见性、平面位置及实际字幕仍需定向调查。证据 `analysis/runtime/ui-hud-plane1-46800.audit.json`，不计为真头显验收。

截图限制已查明：MCP D3D12截图读取模拟器窗口预览RT（本轮1734×257），随后压到1280×189；Fill Window模式将完整眼图压扁进预览，并非原始1920×2160眼图读取。截图不足以证明HUD消失；不能将这种预览变形归因于游戏投影。单眼MCP也只裁切预览，不能恢复丢失像素。后续需尺寸正常的预览或原始GPU读回才能判断小字体位置。

准星缓存赋值的纯辅助模块已加入 `crosshair_assignment.h`，验证 execLet 的精确字节码、属性/owner 链、原调用后完成的 Vector 复制，包括同值赋值。独立 CPU 测试通过；尚未安装赋值 hook，也没有与玩家 tick/实际场景 ticket 关联，不能据此声称瞄准点已经修复。

## 原始 GPU 像素与 Material 路由（2026-10-15）

`manifest-ui-material1.json` / PID51800 增加默认关闭的 Material UI 消费观察器（第35个入口）。只检查有界追加、不可变payload副本和实际消费回调，不修改材质顶点或GPU状态。本轮已发送开镜、切枪，6574次projection、5036张新图，正常停止及35入口恢复通过；Tile/Text路径有准星、换枪、HUD样本，Material计数全部为0。因此当前样本应沿Tile/Text处理，不能宣称整个游戏不使用Material，或已完成该分支的运行时验证。

原始GPU快照现在保留完整3840×2160 SBS像素，可在明确开启诊断的会话内按需请求，每次会话最多8张。PID22904 / Pixels2 首次在操作期间成功保存独立文件，清楚看见开镜准星、弹药HUD及换枪图标/数字。换枪与弹药两个区域的亮色像素匹配均在右眼局部偏移(-493,0)附近最佳，与本次HUD平面预测(-493.356,0)一致；这只是当前场景的像素证据，不代表所有UI/真头显体验已通过。图像与量化记录：`analysis/runtime/ui-pixels2-ui-alignment.json`、同轮`.gpu.ppm.1.ppm`。准星仍处于每眼图像中心，未应用非对称FOV下原游戏方向的投影修正。

Pixels2 的快照日志在acknowledge之前读取serial，记录的是上一次已确认图像；图像及ticket有效，serial不能用于同帧关联。Pixels3修正为ack成功、GPU fence完成后读取，并增加保存完成的单调时钟时间。PID424 / `manifest-ui-pixels3.json` 实际采到3张完整图，均与image-ticket日志的ticket及serial匹配；其中两张保存时间落在开镜、切枪动作期间。该轮5667次projection、4137张新图，正常停止及35入口恢复通过。首张Pixels1原实现是在保存时读取serial，未受Pixels2这一回归影响。三轮审核文件为各session前缀加`.ui-pixels-audit.json`。

下一项先校正纯准星segment的原游戏屏幕中心方向，使用同ticket冻结矩阵，以无限远方向投射到每眼后平移viewport；保持原图形尺寸。该方案不等同于近处命中点、枪口偏移或真实弹着点修复。字幕仍需有对白的实际glyph样本，普通PostRender语义也不能自动推广到所有世界标记。

## UiReticle1：原游戏方向修正已通过像素核对

`manifest-ui-reticle1.json` / PID47092 首次开启默认OFF的 `GEARS_UI_RETICLE_DIRECTION`。仅当当前原生segment索引全部属于同一次Tile准星调用、独立顶点副本位置有限且z=0/w=1、包围盒居中误差不超过1像素且宽高各小于画布1/3时启用；额外准星/整屏瞄准镜/混合/未知批次不自动接管。Extent在原consumer调用前按实际索引保存为值，之后不读取已销毁payload。使用实际overlay ticket的source view/projection及双眼矩阵，以w=0映射屏幕中心方向，只平移viewport，裁剪scissor并恢复原状态。

实际458次准星计划及双眼draw通过，居中/矩阵/投影拒绝均0，其他HUD 2134次平面修正，sequence_fault=0。10775次projection、8012张新图，正常停止、35入口恢复及游戏正常关闭。原图开镜ticket650/serial643与XR记录吻合；阈值提取的准星白色像素框中心由基线(959.5,1079.5)/(2879.5,1079.5)，移动到(1192.5,884.5)/(2646.5,884.5)，与计算位置(1192.812,885.062)/(2647.188,885.062)在像素栅格范围内一致。四个白色像素框均为60×104像素，原图同时能直接看见正常准星形状。

CPU测试以 `/W4 /WX /UNDEBUG` 编译，覆盖非对称FOV、旋转、平移不影响方向、索引归属、混合来源/多个root拒绝、失败不修改输出；HUD与索引回归、continuous ABI自测通过。代码已只读审查。DLL SHA256 `185161b70a9fa00ce755644f014382fef269d46ab00e558897d7053eafdd4ef4`，证据 `analysis/runtime/ui-reticle1-47092.audit.json` 和原始GPU图。这证实当前场景FOV方向偏移的修正，不证明近处命中收敛、枪口弹着点、所有武器/菜单/字幕或真实头显体验。

## 首次非空字幕生产记录（2026-10-15）

PID53760正常推进开始房间、走廊及庭院入口，运行727.19秒、65225次projection、45428张新图，正常停止与35入口恢复。字幕统计桶source=1（receipt枚举为NativeSubtitle=2）首次取得559个非空调用、1845段、110312顶点；生产没有delta或publish拒绝。这里是精确2a4370入口内的文字追加证据，不是文字像素已显示。

完整3840×2160的6张快照未截到对白。晚期只读队列观察器在源getter/selector字节与历史快照匹配后，通过getter的RIP相对寻址取得global RVA1c7dbc8；稳定的manager header为count=0/capacity=0。该观察比进入走廊晚约181秒，不能由此否定此前非空glyph。随后PID46520在移动前开启60秒watch，仍采到空队列；该轮停止日志却有65个非空字幕调用、129段、2820顶点，说明当前队列时序与截图覆盖仍不足。实际画面还显示玩家在庭院门边进入掩体，不能把发送前进键当作已经通过该位置。

当时GPU receipt上下文仅保留HUD kind，丢弃了原NativeSubtitle source，不能拿任意kind4行当作字幕ticket。证据：`analysis/runtime/continuous-stereo-53760-20261015-015852-f4f5fcfa.*`、`continuous-stereo-46520-20261015-021439-a72cff7f.*`、`subtitle1-queue-walk.jsonl`、`subtitle2-queue-walk.jsonl`。

GraphicsLock1已增加`ui_segment_sources.h`，按真实复制索引保留source/root范围，拒绝混合、未知、过期代次及索引不匹配；独立16条字幕GPU记录保留ticket、list generation、execute sequence、QPC和双眼replay/命令完成状态。`UiSegmentSourcesTest`以/W4 /WX编译并通过，未增加原生hook或修改顶点。PID53840本轮菜单测试字幕GPU calls/rows均0，尚未取得真实字幕GPU命中，不能据此宣称可见性通过。

现有快照event没有指定ticket，GPU done fence仅为复用ack。后续精准采图应将已提交的字幕list与Present发布的相同ticket关联，接收端只保存指定ticket/session；越过则明确miss，不能自动用下一帧冒充。最后仍需检查文字像素与双眼位置。

## 2026-10-15：独立输出尚未接通Canvas/UI

Sustain1、Menu1、Reload1已验证独立Low场景持续输出和部分状态切换，但不能沿用上面的旧backbuffer UI验收结论。当前源代码有三个明确断点：

1. `capture.cpp::hook_family`接收FCanvas；`try_submit_pair`只把copied family的`+0x10`场景目标换成owned adapter。后续汇编核对确认，renderer第三参数虽然命名为canvas，实际是从FCanvas+0x18加载的值，不是FCanvas对象本身，不能替换为owned adapter。Canvas目标和异步持有关系需要独立验证，`native_owned_scene`状态明确为`canvas_redirected=0`。
2. `hook_draw_indexed`的双眼UI replay要求目标是已知swapchain buffer，再用`frame_link.overlay_pair`取关联。owned场景使用独立tracker和`image_tickets`；现有HUD/准星规划却查询`stereo_ticket_store`。仅放宽buffer判断会缺少同ticket的命令表代次、目标、队列和冻结姿态/几何关联，不能作为修复。
3. `native_owned_scene_copy`在场景消费返回、renderer销毁及两个世界视图提交后即可发布。若Canvas稍后消费，必须先取得该ticket的Canvas结束及真实命令提交边界，或明确的同ticket无UI证据，再复制/发布和推进ticket。当前Trial要求恰好两个世界draw；UI应独立记作overlay，不能混入世界draw计数或把未知写入当成成功。

下一步定向复用资料库中的family `0x3a148c`、renderer `0x116250`、已观察UI execute `0x19e64c`及其调用上下文，确认Canvas目标持有、排队与结束边界，再实现独立目标UI关联。以上来自当前Mod源代码审查，不等于新的原生反编译结论或GPU证据。先解决关联/时序，再做准星、换枪UI、菜单、字幕像素验证；保持已有票据身份和姿态时效约束。

## 2026-10-15：Canvas到场景票据的定向运行验证

复用完整资料库与既有汇编，仅补查Flush和析构调用片段，没有重新批量反编译。历史快照SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`、模块基址0x140000000；仍与磁盘EXE区分。IDA在`analysis/ui-position-runtime/owned-ui1-ida/snapshot.i64`副本工作，lease已关闭，原库SHA256保持`66d17edbf21bf81f0fe5cf740111e738a0308ac9228f7576fe39f460baad1813`。

确定的ABI/字段关系：family入口RCX为FCanvas；FCanvas+4是目标接口，+0xc是可选目标；batch producer `0x117430(item,FCanvas)`把item+8的payload交给48字节命令，命令+8是payload、+0x20/+0x28是目标。`0x19e64c(command)`返回48并释放payload。Flush `0x1172c4(FCanvas)`通过item虚表分派；析构中的Flush返回地址为0x2a6b9f。Reload1的四份运行记录也确认FCanvas+4等于原family+0x10。证据为`owned-ui1-static-contract.json`及该目录的逐字节校验汇编；偏移不是跨版本通用定义。

新增可选`GEARS_OWNED_UI_ROUTE_PROBE`，只观察Flush/producer两处入口，复用既有Execute观察器；不更换目标、不改变原始调用次数或发布条件。固定容量ledger按Canvas、线程、明确选中的ticket关联；payload在生产前登记、转交后检查清空、Execute返回后只使用值拷贝。闭合后仍保留待执行/待提交记录，重复payload、错目标、旧ID和容量耗尽拒绝关联。

Ui1首次安装失败：新增两处Hook时漏改resize编译分支的容量，安装在应用队列前拒绝，未取得运行证据；已修复为各基础容量加2，失败构建与清单保留。Ui2 SHA256 `2f8c3c64537b9b6548150706093af76bd2e35869ea85eb65e49a9ff8601047d9`随后进行了两轮模拟器测试：

- PID52888：40.092秒、1,364个owned场景、1,362次发布、1,360次XR复制。Canvas关联与析构闭合各1,364次，但生产/执行均0，首32个Flush样本为空。首32个空UI场景均在消费前闭合，不能把“发布过早”宣布为故障根因。独立入口恢复检查被协调器提前关闭游戏打断，没有独立恢复通过结论；游戏正常退出。
- PID38236：通过内存状态确认进入`DeltaPause-Campaign-Host`、焦点`LBtnResumeSingle`，随后Esc恢复。40.134秒、1,280个owned场景、1,279次发布、1,277次XR复制；11,187个关联UI批次的生产/转交/执行/完成数一致，失败、错目标、payload冲突、容量溢出均0；全部1,280个cycle闭合并回收。另有11,168个producer不在选中Canvas scope中，其Execute没有关联，明确不算成功批次。61处入口独立核验全匹配后游戏正常退出，无强杀。

这证明了本场景暂停菜单的CPU批次归属，不是独立输出UI像素或位置验收。Ui2只保留首32个cycle，恰好全为空，未保存后续非空批次的逐cycle时序；不能从汇总推断它们都先于/晚于发布。源码随后增加全程时序计数和独立首32个非空cycle保存，Ui3仅编译/CPU测试，尚未实机运行。ledger测试1,569项通过；FrameLink、payload、index、owned scene与ABI相关回归通过。

运行审计：`analysis/ui-position-runtime/owned-ui2-52888.audit.json`、`owned-ui2-menu-38236.audit.json`。静态、运行证据及脚本均留在ignored的analysis。默认玩家清单不变，native资源仍保留到进程退出，launcher的incomplete没有消除。下一项实现是把关联批次导向owned目标、接入owned tracker的双眼overlay与同ticket姿态/几何，并把Canvas闭合、全部关联Execute完成及真实GPU提交作为复制前置条件；Material及其他未认证路由不能自动纳入。之后再分别核对准星、换枪提示、菜单、字幕像素。

## 2026-10-15：暂停菜单已绘入独立双眼输出

增加默认关闭的 `GEARS_OWNED_UI_BATCH`，依赖 owned UI 关联和 owned XR。先查询知识库的 Execute `0x19e64c`，复用已保存汇编，无新增 IDA 会话。安装前校验当前进程两个完整函数块 `19e64c..19e791`、`8bee1e..8beec0`，合并 SHA256 必须为 `1b804155ae4801e5c554227420cf988fb994138d305d6d11c349295410f8be25`。这把目标替换限定在已检查的运行代码，不能沿用到其他游戏版本。

执行关联批次时在栈上复制48字节命令，仅替换 `+0x20` 目标；原 FCanvas、原排队命令和 payload 不改写。原 Execute 恰好调用一次并释放原 payload。绘制时用 owned tracker 的同 ticket、命令表代次和资源关联，执行左右眼 overlay；UI 不计作第三个世界 draw。owned 图像票据同时保存原镜头矩阵及双眼几何，准星和普通 UI 不再查询旧 backbuffer 票据仓库。

发布必须同时满足：本 Canvas 已析构闭合、完整 Flush 清单的所有支持批次已转交且 Execute 完成、世界双眼已提交，以及相关 D3D 录制已真实提交。GPU 等待查询不消费或破坏 tracker 证明，随后仍执行原有资源/队列/状态检查。等待时不再创建未关联的 backbuffer 双目副本。当前仅接通已认证 batch；Material/未知项触发实验路径停止，不视为修复完成。

HUD 增加独立源宽高比，方形单眼输出仍保持原 Canvas 的16:9形状；准星规划也支持源像素比例并保留原屏幕中心方向算法。无语义标签的菜单 batch 使用同票据的70度、2米头锁定面板。有限距离命中、所有武器和 canted-eye 支持没有因此获得验证。

- Route1，PID25320，DLL SHA256 `67e4ccdbae25417f6bb4c5651ec61c6e9e16d81353e6b8e29aba50a2c6f615d4`：40.918秒，1,307个owned场景、1,306次发布、1,303次XR复制、3,675次projection；10,963个批次完成双眼重绘。打开暂停菜单后返回关卡；667个非空cycle，提前发布和路由错误均0。该构建的 `observed_only=1` 是旧日志文字错误，实际 `owned_ui_batch enabled=1`；后续已修正。
- Route2，PID47320，DLL SHA256 `78973d4a5addeb0386d34a55afc1fed8eefae1bec097b7f03860e25ef0e1184f`：加入等待期间不创建额外双目副本，保持暂停菜单至停止。70.978秒，3,116个owned场景、3,114次发布、3,111次XR复制、6,382次projection，最大提交间隔55.694ms；48,279个批次完成双眼重绘，2,414个非空cycle，提前发布、错目标、路由故障和未知世界draw均0。

两轮都通过61处入口独立恢复核验，然后正常关闭游戏，无强杀。Route2的第1,800个owned场景（ticket1801）保存GPU fence完成后的4096×2048原图。直接查看可见左右眼都有暂停菜单选项、任务标题及底部提示，字形没有明显拉伸。图像与场景状态相互印证；它只证明该次暂停菜单可见，不代表全UI或真头显舒适度验收。

证据：`analysis/ui-position-runtime/owned-ui-route1-menu-25320.audit.json`、`owned-ui-route2-menu-47320.audit.json`；原图 `tools/native-capture/build-owned-ui-route2/owned-scene-47320-late.ppm`，审阅缩图在 `analysis/ui-position-runtime/owned-ui-route2-47320-menu-review.png`。所有衍生资料和构建均被忽略，不加入发布包。

FrameLink 17,455项、ledger 1,665项、owned scene 179项及HUD/准星比例、viewport恢复、Capture ABI/family测试通过。Route2运行后的源码另补齐Execute非48返回时立即停止，原先已阻止发布但只等待超时；Route3用于编译该差异，不能把Route2运行记录当作Route3真机验收。默认玩家清单不变，资源仍保留到进程退出，部署退出流程尚未完成。下一步是使用相同owned路径分别验证开镜准星、换枪和字幕，随后处理其他菜单/Material路径。

## 2026-10-15：独立输出的开镜、换枪画面与字幕测试边界

Route3 / PID50132 / `manifest-owned-ui-route3-ads.json` 已实际运行40.107秒，3,605次projection、1,739次XR新图复制，最大提交间隔55.429ms。1,743个Canvas cycle全部闭合回收，其中314个非空；1,768个批次完成双眼重绘，普通平面1,480、准星288，路由故障与提前消费均0。3,540个索引上下文全部完成，无sequence fault。61处入口独立恢复通过后正常关闭游戏。

通过原生菜单状态确认在关卡内，以用户指定Win+0聚焦，虚拟手柄分别开镜和显示换枪菜单，每段3秒，结束均恢复中性。接收端原始GPU图ticket719、887分别落在两个操作期间；4096×2048原图中双眼均可见准星、换枪图标/文字及弹药HUD，形状初步正常。摘要 `analysis/ui-position-runtime/owned-ui-route3-ads-50132.audit.json`，缩图 `owned-ui-route3-ads-50132-review.png`。本轮没有字幕，不能据此推广到对白；准星仍是屏幕中心方向修正，不是有限距离弹着点验证，也不是头显验收。

随后增加默认关闭的 `GEARS_OWNED_SUBTITLE_PIXELS`。只有真实索引归属为NativeSubtitle的owned双眼draw才登记候选；最多3份，按ticket/session/pose sequence/资源代次严格匹配，在完整UI提交认证后、下一次写入前，向同一DIRECT队列排入读回。文件仅在GPU fence完成后保存，过期候选明确miss，不以随后图像替代。这是原生owned纹理证据，不能单独证明XR接收或头显可见性。

同轮修复两个完整性缺口：未提交的owned overlay若被命令列表Reset或重新Create丢弃，tracker失去发布认证；已提交列表和无关资源不受影响。语义索引cursor必须先finish，owned ledger才允许完成，不能在后续发现序列失败前把ticket置为可发布。不存在语义context的普通菜单batch仍可走已有认证路径。FrameLink新增覆盖后17,529项通过，ledger 1,665项、索引membership、owned scene和Capture ABI/family相关检查通过。

Route4 / PID51376 的字幕路线是失败实验：脚本由菜单状态和camera telemetry控制短距离移动，进入走廊后owned UI first_fault=2，保护停止；2,162个场景在此前消费，字幕候选0。该构建的首32份Flush样本均为空，不能进一步区分清单读取失败、Material/未知类型或ledger拒绝。没有证据将此归因于字幕本身或先前全部黑屏。61处入口恢复通过，随后正常关闭。负实验摘要 `analysis/ui-position-runtime/owned-ui-route4-subtitle-51376.audit.json`。后续构建只补存第一份失败清单及具体拒绝原因，继续定位实际阻断，未放宽发布门槛。

Route5 / PID50488复现并缩小原因：首次拒绝ticket2748、caller RVA1167f4，清单读取成功，total=4、batch=3、material=1、other=0，reason=2明确为未支持Material，而非读取/未知类型/ledger拒绝。此前2,746个场景发布，1,728个普通批次双眼重绘；3,474个索引序列完整，字幕仍0。已有Material观察器实际published=9、claims=8、content_rejected=0、bad_return=0，但gpu_candidates=0、gpu_rejected=8，缺口位于quad或draw参数认证，尚不能称为Material已正确绘制。61处入口恢复、游戏正常关闭，负实验见 `owned-ui-route5-subtitle-50488.audit.json`。Route5及功能关闭的默认分支均编译通过。

资料库命中ui-05及既有Material静态合同，无需重新全量分析或打开IDA。该路径使用独立36字节命令、112字节payload与32字节GPU顶点，不能套用普通Canvas批次的48字节命令或顶点解释。下一项先记录现有8次GPU拒绝的具体参数/步骤，并核对真实IA、目标和同ticket关系，继续保持未认证Material阻断发布；字幕视觉验收仍未完成。

## 2026-10-15：Material候选认证的标志位假设已修正

Route6 / PID36168在保持Material阻断的条件下保存首4份GPU拒绝值副本。15个consumer均claim成功且正常返回，GPU候选全部被metadata检查拒绝。保留的4份记录均为非索引DrawInstanced，caller RVA1af345，4顶点/1实例、起始0；stride32、first0、primitives2、min0/max3、proxy和expected_proxy相等。唯一不符旧判断的是flags=9237（0x2415），旧代码要求整个16位字段等于0x411。目标仍是3840×2160原生backbuffer，不能当作owned双眼UI。

复用资料库的`0x14015a248`伪代码和既有汇编，并从SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`原始历史blob逐字节核对：RVA15a3ca为`25 9d f5 ff ff`（AND EAX,0xfffff59d），15a3d6为`0d 11 04 00 00`（OR EAX,0x411）。等价简化为`(old & 0xf18c) | 0x411`；初始化保留其他位，不能要求全字段相等。实际固定位条件为`(flags & 0x0e73)==0x411`。两段指令现在也进入实际进程的binding校验；新增测试覆盖现场值和16位逐位翻转，源blob验证脚本对全部65,536个低16位值检查等价性。最初手算AND地址偏2字节，由独立字节断言捕获并在Route7运行前修正，没有运行错误地址构建。

Route7 / PID49384使用修正后的只读条件，10次Material生产/claim/Execute正常完成，10个quad/draw候选通过，GPU拒绝0；10条保留行均为kind3、非索引RVA1af345、command_completed=1。所有ticket_candidate仍为0，mutation_allowed=0，IA上传/分配归属及Material投影未认证。owned清单仍拒绝Material，不能把候选通过等同于该UI已经双目显示或首版稳定完成。

两轮停止后均通过61处入口独立恢复，再正常关闭游戏。Route6/7清单、源文件和DLL哈希分别冻结；CPU Material测试与DLL ABI通过，功能关闭分支编译通过。摘要 `analysis/ui-position-runtime/owned-ui-route6-material-36168-material.audit.json`、`owned-ui-route7-material-49384-material.audit.json`，字节证据 `material-flag-mask-bytes.json`。没有新开IDA库、重建全量语料或截图导航。

下一步围绕Material producer `3a514c`、consumer `15a420`已有合同，补齐当前真实目标/上传IA/Canvas完成与owned ticket关联，并保留consumer一次调用和原资源生命周期；不能沿用Tile命令`+0x20`替换，因为Material该偏移是flags，逻辑目标在独立SceneView所属family中。完成该路径后再继续对白字幕和其他UI验证。

## 2026-10-15：Material 生命周期基础与定向构造验证

owned UI ledger 已区分 Batch 与 Material，分别核对预期数量、生产、转交、执行和完成。Material 必须在 producer 关联时明确给出预期逻辑目标，consumer 不能反向决定目标；旧的 `transferred(true)` 只证明 Batch 清空，不能用于 Material。新增 `MaterialDispatched` 只表示生产路径完成转交，不表示 GPU 绘制成功。类型间共用 payload 唯一性检查，地址复用、错类型、错目标、重复完成和失败均不能使 ticket 可发布；允许同步 consumer 先完成、producer 后返回的顺序。现有普通批次调用保持兼容。

复用资料库命中后，只在私有 IDA 副本中补取 producer、上传函数和两个构造函数的相关汇编范围，逐指令对照历史 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`。确认 `11739b` 将原 FCanvas 作为 RDX 调用 item producer；`3a514c` 读取其 `+4` 逻辑目标、`+18` 模式和 `+20` 代理，Material payload 在转交后没有 Batch 的清空约定。`3a574c→15e428` 将目标宽高写入 view `+50/+54`，将原 identity view 矩阵和 payload 保存的矩阵分别原样复制至 `+70` 和 `+b0`。这是构造数据流证据，不等于最终 shader/viewport 或实际 OM 绑定已认证。

原始 `3a52e3` 指令字节 `48 8d 05 86 47 75 01` 按 RIP 求值得到矩阵 RVA `1af9a70`；该 raw IDB 的显示标签为 `1af8a70`，不能据显示文字改动实际地址。证据保存在忽略目录 `analysis/ui-position-runtime/owned-material1-ida/material-producer-upload.json`、`material-view-ctor.json`。工作副本 lease 已关闭，复制来源 IDB 的 SHA256 前后保持 `b47742b1c7d0d97201432d395283c853571193f1fdaffd3ed9c6e290b71ae55fb`。

新建 `build-owned-material-ledger1` 编译通过，ledger 实跑 1,877 checks，Material 测试及 DLL ABI/family 自测均通过。未启动游戏，不借用之前构建的运行记录作为本次 GPU 验证。

此提交只准备类型化生命周期，尚未启用 Material 输出路由；现有清单仍会阻止未支持的 Material 发布。接下来将 producer 的目标/票据关联接入 consumer，并用实际绘制目标和矩阵验证决定最小双眼绘制改动，随后继续字幕路径。没有将静态资料库覆盖率当作 V1 完成率。

## 2026-10-15：Material 已进入实际双目输出，下一阻断是绑定跟踪生命周期

新增默认关闭的 `GEARS_OWNED_UI_MATERIAL`，要求 owned Batch 和 Material observer。producer `3a514c` 将原 FCanvas/逻辑目标与 typed receipt 关联；consumer `15a420` 仍执行一次并按原流程销毁 view/payload。未改原生目标指针或矩阵。只对同票据、实际 OM 已绑定 owned 4096×2048 纹理的非索引 Material quad 执行双眼 viewport/scissor 重绘；原状态随后恢复。仍不支持任意 indexed/world/projective Material。

独立 upload scope 核对 `1af054` 的 context、拓扑1、2 primitives、32字节stride及确切128字节quad来源；Draw处再核对同一context的实际命令列表、stride32、拓扑5和quad内容。producer和上传到draw范围均在安装前进行历史字节SHA256比对。ViewPlane只认证原SceneView的identity view、原矩阵逐字节复制一致、有限标准Canvas XY/W映射及最多1源像素偏移，不改源数据。GPU目标/命令列表代次/票据证据仍分别检查。独立复核确认5参数upload ABI及原函数调用次数，修复了局部 `simulator_operator` 遮蔽；首轮实际全局开关也是0，因此该修正没有冒称改变首轮结果。

- Route1-a / PID38532：40次Material生产、执行、完成、原生上传及双眼重绘均通过，Material faults=0。2,635个owned场景消费，普通Batch first_fault=9终止后续发布，字幕候选0。首次独立恢复工具尚未认识新增两项，拒绝63项清单；补充两个完整范围的独立哈希核验后63处全部匹配，正常关闭，无强杀。更早Route1启动尝试仅因新目录缺配套测试EXE失败，未启动游戏；已补齐启动器所需测试目标。
- Route2 / PID44700：20次Material完整生命周期及双眼重绘通过，Material faults=0；2,699个owned场景消费，1,589个普通批次双眼重绘。ticket2236在UI完成和GPU提交后对同一纹理读回，fence完成后保存4096×2048原图；直接查看左右眼均有武器HUD、字形无明显拉伸。它证明该帧的双眼UI可见，不证明所有Material分支、字幕或头显验收。63项独立恢复核验后正常关闭。

Route2同时取得普通UI的具体拒绝：ticket2701、caller172741、正确调用链，但 `tracked=0`、实际目标和viewport/scissor均无记录；`frame_link_capacity` 中 `target_binding_overflow=96`，list highwater268超过旧绑定表256槽。旧 `frame_target_stats` 还随Present清空，而owned输出可跨越该边界。不能把缺失观察数据解释为实际GPU目标为null，也不能将此认作此前所有黑屏的唯一根因。

下一修复：owned路径使用独立的录制期绑定状态，同步Reset/RS/OM和启动阶段descriptor create/copy，保留descriptor来源，不随Present清空；提交后只回收该批closed列表的CPU绑定，保留owned GPU tracker的generation/执行证据和其他正在录制的列表。必须继续拒绝真实并发容量不足、缺Reset和未知来源。字幕INI已有 `Subtitles=True`，但脚本另从profile setting ID40设置运行时开关；后续需从实际菜单/profile状态核对，不能仅凭INI假定已开启。

本轮编译、ledger 1,877 checks、Material/ViewPlane测试和DLL ABI/family检查通过，启动器要求的附加自测也通过；功能关闭分支编译通过。证据：`analysis/ui-position-runtime/owned-material-route1-a-38532.audit.json`、`owned-material-route2-44700.audit.json`，视觉审阅 `owned-material-route2-44700-review.png`。实验清单单独保存；默认玩家清单不变。V1仍未完成。

## 2026-10-15：独立录制绑定、持续 UI 与对白字幕

owned pixels 路径新增严格录制期绑定表。只有观察到成功 Reset 或首次 open creation 才能建立记录；viewport/scissor/OM 不隐式补建未知列表。使用 canonical COM identity，接收 priming 阶段 RTV 创建/复制，保留 descriptor 来源，不随桌面 Present 清空。原 Execute 返回并完成提交登记后，只退休该批实际成员的 CPU 绑定；其他正在录制的列表和 GPU generation/执行证明保留。Scene、普通 UI、Material、Clear 和 MRT 检查均读取独立来源。真实并发 256 槽耗尽继续明确拒绝。未开启 pixels 的旧 scene-only 路线保持原有跟踪。

严格生命周期测试覆盖 1024 个顺序列表、并行保留另一录制、提交后禁止隐式重建、Reset 恢复、descriptor 保留和第 257 个并发列表拒绝。完整构建、相关 CPU 测试、DLL family/frame-link 自测以及默认关闭分支编译通过。

- Binding1 / PID33460：2913 对 owned 场景，独立绑定 overflow=0；没有触发 UI，输入因焦点校验失败停止。XR 退出等待超时，不能算完整持续或退出验收。
- Binding1-b / PID32724：3158 对 owned 场景；3565 个普通 UI 命令及双眼重绘、348 次 Material 生产/执行/完成/上传/双眼重绘，UI faults=0、绑定 overflow=0、descriptor loss=0。没有在 UI 完成前发布。按计划 130 秒测试窗口停止，但最后计数停滞，XR 最大提交间隔 38188.837ms；不能称全程流畅。
- 字幕 ticket3024、3036 各有同票据 glyph/提交/fence/读回记录，均保存成功。直接查看 ticket3024 的双眼图像，两眼均可见对白“Kim: Who's next!”及武器 HUD；这只证明该帧字幕可见，未验证所有字幕、字号舒适度或真头显表现。

两轮停止后均独立核验 63 项入口恢复，并正常关闭游戏；native owner 仍保留至游戏退出，launcher 的 cleanup 状态均为 incomplete。第二轮 XR 自身关闭完成，未强制清理 XR。用户另报告桌面两幅画面频繁交替，现有采样没有直接捕获该现象；原因未确定，不归因于全部历史黑屏。

冻结清单为 `manifest-owned-binding1.json`、`manifest-owned-binding1-b.json`，共用未重建的 `build-owned-binding1`。审计及源文件/DLL哈希位于 `analysis/ui-position-runtime/owned-binding1-33460.audit.json`、`owned-binding1-b-32724.audit.json`；图像审阅为 `owned-binding1-b-subtitle-review.png`。接下来先关联 anchor identity 拒绝、停止发布和实际状态变化，再完善资源退出、菜单、High、跨关卡及真头显验收。V1仍未完成。

## 2026-10-15：已复现死亡菜单导致双目停止接续

Binding2 / PID12880 为既有 Pawn 绑定读取增加可选诊断，保留原读取顺序和所有拒绝条件，记录前16次失败；没有增加游戏内存读取或新 Hook。CPU测试覆盖带/不带诊断的读取序列相同、空Pawn、六个关联、class/outer和复核期间变化；冻结构建自测及默认关闭分支编译通过。

实际游戏＋模拟器完成2868对owned场景、2391次普通UI和215次Material，UI故障与绑定溢出均为0。首个拒绝及全部16条保留记录均为 `stage=pawn field=controller_pawn actual=0 actual_valid=1`。独立读取同进程确认 Controller.Pawn 为0，Camera.ViewTarget仍指向原Pawn；随后重新认证的菜单读取确认 `DeltaGameover`、焦点 `btnCheckpoint`、consistent_reread=true。这证明本轮为角色死亡、取消占有后，双目守卫正确拒绝旧玩家场景，但现有一次性启动面板没有恢复显示死亡菜单。不能将它归因于全部历史黑屏或用户报告的桌面交替。

停止后63处入口独立恢复，正常关闭游戏、没有强杀；原生Owner仍保留到进程退出，尚未解决正常退休。审计 `analysis/ui-position-runtime/owned-binding2-12880.audit.json` 保存原始记录与DLL/清单哈希。下一步增加显式、可逆的场景／菜单显示状态，保持原场景身份和姿态时效检查；桌面镜像缺口单独处理。本提交只有诊断，不宣称菜单恢复已经实现。

## 2026-10-15：可逆死亡菜单显示实现，实际恢复分支尚待覆盖

新增可选 `menu_recovery` 清单项，仅在已有startup_menu生命周期中启用。启动器复用只读 MenuReader，认证同一PID/创建时间、注册UI树及一致复读后，发布带游戏/owner身份、序号及采样起始时间的64字节原子文件。初期只认证 `DeltaGameover/WarUIScene/input_mode=1` 为死亡菜单，已观察的常驻Crossplay场景为恢复候选；未知、采样失败、过期及身份错误不会授权死亡面板。没有修改 native Pawn 守卫或放宽100ms姿态有效期，也没有增加 Hook。

XR可保留窗口捕获用于返回菜单。进入菜单时清空旧图并建立epoch，使用WGC SystemRelativeTime确认真实捕获发生在epoch之后，缓存最多1秒。菜单状态租期1.5秒，进入死亡菜单后必须再次收到有效游戏候选状态、并取得晚于恢复边界的新鲜已认证双目ticket，才在成功xrEndFrame后完成接续。屏幕平面与projection每帧只选择一种；旧startup-only清单仍在首次成功双目后释放窗口捕获。菜单面板不计作立体世界渲染。暂停、其他菜单及跨Level重新认证仍须后续接入。

菜单时间/协议/状态机、XR接收与退出等8个C++检查通过，Python菜单观察4项、continuous runner36项、launcher35项通过。实际 `MenuRecovery1 / PID22156` 使用冻结Binding2 native及新 `build-menu-recovery1` XR：启动面板2933次后自动接续，261.105秒内8097个owned场景、7475次普通UI、499次Material；owned绑定overflow=0、UI/Material故障=0。XR复制8089张、projection提交23433次，最大间隔112.4373ms，不宣称全程无卡顿。老backbuffer绑定表仍记录73次overflow，不与新owned绑定表混为一谈。

本轮正常移动后没有发生死亡，`entries=0/quad_layers=0/recoveries=0`，因此没有证明实际死亡菜单或检查点恢复已经成功，未提升其验收状态。XR正常关闭，63个入口全部恢复，游戏正常关闭；native资源仍保留到游戏退出，启动器仍为incomplete。清单 `manifest-menu-recovery1.json`，审计 `analysis/ui-position-runtime/menu-recovery1-22156.audit.json`；默认玩家清单未改变。下一轮须实际覆盖死亡菜单和恢复，同时继续处理桌面镜像与释放缺口。

## 2026-10-15：桌面独立左眼镜像及仍未解决的闪烁报告

新增默认关闭的 `GEARS_DESKTOP_MIRROR`。在 owned 场景、关联 UI 和实际队列提交都满足发布条件时，向独立快照复制完整 SBS，恢复源状态；Present 前仅将快照左眼等比画入已认证的当前 swapchain buffer，两侧留黑。绑定同一 DIRECT 队列，私有 GPU 指令绕过原生观察器，未增加 Hook 入口。Present 使用 D3D12 文档规定的 PRESENT/COMMON 状态契约，不宣称新加了独立的 backbuffer 状态跟踪。Present1 成功后要求整帧更新。

当前版本同步等待镜像 draw fence 完成，成功返回前释放 backbuffer 引用。等待上限一秒；超时后关闭镜像绘制，但 worker 继续非阻塞查询 fence，完成后释放目标引用。continuous 停机先排空，再关闭；未确认完成时保留资源供重试，不能先永久 Detach 后无限重试 close。独立测试实际阻塞队列并覆盖恢复路径。

Mirror1 / PID33548 取得918份镜像快照，但桌面绘制为0，目标校验拒绝。运行记录的桌面 RTV 格式为24（R10G10B10A2_UNORM），超出初版四种8位格式；Mirror2增加10位目的格式，源仍限定已支持的8位格式。这里只保持 SDR 数值，不包含 HDR/PQ 或色域转换认证。Mirror1 同时因 owner 的 PermissionError 提前停止，约24.10秒、917张XR复制；监测脚本当时以普通文件读取打开 menu.bin，与 atomic replace 存在共享删除冲突的可能，未取得异常调用栈，原因保留为推断。

菜单观察器现仅在发布阶段捕获 OSError，保留原包及原采样时间，下次 poll 重新采样再原子替换；失败及恢复去重记录，不原地覆盖或把旧包标成新鲜，游戏身份改变仍抛异常。7项 Python 测试覆盖文件被占用、恢复、首次发布失败、清理与身份变化。正式 XR reader 已使用 FILE_SHARE_DELETE；新监测脚本改读变化日志，避免自己阻挡替换。

Mirror2 / PID25668 保留用户设置的1920×1080桌面，owned资源仍为4096×2048，即每眼2048²。实际运行111.039秒，5,056次镜像快照、10,106次桌面镜像绘制，目的格式24；busy=0、timeout=0、unsafe=0。CPU fence等待平均0.1542ms，最大2.7639ms；不等于完整GPU耗时。XR复制5,052张、projection提交9,988次、最大提交间隔55.92ms。345次inventory拒绝集中在启动阶段，运行采样中计数不再增长；时效拒绝5次，仍保留100ms门限。普通UI和Material本轮均未出现，不借此复验UI。

四张640×360窗口采样均为完整单眼画面、左右黑边。模拟器保存22帧滚动证据，目视审阅其中6帧未见明显交替；这只能说明这段样本，不能否定用户报告的桌面及模拟器左眼闪烁。初始原始GPU图与后续模拟器预览均见右眼明显较暗，原因尚未确定，不能把整套双眼显示验收判为通过。下一步应比较连续源眼图、同帧姿态/FOV、渲染状态与合成预览，定位曝光/后处理或其他眼间差异；这些是待验证方向。

独立WARP最终运行5,330项检查通过，覆盖RGBA/BGRA、sRGB、10位目的格式、黑边、快照独立性、源状态恢复、忙跳过、超时及恢复。测试环境D3D12 Debug Layer不可用，已显式输出该限制；早先辅助报告“Debug Layer无错误”不能作为调试层通过证据。相关CPU、DLL ABI/family/frame-link测试通过，关闭功能分支编译及frame-link自测通过。两轮63处入口独立核验恢复后正常关闭游戏，无强杀；native Owner仍保留至进程退出，launcher仍incomplete，默认玩家清单未切换。审计：`analysis/ui-position-runtime/owned-mirror1-33548.audit.json`、`owned-mirror2-25668.audit.json`。模拟器事件目录：`%LOCALAPPDATA%/OpenXR-Simulator/flicker_incidents/session_33872/incident_9026`。

进一步比较同一进程的原生owned读回：ticket2两眼灰度均值约27.16/24.97；ticket121和1801均为左约29.4、右约9.2。后两张之间，左眼365,252像素改变，右眼只有41/4,194,304像素改变且最大通道差1。模拟器22帧采样的右眼像素完全相同，左眼仍有细微变化。这把“右眼几乎不更新/亮度异常”的疑点推进到原生输出阶段，但不证明具体原因，也不等于已解释用户报告的左眼闪烁。原图左右分别占据4096×2048纹理的两个2048²区域，未见两张完整画面简单叠在同一区；上游采样串眼、共享历史/渲染状态仍待查。证据为 `analysis/ui-position-runtime/owned-mirror2-temporal-measurement.json`、`owned-mirror2-eye-change.json` 和源图哈希。下一最小实验可在模拟器提供相同双眼FOV及零IPD，核对实际消费矩阵后比较连续眼图，区分视角差异与右眼更新路径；完成后恢复配置。
