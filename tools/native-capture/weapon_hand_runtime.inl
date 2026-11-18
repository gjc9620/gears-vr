// Include weapon_hand_broker.h, <new> and <cstdio> at global scope first.
// Native worker configures/initializes before Scene admission or exports can
// register. This module creates one stable Broker, never an input/hand owner.
namespace full_body_c0_native_live {bool body_mount_source_current(gears_body_main_mount_provider::Source const&) noexcept;}
namespace troika_scene_tracking {bool geometry_current(gears_troika_geometry::Source const&) noexcept;}
namespace troika_control_live {
bool decide(gears_troika_geometry::Request const&,gears_troika_geometry::Response const&,gears_troika_control::Decision&) noexcept;
bool complete(gears_troika_geometry::Request const&,gears_troika_control::Decision const&,bool) noexcept;
}
namespace weapon_part_visibility_live {bool pending() noexcept;void request_drain() noexcept;}
namespace lancer_handle_morph_live {bool pending() noexcept;void request_drain() noexcept;}
namespace lancer_original_handle_live {bool pending() noexcept;void request_drain() noexcept;}
namespace boltok_original_live {bool pending() noexcept;void request_drain() noexcept;}
namespace weapon_hand_runtime {
namespace api=::gears_weapon_hand;
namespace call_api=::gears_weapon_call;
namespace action_api=::gears_weapon_action;
namespace mount_api=::gears_weapon_main_mount_provider;namespace body_mount_api=::gears_body_main_mount_provider;namespace rig_api=::gears_rig_tracking;
namespace boundary_api=::gears_weapon_boundary;
bool enabled{}; // Startup-only configuration; immutable after configure.
namespace detail {
alignas(api::Broker) unsigned char storage[sizeof(api::Broker)]{};
std::atomic<api::Broker*> published{};
api::BrokerSession binding{};api::BrokerBackend backend{}; // Immutable before release-publication.
std::mutex initialization_mutex;
bool configuration_seen{},configuration_ok{};unsigned configuration_request{};
unsigned restore_request{};bool engine_restore_enabled{};
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
    auto result=api::weapon_hand_win32_backend();
#if defined(GEARS_BODY_MAIN_MOUNT_SOURCE) && GEARS_BODY_MAIN_MOUNT_SOURCE
    result.body_mount_current=[](void*,body_mount_api::Source const& source)noexcept{return full_body_c0_native_live::body_mount_source_current(source);};
#endif
#if defined(GEARS_TROIKA_MOUNTED_SOURCE) && GEARS_TROIKA_MOUNTED_SOURCE
    result.troika_geometry_current=[](void*,gears_troika_geometry::Source const& source)noexcept{return troika_scene_tracking::geometry_current(source);};
    result.troika_control_decide=[](void*,gears_troika_geometry::Request const& q,gears_troika_geometry::Response const& g,gears_troika_control::Decision& d)noexcept{return troika_control_live::decide(q,g,d);};
    result.troika_control_complete=[](void*,gears_troika_geometry::Request const& q,gears_troika_control::Decision const& d,bool accepted)noexcept{return troika_control_live::complete(q,d,accepted);};
#endif
    return result;
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
// Atomic-only fault path for Revoke and uncertain external safe-copy. It must
// never turn a failed POD copy into a callback, mutex or module query.
inline void boundary_fault() noexcept {
    ++faults;unknown_fault.store(true);stop_requested.store(true);admission_closed.store(true);
    if(auto* b=published.load(std::memory_order_acquire))b->boundary_uncertain();
}
} // namespace detail

api::Broker* current() noexcept {return detail::published.load(std::memory_order_acquire);}
api::BrokerSession session() noexcept {return current()?detail::binding:api::BrokerSession{};}

bool configure(const wchar_t* ini,FILE* file) noexcept {
    detail::Call call;
    try {
        const unsigned request=ini?GetPrivateProfileIntW(L"WeaponHandParticipant",L"enabled",0,ini):0;
        const unsigned restore=ini?GetPrivateProfileIntW(L"WeaponActionGate",L"engine_restore",0,ini):0;
        bool prerequisites=false;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
        prerequisites=continuous_session&&stereo_xr&&anchor_observer_enabled;
#endif
        const bool valid_request=(request==0||(request==1&&prerequisites))&&restore<=1&&(!restore||request==1);
        std::lock_guard<std::mutex> lock(detail::initialization_mutex);
        if(detail::configuration_seen){
            const bool exact=detail::configuration_ok&&request==detail::configuration_request&&restore==detail::restore_request&&valid_request&&!detail::stop_requested.load();
            if(!exact)++detail::configure_rejected;return exact;
        }
        detail::configuration_seen=true;detail::configuration_request=request;
        detail::restore_request=restore;
        bool accepted=valid_request;
        if(detail::stop_requested.load())accepted=false;
        detail::configuration_ok=accepted;enabled=accepted&&request==1;
        detail::engine_restore_enabled=enabled&&restore==1;
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
            if(!b->configure_engine_restore(detail::engine_restore_enabled)){++detail::initialize_rejected;detail::boundary_fault();return false;}
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

api::Reply attach_troika_control(gears_troika_control::Registration const* input,api::RegistrationResult* output,bool consumer_owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    gears_troika_control::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof registration)||!api::weapon_broker_win32::copy(&result,output,sizeof result)){detail::fault();return api::Reply::invalid;}
    try{const auto reply=b->attach_troika_control(&registration,&result,consumer_owned);
        if(reply==api::Reply::accepted&&!api::weapon_broker_win32::copy(output,&result,sizeof result)){detail::fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::fault();return api::Reply::invalid;}
}
api::MainMountQueryResult transact_troika_control(gears_troika_geometry::Source const& source,gears_troika_control::Decision& output,gears_troika_geometry::Response& geometry) noexcept {
    detail::Call call;output={};geometry={};api::MainMountQueryResult result{};
    if(detail::admission_closed.load()||detail::stop_requested.load())return result;
    auto* b=current();if(!b)return result;
    result=b->transact_troika_control(source,output,geometry);
    if(detail::admission_closed.load()||detail::stop_requested.load()){output={};geometry={};result.code=api::MainMountQueryResult::Code::unavailable;}
    return result;
}
api::Reply attach_troika_geometry(gears_troika_geometry::Registration const* input,api::RegistrationResult* output,bool consumer_owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    gears_troika_geometry::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof registration)||!api::weapon_broker_win32::copy(&result,output,sizeof result)){detail::fault();return api::Reply::invalid;}
    try{const auto reply=b->attach_troika_geometry(&registration,&result,consumer_owned);
        if(reply==api::Reply::accepted&&!api::weapon_broker_win32::copy(output,&result,sizeof result)){detail::fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::fault();return api::Reply::invalid;}
}
api::MainMountQueryResult query_troika_geometry(gears_troika_geometry::Source const& source,gears_troika_geometry::Response& output) noexcept {
    detail::Call call;output={};api::MainMountQueryResult result{};
    if(detail::admission_closed.load()||detail::stop_requested.load())return result;
    auto* b=current();if(!b)return result;
    result=b->query_troika_geometry(source,output);
    if(detail::admission_closed.load()||detail::stop_requested.load()){output={};result.code=api::MainMountQueryResult::Code::unavailable;}
    return result;
}
api::Reply attach_main_mount_provider(mount_api::Registration const* input,api::RegistrationResult* output,bool consumer_owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    mount_api::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof registration)||!api::weapon_broker_win32::copy(&result,output,sizeof result)){detail::fault();return api::Reply::invalid;}
    try{const auto reply=b->attach_main_mount_provider(&registration,&result,consumer_owned);
        if(reply==api::Reply::accepted&&!api::weapon_broker_win32::copy(output,&result,sizeof result)){detail::fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::fault();return api::Reply::invalid;}
}
api::MainMountQueryResult query_main_mount(rig_api::Frame const& retained,mount_api::BodyGeometry const& body,mount_api::Response& output) noexcept {
    detail::Call call;output={};api::MainMountQueryResult result{};
    if(detail::admission_closed.load()||detail::stop_requested.load())return result;
    auto* b=current();if(!b)return result;
    result=b->query_main_mount(retained,body,output);
    if(detail::admission_closed.load()||detail::stop_requested.load()){output={};result.code=api::MainMountQueryResult::Code::unavailable;result.status=mount_api::Status::inactive;}
    return result;
}

