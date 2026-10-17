# VRDof62 runtime startup regression

The user authorizes unattended background game and simulator tests at any time, whether present or away. Keep game/simulator startup nonactivating and physical-controller background input disabled. Do not repeat the permission question. The simulator may remain visible. This authorization was persisted in local AGENTS.md.

## First actual attempt: PID17640

Used explicit `analysis/runtime/manifest-vrdof62.json`, the quiet2 simulator runtime, and `--auto-continue`, without `--foreground`. All 22 same-invocation startup self-tests passed. The state-based navigator dispatched five verified menu actions and a stable level camera was observed. No scene stereo activation was completed.

The native preparation failed with `IndexError: tuple index out of range` in `Resolver.name()`, called from UI owner-class discovery. The anchor sampler refreshes the live name count/header; the discovery pointer tuple remains the original snapshot. A name index can pass the live count bound while exceeding that tuple. This is a demonstrated preparation failure, not evidence that this explains mounted-weapon black output or every previous startup failure.

The owned prime was cancelled, entry restoration independently verified, the owned game normally closed, and all four saved profile files restored with hashes checked. The report correctly retains `native_started=false` and `native_cleanup_verified=false`, rather than claiming a successful VR lifecycle.

Evidence (ignored): `analysis/runtime/continuous-stereo-17640-20261017-180938-edade379.*`, `vrdof108-audit.json`, `vrdof108-normal-close.json`, `vrdof108-save-restored.json`; backup `analysis/savebackups/vrdof108-before-chapter/manifest.json`.

## Source correction

`Resolver.name()` now bounds snapshot reads using the tuple length. For a newly added name outside that snapshot, it validates a bounded current registry header and slot, the entry index and strict UTF-16, and verifies the header before caching. An observed header change raises typed `DiscoveryRegistryChanged`; UI discovery propagates it and preparation discards all certificates within its existing retry/deadline budget. Corrupt names, unexpected exceptions and target identities remain rejected. No native DLL, rendering route, deadline or retry count changed.

Validation: 14 resolver name/discovery tests, 31 UI tests and 29 startup anchor tests passed (74 total). The actual follow-up is recorded separately below; these CPU checks do not establish GPU/headset behavior.

## Follow-up: PID7124

After the Python correction, actual native preparation completed (certificate acquisition 2286.5424 ms; activation 517.9055 ms). No name indexing exception recurred. This supports the corrected preparation path, not a successful stereo display: XR received zero scene copies/submitted frames and the scheduled death-menu/checkpoint test timed out.

The final native report records 37358 accepted stereo tickets but zero publication/capture attempts. All 37368 final-transfer attempts were policy-rejected; source texture was 2560x1080 while presented buffers were 2534x1009, and the transfer arguments included a horizontal crop. The current strict final-transfer policy supports equal widths and at most one extra destination row, so this geometry cannot pass. This is a demonstrated reason for this run's no-publication; a subsequent controlled geometry test is required before assigning a broader root cause.

`frame_link_compare kind=first anchor=1 reason_mask=8` is an accepted initial reanchor, not a binding fault: binding rejected=0, changed=0 and anchors=1. The nonactivating tiny-output recovery completed before activation.

Native cleanup, entry restoration and XR object destruction were verified; the owned game was normally closed and all four profiles restored. Evidence: `continuous-stereo-7124-20261017-181337-3ae20545.*`, `vrdof109-{audit,live-status,normal-close,save-restored}.json` under ignored `analysis/runtime/`. The existing development ZIP predates the Python fix and has not been promoted.

## Geometry experiment blocked before activation: PID10756

A follow-up using the same candidate acquired certificates in 2239.7163 ms, then native startup rejected `crosshair observer requires compiled support and fresh assignment/tick bindings`. No geometry adjustment or GPU snapshot was performed. The existing crosshair safety check was not disabled. The generic native rejection still needs separation of compiled support, assignment and tick validation before attributing this intermittent startup failure. Prime cancellation/entry restoration, normal owned-game exit and four-profile restoration were verified. Evidence: `continuous-stereo-10756-20261017-182746-6ba84fa0.*`, `vrdof110-startup-{audit,normal-close,save-restored}.json`.

Next: isolate that rejection with existing binding evidence; complete the reversible, nonactivating window-geometry experiment without weakening transfer or UI admission checks. Mounted black output, fork UI placement, recoil ring alignment and new headset acceptance remain open.
