# Guarded final-transfer normalization

2026-10-17. Native66 source revision `4fcd1af`; frozen DLL SHA256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`. This candidate follows the bounded pending-state evidence in [137](137-pending-final-transfer-target.md) and the [Native65 runtime comparison](138-pending-final-target-runtime.md). `GEARS_FINAL_TRANSFER_NORMALIZE` remains **OFF by default**. No launcher default or release package was promoted.

## Guarded behavior

For the attested final quad, the candidate validates a fresh complete source pair, source/list generation, pending destination and descriptor provenance, world/inventory, and consumed OM/viewport/scissor binds. It holds the existing recursive frame mutex across the original call and completion. Only sampling UV changes: the observed native mapping `-76.154541,0,2712.30908,1080` becomes `0,0,2560,1080`; destination geometry and declared dimensions remain unchanged. No game or window resolution is forced.

Identity plans use the existing legacy path. Failed preflight leaves original arguments untouched. A mismatch after mutation, extra/nested draw, bypass, exception, or reentrant Execute/Present disables further normalization and publication for the session. The legacy transfer predicate remains unchanged.

## Actual115 result

Game PID32848, session `continuous-stereo-32848-20261017-193347-21d52549`, rendered a 2560×1080 source into a 2534×1009 backbuffer. The 30-second monitor recorded 61 samples and publication increasing from 0 to 1305. Final native summaries recorded 2976 planned, linked and completed normalized transfers, zero normalization failures, and 2968 published frames. Seven entry preflight rejections did not mutate arguments. Frame-link inventory changes and contamination were zero.

Two original-resolution GPU snapshots, tickets 49 and 457, were saved at 2534×1009 in format 24. Their pixel receipt records different eye content (mean absolute byte differences about 18.30 and 18.80). The coordinating review saw two complete SBS gameplay images with no obvious black half. This verifies GPU image output for this ordinary scene and this nonstandard mapping; eye differences and visual review do not establish correct projection.

Native cleanup and entry restoration passed. XR reported orderly shutdown and 2968 scene copies over approximately 73.34 seconds; the owned game closed normally. All owned processes exited and the four saved profiles were restored. Ignored receipts: `analysis/runtime/vrdof115-result.json`, `vrdof115-pixels.json`, `vrdof115-normal-close.json`, and `vrdof115-save-restored.json`.

## Validation limits

This run does **not** accept UI coverage, projection accuracy, physical tracking, headset presentation, mounted gameplay, or long-session continuity. It does not decode the pixel shader or establish correctness for arbitrary resizing or scenes. The result receipts explicitly retain `headset_verified=false` and `release_ready=false`.

`FinalTransferNormalizeTest` is now a CMake CPU target with `/UNDEBUG` so Release builds retain assertions. It exercises guarded plans, identity passthrough, invalid geometry/modes, changed post-call proof, and completion ambiguity. CPU tests complement the runtime receipts; they do not replace GPU or headset acceptance.
