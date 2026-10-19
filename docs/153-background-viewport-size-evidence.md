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
