# Native67 alternate DOF quad and pause recovery

2026-10-17. Actual117 used Native67 source `f55f71313df32c96e588f658b7d4caf76eea2777`, DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. Game PID10656, session `continuous-stereo-10656-20261017-195136-53668504`. The tested pause pattern now works in the simulator; this does not accept all menus or headset presentation.

## Evidence behind the caller correction

In [Actual116](140-vrdof66-checkpoint-pause-regression.md), actual `gpu_output_quad` counts at return RVA `d808d8` were 5698 per eye, matching the missing pre-quad proofs. Saved historical function `140d7fd14` contains a common setter followed by mutually exclusive quad calls returning at `d808d8` and `d80a7b`. This is not a resolution-specific branch, nor evidence that it is a mounted-gameplay branch.

The ignored `alternate-dof-branch-attestation.json` binds that saved analysis to historical artifact1 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, module base `0x140000000`, and pseudocode SHA256 `a5cf75e88d0d204574958c55bb6158364c664d7594293caab7629c0c7b6eddcb`. No IDA database was opened. Those historical identities alone do not attest current code.

Native67 explicitly accepts both exact quad callers using the unchanged fresh pre-quad certificate and one shared per-eye reserve. There is no setter-only or NoSite bypass. Caller attestation was strengthened from 96 bytes to the full 1819-byte range at `d80360`, SHA256 `e7bdb0107d1243e27ccdfc2c24e2a477b209ca1e31c3a78ec9e3489ee5423997`; the 200-byte setter check remains. Actual117 verified that live full range with `match=1`. Tests preserve unknown-site, duplicate/cross-site duplicate, one-eye and missing-quad rejection.

## Actual117 observations

The `d808d8` site produced 3943 calls and successful proofs per eye; `d80a7b` was unused in this run. Publication rejection was zero. The saved allowed receipt preserves world1/ticket4616, setter and quad masks3, and successful caller pair `d808d8,d808d8`. Normalization completed 4614 transfers with zero faults; native publication reached 4609.

Both pause entries produced successful fresh XR `eye_menu_first_frame` receipts: epoch1/ticket287 and epoch2/ticket1333, each with sample time later than its menu-entry minimum. Resume1 recovered gameplay projection. GPU snapshot2 (`c7ae7d21320144bf81df618cef960545d8bafd78c9e83c60094902be0582ed08`) contains complete duplicated pause UI in both halves according to the coordinating visual review. Snapshot3 (`cda4af5b1754ed03f70d2598cabf6cb07c305867bce985bccee5588ea8033a0f`) records resumed gameplay. Both are 2534×1009.

The pause image's approximately 93–94% black fraction reflects its black menu background; aggregate darkness is not evidence that an entire eye is blank. The common eye-menu panel is explicitly not stereo gameplay. Likewise, XR's approximately 62.8-second maximum projection-submission gap includes paused eye-menu composition: projection-only submitted counters omit that panel. It must not be reported as 62 seconds of black display.

## Cleanup and remaining acceptance

Native cleanup, entry restoration, orderly XR shutdown, normal game closure, all owned-process exits and restoration of four saved profiles were verified. Receipts remain ignored under `analysis/runtime/vrdof117-{result,pixels,scenario,normal-close,save-restored}.json` and `vrdof117-native-status.txt`.

This fixes the observed simulator pause pattern. General menu coverage, projection accuracy, physical tracking/headset presentation, mounted gameplay and long-duration continuity remain unaccepted. Defaults and release packages have not been promoted; `headset_verified=false` and `release_ready=false` remain in the result.
