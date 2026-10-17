# Scoped background keyboard diagnostic

2026-10-17. Actual118 used the selected Native67 development build from [docs142](142-vrdof67-development-candidate.md). The ordinary-scene startup/output monitor passed: publication advanced from 0 to 3,342 during approximately 90 seconds. This was not a mounted-machine-gun test or headset acceptance.

The fresh process (PID 35620) had 151 reflected `PlayerInput.Bindings` entries, with a 28-byte stride. W mapped to `Game_MoveForward`; A/D/S mapped to strafe left/right and move backward. Left/Right contained only content-reader commands. No physical keyboard turn binding was found. Saved package defaults support `Game_TurnLeft`/`Game_TurnRight` aliases, but those defaults do not establish a physical binding in this live profile.

One W keydown/keyup pair was posted to the target game HWND for 0.3 seconds; keyup was recorded. Foreground HWND remained 789028. Physical background controller input remained disabled (`background_input` status `original`). The diagnostic used no ViGEm, global input, focus action or game-memory writes.

Pawn location was unchanged: baseline drift and distance after W were both 0.0. Thus gameplay movement was not demonstrated. This result does not distinguish undelivered input, blocked movement or focus gating. Scoped keyboard input is not currently an established autonomous navigation solution, and this run did not reach a Troika.

Normal game close, native cleanup and entry restoration were verified. All session processes exited and all four protected save/profile files were restored. The current user authorization permits unattended background game/simulator tests at any time; older no-launch preferences do not supersede it. Focus stealing remains forbidden and physical background gamepad input remains disabled.

Evidence under ignored `analysis/runtime/`:

- `vrdof118-window-keyboard.json`: live bindings, scoped key event and pawn samples.
- `vrdof118-audit.json`: startup/output monitoring (`passed=true`).
- `vrdof118-normal-close.json` and `vrdof118-save-restored.json`: cleanup and restoration.
- `continuous-stereo-35620-20261017-200011-e4eb5180.json`: session record.
