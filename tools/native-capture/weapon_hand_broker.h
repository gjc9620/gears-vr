#pragma once
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_call_boundary.h"
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
class Broker;
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
} // namespace weapon_broker_win32
inline BrokerBackend weapon_hand_win32_backend() noexcept {
    return {nullptr,weapon_broker_win32::pin,weapon_broker_win32::unpin,weapon_broker_win32::clock_ns,
        weapon_broker_win32::thread,weapon_broker_win32::prepare,weapon_broker_win32::commit,weapon_broker_win32::invalidate,
        weapon_broker_win32::call_module,weapon_broker_win32::call_begin,weapon_broker_win32::call_returned,weapon_broker_win32::call_unwind};
}

class Broker {
    enum class State {empty,registering,active,draining,quiescent,collecting};
    struct CallbackMark {
        Broker* broker{};bool violated{};CallbackMark* previous{};
        explicit CallbackMark(Broker* b) noexcept:broker(b),previous(current_){current_=this;}
        ~CallbackMark(){current_=previous;}
    };
    inline static thread_local CallbackMark* current_{};
    inline static thread_local CallScope* call_top_{};
    struct InvalidateCall {Registration registration{};Invalidation message{};bool needed{};};
    BrokerSession session_;BrokerBackend backend_;std::mutex mutex_;
    std::atomic<State> state_{State::empty};
    std::atomic<Availability> availability_{Availability::absent_quiescent};
    std::atomic<std::uint64_t> revision_{1},cookie_{};
    std::atomic<unsigned> callbacks_{},transactions_{};
    std::atomic<unsigned> aux_scopes_{};
    std::atomic<bool> aux_debt_{},call_attached_{};
    std::atomic<bool> authorization_{},pin_retained_{};
    Registration registration_{};ModulePin pin_{};
    call::Registration call_registration_{};std::uint64_t next_call_id_{1};
    std::uint64_t next_cookie_{1},transaction_revision_{},operation_epoch_{1};
    std::uint32_t game_thread_{};unsigned normal_calls_{};
    bool published_{},cancel_registration_{},stopping_{},phase_invalid_{},invalidate_running_{},invalidate_done_{true},invalidate_failed_{};
    bool final_invalidate_{};InvalidateReason phase_reason_{};
    bool have_attempt_{},have_commit_{},commit_started_{};
    Frame attempt_{},frozen_{};Proposal proposal_{};Decision committed_{};
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
    template<class T> static bool equal_bytes(T const& a,T const& b) noexcept {return std::memcmp(&a,&b,sizeof(T))==0;}
    template<class T> static void snapshot(T& out,T const& value) noexcept {std::memcpy(&out,&value,sizeof(T));}
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
        if(state_.load()==State::draining&&!callbacks_.load()&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load()&&invalidate_done_&&!invalidate_running_&&!final_invalidate_)
            state_.store(State::quiescent,std::memory_order_release);
        const auto s=state_.load();
        availability_.store(s==State::empty&&!aux_scopes_.load()&&!aux_debt_.load()?Availability::absent_quiescent:
            s==State::active&&!phase_invalid_?Availability::ready:Availability::unavailable,std::memory_order_release);
    }
    void invalidate_locked(InvalidateReason reason,bool hard,InvalidateCall& call,bool retry=false) noexcept {
        const auto s=state_.load();authorization_.store(false,std::memory_order_release);
        if(s==State::empty||s==State::quiescent||s==State::collecting)return;
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
        if(published_&&!invalidate_done_&&!invalidate_running_&&(!invalidate_failed_||retry)){
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
public:
    // Construct on a stable Native session; the explicit owner worker is the
    // ONLY thread allowed to release a module ref. No destructor auto-unpins.
    explicit Broker(BrokerSession session,BrokerBackend backend=weapon_hand_win32_backend()):session_(session),backend_(backend){}
    Broker(Broker const&)=delete;Broker& operator=(Broker const&)=delete;
    ~Broker()=default; // Caller must keep this object/context alive through drain+collect.
    std::uint64_t lifecycle_revision() const noexcept {return revision_.load(std::memory_order_acquire);}
    std::uint64_t cookie() const noexcept {return cookie_.load(std::memory_order_acquire);}
    Availability availability() const noexcept {return availability_.load(std::memory_order_acquire);}
    bool authorized() const noexcept {return authorization_.load(std::memory_order_acquire);}
    unsigned pending_callbacks() const noexcept {return callbacks_.load(std::memory_order_acquire);}
    unsigned pending_transactions() const noexcept {return transactions_.load(std::memory_order_acquire);}
    unsigned pending_aux_scopes() const noexcept {return aux_scopes_.load(std::memory_order_acquire);}
    bool aux_debt() const noexcept {return aux_debt_.load(std::memory_order_acquire);}
    bool call_attached() const noexcept {return call_attached_.load(std::memory_order_acquire);}
    bool pin_retained() const noexcept {return pin_retained_.load(std::memory_order_acquire);}
    // Transport quiescence only: no callback or frozen-frame obligation. The
    // void atomic invalidate is NOT a receipt for PG's game-resource restore.
    bool quiescent() const noexcept {const auto s=state_.load();return (s==State::empty||s==State::quiescent)&&!callbacks_.load()&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load();}
    bool draining() const noexcept {const auto s=state_.load();return s==State::draining||s==State::quiescent||s==State::collecting;}
    bool pending() const noexcept {const auto s=state_.load();return callbacks_.load()||transactions_.load()||aux_scopes_.load()||aux_debt_.load()||(s!=State::empty&&s!=State::active);}

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

    bool begin_call(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,CallScope& scope) noexcept {
        if(reentrant()){auxiliary_unknown();return false;}
        try {
            bool bad=false;
            {std::lock_guard<std::mutex> lock(mutex_);
                if(!published_||!call_attached_.load()||stopping_||aux_debt_.load()||state_.load()!=State::active)return false;
                if(scope.active||scope.owner||scope.address||scope.token)bad=true;
                unsigned depth=0;for(auto* p=call_top_;p;p=p->previous){if(p==&scope||++depth>=64){bad=true;break;}}
                if(bad){/* Preserve the prior token; it remains owed. */}
                else if(!backend_.thread(backend_.context)||next_call_id_==(std::numeric_limits<std::uint64_t>::max)()||
                        aux_scopes_.load()==(std::numeric_limits<unsigned>::max)())bad=true;
                else {
                    scope={};scope.owner=this;scope.previous=call_top_;scope.address=reinterpret_cast<std::uintptr_t>(&scope);
                    scope.token=next_call_id_++;scope.thread=backend_.thread(backend_.context);scope.registration=call_registration_;
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
        if(reentrant()){auxiliary_unknown();return;}
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
            if(stopping_||aux_debt_.load())return Reply::draining;
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
        {std::lock_guard<std::mutex> lock(mutex_);if(reason==InvalidateReason::native_stopping)stopping_=true;
            invalidate_locked(reason,reason==InvalidateReason::native_stopping||reason==InvalidateReason::unregistering||reason==InvalidateReason::callback_fault,call);}
        run_invalidate(call);return quiescent()?Reply::quiescent:draining()?Reply::draining:Reply::accepted;
    }

    // Caller holds its Scene admission/stereo lock and uses one immutable Frame
    // on this synchronous GT stack. An accepted proposal creates an obligation:
    // commit it once, or abort_frame in an outer RAII guard. No worker may erase it.
    Reply prepare(Frame const& frame,Proposal& output) {
        if(reentrant())return Reply::reentrant;Frame local{};snapshot(local,frame);
        Registration r{};std::uint64_t revision{},epoch{};InvalidateCall invalidation{};Reply early=Reply::accepted;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!cookie_.load()||local.key.registration_cookie!=cookie_.load())return cookie_.load()?Reply::wrong_cookie:Reply::unavailable;
            if(state_.load()!=State::active)return Reply::draining;
            if(!key_session(local.key)||!frame_shape_valid(local)||!current_thread(local)){
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(have_attempt_&&(local.key.lifecycle<attempt_.key.lifecycle||
                (local.key.lifecycle==attempt_.key.lifecycle&&local.key.frame<=attempt_.key.frame))){
                if(same(local.key,attempt_.key)&&equal_bytes(local,attempt_))return Reply::busy;
                invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;
            }else if(transactions_.load()){
                invalidate_locked(InvalidateReason::context_lost,false,invalidation);early=Reply::busy;
            }else {
                if(!game_thread_)game_thread_=local.key.thread_id;
                const bool time_replayed=have_attempt_&&local.now_ns<attempt_.now_ns;
                snapshot(attempt_,local);have_attempt_=true;have_commit_=false;
                if(time_replayed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);early=Reply::invalid;}
                else if(!active_now(local,backend_.clock_ns(backend_.context))){invalidate_locked(InvalidateReason::input_lost,false,invalidation);early=Reply::unavailable;}
                else if(callbacks_.load()||invalidate_running_||invalidate_failed_||(phase_invalid_&&!invalidate_done_)){early=Reply::busy;}
                else {r=registration_;revision=revision_.load();snapshot(frozen_,local);proposal_={};
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
            else if(state_.load()!=State::active||revision!=revision_.load()||epoch!=operation_epoch_){transactions_.store(0);reply=draining()?Reply::draining:Reply::unavailable;}
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
            else {authorization_.store(original_decision.lease.kind!=LeaseKind::none&&!original_decision.release_required,std::memory_order_release);reply=Reply::accepted;}
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
        ModulePin held{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(game_thread_==session_.owner_worker_thread)return Reply::invalid;
            refresh_locked();if(state_.load()==State::empty)return aux_scopes_.load()||aux_debt_.load()?Reply::draining:Reply::quiescent;
            if(state_.load()!=State::quiescent||aux_scopes_.load()||aux_debt_.load())return Reply::draining;
            held=pin_;state_.store(State::collecting);refresh_locked();
        }
        const bool released=!held.handle||backend_.unpin(backend_.context,held);
        std::lock_guard<std::mutex> lock(mutex_);
        if(!released){state_.store(State::quiescent);refresh_locked();return Reply::unavailable;}
        remember_retired_locked();
        pin_={};registration_={};call_registration_={};call_attached_.store(false);published_=false;cookie_.store(0);pin_retained_.store(false);authorization_.store(false);
        phase_invalid_=invalidate_running_=invalidate_failed_=final_invalidate_=false;invalidate_done_=true;have_attempt_=have_commit_=false;
        state_.store(State::empty);refresh_locked();return Reply::quiescent;
    }
};
} // namespace gears_weapon_hand
