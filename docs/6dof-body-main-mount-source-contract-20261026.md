# Tick/C0 body main-mount source contract

Implementation draft,2026-10-26. Body owns the shared header and Native implementation; Weapon owns the PG equipment-witness implementation after header agreement. No default or runtime acceptance is implied. The old Rig/Hand/supplemental main-mount V1 interfaces remain unchanged, including their complete old-tracking freshness and world-grip checks.

## Reason for the separate source

EO used the same Native46 and original action with one PG parsing change. All19 held records had exact planned consumer/after Local and matching FK, but the original40 held/6s gate still failed. Saved ready-prefix PG query medians were EN6.5387ms and EO3.7711ms. Original Scene-to-publication remained about6.27ms and publication-to-C0 about30ms. EO controller sync preceded that Scene by median8.1922ms. These are different prefix observations, not a paired FPS benchmark.

The old tracking is an actual qualification dependency: Native Broker before/after checks, PG Client Frame freshness, bind's exact stereo/controllers and world-grip recomputation, and old-right-Grip usability. It cannot simply be called unused metadata and have its checks removed. Conversely, the current body target uses genuine new Tick raw tracking and current C0 neutral/CTW, while the returned mount is a fixed, certified model calibration. The new source makes those dependencies explicit rather than forging a new Scene or changing an old timestamp.

## Three separate inputs and authority

| Part | Actual origin | Required current checks |
| --- | --- | --- |
| Original Scene/device ancestry | Original cached Core projected without modifying any field; exact committed parent Hand Scene key | Native incarnation/cookie/lifecycle/action generation, world/controller/pawn/mesh/weapon/reference/model/units, immutable original cache association and source ticket. Original Scene observation and publication remain within50ms. Old tracking is historical data; its V1 validation can remain stale. |
| Tick tracking | One actual certified outer MeshTick acquisition of matching raw HMD/controllers/continuity | Original sync/receive/acquisition times within40ms, exact source/session/sequence/space/publisher/profile/input/continuity/reference, no regression, no second acquisition or reference mutation in C0. |
| Current C0 basis | Actual canonical Compose, actual capture-start time, current actor/CTW/reference/model and actor-local authored neutral | Capture within40ms; actual GT/world lease/caller, current actor/metadata/model geometry/scale/units/reference; exact immutable basis/source association before/after query and before each write. |

Ids and matrices are data. A request claiming a Tick, cache revision or Compose cannot authenticate itself. Native constructs this source only inside the existing certified owner and admits it through the exact live parent broker. Native alone owns pose conversion, target composition, fixed-profile reconstruction and the existing single CAS/Kernel writer.

The original Core/Rig/Hand payload is never patched with newer poses or clocks. A pure projection for the new historical-association field must be named and checked as such; it is not a successful V1 fresh read. No `now` substitution, source renewal, cache permission or producer-time relabeling is permitted. A failed new dependency cannot resample or silently fall back to older tracking.

## Shared data shape

Draft header: `tools/vr-common/body_main_mount_provider.h`, namespace `gears_body_main_mount_provider`, separate export `GearsVrAttachBodyMainMountProviderV1`. Its version1 is a new contract, not a version change to the existing weapon main-mount V1.

- `Source.original_scene` retains the complete original Rig POD and original timestamps. `scene_ticket` and `scene_cache_revision` refer to the actual Native cache/latch association.
- `Tracking` carries actual Tick/thread/cookie, acquisition interval and unmodified raw Stereo, controllers, continuity, publisher and Native reference revision.
- `Basis` carries canonical Compose, actual capture-start, certified `BodyGeometry`, current component-to-world and base view. PG returns no world pose.
- The admitted Broker assigns query id and snapshots actual Hand lifecycle/action generation. The three age ceilings are fixed at new tracking/capture40ms, original Scene50ms and fresh equipment50ms.
- `Response` echoes the exact source/basis key, query and Native parent stamps, and carries actual equipment observation times plus fixed mount data. The embedded mount's original FrameKey remains the Scene/device key, never Tick/Compose.
- Registration uses the same active Hand parent context/cookie/pinned image, existing callback counts, reentry exclusion, stop/revocation/action-barrier and drain. No independent registrar/pin/detach or equipment-call capability is created. Large scratch stays in persistent owned storage.

The Broker's actual C0 validator is fixed and Native-private, not a public POD permit. Under its mutex the Broker authenticates/snapshots parent and original Scene association, reserves query scratch and inflight lifetime; it releases that mutex before actual C0/source/basis validation and PG. It reacquires it to check exact parent/revocation/query state before publishing. Do not call a full context function that reenters Broker while holding this mutex. The source validator must not use the generic zero-pending-callback parent predicate to reject its own precisely reserved query, and must not globally subtract callback counts. Parent validation and Native source validation remain distinct; every later write still requires the normal fully idle parent.

