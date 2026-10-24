#pragma once
#include "weapon_hand_participant.h"

// Draft: supplementary provider permission for existing PG PE/bridge entries.
// Neither frozen hand V1 nor CallFunction V1 nor ActionGate V1 is changed.
namespace gears_weapon_boundary {
constexpr std::uint32_t version=1,state_bytes=512;
enum class Source:std::uint32_t {process_event=1,owned_bridge=2};
enum RequestFlags:std::uint32_t {related=1,unresolved=2};
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    Source source{};std::uint32_t flags{};
    std::uint64_t hand_cookie{},native_started_qpc{},context{};
    std::uint64_t object{},function{};
    std::uint32_t thread_id{},reserved{};
};
inline bool request_valid(Request const& r) noexcept {
    return r.size==sizeof(Request)&&r.abi_version==version&&
        (r.source==Source::process_event||r.source==Source::owned_bridge)&&
        (r.flags&related)&&!(r.flags&~(related|unresolved))&&
        r.hand_cookie&&r.native_started_qpc&&r.context&&r.object&&r.function&&r.thread_id&&!r.reserved;
}
enum ReceiptFlags:std::uint32_t {scope_active=1,engine_calls_allowed=2};
struct Receipt {
    std::uint32_t size{sizeof(Receipt)},abi_version{version};
    std::uint64_t hand_cookie{},native_started_qpc{},scope_id{},revocation_generation{};
    std::uint32_t thread_id{},flags{};
};
struct alignas(8) State {std::array<std::byte,state_bytes> storage{};};
enum class Outcome:std::uint32_t {returned,original_exception,notification_fault};
enum FinishFlags:std::uint32_t {stage_closed=1,handback_complete=2};
struct Finish {
    std::uint32_t size{sizeof(Finish)},abi_version{version};
    Outcome outcome{};std::uint32_t flags{};
};
enum class End:std::uint32_t {closed,unresolved};

#if defined(_WIN32)
#define GEARS_WEAPON_BOUNDARY __cdecl
#else
#define GEARS_WEAPON_BOUNDARY
#endif
using Revoke=gears_weapon_hand::Reply(GEARS_WEAPON_BOUNDARY*)(Request const*,std::uint64_t* generation) noexcept;
using Open=gears_weapon_hand::Reply(GEARS_WEAPON_BOUNDARY*)(Request const*,State*,Receipt*) noexcept;
using Close=End(GEARS_WEAPON_BOUNDARY*)(State*,Receipt const*,Finish const*) noexcept;
inline constexpr char revoke_export[]="GearsVrRevokeWeaponActionV1";
inline constexpr char open_export[]="GearsVrOpenWeaponActionBoundaryV1";
inline constexpr char close_export[]="GearsVrCloseWeaponActionBoundaryV1";
#undef GEARS_WEAPON_BOUNDARY

// PG's separately audited, ENGINE-FREE inspect runs BEFORE component/timing
// RAII or any old before_action. Unrelated entries need no external scope.
// A related entry first revokes its own plan/bindings/published authorizer,
// then calls Revoke. Revoke validates the exact active cookie/incarnation/
// context via a Native atomic authentication snapshot. It performs only bounded
// safe-copy + atomic revocation: no mutex, callback, engine call or module query.
// Source::owned_bridge is diagnostic, never proof of PG's one-shot self token.
// The existing exact consume_self/prepared_function/authorize_once fence stays.
//
// Revoke immediately disarms Native authorization and increments a nonwrapping
// generation. Scene checks this generation/barrier BEFORE same-frame reuse;
// commit stores the generation frozen at frame begin, and authorized() compares
// against the current generation. A concurrent revoke cannot be undone by an
// old commit. Overflow or Request::unresolved is sticky debt and retains the
// existing module/context lifetime; later successful notices cannot erase it.
// A successful Revoke is NOT a cleanup, permission or quiescence receipt.
//
// Open is OPTIONAL after revocation. Native independently verifies the current
// GT, explicit Scene/stereo/callback/action/restore markers and exact active
// registration. If any inherited context is unsafe, it returns a no-auth reply
// WITHOUT acquiring a mutex/calling PG/opening a scope; State remains zero and
// Receipt has no active/engine flag. PG stays on its separate engine-free path.
// If required handback cannot be made, PG reports Revoke with unresolved too.
// Other nonaccepted replies never imply detached/quiescent code.
//
// Only a normally returned accepted Open with BOTH flags yields one invocation-
// local restore stage. It creates a Native address-bound, GT/LIFO-bound whole-
// gap lease in this exact State, using the existing hand pin, not a second pin.
// PG records its own single stage before the first engine call. It may restore
// only existing owned fields through their original validated fences. This is
// not ambient TLS permission and grants no new field, frame or game operation.
// Every recursive PE/bridge independently classifies/revokes; it cannot borrow
// the parent's stage. Nested CF still receives NoAuth ActionGate + V1 + original.
//
// The scope starts before old action helpers and lives through PE original or
// invoke_engine/execute_component_forward and original bridge_after. Never put
// an export, VM operation or log between final authorize_once and its setter.
// PG uses POD finally to Close every accepted scope, even on SEH/stop. Original
// args/RAX/exceptions remain PG's existing forwarding responsibility.
//
// Close validates immutable Receipt + exact State address, owner GT and LIFO.
// It does bookkeeping only; never engine calls, callback, restore, wait or unpin.
// A valid Finish needs both stage_closed and handback_complete. Unknown outcome,
// exception bookkeeping failure, missing handback, mutation or an invalid token
// retains sticky debt. closed concerns Native scope accounting, not animation
// completion. It cannot clear prior debt. All these scopes join existing hand/
// Gate/V1/frame transactions in the worker-only collect/quiescence condition.
//
// This draft is not connected. STREAM/STOW remains disabled until provider,
// PG restoration/no-auth paths, cache revocation and actual actions are tested.
static_assert(sizeof(Request)==64&&sizeof(Receipt)==48&&sizeof(Finish)==16);
static_assert(alignof(State)==8&&sizeof(State)==state_bytes);
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
static_assert(std::is_standard_layout_v<Receipt>&&std::is_trivially_copyable_v<Receipt>);
static_assert(std::is_standard_layout_v<State>&&std::is_trivially_copyable_v<State>);
} // namespace gears_weapon_boundary
