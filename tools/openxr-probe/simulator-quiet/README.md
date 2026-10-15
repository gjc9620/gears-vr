# Visible simulator preview without automatic activation

The user wants a continuously visible preview on a second monitor, without taking
desktop focus. This option does not minimize the window or suppress manual clicks.
Normal continuous VR sessions have no preview timeout; stopping the XR host still
closes its preview. No window positioning loop is installed.

Apply `quiet.patch` after `../simulator-focus/focus.patch` to the source revision
recorded in `provenance.json`. Set `XR_SIMULATOR_NO_ACTIVATE=1` in the child process.
Unset values, `0`, and any other value retain the simulator's original activation
behavior. This option is independent of `XR_SIMULATOR_FORCE_FOCUS`.

Automatic window creation, persistent-window adoption and aspect fitting use
nonactivating flags. Explicit foreground activation at startup is skipped. Manual
clicks and keyboard commands retain their normal behavior.

The local compiled copy is `analysis/runtime/simulator-quiet1/openxr_simulator.json`.
The original installed runtime and earlier focus-only copy were not overwritten.
The continuous runner and pose diagnostic default the option only in their child
environment; the GUI and SimulatorVR entry points select the quiet local runtime.
No registry change or global focus Hook is involved.

Build: private source copy, existing local Vulkan headers, Ninja Release. The three
existing simulator CTest suites passed. Forward/reverse patch checks passed.
An isolated 5-second XR test submitted 445 test-image layers; 532 foreground polls
and a WinEvent foreground listener observed no activation by the owned XR process.
The foreground HWND was unchanged from start to finish. This verifies that one
preview startup; it does not prove game startup or every Windows activation path.
Evidence is in ignored `analysis/runtime/simulator-quiet1-check.*`.
