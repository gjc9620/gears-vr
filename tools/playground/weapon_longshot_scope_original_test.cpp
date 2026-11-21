// Runs the production adapter, injecting original metadata/dispatcher and
// fixture_source/lifetime boundaries. This is not a retail metadata or game proof.
#include "../vr-common/weapon_hand_participant.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <type_traits>
using uptr=std::uintptr_t;constexpr uptr base=0x100000;
namespace hand=gears_weapon_hand;
namespace gears_weapon_hand {struct ExistingActionLifetime {};}
struct Request {uptr object{},function{};unsigned size{};unsigned char params[4096]{};};
bool live=true,lifetime=true,quiet=true,gt=true,objects=true,retained=true,dispatch_ok=true;
unsigned calls{};float returned_fov=20;Request last{};
hand::Frame fixture_source{};
std::wstring method_name=L"SetTargetingMode",owner_name=L"WarPC";
unsigned parameter_bytes=4;std::uint16_t return_offset=0xffff,native_index{};
unsigned function_flags{},mask=4;uptr exec_address=base+0x1d6ca0,package_outer{};
std::uint64_t fixture_script_hash=1234;
struct Prop {std::wstring name,type;unsigned offset{};std::uint64_t flags{};uptr next{};};
std::array<Prop,2> properties{};unsigned prop_count=1;
namespace gripdamage_metadata {
constexpr unsigned max_fields=64;
enum class Stage {function_path};
struct RawName {unsigned index{},number{};};
struct Identity {uptr address{},outer{},klass{};unsigned index{};RawName name{};};
struct Table {uptr data{};int count{},capacity{};};
struct Snapshot {uptr module_base{};Table objects{},names{};std::array<Identity,32> identities{};};
struct Structure {uptr children{};unsigned extent{};};
struct Property {unsigned identity{},type{};uptr next{};std::uint64_t flags{};unsigned dimension{1},element_bytes{4},offset{};};
bool tables(uptr,Table&,Table&){return objects;}
bool property_fits(Property const& p,unsigned bytes){return p.offset<=bytes&&p.element_bytes<=bytes-p.offset;}
struct Reader {
    Snapshot& result;Stage stage;
    bool path(uptr address,unsigned& slot){
        if(!address||address%16||address/16>=result.identities.size())return false;
        slot=unsigned(address/16);auto& id=result.identities[slot];id={address,0,16,slot,{slot,0}};
        if(address==128){id.outer=80;id.klass=160;}
        if(address==80){id.outer=96;id.klass=176;}
        if(address==96)id.outer=package_outer;
        if(address==256||address==272)id.outer=128;
        return true;
    }
    template<std::size_t N>bool named(unsigned slot,wchar_t const(&name)[N])const{
        const auto address=result.identities[slot].address;
        std::wstring actual;
        if(address==128)actual=method_name;else if(address==80)actual=owner_name;
        else if(address==96)actual=L"WarfareGame";else if(address==160)actual=L"Function";
        else if(address==176)actual=L"Class";
        else if(address==256||address==272)actual=properties[(address-256)/16].name;
        else if(address==288)actual=L"BoolProperty";else if(address==304)actual=L"FloatProperty";
        return actual==name;
    }
    bool structure(uptr at,Structure& out){out={256,parameter_bytes};return at==128;}
    bool property(uptr at,Property& out){
        if(at<256||at>=256+16*prop_count||at%16)return false;
        const auto& p=properties[(at-256)/16];unsigned id{},type{};
        if(!path(at,id)||!path(p.type==L"BoolProperty"?288:304,type))return false;
        out={id,type,p.next,p.flags,1,4,p.offset};return true;
    }
    template<class T>bool at(uptr address,unsigned offset,T& out){
        const auto assign=[&](auto value){if constexpr(sizeof(T)==sizeof(value)){std::memcpy(&out,&value,sizeof out);return true;}else return false;};
        if(address==128){
            if(offset==0x106)return assign(std::uint16_t(parameter_bytes));
            if(offset==0x108)return assign(return_offset);
            if(offset==0xf4)return assign(function_flags);
            if(offset==0xf8)return assign(native_index);
            if(offset==0x114)return assign(exec_address);
            if(offset==0x8c)return assign(Table{4096,32,32});
        }
        return (address==256||address==272)&&offset==0xd0&&assign(mask);
    }
};
}
namespace playground {
struct Args {Request r{};bool valid{};Args(uptr object,const wchar_t*){r.object=object;r.function=128;r.size=parameter_bytes;valid=true;}};
uptr pointer(uptr at,const wchar_t* name){
    if(at==fixture_source.controller.address&& !std::wcscmp(name,L"Pawn"))return fixture_source.pawn.address;
    if(at==fixture_source.pawn.address&&!std::wcscmp(name,L"WorldInfo"))return fixture_source.world.address;
    if(at==fixture_source.weapon.address&&!std::wcscmp(name,L"Instigator"))return fixture_source.pawn.address;
    if(at==fixture_source.pawn.address&&!std::wcscmp(name,L"Weapon"))return fixture_source.weapon.address;
    return 0;
}
}
namespace weaponhand {
bool current_gt(void*){return gt;}
bool object(gripdamage_metadata::Reader&,hand::ObjectIdentity const&){return objects;}
template<std::size_t N>bool core_type(gripdamage_metadata::Reader& r,unsigned slot,wchar_t const(&name)[N]){return r.named(slot,name);}
struct Client {bool existing_action_lifetime_on_gt(hand::ExistingActionLifetime const&){return lifetime;}}client;
}
namespace weapon_part_assets {bool object(hand::ObjectIdentity const&){return objects;}}
namespace nativeequipment {bool current(void*,hand::Frame const&){return live;}bool restoration_quiet(){return quiet;}}
namespace weaponshot {bool hash_script(uptr,unsigned,std::uint64_t& out){out=fixture_script_hash;return true;}}
namespace unified_weapon_fire {
bool retained_metadata(gripdamage_metadata::Reader&){return retained;}
struct Fingerprint {std::uint64_t value=1469598103934665603ull;void word(std::uint64_t v){value^=v;value*=1099511628211ull;}};
}
bool invoke(Request* request){
    ++calls;last=*request;
    if(method_name==L"GetTargetingFOV")std::memcpy(request->params+return_offset,&returned_fov,sizeof returned_fov);
    return dispatch_ok;
}
#include "weapon_longshot_scope_original.inl"
namespace api=longshot_scope_original;
void configure(api::Method method){
    method_name=api::label(method);owner_name=method==api::Method::target?L"WarPC":L"Weap_SniperRifle";
    prop_count=method==api::Method::fov?2:1;parameter_bytes=4*prop_count;return_offset=prop_count==2?4:0xffff;
    properties={};properties[0]={method==api::Method::target?L"bNewIsTargeting":method==api::Method::zoom?L"bNewZoomed":
        method==api::Method::visibility?L"bNewOwnerSee":L"BaseFOV",prop_count==2?L"FloatProperty":L"BoolProperty",0,0x80,prop_count==2?272u:0u};
    if(prop_count==2)properties[1]={L"ReturnValue",L"FloatProperty",4,0x580,0};
}
hand::ObjectIdentity id(uptr at){return {at,0,16,unsigned(at/16),unsigned(at/16),0,1};}
int main(){
    fixture_source.world=id(32);fixture_source.controller=id(48);fixture_source.pawn=id(64);fixture_source.weapon=id(112);
    for(auto method:{api::Method::target,api::Method::zoom,api::Method::visibility}){
        configure(method);api::Entry entry{};const auto receiver=method==api::Method::target?fixture_source.controller:fixture_source.weapon;
        assert(api::bind(receiver,method,entry));bool dispatched{};
        assert(api::set(fixture_source,entry,true,&dispatched)&&dispatched&&last.object==receiver.address);
        unsigned value{};std::memcpy(&value,last.params,4);assert(value==mask);
        auto prior=calls;live=false;assert(!api::set(fixture_source,entry,true,&dispatched)&&!dispatched&&calls==prior);
        hand::ExistingActionLifetime old{};
        assert(!api::set(fixture_source,entry,true,&dispatched,&old)&&!dispatched&&calls==prior);
        assert(api::set(fixture_source,entry,false,&dispatched,&old)&&dispatched);std::memcpy(&value,last.params,4);assert(value==0);
        lifetime=false;prior=calls;assert(!api::set(fixture_source,entry,false,&dispatched,&old)&&calls==prior);lifetime=true;live=true;
        ++fixture_script_hash;prior=calls;assert(!api::set(fixture_source,entry,true,&dispatched)&&!dispatched&&calls==prior);--fixture_script_hash;
        owner_name=L"OtherWeapon";assert(!api::bind(receiver,method,entry));configure(method);
        mask=3;assert(!api::bind(receiver,method,entry));mask=4;
        properties[0].flags=0x180;assert(!api::bind(receiver,method,entry));configure(method);
        properties[0].offset=4;assert(!api::bind(receiver,method,entry));configure(method);
        properties[0].next=256;assert(!api::bind(receiver,method,entry));configure(method);
        native_index=1;assert(!api::bind(receiver,method,entry));native_index=0;
        package_outer=16;assert(!api::bind(receiver,method,entry));package_outer=0;
    }
    configure(api::Method::fov);api::Entry getter{};assert(api::bind(fixture_source.weapon,api::Method::fov,getter));
    float output{};assert(api::fov(fixture_source,getter,85,output)&&output==20);float base_fov{};std::memcpy(&base_fov,last.params,4);assert(base_fov==85);
    assert(!api::set(fixture_source,getter,true));
    returned_fov=0;assert(!api::fov(fixture_source,getter,85,output)&&output==0);
    returned_fov=NAN;assert(!api::fov(fixture_source,getter,85,output));returned_fov=20;
    auto prior=calls;assert(!api::fov(fixture_source,getter,0,output)&&calls==prior);
    properties[1].offset=0;assert(!api::bind(fixture_source.weapon,api::Method::fov,getter));configure(api::Method::fov);
    properties[0].name=L"GuessedFOV";assert(!api::bind(fixture_source.weapon,api::Method::fov,getter));
    std::puts("Longshot original scope production adapter: typed stock declarations, argument layout, script identity, cycles, masks, current fixture_source, release-only lifetime and returned FOV checks passed; metadata/dispatcher boundaries injected");
}