Two distinctions are mandatory. The committed Hand Frame does not contain Rig publication/binding generation: only Native can authenticate those against its Core/cache. PG can independently obtain the original Hand key, five object identities and reference generation; echoing additional Native ids is correlation, not a claim that PG observed them. Likewise PG cannot observe Native's private action generation. It echoes that broker stamp; Native verifies generation/barrier before/after. PG independently checks its existing domain/shot/action depth and its own Client epoch, which it captures locally under its Gate rather than trusting a requested epoch.

`Tracking.reference_revision` is Native's actual `reference_snapshot` revision, the same source as Core.native_reference_revision. It is distinct from original_scene.reference_generation, which is the original StereoGate generation and is matched to the actual committed Client Frame. Never compare those two counters for equality. PG checks the Native revision's shape/echo only and exact stable NativeReference against the original Scene. Native verifies the actual current snapshot revision and complete NativeReference before/after and every write; the request cannot authenticate its own Native revision.

## Query-local PG equipment witness

Add one separate synchronous operation under the Client's existing Gate/epoch/parent lifetime. It extracts the exact registered committed key and identity fields as ancestry, without returning or manufacturing a Hand Frame and without using old tracking as the new tracking qualification.

The fixed read-only provider performs a fresh Game/held observation, both original typed asset reads, exact right attachment and the independent final Game/held observation with `same_game`, hold generation and held binding. All original object/registry/class/outer/property/attachment repetitions remain. The fresh observations enclose the metadata work; registration/domain/epoch/held generation are rechecked at the end. Equipment begin/end are actual clocks from this call and must remain within50ms. New right-Grip eligibility uses the new authenticated raw bundle, including usable/active/focus/continuity and40ms checks.

Take equipment begin before the first actual Game read and end after both assets/attachment/final Game and epoch/hold/domain checks. Require positive ordered nonfuture times and measure age from begin; end cannot renew earlier evidence. Native validates both outer and embedded mount query ids against the request, both headers/reserved fields, exact old Scene FrameKey/lifecycle/identities and geometry/calibration. SourceKey is deliberately an echo subset; exact complete-source/reference/basis checks remain necessary even when it matches.

Reuse the real metadata primitives, factoring only where the existing reader's Frame carrier is incidental. Do not call unchanged `copy_frame_on_gt`, `source_game_on_gt` or the outer `attached_frame` wrapper and pretend the old Frame is fresh. Do not recursively enter a Client Gate. PG returns query-bound equipment/asset/calibration metadata, no world pose, shot, ownership grant or arbitrary calibration. Native converts the new raw grip using its certified current basis and independently reconstructs the fixed Lancer mount.

## Native source and write path

The new mode needs its own typed source classification/admission, not a weakened old `same_source` predicate. Every hard identity, cache/ticket, reference, world/actor/metadata/model, lifecycle/action and structural/nested/stop check carried by the old route must have an explicit equivalent. Keep the old observer's historical age failure separate from the new source's qualification, with permanent independent hard-fault propagation and original node/stack before/after checks.

Each actual write still checks immutable new raw/capture40ms, original Scene-observation/publication50ms, query-local equipment evidence50ms, current parent/actor/metadata/model/reference and the original CAS/Kernel protocol. No successful guard is cached as permission; no second writer, alternate restore receipt, repeated notify or new packet within C0 is introduced. Retained consumption after a real commit stays a separate authenticated same-lease operation; it cannot generate a new write.

The actual pre-query parent identity/revision/action-generation snapshot must remain a dependency after any query result, including non-ready. A parent revoked during the query may not turn into a permitted freehand write in that C0. Keep this snapshot separate from accepted equipment evidence. It is stored after the synchronous query returns, so the Native source validator does not run a normal zero-callback predicate against its own reserved callback. Every later write checks the normal idle parent again.

An acquired nonzero Tick parent cookie is already a hard identity dependency before selection or any query. Every current/retained source check requires that exact registration and Native incarnation still exist; disappearance or replacement cannot take a pre-query freehand early return into an allowed write. This identity check does not require zero callbacks. A Tick acquired with cookie0 retains the independent freehand bootstrap path.

After the exact Kernel lease has actually been consumed, elapsed equipment age alone must not poison original control consumption. The retained operation still checks hard source continuity and normal idle parent identity/revision/action/barriers/debt; it does not renew equipment age or grant a new write. A new write still requires equipment age from begin within50ms. This distinction is covered by a targeted equipment49→51ms Host boundary, independently of fresh raw tracking.

## Minimum proof before one original game action

Use one concrete boundary case with old tracking>50ms, original Scene<50ms, genuine new Tick raw/capture<40ms and fresh equipment observations. V1 must still reject stale. The new typed route may pass only under its distinct source, through the same actual query, native context and each real write, with recorded original source ids/times/basis and response association. A reference/source/parent revocation between query and write must deny without substituting a packet; stop uses the existing callback drain. Host seams and partial-retirement evidence stay explicit.

After the shared header/ABI and this bounded path are checked, build a separately identified candidate and run the unchanged retail0.4 background action and40/6s gate. Keep prior EN/EO failures and all original restoration checks. Do not infer continuous held, aiming/fire, full cover, run-camera or headset completion from this contract or a successful Host boundary case.
