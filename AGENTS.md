# Project reverse-engineering workflow

## Current development version (2026-10-23)

- At the user's request, discard the uncommitted 0.0.2 comparison-only startup changes, switch back to `codex/0.0.3`, and create `codex/0.0.4` from its current HEAD `9ffa710`. Current work targets fixing the left-eye blur/sharp fluctuation on `codex/0.0.4`; record subsequent fixes and validation in `docs/releases/0.0.4/PROGRESS.md`. Preserve the published 0.0.3 package and the 0.0.2 comparison branch.
- The user also observed flicker in 0.0.2; this is not proof that a 0.0.3-only FPS optimization caused it. The left-eye blur/sharp fluctuation was reproduced in full-resolution native SBS source with motion blur enabled. Isolating the VR left-eye history from the ordinary player's view state removed it in matched real-game/simulator comparisons, including ordinary-frame fallbacks, aim/fire, pause and checkpoint reload. Physical headset acceptance remains pending; prior white-frame detector results do not cover clarity fluctuation.
- The current default adds passive support diagnostics on 2026-10-24: native `tools/native-capture/build-support1/GearsCaptureSupport1.dll`, SHA256 `5e78c2a736526b3bf22a87023959d1635d82d8bcec80ddd05ed24fede4d6fe7a`; XR `tools/openxr-probe/build-support1/OpenXRPoseProbe.exe`, SHA256 `df54d17c03068f46cad60f43c5b17114dcc61bf612321efc85611822cd181b48`. These are minimal observation deltas on the actual frozen left-history1 native (`a637128c…d758`) and original-color XR (`58ef6c39…539a`), with their original flags, independent left history and original color policy preserved. Provenance: ignored `analysis/runtime/support1-frozen.json` and `analysis/support-instrumentation-20261024/`; preserve the previous `left-history1-frozen.json` baseline too. Do not rebuild the whole current source with unrelated experimental flags for subsequent fixes. Normal Export logs now includes runtime-provided headset/system identity, per-chain game/XR adapter matching, loaded graphics module versions, swapchain progress, live native status and durable first-failure receipts. Bounded journals retain recent events and repeat device context with its original observation time after rotation. Unavailable metadata is explicit; registered layers are not proof of active hooks, and a first failed API return is not by itself the root cause. Root GUI/diagnostics remain 0.0.4 development; no 0.0.4 release/tag exists.

## Published 0.0.3 baseline (2026-10-23)

- At the user's request, the previous development content was released as 0.0.3 instead of 0.1.0 on `codex/0.0.3`; `codex/0.1.0` was deleted after verifying that the new branch preserves its HEAD and all working files. Retain `docs/releases/0.0.3/PROGRESS.md` and `docs/releases/0.1.0/PROGRESS.md` as historical evidence.
- The 0.0.2 startup compatibility, executable discovery and diagnostics fixes from `codex/0.0.2` are merged into this branch. Preserve the first-person, Playground and renderer content previously developed under 0.1.0, and its frozen native/XR manifests. The requested color rollback has completed; the 0.0.3 snapshot uses `build-runtime-colors1` with XR SHA256 `58ef6c397af8facc5ae50c8adba659005a13efd603e7bcfee73ba890d1e2539a`. The left-eye white-flash report remains known and deferred, not certified fixed.
- The public 0.0.1 source snapshot `4b621fc430fa5f96eba3bf70399810d3fd51e202` is the content merge baseline; the two branches have separate root histories. Preserve both histories and the pre-merge 0.1.0 checkpoint `963d965b8590555bae31d047bef6ccc27d3e6469`.
- Local commit `5787a48` preserves the current source at the 0.0.1 closeout, including ongoing development. It is not the published `v0.0.1` source tag.
- Preserve the published `v0.0.1` and `v0.0.2` tags/packages and the `codex/0.0.2` hotfix branch. A 0.1.0 development checkpoint does not constitute a new release or headset acceptance.

## Retail game baseline (2026-10-21)

