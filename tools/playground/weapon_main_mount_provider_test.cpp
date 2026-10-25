#include <windows.h>
#include <atomic>
#include <thread>
#include "weapon_main_mount_sender.h"
#define main weapon_main_mount_provider_owned_fixture
#include "weapon_rig_binding_test.cpp"
#undef main

namespace {
using uptr=std::uintptr_t;
namespace wire=gears_weapon_main_mount_provider;
namespace policy=gears_weapon_main_mount;
namespace source=gears_weapon_main_mount_source;
std::atomic<unsigned> callbacks{};
std::atomic<bool> enabled{true};
FILE* log_file{};
namespace component_names {thread_local bool enabled=true;}
namespace weaponhand {
struct Client {
    std::uint64_t cookie_value{61},started{51};std::atomic<bool> alive{true},armed_value{true};
    std::uint64_t cookie() const noexcept {return cookie_value;}
    std::uint64_t native_started_qpc() const noexcept {return started;}
    bool armed() const noexcept {return armed_value.load();}
    bool registration_alive(std::uint64_t cookie,std::uint64_t native) const noexcept {
        return alive.load()&&cookie==cookie_value&&native==started;
    }
} client;
HMODULE native_module=reinterpret_cast<HMODULE>(uptr{0x7000});
bool on_gt=true,code_valid=true;
std::uint32_t current_gt(void*) noexcept {return on_gt?GetCurrentThreadId():0;}
std::int64_t clock_ns(void*) noexcept {return now;}
bool executable(FARPROC address,HMODULE image) noexcept {return code_valid&&address&&image==native_module;}
}
namespace nativeequipment {bool domain=true;bool domain_ready() noexcept {return domain;}}
uptr unreadable{};unsigned copies{},reader_calls{},resolver_calls{},attach_calls{};
bool copy(void* out,uptr from,std::size_t size) noexcept {
    ++copies;if(!from||from==unreadable)return false;
    __try{std::memcpy(out,reinterpret_cast<void const*>(from),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
enum class ReaderMode {ready,unheld,busy,revoke,soft_revoke,reenter,seh,wrong_candidate,publish_fail};
ReaderMode reader_mode=ReaderMode::ready;
wire::Request const* active_request{};wh::Frame active_hand{};
constexpr DWORD custom_exception=0xe0424781;
std::uint32_t nested_status{};wire::Response nested_response{};
wire::Response* protected_response{};
namespace nativeequipment_main_mount {
struct RetainedDiagnostic {
    unsigned marker{},stage{};source::SourceStatus binding{};gears_equipment_rig::Status agreement{};
    struct {unsigned asset_status{};} mount{};
};
policy::Status prepare_retained(rt::Frame const&,source::BodyContext const&,policy::Candidate&,RetainedDiagnostic&) noexcept;
}
unsigned receipt_fault{};bool revoke_on_attach{},attach_seh{},resolve_missing{};
wh::Reply attach_reply=wh::Reply::accepted;wire::Registration received_registration{};
wh::Reply __cdecl fake_attach(wire::Registration const* registration,wh::RegistrationResult* receipt) noexcept {
    ++attach_calls;received_registration=*registration;
    CHECK(wire::registration_shape_valid(*registration));
    if(attach_seh)RaiseException(custom_exception,0,0,nullptr);
    receipt->cookie=registration->hand_cookie;receipt->native_started_qpc=registration->native_started_qpc;
    if(receipt_fault==1)++receipt->size;if(receipt_fault==2)++receipt->abi_version;
    if(receipt_fault==3)++receipt->cookie;if(receipt_fault==4)++receipt->native_started_qpc;
    if(revoke_on_attach){std::thread worker([]{weaponhand::client.alive.store(false);});worker.join();}
    return attach_reply;
}
FARPROC fake_resolver(HMODULE image,LPCSTR name) noexcept {
    ++resolver_calls;CHECK(image==weaponhand::native_module&&std::strcmp(name,wire::attach_export)==0);
    return resolve_missing?nullptr:reinterpret_cast<FARPROC>(fake_attach);
}
#define GetProcAddress fake_resolver
#include "weapon_main_mount_provider.inl"
#undef GetProcAddress

namespace nativeequipment_main_mount {
// This stub is deliberately NOT an asset/attachment reader. It supplies an
// owned synthetic candidate solely to exercise the actual callback wrapper.
policy::Status prepare_retained(rt::Frame const& retained,source::BodyContext const& body,
    policy::Candidate& out,RetainedDiagnostic& diagnostic) noexcept {
    ++reader_calls;CHECK(callbacks.load()==1&&!component_names::enabled);
    CHECK(active_request&&retained.scene_invocation==active_request->retained.scene_invocation&&body.known);
    const auto query=mainmountprovider::scratch.request.query_id;
    if(reader_mode==ReaderMode::reenter){
        const auto outer=mainmountprovider::scratch;
        nested_response.query_id=999;
        nested_status=mainmountprovider::prepare(&weaponhand::client,active_request,&nested_response);
        CHECK(nested_status==~0u&&callbacks.load()==1&&!component_names::enabled);
        CHECK(mainmountprovider::scratch.request.query_id==query); // Inner entry did not erase outer scratch.
        CHECK(!std::memcmp(&outer,&mainmountprovider::scratch,sizeof outer));
    }
    if(reader_mode==ReaderMode::seh)RaiseException(custom_exception,0,0,nullptr);
    if(reader_mode==ReaderMode::revoke){std::thread worker([]{weaponhand::client.alive.store(false);});worker.join();}
    if(reader_mode==ReaderMode::soft_revoke){std::thread worker([]{weaponhand::client.armed_value.store(false);});worker.join();}
    out={};out.key=active_hand.key;out.world=active_hand.world;out.controller=active_hand.controller;
    out.pawn=active_hand.pawn;out.mesh=active_hand.mesh;out.weapon=active_hand.weapon;
    out.component=object(0x7000,8);out.asset=object(0x7100,9);out.hold_generation=9;
    out.provider_revision=policy::provider_revision;out.body_revision=body.revision;out.basis_revision=body.basis_revision;
    out.basis=body.basis;out.geometry=body.geometry;out.model_scale=body.model_scale;out.units_per_metre=body.units_per_metre;
    policy::lancer::Profile profile{};CHECK(policy::lancer::marcus115(body.geometry,body.model_scale,body.units_per_metre,profile));
    out.right=profile.mount.calibration;diagnostic.marker=17;
    if(reader_mode==ReaderMode::wrong_candidate)++out.key.frame;
    if(reader_mode==ReaderMode::publish_fail){DWORD old{};
        CHECK(protected_response&&VirtualProtect(protected_response,sizeof(wire::Response),PAGE_READONLY,&old));}
    return reader_mode==ReaderMode::unheld?policy::Status::unheld:reader_mode==ReaderMode::busy?policy::Status::busy:policy::Status::ready;
}
}
bool empty(wire::Response const& response){const wire::Response zero{};return !std::memcmp(&response,&zero,sizeof zero);}
bool scratch_empty(){const auto* p=reinterpret_cast<unsigned char const*>(&mainmountprovider::scratch);
    for(std::size_t i=0;i<sizeof(mainmountprovider::scratch);++i)if(p[i])return false;return true;}
struct Trial {
    Fixture fixture;wire::Request request{};wire::Response response{};
    explicit Trial(unsigned selected=0):fixture(selected){
        enabled=true;nativeequipment::domain=weaponhand::on_gt=weaponhand::code_valid=true;
        weaponhand::client.alive=true;weaponhand::client.armed_value=true;reader_mode=ReaderMode::ready;
        weaponhand::client.cookie_value=61;weaponhand::client.started=51;
        receipt_fault=0;revoke_on_attach=attach_seh=resolve_missing=false;attach_reply=wh::Reply::accepted;
        unreadable=0;copies=reader_calls=resolver_calls=attach_calls=0;component_names::enabled=true;
        auto& f=fixture;f.rig.native.game_pid=f.hand.key.game_pid=GetCurrentProcessId();
        f.rig.native.game_thread=f.hand.key.thread_id=GetCurrentThreadId();active_hand=f.hand;
        request.query_id=901;request.hand_lifecycle=f.hand.key.lifecycle;request.retained=f.rig;
        request.body.mesh=f.rig.mesh;request.body.geometry=gears_hand_authored_mount::marcus115_signature;
        request.body.model_scale=1;request.body.units_per_metre=f.rig.units_per_metre;
        request.body.revision=f.rig.binding_generation;request.body.basis_revision=wire::supported_basis_revision;
        request.body.basis=wire::Basis::marcus_mesh_face_forward_y;active_request=&request;
    }
    std::uint32_t call(void* context=&weaponhand::client){response.query_id=999;
        const auto result=mainmountprovider::prepare(context,&request,&response);
        CHECK(callbacks.load()==0&&component_names::enabled&&scratch_empty());return result;}
};
bool caught_custom(wire::Request const* request,wire::Response* response) noexcept {
    __try{mainmountprovider::prepare(&weaponhand::client,request,response);return false;}
    __except(GetExceptionCode()==custom_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}
}
int main(){
    for(unsigned selected=0;selected<2;++selected){Trial t(selected);const auto original=t.request;
        CHECK(t.call()==unsigned(wire::Status::ready)&&reader_calls==1);
        const auto& out=t.response;CHECK(out.query_id==901&&wh::same(out.key,t.fixture.hand.key)&&out.calibration_known==1);
        CHECK(wh::same(out.world,t.fixture.hand.world)&&wh::same(out.controller,t.fixture.hand.controller)&&
            wh::same(out.pawn,t.fixture.hand.pawn)&&wh::same(out.mesh,t.fixture.hand.mesh)&&wh::same(out.weapon,t.fixture.hand.weapon));
        CHECK(out.component.address==0x7000&&out.asset.address==0x7100&&out.hold_generation==9&&out.body_revision==t.request.body.revision);
        CHECK(out.geometry==t.request.body.geometry&&out.units_per_metre==100&&out.provider_revision==wire::provider_revision);
        CHECK(out.basis==wire::Basis::marcus_mesh_face_forward_y&&out.basis_revision==2&&out.grip_to_wrist_rotation.w==1);
        CHECK(!std::memcmp(&t.request,&original,sizeof original));}
    for(auto mode:{ReaderMode::unheld,ReaderMode::busy}){Trial t;reader_mode=mode;
        CHECK(t.call()==unsigned(mode==ReaderMode::unheld?wire::Status::unheld:wire::Status::busy)&&empty(t.response));}
    for(unsigned reject=0;reject<5;++reject){Trial t;
        if(reject==1)weaponhand::on_gt=false;if(reject==2)enabled=false;if(reject==3)nativeequipment::domain=false;
        if(reject==4)weaponhand::client.armed_value=false;
        CHECK(t.call(reject==0?nullptr:&weaponhand::client)==unsigned(wire::Status::inactive));
        CHECK(reader_calls==0&&empty(t.response));if(reject<4)CHECK(copies==0);}
    {Trial t;reader_mode=ReaderMode::revoke;CHECK(t.call()==unsigned(wire::Status::inactive)&&reader_calls==1&&empty(t.response));}
    {Trial t;reader_mode=ReaderMode::soft_revoke;CHECK(t.call()==unsigned(wire::Status::inactive)&&reader_calls==1&&empty(t.response));}
    {Trial t;reader_mode=ReaderMode::reenter;CHECK(t.call()==unsigned(wire::Status::ready)&&empty(nested_response)&&reader_calls==1);}
    for(bool parser:{false,true}){Trial t;component_names::enabled=parser;reader_mode=ReaderMode::seh;
        CHECK(caught_custom(&t.request,&t.response));CHECK(callbacks.load()==0&&component_names::enabled==parser&&scratch_empty()&&empty(t.response));
        reader_mode=ReaderMode::ready;component_names::enabled=true;CHECK(t.call()==unsigned(wire::Status::ready));}
    {Trial t;unreadable=reinterpret_cast<uptr>(&t.request);CHECK(t.call()==~0u&&empty(t.response)&&reader_calls==0);}
    {Trial t;++t.request.size;CHECK(t.call()==unsigned(wire::Status::source_mismatch)&&empty(t.response)&&reader_calls==0);}
    {Trial t;CHECK(mainmountprovider::prepare(&weaponhand::client,&t.request,nullptr)==~0u&&callbacks.load()==0&&reader_calls==0);}
    {Trial t;weaponhand::on_gt=false;CHECK(mainmountprovider::attach(weaponhand::native_module)==wh::Reply::accepted);
        CHECK(resolver_calls==1&&attach_calls==1&&received_registration.context==&weaponhand::client&&
            received_registration.prepare==mainmountprovider::prepare&&received_registration.hand_cookie==61&&received_registration.native_started_qpc==51);}
    {Trial t;weaponhand::on_gt=false;weaponhand::client.armed_value=false;
        CHECK(mainmountprovider::attach(weaponhand::native_module)==wh::Reply::accepted&&attach_calls==1);}
    for(unsigned reject=0;reject<6;++reject){Trial t;weaponhand::on_gt=false;auto image=weaponhand::native_module;
        if(reject==0)weaponhand::on_gt=true;if(reject==1)image=reinterpret_cast<HMODULE>(uptr{0x8000});
        if(reject==2)enabled=false;if(reject==3)resolve_missing=true;if(reject==4)weaponhand::code_valid=false;
        if(reject==5)weaponhand::client.alive=false;
        CHECK(mainmountprovider::attach(image)==wh::Reply::unavailable&&attach_calls==0);}
    for(unsigned fault=1;fault<=4;++fault){Trial t;weaponhand::on_gt=false;receipt_fault=fault;
        CHECK(mainmountprovider::attach(weaponhand::native_module)==wh::Reply::invalid&&attach_calls==1);}
    {Trial t;weaponhand::on_gt=false;attach_reply=wh::Reply::busy;CHECK(mainmountprovider::attach(weaponhand::native_module)==wh::Reply::busy);}
    {Trial t;weaponhand::on_gt=false;attach_seh=true;CHECK(mainmountprovider::attach(weaponhand::native_module)==wh::Reply::unavailable);}
    {Trial t;weaponhand::on_gt=false;revoke_on_attach=true;
        CHECK(mainmountprovider::attach(weaponhand::native_module)!=wh::Reply::accepted);}
    mainmountprovider::receipt_count=0;mainmountprovider::receipt_attempts=0;
    {Trial t;const auto before=mainmountprovider::receipt_count;
        CHECK(t.call()==unsigned(wire::Status::ready));
        CHECK(mainmountprovider::receipt_count==before+1);const auto& r=mainmountprovider::receipts[before];
        CHECK(r.returned==1&&r.stage==5&&r.status==unsigned(wire::Status::ready)&&r.context_status==0&&r.retained_status==0&&r.response_status==0);
        CHECK(r.query==t.request.query_id&&r.cookie==61&&r.native_started==51&&r.lifecycle==t.request.hand_lifecycle);}
    {Trial t;mainmountprovider::receipt_count=0;++t.request.maximum_age_ns;
        CHECK(t.call()==unsigned(wire::Status::source_mismatch));const auto& r=mainmountprovider::receipts[0];
        CHECK(r.returned==1&&r.stage==2&&r.context_status==12&&r.retained_status==UINT32_MAX&&reader_calls==0);}
    {Trial t;mainmountprovider::receipt_count=0;reader_mode=ReaderMode::wrong_candidate;
        CHECK(t.call()==unsigned(wire::Status::source_mismatch));const auto& r=mainmountprovider::receipts[0];
        CHECK(r.returned==1&&r.stage==4&&r.context_status==0&&r.retained_status==0&&r.response_status==12);}
    {Trial t;mainmountprovider::receipt_count=0;reader_mode=ReaderMode::seh;
        CHECK(caught_custom(&t.request,&t.response));const auto& r=mainmountprovider::receipts[0];
        CHECK(r.returned==0&&r.stage==3&&r.finish_ns==0&&callbacks.load()==0&&scratch_empty());}
    {Trial t;mainmountprovider::receipt_count=0;reader_mode=ReaderMode::publish_fail;
        protected_response=static_cast<wire::Response*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        CHECK(protected_response);CHECK(mainmountprovider::prepare(&weaponhand::client,&t.request,protected_response)==~0u);
        const auto& r=mainmountprovider::receipts[0];CHECK(r.returned==1&&r.stage==5&&r.status==UINT32_MAX&&r.finish_ns>0);
        CHECK(callbacks.load()==0&&scratch_empty()&&empty(*protected_response));
        CHECK(VirtualFree(protected_response,0,MEM_RELEASE));protected_response=nullptr;}
    {Trial t;mainmountprovider::receipt_count=0;const auto before=mainmountprovider::receipt_attempts;
        for(unsigned i=0;i<34;++i)CHECK(t.call()==unsigned(wire::Status::ready));
        CHECK(mainmountprovider::receipt_count==32&&mainmountprovider::receipt_attempts==before+34);}
    {Trial t;mainmountprovider::receipt_count=0;CHECK(t.call()==unsigned(wire::Status::ready));
        mainmountprovider::reset_diagnostics_after_drain();CHECK(mainmountprovider::receipt_count==1);
        CHECK(tmpfile_s(&log_file)==0&&log_file);mainmountprovider::write_diagnostics_after_drain();CHECK(std::ftell(log_file)==0);
        enabled=false;weaponhand::native_module=nullptr;weaponhand::on_gt=false;
        mainmountprovider::write_diagnostics_after_drain();const auto bytes=std::ftell(log_file);CHECK(bytes>0);
        mainmountprovider::write_diagnostics_after_drain();CHECK(std::ftell(log_file)==bytes);
        CHECK(std::fclose(log_file)==0);log_file=nullptr;
        mainmountprovider::reset_diagnostics_after_drain();CHECK(mainmountprovider::receipt_count==0&&!mainmountprovider::diagnostics_written);}
    std::printf("WeaponMainMountProviderTest: %u checks passed; actual callback/attach wrapper, owned data and synthetic host only\n",checks);return 0;
}
