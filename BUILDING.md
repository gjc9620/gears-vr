# Building GearsVR 0.0.3

Players should use the complete ZIP from Releases. This repository contains authored source for the frozen native renderer, XR application, Playground bridge and launcher/runtime scripts. Local research data and private development history are excluded.

## Dependencies

- Windows x64, MSVC, CMake 3.21+ and Ninja; .NET Framework C# compiler and Windows PowerShell for the launcher.
- MinHook 1.3.3 in tools/third-party/minhook: https://github.com/TsudaKageyu/minhook
- OpenXR headers and x64 loader 1.1.63 in tools/third-party/openxr-loader (include/openxr, x64/lib, x64/bin): https://github.com/KhronosGroup/OpenXR-SDK
- Python 3.12 and pefile 2024.8.26. Tooling expects pefile and its supporting modules in tools/package-inspector/python-libs.

Dependencies are not vendored in this source repository. The downloadable ZIP includes its Python runtime and dependency notices.

## Build

From an x64 MSVC developer PowerShell at the repository root:

```powershell
cmake -S tools/native-capture --preset gears-vr-0.0.3
cmake --build tools/native-capture/build-avp90-two-slot-ready1
ctest --test-dir tools/native-capture/build-avp90-two-slot-ready1 --output-on-failure
cmake -S tools/openxr-probe --preset gears-vr-0.0.3
cmake --build tools/openxr-probe/build-runtime-colors1
ctest --test-dir tools/openxr-probe/build-runtime-colors1 --output-on-failure
cmake -S tools/playground -B tools/playground/build-playground6
cmake --build tools/playground/build-playground6 --config Release
powershell -NoProfile -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1
```

The presets retain the feature switches of the frozen release builds. The launcher embeds the supplied icon/artwork. The complete portable package also requires the quiet-startup and game-audio helpers and their verified provenance.

Source builds may produce different hashes. Review and validate rebuilt artifacts before updating manifest/provenance values; do not disable checks. The published ZIP is the verified artifact. Offline tests do not establish complete game, GPU or headset acceptance.

For Python unit tests, supply the declared pefile dependency and create the ignored analysis/runtime directory used by test fixtures. The release test record identifies the exact checks executed.
