# Bounded final-transfer entry-to-draw observation

## VRDof64 identity and behavior

Source revision `adcbffb` adds an observation-only scope to the existing quad hook, plus cheap counters in existing Draw/DrawIndexed hooks. No hook target, source sampling argument, admission condition, transfer mapping or publication rule changed. Preflight TLS/COM work is capped at 64 invocations; retained rows have separate quotas of four incomplete and twelve complete entries. The diagnostic runs only with the existing final-transfer, outage and recording gates. After that cap, no further diagnostic preflight resource queries occur.

Rows preserve nested TLS ownership and report normal/exceptional scope exit. Existing Draw-hook calls are counted separately from calls reaching transfer observation, including bypass calls separately. “Exactly one routed draw” concerns those existing direct Draw hooks, not all possible GPU work. Complete entry means the bounded snapshot fields and local pair are available; it does not grant a rendering certificate.

Focused MSVC CPU tests covered comparison differences, attempt/row quotas, missing routing, duplicate draws, mismatched tuples and bypass. Existing transfer policy tests passed. Frozen normal-profile DLL `tools/native-capture/build-vrdof64/GearsCaptureVrdof64.dll` SHA256 `320f885a42decbe95238d9e49b1a81e09b85a71244e1b407a11e726354f0fc53`; source/options/vendor receipt is ignored `analysis/runtime/vrdof64-build-source.json`. Root default and release packages were not promoted.

## Actual PID37760 result

Session `continuous-stereo-37760-20261017-190838-46932807`, using quiet2 simulator and background auto-continue. The 22 same-invocation startup tests passed and native activation completed. The guarded monitor sampled for 30 seconds; publication remained zero, so no GPU snapshot was requested and display acceptance failed.

The collector attempted exactly 64 preflights, retained 14 rows (two incomplete, twelve complete) and stopped extra preflight reads. All twelve complete rows had one routed indexed draw (six indices, one instance), normal scope exit, no bypass, and difference mask **12 = target + rectangles**. The command list/canonical generation, native context, source object/resource/descriptor, source pair/ticket, world and inventory matched entry to actual Draw.

Entry D3D tracked target was still the 2560x1080 source texture, while actual Draw targeted a known 2534x1009 backbuffer. Viewport changed accordingly inside the original quad call. Therefore an entry check based on already-bound D3D OM/viewport state is insufficient for a normalization decision: the engine commits pending state inside that function. This positive CPU observation is not GPU pixel or headset proof.

Native cleanup, entry restoration, XR object destruction, ordinary owned-game exit and four-profile restore were verified. Raw evidence remains ignored: session files, `vrdof113-{audit,bracket-summary,normal-close,save-restored}.json`.

## Next bounded native question

Before changing final sampling, identify the native **pending** destination/viewport and compare it with the actual Draw target. Existing cached historical artifact1 contains quad RVA1af42c and its calls to 1ac950 and 177e90 before DrawIndexed return address1af92b. The knowledge base was searched before reading these saved functions. These are historical runtime derivatives (source blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, base0x140000000), not current byte-attestation merely because the RVA matches.

Do not normalize based on the previous tracked source RTV, force a standard resolution as the release fix, weaken the full-eye mapping checks, or assume a transient intermediate is safe to copy. Pending-state runtime evidence, full code/ABI attestation, representative source/destination pixels, UI coverage and headset acceptance remain separate requirements.
