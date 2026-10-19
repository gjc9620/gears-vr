# Runtime-recommended render size: required 0.0.1 default

Release priority: this mode replaces game-resolution-based VR rendering in
0.0.1 and takes precedence over DLSS compatibility. The launcher exposes the
render multiplier; 1.0 uses the OpenXR runtime's recommended per-eye dimensions.
Game resolution does not select the VR render dimensions. Remaining validation
is release work for this default, not a reason to defer it to 0.0.2.

## Default configuration regression, 2026-10-19

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
