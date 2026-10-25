// Optional data-only callback on the existing parent Hand registration.
// Native owns the supplemental admission/pin/inflight; PG additionally counts
// every entry, including rejection/fault, in its existing callback lifetime.
namespace mainmountprovider {
namespace wire=gears_weapon_main_mount_provider;
namespace sender=gears_weapon_main_mount_sender;
namespace source=gears_weapon_main_mount_source;
namespace hand=gears_weapon_hand;
struct Workspace {
    wire::Request request{};wire::Response response{};
    source::BodyContext body{};
    gears_weapon_main_mount::Candidate candidate{};
    nativeequipment_main_mount::RetainedDiagnostic diagnostic{};
};
// These are caller-owned scratch values, never a cross-query permit/cache.
Workspace scratch;std::atomic_flag entered=ATOMIC_FLAG_INIT;
const wire::Response empty_response{};
static_assert(std::is_trivially_copyable_v<Workspace>);
bool publish(wire::Response* out,wire::Response const& value) noexcept {
    __try{if(!out)return false;std::memcpy(out,&value,sizeof(value));return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool running(sender::Parent const& parent) noexcept {
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)==parent.game_thread&&
        nativeequipment::domain_ready()&&weaponhand::client.registration_alive(parent.cookie,parent.native_started_qpc)&&
        weaponhand::client.armed();
}
std::uint32_t prepare_data(wire::Request const* request) noexcept {
    std::uint32_t header[2]{};
    if(!request||!copy(header,reinterpret_cast<uptr>(request),sizeof(header)))return ~0u;
    if(header[0]!=sizeof(wire::Request)||header[1]!=wire::version)return unsigned(wire::Status::source_mismatch);
    if(!copy(&scratch.request,reinterpret_cast<uptr>(request),sizeof(scratch.request)))return ~0u;
    scratch.response=empty_response;
    const sender::Parent parent{weaponhand::client.cookie(),weaponhand::client.native_started_qpc(),
        GetCurrentProcessId(),weaponhand::current_gt(nullptr)};
    if(!running(parent))return unsigned(wire::Status::inactive);
    const auto request_status=sender::context(scratch.request,parent,weaponhand::clock_ns(nullptr),scratch.body);
    if(request_status!=wire::Status::ready)return unsigned(request_status);
    const auto status=nativeequipment_main_mount::prepare_retained(scratch.request.retained,scratch.body,
        scratch.candidate,scratch.diagnostic);
    if(!running(parent))return unsigned(wire::Status::inactive);
    if(status!=gears_weapon_main_mount::Status::ready)return unsigned(sender::translate(status));
    const auto result=sender::response(scratch.request,parent,scratch.candidate,
        weaponhand::clock_ns(nullptr),scratch.response);
    if(!running(parent)){scratch.response=empty_response;return unsigned(wire::Status::inactive);}
    return unsigned(result);
}
std::uint32_t __cdecl prepare(void* context,wire::Request const* request,wire::Response* out) noexcept {
    callbacks.fetch_add(1,std::memory_order_acq_rel);
    bool claimed{};
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    bool parser_saved{},saved_parser{};
#endif
    std::uint32_t result=unsigned(wire::Status::inactive);
    // Only POD locals here: finally also releases PG's count and scratch on a
    // structured callback fault. The parent handles invalid status/SEH itself.
    __try{
        if(!publish(out,empty_response))return ~0u;
        if(context!=&weaponhand::client||!weaponhand::current_gt(nullptr)||
            !enabled.load(std::memory_order_acquire)||!nativeequipment::domain_ready())return result;
        if(entered.test_and_set(std::memory_order_acquire))return ~0u;
        claimed=true;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        saved_parser=component_names::enabled;parser_saved=true;component_names::enabled=false;
#endif
        result=prepare_data(request);
        if(wire::status_valid(result)&&result==unsigned(wire::Status::ready)){
            if(!publish(out,scratch.response))return ~0u;
        }
        return result;
    }__finally{
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(parser_saved)component_names::enabled=saved_parser;
#endif
        if(claimed){std::memset(&scratch,0,sizeof(scratch));entered.clear(std::memory_order_release);}
        callbacks.fetch_sub(1,std::memory_order_release);
    }
}
bool call_attach(wire::Attach fn,wire::Registration const& registration,hand::RegistrationResult& receipt,hand::Reply& reply) noexcept {
    __try{reply=fn(&registration,&receipt);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
hand::Reply attach(HMODULE native) noexcept {
    if(weaponhand::current_gt(nullptr)||!enabled.load(std::memory_order_acquire)||!native||
        native!=weaponhand::native_module)return hand::Reply::unavailable;
    const auto address=GetProcAddress(native,wire::attach_export);
    if(!weaponhand::executable(address,native))return hand::Reply::unavailable;
    wire::Registration registration{};registration.hand_cookie=weaponhand::client.cookie();
    registration.native_started_qpc=weaponhand::client.native_started_qpc();registration.context=&weaponhand::client;
    registration.prepare=prepare;
    if(!wire::registration_shape_valid(registration)||
        !weaponhand::client.registration_alive(registration.hand_cookie,registration.native_started_qpc))return hand::Reply::unavailable;
    hand::RegistrationResult receipt{};hand::Reply reply=hand::Reply::unavailable;
    if(!call_attach(reinterpret_cast<wire::Attach>(address),registration,receipt,reply))return hand::Reply::unavailable;
    if(reply!=hand::Reply::accepted)return reply;
    if(receipt.size!=sizeof(receipt)||receipt.abi_version!=hand::version||receipt.cookie!=registration.hand_cookie||
        receipt.native_started_qpc!=registration.native_started_qpc)return hand::Reply::invalid;
    if(!weaponhand::client.registration_alive(registration.hand_cookie,registration.native_started_qpc))return hand::Reply::draining;
    return hand::Reply::accepted;
}
} // namespace mainmountprovider
