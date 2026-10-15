# Experimental background startup

This is an opt-in Windows x64 startup policy. It has passed the small native
fixture, but has not yet passed actual-game startup or manual activation tests.
It does not replace the current player default.

The first actual-game observation (9j) recorded two foreground events even though
all eight API hooks remained installed. The user subsequently confirmed manually
switching to the game, so these events do not establish automatic focus stealing.
VR separately stopped because the simulator reused an existing
swapchain handle after a menu resize. See `docs/68-window-recovery-and-quiet-startup.md`;
neither issue is covered by the fixture's passing result.

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

The command builder leaves simple arguments such as `-NoPauseUnfocus` unquoted.
The game's raw command parser accepts only end-of-string, space or tab after a
switch, so surrounding that switch with quotes disables it even though Windows
argv parsing succeeds. Arguments that need quotes still use Windows backslash
escaping. `QuietCommandLineTest` checks the exact raw switch and argv round trips;
the actual game must additionally verify its internal command line and behavior.

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

For bounded diagnosis only, `GEARSVR_QUIET_TRACE=1` records the first 128 API calls
in process memory, including suppressed and passed calls, NULL targets, arguments,
foreground windows/timestamps and raw return addresses. It does not wrap or log
to disk. `GearsQuietTrace(void*)` copies completed records into a caller-owned
`TraceSnapshot` (37920 bytes); its independent ABI does not change `Status`.
Busy slots are skipped and overflow is explicit. Treat the addresses as private
runtime evidence, not release content.

Tracing additionally observes five activation-related messages through
DefWindowProcW/A, forwarding them unchanged. These two extra hooks are absent
when tracing is disabled. Their records describe message delivery, which can
occur after activation; they do not by themselves identify the requesting API
or process. Actual game-only tests still reproduce foreground activation while
all eight policy hooks remain installed. The quiet policy remains experimental.
