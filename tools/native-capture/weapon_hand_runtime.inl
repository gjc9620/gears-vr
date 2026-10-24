// Include weapon_hand_broker.h, <new> and <cstdio> at global scope first.
// Native worker configures/initializes before Scene admission or exports can
// register. This module creates one stable Broker, never an input/hand owner.
namespace weapon_hand_runtime {
namespace api=::gears_weapon_hand;
namespace call_api=::gears_weapon_call;
bool enabled{}; // Startup-only configuration; immutable after configure.
namespace detail {
alignas(api::Broker) unsigned char storage[sizeof(api::Broker)]{};
std::atomic<api::Broker*> published{};
api::BrokerSession binding{};api::BrokerBackend backend{}; // Immutable before release-publication.
std::mutex initialization_mutex;
bool configuration_seen{},configuration_ok{};unsigned configuration_request{};
std::atomic<bool> configured{},enabled_snapshot{},admission_closed{true},stop_requested{},unknown_fault{};
std::atomic<unsigned long long> calls{},configure_rejected{},initialize_attempts{},initialize_rejected{},
    register_attempts{},register_accepted{},register_rejected{},unregister_attempts{},stop_requests{},worker_ticks{},
    wrong_worker_ticks{},collect_attempts{},collect_failed{},faults{},call_attach_attempts{},call_attach_accepted{},call_begin_attempts{},call_begin_admitted{},call_finishes{};
struct Call {
    Call() noexcept {++calls;}~Call(){--calls;}
    Call(Call const&)=delete;Call& operator=(Call const&)=delete;
};
inline bool same_session(api::BrokerSession const& a,api::BrokerSession const& b) noexcept {
    return a.game_pid==b.game_pid&&a.owner_worker_thread==b.owner_worker_thread&&
        a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc;
}
inline api::BrokerBackend make_backend() {
#if defined(GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND)
    return GEARS_WEAPON_HAND_RUNTIME_TEST_BACKEND();
#else
    return api::weapon_hand_win32_backend();
#endif
}
// Only own C++ failures / failed short POD copies reach this path. Do NOT put
// an __except around Broker methods with RAII locks: foreign PG SEH is already
// contained by its small no-RAII bridges. Unknown responsibility stays retained.
inline void fault() noexcept {
    ++faults;unknown_fault.store(true);stop_requested.store(true);admission_closed.store(true);
    if(auto* b=published.load(std::memory_order_acquire)){
        try {b->invalidate(api::InvalidateReason::native_stopping);}catch(...){++faults;}
    }
}
} // namespace detail

api::Broker* current() noexcept {return detail::published.load(std::memory_order_acquire);}
api::BrokerSession session() noexcept {return current()?detail::binding:api::BrokerSession{};}

bool configure(const wchar_t* ini,FILE* file) noexcept {
    detail::Call call;
    try {
        const unsigned request=ini?GetPrivateProfileIntW(L"WeaponHandParticipant",L"enabled",0,ini):0;
        bool prerequisites=false;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
        prerequisites=continuous_session&&stereo_xr&&anchor_observer_enabled;
#endif
        const bool valid_request=request==0||(request==1&&prerequisites);
        std::lock_guard<std::mutex> lock(detail::initialization_mutex);
        if(detail::configuration_seen){
            const bool exact=detail::configuration_ok&&request==detail::configuration_request&&valid_request&&!detail::stop_requested.load();
            if(!exact)++detail::configure_rejected;return exact;
        }
        detail::configuration_seen=true;detail::configuration_request=request;
        bool accepted=valid_request;
        if(detail::stop_requested.load())accepted=false;
        detail::configuration_ok=accepted;enabled=accepted&&request==1;
        detail::enabled_snapshot.store(enabled);detail::configured.store(true,std::memory_order_release);
        if(!accepted)++detail::configure_rejected;
        if(file)std::fprintf(file,"weapon_hand_runtime_config enabled=%u requested=%u accepted=%u transport_only=1\n",
            unsigned(enabled),request,unsigned(accepted));
        return accepted;
    }catch(...){detail::fault();return false;}
}

bool initialize(std::uint64_t game_start,std::uint64_t native_started_qpc,std::uint32_t worker_thread) noexcept {
    detail::Call call;++detail::initialize_attempts;
    try {
        api::Broker* b{};
        {
            std::lock_guard<std::mutex> lock(detail::initialization_mutex);
            const api::BrokerSession requested{GetCurrentProcessId(),worker_thread,game_start,native_started_qpc};
            if(auto* existing=current()){
                const bool exact=detail::same_session(requested,detail::binding)&&detail::backend.thread&&
                    detail::backend.thread(detail::backend.context)==worker_thread;
                if(!exact)++detail::initialize_rejected;
                (void)existing;return exact; // Never reopens admission after stop.
            }
            if(!detail::configured.load(std::memory_order_acquire)||!detail::enabled_snapshot.load()||
               detail::stop_requested.load()||!game_start||!native_started_qpc||!worker_thread){++detail::initialize_rejected;return false;}
            auto backend=detail::make_backend();
            if(!backend.pin||!backend.unpin||!backend.thread||!backend.clock_ns||!backend.prepare||!backend.commit||!backend.invalidate||
               backend.thread(backend.context)!=worker_thread){++detail::initialize_rejected;return false;}
            b=::new(static_cast<void*>(detail::storage)) api::Broker(requested,backend);
            detail::binding=requested;detail::backend=backend;
            detail::published.store(b,std::memory_order_release);
            detail::admission_closed.store(false);
        }
        // Stop may have arrived while the factory/constructor had no published
        // pointer. Its sticky flag prevents any transient open gate from being
        // usable, and this recheck closes the newly created Broker as well.
        if(detail::stop_requested.load()){
            detail::admission_closed.store(true);b->invalidate(api::InvalidateReason::native_stopping);return false;
        }
        return true;
    }catch(...){++detail::initialize_rejected;detail::fault();return false;}
}

api::Reply register_participant(api::Registration const* input,api::RegistrationResult* output) noexcept {
    detail::Call call;++detail::register_attempts;
    if(detail::admission_closed.load()||detail::stop_requested.load()){
        ++detail::register_rejected;return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    }
    auto* b=current();if(!b){++detail::register_rejected;return api::Reply::unavailable;}
    api::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof(registration))||
       !api::weapon_broker_win32::copy(&result,output,sizeof(result))){++detail::register_rejected;detail::fault();return api::Reply::invalid;}
    try {
        const auto reply=b->register_participant(&registration,&result);
        if(reply==api::Reply::accepted){
            if(!api::weapon_broker_win32::copy(output,&result,sizeof(result))){++detail::register_rejected;detail::fault();return api::Reply::invalid;}
            ++detail::register_accepted;
        }else ++detail::register_rejected;
        return reply;
    }catch(...){++detail::register_rejected;detail::fault();return api::Reply::invalid;}
}

