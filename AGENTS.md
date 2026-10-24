# Project reverse-engineering workflow

## Current release version (2026-10-23)

- At the user's request, release the current development content as 0.0.3 instead of 0.1.0. The active branch is `codex/0.0.3`; `codex/0.1.0` has been deleted after verifying that the new branch preserves its HEAD and all working files. Record subsequent release work in `docs/releases/0.0.3/PROGRESS.md`; retain `docs/releases/0.1.0/PROGRESS.md` as historical evidence.
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
- At the user's request (2026-10-24), use `render_scale=0.4` per dimension for all future autonomous background tests, and pass `--render-scale 0.4` into XR sessions. Use `tools/multi-instance/start.py`'s `prepare(..., background_test=True)` for new private test instances (`--playtest` applies this automatically). It applies `tools/test-support/background_render_policy.py` to the private config and startup INIs, retaining their copied original bytes. Preserve normal launcher settings and sealed historical inputs. Probes that do not start XR also require the reduced private startup dimensions. Record effective dimensions when available; configuration and prelaunch byte checks alone do not prove the runtime size took effect. Do not resize existing windows to achieve this.

# Persistent fix integration preference

- Integrate completed bug fixes into the latest default launcher build after the necessary automated checks. Preserve fixes already integrated by other work; rebase on the latest frozen default rather than replacing it with an older candidate.
- The user will perform headset and gameplay acceptance for accumulated fixes together later. Do not require a separate manual test entry or per-fix user verification before default integration. Keep pending runtime/headset validation explicit; this does not make unfinished experiments completed fixes.

# Reticle and aiming behavior

- The reticle should represent the weapon's authoritative aim line and its scene intersection. HMD motion changes the view/projection; looking up alone must not retarget the weapon's world-space aim. Screen movement caused by that projection is expected.
- Distinguish the aim center from per-shot random spread and final impact. The original HUD query cache, animated muzzle direction and render-camera forward are not interchangeable proofs of the actual firing ray. Verify their relation before claiming weapon/reticle alignment.
