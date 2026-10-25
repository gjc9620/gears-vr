# 6DoF held main-hand mount query — Native/PG contract

Status: implementation contract for the next private candidate. Nothing in this
document enables a default launcher or changes Rig V1, Hand V1, aiming, shooting,
or the original wrist → weapon bone animation increments. Native owns the shared
C0 consumer and broker; the weapon chat owns the PG callback.

## Purpose and current evidence

Native39/B17 accepts a fixed dual-wrist point through the original Compose return.
B18 accepts the bounded fixed-head hand trajectory. B19 accepts eligibility
fallthrough to the original controllers and reachable-goal reacquisition; its
asynchronous intermediate Limb read remains explicitly separate from a final
handoff return fence. These are development checks, not full cover/headset
acceptance. The next target is consuming the actual held Lancer calibration in
that same C0 transaction, with free-hand calibration on an ordinary unavailable
query.

The existing PG-local `nativeequipment_main_mount::prepare_retained` from weapon
commit `b58524c` is the sender implementation. Its data agreement remains intact:
the committed Hand frame must name the retained Rig Scene, not a C0 counter, and
all Native/actor/stream/input/reference/binding identities must agree.

## Registration and lifetime

- Add one supplemental export, `GearsVrAttachWeaponMainMountProviderV1`, following
  the existing optional Call/Action attachment pattern on `weapon_hand::Broker`.
  Use the current parent hand cookie and Native incarnation. No new cookie,
  independent pin, second broker, or mutable Frame extension is introduced.
- Registration supplies exact request/response sizes, one prepare callback and
  the same context as the parent. The executable callback must belong to the
  already-pinned parent PG image. An identical retry is idempotent; conflicting
  attachment is busy. Wrong/retired cookies do not disturb a newer registration.
- The provider attaches once per parent lifetime. Parent unregister/Native stop
  disables admission and drains it together with existing callbacks/transactions.
  There is no separate detach or permission to free PG context early.
- Every query takes the parent's existing callback count/admission before calling
  PG, copies the registration while admitted, releases the admission mutex, then
  invokes PG under the ordinary engine-free callback/reentry guard. End accounting
  runs exactly once on return or fault. Stop during a query cannot unpin or reset
  its callback/context; worker collection still requires exact-cookie quiescence.
- Admission also excludes existing callback/restore, Call/Action/external scopes,
  transactions/invalidation/debt and a poisoned ledger. This C0 query additionally
  requires Scene/stereo depths zero; it does not relax other callback contexts.
  It maintains both `callbacks_` and `normal_calls_` and rechecks cookie, revision,
  operation epoch, revocation generation and the committed Frame after return.
  A nonempty support/left-hand lease or `Broker::authorized()` is not required.
- PG additionally enters its local active-callback lifetime guard, checks running,
  exact cookie/incarnation and GT before touching local equipment state, and
  closes that guard on SEH too. Worker LifecycleScope/SRW is not a GT substitute.
  No wait, join, engine call, setter, asynchronous retention or source publication
  is permitted in the callback. Existing copied-Frame/current-Game accessors and
  bounded read-only asset/attachment checks are allowed.

## Input

The new header will be `tools/vr-common/weapon_main_mount_provider.h`. It contains
fixed-width, standard-layout POD and explicit x64 layout tests; PG-local structs
are mapped field-by-field, never passed as an accidental ABI.

Frozen header SHA256:
`3f20163b1b65b7a74811958d4547b5c43605a6708d063a4049e4935deaa2f2cc`.
Measured MSVC x64 layout: Rig V1 Frame=1824, BodyGeometry=112, Request=1968
(body offset=1856), Response=504, Registration=48. The affected wire test passes
28 checks. These layout tests prove data shape, not runtime admission/lifetime.

`Request` contains its size/version, an immutable retained Rig V1 Frame and actual
Native C0 body data: mesh identity, the complete Marcus geometry signature,
uniform model scale, units, nonzero body revision, basis=2 and basis revision=2.
The maximum source age is exactly 50 ms. The parent cookie/NativeKey comes from
the actual admitted parent; Native projects the same immutable Core with
`project_for_authenticated_registration`. Projection alone is not admission.

