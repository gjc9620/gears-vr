// Optional data-only callback on the existing parent Hand registration.
// Native owns the supplemental admission/pin/inflight; PG additionally counts
// every entry, including rejection/fault, in its existing callback lifetime.
#include "weapon_metadata_parse.h"
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
    weapon_metadata_parse::Workspace parsing{};
};
// These are caller-owned scratch values, never a cross-query permit/cache.
Workspace scratch;std::atomic_flag entered=ATOMIC_FLAG_INIT;
const wire::Response empty_response{};
static_assert(std::is_trivially_copyable_v<Workspace>);
// First32 claimed callbacks, POD only on the GT. These observations neither
// extend source age nor survive as a permission. Output occurs after the
// existing parent/PG drain on the normal lifecycle worker, never in C0.
struct Receipt {
    std::uint64_t query{},scene{},publication{},cookie{},native_started{},lifecycle{};
    std::int64_t begin_ns{},context_ns{},retained_ns{},response_ns{},finish_ns{},sync_ns{},received_ns{},published_ns{};
    unsigned stage{},returned{},status{UINT32_MAX},context_status{UINT32_MAX},retained_status{UINT32_MAX},response_status{UINT32_MAX};
    unsigned retained_stage{},binding{},agreement{},asset_status{};
};
Receipt active_receipt{};std::array<Receipt,32> receipts{};unsigned receipt_count{};
std::uint64_t receipt_attempts{};
bool diagnostics_written{};
bool diagnostics_drained() noexcept {
    return !enabled.load(std::memory_order_acquire)&&!callbacks.load(std::memory_order_acquire)&&
        !weaponhand::native_module&&!weaponhand::current_gt(nullptr);
}
void reset_diagnostics_after_drain() noexcept {
    if(!diagnostics_drained())return;
    active_receipt={};receipts={};receipt_count=0;receipt_attempts=0;diagnostics_written=false;
}
void write_diagnostics_after_drain() noexcept {
    if(!diagnostics_drained()||!log_file||diagnostics_written)return;
    diagnostics_written=true;
    for(unsigned i=0;i<receipt_count;++i){const auto& r=receipts[i];
        fwprintf(log_file,L"native_main_mount_query query=%llu scene=%llu publication=%llu cookie=%llu native_started=%llu lifecycle=%llu stage=%u returned=%u status=%u context_status=%u retained_status=%u response_status=%u retained_stage=%u binding=%u agreement=%u asset_status=%u begin_ns=%lld context_ns=%lld retained_ns=%lld response_ns=%lld finish_ns=%lld sync_ns=%lld received_ns=%lld published_ns=%lld\n",
            r.query,r.scene,r.publication,r.cookie,r.native_started,r.lifecycle,r.stage,r.returned,r.status,r.context_status,r.retained_status,r.response_status,
            r.retained_stage,r.binding,r.agreement,r.asset_status,r.begin_ns,r.context_ns,r.retained_ns,r.response_ns,r.finish_ns,
            r.sync_ns,r.received_ns,r.published_ns);
    }
    fwprintf(log_file,L"native_main_mount_query_summary attempts=%llu retained=%u truncated=%u\n",receipt_attempts,receipt_count,unsigned(receipt_attempts>receipts.size()));
    fflush(log_file);
}
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
    active_receipt.stage=1;
    std::uint32_t header[2]{};
    if(!request||!copy(header,reinterpret_cast<uptr>(request),sizeof(header)))return ~0u;
    if(header[0]!=sizeof(wire::Request)||header[1]!=wire::version)return unsigned(wire::Status::source_mismatch);
    if(!copy(&scratch.request,reinterpret_cast<uptr>(request),sizeof(scratch.request)))return ~0u;
    active_receipt.query=scratch.request.query_id;active_receipt.scene=scratch.request.retained.scene_invocation;
    active_receipt.publication=scratch.request.retained.publication_sequence;
    active_receipt.sync_ns=scratch.request.retained.controllers.sync_ns;
    active_receipt.received_ns=scratch.request.retained.controllers.received_ns;
    active_receipt.published_ns=scratch.request.retained.published_ns<=std::uint64_t(INT64_MAX)?
        static_cast<std::int64_t>(scratch.request.retained.published_ns):-1;
    active_receipt.cookie=scratch.request.retained.native.registration_cookie;
    active_receipt.native_started=scratch.request.retained.native.native_started_qpc;
    active_receipt.lifecycle=scratch.request.hand_lifecycle;
    scratch.response=empty_response;
    const sender::Parent parent{weaponhand::client.cookie(),weaponhand::client.native_started_qpc(),
        GetCurrentProcessId(),weaponhand::current_gt(nullptr)};
    if(!running(parent))return unsigned(wire::Status::inactive);
    active_receipt.stage=2;active_receipt.context_ns=weaponhand::clock_ns(nullptr);
    const auto request_status=sender::context(scratch.request,parent,active_receipt.context_ns,scratch.body);
    active_receipt.context_status=unsigned(request_status);
    if(request_status!=wire::Status::ready)return unsigned(request_status);
    active_receipt.stage=3;
    const auto status=nativeequipment_main_mount::prepare_retained(scratch.request.retained,scratch.body,
        scratch.candidate,scratch.diagnostic);
    active_receipt.retained_ns=weaponhand::clock_ns(nullptr);active_receipt.retained_status=unsigned(status);
    active_receipt.retained_stage=unsigned(scratch.diagnostic.stage);active_receipt.binding=unsigned(scratch.diagnostic.binding);
    active_receipt.agreement=unsigned(scratch.diagnostic.agreement);active_receipt.asset_status=unsigned(scratch.diagnostic.mount.asset_status);
    if(!running(parent))return unsigned(wire::Status::inactive);
    if(status!=gears_weapon_main_mount::Status::ready)return unsigned(sender::translate(status));
    active_receipt.stage=4;active_receipt.response_ns=weaponhand::clock_ns(nullptr);
    const auto result=sender::response(scratch.request,parent,scratch.candidate,active_receipt.response_ns,scratch.response);
    active_receipt.response_status=unsigned(result);
    if(!running(parent)){scratch.response=empty_response;return unsigned(wire::Status::inactive);}
    return unsigned(result);
}
std::uint32_t __cdecl prepare(void* context,wire::Request const* request,wire::Response* out) noexcept {
    callbacks.fetch_add(1,std::memory_order_acq_rel);
    bool claimed{},parse_installed{};
    weapon_metadata_parse::Workspace* previous_parse{};
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
        active_receipt={};active_receipt.begin_ns=weaponhand::clock_ns(nullptr);
        if(receipt_attempts!=UINT64_MAX)++receipt_attempts;
        previous_parse=weapon_metadata_parse::activate(scratch.parsing);parse_installed=true;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        saved_parser=component_names::enabled;parser_saved=true;component_names::enabled=false;
#endif
        result=prepare_data(request);
        if(wire::status_valid(result)&&result==unsigned(wire::Status::ready)){
            active_receipt.stage=5;
            if(!publish(out,scratch.response))result=~0u;
        }
        active_receipt.status=result;active_receipt.returned=1;active_receipt.finish_ns=weaponhand::clock_ns(nullptr);
        return result;
    }__finally{
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        if(parser_saved)component_names::enabled=saved_parser;
#endif
        if(claimed){
            if(parse_installed)weapon_metadata_parse::restore(previous_parse);
            if(receipt_count<receipts.size())receipts[receipt_count++]=active_receipt;
            std::memset(&scratch,0,sizeof(scratch));entered.clear(std::memory_order_release);
        }
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
