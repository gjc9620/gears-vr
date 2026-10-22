# happyotaku playground

Enabled in the ordinary 0.1.0 development launcher. Select `happyotaku playground`
at the bottom of the game's main menu. The launcher loads the
version-gated DLL before entering gameplay. It adds a styled button to the active
`WarUIScene_MainMenu`, then travels locally to `MP_WarMachine` with `WarGameSP`.
No cooked game package is patched or distributed.

The range uses the map's existing Troika, eleven equipment pickups behind it,
and six stationary Locust targets along the firing lanes. Targets respawn after
three seconds; equipment is replenished after pickup. Infinite spare ammunition
preserves the original magazine and reload behavior. Only the requested range
world and its local player are maintained, including while occupying the Troika.

Build with the existing MSVC/CMake toolchain:

```powershell
cmake -S tools/playground -B tools/playground/build-playground2 -A x64
cmake --build tools/playground/build-playground2 --config Release
```

`playground_menu`, `playground_build`, `playground_dll`, and `playground_sha256`
bind the optional component in the launcher manifest. The normal native/XR
artifacts remain separately pinned. `stage_portable.py` includes only declared
runtime files and the DLL; analysis evidence and game assets remain excluded.

Engine work is scheduled on the observed local game thread. Indexed native
functions use an FFrame with explicit native parameters and out-parameter links;
unindexed functions use the original ProcessEvent. The loader checks executable
identity and live entry bytes, and independently verifies restoration on stop.
Status and logs are PID-specific beside the DLL. `PlaygroundTestEnter` is gated to
the explicit muted, behind-windows autonomous test environment.

Game-version offsets, layouts, and observed behavior are specific to the checked
Reloaded executable. See the development record in `docs/releases/0.1.0/PROGRESS.md`
for completed validation and remaining headset acceptance.

The weapon-6DoF branch also has a **private, opt-in Hammerburst R2 candidate**.
Build with `GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2=ON` and a distinct
`GEARS_PLAYGROUND_OUTPUT_NAME`; this exposes `PlaygroundVrFireEnable`,
`PlaygroundVrFireDisable`, and `PlaygroundVrFireStatus`. Enable it only after
the matching stereo and controller-input publishers are live. It consumes exact
XR frame keys on the local player's `PlayerTick` and forwards right-trigger
edges to the original `ButtonPress/Release('R2')`. Short bridge gaps keep the
last valid frame until its 100 ms limit; longer loss safely releases this
verified automatic weapon. Disable the adapter before `PlaygroundShutdown`.
The default build excludes these exports and does not forward VR fire.

`GEARS_PLAYGROUND_VR_R2_SIM_TEST=ON` additionally builds the isolated
`VrFireTestPublisher` and accepts simulated input only in the muted,
behind-windows test child. It is a verification tool, not a launcher option or
headset acceptance. Other weapons remain gated until their original
press/release, cancellation, and co-input behavior are verified.

The additional `GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2=ON` candidate requires
the R2 frame bridge. The same explicit `PlaygroundVrFireEnable/Disable` session
then forwards left-trigger edges to the original `ButtonPress/Release('L2')`
and exposes `PlaygroundVrAimStatus`. It is limited to Hammerburst while the
game's `bToggleTargeting` setting is false. Toggle-to-aim is gated because a
release does not cancel its latched state. One direct original-button retail
probe confirmed hold-to-aim, and the edge state machine passes isolated tests;
the combined XR-frame-to-game L2 candidate still needs a successful running
game test and headset acceptance. The default DLL excludes all VR input
exports.

For private inventory verification, build with
`GEARS_PLAYGROUND_EQUIPMENT_OBSERVE=ON` and a distinct
`GEARS_PLAYGROUND_OUTPUT_NAME`. Every 500 ms on the local player's game
thread, the observer checks the four original inventory slots, each occupied
weapon's `HasAnyAmmo`, current weapon, switch permission, switch animation flag
and grenade charges. It writes
changes to the PID-specific Playground log. The grenade charge read follows
the original human-player `HasAnyAmmo`/spare-ammo rule; unknown quantities
fail closed in the equipment state model. This candidate only observes: it
does not move a gun, suppress an animation, play a sound or call
`SetWeaponFromSlot`. The default build excludes the observer, and a successful
compile does not establish a valid retail runtime snapshot.

