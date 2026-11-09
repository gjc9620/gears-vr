// Actual production cancellation adapter, bounded fake metadata/engine calls.
// These tests prove its refusals/order, not retail memory or native ABI.
#include <array>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cassert>
#include <cstdio>
#include <functional>
#include <type_traits>
#include "../vr-common/weapon_hand_participant.h"
#ifdef NDEBUG
#error Assertions required
#endif
using uptr=std::uint64_t;
constexpr uptr base=0x140000000;
struct Name {unsigned index{},number{};};
struct Array {uptr data{};int count{},capacity{};};
namespace fixture {
enum :unsigned {none,engine,core,weapon_owner,inventory_owner,object_owner,hb_owner,burst,
    function_type,class_type,state_type,byte_type,name_type,bool_type,goto_fn,
    manager_property,pending_property,inner_property,newstate,label,force,keep,inventory_base,hb_count,hb_limit,count};
constexpr uptr address(unsigned i){return i?0x1000+0x100*i:0;}
struct Model {
    bool gt{true},hold{true},registration{true},tables{true},objects{true},receiver{true},game{true},r2{},
        manager_class{true},retained{true},native_function{true},invoke{true},change_state{true},
        pending_race{},pointer_race{},inner_race{},field_race{},array_race{},read_fail{},post_pending{},
        native_source{true},owned_press{true};
    uptr manager{0xa000},world{0xb000},controller{0xc000},pawn{0xd000},weapon{0xe000};
    Name state{101,0},destination{};
    Array array{0xf000,2,2};std::array<unsigned char,256> bytes{};
    unsigned copies{},pointer_reads{},field_reads{},inner_reads{},array_reads{},go_calls{};
    unsigned burst_count{},burst_limit{6};std::int64_t now{1000000000};
} model;
const wchar_t* text(unsigned i){switch(i){
    case engine:return L"Engine";case core:return L"Core";case weapon_owner:return L"Weapon";
    case inventory_owner:return L"InventoryManager";case object_owner:return L"Object";
    case inventory_base:return L"Inventory";
    case hb_owner:return L"Weap_LocustAssaultRifle";case burst:return L"BurstDelaying";
    case function_type:return L"Function";case class_type:return L"Class";case state_type:return L"State";
    case byte_type:return L"IntProperty";case name_type:return L"NameProperty";case bool_type:return L"BoolProperty";
    case goto_fn:return L"GotoState";case newstate:return L"NewState";case label:return L"Label";
    case force:return L"bForceEvents";case keep:return L"bKeepStack";
    case 100:return L"WarfareGame";case 101:return L"WeaponFiring";case 102:return L"Active";
    default:return L"unknown";}}
}
namespace gripdamage_metadata {
constexpr unsigned max_fields=64;
enum class Stage {final_check};
struct Identity {uptr address{},outer{},klass{};Name name{};};
struct Property {unsigned identity{},type{},dimension{1},element_bytes{},offset{};std::uint64_t flags{0x90};};
struct Structure {uptr children{};unsigned extent{24};};
struct PointerField {Property property{};unsigned owner{};uptr value{};};
struct NameText {unsigned index{};};
struct Snapshot {
    uptr module_base{};unsigned objects{},names{},field_count{};
    std::array<Identity,128> identities{};std::array<Property,max_fields> fields{};
};
Snapshot template_snapshot;
bool same_property(Property const& a,Property const& b){return a.identity==b.identity&&a.type==b.type&&
    a.dimension==b.dimension&&a.element_bytes==b.element_bytes&&a.offset==b.offset&&a.flags==b.flags;}
bool property_fits(Property const& p,unsigned n){return p.offset<=n&&p.element_bytes<=n-p.offset;}
struct Reader {
    Snapshot& result;
    Reader(Snapshot& s,Stage):result(s){if(!s.identities[fixture::engine].address)s=template_snapshot;}
    bool path(uptr p,unsigned& slot){slot=0;for(unsigned i=1;i<result.identities.size();++i)
        if(result.identities[i].address==p){slot=i;return true;}return false;}
    bool named(unsigned slot,wchar_t const* name){return std::wcscmp(fixture::text(slot),name)==0;}
    bool property(uptr p,Property& out){unsigned slot{};if(!path(p,slot))return false;out=properties[slot];return true;}
    bool structure(uptr p,Structure& s){if(p!=fixture::address(fixture::goto_fn))return false;s=structure_value;return true;}
    template<class T>bool at(uptr p,unsigned offset,T& out){
        using namespace fixture;
        if constexpr(std::is_same_v<T,Array>){
            if(p!=model.manager||offset!=32)return false;out=model.array;
            if(model.array_race&&++model.array_reads>1)++out.capacity;return true;
        }else{
            std::uint64_t value{};
            if(p==model.weapon&&(offset==48||offset==49))value=offset==48?model.burst_count:model.burst_limit;
            else if(p==address(goto_fn)&&offset==0xf8)value=native_index;
            else if(p==address(goto_fn)&&offset==0x108)value=return_offset;
            else if(p==address(pending_property)&&offset==0xd0){value=address(inner_property);if(model.inner_race&&++model.inner_reads>1)++value;}
            else return false;
            out=static_cast<T>(value);return true;
        }
    }
    static std::array<Property,128> properties;
    static Structure structure_value;
    static unsigned native_index,return_offset;
};
std::array<Property,128> Reader::properties{};Structure Reader::structure_value{};
unsigned Reader::native_index=113,Reader::return_offset=0xffff;
bool tables(uptr,unsigned&,unsigned&){return fixture::model.tables;}
bool fresh_name(unsigned,unsigned i,NameText& out){out.index=i;return true;}
bool exact_text(NameText const& t,wchar_t const* name){return std::wcscmp(fixture::text(t.index),name)==0;}
}
namespace weaponhand {
using Id=gears_weapon_hand::ObjectIdentity;
struct HoldBinding {Id world{},controller{},pawn{},weapon{};
    std::uint64_t native_cookie{1},native_started{2},world_generation{3},generation{4};bool known{true};};
HoldBinding held_binding;
struct Client {bool registration_alive(std::uint64_t,std::uint64_t){return fixture::model.registration;}}client;
bool current_gt(void*){return fixture::model.gt;}
std::int64_t clock_ns(void*){return fixture::model.now;}
bool object(gripdamage_metadata::Reader&,Id const&){return fixture::model.objects;}
bool core_type(gripdamage_metadata::Reader& r,unsigned slot,wchar_t const* name){return r.named(slot,name);}
struct BoolField {gripdamage_metadata::Property property{};unsigned owner{};};
bool derives(gripdamage_metadata::Reader&,uptr,wchar_t const*,wchar_t const*){return fixture::model.manager_class;}
bool read_pointer(gripdamage_metadata::Reader&,uptr,wchar_t const*,gripdamage_metadata::PointerField& out){
    using namespace fixture;out.property=gripdamage_metadata::Reader::properties[manager_property];
    out.owner=inventory_base;out.value=model.manager;if(model.pointer_race&&++model.pointer_reads>1)++out.value;return true;
}
bool field(gripdamage_metadata::Reader&,uptr,wchar_t const*,wchar_t const*,unsigned,BoolField& out){
    using namespace fixture;out.property=gripdamage_metadata::Reader::properties[pending_property];out.owner=inventory_owner;
    if(model.field_race&&++model.field_reads>1)++out.property.offset;return true;
}
}
namespace playground {
struct Args {
    struct {uptr function{fixture::address(fixture::goto_fn)};unsigned size{24};}r;
    bool valid{true};
    Args(uptr,wchar_t const*){}
    uptr prop(wchar_t const* name){for(unsigned i=fixture::newstate;i<=fixture::keep;++i)
        if(!std::wcscmp(name,fixture::text(i)))return fixture::address(i);return 0;}
    Args& set(wchar_t const*,Name n){fixture::model.destination=n;return *this;}
    bool run(){auto& m=fixture::model;++m.go_calls;if(m.invoke&&m.change_state)m.state=m.destination;
        if(m.post_pending)m.bytes[0]=1;return m.invoke;}
};
uptr field(uptr,wchar_t const*,wchar_t const*){return fixture::address(fixture::burst);}
}
namespace weapon_part_assets {
bool native_function(playground::Args const&,wchar_t const*,wchar_t const*,wchar_t const*){return fixture::model.native_function;}
}
namespace unified_weapon_fire {bool retained_metadata(gripdamage_metadata::Reader&){return fixture::model.retained;}}
namespace vrfire {
namespace lr {struct Actors {gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},weapon{};};
struct Game {Actors actors{};bool known{},r2_known{},r2_held{};};}
struct Input {lr::Actors actors(){auto& h=weaponhand::held_binding;return {h.world,h.controller,h.pawn,h.weapon};}
    bool owns_press(){return fixture::model.owned_press;}}input;
