# Runtime-recommended render size: required 0.0.1 default

Release priority: this mode replaces game-resolution-based VR rendering in
0.0.1 and takes precedence over DLSS compatibility. The launcher exposes the
render multiplier; 1.0 uses the OpenXR runtime's recommended per-eye dimensions.
Game resolution does not select the VR render dimensions. Remaining validation
is release work for this default, not a reason to defer it to 0.0.2.

## Default configuration regression, 2026-10-19

### Current default: engine-extent fix

The default and named `manifest-runtime-extent-fix1.json` now select
`GearsCaptureRuntimeExtentFix1.dll` SHA256
`6ee31a6fbd48302ba99096d1fda77f0952099b5ad3df722773299c539eb7cd93`.
In addition to the 0.71x counterfactual below, background regressions at1.0x and
2.0x each passed two checkpoint reloads, cleanup/entry restoration and five-file
restoration with a1280x720 desktop. The GPU sources were2560x1400 and5120x2800;
saved image inspection confirms scenery beyond the old desktop edges, without
the previous right/bottom rectangular holes. Evidence prefixes:
`resolution-extent-fix1-scale{1,2}-20261019`, plus corresponding image/coverage
receipts under ignored `analysis/runtime`. All168 launcher offline tests passed
after promotion. Release2 remains a named historical rollback, not the default.

Current development ZIP:
`analysis/runtime/GearsVR-0.0.1-resolution-extent-fixed-20261019.zip`, SHA256
`7e9deb70debafc7790b8608640fc0e18536dc65ddac98a0640549be2da5ce814`.
Fresh stage and extracted-package isolated preflight passed; all717 ZIP members
were verified against the payload inventory. Receipt:
`analysis/runtime/resolution-extent-fixed-receipt.json`. This supersedes the
earlier resolution-default-ready/feedback archives and remains a test package.

The desktop Present surface still becomes8x8 in these background runs, producing
a blurry desktop mirror. This is a separate known limitation; no claim that the
desktop mirror, mounted gun, DLSS or physical-headset acceptance is complete.

**Counterfactual restores the small-window missing scene region.** Candidate
`build-runtime-extent-fix1/GearsCaptureRuntimeExtentFix1.dll`, SHA256
`6ee31a6fbd48302ba99096d1fda77f0952099b5ad3df722773299c539eb7cd93`, freezes
authenticated original source output dimensions into each world/ticket before
submission. It compares the clear viewport against that frozen engine extent,
while retaining current inventory/queue/owned-resource/native-route checks.
The source extent is scalar evidence, not DSV allocation proof.

Fresh compilation with469 tracked dependencies, focused component tests and the
continuous startup suite passed. Actual background run
`resolution-extent-fix1-run1-20261019` retained1280x720 desktop /8x8 Present and
1818x994 VR output. Both post-checkpoint GPU images were visually inspected:
the old right/bottom rectangular black regions are replaced by scene geometry.
Scene-specific bright occupancy right ofx1280 rose from0.061% to60.7%, and below
y720 from7–8% to85–86%. These thresholds apply only to this known checkpoint,
not generic dark-frame classification. There were1570 clear applications and
1570 restorations; both checkpoint recoveries, native cleanup, entry restoration
and five-file settings/profile restoration passed. Evidence:
`resolution-extent-fix1-coverage.json` and the run prefix under `analysis/runtime`.
At this counterfactual stage, default/portable manifests still selected Release2;
the later promotion above supersedes that state. Older Release2 development ZIPs
still contain the observed bug and should not be used for this acceptance.

Targeted observation confirms the previously missing route: diagnostic DLL
`47ada7fa94d7a47fd28008f28fbd65ebe9e1bc90c00d009314ba2c51569956d1`
captured owned clear viewport 1280x720 on native returns 1af345/1ade10/27e752,
while the exact current same-queue Present inventory was 8x8 and VR plan1818x994.
The existing desktop-extent guard rejected it (reason9). An unrelated80x56 depth
clear has a different native route and must remain excluded. This confirms the
guard mismatch; correcting it still needs a GPU counterfactual, not just counters.
Fresh build, focused tests, continuous startup suite and the bounded background
run's lifecycle/cleanup passed. The run remains visually failing by design:
`resolution-extent-observe1-run1-20261019.final.native-status.txt` stores receipts.

**Visual failure discovered after the small-window run:** user screenshot and
inspection of saved world3 GPU source
`continuous-stereo-44132-20261019-091759-f5419aaf.gpu.ppm.1.ppm` (see the run's
visual path for exact selected sample) show large black right/bottom regions.
The 1818x994 allocation and successful world handoffs do NOT prove full scene
coverage. The automatic `passed` result only covered dimensions/lifecycle and
missed this visual failure. Small-desktop independence is therefore NOT accepted;
do not publish this candidate as resolution-complete. The abrupt coverage boundary
appears consistent with the 1280x720 desktop extent, but the responsible render
stage remains to be identified. Inspect actual viewport/scissor/depth/postprocess
coverage rather than changing texture dimensions or declaring simulator failure.

Fractional background regressions also passed: scale 0.7 produced 896x980 eyes
and 1792x980 SBS after both checkpoint reloads. Scale 0.71 produced odd-width
909x994 eyes and 1818x994 SBS after both reloads. The latter temporarily used
1280x720 desktop settings; a read-only HWND client-size observation confirmed
1280x720 while the VR GPU source remained 1818x994. Both runs passed cleanup,
entry restoration and protected-file restoration. Evidence prefixes:
`resolution-default-background-scale07-20261019` and
`resolution-default-background-scale071-smallwindow-20261019`; actual desktop
measurement: `resolution-default-smallwindow-client-size.json`. All are ignored
runtime evidence. This covers fractional scaling/odd eye width and one smaller
desktop size; it is not a physical-headset quality or performance measurement.

Background default regression: `resolution-default-background-run1-20261019`
passed with scale 1.0 and simulator recommendation 1280x1400 per eye. Two
checkpoint reloads resumed new-world XR projection; both post-reload GPU source
images matched the planned 2560x1400 SBS size. Native cleanup, independent entry
restoration, normal owner exit and restoration of five protected settings/profile
files passed. The driver selected the opt-in behind-window game and simulator
policies, muted game audio, and requested no foreground activation or movement.
This is actual game/simulator evidence, not physical headset or mounted-gun
acceptance. Full identity/status/image evidence remains under ignored
`analysis/runtime/resolution-default-background-run1-20261019*`.

Launcher preview follow-up: changing runtime selection/path, checking size again,
or starting VR clears the previous size label. A failed size command displays a
failure instead of retaining the old runtime's dimensions. The GUI was rebuilt
and its offline self-test passed; no interactive/headset test was performed.
The repository launcher includes this fix; the feedback ZIP below predates this
GUI-only change.

The combined current test package is
`analysis/runtime/GearsVR-0.0.1-resolution-default-ready-20261019.zip`, SHA256
`6cab48aa895cb5eac2adf52b9a72f00c15f442ada09d22aebd2439aab06d36c2`.
It includes both the preview fix and curated size diagnostics. Fresh staging,
complete ZIP verification and isolated preflight after extraction passed;
the extracted default is runtime sizing enabled, scale 1.0, English. Native/XR
binaries are unchanged. Evidence: `resolution-default-ready-receipt.json` under
ignored `analysis/runtime/`. This remains a development package, not a release.

Diagnostics export now retains the latest launcher's recommended eye dimensions,
selected multiplier, planned eye dimensions and SBS dimensions. It labels these
as a preflight plan, not measured GPU output, and does not attribute them to older
sessions. Invalid or inconsistent plans are omitted with a warning; arbitrary
runtime paths are excluded. The diagnostics, launcher, render-size and portable
stage/package suites passed 140 offline tests. This source change requires a
fresh portable package; the originally archived ZIP below predates it.

The updated development archive is
`analysis/runtime/GearsVR-0.0.1-development-RuntimeRelease2-feedback-20261019.zip`,
SHA256 `56be16ccff0e3541a30e98fb8fda8f1362b43db8e280feeb92521f155db0c43e`.
All 717 archive entries were verified, then extracted into a fresh path with
spaces. The bundled isolated interpreter passed imports/PE/hash preflight for
716 payload files. Receipt:
`analysis/runtime/runtime-release2-feedback-extracted-preflight.json`.
No game, simulator or GUI was launched. Native/XR binaries are unchanged; this
updates the feedback export, not mounted/DLSS acceptance.

RuntimeRelease2 is now the launcher default and named manifest
`manifest-runtime-release2.json`. Native SHA
`249aeb07684891c1a7d1813b732a4e8e185b36b2df5827f99659527ee0b26b5f`;
XR artifact remains unchanged. It preserves the Integration1 feature flags,
enables generic depth clearing, and disables source-pixel and size/draw/quad
trace collectors. Attachment tracking required by the clear control remains
enabled. Fresh compilation, six focused native tests and the continuous
startup suite passed. This exact build passed scale2 and default scale1 actual
game runs, each with two checkpoint reloads, final GPU readbacks, native cleanup,
independent entry restoration and five-file restoration. Scale2 final images
have full5120x2800 coverage; reclaim receipts show455->0 and400->0, with zero
capacity faults. Evidence: `runtime-release2-scale{2,1}-run1-20261019*`.

Launcher/render-plan/stage/package suites passed110 tests. Fresh portable
stage `analysis/runtime/portable-runtime-release2-20261019` contains716 files;
isolated interpreter/import/PE/hash preflight passed. This is a development
artifact, not public release acceptance; mounted gun, remaining UI, DLSS and
headset verification remain separate requirements.

The actual development ZIP was subsequently hash-verified, its complete member
inventory verified, and extracted to a fresh directory containing spaces.
Its bundled interpreter passed isolated import/PE/hash preflight for all716
payload files without launching a GUI or game. ZIP SHA256:
`fd4be4e7ec48e5ae3c87da4ad1ce7e532b17097ada65d9acd2cbea7758e19110`.
Evidence: `analysis/runtime/runtime-release2-extracted-preflight.json`;
driver: `analysis/runtime/verify-release2-extracted.py`. This verifies the
delivered archive's offline dependency closure, not installation or headset
behavior on another player's machine.

RuntimeRelease2 also passed the automatic PathChoice lifecycle at1.25x
(1600x1750 per eye, source3200x1750) with the clean-package HUD65 setting.
Session2808 `continuous-stereo-2808-20261019-075038-d22d0000` acquired one
group position, held42 seconds with10 renewals and restored after normal choice.
Final applied/restored counters were1/1; rejection, unresolved and service-terminal
counters were0. Native cleanup and entry restoration passed, the game closed
normally, and all five backed-up settings/profile files were restored.
Evidence: `analysis/runtime/path-fit-release2-scale125-20261019.json` and its
launcher log. Four GPU readback files were saved; no new headset or Hammer-pickup
acceptance is inferred. The test explicitly allowed the owned game foreground,
muted game audio and used a child-only simulator runtime.

The same DepthReclaim1 DLL also passed scale0.7: runtime recommendation
1280x1400 produced896x980 per eye (SBS1792x980). Both checkpoint reloads,
native cleanup, independent entry restoration and five-file restoration
passed. Final GPU previews show two complete scene halves at that size.
Reclaim receipts show375->0 and378->0 active identities, no overflow.
No depth-clear viewport mutations were admitted in this lower-size run;
its passing images do not prove a low-size mutation occurred. The high-size
run below is the actual mutation evidence. Evidence:
`runtime-depth-reclaim1-scale07-run1-20261019*`, session37060.

DepthReclaim1 (`2f9276a7e818047ec68f08db165102544918d935cbc520fafba13e45f91ee9cd`)
passed fresh build/component/startup checks and the scale2 repeat with two
checkpoint reloads. Certified retirement now precedes explicit association
reclamation, under the existing session/frame locks and closed admission;
canonical COM leases are released afterward. Pending or unsubmitted owned
work still prevents retirement. The existing primitive preserves monotonic
list generations and sticky faults; tests cover768 lifetime list identities,
stale references/reuse and failed retirement without erasing evidence.

Actual receipts show world1 active476->0 and world2 active404->0, no capacity
fault or list overflow; final active326, lifetime high-water476. Each world
has accepted generic depth-clear receipts (1916 applications/restorations
total). Both final5120x2800 readbacks retain scene content beyond the old
desktop width/height. Two reloads, native cleanup, independent entry checks,
normal close and five-file restoration all passed. This proves the tested
cross-world accumulation case, not unlimited identities within one world.
Evidence: `runtime-depth-reclaim1-scale2-run1-20261019*`, session40880.
Release-build validation remains pending.

