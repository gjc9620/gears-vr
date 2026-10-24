#pragma once
#include "weapon_hand_participant.h"

// Supplementary notification channel for Native's single CallFunction owner.
// This does not add an arbiter, engine call, hook, original function pointer,
// input source or damage writer. The existing hand V1 ABI remains unchanged.
namespace gears_weapon_call {
constexpr std::uint32_t version=1;
constexpr std::uint32_t state_bytes=128;

struct Invocation {
    std::uint32_t size{sizeof(Invocation)},abi_version{version};
    std::uint64_t hand_cookie{},native_started_qpc{},call_id{};
    std::uint32_t thread_id{},reserved{};
    // Exact original arguments, including null/opaque values. These are not
    // a parameter buffer or permission to dereference a game object.
    std::uint64_t self{},caller_frame{},result{},callee{};
};
inline bool invocation_valid(Invocation const& call) noexcept {
    return call.size==sizeof(Invocation)&&call.abi_version==version&&call.hand_cookie&&
        call.native_started_qpc&&call.call_id&&call.thread_id&&!call.reserved;
}
inline bool same(Invocation const& a,Invocation const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.hand_cookie==b.hand_cookie&&
        a.native_started_qpc==b.native_started_qpc&&a.call_id==b.call_id&&a.thread_id==b.thread_id&&
        a.reserved==b.reserved&&a.self==b.self&&a.caller_frame==b.caller_frame&&a.result==b.result&&a.callee==b.callee;
}
// Native zero-initializes one aligned, opaque, invocation-local journal before
// begin. The same storage remains live through original and the final notice.
// PG may retain POD tokens here; neither side retains this pointer afterward.
struct alignas(8) State {std::array<std::byte,state_bytes> storage{};};
enum class End:std::uint32_t {closed,unresolved};
inline bool end_closed(End value) noexcept {return value==End::closed;}
enum class UnwindReason:std::uint32_t {original_exception,notification_fault};

#if defined(_WIN32)
#define GEARS_WEAPON_CALL __cdecl
#else
#define GEARS_WEAPON_CALL
#endif
using Begin=void(GEARS_WEAPON_CALL*)(void*,Invocation const*,State*) noexcept;
using Returned=End(GEARS_WEAPON_CALL*)(void*,Invocation const*,State*,std::uint64_t original_rax) noexcept;
using Unwind=End(GEARS_WEAPON_CALL*)(void*,Invocation const*,State*,UnwindReason) noexcept;
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
using Attach=gears_weapon_hand::Reply(GEARS_WEAPON_CALL*)(Registration const*,gears_weapon_hand::RegistrationResult*) noexcept;
inline constexpr char attach_export[]="GearsVrAttachWeaponCallBoundaryV1";
#undef GEARS_WEAPON_CALL

// Attach once to the exact live hand cookie/native incarnation. Context must
// equal Registration.context of that hand participant; all three executable
// callback addresses must belong to the SAME pinned PG image as its callbacks.
// An identical attach retry is idempotent; a different attach is rejected.
// There is no independent detach or lifetime. The accepted acknowledgement is
// the exact existing hand RegistrationResult. Native must actually own the
// wrapper before accepting. It never exposes original/trampoline to PG.
//
// Before every admitted pre, Native allocates a nonzero, non-reused call_id,
// freezes Invocation and holds a transaction lease. Soft hand/input loss does
// not suppress pre: PG must see nested calls to mask unsupported fire scopes.
// Hard terminal stop closes NEW admission; every attempted begin still gets
// a matching returned or unwind with identical Invocation/State, even after
// decline, stop or a notification fault. Callbacks are synchronous on the
// invoking thread, with no engine calls, allocation or waiting by PG.
//
// Native alone invokes the original exactly once with its four arguments and
// returns its unchanged RAX bits, or propagates its original exception. Notices
// cannot skip/retry/replace original. Returned runs after original returned and
// before its caller resumes. Original exception cleanup calls unwind from a
// POD-only __finally bridge. A fault in returned additionally attempts unwind
// with notification_fault; PG cleanup must be idempotent. Faults in notices are
// isolated from the game's outcome and terminally invalidate the hand client.
// Only a normally completed notice returning exact End::closed closes work.
// Any other enum value or notification fault creates sticky auxiliary debt:
// later End::closed does not erase it. Keep the module/context pinned. V1 has no
// repair/reset that turns an uncertain partially executed callback into closed.
//
// Existing hand unregister/native stop drains the WHOLE begin-original-end
// transaction, including the original gap and cleanup, plus auxiliary debt.
// Callback count zero alone is insufficient. Exact-cookie quiescent/retired
// receipts require both hand and auxiliary work fully closed; wrong_cookie or
// draining never grants permission to free code/context. No wait under GT,
// callback or stereo_mutex. PG also keeps its outer callback lease across the
// original gap; Native quiescence does not replace PG's own hook drain.
//
// Immutable Invocation describes observation only. Existing separately gated
// PG damage code may validate and CAS authorized Result contents; this channel
// itself grants no game-memory write permission or damage semantics.
static_assert(sizeof(void*)==8&&sizeof(Invocation)==72&&alignof(Invocation)==8);
static_assert(offsetof(Invocation,hand_cookie)==8&&offsetof(Invocation,thread_id)==32&&offsetof(Invocation,self)==40);
static_assert(sizeof(State)==state_bytes&&alignof(State)==8&&sizeof(Registration)==64);
static_assert(offsetof(Registration,hand_cookie)==16&&offsetof(Registration,context)==32&&offsetof(Registration,begin)==40);
static_assert(std::is_standard_layout_v<Invocation>&&std::is_trivially_copyable_v<Invocation>);
static_assert(std::is_standard_layout_v<State>&&std::is_trivially_copyable_v<State>);
static_assert(std::is_standard_layout_v<Registration>&&std::is_trivially_copyable_v<Registration>);
} // namespace gears_weapon_call
