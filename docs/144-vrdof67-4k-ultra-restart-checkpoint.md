# Native67 4K Ultra restart and checkpoint regression

2026-10-17. Actual119 and Actual120 used the unchanged Native67 development candidate from [docs142](142-vrdof67-development-candidate.md): source `f55f71313df32c96e588f658b7d4caf76eea2777`, DLL SHA256 `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. The tested settings/restart/checkpoint sequence succeeded in the simulator. This does not accept mounted gameplay, general UI, headset pacing or extended play.

## Settings sequence and actual output

Actual119 (PID43328, session suffix `9cf5026c`) selected 3840×2160 through the game's Display menu, accepted the timed video confirmation, and obtained a 3840×2160 GPU image containing both scene halves. The image SHA256 is `c9e38c6814f0786f157334ef47bb04159c235bad24c34d49a593c2678a3b51d6`. This image preceded the Ultra selection and is not Ultra evidence.

The same run selected the game's **Ultra** graphics preset, accepted the video confirmation and acknowledged the separate restart-required notice. Persisted settings were preserved for a normal restart; the prelaunch INI SHA256 was `e56d4a4a2f35b9ba07bf401de75f52922f8976f545dbcae2b49afa94269c520d`. Its ClientSettings selected 3840×2160/windowed; DLSS remained False and the graphics UI reported anti-aliasing None / resolution scaling Native. Preset labels and persistence do not independently attest each shader's live quality setting.

Actual120 (PID20316, session `continuous-stereo-20316-20261017-203219-ed8800b7`) restarted from that INI. The captured final GPU output was **3840×2161**, with a 3840×2160 engine source and two 1920×2161 XR images. The cause of the final one-row difference is not established. This is a test of the selected 4K mode, not evidence that each eye rendered at 3840×2160.

The gameplay GPU image SHA256 is `d0b75ef3d0229f16226c09af63b200650de2ad41e406f8d0fdea537f9fcd48ab`. After checkpoint recovery it is `7b40626b3747ba652769707075c2f78b377080acf608b77f124f2705381a464c`. Coordinating visual inspection found complete scene content in both halves. These images establish their sampled output, not uninterrupted image continuity or projection accuracy.

## Restart, pause and checkpoint evidence

Actual120 reached a successful first XR projection approximately 24.47 seconds after runner start, including automatic menu/level loading. The interval from the guarded camera-ready observation to that submission was approximately 4.31 seconds. The initial VR delay therefore remains a performance/experience issue, even though this startup succeeded.

The read-only monitor sampled approximately 90.21 seconds, with native publication advancing 2645→5696 and no monitor errors. It included pause entry, so it is not 90 seconds of uninterrupted gameplay. A fresh successful `eye_menu_first_frame` receipt was recorded after pause entry. That common panel is not stereo gameplay.

A state-checked checkpoint confirmation then committed world generation **1→2**. Polling observed generation2 with 33 new epoch submissions approximately **6.65 seconds after confirmation**, followed by the GPU image above. This is an observed recovery bound after at least ten submissions, not first-display latency. The runner recorded a `world_handoff_card_ended` event with replacement projection. An earlier `AnchorRegistryChanged` rebind failure remains in the log; it did not prevent the later committed recovery and has not been erased from evidence.

Native120 final publication reached 8417. Final-transfer normalization recorded 8421 planned/linked/completed transfers, 97 preflight rejections, zero post-mutation failures and `faulted=0`. The rejected preflights are retained, not counted as successful transfers. DOF publication rejection was zero; `d80a7b` produced 2698 valid calls/proofs per eye. Actual119 additionally exercised both certified sites (`d808d8`: 41909 per eye; `d80a7b`: 11506 per eye), with zero DOF publication rejections. This expands caller coverage; it does not identify a mounted-machine-gun path.

Projection-only shutdown counters omit paused eye-menu composition. Their approximately 64.24-second maximum projection gap must not be presented as 64 seconds of black output. Physical tracking and headset display remain unverified in these runs.

## Input scope and restoration

All navigation used fresh game menu state and messages posted only to the identity-checked game HWND. No foreground activation, global controller input or game-memory writes were used; the checkpoint operation's foreground HWND remained 789028 before and after. Physical controller background response remained disabled.

Native cleanup, entry restoration, orderly XR shutdown and normal owned-game closure were verified for both runs. Four protected save/profile files were restored after each run. After Actual120, the complete original settings INI was also restored and verified against SHA256 `449746ec00c9df271aca1d7ac6947b660a1bd98861e70ba35fb92653e7b626c9`; after-test settings copies were retained. All owned processes exited.

Ignored evidence is summarized by `analysis/runtime/vrdof120-result.json`, with the `vrdof119-*` and `vrdof120-*` receipts and original session logs retained alongside it. No implementation/default/package changes were made by these two tests. `headset_verified=false` and `release_ready=false` remain.

The user's current persistent authorization permits background game/simulator tests at any time, whether present or away, without repeated permission requests. Tests must still avoid stealing focus and keep physical background controller response off. This supersedes earlier requests to wait before starting background tests.
