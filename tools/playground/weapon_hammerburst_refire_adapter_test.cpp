// Actual production adapter with bounded metadata fakes. Engine certificate
// and memory lifetimes are stubs here, not live original-function proof.
#include "../vr-common/weapon_hammerburst_refire.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <map>
#include <functional>
#ifdef NDEBUG
#error Assertions required
#endif
using uptr=std::uint64_t;
constexpr uptr base=0x140000000;
namespace fixture {
enum :unsigned {none,timer,state,owner,package,fn_type,state_type,class_type,bool_type,
    engine_owner,engine_state,engine_timer,put,should,ammo,timing,down,target,returned,burst,war_owner,count};
struct Object {uptr address{},outer{},klass{};gears_hammerburst_refire::Name name{};std::wstring label;};
struct Structure {uptr children{};unsigned extent{};};
struct Property {unsigned identity{},type{};std::uint64_t flags{};unsigned dimension{1},element_bytes{4},offset{};};
struct Model {
    std::array<Object,count> identities{};
    std::array<Structure,count> structures{};
    std::array<Property,count> properties{};
    std::array<unsigned,count> sizes{},results{},native{},flags{};
    std::array<uptr,count> exec{};
    std::array<unsigned char,79> code{};
    bool receiver{true},bools{true},args{true},body{true},unchanged{true},retained{true},engine_proof{true};
    uptr engine_state_link{},engine_timer_link{},burst_link{};
    unsigned engine_calls{};
} model;
unsigned slot(uptr p){for(unsigned i=1;i<count;++i)if(model.identities[i].address==p)return i;return 0;}
uptr address(unsigned i){return model.identities[i].address;}
void reset(){
    model={};
    for(unsigned i=1;i<count;++i){model.identities[i].address=0x1000+i*0x100;model.results[i]=0xffff;model.exec[i]=base+0x1d6ca0;}
    auto set=[](unsigned i,wchar_t const* label,unsigned klass,unsigned outer,unsigned name=1){
        auto& o=model.identities[i];o.label=label;o.klass=address(klass);o.outer=address(outer);o.name={name,0};};
    set(fn_type,L"Function",0,0);set(state_type,L"State",0,0);set(class_type,L"Class",0,0);set(bool_type,L"BoolProperty",0,0);
    set(package,L"WarfareGame",0,0);set(owner,L"Weap_LocustAssaultRifle",class_type,package);
    set(war_owner,L"WarWeapon",class_type,package);
    set(state,L"WeaponFiring",state_type,owner);set(timer,L"RefireCheckTimer",fn_type,state);
    set(engine_owner,L"Weapon",class_type,0);set(engine_state,L"WeaponFiring",state_type,engine_owner);
    set(engine_timer,L"RefireCheckTimer",fn_type,engine_state);
    set(put,L"PutDownWeapon",fn_type,war_owner,23997);set(should,L"ShouldRefire",fn_type,owner,27558);
    set(ammo,L"FireAmmunition",fn_type,owner,13002);set(timing,L"TimeWeaponFiring",fn_type,engine_owner);
    set(down,L"bWeaponPutDown",bool_type,engine_owner);set(target,L"bWasTargeting",bool_type,owner);
    set(returned,L"ReturnValue",bool_type,should);
    set(burst,L"BurstDelaying",state_type,owner,4581);
    model.properties[down]={down,bool_type,0,1,4,0};model.properties[target]={target,bool_type,0,1,4,4};
    model.properties[returned]={returned,bool_type,0x580,1,4,0};model.sizes[should]=4;model.results[should]=0;
    model.structures[should]={address(returned),4};
    model.engine_state_link=address(engine_state);model.engine_timer_link=address(engine_timer);model.burst_link=address(burst);
    const unsigned char raw[]={0x07,0x19,0x00,0x2d,0x01,0xcf,0xfe,0xff,0xff,0x1b,0xbd,0x5d,0,0,0,0,0,0,0x16,0x04,0x0b,0x07,0x32,0,0x1b,0xa6,0x6b,0,0,0,0,0,0,0x16,0x1b,0xca,0x32,0,0,0,0,0,0,0x16,0x04,0x0b,0x14,0x2d,0x01,0x32,0x94,0,0,0x28,0x71,0x21,0xe5,0x11,0,0,0,0,0,0,0x50,0x50,0x50,0x16,0x04,0x0b,0x53};
    static_assert(sizeof raw==71);
    auto* p=model.code.data();std::memcpy(p,raw,5);p+=5;auto v=address(down);std::memcpy(p,&v,8);p+=8;
    std::memcpy(p,raw+9,40);p+=40;v=address(target);std::memcpy(p,&v,8);p+=8;std::memcpy(p,raw+53,18);
}
}
namespace gripdamage_metadata {
using Structure=fixture::Structure;using Property=fixture::Property;
bool property_fits(Property const& p,unsigned bytes){return p.dimension==1&&p.offset<=bytes&&p.element_bytes<=bytes-p.offset;}
struct Reader {
    fixture::Model& result{fixture::model};
    bool path(uptr p,unsigned& i){
        i=fixture::slot(p);return i!=0;
    }
    bool named(unsigned i,wchar_t const* label){return i<fixture::count&&result.identities[i].label==label;}
    bool structure(uptr p,Structure& out){const auto i=fixture::slot(p);if(!i)return false;out=result.structures[i];return true;}
    bool property(uptr p,Property& out){const auto i=fixture::slot(p);if(!i)return false;out=result.properties[i];return out.identity!=0;}
    template<class T>bool at(uptr p,unsigned offset,T& out){
        const auto i=fixture::slot(p);if(!i)return false;uptr v{};
        switch(offset){case 0x106:v=result.sizes[i];break;case 0x108:v=result.results[i];break;
            case 0xf8:v=result.native[i];break;case 0xf4:v=result.flags[i];break;case 0x114:v=result.exec[i];break;default:return false;}
        out=static_cast<T>(v);return true;
    }
};
}
namespace weaponhand {
struct BoolField {gripdamage_metadata::Property property{};};
bool core_type(gripdamage_metadata::Reader& r,unsigned i,wchar_t const* name){return r.named(i,name);}
bool boolean(gripdamage_metadata::Reader& r,uptr,wchar_t const* name,BoolField& out){
    if(!fixture::model.bools)return false;const auto i=std::wstring(name)==L"bWeaponPutDown"?fixture::down:fixture::target;
    out.property=fixture::model.properties[i];return r.named(out.property.type,L"BoolProperty")&&out.property.dimension==1&&out.property.element_bytes==4;
}
}
namespace lancermain_asset {
enum class AssetScope {lancer_only,supported_instant_fire};enum class HeldAsset {unknown,lancer,hammerburst};
HeldAsset held_asset_name(gripdamage_metadata::Reader&,uptr,AssetScope scope){
    return scope==AssetScope::supported_instant_fire&&fixture::model.receiver?HeldAsset::hammerburst:HeldAsset::unknown;
}
}
namespace playground {
struct Args {
    bool valid{fixture::model.args};struct {uptr function{};unsigned size{};}r;
    Args(uptr,wchar_t const* name){for(unsigned i=1;i<fixture::count;++i)if(fixture::model.identities[i].label==name){r.function=fixture::address(i);r.size=fixture::model.sizes[i];return;}valid=false;}
    uptr prop(wchar_t const*)const{return fixture::address(fixture::returned);}
};
uptr field(uptr object,wchar_t const* name,wchar_t const*){
    const std::wstring label(name);
    if(object==fixture::address(fixture::owner)&&label==L"BurstDelaying")return fixture::model.burst_link;
    if(object==fixture::address(fixture::engine_owner)&&label==L"WeaponFiring")return fixture::model.engine_state_link;
    if(object==fixture::address(fixture::engine_state)&&label==L"RefireCheckTimer")return fixture::model.engine_timer_link;
    return 0;
}
}
namespace unified_weapon_fire {bool retained_metadata(gripdamage_metadata::Reader&){return fixture::model.retained;}}
namespace lancerrefire {
namespace meta=gripdamage_metadata;
struct Array {uptr data{};int count{},capacity{};};struct Script {Array array{};std::array<unsigned char,256> bytes{};};
bool script(meta::Reader&,uptr fn,Script& out){if(!fixture::model.body||fn!=fixture::address(fixture::timer))return false;out.array.count=79;std::memcpy(out.bytes.data(),fixture::model.code.data(),79);return true;}
bool unchanged(meta::Reader&,uptr,Script const&){return fixture::model.unchanged;}
template<std::size_t N>bool weapon_function(meta::Reader& r,uptr fn,wchar_t const(&label)[N],unsigned& slot,meta::Structure& s,bool allow_war=false){
    return r.path(fn,slot)&&r.named(slot,label)&&fixture::model.identities[slot].klass==fixture::address(fixture::fn_type)&&
        (fixture::model.identities[slot].outer==fixture::address(fixture::engine_owner)||
         (allow_war&&fixture::model.identities[slot].outer==fixture::address(fixture::war_owner)))&&r.structure(fn,s);
}
bool original_loop(meta::Reader&,uptr,uptr fn,unsigned& reason){++fixture::model.engine_calls;reason=0;return fn==fixture::address(fixture::engine_timer)&&fixture::model.engine_proof;}
#include "weapon_hammerburst_refire.inl"
}
int main(){
    using namespace fixture;unsigned checks{};
    const auto run=[](){gripdamage_metadata::Reader r;unsigned reason{};
        return lancerrefire::hammerburst_refire::original_loop(r,0x9999,address(timer),reason);};
    reset();assert(run()&&model.engine_calls==1);++checks;
    reset();model.identities[put].outer=address(engine_owner);assert(run());++checks;
    const auto reject=[&](std::function<void()> const& change){reset();change();assert(!run());++checks;};
    reject([]{model.receiver=false;});
    reject([]{model.identities[timer].label=L"OtherTimer";});
    reject([]{model.identities[timer].klass=address(class_type);});
    reject([]{model.identities[state].label=L"OtherState";});
    reject([]{model.identities[state].klass=address(fn_type);});
    reject([]{model.identities[owner].label=L"ForeignHB";});
    reject([]{model.identities[owner].klass=address(fn_type);});
    reject([]{model.identities[package].label=L"OtherPackage";});
    reject([]{model.identities[package].outer=address(owner);});
    reject([]{model.sizes[timer]=1;});
    reject([]{model.results[timer]=0;});
    reject([]{model.native[timer]=1;});
    reject([]{model.flags[timer]=0x400;});
    reject([]{++model.exec[timer];});
    reject([]{model.structures[timer].children=address(returned);});
    reject([]{model.structures[timer].extent=4;});
    reject([]{model.bools=false;});
    reject([]{model.identities[target].outer=address(engine_owner);});
    reject([]{model.properties[target].type=fn_type;});
    reject([]{model.args=false;});
    reject([]{model.identities[put].klass=address(state_type);});
    reject([]{model.identities[put].outer=address(owner);});
    reject([]{model.identities[should].klass=address(state_type);});
    reject([]{model.identities[should].outer=address(engine_owner);});
    reject([]{model.identities[ammo].klass=address(state_type);});
    reject([]{model.identities[ammo].outer=address(engine_owner);});
    reject([]{model.sizes[should]=0;});
    reject([]{model.results[should]=0xffff;});
    reject([]{model.native[should]=1;});
    reject([]{model.flags[should]=0x400;});
    reject([]{model.structures[should].extent=0;});
    reject([]{model.sizes[ammo]=4;});
    reject([]{model.structures[ammo].children=address(returned);});
    reject([]{model.structures[ammo].extent=4;});
    reject([]{model.identities[returned].outer=address(ammo);});
    reject([]{model.properties[returned].type=fn_type;});
    reject([]{model.properties[returned].flags=0x80;});
    reject([]{model.properties[returned].dimension=2;});
    reject([]{model.properties[returned].element_bytes=8;});
    reject([]{model.properties[returned].offset=4;});
    reject([]{model.burst_link=0;});
    reject([]{model.identities[burst].outer=address(engine_owner);});
    reject([]{model.identities[burst].klass=address(fn_type);});
    reject([]{model.identities[burst].label=L"OtherState";});
    reject([]{model.body=false;});
    reject([]{model.code[1]^=1;});
    reject([]{model.identities[timing].outer=address(owner);});
    reject([]{model.engine_state_link=0;});
    reject([]{model.engine_timer_link=address(timer);});
    reject([]{model.engine_proof=false;});
    reject([]{model.retained=false;});
    reject([]{model.unchanged=false;});
    std::printf("HB actual timer adapter: %u metadata/refusal cases passed; Engine and memory are fakes, not live proof\n",checks);
}
