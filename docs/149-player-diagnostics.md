# Player diagnostics for V1

The bilingual launcher includes **Export logs / 导出日志** beside Open logs. It opens a ZIP save dialog and runs the exporter independently of the VR owner. Export is available during a session, after a normal exit and after an incomplete startup. It does not launch, stop, focus or attach to the game and does not upload anything.

Ask players to save the ZIP soon after an issue and attach it with a description, chapter/checkpoint, headset and connection method. The archive contains a short issue template. A screenshot or headset observation remains useful when the issue is visual; telemetry cannot establish what the player actually saw.

The exporter selects recent owned session metadata, lifecycle/startup/world/menu transitions, bounded recent numeric telemetry, artifact fingerprints, graphics INI settings and selected Windows hardware inventory. It excludes game binaries, native instruction dumps, saves/profiles, screenshots, analysis databases and pseudocode. It filters personal paths, account identifiers and email addresses. Unsafe links, corrupt/partial records and unavailable sources produce omissions/warnings; the remaining report can still be exported.

Native diagnostic timestamps use QPC. Existing bound, top-level DXGI Present observations exclude TEST calls; their rate is not engine-update or physical display FPS. Stereo-pair publication counts new native image publications. XR loop frequency uses sampled frame counts and the runtime's predicted display timestamps; a 90 Hz loop can repeatedly submit a scene produced at a lower rate. Final successful projection averages/gaps are reported separately only when final session evidence exists. First, unchanged, reset or unavailable samples have null rates; measured unchanged counters over an advancing clock can establish zero.

The session recorder samples at most once per second, records state changes, and rotates two backups with a 2 MiB bound per file. It reuses existing counters and introduces no rendering hooks. Sampling write failures disable diagnostics rather than stopping VR. Exports are bounded and atomically written to the selected destination; replacing an existing ZIP requires the save-dialog confirmation or explicit CLI `--overwrite`.

CLI equivalent:

```powershell
python tools/vr-launcher/launcher.py export-logs --output C:\Feedback\GearsVR-Diagnostics.zip
```

Validation receipts, actual-game export samples and portable package verification are kept under ignored `analysis/runtime/` and `analysis/release-zips/`. Simulator tests do not establish physical headset acceptance or readiness of the other V1 features.