DepthGeneral1 replaces the trial's fixed dimensions with the validated
session RenderSizePlan. It compares the desktop Present inventory against
a read-only, single-lock snapshot of the currently admitted anchored
swapchain binding, including every buffer identity, dimensions, queue and
generation. Only an exact full desktop viewport on the certified native
clear route is changed. Already-correct VR viewports remain unchanged.
Accepted receipts now have a separate16-row bank from rejected draws.
Fresh build SHA `1fdb2f22053a20a884ce18dcac9ca0ee051036ada413b73f0b6fe1d745dad97a`
passed OwnedDepthClearTrialTest, FrameLinkTest, source-pixel/NGX checks and
the continuous startup suite. The helper tests cover multiple rectangular
sizes, invalid plans, stale inventories and failed resize revalidation.

Actual scale2 session32040 **failed lifecycle acceptance** before its second
checkpoint reload completed. It did apply and restore1563 generic clears;
world2 ticket449 HDR source SHA
`af210e65632ba0920d1e6332addf6aa2d9be4e9224f8bdbbd11ec2190b773898`
has99.9934% positive pixels beyond x3840 and99.9987% below y2160. This supports
the generalized extent calculation, not completed lifecycle or lower-scale
acceptance. The exact failure is now recorded: tracker_create subtype6,
active_lists512/list_capacity512, overflow0->1, capacity_bad0->1. Only60
retained creation leases existed, so their separate capacity was not hit.
The owner stopped with owned_error14/proof_failure; native cleanup timed out.
The harness subsequently closed the game normally and restored five files,
but entry restoration was not independently verified. Preserve this failure;
do not count the normal process exit as native cleanup success. Raw statuses
were archived as `runtime-depth-general1-scale2-run1-20261019.failed.*` before
any new run. Next work is safe tracker identity reclamation after certified
retirement, then repeat high/low multiplier lifecycle tests. Default artifacts
have not been advanced to this diagnostic build.

DepthClear2 counterfactual restored the previously missing high-scale scene
region. DLL SHA `c6383e60ea63f9cdb71a0a759d274aae59ed2dd32e429c80f975e952d0c5c821`;
fresh compilation, component checks and continuous startup suite passed.
Actual session `continuous-stereo-45744-20261019-065223-169d596a` completed two
checkpoint reloads, normal cleanup, 77 independent entry-byte checks and
restoration of all five protected files. The private trial changed only the
GPU viewport around the byte-attested depth-clear draw on the current owned
scene, from3840x2160 to5120x2800, then restored it. Runtime counters report1906
applications and1906 restorations; both native chunk hashes matched.

World2 ticket490 HDR source copy1 SHA
`e36e4120f3bffb9b712d46f5998835f4ce086908c68030479fe4775dac0e08a6`
has99.9995% positive pixels right of x3840 (previous baseline: all zero), and
99.9984% below y2160. Copy2 independently has99.9968% and99.9987% respectively.
Source and final XR readback previews show full scene coverage in both halves;
both post-reload final GPU images cover5120x2800 and exceed97.6% occupancy to
the right of the old desktop boundary. These scene-specific measurements and
visual inspection support the stale depth-clear viewport as a cause of the
observed clipping. They do not establish every rendering effect or headset
quality, nor explain all historical black screens.

The trial remains default OFF and hardcoded to the measured counterfactual,
not the release implementation. Generalization must derive dimensions from
the certified runtime plan and retain native-route/ownership validation.
Its32 diagnostic rows filled with unrelated rejected draws before world2;
aggregate counts remain available, but per-ticket accepted receipts need a
separate bounded allocation before further attribution. Active DSV resource
dimensions remain unobserved. Evidence: ignored
`analysis/runtime/runtime-depth-clear2-scale2-run1-20261019*` and raw readbacks
under `tools/native-capture/build-runtime-depth-clear2/`.

Attachments1 actual run completed two reloads and cleanup. DLL SHA
`d2be0253dc8fcdd6e7a85811d28616dbde05b939969bb54948706c3b6b446193`.
Ticket528/world2:1269 direct draws,120 groups, no overflow. Scene MRT slots1-3
are5120x2800, formats24/26/29; first scene color slots also5120x2800.
Two depth-only candidates (ordinals1 and1230) use3840x2160 viewport and the
same raw DSV handle145b43cc0. Both observed stacks include native returns
1af345 ->1ade10 ->27e752 ->119347 ->15a943. DSV resource size remains unknown.
Cached27e580 pseudocode and newly exported historical assembly show its
27e74d call passes color-disable edx0 and depth-enable r9d1 to1ad75c before
the per-view loop. The two reads therefore identify a concrete depth-clear
candidate still using desktop viewport; causal pixel effect remains untested.
Historical slices27e580..27e752 and1ad75c..1ade10 match source blob
911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6;
hashes dd5aeb84d7bb96ebeb1b0d2f24f37ba12127dac74b872be038632fd94cdf0cea
and34142122907196b48fa7c7b7ab28508837420c5a8e930c7c2b2f38fe57172622.
Private IDA lease closed. Evidence: runtime-attachments1-scale2-run1-20261019
artifacts and analysis/ida/ngx-copy-contract-20261019/clear-route-chunks.json.

SourceRanges1 narrows the viewport hypothesis. DLL
`1f4dc427d2a759cde44c2066bee5e1fff2ca95b26b022d25c3306405a639b7ed`
passed startup/component tests and two actual checkpoint reloads with clean
restoration. Ticket503/world2 records1135 nonempty direct draws in113 exact
range groups (limit128, no overflow);659 draws lack a first color target and
one lacks eye attribution. Readback source canonical16ae081640 still has
all-zero columns x>=3840, raw SHA
`3e6b4f13617e555552b67ece9e0b13e7114c8b811000c0e29c951c09af6e19f2`.
Matching source writes have full or correct per-eye5120x2800/2560x2800
viewports, with several unrestricted scissors; there is no universal3840
scissor bound in these observed writes. Some use four render targets.
This rules out a universal observed direct-draw viewport/scissor bound, not
all upstream clipping: only first RTV is resolved; other MRTs, bound DSV,
compute/copy/clear/indirect writes remain outside this diagnostic.
Next resolve other OM attachments at these writes. Existing RTV descriptor
hooks can cover all slots; existing viewport depth_known is only MinDepth/
MaxDepth, not depth-buffer identity, and must not be used as DSV evidence.
Evidence: `analysis/runtime/runtime-source-ranges1-scale2-run1-20261019.json`,
`.final.native-status.txt`, `.final.native-status.source-ranges.json` and
ignored build-runtime-source-ranges1/owned-hdr-source-41744*.

SourcePixels3 provides actual intermediate HDR evidence. Private DLL
`f4fcced4ca35b72c9421960e7c15e7ec9c7e442b69c5a2ac867583366c143309`
passed the source-readback and NGX regression tests plus continuous startup
suite. Actual session `continuous-stereo-27764-20261019-062549-a0ef966e`
completed two checkpoint reloads, native cleanup and protected-file restoration.
The previous error14 did not recur: owned tracker highwater459/512, no overflow;
new first-failure subtype diagnostics remain available without changing policy.

At ticket495/world2, two sequential native color copies at return RVA1c6093
captured the same source canonical16c3184880/destination16c317fc00, format26,
5120x2800. Both completed real queue fences and command-list retirement before
readback; all pixels decoded finite. Both contain exactly zero RGB throughout
x>=3840 (3,584,000 pixels), with asymmetric scene coverage in the preview.
Raw hashes: `a5cd13d7421bd8bb5ad5940cccc2fa2d5b39102119188a95c43294a2739b7471`
and `1ba7406d44e83df55c8e88b585201037ce3d03a24f1245e9cd46a83e7071a5ff`.
Evidence is under ignored build-runtime-source-pixels3/owned-hdr-source-27764*
and analysis/runtime/runtime-source-pixels3-scale2-run1-20261019*.
This locates missing pixels in an intermediate native color source, before
XR transport; it does not yet identify the producer responsible or prove exact
final-output association. Both copies report TLS eye1, not independently
certified eye content. Next inspect the producer/pass immediately upstream of
this source, rather than further changing XR output size.

SourcePixels2 diagnostic follow-up: the private build
`a79ade0afe7b00df7bfb5a635bf4401edf6bcb13fc7244e2114865263d96831a`
entered projection at recommended1280x1400 times2, but failed during the first
checkpoint reload: owned_error14, stop_reason proof_failure; native cleanup
was not verified. No HDR raw readback was obtained. This is a failed diagnostic,
not evidence that source pixels or checkpoint rebuilding are correct.
The coordinator subsequently requested normal game exit and restored all five
protected configuration/profile files with matching hashes. Original failure
report is preserved at
`analysis/runtime/runtime-source-pixels2-scale2-run1-20261019.json`, with
separate `.recovery.json`. The diagnostic is not the launcher default.
The required0.0.1 replacement route and priority above DLSS remain unchanged.

### Blocking image defect at scale 2.0

Synthetic transport check,2026-10-19: private
`analysis/runtime/high-size-share-test.cpp` reuses the real GpuPublisher and
GpuReceiver with an owned5120x2800 RGBA8 target, independent of any swapchain.
Four distinct GPU-clear regions cross x3840 and y2160. After actual shared
publication, acquisition and full-footprint GPU readback, every RGBA channel
of14,336,000 pixels matched (zero differences); ticket and dimensions matched,
ACK/drain/close succeeded. D3D12 debug layer was unavailable (`false`).
This supports the transport's ability to preserve pixels beyond desktop size;
it does not prove the live game selects the correct source/state or produces
correct scene pixels. Source HDR readback remains required. Evidence:
`analysis/runtime/high-size-share-test-result.json` and build log/source.

Owned postprocess trace: RuntimeQuadTrace1 DLL
`95137583c2057582f51446993198684e92c62e6196c1ce2795a9db67a60a2dcd`
passed22 startup checks and two reloads, normal shutdown/restoration.
Session `continuous-stereo-45540-20261019-055214-cba73105`, ticket487/world2:
40 ordered quad draws, no overflow. Final d83b1c rows20/39 have correct
left/right2560x2800 destination/source rectangles,5120x2800 target/texture
dimensions, actual RTV5120x2800, full viewport, and non-clipping scissor.
Both bind slot0 actual5120x2800 format26 color texture. Intermediate rows
include reduced-size passes; size reduction alone is not evidence of a bug.
Rows0/1 have unknown RTV identity (resource0), not proof of missing output.
Corrected sampling-buffer reads now match actual allocation dimensions.

Both post-reload source readbacks still have zero pixels above channel4 in
columns3840+. Thus the CPU final-quad dimensions do not explain the crop.
Need actual intermediate color pixels versus final output, or independently
verified shader/constant evidence; don't infer correct pixels from descriptors.
The report/complete status remain ignored under
`analysis/runtime/runtime-quad-trace1-scale2-run1-20261019*`.

Later-world counterexample: RuntimeSizeTrace3, DLL
`19417a286f414f9a706be33fa7916ff2f905ed70657512d437570c4f279e8df8`,
passed22 startup checks and two actual reloads with normal restoration.
Session `continuous-stereo-46068-20261019-054626-e3d3b745`, ticket481/world2:
built/pre/post sampling is already `.25,-.5,.5,.25` (left) and
`.25,-.5,.5,.75` (right), with correct2560x2800 eye rectangles and observed
geometry viewports/5120x2800 target. Yet both post-reload GPU captures still
have no pixels above channel4 in columns3840..5119. Thus stale constructor
sampling is **not a sufficient explanation for the persistent clipping**;
do not promote a four-float sampling patch as its fix. Next inspect actual
owned postprocess/final source bounds and copies.

Trace3's extra `owned_ticket_sampling_buffer` rows are INVALID evidence:
that binary read the misleading IDA label's RVA1c3a3b8. Its independent
view/target/draw rows remain usable; source now reads RIP-confirmed1c3b3b8.
The diagnostic world filter is build-only (`GEARS_SIZE_TRACE_MIN_WORLD=2`).
Trace2 was never run in-game: overriding CMAKE_CXX_FLAGS omitted `/EHsc`,
and viewport exception-cleanup self-test31 failed. Trace3 restored all
original flags and passed the full suite before starting the game.

2026-10-19 bounded owned-ticket trace: private RuntimeSizeTrace1 DLL
`91d2cbe6bd1074664c56129bd53807e720120f23bb377b0802e8578fdcb79ea0`
passed the 22 startup checks and two actual checkpoint reloads, with normal
cleanup/restoration. Session `continuous-stereo-34916-20261019-053954-4fba6fbe`.
Ticket2/world1 has all eight source/built/prepare rows (mask127): owned target
5120x2800, built and prepare eye rectangles2560x2800 at x0/2560. The first
observed indexed geometry draw per eye also targets5120x2800 with the correct
eye viewport and a non-clipping scissor. This is bounded direct-draw evidence,
not proof that all passes use correct dimensions.

