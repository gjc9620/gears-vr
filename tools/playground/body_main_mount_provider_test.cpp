#include <windows.h>
#include <atomic>
#include <thread>
#include "../vr-common/weapon_hand_client.h"
#include "weapon_metadata_parse.h"
#include "body_mount_owned_fixture.h"
namespace {
using uptr=std::uintptr_t;
std::atomic<unsigned> callbacks{};std::atomic<bool> enabled{true};FILE* log_file{};
namespace weaponhand {
struct Client {
    bool alive=true,armed_value=true;std::uint64_t generation=1;
    std::uint64_t cookie() const noexcept{return 61;}
    std::uint64_t native_started_qpc() const noexcept{return 51;}
    bool armed() const noexcept{return armed_value;}
    std::uint64_t metadata_generation_on_gt() const noexcept{return alive?generation:0;}
    bool registration_alive(std::uint64_t c,std::uint64_t n) const noexcept{return alive&&c==61&&n==51;}
    template<class Reader>bool observe_equipment_on_gt(wh::EquipmentIdentity const& i,Reader& reader,wh::EquipmentObservation& out) noexcept {
        out={};if(!alive||!armed_value)return false;
        const auto current=[&]() noexcept{return alive&&armed_value;};
        if(!reader(i,current,out)){out={};return false;}
        out.begin_ns=fixture_now-1000000;out.end_ns=fixture_now;return true;
    }
    template<class Reader>bool observe_registered_equipment_on_gt(wh::EquipmentIdentity const& i,std::uint64_t epoch,
        Reader& reader,wh::EquipmentObservation& out) noexcept {
        out={};const auto current=[&]() noexcept{return alive&&epoch&&epoch==generation;};
        if(!current()||!reader(i,current,out)||!current()){out={};return false;}
        out.begin_ns=fixture_now-1000000;out.end_ns=fixture_now;return true;
    }
} client;
struct Held {wh::ObjectIdentity world{},controller{},pawn{},weapon{};std::uint64_t generation{},native_cookie{},native_started{},world_generation{};bool known{};} held_binding;
bool observe_equipment(wh::EquipmentIdentity const&,wh::GripGameState&) noexcept;
HMODULE native_module=reinterpret_cast<HMODULE>(uptr{0x7000});bool on_gt=true,code=true;
std::uint32_t current_gt(void*) noexcept{return on_gt?GetCurrentThreadId():0;}
std::int64_t clock_ns(void*) noexcept{return fixture_now;}
bool executable(FARPROC p,HMODULE m) noexcept{return p&&code&&m==native_module;}
}
namespace nativeequipment {
bool domain=true;bool domain_ready() noexcept{return domain;}
bool read_attached_equipment(wh::EquipmentIdentity const&,uptr) noexcept;
}
namespace vrequip {
struct Model {
    std::uint64_t generation=9;
    std::uint64_t hold_generation() const noexcept{return generation;}
    bool held_for(std::uint64_t,std::uint64_t,std::uint64_t) const noexcept{return true;}
} model;
}
namespace lancermain_asset {
enum class Status {ready,changed};struct Evidence {wh::ObjectIdentity component{},asset{};};
enum class AssetScope {lancer_only,basic_hold,supported_instant_fire,frag_hold};
Status read_equipment(wh::EquipmentIdentity const&,Evidence&,AssetScope) noexcept;
}
namespace mainmountprovider {
std::atomic_flag entered=ATOMIC_FLAG_INIT;
bool diagnostics_drained() noexcept{return !enabled.load()&&!callbacks.load()&&!weaponhand::native_module&&!weaponhand::current_gt(nullptr);}
}
uptr unreadable{};
bool copy(void* out,uptr from,std::size_t count) noexcept {
    if(!from||from==unreadable)return false;
    __try{std::memcpy(out,reinterpret_cast<void const*>(from),count);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
enum class Inject {none,unheld,revoke,soft_revoke,publication_loss,domain,reenter,seh,asset_changed,game_changed,generation};Inject injection{};
constexpr DWORD fault_code=0xe0424783;
unsigned game_reads{},asset_reads{},attachment_reads{};bool frag_visual{};
#include "body_mount_equipment.inl"
unsigned attach_calls{},receipt_fault{};bool attach_seh{},missing{},revoke_attach{};
wh::Reply attach_reply=wh::Reply::accepted;bodywire::Registration received{};
wh::Reply __cdecl fake_attach(bodywire::Registration const* r,wh::RegistrationResult* receipt) noexcept {
    ++attach_calls;received=*r;CHECK(bodywire::registration_shape_valid(*r));
    if(attach_seh)RaiseException(fault_code,0,0,nullptr);
    receipt->cookie=r->hand_cookie;receipt->native_started_qpc=r->native_started_qpc;
    if(receipt_fault==1)++receipt->size;if(receipt_fault==2)++receipt->abi_version;
    if(receipt_fault==3)++receipt->cookie;if(receipt_fault==4)++receipt->native_started_qpc;
    if(revoke_attach)weaponhand::client.alive=false;return attach_reply;
}
FARPROC fake_resolver(HMODULE image,LPCSTR name) noexcept {
    CHECK(image==weaponhand::native_module&&std::strcmp(name,bodywire::attach_export)==0);
    return missing?nullptr:reinterpret_cast<FARPROC>(fake_attach);
}
#define GetProcAddress fake_resolver
#include "body_main_mount_provider.inl"
#undef GetProcAddress
namespace nativeequipment {
bool read_attached_equipment(wh::EquipmentIdentity const&,uptr component) noexcept {
    ++attachment_reads;CHECK(game_reads==1&&asset_reads==1&&component==0x7000);
    CHECK(callbacks.load()==1&&weapon_metadata_parse::active==&bodymountprovider::scratch.parsing);
    CHECK(weapon_metadata_parse::length_bound(0xf000,17)==0);weapon_metadata_parse::remember_length(0xf000,17,7);
    if(injection==Inject::seh){bodymountprovider::scratch.parsing.walk_owned=true;RaiseException(fault_code,0,0,nullptr);}
    if(injection==Inject::revoke)weaponhand::client.alive=false;
    if(injection==Inject::soft_revoke){weaponhand::client.armed_value=false;++weaponhand::client.generation;}
    if(injection==Inject::publication_loss)weaponhand::client.armed_value=false;
    if(injection==Inject::domain)nativeequipment::domain=false;
    if(injection==Inject::generation)++vrequip::model.generation;
    const auto& request=bodymountprovider::scratch.request;
    if(injection==Inject::reenter){bodywire::Response nested{};const auto original=bodymountprovider::scratch.request.query_id;
        CHECK(bodymountprovider::prepare(&weaponhand::client,&request,&nested)==~0u&&empty(nested));
        CHECK(callbacks.load()==1&&bodymountprovider::scratch.request.query_id==original);
        CHECK(weapon_metadata_parse::length_bound(0xf000,17)==7);
        std::thread worker([&]{CHECK(weapon_metadata_parse::active==nullptr);
            CHECK(bodymountprovider::prepare(&weaponhand::client,&request,&nested)==~0u&&weapon_metadata_parse::active==nullptr);});worker.join();
    }
    return true;
}
}
namespace weaponhand {
bool observe_equipment(wh::EquipmentIdentity const& i,wh::GripGameState& out) noexcept {
    ++game_reads;CHECK(game_reads<=2&&wh::same(i.key,bodymountprovider::scratch.identity.key));
    if(game_reads==2)CHECK(asset_reads==2&&attachment_reads==1);
    out={};out.known=true;out.main_hand_held=injection!=Inject::unheld;out.kind=frag_visual?wh::WeaponKind::hand_thrown:wh::WeaponKind::firearm;
    out.busy=injection==Inject::game_changed&&game_reads==2;return true;
}
}
namespace lancermain_asset {
Status read_equipment(wh::EquipmentIdentity const&,Evidence& out,AssetScope scope) noexcept {
    CHECK(scope==(frag_visual?AssetScope::frag_hold:AssetScope::basic_hold));
    ++asset_reads;CHECK(game_reads==1&&asset_reads<=2);out.component=object(0x7000,8);out.asset=object(0x7100,9);
    if(injection==Inject::asset_changed&&asset_reads==2)++out.asset.name_number;return Status::ready;
}
}
bool clean(){const auto* p=reinterpret_cast<unsigned char const*>(&bodymountprovider::scratch);
    for(std::size_t i=0;i<sizeof(bodymountprovider::scratch);++i)if(p[i])return false;return true;}
struct Trial:BodyFixture {
    bodywire::Response out{};
    Trial(){
        enabled=true;nativeequipment::domain=weaponhand::on_gt=weaponhand::code=weaponhand::client.alive=weaponhand::client.armed_value=true;
        weaponhand::client.generation=1;
        injection=Inject::none;frag_visual=false;unreadable=0;receipt_fault=attach_calls=0;attach_seh=missing=revoke_attach=false;attach_reply=wh::Reply::accepted;
        game_reads=asset_reads=attachment_reads=0;vrequip::model.generation=9;
        auto& n=request.source.original_scene.native;n.game_pid=GetCurrentProcessId();n.game_thread=GetCurrentThreadId();
        request.source.tracking.thread=n.game_thread;parent.game_pid=n.game_pid;parent.game_thread=n.game_thread;
        CHECK(sender::context(request,parent,fixture_now,identity)==bodywire::Status::ready);
        weaponhand::held_binding={identity.world,identity.controller,identity.pawn,identity.weapon,9,61,51,identity.key.world_generation,true};
    }
    std::uint32_t call(void* context=&weaponhand::client){out.query_id=999;auto* previous=weapon_metadata_parse::active;
        const auto result=bodymountprovider::prepare(context,&request,&out);CHECK(clean()&&!callbacks.load()&&weapon_metadata_parse::active==previous);return result;}
};
bool catch_fault(Trial& t) noexcept {
    __try{bodymountprovider::prepare(&weaponhand::client,&t.request,&t.out);return false;}
    __except(GetExceptionCode()==fault_code?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}
}
int main(){
    {Trial t;frag_visual=true;CHECK(t.call()==unsigned(bodywire::Status::ready)&&t.out.hold_kind==bodywire::HoldKind::frag);}
    {Trial t;CHECK(t.call()==unsigned(bodywire::Status::ready)&&t.out.query_id==901&&t.out.mount.query_id==901);}
    {Trial t;weaponhand::client.armed_value=false;
        CHECK(t.call()==unsigned(bodywire::Status::ready)&&!weaponhand::client.armed());}
    {Trial t;injection=Inject::publication_loss;
        CHECK(t.call()==unsigned(bodywire::Status::ready)&&!weaponhand::client.armed());}
    for(auto mode:{Inject::unheld,Inject::revoke,Inject::soft_revoke,Inject::domain,Inject::asset_changed,Inject::game_changed,Inject::generation}){Trial t;injection=mode;
        CHECK(t.call()!=unsigned(bodywire::Status::ready)&&empty(t.out));}
    {Trial t;injection=Inject::reenter;CHECK(t.call()==unsigned(bodywire::Status::ready));}
    {Trial t;injection=Inject::seh;CHECK(catch_fault(t)&&clean()&&!callbacks.load()&&empty(t.out)&&weapon_metadata_parse::active==nullptr);
        injection=Inject::none;game_reads=asset_reads=attachment_reads=0;CHECK(t.call()==unsigned(bodywire::Status::ready));}
    {Trial t;CHECK(!mainmountprovider::entered.test_and_set());CHECK(t.call()==~0u&&empty(t.out));
        CHECK(mainmountprovider::entered.test_and_set());mainmountprovider::entered.clear();}
    {Trial t;CHECK(t.call(nullptr)==unsigned(bodywire::Status::inactive)&&empty(t.out));}
    {Trial t;++t.request.size;CHECK(t.call()==unsigned(bodywire::Status::source_mismatch)&&empty(t.out));}
    {Trial t;unreadable=reinterpret_cast<uptr>(&t.request);CHECK(t.call()==~0u&&empty(t.out));}
    {Trial t;weaponhand::on_gt=false;CHECK(bodymountprovider::attach(weaponhand::native_module)==wh::Reply::accepted&&attach_calls==1&&received.context==&weaponhand::client);}
    for(unsigned f=1;f<=4;++f){Trial t;weaponhand::on_gt=false;receipt_fault=f;CHECK(bodymountprovider::attach(weaponhand::native_module)==wh::Reply::invalid);}
    {Trial t;weaponhand::on_gt=false;attach_seh=true;CHECK(bodymountprovider::attach(weaponhand::native_module)==wh::Reply::unavailable);}
    {Trial t;weaponhand::on_gt=false;revoke_attach=true;CHECK(bodymountprovider::attach(weaponhand::native_module)==wh::Reply::draining);}
    std::printf("BodyMainMountProviderTest: %u checks; actual typed callback/sender/equipment orchestration, owned Game/asset/attachment/Client seams and parent-shared reentry/drain storage; no Game\n",checks);
}