The initial body revision is the actual retained Core binding generation and is
echoed for this query only; full current mesh/signature/scale/basis checks remain
independent. This number is not a Hand Frame, C0-to-Scene conversion or writer
token. All source timestamps and the original Scene invocation remain unchanged.

The admitted broker also supplies a unique query id and the actual committed
parent Hand lifecycle. Ready echoes that id and must match that committed Hand
key. Neither value is synthesized from a C0/Kernel counter.

Native prepares this request only after `body::plan` succeeds and before
`hands::compose`, from its current authenticated context and model. It rechecks
that context/source after the callback and retains the original 40 ms admission
budget and 50 ms per-write guards. Query time never renews a source.

## Output and PG mapping

The prepare callback returns a bounded status: ready, inactive, no_weapon,
unheld, animation, busy, unsupported_asset, not_attached, source_mismatch,
body_mismatch, unsupported_geometry. A non-ready result has an empty payload.
Header size/version stay valid; reserved fields are zero. Invalid status, SEH,
request mutation, malformed ready payload or forbidden reentry is a callback
fault handled by the existing parent invalidation/drain path, not a free-hand
ownership grant.

`Response` contains the actual committed Hand FrameKey; world/controller/pawn/
mesh/weapon/component/asset identities; hold generation; provider/body/basis
revisions; geometry/scale/units and the explicit known right grip-to-wrist
translation/quaternion. No pointers, target world pose, aim ray, muzzle, support
mount or writer lease are returned.

PG maps the retained Frame and Native body data into the existing local
`BodyContext`, calls `prepare_retained`, and maps the ready Candidate into the
response. It retains the actual held Lancer asset/right attachment checks,
hold-generation checks, before/after source binding and final source-age checks.
It must not synthesize a Hand Frame, force busy/held false, assume geometry, alter
the frame key or carry a previous Candidate into a later query.

## Native consumption

Native accepts ready only when the returned key/cookie/incarnation, Scene,
identities and source agreement match the exact retained input, and the returned
geometry/scale/units/revisions match its actual C0. It independently reconstructs
the fixed `marcus115-lancer-held-grip-v1` profile and compares the full right
calibration, rather than accepting arbitrary callback offsets.

The held profile is a separately identified internal mount choice. It does not
relabel or overwrite the immutable Core's stock authored mount. The pure target
adapter explicitly validates either the existing stock mount or the verified
held-Lancer profile. Native supplies the latter only for this ready query.

The one shared C0 output/Kernel remains the only bone writer. Left-hand targets
retain their current policy; 35→51→52 propagates through original FK/increments.
An unavailable/stale/unheld/busy/unsupported query uses the existing stock mount;
it cannot retarget a weapon's authoritative fire ray or claim muzzle alignment.
Malformed responses grant nothing. No result is cached beyond the exact C0
query/transaction. All large query/work buffers are persistent GT-owned storage,
not added to the assembly hook stack.

## Targeted validation

Before gameplay: fixed ABI/layout; exact same-cookie/module/context attachment;
idempotent/wrong-cookie/conflicting cases; callback stop/unregister race and
worker-only unpin; nested/SEH/malformed response; normal non-ready stock fallback;
actual profile/geometry/key/source rejection; unchanged-source/sole-writer target
composition and no-CAS age behavior. Test only changed scopes, not full unrelated
renderer or PG history.

First game candidate uses 0.4/background/behind/observe-only/mute, current save
and audio protection, and R2 mode OFF. Observe one actual Lancer RGrip hold,
controlled simulator orientation/position changes, then unhold. Physical
controller background input remains disabled. Correlate actual
PG ready key/held asset with the Native mount choice and C0 wrist result. Keep
normal cleanup and pending final render/headset/aim/cover checks explicit.
The owner starts any later physical-controller/headset check manually; it is a
separate acceptance step.