// Deliberately available after stop/collect: late PG callers need the exact
// retired-cookie quiescence receipt. Broker/context is never deleted/replaced.
api::Reply unregister(std::uint64_t cookie) noexcept {
    detail::Call call;++detail::unregister_attempts;
    auto* b=current();if(!b)return api::Reply::wrong_cookie;
    try {return b->unregister(cookie);}catch(...){detail::fault();return api::Reply::invalid;}
}

api::Reply attach_call_participant(call_api::Registration const* input,api::RegistrationResult* output,bool wrapper_owned) noexcept {
    detail::Call call;++detail::call_attach_attempts;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    call_api::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof(registration))||
       !api::weapon_broker_win32::copy(&result,output,sizeof(result))){detail::fault();return api::Reply::invalid;}
    try {
        const auto reply=b->attach_call_participant(&registration,&result,wrapper_owned);
        if(reply==api::Reply::accepted){
            if(!api::weapon_broker_win32::copy(output,&result,sizeof(result))){detail::fault();return api::Reply::invalid;}
            ++detail::call_attach_accepted;
        }
        return reply;
    }catch(...){detail::fault();return api::Reply::invalid;}
}

bool begin_call(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,api::CallScope& scope) noexcept {
    detail::Call call;++detail::call_begin_attempts;
    if(detail::admission_closed.load()||detail::stop_requested.load())return false;
    auto* b=current();if(!b)return false;
    const bool admitted=b->begin_call(self,caller_frame,result,callee,scope);
    if(admitted)++detail::call_begin_admitted;return admitted;
}
// Always finish the already-admitted stack token, including stop/disable and
// original-exception paths. No enabled/admission test may suppress its notice.
void finish_call(api::CallScope& scope,bool original_returned,std::uint64_t rax) noexcept {
    detail::Call call;if(!scope.active)return;++detail::call_finishes;
    auto* b=current();if(!b||scope.owner!=b){detail::fault();return;}
    b->finish_call(scope,original_returned,rax);
}

void request_stop() noexcept {
    detail::Call call;++detail::stop_requests;
    detail::stop_requested.store(true);detail::admission_closed.store(true);
    if(auto* b=current())try {b->invalidate(api::InvalidateReason::native_stopping);}catch(...){detail::fault();}
}