The built sampling vector is nevertheless
`0.333333343,-0.648148119,0.648148119,0.333333343` for the left eye (right
last component1), consistent with denominator3840x2160. It remains unchanged
through prepare. Constructor observation QPC2211057222908 precedes the actual
resource resize QPC2211057579650..2211058176743; prepare starts afterward.
Historical snapshot assembly was re-exported and byte-matched for
116250..116b34: at116590/1165bf it reads globals1c3b3b8/1c3b3bc to derive
view+3d0..3dc, independently of the target's virtual width/height clamp.
Slice SHA256 `b3614facc38c953bc9b2ead6c8cca42783225619757a630d06b9d1982733a07e`.
Corrected the original address transcription using raw RIP-relative bytes:
`8b05224eb201` at116590 and `8b05f74db201` at1165bf. IDA's cs display labels
are 0x1000 lower than the real targets; KB xrefs agree with the decoded targets.
Evidence is under ignored `analysis/ida/ngx-copy-contract-20261019/renderer-sampling.*`.
This identifies a constructor/resize ordering discrepancy for the sampled
ticket, **not yet the complete explanation for persistent clipping**. Both
post-reload GPU images still have zero pixels above channel4 in columns3840+.
Next establish later-ticket sampling and postprocess bounds before choosing
the corrective lifetime boundary. No new default package was promoted.

Source-owner candidate tested, **insufficient to fix coverage**: source view
construction now receives the same owned family/target copy as renderer
construction, while original source/family authentication and immutability
remain. Historical assembly at 15efd3–15efed reads family target dimensions
into +60/+64 and 15f056–15f07b derives sampling values. Coordinator re-exported
15ef46..15f171 and matched source blob bytes; slice SHA256
`aa69699eb320146c91b18aec3a0d8182d8bbb8246f8a61cde0d2bc977fa80c4e`.

Private build `build-runtime-source-owner1` (DLL SHA256
`9f1c581f07091dea5789dcb38fb63a0619386eaa3a302baf1841da1eb3378f25`)
compiled with 462 tracked dependencies; 22 continuous startup checks passed.
Actual `runtime-source-owner1-scale2-run2-20261019` completed two reloads and
77 restored-entry checks; five file hashes independently matched and no game
remained. But both 5120x2800 GPU images still have zero pixels above channel
value4 in columns3840..5119. The first image was visually inspected; the
original right/bottom defect persists. Report SHA256
`3a7639ec12ceeccb60e10e0a6813c3d51830d76e4ab5bd4923c2f5c084820eea`.
Do not promote this candidate or claim the constructor mismatch is the sole
cause. Next evidence must follow owned-ticket dimensions through renderer
construction/prepare and scene passes; existing first-frame traces are
bootstrap observations. Run1 stopped before game creation because the fresh
build lacked GraphicsMenuLockTest; built the missing target before run2.

Actual `runtime-integration-checkpoint-scale2-run2-20261019` completed two
reloads, new-world XR projection, normal shutdown and restoration. Its
`passed=true` covers those mechanical checks only: both GPU files are 5120x2800
as requested, but coordinator inspection of the second image shows substantial
black right/bottom regions and scene content apparently limited to the old
3840x2160 extent. **This is NOT successful scale2 image acceptance.** Investigate
the actual scene view/target/viewport dimensions before accepting the runtime
size default as release-ready. This is higher priority than DLSS.

Session `continuous-stereo-2412-20261019-051846-06cd3de6`; report SHA256
`25f528fd7931f0dbbd83667ab5b9a60e206dc4c59caa5b973a5495477c7faaf3`.
Second GPU image SHA256
`e1b8adc1362a02d7b38c97c3cdda61f2e1b0f069fb2852a9807d1abf604be250`.
No exact per-ticket image association was established. Independent 77 entry
checks and five protected-file hashes passed; no game remained running.
Run1 stopped on a new menu object's index exceeding cached inventory; run2's
private driver uses a fresh reader per observation, retaining exact two-read
input checks. No production menu rejection rule was weakened.

The launcher suite found the promoted Integration1 default lacked an identical
named manifest. Added `manifest-runtime-release-integration1.json` with the
current verified artifact hashes; the active manifest and binaries are unchanged.
The default regression now explicitly requires runtime sizing and scale 1.0,
and accepts user multipliers 0.25, 0.7, 1.0 and 2.0. All 74 launcher tests and
four render-size tests pass. These are configuration/CPU checks, not additional
headset or high-resolution GPU acceptance.

## Integrated default: two actual checkpoint reloads, 2026-10-19

After default promotion to Integration1 (native SHA256
`41a5b882ab1202940c0590d88a6c732609fd0db9511157e19f51f802064cc065`,
PathChoice service enabled), `runtime-integration-checkpoint-20261019-run3`
completed two normal reloads of the current checkpoint in the same game process.
Session: `continuous-stereo-9480-20261019-044100-2fdf1b18`.
The launcher authenticated the child simulator's recommended 1280x1400 per eye
at scale1; desktop settings were not used to select the VR dimensions.

World generation advanced 1→2→3 and commit count 0→1→2. Each generation had new
native publication/tickets and a matching XR `world_handoff_card_ended` receipt
with `world_valid=true`, `quiescing=false`, `replacement_projection=true`.
Each fresh GPU file was complete 2560x1400 SBS, matching the authenticated plan;
the coordinator viewed the second reload's image and confirmed both scene eyes
were present. Native cleanup, entry restoration, owner exit0 and independent
five-file original SHA256 restoration passed. Report SHA256:
`cd5d64c933382ef6d6040cf46e121f91a23db86a805db68b58e8480af4e87c91`.

Runs1/2 each completed the first reload, then the test's second menu-open guard
rejected the observed player_index0 because it only accepted initial -1.
Run2 retained the exact read-only consistent snapshot; the private test now
accepts those two observed indices with all other identity/scene checks intact.
Twelve offline driver checks passed. No production admission rule was changed.
This evidence covers enabled PathChoice service across world replacement, not
an open PathChoice panel during reload, all checkpoints, mid-retirement stop,
real-headset quality, mounted play or DLSS. Runtime-recommended dimensions ×
launcher multiplier remain the required 0.0.1 default, ahead of DLSS.

## Interrupted-handoff coverage attempt, 2026-10-19

Follow-up `checkpoint-prepare-stop-actual3-20261019` covers cancellation during
preparation, separately from the unobserved native phase-1/2 race. The current
default requested generation 2 while generation 1 remained active; preparation
count was 1, handoff phase 0 and quiesce 0. Owner cancellation then completed
with generation still 1 and commit QPC 0. Native cleanup, all 65 entry-byte
checks, normal game exit and the five original file hashes passed and were
independently verified. This proves the observed preparation-cancellation path;
it does not prove interruption inside GPU retirement or recreation.

`checkpoint-handoff-stop-actual2-20261019` used the current default manifest,
scale 1.0, child-only simulator and muted background launch. State-checked
checkpoint confirmation completed; sampled generation changed 1 to 2 and the
native commit QPC was 2143641623022. Stop QPC was 2143650047135, **after** that
commit. The test correctly reports `race_observed=false`: its status polling
missed phases 1/2, so it does not prove interruption during resource recreation.
Native cleanup, all 65 restored-entry checks, normal exit and five original
file hashes passed, independently checked by the coordinator. The earlier
actual1 stopped before menu input after a PermissionError; driver2 uses the
existing share-delete JSON reader and preserves traceback/path diagnostics.
The original error's exact location was not recorded and remains unproven.

## Default mode promotion, 2026-10-19

Development package after audio integration: ignored
`analysis/release-zips/GearsVR-0.0.1-development-RuntimeScale-Audio-20261019.zip`,
13,979,905 bytes, SHA256
`3fd496ac54b730317f8843fc36d4725227f0606036a1715e6d82918840d9d233`.
The staged inventory and freshly extracted package both passed bundled-Python
preflight. Extraction receipt is `runtime-scale-audio-extracted-preflight.json`.
This package contains the default runtime-size mode and packaged audio helper;
it is not a public release or a fix for outstanding mounted/UI/DLSS issues.

User confirmed runtime-recommended per-eye size times launcher render scale
replaces game-resolution VR rendering in 0.0.1 and takes priority over DLSS.
`tools/vr-launcher/manifest.json` now selects the previously tested world-handoff
candidate from `manifest-runtime-resolution1.json`, with `runtime_render_size`
enabled. Both native and XR binary hashes were checked before promotion. The
launcher defaults missing scale to 1.0; desktop resolution controls the mirror.
Earlier statements below that the player default is unchanged are historical
and superseded by this promotion. This is a default-mode change, not a claim
that all remaining 0.0.1 issues have passed acceptance.

The current GUI was rebuilt, passed its hidden self-test and installed as the
root `GearsVR Launcher.exe`. Four render-size tests and sixteen portable-stage
tests passed. An independent development stage at ignored
`analysis/release-runtime-default-001` passed bundled-runtime preflight; its
manifest enables runtime size, its config sets render_scale=1.0, and its GUI
hash matches the installed executable. Receipt:
`analysis/runtime/runtime-default-001-preflight.json`.

No new game or headset run occurred during this promotion. Actual checkpoint
evidence remains the two reloads documented below. UI fixes, mounted gameplay,
DLSS compatibility, interrupted handoff and final headset/package acceptance
remain outstanding; the development stage has not been publicly released.

## First actual-game evidence, 2026-10-18

### Post-promotion startup smoke, 2026-10-19

The first background smoke cancelled before activation when the occlusion guard
observed the game above other applications. Thus the background placement fix is
not universal. With the user's explicit overnight permission for foreground
operation, the second smoke entered gameplay using the promoted native/XR pair
and a child-only simulator runtime. Recommendation 1280x1400 at 1.0x yielded
2560x1400 SBS readbacks: three files with distinct hashes and nonzero means in
both halves. This is simulator-size evidence, not physical-headset quality.
XR recorded 1353 stereo projections and 409 scene copies. The session stopped
with native cleanup and independent entry restoration verified.

The private driver's final normal-close loop hit Win32 error 1400 after a window
disappeared. Root separately verified no game remained and restored all five
hashed settings/profile backups. Do not call the driver itself fully passed.
Receipts: ignored `runtime-default-smoke2-recovery.json`,
`runtime-default-smoke2-pixels.json`, and
`continuous-stereo-32904-20261019-024011-980a4109.json`.

### Actual 0.7x scale smoke

A separate child config set render_scale=0.7 without changing the user's config.
With the same simulator recommendation 1280x1400, the actual GPU readbacks were
1792x980 SBS (896x980 per eye). Three samples had distinct hashes and nonzero
means in both halves. Session `continuous-stereo-40272-20261019-024237-e8f3debe`
recorded 1344 stereo projection layers and 399 scene copies, then stopped with
native cleanup, entry restoration, normal game exit and all five settings/profile
restores verified. The driver tolerated a disappearing normal-close window so
one stale HWND could not skip remaining cleanup. Evidence:
`analysis/runtime/runtime-scale07-smoke-evidence.json` and the matching full
driver/session reports. This proves actual size scaling for this recommendation
and startup path; it does not establish physical-headset quality, frame-rate
improvement or all scale/runtime combinations.

### Verified terminal retirement follow-up

Session `continuous-stereo-17496-20261018-192745-63abd9ec` ran the retirement candidate for 59.37 seconds of scene submission: 1202 published pairs / copies and 5347 simulator scene submissions. Native shutdown reached `live_phase=4`, `shutdown_phase=3`, `retained=0`, `resources_retained=0`, `owned_shutdown_closed=1`, fence state 2 and no cleanup timeout or owned error. The session runner independently confirmed both native cleanup and entry-byte restoration; its scoped session result passed. Game then exited normally and all four pre-test profile hashes were restored. Raw status and GPU samples are preserved in ignored `analysis/runtime/retirement-17496-evidence/`. The independent runtime-size candidate manifest now selects this build; the player default remains unchanged.

This establishes one actual-game terminal retirement and short sustained simulator run. It does not establish world handoff, restart inside the same game process, mounted-camera behavior, DLSS, headset appearance or long-session stability. `world_handoff_supported` remains false and must not be presented as delivered checkpoint recovery.

Implementation underlying the verified retirement run above: `build-runtime-resolution1-retirement` now freezes native/Canvas CPU drain, refuses unsubmitted or uncertified owned tracking, closes publisher/mirror, waits for a dedicated fence on the certified queue, releases diagnostic references, and services the existing Owner retirement on its original RHI thread/TLS. Hooks remain available for this service until completion or the existing cleanup deadline. Queue/resource/generation identity and the session-native → frame lock order are checked. Completed unpublished UI cycles have a separate discard receipt; they are not reported as published. World handoff remains disabled. Fresh full compilation, 24 startup invocations, 31 WARP queue checks, 375 Owner checks and 2012 ledger checks passed. These component results preceded the actual-game retirement evidence above. The player default remains unchanged; the independent runtime candidate now selects the retirement build.

First/later GPU samples from the run below have different hashes for each eye, with changes spanning each 1280×1400 half. This excludes a completely frozen eye across these two samples only; it does not establish correct temporal history. Receipt: ignored `resolution-material-24120-evidence/eye-temporal-comparison.json`.

