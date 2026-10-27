#pragma once
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_call_boundary.h"
#include "../vr-common/weapon_action_gate.h"
#include "../vr-common/weapon_main_mount_provider.h"
#include "../vr-common/body_main_mount_provider.h"
#include "../vr-common/weapon_lancer_wrist_mount.h"
#include "weapon_action_boundary_state.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <mutex>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

// Transport/lifetime only: no arbiter, logical-input adapter, token allocator,
// engine call or game-memory access. The Native Scene caller owns eligibility.
namespace gears_weapon_hand {
namespace call=::gears_weapon_call;
namespace action=::gears_weapon_action;
namespace boundary=::gears_weapon_boundary;
namespace boundary_state=::gears_weapon_boundary_state;
namespace context=::gears_native_context;
namespace mount=::gears_weapon_main_mount_provider;namespace body_mount=::gears_body_main_mount_provider;namespace rig=::gears_rig_tracking;
struct MainMountQueryResult {
    enum class Code {unavailable,ready,not_ready,context_rejected,stale,fault};
    // Internal observation only. This is not part of the supplemental wire ABI.
    enum class Branch:std::uint32_t {not_run,unavailable,context,source_before,body,callback_fault,changed,source_after,reply,exception,query_limit,reentrant,count};
    Code code{Code::unavailable};mount::Status status{mount::Status::inactive};std::uint64_t query_id{};
    Branch branch{Branch::unavailable};std::uint32_t callback_status{UINT32_MAX},source_failure_mask{};
    // Rig publication, committed now, Scene, stereo receive, controller receive, sync.
    std::array<std::int64_t,6> source_ages_ns{{-1,-1,-1,-1,-1,-1}};
};
class Broker;
// Pure classification; Native GT certification is supplied by the Scene owner,
// never by an invocation/request's claimed thread. This grants no field rights.
inline bool classify_action_context(context::Ledger const& ledger,bool native_gt,bool outermost,bool enabled) noexcept {
    return enabled&&outermost&&context::engine_context_clear(ledger,native_gt);
}
struct ActionScope {
    Broker* owner{};ActionScope* previous{};std::uintptr_t address{};
    std::uint64_t token{};std::uint32_t thread{};
    bool active{},attempted{},notification_fault{},revoke_owner_cache{},barrier_active{};
    action::Registration registration{};call::Registration call_registration{};action::Invocation invocation{},notice{};
    action::State state{};context::Ticket ticket{};
};
static_assert(std::is_standard_layout_v<ActionScope>&&std::is_trivially_copyable_v<ActionScope>&&std::is_trivially_destructible_v<ActionScope>);
// Native stack token only. PG receives notice/state, never this enclosing POD.
// The caller must keep its code-residency admission across original and invoke
// finish_call from __finally even when begin_call's notification faulted.
struct CallScope {
    Broker* owner{};CallScope* previous{};std::uintptr_t address{};
    std::uint64_t token{};std::uint32_t thread{};
    bool active{},attempted{},notification_fault{};
    call::Registration registration{};
    call::Invocation invocation{},notice{};
    call::State state{};
};
static_assert(std::is_standard_layout_v<CallScope>&&std::is_trivially_copyable_v<CallScope>&&std::is_trivially_destructible_v<CallScope>);
// Ready still requires a valid per-frame prepare. Unavailable (including an
// uncollected registration) MUST NOT be converted into an empty/free hand.
enum class Availability {ready,absent_quiescent,unavailable};
struct BrokerSession {
    std::uint32_t game_pid{},owner_worker_thread{};
    std::uint64_t game_start{},native_started_qpc{};
};
struct ModulePin {std::uintptr_t handle{},module{};};
struct BrokerBackend {
    void* context{};
    // Failure may still return an acquired handle. It too is worker-collected.
    bool (*pin)(void*,Registration const&,ModulePin&) noexcept{};
    bool (*unpin)(void*,ModulePin) noexcept{};
    std::int64_t (*clock_ns)(void*) noexcept{};
    std::uint32_t (*thread)(void*) noexcept{};
    // False means the protected callback faulted. Callbacks remain noexcept;
    // these wrappers must not turn C++ throw/terminate into a successful reply.
    bool (*prepare)(void*,Registration const&,Frame*,Proposal*,std::uint32_t&) noexcept{};
    bool (*commit)(void*,Registration const&,Frame*,Proposal*,Decision*,std::uint32_t&) noexcept{};
    bool (*invalidate)(void*,Registration const&,Invalidation*) noexcept{};
    // Auxiliary callbacks share the EXISTING pin. No second module ref or owner.
    bool (*call_module)(void*,call::Registration const&,ModulePin) noexcept{};
    bool (*call_begin)(void*,call::Registration const&,call::Invocation*,call::State*) noexcept{};
    bool (*call_returned)(void*,call::Registration const&,call::Invocation*,call::State*,std::uint64_t,std::uint32_t&) noexcept{};
    bool (*call_unwind)(void*,call::Registration const&,call::Invocation*,call::State*,call::UnwindReason,std::uint32_t&) noexcept{};
    bool (*action_module)(void*,action::Registration const&,ModulePin) noexcept{};
    bool (*action_begin)(void*,action::Registration const&,action::Invocation*,action::State*,std::uint32_t&) noexcept{};
    bool (*action_returned)(void*,action::Registration const&,action::Invocation*,action::State*,std::uint64_t,std::uint32_t&) noexcept{};
    bool (*action_unwind)(void*,action::Registration const&,action::Invocation*,action::State*,action::UnwindReason,std::uint32_t&) noexcept{};
    bool (*main_mount_module)(void*,mount::Registration const&,ModulePin) noexcept{};
    bool (*main_mount_prepare)(void*,mount::Registration const&,mount::Request const*,mount::Response*,std::uint32_t&) noexcept{};
    bool (*body_mount_module)(void*,body_mount::Registration const&,ModulePin) noexcept{};
    bool (*body_mount_prepare)(void*,body_mount::Registration const&,body_mount::Request const*,body_mount::Response*,std::uint32_t&) noexcept{};
    // Fixed Native-private validator, never a callback supplied by PG or by
    // Request. Called outside mutex with exact query lifetime reserved.
    bool (*body_mount_current)(void*,body_mount::Source const&) noexcept{};
};

namespace weapon_broker_win32 {
inline bool copy(void* out,void const* in,std::size_t size) noexcept {
    if(!out||!in)return false;
#if defined(_WIN32) && defined(_MSC_VER)
    __try {std::memcpy(out,in,size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
#else
    std::memcpy(out,in,size);return true;
#endif
}
#if defined(_WIN32)
inline bool executable_image(std::uintptr_t address,std::uintptr_t& module) noexcept {
    MEMORY_BASIC_INFORMATION region{};
    if(!address||VirtualQuery(reinterpret_cast<void const*>(address),&region,sizeof(region))!=sizeof(region)||
       region.State!=MEM_COMMIT||region.Type!=MEM_IMAGE||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto access=region.Protect&0xffu;
    if(access!=PAGE_EXECUTE&&access!=PAGE_EXECUTE_READ&&access!=PAGE_EXECUTE_READWRITE&&access!=PAGE_EXECUTE_WRITECOPY)return false;
    module=reinterpret_cast<std::uintptr_t>(region.AllocationBase);return module!=0;
}
inline bool pin(void*,Registration const& r,ModulePin& out) noexcept {
    out={};std::uintptr_t first{},second{},third{};
    const auto p=reinterpret_cast<std::uintptr_t>(r.prepare),c=reinterpret_cast<std::uintptr_t>(r.commit),i=reinterpret_cast<std::uintptr_t>(r.invalidate);
    if(!executable_image(p,first)||!executable_image(c,second)||!executable_image(i,third)||first!=second||first!=third)return false;
    HMODULE held{};
    // An ordinary refcount pin, not GET_MODULE_HANDLE_EX_FLAG_PIN (irreversible).
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(p),&held))return false;
    out={reinterpret_cast<std::uintptr_t>(held),first};
    std::uintptr_t again{};
    return out.handle==first&&executable_image(p,again)&&again==first&&
        executable_image(c,again)&&again==first&&executable_image(i,again)&&again==first;
}
inline bool unpin(void*,ModulePin pin_value) noexcept {return pin_value.handle&&FreeLibrary(reinterpret_cast<HMODULE>(pin_value.handle))!=FALSE;}
inline std::uint32_t thread(void*) noexcept {return GetCurrentThreadId();}
inline int filter(unsigned long code) noexcept {
    // Respect noexcept/C++ termination and an exhausted stack; no RAII exists
    // in the small __try bridge, and the original fault is never called success.
    return code==0xe06d7363ul||code==EXCEPTION_STACK_OVERFLOW?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;
}
#else
inline bool pin(void*,Registration const&,ModulePin&) noexcept {return false;}
inline bool unpin(void*,ModulePin) noexcept {return false;}
inline std::uint32_t thread(void*) noexcept {return 0;}
#endif
inline std::int64_t counter_ns(std::int64_t count,std::int64_t frequency) noexcept {
    constexpr std::int64_t billion=1000000000;
    const auto limit=(std::numeric_limits<std::int64_t>::max)();
    if(count<=0||frequency<=0)return 0;
    const auto seconds=count/frequency,remainder=count%frequency;
    if(seconds>limit/billion||remainder>limit/billion)return 0;
    const auto whole=seconds*billion,fraction=remainder*billion/frequency;
    return whole>limit-fraction?0:whole+fraction;
}
inline std::int64_t clock_ns(void*) noexcept {
#if defined(_WIN32)
    LARGE_INTEGER counter{},frequency{};
    if(!QueryPerformanceCounter(&counter)||!QueryPerformanceFrequency(&frequency))return 0;
    // Same integer QPC/QPF formula/epoch as pose_bridge::monotonic_ns.
    return counter_ns(counter.QuadPart,frequency.QuadPart);
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}
inline bool prepare(void*,Registration const& r,Frame* f,Proposal* p,std::uint32_t& reply) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {reply=r.prepare(r.context,f,p);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    reply=r.prepare(r.context,f,p);return true;
#endif
}
inline bool commit(void*,Registration const& r,Frame* f,Proposal* p,Decision* d,std::uint32_t& reply) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {reply=r.commit(r.context,f,p,d);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    reply=r.commit(r.context,f,p,d);return true;
#endif
}
inline bool invalidate(void*,Registration const& r,Invalidation* message) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {r.invalidate(r.context,message);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    r.invalidate(r.context,message);return true;
#endif
}
inline bool call_module(void*,call::Registration const& r,ModulePin pin_value) noexcept {
#if defined(_WIN32)
    std::uintptr_t module{};
    return pin_value.handle&&pin_value.handle==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.begin),module)&&module==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.returned),module)&&module==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.unwind),module)&&module==pin_value.module;
#else
    (void)r;(void)pin_value;return false;
#endif
}
inline bool action_module(void*,action::Registration const& r,ModulePin pin_value) noexcept {
#if defined(_WIN32)
    std::uintptr_t module{};
    return pin_value.handle&&pin_value.handle==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.begin),module)&&module==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.returned),module)&&module==pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.unwind),module)&&module==pin_value.module;
#else
    (void)r;(void)pin_value;return false;
