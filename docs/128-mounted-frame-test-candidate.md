# Mounted-frame retry test candidate

2026-10-17. Offline staging only; no game, simulator or launcher GUI was started and no foreground/runtime setting was changed.

The new independent developer package includes the outer mounted-camera frame-change retry from `d803216`, which the preceding MountedPrep ZIP did not contain. A changed frame is discarded and retried within the existing acquisition bounds; role/link/schema changes still reject. This is a candidate for the reported mount-black/dismount-recover case, not proof of its cause or resolution.

- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof61-MountedFrame-English-20261017.zip`
- SHA256: `5880480df02f13f94ae2d6f9267716dd42f2b87357b8f273ef792f95a43a364a`
- Size: 13,647,810 bytes; 701 archive members.
- Python and launcher payloads: source checkout `c39e090`; launcher defaults English, game selection is empty, personal settings are not imported.
- Native: unchanged VRDof61, frozen source `88cfae6`, SHA256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`.
- XR: unchanged Borderless1, SHA256 `2c27d29ca68a1629cb987addeedbc49ea59f2f7355408802a4b84ca7d96fe5eb`.

Staged and freshly extracted bundled Python isolated preflights both passed. Every archive payload digest and inventory digest matched. All preceding ZIP digests remained unchanged. Full receipt and packaging script are under ignored `analysis/runtime/vrdof61-mountedframe-english-20261017-package-receipt.json` and `analysis/runtime/package-mountedframe-candidate.py`.

The local default native manifest was not replaced. The new C++ shared-group placement helper is not in the frozen DLL, and PathChoice placement is not enabled. This candidate remains `runtime_verified=false`, `headset_verified=false`, `release_ready=false`.

## Next coordinated actual test

Use a fresh launch with this candidate's matching Python/native combination; first observe normal gameplay, then mount and dismount the problematic machine gun. Preserve the session outputs to distinguish registry change, timestamp retry, late acquisition, native rendering and XR submission failures. Extend to checkpoint/CG and headset pacing after mounted acquisition is understood. The user currently uses the computer; no test launch is authorized until they explicitly indicate readiness.

## Existing pacing evidence boundary

The saved session `continuous-stereo-24696-20261017-150718-1be1dbc1` retains only the first 4096 selected diagnostic observations. A bounded initial projection window contains 364 successful new-image observations, with median adjacent interval 33.3292 ms and p95 41.68408 ms. The observations occur after `xrEndFrame`; they are neither physical display timing nor whole-session coverage. They do not establish a VD network cause. Exact source digest and window are preserved in `analysis/runtime/user24696-render-cadence-audit.json`.
