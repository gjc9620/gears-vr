# Independent game instances

The isolated simulator entry `isolated_sim.py` has no fixed instance-count
limit (owner override, 2026-11-15). The owner also removed the 1 GiB free-RAM /
6 GiB free-commit admission threshold: memory is recorded but does not block a
launch. Disk availability and process identity checks remain. Existing sessions
keep running if a new launch fails.
Its background mode hides the owned game and simulator windows with DWM cloak;
the processes continue rendering, but no preview appears on the desktop or
taskbar. Instance count and window visibility are separate settings.

For owner-requested desktop-visible instances, use a fresh name:

```powershell
$instanceName = "visible-$([guid]::NewGuid().ToString('N'))"
python tools/multi-instance/isolated_sim.py prepare --name $instanceName --visible --lowest --render-scale 0.5
python tools/multi-instance/isolated_sim.py start --name $instanceName --wait-resources-seconds 0
```

Visible mode selects the verified non-cloaking startup/runtime, shows the game
and simulator as normal taskbar windows, and retains private profiles and normal
stop/cleanup. An already-running hidden instance must be stopped normally and
replaced with a fresh visible instance; it is not hot-switched.

Add `--lowest` to `prepare` for the owner's low-load multi-instance comparison.
It backs up each private EngineUserSettings INI, lowers existing graphics fields
(shadows/AO/reflections/post effects off, low detail, texture groups capped at
256), sets the desktop startup dimensions to 800x600 and VR scale to 0.25.
The per-instance `lowest-graphics.json` lists every changed field and its hash;
`lowest-graphics-originals/` retains the copied original settings. This does not
overwrite shared settings or represent a verified in-game named preset.

The current owner-requested comparison overrides the low-load preset's VR
scale with `--render-scale 0.5`; there is no fixed 1080p requirement. Use a new
name for every restart, including after a failed start. Do not adopt or stop
another chat's instance. No recording is enabled by these commands.

The optional older `--full-hd` mode requests a 1920x1080 desktop and total
side-by-side stereo image, with 960x1080 per eye at scale 1.0. Runtime render
receipts remain separate from the requested size plan.

Visible load-test instances skip startup movies via the private child-only
`-nomoviestartup` switch. This avoids the observed intro-movie startup stalls;
normal shared/manual launch behavior is unchanged. Private player discovery is
bounded to 180 seconds and uses the existing normal cancellation/cleanup path.
`status --name ...` now includes `startup_status.startup_confirmed`: it requires
an actual first-stereo receipt, a running native session, and matching live
launcher/game/XR identities. A process or queued map request alone is not ready.

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
