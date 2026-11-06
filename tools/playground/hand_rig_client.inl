// One data-only Native reader. The existing closing-bit/count admission is
// reused solely to retain the module/client tuple across a getter. Its stored
// boundary functions are never called here and confer no write permission.
namespace handrig {
namespace wire=gears_rig_tracking;
namespace lifetime=gears_weapon_boundary;
lifetime::Admission admission;
wire::Read provider{};
gears_committed_hand::Read endpoint_provider{};
gears_committed_interaction::Read interaction_provider{};
wire::NativeKey key{};
thread_local bool reading{};
void seal() noexcept {admission.seal();}
bool quiescent() noexcept {return admission.quiescent();}
bool detached() noexcept {
    if(!admission.detach())return false;
    provider=nullptr;endpoint_provider=nullptr;interaction_provider=nullptr;key={};return true;
}
std::int64_t clock_ns() noexcept {return weaponhand::clock_ns(nullptr);}
wire::Reply provider_read(wire::ReadRequest const* request,wire::Frame* frame) noexcept {
    __try{return provider(request,frame);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){
        admission.quarantine();return wire::Reply::unavailable;
    }
}
gears_weapon_hand::Reply attach(void* image,std::uint64_t birth,std::uint32_t gt) noexcept {
    using Reply=gears_weapon_hand::Reply;
    const auto native=static_cast<HMODULE>(image);
    if(!native||!birth||!gt||native!=weaponhand::native_module||
       weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active)return Reply::unavailable;
    const auto read_address=GetProcAddress(native,wire::read_export);
    const auto revoke_address=GetProcAddress(native,lifetime::revoke_export);
    const auto open_address=GetProcAddress(native,lifetime::open_export);
    const auto close_address=GetProcAddress(native,lifetime::close_export);
    if(!weaponhand::executable(read_address,native)||!weaponhand::executable(revoke_address,native)||
       !weaponhand::executable(open_address,native)||!weaponhand::executable(close_address,native))return Reply::unavailable;
    key={GetCurrentProcessId(),gt,birth,weaponhand::client.native_started_qpc(),weaponhand::client.cookie()};
    provider=reinterpret_cast<wire::Read>(read_address);
    const auto endpoint_address=GetProcAddress(native,gears_committed_hand::read_export);
    endpoint_provider=weaponhand::executable(endpoint_address,native)?reinterpret_cast<gears_committed_hand::Read>(endpoint_address):nullptr;
    const auto interaction_address=GetProcAddress(native,gears_committed_interaction::read_export);
    interaction_provider=weaponhand::executable(interaction_address,native)?reinterpret_cast<gears_committed_interaction::Read>(interaction_address):nullptr;
    const lifetime::Binding binding{key.registration_cookie,key.native_started_qpc,&weaponhand::client,
        reinterpret_cast<lifetime::Revoke>(revoke_address),reinterpret_cast<lifetime::Open>(open_address),
        reinterpret_cast<lifetime::Close>(close_address)};
    return admission.connect(binding)?Reply::accepted:Reply::busy;
}
wire::ReadResult read(wire::Frame& out,std::uint64_t minimum_sequence,std::uint32_t expected_source) noexcept {
    out={};wire::ReadResult result{};
    if(reading)return result;
    struct Local {
        lifetime::Admission::Entry entry{};
        bool held{};
        ~Local(){if(held){reading=false;admission.release(entry);}}
    } local;
    if(!admission.acquire(local.entry))return result;
    local.held=true;reading=true;
    const auto binding=admission.view(local.entry);
    const auto gt=weaponhand::current_gt(nullptr);
    if(!binding||!provider||!gt||gt!=key.game_thread||
       binding->context!=&weaponhand::client||binding->cookie!=key.registration_cookie||
       binding->native_started_qpc!=key.native_started_qpc||
       weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active||
       weaponhand::client.cookie()!=key.registration_cookie||
       weaponhand::client.native_started_qpc()!=key.native_started_qpc)return result;
    const wire::ReaderBinding reader{key,expected_source,provider_read,clock_ns};
    result=wire::read_frame(reader,minimum_sequence,out);
    if(weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active||
       weaponhand::client.cookie()!=key.registration_cookie||
       weaponhand::client.native_started_qpc()!=key.native_started_qpc){
        out={};result.reply=wire::Reply::unavailable;result.failure=wire::ReadFailure::binding;
    }
    reading=false;
    if(!admission.release(local.entry)){
        out={};result.reply=wire::Reply::unavailable;result.failure=wire::ReadFailure::binding;
    }
    local.held=false;
    return result;
}
wire::Reply endpoint_call(gears_committed_hand::Request const* request,gears_committed_hand::Frame* out) noexcept {
    __try{return endpoint_provider(request,out);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){admission.quarantine();return wire::Reply::unavailable;}
}
bool read_endpoint(gears_committed_hand::Frame& out) noexcept {
    out={};if(reading||!endpoint_provider)return false;
    struct Local {lifetime::Admission::Entry entry{};bool held{};
        ~Local(){if(held){reading=false;admission.release(entry);}}} local;
    if(!admission.acquire(local.entry))return false;local.held=true;reading=true;
    const auto binding=admission.view(local.entry);const auto gt=weaponhand::current_gt(nullptr);
    if(!binding||!gt||gt!=key.game_thread||binding->context!=&weaponhand::client||
       binding->cookie!=key.registration_cookie||binding->native_started_qpc!=key.native_started_qpc||
       !weaponhand::client.registration_alive(key.registration_cookie,key.native_started_qpc)||
       weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active)return false;
    gears_committed_hand::Request request{};request.cookie=key.registration_cookie;request.native_started=key.native_started_qpc;
    gears_committed_hand::Frame candidate{};const auto reply=endpoint_call(&request,&candidate);
    const bool valid=reply==wire::Reply::accepted&&weaponhand::client.registration_alive(key.registration_cookie,key.native_started_qpc)&&
        weaponhand::client.phase()==gears_weapon_hand::ClientPhase::active;
    reading=false;const bool released=admission.release(local.entry);local.held=false;
    if(!valid||!released)return false;out=candidate;return true;
}
wire::Reply interaction_call(gears_committed_interaction::Request const* request,gears_committed_interaction::Frame* out) noexcept {
    __try{return interaction_provider(request,out);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){admission.quarantine();return wire::Reply::unavailable;}
}
bool read_interaction(gears_committed_interaction::Frame& out) noexcept {
    out={};if(reading||!interaction_provider)return false;
    struct Local {lifetime::Admission::Entry entry{};bool held{};
        ~Local(){if(held){reading=false;admission.release(entry);}}} local;
    if(!admission.acquire(local.entry))return false;local.held=true;reading=true;
    const auto binding=admission.view(local.entry);const auto gt=weaponhand::current_gt(nullptr);
    if(!binding||!gt||gt!=key.game_thread||binding->context!=&weaponhand::client||
       binding->cookie!=key.registration_cookie||binding->native_started_qpc!=key.native_started_qpc||
       !weaponhand::client.registration_alive(key.registration_cookie,key.native_started_qpc)||
       weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active)return false;
    gears_committed_interaction::Request request{};request.cookie=key.registration_cookie;request.native_started=key.native_started_qpc;
    gears_committed_interaction::Frame candidate{};const auto reply=interaction_call(&request,&candidate);
    const bool valid=reply==wire::Reply::accepted&&weaponhand::client.registration_alive(key.registration_cookie,key.native_started_qpc)&&
        weaponhand::client.phase()==gears_weapon_hand::ClientPhase::active;
    reading=false;const bool released=admission.release(local.entry);local.held=false;
    if(!valid||!released)return false;out=candidate;return true;
}
#if defined(GEARS_PLAYGROUND_HAND_RIG_TRACKING_TRIAL)
wire::ReadResult read(wire::Frame& out,std::uint64_t minimum_sequence) noexcept {
    return read(out,minimum_sequence,GEARS_PLAYGROUND_HAND_MOTION_EXPECTED_SOURCE);
}
#endif
} // namespace handrig
