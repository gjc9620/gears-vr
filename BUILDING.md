# Building GearsVR 0.0.2

Players should download the ready-to-play ZIP from Releases. This guide is for source builds.

The native source matches the AdsAnchor2 release freeze. The OpenXR source matches
MountedLatency1. Both native/XR components retain the published 0.0.1 binaries.
Runtime Python modules and the launcher include the 0.0.2 startup hotfixes.
The launcher includes local game discovery, the supplied artwork, and System OpenXR as its default.

## Tools and dependencies

- Windows x64, an MSVC developer command prompt, CMake 3.21+ and Ninja.
- .NET Framework's C# compiler for the WinForms launcher; Windows PowerShell.
- MinHook 1.3.3 source at `tools/third-party/minhook`.
  Upstream: https://github.com/TsudaKageyu/minhook
- OpenXR headers and x64 loader 1.1.63 at `tools/third-party/openxr-loader`:
  `include/openxr`, `x64/lib/openxr_loader.lib`, `x64/bin/openxr_loader.dll`.
  Upstream: https://github.com/KhronosGroup/OpenXR-SDK
- Python 3.12 and pefile 2024.8.26 for the launcher tooling. Place pefile and its
  supporting modules at `tools/package-inspector/python-libs`, as required by the tooling.

Third-party dependency downloads, game files and local research data are not in Git.

## Compile

Run from an x64 MSVC developer PowerShell at the repository root:

```powershell
cmake -S tools/native-capture --preset gears-vr-0.0.1
cmake --build tools/native-capture/build-ads-anchor2
ctest --test-dir tools/native-capture/build-ads-anchor2 --output-on-failure
cmake -S tools/openxr-probe --preset gears-vr-0.0.1
cmake --build tools/openxr-probe/build-mounted-latency1
ctest --test-dir tools/openxr-probe/build-mounted-latency1 --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/vr-launcher/ui/build.ps1
```

The launcher source references embedded artwork built by `build-icon.ps1`.
The native/XR presets preserve the release feature switches. Paths are repository-relative.
The complete portable package also uses the quiet-startup and game-audio helpers;
their build tooling is in the respective folders. The portable stager verifies the
helper provenance, expected DLL/EXE hashes, bundled Python and required startup tests.

Locally rebuilt binaries can have different hashes. Review and update the matching
manifest/provenance values after validating a build; do not disable version checks.
The distributed ZIP is the verified 0.0.2 artifact. Native/XR build presets keep their
0.0.1 names because those components are unchanged in this hotfix.

This source export excludes local research reports, runtime snapshots, decompilation
output, private databases, local settings and the development Git history.
