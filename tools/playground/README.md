# happyotaku playground

Enabled in the ordinary 0.1.0 development launcher. Select `happyotaku playground`
at the bottom of the game's main menu. The launcher loads the
version-gated DLL during startup and reserves a disabled trampoline before the
engine's later allocations. After the registries are ready it enables the bridge.
It adds a styled button to the active
`WarUIScene_MainMenu`, then travels locally to `MP_WarMachine` with `WarGameSP`.
No cooked game package is patched or distributed.

The range uses the map's existing Troika, eleven equipment pickups behind it,
and six stationary Locust targets along the firing lanes. Targets respawn after
three seconds; equipment is replenished after pickup. Infinite spare ammunition
preserves the original magazine and reload behavior. Only the requested range
world and its local player are maintained, including while occupying the Troika.

Build with the existing MSVC/CMake toolchain:

```powershell
cmake -S tools/playground -B tools/playground/build-local -A x64
cmake --build tools/playground/build-local --config Release
& tools/playground/build-local/Release/PlaygroundHookTest.exe
```

`playground_menu`, `playground_early_reserve`, `playground_build`, `playground_dll`, and `playground_sha256`
bind the optional component in the launcher manifest. The normal native/XR
artifacts remain separately pinned. `stage_portable.py` includes only declared
runtime files and the DLL; analysis evidence and game assets remain excluded.

Engine work is scheduled on the observed local game thread. Indexed native
functions use an FFrame with explicit native parameters and out-parameter links;
unindexed functions use the original ProcessEvent. The loader checks executable
identity and live entry bytes, and independently verifies restoration on stop.
Shutdown also releases a reserved hook when startup stops before activation.
An optional initialization resource failure may continue VR only after an
independent full-entry restoration check; identity and certificate failures still
stop startup. Allocation diagnostics distinguish the trampoline and hook table.
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
bolt stroke, perfect reward, or default-launcher behavior is implied. GVE6
events include input epoch and are rejected against a stale hand-pose epoch;
each certified hand pose also carries the same XR ticket's game units per metre
for physical gesture distances. Older GVE5 publishers cannot open this bridge.

`GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST=ON` is a separate, default-off
original-input boundary trial in the owned muted arena. Its test exports queue
one Lancer `ButtonPress('B')` on the game thread, clear the original
`CheckForMeleeTarget` timer immediately and on each owned tick, and release B
on explicit stop, identity loss, timer failure, or Playground shutdown. A
release is accepted only after the original `bHoldingButton_B` field clears.
The separate `GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST=ON` option compiles a
default-disarmed `PlaygroundLancerSawXrPullArm/Stop/Status` trial. Only a fresh
GVE6 same-ticket left-hand pull near the provisional Lancer starter requests
the existing original B hold; no enemy contact, visible starter or default
launcher input is enabled. Status 2 is armed, 3 pulling, 4 B sent, 5 original
hold, 6 original animation/deferred stop, 7 restored, 8 stopped, and 9 a state
or release failure. The private trial auto-requests stop after 15 seconds.
Once the original attack special move 34 begins, Stop and shutdown defer B
release until the original attack state leaves; an animation longer than the
shutdown wait can make shutdown refuse, preserving the live hook until a
later safe retry.
The retail no-victim trial confirmed sustained original blade/melee state and
timer suppression; a second trial closed Playground while B was held and
confirmed the original B, melee and blade states returned to idle. This is
not yet driven by the hand-pull solver and does not perform blade contact,
paired execution, visual starter placement, or headset verification.

`GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST=ON` additionally requires the private
mesh-follow helpers and adds a read-only, post-PlayerTick sample of the
original `b_AR_Chainsaw` and `b_AR_ChainsawEnd` bone matrices with the original
gun matrix in the same game-thread tick. Separate start exports sample an idle
or already-running original saw. Both observed 16-frame retail trials found
stable pivots about 22.47 original units apart. This locates bone pivots only;
the visible tooth edge, target collision, occlusion, and paired execution
remain unverified. The sequential remote-reflection attempt was discarded
because the original gun component moved between calls.

`GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL=ON` is another private, default-off
trial. It uses the current original gun matrix and a provisional housing-side
starter anchor, feeds a 19 cm synthetic left-hand pull through the same
world-scale gesture model, then requests the original B hold only when the
gesture qualifies. The owned retail trial observed one pull request at about
18.81 cm in 297 ms, nine subsequent original blade/melee samples with the
automatic target timer off, and clean B release. The test publishes no XR
hand frame and draws no starter; the mount is not calibrated for a Quest hand.

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