api::Reply attach_body_mount_provider(body_mount_api::Registration const* input,api::RegistrationResult* output,bool consumer_owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    body_mount_api::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof registration)||!api::weapon_broker_win32::copy(&result,output,sizeof result)){detail::fault();return api::Reply::invalid;}
    try{const auto reply=b->attach_body_mount_provider(&registration,&result,consumer_owned);
        if(reply==api::Reply::accepted&&!api::weapon_broker_win32::copy(output,&result,sizeof result)){detail::fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::fault();return api::Reply::invalid;}
}
api::MainMountQueryResult query_body_mount(body_mount_api::Source const& source,body_mount_api::Response& output) noexcept {
    detail::Call call;output={};api::MainMountQueryResult result{};
    if(detail::admission_closed.load()||detail::stop_requested.load())return result;
    auto* b=current();if(!b)return result;
    result=b->query_body_mount(source,output);
    if(detail::admission_closed.load()||detail::stop_requested.load()){output={};result.code=api::MainMountQueryResult::Code::unavailable;result.status=mount_api::Status::inactive;}
    return result;
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

bool submit_hud_query(gears_weapon_hud_receipt::Receipt const* input) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return false;
    auto* b=current();if(!b)return false;
    gears_weapon_hud_receipt::Receipt value{};
    if(!api::weapon_broker_win32::copy(&value,input,sizeof(value)))return false;
    return b->submit_hud_query(value);
}
api::Reply attach_hand_visual(gears_committed_hand_consumer::Registration const* input,api::RegistrationResult* output,bool owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return api::Reply::draining;
    auto* b=current();if(!b)return api::Reply::unavailable;
    try{return b->attach_hand_visual(input,output,owned);}catch(...){detail::fault();return api::Reply::invalid;}
}
bool take_hud_query(std::uintptr_t root,std::uintptr_t controller,std::uintptr_t pawn,
                    gears_weapon_hud_receipt::Receipt& output) noexcept {
    detail::Call call;output={};
    if(detail::admission_closed.load()||detail::stop_requested.load())return false;
    auto* b=current();return b&&b->take_hud_query(root,controller,pawn,output);
}
bool begin_call(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,api::CallScope& scope,api::ActionScope const* gate=nullptr) noexcept {
    detail::Call call;++detail::call_begin_attempts;
    if((detail::admission_closed.load()||detail::stop_requested.load())&&(!gate||!gate->active))return false;
    auto* b=current();if(!b)return false;
    const bool admitted=b->begin_call(self,caller_frame,result,callee,scope,gate);
    if(admitted)++detail::call_begin_admitted;return admitted;
}
// Always finish the already-admitted stack token, including stop/disable and
// original-exception paths. No enabled/admission test may suppress its notice.
void finish_call(api::CallScope& scope,bool original_returned,std::uint64_t rax) noexcept {
    detail::Call call;if(!scope.active)return;++detail::call_finishes;
    auto* b=current();if(!b||scope.owner!=b){detail::fault();return;}
    b->finish_call(scope,original_returned,rax);
}

