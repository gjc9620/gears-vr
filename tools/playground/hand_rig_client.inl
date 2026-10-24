// One data-only Native reader. The existing closing-bit/count admission is
// reused solely to retain the module/client tuple across a getter. Its stored
// boundary functions are never called here and confer no write permission.
namespace handrig {
namespace wire=gears_rig_tracking;
namespace lifetime=gears_weapon_boundary;
lifetime::Admission admission;
wire::Read provider{};
wire::NativeKey key{};
thread_local bool reading{};
void seal() noexcept {admission.seal();}
bool quiescent() noexcept {return admission.quiescent();}
bool detached() noexcept {
    if(!admission.detach())return false;
    provider=nullptr;key={};return true;
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
    const lifetime::Binding binding{key.registration_cookie,key.native_started_qpc,&weaponhand::client,
        reinterpret_cast<lifetime::Revoke>(revoke_address),reinterpret_cast<lifetime::Open>(open_address),
        reinterpret_cast<lifetime::Close>(close_address)};
    return admission.connect(binding)?Reply::accepted:Reply::busy;
}
wire::ReadResult read(wire::Frame& out,std::uint64_t minimum_sequence) noexcept {
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
    const wire::ReaderBinding reader{key,GEARS_PLAYGROUND_HAND_MOTION_EXPECTED_SOURCE,provider_read,clock_ns};
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
} // namespace handrig
