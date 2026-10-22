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
when neither weapon has one, and weapon pose transfer are not yet implemented.
The default DLL has no XR equipment-intent consumer.
The same private bridge also carries left-shoulder ammo-grab presses. The
adapter checks the held weapon's original spare count and infinite-spare rule:
status 1 means ammo exists, 2 means empty with the original cue, 3 means empty
without a cue, and 4 means the game identity or count could not be validated.
No round or magazine is spawned or deducted at this stage.
