#pragma once
#include "weapon_hand_participant.h"

// Independent supplement to the sole Native CallFunction wrapper. No new
// hook, arbiter, engine pointer, frame source or game-write authority.
// The existing hand V1 and call V1 bytes and meanings remain unchanged.
namespace gears_weapon_action {
constexpr std::uint32_t version=1;
constexpr std::uint32_t state_bytes=128;
enum InvocationFlags:std::uint32_t {engine_calls_allowed=1};

struct Invocation {
    std::uint32_t size{sizeof(Invocation)},abi_version{version};
    std::uint64_t hand_cookie{},native_started_qpc{},gate_id{};
    std::uint32_t thread_id{},flags{};
    // Exact original opaque arguments; caller_frame is not a params buffer.
    std::uint64_t self{},caller_frame{},result{},callee{};
};
inline bool invocation_valid(Invocation const& call) noexcept {
    return call.size==sizeof(Invocation)&&call.abi_version==version&&call.hand_cookie&&
        call.native_started_qpc&&call.gate_id&&call.thread_id&&!(call.flags&~engine_calls_allowed);
}
inline bool same(Invocation const& a,Invocation const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.hand_cookie==b.hand_cookie&&
        a.native_started_qpc==b.native_started_qpc&&a.gate_id==b.gate_id&&a.thread_id==b.thread_id&&
        a.flags==b.flags&&a.self==b.self&&a.caller_frame==b.caller_frame&&a.result==b.result&&a.callee==b.callee;
}
struct alignas(8) State {std::array<std::byte,state_bytes> storage{};};

// Only exact, normally returned values are valid. A recognized action always
// invalidates Native's existing owner/cache before its original executes.
// "complete" concerns the required pre-action handback at that instant; it
// never says the animation is over or grants a new frame/write.
enum class BeginReply:std::uint32_t {unrelated,handback_complete,handback_unresolved};
inline bool begin_reply_valid(BeginReply value) noexcept {
    return value==BeginReply::unrelated||value==BeginReply::handback_complete||value==BeginReply::handback_unresolved;
}
inline bool action_seen(BeginReply value) noexcept {return value!=BeginReply::unrelated;}
enum class End:std::uint32_t {closed,unresolved};
inline bool end_closed(End value) noexcept {return value==End::closed;}
enum class UnwindReason:std::uint32_t {original_exception,notification_fault};

#if defined(_WIN32)
#define GEARS_WEAPON_ACTION __cdecl
#else
#define GEARS_WEAPON_ACTION
#endif
using Begin=BeginReply(GEARS_WEAPON_ACTION*)(void*,Invocation const*,State*) noexcept;
using Returned=End(GEARS_WEAPON_ACTION*)(void*,Invocation const*,State*,std::uint64_t original_rax) noexcept;
using Unwind=End(GEARS_WEAPON_ACTION*)(void*,Invocation const*,State*,UnwindReason) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version};
    std::uint32_t invocation_bytes{sizeof(Invocation)},state_bytes{sizeof(State)};
    std::uint64_t hand_cookie{},native_started_qpc{};
    void* context{};Begin begin{};Returned returned{};Unwind unwind{};
};
inline bool registration_shape_valid(Registration const& registration) noexcept {
    return registration.size==sizeof(Registration)&&registration.abi_version==version&&
        registration.invocation_bytes==sizeof(Invocation)&&registration.state_bytes==sizeof(State)&&
        registration.hand_cookie&&registration.native_started_qpc&&registration.context&&
        registration.begin&&registration.returned&&registration.unwind;
}
inline bool same(Registration const& a,Registration const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.invocation_bytes==b.invocation_bytes&&
        a.state_bytes==b.state_bytes&&a.hand_cookie==b.hand_cookie&&a.native_started_qpc==b.native_started_qpc&&
        a.context==b.context&&a.begin==b.begin&&a.returned==b.returned&&a.unwind==b.unwind;
}
using Attach=gears_weapon_hand::Reply(GEARS_WEAPON_ACTION*)(Registration const*,gears_weapon_hand::RegistrationResult*) noexcept;
inline constexpr char attach_export[]="GearsVrAttachWeaponActionGateV1";
#undef GEARS_WEAPON_ACTION

