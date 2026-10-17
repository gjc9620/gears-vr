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
