# happyotaku playground

Enabled in the ordinary 0.1.0 development launcher. Select `happyotaku playground`
at the bottom of the game's main menu. The launcher loads the
version-gated DLL before entering gameplay. It adds a styled button to the active
`WarUIScene_MainMenu`, then travels locally to `MP_WarMachine` with `WarGameSP`.
No cooked game package is patched or distributed.

The range uses the map's existing Troika, eleven equipment pickups behind it,
and six stationary Locust targets along the firing lanes. Targets respawn after
three seconds; equipment is replenished after pickup. Infinite spare ammunition
preserves the original magazine and reload behavior. Only the requested range
world and its local player are maintained, including while occupying the Troika.

Build with the existing MSVC/CMake toolchain:

```powershell
cmake -S tools/playground -B tools/playground/build-playground2 -A x64
cmake --build tools/playground/build-playground2 --config Release
```

`playground_menu`, `playground_build`, `playground_dll`, and `playground_sha256`
bind the optional component in the launcher manifest. The normal native/XR
artifacts remain separately pinned. `stage_portable.py` includes only declared
runtime files and the DLL; analysis evidence and game assets remain excluded.

Engine work is scheduled on the observed local game thread. Indexed native
functions use an FFrame with explicit native parameters and out-parameter links;
unindexed functions use the original ProcessEvent. The loader checks executable
identity and live entry bytes, and independently verifies restoration on stop.
Status and logs are PID-specific beside the DLL. `PlaygroundTestEnter` is gated to
the explicit muted, behind-windows autonomous test environment.

Game-version offsets, layouts, and observed behavior are specific to the checked
Reloaded executable. See the development record in `docs/releases/0.1.0/PROGRESS.md`
for completed validation and remaining headset acceptance.

The weapon-6DoF branch also has a **private, opt-in Hammerburst R2 candidate**.
Build with `GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2=ON` and a distinct
`GEARS_PLAYGROUND_OUTPUT_NAME`; this exposes `PlaygroundVrFireEnable`,
`PlaygroundVrFireDisable`, and `PlaygroundVrFireStatus`. Enable it only after
the matching stereo and controller-input publishers are live. It consumes exact
XR frame keys on the local player's `PlayerTick` and forwards right-trigger
edges to the original `ButtonPress/Release('R2')`. Short bridge gaps keep the
last valid frame until its 100 ms limit; longer loss safely releases this
verified automatic weapon. Disable the adapter before `PlaygroundShutdown`.
The default build excludes these exports and does not forward VR fire.

`GEARS_PLAYGROUND_VR_R2_SIM_TEST=ON` additionally builds the isolated
`VrFireTestPublisher` and accepts simulated input only in the muted,
behind-windows test child. It is a verification tool, not a launcher option or
headset acceptance. Other weapons remain gated until their original
press/release, cancellation, and co-input behavior are verified.
