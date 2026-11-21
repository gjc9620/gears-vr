# Lancer live editor — integrated development candidate (2026-11-11)

The current package is `J:/gearsVr/candidates/integrated-editor-20261111`.
`Open Desktop Editor.cmd` opens the normal launcher in packaged-simulator mode, the five-part pose editor, and the mouse-operated left/right controller panel. `Open Live Editor.cmd` opens the headset/editor workflow. The candidate's `先看这里.txt` has the complete Chinese steps. Public v71 and the fixed previous manual entry are preserved.

The desktop panel supplies human-controlled position, yaw/pitch/roll, held Grip/trigger, and A/B/X/Y/menu pulses. It uses this launch's isolated simulator directory, with serialized consume-once commands. No global runtime registry or LOCALAPPDATA changes. A separate heartbeat makes the simulator release buttons after the control application stops. Physical headset runtimes ignore this private simulator interface. Moving a controller is separate from editing a wrist/finger offset; real skin applies the edited profile only during the existing actual current grip.

Current editor protocol is **GVRPOSE2**, with five static part banks and display flags. Native/Playground use the matching V2 exports. Main grip adjustments change the weapon/muzzle mount; left adjustments are wrist-local changes to the admitted left visual pose. All original identity/current-source/DesiredBones/fixed-length checks remain. Yellow in-game lines preview the selected Grab Pose using the original rig translations, while red/cyan/purple ranges display current acquisition/insertion/retention geometry. There is no standalone skinned viewport or direct three-dimensional gizmo yet.

All five production components compile/link. Focused CPU/host/protocol/launcher-order and package checks pass; actual gameplay, visible pose changes in this integrated version, and headset acceptance are still pending. Source provenance, hashes, failures and results are saved in `J:/gearsVr/analysis/integrated-editor-20261111`. No game was launched for this integration round. Earlier development-version notes below describe the initial main-grip implementation and its historical evidence; their V1-only scope is superseded here.

---

# Lancer Pose Editor — development version 1

Windows desktop panel for the existing Lancer right main grip. Open `Open Pose Editor.cmd` on a machine with Python 3 and Tk installed. The current development machine has Python 3.14 with Tk. The panel can edit and save profiles offline; the installed public v71 does not expose the new runtime interface.

## Implemented

- Main-grip contact translation (millimetres) and rotation (degrees).
- Fifteen right-hand joints, each with three local rotation corrections; finger-wide incremental adjustment.
- Live preview, explicit send, undo/redo, save/load JSON, restore baseline.
- Separate received-configuration, completed C0 finger transaction, and completed weapon-component-write receipts. Stale runtime output is shown as stale; receipts are not final GPU/skin verification.
- Optional Native worker file reload, default OFF. No file parsing or file writes in C0/render callbacks. Callbacks take a nonblocking copy of static authoring data; transient contention can retain the previous static configuration, never old input or permission.

## Integration

Build Native and Playground together with the new optional exports. For a newly prepared PRIVATE capture session, add the following to that session's `capture.ini` **before Native starts**. The channel must be a separate, existing absolute directory for this session:

```ini
[PoseEditor]
enabled=1
channel=J:\gearsVr\analysis\pose-editor-20261111\session
```

Open the editor with `--channel` pointing to the same directory. Do not attach it by replacing DLLs in an already-running game. No public launcher, manifest or installed candidate was modified in this change.

The editor atomically replaces `preview.bin`; Native checks the fixed size, magic, FNV-1a corruption checksum, geometry signature, finite values, bounds and monotonically increasing revision. FNV is a corruption check, not an authentication mechanism. This is local authoring data and grants no action rights. Native retains the last valid configuration after an invalid, missing, older or revision-reused file and writes `status.json` from its existing collection worker. Poll cadence is 100 ms, not a measured end-to-end latency guarantee.

Each right main-hand C0 use still requires the actual admitted Lancer authored profile, current source, model, rig reference and all fifteen bones in the captured LOD DesiredBones. Only its existing finger writer postmultiplies the correction onto the current admitted pose. No second bone writer, controller sample, timestamp, grant, or relaxed freshness interval is added.

The main contact delta uses the original grip's forward/right/up axes, in metres. If the baseline hand-to-mesh transform is `B` and the edited palm contact is `D`, the new relation is `inverse(D) * B`. Derive it anew from the original mount on every update. The same adjusted mount feeds model matrices, weapon following, contact consumers and the existing muzzle calculation. **This alignment edit can change the gun and muzzle direction.** It is not a visual-only wrist correction. Existing identity, endpoint, source and component lease checks still gate every write and action.

Finger corrections use `current_local_rotation * delta`, with `delta = Rz * Ry * Rx`. The interface's index/middle/ring/little/thumb order matches existing Marcus115 bones 36..50. Translation norm is limited to 50 mm, palm rotation to 45 degrees, and each joint correction to 90 degrees; these are authoring bounds, not a claim of anatomical validity. The original joint translations, scales and bone lengths remain fixed.

Zero deltas select the existing compiled authored posture. Disabling preview restores the existing baseline through normal current-frame processing. Closing the panel normally sends a disabled revision. An editor crash does not send a disable: Native keeps its last valid static configuration until a new command or the end of the Native session. One editor should own a session directory.

## Evidence and remaining work

The first isolated production build is based on frozen natural-hand V144, with only this editor's changes overlaid. The live shared checkout contains unrelated in-progress weapon/cover changes; an attempted broad build failed there and was not used as an editor candidate. The isolated sources, exact diff, binaries, hashes, and logs are retained under `J:/gearsVr/analysis/pose-editor-20261111`.

Completed checks: Python profile validation/save/atomic transport; Python-to-C++ wire interoperability; independent inverse-contact transform and nonaccumulation; real Native worker reload/rejection/restoration in a host fixture; original Marcus115 owned-buffer finger rotation and fixed-bone preservation; desktop button/undo/send/normal-close inspection. Native and Playground production DLLs compile/link.

**Not yet game-recorded:** changing position plus one joint while holding Lancer, observing both actual rendered changes and same-version receipts, restoring and reloading a saved profile in the same game, and normal game/XR cleanup with this build. No headset acceptance or complete natural-fitting claim. The first version also does not include a skinned 3D viewport, VR point recording, left-hand editing, mirroring, collision fitting or an IK visualizer.

## Focused checks

Run Python checks with `py -3 -m unittest discover -s tools/pose-editor -v`. The CMake project in this directory builds three focused host/CPU executables. `PoseEditorTest` optionally accepts the generated Python wire fixture; `PoseEditorRuntimeTest` takes a private test channel directory; `PoseEditorC0Test` takes a retained original-rig fixture directory containing `reference.bin`, `C0-local.bin` and `C0-component.bin`. These executables never start a game.
