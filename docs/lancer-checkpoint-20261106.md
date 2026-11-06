# Lancer / unified 6DoF source checkpoint — 2026-11-06

This is a source checkpoint of the accumulated work on `codex/6dof-single`, not a declaration that the complete Lancer feature or physical-headset acceptance is finished.

The installed user candidate remains **6dof-unified-20261106-71** at `J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe`. This Git commit does not install a new candidate.

## Included implementation

- Unified first-person/body/hand integration, controller input and fixed-space menu work, including the existing single-writer and input-isolation protections.
- Lancer following, aiming, authoritative muzzle firing, two-handed support, manual magazine extraction/insertion, empty-only physical charging, configured perfect-reload timing/UI, interaction guides, retained weapon state and chainsaw handling.
- The C-handle rule: actual full-forward plus held input runs continuously; the first actual release starts one three-second tail; backward movement stops it. The installed v71 has scoped simulation evidence for this rule. It is not physical comfort acceptance.
- Authored hand/contact poses, original-part observation, startup/resource protections and the existing numerical/recorded-review infrastructure.

Earlier Snub, Hammerburst and grenade source already present in this checkout is preserved in the checkpoint. It is not a claim that those weapons have just been expanded or newly accepted.

## Latest source repair and evidence boundary

Games 67624 and 3088 recorded withdrawal of a committed hand publication when only the installed-magazine `magazine_observation_pending` marker changed (`game_difference=16`). That marker describes acquisition availability; it is not by itself evidence that the main hand or an already-held C part ceased to exist.

`weapon_hand_client.h` now tolerates **only that marker** during read-only consumption when either:

- no left-hand lease or object incumbent was granted; or
- the exact same sole magazine-route part lease is already retained, with matching lifecycle and press token.

The returned observation remains the actual current one. This creates no new grant, input edge, pose timestamp or mechanical travel. Prepare/commit retains strict observation equality. Actual identity/mount changes, missing or conflicting incumbents, busy/animation ownership, expired tracking, and release rules remain effective.

Validation at this checkpoint:

- Native `GearsCapture` and Playground `GearsPlayground` compiled successfully. The normal private main-grip diagnostic option remains OFF.
- `WeaponHandClientSourceStateTest` passed, including **23 focused pending-marker cases** plus its existing source, lifetime and retained-motion cases. Both newly covered failure paths were reproduced as failing assertions before their respective repair.
- Frozen private build `equipment-controls-prepared-20261106-135632` completed two original enemy chainsaw executions, post-execution authoritative firing, and pause/B resume in game 3088, followed by normal Native/input/entry/XR cleanup. Its first, no-left-lease-only repair still recorded a retained-C withdrawal; this prompted the second source repair above.
- The final retained-C extension has been compiled and checked locally; **it has not yet received its targeted recorded game recheck**. The earlier intermittent token12 acquisition failure is preserved and is not declared causally fixed.

## Remaining work

Complete the targeted recorded recheck after this source save, then continue actual five-part surface/anatomical fitting and the preserved walking flicker, HUD and startup third-person defects. Do not infer full visual quality from matching point data or successful action receipts. Physical headset feel and final acceptance remain pending. Do not broaden to other weapons while closing the current Lancer batch.

Runtime recordings, large generated screenshots, build products and local research caches remain outside Git under the existing ignore rules. The two Lancer interaction diagrams under `docs/diagrams/` are documentation assets, not recorded test sessions.
