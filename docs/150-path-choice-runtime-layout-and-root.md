# PathChoice: actual layout and deferred scene geometry

2026-10-17. Actual129 used the private Native69 UI-root observation build, SHA256 `a7ea494b0414426a281324e541d7dec2a7c73a7b0c02f45b38172c292836a070`, and the already fixed quiet2 simulator. The player default remains Native70. Original profiles/settings were protected before applying the previously recorded chapter fixture, then restored and hash-verified after normal game closure.

## Actual scene evidence

Five HWND-scoped forward pulses reproduced four movements followed by collision; every release had zero measured neutral drift. The subsequent GPU snapshot displayed CHOOSE PATH near the top. The earlier immediate movement probe had ended before observing this delayed scene, so collision was not evidence that the trigger never fired. This run used no foreground activation, global input, controller emulation, native function invocation or game-memory write. Physical controller background response stayed disabled.

After the scene appeared, the corrected optional reader obtained a consistent 13-node `DeltaChoosePath` / `UIScene_PathChoice` layout, a fresh owner graph classifying it as `local_player`, and current client/active-scene membership. The safe-region panel spans most of the screen; the actual title, background panels, arrows and choices occupy approximately y=72..260 in the scene's original 1080-height CPU coordinates. This agrees with the high placement visible in the GPU snapshot, without equating CPU bounds with GPU scissor ownership.

All eight bounded native root observations have the exact same scene address as that CPU scene. They show valid before/after inventories, four simple geometry ranges, 284 added vertices, zero material records, restored transform stacks, and zero simple/material Execute calls **inside** the root invocation. This identifies a concrete deferred geometry producer. It does not yet prove the later flush's exact indices, mixed-scene membership or replay lifetime, and does not authorize identifying arbitrary GPU draws by their coordinates.

## Reader corrections established by runtime

The right arrow has directed bounds approximately `[1331.812,148.392,1281.811,248.392]`. The old optional sampler rejected all layout evidence when left exceeded right. It now preserves those original edges/raw bytes, separately returns normalized `ordered_values`, and marks reversed axes. Finite-value, reflection layout, registration, ancestry and consistent-reread checks remain. Synthetic tests cover both axes, zero-size bounds, the actual recorded values and nonfinite values on every edge.

Owner sampling also rejected an unrelated, registered WarPC field whose FName number is 481. It now compares the complete `(name_index, number)` for required property membership while retaining every traversed field header in the final reread. Required properties still require the exact unnumbered identity and declaring owner; numbered siblings cannot shadow them. Corrupt ownership, registry membership and mid-read changes still reject.

Finally, the real one-player GamePlayers array has count=1 and capacity=17. The reader had applied its 16-member budget to allocator capacity. It now separates bounded occupied members from allocator slack, caps capacity at 4096, preserves address-span checks, and reads only the bounded member count. The complete graph still requires exactly one matching local player. Tests retain count, negative/overflow capacity and link rejection.

## Validation and next step

The menu-state suite passes 66 tests and scene-owner suite passes 13. Actual fixed layout/owner sampling succeeded with `consistent_reread=true`. The session stopped normally with native cleanup, 45 independently restored entries, XR exit0 and all owned processes gone. All four original profiles and the entire settings file were restored with hashes verified.

Next, associate the authenticated root's deferred ranges with existing receipt/generation/exact-index replay rules before applying a group transform. No UI placement fix or physical headset acceptance is claimed yet. Evidence is ignored under `analysis/runtime/vrdof129-*` and `continuous-stereo-36772-20261017-231358-90f5aede.*`; no proprietary derivative is shipped.