api::Reply attach_action_gate(action_api::Registration const* input,api::RegistrationResult* output,bool wrapper_owned) noexcept {
    detail::Call call;
    if(detail::admission_closed.load()||detail::stop_requested.load())return detail::stop_requested.load()?api::Reply::draining:api::Reply::unavailable;
    auto* b=current();if(!b)return api::Reply::unavailable;
    action_api::Registration registration{};api::RegistrationResult result{};
    if(!api::weapon_broker_win32::copy(&registration,input,sizeof(registration))||
       !api::weapon_broker_win32::copy(&result,output,sizeof(result))){detail::boundary_fault();return api::Reply::invalid;}
    try {const auto reply=b->attach_action_gate(&registration,&result,wrapper_owned);
        if(reply==api::Reply::accepted&&!api::weapon_broker_win32::copy(output,&result,sizeof(result))){detail::boundary_fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::boundary_fault();return api::Reply::invalid;}
}
bool begin_action(std::uintptr_t self,std::uintptr_t caller_frame,std::uintptr_t result,std::uintptr_t callee,api::ActionScope& scope) noexcept {
    detail::Call call;if(detail::admission_closed.load()||detail::stop_requested.load())return false;
    auto* b=current();return b&&b->begin_action(self,caller_frame,result,callee,scope);
}
void finish_action(api::ActionScope& scope,bool original_returned,std::uint64_t rax) noexcept {
    detail::Call call;if(!scope.active)return;auto* b=current();
    if(!b||scope.owner!=b){detail::boundary_fault();return;}b->finish_action(scope,original_returned,rax);
}
bool action_barrier() noexcept {auto* b=current();return detail::unknown_fault.load()||(b&&b->action_barrier());}
std::uint64_t action_generation() noexcept {auto* b=current();return b?b->action_generation():0;}

// Revoke is intentionally isolated from the ordinary runtime fault path.
api::Reply revoke_action(boundary_api::Request const* input,std::uint64_t* generation) noexcept {
    detail::Call call;boundary_api::Request request{};std::uint64_t result{};
    if(!api::weapon_broker_win32::copy(&request,input,sizeof(request))){detail::boundary_fault();return api::Reply::invalid;}
    auto* b=current();const auto reply=b?b->revoke_action(request,result):api::Reply::unavailable;
    if(!api::weapon_broker_win32::copy(generation,&result,sizeof(result))){detail::boundary_fault();return api::Reply::invalid;}return reply;
}
// Safe Native snapshots are the only inputs to Pool; its TLS link stays inside
// Native Cell. The original PG pointer is retained solely as an integer token.
struct NativeBoundaryState {boundary_api::Request request{};boundary_api::State state{};boundary_api::Receipt receipt{};
    boundary_api::Finish finish{};std::uintptr_t address{};};
static_assert(std::is_trivially_copyable_v<NativeBoundaryState>&&std::is_trivially_destructible_v<NativeBoundaryState>);
bool boundary_outputs_separate(boundary_api::State const* state,boundary_api::Receipt const* receipt) noexcept {
    const auto first=reinterpret_cast<std::uintptr_t>(state),second=reinterpret_cast<std::uintptr_t>(receipt);
    constexpr auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    return first&&second&&first<=maximum-sizeof(*state)&&second<=maximum-sizeof(*receipt)&&
        (first+sizeof(*state)<=second||second+sizeof(*receipt)<=first);
}
api::Reply open_boundary(boundary_api::Request const* input,boundary_api::State* state,boundary_api::Receipt* receipt) noexcept {
    detail::Call call;NativeBoundaryState native{};native.address=reinterpret_cast<std::uintptr_t>(state);
    if(!boundary_outputs_separate(state,receipt)){detail::boundary_fault();return api::Reply::invalid;}
    if(!api::weapon_broker_win32::copy(&native.request,input,sizeof(native.request))||
       !api::weapon_broker_win32::copy(&native.state,state,sizeof(native.state))){detail::boundary_fault();return api::Reply::invalid;}
    try {auto* b=current();api::Reply reply{};
        if(b)reply=b->open_boundary(native.request,native.state,native.receipt,native.address);
        else {const boundary_api::State zero{};const bool dirty=std::memcmp(&zero,&native.state,sizeof(zero))!=0;native.state={};
            reply=dirty||!boundary_api::request_valid(native.request)?api::Reply::invalid:api::Reply::unavailable;}
        if(!api::weapon_broker_win32::copy(state,&native.state,sizeof(native.state))||
           !api::weapon_broker_win32::copy(receipt,&native.receipt,sizeof(native.receipt))){detail::boundary_fault();return api::Reply::invalid;}
        return reply;
    }catch(...){detail::boundary_fault();return api::Reply::invalid;}
}
boundary_api::End close_boundary(boundary_api::State* state,boundary_api::Receipt const* receipt,boundary_api::Finish const* finish) noexcept {
    detail::Call call;NativeBoundaryState native{};native.address=reinterpret_cast<std::uintptr_t>(state);
    if(!api::weapon_broker_win32::copy(&native.state,state,sizeof(native.state))||
       !api::weapon_broker_win32::copy(&native.receipt,receipt,sizeof(native.receipt))||
       !api::weapon_broker_win32::copy(&native.finish,finish,sizeof(native.finish))){detail::boundary_fault();return boundary_api::End::unresolved;}
    try {auto* b=current();if(!b){detail::boundary_fault();return boundary_api::End::unresolved;}
        const auto reply=b->close_boundary(native.state,native.receipt,native.finish,native.address);
        if(!api::weapon_broker_win32::copy(state,&native.state,sizeof(native.state))){detail::boundary_fault();return boundary_api::End::unresolved;}return reply;
    }catch(...){detail::boundary_fault();return boundary_api::End::unresolved;}
}

void request_stop() noexcept {
    detail::Call call;++detail::stop_requests;
    detail::stop_requested.store(true);detail::admission_closed.store(true);
    weapon_part_visibility_live::request_drain();
    lancer_original_handle_live::request_drain();
    boltok_original_live::request_drain();
    lancer_handle_morph_live::request_drain();
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
    if(weapon_part_visibility_live::pending()||lancer_handle_morph_live::pending()||lancer_original_handle_live::pending()||boltok_original_live::pending())return true;
    if(detail::unknown_fault.load()||detail::calls.load())return true;
    auto* b=current();return b&&(b->cookie()||b->pin_retained()||b->pending_callbacks()||b->pending_transactions()||b->pending_aux_scopes()||b->aux_debt()||b->pending());
}
bool cleanup_ready() noexcept {
    if(weapon_part_visibility_live::pending()||lancer_handle_morph_live::pending()||lancer_original_handle_live::pending()||boltok_original_live::pending())return false;
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
    std::fprintf(file,"weapon_action_attached=%u weapon_action_scopes=%u weapon_external_scopes=%u weapon_action_sticky_debt=%u weapon_action_barrier=%u weapon_action_generation=%llu weapon_action_engine_restore=%u weapon_action_pending_invalidation=%u ",
        unsigned(b&&b->action_attached()),b?b->pending_action_scopes():0,b?b->pending_external_scopes():0,unsigned(b&&b->action_debt()),unsigned(action_barrier()),
        static_cast<unsigned long long>(action_generation()),unsigned(detail::engine_restore_enabled),unsigned(b&&b->pending_action_invalidation()));
}
} // namespace weapon_hand_runtime