The private `GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST` candidate additionally
requires the inventory observer and `GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST`.
With the certified native controller observer active, a right-grip press near
the left/right shoulder or trouser holster publishes only the matching original
slot number plus current game identities. Playground discards old or overflowed
events, then re-reads the local player's inventory on the game thread and
requires the private owned arena, behind-windows and muted test environment
before calling the original slot function. Empty grenade charges fail closed.
When a requested slot is empty or its weapon has no usable ammunition, the
private adapter tries the slot weapon's
original `FireNoAmmoSound` cue, then the current weapon's cue, through the
original local playback function. Status 9 means neither cue could be played.
This is a wiring test: original switch animation, visible stow, an audio cue
when neither weapon has one, and playable weapon pose transfer are not yet validated.
The default DLL has no XR equipment-intent consumer.
The same private bridge also carries left-shoulder ammo-grab presses. The
adapter checks the held weapon's original spare count and infinite-spare rule:
status 1 means ammo exists, 2 means empty with the original cue, 3 means empty
without a cue, and 4 means the game identity or count could not be validated.
No round or magazine is spawned or deducted at this stage.

The separate `GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST` option requires the
certified equipment-intent candidate and remains off in the default DLL. A
fresh R3 event can start the original Lancer `Reloading` state in the owned,
muted, behind-windows arena; the adapter replaces its 2.5-second completion
timer with an eight-second original-game fallback. The test-only start and
complete exports exercise the same game-thread path without physical input.
Explicit completion invokes the original `Reloading.EndOfReloadTimer` state
function, which performs the original ammo settlement. No VR magazine stroke,
bolt stroke, perfect reward, or default-launcher behavior is implied. GVE5
events include input epoch and are rejected against a stale hand-pose epoch.

While certified hand poses are being observed, the private bridge also logs a
read-only `weapon_magazine_probe` beside `weapon_mesh_probe`. It validates the
current weapon's original `MagazineMesh` component, names its `StaticMesh`,
reads `HiddenGame`, and asks the pawn's skeletal mesh whether the component is
attached there. `known=0` rejects an incomplete or unexpected layout; `present=0`
with `known=1` means this weapon has no current independent magazine component.
The adjacent read-only `weapon_ammo_bone_probe` now uses the original
`FindSkelControl(AmmoControl)` lookup and records `ControlStrength`,
`StrengthTarget`, and `BoneScale` when present. A missing control is
recorded separately from an invalid reflection layout. These values are
not by themselves proof of rendered magazine visibility. Neither probe
changes meshes or reload state; a retail runtime sample is still required.

In the same private owned arena, `weapon_reload_native_probe` reads the
original `WarWeapon` UFunction objects for `GetActiveReloadValues` and
`DetermineActiveReloadSuccessOrFailure`. It validates their class ownership
and executable entry pointers inside the current game image, then logs only
module-relative addresses, native indices, and parameter sizes. It does not
call, detour, or patch either function. Unlike the hand-pose probes, this
lookup needs only an authenticated equipped weapon, so simulator sessions
without physical controllers can reach it. A build pass alone provides no
retail address or perfect-reload deadline; a valid runtime log and byte
comparison are required before native analysis or timing claims.
The retail owned-arena sample located the two UFunction wrappers and matched
their first 256 bytes to the historical runtime snapshot (the shorter judge
wrapper was matched in full). Those wrappers dispatch to virtual slots `0x638`
and `0x630` on the equipped weapon. The probe now also validates and logs the
two live virtual targets as RVAs. In a later owned retail arena, the equipped
Lancer resolved the getter to RVA `0x3e81b0` and the judge to RVA `0x3e7d28`.
The first 4 KiB at each target matched the corresponding historical runtime
snapshot byte for byte. This covers the main function blocks, but not the
getter's separate tail block. The probe remains read-only; this match does
not establish any weapon's perfect-reload deadline.
The adjacent `weapon_reload_window_probe` validates the current weapon's
reflected float offsets and reads the original duration array and owner index
on the same authenticated game thread. It never calls the getter or the judge.
One owned retail Lancer instance yielded start `0.7`, array index 0 and width
`0.1`, giving a derived endpoint near `0.8` seconds. This is a checkable
candidate, pending a direct original getter-output comparison.

`GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST=ON` adds an even narrower private
Lancer trial to the equipment-intent build. It requires an owned, muted,
behind-windows Playground range and an already held Lancer. Call
`PlaygroundWeaponMeshFollowEnable` after the certified hand-pose publisher is
live; `PlaygroundWeaponMeshFollowStatus` returns bit 16 for an active component
override, bit 17 while the left support grip is latched, and a low status of 2
while the original `Weapon.Mesh` is following the right hand. Only the
`COGAssaultRifle` asset is accepted. A fresh LGrip press near the measured
left-palm point on that mesh enables two-hand steering; tracking loss, grip
release, or moving out of range drops back to the right-hand pose. Holding
LGrip through component binding does not count as a new press. Call
`PlaygroundWeaponMeshFollowDisable`, wait until bit 16 is
clear and status 6 reports restored (or 7 reports a destroyed component), then
call `PlaygroundShutdown`. Shutdown returns 20 while restoration is still
pending. The first tracked frame retains the game's current gun pose, so this
trial checks component motion without claiming a correct per-weapon grip mount
or a measured controller-to-palm offset. The original-pose contact coordinate
comes from the 24-frame retail sample described below. It has pure transform
and build coverage, but no successful mesh-motion runtime or headset result
yet; the normal DLL contains none of these exports.

The same private DLL has a separate read-only Lancer support-hand probe.
`lancer_support_probe.py` equips the original Lancer in an owned muted range,
calls `PlaygroundLancerSupportProbeEnable`, and requires status 3 with 24 samples
before accepting the ignored `analysis/` receipt. The callback captures the
original character's left IK, left/right hand bone matrices, and unchanged
pawn/gun component matrices after each original `PlayerTick`. Status 4 means a game
identity, bone, matrix, or test-environment guard rejected the probe. Do not
enable mesh following during this sample; either export refuses that overlap.
The matrices are evidence for later grip-point calibration, not a validated
grip offset or a component to ship with the Mod. An owned retail capture did
complete 24 settled Lancer frames with left-hand IK active. The median left
palm point in the original gun frame was `(8.556242, 43.525180, 0.165157)`
raw game units; maximum frame drift was `0.000231` units. The capture and
derived report stay under ignored `analysis/lancer-support-probe-20261022/`.
The same bounded callback can read the original `MagazineMesh` component and
`AmmoControl` without changing them. One owned retail held-Lancer sample found
`Assault_Rifle_Magazine` present and `HiddenGame=false`, but not attached to
the pawn mesh; `AmmoControl` was present with strength, target, and bone scale
all zero. These idle-state fields do not establish final rendered visibility
or the handoff timing during the original reload.

Run `lancer_support_probe.py --mode reload_visual` in a separate owned range
to seed one missing round in the private arena and invoke the original
`ForceReload`. The private game-thread callback records up to five seconds of
`AmmoControl`, `MagazineMesh` pawn attachment, original state, and ammo counts;
`tools/weapon-models/analyze_lancer_reload_visual_runtime.py` reports sampled
event intervals. One retail Lancer run captured 318 samples: gun magazine
control turned on at elapsed 0.258–0.275 s, the separate hand magazine
attached at 0.774–0.791 s, both reverted at 1.479–1.500 s, and original
reload/one-round ammo commit finished at 2.497–2.510 s. These are sampled
intervals from one original animation, not VR interaction deadlines. The
normal DLL does not export this probe.
