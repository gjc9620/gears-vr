# Experimental background startup

This is an opt-in Windows x64 startup policy. It has passed the small native
fixture, but has not yet passed actual-game startup or manual activation tests.
It does not replace the current player default.

`GearsEarlyLaunch.exe EXE GearsQuietStartup.dll [arguments]` creates a new owned
child under a temporary debugger, restores its one-byte entry breakpoint, holds
the initial thread, and detaches before loading the policy. Only that new child
is targeted. A failed bootstrap terminates its own unreleased child; it never
attaches to an existing game, retries ordinary startup, or modifies disk code.
Remote-thread timeouts do not free live arguments or terminate individual threads.
Before release, the launcher queries the installed policy and checks the restored
entry. Visible windows before the entry point are rejected as too late.

The policy covers process-local ShowWindow, SetForegroundWindow, SetActiveWindow,
SetFocus, SetWindowPos, BringWindowToTop and CreateWindowExA/W. When the game is
already the foreground process, calls are passed through. Background top-level
windows are created hidden and shown without activation; caller extended styles
are preserved. Hiding uses SetWindowPos with NOACTIVATE; minimizing uses
SHOWMINNOACTIVE. No global focus lock, synthetic keyboard input or focus-restoring
loop is installed. Policy code remains loaded until process exit.

For an explicit repository test, set `GEARSVR_QUIET_STARTUP_BUILD` only in the
launcher's child environment to the frozen build directory. `background_launch.py`
also accepts `--quiet-build`; it rejects combination with `--foreground`, validates
the game installation and returned PID/creation time/path, then arms the existing
background-controller helper against the game PID. The bootstrap PID is distinct.
Unset the option to use the existing initial-show hint. No global registry/runtime
setting is changed.

Build with the existing MSVC x64 toolchain and CMake; MinHook is the existing
`tools/third-party/minhook` dependency. Use a new build directory after changes.
The native fixture runs offscreen and exits, while observing creation, delayed
activation, parent/child visibility, owned windows, hide/minimize/restore and
unchanged extended styles. `check_fixture.py` separately checks foreground events,
missing policy rejection and actual failed-child exit. It never manipulates the
user's keyboard or mouse. Passing it does not prove game compatibility or manual
click/Alt-Tab behavior.

Known limits: TLS/DLL initialization occurs before the EXE entry; this boundary
does not guarantee that no earlier focus request occurred. A whole-launch event
monitor is still required. Creating initially hidden changes visibility during
creation callbacks. A background automatic maximize request uses a nonactivating
normal show; manual foreground maximize is passed through. The current default
is windowed. No multiplayer or anti-cheat compatibility is claimed.

Windows reference: [debug events](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent),
[remote thread lifecycle](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread),
[show commands](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow).

Development evidence is under ignored `analysis/runtime/quiet-startup*-*.json`.
No proprietary snapshots, analysis databases or pseudocode belong in this folder
or in a release package.