Session `continuous-stereo-24120-20261018-185551-6eeae1ab` used the material-policy candidate and simulator recommendation 1280×1400 at 1.0. The native engine target was 2560×1400 with `built=1`, both-eye draw/submission masks 3 and error 0. It published 146 eye pairs; XR reported 610 successful scene submissions and 146 copies over about 6.75 seconds. The saved 2560×1400 last GPU image visually contains game scenery in both halves. This is actual-game source image and simulator submission evidence, not headset acceptance, temporal correctness or long-session stability.

Desktop swapchain output remained 8×8. The coordinator stopped this short test on that observation; subsequent logs and GPU image establish that the independent VR path was nevertheless producing scenery. Future tests must distinguish the desktop observer from owned VR target evidence. No window movement or activation was used. Game exited normally and the four pre-test profile hashes were restored.

The session remains **incomplete**: native reports `resources_retained=1`, with scene Owner retention until process exit, so the existing cleanup gate correctly failed. It also reports `world_handoff_supported=0`, which remains a delivery gap for checkpoint/world changes. Do not waive these checks. Independent entry verification initially rejected the 65-hook inventory against its obsolete 64 limit; the limit now matches capture.cpp's maximum 87 slots, with all per-entry byte checks preserved. All 65 entries then independently matched. Verifier tests cover the larger inventory, duplicate and over-capacity rejection (18 tests pass). Raw status and both GPU images are preserved under ignored `analysis/runtime/resolution-material-24120-evidence/`.

This candidate implements the 0.0.1 requirement to render VR independently of the desktop resolution. It is not enabled by the default manifest. The short actual-game evidence above does not establish full game lifecycle or headset acceptance.

`runtime_render_size: true` in a candidate manifest enables the launcher control. `render_scale` in the user configuration defaults to `1.0`, accepts `0.25` through `2.0`, and multiplies each runtime recommended eye dimension. Dimensions round upward to whole pixels. Both eyes use the larger recommended width and height when recommendations differ. A recommendation of 3072×3264 yields 2151×2285 per eye at 0.7, or 3072×3264 at 1.0.

Before launching the game, the launcher authenticates the candidate XR executable and invokes `--render-size-info --render-scale VALUE`. This enumerates the runtime without creating an XR session. Requested dimensions exceeding runtime limits or the existing 8192-pixel SBS width/height limit fail explicitly; no value is silently reduced. The GUI provides “Check eye size,” displays the returned dimensions, and retains the selected multiplier for the next session. `GEARSVR_MANIFEST` selects an explicit candidate manifest for the GUI without overwriting the player default.

The XR producer publishes an immutable, versioned `RenderSizePacket` bound to game PID, process creation time and stereo session. Native code verifies and freezes that plan. The native engine surface, family adapter, reconstructed eye view rectangles, GPU target proof, image-ticket rectangles and XR swapchains must agree on the same dimensions. A desktop-sized source that differs from the plan is rejected. This path does not upscale the existing desktop image to claim higher scene resolution.

The existing engine renderer creates and consumes its scene resources for the owned target. Capacity reuse avoids alternating engine allocations between VR and desktop dimensions. Desktop mirror rendering reads a separate snapshot and letterboxes the left eye using its actual aspect ratio. Odd eye heights are supported by the readback path. UI and Material routing retain their existing source geometry and use the planned destination dimensions.

The initial multiplier is fixed for one session. Changing it requires a new session. The candidate has verified terminal Owner retirement in the short actual-game run above, but world handoff remains disabled. Historical right-eye/postprocessing failures still require broader runtime coverage. The candidate now retains the default DOF safeguard: the exact existing setter/quad proof is required at the owned publication boundary, and its ticket is retired after that boundary rather than at desktop Present. Experimental owned transports without a runtime size plan remain denied. Static compatibility does not establish that historical pixel failures are fixed.

Validation completed for the component implementation: render-plan/runtime limits and bridge identity checks; exact XR crop policy; rectangular native constructor and source-preservation checks; owned-surface creation/retirement mocks (317 checks); rectangular capacity reuse; odd-height WARP readback in three idle states; WARP desktop mirror (10,639 checks, Debug Layer unavailable); launcher tests (68 existing + 3 new); continuous runner tests (51); GUI compilation/self-tests and a nonactivating offscreen layout inspection; XR translation-unit syntax compilation. These do not establish actual engine depth/postprocessing resolution, temporal eye updates, game lifecycle correctness or headset quality. Those require the coordinator's separate candidate build and actual-game/GPU/headset tests.

The coordinator applied the capture/CMake integration and DOF compatibility wiring. A complete independent native/XR build and 56 native startup/component/integration invocations passed; the owned GPU fixture includes 257 real WARP publications with synthetic scene metadata and verifies incomplete DOF proof and world changes prevent publication. The first validation invocation omitted required test arguments; a later fixture lacked valid geometry, and its first negative test hit the resource-state guard before the DOF guard. Those failures remain recorded under ignored `analysis/runtime/`. The corrected frozen build passed.

A focused follow-up corrected the previous square-only integration coverage. New modes 22–24 use the public render-size planner and real WARP resources for per-eye 1280×1400 at 1.0, 896×980 at 0.7, and a synthetic odd-height recommendation of 1280×1401 at 1.0. The first two are seeded from the coordinator's recorded simulator recommendation; these fixtures do not enumerate a runtime themselves. Each case creates the planned SBS resource, clears left/right regions to different colors, publishes one exact ticket, acquires it through the real shared GPU receiver, checks both ticket rectangles, reads back both source and receiver, and matches the separate eye hashes and pixel counts. The publication gate requires matching world/ticket and synthetic complete two-eye DOF setter/quad proof, which survives the desktop retirement call and retires after publication. All three focused cases passed on a fresh native rebuild. The original 56-invocation report applies to the preceding binary; it was not rerun for this test-only change. The follow-up report is `analysis/runtime/runtime-resolution1-rectangles-validation.json`. An initial missing stdout flush was detected by the evidence parser and is preserved separately.

This establishes rectangular/odd-height D3D resource and publisher/receiver plumbing with synthetic scene and DOF metadata. It does not establish native engine RT creation, real setter/quad execution, engine view/depth/postprocessing dimensions, OpenXR swapchain submission, menu transitions or actual headset quality.

Follow-up integration review found a concrete UI regression: the owned Material path always used default HUD parameters, ignoring the existing kind classification, HUD switch and user layout controls. `material_overlay_plan.h` now preserves the Mounted104 policy: kind 0 uses ordinary geometry; kinds 2/3 use the user HUD parameters and layout only when enabled. The owned consumer retains its resource, command-list and ticket checks. A 120-case CPU comparison covers classification, 45/65/100-degree user fields of view, depth, layout, rectangular and odd-height targets, replay and restoration. A fresh full DLL build plus the registered `MaterialOverlayPlanTest` passed in `build-runtime-resolution1-material-policy`; host observation remains OFF. This is a source/build correction, not game/UI acceptance. The candidate manifest was subsequently advanced after the startup checks below; the player default was not changed.

`tools/vr-launcher/manifest-runtime-resolution1.json` initially selected the material-policy build after 24 startup/component invocations passed against that exact DLL (receipt: `analysis/runtime/material-startup-validation.json`). The default remains Mounted104. The component-only results below were followed by the short actual-game run recorded above; full lifecycle and headset acceptance remain outstanding. Test reports and build logs are under ignored `analysis/runtime/`. Local Ninja header dependency detection was unreliable with the installed Chinese compiler resources, so the candidate was compiled with `cmake --fresh`; do not treat a bare relink as proof that changed headers were compiled. No default manifest or existing player executable was replaced by this component task.

## World handoff components in development

The desktop mirror can now retire a source without terminally closing the mirror. It freezes capture/presentation, polls both GPU fences, and retains all references while either is pending. Once drained it accepts only a newer source generation and preserves the image-ticket high-water mark and counters. A stalled-queue test rejects early replacement, then binds a different-size source and reads back the new green pixels; terminal close still cannot reopen. The real WARP mirror suite passed 10,659 checks; D3D debug layer was unavailable. This is component evidence, not game checkpoint recovery. Frame-link retirement is likewise an explicit externally certified transition; host integration and actual checkpoint testing remain outstanding.

The coordinator has integrated these stages behind default-OFF `GEARS_OWNED_WORLD_HANDOFF`. It requires a runtime-size owned target and UI batch route. Old CPU consumers remain admitted until renderer/UI completion, then publisher and mirror detach their old source after GPU completion. The native Owner retires on its original RHI thread; the short closed-world commit verifies resource/UI receipts, retires residual completed DOF entries, advances epochs and arms later RHI creation. Ticket counters stay monotonic. Publisher rebinding preserves the shared slot and serial and requires the same queue/device/descriptor, a later world and newer resource generation. Ordinary unrelated desktop commands no longer invalidate the source-specific retirement receipt; late old-source use still does.

A fresh full candidate compilation (including capture.cpp) and 29 startup/component invocations passed after the DOF integration. Candidate DLL SHA256: `e4c1aad9b348868d2df5286dcb5a08118955a52ec62fce6668383ea4243f7929`. Evidence: ignored `analysis/runtime/world-handoff-final-build.log` and `world-handoff-startup-validation.json`; test-only manifest: `manifest-runtime-resolution-world-handoff.json`. The owned publisher GPU test also passed real producer/consumer fence stalls and new-source pixel verification in COMMON, RT and PSR idle states. These tests do not execute an actual game checkpoint transition. Neither the player default nor the tracked runtime-size candidate manifest has been advanced to this build. Next validation must exercise actual native retirement/recreation and publication across a checkpoint, and stop during the transition; terminal shutdown must also remain correct.

### First actual checkpoint trial and bootstrap correction

Game 36304 / session `9c944910c30a4538b9a40c6594ccbb0c` successfully retired the old owned resources and committed world 1 to 2, but new-world submissions remained zero and publication stopped at 4286. This checkpoint trial FAILED despite the runner reporting an orderly successful shutdown; independent cleanup and entry restoration passed. The captured status and explicit failed acceptance receipt are in ignored `world-handoff-36304-evidence/` and `world-handoff-36304-result.json`. Four profile files were restored after normal game exit.

The observed stall matched a concrete host guard: after handoff arm, the old trial ticket remained nonzero while candidate selection was closed, so the UI single-frame gate rejected the bootstrap renderer needed to run RHI recreation. Commits `5c95a77` and `a0bebd2` admit exactly one certified bootstrap without clearing the old ticket or releasing pending work. Recreation requires this claim and still rejects stop, wrong epoch and wrong RHI/TLS. The rebuilt DLL SHA256 is `17f2cf7d7e0d09520717e11716b6d9c9610218b98b986a02d38ceb8c4c889026`; 29 startup/component invocations passed, including 427 Owner CPU checks. Actual checkpoint recovery remains unproven until the repeat trial publishes new-world images.

### Repeat actual-game checkpoint evidence

Session `21e09aedbfa44fc9b6a756e4be69fbe2` (game PID 31392; exact identity recorded in `analysis/runtime/world-handoff-31392-result.json`). Two state-checked background checkpoint reloads completed worlds 1→2→3. Each new resource generation produced two different 2560×1400 GPU samples with distinct, nonblack left/right halves, and XR explicitly recorded replacement projection for generations 2 and 3. The final native counters were 4,894 published pairs, two resource recreations and 1,174 submissions in world 3. No owned error was reported. Normal stop confirmed no retained resources, no cleanup timeout and independently verified restored entry bytes. The game closed normally and all four profile hashes were restored.

The independent `manifest-runtime-resolution1.json` now selects this tested handoff/bootstrap candidate; the player default remains unchanged. This covers two ordinary reloads of the tested checkpoint and terminal cleanup after them. It does not cover a stop racing with handoff, mounted gun use, DLSS, other maps, real headset quality or uninterrupted frame rate. The test includes intentional menu pauses and loading gaps; the runner's maximum submission gap must not be advertised as normal gameplay latency. Evidence is under ignored `world-handoff-31392-evidence/` and the result JSON above.

---

## 中文

# 运行时间推荐的渲染大小: 需要 0.0.1 default

发布优先级: 此模式取代基于游戏分辨率的 VR 渲染
0.0.1 并优先于 DLSS 兼容性。
使乘数; 1.0 使用 OpenXR 运行时推荐的每眼尺寸.
游戏解析度不选择 VR 渲染维度。 剩余验证
默认情况下的释放工作,而不是将其推迟到 0.0.2.

## 默认配置回归, 2026-10-19

### 当前默认: 引擎延伸的固定

默认并命名 `manifest-runtime-extent-fix1.json` 现在选择
`GearsCaptureRuntimeExtentFix1.dll` SHA256
`6ee31a6fbd48302ba99096d1fda77f0952099b5ad3df722773299c539eb7cd93`.
外加 外 关 0.71 反事实,背景回归1.0 和
2.0 x 每个检查站通过两个重新装货、清理/进入恢复和五个文件
恢复为 a1280x720 桌面。 GPU 源代码为2560x1400 and5120x2800;
保存的图像检查确认旧桌面边缘以外的景色, 不
右下方矩形孔。证据前缀:
`resolution-extent-fix1-scale{1,2}-20261019`,加相应的图像/覆盖
被忽略的收款 `analysis/runtime`全部168 发射装置离线测试通过
升级后,释放2 仍然是一个命名的历史倒转,而不是默认。

