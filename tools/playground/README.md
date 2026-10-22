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
two live virtual targets as RVAs; this extension has compile coverage but no
retail log yet. It remains read-only and does not establish the reload window.

`GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST=ON` adds an even narrower private
Lancer trial to the equipment-intent build. It requires an owned, muted,
behind-windows Playground range and an already held Lancer. Call
`PlaygroundWeaponMeshFollowEnable` after the certified hand-pose publisher is
live; `PlaygroundWeaponMeshFollowStatus` returns bit 16 for an active component
override and a low status of 2 while the original `Weapon.Mesh` is following
the right hand. Call `PlaygroundWeaponMeshFollowDisable`, wait until bit 16 is
clear and status 6 reports restored (or 7 reports a destroyed component), then
call `PlaygroundShutdown`. Shutdown returns 20 while restoration is still
pending. The first tracked frame retains the game's current gun pose, so this
trial checks component motion without claiming a correct per-weapon grip mount.
It has pure transform and build coverage, but no successful retail runtime or
headset result yet; the normal DLL contains none of these exports.

The same private DLL has a separate read-only Lancer support-hand probe.
`lancer_support_probe.py` equips the original Lancer in an owned muted range,
calls `PlaygroundLancerSupportProbeEnable`, and requires status 3 with 24 samples
before accepting the ignored `analysis/` receipt. The callback captures the
original character's left IK, left/right hand bone matrices, and unchanged
pawn/gun component matrices after each original `PlayerTick`. Status 4 means a game
identity, bone, matrix, or test-environment guard rejected the probe. Do not
enable mesh following during this sample; either export refuses that overlap.
The matrices are evidence for later grip-point calibration, not a validated
grip offset or a component to ship with the Mod.
