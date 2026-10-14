# Simulator automation focus policy

`focus.patch` applies to the separate `OpenXR-Simulator-Source` repository at
commit `8de34575bfb823254297663aaf4997bc27da0ef8`. It includes only the runtime
focus change, its new header/test, and CMake registration. It excludes unrelated
local changes. `provenance.json` records source, dependency, and DLL hashes.

Set `XR_SIMULATOR_FORCE_FOCUS=1` in the simulator host's launch environment before
loading the DLL. The value is read once; unset, `0`, and other values preserve
normal OS focus behavior. This is an explicit simulator automation policy.
Actual OS activation logs and mouse release remain intact. Only transitions
between VISIBLE and FOCUSED change; activation cannot restart an ended session.

From the workspace root, apply or reverse the patch:

```powershell
git -C OpenXR-Simulator-Source apply ../tools/openxr-probe/simulator-focus/focus.patch
git -C OpenXR-Simulator-Source apply --reverse ../tools/openxr-probe/simulator-focus/focus.patch
```

Build-only reproduction (run in an x64 Visual Studio developer environment with
CMake and Ninja on PATH):

```powershell
git clone --depth 1 --branch v1.3.290 https://github.com/KhronosGroup/Vulkan-Headers.git analysis/deps/Vulkan-Headers-v1.3.290
git -C analysis/deps/Vulkan-Headers-v1.3.290 rev-parse HEAD
# Expected: b379292b2ab6df5771ba9870d53cf8b2c9295daf
cmake -S OpenXR-Simulator-Source -B analysis/simulator-focus-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMXR_VULKAN_INCLUDE_DIR="$PWD/analysis/deps/Vulkan-Headers-v1.3.290/include"
cmake --build analysis/simulator-focus-build --target openxr_simulator focus_policy_tests projection_timing_tests ui_resolution_tests
ctest --test-dir analysis/simulator-focus-build --output-on-failure
```

CMake outputs the new DLL and manifest to `OpenXR-Simulator-Source/bin`.
The installation script is not used. This does not overwrite the deployed
`OpenXR-Simulator-v1.5.0-win64` DLL, register a runtime, or launch an XR process.

Validation: simulator DLL compiled; all three CTest tests passed. The focus test
performs 82 checks with assertions active in Release, including all session
states, default behavior, explicit opt-in, and duplicate inactive notifications.
This establishes CPU policy behavior; focus events and game rendering still
require runtime validation.