#endif
}
inline bool action_begin(void*,action::Registration const& r,action::Invocation* invocation,action::State* state,std::uint32_t& result) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {result=static_cast<std::uint32_t>(r.begin(r.context,invocation,state));return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    result=static_cast<std::uint32_t>(r.begin(r.context,invocation,state));return true;
#endif
}
inline bool action_returned(void*,action::Registration const& r,action::Invocation* invocation,action::State* state,std::uint64_t rax,std::uint32_t& result) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {result=static_cast<std::uint32_t>(r.returned(r.context,invocation,state,rax));return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    result=static_cast<std::uint32_t>(r.returned(r.context,invocation,state,rax));return true;
#endif
}
inline bool action_unwind(void*,action::Registration const& r,action::Invocation* invocation,action::State* state,action::UnwindReason reason,std::uint32_t& result) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {result=static_cast<std::uint32_t>(r.unwind(r.context,invocation,state,reason));return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    result=static_cast<std::uint32_t>(r.unwind(r.context,invocation,state,reason));return true;
#endif
}
inline bool call_begin(void*,call::Registration const& r,call::Invocation* invocation,call::State* state) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {r.begin(r.context,invocation,state);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    r.begin(r.context,invocation,state);return true;
#endif
}
inline bool call_returned(void*,call::Registration const& r,call::Invocation* invocation,call::State* state,std::uint64_t rax,std::uint32_t& result) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {result=static_cast<std::uint32_t>(r.returned(r.context,invocation,state,rax));return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    result=static_cast<std::uint32_t>(r.returned(r.context,invocation,state,rax));return true;
#endif
}
inline bool call_unwind(void*,call::Registration const& r,call::Invocation* invocation,call::State* state,call::UnwindReason reason,std::uint32_t& result) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {result=static_cast<std::uint32_t>(r.unwind(r.context,invocation,state,reason));return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    result=static_cast<std::uint32_t>(r.unwind(r.context,invocation,state,reason));return true;
#endif
}
inline bool main_mount_module(void*,mount::Registration const& r,ModulePin pin_value) noexcept {
#if defined(_WIN32)
    std::uintptr_t module{};return pin_value.handle&&pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.prepare),module)&&module==pin_value.module;
#else
    (void)r;(void)pin_value;return false;
#endif
}
inline bool main_mount_prepare(void*,mount::Registration const& r,mount::Request const* input,mount::Response* output,std::uint32_t& reply) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {reply=r.prepare(r.context,input,output);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    reply=r.prepare(r.context,input,output);return true;
#endif
}
inline bool body_mount_module(void*,body_mount::Registration const& r,ModulePin pin_value) noexcept {
#if defined(_WIN32)
    std::uintptr_t module{};return pin_value.handle&&pin_value.module&&
        executable_image(reinterpret_cast<std::uintptr_t>(r.prepare),module)&&module==pin_value.module;
#else
    (void)r;(void)pin_value;return false;
#endif
}
inline bool body_mount_prepare(void*,body_mount::Registration const& r,body_mount::Request const* input,body_mount::Response* output,std::uint32_t& reply) noexcept {
#if defined(_WIN32) && defined(_MSC_VER)
    __try {reply=r.prepare(r.context,input,output);return true;}
    __except(filter(GetExceptionCode())){return false;}
#else
    reply=r.prepare(r.context,input,output);return true;
#endif
}
} // namespace weapon_broker_win32
inline BrokerBackend weapon_hand_win32_backend() noexcept {
    return {nullptr,weapon_broker_win32::pin,weapon_broker_win32::unpin,weapon_broker_win32::clock_ns,
        weapon_broker_win32::thread,weapon_broker_win32::prepare,weapon_broker_win32::commit,weapon_broker_win32::invalidate,
        weapon_broker_win32::call_module,weapon_broker_win32::call_begin,weapon_broker_win32::call_returned,weapon_broker_win32::call_unwind,
        weapon_broker_win32::action_module,weapon_broker_win32::action_begin,weapon_broker_win32::action_returned,weapon_broker_win32::action_unwind,
        weapon_broker_win32::main_mount_module,weapon_broker_win32::main_mount_prepare,
        weapon_broker_win32::body_mount_module,weapon_broker_win32::body_mount_prepare,nullptr};
}

class Broker {
    enum class State {empty,registering,active,draining,quiescent,collecting};
    struct CallbackMark {
        Broker* broker{};bool violated{},action_phase{};CallbackMark* previous{};context::Ticket ticket{};
        explicit CallbackMark(Broker* b,bool restore=false) noexcept:broker(b),action_phase(restore),previous(current_){
            current_=this;if(!context::enter(context::thread_ledger,ticket,restore?context::Kind::restore:context::Kind::callback))violated=true;
        }
        ~CallbackMark(){if(ticket.active)(void)context::leave(context::thread_ledger,ticket);current_=previous;}
    };
    inline static thread_local CallbackMark* current_{};
    inline static thread_local CallScope* call_top_{};
    inline static thread_local ActionScope* action_top_{};
    struct InvalidateCall {Registration registration{};Invalidation message{};bool needed{};};
    BrokerSession session_;BrokerBackend backend_;std::mutex mutex_;
    std::atomic<State> state_{State::empty};
    std::atomic<Availability> availability_{Availability::absent_quiescent};
    std::atomic<std::uint64_t> revision_{1},cookie_{};
    std::atomic<unsigned> callbacks_{},transactions_{};
    std::atomic<unsigned> aux_scopes_{};
    std::atomic<bool> aux_debt_{},call_attached_{};
    std::atomic<unsigned> action_scopes_{},action_barriers_{},external_scopes_{};
    std::atomic<bool> action_debt_{},action_attached_{};
    std::atomic<bool> action_invalidation_pending_snapshot_{};
    std::atomic<std::uint64_t> action_context_{},authorized_frame_{},authorized_generation_{};
    std::atomic<std::uint32_t> certified_game_thread_{};
    boundary_state::Revocation revocation_{};boundary_state::Pool external_{revocation_};
    // Atomic address-only index permits an unsafe-context NoAuth refusal without
    // taking admission mutex. Pool remains the sole scope/TLS authority.
    std::array<std::atomic<std::uintptr_t>,64> boundary_addresses_{};
    std::array<std::uint64_t,64> boundary_ids_{}; // admission mutex only
    boundary_state::Stamp frame_stamp_{};
    action::Registration action_registration_{};std::uint64_t next_action_id_{1};
    bool engine_restore_enabled_{};
    bool action_invalidation_pending_{}; // admission mutex only
    std::atomic<bool> authorization_{},pin_retained_{};
    Registration registration_{};ModulePin pin_{};
    call::Registration call_registration_{};std::uint64_t next_call_id_{1};
    std::uint64_t next_cookie_{1},transaction_revision_{},operation_epoch_{1};
    std::uint32_t game_thread_{};unsigned normal_calls_{};
    bool published_{},cancel_registration_{},stopping_{},phase_invalid_{},invalidate_running_{},invalidate_done_{true},invalidate_failed_{};
    bool final_invalidate_{};InvalidateReason phase_reason_{};
    bool have_attempt_{},have_commit_{},commit_started_{};
    Frame attempt_{},frozen_{};Proposal proposal_{};Decision committed_{};
    mount::Registration main_mount_registration_{};bool main_mount_attached_{};std::uint64_t next_main_mount_query_{1};
    // Single admitted GT query; counters exclude other operations while these
    // large immutable/request/response buffers are live. Never on hook stack.
    struct MainMountWork {mount::Request request{},shadow{};mount::Response response{};Frame committed{};};
    MainMountWork main_mount_work_{};
    body_mount::Registration body_mount_registration_{};bool body_mount_attached_{};std::uint64_t next_body_mount_query_{1};
    struct BodyMountWork {body_mount::Request request{},shadow{};body_mount::Response response{};Frame committed{};};
    BodyMountWork body_mount_work_{};
    inline static const body_mount::Response empty_body_mount_response_{};
    struct RetiredCookie {
        std::uint64_t cookie{},native_started_qpc{},game_start{};std::uint32_t game_pid{};
    };
    // Per-Broker/session receipts, never a process-global or cross-incarnation
    // cache. Rotation loses a receipt conservatively; it never certifies a range.
    std::array<RetiredCookie,16> retired_{};std::size_t next_retired_{};

    bool retired_locked(std::uint64_t value) const noexcept {
        if(!value)return false;
        for(auto const& receipt:retired_)if(receipt.cookie==value&&receipt.native_started_qpc==session_.native_started_qpc&&
            receipt.game_start==session_.game_start&&receipt.game_pid==session_.game_pid)return true;
        return false;
    }
    void remember_retired_locked() noexcept {
        // Called ONLY after a successful worker unpin, for a registration that
        // was actually published. Failed/partial registration reserves no receipt.
        if(!published_||!cookie_.load())return;
        retired_[next_retired_]={cookie_.load(),session_.native_started_qpc,session_.game_start,session_.game_pid};
        next_retired_=(next_retired_+1)%retired_.size();
    }

