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

## 2026-10-18: Shared fit and Native105 buffered HUD baseline

The shared helper now computes one normalized fit from a complete logical group to an explicit target rectangle. It preserves the requested uniform size when it fits and reduces uniformly otherwise; it has no page-specific offset or default target. HUD bounds use the shared type without changing existing player layout. This is preparation, not an enabled general UI correction.

The root coordinator built private UiGroup105, passed22 startup self-checks, and ran a background baseline. The game had an8×8 background output and the VR scene submitted/copies remained0, so there is no visual acceptance. The logical observer's separate CPU counter published175 samples and retained the latest8 authenticated PostRender groups; it must not be confused with VR publication. All8 retained groups have before/return/root/delta/bounds checks passing,20–22 Canvas calls,12–13 simple ranges and3–4 material ranges, with no call overflow, unclassified Canvas calls or synchronous simple/material Execute. There are no PathChoice root samples. The coordinator stopped normally and verified Hook restoration, game exit and settings/profile restoration.

Runtime frame/name evidence includes WarHUD.PostRender, DrawHUD, DrawHealth, UpdateWeaponIndicator, DrawWeaponInfo, DrawWeaponIcon, DrawEmptyAmmo, DrawAmmo and teammate/locator routines. At the actually observed DrawWeaponInfo UFunction node0x37e51de0, the decoder finds copied geometry candidates in7 of8 retained groups; row2 rejects an incomplete selected-call member proof. Filtering by a UFunction ancestor alone can combine repeated invocations, so these are diagnostic unions, not proven single logical prompts. DrawActionInfo, the Hammer pickup event and Choose Path were not covered. The result does not establish the reported pickup route, GPU clipping, input hitboxes or headset layout.

Evidence remains ignored: `analysis/runtime/continuous-stereo-23004-20261018-180334-03ce8195.native-status.txt`, `uigroup105-logical-group-bounded-report.json`, `uigroup105-weaponinfo-ancestor-report.json` and `uigroup105-logical-group-bounded-proof.json`. The proof saves the source status SHA256. The10 relevant UI files matched the frozen `analysis/native-source-uigroup105/` copy before the next extension; per-file SHA256 pairs are saved in `uigroup105-ui-source-comparison.json`. Subsequent unique-token source differences are explicitly saved in `uigroup105-ui-source-after-unique-token.json`, not attributed to the105 run.

The next observation-only extension uses the already existing, byte-checked1d6f30 script-call boundary; it installs no additional Hook and changes no configured semantic scope. Under a sampled authenticated HUD parent, live Function/WarHUD identity, script range, original VM entry and caller ancestry must pass before a unique child token is assigned. Canvas calls retain a token only when their actual body frame matches that callee/self/caller and code range. Normal return revalidates the function identity and script/entry. Repeated use of the same UFunction and stack address receives distinct tokens. Original arguments, return bits, exceptions and exactly-one original call are preserved. The decoder's `--invocation` checks the token, normal completion, body-call count, ancestry and typed member ranges before emitting a copied group candidate; it still never authorizes replay. This extension has offline checks, not a runtime claim from105. Default player manifest and private PathChoice positioning policy remain unchanged.
