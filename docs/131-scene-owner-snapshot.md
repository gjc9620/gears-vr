# Fresh PathChoice owner graph sampling

2026-10-17. Offline implementation and synthetic verification only. No game, simulator, process memory, GUI, input or XR Runtime was accessed during this work.

`tools/game-io/scene_owner_snapshot.py` adds `capture_scene_owner(resolver, client, scene, expected_controller, expected_local_player)`. The caller owns an exclusively used, prepared/build-verified Resolver. The helper checks the supported profile, base/PID and fresh relevant UObject/name/registry fields. It does not hash the executable itself; the output names the profile's game digest `profile_game_sha256`.

The sampled graph requires:

- Exact GameUISceneClient class, its UIInteraction outer and reciprocal SceneClient.
- Interaction's viewport outer and the viewport UIController backlink.
- Expected LocalPlayer's ViewportClient and Player.Actor, plus expected PlayerController.Player.
- A common Engine outer with exactly one GamePlayers member equal to that local player.
- Unique ActiveScenes membership, UIScene_PathChoice/UIScene ancestry and the scene's SceneClient backlink.
- PlayerOwner equal to that player, or null under the complete single-player graph. Null is labelled `unowned_single_local_player`; this does not infer the engine's render scope or authorize placement.

Cached reflection supplies candidate addresses only. Selected properties are independently checked for fresh identity, exact name/kind/defining owner, dimensions/element size/offset, actual field-chain reachability, earlier property shadows and array-inner ownership/type. Unrelated fields use lightweight index/slot/outer/FName/next headers rather than decoding every field's name/class. Hint reads share the bounded read budget and the original/nested reader is restored even on failure.

All accepted source bytes are reread and registry headers close the acquisition. Result evidence contains bounded identities/classes/property/field-header closure, graph links, arrays, counts and a source-byte digest. It explicitly reports no atomic snapshot, allocation identity, render admission or GPU proof. It is not interchangeable with the root callback's FCanvas, target, frame ticket or draw membership.

## Realistic bounds and tests

The existing package inventories contain 1,359 immediate WarPC entries and 453 PlayerController entries. These are exported declarations, not a current runtime field-chain measurement. They exposed the original 512-per-class limit as unsuitable for this schema. Bounds are now 2,048 fields/class, 4,096 total fields, 32,768 total read calls and 2 MiB read bytes. The synthetic 1,359 + 453 scale fixture succeeds in 19,538 reads while preserving late-header mutation rejection; actual process timing remains unmeasured.

Inventories: Engine objects.tsv SHA256 `cf331d8b9142183f6420d5d68ebfe55082f5d33340209645feebd06d50b7d719`, WarfareGame objects.tsv SHA256 `c414f4b797705af9991d16a9035f59e27c61174efe4c2fd1beb92a63dd5a8c0a`. Counts/script remain under ignored `analysis/runtime/scene-owner-field-counts.json` and `count-owner-field-inventory.py`. They are script-package evidence, separate from native historical code bytes.

195 tests passed across owner sampling, menu state/control/navigation/display monitoring and staging/packaging. Coverage includes stale hints, wrong/unreachable/shadowed properties, metadata and graph mutations, budget/read failures, exact-scale chains, default-path preservation and restoring nested readers.

## Optional menu integration

`MenuReader.sample(include_owner=True, expected_controller=..., expected_local_player=...)` shares the existing outer sample verification. CLI `--owner` requires paired `--owner-controller` and `--owner-player` current addresses; incomplete arguments reject before opening a process. It can accompany `--layout` to gather both CPU graph and layout evidence when the branch UI is visible. Default menu sampling performs no owner reads. The portable allowlist includes the helper; existing ZIPs were not replaced or repackaged.

Next actual work requires a coordinated visible PathChoice sample, compared with the native root observer's scene identity and collection/flush behavior. Neither this CPU graph snapshot nor passing synthetic tests enables UI movement. Fresh native entry/completion admission and shared simple/material receipts remain to be integrated after that evidence.
