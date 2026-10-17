# VRDof63 final-transfer geometry investigation

## Candidate identity and scope

Source revision `e095c5e` adds one bounded crosshair activation failure-stage report. It changes neither admission checks nor retries. Current-source MSVC assignment, weapon-binding and INI-loader CPU tests passed. The normal feature profile is retained; scene-root/call-boundary diagnostics stay off. DLL `tools/native-capture/build-vrdof63/GearsCaptureVrdof63.dll` SHA256 `70035760f30b406c24cba0d41b456e961238b52333679e691611b7815db6b337`. Frozen source/options/vendor receipt: ignored `analysis/runtime/vrdof63-build-source.json`. Startup uses the full 22-test same-invocation suite. Root default and existing release ZIP remain unchanged.

## Actual PID41364: physical window correction did not change DXGI output

Session `continuous-stereo-41364-20261017-185342-b585024b`. Native preparation and activation succeeded. An identity-checked, per-monitor-v2-aware helper measured client 2534x1009, changed only owned window geometry to client 2560x1080 without activation, observed for 25 seconds, then restored the original outer rectangle and observed another 10 seconds. Foreground HWND stayed `789028` at both actions. No input helper was enabled.

All phase output samples remained 2534x1009 and publication remained zero. Two actual ResizeBuffers calls occurred near restoration; both requested and returned 2534x1009, not the temporary client size. Thus this experiment did not establish a successful matched-size DXGI control. A Win32 client rectangle is not proof of render-target size.

Native final report: 2048 transfer attempts, all policy-rejected, no paired publication. The late quad reports destination 2534x1009, source 2560x1080 and source rectangle approximately `(-76.154541, 0, 2712.30908, 1080)`. That apparent sampling extent exceeds source width. Exact padding/cropping/sampler behavior has not been proven by pixels; the earlier shorthand “crop” must not be treated as a decoded shader conclusion.

Native cleanup, entry restoration, XR destruction, normal owned-game exit and four-profile restore were verified. Evidence: ignored `vrdof111-{audit,normal-close,save-restored}.json`, `vrdof111-geometry-experiment.py` and the session files. The geometry test completed with no helper error, but display acceptance failed.

## Actual PID41244: saved resolution was not an effective control

Session `continuous-stereo-41244-20261017-190009-645e35fa`. Before launch, both user-settings resolution keys were backed up and temporarily set to 1920x1080. Native activation succeeded; 30 seconds of guarded status sampling still showed actual DXGI 2534x1009 and zero publication. Final native report records 1892 transfer attempts, all rejected; source and late quad geometry stayed the same as above. No GPU snapshot was requested because publication never began. This is not a passed 1080p rendering test.

After native/entry/XR cleanup and normal game close, all four saves and the two original resolution keys were restored, preserving other current settings. A transient process-exit check initially refused the settings write; the subsequent authoritative process check found no game, and restoration then succeeded. Evidence: ignored `vrdof112-{audit,normal-close,save-restored,settings-backup,settings-restored}.json` and raw before/after settings backups. These were temporary diagnostics, not a return of the removed Low/High resolution policy.

## Required next proof

An independent source review confirmed that accepting arbitrary native padding/cropping is unsafe: `frame_link::draw_transfer` synthesizes full-height destination eye halves and the frozen ticket retains the original projection. A permissive size predicate would label altered sampling as full eyes. Capturing a transient intermediate also requires state, resource/allocation lifetime, queue ordering and complete UI coverage proofs.

The smaller candidate route intentionally normalizes only an authenticated final transfer to full-source SBS sampling. Existing native TLS access can obtain the current command list before `original_quad`, but lazy target/list/SRV changes inside that call remain unproven. Next collect a bounded observation-only entry-to-actual-draw bracket (same list/generation, source, target, rectangles, complete current-ticket pair and one expected draw). Do not modify sampling or enable broader publication until that evidence is available. GPU pixel and headset checks remain separate.

Still open for V1: general output sizing, intermittent startup binding rejection, mounted black output, fork/UI placement, recoil ring alignment, continuity/performance and updated headset acceptance.
