# Native67 development candidate

2026-10-17. The repository launcher now selects Native67 after the ordinary-scene GPU result in docs139, the two checkpoint recoveries in docs140 and the pause/resume correction in docs141. Only `native_build`, `native_dll` and `native_sha256` changed in its manifest. Local player configuration and the Borderless1 XR executable are unchanged. This is a development selection, not V1 completion or Discord publication.

Native source is frozen at `f55f71313df32c96e588f658b7d4caf76eea2777`; DLL SHA256 is `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. The build retains the previous feature profile and explicitly enables guarded final-transfer normalization. Root-scene and call-boundary observers remain OFF. The frozen normalization and DOF CPU tests passed with assertions enabled.

The preceding Native60 manifest is preserved at ignored `analysis/runtime/default-native60-before67.json`; its DLL/build remain available. The selection receipt is `native67-default-selection.json` in that directory. Git also preserves the previous manifest.

## Portable artifact

`analysis/release-zips/GearsVR-V1-development-Vrdof67-TransferPause-English-20261017.zip`

- SHA256: `0a0b534152716fea98ccc7f1dc655f69fba886a6c21ef899b7b48333b685d067`
- 13,684,546 bytes; 702 archive members.
- Both new staging and fresh extraction passed isolated bundled-Python preflight; every payload digest and the inventory digest matched.
- English remains the first-run language, with Chinese selectable. Players select their own game EXE and OpenXR runtime. No game, simulator, reverse-engineering database, snapshot or pseudocode is bundled.
- Game resolution remains authoritative, DLSS/DLAA remain disabled, and physical background controller input stays disabled. Local camera/HUD preferences were not imported into the portable defaults.

Packaging did not launch the game or simulator. The underlying native candidate has simulator evidence; the freshly extracted portable artifact itself has not undergone gameplay or headset acceptance. Existing ZIP hashes were checked and preserved. The ignored full package receipt is `analysis/runtime/vrdof67-transferpause-english-20261017-package-receipt.json`.

Mounted-machine-gun output, PathChoice placement, recoil/spread-ring alignment, general UI coverage, arbitrary resolution transitions, headset pacing and extended play remain open. The artifact retains `headset_verified=false` and `release_ready=false`.
