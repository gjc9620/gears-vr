# UI consumer isolation during internal publication

2026-10-17. Offline source and dedicated loader-process verification only. No game, simulator, headset, GUI, runtime switch or controller input was used.

`StereoPublishBypass` tells the D3D hooks to forward internal publisher/mirror work without updating game-frame observation. Native UI consumers must preserve that same contract if entered within such a scope. Previously a simple Execute could consume a pending payload receipt and clear parent TLS while its indexed-draw callbacks were bypassed; material Execute/upload had analogous state changes. The optional root observer could also consume sample slots or increment a parent row's counters.

Simple Execute, material Execute, material upload and the optional scene-root hook now forward directly before Scope admission, TLS replacement, receipt claims, completion bookkeeping or diagnostics. Execute callbacks are counted by the root observer only after the bypass check. Original arguments, call count, return bits and exceptions remain forwarded.

This fixes a conditional hook contract. The reviewed publisher/mirror paths call D3D Reset/barriers/copy/ExecuteCommandLists and, for mirroring, DrawInstanced. No explicit source edge from those paths into native engine UI Execute/upload/root rendering was established. Actual occurrence and any relation to the machine-gun black screen remain unproven.

## Dedicated actual-hook tests

The DLL loader accepts these modes; each runs in its own process:

- `--ui-bypass`: pending simple receipt, parent payload/index cursor/TLS, exact native/indexed forwarding, non-48 return, nesting, exceptions and observer-on non-bypass control.
- `--ui-material-bypass`: pending material entry, parent material/upload context, completion and synthetic stereo frame pair; calls the actual Execute/upload/nonindexed draw hooks; non-36 return, nesting, exceptions and observer-on control.
- `--ui-root-bypass`: parent root row, sample-slot/counter preservation, actual simple/material Execute calls, 64-bit return bits, nesting, exceptions and observer-on control.

Material/root modes return `ERROR_NOT_SUPPORTED` when their feature is not compiled, rather than reporting a pass. These modes are not added to the existing fixed startup suite because older supported DLLs do not export them; local default manifests and existing packages remain unchanged.

## Results

The private experimental build uses the prior VRDof61 feature options plus the optional root observer. Full DLL compilation, ABI check and all three hook modes passed. DLL SHA256: `ee95be1a68ec05a4c1aec7b15e2c59452f14b69a2aceaddf11b8b9510cc8566e`.

An independent private source copy removed the four new bypass guards. The same loader detected all three regressions, returning 311, 337 and 405 respectively. That negative DLL SHA256 is `24beec34821e288d33fd0832b4761e288c8f54bd85f3dc4e711ea79df84ae0ef`; it was never installed or substituted into the working tree/player package.

Frozen source hashes, build logs and positive/negative receipts are under ignored `analysis/runtime/ui-bypass2-*`. Reproduction scripts are `build-ui-bypass2.py`, `check-ui-bypass2.py`, `build-ui-bypass2-negative.py`, `check-ui-bypass2-negative.py` in that directory. Existing local tools and MinHook sources were reused; no tools were installed. Builds used two compiler workers. The game/headset status remains unverified.

This is an unattended offline regression result, not a solution claim for mounted-camera black screens, PathChoice placement, reticle spread rings or headset pacing. Those first-version issues remain open.
