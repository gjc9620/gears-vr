#include "../vr-common/weapon_longshot_scope_view.h"
#include "../vr-common/weapon_longshot_scope_projection.h"
#include "../vr-common/weapon_longshot_eye_contact.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
using DWORD=unsigned long;
DWORD GetCurrentThreadId(){return 7;}
namespace hand=gears_weapon_hand;
struct alignas(8) Storage {std::array<unsigned char,1024> bytes{};};
Storage pawn,weapon,component,asset_storage,world_storage,controller,property,matrix_type;
std::vector<Storage*> storage{&pawn,&weapon,&component,&asset_storage,&world_storage,&controller,&property,&matrix_type};
std::uintptr_t request_at{};std::size_t request_bytes{};
template<class T>void put(Storage& s,unsigned offset,T const& value){std::memcpy(s.bytes.data()+offset,&value,sizeof value);}
std::uintptr_t address(Storage const& s){return reinterpret_cast<std::uintptr_t>(s.bytes.data());}
bool safe_copy(void* out,void const* in,std::size_t size){
    const auto at=reinterpret_cast<std::uintptr_t>(in);
    if(at==request_at&&size==request_bytes){std::memcpy(out,in,size);return true;}
    for(auto* s:storage)if(at>=address(*s)&&at-address(*s)<=1024&&size<=1024-(at-address(*s))){std::memcpy(out,in,size);return true;}
    return false;
}
hand::ObjectIdentity id(Storage const& s,unsigned slot){return {address(s),0,1000+8*slot,slot,slot,0,1};}
std::array<hand::ObjectIdentity,6> identities{id(world_storage,1),id(controller,2),id(pawn,3),id(weapon,4),id(component,5),id(asset_storage,6)};
bool admitted=true,metadata_live=true,proof_live=true;
namespace gears_anchor {struct Identity {std::uintptr_t address{},outer{},cls{};unsigned index{},name_index{},number{};const wchar_t* label{};};}
namespace weapon_hand_scene_binding {
std::mutex mutex;
struct Proof{};
struct Property {gears_anchor::Identity id{};unsigned offset{};};
struct ClassPath {std::array<gears_anchor::Identity,1> ids{};unsigned count{};};
bool metadata_current(){return metadata_live;}
bool current(Proof const&){return proof_live;}
bool identity(std::uintptr_t at,gears_anchor::Identity& out,bool=false){
    if(at==address(matrix_type)){out={at,0,0,0,0,0,L"Matrix"};return true;}
    if(at==999){out={999,0,0,0,0,0,L"WarfareGame"};return true;}
    if(at==998){out={998,0,0,0,0,0,L"Engine"};return true;}
    for(auto const& i:identities)if(i.address==at){out={i.address,i.outer,i.klass,i.registry_index,i.name_index,i.name_number,
        at==address(asset_storage)?L"LongShot":L"Object"};return true;}
    return false;
}
bool named(gears_anchor::Identity const& value,wchar_t const* label){return value.label&&!std::wcscmp(value.label,label);}
bool derives(std::uintptr_t,wchar_t const* type,std::uintptr_t,ClassPath& out){
    out.count=1;out.ids[0].label=type;out.ids[0].outer=!std::wcscmp(type,L"Weap_SniperRifle")?999:998;return true;
}
bool property(Proof&,std::uintptr_t,wchar_t const* label,wchar_t const*,unsigned,wchar_t const*,Property& out){
    out.id.address=address(::property);out.offset=!std::wcscmp(label,L"LocalToWorld")?64:
        !std::wcscmp(label,L"Instigator")?8:0;return true;
}
template<class T>bool source_field(Proof&,T& out,std::uintptr_t at,unsigned offset){return safe_copy(&out,reinterpret_cast<void const*>(at+offset),sizeof out);}
bool certified(Proof&,std::uintptr_t at,gears_anchor::Identity& out){return identity(at,out);}
}
namespace weapon_hand_runtime {
struct Broker {bool admit_weapon_render(hand::Frame const&){return admitted;}}broker;
Broker* current(){return &broker;}
}
#include "weapon_longshot_scope_view_live.inl"
int main(){
    namespace api=gears_weapon_longshot_scope_view;namespace view=longshot_scope_view_live;
    api::Request q{};q.source.world=identities[0];q.source.controller=identities[1];q.source.pawn=identities[2];q.source.weapon=identities[3];
    q.component=identities[4];q.asset=identities[5];q.source.key.world_generation=4;q.source.reference_generation=3;
    q.source.stereo.session=1;q.source.stereo.space_epoch=2;q.active=1;q.original_targeting_fov_degrees=20;
    put(pawn,0,address(weapon));put(weapon,0,address(component));put(weapon,8,address(pawn));put(component,0,address(asset_storage));
    put(property,0xd0,address(matrix_type));put(matrix_type,0x88,unsigned(64));
    std::array<float,16> m{1,0,0,0,0,0,1,0,0,-1,0,0,0,0,0,1};
    const auto c=gears_weapon::longshot_eye_contact::ocular_center_raw;
    // The measured ocular is exactly6cm ahead of the left eye; bore is+Z.
    m[12]=float(-3.2-c.x);m[13]=float(c.z);m[14]=float(6-c.y);put(component,64,m);
    q.model_world={{m[12],m[13],m[14]},{.7071067811865475,0,0,.7071067811865475}};q.model_scale=1;
    request_at=reinterpret_cast<std::uintptr_t>(&q);request_bytes=sizeof q;
    admitted=false;assert(!view::publish(&q));admitted=true;
    assert(view::publish(&q)==unsigned(api::Result::applied));
    gears_vr::StereoMatrices normal{};
    for(unsigned i=0;i<2;++i){
        normal.view[i]={1,0,0,0,0,1,0,0,0,0,1,0,i?-3.2f:3.2f,0,0,1};
        normal.projection[i]={1.2f,0,0,0,0,1.5f,0,0,0,0,.001f,1,0,0,5,0};
    }
    auto selected=normal;
    assert(view::select(4,3,q.source.stereo,100,selected));
    assert(selected.projection[0][0]>normal.projection[0][0]);
    assert(selected.view[0]==normal.view[0]&&selected.view[1]==normal.view[1]);
    const auto untouched=normal;
    auto reject=[&](){selected=normal;assert(!view::select(4,3,q.source.stereo,100,selected));
        assert(!std::memcmp(&selected,&untouched,sizeof selected));};
    admitted=false;reject();admitted=true;
    metadata_live=false;reject();metadata_live=true;
    proof_live=false;reject();proof_live=true;
    m[12]+=100;put(component,64,m);selected=normal;
    assert(view::select(4,3,q.source.stereo,100,selected)); // Ordinary component restoration does not move the held model.
    m[12]-=100;put(component,64,m);
    q.model_world.position.x+=100;assert(view::publish(&q));reject();q.model_world.position.x-=100;assert(view::publish(&q));
    q.version=1;assert(!view::publish(&q));q.version=2;
    put(pawn,0,address(asset_storage));reject();put(pawn,0,address(weapon));
    selected=normal;assert(!view::select(5,3,q.source.stereo,100,selected));
    assert(!view::select(4,8,q.source.stereo,100,selected));
    q.active=0;assert(view::publish(&q));reject();
    std::puts("Longshot Native view production adapter: admitted source, current object links/matrix/metadata, measured ocular/eye, world/reference rejection and default/disarmed neutrality passed; boundaries injected, no game rendering");
}