当前发展ZIP:
`analysis/runtime/GearsVR-0.0.1-resolution-extent-fixed-20261019.zip`, 莎莎 (英语).256
`7e9deb70debafc7790b8608640fc0e18536dc65ddac98a0640549be2da5ce814`.
新鲜的舞台和提取的包件,孤立的飞行前通过;717 津巴布韦执行方案成员
对照有效载荷库存核实。
`analysis/runtime/resolution-extent-fixed-receipt.json`中,此选项取代
先前的解析- 默认准备/ 反馈档案, 并且仍然是测试包 。

桌面现面仍然变成8x8 运行,生成
模糊的桌面镜像。 这是单独的已知限制; 没有声称
桌面镜像、挂载枪、DLSS或物理头套接收完成。

**反事实恢复小窗缺失场景区域.** Candidate
`build-runtime-extent-fix1/GearsCaptureRuntimeExtentFix1.dll`, 莎莎 (英语).256
`6ee31a6fbd48302ba99096d1fda77f0952099b5ad3df722773299c539eb7cd93`,冻结
验证到每个世界/台表的原始源输出维度
对比了清晰的视门 与冷冻引擎的高度
保留目前的库存/排队/自有资源/国内例行检查。
来源范围是scalar证据,而不是DSV分配证明.

新的编译方式469 跟踪依赖性、重点突出的组件测试和
连续启动套件通过。 实际背景运行
`resolution-extent-fix1-run1-20261019` retained1280x720 桌面/ 数据8x8 现职和
1818x994 VR 输出。 检查点后, GPU 的图像都经过目视检查 :
右/下方的黑黑区被场景几何所取代。
场景特定亮度占用权限x1280 玫瑰从0.061% to60.7%,以及以下
y720 from7–8% to85–86%这些门槛仅适用于已知的检查站,
并非通用的暗框分类。1570 明确的应用程序和
1570 恢复;两个检查站恢复、本土清理、入口恢复
并通过了五份文件设置/简介修复。
`resolution-extent-fix1-coverage.json` 运行前缀 `analysis/runtime`.
在这个反事实阶段,默认/便携式清单仍然被选中2;
以上晋升取代了这一状态。2 区域投资方案
仍然包含被观测到的bug,不应用于此接收.

目标观察证实先前缺失的路径:诊断DLL
`47ada7fa94d7a47fd28008f28fbd65ebe9e1bc90c00d009314ba2c51569956d1`
已捕获的自有清晰的视图 1280x720 返回原住地时 1af345/1ade10/27e752,
准确的当前同排的库存 8x8 和自愿计划1818x994.
现有的桌面宽度守护器拒绝了( 理由)9。无关的80x56 depth
明确有不同的本地路线,必须排除。
守护不匹配;纠正它仍然需要一个GPU反事实,而不仅仅是计数器.
新鲜的构建、重点测试、连续启动套件和有界限的背景
运行的寿命周期/清理通过。运行在设计上仍然视效失败 :
`resolution-extent-observe1-run1-20261019.final.native-status.txt` 存储收据。

**小窗口运行后发现的视觉故障 :** 用户截图和
检查拯救的世界3 GPU 源码
`continuous-stereo-44132-20261019-091759-f5419aaf.gpu.ppm.1.ppm` (见跑步赛)
精确选定样本的视觉路径)显示大面积的右下黑区域。
The 1818x994 分配和成功的世界交割并不证明是完整的
自动覆盖。 `passed` 仅涵盖维度/寿命周期和
因此,小桌面独立不被接受;
不将此候选选项公布为解析完成。 突然覆盖边界
似乎与 1280x720 桌面范围, 但责任人渲染
检查实际的视图/剪切/深度/后期进程
覆盖,而不是改变纹理尺寸或宣布模拟器故障。

小数背景回归也经过:比例 0.7 produced 896x980 eyes
and 1792x980 两个检查站重新装入后, SBS 都装入。 Space 0.71 产生的奇数
909x994 眼睛和眼睛, 和 1818x994 两次重装后 SBS 。 后者暂时使用
1280x720 桌面设置; 已确认只读 HWND 客户端大小的观察
1280x720 VR GPU 源源未变 1818x994 都经过清理
条目恢复和保护文件恢复。证据前缀:
`resolution-default-background-scale07-20261019` and
`resolution-default-background-scale071-smallwindow-20261019`; 实际桌面
measurement: `resolution-default-smallwindow-client-size.json`。所有内容均被忽略
运行时证据。它包括分大小/ 缩放眼宽和一个较小的
桌面大小; 这不是物理头集质量或性能测量。

背景默认回归 : `resolution-default-background-run1-20261019`
随比例递减而通过 1.0 模拟建议 1280x1400 每只眼睛 两只
检查站重新装入已恢复的新世界 XR 投影; 两次重新装入后 GPU 源
图像匹配计划 2560x1400 SBS 大小 本地清理, 独立输入
恢复、所有者正常退出和恢复5个受保护的设置/简介
文件通过。驱动程序选择了窗口后选择的游戏和模拟器
策略, 声音不响, 并且要求不启动或移动 。
这是实际的游戏/模拟器证据,不是物理耳机或挂载枪
完全身份/状况/图像证据仍然被忽视
`analysis/runtime/resolution-default-background-run1-20261019*`.

启动器预览后续: 更改运行时间选择/ 路径, 检查大小 ,
或启动 VR 清除上一个大小标签。失败的大小命令显示 a
失败而不是保留旧运行时间的尺寸。 GUI 重建了
并且其离线自试通过; 没有进行交互式/头套测试。
寄存器发射器包括此固定; 下方的反馈 ZIP 早于此
仅限图形用户界面更改 。

当前综合测试包是
`analysis/runtime/GearsVR-0.0.1-resolution-default-ready-20261019.zip`, 莎莎 (英语).256
`6cab48aa895cb5eac2adf52b9a72f00c15f442ada09d22aebd2439aab06d36c2`.
包括预览修复和大小诊断
完成ZIP核查,并在提取后进行隔离飞行;
已提取的默认值是启用的运行时间大小, 缩放 1.0,英语。
二进制不变。 证据 : `resolution-default-ready-receipt.json` under
ignored `analysis/runtime/`。这仍然是一套发展计划,而不是发行。

诊断输出现在保留了发射装置推荐眼尺寸的最新数据,
选中的乘数、 计划中的眼尺寸和 SBS 尺寸。 它标记这些
而不是将GPU输出量 归结为更老的
无效或不一致的计划会被删除,同时发出警告;任意性
不包括运行时路径。
通过了舞台/包装套房 140 离线测试。 此源更改需要
新鲜的便携式包;以下原始存档的ZIP在它之前.

更新的开发档案是
`analysis/runtime/GearsVR-0.0.1-development-RuntimeRelease2-feedback-20261019.zip`,
SHA256 `56be16ccff0e3541a30e98fb8fda8f1362b43db8e280feeb92521f155db0c43e`.
All 717 归档项已核实,然后提取到新路径
。捆绑的孤立的口译通过导入/PE/hash 预飞
716 有效载荷文件。 收到 :
`analysis/runtime/runtime-release2-feedback-extracted-preflight.json`.
没有推出游戏、模拟器或图形界面。 原生/ XR 二进制没有改变; 此操作
更新反馈导出,而不是挂载/ DLSS 接受。

RuntimeRelease2 现已默认发射装置并命名为 Express
`manifest-runtime-release2.json`. 土著性健康协会
`249aeb07684891c1a7d1813b732a4e8e185b36b2df5827f99659527ee0b26b5f`;
XR 文物不变,保存了集成1 特征旗,
允许通用深度清除,并禁用源像素和大小/draw/quad
追踪收集器。
启用。新的汇编、6个重点本土测试和持续
启动套件通过。 此精确的构建通过比例2 默认比例1 actual
游戏运行,每个游戏有两个检查点重新装入,最后的GPU回读,本土清理,
独立条目恢复和五文件恢复。2 最后图像
满座5120x2800 覆盖;收回收据455->0 and400->0 零,则
能力缺陷。 `runtime-release2-scale{2,1}-run1-20261019*`.

发射/交付计划/阶段/包装套房通过110 新鲜便携式
stage `analysis/runtime/portable-runtime-release2-20261019` contains716 files;
孤立的口译/进口/PE/hash飞行前通过。这是一个发展动态
文物,不公开释放接受;挂枪,剩余UI、DLSS和
耳机核查仍然是单独的要求。

实际开发的ZIP后来被快速验证,其完全成员
清单已核实,并提取到包含空格的新目录。
其捆绑的口译器通过了对所有人的孤立导入/PE/hash飞行前716
有效载荷文件不发射图形用户界面或游戏. ZIP SHA256:
`fd4be4e7ec48e5ae3c87da4ad1ce7e532b17097ada65d9acd2cbea7758e19110`.
Evidence: `analysis/runtime/runtime-release2-extracted-preflight.json`;
driver: `analysis/runtime/verify-release2-extracted.py`。这验证了
已发送归档的离线依赖关闭, 不安装或头盔
另一玩家的机器上的行为.

