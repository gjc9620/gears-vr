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
uptr selected_weapon=112;
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
enum class Stage {function_path,final_check};
struct RawName {unsigned index{},number{};};
struct Identity {uptr address{},outer{},klass{};unsigned index{};RawName name{};};
struct Table {uptr data{};int count{},capacity{};};
struct Snapshot {uptr module_base{};Table objects{},names{};std::array<Identity,32> identities{};};
struct Structure {uptr children{};unsigned extent{};};
struct PointerField {};
struct NameText {};
struct Property {unsigned identity{},type{};uptr next{};std::uint64_t flags{};unsigned dimension{1},element_bytes{4},offset{};};
bool tables(uptr,Table&,Table&){return objects;}
bool fresh_name(Table const&,unsigned index,NameText&){return index==2;}
template<std::size_t N>bool exact_text(NameText const&,wchar_t const(&)[N]){return true;}
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
        else if(address==96)actual=owner_name==L"WEAP_HammerOfDawn"?L"warfaregamecontentweapons":L"WarfareGame";else if(address==160)actual=L"Function";
        else if(address==176)actual=L"Class";
        else if(address==256||address==272)actual=properties[(address-256)/16].name;
        else if(address==288)actual=L"NameProperty";else if(address==304)actual=L"ByteProperty";
        return actual==name;
    }
    bool structure(uptr at,Structure& out){out={256,parameter_bytes};return at==128;}
    bool property(uptr at,Property& out){
        if(at<256||at>=256+16*prop_count||at%16)return false;
        const auto& p=properties[(at-256)/16];unsigned id{},type{};
        if(!path(at,id)||!path(p.type==L"NameProperty"?288:304,type))return false;
        out={id,type,p.next,p.flags,1,p.type==L"NameProperty"?8u:1u,p.offset};return true;
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
    if(at==fixture_source.pawn.address&&!std::wcscmp(name,L"Weapon"))return selected_weapon;
    return 0;
}
}
namespace weaponhand {
bool current_gt(void*){return gt;}
bool object(gripdamage_metadata::Reader&,hand::ObjectIdentity const&){return objects;}
template<std::size_t N,std::size_t M>bool derives(gripdamage_metadata::Reader&,uptr,wchar_t const(&)[N],wchar_t const(&)[M]){return objects;}
template<std::size_t N>bool pointer(gripdamage_metadata::Reader&,uptr,wchar_t const(&)[N],uptr,gripdamage_metadata::PointerField&){return objects;}
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
namespace weaponshot {hand::ObjectIdentity identity(gripdamage_metadata::Identity const& id){return {id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};}}
namespace unified_weapon_fire {void schema_fingerprint(gripdamage_metadata::Snapshot const&,Fingerprint&) {}}
struct Name {unsigned index{},number{};};
bool invoke(Request* request){
    ++calls;last=*request;
    if(method_name==L"GetTargetingFOV")std::memcpy(request->params+return_offset,&returned_fov,sizeof returned_fov);
    return dispatch_ok;
}
#include "weapon_hod_original.inl"
namespace api=hod_original;
void configure(api::Method method){
    method_name=api::label(method);const bool end=method==api::Method::end_fire;
    owner_name=end?L"WEAP_HammerOfDawn":L"WarPC";
    prop_count=1;parameter_bytes=end?1:8;return_offset=0xffff;
    properties={};properties[0]={end?L"FireModeNum":L"ButtonName",end?L"ByteProperty":L"NameProperty",0,0x80,0};
}
hand::ObjectIdentity id(uptr at){return {at,0,16,unsigned(at/16),unsigned(at/16),0,1};}
int main(){
    fixture_source.world=id(32);fixture_source.controller=id(48);fixture_source.pawn=id(64);fixture_source.weapon=id(112);
    for(auto method:{api::Method::press,api::Method::release,api::Method::end_fire}){
        configure(method);const bool end=method==api::Method::end_fire;
        const auto receiver=end?fixture_source.weapon:fixture_source.controller;
        api::Entry entry{};assert(api::bind(receiver,method,entry));bool sent{};
        assert(api::execute(fixture_source,entry,{2,0},sent)&&sent);
        assert(last.object==receiver.address&&last.size==parameter_bytes);
        if(end)assert(last.params[0]==0);else {Name n{};std::memcpy(&n,last.params,8);assert(n.index==2&&n.number==0);}
        auto before=calls;live=false;
        assert(!api::execute(fixture_source,entry,{2,0},sent)&&!sent&&calls==before);
        hand::ExistingActionLifetime old{};
        const auto stopped=api::execute(fixture_source,entry,{2,0},sent,&old);
        assert(stopped==(method!=api::Method::press));
        lifetime=false;before=calls;assert(!api::execute(fixture_source,entry,{2,0},sent,&old)&&!sent&&calls==before);
        lifetime=true;live=true;
        ++fixture_script_hash;before=calls;assert(!api::execute(fixture_source,entry,{2,0},sent)&&!sent&&calls==before);--fixture_script_hash;
        owner_name=L"OtherWeapon";assert(!api::bind(receiver,method,entry));configure(method);
        properties[0].flags=0x180;assert(!api::bind(receiver,method,entry));configure(method);
        properties[0].offset=parameter_bytes;assert(!api::bind(receiver,method,entry));configure(method);
        properties[0].next=256;assert(!api::bind(receiver,method,entry));configure(method);
        native_index=1;assert(!api::bind(receiver,method,entry));native_index=0;
        package_outer=16;assert(!api::bind(receiver,method,entry));package_outer=0;
        assert(api::bind(receiver,method,entry));
        selected_weapon=144;before=calls;
        assert(!api::execute(fixture_source,entry,{2,0},sent,&old)&&!sent&&calls==before);
        selected_weapon=112;
        if(!end){before=calls;assert(!api::execute(fixture_source,entry,{3,0},sent)&&!sent&&calls==before);
            assert(!api::execute(fixture_source,entry,{2,1},sent)&&!sent&&calls==before);}
    }
    std::puts("HOD original adapter boundaries passed (synthetic metadata/dispatcher only)");
}
