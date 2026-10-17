# Scoped background firing and camera-class discovery batching

2026-10-17. Actual121 and Actual122 used Native67 DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`, frozen native source `f55f713`, with the Python discovery change below. These tests establish a scoped background firing method and new paired reticle evidence; they do not accept the reported headset reticle defect or mounted gameplay.

## Bounded startup change

`Resolver.find_camera_class()` now uses the existing `discovery_identity_reader()` for its single registry scan. The reader's bounded page hints live only for that scan, fall back to exact reads, and do not supply certificate validation. Camera-class predicates, registry/index rejection, subsequent instance/link/layout/POV checks and fresh acquisition on retry remain intact. No wait or retry cap changed.

Synthetic integration coverage compares the former exact-read scan and the batched scan for successful discovery, missing class and index mismatch. It verifies identical discovered identities/rejections and fewer underlying reads. The bundled Python passed 18 discovery, name and initial-preparation tests.

Actual120's certificate acquisition including retry was 3618.36 ms. Actual121's was 3163.26 ms, with an observer-preparation registry change causing reacquisition. Actual122 also succeeded but native preparation took 5025.81 ms; its first projection followed camera readiness by 5116.18 ms. Different retry/registration timing prevents a causal performance claim from these runs. This optimization reduces synthetic discovery reads; startup latency remains open.

## Scoped input established

Actual121: `continuous-stereo-42928-20261017-204506-fc307b39`. Right/left mouse down/up messages were posted only to the fresh identity-checked game HWND after a verified first stereo submission and a consistent gameplay menu state. There was no focus action, global controller input or game-memory write; physical background controller response remained disabled. Foreground HWND remained 789028 before and after. The current live weapon was `Weap_AssaultRifle`.

The strict read-only ammunition observer completed 160 accepted samples; `AmmoUsedCount` advanced from 0 through 11. This establishes actual game response to this scoped firing sequence. The earlier W-only negative result in [docs143](143-scoped-background-input-test.md) remains valid for that bounded test; it did not prove that every scoped gameplay input is unusable. Walking, turning and mounting remain to be demonstrated independently.

Actual121's four paired outer/inner draw groups, tickets1009–1012, had identical per-eye viewport/scissor maps. They preceded left-button down; firing GPU image ticket1030 is a different ticket. Those pairs alone are not firing-frame pixel proof.

## Same-ticket firing evidence

Actual122: `continuous-stereo-19184-20261017-205029-ed02aaef`. The first diagnostic helper exited before any input because its startup index did not yet contain a session path. The helper was corrected to wait for the path, and resumed against the same running session; the game was not restarted for that observation failure.

The subsequent scoped sequence posted aim/fire together and requested a bounded set of early GPU snapshots. Every posted down had its corresponding up in the dispatch ledger, with no release error, and foreground HWND again remained 789028. The ammunition observer accepted values0,1,2 before rejecting registry growth and ending incomplete. Thus this is partial actual firing evidence, **not** a passing eight-second observer run. The original failure is retained in `vrdof122b-ammo.json`; it was not hidden by rebinding.

Four paired native groups were recorded at tickets261–264. Each group contained an outer 16-vertex/four-receipt payload and a separate inner four-vertex/one-receipt payload sharing root, ticket, command list/generation and target. Actual index callback, command completion and geometry matching were recorded. Both parts' viewport/scissor maps were identical for each eye. All72 considered reticle draws used the finite plan: no center, geometry or projection rejection and no direction fallback.

Three groups match saved GPU images by exact ticket:

| Ticket | GPU snapshot index | SHA256 |
| --- | --- | --- |
| 261 | 2 | `747cb519bc375f3d7aa990ce684e9c4d41f8f263db74bb40453c2a47076617c6` |
| 262 | 3 | `e70b07ce1d5da6b25b93de9c2a755a39814362e1c85fccd8fa015c3afb3f10e5` |
| 264 | 4 | `825822b96f8c3f65ea8de3923214a95946314e21a10037b3071f12140446b4e6` |

The 2534×1009 final GPU images contain both scene halves. Coordinating inspection of bounded crops shows the sampled aim/fire content, but the near-white pixel heuristic also selects background lights and is not semantic reticle segmentation. It does not prove visible inner/outer concentricity, bullet impact alignment or headset fusion. The matched samples rule out differing inner/outer viewport/scissor maps for these groups, not all weapons or recoil states. The known cross-payload quadrant coverage gap remains a separate source-level possibility that these complete four-quadrant payloads did not exercise. No guessed reticle offset or widened admission follows.

## Lifecycle and remaining work

Both 60-second read-only monitors passed. Native/XR stop and cleanup, entry restoration, normal owned-game closure, all owned-process exits, four profile restorations per run and full original settings restoration were verified. Native source/defaults/ZIPs were unchanged; the Python source change is local and not yet in the previous portable ZIP.

Ignored summaries: `analysis/runtime/vrdof121-result.json`, `vrdof122-result.json`, both `*-reticle-pairs.json` files, `vrdof122-matched-reticle-pixels.json` and the underlying native/GPU/menu/ammunition/session evidence. Mounted-machine-gun output, PathChoice placement, the reported reticle appearance, startup latency, headset pacing and extended continuity remain open. `headset_verified=false`, `release_ready=false`.