// Attach-once to the active exact hand cookie/native incarnation, AFTER call
// V1 attached, with identical hand context and executable callbacks in that
// same already-pinned PG image. Identical retry is idempotent. No independent
// detach, module lifetime or pin. Native must actually own the sole wrapper.
//
// Native freezes a nonzero, never-reused gate id, arguments and flags. It
// zeroes invocation-local State and holds a WHOLE transaction lease across:
// Gate begin -> existing call V1 begin -> sole original -> V1 end -> Gate end.
// Broker admission locks are released before notices. A NoAuth notice can
// inherit a caller's Scene/stereo scope; Native must not unlock it. Only the
// engine-capable begin described below requires all tracked locks absent.
// Original arguments, RAX and exception remain unchanged; Gate cannot skip,
// retry or replace it.
//
// engine_calls_allowed defaults FALSE. Native may set it ONLY for an outermost
// Gate begin on its certified GT, outside ordinary callbacks, with Scene TLS
// inactive and explicitly tracked stereo-lock depth zero. World Scope.depth
// alone does not prove this. PG must independently validate context, exact
// cookie/incarnation and GT before touching its non-atomic action state.
// With the flag, begin may only perform the EXISTING bounded GT restoration
// and reflection/optional observation paths. It grants no extra game operation
// or field rights. Without it PG must not invoke engine code; required but
// unavailable handback is unresolved, never silently reported complete.
// Returned/unwind never invoke engine code, wait, join, unload or restore again.
//
// A dedicated Native Action callback phase permits the RESTORATION's nested
// CallFunction transactions. Every nested call still gets its own Gate, V1
// notices and sole original, but engine_calls_allowed is FALSE. Ordinary V1 /
// Frame / invalidate reentry remains forbidden. Keep the old PG self-token,
// mutating/releasing/dual-drain guards; a real game call cannot borrow the owned
// bridge exemption. A no-auth gate does not make a second restoration attempt.
// Permission belongs to that OUTERMOST begin's single restore stage. It is not
// an ambient TLS permission inherited by PE/bridge calls made by its setters:
// such calls may arrive before a nested Native Gate exists. Every PG entry
// still classifies/revokes, but cannot start another restoration. PG records
// this stage before its first engine call and explicitly closes it on SEH.
// NoAuth cannot reuse old before_action just by suppressing a visible setter:
// state queries, pending cleanup and RAII finish/drain can themselves call the
// engine. Its classify/revoke/bookkeeping path must be separately engine-free.
//
// Native establishes a Scene/cache barrier BEFORE invoking begin: a Scene
// reentered during begin cannot reuse even a same-frame cached hand grant.
// For a related action PG revokes its old Plan/bindings/published authorizer
// BEFORE its first restore setter. Native then invalidates its existing sole
// owner/cache OUTSIDE begin and BEFORE original on action_seen, malformed reply
// or notification fault. PG keeps action-held observation until existing quiet
// slots, identity/stability, required grip release and a fresh valid source
// permit recovery. Function return alone is not animation completion.
//
// Every attempted begin owes returned or unwind with identical Invocation and
// State, including stop and notification faults. POD __finally cleanup closes
// Native/PG TLS bookkeeping on SEH; /EHsc C++ destructors alone are insufficient.
// Original SEH propagates unchanged. A returned-notice fault also attempts
// notification_fault unwind; end is idempotent and cannot double-decrement.
//
// Malformed reply, notification fault, handback_unresolved, broken accounting
// or a non-closed end creates sticky Gate debt. Later closed cannot erase it.
// Existing hand unregister/stop includes whole Gate/V1/Frame transactions,
// callbacks and both debts; callback count zero during original is not drained.
// Exact-cookie quiescence plus PG's local hook/journal cleanliness are required
// before worker-only collection/unpin. Timeout/draining/wrong_cookie never frees
// code/context. No lifecycle wait in a callback, GT or stereo lock.
static_assert(sizeof(void*)==8&&sizeof(Invocation)==72&&alignof(Invocation)==8);
static_assert(offsetof(Invocation,hand_cookie)==8&&offsetof(Invocation,thread_id)==32&&offsetof(Invocation,self)==40);
static_assert(sizeof(State)==state_bytes&&alignof(State)==8&&sizeof(Registration)==64);
static_assert(offsetof(Registration,hand_cookie)==16&&offsetof(Registration,context)==32&&offsetof(Registration,begin)==40);
static_assert(std::is_standard_layout_v<Invocation>&&std::is_trivially_copyable_v<Invocation>);
static_assert(std::is_standard_layout_v<State>&&std::is_trivially_copyable_v<State>);
static_assert(std::is_standard_layout_v<Registration>&&std::is_trivially_copyable_v<Registration>);
} // namespace gears_weapon_action