// ONLY the initialized owner worker, outside stereo/body/owner/loader locks.
// Never call collect from a Scene callback. A failed unpin keeps the same debt.
void worker_tick() noexcept {
    detail::Call call;++detail::worker_ticks;auto* b=current();if(!b)return;
    if(!detail::backend.thread||detail::backend.thread(detail::backend.context)!=detail::binding.owner_worker_thread){++detail::wrong_worker_ticks;return;}
    try {
        if(detail::stop_requested.load()){
            detail::admission_closed.store(true);b->invalidate(api::InvalidateReason::native_stopping);
        }
        if(b->draining()){
            ++detail::collect_attempts;
            const auto reply=b->collect_on_owner_worker();
            if(reply!=api::Reply::quiescent)++detail::collect_failed;
        }
    }catch(...){detail::fault();}
}

bool retained() noexcept {
    if(detail::unknown_fault.load()||detail::calls.load())return true;
    auto* b=current();return b&&(b->cookie()||b->pin_retained()||b->pending_callbacks()||b->pending_transactions()||b->pending_aux_scopes()||b->aux_debt()||b->pending());
}
bool cleanup_ready() noexcept {
    if(!detail::admission_closed.load()||detail::unknown_fault.load()||detail::calls.load())return false;
    auto* b=current();return !b||(!b->cookie()&&!b->pin_retained()&&!b->pending_callbacks()&&!b->pending_transactions()&&!b->pending_aux_scopes()&&!b->aux_debt()&&!b->pending()&&
        b->availability()==api::Availability::absent_quiescent);
}
void summary(FILE* file) noexcept {
    if(!file)return;auto* b=current();const auto binding=session();
    std::fprintf(file,"weapon_hand_enabled=%u weapon_hand_initialized=%u weapon_hand_admission_closed=%u weapon_hand_stop_requested=%u weapon_hand_unknown_fault=%u weapon_hand_retained=%u weapon_hand_cleanup_ready=%u weapon_hand_runtime_calls=%llu weapon_hand_register_attempts=%llu weapon_hand_register_accepted=%llu weapon_hand_register_rejected=%llu weapon_hand_unregister_attempts=%llu weapon_hand_stop_requests=%llu weapon_hand_worker_ticks=%llu weapon_hand_wrong_worker_ticks=%llu weapon_hand_collect_attempts=%llu weapon_hand_collect_failed=%llu weapon_hand_faults=%llu ",
        unsigned(detail::enabled_snapshot.load()),unsigned(b!=nullptr),unsigned(detail::admission_closed.load()),unsigned(detail::stop_requested.load()),unsigned(detail::unknown_fault.load()),unsigned(retained()),unsigned(cleanup_ready()),
        detail::calls.load(),detail::register_attempts.load(),detail::register_accepted.load(),detail::register_rejected.load(),detail::unregister_attempts.load(),detail::stop_requests.load(),detail::worker_ticks.load(),detail::wrong_worker_ticks.load(),detail::collect_attempts.load(),detail::collect_failed.load(),detail::faults.load());
    std::fprintf(file,"weapon_hand_cookie=%llu weapon_hand_revision=%llu weapon_hand_availability=%u weapon_hand_callbacks=%u weapon_hand_transactions=%u weapon_hand_pin=%u weapon_hand_game_pid=%u weapon_hand_game_start=%llu weapon_hand_native_started_qpc=%llu weapon_hand_worker_thread=%u ",
        static_cast<unsigned long long>(b?b->cookie():0),static_cast<unsigned long long>(b?b->lifecycle_revision():0),unsigned(b?b->availability():api::Availability::absent_quiescent),
        b?b->pending_callbacks():0,b?b->pending_transactions():0,unsigned(b&&b->pin_retained()),binding.game_pid,
        static_cast<unsigned long long>(binding.game_start),static_cast<unsigned long long>(binding.native_started_qpc),binding.owner_worker_thread);
    std::fprintf(file,"weapon_call_attached=%u weapon_call_aux_scopes=%u weapon_call_sticky_debt=%u weapon_call_attach_attempts=%llu weapon_call_attach_accepted=%llu weapon_call_begin_attempts=%llu weapon_call_begin_admitted=%llu weapon_call_finishes=%llu ",
        unsigned(b&&b->call_attached()),b?b->pending_aux_scopes():0,unsigned(b&&b->aux_debt()),detail::call_attach_attempts.load(),detail::call_attach_accepted.load(),detail::call_begin_attempts.load(),detail::call_begin_admitted.load(),detail::call_finishes.load());
}
} // namespace weapon_hand_runtime
