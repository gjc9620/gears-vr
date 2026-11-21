## Adopt real left-hand captures into Lancer source (2026-11-12)

`adopt_lancer_captures.py` is a separate offline source-authoring path, distinct from the older fingers-only `transfer.py` V2 editor export. It selects ONLY each archive's capture.part/capture.side, requires four calibrated, unobstructed, fully tracked left snapshots for the exact original Lancer/Marcus model, and preserves the right main grip. It converts all15 finger quaternions AND the weapon-local wrist, including reflected object/wrist bases. Magazine wrists become item-local using the exact detached-magazine baked basis; C is unposed from its captured deployment before following the actual current angle/stroke; E follows its actual slide. Capture volumes are retained in the raw archives, not imported by this operation.

Example (choose a new output folder; raw archives are never overwritten):

```powershell
python tools/quest-workbench/adopt_lancer_captures.py --captures <capture-directory> --output <new-review-directory> --apply
```

The generated `weapon_lancer_owner_grabposes.h`, paired left finger tables and the Lancer support/magazine/E/C consumers form one source change. The operation preserves the right-hand SHA and refreshes source mod-baselines.json after adoption. Do not apply an old baseline's finger-delta export on top of the new compiled baseline. This does not install an APK, start a game, replace an active editor, or prove collision-free full arms/gameplay/headset fitting. Original independent capture IDs/SHA and previous source tables are kept in the review directory.

For0.3 captures carrying `part_delta_known`, use `adopt_moving_lancer_captures.py` with the same
arguments. It removes the full captured moving-part transform, then calls the unchanged legacy
selection/tracking validation and conversion. The original adopter remains frozen for existing
source-adoption consumers; calling it directly on a0.3 moving-part archive is unsupported.

# Quest weapon workbench

Private standalone Quest3/3S OpenXR authoring and controller practice application.
Current candidate: J:/gearsVr/candidates/quest-workbench-20261111, version0.4.0-dev, build13.
The Windows companion is Quest Workbench.exe. See 使用说明.txt for the complete Chinese guide.

0.3 removes runtime recording/history/replay. Legacy core.cpp/legacy_recording.h and desktop .take
readers are archival compatibility only, excluded from Android and host build targets. Single real
hand capture, independent capture archives, retarget/FK, current-pose overview and finger/wrist edits
remain. No controller-derived or authored preview is labelled a real26-joint hand observation.

The edit/practice loop uses A/X to return without restarting. Five colored acquisition regions and
part controls remain; the inspector defaults pinned60cm to the side and can be set45..120cm away.
A library preview can be explicitly adopted for testing. Stored real captures override authored defaults.
Each untouched default is identified as authored_default, with no tracked flags/source/time claim.

Controller practice implements main pickup, two-hand support, gun-owned25m sight zero and10m target
hits, installed-magazine extraction, old-magazine reinsertion, left-shoulder replacement supply,
assisted insertion, empty reload/observed E return and original C rail/full-travel/held motor/first-real-
release3s tail. Focus, tracking, time/space discontinuities clear held ownership and require a fresh
press. Grip requires actual OpenXR position/orientation tracked+valid flags15; aim remains display-only.
Menu hover suppresses trigger shooting and requires release before resuming fire.

practice.h is a standalone scene adapter around exact frozen portable Mod headers in game_rules/.
freeze_rules.py pins their transitive include bytes; assets/game-rules.json lists37 header hashes (the prior36 plus the Gnasher lever kernel).
These are the same geometry/state kernels, not the original engine, hooks, live actor identities,
inventory writer, animations, physics, recoil or audio. The shoulder uses the default body_slot layout
with stable initial/workspace yaw and head translation as a body-position proxy, adjustable center/scale.
The no-body-tracker limit and need to recenter after turning remain explicit.

Shooting cadence, visual recoil, vibration gain and quiet synthesized AAudio sounds are adjustable
PREVIEW parameters. The firing simulation does not prove the original game has fired. C uses simulated
scene acknowledgments paired with actual sandbox render submissions; those never claim game engine
execution receipts. E waits for visible mechanism acknowledgments and the shared return sequence.
Shared stroke speed, travel, timing, identity and fresh input rules stay unmodified.

Original geometry is preserved: body10798 + receiver1300 + C1089 + E54 + magazine156 =13397 triangles.
The detached magazine is the original bone8 partition. Full Lancer.glb remains byte-identical to0.2.0.
Original vertices are transformed by A=(-X,Z,-Y)*.01 with reversed winding. Marcus wrist geometry/
reference uses S=diag(-1,1,1); wrist transforms use A*R*S. Current poses follow moving-part deltas,
including C deployment/pull, E travel and detached magazine. Legacy capture fields remain supported.
General part_delta_in_object permits offline adoption to remove the actual captured part transform.

Build Android: py tools/quest-workbench/build.py --dependencies PINNED_DEPS --output FRESH_OUTPUT.
Isolated pinned NDKr27c, Android35 packaging tools/platform, Android29 minimum/32 target and Khronos
1.1.63 loader remain. No Java/Gradle application layer or development keystore is shipped.
prepare_assets.py uses retained hash-checked retail geometry and numpy/Pillow. Materials remain clay.
Rendering retains1.15 recommended dimensions,2xMSAA, batched text and instanced debug primitives.