RuntimeRelease2 也通过了路径选择自动周期,时间为1.25x
(1600x1750 每只眼睛,来源3200x1750 与清洁包装HUD65 setting.
Session2808 `continuous-stereo-2808-20261019-075038-d22d0000` 获得一个
团体职位,持有42 秒为10 正常选择后续修并恢复.
最后应用/修复的计数器1/1;拒绝、未解决和服现役
计数器是0. 原生清理和入口恢复通过,游戏关闭
通常,并且所有五个备份设置/配置文件都被恢复。
Evidence: `analysis/runtime/path-fit-release2-scale125-20261019.json` 财务报告和已审计财务报表
启动器日志。 保存了四个 GPU 读回文件; 没有新的头盔或锤子拾取器
测试明确允许自有游戏前缘,
变音游戏音频并使用只使用儿童模拟器运行时间.

同样的深度要求1 DLL 也通过了比例0.7: 运行时间建议
1280x1400 produced896x980 每只眼睛( SBS)1792x980 两个检查站都重新装货,
本土清理、独立入口修复和5个文件修复
。 GPU 预览显示, 有两个完整的场景在大小上是一半 。
重新索回收据显示375->0 and378->0 活动身份,不溢出。
本次低尺寸运行中没有接受深度清晰的视网膜突变;
它的流传图像不能证明发生了小大小的突变。
如下所示,为实际变异证据。
`runtime-depth-reclaim1-scale07-run1-20261019*`届会37060.

DepthReclaim1 (`2f9276a7e818047ec68f08db165102544918d935cbc520fafba13e45f91ee9cd`)
通过新建筑/组件/启动检查和规模2 重复为 2
检查站重新装货。现在在明确联系之前就已核证退休
复垦,在现有会话/框架锁和封闭式接纳下;
以后将解除COM的租约。
工作仍然无法退休。
列表代数和粘性断层; 测试覆盖768 寿命列表身份,
过时的参考文献/重复使用,以及未删除证据而未能退休。

实际收款情况显示世界1 active476->0 和世界2 active404->0,无容量
过错或列表溢出; 最后活动326,终生高水位476. 每一个世界
已接受通用深度清晰收据( Q)1916 applications/restorations
两项最后决定5120x2800 重新读取保留旧的场景内容
桌面宽度/ 高度。 两次重装、 本地清理、 独立条目检查、
普通的近距离和五文件恢复全部通过。这证明了测试
跨世界积累案例,不是一个世界中的无限身份.
Evidence: `runtime-depth-reclaim1-scale2-run1-20261019*`届会40880.
尚未核实是否建造释放装置。

DepthGeneral1 用验证的尺寸取代试验的固定尺寸
。它比较桌面当前库存与
仅读的单锁当前已接受锚定的快照
互换链绑定,包括每个缓冲身份、尺寸、队列和
生成。只有认证本地端的精确完整的桌面视图端口
已更正的 VR 视图端口保持不变。
接受的收据现在有单独的16- 银行被拒收的钞票
新鲜的建筑 SHA `1fdb2f22053a20a884ce18dcac9ca0ee051036ada413b73f0b6fe1d745dad97a`
通过了自有深度清空试运行、框架链接试运行、源像素/NGX检查和
连续启动套件。帮助测试覆盖多个矩形
大小、无效计划、呆滞目录和调整大小失败。

实际比额表2 session32040 **未成功接受生命周期** 第二届大会之前
检查点重新装入完成。 它确实应用并恢复了1563 通用的清晰度;
world2 ticket449 人类发展报告来源
`af210e65632ba0920d1e6332addf6aa2d9be4e9224f8bdbbd11ec2190b773898`
has99.9934% x 以外的正像素3840 and99.9987% 下层为y2160。此选项支持
通用范围计算、未完成生命周期或较低尺度
接受。现在记录了确切的失败: tracker_create 子类型6,
active_lists512/list_capacity512,溢出时0->1,容量为负值0->1 仅选择60
保留创作权的租赁是存在的,因此它们各自的能力没有受到影响。
车主停止使用自有的14/防_失败;本土清理超时.
后被套在游戏正常状态下恢复了5个文件,
但是,条目恢复没有经过独立的核实。
将正常进程退出不计为本地清理成功。 Raw status
存档于 `runtime-depth-general1-scale2-run1-20261019.failed.*` before
任何新运行。下一步工作是认证后安全跟踪器身份的恢复
退休,然后重复高/低倍数生命周期测试。
还没有推进到这一诊断性建设。

DepthClear2 反事实恢复了先前缺失的高尺度场景
区域。 DLL SHA 数据 `c6383e60ea63f9cdb71a0a759d274aae59ed2dd32e429c80f975e952d0c5c821`;
新的编译、组件检查和连续启动套件通过。
实际会议 `continuous-stereo-45744-20261019-065223-169d596a` 完成两个
检查站重新装货,正常清理, 77 独立输入字节检查和
恢复所有五个受保护的文件。
GPU 视图端口, 围绕已测试的字节深度清晰绘制当前自有的图
现场,从3840x2160 to5120x2800。然后恢复。运行时计数器报告1906
应用和1906 修复; 两种本地块散列匹配。

World2 ticket490 HDR 源文件副本1 SHA
`e36e4120f3bffb9b712d46f5998835f4ce086908c68030479fe4775dac0e08a6`
has99.9995% x 的正像素右侧3840 (上一个基线:全部为零),和
99.9984% 下层为y2160 复制2 独立99.9968% and99.9987% respectively.
源头和最后XR回读预览显示,在两个半边部分中都覆盖了全部场景;
后装入最终 GPU 图像覆盖5120x2800 超过97.6% 占用人数
。这些场景特定测量和
视觉检查支持 沉积的深度清晰的视门 作为原因
已观察到剪辑。 它们没有确定每个渲染效果或头盔
质量,也不解释所有历史黑幕。

审判仍然是默认的 关闭和硬码到测量反事实,
通用必须从
认证的运行时间计划,并保留本地路线/所有权的验证。
Its32 诊断行中填充着世界前被否决的不相关的图纸2;
仍可提供总账,但每张票接受收据需要
使用 DSV 资源
维度仍然未观测到。
`analysis/runtime/runtime-depth-clear2-scale2-run1-20261019*` 和原始读取
under `tools/native-capture/build-runtime-depth-clear2/`.

Attachments1 实际运行已完成两次重装和清理。 DLL SHA
`d2be0253dc8fcdd6e7a85811d28616dbde05b939969bb54948706c3b6b446193`.
Ticket528/world2:1269 直接画图,120 组合,不溢出。场景 MRT 槽1-3
are5120x2800,格式24/26/29; 第一个场景颜色插槽5120x2800.
两名只深度候选人(正文)1 and1230 使用( )3840x2160 视图和
相同的原始DSV 手柄145b43cc0。两个观测到的堆栈包括原生返回
1af345 ->1ade10 ->27e752 ->119347 ->15a943. DSV资源规模仍然未知.
Cached27e580 伪编码和新出口的历史组装显示其
27e74 d 调用出入色调 edx0 和可深度 r9d1 to1ad75 c 在此之前
。因此,两者应明确一个具体的深度清晰度
仍然使用桌面视图端口的候选;因果像素效果仍未测试.
历史片段27e580..27e752 and1ad75c..1ade10 匹配源块
911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6;
hashes dd5aeb84d7bb96ebeb1b0d2f24f37ba12127dac74b872be038632fd94cdf0cea
and34142122907196b48fa7c7b7ab28508837420c5a8e930c7c2b2f38fe57172622.
关闭了开发协会私人租约。1-scale2-run1-20261019
文物和 analysis/ida/ngx-copy-contract-20261019/clear-route-chunks.json.

SourceRanges1 缩小视图假说。 DLL
`1f4dc427d2a759cde44c2066bee5e1fff2ca95b26b022d25c3306405a639b7ed`
通过启动/组件测试,并用清洁设备重新装入两个检查站
恢复。票503/world2 records1135 非空直接抽取113 exact
范围组(限制)128(b) 不溢出;659 绘制缺少第一个颜色目标,
缺乏眼力的,读取源头的16ae081640 仍然有,没有
全零列 x ⁇3840 生的SHA(原始内容存档于2018-09-21).
`3e6b4f13617e555552b67ece9e0b13e7114c8b811000c0e29c951c09af6e19f2`.
匹配源写入的每眼全数或正确5120x2800/2560x2800
视门,有几根无限制剪刀;没有通用的3840
剪刀绑在这些观察的文字中,有些用四个渲染目标。
排除了通用的直径视窗/剪刀绑定,而不是
所有上游剪辑:只有第一个RTV解决;其他MRT,绑定DSV,
计算/复制/清晰/间接写作不在本诊断书中。
下一步解决这些写作中的其他 OM 附件。 已有的 RTV 描述符
钩子可以覆盖所有槽; 现有的视图深度_ 已知的只有 MinDepth/
最大深度,不是深度缓冲身份,不能作为DSV证据使用.
Evidence: `analysis/runtime/runtime-source-ranges1-scale2-run1-20261019.json`,
`.final.native-status.txt`, `.final.native-status.source-ranges.json` and
忽略构建运行时源区域1/owned-hdr-source-41744*.

SourcePixels3 提供实际的中期人类发展报告证据。
`f4fcced4ca35b72c9421960e7c15e7ec9c7e442b69c5a2ac867583366c143309`
通过了源读回和 NGX 回归测试加连续启动
套件。实际会话 `continuous-stereo-27764-20261019-062549-a0ef966e`
完成了两个检查站的重新装货、本土清理和防护文件修复。
上一个错误14 没有重现: 属于自己的跟踪器高水量459/512 不得溢出;
新的首发故障子型诊断仍然可用,而不改变政策。

罚单495/world2,返回 RVA 时,有两个相继的本地颜色副本1c6093
抓获了同一条线索16c3184880/destination16c317fc00,格式26,
5120x2800。在命令名单退役前,既完成了真正的排队围栏,也完成了命令名单的退役
readback; 所有像素解码为有限。 两者都包含完全零 RGB 整个
x>=3840 (3,584,000 像素),预览中包含不对称场景覆盖.
原始散列图 : `a5cd13d7421bd8bb5ad5940cccc2fa2d5b39102119188a95c43294a2739b7471`
and `1ba7406d44e83df55c8e88b585201037ce3d03a24f1245e9cd46a83e7071a5ff`.
证据被忽略在构建- 运行时源- 像素下3/owned-hdr-source-27764*
and analysis/runtime/runtime-source-pixels3-scale2-run1-20261019*.
此处定位到中间原生色源中缺失的像素, 在此之前
XR运输;它尚未确定负责或证明准确的生产者
最后产出协会。1,不独立
认证的眼内容。下一次检查生产/通过
源代码,而不是进一步改变 XR 输出大小。

SourcePixels2 诊断性后续行动:私人建筑
`a79ade0afe7b00df7bfb5a635bf4401edf6bcb13fc7244e2114865263d96831a`
建议输入的预测数1280x1400 times2,但在第一个阶段失败
检查站重新装入: 自有的误差14,停止_理由证明_失败;本土清理
尚未验证。 没有获得 HDR 原始回读。 这是失败的诊断 。
无法证明源像素或检查站重建是正确的。
协调员随后请求正常的游戏退出,并恢复了全部5个
保护配置/ 配置文件与匹配散列。 原始失败
报告保存于
`analysis/runtime/runtime-source-pixels2-scale2-run1-20261019.json`,与
separate `.recovery.json`。诊断不是发射装置默认值。
所需数额0.0.1 更换路线和优先级高于DLSS保持不变。

### 将图像缺陷按比例封存 2.0

合成运输检查,2026-10-19: 非公开
`analysis/runtime/high-size-share-test.cpp` 重新使用真正的 GpuPullisher 和
带有自有设备的 Gpureaver5120x2800 RGBA8 目标,独立于任何互换链。
四个不同的GPU清晰区域横跨x3840 和 Y 类2160。在实际共享之后
出版、获取和全脚印 GPU 读取,每个 RGBA 频道
of14,336,000 匹配像素( 0 个差); 票和尺寸匹配,
ACK/drain/接近成功。 D3D12 调试层没有可用( E)`false`).
这支持传输器在桌面大小之外保存像素的能力;
它不证明现场游戏选择正确的源/状态或制作
正确的场景像素。 源 HDR 仍然需要回读。 证据 :
`analysis/runtime/high-size-share-test-result.json` 并构建日志/源代码。

拥有的后处理跟踪: 运行时四面体跟踪1 DLL
`95137583c2057582f51446993198684e92c62e6196c1ce2795a9db67a60a2dcd`
passed22 启动检查和两次重装,正常关闭/恢复。
Session `continuous-stereo-45540-20261019-055214-cba73105`票号487/world2:
40 命令四平,不溢出。83b1 c 行数20/39 正确
left/right2560x2800 目的地/源矩形,5120x2800 target/texture
维度, 实际RTV5120x2800 完全的视门,和不剪刀。
双绑定槽0 actual5120x2800 format26 颜色纹理,中间行
包括缩小大小的通行证; 缩小尺寸本身并不是错误的证据。
Rows0/1 未知的RTV 身份( 资源)0),而不是缺少输出的证明。
修正的取样-缓冲器现在读作符合实际分配的维度。

重新装入后源重读的频道上方仍为零像素4 in
columns3840+. 因此,CPU最终方位维度不能解释作物。
需要实际中间色像素与最终输出,或独立
已校验的阴影/ 恒定证据; 不要推断描述符中的正确像素。
报告/完成情况在下文中仍然被忽视。
`analysis/runtime/runtime-quad-trace1-scale2-run1-20261019*`.

后世反例:运行时SizeTrace3, DLL 键
`19417a286f414f9a706be33fa7916ff2f905ed70657512d437570c4f279e8df8`,
passed22 启动检查和两次实际重装,正常恢复.
Session `continuous-stereo-46068-20261019-054626-e3d3b745`票号481/world2:
已建/预/后取样 `.25,-.5,.5,.25` (左)和
`.25,-.5,.5,.75` (右),有正确的2560x2800 眼睛矩形和观察
几何视图/5120x2800 目标。然而,两个重装后 GPU 仍然
频道上方没有像素4 列数3840..5119。因此,陈旧的建筑师
取样是 **没有充分解释持续剪辑的原因**;
不提倡四浮采样补丁作为固定的。下一次检查实际
拥有后进程/最终源边框和副本。

Trace3 额外 `owned_ticket_sampling_buffer` 列是 INVALID 证据 :
二进制读取了误导性IDA标签的RVA1c3a3b8 独立
视图/目标/绘图行仍然可用;资料来源现在已读取 RIP 已确认1c3b3b8.
诊断世界过滤器是只建(`GEARS_SIZE_TRACE_MIN_WORLD=2`).
Trace2 从未在比赛中运行: 超度 CMAKE_CQQFLAGS 省略 `/EHsc`,
和视图例外 -- -- 清理自试31 失败。 追踪3 全部恢复( E)
原版旗和游戏开始前通过完整套房.

2026-10-19 边框自有- ticket 跟踪: 私有运行时SizeTrace1 DLL
`91d2cbe6bd1074664c56129bd53807e720120f23bb377b0802e8578fdcb79ea0`
传过 22 启动检查和两个实际检查站重新装货,正常
清理/恢复。 会话 `continuous-stereo-34916-20261019-053954-4fba6fbe`.
Ticket2/world1 拥有全部8个源/ 已建/ 预置行( 任务)127: 自主目标
5120x2800,并准备眼矩2560x2800 时值 x0/2560。第一个
观察到的每只眼睛的几何图样5120x2800 右侧
眼睛视门和不剪刀 这是直接拔出的证据
并不是所有通行证都使用正确尺寸的证据。

不过,已建成的取样载体
`0.333333343,-0.648148119,0.648148119,0.333333343` 以左眼为正眼。
最后一个组件1),与分母一致3840x2160。它保持不变
构造观测 QPC2211057222908 实际数之前
资源大小调整QPC2211057579650..2211058176743;准备从后开始。
历史快照组装被重新导出, 并按字节排序
116250..116b34: at (中文(简体) ).116590/1165 bf 读取全局1c3b3b8/1c3b3 bc 得出数据
view+3d0..3 dc,独立于目标虚拟宽度/高夹.
切片莎( Slice SHA) Name256 `b3614facc38c953bc9b2ead6c8cca42783225619757a630d06b9d1982733a07e`.
使用原始 RIP 相对字节修正了原始地址抄录:
`8b05224eb201` at116590 and `8b05f74db201` at1165 bf. 开发协会的cs显示标签
are 0x1000 低于实际目标; KB xrefs 同意解码目标.
证据被忽略 `analysis/ida/ngx-copy-contract-20261019/renderer-sampling.*`.
此处识别样本的构造器/调整大小的顺序差异
ticket, **是否持续剪辑,尚不完全说明**双向
重装 GPU 图像后, 通道上方仍为零 像素4 列数3840+.
下一步在选择前先建立后端盘样和后端进程界限
修改寿命边界。没有推广新的默认包。

