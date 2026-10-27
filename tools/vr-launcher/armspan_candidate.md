# Candidate startup armspan calibration

This path is limited to a source-worktree 6DoF candidate. The frozen 0.1.0
manifest, installed launcher EXE, native/XR artifacts, and portable-package
allowlist are unchanged. Enable both boolean capabilities
`controller_input_observer` and `armspan_startup_calibration` in a separately
validated candidate manifest, together with `startup_menu` and the existing
crosshair/UI/anchor requirements. Do not turn these on for an XR build without
the existing `--controller-input` publisher.

The launcher UI shows bilingual instructions and progress. The CLI uses the
same gate; `start --remeasure-armspan` requests a replacement measurement.
Missing or invalid records require measurement even without that flag. A valid
record is reused without opening a calibration XR child.

The first-run sequence is:

1. Reserve the owned startup-menu game through the existing launcher path.
2. Start one owned `OpenXRPoseProbe.exe` with its existing continuous-session,
   owner identity, startup-menu window, and controller-input options. Native
   gameplay activation has not occurred.
3. Copy Stereo-v4 and GVC3 mappings under their existing mutexes through a
   read-only view. Validate the exact hand/stereo ticket, ring retention,
   process creation times, publisher identity, QPC frequency, runtime source0,
   focus, tracking, geometry, and maximum 100 ms age. Mismatched protocol or
   process identity aborts; temporary tracking loss resets the hold.
4. Require both squeezes at least 0.8, arms on their proper sides in a
   heading-relative T-pose, a plausible 0.9–2.4 m span, and a steady pose within
   8 cm. The hold needs two uninterrupted seconds and at least 60 unique
   samples. Repeated polling cannot pad the count. Session, reference-space,
   input-profile, and input-epoch changes start a new hold.
5. Immediately write schema1 to
   `%LOCALAPPDATA%/GearsVR/weapon-calibration.json` using a flushed temporary file
   in the same directory and `os.replace`. Existing bytes are preserved until
   replacement succeeds. A changed record is refused rather than overwritten.
6. Close the consumer's own mapping/mutex handles, stop and wait for its own XR
   child, and require orderly exit before handing off. Timeout, cancellation,
   early exit, identity change, or cleanup failure prevents Native activation.
   Failure before a successful measurement and atomic save preserves the prior
   record. Once saved, the new valid record remains even if later XR cleanup
   fails; Native still stays inactive. No rollback is attempted, because it
   could overwrite another writer's intervening update.
7. Load the saved record into the normal runner before its Native command is
   built. The activation callback requires the same valid value, closing the
   record-disappearance/replacement gap during handoff.

The profile has exactly `schema`, `armspan_metres`, and
`hands_below_head_metres`. CLI and UI reject duplicate keys, invalid UTF-8,
boolean/string numbers, nonfinite values, unsupported schema, extra keys, and
out-of-range measurements. The record uses UTF-8 without BOM, matching the
frozen Native consumer. Autonomous behind-window tests cannot create a
personal measurement; they may reuse an existing valid profile.

Validation performed without a game or IDA:

```powershell
py -3 -m unittest discover -s tools/vr-launcher -p 'test_armspan*.py' -v
py -3 -m unittest discover -s tools/vr-launcher -p test_launcher.py -v
py -3 -m unittest discover -s tools/native-capture -p test_weapon_calibration.py -v
powershell -NoProfile -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1 -OutputDirectory analysis/vr-launcher-armspan -NoInstall
```

`armspan_wire_layout_test.cpp` is a standalone x64 `/W4 /WX` compile against the
existing C++ publisher headers; no CMake/default build change is needed. The
Python mapping tests use temporary uniquely named Windows objects and verify
that the last consumer close removes them. Startup lifecycle tests use fake
children, explicitly check wait/cleanup order and immediate persistence, and
never spawn OpenXR or the game.

Dedicated headset instructions, a headset progress display, slot try-grab
confirmation, real-controller runtime acceptance, and packaging are pending.
The saved span is a controller-to-controller T-pose measurement; this does not
establish character IK correctness or hand-animation ownership.
