# VRDof62 consolidated offline candidate

2026-10-17. This preserves a normal-profile development candidate for the next actual regression. No game, simulator, GUI, focus, controller or XR Runtime action was performed.

## Contents and configuration

Native source is frozen at `bdb23b2`; the supported VRDof61 feature options are retained. The root-scene observer and inline call-boundary observer are OFF. The candidate includes the UI publication-bypass fixes, current Python mounted-camera acquisition changes, English launcher default and optional owner/layout tools. PathChoice placement is not enabled. Game resolution remains authoritative; DLSS/DLAA remain disabled and 6DoF is retained.

- DLL: `tools/native-capture/build-vrdof62/GearsCaptureVrdof62.dll`
- DLL SHA256: `ab3a2cf54c82b72de4080c69cda13bcd1405081311a366fa414bc2289a35c438`
- Explicit development manifest: ignored `analysis/runtime/manifest-vrdof62.json`
- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof62-ReplayGuard-English-20261017.zip`
- ZIP SHA256: `b6194f104692736d976cfcb8e80a09a8da1ad8fdc112cf9a6c5d1fe6c3734530`
- ZIP size: 13,667,906 bytes, 702 members.

Local default manifest/config and every existing ZIP were digest-checked and remain unchanged. Portable defaults are English, 100 game units/metre and an empty game path requiring selection. Personal settings are not imported. Native/XR artifacts and every payload digest were checked after fresh extraction. No game, simulator, knowledge database, pseudocode or snapshots are bundled.

## Verification scope

The complete candidate DLL and all ten required native test executables were rebuilt, together with five focused UI CPU tests. The offline runner completed 18 checks with expected results: 17 successful ABI/hook/CPU test cases and an explicit `ERROR_NOT_SUPPORTED=50` assertion for the disabled root-observer check. That last case proves feature exclusion, not a passed root runtime test. No actual-game GPU/frame-link self-test was invoked.

Staged and freshly extracted bundled-Python isolated preflights both passed. Full source/vendor hashes, build log, per-case outcomes, package inventory and protected previous/default hashes are under ignored `analysis/runtime/vrdof62-*`. The standard same-invocation startup suite still runs on a future actual launch; this offline result does not replace it.

`runtime_verified=false`, `headset_verified=false`, `release_ready=false`. Mounted-camera black screens, reticle recoil/visible center, PathChoice placement and sustained headset pacing still require actual evidence.

## Reticle decision

The existing evidence in docs 83, 93, 96 and 98 was reused before deciding whether another offset change was justified. Cross-payload grouping remains a static coverage gap, but historical ring/icon pairs already had identical actual viewport/scissor maps, and an earlier first-quad provenance hole was independently repaired. Those records do not cover the latest user recoil observation. No new guessed reticle offset or broader kind-1 merge is included. Any future shared group must use the authenticated inline child/body boundary, complete member ranges, compatible transforms and actual ticket/target association; a PostRender parent token alone is insufficient.