已测试的源头候选人; **不足以确定覆盖范围**: 源视图
建筑现在得到与制作人相同的自有家庭/目标副本
建筑,而原源/家族认证和不可使用性
历史集会 15efd3–15 efed读取家庭目标维度
改为+60/+64 and 15f056–15f07 b 得出取样值。
15ef46..15f171 和匹配源字节;切片 SHA256
`aa69699eb320146c91b18aec3a0d8182d8bbb8246f8a61cde0d2bc977fa80c4e`.

私人建筑 `build-runtime-source-owner1` (DLLSHA ) (打倒沙发)256
`9f1c581f07091dea5789dcb38fb63a0619386eaa3a302baf1841da1eb3378f25`)
编译为 462 跟踪依赖关系; 22 连续的启动检查通过。
Actual `runtime-source-owner1-scale2-run2-20261019` 完成了两次重装,
77 恢复输入检查; 五个文件散列独立匹配, 没有游戏
都留下来了,但两者都一样 5120x2800 GPU 图像在频道上方仍为零像素
value4 列数3840..5119。第一个图像经过目视检查;
原右/下方缺陷依然存在。256
`3a7639ec12ceeccb60e10e0a6813c3d51830d76e4ab5bd4923c2f5c084820eea`.
不要推举这个候选人, 或声称建筑师的错配是唯一的
。下一个证据必须跟随自有刻板尺寸通过渲染器
施工/准备和场景通行证;现有第一帧痕迹
脚步拖曳观察。 运行1 在游戏创建前停止, 因为新鲜
构建缺少图形化 MenuLock Test; 在运行前创建缺失的目标2.

Actual `runtime-integration-checkpoint-scale2-run2-20261019` 完成两个
重新装入,新世界XR投影,正常关闭和恢复。
`passed=true` 仅涵盖机械检查: GPU 文件均为 5120x2800
协调员对第二幅图像的检查显示,
黑右/下方区域以及场景内容显然仅限于旧的
3840x2160 extent. **缩放不成功2 图像接受。** Investigate
接受运行时间前的实际场景视图/目标/视图端口尺寸
大小默认为已放行。此优先级高于 DLSS 。

Session `continuous-stereo-2412-20261019-051846-06cd3de6`; 报告 SHA256
`25f528fd7931f0dbbd83667ab5b9a60e206dc4c59caa5b973a5495477c7faaf3`.
第二张 GPU 图像 SHA256
`e1b8adc1362a02d7b38c97c3cdda61f2e1b0f069fb2852a9807d1abf604be250`.
没有建立精确的每盘图像协会。 77 entry
检查和5个保护文件散列通过;没有游戏继续运行。
Run1 停止在新菜单对象索引中超过缓存的目录; 运行2's
私人司机每次观察时使用新读取器,保留精确的两读
输入检查。没有削弱生产菜单拒绝规则。

发射套件找到了推广的集成1 默认缺少相同的
名称为 清单。添加 `manifest-runtime-release-integration1.json` 与
当前的经核实的文物散列; 活动清单和二进制没有变化。
默认回归现在明确要求运行时间大小和规模 1.0,
并接受用户乘数 0.25, 0.7, 1.0 and 2.0 全部 74 发射器试验和
4个制成大小测试合格证。这些是配置/CPU检查,而不是额外的检查
耳机或高分辨率GPU接受.

## 综合默认:两个实际检查站重新装货, 2026-10-19

默认晋升到整合后1 (国内卫生署)256
`41a5b882ab1202940c0590d88a6c732609fd0db9511157e19f51f802064cc065`,
启用了 PathChoice 服务 , ), `runtime-integration-checkpoint-20261019-run3`
在同一游戏过程中完成了两个正常的重新装入当前检查站的工作.
Session: `continuous-stereo-9480-20261019-044100-2fdf1b18`.
发射装置验证了儿童模拟器的建议 1280x1400 每只眼睛
比额表1;没有使用桌面设置来选择 VR 维度 。

世界一代先进 1→2→3 并进行计数 0→1→2。每代都有新的
本地出版物/目录和匹配的 XR `world_handoff_card_ended` receipt
with `world_valid=true`, `quiescing=false`, `replacement_projection=true`.
每个新 GPU 文件都已完成 2560x1400 与经认证的计划匹配的SBS;
协调员看了第二张重装图像,确认了两张现场眼睛
原生清理、入口修复、所有者退出0 独立
原始的 SHA 5 文件256 报告 SHA 已恢复。256:
`cd5d64c933382ef6d6040cf46e121f91a23db86a805db68b58e8480af4e87c91`.

Runs1/2 每一个都完成了第一次重装,然后测试的第二个菜单打开的守护
拒绝已观察到的播放器_index0 因为它只接受了初始1.
Run2 保留精确的只读一致的快照; 现在的私用测试
接受这两个指数,并进行所有其他身份/现场检查。
12次离线驾驶员检查通过,未改变生产录取规则.
证据涵盖全球范围内的PathChoice服务,而不是
重新装填时开放的路径选择面板,所有检查站,中途退休站,
实时首集质量,挂载播放或 DLSS。运行时推荐尺寸×
发射装置的乘数仍然是所需的 0.0.1 默认, 领先 DLSS 。

## 中断的覆盖尝试, 2026-10-19

Follow-up `checkpoint-prepare-stop-actual3-20261019` 涵盖注销期间
准备,与未观测到的本土阶段分开1/2 种族。当前
默认请求生成 2 生成时 1 仍在活动;准备
计数是 1,交接阶段 0 和 笔 笔 0。然后完成所有者注销
一代人还剩 1 并实施快速控制 0. 本地清理,所有 65 entry-byte
检查, 正常游戏退出和五个原始文件散列经过并曾经
独立核查。这证明了所观察到的准备取消路径;
无法证明GPU退休或娱乐内部的中断。

`checkpoint-handoff-stop-actual2-20261019` 使用当前默认运算表,
scale 1.0,仅限儿童模拟器和哑音背景发射。
检查站确认完成;样本生成方式改变 1 to 2 编辑
本地犯罪 QPC 是 2143641623022 停止QPC是 2143650047135, **after** that
执行。测试报告正确 `race_observed=false`: 状况调查
已丢失的阶段 1/2,因此在资源娱乐期间不会证明中断。
原生清理,全部 65 恢复入境检查、正常出境和5次原始
文件散列通过, 由协调员独立检查。 时间越早
actual1 权限错误后在菜单输入前停止; 驱动程序2 使用该
现有共享删除的JSON读取器,并保留追溯/路径诊断。
原错误的确切位置没有记录,至今仍未证实.

## 默认模式促进, 2026-10-19

音频集成后开发软件包: 忽略
`analysis/release-zips/GearsVR-0.0.1-development-RuntimeScale-Audio-20261019.zip`,
13,979,905 字节, SHA256
`3fd496ac54b730317f8843fc36d4725227f0606036a1715e6d82918840d9d233`.
已上架的盘点和刚提取的包裹都通过了捆绑的Python
提取收据是 `runtime-scale-audio-extracted-preflight.json`.
此软件包包含默认运行时大小模式和包装的音频帮助器;
这并不是公开发布,也不是解决未决的挂载/UI/DLSS问题。

用户确认运行时间推荐的每眼大小发射器使规模
替换游戏分辨率 VR 渲染 0.0.1 并优先于 DLSS。
`tools/vr-launcher/manifest.json` 现在选择以前测试过的 手握世界
候选人来自 `manifest-runtime-resolution1.json`,与 `runtime_render_size`
启用。在升级前检查了本地和XR二进制散列。
发射装置默认缺漏比例表至 1.0;桌面分辨率控制镜像。
之前在下面声明玩家默认不变是历史
被这次升级所取代。这是默认模式的改变,而不是索赔。
剩下的所有东西 0.0.1 问题已经通过接受。

重建了目前的图形用户界面,通过了隐藏的自我测试并安装为
root `GearsVR Launcher.exe`四次变质试验和十六次便携式试验
测试通过。忽略的独立开发阶段
`analysis/release-runtime-default-001` 飞行前通过捆绑运行时间;
显示启用运行时间大小, 其配置集 compare_ scale=1.0,及其图形界面
匹配已安装可执行文件的散列。 接收 :
`analysis/runtime/runtime-default-001-preflight.json`.

本次升职期间没有新游戏或耳机运行。 实际检查点
证据仍然是下面记录的两款重载。
DLSS 兼容性,中断的交接和最后耳机/包装接受
开发阶段尚未公开。

## 第一次实际游戏证据 2026-10-18

### 升学后开始吸烟, 2026-10-19

隔离警卫启动前,第一个背景烟烟就被取消
观察游戏高于其他应用程序。 因此背景放置修正为
无法通用。 用户在夜间明确允许前瞻
操作,使用推广的本地/XR对输入的第二个烟雾游戏
和只使用儿童模拟器运行时间。建议 1280x1400 at 1.0 已产生
2560x1400 SBS 重读: 三份文件, 带有明显的散列和非零手段
两者都一半。这是模拟器大小的证据,而不是物理头集质量。
记录的XR 1353 立体声预测和 409 现场复制。会话停止
并核查本地清理和独立的入境恢复情况。

私人驾驶员最后的正常近环击中了Win32 error 1400 窗口之后
。根单独校验了没有游戏存在,并恢复了全部五种
散列设置/ 配置备份。 不要调用驱动程序本身完全通过 。
收件: 忽略 `runtime-default-smoke2-recovery.json`,
`runtime-default-smoke2-pixels.json`,以及
`continuous-stereo-32904-20261019-024011-980a4109.json`.

### Actual 0.7 x 规模烟雾

单独的子配置集compare_scale=0.7 ,而不改变用户的配置。
同样的模拟建议 1280x1400,实际的GPU读取是
1792x980 统计局(SBS)896x980 三个样本有明显的散列物和非零
表示两个半个。 `continuous-stereo-40272-20261019-024237-e8f3debe`
recorded 1344 立体投影层和 399 现场复制,然后停止
本地清理、 条目恢复、 游戏正常退出和所有五个设置/ 配置
已校验恢复。驱动程序容忍一个普通关闭窗口消失, 所以
无法跳过剩下的清理。
`analysis/runtime/runtime-scale07-smoke-evidence.json` 和匹配的全数
驱动程序/会话报告。这证明本建议的实际规模缩放
和启动路径; 它没有建立物理头集质量、帧率
改进或所有规模/运行时间组合。

### 核实的终端退休后续

Session `continuous-stereo-17496-20261018-192745-63abd9ec` 退休候选人的竞选资格 59.37 场景提交秒 : 1202 已出版的对/ 副本和 5347 模拟场景提交。 已到达本地关闭 `live_phase=4`, `shutdown_phase=3`, `retained=0`, `resources_retained=0`, `owned_shutdown_closed=1`,栅栏状态 2 且没有清理超时或自有错误。会话运行器独立确认本地清理和条目字节恢复;其范围会议结果通过。游戏随后正常退出,并恢复了全部四个预测试配置散列。 Raw status and GP U 样本被忽略保存 `analysis/runtime/retirement-17496-evidence/`。独立运行时大小候选人现在选择此构建;玩家默认不变。

这确立了一个实际游戏终端退役和短的持续模拟运行。它并不建立世界交接,在同一游戏进程内重启,挂载相机行为,DLSS,耳机外观或长会场稳定性。 `world_handoff_supported` 检查站的恢复工作仍不实,不得作为已交付的检查站恢复工作。

以上经核实的退休情况所依据的执行情况: `build-runtime-resolution1-retirement` 现在冻结本地/加拿大CPU排水管,拒绝未提交或未经认证的自有跟踪,关闭出版商/录像机,等待认证队列上设置专用围栏,发布诊断参考,并在原RHI上为现有所有人退休提供服务 线程/ TLS. 钩子在完成或现有清理截止日期之前仍然可供使用。 队列/ 资源/ 生成身份和会话- 内设框锁定命令被检查。 未公布的 UI 周期有单独的丢弃收据; t 尚未报告是否公布。 世界交割仍然被禁用。 重新完整汇编 。 24 启动引用, 31 WARP 队列检查, 375 业主检查和 2012 分类账检查通过。 这些组件结果在前面的实际游戏退休证据之前。 玩家默认值保持不变; 独立运行时候选人现在选择退休建筑 。

