# Automatic-test window placement

This opt-in policy keeps the game's own automatically shown windows behind
existing windows. It does not minimize, resize, move or activate windows, and
does not poll or repeatedly repair desktop Z order. A user's foreground game
window retains the existing behavior.

For an automatic test, set these variables in the test's child environment:

```text
GEARSVR_TEST_WINDOWS_BEHIND=1
GEARSVR_QUIET_STARTUP_BUILD=J:\gearsVr\tools\quiet-startup\build-test-behind1
```

The explicit directory must contain a successfully built `GearsEarlyLaunch.exe`
and `GearsQuietStartup.dll`, plus `test-background-policy.json`. The proof must
use schema 1, policy `hidden_bottom_showna_v1`, and a `sha256` object containing
the matching hashes of both artifacts. Missing or mismatched artifacts reject
launch before spawning. The proof is a local build receipt, not a signature
authenticating an untrusted build.

New visible windows are created hidden, placed at `HWND_BOTTOM` without
activation, then shown using `SW_SHOWNA`. A failed initial placement leaves the
window hidden. Subsequent show/position requests from the background game keep
the same opt-in placement policy; there is no desktop watchdog. The original
quiet11 default directory and hashes are unchanged. `--foreground` retains its
ordinary behavior.

`BackgroundPolicyTest` is a pure C++ test with injected callbacks; it creates no
window and calls no window-management APIs. CMake applies `/UNDEBUG` to this
target so Release assertions remain enabled. Run it with:

```text
ctest --test-dir tools/quiet-startup/build-test-behind1 -R "^BackgroundPolicyTest$" --output-on-failure
```

This policy has only offline/mock validation so far. Actual window occlusion
and continued game rendering have not been verified. The simulator preview
requires its own runtime-side policy; this game startup DLL does not control
windows owned by the OpenXR probe process.
