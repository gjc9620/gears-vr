# Opt-in native UI root observation

2026-10-17. This integrates the mixed batch inventory with the saved native root adapter. It does not enable PathChoice placement, semantic publication or rendering changes.

`GEARS_UI_SCENE_ROOT_OBSERVER` defaults OFF. An experimental build can reserve the adapter at RVA `296178` only when its exact 13 bytes match. Runtime selection requires the exact client call return `2c3dce`, caller instruction bytes, client table/slot and scene table/slot, with graph rereads. These are structural candidates only: registry identities, ActiveScenes membership and local-player relationships remain unproven. The patched adapter entry is deliberately not compared to original bytes during callbacks.

Source evidence was queried through the existing KB and reviewed in the saved `1402c3cf0.chunks.json` and `root-adapter-window.json`. Caller `2c3dc8` is `ff90b8020000` (six bytes), returning to `2c3dce`. The historical code snapshot SHA256 is `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, module base `140000000`; source/provenance boundaries remain as documented in docs 122 and 124. No new IDA database was opened.

For at most eight matching invocations per session, the hook captures a bounded before inventory and after delta, plus the top 80-byte matrix descriptor and stack header. It records simple/material Execute callbacks occurring synchronously on that same thread. Nesting restores the previous TLS observation pointer. The native original runs once and its returned RAX bits are forwarded; return semantics are not inferred.

Counts are printed only after the existing capture lifecycle drains active readers. They do not prove payload ownership, whole-scene completeness, absence of an internal/asynchronous flush, GPU execution or pixel visibility. A restored matrix report means identical saved stack header and top descriptor; a changed allocation header can make it false without establishing a rendering error. Unknown types, larger inventories or changed prefixes reject collection; they must not authorize partial placement.

## Offline verification

- `UiSceneRootCandidateTest`: saved instruction/table drift, wrong caller, read failures, overflow, graph reread races and matrix-stack/top changes; output remains unchanged on rejection.
- `UiSceneBatchTest`, `UiGroupPlacementTest`, `UiHudLayoutTest`: all passed in the same frozen experimental build.
- Full DLL compiled with the new observer ON and the prior VRDof61 option set. The ABI/self-test loader returned 0 against this DLL.
- Experimental DLL: `analysis/native-build-ui-scene-root1/GearsCaptureUiSceneRoot1.dll`, SHA256 `8acf295a4cc7136bf4e1109edaf2f5969cbfa434abc34f19d44bb82614775986`.

Frozen source overlay hashes and build logs are under ignored `analysis/runtime/ui-scene-root1-*`; existing local MinHook sources were copied with recorded hashes, without installing tools. The first configure attempt lacked ignored vendor sources and was corrected. An initial self-test invocation lacked its DLL argument and returned usage code 2; the corrected invocation passed. No game, simulator, GUI or XR Runtime action was performed, and no local default manifest or existing test package was replaced.

## Remaining before UI movement

Use a coordinated actual run to establish that this route covers the reported scene, obtain its layout/coordinate contract and inspect collection/flush behavior. Then add fresh scene/client/local-player admission and carry one immutable group identity through both simple indexed and material consumers before applying the shared placement recipe once. This observer supplies the missing runtime sampling point; static table matches alone cannot authorize that transformation.