    bool reentrant() noexcept {
        for(auto* p=current_;p;p=p->previous)if(p->broker==this){p->violated=true;return true;}
        return false;
    }
    bool call_reentrant() noexcept {
        bool rejected=false;
        for(auto* p=current_;p;p=p->previous)if(p->broker==this&&!p->action_phase){p->violated=true;rejected=true;}
        return rejected;
    }
    boundary_state::Auth action_auth() const noexcept {return {cookie_.load(),session_.native_started_qpc,action_context_.load()};}
    bool action_pending() const noexcept {
        return action_scopes_.load()||external_scopes_.load()||action_debt_.load()||revocation_.debt()||action_invalidation_pending_snapshot_.load();
    }
    void action_unknown() noexcept {
        action_debt_.store(true,std::memory_order_release);revocation_.poison();auxiliary_unknown();
    }
    template<class T> static bool equal_bytes(T const& a,T const& b) noexcept {return std::memcmp(&a,&b,sizeof(T))==0;}
    template<class T> static void snapshot(T& out,T const& value) noexcept {std::memcpy(&out,&value,sizeof(T));}
    bool main_mount_context_clear() const noexcept {
        return !current_&&!call_top_&&!action_top_&&context::engine_context_clear(context::thread_ledger,
            game_thread_&&backend_.thread(backend_.context)==game_thread_&&game_thread_!=session_.owner_worker_thread);
    }
    bool main_mount_idle_locked() const noexcept {
        return main_mount_context_clear()&&!callbacks_.load()&&!normal_calls_&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load()&&
            !action_scopes_.load()&&!external_scopes_.load()&&!action_barriers_.load()&&!action_pending()&&
            !invalidate_running_&&!invalidate_failed_&&!final_invalidate_&&!phase_invalid_&&invalidate_done_;
    }
    bool main_mount_source(rig::Frame const& r,Frame const& f,std::int64_t now) const noexcept {
        const bool sim=(f.flags&allow_sim)!=0;
        return rig::frame_valid(r,now,mount::maximum_source_age_ns,sim)&&active_now(f,now)&&
            now>=f.now_ns&&std::uint64_t(now-f.now_ns)<=mount::maximum_source_age_ns&&
            r.native.game_pid==f.key.game_pid&&r.native.game_thread==f.key.thread_id&&r.native.game_start==f.key.game_start&&
            r.native.native_started_qpc==f.key.native_started_qpc&&r.native.registration_cookie==f.key.registration_cookie&&
            r.scene_invocation==f.key.frame&&r.world_generation==f.key.world_generation&&
            same(r.world,f.world)&&same(r.controller,f.controller)&&same(r.pawn,f.pawn)&&same(r.mesh,f.mesh)&&same(r.weapon,f.weapon)&&
            r.reference_generation==f.reference_generation&&equal_bytes(r.reference,f.reference)&&
            equal_bytes(r.stereo,f.stereo)&&equal_bytes(r.controllers,f.controllers)&&
            counter_fresh(r.scene_qpc,r.qpc_frequency,now)&&
            r.controllers.sync_ns>0&&now>=r.controllers.sync_ns&&std::uint64_t(now-r.controllers.sync_ns)<=mount::maximum_source_age_ns;
    }
    static bool counter_fresh(std::int64_t ticks,std::int64_t frequency,std::int64_t now) noexcept {
        const auto stamp=weapon_broker_win32::counter_ns(ticks,frequency);return stamp>0&&now>=stamp&&std::uint64_t(now-stamp)<=mount::maximum_source_age_ns;
    }
    void observe_main_mount_source(rig::Frame const& r,Frame const& f,std::int64_t now,MainMountQueryResult& result) const noexcept {
        // Called only after the ORIGINAL predicate rejected. Re-evaluating pure
        // POD predicates at that same clock records causes, never authorization.
        const bool sim=(f.flags&allow_sim)!=0;
        const bool checks[]={rig::frame_valid(r,now,mount::maximum_source_age_ns,sim),active_now(f,now),
            now>=f.now_ns&&std::uint64_t(now-f.now_ns)<=mount::maximum_source_age_ns,
            r.native.game_pid==f.key.game_pid&&r.native.game_thread==f.key.thread_id&&r.native.game_start==f.key.game_start&&r.native.native_started_qpc==f.key.native_started_qpc&&r.native.registration_cookie==f.key.registration_cookie,
            r.scene_invocation==f.key.frame,r.world_generation==f.key.world_generation&&same(r.world,f.world)&&same(r.controller,f.controller)&&same(r.pawn,f.pawn)&&same(r.mesh,f.mesh)&&same(r.weapon,f.weapon),
            r.reference_generation==f.reference_generation&&equal_bytes(r.reference,f.reference),equal_bytes(r.stereo,f.stereo),equal_bytes(r.controllers,f.controllers),
            counter_fresh(r.scene_qpc,r.qpc_frequency,now),r.controllers.sync_ns>0&&now>=r.controllers.sync_ns&&std::uint64_t(now-r.controllers.sync_ns)<=mount::maximum_source_age_ns};
        result.source_failure_mask=0;
        for(unsigned i=0;i<std::size(checks);++i)if(!checks[i])result.source_failure_mask|=1u<<i;
        const auto age=[now](std::int64_t stamp){return stamp>0&&now>=stamp?now-stamp:std::int64_t(-1);};
        result.source_ages_ns={{age(r.published_ns<=INT64_MAX?std::int64_t(r.published_ns):0),age(f.now_ns),
            age(weapon_broker_win32::counter_ns(r.scene_qpc,r.qpc_frequency)),age(r.stereo.received_ns),age(r.controllers.received_ns),age(r.controllers.sync_ns)}};
    }
    static bool main_mount_body(mount::BodyGeometry const& b,rig::Frame const& r) noexcept {
        return same(b.mesh,r.mesh)&&b.geometry==gears_hand_authored_mount::marcus115_signature&&b.model_scale==1&&b.units_per_metre==r.units_per_metre&&
            b.revision&&b.revision==r.binding_generation&&b.basis==mount::Basis::marcus_mesh_face_forward_y&&
            b.basis_revision==mount::supported_basis_revision&&!b.reserved;
    }
    bool main_mount_response(std::uint32_t raw) const noexcept {
        auto const& r=main_mount_work_.response;auto const& q=main_mount_work_.shadow;auto const& f=main_mount_work_.committed;
        if(!mount::status_valid(raw)||r.size!=sizeof r||r.abi_version!=mount::version)return false;
        if(raw!=unsigned(mount::Status::ready)){
            // Response has no padding on this ABI. Still use explicit fields
            // rather than allowing unread bytes to stand for an empty payload.
            const mount::Signature empty_geometry{};
            return !r.query_id&&same(r.key,FrameKey{})&&same(r.world,ObjectIdentity{})&&same(r.controller,ObjectIdentity{})&&
                same(r.pawn,ObjectIdentity{})&&same(r.mesh,ObjectIdentity{})&&same(r.weapon,ObjectIdentity{})&&same(r.component,ObjectIdentity{})&&same(r.asset,ObjectIdentity{})&&
                !r.hold_generation&&!r.provider_revision&&!r.body_revision&&!r.basis_revision&&r.basis==mount::Basis::unknown&&!r.reserved&&
                r.geometry==empty_geometry&&r.model_scale==0&&r.units_per_metre==0&&!r.calibration_known&&!r.calibration_reserved&&
                r.grip_to_wrist_metres.x==0&&r.grip_to_wrist_metres.y==0&&r.grip_to_wrist_metres.z==0&&
                r.grip_to_wrist_rotation.x==0&&r.grip_to_wrist_rotation.y==0&&r.grip_to_wrist_rotation.z==0&&r.grip_to_wrist_rotation.w==0;
        }
        gears_weapon_lancer_wrist_mount::Profile profile{};
        if(!gears_weapon_lancer_wrist_mount::marcus115(q.body.geometry,q.body.model_scale,q.body.units_per_metre,profile))return false;
        auto const& c=profile.mount.calibration;
        return r.query_id==q.query_id&&same(r.key,f.key)&&same(r.world,f.world)&&same(r.controller,f.controller)&&same(r.pawn,f.pawn)&&
            same(r.mesh,f.mesh)&&same(r.weapon,f.weapon)&&identity_valid(r.weapon)&&identity_valid(r.component)&&identity_valid(r.asset)&&r.component.outer==r.weapon.address&&
            r.hold_generation&&r.provider_revision==mount::provider_revision&&r.body_revision==q.body.revision&&r.basis_revision==q.body.basis_revision&&r.basis==q.body.basis&&
            !r.reserved&&r.geometry==q.body.geometry&&r.model_scale==q.body.model_scale&&r.units_per_metre==q.body.units_per_metre&&
            r.calibration_known==1&&!r.calibration_reserved&&r.grip_to_wrist_metres.x==c.grip_to_wrist_metres.x&&r.grip_to_wrist_metres.y==c.grip_to_wrist_metres.y&&r.grip_to_wrist_metres.z==c.grip_to_wrist_metres.z&&
            r.grip_to_wrist_rotation.x==c.grip_to_wrist_rotation.x&&r.grip_to_wrist_rotation.y==c.grip_to_wrist_rotation.y&&r.grip_to_wrist_rotation.z==c.grip_to_wrist_rotation.z&&r.grip_to_wrist_rotation.w==c.grip_to_wrist_rotation.w;
    }
    bool body_mount_parent(body_mount::Source const& s,Frame const& f,std::int64_t now) const noexcept {
        auto const& r=s.original_scene;
        // Historical old packets are compared exactly, never declared fresh.
        // The private Native validator separately authenticates the full
        // original metadata source plus the genuine new raw/capture source.
        return frame_shape_valid(f)&&s.metadata_begin_ns>0&&s.metadata_end_ns>=s.metadata_begin_ns&&
            s.metadata_end_ns<=s.tracking.begin_ns&&now>=s.metadata_end_ns&&
            std::uint64_t(now-s.metadata_begin_ns)<=body_mount::scene_age_ns&&
            s.metadata_begin_ns>=f.now_ns&&std::uint64_t(s.metadata_begin_ns-f.now_ns)<=body_mount::scene_age_ns&&
            r.size==sizeof r&&r.abi_version==rig::version&&r.status==rig::Status::ready&&
            r.published_ns&&r.published_ns<=std::uint64_t(s.metadata_begin_ns)&&std::uint64_t(s.metadata_begin_ns)-r.published_ns<=body_mount::scene_age_ns&&
            counter_fresh(r.scene_qpc,r.qpc_frequency,s.metadata_begin_ns)&&s.scene_ticket&&s.scene_cache_revision&&
            s.tracking.parent_cookie==cookie_.load()&&s.tracking.thread==game_thread_&&s.basis.compose&&
            r.native.game_pid==f.key.game_pid&&r.native.game_thread==f.key.thread_id&&r.native.game_start==f.key.game_start&&
            r.native.native_started_qpc==f.key.native_started_qpc&&r.native.registration_cookie==f.key.registration_cookie&&
            r.scene_invocation==f.key.frame&&r.world_generation==f.key.world_generation&&
            same(r.world,f.world)&&same(r.controller,f.controller)&&same(r.pawn,f.pawn)&&same(r.mesh,f.mesh)&&same(r.weapon,f.weapon)&&
            r.reference_generation==f.reference_generation&&equal_bytes(r.reference,f.reference)&&
            equal_bytes(r.stereo,f.stereo)&&equal_bytes(r.controllers,f.controllers)&&main_mount_body(s.basis.geometry,r);
    }
    bool body_mount_response(std::uint32_t raw,std::int64_t now) const noexcept {
        auto const& outer=body_mount_work_.response;auto const& q=body_mount_work_.shadow;auto const& f=body_mount_work_.committed;
        if(!mount::status_valid(raw)||outer.size!=sizeof outer||outer.abi_version!=body_mount::version)return false;
        if(raw!=unsigned(mount::Status::ready))return equal_bytes(outer,empty_body_mount_response_);
        auto const& r=outer.mount;auto const& b=q.source.basis.geometry;
        if(outer.query_id!=q.query_id||outer.hand_lifecycle!=q.hand_lifecycle||outer.action_generation!=q.action_generation||
           !body_mount::same(outer.source,body_mount::source_key(q.source))||
           outer.equipment_begin_ns<=0||outer.equipment_end_ns<outer.equipment_begin_ns||now<outer.equipment_end_ns||
           r.size!=sizeof r||r.abi_version!=mount::version)return false;
        gears_weapon_lancer_wrist_mount::Profile profile{};
        if(!gears_weapon_lancer_wrist_mount::marcus115(b.geometry,b.model_scale,b.units_per_metre,profile))return false;
        auto const& c=profile.mount.calibration;
        return r.query_id==q.query_id&&same(r.key,f.key)&&r.key.lifecycle==q.hand_lifecycle&&same(r.world,f.world)&&same(r.controller,f.controller)&&same(r.pawn,f.pawn)&&
            same(r.mesh,f.mesh)&&same(r.weapon,f.weapon)&&identity_valid(r.weapon)&&identity_valid(r.component)&&identity_valid(r.asset)&&r.component.outer==r.weapon.address&&
            r.hold_generation&&r.provider_revision==mount::provider_revision&&r.body_revision==b.revision&&r.basis_revision==b.basis_revision&&r.basis==b.basis&&
            !r.reserved&&r.geometry==b.geometry&&r.model_scale==b.model_scale&&r.units_per_metre==b.units_per_metre&&
            r.calibration_known==1&&!r.calibration_reserved&&r.grip_to_wrist_metres.x==c.grip_to_wrist_metres.x&&r.grip_to_wrist_metres.y==c.grip_to_wrist_metres.y&&r.grip_to_wrist_metres.z==c.grip_to_wrist_metres.z&&
            r.grip_to_wrist_rotation.x==c.grip_to_wrist_rotation.x&&r.grip_to_wrist_rotation.y==c.grip_to_wrist_rotation.y&&r.grip_to_wrist_rotation.z==c.grip_to_wrist_rotation.z&&r.grip_to_wrist_rotation.w==c.grip_to_wrist_rotation.w;
    }
    bool backend_valid() const noexcept {return backend_.pin&&backend_.unpin&&backend_.clock_ns&&backend_.thread&&backend_.prepare&&backend_.commit&&backend_.invalidate;}
    bool session_valid() const noexcept {return session_.game_pid&&session_.game_start&&session_.native_started_qpc&&session_.owner_worker_thread;}
    bool bump_locked() noexcept {
        const auto old=revision_.load(std::memory_order_relaxed);
        if(old==(std::numeric_limits<std::uint64_t>::max)()){
            stopping_=true;state_.store(State::draining);authorization_.store(false);return false;
        }
        revision_.store(old+1,std::memory_order_release);return true;
    }
    void refresh_locked() noexcept {
        if(state_.load()==State::draining&&!callbacks_.load()&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load()&&!action_pending()&&invalidate_done_&&!invalidate_running_&&!final_invalidate_)
            state_.store(State::quiescent,std::memory_order_release);
        const auto s=state_.load();
        availability_.store(s==State::empty&&!aux_scopes_.load()&&!aux_debt_.load()&&!action_pending()?Availability::absent_quiescent:
            s==State::active&&!phase_invalid_&&!action_debt_.load()&&!revocation_.debt()?Availability::ready:Availability::unavailable,std::memory_order_release);
    }
    void invalidate_locked(InvalidateReason reason,bool hard,InvalidateCall& call,bool retry=false,bool action_safe_frame=false) noexcept {
        const auto s=state_.load();authorization_.store(false,std::memory_order_release);
        if(hard&&s==State::active)(void)revocation_.publish({});
        if(s==State::empty||s==State::quiescent||s==State::collecting)return;
        if(!action_safe_frame&&(action_scopes_.load()||external_scopes_.load())){
            action_invalidation_pending_=true;action_invalidation_pending_snapshot_.store(true);invalidate_done_=false;
        }
        if(s==State::registering){cancel_registration_=true;phase_invalid_=true;bump_locked();refresh_locked();return;}
        const bool first=!phase_invalid_;
        if(first){phase_invalid_=true;invalidate_done_=false;invalidate_failed_=false;phase_reason_=reason;bump_locked();}
        else if(hard&&s==State::active){phase_reason_=reason;bump_locked();}
        // A recovering prepare can still be inside the prior unavailable
        // phase. Cancel that operation even though the public revision does
        // not advance a second time for the same failure phase.
        if(first||normal_calls_){
            if(operation_epoch_!=(std::numeric_limits<std::uint64_t>::max)())++operation_epoch_;
            else {stopping_=true;hard=true;}
        }
        if(normal_calls_){final_invalidate_=true;invalidate_done_=false;phase_reason_=reason;}
        if(hard)state_.store(State::draining,std::memory_order_release);
        if(published_&&!action_invalidation_pending_&&!invalidate_done_&&!invalidate_running_&&(!invalidate_failed_||retry)){
            call.registration=registration_;call.message.registration_cookie=cookie_.load();
            call.message.native_started_qpc=session_.native_started_qpc;call.message.reason=phase_reason_;call.needed=true;
            if(!normal_calls_)final_invalidate_=false;
            invalidate_running_=true;++callbacks_;
        }
        // Never clear the prepare->arbiter->commit obligation here. A void
        // invalidate only disarms publication; it proves no GT cleanup.
        refresh_locked();
    }
    void run_invalidate(InvalidateCall const& call) {
        auto current=call;
        while(current.needed){
            auto message=current.message;bool ok{},violated{};
            {CallbackMark mark(this);ok=backend_.invalidate(backend_.context,current.registration,&message);violated=mark.violated;}
            InvalidateCall final_call{};
            {
                std::lock_guard<std::mutex> lock(mutex_);--callbacks_;invalidate_running_=false;
                if(!ok||violated||!equal_bytes(message,current.message)){
                    authorization_.store(false,std::memory_order_release);
                    if(state_.load()==State::active)bump_locked();
                    state_.store(State::draining,std::memory_order_release);phase_invalid_=true;invalidate_failed_=true;invalidate_done_=false;
                }else {
                    invalidate_failed_=false;invalidate_done_=!final_invalidate_;
                    if(final_invalidate_&&!normal_calls_)invalidate_locked(phase_reason_,state_.load()==State::draining,final_call);
                }
                refresh_locked();
            }
            // At most one final repeat for an invalidation overlapping the
            // synchronous callback. No polling/waiting or participant retry.
            current=final_call;
        }
    }
    bool key_session(FrameKey const& key) const noexcept {
        return key.game_pid==session_.game_pid&&key.game_start==session_.game_start&&key.native_started_qpc==session_.native_started_qpc;
    }
    bool current_thread(Frame const& f) const noexcept {
        const auto id=backend_.thread(backend_.context);
        return id&&id!=session_.owner_worker_thread&&f.key.thread_id==id&&(!game_thread_||game_thread_==id);
    }
    bool active_now(Frame const& f,std::int64_t stamp) const noexcept {
        return stamp>0&&stamp>=f.now_ns&&stamp-f.now_ns<=100000000&&frame_active(f,stamp);
    }
    static bool proposal_valid_here(Proposal const& p,Frame const& f) noexcept {
        if(p.size!=sizeof(Proposal)||p.abi_version!=version||!same(p.key,f.key)||!p.nonce||p.support>1||p.ammo>1||p.magazine>1||p.reserved)return false;
        for(auto const& lease:p.incumbents)if(!lease_valid(lease,f.key.lifecycle)||lease.kind==LeaseKind::cover)return false;
        return p.incumbents[0].kind==LeaseKind::none||p.incumbents[1].kind==LeaseKind::none||!same(p.incumbents[0],p.incumbents[1]);
    }
    static bool decision_valid(Decision const& d,Frame const& f,Proposal const& p,std::int64_t stamp) noexcept {
        if(d.size!=sizeof(Decision)||d.abi_version!=version||!same(d.key,f.key)||d.proposal_nonce!=p.nonce||d.press_token!=f.press_token||
           d.reserved||d.new_grant>1||d.release_required>1||unsigned(d.status)>unsigned(DecisionStatus::lease_lost)||
           !lease_valid(d.lease,f.key.lifecycle)||d.now_ns<f.now_ns||d.now_ns>stamp)return false;
        const bool selected=d.status==DecisionStatus::granted||d.status==DecisionStatus::retained;
        if(selected!=(d.lease.kind!=LeaseKind::none)||bool(d.new_grant)!=(d.status==DecisionStatus::granted))return false;
        if(selected&&(!f.left_grip_down||!f.left_grip_known||!f.left_tracked||d.release_required))return false;
        if(d.new_grant){
            if(d.lease.kind==LeaseKind::other)return false;
            if(d.lease.kind!=LeaseKind::cover&&d.lease.id!=f.press_token)return false;
            if((d.lease.kind==LeaseKind::support&&!p.support)||(d.lease.kind==LeaseKind::ammo&&!p.ammo)||(d.lease.kind==LeaseKind::magazine&&!p.magazine))return false;
        }else if(selected&&d.lease.kind!=LeaseKind::cover){
            if(!same(d.lease,p.incumbents[0])&&!same(d.lease,p.incumbents[1]))return false;
        }
        return true;
    }
    void auxiliary_unknown() noexcept {
        // Own lock failures / corrupted Native token cannot be certified away.
        aux_debt_.store(true,std::memory_order_release);authorization_.store(false,std::memory_order_release);
        state_.store(State::draining,std::memory_order_release);availability_.store(Availability::unavailable,std::memory_order_release);
    }
    void auxiliary_terminal() noexcept {
        aux_debt_.store(true,std::memory_order_release);
        try {InvalidateCall invalidation{};
            {std::lock_guard<std::mutex> lock(mutex_);invalidate_locked(InvalidateReason::callback_fault,true,invalidation);refresh_locked();}
            run_invalidate(invalidation);
        }catch(...){auxiliary_unknown();}
    }
    // kind: 0 pre, 1 returned, 2 unwind. No lock/CallbackMark spans original.
    bool call_notice(CallScope& scope,unsigned kind,std::uint64_t rax,call::UnwindReason reason,std::uint32_t& end) noexcept {
        bool counted=false;
        try {
            {std::lock_guard<std::mutex> lock(mutex_);++callbacks_;++normal_calls_;counted=true;
                if(phase_invalid_){final_invalidate_=true;invalidate_done_=false;}}
            snapshot(scope.notice,scope.invocation);bool ok{},violated{};
            {CallbackMark mark(this);
                if(kind==0)ok=backend_.call_begin(backend_.context,scope.registration,&scope.notice,&scope.state);
                else if(kind==1)ok=backend_.call_returned(backend_.context,scope.registration,&scope.notice,&scope.state,rax,end);
                else ok=backend_.call_unwind(backend_.context,scope.registration,&scope.notice,&scope.state,reason,end);
                violated=mark.violated;
            }
            const bool valid=ok&&!violated&&call::same(scope.notice,scope.invocation)&&(kind==0||end<=unsigned(call::End::unresolved));
            InvalidateCall invalidation{};
            {std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;counted=false;
                if(!valid){scope.notification_fault=true;aux_debt_.store(true);invalidate_locked(InvalidateReason::callback_fault,true,invalidation);}
                if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)
                    invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
                refresh_locked();}
            run_invalidate(invalidation);return valid;
        }catch(...){
            // A failed own accounting lock leaves an explicit permanent debt;
            // never invent a balanced decrement after an uncertain operation.
            (void)counted;scope.notification_fault=true;auxiliary_unknown();return false;
        }
    }
    // Gate begin has a dedicated restore phase. Only this phase may recursively
    // enter the sole CF wrapper; ordinary V1/frame/invalidate callbacks cannot.
    bool action_notice(ActionScope& scope,unsigned kind,std::uint64_t rax,action::UnwindReason reason,std::uint32_t& reply) noexcept {
        try {
            {std::lock_guard<std::mutex> lock(mutex_);++callbacks_;++normal_calls_;
                if(phase_invalid_){final_invalidate_=true;invalidate_done_=false;}}
            snapshot(scope.notice,scope.invocation);bool ok{},violated{};
            {CallbackMark mark(this,kind==0&&(scope.invocation.flags&action::engine_calls_allowed)!=0);
                if(kind==0)ok=backend_.action_begin(backend_.context,scope.registration,&scope.notice,&scope.state,reply);
                else if(kind==1)ok=backend_.action_returned(backend_.context,scope.registration,&scope.notice,&scope.state,rax,reply);
                else ok=backend_.action_unwind(backend_.context,scope.registration,&scope.notice,&scope.state,reason,reply);
                violated=mark.violated;
            }
            const bool valid=ok&&!violated&&action::same(scope.notice,scope.invocation)&&
                (kind==0?action::begin_reply_valid(static_cast<action::BeginReply>(reply)):reply<=unsigned(action::End::unresolved));
            InvalidateCall invalidation{};
            {std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;
                const bool unresolved=kind==0?reply==unsigned(action::BeginReply::handback_unresolved):reply!=unsigned(action::End::closed);
                if(kind==0&&(!valid||action::action_seen(static_cast<action::BeginReply>(reply)))){
                    scope.revoke_owner_cache=true;authorization_.store(false);
                    (void)revocation_.publish(state_.load()==State::active&&!stopping_?action_auth():boundary_state::Auth{});
                    invalidate_locked(InvalidateReason::context_lost,false,invalidation);
                }
                if(kind==0&&valid&&reply==unsigned(action::BeginReply::unrelated)&&scope.barrier_active){
                    scope.barrier_active=false;--action_barriers_;
                }
                if(!valid||unresolved){scope.notification_fault|=!valid;action_debt_.store(true);revocation_.poison();
                    invalidate_locked(InvalidateReason::callback_fault,true,invalidation);}
                if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)
                    invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
                refresh_locked();}
            // No Broker mutex or dedicated Action callback phase spans this.
            run_invalidate(invalidation);return valid;
        }catch(...){scope.notification_fault=true;scope.revoke_owner_cache=true;action_unknown();return false;}
    }