The private `PlaygroundWeaponMeshFollowTrialStart` entry exercises the same
game-thread component path without pretending that the simulator supplies a
Quest controller. In a muted, behind-windows owned arena, first use the
private slot-2 tap to establish VR ownership of the equipped Lancer. The
trial then applies twelve bounded synthetic right-hand poses, holds each long
enough for the original component world matrix to update, checks translation
and rotation, disables the override, and requires restoration. Its status
encodes stage in the high 16 bits and completed samples in the low 16 bits;
3/12 means the trial passed, 4 means failed. The retail PID 40068 sample
passed with a maximum stable X error of 0.00977 original units and returned
`PlaygroundShutdown=0`. This verifies component motion/restoration only;
the actual OpenXR hand publisher, headset rendering, firing ray and recoil
still need their own tests. `lancer_support_probe.py --mode mesh_follow_trial`
records the bounded evidence under ignored `analysis/`.

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

`lancer_support_probe.py --mode socket` samples the original Lancer
`Magazine` socket and the gun component matrix in the same bounded game-thread
callback. `--mode socket_reload` repeats this during one original `ForceReload`.
Both modes require the same private, owned retail test range; their reports
remain under ignored `analysis/`. Use
`tools/weapon-models/analyze_lancer_socket_runtime.py` to project the samples
into the gun frame. The original socket is stable at idle but follows the
magazine bone far away during the reload animation. A VR feed-port guide must
freeze a gun-frame reference instead of following that animated socket. The
probe establishes neither insertion direction nor controller grip calibration,
and the normal DLL exports none of its controls.

The same private callback now also samples `b_AR_Magazine`'s original bone
matrix. An owned retail idle capture placed its pivot near the top of the
magazine and the socket near the bottom, separated along the gun's local `-Z`
axis. The pivot is a candidate upper feed-port anchor; controller-held item
orientation and the visible receiver opening still need validation before a
physical insertion can commit inventory.

The opt-in `GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST` candidate reuses the
original independent `MagazineMesh` after the private R3 reload hold starts.
An owned retail synthetic-left-hand trial established component attachment,
motion and release restoration. Its subsequent insertion-stroke observer
compares the held magazine against the current fixed gun-frame feed anchor,
requiring aligned, continuous movement from outside the port to a near-seated
position. The thresholds and hand mount remain uncalibrated for Quest; the
new observer has synthetic geometry tests and compiles. A retail follow-path
rerun showed no false insertion for an unaligned synthetic hand trajectory;
a separate aligned trajectory yielded exactly one arrival candidate in the
owned retail test. Both use simulated hand poses, not Quest controllers. An
insertion candidate only writes a private
status/log: it cannot settle ammo, complete a reload or grant a perfect bonus.

`lancer_support_probe.py --mode chainsaw_semantics` is a bounded owned-retail
test of the original Lancer melee input. It presses and releases original B,
reads the pawn/weapon's melee and blade state, clears the original
`CheckForMeleeTarget` timer in an empty arena, and confirms that the blade
continues until release without acquiring a victim. It does not implement a
VR pull gesture or exercise the paired execution.

The opt-in Lancer R3 reload-hold trial also projects this fixed upper anchor
through the current original weapon mesh matrix while the original reload is
held. It verifies the same mesh identity on every update and leaves the
original eight-second fallback timer in place if the mesh becomes invalid.
An owned retail trial captured seven bounded world-anchor rows while the
original explicit completion still settled ammo. These rows do not represent
a tracked left-hand magazine or a physical insertion.

The read-only original-reload visual probe now records matrices for the
separate `Assault_Rifle_Magazine`, original left-hand bone, and gun in one
callback while the magazine is attached to the pawn. The offline grip
analyzer finds the near-rigid middle hold and reports the moving pickup/return
portions separately. One retail run yielded 43 attached frames and a 21-frame
stable middle hold. This reference is in the original hand-bone frame, not a
Quest controller grip frame; it cannot commit a VR insertion on its own.

`GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` is a separate default-OFF, read-only
pickup/melee lifecycle observer. Its Start/Stop/Status exports sample only the
owned, behind-windows, muted test arena. It separates real ProcessEvent Hook
hits from bridge calls that bypass that Hook, logs current body-slot child
weights/targets/blends and eight original hand controls, and always leaves
`ownership_proved=0`. See the [probe schema and limits](../../docs/hand-animation-probe-20261022.md).