首/后 GPU 样本从下面运行,每个眼都有不同的散列,每个眼都有变化 1280×1400 半,这只排除了这两个样本的全冷冻眼;它没有确定正确的时间历史。 `resolution-material-24120-evidence/eye-temporal-comparison.json`.

Session `continuous-stereo-24120-20261018-185551-6eeae1ab` 采用物质政策候选人和模拟建议 1280×1400 at 1.0 原生引擎目标 2560×1400 with `built=1`,双眼画图/提交口罩 3 错误 0。它出版了 146 双眼; XR 报告 610 成功场景提交和 146 复制件,内容是 6.75 秒。保存的秒数 2560×1400 最后一个 GPU 图像可视化地包含两个半部分的游戏场景。 这是实际游戏源图像和模拟器提交证据, 不是头盔接受、 时间正确性或长会话稳定性 。

桌面互换链输出仍保留 8×8。协调员停止了对该观测的这一短暂测试;随后的日志和GPU图像确定,独立的VR路径仍然在产生景色。未来的测试必须区分桌面观察者和拥有的VR目标证据。没有窗口 使用移动或激活。游戏正常退出,四个预测试配置散列恢复。

本届会议仍为 **incomplete**: 本地报告 `resources_retained=1`,在进程退出前保留现场所有者,所以现有的清理门正确失败。它也报告 `world_handoff_supported=0`,这仍然是检查点/世界变化的交付缺口。不要放弃这些检查。 65- 库存与过时的 64 限制; 限制现在符合捕获. cpp 的最大值 87 插槽,保留所有每输入字节检查。 65 验证测试涵盖更大的库存、重复和超能力拒绝(18 测试通过。 Raw 状态和 GPU 图像均在忽略下保存 `analysis/runtime/resolution-material-24120-evidence/`.

该候选人执行 0.0.1 。上面的短小实际游戏证据不能建立完整的游戏生命周期或耳机接受。

`runtime_render_size: true` 在一个候选清单中,可以控制发射装置。 `render_scale` 默认在用户配置中 `1.0`,接受 `0.25` through `2.0`。每一次运行时建议眼尺寸乘以乘。尺寸向上到整个像素。当建议不同时,双眼使用建议宽度和高度。 3072×3264 yields 2151×2285 每只眼睛,在 0.7,或 3072×3264 at 1.0.

在游戏推出前, 启动者认证候选 XR 可执行文件并引用 `--render-size-info --render-scale VALUE`。此选项列出运行时间,而不创建 XR 会话。请求的维度超过运行时间限制或现有的 8192-像素 SBS 宽度/ 高度限制明显失败; 没有数值静默的缩小。 GUI 提供了“ 检查眼睛大小 ” , 显示返回的尺寸, 并为下个会话保留选定的乘数 。 `GEARSVR_MANIFEST` 选择一个显示在图形界面中的明确候选人,而不覆盖玩家默认值。

XR 制作人出版的不可改变版本 `RenderSizePacket` 绑定到游戏 PID, 处理创建时间和立体会话。 本地代码验证并冻结了这个计划。 本地引擎表面、 家庭适配器、 重建的视窗矩形、 GPU 目标证明、 图像刻板矩形和 XR 互换链必须商定 。e 尺寸相同。 拒绝使用不同于计划的桌面大小源。 此路径不会提升现有的桌面图像, 以声称更高的场景分辨率 。

现有的引擎渲染器为自有目标创建并消耗其场景资源。 容量再利用避免了在 VR 和桌面维度之间交替分配引擎。 桌面镜像渲染读取单独的快照, 并字母框显示左眼 使用其实际的侧面比。奇点眼高由回读路径支持。UI和材料路由保留其现有的源几何并使用计划的目标维度。

初始乘数固定在一课。 更改时需要新课。 候选人已经核实终端所有者在短时间实际比赛中退休, 但世界交接仍然被禁用。 历史的右眼/ 后置处理失败 。 候选人现在保留了默认的DOF保障:在自有出版边界需要准确的现有设置/方块证明,其票在边界之后而不是桌面现货时退役。al 拥有的运输, 没有运行时大小计划, 仍然被拒绝。 静态兼容性不能确定历史像素故障是固定的 。

完成执行构成部分的验证:制定计划/运行时限和桥梁身份检查;XR作物政策准确;矩形土建商和源保检查;自有地面创建/退休模拟(317 检查); 矩形容量再利用; 奇高WARP在三个闲置状态的回读; WARP 桌面镜像 (10,639 检查, 调试层无法使用); 发射器测试( )68 现有 + 3 新的);连续的跑腿测试(51; GUI 编译/自测试和不激活屏幕外布局检查; XR 翻译单元语法编译。这些并不确定引擎的深度/后处理分辨率、时间眼更新、游戏生命周期正确性或耳机质量。ose需要协调员单独进行候选的构建和实际的游戏/GPU/头套测试.

协调员应用了抓取/CMake集成和DOF兼容线条。 56 本地启动/组件/整合引用通过;自有的GPU固定包括 257 真实的 WARP 出版物,其中包含合成场景元数据,并验证不完整的 DOF 证明和世界变化阻止了出版。第一次验证引用省略了所需的测试参数;一次后期的固定缺乏有效的几何,以及第一次负测试 h 用于在国防部警卫之前进行资源状态的警卫。这些失败仍然被忽略。 `analysis/runtime/`被冻结的建筑物已经过关了

重点跟踪纠正了之前的平方化整合覆盖。 22–24 利用公共产品大小规划员和真正的WARP资源进行每眼 1280×1400 at 1.0, 896×980 at 0.7,并合成一个奇数高的推荐 1280×1401 at 1.0。前两个是协调员记录的模拟建议所衍生出来的;这些定点不列出运行时间本身。每个案例都创建了规划中的SBS资源,将左/右区域分解为不同的颜色,公布一个精确的票,ac 通过真正的共享GPU接收器进行 ⁇ ,检查票面矩形,回读源头和接收器,并匹配单独的眼花序和像素计数. 出版门需要匹配世界/ticet和合成完整的双眼 DOF 集 三位/等值证明,在桌面退休电话中幸存下来,出版后退休。所有三个重点案件都转而进行新的本土重建。原件 56- 引用报告适用于前一个二进制;它不是为这个仅测试的更改而重运行的。后续报告是 `analysis/runtime/runtime-resolution1-rectangles-validation.json`。证据解析器检测到一个初始缺失的 stdout 冲水,并分别保存。

确定长方形/高音D3 D 资源发布/ 接收器管道, 包含合成场景和 DOF 元数据。 它没有建立本地引擎 RT 创建、 真正的设置/ quad 执行、 引擎视图/ 深度/ 后处理维度、 OpenXR 互换链提交、 菜单转换或实际操作 耳机质量.

后续集成审查发现一个具体的UI回归:自有材料路径总是使用默认的HUD参数,忽略了现有的类型分类,HUD开关和用户布局控制. `material_overlay_plan.h` 现在保存了挂载104 政策:种类 0 使用普通几何; 类型 2/3 用户 HUD 参数和布局仅在启用时才使用。自有消费者保留其资源、命令列表和票证。 120- CPU比较包括分类, 45/65/100- 度值的用户视图、深度、布局、矩形和奇数高度目标、重播和恢复。新的全 DLL 构建加注册 `MaterialOverlayPlanTest` 传入 `build-runtime-resolution1-material-policy`; 主机观察仍然关闭。 这是源/ 构建更正, 而不是游戏/ UI 接受。 候选人名单随后在下面的启动检查后被提前; 玩家默认值没有更改 。

`tools/vr-launcher/manifest-runtime-resolution1.json` 最初选择了以下建材: 24 启动/组件引用时所参照的精确 DLL( 收到: `analysis/runtime/material-startup-validation.json`。默认值仍然在挂载104。下文仅列出各项结果,随后是上文记录的实际短赛;整个生命周期和耳机接受率仍未确定。测试报告和构建日志被忽略。 `analysis/runtime/`。本地忍者头目依赖检测不可靠,因为安装了中文编译器资源,所以该候选人的编译方式是 `cmake --fresh`; 不要将赤裸的重链接作为编辑了更改头文件的证明。 没有默认的显示或已有的玩家可执行文件被此组件任务所取代 。

## 世界在发展方面的交割部分

桌面镜像现在可以不用最终关闭镜像就退出源。 它会冻结捕获/演示, 投票时会同时使用 GPU 的栅栏, 并保留所有参考文献。 一旦被排出, 它只接受较新的源生成并保存 t。一个停滞的排队测试拒绝早期替换,然后绑定一个不同大小的源并读取新的绿色像素;终端关闭仍然无法重新打开。真正的 WARP 镜像套件已经通过 10,659 检查; D3 D 调试层没有可用。 这是组件证据, 而不是游戏检查点恢复。 框架链接的退出同样是一个外部认证的明确过渡; 主机集成和实际检查点测试仍未完成 。

协调员将这些阶段纳入默认操作后 `GEARS_OWNED_WORLD_HANDOFF`。它需要运行时大小的自有目标和UI批量路由。老CPU消费者在完成渲染器/UI之前仍然被允许,然后在GPU完成后发布并镜像解开其旧源。本地所有者退休时使用原始的 RHI 线程;短 闭路世界承诺验证资源/UI收据, 退出剩余完成的DOF条目, 提前日期和武器后创建 RHI。 票台计数器保持单调状态。 发布器重新绑定保存共享的槽和序列, 并需要相同的队列/ 设备 。/descriptionor,一个后期世界和较新的资源生成. 普通不相关的桌面命令不再取消特定源的退休收据; 过时的旧源使用仍然有效.

一份新的候选人完整汇编(包括capt.cpp)和 29 启动/ 组件引用在 DOF 整合后通过。 候选人 DLL SHA256: `e4c1aad9b348868d2df5286dcb5a08118955a52ec62fce6668383ea4243f7929`证据:被忽视 `analysis/runtime/world-handoff-final-build.log` and `world-handoff-startup-validation.json`; 仅测试的显示 : `manifest-runtime-resolution-world-handoff.json`。自有的发布器 GPU 测试还通过了COMMON、RT和PSR 闲置状态下真正的生产者/消费者栅栏档和新源像素验证。这些测试没有执行实际的游戏检查点转换。玩家默认或跟踪的运行记录都不存在。IME大小的候选人名单已经提交给了这一大楼。 下一次验证必须在检查站进行实际的本地退休/娱乐和出版,并在过渡期间停止;终端关闭也必须保持正确。

### 第一次实际检查点审判和靴子钉的改正

Game 36304 届会 `9c944910c30a4538b9a40c6594ccbb0c` 成功退役了旧有的资源 和奉献的世界 1 to 2,但新世界提交的数量仍然为零,并停止出版 4286。尽管跑者报告说有秩序地关闭,但这次检查站审判失败;独立清理和进入恢复工作通过。 `world-handoff-36304-evidence/` and `world-handoff-36304-result.json`。正常游戏退出后,恢复了四个配置文件。

观察到的摊位与混凝土主保相匹配:在交接臂后,旧的试票仍然为零,而候选人的选择则关闭,因此UI单帧门拒绝运行RHI娱乐所需的靴子护身器。 `5c95a77` and `a0bebd2` 完全承认一个认证的靴子, 不清理旧票或释放待办工作。 娱乐需要这个声明, 并且仍然拒绝停止、 错误的时代和错误的 RHI/ TLS 。 重建的 DLL SHA256 is `17f2cf7d7e0d09520717e11716b6d9c9610218b98b986a02d38ceb8c4c889026`; 29 启动/组件引用,包括 427 个人CPU检查。 在重复审判公布新世界图像之前, 检查站的实际恢复仍未得到证实 。

### 重复实际游戏检查站证据

Session `21e09aedbfa44fc9b6a756e4be69fbe2` (游戏 PID ) 31392; 准确身份记录在 `analysis/runtime/world-handoff-31392-result.json`) 两个经国家检查的背景检查站重新装入完成的世界 1→2→3. 每一新的资源生成产生两种不同的 2560×1400 左/右半身有明显、非黑色的GPU样本,XR明确记录了代代取代预测 2 and 3 最终的原创计数器是 4,894 出版的对子、两次资源娱乐和 1,174 世界提交的材料 3。没有报告自有错误。正常的停止确认没有保留资源,没有清理超时,并且独立核实恢复的条目字节。游戏正常关闭,所有四个配置散列都恢复了。

独立 `manifest-runtime-resolution1.json` 现在选择这个已测试的手动/ 跳板候选人; 玩家默认值保持不变。 它包括两个普通的重新装入已测试的检查站和之后的终端清理。 它不包含用手动、 挂枪、 DLSS、 其他 ma 来停止比赛 ps, 真实耳机质量或不间断帧率。 测试包括有意的菜单暂停和加载缺口; 跑者的最大提交缺口不能作为普通游戏游戏时空广告 。 证据被忽略 。 `world-handoff-31392-evidence/` 上面的结果是JSON
