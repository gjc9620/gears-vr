# Game audio launch policy

`GEARSVR_TEST_MUTE_GAME=1` opts the repository launcher into per-process test muting.
Set it only on the automated test's child environment. It takes precedence over
the manual foreground launch policy. Manual foreground launches set each newly
observed exclusive game audio session to 100% and unmute it once; subsequent user
adjustments are preserved. Background launches without the test flag do not
change audio. No system/endpoint master, grouping, registry or other application's
volume is written. Game-internal sound settings are unchanged.

For the packaged helper, run `build_helper.py` from an x64 MSVC developer shell
with a new `--build-dir` and `--output-dir tools/game-audio`. Supply `--cmake` and
`--ninja` if they are not on PATH. The builder runs only pure policy tests and
produces the EXE plus `audio-helper-provenance.json`; it never runs an audio mode.
Existing output is refused. The launcher and package preflight verify the receipt
and EXE hash. Both files and the Python guard are included by the portable stager.

2026-10-19 integration: root compiled the helper and passed 117 Python tests plus
the native policy test. The runtime-resolution development stage with this helper
passed isolated bundled-Python/import/PE dependency preflight. These are offline
and package checks, not actual manual 100% audio acceptance. Root receipts are in
ignored `analysis/runtime/manual-audio-root-tests.json` and
`runtime-default-001-audio-preflight.json`.

Build with the existing Windows SDK/MSVC toolchain (no dependencies to install):

```powershell
cmake -G Ninja -S tools/game-audio -B analysis/runtime/test-audio-guard-build -DCMAKE_BUILD_TYPE=Release
cmake --build analysis/runtime/test-audio-guard-build --config Release
```

Use a single-configuration generator so `GearsTestAudioGuard.exe` is directly in
that directory; run in the existing x64 developer environment. `AudioGuardPolicyTest`
is a fake-session CPU test. `GearsTestAudioGuard --enumerate` performs read-only
Core Audio enumeration and prints counts without session names or identifiers.

The launcher starts the guard after successful reservation and exact executable /
PID / creation-time verification. This minimum integration does **not** mute the
initial reservation/intro window. The guard independently authenticates game and
launcher process creation times using retained process handles. It enumerates all
active render endpoints every 250 ms for at most six hours, handling later audio
sessions. Cross-process and system-sound sessions are excluded. No matching session
is recorded as zero matches, never as confirmed muting.

The first mute state and exact session COM object are retained before mutation.
Only `SetMute` is called. `SetMasterVolume` is never called, so user volume changes
are preserved. Normal stop, cancellation and startup exceptions signal a unique
stop event before existing prime cleanup. Game exit, owner exit and the bounded
deadline independently trigger restoration in the guard. The original session
objects survive until restoration attempts finish, including after process exit;
no replacement PID/session is selected for restoration. Failed restorations retry
for three seconds and remain a reported failure. Python waits four seconds and
never kills an audio guardian on timeout; existing native/prime cleanup continues.

JSONL counts distinguish matched sessions, successfully muted sessions, restore
attempts, successful restores and pending restores. Windows persists session mute
settings, so a failed restore is not success and must be inspected before another
player launch. An OS/audio-service failure or forcibly killing the guardian cannot
be claimed to restore audio. Offline policy/build tests and read-only enumeration
do not establish actual-game muting, delayed game-session behavior or persistence
after game exit; those require the coordinator's separate runtime test.

API references: [session persistence](https://learn.microsoft.com/en-us/windows/win32/coreaudio/audio-sessions),
[GetProcessId and cross-process rejection](https://learn.microsoft.com/en-us/windows/win32/api/audiopolicy/nf-audiopolicy-iaudiosessioncontrol2-getprocessid).

Actual game check (2026-10-18): session `continuous-stereo-35320-20261018-212357-b5ff9b15` matched and muted one game audio session; stop restored that same session, with restore_pending=0, read_failed=0 and mute_failed=0. Receipt: `analysis/runtime/vr-launcher-session.1bfb91ffd2374e8389066345a9da13e0.audio.jsonl`. The background game and audio helper exited. This validates audio mute/restore only: the combined UI test did not acquire its layout lease, and its native cleanup verification failed, so it must not be advertised as a successful VR regression.
