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

## 2026-10-18: Pickup applicability boundary and one-invocation fit candidate

Bounded KB lookup and seven existing script exports establish a static pickup data-flow candidate. `WarPickupFactory.GetPickupAction` explicitly tests `WEAP_HammerOfDawn` and fills action icons and item-name tooltip; `WarDroppedPickup.GetPickupAction` also has an explicit Hammer branch. `WarPC.UpdateHUDActions` passes successful pickup actions to `MyWarHud.SetActionInfo(13, Action)`. That setter writes `ActiveAction`; `DrawActions` can draw both `ActiveAction` and fading `PreviousAction` through separate `DrawActionInfo` calls in one frame. The latter emits background text and Canvas tiles. Thus neither a function-name filter nor a union of every `DrawActionInfo` ancestor identifies one prompt. These exports contain unresolved native tokens/types; this is static data flow, not proof of the user's actual Hammer instance or of every branch condition.

Source identity is KB artifact2, `WarfareGame.u`, SHA256 `91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13`, module base0 for the package. `DrawActionInfo` export SHA256 is `1a1fb2e41b64012bd834815609812e2a1a0e7355edc527998af00a939a8f1085`; factory pickup export SHA256 is `5031b1a09fec3a0568f84a7925a59e7215b0ea1984b26414886c5657e7e4c36e`. Full per-export object identities, hashes, unresolved markers and bounded line excerpts are saved in ignored `analysis/ui-group-placement-cpu/layout-applicability-20261018.json`, along with the unchanged105 negative coverage proof. No new native analysis, IDA or game execution occurred.

The shared implementation boundary is complete normalized content bounds plus `gears_ui_group::fit`, not a common proven scene parent. PathChoice has its separately authenticated13-node tree and10 direct SafePanel content children, native layout invalidation and restoration. The pickup candidate uses Canvas text/tile groups; no SafePanel route has been established for it. A UIScene root has no UIObject `RenderBounds` (docs148), so a scene rectangle or a whole PostRender rectangle must not substitute for content bounds.

`ui_scene_logical_layout.h::fit_invocation` now produces a CPU candidate from the existing authenticated HUD observation's unique child ID. It checks normal root/child return and identity proofs, body ancestry/counts, complete typed member mapping across sort keys, interval alignment and absence of consumer flush/overflow. Exact duplicate references are deduplicated; partial overlap or shared geometry from another invocation rejects. It copies the selected complete text-atlas/material geometry, unions its original-matrix bounds and computes one shared fit recipe. It never moves individual quads. Rejection preserves the previous output. The API deliberately rejects structural PathChoice rows: that route continues to use its actual SafePanel adapter and the same fit math.

`UiSceneLogicalLayoutTest` passes MSVC `/O2 /W4 /WX /UNDEBUG`, including two invocations with identical function/frame addresses, split text atlases, material geometry, foreign overlap, incomplete/changed proof and atomic rejection. The existing observer test instantiates the adapter against its real Row type and passes; shared placement/HUD tests and seven decoder tests also pass. CMake test wiring is supplied separately to the coordinator; no runtime Hook, admission scope, layout policy or default manifest changes.

The minimum remaining evidence is one actual Hammer event linked to its pickup action identity and unique completed invocation, all of that invocation's text/tile/material members without an intervening flush, and the independent world/ticket/resource plus GPU viewport/scissor proof. Those facts must then authorize one immutable recipe for every member and bypass ordinary per-entry layout for that group. Current105 evidence has no Hammer or PathChoice coverage and predates runtime verification of the unique-token extension. This candidate has no GPU replay, hitbox or headset acceptance; enabling it for all HUD or writing a guessed Hammer offset remains unsupported.

## 2026-10-19 — complete-group sampling candidate prepared

Root built a separate `build-runtime-resolution1-ui-group` candidate with both
`GEARS_UI_CALL_BOUNDARY_OBSERVER` and `GEARS_UI_LOGICAL_GROUP_OBSERVER` enabled.
It includes the current PathChoice close/position renewal fixes. DLL SHA256:
`7066f6c2faebc7b5a8a706c75de57c9cba7cbe7d64d531e178f9bb4624413366`.
All 22 startup checks plus PositionService and CloseRestore tests passed.
The private build command, manifest and checks are under ignored analysis/runtime
(`build-runtime-ui-group.cmd`, `manifest-runtime-ui-group.json`,
`ui-group-startup-checks.json`). No default player manifest/package was changed.

The missing evidence remains an actual Hammer pickup/action invocation with its
complete text/icon/material membership and corresponding GPU output. Historical
chapter providers must be read again in the live process; old PID/address/index
values cannot be replayed. Current fit_invocation is not yet wired into shared
text/material replay. This build prepares that evidence collection only; it does
not claim Hammer placement or common UI layout is fixed. No game was started for
this candidate's checks.