Host checks: CMake/MSVC builds WorkbenchCoreTest and WorkbenchPracticeTest. Run the former with a
fresh fixture directory, the latter with assets/Lancer.parts.json. Practice trajectories cover cold
press, menu suppression, source loss, volume/support release, stable shoulder, tactical and empty
reload/replacement/insertion/fire, old magazine restore, short/full/observed E, C no-gesture/no-render
rejection, indefinite hold, release tail, reverse stop/restart and tracking loss. They are CPU fixtures,
not physical headset acceptance. Python conversion checks: py -m unittest discover -s tools/quest-workbench
-p test_transfer.py -v. Four retained real captures also round-trip through arbitrary moving-part deltas.

Saving project.json/model.project.json preserves poses/regions, shoulder settings, practice feedback
parameters and panel placement; interaction-settings.json is a companion settings copy. Exports keep
all archives. The existing desktop V2 editor conversion remains finger-only. The separate offline
adopt_lancer_captures.py source-authoring flow described above handles selected full wrists/fingers.
New region/snap/shoulder/preview values are not automatically installed into the Mod. A later explicit
Mod configuration integration and real-game comparison are required for complete parity.

No PC game or simulator is started for this workbench change. No full-arm IK, generic imported model
rig/animation retargeting, textures, enemies, executions, simulated weight or collision acceptance is
claimed. A successful compile/host check is separate from Quest startup, UI visibility and owner feel.


0.5.0/build14 extends the twelve-model catalog with curated interactive roles, editable starting
poses and standalone per-weapon practice. armory_practice.h implements the requested video/user
reload policies, including Boltok group deficit loading + fresh Grip closure, Gnasher right-down/
return, Hammerburst no charge, Longshot lift/pull/push/lock, Snub empty slide, Boomshot carrier and
Bow release fire. Beam, turret and grenade previews remain distinct local approximations.
Yellow next-step guides default on and persist with project data. Main UI is weapon picker -> part
chips -> two capture buttons + large test button; advanced controls collapse. Part chips preview
current stored poses. Switching preserves workspace heading and grip contact. No recording returns.
Generic profiles are exported as workshop_profile; Mod import is not automatic.

Asset pipeline: prepare_catalog.py -> prepare_workshop.py -> prepare_assets.py, using the pinned
numpy/Pillow Python runtime. Original surfaces/bone inventories remain conserved in the retained
meshes. Boltok rendering uses the documented authored open-cylinder replacement; arrow is an
explicit authored preview. Cached Boomshot vtx60 positions are ALREADY METRES; apply .75 player
scale, not another .01. test_catalog.py checks geometry, curated roles, ammo bounds and atlas.
WorkbenchArmoryTest adds eleven non-Lancer routes and yellow-stage/input-loss checks. Native UI
and physical hand fit still require actual Quest inspection, distinct from these CPU fixtures.


0.5.1/build15 removes revolver/pistol fore-end editor entries. Both sides use the same main-grip
pose/volume; support does not steer aim via the tiny clasp baseline. Internal legacy support slots
remain for archive compatibility but cannot be selected as independent editor parts. Gnasher lever
is not independently grabbed and its editor entry is hidden. No quickloader plate is rendered;
revolver supply remains grouped loose rounds with the unchanged C-L-S deficit accounting.
Ammo entries are labeled 子弹: selecting hides all gun meshes, all other acquisition regions and
non-interactive aliases. It displays only the actual reload item (box magazines for magazine-fed
guns), keeping its original object frame and capture schema. Captures feed the existing ammo
wrist/finger route. Gun/parts return on selecting a different part. Data is not deleted or relabeled.
preview_captures.py is read-only: reconstructs Marcus LBS from stored finger quaternions and wrist,
with actual captured component transforms. Inline preview uses explicitly simplified connected
surfaces; raw26 tracked joints can be compared. It is not a headset screenshot/contact acceptance.


0.5.2/build16 owner correction: support remains an independent Second Hand entry, including
revolver and Snub. Its volume, wrist, fingers and archive identity remain support, never aliasing
main_grip. Existing support captures are used directly; automatic copying into main-left is removed.
shared_grip now only prevents short-baseline two-hand steering for pistols. Starting support points
for pistols are near the grip; other weapon support points remain unchanged. Original capture labels
are retained as history. Previously rejected three archives stay excluded from the22-entry curated
dataset; no deleted files are restored. Ammo-only editing and no quickloader plate remain unchanged.


0.5.3/build17 uses the whole six-chamber cylinder as the revolver reload item, with its own
feed-relative mesh exactly aligned to the mounted replacement cylinder. No single rounds/loader
plate are rendered as the revolver item. The mounted cylinder is hidden while the replacement is
held; insertion restores it. C-L-S accounting still transfers only the actual missing rounds.
The editor exposes 装填弹巢 separately from the mounted 弹巢 operation pose.
Isolated ammo can be picked up with either valid controller Grip, translated/rotated, released to
fix placement, summoned in front of the operating hand or reset. Editor pickup uses padded actual
mesh bounds independent of authored gameplay volumes. The hidden object frame follows the item,
so stored wrists/raw joints/fingers stay object-local. The gun world placement is restored before
leaving the item editor, switching weapons or entering practice. Held-item capture is rejected.
Real captures on all other weapons remain unchanged. CPU global rigid-transform capture invariance
and cylinder-seat geometry/visibility/deficit tests cover this change; Quest feel remains separate.
