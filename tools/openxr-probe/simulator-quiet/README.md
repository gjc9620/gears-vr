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

## Optional quiet2 swapchain handle fix

Apply `swapchain-handles.patch` after the focus and quiet patches. Its independent
source/build/DLL are recorded in `swapchain-handles-provenance.json`; this does not
change the default runtime or overwrite quiet1.

The old runtime assigned `g_swapchains.size()+2` and ignored failed insertion.
Actual game9j evidence showed menu2 (1920×764), replacement3 (1920×1080), destroy2,
then both requested 960×1080 eyes returning handle3. The fix allocates monotonically
without wrapping and reserves an empty map node before transferring resources.
All backends check publication; failed Vulkan/GL candidates retain cleanup ownership.

The coordinator ran the isolated real OpenXR+D3D12 test against both DLLs:

| Runtime | Exit | Result |
| --- | --- | --- |
| quiet1 | 1 | Left eye reused handle3; all three textures were the menu's 1920×1080 format87 resources, despite a 960×1080 format24 request. |
| quiet2 | 0 | Eyes4/5 were distinct 960×1080 format24 resources; recreated eyes6/7 validated 1280×720 and640×360. |

Saved results: `analysis/runtime/simulator-quiet2-evidence/runtime-old.txt`
(SHA256 `6e31b0416266130ce04e81c7ede7205d1ec0a3dbc6cdeb582ff432ba85af4878`)
and `runtime-new.txt`
(SHA256 `4916d95a55ac3ccdd46a19eb82265ec260d7095814d606f3ef940751b4a624ab`).
The test calls neither BeginSession nor EndFrame and requests no preview/window.
It verifies resources and handle ownership, not gameplay, submitted frames or a headset.
CPU tests additionally passed 8,031 checks; all four simulator CTest suites passed.

### Rebuild from source

Use an independent directory containing quiet1's `include`, `src`, `tests` and
`CMakeLists.txt`; omit its `bin`, build directories and `.git`. Apply this patch in
that copy. The following commands use the already installed x64 MSVC environment,
local CMake/Ninja and Vulkan headers, from the repository root:

```bat
E:\CMake\bin\cmake.exe -S analysis/simulator-quiet2-source -B analysis/simulator-quiet2-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_MAKE_PROGRAM=E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DSIMXR_VULKAN_INCLUDE_DIR=J:/gearsVr/analysis/deps/Vulkan-Headers-v1.3.290/include
E:\CMake\bin\cmake.exe --build analysis/simulator-quiet2-build
E:\CMake\bin\ctest.exe --test-dir analysis/simulator-quiet2-build --output-on-failure
```

The generated manifest is `analysis/simulator-quiet2-source/bin/openxr_simulator.json`.
The preserved local deployment is `analysis/runtime/simulator-quiet2/openxr_simulator.json`;
its DLL SHA256 is `760dccf583ef05d2c8dbb2d26f514e91f3a67e21142870afae70da651683432c`.
No dependencies are installed by these commands.

Build the independent integration test in an existing empty output directory:

```bat
cl /nologo /std:c++17 /EHsc /O2 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Itools/third-party/openxr-loader/include /Fe:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.exe /Fo:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.obj tools/openxr-probe/simulator-quiet/swapchain_runtime_test.cpp tools/third-party/openxr-loader/x64/lib/openxr_loader.lib d3d12.lib dxgi.lib
copy tools\third-party\openxr-loader\x64\bin\openxr_loader.dll analysis\simulator-quiet2-runtime-test\openxr_loader.dll
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet1/openxr_simulator.json
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet2/openxr_simulator.json
```

`--runtime` creates an isolated child with its own `XR_RUNTIME_JSON`; it does not
change the registry or parent/global environment. The child rejects any runtime
whose reported name is not exactly `OpenXR Simulator Runtime` before creating its
D3D12 session. Existing container concurrency requirements remain unchanged.