bool read_game(lr::Actors const& a,void*,lr::Game& g){g={a,true,true,fixture::model.r2};return fixture::model.game;}
}
namespace lancerrefire {
bool same_hold(weaponhand::HoldBinding const&,weaponhand::HoldBinding const&){return fixture::model.hold;}
bool state_name(gripdamage_metadata::Reader&,uptr,Name& n){n=fixture::model.state;return true;}
namespace hammerburst_refire {bool receiver(gripdamage_metadata::Reader&,uptr){return fixture::model.receiver;}}
}
bool copy(void* out,uptr p,std::size_t n){auto& m=fixture::model;
    if(!m.read_fail&&n==1&&(p==m.weapon+48||p==m.weapon+49)){
        *static_cast<unsigned char*>(out)=static_cast<unsigned char>(p==m.weapon+48?m.burst_count:m.burst_limit);return true;}
    if(m.read_fail||p!=m.array.data||n>m.bytes.size())return false;
    std::memcpy(out,m.bytes.data(),n);if(m.pending_race&&++m.copies>1&&n)static_cast<unsigned char*>(out)[0]=1;return true;
}
#include "weapon_hammerburst_burst_cancel.inl"
gripdamage_metadata::Snapshot& snapshot=gripdamage_metadata::template_snapshot;
void reset(){
    using namespace fixture;using namespace gripdamage_metadata;
    model={};snapshot={};Reader::properties={};Reader::structure_value={};Reader::native_index=113;Reader::return_offset=0xffff;
    auto put=[](unsigned i,unsigned klass,unsigned outer){snapshot.identities[i]={address(i),address(outer),address(klass),{i,0}};};
    for(unsigned i=1;i<count;++i)put(i,0,0);
    put(weapon_owner,class_type,engine);put(inventory_owner,class_type,engine);put(inventory_base,class_type,engine);put(object_owner,class_type,core);
    put(hb_owner,class_type,100);put(100,0,0);put(burst,state_type,hb_owner);put(goto_fn,function_type,object_owner);
    for(unsigned i=newstate;i<=keep;++i){put(i,i<=label?name_type:bool_type,goto_fn);
        Reader::properties[i]={i,i<=label?name_type:bool_type,1,i<=label?8u:4u,i==newstate?0u:i==label?8u:i==force?16u:20u,0x90};}
    put(inner_property,byte_type,pending_property);Reader::properties[inner_property]={inner_property,byte_type,1,4,0,0};
    Reader::properties[manager_property]={manager_property,0,1,8,8,0};
    Reader::properties[pending_property]={pending_property,0,1,16,32,0};
    Reader::properties[hb_count]={hb_count,0,1,1,48,0};Reader::properties[hb_limit]={hb_limit,0,1,1,49,0};
    weaponhand::held_binding={};weaponhand::held_binding.world.address=model.world;
    weaponhand::held_binding.controller.address=model.controller;weaponhand::held_binding.pawn.address=model.pawn;
    weaponhand::held_binding.weapon.address=model.weapon;
    weaponhand::held_binding.weapon.klass=address(hb_owner);
}
#ifndef HB_CANCEL_FIXTURE_ONLY
int main(){
    using namespace fixture;using namespace gripdamage_metadata;unsigned checks{};
    const auto pending=[](){auto s=snapshot;Reader r{s,Stage::final_check};return hammerburst_burst_cancel::pending_up(r,model.weapon);};
    const auto layout=[](){auto s=snapshot;Reader r{s,Stage::final_check};playground::Args a(model.weapon,L"GotoState");return hammerburst_burst_cancel::goto_layout(r,a);};
    reset();assert(pending());++checks;
    for(unsigned mode=0;mode<2;++mode){reset();model.bytes[mode*4]=1;assert(!pending());assert(!model.go_calls);checks+=2;}
    const auto reject=[&](std::function<void()> f){reset();f();assert(!pending());assert(!model.go_calls);checks+=2;};
    reject([]{model.manager=0;});reject([]{model.manager_class=false;});
    reject([]{model.array.count=-1;});reject([]{model.array.capacity=1;});reject([]{model.array.capacity=65;});
    reject([]{model.array.data=0;});reject([]{model.read_fail=true;});reject([]{model.pending_race=true;});
    reject([]{model.pointer_race=true;});reject([]{model.inner_race=true;});reject([]{model.field_race=true;});
    reject([]{model.array_race=true;});reject([]{model.retained=false;});
    reject([]{Reader::properties[inner_property].element_bytes=1;});
    reject([]{Reader::properties[inner_property].offset=1;});
    reject([]{snapshot.identities[inner_property].outer=address(manager_property);});
    reject([]{snapshot.identities[inventory_owner].outer=address(core);});
    reset();model.array={};assert(pending());++checks;
    reset();assert(layout());++checks;
    for(unsigned p=newstate;p<=keep;++p)for(unsigned mutation=0;mutation<6;++mutation){
        reset();auto& v=Reader::properties[p];switch(mutation){case 0:v.dimension=2;break;
            case 1:++v.offset;break;case 2:++v.element_bytes;break;case 3:v.flags=0x80;break;
            case 4:v.type=byte_type;break;case 5:snapshot.identities[p].outer=address(weapon_owner);break;}
        assert(!layout());++checks;
    }
    reset();Reader::native_index=114;assert(!layout());++checks;
    reset();Reader::return_offset=0;assert(!layout());++checks;
    reset();Reader::structure_value.extent=28;assert(!layout());++checks;
    reset();model.native_function=false;assert(!layout());++checks;
    const auto complete=[](){return hammerburst_burst_cancel::complete(weaponhand::held_binding);};
    reset();assert(complete());assert(model.go_calls==1&&model.state.index==burst);checks+=2;
    reset();model.state={102,0};assert(complete());assert(!model.go_calls);checks+=2;
    for(unsigned failure=0;failure<12;++failure){reset();switch(failure){
        case 0:model.gt=false;break;case 1:model.hold=false;break;case 2:model.registration=false;break;
        case 3:model.tables=false;break;case 4:model.objects=false;break;case 5:model.receiver=false;break;
        case 6:model.game=false;break;case 7:model.r2=true;break;case 8:model.bytes[0]=1;break;
        case 9:model.native_function=false;break;case 10:model.retained=false;break;
        case 11:snapshot.identities[burst].outer=address(weapon_owner);break;}
        assert(!complete());assert(!model.go_calls);checks+=2;
    }
    for(unsigned failure=0;failure<3;++failure){reset();if(failure==0)model.invoke=false;
        if(failure==1)model.change_state=false;if(failure==2)model.post_pending=true;
        assert(!complete());assert(model.go_calls==1);checks+=2;
    }
    std::printf("HB actual cancellation adapter: %u fake boundary/order checks passed; not a game/ABI proof\n",checks);
}
#endif
