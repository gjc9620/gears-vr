# Independent game instances

`start.py` provides an explicit multi-instance entry point. Each new name gets
private game settings, profiles, game logs, shader-cache copies, native DLL/status
files and a launcher index under `analysis/instances/<name>/`. The game EXE and
shared game assets are unchanged. The process-local early startup policy maps
only `Guardian_UnrealEngine3_1` to the chosen instance namespace. Unrelated OS,
input and driver mutexes retain their original names.

Start another ordinary player instance with the configured OpenXR runtime:

```powershell
python tools/multi-instance/start.py --name player2
```

Start an autonomous Mod regression beside an existing player instance:

```powershell
python tools/multi-instance/start.py --name autoplay --playtest --seconds 21600 --fixture analysis/playtest-fixtures/20261020-cole-window47-history.json
```

Use a fresh name for each launch. The command prints the worker PID, private
launcher index and test report directory. A player launch retains the normal
configured runtime; an autonomous run uses the verified behind-windows test
simulator, muted audio, observe-only tiny-output monitoring, disabled physical
background-controller input and no saved images/video. It never changes the
system OpenXR runtime.

After explicit user authorization, set
`GEARSVR_TEST_ALLOW_MANUAL_PREVIEW_ACTIVATION=1` in the test child environment
to keep playing when the user brings that run's simulator preview forward.
The observer verifies the XR process identity and exact preview window class,
records the allowed overlap, and retains the exception while that same preview
remains present. Startup still uses the behind-windows policy; the game window,
other runs and unrelated dialogs do not receive this exception.

To stop an autonomous run, create `stop` inside its printed playtest directory.
The runner releases its inputs, cleans up its own capture/XR session, closes
its own game normally and restores its private settings/profiles. It does not
restore old backups over a user's shared profile or terminate another game.

Normal repository launches ignore only registered private processes whose PID,
creation time and EXE path still match their private index. Unknown games retain
the shared-profile protection; use this entry point to give another player a
separate profile. The ordinary player launcher and the test launcher hold
different named owner locks.

The early folder override is restricted to LocalAppData queries originating in
the main game image. RoamingAppData and driver/SDK folder queries retain their
normal locations. Game-data/log path operations are redirected consistently,
including ordinary reads/writes, attributes, enumeration, atomic move and delete.
This is targeted game-instance isolation, not a general Windows sandbox.

Initial shader-cache copies are bounded to 512 files / 2 GiB. Copies consume
private disk space but avoid the large cold-cache rebuild seen with empty
directories. Each source is read twice and each destination is checked; a changing
source stops preparation. Native artifacts and copied files are hashed in
`instance.json`. Private game derivatives remain ignored under `analysis/`.

The new startup policy passes its exact-mutex, unrelated-mutex, folder and
physical-original-file preservation tests. Background placement is also checked
separately. A legacy fixture's restore/minimize expectations do not apply to the
test-only behind-windows show policy; its whole-suite result is retained without
being promoted to a pass. Actual game occlusion and foreground conditions remain
input guards: a failure is recorded and never repaired by moving user windows.