public:
    // Construct on a stable Native session; the explicit owner worker is the
    // ONLY thread allowed to release a module ref. No destructor auto-unpins.
    explicit Broker(BrokerSession session,BrokerBackend backend=weapon_hand_win32_backend()):session_(session),backend_(backend){}
    Broker(Broker const&)=delete;Broker& operator=(Broker const&)=delete;
    ~Broker()=default; // Caller must keep this object/context alive through drain+collect.
    std::uint64_t lifecycle_revision() const noexcept {return revision_.load(std::memory_order_acquire);}
    std::uint64_t cookie() const noexcept {return cookie_.load(std::memory_order_acquire);}
    // Pure registration receipt, not hand/frame authorization. A reserved cookie
    // is visible while registering, before output publication or even failure.
    // State::active is published only after a successful result copy. SC loads
    // bracket that transition with the nonwrapping cookie/lifecycle counters.
    bool registration_current(std::uint64_t expected_cookie,std::uint64_t native_started_qpc) const noexcept {
        if(!expected_cookie||!native_started_qpc||native_started_qpc!=session_.native_started_qpc)return false;
        const auto revision=revision_.load(std::memory_order_seq_cst);
        if(state_.load(std::memory_order_seq_cst)!=State::active||cookie_.load(std::memory_order_seq_cst)!=expected_cookie)return false;
        return state_.load(std::memory_order_seq_cst)==State::active&&cookie_.load(std::memory_order_seq_cst)==expected_cookie&&
            revision_.load(std::memory_order_seq_cst)==revision;
    }
    Availability availability() const noexcept {return availability_.load(std::memory_order_acquire);}
    bool authorized() const noexcept {return authorization_.load(std::memory_order_acquire)&&!action_barrier()&&
        revocation_.frame_current({action_auth(),authorized_frame_.load(),authorized_generation_.load()});}
    bool action_barrier() const noexcept {return action_barriers_.load(std::memory_order_acquire)||external_scopes_.load(std::memory_order_acquire)||action_debt_.load()||revocation_.debt();}
    std::uint64_t action_generation() const noexcept {return revocation_.generation();}
    unsigned pending_action_scopes() const noexcept {return action_scopes_.load();}
    unsigned pending_external_scopes() const noexcept {return external_scopes_.load();}
    bool action_debt() const noexcept {return action_debt_.load()||revocation_.debt();}
    bool pending_action_invalidation() const noexcept {return action_invalidation_pending_snapshot_.load();}
    bool action_attached() const noexcept {return action_attached_.load();}
    // Startup only; no PG-facing operation can enable this permission.
    bool configure_engine_restore(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);if(state_.load()!=State::empty||stopping_)return false;
        engine_restore_enabled_=enabled;return true;
    }
    unsigned pending_callbacks() const noexcept {return callbacks_.load(std::memory_order_acquire);}
    unsigned pending_transactions() const noexcept {return transactions_.load(std::memory_order_acquire);}
    unsigned pending_aux_scopes() const noexcept {return aux_scopes_.load(std::memory_order_acquire);}
    bool aux_debt() const noexcept {return aux_debt_.load(std::memory_order_acquire);}
    bool call_attached() const noexcept {return call_attached_.load(std::memory_order_acquire);}
    bool pin_retained() const noexcept {return pin_retained_.load(std::memory_order_acquire);}
    // Transport quiescence only: no callback or frozen-frame obligation. The
    // void atomic invalidate is NOT a receipt for PG's game-resource restore.
    bool quiescent() const noexcept {const auto s=state_.load();return (s==State::empty||s==State::quiescent)&&!callbacks_.load()&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load()&&!action_pending();}
    bool draining() const noexcept {const auto s=state_.load();return s==State::draining||s==State::quiescent||s==State::collecting||action_debt();}
    bool pending() const noexcept {const auto s=state_.load();return callbacks_.load()||transactions_.load()||aux_scopes_.load()||aux_debt_.load()||action_pending()||(s!=State::empty&&s!=State::active);}

    Reply attach_main_mount_provider(mount::Registration const* input,RegistrationResult* output,bool consumer_owned) {
        if(reentrant())return Reply::reentrant;
        mount::Registration r{};RegistrationResult result{};
        if(!consumer_owned||!weapon_broker_win32::copy(&r,input,sizeof r)||!weapon_broker_win32::copy(&result,output,sizeof result)||
           !mount::registration_shape_valid(r)||result.size!=sizeof result||result.abi_version!=version||!backend_.main_mount_module||!backend_.main_mount_prepare)return Reply::invalid;
        std::lock_guard<std::mutex> lock(mutex_);
        if(!published_||r.hand_cookie!=cookie_.load()||r.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
        if(stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return Reply::draining;
        if(r.context!=registration_.context||!backend_.main_mount_module(backend_.context,r,pin_))return Reply::invalid;
        if(main_mount_attached_&&!mount::same(r,main_mount_registration_))return Reply::busy;
        result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
        if(!weapon_broker_win32::copy(output,&result,sizeof result))return Reply::invalid;
        main_mount_registration_=r;main_mount_attached_=true;return Reply::accepted;
    }
    MainMountQueryResult query_main_mount(rig::Frame const& retained,mount::BodyGeometry const& body,mount::Response& output) noexcept {
        using Code=MainMountQueryResult::Code;output={};MainMountQueryResult result{};InvalidateCall invalidation{};bool counted{};
        if(current_){for(auto* p=current_;p;p=p->previous)p->violated=true;result.code=Code::fault;result.branch=MainMountQueryResult::Branch::reentrant;return result;}
        mount::Registration registration{};std::uint64_t revision{},epoch{},generation{},parent_cookie{};
        try{
            {std::lock_guard<std::mutex> lock(mutex_);
                if(!main_mount_attached_||!published_||stopping_||state_.load()!=State::active||!have_commit_||phase_invalid_)return result;
                if(!main_mount_idle_locked()||!revocation_.frame_current(frame_stamp_)){result.code=Code::context_rejected;result.branch=MainMountQueryResult::Branch::context;return result;}
                const auto now=backend_.clock_ns(backend_.context);
                if(!main_mount_source(retained,frozen_,now)){result.code=Code::stale;result.branch=MainMountQueryResult::Branch::source_before;observe_main_mount_source(retained,frozen_,now,result);return result;}
                if(!main_mount_body(body,retained)){result.code=Code::context_rejected;result.branch=MainMountQueryResult::Branch::body;return result;}
                if(next_main_mount_query_==(std::numeric_limits<std::uint64_t>::max)()){
                    invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=MainMountQueryResult::Branch::query_limit;
                }else{
                    auto& w=main_mount_work_;w.request={};w.request.query_id=next_main_mount_query_++;w.request.hand_lifecycle=frozen_.key.lifecycle;
                    snapshot(w.request.retained,retained);snapshot(w.request.body,body);snapshot(w.shadow,w.request);snapshot(w.committed,frozen_);w.response={};
                    registration=main_mount_registration_;parent_cookie=cookie_.load();revision=revision_.load();epoch=operation_epoch_;generation=revocation_.generation();
                    ++callbacks_;++normal_calls_;counted=true;result.query_id=w.request.query_id;
                }
            }
            if(!counted){run_invalidate(invalidation);return result;}
            bool okay{},violated{};std::uint32_t raw=UINT32_MAX;
            {CallbackMark mark(this);okay=!mark.violated&&backend_.main_mount_prepare(backend_.context,registration,&main_mount_work_.request,&main_mount_work_.response,raw);violated=mark.violated;}
            result.callback_status=raw;
            {std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;counted=false;
                const bool malformed=!okay||violated||!main_mount_context_clear()||!equal_bytes(main_mount_work_.request,main_mount_work_.shadow)||!main_mount_response(raw);
                if(malformed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=MainMountQueryResult::Branch::callback_fault;}
                else if(stopping_||state_.load()!=State::active||cookie_.load()!=parent_cookie||revision_.load()!=revision||operation_epoch_!=epoch||revocation_.generation()!=generation||
                    !have_commit_||phase_invalid_||!main_mount_idle_locked()||!revocation_.frame_current(frame_stamp_)||!same(frozen_.key,main_mount_work_.committed.key)||!equal_bytes(frozen_,main_mount_work_.committed)){result.code=Code::unavailable;result.branch=MainMountQueryResult::Branch::changed;}
                else{
                    const auto now=backend_.clock_ns(backend_.context);
                    if(!main_mount_source(main_mount_work_.shadow.retained,frozen_,now)){result.code=Code::stale;result.branch=MainMountQueryResult::Branch::source_after;observe_main_mount_source(main_mount_work_.shadow.retained,frozen_,now,result);}
                    else{result.status=static_cast<mount::Status>(raw);result.code=result.status==mount::Status::ready?Code::ready:Code::not_ready;result.branch=MainMountQueryResult::Branch::reply;
                        if(result.code==Code::ready)snapshot(output,main_mount_work_.response);}
                }
                if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
                refresh_locked();
            }
            run_invalidate(invalidation);return result;
        }catch(...){
            try{std::lock_guard<std::mutex> lock(mutex_);if(counted){--callbacks_;--normal_calls_;}invalidate_locked(InvalidateReason::callback_fault,true,invalidation);refresh_locked();}catch(...){}
            try{run_invalidate(invalidation);}catch(...){}output={};result.code=Code::fault;result.branch=MainMountQueryResult::Branch::exception;return result;
        }
    }

    Reply attach_body_mount_provider(body_mount::Registration const* input,RegistrationResult* output,bool consumer_owned) {
        if(reentrant())return Reply::reentrant;
        body_mount::Registration r{};RegistrationResult result{};
        if(!consumer_owned||!weapon_broker_win32::copy(&r,input,sizeof r)||!weapon_broker_win32::copy(&result,output,sizeof result)||
           !body_mount::registration_shape_valid(r)||result.size!=sizeof result||result.abi_version!=version||
           !backend_.body_mount_module||!backend_.body_mount_prepare||!backend_.body_mount_current)return Reply::invalid;
        std::lock_guard<std::mutex> lock(mutex_);
        if(!published_||r.hand_cookie!=cookie_.load()||r.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
        if(stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return Reply::draining;
        if(r.context!=registration_.context||!backend_.body_mount_module(backend_.context,r,pin_))return Reply::invalid;
        if(body_mount_attached_&&!body_mount::same(r,body_mount_registration_))return Reply::busy;
        result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
        if(!weapon_broker_win32::copy(output,&result,sizeof result))return Reply::invalid;
        body_mount_registration_=r;body_mount_attached_=true;return Reply::accepted;
    }
    MainMountQueryResult query_body_mount(body_mount::Source const& source,body_mount::Response& output) noexcept {
        using Code=MainMountQueryResult::Code;output={};MainMountQueryResult result{};InvalidateCall invalidation{};bool counted{};
        if(current_){for(auto* p=current_;p;p=p->previous)p->violated=true;result.code=Code::fault;result.branch=MainMountQueryResult::Branch::reentrant;return result;}
        body_mount::Registration registration{};std::uint64_t revision{},epoch{},generation{},parent_cookie{};
        try{
            {std::lock_guard<std::mutex> lock(mutex_);
                if(!body_mount_attached_||!published_||stopping_||state_.load()!=State::active||!have_commit_||phase_invalid_||!backend_.body_mount_current)return result;
                if(!main_mount_idle_locked()||!revocation_.frame_current(frame_stamp_)){result.code=Code::context_rejected;result.branch=MainMountQueryResult::Branch::context;return result;}
                if(!body_mount_parent(source,frozen_,backend_.clock_ns(backend_.context))){result.code=Code::stale;result.branch=MainMountQueryResult::Branch::source_before;return result;}
                if(next_body_mount_query_==(std::numeric_limits<std::uint64_t>::max)()){
                    invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=MainMountQueryResult::Branch::query_limit;
                }else{
                    auto& w=body_mount_work_;w.request={};w.request.query_id=next_body_mount_query_++;w.request.hand_lifecycle=frozen_.key.lifecycle;
                    w.request.action_generation=revocation_.generation();snapshot(w.request.source,source);snapshot(w.shadow,w.request);snapshot(w.committed,frozen_);w.response={};
                    registration=body_mount_registration_;parent_cookie=cookie_.load();revision=revision_.load();epoch=operation_epoch_;generation=revocation_.generation();
                    ++callbacks_;++normal_calls_;counted=true;result.query_id=w.request.query_id;
                }
            }
            if(!counted){run_invalidate(invalidation);return result;}
            // Reserved lifetime prevents collect/unpin; these source checks
            // must run OUTSIDE mutex and must not demand zero callbacks from
            // their own exact reservation. They do not grant a pose writer.
            const bool source_before=backend_.body_mount_current(backend_.context,body_mount_work_.shadow.source);
            bool okay=true,violated=false;std::uint32_t raw=unsigned(mount::Status::source_mismatch);
            if(source_before){CallbackMark mark(this);okay=!mark.violated&&backend_.body_mount_prepare(backend_.context,registration,&body_mount_work_.request,&body_mount_work_.response,raw);violated=mark.violated;}
            const bool source_after=source_before&&backend_.body_mount_current(backend_.context,body_mount_work_.shadow.source);
            result.callback_status=source_before?raw:UINT32_MAX;
            {std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;counted=false;
                const auto now=backend_.clock_ns(backend_.context);
                const bool malformed=!okay||violated||!main_mount_context_clear()||!equal_bytes(body_mount_work_.request,body_mount_work_.shadow)||
                    (source_before&&!body_mount_response(raw,now));
                if(malformed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=MainMountQueryResult::Branch::callback_fault;}
                else if(stopping_||state_.load()!=State::active||cookie_.load()!=parent_cookie||revision_.load()!=revision||operation_epoch_!=epoch||revocation_.generation()!=generation||
                    !have_commit_||phase_invalid_||!main_mount_idle_locked()||!revocation_.frame_current(frame_stamp_)||!same(frozen_.key,body_mount_work_.committed.key)||!equal_bytes(frozen_,body_mount_work_.committed)){result.code=Code::unavailable;result.branch=MainMountQueryResult::Branch::changed;}
                else if(!source_before||!source_after){result.code=Code::context_rejected;result.branch=source_before?MainMountQueryResult::Branch::source_after:MainMountQueryResult::Branch::source_before;}
                else if(!body_mount_parent(body_mount_work_.shadow.source,frozen_,now)){result.code=Code::stale;result.branch=MainMountQueryResult::Branch::source_after;}
                else if(raw==unsigned(mount::Status::ready)&&std::uint64_t(now-body_mount_work_.response.equipment_begin_ns)>body_mount::equipment_age_ns){result.code=Code::stale;result.branch=MainMountQueryResult::Branch::source_after;}
                else{result.status=static_cast<mount::Status>(raw);result.code=result.status==mount::Status::ready?Code::ready:Code::not_ready;result.branch=MainMountQueryResult::Branch::reply;
                    if(result.code==Code::ready)snapshot(output,body_mount_work_.response);}
                if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
                refresh_locked();
            }
            run_invalidate(invalidation);return result;
        }catch(...){
            try{std::lock_guard<std::mutex> lock(mutex_);if(counted){--callbacks_;--normal_calls_;}invalidate_locked(InvalidateReason::callback_fault,true,invalidation);refresh_locked();}catch(...){}
            try{run_invalidate(invalidation);}catch(...){}output={};result.code=Code::fault;result.branch=MainMountQueryResult::Branch::exception;return result;
        }
    }

    Reply attach_call_participant(call::Registration const* input,RegistrationResult* output,bool wrapper_owned) {
        if(reentrant())return Reply::reentrant;
        call::Registration r{};RegistrationResult result{};
        if(!wrapper_owned||!weapon_broker_win32::copy(&r,input,sizeof(r))||!weapon_broker_win32::copy(&result,output,sizeof(result))||
           !call::registration_shape_valid(r)||result.size!=sizeof(result)||result.abi_version!=version||
           !backend_.call_module||!backend_.call_begin||!backend_.call_returned||!backend_.call_unwind)return Reply::invalid;
        std::lock_guard<std::mutex> lock(mutex_);
        if(!published_||r.hand_cookie!=cookie_.load()||r.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
        if(stopping_||state_.load()!=State::active||aux_debt_.load())return Reply::draining;
        if(r.context!=registration_.context||!backend_.call_module(backend_.context,r,pin_))return Reply::invalid;
        if(call_attached_.load()&&!call::same(r,call_registration_))return Reply::busy;
        result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
        if(!weapon_broker_win32::copy(output,&result,sizeof(result)))return Reply::invalid;
        call_registration_=r;call_attached_.store(true,std::memory_order_release);return Reply::accepted;
    }

    bool begin_call(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,CallScope& scope,ActionScope const* gate=nullptr) noexcept {
        if(call_reentrant()){auxiliary_unknown();return false;}
        try {
            bool bad=false;
            {std::lock_guard<std::mutex> lock(mutex_);
                const bool admitted_gate=gate&&gate->active&&gate->owner==this&&gate==action_top_&&
                    gate->address==reinterpret_cast<std::uintptr_t>(gate)&&gate->thread==backend_.thread(backend_.context);
                if(!admitted_gate&&(!published_||!call_attached_.load()||stopping_||aux_debt_.load()||state_.load()!=State::active))return false;
                if(scope.active||scope.owner||scope.address||scope.token)bad=true;
                unsigned depth=0;for(auto* p=call_top_;p;p=p->previous){if(p==&scope||++depth>=64){bad=true;break;}}
                if(bad){/* Preserve the prior token; it remains owed. */}
                else if(!backend_.thread(backend_.context)||next_call_id_==(std::numeric_limits<std::uint64_t>::max)()||
                        aux_scopes_.load()==(std::numeric_limits<unsigned>::max)())bad=true;
                else {
                    scope={};scope.owner=this;scope.previous=call_top_;scope.address=reinterpret_cast<std::uintptr_t>(&scope);
                    scope.token=next_call_id_++;scope.thread=backend_.thread(backend_.context);scope.registration=admitted_gate?gate->call_registration:call_registration_;
                    scope.invocation.hand_cookie=cookie_.load();scope.invocation.native_started_qpc=session_.native_started_qpc;
                    scope.invocation.call_id=scope.token;scope.invocation.thread_id=scope.thread;
                    scope.invocation.self=self;scope.invocation.caller_frame=caller_frame;scope.invocation.result=result;scope.invocation.callee=callee;
                    scope.active=scope.attempted=true;call_top_=&scope;++aux_scopes_;
                }
            }
            if(bad){auxiliary_terminal();return false;}
            std::uint32_t ignored{};(void)call_notice(scope,0,0,call::UnwindReason::notification_fault,ignored);return true;
        }catch(...){auxiliary_unknown();return scope.active&&scope.owner==this&&scope.address==reinterpret_cast<std::uintptr_t>(&scope);}
    }

    void finish_call(CallScope& scope,bool original_returned,std::uint64_t rax) noexcept {
        if(!scope.active)return;
        if(call_reentrant()){auxiliary_unknown();return;}
        try {
            if(scope.owner!=this||scope.address!=reinterpret_cast<std::uintptr_t>(&scope)||!scope.token||
               scope.token!=scope.invocation.call_id||scope.thread!=backend_.thread(backend_.context)||call_top_!=&scope){auxiliary_terminal();return;}
            std::uint32_t end=unsigned(call::End::unresolved);bool valid{};
            if(scope.notification_fault)valid=call_notice(scope,2,0,call::UnwindReason::notification_fault,end);
            else if(original_returned){
                valid=call_notice(scope,1,rax,call::UnwindReason::original_exception,end);
                if(!valid){std::uint32_t ignored=unsigned(call::End::unresolved);(void)call_notice(scope,2,0,call::UnwindReason::notification_fault,ignored);}
            }else valid=call_notice(scope,2,0,call::UnwindReason::original_exception,end);
            if(!valid||scope.notification_fault||end!=unsigned(call::End::closed))auxiliary_terminal();
            {std::lock_guard<std::mutex> lock(mutex_);call_top_=scope.previous;scope.active=false;--aux_scopes_;refresh_locked();}
            // Keep owner/token as a consumed receipt: accidental reuse is not a
            // second pre or a second end. A new wrapper invocation uses fresh {}.
        }catch(...){auxiliary_unknown();}
    }

    Reply attach_action_gate(action::Registration const* input,RegistrationResult* output,bool wrapper_owned) {
        if(reentrant())return Reply::reentrant;
        action::Registration r{};RegistrationResult result{};
        if(!wrapper_owned||!weapon_broker_win32::copy(&r,input,sizeof(r))||!weapon_broker_win32::copy(&result,output,sizeof(result))||
           !action::registration_shape_valid(r)||result.size!=sizeof(result)||result.abi_version!=version||
           !backend_.action_module||!backend_.action_begin||!backend_.action_returned||!backend_.action_unwind)return Reply::invalid;
        std::lock_guard<std::mutex> lock(mutex_);
        if(!published_||r.hand_cookie!=cookie_.load()||r.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
        if(stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return Reply::draining;
        if(!call_attached_.load()||r.context!=registration_.context||!backend_.action_module(backend_.context,r,pin_))return Reply::invalid;
        if(action_attached_.load()&&!action::same(r,action_registration_))return Reply::busy;
        result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
        if(!weapon_broker_win32::copy(output,&result,sizeof(result)))return Reply::invalid;
        action_registration_=r;action_attached_.store(true,std::memory_order_release);return Reply::accepted;
    }
    bool begin_action(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,ActionScope& scope) noexcept {
        if(call_reentrant()){action_unknown();return false;}
        try {
            bool bad=false;
            {std::lock_guard<std::mutex> lock(mutex_);
                if(!published_||!action_attached_.load()||stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return false;
                if(scope.active||scope.owner||scope.address||scope.token)bad=true;
                unsigned depth=0;for(auto* p=action_top_;p;p=p->previous)if(p==&scope||++depth>=64){bad=true;break;}
                const auto thread=backend_.thread(backend_.context);
                if(!thread||next_action_id_==(std::numeric_limits<std::uint64_t>::max)()||
                   action_scopes_.load()==(std::numeric_limits<unsigned>::max)())bad=true;
                if(!bad){
                    const bool engine=classify_action_context(context::thread_ledger,thread==game_thread_&&thread!=session_.owner_worker_thread,
                        !action_top_&&!call_top_&&!current_&&!callbacks_.load()&&!transactions_.load(),engine_restore_enabled_);
                    scope={};scope.owner=this;scope.previous=action_top_;scope.address=reinterpret_cast<std::uintptr_t>(&scope);
                    scope.token=next_action_id_++;scope.thread=thread;scope.registration=action_registration_;scope.call_registration=call_registration_;
                    scope.invocation.hand_cookie=cookie_.load();scope.invocation.native_started_qpc=session_.native_started_qpc;
                    scope.invocation.gate_id=scope.token;scope.invocation.thread_id=thread;scope.invocation.flags=engine?action::engine_calls_allowed:0;
                    scope.invocation.self=self;scope.invocation.caller_frame=caller_frame;scope.invocation.result=result;scope.invocation.callee=callee;
                    if(!context::enter(context::thread_ledger,scope.ticket,context::Kind::action))bad=true;
                    else {scope.active=scope.attempted=scope.barrier_active=true;action_top_=&scope;++action_scopes_;++action_barriers_;}
                }
            }
            if(bad){action_unknown();return false;}
            std::uint32_t reply=unsigned(action::BeginReply::handback_unresolved);
            (void)action_notice(scope,0,0,action::UnwindReason::notification_fault,reply);return true;
        }catch(...){action_unknown();return scope.active&&scope.owner==this&&scope.address==reinterpret_cast<std::uintptr_t>(&scope);}
    }
    void finish_action(ActionScope& scope,bool original_returned,std::uint64_t rax) noexcept {
        if(!scope.active)return;if(call_reentrant()){action_unknown();return;}
        try {
            if(scope.owner!=this||scope.address!=reinterpret_cast<std::uintptr_t>(&scope)||scope.token!=scope.invocation.gate_id||
               !scope.token||scope.thread!=backend_.thread(backend_.context)||action_top_!=&scope||context::thread_ledger.top!=&scope.ticket){action_unknown();return;}
            std::uint32_t reply=unsigned(action::End::unresolved);bool valid{};
            if(scope.notification_fault)valid=action_notice(scope,2,0,action::UnwindReason::notification_fault,reply);
            else if(original_returned){valid=action_notice(scope,1,rax,action::UnwindReason::original_exception,reply);
                if(!valid){std::uint32_t ignored=unsigned(action::End::unresolved);(void)action_notice(scope,2,0,action::UnwindReason::notification_fault,ignored);}}
            else valid=action_notice(scope,2,0,action::UnwindReason::original_exception,reply);
            if(!valid||reply!=unsigned(action::End::closed))action_unknown();
            {std::lock_guard<std::mutex> lock(mutex_);
                if(!context::leave(context::thread_ledger,scope.ticket)){action_unknown();return;}
                action_top_=scope.previous;scope.active=false;--action_scopes_;
                if(scope.barrier_active){scope.barrier_active=false;--action_barriers_;}refresh_locked();}
        }catch(...){action_unknown();}
    }
    // The fast exported Revoke path: caller safe-copies POD. No lock, backend,
    // module query or callback; Native atomic auth is the sole binding proof.
    Reply revoke_action(boundary::Request const& request,std::uint64_t& generation) noexcept {
        const auto result=revocation_.revoke(request,generation);
        if(result==Reply::invalid)action_unknown();
        if(generation)authorization_.store(false,std::memory_order_release);return result;
    }
    void boundary_uncertain() noexcept {action_unknown();}
    Reply open_boundary(boundary::Request const& request,boundary::State& state,boundary::Receipt& receipt,std::uintptr_t address) {
        const boundary::State zero{};const bool dirty=!equal_bytes(state,zero);bool alias=false;
        for(auto const& item:boundary_addresses_)if(item.load(std::memory_order_acquire)==address)alias=true;
        const auto thread=backend_.thread(backend_.context);
        const bool gt=thread&&thread==certified_game_thread_.load()&&thread!=session_.owner_worker_thread;
        const bool obligations=callbacks_.load()||transactions_.load()||aux_scopes_.load()||action_scopes_.load();
        const bool clear=classify_action_context(context::thread_ledger,gt,!obligations,engine_restore_enabled_);
        // Unsafe contexts never acquire admission mutex; the Native atomic
        // address index excludes reopen even after PG zeroed its descriptor.
        if(!clear){state={};receipt={};
            if(dirty||alias||!address||(address&7)){action_unknown();return Reply::invalid;}
            if(!boundary::request_valid(request)){action_unknown();return Reply::invalid;}
            if(!revocation_.matches({request.hand_cookie,request.native_started_qpc,request.context}))return Reply::wrong_cookie;
            if(action_debt()||draining())return Reply::draining;
            return gt?(context::engine_context_clear(context::thread_ledger,true)&&obligations?Reply::busy:Reply::reentrant):Reply::unavailable;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if(!action_attached_.load()||state_.load()!=State::active||stopping_){
            // Pool still checks dirty/reopen before its clean NoAuth predicates.
            const auto reply=external_.open(request,state,receipt,{action_auth(),thread,false},context::thread_ledger,address);
            if(reply==Reply::invalid)action_unknown();return reply;
        }
        unsigned spare=unsigned(boundary_addresses_.size());
        for(unsigned i=0;i<boundary_addresses_.size();++i)if(!boundary_addresses_[i].load()){spare=i;break;}
        if(spare==boundary_addresses_.size()){state={};receipt={};if(dirty||alias){action_unknown();return Reply::invalid;}return Reply::busy;}
        const auto reply=external_.open(request,state,receipt,{action_auth(),thread,gt},context::thread_ledger,address);
        if(reply==Reply::accepted){boundary_ids_[spare]=receipt.scope_id;boundary_addresses_[spare].store(address,std::memory_order_release);++external_scopes_;}
        if(reply==Reply::invalid)action_unknown();refresh_locked();return reply;
    }
    boundary::End close_boundary(boundary::State& state,boundary::Receipt const& receipt,boundary::Finish const& finish,std::uintptr_t address) {
        std::lock_guard<std::mutex> lock(mutex_);const auto thread=backend_.thread(backend_.context);
        const auto result=external_.close(state,receipt,finish,context::thread_ledger,thread,thread&&thread==game_thread_&&thread!=session_.owner_worker_thread,address);
        if(result==boundary::End::closed){bool found=false;
            for(unsigned i=0;i<boundary_addresses_.size();++i)if(boundary_addresses_[i].load()==address&&boundary_ids_[i]==receipt.scope_id){
                boundary_ids_[i]=0;boundary_addresses_[i].store(0,std::memory_order_release);found=true;break;}
            if(found&&external_scopes_.load())--external_scopes_;else action_unknown();
        }else action_unknown();refresh_locked();return result;
    }

    Reply register_participant(Registration const* input,RegistrationResult* output) {
        if(reentrant())return Reply::reentrant;
        Registration r{};RegistrationResult result{};
        if(!weapon_broker_win32::copy(&r,input,sizeof(r))||!weapon_broker_win32::copy(&result,output,sizeof(result))||
           r.size!=sizeof(r)||r.abi_version!=version||r.frame_bytes!=sizeof(Frame)||r.proposal_bytes!=sizeof(Proposal)||
           r.decision_bytes!=sizeof(Decision)||r.reserved||!r.prepare||!r.commit||!r.invalidate||
           result.size!=sizeof(result)||result.abi_version!=version||!session_valid()||!backend_valid())return Reply::invalid;
        std::uint64_t reserved_cookie{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(stopping_||aux_debt_.load()||action_debt())return Reply::draining;
            if(state_.load()!=State::empty)return Reply::busy;
            if(next_cookie_==(std::numeric_limits<std::uint64_t>::max)()||revision_.load()==(std::numeric_limits<std::uint64_t>::max)())return Reply::invalid;
            reserved_cookie=next_cookie_++;state_.store(State::registering);cancel_registration_=false;phase_invalid_=false;refresh_locked();
        }
        ModulePin acquired{};const bool pinned=backend_.pin(backend_.context,r,acquired);
        std::lock_guard<std::mutex> lock(mutex_);pin_=acquired;pin_retained_.store(acquired.handle!=0);
        registration_=r;published_=false;cookie_.store(reserved_cookie);
        if(!pinned||!acquired.handle||!acquired.module||acquired.handle!=acquired.module||cancel_registration_){
            state_.store(acquired.handle?State::quiescent:State::empty);if(!acquired.handle)cookie_.store(0);refresh_locked();return cancel_registration_?Reply::draining:Reply::invalid;
        }
        result.cookie=reserved_cookie;result.native_started_qpc=session_.native_started_qpc;
        if(!weapon_broker_win32::copy(output,&result,sizeof(result))){state_.store(State::quiescent);refresh_locked();return Reply::invalid;}
        action_context_.store(reinterpret_cast<std::uintptr_t>(r.context));
        if(!revocation_.publish(action_auth())){state_.store(State::draining);action_unknown();return Reply::invalid;}
        bump_locked();published_=true;phase_invalid_=false;invalidate_running_=invalidate_failed_=final_invalidate_=false;invalidate_done_=true;
        have_attempt_=have_commit_=commit_started_=false;authorization_.store(false);state_.store(State::active);refresh_locked();return Reply::accepted;
    }

    Reply unregister(std::uint64_t expected_cookie) {
        if(reentrant())return Reply::reentrant;InvalidateCall call{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!expected_cookie||expected_cookie!=cookie_.load()||state_.load()==State::empty)
                return retired_locked(expected_cookie)?Reply::quiescent:Reply::wrong_cookie;
            invalidate_locked(InvalidateReason::unregistering,true,call,true);
        }
        run_invalidate(call);return quiescent()?Reply::quiescent:Reply::draining;
    }
    Reply invalidate(InvalidateReason reason) {
        if(reentrant())return Reply::reentrant;
        if(unsigned(reason)>unsigned(InvalidateReason::callback_fault))return Reply::invalid;
        InvalidateCall call{};
        {std::lock_guard<std::mutex> lock(mutex_);if(reason==InvalidateReason::native_stopping){stopping_=true;external_.stop();}
            invalidate_locked(reason,reason==InvalidateReason::native_stopping||reason==InvalidateReason::unregistering||reason==InvalidateReason::callback_fault,call);}
        run_invalidate(call);return quiescent()?Reply::quiescent:draining()?Reply::draining:Reply::accepted;
    }

    // Caller holds its Scene admission/stereo lock and uses one immutable Frame
    // on this synchronous GT stack. An accepted proposal creates an obligation:
    // commit it once, or abort_frame in an outer RAII guard. No worker may erase it.
    Reply prepare(Frame const& frame,Proposal& output) {
        if(reentrant())return Reply::reentrant;Frame local{};snapshot(local,frame);
        if(action_barrier())return action_debt()?Reply::draining:Reply::busy;
        // A genuine Native Scene frame is the existing safe callback point.
        // Drain once before prepare; no Gate/original gap invents a callback.
        InvalidateCall deferred{};
        {std::lock_guard<std::mutex> lock(mutex_);
            if(action_invalidation_pending_&&state_.load()==State::active&&!action_barrier()&&!callbacks_.load()&&!transactions_.load()&&
               !invalidate_running_&&key_session(local.key)&&local.key.registration_cookie==cookie_.load()&&frame_shape_valid(local)&&current_thread(local)&&
               active_now(local,backend_.clock_ns(backend_.context))&&(!have_attempt_||(local.now_ns>=attempt_.now_ns&&
               (local.key.lifecycle>attempt_.key.lifecycle||(local.key.lifecycle==attempt_.key.lifecycle&&local.key.frame>attempt_.key.frame))))){
                if(!game_thread_){game_thread_=local.key.thread_id;certified_game_thread_.store(game_thread_,std::memory_order_release);}
                action_invalidation_pending_=false;action_invalidation_pending_snapshot_.store(false);
                invalidate_locked(phase_reason_,false,deferred,true,true);
            }}
        run_invalidate(deferred);
        Registration r{};std::uint64_t revision{},epoch{};InvalidateCall invalidation{};Reply early=Reply::accepted;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!cookie_.load()||local.key.registration_cookie!=cookie_.load())return cookie_.load()?Reply::wrong_cookie:Reply::unavailable;
            if(state_.load()!=State::active||action_debt())return Reply::draining;
            if(!key_session(local.key)||!frame_shape_valid(local)||!current_thread(local)){
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(have_attempt_&&(local.key.lifecycle<attempt_.key.lifecycle||
                (local.key.lifecycle==attempt_.key.lifecycle&&local.key.frame<=attempt_.key.frame))){
                if(same(local.key,attempt_.key)&&equal_bytes(local,attempt_))return Reply::busy;
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(transactions_.load()){
                invalidate_locked(InvalidateReason::context_lost,false,invalidation);early=Reply::busy;
            }else {
                if(!game_thread_){game_thread_=local.key.thread_id;certified_game_thread_.store(game_thread_,std::memory_order_release);}
                const bool time_replayed=have_attempt_&&local.now_ns<attempt_.now_ns;
                snapshot(attempt_,local);have_attempt_=true;have_commit_=false;
                if(time_replayed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;}
                else if(!active_now(local,backend_.clock_ns(backend_.context))){invalidate_locked(InvalidateReason::input_lost,false,invalidation);early=Reply::unavailable;}
                else if(callbacks_.load()||invalidate_running_||invalidate_failed_||(phase_invalid_&&!invalidate_done_)){early=Reply::busy;}
                else {r=registration_;revision=revision_.load();snapshot(frozen_,local);proposal_={};frame_stamp_=revocation_.freeze(action_auth(),local.key.frame);
                    transaction_revision_=revision;epoch=operation_epoch_;commit_started_=false;transactions_.store(1);++callbacks_;++normal_calls_;}
            }
        }
        if(early!=Reply::accepted){run_invalidate(invalidation);return early;}
        Frame original{};snapshot(original,local);Proposal proposed{};std::uint32_t raw=unsigned(Reply::invalid);bool ok{},violated{};
        {CallbackMark mark(this);ok=backend_.prepare(backend_.context,r,&local,&proposed,raw);violated=mark.violated;}
        Reply reply=Reply::unavailable;
        {
            std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;
            const bool malformed=!ok||violated||!equal_bytes(local,original)||
                (raw!=unsigned(Reply::accepted)&&raw!=unsigned(Reply::unavailable)&&raw!=unsigned(Reply::busy))||
                (raw==unsigned(Reply::accepted)&&!proposal_valid_here(proposed,original));
            if(malformed){transactions_.store(0);invalidate_locked(InvalidateReason::callback_fault,true,invalidation);reply=Reply::invalid;}
            else if(state_.load()!=State::active||revision!=revision_.load()||epoch!=operation_epoch_||frame_stamp_.generation!=revocation_.generation()){
                transactions_.store(0);invalidate_locked(InvalidateReason::context_lost,false,invalidation);reply=draining()?Reply::draining:Reply::unavailable;}
            else if(raw!=unsigned(Reply::accepted)){transactions_.store(0);invalidate_locked(InvalidateReason::context_lost,false,invalidation);reply=static_cast<Reply>(raw);}
            else {
                snapshot(frozen_,original);snapshot(proposal_,proposed);transaction_revision_=revision;
                commit_started_=false;transactions_.store(1);phase_invalid_=false;authorization_.store(false);refresh_locked();reply=Reply::accepted;
            }
            if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)
                invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
            refresh_locked();
        }
        run_invalidate(invalidation);if(reply==Reply::accepted)snapshot(output,proposed);return reply;
    }

    Reply commit(Frame const& frame,Proposal const& proposal,Decision const& decision) {
        if(reentrant())return Reply::reentrant;
        if(action_barrier())return action_debt()?Reply::draining:Reply::busy;
        Frame local{};Proposal local_proposal{};Decision local_decision{};
        snapshot(local,frame);snapshot(local_proposal,proposal);snapshot(local_decision,decision);
        Registration r{};std::uint64_t revision{},epoch{};InvalidateCall invalidation{};Reply early=Reply::accepted;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!cookie_.load()||local.key.registration_cookie!=cookie_.load())return cookie_.load()?Reply::wrong_cookie:Reply::unavailable;
            if(!current_thread(local)){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;}
            else if(have_commit_&&same(local.key,frozen_.key)){
                if(equal_bytes(local,frozen_)&&equal_bytes(local_proposal,proposal_)&&equal_bytes(local_decision,committed_))return Reply::busy;
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(!transactions_.load()||commit_started_||!equal_bytes(local,frozen_)||!equal_bytes(local_proposal,proposal_)){
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(state_.load()!=State::active||phase_invalid_||transaction_revision_!=revision_.load())return draining()?Reply::draining:Reply::unavailable;
            else {
                const auto stamp=backend_.clock_ns(backend_.context);
                if(stamp<=0){invalidate_locked(InvalidateReason::input_lost,false,invalidation);early=Reply::unavailable;}
                else if(!decision_valid(local_decision,local,local_proposal,stamp)){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;}
                else if(!active_now(local,stamp)){invalidate_locked(InvalidateReason::input_lost,false,invalidation);early=Reply::unavailable;}
                else {r=registration_;revision=revision_.load();epoch=operation_epoch_;commit_started_=true;++callbacks_;++normal_calls_;}
            }
        }
        if(early!=Reply::accepted){run_invalidate(invalidation);return early;}
        Frame original{};Proposal original_proposal{};Decision original_decision{};
        snapshot(original,local);snapshot(original_proposal,local_proposal);snapshot(original_decision,local_decision);
        std::uint32_t raw=unsigned(Reply::invalid);bool ok{},violated{};
        {CallbackMark mark(this);ok=backend_.commit(backend_.context,r,&local,&local_proposal,&local_decision,raw);violated=mark.violated;}
        Reply reply=Reply::unavailable;
        {
            std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;transactions_.store(0);have_commit_=true;snapshot(committed_,original_decision);
            const bool malformed=!ok||violated||!equal_bytes(local,original)||!equal_bytes(local_proposal,original_proposal)||!equal_bytes(local_decision,original_decision)||
                (raw!=unsigned(Reply::accepted)&&raw!=unsigned(Reply::unavailable)&&raw!=unsigned(Reply::busy));
            if(malformed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);reply=Reply::invalid;}
            else if(state_.load()!=State::active||revision!=revision_.load()||epoch!=operation_epoch_){reply=draining()?Reply::draining:Reply::unavailable;}
            else if(raw!=unsigned(Reply::accepted)){invalidate_locked(InvalidateReason::context_lost,false,invalidation);reply=static_cast<Reply>(raw);}
            else if(!revocation_.commit(frame_stamp_)){invalidate_locked(InvalidateReason::context_lost,false,invalidation);reply=draining()?Reply::draining:Reply::unavailable;}
            else {authorized_frame_.store(frame_stamp_.frame);authorized_generation_.store(frame_stamp_.generation);
                authorization_.store(original_decision.lease.kind!=LeaseKind::none&&!original_decision.release_required,std::memory_order_release);reply=Reply::accepted;}
            if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)
                invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
            refresh_locked();
        }
        run_invalidate(invalidation);return reply;
    }

    Reply abort_frame(FrameKey const& key,std::uint64_t nonce) {
        if(reentrant())return Reply::reentrant;InvalidateCall call{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!cookie_.load()||key.registration_cookie!=cookie_.load())return Reply::wrong_cookie;
            if(!transactions_.load())return Reply::invalid;
            if(!same(key,frozen_.key)||nonce!=proposal_.nonce||!nonce||backend_.thread(backend_.context)!=game_thread_||commit_started_)return Reply::invalid;
            transactions_.store(0);invalidate_locked(InvalidateReason::context_lost,false,call);refresh_locked();
        }
        run_invalidate(call);return quiescent()?Reply::quiescent:Reply::accepted;
    }

    // MUST run on session.owner_worker_thread, outside callbacks, Scene/stereo
    // locks and loader lock. FreeLibrary/DllMain is deliberately outside mutex_.
    // A failed unpin keeps the same handle for a later explicit worker retry.
    Reply collect_on_owner_worker() {
        if(reentrant())return Reply::reentrant;
        if(!backend_valid()||backend_.thread(backend_.context)!=session_.owner_worker_thread)return Reply::invalid;
        InvalidateCall deferred{};
        {std::lock_guard<std::mutex> lock(mutex_);
            if(action_invalidation_pending_&&state_.load()==State::draining&&!callbacks_.load()&&!transactions_.load()&&
               !aux_scopes_.load()&&!action_scopes_.load()&&!external_scopes_.load()&&!invalidate_running_){
                action_invalidation_pending_=false;action_invalidation_pending_snapshot_.store(false);
                invalidate_locked(phase_reason_,true,deferred,true);
            }}
        run_invalidate(deferred);
        ModulePin held{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(game_thread_==session_.owner_worker_thread)return Reply::invalid;
            refresh_locked();if(state_.load()==State::empty)return aux_scopes_.load()||aux_debt_.load()||action_pending()?Reply::draining:Reply::quiescent;
            if(state_.load()!=State::quiescent||aux_scopes_.load()||aux_debt_.load()||action_pending()||!external_.counts().quiescent())return Reply::draining;
            held=pin_;state_.store(State::collecting);refresh_locked();
        }
        const bool released=!held.handle||backend_.unpin(backend_.context,held);
        std::lock_guard<std::mutex> lock(mutex_);
        if(!released){state_.store(State::quiescent);refresh_locked();return Reply::unavailable;}
        remember_retired_locked();
        pin_={};registration_={};main_mount_registration_={};main_mount_attached_=false;body_mount_registration_={};body_mount_attached_=false;call_registration_={};call_attached_.store(false);action_registration_={};action_attached_.store(false);
        (void)revocation_.publish({});action_context_.store(0);published_=false;cookie_.store(0);pin_retained_.store(false);authorization_.store(false);
        phase_invalid_=invalidate_running_=invalidate_failed_=final_invalidate_=false;invalidate_done_=true;have_attempt_=have_commit_=false;
        state_.store(State::empty);refresh_locked();return Reply::quiescent;
    }
};
} // namespace gears_weapon_hand
