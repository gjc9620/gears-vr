# Test-only game audio guard

`GEARSVR_TEST_MUTE_GAME=1` opts the repository launcher into per-process test muting.
Set it only on the automated test's child environment. With the variable absent,
the launcher does not start this tool or change audio. No system, endpoint master,
session master volume, grouping, registry or other application volume is written.

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