- All subsequent Mod development and runtime tests use the user's purchased Steam game. The default working copy is `J:\gearsVr\Gears of War Reloaded`; the untouched Steam-managed copy is `J:\SteamLibrary\steamapps\common\Gears of War Reloaded`. Do not launch the former P2P directory for new tests. Historical manifests and evidence retain their recorded paths and identity.
- Both retail copies were completely hashed before testing: all 3,609 files match. The core EXE also matches the historical supported SHA256. This does not establish runtime/GPU/headset acceptance.
- The user stopped additional compatibility runtime tests after the hash comparison. Do not restart this investigation's game tests merely to recheck compatibility. This does not cancel the standing authorization for future requested Mod development tests on the retail game.
- Current save progress was copied to the signed-in retail account's four profile filenames, with source and prior retail backups under ignored `analysis/retail-compatibility-20261021/`. Keep account identifiers and save contents out of release packages and public documentation. Do not overwrite the migrated progress with historical autoplay fixtures outside a protected test.

## Delivery scope confirmed by the user on 2026-10-20

- DLSS is optional for the first version. A separate CMD launcher for DLSS testing/use is acceptable; it does not have to be enabled in the normal launcher.
- Integrate other bug fixes into the normal launcher after the necessary verification, without asking again or requiring separate experimental entry points.
- This supersedes older notes calling DLSS a mandatory V1 / 0.0.1 release item.

## Reverse engineering

- Prioritize the current VR Mod deliverable. Knowledge-base expansion is bounded and must not displace it.
- Use the existing IDA Pro / Hex-Rays / IDAPython toolchain. Do not install or migrate analysis tools automatically.
- Before new native reverse engineering, search `analysis/re-kb/knowledge.sqlite` with `tools/re-kb/kb.py`, then inspect relevant existing evidence and `docs/58-reverse-engineering-knowledge-base.md`.
- Distinguish disk EXE, historical runtime snapshots, and UnrealScript declarations. Never equate matching RVA with matching bytes or version. Preserve source hashes, module base, evidence and confidence.
- Only the coordinating agent operates an IDA database. Work on private copies, preserve originals, close owned leases. Other agents use saved evidence.
- Export mechanically in IDAPython; never send full-game pseudocode to AI. Default to no subagents for mechanical work; when needed use one narrowly scoped light subagent and escalate only concrete hard questions. Maximum three simultaneous subagents.
- User prefers low Token consumption. Avoid full-history forks for future lightweight subtasks; provide only necessary paths and a bounded question. Do not invent Token usage when counters are unavailable.
- Static conclusions do not establish runtime/GPU behavior. Keep actual-game, RenderDoc and headset acceptance separate.
- FPS comparisons must exclude all concurrent game instances and active autoplay workers. The shared launcher owner mutex does not exclude isolated instances; wait for their work to finish and discard overlapping samples rather than interrupting them.
- Keep snapshots, proprietary derivatives, pseudocode and SQLite under ignored `analysis/`; never include them in release packages.

# Persistent runtime testing preference

- User authorizes unattended background game and simulator tests at any time, whether present or away. Do not request this permission again. Do not steal focus; keep physical controller background input disabled. Simulator preview may remain visible without activation.
- For autonomous background tests use `GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1`; this keeps the tiny-output monitor read-only and prevents automatic window resize/recovery while the user is using the computer. Background testing remains authorized. Do not silently substitute foreground launch or window adjustment when output is black.
- Automatic test windows must also stay behind the user's existing windows; merely preserving foreground focus is insufficient. Use the explicit test-only behind-windows game build and simulator runtime once their checks pass, with `GEARSVR_TEST_WINDOWS_BEHIND=1` in the test child environment. Do not fall back to old visible-on-top startup. Do not minimize, move, resize, or periodically reorder the user's windows. Manual launcher play keeps normal foreground behavior.
- Set `GEARSVR_TEST_MUTE_GAME=1` only for autonomous tests; preserve the user's normal launcher audio and restore the game's original mute state afterward.

# Persistent fix integration preference

- Integrate completed bug fixes into the latest default launcher build after the necessary automated checks. Preserve fixes already integrated by other work; rebase on the latest frozen default rather than replacing it with an older candidate.
- The user will perform headset and gameplay acceptance for accumulated fixes together later. Do not require a separate manual test entry or per-fix user verification before default integration. Keep pending runtime/headset validation explicit; this does not make unfinished experiments completed fixes.

# Reticle and aiming behavior

- The reticle should represent the weapon's authoritative aim line and its scene intersection. HMD motion changes the view/projection; looking up alone must not retarget the weapon's world-space aim. Screen movement caused by that projection is expected.
- Distinguish the aim center from per-shot random spread and final impact. The original HUD query cache, animated muzzle direction and render-camera forward are not interchangeable proofs of the actual firing ray. Verify their relation before claiming weapon/reticle alignment.
