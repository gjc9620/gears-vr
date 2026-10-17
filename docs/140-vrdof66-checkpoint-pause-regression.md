# Native66 checkpoint recovery and pause-menu regression

2026-10-17. Actual116 used unchanged Native66 source `4fcd1af`, DLL SHA256 `d2264dd811e104633d1e9d97e4647352607fe4a095a47d61163007ff183b2500`. Game PID43000, session `continuous-stereo-43000-20261017-193848-446ca183`. This extends [the ordinary-scene normalization result](139-guarded-final-transfer-normalization.md); it does not establish menu acceptance.

## Recovery succeeded; pause presentation failed

Two checkpoint reloads committed world generations **1→2→3**, and gameplay GPU images were obtained after recovery. The two recorded images are 2534×1009 with nonblank content in both halves. Native normalization recorded 9856 planned, linked and completed transfers, zero failures, and `faulted=0`. Final native publication reached 4152; frame-link inventory changes and contamination were zero.

Recorded XR composition shows long pause intervals with **zero submitted layers**, rather than a visible pause panel. Pause/menu acceptance therefore failed. A single `eye_menu_first_frame` receipt exists for epoch2, but occurred after the second confirm-reload command. It cannot establish that the preceding paused menu was visible. A common menu panel also does not count as stereo gameplay.

## Publication evidence and unresolved cause

`vr_dof_compat` recorded 5698 setter proofs per eye but **zero pre-quad proofs per eye**. It rejected 5698 publications. The saved rejected example is a completed record with `setters=3`, `quads=0`, `failure=0`; successful transfer normalization alone does not satisfy this later publication gate.

The XR eye-menu path requires a fresh native image. Missing publication is consistent with the observed blank pause display, but the general explanation that pre-quad hook coverage is incomplete remains a **hypothesis**. It requires bounded static/current-runtime trace evidence; these counters do not identify the missing native path or prove a normalizer defect.

Movie fallback separately recorded 352 submissions, zero end-frame failures, and two entries/two exits. Those receipts concern movie fallback, not successful pause-menu rendering.

## Timing and cleanup limits

The XR lifecycle spanned approximately 214.92 seconds, with a maximum submission gap of 102379.168 ms that includes deliberate paused-menu dwell. This is **not** evidence of 214 seconds of uninterrupted presentation. The first recovery receipt's approximately 14.8 seconds is delayed sampling to an observed recovery, not first-display latency; no display-latency claim is made here.

Native cleanup, entry restoration, orderly XR shutdown and normal owned-game closure were verified. All owned processes exited and all four saved profiles were restored. The lifecycle receipt's `session_passed=true` does not override the failed pause/menu observation.

Evidence remains ignored under `analysis/runtime/`: `vrdof116-audit.json`, `vrdof116-recovery.json`, `vrdof116-recovery2.json`, `vrdof116-pixels.json`, `vrdof116-native-status.txt`, `vrdof116-normal-close.json`, `vrdof116-save-restored.json`, and the session's stdout/world/menu logs. No default or release promotion follows. UI, projection accuracy, headset, mounted gameplay and long-session continuity remain unaccepted.
