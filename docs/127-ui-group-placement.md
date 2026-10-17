# Shared UI group placement preparation

This is offline preparation, not an enabled PathChoice UI fix or headset acceptance.

`ui_group_placement.h` applies one source-normalized uniform scale and translation to a complete admitted group. Each eye retains its independent panel viewport. The saved source scissor follows the same affine transform and intersects that eye's bounds. Empty or invalid results reject atomically; the helper cannot recover geometry already culled by the game.

The existing HUD layout delegates its established affine calculation to this helper. An actual identity HUD policy preserves the panel unchanged. A nonidentity HUD policy whose resulting affine happens to be identity still recomputes the saved source clip, preserving the previous behavior.

## Verification

Both standalone CPU executables passed on 2026-10-17:

- `UiGroupPlacementTest`: common movement of arbitrary group members, asymmetric eyes, independent expected pixel clips, scaling, partial/outside clipping, numeric limits and unchanged output on rejection.
- `UiHudLayoutTest`: existing HUD behavior and the centered-bounds identity-affine regression case.

The HUD test uses a 16 MiB executable stack for its existing large CPU fixtures; the CMake link option affects this test only. Private executables are under ignored `analysis/ui-group-placement-cpu/`.

## Remaining integration

The native scene route and matrix evidence are recorded in docs 122 and 124. Production use still requires an admitted root scene, one immutable group receipt shared by text/tile/material payloads, complete collection across relevant sort keys, and verification against actual GPU viewport/scissor state. Ordinary per-entry HUD layout must not apply a second transform to that group.

No root hook, PathChoice translation, new DLL, game launch, simulator launch or runtime test is included in this change. Mounted-camera sampling changes in `d803216` remain separately unverified in an actual mounted session. The latest development ZIP predates that commit and this helper.
