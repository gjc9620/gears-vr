# Automatic-test window placement

User operating preference (2026-10-19): background game and simulator tests are
authorized at any time; deferring a physical-headset test does not defer these
tests. Keep audio muted and windows behind the user's applications. Do not
activate or cover their current window. For unattended resolution tests, use a
temporary 1280x720 windowed desktop setting, with exact pre-test settings/profile
backup and restoration after the owned game exits. Do not change the normal
player launch configuration or infer the VR eye dimensions from this window.

This opt-in policy keeps the game's own automatically shown windows behind
existing windows. It does not minimize, resize, move or activate windows, and
does not poll or repeatedly repair desktop Z order. A user's foreground game
window retains the existing behavior.

For an automatic test, set these variables in the test's child environment:

```text
GEARSVR_TEST_WINDOWS_BEHIND=1
GEARSVR_QUIET_STARTUP_BUILD=J:\gearsVr\tools\quiet-startup\build-test-behind2
```

The explicit directory must contain a successfully built `GearsEarlyLaunch.exe`
and `GearsQuietStartup.dll`, plus `test-background-policy.json`. The proof must
use schema 1, policy `hidden_bottom_showwindow_v2`, and a `sha256` object containing
the matching hashes of both artifacts. Missing or mismatched artifacts reject
launch before spawning. The proof is a local build receipt, not a signature
authenticating an untrusted build.

New visible windows are created hidden, then shown and placed at `HWND_BOTTOM`
in one `SetWindowPos` call using `SWP_SHOWWINDOW | SWP_NOACTIVATE`. No later
`ShowWindow` call follows this operation. A failed initial show leaves the
window hidden. Subsequent show/position requests from the background game keep
the same opt-in placement policy; there is no desktop watchdog. The original
quiet11 default directory and hashes are unchanged. `--foreground` retains its
ordinary behavior.

`BackgroundPolicyTest` is a pure C++ test with injected callbacks; it creates no
window and calls no window-management APIs. CMake applies `/UNDEBUG` to this
target so Release assertions remain enabled. Run it with:

```text
ctest --test-dir tools/quiet-startup/build-test-behind2 -R "^BackgroundPolicyTest$" --output-on-failure
```

The combined actual-run check below supplements the offline/mock validation;
it does not establish every startup instant or desktop layout. The simulator
preview requires its own runtime-side policy; this game startup DLL does not
control windows owned by the OpenXR probe process.

The matching simulator candidate is described by
`tools/openxr-probe/simulator-quiet/test-behind-atomic-provenance.json`. For local
automatic simulator tests, explicitly select
`J:\gearsVr\analysis\simulator-behind-atomic-source\bin\openxr_simulator.json` using the
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
`window_snapshot.py` supplies a bounded read-only Win32/DWM snapshot, with no
window title collection. It checks HWND process/thread identity and repeats the
Z-order walk; query failure or order changes invalidate the sample. Five further
mocked tests cover bounds, failed queries, identity/order races and the traversal
limit. The unexecuted private `analysis/runtime/dlss-native-menu5.py` driver now
uses both helpers at 250 ms intervals after owned identities are available,
requesting owned-test cancellation on occlusion or invalid observations. It does
not repair desktop Z order. Startup before identity discovery remains a gap.

These tests verify only the snapshot logic and rectangle analysis, not live
placement or the cause of the user's reported
occlusion. Live snapshots can race window changes and rectangles do not model
transparent pixels or irregular regions. No new game launch is justified by
these offline results alone.

## 2026-10-19: actual USER32 cause isolated without using the input desktop

`private_desktop_show_test.cpp` creates a private desktop, attaches only its own
thread, and destroys its own fixture windows before restoring that thread. It
never calls SwitchDesktop or addresses windows on the user's desktop. Failure
to create/attach the private desktop stops the fixture without a fallback.

On this machine, bottom-then-SW_SHOWNA raised the test window above the existing
fixture in all four cases (ordinary/topmost creation, ShowWindow/ShowWindowAsync).
A single SetWindowPos bottom+show operation kept it below in all four controls.
There were zero API/setup errors. This demonstrates the ordering defect in the
old API sequence; it does not prove all actual game/preview lifecycle paths.
The asynchronous case uses the same window thread and does not certify cross-
thread dispatch timing. The test stays opt-in, outside ordinary CTest execution.

The new game build passes both registered C++ tests; all 24 background-launch
Python tests pass, including rejection of the old v1 receipt. Ordinary launcher
behavior remains unchanged. The simulator's incremental atomic patch passes its
five registered tests and preserves the default show callback. No game or XR
session was started for this revision. Manual game play and system runtime
selection were not modified.

The v2 protocol uses ShowWindow's prior-visibility return semantics for intercepted
synchronous shows; asynchronous shows return the positioning API acceptance.
See Microsoft's [SetWindowPos reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
and [ShowWindow reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

### First actual v2 attempt: cancelled before VR activation

The `dlss-native-menu5` run reached reservation, then its new snapshot guard
cancelled before VR activation because the desktop contained 637 top-level
windows, exceeding the original 512 traversal limit. This was not a recorded
occlusion. The prime entry restoration was verified; the game subsequently
exited and all five settings/profile backups were restored. No DLSS pair or
actual combined game/preview layering acceptance was obtained.

Traversal now has a separate 4096-handle bound and a 256-visible-window bound.
Cycle, traversal overflow and visible overflow have distinct errors. Hidden,
minimized and cloaked windows do not consume the visible budget. Thirteen
snapshot/rectangle tests pass, including a 637-window/40-visible fixture. A
subsequent read-only desktop capture completed with 637 traversed and 24 visible
non-cloaked windows. These numbers are a sample, not a fixed desktop assumption.

### Actual v2 menu6 interval

The subsequent `dlss-native-menu6` run completed its bounded native graphics-menu
route and returned to the game UI. Its 527 cross-window snapshots reported no
occlusion or invalid samples; no actual game-foreground sample was recorded.
This supports v2 during the sampled interval only, not every startup instant or
all desktop layouts. Native cleanup, independent entry restoration, normal game
close and five settings/profile restores passed. No GPU projection was published
by this observer candidate, so this is not visual or DLSS acceptance.
