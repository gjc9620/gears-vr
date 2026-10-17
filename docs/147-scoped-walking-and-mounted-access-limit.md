# Scoped walking and mounted access limit

2026-10-17. Actual123 and Actual124 used the existing Native67 DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`; no native rendering change was made in these runs. Python includes the discovery and failure-evidence changes in `2bb3cc3` and `7f74a50`. This is test-access progress, not a mounted black-screen fix.

## Input evidence

Actual123 (`continuous-stereo-23252-20261017-205924-dd35ddea`) checked the live WASD bindings, then posted each key only to the owned game HWND for 0.55 seconds. All four measured displacements were zero. During a separate A-key probe, the bounded live PressedKeys/axis observations remained empty/zero; the mouse comparison admitted RightMouseButton. Cached historical input-loop assembly suggests a keyboard cleanup boundary, but no exact imported key-query identity or causal runtime explanation was established. A failed read-only import assertion is retained rather than converted into such proof.

Actual124 (`continuous-stereo-38880-20261017-211141-64dddf4a`) temporarily used the game's own keyboard-binding menu and normal profile-save path to bind MiddleMouseButton to secondary backward movement and ThumbMouseButton1 to left turn. The capture/duplicate dialogs were checked through reflected callback, sender, owner and reserved-key evidence because their rich text was outside the existing strict string decoder. This test-specific reader did not relax production menu input checks.

A 0.65-second middle-button pulse moved the pawn 144.42 units; subsequent neutral observations had zero displacement. A 0.15-second side-button pulse changed controller yaw by 17.52 degrees; its neutral yaw change was zero. Later bounded navigation also recorded releases and neutral checks. No foreground activation, global input, controller emulation, native function invocation or game-memory write was used. Physical background controller response remained in the original disabled state. Foreground HWND was unchanged across these recorded navigation actions; this is not a claim that the foreground never changed during the entire session.

## Mounted reproduction remains blocked by access

Fresh same-WorldInfo discovery identified two non-template Troika actors, both without Driver/Controller. The chosen nearby actor's origin was only a navigation hint. Direct and short detour attempts stopped at a wall, with collisions near y=-11851.8 at distinct x positions. The final player-to-actor distance was 489.63 units, versus an observed EntryRadius of 100. No Use input or possession occurred.

Target-facing GPU crops show masonry and a projecting column, not a verified passage. A bounded independent route review recommended retracing previously traversed floor before further reconnaissance; it did not establish a shortcut or justify a checkpoint change. Saved artifact2 `GetEntryLocation` declarations also compute entry from skeletal references and an offset, so Actor.Location is not an established exact entry point. Attempts to inspect the relevant current reflected fields stopped at prerequisite guards and produced no field values.

The latest admitted menu observation changed to `DeltaMainMenu`. The subsequent read-only field query rejected its gameplay guard, without sending input. Neither the main-menu transition's cause nor mounted rendering behavior is established by these observations. Mounted target admission must not be widened on this evidence.

## Lifecycle and limits

Both initial 90-second read-only monitors passed. Actual124's final lifecycle report records 2242.97 seconds and 130234 projection submissions, but includes menus/binding work and a maximum submission gap of 583377.968 ms. It is **not** a continuous-play, pacing or headset acceptance result.

Both sessions verified orderly XR shutdown, native cleanup, entry restoration, normal owned-game closure and all owned-process exits. All four protected profile files and the full original game-settings file were restored and hash-verified after each run, including removal of the temporary test bindings. Actual124 settings restored SHA256 is `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`.

Ignored receipts include `vrdof123-window-walk.json`, `vrdof123-key-state.json`, `vrdof124-mousewalk-window-walk.json`, `vrdof124-thumb-turn.json`, `vrdof124-troika-approach2.json`, the `vrdof124-obstacle-*`, `vrdof124-detour-*`, `vrdof124-wallfront-*` and `vrdof124-entry-fields-*` records, and both sessions' normal-close/profile/settings restoration receipts. No proprietary derivative was added to Git or a release package. Mounted output, PathChoice placement, the reported recoil-ring appearance, startup latency and headset pacing/extended play remain open; `headset_verified=false`, `release_ready=false`.
