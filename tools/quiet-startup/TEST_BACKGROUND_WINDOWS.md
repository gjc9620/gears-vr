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

The combined actual-run check below supplements the offline/mock validation;
it does not establish every startup instant or desktop layout. The simulator
preview requires its own runtime-side policy; this game startup DLL does not
control windows owned by the OpenXR probe process.

The matching simulator candidate is described by
`tools/openxr-probe/simulator-quiet/test-behind-provenance.json`. For local
automatic simulator tests, explicitly select
`J:\gearsVr\analysis\runtime\simulator-behind\openxr_simulator.json` using the
launcher's runtime argument (or `XR_RUNTIME_JSON` in the probe child environment),
alongside the two variables above. Do not change the system ActiveRuntime.
The preview is owned by the OpenXR probe process, not a separate simulator.exe.
Both policies must be selected: using the old quiet2 runtime would leave preview
placement unchanged even with the new game startup DLL. The candidate suppresses
showing if initial placement fails; it never uses a periodic desktop watchdog.

Keep `GEARSVR_TEST_MUTE_GAME=1` and
`GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY=1` on autonomous test children. Do not set
these variables globally or on normal player launches. Do not fall back to the
old startup/runtime pair if the test candidates or their hashes are unavailable.

## Actual combined check (2026-10-18)

`analysis/runtime/path-fit-runtime9-20261018.json` used both candidates and the
explicit test flags. The game and OpenXR probe ran, and XR reported projection
submissions. A read-only Z-order walk sampled 785 times after owned identities
were available; no visible owned game/probe window appeared above the current
foreground window. This is sampled evidence, not proof of every startup instant
or every multi-monitor layout. The check made no focus, move or resize requests.

The longer UI hold check stopped on an unverified native-status read, so the
whole VR regression did not pass. Native cleanup, independent entry restoration,
normal game close and five settings/profile restores did pass. This outcome
supports the new placement policy only within the sampled interval; the status
read failure remains separate work. The system runtime registry was not changed.

## Coverage limitation and expanded offline detector

The foreground-only walk above can miss a test window covering another visible
application on a second monitor or below the foreground window. Its zero count
must not be described as proof that no user windows were covered.

`window_occlusion.py` analyzes a supplied top-to-bottom snapshot across monitors,
subtracting rectangles already covered by higher windows. It reports test-window
areas above other visible applications, including negative screen coordinates.
Hidden, minimized and cloaked windows are excluded. It neither reads window
titles nor moves, activates or reorders anything. Explicit shell HWND exclusions
belong to the snapshot caller; do not exclude ordinary applications by guessing.

Five offline regression tests cover the foreground-only blind spot, partial and
complete coverage, invisible windows, and nonoverlap. Run with Python unittest
discovery in this directory using pattern `test_window_occlusion.py`.
The helper is not yet wired into the runtime driver. These tests verify only the
rectangle analysis, not live placement or the cause of the user's reported
occlusion. Live snapshots can race window changes and rectangles do not model
transparent pixels or irregular regions. No new game launch is justified by
these offline results alone.
